/********************************************************************\

  Name:         xmlServer.cxx
  Created by:   Konstantin Olchanski

  Contents:     Serve XML-encoded ROOT objects through a TCP socket

  $Id$

\********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "xmlServer.h"

/*==== ROOT socket histo server ====================================*/

#if 1
#define THREADRETURN
#define THREADTYPE void
#endif
#if defined( OS_WINNT )
#define THREADRETURN 0
#define THREADTYPE DWORD WINAPI
#endif
#ifndef THREADTYPE
#define THREADTYPE int
#define THREADRETURN 0
#endif

#include <TROOT.h>
#include <TClass.h>
#include <TDirectory.h>
#include <TKey.h>
#include <TFolder.h>
#include <TSocket.h>
#include <TServerSocket.h>
#include <TThread.h>
#include <TMessage.h>
#include <TObjString.h>
#include <TH1.h>
#include <TCutG.h>
#include <TBufferXML.h>

#include <deque>
#include <map>
#include <string>

//#include "RootLock.h"
class LockRootGuard { public: int i; void Unlock() {}; };

static bool gVerbose = false;

static std::deque<std::string> gExports;
static std::map<std::string,std::string> gExportNames;

/*------------------------------------------------------------------*/

static TObject* FollowPath(TObject* container, char* path)
{
  if (0)
    printf("Follow path [%s] in container %p\n", path, container);

  while (1)
    {
      while (*path == '/')
        path++;

      char* s = strchr(path,'/');

      if (s)
        *s = 0;

      TObject *obj = NULL;

      if (container->InheritsFrom(TDirectory::Class()))
        obj = ((TDirectory*)container)->FindObject(path);
      else if (container->InheritsFrom(TFolder::Class()))
        obj = ((TFolder*)container)->FindObject(path);
      else if (container->InheritsFrom(TCollection::Class()))
        obj = ((TCollection*)container)->FindObject(path);
      else
        {
          printf("ERROR: Container \'%s\' of type %s is not a TDirectory, TFolder or TCollection\n", container->GetName(), container->IsA()->GetName());
          return NULL;
        }

      if (!s)
        return obj;

      container = obj;

      path = s+1;
    }
  /* NOT REACHED */
}

static TObject* TopLevel(char* path, char**opath)
{
  if (0)
    printf("Extract top level object from [%s]\n", path);

  while (*path == '/')
    path++;

  char* s = strchr(path,'/');
  
  if (s)
    {
      *s = 0;
      *opath = s+1;
    }
  else
    {
      *opath = NULL;
    }

  TObject *obj = NULL;

  for (unsigned int i=0; i<gExports.size(); i++)
    {
      const char* ename = gExports[i].c_str();
      //printf("Compare [%s] and [%s]\n", path, ename);
      if (strcmp(path, ename) == 0)
        {
          const char* xname = gExportNames[ename].c_str();
          obj = gROOT->FindObjectAny(xname);
          //printf("Lookup of [%s] returned %p\n", xname, obj);
          break;
        }
    }

  if (!obj)
    {
      printf("ERROR: Top level object \'%s\' not found in exports list\n", path);
      return NULL;
    }

  return obj;
}

static TObject* FollowPath(char* path)
{
  if (0)
    printf("Follow path [%s]\n", path);

  char *s;
  TObject *obj = TopLevel(path, &s);

  if (!obj)
    return NULL;

  if (!s)
    return obj;

  return FollowPath(obj, s);
}

/*------------------------------------------------------------------*/

void ResetObject(TObject* obj)
{
  assert(obj!=NULL);

  if (gVerbose)
    printf("ResetObject object %p name [%s] type [%s]\n", obj, obj->GetName(), obj->IsA()->GetName());

  if (obj->InheritsFrom(TH1::Class()))
    {
      ((TH1*)obj)->Reset();
    }
  else if (obj->InheritsFrom(TDirectory::Class()))
    {
      TDirectory* dir = (TDirectory*)obj;
      TList* objs = dir->GetList();

      TIter next = objs;
      while(1)
        {
          TObject *obj = next();
          if (obj == NULL)
            break;
          ResetObject(obj);
        }
    }
}

/*------------------------------------------------------------------*/

static TKey* MakeKey(TObject* obj, int cycle, TDirectory* dir, const char* name = NULL)
{
  TClass *xclass = obj->IsA();
  
  if (xclass->InheritsFrom(TDirectory::Class()))
    xclass = TDirectory::Class();
  else if (xclass->InheritsFrom(TFolder::Class()))
    xclass = TDirectory::Class();
  else if (xclass->InheritsFrom(TCollection::Class()))
    xclass = TDirectory::Class();

  if (!name)
    name = obj->GetName();
  
  return new TKey(name, obj->GetTitle(), xclass, cycle, dir);
}
 
/*------------------------------------------------------------------*/

static void SendString(TSocket* sock, const char* str)
{
   sock->SendRaw(str, strlen(str));
}

static void SendHttpReply(TSocket* sock, const char* mimetype, const char* message)
{
   char buf[256];
   int len = strlen(message);
   SendString(sock, "HTTP/1.1 200 OK\n");
   //SendString(sock, "Date: Tue, 15 May 2012 16:50:31 GMT\n");
   SendString(sock, "Server: ROOTANA xmlServer\n");
   sprintf(buf, "Content-Length: %d\n", len);
   SendString(sock, buf);
   //Connection: close\n
   sprintf(buf, "Content-Type: %s\n", mimetype);
   SendString(sock, buf);
   //charset=iso-8859-1\n
   SendString(sock, "\n");
   SendString(sock, message);
}

static void SendHttpReply(TSocket* sock, const char* mimetype, const std::string& str)
{
   SendHttpReply(sock, mimetype, str.c_str());
}

static std::string HtmlTag(const char* tag, const char* contents)
{
   std::string s;
   s += "<";
   s += tag;
   s += ">";
   s += contents;
   s += "</";
   s += tag;
   s += ">";
   return s;
}

static std::string HtmlTag(const char* tag, const std::string& contents)
{
   std::string s;
   s += "<";
   s += tag;
   s += ">";
   s += contents;
   s += "</";
   s += tag;
   s += ">";
   return s;
}

static THREADTYPE root_server_thread(void *arg)
/*
  Serve histograms over TCP/IP socket link
*/
{
   char request[2560];

   TSocket *sock = (TSocket *) arg;
   //TMessage message(kMESS_OBJECT);

   do {

      /* close connection if client has disconnected */
      int rd = sock->RecvRaw(request, sizeof(request), kDontBlock); // (ESendRecvOptions)-1);
      if (rd <= 0)
        {
          if (gVerbose)
            fprintf(stderr, "TXmlServer connection from %s closed\n", sock->GetInetAddress().GetHostName());
          sock->Close();
          delete sock;
          return THREADRETURN;
        }

      if (1) {
         char *p;
         p = strchr(request, '\n');
         if (p)
            *p = 0;
         
         p = strchr(request, '\r');
         if (p)
            *p = 0;
      }
         
      if (gVerbose)
        printf("xmlServer: Request [%s] from %s\n", request, sock->GetInetAddress().GetHostName());

      if (0) {} 
      else if (strstr(request, "GET / "))
        {
          // enumerate top level exported directories

          LockRootGuard lock;
          
          std::string reply;

          reply += "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html><head>\n";
          reply += HtmlTag("title", "Export list") + "\n";
          reply += "</head><body>\n";
          reply += HtmlTag("h1", "Export list") + "\n";

          for (unsigned int i=0; i<gExports.size(); i++) {
             const char* ename = gExports[i].c_str();
             const char* xname = gExportNames[ename].c_str();
             
             TObject* obj = gROOT->FindObjectAny(xname);
             
             if (obj) {
                std::string s;
                s += "<a href=\"";
                s += ename;
                s += "\">";
                s += ename;
                s += "</a>\n";
                reply += HtmlTag("p", s) + "\n";
             } else {
                std::string s;
                s += ename;
                s += " (cannot be found. maybe deleted?)\n";
                reply += HtmlTag("p", s) + "\n";
             }
          }
          
          lock.Unlock();

          reply += "</body></html>\n";
          SendHttpReply(sock, "text/html", reply);
        }
      else if (strstr(request, "GET /index.xml "))
        {
          // enumerate top level exported directories

          LockRootGuard lock;
          
          //printf("Top level exported directories are:\n");
          TList* keys = new TList();
          
          for (unsigned int i=0; i<gExports.size(); i++)
            {
              const char* ename = gExports[i].c_str();
              const char* xname = gExportNames[ename].c_str();

              TObject* obj = gROOT->FindObjectAny(xname);

              if (!obj)
                {
                  fprintf(stderr, "GetListOfKeys: Exported name \'%s\' cannot be found!\n", xname);
                  continue;
                }

              TKey* key = MakeKey(obj, 1, gROOT, ename);
              keys->Add(key);
            }
          
          if (gVerbose)
            {
              printf("Sending keys %p\n", keys);
              keys->Print();
            }

          const char*s = TBufferXML::ConvertToXML(keys);

          delete keys;
          lock.Unlock();
          SendHttpReply(sock, "application/xml", s);
        }
      else if (strncmp(request, "GetListOfKeys ", 14) == 0)
        {
          LockRootGuard lock;

          char* dirname = request + 14;

          TObject* obj = FollowPath(dirname);

          if (obj && obj->InheritsFrom(TDirectory::Class()))
            {
              TDirectory* dir = (TDirectory*)obj;

              //printf("Directory %p\n", dir);
              //dir->Print();
              
              TList* xkeys = dir->GetListOfKeys();
              TList* keys = xkeys;
              if (!keys)
                keys = new TList();

              //printf("Directory %p keys:\n", dir);
              //keys->Print();
              
              TList* objs = dir->GetList();

              //printf("Directory %p objects:\n", dir);
              //objs->Print();
              
              TIter next = objs;
              while(1)
                {
                  TObject *obj = next();

                  //printf("object %p\n", obj);

                  if (obj == NULL)
                    break;
                  
                  const char* name      = obj->GetName();
                  
                  if (!keys->FindObject(name))
                    {
                      TKey* key = MakeKey(obj, 1, dir);
                      keys->Add(key);
                    }
                }

              //printf("Sending keys %p\n", keys);
              //keys->Print();

              const char*s = TBufferXML::ConvertToXML(keys);
              sock->SendRaw(s, strlen(s) + 1);

              if (keys != xkeys)
                delete keys;
            }
          else if (obj && obj->InheritsFrom(TFolder::Class()))
            {
              TFolder* folder = (TFolder*)obj;

              //printf("Folder %p\n", folder);
              //folder->Print();

              TIterator *iterator = folder->GetListOfFolders()->MakeIterator();

              TList* keys = new TList();

              while (1)
                {
                  TNamed *obj = (TNamed*)iterator->Next();
                  if (obj == NULL)
                    break;
      
                  const char* name      = obj->GetName();
                  
                  if (!keys->FindObject(name))
                    {
                      TKey* key = MakeKey(obj, 1, gROOT);
                      keys->Add(key);
                    }
                }
              
              delete iterator;

              if (gVerbose)
                {
                  printf("Sending keys %p\n", keys);
                  keys->Print();
                }

              //message.Reset(kMESS_OBJECT);
              //message.WriteObject(keys);

              const char*s = TBufferXML::ConvertToXML(keys);
              sock->SendRaw(s, strlen(s) + 1);

              delete keys;
            }
          else if (obj && obj->InheritsFrom(TCollection::Class()))
            {
              TCollection* collection = (TCollection*)obj;

              //printf("Collection %p\n", collection);
              //collection->Print();

              TIterator *iterator = collection->MakeIterator();

              TList* keys = new TList();

              while (1)
                {
                  TNamed *obj = (TNamed*)iterator->Next();
                  if (obj == NULL)
                    break;
      
                  const char* name      = obj->GetName();
                  
                  if (!keys->FindObject(name))
                    {
                      TKey* key = MakeKey(obj, 1, gROOT);
                      keys->Add(key);
                    }
                }
              
              delete iterator;

              if (gVerbose)
                {
                  printf("Sending keys %p\n", keys);
                  keys->Print();
                }

              //message.Reset(kMESS_OBJECT);
              //message.WriteObject(keys);

              const char*s = TBufferXML::ConvertToXML(keys);
              sock->SendRaw(s, strlen(s) + 1);

              delete keys;
            }
          else if (obj)
            {
              fprintf(stderr, "netDirectoryServer: ERROR: obj %p name %s, type %s is not a directory!\n", obj, obj->GetName(), obj->IsA()->GetName());
              //TObjString s("Not a directory");
              //message.Reset(kMESS_OBJECT);
              //message.WriteObject(&s);

              const char*s = "Not a directory";
              sock->SendRaw(s, strlen(s) + 1);
            }
          else
            {
              fprintf(stderr, "netDirectoryServer: ERROR: obj %p not found\n", obj);
              //TObjString s("Not found");
              //message.Reset(kMESS_OBJECT);
              //message.WriteObject(&s);

              const char*s = "Not found";
              sock->SendRaw(s, strlen(s) + 1);
            }
              
          lock.Unlock();
          //sock->Send(message);
        }
      else if (strncmp(request, "FindObjectByName ", 17) == 0)
        {
          LockRootGuard lock;

          char* top  = request + 17;

          char *s;
          TObject *obj = TopLevel(top, &s);

          if (obj && !s)
            {
              // they requested a top-level object. Give out a fake name

              char str[256];
              sprintf(str, "TDirectory %s", obj->GetName());

              for (unsigned int i=0; i<gExports.size(); i++)
                {
                  const char* ename = gExports[i].c_str();
                  const char* xname = gExportNames[ename].c_str();

                  if (strcmp(xname, obj->GetName()) == 0)
                    {
                      sprintf(str, "TDirectory %s", ename);
                      break;
                    }
                }

              obj = new TObjString(str); // FIXME: memory leak!
            }
          else if (obj)
            {
              obj = FollowPath(obj, s);
            }

          if (obj && obj->InheritsFrom(TDirectory::Class()))
            {
              char str[256];
              sprintf(str, "TDirectory %s", obj->GetName());
              obj = new TObjString(str);
            }
          
          if (obj && obj->InheritsFrom(TFolder::Class()))
            {
              char str[256];
              sprintf(str, "TDirectory %s", obj->GetName());
              obj = new TObjString(str);
            }
          
          if (obj && obj->InheritsFrom(TCollection::Class()))
            {
              char str[256];
              sprintf(str, "TDirectory %s", obj->GetName());
              obj = new TObjString(str);
            }
          
          if (gVerbose)
            {
              if (obj)
                printf("Sending object %p name \'%s\' class \'%s\'\n", obj, obj->GetName(), obj->IsA()->GetName());
              else
                printf("Sending object %p\n", obj);
              //obj->Print();
            }

          const char *msg = TBufferXML::ConvertToXML(obj);
          lock.Unlock();

          sock->SendRaw(msg, strlen(msg) + 1);
        }
      else if (strncmp(request, "ResetTH1 ", 9) == 0)
        {
          LockRootGuard lock;
          
          char* path = request + 9;

          if (strlen(path) > 1)
            {
              TObject *obj = FollowPath(path);

              if (obj)
                ResetObject(obj);
            }
          else
            {
              for (unsigned int i=0; i<gExports.size(); i++)
                {
                  const char* ename = gExports[i].c_str();
                  const char* xname = gExportNames[ename].c_str();

                  TObject* obj = gROOT->FindObjectAny(xname);

                  if (!obj)
                    {
                      fprintf(stderr, "ResetTH1: Exported name \'%s\' cannot be found!\n", xname);
                      continue;
                    }

                  ResetObject(obj);
                }
            }
          
          //TObjString s("Success");

          //message.Reset(kMESS_OBJECT);
          //message.WriteObject(&s);
          lock.Unlock();
          //sock->Send(message);

          const char *msg = "Success";
          sock->SendRaw(msg, strlen(msg) + 1);
        }
      else
        {
          fprintf(stderr, "xmlServer: Received unknown request \"%s\"\n", request);

          std::string reply;
          reply += "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html><head>\n";
          reply += HtmlTag("title", "Unknown request") + "\n";
          reply += "</head><body>\n";
          reply += HtmlTag("h1", "Unknown request") + "\n";
          reply += HtmlTag("p", request) + "\n";
          reply += "</body></html>\n";
          SendHttpReply(sock, "text/html", reply);
        }
   } while (1);

   return THREADRETURN;
}

/*------------------------------------------------------------------*/

static THREADTYPE socket_listener(void *arg)
{
  // Server loop listening for incoming network connections on specified port.
  // Starts a searver_thread for each connection.

  int port = *(int *) arg;
  
  fprintf(stderr, "XmlServer listening on port %d...\n", port);
  TServerSocket *lsock = new TServerSocket(port, kTRUE);
  
  while (1)
    {
      TSocket *sock = lsock->Accept();
      
      if (sock==NULL)
        {
          printf("XmlServer accept() error\n");
          break;
        }
      
      if (gVerbose)
        fprintf(stderr, "XmlServer connection from %s\n", sock->GetInetAddress().GetHostName());
      
#if 1
      TThread *thread = new TThread("XmlServer", root_server_thread, sock);
      thread->Run();
#else
      LPDWORD lpThreadId = 0;
      CloseHandle(CreateThread(NULL, 1024, &root_server_thread, sock, 0, lpThreadId));
#endif
    }
  
  return THREADRETURN;
}

/*------------------------------------------------------------------*/

void XmlServer::SetVerbose(bool verbose)
{
  gVerbose = verbose;
  //gDebugLockRoot = verbose;
}

/*------------------------------------------------------------------*/

void XmlServer::Export(TDirectory* dir, const char* exportName)
{
  if (gVerbose)
    printf("Export TDirectory %p named [%s] of type [%s] as [%s]\n", dir, dir->GetName(), dir->IsA()->GetName(), exportName);

  bool found = false;
  for (unsigned int i=0; i<gExports.size(); i++)
    {
      const char* ename = gExports[i].c_str();
      if (strcmp(ename, exportName) == 0)
        found = true;
    }

  if (!found)
    gExports.push_back(exportName);
  gExportNames[exportName] = dir->GetName();
}

void XmlServer::Export(TFolder* folder, const char* exportName)
{
  if (gVerbose)
    printf("Export TFolder %p named [%s] of type [%s] as [%s]\n", folder, folder->GetName(), folder->IsA()->GetName(), exportName);

  bool found = false;
  for (unsigned int i=0; i<gExports.size(); i++)
    {
      const char* ename = gExports[i].c_str();
      if (strcmp(ename, exportName) == 0)
        found = true;
    }

  if (!found)
    gExports.push_back(exportName);
  gExportNames[exportName] = folder->GetName();
}

void XmlServer::Export(TCollection* collection, const char* exportName)
{
  if (gVerbose)
    printf("Export TCollection %p named [%s] of type [%s] as [%s]\n", collection, collection->GetName(), collection->IsA()->GetName(), exportName);

  bool found = false;
  for (unsigned int i=0; i<gExports.size(); i++)
    {
      const char* ename = gExports[i].c_str();
      if (strcmp(ename, exportName) == 0)
        found = true;
    }

  if (!found)
    gExports.push_back(exportName);
  gExportNames[exportName] = collection->GetName();
}

void XmlServer::Start(int port)
{
  if (port==0)
    return;

  printf("Here!\n");

  //StartLockRootTimer();

  static int pport = port;
#if 1
  TThread *thread = new TThread("XmlServer", socket_listener, &pport);
  thread->Run();
#else
  LPDWORD lpThreadId = 0;
  CloseHandle(CreateThread(NULL, 1024, &root_socket_server, &pport, 0, lpThreadId));
#endif
}

// end

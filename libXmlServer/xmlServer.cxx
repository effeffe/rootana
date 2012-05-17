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

static std::string HtmlEncode(const char* s)
{
   std::string r;
   while (*s) {
      char c = *s;
      if (isalpha(c)) {
         r += c;
      } else if (isdigit(c)) {
         r += c;
      } else {
         char buf[16];
         sprintf(buf, "%%%02x", c);
         r += buf;
      }
      s++;
   }
   return r;
}

static int atohex(int a)
{
   if (a>='0' && a<='9')
      return a-'0';
   if (a>='a' && a<='f')
      return a-'a'+10;
   if (a>='A' && a<='F')
      return a-'A'+10;
   return 0;
}

static std::string HtmlDecode(const char* s)
{
   std::string r;
   while (*s) {
      if (*s == '%') {
         s++;
         char c = 0;
         c = c*16 + atohex(s[0]);
         c = c*16 + atohex(s[1]);
         r += c;

         // be careful when incrementing "s" to not overrun the '\0' string terminator!
         if (*s)
            s++;
         if (*s)
            s++;
      } else {
         r += *s++;
      }
   }
   return r;
}

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

      std::string xpath = HtmlDecode(path);
      //printf("FindObject %s, decoded [%s]\n", path, xpath.c_str());

      if (container->InheritsFrom(TDirectory::Class()))
        obj = ((TDirectory*)container)->FindObject(xpath.c_str());
      else if (container->InheritsFrom(TFolder::Class()))
        obj = ((TFolder*)container)->FindObject(xpath.c_str());
      else if (container->InheritsFrom(TCollection::Class()))
        obj = ((TCollection*)container)->FindObject(xpath.c_str());
      else
        {
          fprintf(stderr, "XmlServer: ERROR: Container \'%s\' of type %s is not a TDirectory, TFolder or TCollection\n", container->GetName(), container->IsA()->GetName());
          return NULL;
        }

      if (!obj)
        return NULL;

      if (!s)
        return obj;

      container = obj;

      path = s+1;
    }
  /* NOT REACHED */
}

static TObject* FindTopLevelObject(const char* name)
{
   TObject *obj = NULL;
   //gROOT->GetListOfFiles()->Print();
   obj = gROOT->GetListOfFiles()->FindObject(name);
   if (obj)
      return obj;
   obj = gROOT->FindObjectAny(name);
   if (obj)
      return obj;
   return NULL;
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
          obj = FindTopLevelObject(xname);
          //printf("Lookup of [%s] returned %p\n", xname, obj);
          break;
        }
    }

  if (!obj)
    {
      fprintf(stderr, "XmlServer: ERROR: Top level object \'%s\' not found in exports list\n", path);
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

/*------------------------------------------------------------------*/

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

/*------------------------------------------------------------------*/

std::string MakeXmlEntry(const TObject* obj)
{
   const char* objname = obj->GetName();
   const char* classname = obj->ClassName();
   bool isSubdir = false;
   std::string xkey = "";

   if (obj->InheritsFrom(TKey::Class())) {
      TKey* key = (TKey*)obj;
      classname = key->GetClassName();
      xkey = "<key/>";
   }

   const TClass *xclass = TClass::GetClass(classname);

   if (xclass->InheritsFrom(TDirectory::Class()))
      isSubdir = true;

   if (xclass->InheritsFrom(TFolder::Class()))
      isSubdir = true;

   if (xclass->InheritsFrom(TCollection::Class()))
      isSubdir = true;

   //printf("xclass %s, classname %s, subdir %d\n", xclass->GetName(), classname, isSubdir);

   if (isSubdir)
      return HtmlTag("subdir", HtmlTag("name", HtmlEncode(objname)) + HtmlTag("class", HtmlEncode(classname)) + xkey) + "\n";
   else
      return HtmlTag("object", HtmlTag("name", HtmlEncode(objname)) + HtmlTag("class", HtmlEncode(classname)) + xkey) + "\n";
}

std::string MakeHtmlEntry(const TObject* obj, const char* path)
{
   const char* objname = obj->GetName();
   const char* classname = obj->ClassName();
   bool isKey = false;

   if (obj->InheritsFrom(TKey::Class())) {
      TKey* key = (TKey*)obj;
      classname = key->GetClassName();
      isKey = true;
   }

   std::string s;

   s += "<a href=\"";
   s += path; //dir->GetName();
   s += "/";
   s += HtmlEncode(objname);
   s += "\">";
   s += objname;
   s += "</a>\n";
   s += " (";
   s += classname;
   s += ")";
   if (isKey)
      s += "-KEY";
   s += " ";
   s += "<a href=\"";
   s += path; //dir->GetName();
   s += "/";
   s += HtmlEncode(objname);
   s += ".xml";
   s += "\">";
   s += "XML</a>\n";

   return s;
}
   
/*------------------------------------------------------------------*/

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
            fprintf(stderr, "XmlServer: connection from %s closed\n", sock->GetInetAddress().GetHostName());
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
        printf("XmlServer: Request [%s] from %s\n", request, sock->GetInetAddress().GetHostName());

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
             
             TObject* obj = FindTopLevelObject(xname);
             
             if (obj) {
                std::string s;
                s += " ";
                s += "<a href=\"";
                s += HtmlEncode(ename);
                s += "\">";
                s += ename;
                s += "</a>\n";
                s += "<a href=\"";
                s += HtmlEncode(ename);
                s += ".xml";
                s += "\">";
                s += "XML</a>\n";
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
      else if (strstr(request, "GET /favicon.ico "))
        {
          std::string reply;

          reply += "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html><head>\n";
          reply += HtmlTag("title", "No favicon") + "\n";
          reply += "</head><body>\n";
          reply += HtmlTag("h1", "No favicon") + "\n";
          reply += HtmlTag("p", "No favicon") + "\n";
          reply += "</body></html>\n";
          SendHttpReply(sock, "text/html", reply);
        }
      else if (strstr(request, "GET /index.xml "))
        {
          // enumerate top level exported directories

          LockRootGuard lock;

          std::string xml;

          xml += "<xml>\n";
          xml += "<dir>\n";
          
          for (unsigned int i=0; i<gExports.size(); i++)
            {
              const char* ename = gExports[i].c_str();
              const char* xname = gExportNames[ename].c_str();

              TObject* obj = FindTopLevelObject(xname);

              if (!obj) {
                 xml += HtmlTag("subdir", HtmlTag("name", HtmlEncode(ename)) + "<deleted/>") + "\n";
                 continue;
              }

              const char* cname = obj->ClassName();

              xml += HtmlTag("subdir", HtmlTag("name", HtmlEncode(ename)) + HtmlTag("class", HtmlEncode(cname))) + "\n";
            }
          
          lock.Unlock();

          xml += "</dir>\n";
          xml += "</xml>\n";

          SendHttpReply(sock, "application/xml", xml);
        }
      else if (strncmp(request, "GET /", 5) == 0)
        {
          LockRootGuard lock;

          char* dirname = request + 5;

          char* x = strstr(dirname, " HTTP");
          if (x)
             *x = 0;

          bool xmlOutput = false;

          x = strstr(dirname, ".xml");
          if (x) {
             *x = 0;
             xmlOutput = true;
          }

          std::string path;
          path += "/";
          path += dirname;

          std::string xpath = HtmlDecode(dirname);

          TObject* obj = FollowPath(dirname);

          if (!obj) {
             std::string reply;
             std::string buf;
             
             reply += "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html><head>\n";

             buf = "Not found ";
             buf += xpath;
             reply += HtmlTag("title", buf) + "\n";
             reply += "</head><body>\n";
             reply += HtmlTag("h1", buf) + "\n";
             
             reply += HtmlTag("p", "Object not found");

             reply += "</body></html>\n";
             SendHttpReply(sock, "text/html", reply);

          } else if (obj && obj->InheritsFrom(TDirectory::Class())) {
             TDirectory* dir = (TDirectory*)obj;

             std::string reply;
             std::string buf;
             std::string xml;

             xml += "<xml>\n";
             xml += "<dir>\n";
             
             reply += "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html><head>\n";

             buf = "Dir ";
             buf += xpath;
             reply += HtmlTag("title", buf) + "\n";
             reply += "</head><body>\n";
             reply += HtmlTag("h1", buf) + "\n";

             //printf("Directory %p\n", dir);
             //dir->Print();

             std::map<std::string, std::string> alist;

             if (1) {
                TList* keys = dir->GetListOfKeys();

                //printf("Directory %p keys:\n", dir);
                //keys->Print();
             
                TIter next = keys;
                while(1) {
                   TObject *obj = next();
                   
                   //printf("object %p\n", obj);
                   
                   if (obj == NULL)
                      break;

                   std::string a = HtmlTag("p", MakeHtmlEntry(obj, path.c_str())) + "\n";
                   //alist[objname] = a;
                   reply += a;
                   
                   xml += MakeXmlEntry(obj);
                }
             }

             if (1) {
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
                      
                      std::string a = HtmlTag("p", MakeHtmlEntry(obj, path.c_str())) + "\n";
                      //alist[objname] = a;
                      reply += a;

                      xml += MakeXmlEntry(obj);
                   }
             }
                
             lock.Unlock();
             
             if (1) {
                std::map<std::string, std::string>::iterator iter = alist.begin();
                for (; iter!=alist.end(); iter++) {
                   // iter->first is your key
                   // iter->second is it''s value
                   reply += iter->second;
                }
             }
              
             reply += "</body></html>\n";

             xml += "</dir></xml>\n";

             if (xmlOutput)
                SendHttpReply(sock, "application/xml", xml);
             else
                SendHttpReply(sock, "text/html", reply);
             
          } else if (obj && obj->InheritsFrom(TFolder::Class())) {
             TFolder* folder = (TFolder*)obj;

             //printf("Folder %p\n", folder);
             //folder->Print();

             std::string xml;

             xml += "<xml>\n";
             xml += "<dir>\n";
             
             std::string reply;
             std::string buf;
             
             reply += "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html><head>\n";

             buf = "Folder ";
             buf += xpath;
             reply += HtmlTag("title", buf) + "\n";
             reply += "</head><body>\n";
             reply += HtmlTag("h1", buf) + "\n";
             
             TIterator *iterator = folder->GetListOfFolders()->MakeIterator();
             
             while (1)
                {
                   TNamed *obj = (TNamed*)iterator->Next();
                   if (obj == NULL)
                      break;
                   
                   reply += HtmlTag("p", MakeHtmlEntry(obj, path.c_str())) + "\n";
                   xml += MakeXmlEntry(obj);
                }
             
             delete iterator;

             lock.Unlock();

             xml += "</dir></xml>\n";
             reply += "</body></html>\n";

             if (xmlOutput)
                SendHttpReply(sock, "application/xml", xml);
             else
                SendHttpReply(sock, "text/html", reply);

          } else if (obj && obj->InheritsFrom(TCollection::Class())) {
             TCollection* collection = (TCollection*)obj;
             
             //printf("Collection %p\n", collection);
             //collection->Print();
             //printf("Entries %d\n", collection->GetEntries());
             //printf("IsEmpty %d\n", collection->IsEmpty());
             
             std::string xml;

             xml += "<xml>\n";
             xml += "<dir>\n";
             
             std::string reply;
             std::string buf;
             
             reply += "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html><head>\n";

             buf = "Collection ";
             buf += xpath;
             reply += HtmlTag("title", buf) + "\n";
             reply += "</head><body>\n";
             reply += HtmlTag("h1", buf) + "\n";
             
             TIterator *iterator = collection->MakeIterator();
             
             while (1)
                {
                   TNamed *obj = (TNamed*)iterator->Next();
                   if (obj == NULL)
                      break;
                   
                   reply += HtmlTag("p", MakeHtmlEntry(obj, path.c_str())) + "\n";
                   xml += MakeXmlEntry(obj);
                }
             
             delete iterator;

             lock.Unlock();
             
             xml += "</dir></xml>\n";
             reply += "</body></html>\n";

             if (xmlOutput)
                SendHttpReply(sock, "application/xml", xml);
             else
                SendHttpReply(sock, "text/html", reply);

          } else {
             if (xmlOutput) {
                std::string xml;
                xml += "<xml>\n";
                xml += "<ROOTobject>\n";
                const char *msg = TBufferXML::ConvertToXML(obj);
                xml += msg;
                xml += "</ROOTobject>\n";
                xml += "</xml>\n";
                SendHttpReply(sock, "application/xml", xml);
             } else {
                std::string reply;
                std::string buf;
                
                reply += "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html><head>\n";
                
                buf = "Object ";
                buf += xpath;
                reply += HtmlTag("title", buf) + "\n";
                reply += "</head><body>\n";
                reply += HtmlTag("h1", buf) + "\n";
                
                reply += HtmlTag("p", obj->GetName());
                reply += HtmlTag("p", obj->ClassName());
                
                reply += "</body></html>\n";
                SendHttpReply(sock, "text/html", reply);
             }
          }
        }
#if 0
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

                  TObject* obj = FindTopLevelObject(xname);

                  if (!obj)
                    {
                      fprintf(stderr, "XmlServer: ResetTH1: Exported name \'%s\' cannot be found!\n", xname);
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
#endif
      else
        {
          fprintf(stderr, "XmlServer: Received unknown request \"%s\"\n", request);

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
  
  fprintf(stderr, "XmlServer: Listening on port %d...\n", port);
  TServerSocket *lsock = new TServerSocket(port, kTRUE);
  
  while (1)
    {
      TSocket *sock = lsock->Accept();
      
      if (sock==NULL)
        {
          fprintf(stderr, "XmlServer: TSocket->Accept() error\n");
          break;
        }
      
      if (gVerbose)
        fprintf(stderr, "XmlServer: connection from %s\n", sock->GetInetAddress().GetHostName());
      
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

  //printf("Here!\n");

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

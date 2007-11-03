/********************************************************************\

  Name:         mana.c
  Created by:   Stefan Ritt

  Contents:     The system part of the MIDAS analyzer. Has to be
                linked with analyze.c to form a complete analyzer

  $Id: mana.c 3326 2006-09-20 13:31:46Z ritt $

\********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "netDirectoryServer.h"

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
#include <TSemaphore.h>
#include <TKey.h>
#include <TFolder.h>
#include <TSocket.h>
#include <TServerSocket.h>
#include <TThread.h>
#include <TMessage.h>
#include <TObjString.h>
#include <TH1.h>
#include <TCutG.h>

#include <deque>
#include <map>
#include <string>

static bool gVerbose = false;

static std::deque<std::string> gExports;
static std::map<std::string,std::string> gExportNames;

static TSemaphore gRootSema(0); // wait by server, post by timer
static TSemaphore gWaitSema(0); // post by server, wait by timer
static TSemaphore gDoneSema(0); // post by server, wait by timer

static bool gDebugLock = false;

static void LockRoot()
{
  if (gDebugLock)
    printf("Try Lock ROOT!\n");
  gWaitSema.Post();
  gRootSema.Wait();
  if (gDebugLock)
    printf("Lock ROOT!\n");
}

static void UnlockRoot()
{
  if (gDebugLock)
    printf("Unlock ROOT!\n");
  gDoneSema.Post();
}

struct LockRootGuard
{
  bool fLocked;

  LockRootGuard()
  {
    fLocked = true;
    LockRoot();
  }

  ~LockRootGuard()
  {
    if (fLocked)
      Unlock();
  }

  void Unlock()
  {
    UnlockRoot();
    fLocked = false;
  }
};

class ServerTimer : public TTimer
{
public:

  ServerTimer()
  {
    //int period_msec = 100;
    //Start(period_msec,kTRUE);
  }

  Bool_t Notify()
  {
    if (gDebugLock)
      fprintf(stderr, "ServerTimer::Notify!!\n");

    int notWaiting = gWaitSema.TryWait();
    if (gDebugLock)
      printf("NotWaiting %d!\n", notWaiting);
    if (!notWaiting)
      {
        if (gDebugLock)
          printf("Yeld root sema!\n");
        gRootSema.Post();
        TThread::Self()->Sleep(0, 1000000); // sleep in ns
        gDoneSema.Wait();
        if (gDebugLock)
          printf("Recapture root sema!\n");
      }

    Reset();
    return kTRUE;
  }

  ~ServerTimer()
  {
    //TurnOff();
  }
};

static ServerTimer gTimer;

/*------------------------------------------------------------------*/

#include <map>
#include <string>

/*------------------------------------------------------------------*/

static TObject* FollowPath(char* path)
{
  TDirectory* dir = NULL;

  //printf("Path [%s]\n", path);

  for (int level=0; ; level++)
    {
      while (*path == '/')
        path++;

      char* s = strchr(path,'/');

      if (s)
        *s = 0;

      //printf("Level %d, element [%s], next %p\n", level, path, s);

      if (level==0)
        {
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

          if (!obj->InheritsFrom(TDirectory::Class()))
            {
              printf("ERROR: Object \'%s\' of type %s not a directory\n", obj->GetName(), obj->IsA()->GetName());
              return NULL;
            }

          dir = (TDirectory*)obj;
        }
      else if (s)
        {
          assert(dir);

          TObject *obj = dir->FindObject(path);

          //printf("Looking for [%s], got %p\n", path, obj);

          if (!obj)
            return NULL;

          if (!obj->InheritsFrom(TDirectory::Class()))
            {
              printf("ERROR: Object \'%s\' of type %s not a directory\n", path, obj->IsA()->GetName());
              return NULL;
            }

          dir = (TDirectory*)obj;
        }
      else
        {
          assert(dir);

          TObject *obj = dir->FindObject(path);

          //printf("Looking for [%s], got %p\n", path, obj);

          return obj;
        }

      if (!s)
        return dir;

      path = s+1;
    }

  return dir;
}

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

static THREADTYPE root_server_thread(void *arg)
/*
  Serve histograms over TCP/IP socket link
*/
{
   char request[2560];

   TSocket *sock = (TSocket *) arg;
   TMessage message(kMESS_OBJECT);

   do {

      /* close connection if client has disconnected */
      int rd = sock->Recv(request, sizeof(request));
      if (rd <= 0)
        {
          if (gVerbose)
            fprintf(stderr, "TNetDirectory connection from %s closed\n", sock->GetInetAddress().GetHostName());
          sock->Close();
          delete sock;
          return THREADRETURN;
        }

      if (gVerbose)
        printf("Request [%s] from %s\n", request, sock->GetInetAddress().GetHostName());

      if (strcmp(request, "GetListOfKeys") == 0)
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

              const char* name      = ename;
              const char* title     = obj->GetTitle();
              
              
              TClass *xclass = obj->IsA();
              if (xclass->InheritsFrom(TDirectory::Class()))
                xclass = TDirectory::Class();

              if (gVerbose)
                printf("Class \'%s\', name \'%s\', title \'%s\'\n", xclass->GetName(), name, title);

              TKey* key = new TKey(name, title, xclass, 1, gROOT);
              keys->Add(key);
            }
          
          if (gVerbose)
            {
              printf("Sending keys %p\n", keys);
              keys->Print();
            }

          message.Reset(kMESS_OBJECT);
          message.WriteObject(keys);
          delete keys;
          lock.Unlock();
          sock->Send(message);
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
                  const char* title     = obj->GetTitle();
                  
                  //printf("Enumerating objects: %s %s %s\n", obj->IsA()->GetName(), name, title);
                  
                  if (!keys->FindObject(name))
                    {
                      TKey* key = new TKey(name, title, obj->IsA(), 1, dir);
                      keys->Add(key);
                    }
                }

              //printf("Sending keys %p\n", keys);
              //keys->Print();

              message.Reset(kMESS_OBJECT);
              message.WriteObject(keys);
              if (keys != xkeys)
                delete keys;
            }
          else
            {
              TObjString s("Not a directory");
              message.Reset(kMESS_OBJECT);
              message.WriteObject(&s);
            }
              
          lock.Unlock();
          sock->Send(message);
        }
      else if (strncmp(request, "FindObjectByName ", 17) == 0)
        {
          LockRootGuard lock;

          char* top  = request + 17;

          TObject *obj = FollowPath(top);

          if (obj && obj->InheritsFrom(TDirectory::Class()))
            {
              char str[256];
              sprintf(str, "TDirectory %s", obj->GetName());

              if (1) // top level objects use fake names
                {
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
                }

              obj = new TObjString(str);
            }
          
          if (gVerbose)
            printf("Sending object %p name \'%s\' class \'%s\'\n", obj, obj->GetName(), obj->IsA()->GetName());
          //obj->Print();

          message.Reset(kMESS_OBJECT);
          message.WriteObject(obj);
          lock.Unlock();
          sock->Send(message);
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
          
          TObjString s("Success");

          message.Reset(kMESS_OBJECT);
          message.WriteObject(&s);
          lock.Unlock();
          sock->Send(message);
        }
      else
        {
          fprintf(stderr, "netDirectoryServer: Received unknown request \"%s\"\n", request);
          LockRootGuard lock;
          TObjString s("Unknown request");
          message.Reset(kMESS_OBJECT);
          message.WriteObject(&s);
          lock.Unlock();
          sock->Send(message);
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
  
  fprintf(stderr, "NetDirectory server listening on port %d...\n", port);
  TServerSocket *lsock = new TServerSocket(port, kTRUE);
  
  while (1)
    {
      TSocket *sock = lsock->Accept();
      
      if (sock==NULL)
        {
          printf("TNetDirectory server accept() error\n");
          break;
        }
      
      if (gVerbose)
        fprintf(stderr, "TNetDirectory connection from %s\n", sock->GetInetAddress().GetHostName());
      
#if 1
      TThread *thread = new TThread("NetDirectoryServer", root_server_thread, sock);
      thread->Run();
#else
      LPDWORD lpThreadId = 0;
      CloseHandle(CreateThread(NULL, 1024, &root_server_thread, sock, 0, lpThreadId));
#endif
    }
  
  return THREADRETURN;
}

/*------------------------------------------------------------------*/

void VerboseNetDirectoryServer(bool verbose)
{
  gVerbose = verbose;
}

/*------------------------------------------------------------------*/

static bool gAlreadyRunning = false;

void NetDirectoryExport(TDirectory* dir, const char* exportName)
{
  if (gVerbose)
    printf("Export object %p named [%s] of type [%s] as [%s]\n", dir, dir->GetName(), dir->IsA()->GetName(), exportName);

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

void StartNetDirectoryServer(int port, TDirectory* dir)
{
  if (dir)
    NetDirectoryExport(dir, dir->GetName());

  if (gAlreadyRunning)
    return;

  if (port==0)
    return;

  gAlreadyRunning = true;

  int period_msec = 100;
  gTimer.Start(period_msec,kTRUE);

  static int pport = port;
#if 1
  TThread *thread = new TThread("NetDirectoryServer", socket_listener, &pport);
  thread->Run();
#else
  LPDWORD lpThreadId = 0;
  CloseHandle(CreateThread(NULL, 1024, &root_socket_server, &pport, 0, lpThreadId));
#endif
}

// end

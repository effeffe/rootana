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

#if defined (__linux__)
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
#include <string>

static bool gVerbose = false;

static std::deque<std::string> gExports;

static TSemaphore gRootSema(0);
static TSemaphore gWaitSema(0);

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
  gRootSema.Post();
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
    if (!notWaiting)
      {
        if (gDebugLock)
          printf("Yeld root sema!\n");
        gRootSema.Post();
        TThread::Self()->Sleep(0, 1000000); // sleep in ns
        gRootSema.Wait();
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

static TObject* FollowPath(TDirectory* dir, char* path)
{
  if (!dir)
    {
      gROOT->cd("/");
      dir = gROOT;
    }

  //printf("FollowPath [%s] from %p (%s)\n", path, dir, dir->GetName());

  bool topLevel = (*path == '/');

  for (int level=0; ; level++)
    {
      while (*path == '/')
        path++;

      char* s = strchr(path,'/');
      if (!s)
        break;

      *s = 0;

      if (topLevel && level==0)
        {
          bool found = false;
          for (unsigned int i=0; i<gExports.size(); i++)
            {
              const char* xname = gExports[i].c_str();
              if (strcmp(path, xname) == 0)
                found = true;
            }

          if (!found)
            {
              printf("ERROR: Top level object \'%s\' not found in exports list\n", path);
              return NULL;
            }
        }

      TObject* obj = dir->FindObject(path);

      //printf("Looking for [%s], got %p\n", path, obj);

      if (!obj)
        return NULL;

      if (!obj->InheritsFrom(TDirectory::Class()))
        {
          printf("ERROR: Object \'%s\' of type %s not a directory\n", path, obj->IsA()->GetName());
          return NULL;
        }

      dir = (TDirectory*)obj;

      path = s+1;
    }

  TObject* obj = dir->FindObject(path);
  //printf("Final element [%s], got %p\n", path, obj);
  return obj;
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
              const char* xname = gExports[i].c_str();

              TObject* obj = gROOT->FindObjectAny(xname);

              if (!obj)
                {
                  fprintf(stderr, "GetListOfKeys: Exported name \'%s\' cannot be found!\n", xname);
                  continue;
                }

              const char* classname = obj->IsA()->GetName();
              const char* name      = obj->GetName();
              const char* title     = obj->GetTitle();
              
              //printf("Class \'%s\', name \'%s\', title \'%s\'\n", classname, name, title);
              
              TKey* key = new TKey(name, title, obj->IsA(), 1, gROOT);
              keys->Add(key);
            }
          
          //printf("Sending keys %p\n", keys);
          //keys->Print();

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

          TObject* obj = FollowPath(NULL, dirname);

          if (obj->InheritsFrom(TDirectory::Class()))
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
                  
                  const char* classname = obj->IsA()->GetName();
                  const char* name      = obj->GetName();
                  const char* title     = obj->GetTitle();
                  
                  //printf("Enumerating objects: %s %s %s\n", classname, name, title);
                  
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

          TObject *obj = FollowPath(NULL, top);

          if (strcmp(obj->IsA()->GetName(), "TDirectory") == 0)
            {
              char str[256];
              sprintf(str, "TDirectory %s", obj->GetName());
              obj = new TObjString(str);
            }
          
          //printf("Sending object %p name \'%s\' class \'%s\'\n", obj, obj->GetName(), obj->IsA()->GetName());
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
              TObject *obj = FollowPath(NULL, path);

              if (obj)
                ResetObject(obj);
            }
          else
            {
              for (unsigned int i=0; i<gExports.size(); i++)
                {
                  const char* xname = gExports[i].c_str();

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

void StartNetDirectoryServer(int port, TDirectory* dir)
{
  gExports.push_back(dir->GetName());

  if (gAlreadyRunning)
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

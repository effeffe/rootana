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

static TDirectory *gDir;

/*------------------------------------------------------------------*/

static THREADTYPE root_server_thread(void *arg)
/*
  Serve histograms over TCP/IP socket link
*/
{
   char request[256];

   TSocket *sock = (TSocket *) arg;
   TMessage message(kMESS_OBJECT);

   do {

      /* close connection if client has disconnected */
      int rd = sock->Recv(request, sizeof(request));
      if (rd <= 0) {
         // printf("Closed connection to %s\n", sock->GetInetAddress().GetHostName());
         sock->Close();
         delete sock;
         return THREADRETURN;
      }

      printf("Request %s\n", request);

      if (strcmp(request, "GetListOfKeys") == 0)
        {
          LockRootGuard lock;
          
          printf("Directory %p contains:\n", gDir);
          gDir->Print();
          
          TList* keys = gDir->GetListOfKeys();
          printf("Directory %p keys:\n", gDir);
          keys->Print();
          
          TList* objs = gDir->GetList();
          printf("Directory %p objects:\n", gDir);
          objs->Print();
          
          TIter next = objs;
          while(1)
            {
              TObject *obj = next();
              printf("object %p\n", obj);
              if (obj == NULL)
                break;
              
              const char* classname = obj->IsA()->GetName();
              const char* name      = obj->GetName();
              const char* title     = obj->GetTitle();
              
              printf("Enumerating objects: %s %s %s\n", classname, name, title);
              
              if (!keys->FindObject(name))
                {
                  TKey* key = new TKey(name, title, obj->IsA(), 1, gDir);
                  keys->Add(key);
                }
            }
          
          printf("Sending keys %p\n", keys);
          keys->Print();
          message.Reset(kMESS_OBJECT);
          message.WriteObject(keys);
          lock.Unlock();
          sock->Send(message);
        }
      else if (strncmp(request, "FindObjectByName ", 17) == 0)
        {
          LockRootGuard lock;

          const char* name = request + 17;

          printf("Directory %p looking for [%s]\n", gDir, name);
          
          TObject* obj = gDir->FindObject(name);
          
          printf("Sending object %p\n", obj);
          obj->Print();
          message.Reset(kMESS_OBJECT);
          message.WriteObject(obj);
          lock.Unlock();
          sock->Send(message);
        }
      else if (strncmp(request, "ResetTH1 ", 9) == 0)
        {
          LockRootGuard lock;
          
          const char* name = request + 9;

          printf("Directory %p reset TH1 [%s]\n", gDir, name);

          bool all = false;
          if (strlen(name) < 1)
            all = true;
          
          TList* objs = gDir->GetList();
          printf("Directory %p objects:\n", gDir);
          objs->Print();
          
          TIter next = objs;
          while(1)
            {
              TObject *obj = next();
              printf("object %p\n", obj);
              if (obj == NULL)
                break;
              
              const char* classname = obj->IsA()->GetName();
              const char* name      = obj->GetName();
              const char* title     = obj->GetTitle();
              
              printf("Enumerating objects: %s %s %s\n", classname, name, title);
              
              if (all || strcmp(name, obj->GetName())==0)
                if (obj->InheritsFrom(TH1::Class()))
                  {
                    ((TH1*)obj)->Reset();
                  }
            }
          
          TObjString s("Success");

          message.Reset(kMESS_OBJECT);
          message.WriteObject(&s);
          lock.Unlock();
          sock->Send(message);
        }
      else if (strncmp(request, "Command", 7) == 0)
        {
          char objName[100], method[100];
          sock->Recv(objName, sizeof(objName));
          sock->Recv(method, sizeof(method));
          
          LockRootGuard lock;
          
          TObject *object = gROOT->FindObjectAny(objName);
          if (object && object->InheritsFrom(TH1::Class())
              && strcmp(method, "Reset") == 0)
            static_cast < TH1 * >(object)->Reset();
          
        }
      else if (strncmp(request, "SetCut", 6) == 0)
        {
          
          //read new settings for a cut
          char name[256];
          sock->Recv(name, sizeof(name));
          
          LockRootGuard lock;
          
          TCutG *cut = (TCutG *) gDir->FindObjectAny(name);
          
          TMessage *m = 0;
          sock->Recv(m);
          TCutG *newc = ((TCutG *) m->ReadObject(m->GetClass()));
          
          if (cut) {
            fprintf(stderr, "root server thread: changing cut %s\n", newc->GetName());
            newc->TAttMarker::Copy(*cut);
            newc->TAttFill::Copy(*cut);
            newc->TAttLine::Copy(*cut);
            newc->TNamed::Copy(*cut);
            cut->Set(newc->GetN());
            for (int i = 0; i < cut->GetN(); ++i) {
              cut->SetPoint(i, newc->GetX()[i], newc->GetY()[i]);
            }
          } else {
            fprintf(stderr, "root server thread: ignoring receipt of unknown cut \'%s\'\n",
                    newc->GetName());
          }
          delete newc;
          
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

   bool trace = true;
   int port = *(int *) arg;

   fprintf(stderr, "NetDirectory server listening on port %d...\n", port);
   TServerSocket *lsock = new TServerSocket(port, kTRUE);

   while (1) {
      TSocket *sock = lsock->Accept();

      if (sock==NULL) {
        printf("TNetDirectory server accept() error\n");
        break;
      }

      if (trace)
        fprintf(stderr, "TNetDirectory connection from %s\n", sock->GetInetAddress().GetHostName());

#if 1
      TThread *thread = new TThread("Server", root_server_thread, sock);
      thread->Run();
#else
      LPDWORD lpThreadId = 0;
      CloseHandle(CreateThread(NULL, 1024, &root_server_thread, sock, 0, lpThreadId));
#endif
   }

   return THREADRETURN;
}

/*------------------------------------------------------------------*/

void StartNetDirectoryServer(int port, TDirectory* dir)
{
  /* create the folder for analyzer histograms */
  gDir = dir;

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

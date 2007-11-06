/********************************************************************\

  Name:         mana.c
  Created by:   Stefan Ritt

  Contents:     The system part of the MIDAS analyzer. Has to be
                linked with analyze.c to form a complete analyzer

  $Id: mana.c 3326 2006-09-20 13:31:46Z ritt $

\********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "midasServer.h"

/*==== ROOT socket histo server ====================================*/

#if defined (OS_LINUX) || defined (OS_DARWIN)
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
#include <TSemaphore.h>
#include <TFolder.h>
#include <TSocket.h>
#include <TServerSocket.h>
#include <TThread.h>
#include <TMessage.h>
#include <TObjString.h>
#include <TObjArray.h>
#include <TH1.h>
#include <TCutG.h>

#include "libNetDirectory/RootLock.h"

static bool gVerbose   = false;

void VerboseMidasServer(bool verbose)
{
  fprintf(stderr, "VerboseMidasServer: change verbose level from %d to %d\n", gVerbose, verbose);
  gVerbose = verbose;
}

/*------------------------------------------------------------------*/

#include <map>
#include <string>

static std::map<uint32_t,std::string> gPointers;
static std::map<std::string,uint32_t> gRevPointers;
static uint32_t gLastPointer = 0;

TFolder *ReadFolderPointer(TSocket * fSocket)
{
   //read pointer to current folder
   TMessage *m = 0;
   fSocket->Recv(m);
   uint32_t p;
   *m >> p;

   const char* name = gPointers[p].c_str();

   if (gVerbose)
     printf("converted %d to \'%s\'\n", p, name);

   TObject* obj = gROOT->FindObjectAny(name);

   if (!obj)
     obj = gManaHistosFolder;

   return (TFolder*) obj;
}

/*------------------------------------------------------------------*/

THREADTYPE root_server_thread(void *arg)
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

      if (gVerbose)
        printf("Request %s\n", request);

      if (strcmp(request, "GetListOfFolders") == 0) {

        LockRootGuard lock;
        
        TFolder *folder = ReadFolderPointer(sock);
        if (folder == NULL) {
          message.Reset(kMESS_OBJECT);
          message.WriteObject(NULL);
          lock.Unlock();
          sock->Send(message);
          continue;
        }
        //get folder names
        TObject *obj;
        TObjArray *names = new TObjArray(100);
        
        TCollection *folders = folder->GetListOfFolders();
        TIterator *iterFolders = folders->MakeIterator();
        while ((obj = iterFolders->Next()) != NULL)
          names->Add(new TObjString(obj->GetName()));
        
        //write folder names
        message.Reset(kMESS_OBJECT);
        message.WriteObject(names);
        sock->Send(message);
        
        for (int i = 0; i < names->GetLast() + 1; i++)
          delete(TObjString *) names->At(i);
        
        delete names;
        
      } else if (strncmp(request, "FindObject", 10) == 0) {
        
        LockRootGuard lock;

        TFolder *folder = ReadFolderPointer(sock);
        
        //get object
        TObject *obj;
        if (strncmp(request + 10, "Any", 3) == 0)
          obj = folder->FindObjectAny(request + 14);
        else
          obj = folder->FindObject(request + 11);
        
        //write object
        if (!obj)
          sock->Send("Error");
        else {
          message.Reset(kMESS_OBJECT);
          message.WriteObject(obj);

          lock.Unlock();
          sock->Send(message);
        }
        
      } else if (strncmp(request, "FindFullPathName", 16) == 0) {
        
        LockRootGuard lock;

        TFolder *folder = ReadFolderPointer(sock);
        
        //find path
        const char *path = folder->FindFullPathName(request + 17);
        
        //write path
        if (!path) {
          sock->Send("Error");
        } else {
          TObjString *obj = new TObjString(path);
          message.Reset(kMESS_OBJECT);
          message.WriteObject(obj);
          lock.Unlock();
          sock->Send(message);
          delete obj;
        }
        
      } else if (strncmp(request, "GetPointer", 10) == 0) {

        //find object
        uint32_t p = 0;
        TObject *obj = gROOT->FindObjectAny(request + 11);
        
        //write pointer
        message.Reset(kMESS_ANY);

        if (obj)
          {
            const char* name = obj->GetName();
            p = gRevPointers[name];
            if (p==0)
              {
                p = ++gLastPointer;
                gPointers[p] = name;
                gRevPointers[name] = p;
              }

            if (gVerbose)
              printf("give %d for \'%s\'\n", p, name);
          }

        message << p;
        sock->Send(message);
        
      } else if (strncmp(request, "Command", 7) == 0) {
        char objName[100], method[100];
        sock->Recv(objName, sizeof(objName));
        sock->Recv(method, sizeof(method));

        LockRootGuard lock;

        TObject *object = gROOT->FindObjectAny(objName);
        if (object && object->InheritsFrom(TH1::Class())
            && strcmp(method, "Reset") == 0)
          static_cast < TH1 * >(object)->Reset();
        
      } else if (strncmp(request, "SetCut", 6) == 0) {
        
        //read new settings for a cut
        char name[256];
        sock->Recv(name, sizeof(name));

        LockRootGuard lock;

        TCutG *cut = (TCutG *) gManaHistosFolder->FindObjectAny(name);
        
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
        
      } else {
        fprintf(stderr, "midasServer: Received unknown request \"%s\"\n", request);
      }

   } while (1);

   return THREADRETURN;
}

/*------------------------------------------------------------------*/

THREADTYPE root_socket_server(void *arg)
{
// Server loop listening for incoming network connections on specified port.
// Starts a searver_thread for each connection.
   int port;

   port = *(int *) arg;

   printf("MIDAS ROOT server listening on port %d...\n", port);
   TServerSocket *lsock = new TServerSocket(port, kTRUE);

   do {
      TSocket *sock = lsock->Accept();

      if (sock==NULL) {
        fprintf(stderr, "MIDAS ROOT server accept() error\n");
        break;
      }

      // printf("Established connection to %s\n", sock->GetInetAddress().GetHostName());

#if defined ( __linux__ )
      TThread *thread = new TThread("Server", root_server_thread, sock);
      thread->Run();
#endif
#if defined( _MSC_VER )
      LPDWORD lpThreadId = 0;
      CloseHandle(CreateThread(NULL, 1024, &root_server_thread, sock, 0, lpThreadId));
#endif
   } while (1);

   return THREADRETURN;
}

/*------------------------------------------------------------------*/

TFolder* gManaHistosFolder = NULL;

void StartMidasServer(int port)
{
  /* create the folder for analyzer histograms */
  gManaHistosFolder = gROOT->GetRootFolder()->AddFolder("histos", "MIDAS Analyzer Histograms");
  gROOT->GetListOfBrowsables()->Add(gManaHistosFolder, "histos");

  StartLockRootTimer();
  
  static int pport = port;
#if defined (OS_LINUX)
  TThread *thread = new TThread("server_loop", root_socket_server, &pport);
  thread->Run();
#endif
#if defined( _MSC_VER )
  LPDWORD lpThreadId = 0;
  CloseHandle(CreateThread(NULL, 1024, &root_socket_server, &pport, 0, lpThreadId));
#endif
}

// end

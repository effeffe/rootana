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

#include <TFolder.h>
#include <TSocket.h>
#include <TServerSocket.h>
#include <TThread.h>
#include <TMessage.h>
#include <TObjString.h>
#include <TH1.h>
#include <TCutG.h>

typedef void* POINTER_T;

/*------------------------------------------------------------------*/

TFolder *ReadFolderPointer(TSocket * fSocket)
{
   //read pointer to current folder
   TMessage *m = 0;
   fSocket->Recv(m);
   uint32_t p;
   *m >> p;
   return (TFolder *) p;
}

/*------------------------------------------------------------------*/

THREADTYPE root_server_thread(void *arg)
/*
  Serve histograms over TCP/IP socket link
*/
{
   char request[256];

   TSocket *sock = (TSocket *) arg;

   do {

      /* close connection if client has disconnected */
      if (sock->Recv(request, sizeof(request)) <= 0) {
         // printf("Closed connection to %s\n", sock->GetInetAddress().GetHostName());
         sock->Close();
         delete sock;
         return THREADRETURN;

      } else {

         TMessage *message = new TMessage(kMESS_OBJECT);

         if (strcmp(request, "GetListOfFolders") == 0) {

            TFolder *folder = ReadFolderPointer(sock);
            if (folder == NULL) {
               message->Reset(kMESS_OBJECT);
               message->WriteObject(NULL);
               sock->Send(*message);
               delete message;
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
            message->Reset(kMESS_OBJECT);
            message->WriteObject(names);
            sock->Send(*message);

            for (int i = 0; i < names->GetLast() + 1; i++)
               delete(TObjString *) names->At(i);

            delete names;

            delete message;

         } else if (strncmp(request, "FindObject", 10) == 0) {

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
               message->Reset(kMESS_OBJECT);
               message->WriteObject(obj);
               sock->Send(*message);
            }
            delete message;

         } else if (strncmp(request, "FindFullPathName", 16) == 0) {

            TFolder *folder = ReadFolderPointer(sock);

            //find path
            const char *path = folder->FindFullPathName(request + 17);

            //write path
            if (!path) {
               sock->Send("Error");
            } else {
               TObjString *obj = new TObjString(path);
               message->Reset(kMESS_OBJECT);
               message->WriteObject(obj);
               sock->Send(*message);
               delete obj;
            }
            delete message;

         } else if (strncmp(request, "Occurence", 9) == 0) {

            TFolder *folder = ReadFolderPointer(sock);

            //read object
            TMessage *m = 0;
            sock->Recv(m);
            TObject *obj = ((TObject *) m->ReadObject(m->GetClass()));

            //get occurence
            Int_t retValue = folder->Occurence(obj);

            //write occurence
            message->Reset(kMESS_OBJECT);
            *message << retValue;
            sock->Send(*message);

            delete message;

         } else if (strncmp(request, "GetPointer", 10) == 0) {

            //find object
            TObject *obj = gROOT->FindObjectAny(request + 11);

            //write pointer
            message->Reset(kMESS_ANY);
            int p;
            memcpy(&p, &obj, 4);
            *message << p;
            sock->Send(*message);

            delete message;

         } else if (strncmp(request, "Command", 7) == 0) {
            char objName[100], method[100];
            sock->Recv(objName, sizeof(objName));
            sock->Recv(method, sizeof(method));
            TObject *object = gROOT->FindObjectAny(objName);
            if (object && object->InheritsFrom(TH1::Class())
                && strcmp(method, "Reset") == 0)
               static_cast < TH1 * >(object)->Reset();

         } else if (strncmp(request, "SetCut", 6) == 0) {

            //read new settings for a cut
            char name[256];
            sock->Recv(name, sizeof(name));
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
	      fprintf(stderr, "root server thread: ignoring receipt of unknown cut %s\n",
                      newc->GetName());
            }
            delete newc;

         } else
	    fprintf(stderr, "SocketServer: Received unknown command \"%s\"\n", request);
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

   printf("Root server listening on port %d...\n", port);
   TServerSocket *lsock = new TServerSocket(port, kTRUE);

   do {
      TSocket *sock = lsock->Accept();

      if (sock==NULL) {
        printf("Root server accept() error\n");
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
  
  static int pport = port;
#if defined ( __linux__ )
  TThread *thread = new TThread("server_loop", root_socket_server, &pport);
  thread->Run();
#endif
#if defined( _MSC_VER )
  LPDWORD lpThreadId = 0;
  CloseHandle(CreateThread(NULL, 1024, &root_socket_server, &pport, 0, lpThreadId));
#endif
}

// end

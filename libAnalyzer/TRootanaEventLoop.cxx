// Nothing here..

// Rootana includes
#include "TRootanaEventLoop.hxx"
#include "XmlOdb.h"
#ifdef OLD_SERVER
#include "midasServer.h"
#endif
#ifdef HAVE_LIBNETDIRECTORY
#include "libNetDirectory/netDirectoryServer.h"
#endif
#include "TPeriodicClass.hxx"
#include "MainWindow.hxx"

// ROOT includes.
#include <TSystem.h>
#include <TROOT.h>
#include <TH1D.h>

#include <stdio.h>
#include <sys/time.h>
#include <iostream>
#include <assert.h>
#include <signal.h>


TRootanaEventLoop* TRootanaEventLoop::fTRootanaEventLoop = NULL;

TRootanaEventLoop& TRootanaEventLoop::Get(void) {
  
  if(!fTRootanaEventLoop){
    std::cerr << "Singleton Not Instantiated! " 
	      << " Need to call something like SomeClass::CreateSingleton<SomeClass>(); Exiting!"
	      <<std::endl; exit(0);
  }
  return *fTRootanaEventLoop;
}
  


TRootanaEventLoop::TRootanaEventLoop (){

  fOutputFile = 0;
  fODB = 0;
  fOnlineHistDir = 0;
  fMaxEvents = 0;
  fCurrentRunNumber = 0;
  fIsOffline = true;
  fCreateMainWindow = true;

  /// Create the TApplication
  char **argv2 = NULL;
  fApp = new TApplication("rootana", 0, argv2);

}
 
TRootanaEventLoop::~TRootanaEventLoop (){

  if(fODB) delete fODB;
  if(fOutputFile){
    fOutputFile->Write();
    fOutputFile->Close();
    delete fOutputFile;
  }
}


void TRootanaEventLoop::Initialize(void){};
  
void TRootanaEventLoop::BeginRun(int transition,int run,int time){};

void TRootanaEventLoop::EndRun(int transition,int run,int time){};
  
void TRootanaEventLoop::Finalize(){};

void TRootanaEventLoop::Usage(void){};
  

bool TRootanaEventLoop::CheckOption(std::string option){return false;}


void TRootanaEventLoop::PrintHelp(){

  printf("\nUsage:\n");
  printf("\n./analyzer.exe [-h] [-Hhostname] [-Eexptname] [-eMaxEvents] [-P9091] [-p9090] [-m] [-g] [file1 file2 ...]\n");
  printf("\n");
  printf("\t-h: print this help message\n");
  printf("\t-T: test mode - start and serve a test histogram\n");
  printf("\t-Hhostname: connect to MIDAS experiment on given host\n");
  printf("\t-Eexptname: connect to this MIDAS experiment\n");
  printf("\t-P: Start the TNetDirectory server on specified tcp port (for use with roody -Plocalhost:9091)\n");
  printf("\t-p: Start the old midas histogram server on specified tcp port (for use with roody -Hlocalhost:9090)\n");
  printf("\t-e: Number of events to read from input data files\n");
  //printf("\t-m: Enable memory leak debugging\n");
  printf("\t-g: Enable graphics display when processing data files\n");
  Usage();  // Print description of user options.
  printf("\n");
  printf("Example1: analyze online data: ./analyzer.exe -P9091\n");
  printf("Example2: analyze existing data: ./analyzer.exe /data/alpha/current/run00500.mid\n");

  exit(1);
}


int TRootanaEventLoop::ExecuteLoop(int argc, char *argv[]){
  
  setbuf(stdout,NULL);
  setbuf(stderr,NULL);
  
  signal(SIGILL,  SIG_DFL);
  signal(SIGBUS,  SIG_DFL);
  signal(SIGSEGV, SIG_DFL);

  std::vector<std::string> args;
  for (int i=0; i<argc; i++)
    {
      if (strcmp(argv[i],"-h")==0)
	PrintHelp(); // does not return
      args.push_back(argv[i]);
    }
  
  
  if(gROOT->IsBatch()) {
    printf("Cannot run in batch mode\n");
    return 1;
  }

  bool forceEnableGraphics = false;
  bool testMode = false;
  int  tcpPort = 0;
  const char* hostname = NULL;
  const char* exptname = NULL;
  
  for (unsigned int i=1; i<args.size(); i++) // loop over the commandline options
    {
      const char* arg = args[i].c_str();
      //printf("argv[%d] is %s\n",i,arg);
      
      if (strncmp(arg,"-e",2)==0)  // Event cutoff flag (only applicable in offline mode)
	fMaxEvents = atoi(arg+2);
      else if (strncmp(arg,"-m",2)==0) // Enable memory debugging
	;//	 gEnableShowMem = true;
      else if (strncmp(arg,"-P",2)==0) // Set the histogram server port
	tcpPort = atoi(arg+2);
      else if (strcmp(arg,"-T")==0)
	testMode = true;
      else if (strcmp(arg,"-g")==0)
	forceEnableGraphics = true;
      else if (strncmp(arg,"-H",2)==0)
	hostname = strdup(arg+2);
      else if (strncmp(arg,"-E",2)==0)
	exptname = strdup(arg+2);
      else if (strcmp(arg,"-h")==0)
	PrintHelp(); // does not return
      else if(arg[0] == '-' && !CheckOption(args[i])) // Chec if a user-defined option	
	PrintHelp(); // does not return
    }
    
  Initialize();

  MainWindow *mainWindow=0;
  if(fCreateMainWindow){
    mainWindow = new MainWindow(gClient->GetRoot(), 200, 300);
  }

   gROOT->cd();
   fOnlineHistDir = new TDirectory("rootana", "rootana online plots");

#ifdef HAVE_LIBNETDIRECTORY
   if (tcpPort)
     StartNetDirectoryServer(tcpPort, fOnlineHistDir);
#else
   if (tcpPort)
     fprintf(stderr,"ERROR: No support for the TNetDirectory server!\n");
#endif
	 
   fIsOffline = false;

   for (unsigned int i=1; i<args.size(); i++){
     const char* arg = args[i].c_str();
     if (arg[0] != '-')  
       {  
	 fIsOffline = true;
	 ProcessMidasFile(fApp,arg);
       }
   }


   if (testMode){
     std::cout << "Entering test mode." << std::endl;
     fOnlineHistDir->cd();
     TH1D* hh = new TH1D("test", "test", 100, 0, 100);
     hh->Fill(1);
     hh->Fill(10);
     hh->Fill(50);
     
     fApp->Run(kTRUE);
     if(fCreateMainWindow) delete mainWindow;
     return 0;
   }

   // if we processed some data files,
   // do not go into online mode.
   if (fIsOffline){
     if(fCreateMainWindow) delete mainWindow;
     return 0;
   }
	   
#ifdef HAVE_MIDAS
   ProcessMidasOnline(fApp, hostname, exptname);;
#endif
   
   if(fCreateMainWindow) delete mainWindow;
   
   Finalize();
   
   return 0;
  
}



int TRootanaEventLoop::ProcessMidasFile(TApplication*app,const char*fname)
{
  TMidasFile f;
  bool tryOpen = f.Open(fname);

  if (!tryOpen){
    printf("Cannot open input file \"%s\"\n",fname);
    return -1;
  }

  int i=0;
  while (1)
    {
      TMidasEvent event;
      if (!f.Read(&event))
	break;

      /// Treat the begin run and end run events differently.
      int eventId = event.GetEventId();

      if ((eventId & 0xFFFF) == 0x8000){// begin run event
	
	event.Print();
	
	// Load ODB contents from the ODB XML file
	if (fODB) delete fODB;
	fODB = new XmlOdb(event.GetData(),event.GetDataSize());
	
	BeginRun(0,event.GetSerialNumber(),0);
	fCurrentRunNumber = event.GetSerialNumber();

      } else if ((eventId & 0xFFFF) == 0x8001){// end run event
	  
	event.Print();
	

      } else { // all other events

	event.SetBankList();
	ProcessEvent(event);

      }
	
      if((i%500)==0){
	printf("Processing event %d\n",i);
      }
      
      // Check if we have processed desired number of events.
      i++;
      if ((fMaxEvents!=0)&&(i>=fMaxEvents)){
	printf("Reached event %d, exiting loop.\n",i);
	break;
      }
    }
  
  f.Close(); 

  EndRun(0,fCurrentRunNumber,0);

  // start the ROOT GUI event loop
  //  app->Run(kTRUE);

  return 0;
}


/// _________________________________________________________________________
/// _________________________________________________________________________
/// _________________________________________________________________________
/// The following code is only applicable for online MIDAS programs

#ifdef HAVE_MIDAS

// This global variable allows us to keep track of whether we are already in the process
// of analyzing a particular event. 
static bool onlineEventLock = false;

/// We need to use a regular function, so that it can be passed 
/// to the TMidasOnline event handler.  This function calles the 
/// event loop singleton, allowing the user to add their own function code. 
void onlineEventHandler(const void*pheader,const void*pdata,int size)
{

  // If we are already processing a previous event, then just dump this one.
  // !!!!!!!!!!! This is adding a potential dangerous race condition!!!!!
  // !!!!!!!!!!! Need to think hard if this is safe!!!!!!!!!!!!!!!!!!!!!!
  if(onlineEventLock) return;
  onlineEventLock = true;

  // Make a MIDAS event.
  TMidasEvent event;
  memcpy(event.GetEventHeader(), pheader, sizeof(EventHeader_t));
  event.SetData(size, (char*)pdata);
  event.SetBankList();
  

  // Now pass this to the user event function.
  TRootanaEventLoop::Get().ProcessEvent(event);
  onlineEventLock = false;
}


void onlineBeginRunHandler(int transition,int run,int time)
{
  //  if(gOutputFile!=NULL)
  //{
  //  gOutputFile->Write();
  //  gOutputFile->Close();
  //  gOutputFile=NULL;
  //}  

  //char filename[1024];
  //sprintf(filename, "output%05d.root", run);
  //gOutputFile = new TFile(filename,"RECREATE");
  //printf("gOutputFile: %p, isOpen %d\n", gOutputFile, gOutputFile->IsOpen());
  //assert(gOutputFile);
  //assert(gOutputFile->IsOpen());

  //#ifdef HAVE_LIBNETDIRECTORY
  //  NetDirectoryExport(gOutputFile, "outputFile");
  //#endif
  TRootanaEventLoop::Get().SetCurrentRunNumber(run);
  TRootanaEventLoop::Get().BeginRun(transition,run,time);
}

void onlineEndRunHandler(int transition,int run,int time)
{
  TRootanaEventLoop::Get().SetCurrentRunNumber(run);
  TRootanaEventLoop::Get().EndRun(transition,run,time);
}


void MidasPollHandlerLocal()
{
  if (!(TMidasOnline::instance()->poll(0)))
    gSystem->ExitLoop();
}

int TRootanaEventLoop::ProcessMidasOnline(TApplication*app, const char* hostname, const char* exptname)
{
   TMidasOnline *midas = TMidasOnline::instance();

   int err = midas->connect(hostname, exptname, "rootana");
   if (err != 0)
     {
       fprintf(stderr,"Cannot connect to MIDAS, error %d\n", err);
       return -1;
     }

   fODB = midas;

   // Register begin and end run handlers.
   midas->setTransitionHandlers(onlineBeginRunHandler,onlineEndRunHandler,NULL,NULL);
   midas->registerTransitions();

   /* reqister event requests */
   midas->setEventHandler(onlineEventHandler);

   midas->eventRequest("SYSTEM",-1,-1,(1<<1));

   /* fill present run parameters */

   fCurrentRunNumber = fODB->odbReadInt("/runinfo/Run number");

   //   if ((fODB->odbReadInt("/runinfo/State") == 3))
   //startRun(0,gRunNumber,0);

   // printf("Startup: run %d, is running: %d, is pedestals run: %d\n",gRunNumber,gIsRunning,gIsPedestalsRun);
   
   TPeriodicClass tm(100,MidasPollHandlerLocal);

   /*---- start main loop ----*/

   //loop_online();
   app->Run(kTRUE); // kTRUE means return to here after finished with online processing... this ensures that we can disconnect.

   /* disconnect from experiment */
   midas->disconnect();

   return 0;
}

#endif
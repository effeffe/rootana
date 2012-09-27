//
// ROOT analyzer
//
// K.Olchanski
//
// $Id$
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>
#include <assert.h>
#include <signal.h>

#ifdef HAVE_MIDASSERVER
#include "midasServer.h"
#endif
#ifdef HAVE_LIBNETDIRECTORY
#include "netDirectoryServer.h"
#endif
#ifdef HAVE_XMLSERVER
#include "xmlServer.h"
#endif

#include <TSystem.h>
#include <TROOT.h>
#include <TApplication.h>
#include <TTimer.h>
#include <TFile.h>
#include <TDirectory.h>
#include <TFolder.h>
#include <TH1D.h>
//#include <TBrowser.h>

TDirectory* gOnlineHistDir = NULL;
TFile* gOutputFile = NULL;

double GetTimeSec()
{
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec + 0.000001*tv.tv_usec;
}

class MyPeriodic : public TTimer
{
public:
  typedef void (*TimerHandler)(void);

  int          fPeriod_msec;
  TimerHandler fHandler;
  double       fLastTime;

  MyPeriodic(int period_msec,TimerHandler handler)
  {
    assert(handler != NULL);
    fPeriod_msec = period_msec;
    fHandler  = handler;
    fLastTime = GetTimeSec();
    Start(period_msec,kTRUE);
  }

  Bool_t Notify()
  {
    double t = GetTimeSec();
    //printf("timer notify, period %f should be %f!\n",t-fLastTime,fPeriod_msec*0.001);

    if (t - fLastTime >= 0.9*fPeriod_msec*0.001)
      {
	//printf("timer: call handler %p\n",fHandler);
	if (fHandler)
	  (*fHandler)();

	fLastTime = t;
      }

    Reset();
    return kTRUE;
  }

  ~MyPeriodic()
  {
    TurnOff();
  }
};

static bool gEnableShowMem = false;

int ShowMem(const char* label)
{
  if (!gEnableShowMem)
    return 0;

  FILE* fp = fopen("/proc/self/statm","r");
  if (!fp)
    return 0;

  int mem = 0;
  fscanf(fp,"%d",&mem);
  fclose(fp);

  if (label)
    printf("memory at %s is %d\n", label, mem);

  return mem;
}

void help()
{
  printf("\nUsage:\n");

  printf("\n./test_midasServer.exe [-h] [-PtcpPort] [-pTcpPort] [-XtmpPort] [-m]\n");
  printf("\n");
  printf("\t-h: print this help message\n");
  printf("\t-X: Start the XML Server on specified tcp port (for use with roody)\n");
  printf("\t-P: Start the TNetDirectory server on specified tcp port (for use with roody)\n");
  printf("\t-p: Start the old midas histogram server on specified tcp port (for use with roody)\n");
  printf("\t-m: Enable memory leak debugging\n");
  printf("\n");

  exit(1);
}

void IncrFunc()
{
  TH1D *h;
  h = (TH1D*)gROOT->FindObjectAny("test1");
  //printf("Histogram %p\n", h);
  if (h)
    {
      static int x = 0;
      h->Fill(x);
      x++;
      if (x>100)
        x = 0;
    }

  h = (TH1D*)gROOT->FindObjectAny("test3");
  //printf("Histogram %p\n", h);
  if (h)
    {
      h->Fill(33);
    }
}

// Main function call

int main(int argc, char *argv[])
{
   setbuf(stdout,NULL);
   setbuf(stderr,NULL);
 
   signal(SIGILL,  SIG_DFL);
   signal(SIGBUS,  SIG_DFL);
   signal(SIGSEGV, SIG_DFL);
   signal(SIGPIPE, SIG_DFL);
 
   std::vector<std::string> args;
   for (int i=0; i<argc; i++)
     {
       if (strcmp(argv[i],"-h")==0)
	 help(); // does not return
       args.push_back(argv[i]);
     }

   TApplication *app = new TApplication("rootana", &argc, argv);

   if(gROOT->IsBatch()) {
   	printf("Cannot run in batch mode\n");
	return 1;
   }

   int  oldTcpPort = 0;
   int  tcpPort = 0;
   int  xmlTcpPort = 0;

   for (unsigned int i=1; i<args.size(); i++) // loop over the commandline options
     {
       const char* arg = args[i].c_str();
       //printf("argv[%d] is %s\n",i,arg);
	   
       if (false)
         ;
       else if (strncmp(arg,"-m",2)==0) // Enable memory debugging
	 gEnableShowMem = true;
       else if (strncmp(arg,"-p",2)==0) // Set the histogram server port
	 oldTcpPort = atoi(arg+2);
       else if (strncmp(arg,"-P",2)==0) // Set the histogram server port
	 tcpPort = atoi(arg+2);
       else if (strncmp(arg,"-X",2)==0) // Set the histogram server port
	 xmlTcpPort = atoi(arg+2);
       else if (strcmp(arg,"-h")==0)
	 help(); // does not return
       else if (arg[0] == '-')
	 help(); // does not return
    }
    
   gROOT->cd();
   gOnlineHistDir = new TDirectory("rootana", "rootana online plots");

#ifdef HAVE_MIDASSERVER
   if (oldTcpPort)
     StartMidasServer(oldTcpPort);
#else
   if (oldTcpPort)
     fprintf(stderr,"ERROR: No support for the old midas server!\n");
#endif
#ifdef HAVE_LIBNETDIRECTORY
   if (tcpPort)
     {
       //VerboseNetDirectoryServer(true);
       StartNetDirectoryServer(tcpPort, gOnlineHistDir);
     }
#else
   if (tcpPort)
     fprintf(stderr,"ERROR: No support for the TNetDirectory server!\n");
#endif
#ifdef HAVE_XMLSERVER
   XmlServer* xmlServer = NULL;
   if (xmlTcpPort)
     {
        xmlServer = new XmlServer();
        xmlServer->SetVerbose(true);
        xmlServer->Start(xmlTcpPort);
        xmlServer->Export(gOnlineHistDir, gOnlineHistDir->GetName());
     }
#else
   if (xmlTcpPort)
     fprintf(stderr,"ERROR: No support for the XML Server!\n");
#endif
	 
   gOnlineHistDir->cd();

   //TFile *f = new TFile("/Users/olchansk/output.root", "RECREATE");
   TFile *f = new TFile("output.root", "RECREATE");
   f->cd();

   NetDirectoryExport(f, "outputFile");

   if (xmlServer)
      xmlServer->Export(f, "outputFile");

   TH1D* hh = new TH1D("test1", "test1", 100, 0, 100);
   hh->Fill(1);
   hh->Fill(10);
   hh->Fill(50);

   TDirectory* subdir = gDirectory->mkdir("subdir with space");
   subdir->cd();

   TH1D* hh2 = new TH1D("test2 with space", "test2", 100, 0, 100);
   hh2->Fill(25);

   TFolder* subfolder = new TFolder("subfolder", "subfolder");
   f->Add(subfolder);

   TH1D* hh3 = new TH1D("test3", "test3", 100, 0, 100);
   hh3->Fill(33);
   subfolder->Add(hh3);

   //f->Write();
   //f->Close();

#ifdef HAVE_MIDASSERVER
   if (oldTcpPort>0 && gManaHistosFolder)
     {
       NetDirectoryExport(gManaHistosFolder, "histos");

       TFolder *subfolder = gManaHistosFolder->AddFolder("subfolder", "Sub folder");
       subfolder->Add(hh2);

       gManaHistosFolder->Add(hh);
     }
#endif

   //gDirectory->cd("/");
   //gDirectory->pwd();
   //gROOT->cd();

   //NetDirectoryExport(gROOT->GetListOfFiles(), "ListOfFiles");
   //NetDirectoryExport(gROOT->GetListOfGlobals(), "ListOfGlobals");

   if (xmlServer) {
      xmlServer->Export(gROOT->GetListOfFiles(), "ListOfFiles");
      //xmlServer->Export(gROOT->GetListOfGlobals(), "ListOfGlobals");
   }

   new MyPeriodic(100, IncrFunc);
   
   //new TBrowser();
   app->Run(kTRUE);
   return 0;
}

//end

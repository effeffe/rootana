//
// MIDAS analyzer
//
// K.Olchanski
//

#include <stdio.h>
#include <sys/time.h>
#include <iostream>
#include <assert.h>
#include <signal.h>

#include "manalyzer.h"
#include "midasio.h"

TARunInfo::TARunInfo(int runno, const std::string& filename)
{
   printf("TARunInfo::ctor!\n");
   fRunNo = runno;
   fFileName = filename;
   fOdb = NULL;
}

TARunInfo::~TARunInfo()
{
   fRunNo = 0;
   fFileName = "(deleted)";
   if (fOdb) {
      delete fOdb;
      fOdb = NULL;
   }
}


std::vector<TAModuleInterface*> *gModules = NULL;

TARegisterModule::TARegisterModule(TAModuleInterface* m)
{
   if (!gModules)
      gModules = new std::vector<TAModuleInterface*>;
   gModules->push_back(m);
}

#include "VirtualOdb.h"
#ifdef HAVE_MIDAS
#include "TMidasOnline.h"
#endif
//#include "TMidasEvent.h"
//#include "TMidasFile.h"
#ifdef HAVE_ROOT
#include "XmlOdb.h"
#endif
#ifdef HAVE_MIDASSERVER
#include "midasServer.h"
#endif
#ifdef HAVE_LIBNETDIRECTORY
#include "libNetDirectory/netDirectoryServer.h"
#endif
#ifdef HAVE_XMLSERVER
#include "libXmlServer/xmlServer.h"
#endif

#ifdef HAVE_ROOT
#include <TSystem.h>
#include <TROOT.h>
#include <TFile.h>
#include <TDirectory.h>
#include <TFolder.h>

TDirectory* gOnlineHistDir = NULL;
#endif

VirtualOdb* gOdb = NULL;

double GetTimeSec()
{
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec + 0.000001*tv.tv_usec;
}

#ifdef HAVE_ROOT
TARootRunInfo::TARootRunInfo(const TARunInfo* runinfo) // ctor
   : TARunInfo(*runinfo)
{
   printf("TARootRunInfo::ctor!\n");

   char xfilename[1024];
   sprintf(xfilename, "output%05d.root", fRunNo);
   fRootFile = new TFile(xfilename, "RECREATE");
   
   assert(fRootFile); // FIXME: new never returns NULL
   assert(fRootFile->IsOpen()); // FIXME: survive failure to open ROOT file
}

TARootRunInfo::~TARootRunInfo() // dtor
{
   printf("TARootRunInfo::dtor!\n");
   
   if (fRootFile != NULL) {
      fRootFile->Write();
      fRootFile->Close();
      fRootFile = NULL;
   }
}
#endif

#ifdef HAVE_MIDAS
//#ifdef HAVE_ROOT
//static TARootRunInfo* gRunInfo = NULL;
//#else
static TARunInfo* gRunInfo = NULL;
//#endif

void startRun(int transition,int run,int time)
{
  if (gRunInfo) {
    if (gRunInfo->fRunNo != run) {
      delete gRunInfo;
      gRunInfo = NULL;
    }
  }

  if (!gRunInfo) {
#ifdef HAVE_ROOT
    gRunInfo = new TARootRunInfo(run, "unknown");
#else
    gRunInfo = new TARunInfo(run, "unknown");
#endif
  }
  
  printf("Begin run: %d\n", run);

#ifdef HAVE_LIBNETDIRECTORY
  NetDirectoryExport(gOutputFile, "outputFile");
#endif

  for (unsigned i=0; i<(*gModules).size(); i++)
    (*gModules)[i]->StartRun(gRunInfo);
}

void endRun(int transition,int run,int time)
{
  for (unsigned i=0; i<(*gModules).size(); i++)
    (*gModules)[i]->EndRun(gRunInfo);

  printf("End of run %d\n", run);
}

void HandleMidasEvent(TMidasEvent& event)
{
   for (unsigned i=0; i<(*gModules).size(); i++)
      (*gModules)[i]->Analyze(gRunInfo, &event);
}

void eventHandler(const void*pheader,const void*pdata,int size)
{
  TMidasEvent event;
  memcpy(event.GetEventHeader(), pheader, sizeof(TMidas_EVENT_HEADER));
  event.SetData(size, (char*)pdata);
  event.SetBankList();
  HandleMidasEvent(event);
}

void MidasPollHandler()
{
  if (!(TMidasOnline::instance()->poll(0)))
    gSystem->ExitLoop();
}

int ProcessMidasOnline(const char* hostname, const char* exptname)
{
   TMidasOnline *midas = TMidasOnline::instance();

   int err = midas->connect(hostname, exptname, "rootana");
   if (err != 0)
     {
       fprintf(stderr,"Cannot connect to MIDAS, error %d\n", err);
       return -1;
     }

   gOdb = midas;

   midas->setTransitionHandlers(startRun,endRun,NULL,NULL);
   midas->registerTransitions();

   /* reqister event requests */

   midas->setEventHandler(eventHandler);
   midas->eventRequest("SYSTEM",-1,-1,(1<<1));

   /* fill present run parameters */

   int runno = gOdb->odbReadInt("/runinfo/Run number");

   if ((gOdb->odbReadInt("/runinfo/State") == 3)) // running state
     startRun(0,runno,0);

   printf("Startup: run %d\n", runno);
   
   MyPeriodic tm(100,MidasPollHandler);
   //MyPeriodic th(1000,SISperiodic);
   //MyPeriodic tn(1000,StepThroughSISBuffer);
   //MyPeriodic to(1000,Scalerperiodic);

   /*---- start main loop ----*/

   //loop_online();
   app->Run(kTRUE);

   /* disconnect from experiment */
   midas->disconnect();

   return 0;
}

#endif

int ProcessMidasFiles(const std::vector<std::string>& args, int num_skip, int num_analyze)
{
   for (unsigned i=0; i<(*gModules).size(); i++)
      (*gModules)[i]->Init(args);

   TARunInfo* runinfo = NULL;

   bool done = false;
   bool need_end_run = true;

   for (unsigned i=1; i<args.size(); i++) {
      if (args[i][0] == '-') // skip command line options
         continue;

      std::string filename = args[i];

      TMReaderInterface *reader = TMNewReader(filename.c_str());

      while (1) {
         TMEvent* event = TMReadEvent(reader);

         if (!event) // EOF
            break;

         if (event->error) {
            delete event;
            break;
         }

         if (event->event_id == 0x8000) // begin of run event
            {
               int runno = event->serial_number;

               if (runinfo) {
                  if (runinfo->fRunNo == runno) {
                     // next subrun file, nothing to do
                     runinfo->fFileName = filename;
                  } else {
                     // file with a different run number
                     if (need_end_run) {
                        need_end_run = false;
                        for (unsigned i=0; i<(*gModules).size(); i++)
                           (*gModules)[i]->EndRun(runinfo);
                        delete runinfo;
                        runinfo = NULL;
                     }
                  }
               }

               //
               // Load ODB contents from the ODB XML file
               //
               //if (gOdb)
               //   delete gOdb;
               //gOdb = new XmlOdb(event.GetData(),event.GetDataSize());

               if (!runinfo) {
                  runinfo = new TARunInfo(runno, filename);
                  for (unsigned i=0; i<(*gModules).size(); i++)
                     (*gModules)[i]->BeginRun(runinfo);
                  need_end_run = true;
               }

               for (unsigned i=0; i<(*gModules).size(); i++)
                  (*gModules)[i]->AnalyzeSpecialEvent(runinfo, event);
            }
         else if (event->event_id == 0x8001) // end of run event
            {
               for (unsigned i=0; i<(*gModules).size(); i++)
                  (*gModules)[i]->AnalyzeSpecialEvent(runinfo, event);

               //int runno = event->serial_number;
               
               for (unsigned i=0; i<(*gModules).size(); i++)
                  (*gModules)[i]->EndRun(runinfo);
               
               need_end_run = false;
            }
         else if (event->event_id == 0x8002) // message event
            {
               for (unsigned i=0; i<(*gModules).size(); i++)
                  (*gModules)[i]->AnalyzeSpecialEvent(runinfo, event);
            }
         else
            {
               if (!runinfo) {
                  // missing the begin of run
                  runinfo = new TARunInfo(0, filename);
                  for (unsigned i=0; i<(*gModules).size(); i++)
                     (*gModules)[i]->BeginRun(runinfo);
                  need_end_run = true;
               }

               if (num_skip > 0) {
                  num_skip--;
               } else {
                  if (num_analyze > 0) {
                     num_analyze--;
                     for (unsigned i=0; i<(*gModules).size(); i++)
                        (*gModules)[i]->Analyze(runinfo, event);
                  } else {
                     done = true;
                     break;
                  }
               }
            }

         delete event;

         if (done)
            break;
      }

      delete reader;

      if (done)
         break;
   }

   if (runinfo && need_end_run)
      for (unsigned i=0; i<(*gModules).size(); i++)
         (*gModules)[i]->EndRun(runinfo);

   delete runinfo;
   runinfo = NULL;
   
   for (unsigned i=0; i<(*gModules).size(); i++)
      (*gModules)[i]->Finish();
   
   return 0;
}

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

class EventDumpModule: public TAModuleInterface
{
public:
   //TARunInfo* NewRunInfo()
   //{
   //   return new TARunInfo;
   //}
   
   void Init(const std::vector<std::string> &args)
   {
      printf("EventDumpModule::Init!\n");
   }
   
   void Finish()
   {
      printf("EventDumpModule::Finish!\n");
   }
   
   void BeginRun(TARunInfo* runinfo)
   {
      printf("EventDumpModule::BeginRun, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
   }

   void EndRun(TARunInfo* runinfo)
   {
      printf("EventDumpModule::EndRun, run %d\n", runinfo->fRunNo);
   }

   void PauseRun(TARunInfo* runinfo)
   {
      printf("EventDumpModule::PauseRun, run %d\n", runinfo->fRunNo);
   }

   void ResumeRun(TARunInfo* runinfo)
   {
      printf("EventDumpModule::ResumeRun, run %d\n", runinfo->fRunNo);
   }

   void Analyze(TARunInfo* runinfo, TMEvent* event)
   {
      printf("EventDumpModule::Analyze, run %d, ", runinfo->fRunNo);
      event->FindAllBanks();
      std::string h = event->HeaderToString();
      std::string b = event->BankListToString();
      printf("%s: %s\n", h.c_str(), b.c_str());
   }

   void AnalyzeSpecialEvent(TARunInfo* runinfo, TMEvent* event)
   {
      printf("EventDumpModule::AnalyzeSpecialEvent, run %d, event serno %d, id 0x%04x, data size %d\n", runinfo->fRunNo, event->serial_number, (int)event->event_id, event->data_size);
   }
};

void help()
{
  printf("\nUsage:\n");
  printf("\n./analyzer.exe [-h] [-Hhostname] [-Eexptname] [-eMaxEvents] [-P9091] [-p9090] [-m] [file1 file2 ...]\n");
  printf("\n");
  printf("\t-h: print this help message\n");
  printf("\t-T: test mode - start and serve a test histogram\n");
  printf("\t-Hhostname: connect to MIDAS experiment on given host\n");
  printf("\t-Eexptname: connect to this MIDAS experiment\n");
  printf("\t-P: Start the TNetDirectory server on specified tcp port (for use with roody -Plocalhost:9091)\n");
  printf("\t-p: Start the old midas histogram server on specified tcp port (for use with roody -Hlocalhost:9090)\n");
  printf("\t-eNNN: Number of events to analyze\n");
  printf("\t-sNNN: Number of events to skip before starting analysis\n");
  printf("\t--dump: activate the event dump module\n");
  printf("\t-m: Enable memory leak debugging\n");
  printf("\t-g: Enable graphics display when processing data files\n");
  printf("\n");
  printf("Example1: analyze online data: ./analyzer.exe -P9091\n");
  printf("Example2: analyze existing data: ./analyzer.exe /data/alpha/current/run00500.mid\n");
  exit(1);
}

// Main function call

int main(int argc, char *argv[])
{
   setbuf(stdout, NULL);
   setbuf(stderr, NULL);
 
   signal(SIGILL,  SIG_DFL);
   signal(SIGBUS,  SIG_DFL);
   signal(SIGSEGV, SIG_DFL);
   signal(SIGPIPE, SIG_DFL);
   
   std::vector<std::string> args;
   for (int i=0; i<argc; i++) {
      if (strcmp(argv[i],"-h")==0)
         help(); // does not return
      args.push_back(argv[i]);
   }

   bool testMode = false;
   int  oldTcpPort = 0;
   int  tcpPort = 0;
   int  xmlTcpPort = 0;
   const char* hostname = NULL;
   const char* exptname = NULL;

   int num_skip = 0;
   int num_analyze = 0;

   bool event_dump = false;

   for (unsigned int i=1; i<args.size(); i++) { // loop over the commandline options
      const char* arg = args[i].c_str();
      //printf("argv[%d] is %s\n",i,arg);

      if (0) {
      } else if (args[i] == "--dump") {
         event_dump = true;
      } else if (strncmp(arg,"-s",2)==0) {
         num_skip = atoi(arg+2);
      } else if (strncmp(arg,"-e",2)==0) {
         num_analyze = atoi(arg+2);
      } else if (strncmp(arg,"-m",2)==0) { // Enable memory debugging
         gEnableShowMem = true;
      } else if (strncmp(arg,"-p",2)==0) // Set the histogram server port
         oldTcpPort = atoi(arg+2);
      else if (strncmp(arg,"-P",2)==0) // Set the histogram server port
         tcpPort = atoi(arg+2);
      else if (strncmp(arg,"-X",2)==0) // Set the histogram server port
         xmlTcpPort = atoi(arg+2);
      else if (strcmp(arg,"-T")==0)
         testMode = true;
      else if (strncmp(arg,"-H",2)==0)
         hostname = strdup(arg+2);
      else if (strncmp(arg,"-E",2)==0)
         exptname = strdup(arg+2);
      else if (strcmp(arg,"-h")==0)
         help(); // does not return
      else if (arg[0] == '-')
         help(); // does not return
   }

   if (!gModules)
      gModules = new std::vector<TAModuleInterface*>;

   if (event_dump)
      (*gModules).push_back(new EventDumpModule);

   printf("Registered modules: %d\n", (int)(*gModules).size());
   
#ifdef HAVE_ROOT
   gROOT->cd();
   gOnlineHistDir = new TDirectory("rootana", "rootana online plots");
#endif

#ifdef HAVE_MIDASSERVER
   if (oldTcpPort)
      StartMidasServer(oldTcpPort);
#else
   if (oldTcpPort)
      fprintf(stderr,"ERROR: No support for the old midas server!\n");
#endif
#ifdef HAVE_LIBNETDIRECTORY
   if (tcpPort)
      StartNetDirectoryServer(tcpPort, gOnlineHistDir);
#else
   if (tcpPort)
      fprintf(stderr,"ERROR: No support for the TNetDirectory server!\n");
#endif
#ifdef HAVE_XMLSERVER
   XmlServer* xmlServer = NULL;
   if (xmlTcpPort) {
      xmlServer = new XmlServer();
      xmlServer->SetVerbose(true);
      xmlServer->Start(xmlTcpPort);
      xmlServer->Export(gOnlineHistDir, gOnlineHistDir->GetName());
   }
#else
   if (xmlTcpPort)
      fprintf(stderr,"ERROR: No support for the XML Server!\n");
#endif
   
   bool run_from_file = false;
   
   for (unsigned int i=1; i<args.size(); i++) {
      if (args[i][0] != '-') {
         run_from_file = true;
         break;
      }
   }

   if (run_from_file) {
      ProcessMidasFiles(args, num_skip, num_analyze);
   } else {
#ifdef HAVE_MIDAS
      ProcessMidasOnline(hostname, exptname, num_analyze);
#endif
   }

   return 0;
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

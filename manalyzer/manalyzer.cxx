//
// MIDAS analyzer
//
// K.Olchanski
//

#include <stdio.h>
#include <assert.h>

#include "manalyzer.h"
#include "midasio.h"

//////////////////////////////////////////////////////////

static bool gTrace = false;

//////////////////////////////////////////////////////////
//
// Methods of TARunInfo
//
//////////////////////////////////////////////////////////

TARunInfo::TARunInfo(int runno, const char* filename, const std::vector<std::string>& args)
{
   if (gTrace)
      printf("TARunInfo::ctor!\n");
   fRunNo = runno;
   if (filename)
      fFileName = filename;
   fOdb = NULL;
#ifdef HAVE_ROOT
   fRoot = new TARootHelper(this);
#else
   fRoot = NULL;
#endif
   fMtInfo = NULL;
   fArgs = args;
}

TARunInfo::~TARunInfo()
{
   if (gTrace)
      printf("TARunInfo::dtor!\n");
   fRunNo = 0;
   fFileName = "(deleted)";
   if (fOdb) {
      delete fOdb;
      fOdb = NULL;
   }
#ifdef HAVE_ROOT
   if (fRoot) {
      delete fRoot;
      fRoot = NULL;
   }
#endif
   int count = 0;
   while (1) {
      TAFlowEvent* flow = ReadFlowQueue();
      if (!flow)
         break;
      delete flow;
      count++;
   }
   if (gTrace) {
      printf("TARunInfo::dtor: deleted %d queued flow events!\n", count);
   }
#ifdef HAVE_CXX11_THREADS
   if (fMtInfo) {
      delete fMtInfo;
      fMtInfo = NULL;
   }
#endif
}

//////////////////////////////////////////////////////////
//
// Methods of TAFlowEvent
//
//////////////////////////////////////////////////////////

TAFlowEvent::TAFlowEvent(TAFlowEvent* flow) // ctor
{
   if (gTrace)
      printf("TAFlowEvent::ctor: chain %p\n", flow);
   fNext = flow;
}

TAFlowEvent::~TAFlowEvent() // dtor
{
   if (gTrace)
      printf("TAFlowEvent::dtor: this %p, next %p\n", this, fNext);
   if (fNext)
      delete fNext;
   fNext = NULL;
}

//////////////////////////////////////////////////////////
//
// Methods of TARunObject
//
//////////////////////////////////////////////////////////

TARunObject::TARunObject(TARunInfo* runinfo)
{
   if (gTrace)
      printf("TARunObject::ctor, run %d\n", runinfo->fRunNo);
}

void TARunObject::BeginRun(TARunInfo* runinfo)
{
   //If modulename isn't defined in the users contructor. Make an automatic name
   //Issue: This breaks down if mutiple modules are the same class...
   //if (!ModuleName.size())
   //   ModuleName=typeid(*this).name();
   if (gTrace)
      printf("TARunObject::BeginRun, run %d\n", runinfo->fRunNo);
}

void TARunObject::EndRun(TARunInfo* runinfo)
{
   if (gTrace)
      printf("TARunObject::EndRun, run %d\n", runinfo->fRunNo);
}

void TARunObject::NextSubrun(TARunInfo* runinfo)
{
   if (gTrace)
      printf("TARunObject::NextSubrun, run %d\n", runinfo->fRunNo);
}

void TARunObject::PauseRun(TARunInfo* runinfo)
{
   if (gTrace)
      printf("TARunObject::PauseRun, run %d\n", runinfo->fRunNo);
}

void TARunObject::ResumeRun(TARunInfo* runinfo)
{
   if (gTrace)
      printf("TARunObject::ResumeRun, run %d\n", runinfo->fRunNo);
}

void TARunObject::PreEndRun(TARunInfo* runinfo)
{
   if (gTrace)
      printf("TARunObject::PreEndRun, run %d\n", runinfo->fRunNo);
}

TAFlowEvent* TARunObject::Analyze(TARunInfo* runinfo, TMEvent* event, TAFlags* flags, TAFlowEvent* flow)
{
   //Nothing happened, skip profiling
   (*flags)|=TAFlag_SKIP_PROFILE;
   if (gTrace)
      printf("TARunObject::Analyze!\n");
   return flow;
}

TAFlowEvent* TARunObject::AnalyzeFlowEvent(TARunInfo* runinfo, TAFlags* flags, TAFlowEvent* flow)
{
   //Nothing happened, skip profiling
   (*flags)|=TAFlag_SKIP_PROFILE;
   if (gTrace)
      printf("TARunObject::Analyze!\n");
   return flow;
}

void TARunObject::AnalyzeSpecialEvent(TARunInfo* runinfo, TMEvent* event)
{
   if (gTrace)
      printf("TARunObject::AnalyzeSpecialEvent!\n");
}

//////////////////////////////////////////////////////////
//
// Methods of TAFactory
//
//////////////////////////////////////////////////////////

void TAFactory::Usage()
{
   if (gTrace)
      printf("TAFactory::Usage!\n");
}

void TAFactory::Init(const std::vector<std::string> &args)
{
   if (gTrace)
      printf("TAFactory::Init!\n");
}

void TAFactory::Finish()
{
   if (gTrace)
      printf("TAFactory::Finish!\n");
}

#ifdef HAVE_ROOT

//////////////////////////////////////////////////////////
//
// Methods of TARootHelper
//
//////////////////////////////////////////////////////////

#ifdef HAVE_XMLSERVER
#include "xmlServer.h"
#include "TROOT.h"
#endif

#ifdef XHAVE_LIBNETDIRECTORY
#include "netDirectoryServer.h"
#endif

std::string   TARootHelper::fOutputFileName = "";
TApplication* TARootHelper::fgApp = NULL;
TDirectory*   TARootHelper::fgDir = NULL;
XmlServer*    TARootHelper::fgXmlServer = NULL;
THttpServer*  TARootHelper::fgHttpServer = NULL;

TARootHelper::TARootHelper(const TARunInfo* runinfo) // ctor
{
   if (gTrace)
      printf("TARootHelper::ctor!\n");
   char xfilename[1024];
   if (TARootHelper::fOutputFileName.empty())
   {
      sprintf(xfilename, "output%05d.root", runinfo->fRunNo);
      TARootHelper::fOutputFileName=xfilename;
   }

   //Get the last 5 charaters of the output file name
   std::string extension="";
   if (TARootHelper::fOutputFileName.size()>5)
      extension=TARootHelper::fOutputFileName.substr(TARootHelper::fOutputFileName.size()-5,5);
   //If .root wasn't specified, add it
   if (extension.compare(".root")!=0)
      TARootHelper::fOutputFileName+=".root";

   fOutputFile = new TFile(TARootHelper::fOutputFileName.data(), "RECREATE");
   
   assert(fOutputFile->IsOpen()); // FIXME: survive failure to open ROOT file

   fOutputFile->cd();

#ifdef XHAVE_LIBNETDIRECTORY
   NetDirectoryExport(fOutputFile, "ManalyzerOutputFile");
#endif
#ifdef HAVE_XMLSERVER
   if (fgXmlServer)
      fgXmlServer->Export(fOutputFile, "ManalyzerOutputFile");
#endif
}

TARootHelper::~TARootHelper() // dtor
{
   if (gTrace)
      printf("TARootHelper::dtor!\n");
   
   if (fOutputFile != NULL) {
      fOutputFile->Write();
      fOutputFile->Close();
      fOutputFile = NULL;
   }

   if (fgDir)
      fgDir->cd();
}

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>
#include <signal.h>

#include "manalyzer.h"
#include "midasio.h"

#ifdef HAVE_THTTP_SERVER
#include "THttpServer.h"
#endif

#ifdef HAVE_ROOT
#include <TSystem.h>
#endif

//////////////////////////////////////////////////////////
//
// Methods and Defaults of TAMultithreadHelper
//
//////////////////////////////////////////////////////////

#ifdef HAVE_CXX11_THREADS

static int gDefaultMultithreadQueueLength = 100;
static int gDefaultMultithreadWaitEmpty = 100; // microseconds
static int gDefaultMultithreadWaitFull = 100; // microseconds

TAMultithreadHelper::TAMultithreadHelper() // ctor
{
   // default max queue size
   fMtQueueDepth = gDefaultMultithreadQueueLength;
   // special end-of-run marker event
   fMtLastItemInQueue = new TAFlowEvent(NULL);
   // flag to shutdown the threads
   fMtShutdown = false;
   // quit flag from AnalyzeFlowEvent()
   fMtQuit = false;
   // queue settings
   fMtQueueFullUSleepTime  = gDefaultMultithreadWaitFull; //u seconds
   fMtQueueEmptyUSleepTime = gDefaultMultithreadWaitEmpty; //u seconds
}

TAMultithreadHelper::~TAMultithreadHelper() // dtor
{
   if (fMtLastItemInQueue) {
      delete fMtLastItemInQueue;
      fMtLastItemInQueue = NULL;
   }
   int nmodules = fMtFlowQueueMutex.size();

   // just for kicks, check that all queues have correct size
   assert(nmodules == (int)fMtFlowQueue.size());
   assert(nmodules == (int)fMtFlagQueue.size());
   assert(nmodules == (int)fMtFlowQueueMutex.size());

   // should not come to the destructor while threads are still running
   assert(fMtShutdown == true);

   int count = 0;
   for (int i=0; i<nmodules; i++) {
      // check that the thread is stopped
      //assert(!fMtThread[i].joinable());
      // empty the thread queue
      std::lock_guard<std::mutex> lock(fMtFlowQueueMutex[i]);
      while (!fMtFlowQueue[i].empty()) {
         TAFlowEvent* flow = fMtFlowQueue[i].front();
         TAFlags* flag = fMtFlagQueue[i].front();
         fMtFlowQueue[i].pop_front();
         fMtFlagQueue[i].pop_front();
         delete flow;
         delete flag;
         count++;
      }
      // implicit unlock of mutex
   }
   if (gTrace) {
      printf("TAMultithreadInfo::dtor: deleted %d queued flow events!\n", count);
   }
}

bool TAMultithreadHelper::gfMultithread           = false;
int TAMultithreadHelper::gfMtMaxBacklog           = 100;
std::mutex TAMultithreadHelper::gfLock; //Lock for modules to execute code that is not thread safe (many root fitting libraries)

static void MtQueueFlowEvent(TAMultithreadHelper* mt, int i, TAFlags* flag, TAFlowEvent* flow)
{
   assert(mt);

   if (flag == NULL) {
      flag = new TAFlags;
      *flag = 0;
   }

   //PrintQueueLength();

   while (1) {
      {
         //Lock and queue events
         std::lock_guard<std::mutex> lock(mt->fMtFlowQueueMutex[i]);
         
         if ((((int)mt->fMtFlowQueue[i].size()) < mt->fMtQueueDepth) || mt->fMtShutdown) {
            mt->fMtFlowQueue[i].push_back(flow);
            mt->fMtFlagQueue[i].push_back(flag);
            return;
         }
         // Unlock when we go out of scope
      }
      
      usleep(mt->fMtQueueFullUSleepTime);
   }
}

//Function to print the length of the flow queue when in multithread mode
//Maybe make root update a graphical window?
void PrintMtQueueLength(TAMultithreadHelper* mt)
{
   printf("Multithread queue lengths:\n");
   for (unsigned i=0; i<mt->fMtFlowQueue.size(); i++) {
      printf("%d:\t%zu\n",i,mt->fMtFlowQueue[i].size());
   }
}

#else
static void MtQueueFlowEvent(TAMultithreadHelper* mt, int i, TAFlags* flag, TAFlowEvent* flow)
{
   fprintf(stderr, "MtQueueFlowEvent() should not have been called when compiled without C++11 thread support\n");
   abort();
}
#endif

//////////////////////////////////////////////////////////
//
// Methods of TARegister
//
//////////////////////////////////////////////////////////

std::vector<TAFactory*> *gModules = NULL;

TARegister::TARegister(TAFactory* m)
{
   if (!gModules)
      gModules = new std::vector<TAFactory*>;
   gModules->push_back(m);
}

#if 0
static double GetTimeSec()
{
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec + 0.000001*tv.tv_usec;
}
#endif

class RunHandler
{
public:
   TARunInfo* fRunInfo;
   std::vector<TARunObject*> fRunRun;
   std::vector<std::string>  fArgs;
   bool fMultithreadMode;

   Profiler* manaprofiler;

   RunHandler(const std::vector<std::string>& args, bool multithread, Profiler* profiler) // ctor
   {
      fRunInfo = NULL;
      fArgs = args;
      fMultithreadMode = multithread;
      manaprofiler=profiler;
   }

   ~RunHandler() // dtor
   {
      if (fRunInfo) {
         delete fRunInfo;
         fRunInfo = NULL;
      }
   }

#ifdef HAVE_CXX11_THREADS
   void PerModuleThread(int i)
   {
      bool data_processing=true;
      int nModules=(*gModules).size();

      TAMultithreadHelper* mt = fRunInfo->fMtInfo;

      assert(nModules == (int)mt->fMtFlowQueueMutex.size());
      assert(nModules == (int)mt->fMtFlowQueue.size());
      assert(nModules == (int)mt->fMtFlagQueue.size());

      while (data_processing) {
         if (mt->fMtShutdown) {
            // shut down the thread
            return;
         }

         TAFlowEvent* flow = NULL;
         TAFlags* flag = NULL;

         { //Lock scope
            std::lock_guard<std::mutex> lock(mt->fMtFlowQueueMutex[i]);
            if (!mt->fMtFlowQueue[i].empty()) {
               flow=mt->fMtFlowQueue[i].front();
               flag=mt->fMtFlagQueue[i].front();
               mt->fMtFlowQueue[i].pop_front();
               mt->fMtFlagQueue[i].pop_front();
            }
            // implicit unlock of mutex
         }

         if (flow == NULL) { // wait until queue not empty
            usleep(mt->fMtQueueEmptyUSleepTime);
            continue;
         }

         //If the flow has the last item, stop this thread
         if (flow == mt->fMtLastItemInQueue) {
            // this is the last event, stop the thread after processing it
            data_processing = false;
         } else {
            START_TIMER
            flow = fRunRun[i]->AnalyzeFlowEvent(fRunInfo, flag, flow);
            manaprofiler->log(flag, flow, i,fRunRun[i]->ModuleName.c_str(),timer_start);
            if ((*flag) & TAFlag_QUIT) { // shut down the analyzer
               data_processing=false;
               delete flow;
               delete flag;
               flow = NULL;
               flag = NULL;
               mt->fMtQuit = true;
               continue;
            }

            if ((*flag) & TAFlag_SKIP) { // stop processing this event
               delete flow;
               delete flag;
               flow = NULL;
               flag = NULL;
               continue;
            }
         }

         if (i==nModules-1) //If I am the last module... free memory, else queue up for next module to process
         {
            if (flow == mt->fMtLastItemInQueue) {
               // we are deleting the end-of-run marker,
               // tell the destructor that it does not need to delete it, too
               mt->fMtLastItemInQueue = NULL;
            }
            manaprofiler->log_user_profiling(flag, flow);
            delete flow;
            delete flag;
            flow = NULL;
            flag = NULL;
         }
         else 
         {
            MtQueueFlowEvent(mt, i+1, flag, flow);
            flow = NULL;
            flag = NULL;
         }
      }
   }
#endif

   void CreateRun(int run_number, const char* file_name)
   {
      assert(fRunInfo == NULL);
      assert(fRunRun.size() == 0);
      
      fRunInfo = new TARunInfo(run_number, file_name, fArgs);

      int nModules = (*gModules).size();

      for (int i=0; i<nModules; i++)
         fRunRun.push_back((*gModules)[i]->NewRunObject(fRunInfo));

#ifdef HAVE_CXX11_THREADS
      if (fMultithreadMode) {
         TAMultithreadHelper* mt = new TAMultithreadHelper();
         fRunInfo->fMtInfo = mt;
         mt->fMtFlowQueue.resize(nModules);
         mt->fMtFlagQueue.resize(nModules);
         std::vector<std::mutex> mutsize(nModules);
         mt->fMtFlowQueueMutex.swap(mutsize);
         mt->fMtThreads.resize(nModules);
         for (int i=0; i<nModules; i++) {
            printf("Create fMtFlowQueue thread %d\n",i);
            mt->fMtThreads[i]=new std::thread(&RunHandler::PerModuleThread,this,i);
         }
      }
#endif
   }

   void BeginRun()
   {
      assert(fRunInfo != NULL);
      assert(fRunInfo->fOdb != NULL);
      for (unsigned i=0; i<fRunRun.size(); i++)
         fRunRun[i]->BeginRun(fRunInfo);
      manaprofiler->begin(fRunInfo,fRunRun);
   }

   void EndRun(TAFlags* flags)
   {
      assert(fRunInfo);

      // make sure the shutdown sequence matches the description in the README file!

      // first, call PreEndRun() to tell analysis modules that there will be no more
      // MIDAS events, no more calls to Analyze(). PreEndRun() may generate more
      // flow events, they to into the flow queue or into the multithread queue
   
      for (unsigned i=0; i<fRunRun.size(); i++)
         fRunRun[i]->PreEndRun(fRunInfo);

      // if in single threaded mode, analyze all queued flow events - call AnalyzeFlowEvent()
      // this can generate additional flow events that will be queued in the queue.

      AnalyzeFlowQueue(flags);

#ifdef HAVE_CXX11_THREADS
      // if in multithreaded mode, queue the special end-of-run marker event
      // then wait for the threads to complete.
      // FIXME: AnalyzeFlowEvent() can generate additional flow events, they will
      // be queued after the special end-of-run marker and will not be analyzed.
      // perhaps instead of the special end-of-run marker, we should wait
      // for the queues to empty naturally, then set fMtShutdown to tell the threads
      // to stop, then to thread join to wait until they actually shutdown. K.O.
      
      if (fRunInfo->fMtInfo) {
         MtQueueFlowEvent(fRunInfo->fMtInfo, 0, NULL, fRunInfo->fMtInfo->fMtLastItemInQueue);
         for (unsigned i=0; i<fRunRun.size(); i++) {
            printf("Waiting for thread %d to finish...\n", i);
            fRunInfo->fMtInfo->fMtThreads[i]->join();
         }

         if (fRunInfo->fMtInfo->fMtQuit) {
            (*flags) |= TAFlag_QUIT;
         }

         fRunInfo->fMtInfo->fMtShutdown = true; // FIXME: this tells the destructor that threads are shutdown
      }
#endif

      // all data analysis is complete
      
      for (unsigned i=0; i<fRunRun.size(); i++)
         fRunRun[i]->EndRun(fRunInfo);
      manaprofiler->end();
   }

   void NextSubrun()
   {
      assert(fRunInfo);
      
      for (unsigned i=0; i<fRunRun.size(); i++)
         fRunRun[i]->NextSubrun(fRunInfo);
   }

   void DeleteRun()
   {
      assert(fRunInfo);

      for (unsigned i=0; i<fRunRun.size(); i++) {
         delete fRunRun[i];
         fRunRun[i] = NULL;
      }

      fRunRun.clear();
      assert(fRunRun.size() == 0);

      delete fRunInfo;
      fRunInfo = NULL;
   }

   void AnalyzeSpecialEvent(TMEvent* event)
   {
      for (unsigned i=0; i<fRunRun.size(); i++)
         fRunRun[i]->AnalyzeSpecialEvent(fRunInfo, event);
   }

   TAFlowEvent* AnalyzeFlowEvent(TAFlags* flags, TAFlowEvent* flow)
   {
      for (unsigned i=0; i<fRunRun.size(); i++) {
         START_TIMER;
         flow = fRunRun[i]->AnalyzeFlowEvent(fRunInfo, flags, flow);
         manaprofiler->log(flags, flow,i,fRunRun[i]->ModuleName.c_str(),timer_start);
         if (!flow)
            break;
         if ((*flags) & TAFlag_SKIP)
               break;
         if ((*flags) & TAFlag_QUIT)
            break;
      }
      return flow;
   }

   void AnalyzeFlowQueue(TAFlags* ana_flags)
   {
      while (1) {
         TAFlowEvent* flow = fRunInfo->ReadFlowQueue();
         if (!flow)
            break;

         int flags = 0;
         flow = AnalyzeFlowEvent(&flags, flow);
         
         if (flow)
            delete flow;
         if (flags & TAFlag_QUIT) {
            *ana_flags |= TAFlag_QUIT;
            break;
         }
      }
   }

   void AnalyzeEvent(TMEvent* event, TAFlags* flags, TMWriterInterface *writer)
   {
      assert(fRunInfo != NULL);
      assert(fRunInfo->fOdb != NULL);
      
      TAFlowEvent* flow = NULL;
                  
      for (unsigned i=0; i<fRunRun.size(); i++) {
         START_TIMER;
         flow = fRunRun[i]->Analyze(fRunInfo, event, flags, flow);
         manaprofiler->log_unpack_time(flags,flow,i,fRunRun[i]->ModuleName.c_str(),timer_start);
         if (*flags & TAFlag_SKIP)
            break;
         if (*flags & TAFlag_QUIT)
            break;
      }

      if (flow) {
         if ((*flags & TAFlag_SKIP)||(*flags & TAFlag_QUIT)) {
            // skip further processing of this event
         } else {
            if (fRunInfo->fMtInfo) {
               MtQueueFlowEvent(fRunInfo->fMtInfo, 0, NULL, flow);
               flow = NULL; // ownership passed to the multithread event queue
            } else {
               flow = AnalyzeFlowEvent(flags, flow);
            }
         }
      }
      manaprofiler->log_user_profiling(flags, flow);
      if (fMultithreadMode)
         manaprofiler->log_mt_queue_length(fRunInfo);

      if (*flags & TAFlag_WRITE)
         if (writer)
            TMWriteEvent(writer, event);
      
      if (flow)
         delete flow;

      if (*flags & TAFlag_QUIT)
         return;

      AnalyzeFlowQueue(flags);
   }
};

TAFlowEvent* TARunInfo::ReadFlowQueue()
{
   if (fFlowQueue.empty())
      return NULL;

   TAFlowEvent* flow = fFlowQueue.front();
   fFlowQueue.pop_front();
   return flow;
}

void TARunInfo::AddToFlowQueue(TAFlowEvent* flow)
{
   if (fMtInfo) {
      MtQueueFlowEvent(fMtInfo, 0, NULL, flow);
   } else {
      fFlowQueue.push_back(flow);
   }
}

#ifdef HAVE_MIDAS

#include "TMidasOnline.h"

#ifdef HAVE_ROOT
#include "TSystem.h"
#endif

class OnlineHandler: public TMHandlerInterface
{
public:
   RunHandler fRun;
   int fNumAnalyze = 0;
   TMWriterInterface* fWriter = NULL;
   bool fQuit = false;
   MVOdb* fOdb = NULL;

   OnlineHandler(int num_analyze, TMWriterInterface* writer, MVOdb* odb, const std::vector<std::string>& args, bool multithread) // ctor
      : fRun(args, multithread)
   {
      fNumAnalyze = num_analyze;
      fWriter = writer;
      fOdb = odb;
   }

   ~OnlineHandler() // dtor
   {
      fWriter = NULL;
      fOdb = NULL;
   }

   void StartRun(int run_number)
   {
      fRun.CreateRun(run_number, NULL);
      fRun.fRunInfo->fOdb = fOdb;
      fRun.BeginRun();
   }

   void Transition(int transition, int run_number, int transition_time)
   {
      //printf("OnlineHandler::Transtion: transition %d, run %d, time %d\n", transition, run_number, transition_time);
      
      if (transition == TR_START) {
         if (fRun.fRunInfo) {
            TAFlags flags = 0;
            fRun.EndRun(&flags);
            if (flags & TAFlag_QUIT)
               fQuit = true;
            fRun.fRunInfo->fOdb = NULL;
            fRun.DeleteRun();
         }
         assert(fRun.fRunInfo == NULL);

         StartRun(run_number);
         printf("Begin run: %d\n", run_number);
      } else if (transition == TR_STOP) {
         TAFlags flags = 0;
         fRun.EndRun(&flags);
         if (flags & TAFlag_QUIT)
            fQuit = true;
         fRun.fRunInfo->fOdb = NULL;
         fRun.DeleteRun();
         printf("End of run %d\n", run_number);
      }
   }

   void Event(const void* data, int data_size)
   {
      //printf("OnlineHandler::Event: ptr %p, size %d\n", data, data_size);

      if (!fRun.fRunInfo) {
         StartRun(0); // start fake run for events outside of a run
      }

      TMEvent* event = new TMEvent(data, data_size);

      TAFlags flags = 0;
      
      fRun.AnalyzeEvent(event, &flags, fWriter);

      if (flags & TAFlag_QUIT)
         fQuit = true;

      if (fNumAnalyze > 0) {
         fNumAnalyze--;
         if (fNumAnalyze == 0)
            fQuit = true;
      }

      if (event) {
         delete event;
         event = NULL;
      }
   }
};

static int ProcessMidasOnline(const std::vector<std::string>& args, const char* hostname, const char* exptname, int num_analyze, TMWriterInterface* writer, bool multithread,Profiler* profile)
{
   TMidasOnline *midas = TMidasOnline::instance();

   int err = midas->connect(hostname, exptname, "rootana");
   if (err != 0) {
      fprintf(stderr,"Cannot connect to MIDAS, error %d\n", err);
      return -1;
   }

   MVOdb* odb = MakeMidasOdb(midas->fDB);
   
   OnlineHandler* h = new OnlineHandler(num_analyze, writer, odb, args, multithread, profile);

   midas->RegisterHandler(h);
   midas->registerTransitions();

   /* reqister event requests */

   midas->eventRequest("SYSTEM",-1,-1,(1<<1));

   int run_number = 0; // midas->odbReadInt("/runinfo/Run number");
   int run_state  = 0; // midas->odbReadInt("/runinfo/State");

   odb->RI("runinfo/run number", &run_number);
   odb->RI("runinfo/state", &run_state);

   for (unsigned i=0; i<(*gModules).size(); i++)
      (*gModules)[i]->Init(args);

   if ((run_state == STATE_RUNNING)||(run_state == STATE_PAUSED)) {
      h->StartRun(run_number);
   }

   while (!h->fQuit) {
#ifdef HAVE_THTTP_SERVER
      if (TARootHelper::fgHttpServer) {
         TARootHelper::fgHttpServer->ProcessRequests();
      }
#endif
#ifdef HAVE_ROOT
      if (TARootHelper::fgApp) {
         gSystem->DispatchOneEvent(kTRUE);
      }
#endif
      if (!TMidasOnline::instance()->poll(10))
         break;
   }

   if (h->fRun.fRunInfo) {
      TAFlags flags = 0;
      h->fRun.EndRun(&flags);
      h->fRun.fRunInfo->fOdb = NULL;
      h->fRun.DeleteRun();
   }

   for (unsigned i=0; i<(*gModules).size(); i++)
      (*gModules)[i]->Finish();

   delete h; h = NULL;
   delete odb; odb = NULL;
   
   /* disconnect from experiment */
   midas->disconnect();

   return 0;
}

#endif

std::vector<std::string> TARunInfo::fgFileList;
int TARunInfo::fgCurrentFileIndex = 0;

static int ProcessMidasFiles(const std::vector<std::string>& files, const std::vector<std::string>& args, int num_skip, int num_analyze, TMWriterInterface* writer, bool multithread, Profiler* profile)
{
   TARunInfo::fgFileList.clear();

   for (unsigned i=0; i<files.size(); i++)
      TARunInfo::fgFileList.push_back(files[i]);
   
   for (unsigned i=0; i<(*gModules).size(); i++)
      (*gModules)[i]->Init(args);

   RunHandler run(args, multithread, profile);

   bool done = false;

   for (TARunInfo::fgCurrentFileIndex = 0;
        TARunInfo::fgCurrentFileIndex < (int)TARunInfo::fgFileList.size();
        TARunInfo::fgCurrentFileIndex++) {
      std::string filename = TARunInfo::fgFileList[TARunInfo::fgCurrentFileIndex];

      TMReaderInterface *reader = TMNewReader(filename.c_str());

      if (reader->fError) {
         printf("Could not open \"%s\", error: %s\n", filename.c_str(), reader->fErrorString.c_str());
         delete reader;
         continue;
      }

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

               if (run.fRunInfo) {
                  if (run.fRunInfo->fRunNo == runno) {
                     // next subrun file, nothing to do
                     run.fRunInfo->fFileName = filename;
                     run.NextSubrun();
                  } else {
                     // file with a different run number
                     TAFlags flags = 0;
                     run.EndRun(&flags);
                     if (flags & TAFlag_QUIT) {
                        done = true;
                     }
                     run.DeleteRun();
                  }
               }

               if (!run.fRunInfo) {
                  run.CreateRun(runno, filename.c_str());
                  run.fRunInfo->fOdb = MakeFileDumpOdb(event->GetEventData(), event->data_size);
                  run.BeginRun();
               }

               assert(run.fRunInfo);

               run.AnalyzeSpecialEvent(event);

               if (writer)
                  TMWriteEvent(writer, event);
            }
         else if (event->event_id == 0x8001) // end of run event
            {
               //int runno = event->serial_number;
               run.AnalyzeSpecialEvent(event);
               if (writer)
                  TMWriteEvent(writer, event);

               if (run.fRunInfo->fOdb) {
                  delete run.fRunInfo->fOdb;
                  run.fRunInfo->fOdb = NULL;
               }
               
               run.fRunInfo->fOdb = MakeFileDumpOdb(event->GetEventData(), event->data_size);
            }
         else if (event->event_id == 0x8002) // message event
            {
               run.AnalyzeSpecialEvent(event);
               if (writer)
                  TMWriteEvent(writer, event);
            }
         else
            {
               if (!run.fRunInfo) {
                  // create a fake begin of run
                  run.CreateRun(0, filename.c_str());
                  run.fRunInfo->fOdb = MakeNullOdb();
                  run.BeginRun();
               }

               if (num_skip > 0) {
                  num_skip--;
               } else {
                  TAFlags flags = 0;

                  run.AnalyzeEvent(event, &flags, writer);

                  if (flags & TAFlag_QUIT)
                     done = true;

                  if (num_analyze > 0) {
                     num_analyze--;
                     if (num_analyze == 0)
                        done = true;
                  }
               }
            }

         delete event;

         if (done)
            break;

#ifdef HAVE_ROOT
         if (TARootHelper::fgApp) {
            gSystem->DispatchOneEvent(kTRUE);
         }
#endif
      }

      reader->Close();
      delete reader;

      if (done)
         break;
   }

   if (run.fRunInfo) {
      TAFlags flags = 0;
      run.EndRun(&flags);
      if (flags & TAFlag_QUIT)
         done = true;
      run.DeleteRun();
   }
   
   for (unsigned i=0; i<(*gModules).size(); i++)
      (*gModules)[i]->Finish();
   
   return 0;
}

static int ProcessDemoMode(const std::vector<std::string>& args, int num_skip, int num_analyze, TMWriterInterface* writer, bool multithread, Profiler* profile)
{
   for (unsigned i=0; i<(*gModules).size(); i++)
      (*gModules)[i]->Init(args);

   RunHandler run(args, multithread, profile);

   bool done = false;

   int runno = 1;
   
   for (unsigned i=0; true; i++) {
      char s[256];
      sprintf(s, "%03d", i);
      std::string filename = std::string("demo_subrun_") + s;

      if (!run.fRunInfo) {
         run.CreateRun(runno, filename.c_str());
         run.fRunInfo->fOdb = MakeNullOdb();
         run.BeginRun();
      }

      // we do not generate a fake begin of run event...
      //run.AnalyzeSpecialEvent(event);

      // only switch subruns after the first subrun file
      if (i>0) {
         run.fRunInfo->fFileName = filename;
         run.NextSubrun();
      }

      for (unsigned j=0; j<100; j++) {
         TMEvent* event = new TMEvent();

         if (num_skip > 0) {
            num_skip--;
         } else {
            TAFlags flags = 0;
            
            run.AnalyzeEvent(event, &flags, writer);
            
            if (flags & TAFlag_QUIT)
               done = true;
            
            if (num_analyze > 0) {
               num_analyze--;
               if (num_analyze == 0)
                  done = true;
            }
         }

         delete event;

         if (done)
            break;

#ifdef HAVE_ROOT
         if (TARootHelper::fgApp) {
            gSystem->DispatchOneEvent(kTRUE);
         }
#endif
      }

      // we do not generate a fake end of run event...
      //run.AnalyzeSpecialEvent(event);

      if (done)
         break;
   }

   if (run.fRunInfo) {
      TAFlags flags = 0;
      run.EndRun(&flags);
      run.DeleteRun();
   }
   
   for (unsigned i=0; i<(*gModules).size(); i++)
      (*gModules)[i]->Finish();
   
   return 0;
}

static bool gEnableShowMem = false;

#if 0
static int ShowMem(const char* label)
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
#endif

class EventDumpModule: public TARunObject
{
public:
   EventDumpModule(TARunInfo* runinfo)
      : TARunObject(runinfo)
   {
      if (gTrace)
         printf("EventDumpModule::ctor, run %d\n", runinfo->fRunNo);
   }
   
   ~EventDumpModule()
   {
      if (gTrace)
         printf("EventDumpModule::dtor!\n");
   }

   void BeginRun(TARunInfo* runinfo)
   {
      printf("EventDumpModule::BeginRun, run %d\n", runinfo->fRunNo);
   }

   void EndRun(TARunInfo* runinfo)
   {
      printf("EventDumpModule::EndRun, run %d\n", runinfo->fRunNo);
   }

   void NextSubrun(TARunInfo* runinfo)
   {
      printf("EventDumpModule::NextSubrun, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
   }

   void PauseRun(TARunInfo* runinfo)
   {
      printf("EventDumpModule::PauseRun, run %d\n", runinfo->fRunNo);
   }

   void ResumeRun(TARunInfo* runinfo)
   {
      printf("EventDumpModule::ResumeRun, run %d\n", runinfo->fRunNo);
   }

   TAFlowEvent* Analyze(TARunInfo* runinfo, TMEvent* event, TAFlags* flags, TAFlowEvent* flow)
   {
      printf("EventDumpModule::Analyze, run %d, ", runinfo->fRunNo);
      event->FindAllBanks();
      std::string h = event->HeaderToString();
      std::string b = event->BankListToString();
      printf("%s: %s\n", h.c_str(), b.c_str());
      return flow;
   }

   void AnalyzeSpecialEvent(TARunInfo* runinfo, TMEvent* event)
   {
      printf("EventDumpModule::AnalyzeSpecialEvent, run %d, event serno %d, id 0x%04x, data size %d\n", runinfo->fRunNo, event->serial_number, (int)event->event_id, event->data_size);
   }
};

class EventDumpModuleFactory: public TAFactory
{
public:

   void Init(const std::vector<std::string> &args)
   {
      if (gTrace)
         printf("EventDumpModuleFactory::Init!\n");
   }
   
   void Finish()
   {
      if (gTrace)
         printf("EventDumpModuleFactory::Finish!\n");
   }
   
   TARunObject* NewRunObject(TARunInfo* runinfo)
   {
      if (gTrace)
         printf("EventDumpModuleFactory::NewRunObject, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
      return new EventDumpModule(runinfo);
   }
};

#ifdef HAVE_ROOT
#include <TGMenu.h>
#include <TGButton.h>
#include <TBrowser.h>

#define CTRL_QUIT 1
#define CTRL_NEXT 2
#define CTRL_CONTINUE 3
#define CTRL_PAUSE    4
#define CTRL_NEXT_FLOW 5

#define CTRL_TBROWSER 11

class ValueHolder
{
public:
   int fValue;

   ValueHolder() // ctor
   {
      fValue = 0;
   }
};

class TextButton: public TGTextButton
{
public:
   ValueHolder* fHolder;
   int    fValue;
   
   TextButton(TGWindow*p, const char* text, ValueHolder* holder, int value) // ctor
      : TGTextButton(p, text)
   {
      fHolder = holder;
      fValue = value;
   }

#if 0
   void Pressed()
   {
      printf("Pressed!\n");
   }
   
   void Released()
   {
      printf("Released!\n");
   }
#endif
   
   void Clicked()
   {
      //printf("Clicked button %s, value %d!\n", GetString().Data(), fValue);
      if (fHolder)
         fHolder->fValue = fValue;
      //gSystem->ExitLoop();
   }
};

class MainWindow: public TGMainFrame
{
public:
   TGPopupMenu*		fMenu;
   TGMenuBar*		fMenuBar;
   TGLayoutHints*	fMenuBarItemLayout;

   TGCompositeFrame*    fButtonsFrame;

   ValueHolder* fHolder;

   TextButton* fNextButton;
   TextButton* fNextFlowButton;
   TextButton* fContinueButton;
   TextButton* fPauseButton;

   TextButton* fQuitButton;
  
public:
   MainWindow(const TGWindow*w, int s1, int s2, ValueHolder* holder) // ctor
      : TGMainFrame(w, s1, s2)
   {
      if (gTrace)
         printf("MainWindow::ctor!\n");

      fHolder = holder;
      //SetCleanup(kDeepCleanup);
   
      SetWindowName("ROOT Analyzer Control");

      // layout the gui
      fMenu = new TGPopupMenu(gClient->GetRoot());
      fMenu->AddEntry("New TBrowser", CTRL_TBROWSER);
      fMenu->AddEntry("-", 0);
      fMenu->AddEntry("Next",     CTRL_NEXT);
      fMenu->AddEntry("NextFlow", CTRL_NEXT_FLOW);
      fMenu->AddEntry("Continue", CTRL_CONTINUE);
      fMenu->AddEntry("Pause",    CTRL_PAUSE);
      fMenu->AddEntry("-", 0);
      fMenu->AddEntry("Quit",     CTRL_QUIT);

      fMenuBarItemLayout = new TGLayoutHints(kLHintsTop|kLHintsLeft, 0, 4, 0, 0);

      fMenu->Associate(this);

      fMenuBar = new TGMenuBar(this, 1, 1, kRaisedFrame);
      fMenuBar->AddPopup("&Rootana", fMenu, fMenuBarItemLayout);
      fMenuBar->Layout();

      AddFrame(fMenuBar, new TGLayoutHints(kLHintsTop|kLHintsLeft|kLHintsExpandX));

      fButtonsFrame = new TGVerticalFrame(this);

      fNextButton = new TextButton(fButtonsFrame, "Next", holder, CTRL_NEXT);
      fNextFlowButton = new TextButton(fButtonsFrame, "Next Flow Event", holder, CTRL_NEXT_FLOW);

      fButtonsFrame->AddFrame(fNextButton, new TGLayoutHints(kLHintsExpandX, 1, 1, 1, 1));
      fButtonsFrame->AddFrame(fNextFlowButton, new TGLayoutHints(kLHintsExpandX, 1, 1, 1, 1));

      TGHorizontalFrame *hframe = new TGHorizontalFrame(fButtonsFrame);

      fContinueButton = new TextButton(hframe, " Continue ", holder, CTRL_CONTINUE);
      fPauseButton = new TextButton(hframe, " Pause ", holder, CTRL_PAUSE);

      hframe->AddFrame(fContinueButton, new TGLayoutHints(kLHintsExpandX, 1, 1, 1, 1));
      hframe->AddFrame(fPauseButton, new TGLayoutHints(kLHintsExpandX, 1, 1, 1, 1));

      fButtonsFrame->AddFrame(hframe, new TGLayoutHints(kLHintsExpandX));

      fQuitButton = new TextButton(fButtonsFrame, "Quit ", holder, CTRL_QUIT);
      fButtonsFrame->AddFrame(fQuitButton, new TGLayoutHints(kLHintsExpandX, 1, 1, 1, 1));
   
      AddFrame(fButtonsFrame, new TGLayoutHints(kLHintsExpandX));

      MapSubwindows(); 
      Layout();
      Resize(GetDefaultSize());
      MapWindow();
   }

   ~MainWindow() // dtor // Closing the control window closes the whole program
   {
      if (gTrace)
         printf("MainWindow::dtor!\n");

      delete fMenu;
      delete fMenuBar;
      delete fMenuBarItemLayout;
   }

   void CloseWindow()
   {
      if (gTrace)
         printf("MainWindow::CloseWindow()\n");

      if (fHolder)
         fHolder->fValue = CTRL_QUIT;
      //gSystem->ExitLoop();
   }
  
   Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t parm2)
   {
      //printf("GUI Message %d %d %d\n",(int)msg,(int)parm1,(int)parm2);
      switch (GET_MSG(msg))
         {
         default:
            break;
         case kC_COMMAND:
            switch (GET_SUBMSG(msg))
               {
               default:
                  break;
               case kCM_MENU:
                  //printf("parm1 %d\n", (int)parm1);
                  switch (parm1)
                     {
                     case CTRL_TBROWSER:
                        new TBrowser();
                        break;
                     default:
                        //printf("Control %d!\n", (int)parm1);
                        if (fHolder)
                           fHolder->fValue = parm1;
                        //gSystem->ExitLoop();
                        break;
                     }
                  break;
               }
            break;
         }

      return kTRUE;
   }
};
#endif

class InteractiveModule: public TARunObject
{
public:
   bool fContinue;
   bool fNextFlow;
   int  fSkip;
#ifdef HAVE_ROOT
   static ValueHolder* fgHolder;
   static MainWindow *fgCtrlWindow;
#endif
   
   InteractiveModule(TARunInfo* runinfo)
      : TARunObject(runinfo)
   {
      if (gTrace)
         printf("InteractiveModule::ctor, run %d\n", runinfo->fRunNo);
      fContinue = false;
      fNextFlow = false;
      fSkip = 0;
#ifdef HAVE_ROOT
      if (!fgHolder)
         fgHolder = new ValueHolder;
      if (!fgCtrlWindow && runinfo->fRoot->fgApp) {
         fgCtrlWindow = new MainWindow(gClient->GetRoot(), 200, 300, fgHolder);
      }
#endif
   }
   
   ~InteractiveModule()
   {
      if (gTrace)
         printf("InteractiveModule::dtor!\n");
   }

   void BeginRun(TARunInfo* runinfo)
   {
      printf("InteractiveModule::BeginRun, run %d\n", runinfo->fRunNo);
   }

   void EndRun(TARunInfo* runinfo)
   {
      printf("InteractiveModule::EndRun, run %d\n", runinfo->fRunNo);

#ifdef HAVE_ROOT
      if (fgCtrlWindow && runinfo->fRoot->fgApp) {
         fgCtrlWindow->fNextButton->SetEnabled(false);
         fgCtrlWindow->fNextFlowButton->SetEnabled(false);
         fgCtrlWindow->fContinueButton->SetEnabled(false);
         fgCtrlWindow->fPauseButton->SetEnabled(false);
         while (1) {
#ifdef HAVE_THTTP_SERVER
            if (TARootHelper::fgHttpServer) {
               TARootHelper::fgHttpServer->ProcessRequests();
            }
#endif
#ifdef HAVE_ROOT
            if (TARootHelper::fgApp) {
               gSystem->DispatchOneEvent(kTRUE);
            }
#endif
#ifdef HAVE_MIDAS
            if (!TMidasOnline::instance()->sleep(10)) {
               // FIXME: indicate that we should exit the analyzer
               return;
            }
#else
            gSystem->Sleep(10);
#endif

            int ctrl = fgHolder->fValue;
            fgHolder->fValue = 0;

            switch (ctrl) {
            case CTRL_QUIT:
               return;
            case CTRL_NEXT:
               return;
            case CTRL_CONTINUE:
               return;
            }
         }
      }
#endif
   }

   void PauseRun(TARunInfo* runinfo)
   {
      printf("InteractiveModule::PauseRun, run %d\n", runinfo->fRunNo);
   }

   void ResumeRun(TARunInfo* runinfo)
   {
      printf("InteractiveModule::ResumeRun, run %d\n", runinfo->fRunNo);
   }

   void InteractiveLoop(TARunInfo* runinfo, TAFlags* flags)
   {
#ifdef HAVE_ROOT
      if (fgCtrlWindow && runinfo->fRoot->fgApp) {
         while (1) {
#ifdef HAVE_THTTP_SERVER
            if (TARootHelper::fgHttpServer) {
               TARootHelper::fgHttpServer->ProcessRequests();
            }
#endif
#ifdef HAVE_ROOT
            if (TARootHelper::fgApp) {
               gSystem->DispatchOneEvent(kTRUE);
            }
#endif
#ifdef HAVE_MIDAS
            if (!TMidasOnline::instance()->sleep(10)) {
               *flags |= TAFlag_QUIT;
               return;
            }
#else
            gSystem->Sleep(10);
#endif

            int ctrl = fgHolder->fValue;
            fgHolder->fValue = 0;

            switch (ctrl) {
            case CTRL_QUIT:
               *flags |= TAFlag_QUIT;
               return;
            case CTRL_NEXT:
               return;
            case CTRL_NEXT_FLOW:
               fNextFlow = true;
               return;
            case CTRL_CONTINUE:
               fContinue = true;
               return;
            }
         }
      }
#endif

      while (1) {
         char str[256];
         fprintf(stdout, "manalyzer> "); fflush(stdout);
         fgets(str, sizeof(str)-1, stdin);
         
         printf("command [%s]\n", str);

         if (str[0] == 'h') { // "help"
            printf("Interactive manalyzer commands:\n");
            printf(" q - quit\n");
            printf(" h - help\n");
            printf(" c - continue until next TAFlag_DISPLAY event\n");
            printf(" n - next event\n");
            printf(" aNNN - analyze N events, i.e. \"a10\"\n");
         } else if (str[0] == 'q') { // "quit"
            *flags |= TAFlag_QUIT;
            return;
         } else if (str[0] == 'n') { // "next"
            return;
         } else if (str[0] == 'c') { // "continue"
            fContinue = true;
            return;
         } else if (str[0] == 'a') { // "analyze" N events
            int num = atoi(str+1);
            printf("Analyzing %d events\n", num);
            if (num > 0) {
               fSkip = num-1;
            }
            return;
         }
      }
   }

   TAFlowEvent* Analyze(TARunInfo* runinfo, TMEvent* event, TAFlags* flags, TAFlowEvent* flow)
   {
      printf("InteractiveModule::Analyze, run %d, %s\n", runinfo->fRunNo, event->HeaderToString().c_str());

#ifdef HAVE_ROOT
      if (fgHolder->fValue == CTRL_QUIT) {
         *flags |= TAFlag_QUIT;
         return flow;
      } else if (fgHolder->fValue == CTRL_PAUSE) {
         fContinue = false;
      }
#endif

      if ((fContinue||fNextFlow) && !(*flags & TAFlag_DISPLAY)) {
         return flow;
      } else {
         fContinue = false;
      }

      if (fSkip > 0) {
         fSkip--;
         return flow;
      }

      InteractiveLoop(runinfo, flags);

      return flow;
   }

   TAFlowEvent* AnalyzeFlowEvent(TARunInfo* runinfo, TAFlags* flags, TAFlowEvent* flow)
   {
      printf("InteractiveModule::AnalyzeFlowEvent, run %d\n", runinfo->fRunNo);

#ifdef HAVE_ROOT
      if (fgHolder->fValue == CTRL_QUIT) {
         *flags |= TAFlag_QUIT;
         return flow;
      } else if (fgHolder->fValue == CTRL_PAUSE) {
         fContinue = false;
      }
#endif

      if ((!fNextFlow) && !(*flags & TAFlag_DISPLAY)) {
         return flow;
      }

      fNextFlow = false;

      InteractiveLoop(runinfo, flags);
      
      return flow;
   }
   
   void AnalyzeSpecialEvent(TARunInfo* runinfo, TMEvent* event)
   {
      if (gTrace)
         printf("InteractiveModule::AnalyzeSpecialEvent, run %d, event serno %d, id 0x%04x, data size %d\n", runinfo->fRunNo, event->serial_number, (int)event->event_id, event->data_size);
   }
};

#ifdef HAVE_ROOT
MainWindow* InteractiveModule::fgCtrlWindow = NULL;
ValueHolder* InteractiveModule::fgHolder = NULL;
#endif

class InteractiveModuleFactory: public TAFactory
{
public:

   void Init(const std::vector<std::string> &args)
   {
      if (gTrace)
         printf("InteractiveModuleFactory::Init!\n");
   }
   
   void Finish()
   {
      if (gTrace)
         printf("InteractiveModuleFactory::Finish!\n");
   }
   
   TARunObject* NewRunObject(TARunInfo* runinfo)
   {
      if (gTrace)
         printf("InteractiveModuleFactory::NewRunObject, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
      return new InteractiveModule(runinfo);
   }
};
#if __cplusplus >= 201103L 
//C++11 or above is needed for chrono... therefor the profiler needs c++11 for any functionality

Profiler::Profiler()
{
   if (gTrace)
      printf("Profiler::ctor\n");
   //By default the profiler is on
   TimeModules=true;
   midas_start_time=0;
   midas_stop_time=0;
   tStart_cpu = clock();
   tStart_user =  std::chrono::system_clock::now();
}

Profiler::~Profiler()
{
   if (gTrace)
      printf("Profiler::dtor\n");
}

void Profiler::begin(TARunInfo* runinfo,const std::vector<TARunObject*> fRunRun )
{
   if (gTrace)
      printf("Profiler::begin\n");

#ifdef INCLUDE_VirtualOdb_H
   midas_start_time = runinfo->fOdb->odbReadUint32("/Runinfo/Start time binary", 0, 0);
#endif
#ifdef INCLUDE_MVODB_H
   runinfo->fOdb->RU32("/Runinfo/Start time binary",(uint32_t*) &midas_start_time);
#endif

#ifdef HAVE_ROOT
   runinfo->fRoot->fOutputFile->cd(); // select correct ROOT directory
   gDirectory->mkdir("ProfilerReport")->cd();

   //Setup module histograms
   Int_t Nbins=100;
   Double_t bins[Nbins+1];
   Double_t TimeRange=10; //seconds
   //Set uneven binning to better sample fast modules with accuracy
   for (int i=0; i<Nbins+1; i++)
   {
      bins[i]=TimeRange*pow(1.1,i)/pow(1.1,Nbins);
   }
#endif

   const int nmodules=fRunRun.size();
   for (int i=0;i<nmodules; i++)
   {
      MaxUnpackTime.push_back(0);
      TotalUnpackTime.push_back(0);
#ifdef HAVE_ROOT
      TString ModuleName;
      if (!fRunRun.at(i)->ModuleName.size())
         ModuleName="Unnamed Module " + std::to_string(i);
      else
         ModuleName=fRunRun.at(i)->ModuleName;
      TH1D* UnpackHisto=new TH1D(ModuleName+"_TMEvent",ModuleName,Nbins,bins);
      UnpackTimeHistograms.push_back(UnpackHisto);
#else
      if (!fRunRun.at(i)->ModuleName.size())
         ModuleNames.push_back("Unnamed Module " + std::to_string(i));
      else
         ModuleNames.push_back(fRunRun.at(i)->ModuleName);
      //We dont have root, so can't use thier histograms
      unpack_mean.push_back(0);
      unpack_rms.push_back(0);
      unpack_entries.push_back(0);
#endif
      MaxModuleTime.push_back(0);
      TotalModuleTime.push_back(0);
#ifdef HAVE_ROOT
      TH1D* Histo=new TH1D(ModuleName,ModuleName,Nbins,bins);
      ModuleTimeHistograms.push_back(Histo);
#else
      module_mean.push_back(0);
      module_rms.push_back(0);
      module_entries.push_back(0);
#endif
   }
}

void Profiler::log(TAFlags* flag, TAFlowEvent* flow,int i,const char* module_name,CLOCK_TYPE start)
{
   if (gTrace)
      printf("Profiler::log\n");
   if (!TimeModules) return;
   CLOCK_TYPE stop=CLOCK_NOW;
   if ((*flag) & TAFlag_SKIP_PROFILE)
   {
      //Unset bit
      *flag -= TAFlag_SKIP_PROFILE;
      return;
   }
   std::chrono::duration<double> elapsed_seconds = stop - start;
   double dt=elapsed_seconds.count();
   TotalModuleTime[i]+=dt;
   if (dt>MaxModuleTime[i])
      MaxModuleTime[i]=dt;
#ifdef HAVE_ROOT
   ModuleTimeHistograms.at(i)->Fill(dt);
#else
   module_mean[i]  +=dt;
   module_rms[i]   +=dt*dt;
   module_entries[i]++;
#endif
}

void Profiler::log_unpack_time(TAFlags* flag, TAFlowEvent* flow,int i,const char* module_name,CLOCK_TYPE start)
{
   if (gTrace)
      printf("Profiler::log_unpack_time\n");
   if (!TimeModules)
      return;
   CLOCK_TYPE stop=CLOCK_NOW;
   if ((*flag) & TAFlag_SKIP_PROFILE)
   {
      //Unset bit
      *flag -= TAFlag_SKIP_PROFILE;
      return;
   }
   std::chrono::duration<double> elapsed_seconds = stop - start;
   double dt=elapsed_seconds.count();
   TotalUnpackTime[i]+=dt;
   if (dt>MaxUnpackTime[i])
      MaxUnpackTime[i]=dt;
#ifdef HAVE_ROOT
   UnpackTimeHistograms.at(i)->Fill(dt);
#else
   //We dont have root, so can't use thier histograms
   unpack_mean[i]   +=dt;
   unpack_rms[i]    +=dt*dt;
   unpack_entries[i]++;
#endif
}

void Profiler::log_mt_queue_length(TARunInfo* runinfo)
{
   if (gTrace)
      printf("Profiler::log_mt_queue_length\n");
#ifdef HAVE_CXX11_THREADS
#ifdef HAVE_ROOT
   QueueIntervalCounter++;
   if (runinfo->fMtInfo && (QueueIntervalCounter%QueueInterval==0))
   {
      for (int i=0; i<NQueues; i++)
      {
         int j=0;
         {
            std::lock_guard<std::mutex> lock(runinfo->fMtInfo->fMtFlowQueueMutex[i]);
            j=runinfo->fMtInfo->fMtFlowQueue[i].size();
         }
         AnalysisQueue.at(i)->Fill(j);
      }
   }
#endif
#endif
}


void Profiler::log_user_profiling(TAFlags* flag, TAFlowEvent* flow)
{
   if (gTrace)
      printf("Profiler::log_user_profiling\n");
#ifdef HAVE_ROOT
   //Clocks unfold backwards... 
   std::vector<TAFlowEvent*> flowArray;
   int FlowEvents=0;
   TAFlowEvent* f = flow;
   while (f) 
   {
      flowArray.push_back(f);
      f=f->fNext;
      FlowEvents++;
   }
   for (int ii=FlowEvents-1; ii>=0; ii--)
   {
      f=flowArray[ii];
      UserProfilerFlow* timer=dynamic_cast<UserProfilerFlow*>(f);
      if (timer)
      {
         const char* name=timer->ModuleName.c_str();
         unsigned int hash=std::hash<std::string>{}(timer->ModuleName);
         if (!UserMap.count(hash))
            AddModuleMap(name,hash);
         double dt=999.;
         dt=timer->GetTimer();
         int i=UserMap[hash];
         TotalModuleTime[i]+=dt;
         if (dt>MaxModuleTime[i])
            MaxModuleTime.at(i)=dt;
         UserHistograms.at(i)->Fill(dt);
      }
   }
#else
   printf("root required for user profiling tools\n");
#endif
}
void Profiler::AddModuleMap( const char* UserProfileName, unsigned long hash)
{
   if (gTrace)
     printf("Profiler::AddModuleMap\n");
#ifdef HAVE_CXX11_THREADS
   std::lock_guard<std::mutex> lock(TAMultithreadHelper::gfLock);
#endif
#ifdef HAVE_ROOT
   gDirectory->cd("/ProfilerReport");
   UserMap[hash]= UserHistograms.size();
   Int_t Nbins=100;
   Double_t bins[Nbins+1];
   Double_t TimeRange=10; //seconds
   for (int i=0; i<Nbins+1; i++)
   {
      bins[i]=TimeRange*pow(1.1,i)/pow(1.1,Nbins);
      //std::cout <<"BIN:"<<bins[i]<<std::endl;
   }
   TH1D* Histo=new TH1D(UserProfileName,UserProfileName,Nbins,bins);
   UserHistograms.push_back(Histo);
   TotalUserTime.push_back(0.);
   MaxUserTime.push_back(0.);
#endif
   return;
}

void Profiler::end()
{
  if (gTrace)
     printf("Profiler::end\n");
#ifdef HAVE_ROOT
   if (ModuleTimeHistograms.size()>0)
#else
   for (size_t i=0; i<unpack_mean.size(); i++)
   {
      unpack_mean.at(i)=unpack_mean.at(i)/unpack_entries.at(i);
      unpack_rms.at(i) = unpack_rms.at(i) / unpack_entries.at(i) - 
         ( unpack_mean.at(i) / unpack_entries.at(i) ) * ( unpack_mean.at(i) / unpack_entries.at(i) );
   }
   for (size_t i=0; i<module_mean.size(); i++)
   {
      module_mean.at(i)=module_mean.at(i)/module_entries.at(i);
      module_rms.at(i) = module_rms.at(i) / module_entries.at(i) - 
         ( module_mean.at(i) / module_entries.at(i) ) * ( module_mean.at(i) / module_entries.at(i) );
   }
   if (unpack_entries.size()>0)
#endif
   {
      double AllModuleTime=0;
      for (auto& n : TotalModuleTime)
         AllModuleTime += n;
      double AllUnpackTime=0;
      for (auto& n : TotalUnpackTime)
         AllUnpackTime += n;
      //double max_unpack_time=*std::max_element(TotalUnpackTime.begin(),TotalUnpackTime.end());
      printf("Module average processing time\n");
      printf("      \t\t\t\tUnpack (one thread)                \tFlow (multithreadable)\n");
      printf("Module\t\t\t\tEntries\tMean(ms)RMS(ms)\tMax(ms)\tSum(s)\tEntries\tMean(ms)RMS(ms)\tMax(ms)\tSum(s)\n");
      //double max_total_time=*std::max_element(TotalModuleTime.begin(),TotalModuleTime.end());
      printf("----------------------------------------------------------------------------------------------------------------\n");
#ifdef HAVE_ROOT
      for (size_t i=0; i<ModuleTimeHistograms.size(); i++)
      {
         printf("%-25s", ModuleTimeHistograms.at(i)->GetTitle());
         if ((int)UnpackTimeHistograms.at(i)->GetEntries())
            printf("\t%d\t%.1f\t%.1f\t%.1f\t%.3f",
               (int)UnpackTimeHistograms.at(i)->GetEntries(),
               UnpackTimeHistograms.at(i)->GetMean()*1000., //ms
               UnpackTimeHistograms.at(i)->GetRMS()*1000., //ms
#else
      for (size_t i=0; i<ModuleNames.size(); i++)
      {
         printf("%-25s", ModuleNames.at(i).c_str());
         if (unpack_entries.at(i))
            printf("\t%d\t%.1f\t%.1f\t%.1f\t%.3f",
               unpack_entries.at(i),
               unpack_mean.at(i)*1000.,
               unpack_rms.at(i)*1000.,
#endif
               MaxUnpackTime.at(i)*1000., //ms
               TotalUnpackTime.at(i)); //s
         else
            printf("\t-\t-\t-\t-\t-");

#ifdef HAVE_ROOT         
         if ((int)ModuleTimeHistograms.at(i)->GetEntries())
            printf("\t%d\t%.1f\t%.1f\t%.1f\t%.3f",
               (int)ModuleTimeHistograms.at(i)->GetEntries(),
               ModuleTimeHistograms.at(i)->GetMean()*1000., //ms
               ModuleTimeHistograms.at(i)->GetRMS()*1000., //ms
#else
         if (module_entries.at(i))
            printf("\t%d\t%.1f\t%.1f\t%.1f\t%.3f",
               module_entries.at(i),
               module_mean.at(i)*1000.,
               module_rms.at(i)*1000.,
#endif
               MaxModuleTime.at(i)*1000., //ms
               TotalModuleTime.at(i)); //s
         else
            printf("\t-\t-\t-\t-\t-");
         //printf("\t%.1f%%\t",100.*TotalModuleTime.at(i)/AllModuleTime);
         //printf("\t%.0f%%",100.*TotalModuleTime.at(i)/max_total_time);
         printf("\n");
         
      }
      printf("----------------------------------------------------------------------------------------------------------------\n");
      printf("                                   Analyse TMEvent total time   %f\n",AllUnpackTime);
      printf("                                                                           Analyse FlowEvent total time %f\n",AllModuleTime);
#ifdef HAVE_ROOT
      if (UserHistograms.size())
      {
         printf("Custom profiling windows\tEntries\tMean(ms)RMS(ms)\tMax(ms)\tSum(s)\n");
         printf("----------------------------------------------------------------------\n");
         for (size_t i=0; i<UserHistograms.size(); i++)
         {
            //std::cout<<ModuleHistograms.at(i)->GetTitle()<<"\t\t";
            printf("%-25s\t%d\t%.1f\t%.1f\t%.1f\t%.3f\t\n",UserHistograms.at(i)->GetTitle(),
            (int)UserHistograms.at(i)->GetEntries(),
            UserHistograms.at(i)->GetMean()*1000., //ms
            UserHistograms.at(i)->GetRMS()*1000., //ms
            MaxUserTime.at(i)*1000., //ms
            TotalUserTime.at(i)); //s
         }
         printf("----------------------------------------------------------------------\n");
      }
      else
      {
         printf("----------------------------------------------------------------------------------------------------------------\n");
      }
#else
      printf("To use custom profile windows, please build rootana with root\n");
#endif
   }
   //CPU and Wall clock time:
   
   double cputime = (double)(clock() - tStart_cpu)/CLOCKS_PER_SEC;
   //double usertime = difftime(time(NULL),tStart_user);
   std::chrono::duration<double> usertime = std::chrono::system_clock::now() - tStart_user;
   printf("%s\tCPU time: %.2fs\tUser time: %.2fs\tAverage CPU Usage: ~%.1f%%\n",
      getenv("_"),
      cputime,
      usertime.count(),
      100.*cputime/usertime.count());
}
#endif
static void help()
{
  printf("\nUsage:\n");
  printf("\n./analyzer.exe [-h] [-R8081] [-oOutputfile.mid] [file1 file2 ...] [-- arguments passed to modules ...]\n");
  printf("\n");
  printf("\t-h: print this help message\n");
  printf("\t--demo: activate the demo mode, online connection or input file not needed, midas events are generated internally\n");
  printf("\t-Hhostname: connect to MIDAS experiment on given host\n");
  printf("\t-Eexptname: connect to this MIDAS experiment\n");
  printf("\t-oOutputfile.mid: write selected events into this file\n");
  printf("\t-Rnnnn: Start the ROOT THttpServer HTTP server on specified tcp port, access by firefox http://localhost:8081\n");
  printf("\t-Xnnnn: Start the Xml server on specified tcp port (for use with roody -Xlocalhost:9091)\n");
  printf("\t-Pnnnn: Start the TNetDirectory server on specified tcp port (for use with roody -Plocalhost:9091)\n");
  printf("\t-eNNN: Number of events to analyze\n");
  printf("\t-sNNN: Number of events to skip before starting analysis\n");
  printf("\t--dump: activate the event dump module\n");
  printf("\t-t: Enable tracing of constructors, destructors and function calls\n");
  printf("\t-m: Enable memory leak debugging\n");
  printf("\t-g: Enable graphics display when processing data files\n");
  printf("\t-i: Enable intractive mode\n");
#ifdef HAVE_CXX11_THREADS
  printf("\t--mt: Enable multithreaded mode. Extra multithread config settings:\n");
  printf("\t\t--mtqlNNN: Module thread queue length (buffer).              Default: %d\n", gDefaultMultithreadQueueLength);
  printf("\t\t--mtseNNN: Module thread sleep time with empty queue (usec). Default: %d\n", gDefaultMultithreadWaitEmpty);
  printf("\t\t--mtsfNNN: Module thread sleep time when next queue is full (usec). Default: %d\n", gDefaultMultithreadWaitFull);
#else
  printf("\t--mt: Enable multithreaded mode is not available: no C++11 threads\n");
#endif
  printf("\t--no-profiler: Turn off manalyzer module profiler\n");
  printf("\t--pqiNNN: Profile multithread queue lengths every NNN events \n");
#ifdef HAVE_ROOT
  printf("\t-OXXX.root: Specify output root file filename (.root extension optional)\n");
#endif
  printf("\t--: All following arguments are passed to the analyzer modules Init() method\n");
  printf("\n");
  printf("Analyzer modules usage:\n");
  if (gModules)
     for (unsigned i=0; i<(*gModules).size(); i++)
        (*gModules)[i]->Usage();
  printf("Example1: analyze online data: ./analyzer.exe -P9091\n");
  printf("Example2: analyze existing data: ./analyzer.exe /data/alpha/current/run00500.mid\n");
  exit(1);
}

// Main function call

int manalyzer_main(int argc, char* argv[])
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

   int  tcpPort = 0;
   int  xmlTcpPort = 0;
   int  httpPort = 0;
#ifdef HAVE_MIDAS
   const char* hostname = NULL;
   const char* exptname = NULL;
#endif
   int num_skip = 0;
   int num_analyze = 0;

   TMWriterInterface *writer = NULL;

   bool event_dump = false;
   bool demo_mode = false;
   bool root_graphics = false;
   bool interactive = false;

   bool multithread = false;

   Profiler* performance_profiler=new Profiler();
   std::vector<std::string> files;
   std::vector<std::string> modargs;

   for (unsigned int i=1; i<args.size(); i++) { // loop over the commandline options
      const char* arg = args[i].c_str();
      //printf("argv[%d] is %s\n",i,arg);

      if (args[i] == "--") {
         for (unsigned j=i+1; j<args.size(); j++)
            modargs.push_back(args[j]);
         break;
      } else if (args[i] == "--dump") {
         event_dump = true;
      } else if (args[i] == "--demo") {
         demo_mode = true;
      } else if (args[i] == "-g") {
         root_graphics = true;
      } else if (args[i] == "-i") {
         interactive = true;
      } else if (args[i] == "-t") {
         gTrace = true;
         TMReaderInterface::fgTrace = true;
         TMWriterInterface::fgTrace = true;
      } else if (strncmp(arg,"-o",2)==0) {
         writer = TMNewWriter(arg+2);
      } else if (strncmp(arg,"-s",2)==0) {
         num_skip = atoi(arg+2);
      } else if (strncmp(arg,"-e",2)==0) {
         num_analyze = atoi(arg+2);
      } else if (strncmp(arg,"-m",2)==0) { // Enable memory debugging
         gEnableShowMem = true;
      } else if (strncmp(arg,"-P",2)==0) { // Set the histogram server port
         tcpPort = atoi(arg+2);
      } else if (strncmp(arg,"-X",2)==0) { // Set the histogram server port
         xmlTcpPort = atoi(arg+2);
      } else if (strncmp(arg,"-R",2)==0) { // Set the ROOT THttpServer HTTP port
         httpPort = atoi(arg+2);
#ifdef HAVE_MIDAS
      } else if (strncmp(arg,"-H",2)==0) {
         hostname = strdup(arg+2);
      } else if (strncmp(arg,"-E",2)==0) {
         exptname = strdup(arg+2);
#endif
#ifdef HAVE_CXX11_THREADS
      } else if (strncmp(arg,"--mtql",6)==0) {
         gDefaultMultithreadQueueLength = atoi(arg+6);
      } else if (strncmp(arg,"--mtse",6)==0) {
         gDefaultMultithreadWaitEmpty = atoi(arg+6);
      } else if (strncmp(arg,"--mtsf",6)==0) {
         gDefaultMultithreadWaitFull = atoi(arg+6);
      } else if (strncmp(arg,"--mt",4)==0) {
         multithread=true;
#endif
#if __cplusplus >= 201103L 
//C++11 or above is needed for chrono... therefor the profiler needs c++11 for any functionality
      } else if (strncmp(arg,"--no-profiler",13)==0) {
         performance_profiler->TimeModules=false;
      } else if (strncmp(arg,"--pqi",13)==0) {
         performance_profiler->QueueInterval=atoi(arg+5);
#endif
#ifdef HAVE_ROOT
      } else if (strncmp(arg,"-O",2)==0) {
          TARootHelper::fOutputFileName = strdup(arg+2);
          continue;
#endif
      } else if (strcmp(arg,"-h")==0) {
         help(); // does not return
      } else if (arg[0] == '-') {
         help(); // does not return
      } else {
         files.push_back(args[i]);
      }
   }

   if (!gModules)
      gModules = new std::vector<TAFactory*>;

   if ((*gModules).size() == 0)
      event_dump = true;

   if (event_dump)
      (*gModules).push_back(new EventDumpModuleFactory);

   if (interactive)
      (*gModules).push_back(new InteractiveModuleFactory);

   printf("Registered modules: %d\n", (int)(*gModules).size());

#ifdef HAVE_ROOT
   if (root_graphics) {
      TARootHelper::fgApp = new TApplication("manalyzer", NULL, NULL, 0, 0);
   }

   TARootHelper::fgDir = new TDirectory("manalyzer", "location of histograms");
   TARootHelper::fgDir->cd();
#endif
#ifdef XHAVE_LIBNETDIRECTORY
   if (tcpPort) {
      VerboseNetDirectoryServer(true);
      StartNetDirectoryServer(tcpPort, TARootHelper::fgDir);
   }
#else
   if (tcpPort)
      fprintf(stderr,"ERROR: No support for the TNetDirectory server!\n");
#endif

#ifdef HAVE_XMLSERVER
   if (xmlTcpPort) {
      XmlServer* s = new XmlServer();
      s->SetVerbose(true);
      s->Start(xmlTcpPort);
      s->Export(gROOT, "ROOT");
      s->Export(TARootHelper::fgDir, "manalyzer");
      TARootHelper::fgXmlServer = s;
   }
#else
   if (xmlTcpPort)
      fprintf(stderr,"ERROR: No support for the XML Server!\n");
#endif
   
   if (httpPort) {
#ifdef HAVE_THTTP_SERVER
      char str[256];
      sprintf(str, "http:127.0.0.1:%d", httpPort);
      THttpServer *s = new THttpServer(str);
      //s->SetTimer(100, kFALSE);
      TARootHelper::fgHttpServer = s;
#else
      fprintf(stderr,"ERROR: No support for the THttpServer!\n");
#endif
   }
   
   for (unsigned i=0; i<files.size(); i++) {
      printf("file[%d]: %s\n", i, files[i].c_str());
   }

   if (demo_mode) {
      ProcessDemoMode(modargs, num_skip, num_analyze, writer, multithread, performance_profiler);
   } else if (files.size() > 0) {
      ProcessMidasFiles(files, modargs, num_skip, num_analyze, writer, multithread, performance_profiler);
   } else {
#ifdef HAVE_MIDAS
      ProcessMidasOnline(modargs, hostname, exptname, num_analyze, writer, multithread, performance_profiler);
#endif
   }

   if (writer) {
      writer->Close();
      delete writer;
      writer = NULL;
   }
   delete performance_profiler;

   return 0;
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

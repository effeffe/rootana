// manalyzer.h

#ifndef MANALYZER_H
#define MANALYZER_H

#include <string>
#include <vector>
#include <deque>

#include "rootana_config.h"

#ifdef HAVE_CXX11_THREADS
#include <thread>
#include <mutex>
#endif

#include "midasio.h"
#include "mvodb.h"

class TARootHelper;
class TAMultithreadHelper;
class TAFlowEvent;

class TARunInfo
{
 public:
   int fRunNo;
   std::string fFileName;
   MVOdb* fOdb;
   TARootHelper* fRoot;
   TAMultithreadHelper* fMtInfo;
   std::vector<std::string> fArgs;
   static std::vector<std::string> fgFileList;
   static int fgCurrentFileIndex;
   
 public:
   TARunInfo(int runno, const char* filename, const std::vector<std::string>& args);
   ~TARunInfo();

 public:
   void AddToFlowQueue(TAFlowEvent*);
   TAFlowEvent* ReadFlowQueue();

 private:
   TARunInfo() {}; // hidden default constructor

 private:
   std::deque<TAFlowEvent*> fFlowQueue;
};

class TAFlowEvent
{
 public:
   TAFlowEvent* fNext;

 public:
   TAFlowEvent(TAFlowEvent*);
   virtual ~TAFlowEvent();

   template<class T> T* Find()
      {
         TAFlowEvent* f = this;
         while (f) {
            T *ptr = dynamic_cast<T*>(f);
            if (ptr) return ptr;
            f = f->fNext;
         }
         return NULL;
      }

 private:
   TAFlowEvent() {}; // hidden default constructor 
};

typedef int TAFlags;
#define TAFlag_OK          0
#define TAFlag_SKIP    (1<<0)
#define TAFlag_QUIT    (1<<1)
#define TAFlag_WRITE   (1<<2)
#define TAFlag_DISPLAY (1<<3)
#define TAFlag_SKIP_PROFILE (1<<4)
//Can we make TAFlags a class? (keep compatability of int, but users 
//dont need to learn bitwise operations)
#if 0
class TAFlags
{
private:
   int flag;
public:
   TAFlags()
   {
      flag=0;
   }
   void SetOK()             { flag =  0; }
   void SetSkip()           { flag |= TAFlag_SKIP; }
   void SetQuit()           { flag |= TAFlag_QUIT; }
   void SetWrite()          { flag |= TAFlag_WRITE; }
   void SetDisplay()        { flag |= TAFlag_DISPLAY; }
   void SkipProfiler()      { flag |= TAFlag_SKIP_PROFILE; }

   void UnsetSkip()         { flag -= flag & TAFlag_SKIP; }
   void UnsetQuit()         { flag -= flag & TAFlag_QUIT; }
   void UnsetWrite()        { flag -= flag & TAFlag_WRITE; }
   void UnsetDisplay()      { flag -= flag & TAFlag_DISPLAY; }
   void UnsetSkipProfiler() { flag -= flag & TAFlag_SKIP_PROFILE; }
   
   int operator&(int rhs)  { return flag & rhs; }
};
#endif
class TARunObject
{
   
public:
   std::string ModuleName;

   TARunObject(TARunInfo* runinfo); // ctor
   virtual ~TARunObject() {}; // dtor
 public:
   virtual void BeginRun(TARunInfo* runinfo); // begin of run
   virtual void EndRun(TARunInfo* runinfo); // end of run
   virtual void NextSubrun(TARunInfo* runinfo); // next subrun file

   virtual void PauseRun(TARunInfo* runinfo); // pause of run (if online)
   virtual void ResumeRun(TARunInfo* runinfo); // resume of run (if online)

   virtual void PreEndRun(TARunInfo* runinfo); // generate flow events before end of run

   virtual TAFlowEvent* Analyze(TARunInfo* runinfo, TMEvent* event, TAFlags* flags, TAFlowEvent* flow);
   virtual TAFlowEvent* AnalyzeFlowEvent(TARunInfo* runinfo, TAFlags* flags, TAFlowEvent* flow);
   virtual void AnalyzeSpecialEvent(TARunInfo* runinfo, TMEvent* event);

 private:
   TARunObject(); // hidden default constructor
};

class TAFactory
{
 public:
   TAFactory() {}; // ctor
   virtual ~TAFactory() {}; // dtor

 public:
   virtual TARunObject* NewRunObject(TARunInfo* runinfo) = 0; // factory for Run objects

 public:
   virtual void Usage(); // Display usage (flags to pass to init etc)
   virtual void Init(const std::vector<std::string> &args); // start of analysis
   virtual void Finish(); // end of analysis
};

template<class T> class TAFactoryTemplate: public TAFactory
{
   T* NewRunObject(TARunInfo* runinfo)
   {
      return new T(runinfo);
   }
};

class TARegister
{
 public:
   TARegister(TAFactory* m);
   //static void Register(TAModuleInterface* m);
   //static std::vector<TAModuleInterface*>* fgModules;
   //static std::vector<TAModuleInterface*>* Get() const;
};

#ifdef HAVE_ROOT

#include "TFile.h"
#include "TDirectory.h"
#include "TApplication.h"

class XmlServer;
class THttpServer;

class TARootHelper
{
 public:
   static std::string fOutputFileName;
   TFile* fOutputFile;
   static TDirectory*   fgDir;
   static TApplication* fgApp;
   static XmlServer*    fgXmlServer;
   static THttpServer*  fgHttpServer;

 public:
   TARootHelper(const TARunInfo*);
   ~TARootHelper(); // dtor

 private:
   TARootHelper() { }; // hidden default constructor
};
#endif

#ifdef HAVE_CXX11_THREADS

typedef std::deque<TAFlowEvent*> TAFlowEventQueue;
typedef std::deque<TAFlags*>     TAFlagsQueue;

class TAMultithreadHelper
{
public:
   // per-module queues
   std::vector<TAFlowEventQueue> fMtFlowQueue;
   std::vector<TAFlagsQueue>     fMtFlagQueue;
   // lock for the queues
   std::vector<std::mutex>       fMtFlowQueueMutex;
   // per-module threads
   std::vector<std::thread*> fMtThreads;
   // maximum number of flow events to queue
   int fMtQueueDepth;
   // end-of-run event marker
   TAFlowEvent* fMtLastItemInQueue;
   // flag to shutdown all threads
   bool fMtShutdown;
   // quit flag from AnalyzeFlowEvent()
   bool fMtQuit;
   // queue settings
   int  fMtQueueFullUSleepTime;  // u seconds
   int  fMtQueueEmptyUSleepTime; // u seconds
   
   static bool gfMultithread;
   static int  gfMtMaxBacklog;
   static std::mutex gfLock; //Lock for modules to execute code that is not thread safe (many root fitting libraries)

public:
   TAMultithreadHelper(); // ctor
   ~TAMultithreadHelper(); // dtor
};
#endif



#define CLOCK_TYPE std::chrono::time_point<std::chrono::system_clock>
#define CLOCK_NOW std::chrono::high_resolution_clock::now();
#define START_TIMER auto timer_start=CLOCK_NOW
class UserProfilerFlow: public TAFlowEvent
{

private:
   CLOCK_TYPE start;
   CLOCK_TYPE stop;
public:
   const std::string ModuleName;

   double GetTimer()
   {
      std::chrono::duration<double> elapsed_seconds = stop - start;
      return  elapsed_seconds.count();
      //return (double)(stop - start)/CLOCKS_PER_SEC;
   }
   //std::chrono::time_point<std::chrono::high_resolution_clock> time;
   UserProfilerFlow(TAFlowEvent* flow, const char* _name, CLOCK_TYPE _start) : TAFlowEvent(flow), ModuleName(_name)
   {
      start=_start;
      stop=CLOCK_NOW
   }
   ~UserProfilerFlow() // dtor
   {
   }
};

#ifdef HAVE_ROOT
#include "TH1D.h"
#endif
#include <chrono>
#include <unistd.h>  //readlink
#include <algorithm> //std::max_element

class Profiler
{
private:
   std::string binary_name;
   std::string binary_path;
   clock_t tStart_cpu;
   time_t tStart_user;
   time_t midas_start_time;
   time_t midas_stop_time;

   //Track Queue lengths when multithreading
#ifdef HAVE_ROOT
   int NQueues=0;
   std::vector<TH1D*> AnalysisQueue;
   int QueueIntervalCounter=0;
   //Number of events between samples
   int QueueInterval=100;
#endif


   //Track Analyse TMEvent time per module (main thread)
#ifdef HAVE_ROOT
   std::vector<TH1D*>  UnpackTimeHistograms;
#else
   std::vector<std::string> ModuleNames;
   double unpack_mean;
   double unpack_rms;
   int    unpack_entries;
#endif
   std::vector<double> MaxUnpackTime;
   std::vector<double> TotalUnpackTime;
   //Track Analyse flow event time per module (can be multiple threads)
#ifdef HAVE_ROOT
   std::vector<TH1D*>  ModuleTimeHistograms;
#else
   double module_mean;
   double module_rms;
   int    module_entries;
#endif
   std::vector<double> MaxModuleTime;
   std::vector<double> TotalModuleTime;
   
   //Track user profiling
   std::map<unsigned int,int> UserMap;
   std::vector<TH1D*> UserHistograms;
   std::vector<double> TotalUserTime;
   std::vector<double> MaxUserTime;


public:
   bool TimeModules;
   Profiler();

   void AddModuleMap( const char* UserProfileName, unsigned long hash)
   {
      #ifdef HAVE_CXX11_THREADS
      std::lock_guard<std::mutex> lock(TAMultithreadHelper::gfLock);
      #endif
      gDirectory->cd("/AnalysisReport");
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
      return;
   }
   
   void begin(TARunInfo* runinfo,const std::vector<TARunObject*> fRunRun );
   void log(TAFlags* flag, TAFlowEvent* flow,int i,const char* module_name,CLOCK_TYPE start);
   void log_unpack_time(TAFlags* flag, TAFlowEvent* flow,int i,const char* module_name,CLOCK_TYPE start);
   void log_user_profiling(TAFlags* flag, TAFlowEvent* flow);
   void log_mt_queue_length(TARunInfo* runinfo);
   void end();
};


int manalyzer_main(int argc, char* argv[]);

#endif

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */


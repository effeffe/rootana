// manalyzer.h

#ifndef MANALYZER_H
#define MANALYZER_H

#include <string>
#include <vector>

#include "midasio.h"
#include "VirtualOdb.h"

class TARunInfo
{
 public:
   int fRunNo;
   std::string fFileName;
   VirtualOdb* fOdb;

 public:
   TARunInfo(int runno, const std::string& filename);
   ~TARunInfo();

 private:
   TARunInfo() {}; // hidden default constructor
};

class TAFlowEvent
{
 public:
   TAFlowEvent* fNext;

 public:
   TAFlowEvent(TAFlowEvent*);
   virtual ~TAFlowEvent();

 private:
   TAFlowEvent() {}; // hidden default constructor 
};

typedef int TAFlags;

#define TAFlag_OK          0
#define TAFlag_SKIP    (1<<0)
#define TAFlag_QUIT    (1<<1)
#define TAFlag_WRITE   (1<<2)
#define TAFlag_DISPLAY (1<<3)

class TARunInterface
{
 public:
   TARunInterface(TARunInfo* runinfo); // ctor
   virtual ~TARunInterface() {}; // dtor

 public:
   virtual void BeginRun(TARunInfo* runinfo) = 0; // begin of run
   virtual void EndRun(TARunInfo* runinfo) = 0; // end of run

   virtual void PauseRun(TARunInfo* runinfo) = 0; // pause of run (if online)
   virtual void ResumeRun(TARunInfo* runinfo) = 0; // resume of run (if online)

   virtual TAFlowEvent* Analyze(TARunInfo* runinfo, TMEvent* event, TAFlags* flags, TAFlowEvent* flow) = 0;
   virtual void AnalyzeSpecialEvent(TARunInfo* runinfo, TMEvent* event) = 0;

 private:
   TARunInterface(); // hidden default constructor
};

class TAModuleInterface
{
 public:
   TAModuleInterface() {}; // ctor
   virtual ~TAModuleInterface() {}; // dtor

 public:
   virtual TARunInterface* NewRun(TARunInfo* runinfo) = 0; // factory for module-specific Run objects

 public:
   virtual void Init(const std::vector<std::string> &args) = 0; // start of analysis
   virtual void Finish() = 0; // end of analysis
};

class TARegisterModule
{
 public:
   TARegisterModule(TAModuleInterface* m);
};

#ifdef HAVE_ROOT

#include "TFile.h"

class TARootRunInfo: public TARunInfo
{
 public:
   TFile* fRootFile;

 public:
   TARootRunInfo(const TARunInfo*);
   ~TARootRunInfo(); // dtor

   //private:
   //TARootRunInfo() { printf("TARootRunInfo::default ctor!\n"); }; // hidden default constructor
};
#endif

#endif

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */


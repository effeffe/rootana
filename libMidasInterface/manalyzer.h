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
   virtual ~TARunInfo();

 private:
   TARunInfo() {}; // hidden default constructor
};

class TAModuleInterface
{
 public:
   TAModuleInterface() {}; // ctor
   virtual ~TAModuleInterface() {}; // dtor

   //public:
   //virtual TARunInfo* NewRunInfo() = 0; // factory for module-specific RunInfo objects

 public:
   virtual void Init(const std::vector<std::string> &args) = 0; // start of analysis
   virtual void Finish() = 0; // end of analysis

   virtual void BeginRun(TARunInfo* runinfo) = 0; // begin of run
   virtual void EndRun(TARunInfo* runinfo) = 0; // end of run

   virtual void PauseRun(TARunInfo* runinfo) = 0; // pause of run (if online)
   virtual void ResumeRun(TARunInfo* runinfo) = 0; // resume of run (if online)

   virtual void Analyze(TARunInfo* runinfo, TMEvent* event) = 0;
   virtual void AnalyzeSpecialEvent(TARunInfo* runinfo, TMEvent* event) = 0;
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


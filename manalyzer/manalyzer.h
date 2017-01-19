// manalyzer.h

#ifndef MANALYZER_H
#define MANALYZER_H

#include <string>
#include <vector>

#include "rootana_config.h"
#include "midasio.h"
#include "VirtualOdb.h"

class TARootHelper;

class TARunInfo
{
 public:
   int fRunNo;
   std::string fFileName;
   VirtualOdb* fOdb;
   TARootHelper* fRoot;

 public:
   TARunInfo(int runno, const char* filename);
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

class TARunInterface
{
 public:
   TARunInterface(TARunInfo* runinfo); // ctor
   virtual ~TARunInterface() {}; // dtor

 public:
   virtual void BeginRun(TARunInfo* runinfo); // begin of run
   virtual void EndRun(TARunInfo* runinfo); // end of run

   virtual void PauseRun(TARunInfo* runinfo); // pause of run (if online)
   virtual void ResumeRun(TARunInfo* runinfo); // resume of run (if online)

   virtual TAFlowEvent* Analyze(TARunInfo* runinfo, TMEvent* event, TAFlags* flags, TAFlowEvent* flow);
   virtual void AnalyzeSpecialEvent(TARunInfo* runinfo, TMEvent* event);

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
   virtual void Init(const std::vector<std::string> &args); // start of analysis
   virtual void Finish(); // end of analysis
};

class TARegisterModule
{
 public:
   TARegisterModule(TAModuleInterface* m);
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

int manalyzer_main(int argc, char* argv[]);

#endif

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */


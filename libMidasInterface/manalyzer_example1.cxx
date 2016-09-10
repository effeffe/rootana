//
// MIDAS analyzer example 1: pure C++ analyzer
//
// K.Olchanski
//

#include "manalyzer.h"
#include "midasio.h"

class ExampleCxxModule: public TAModuleInterface
{
public:
   void Init(const std::vector<std::string> &args);
   void Finish();
   TARunInterface* NewRun(TARunInfo* runinfo);

   int fTotalEventCounter;
};

class ExampleCxxRun: public TARunInterface
{
public:
   ExampleCxxRun(TARunInfo* runinfo, ExampleCxxModule *m)
      : TARunInterface(runinfo)
   {
      printf("ctor, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
      fModule = m;
      fRunEventCounter = 0;
   }

   ~ExampleCxxRun()
   {
      printf("dtor!\n");
   }
  
   void BeginRun(TARunInfo* runinfo)
   {
      printf("BeginRun, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
      fRunEventCounter = 0;
   }

   void EndRun(TARunInfo* runinfo)
   {
      printf("EndRun, run %d\n", runinfo->fRunNo);
      printf("Counted %d events in run %d\n", fRunEventCounter, runinfo->fRunNo);
   }

   void PauseRun(TARunInfo* runinfo)
   {
      printf("PauseRun, run %d\n", runinfo->fRunNo);
   }

   void ResumeRun(TARunInfo* runinfo)
   {
      printf("ResumeRun, run %d\n", runinfo->fRunNo);
   }

   TAFlowEvent* Analyze(TARunInfo* runinfo, TMEvent* event, TAFlags* flags, TAFlowEvent* flow)
   {
      printf("Analyze, run %d, event serno %d, id 0x%04x, data size %d\n", runinfo->fRunNo, event->serial_number, (int)event->event_id, event->data_size);
      //event->old_event.SetBankList();
      //event->old_event.Print();

      fRunEventCounter++;
      fModule->fTotalEventCounter++;

      return flow;
   }

   void AnalyzeSpecialEvent(TARunInfo* runinfo, TMEvent* event)
   {
      printf("AnalyzeSpecialEvent, run %d, event serno %d, id 0x%04x, data size %d\n", runinfo->fRunNo, event->serial_number, (int)event->event_id, event->data_size);
   }

   int fRunEventCounter;

   ExampleCxxModule* fModule;
};

void ExampleCxxModule::Init(const std::vector<std::string> &args)
{
   printf("Init!\n");
   fTotalEventCounter = 0;
}
   
void ExampleCxxModule::Finish()
{
   printf("Finish!\n");
   printf("Counted %d events grand total\n", fTotalEventCounter);
}
   
TARunInterface* ExampleCxxModule::NewRun(TARunInfo* runinfo)
{
   printf("NewRun, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
   return new ExampleCxxRun(runinfo, this);
}

TARegisterModule tarm(new ExampleCxxModule);

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

//
// MIDAS analyzer example 1: pure C++ analyzer
//
// K.Olchanski
//

#include "manalyzer.h"
#include "midasio.h"

class ExampleCxxAnalyzer: public TAModuleInterface
{
   void Init(const std::vector<std::string> &args)
   {
      printf("Init!\n");
   }
   
   void Finish()
   {
      printf("Finish!\n");
   }
   
   void BeginRun(TARunInfo* runinfo)
   {
      printf("BeginRun, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
   }

   void EndRun(TARunInfo* runinfo)
   {
      printf("EndRun, run %d\n", runinfo->fRunNo);
   }

   void PauseRun(TARunInfo* runinfo)
   {
      printf("PauseRun, run %d\n", runinfo->fRunNo);
   }

   void ResumeRun(TARunInfo* runinfo)
   {
      printf("ResumeRun, run %d\n", runinfo->fRunNo);
   }

   void Analyze(TARunInfo* runinfo, TMEvent* event)
   {
      printf("Analyze, run %d, event serno %d, id 0x%04x, data size %d\n", runinfo->fRunNo, event->serial_number, (int)event->event_id, event->data_size);
      //event->old_event.SetBankList();
      //event->old_event.Print();
   }

   void AnalyzeSpecialEvent(TARunInfo* runinfo, TMEvent* event)
   {
      printf("AnalyzeSpecialEvent, run %d, event serno %d, id 0x%04x, data size %d\n", runinfo->fRunNo, event->serial_number, (int)event->event_id, event->data_size);
   }
};

TARegisterModule tarm(new ExampleCxxAnalyzer);

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

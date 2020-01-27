//
// MIDAS analyzer example 2: ROOT analyzer
//
// K.Olchanski
//

#include <stdio.h>

#include "manalyzer.h"
#include "midasio.h"

#include "TH1D.h"

struct ExampleRoot: public TARunObject
{
   int fCounter;
   TH1D* hperrun;
   
   ExampleRoot(TARunInfo* runinfo)
      : TARunObject(runinfo)
   {
      printf("ExampleRoot::ctor!\n");
   }

   ~ExampleRoot()
   {
      printf("ExampleRoot::dtor!\n");
   }

   void BeginRun(TARunInfo* runinfo)
   {
      printf("BeginRun, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
      uint32_t run_start_time_binary = 0;
      runinfo->fOdb->RU32("/Runinfo/Start time binary", &run_start_time_binary);
      time_t run_start_time = run_start_time_binary;
      printf("ODB Run start time: %d: %s", (int)run_start_time, ctime(&run_start_time));
      fCounter = 0;
      runinfo->fRoot->fOutputFile->cd(); // select correct ROOT directory
      hperrun = new TH1D("hperrun", "hperrun", 200, -100, 100);
   }

   void EndRun(TARunInfo* runinfo)
   {
      printf("EndRun, run %d, events %d\n", runinfo->fRunNo, fCounter);
      uint32_t run_stop_time_binary = 0;
      runinfo->fOdb->RU32("/Runinfo/Stop time binary", &run_stop_time_binary);
      time_t run_stop_time = run_stop_time_binary;
      printf("ODB Run stop time: %d: %s", (int)run_stop_time, ctime(&run_stop_time));
      hperrun->SaveAs("hperrun.root");
      hperrun->SaveAs("hperrun.pdf");
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
      if (event->event_id != 2)
         return flow;

      TMBank* bslow = event->FindBank("SLOW");
      if (!bslow)
         return flow;

      float* dslow = (float*)event->GetBankData(bslow);
      if (!dslow)
         return flow;

      double v = *dslow;

      printf("event %d, slow %f\n", event->serial_number, v);

      hperrun->Fill(v);
      fCounter++;

      return flow;
   }

   void AnalyzeSpecialEvent(TARunInfo* runinfo, TMEvent* event)
   {
      printf("AnalyzeSpecialEvent, run %d, event serno %d, id 0x%04x, data size %d\n", runinfo->fRunNo, event->serial_number, (int)event->event_id, event->data_size);
   }
};

class ExampleRootFactory: public TAFactory
{
public:
   void Init(const std::vector<std::string> &args)
   {
      printf("Init!\n");
   }
      
   void Finish()
   {
      printf("Finish!\n");
   }
      
   TARunObject* NewRunObject(TARunInfo* runinfo)
   {
      printf("NewRunObject, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
      return new ExampleRoot(runinfo);
   }
};

static TARegister tar(new ExampleRootFactory);

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

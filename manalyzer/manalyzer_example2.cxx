//
// MIDAS analyzer example 2: ROOT analyzer
//
// K.Olchanski
//

#include <stdio.h>

#include "manalyzer.h"
#include "midasio.h"

#include "TH1D.h"

class ExampleRootModule: public TAModuleInterface
{
public:
   void Init(const std::vector<std::string> &args);
   void Finish();
   TARunInterface* NewRun(TARunInfo* runinfo);

   int fTotalEventCounter;
   TH1D* htotal;
};

struct ExampleRootRun: public TARunInterface
{
   ExampleRootModule* fModule;
   int fCounter;
   TH1D* hperrun;
   
   ExampleRootRun(TARunInfo* runinfo, ExampleRootModule* m)
      : TARunInterface(runinfo)
   {
      printf("ExampleRootRun::ctor!\n");
      fModule = m;
   }

   void BeginRun(TARunInfo* runinfo)
   {
      printf("BeginRun, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
      time_t run_start_time = runinfo->fOdb->odbReadUint32("/Runinfo/Start time binary", 0, 0);
      printf("ODB Run start time: %d: %s", (int)run_start_time, ctime(&run_start_time));
      fCounter = 0;
      runinfo->fRoot->fOutputFile->cd(); // select correct ROOT directory
      hperrun = new TH1D("hperrun", "hperrun", 200, -100, 100);
   }

   void EndRun(TARunInfo* runinfo)
   {
      printf("EndRun, run %d, events %d\n", runinfo->fRunNo, fCounter);
      time_t run_stop_time = runinfo->fOdb->odbReadUint32("/Runinfo/Stop time binary", 0, 0);
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
         return TAFlag_OK;

      TMBank* bslow = event->FindBank("SLOW");
      if (!bslow)
         return flow;

      float* dslow = (float*)event->GetBankData(bslow);
      if (!dslow)
         return flow;

      double v = *dslow;

      printf("event %d, slow %f\n", event->serial_number, v);

      hperrun->Fill(v);
      fModule->htotal->Fill(v);

      fCounter++;
      fModule->fTotalEventCounter++;

      return flow;
   }

   void AnalyzeSpecialEvent(TARunInfo* runinfo, TMEvent* event)
   {
      printf("AnalyzeSpecialEvent, run %d, event serno %d, id 0x%04x, data size %d\n", runinfo->fRunNo, event->serial_number, (int)event->event_id, event->data_size);
   }
};

void ExampleRootModule::Init(const std::vector<std::string> &args)
{
   printf("Init!\n");
   fTotalEventCounter = 0;
   TARootHelper::fgDir->cd(); // select correct ROOT directory
   htotal = new TH1D("htotal", "htotal", 200, -100, 100);
}
   
void ExampleRootModule::Finish()
{
   printf("Finish!\n");
   printf("Counted %d events grand total\n", fTotalEventCounter);
   htotal->SaveAs("htotal.root");
   htotal->SaveAs("htotal.pdf");
}
   
TARunInterface* ExampleRootModule::NewRun(TARunInfo* runinfo)
{
   printf("NewRun, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
   return new ExampleRootRun(runinfo, this);
}

TARegisterModule tarm(new ExampleRootModule);

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

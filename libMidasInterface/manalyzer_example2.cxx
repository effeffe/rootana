//
// MIDAS analyzer example 2: ROOT analyzer
//
// K.Olchanski
//

#include "manalyzer.h"
#include "midasio.h"

#include "TH1D.h"

struct ExampleRunData: public TARootRunInfo
{
   TH1D* hperrun;
   
   ExampleRunData(const TARunInfo* runinfo)
      : TARootRunInfo(runinfo)
   {
      printf("ExampleRunData::ctor!\n");
      hperrun = new TH1D("hperrun", "hperrun", 100, -1, 1);
   }

   ~ExampleRunData()
   {

   }
};

class ExampleRootAnalyzer: public TAModuleInterface
{
   TH1D* htotal;

   TARunInfo* NewRunInfo(const TARunInfo* runinfo)
   {
      return new ExampleRunData(runinfo);
   }
   
   void Init(const std::vector<std::string> &args)
   {
      printf("Init!\n");

      htotal = new TH1D("htotal", "htotal", 100, -1, 1);
   }
   
   void Finish()
   {
      printf("Finish!\n");
      htotal->SaveAs("htotal.pdf");
   }
   
   void BeginRun(TARunInfo* runinfo)
   {
      printf("BeginRun, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
      //new ExampleRunData(runinfo);
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

TARegisterModule tarm(new ExampleRootAnalyzer);

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

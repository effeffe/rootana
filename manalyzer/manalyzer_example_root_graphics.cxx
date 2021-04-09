//
// MIDAS analyzer example: ROOT analyzer with graphics
//
// K.Olchanski
//

#include <stdio.h>

#include "manalyzer.h"
#include "midasio.h"

#include "TRandom3.h"
#include "TCanvas.h"
#include "TH1D.h"

struct ExampleGRoot: public TARunObject
{
   int fCounter;
   TCanvas *fCanvas;
   TH1D* hperrun;
   TH1D* hExample;
   TRandom* fRndm;
   
   ExampleGRoot(TARunInfo* runinfo)
      : TARunObject(runinfo)
   {
      fName="ExamlpeGRoot";
      printf("ExampleGRoot::ctor!\n");
      fCanvas = new TCanvas("example", "example", 500, 500);
      fRndm = new TRandom3(); // recommended random number generator
   }

   ~ExampleGRoot()
   {
      printf("ExampleGRoot::dtor!\n");
      if (fCanvas)
         delete fCanvas;
      fCanvas = NULL;
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
      hExample = new TH1D("hexample", "hexample", 100, -5, 5);
      fCanvas->Divide(1, 2);
      fCanvas->cd(1);
      hperrun->Draw();
      fCanvas->cd(2);
      hExample->Draw();
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

   void NextSubrun(TARunInfo* runinfo)
   {
      printf("NextSubrun, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
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

      double v = 0;
      for (int i=0; i<12; i++)
         v += fRndm->Rndm();
      v -= 6.0;
      
      hExample->Fill(v);
      
      if (event->event_id == 2) {
         TMBank* bslow = event->FindBank("SLOW");
         if (bslow) {
            float* dslow = (float*)event->GetBankData(bslow);
            if (dslow) {
               double v = *dslow;

               printf("event %d, slow %f\n", event->serial_number, v);
               
               hperrun->Fill(v);
               fCounter++;
            }
         }
      }

      //fCanvas->cd();
      //hperrun->Draw();

      fCanvas->Modified();
      fCanvas->Draw();
      fCanvas->Update();

      return flow;
   }

   void AnalyzeSpecialEvent(TARunInfo* runinfo, TMEvent* event)
   {
      printf("AnalyzeSpecialEvent, run %d, event serno %d, id 0x%04x, data size %d\n", runinfo->fRunNo, event->serial_number, (int)event->event_id, event->data_size);
   }
};

class ExampleGRootFactory: public TAFactory
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
      return new ExampleGRoot(runinfo);
   }
};

static TARegister tar(new ExampleGRootFactory);

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

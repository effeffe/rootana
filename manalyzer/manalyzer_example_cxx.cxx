//
// MIDAS analyzer example 1: pure C++ analyzer
//
// K.Olchanski
//

#include <stdio.h>

#include "manalyzer.h"
#include "midasio.h"

class ExampleCxx: public TARunObject
{
public:
   ExampleCxx(TARunInfo* runinfo)
      : TARunObject(runinfo)
   {
      fName="ExampleCxx";
      printf("ctor, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
      fRunEventCounter = 0;
   }

   ~ExampleCxx()
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
      //event->old_event.SetBankList();
      //event->old_event.Print();
      START_TIMER;

      fRunEventCounter++;
      flow = new UserProfilerFlow(flow,"custom profiling",timer_start);
      return flow;
   }

   void AnalyzeSpecialEvent(TARunInfo* runinfo, TMEvent* event)
   {
      printf("AnalyzeSpecialEvent, run %d, event serno %d, id 0x%04x, data size %d\n", runinfo->fRunNo, event->serial_number, (int)event->event_id, event->data_size);
   }

   int fRunEventCounter;
};

class ExampleCxxFactory: public TAFactory
{
public:
   void Usage()
   {
      printf("\tExample TAFactory Usage!\n");
      printf("\tPrint valid arguements for this modules here!");
   }
   void Init(const std::vector<std::string> &args)
   {
      printf("Init!\n");
      printf("Arguments:\n");
      for (unsigned i=0; i<args.size(); i++)
         printf("arg[%d]: [%s]\n", i, args[i].c_str());
   }
   
   void Finish()
   {
      printf("Finish!\n");
   }
   
   TARunObject* NewRunObject(TARunInfo* runinfo)
   {
      printf("NewRunObject, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
      return new ExampleCxx(runinfo);
   }
};

static TARegister tar(new ExampleCxxFactory);

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

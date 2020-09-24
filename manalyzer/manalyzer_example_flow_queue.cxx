//
// MIDAS analyzer example 4: C++ flow analyzer
//
// K.Olchanski
//

#include <stdio.h>

#include "manalyzer.h"
#include "midasio.h"

class PhysicsEvent : public TAFlowEvent
{
public:
   int fSeqNo;
   static int gfCounter;

   PhysicsEvent(TAFlowEvent* flow)
      : TAFlowEvent(flow)
   {
      fSeqNo = ++gfCounter;
      printf("PhysicsEvent::ctor: %d\n", fSeqNo);
   }

   ~PhysicsEvent()
   {
      printf("PhysicsEvent::dtor: %d\n", fSeqNo);
   }
};

int PhysicsEvent::gfCounter = 0;

class Module1: public TARunObject
{
public:
   Module1(TARunInfo* runinfo)
      : TARunObject(runinfo)
   {
      ModuleName="Module1";
      printf("Module1::ctor, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
   }

   ~Module1()
   {
      printf("Module1::dtor!\n");
   }
  
   TAFlowEvent* Analyze(TARunInfo* runinfo, TMEvent* event, TAFlags* flags, TAFlowEvent* flow)
   {
      printf("Module1::Analyze, run %d, event serno %d, id 0x%04x, data size %d\n", runinfo->fRunNo, event->serial_number, (int)event->event_id, event->data_size);

      //
      // one midas event contains multiple physics events:
      //
      for (int i=0; i<3; i++) {
         //
         // unpack the physics events
         //
         // PhysicsEvent* e = unpack(event, i);
         PhysicsEvent* e = new PhysicsEvent(NULL);

         // push physics events into the event queue
         runinfo->AddToFlowQueue(e);

         // analysis of the PhysicsEvent should be done
         // in AnalyzeFlowEvent()
      }

      return flow; // normal flow mechanism is not used here
   }

   void PreEndRun(TARunInfo* runinfo)
   {
      printf("Module1::PreEndRun, run %d\n", runinfo->fRunNo);

      // after receiving the last midas event in Analyze(),
      // the upacking code may have some left-over data
      // in it's buffers and queues, here we convert it into
      // the last physics events
      
      for (int i=0; i<3; i++) {
         //
         // unpack the physics events
         //
         // PhysicsEvent* e = unpack_buffered_data(i);
         PhysicsEvent* e = new PhysicsEvent(NULL);

         // push physics events into the event queue
         runinfo->AddToFlowQueue(e);
      }
   }

   TAFlowEvent* AnalyzeFlowEvent(TARunInfo* runinfo, TAFlags* flags, TAFlowEvent* flow)
   {
      PhysicsEvent* e = flow->Find<PhysicsEvent>();
      if (e) {
         printf("Module1::AnalyzeFlowEvent, run %d, PhysicsEvent.seqno %d\n", runinfo->fRunNo, e->fSeqNo);
         //
         // analyze the physics event here
         //
      }
      return flow;
   }

   void EndRun(TARunInfo* runinfo)
   {
      printf("Module1::EndRun, run %d\n", runinfo->fRunNo);

      // NB: we should not create and queue any new PhysicsEvents
      // because they will not be analyzed.
      // not permitted: runinfo->fFlowQueue.push_back(e);
   }
};

class Module2: public TARunObject
{
public:
   Module2(TARunInfo* runinfo)
      : TARunObject(runinfo)
   {
      ModuleName="Module2";
      printf("Module2::ctor, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
   }

   ~Module2()
   {
      printf("Module2::dtor!\n");
   }
  
   TAFlowEvent* AnalyzeFlowEvent(TARunInfo* runinfo, TAFlags* flags, TAFlowEvent* flow)
   {
      PhysicsEvent* e = flow->Find<PhysicsEvent>();
      if (e) {
         printf("Module2::AnalyzeFlowEvent, run %d, PhysicsEvent.seqno %d\n", runinfo->fRunNo, e->fSeqNo);
         //
         // do some additional analysis of PhysicsEvent here
         //
      }
      return flow;
   }
};

static TARegister tar1(new TAFactoryTemplate<Module1>);
static TARegister tar2(new TAFactoryTemplate<Module2>);

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

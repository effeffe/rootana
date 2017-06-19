//
// MIDAS analyzer example 3: C++ flow analyzer
//
// K.Olchanski
//

#include <stdio.h>

#include "manalyzer.h"
#include "midasio.h"

class Object1 : public TAFlowEvent
{
public:
   int fIntValue;

   Object1(TAFlowEvent* flow, int value)
      : TAFlowEvent(flow)
   {
      fIntValue = value;
   }
};

class Object2 : public TAFlowEvent
{
public:
   std::string fStringValue;

   Object2(TAFlowEvent* flow, const std::string& stringValue)
      : TAFlowEvent(flow)
   {
      fStringValue = stringValue;
   }
};

class Object3 : public TAFlowEvent
{
public:
   double* fPtrValue;

   Object3(TAFlowEvent* flow, double* doublePtr)
      : TAFlowEvent(flow)
   {
      fPtrValue = doublePtr;
   }

   ~Object3() // dtor
   {
      if (fPtrValue) {
         delete fPtrValue;
         fPtrValue = NULL;
      }
   }
};

class Example1: public TARunObject
{
public:
   Example1(TARunInfo* runinfo)
      : TARunObject(runinfo)
   {
      printf("Example1::ctor, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
   }

   ~Example1()
   {
      printf("Example1::dtor!\n");
   }
  
   TAFlowEvent* Analyze(TARunInfo* runinfo, TMEvent* event, TAFlags* flags, TAFlowEvent* flow)
   {
      printf("Example1::Analyze, run %d, event serno %d, id 0x%04x, data size %d\n", runinfo->fRunNo, event->serial_number, (int)event->event_id, event->data_size);

      flow = new Object1(flow, 10);
      flow = new Object2(flow, "some text");

      return flow;
   }
};

class Example2: public TARunObject
{
public:
   Example2(TARunInfo* runinfo)
      : TARunObject(runinfo)
   {
      printf("Example2::ctor, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
   }

   ~Example2()
   {
      printf("Example2::dtor!\n");
   }
  
   void PreEndRun(TARunInfo* runinfo, std::deque<TAFlowEvent*>* flow_queue)
   {
      TAFlowEvent* flow = NULL;
      
      printf("Example2::PreEndRun, run %d\n", runinfo->fRunNo);

      double *dptr = new double;
      *dptr = 17.1;
      
      flow = new Object3(flow, dptr);

      flow_queue->push_back(flow);
   }
      
   TAFlowEvent* Analyze(TARunInfo* runinfo, TMEvent* event, TAFlags* flags, TAFlowEvent* flow)
   {
      printf("Example2::Analyze, run %d, event serno %d, id 0x%04x, data size %d\n", runinfo->fRunNo, event->serial_number, (int)event->event_id, event->data_size);

      double *dptr = new double;
      *dptr = 3.14;
      
      flow = new Object3(flow, dptr);

      return flow;
   }
      
   TAFlowEvent* AnalyzeFlowEvent(TARunInfo* runinfo, TAFlags* flags, TAFlowEvent* flow)
   {
      printf("Example2::AnalyzeFlowEvent, run %d\n", runinfo->fRunNo);

      // example iterating over flow events

      if (flow) {
         TAFlowEvent* f = flow;
         while (f) {
            Object1* o1 = dynamic_cast<Object1*>(f);
            Object2* o2 = dynamic_cast<Object2*>(f);
            Object3* o3 = dynamic_cast<Object3*>(f);

            printf("flow event %p, o1: %p, o2: %p, o3: %p\n", f, o1, o2, o3);

            if (o1)
               printf("object1 int value: %d\n", o1->fIntValue);
            
            if (o2)
               printf("object2 string value: %s\n", o2->fStringValue.c_str());
            
            if (o3)
               printf("object3 pointer to double value: %f\n", *o3->fPtrValue);
            
            f = f->fNext;
         }
      }

      // example direct get of flow events

      if (flow) {
         Object1* o1 = flow->Find<Object1>();
         Object2* o2 = flow->Find<Object2>();
         Object3* o3 = flow->Find<Object3>();

         if (o1)
            printf("find object1 int value: %d\n", o1->fIntValue);
         
         if (o2)
            printf("find object2 string value: %s\n", o2->fStringValue.c_str());

         if (o3)
            printf("find object3 pointer to double value: %f\n", *o3->fPtrValue);
      }

      return flow;
   }
};

static TARegister tar1(new TAFactoryTemplate<Example1>);
static TARegister tar2(new TAFactoryTemplate<Example2>);

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

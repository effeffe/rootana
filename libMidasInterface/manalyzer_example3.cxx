//
// MIDAS analyzer example 3: C++ flow analyzer
//
// K.Olchanski
//

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

class ExampleModule1: public TAModuleInterface
{
public:
   TARunInterface* NewRun(TARunInfo* runinfo);
};

class ExampleModule2: public TAModuleInterface
{
public:
   TARunInterface* NewRun(TARunInfo* runinfo);
};

class ExampleRun1: public TARunInterface
{
public:
   ExampleRun1(TARunInfo* runinfo, ExampleModule1 *m)
      : TARunInterface(runinfo)
   {
      printf("ExampleRun1::ctor, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
      fModule = m;
   }

   ~ExampleRun1()
   {
      printf("ExampleRun1::dtor!\n");
   }
  
   TAFlowEvent* Analyze(TARunInfo* runinfo, TMEvent* event, TAFlags* flags, TAFlowEvent* flow)
   {
      printf("ExampleRun1::Analyze, run %d, event serno %d, id 0x%04x, data size %d\n", runinfo->fRunNo, event->serial_number, (int)event->event_id, event->data_size);

      flow = new Object1(flow, 10);
      flow = new Object2(flow, "some text");

      return flow;
   }

   ExampleModule1* fModule;
};

class ExampleRun2: public TARunInterface
{
public:
   ExampleRun2(TARunInfo* runinfo, ExampleModule2 *m)
      : TARunInterface(runinfo)
   {
      printf("ExampleRun2::ctor, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
      fModule = m;
   }

   ~ExampleRun2()
   {
      printf("ExampleRun2::dtor!\n");
   }
  
   TAFlowEvent* Analyze(TARunInfo* runinfo, TMEvent* event, TAFlags* flags, TAFlowEvent* flow)
   {
      printf("ExampleRun2::Analyze, run %d, event serno %d, id 0x%04x, data size %d\n", runinfo->fRunNo, event->serial_number, (int)event->event_id, event->data_size);

      // example iterating over flow events

      if (flow) {
         TAFlowEvent* f = flow;
         while (f) {
            Object1* o1 = dynamic_cast<Object1*>(f);
            Object2* o2 = dynamic_cast<Object2*>(f);

            printf("flow event %p, o1: %p, o2: %p\n", f, o1, o2);

            if (o1)
               printf("object1 int value: %d\n", o1->fIntValue);
            
            if (o2)
               printf("object2 string value: %s\n", o2->fStringValue.c_str());
            
            f = f->fNext;
         }
      }

      // example direct get of flow events

      if (flow) {
         Object1* o1 = flow->Find<Object1>();
         Object2* o2 = flow->Find<Object2>();

         if (o1)
            printf("find object1 int value: %d\n", o1->fIntValue);
         
         if (o2)
            printf("find object2 string value: %s\n", o2->fStringValue.c_str());
      }

      return flow;
   }

   ExampleModule2* fModule;
};

TARunInterface* ExampleModule1::NewRun(TARunInfo* runinfo)
{
   return new ExampleRun1(runinfo, this);
}

TARunInterface* ExampleModule2::NewRun(TARunInfo* runinfo)
{
   return new ExampleRun2(runinfo, this);
}

TARegisterModule tarm1(new ExampleModule1);
TARegisterModule tarm2(new ExampleModule2);

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

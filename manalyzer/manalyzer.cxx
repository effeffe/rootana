//
// MIDAS analyzer
//
// K.Olchanski
//

#include <stdio.h>
#include <assert.h>

#include "manalyzer.h"
#include "midasio.h"

//////////////////////////////////////////////////////////
//
// Methods of TARunInfo
//
//////////////////////////////////////////////////////////

TARunInfo::TARunInfo(int runno, const char* filename)
{
   printf("TARunInfo::ctor!\n");
   fRunNo = runno;
   if (filename)
      fFileName = filename;
   fOdb = NULL;
#ifdef HAVE_ROOT
   fRoot = new TARootHelper(this);
#endif
}

TARunInfo::~TARunInfo()
{
   printf("TARunInfo::dtor!\n");
   fRunNo = 0;
   fFileName = "(deleted)";
   if (fOdb) {
      delete fOdb;
      fOdb = NULL;
   }
#ifdef HAVE_ROOT
   if (fRoot) {
      delete fRoot;
      fRoot = NULL;
   }
#endif
}

//////////////////////////////////////////////////////////
//
// Methods of TAFlowEvent
//
//////////////////////////////////////////////////////////

TAFlowEvent::TAFlowEvent(TAFlowEvent* flow) // ctor
{
   printf("TAFlowEvent::ctor: chain %p\n", flow);
   fNext = flow;
}

TAFlowEvent::~TAFlowEvent() // dtor
{
   printf("TAFlowEvent::dtor: this %p, next %p\n", this, fNext);
   if (fNext)
      delete fNext;
   fNext = NULL;
}

//////////////////////////////////////////////////////////
//
// Methods of TARunInterace
//
//////////////////////////////////////////////////////////

TARunInterface::TARunInterface(TARunInfo* runinfo)
{
   printf("TARunInterface::ctor, run %d\n", runinfo->fRunNo);
}

void TARunInterface::BeginRun(TARunInfo* runinfo)
{
   printf("TARunInterface::BeginRun, run %d\n", runinfo->fRunNo);
}

void TARunInterface::EndRun(TARunInfo* runinfo)
{
   printf("TARunInterface::EndRun, run %d\n", runinfo->fRunNo);
}

void TARunInterface::PauseRun(TARunInfo* runinfo)
{
   printf("TARunInterface::PauseRun, run %d\n", runinfo->fRunNo);
}

void TARunInterface::ResumeRun(TARunInfo* runinfo)
{
   printf("TARunInterface::ResumeRun, run %d\n", runinfo->fRunNo);
}

TAFlowEvent* TARunInterface::Analyze(TARunInfo* runinfo, TMEvent* event, TAFlags* flags, TAFlowEvent* flow)
{
   printf("TARunInterface::Analyze!\n");
   return flow;
}

void TARunInterface::AnalyzeSpecialEvent(TARunInfo* runinfo, TMEvent* event)
{
   printf("TARunInterface::AnalyzeSpecialEvent!\n");
}

//////////////////////////////////////////////////////////
//
// Methods of TAModuleInterface
//
//////////////////////////////////////////////////////////

void TAModuleInterface::Init(const std::vector<std::string> &args)
{
   printf("TAModuleInterface::Init!\n");
}

void TAModuleInterface::Finish()
{
   printf("TAModuleInterface::Finish!\n");
}

#ifdef HAVE_ROOT

//////////////////////////////////////////////////////////
//
// Methods of TARootHelper
//
//////////////////////////////////////////////////////////

#ifdef HAVE_XMLSERVER
#include "xmlServer.h"
#include "TROOT.h"
#endif

#ifdef XHAVE_LIBNETDIRECTORY
#include "netDirectoryServer.h"
#endif

TApplication* TARootHelper::fgApp = NULL;
TDirectory*   TARootHelper::fgDir = NULL;
XmlServer*    TARootHelper::fgXmlServer = NULL;
THttpServer*  TARootHelper::fgHttpServer = NULL;

TARootHelper::TARootHelper(const TARunInfo* runinfo) // ctor
{
   printf("TARootHelper::ctor!\n");

   char xfilename[1024];
   sprintf(xfilename, "output%05d.root", runinfo->fRunNo);

   fOutputFile = new TFile(xfilename, "RECREATE");
   
   assert(fOutputFile->IsOpen()); // FIXME: survive failure to open ROOT file

   fOutputFile->cd();

#ifdef XHAVE_LIBNETDIRECTORY
   NetDirectoryExport(fOutputFile, "ManalyzerOutputFile");
#endif
#ifdef HAVE_XMLSERVER
   if (fgXmlServer)
      fgXmlServer->Export(fOutputFile, "ManalyzerOutputFile");
#endif
}

TARootHelper::~TARootHelper() // dtor
{
   printf("TARootHelper::dtor!\n");
   
   if (fOutputFile != NULL) {
      fOutputFile->Write();
      fOutputFile->Close();
      fOutputFile = NULL;
   }

   if (fgDir)
      fgDir->cd();
}

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>
#include <signal.h>

#include "manalyzer.h"
#include "midasio.h"

#ifdef HAVE_ROOT_XML
#include "XmlOdb.h"
#endif

#ifdef HAVE_THTTP_SERVER
#include "THttpServer.h"
#endif

#ifdef HAVE_ROOT
#include <TSystem.h>
#endif

//////////////////////////////////////////////////////////
//
// Methods of TARegisterModule
//
//////////////////////////////////////////////////////////

std::vector<TAModuleInterface*> *gModules = NULL;

TARegisterModule::TARegisterModule(TAModuleInterface* m)
{
   if (!gModules)
      gModules = new std::vector<TAModuleInterface*>;
   gModules->push_back(m);
}

//////////////////////////////////////////////////////////
//
// Methods of EmptyOdb
//
//////////////////////////////////////////////////////////

class EmptyOdb: public VirtualOdb
{
public:
   int      odbReadArraySize(const char*name) { return 0; }
   int      odbReadAny(   const char*name, int index, int tid,void* buf, int bufsize = 0) { return 0; };
   int      odbReadInt(   const char*name, int index = 0, int      defaultValue = 0) { return defaultValue; }
   uint32_t odbReadUint32(const char*name, int index = 0, uint32_t defaultValue = 0) { return defaultValue; }
   float     odbReadFloat(const char*name, int index = 0, float   defaultValue = 0) { return defaultValue; }
   double   odbReadDouble(const char*name, int index = 0, double   defaultValue = 0) { return defaultValue; }
   bool     odbReadBool(  const char*name, int index = 0, bool     defaultValue = false) { return defaultValue; }
   const char* odbReadString(const char*name, int index = 0,const char* defaultValue = NULL) { return defaultValue; }
};

#if 0
static double GetTimeSec()
{
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec + 0.000001*tv.tv_usec;
}
#endif

class RunHandler
{
public:
   TARunInfo* fRunInfo;
   std::vector<TARunInterface*> fRunRun;

   RunHandler() // ctor
   {
      fRunInfo = NULL;
   }

   ~RunHandler() // dtor
   {
      if (fRunInfo) {
         delete fRunInfo;
         fRunInfo = NULL;
      }
   }

   void CreateRun(int run_number, const char* file_name)
   {
      assert(fRunInfo == NULL);
      assert(fRunRun.size() == 0);
      
      fRunInfo = new TARunInfo(run_number, file_name);

      for (unsigned i=0; i<(*gModules).size(); i++)
         fRunRun.push_back((*gModules)[i]->NewRun(fRunInfo));
   }

   void BeginRun()
   {
      assert(fRunInfo != NULL);
      assert(fRunInfo->fOdb != NULL);
      for (unsigned i=0; i<fRunRun.size(); i++)
         fRunRun[i]->BeginRun(fRunInfo);
   }

   void EndRun()
   {
      assert(fRunInfo);
      
      for (unsigned i=0; i<fRunRun.size(); i++)
         fRunRun[i]->EndRun(fRunInfo);
   }

   void DeleteRun()
   {
      assert(fRunInfo);

      for (unsigned i=0; i<fRunRun.size(); i++) {
         delete fRunRun[i];
         fRunRun[i] = NULL;
      }

      fRunRun.clear();
      assert(fRunRun.size() == 0);

      delete fRunInfo;
      fRunInfo = NULL;
   }

   void AnalyzeSpecialEvent(TMEvent* event)
   {
      for (unsigned i=0; i<fRunRun.size(); i++)
         fRunRun[i]->AnalyzeSpecialEvent(fRunInfo, event);
   }

   void AnalyzeEvent(TMEvent* event, TAFlags* flags, TMWriterInterface *writer)
   {
      assert(fRunInfo != NULL);
      assert(fRunInfo->fOdb != NULL);
      
      TAFlowEvent* flow = NULL;
                  
      for (unsigned i=0; i<fRunRun.size(); i++) {
         flow = fRunRun[i]->Analyze(fRunInfo, event, flags, flow);
         if (*flags & TAFlag_SKIP)
            break;
      }
      
      if (*flags & TAFlag_WRITE)
         if (writer)
            TMWriteEvent(writer, event);
      
      if (flow)
         delete flow;
   }
};

#ifdef HAVE_MIDAS

#include "TMidasOnline.h"

#ifdef HAVE_ROOT
#include "TSystem.h"
#endif

class OnlineHandler: public TMHandlerInterface
{
public:
   RunHandler fRun;
   int fNumAnalyze;
   TMWriterInterface* fWriter;
   bool fQuit;

   OnlineHandler(int num_analyze, TMWriterInterface* writer) // ctor
   {
      fQuit = false;
      fNumAnalyze = num_analyze;
      fWriter = writer;
   }

   ~OnlineHandler() // dtor
   {
      fWriter = NULL;
   }

   void StartRun(int run_number)
   {
      fRun.CreateRun(run_number, NULL);
      fRun.fRunInfo->fOdb = TMidasOnline::instance();
      fRun.BeginRun();
   }

   void Transition(int transition, int run_number, int transition_time)
   {
      //printf("OnlineHandler::Transtion: transition %d, run %d, time %d\n", transition, run_number, transition_time);
      
      if (transition == TR_START) {
         if (fRun.fRunInfo) {
            fRun.EndRun();
            fRun.fRunInfo->fOdb = NULL;
            fRun.DeleteRun();
         }
         assert(fRun.fRunInfo == NULL);

         StartRun(run_number);
         printf("Begin run: %d\n", run_number);
      } else if (transition == TR_STOP) {
         fRun.EndRun();
         fRun.fRunInfo->fOdb = NULL;
         fRun.DeleteRun();
         printf("End of run %d\n", run_number);
      }
   }

   void Event(const void* data, int data_size)
   {
      //printf("OnlineHandler::Event: ptr %p, size %d\n", data, data_size);

      if (!fRun.fRunInfo) {
         StartRun(0); // start fake run for events outside of a run
      }

      TMEvent* event = new TMEvent(data, data_size);

      TAFlags flags = 0;
      
      fRun.AnalyzeEvent(event, &flags, fWriter);

      if (flags & TAFlag_QUIT)
         fQuit = true;

      if (fNumAnalyze > 0) {
         fNumAnalyze--;
         if (fNumAnalyze == 0)
            fQuit = true;
      }

      if (event) {
         delete event;
         event = NULL;
      }
   }
};

static int ProcessMidasOnline(const std::vector<std::string>& args, const char* hostname, const char* exptname, int num_analyze, TMWriterInterface* writer)
{
   TMidasOnline *midas = TMidasOnline::instance();

   int err = midas->connect(hostname, exptname, "rootana");
   if (err != 0) {
      fprintf(stderr,"Cannot connect to MIDAS, error %d\n", err);
      return -1;
   }

   OnlineHandler* h = new OnlineHandler(num_analyze, writer);

   midas->RegisterHandler(h);
   midas->registerTransitions();

   /* reqister event requests */

   midas->eventRequest("SYSTEM",-1,-1,(1<<1));

   int run_number = midas->odbReadInt("/runinfo/Run number");
   int run_state  = midas->odbReadInt("/runinfo/State");

   for (unsigned i=0; i<(*gModules).size(); i++)
      (*gModules)[i]->Init(args);

   if ((run_state == STATE_RUNNING)||(run_state == STATE_PAUSED)) {
      h->StartRun(run_number);
   }

   while (!h->fQuit) {
#ifdef HAVE_THTTP_SERVER
      if (TARootHelper::fgHttpServer) {
         TARootHelper::fgHttpServer->ProcessRequests();
      }
#endif
#ifdef HAVE_ROOT
      if (TARootHelper::fgApp) {
         gSystem->DispatchOneEvent(kTRUE);
      }
#endif
      if (!TMidasOnline::instance()->poll(10))
         break;
   }

   if (h->fRun.fRunInfo) {
      h->fRun.EndRun();
      h->fRun.fRunInfo->fOdb = NULL;
      h->fRun.DeleteRun();
   }

   for (unsigned i=0; i<(*gModules).size(); i++)
      (*gModules)[i]->Finish();
   
   /* disconnect from experiment */
   midas->disconnect();

   return 0;
}

#endif

static int ProcessMidasFiles(const std::vector<std::string>& files, const std::vector<std::string>& args, int num_skip, int num_analyze, TMWriterInterface* writer)
{
   for (unsigned i=0; i<(*gModules).size(); i++)
      (*gModules)[i]->Init(args);

   RunHandler run;

   bool done = false;

   for (unsigned i=0; i<files.size(); i++) {
      std::string filename = files[i];

      TMReaderInterface *reader = TMNewReader(filename.c_str());

      if (reader->fError) {
         printf("Could not open \"%s\", error: %s\n", filename.c_str(), reader->fErrorString.c_str());
         delete reader;
         continue;
      }

      while (1) {
         TMEvent* event = TMReadEvent(reader);

         if (!event) // EOF
            break;

         if (event->error) {
            delete event;
            break;
         }

         if (event->event_id == 0x8000) // begin of run event
            {
               int runno = event->serial_number;

               if (run.fRunInfo) {
                  if (run.fRunInfo->fRunNo == runno) {
                     // next subrun file, nothing to do
                     run.fRunInfo->fFileName = filename;
                  } else {
                     // file with a different run number
                     run.EndRun();
                     run.DeleteRun();
                  }
               }

               if (!run.fRunInfo) {
                  run.CreateRun(runno, filename.c_str());
#ifdef HAVE_ROOT_XML
                  run.fRunInfo->fOdb = new XmlOdb(event->GetEventData(), event->data_size);
#else
                  run.fRunInfo->fOdb = new EmptyOdb();
#endif
                  run.BeginRun();
               }

               assert(run.fRunInfo);

               run.AnalyzeSpecialEvent(event);

               if (writer)
                  TMWriteEvent(writer, event);
            }
         else if (event->event_id == 0x8001) // end of run event
            {
               //int runno = event->serial_number;
               run.AnalyzeSpecialEvent(event);
               if (writer)
                  TMWriteEvent(writer, event);

               if (run.fRunInfo->fOdb) {
                  delete run.fRunInfo->fOdb;
                  run.fRunInfo->fOdb = NULL;
               }
               
#ifdef HAVE_ROOT_XML
               run.fRunInfo->fOdb = new XmlOdb(event->GetEventData(), event->data_size);
#else
               run.fRunInfo->fOdb = new EmptyOdb();
#endif
            }
         else if (event->event_id == 0x8002) // message event
            {
               run.AnalyzeSpecialEvent(event);
               if (writer)
                  TMWriteEvent(writer, event);
            }
         else
            {
               if (!run.fRunInfo) {
                  // create a fake begin of run
                  run.CreateRun(0, filename.c_str());
                  run.fRunInfo->fOdb = new EmptyOdb();
                  run.BeginRun();
               }

               if (num_skip > 0) {
                  num_skip--;
               } else {
                  TAFlags flags = 0;

                  run.AnalyzeEvent(event, &flags, writer);

                  if (flags & TAFlag_QUIT)
                     done = true;

                  if (num_analyze > 0) {
                     num_analyze--;
                     if (num_analyze == 0)
                        done = true;
                  }
               }
            }

         delete event;

         if (done)
            break;

#ifdef HAVE_ROOT
         if (TARootHelper::fgApp) {
            gSystem->DispatchOneEvent(kTRUE);
         }
#endif
      }

      reader->Close();
      delete reader;

      if (done)
         break;
   }

   if (run.fRunInfo) {
      run.EndRun();
      run.DeleteRun();
   }
   
   for (unsigned i=0; i<(*gModules).size(); i++)
      (*gModules)[i]->Finish();
   
   return 0;
}

static bool gEnableShowMem = false;

#if 0
static int ShowMem(const char* label)
{
  if (!gEnableShowMem)
    return 0;

  FILE* fp = fopen("/proc/self/statm","r");
  if (!fp)
    return 0;

  int mem = 0;
  fscanf(fp,"%d",&mem);
  fclose(fp);

  if (label)
    printf("memory at %s is %d\n", label, mem);

  return mem;
}
#endif

class EventDumpRun: public TARunInterface
{
public:
   EventDumpRun(TARunInfo* runinfo)
      : TARunInterface(runinfo)
   {
      printf("EventDumpRun::ctor, run %d\n", runinfo->fRunNo);
   }
   
   ~EventDumpRun()
   {
      printf("EventDumpRun::dtor!\n");
   }

   void BeginRun(TARunInfo* runinfo)
   {
      printf("EventDumpRun::BeginRun, run %d\n", runinfo->fRunNo);
   }

   void EndRun(TARunInfo* runinfo)
   {
      printf("EventDumpRun::EndRun, run %d\n", runinfo->fRunNo);
   }

   void PauseRun(TARunInfo* runinfo)
   {
      printf("EventDumpRun::PauseRun, run %d\n", runinfo->fRunNo);
   }

   void ResumeRun(TARunInfo* runinfo)
   {
      printf("EventDumpRun::ResumeRun, run %d\n", runinfo->fRunNo);
   }

   TAFlowEvent* Analyze(TARunInfo* runinfo, TMEvent* event, TAFlags* flags, TAFlowEvent* flow)
   {
      printf("EventDumpRun::Analyze, run %d, ", runinfo->fRunNo);
      event->FindAllBanks();
      std::string h = event->HeaderToString();
      std::string b = event->BankListToString();
      printf("%s: %s\n", h.c_str(), b.c_str());
      return flow;
   }

   void AnalyzeSpecialEvent(TARunInfo* runinfo, TMEvent* event)
   {
      printf("EventDumpRun::AnalyzeSpecialEvent, run %d, event serno %d, id 0x%04x, data size %d\n", runinfo->fRunNo, event->serial_number, (int)event->event_id, event->data_size);
   }
};

class EventDumpModule: public TAModuleInterface
{
public:

   void Init(const std::vector<std::string> &args)
   {
      printf("EventDumpModule::Init!\n");
   }
   
   void Finish()
   {
      printf("EventDumpModule::Finish!\n");
   }
   
   TARunInterface* NewRun(TARunInfo* runinfo)
   {
      printf("EventDumpModule::NewRun, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
      return new EventDumpRun(runinfo);
   }
};

#ifdef HAVE_ROOT
#include <TGMenu.h>
#include <TGButton.h>

#define CTRL_QUIT 1
#define CTRL_NEXT 2
#define CTRL_CONTINUE 3
#define CTRL_PAUSE    4

class XCtrl
{
public:
   int fValue;

   XCtrl() // ctor
   {
      fValue = 0;
   }
};

class XButton: public TGTextButton
{
public:
   XCtrl* fCtrl;
   int    fValue;
   
   XButton(TGWindow*p, const char* text, XCtrl* ctrl, int value) // ctor
      : TGTextButton(p, text)
   {
      fCtrl = ctrl;
      fValue = value;
   }

#if 0
   void Pressed()
   {
      printf("Pressed!\n");
   }
   
   void Released()
   {
      printf("Released!\n");
   }
#endif
   
   void Clicked()
   {
      printf("Clicked button %s, value %d!\n", GetString().Data(), fValue);
      if (fCtrl)
         fCtrl->fValue = fValue;
      gSystem->ExitLoop();
   }
};

class MainWindow: public TGMainFrame
{
private:
   TGPopupMenu*		fMenu;
   TGMenuBar*		fMenuBar;
   TGLayoutHints*	menuBarLayout;
   TGLayoutHints*	menuBarItemLayout;

public:
   XCtrl* fCtrl;
  
public:
   MainWindow(const TGWindow*w, int s1, int s2, XCtrl* ctrl) // ctor
      : TGMainFrame(w, s1, s2)
   {
      fCtrl = ctrl;
      //SetCleanup(kDeepCleanup);
   
      SetWindowName("ROOT Analyzer Control");

      // layout the gui
      fMenu = new TGPopupMenu(gClient->GetRoot());
      fMenu->AddEntry("Next",     CTRL_NEXT);
      fMenu->AddEntry("Continue", CTRL_CONTINUE);
      fMenu->AddEntry("Pause",    CTRL_PAUSE);
      fMenu->AddEntry("Quit",     CTRL_QUIT);

      menuBarItemLayout = new TGLayoutHints(kLHintsTop|kLHintsLeft, 0, 4, 0, 0);

      fMenu->Associate(this);

      fMenuBar = new TGMenuBar(this, 1, 1, kRaisedFrame);
      fMenuBar->AddPopup("&Rootana", fMenu, menuBarItemLayout);
      fMenuBar->Layout();

      menuBarLayout = new TGLayoutHints(kLHintsTop|kLHintsLeft|kLHintsExpandX);
      AddFrame(fMenuBar, menuBarLayout);

      // Create a container frames containing buttons

      // one button is resized up to the parent width.
      // Note! this width should be fixed!
      TGVerticalFrame *hframe1 = new TGVerticalFrame(this, 170, 50, kFixedWidth);

      // to take whole space we need to use kLHintsExpandX layout hints
      hframe1->AddFrame(new XButton(hframe1, "&Next ", ctrl, CTRL_NEXT),
                        new TGLayoutHints(kLHintsTop | kLHintsExpandX, 2, 0, 2, 2));
      AddFrame(hframe1, new TGLayoutHints(kLHintsCenterX, 2, 2, 5, 1));

      // two buttons are resized up to the parent width.
      // Note! this width should be fixed!
      TGCompositeFrame *cframe1 = new TGCompositeFrame(this, 170, 20, kHorizontalFrame | kFixedWidth);

      // to share whole parent space we need to use kLHintsExpandX layout hints
      cframe1->AddFrame(new XButton(cframe1, "&Continue", ctrl, CTRL_CONTINUE),
                        new TGLayoutHints(kLHintsTop | kLHintsExpandX, 2, 2, 2, 2));

      cframe1->AddFrame(new XButton(cframe1, "&Pause", ctrl, CTRL_PAUSE),
                        new TGLayoutHints(kLHintsTop | kLHintsExpandX, 2, 2, 2, 2));

      AddFrame(cframe1, new TGLayoutHints(kLHintsCenterX, 2, 2, 5, 1));

      // three buttons are resized up to the parent width.
      // Note! this width should be fixed!
      TGCompositeFrame *cframe2 = new TGCompositeFrame(this, 170, 20, kHorizontalFrame | kFixedWidth);

#if 0
      TGButton* ok = new XButton(cframe2, "OK", ctrl, 0);
      // to share whole parent space we need to use kLHintsExpandX layout hints
      cframe2->AddFrame(ok, new TGLayoutHints(kLHintsTop | kLHintsExpandX, 3, 2, 2, 2));

      TGButton* cancel = new XButton(cframe2, "Cancel ", ctrl, 0);
      cframe2->AddFrame(cancel, new TGLayoutHints(kLHintsTop | kLHintsExpandX, 3, 2, 2, 2));
#endif

      cframe2->AddFrame(new XButton(cframe2, "&Quit ", ctrl, CTRL_QUIT),
                        new TGLayoutHints(kLHintsTop | kLHintsExpandX, 2, 0, 2, 2));
      
      AddFrame(cframe2, new TGLayoutHints(kLHintsCenterX, 2, 2, 5, 1));
   
      MapSubwindows(); 
      Layout();
      MapWindow();
   }

   ~MainWindow() // dtor // Closing the control window closes the whole program
   {
      printf("MainWindow::dtor!\n");
      delete fMenu;
      delete fMenuBar;
      delete menuBarLayout;
      delete menuBarItemLayout;
   }

   void CloseWindow()
   {
      printf("MainWindow::CloseWindow()\n");
      if (fCtrl)
         fCtrl->fValue = CTRL_QUIT;
      gSystem->ExitLoop();
   }
  
   Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t parm2)
   {
      printf("GUI Message %d %d %d\n",(int)msg,(int)parm1,(int)parm2);
      switch (GET_MSG(msg))
         {
         default:
            break;
         case kC_COMMAND:
            switch (GET_SUBMSG(msg))
               {
               default:
                  break;
               case kCM_MENU:
                  printf("parm1 %d\n", (int)parm1);
                  switch (parm1)
                     {
                     default:
                        printf("Control %d!\n", (int)parm1);
                        if (fCtrl)
                           fCtrl->fValue = parm1;
                        gSystem->ExitLoop();
                        break;
                     }
                  break;
               }
            break;
         }

      return kTRUE;
   }
};
#endif

class InteractiveRun: public TARunInterface
{
public:
   bool fContinue;
   int  fSkip;
#ifdef HAVE_ROOT
   static XCtrl* fgCtrl;
   static MainWindow *fgCtrlWindow;
#endif
   
   InteractiveRun(TARunInfo* runinfo)
      : TARunInterface(runinfo)
   {
      printf("InteractiveRun::ctor, run %d\n", runinfo->fRunNo);
      fContinue = false;
      fSkip = 0;
#ifdef HAVE_ROOT
      if (!fgCtrl)
         fgCtrl = new XCtrl;
      if (!fgCtrlWindow && runinfo->fRoot->fgApp) {
         fgCtrlWindow = new MainWindow(gClient->GetRoot(), 200, 300, fgCtrl);
      }
#endif
   }
   
   ~InteractiveRun()
   {
      printf("InteractiveRun::dtor!\n");
   }

   void BeginRun(TARunInfo* runinfo)
   {
      printf("InteractiveRun::BeginRun, run %d\n", runinfo->fRunNo);
   }

   void EndRun(TARunInfo* runinfo)
   {
      printf("InteractiveRun::EndRun, run %d\n", runinfo->fRunNo);
   }

   void PauseRun(TARunInfo* runinfo)
   {
      printf("InteractiveRun::PauseRun, run %d\n", runinfo->fRunNo);
   }

   void ResumeRun(TARunInfo* runinfo)
   {
      printf("InteractiveRun::ResumeRun, run %d\n", runinfo->fRunNo);
   }

   TAFlowEvent* Analyze(TARunInfo* runinfo, TMEvent* event, TAFlags* flags, TAFlowEvent* flow)
   {
      printf("InteractiveRun::Analyze, run %d, %s\n", runinfo->fRunNo, event->HeaderToString().c_str());

#ifdef HAVE_ROOT
      if (fgCtrl->fValue == CTRL_QUIT) {
         *flags |= TAFlag_QUIT;
         return flow;
      } else if (fgCtrl->fValue == CTRL_PAUSE) {
         fContinue = false;
      }
#endif

      if (fContinue && !(*flags & TAFlag_DISPLAY)) {
         return flow;
      } else {
         fContinue = false;
      }

      if (fSkip > 0) {
         fSkip--;
         return flow;
      }

#ifdef HAVE_ROOT
      if (fgCtrlWindow && runinfo->fRoot->fgApp) {
         while (1) {
#ifdef HAVE_THTTP_SERVER
            if (TARootHelper::fgHttpServer) {
               TARootHelper::fgHttpServer->ProcessRequests();
            }
#endif
#ifdef HAVE_ROOT
            if (TARootHelper::fgApp) {
               gSystem->DispatchOneEvent(kTRUE);
            }
#endif
            if (!TMidasOnline::instance()->sleep(10)) {
               *flags |= TAFlag_QUIT;
               return flow;
            }

            int ctrl = fgCtrl->fValue;
            fgCtrl->fValue = 0;

            switch (ctrl) {
            case CTRL_QUIT:
               *flags |= TAFlag_QUIT;
               return flow;
            case CTRL_NEXT:
               return flow;
            case CTRL_CONTINUE:
               fContinue = true;
               return flow;
            }
         }
      }
#endif

      while (1) {
         char str[256];
         fprintf(stdout, "manalyzer> "); fflush(stdout);
         fgets(str, sizeof(str)-1, stdin);
         
         printf("command [%s]\n", str);

         if (str[0] == 'h') { // "help"
            printf("Interactive manalyzer commands:\n");
            printf(" q - quit\n");
            printf(" h - help\n");
            printf(" c - continue until next TAFlag_DISPLAY event\n");
            printf(" n - next event\n");
            printf(" aNNN - analyze N events, i.e. \"a10\"\n");
         } else if (str[0] == 'q') { // "quit"
            *flags |= TAFlag_QUIT;
            return flow;
         } else if (str[0] == 'n') { // "next"
            return flow;
         } else if (str[0] == 'c') { // "continue"
            fContinue = true;
            return flow;
         } else if (str[0] == 'a') { // "analyze" N events
            int num = atoi(str+1);
            printf("Analyzing %d events\n", num);
            if (num > 0) {
               fSkip = num-1;
            }
            return flow;
         }
      }

      return flow;
   }

   void AnalyzeSpecialEvent(TARunInfo* runinfo, TMEvent* event)
   {
      printf("InteractiveRun::AnalyzeSpecialEvent, run %d, event serno %d, id 0x%04x, data size %d\n", runinfo->fRunNo, event->serial_number, (int)event->event_id, event->data_size);
   }
};

MainWindow* InteractiveRun::fgCtrlWindow = NULL;
XCtrl* InteractiveRun::fgCtrl = NULL;

class InteractiveModule: public TAModuleInterface
{
public:

   void Init(const std::vector<std::string> &args)
   {
      printf("InteractiveModule::Init!\n");
   }
   
   void Finish()
   {
      printf("InteractiveModule::Finish!\n");
   }
   
   TARunInterface* NewRun(TARunInfo* runinfo)
   {
      printf("InteractiveModule::NewRun, run %d, file %s\n", runinfo->fRunNo, runinfo->fFileName.c_str());
      return new InteractiveRun(runinfo);
   }
};

static void help()
{
  printf("\nUsage:\n");
  printf("\n./analyzer.exe [-h] [-R8081] [-oOutputfile.mid] [file1 file2 ...] [-- arguments passed to modules ...]\n");
  printf("\n");
  printf("\t-h: print this help message\n");
  printf("\t-Hhostname: connect to MIDAS experiment on given host\n");
  printf("\t-Eexptname: connect to this MIDAS experiment\n");
  printf("\t-oOutputfile.mid: write selected events into this file\n");
  printf("\t-Rnnnn: Start the ROOT THttpServer HTTP server on specified tcp port, access by firefox http://localhost:8081\n");
  printf("\t-Xnnnn: Start the Xml server on specified tcp port (for use with roody -Xlocalhost:9091)\n");
  printf("\t-Pnnnn: Start the TNetDirectory server on specified tcp port (for use with roody -Plocalhost:9091)\n");
  printf("\t-eNNN: Number of events to analyze\n");
  printf("\t-sNNN: Number of events to skip before starting analysis\n");
  printf("\t--dump: activate the event dump module\n");
  printf("\t-m: Enable memory leak debugging\n");
  printf("\t-g: Enable graphics display when processing data files\n");
  printf("\t-i: Enable intractive mode\n");
  printf("\t--: All following arguments are passed to the analyzer modules Init() method\n");
  printf("\n");
  printf("Example1: analyze online data: ./analyzer.exe -P9091\n");
  printf("Example2: analyze existing data: ./analyzer.exe /data/alpha/current/run00500.mid\n");
  exit(1);
}

// Main function call

int manalyzer_main(int argc, char *argv[])
{
   setbuf(stdout, NULL);
   setbuf(stderr, NULL);
 
   signal(SIGILL,  SIG_DFL);
   signal(SIGBUS,  SIG_DFL);
   signal(SIGSEGV, SIG_DFL);
   signal(SIGPIPE, SIG_DFL);
   
   std::vector<std::string> args;
   for (int i=0; i<argc; i++) {
      if (strcmp(argv[i],"-h")==0)
         help(); // does not return
      args.push_back(argv[i]);
   }

   int  tcpPort = 0;
   int  xmlTcpPort = 0;
   int  httpPort = 0;
   const char* hostname = NULL;
   const char* exptname = NULL;

   int num_skip = 0;
   int num_analyze = 0;

   TMWriterInterface *writer = NULL;

   bool event_dump = false;
   bool root_graphics = false;
   bool interactive = false;

   std::vector<std::string> files;
   std::vector<std::string> modargs;

   for (unsigned int i=1; i<args.size(); i++) { // loop over the commandline options
      const char* arg = args[i].c_str();
      //printf("argv[%d] is %s\n",i,arg);

      if (args[i] == "--") {
         for (unsigned j=i+1; j<args.size(); j++)
            modargs.push_back(args[j]);
         break;
      } else if (args[i] == "--dump") {
         event_dump = true;
      } else if (args[i] == "-g") {
         root_graphics = true;
      } else if (args[i] == "-i") {
         interactive = true;
      } else if (strncmp(arg,"-o",2)==0) {
         writer = TMNewWriter(arg+2);
      } else if (strncmp(arg,"-s",2)==0) {
         num_skip = atoi(arg+2);
      } else if (strncmp(arg,"-e",2)==0) {
         num_analyze = atoi(arg+2);
      } else if (strncmp(arg,"-m",2)==0) { // Enable memory debugging
         gEnableShowMem = true;
      } else if (strncmp(arg,"-P",2)==0) { // Set the histogram server port
         tcpPort = atoi(arg+2);
      } else if (strncmp(arg,"-X",2)==0) { // Set the histogram server port
         xmlTcpPort = atoi(arg+2);
      } else if (strncmp(arg,"-R",2)==0) { // Set the ROOT THttpServer HTTP port
         httpPort = atoi(arg+2);
      } else if (strncmp(arg,"-H",2)==0) {
         hostname = strdup(arg+2);
      } else if (strncmp(arg,"-E",2)==0) {
         exptname = strdup(arg+2);
      } else if (strcmp(arg,"-h")==0) {
         help(); // does not return
      } else if (arg[0] == '-') {
         help(); // does not return
      } else {
         files.push_back(args[i]);
      }
   }

   if (!gModules)
      gModules = new std::vector<TAModuleInterface*>;

   if ((*gModules).size() == 0)
      event_dump = true;

   if (event_dump)
      (*gModules).push_back(new EventDumpModule);

   if (interactive)
      (*gModules).push_back(new InteractiveModule);

   printf("Registered modules: %d\n", (int)(*gModules).size());

#ifdef HAVE_ROOT
   if (root_graphics) {
      TARootHelper::fgApp = new TApplication("manalyzer", NULL, NULL, 0, 0);
   }

   TARootHelper::fgDir = new TDirectory("manalyzer", "location of histograms");
   TARootHelper::fgDir->cd();
#endif

#ifdef XHAVE_LIBNETDIRECTORY
   if (tcpPort) {
      VerboseNetDirectoryServer(true);
      StartNetDirectoryServer(tcpPort, TARootHelper::fgDir);
   }
#else
   if (tcpPort)
      fprintf(stderr,"ERROR: No support for the TNetDirectory server!\n");
#endif

#ifdef HAVE_XMLSERVER
   if (xmlTcpPort) {
      XmlServer* s = new XmlServer();
      s->SetVerbose(true);
      s->Start(xmlTcpPort);
      s->Export(gROOT, "ROOT");
      s->Export(TARootHelper::fgDir, "manalyzer");
      TARootHelper::fgXmlServer = s;
   }
#else
   if (xmlTcpPort)
      fprintf(stderr,"ERROR: No support for the XML Server!\n");
#endif
   
   if (httpPort) {
#ifdef HAVE_THTTP_SERVER
      char str[256];
      sprintf(str, "http:127.0.0.1:%d", httpPort);
      THttpServer *s = new THttpServer(str);
      //s->SetTimer(100, kFALSE);
      TARootHelper::fgHttpServer = s;
#else
      fprintf(stderr,"ERROR: No support for the THttpServer!\n");
#endif
   }
   
   for (unsigned i=0; i<files.size(); i++) {
      printf("file[%d]: %s\n", i, files[i].c_str());
   }

   if (files.size() > 0) {
      ProcessMidasFiles(files, modargs, num_skip, num_analyze, writer);
   } else {
#ifdef HAVE_MIDAS
      ProcessMidasOnline(modargs, hostname, exptname, num_analyze, writer);
#endif
   }

   if (writer) {
      writer->Close();
      delete writer;
      writer = NULL;
   }

   return 0;
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

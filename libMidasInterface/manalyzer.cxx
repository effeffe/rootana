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

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

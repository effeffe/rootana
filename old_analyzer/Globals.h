//
// Global variables for the ROOT analyzer
//
// Name: Globals.h
//
// $Id$
//

// Run parameters

extern int  gRunNumber;
extern bool gIsRunning;
extern bool gIsPedestalsRun;
extern bool gIsOffline;

// Output files

#ifdef HAVE_ROOT
extern TFile* gOutputFile;
extern TDirectory* gOnlineHistDir;
#endif

// ODB access

#include "mvodb.h"

extern MVOdb* gOdb;

// end


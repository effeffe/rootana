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

extern TFile* gOutputFile;
extern TDirectory* gOnlineHistDir;

// ODB access

#include "VirtualOdb.h"

extern VirtualOdb* gOdb;

// end


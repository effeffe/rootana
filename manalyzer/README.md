# MIDAS analyzer README

### Introduction

The new MIDAS analyzer was written with explicit goals in mind:

* preserve the idea of a modular analyzer (from the old MIDAS analyzer mana.c)
* preserve the idea of "data flow" (from the "flow analyzer")
* the same analysis code can be used online and offline, no dependancies on the MIDAS package
* the same analysis code can be used in batch mode or in interactive graphical mode
* better management of life time of ROOT objects (separation of module and run objects)

### Quick start

TBW - explain:

* copy example, copy Makefile, copy manalyzer_main.
* create histograms in the BeginRun() method
* save results in the EndRun() method
* process data and fill histograms in the Analyze() method
* extract midas data banks like this: TBW

### ROOT Web server

TBW - explain that all histograms are automatically exported to web browser

### manalyzer command line switches

TBW

* -g - enable graphical mode
* -i - enable interactive mode (see section XXX)
* -Rport - enable ROOT web server on given port (use port 8081), connect via "firefox http://localhost:8081"
* -Xport - enable XML Web server for roody (use port 9091), connect via "roody -H http://localhost:9091"
* -Pport - enable old TNetDirectory server for roody (use port 9091), connect via "roody -P localhost:9091"

### Concept of analyzer module

TBW (explain separation of module and run objects)

### The flow object

TBW (explain how to use the flow object to pass data between modules)

### Event analysis flags

* TAFlag_OK - all is good
* TAFlag_SKIP - tells manalyzer to skip processing of this event by subsequent modules (for implementing "filter" modules)
* TAFlag_QUIT - stop analyzing events and shutdown manalyzer (the normal end of run sequence is followed)
* TAFlag_WRITE - write this event to the output file stream
* TAFlag_DISPLAY - mark this event as "interesting" for the interactive display module

### manalyzer module and object life time

* analyzer start:
    - call module constructors
    - call module Init() methods

* run start:
    - call module NewRun() methods
    - call run constructors
    - call run BeginRun() methods

* if running from file:
    - call run AnalyzeSpecialEvent() methods for the ODB dump event (evid 0x8000)

* for each event:
    - call run Analyze() methods

* when switching from one subrun file to the next subrun file:
    - BeginRun()/EndRun() are not called
    - AnalyzeSpecialEvent() is called twice: once for the ODB dump event in the old subrun file (evid 0x8001) and once for the ODB dump event in the new subrun file (evid 0x8000)

* run end:
    - call run AnalyzeSpecialEvent() methods for the ODB dump event (evid 0x8001)
    - call run EndRun() methods
    - call run destructors

* analyzer shutdown:
    - definitely do the "run end" activity (all run objects destroyed)
    - call module Finish() methods
    - call module destructors
    - return from manalyzer_main()

### The "run info" class TARunInfo

The "run info" object has the information about the currently
analyzed run - run number, file name, etc.

This object is created when a new run is started (before calling the first BeginRun() method)
and is destroyed at the end of a run (after calling the last EndRun() method).

Ownership of this object remains with the analyzer framework. User code should not
keep a pointer to it or to any of it's components. (A pointer to this object
is passed to all user methods).

When analyzing multiple subrun files, no new runinfo objects are created, but the current
file name is always updated when switching from one subrun file to the next.

TARunInfo data members:

* fRunNo - is the current run number. Special run number 0 is used when processing online events when the midas run is stopped (no run).
* fFileName - current file name. (An empty string when processing online events)
* fOdb - pointer to a VirtualOdb object. When online, it is connected to the live online ODB. When processing data files, it is connected to the last seen ODB dump event (evid 0x8000 and 0x8001). If none available, it is connected to the special EmptyOdb object (all "odb get" methods return the default values).
* fRoot - pointer to the ROOT helper object (TARootHelper, see the next section). This member only exists if manalyzer is built with HAVE_ROOT.

### The ROOT helper class TARootHelper

ROOT-related functions provided by the older analyzers via global variables
are consolidated in the ROOT helper object with improved object lifetime management.

(If manalyzer is built without HAVE_ROOT, the ROOT helper object is not available).

The root helper object is created and destroyed at the same time as the "run info" object.

Ownership of the TARootHelper object remains with the manalyzer framework, user code
should not save pointers to it or any of it's components. A pointer to this object
is always available to all user methods via "runinfo->fRoot".

TARootHelper data members:

* TFile*        fOutputFile - ROOT output file. To save ROOT histograms into the output file, do "runinfo->fRoot->fOutputFile->cd()" before calling "new TH1D & co". This file is always present and always open for writing (no need to check for NULL pointer).
* TDirectory*   fgDir - ROOT in-memory directory. To avoid saving ROOT histograms in the output file, do "runinfo->fRoot->fgDir->cd()" before calling "new TH1D & co". This directory is always present (no need to check for NULL pointer). The contents of this directory are automatically exported to the ROOT web server.
* TApplication* fgApp - pointer to ROOT graphics environement, if running in graphical mode. Value is NULL if running in batch mode (no graphics).
* THttpServer*  fgHttpServer - pointer to the ROOT web server object (-R switch).
* XmlServer*    fgXmlServer - pointer to the old XML Web server for use with the roody application (-X switch).
* TNetDirectory server (-P switch) is also available (it has no corresponding c++ object).

### Module registration

TBW

### The main program

TBW

### The event dump module

TBW

### The interative display module

TBW

### XXX

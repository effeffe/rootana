# README #

Welcome to ROOTANA, the ROOT based data analysis package for the MIDAS Data Acquisition system.

### Introduction ###

The ROOTANA package includes 5 major components:
- a standalone library for reading and writing data files in the MIDAS .mid format, and for decoding the XML formatted dump of MIDAS ODB (usually the first and last events in every MIDAS data file) (libMidasInterface)
- a C++ interface class for connecting to an active MIDAS experiment, accessing ODB (read and write) and getting event data (libMidasInterface)
- a set of C++ classes for exporting ROOT histogram and other objects to an external viewer for interactive visualization of live data, typically using the ROODY histogram viewer or using a standard web browser (experimental feature). (libMidasServer, libNetDirectory, libXmlServer)
- simple examples of using these components (a graphical analyzer, an event dump and an event skim programs) (analyzer.cxx, event_dump.cxx, event_skim.cxx)
- a full featured framework for graphical data analysis including code to unpack typical VME and CAMAC modules (examples, libAnalyzer, linAnalyzerDisplay)

The ROOTANA package can be used without installing ROOT and MIDAS:

- without both ROOT and MIDAS one can only read (and write) existing .mid files and access the embedded XML ODB dumps. Only event_dump and event_skim will be built.

- if only the MIDAS package is installed, access to live data and access to live ODB becomes possible. Only event_dum and event_skim will be built.

- if the ROOT package is installed but MIDAS is absent, full function of ROOTANA is available, except for access to live data. This mode is suitable for offline data analysis, i.e. the user has a copy of MIDAS data files on their laptop and wants to analyze them.

### How do I get set up? ###

* Summary of set up
* Configuration
* Dependencies
* Database configuration
* How to run tests
* Deployment instructions

### Contribution guidelines ###

* Writing tests
* Code review
* Other guidelines

### Who do I talk to? ###

* Repo owner or admin
* Other community or team contact

### Historical note by Konstantin Olchanski ###

The origins of this package date back a few years when I wrote some C++ classes to read MIDAS files for the Dragon experiment. Jonty Pearson and Joe Chuma have since improved and added to my work. Then during the Summer of 2006, I wrote some more C++ classes for access to live data and for access to the live ODB, for use by the ALPHA experiment at CERN. This code was then reused for couple of test DAQ stations at TRIUMF. With the addition of ROODY access using the "midas server", ripped out from mana.c, it is now used for the PIENU beam test.


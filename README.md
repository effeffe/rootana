# README #

Welcome to ROOTANA, the ROOT (http://root.cern.ch) based data analysis package for the MIDAS Data Acquisition system (http://midas.trimf.ca).

### Introduction ###

The ROOTANA package includes 5 major components:

* a standalone library for reading and writing data files in the MIDAS .mid format, and for decoding the XML dump of MIDAS ODB typically embedded in every MIDAS data file. (libMidasInterface)
* a C++ interface class for connecting to an active MIDAS experiment, accessing ODB (read and write) and getting event data. (libMidasInterface)
* a set of C++ classes for exporting ROOT histogram and other objects to an external viewer for interactive visualization of live data, typically using the ROODY histogram viewer or using a standard web browser (experimental feature). (libMidasServer, libNetDirectory, libXmlServer)
* simple examples of using these components (a graphical analyzer, an event dump and an event skim programs) (analyzer.cxx, event_dump.cxx, event_skim.cxx)
* a full featured framework for graphical data analysis including code to unpack typical VME and CAMAC modules (examples, libAnalyzer, linAnalyzerDisplay)

The ROOTANA package can be used without installing ROOT and MIDAS:

* without both ROOT and MIDAS one can only read (and write) existing .mid files. Only event_dump and event_skim will be built.
* if only the MIDAS package is installed, access to live data and access to live ODB becomes possible. Only event_dump and event_skim will be built.
* decoding of XML ODB dumps embedded in MIDAS files requires ROOT TXML and TDOMParser components (a version of XML decoder using libxml2 used to exist in the past).
* if the ROOT package is installed but MIDAS is absent, full function of ROOTANA is available, except for access to live data. This mode is suitable for offline data analysis, i.e. the user has a copy of MIDAS data files on their laptop and wants to analyze them.

### Quick start guide ###

* install ROOT (http://root.cern.ch), make sure ROOTSYS is defined ($ROOTSYS/include is accessible) or root-config is in the PATH
* (optional) install MIDAS (http://midas.triumf.ca), make sure MIDASSYS is defined ($MIDASSYS/include is accessible)
* git clone https://bitbucket.org/tmidas/rootana.git
* cd rootana
* make
* examine simple examples: more event_dump.cxx, more analyzer.cxx
* examine more complex examples: cd examples; make; more README.txt; more TAnaManager.cxx, etc

### Contacts ###

* Before conversion from svn to git, ROOTANA was managed by members of the TRIUMF DAQ group
* The git version hosted on bitbucket is managed by the members of the MIDAS developers group.
* to report bugs, request improvements, contribute bug fixes - please go to the ROOTANA issue tracker https://bitbucket.org/tmidas/rootana/issues
* for questions and discussion - please go to the MIDAS discussion forum https://midas.triumf.ca/forum

### Historical note by Konstantin Olchanski ###

The origins of this package date back a few years when I wrote some C++ classes to read MIDAS files for the Dragon experiment. Jonty Pearson and Joe Chuma have since improved and added to my work. Then during the Summer of 2006, I wrote some more C++ classes for access to live data and for access to the live ODB, for use by the ALPHA experiment at CERN. This code was then reused for couple of test DAQ stations at TRIUMF. With the addition of ROODY access using the "midas server", ripped out from mana.c, it is now used for the PIENU beam test.

### Historical change log ###

(UPDATE 26-MAR-2007) The code was tested on 32-bit and 64-bit Linux/x86 and on 32-bit MacOS/PPC. The 64-bit compilation issues have been mostly cleaned up. MacosX/x86 not tested yet.

(UPDATE 01-JUL-2007) Added ability to read gzipped midas files (using zlib).

(UPDATE 16-AUG-2007) Added ODB access functions odbReadArraySize() and odbReadDouble(). Fully implemented the XmlOdb interface (Except for odbReadAny()).

(UPDATE 02-OCT-2007) Tested with ROOT version 5.16. Implemented enough TNetDirectory code to replace the midasServer code for serving data to ROODY.

(UPDATE 04-MAR-2008) Implemented option for polling midas events, updated documentation.

(UPDATE 06-OCT-2008) Implemented pipes for reading remote midas files through ssh and dcache tunnels.

(UPDATE 21-DEC-2008) Implemented HttpOdb to access ODB through MIDAS HTTP server mhttpd


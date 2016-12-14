# README #

Welcome to ROOTANA, the ROOT (https://root.cern.ch) based data analysis package for the MIDAS Data Acquisition system (https://midas.triumf.ca).

### Introduction ###

The ROOTANA package includes 4 major components:

* a C++ library for reading and writing data files in the MIDAS .mid format, for connecting to live MIDAS experiments, and for accessing ODB data (live or from ODB dumps in the data files) [libMidasInterface](libMidasInterface/)
* the old ROOT analyzer [old_analyzer](old_analyzer/)
* the new MIDAS analyzer [manalyzer](manalyzer/)
* an interactive graphical analyzer, see [examples](examples/), [libAnalyzer](libAnalyzer/), [libAnalyzerDisplay](libAnalyzerDisplay/)

Other notable features include:

* reading compressed MIDAS files using compiled-in libraries for GZIP and LZ4 and using external decompressor for BZIP2 and PBZIP2.
* reading MIDAS events from remote MIDAS files through ssh and dcache pipes, see TMNewReader() and TMidasFile::Open()
* access to live MIDAS events from local MIDAS shared memory or from remote MIDAS system via the MIDAS mserver.
* code for unpacking typical VME and CAMAC modules
* (largely obsoleted by ROOT's THttpServer) a set of C++ classes for exporting ROOT histogram and other objects to an external viewer for interactive visualization of live data, typically using the ROODY histogram viewer or using a standard web browser (experimental feature). (libMidasServer, libNetDirectory, libXmlServer)
* simple examples of using these components (a graphical analyzer, an event dump and an event skim programs) (analyzer.cxx, event_dump.cxx, event_skim.cxx)

The ROOTANA package can be used without installing ROOT and MIDAS:

* without both ROOT and MIDAS one can only read (and write) existing midas files using pure C++ code. Only event_dump and event_skim will be built.
* if only the MIDAS package is installed, access to live data and access to live ODB becomes possible. Only event_dump and event_skim will be built.
* decoding of XML ODB dumps embedded in MIDAS files requires ROOT TXML and TDOMParser components (a version of XML decoder using libxml2 used to exist in the past).
* if the ROOT package is installed but MIDAS is absent, full function of ROOTANA is available, except for access to live data. This mode is suitable for offline data analysis, i.e. the user has a copy of MIDAS data files on their laptop and wants to analyze them.

Access to optional components is controlled by these Makefile and C++ symbols:

* HAVE_MIDAS - defined if MIDAS is installed ($MIDASSYS is set)
* HAVE_ROOT - if ROOT is installed (root-config is in the $PATH)
* HAVE_ROOT_XML - ROOT XML component is installed ("xml" in root-config --features)
* HAVE_ROOT_HTTP - ROOT HTTP component is installed ("http" in root-config --features)
* (gzip/zlib is now required, no longer optional!) HAVE_LIBZ - if the gzip/zlib library is installed (required for reading mid.gz files)

### Quick start guide ###

* install ROOT (https://root.cern.ch), per ROOT documentation, run "thisroot" to define ROOTSYS and to add root-config, root and rootcint to the PATH
* (optional) install MIDAS (https://midas.triumf.ca), make sure MIDASSYS is defined ($MIDASSYS/include is accessible)
* git clone https://bitbucket.org/tmidas/rootana.git
* cd rootana
* make
* examine simple examples: more event_dump.cxx, more analyzer.cxx
* examine more complex examples: cd examples; make; more README.txt; more TAnaManager.cxx, etc
* if desired, run "make dox" to generate the Doxygen code reference documentation for the ROOTANA. This may take some time, at the end, open html/index.html.

More documentation [Analyzer Framework](http://ladd00.triumf.ca/~lindner/rootana/html/analyzerClass.html) and [Display Framework](http://ladd00.triumf.ca/~lindner/rootana/html/displayClass.html)

### Contacts ###

* to report bugs, request improvements, contribute bug fixes - please go to the ROOTANA issue tracker https://bitbucket.org/tmidas/rootana/issues
* for questions and discussion - please go to the MIDAS discussion forum https://midas.triumf.ca/forum
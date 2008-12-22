/// $Id$
///
/// \mainpage
///
/// \section intro_sec Introduction
///
/// This "ROOT analyzer" package is a collection of C++ classes to
/// simplify online and offline analysis of data
/// collected using the MIDAS data acquisition system.
///
/// To permit standalone data analysis in mobile and "home institution"
/// environments, this package does not generally require that MIDAS
/// itself be present or installed.
///
/// It is envisioned that the user will use this package to develop
/// their experiment specific analyzer using the online data
/// connection to a MIDAS experiment. Then they could copy all the code
/// and data (.mid files) to their laptop and continue further analysis
/// without depending on or requiring installation of MIDAS software.
///
/// It is assumed that data will be analyzed using the ROOT
/// toolkit. However, to permit the most wide use of this
/// package, most base classes do not use or require ROOT.
///
/// \section features_sec Features
///
/// - C++ classes for reading MIDAS events from .mid files, including compressed midas files and remote midas files through ssh and dcache pipes, see TMidasFile::Open()
/// - C++ classes for reading MIDAS events from a running
/// MIDAS experiment via the mserver or directly from the MIDAS
/// shared memory (this requires linking with MIDAS libraries) (see TMidasOnline, analyzer.cxx and event_dump.cxx)
/// - C++ classes for accessing XML ODB data from .mid files (see XmlOdb)
/// - C++ classes for accessing ODB through HTTP/mhttpd (see HttpOdb)
/// - C++ classes for accessing ODB from MIDAS shared memory (see TMidasOnline,
///   requires linking with MIDAS libraries).
/// - an example event dump program
/// - an example C++ analyzer main program
/// - the example analyzer creates a graphical ROOT application permitting full
/// use of ROOT graphics in online and offline modes.
/// - for looking at "live" histograms using the ROODY graphical histogram viewer,
/// included is the "TNetDirectory" package for accessing remote ROOT objects.
///
/// \section links_sec Links to external packages
///
/// - ROOT data analysis toolkit: http://root.cern.ch
/// - MIDAS data acquisition system: http://midas.psi.ch
/// - ROODY graphical histogram viewer: http://daq-plone.triumf.ca/SR/ROODY/
///
/// \section docs_sec Documentation
///
/// - important classes: <a href="classTMidasEvent.html">MIDAS event</a>, <a href="classTMidasFile.html">MIDAS file reader</a>, <a href="classTMidasOnline.html">MIDAS online connection</a>, <a href="structVirtualOdb.html">access ODB data</a>
/// - <a href="analyzer_8cxx-source.html">example analyzer</a>
/// - <a href="annotated.html">all ROOTANA classes</a>
/// - <a href="files.html">all ROOTANA files</a>
///
/// \section starting_sec Getting started
///
/// - "get" the sources: svn checkout svn://ladd00.triumf.ca/rootana/trunk rootana
/// - cd rootana
/// - make
/// - make dox (generate this documentation); cd html; mozilla index.html
/// - edit analyzer.cxx, look at how different MIDAS events are handled in HandleMidasEvent() and how event data is passed to the example user function HandleSample(). Create your own HandleFoo() functions to handle your data.
/// - for more advanced analysis, use rootana as a library: in your own analysis directory, copy the example analyzer.cxx (rename it according to your tastes, popular choices are "alpharoot", "dragonroot", "fgdroot", etc), place the HandleFoo() functions into separate files HandleFoo{.h,.cxx}.
///  
/// \section config_sec Configuration
///
/// - env.variable ROOTSYS controls support for ROOT (also the C++ define -DHAVE_ROOT). Most rootana classes require ROOT, so if ROOTSYS is not defined, only code that works without ROOT will be compiled - TMidasEvent and TMidasFile at this moment.
/// - env.variable MIDASSYS controls support for connecting to live MIDAS data (also the C++ define -DHAVE_MIDAS). All of rootana is supposed to work without MIDAS, except for the TMidasOnline class.
/// - C++ define -DHAVE_LIBNETDIRECTORY enables use of the newer "libNetDirectory" interface for Roody. This replaces the "-DOLD_SERVER" functions that use the "MIDAS server" from MIDAS mana.c.
/// - C++ define "-DOLD_SERVER" enables the obsolete MIDAS server from MIDAS mana.c. Please use the newer libNetDirectory instead.
///
///
/// \section histo_sec Creating histograms, etc
///
/// Think of the example ROOT analyzer as a normal ROOT application where
/// you can do all the normal ROOT things to create histograms, plots, etc
///
/// However, one needs to be aware of two things traditionally done by MIDAS analyzers.
/// As can be seen by reading the example analyzer, it creates an output ROOT file
/// and it makes histograms, plots, etc available to other aplications
/// using a network socket connection (via the netDirectoryServer or midasServer). Neither
/// of this functions is required and the corresponding code can be safely removed.
///
/// - interaction with the output ROOT file: by default, for each run, the analyzer opens a new ROOT file outputNNNNN.root
/// and make it the current directory (gOutputFile) for newly created ROOT objects, i.e. those create by "new TH1(...)".
/// At the end of a run, all these objects
/// are saved into the file, the file is closed and all these objects disappear from memory. To create ROOT objects
/// that persist across runs, use some other ROOT directory (i.e. gOnlineHistDir->cd()).
///
/// - when using the netDirectoryServer (HAVE_LIBNETDIRECTORY), the contents of gOnlineHistDir and gOutputFile
/// are exported to outside applications. Other programs, i.e. ROODY, can use TNetDirectory to "see"
/// the histograms (and other objects) as they are filled. (Note: this does not work for most TTree objects
/// because they cannot be easily "exported").
///
/// \section graph_sec Creating interactive graphics, etc
///
/// Think of the example ROOT analyzer as a normal ROOT application where
/// you can use all the normal ROOT graphics operations - create new TCanvas objects,
/// draw on them, interact with them at will. This is possible because in the online mode,
/// the application is controlled by the ROOT TApplication->Run() method - MIDAS event processing
/// happens from TTimer events. In the offline mode, the example analyzer does not start
/// the ROOT event loop until the last file has been processed.
///
/// The example analyzer contains graphics code to create a new "main window" with an example
/// menu and provides an example ROOT event handler to execute menu commands and
/// to interact with the user.
///

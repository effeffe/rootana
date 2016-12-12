#
# Makefile for the ROOTANA package
#

CXXFLAGS = -g -O2 -Wall -Wuninitialized -I./include

# required/non-optional libz package for GZIP decompression

HAVE_LIBZ := 1
CXXFLAGS += -DHAVE_LIBZ

# optional ROOT libraries

ifndef NO_ROOT
ROOTVERSION := $(shell root-config --version)
endif

ifdef ROOTVERSION
HAVE_ROOT=1
ROOTFEATURES := $(shell root-config --features)
ROOTLIBDIR := $(shell root-config --libdir)
ROOTLIBS   := -L$(ROOTLIBDIR) $(shell root-config --libs) -lThread
ROOTGLIBS  := -L$(ROOTLIBDIR) $(shell root-config --glibs) -lThread
ROOTCFLAGS := $(shell root-config --cflags)
RPATH    += -Wl,-rpath,$(ROOTLIBDIR)
CXXFLAGS += -DHAVE_ROOT $(ROOTCFLAGS)
CXXFLAGS_ROOTCINT += -DHAVE_ROOT
HAVE_ROOT_HTTP := $(findstring http,$(ROOTFEATURES))
HAVE_ROOT_XML  := $(findstring xml,$(ROOTFEATURES))

ifdef HAVE_ROOT_XML
CXXFLAGS += -DHAVE_ROOT_XML
ROOTLIBS += -lXMLParser -lXMLIO
ROOTGLIBS += -lXMLParser -lXMLIO
endif

ifdef HAVE_ROOT_HTTP
CXXFLAGS += -DHAVE_ROOT_HTTP -DHAVE_THTTP_SERVER
ROOTLIBS += -lRHTTP
ROOTGLIBS += -lRHTTP
endif

#xhere: ; @echo Have ROOT: features: $(ROOTFEATURES), libdir: $(ROOTLIBDIR), libs: $(ROOTLIBS) cflags: $(CXXFLAGS)
else
#xnoroot: ; @echo ROOT not available, please run: make minimal
endif

# optional MIDAS libraries

ifdef MIDASSYS

HAVE_MIDAS=1
MIDASLIBS = $(MIDASSYS)/linux/lib/libmidas.a -lutil -lrt
CXXFLAGS += -DHAVE_MIDAS -DOS_LINUX -Dextname -I$(MIDASSYS)/include

UNAME=$(shell uname)
ifeq ($(UNAME),Darwin)
CXXFLAGS += -DOS_LINUX -DOS_DARWIN
CXXFLAGS_ROOTCINT += -DOS_LINUX -DOS_DARWIN
MIDASLIBS = $(MIDASSYS)/darwin/lib/libmidas.a
RPATH=
endif

endif

ifdef HAVE_ROOT
OBJS += obj/RootLock.o
endif

# optional TNetDirectory code

ifdef HAVE_ROOT
CXXFLAGS += -DHAVE_LIBNETDIRECTORY
OBJS     += obj/netDirectoryServer.o
OBJS     += obj/TNetDirectory.o
OBJS     += obj/TNetDirectoryDict.o
endif

# optional XmlServer code

ifdef HAVE_ROOT_XML
CXXFLAGS += -DHAVE_XMLSERVER
OBJS     += obj/xmlServer.o
endif

# optional libAnalyzer and libAnalyzerDisplay code
ifdef HAVE_ROOT

#CXXFLAGS += -DNO_CINT

ALL  += libAnalyzer/analyzer_example.exe
ALL  += libAnalyzerDisplay/display_example.exe

OBJS += obj/TRootanaEventLoop.o
OBJS += obj/TDataContainer.o
OBJS += obj/TPeriodicClass.o
OBJS += obj/TV792Data.o
OBJS += obj/TV1190Data.o
OBJS += obj/TV1190Data.o
OBJS += obj/TV1720RawData.o
OBJS += obj/TV1730DppData.o
OBJS += obj/TV1730RawData.o
OBJS += obj/TDT724RawData.o
OBJS += obj/TV792Data.o
OBJS += obj/TL2249Data.o
OBJS += obj/TMesytecData.o

OBJS += obj/TRootanaDisplay.o
OBJS += obj/TMainDisplayWindow.o
OBJS += obj/TRootanaDisplay.o
OBJS += obj/TRootanaDisplay.o
OBJS += obj/TSimpleExampleCanvas.o
OBJS += obj/TComplicatedExampleCanvas.o
OBJS += obj/TInterestingEventManager.o
OBJS += obj/TFancyHistogramCanvas.o

OBJS += obj/TMainDisplayWindowDict.o
OBJS += obj/TRootanaDisplayDict.o
OBJS += obj/TFancyHistogramCanvasDict.o

endif

ALL  += lib/librootana.a
ALL  += event_dump.o event_dump.exe
ALL  += event_skim.o event_skim.exe
ALL  += analyzer.o analyzer.exe
ALL  += manalyzer.exe
ALL  += obj/manalyzer_example1.o manalyzer_example1.exe
ifdef HAVE_ROOT
ALL  += obj/manalyzer_example2.o manalyzer_example2.exe
ALL  += obj/manalyzer_example3.o manalyzer_example3.exe
ALL  += tests/test_midasServer.o tests/test_midasServer.exe
ifdef HAVE_MIDAS
ALL  += tests/testODB.o tests/testODB.exe
endif
endif

# libMidasInterface

OBJS += obj/TMidasEvent.o
OBJS += obj/TMidasFile.o
ifdef HAVE_MIDAS
OBJS += obj/TMidasOnline.o
endif
ifdef HAVE_ROOT
OBJS += obj/HttpOdb.o
endif
ifdef HAVE_ROOT_XML
OBJS += obj/XmlOdb.o
endif
OBJS += obj/midasio.o
OBJS += obj/lz4.o
OBJS += obj/lz4hc.o
OBJS += obj/xxhash.o
OBJS += obj/lz4frame.o
OBJS += obj/midasio.o
OBJS += obj/manalyzer.o
OBJS += obj/manalyzer_main.o

all: $(ALL)

$(ALL): include
$(OBJS): include

#obj/midasio.o: include/midasio.h
#obj/manalyzer.o: include/manalyzer.h include/midasio.h include/VirtualOdb.h
#obj/manalyzer_main.o: include/manalyzer.h include/midasio.h include/VirtualOdb.h include/TMidasOnline.h

include:
	mkdir -p include lib obj
	cd include; ln -sfv ../lib*/*.h .
	cd include; ln -sfv ../lib*/*.hxx .

lib/librootana.a: $(OBJS)
	mkdir -p lib
	-rm -f $@
	ar -rv $@ $(OBJS)

#include/%.h: include
#	@true

%Dict.o: %Dict.cxx
	$(CXX) -o $@ $(CXXFLAGS) -c -I. $<

obj/TMainDisplayWindowDict.cxx obj/TRootanaDisplayDict.cxx obj/TFancyHistogramCanvasDict.cxx: obj/%Dict.cxx:
	rootcint -f $@ -c -p $(CXXFLAGS_ROOTCINT) -I./include include/$*.hxx include/$*_LinkDef.h

obj/TNetDirectoryDict.cxx: obj/%Dict.cxx:
	rootcint -f $@ -c -p $(CXXFLAGS_ROOTCINT) -I./include include/$*.h include/$*_LinkDef.h

%.exe: %.o lib/librootana.a
	$(CXX) -o $@ $(CXXFLAGS) $< lib/librootana.a $(MIDASLIBS) $(ROOTGLIBS) -lm -lz -lpthread $(RPATH)

%.exe: obj/%.o lib/librootana.a
	$(CXX) -o $@ $(CXXFLAGS) $< lib/librootana.a $(MIDASLIBS) $(ROOTGLIBS) -lm -lz -lpthread $(RPATH)

%.o: %.cxx
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/%.o: libMidasInterface/%.cxx
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/%.o: libMidasInterface/%.c
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/%.o: libNetDirectory/%.cxx
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/%.o: libXmlServer/%.cxx
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/%.o: libAnalyzer/%.cxx
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/%.o: libAnalyzerDisplay/%.cxx
	$(CXX) $(CXXFLAGS) -o $@ -c $<

html/index.html:
	-mkdir html
	-make -k dox
	touch html/index.html

dox: include
	doxygen

clean::
	-rm -f *.o *.a *.exe $(ALL)
	-rm -f */*Dict.cxx */*Dict.h */*Dict_rdict.pcm
	-rm -rf lib
	-rm -rf include
	-rm -rf obj

clean::
	-rm -rf */*.exe.dSYM

clean::
	-rm -rf html

# end

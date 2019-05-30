#
# Makefile for the ROOTANA package
#
# Options:
# make NO_MIDAS=1 # build without MIDAS support
# make NO_ROOT=1 # build without ROOT support
#

CXXFLAGS = -g -O2 -Wall -Wuninitialized -I./include

# required/non-optional libz package for GZIP decompression

HAVE_LIBZ := 1
CXXFLAGS  += -DHAVE_LIBZ
USER_LIBS += -lz

# optional ROOT libraries

ifndef NO_ROOT
ROOTVERSION := $(shell root-config --version)
endif

ifdef ROOTVERSION
HAVE_ROOT  := 1
ROOTFEATURES := $(shell root-config --features)
ROOTLIBDIR := $(shell root-config --libdir)
ROOTLIBS   := -L$(ROOTLIBDIR) $(shell root-config --libs) -lThread
ROOTGLIBS  := -L$(ROOTLIBDIR) $(shell root-config --glibs) -lThread
ROOTCFLAGS := $(shell root-config --cflags)
RPATH    += -Wl,-rpath,$(ROOTLIBDIR)
CXXFLAGS += -DHAVE_ROOT $(ROOTCFLAGS)
CXXFLAGS_ROOTCINT += -DHAVE_ROOT
USER_CFLAGS += $(ROOTCFLAGS)
USER_LIBS   += $(ROOTGLIBS)
HAVE_ROOT_HTTP := $(findstring http,$(ROOTFEATURES))
HAVE_ROOT_XML  := $(findstring xml,$(ROOTFEATURES))
HAVE_CXX11_THREADS := $(findstring cxx11,$(ROOTFEATURES))

ifdef HAVE_ROOT_XML
CXXFLAGS += -DHAVE_ROOT_XML
ROOTLIBS += -lXMLParser -lXMLIO
ROOTGLIBS += -lXMLParser -lXMLIO
USER_LIBS += -lXMLParser -lXMLIO
endif

ifdef HAVE_ROOT_HTTP
HAVE_THTTP_SERVER := 1
CXXFLAGS += -DHAVE_ROOT_HTTP -DHAVE_THTTP_SERVER
ROOTLIBS += -lRHTTP
ROOTGLIBS += -lRHTTP
USER_LIBS += -lRHTTP
endif

#xhere: ; @echo Have ROOT: features: $(ROOTFEATURES), libdir: $(ROOTLIBDIR), libs: $(ROOTLIBS) cflags: $(CXXFLAGS)
else
#xnoroot: ; @echo ROOT not available, please run: make minimal
endif

# optional MIDAS libraries

ifdef NO_MIDAS
MIDASSYS:=
endif

ifneq ($(MIDASSYS),)

HAVE_MIDAS=1
MIDASLIBS = $(MIDASSYS)/linux/lib/libmidas.a -lutil -lrt
CXXFLAGS += -DHAVE_MIDAS -DOS_LINUX -Dextname -I$(MIDASSYS)/include
USER_CFLAGS += -DOS_LINUX -Dextname -I$(MIDASSYS)/include

UNAME=$(shell uname)
ifeq ($(UNAME),Darwin)
CXXFLAGS += -DOS_LINUX -DOS_DARWIN
USER_CFLAGS += -DOS_LINUX -DOS_DARWIN
CXXFLAGS_ROOTCINT += -DOS_LINUX -DOS_DARWIN
MIDASLIBS = $(MIDASSYS)/darwin/lib/libmidas.a
RPATH=
endif

USER_LIBS += $(MIDASLIBS)

endif # MIDASSYS

ifdef HAVE_ROOT
OBJS += obj/RootLock.o
endif

# optional TNetDirectory code

ifdef HAVE_ROOT
HAVE_LIBNETDIRECTORY := 1
CXXFLAGS += -DHAVE_LIBNETDIRECTORY
OBJS     += obj/netDirectoryServer.o
OBJS     += obj/TNetDirectory.o
OBJS     += obj/TNetDirectoryDict.o
endif

# optional XmlServer code

ifdef HAVE_ROOT_XML
HAVE_XMLSERVER := 1
CXXFLAGS += -DHAVE_XMLSERVER
OBJS     += obj/xmlServer.o
endif

# optional libAnalyzer and libAnalyzerDisplay code
ifdef HAVE_ROOT

#CXXFLAGS += -DNO_CINT

ALL  += libAnalyzer/analyzer_example.exe
ALL  += libAnalyzerDisplay/display_example.exe

DALL += libAnalyzer/analyzer_example.o
DALL += libAnalyzerDisplay/display_example.o

OBJS += obj/TRootanaEventLoop.o
OBJS += obj/TDataContainer.o
OBJS += obj/TPeriodicClass.o
OBJS += obj/TV792Data.o
OBJS += obj/TV792NData.o
OBJS += obj/TV1190Data.o
OBJS += obj/TV1720RawData.o
OBJS += obj/TV1730DppData.o
OBJS += obj/TV1730RawData.o
OBJS += obj/TDT724RawData.o
OBJS += obj/TL2249Data.o
OBJS += obj/TMesytecData.o
OBJS += obj/TRB3Decoder.o
OBJS += obj/TTRB3Data.o
OBJS += obj/TCamacADCData.o

OBJS += obj/TRootanaDisplay.o
OBJS += obj/TMainDisplayWindow.o
OBJS += obj/THistogramArrayBase.o
OBJS += obj/TSimpleExampleCanvas.o
OBJS += obj/TComplicatedExampleCanvas.o
OBJS += obj/TInterestingEventManager.o
OBJS += obj/TSimpleHistogramCanvas.o
OBJS += obj/TFancyHistogramCanvas.o
OBJS += obj/TMulticanvas.o

OBJS += obj/TMainDisplayWindowDict.o
OBJS += obj/TRootanaDisplayDict.o
OBJS += obj/TFancyHistogramCanvasDict.o

DALL += obj/TNetDirectoryDict.cxx
DALL += obj/TMainDisplayWindowDict.cxx
DALL += obj/TRootanaDisplayDict.cxx
DALL += obj/TFancyHistogramCanvasDict.cxx

endif

# libUnpack

OBJS += obj/UnpackVF48A.o
OBJS += obj/Alpha16.o
OBJS += obj/v1190unpack.o
OBJS += obj/v1742unpack.o

#

ALL  += lib/librootana.a

# old analyzer

ALL  += old_analyzer/event_dump.exe
ALL  += old_analyzer/event_skim.exe
ALL  += old_analyzer/analyzer.exe

DALL += old_analyzer/event_dump.o
DALL += old_analyzer/event_skim.o
DALL += old_analyzer/analyzer.o

# new midas analyzer

ALL  += obj/manalyzer_main.o
ALL  += manalyzer/manalyzer.exe
#MALL  += manalyzer/manalyzer.exe
#MALL  += manalyzer/manalyzer_example_cxx.exe
#ifdef HAVE_ROOT
#MALL  += manalyzer/manalyzer_example_root.exe
#MALL  += manalyzer/manalyzer_example_flow.exe
#MALL  += manalyzer/manalyzer_example_root_graphics.exe
#endif
#ALL   += $(MALL)

# test programs

ifdef HAVE_ROOT
ALL  += libMidasServer/test_midasServer.o libMidasServer/test_midasServer.exe
ifdef HAVE_MIDAS
ALL  += libMidasInterface/tests/testODB.o libMidasInterface/tests/testODB.exe
ALL  += libMidasInterface/tests/test_mvodb.o
ALL  += libMidasInterface/tests/test_mvodb.exe
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
OBJS += obj/mvodb.o
OBJS += obj/nullodb.o
ifdef HAVE_MIDAS
OBJS += obj/midasodb.o
endif
OBJS += obj/mxml.o
OBJS += obj/mxmlodb.o
OBJS += obj/mjson.o
OBJS += obj/mjsonodb.o

# manalyzer

OBJS += obj/manalyzer.o

all: $(ALL)

$(ALL): include
$(OBJS): include
#$(MALL): include
$(DALL): include

RC := include/rootana_config.h
RF := include/rootana_cflags.txt
RL := include/rootana_libs.txt

include:
	mkdir -p include lib obj
	-rm -f $(RC)
	touch $(RC)
	echo // rootana_config.h >> $(RC)
	echo // generated by rootana Makefile `date` >> $(RC)
	echo // >> $(RC)
ifdef HAVE_MIDAS
	echo "#define HAVE_MIDAS 1" >> $(RC)
	echo "//#define MIDASSYS $(MIDASSYS)" >> $(RC)
else
	echo "//#define HAVE_MIDAS 1" >> $(RC)
endif
ifdef HAVE_LIBZ
	echo "#define HAVE_LIBZ 1" >> $(RC)
else
	echo "//#define HAVE_LIBZ 1" >> $(RC)
endif
ifdef HAVE_ROOT
	echo "#define HAVE_ROOT 1" >> $(RC)
	echo "//#define ROOTSYS $(ROOTSYS)" >> $(RC)
else
	echo "//#define HAVE_ROOT 1" >> $(RC)
endif
ifdef HAVE_ROOT_HTTP
	echo "#define HAVE_ROOT_HTTP 1" >> $(RC)
else
	echo "//#define HAVE_ROOT_HTTP 1" >> $(RC)
endif
ifdef HAVE_ROOT_XML
	echo "#define HAVE_ROOT_XML 1" >> $(RC)
else
	echo "//#define HAVE_ROOT_XML 1" >> $(RC)
endif
ifdef HAVE_THTTP_SERVER
	echo "#define HAVE_THTTP_SERVER 1" >> $(RC)
else
	echo "//#define HAVE_THTTP_SERVER 1" >> $(RC)
endif
ifdef HAVE_XMLSERVER
	echo "#define HAVE_XMLSERVER 1" >> $(RC)
else
	echo "//#define HAVE_XMLSERVER 1" >> $(RC)
endif
ifdef HAVE_LIBNETDIRECTORY
	echo "#define HAVE_LIBNETDIRECTORY 1" >> $(RC)
else
	echo "//#define HAVE_LIBNETDIRECTORY 1" >> $(RC)
endif
ifdef HAVE_CXX11_THREADS
	echo "#define HAVE_CXX11_THREADS 1" >> $(RC)
else
	echo "//#define HAVE_CXX11_THREADS 1" >> $(RC)
endif

	echo "// end" >> $(RC)
	-rm -f $(RF)
	touch $(RF)
	echo "$(USER_CFLAGS)" >> $(RF)
	-rm -f $(RL)
	touch $(RL)
	echo "$(USER_LIBS)" >> $(RL)
	cd include; ln -sfv ../lib*/*.h .
	cd include; ln -sfv ../lib*/*.hxx .
	cd include; ln -sfv ../manalyzer/*.h .

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

#$(MALL): %.exe: %.o obj/manalyzer_main.o lib/librootana.a
#	$(CXX) -o $@ $(CXXFLAGS) $< obj/manalyzer_main.o lib/librootana.a $(MIDASLIBS) $(ROOTGLIBS) -lm -lz -lpthread $(RPATH)

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

obj/%.o: libUnpack/%.cxx
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/%.o: old_analyzer/%.cxx
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/%.o: manalyzer/%.cxx
	$(CXX) $(CXXFLAGS) -o $@ -c $<

manalyzer/manalyzer.exe: lib/librootana.a
	make -C manalyzer ROOTANASYS=.. $(MFLAGS)

html/index.html:
	-mkdir html
	-make -k dox
	touch html/index.html

dox: include
	doxygen

clean::
	-rm -f *.o *.a *.exe $(ALL)
	-rm -f */*.exe
	-rm -f */*Dict.cxx */*Dict.h */*Dict_rdict.pcm
	-rm -rf lib
	-rm -rf include
	-rm -rf obj

clean::
	make -C manalyzer clean

clean::
	-rm -f */*.o

clean::
	-rm -rf */*.exe.dSYM

clean::
	-rm -rf html

# end

# Makefile

CXXFLAGS_BASE = -g -O2 -Wall -Wuninitialized -I../libMidasInterface/ -I../libAnalyzer/

# optional ZLIB library

CXXFLAGS += -DHAVE_ZLIB

# ROOT libraries

ifdef ROOTSYS
ROOTLIBDIR := $(shell $(ROOTSYS)/bin/root-config --libdir)
ROOTGLIBS = -L$(ROOTLIBDIR) $(shell $(ROOTSYS)/bin/root-config --glibs) -lXMLParser -lThread
HAVE_RHTTP = $(shell $(ROOTSYS)/bin/root-config --has-http)
ifeq ($(HAVE_RHTTP),yes)
ROOTGLIBS += -lRHTTP
endif
CXXFLAGS += -DHAVE_ROOT $(shell $(ROOTSYS)/bin/root-config --cflags)
endif

# optional MIDAS libraries

ifdef MIDASSYS
MIDASLIBS = $(MIDASSYS)/linux/lib/libmidas.a -lutil -lrt
CXXFLAGS_BASE += -DHAVE_MIDAS -DOS_LINUX -Dextname -I$(MIDASSYS)/include

UNAME=$(shell uname)
ifeq ($(UNAME),Darwin)
CXXFLAGS_BASE += -DOS_LINUX -DOS_DARWIN
MIDASLIBS = $(MIDASSYS)/darwin/lib/libmidas.a
RPATH=
endif

endif


ALL:= 
OBJS:=
ifdef ROOTSYS
ALL  += libAnalyzerDisplay.a display_example.exe
OBJS += TComplicatedExampleCanvas.o TMainDisplayWindow.o TRootanaDisplay.o TSimpleExampleCanvas.o
OBJS += TMainDisplayWindowDict.o TRootanaDisplayDict.o
OBJS += TFancyHistogramCanvas.o TFancyHistogramCanvasDict.o
OBJS += TMulticanvas.o
endif

ifdef ROOTSYS
CXXFLAGS += -DHAVE_LIBNETDIRECTORY -I../libNetDirectory -I../
LIBS     += ../libNetDirectory/libNetDirectory.a
#CXXFLAGS_ROOTCINT = $(CXXFLAGS) -DHAVE_ROOT $(shell $(ROOTSYS)/bin/root-config) 
CXXFLAGS_ROOTCINT = $(CXXFLAGS_BASE) -DHAVE_ROOT 
CXXFLAGS = $(CXXFLAGS_BASE) -DHAVE_ROOT $(shell $(ROOTSYS)/bin/root-config --cflags)
endif

all: $(ALL)

libAnalyzerDisplay.a: $(OBJS)
	-rm -f $@
	ar -rv $@ $^

display_example.exe: display_example.cxx libAnalyzerDisplay.a ../libAnalyzer/libAnalyzer.a ../libMidasInterface/libMidasInterface.a
	$(CXX) -o $@ $(CXXFLAGS) $^ $(MIDASLIBS) $(LIBS) $(ROOTGLIBS) -lm -lz -lpthread $(RPATH)


%Dict.o: %Dict.cxx
	$(CXX) $(CXXFLAGS) -c $<

%Dict.cxx: %.hxx %_LinkDef.h
	LD_LIBRARY_PATH=$(ROOTLIBDIR) $(ROOTSYS)/bin/rootcint -f $@ -c -p $(CXXFLAGS_ROOTCINT) $^


%.o: %.cxx
	$(CXX) $(CXXFLAGS) -c $<

dox:
	doxygen

clean::
	-rm -f *.o *.a *.exe *Dict.h *Dict.cxx $(ALL)

# end

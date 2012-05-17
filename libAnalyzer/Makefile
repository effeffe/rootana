# Makefile

CXXFLAGS = -g -O2 -Wall -Wuninitialized -I../libMidasInterface/

# optional ZLIB library

CXXFLAGS += -DHAVE_ZLIB

# ROOT libraries

ifdef ROOTSYS
#ROOTLIBS  = -L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --libs)  -lXMLParser -lThread
ROOTGLIBS = -L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --glibs) -lXMLParser -lThread
#RPATH    += -Wl,-rpath,$(ROOTSYS)/lib
CXXFLAGS += -DHAVE_ROOT $(shell $(ROOTSYS)/bin/root-config --cflags)
endif

ALL:= 
OBJS:=
ifdef ROOTSYS
ALL  += libAnalyzer.a analyzer_example.exe
OBJS += TPeriodicClass.o
OBJS += TRootanaEventLoop.o
OBJS += TDataContainer.o
OBJS += TV1190Data.o
OBJS += TV792Data.o
endif

all: $(ALL)

libAnalyzer.a: $(OBJS)
	-rm -f $@
	ar -rv $@ $^

analyzer_example.exe: analyzer_example.cxx libAnalyzer.a ../libMidasInterface/libMidasInterface.a
	$(CXX) -o $@ $(CXXFLAGS) $^ $(MIDASLIBS) $(ROOTGLIBS) -lm -lz -lpthread $(RPATH) 



#%Dict.cxx: %.h
#	LD_LIBRARY_PATH=$(ROOTSYS)/lib $(ROOTSYS)/bin/rootcint -f $@ -c $^

%.o: %.cxx
	$(CXX) $(CXXFLAGS) -c $<

dox:
	doxygen

clean::
	-rm -f *.o *.a *.exe $(ALL)

# end

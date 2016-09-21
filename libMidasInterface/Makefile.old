# Makefile

CXXFLAGS = -g -O2 -Wall -Wuninitialized

# optional ZLIB library

CXXFLAGS += -DHAVE_LIBZ

# ROOT libraries

ifdef ROOTSYS
ROOTLIBS  = -L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --libs)  -lXMLParser -lThread
ROOTGLIBS = -L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --glibs) -lXMLParser -lThread
RPATH    += -Wl,-rpath,$(ROOTSYS)/lib
CXXFLAGS += -DHAVE_ROOT $(shell $(ROOTSYS)/bin/root-config --cflags)
endif

ALL:= libMidasInterface.a

OBJS:=
OBJS += TMidasEvent.o
OBJS += TMidasFile.o

ifdef MIDASSYS
CXXFLAGS += -I$(MIDASSYS)/include -DHAVE_MIDAS
OBJS += TMidasOnline.o
MOBJS += TMidasOnline.o -L$(MIDASSYS)/darwin/lib -lmidas
UNAME=$(shell uname)
ifeq ($(UNAME),Darwin)
CXXFLAGS += -DOS_LINUX -DOS_DARWIN
#MIDASLIBS = $(MIDASSYS)/darwin/lib/libmidas.a
RPATH= 
endif                                                                                                                                                        
endif

ifdef ROOTSYS
OBJS += XmlOdb.o HttpOdb.o
endif

OBJS += midasio.o lz4.o lz4hc.o lz4frame.o xxhash.o manalyzer.o

#ALL  += manalyzer.o manalyzer.exe
#ALL  += manalyzer_example1.o manalyzer_example1.exe
#ALL  += manalyzer_example2.o manalyzer_example2.exe
#ALL  += manalyzer_example3.o manalyzer_example3.exe

all: $(ALL)

libMidasInterface.a: $(OBJS)
	-rm -f $@
	ar -rv $@ $^

#%Dict.cxx: %.h
#	LD_LIBRARY_PATH=$(ROOTSYS)/lib $(ROOTSYS)/bin/rootcint -f $@ -c $^

%.o: %.cxx
	$(CXX) $(CXXFLAGS) -c $<

manalyzer.exe: %.exe: %.o manalyzer.o midasio.o
	$(CXX) -o $@ $(CXXFLAGS) $^ $(MOBJS) -L../lib -lrootana -lz $(ROOTLIBS)

manalyzer_example1.exe: %.exe: %.o manalyzer.o midasio.o TMidasEvent.o
	$(CXX) -o $@ $(CXXFLAGS) $^ $(MOBJS) -L../lib -lrootana -lz $(ROOTLIBS)

manalyzer_example2.exe: %.exe: %.o manalyzer.o midasio.o TMidasEvent.o
	$(CXX) -o $@ $(CXXFLAGS) $^ $(MOBJS) -L../lib -lrootana -lz $(ROOTLIBS)

manalyzer_example3.exe: %.exe: %.o manalyzer.o midasio.o TMidasEvent.o
	$(CXX) -o $@ $(CXXFLAGS) $^ $(MOBJS) -L../lib -lrootana -lz $(ROOTLIBS)

dox:
	doxygen

clean::
	-rm -f *.o *.a *.exe $(ALL)

# end

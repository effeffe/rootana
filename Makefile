# Makefile

# ROOTANA library
OBJS = TMidasFile.o TMidasEvent.o

CXXFLAGS = -g -O2 -Wall -Wuninitialized

# optional ROOT libraries

ifdef ROOTSYS
ROOTLIBS  = -L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --libs)  -lXMLParser -lThread
ROOTGLIBS = -L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --glibs) -lXMLParser -lThread
RPATH    += -Wl,-rpath,$(ROOTSYS)/lib
CXXFLAGS += -DHAVE_ROOT -I$(ROOTSYS)/include
OBJS     +=  XmlOdb.o midasServer.o
endif

# optional MIDAS libraries

ifdef MIDASSYS
MIDASLIBS = $(MIDASSYS)/linux/lib/libmidas.a -lutil
CXXFLAGS += -DHAVE_MIDAS -DOS_LINUX -Dextname -I$(MIDASSYS)/include
OBJS     += TMidasOnline.o
endif

# optional ZLIB library

CXXFLAGS += -DHAVE_ZLIB

ALL:= librootana.a

ifdef ROOTSYS
ALL+= analyzer.exe
endif

all: $(ALL)

librootana.a: $(OBJS)
	-rm -f $@
	ar -rv $@ $^

analyzer.exe: %.exe: %.o librootana.a
	$(CXX) -o $@ $(CXXFLAGS) $^ $(MIDASLIBS) $(ROOTGLIBS) -lm -lz -lpthread $(RPATH)

%.o: %.cxx
	$(CXX) $(CXXFLAGS) -c $<

dox:
	doxygen

clean::
	-rm -f *.o *.exe $(ALL)

# end

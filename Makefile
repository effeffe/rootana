# Makefile

# ROOT
ifndef ROOTSYS
export ROOTSYS := $(HOME)/packages/root
endif

# ROOT library
ROOTLIBS  = -L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --libs)  -lXMLParser -lThread -Wl,-rpath,$(ROOTSYS)/lib
ROOTGLIBS = -L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --glibs) -lXMLParser -lThread -Wl,-rpath,$(ROOTSYS)/lib

# ROOTANA library
OBJS = midasServer.o TMidasFile.o TMidasEvent.o XmlOdb.o

CXXFLAGS = -g -O2 -Wall -Wuninitialized -I$(ROOTSYS)/include

ifdef MIDASSYS
CXXFLAGS += -DHAVE_MIDAS -DOS_LINUX -Dextname -I$(MIDASSYS)/include
MIDASLIBS = $(MIDASSYS)/linux/lib/libmidas.a
OBJS     += TMidasOnline.o
endif

all: librootana.a analyzer.exe

librootana.a: $(OBJS)
	-rm -f $@
	ar -rv $@ $^

analyzer.exe: %.exe: %.o librootana.a $(MIDASLIBS)
	$(CXX) -o $@ $(CXXFLAGS) $^ $(MIDASLIBS) $(ROOTGLIBS) -lm -lz -lutil -lnsl -lpthread 

%.o: %.cxx
	$(CXX) $(CXXFLAGS) -c $<

dox:
	doxygen

clean::
	-rm -f *.o *.exe

# end

# Makefile

#MIDASSYS=/home1/midas/midas
#MIDASSYS=/home/midas/midas
MIDASSYS=/home/olchansk/daq/midas/midas

# MIDAS library
MIDASLIBS = $(MIDASSYS)/linux/lib/libmidas.a

# ROOT
ifndef ROOTSYS
export ROOTSYS := /triumfcs/trshare/olchansk/root/root_v5.12.00_SL42_amd64
endif

# ROOT library
ROOTLIBS  = -L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --libs)  -lXMLParser -lThread -Wl,-rpath,$(ROOTSYS)/lib
ROOTGLIBS = -L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --glibs) -lXMLParser -lThread -Wl,-rpath,$(ROOTSYS)/lib

CXXFLAGS = -DOS_LINUX -Dextname -g -O2 -Wall -Wuninitialized -I$(MIDASSYS)/include -I$(ROOTSYS)/include

all: analyzer.exe

analyzer.exe: %.exe: %.o midasServer.o TMidasOnline.o TMidasFile.o TMidasEvent.o XmlOdb.o $(MIDASLIBS)
	$(CXX) -o $@ $(CXXFLAGS) $^ $(MIDASLIBS) $(ROOTGLIBS) -lm -lz -lutil -lnsl -lpthread 

%.o: %.cxx
	$(CXX) $(CXXFLAGS) -I$(ROOTSYS)/include $(OSFLAGS) -c $<

dox:
	doxygen

clean::
	-rm -f *.o *.exe

# end

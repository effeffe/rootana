# Makefile

# ROOTANA library
OBJS = TMidasFile.o TMidasEvent.o

CXXFLAGS = -g -O2 -Wall -Wuninitialized

# optional ROOT libraries

ifdef ROOTSYS
ROOTLIBS  = -L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --libs)  -lXMLParser -lThread -Wl,-rpath,$(ROOTSYS)/lib
ROOTGLIBS = -L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --glibs) -lXMLParser -lThread -Wl,-rpath,$(ROOTSYS)/lib
OBJS     +=  XmlOdb.o midasServer.o
CXXFLAGS += -DHAVE_ROOT -I$(ROOTSYS)/include
endif

# optional MIDAS libraries

ifdef MIDASSYS
CXXFLAGS += -DHAVE_MIDAS -DOS_LINUX -Dextname -I$(MIDASSYS)/include
MIDASLIBS = $(MIDASSYS)/linux/lib/libmidas.a
OBJS     += TMidasOnline.o
endif

ALL:= librootana.a

ifdef ROOTSYS
ALL+= analyzer.exe
endif

all: $(ALL)

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
	-rm -f *.o *.exe $(ALL)

# end

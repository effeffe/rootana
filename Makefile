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

# optional TNetDirectory code

CXXFLAGS += -DHAVE_LIBNETDIRECTORY
OBJS     += ./libNetDirectory/netDirectoryServer.o

ALL+= libNetDirectory/libNetDirectory.a

# optional old midas server

CXXFLAGS += -DOLD_SERVER

ALL+= librootana.a

ALL+= testODB.exe

ifdef ROOTSYS
ALL+= analyzer.exe
ALL+= test_midasServer.exe
endif

all: $(ALL)

libNetDirectory/libNetDirectory.a:
	make -C libNetDirectory

librootana.a: $(OBJS)
	-rm -f $@
	ar -rv $@ $^

%.exe: %.o librootana.a
	$(CXX) -o $@ $(CXXFLAGS) $^ $(MIDASLIBS) $(ROOTGLIBS) -lm -lz -lpthread $(RPATH)

%.o: %.cxx
	$(CXX) $(CXXFLAGS) -o $@ -c $<

dox:
	doxygen

clean::
	-rm -f *.o *.a *.exe $(ALL)

clean::
	make -C libNetDirectory clean

# end

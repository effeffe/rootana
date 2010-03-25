# Makefile

# ROOTANA library
OBJS = TMidasFile.o TMidasEvent.o

CXXFLAGS = -g -O2 -Wall -Wuninitialized

# optional ROOT libraries

ifdef ROOTSYS
ROOTLIBS  = -L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --libs)  -lXMLParser -lThread
ROOTGLIBS = -L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --glibs) -lXMLParser -lThread
RPATH    += -Wl,-rpath,$(ROOTSYS)/lib
CXXFLAGS += -DHAVE_ROOT $(shell $(ROOTSYS)/bin/root-config --cflags)
OBJS     +=  XmlOdb.o HttpOdb.o midasServer.o libNetDirectory/RootLock.o
endif

# optional MIDAS libraries

ifdef MIDASSYS
MIDASLIBS = $(MIDASSYS)/linux/lib/libmidas.a -lutil -lrt
CXXFLAGS += -DHAVE_MIDAS -DOS_LINUX -Dextname -I$(MIDASSYS)/include
OBJS     += TMidasOnline.o

UNAME=$(shell uname)
ifeq ($(UNAME),Darwin)
CXXFLAGS += -DOS_LINUX -DOS_DARWIN
MIDASLIBS = $(MIDASSYS)/darwin/lib/libmidas.a
RPATH=
endif

endif

# optional ZLIB library

CXXFLAGS += -DHAVE_ZLIB

# optional TNetDirectory code

ifdef ROOTSYS
CXXFLAGS += -DHAVE_LIBNETDIRECTORY
OBJS     += ./libNetDirectory/netDirectoryServer.o

ALL+= libNetDirectory/libNetDirectory.a
endif

# optional old midas server

CXXFLAGS += -DOLD_SERVER

ALL+= librootana.a
ALL+= event_dump.exe

ifdef ROOTSYS
ifdef MIDASSYS
ALL+= testODB.exe
endif
endif

ALL+= html/index.html

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

html/index.html:
	-mkdir html
	-make -k dox
	touch html/index.html

dox:
	doxygen

clean::
	-rm -f *.o *.a *.exe $(ALL)

clean::
	make -C libNetDirectory clean

clean::
	-rm -f html/*

# end

# Makefile

CXXFLAGS = -g -O2 -Wall -Wuninitialized

# MIDAS interface library

CXXFLAGS += -I./libMidasInterface
#OBJS     += ./libMidasInterfacerectory/netDirectoryServer.o
ALL+= libMidasInterface/libMidasInterface.a
LIBS+= libMidasInterface/libMidasInterface.a

# optional ROOT libraries

ifdef ROOTSYS
ROOTLIBS  = -L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --libs)  -lXMLParser -lXMLIO -lThread
ROOTGLIBS = -L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --glibs) -lXMLParser -lXMLIO -lThread
RPATH    += -Wl,-rpath,$(ROOTSYS)/lib
CXXFLAGS += -DHAVE_ROOT $(shell $(ROOTSYS)/bin/root-config --cflags)
endif

# optional MIDAS libraries

ifdef MIDASSYS
MIDASLIBS = $(MIDASSYS)/linux/lib/libmidas.a -lutil -lrt
CXXFLAGS += -DHAVE_MIDAS -DOS_LINUX -Dextname -I$(MIDASSYS)/include

#UNAME=$(shell uname)
#ifeq ($(UNAME),Darwin)
#CXXFLAGS += -DOS_LINUX -DOS_DARWIN
#MIDASLIBS = $(MIDASSYS)/darwin/lib/libmidas.a
#RPATH=
#endif

endif

# optional TNetDirectory code

ifdef ROOTSYS
CXXFLAGS += -DHAVE_LIBNETDIRECTORY -IlibNetDirectory
OBJS     += ./libNetDirectory/netDirectoryServer.o
LIBS     += libNetDirectory/libNetDirectory.a

ALL+= libNetDirectory/libNetDirectory.a
endif

# optional XmlServer code

ifdef ROOTSYS
CXXFLAGS += -DHAVE_XMLSERVER -IlibXmlServer
OBJS     += ./libXmlServer/xmlServer.o

#ALL+= libNetDirectory/libNetDirectory.a
endif

# optional old midas server

ifdef OLD_SERVER
CXXFLAGS += -DOLD_SERVER -I./obsolete -IlibNetDirectory
OBJS     += ./obsolete/midasServer.o
endif

#ALL+= librootana.a
ALL+= event_dump.exe
ALL+= event_skim.exe

ifdef ROOTSYS
ifdef MIDASSYS
ALL+= tests/testODB.exe
endif
endif

ALL+= html/index.html

ifdef ROOTSYS
ALL+= analyzer.exe
ALL+= tests/test_midasServer.exe
endif

all: $(ALL)

libMidasInterface/libMidasInterface.a:
	make -C libMidasInterface

libNetDirectory/libNetDirectory.a:
	make -C libNetDirectory

#librootana.a: $(OBJS)
#	-rm -f $@
#	ar -rv $@ $^

%.exe: %.o $(OBJS) $(LIBS)
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
	make -C libMidasInterface clean

clean::
	make -C libNetDirectory clean

clean::
	-rm -f html/*

# end

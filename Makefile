# Makefile

CXXFLAGS = -g -O2 -Wall -Wuninitialized -I./include

# MIDAS interface library

CXXFLAGS += -I./libMidasInterface
ALL+=  libMidasInterface/libMidasInterface.a
LIBS+= libMidasInterface/libMidasInterface.a

# optional ROOT libraries

ifdef ROOTSYS
ROOTLIBDIR := $(shell $(ROOTSYS)/bin/root-config --libdir)
ROOTLIBS   := -L$(ROOTLIBDIR) $(shell $(ROOTSYS)/bin/root-config --libs)  -lXMLParser -lXMLIO -lThread
ROOTGLIBS  := -L$(ROOTLIBDIR) $(shell $(ROOTSYS)/bin/root-config --glibs) -lXMLParser -lXMLIO -lThread
ROOTCFLAGS := $(shell $(ROOTSYS)/bin/root-config --cflags)
RPATH    += -Wl,-rpath,$(ROOTLIBDIR)
CXXFLAGS += -DHAVE_ROOT $(ROOTCFLAGS)
endif

# optional MIDAS libraries

ifdef MIDASSYS
MIDASLIBS = $(MIDASSYS)/linux/lib/libmidas.a -lutil -lrt
CXXFLAGS += -DHAVE_MIDAS -DOS_LINUX -Dextname -I$(MIDASSYS)/include

UNAME=$(shell uname)
ifeq ($(UNAME),Darwin)
CXXFLAGS += -DOS_LINUX -DOS_DARWIN
MIDASLIBS = $(MIDASSYS)/darwin/lib/libmidas.a
RPATH=
endif

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
endif

# optional old midas server

HAVE_MIDASSERVER = 1

ifdef HAVE_MIDASSERVER
CXXFLAGS += -DOLD_SERVER -DHAVE_MIDASSERVER
OBJS     += ./libMidasServer/midasServer.o
endif

# optional libAnalyzer and libAnalyzerDisplay code
ifdef ROOTSYS
LIBS     += libAnalyzer/libAnalyzer.a libAnalyzerDisplay/libAnalyzerDisplay.a
ALL+= libAnalyzer/libAnalyzer.a libAnalyzerDisplay/libAnalyzerDisplay.a
endif

ALL+= lib/librootana.a
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

$(ALL): include

include:
	mkdir -p include
	cd include; ln -sfv ../lib*/*.h .
	cd include; ln -sfv ../lib*/*.hxx .

lib/librootana.a: $(LIBS) $(OBJS)
	mkdir -p lib
	-rm -f $@
	ar -rv $@ lib*/*.o

libMidasInterface/libMidasInterface.a:
	make -C libMidasInterface

libNetDirectory/libNetDirectory.a:
	make -C libNetDirectory

libAnalyzer/libAnalyzer.a:
	make -C libAnalyzer

libAnalyzerDisplay/libAnalyzerDisplay.a:
	make -C libAnalyzerDisplay

%.exe: %.o lib/librootana.a
	$(CXX) -o $@ $(CXXFLAGS) $< lib/librootana.a $(MIDASLIBS) $(ROOTGLIBS) -lm -lz -lpthread $(RPATH)

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
	-rm -rf lib
	-rm -rf include

clean::
	make -C libMidasInterface clean

clean::
	make -C libNetDirectory clean

clean::
	make -C libAnalyzer clean

clean::
	make -C libAnalyzerDisplay clean

clean::
	-rm -rf html

# end

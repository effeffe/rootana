# Makefile

CXXFLAGS = -g -O2 -Wall -Wuninitialized

# ROOT libraries

ifdef ROOTSYS
ROOTLIBDIR := $(shell $(ROOTSYS)/bin/root-config --libdir)
ROOTLIBS  = -L$(ROOTLIBDIR) $(shell $(ROOTSYS)/bin/root-config --libs)  -lXMLParser -lThread
ROOTGLIBS = -L$(ROOTLIBDIR) $(shell $(ROOTSYS)/bin/root-config --glibs) -lXMLParser -lThread
RPATH    += -Wl,-rpath,$(ROOTLIBDIR)
CXXFLAGS += -DHAVE_ROOT $(shell $(ROOTSYS)/bin/root-config --cflags)
endif

ALL:= libNetDirectory.a

OBJS = TNetDirectory.o TNetDirectoryDict.o netDirectoryServer.o RootLock.o

all: $(ALL)

libNetDirectory.a: $(OBJS)
	-rm -f $@
	ar -rv $@ $^

%Dict.cxx: %.h
	LD_LIBRARY_PATH=$(ROOTLIBDIR) $(ROOTSYS)/bin/rootcint -f $@ -c $^

%.o: %.cxx
	$(CXX) $(CXXFLAGS) -c $<

dox:
	doxygen

clean::
	-rm -f *.o *.a *.exe *Dict.h $(ALL)

# end

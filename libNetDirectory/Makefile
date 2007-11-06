# Makefile

CXXFLAGS = -g -O2 -Wall -Wuninitialized

# ROOT libraries

ifdef ROOTSYS
ROOTLIBS  = -L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --libs)  -lXMLParser -lThread
ROOTGLIBS = -L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --glibs) -lXMLParser -lThread
RPATH    += -Wl,-rpath,$(ROOTSYS)/lib
CXXFLAGS += -DHAVE_ROOT -I$(ROOTSYS)/include
endif

ALL:= libNetDirectory.a

OBJS = TNetDirectory.o TNetDirectoryDict.o netDirectoryServer.o RootLock.o

all: $(ALL)

libNetDirectory.a: $(OBJS)
	-rm -f $@
	ar -rv $@ $^

%Dict.cxx: %.h
	LD_LIBRARY_PATH=$(ROOTSYS)/lib $(ROOTSYS)/bin/rootcint -f $@ -c $^

%.o: %.cxx
	$(CXX) $(CXXFLAGS) -c $<

dox:
	doxygen

clean::
	-rm -f *.o *.a *.exe $(ALL)

# end

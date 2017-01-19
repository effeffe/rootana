#
# Makefile for the MIDAS analyzer component of ROOTANA
#
# Options:
# make NO_MIDAS=1 # build without MIDAS support
# make NO_ROOT=1 # build without ROOT support
#

CXXFLAGS = -g -O2 -Wall -Wuninitialized

# ROOTANA libraries

ifndef ROOTANASYS
norootanasys:
	@echo Error: ROOTANASYS in not defined, please source thisrootana.{sh,csh}
else
CXXFLAGS += -I$(ROOTANASYS)/include
LIBS     += -L$(ROOTANASYS)/lib -lrootana
endif # ROOTANASYS

# optional ROOT libraries

ifndef NO_ROOT
ROOTVERSION := $(shell root-config --version)
endif

ifdef ROOTVERSION
HAVE_ROOT  := 1
ROOTFEATURES := $(shell root-config --features)
ROOTLIBDIR := $(shell root-config --libdir)
LIBS += -L$(ROOTLIBDIR) $(shell root-config --glibs) -lThread
ROOTCFLAGS := $(shell root-config --cflags)
RPATH    += -Wl,-rpath,$(ROOTLIBDIR)
CXXFLAGS += $(ROOTCFLAGS)
HAVE_ROOT_HTTP := $(findstring http,$(ROOTFEATURES))
HAVE_ROOT_XML  := $(findstring xml,$(ROOTFEATURES))

ifdef HAVE_ROOT_XML
LIBS += -lXMLParser -lXMLIO
endif

ifdef HAVE_ROOT_HTTP
LIBS += -lRHTTP
endif

endif # HAVE_ROOT

# optional MIDAS libraries

ifdef NO_MIDAS
MIDASSYS:=
endif

ifneq ($(MIDASSYS),)

HAVE_MIDAS=1
MIDASLIBS = $(MIDASSYS)/linux/lib/libmidas.a -lutil -lrt
CXXFLAGS += -DHAVE_MIDAS -DOS_LINUX -Dextname -I$(MIDASSYS)/include

UNAME=$(shell uname)
ifeq ($(UNAME),Darwin)
CXXFLAGS += -DOS_LINUX -DOS_DARWIN
MIDASLIBS = $(MIDASSYS)/darwin/lib/libmidas.a
RPATH=
endif

LIBS += $(MIDASLIBS)

endif # HAVE_MIDAS

USER_MODULES:=
# add more user modules here
#USER_MODULES += module_agbars.o

ALL += manalyzer.exe
ALL += manalyzer_example_cxx.exe
ALL += manalyzer_example_flow.exe
ifdef HAVE_ROOT
ALL += manalyzer_example_root.exe
ALL += manalyzer_example_root_graphics.exe
endif

all: $(ALL)

%.exe: %.o manalyzer_main.o
	$(CXX) -o $@ manalyzer_main.o $< $(CXXFLAGS) $(LIBS) -lm -lz -lpthread

%.o: %.cxx
	$(CXX) -o $@ $(CXXFLAGS) -c $<

html/index.html:
	-mkdir html
	-make -k dox
	touch html/index.html

dox:
	doxygen

clean::
	-rm -f *.o *.a *.exe $(ALL)

clean::
	-rm -rf *.exe.dSYM

clean::
	-rm -rf html

# end

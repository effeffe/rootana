#
# Makefile for the MIDAS analyzer component of ROOTANA
#
# Options:
# make NO_ROOT=1 # build without ROOT support
#

CXXFLAGS += -g -O2 -Wall -Wuninitialized

ifndef ROOTANASYS
norootanasys:
	@echo Error: ROOTANASYS in not defined, please source thisrootana.{sh,csh}
endif

# check for ROOT

ifdef NO_ROOT
ROOTSYS:=
endif

ifneq ($(ROOTSYS),)
HAVE_ROOT:=1
endif

# get the rootana Makefile settings

CXXFLAGS += -I$(ROOTANASYS)/include $(shell cat $(ROOTANASYS)/include/rootana_cflags.txt)
LIBS     += -L$(ROOTANASYS)/lib -lrootana $(shell cat $(ROOTANASYS)/include/rootana_libs.txt)

# select the main program - local custom main()
# or standard main() from rootana

#MAIN := manalyzer_main.o
MAIN := $(ROOTANASYS)/obj/manalyzer_main.o

# uncomment and define analyzer modules here

#MODULES += agbars_module.o
#ALL     += barsana.exe

# examples

EXAMPLE_ALL += manalyzer.exe
EXAMPLE_ALL += manalyzer_example_cxx.exe
EXAMPLE_ALL += manalyzer_example_flow.exe
ifdef HAVE_ROOT
EXAMPLE_ALL += manalyzer_example_root.exe
EXAMPLE_ALL += manalyzer_example_root_graphics.exe
endif

all:: $(EXAMPLE_ALL)
all:: $(MODULES)
all:: $(ALL)

%.exe: %.o manalyzer_main.o
	$(CXX) -o $@ manalyzer_main.o $< $(CXXFLAGS) $(LIBS) -lm -lz -lpthread

$(EXAMPLE_ALL): %.exe:
	$(CXX) -o $@ $(ROOTANASYS)/obj/manalyzer_main.o $< $(CXXFLAGS) $(LIBS) -lm -lz -lpthread

%.exe: $(MAIN) $(MODULES)
	$(CXX) -o $@ $(MAIN) $(MODULES) $(CXXFLAGS) $(LIBS) -lm -lz -lpthread

%.o: %.cxx
	$(CXX) -o $@ $(CXXFLAGS) -c $<

html/index.html:
	-mkdir html
	-make -k dox
	touch html/index.html

dox:
	doxygen

clean::
	-rm -f *.o *.a *.exe

clean::
	-rm -f $(ALL)

clean::
	-rm -f $(EXAMPLE_ALL)

clean::
	-rm -rf *.exe.dSYM

clean::
	-rm -rf html

# end

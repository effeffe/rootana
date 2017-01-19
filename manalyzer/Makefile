#
# Makefile for the MIDAS analyzer component of ROOTANA
#
# Options:
# make NO_MIDAS=1 # build without MIDAS support
# make NO_ROOT=1 # build without ROOT support
#

CXXFLAGS += -g -O2 -Wall -Wuninitialized

ifndef ROOTANASYS
norootanasys:
	@echo Error: ROOTANASYS in not defined, please source thisrootana.{sh,csh}
endif

CXXFLAGS += -I$(ROOTANASYS)/include $(shell cat $(ROOTANASYS)/include/rootana_cflags.txt)
LIBS     += -L$(ROOTANASYS)/lib -lrootana $(shell cat $(ROOTANASYS)/include/rootana_libs.txt)

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

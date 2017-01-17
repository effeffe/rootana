# MIDAS analyzer README

### Introduction

The new MIDAS analyzer was written with explicit goals in mind:

* preserve the idea of a modular analyzer (from the old MIDAS analyzer mana.c)
* preserve the idea of "data flow" (from the "flow analyzer")
* the same analysis code can be used online and offline, no dependancies on the MIDAS package
* the same analysis code can be used in batch mode or in interactive graphical mode
* better management of life time of ROOT objects (separation of module and run objects)

### Concept of analyzer module

TBW (explain separation of module and run objects)

### The flow object

TBW (explain how to use the flow object to pass data between modules)

### Event analysis flags

* TAFlag_OK - all is good
* TAFlag_SKIP - tells manalyzer to skip processing of this event by subsequent modules (for implementing "filter" modules)
* TAFlag_QUIT - stop analyzing events and shutdown manalyzer (the normal end of run sequence is followed)
* TAFlag_WRITE - write this event to the output file stream
* TAFlag_DISPLAY - mark this event as "interesting" for the interactive display module

### manalyzer module and object life time

* analyzer start:
  call module constructors
  call module Init() methods

* run start:
  call module NewRun() methods
  call run constructors
  call run BeginRun() methods

* if running from file:
  call run AnalyzeSpecialEvent() methods for the ODB dump event (evid 0x8000)

* for each event:
  call run Analyze() methods

* when switching from one subrun file to the next subrun file:
  BeginRun()/EndRun() are not called
  AnalyzeSpecialEvent() is called twice: once for the ODB dump event in the old subrun file (evid 0x8001) and once for the ODB dump event in the new subrun file (evid 0x8000)

* run end:
  call run AnalyzeSpecialEvent() methods for the ODB dump event (evid 0x8001)
  call run EndRun() methods
  call run destructors

* analyzer shutdown:
  definitely do the "run end" activity (all run objects destroyed)
  call module Finish() methods
  call module destructors
  return from manalyzer_main()

### The ROOT helper class TARootHelper

TBW

### Module registration

TBW

### The main program

TBW

### The event dump module

TBW

### The interative display module

TBW

### XXX

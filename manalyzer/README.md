# MIDAS analyzer README

### Introduction

The new MIDAS analyzer was written to combine the good ideas from the existing analyzers
and to correct some of the known problems:

* preserve the idea of a modular analyzer (from the old MIDAS analyzer mana.c)
* preserve the idea of "data flow" (from the "flow analyzer")
* the same analysis code can be used online and offline, no dependancies on the MIDAS package
* the same analysis code can be used in batch mode or in interactive graphical mode
* correct processing of subrun files
* better management of life time for ROOT objects

### Quick start

TBW - explain creating of analyzer:

* copy example, copy Makefile, copy manalyzer_main.
* create histograms in the BeginRun() method
* save results in the EndRun() method
* process data and fill histograms in the Analyze() method
* extract midas data banks like this: TBW

TBW - explain running of analyzer:

* invoke as "./ana.exe runNNNsub*.mid.gz"
* ROOT output file "outputNNN.root" is created
* histograms booked in BeginRun() "live" inside this ROOT output file (by default)
* book histograms in the Analyze() method
* do final computations (fit histograms, etc) in EndRun()
* ROOT output file is closed
* module "run object" is destroyed
* after the ROOT output file is closed, histograms created "inside the ROOT output file" vanish,
but no user analysis code is called after EndRun() making is impossible to crash on trying to use
the vanished objects.
* if desired, one can place ROOT objects into the ROOT memory instead of the ROOT output file

### ROOT Web server

TBW - explain that all histograms are automatically exported to web browser

### manalyzer command line switches

TBW

* -g - enable graphical mode
* -i - enable interactive mode (see section XXX)
* -Rport - enable ROOT web server on given port (use port 8081), connect via "firefox http://localhost:8081"
* -Xport - enable XML Web server for roody (use port 9091), connect via "roody -H http://localhost:9091"
* -Pport - enable old TNetDirectory server for roody (use port 9091), connect via "roody -P localhost:9091"

### Concept of analyzer module

An analyzer for a non-trivial experiment may be quite complicated. To manage this complexity
one can arrange the code in independant analyzer modules. To communicate results between modules
one can use ordinary C++ coding or one can use the mechanism of flow objects described in the next section.

A typical analyzer module may perform several duties:

* extract data from a MIDAS event data bank, perform some computations, fill histograms: this is done in the module's Analyze() method.
* perform final computations, save results: this is done in the module's EndRun() method.
* prepare to start analyzes, create histogram objects, initialize data structures, load calibrations: in the module's BeginRun() method.

An analyzer may be used to process just one data file, a sequence of data files from the same run (subrun files) or several
different runs.

To correctly manage the lifetime (creation and destructions) of all data objects,
the analyzer uses "run objects".

A "run object" holds all the data (histograms, canvases, c++ structures, etc) for the run currently being processed.
these data are created when the run starts, and are destroyed when the run ends, encouraging a coding style
where pointers to deleted objects will not be accidentally kept and used, leading to memory corruption and crashes.

The analyzer framework manages (creates and destroys) run objects using the factory pattern. It the typical case,
the user run object is connected to the framework by creating a TAFactory object (written explicitely
or using the TAFactoryTemplate<T>) and passing it to the TARegister object via static initialization.

Previous analyzer frameworks did not use this type of "run object" to manage per-run data, and encouraged
the use of global variables for ROOT file objects, for ROOT histograms, etc. Together with ROOT's idiosyncratic
emory management, where some ROOT objects "live" inside in normal memory and behave like normal C++ objects while other
ROOT objects live "inside ROOT file" objects and "vanish" when the ROOT file is closed,
made it very easy to write analyzers that crash at the end of run or crash
when switching from one run to the next. (a big problem when writing online analyzers).

### The flow object

An analyzer for a non-trivial experiment may have several analyzer modules
performing different tasks separately (an ADC module may unpack and calibrate ADC data, a TDC module
may unpack and sort TDC data). Result of these modules is often C++ classes (array of ADC pulse heights,
array of TDC hit times). To pass these C++ objects to the next analyzer module where these data can be
ombined together, one can use the "flow event". (as the bank structure of MIDAS events is inconvinient
or handling C++ objects).

C++ objects that will be passed between modules should extend the class TAFlowEvent
(as demonstrated by Object1 and Object2 in manalyzer_example_flow.cxx). The TAFlowEvent object
maintains a simple linked list of all flow objects. Each module Analyze() method has access
to all existing flow objects and can add new flow objects as desired. The flow event and all the flow objects
are automatically deleted after the last analyzer module Analyze() method is completed.

Here is some examples taken from manalyzer_example_flow.cxx:

* define a flow object: class Object2 : public TAFlowEvent { public: std::string fStringValue; ... };
* add flow object: flow = new Object2(flow, "some text");
* get a specific object (by C++ class type): Object2* o2 = flow->Find<Object2>();
* loop over all objects: TAFlowEvent* f = flow; while (f) { Object1* o1 = dynamic_cast<Object1*>(f); if (o1) { ... }; f = f->fNext; }

If desired, one can use the function AnalyzeFlowEvent() to separate the analysis of flow events
from the analysis of MIDAS events. For example, one can unpack MIDAS data banks into C++ structures
stored in a flow event in the function Analyze() of one module and process the data and fill the histograms
in the function AnalyzeFlowEvent() of a different module (separate data unpacking module and data
analysis module).

### The flow event queue

Non-trivial experiments may have multiple physics events contained inside a single MIDAS
event. In this situation one can unpack each physics event into it's own separate flow event
and ask manalyzer to process each of these flow events separately, as if there were multiple
midas events.

This is done in the Analyze() method by placing the individual flow events
into the flow queue: TAFlowEvent*e = unpack_event(midas_event); runinfo->fFlowQueue.push_back(e);

After manalyzer finishes processing the current midas event, it will proceed
with processing the queued flow events. Each queued flow event is processed the same way
as normal midas events, except that the Analyze() method is not called (there is no midas event!),
so only the AnalyzeFlowEvent() method will be used. The flags work the same way, and one can chain
additional flow objects to the flow event as it passes from one module to the next. At the
very end, the flow event is automatically deleted.

After all queued flow events are processed, manalyzer will continue with processing
the next midas event.

The flow event queue can also be used to finish processing any events remaining buffered
or queued at the EndRun() time as described in the next section.

### The PreEndRun method

Sometimes physics events need to be generated and processed at the end of a run after all
midas events have already been processed, after the last Analyze() call, but before
the final EndRun() call.

This happens when MIDAS midas events contain a continuous stream of data
and the stream unpacker has to maintain a buffer of incomplete data between Analyze() calls.

This also happens when the analyzer contains an event builder component which may contain
a buffer for incomplete or pending physics events.

To ensure that all of this buffered data is analyzed and no unprocessed data is left behind,
use the method PreEndRun().

It is called before the final EndRun() and it gives the analysis module an opportunuty to generate
any number of flow events (via push_back() into an std::deque). After calling PreEndRun()
for all modules, the accumulated flow events are processed by calling AnalyzeFlowEvent()
similar to processing normal flow events.

### Event analysis flags

The user analysis code in the Analyze() method can influence data processing by the manalyzer framework
by manipulating the TAFlags:

* TAFlag_OK - all is good
* TAFlag_SKIP - tells manalyzer to skip processing of this event by subsequent modules (for implementing "filter" modules)
* TAFlag_QUIT - stop analyzing events and shutdown manalyzer (the normal end of run sequence is followed)
* TAFlag_WRITE - write this event to the output file stream
* TAFlag_DISPLAY - mark this event as "interesting" for the interactive display module

### manalyzer module and object life time

* analyzer start:
    - call module constructors
    - call module Init() methods

* run start:
    - call module NewRun() methods
    - call run constructors
    - call run BeginRun() methods

* if running from file:
    - call run AnalyzeSpecialEvent() methods for the ODB dump event (evid 0x8000)

* for each event:
    - call run Analyze() methods, which may generate a flow event
    - if there is a flow event, call run AnalyzeFlowEvent methods

* when switching from one subrun file to the next subrun file:
    - BeginRun()/EndRun() are not called
    - NextSubrun() is called with runinfo containing the new subrun file name
    - AnalyzeSpecialEvent() is called twice: once for the ODB dump event in the old subrun file (evid 0x8001) and once for the ODB dump event in the new subrun file (evid 0x8000)

* run end:
    - call run AnalyzeSpecialEvent() methods for the ODB dump event (evid 0x8001)
    - call run PreEndRun() methods, which may generate flow events
    - if there are flow events, call run AnalyzeFlowEvent methods
    - call run EndRun() methods
    - call run destructors

* analyzer shutdown:
    - definitely do the "run end" activity (all run objects destroyed)
    - call module Finish() methods
    - call module destructors
    - return from manalyzer_main()

### The "run info" class TARunInfo

The "run info" object has the information about the currently
analyzed run - run number, file name, etc.

This object is created when a new run is started (before calling the first BeginRun() method)
and is destroyed at the end of a run (after calling the last EndRun() method).

Ownership of this object remains with the analyzer framework. User code should not
keep a pointer to it or to any of it's components. (A pointer to this object
is passed to all user methods).

When analyzing multiple subrun files, no new runinfo objects are created, but the current
file name is always updated when switching from one subrun file to the next.

TARunInfo data members:

* fRunNo - is the current run number. Special run number 0 is used when processing online events when the midas run is stopped (no run).

* fFileName - current file name. (An empty string when processing online events)

* fOdb - pointer to a VirtualOdb object. When online, it is connected to the live online ODB. When processing data files, it is connected to the last seen ODB dump event (evid 0x8000 and 0x8001). If none available, it is connected to the special EmptyOdb object (all "odb get" methods return the default values).

* fRoot - pointer to the ROOT helper object (TARootHelper, see the next section). This member only exists if manalyzer is built with HAVE_ROOT.

### The ROOT helper class TARootHelper

ROOT-related functions provided by the older analyzers via global variables
are consolidated in the ROOT helper object with improved object lifetime management.

(If manalyzer is built without HAVE_ROOT, the ROOT helper object is not available).

The root helper object is created and destroyed at the same time as the "run info" object.

Ownership of the TARootHelper object remains with the manalyzer framework, user code
should not save pointers to it or any of it's components. A pointer to this object
is always available to all user methods via "runinfo->fRoot".

TARootHelper data members:

* TFile*        fOutputFile - ROOT output file. To save ROOT histograms into the output file, do "runinfo->fRoot->fOutputFile->cd()" before calling "new TH1D & co". This file is always present and always open for writing (no need to check for NULL pointer).

* TDirectory*   fgDir - ROOT in-memory directory. To avoid saving ROOT histograms in the output file, do "runinfo->fRoot->fgDir->cd()" before calling "new TH1D & co". This directory is always present (no need to check for NULL pointer). The contents of this directory are automatically exported to the ROOT web server.

* TApplication* fgApp - pointer to ROOT graphics environement, if running in graphical mode. Value is NULL if running in batch mode (no graphics).

* THttpServer*  fgHttpServer - pointer to the ROOT web server object (-R switch).

* XmlServer*    fgXmlServer - pointer to the old XML Web server for use with the roody application (-X switch).

* TNetDirectory server (-P switch) is also available (it has no corresponding c++ object).

### manalyzer multithreaded mode and TAMultithreadHelper

Multithreading works by giving each module its own thread, passing flow events between these thread via queues.

Running with the flag --mt will enable PER-MODULE multithreading.
Multithreading configuration settings can be changed with the flags:
  
* --mtql NNN		Number of flow events to queue per module. A more events queue, the more memory will be consumed
* --mtse NNN		The number of u seconds a thread will go to sleep for if there are not flow events in its queue.
* --mtsf NNN		The number of u seconds a thread will go to sleep for if the next queue length is full


####Thread locking convention:
Each flow queue has a std::mutex lock. The queue is locked when its read, the front is popped, or data is push to the back. A thread process will be reading its own queue, and writing to the next queue.
There is a global lock for processing TAMultithreadHelper::gfLock, to be used whenever running code that isn't thread safe (ROOT fitting libraries for example).

- inside the module, AnalyzeEvent() and AnalyzeSpecialEvent() are always called in the main thread

- inside the module, AnalyzeFlowEvent() is called from a worker thread and may have to lock things:

 - the flow object and the flags object are "owned" by the worker thread, locking is not needed to modify them

 - the runinfo object is shared by all threads, anything non-thread safe, i.e. access to TARootHelper,
   direct manipulation of fFlowQueue, etc needs to be locked via TAMultithreadHelper::gfLock
 - per-module data members and global variables do not need to be locked (each module is only used by one thread at a time (by it's own thread).
 - non-thread safe libraries, i.e. ROOT, have to be locked via TAMultithreadHelper::gfLock

####How to prepare your code:
Place locks around any root fitting functions:

```cpp
{
#ifdef MODULE_MULTITHREAD
std::lock_guard<std::mutex> lock(TAMultithreadHelper::gfLock);
#endif
SomeNotThreadSafeFunctions()
} //When lock goes out of scope, gfLock is unlocked
```


Deconstructors for data put into the flow must be setup in the flow classes, not inside your modules. Do not put local variables into the flow, I recommend creading pointers and insert those.

Remember, you are putting data into the flow, the module it came from no longer has any connection to it.


Debugging advice:
* Use helgrind to analyse your program http://valgrind.org/docs/manual/hg-manual.html

* Note that you will see false race track condition warnings if you are using cout. cout is a global object so multiple print commands will collide, you probably dont care, the user side afect is text written to the screen at that line gets mashed into another print statement, maybe just reduce the verbosity of your modules...

### Module registration

TBW

### The main program

TBW

### The event dump module

TBW

### The interactive display module

TBW

### XXX

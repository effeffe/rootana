
/*! \page displayClass Display Classes

\section displayIntro Introduction

TRootanaDisplay is an attempt to provide a simple way of 
making ROOT event displays that can quickly be used to show
ADC/TDC spectrum or whatever else you'd like to plot.


\section displayUsage Using TRootanaDisplay

TRootanaDisplay is an abstract base class for event displays.  
Users need to define a class that derives from this class in order to 
make an event display.  The only method that users must implement
is the method AddAllCanvas(), where the user defines which tabs to use.

The user then needs to define how they what to update and plot histograms.
The updating of histograms happens for each event.
In online mode, the plotting of histograms only happens for each NN events, where
NN is controlled by the user.
For offline mode the plotting happens for each event.

There are three ways that users can decide to update and plot histograms:
1) They can create histograms in their event display class and then fill 
   the methods UpdateHistograms(TDataContainer) and PlotCanvas(TDataContainer).
   This histograms can then fill in the canvases that are added using 
   AddSingleCanvas(std::string name).
2) They can create classes that are derived from TCanvasHandleBase.  The derived
   classes are then added using the method AddSingleCanvas(TCanvasHandleBase* handleClass).
3) They can create a histogram-set class (derived from THistogramArrayBase) and
pass it to TFancyHistogramCanvas.  This provides a richer set of button control
of the histogram display.

There is no substantial difference between the first two methods.  The second method
is mainly meant to allow the user to separate the methods into separate files
for code cleaniness.

The actual ROOT GUI is encapsulated in a separate class TMainDisplayWindow.
The TRootanaDisplay has an instance of this TMainDisplayWindow class.
Users will be need to access the TMainDisplayWindow by calling

TRootanaDisplay::GetDisplayWindow()

in order to grab the particular canvas that we want plot on.

The program display_example.cxx shows an nice example plot using all three 
methods for histogram filling.

\section roodyComp Comparison of RootanaDisplay and Roody

rootana also provides a different way of visualizing histograms
through roody and the TNetDirectoryServer.  Should the user use TRootanaDisplay 
or roody?  The following are some of the differences between the two tools:

- roody requires two programs to be running: an analyzer that produces the histogram 
and the roody program for displaying them.  This has some advantages in that you can 
independently restart the roody part; also you can run the analyzer on the midas server
machine, but run roody on your desktop. 
But it also some disadvantages in complexity, since you need to keep track of both programs. 
- Using roody is simpler from a coding point of view, since all the visualization is 
handled for you.  The TRootanaDisplay option requires some understanding of ROOT canvases
and GUIs.
- roody can only handle a limited set of ROOT classes, like TH1 histograms.  If you 
want to be able to plot more complicated ROOT options or plot them in a more complicated
way (overlaying histograms, fits and labels) then TRootanaDisplay is a better option.

So, in summary: if you are only producing a simple set of ROOT histograms and/or
you don't want to do much programming then roody might be the right solution; if you want
complicated plots and don't mind using ROOT display classes, then TRootanaDisplay is the way
to go.

Of course, the user can easily have a program that implements both solutions 
and get the best of both worlds.


T. Lindner
*/

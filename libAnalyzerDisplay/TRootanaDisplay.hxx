#ifndef TRootanaDisplay_hxx_seen
#define TRootanaDisplay_hxx_seen


#include "TRootanaEventLoop.hxx"

#include "TCanvasHandleBase.hxx"

#include <TRootEmbeddedCanvas.h>
#include "TMainDisplayWindow.hxx"

#include "TCanvasHandleBase.hxx"

class TCanvasHandleBase;

/// This is an abstract base class for event displays.  
/// Users need to define a class that derives from this class in order to 
/// make an event display.  The only method that users must implement
/// is the method AddAllCanvas(), where the user defines which tabs to use.
/// 
/// The user then needs to define how they what to update and plot histograms.
/// The updating of histograms happens for each event.
/// In online mode, the plotting of histograms only happens for each XX events;
/// for offline mode the plotting happens for each event.
///
/// There are two ways that users can decide to update and plot histograms:
///
/// 1) They can create histograms in their event display class and then fill 
///    the methods UpdateHistograms(TDataContainer) and PlotCanvas(TDataContainer).
///    This histograms can then file in the canvases that are added using 
///    AddSingleCanvas(std::string name).
///
/// 2) They can create classes that are derived from TCanvasHandleBase.  The derived
///    classes are then added using the method AddSingleCanvas(TCanvasHandleBase* handleClass).
///    
/// There is no substantial difference between the two methods.  The second method
/// is mainly meant to allow the user to separate the methods into separate files
/// for code cleaniness.
///
/// Examples of both these methods are available in examples/display_example.cxx
///
/// The actual ROOT GUI is encapsulated in a separate class TMainDisplayWindow.
/// The TRootanaDisplay has an instance of this TMainDisplayWindow class.
/// Users will be need to access the TMainDisplayWindow by calling
///
/// TRootanaDisplay::GetDisplayWindow()
/// 
/// in order to grab the particular canvas that we want plot on.
///
/// There is also the functionality to add sub-tab groups to a 
/// particular tab, so that you can have a set of tabs of tabs.
/// To use this functionality you use the syntax
///
/// AddSingleCanvas(..., <tab name string>) 
/// 
/// to add a new tab to the top level tab named 'tab name string'.
class TRootanaDisplay: public TRootanaEventLoop {


public:

  TRootanaDisplay();

  virtual ~TRootanaDisplay();

  /// User must 
  virtual void AddAllCanvases() = 0;

  /// Add a new canvas; user will interactively fill it.
  void AddSingleCanvas(std::string name, std::string subtab_name = std::string("")){
    fMainWindow->AddCanvas(name,subtab_name);
  }

  /// Add a new canvas, using a TCanvasHandleBase class.
  /// TRootanaDisplay will take ownership of pointer
  /// and delete memory it points to.
  void AddSingleCanvas(TCanvasHandleBase* handleClass, std::string subtab_name = std::string(""));


  /// Retrieve the main display window, so that users can 
  /// do things like grab the canvases and update them.
  TMainDisplayWindow* GetDisplayWindow(){ return fMainWindow;}

  /// This method can be implemented by users to update user histograms.
  virtual void UpdateHistograms(TDataContainer& dataContainer){};
  
  /// This method can be implemented by users to plotting of current canvas
  virtual void PlotCanvas(TDataContainer& dataContainer){};

  /// This method can be implemented by users to plotting of current canvas
  virtual void ResetHistograms(){};

  /// Method for when next button is pushed (offline mode)
  void NextButtonPushed(){
    waitingForNextButton = false;
  }
  /// Method for when skip event button is pushed (online mode)
  void EventSkipButtonPushed(){
    fNumberSkipEventsOnline = fMainWindow->GetSkipEventButton()->GetNumberEntry()->GetIntNumber();
  }

  /// This method calls a couple other methods for resets the histograms.
  /// This method is attached using the ROOT signal/input system to the reset
  /// button on the canvas.
  void Reset();

  /// This is a generic action to call when some button gets pushed.
  /// Also called in regular event handling loop
  void UpdatePlotsAction();

  /// Method to call when 'quit' button is pressed.
  void QuitButtonAction();
  
  /// Function so that user can specify at outset how many events to skip before
  /// refreshing display (in online mode).
  void SetNumberSkipEvent(int number){
    fNumberSkipEventsOnline = number;
    if(fMainWindow->GetSkipEventButton())
      fMainWindow->GetSkipEventButton()->GetNumberEntry()->SetIntNumber(number);
  }

  /// Function so that user can skip how many seconds to wait in free-running mode
  void SetSecondsBeforeUpdating(double SecondsBeforeUpdating){
    fSecondsBeforeUpdating = SecondsBeforeUpdating;
  }
  
  /// Get Display name
  std::string GetDisplayName(){return fDisplayName;}
  /// Set Display name
  void SetDisplayName(std::string name){fDisplayName = name;}
  
  void Initialize(){
    InitializeMainWindow();
  }

  bool CheckOptionRAD(std::string option){
    if(option.find("-s") != std::string::npos){
      std::string sub = option.substr(2);
      fNumberSkipEventsOffline = atoi(sub.c_str());
      printf("Will process %i events before plotting first event.\n",fNumberSkipEventsOffline);
      return true;
    }
    return false;
  }
  void UsageRAD(){
    printf("\t-s: will process specified number of events before displaying (for display programs)\n");
  }

private:

  /// Method to initialize the Main display window.
  void InitializeMainWindow();

  // Variable to keep track of waiting for next event button (offline mode)
  bool waitingForNextButton; 

  /// Variable to keep track of how many events to skip before updating display;
  /// we have separate variable for online and offline modes.
  int fNumberSkipEventsOnline;

  /// Variable to keep track of how many events to skip when running offline;
  /// defined by command line argument.
  int fNumberSkipEventsOffline;

  // Variable to keep track of number of processed events.
  int fNumberProcessed;

  // Seconds to wait in free-running mode before updating
  double fSecondsBeforeUpdating;
  
  /// Flag to keep track of if quite button has been pushed.
  bool fQuitPushed;

  /// The pointer to our display window
  TMainDisplayWindow* fMainWindow;

  /// Process each midas event
  bool ProcessMidasEvent(TDataContainer& dataContainer);

    /// Process each online midas event
  bool ProcessMidasEventOnline(TDataContainer& dataContainer);
  
  /// Process each offline midas event
  bool ProcessMidasEventOffline(TDataContainer& dataContainer);
  
  /// Called before the first event of a file is read, but you should prefer
  /// Initialize() for general initialization.  This method will be called
  /// once for each input file.  
  void BeginRun(int transition,int run,int time);
  
  /// Called after the last event of a file is read, but you should prefer
  /// Finalize() for general finalization.  This method will be called once
  /// for each input file.
  void EndRun(int transition,int run,int time);
  

  /// We keep a cached copy of the midas event (so that it can used for callback).
  TDataContainer* fCachedDataContainer;

  /// Set the cached copy of midas dataContainer.
  /// !!! This is very questionable!  Caching each dataContainer might add a considerable overhead
  /// to the processing!
  void SetCachedDataContainer(TDataContainer& dataContainer){
    if(fCachedDataContainer) delete fCachedDataContainer;
    fCachedDataContainer = new TDataContainer(dataContainer);
  }

  /// Display name
  std::string fDisplayName;

  /// This is a vector of user-defined canvas handler classes.
  /// The first part of pair is the tab number.
  std::vector< std::pair<std::pair<int,int> ,TCanvasHandleBase*> > fCanvasHandlers;

  ClassDef(TRootanaDisplay,1)
}; 





#endif


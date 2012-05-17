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
///
class TRootanaDisplay: public TRootanaEventLoop {


public:

  TRootanaDisplay();

  virtual ~TRootanaDisplay() {};

  /// User must 
  virtual void AddAllCanvases() = 0;

  /// 
  void AddSingleCanvas(std::string name){
    fMainWindow->AddCanvas(name);
  }

  void AddSingleCanvas(TCanvasHandleBase* handleClass);


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
    fNumberSkipEvents = fMainWindow->GetSkipEventButton()->GetNumberEntry()->GetIntNumber();
  }

  /// This method calls a couple other methods for resets the histograms.
  /// This method is attached using the ROOT signal/input system to the reset
  /// button on the canvas.
  void Reset();

  // This is a generic action to call when some button gets pushed.
  // Also called in regular event handling loop
  void UpdatePlotsAction();

  /// Function so that user can specify at outset how many events to skip before
  /// refreshing display (in online mode).
  void SetNumberSkipEvent(int number){
    fNumberSkipEvents = number;
    if(fMainWindow->GetSkipEventButton())
      fMainWindow->GetSkipEventButton()->GetNumberEntry()->SetIntNumber(number);
  }

  /// Get Display name
  std::string GetDisplayName(){return fDisplayName;}
  /// Set Display name
  void SetDisplayName(std::string name){fDisplayName = name;}
  
private:

  // A bool to keep track of whether we have processed first event
  // (and initialize main display window).
  bool fFirstEvent;

  /// Method to initialize the Main display window.  Happens once we get to first event.
  void InitializeMainWindow();

  // Variable to keep track of waiting for next event button (offline mode)
  bool waitingForNextButton; 

  // Variable to keep track of how many events to skip before updating display (online mode)
  int fNumberSkipEvents;

  // Variable to keep track of number of processed events.
  int fNumberProcessed;

  /// The pointer to our display window
  TMainDisplayWindow* fMainWindow;

  bool ProcessMidasEvent(TDataContainer& dataContainer);

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
  std::vector< std::pair<int,TCanvasHandleBase*> > fCanvasHandlers;

  ClassDef(TRootanaDisplay,1)
}; 





#endif


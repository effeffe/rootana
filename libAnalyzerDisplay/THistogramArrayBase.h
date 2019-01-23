#ifndef THistogramArrayBase_h
#define THistogramArrayBase_h


#include <iostream>
#include <string>
#include <stdlib.h>

#include "TCanvasHandleBase.hxx"
#include "TDataContainer.hxx"
#include "TH1F.h"
#include <vector>

/// Base class for user to create an array of histograms.
/// Features of the histogram array
/// i) Histograms are all defined together.
/// ii) Histograms are updated together
/// 
/// Users of this abstract base class should implement a class
/// derived from must THistogramArrayBase that
///  1) define the histograms that you want in the constructor.
///  2) implement the UpdateHistograms(TDataContainer&) method.
///
/// Most, though not all, of logic of this histogram array is based on it being 
/// a set of similar histograms; if you use this class for
/// a set of dissimilar histograms (different binning, different quantities)
/// then the class will be less intuitive.
/// The array'ness is actually implemented as a vector of TH1s.
///
/// The default representation is to have a 1D array of histograms. 
/// But the user can also use the class a 2D array of histograms by specifying the functions
/// 
/// Grouping histograms together like this is most beneficial when using the display programs.
class THistogramArrayBase : public std::vector<TH1*> {
 public:
  THistogramArrayBase():fNumberChannelsInGroups(-1),fGroupName(""),fChannelName(""),
    fDisableAutoUpdate(false),fHasAutoUpdate(false),fSubTabName("DEFAULT"),fTabName(""),
    fUpdateWhenPlotted(false){};

  virtual ~THistogramArrayBase(){}

  /// Update the histograms for this canvas.
  virtual void UpdateHistograms(TDataContainer& dataContainer) = 0;

  /// A helper method for accessing each histogram.  Does bounds checking.
  TH1* GetHistogram(unsigned i);
  
  /// Take actions at begin run
  virtual void BeginRun(int transition,int run,int time){};

  /// Take actions at end run  
  virtual void EndRun(int transition,int run,int time){};

  /// Function to define the number of channels in group and 
  /// allow user to treat the array as 2D array.
  void SetNumberChannelsInGroup(int numberChannelsInGroups){ fNumberChannelsInGroups = numberChannelsInGroups; }
  const int  GetNumberChannelsInGroup(){ return fNumberChannelsInGroups; }
  
  /// Set name for the 'group'.
  void SetGroupName(std::string name){  fGroupName = name;  }
  const std::string GetGroupName(){ return fGroupName;  }

  /// Set name for the 'channel'.
  void SetChannelName(std::string name){  fChannelName = name;  }
  const std::string GetChannelName(){ return fChannelName;  }

  
  
  /// Define whether the histogram gets automatically updated by rootana display.
  /// 'True' means that rootana display will NOT call UpdateHistograms automatically.
  void DisableAutoUpdate(bool DisableautoUpdate=true){ fDisableAutoUpdate = DisableautoUpdate; fHasAutoUpdate = true;}  
  const bool GetDisableAutoUpdate(){ return fDisableAutoUpdate; }  
  const bool HasAutoUpdate(){ return fHasAutoUpdate; }  



  /// Get the name of the top-level tab for these plots, if running DaqDisplay.
  virtual std::string GetTabName() {
    return fTabName;
  }
  
    /// Get the name of the sub-tab for these plots, if running DaqDisplay.
  virtual std::string GetSubTabName() {
    return fSubTabName;
  }
  
  /// Get whether these histograms should be drawn by DaqDisplay::DrawCanvas()
  /// rather than DaqAnalyzer::ProcessMidasEvent()
  virtual bool IsUpdateWhenPlotted() {
    return fUpdateWhenPlotted;
  }
  
  /// If you are creating a specialized canvas (for example, showing several
  /// different plots in the same canvas) you should implement this function.
  /// If you are just creating a standard histogram canvas, you do not need
  /// to implement this function.
  virtual TCanvasHandleBase* CreateCanvas();
  
 protected:
  
  /// Set the name of the top-level tab for these plots, if running DaqDisplay.
  virtual void SetTabName(std::string name) {
    fTabName = name;
  }
  
  /// Set the name of the sub-tab for these plots, if running DaqDisplay.
  virtual void SetSubTabName(std::string name) {
    fSubTabName = name;
  }
  
  /// Set whether these histograms should be drawn by DrawCanvas()
  /// rather than AProcessMidasEvent(). This should only be set
  /// to true for histograms that only are plotting information
  /// for a single event (ie, don't set this for any histogram that is cumulative).
  virtual void SetUpdateOnlyWhenPlotted(bool whenupdate) {
    fUpdateWhenPlotted = whenupdate;
  }  
  
private:

  /// This is the number of channels in a given group.
  /// This is mostly used by rootana display, but could 
  /// also be used to specify histograms as a 2D array of [group][channel.
  int fNumberChannelsInGroups;
  
  /// The name for the 'group'.
  std::string fGroupName;

  /// The name for the 'channel'.
  std::string fChannelName;

  /// Defines whether the histogram should be automatically updated 
  /// by TRootanaDisplay.
  bool fDisableAutoUpdate;
  bool fHasAutoUpdate;

  // The tab and sub-tab name for when displaying
  std::string fSubTabName;
  std::string fTabName;

  // Some histograms should only get updated when they are being plotted
  // This is mainly for histograms that show a single event (as opposed to cumulative histograms)
  bool fUpdateWhenPlotted;
  
};

#endif

#ifndef TFancyHistogramCanvas_h
#define TFancyHistogramCanvas_h


#include <iostream>
#include <string>

#include "TH1F.h"
#include "TCanvasHandleBase.hxx"
#include "TGNumberEntry.h"
#include "TGLabel.h"
#include "THistogramArrayBase.h"
#include "TGButton.h"
#include "TGButtonGroup.h"
#include "TLegend.h"

/// A canvas that plots THistogramArrayBase type histograms in 
/// a nice way.
/// The basic idea is to allow the user to easily plot an array
/// of histograms in three different ways:
/// 1) They can scroll through the histograms one by one, using
///    the "Histogram Number" button.
/// 2) They can have a set of NNN different canvases, each 
///    with a different histogram, where NNN can be 2, 4, 8 or 16.
/// 3) They can have multiple histograms overlaid on the same
///    canvas; the user can choose how many histograms to overlay.
///
class TFancyHistogramCanvas : public TCanvasHandleBase{

public:

  /// Pass the THistogramArrayBase pointer during constructor;
  /// does not pass ownership (TFancyHistogramCanvas will not delete histoArray);
  /// parameters:
  /// 'histoArray': this is the pointer to the class derived from THistogramArrayBase base;
  ///   - does not assume ownership of pointer memory.
  /// 'name' is the name that the histogram will have.
  /// 'numberChannelsInGroups': if this value is greater than 1, then the fancy canvas
  /// will have an additional button allowing the user to specify particular groups;
  /// the histograms will be organized into size/fNumberChannelsInGroups of groups,
  /// with each group having fNumberChannelsInGroups entries.
  /// will have an additional button allowing the user to specify particular groups.
  /// 'disableAutoUpdate' will tell fancy histogram to not call histoArray->UpdateHistograms()
  ///   -> the assumption is that the user will take care of calling this function.
  ///
  TFancyHistogramCanvas(THistogramArrayBase* histoArray, 
			std::string name, int numberChannelsInGroups = -1,
			bool disableAutoUpdate=false);

  ~TFancyHistogramCanvas();

  /// Reset the histograms for this canvas
  void ResetCanvasHistograms();
  
  /// Update the histograms for this canvas.
  void UpdateCanvasHistograms(TDataContainer& dataContainer);
  
  /// Plot the histograms for this canvas
  void PlotCanvas(TDataContainer& dataContainer, TRootEmbeddedCanvas *embedCanvas);

  /// Take actions at begin run
  void BeginRun(int transition,int run,int time);

  /// Take actions at end run  
  void EndRun(int transition,int run,int time);


  void SetUpCompositeFrame(TGCompositeFrame *compFrame, TRootanaDisplay *display);
  
  /// These methods are callbacks to ensure that multi-canvas and overlay-histo modes 
  /// are used exclusively.
  void ActivateMultiCanvasButton();
  void ActivateOverlayButton();

  /// Allow the user to set explicitly the group name.
  void SetGroupName(std::string groupName);
  /// Allow the user to set explicitly the channel name.
  void SetChannelName(std::string channelName);

private:

  /// Pointer to the THistogramArrayBase class; memory is not owned by TFancyHistogramCanvas.
  THistogramArrayBase* fHistoArray;

  /// 'disableAutoUpdate' will tell fancy histogram to not call histoArray->UpdateHistograms()
  ///   -> the assumption is that the user will take care of calling this function.  
  bool fDisableAutoUpdate;

  /// 'fNumberChannelsInGroups': if this value is greater than 1, then the fancy canvas
  /// will have an additional button allowing the user to specify particular groups;
  /// the histograms will be organized into size/fNumberChannelsInGroups of groups,
  /// with each group having fNumberChannelsInGroups entries.
  int fNumberChannelsInGroups;

  /// Overall frame in which we will add buttons and widgets.
  TGHorizontalFrame *fLabelframe;

  /// Button for the histogram number.
  TGNumberEntry *fChannelCounterButton;

  /// A label for the histogram number button.
  TGLabel *fLabelChannels;

  /// Name for the channel button.
  std::string fChannelName;

  /// Button for the group number.
  TGNumberEntry *fGroupCounterButton;

  /// A label for the group button.
  TGLabel *fLabelGroup;

  /// Name for the group button.
  std::string fGroupName;

  /// This button controls whether to display mutliple sub-canvases.
  TGCheckButton *fMultiCanvasButton;

  /// Button group to select how many canvases to show.
  TGHButtonGroup *fNCanvasButtonGroup;
  TGRadioButton *fNCanvasButtons[4];

  /// This button controls whether to display overlaid histograms.
  TGCheckButton *fOverlayHistoButton;

  /// Button group to select how many histograms to show.
  TGNumberEntry *fNHistoButton;
  TGLabel *labelNHisto;
  TLegend *fNHistoLegend;

  /// Helper function to remove extra buttons if not needed anymore.
  void CheckExtraButtons();

  /// Cached pointer to rootana display; needed so that we can 
  /// create new buttons with correct callbacks.
  TRootanaDisplay *fDisplay;

  // Don't define default constructor.
  TFancyHistogramCanvas();

  // Need to rootcint this class, in order for call-backs to work.
  ClassDef(TFancyHistogramCanvas,1)
};


#endif

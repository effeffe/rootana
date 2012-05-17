#ifndef TComplicatedExampleCanvas_h
#define TComplicatedExampleCanvas_h


#include <iostream>
#include <string>

#include "TH1F.h"
#include "TCanvasHandleBase.hxx"
#include "TGNumberEntry.h"

/// A complicated canvas example, using the values from V792 module
class TComplicatedExampleCanvas : public TCanvasHandleBase{

public:
  TComplicatedExampleCanvas();

  /// Reset the histograms for this canvas
  void ResetCanvasHistograms();
  
  /// Update the histograms for this canvas.
  void UpdateCanvasHistograms(TDataContainer& dataContainer);
  
  /// Plot the histograms for this canvas
  void PlotCanvas(TDataContainer& dataContainer, TRootEmbeddedCanvas *embedCanvas);

  void SetUpCompositeFrame(TGCompositeFrame *compFrame, TRootanaDisplay *display);
  

private:
  TH1F *adcValue[32];
  TGNumberEntry *fBankCounterButton;

};


#endif

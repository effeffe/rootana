#ifndef TComplicatedExampleCanvas_h
#define TComplicatedExampleCanvas_h


#include <iostream>
#include <string>

#include "TH1F.h"
#include "TCanvasHandleBase.hxx"
#include "TGNumberEntry.h"

class TComplicatedExampleCanvas : public TCanvasHandleBase{

public:
  TComplicatedExampleCanvas();

  /// Reset the histograms for this canvas
  void ResetCanvasHistograms();
  
  /// Update the histograms for this canvas.
  void UpdateCanvasHistograms(TMidasEvent* event);
  
  /// Plot the histograms for this canvas
  void PlotCanvas(TMidasEvent* event, TRootEmbeddedCanvas *embedCanvas);

  void SetUpCompositeFrame(TGCompositeFrame *compFrame, TRootanaDisplay *display);
  

private:
  TH1F *sizeBank[4];
  TGNumberEntry *fBankCounterButton;

};


#endif

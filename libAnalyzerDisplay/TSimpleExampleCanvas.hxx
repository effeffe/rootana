#ifndef TSimpleExampleCanvas_h
#define TSimpleExampleCanvas_h


#include <iostream>
#include <string>

#include "TH1F.h"
#include "TCanvasHandleBase.hxx"


class TSimpleExampleCanvas : public TCanvasHandleBase{

public:
  TSimpleExampleCanvas();

  /// Reset the histograms for this canvas
  void ResetCanvasHistograms();
  
  /// Update the histograms for this canvas.
  void UpdateCanvasHistograms(TMidasEvent* event);
  
  /// Plot the histograms for this canvas
  void PlotCanvas(TMidasEvent* event,TRootEmbeddedCanvas *embedCanvas);

private:
  TH1F *sizeBankFR11;


};


#endif

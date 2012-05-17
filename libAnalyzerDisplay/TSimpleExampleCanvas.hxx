#ifndef TSimpleExampleCanvas_h
#define TSimpleExampleCanvas_h


#include <iostream>
#include <string>

#include "TH1F.h"
#include "TCanvasHandleBase.hxx"


/// A simple example, using V1190 data
///
///
class TSimpleExampleCanvas : public TCanvasHandleBase{

public:
  TSimpleExampleCanvas();

  /// Reset the histograms for this canvas
  void ResetCanvasHistograms();
  
  /// Update the histograms for this canvas.
  void UpdateCanvasHistograms(TDataContainer& dataContainer);
  
  /// Plot the histograms for this canvas
  void PlotCanvas(TDataContainer& dataContainer, TRootEmbeddedCanvas *embedCanvas);

private:
  TH1F *tdcHistogram;


};


#endif

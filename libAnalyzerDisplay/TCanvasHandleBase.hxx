#ifndef TCanvasHandleBase_h
#define TCanvasHandleBase_h

// Base class for user to create a set of histogram or plots 
// for a particular canvas.

#include <iostream>
#include <string>
#include <stdlib.h>

#include "TRootEmbeddedCanvas.h"
#include "TCanvas.h"
#include "TMidasEvent.h"
#include "TRootanaDisplay.hxx"

class TRootanaDisplay;

class TCanvasHandleBase{
 public:
  TCanvasHandleBase(std::string tabName){fTabName=tabName;}
  virtual ~TCanvasHandleBase(){}

  /// Reset the histograms for this canvas
  virtual void ResetCanvasHistograms() = 0;

  /// Update the histograms for this canvas.
  virtual void UpdateCanvasHistograms(TMidasEvent* event) = 0;

  /// Plot the histograms for this canvas
  virtual void PlotCanvas(TMidasEvent* event,TRootEmbeddedCanvas *embedCanvas) = 0;

  std::string GetTabName(){return fTabName;}

  /// This method is only 
  TRootEmbeddedCanvas* GetEmbeddedCanvas(){
    std::cerr<< "Not implemented for base class; exiting." << std::endl;
    exit(0);
    return 0;
  }

  ///
  virtual void SetUpCompositeFrame(TGCompositeFrame*, TRootanaDisplay *display){}

private:
  // Don't allow the user to use default constructor
  TCanvasHandleBase(){};

  std::string fTabName;

};

#endif
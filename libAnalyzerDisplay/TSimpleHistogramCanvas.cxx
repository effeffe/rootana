#include "TSimpleHistogramCanvas.hxx"

#include "TMesytecData.hxx"
#include "TH2.h"


TSimpleHistogramCanvas::TSimpleHistogramCanvas(TH1* histo, std::string name): TCanvasHandleBase(name){

  fHisto = histo;

}

TSimpleHistogramCanvas::~TSimpleHistogramCanvas(){

}


/// Reset the histograms in fHistoArray.
void TSimpleHistogramCanvas::ResetCanvasHistograms(){
  fHisto->Reset();
}
  
/// Update the histograms for this canvas.
void TSimpleHistogramCanvas::UpdateCanvasHistograms(TDataContainer& dataContainer){

}



/// Plot the histograms for this canvas
void TSimpleHistogramCanvas::PlotCanvas(TDataContainer& dataContainer, TRootEmbeddedCanvas *embedCanvas){


  TCanvas* c1 = embedCanvas->GetCanvas();
  c1->Clear();
  fHisto->Draw();
  c1->Modified();
  c1->Update();


}



/// Take actions at begin run
void TSimpleHistogramCanvas::BeginRun(int transition,int run,int time){
  fHisto->Reset();
};

/// Take actions at end run  
void TSimpleHistogramCanvas::EndRun(int transition,int run,int time){
};



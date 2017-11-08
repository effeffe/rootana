#include "TSimpleHistogramCanvas.hxx"

#include "TMesytecData.hxx"
#include "TH2.h"


TSimpleHistogramCanvas::TSimpleHistogramCanvas(TH1* histo, std::string name, std::string printoption): TCanvasHandleBase(name){

  fHisto = histo;
  fGraph = 0;
  fPrintOption = printoption;
}

TSimpleHistogramCanvas::TSimpleHistogramCanvas(TGraph* graph, std::string name): TCanvasHandleBase(name){

  fHisto = 0;
  fGraph = graph;
}



TSimpleHistogramCanvas::~TSimpleHistogramCanvas(){

  if(fHisto) delete fHisto;
  if(fGraph) delete fGraph;
  
}


/// Reset the histograms in fHistoArray.
void TSimpleHistogramCanvas::ResetCanvasHistograms(){
  if(fHisto)fHisto->Reset();
}
  
/// Update the histograms for this canvas.
void TSimpleHistogramCanvas::UpdateCanvasHistograms(TDataContainer& dataContainer){

}



/// Plot the histograms for this canvas
void TSimpleHistogramCanvas::PlotCanvas(TDataContainer& dataContainer, TRootEmbeddedCanvas *embedCanvas){


  TCanvas* c1 = embedCanvas->GetCanvas();
  c1->Clear();

  if(fHisto){
    fHisto->Draw(fPrintOption.c_str());
  }
  if(fGraph){
    fGraph->Draw("AP*");
    fGraph->SetMarkerStyle(20);
  }
  
  c1->Modified();
  c1->Update();


}



/// Take actions at begin run
void TSimpleHistogramCanvas::BeginRun(int transition,int run,int time){
  if(fHisto)fHisto->Reset();
};

/// Take actions at end run  
void TSimpleHistogramCanvas::EndRun(int transition,int run,int time){
};



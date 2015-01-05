#include "TMulticanvas.h"


TMulticanvas::TMulticanvas(std::string CanvasName): TCanvasHandleBase(CanvasName.c_str()){

  for(int i = 0; i < gMaxSubCanvasesMuCa; i++){
		fHasHistoSingle[i] = false;
		fHasHisto2D[i] = false;
		fHasGraphSingle[i] = false;
    fSummaryHistoSingle[i] = 0;
    fSummaryHisto2D[i] = 0;
    fSummaryGraphSingle[i] = 0;      
  }

};


/// Plot the histograms for this canvas for a particular index; 
/// this is the function that user must provide.
void TMulticanvas::PlotCanvas(TDataContainer& dataContainer, TRootEmbeddedCanvas *embedCanvas){
  
  TCanvas* c1 = embedCanvas->GetCanvas();
  c1->Clear();
  c1->Divide(2,2);
  for(int i = 0; i < gMaxSubCanvasesMuCa; i++){

    c1->cd(i+1);

    // Try plotting the histogram, then the graph.

    if(fHasHistoSingle[i]){
      fSummaryHistoSingle[i]->Draw();      
    }else if(fHasHisto2D[i]){
      fSummaryHisto2D[i]->Draw("COLZ");
    }else if(fHasGraphSingle[i]){
      fSummaryGraphSingle[i]->Draw("AP*");
      fSummaryGraphSingle[i]->SetMarkerStyle(20);
    }

  }

  //fSumm[index]->Draw();
  
  
  c1->Modified();
  c1->Update();

}




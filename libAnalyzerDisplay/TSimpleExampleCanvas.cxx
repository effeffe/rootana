#include "TSimpleExampleCanvas.hxx"



TSimpleExampleCanvas::TSimpleExampleCanvas(): TCanvasHandleBase("FR11"){

  sizeBankFR11 = new TH1F("sizeBankFR11","Size of FR11 bank",2000,0,10000);

}



/// Reset the histograms for this canvas
void TSimpleExampleCanvas::ResetCanvasHistograms(){

  sizeBankFR11->Reset();
}
  
/// Update the histograms for this canvas.
void TSimpleExampleCanvas::UpdateCanvasHistograms(TMidasEvent* event){

  void *ptr;
  int size = event->LocateBank(NULL, "FR11", &ptr);
  sizeBankFR11->Fill(size);

}
  
/// Plot the histograms for this canvas
void TSimpleExampleCanvas::PlotCanvas(TMidasEvent* event,TRootEmbeddedCanvas *embedCanvas){

  TCanvas* c1 = embedCanvas->GetCanvas();
  c1->Clear();
  sizeBankFR11->Draw();
  c1->Modified();
  c1->Update();
}

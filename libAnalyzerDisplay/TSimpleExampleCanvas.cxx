#include "TSimpleExampleCanvas.hxx"

#include "TV1190Data.hxx"


TSimpleExampleCanvas::TSimpleExampleCanvas(): TCanvasHandleBase("V1190 TDC Values"){

  tdcHistogram = new TH1F("tdcHistogram","Histogram of all V1190 TDC value",2000,0,100000);

}



/// Reset the histograms for this canvas
void TSimpleExampleCanvas::ResetCanvasHistograms(){

  tdcHistogram->Reset();
}
  
/// Update the histograms for this canvas.
void TSimpleExampleCanvas::UpdateCanvasHistograms(TDataContainer& dataContainer){

  TV1190Data *v1190 = dataContainer.GetEventData<TV1190Data>("TDC0");
  if(v1190){ 
    
    std::vector<TDCMeasurement>& measurements = v1190->GetMeasurements();
    for(unsigned int i = 0; i < measurements.size(); i++){
      TDCMeasurement tdcmeas = measurements[i];
      tdcHistogram->Fill(tdcmeas.GetMeasurement());      
    }	  
    
  }
  

}
  
/// Plot the histograms for this canvas
void TSimpleExampleCanvas::PlotCanvas(TDataContainer& dataContainer,TRootEmbeddedCanvas *embedCanvas){

  TCanvas* c1 = embedCanvas->GetCanvas();
  c1->Clear();
  tdcHistogram->Draw();
  c1->Modified();
  c1->Update();
}

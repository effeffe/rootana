#include "TComplicatedExampleCanvas.hxx"

//#include "TGHorizontalFrame.h"
#include "TGNumberEntry.h"
#include <TGLabel.h>

#include "TV792Data.hxx"

TComplicatedExampleCanvas::TComplicatedExampleCanvas(): TCanvasHandleBase("V792 ADC Values"){

  for(int i = 0; i < 32; i++){
    char name[100];
    char title[100];
    sprintf(name,"adcValue%i",i+10);
    sprintf(title,"ADC values for channel %i",i);
    adcValue[i] = new TH1F(name,title,2000,0,4000);
  }
}

void TComplicatedExampleCanvas::SetUpCompositeFrame(TGCompositeFrame *compFrame, TRootanaDisplay *display){


  // Now create my embedded canvas, along with the various buttons for this canvas.
  
  TGHorizontalFrame *labelframe = new TGHorizontalFrame(compFrame,200,40);
  
  fBankCounterButton = new TGNumberEntry(labelframe, 0, 9,999, TGNumberFormat::kNESInteger,
					      TGNumberFormat::kNEANonNegative, 
					      TGNumberFormat::kNELLimitMinMax,
					      0, 31);

  fBankCounterButton->Connect("ValueSet(Long_t)", "TRootanaDisplay", display, "UpdatePlotsAction()");
  fBankCounterButton->GetNumberEntry()->Connect("ReturnPressed()", "TRootanaDisplay", display, "UpdatePlotsAction()");
  labelframe->AddFrame(fBankCounterButton, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5));
  TGLabel *labelMinicrate = new TGLabel(labelframe, "ADC Channel (0-31)");
  labelframe->AddFrame(labelMinicrate, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5));

  compFrame->AddFrame(labelframe, new TGLayoutHints(kLHintsCenterX,2,2,2,2));


}



/// Reset the histograms for this canvas
void TComplicatedExampleCanvas::ResetCanvasHistograms(){

  for(int i = 0; i < 32; i++)
    adcValue[i]->Reset();
}
  
/// Update the histograms for this canvas.
void TComplicatedExampleCanvas::UpdateCanvasHistograms(TDataContainer& dataContainer){

  // Get the V792 data
  TV792Data *v792 = dataContainer.GetEventData<TV792Data>("ADC0");
  if(v792 ){ 

    // Loop over ADC measurements
    std::vector<VADCMeasurement>& measurements = v792->GetMeasurements();
    for(unsigned int i = 0; i < measurements.size(); i++){
      VADCMeasurement adcmeas = measurements[i];
	
      // For each measurement, update histogram
      int chan = adcmeas.GetChannel();
      if(chan >= 0 && chan < 32){
	adcValue[chan]->Fill(adcmeas.GetMeasurement());
      }
    }      
  }
}
  
/// Plot the histograms for this canvas
void TComplicatedExampleCanvas::PlotCanvas(TDataContainer& dataContainer, TRootEmbeddedCanvas *embedCanvas){

  TCanvas* c1 = embedCanvas->GetCanvas();
  c1->Clear();
  int whichbank = fBankCounterButton->GetNumberEntry()->GetIntNumber();
  if(whichbank >=0 && whichbank <32)
    adcValue[whichbank]->Draw();
  c1->Modified();
  c1->Update();

}

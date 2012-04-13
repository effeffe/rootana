#include "TComplicatedExampleCanvas.hxx"

//#include "TGHorizontalFrame.h"
#include "TGNumberEntry.h"
#include <TGLabel.h>


TComplicatedExampleCanvas::TComplicatedExampleCanvas(): TCanvasHandleBase("Banks sizes"){

  for(int i = 0; i < 4; i++){
    char name[100];
    char title[100];
    sprintf(name,"sizeBank%i",i+10);
    sprintf(title,"Size of FR%i bank",i+10);
    sizeBank[i] = new TH1F(name,title,2000,0,10000);
  }
}

void TComplicatedExampleCanvas::SetUpCompositeFrame(TGCompositeFrame *compFrame, TRootanaDisplay *display){


  // Now create my embedded canvas, along with the various buttons for this canvas.
  
  TGHorizontalFrame *labelframe = new TGHorizontalFrame(compFrame,200,40);
  
  fBankCounterButton = new TGNumberEntry(labelframe, 10, 9,999, TGNumberFormat::kNESInteger,
					      TGNumberFormat::kNEANonNegative, 
					      TGNumberFormat::kNELLimitMinMax,
					      10, 13);

  fBankCounterButton->Connect("ValueSet(Long_t)", "TRootanaDisplay", display, "UpdatePlotsAction()");
  fBankCounterButton->GetNumberEntry()->Connect("ReturnPressed()", "TRootanaDisplay", display, "UpdatePlotsAction()");
  labelframe->AddFrame(fBankCounterButton, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5));
  TGLabel *labelMinicrate = new TGLabel(labelframe, "Bank ( 10 to 13 )");
  labelframe->AddFrame(labelMinicrate, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5));

  compFrame->AddFrame(labelframe, new TGLayoutHints(kLHintsCenterX,2,2,2,2));


}



/// Reset the histograms for this canvas
void TComplicatedExampleCanvas::ResetCanvasHistograms(){

  for(int i = 0; i < 4; i++)
    sizeBank[i]->Reset();
}
  
/// Update the histograms for this canvas.
void TComplicatedExampleCanvas::UpdateCanvasHistograms(TMidasEvent* event){

  void *ptr;
  for(int i = 0; i < 4; i++){
    char name[100];
    sprintf(name,"FR%2i",i+10);
    int size = event->LocateBank(NULL, name, &ptr);
    sizeBank[i]->Fill(size);
  }
}
  
/// Plot the histograms for this canvas
void TComplicatedExampleCanvas::PlotCanvas(TMidasEvent* event, TRootEmbeddedCanvas *embedCanvas){

  TCanvas* c1 = embedCanvas->GetCanvas();
  c1->Clear();
  int whichbank = fBankCounterButton->GetNumberEntry()->GetIntNumber() - 10;
  if(whichbank >=0 && whichbank <4)
    sizeBank[whichbank]->Draw();
  c1->Modified();
  c1->Update();

}

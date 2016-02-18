#include "TFancyHistogramCanvas.hxx"

#include "TMesytecData.hxx"
#include "TH2.h"

ClassImp(TFancyHistogramCanvas)

TFancyHistogramCanvas::TFancyHistogramCanvas(THistogramArrayBase* histoArray, 
					     std::string name, int numberChannelsInGroups,
					     bool disableAutoUpdate):
  TCanvasHandleBase(name),
  fHistoArray(histoArray){

  if(histoArray->GetNumberChannelsInGroup() != -1){
    fNumberChannelsInGroups = histoArray->GetNumberChannelsInGroup();  
  }else if(numberChannelsInGroups > 1)
    fNumberChannelsInGroups = numberChannelsInGroups;
  else
    fNumberChannelsInGroups = -1;

  fLabelframe = 0;
  fChannelCounterButton = 0;
  fLabelChannels = 0;
  fChannelName = "Histogram";
  if(histoArray->GetChannelName().compare("")!=0)
    fChannelName = histoArray->GetChannelName();

  fGroupCounterButton = 0;
  fLabelGroup = 0;
  fGroupName = "Group";
  if(histoArray->GetGroupName().compare("")!=0)
    fGroupName = histoArray->GetGroupName();

  fMultiCanvasButton = 0;
  fNCanvasButtonGroup =0;
  fNCanvasButtons[0]=0;
  fNCanvasButtons[1]=0;
  fNCanvasButtons[2]=0;
  fNCanvasButtons[3]=0;
  fOverlayHistoButton = 0;
  fNHistoButton = 0;
  labelNHisto = 0;

  // Construct the legend here, so that user can modify position immediately.
  fNHistoLegend = new TLegend(0.6,0.6,0.89,0.8);
  fNHistoLegend->SetFillColor(10);

  fDisableAutoUpdate = disableAutoUpdate;
  if(histoArray->HasAutoUpdate())
    fDisableAutoUpdate = histoArray->GetDisableAutoUpdate();

}

TFancyHistogramCanvas::~TFancyHistogramCanvas(){

}

/// This is the complicated part, where we create a bunch of buttons and widgets.
void TFancyHistogramCanvas::SetUpCompositeFrame(TGCompositeFrame *compFrame, TRootanaDisplay *display){

  /// Cached pointer to rootana display; needed so that we can 
  /// create new buttons with correct callbacks.
  fDisplay = display;
  
  // Now create my embedded canvas, along with the various buttons for this canvas.  
  // Sub-frame, to reduce space used
  fLabelframe = new TGHorizontalFrame(compFrame,200,40);
  compFrame->AddFrame(fLabelframe, new TGLayoutHints(kLHintsCenterX,2,2,2,2));
  
  // ________________________________________________________________________________
  // Add a button for the groups, if we are using groups
  if(fNumberChannelsInGroups > 1){
    
    TGVerticalFrame *hframe1 = new TGVerticalFrame(fLabelframe, 120, 40, kFixedWidth);
    fLabelframe->AddFrame(hframe1, new TGLayoutHints(kLHintsLeft,2,2,2,2));
    int numberGroups = (int)(((double)fHistoArray->size())/((double)fNumberChannelsInGroups));

    fGroupCounterButton = new TGNumberEntry(hframe1, 0, 9,999, TGNumberFormat::kNESInteger,
					      TGNumberFormat::kNEANonNegative, 
					      TGNumberFormat::kNELLimitMinMax,
					      0, numberGroups-1);
    
    hframe1->AddFrame(fGroupCounterButton, new TGLayoutHints(kLHintsLeft, 1, 1, 1, 1));
    // Add call-backs to update plots if widget is used
    fGroupCounterButton->Connect("ValueSet(Long_t)", "TRootanaDisplay", display, "UpdatePlotsAction()");
    fGroupCounterButton->GetNumberEntry()->Connect("ReturnPressed()", "TRootanaDisplay", display, "UpdatePlotsAction()");
    
    // Add a label for channel selector    
    fLabelGroup = new TGLabel(hframe1, "");
    hframe1->AddFrame(fLabelGroup, new TGLayoutHints(kLHintsLeft , 1, 1, 1, 1));
    SetGroupName(fGroupName);
  }

  // ________________________________________________________________________________
  // Create the default set of widgets used for all display options: namely a counter
  // that keeps track of which channel to start from.

  // Sub-frame, to reduce space used
  TGVerticalFrame *hframe2 = new TGVerticalFrame(fLabelframe, 120, 40, kFixedWidth);
  fLabelframe->AddFrame(hframe2, new TGLayoutHints(kLHintsLeft,2,2,2,2));
  int numberChannels = fHistoArray->size();
  if(fNumberChannelsInGroups > 1){
    numberChannels = fNumberChannelsInGroups;    
  }
  fChannelCounterButton = new TGNumberEntry(hframe2, 0, 9,999, TGNumberFormat::kNESInteger,
					 TGNumberFormat::kNEANonNegative, 
					 TGNumberFormat::kNELLimitMinMax,
					 0, numberChannels-1);
  
  hframe2->AddFrame(fChannelCounterButton, new TGLayoutHints(kLHintsLeft, 1, 1, 1, 1));
  // Add call-backs to update plots if widget is used
  fChannelCounterButton->Connect("ValueSet(Long_t)", "TRootanaDisplay", display, "UpdatePlotsAction()");
  fChannelCounterButton->GetNumberEntry()->Connect("ReturnPressed()", "TRootanaDisplay", display, "UpdatePlotsAction()");

  // Add a label for channel selector
  fLabelChannels = new TGLabel(hframe2, "");
  hframe2->AddFrame(fLabelChannels, new TGLayoutHints(kLHintsLeft, 5, 5, 5, 5));
  SetChannelName(fChannelName);

  // ________________________________________________________________________________
  // Create a button to control whether to use multiple canvases
  fMultiCanvasButton = new TGCheckButton(fLabelframe, new TGHotString("Multi-Canvas"));
  fLabelframe->AddFrame(fMultiCanvasButton, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5));

  // Add call-backs to update plots if button is used; ensure both not all modes are simultaneously active
  fMultiCanvasButton->Connect("Toggled(Bool_t)", "TFancyHistogramCanvas", this, "ActivateMultiCanvasButton()");
  fMultiCanvasButton->Connect("Toggled(Bool_t)", "TRootanaDisplay", display, "UpdatePlotsAction()");

  /// Add the button group to control how many sub-canvases to plot.
  fNCanvasButtonGroup = new TGHButtonGroup(fLabelframe, "Number of Canvases:");
  fNCanvasButtons[0] = new TGRadioButton(fNCanvasButtonGroup, new TGHotString("2 | "),1);
  fNCanvasButtons[1] = new TGRadioButton(fNCanvasButtonGroup, new TGHotString("4 | "),2);
  fNCanvasButtons[2] = new TGRadioButton(fNCanvasButtonGroup, new TGHotString("8 | "),3);
  fNCanvasButtons[3] = new TGRadioButton(fNCanvasButtonGroup, new TGHotString("16 | "),4);
  fNCanvasButtons[0]->SetOn();
  for(int i = 0; i < 4; i++)
    fNCanvasButtons[i]->Connect("Toggled(Bool_t)", "TRootanaDisplay", fDisplay, "UpdatePlotsAction()");
  
  fNCanvasButtonGroup->SetRadioButtonExclusive(kTRUE);
  
  fNCanvasButtonGroup->Show();
  fLabelframe->AddFrame(fNCanvasButtonGroup, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5));
  fNCanvasButtonGroup->SetState(false);
  
  // ________________________________________________________________________________
  // Create a button to control whether to overlay histograms in a single canvas
  fOverlayHistoButton  = new TGCheckButton(fLabelframe, new TGHotString("Overlay-Histo"));
  fLabelframe->AddFrame(fOverlayHistoButton, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5));

  // Add call-backs to update plots if button is used; ensure both not all modes are simultaneously active
  fOverlayHistoButton->Connect("Toggled(Bool_t)", "TFancyHistogramCanvas", this, "ActivateOverlayButton()");
  fOverlayHistoButton->Connect("Toggled(Bool_t)", "TRootanaDisplay", display, "UpdatePlotsAction()");

  /// Add the button to control how many histograns to plot to plot.
  
  // Sub-frame, to reduce space used
  TGVerticalFrame *hframe3 = new TGVerticalFrame(fLabelframe, 120, 40, kFixedWidth);
  fLabelframe->AddFrame(hframe3, new TGLayoutHints(kLHintsLeft,2,2,2,2));

  fNHistoButton = new TGNumberEntry(hframe3, 2, 9,999, TGNumberFormat::kNESInteger,
				    TGNumberFormat::kNEANonNegative, 
				    TGNumberFormat::kNELLimitMinMax,
				    2, 20);
  hframe3->AddFrame(fNHistoButton, new TGLayoutHints(kLHintsLeft, 5, 5, 5, 5));
  // Add call-backs to update plots if widget is used
  fNHistoButton->Connect("ValueSet(Long_t)", "TRootanaDisplay", display, "UpdatePlotsAction()");
  fNHistoButton->GetNumberEntry()->Connect("ReturnPressed()", "TRootanaDisplay", display, "UpdatePlotsAction()");
  fNHistoButton->SetState(false);
  // Add a label
  labelNHisto = new TGLabel(hframe3, "# Histo in Canvas");
  hframe3->AddFrame(labelNHisto, new TGLayoutHints(kLHintsLeft, 5, 5, 5, 5));
   
}



/// Reset the histograms in fHistoArray.
void TFancyHistogramCanvas::ResetCanvasHistograms(){
  for(unsigned int i = 0; i < fHistoArray->size(); i++)
    (*fHistoArray)[i]->Reset();
}
  
/// Update the histograms for this canvas.
void TFancyHistogramCanvas::UpdateCanvasHistograms(TDataContainer& dataContainer){

  // If this variable is set, then skip calling the histogram updating;
  // histogram updating will happen elsewhere.
  if(fDisableAutoUpdate) return;

  fHistoArray->UpdateHistograms(dataContainer);
}


// Helper method to be able to treat TH2 histograms differently
void DrawHistogram(TH1* histo, std::string option = std::string("")){

  if(dynamic_cast<TH2*>(histo)){
    std::string fulloption = std::string("COLZ") + option;   
    histo->Draw(fulloption.c_str());
  }else{
    histo->Draw(option.c_str());
  }  
  
}

/// Plot the histograms for this canvas
void TFancyHistogramCanvas::PlotCanvas(TDataContainer& dataContainer, TRootEmbeddedCanvas *embedCanvas){


  TCanvas* c1 = embedCanvas->GetCanvas();
  c1->Clear();

  int channel = fChannelCounterButton->GetNumberEntry()->GetIntNumber();
  if(fNumberChannelsInGroups > 1)
    channel = fGroupCounterButton->GetNumberEntry()->GetIntNumber() * fNumberChannelsInGroups
      + fChannelCounterButton->GetNumberEntry()->GetIntNumber();
  
  // Choose the display pattern based on which buttons have been pushed.

  // Multiple canvas option
  if(fMultiCanvasButton->IsOn()){

    int ncanvas = 1;
    if(fNCanvasButtons[0]->IsOn()){
      c1->Divide(1,2);
      ncanvas = 2;
    }
    if(fNCanvasButtons[1]->IsOn()){
      c1->Divide(2,2);
      ncanvas = 4;
    }
    if(fNCanvasButtons[2]->IsOn()){
      c1->Divide(2,4);
      ncanvas = 8;
    }
    if(fNCanvasButtons[3]->IsOn()){
      c1->Divide(4,4);
      ncanvas = 16;
    }

    for(int i = 0; i < ncanvas; i++){
      c1->cd(i+1);
      int index = i + channel;
      if(index >=0 && index < (int)fHistoArray->size() && (*fHistoArray)[index]){
	DrawHistogram((*fHistoArray)[index]);
	(*fHistoArray)[index]->SetLineColor(1);
      }
    }

  }else if(fOverlayHistoButton->IsOn()){ // Show multiple histograms on same canvas.

    int nhisto = fNHistoButton->GetNumberEntry()->GetIntNumber();

    // Kind of annoying; need to do two loops through the data in order to check which is 
    // the maximum histogram.
    int first_channel = channel;
    int last_channel = channel + nhisto;
    if(last_channel > (int)fHistoArray->size())
      last_channel = fHistoArray->size();

    // find the maximum histogram;
    TH1 *max_histo = 0;
    for(int ichan = first_channel; ichan < last_channel; ichan++){
      if(max_histo == 0)
	max_histo = (*fHistoArray)[ichan];
      else if((*fHistoArray)[ichan]->GetMaximum() > max_histo->GetMaximum())
	max_histo = (*fHistoArray)[ichan];
      
    }

    // New loop again and draw as we go.

    DrawHistogram(max_histo);
    fNHistoLegend->Clear(); 
    char name[100];
    for(int ichan = first_channel; ichan < last_channel; ichan++){

      (*fHistoArray)[ichan]->SetLineColor(ichan-channel+1);
      sprintf(name,"Histogram # %i",ichan);
      fNHistoLegend->AddEntry((*fHistoArray)[ichan],(*fHistoArray)[ichan]->GetTitle());

      if((*fHistoArray)[ichan] == max_histo) continue;      

      DrawHistogram((*fHistoArray)[ichan],"SAME");      
    }

    fNHistoLegend->Draw("SAME");
  
  }else{ // Default: single canvas, single histogram.


    if(channel >=0 && channel < (int) fHistoArray->size() && (*fHistoArray)[channel]){

      DrawHistogram((*fHistoArray)[channel]);
      
      (*fHistoArray)[channel]->SetLineColor(1);
    }

  }
  c1->Modified();
  c1->Update();

}



/// Take actions at begin run
void TFancyHistogramCanvas::BeginRun(int transition,int run,int time){
  fHistoArray->BeginRun(transition, run, time);
};

/// Take actions at end run  
void TFancyHistogramCanvas::EndRun(int transition,int run,int time){
  fHistoArray->EndRun(transition, run, time);  
};


void TFancyHistogramCanvas::ActivateMultiCanvasButton(){
  if(fMultiCanvasButton->IsOn()){
    fOverlayHistoButton->SetDown(false); // Deactivate overlay button
    fNCanvasButtonGroup->SetState(true); // Enable the set of Ncanvas buttons
    fNHistoButton->SetState(false);
  }else{
    fNCanvasButtonGroup->SetState(false);// Disable the set of Ncanvas buttons

  }
}
void TFancyHistogramCanvas::ActivateOverlayButton(){
  if(fOverlayHistoButton->IsOn()){
    fMultiCanvasButton->SetDown(false); // Deactivate multi-canvas button
    fNCanvasButtonGroup->SetState(false); // Disable the set of Ncanvas buttons
    fNHistoButton->SetState(true);
  }else{
    fNHistoButton->SetState(false);
  }

  

}


void TFancyHistogramCanvas::SetGroupName(std::string groupName){

  fGroupName = groupName;
  
  if(!fLabelGroup) return;
  int numberGroups = (int)(((double)fHistoArray->size())/((double)fNumberChannelsInGroups));
  char tlabel[100];
  sprintf(tlabel,"%s# (0-%i)",groupName.c_str(),(int)numberGroups-1);    

  fLabelGroup->SetText(tlabel);//TGLabel
}

void TFancyHistogramCanvas::SetChannelName(std::string channelName){

  fChannelName = channelName;

  if(!fLabelChannels) return;
  int numberChannels = fHistoArray->size();
  if(fNumberChannelsInGroups > 1){
    numberChannels = fNumberChannelsInGroups;    
  }

  char tlabel[100];
  sprintf(tlabel,"%s# (0-%i)",channelName.c_str(),(int)numberChannels-1);    

  fLabelChannels->SetText(tlabel);//TGLabel


}

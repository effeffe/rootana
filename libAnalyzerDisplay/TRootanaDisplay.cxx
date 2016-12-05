#include "TRootanaDisplay.hxx"
#include "TPad.h"
#include "TSystem.h"
#include <stdlib.h>
#include "math.h"

#ifndef NO_CINT
ClassImp(TRootanaDisplay)
#endif

TRootanaDisplay::TRootanaDisplay() 
{
  fNumberSkipEventsOnline = 5; 
  fNumberSkipEventsOffline = 0;
  fNumberProcessed = 0;
  fCachedDataContainer = 0;
  fSecondsBeforeUpdating = 2.0;
  fLastUpdateTime = 0.0;

  SetDisplayName("Rootana Display");
  waitingForNextButton = true;
  waitingForNextInterestingButton = true;
  
  fUpdatingBasedSeconds = false;
  fQuitPushed = false;

  // Don't create second main window.
  DisableAutoMainWindow();

  // Don't create output ROOT files;
  // Output ROOT files will kill histograms.
  DisableRootOutput(true);

}

TRootanaDisplay::~TRootanaDisplay() {

  for(unsigned int i = 0; i < fCanvasHandlers.size(); i++)
    delete fCanvasHandlers[i].second;

};


#ifdef OS_DARWIN 
static int gMainDisplayDefaultWidth = 1100;
static int gMainDisplayDefaultHeight = 580;
#else
static int gMainDisplayDefaultWidth = 1200;
static int gMainDisplayDefaultHeight = 800;
#endif

void TRootanaDisplay::InitializeMainWindow(){

  fMainWindow = new TMainDisplayWindow(gClient->GetRoot(),gMainDisplayDefaultWidth,
                                       gMainDisplayDefaultHeight,IsOffline(),fUpdatingBasedSeconds); 
  
  // Link the a bunch of buttons in TMainWindowDisplay to functions in TRootanaDisplay.
  // This bit of ROOT magic requires that the TRootanaDisplay class get rootcint-ed.
  // It also requires that TRootanaDisplay methods be public (protected doesn't work).

  // The reset button
  fMainWindow->GetResetButton()->Connect("Clicked()", "TRootanaDisplay", this, "Reset()");

  // The tab buttons
  fMainWindow->GetTab()->Connect("Selected(Int_t)", "TRootanaDisplay", this, "UpdatePlotsAction()");

  // The quit button
  fMainWindow->GetQuitButton()->Connect("Clicked()", "TRootanaDisplay", this, "QuitButtonAction()");

  // The next button
  fMainWindow->GetNextButton()->Connect("Clicked()", "TRootanaDisplay", this, "NextButtonPushed()");

  // The next interesting button
  if(iem_t::instance()->IsEnabled())
    fMainWindow->GetNextInterestingButton()->Connect("Clicked()", "TRootanaDisplay", this, "NextInterestingButtonPushed()");

  // The event skip counter
  if(IsOnline()){
    TGNumberEntry *skipButton = fMainWindow->GetSkipEventButton();
    skipButton->Connect("ValueSet(Long_t)", "TRootanaDisplay",this, "EventSkipButtonPushed()");
    skipButton->GetNumberEntry()->Connect("ReturnPressed()", "TRootanaDisplay", this, "EventSkipButtonPushed()");
    // set the initial value of this field
    if(fUpdatingBasedSeconds){
      skipButton->GetNumberEntry()->SetIntNumber((int)fSecondsBeforeUpdating);
    }else{
      skipButton->GetNumberEntry()->SetIntNumber(fNumberSkipEventsOnline);    
    }
  }

  // Let the user add all the canvases they want.
  AddAllCanvases();

  // Check: make sure we have at least one canvas.
  if(GetDisplayWindow()->GetTab()->GetNumberOfTabs() <= 0){
    std::cerr << "Error in TRootanaDisplay: you have not created any canvases; you must create at least one canvas. Exiting. " << std::endl;
    exit(0);
    
  }
  // Now map out window.
  GetDisplayWindow()->BuildWindow();


}
 

void TRootanaDisplay::AddSingleCanvas(TCanvasHandleBase* handleClass, std::string subtab_name){
  
  std::pair<int,int> index = GetDisplayWindow()->AddCanvas(handleClass->GetTabName(),subtab_name);

  std::pair< std::pair<int,int>, TCanvasHandleBase*> tmp(index,handleClass);
  
  fCanvasHandlers.push_back(tmp);
  // Now set up the embedded canvas, if user so desires.
  TGCompositeFrame* embed = GetDisplayWindow()->GetCompositeFrame(index);//ssGetTab()->GetTabContainer(tab_index);
  handleClass->SetUpCompositeFrame(embed,this);

  // If we just created a new sub-tab, grab the tab and add
  // call-back to UpdatePlot
  if(index.second == 0){
    TGTab* tab = GetDisplayWindow()->GetSubTab(index.first);
    tab->Connect("Selected(Int_t)", "TRootanaDisplay", this, "UpdatePlotsAction()");
  }


}


/// Make separate treatment of online and offline data, since both are quite different
bool TRootanaDisplay::ProcessMidasEvent(TDataContainer& dataContainer){

  fNumberProcessed++;

  iem_t::instance()->Reset(); // Reset the interesting event manager each event.

  if(IsOnline()){
    return ProcessMidasEventOnline(dataContainer);
  }else{
    return ProcessMidasEventOffline(dataContainer);
  }
  

}


/// Handle online processing of MIDAS events.
bool TRootanaDisplay::ProcessMidasEventOnline(TDataContainer& dataContainer){

  // If processing is not paused or the next button was previously pressed, then update plots.
  if(!fMainWindow->IsDisplayPaused() || !waitingForNextButton || !waitingForNextInterestingButton){
    SetCachedDataContainer(dataContainer);
    
    // Perform any histogram updating from user code.
    UpdateHistograms(*fCachedDataContainer);
    for(unsigned int i = 0; i < fCanvasHandlers.size(); i++)
      fCanvasHandlers[i].second->UpdateCanvasHistograms(*fCachedDataContainer);       

  }

  // If processing is not paused, then just update plot and return
  if(!fMainWindow->IsDisplayPaused()){
    SetCachedDataContainer(dataContainer);
    // Do canvas updating from user code; 
    // we have two modes; we can either update after X seconds or X events
    if(fUpdatingBasedSeconds){
      struct timeval nowTime;  
      gettimeofday(&nowTime, NULL);
      
      double dnowtime = nowTime.tv_sec  + (nowTime.tv_usec)/1000000.0;
      double diffTime = dnowtime - fLastUpdateTime;
      if(diffTime > fSecondsBeforeUpdating){
        UpdatePlotsAction();
        fLastUpdateTime = dnowtime;
      }

    }else{      
      // only do plot if we have processed enough events
      if(fNumberSkipEventsOnline == 1 || fNumberProcessed % fNumberSkipEventsOnline  == 1){      
        UpdatePlotsAction();
      }
    }
    
    return true;
  }

  if(!waitingForNextButton)
    UpdatePlotsAction();

  // If we pressed the next interesting button, then check if this event was
  // interesting; if yes, then update plot and let user look at it.
  // If no, then just return (and check again for next event).
  if(!waitingForNextInterestingButton){
    if(iem_t::instance()->IsInteresting()){
      std::cout << "Found next interesting event " << std::endl;
      UpdatePlotsAction();
    }else{
      return true;
    }
  }

  // If online and paused, then keep looping till the free-flowing button or
  // next button is pushed.
  waitingForNextButton = true;
  waitingForNextInterestingButton = true;
  while(1){
    
    // Add some sleeps; otherwise program takes 100% of CPU...
    usleep(10000);

    // Break out if no longer in paused state.
    if(!fMainWindow->IsDisplayPaused()) break;    

    // Break out if next button or next interesting button pressed.
    if(!waitingForNextButton || !waitingForNextInterestingButton) break;
     
    // Check if quit button has been pushed.  See QuitButtonAction() for details
    if(fQuitPushed) break;
    
    // Resize windows, if needed.
    fMainWindow->ResetSize();
    
    // handle GUI events
    gSystem->ProcessEvents(); 
    
  }
  return true;

}



bool TRootanaDisplay::ProcessMidasEventOffline(TDataContainer& dataContainer){


  SetCachedDataContainer(dataContainer);
  
  // Perform any histogram updating from user code.
  UpdateHistograms(*fCachedDataContainer);
  for(unsigned int i = 0; i < fCanvasHandlers.size(); i++)
    fCanvasHandlers[i].second->UpdateCanvasHistograms(*fCachedDataContainer);

  // Keep skipping if we haven't processed enough
  if(fNumberSkipEventsOffline >= fNumberProcessed){
    return true;
  }

  // If we pressed the next interesting button, then check if this event was
  // interesting; if yes, then update plot and let user look at it.
  // If no, then just return (and check again for next event).
  if(!waitingForNextInterestingButton){
    if(!iem_t::instance()->IsInteresting()){
      return true;
    }else{
      std::cout << "Found next interesting event " << std::endl;
    }
  }

  UpdatePlotsAction();

  // Reset clock for next time 'FreeRunning' is pushed.
  double firstFreeRunningTime = 0.0;
  
  // If offline, then keep looping till the next event button is pushed.
  waitingForNextButton = true;
  waitingForNextInterestingButton = true;
  while(1){
    
    // Add some sleeps; otherwise program takes 100% of CPU...
    usleep(10000);

    // Break out if next button or next interesting button pressed.
    if(!waitingForNextButton || !waitingForNextInterestingButton) break;

    // In offline free-running mode, go to next event after a couple seconds.
    if(fMainWindow->IsDisplayFreeRunning()){
      struct timeval nowTime;  
      gettimeofday(&nowTime, NULL);
      
      double dnowtime = nowTime.tv_sec  + (nowTime.tv_usec)/1000000.0;
      if(fabs(firstFreeRunningTime) < 0.00000001){ // first event of free-running...
	firstFreeRunningTime = dnowtime; // ... so, start the clock
      }else{ // otherwise, check if enough seconds elapsed.
	double diffTime = dnowtime - firstFreeRunningTime;
	if(diffTime > fSecondsBeforeUpdating){
	  firstFreeRunningTime = 0.0;
	  break;
	}
      }
    }else{
      firstFreeRunningTime = 0.0;
    }

    
    // Resize windows, if needed.
    fMainWindow->ResetSize();
    
    // handle GUI events
    gSystem->ProcessEvents(); 
    
  }
  return true;

}

void TRootanaDisplay::BeginRunRAD(int transition,int run,int time){
  
  std::cout << "Begin of run " << run << " at time " << time << std::endl;
  for(unsigned int i = 0; i < fCanvasHandlers.size(); i++)    
    fCanvasHandlers[i].second->BeginRun(transition,run,time);
  UpdatePlotsAction();
}

void TRootanaDisplay::EndRunRAD(int transition,int run,int time){

  std::cout << "End of run " << run << " at time " << time << std::endl;
  for(unsigned int i = 0; i < fCanvasHandlers.size(); i++)
    fCanvasHandlers[i].second->EndRun(transition,run,time);
  UpdatePlotsAction();

}



void TRootanaDisplay::UpdatePlotsAction(){

  if(!fCachedDataContainer){
    char displayTitle[200];
    sprintf(displayTitle,"%s (): run %i (no events yet)",
	    GetDisplayName().c_str(),GetCurrentRunNumber());
    GetDisplayWindow()->GetMain()->SetWindowName(displayTitle);
    return;
  }
    
  // Execute the plotting actions from user event loop.
  PlotCanvas(*fCachedDataContainer);
  
  // See if we find a user class that describes this tab.
  std::pair<int,int> tabdex = GetDisplayWindow()->GetCurrentTabIndex();
  for(unsigned int i = 0; i < fCanvasHandlers.size(); i++){
    if(tabdex == fCanvasHandlers[i].first){
      TRootEmbeddedCanvas* embed = GetDisplayWindow()->GetCurrentEmbeddedCanvas();
      fCanvasHandlers[i].second->PlotCanvas(*fCachedDataContainer,embed);
    }
  }
    
  
  // Set the display title
  char displayTitle[200];
  if(IsOnline())
    sprintf(displayTitle,"%s (online): run %i event %i",
	    GetDisplayName().c_str(),GetCurrentRunNumber(),
	    fCachedDataContainer->GetMidasData().GetSerialNumber());
  else
    sprintf(displayTitle,"%s (offline): run %i event %i",
	    GetDisplayName().c_str(),GetCurrentRunNumber(),
	    fCachedDataContainer->GetMidasData().GetSerialNumber());
    
  GetDisplayWindow()->GetMain()->SetWindowName(displayTitle);


  // Update canvas and window sizes    
  fMainWindow->ResetSize();
  
}

void TRootanaDisplay::Reset(){
  // Call the reset functions defined in user event loop.
  ResetHistograms();
  // Call the user defined canvas classes.
  for(unsigned int i = 0; i < fCanvasHandlers.size(); i++)
      fCanvasHandlers[i].second->ResetCanvasHistograms();
  UpdatePlotsAction();
}


void TRootanaDisplay::QuitButtonAction()
{
  // If we are offline, then we close the ROOT file here.
  // If we are online then the control will return to TRootanaEventLoop::ProcessMidasOnline
  // which will take care of closing the file.

  if(!IsOnline()){
    EndRun(0,GetCurrentRunNumber(),0);
    CloseRootFile();  
  }

  // Set a flag so that we can breakout of loop if 
  // we are ONLINE and PAUSED.
  // It is odd that gApplication->Terminate(0) doesn't 
  // finish, but somehow it seems to wait for the the 
  // RootanaDisplay::ProcessMidasEvent() to finish.
  fQuitPushed = true;
  gApplication->Terminate(0);   

  // Hmm, don't quite understand this.
  // If we opened a TBrowser, we need to delete it before 
  // we close the ROOT file... hmm, ROOT...
  GetDisplayWindow()->CleanTBrowser();
    
}

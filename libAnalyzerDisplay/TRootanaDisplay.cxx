#include "TRootanaDisplay.hxx"
#include "TPad.h"
#include "TSystem.h"
#include <stdlib.h>

ClassImp(TRootanaDisplay)

TRootanaDisplay::TRootanaDisplay() 
{
  fNumberSkipEventsOnline = 1; 
  fNumberSkipEventsOffline = 0;
  fNumberProcessed = 0;
  fCachedDataContainer = 0;
  fSecondsBeforeUpdating = 2.0;
  
  SetDisplayName("Rootana Display");

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

void TRootanaDisplay::InitializeMainWindow(){

  fMainWindow = new TMainDisplayWindow(gClient->GetRoot(),1200,800,IsOffline()); 
  
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
  if(IsOffline())
    fMainWindow->GetNextButton()->Connect("Clicked()", "TRootanaDisplay", this, "NextButtonPushed()");

  // The event skip counter
  if(IsOnline()){
    TGNumberEntry *skipButton = fMainWindow->GetSkipEventButton();
    skipButton->Connect("ValueSet(Long_t)", "TRootanaDisplay",this, "EventSkipButtonPushed()");
    skipButton->GetNumberEntry()->Connect("ReturnPressed()", "TRootanaDisplay", this, "EventSkipButtonPushed()");
    fNumberSkipEventsOnline = skipButton->GetNumberEntry()->GetIntNumber();    
  }

  // Let the user add all the canvases they want.
  AddAllCanvases();

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

  if(IsOnline()){
    return ProcessMidasEventOnline(dataContainer);
  }else{
    return ProcessMidasEventOffline(dataContainer);
  }
  

}


/// Handle online processing of MIDAS events.
bool TRootanaDisplay::ProcessMidasEventOnline(TDataContainer& dataContainer){

  // If processing is not paused, then just update plot and return
  if(!fMainWindow->IsDisplayPaused()){
    SetCachedDataContainer(dataContainer);
    
    // Perform any histogram updating from user code.
    UpdateHistograms(*fCachedDataContainer);
    for(unsigned int i = 0; i < fCanvasHandlers.size(); i++)
      fCanvasHandlers[i].second->UpdateCanvasHistograms(*fCachedDataContainer);
       
    // Do canvas plotting from user code; 
    // only do plot if we have processed enough events
    if(fNumberSkipEventsOnline == 1 || fNumberProcessed % fNumberSkipEventsOnline  == 1){      
      UpdatePlotsAction();
    }
    
    return true;
  }

  UpdatePlotsAction();

  // If online and paused, then keep looping till the resume button is pushed.
  while(1){
    
    // Add some sleeps; otherwise program takes 100% of CPU...
    usleep(10000);

    // Break out if no longer in paused state.
    if(!fMainWindow->IsDisplayPaused()) break;    
      
    // Check if quit button has been pushed.  See QuitButtonAction() for details
    if(fQuitPushed) break;
    
    // Resize windows, if needed.
    fMainWindow->ResetSize();
    
    // handle GUI events
    bool result = gSystem->ProcessEvents(); 
    
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

  UpdatePlotsAction();

  // Reset clock for next time 'FreeRunning' is pushed.
  double firstFreeRunningTime = 0.0;
  
  // If offline, then keep looping till the next event button is pushed.
  waitingForNextButton = true;
  while(1){
    
    // Add some sleeps; otherwise program takes 100% of CPU...
    usleep(10000);

    // ROOT signal/slot trick; this variable will magically 
    // get changed to false once the next button is pushed.
    if(!waitingForNextButton) break;

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
    bool result = gSystem->ProcessEvents(); 
    
  }
  return true;

}

void TRootanaDisplay::BeginRun(int transition,int run,int time){
  
  std::cout << "Begin of run " << run << " at time " << time << std::endl;
  for(unsigned int i = 0; i < fCanvasHandlers.size(); i++)    
    fCanvasHandlers[i].second->BeginRun(transition,run,time);
  UpdatePlotsAction();
}

void TRootanaDisplay::EndRun(int transition,int run,int time){

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

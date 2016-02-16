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


bool TRootanaDisplay::ProcessMidasEvent(TDataContainer& dataContainer){

  fNumberProcessed++;

  // Only update histograms if we are "offline" or "online and but paused".
  // This ensures that we don't update if the user pressed 'pause' since the 
  // last event.
  if(!IsOnline() || (IsOnline() && !fMainWindow->IsDisplayPaused())){
    SetCachedDataContainer(dataContainer);
    
    // Perform any histogram updating from user code.
    UpdateHistograms(*fCachedDataContainer);
    for(unsigned int i = 0; i < fCanvasHandlers.size(); i++)
      fCanvasHandlers[i].second->UpdateCanvasHistograms(*fCachedDataContainer);
  }  

  // If processing online and if processing is not paused, then just plot and return
  if(IsOnline() && !fMainWindow->IsDisplayPaused() ){
       
    // Do canvas plotting from user code; 
    // only do plot if we have processed enough events
    if(fNumberSkipEventsOnline == 1 || fNumberProcessed % fNumberSkipEventsOnline  == 1){      
      UpdatePlotsAction();
    }
    
    return true;
  }

  // If processing offline, make sure we have skipped the right number of events.
  if(!IsOnline() && fNumberSkipEventsOffline >= fNumberProcessed){
    return true;
  }

  UpdatePlotsAction();

  // If offline, then keep looping till the next event button is pushed.
  // If online and paused, then keep looping till the resume button is pushed.
  waitingForNextButton = true;
  while(1){
    
    // Add some sleeps; otherwise program takes 100% of CPU...
    usleep(10000);

    // ROOT signal/slot trick; this variable will magically 
    // get changed to false once the next button is pushed.
    if(!waitingForNextButton) break;

    // Alternately, break out if in online mode and 
    // no longer in paused state.  Again, the state of variable
    // will be changed by ROOT signal/slot callback.
    if(IsOnline() && !fMainWindow->IsDisplayPaused()) break;    
      
    // Check if quit button has been pushed.  See QuitButtonAction() for details
    if(IsOnline() && fQuitPushed) break;
    
    // In offline free-running mode, go to next event after 5 second.
    static double firstFreeRunningTime = 0.0;
    struct timeval nowTime;  
    gettimeofday(&nowTime, NULL);
    
    double dnowtime = nowTime.tv_sec  + (nowTime.tv_usec)/1000000.0;
    if(fabs(firstFreeRunningTime) < 0.00000001){
      firstFreeRunningTime = dnowtime;
    }else{
      double diffTime = dnowtime - firstFreeRunningTime;
      if(diffTime > 2.0){
	firstFreeRunningTime = 0.0;
	break;
      }
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

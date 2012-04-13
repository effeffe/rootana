#include "TRootanaDisplay.hxx"
#include "TPad.h"
#include "TSystem.h"
#include <stdlib.h>

ClassImp(TRootanaDisplay)

TRootanaDisplay::TRootanaDisplay() 
{
  fFirstEvent = false;
  fNumberSkipEvents = 1;
  fNumberProcessed = 0;
  fCachedEvent = 0;
  SetDisplayName("Rootana Display");

  // Don't create second main window.
  DisableAutoMainWindow();
}

void TRootanaDisplay::InitializeMainWindow(){

  fMainWindow = new TMainDisplayWindow(gClient->GetRoot(),1200,800,IsOffline()); 
  
  // Link the a bunch of buttons in TMainWindowDisplay to functions in TRootanaDisplay.
  // This bit of ROOT magic requires that the TRootanaDisplay class get rootcint-ed.
  // It also requires that TRootanaDisplay methods be public (protected doesn't work).

  // The reset button
  fMainWindow->GetResetButton()->Connect("Clicked()", "TRootanaDisplay", this, "Reset()");

  // The tab buttons
  fMainWindow->GetTab()->Connect("Selected(Int_t)", "TRootanaDisplay", this, "UpdatePlotsAction()");


  // The next button
  if(IsOffline())
    fMainWindow->GetNextButton()->Connect("Clicked()", "TRootanaDisplay", this, "NextButtonPushed()");

  // The event skip counter
  if(IsOnline()){
    TGNumberEntry *skipButton = fMainWindow->GetSkipEventButton();
    skipButton->Connect("ValueSet(Long_t)", "TRootanaDisplay",this, "EventSkipButtonPushed()");
    skipButton->GetNumberEntry()->Connect("ReturnPressed()", "TRootanaDisplay", this, "EventSkipButtonPushed()");
    fNumberSkipEvents = skipButton->GetNumberEntry()->GetIntNumber();    
  }

  // Let the user add all the canvases they want.
  AddAllCanvases();

  // Now map out window.
  GetDisplayWindow()->BuildWindow();
}
 

void TRootanaDisplay::AddSingleCanvas(TCanvasHandleBase* handleClass){
  
  GetDisplayWindow()->AddCanvas(handleClass->GetTabName());
  int tab_index = GetDisplayWindow()->GetTab()->GetNumberOfTabs() - 1;
  fCanvasHandlers.push_back(std::pair<int,TCanvasHandleBase*>(tab_index,handleClass));

  // Now set up the embedded canvas, if user so desires.
  TGCompositeFrame* embed = GetDisplayWindow()->GetTab()->GetTabContainer(tab_index);
  handleClass->SetUpCompositeFrame(embed,this);
 

}


bool TRootanaDisplay::ProcessEvent(TMidasEvent& event){

  if(!fFirstEvent){
    InitializeMainWindow();
    fFirstEvent = true;
  }
  fNumberProcessed++;

  SetCachedEvent(event);

  // Perform any histogram updating from user code.
  UpdateHistograms(fCachedEvent);
  for(unsigned int i = 0; i < fCanvasHandlers.size(); i++)
    fCanvasHandlers[i].second->UpdateCanvasHistograms(fCachedEvent);
  
  /// If processing online and if processing is not paused, then just plot and return
  if(IsOnline() && !fMainWindow->IsDisplayPaused() ){
       
    // Do canvas plotting from user code; 
    // only do plot if we have processed enough events
    if(fNumberSkipEvents == 1 || fNumberProcessed % fNumberSkipEvents  == 1){
      
      UpdatePlotsAction();

    }
    
    return true;
  }

  UpdatePlotsAction();

  /// If offline, then keep looping till the next event button is pushed.
  /// If online, then keep looping till the resume button is pushed.
  waitingForNextButton = true;
  while(1){
    
    // This is crude! Need to add some sort of wait function, otherwise 
    // this will use 99% of CPU time, for no good reason...

    // ROOT signal/slot trick; this variable will magically 
    // get changed to false once the next button is pushed.
    if(!waitingForNextButton){
      break;
    }

    // Alternately, break out if in online mode and 
    // no longer in paused state.  Again, the state of variable
    // will be changed by ROOT signal/slot callback.
    if(IsOnline() && !fMainWindow->IsDisplayPaused()){
      break;
    }
      
    // Resize windows, if needed.
    fMainWindow->ResetSize();

    // handle GUI events
    bool result = gSystem->ProcessEvents(); 

  }

  return true;

}

  
void TRootanaDisplay::UpdatePlotsAction(){

  // Execute the plotting actions from user event loop.
  PlotCanvas(fCachedEvent);

  // See if we find a user class that describes this tab.
  int currentTab = GetDisplayWindow()->GetCurrentTabNumber();
  for(unsigned int i = 0; i < fCanvasHandlers.size(); i++){
    if(currentTab == fCanvasHandlers[i].first){
      TRootEmbeddedCanvas* embed = GetDisplayWindow()->GetEmbeddedCanvas(currentTab);
      fCanvasHandlers[i].second->PlotCanvas(fCachedEvent,embed);
    }
  }
  
  // Set the display title
  char displayTitle[200];
  if(IsOnline())
    sprintf(displayTitle,"%s (online): run %i event %i",
	    GetDisplayName().c_str(),GetCurrentRunNumber(),
	    fCachedEvent->GetSerialNumber());
  else
    sprintf(displayTitle,"%s (offline): run %i event %i",
	    GetDisplayName().c_str(),GetCurrentRunNumber(),
	    fCachedEvent->GetSerialNumber());
    
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

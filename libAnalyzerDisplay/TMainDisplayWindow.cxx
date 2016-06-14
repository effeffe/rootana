#include <iostream>
#include <cstdlib>

#include <TGFileDialog.h>
#include "TMainDisplayWindow.hxx"
#include <TObject.h>
#include "TInterestingEventManager.hxx"

ClassImp(TMainDisplayWindow)
static int fDefaultWidth = 1200;
static int fDefaultHeight = 800;

TMainDisplayWindow::TMainDisplayWindow(const TGWindow *p,UInt_t w,UInt_t h, bool isOffline, bool updatingBasedSeconds)
  
{
  fIsOffline = isOffline;
  fProcessingPaused = false;
  fProcessingFreeRunning = false;
  fNumberSkipEventButton = 0;
  fTBrowser = 0;
  fNextInterestingButton = 0;

  // Create a main frame
  fMain = new TGMainFrame(p,w,h);

  fTab = new TGTab(fMain);
  fTab->Resize(fTab->GetDefaultSize());
  fMain->AddFrame(fTab, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
  fTab->MoveResize(2,2,300,300);

  // _______________________________________________________________________________________
  // Set up buttons for bottom of mainframe.

  // Create a horizontal frame widget with buttons
  fHframe = new TGHorizontalFrame(fMain,200,40);


  // Set different options for bottom, depending on if using offline or online.
  if(fIsOffline){

    fNextButton = new TGTextButton(fHframe,"&Next");
    fHframe->AddFrame(fNextButton, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
      
    if(iem_t::instance()->IsEnabled()){
      fNextInterestingButton = new TGTextButton(fHframe,"&Next Interesting");
      fHframe->AddFrame(fNextInterestingButton, new TGLayoutHints(kLHintsCenterX,5,5,3,4));    
    }

  }else{

    fNumberSkipEventButton = new TGNumberEntry(fHframe, 0, 9,999, TGNumberFormat::kNESInteger,
				  TGNumberFormat::kNEANonNegative, 
				  TGNumberFormat::kNELLimitMinMax,
				  1, 100000);
    fNumberSkipEventButton->SetIntNumber(5);
    fHframe->AddFrame(fNumberSkipEventButton, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5));

    if(updatingBasedSeconds){
      TGLabel *labelEv = new TGLabel(fHframe, "Refresh each Xth second");
      fHframe->AddFrame(labelEv, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5));
    }else{
      TGLabel *labelEv = new TGLabel(fHframe, "Plot every Xth events");
      fHframe->AddFrame(labelEv, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5));
    }
  }


  // Add the histogram reset button.
  fResetButton = new TGTextButton(fHframe,"&Reset Histograms");
  fHframe->AddFrame(fResetButton, new TGLayoutHints(kLHintsCenterX,5,5,3,4));

  // Add pause button
  if(!fIsOffline){
    fPauseButton = new TGTextButton(fHframe,"&Pause updates");
    fHframe->AddFrame(fPauseButton, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
    fPauseButton->Connect("Clicked()", "TMainDisplayWindow", this, "PauseResumeButtonAction()");

    fNextButton = new TGTextButton(fHframe,"&Next");
    fHframe->AddFrame(fNextButton, new TGLayoutHints(kLHintsCenterX,5,5,3,4));    
    fNextButton->SetEnabled(false);

    if(iem_t::instance()->IsEnabled()){
      fNextInterestingButton = new TGTextButton(fHframe,"&Next Interesting");
      fHframe->AddFrame(fNextInterestingButton, new TGLayoutHints(kLHintsCenterX,5,5,3,4));    
      fNextInterestingButton->SetEnabled(false);
    }
   }

  // Add buttons to save current pad or current canvas.
  fSavePadButton = new TGTextButton(fHframe,"&Save Active Pad");
  fSavePadButton->Connect("Clicked()", "TMainDisplayWindow", this, "SavePadButtonAction()");
  fHframe->AddFrame(fSavePadButton, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
  
  fSaveCanvasButton = new TGTextButton(fHframe,"&Save Canvas");
  fSaveCanvasButton->Connect("Clicked()", "TMainDisplayWindow", this, "SaveCanvasButtonAction()");
  fHframe->AddFrame(fSaveCanvasButton, new TGLayoutHints(kLHintsCenterX,5,5,3,4));

  fOpenNewTBrowser = new TGTextButton(fHframe,"&Open TBrowser");
  fOpenNewTBrowser->Connect("Clicked()", "TMainDisplayWindow", this, "NewTBrowserButtonAction()");
  fHframe->AddFrame(fOpenNewTBrowser, new TGLayoutHints(kLHintsCenterX,5,5,3,4));

  // Disable the save buttons if online
  if(!fIsOffline){
    fSavePadButton->SetEnabled(false);
    fSaveCanvasButton->SetEnabled(false);
    fOpenNewTBrowser->SetEnabled(false);
  }  

  if(fIsOffline){
    fFreeRunningButton = new TGTextButton(fHframe,"&Free Running");
    fHframe->AddFrame(fFreeRunningButton, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
    fFreeRunningButton->Connect("Clicked()", "TMainDisplayWindow", this, "FreeRunningButtonAction()");
  }


  fQuitButton = new TGTextButton(fHframe,"&Quit");
  fHframe->AddFrame(fQuitButton, new TGLayoutHints(kLHintsCenterX,5,5,3,4));


  fMain->AddFrame(fHframe, new TGLayoutHints(kLHintsNormal,2,2,2,2));

}



/// This resizing is still not working right! 
/// You can easily get a situation where resizing doesn't work.  Argh.
void TMainDisplayWindow::ResetSize(){  

  // Resize each part of the tab.
  for(int itab = 0; itab < fTab->GetNumberOfTabs() ; itab++){    

    TGCompositeFrame* frame = fTab->GetTabContainer(itab);

    // This is awkward.  Each composite frame actually has a bunch of elements.
    // Try to resize all the elements that are TGFrameElements. 
    // This seems to work if the added frame is a TRootEmbeddedCanvas; not sure 
    // otherwise.
    TList* mylist = frame->GetList();
    
    TIter next(mylist);
    while (TObject *obj = next()){

      
      TGFrameElement *frame_element = dynamic_cast<TGFrameElement*>(obj);

      if(frame_element){
	frame_element->fFrame->Resize((frame_element->fFrame->GetDefaultSize().fWidth)
				      *(fMain->GetSize().fWidth)/(fMain->GetDefaultSize().fWidth),
				      (frame_element->fFrame->GetDefaultSize().fHeight)
				      *(fMain->GetSize().fHeight)/(fMain->GetDefaultSize().fHeight));


        TGTab *this_tab = dynamic_cast<TGTab*>(frame_element->fFrame);
        if(this_tab){
          // Resize each part of the tab.
          for(int jtab = 0; jtab < this_tab->GetNumberOfTabs() ; jtab++){    
            
            TGCompositeFrame* frame2 = this_tab->GetTabContainer(jtab);
            
            // This is awkward.  Each composite frame actually has a bunch of elements.
            // Try to resize all the elements that are TGFrameElements. 
            // This seems to work if the added frame is a TRootEmbeddedCanvas; not sure 
            // otherwise.
            TList* mylist2 = frame2->GetList();
            
            TIter next2(mylist2);
            while (TObject *obj2 = next2()){

            
              TGFrameElement *frame_element = dynamic_cast<TGFrameElement*>(obj2);
              
              if(frame_element){
                frame_element->fFrame->Resize((frame_element->fFrame->GetDefaultSize().fWidth)
                                              *(fMain->GetSize().fWidth)/(fMain->GetDefaultSize().fWidth),
                                              (frame_element->fFrame->GetDefaultSize().fHeight)
                                              *(fMain->GetSize().fHeight)/(fMain->GetDefaultSize().fHeight));
              }
            }
          }

        }
        

      }else
	std::cout << "TMainDisplayWindow::ResetSize::  Dynamic cast of obj to TGFrameElement failed." << std::endl;


    }
   

  }

}
 
TGTab* TMainDisplayWindow::GetSubTab(int index){

  TGCompositeFrame* frame = fTab->GetTabContainer(index);
  
  TList* mylist = frame->GetList();
  
  TIter next(mylist);
  while (TObject *obj = next()){    
    TGFrameElement *frame_element = dynamic_cast<TGFrameElement*>(obj);
     
    if(frame_element){
      TGTab *this_tab = dynamic_cast<TGTab*>(frame_element->fFrame);
      if(this_tab)   return this_tab;       
    }
  }
  
  return 0;
  
}

std::pair<int,int> TMainDisplayWindow::AddSingleTab(std::string name, TGTab * tab, int mainTabIndex){


  // Add a new tab element, of type TGCompositeFrame
  TGCompositeFrame *compositeFrame;
  if(tab==0)
    compositeFrame = fTab->AddTab(name.c_str());
  else
    compositeFrame = tab->AddTab(name.c_str());
  compositeFrame->SetLayoutManager(new TGVerticalLayout(compositeFrame));

  // Add a canvas within that composite frame.
  char cname[500];
  std::pair<int,int> index;
  if(tab ==0){
    sprintf(cname,"Canvas_%i",fTab->GetNumberOfTabs() - 1);
    index.first = fTab->GetNumberOfTabs() - 1;
    index.second = - 1;
  }else{
    sprintf(cname,"Canvas_%i_%i",mainTabIndex,tab->GetNumberOfTabs() - 1);
    //sprintf(cname,"Canvas_%i_%i",fTab->GetNumberOfTabs() - 1,tab->GetNumberOfTabs() - 1);
    index.first = mainTabIndex; // fTab->GetNumberOfTabs() - 1;
    index.second = tab->GetNumberOfTabs() - 1;
  }
  TRootEmbeddedCanvas *embed_canvas  = new TRootEmbeddedCanvas(cname, compositeFrame,fDefaultWidth,fDefaultHeight);
  compositeFrame->AddFrame(embed_canvas, new TGLayoutHints(kLHintsTop | kLHintsExpandX,5,5,5,0));

  return index;

}

std::pair<int,int> TMainDisplayWindow::AddCanvas(std::string subtabname, std::string tabname){

  // If we want to add tab to main tab-list, tabname is empty
  if(tabname.empty()){
    std::cout << "Adding new canvas in tab named '" << subtabname << "'" << std::endl;
    return AddSingleTab(subtabname);
  }

  // Otherwise we try to find an existing tab with that name.
  bool foundTab = false;
  TGTab *subtab = 0;
  int mainTabIndex = 0;
  for(int itab = 0; itab < fTab->GetNumberOfTabs() ; itab++){    
        
    if(GetTabName(fTab,itab).compare(tabname) == 0){
      subtab = GetSubTab(itab);    
      mainTabIndex = itab;
    }
  }
  
  if(subtab){
    std::cout << "Adding new canvas in sub-tab named '" << subtabname << "' of tab " << tabname << std::endl;    
    return AddSingleTab(subtabname,subtab,mainTabIndex);
  }

  // We didn't find an existing sub-tab; create it.
  TGCompositeFrame *compositeFrame = fTab->AddTab(tabname.c_str());
  compositeFrame->SetLayoutManager(new TGVerticalLayout(compositeFrame));
  subtab = new TGTab(compositeFrame);
  compositeFrame->AddFrame(subtab, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

  std::cout << "Creating new tab named '"<< tabname <<"'. "
            << "Creating a new canvas in sub-tab named '" << subtabname << "'" << std::endl;    
  return AddSingleTab(subtabname,subtab,fTab->GetNumberOfTabs()-1);
  
 
}

std::pair<int,int> TMainDisplayWindow::GetCurrentTabIndex(){
  std::pair<int,int> tmp;
  tmp.first = fTab->GetCurrent();
  TGTab *subtab = GetSubTab(fTab->GetCurrent());
  if(subtab)
    tmp.second = subtab->GetCurrent();
  else
    tmp.second = -1;

  return tmp;

}


void TMainDisplayWindow::BuildWindow(){

  fMain->MapSubwindows();
  // Initialize the layout algorithm
  fMain->Resize(fMain->GetDefaultSize());
  // Map main frame
  fMain->MapWindow();


}

/// Pull the embedded canvas out of the compositeframe.
TRootEmbeddedCanvas* GetTRootEmbeddedCanvasFromTGCompositeFrame(TGCompositeFrame* frame){

  if(!frame) return 0;
  
  TList* mylist = frame->GetList();
  
  TIter next(mylist);
  while (TObject *obj = next()){
    
    TGFrameElement *frame_element = dynamic_cast<TGFrameElement*>(obj);
    
    if(frame_element){
      
      TGFrame* frame = frame_element->fFrame;
      TRootEmbeddedCanvas *embed_canvas = dynamic_cast<TRootEmbeddedCanvas*>(frame);

      if(embed_canvas){
	return embed_canvas;
      }
    }    
  }
  
  return 0;

}

TGCompositeFrame* TMainDisplayWindow::GetCompositeFrame(std::pair<int,int> index){

  TGTab *subtab = GetSubTab(index.first);
  TGCompositeFrame* frame;
  if(subtab)
    frame = subtab->GetTabContainer(index.second);
  else
    frame = fTab->GetTabContainer(index.first);
  return frame;

}

TGCompositeFrame* TMainDisplayWindow::GetCurrentCompositeFrame(){
  TGTab *subtab = GetSubTab(fTab->GetCurrent());
  TGCompositeFrame* frame;
  if(subtab)
    frame = subtab->GetTabContainer(subtab->GetCurrent());
  else
    frame = fTab->GetTabContainer(fTab->GetCurrent());
  return frame;
}

TRootEmbeddedCanvas* TMainDisplayWindow::GetCurrentEmbeddedCanvas(){

  TGCompositeFrame* frame = GetCurrentCompositeFrame();
  return GetTRootEmbeddedCanvasFromTGCompositeFrame(frame);

}


TRootEmbeddedCanvas* TMainDisplayWindow::GetEmbeddedCanvas(const char *name){

  
  // Complicated.  Need to loop over all possible tabs and sub-tabs, looking for
  // first match.
  for(int itab = 0; itab < fTab->GetNumberOfTabs() ; itab++){    

    if(GetTabName(fTab, itab).compare(name) == 0){
      TGCompositeFrame* frame = fTab->GetTabContainer(itab);
      return GetTRootEmbeddedCanvasFromTGCompositeFrame(frame);
    }

    TGTab *subtab = 0;
    if(subtab = GetSubTab(itab)){
      for (int isubtab = 0; isubtab < subtab->GetNumberOfTabs() ; isubtab++){    
    
        //std::cout << itab << " " << isubtab << " Name! " << GetTabName(subtab, isubtab) << std::endl;
        if(GetTabName(subtab, isubtab).compare(name) == 0){
          TGCompositeFrame* frame = subtab->GetTabContainer(isubtab);
          return GetTRootEmbeddedCanvasFromTGCompositeFrame(frame);
        }       
      }
    }
  }

  return 0;


}





TCanvas* TMainDisplayWindow::GetCanvas(const char *name){
  TRootEmbeddedCanvas* embed = GetEmbeddedCanvas(name);
  if(embed) return embed->GetCanvas();
  
  return 0;
}

 
TGTab* TMainDisplayWindow::GetTab(std::pair<int,int> tabindex){

  TGTab *tab = 0;
  if(tabindex.second == -1)
    tab = fTab;
  else{
    tab = GetSubTab(tabindex.first);
  }

  if(!tab) return 0;  
  return tab;
  
}
std::string TMainDisplayWindow::GetTabName(TGTab *tab, int index){

  TIter nextTab(tab->GetList());
  nextTab();  // Skip the first element.  This is the overall TGCompositeFrame, not the tabbed one.

  // Get the index for the current canvas
  if(index < 0)
    index = tab->GetCurrent();
  TGFrameElement *el;
  TGTabElement *tabel = 0;
  TGCompositeFrame *comp = 0;
  // Loop over the number of tabs, until we find the right one.
  for(int i = 0; i <= index; i++){
    el = (TGFrameElement *) nextTab();
    tabel  = (TGTabElement *) el->fFrame;
    el   = (TGFrameElement *) nextTab();
    comp = (TGCompositeFrame *) el->fFrame;
  }
  if(tabel){
    return std::string(*tabel->GetText());
  }

  return std::string("not found");
}

std::string TMainDisplayWindow::GetCurrentTabName(){
 
  std::pair<int,int> tabindex = GetCurrentTabIndex();
  TGTab *tab = GetTab(tabindex);
  if(!tab || tab->GetNumberOfTabs() == 0){
    return std::string("not found");
  }
  return GetTabName(tab);


}


TMainDisplayWindow::~TMainDisplayWindow() {

  std::cout << "Destructor! " << std::endl;
  delete fMain;
  delete fTab;  

}



const char *filetypes[] = { "GIF files",    "*.gif",
			    "JPG files",     "*.jpg",
			    "PNG files",     "*.png",
                            "EPS files",   "*.eps",
                            0,               0 };

void TMainDisplayWindow::SavePadButtonAction(){
  static TString dir(".");
  TGFileInfo fi;
  fi.fFileTypes = filetypes;
  fi.fIniDir    = StrDup(dir);
  new TGFileDialog(gClient->GetRoot(), fMain, kFDSave, &fi);
  printf("Open file: %s (dir: %s)\n", fi.fFilename,
	 fi.fIniDir);

  // Resize pad while saving; to get good image resolution.fTab
  double xl = gPad->GetXlowNDC(), xh = gPad->GetXlowNDC() + gPad->GetWNDC();
  double yl = gPad->GetYlowNDC(), yh = gPad->GetYlowNDC() + gPad->GetHNDC();

  gPad->SetPad(0,0,1.0,1.0);
  gPad->SaveAs(fi.fFilename);
  gPad->SetPad(xl,yl,xh,yh);
  gPad->Update();
  
}

void TMainDisplayWindow::SaveCanvasButtonAction(){
  
  static TString dir(".");
  TGFileInfo fi;
  fi.fFileTypes = filetypes;
  fi.fIniDir    = StrDup(dir);
  new TGFileDialog(gClient->GetRoot(), fMain, kFDSave, &fi);
  printf("Open file: %s (dir: %s)\n", fi.fFilename,
	 fi.fIniDir);

  gPad->GetCanvas()->SaveAs(fi.fFilename);

}

void TMainDisplayWindow::NewTBrowserButtonAction(){
  fTBrowser = new TBrowser();
}

void TMainDisplayWindow::PauseResumeButtonAction(){

  if(fProcessingPaused){
    fProcessingPaused = false;
    fPauseButton->SetText(TString("Pause updates"));
    fSavePadButton->SetEnabled(false);
    fSaveCanvasButton->SetEnabled(false);
    fOpenNewTBrowser->SetEnabled(false);
    fNextButton->SetEnabled(false);
    if(fNextInterestingButton)fNextInterestingButton->SetEnabled(false);
 }else{
    fProcessingPaused = true;
    fPauseButton->SetText(TString("Free Running"));
    fSavePadButton->SetEnabled(true);
    fSaveCanvasButton->SetEnabled(true);
    fOpenNewTBrowser->SetEnabled(true);
    fNextButton->SetEnabled(true);
    if(fNextInterestingButton) fNextInterestingButton->SetEnabled(true);
  }
}


void TMainDisplayWindow::FreeRunningButtonAction(){

  if(fProcessingFreeRunning){
    fProcessingFreeRunning = false;
    fFreeRunningButton->SetText(TString("Free Running"));
    fSavePadButton->SetEnabled(true);
    fSaveCanvasButton->SetEnabled(true);
    fOpenNewTBrowser->SetEnabled(true);
  }else{
    fProcessingFreeRunning = true;
    fFreeRunningButton->SetText(TString("Stop FreeRun"));
    fSavePadButton->SetEnabled(false);
    fSaveCanvasButton->SetEnabled(false);
    fOpenNewTBrowser->SetEnabled(false);
  }
}


int TMainDisplayWindow::GetNumberSubTabs(int i){


  TGCompositeFrame* frame = fTab->GetTabContainer(i);
  
  // This is awkward.  Each composite frame actually has a bunch of elements,
  // one of which might be a TGTab
  TList* mylist = frame->GetList();
  
  TIter next(mylist);
  
  while (TObject *obj = next()){    
    TGFrameElement *frame_element = dynamic_cast<TGFrameElement*>(obj);    
    if(frame_element){         
      TGTab *this_tab = dynamic_cast<TGTab*>(frame_element->fFrame);
      if(this_tab){        
        return this_tab->GetNumberOfTabs();
      }
    }
  }

  // didn't find sub-tab, so return 0
  return 0;
}

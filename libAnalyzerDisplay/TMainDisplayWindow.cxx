#include <iostream>
#include <cstdlib>

#include <TGFileDialog.h>
#include "TMainDisplayWindow.hxx"
#include <TObject.h>

ClassImp(TMainDisplayWindow)

TMainDisplayWindow::TMainDisplayWindow(const TGWindow *p,UInt_t w,UInt_t h, bool isOffline)
  
{
  fIsOffline = isOffline;
  fProcessingPaused = false;
  fNumberSkipEventButton = 0;

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
      
  }else{

    fNumberSkipEventButton = new TGNumberEntry(fHframe, 0, 9,999, TGNumberFormat::kNESInteger,
				  TGNumberFormat::kNEANonNegative, 
				  TGNumberFormat::kNELLimitMinMax,
				  1, 100000);
    fNumberSkipEventButton->SetIntNumber(5);
    fHframe->AddFrame(fNumberSkipEventButton, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5));

    TGLabel *labelEv = new TGLabel(fHframe, "Plot every Xth events");
    fHframe->AddFrame(labelEv, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5));

  }


  // Add the histogram reset button.
  fResetButton = new TGTextButton(fHframe,"&Reset Histograms");
  fHframe->AddFrame(fResetButton, new TGLayoutHints(kLHintsCenterX,5,5,3,4));


  // Add buttons to save current pad or current canvas.
  fSavePadButton = new TGTextButton(fHframe,"&Save Active Pad");
  fSavePadButton->Connect("Clicked()", "TMainDisplayWindow", this, "SavePadButtonAction()");
  fHframe->AddFrame(fSavePadButton, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
  
  fSaveCanvasButton = new TGTextButton(fHframe,"&Save Canvas");
  fSaveCanvasButton->Connect("Clicked()", "TMainDisplayWindow", this, "SaveCanvasButtonAction()");
  fHframe->AddFrame(fSaveCanvasButton, new TGLayoutHints(kLHintsCenterX,5,5,3,4));

  // Disable the save buttons if online
  if(!fIsOffline){
    fSavePadButton->SetEnabled(false);
    fSaveCanvasButton->SetEnabled(false);
  }
  

  if(!fIsOffline){
    fPauseButton = new TGTextButton(fHframe,"&Pause");
    fHframe->AddFrame(fPauseButton, new TGLayoutHints(kLHintsCenterX,5,5,3,4));
    fPauseButton->Connect("Clicked()", "TMainDisplayWindow", this, "PauseResumeButtonAction()");
  }


  fQuitButton = new TGTextButton(fHframe,"&Quit");
  fHframe->AddFrame(fQuitButton, new TGLayoutHints(kLHintsCenterX,5,5,3,4));


  fMain->AddFrame(fHframe, new TGLayoutHints(kLHintsNormal,2,2,2,2));


}


static int fDefaultWidth = 1200;
static int fDefaultHeight = 800;

/// This resizing is still not working right! The secondary tabs are not properly resizing!!!
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

	//std::cout << "Frame " << frame_element->fFrame->GetDefaultSize().fWidth << " " << fMain->GetSize().fWidth << " " 
	//	  << fMain->GetDefaultSize().fWidth << " " << frame_element->fFrame->GetDefaultSize().fHeight << " " 
	//	  << fMain->GetSize().fHeight << " " << fMain->GetDefaultSize().fHeight << std::endl;
      }else
	std::cout << "TMainDisplayWindow::ResetSize::  Dynamic cast of obj to TGFrameElement failed." << std::endl;
    }
  }

}

void TMainDisplayWindow::AddCanvas(std::string name){

  // Add a new tab element, of type TGCompositeFrame
  TGCompositeFrame *compositeFrame = fTab->AddTab(name.c_str());
  compositeFrame->SetLayoutManager(new TGVerticalLayout(compositeFrame));

  // Add a canvas within that composite frame.
  char cname[500];
  sprintf(cname,"Canvas_%i",fTab->GetNumberOfTabs() - 1);
  TRootEmbeddedCanvas *embed_canvas  = new TRootEmbeddedCanvas(cname, compositeFrame,fDefaultWidth,fDefaultHeight);
  compositeFrame->AddFrame(embed_canvas, new TGLayoutHints(kLHintsTop | kLHintsExpandX,5,5,5,0));

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


TRootEmbeddedCanvas* TMainDisplayWindow::GetEmbeddedCanvas(int iTab){

  TGCompositeFrame* frame = fTab->GetTabContainer(iTab);
  return GetTRootEmbeddedCanvasFromTGCompositeFrame(frame);

}


TRootEmbeddedCanvas* TMainDisplayWindow::GetEmbeddedCanvas(const char *name){

  // First find which tab has this name.  This is awkward because the 
  // TGCompositeFrame doesn't actully store the name; only the tab element does.

  TIter nextTab(fTab->GetList());
  nextTab();  // Skip the first element.  This is the overall TGCompositeFrame, not the tabbed one.

  TGFrameElement *el;
  TGTabElement *tab = 0;
  TGCompositeFrame *comp = 0;
  while ((el = (TGFrameElement *) nextTab())) {
    tab  = (TGTabElement *) el->fFrame;
    el   = (TGFrameElement *) nextTab();
    if (*tab->GetText() == name){
      comp = (TGCompositeFrame *) el->fFrame;
    }
  }

  // Now return the embedded canvas
  return GetTRootEmbeddedCanvasFromTGCompositeFrame(comp);

}


TCanvas* TMainDisplayWindow::GetCanvas(int iTab){
  TRootEmbeddedCanvas* embed = GetEmbeddedCanvas(iTab);
  if(embed) return embed->GetCanvas();
  
  return 0;
}

TCanvas* TMainDisplayWindow::GetCanvas(const char *name){
  TRootEmbeddedCanvas* embed = GetEmbeddedCanvas(name);
  if(embed) return embed->GetCanvas();
  
  return 0;
}

std::string TMainDisplayWindow::GetCurrentTabName(){

  TIter nextTab(fTab->GetList());
  nextTab();  // Skip the first element.  This is the overall TGCompositeFrame, not the tabbed one.

  // Get the index for the current canvas
  int index = fTab->GetCurrent();
  TGFrameElement *el;
  TGTabElement *tab = 0;
  TGCompositeFrame *comp = 0;

  // Loop over the number of tabs, until we find the right one.
  for(int i = 0; i <= index; i++){
    el = (TGFrameElement *) nextTab();
    tab  = (TGTabElement *) el->fFrame;
    el   = (TGFrameElement *) nextTab();
    comp = (TGCompositeFrame *) el->fFrame;
  }
  if(tab){
    return std::string(*tab->GetText());
  }

  return std::string("not found");
}


TMainDisplayWindow::~TMainDisplayWindow() {

  delete fMain;
  delete fTab;  

}



const char *filetypes[] = { "GIF files",    "*.gif",
			    "JPG files",     "*.jpg",
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

void TMainDisplayWindow::PauseResumeButtonAction(){

  if(fProcessingPaused){
    fProcessingPaused = false;
    fPauseButton->SetText("Pause");
    fSavePadButton->SetEnabled(false);
    fSaveCanvasButton->SetEnabled(false);
  }else{
    fProcessingPaused = true;
    fPauseButton->SetText("Resume");
    fSavePadButton->SetEnabled(true);
    fSaveCanvasButton->SetEnabled(true);
  }
}



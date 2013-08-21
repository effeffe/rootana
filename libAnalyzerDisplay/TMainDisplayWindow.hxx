#ifndef _WaveformDisplayWindowRUNWINDOW_H_
#define _WaveformDisplayWindowRUNWINDOW_H_

#include <TGClient.h>
#include <TCanvas.h>
#include <TGButton.h>
#include <TGFrame.h>
#include <TRootEmbeddedCanvas.h>
#include <TGObject.h>
#include <TGTab.h>
#include <TGNumberEntry.h>
#include <TGLabel.h>
#include <TFile.h>
#include <TApplication.h>
//#include <TGTextButton.h>


class TMainDisplayWindow {
 
  private:

  
  TGMainFrame *fMain;
  TGTab *fTab;
  TGFrame *fFrame;
  TGHorizontalFrame *fHframe;

  /// Variables to keep track of state of display
  /// Are we processing offline?
  bool fIsOffline;
  /// Is processing paused?
  bool fProcessingPaused;

  /// Buttons to make plots of current pad/canvas
  TGTextButton  *fSavePadButton;
  TGTextButton  *fSaveCanvasButton;

  // Button to reset histograms.
  TGTextButton  *fNextButton;

  // Button to reset histograms.
  TGTextButton  *fResetButton;

  // Button to quit
  TGTextButton  *fQuitButton;
  
  // Button to pause updating (online)
  TGTextButton *fPauseButton;

  // Button to set how many events to skip before plotting
  TGNumberEntry *fNumberSkipEventButton;

 public:

  TGTextButton* GetResetButton(){ return fResetButton;}
  
  TGTextButton* GetNextButton(){ return fNextButton;}
  
  TGTextButton* GetQuitButton(){ return fQuitButton;}

  TGNumberEntry* GetSkipEventButton(){ return fNumberSkipEventButton;}

  TMainDisplayWindow(const TGWindow *p,UInt_t w,UInt_t h, bool isOffline);
  virtual ~TMainDisplayWindow();


  
  /// Method to call when 'save pad' button is pressed.
  void SavePadButtonAction();

  /// Method to call when 'save canvas' button is pressed.
  void SaveCanvasButtonAction();

  /// Method to call when 'pause/resume' button is pressed.
  void PauseResumeButtonAction();

  // Is the display paused?
  bool IsDisplayPaused(){
    return fProcessingPaused;
  }

  void AddCanvas(std::string name);
  
  /// This method should be called just once, after you have added all 
  /// the canvases and tabs that you want.
  void BuildWindow();

  void ResetSize();
  
  TRootEmbeddedCanvas* GetEmbeddedCanvas(int iTab);
  TRootEmbeddedCanvas* GetEmbeddedCanvas(const char *name);

  /// Get a particular canvas based on index number.
  TCanvas* GetCanvas(int iTab);
  /// Get a particular canvas based on canvas name.
  TCanvas* GetCanvas(const char *name);
  
  /// Return the index number for the current tab.
  int GetCurrentTabNumber(){return fTab->GetCurrent();}

  /// Return the canvas name for the current tab.
  std::string GetCurrentTabName();

  // Get the main window.
  TGMainFrame* GetMain(){return fMain;};
  TGTab* GetTab(){return fTab;};

  ClassDef(TMainDisplayWindow,1)
};


#endif

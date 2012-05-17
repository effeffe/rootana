/// This is an example of the a user defined event display.
/// The user then needs to define how they what to update and plot histograms.
/// The updating of histograms happens for each event.
/// In online mode, the plotting of histograms only happens for each XX events;
/// for offline mode the plotting happens for each event.
///
/// There are two ways that users can decide to update and plot histograms:
///
/// 1) They can create histograms in their event display class and then fill 
///    the methods UpdateHistograms(TMidasEvent*) and PlotCanvas(TMidasEvent*).
///    This histograms can then file in the canvases that are added using 
///    AddSingleCanvas(std::string name).
///
///    In this example this method is used for the histogram sizeBankFR10,
///    which is just a histogram of the size of the bank called "FR10".
///
/// 2) They can create classes that are derived from TCanvasHandleBase.  The derived
///    classes are then added using the method AddSingleCanvas(TCanvasHandleBase* handleClass).
///    In this example there are two classes derived from TCanvasHandleBase:
///      a) TSimpleExampleCanvas: this class just creates a tab/canvas with a
///         histogram of the size of the bank called "FR11".
///      a) TComplicatedExampleCanvas: this class creates a set of four different
///         canvases/histograms which the user can select using a ROOT number widget.
///    
/// The actual ROOT GUI is encapsulated in a separate class TMainDisplayWindow.
/// The TRootanaDisplay has an instance of this TMainDisplayWindow class.
/// Users will be need to access the TMainDisplayWindow by calling
///
/// TRootanaDisplay::GetDisplayWindow()
/// 
/// in order to grab the particular canvas that we want plot on.


#include <stdio.h>
#include <iostream>

#include "TRootanaDisplay.hxx"
#include "TH1D.h"

#include "TSimpleExampleCanvas.hxx"
#include "TComplicatedExampleCanvas.hxx"

class MyTestLoop: public TRootanaDisplay {
  
  TH1F *sizeBankFR10;
public:

  MyTestLoop() {
    
    // Initialize histograms.
    sizeBankFR10 = new TH1F("sizeBankFR10","Size of FR10 bank",2000,0,10000);
  }
  
  void AddAllCanvases(){
    // Set up tabbed canvases
    AddSingleCanvas(new TSimpleExampleCanvas());
    AddSingleCanvas(new TComplicatedExampleCanvas());
    AddSingleCanvas("FR10"); 
    SetNumberSkipEvent(20);
    SetDisplayName("Example Display");
  };

  virtual ~MyTestLoop() {};

  void ResetHistograms(){
    sizeBankFR10->Reset();
  }

  void UpdateHistograms(TDataContainer& dataContainer){
    void *ptr;
    // Update histograms
    int size = dataContainer.GetMidasData().LocateBank(NULL, "FR10", &ptr);
    sizeBankFR10->Fill(size);

  }


  void PlotCanvas(TDataContainer& dataContainer){

    if(GetDisplayWindow()->GetCurrentTabName().compare("FR10") == 0){       
      TCanvas* c1 = GetDisplayWindow()->GetCanvas("FR10");
      c1->Clear();
      sizeBankFR10->Draw();
      c1->Modified();
      c1->Update();
    }
    
  }


}; 






int main(int argc, char *argv[])
{
  MyTestLoop::CreateSingleton<MyTestLoop>();  
  return MyTestLoop::Get().ExecuteLoop(argc, argv);
}


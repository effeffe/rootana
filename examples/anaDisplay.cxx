#include <stdio.h>
#include <iostream>

#include "TRootanaDisplay.hxx"
#include "TH1D.h"
#include "TV792Data.hxx"

#include "TFancyHistogramCanvas.hxx"

#define USE_V792
#define USE_V1190


#ifdef  USE_V792
#include "TV792Histogram.h"
#endif 

#ifdef  USE_V1190
#include "TV1190Histogram.h"
#endif 



class MyTestLoop: public TRootanaDisplay {
  
#ifdef  USE_V792
  TV792Histograms *v792_histos;
#endif 
#ifdef  USE_V1190
  TV1190Histograms *v1190_histos;
#endif 

public:

  MyTestLoop() {

    DisableRootOutput(false);
    
#ifdef  USE_V792
    v792_histos = new TV792Histograms();
#endif 
#ifdef  USE_V1190
    v1190_histos = new TV1190Histograms();
#endif 
  }
  

  void AddAllCanvases(){

    SetNumberSkipEvent(1);
    // Set up tabbed canvases
#ifdef  USE_V792
    TFancyHistogramCanvas* v792_all = new TFancyHistogramCanvas(v792_histos,"V792");
    AddSingleCanvas(v792_all);
#endif 
#ifdef  USE_V1190
    TFancyHistogramCanvas* v1190_all = new TFancyHistogramCanvas(v1190_histos,"V1190");
    AddSingleCanvas(v1190_all);
#endif 


    SetDisplayName("Example Display");
  };

  virtual ~MyTestLoop() {};


  void BeginRun(int transition,int run,int time){

#ifdef  USE_V792
    v792_histos->BeginRun(transition,run,time);
#endif 
#ifdef  USE_V1190
    v1190_histos->BeginRun(transition,run,time);
#endif 

  }

  void ResetHistograms(){}

  void UpdateHistograms(TDataContainer& dataContainer){

  }

  void PlotCanvas(TDataContainer& dataContainer){}


}; 






int main(int argc, char *argv[])
{
  MyTestLoop::CreateSingleton<MyTestLoop>();  
  return MyTestLoop::Get().ExecuteLoop(argc, argv);
}


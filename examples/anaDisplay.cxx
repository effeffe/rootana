#include <stdio.h>
#include <iostream>

#include "TRootanaDisplay.hxx"
#include "TH1D.h"
#include "TV792Data.hxx"

#include "TFancyHistogramCanvas.hxx"

//#define USE_V792
//#define USE_V1190
//#define USE_L2249
//#define USE_AGILENT
#define USE_V1720
//#define USE_V1730DPP
#define USE_V1730RAW

#ifdef  USE_V792
#include "TV792Histogram.h"
#endif 
#ifdef  USE_V1190
#include "TV1190Histogram.h"
#endif 
#ifdef  USE_L2249
#include "TL2249Histogram.h"
#endif 
#ifdef  USE_AGILENT
#include "TAgilentHistogram.h"
#endif 
#ifdef  USE_V1720
#include "TV1720Waveform.h"
#endif 
#ifdef  USE_V1730DPP
#include "TV1730DppWaveform.h"
#endif 
#ifdef  USE_V1730RAW
#include "TV1730RawWaveform.h"
#endif 



class MyTestLoop: public TRootanaDisplay { 

public:

  MyTestLoop() {
    SetOutputFilename("example_output");
    DisableRootOutput(false);
  }

  void AddAllCanvases(){

    SetNumberSkipEvent(1);
    // Set up tabbed canvases
#ifdef  USE_V792
    TFancyHistogramCanvas* v792_all = new TFancyHistogramCanvas(new TV792Histograms(),"V792");
    AddSingleCanvas(v792_all);
#endif 
#ifdef  USE_V1190
    TFancyHistogramCanvas* v1190_all = new TFancyHistogramCanvas(new TV1190Histograms(),"V1190");
    AddSingleCanvas(v1190_all);
#endif 
#ifdef  USE_L2249
   TFancyHistogramCanvas* l2249_all = new TFancyHistogramCanvas(new TL2249Histograms(),"L2249");
    AddSingleCanvas(l2249_all);
#endif 
#ifdef  USE_AGILENT
   TFancyHistogramCanvas* agilent_all = new TFancyHistogramCanvas(new TAgilentHistograms(),"AGILENT");
    AddSingleCanvas(agilent_all);
#endif 

#ifdef  USE_V1720
		TFancyHistogramCanvas* v1720_waveform = new TFancyHistogramCanvas(new TV1720Waveform(),"V1720 Waveforms");
    AddSingleCanvas(v1720_waveform);
#endif 

#ifdef  USE_V1730DPP
		TFancyHistogramCanvas* v1730_dpp_waveform = new TFancyHistogramCanvas(new TV1730DppWaveform(),"V1730 Waveforms");
    AddSingleCanvas(v1730_dpp_waveform);
#endif 

#ifdef  USE_V1730RAW
		std::cout << "Adding V1730 Raw waveforms" << std::endl;
		TFancyHistogramCanvas* v1730_raw_waveform = new TFancyHistogramCanvas(new TV1730RawWaveform(),"V1730 Waveforms");
    AddSingleCanvas(v1730_raw_waveform);
#endif 


    SetDisplayName("Example Display");
  };

  virtual ~MyTestLoop() {};

  void ResetHistograms(){}

  void UpdateHistograms(TDataContainer& dataContainer){}

  void PlotCanvas(TDataContainer& dataContainer){}


}; 






int main(int argc, char *argv[])
{
  MyTestLoop::CreateSingleton<MyTestLoop>();  
  return MyTestLoop::Get().ExecuteLoop(argc, argv);
}


// Default program for dealing with various standard TRIUMF VME setups:
// V792, V1190 (VME), L2249 (CAMAC), Agilent current meter
//
//

#include <stdio.h>
#include <iostream>
#include <time.h>

#include "TRootanaEventLoop.hxx"

#define USE_V792
#define USE_V1190
//#define USE_L2249
//#define USE_AGILENT

#ifdef  USE_V792
#include "TV792Histogram.h"
#endif 
#ifdef  USE_V1190
#include "TV1190Histogram.h"
#endif 

#ifdef  USE_L2249
#include <TL2249Data.hxx>
#include "TL2249Histogram.h"
#endif 

#ifdef USE_AGILENT
#include "TAgilentHistogram.h"
#endif

class Analyzer: public TRootanaEventLoop {

#ifdef  USE_V792
  TV792Histograms *v792_histos;
#endif 
#ifdef  USE_V1190
  TV1190Histograms *v1190_histos;
#endif 
#ifdef  USE_L2249
  TL2249Histograms *l2249_histos;
#endif 
#ifdef USE_AGILENT
  TAgilentHistograms *agilent_histos;
#endif


public:

  Analyzer() {
    DisableAutoMainWindow();
    
    // Create histograms (if enabled)

#ifdef  USE_V792
    v792_histos = new TV792Histograms();
#endif 
#ifdef  USE_V1190
    v1190_histos = new TV1190Histograms();
#endif 
#ifdef  USE_L2249
    l2249_histos = new TL2249Histograms();
#endif 
#ifdef USE_AGILENT
    agilent_histos = new TAgilentHistograms();
#endif

  };

  virtual ~Analyzer() {};

  void Initialize(){


  }


  void BeginRun(int transition,int run,int time){

    // Begin of run calls...

#ifdef  USE_V792
    v792_histos->BeginRun(transition,run,time);
#endif 
#ifdef  USE_V1190
    v1190_histos->BeginRun(transition,run,time);
#endif 
#ifdef  USE_L2249
    l2249_histos->BeginRun(transition,run,time);
#endif 
#ifdef USE_AGILENT
    agilent_histos->BeginRun(transition,run,time);
#endif

  }


  bool ProcessMidasEvent(TDataContainer& dataContainer){

    // actually update histograms

#ifdef  USE_V792
    v792_histos->UpdateHistograms(dataContainer);
#endif 
#ifdef  USE_V1190
    v1190_histos->UpdateHistograms(dataContainer);
#endif 
#ifdef  USE_L2249
    l2249_histos->UpdateHistograms(dataContainer);
#endif 
#ifdef USE_AGILENT
    agilent_histos->UpdateHistograms(dataContainer);
#endif

    return true;
  }


}; 


int main(int argc, char *argv[])
{

  Analyzer::CreateSingleton<Analyzer>();
  return Analyzer::Get().ExecuteLoop(argc, argv);

}


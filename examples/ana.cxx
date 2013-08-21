// Default program for dealing with various standard TRIUMF VME setups:
// V792, V1190
//
//

#include <stdio.h>
#include <iostream>
#include <time.h>

#include "TRootanaEventLoop.hxx"

#define USE_V792
#define USE_V1190

#ifdef  USE_V792
#include "TV792Histogram.h"
#endif 
#ifdef  USE_V1190
#include "TV1190Histogram.h"
#endif 


class Analyzer: public TRootanaEventLoop {

#ifdef  USE_V792
  TV792Histograms *v792_histos;
#endif 
#ifdef  USE_V1190
  TV1190Histograms *v1190_histos;
#endif 


public:

  Analyzer() {

#ifdef  USE_V792
    v792_histos = new TV792Histograms();
#endif 
#ifdef  USE_V1190
    v1190_histos = new TV1190Histograms();
#endif 


  };

  virtual ~Analyzer() {};

  void Initialize(){


  }


  void BeginRun(int transition,int run,int time){

#ifdef  USE_V792
    v792_histos->BeginRun(transition,run,time);
#endif 
#ifdef  USE_V1190
    v1190_histos->BeginRun(transition,run,time);
#endif 

  }


  bool ProcessMidasEvent(TDataContainer& dataContainer){

#ifdef  USE_V792
    v792_histos->UpdateHistograms(dataContainer);
#endif 
#ifdef  USE_V1190
    v1190_histos->UpdateHistograms(dataContainer);
#endif 
    return true;
  }


}; 


int main(int argc, char *argv[])
{

  Analyzer::CreateSingleton<Analyzer>();
  return Analyzer::Get().ExecuteLoop(argc, argv);

}


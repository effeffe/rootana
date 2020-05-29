#include "THistogramArrayBase.h"
#include "TFancyHistogramCanvas.hxx"


THistogramArrayBase::~THistogramArrayBase(){
  for(int i = size()-1; i >= 0 ; i--){
    delete (*this)[i];
    //delete tmp;
  }  
}


TH1* THistogramArrayBase::GetHistogram(unsigned i){
  if(i >= size()){
    std::cerr << "Invalid index (=" << i 
              << ") requested in THistogramArrayBase::GetHistogram(int i) " << std::endl;
    return 0;
  }
  return (*this)[i];
}

TCanvasHandleBase* THistogramArrayBase::CreateCanvas() {
  return new TFancyHistogramCanvas(this, fSubTabName);
}

  
  


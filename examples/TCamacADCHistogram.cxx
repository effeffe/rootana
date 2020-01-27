#include "TCamacADCHistogram.h"

#include "TCamacADCData.hxx"
#include "TDirectory.h"

const int Nchannels = 12;

/// Reset the histograms for this canvas
TCamacADCHistograms::TCamacADCHistograms(){  
  
  SetSubTabName("Camac");
  CreateHistograms();
}


void TCamacADCHistograms::CreateHistograms(){
  
  // Otherwise make histograms
  clear();
  
  for(int i = 0; i < Nchannels; i++){ // loop over channels    

    char name[100];
    char title[100];
    sprintf(name,"CamacADC_%i_%i",0,i);

    // Delete old histograms, if we already have them
    TH1D *old = (TH1D*)gDirectory->Get(name);
    if (old){
      delete old;
    }

    // Create new histograms
    
    sprintf(title,"CAMAC ADC for channel=%i",i);	
    
    TH1D *tmp = new TH1D(name,title,2100,0,2100);
    tmp->SetYTitle("ADC value");
    tmp->SetXTitle("Bin");
    push_back(tmp);
  }

}



  
/// Update the histograms for this canvas.
void TCamacADCHistograms::UpdateHistograms(TDataContainer& dataContainer){

  TCamacData *data = dataContainer.GetEventData<TCamacData>("ADC8");
  if(!data) return;

  /// Get the Vector of ADC Measurements.
  std::vector<CamacADCEvent> measurements = data->GetMeasurements();
  for(unsigned int i = 0; i < measurements.size(); i++){ // loop over measurements
	
    // For each measurement, fill the ADC values for each channel.
    for(unsigned int ch = 0; ch < 12; ch++){
      GetHistogram(ch)->Fill(measurements[i].GetADC(ch));
    }
  }
}



/// Take actions at begin run
void TCamacADCHistograms::BeginRun(int transition,int run,int time){

  CreateHistograms();

}

/// Take actions at end run  
void TCamacADCHistograms::EndRun(int transition,int run,int time){

}

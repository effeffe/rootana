#include "TTRB3Histogram.hxx"

#include "TTRB3Data.hxx"
#include "TDirectory.h"

const int NchannelPerFpga = 32; /// what is right value?
const int Nfpga = 4;

/// Reset the histograms for this canvas
TTRB3Histograms::TTRB3Histograms(){  

  SetGroupName("FPGA");
  SetNumberChannelsInGroup(NchannelPerFpga);
  CreateHistograms();
}


void TTRB3Histograms::CreateHistograms(){
  

  // Otherwise make histograms
  clear();
  
  std::cout << "Create Histos" << std::endl;
  for(int j = 0; j < Nfpga; j++){ // loop over FPGA    
    for(int i = 0; i < NchannelPerFpga; i++){ // loop over channels    
      
      char name[100];
      char title[100];
      sprintf(name,"TRB3_%i_%i",j,i);
      
      // Delete old histograms, if we already have them
      TH1D *old = (TH1D*)gDirectory->Get(name);
      if (old){
        delete old;
      }
      
      
      // Create new histograms
      
      sprintf(title,"TRB3 histogram for fpga=%i channel=%i",j, i);	
      
      TH1D *tmp = new TH1D(name,title,50000,0,10000);
      tmp->SetXTitle("Hit time (ns)");
      tmp->SetYTitle("Number of Entries");
      push_back(tmp);
    }
  }

}



  
/// Update the histograms for this canvas.
void TTRB3Histograms::UpdateHistograms(TDataContainer& dataContainer){

  TTRB3Data *data = dataContainer.GetEventData<TTRB3Data>("TRB0");
  if(data){
    //data->Print();
    
    double reftime = 0;
    for(int i = 0; i < data->GetNumberMeasurements(); i++){
      std::vector<TrbTdcMeas> meas =  data->GetMeasurements();
      double time = meas[i].GetFinalTime();
      uint32_t id = meas[i].GetBoardId();
      uint32_t ch = meas[i].GetChannel();
      int hch = NchannelPerFpga*id + ch;
      GetHistogram(hch)->Fill(time);

    }

  }    

}



/// Take actions at begin run
void TTRB3Histograms::BeginRun(int transition,int run,int time){

  CreateHistograms();

}

/// Take actions at end run  
void TTRB3Histograms::EndRun(int transition,int run,int time){

}


/// Reset the histograms for this canvas
TTRB3DiffHistograms::TTRB3DiffHistograms(){  

  SetGroupName("FPGA");
  SetNumberChannelsInGroup(NchannelPerFpga);
  fReferenceFpga = 0; 
  fReferenceCh = 1;
  
  CreateHistograms();
}


void TTRB3DiffHistograms::CreateHistograms(){
  

  // Otherwise make histograms
  clear();
  
  std::cout << "Create Histos" << std::endl;
  for(int j = 0; j < Nfpga; j++){ // loop over FPGA    
    for(int i = 0; i < NchannelPerFpga; i++){ // loop over channels    
      
      char name[100];
      char title[100];
      sprintf(name,"TRB3_diff_%i_%i",j,i);
      
      // Delete old histograms, if we already have them
      TH1D *old = (TH1D*)gDirectory->Get(name);
      if (old){
        delete old;
      }
      
      
      // Create new histograms
      
      sprintf(title,"TRB3 difference histogram for fpga=%i channel=%i",j, i);	
      
      TH1D *tmp = new TH1D(name,title,20000,-109.4,109.4);
      tmp->SetXTitle("Hit time (ns)");
      tmp->SetYTitle("Number of Entries");
      push_back(tmp);
    }
  }

}



  
/// Update the histograms for this canvas.
void TTRB3DiffHistograms::UpdateHistograms(TDataContainer& dataContainer){

  TTRB3Data *data = dataContainer.GetEventData<TTRB3Data>("TRB0");
  if(data){
    //data->Print();

    // Loop over first time to find the reference time
    double reftime = 0;
    bool foundRef = false;
    
    for(int i = 0; i < data->GetNumberMeasurements(); i++){
      std::vector<TrbTdcMeas> meas =  data->GetMeasurements();
      double time = meas[i].GetFinalTime();
      uint32_t id = meas[i].GetBoardId();
      uint32_t ch = meas[i].GetChannel();
      if(fReferenceFpga == id && fReferenceCh == ch){
        reftime = time;
        foundRef = true;
      }

    }

    if(foundRef){

      for(int i = 0; i < data->GetNumberMeasurements(); i++){
        std::vector<TrbTdcMeas> meas =  data->GetMeasurements();
        double time = meas[i].GetFinalTime();
        uint32_t id = meas[i].GetBoardId();
        uint32_t ch = meas[i].GetChannel();
        int hch = NchannelPerFpga*id + ch;
        GetHistogram(hch)->Fill(time-reftime);
      }
    }
    
  }    

}



/// Take actions at begin run
void TTRB3DiffHistograms::BeginRun(int transition,int run,int time){

  CreateHistograms();

}

/// Take actions at end run  
void TTRB3DiffHistograms::EndRun(int transition,int run,int time){

}

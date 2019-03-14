#include "TAnaManager.hxx"
#include "TV1720RawData.h"
#include "TRB3Decoder.hxx"

TAnaManager::TAnaManager(){

#ifdef USE_V792
  AddHistogram(new TV792Histograms());
#endif

#ifdef USE_V1190
  AddHistogram(new TV1190Histograms());
#endif
  
#ifdef USE_L2249
  AddHistogram(new TL2249Histograms());
#endif
  
#ifdef USE_AGILENT
  AddHistogram(new TAgilentHistograms());
#endif

#ifdef USE_V1720
    
    AddHistogram(new TV1720Waveform());
  
  fV1720PHCompare = new TH2F("V1720PHCompare","Pulse Height: Channel 1 vs 0",100,300,700,100,300,700);
  fV1720PHCompare->SetXTitle("Channel 0");
  fV1720PHCompare->SetYTitle("Channel 1");
  fV1720TimeCompare = new TH2F("V1720TimeCompare","Time: Channel 1 vs 0",100,0,2000,100,0,2000);
  fV1720TimeCompare->SetXTitle("Channel 0");
  fV1720TimeCompare->SetYTitle("Channel 1");
#endif

#ifdef USE_V1720
  AddHistogram(new TV1720Correlations());
#endif
  
#ifdef USE_V1730DPP
  AddHistogram(new TV1730DppWaveform());
#endif
  
#ifdef USE_V1730RAW
  AddHistogram(new TV1730RawWaveform());
#endif

#ifdef USE_DT724
  AddHistogram(new TDT724Waveform());
#endif
  
#ifdef USE_TRB3
  // Set the TDC linear calibration values
  Trb3Calib::getInstance().SetTRB3LinearCalibrationConstants(17,471);
  AddHistogram(new TTRB3Histograms());
  AddHistogram(new TTRB3FineHistograms());
  AddHistogram(new TTRB3DiffHistograms());
#endif
  
#ifdef USE_CAMACADC
  AddHistogram(new TCamacADCHistograms());
#endif
  

      
};


void TAnaManager::AddHistogram(THistogramArrayBase* histo) {
  histo->DisableAutoUpdate();

  //histo->CreateHistograms();
  fHistos.push_back(histo);

}


int TAnaManager::ProcessMidasEvent(TDataContainer& dataContainer){

  // Fill all the  histograms
  for (unsigned int i = 0; i < fHistos.size(); i++) {
    // Some histograms are very time-consuming to draw, so we can
    // delay their update until the canvas is actually drawn.
    if (!fHistos[i]->IsUpdateWhenPlotted()) {
      fHistos[i]->UpdateHistograms(dataContainer);
    }
  }
  
  if(1){
    
    TV1720RawData *v1720 = dataContainer.GetEventData<TV1720RawData>("W200");
    
    if(v1720 && !v1720->IsZLECompressed()){      

      double time[2],ph[2];
      
      for(int i = 0; i < 2; i++){ // loop first two channels
        
        TV1720RawChannel channelData = v1720->GetChannelData(i);
        if(channelData.GetNSamples() <= 0) continue;
        
        double max_adc_value = -1.0;
        double max_adc_time = -1;
        for(int j = 0; j < channelData.GetNSamples(); j++){
          double adc = channelData.GetADCSample(j);
          if(adc > max_adc_value){
            max_adc_value = adc;
            max_adc_time = j * 4.0; // 4ns per bin
          }
        }
        time[i] = max_adc_time;
        ph[i] = max_adc_value;
              //std::cout << i << " "  << max_adc_time << " " << max_adc_value << std::endl;
      }
#ifdef USE_V1720
      fV1720PHCompare->Fill(ph[0],ph[1]);
      fV1720TimeCompare->Fill(time[0],time[1]);
#endif      
    }
  }
  return 1;
}


// Little trick; we only fill the transient histograms here (not cumulative), since we don't want
// to fill histograms for events that we are not displaying.
// It is pretty slow to fill histograms for each event.
void TAnaManager::UpdateTransientPlots(TDataContainer& dataContainer){
  std::vector<THistogramArrayBase*> histos = GetHistograms();
  
  for (unsigned int i = 0; i < histos.size(); i++) {
    if (histos[i]->IsUpdateWhenPlotted()) {
      histos[i]->UpdateHistograms(dataContainer);
    }
  }
}



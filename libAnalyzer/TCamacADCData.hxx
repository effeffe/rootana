#ifndef TCamacData_hxx_seen
#define TCamacData_hxx_seen

#include <vector>
#include "stdio.h"

#include "TGenericData.hxx"

#define NADC 12

/// This class is meant to store a single trigger (LAM) of CAMAC ADC data
class CamacADCEvent {

public:
  CamacADCEvent(){
    for(int i = 0; i < NADC; i++) ADCValues[i] = -1;
  };

  // Set the ADC value for this channel.
  void SetADC(int ch, int adc);

  // Get the 
  int GetADC(int ch){
    if(ch >= 0 && ch < NADC)
      return ADCValues[ch];
    return 0;
  }



  

private:
  
  int ADCValues[NADC];
	 
};


/// This class is meant to store a set of triggers (LAMs) of CAMAC ADC data
/// specifically, it has been written to readout a set of events taken with 
/// the Wiener CC-USB... it might work for decoding other CAMAC data, though
/// I haven't tried.  T. Lindner
class TCamacData: public TGenericData {

public:

  /// Constructor
  TCamacData(int bklen, int bktype, const char* name, void *pdata);

  /// Get the Vector of TDC Measurements.
  std::vector<CamacADCEvent>& GetMeasurements() {return fMeasurements;}

  /// Get number of triggers
  int GetNTriggers(){return fMeasurements.size();}

private:

  /// Vector of CAMAC ADC measurements
  std::vector<CamacADCEvent> fMeasurements;

};

#endif

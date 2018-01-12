#include "TCamacADCData.hxx"

#include <iostream>


// Set the ADC value for this channel.
void CamacADCEvent::SetADC(int ch, int adc){
  if(ch >= 0 && ch < NADC)
    ADCValues[ch] = adc;
}


TCamacData::TCamacData(int bklen, int bktype, const char* name, void *pdata):
  TGenericData(bklen, bktype, name, pdata)
{

  int ntriggers = GetData16()[0];
 
  int pointer = 1;

  static int checkword = 0xfeed;

  while(GetData16()[pointer] != 0xffff){
    
    CamacADCEvent event;
    
    pointer++; // not sure what this word is... possibly the number of subsequent words in event.

    // fill ADC values;
    for(int i = 0; i < NADC; i++){
      event.SetADC(i,GetData16()[pointer++]);
    }    
    if(GetData16()[pointer++] != checkword){
      std::cout << "Data " << GetData16()[pointer] << " doesn't match checkword "
		<< checkword << std::endl;
    }

    fMeasurements.push_back(event);
    
  }

  if(fMeasurements.size() != ntriggers){
    std::cout << "TCamacData::CamacData: Triggers don't match!! " << ntriggers << " " << fMeasurements.size() << std::endl;
  }
  

}

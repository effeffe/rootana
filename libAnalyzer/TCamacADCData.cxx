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

  int ntriggers = GetData16()[0] & 0xfff;
 
  int pointer = 1;

  static int checkword = 0xfeed;


  bool fail = false;
  while(GetData16()[pointer] != 0xffff){
    
    CamacADCEvent event;

    // Find the next checkword.  Should be 14 words along.
    if(GetData16()[pointer+13] != checkword){
      // check if the checkword is one word back.
      if(GetData16()[pointer+12] == checkword){
	std::cout << "This event had only 11 samples; not 12.  Setting all values to -1." << std::endl;
	fMeasurements.push_back(event);
	pointer += 13;
	continue;
      }else{
	std::cout << "This event seems mangled; whole set of events probably mangled." << std::endl;	
      }
    }

    
    pointer++; // not sure what this word is... possibly the number of subsequent words in event.

    // fill ADC values;
    for(int i = 0; i < NADC; i++){
      event.SetADC(i,GetData16()[pointer++]);
    }    
    if(GetData16()[pointer++] != checkword){
      std::cout << "Data (@" << pointer << ") " << GetData16()[pointer] << " doesn't match checkword "
		<< checkword << std::endl;
      fail = true;
    }

    fMeasurements.push_back(event);
    
  }

  if((int)fMeasurements.size() != ntriggers){
    std::cout << "TCamacData::CamacData: Triggers don't match!! " << ntriggers << " " << fMeasurements.size() << std::endl;
  }

  if(fail){
    std::cout << "bklen: " << bklen <<  std::endl;
    for(int i = 0; i < bklen; i++){
      if(i%16 == 0){ std::cout << i << std::endl;}
      std::cout << std::hex << GetData16()[i] << " " << std::dec;
      if(i%16 == 15) std::cout << std::endl;
      
    }
    std::cout << std::endl;
  }
  
  
}

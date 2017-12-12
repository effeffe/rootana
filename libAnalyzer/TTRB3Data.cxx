#include "TTRB3Data.hxx"

#include <iostream>



TTRB3Data::TTRB3Data(int bklen, int bktype, const char* name, void *pdata):
    TGenericData(bklen, bktype, name, pdata)
{
  
  // Get header information  
  fPacketSize = GetData32()[0];
  fDecoding = GetData32()[1];
  fBoardId = GetData32()[2];
  fSeqNr = GetData32()[3];
  fRunNr = GetData32()[4];
  fDate = GetData32()[5];
  fTime = GetData32()[6];

  // Loop over rest of bank
  bool foundFpgaBank = false;
  uint32_t fpgaWord, headerWord;
  //std::cout << "Number of words: " << GetSize() << std::endl;
  for(int pointer = 7; pointer < GetSize(); pointer++){

    uint32_t word = GetData32()[pointer];
    //std::cout << pointer << " " << std::hex
    //        << word << std::dec << std::endl;
    // Look for word indicating a new FPGA
    if((word & 0xfff0ffff) == 0x00000100 ||
       (word & 0xfff0ffff) == 0x00000101){
      fpgaWord = word;
      // next word if TDC header
      pointer++;
      headerWord = GetData32()[pointer];
      //std::cout << "Found header: " << fpgaWord << std::endl;
      continue;
    }

    // Look for the epoch counter word; the TDC data word follows it
    if((word & 0xe0000000) == 0x60000000){
      uint32_t epochWord = word;
      pointer++;
      uint32_t tdcWord = GetData32()[pointer];

      //  std::cout << "TDC? " << std::hex
      //        << tdcWord << std::dec << std::endl; 
      if((tdcWord & 0xe0000000) == 0x80000000){
        fMeasurements.push_back(TrbTdcMeas(fpgaWord, headerWord,
                                           epochWord, tdcWord));
      }
    }
    
    
  }
  
  
}

void TTRB3Data::Print(){

  std::cout << "TRB3 decoder for bank " << GetName().c_str() << std::endl;
  std::cout << "Packet size: " << GetPacketSize() << std::endl;
  std::cout << "Board ID/Seq Number/Run Number: " << GetBoardId()
            << "/" << GetSeqNr()
            << "/" << GetRunNr() << std::endl;
  std::cout << "Date: " << GetYear() << "/" << GetMonth() << "/" << GetDay() << std::endl;
  std::cout << "Time: " << GetTime() << std::endl;  
  std::cout << "Number of measurements: " << GetNumberMeasurements()
            << std::endl;

}

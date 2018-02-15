#include "TRB3Decoder.hxx"




TrbDecoder::TrbDecoder(int bklen, void *pdata){
  
  uint32_t* fData = reinterpret_cast<uint32_t*>(pdata);

  // Get header information  
  fPacketSize = fData[0];
  fDecoding = fData[1];
  fBoardId = fData[2];
  fSeqNr = fData[3];
  fRunNr = fData[4];
  fDate = fData[5];
  fTime = fData[6];

  // Loop over rest of bank
  bool foundFpgaBank = false;
  uint32_t fpgaWord, headerWord;
  //std::cout << "Number of words: " << GetSize() << std::endl;
  for(int pointer = 7; pointer < bklen; pointer++){

    uint32_t word = fData[pointer];
    //std::cout << pointer << " " << std::hex
    //        << word << std::dec << std::endl;
    // Look for word indicating a new FPGA
    if((word & 0xfff0ffff) == 0x00000100 ||
       (word & 0xfff0ffff) == 0x00000101){
      fpgaWord = word;
      // next word if TDC header
      pointer++;
      headerWord = fData[pointer];
      //std::cout << "Found header: " << fpgaWord << std::endl;
      continue;
    }

    // Look for the epoch counter word; the TDC data word follows it
    if((word & 0xe0000000) == 0x60000000){
      uint32_t epochWord = word;
      pointer++;
      uint32_t tdcWord = fData[pointer];

      //  std::cout << "TDC? " << std::hex
      //        << tdcWord << std::dec << std::endl; 
      if((tdcWord & 0xe0000000) == 0x80000000){
        fMeasurements.push_back(TrbTdcMeas(fpgaWord, headerWord,
                                           epochWord, tdcWord));
      }
    }       
  }  
}



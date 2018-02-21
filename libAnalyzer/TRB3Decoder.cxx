#include "TRB3Decoder.hxx"


void Trb3Calib::UseTRB3LinearCalibration(bool uselinear){useLinearCalibration = uselinear;};
void Trb3Calib::SetTRB3LinearCalibrationConstants(float low_value, float high_value){
  trb3LinearLowEnd = low_value;
  trb3LinearHighEnd = high_value;
}

#include <byteswap.h>

TrbDecoder::TrbDecoder(int bklen, void *pdata, std::string bankname){

  // New decoder, for data read directly from UDP socket
  if(bankname != std::string("TRB0")){
    uint32_t* fData = reinterpret_cast<uint32_t*>(pdata);

    // check for correct endian-ness; if wrong, flip.
    // fourth word is endian encoding check word
    if(!((fData[3] & 0x1) == 1 && (fData[3] & 0x80000000) == 0)){
      for(int i = 0; i < bklen/4; i++){
	fData[i] = __bswap_32 (fData[i]);
	if(0)std::cout << "Flip/flop: " << i << " " << std::hex << fData[i] << std::dec << std::endl;
      }
    }
    int size_subevent = fData[2]/4;
    fDecoding = fData[3];

    // Loop over rest of bank
    bool foundFpgaBank = false;
    uint32_t fpgaWord, headerWord;
    //std::cout << "Number of words: " << GetSize() << std::endl;
    for(int pointer = 6; pointer < 6 + size_subevent; pointer++){
      
      uint32_t word = fData[pointer];
      //std::cout << pointer << " " << std::hex
      //        << word << std::dec << std::endl;
      // Look for word indicating a new FPGA
      if((word & 0xfff0ffff) == 0x00000100 ||
	 (word & 0xfff0ffff) == 0x00000101 ||
	 (word & 0xfff0ffff) == 0x00000102 ||
	 (word & 0xfff0ffff) == 0x00000103 ){
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
	
	if((fpgaWord & 0xf) > 3 )
	  std::cout << "TDC FPGA ID? " << std::hex << fpgaWord << " " << headerWord 
		    << " " << tdcWord << std::dec << std::endl; 
	if((tdcWord & 0xe0000000) == 0x80000000){
	  fMeasurements.push_back(TrbTdcMeas(fpgaWord, headerWord,
					     epochWord, tdcWord));
	}
      }       
    }  
  
    // Need to add some bank error checking here!!! And do a more careful loop over the data packet...
    

  }else{ // Old decoder, for data read from the DABC event builder
    
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
	 (word & 0xfff0ffff) == 0x00000101 ||
	 (word & 0xfff0ffff) == 0x00000102 ||
	 (word & 0xfff0ffff) == 0x00000103 ){
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
}



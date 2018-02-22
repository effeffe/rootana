#include "TRB3Decoder.hxx"


void Trb3Calib::UseTRB3LinearCalibration(bool uselinear){useLinearCalibration = uselinear;};
void Trb3Calib::SetTRB3LinearCalibrationConstants(float low_value, float high_value){
  trb3LinearLowEnd = low_value;
  trb3LinearHighEnd = high_value;
}

// Get endian byte swapping code; use built-in code for non-MacOS Linux
#if defined(OS_LINUX) && !defined(OS_DARWIN)
#include <byteswap.h>
#else
/* Swap bytes in 32 bit value.  */
#define R__bswap_constant_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |               \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
#endif

// See figure 23 of TRB3 manual for description of TRB3 packet

TrbDecoder::TrbDecoder(int bklen, void *pdata, std::string bankname){
  
  fPacketSize = 0;
  fDecoding = 0;
  fBoardId = 0;
  fSeqNr = 0;
  fRunNr = 0;
  fDate = 0;
  fTime = 0;
  
  // New decoder, for data read directly from UDP socket
  if(bankname != std::string("TRB0")){
    uint32_t* fData = reinterpret_cast<uint32_t*>(pdata);

    // check for correct endian-ness; if wrong, flip.
    // fourth word is endian encoding check word
    if(!((fData[3] & 0x1) == 1 && (fData[3] & 0x80000000) == 0)){
      for(int i = 0; i < bklen/4; i++){
#if defined(OS_LINUX) && !defined(OS_DARWIN)
	fData[i] = __bswap_32 (fData[i]);
#else
        fData[i] = R__bswap_constant_32(fData[i]);
#endif
      }
    }
    // This is the number of words in the sub-event packet, not including this word.
    int size_subevent = fData[2]/4;
    fDecoding = fData[3];

    // Decode sub-event ID, trigger number and trigger code!!!
    
    // Loop over rest of bank
    uint32_t fpgaWord = 0, headerWord = 0;
    int pointer = 6;

    // We search for words that look like the FPGA header words (sub-sub-event IDs);
    // if we don't find them, break out.
    bool finished = false;
    while (!finished){
      uint32_t word = fData[pointer];
      // Look for word indicating a new FPGA
      if((word & 0x0000ffff) == 0x00000100 ||
	 (word & 0x0000ffff) == 0x00000101 ||
	 (word & 0x0000ffff) == 0x00000102 ||
	 (word & 0x0000ffff) == 0x00000103 ){
	fpgaWord = word;

        int nwords_subevent = ((word & 0xffff0000) >> 16);

        //	std::cout << "Found header: " << std::hex << fpgaWord << std::dec << " at " << pointer << " nwords: " << nwords_subevent<< std::endl;
          
       	// next word if TDC header
	pointer++;
	headerWord = fData[pointer];
                
        // Now loop over the next couple words, grabbing the TDC data
        for(int i = 0; i < nwords_subevent; i++){
          pointer++;
          uint32_t word = fData[pointer];
          // Look for the epoch counter word; the TDC data word follows it
          if((word & 0xe0000000) == 0x60000000){
            uint32_t epochWord = word;
            pointer++; i++;
            uint32_t tdcWord = fData[pointer];
            
            if((fpgaWord & 0xf) > 3 ){
              std::cout << "TDC FPGA ID > 3?  Not possible... " << std::hex << fpgaWord << " " << headerWord 
                        << " " << tdcWord << " " << pointer << " " << size_subevent << std::dec << std::endl;
              for(int i = 0; i < 6 ; i++)
                std::cout << i << " 0x"<<std::hex
                          << fData[i] << std::dec << std::endl;
            }else{
              
              if((tdcWord & 0xe0000000) == 0x80000000){
                //                std::cout << std::hex << "Adding TDC " << headerWord << " " <<  epochWord << " " << tdcWord << std::endl;
                fMeasurements.push_back(TrbTdcMeas(fpgaWord, headerWord,
                                                   epochWord, tdcWord));
              }
              
            }
          }          
        }
        
      }else{
        // The next word isn't a sub-sub-event ID word, so break out.
        finished = true;
      }
      
      
    }
    

    // Go to end of sub-event; check the trailer word
    int end_packet = 2+ size_subevent -1;
    if( fData[end_packet-1] != 0x15555){
      std::cout << "TRB3 sub-event ID trailer word = " << fData[end_packet-1] << "; not expected 0x15555; bank decoding error!!!" << std::endl;
    }

    
    //    std::cout << "Check  " << std::dec << pointer << " " << end_packet << " num measurements: " << fMeasurements.size() <<"\n_______\n"<< std::endl;
    

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



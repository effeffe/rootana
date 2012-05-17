#ifndef TV1190Data_hxx_seen
#define TV1190Data_hxx_seen

#include <vector>

#include "TGenericData.hxx"

/// Class for each TDC measurement
/// For the definition of obscure variables see the CAEN V1190 manual.
class TDCMeasurement {

  friend class TV1190Data;

public:
  
  /// Is this the leading edge measurement?
  bool IsLeading() const {return ((tdc_measurement_word & 0x4000000) == 0x0000000);}
  /// Is this the trailing edge measurement?
  bool IsTrailing() const {return ((tdc_measurement_word & 0x4000000) == 0x4000000);}
  /// Get the TDC measurement
  uint32_t GetMeasurement() const {return (tdc_measurement_word & 0x7ffff);}

/// Get the TDC number
  uint32_t GetTDCNumber() const;
  /// Get the channel number
  uint32_t GetChannel() const {return ((tdc_measurement_word & 0x3f80000) >> 19 );}

  /// Get Event ID
  uint32_t GetEventID() const;

  /// Get Bunch ID
  uint32_t GetBunchID() const;

  /// Check if measurement has a TDC header
  bool HasTDCHeader() const {return (tdc_header_word != 0);}
  /// Check if measurement has a TDC trailer
  bool HasTDCTrailer() const {return (tdc_trailer_word != 0);}
  /// Check if measurement has a TDC error word
  bool HasTDCErrorWord() const {return (tdc_error_error != 0);}

  /// Get Errors
  uint32_t GetErrors() const;

private:

  /// Found fields to hold the header, measurement, trailer and error words.
  uint32_t tdc_header_word;
  uint32_t tdc_measurement_word;
  uint32_t tdc_trailer_word;
  uint32_t tdc_error_error;
  
  /// Constructor; need to pass in header and measurement.
  TDCMeasurement(uint32_t header, uint32_t measurement):
    tdc_header_word(header),
    tdc_measurement_word(measurement),
    tdc_trailer_word(0),tdc_error_error(0){};

  /// Set the trailer word.
  void SetTrailer(uint32_t trailer);

  /// Set the error word.
  void SetErrors(uint32_t error){tdc_error_error = error;}  


  TDCMeasurement();    
};


/// Class to store data from CAEN V1190.
/// We store the information as a vector of TDCMeasurement's
/// Question: do we need a way of retrieving information about TDCs that
/// have no measurements?  Currently this information is not exposed.
/// For the definition of obscure variables see the CAEN V1190 manual.
class TV1190Data: public TGenericData {

public:

  /// Constructor
  TV1190Data(int bklen, int bktype, const char* name, void *pdata);


  /// Get Event Counter
  uint32_t GetEventCounter() const {return (fGlobalHeader & 0x07ffffe0) >> 5;};

  /// Get Geographical Address
  uint32_t GetGeoAddress() const {return (fGlobalHeader & 0x1f) ;};

  /// Get the extended trigger time tag
  int GetExtendedTriggerTimeTag() const {return fExtendedTriggerTimeTag;};

  /// Get the word count
  int GetWordCount() const {return fWordCount;};

  // Methods to check the status flags in trailer.
  bool IsTriggerLost() const {
    if(fStatus & 0x4)
      return true;
    else
      return false;
  }  
  bool HasBufferOverflow() const {
    if(fStatus & 0x2)
      return true;
    else
      return false;
  }
  bool HasTDCError() const {
    if(fStatus & 0x1)
      return true;
    else
      return false;
  }


  void Print();

  /// Get the Vector of TDC Measurements.
  std::vector<TDCMeasurement>& GetMeasurements() {return fMeasurements;}


private:
  
  /// The overall global header
  uint32_t fGlobalHeader;
  
  /// The overall global trailer
  uint32_t fGlobalTrailer;
    
  int fExtendedTriggerTimeTag; // Extended trigger time
  int fWordCount; // Word count

  uint32_t fStatus;


  /// Vector of TDC Measurements.
  std::vector<TDCMeasurement> fMeasurements;

};

#endif

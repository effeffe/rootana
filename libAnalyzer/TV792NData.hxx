#ifndef TV792NData_hxx_seen
#define TV792NData_hxx_seen

#include <vector>

#include "TGenericData.hxx"

/// Class for each TDC measurement
/// For the definition of obscure variables see the CAEN V792N and V785N manual.
/// Data organization is almost same as V792, but with a shifted bit.
class VADCNMeasurement {

  friend class TV792NData;

public:
  
  /// Get the ADC measurement
  uint32_t GetMeasurement() const {return (adc_measurement_word & 0xfff);}

  /// Get GEO address
  uint32_t GetGeoAddress() const {return ((adc_measurement_word & 0xf8000000) >> 27);}
  
  /// Get the crate number
  uint32_t GetCrate() const {return ((adc_header_word & 0xff0000) >> 16);}

  /// Get the channel number
  uint32_t GetChannel() const {return ((adc_measurement_word & 0x1f0000) >> 17);}

  /// Is Under Threshold?
  bool IsUnderThreshold() const {return ((adc_measurement_word & 0x2000) == 0x2000);}

  /// Is OverFlow?
  bool IsOverFlow() const {return ((adc_measurement_word & 0x1000) == 0x1000);}
  
private:

  /// Fields to hold the header, measurement, trailer and error words.
  uint32_t adc_header_word;
  uint32_t adc_measurement_word;
  
  /// Constructor; need to pass in header and measurement.
  VADCNMeasurement(uint32_t header, uint32_t measurement):
    adc_header_word(header),
    adc_measurement_word(measurement){

  }


  VADCNMeasurement();    
};


/// Class for storing data from CAEN V792 module
class TV792NData: public TGenericData {

 
public:

  /// Constructor
  TV792NData(int bklen, int bktype, const char* name, void *pdata);


  void Print();
  
  /// Get GEO address
  uint32_t GetGeoAddress() const {return ((fAdc_header_word & 0xf8000000) >> 27);}
  
  /// Get the crate number
  uint32_t GetCrate() const {return ((fAdc_header_word & 0xff0000) >> 16);}

  /// Get the number of converted channels.
  uint32_t GetNumberChannels() const {return ((fAdc_header_word & 0x3f00) >> 8); };

  /// Get the event counter
  uint32_t GetEventCounter() const {return (fAdc_trailer_word & 0xffffff); };

    /// Get the Vector of TDC Measurements.
  std::vector<VADCNMeasurement>& GetMeasurements() {return fMeasurements;}


private:

  /// Vector of TDC Measurements.
  std::vector<VADCNMeasurement> fMeasurements;


  /// Fields to hold the header, measurement, trailer and error words.
  uint32_t fAdc_header_word;
  uint32_t fAdc_trailer_word;

};

#endif

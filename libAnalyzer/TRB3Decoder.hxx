#ifndef TRB3Decoder_hxx_seen
#define TRB3Decoder_hxx_seen

// Decoder for the GSI TRB3 FPGA-TDC
// Details on TRB3 here: http://trb.gsi.de/
//
// Thomas Lindner
// Feb 14, 2018


#include <vector>
#include <iostream>
#include <inttypes.h>

// Some global variables for setting the TRB3 calibration
class Trb3Calib
{
public:
  static Trb3Calib& getInstance()
  {
    static Trb3Calib    instance;
    return instance;
  }

  void UseTRB3LinearCalibration(bool uselinear);  
  void SetTRB3LinearCalibrationConstants(float low_value, float high_value);  

  bool LinearCalib(){return useLinearCalibration;}
  float LinearCalibLowEnd(){ return trb3LinearLowEnd;};
  float LinearCalibHighEnd(){ return trb3LinearHighEnd;};

private:
  Trb3Calib() {
    useLinearCalibration = true; 
    trb3LinearLowEnd = 17.0; 
    trb3LinearHighEnd = 473.0; 
  }
  Trb3Calib(Trb3Calib const&);              // Don't Implement.
  void operator=(Trb3Calib const&); // Don't implement

  bool useLinearCalibration = true; // use linear calibration
  
  float trb3LinearLowEnd = 17.0; // low value of fine TDC hits
  float trb3LinearHighEnd = 473.0; // high value of fine TDC hits
  
  
};

// Function to allow 



/// Decoder for individual hits from GSI TFB3 FPGA-TDC
class TrbTdcMeas {

  friend class TTRB3Data;

public:
  
  /// Is this the leading edge measurement?
  bool IsLeading() const {return 0;}
  /// Is this the trailing edge measurement?
  bool IsTrailing() const {return 0;}

  uint32_t GetBoardId() const {return fgpa_header_word & 0xf;}
  
  /// Get the TDC measurement
  uint32_t GetMeasurement() const {
    
    return (tdc_measurement_word & 0x7ffff);
  }

  // semi calibrated time in picoseconds
  double GetFinalTime() const { 
    if(Trb3Calib::getInstance().LinearCalib()){ // use linear calibration, if requested
      return ((double) GetCoarseTime()) * 5000.0
        - ((((double)GetFineTime())-Trb3Calib::getInstance().LinearCalibLowEnd())
           /(Trb3Calib::getInstance().LinearCalibHighEnd() - Trb3Calib::getInstance().LinearCalibLowEnd())) *5000.0;
    }
    return -99.0;
  }
  
  uint32_t GetFineTime() const {
    return (tdc_measurement_word & 0x1ff000) >> 12;
  };

  uint32_t GetCoarseTime() const {return tdc_measurement_word & 0x7ff;};

  uint32_t GetEpochTime() const {return tdc_epoch_word & 0xfffffff;};
  
  /// Get the channel number
  uint32_t GetChannel() const {
    return ((tdc_measurement_word & 0x7c00000 ) >> 22 );
  }

 
  /// Constructor; 
  TrbTdcMeas(uint32_t fpga, uint32_t header, uint32_t epoch, uint32_t measurement):
    fgpa_header_word(fpga),
    tdc_header_word(header),
    tdc_epoch_word(epoch),
    tdc_measurement_word(measurement),
    event_index(0){};

  TrbTdcMeas();    

private:

  /// Found fields to hold the header, measurement error words.
  uint32_t fgpa_header_word;
  uint32_t tdc_header_word;
  uint32_t tdc_epoch_word;
  uint32_t tdc_measurement_word;
  int event_index;
 

};


/// Decoder for data packets from TRB3.
class TrbDecoder {

public:

  /// Constructor
  TrbDecoder(int bklen, void *pdata);


  void Print();

  const int GetNumberMeasurements(){return fMeasurements.size();}
  
  /// Get the Vector of TDC Measurements.
  std::vector<TrbTdcMeas>& GetMeasurements() {return fMeasurements;}

  /// Get Packet size
  const uint32_t GetPacketSize(){return fPacketSize;}
  // Get Board ID
  const uint32_t GetBoardId(){return fBoardId;}
  // Get sequence number
  const uint32_t GetSeqNr(){return fSeqNr;}
  // Get run number
  const uint32_t GetRunNr(){return fRunNr;};
  // Get year
  const uint32_t GetYear(){return (fDate &0xff0000) >> 16;};
  // Get month
  const uint32_t GetMonth(){return (fDate &0xff00) >> 8;};
  // Get day
  const uint32_t GetDay(){return (fDate &0xff);};
  // Get time; seconds since when???
  const uint32_t GetTime(){return (fTime);};;
  

  
private:

  uint32_t fPacketSize;
  uint32_t fDecoding;
  uint32_t fBoardId;
  uint32_t fSeqNr;
  uint32_t fRunNr;
  uint32_t fDate;
  uint32_t fTime;
  
  /// Vector of TDC Measurements.
  std::vector<TrbTdcMeas> fMeasurements;


  
};

#endif

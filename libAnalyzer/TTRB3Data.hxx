#ifndef TTRB3Data_hxx_seen
#define TTRB3Data_hxx_seen

#include <vector>

#include "TGenericData.hxx"

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

  double GetFinalTime() const { // Currently return time with crude calibration
    return ((double) GetCoarseTime()) * 5.0 - ((((double)GetFineTime())-17.0)/457.0) *5.0;        
  }
  
  uint32_t GetFineTime() const {
    return (tdc_measurement_word & 0x1ff000) >> 12;
  };

  uint32_t GetCoarseTime() const {return tdc_measurement_word & 0x7ff;};
  
  
  /// Get the channel number
  uint32_t GetChannel() const {
    return ((tdc_measurement_word & 0x7c00000 ) >> 22 );
  }


private:

  /// Found fields to hold the header, measurement error words.
  uint32_t fgpa_header_word;
  uint32_t tdc_header_word;
  uint32_t tdc_epoch_word;
  uint32_t tdc_measurement_word;
  int event_index;
  
  /// Constructor; need to pass in header and measurement.
  TrbTdcMeas(uint32_t fpga, uint32_t header, uint32_t epoch, uint32_t measurement):
    fgpa_header_word(fpga),
    tdc_header_word(header),
    tdc_epoch_word(epoch),
    tdc_measurement_word(measurement),
    event_index(0){};

  TrbTdcMeas();    
};


/// Decoder for data packets from TRB3.
class TTRB3Data: public TGenericData {

public:

  /// Constructor
  TTRB3Data(int bklen, int bktype, const char* name, void *pdata);


  /// Get Event Counter
  uint32_t GetEventCounter(int index = 0) const {return (fGlobalHeader[index] & 0x07ffffe0) >> 5;};

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
  
  /// The overall global header
  std::vector<uint32_t> fGlobalHeader;
  
  /// Vector of TDC Measurements.
  std::vector<TrbTdcMeas> fMeasurements;


  
};

#endif

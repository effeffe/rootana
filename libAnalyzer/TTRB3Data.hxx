#ifndef TTRB3Data_hxx_seen
#define TTRB3Data_hxx_seen

#include <vector>

#include "TGenericData.hxx"
#include "TRB3Decoder.hxx"


/// Container for data packets from TRB3.
/// Uses the TRB3Decoder class.
class TTRB3Data: public TGenericData {

public:

  /// Constructor
  TTRB3Data(int bklen, int bktype, const char* name, void *pdata);

  void Print();

  const int GetNumberMeasurements(){return decoder.GetNumberMeasurements();}
  
  /// Get the Vector of TDC Measurements.
  std::vector<TrbTdcMeas>& GetMeasurements() {return decoder.GetMeasurements();}

  /// Get Packet size
  const uint32_t GetPacketSize(){return decoder.GetPacketSize();}
  // Get sequence number
  const uint32_t GetSeqNr(){return decoder.GetSeqNr();}
  // Get run number
  const uint32_t GetRunNr(){return decoder.GetRunNr();};
  // Get year
  const uint32_t GetYear(){return decoder.GetYear();};
  // Get month
  const uint32_t GetMonth(){return decoder.GetMonth();};
  // Get day
  const uint32_t GetDay(){return decoder.GetDay();};
  // Get time; seconds since when???
  const uint32_t GetTime(){return decoder.GetTime();};;
  
  /// Get decoder
  TrbDecoder& GetDecoder() {return decoder;}

  
private:

  /// Vector of TDC Measurements.
  std::vector<TrbTdcMeas> fMeasurements;

  TrbDecoder decoder;
  
};

#endif

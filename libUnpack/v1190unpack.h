// v1190unpack.h

#include <vector>

class v1190hit
{
 public:
  int  channel; // 7 bits
  bool trailing; // 1 bit
  int  measurement; // 19 bits
};

class v1190event
{
 public:
  bool error;
  // global header
  int  event_count; // 22 bits
  int  geo; // 5 bits
  // tdc header
  int  tdc_header_tdc; // 2 bits
  int  tdc_header_event_id; // 12 bits
  int  tdc_header_bunch_id; // 12 bits
  // tdc trailer
  int  tdc_trailer_tdc; // 2 bits
  int  tdc_trailer_event_id; // 12 bits
  int  tdc_trailer_word_count; // 12 bits
  // tdc error
  int  tdc_error_tdc; // 2 bits
  int  tdc_error_flags; // 15 bits
  // externded trigger time tag
  int  ettt; // 27 bits
  // trailer
  bool trailer_trigger_lost;
  bool trailer_output_buffer_overflow;
  bool trailer_tdc_error;
  int  trailer_word_count; // 16 bits
  int  trailer_geo; // 5 bits

  std::vector<v1190hit> hits;

 public:
  v1190event(); // ctor
  void Print() const;
};

v1190event* UnpackV1190(const char** data, int* datalen, bool verbose);

// end

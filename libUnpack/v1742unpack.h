// v1742unpack.h

class v1742event
{
 public:
  bool error;

  int total_event_size; // 28 bits
  int board_id; // 5 bits
  int pattern;  // 14 bits
  int group_mask; // 4 bits
  int event_counter; // 22 bits
  int event_time_tag; // 32 bits

  int len[4]; // 12 bits, size, 1024 samples is 0xC00
  int tr[4];  // 1 bit, TR signal is present or not
  int freq[4]; // 2 bits, 00=5Gs/s, 01=2.5Gs/s, 10=1Gs/s, 11=not used
  int cell[4]; // 10 bits, start cell index of DRS4 SCA
  int trigger_time_tag[4]; // maybe 30 bits

  int adc[32][1024];
  int adc_tr[4][1024];
  
  bool adc_overflow[32];
  bool adc_tr_overflow[4];

 public:
  v1742event(); // ctor
  void Print() const;
};

v1742event* UnpackV1742(const char** data, int* datalen, bool verbose);

// end

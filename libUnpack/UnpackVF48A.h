// UnpackVF48A.h

#ifndef UnpackVF48A_H
#define UnpackVF48A_H

#include <stdint.h>
#include <deque>

#define VF48_MAX_MODULES   22
#define VF48_MAX_GROUPS     6
#define VF48_MAX_CHANNELS  48
#define VF48_MAX_SAMPLES 1100

struct VF48channel
{
  int channel;
  uint32_t time;
  uint32_t charge;
  bool     complete;
  int numSamples;
  uint16_t samples[VF48_MAX_SAMPLES];
};

struct VF48module
{
  int      unit;
  uint32_t trigger;
  int      error;
  int      tgroup;
  uint32_t timestampGroupMask;
  uint64_t timestamp64[VF48_MAX_GROUPS];
  double   timestamps[VF48_MAX_GROUPS];
  uint32_t trigno[VF48_MAX_GROUPS];
  uint32_t groupMask;
  uint32_t completeGroupMask;
  bool     complete;
  //int      numSamples;
  //int      numChannels;
  int      presentMask[VF48_MAX_GROUPS];  // mask of channels present in the data
  int      hitMask[VF48_MAX_GROUPS]; // mask of channels that have hits detected by firmware
  VF48channel channels[VF48_MAX_CHANNELS];

  VF48module(int i); // ctor  

  void PrintModule() const;
};

struct VF48event
{
  int         eventNo;
  int         complete;
  int         error;
  uint32_t    modulesMask;
  double      timestamp;
  VF48module* modules[VF48_MAX_MODULES];

  VF48event(int eventNo); // ctor

  ~VF48event(); // dtor

  void PrintSummary() const;
  void PrintEvent() const;
};

class UnpackVF48
{
 public:
  UnpackVF48(); // ctor
  ~UnpackVF48(); // dtor

  void Reset();

  void UnpackStream(int module, const void* data_ptr, int data_size);
  VF48event* GetEvent(bool flush=false);

  void SetNumModules(int num_modules);
  void SetModulesMask(uint32_t mask);
  void SetGroupEnableMask(int module, int grp_enable_mask);
  void SetNumSamples(int module, int num_samples);

  void SetDisasm(bool stream, bool structure, bool samples);

  void SetCoincTime(double time_sec);
  void SetTimestampResync(bool enable);
  void SetTsFreq(int module, double ts_freq_hz);
  void SetChanEnable(int module, int grp, int chan_enable);
  void SetFlushIncompleteThreshold(int threshold);

  double GetTsFreq(int module);

 public:
  int      fNumModules;
  uint32_t fModulesMask;
  uint32_t fGroupEnabled[VF48_MAX_MODULES];
  bool     fChanEnabled[VF48_MAX_MODULES][VF48_MAX_CHANNELS];
  int      fNumSamples[VF48_MAX_MODULES];
  double   fFreq[VF48_MAX_MODULES];
  double   fCoinc;
  unsigned fFlushIncompleteThreshold;

  bool fDisasmStream;
  bool fDisasmStructure;
  bool fDisasmSamples;

  bool fTimestampResyncEnable;

  int  fEventNo;
  int  fBadDataCount;

  // buffer of pending events
  std::deque<VF48event*> fBuffer;

  // buffer of stream data
  int       wgrp[VF48_MAX_MODULES];
  int       wmax[VF48_MAX_MODULES][VF48_MAX_GROUPS];
  int       wptr[VF48_MAX_MODULES][VF48_MAX_GROUPS];
  uint32_t *wbuf[VF48_MAX_MODULES][VF48_MAX_GROUPS];
  bool      wacc[VF48_MAX_MODULES][VF48_MAX_GROUPS];
  int       wdiscarded[VF48_MAX_MODULES];

  // timestamps
  uint64_t  ts_first[VF48_MAX_MODULES][VF48_MAX_GROUPS];
  uint64_t  ts_last[VF48_MAX_MODULES][VF48_MAX_GROUPS];

 private:
  void UnpackEvent(int module, int group, const uint32_t* data, int wcount);
  VF48event*  FindEvent(int unit, double timestamp, bool dup = false);
  VF48module* FindModule(int unit, int group, double timestamp);
  void AddToEvent(VF48module* m);
  void CompleteModule(VF48module* m);
  void CompleteEvent(VF48event* e);
  void ResetEventBuffer();
  void ResetStreamBuffer();
  void PrintAllEvents();
  int NewGroup(int unit);
};

#endif
// end

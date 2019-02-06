// UnpackVF48A.cxx

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#include "UnpackVF48A.h"

VF48module::VF48module(int i) // ctor  
{
  unit = i;
  trigger = 0;
  error = 0;
  for (int i=0; i<VF48_MAX_GROUPS; i++) {
    trigno[i] = 0;
    timestamp64[i] = 0;
    timestamps[i] = 0;
  }
  for (int i=0; i<VF48_MAX_GROUPS; i++) {
    presentMask[i] = 0;
    hitMask[i] = 0;
  }
  complete = false;
  completeGroupMask = 0;
  timestampGroupMask = 0;
  tgroup = 0;
  groupMask = 0;
  //numSamples = 0;
  //numChannels = 0;
  for (int i=0; i<VF48_MAX_CHANNELS; i++)
    {
      channels[i].channel    = -1;
      channels[i].numSamples =  0;
      channels[i].complete   =  false;
    }
}

void VF48module::PrintModule() const
{
  printf("Unit %d, trigger %d, timestamp %.3fms, tgroup %d, grpmask 0x%02x 0x%02x, complete %d, error %d\n", 
         unit,
         trigger,
         timestamps[tgroup]*1000.0, // convert sec to msec
         tgroup,
         groupMask,
         completeGroupMask,
         complete,
	 error);

  for (int i=0; i<VF48_MAX_GROUPS; i++) {
     printf("Group %d: ts64: 0x%016llx, %.3fms, trigno: %d, hit masks: 0x%02x 0x%02x\n", i, (unsigned long long)timestamp64[i], timestamps[i]*1000.0, trigno[i], presentMask[i], hitMask[i]);
  }
}

VF48event::VF48event(int xeventNo) // ctor
{
  eventNo = xeventNo;
  timestamp = 0;
  complete = 0;
  error = 0;
  modulesMask = 0;
  for (int i=0; i<VF48_MAX_MODULES; i++)
    modules[i] = NULL;
}

VF48event::~VF48event() // dtor
{
  eventNo = 0;
  timestamp = 0;
  for (int i=0; i<VF48_MAX_MODULES; i++)
    if (modules[i])
      {
        delete modules[i];
        modules[i] = NULL;
      }
}

void VF48event::PrintSummary() const
{
  int max = 0;

  for (int i=0; i<VF48_MAX_MODULES; i++)
    if (modules[i])
       max = i+1;

  printf("VF48 event: %6d, ", eventNo);
  printf("Triggers: ");
  for (int i=0; i<max; i++)
    if (modules[i])
      printf(" %4d", modules[i]->trigger);
    else
      printf(" null");
  printf(", Timestamps: ");
  for (int i=0; i<max; i++)
    if (modules[i])
       printf(" %.3f", modules[i]->timestamps[modules[i]->tgroup]*1000.0);
    else
      printf("    (null)");
  printf(" complete %d, error %d", complete, error);
  printf("\n");
}

void VF48event::PrintEvent() const
{
   PrintSummary();
   
   for (int i=0; i<VF48_MAX_MODULES; i++)
      if (modules[i])
         modules[i]->PrintModule();
}

//static double GetTimeSec()
//{
//  struct timeval tv;
//  gettimeofday(&tv,NULL);
//  return tv.tv_sec + 0.000001*tv.tv_usec;
//}

void PrintEvent(const VF48event* e)
{
  printf("VF48 event: %d\n", e->eventNo);
  printf("Timestamps: \n");
  for (int i=0; i<VF48_MAX_MODULES; i++)
    {
      printf("Unit %d: ", i);
      for (int j=0; j<VF48_MAX_GROUPS; j++)
        printf(" %u", e->modules[i]->trigno[j]);
      for (int j=0; j<VF48_MAX_GROUPS; j++)
        printf(" %.3fms", e->modules[i]->timestamps[j]*1000.0);
      //printf(" freq %fMHz", gVF48freq[i]/1e6);
      printf("\n");
    }
}

UnpackVF48::UnpackVF48() // ctor
{
   fNumModules = 0;
   fModulesMask = 0;
   
   for (int i=0; i<VF48_MAX_MODULES; i++) {
      fGroupEnabled[i] = 0x3F;
      fNumSamples[i] = 1000;
      fFreq[i] = 60.0e6; // 60MHz VF48 ADC clock
      for (int j=0; j<VF48_MAX_CHANNELS; j++)
         fChanEnabled[i][j] = true;
   }

   fCoinc = 0.000100; // in seconds

   fFlushIncompleteThreshold = 20;

   fDisasmStream = false;
   fDisasmStructure = false;
   fDisasmSamples = false;

   fTimestampResyncEnable = true;

   fEventNo = 0;
   fBadDataCount = 0;

   for (int i=0; i<VF48_MAX_MODULES; i++)
      for (int j=0; j<VF48_MAX_GROUPS; j++)
         wbuf[i][j] = NULL;

   Reset();
}

UnpackVF48::~UnpackVF48() // dtor
{
   ResetEventBuffer();
   ResetStreamBuffer();
   for (int i=0; i<VF48_MAX_MODULES; i++)
      for (int j=0; j<VF48_MAX_GROUPS; j++)
         if (wbuf[i][j]) {
            free(wbuf[i][j]);
            wbuf[i][j] = NULL;
      }
}

void UnpackVF48::PrintAllEvents()
{
   printf("--- all buffered events ---\n");
   for (unsigned i=0; i<fBuffer.size(); i++)
      fBuffer[i]->PrintSummary();
   printf("--- end ---\n");
}

void UnpackVF48::SetCoincTime(double time_sec)
{
   fCoinc = time_sec;
}

void UnpackVF48::SetTimestampResync(bool enable)
{
   fTimestampResyncEnable = enable;
}

void UnpackVF48::SetNumModules(int num_modules)
{
   assert(num_modules > 0);
   assert(num_modules < VF48_MAX_MODULES);
   fNumModules = num_modules;
   fModulesMask = 0;
   for (int i=0; i<fNumModules; i++) {
      fModulesMask |= (1<<i);
   }
}

void UnpackVF48::SetModulesMask(uint32_t mask)
{
   //fNumModules = num_modules;
   fModulesMask = mask;
}

void UnpackVF48::SetGroupEnableMask(int unit, int groupMask)
{
   for (int i=0; i<fNumModules; i++)
      if (i==unit || unit<0)
         fGroupEnabled[i] = groupMask;
}

void UnpackVF48::SetNumSamples(int unit, int num_samples)
{
   for (int i=0; i<fNumModules; i++)
      if (i==unit || unit<0)
         fNumSamples[i] = num_samples;
}

void UnpackVF48::SetChanEnable(int unit, int grp, int chan_enable_mask)
{
   assert(unit >= 0);
   assert(unit < fNumModules);
   assert(grp >= 0);
   assert(grp < VF48_MAX_GROUPS);
   for (int i=0; i<VF48_MAX_CHANNELS/VF48_MAX_GROUPS; i++)
      if (chan_enable_mask & (1<<i))
         fChanEnabled[unit][grp*VF48_MAX_CHANNELS/VF48_MAX_GROUPS + i] = true;
      else
         fChanEnabled[unit][grp*VF48_MAX_CHANNELS/VF48_MAX_GROUPS + i] = false;
}

void UnpackVF48::SetTsFreq(int unit, double ts_freq_hz)
{
   for (int i=0; i<fNumModules; i++)
      if (i==unit || unit<0)
         fFreq[unit] = ts_freq_hz;
}

double UnpackVF48::GetTsFreq(int unit)
{
   assert(unit >= 0);
   assert(unit < fNumModules);
   return fFreq[unit];
}

void UnpackVF48::SetFlushIncompleteThreshold(int threshold)
{
   fFlushIncompleteThreshold = threshold;
}

VF48event* UnpackVF48::FindEvent(int unit, double timestamp, bool dup)
{
   for (unsigned i=0; i<fBuffer.size(); i++) {
      VF48event* e = fBuffer[i];
      if (fabs(e->timestamp - timestamp) < fCoinc)
         if (e->modules[unit] == NULL || dup)
            return e;
   }

   VF48event* e = new VF48event(++fEventNo);
   e->timestamp = timestamp;
   fBuffer.push_back(e);
   return e;
}

VF48event* UnpackVF48::GetEvent(bool flush)
{
   // if buffer is empty, return nothing

   if (fBuffer.size() < 1)
      return NULL;

   VF48event* e = fBuffer[0];

   if (!e->complete)
      CompleteEvent(e);

   // if buffer has a complete event
   // or we are flushing out anything remaing after
   // the end of file or end of run, return it

   if (e->complete || flush) {
      fBuffer.pop_front();
      return e;
   }

   // we have an incomplete event
   //
   // look at the buffer, if there are complete
   // events after this incomplete event, push this
   // incomplete event out.

   for (unsigned i=0; i<fBuffer.size(); i++) {

      VF48event* ee = fBuffer[i];
      if (!ee->complete)
         CompleteEvent(ee);

      if (ee->complete) {
         fBuffer.pop_front();
         return e;
      }
   }

   // the buffer only has incomplete events
   //
   // one possibility is if one VF48 module died
   // and does not produce any data and there will
   // be no complete events ever again. To avoid
   // fill the buffer with an infinite number of
   // incomplete events and running out of memory,
   // we start flushing incomplete events and let
   // the user deal with it.

   if (fBuffer.size() > fFlushIncompleteThreshold) {
      fBuffer.pop_front();
      return e;
   }
      
   // maybe more data will arrive later to complete
   // the current event or some events after it.

   return NULL;
}

void UnpackVF48::Reset()
{
  ResetEventBuffer();
  ResetStreamBuffer();

  fBadDataCount = 0;
  fEventNo = 0;

  for (int unit=0; unit<VF48_MAX_MODULES; unit++) {
     for (int grp=0; grp<VF48_MAX_GROUPS; grp++) {
        ts_first[unit][grp] = 0;
        ts_last[unit][grp] = 0;
     }
  }

  // which modules and channels are enabled?
  for (int unit=0; unit<VF48_MAX_MODULES; unit++)
    {
      int count = 0;
      for (int i=0; i<VF48_MAX_GROUPS; i++)
        for (int j=0; j<8; j++)
          {
            if (fGroupEnabled[unit] & (1<<i))
              {
                fChanEnabled[unit][i*8+j] = true;
                count ++;
              }
            else
              {
                fChanEnabled[unit][i*8+j] = false;
              }
          }
          

      //printf("VF48: module %d, groups enabled: 0x%x, channels enabled: %d, num samples: %d\n", unit, gGroupEnabled[unit], count, gNumSamples);
    }
}

void UnpackVF48::CompleteModule(VF48module* m)
{
  if (m->groupMask == 0)
    {
      // empty event
      m->complete = true;
      return;
    }

  for (int i=0; i<VF48_MAX_CHANNELS; i++)
    {
      if (!fChanEnabled[m->unit][i])
        continue;

      // no data for this channel
      if (m->channels[i].channel==(-1) && m->channels[i].numSamples==0)
        continue;
      
      // channel is suppressed
      if (m->channels[i].numSamples==0)
        continue;
      
      if (m->channels[i].numSamples != fNumSamples[m->unit])
        {
          printf("*** Unit %d, trigger %d: Channel %d %d has %4d samples, should be %3d\n", m->unit, m->trigger, i, m->channels[i].channel, m->channels[i].numSamples, fNumSamples[m->unit]), fBadDataCount++;
          m->error = 1;
        }

      if (m->channels[i].numSamples > VF48_MAX_SAMPLES)
         m->channels[i].numSamples = VF48_MAX_SAMPLES;
    }
  
  uint32_t tn = m->trigno[m->tgroup];
  double t = m->timestamps[m->tgroup];
  for (int i=0; i<VF48_MAX_GROUPS; i++)
    if (fGroupEnabled[m->unit] & (1<<i))
       {
          if (!(m->groupMask & (1<<i)))
             {
                printf("*** Unit %d, trigger %d: No data for group %d\n", m->unit, m->trigger, i), fBadDataCount++;
                m->error = 1;
             }
          else if (!(m->timestampGroupMask & (1<<i)))
             {
                printf("*** Unit %d, trigger %d: No timestamp for group %d\n", m->unit, m->trigger, i), fBadDataCount++;
                m->error = 1;
             }
          else if (fabs(m->timestamps[i] - t) > 0.010)
             {
                printf("*** Unit %d, trigger %d: Internal timestamp mismatch: group %d timestamp %.6fs should be %.6fs\n", m->unit, m->trigger, i, m->timestamps[i], t), fBadDataCount++;
                printf("groups 0x%x\n", m->groupMask);
                m->PrintModule();
                m->error = 1;
             }

          if (m->trigno[i] != tn)
             {
                printf("*** Unit %d, trigger %d: Internal trigno mismatch: group %d trigno %u should be %u\n", m->unit, m->trigger, i, m->trigno[i], tn), fBadDataCount++;
                printf("groups 0x%x\n", m->groupMask);
                m->PrintModule();
                m->error = 1;
             }
       }
          
  m->complete = true;
  
  //printf("Complete: "); Print();
}

void UnpackVF48::CompleteEvent(VF48event* e)
{
   if (fModulesMask == 0) {
      // empty event
      e->complete = true;
      return;
   }

   for (int i=0; i<fNumModules; i++)
      if (e->modules[i]) {
         if (e->modules[i]->error > e->error)
            e->error = e->modules[i]->error;
         if (e->modules[i]->complete)
            e->modulesMask |= (1<<i);
      }

   //printf("complete event: mask 0x%04x vs 0x%04x\n", e->modulesMask, fModulesMask);
   
   if (e->modulesMask == fModulesMask)
      e->complete = true;
}

void UnpackVF48::AddToEvent(VF48module* m)
{
  VF48event* e = FindEvent(m->unit, m->timestamps[m->tgroup]); // find event corresponding to this time, if not found create new event
  //printf("For unit %d, trigger %d, ts %f, Find event: ", m->unit, m->trigger, m->timestamps[m->tgroup]); e->PrintSummary();

  if (e->modules[m->unit] != NULL)
    {
      printf("*** Unit %d has duplicate data for trigger %d timestamp %.6f ms: FindEvent yelds trigger %d timestamp %.6f ms, diff %.6f ms\n", m->unit, m->trigger, m->timestamps[m->tgroup]*1000.0, e->modules[m->unit]->trigger, e->timestamp*1000.0, (m->timestamps[m->tgroup] - e->timestamp)*1000.0), fBadDataCount++;

      PrintAllEvents();

#if 1
      m->PrintModule();
      printf("to event:\n");
      e->PrintSummary();
#endif
      // we are not adding this module to this event,
      // but somebody has to delete the module data.
      // We do it here.
      delete m;
      return;
    }

  e->modules[m->unit] = m;

  if (m->error > e->error)
    e->error = m->error;

  // PrintAllEvents();
}

void PrintVF48word(int unit, int i, uint32_t w)
{
  switch (w & 0xF0000000)
    {
    default: // unexpected data
      {
        printf("unit %d, word %5d: 0x%08x: Unknown data\n", unit, i, w);
        break;
      }
    case 0x80000000: // event header
      {
        uint32_t trigNo = w & 0x00FFFFFF;
        printf("unit %d, word %5d: 0x%08x: Event header:  unit %d, trigger %d\n", unit, i, w, unit, trigNo);
        break;
      }
    case 0xA0000000: // time stamp
      {
        uint32_t ts = w&0x00FFFFFF;
        printf("unit %d, word %5d: 0x%08x: Timestamp %d\n", unit, i, w, ts);
        break;
      }
    case 0xC0000000: // channel header
      {
        int group = (w&0x70)>>4;
        int chan = (w&0x7) | (group<<3);
        printf("unit %d, word %5d: 0x%08x: Group %d, Channel %2d\n", unit, i, w, group, chan);
        break;
      }
    case 0x00000000: // adc samples
      {
        if (w==0) // 64-bit padding
          {
            printf("unit %d, word %5d: 0x%08x: 64-bit padding\n", unit, i, w);
            break;
          }

        int sample1 = w & 0x3FFF;
        int sample2 = (w>>16) & 0x3FFF;
        printf("unit %d, word %5d: 0x%08x: samples 0x%04x 0x%04x (%d %d)\n", unit, i, w, sample1, sample2, sample1, sample2);
        break;
      }
    case 0x40000000: // time analysis
      {
        int t = w & 0x00FFFFFF;
        printf("unit %d, word %5d: 0x%08x: Time %d\n", unit, i, w, t);
        break;
      }
    case 0x50000000: // charge analysis
      {
        int c = w & 0x00FFFFFF;
        printf("unit %d, word %5d: 0x%08x: Charge %d\n", unit, i, w, c);
        break;
      }
    case 0xE0000000: // event trailer
      {
        uint32_t trigNo2 = w & 0x00FFFFFF;
        printf("unit %d, word %5d: 0x%08x: Event trailer: unit %d, trigger %d\n", unit, i, w, unit, trigNo2);
        break;
      }
    }
}

void UnpackVF48::ResetStreamBuffer()
{
  for (int i=0; i<VF48_MAX_MODULES; i++) {
    wgrp[i] = 0;
    wdiscarded[i] = 0;
    for (int j=0; j<VF48_MAX_GROUPS; j++) {
      wmax[i][j] = 0;
      wptr[i][j] = 0;
      wacc[i][j] = 0;
    }
  }
}

void UnpackVF48::ResetEventBuffer()
{
  //PrintAllEvents();

  for (unsigned i=0; i<fBuffer.size(); i++) {
     VF48event *e = fBuffer[i];
     delete e;
  }
  fBuffer.clear();
}

VF48module* UnpackVF48::FindModule(int unit, int group, double timestamp)
{
  //printf("find module %d, group %d, timestamp %f\n", unit, group, timestamp);

  VF48event* e = FindEvent(unit, timestamp, true);

  if (e) {
    //printf("e %p, module %d is %p\n", e, unit, e->modules[unit]);
    if (e->modules[unit])
      return e->modules[unit];

    VF48module* m = new VF48module(unit);
    e->modules[unit] = m;
    return m;
  }

  VF48module* m = new VF48module(unit);
  AddToEvent(m);
  return m;
}

static void DumpWords(const uint32_t* wptr, int wcount)
{
   for (int i=0; i<wcount; i++)
      printf("word[%d]: 0x%08x\n", i, wptr[i]);
}

void UnpackVF48::UnpackEvent(int unit, int group, const uint32_t data[], int wcount)
{
  #define SWAP 0
  #if SWAP
  static bool swap = SWAP;
  #endif
  assert(unit>=0);
  assert(unit<VF48_MAX_MODULES);

  assert(group>=0);
  assert(group<VF48_MAX_GROUPS);

  if (fDisasmStream) {
     printf("UnpackVF48A: UnpackEvent unit %d, group %d, wcount %d, first word: 0x%08x, last word: 0x%08x\n", unit, group, wcount, data[0], data[wcount-1]);
  }

  bool disasm = fDisasmStructure;
  bool disasmSamples = fDisasmSamples;

  bool doexit = false;

  VF48module* m = NULL;

  int chan = -1;
  int headerTrigNo = -1;
  int trailerTrigNo = -1;

  int timestampCount  = 0;
  int timestampFlag   = 0;
  uint32_t timestamp1 = 0;
  uint32_t timestamp2 = 0;

  for (int i=0; i<wcount; i++)
    {
      uint32_t w = data[i];

#if SWAP
      if (((w&0xF0FFFFFF)==0xF0FFFFFF) && (w != 0xFFFFFFFF))
         swap = true;

      if (swap && (w != 0xdeadbeef)) {
         uint32_t w0 = w&0x000000FF;
         uint32_t w1 = w&0x0000FF00;
         uint32_t w2 = w&0x00FF0000;
         uint32_t w3 = w&0xFF000000;
         w = (w0<<(16+8)) | (w1<<8) | (w2>>8) | (w3>>(16+8));
         //printf("swap 0x%08x -> 0x%08x\n", data32[i], w);
      }
#endif
      
      switch (w & 0xF0000000)
        {
        default: // unexpected data
          {
            if (w == 0xdeaddead) // skip the buffer padding
              {
                if (disasm)
                  printf("unit %d, word %5d: 0x%08x: reading from empty EB FIFO\n", unit, i, w);
                break;
              }
            
            if (w == 0xdeadbeef) // dma marker
              {
                if (disasm)
                  printf("unit %d, word %5d: 0x%08x: DMA marker\n", unit, i, w);
                break;
              }
            
            if (m)
              if (!m->channels[chan].complete)
                if (disasm)
                  printf("unit %d, adc samples: %d\n", unit, m->channels[chan].numSamples);
            
            if (disasm)
              printf("unit %d, word %5d: 0x%08x: unexpected data\n", unit, i, w);
            
            if (m)
              m->error = 1;
            
            fBadDataCount++;
            printf("*** Unit %d, group %d, trigger %d: Unexpected data at %5d: 0x%08x, skipping to next event header\n", unit, group, headerTrigNo, i, w);
            
            //accept = false;
            break;
          }
        case 0xF0000000: // switch to data from different group
          {
            int group = (w & 0x00000007);
            
            if (disasm)
              printf("unit %d, word %5d: 0x%08x: switch to data from group %d\n", unit, i, w, group);

	    // do nothing - should not encounter these - we are here after the demuxer
            break;
          }
        case 0x80000000: // event header
          {
             uint32_t trigNo = w & 0x00FFFFFF;

             if (disasm)
	       printf("unit %d, word %5d: 0x%08x: Event header:  unit %d, group %d, trigger %d\n", unit, i, w, unit, group, trigNo);

#if 0
             if (0 && trigNo > 1) {
                if (trigNo != c->headerTrigNo+1) {
                   printf("*** Unit %d, group %d, trigger %d: Out of sequence trigger %d should be %d\n", unit, group, trigNo, trigNo, c->headerTrigNo+1);
                   fBadDataCount++;
                }
             }
#endif

	     headerTrigNo = trigNo;

	     //printf("*** Unit %d, group %d, trigger %d: Event header for trigger %d without an event trailer for previous trigger %d, group %d\n", unit, group, trigNo, trigNo, c->headerTrigNo, c->group);
	     //fBadDataCount++;

             break;
          }
        case 0xA0000000: // time stamp
           {
              uint32_t ts = w&0x00FFFFFF;
              
              if (disasm)
                 printf("unit %d, word %5d: 0x%08x: Timestamp %d, count %d\n", unit, i, w, ts, timestampCount);
              
              if (timestampCount == 0)
		timestamp1 = ts;
              else if (timestampCount == 1)
		timestamp2 = ts;
              else {
                printf("*** Unit %d, group %d, trigger %d: Unexpected timestamp count: %d\n", unit, group, headerTrigNo, timestampCount);
		fBadDataCount++;
                if (m)
                   m->error = 1;
	      }
              
              timestampCount++;
              timestampFlag = 1;
           }
           break;
        case 0xC0000000: // channel header
          {
            int merror = 0;

            int cgroup = (w&0x70)>>4;

            if (group != 0) { // definitely new mode
               if (cgroup != 0) { // group number in 0xC header is always zero for new data
                  fBadDataCount++;
                  merror = 1;
                  printf("*** Unit %d, group %d, trigger %d: data at %5d: 0x%08x, Invalid cgroup number %d should be zero\n", unit, group, headerTrigNo, i, w, cgroup);
               }
            }

            if (cgroup != 0) { // definitely old data
               if (group != 0) { // old data should always come from data buffer for group 0
                  fBadDataCount++;
                  merror = 1;
                  printf("*** Unit %d, group %d, trigger %d: data at %5d: 0x%08x, Invalid group number %d should be zero\n", unit, group, headerTrigNo, i, w, group);
               }
            }

            if (cgroup == 0)
               cgroup = group;

            chan = (w&0x7) | (cgroup<<3);

            if (disasm)
               printf("unit %d, word %5d: 0x%08x: Group %d, Channel %2d\n", unit, i, w, cgroup, chan);

            if ((chan < 0) || (chan >= VF48_MAX_CHANNELS)) {
              printf("*** Unit %d, group %d, trigger %d: data at %5d: 0x%08x, Bad channel number %d\n", unit, group, headerTrigNo, i, w, chan);
              merror = 1;
	      fBadDataCount++;
	      break;
	    }

            if (timestampCount) {
	      uint64_t ts48 = timestamp1 | (((uint64_t)timestamp2) << 24);
                  
              if (ts48 == 0 && headerTrigNo == 1 && ts_first[unit][group] != 0) {
                  printf("*** Unit %d, group %d, trigger %d: Group %d unexpected event counter and timestamp reset!\n", unit, group, headerTrigNo, group);
		  fBadDataCount++;
                  merror = 1;
		  ts_first[unit][group] = 0;
              }

	      if (fTimestampResyncEnable) {
		if (ts48 == 0 && headerTrigNo == 1)
		  {
		    timestamp1 = 1;
		    ts48 = 1;
		  }
		
		if (!ts_first[unit][group])
		  {
		    ts_first[unit][group] = ts48;
		  }
	      }
              
	      if (ts48 == ts_last[unit][cgroup])
		{
                  printf("*** Unit %d, group %d, trigger %d: Group %d has invalid timestamp %d, should be more than %d\n", unit, group, headerTrigNo, group, (int)ts48, (int)ts_last[unit][cgroup]);
		  fBadDataCount++;
                  merror = 1;
		  ts48 = ts_last[unit][cgroup] + (int)(fCoinc*fFreq[unit])+1;
		}
	      
	      ts_last[unit][cgroup] = ts48;

	      double timestamp = (ts48 - ts_first[unit][group])/fFreq[unit];

	      if (!m) {
		m = FindModule(unit, group, timestamp);
		
		m->unit = unit;
		m->trigger = headerTrigNo;

                //if (fDisasmStream)
                //printf("unit %d, group %d, timestamp %f -> module %p\n", unit, group, timestamp, m);
	      }

#if 0
              if (headerTrigNo < 4) {
                 printf("AAA unit %d, group %d, trigNo %d, ts 0x%x 0x%x, base 0x%08x, ts %f\n", unit, group, headerTrigNo, timestamp2, timestamp1, (uint32_t)(ts_first[unit][group]&0xFFFFFFFF), timestamp);

                 if (headerTrigNo == 2)
                    exit(123);
              }
#endif

              m->trigno[cgroup] = headerTrigNo;
	      m->timestamp64[cgroup] = ts48;
	      m->timestamps[cgroup] = timestamp;
              m->tgroup = cgroup;
              if (merror)
                 m->error = merror;
	    }

            if (m->channels[chan].numSamples != 0)
               {
                  printf("*** Unit %d, group %d, trigger %d: Duplicate data for channel %d: already have %d samples\n", unit, group, headerTrigNo, chan, m->channels[chan].numSamples);
                  fBadDataCount++;
                  m->error = 1;
               }

            timestampCount = 0;
            m->groupMask |= (1<<cgroup);
            if (timestampFlag)
               m->timestampGroupMask |= (1<<cgroup);
            timestampFlag = 0;
            m->channels[chan].complete   = false;
            m->channels[chan].numSamples = 0;
            m->channels[chan].channel    = chan;

	    m->completeGroupMask |= (1<<cgroup);
          }
          break;
        case 0x00000000: // adc samples
           {
	     if (!m || (chan<0) || (chan>=VF48_MAX_CHANNELS))
                 {
                    if (disasm)
                       printf("unit %d, word %5d: 0x%08x: out of sequence adc samples\n", unit, i, w);

                    if (m)
                       m->error = 1;
                    fBadDataCount++;
                    printf("*** Unit %d, group %d, trigger %d: Unexpected adc samples data at %5d: 0x%08x (no module %p or bad channel number %d)\n", unit, group, headerTrigNo, i, w, m, chan);
                    
                    //accept[unit] = false;
                    break;
                 }
              
              VF48channel *cc = &m->channels[chan];
              
              int sample1 = w & 0x3FFF;
              int sample2 = (w>>16) & 0x3FFF;
              
              if (disasmSamples)
                 printf("unit %d, word %5d: 0x%08x: samples %4d: 0x%04x 0x%04x (%d %d)\n", unit, i, w, cc->numSamples, sample1, sample2, sample1, sample2);

              if (m->channels[chan].complete)
                 {
                    if (1)
                       {
                          fBadDataCount++;
                          m->error = 1;
                          
                          static int xi = 0;
                          static int xc = 0;
                          
                          if (xi+1 != i)
                             {
                                if (xc > 0)
                                   printf("*** Previously ignored %d out of sequence adc samples\n", xc);
                                
                                printf("*** Unit %d, group %d, trigger %d: Out of sequence adc samples for channel %d at %d, word 0x%08x\n", unit, group, headerTrigNo, cc->channel, i, w);
                                xc = 0;
                             }
                          
                          xi = i;
                          xc ++;
                       }
                 }
              else if (cc->numSamples < VF48_MAX_SAMPLES-2)
                 {
                    cc->samples[cc->numSamples++] = sample1;
                    cc->samples[cc->numSamples++] = sample2;

                    m->presentMask[chan>>3] |= (1<<(chan&7));
                 }
              else
                 {
                   printf("*** Unit %d group %d channel %d has too many samples: %d\n", unit, group, chan, cc->numSamples);
		   fBadDataCount++;
		   cc->numSamples += 2;
		   m->error = 1;
                 }
           }
           break;
        case 0x40000000: // time analysis
           {
	     if (!m || (chan<0) || (chan>=VF48_MAX_CHANNELS))
                 {
                    if (disasm)
                       printf("unit %d, word %5d: 0x%08x: out of sequence data\n", unit, i, w);

                    if (m)
                       m->error = 1;
                    fBadDataCount++;
                    printf("*** Unit %d, group %d, trigger %d: Unexpected time data at %5d: 0x%08x (no module %p or bad channel number %d)\n", unit, group, headerTrigNo, i, w, m, chan);
                    
                    //accept[unit] = false;
                    break;
                 }

              if (!m->channels[chan].complete)
                 if (disasm)
                    printf("unit %d, adc samples: %d\n", unit, m->channels[chan].numSamples);

              int t = w & 0x00FFFFFF;
              if (disasm)
                 printf("unit %d, word %5d: 0x%08x: Time %d\n", unit, i, w, t);
              m->channels[chan].time = t;
              m->channels[chan].complete = true;
           }
          break;
        case 0x50000000: // charge analysis
           {
	     if (!m || (chan<0) || (chan>=VF48_MAX_CHANNELS))
                 {
                    if (disasm)
                       printf("unit %d, word %5d: 0x%08x: out of sequence data\n", unit, i, w);

                    if (m)
                       m->error = 1;
                    fBadDataCount++;
                    printf("*** Unit %d, group %d, trigger %d: Unexpected charge data at %5d: 0x%08x (no module %p or bad channel number %d)\n", unit, group, headerTrigNo, i, w, m, chan);
                    
                    //accept[unit] = false;
                    break;
                 }

              if (!m->channels[chan].complete)
                 if (disasm)
                    printf("unit %d, adc samples: %d\n", unit, m->channels[chan].numSamples);
              int charge = w & 0x00FFFFFF;
              if (disasm)
                 printf("unit %d, word %5d: 0x%08x: Charge %d\n", unit, i, w, charge);
              
              if ((w & 0x0F000000) == 0x00000000)
                 m->hitMask[chan>>3] = w & 0xFF;
              
              m->channels[chan].charge = charge;
              m->channels[chan].complete = true;
           }
           break;
        case 0xE0000000: // event trailer
           {
	     if (!m || (chan<0) || (chan>=VF48_MAX_CHANNELS))
                 {
                    if (disasm)
                       printf("unit %d, word %5d: 0x%08x: out of sequence data\n", unit, i, w);

                    if (m)
                       m->error = 1;
                    fBadDataCount++;
                    printf("*** Unit %d, group %d, trigger %d: Unexpected event trailer at %5d: 0x%08x (no module %p or bad channel number %d)\n", unit, group, headerTrigNo, i, w, m, chan);

                    //DumpWords(data, wcount);
                    //exit(123);
                    
                    //accept[unit] = false;
                    break;
                 }

              if (!m->channels[chan].complete)
                 if (disasm)
                    printf("unit %d, adc samples: %d\n", unit, m->channels[chan].numSamples);
              
              uint32_t trigNo = w & 0x00FFFFFF;

              trailerTrigNo = trigNo;

              if (disasm)
		printf("unit %d, word %5d: 0x%08x: Event trailer: unit %d, group %d, trigger %d, module complete mask 0x%x\n", unit, i, w, unit, group, trigNo, m->completeGroupMask);
              
              m->channels[chan].complete = true;
              
              if (trigNo != (uint32_t)headerTrigNo)
                 {
                    printf("*** Unit %d, group %d, trigger %d: event trailer trigger mismatch: see %d, should be %d\n", unit, group, headerTrigNo, trigNo, m->trigger);
                    fBadDataCount++;
                    m->error = 1;
                 }

              if (m->completeGroupMask == fGroupEnabled[unit]) {
                 CompleteModule(m);
		 m = NULL;
              }
           }
           break;
        }
    }
  
  if (doexit)
     exit(123);
}

int UnpackVF48::NewGroup(int unit)
{
  int grp = wgrp[unit];
  assert(grp>=0 && grp<VF48_MAX_GROUPS);

  if (!wbuf[unit][grp]) {
    wmax[unit][grp] = VF48_MAX_SAMPLES*2*VF48_MAX_CHANNELS + 1024;
    wbuf[unit][grp] = (uint32_t*)malloc(wmax[unit][grp]);
    assert(wbuf[unit][grp]);
    wptr[unit][grp] = 0;
    wacc[unit][grp] = false;
    wdiscarded[unit] = 0;
  }

  return grp;
}

void UnpackVF48::UnpackStream(int unit, const void* data, int size)
{
  int       size32 = size;
  const uint32_t *data32 = (const uint32_t*)data;

  //printf("UnpackVF48: unit %d, size %d\n", unit, size);

  bool doDump = false;

  assert(unit>=0);
  assert(unit<VF48_MAX_MODULES);

  int grp = NewGroup(unit);

  for (int i=0; i<size32; i++)
    {
      uint32_t w = data32[i];

      //printf("unit %d, at %d, word 0x%08x\n", unit, i, w);

      if ((w & 0xFFFFFFF0) == 0xFFFFFFF0) // group marker
        {
          int group = (w & 0x00000007);

          //if (fDisasmStream)
          //printf("unit %d, word %5d: 0x%08x: switch to data from group %d\n", unit, i, w, group);

          wgrp[unit] = group;
          grp = NewGroup(unit);

          //if (wptr[unit][grp] == 0)
	  //wbuf[unit][grp][wptr[unit][grp]++] = w;
        }
      else if ((w & 0xF0000000) == 0x80000000) // event header
        {
          if (wdiscarded[unit]) {
            printf("*** Unit %d, group %d, at word %d, skipped %d words preceeding event header 0x%08x\n", unit, grp, i, wdiscarded[unit], w);
            doDump = true;
          }

	  if (fDisasmStream) {
             printf("UnpackVF48A: Header  0x%08x, unit %d, group %d, at word %d\n", w, unit, grp, i);
	  }

          if (wptr[unit][grp] > 1) {
            UnpackEvent(unit, grp, wbuf[unit][grp], wptr[unit][grp]);
            wptr[unit][grp] = 0;       
          }

          wacc[unit][grp] = true;
          wdiscarded[unit] = 0;

          wbuf[unit][grp][wptr[unit][grp]++] = w;
        }
      else if ((w & 0xF0000000) == 0xE0000000) // event trailer
        {
          wbuf[unit][grp][wptr[unit][grp]++] = w;

	  if (fDisasmStream) {
             printf("UnpackVF48A: Trailer 0x%08x, unit %d, group %d, at word %d\n", w, unit, grp, i);
	  }

          if (0 && wptr[unit][grp] == 1) {
             printf("*** Unit %d, group %d, at word %d, lonely event trailer 0x%08x\n", unit, grp, i, w);
             doDump = true;
          }

          if (wacc[unit][grp]) {
             UnpackEvent(unit, grp, wbuf[unit][grp], wptr[unit][grp]);
          } else {
             printf("*** Unit %d, group %d, at word %d, missing event header for trailer 0x%08x, %d data words were discarded\n", unit, grp, i, w, wdiscarded[unit]);
             wdiscarded[unit] = 0;
          }

          wptr[unit][grp] = 0;
          wacc[unit][grp] = false;
        }
      else if (wacc[unit][grp])
        {
          wbuf[unit][grp][wptr[unit][grp]++] = w;
        }
      else
        {
          //if (fDisasmStream)
          //printf("unit %d, word %5d: 0x%08x - skipped waiting for event header\n", unit, i, w);
          
          if (w != 0xdeadbeef) // DMA marker, do not count as skipped
             if (w != 0xdeaddead) {
                wdiscarded[unit]++;
                //printf("*** Unit %d, group %d, at word %d, skipped word 0x%08x\n", unit, grp, i, w);
             }
        }
    }

  if (0 && doDump) {
     printf("*** Unit %d, bank dump, %d words:\n", unit, size32);
     DumpWords(data32, size32);
  }
}

// end


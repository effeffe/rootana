//
// ALPHA-g TPC
//
// GRIF-16/ALPHA-16 ADC functions
//
// Class functions for Alpha16.h
//

#include "Alpha16.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#define MEMZERO(array) memset((array),0,sizeof(array))

static uint8_t getUint8(const void* ptr, int offset)
{
   return *(uint8_t*)(((char*)ptr)+offset);
}

static uint16_t getUint16(const void* ptr, int offset)
{
   uint8_t *ptr8 = (uint8_t*)(((char*)ptr)+offset);
   return ((ptr8[0]<<8) | ptr8[1]);
}

static uint32_t getUint32(const void* ptr, int offset)
{
   uint8_t *ptr8 = (uint8_t*)(((char*)ptr)+offset);
   return (ptr8[0]<<24) | (ptr8[1]<<16) | (ptr8[2]<<8) | ptr8[3];
}

// CRC16 from http://stackoverflow.com/questions/10564491/function-to-calculate-a-crc16-checksum
static unsigned short crc16(const unsigned char* data_p, unsigned char length){
   unsigned char x;
   unsigned short crc = 0xFFFF;
   
   while (length--){
      x = crc >> 8 ^ *data_p++;
      x ^= x>>4;
      crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x <<5)) ^ ((unsigned short)x);
   }
   return crc;
}

void Alpha16Packet::Unpack(const void*ptr, int bklen8)
{
   /*
     ALPHA16 UDP packet data format from Bryerton: this is packet version 1:
     
     Date: Mon, 20 Jun 2016 15:07:08 -0700
     From bryerton@triumf.ca  Mon Jun 20 15:07:05 2016
     From: Bryerton Shaw <bryerton@triumf.ca>
     To: Konstantin Olchanski <olchansk@triumf.ca>
     Subject: Re: eta on udp data?
     
     Hi Konstantin,
     
     Just trying to iron out one last bug, it was (is?) locking up if I
     saturated the link, but I think I just resolved that! So we ve got all
     16 channels outputting pretty steadily up to 40kHz or so, if I reduce
     it to 3 channels, we can get 150+ kHz event rates per channel.
     
     I am going to add a checksum onto the packet structure but it looks as
     follows, broken down by BYTE offset
      
      0 Packet Type - Currently fixed at0x01
      1 Packet Version - Currently fixed at0x01
      2 Accepted Trigger MSB - Inside the firmware logic Accepted Trigger is unsigned 32bits, providing the lower 16bits here for useful as a dropped UDP packet check
      3 Accepted Trigger LSB
      4 MSB Hardware ID - Currently the lower 48 bits of the ArriaV ChipID. It will be the MAC address shortly however
      5 "" ""
      6 "" ""
      7 "" ""
      8 "" ""
      9 LSB Hardware ID
      10 Build Timestamp (UNIX timestamp, aka seconds since Jan 01, 1980 UTC)
      11 "" ""
      12 "" ""
      13 "" ""
      14 0x00
      15 0x00
      16 MSB Event Timestamp
      17 "" ""
      18 "" ""
      19 "" ""
      20 "" ""
      21 LSB Event Timestamp
      22 MSB Trigger Offset - Trigger Point in relation to the start of the waveform packet. Signed 32bit integer
      23
      24
      25 LSB Trigger Offset
      26 ModuleID - Logical Identifier for the Alpha16 board. unsigned byte
      27 [7] Channel Type - Either 0 or 1, for BScint or Anode. The MSB of this byte
      27 [6:0] Channel ID - Unsigned 7bits, identifies the ADC channel (0-15) used
      28 MSB Sample Count - Unsigned 16 bit value indicating the number of samples (1-N)
      29 LSB Sample Count
      30 MSB First Waveform Sample - Signed 16 bit value
      31 LSB First Waveform Sample
      ....
      30 + (SampleCount*2) MSB Checksum
      31 + (SampleCount*2) LSB Checksum
      
      I will give you the checksum details in a moment, I am just adding it in
      now. Most likely will be a crc16 based on 1+x^2+x^15+x^16 .
      The byte positions may not be ideal, but we can sort that out.
      
      Cheers,
      
      Bryerton
   */
   
   bankLength = bklen8;
   packetType = getUint8(ptr, 0);
   packetVersion = getUint8(ptr, 1);
   acceptedTrigger = getUint16(ptr, 2);
   hardwareId = getUint32(ptr, 4);
   buildTimestamp = getUint32(ptr, 10);
   //int zero = getUint16(ptr, 14);
   eventTimestamp = getUint32(ptr, 18);
   triggerOffset = getUint32(ptr, 22);
   moduleId = getUint8(ptr, 26);
   int chanX = getUint8(ptr, 27);
   channelType = chanX & 0x80;
   channelId = chanX & 0x7F;
   nsamples = getUint16(ptr, 28);
   checksum = getUint16(ptr, 30 + nsamples*2);
   length = 32 + nsamples*2;
   
   xcrc16 = crc16((const unsigned char*)ptr, 32 + nsamples*2);
}

int Alpha16Packet::PacketType(const void*ptr, int bklen8)
{
   return getUint8(ptr, 0);
}

int Alpha16Packet::PacketVersion(const void*ptr, int bklen8)
{
   return getUint8(ptr, 1);
}

uint32_t Alpha16Packet::PacketTimestamp(const void*ptr, int bklen8)
{
   return getUint32(ptr, 18);
}

int Alpha16Packet::PacketChannel(const void*ptr, int bklen8)
{
   int chanX = getUint8(ptr, 27);
   //int channelType = chanX & 0x80;
   int channelId = chanX & 0x7F;
   return channelId;
}

void Alpha16Packet::Print() const
{
   printf("ALPHA16 data packet:\n");
   printf("  packet type:    0x%02x (%d)\n", packetType, packetType);
   printf("  packet version: 0x%02x (%d)\n", packetVersion, packetVersion);
   printf("  hwid:     0x%08x\n", hardwareId);
   printf("  buildts:  0x%08x\n", buildTimestamp);
   printf("  mod id:   0x%02x\n", moduleId);
   printf("  trig no:  0x%04x (%d)\n", acceptedTrigger, acceptedTrigger);
   printf("  event ts: 0x%08x (%d)\n", eventTimestamp, eventTimestamp);
   printf("  trig offset:   %d\n", triggerOffset);
   printf("  channel: type: %d, id: %d\n", channelType, channelId);
   printf("  nsamples: %d\n", nsamples);
   printf("  checksum: 0x%04x, computed checksum 0x%04x\n", checksum, xcrc16);
   printf("length: %d, bank length %d\n", length, bankLength);
};

void Alpha16Waveform::Unpack(const void* bkptr, int bklen8)
{
   int nsamples = getUint16(bkptr, 28);

   this->reserve(nsamples);
   this->clear();
   
   //printf("Unpacking: "); Print();
   for (int i=0; i<nsamples; i++) {
      int16_t v = getUint16(bkptr, 30 + i*2);
      //printf("sample %d: 0x%02x (%d)\n", i, v, v);
      this->push_back(v);
   }
};

Alpha16Event::Alpha16Event() // ctor
{
   Reset();
}

void Alpha16Event::Reset()
{
   eventNo = 0;
   eventTime = 0;
   prevEventTime = 0;
   MEMZERO(udpPresent);
   MEMZERO(udpEventTs);
   numChan = 0;
   error    = false;
   complete = false;
}
   
Alpha16Event::~Alpha16Event() // dtor
{
}

void Alpha16Event::Print() const
{
   printf("ALPHA16 event: %d, time %.0f (incr %.0f), channels: %d, error %d, complete %d\n", eventNo, eventTime, eventTime - prevEventTime, numChan, error, complete);
}

Alpha16EVB::Alpha16EVB() // ctor
{
   Reset();
}
   
void Alpha16EVB::Reset()
{
   //printf("Alpha16EVB::Reset!\n");
   fEventCount = 0;
   fHaveEventTs = false;
   MEMZERO(fFirstEventTs);
   fLastEventTs = 0;
   fConfModMap.clear();
   fConfNumChan = 0;
   fConfNumSamples = 0;
}

void Alpha16EVB::Configure(int runno)
{
   const int modmap[][20] = {
      { 269,  1,   2,   3,   4,   5,   6,   7,   8, 0 },
      { 264,  1,   2,   3,   4,  -5,  -6,  -7,  -8, 0 },
      { 257,  1,   2,   3,   4, -18,  -6,  -7,  -8, 9, 10, 11, 12, 17, 14, 15, 16, 0 },
      { 245,  1,   2,   3,   4, -19,  -6,  -7,  -8, 9, 10, 11, 12, 18, 14, 15, 16, 0 },
      { 238,  1,   2,   9,   4, -10,  -6, -14, -19, 0 },
      { 227, 14,  19,   9,   4, -10,  -6,  -7,  -8, 0 },
      { 220,  1,   2,   9,   4, -10,  -6,  -7,  -8, 0 },
      { 218,  1,   2,   8,   4,  -9, -10, -11, -12, 0 },
      { 194,  1,   2,  13,   4,  -9, -10, -11, -12, 0 },
      { 186,  1,   2,  12,   4, -10, -15, -17, -18, 0 },
      { 184, -9, -12,  -4, -11, -10, -15, -17, -18, 0 },
      { 183, -9, -12,  -4, -11, -14, -10, -15, -18, 0 },
      { 180, -9, -12,  -4,  -7,  -8, -10, -15, -16, 0 },
      { 177,  9,  12,   4,   7,   8,  10,  15,  16, 0 },
      { 173,  9,  12,   4,   6,   8,  10,  15,  16, 0 },
      { 171,  9,  12,   4,  19,   8,  10,  15,  16, 0 },
      { 158,  1,   2,   4,   7,  -8, -10, -15, -16, 0 },
      { 154, -1,  -2,  -4,  -7,  -8, -10, -15, -16, 0 },
      { 151,  1,   2,   4,   7,   8,  10,  15,  16, 0 },
      { 147,  1,   2,   3,   4,   6,   7,   8,  16, 0 },
      { 140,  4,  10,  13,  15,   1,  16,  17,   2, 0 },
      { 0 }
   };

   // ensure modmap is sorted by run number in descending order

   for (int i=0; modmap[i][0] != 0; i++) {
      assert(modmap[i][0] > modmap[i+1][0]);
   }

   int imap = -1;
   
   for (int i=0; modmap[i][0] != 0; i++) {
      if (runno >= modmap[i][0]) {
         imap = i;
         break;
      }
   }
   
   assert(imap >= 0);
   
   printf("Alpha16EVB::Configure: for run %d found map index %d for run %d\n", runno, imap, modmap[imap][0]);
   
   fConfModMap.clear();
   for (int j=1; modmap[imap][j] != 0; j++) {
      fConfModMap.push_back(modmap[imap][j]);
   }
   
   fConfNumChan = NUM_CHAN_ALPHA16 * fConfModMap.size();
}

#if 0
void Alpha16EVB::Print() const
{
   printf("EVB contents:\n");
   for (unsigned i=0; i<fEvents.size(); i++) {
      printf("Entry %d: ", i);
      fEvents[i]->Print();
   }
}
#endif

#if 0
bool Alpha16EVB::Match(const Alpha16Event* e, int imodule, uint32_t udpTs)
{
   return (e->udpEventTs[imodule] == udpTs);
}
#endif

//static const int chanmap_top[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
static const int chanmap_top[] = { 7, 15, 6, 14, 5, 13, 4, 12, 3, 11, 2, 10, 1, 9, 0, 8 };
static const int chanmap_bot[] = { 8, 0, 9, 1, 10, 2, 11, 3, 12, 4, 13, 5, 14, 6, 15, 7 };

void Alpha16EVB::AddBank(Alpha16Event* e, int xmodule, const void* bkptr, int bklen)
{
   int ymodule = -1;
   int top_bot = 0;
   
   for (unsigned x=0; x<fConfModMap.size(); x++)
      if (fConfModMap[x] == xmodule) {
         ymodule = x;
         top_bot = 1;
         break;
      } else if (fConfModMap[x] == -xmodule) {
         ymodule = x;
         top_bot = -1;
         break;
      }
   
   if (ymodule < 0)
      return;

   int xchan = Alpha16Packet::PacketChannel(bkptr, bklen);

   if (xchan<0 || xchan >= NUM_CHAN_ALPHA16) {
      // invalid channel number
      e->error = true;
      return;
   }

   int ychan = -1; 

   if (top_bot < 0)
      for (int i=0; i<16; i++)
         if (chanmap_bot[i] == xchan) {
            ychan = i;
            break;
         }

   if (top_bot > 0)
      for (int i=0; i<16; i++)
         if (chanmap_top[i] == xchan) {
            ychan = i;
            break;
         }

   assert(ychan>=0);

   int chan = ymodule*NUM_CHAN_ALPHA16 + ychan;

   if (e->udpPresent[chan]) {
      // duplicate udp packet
      e->error = true;
      return;
   }
   
   e->udpPacket[chan].Unpack(bkptr, bklen);
   e->waveform[chan].Unpack(bkptr, bklen);
   e->udpPresent[chan] = true;
   e->udpEventTs[chan] = e->udpPacket[chan].eventTimestamp;
   e->numChan++;
}

Alpha16Event* Alpha16EVB::NewEvent()
{
   Alpha16Event *e = new Alpha16Event();
   fEventCount++;
   e->eventNo = fEventCount;
   return e;
}

const double TSCLK = 0.1; // GHz

void Alpha16EVB::CheckEvent(Alpha16Event* e)
{
   int count = 0;
   unsigned num_samples = 0;

   for (int i=0; i<MAX_ALPHA16 * NUM_CHAN_ALPHA16; i++) {
      if (e->udpPresent[i]) {
         count++;
         if (num_samples == 0)
            num_samples = e->waveform[i].size();
         if (e->waveform[i].size() != num_samples) {
            // wrong number of ADC samples
            e->error = true;
         }
      }
   }

   if (fConfNumSamples == 0)
      fConfNumSamples = num_samples;

   if ((int)num_samples != fConfNumSamples) {
      // wrong number of ADC samples
      e->error = true;
   }

   if ((count == fConfNumChan) && (e->numChan == fConfNumChan)) {
      e->complete = true;
   }

   if (!fHaveEventTs && e->complete) {
      fHaveEventTs = true;
      // set timestamp offsets from the first complete event
      for (int i=0; i<MAX_ALPHA16 * NUM_CHAN_ALPHA16; i++) {
         if (e->udpPresent[i]) {
            fFirstEventTs[i] = e->udpEventTs[i];
            printf("XXX %d -> 0x%08x\n", i, fFirstEventTs[i]);
         }
      }
   }

   if (fHaveEventTs && e->complete) {
      uint32_t ets = 0;
      // check timestamps
      for (int i=0; i<MAX_ALPHA16 * NUM_CHAN_ALPHA16; i++) {
         if (e->udpPresent[i]) {
            uint32_t ts = e->udpEventTs[i] - fFirstEventTs[i];
            if (ets == 0)
               ets = ts;
            if (ts != ets && ts != ets+1 && ts+1 != ets) {
               printf("ts mismatch %d: 0x%08x vs 0x%08x diff %d\n", i, ts, ets, ts-ets);
               e->error = true;
            }
         }
      }
      e->eventTime = ets/TSCLK;
      e->prevEventTime = fLastEventTs/TSCLK;
      fLastEventTs = ets;
   }
}

#if 0
Alpha16Event* Alpha16EVB::FindEvent(int imodule, uint32_t udpTs)
{
   // loop over buffered events, look for match
   
   for (unsigned i=0; i<fEvents.size(); i++) {
      if (Match(fEvents[i], imodule, udpTs)) {
         return fEvents[i];
      }
   }
   
   Alpha16Event *e = new Alpha16Event();
   fEventCount++;
   e->eventNo = fEventCount;
   e->udpEventTs[imodule] = udpTs;
   if (fFirstEventTs[imodule] == 0) {
      fFirstEventTs[imodule] = udpTs;
      fLastEventTs[imodule] = fFirstEventTs[imodule];
   }
   e->eventTs = udpTs - fFirstEventTs[imodule];
   e->prevEventTs = fLastEventTs[imodule] - fFirstEventTs[imodule];
   
   fLastEventTs[imodule] = udpTs;
   
   //printf("Event %d ts 0x%08x: new!\n", e->eventNo, e->eventTs);
   
   fEvents.push_front(e);

   return e;
}
#endif

#if 0
Alpha16Event* Alpha16EVB::GetNextEvent()
{
   if (fEvents.size() < 1)
      return NULL;
   
   Alpha16Event* e = fEvents.back();
   
   if (e->complete || fEvents.size() > 10) {
      fEvents.pop_back();
      return e;
   }
  
   return NULL;
}
#endif

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

//
// ALPHA-g TPC
//
// GRIF-16/ALPHA-16 ADC functions
//
// Class definitions
//

#ifndef ALPHA16H
#define ALPHA16H

#include <stdint.h>
#include <vector>

struct Alpha16Packet
{
   int bankLength;
   int packetType;
   int packetVersion;
   int acceptedTrigger;
   uint32_t hardwareId;
   uint32_t buildTimestamp;
   uint32_t eventTimestamp;
   uint32_t triggerOffset;
   int moduleId;
   int channelType;
   int channelId;
   int nsamples;
   int checksum;
   int length;
   int xcrc16;

   static int PacketType(const void*ptr, int bklen8);
   static int PacketVersion(const void*ptr, int bklen8);
   static uint32_t PacketTimestamp(const void*ptr, int bklen8);
   static int PacketChannel(const void*ptr, int bklen8);
   
   void Unpack(const void* bkptr, int bklen8);
   void Print() const;
};

typedef std::vector<int16_t> Alpha16WaveformVector;

class Alpha16Waveform: public Alpha16WaveformVector
{
 public:
   void Unpack(const void* bkptr, int bklen8);
};

#define MAX_ALPHA16 32
#define NUM_CHAN_ALPHA16 16

struct Alpha16Event
{
   int      eventNo; // event counter, starting from 1
   double   eventTime; // event time stamp, in ns, time of first event is zero
   double   prevEventTime; // time of previous event, in ns, zero for first event

   bool     udpPresent[MAX_ALPHA16*NUM_CHAN_ALPHA16];  // udp packet received
   uint32_t udpEventTs[MAX_ALPHA16*NUM_CHAN_ALPHA16];  // timestamp from udp packet

   Alpha16Packet  udpPacket[MAX_ALPHA16*NUM_CHAN_ALPHA16];
   Alpha16Waveform waveform[MAX_ALPHA16*NUM_CHAN_ALPHA16];

   int  numChan;  // count of received channels

   bool error;    // event has an error
   bool complete; // event is complete
   
   Alpha16Event(); // ctor
   ~Alpha16Event(); // dtor

   void Reset();
   void Print() const;
};

struct Alpha16EVB
{
   int fEventCount; // event counter

   bool     fHaveEventTs;
   uint32_t fFirstEventTs[MAX_ALPHA16*NUM_CHAN_ALPHA16]; // udp timestamp of first event
   //uint32_t fLastEventTs[MAX_ALPHA16*NUM_CHAN_ALPHA16];  // udp timestamp of last seen event
   uint32_t fLastEventTs;

   int fConfNumChan;
   int fConfNumSamples;

   std::vector<int> fConfModMap;
   
   Alpha16EVB(); // ctor
   
   void Reset();
   //void Print() const;

   void Configure(int runno);

   Alpha16Event* NewEvent();
   void AddBank(Alpha16Event* e, int imodule, const void* bkptr, int bklen);
   void CheckEvent(Alpha16Event* e);

   //Alpha16Event* FindEvent(int imodule, uint32_t udpTs);
   //Alpha16Event* GetNextEvent();
   //static bool Match(const Alpha16Event* e, int imodule, uint32_t udpTs);
};

#endif

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

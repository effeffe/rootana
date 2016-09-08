// midasio.h

#ifndef MIDASIO_H
#define MIDASIO_H

#include <string>
#include <vector>

#if 1
#include <stdint.h>
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
#endif

#include "TMidasEvent.h"

#define TID_BYTE      1       /**< unsigned byte         0       255    */
#define TID_SBYTE     2       /**< signed byte         -128      127    */
#define TID_CHAR      3       /**< single character      0       255    */
#define TID_WORD      4       /**< two bytes             0      65535   */
#define TID_SHORT     5       /**< signed word        -32768    32767   */
#define TID_DWORD     6       /**< four bytes            0      2^32-1  */
#define TID_INT       7       /**< signed dword        -2^31    2^31-1  */
#define TID_BOOL      8       /**< four bytes bool       0        1     */
#define TID_FLOAT     9       /**< 4 Byte float format                  */
#define TID_DOUBLE   10       /**< 8 Byte float format                  */
#define TID_BITFIELD 11       /**< 32 Bits Bitfield      0  111... (32) */
#define TID_STRING   12       /**< zero terminated string               */

struct TMBank
{
   std::string name; ///< bank name, 4 characters max
   u32         type; ///< type of data, enum of TID_xxx
   u32         num_items; ///< number of data items
   u32         data_size; ///< total data size in bytes
   u32         data_offset; ///< offset of data for this bank in the event data buffer
};

struct TMEvent
{
   bool error; ///< event has an error - incomplete, truncated, inconsistent or corrupted
   
   u16 event_id; 
   u16 trigger_mask;
   u32 serial_number;
   u32 time_stamp;
   u32 data_size;

   u32 bank_header_flags;

   std::vector<TMBank> banks;

   std::vector<u8> data;

   TMidasEvent old_event;

public:
   void Print() const;
   TMBank* FindBank(const char* bank_name);
   char* GetBankData(const TMBank*);
   void DeleteBank(const TMBank*);
   void AddBank(const char* bank_name, int tid, const char* data, int size);
};

class TMReaderInterface
{
 public:
   virtual int Read(void* buf, int count) = 0;
   virtual int Close() = 0;
   virtual ~TMReaderInterface() {};
};

class TMWriterInterface
{
 public:
   virtual int Write(const void* buf, int count) = 0;
   virtual int Close() = 0;
   virtual ~TMWriterInterface() {};
};

TMReaderInterface* TMNewReader(const char* source);

TMEvent* TMReadEvent(TMReaderInterface* reader);
void TMWriteEvent(TMWriterInterface* writer, const TMEvent* event);

#endif

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */


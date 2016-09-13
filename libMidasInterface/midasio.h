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

#ifndef TID_LAST
/**
Data types Definition                         min      max    */
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
#define TID_ARRAY    13       /**< array with unknown contents          */
#define TID_STRUCT   14       /**< structure with fixed length          */
#define TID_KEY      15       /**< key in online database               */
#define TID_LINK     16       /**< link in online database              */
#define TID_LAST     17       /**< end of TID list indicator            */
#endif

struct TMBank
{
   std::string name; ///< bank name, 4 characters max
   u32         type; ///< type of data, enum of TID_xxx
   //u32         num_values; ///< number of data items
   u32         data_size; ///< total data size in bytes
   u32         data_offset; ///< offset of data for this bank in the event data buffer
};

struct TMEvent
{
   bool error; ///< event has an error - incomplete, truncated, inconsistent or corrupted
   bool found_all_banks; ///< all the banks in the event data have been discovered
   
   u16 event_id; 
   u16 trigger_mask;
   u32 serial_number;
   u32 time_stamp;
   u32 data_size;

   u32 bank_header_flags;

   std::vector<TMBank> banks;
   std::vector<u8> data;

   u32 bank_scan_position;

public:
   std::string HeaderToString() const;
   std::string BankListToString() const;
   std::string BankToString(const TMBank*) const;

   TMEvent(); // ctor
   TMEvent(const void* data, int data_size); // ctor
   void FindAllBanks();
   TMBank* FindBank(const char* bank_name);
   char* GetBankData(const TMBank*);
   void DeleteBank(const TMBank*);
   void AddBank(const char* bank_name, int tid, int num_items, const char* data, int size);
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
TMWriterInterface* TMNewWriter(const char* destination);

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


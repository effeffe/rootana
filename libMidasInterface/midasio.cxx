// midasio.cxx

#include "midasio.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h> // malloc()
#include <string.h> // memcpy()

#include <string>

class FileReader: public TMReaderInterface
{
 public:
   FileReader(const char* filename)
   {
      fFilename = filename;
      fFp = fopen(filename, "r");
      assert(fFp != NULL); // FIXME: check for error
   }

   int Read(void* buf, int count)
   {
      assert(fFp != NULL);
      return fread(buf, 1, count, fFp);
   }

   int Close()
   {
      assert(fFp != NULL);
      fclose(fFp);
      fFp = NULL;
      return 0;
   }

   std::string fFilename;
   FILE* fFp;
};

class PipeReader: public TMReaderInterface
{
 public:
   PipeReader(const char* pipename)
   {
      fPipename = pipename;
      fPipe = popen(pipename, "r");
      assert(fPipe != NULL); // FIXME: check for error
   }

   int Read(void* buf, int count)
   {
      assert(fPipe != NULL);
      return fread(buf, 1, count, fPipe); // FIXME: read pipe in a loop!
   }

   int Close()
   {
      assert(fPipe != NULL);
      pclose(fPipe); // FIXME: check error
      fPipe = NULL;
      return 0;
   }

   std::string fPipename;
   FILE* fPipe;
};

#ifdef HAVE_ZLIB

#include <zlib.h>

class ZlibReader: public TMReaderInterface
{
 public:
   ZlibReader(const char* filename)
   {
      fFilename = filename;
      fGzFile = gzopen(filename, "rb");
      //if (fGzFile == NULL)
      //   {
            //fLastErrno = -1;
            //fLastError = "zlib gzdopen() error";
            //return false;
      //   }
      assert(fGzFile != NULL); // FIXME: check for error
   }

   int Read(void* buf, int count)
   {
      assert(fGzFile != NULL);
      return gzread(fGzFile, buf, count);
   }

   int Close()
   {
      assert (fGzFile != NULL);
      gzclose(fGzFile);
      fGzFile = NULL;
      return 0;
   }

   std::string fFilename;
   gzFile fGzFile;
};
#endif

#include "lz4frame.h"

class Lz4Reader: public TMReaderInterface
{
public:
   Lz4Reader(TMReaderInterface* reader)
   {
      assert(reader);
      fReader = reader;

      printf("Lz4Reader::ctor!\n");

      LZ4F_errorCode_t errorCode = LZ4F_createDecompressionContext(&fContext, LZ4F_VERSION);
      if (LZ4F_isError(errorCode))
         printf("Can't create LZ4F context : %s", LZ4F_getErrorName(errorCode));

      fBufSize = 10*1024*1024;
      fBuf = (char*) malloc(fBufSize);
      fBufStart = 0;
      fBufHave = 0;

      fSrcBufSize = 10*1024*1024;
      fSrcBuf = (char*) malloc(fSrcBufSize);
      fSrcBufStart = 0;
      fSrcBufHave = 0;
   }

   ~Lz4Reader() {
      printf("Lz4Reader::dtor!\n");

      free(fBuf);
      fBuf = NULL;

      free(fSrcBuf);
      fSrcBuf = NULL;

      LZ4F_errorCode_t errorCode = LZ4F_freeDecompressionContext(fContext);
      if (LZ4F_isError(errorCode))
         printf("Error : can't free LZ4F context resource : %s", LZ4F_getErrorName(errorCode));
   }

   int ReadMore()
   {
      LZ4F_decompressOptions_t* dOptPtr = NULL;
               
      fBufStart = 0;
      fBufHave = 0;

      printf("read more: src buf: size %d, start %d, have %d\n", fSrcBufSize, fSrcBufStart, fSrcBufHave);

      size_t size = fBufSize;

      if (fSrcBufHave <= 0) {
         int rd = fReader->Read(fSrcBuf, fSrcBufSize);

         printf("read asked %d, got %d\n", fSrcBufSize, rd);

         if (rd <= 0)
            return rd;

         fSrcBufStart = 0;
         fSrcBufHave = rd;
      }
      
      size_t remaining = fSrcBufHave;
      
      int more = LZ4F_decompress(fContext, fBuf, &size, fSrcBuf + fSrcBufStart, &remaining, dOptPtr);

      printf("decompress: more %d, size %d, remaining %d\n", (int)more, (int)size, (int)remaining);

      if (remaining > 0) {
         fSrcBufStart += remaining;
         fSrcBufHave -= remaining;
      }

      fBufStart = 0;
      fBufHave = size;

      return size;
   }
   
   int Read(void* buf, int count)
   {
      printf("Lz4Reader::Read %d bytes!\n", count);

      char* cptr = (char*)buf;
      int clen = 0;

      while (clen < count) {
         int more = count - clen;

         printf("count %d, clen %d, fBufStart %d, fBufHave %d\n", count, clen, fBufStart, fBufHave);

         if (fBufHave > 0) {
            int copy = more;
            if (copy > fBufHave)
               copy = fBufHave;

            memcpy(cptr, fBuf+fBufStart, copy);
            cptr += copy;
            clen += copy;
            fBufStart += copy;
            fBufHave -= copy;

            // check if done reading
            if (clen == count)
               return clen;

            // buffer must be empty now
            assert(fBufHave == 0);
         }

         int rd = ReadMore();
         if (rd == 0)
            return clen;
         else if (rd < 0)
            return rd;
      }
      
      return clen;
   }
   
   int Close()
   {
      printf("Lz4Reader::Close!\n");
      return fReader->Close();
   }
   
   TMReaderInterface *fReader;
   LZ4F_decompressionContext_t fContext;

   int fBufSize;
   int fBufStart;
   int fBufHave;
   char* fBuf;

   int fSrcBufSize;
   int fSrcBufStart;
   int fSrcBufHave;
   char* fSrcBuf;
};

class FileWriter: public TMWriterInterface
{
 public:
   FileWriter(const char* filename)
   {
      fFilename = filename;
      fFp = fopen(filename, "w");
      assert(fFp != NULL); // FIXME: check for error
   }

   int Write(const void* buf, int count)
   {
      assert(fFp != NULL);
      return fwrite(buf, 1, count, fFp);
   }

   int Close()
   {
      assert(fFp != NULL);
      fclose(fFp);
      fFp = NULL;
      return 0;
   }

   std::string fFilename;
   FILE* fFp;
};

static int hasSuffix(const char*name,const char*suffix)
{
   const char* s = strstr(name,suffix);
   if (s == NULL)
      return 0;
   
   return (s-name)+strlen(suffix) == strlen(name);
}

TMReaderInterface* TMNewReader(const char* source)
{
   if (0) {
#if HAVE_ZLIB
   } else if (hasSuffix(source, ".gz")) {
      return new ZlibReader(source);
#endif
   } else if (hasSuffix(source, ".bz2")) {
      return new PipeReader((std::string("bzip2 -dc ") + source).c_str());
   } else if (hasSuffix(source, ".lz4")) {
      return new Lz4Reader(new FileReader(source));
   } else {
      return new FileReader(source);
   }
}

TMWriterInterface* TMNewWriter(const char* destination)
{
   if (0) {
#if 0
#if HAVE_ZLIB
   } else if (hasSuffix(source, ".gz")) {
      return new ZlibReader(source);
#endif
   } else if (hasSuffix(source, ".bz2")) {
      return new PipeReader((std::string("bzip2 -dc ") + source).c_str());
   } else if (hasSuffix(source, ".lz4")) {
      return new Lz4Reader(new FileReader(source));
#endif
   } else {
      return new FileWriter(destination);
   }
}

u16 GetU16(const void*ptr)
{
   return *(u16*)ptr;
}

u32 GetU32(const void*ptr)
{
   return *(u32*)ptr;
}

static void ZeroEvent(TMEvent* e)
{
   e->error = false;
   e->found_all_banks = false;
   e->event_id      = 0;
   e->trigger_mask  = 0;
   e->serial_number = 0;
   e->time_stamp    = 0;
   e->data_size     = 0;
   e->data_offset   = 0;
   e->bank_header_flags  = 0;
   e->bank_scan_position = 0;
}

TMEvent* TMReadEvent(TMReaderInterface* reader)
{
   bool gOnce = true;
   if (gOnce) {
      gOnce = false;
      assert(sizeof(u8)==1);
      assert(sizeof(u16)==2);
      assert(sizeof(u32)==4);
   }
   
   TMEvent* e = new TMEvent;

   ZeroEvent(e);

   const int event_header_size = 4*4;
   char event_header[event_header_size];

   int rd = reader->Read(event_header, event_header_size);

   if (rd == 0) { // end of file
      delete e;
      return NULL;
   } else if (rd != event_header_size) { // truncated data in file
      e->error = true;
      return e;
   }

   e->event_id      = GetU16(event_header+0);
   e->trigger_mask  = GetU16(event_header+2);
   e->serial_number = GetU32(event_header+4);
   e->time_stamp    = GetU32(event_header+8);
   e->data_size     = GetU32(event_header+12);

   e->bank_header_flags = 0;

   //if (!e->old_event.IsGoodSize()) { // invalid event size
   //   e->error = true;
   //   return e;
   //}

   int to_read = e->data_size;

   e->data.resize(event_header_size + to_read);

   memcpy(&e->data[0], event_header, event_header_size);

   rd = reader->Read(&e->data[event_header_size], to_read);

   if (rd != to_read) { // truncated data in file
      e->error = true;
      return e;
   }

   e->data_offset = event_header_size;

   return e;
}

void TMWriteEvent(TMWriterInterface* writer, const TMEvent* event)
{
   writer->Write(&(event->data[0]), event->data.size());
}

std::string TMEvent::HeaderToString() const
{
   char buf[1024];
   sprintf(buf, "event: id %d, mask 0x%04x, serial %d, time %d, size %d, error %d, banks %d", event_id, trigger_mask, serial_number, time_stamp, data_size, error, (int)banks.size());
   return buf;
}

std::string TMEvent::BankListToString() const
{
   std::string s;
   for (unsigned i=0; i<banks.size(); i++) {
      if (i>0)
         s += ",";
      s += banks[i].name;
   }
   return s;
}

std::string TMEvent::BankToString(const TMBank*b) const
{
   char buf[1024];
   sprintf(buf, "name \"%s\", type %d, size %d, offset %d\n", b->name.c_str(), b->type, b->data_size, b->data_offset);
   return buf;
}

TMEvent::TMEvent() // ctor
{
   ZeroEvent(this);
}

TMEvent::TMEvent(const void* xdata, int xdata_size)
{
   bool gOnce = true;
   if (gOnce) {
      gOnce = false;
      assert(sizeof(u8)==1);
      assert(sizeof(u16)==2);
      assert(sizeof(u32)==4);
   }

   ZeroEvent(this);

   const int event_header_size = 4*4;
   const char* event_header = (const char*)xdata;

   if (xdata_size < event_header_size) {
      error = true;
      return;
   }

   event_id      = GetU16(event_header+0);
   trigger_mask  = GetU16(event_header+2);
   serial_number = GetU32(event_header+4);
   time_stamp    = GetU32(event_header+8);
   data_size     = GetU32(event_header+12);
   data_offset   = event_header_size;

   bank_header_flags = 0;

   //if (!e->old_event.IsGoodSize()) { // invalid event size
   //   e->error = true;
   //   return e;
   //}

   int to_read = data_size;

   data.resize(event_header_size + to_read);

   memcpy(&data[0], xdata, event_header_size + to_read);
}



static int FindFirstBank(TMEvent* e)
{
   if (e->error)
      return -1;
   
   u32 off = e->data_offset;
   
   if (e->data.size() < off + 8) {
      e->error = true;
      return -1;
   }

   u32 bank_header_data_size = GetU32(&e->data[off]);
   u32 bank_header_flags     = GetU32(&e->data[off+4]);

   //printf("bank header: data size %d, flags 0x%08x\n", bank_header_data_size, bank_header_flags);

   if (bank_header_data_size + 8 != e->data_size) {
      e->error = true;
      return -1;
   }

   e->bank_header_flags = bank_header_flags;

   return off+8;
}

#if 0
static char xchar(char c)
{
   if (c>='0' && c<='9')
      return c;
   if (c>='a' && c<='z')
      return c;
   if (c>='A' && c<='Z')
      return c;
   return '$';
}
#endif

static int FindNextBank(TMEvent* e, int pos, TMBank** pb)
{
   if (e->error)
      return -1;

   int remaining = e->data.size() - pos;

   //printf("pos %d, event data_size %d, size %d, remaining %d\n", pos, e->data_size, (int)e->data.size(), remaining);

   if (remaining == 0) {
      // end of data, no more banks to find
      e->found_all_banks = true;
      return -1;
   }

   if (remaining < 8) {
      printf("too few bytes remaining at the end of event: %d\n", remaining);
      e->error = true;
      return -1;
   }

   int ibank = e->banks.size();
   e->banks.resize(ibank+1);

   TMBank* b = &e->banks[ibank];

   b->name.resize(4);
   b->name[0] = e->data[pos+0];
   b->name[1] = e->data[pos+1];
   b->name[2] = e->data[pos+2];
   b->name[3] = e->data[pos+3];
   b->name[4] = 0;

   u32 data_offset = 0;

   if (e->bank_header_flags & (1<<4)) {
      b->type      = GetU32(&e->data[pos+4+0]);
      b->data_size = GetU32(&e->data[pos+4+4]);
      data_offset = pos+4+4+4;
   } else {
      b->type      = GetU16(&e->data[pos+4+0]);
      b->data_size = GetU16(&e->data[pos+4+2]);
      data_offset = pos+4+2+2;
   }

   b->data_offset = data_offset;

   //printf("found bank at pos %d: %s\n", pos, e->BankToString(b).c_str());
   
   if (b->type < 1 || b->type >= TID_LAST) {
      printf("invalid tid %d\n", b->type);
      e->error = true;
      return -1;
   }

   int aligned_data_size = (b->data_size + 7) & ~7;

   //printf("data_size %d, alignemnt: %d %d, aligned %d\n", b->data_size, align, b->data_size%align, aligned_data_size);

   int npos = data_offset + aligned_data_size;

   //printf("pos %d, next bank at %d: [%c%c%c%c]\n", pos, npos, xchar(e->data[npos+0]), xchar(e->data[npos+1]), xchar(e->data[npos+2]), xchar(e->data[npos+3]));

   if (npos > (int)e->data.size()) {
      printf("invalid bank data size %d\n", b->data_size);
      e->error = true;
      return -1;
   }

   if (pb)
      *pb = b;

   return npos;
}

char* TMEvent::GetEventData()
{
   if (error)
      return NULL;
   return (char*)&data[data_offset];
}

char* TMEvent::GetBankData(const TMBank* b)
{
   if (error)
      return NULL;
   if (!b)
      return NULL;
   if (b->data_offset <= 0)
      return NULL;
   if (b->data_offset >= data.size())
      return NULL;
   return (char*)&data[b->data_offset];
}

TMBank* TMEvent::FindBank(const char* bank_name)
{
   if (error)
      return NULL;

   if (bank_name)
      for (unsigned i=0; i<banks.size(); i++) {
         if (banks[i].name == bank_name)
            return &banks[i];
      }

   //printf("found_all_banks %d\n", found_all_banks);

   if (found_all_banks)
      return NULL;
   
   int pos = bank_scan_position;

   if (pos == 0)
      pos = FindFirstBank(this);

   //printf("pos %d\n", pos);

   while (pos > 0) {
      TMBank* b = NULL;
      pos = FindNextBank(this, pos, &b);
      bank_scan_position = pos;
      //printf("pos %d, b %p\n", pos, b);
      if (pos>0 && b && bank_name) {
         if (b->name == bank_name)
            return b;
      }
   }

   found_all_banks = true;

   return NULL;
}

void TMEvent::FindAllBanks()
{
   if (found_all_banks)
      return;

   FindBank(NULL);

   assert(found_all_banks);
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */


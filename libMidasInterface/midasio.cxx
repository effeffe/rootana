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

   e->error = false;
   e->event_id = 0;
   e->trigger_mask = 0;
   e->serial_number = 0;
   e->time_stamp = 0;
   e->data_size = 0;
   e->bank_header_flags = 0;

   e->old_event.Clear();

   int rd = reader->Read((char*)e->old_event.GetEventHeader(), sizeof(TMidas_EVENT_HEADER));

   if (rd == 0) { // end of file
      delete e;
      return NULL;
   } else if (rd != sizeof(TMidas_EVENT_HEADER)) { // truncated data in file
      e->error = true;
      return e;
   }

   //if (fDoByteSwap)
   //   e->old_event.SwapBytesEventHeader();

   if (!e->old_event.IsGoodSize()) { // invalid event size
      e->error = true;
      return e;
   }

   rd = reader->Read((char*)e->old_event.GetData(), e->old_event.GetDataSize());

   if (rd != (int)e->old_event.GetDataSize()) { // truncated data in file
      e->error = true;
      return e;
   }

   e->old_event.SwapBytes(false);

   e->event_id = e->old_event.GetEventId();
   e->trigger_mask = e->old_event.GetTriggerMask();
   e->serial_number = e->old_event.GetSerialNumber();
   e->time_stamp = e->old_event.GetTimeStamp();
   e->data_size = e->old_event.GetDataSize();

   return e;
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */


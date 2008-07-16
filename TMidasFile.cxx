//
//  TMidasFile.cxx.
//

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

#include "TMidasFile.h"
#include "TMidasEvent.h"

TMidasFile::TMidasFile()
{
  uint32_t endian = 0x12345678;

  fFile = -1;
  fGzFile = NULL;
  fLastErrno = 0;

  fDoByteSwap = *(char*)(&endian) != 0x78;
}

TMidasFile::~TMidasFile()
{
  Close();
}

bool TMidasFile::Open(const char *filename)
{
  /// Opens a midas .mid file with given file name.
  /// \param [in] filename The file to open.
  /// \returns "true" for succes, "false" for error, use GetLastError() to see why

  if (fFile > 0)
    Close();

  fFilename = filename;

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

  fFile = open(filename, O_RDONLY | O_LARGEFILE);

  if (fFile <= 0)
    {
      fLastErrno = errno;
      fLastError = strerror(errno);
      return false;
    }

  int len = strlen(filename);
  if (filename[len-2]=='g' && filename[len-1]=='z')
    {
      // this is a compressed file
#ifdef HAVE_ZLIB
      fGzFile = new gzFile;
      (*(gzFile*)fGzFile) = gzdopen(fFile,"rb");
      if ((*(gzFile*)fGzFile) == NULL)
        {
          fLastErrno = -1;
          fLastError = "zlib gzdopen() error";
          return false;
        }
#else
      fLastErrno = -1;
      fLastError = "Do not know how to read compressed MIDAS files";
      return false;
#endif
    }

  return true;
}

static int readpipe(int fd, char* buf, int length)
{
  int count = 0;
  while (length > 0)
    {
      int rd = read(fd, buf, length);
      if (rd > 0)
        {
          buf += rd;
          length -= rd;
          count += rd;
        }
      else if (rd == 0)
        {
          return count;
        }
      else
        {
          return -1;
        }
    }
  return count;
}

bool TMidasFile::Read(TMidasEvent *midasEvent)
{
  /// \param [in] midasEvent Pointer to an empty TMidasEvent 
  /// \returns "true" for success, "false" for failure, see GetLastError() to see why

  midasEvent->Clear();

  int rd = 0;

  if (fGzFile)
#ifdef HAVE_ZLIB
    rd = gzread(*(gzFile*)fGzFile, (char*)midasEvent->GetEventHeader(), sizeof(EventHeader_t));
#else
    assert(!"Cannot get here");
#endif
  else
    rd = readpipe(fFile, (char*)midasEvent->GetEventHeader(), sizeof(EventHeader_t));

  if (rd == 0)
    {
      fLastErrno = 0;
      fLastError = "EOF";
      return false;
    }
  else if (rd != sizeof(EventHeader_t))
    {
      fLastErrno = errno;
      fLastError = strerror(errno);
      return false;
    }

  if (fDoByteSwap)
    midasEvent->SwapBytesEventHeader();

  if (!midasEvent->IsGoodSize())
    {
      fLastErrno = -1;
      fLastError = "Invalid event size";
      return false;
    }

  if (fGzFile)
#ifdef HAVE_ZLIB
    rd = gzread(*(gzFile*)fGzFile, midasEvent->GetData(), midasEvent->GetDataSize());
#else
    assert(!"Cannot get here");
#endif
  else
    rd = readpipe(fFile, midasEvent->GetData(), midasEvent->GetDataSize());

  if (rd != (int)midasEvent->GetDataSize())
    {
      fLastErrno = errno;
      fLastError = strerror(errno);
      return false;
    }

  midasEvent->SwapBytes(false);

  return true;
}

void TMidasFile::Close()
{
#ifdef HAVE_ZLIB
  if (fGzFile)
    gzclose(*(gzFile*)fGzFile);
  fGzFile = NULL;
#endif
  if (fFile > 0)
    close(fFile);
  fFile = -1;
  fFilename = "";
}

// end

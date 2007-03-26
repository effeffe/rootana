//
//  TMidasFile.cxx.
//

#include "TMidasFile.h"
#include "TMidasEvent.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

TMidasFile::TMidasFile()
{
  uint32_t endian = 0x12345678;

  fFile = -1;
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

  int rd = readpipe(fFile, (char*)midasEvent->GetEventHeader(), sizeof(EventHeader_t));
  if (rd != sizeof(EventHeader_t))
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

  rd = readpipe(fFile, midasEvent->GetData(), midasEvent->GetDataSize());
  if (rd != midasEvent->GetDataSize())
    {
      fLastErrno = errno;
      fLastError = strerror(errno);
      return false;
    }

  if (fDoByteSwap)
    midasEvent->SwapBytes(true);

  return true;
}

void TMidasFile::Close()
{
  if (fFile > 0)
    close(fFile);
  fFile = -1;
  fFilename = "";
}

// end

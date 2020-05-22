//
//  TMidasFile.cxx.
//

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h> // close()

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

#include "TMidasFile.h"
#include "TMidasEvent.h"

TMidasFile::TMidasFile()
{
  uint32_t endian = 0x12345678;

  fLastErrno = 0;

  fReader = NULL;

  fDoByteSwap = *(char*)(&endian) != 0x78;
}

TMidasFile::~TMidasFile()
{
  Close();
}

bool TMidasFile::Open(const char *filename)
{
  fFilename = filename;
  fReader = TMNewReader(filename);
  return true;
}

bool TMidasFile::Read(TMidasEvent *midasEvent)
{
  /// \param [in] midasEvent Pointer to an empty TMidasEvent 
  /// \returns "true" for success, "false" for failure, see GetLastError() to see why

  midasEvent->Clear();

  assert(fReader);

  int rd = fReader->Read((char*)midasEvent->GetEventHeader(), sizeof(TMidas_EVENT_HEADER));

  if (rd == 0)
    {
      fLastErrno = 0;
      fLastError = "EOF";
      return false;
    }
  else if (rd != sizeof(TMidas_EVENT_HEADER))
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

  assert(fReader);
  rd = fReader->Read((char*)midasEvent->GetData(), midasEvent->GetDataSize());

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
  if (fReader) {
    fReader->Close();
    delete fReader;
    fReader = NULL;
  }
  fFilename = "";
}

// end

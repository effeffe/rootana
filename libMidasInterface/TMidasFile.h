//
// TMidasFile.h.
//

#ifndef TMIDASFILE_H
#define TMIDASFILE_H

#include <string>

#include "midasio.h"

class TMidasEvent;

/// Reader for MIDAS .mid files

class TMidasFile
{
public:
  TMidasFile(); ///< default constructor
  ~TMidasFile(); ///< destructor

  bool Open(const char* filename); ///< Open input file
  void Close(); ///< Close input file
  bool Read(TMidasEvent *event); ///< Read one event from the file

  const char* GetFilename()  const { return fFilename.c_str();  } ///< Get the name of this file
  int         GetLastErrno() const { return fLastErrno; }         ///< Get error value for the last file error
  const char* GetLastError() const { return fLastError.c_str(); } ///< Get error text for the last file error

protected:

  std::string fFilename; ///< name of the currently open file
  std::string fOutFilename; ///< name of the currently open file

  int         fLastErrno; ///< errno from the last operation
  std::string fLastError; ///< error string from last errno

  bool fDoByteSwap; ///< "true" if file has to be byteswapped

  TMReaderInterface *fReader;
};

#endif // TMidasFile.h

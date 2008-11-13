//
// Access ODB via mhttpd http requests
//
// Name: HttpOdb.h
// Author: K.Olchanski, 12-Nov-2008
//
// $Id$
//

#ifndef INCLUDE_HttpOdb_H
#define INCLUDE_HttpOdb_H

#include "VirtualOdb.h"

///
/// Access to ODB through the MIDAS HTTP server mhttpd.
///
/// To enable this function, create an ODB string entry "/Custom/secret.html!" with blank value.
/// Then use the HttpOdb URL http://hostname:port/CS/secret.html
/// In this usage "secret.html" functions as an access password.
///

class HttpOdb : public VirtualOdb
{
 protected:
  const char* fUrl;
  int fDebug;

 public:
  /// Contructor from a base URL
  HttpOdb(const char* url);

  /// Destructor
  virtual ~HttpOdb();

  int      odbReadAny(   const char*name, int index, int tid,void* buf, int bufsize=0);
  int      odbReadInt(   const char*name, int index, int      defaultValue);
  uint32_t odbReadUint32(const char*name, int index, uint32_t defaultValue);
  bool     odbReadBool(  const char*name, int index, bool     defaultValue);
  double   odbReadDouble(const char*name, int index, double   defaultValue);
  const char* odbReadString(const char*name, int index, const char* defaultValue);
  int      odbReadArraySize(const char*name);

 protected:
  const char* jget(const char* path, int index);

 protected:
  char fRequestBuf[1024];
  char fReplyBuf[1024];
};

#endif
//end

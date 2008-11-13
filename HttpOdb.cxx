//
// Name: HttpOdb.cxx
// Author: K.Olchanski, 12-Nov-2008
//
// $Id$
//

#include <stdio.h>
#include <iostream>
#include <assert.h>

#include "HttpOdb.h"

HttpOdb::HttpOdb(const char* url) //ctor
{
  fDebug = 0;
  fUrl = strdup(url);
}

HttpOdb::~HttpOdb() // dtor
{
  if (fUrl)
    free((void*)fUrl);
  fUrl = NULL;
}

// GET /CS/F000.html?cmd=jset&odb=/equipment/mscb/settings/FGD/Feb_demand/power[0]&value=y HTTP/1.1

#include "TUrl.h"
#include "TSocket.h"

const char* HttpOdb::jget(const char* path, int index)
{
  TUrl xurl(fUrl);

  TSocket s(xurl.GetHost(), xurl.GetPort());

  //char *req = "GET /CS/F000.html?cmd=jset&odb=/equipment/mscb/settings/FGD/Feb_demand/power[0]&value=y HTTP/1.1\r\n\r\n";
  //char *req = "GET /CS/F000.html?cmd=jget&odb=/runinfo/run number HTTP/1.1\r\n\r\n";

  sprintf(fRequestBuf,"GET /%s?cmd=jget&odb=%s[%d] HTTP/1.1\r\n\r\n", xurl.GetFileAndOptions(), path, index);

  if (fDebug)
    printf("Sending [%s]\n", fRequestBuf);

  s.SendRaw(fRequestBuf, strlen(fRequestBuf));
  int rd = s.RecvRaw(fReplyBuf, sizeof(fReplyBuf));

  if (fDebug)
    printf("Received %d [%s]\n", rd, fReplyBuf);

  if (rd < 10)
    return NULL;

  if (strstr(fReplyBuf, "200 Document follows") == NULL)
    return NULL;

  fReplyBuf[rd] = 0;

  const char* p = strstr(fReplyBuf,"\r\n\r\n");
  if (!p)
    return NULL;

  p += 4;

  if (strcmp(p, "<unknown>") == 0)
    return NULL;

  if (strstr(p, "<html>") != NULL)
    {
      fprintf(stderr, "HttpOdb::jget: Bad mhttpd response: %s\n", p);
      return NULL;
    }

  if (fDebug)
    printf("----> mhttpd %s[%d] return [%s]\n", path, index, p);
  
  return p;
}

int      HttpOdb::odbReadAny(   const char*name, int index, int tid,void* buf, int bufsize)    { assert(!"Not implemented!"); }

uint32_t HttpOdb::odbReadUint32(const char*name, int index, uint32_t defaultValue)
{
  const char* reply = jget(name, index);
  if (!reply)
    return defaultValue;
  return strtoul(reply, NULL, 0);
}

double   HttpOdb::odbReadDouble(const char*name, int index, double defaultValue)
{
  const char* reply = jget(name, index);
  if (!reply)
    return defaultValue;
  return atof(reply);
}

int      HttpOdb::odbReadInt(   const char*name, int index, int      defaultValue)
{
  const char* reply = jget(name, index);
  if (!reply)
    return defaultValue;
  return atoi(reply);
}

bool     HttpOdb::odbReadBool(  const char*name, int index, bool     defaultValue)
{
  const char* reply = jget(name, index);
  if (!reply)
    return defaultValue;
  if (*reply == 'n')
    return false;
  return true;
}

const char* HttpOdb::odbReadString(const char* name, int index, const char* defaultValue)
{
  const char* reply = jget(name, index);
  if (!reply)
    return defaultValue;
  return reply;
}

int      HttpOdb::odbReadArraySize(const char*name)
{
  return 1;
#if 0
  TXMLNode *node = FindPath(NULL,name);
  if (!node)
    return 0;
  const char* num_values = GetAttrValue(node,"num_values");
  if (!num_values)
    return 1;
  return atoi(num_values);
#endif
}

//end

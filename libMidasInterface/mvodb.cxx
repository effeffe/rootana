/********************************************************************\

  Name:         mvodb.cxx
  Created by:   K.Olchanski

  Contents:     common functions of MVOdb ODB interface

\********************************************************************/

//#include <stdio.h>
//#include <string.h> // strlen()
//#include <assert.h>
//#include <stdlib.h> // malloc()

#include "mvodb.h"

static std::string toString(int value)
{
   char buf[256];
   sprintf(buf, "%d", value);
   return buf;
}

MVOdbError::MVOdbError()
{
   SetOk(this);
}

void SetOk(MVOdbError* error)
{
   if (error) {
      error->fError = false;
      error->fErrorString = "";
      error->fPath = "";
      error->fStatus = 0;
   }
}

void SetMidasStatus(MVOdbError* error, bool print, const std::string& path, const char* midas_func_name, int status)
{
   if (error) {
      error->fStatus = status;
      error->fPath = path;
      if (status == 1) {
         error->fError = false;
         error->fErrorString = "";
      } else {
         error->fError = true;
         error->fErrorString = "";
         error->fErrorString += "MIDAS ";
         error->fErrorString += midas_func_name;
         error->fErrorString += "()";
         error->fErrorString += " for ODB path \"";
         error->fErrorString += path;
         error->fErrorString += "\" returned status ";
         error->fErrorString += toString(status);
         if (print) {
            fprintf(stderr, "MVOdb::SetMidasStatus: %s\n", error->fErrorString.c_str());
         }
      }
   } else {
      if (print) {
         fprintf(stderr, "MVOdb::SetMidasStatus: Error: MIDAS %s() for ODB path \"%s\" returned status %d\n", midas_func_name, path.c_str(), status);
      }
   }
}

void SetError(MVOdbError* error, bool print, const std::string& path, const std::string& message)
{
   if (error) {
      error->fStatus = 0;
      error->fPath = path;
      error->fError = true;
      error->fErrorString = "";
      error->fErrorString += message;
      error->fErrorString += " for ODB path \"";
      error->fErrorString += path;
      error->fErrorString += "\"";
      if (print) {
         fprintf(stderr, "MVOdb::SetError: %s\n", error->fErrorString.c_str());
      }
   } else {
      if (print) {
         fprintf(stderr, "MVOdb::SetError: Error: %s for ODB path \"%s\"\n", message.c_str(), path.c_str());
      }
   }
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

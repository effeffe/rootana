/********************************************************************\

  Name:         mvodb.h
  Created by:   K.Olchanski

  Contents:     Virtual ODB interface

  Discussion:

\********************************************************************/

#ifndef INCLUDE_MVODB_H
#define INCLUDE_MVODB_H

#include <string>
#include <vector>
#include <stdint.h>

class MVOdbError;

class MVOdb
{
public:
   virtual MVOdb* Chdir(const char* subdirname, bool create, MVOdbError* error = NULL) = 0;
   
   // read array information: number of elements and element size (string size for TID_STRING arrays)

   //virtual void RAInfo(const char* varname, int *num_elements, int *element_size, MVOdbError* error = NULL) = 0;

   virtual void ReadKey(const char* varname, int *tid, int *num_values, int *total_size, int *item_size, MVOdbError* error = NULL) = 0;

   virtual void ReadDir(std::vector<std::string>* varname, std::vector<int> *tid, std::vector<int> *num_values, std::vector<int> *total_size, std::vector<int> *item_size, MVOdbError* error = NULL) = 0;

   // create and read individual variables or array elements

   virtual void RB(const char* varname, int index, bool   *value, bool create, MVOdbError* error = NULL) = 0; // TID_BOOL
   virtual void RI(const char* varname, int index, int    *value, bool create, MVOdbError* error = NULL) = 0; // TID_INT
   virtual void RD(const char* varname, int index, double *value, bool create, MVOdbError* error = NULL) = 0; // TID_DOUBLE
   virtual void RF(const char* varname, int index, float  *value, bool create, MVOdbError* error = NULL) = 0; // TID_FLOAT
   virtual void RS(const char* varname, int index, std::string *value, bool create, MVOdbError* error = NULL) = 0; // TID_STRING
   virtual void RU16(const char* varname, int index, uint16_t *value, bool create, MVOdbError* error = NULL) = 0; // TID_WORD
   virtual void RU32(const char* varname, int index, uint32_t *value, bool create, MVOdbError* error = NULL) = 0; // TID_DWORD

   // create and read whole arrays

   virtual void RBA(const char* varname, std::vector<bool>   *value, bool create, int create_size, MVOdbError* error = NULL) = 0;
   virtual void RIA(const char* varname, std::vector<int>    *value, bool create, int create_size, MVOdbError* error = NULL) = 0;
   virtual void RDA(const char* varname, std::vector<double> *value, bool create, int create_size, MVOdbError* error = NULL) = 0;
   virtual void RFA(const char* varname, std::vector<float>  *value, bool create, int create_size, MVOdbError* error = NULL) = 0;
   virtual void RSA(const char* varname, std::vector<std::string> *value, bool create, int create_size, int string_size, MVOdbError* error = NULL) = 0;
   virtual void RU16A(const char* varname, std::vector<uint16_t> *value, bool create, int create_size, MVOdbError* error = NULL) = 0;
   virtual void RU32A(const char* varname, std::vector<uint32_t> *value, bool create, int create_size, MVOdbError* error = NULL) = 0;

   // create and write individual variables

   virtual void WB(const char* varname, int index, bool   v, MVOdbError* error = NULL) = 0;
   virtual void WI(const char* varname, int index, int    v, MVOdbError* error = NULL) = 0;
   virtual void WD(const char* varname, int index, double v, MVOdbError* error = NULL) = 0;
   virtual void WF(const char* varname, int index, float  v, MVOdbError* error = NULL) = 0;
   virtual void WS(const char* varname, int index, const char* v, MVOdbError* error = NULL) = 0;
   virtual void WU16(const char* varname, int index, uint16_t v, MVOdbError* error = NULL) = 0;
   virtual void WU32(const char* varname, int index, uint32_t v, MVOdbError* error = NULL) = 0;

   // create and write whole arrays

   virtual void WBA(const char* varname, const std::vector<bool>&   v, MVOdbError* error = NULL) = 0;
   virtual void WIA(const char* varname, const std::vector<int>&    v, MVOdbError* error = NULL) = 0;
   virtual void WDA(const char* varname, const std::vector<double>& v, MVOdbError* error = NULL) = 0;
   virtual void WFA(const char* varname, const std::vector<float>&  v, MVOdbError* error = NULL) = 0;
   virtual void WSA(const char* varname, const std::vector<std::string>& v, int odb_string_length, MVOdbError* error = NULL) = 0;
   virtual void WU16A(const char* varname, const std::vector<uint16_t>& v, MVOdbError* error = NULL) = 0;
   virtual void WU32A(const char* varname, const std::vector<uint32_t>& v, MVOdbError* error = NULL) = 0;

   // delete functions
   virtual void Delete(const char* odbname, MVOdbError* error = NULL) = 0;

   // report errors to stderr or not
   virtual void SetPrintError(bool v) = 0;
   virtual bool GetPrintError() const = 0;
};

MVOdb* MakeNullOdb();
MVOdb* MakeMidasOdb(int hDB, MVOdbError* error = NULL);
//MVOdb* MakeXmlOdb(???);
//MVOdb* MakeJsonOdb(???);
//MVOdb* MakeJsonRpcOdb(???);

class MVOdbError
{
 public:
   bool fError; // true if there is an error, false if no error
   std::string fErrorString; // error text suitable for printing an error message
   std::string fPath; // odb path corresponding to the error
   int fStatus; // MIDAS ODB status numerical value

 public:
   MVOdbError();
};

void SetOk(MVOdbError* error);
void SetError(MVOdbError* error, bool print, const std::string& path, const std::string& message);
void SetMidasStatus(MVOdbError* error, bool print, const std::string& path, const char* midas_func_name, int status);

#endif

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
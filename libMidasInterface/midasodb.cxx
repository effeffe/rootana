/********************************************************************\

  Name:         midasodb.cxx
  Created by:   K.Olchanski

  Contents:     MIDAS implementation of MVOdb ODB interface

\********************************************************************/

#include <stdio.h>
#include <string.h> // strlen()
#include <assert.h>
#include <stdlib.h> // malloc()

#include "mvodb.h"
#include "midas.h"

static std::string toString(int value)
{
   char buf[256];
   sprintf(buf, "%d", value);
   return buf;
}

class MidasOdb: public MVOdb
{
public:
   HNDLE fDB;
   std::string fRoot;
   bool fTrace;
   bool fPrintError;

public:
   MidasOdb(HNDLE hDB, const char* root)
   {
      fDB = hDB;
      fRoot = root;
      fTrace = false;
      fPrintError = true;
   }

   std::string Path(const char* varname)
   {
      std::string path;
      path += fRoot;
      path += "/";
      path += varname;
      return path;
   }

   void SetPrintError(bool v)
   {
      fPrintError = v;
   }

   bool GetPrintError() const
   {
      return fPrintError;
   }

   MVOdb* Chdir(const char* subdir, bool create, MVOdbError* error)
   {
      std::string path = Path(subdir);
      MidasOdb* p = new MidasOdb(fDB, path.c_str());
      return p;
   }

   void RAInfo(const char* varname, int* num_elements, int* element_size, MVOdbError* error)
   {
      std::string path = Path(varname);

      if (num_elements)
         *num_elements = 0;
      if (element_size)
         *element_size = 0;

      int status;
      HNDLE hkey;
      status = db_find_key(fDB, 0, path.c_str(), &hkey);
      if (status != DB_SUCCESS)
         return;
      
      KEY key;
      status = db_get_key(fDB, hkey, &key);
      if (status != DB_SUCCESS)
         return;

      if (num_elements)
         *num_elements = key.num_values;

      if (element_size)
         *element_size = key.item_size;
   }

   void ReadKey(const char* varname, int *tid, int *num_values, int *total_size, int *item_size, MVOdbError* error)
   {
      if (tid) *tid = 0;
      if (num_values) *num_values = 0;
      if (total_size) *total_size = 0;
      if (item_size)  *item_size = 0;

      std::string path = Path(varname);

      int status;
      HNDLE hkey;

      status = db_find_key(fDB, 0, path.c_str(), &hkey);
      if (status != DB_SUCCESS) {
         SetMidasStatus(error, fPrintError, path, "db_find_key", status);
         return;
      }
      
      KEY key;
      status = db_get_key(fDB, hkey, &key);
      if (status != DB_SUCCESS) {
         SetMidasStatus(error, fPrintError, path, "db_get_key", status);
         return;
      }

      if (tid)
         *tid = key.type;
      
      if (num_values)
         *num_values = key.num_values;
      
      if (total_size)
         *total_size = key.total_size;

      if (item_size)
         *item_size = key.item_size;

      SetOk(error);
   }

   void ReadDir(std::vector<std::string>* varname, std::vector<int> *tid, std::vector<int> *num_values, std::vector<int> *total_size, std::vector<int> *item_size, MVOdbError* error)
   {
      // FIXME: incomplete!
      SetOk(error);
   }

   void Resize(const char* varname, int new_size, MVOdbError* error)
   {
      std::string path = Path(varname);

      int status;
      HNDLE hkey;
      
      status = db_find_key(fDB, 0, path.c_str(), &hkey);

      if (status != DB_SUCCESS) {
         SetMidasStatus(error, fPrintError, path, "db_find_key", status);
         return;
      }
      
      status = db_set_num_values(fDB, hkey, new_size);
      if (status != SUCCESS) {
         SetMidasStatus(error, fPrintError, path, "db_set_num_values", status);
         return;
      }

      SetOk(error);
   }

   void R(const char* varname, int index, int tid, void *value, int size, bool create, MVOdbError* error)
   {
      std::string path = Path(varname);
   
      path += "[";
      path += toString(index);
      path += "]";

      if (create && index > 0) {
         std::string msg = "";
         msg += "Cannot create arrays while reading \"";
         msg += path;
         msg += "\", \"create\" should be false or \"index\" should be zero";
         SetError(error, fPrintError, path, msg);
         return;
      }
   
      assert(value);

      int status = db_get_value(fDB, 0, path.c_str(), value, &size, tid, create);
      
      if (status != DB_SUCCESS) {
         SetMidasStatus(error, fPrintError, path, "db_get_value", status);
         return;
      }

      SetOk(error);
   }

   void RI(const char* varname, int index, int *value, bool create, MVOdbError* error)
   {
      R(varname, index, TID_INT, value, sizeof(int), create, error);
   }

   void RU16(const char* varname, int index, uint16_t *value, bool create, MVOdbError* error)
   {
      R(varname, index, TID_WORD, value, sizeof(uint16_t), create, error);
   }

   void RU32(const char* varname, int index, uint32_t *value, bool create, MVOdbError* error)
   {
      R(varname, index, TID_DWORD, value, sizeof(uint32_t), create, error);
   }

   void RD(const char* varname, int index, double *value, bool create, MVOdbError* error)
   {
      R(varname, index, TID_DOUBLE, value, sizeof(double), create, error);
   }

   void RF(const char* varname, int index, float *value, bool create, MVOdbError* error)
   {
      R(varname, index, TID_FLOAT, value, sizeof(float), create, error);
   }

   void RB(const char* varname, int index, bool *value, bool create, MVOdbError* error)
   {
      assert(value);
      BOOL v = *value;
      R(varname, index, TID_BOOL, &v, sizeof(BOOL), create, error);
      *value = v;
   }

   void RS(const char* varname, int index, std::string* value, bool create, MVOdbError* error)
   {
      std::string path = Path(varname);
   
      if (create && index > 0) {
         std::string msg = "";
         msg += "String arrays must be created using WSA() or RSA(). Error while reading \"";
         msg += path;
         msg += "\", \"create\" should be false or \"index\" should be zero";
         SetError(error, fPrintError, path, msg);
         return;
      }
   
      int status = db_get_value_string(fDB, 0, path.c_str(), index, value, create);

      if (status != DB_SUCCESS) {
         SetMidasStatus(error, fPrintError, path, "db_get_value_string", status);
         return;
      }

      SetOk(error);
   }

   void RA(const std::string& path, int tid, void* buf, int size, MVOdbError* error)
   {
      int status = db_get_value(fDB, 0, path.c_str(), buf, &size, tid, FALSE);

      if (status != DB_SUCCESS) {
         SetMidasStatus(error, fPrintError, path, "db_get_value", status);
         return;
      }

      SetOk(error);
   }

   int CheckSize(const std::string& path, const char* varname, int tid, int tsize, MVOdbError* error)
   {
      int xtid = 0;
      int xnum_values = 0;
      int xtotal_size = 0;
      int xitem_size = 0;

      ReadKey(varname, &xtid, &xnum_values, &xtotal_size, &xitem_size, error);

      if (error && error->fError)
         return -1;

      if (xtid == 0) {
         return -1;
      }

      if (xitem_size != tsize) {
         return -1;
      }

      if (xtid != tid) {
         return -1;
      }

      return xnum_values;
   }

   int CWA(const std::string& path, const char* varname, int tid, int tsize, int size, const void* data, bool create, int create_size, MVOdbError* error)
   {
      int num = CheckSize(path, varname, TID_INT, sizeof(int), error);

      if (num >= 0) {
         return num;
      }

      if (!create) {
         return 0;
      }

      WA(varname, tid, data, size*tsize, size, error);
      
      if (create_size > size) {
         Resize(varname, create_size, error);
      }

      return CheckSize(path, varname, TID_INT, sizeof(int), error);
   }

   template <class X> void RXA(const char* varname, int tid, std::vector<X> *value, bool create, int create_size, MVOdbError* error)
   {
      std::string path = Path(varname);
      int size = 0;
      const void* vptr = NULL;
      if (value) {
         size = value->size();
         vptr = &((*value)[0]);
      }

      int num = CWA(path, varname, tid, sizeof(X), size, vptr, create, create_size, error);

      if (value) {
         value->clear();
         if (num > 0) {
            value->resize(num);
            RA(path, tid, &((*value)[0]), num*sizeof(X), error);
         }
      }
   }

   void RIA(const char* varname, std::vector<int> *value, bool create, int create_size, MVOdbError* error)
   {
      RXA<int>(varname, TID_INT, value, create, create_size, error);
   }

   void RFA(const char* varname, std::vector<float> *value, bool create, int create_size, MVOdbError* error)
   {
      RXA<float>(varname, TID_FLOAT, value, create, create_size, error);
   }

   void RDA(const char* varname, std::vector<double> *value, bool create, int create_size, MVOdbError* error)
   {
      RXA<double>(varname, TID_DOUBLE, value, create, create_size, error);
   }

   void RU16A(const char* varname, std::vector<uint16_t> *value, bool create, int create_size, MVOdbError* error)
   {
      RXA<uint16_t>(varname, TID_WORD, value, create, create_size, error);
   }
   
   void RU32A(const char* varname, std::vector<uint32_t> *value, bool create, int create_size, MVOdbError* error)
   {
      RXA<uint32_t>(varname, TID_DWORD, value, create, create_size, error);
   }
   
   void RBA(const char* varname, std::vector<bool> *value, bool create, int create_size, MVOdbError* error)
   {
#if 0
      std::string path;
      path += fRoot;
      path += "/";
      path += varname;
   
      if (value) {
         value->clear();
      }

      if (fTrace) {
         printf("Read ODB %s\n", path.c_str());
      }

      int num = 0;
      int esz = 0;

      RAInfo(varname, &num, &esz, error);

      if (error && error->fError)
         return;

      if (num <= 0 || esz <= 0) {
         if (create) {
            num = create_size;
            esz = sizeof(BOOL);
         } else {
            return;
         }
      }

      assert(esz == sizeof(BOOL));

      int size = sizeof(BOOL)*num;

      assert(size > 0);

      BOOL* buf = (BOOL*)malloc(size);
      assert(buf);
      memset(buf, 0, size);
      int status = db_get_value(fDB, 0, path.c_str(), buf, &size, TID_BOOL, create);

      if (status != DB_SUCCESS) {
         printf("RBA: db_get_value status %d\n", status);
      }

      if (value) {
         for (int i=0; i<num; i++) {
            value->push_back(buf[i]);
         }
      }

      free(buf);
#endif
   }


   void RSA(const char* varname, std::vector<std::string> *value, bool create, int create_size, int string_size, MVOdbError* error)
   {
#if 0
      std::string path;
      path += fRoot;
      path += "/";
      path += varname;

      if (value) {
         value->clear();
      }

      if (fTrace) {
         printf("Read ODB %s\n", path.c_str());
      }

      int num = 0;
      int esz = 0;

      RAInfo(varname, &num, &esz, error);

      if (error && error->fError)
         return;

      bool create_first = false;

      if (num <= 0 || esz <= 0) {
         if (create) {
            num = create_size;
            esz = string_size;
            create_first = true;
         } else {
            return;
         }
      }

      int size = esz*num;

      assert(size > 0);

      char* buf = (char*)malloc(size);

      int status;
      if (create_first) {
         memset(buf, 0, size);
         status = db_set_value(fDB, 0, path.c_str(), buf, size, num, TID_STRING);
      }

      status = db_get_value(fDB, 0, path.c_str(), buf, &size, TID_STRING, create);

      if (status != DB_SUCCESS) {
         printf("RSA: db_get_value status %d\n", status);
      }

      for (int i=0; i<num; i++) {
         value->push_back(buf+esz*i);
      }

      free(buf);
#endif
   }

   void W(const char* varname, int index, int tid, const void* v, int size, MVOdbError* error)
   {
      std::string path = Path(varname);
   
      int status = db_set_value(fDB, 0, path.c_str(), v, size, 1, tid);

      if (status != DB_SUCCESS) {
         SetMidasStatus(error, fPrintError, path, "db_set_value", status);
         return;
      }

      SetOk(error);
   }

   void WB(const char* varname, int index, bool v, MVOdbError* error)
   {
      BOOL vv = v;
      W(varname, index, TID_BOOL, &vv, sizeof(BOOL), error);
   }

   void WI(const char* varname, int index, int v, MVOdbError* error)
   {
      W(varname, index, TID_INT, &v, sizeof(int), error);
   }

   void WU16(const char* varname, int index, uint16_t v, MVOdbError* error)
   {
      W(varname, index, TID_WORD, &v, sizeof(uint16_t), error);
   }
   
   void WU32(const char* varname, int index, uint32_t v, MVOdbError* error)
   {
      W(varname, index, TID_DWORD, &v, sizeof(uint32_t), error);
   }
   
   void WD(const char* varname, int index, double v, MVOdbError* error)
   {
      W(varname, index, TID_DOUBLE, &v, sizeof(double), error);
   }

   void WF(const char* varname, int index, float v, MVOdbError* error)
   {
      W(varname, index, TID_FLOAT, &v, sizeof(float), error);
   }

   void WS(const char* varname, int index, const char* v, MVOdbError* error)
   {
      int len = strlen(v);
      W(varname, index, TID_STRING, v, len+1, error);
   }

   void WA(const char* varname, int tid, const void* v, int size, int count, MVOdbError* error)
   {
      std::string path = Path(varname);

      int status;

      if (size == 0) {
         status = db_create_key(fDB, 0, path.c_str(), tid);

         if (status != DB_SUCCESS) {
            SetMidasStatus(error, fPrintError, path, "db_set_value", status);
            return;
         }
      } else {
         status = db_set_value(fDB, 0, path.c_str(), v, size, count, tid);

         //printf("WA db_set_value(tid %d, size %d, count %d) status %d\n", tid, size, count, status);
         
         if (status != DB_SUCCESS) {
            SetMidasStatus(error, fPrintError, path, "db_set_value", status);
            return;
         }
      }

      SetOk(error);
   }

   void WBA(const char* varname, const std::vector<bool>& v, MVOdbError* error)
   {
      unsigned num = v.size();
      BOOL val[num];
      
      for (unsigned i=0; i<num; i++) {
         val[i] = v[i];
      }
      
      WA(varname, TID_BOOL, val, num*sizeof(BOOL), num, error);
   }
   
   void WU16A(const char* varname, const std::vector<uint16_t>& v, MVOdbError* error)
   {
      WA(varname, TID_WORD, &v[0], v.size()*sizeof(uint16_t), v.size(), error);
   }
   
   void WU32A(const char* varname, const std::vector<uint32_t>& v, MVOdbError* error)
   {
      WA(varname, TID_DWORD, &v[0], v.size()*sizeof(uint32_t), v.size(), error);
   }
   
   void WIA(const char* varname, const std::vector<int>& v, MVOdbError* error)
   {
      WA(varname, TID_INT, &v[0], v.size()*sizeof(int), v.size(), error);
   }

   void WFA(const char* varname, const std::vector<float>& v, MVOdbError* error)
   {
      WA(varname, TID_FLOAT, &v[0], v.size()*sizeof(float), v.size(), error);
   }

   void WDA(const char* varname, const std::vector<double>& v, MVOdbError* error)
   {
      WA(varname, TID_DOUBLE, &v[0], v.size()*sizeof(double), v.size(), error);
   }

   void WSA(const char* varname, const std::vector<std::string>& v, int odb_string_size, MVOdbError* error)
   {
      unsigned num = v.size();
      unsigned length = odb_string_size;

      char val[length*num];
      memset(val, 0, length*num);
      
      for (unsigned i=0; i<num; i++)
         strlcpy(val+length*i, v[i].c_str(), length);
      
      WA(varname, TID_STRING, val, num*length, num, error);
   }

   void Delete(const char* odbname, MVOdbError* error)
   {
      // FIXME: incomplete
      SetOk(error);
   };
};

MVOdb* MakeMidasOdb(int hDB, MVOdbError* error)
{
   return new MidasOdb(hDB, "");
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

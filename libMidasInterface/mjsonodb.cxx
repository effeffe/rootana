//
// ALPHA ROOT analyzer
//
// Access to ODB stored in JSON odb save file or ODB JSON dump in MIDAS data file.
//
// Name: mjsonodb.cxx
// Author: K.Olchanski, 28-May-2019
//

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h> // memset()
#include <errno.h> // errno

#include "mvodb.h"
#include "mjson.h"

#include "rootana_stdint.h"

static std::string toString(int i)
{
   char buf[256];
   sprintf(buf, "%d", i);
   return buf;
}

#if 0
/// Print out the contents of the ODB tree
static void DumpTree(PMXML_NODE node, int level = 0)
{
   assert(node);

   for (int k=0; k<level; k++)
      printf(" ");
   printf("node name: \"%s\"\n", node->name);
   for (int i=0; i<node->n_attributes; i++) {
      for (int k=0; k<level; k++)
         printf(" ");
      printf("attribute name: \"%s\", value: \"%s\"\n", GetAttrName(node, i), GetAttrValue(node, i));
   }
   if (node->value) {
      for (int k=0; k<level; k++)
         printf(" ");
      printf("node text: \"%s\"\n", node->value);
   }
   for (int i=0; i<node->n_children; i++)
      DumpTree(node->child + i, level + 1);
}

/// Print out the directory structure of the ODB tree
static void DumpDirTree(PMXML_NODE node, int level = 0)
{
   assert(node);

   const char* name = node->name;
   
   if (strcmp(name,"dir") != 0)
      return;
   
   for (int k=0; k<level; k++)
      printf(" ");
   printf("node name: \"%s\"\n", node->name);
   
   for (int i=0; i<node->n_attributes; i++) {
      for (int k=0; k<level; k++)
         printf(" ");
      printf("attribute name: \"%s\", value: \"%s\"\n", GetAttrName(node, i), GetAttrValue(node, i));
   }
   
   for (int i=0; i<node->n_children; i++) {
      DumpDirTree(node->child + i, level + 1);
   }
}
#endif

#if 0
template <typename T>
static T GetXmlValue(const char* text);

template<>
int GetXmlValue<int>(const char* text)
{
   return atoi(text);
}

template<>
double GetXmlValue<double>(const char* text)
{
   return atof(text);
}

template<>
float GetXmlValue<float>(const char* text)
{
   return atof(text);
}

template<>
bool GetXmlValue<bool>(const char* text)
{
   if (*text == 'n')
      return false;
   else
      return true;
}

template<>
uint16_t GetXmlValue<uint16_t>(const char* text)
{
   return 0xFFFF & strtoul(text, NULL, 0);
}

template<>
uint32_t GetXmlValue<uint32_t>(const char* text)
{
   return strtoul(text, NULL, 0);
}

template<>
std::string GetXmlValue<std::string>(const char* text)
{
   return text;
}
#endif

/// Access to ODB saved in JSON format inside midas .mid files

class JsonOdb : public MVOdb
{
public:
   MJsonNode* fRoot; // root of JSON document, NULL if we are a subdirectory
   MJsonNode* fDir;  // current ODB directory
   std::string fPath; // path to correct ODB directory
   bool fPrintError;

public:
   JsonOdb(MJsonNode* root, MJsonNode* dir, MVOdbError* error) // ctor
   {
      fPrintError = false;
      fRoot = root;
      fDir  = dir;
      fPath = "";

      //DumpTree(fRoot);
      //DumpTree(fDir);
      
      SetOk(error);
   }

   ~JsonOdb() // dtor
   {
      if (fRoot) {
         delete fRoot;
         fRoot = NULL;
      }
      fDir = NULL;
   }

public:
   void SetPrintError(bool v)
   {
      fPrintError = true;
   }

   bool GetPrintError() const
   {
      return fPrintError;
   }

   void SetNotFound(MVOdbError* error, const char* varname)
   {
      if (!error)
         return;
      std::string path;
      path += fPath;
      path += "/";
      path += varname;
      std::string msg;
      msg += "Cannot find ";
      msg += "\"";
      msg += path;
      msg += "\"";
      SetError(error, fPrintError, path, msg);
   }

   void SetNullValue(MVOdbError* error, const char* varname)
   {
      if (!error)
         return;
      std::string path;
      path += fPath;
      path += "/";
      path += varname;
      std::string msg;
      msg += "XML node for ";
      msg += "\"";
      msg += path;
      msg += "\"";
      msg += " is NULL";
      SetError(error, fPrintError, path, msg);
   }

   /// Follow the ODB path through the JSON tree
   static MJsonNode* FindPath(MJsonNode* dir, const char* path)
   {
      assert(dir);
      
      while (1) {
         // skip leading slashes
         while (*path == '/')
            path++;
         
         if (*path == 0)
            return dir;
         
         std::string elem;
         
         // copy the next path element into "elem"-
         // copy "path" until we hit "/" or end of string
         while (1) {
            if (*path==0 || *path=='/')
               break;
            elem += *path++;
         }
      
         //printf("looking for \"%s\" more \"%s\"\n", elem.c_str(), path);

         MJsonNode* found = NULL;

         const MJsonStringVector* s = dir->GetObjectNames();
         const MJsonNodeVector*   n = dir->GetObjectNodes();
         assert(s->size() == n->size());
         for (unsigned i=0; i<s->size(); i++) {
            //const MJsonNode* node = (*a)[i];
            
            //const char* nodename = node->name;
            //const char* namevalue = GetAttr(node, "name");
            
            //printf("node name: \"%s\", \"name\" value: \"%s\"\n", node->name, namevalue);
            
            //bool isDir = strcmp(nodename, "dir") == 0;
            //bool isKey = strcmp(nodename, "key") == 0;
            //bool isKeyArray = strcmp(nodename, "keyarray") == 0;
            
            //if (!isKey && !isDir && !isKeyArray)
            //   continue;
	  
            //
            // compare directory names
            //
	  
            if (strcasecmp(elem.c_str(), (*s)[i].c_str()) == 0) {
               if (dir->GetType() == MJSON_OBJECT) {
                  // found the right subdirectory, descend into it
                  found = (*n)[i];
                  break;
               } else {
                  return (*n)[i];
               }
            }
         }

         if (!found)
            return NULL;
         dir = found;
      }
   }
   
   MVOdb* Chdir(const char* subdir, bool create, MVOdbError* error)
   {
      MJsonNode* node = FindPath(fDir, subdir);
      if (!node) {
         //printf("Not Found subdir [%s], create %d\n", subdir, create);
         if (create) {
            SetOk(error);
            return MakeNullOdb();
         }
         SetNotFound(error, subdir);
         return NULL;
      }

      if (node->GetType() != MJSON_OBJECT) {
         std::string msg;
         msg += "\"";
         msg += subdir;
         msg += "\"";
         msg += " JSON node is ";
         msg += "\"";
         msg += MJsonNode::TypeToString(node->GetType());
         msg += "\"";
         msg += " instead of \"dir\"";
         SetError(error, fPrintError, fPath, msg);
         return NULL;
      }

      //printf("Found subdir [%s]\n", subdir);
      //DumpTree(node);

      JsonOdb* x = new JsonOdb(NULL, node, error);
      x->fPath = fPath + "/" + subdir;
      
      SetOk(error);
      return x;
   }

   void ReadKey(const char* varname, int *tid, int *num_values, int *total_size, int *item_size, MVOdbError* error)
   {
      if (tid) *tid = 0;
      if (num_values) *num_values = 0;
      if (total_size) *total_size = 0;
      if (item_size)  *item_size = 0;
      // FIXME: not implemented
      SetOk(error);
   }

   void ReadDir(std::vector<std::string>* varname, std::vector<int> *tid, std::vector<int> *num_values, std::vector<int> *total_size, std::vector<int> *item_size, MVOdbError* error)
   {
      // FIXME: not implemented
      SetOk(error);
   }

   void RB(const char* varname, bool   *value, bool create, MVOdbError* error)
   {
      RBAI(varname, 0, value, error);
   };

   void RI(const char* varname, int    *value, bool create, MVOdbError* error)
   {
      RIAI(varname, 0, value, error);
   };

   void RD(const char* varname, double *value, bool create, MVOdbError* error)
   {
      RDAI(varname, 0, value, error);
   };
      
   void RF(const char* varname, float  *value, bool create, MVOdbError* error)
   {
      RFAI(varname, 0, value, error);
   };
      
   void RS(const char* varname, std::string *value, bool create, int create_string_length, MVOdbError* error)
   {
      RSAI(varname, 0, value, error);
   };

   void RU16(const char* varname, uint16_t *value, bool create, MVOdbError* error)
   {
      RU16AI(varname, 0, value, error);
   };

   void RU32(const char* varname, uint32_t *value, bool create, MVOdbError* error)
   {
      RU32AI(varname, 0, value, error);
   };

#if 0
   MJsonNode* FindJsonNode(MJsonNode* dir, const char* varname, const char* type, MVOdbError* error)
   {
      MJsonNode* node = FindPath(dir, varname);
      if (!node) {
         SetNotFound(error, varname);
         return NULL;
      }

      if (strcmp(GetAttr(node, "type"), type) != 0) {
         fprintf(stderr, "type mismatch!\n");
         SetNullValue(error, varname);
         return NULL;
      }

      return node;
   }
#endif

   template <typename T>
   void RXA(const char* varname, const char* type, std::vector<T> *value, MVOdbError* error)
   {
      if (!value) {
         SetOk(error);
         return;
      }
      
      MJsonNode* node = FindPath(fDir, varname);
      if (!node) {
         SetNotFound(error, varname);
         return;
      }

      //DumpTree(node);

      if (node->GetType() == MJSON_OBJECT) {
         fprintf(stderr, "wrong type: MJSON_OBJECT in RXA!\n");
         return;
      } else if (node->GetType() == MJSON_ARRAY) {
#if 0
         const char* num_values_text = GetAttr(node, "num_values");
         if (!num_values_text) {
            fprintf(stderr, "no num_values!\n");
            SetNullValue(error, varname);
            return;
         }
         
         int num_values = atoi(num_values_text);
         
         if (num_values != node->n_children) {
            fprintf(stderr, "num_values mismatch %d vs %d children!\n", num_values, node->n_children);
            SetNullValue(error, varname);
            return;
         }
         
         for (int i=0; i<node->n_children; i++) {
            PMXML_NODE elem = node->child+i;
            const char* text = elem->value;
            if (!text) {
               SetNullValue(error, varname);
               return;
            }
            T v = GetXmlValue<T>(text);
            value->push_back(v);
         }

         SetOk(error);
         return;
#endif
      } else {
#if 0
         const char* text = node->value;
         if (!text) {
            SetNullValue(error, varname);
            return;
         }
         T v = GetXmlValue<T>(text);
         value->push_back(v);
         SetOk(error);
         return;
      } else {
         fprintf(stderr, "unexpected node %s\n", node->name);
         SetNullValue(error, varname);
#endif
         return;
      }
   };

   void RBA(const char* varname, std::vector<bool> *value, bool create, int create_size, MVOdbError* error)
   {
      RXA(varname, "BOOL", value, error);
   }

   void RIA(const char* varname, std::vector<int> *value,  bool create, int create_size, MVOdbError* error)
   {
      RXA(varname, "INT", value, error);
   }

   void RDA(const char* varname, std::vector<double> *value,  bool create, int create_size, MVOdbError* error)
   {
      RXA(varname, "DOUBLE", value, error);
   }
   
   void RFA(const char* varname, std::vector<float>  *value,  bool create, int create_size, MVOdbError* error)
   {
      RXA(varname, "FLOAT", value, error);
   }
   
   void RSA(const char* varname, std::vector<std::string> *value, bool create, int create_size, int create_string_length, MVOdbError* error)
   {
      RXA(varname, "STRING", value, error);
   }

   void RU16A(const char* varname, std::vector<uint16_t> *value,  bool create, int create_size, MVOdbError* error)
   {
      RXA(varname, "WORD", value, error);
   }

   void RU32A(const char* varname, std::vector<uint32_t> *value,  bool create, int create_size, MVOdbError* error)
   {
      RXA(varname, "DWORD", value, error);
   }

   template <typename T>
   void RXAI(const char* varname, int index, const char* type, T* value, MVOdbError* error)
   {
      if (!value) {
         SetOk(error);
         return;
      }

      MJsonNode* node = FindPath(fDir, varname);
      if (!node) {
         SetNotFound(error, varname);
         return;
      }

#if 0
      //DumpTree(node);

      if (strcmp(node->name, "keyarray") == 0) {
         const char* num_values_text = GetAttr(node, "num_values");
         if (!num_values_text) {
            fprintf(stderr, "no num_values!\n");
            SetNullValue(error, varname);
            return;
         }
         
         int num_values = atoi(num_values_text);
         
         if (num_values != node->n_children) {
            fprintf(stderr, "num_values mismatch %d vs %d children!\n", num_values, node->n_children);
            SetNullValue(error, varname);
            return;
         }

         if (index < 0) {
            fprintf(stderr, "bad index %d, num_values %d!\n", index, num_values);
            SetNullValue(error, varname);
            return;
         }
         
         if (index >= num_values) {
            fprintf(stderr, "bad index %d, num_values %d!\n", index, num_values);
            SetNullValue(error, varname);
            return;
         }
         
         PMXML_NODE elem = node->child+index;
         const char* text = elem->value;
         if (!text) {
            SetNullValue(error, varname);
            return;
         }

         *value = GetXmlValue<T>(text);

         SetOk(error);
         return;
      } else if (strcmp(node->name, "key") == 0) {

         if (index != 0) {
            fprintf(stderr, "non-zero index %d for non-array!\n", index);
            SetNullValue(error, varname);
            return;
         }

         const char* text = node->value;
         if (!text) {
            SetNullValue(error, varname);
            return;
         }

         *value = GetXmlValue<T>(text);

         SetOk(error);
         return;
      } else {
         fprintf(stderr, "unexpected node %s\n", node->name);
         SetNullValue(error, varname);
         return;
      }
#endif
   }

   void RBAI(const char* varname, int index, bool   *value, MVOdbError* error)
   {
      RXAI(varname, index, "BOOL", value, error);
   }

   void RIAI(const char* varname, int index, int    *value, MVOdbError* error)
   {
      RXAI(varname, index, "INT", value, error);
   }
   
   void RDAI(const char* varname, int index, double *value, MVOdbError* error)
   {
      RXAI(varname, index, "DOUBLE", value, error);
   }
   
   void RFAI(const char* varname, int index, float  *value, MVOdbError* error)
   {
      RXAI(varname, index, "FLOAT", value, error);
   }
   
   void RSAI(const char* varname, int index, std::string *value, MVOdbError* error)
   {
      RXAI(varname, index, "STRING", value, error);
   }
   
   void RU16AI(const char* varname, int index, uint16_t *value, MVOdbError* error)
   {
      RXAI(varname, index, "WORD", value, error);
   }

   void RU32AI(const char* varname, int index, uint32_t *value, MVOdbError* error)
   {
      RXAI(varname, index, "DWORD", value, error);
   }

   // write functions do nothing

   void WB(const char* varname, bool v,   MVOdbError* error) { SetOk(error); };
   void WI(const char* varname, int v,    MVOdbError* error)  { SetOk(error); };
   void WD(const char* varname, double v, MVOdbError* error) { SetOk(error); };
   void WF(const char* varname, float  v, MVOdbError* error) { SetOk(error); };
   void WS(const char* varname, const char* v, int string_length, MVOdbError* error) { SetOk(error); };
   void WU16(const char* varname, uint16_t v, MVOdbError* error) { SetOk(error); };
   void WU32(const char* varname, uint32_t v, MVOdbError* error) { SetOk(error); };

   void WBA(const char* varname, const std::vector<bool>& v, MVOdbError* error) { SetOk(error); };
   void WIA(const char* varname, const std::vector<int>& v, MVOdbError* error) { SetOk(error); };
   void WDA(const char* varname, const std::vector<double>& v, MVOdbError* error) { SetOk(error); };
   void WFA(const char* varname, const std::vector<float>& v, MVOdbError* error) { SetOk(error); };
   void WSA(const char* varname, const std::vector<std::string>& data, int odb_string_length, MVOdbError* error) { SetOk(error); };
   void WU16A(const char* varname, const std::vector<uint16_t>& v, MVOdbError* error) { SetOk(error); };
   void WU32A(const char* varname, const std::vector<uint32_t>& v, MVOdbError* error) { SetOk(error); };

   void WBAI(const char* varname, int index, bool v,   MVOdbError* error) { SetOk(error); };
   void WIAI(const char* varname, int index, int v,    MVOdbError* error)  { SetOk(error); };
   void WDAI(const char* varname, int index, double v, MVOdbError* error) { SetOk(error); };
   void WFAI(const char* varname, int index, float  v, MVOdbError* error) { SetOk(error); };
   void WSAI(const char* varname, int index, const char* v, MVOdbError* error) { SetOk(error); };
   void WU16AI(const char* varname, int index, uint16_t v, MVOdbError* error) { SetOk(error); };
   void WU32AI(const char* varname, int index, uint32_t v, MVOdbError* error) { SetOk(error); };

   // delete function does nothing

   void Delete(const char* odbname, MVOdbError* error) { SetOk(error); };
};

MVOdb* MakeJsonFileOdb(const char* filename, MVOdbError* error)
{
   std::string data;

   {
      FILE *fp = fopen(filename, "r");
      if (!fp) {
         std::string msg;
         msg += "Cannot open file ";
         msg += "\"";
         msg += filename;
         msg += "\"";
         msg += " fopen() errno: ";
         msg += toString(errno);
         msg += " (";
         msg += strerror(errno);
         msg += ")";
         SetError(error, true, filename, msg);
         return MakeNullOdb();
      }
      
      while (1) {
         char buf[1024*1024];
         const char* s = fgets(buf, sizeof(buf), fp);
         if (!s)
            break;
         data += s;
      }
      
      fclose(fp);
      fp = NULL;
   }

   MJsonNode* root = MJsonNode::Parse(data.c_str());
   //root->Dump();
   return new JsonOdb(root, root, error);
}

MVOdb* MakeJsonBufferOdb(const char* buf, int bufsize, MVOdbError* error)
{
   return MakeNullOdb();
#if 0
   SetOk(error);
   return new XmlOdb(NULL, NULL, error);
#endif
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

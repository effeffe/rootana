//
// ALPHA ROOT analyzer
//
// Name: mxmlodb.cxx
// Author: K.Olchanski, 11-July-2006
// Author: K.Olchanski, 16-May-2019
//

#include <stdio.h>
#include <iostream>
#include <assert.h>

#include "mvodb.h"
#include "mxml.h"

#include <TList.h>

#include <stdlib.h>
#include "TXMLNode.h"
#include "TXMLAttr.h"
#include "TDOMParser.h"
#include "rootana_stdint.h"

/// Access to ODB saved in XML format inside midas .mid files

class XmlOdb : public MVOdb
{
public:
   TXMLNode* fOdb;    ///< Pointer to the root of the ODB tree
   bool fPrintError;

public:
   XmlOdb() // ctor
   {
      fPrintError = false;
   }

   ~XmlOdb() // dtor
   {

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

   TXMLNode* FindNode(TXMLNode* node, const char* name)
   {
      for (; node != NULL; node = node->GetNextNode()) {
         //printf("node name: \"%s\"\n",node->GetNodeName());
         if (strcmp(node->GetNodeName(),name) == 0)
            return node;
         
         if (node->HasChildren()) {
            TXMLNode* found = FindNode(node->GetChildren(),name);
            if (found)
               return found;
         }
      }
      
      return NULL;
   }
   
   /// Print out the contents of the ODB tree
   void DumpTree(TXMLNode* node = NULL, int level = 0)
   {
      if (!node)
         node = fOdb;
      
      if (!node) {
         fprintf(stderr,"XmlOdb::DumpTree: node is NULL!\n");
         return;
      }

      while (node) {
         for (int i=0; i<level; i++)
            printf(" ");
         printf("node name: \"%s\"\n",node->GetNodeName());
         TList* attrs = node->GetAttributes();
         TIter next(attrs);                           
         while (TXMLAttr *attr = (TXMLAttr*)next()) {
            for (int i=0; i<level; i++)
               printf(" ");
            printf("attribute name: \"%s\", value: \"%s\"\n",attr->GetName(),attr->GetValue());
         }
         const char*text = node->GetText();
         if (text) {
            for (int i=0; i<level; i++)
               printf(" ");
            printf("node text: \"%s\"\n",node->GetText());
         }
         if (node->HasChildren())
            DumpTree(node->GetChildren(),level + 1);
         node = node->GetNextNode();
      }
      //printf("no more next nodes...\n");
   }

   /// Print out the directory structure of the ODB tree
   void DumpDirTree(TXMLNode* node = NULL, int level = 0)
   {
      if (!node)
         node = fOdb;
      
      if (!node) {
         fprintf(stderr,"XmlOdb::DumpDirTree: node is NULL!\n");
         return;
      }

      for (; node != NULL; node = node->GetNextNode()) {
         const char* name = node->GetNodeName();
         
         if (strcmp(name,"dir") != 0)
            continue;
         
         for (int i=0; i<level; i++)
            printf(" ");
         printf("node name: \"%s\"\n",node->GetNodeName());
         TList* attrs = node->GetAttributes();
         TIter next(attrs);                           
         while (TXMLAttr *attr = (TXMLAttr*)next()) {
            for (int i=0; i<level; i++)
               printf(" ");
            printf("attribute name: \"%s\", value: \"%s\"\n",attr->GetName(),attr->GetValue());
         }
         if (node->HasChildren())
            DumpDirTree(node->GetChildren(),level + 1);
      }
      //printf("no more next nodes...\n");
   }

   /// Return the value of the named attribute
   const char* GetAttrValue(TXMLNode* node, const char* attrName)
   {
      TList* attrs = node->GetAttributes();
      TIter next(attrs);                           
      while (TXMLAttr *attr = (TXMLAttr*)next()) {
         //printf("attribute name: \"%s\", value: \"%s\"\n",attr->GetName(),attr->GetValue());
         
         if (strcmp(attr->GetName(),attrName)==0)
            return attr->GetValue();
      }
      return NULL;
   }
   
   /// Follow the ODB path through the XML DOM tree
   TXMLNode* FindPath(TXMLNode* node, const char* path)
   {
      if (!fOdb)
         return NULL;
      
      if (!node)
         node = fOdb->GetChildren();
      
      while (1) {
         // skip leading slashes
         while (*path == '/')
            path++;
         
         if (*path == 0)
            return node;
         
         const int kElemSize = 256;
         char elem[kElemSize+1];
         memset(elem,0,kElemSize+1);
         
         // copy the next path element into "elem"-
         // copy "path" until we hit "/" or end of string
         for (int i=0; i<kElemSize; i++) {
            if (*path==0 || *path=='/')
               break;
            elem[i] = *path++;
         }
      
         //printf("looking for \"%s\" more \"%s\"\n",elem,path);
      
         for (; node != NULL; node = node->GetNextNode()) {
            const char* nodename = node->GetNodeName();
            const char* namevalue = GetAttrValue(node,"name");
            
            //printf("node name: \"%s\", \"name\" value: \"%s\"\n",node->GetNodeName(),namevalue);
            
            bool isDir = strcmp(nodename,"dir") == 0;
            bool isKey = strcmp(nodename,"key") == 0;
            bool isKeyArray = strcmp(nodename,"keyarray") == 0;
            
            if (!isKey && !isDir && !isKeyArray)
               continue;
	  
            //
            // compare directory names
            //
	  
            if (strcasecmp(elem,namevalue) == 0) {
               if (isDir) {
                  // found the right subdirectory, descend into it
                  node = node->GetChildren();
                  break;
               } else if (isKey || isKeyArray) {
                  return node;
               }
            }
         }
      }
   }
   
   /// Same as FindPath(), but also index into an array
   TXMLNode* FindArrayPath(TXMLNode*node,const char* path,const char* type,int index)
   {
      if (!fOdb)
         return NULL;

      if (!node)
         node = fOdb->GetChildren();

      node = FindPath(node, path);
      
      if (!node)
         return NULL;
      
      const char* nodename = node->GetNodeName();
      const char* num_values = GetAttrValue(node,"num_values");
      
      const char* typevalue = GetAttrValue(node,"type");
      
      if (!typevalue || (strcasecmp(typevalue,type) != 0)) {
         fprintf(stderr,"XmlOdb::FindArrayPath: Type mismatch: \'%s\' has type \'%s\', we expected \'%s\'\n", path, typevalue, type);
         return NULL;
      }

      bool isKeyArray = (num_values!=NULL) && (strcmp(nodename,"keyarray")==0);

      if (!isKeyArray) {
         if (index != 0) {
            fprintf(stderr,"XmlOdb::FindArrayPath: Attempt to access array element %d, but \'%s\' is not an array\n", index, path);
            return NULL;
         }

         return node;
      }

      int max_index = atoi(num_values);
      
      if (index < 0 || index >= max_index) {
         fprintf(stderr,"XmlOdb::FindArrayPath: Attempt to access array element %d, but size of array \'%s\' is %d\n", index, path, max_index);
         return NULL;
      }

      //printf("nodename [%s]\n", nodename);
      
      TXMLNode* elem = node->GetChildren();
      
      for (int i=0; elem!=NULL; ) {
         const char* name = elem->GetNodeName();
         //const char* text = elem->GetText();
         //printf("index %d, name [%s] text [%s]\n", i, name, text);
         
         if (strcmp(name,"value") == 0) {
            if (i == index)
               return elem;
            i++;
         }
         
         elem = elem->GetNextNode();
      }

      return node;
   }
   
   int      odbReadAny(   const char*name, int index, int tid,void* buf, int bufsize=0);
   int      odbReadInt(   const char*name, int index, int      defaultValue);
   uint32_t odbReadUint32(const char*name, int index, uint32_t defaultValue);
   bool     odbReadBool(  const char*name, int index, bool     defaultValue);
   double   odbReadDouble(const char*name, int index, double   defaultValue);
   float    odbReadFloat(const char*name, int index, float  defaultValue);
   const char* odbReadString(const char*name, int index, const char* defaultValue);
   int      odbReadArraySize(const char*name);
   
   MVOdb* Chdir(const char* subdir, bool create, MVOdbError* error)
   {
      SetOk(error);
      return NULL;
   }

   void ReadKey(const char* varname, int *tid, int *num_values, int *total_size, int *item_size, MVOdbError* error)
   {
      if (tid) *tid = 0;
      if (num_values) *num_values = 0;
      if (total_size) *total_size = 0;
      if (item_size)  *item_size = 0;
      SetOk(error);
   }

   void ReadDir(std::vector<std::string>* varname, std::vector<int> *tid, std::vector<int> *num_values, std::vector<int> *total_size, std::vector<int> *item_size, MVOdbError* error)
   {
      SetOk(error);
   }

   void RB(const char* varname, bool   *value, bool create, MVOdbError* error) { SetOk(error); };
   void RI(const char* varname, int    *value, bool create, MVOdbError* error) { SetOk(error); };
   void RD(const char* varname, double *value, bool create, MVOdbError* error) { SetOk(error); };
   void RF(const char* varname, float  *value, bool create, MVOdbError* error) { SetOk(error); };
   void RS(const char* varname, std::string *value, bool create, int create_string_length, MVOdbError* error) { SetOk(error); };
   void RU16(const char* varname, uint16_t *value, bool create, MVOdbError* error) { SetOk(error); };
   void RU32(const char* varname, uint32_t *value, bool create, MVOdbError* error) { SetOk(error); };

   void RBA(const char* varname, std::vector<bool> *value, bool create, int create_size, MVOdbError* error) { SetOk(error); };
   void RIA(const char* varname, std::vector<int> *value,  bool create, int create_size, MVOdbError* error) { SetOk(error); };
   void RDA(const char* varname, std::vector<double> *value,  bool create, int create_size, MVOdbError* error) { SetOk(error); };
   void RFA(const char* varname, std::vector<float>  *value,  bool create, int create_size, MVOdbError* error) { SetOk(error); };
   void RSA(const char* varname, std::vector<std::string> *value, bool create, int create_size, int create_string_length, MVOdbError* error) { SetOk(error); };
   void RU16A(const char* varname, std::vector<uint16_t> *value,  bool create, int create_size, MVOdbError* error) { SetOk(error); };
   void RU32A(const char* varname, std::vector<uint32_t> *value,  bool create, int create_size, MVOdbError* error) { SetOk(error); };

   void RBAI(const char* varname, int index, bool   *value, MVOdbError* error) { SetOk(error); };
   void RIAI(const char* varname, int index, int    *value, MVOdbError* error) { SetOk(error); };
   void RDAI(const char* varname, int index, double *value, MVOdbError* error) { SetOk(error); };
   void RFAI(const char* varname, int index, float  *value, MVOdbError* error) { SetOk(error); };
   void RSAI(const char* varname, int index, std::string *value, MVOdbError* error) { SetOk(error); };
   void RU16AI(const char* varname, int index, uint16_t *value, MVOdbError* error) { SetOk(error); };
   void RU32AI(const char* varname, int index, uint32_t *value, MVOdbError* error) { SetOk(error); };

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

   void Delete(const char* odbname, MVOdbError* error) { SetOk(error); };
};

#if 0
XmlOdb::XmlOdb(const char*xbuf,int bufLength) //ctor
{
   fPrintError = true;

  fOdb = NULL;
  fParser = new TDOMParser();
  fParser->SetValidate(false);

  char*buf = (char*)malloc(bufLength);
  memcpy(buf, xbuf, bufLength);
  for (int i=0; i<bufLength; i++)
    if (!isascii(buf[i])) {
      buf[i] = 'X';
    } else if (buf[i]=='\n') {
    } else if (buf[i]=='\r') {
    } else if (!isprint(buf[i])) {
      buf[i] = 'X';
    } else if (buf[i] == 0x1D) {
      buf[i] = 'X';
    }

  char* xend = strstr(buf,"odb>");
  if (xend)
    xend[4] = 0;

  //printf("end: %s\n", buf+bufLength-5);

  fParser->ParseBuffer(buf,bufLength);
  free(buf);
  TXMLDocument* doc = fParser->GetXMLDocument();
  if (!doc)
    {
      fprintf(stderr,"XmlOdb::XmlOdb: Malformed ODB dump: cannot get XML document\n");
      return;
    }

  fOdb = FindNode(doc->GetRootNode(),"odb");
  if (!fOdb)
    {
      fprintf(stderr,"XmlOdb::XmlOdb: Malformed ODB dump: cannot find <odb> tag\n");
      return;
    }
}

XmlOdb::XmlOdb(const char* filename) //ctor
{
   fPrintError = true;

  fOdb = NULL;
  fParser = new TDOMParser();
  fParser->SetValidate(false);

  int status = fParser->ParseFile(filename);
  if (status != 0)
    {
      fprintf(stderr,"XmlOdb::XmlOdb: Failed to parse XML file \'%s\', ParseFile() returned %d\n", filename, status);
      return;
    }

  TXMLDocument* doc = fParser->GetXMLDocument();
  if (!doc)
    {
      fprintf(stderr,"XmlOdb::XmlOdb: Malformed ODB dump: cannot get XML document\n");
      return;
    }

  fOdb = FindNode(doc->GetRootNode(),"odb");
  if (!fOdb)
    {
      fprintf(stderr,"XmlOdb::XmlOdb: Malformed ODB dump: cannot find <odb> tag\n");
      return;
    }
}
#endif

uint32_t XmlOdb::odbReadUint32(const char*name, int index, uint32_t defaultValue)
{
  TXMLNode *node = FindArrayPath(NULL,name,"DWORD",index);
  if (!node)
    return defaultValue;
  const char* text = node->GetText();
  if (!text)
    return defaultValue;
  return strtoul(text,NULL,0);
}

double   XmlOdb::odbReadDouble(const char*name, int index, double defaultValue)
{
  TXMLNode *node = FindArrayPath(NULL,name,"DOUBLE",index);
  if (!node)
    return defaultValue;
  const char* text = node->GetText();
  if (!text)
    return defaultValue;
  return atof(text);
}

float  XmlOdb::odbReadFloat(const char*name, int index, float defaultValue)
{
  TXMLNode *node = FindArrayPath(NULL,name,"FLOAT",index);
  if (!node)
    return defaultValue;
  const char* text = node->GetText();
  if (!text)
    return defaultValue;
  return atof(text);
}

int      XmlOdb::odbReadInt(   const char*name, int index, int      defaultValue)
{
  TXMLNode *node = FindArrayPath(NULL,name,"INT",index);
  if (!node)
    return defaultValue;
  const char* text = node->GetText();
  if (!text)
    return defaultValue;
  return atoi(text);
  //printf("for \'%s\', type is \'%s\', text is \'%s\'\n", name, typevalue, text);
  //DumpTree(node);
  //exit(1);
  return 0;
}

bool     XmlOdb::odbReadBool(  const char*name, int index, bool     defaultValue)
{
  TXMLNode *node = FindArrayPath(NULL,name,"BOOL",index);
  if (!node)
    return defaultValue;
  const char* text = node->GetText();
  if (!text)
    return defaultValue;
  if (*text == 'n')
    return false;
  return true;
}

const char* XmlOdb::odbReadString(const char* name, int index, const char* defaultValue)
{
  TXMLNode *node = FindArrayPath(NULL, name, "STRING", index);
  if (!node)
    return defaultValue;
  const char* text = node->GetText();
  if (!text)
    return defaultValue;
  return text;
}

int XmlOdb::odbReadArraySize(const char*name)
{
  TXMLNode *node = FindPath(NULL,name);
  if (!node)
    return 0;
  const char* num_values = GetAttrValue(node,"num_values");
  if (!num_values)
    return 1;
  return atoi(num_values);
}

MVOdb* MakeXmlFileOdb(const char* filename, MVOdbError* error)
{
   SetOk(error);
   return new XmlOdb();
}

MVOdb* MakeXmlBufferOdb(const char* buf, int bufsize, MVOdbError* error)
{
   SetOk(error);
   return new XmlOdb();
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

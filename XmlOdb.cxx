//
// ALPHA ROOT analyzer
//
// Name: XmlOdb.cxx
// Author: K.Olchanski, 11-July-2006
//
// $Id: XmlOdb.cxx,v 1.1 2006/07/11 18:53:18 alpha Exp $
//
// $Log: XmlOdb.cxx,v $
// Revision 1.1  2006/07/11 18:53:18  alpha
// KO- code to access to ODB from XML ODB dumps in MIDAS data files
//
//

#include <stdio.h>
#include <iostream>
#include <assert.h>

#include "XmlOdb.h"

XmlOdb::XmlOdb(const char*xbuf,int bufLength) //ctor
{
  fParser = new TDOMParser();
  fParser->SetValidate(false);

  char*buf = (char*)malloc(bufLength);
  memcpy(buf, xbuf, bufLength);
  for (int i=0; i<bufLength; i++)
    if (!isascii(buf[i]))
      buf[i] = 'X';
    else if (buf[i]=='\n')
      0;
    else if (buf[i]=='\r')
      0;
    else if (!isprint(buf[i]))
      buf[i] = 'X';
    else if (buf[i] == 0x1D)
      buf[i] = 'X';

  char* xend = strstr(buf,"odb>");
  if (xend)
    xend[4] = 0;

  //printf("end: %s\n", buf+bufLength-5);

  fParser->ParseBuffer(buf,bufLength);
  //TXmlDocument* xxx = fParser->GetXMLDocument();
  //assert(xxx);
  fOdb = FindNode(fParser->GetXMLDocument()->GetRootNode(),"odb");
  assert(fOdb);
}

XmlOdb::~XmlOdb() // dtor
{
  delete fParser;
  fParser = NULL;
}

TXMLNode* XmlOdb::FindNode(TXMLNode*node,const char*name)
{
  for (; node != NULL; node = node->GetNextNode())
    {
      //printf("node name: \"%s\"\n",node->GetNodeName());
      if (strcmp(node->GetNodeName(),name) == 0)
	return node;
      
      if (node->HasChildren())
	{
	  TXMLNode* found = FindNode(node->GetChildren(),name);
	  if (found)
	    return found;
	}
    }
  
  return NULL;
}

void XmlOdb::DumpTree(TXMLNode*node,int level)
{
  while (node)
    {
      for (int i=0; i<level; i++)
	printf(" ");
      printf("node name: \"%s\"\n",node->GetNodeName());
      TList* attrs = node->GetAttributes();
      TIter next(attrs);                           
      while (TXMLAttr *attr = (TXMLAttr*)next())                                
	{
	  for (int i=0; i<level; i++)
	    printf(" ");
	  printf("attribute name: \"%s\", value: \"%s\"\n",attr->GetName(),attr->GetValue());
	}
      const char*text = node->GetText();
      if (text)
	{
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

void XmlOdb::DumpDirTree(TXMLNode*node,int level)
{
  for (; node != NULL; node = node->GetNextNode())
    {
      const char* name = node->GetNodeName();
      
      if (strcmp(name,"dir") != 0)
	continue;
      
      for (int i=0; i<level; i++)
	printf(" ");
      printf("node name: \"%s\"\n",node->GetNodeName());
      TList* attrs = node->GetAttributes();
      TIter next(attrs);                           
      while (TXMLAttr *attr = (TXMLAttr*)next())                                
	{
	  for (int i=0; i<level; i++)
	    printf(" ");
	  printf("attribute name: \"%s\", value: \"%s\"\n",attr->GetName(),attr->GetValue());
	}
      if (node->HasChildren())
	DumpDirTree(node->GetChildren(),level + 1);
    }
  //printf("no more next nodes...\n");
}

//
// Return the value of the named attribute
//
const char* XmlOdb::GetAttrValue(TXMLNode*node,const char*attrName)
{
  TList* attrs = node->GetAttributes();
  TIter next(attrs);                           
  while (TXMLAttr *attr = (TXMLAttr*)next())                                
    {
      //printf("attribute name: \"%s\", value: \"%s\"\n",attr->GetName(),attr->GetValue());
      
      if (strcmp(attr->GetName(),attrName)==0)
	return attr->GetValue();
    }
  return NULL;
}

//
// Follow the ODB path through the XML DOM tree
//
TXMLNode* XmlOdb::FindPath(TXMLNode*node,const char* path)
{
  if (!node)
    node = fOdb->GetChildren();
  
  while (1)
    {
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
      for (int i=0; i<kElemSize; i++)
        {
          if (*path==0 || *path=='/')
            break;
          elem[i] = *path++;
        }
      
      //printf("looking for \"%s\" more \"%s\"\n",elem,path);
      
      for (; node != NULL; node = node->GetNextNode())
        {
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
	  
          if (strcasecmp(elem,namevalue) == 0)
            {
              if (isDir)
                {
                  // found the right subdirectory, descend into it
                  node = node->GetChildren();
                  break;
                }
              else if (isKey || isKeyArray)
                {
                  return node;
                }
            }
        }
    }
}

int      XmlOdb::odbReadAny(   const char*name, int index, int tid,void* value)    { assert(!"Not implemented!"); }
uint32_t XmlOdb::odbReadUint32(const char*name, int index, uint32_t defaultValue)  { assert(!"Not implemented!"); }

int      XmlOdb::odbReadInt(   const char*name, int index, int      defaultValue)
{
  TXMLNode *node = FindPath(NULL,name);
  if (!node)
    return defaultValue;
  const char* typevalue = GetAttrValue(node,"type");
  if (strcasecmp(typevalue,"INT") != 0)
    return defaultValue;
  const char* text = node->GetText();
  printf("for %s, type is %s, text is %s\n", name, typevalue, text);
  DumpTree(node);
  exit(1);
  return 0;
}

bool     XmlOdb::odbReadBool(  const char*name, int index, bool     defaultValue)
{
  TXMLNode *node = FindPath(NULL,name);
  if (!node)
    return defaultValue;
  const char* typevalue = GetAttrValue(node,"type");
  if (strcasecmp(typevalue,"BOOL") != 0)
    return defaultValue;
  const char* text = node->GetText();
  if (*text == 'n')
    return false;
  return true;
}

//end

//
// Access ODB from the XML ODB dump file
//
// Name: XmlOdb.h
// Author: K.Olchanski, 11-July-2006
//
// $Id: XmlOdb.h,v 1.1 2006/07/11 18:53:18 alpha Exp $
//

#ifndef INCLUDE_XmlOdb_H
#define INCLUDE_XmlOdb_H

#include "TXMLNode.h"
#include "TXMLAttr.h"
#include "TDOMParser.h"

#include "VirtualOdb.h"

/// Access to ODB saved in XML format inside midas .mid files

struct XmlOdb : VirtualOdb
{
  TDOMParser* fParser; ///< XML parser for the XML-encoded ODB data
  TXMLNode*   fOdb;    ///< Pointer to the root of the ODB tree

  /// Contructor
  XmlOdb(const char*buf,int bufLength);

  /// Destructor
  virtual ~XmlOdb();

  /// Find node with given name anywhere inside the given ODB tree
  TXMLNode* FindNode(TXMLNode*node,const char*name);

  /// Print out the contents of the ODB tree
  void DumpTree(TXMLNode*node,int level = 0);

  /// Print out the directory structure of the ODB tree
  void DumpDirTree(TXMLNode*node,int level = 0);

  /// Return the value of the named attribute
  const char* GetAttrValue(TXMLNode*node,const char*attrName);

  /// Follow the ODB path through the XML DOM tree
  TXMLNode* FindPath(TXMLNode*node,const char* path);

  int      odbReadAny(   const char*name, int index, int tid,void* value);
  int      odbReadInt(   const char*name, int index, int      defaultValue);
  uint32_t odbReadUint32(const char*name, int index, uint32_t defaultValue);
  bool     odbReadBool(  const char*name, int index, bool     defaultValue);
};

#endif
//end

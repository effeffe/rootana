//
// Virtual ODB access methods: online or offline from XML file
//
// Name: VirtualOdb.h
//
// $Id: VirtualOdb.h,v 1.1 2006/07/11 18:53:18 alpha Exp $
//
//

#ifndef INCLUDE_VirtualOdb_H
#define INCLUDE_VirtualOdb_H

/// Interface class for ODB access

struct VirtualOdb
{
  // ODB functions

  /// Read value of arbitrary type
  virtual int      odbReadAny(   const char*name, int index, int tid,void* value) = 0;
  /// Read an integer value, midas type TID_INT
  virtual int      odbReadInt(   const char*name, int index = 0, int      defaultValue = 0) = 0;
  /// Read an unsigned 32-bit integer value, midas type TID_DWORD
  virtual uint32_t odbReadUint32(const char*name, int index = 0, uint32_t defaultValue = 0) = 0;
  /// Read a boolean value, midas type TID_BOOL
  virtual bool     odbReadBool(  const char*name, int index = 0, bool     defaultValue = false) = 0;
};

#endif
// end


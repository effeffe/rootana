// @(#)root/base:$Name: v5-14-00f $:$Id: TDirectory.h,v 1.37 2006/06/20 18:17:34 pcanal Exp $
// Author: Rene Brun   28/11/94

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TNetDirectory
#define ROOT_TNetDirectory


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TDirectory                                                           //
//                                                                      //
// Describe directory structure in memory.                              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TDirectory
#include "TDirectory.h"
#endif

#include <string>
#include <deque>

class TNetDirectoryConnection;

class TNetDirectory : public TDirectory {

protected:
   TNetDirectoryConnection* fConn; //! pointer to network connection
   std::string              fPath;
   std::deque<TNetDirectory*> fSubDirs;

private:
   TNetDirectory(const TNetDirectory &directory);  //Directories cannot be copied
   void operator=(const TNetDirectory &); //Directorise cannot be copied
   TNetDirectory(); //TNetDirectory has no default constructor

public:

   //TNetDirectory(const char *name, const char *title, Option_t *option="", TDirectory* motherDir = 0);
   TNetDirectory(const char *name, TDirectory* motherDir = 0);
   TNetDirectory(TNetDirectoryConnection* conn, const char* dirname, const std::string& path, TDirectory* motherDir);
   virtual ~TNetDirectory();

   // manage connections
   int Reconnect();

   // special operations for online data
   void ResetTH1(const char* name);

   virtual void        Append(TObject *obj);
   virtual void        Browse(TBrowser *b);
   virtual void        Clear(Option_t *option="");
   virtual void        Close(Option_t *option="");
   virtual Bool_t      cd(const char *path = 0);
   virtual void        DeleteAll(Option_t *option="");
   virtual void        Delete(const char *namecycle="");
   virtual void        Draw(Option_t *option="");
   virtual void        FillBuffer(char *&buffer);
   virtual TKey       *FindKey(const char *keyname) const;
   virtual TKey       *FindKeyAny(const char *keyname) const;
   virtual TObject    *FindObject(const char *name) const;
   virtual TObject    *FindObject(const TObject *obj) const;
   virtual TObject    *FindObjectAny(const char *name) const;
   virtual TObject    *Get(const char *namecycle);
   virtual TDirectory *GetDirectory(const char *namecycle, Bool_t printError = false, const char *funcname = "GetDirectory");
   virtual void       *GetObjectChecked(const char *namecycle, const char* classname);
   virtual void       *GetObjectChecked(const char *namecycle, const TClass* cl);
   virtual void       *GetObjectUnchecked(const char *namecycle);
   virtual Int_t       GetBufferSize() const;
   virtual TFile      *GetFile() const;
   virtual TKey       *GetKey(const char *name, Short_t cycle=9999) const;
   virtual TList      *GetList() const;
   virtual TList      *GetListOfKeys() const;
   virtual Int_t       GetNbytesKeys() const;
   virtual Int_t       GetNkeys() const;
   virtual Long64_t    GetSeekDir() const;
   virtual Long64_t    GetSeekParent() const;
   virtual Long64_t    GetSeekKeys() const;
   virtual const char *GetPathStatic() const;
   virtual const char *GetPath() const;
   virtual void        ls(Option_t *option="") const;
   virtual TDirectory *mkdir(const char *name, const char *title="");
   virtual void        Paint(Option_t *option="");
   virtual void        Print(Option_t *option="") const;
   virtual void        Purge(Short_t nkeep=1);
   virtual void        pwd() const;
   virtual void        ReadAll(Option_t *option="");
   virtual Int_t       ReadKeys();
   virtual void        RecursiveRemove(TObject *obj);
   virtual void        rmdir(const char *name);
   virtual void        Save();
   virtual void        SaveSelf(Bool_t force = kFALSE);
   virtual void        SetBufferSize(Int_t bufsize);
   virtual void        SetName(const char* newname);
   virtual Int_t       Sizeof() const;
   virtual Int_t       Write(const char *name=0, Int_t opt=0, Int_t bufsiz=0);
   virtual Int_t       Write(const char *name=0, Int_t opt=0, Int_t bufsiz=0) const ;
   virtual Int_t       WriteTObject(const TObject *obj, const char *name=0, Option_t *option="");
   virtual Int_t       WriteObjectAny(const void *obj, const char *classname, const char *name, Option_t *option="");
   virtual Int_t       WriteObjectAny(const void *obj, const TClass *cl, const char *name, Option_t *option="");
   virtual void        WriteDirHeader();
   virtual void        WriteKeys();

   ClassDef(TNetDirectory,1)  //Describe directory structure in memory
};

#endif


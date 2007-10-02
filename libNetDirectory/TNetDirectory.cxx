//
// TNetDirectory.cxx
//

#include "TNetDirectory.h"

#include "TSocket.h"
#include "TMessage.h"
#include "TClass.h"
#include "TObjString.h"

static bool gVerbose = false;

class TNetDirectoryConnection
{
  TSocket* fSocket;

public:

  TNetDirectoryConnection(const char* host, int port)
  {
    fSocket = new TSocket(host, port);
    printf("Connected to %s:%d\n", host, port);
  }

  int Reconnect()
  {
    std::string host = fSocket->GetName();
    int         port = fSocket->GetPort();
    fSocket->Close();
    delete fSocket;
    fSocket = new TSocket(host.c_str(), port);
    printf("Connected to %s:%d\n", host.c_str(), port);
    return 0;
  }

  void Request(const char* req)
  {
    if (gVerbose)
      printf("Request [%s]\n", req);
    int s = fSocket->Send(req);
    //printf("Request sent %d\n", s);
  }

  TObject* ReadObject(TClass* type)
  {
    TMessage *mr = 0;
    int r = fSocket->Recv(mr);
    //printf("ReadObject recv %d\n", r);
    if (r <= 0) {
      printf("Error reading from socket!\n");
      return NULL;
    }

    TObject *obj = NULL;

    if (mr) {
      //mr->Print();
      obj = (TObject*)mr->ReadObjectAny(mr->GetClass());
    }

    //printf("mr %p, obj %p, class %p, %s\n", mr, obj, mr->GetClass(), mr->GetClass()->GetName());

    if (obj) {
      //obj->Print();

      if (!obj->InheritsFrom(type)) {
        printf("Object type mismatch, received %s, expected %s\n", obj->IsA()->GetName(), type->GetName());
        return NULL;
      }
    }

    if (mr)
      delete mr;

    return obj;
  }
};

TNetDirectory::TNetDirectory(const char *remoteServerName, TDirectory* motherDir)
  : TDirectory(remoteServerName, remoteServerName, "", motherDir)
{
  if (gVerbose)
    printf("TNetDirectory::ctor: %s\n", remoteServerName);
  int port = 9091;
  char hostname[256];
  strcat(hostname, remoteServerName);
  char* s = strchr(hostname, ':');
  if (s)
    {
      *s = 0;
      port = atoi(s+1);
    }
  fConn = new TNetDirectoryConnection(hostname, port);
  fPath = "";
}

TNetDirectory::TNetDirectory(TNetDirectoryConnection* conn, const char* dirname, const std::string& path, TDirectory* motherDir)
  : TDirectory(dirname, dirname, "", motherDir)
{
  if (gVerbose)
    printf("TNetDirectory::ctor: conn %p, path [%s]\n", conn, path.c_str());
  fConn = conn;
  fPath = path;
}

TNetDirectory::~TNetDirectory()
{
  if (gVerbose)
    printf("TNetDirectory::dtor\n");
  fConn = NULL;
}

int TNetDirectory::Reconnect()
{
  return fConn->Reconnect();
}

void TNetDirectory::ResetTH1(const char *name)
{
  if (gVerbose)
    printf("TNetDirectory(%s)::ResetTH1(%s)\n", fPath.c_str(), name);

  std::string req = "ResetTH1 ";
  if (fPath.length() > 0)
    {
      req += fPath;
      req += "/";
    }

  if (name && strlen(name)>0)
    {
      req += name;
    }

  fConn->Request(req.c_str());
  TObject *obj = fConn->ReadObject(TObject::Class());
  delete obj;
}

void        TNetDirectory::Append(TObject *obj)
{
  if (gVerbose)
    printf("TNetDirectory(%s)::Append(%p, name: %s)\n", fPath.c_str(), obj, obj->GetName());
}

void        TNetDirectory::Browse(TBrowser *b) { assert(!"not implemented"); }
void        TNetDirectory::Clear(Option_t *option) { assert(!"not implemented"); }
void        TNetDirectory::Close(Option_t *option) { assert(!"not implemented"); }
Bool_t      TNetDirectory::cd(const char *path) { assert(!"not implemented"); }
void        TNetDirectory::DeleteAll(Option_t *option) { assert(!"not implemented"); }
void        TNetDirectory::Delete(const char *namecycle) { assert(!"not implemented"); }
void        TNetDirectory::Draw(Option_t *option) { assert(!"not implemented"); }
void        TNetDirectory::FillBuffer(char *&buffer) { assert(!"not implemented"); }
TKey       *TNetDirectory::FindKey(const char *keyname) const { assert(!"not implemented"); };
TKey       *TNetDirectory::FindKeyAny(const char *keyname) const { assert(!"not implemented"); };

TObject    *TNetDirectory::FindObject(const char *name) const
{
  if (gVerbose)
    printf("TNetDirectory(%s)::FindObject(%s)\n", fPath.c_str(), name);

  for (unsigned int i=0; i<fSubDirs.size(); i++)
    {
      TNetDirectory *s = fSubDirs[i];
      if (strcmp(name, s->GetName()) == 0)
        {
          //printf("Return subdirectory %p [%s]\n", s, s->GetName());
          return s;
        }
    }

  std::string req = "FindObjectByName ";
  if (fPath.length() > 0)
    {
      req += fPath;
      req += "/";
    }
  req += name;

  fConn->Request(req.c_str());
  TObject *obj = fConn->ReadObject(TObject::Class());

  if (obj && strcmp(obj->IsA()->GetName(), "TObjString") == 0)
    {
      const char* s = ((TObjString*)obj)->GetName();
      if (strncmp(s, "TDirectory ", 11) == 0)
        {
          return NULL;
        }
    }

  return obj;
}

TObject    *TNetDirectory::FindObject(const TObject *obj) const
{
  assert(!"not implemented"); 
}

TObject    *TNetDirectory::FindObjectAny(const char *name) const { assert(!"not implemented"); }

TObject    *TNetDirectory::Get(const char *namecycle)
{
  if (gVerbose)
    printf("TNetDirectory(%s)::Get(%s)\n", fPath.c_str(), namecycle);

  std::string req = "FindObjectByName "; // "Get ";
  if (fPath.length() > 0)
    {
      req += fPath;
      req += "/";
    }
  req += namecycle;

  fConn->Request(req.c_str());
  TObject *obj = fConn->ReadObject(TObject::Class());

  if (obj && strcmp(obj->IsA()->GetName(), "TObjString") == 0)
    {
      const char* s = ((TObjString*)obj)->GetName();
      if (strncmp(s, "TDirectory ", 11) == 0)
        {
          const char* dirname = s+11;
          if (gVerbose)
            printf("Got TDirectory %s\n", dirname);
          TNetDirectory *s = new TNetDirectory(fConn, dirname, fPath + "/" + dirname, this);
          fSubDirs.push_back(s);
          return s;
        }
    }

  return obj;
}

TDirectory *TNetDirectory::GetDirectory(const char *namecycle, Bool_t printError, const char *funcname) { assert(!"not implemented"); }
void       *TNetDirectory::GetObjectChecked(const char *namecycle, const char* classname) { assert(!"not implemented"); }
void       *TNetDirectory::GetObjectChecked(const char *namecycle, const TClass* cl) { assert(!"not implemented"); }
void       *TNetDirectory::GetObjectUnchecked(const char *namecycle) { assert(!"not implemented"); }
Int_t       TNetDirectory::GetBufferSize() const { assert(!"not implemented"); }
TFile      *TNetDirectory::GetFile() const { return NULL; assert(!"not implemented"); return NULL; }
TKey       *TNetDirectory::GetKey(const char *name, Short_t cycle) const { assert(!"not implemented"); }
TList      *TNetDirectory::GetList() const { assert(!"not implemented"); }

TList      *TNetDirectory::GetListOfKeys() const
{
  if (gVerbose)
    printf("TNetDirectory(%s)::GetListOfKeys()\n", fPath.c_str());

  std::string req = "GetListOfKeys";
  if (fPath.length() > 0)
    {
      req += " ";
      req += fPath;
    }

  fConn->Request(req.c_str());
  TList *keys = (TList*)fConn->ReadObject(TList::Class());
  if (keys == NULL)
    //    return fKeys;
    return NULL;
  //keys->Print();
  //keys->ls();
  return keys;
}

Int_t       TNetDirectory::GetNbytesKeys() const { assert(!"not implemented"); }
Int_t       TNetDirectory::GetNkeys() const { assert(!"not implemented"); }
Long64_t    TNetDirectory::GetSeekDir() const { assert(!"not implemented"); }
Long64_t    TNetDirectory::GetSeekParent() const { assert(!"not implemented"); }
Long64_t    TNetDirectory::GetSeekKeys() const { assert(!"not implemented"); }
const char *TNetDirectory::GetPathStatic() const { assert(!"not implemented"); }
const char *TNetDirectory::GetPath() const { assert(!"not implemented"); }
void        TNetDirectory::ls(Option_t *option) const { assert(!"not implemented"); }
TDirectory *TNetDirectory::mkdir(const char *name, const char *title) { assert(!"not implemented"); }
void        TNetDirectory::Paint(Option_t *option) { assert(!"not implemented"); }

void        TNetDirectory::Print(Option_t *option) const
{
  TDirectory::Print(option);
}

void        TNetDirectory::Purge(Short_t nkeep) { assert(!"not implemented"); }
void        TNetDirectory::pwd() const { assert(!"not implemented"); }
void        TNetDirectory::ReadAll(Option_t *option) { assert(!"not implemented"); }
Int_t       TNetDirectory::ReadKeys() { assert(!"not implemented"); }
void        TNetDirectory::RecursiveRemove(TObject *obj) { assert(!"not implemented"); }
void        TNetDirectory::rmdir(const char *name) { assert(!"not implemented"); }
void        TNetDirectory::Save() { assert(!"not implemented"); }
void        TNetDirectory::SaveSelf(Bool_t force) { assert(!"not implemented"); }
void        TNetDirectory::SetBufferSize(Int_t bufsize) { assert(!"not implemented"); }
void        TNetDirectory::SetName(const char* newname) { assert(!"not implemented"); }
Int_t       TNetDirectory::Sizeof() const { assert(!"not implemented"); }
Int_t       TNetDirectory::Write(const char *name, Int_t opt, Int_t bufsiz) { assert(!"not implemented"); }
Int_t       TNetDirectory::Write(const char *name, Int_t opt, Int_t bufsiz) const { assert(!"not implemented"); }
Int_t       TNetDirectory::WriteTObject(const TObject *obj, const char *name, Option_t *option) { assert(!"not implemented"); }
Int_t       TNetDirectory::WriteObjectAny(const void *obj, const char *classname, const char *name, Option_t *option) { assert(!"not implemented"); }
Int_t       TNetDirectory::WriteObjectAny(const void *obj, const TClass *cl, const char *name, Option_t *option) { assert(!"not implemented"); }
void        TNetDirectory::WriteDirHeader() { assert(!"not implemented"); }
void        TNetDirectory::WriteKeys() { assert(!"not implemented"); }

//end

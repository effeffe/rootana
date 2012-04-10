//
// netDirectoryServer.h
//
// $Id$
//

class TDirectory;
class TFolder;
class TCollection;

class XmlServer
{
 public:
   void Start(int port);
   void SetVerbose(bool verbose);
   void Export(TDirectory* dir, const char* exportName);
   void Export(TFolder* folder, const char* exportName);
   void Export(TCollection* collection, const char* exportName);
};

// end

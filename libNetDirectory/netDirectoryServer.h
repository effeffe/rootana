//
// netDirectoryServer.h
//
// $Id$
//

class TDirectory;

void VerboseNetDirectoryServer(bool verbose);
void StartNetDirectoryServer(int port, TDirectory* dir);
void NetDirectoryExport(TDirectory* dir, const char* exportName);

// end

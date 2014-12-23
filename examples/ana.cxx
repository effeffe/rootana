// Default program for dealing with various standard TRIUMF VME setups:
// V792, V1190 (VME), L2249 (CAMAC), Agilent current meter
//
//

#include <stdio.h>
#include <iostream>
#include <time.h>

#include "TRootanaEventLoop.hxx"
#include "TAnaManager.hxx"


class Analyzer: public TRootanaEventLoop {




public:

	// An analysis manager.  Define and fill histograms in 
	// analysis manager.
	TAnaManager *anaManager;

  Analyzer() {
    //DisableAutoMainWindow();

		anaManager = 0;
    
  };

  virtual ~Analyzer() {};

  void Initialize(){


  }

	void Init(){

		if(anaManager)
			delete anaManager;
		anaManager = new TAnaManager();
	}


  void BeginRun(int transition,int run,int time){

		Init();

  }


  bool ProcessMidasEvent(TDataContainer& dataContainer){

		if(!anaManager) Init();

		anaManager->ProcessMidasEvent(dataContainer);

    return true;
  }


}; 


int main(int argc, char *argv[])
{

  Analyzer::CreateSingleton<Analyzer>();
  return Analyzer::Get().ExecuteLoop(argc, argv);

}


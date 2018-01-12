#ifndef TCamacADCHistograms_h
#define TCamacADCHistograms_h

#include <string>
#include "THistogramArrayBase.h"

/// Class for making histograms of Camac 12-chan ADC data
class TCamacADCHistograms : public THistogramArrayBase {
 public:
  TCamacADCHistograms();
  virtual ~TCamacADCHistograms(){};
  
  /// Update the histograms for this canvas.
  void UpdateHistograms(TDataContainer& dataContainer);

  /// Take actions at begin run
  void BeginRun(int transition,int run,int time);

  /// Take actions at end run  
  void EndRun(int transition,int run,int time);

private:

  void CreateHistograms();
    
};

#endif



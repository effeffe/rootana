#ifndef TTRB3Histograms_h
#define TTRB3Histograms_h

#include <string>
#include "THistogramArrayBase.h"

/// Class for making histograms of TRB3 FPGA-TDC data.
class TTRB3Histograms : public THistogramArrayBase {
 public:
  TTRB3Histograms();
  virtual ~TTRB3Histograms(){};
  
  /// Update the histograms for this canvas.
  void UpdateHistograms(TDataContainer& dataContainer);

  /// Take actions at begin run
  void BeginRun(int transition,int run,int time);

  /// Take actions at end run  
  void EndRun(int transition,int run,int time);

private:

  void CreateHistograms();
    
};

/// Class for making diff histograms of TRB3 FPGA-TDC data.
class TTRB3DiffHistograms : public THistogramArrayBase {
 public:
  TTRB3DiffHistograms();
  virtual ~TTRB3DiffHistograms(){};
  
  /// Update the histograms for this canvas.
  void UpdateHistograms(TDataContainer& dataContainer);

  /// Take actions at begin run
  void BeginRun(int transition,int run,int time);

  /// Take actions at end run  
  void EndRun(int transition,int run,int time);

private:

  void CreateHistograms();

  // specify the reference FPGA and Ch
  int fReferenceFpga; 
  int fReferenceCh;
  
};

#endif



#ifndef TSimpleHistogramCanvas_h
#define TSimpleHistogramCanvas_h


#include <iostream>
#include <string>
#include "TCanvasHandleBase.hxx"

#include "TH1.h"
#include "TGraph.h"

/// A canvas that plots a single TH1 or TGraph
class TSimpleHistogramCanvas : public TCanvasHandleBase{

public:


  TSimpleHistogramCanvas(TH1* histo, std::string name, std::string printoption = "");
  TSimpleHistogramCanvas(TGraph* graph, std::string name);

  ~TSimpleHistogramCanvas();

  /// Reset the histograms for this canvas
  void ResetCanvasHistograms();
  
  /// Update the histograms for this canvas. Doesn't do anything.
  // assumption is that the user will take care of filling histogram themself
  void UpdateCanvasHistograms(TDataContainer& dataContainer);
  
  /// Plot the histograms for this canvas
  void PlotCanvas(TDataContainer& dataContainer, TRootEmbeddedCanvas *embedCanvas);

  /// Take actions at begin run
  void BeginRun(int transition,int run,int time);

  /// Take actions at end run  
  void EndRun(int transition,int run,int time);

  // Add extra histogram
  void AddExtraHisto(TH1* h1){
    fExtraHistos.push_back(h1);
  };    
  
  // Add extra graph
  void AddExtraGraph(TGraph* g1){
    fExtraGraphs.push_back(g1);
  };

private:

  /// Pointer to the histogram 
  TH1* fHisto;
  /// Pointer to the graph 
  TGraph* fGraph;

  // Extra histograms and graphs
  std::vector<TH1*> fExtraHistos;
  std::vector<TGraph*> fExtraGraphs;
  
  // Print option for TH1F
  std::string fPrintOption;

  // Don't define default constructor.
  TSimpleHistogramCanvas();

};


#endif

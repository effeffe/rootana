#ifndef TMultiCanvas_h
#define TMultiCanvas_h

#include "TGraph.h"
#include "TH1.h"
#include "TH2.h"
#include "TCanvasHandleBase.hxx"

const int gMaxSubCanvasesMuCa = 4;

/// This is a canvas to display a set of four  histograms.
/// 
/// The class also supports adding single histograms or graphs, if you want a 
/// canvas that mixes plots for a part of detector with overall summary plots.
class TMulticanvas : public TCanvasHandleBase {
  
 private: 
    
  TH1 *fSummaryHistoSingle[gMaxSubCanvasesMuCa];
  TH2 *fSummaryHisto2D[gMaxSubCanvasesMuCa];
  TGraph *fSummaryGraphSingle[gMaxSubCanvasesMuCa];

  bool fHasHisto2D[gMaxSubCanvasesMuCa];
  bool fHasHistoSingle[gMaxSubCanvasesMuCa];
  bool fHasGraphSingle[gMaxSubCanvasesMuCa];

public:

  TMulticanvas(std::string CanvasName = "Multi Canvas");
   
 


	void AddHisto2D(TH2 *histo, int index){
		if(index >= 0 && index < gMaxSubCanvasesMuCa){
			fSummaryHisto2D[index] = histo;
			fHasHisto2D[index] = true;
		}        
	}
	
	void AddHistoSingle(TH1 *histo, int index){
		if(index >= 0 && index < gMaxSubCanvasesMuCa){
			fSummaryHistoSingle[index] = histo;
			fHasHistoSingle[index] = true;
		}        
	}
	
	void AddGraphSingle(TGraph *graph, int index){
		if(index >= 0 && index < gMaxSubCanvasesMuCa){
			fSummaryGraphSingle[index] = graph;
			fHasGraphSingle[index] = true;
		}        
	}
	
  
  /// Plot the histograms for this canvas for a particular index; 
	void PlotCanvas(TDataContainer& dataContainer, TRootEmbeddedCanvas *embedCanvas);
  
	// Don't do anything here; let deap ana manager handle updating/resetting.
	void ResetCanvasHistograms(){;};
	void UpdateCanvasHistograms(TDataContainer& dataContainer){;};


private:


};



#endif


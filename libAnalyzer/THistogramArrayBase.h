#ifndef THistogramArrayBase_h
#define THistogramArrayBase_h


#include <iostream>
#include <string>
#include <stdlib.h>

#include "TDataContainer.hxx"
#include "TH1F.h"
#include <vector>

/// Base class for user to create an array of histograms.
/// Features of the histogram array
/// i) Histograms are all defined together.
/// ii) Histograms are updated together
/// 
/// Users of this abstract base class should implement a class
/// derived from must THistogramArrayBase that
///  1) define the histograms that you want in the constructor.
///  2) implement the UpdateHistograms(TDataContainer&) method.
///
/// The logic of this histogram array is based on it being 
/// a set of similar histograms; if you use this class for
/// a set of dissimilar histograms (different binning, different quantities)
/// the results will probably be unsatisfactory.
/// The array'ness is actually implemented as a vector of TH1s.
class THistogramArrayBase : public std::vector<TH1*> {
 public:
  THistogramArrayBase(){}
  virtual ~THistogramArrayBase(){}

  /// Update the histograms for this canvas.
  virtual void UpdateHistograms(TDataContainer& dataContainer) = 0;

  /// A helper method for accessing each histogram.  Does bounds checking.
  TH1* GetHistogram(unsigned int i){
    if(i < 0 || i >= size()){
      std::cerr << "Invalid index (=" << i 
		<< ") requested in THistogramArrayBase::GetHistogram(int i) " << std::endl;
      return 0;
    }
    return (*this)[i];
  }

private:

};

#endif

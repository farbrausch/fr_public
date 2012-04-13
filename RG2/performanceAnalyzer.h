// This code is in the public domain. See LICENSE for details.

#ifndef __RG2_PERFORMANCEANALYZER_H_
#define __RG2_PERFORMANCEANALYZER_H_

#include "types.h"

class frPerformanceAnalyzer
{
  sInt    m_nCategories;
  sU64*   m_sums;
  sU64*   m_starts;
  sU64    m_total;
 
public:
  frPerformanceAnalyzer(sInt nCategories);
  ~frPerformanceAnalyzer();

  void        reset();
  void        beginMeasure(sInt category);
  void        endMeasure(sInt category);

  sInt        getPercentage(sInt category) const; // in 1/10000
};

#endif

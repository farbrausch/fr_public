// This code is in the public domain. See LICENSE for details.

// rg2 performance analyzer.

#include "stdafx.h"
#include "performanceAnalyzer.h"

static sU64 getTimer()
{
  LARGE_INTEGER timer;

  QueryPerformanceCounter(&timer);

  return timer.QuadPart;
}

// ---- frPerformanceAnalyzer

frPerformanceAnalyzer::frPerformanceAnalyzer(sInt nCategories)
  : m_nCategories(nCategories)
{
  m_sums = new sU64[nCategories];
  m_starts = new sU64[nCategories];

  reset();
}

frPerformanceAnalyzer::~frPerformanceAnalyzer()
{
  delete[] m_sums;
  delete[] m_starts;
}

void frPerformanceAnalyzer::reset()
{
  for (sInt i = 0; i < m_nCategories; i++)
  {
    m_sums[i] = 0;
    m_starts[i] = (sU64) -1;
    m_total = 0;
  }
}

void frPerformanceAnalyzer::beginMeasure(sInt category)
{
  FRDASSERT(category < m_nCategories);
  FRDASSERT(m_starts[category] == (sU64) -1);

  m_starts[category] = getTimer();
}

void frPerformanceAnalyzer::endMeasure(sInt category)
{
  FRDASSERT(category < m_nCategories);
  FRDASSERT(m_starts[category] != (sU64) -1);

  sU64 dif = getTimer() - m_starts[category];

  m_sums[category] += dif;
  m_total += dif;
}

sInt frPerformanceAnalyzer::getPercentage(sInt category) const
{
  FRDASSERT(category < m_nCategories);

  return m_total ? m_sums[category] * 10000 / m_total : 0 ;
}

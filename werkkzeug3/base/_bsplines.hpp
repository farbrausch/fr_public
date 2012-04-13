// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __BSPLINES_HPP__
#define __BSPLINES_HPP__

#include "_types.hpp"

/****************************************************************************/

// Sample point for curve fitting
struct sCurveSamplePoint
{
  sF32 Time;
  sF32 Value;
  sF32 DerivLeft;   // left derivative
  sF32 DerivRight;  // right derivative
  sInt Flags;       // bit 0=left deriv. valid, bit 1=right deriv. valid
                    // bit 2=fit to derivatives (default off)
};

// Generic-degree B-Spline class.
// Meant for numeric computations, not efficient real-time evaluation!
class sBSpline
{
  struct Sample
  {
    sF32 Time;      // normalized time in [0,1]
    sF32 Value;
    sInt Type;      // 0=normal 1=derivative
  };

  sInt FindKnot(sF32 time) const;

  void LeastSquaresFit(const Sample *samples,sInt nSamples);
  sBool FitCurveImpl(const Sample *samples,sInt nSamples,sF32 maxError,sInt maxRefinements);

public:
  sInt Degree;            // 0=constant 1=linear 2=quadratic 3=cubic ...
  sArray<sF32> Knots;
  sArray<sF32> Values;

  sBSpline(sInt degree);
  ~sBSpline();

  void CalcBasis(sF32 time,sInt &first,sF32 *weights) const;
  void CalcBasisDeriv(sF32 time,sInt &first,sF32 *weights) const;
  
  sF32 Evaluate(sF32 time) const;
  sF32 EvaluateDeriv(sF32 time) const;

  void FitToCurve(const sCurveSamplePoint *points,sInt nPoints,sF32 skipThresh,sF32 maxError);
};

/****************************************************************************/

#endif

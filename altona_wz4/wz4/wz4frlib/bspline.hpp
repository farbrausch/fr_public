/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_BSPLINE_HPP
#define FILE_WZ4FRLIB_BSPLINE_HPP

#include "base/types.hpp"
#include "base/types2.hpp"
#include "base/math.hpp"
#include "wz4lib/serials.hpp"

/****************************************************************************/

#define sBSPLINE_DEGREE   3   // Cubic B-Splines are perfect for our needs.
#define sBSPLINE_ORDER    (sBSPLINE_DEGREE + 1)

// Tool functions used for both fitting and runtime evaluation.
namespace sBSplineTool
{
  sInt CalcBasis(const sF32 *knots,sInt nKnots,sF32 time,sF32 *weights);
  sInt CalcBasisDeriv(const sF32 *knots,sInt nKnots,sF32 time,sF32 *weights);
}

/****************************************************************************/

// Cubic B-Spline fitting implementation. Don't use directly, sBSpline is the class
// you want to use.
class sBSplineFitter
{
public:
  sInt nChannels;
  sArray<sF32> Knots;
  sArray<sF32> Values;

  sBSplineFitter(sInt chans) : nChannels(chans) { }

  typedef sF32 (*ErrorMetric)(const sF32* a, const sF32* b, void* user);

  void Evaluate(sF32* dest, sF32 time) const;
  void AddKnot(sF32 time, sInt multiplicity);
  sBool FitToCurve(const sF32* samples,sInt nSamples,sF32 maxError,ErrorMetric metric,void* user);

private:
  void LeastSquaresFit(const sF32 *samples,sInt nSamples);
  sBool FitCurveImpl(const sF32 *samples,sInt nSamples,sF32 maxError,sInt maxRefinements,ErrorMetric metric,void *user);
};

/****************************************************************************/

// Lots of helpers for spline fitting. Don't look, this doesn't concern you!
template<class T> struct sBSplineHelper
{
  static T PostWeight(const T &in)        { return in; }

  static sF32 CalcError(const sF32 *ap, const sF32 *bp, void *user)
  {
    const T *a = (const T*) ap;
    const T *b = (const T*) bp;
    return (*a-*b).LengthSq();
  }

  static sF32 PrescaleError(sF32 err, const T *samples, sInt count)
  {
    sF32 magn = 1.0f;
    for(sInt i=0;i<count;i++)
      magn = sMax(magn,samples[i].LengthSq());
    return err*err*magn;
  }
};

template<> struct sBSplineHelper<sF32>
{
  static sF32 PostWeight(sF32 in)        { return in; }

  static sF32 CalcError(const sF32 *a, const sF32 *b, void *user)
  {
    return sSquare(*a - *b);
  }

  static sF32 PrescaleError(sF32 err, const sF32 *samples, sInt count)
  {
    sF32 magn = 1.0f;
    for(sInt i=0;i<count;i++)
      magn = sMax(magn,samples[i]*samples[i]);
    return err*err*magn;
  }
};

template<> struct sBSplineHelper<sQuaternion>
{
  static sQuaternion PostWeight(const sQuaternion& in) { sQuaternion t = in; t.Unit(); return t; }

  static sF32 CalcError(const sF32 *a, const sF32 *b, void *user)
  {
    sQuaternion s = *((const sQuaternion*) a), t = *((const sQuaternion*) b);
    s.Unit();
    t.Unit();
    sQuaternion d(s.r-t.r,s.i-t.i,s.j-t.j,s.k-t.k);
    return dot(d,d);
  }
 
  static sF32 PrescaleError(sF32 err, const sQuaternion *samples, sInt count) { return err*err*4.0f; }
};

/****************************************************************************/

// Cubic B-Spline with automatic fitting and reasonably efficient evaluation.
// This is the class (template) you want to use.
// IMPORTANT: *Only* use with types that only have sF32 members! (At least if you want to Serialize).
template<class T> class sBSpline
{
  sInt NumKnots;
  sF32 *Knots;
  T *ControlPoints;

public:
  sBSpline()                            { NumKnots = 0; Knots = 0; ControlPoints = 0; }
  sBSpline(const sBSpline<T> &x);
  ~sBSpline()                           { delete[] Knots; delete[] ControlPoints; }

  sBSpline<T> &operator =(const sBSpline<T> &x) { T temp = x; Swap(temp); }

  void Swap(sBSpline<T> &x)             { sSwap(NumKnots,x.NumKnots); sSwap(Knots,x.Knots); sSwap(ControlPoints,x.ControlPoints); }
  void Clear()                          { NumKnots = 0; sDelete(Knots); sDelete(ControlPoints); }

  sBool IsEmpty() const                 { return NumKnots == 0; }
  sInt GetNumKnots() const              { return NumKnots; }
  sInt GetNumControlPoints() const      { return NumKnots ? NumKnots - sBSPLINE_ORDER : 0; }

  const sF32* GetKnots() const          { return Knots; }
  const T* GetControlPoints() const     { return ControlPoints; }

  template <class streamer> void Serialize_(streamer &s);
  void Serialize(sWriter &s)            { Serialize_(s); }
  void Serialize(sReader &s)            { Serialize_(s); }

  void Evaluate(T& dest, sF32 time) const
  {
    sF32 w[4];
    sInt first = sBSplineTool::CalcBasis(Knots,NumKnots,time,w);
    const T *s = &ControlPoints[first];
    dest = sBSplineHelper<T>::PostWeight(w[0]*s[0] + w[1]*s[1] + w[2]*s[2] + w[3]*s[3]);
  }

  void EvaluateDeriv(T& dest, sF32 time) const
  {
    sF32 w[4];
    sInt first = sBSplineTool::CalcBasisDeriv(Knots,NumKnots,time,w);
    const T *s = &ControlPoints[first];
    dest = w[0]*s[0] + w[1]*s[1] + w[2]*s[2] + w[3]*s[3]; // this might be off for quaternions (a bit).
  }

  sBool FitToCurve(const T *samples,sInt nSamples,sF32 maxRelativeError,sF32 normalizedLength=1.0f);
  sBool IsConstant(sF32 relErrorThreshold) const;
};

template<class T> sBSpline<T>::sBSpline(const sBSpline<T> &x)
{
  NumKnots = x.NumKnots;
  Knots = new sF32[NumKnots];
  ControlPoints = new T[GetNumControlPoints()];
  sCopyMem(Knots,x.Knots,NumKnots*sizeof(sF32));
  sCopyMem(ControlPoints,x.ControlPoints,GetNumControlPoints()*sizeof(T));
}

template<class T> template<class streamer> void sBSpline<T>::Serialize_(streamer& stream)
{
  sInt version = stream.Header(sSerId::Wz4BSpline,1);
  if(version)
  {
    stream | NumKnots;

    if(stream.IsReading())
    {
      delete[] Knots;
      delete[] ControlPoints;

      if(NumKnots)
      {
        Knots = new sF32[NumKnots];
        ControlPoints = new T[GetNumControlPoints()];
      }
      else
      {
        Knots = 0;
        ControlPoints = 0;
      }
    }

    if(NumKnots)
    {
      // Knot vector: First and last one will always appear multiple times.
      // Only store significant part.
      stream.ArrayF32(&Knots[sBSPLINE_DEGREE],NumKnots - 2 * sBSPLINE_DEGREE);
      for(sInt i=0;i<sBSPLINE_DEGREE;i++)
      {
        Knots[i] = Knots[sBSPLINE_DEGREE];
        Knots[NumKnots - sBSPLINE_DEGREE + i] = Knots[NumKnots - sBSPLINE_ORDER];
      }
      stream.ArrayF32((sF32*) ControlPoints,GetNumControlPoints() * (sizeof(T)/sizeof(sF32)));
    }

    stream.Footer();
  }
}

template<class T> sBool sBSpline<T>::FitToCurve(const T *samples,sInt nSamples,sF32 maxRelativeError,sF32 normalizedLength)
{
  sBSplineFitter fit(sizeof(T)/sizeof(sF32));
  sF32 err = sBSplineHelper<T>::PrescaleError(maxRelativeError, samples, nSamples);
  sBool ok = fit.FitToCurve((const sF32*) samples, nSamples, err, sBSplineHelper<T>::CalcError, 0);

  delete[] Knots;
  delete[] ControlPoints;
  NumKnots = fit.Knots.GetCount(); 
  Knots = new sF32[NumKnots];
  ControlPoints = new T[NumKnots - sBSPLINE_ORDER];

  sCopyMem(Knots,&fit.Knots[0],NumKnots*sizeof(sF32));
  sCopyMem(ControlPoints,&fit.Values[0],GetNumControlPoints()*sizeof(T));

  if(nSamples)
  {
    normalizedLength /= nSamples-1;
    for(sInt i=0;i<NumKnots;i++)
      Knots[i] *= normalizedLength;
  }

  return ok;
}

template<class T> sBool sBSpline<T>::IsConstant(sF32 relErrorThreshold) const
{
  sF32 err = sBSplineHelper<T>::PrescaleError(relErrorThreshold, ControlPoints, GetNumControlPoints());
  for(sInt i=1;i<GetNumControlPoints();i++)
    if(sBSplineHelper<T>::CalcError((const sF32*) &ControlPoints[i],(const sF32*) &ControlPoints[0],0) > err)
      return sFALSE;

  return NumKnots != 0; // an empty curve doesn't count as constant
}

/****************************************************************************/

#endif // FILE_WZ4FRLIB_BSPLINE_HPP
// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#include "_bsplines.hpp"
#include <malloc.h>

/****************************************************************************/

template<class T> sInt lowerBound(const T *values,T key,sInt count)
{
  sInt l = 0;
  sInt r = count;

  while(l < r)
  {
    sInt x = (l + r) >> 1;
    if(key <= values[x])
      r = x;
    else
      l = x+1;
  }

  return l;
}

sInt sBSpline::FindKnot(sF32 time) const
{
  // range check
  if(time < Knots[0] || time >= Knots[Knots.Count - 1])
    return -1;

  // binary search for the right knot interval
  sInt l = lowerBound(&Knots[Degree+1],time,Knots.Count - 2*Degree - 1);

  // skip over equal knots while possible
  while(l < Knots.Count - 2 * (Degree + 1) && Knots[Degree+1+l] == time)
    l++;

  sVERIFY(l >= 0 && l <= Knots.Count - 2 * (Degree + 1));

  return l;
}

void sBSpline::LeastSquaresFit(const Sample *samples,sInt nSamples)
{
  // The input dataset is assumed to be on [0,1)
  // Now do a least-squares fit via normal equations, that is solve
  //   (A^T A) x = A^T b
  // for x.
  //
  // To do this, first compute the weights of the B-Spline at
  // sample points to determine A
  sInt deg = Degree,order = Degree + 1;
  sInt nKnots = Knots.Count;

  sF32 *A = new sF32[nSamples * order];
  sInt *start = new sInt[nSamples];

  for(sInt i=0;i<nSamples;i++)
    if(samples[i].Type == 0) // normal function value
      CalcBasis(samples[i].Time,start[i],A + i*order);
    else // derivative
      CalcBasisDeriv(samples[i].Time,start[i],A + i*order);

  // Next, calculate the matrix A^T A. This is always symmetrical and
  // positive semidefinite, and in this case also a banded matrix, so we
  // only need order * nKnots floats to store it
  sF32 *ATA = new sF32[nKnots * order];

  for(sInt i=0;i<nKnots;i++)
  {
    // samples for knot i
    sInt start1 = lowerBound(start,i-deg,nSamples);
    sInt end1 = lowerBound(start,i+1,nSamples);

    for(sInt j=0;j<order;j++)
    {
      // samples for knot i+j-deg
      sInt start2 = lowerBound(start,i+j-2*deg,nSamples);
      sInt end2 = lowerBound(start,i+j-deg+1,nSamples);

      // calculate overlap
      sInt commonStart = sMax(start1,start2);
      sInt commonEnd = sMin(end1,end2);

      // perform summation
      sF64 sum = 0.0f;
      for(sInt k=commonStart;k<commonEnd;k++)
      {
        sF32 *pos = &A[k*order + i-start[k]];
        sum += pos[0] * pos[j-deg];
      }

      ATA[i*order+j] = sum;
    }
  }

  // Compute a LDL^T decomposition of A^T A (in-place)
  sF32 *scale = new sF32[nKnots];
  for(sInt i=0;i<nKnots;i++)
  {
    // Solve for diagonal element
    sF64 d = ATA[i*order+deg];
    for(sInt j=sMax(i-deg,0);j<i;j++)
      d -= sSquare(ATA[i*order+(j-i)+deg])*ATA[j*order+deg];

    // Calculate inverse d, store back
    d = (sFAbs(d) > 1e-10f) ? d : 0.0f;
    sF64 id = d ? 1.0f / d : 0.0f;
    ATA[i*order+deg] = d;
    scale[i] = id;

    // Solve for rest
    for(sInt j=1;j<order;j++)
    {
      sInt row = i+j;

      if(row < nKnots)
      {
        sF64 sum = ATA[row*order+deg-j];
        for(sInt k=sMax(row-deg,0);k<i;k++)
          sum -= ATA[row*order+(k-row)+deg] * ATA[i*order+(k-i)+deg] * ATA[k*order+deg];

        ATA[row*order+deg-j] = id * sum;
      }
    }
  }

  // Now, actually solve the system. Anything before this only needs to be
  // done once per knot vector.
  // Compute A^T b
  for(sInt i=0;i<nKnots;i++)
  {
    sInt startw = lowerBound(start,i-deg,nSamples);
    sInt endw = lowerBound(start,i+1,nSamples);
    
    sF64 c = 0.0f;
    for(sInt j=startw;j<endw;j++)
      c += A[j*order + i-start[j]] * samples[j].Value;

    Values[i] = c;
  }

  // Solve Lx
  for(sInt i=0;i<nKnots;i++)
  {
    sF64 sum = Values[i];
    for(sInt j=sMax(i-deg,0);j<i;j++)
      sum -= ATA[i*order+(j-i)+deg] * Values[j];

    Values[i] = sum;
  }

  // Solve Dx
  for(sInt i=0;i<nKnots;i++)
    Values[i] *= scale[i];

  // Solve L^Tx (backwards substitution)
  for(sInt i=nKnots-2;i>=0;i--)
  {
    sF64 sum = Values[i];
    for(sInt j=i+1;j<sMin(i+order,nKnots);j++)
      sum -= ATA[j*order+(i-j)+deg] * Values[j];

    Values[i] = sum;
  }

  // Cleanup
  delete[] A;
  delete[] start;
  delete[] ATA;
  delete[] scale;
}

sBool sBSpline::FitCurveImpl(const Sample *samples,sInt nSamples,sF32 maxError,sInt maxRefinements)
{
  sArray<sF32> NewKnots;
  sBool Subdivided = sFALSE;

  // Initial fit
  LeastSquaresFit(samples,nSamples);
  
  // Refine as necessary
  NewKnots.Init(Knots.Count);

  while(maxRefinements-- > 0)
  {
    // Calculate new knot vector: Setup
    NewKnots.Count = 0;
    const Sample *sample = samples;
    const Sample *sampleEnd = samples + nSamples;
    Subdivided = sFALSE;

    *NewKnots.Add() = Knots[0];
    
    // Go through knot intervals, measuring error and subdiving as necesarry
    for(sInt i=1;i<Knots.Count;i++)
    {
      if(Knots[i] != Knots[i-1]) // nonempty interval
      {
        // Measure max. error (currently without derivatives)
        sF32 error = 0.0f;
        
        while(sample < sampleEnd && sample->Time < Knots[i])
        {
          if(sample->Type == 0) // only direct values
            error = sMax<sF32>(error,sFAbs(sample->Value - Evaluate(sample->Time)));

          sample++;
        }

        // Add new knot in middle if error bigger than threshold
        if(error >= maxError)
        {
          *NewKnots.Add() = (Knots[i-1] + Knots[i]) * 0.5f;
          Subdivided = sTRUE;
        }
      }

      // Add old knot value in any case
      *NewKnots.Add() = Knots[i];
    }

    // If not subdivided, we're done, else compute new approximation
    // and iterate
    if(!Subdivided)
      break;
    else
    {
      Knots.Swap(NewKnots);
      Values.Resize(Knots.Count);
      LeastSquaresFit(samples,nSamples);
    }
  }

  NewKnots.Exit();
  return !Subdivided;
}

/****************************************************************************/

sBSpline::sBSpline(sInt degree)
{
  Degree = degree;
  Knots.Init();
  Values.Init();
}

sBSpline::~sBSpline()
{
  Knots.Exit();
  Values.Exit();
}

void sBSpline::CalcBasis(sF32 time,sInt &first,sF32 *weights) const
{
  sInt deg = Degree;

  if(time < Knots[0])
  {
    first = 0;
    for(sInt i=0;i<=deg;i++)
      weights[i] = i == 0;
  }
  else if(time >= Knots[Knots.Count - deg - 1])
  {
    first = Knots.Count - deg - 1;
    for(sInt i=0;i<=deg;i++)
      weights[i] = i == deg;
  }
  else
  {
    first = FindKnot(time);
    sVERIFY(first != -1);

    const sF32 *knot = &Knots[first];

    // compute weights via recurrence
    sInt nWeights = deg * 2 + 1;
    sF32 *work = (sF32 *) _alloca(sizeof(sF32) * nWeights);

    // order-0 weights
    for(sInt i=0;i<nWeights;i++)
      work[i] = (time >= knot[i]) && (time < knot[i+1]);

    // higher orders
    for(sInt i=1;i<=deg;i++)
    {
      for(sInt j=0;j<nWeights-i;j++)
      {
        sF32 value = 0.0f;
        sF32 diff;

        diff = knot[j+i] - knot[j];
        if(diff > 0.0f)
          value += (time - knot[j]) * work[j] / diff;

        diff = knot[j+i+1] - knot[j+1];
        if(diff > 0.0f)
          value += (knot[j+i+1] - time) * work[j+1] / diff;

        work[j] = value;
      }
    }

    // store back
    for(sInt i=0;i<=deg;i++)
      weights[i] = work[i];
  }
}

void sBSpline::CalcBasisDeriv(sF32 time,sInt &first,sF32 *weights) const
{
  sInt deg = Degree;

  // At border points or for curves that don't have continous derivatives,
  // return zero
  if(!deg || time < Knots[0] || time >= Knots[Knots.Count - deg - 1])
  {
    // This special case is necessary so that the sequence of first(time)
    // for monotonously growing t is also monotonous
    first = time >= Knots[Knots.Count - deg - 1] ? Knots.Count - deg - 1 : 0;
    for(sInt i=0;i<=deg;i++)
      weights[i] = 0.0f;
  }
  else
  {
    first = FindKnot(time);
    sVERIFY(first != -1);

    const sF32 *knot = &Knots[first];

    // again, compute weights via recurrence. however, this time we only
    // need those up to degree-1
    // compute weights via recurrence
    sInt nWeights = deg * 2 + 1;
    sF32 *work = (sF32 *) _alloca(sizeof(sF32) * nWeights);

    // order-0 weights
    for(sInt i=0;i<nWeights;i++)
      work[i] = (time >= knot[i]) && (time < knot[i+1]);

    // higher orders
    for(sInt i=1;i<deg;i++)
    {
      for(sInt j=0;j<nWeights-i;j++)
      {
        sF32 value = 0.0f,diff;

        diff = knot[j+i] - knot[j];
        if(diff > 0.0f)
          value += (time - knot[j]) * work[j] / diff;

        diff = knot[j+i+1] - knot[j+1];
        if(diff > 0.0f)
          value += (knot[j+i+1] - time) * work[j+1] / diff;

        work[j] = value;
      }
    }

    // now, calculate derivative weights from what remains
    for(sInt j=0;j<=deg;j++)
    {
      sF32 deriv = 0.0f,diff;

      diff = knot[j+deg] - knot[j];
      if(diff > 0.0f)
        deriv += work[j] / diff;

      diff = knot[j+deg+1] - knot[j+1];
      if(diff > 0.0f)
        deriv -= work[j+1] / diff;

      weights[j] = deriv * deg;
    }
  }
}

sF32 sBSpline::Evaluate(sF32 time) const
{
  sInt deg = Degree;
  sF32 *weights = (sF32 *) _alloca(sizeof(sF32) * (deg + 1));
  sInt first;

  CalcBasis(time,first,weights);

  sF32 value = 0.0f;
  for(sInt i=0;i<=deg;i++)
    value += weights[i] * Values[first+i];

  return value;
}

sF32 sBSpline::EvaluateDeriv(sF32 time) const
{
  sInt deg = Degree;
  sF32 *weights = (sF32 *) _alloca(sizeof(sF32) * (deg + 1));
  sInt first;

  CalcBasisDeriv(time,first,weights);

  sF32 value = 0.0f;
  for(sInt i=0;i<=deg;i++)
    value += weights[i] * Values[first+i];

  return value;
}

void sBSpline::FitToCurve(const sCurveSamplePoint *points,sInt nPoints,sF32 skipThresh,sF32 maxError)
{
  if(nPoints < 2)
    return;

  // What this function does is generate a list of simple sample points
  // plus an initial knot vector for the actual approximation functions
  // to work with.
  sArray<Sample> samples;
  samples.Init(nPoints);

  Knots.Count = 0;

  // Find time range of samples
  sF32 minTime = points[0].Time, maxTime = points[0].Time;
  for(sInt i=1;i<nPoints;i++)
  {
    minTime = sMin(minTime,points[i].Time);
    maxTime = sMax(maxTime,points[i].Time);
  }

  // Generate sample points, plus knots at discontinuities
  for(sInt i=0;i<nPoints;i++)
  {
    sF32 time = (points[i].Time - minTime) / (maxTime - minTime);

    // Add value sample everywhere
    Sample *smp = samples.Add();
    smp->Time = time;
    smp->Value = points[i].Value;
    smp->Type = 0;

    // Add derivative sample if the derivative exists
    sBool derivExists = (points[i].Flags & 3) == 3 && sFAbs(points[i].DerivLeft - points[i].DerivRight) < 1e-6f;

    if(derivExists && (points[i].Flags & 4))
    {
      Sample *smp = samples.Add();
      smp->Time = time;
      smp->Value = points[i].DerivLeft;
      smp->Type = 1;
    }

    // Discontinuity checking
    sInt discontinuityOrder = Degree + 1;
    if(!derivExists && (points[i].Flags & 3)) // C^1 discontinuity
      discontinuityOrder = 1;

    // C^0 discontinuity?
    if(i == 0 || i == nPoints - 1 || sFAbs(points[i].Value - points[i-1].Value) > skipThresh)
      discontinuityOrder = 0;

    // Add knots as necessary
    for(sInt j=discontinuityOrder;j<=Degree;j++)
      *Knots.Add() = time;
  }

  Values.Resize(Knots.Count);

  // Perform actual curve fitting
  FitCurveImpl(&samples[0],samples.Count,maxError,8);

  samples.Exit();
}

/****************************************************************************/

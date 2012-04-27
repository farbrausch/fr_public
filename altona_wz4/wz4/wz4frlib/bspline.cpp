/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4frlib/bspline.hpp"

#define sBSPLINE_MAXCHANNELS    16

/****************************************************************************/

template<class T> static sInt LowerBound(const T *values,T key,sInt count)
{
  sInt l = 0, r = count;

  while(l < r)
  {
    sInt x = l + ((r - l) >> 1);
    if(key <= values[x])
      r = x;
    else
      l = x+1;
  }

  return l;
}

template<class T> static sINLINE sInt LowerBound(const sStaticArray<T>& values, T key)
{
  return LowerBound(&values[0], key, values.GetCount());
}

static sInt FindKnot(const sF32 *knots,sInt nKnots,sF32 time)
{
  // range check
  if(time < knots[0] || time >= knots[nKnots - 1])
    return -1;

  // binary search for the right knot interval
  sInt l = LowerBound(&knots[sBSPLINE_ORDER],time,nKnots - 2*sBSPLINE_DEGREE - 1);
  sVERIFY(l >= 0 && l <= nKnots - 2 * sBSPLINE_ORDER);
  sVERIFY(knots[l+sBSPLINE_DEGREE] <= time && time <= knots[l+sBSPLINE_DEGREE+1]);

  return l;
}

/****************************************************************************/

// Sample numerical approximation to 2nd derivative of target function at one point.
static void Sample2ndDeriv(sF32 *out,const sF32 *in,sInt nChans,sInt nSamples,sInt time)
{
  if(time < 1 || time >= nSamples-1)
  {
    for(sInt ch=0;ch<nChans;ch++)
      out[ch] = 0.0f;
  }
  else
  {
    sInt p = time*nChans;
    for(sInt ch=0;ch<nChans;ch++)
      out[ch] = in[p + ch - nChans] - 2.0f * in[p + ch] + in[p + ch + nChans];
  }
}

// Numerical approximation to 2nd derivative of target function.
static void Numerical2ndDeriv(sF32* out,const sF32* in,sInt nChans,sInt nSamples)
{
  for(sInt i=0;i<nSamples;i++)
    Sample2ndDeriv(out + i*nChans,in,nChans,nSamples,i);
}

// Determine threashold for peak determination.
static sF32 CalcPeakThreshold(const sF32* in,sInt nChans,sInt nSamples,sF32 K)
{
  sVERIFY(nChans <= sBSPLINE_MAXCHANNELS);
  sF32 avg[sBSPLINE_MAXCHANNELS], absSum = 0.0f, max = 0.0f;
  for(sInt i=0;i<nChans;i++)
    avg[i] = 0.0f;

  for(sInt i=0;i<nSamples;i++)
  {
    sF32 sum = 0.0f;
    for(sInt j=0;j<nChans;j++)
    {
      sum += sSquare(in[i*nChans + j]);
      avg[j] += in[i*nChans + j];
    }

    sF32 norm = sFSqrt(sum);
    absSum += norm;
    if(max < norm)
      max = norm;
  }

  for(sInt i=0;i<nChans;i++)
    avg[i] /= nSamples;

  sF32 devSum = 0.0f;
  for(sInt i=0;i<nSamples;i++)
  {
    sF32 sum = 0.0f;
    for(sInt j=0;j<nChans;j++)
      sum += sSquare(in[i*nChans + j] - avg[j]);

    devSum += sFSqrt(sum);
  }

  return 0.5f * (max + absSum / nSamples) + K * (devSum / nSamples);
}

// Square of 2-norm of a vector
static sF32 Norm2Sq(const sF32* in,sInt nChans)
{
  sF32 sum = 0.0f;
  for(sInt i=0;i<nChans;i++)  sum += sSquare(in[i]);
  return sum;
}

// Find peaks (of 2-norm) in a given signal; K determines sensitivity.
// If there's a run of peaks (possibly interrupted by non-peak runs as long as "hysteresis"),
// only the single highest peak in that run is reported.
static void FindPeaks(sArray<sInt>& peaks,const sF32* in,sInt nChans,sInt nSamples,sF32 K,sInt hysteresis)
{
  sF32 threshSq = sSquare(CalcPeakThreshold(in,nChans,nSamples,K));
  sF32 curPeakVal = 0.0f;
  sInt curPeakAt = -1, inPeakRun = 0;

  for(sInt i=0;i<nSamples;i++)
  {
    sF32 sum = Norm2Sq(in + i*nChans,nChans);

    if(sum > threshSq)
    {
      inPeakRun = hysteresis;
      if(sum > curPeakVal)
      {
        curPeakAt = i;
        curPeakVal = sum;
      }
    }
    else if(inPeakRun && --inPeakRun == 0)
    {
      peaks.AddTail(curPeakAt);
      curPeakVal = 0.0f;
    }
  }

  if(inPeakRun)
    peaks.AddTail(curPeakAt);
}


/****************************************************************************/

namespace sBSplineTool
{
  sInt CalcBasis(const sF32 *knots,sInt nKnots,sF32 time,sF32 *weights)
  {
    sInt first;

    if(time < knots[0])
    {
      first = 0;
      for(sInt i=0;i<=sBSPLINE_DEGREE;i++)
        weights[i] = i == 0;
    }
    else if(time >= knots[nKnots - sBSPLINE_ORDER])
    {
      first = nKnots - 2*sBSPLINE_ORDER;
      for(sInt i=0;i<=sBSPLINE_DEGREE;i++)
        weights[i] = i == sBSPLINE_DEGREE;
    }
    else
    {
      first = FindKnot(knots,nKnots,time);
      sVERIFY(first != -1 && first >= 0 && first + sBSPLINE_DEGREE*2 + 1 < nKnots);

      const sF32 *knot = &knots[first];

      // compute weights via recurrence
      const sInt nWeights = sBSPLINE_DEGREE * 2 + 1;
      sF32 work[nWeights];

      // order-0 weights
      for(sInt i=0;i<nWeights;i++)
        work[i] = (time >= knot[i]) && (time < knot[i+1]) ? 1.0f : 0.0f;

      // higher orders
      for(sInt i=1;i<=sBSPLINE_DEGREE;i++)
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
      for(sInt i=0;i<=sBSPLINE_DEGREE;i++)
        weights[i] = work[i];
    }

    return first;
  }

  sInt CalcBasisDeriv(const sF32 *knots,sInt nKnots,sF32 time,sF32 *weights)
  {
    sInt first;

    // At border points or for curves that don't have continous derivatives,
    // return zero
    if(!sBSPLINE_DEGREE || time < knots[0] || time >= knots[nKnots - sBSPLINE_ORDER])
    {
      // This special case is necessary so that the sequence of first(time)
      // for monotonously growing t is also monotonous
      first = time >= knots[nKnots - sBSPLINE_ORDER] ? nKnots - sBSPLINE_ORDER : 0;
      for(sInt i=0;i<=sBSPLINE_DEGREE;i++)
        weights[i] = 0.0f;
    }
    else
    {
      first = FindKnot(knots,nKnots,time);
      sVERIFY(first != -1);

      const sF32 *knot = &knots[first];

      // again, compute weights via recurrence. however, this time we only
      // need those up to degree-1
      // compute weights via recurrence
      const sInt nWeights = sBSPLINE_DEGREE * 2 + 1;
      sF32 work[nWeights];

      // order-0 weights
      for(sInt i=0;i<nWeights;i++)
        work[i] = (time >= knot[i]) && (time < knot[i+1]);

      // higher orders
      for(sInt i=1;i<sBSPLINE_DEGREE;i++)
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
      for(sInt j=0;j<=sBSPLINE_DEGREE;j++)
      {
        sF32 deriv = 0.0f,diff;

        diff = knot[j+sBSPLINE_DEGREE] - knot[j];
        if(diff > 0.0f)
          deriv += work[j] / diff;

        diff = knot[j+sBSPLINE_DEGREE+1] - knot[j+1];
        if(diff > 0.0f)
          deriv -= work[j+1] / diff;

        weights[j] = deriv * sBSPLINE_DEGREE;
      }
    }

    return first;
  }
}

/****************************************************************************/

void sBSplineFitter::Evaluate(sF32* dest, sF32 time) const
{
  sF32 weights[sBSPLINE_ORDER];
  sInt first = sBSplineTool::CalcBasis(&Knots[0],Knots.GetCount(),time,weights);

  const sF32 *vp = &Values[first*nChannels];
  for(sInt ch=0;ch<nChannels;ch++,vp++)
  {
    sF32 value = 0.0f;
    for(sInt i=0;i<=sBSPLINE_DEGREE;i++)
      value += weights[i] * vp[i*nChannels];

    dest[ch] = value;
  }
}

void sBSplineFitter::AddKnot(sF32 time, sInt multiplicity)
{
  while(multiplicity-- > 0)
    Knots.AddTail(time);
}

sBool sBSplineFitter::FitToCurve(const sF32 *samples,sInt nSamples,sF32 maxError,ErrorMetric metric,void* user)
{
  if(nSamples < 2)
    return sFALSE;

  // Find locations of C^1 discontinuities (to insert multiple knots there).
  // You can do the same for C^2 and C^0 discontinuities, and it probably makes sense
  // to do so; but I don't currently have usable test data for this, so I'll be conservative.
  sF32 *deriv2 = new sF32[nSamples * nChannels];
  sArray<sInt> c1Discont;
  Numerical2ndDeriv(deriv2,samples,nChannels,nSamples);
  FindPeaks(c1Discont,deriv2,nChannels,nSamples,2.0f,sBSPLINE_ORDER);
  delete[] deriv2;

  // Generate initial knot vector.
  Knots.Clear();
  AddKnot(0, sBSPLINE_ORDER);
  for(sInt i=0;i<c1Discont.GetCount();i++)
    AddKnot(c1Discont[i],sBSPLINE_ORDER - 1);
  AddKnot(nSamples-1,sBSPLINE_ORDER);

  // Perform actual curve fitting
  return FitCurveImpl(samples,nSamples,maxError,16,metric,user);
}

/****************************************************************************/

void sBSplineFitter::LeastSquaresFit(const sF32 *samples,sInt nSamples)
{
  // Weighted least-squares fit via normal equations, solving (A^T W A) x = A^T W b for x.
  // (via Cholesky decomposition).
  // The input curve is "oversampled" to make sure there are enough data points even in areas
  // where knot density is high; interpolated data points have a significantly lower weight to make
  // sure they don't affect the fit too much.
  static const float eps = 1e-6f;   // Threshold to mark rows as singular. This should be fine.
  static const sInt oversample = 2; // It's important to have samples *between* knots aswell.
  static const sF32 interWeight = 1.0f / 64.0f; // Weight of interpolated samples.
  const sInt deg = sBSPLINE_DEGREE, order = sBSPLINE_ORDER;
  sInt nKnots = Knots.GetCount();
  sInt nControl = nKnots - order;
  sInt nOverSamples = (nSamples - 1) * oversample + 1;

  Values.Resize(nControl * nChannels);

  // First compute A = (a_ij), a nOverSamples*nControl-sized matrix that simply gives the
  // influence of every knot for every sample in the final curve. This is, by construction,
  // relatively sparse (at most sBSPLINE_DEGREE+1 nonzero columns in each row); the (somewhat weird)
  // way to store A exploits this.
  sStaticArray<sF32> A(nOverSamples * order);
  sStaticArray<sInt> Astart(nOverSamples);

  A.AddMany(nOverSamples * order);
  Astart.AddMany(nOverSamples);

  for(sInt i=0;i<nOverSamples;i++)
    Astart[i] = sBSplineTool::CalcBasis(&Knots[0],Knots.GetCount(),1.0f * i / oversample,&A[i*order]);

  // Now compute A^T W A. This is a symmetrical, positive semidefinite, banded matrix;
  // we'll later decompose this (via Cholesky) to LL^T-form, so it makes sense to
  // directly store the components in a suitable format. We only need space for nControl*order
  // values, ATA[i*order+j] = a(i+j,i) <=> a(i,j) = ATA[j*order + (i-j)]
  // (A^T W A)_ij is effectively the dot product of column i of A with column j of A.
  // Compute A^T W b at the same time, while we're at it.
  sStaticArray<sF64> ATA(nControl * order);
  ATA.AddMany(nControl * order);
  for(sInt i=0;i<nControl;i++)
  {
    // Find first and last rows of A that reference column i
    sInt i0 = LowerBound(Astart, i-sBSPLINE_DEGREE);
    sInt i1 = LowerBound(Astart, i+1);

    // A^T W A
    for(sInt j=0;j<=sBSPLINE_DEGREE;j++)
    {
      // Find first and last rows of A that reference column i+j
      sInt j0 = LowerBound(Astart, i+j-sBSPLINE_DEGREE);
      sInt j1 = LowerBound(Astart, i+j+1);

      // Compute the dot product
      sF64 sum = 0.0;
      for(sInt k=sMax(i0,j0);k<sMin(i1,j1);k++)
      {
        sInt kRest = k % oversample;
        sum += A[k*order + (i - Astart[k])] * A[k*order + (i + j - Astart[k])] * (kRest ? interWeight : 1.0f);
      }

      ATA[i*order+j] = sum;
    }

    // A^T W b
    for(sInt ch=0;ch<nChannels;ch++)
    {
      sF64 sum = 0.0;
      for(sInt j=i0;j<i1;j++)
      {
        sInt jRest = j % oversample;
        sInt smp0 = j / oversample, smp1 = sMin(smp0+1,nSamples-1);
        sF32 sFrac = 1.0f * jRest / oversample;
        sF32 sample = (1.0f - sFrac) * samples[smp0*nChannels + ch] + sFrac * samples[smp1*nChannels + ch];
        sum += A[j*order + i-Astart[j]] * (jRest ? interWeight : 1.0f) * sample;
      }

      Values[i*nChannels + ch] = sum;
    }
  }

  // In-place Cholesky (LL^T) decomposition of A^T W A. Outer loop is in *columns*.
  for(sInt i=0;i<nControl;i++)
  {
    for(sInt j=i;j<sMin(i+order,nControl);j++)
    {
      sF64 sum = ATA[i*order + (j-i)];
      for(sInt k=sMax(j-sBSPLINE_DEGREE,0);k<i;k++)
        sum -= ATA[k*order + (i-k)] * ATA[k*order + (j-k)];

      if(i == j) // Diagonal element
        sum = (sum < eps) ? 0.0 : 1.0 / sqrt(sum);
      else
        sum *= ATA[i*order];

      ATA[i*order + (j-i)] = sum;
    }
  }

  // Solve Ly = A^T W b (forwards substitution) - for <nChannels> solution vectors at once.
  for(sInt i=0;i<nControl;i++)
  {
    for(sInt ch=0;ch<nChannels;ch++)
    {
      sF64 sum = Values[i*nChannels + ch];
      for(sInt j=sMax(i-deg,0);j<i;j++)
        sum -= ATA[j*order + (i-j)] * Values[j*nChannels + ch];

      if(!ATA[i*order]) // on singular rows, use variation diminishing approx.
      {
        sF32 tSum = 0.0f;
        for(sInt j=1;j<=sBSPLINE_DEGREE;j++)
          tSum += Knots[i+j];

        tSum /= sBSPLINE_DEGREE;
        sInt tSamp0 = sInt(tSum);
        sInt tSamp1 = sMin(tSamp0+1,nSamples-1);
        sF32 tFrac = tSum - tSamp0;
        sum = (1.0f-tFrac) * samples[tSamp0*nChannels + ch] + tFrac * samples[tSamp1*nChannels + ch];
      }
      else
        sum *= ATA[i*order];

      Values[i*nChannels + ch] = sum;
    }
  }

  // Solve L^T x = y (backwards substitution)
  for(sInt i=nControl-1;i>=0;i--)
  {
    for(sInt ch=0;ch<nChannels;ch++)
    {
      sF64 sum = Values[i*nChannels + ch];
      for(sInt j=i+1;j<sMin(i+order,nControl);j++)
        sum -= ATA[i*order + (j-i)] * Values[j*nChannels + ch];

      if(ATA[i*order])
        Values[i*nChannels + ch] = sum * ATA[i*order];
    }
  }

  // Congratulations, your system is now solved! This would be the right time
  // to fill out your registration card.
}

sBool sBSplineFitter::FitCurveImpl(const sF32 *samples,sInt nSamples,sF32 maxError,sInt maxRefinements,ErrorMetric metric,void* user)
{
  sArray<sF32> NewKnots;
  sVERIFY(nChannels <= sBSPLINE_MAXCHANNELS);

  // Initial fit
  LeastSquaresFit(samples,nSamples);

  // Calculate (de Boor-style) smoothness functional for *samples*, to later use it to drive
  // the refinement process. This is backwards from the way de Boor actually does refinement (which
  // is based on the smoothness of the approximating spline), but we don't have the luxury of knowing
  // beforehand how many knots we'll need. It still gives reasonable results.
  sF32 *smFunctional = new sF32[nSamples];
  sF64 sum = 0.0;
  for(sInt i=0;i<nSamples;i++)
  {
    sF32 deriv2[sBSPLINE_MAXCHANNELS];
    Sample2ndDeriv(deriv2,samples,nChannels,nSamples,i);
    sum += sFSqrt(sFSqrt(Norm2Sq(deriv2,nChannels)));
    smFunctional[i] = sum;
  }
  
  // Refine as necessary
  sBool forceSubd = sFALSE, gotErrors;

  for(sInt iter=0;iter<maxRefinements;iter++)
  {
    // Calculate new knot vector: Setup
    sInt curSample = 0;

    gotErrors = sFALSE;
    NewKnots.Clear();
    NewKnots.HintSize(Knots.GetCount() * 3 / 2);
    NewKnots.AddTail(Knots[0]);
    
    // Go through knot intervals, measuring error and subdividing as necesarry
    for(sInt i=1;i<Knots.GetCount();i++)
    {
      if(Knots[i] != Knots[i-1]) // nonempty interval
      {
        // Measure max error
        sF32 errorMax = 0.0f;
        sInt startSample = curSample;
        
        while(curSample < nSamples && curSample <= Knots[i])
        {
          sF32 temp[sBSPLINE_MAXCHANNELS];
          Evaluate(temp,curSample);
          sF32 error = metric(temp,samples + curSample*nChannels,user);
          errorMax = sMax(errorMax,error);
          curSample++;
        }

        sInt endSample = curSample - 1;
        //sInt len = curSample - startSample;

        if(errorMax >= maxError) // Add new knot in middle if error bigger than threshold
        {
          gotErrors = sTRUE;

          // find midpoint of smoothness functional over this interval
          sInt split = startSample;
          sF32 smCut = 0.5f * (smFunctional[startSample] + smFunctional[endSample]);
          while(split < endSample && smFunctional[split] < smCut)
            split++;

          if(split > startSample && split < endSample)
            NewKnots.AddTail(split);
          else if(iter >= sBSPLINE_ORDER || forceSubd) // bisect if there's nothing else we can do
          {
            sInt t = sInt(0.5f * (Knots[i-1] + Knots[i]));
            if(t <= Knots[i-1]) t++;
            if(t < Knots[i])    NewKnots.AddTail(t);
          }
        }
      }

      // Add old knot value in any case
      NewKnots.AddTail(Knots[i]);
    }

    // If not subdivided, we're done, else compute new approximation and iterate.
    forceSubd = sFALSE;
    if(NewKnots.GetCount() == Knots.GetCount())
    {
      if(!gotErrors)
        break;
      else
        forceSubd = sTRUE;
    }
    else
    {
      Knots.Swap(NewKnots);
      LeastSquaresFit(samples,nSamples);
    }
  }

  delete[] smFunctional;
  return !gotErrors;
}

/****************************************************************************/


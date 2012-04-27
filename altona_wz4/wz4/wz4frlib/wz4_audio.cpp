/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4_audio.hpp"
#include "util/image.hpp"
#include "wz4lib/gui.hpp"

/****************************************************************************/

// from numerical recipes in C
// you need to scale the IFFT by 1/N yourself
static void sFFT(sComplex *dataComplex,sInt count,sBool inverse)
{
  sVERIFY(count && (count & (count-1)) == 0);
  sF32 *data = &dataComplex->r - 1;

  // bit reversal
  sInt n = count*2;
  sInt j = 1;
  for(sInt i=1;i<n;i+=2)
  {
    if(j>i)
    {
      sSwap(data[j],data[i]);
      sSwap(data[j+1],data[i+1]);
    }
    sInt m = n >> 1;
    while(m>=2 && j>m)
    {
      j -= m;
      m >>= 1;
    }
    j += m;
  }

  // actual fft
  sInt mmax = 2;
  while(n > mmax)
  {
    sInt istep = mmax << 1;
    sF32 theta = (inverse ? -1.0f : 1.0f) * sPI2F / mmax;
    sF32 wtemp = sFSin(0.5f * theta);
    sF32 wpr = -2.0f * wtemp * wtemp;
    sF32 wpi = sFSin(theta);
    sF32 wr = 1.0f, wi = 0.0f;

    for(sInt m=1;m<mmax;m+=2)
    {
      for(sInt i=m;i<=n;i+=istep)
      {
        j = i+mmax;
        sF32 tempr = wr*data[j] - wi*data[j+1];
        sF32 tempi = wr*data[j+1] + wi*data[j];
        data[j] = data[i] - tempr;
        data[j+1] =data[i+1] - tempi;
        data[i] += tempr;
        data[i+1] += tempi;
      }
      wtemp = wr;
      wr = wtemp*wpr - wi*wpi + wr;
      wi = wi*wpr + wtemp*wpi + wi;
    }
    
    mmax = istep;
  }
}

/****************************************************************************/


/****************************************************************************/

// Polyphase 2-decimator; expanded symmetrically
static const sInt HALF_FILTER_SIZE = 12;
static const sF32 DecimFilterCoeff[2][HALF_FILTER_SIZE+1] =
{
  {
    0.0000000000000000e+000,
    0.0000000000000000e+000,
    0.0000000000000000e+000,
    0.0000000000000000e+000,
    0.0000000000000000e+000,
    0.0000000000000000e+000,
    0.0000000000000000e+000,
    0.0000000000000000e+000,
    0.0000000000000000e+000,
    0.0000000000000000e+000,
    0.0000000000000000e+000,
    0.0000000000000000e+000,
    5.0000000000000000e-001,
  },
  {
    -1.0053834454252398e-004,
     3.8703770665648128e-004,
    -1.0214027642620211e-003,
     2.2285540583012369e-003,
    -4.3090137395774266e-003,
     7.6610717323862222e-003,
    -1.2841319952087760e-002,
     2.0735593738449538e-002,
    -3.3069171276802908e-002,
     5.4201906591154647e-002,
    -1.0017100235034428e-001,
     3.1628804832562385e-001,
     0.0000000000000000e+000, // ignored
  },
};

/****************************************************************************/

namespace Wz4Audiolyzer
{
  sBool AudioAvailable()
  {
    return App->MusicData != 0;
  }

  void RMSImage(sImage *out,sInt width,sInt height,sInt chunkSize)
  {
    out->Init(width,height);

    sU32 *ptr = out->Data;
    for(sInt i=0;i<width;i++)
    {
      sInt startSmp = i*chunkSize;
      sInt endSmp = sMin((i+1)*chunkSize,App->MusicSize);

      sF32 sum = 0.0f;
      for(sInt j=startSmp;j<endSmp;j++)
      {
        sF32 sample = 0.5f * (App->MusicData[j*2+0] + App->MusicData[j*2+1]) / 32768.0f;
        sum += sample*sample;
      }

      sF32 intens = (endSmp>startSmp) ? sFSqrt(sum / (endSmp-startSmp)) : 0.0f;
      sU32 col = 0xff000000 + sClamp<sInt>(intens*255,0,255)*0x010101;
      for(sInt j=0;j<height;j++)
        ptr[j*width+i] = col;
    }
  }

  void Spectogram(sImage *out,sInt width,sInt fftSize)
  {
    sInt fftHalf = fftSize/2;
    sInt overlap = fftHalf;
    sF32 *windowFunction = new sF32[fftSize];
    for(sInt i=0;i<fftSize;i++)
      windowFunction[i] = 0.5f * (1.0f + sFCos((i - fftHalf) * sPIF / fftHalf));

    out->Init(width,fftHalf);
    sComplex *fftVec = new sComplex[fftSize];

    sU32 *ptr = out->Data;
    for(sInt i=0;i<width;i++)
    {
      // read fft inputs
      sInt startSmp = i*overlap;
      for(sInt j=0;j<fftSize;j++)
      {
        sInt smp = sMin(startSmp+j,App->MusicSize-1);
        sF32 sample = 0.5f * (App->MusicData[smp*2+0] + App->MusicData[smp*2+1]) / 32768.0f;
        fftVec[j] = sample * windowFunction[j];
      }

      // calc fft
      sFFT(fftVec,fftSize,sFALSE);

      // gen output
      for(sInt j=0;j<fftHalf;j++)
      {
        sF32 magn = sFSqrt(fftVec[j].r*fftVec[j].r + fftVec[j].i*fftVec[j].i);
        sU32 col = 0xff000000 + sClamp<sInt>(magn*255,0,255)*0x010101;
        ptr[(fftHalf-1-j)*width+i] = col;
      }
    }

    delete[] fftVec;
    delete[] windowFunction;
  }
}

/****************************************************************************/


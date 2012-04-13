// This file is distributed under a BSD license. See LICENSE.txt for details.

// FRIED
// quantization functions.

#include "_types.hpp"
#include "fried_internal.hpp"

namespace FRIED
{
  // ---- the quantization tables themselves
  static sBool tablesInitialized = sFALSE;
  static sInt qdescale[8][16];
  static sInt qrescale[8][16];

  // ---- macroblock AC scanning pattern

  static sInt macscan[15] = { 1,5,4,8,9,6,2,3,7,10,13,12,14,15,11 };
  static sInt zigzag[15]  = { 4,1,2,5,8,12,9,6,3,7,10,13,14,11,15 };
  static sInt zigzag2[16] = { 0,4,1,2,5,8,12,9,6,3,7,10,13,14,11,15 };

  // ---- transform row norms (2-norm)

  static sF32 xformn[4] = { 1.0000f, 1.3260f, 1.0000f, 1.5104f };
  // exact values: 1.3260 ^= sqrt(3601/2048), 1.5104 ^= sqrt(73/32)

  // ---- helper functions

  static sInt imuls(sInt a,sInt b,sInt s,sInt bias)
  {
    __int64 prod = ((__int64) a) * b;

    if(prod >= 0)
      prod -= bias;
    else
      prod += bias;

    prod += 1 << (s - 1);
    return sInt(prod >> s);
  }

  static sInt imul14(sInt a,sInt b)
  {
    __asm
    {
      mov   eax, [a];
      imul  [b];
      add   eax, 8192;
      adc   edx, 0;
      shrd  eax, edx, 14;

      mov   [a], eax;
    }

    return a;
  }

  static sInt descaleOld(sInt x,sInt bias,sInt factor,sInt shift)
  {
    return imuls(x,factor,shift+14,bias);
  }

  static sInt descale(sInt x,sInt bias,sInt factor,sInt shift)
  {
    sInt p = imul14(x,factor);
    sInt scale = 1 << shift;
    sInt realbias = (scale >> 1) - (bias >> 14);

    if(p >= 0)
      p += realbias;
    else
      p -= realbias - scale + 1;

    return p >> shift;
  }

  static sInt rescale(sInt x,sInt factor,sInt shift)
  {
    sInt p = x * factor;
    if(shift < 4)
      return p >> (4 - shift);
    else
      return p << (shift - 4);
  }

  static void initQuantTables()
  {
    if(tablesInitialized)
      return;

    for(sInt level=0;level<8;level++)
    {
      sF64 factor = pow(2.0,level / 8.0);
      
      for(sInt y=0;y<4;y++)
      {
        for(sInt x=0;x<4;x++)
        {
          sInt rescale = sInt(16 * factor * xformn[x] * xformn[y] + 0.5);
          sInt descale = (524288 + rescale) / (2 * rescale);

          qrescale[level][y*4+x] = rescale;
          qdescale[level][y*4+x] = descale;
        }
      }
    }

    tablesInitialized = sTRUE;
  }

  // ---- actual quantization functions

  sInt newQuantize(sInt qs,sInt *x,sInt npts,sInt cwidth)
  {
    sInt shift,*qtab,bias,i,f,n,*p;

    // prepare quantizer tables
    initQuantTables();

    shift = qs >> 3;
    qtab = qdescale[qs & 7];
    bias = 1024 << shift;

    // quantization by groups
    p = x;

    for(i=0;i<16;i++)
    {
      f = qtab[zigzag2[i]];

      for(n=0;n<cwidth;n++)
        *p++ = descale(*p,bias,f,shift);
    }

    // now find number of zeroes
    for(n=npts-1;n>=0;n--)
      if(x[n])
        break;

    return n+1;
  }

  static void rescaleLoop(sS16 *x,sInt count,sInt f,sInt shift)
  {
    if(shift < 4)
    {
      shift = 4 - shift;

      for(sInt i=0;i<count;i++)
        x[i] = (x[i] * f) >> shift;
    }
    else
    {
      shift -= 4;

      for(sInt i=0;i<count;i++)
        x[i] = (x[i] * f) << shift;
    }
  }

  static void rescaleLoopMMX(sS16 *x,sInt count,sInt f,sInt shift)
  {
    if(shift >= 4)
    {
      shift -= 4;

      __asm
      {
        mov       esi, [x];
        mov       ecx, [count];
        shr       ecx, 3;
        jz        rescale1tail;

        movd      mm6, [f];
        punpcklwd mm6, mm6;
        punpcklwd mm6, mm6;
        movd      mm7, [shift];

  rescale1lp:
        movq      mm0, [esi];
        movq      mm1, [esi+8];

        pmullw    mm0, mm6;
        pmullw    mm1, mm6;

        psllw     mm0, mm7;
        psllw     mm1, mm7;

        movq      [esi], mm0;
        movq      [esi+8], mm1;

        add       esi, 16;
        dec       ecx;
        jnz       rescale1lp;

  rescale1tail:
        mov       edx, [count];
        and       edx, 7;
        jz        rescale1end;
        mov       ebx, [f];
        mov       ecx, [shift];

  rescale1taillp:
        movsx     eax, word ptr [esi];
        imul      eax, ebx;
        shl       eax, cl;
        mov       word ptr [esi], ax;
        
        add       esi, 2;
        dec       edx;
        jnz       rescale1taillp;

  rescale1end:
        emms;
      }
    }
    else
    {
      shift = 4 - shift;

      __asm
      {
        mov       esi, [x];
        mov       ecx, [count];
        shr       ecx, 3;
        jz        rescale2tail;

        movd      mm6, [f];
        punpcklwd mm6, mm6;
        punpcklwd mm6, mm6;
        movd      mm7, [shift];

  rescale2lp:
        movq      mm0, [esi];
        movq      mm1, [esi+8];

        pmullw    mm0, mm6;
        pmullw    mm1, mm6;

        psraw     mm0, mm7;
        psraw     mm1, mm7;

        movq      [esi], mm0;
        movq      [esi+8], mm1;

        add       esi, 16;
        dec       ecx;
        jnz       rescale2lp;

  rescale2tail:
        mov       edx, [count];
        and       edx, 7;
        jz        rescale2end;
        mov       ebx, [f];
        mov       ecx, [shift];

  rescale2taillp:
        movsx     eax, word ptr [esi];
        imul      eax, ebx;
        shr       eax, cl;
        mov       word ptr [esi], ax;
        
        add       esi, 2;
        dec       edx;
        jnz       rescale2taillp;

  rescale2end:
        emms;
      }
    }
  }

  void newDequantize(sInt qs,sS16 *x,sInt npts,sInt cwidth)
  {
    sInt shift,*qtab,i,f,count;

    // prepare quantizer tables
    initQuantTables();

    shift = qs >> 3;
    qtab = qrescale[qs & 7];

    // dequantization by subbands
    for(i=0;i<16;i++)
    {
      f = qtab[zigzag2[i]];
      count = sMin(npts,cwidth);

      if(count)
      {
        rescaleLoopMMX(x,count,f,shift);
        x += count;
        npts -= count;
      }
    }
  }
}

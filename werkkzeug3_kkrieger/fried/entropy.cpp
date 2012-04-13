// This file is distributed under a BSD license. See LICENSE.txt for details.

// FRIED
// entropy coding routines.
// need cleanup

#include "_types.hpp"
#include "fried_internal.hpp"
#include "bitbuffer.hpp"

namespace FRIED
{
  // rlgr encoder
  static void GRadpkr(sInt p,sInt &krp)
  {
    if(p == 1)
      return;

    if(p == 0)
      krp = sMax(krp-2,0);
    else
      krp = sMin(krp + sMin(p,24),191);
  }

  static void GRcode(BitEncoder &coder,sInt &krp,sInt val)
  {
    sInt i,vk;

    sInt kr = krp >> 3;
    vk = val >> kr;

    if(vk < 32)
      coder.PutBits((1 << vk) - 1,vk);
    else
      for(i=0;i<vk;i++)
        coder.PutBits(1,1);

    coder.PutBits(0,1);

    if(kr)
      coder.PutBits(val & ((1 << kr) - 1),kr);

    GRadpkr(vk,krp);
  }

  static sInt GRdecode(BitDecoder &coder,sInt &krp)
  {
    sInt high = 0;

    while(coder.GetBits(1))
      high++;

    sInt kr = krp >> 3;
    sInt val = (high << kr) | coder.GetBits(kr);

    GRadpkr(high,krp);

    return val;
  }

  static sU16 GRlnv0[14],GRlnv1[28],GRlnv2[56],GRlnv3[112],GRlnv4[224],GRlnv5[448];
  static sU16 *GRlnv[] = { GRlnv0,GRlnv1,GRlnv2,GRlnv3,GRlnv4,GRlnv5 };
  static sInt GRmax[] = { 14,28,56,112,224,448 };
  static sU8 GRlow[48];
  static sU8 GRkrp[64*3];
  static sU8 GRktab[192];

  static void CalcGolombTable(sU16 *lnvTab,sInt k)
  {
    sInt kscale = 1 << k;
    sInt kmask = kscale - 1;
    sInt max = 14 << k;
    sInt val,len,kad;

    for(sInt i=0;i<max;i++)
    {
      sInt first4 = i >> k;

      if(first4 < 0x8) // 0xxx
      {
        val = (i >> 3) & kmask;
        len = k + 1;
        kad = 0;
      }
      else if(first4 < 0xc) // 10xx
      {
        val = ((i >> 2) & kmask) + kscale;
        len = k + 2;
        kad = 1;
      }
      else if(first4 < 0xe) // 110x
      {
        val = ((i >> 1) & kmask) + 2*kscale;
        len = k + 3;
        kad = 2;
      }

      lnvTab[i] = (val << 8) | (kad << 6) | len;
    }
  }

  static void CalcGolombTables()
  {
    for(sInt k=0;k<sizeof(GRlnv)/sizeof(*GRlnv);k++)
      CalcGolombTable(GRlnv[k],k);

    for(sInt i=0;i<48;i++)
    {
      GRlow[i] = (i >> 3) + 4; // sie würden es nicht glauben.
      GRkrp[i+  0] = sMax(i-2,0);
      GRkrp[i+ 64] = i;
      GRkrp[i+128] = i+2;
    }

    for(sInt i=0;i<192;i++)
      GRktab[i] = i >> 3; // und ich glaub das auch nicht.
  }

  static sInt GRdecodereal(BitDecoder &coder,sInt &krp)
  {
    // some unrolling for common cases here
    sInt lowBits = krp >> 3;
    sInt high;

    sInt peek = coder.PeekBits(3);
    if(peek < 0x4) // 0xx
    {
      krp = sMax(krp-2,0);
      return coder.GetBits(lowBits+1);
    }
    else if(peek < 0x6) // 10x
    {
      coder.SkipBitsNoCheck(2);
      return coder.GetBits(lowBits) + (1 << lowBits);
    }
    else if(peek < 0x7) // 110
    {
      coder.SkipBitsNoCheck(3);
      krp = sMin(krp+2,191);
      return coder.GetBits(lowBits) + (2 << lowBits);
    }
    else // 111
    {
      // we assume here we won't have lowBits>17
      coder.SkipBitsNoCheck(3);
      high = 3;

      // somewhat unrolled bit-counting loop
      sInt bits;
      do
      {
        bits = coder.PeekBits(4);
        switch(bits)
        {
        case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
          coder.SkipBitsNoCheck(1);
          break;

        case 8: case 9: case 10: case 11:
          high++;
          coder.SkipBitsNoCheck(2);
          break;

        case 12: case 13:
          high += 2;
          coder.SkipBitsNoCheck(3);
          break;

        case 14:
          high += 3;
          coder.SkipBitsNoCheck(4);
          break;

        default:
          high += 4;
          coder.SkipBits(4);
          break;
        }
      }
      while(bits == 15);

      krp = sMin(krp+sMin(high,24),191);
      return (high << lowBits) | coder.GetBits(lowBits);
    }
  }

  static sInt __forceinline GRdecodenew(BitDecoder &coder,sInt &krp)
  {
    // table-based version for small k (<6), rather fast
    if(krp < 48)
    {
      sInt lowBits = GRlow[krp];
      sInt peek = coder.PeekBits(lowBits);

      if(peek < GRmax[lowBits-4])
      {
        sInt lnv = GRlnv[lowBits-4][peek];
        krp = GRkrp[krp + (lnv & 0xc0)];
        coder.SkipBits(lnv & 0xf);

        return lnv >> 8;
      }
    }

    return GRdecodereal(coder,krp);
  }

  sInt rlgrenc(sU8 *bits,sInt nbmax,sInt *x,sInt n,sInt xminit)
  {
    BitEncoder coder;
    sInt kinit,krinit;
    sInt sign,xm;

    coder.Init(bits,nbmax);

    xminit++;
    if(xminit <= 2)
    {
      kinit = 1;
      krinit = 2;
    }
    else
    {
      kinit = 0;
      krinit = 0;

      while(xminit > 1)
      {
        xminit >>= 1;
        krinit++;
      }
    }

    sInt run = 0;
    sInt kp = kinit << 3;
    sInt krp = krinit << 3;

    for(sInt i=0;i<n;i++)
    {
      if(x[i] >= 0)
      {
        xm = x[i];
        sign = 0;
      }
      else
      {
        xm = -x[i];
        sign = 1;
      }

      sInt k = kp >> 3;

      if(k)
      {
        sInt runmax = 1 << k;

        if(!xm)
        {
          if(++run == runmax)
          {
            coder.PutBits(0,1);
            run = 0;
            kp = sMin(kp+4,191);
          }
        }
        else
        {
          coder.PutBits(1,1);
          coder.PutBits(run,k);
          coder.PutBits(sign,1);
          GRcode(coder,krp,xm-1);

          kp -= 5;
          run = 0;
        }
      }
      else
      {
        GRcode(coder,krp,xm*2 - sign);

        if(!xm)
          kp += 3;
        else
          kp = 0;
      }
    }

    if(run > 0)
      coder.PutBits(0,1);

    coder.Flush();
    return coder.BytesWritten();
  }

  sInt rlgrdec(const sU8 *bits,sInt nbmax,sS16 *y,sInt n,sInt xminit)
  {
    sInt kinit,krinit;
    sInt u,sign,xm,run;
    BitDecoder coder;
    static sBool tables = sFALSE;

    if(!tables)
    {
      CalcGolombTables();
      tables = sTRUE;
    }

    coder.Init(bits,nbmax);

    xminit++;
    if(xminit <= 2)
    {
      kinit = 1;
      krinit = 2;
    }
    else
    {
      kinit = 0;
      krinit = 0;

      while(xminit > 1)
      {
        xminit >>= 1;
        krinit++;
      }
    }

    sInt kp = kinit << 3;
    sInt krp = krinit << 3;
    sS16 *yp = y,*yend = y + n;
    sS16 *ystart = y;

    while(yp < yend)
    {
      if(kp < 8)
      {
        // these occur in runs, so save us some unnecessary tests...
        do
        {
          u = GRdecodenew(coder,krp);

          if(u)
          {
            kp = 0;
            *yp++ = (u >> 1) ^ -(u & 1); // negate if u odd
          }
          else
          {
            yp++;
            kp += 3;
            if(kp >= 8)
              break;
          }
        }
        while(yp < yend);
      }
      else
      {
        sInt k = GRktab[kp];

        if(!coder.GetBits(1)) // long zero run
        {
          yp += 1 << k;
          kp += 4; // no overflow will happen with <2^23 zeroes in a row
        }
        else // normal run
        {
          sInt oldkrp = krp;

          run = coder.GetBits(k);
          sign = coder.GetBitsNoCheck(1);
          xm = GRdecodenew(coder,krp) + 1;
          yp += run;

          if(yp < yend)
          {
            *yp++ = (xm ^ -sign) + sign; // negate if sign=1
            kp -= 5; // no underflow check since kp>=8
          }
        }
      }
    }

    return coder.BytesRead();
  }
}

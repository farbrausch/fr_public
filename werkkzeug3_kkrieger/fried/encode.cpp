// This file is distributed under a BSD license. See LICENSE.txt for details.

// FRIED
// encoding functions.

#include "_types.hpp"
#include "fried.hpp"
#include "fried_internal.hpp"

namespace FRIED
{
  static void read_bitmap_row(EncodeContext &ctx,sInt row,sInt *srp)
  {
    if(row < 0)
      row = 0;
    else if(row >= ctx.FH.YRes)
      row = ctx.FH.YRes - 1;

    sInt cols = ctx.FH.XRes;
    sInt colsPad = ctx.XResPadded;
    const sU8 *src = ctx.Image;

    if(ctx.Flags & FRIED_GRAYSCALE)
    {
      src += row * 2 * cols;

      if(ctx.Flags & FRIED_SAVEALPHA)
        gray_alpha_convert_dir(cols,colsPad,src,srp);
      else
        gray_x_convert_dir(cols,colsPad,src,srp);
    }
    else
    {
      src += row * 4 * cols;

      if(ctx.Flags & FRIED_SAVEALPHA)
        color_alpha_convert_dir(cols,colsPad,src,srp);
      else
        color_x_convert_dir(cols,colsPad,src,srp);
    }
  }

  static void reorder(sInt *dest_chunk,sInt *src_chunk,sInt cwidth)
  {
	  static const sU8 psd[16] = {
		  0x00,0x04,0x44,0x40,
		  0x80,0xc0,0xc4,0x84,
		  0x88,0xc8,0xcc,0x8c,
		  0x4c,0x48,0x08,0x0c
	  };

    static const sU8 mfd[15] = {
          0x40,0x04,0x08,
      0x44,0x80,0xc0,0x84,
      0x48,0x0c,0x4c,0x88,
      0xc4,0xc8,0x8c,0xcc
    };

    sInt nmb = cwidth / 16;
    sInt *g0,*g1,*g2;
    sInt *qbp;
    sInt n,k;

    g0 = dest_chunk;
    qbp = src_chunk;

    // macroblock DC coeffs
    for(sInt mb=0;mb<cwidth;mb+=16)
      *g0++ = qbp[mb];

    g1 = g0;
    g0 = dest_chunk;
    g2 = g0 + cwidth;

    for(sInt mb=0;mb<cwidth;mb+=16)
    {
      // macroblock AC coeffs
      qbp = src_chunk + mb;
      for(n=0,k=0;n<15;n++,k+=nmb)
        g1[k] = qbp[(mfd[n] >> 4) * cwidth + (mfd[n] & 0xf)];

      g1++;

      // block AC coeffs
      for(n=0;n<16;n++)
      {
        qbp = src_chunk + mb + (psd[n] >> 4) * cwidth + (psd[n] & 0xf);

        g2[ 1*cwidth] = qbp[1];
        g2[ 2*cwidth] = qbp[2];
        g2[ 8*cwidth] = qbp[3];
        qbp += cwidth;

        g2[ 0*cwidth] = qbp[0];
        g2[ 3*cwidth] = qbp[1];
        g2[ 7*cwidth] = qbp[2];
        g2[ 9*cwidth] = qbp[3];
        qbp += cwidth;

        g2[ 4*cwidth] = qbp[0];
        g2[ 6*cwidth] = qbp[1];
        g2[10*cwidth] = qbp[2];
        g2[13*cwidth] = qbp[3];
        qbp += cwidth;

        g2[ 5*cwidth] = qbp[0];
        g2[11*cwidth] = qbp[1];
        g2[12*cwidth] = qbp[2];
        g2[14*cwidth] = qbp[3];
        g2++;
      }
    }
  }

  static sInt encodeStripe(EncodeContext &ctx,sInt cols,sInt chans,sU8 *bytes,sInt maxbytes,sInt **srp)
  {
    sInt cjs[16];
    sInt cwidth = ctx.FH.ChunkWidth;
    sInt nchunks = (cols + cwidth - 1) / cwidth;
    sInt *g0,n;
    sU8 *byteStart = bytes;
    sU8 *byteEnd = bytes + maxbytes;

    // clear chunk positions
    for(sInt ch=0;ch<ctx.FH.Channels;ch++)
      cjs[ch] = 0;

    // process this stripe chunk by chunk
    for(sInt chunk=0,ncc=0;chunk<nchunks;chunk++,ncc+=cwidth)
    {
      // adjust width for last chunk
      if(chunk == nchunks-1)
        cwidth = cols - ncc;

      // leave space for chunk length field
      sU8 *chunkSizePtr = (sU8 *) bytes;
      bytes += 2;

      // process channels
      for(sInt ch=0;ch<ctx.FH.Channels;ch++)
      {
        sInt so = ctx.Chans[ch].StripeOffset;
        sInt co = ctx.Chans[ch].ChunkOffset;
        sInt qs = ctx.Chans[ch].Quantizer;
        sInt cksize = cwidth * 16;

        // copy data over from stripe buffer to quantization buffer
        for(n=0;n<16;n++)
          sCopyMem(&ctx.QB[n*cwidth],srp[n] + so + cjs[ch],cwidth * sizeof(sInt));

        // reorder and quantize
        reorder(ctx.CK + co,ctx.QB,cwidth);

        g0 = ctx.CK + co;
        sInt encsize = newQuantize(qs,g0,cksize,cwidth);

        // delta encode dc coefficients
        n = sMin(encsize,cwidth/16);

        while(--n > 0)
        {
          sS16 newVal = g0[n] - g0[n-1];
          g0[n] = newVal;
        }

        /*while(--n > 0)
          g0[n] -= g0[n-1];*/

        // write number of encoded coeffs
        encsize = (encsize + 7) & ~7;

        if(encsize < 127 * 8)
          *bytes++ = encsize >> 2;
        else
        {
          sInt temp = (encsize >> 2) + 1;
          *bytes++ = temp & 0xff;
          *bytes++ = temp >> 8;
        }

        // encode the coeffs themselves
        if(encsize)
        {
          sInt xminit,nbs;

          xminit = 625 >> (qs >> 3);
          nbs = rlgrenc(bytes,byteEnd - bytes,g0,sMin(encsize,cwidth),xminit);
          if(nbs < 0)
            return -1;
          else
            bytes += nbs;

          if(encsize > cwidth)
          {
            xminit = 94 >> (qs >> 3);
            nbs = rlgrenc(bytes,byteEnd - bytes,g0+cwidth,encsize-cwidth,xminit);
            if(nbs < 0)
              return -1;
            else
              bytes += nbs;
          }
        }

        // this channel is done
        cjs[ch] += cwidth;
      }

      // write the chunk size
      sInt chunkSize = bytes - chunkSizePtr;
      sVERIFY(chunkSize < 65536);

      chunkSizePtr[0] = chunkSize & 0xff;
      chunkSizePtr[1] = chunkSize >> 8;
    }

    return bytes - byteStart;
  }

  static void hlbt_group1(sInt swidth,sInt so,sInt ib,sInt **srp,sBool ftop)
  {
    sInt *pa,*pb,*p0,*p1,*p2,*p3;
    sInt col;

    // normal rows only
    pa = srp[ib-5] + so;
    pb = srp[ib-4] + so;
    p0 = srp[ib-3] + so;
    p1 = srp[ib-2] + so;
    p2 = srp[ib-1] + so;
    p3 = srp[ib-0] + so;
    lbt4pre4x2(p0,p1,p2,p3);

    for(col=0;col<swidth-4;col+=4)
    {
      if(ftop)
        lbt4pre2x4(pa+col+2,pb+col+2);

      lbt4pre4x4(p0+col+2,p1+col+2,p2+col+2,p3+col+2);
      ndct42D(pa+col,pb+col,p0+col,p1+col);
    }

    lbt4pre4x2(p0+col+2,p1+col+2,p2+col+2,p3+col+2);
    ndct42D(pa+col,pb+col,p0+col,p1+col);
  }

  static void hlbt_group2(sInt swidth,sInt so,sInt **srp,sBool ftop)
  {
    // macroblocks only
    sInt *pa,*pb,*p0,*p1;
    sInt col;

    pa = srp[ 0] + so;
    pb = srp[ 4] + so;
    p0 = srp[ 8] + so;
    p1 = srp[12] + so;

    for(col=0;col<swidth;col+=16)
      ndct42D_MB(pa+col,pb+col,p0+col,p1+col);
  }

  static void hlbt_group3(sInt swidth,sInt so,sInt ib,sInt **srp)
  {
    sInt *pa,*pb,*p0,*p1;
    sInt col;

    // last row
    pa = srp[ib-3] + so;
    pb = srp[ib-2] + so;
    p0 = srp[ib-1] + so;
    p1 = srp[ib-0] + so;

    for(col=0;col<swidth-4;col+=4)
    {
      lbt4pre2x4(p0+col+2,p1+col+2);
      ndct42D(pa+col,pb+col,p0+col,p1+col);
    }

    ndct42D(pa+col,pb+col,p0+col,p1+col);

    // last row of macroblocks
    pa = srp[ib-15] + so;
    pb = srp[ib-11] + so;
    p0 = srp[ib- 7] + so;
    p1 = srp[ib- 3] + so;

    for(col=0;col<swidth;col+=16)
      ndct42D_MB(pa+col,pb+col,p0+col,p1+col);
  }

  static sInt updatebp(sInt **srp,sInt *sb,sInt fr,sInt width,sInt mode)
  {	
    fr = mode ? 0 : (fr ^ 16);
    for(sInt i=0;i<32;i++)
      srp[i] = sb + ((fr + i) & 31) * width;

	  return fr;
  }

  static sInt PerformEncode(EncodeContext &ctx)
  {
    sInt *srp[32];
    sInt fr,ib,k;
    sU8 *bits,*bitsEnd;
    sInt cols,rows,chans;
    sInt stsize;
    
    cols = ctx.XResPadded;
    rows = ctx.YResPadded;
    chans = ctx.FH.Channels;
    stsize = chans * cols;

    bits = ctx.Bits;
    bitsEnd = ctx.Bits + ctx.BitsLength;

    // copy over frame and channel headers
    memcpy(bits,&ctx.FH,sizeof(FileHeader));
    bits += sizeof(FileHeader);

    for(sInt ch=0;ch<chans;ch++)
    {
      memcpy(bits,&ctx.Chans[ch],sizeof(ChannelHeader));
      bits += sizeof(ChannelHeader);
    }

    // actual encoding loop
    fr = updatebp(srp,ctx.SB,0,stsize,1);
    ib = 0;
    k = -1;

    for(sInt row=0;row<rows;row++,k++,ib++)
    {
      read_bitmap_row(ctx,row,srp[ib]);

      if(k == 4)
      {
        sBool top = row == 5;
        for(sInt ch=0;ch<chans;ch++)
          hlbt_group1(cols,ctx.Chans[ch].StripeOffset,ib,srp,top);

        k = 0;
      }

      if(ib == 31)
      {
        sBool top = row == 31;
        for(sInt ch=0;ch<chans;ch++)
          hlbt_group2(cols,ctx.Chans[ch].StripeOffset,srp,top);

        sInt sizeStripe = encodeStripe(ctx,cols,chans,bits,bitsEnd - bits,srp);
        if(sizeStripe < 0)
          return -1;

        bits += sizeStripe;
        fr = updatebp(srp,ctx.SB,fr,stsize,0);
        ib = 15;
      }

      if(row == rows - 1)
      {
        for(sInt ch=0;ch<chans;ch++)
          hlbt_group3(cols,ctx.Chans[ch].StripeOffset,ib,srp);

        sInt sizeStripe = encodeStripe(ctx,cols,chans,bits,bitsEnd - bits,srp);
        if(sizeStripe < 0)
          return -1;

        bits += sizeStripe;
      }
    }

    return bits - ctx.Bits;
  }
}

using namespace FRIED;

static void PrepareChannel(EncodeContext &ctx,sInt num,sInt type,sInt quantize)
{
  ctx.Chans[num].Type = type;
  ctx.Chans[num].Quantizer = quantize;
  ctx.Chans[num].StripeOffset = num * ctx.XResPadded;
  ctx.Chans[num].ChunkOffset = num * ctx.FH.ChunkWidth * 16;
}

sU8 *SaveFRIED(const sU8 *image,sInt xsize,sInt ysize,sInt flags,sInt quality,sInt &outsize)
{
  EncodeContext ctx;

  // fill out file header
  sCopyMem(ctx.FH.Signature,"FRIED001",8);
  ctx.FH.XRes = xsize;
  ctx.FH.YRes = ysize;

  // calculate number of channels to use
  ctx.FH.Channels = (flags & FRIED_GRAYSCALE) ? 1 : 3;
  if(flags & FRIED_SAVEALPHA)
    ctx.FH.Channels++;

  // calculate virtual x resolution
  ctx.XResPadded = (xsize + 31) & ~31;
  ctx.YResPadded = (ysize + 31) & ~31;
  ctx.FH.ChunkWidth = sMin(ctx.XResPadded,512);
  ctx.FH.VirtualXRes = ctx.XResPadded * ctx.FH.Channels;

  // prepare encode context and buffers
  sInt sbw = ctx.FH.Channels * ctx.XResPadded;
  sInt cbw = ctx.FH.Channels * ctx.FH.ChunkWidth;

  ctx.SB = new sInt[sbw * 32];
  ctx.QB = new sInt[cbw * 16];
  ctx.CK = new sInt[cbw * 16];

  ctx.BitsLength = sizeof(FileHeader) +
    ctx.FH.Channels * sizeof(ChannelHeader) +
    ctx.XResPadded * ctx.YResPadded * ctx.FH.Channels * 3 +
    1048576;
  ctx.Bits = new sU8[ctx.BitsLength];

  // prepare channel setup
  sInt chanNum = 0;

  if(flags & FRIED_GRAYSCALE)
    PrepareChannel(ctx,chanNum++,CHANNEL_Y,quality);
  else
  {
    PrepareChannel(ctx,chanNum++,CHANNEL_Y,quality);
    PrepareChannel(ctx,chanNum++,CHANNEL_CO,quality);
    PrepareChannel(ctx,chanNum++,CHANNEL_CG,quality);
  }

  if(flags & FRIED_SAVEALPHA)
    PrepareChannel(ctx,chanNum++,CHANNEL_ALPHA,quality);

  sVERIFY(chanNum == ctx.FH.Channels);

  // image setup
  ctx.Image = image;
  ctx.Flags = flags;

  // perform actual encoding
  outsize = PerformEncode(ctx);

  // free everything
  delete[] ctx.SB;
  delete[] ctx.QB;
  delete[] ctx.CK;

  if(outsize < 0)
  {
    delete[] ctx.Bits;
    ctx.Bits = 0;
  }

  // return the packed data
  return ctx.Bits;
}

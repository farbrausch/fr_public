// This file is distributed under a BSD license. See LICENSE.txt for details.

// FRIED
// decoding functions.

#include "_types.hpp"
#include "fried.hpp"
#include "fried_internal.hpp"

#include <crtdbg.h>

namespace FRIED
{
  static void writeBitmapRow(DecodeContext &ctx,sInt row,sS16 *srp)
  {
    if(row < 0 || row >= ctx.FH.YRes)
      return;

    sInt cols = ctx.FH.XRes;
    sInt colsPad = ctx.XResPadded;
    sU8 *dst = ctx.Image;

    if(ctx.ChannelSetup < 2) // grayscale
    {
      dst += row * 2 * cols;

      if(ctx.ChannelSetup == 0)
        gray_x_convert_inv(cols,colsPad,srp,dst);
      else
        gray_alpha_convert_inv(cols,colsPad,srp,dst);
    }
    else
    {
      dst += row * 4 * cols;

      if(ctx.ChannelSetup == 2)
        color_x_convert_inv(cols,colsPad,srp,dst);
      else if(ctx.ChannelSetup == 3)
        color_alpha_convert_inv(cols,colsPad,srp,dst);
    }

    // TODO: write the corresponding row using the correct innerloop
  }

  #define MM_LOAD4(reg0,reg1,reg2,reg3,offset) \
    __asm mov       eax, [c0] \
    __asm mov       ebx, [c1] \
    __asm mov       ecx, [c2] \
    __asm mov       edx, [c3] \
    __asm movq      reg0, [eax+offset] \
    __asm movq      reg1, [ebx+offset] \
    __asm movq      reg2, [ecx+offset] \
    __asm movq      reg3, [edx+offset]

  #define MM_STORE4(reg0,reg1,reg2,reg3,rc0,rc1,rc2,rc3) \
    __asm mov       eax, [esi+((rc0>>4)*4)] \
    __asm mov       ebx, [esi+((rc1>>4)*4)] \
    __asm mov       ecx, [esi+((rc2>>4)*4)] \
    __asm mov       edx, [esi+((rc3>>4)*4)] \
    __asm movq      [eax+edi+((rc0&0xf)*2)], reg0 \
    __asm movq      [ebx+edi+((rc1&0xf)*2)], reg1 \
    __asm movq      [ecx+edi+((rc2&0xf)*2)], reg2 \
    __asm movq      [edx+edi+((rc3&0xf)*2)], reg3

  #define MM_TRANSPOSE(reg0,reg1,reg2,reg3,reg4,reg5) \
    __asm movq      reg4, reg0 \
    __asm movq      reg5, reg2 \
    __asm punpcklwd reg0, reg1 \
    __asm punpckhwd reg4, reg1 \
    __asm punpcklwd reg2, reg3 \
    __asm punpckhwd reg5, reg3 \
    __asm movq      reg1, reg0 \
    __asm movq      reg3, reg4 \
    __asm punpckldq reg0, reg2 \
    __asm punpckhdq reg1, reg2 \
    __asm punpckldq reg4, reg5 \
    __asm punpckhdq reg3, reg5

  static __forceinline void shuffle4x16(sS16 **dest,sInt xOffs,sS16 *c0,sS16 *c1,sS16 *c2,sS16 *c3)
  {
    __asm
    {
      mov       esi, [dest];
      mov       edi, [xOffs];
      add       edi, edi;
    }

    MM_LOAD4(mm0,mm1,mm2,mm3,0);
    MM_TRANSPOSE(mm0,mm1,mm2,mm3,mm4,mm5);
    MM_LOAD4(mm2,mm5,mm6,mm7,8);
    MM_STORE4(mm0,mm1,mm4,mm3,0x00,0x04,0x44,0x40);
    MM_TRANSPOSE(mm2,mm5,mm6,mm7,mm0,mm1);
    MM_LOAD4(mm1,mm3,mm4,mm6,16);
    MM_STORE4(mm2,mm5,mm0,mm7,0x80,0xc0,0xc4,0x84);
    MM_TRANSPOSE(mm1,mm3,mm4,mm6,mm2,mm5);
    MM_LOAD4(mm0,mm4,mm5,mm7,24);
    MM_STORE4(mm1,mm3,mm2,mm6,0x88,0xc8,0xcc,0x8c);
    MM_TRANSPOSE(mm0,mm4,mm5,mm7,mm1,mm3);
    MM_STORE4(mm0,mm4,mm1,mm7,0x4c,0x48,0x08,0x0c);

    __asm emms;
  }

  static void inv_reorder(sS16 **dest,sInt xOffs,sS16 *src,sInt cwidth)
  {
    sInt nmb = cwidth/16;
    sS16 *g0,*g1,*g2,*g3;
    sInt mb;

    // first row of block AC coeffs+DC
    g0 = src;
    g1 = src +  2 * cwidth;
    g2 = src +  3 * cwidth;
    g3 = src +  9 * cwidth;
    for(mb=0;mb<cwidth;mb+=16)
    {
      sS16 dcs[16];
      sS16 *gp = g0;

      // get dc coeffs (with reordering)
      dcs[ 0] = *gp; gp += nmb;
      dcs[ 3] = *gp; gp += nmb;
      dcs[ 1] = *gp; gp += nmb;
      dcs[14] = *gp; gp += nmb;

      dcs[ 2] = *gp; gp += nmb;
      dcs[ 4] = *gp; gp += nmb;
      dcs[ 5] = *gp; gp += nmb;
      dcs[ 7] = *gp; gp += nmb;

      dcs[13] = *gp; gp += nmb;
      dcs[15] = *gp; gp += nmb;
      dcs[12] = *gp; gp += nmb;
      dcs[ 8] = *gp; gp += nmb;

      dcs[ 6] = *gp; gp += nmb;
      dcs[ 9] = *gp; gp += nmb;
      dcs[11] = *gp; gp += nmb;
      dcs[10] = *gp; g0++;

      // shuffle
      shuffle4x16(dest+0,xOffs+mb,dcs,g1+mb,g2+mb,g3+mb);
    }

    // second rows of block AC coeffs
    g0 = src +  1 * cwidth;
    g1 = src +  4 * cwidth;
    g2 = src +  8 * cwidth;
    g3 = src + 10 * cwidth;
    for(mb=0;mb<cwidth;mb+=16)
      shuffle4x16(dest+1,xOffs+mb,g0+mb,g1+mb,g2+mb,g3+mb);

    // third rows of block AC coeffs
    g0 = src +  5 * cwidth;
    g1 = src +  7 * cwidth;
    g2 = src + 11 * cwidth;
    g3 = src + 14 * cwidth;
    for(mb=0;mb<cwidth;mb+=16)
      shuffle4x16(dest+2,xOffs+mb,g0+mb,g1+mb,g2+mb,g3+mb);

    // fourth rows of block AC coeffs
    g0 = src +  6 * cwidth;
    g1 = src + 12 * cwidth;
    g2 = src + 13 * cwidth;
    g3 = src + 15 * cwidth;
    for(mb=0;mb<cwidth;mb+=16)
      shuffle4x16(dest+3,xOffs+mb,g0+mb,g1+mb,g2+mb,g3+mb);
  }

  static sInt decodeStripe(DecodeContext &ctx,sInt cols,sInt chans,const sU8 *byteStart,sInt maxbytes,sS16 **srp)
  {
    sInt cjs[16];
    sInt cwidth = ctx.FH.ChunkWidth;
    sInt nchunks = (cols + cwidth - 1) / cwidth;
    sS16 *g0;
    const sU8 *bytes,*bytesEnd;

    bytes = byteStart;
    bytesEnd = bytes + maxbytes;

    // clear chunk positions
    for(sInt ch=0;ch<ctx.FH.Channels;ch++)
      cjs[ch] = 0;

    // process this stripe chunk by chunk
    for(sInt chunk=0,ncc=0;chunk<nchunks;chunk++,ncc+=cwidth)
    {
      // adjust width for last chunk
      if(chunk == nchunks-1)
        cwidth = cols - ncc;

      // read chunk length
      if(bytes + 2 > bytesEnd)
        return -1;

      const sU8 *bytesChunkEnd = bytes + (bytes[0] + (bytes[1] << 8));
      bytes += 2;

      // process channels
      for(sInt ch=0;ch<ctx.FH.Channels;ch++)
      {
        sInt so = ctx.Chans[ch].StripeOffset;
        sInt co = ctx.Chans[ch].ChunkOffset;
        sInt qs = ctx.Chans[ch].Quantizer;
        sInt cksize = cwidth * 16;

        // read number of encoded coeffs
        sInt encsize;

        if(bytes >= bytesEnd)
          return -1;

        if(*bytes & 1) // long code
        {
          if(bytes + 1 >= bytesEnd)
            return -1;

          encsize = ((bytes[0] + (bytes[1] << 8)) & ~1) * 4;
          bytes += 2;
        }
        else // short code
          encsize = *bytes++ * 4;

        // decode coefficients
        g0 = ctx.CK + co;
        sSetMem(g0,0,cksize * sizeof(sS16));

        if(encsize)
        {
          sInt xminit,nbs;

          xminit = 625 >> (qs >> 3);
          nbs = rlgrdec(bytes,bytesEnd - bytes,g0,sMin(encsize,cwidth),xminit);
          if(nbs < 0)
            return -1;
          else
            bytes += nbs;

          if(encsize > cwidth)
          {
            xminit = 94 >> (qs >> 3);
            nbs = rlgrdec(bytes,bytesEnd - bytes,g0+cwidth,encsize-cwidth,xminit);
            if(nbs < 0)
              return -1;
            else
              bytes += nbs;
          }
        }

        // un-delta dc coefficients
        sInt nmb = sMin(encsize,cwidth/16);

        sInt n = 0;
        while(++n < nmb)
          g0[n] += g0[n-1];

        // dequantize, undo reordering
        newDequantize(qs,g0,encsize,cwidth);
        inv_reorder(srp+16,so + cjs[ch],g0,cwidth);

        // this channel is done
        cjs[ch] += cwidth;
      }

      if(bytes != bytesChunkEnd)
        return -1;
    }

    return bytes - byteStart;
  }

  static void ihlbt_group1(sInt swidth,sInt so,sS16 **srp)
  {
    sS16 *p0,*p1,*p2,*p3;
    sInt col;
    
    // first row of macroblocks
    p0 = srp[16] + so;
    p1 = srp[20] + so;
    p2 = srp[24] + so;
    p3 = srp[28] + so;

    for(col=0;col<swidth;col+=16)
      indct42D_MB(p0+col,p1+col,p2+col,p3+col);

    // first row
    p0 = srp[16] + so;
    p1 = srp[17] + so;
    p2 = srp[18] + so;
    p3 = srp[19] + so;
    indct42D(p0,p1,p2,p3);

    // rescale top left 2x2 pixels
    p0[0] <<= 2;
    p0[1] <<= 2;
    p1[0] <<= 2;
    p1[1] <<= 2;

    for(sInt col=4;col<swidth;col+=4)
    {
      indct42D(p0+col,p1+col,p2+col,p3+col);
      lbt4post2x4(p0+col-2,p1+col-2);
    }

    // rescale top right 2x2 pixels
    p0[swidth-2] <<= 2;
    p0[swidth-1] <<= 2;
    p1[swidth-2] <<= 2;
    p1[swidth-1] <<= 2;
  }

  static void ihlbt_group2(sInt swidth,sInt so,sS16 **srp,sBool fbot)
  {
    // macroblocks only
    sS16 *pa,*pb,*p0,*p1,*p2,*p3;
    sInt col;

    pa = srp[ 8] + so;
    pb = srp[12] + so;
    p0 = srp[16] + so;
    p1 = srp[20] + so;
    p2 = srp[24] + so;
    p3 = srp[28] + so;

    for(col=0;col<swidth;col+=16)
      indct42D_MB(p0+col,p1+col,p2+col,p3+col);
  }

  static void ihlbt_group3(sInt swidth,sInt so,sInt ib,sS16 **srp,sBool fbot)
  {
    // normal rows only
    sS16 *pa,*pb,*p0,*p1,*p2,*p3;
    sInt col;

    pa = srp[ib+0] + so;
    pb = srp[ib+1] + so;
    p0 = srp[ib+2] + so;
    p1 = srp[ib+3] + so;
    p2 = srp[ib+4] + so;
    p3 = srp[ib+5] + so;

    indct42D(p0,p1,p2,p3);
    lbt4post4x2(pa,pb,p0,p1);

    if(fbot) // rescale bottom left 2x2 pixels
    {
      p2[0] <<= 2;
      p2[1] <<= 2;
      p3[0] <<= 2;
      p3[1] <<= 2;
    }

    for(col=4;col<swidth;col+=4)
    {
      indct42D(p0+col,p1+col,p2+col,p3+col);
      lbt4post4x4(pa+col-2,pb+col-2,p0+col-2,p1+col-2);

      if(fbot)
        lbt4post2x4(p2+col-2,p3+col-2);
    }

    lbt4post4x2(pa+col-2,pb+col-2,p0+col-2,p1+col-2);

    // rescale bottom right 2x2 pixels
    if(fbot)
    {
      p2[swidth-2] <<= 2;
      p2[swidth-1] <<= 2;
      p3[swidth-2] <<= 2;
      p3[swidth-1] <<= 2;
    }
  }

  static sInt updatebp(sS16 **srp,sS16 *sb,sInt fr,sInt width,sInt mode)
  {
    fr = mode ? 0 : (fr ^ 16);
    for(sInt i=0;i<32;i++)
      srp[i] = sb + ((fr + i) & 31) * width;

    return fr;
  }

  static sInt PerformDecode(DecodeContext &ctx,const sU8 *bitsStart,sInt nbytes)
  {
    sS16 *srp[32];
    sInt fr,ib,k;
    const sU8 *bits,*bitsEnd;
    sInt cols,rows,chans;
    sInt stsize;

    cols = ctx.XResPadded;
    rows = ctx.YResPadded;
    chans = ctx.FH.Channels;
    stsize = chans * cols;

    bits = bitsStart;
    bitsEnd = bits + nbytes;

    // actual decoding loop
    fr = updatebp(srp,ctx.SB,0,stsize,1);
    ib = 16;
    k = 2;

    for(sInt row=0;row<rows;row++,k++,ib++)
    {
      if(row == 0)
      {
        sInt sizeStripe = decodeStripe(ctx,cols,chans,bits,bitsEnd - bits,srp);
        if(sizeStripe < 0)
          return -1;

        bits += sizeStripe;
        
        for(sInt ch=0;ch<chans;ch++)
          ihlbt_group1(cols,ctx.Chans[ch].StripeOffset,srp);
      }

      if(ib == 16)
      {
        fr = updatebp(srp,ctx.SB,fr,stsize,0);
        ib = 0;

        if(row != rows - 16)
        {
          sBool bot = (row == rows - 32);
          sInt sizeStripe = decodeStripe(ctx,cols,chans,bits,bitsEnd - bits,srp);
          if(sizeStripe < 0)
            return -1;

          bits += sizeStripe;

          for(sInt ch=0;ch<chans;ch++)
            ihlbt_group2(cols,ctx.Chans[ch].StripeOffset,srp,bot);
        }
      }

      if(k == 4 && row != rows - 2)
      {
        sBool bot = (row == rows - 6);

        for(sInt ch=0;ch<chans;ch++)
          ihlbt_group3(cols,ctx.Chans[ch].StripeOffset,ib,srp,bot);

        k = 0;
      }

      writeBitmapRow(ctx,row,srp[ib]);
    }

    return bitsEnd - bits;
  }
}

using namespace FRIED;

sBool LoadFRIED(const sU8 *data,sInt size,sInt &xout,sInt &yout,sU8 *&dataout)
{
  DecodeContext ctx;
  const sU8 *dataEnd = data + size;

  // check file format
  if(size < sizeof(FileHeader))
    return sFALSE;

  // check signature
  if(sCmpMem(data,"FRIED001",8))
    return sFALSE;

  // copy header over
  sCopyMem(&ctx.FH,data,sizeof(FileHeader));
  data += sizeof(FileHeader);

  // check number of channels and copy channel headers over
  if(ctx.FH.Channels > 16)
    return sFALSE;

  sCopyMem(ctx.Chans,data,ctx.FH.Channels * sizeof(ChannelHeader));
  data += ctx.FH.Channels * sizeof(ChannelHeader);

  // calculate some important constants
  sInt chans = ctx.FH.Channels;

  ctx.XResPadded = (ctx.FH.XRes + 31) & ~31;
  ctx.YResPadded = (ctx.FH.YRes + 31) & ~31;
  sInt sbw = chans * ctx.XResPadded;
  sInt cbw = chans * ctx.FH.ChunkWidth;

  ctx.SB = new sS16[sbw * 32];
  ctx.QB = new sInt[cbw * 16];
  ctx.CK = new sS16[cbw * 16];

  // determine channel setup (rather faked at the moment)
  if(chans <= 1 || ctx.Chans[0].Type != CHANNEL_Y)
    return sFALSE;

  if(chans == 1)
    ctx.ChannelSetup = 0; // gray w/out alpha
  else if(chans == 2 && ctx.Chans[1].Type == CHANNEL_ALPHA)
    ctx.ChannelSetup = 1; // gray w/ alpha
  else if(chans >= 3 && ctx.Chans[1].Type == CHANNEL_CO && ctx.Chans[2].Type == CHANNEL_CG)
  {
    if(chans == 3)
      ctx.ChannelSetup = 2; // color w/out alpha
    else if(chans == 4 && ctx.Chans[3].Type == CHANNEL_ALPHA)
      ctx.ChannelSetup = 3; // color w/ alpha
    else
      return sFALSE;
  }
  else
    return sFALSE;

  // allocate image
  ctx.Image = new sU8[ctx.FH.XRes * ctx.FH.YRes * (ctx.ChannelSetup >= 2 ? 4 : 2)];

  // decode
  if(PerformDecode(ctx,data,dataEnd - data) >= 0)
  {
    xout = ctx.FH.XRes;
    yout = ctx.FH.YRes;
    dataout = ctx.Image;
  }
  else
  {
    xout = 0;
    yout = 0;
    dataout = 0;
    delete[] ctx.Image;
  }

  // free everything
  delete[] ctx.SB;
  delete[] ctx.QB;
  delete[] ctx.CK;

  return dataout != 0;
}

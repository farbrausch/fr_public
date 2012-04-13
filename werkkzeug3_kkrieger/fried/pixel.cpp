// This file is distributed under a BSD license. See LICENSE.txt for details.

// FRIED
// pixel processing innerloops.
// (color conversion and such)

#include "_types.hpp"
#include "fried.hpp"
#include "fried_internal.hpp"

namespace FRIED
{
  // forward conversions
  void gray_alpha_convert_dir(sInt cols,sInt colsPad,const sU8 *src,sInt *dst)
  {
    sInt *outY = dst;
    sInt *outA = dst + colsPad;

    for(sInt i=0;i<cols;i++)
    {
      *outY++ = (*src++ - 128) << 2;
      *outA++ = (*src++ - 128) << 2;
    }

    for(sInt i=cols;i<colsPad;i++)
    {
      *outY++ = 0;
      *outA++ = 0;
    }
  }

  void gray_x_convert_dir(sInt cols,sInt colsPad,const sU8 *src,sInt *dst)
  {
    sInt *outY = dst;

    for(sInt i=0;i<cols;i++)
    {
      *outY++ = (*src++ - 128) << 2;
      src++; // skip unused alpha byte
    }

    for(sInt i=cols;i<colsPad;i++)
      *outY++ = 0;
  }

  void color_alpha_convert_dir(sInt cols,sInt colsPad,const sU8 *src,sInt *dst)
  {
    sInt *outY = dst;
    sInt *outCo = dst + colsPad;
    sInt *outCg = outCo + colsPad;
    sInt *outA = outCg + colsPad;

    for(sInt i=0;i<cols;i++)
    {
      sInt b = (*src++ - 128) << 2;
      sInt g = (*src++ - 128) << 2;
      sInt r = (*src++ - 128) << 2;

      g <<= 1;
 
      *outY++ = (r + g + b + 2) >> 2;
      *outCo++ = (r - b) >> 1;
      *outCg++ = (g - r - b + 2) >> 2; 
      *outA++ = (*src++ - 128) << 2;
    }

    for(sInt i=cols;i<colsPad;i++)
    {
      *outY++ = 0;
      *outCo++ = 0;
      *outCg++ = 0;
      *outA++ = 0;
    }
  }

  void color_x_convert_dir(sInt cols,sInt colsPad,const sU8 *src,sInt *dst)
  {
    sInt *outY = dst;
    sInt *outCo = dst + colsPad;
    sInt *outCg = outCo + colsPad;

    for(sInt i=0;i<cols;i++)
    {
      sInt b = (*src++ - 128) << 2;
      sInt g = (*src++ - 128) << 2;
      sInt r = (*src++ - 128) << 2;
      src++; // ignore alpha

      g <<= 1;
 
      *outY++ = (r + g + b + 2) >> 2;
      *outCo++ = (r - b) >> 1;
      *outCg++ = (g - r - b + 2) >> 2; 
    }

    for(sInt i=cols;i<colsPad;i++)
    {
      *outY++ = 0;
      *outCo++ = 0;
      *outCg++ = 0;
    }
  }

  // inverse conversions (optimize me!)
  static int clampPixel(sInt a)
  {
    return (a < 0) ? 0 : (a > 255) ? 255 : a;
  }

  void gray_alpha_convert_inv(sInt cols,sInt colsPad,const sS16 *src,sU8 *dst)
  {
    const sS16 *inY = src;
    const sS16 *inA = src + colsPad;

    for(sInt i=0;i<cols;i++)
    {
      *dst++ = clampPixel((*inY++ >> 4) + 128);
      *dst++ = clampPixel((*inA++ >> 4) + 128);
    }
  }

  void gray_x_convert_inv(sInt cols,sInt colsPad,const sS16 *src,sU8 *dst)
  {
    const sS16 *inY = src;

    for(sInt i=0;i<cols;i++)
    {
      *dst++ = clampPixel((*inY++ >> 4) + 128);
      *dst++ = 255;
    }
  }

  void color_alpha_convert_inv(sInt cols,sInt colsPad,const sS16 *src,sU8 *dst)
  {
    const sS16 *inY = src;
    const sS16 *inCo = src + colsPad;
    const sS16 *inCg = inCo + colsPad;
    const sS16 *inA = inCg + colsPad;

    for(sInt i=0;i<cols;i++)
    {
      sInt y = *inY++;
      sInt co = *inCo++;
      sInt cg = *inCg++;
      sInt r,g,b;

      g = y + cg;
      b = y - cg;
      r = b + co;
      b = b - co;

      *dst++ = clampPixel((b >> 4) + 128);
      *dst++ = clampPixel((g >> 4) + 128);
      *dst++ = clampPixel((r >> 4) + 128);
      *dst++ = clampPixel((*inA++ >> 4) + 128);
    }
  }

  void color_x_convert_inv(sInt cols,sInt colsPad,const sS16 *src,sU8 *dst)
  {
    const sS16 *inY = src;
    const sS16 *inCo = src + colsPad;
    const sS16 *inCg = inCo + colsPad;

    for(sInt i=0;i<cols;i++)
    {
      sInt y = *inY++;
      sInt co = *inCo++;
      sInt cg = *inCg++;
      sInt r,g,b;

      g = y + cg;
      b = y - cg;
      r = b + co;
      b = b - co;

      *dst++ = clampPixel((b >> 4) + 128);
      *dst++ = clampPixel((g >> 4) + 128);
      *dst++ = clampPixel((r >> 4) + 128);
      *dst++ = 255;
    }
  }
}

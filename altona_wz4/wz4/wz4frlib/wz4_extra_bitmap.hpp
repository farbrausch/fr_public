/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_WZ4_EXTRA_BITMAP_HPP
#define FILE_WZ4FRLIB_WZ4_EXTRA_BITMAP_HPP

#include "base/types.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Bitmap operators that the wz3 generator cannot express.            ***/
/***                                                                      ***/
/***   These work on the same buffer GenBitmap holds -- one sU64 per       ***/
/***   pixel, four 16 bit channels running 0..0x7fff, packed b,g,r,a from  ***/
/***   the low word up -- but they take it as a plain pointer, so nothing  ***/
/***   here needs the engine and all of it can be tested on its own.       ***/
/***   See tools/wz4_extra_test.                                          ***/
/***                                                                      ***/
/****************************************************************************/

namespace Wz4Extra
{

static const sInt MaxChannel = 0x7fff;

// 0xaarrggbb -> the packed 4x16 bit form, matching the wz3 bitmap code.
sU64 Color64(sU32 col);

// border handling for everything that samples a source bitmap
enum SampleWrap
{
  WRAP_CLAMP = 0,
  WRAP_TILE  = 1,
};

// t is the value to look up a channel by
enum ChannelSelect
{
  CHANNEL_LUMINANCE = 0,
  CHANNEL_RED       = 1,
  CHANNEL_GREEN     = 2,
  CHANNEL_BLUE      = 3,
  CHANNEL_ALPHA     = 4,
};

/****************************************************************************/
/***   generators                                                         ***/
/****************************************************************************/

// countx/county cells across the image, phase shifts every other row sideways
void Checker(sU64 *dst,sInt xs,sInt ys,sInt countx,sInt county,sF32 phase,
             sU64 col0,sU64 col1);

// waves plane waves summed up: the classic plasma. angle turns the first one,
// spread fans the rest of them out from it.
void Sine(sU64 *dst,sInt xs,sInt ys,sInt waves,sF32 freq,sF32 angle,sF32 spread,
          sF32 phase,sF32 amp,sF32 gamma,sU64 col0,sU64 col1);

// julia set, or mandelbrot when julia is 0 (then cx/cy are ignored)
void Julia(sU64 *dst,sInt xs,sInt ys,sBool julia,sF32 cx,sF32 cy,
           sF32 centerx,sF32 centery,sF32 zoom,sInt iterations,sF32 cycle,
           sU64 colin,sU64 col0,sU64 col1);

/****************************************************************************/
/***   filters                                                            ***/
/****************************************************************************/

// map a channel of src through a palette taken from the first row of lut
void Colorize(sU64 *dst,const sU64 *src,sInt xs,sInt ys,
              const sU64 *lut,sInt lutlen,sInt channel,sBool keepalpha);

// signed distance to the threshold contour, mapped so that 0.5 sits on it and
// range (in pixels) reaches 0 and 1. Approximate: 8SSEDT, two passes.
void DistanceField(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sInt channel,
                   sF32 threshold,sF32 range,sInt wrap,sU64 col0,sU64 col1);

enum MorphMode
{
  MORPH_ERODE = 0,
  MORPH_DILATE,
  MORPH_OPEN,
  MORPH_CLOSE,
};

// grow or shrink bright areas. radius is in pixels, circle picks the kernel
// shape (which costs radius^2 per pixel, so it is capped tighter).
void Morphology(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sInt mode,
                sInt radius,sBool circle,sInt wrap);

// 3x3 or 5x5 median, per channel -- removes speckle without softening edges
void Median(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sInt size,sInt wrap);

// sobel gradient. mode 0: magnitude, mode 1: directional emboss around 0.5
void Sobel(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sInt mode,sInt channel,
           sF32 angle,sF32 amp,sInt wrap);

/****************************************************************************/
/***   samplers                                                           ***/
/****************************************************************************/

// fold the source into n sectors around a center, optionally mirroring every
// other one so the seams match
void Kaleidoscope(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sInt sectors,
                  sF32 cx,sF32 cy,sF32 angle,sF32 zoom,sBool mirror,sInt wrap);

enum ScatterMode
{
  SCATTER_OVER = 0,
  SCATTER_ADD,
  SCATTER_MAX,
};

// stamp src2 onto src1 count times, at random place, angle, size and tint
void Scatter(sU64 *dst,const sU64 *src,sInt xs,sInt ys,
             const sU64 *stamp,sInt sxs,sInt sys,
             sInt count,sInt seed,sF32 scalemin,sF32 scalemax,
             sF32 anglemin,sF32 anglemax,sF32 alpha,sF32 tint,sInt mode,sInt wrap);

/****************************************************************************/

}

#endif // FILE_WZ4FRLIB_WZ4_EXTRA_BITMAP_HPP

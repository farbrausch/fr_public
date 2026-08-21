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
/***   generators, second batch                                           ***/
/****************************************************************************/

// per pixel random. mono keeps the channels together, colour rolls each one
// separately, which is the difference between film grain and rgb static.
void Noise(sU64 *dst,sInt xs,sInt ys,sInt seed,sBool colour,sF32 gamma,
           sU64 col0,sU64 col1);

// arms spiralling out of a centre; twist is how many turns across the radius
void Spiral(sU64 *dst,sInt xs,sInt ys,sInt arms,sF32 twist,sF32 cx,sF32 cy,
            sF32 sharpness,sU64 col0,sU64 col1);

// a starburst: count wedges around a centre, fading out with distance
void Rays(sU64 *dst,sInt xs,sInt ys,sInt count,sF32 cx,sF32 cy,sF32 angle,
          sF32 sharpness,sF32 falloff,sU64 col0,sU64 col1);

// hexagonal grid. width 0 fills the cells, anything else draws their borders.
void Hexagon(sU64 *dst,sInt xs,sInt ys,sInt countx,sInt county,sF32 width,
             sInt seed,sF32 variation,sU64 col0,sU64 col1);

// truchet tiling: one of two rotations of the same tile per cell, which is
// what makes the arcs join up into a maze that never repeats visibly
void Truchet(sU64 *dst,sInt xs,sInt ys,sInt count,sInt seed,sF32 width,
             sBool diagonal,sU64 col0,sU64 col1);

// over-under basket weave
void Weave(sU64 *dst,sInt xs,sInt ys,sInt countx,sInt county,sF32 width,
           sF32 shadow,sU64 col0,sU64 col1,sU64 colgap);

// growth rings pushed around by turbulence
void Wood(sU64 *dst,sInt xs,sInt ys,sF32 rings,sF32 turbulence,sInt octaves,
          sInt seed,sF32 cx,sF32 cy,sF32 aspect,sU64 col0,sU64 col1);

// veins: a stripe pattern displaced by turbulence, which is the classic way
// to fake marble
void Marble(sU64 *dst,sInt xs,sInt ys,sF32 freq,sF32 turbulence,sInt octaves,
            sInt seed,sF32 angle,sF32 gamma,sU64 col0,sU64 col1);

enum ShapeGradientMode
{
  SHAPEGRAD_RADIAL = 0,
  SHAPEGRAD_ANGULAR,
  SHAPEGRAD_DIAMOND,
  SHAPEGRAD_SQUARE,
};

// the gradient operator interpolates along one axis; this one runs the same
// ramp out from a centre instead
void ShapeGradient(sU64 *dst,sInt xs,sInt ys,sInt mode,sF32 cx,sF32 cy,
                   sF32 radius,sF32 angle,const sF32 *pos,const sU64 *col,
                   sInt count,sBool repeat);

/****************************************************************************/
/***   filters, second batch                                              ***/
/****************************************************************************/

// hard cut with a soft shoulder, which is what a mask usually wants
void Threshold(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sInt channel,
               sF32 threshold,sF32 softness,sU64 col0,sU64 col1);

// piecewise linear tone curve, applied to the channels named in the mask
// (bit 0 red, 1 green, 2 blue, 3 alpha)
void Curve(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sInt channelmask,
           const sF32 *pos,const sF32 *value,sInt count);

// build a pixel out of four sources, one channel at a time. src[i] may repeat,
// and a null source means the constant in fill[i].
void Swizzle(sU64 *dst,sInt xs,sInt ys,const sU64 *const *src,
             const sInt *channel,const sF32 *fill);

enum DitherMode
{
  DITHER_ORDERED = 0,        // 8x8 bayer
  DITHER_ERROR,              // floyd-steinberg
};

// quantise to a number of levels per channel, and spread the error rather
// than dropping it -- which is the difference between this and Bitcrusher
void Dither(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sInt levels,sInt mode,
            sF32 strength);

// blur along one direction. the existing Blur is separable and axis aligned,
// so it cannot do this.
void MotionBlur(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sF32 angle,
                sF32 length,sInt samples,sInt wrap);

// zoom and spin blur around a centre
void RadialBlur(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sF32 cx,sF32 cy,
                sF32 zoom,sF32 spin,sInt samples,sInt wrap);

// pull the channels apart, radially from a centre or along an angle
void ChromaticAberration(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sF32 cx,
                         sF32 cy,sF32 amount,sBool radial,sF32 angle,sInt wrap);

// cross-fade the opposite edges into each other so the image tiles
void Seamless(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sF32 border,sBool mirror);

// spread the histogram out over the full range
void Equalize(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sBool perchannel,
              sF32 amount);

// combine two tangent space normal maps. Adding them and renormalising tilts
// the result wrong; this is the whiteout blend, which keeps the detail of
// both.
void NormalCombine(sU64 *dst,const sU64 *a,const sU64 *b,sInt xs,sInt ys,
                   sF32 strength);

// occlusion from a height map: how much of the sky each texel can still see
void AmbientOcclusion(sU64 *dst,const sU64 *src,sInt xs,sInt ys,sF32 radius,
                      sInt samples,sF32 height,sF32 strength,sInt wrap);

/****************************************************************************/

}

#endif // FILE_WZ4FRLIB_WZ4_EXTRA_BITMAP_HPP

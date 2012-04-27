/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_BITMAP_CODE_HPP
#define FILE_WZ4FRLIB_BITMAP_CODE_HPP

#include "base/types.hpp"
#include "wz4lib/basic.hpp"
#include "wz4lib/doc.hpp"

/****************************************************************************/

#define BI_ADD        0
#define BI_SUB        1
#define BI_MUL        2
#define BI_DIFF       3
#define BI_ALPHA      4
#define BI_MULCOL     5
#define BI_ADDCOL     6
#define BI_SUBCOL     7
#define BI_GRAY       8
#define BI_INVERT     9
#define BI_SCALECOL   10
#define BI_MERGE      11
#define BI_BRIGHTNESS 12
#define BI_SUBR       13
#define BI_MULMERGE   14
#define BI_SHARPEN    15

#define BI_HARDLIGHT  0x10
#define BI_OVER       0x11
#define BI_ADDSMOOTH  0x12
#define BI_MIN        0x13
#define BI_MAX        0x14

#define BI_RANGE      0x15

struct GenBitmapGradientPoint
{
  sF32 Pos;
  sVector4 Color;
};

/****************************************************************************/

class GenBitmap : public BitmapBase
{
public:
  GenBitmap();
  ~GenBitmap();
  void Init(sInt x,sInt y);
  void InitSize(GenBitmap *s) { Init(s->XSize,s->YSize); Atlas.Entries.Copy(s->Atlas.Entries); }
  void Init(const sImage *img);
  void CopyFrom(wObject *);
  void CopyTo(sImage *);
  void CopyTo(sImageI16 *);
  sBool Incompatible(GenBitmap *b) { return XSize!=b->XSize || YSize!=b->YSize; }


  sU64 *Data;                     // the bitmap itself
  sInt XSize;                     // xsize
  sInt YSize;                     // ysize
  sInt Size;                      // xsize*ysize, saves some bytes of code for common loops

  void Loop(sInt mode,GenBitmap *srca,GenBitmap *srcb);
  void Loop(sInt mode,sU64 *srca,GenBitmap *srcb);
  void PreMulAlpha();
  void Blit(sInt x,sInt y,GenBitmap *src);

  // generators

  void Flat(sU32 color);
  void Gradient(GenBitmapGradientPoint *,sInt count,sInt flags);
  void Perlin(sInt freq,sInt oct,sF32 fadeoff,sInt seed,sInt mode,sF32 amp,sF32 gamma,sU32 col0,sU32 col1);
  void GlowRect(sF32 cx,sF32 cy,sF32 rx,sF32 ry,sF32 sx,sF32 sy,sU32 color,sF32 alpha,sF32 power,sInt wrap,sInt flags);
  void Dots(sU32 color0,sU32 color1,sInt count,sInt seed);
  void Cell(sU32 col0,sU32 col1,sU32 col2,sInt max,sInt seed,sF32 amp,sF32 gamma,sInt mode,sF32 mindistf,sInt percent,sF32 aspect);

  // filters

  void Color(sInt mode,sU32 col);
  void Merge(sInt mode,GenBitmap *other);
  void HSCB(sF32 fh,sF32 fs,sF32 fc,sF32 fb);
  void ColorBalance(sVector30 shadows,sVector30 midtones,sVector30 highlights);
  void Blur(sInt flags,sF32 sx,sF32 sy,sF32 _amp);
  void Sharpen(GenBitmap *in,sInt order,sF32 sx,sF32 sy,sF32 amp);

  // sample

  void Rotate(GenBitmap *in,sF32 cx,sF32 cy,sF32 angle,sF32 sx,sF32 sy,sF32 tx,sF32 ty,sInt border);
  void RotateMul(sF32 cx,sF32 cy,sF32 angle,sF32 sx,sF32 sy,sF32 tx,sF32 ty,sInt border,sU32 color,sInt mode,sInt count,sU32 fade);
  void Twirl(GenBitmap *src,sF32 strength,sF32 gamma,sF32 rx,sF32 ry,sF32 cx,sF32 cy,sInt border);
  void Distort(GenBitmap *src,GenBitmap *map,sF32 dist,sInt border);
  void Normals(GenBitmap *src,sF32 _dist,sInt mode);
  void Unwrap(GenBitmap *src,sInt mode);
  void Bulge(GenBitmap *src,sF32 f);

  // special

  void Bump(
    GenBitmap *bb,sInt subcode,sF32 px,sF32 py,sF32 pz,sF32 da,sF32 db,
    sU32 _diff,sU32 _ambi,sF32 outer,sF32 falloff,sF32 amp,
    sU32 _spec,sF32 spow,sF32 samp);

  void Downsample(GenBitmap *in,sInt flags);

  void Text(sF32 x,sF32 y,sF32 width,sF32 height,sU32 col,sU32 flags,sF32 lineskip,const sChar *text,const sChar *fontname);

  void Bricks(
    sInt bmxs,sInt bmys,
    sInt color0,sInt color1,sInt colorf,
    sF32 ffugex,sF32 ffugey,sInt tx,sInt ty,
    sInt seed,sInt heads,sInt flags,
    sF32 side,sF32 colorbalance);

  void Vector(sU32 color,struct GenBitmapArrayVector *arr,sInt count);

  // helpers
  static sU64 GetColor64(sU32 c);
  static void Fade64(sU64 &r,sU64 &c0,sU64 &c1,sInt fade);
};


struct GenBitmapVectorLoop
{
  sInt start;
  sInt end;
  sInt mode;
};

void CalcGenBitmapVectorLoop(struct GenBitmapArrayVector *e,sInt count,sArray<GenBitmapVectorLoop>&a);
sInt LoadAtlas(const sChar *name, GenBitmap *bmp);

/****************************************************************************/

#endif // FILE_WZ4FRLIB_BITMAP_CODE_HPP


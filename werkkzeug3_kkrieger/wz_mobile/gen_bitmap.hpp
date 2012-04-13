// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __GENBITMAP__
#define __GENBITMAP__

#include "_types.hpp"
#include "doc.hpp"

/****************************************************************************/
/****************************************************************************/

class GenBitmap : public GenObject
{  
public:
  GenBitmap();
  ~GenBitmap();
  void Copy(GenBitmap *);
  GenBitmap *Copy();

  void Init(sInt x,sInt y);
  void MakeTexture(sInt format=1);

  void *Data;                     // the bitmap itself
  sInt XSize;                     // xsize
  sInt YSize;                     // ysize
  sInt Size;                      // xsize*ysize, saves some bytes of code for common loops
  sInt Format;                    // format to use when creating texture
  sInt Texture;                   // also available as texture (tm)
  sInt TexMipCount;
  sInt TexMipTresh;
};

/****************************************************************************/
/****************************************************************************/

extern sInt GenBitmapTextureSizeOffset;         // 0 = normal, -1 = smaller, 1 = large

GenBitmap * __stdcall Bitmap_Flat(sInt xs,sInt ys,sU32 color);
GenBitmap * __stdcall Bitmap_Format(GenBitmap *bm,sInt format,sInt count,sInt tresh);
GenBitmap * __stdcall Bitmap_RenderTarget(sInt xs,sInt ys,sU32 format);

GenBitmap * __stdcall Bitmap_Merge(sInt mode,sInt count,GenBitmap *b0,...);
GenBitmap * __stdcall Bitmap_Color(GenBitmap *bm,sInt mode,sU32 color);
GenBitmap * __stdcall Bitmap_Range(GenBitmap *bm,sInt mode,sU32 color0,sU32 color1);
GenBitmap * __stdcall Bitmap_GlowRect(GenBitmap *bm,sF32 cx,sF32 cy,sF32 rx,sF32 ry,sF32 sx,sF32 sy,sU32 color,sF32 alpha,sF32 power,sU32 wrap,sU32 bugfix);
GenBitmap * __stdcall Bitmap_Dots(GenBitmap *bm,sU32 color0,sU32 color1,sInt count,sInt seed);
GenBitmap * __stdcall Bitmap_Blur(GenBitmap *bm,sInt order,sF32 sx,sF32 sy,sF32 amp);
GenBitmap * __stdcall Bitmap_Mask(GenBitmap *bm,GenBitmap *bb,GenBitmap *bc,sInt mask);
GenBitmap * __stdcall Bitmap_HSCB(GenBitmap *bm,sF32 h,sF32 s,sF32 c,sF32 b);
GenBitmap * __stdcall Bitmap_Rotate(GenBitmap *bm,sF32 angle,sF32 sx,sF32 sy,sF32 tx,sF32 ty,sInt border,sInt newWidth,sInt newHeight);
GenBitmap * __stdcall Bitmap_RotateMul(GenBitmap *bm,sF32 angle,sF32 sx,sF32 sy,sF32 tx,sF32 ty,sInt border,sU32 color,sInt mode,sInt count,sU32 fade);
GenBitmap * __stdcall Bitmap_Distort(GenBitmap *bm,GenBitmap *bb,sF32 dist,sInt border);
GenBitmap * __stdcall Bitmap_Normals(GenBitmap *bm,sF32 dist,sInt mode);
GenBitmap * __stdcall Bitmap_Light(GenBitmap *bm,sInt subcode,sF32 px,sF32 py,sF32 pz,sF32 da,sF32 db,
                                   sU32 diff,sU32 ambi,sF32 outer,sF32 falloff,sF32 amp);
GenBitmap * __stdcall Bitmap_Bump(GenBitmap *bm,GenBitmap *bb,sInt subcode,sF32 px,sF32 py,sF32 pz,sF32 da,sF32 db,
                                  sU32 diff,sU32 ambi,sF32 outer,sF32 falloff,sF32 amp,
                                  sU32 spec,sF32 spow,sF32 samp);
//GenBitmap * __stdcall Bitmap_Text(KOp *,KEnvironment *,GenBitmap *bm,sF32 x,sF32 y,sF32 width,sF32 height,sU32 col,sU32 flags,sInt extspace,sF32 intspace,sF32 lineskip,sChar *text,sChar *font);
GenBitmap * __stdcall Bitmap_Perlin(sInt xs,sInt ys,sInt freq,sInt oct,sF32 fadeoff,sInt seed,sInt mode,sF32 amp,sF32 gamma,sU32 col0,sU32 col1);
GenBitmap * __stdcall Bitmap_Cell(sInt xs,sInt ys,sU32 col0,sU32 col1,sU32 col2,sInt count,sInt seed,sF32 amp,sF32 gamma,sInt mode,sF32 mindist,sInt percent,sF32 aspect);
GenBitmap * __stdcall Bitmap_Wavelet(GenBitmap *bm,sInt mode,sInt count);
GenBitmap * __stdcall Bitmap_Gradient(sInt xs,sInt ys,sU32 col0,sU32 col1,sF32 pos,sF32 angle,sF32 length,sInt mode);
GenBitmap * __stdcall Bitmap_Twirl(GenBitmap *bm,sF32 strength,sF32 gamma,sF32 rx,sF32 ry,sF32 cx,sF32 cy,sInt border);
GenBitmap * __stdcall Bitmap_Sharpen(GenBitmap *bm,sInt order,sF32 sx,sF32 sy,sF32 amp);

//GenBitmap * __stdcall Bitmap_Import(KOp *op,sChar *filename);

GenBitmap * __stdcall Bitmap_ColorBalance(GenBitmap *bm,sF323 shadows,sF323 midtones,sF323 highlights);
GenBitmap * __stdcall Bitmap_Unwrap(GenBitmap *bm,sInt mode);

/****************************************************************************/
/****************************************************************************/

sU64 GetColor64(sU32 c);
void Fade64(sU64 &r,sU64 &c0,sU64 &c1,sInt fade);
void AddScale64(sU64 &r,sU64 &c0,sU64 &c1,sInt fade);

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
#define BI_HARDLIGHT  16
#define BI_OVER       17
#define BI_RANGE      18

void __stdcall Bitmap_Inner64(sU64 *d,sU64 *s,sInt count,sInt mode,sU64 *x=0);

/****************************************************************************/

struct BilinearContext
{
  sU64 *Src;                      // image data
  sInt XShift;                    // xsize = 1 << XShift
  sInt XSize1,YSize1;             // xsize-1,ysize-1
  sU32 XAm,YAm;                   // x-andmask, y-andmask
};

void BilinearSetup(BilinearContext *ctx,sU64 *src,sInt w,sInt h,sInt b);
void BilinearFilter(BilinearContext *ctx,sU64 *r,sInt u,sInt v);
//void BilinearFilter(sU64 *r,sU64 *src,sInt w,sInt h,sInt u,sInt v,sInt b);

/****************************************************************************/
/****************************************************************************/

#endif

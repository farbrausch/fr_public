// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __GENBITMAP__
#define __GENBITMAP__

#include "_types.hpp"
class ScriptRuntime;
class GenBitmap;

/****************************************************************************/
/****************************************************************************/

sU32 GetColor32(const sInt4 &col);

/****************************************************************************/
/*
struct GenBitmapPara
{
  sInt op;                        // operation to perform  
  GenBitmap *b;                   // source b
  GenBitmap *c;                   // source c
  sU32 Para[64];                  // parameters
};
*/
class GenBitmap : public sObject
{
/*
  sInt QueueCount;                // number of queued ops (counted down during execution)
  sInt QueueCountMax;             // number of queued ops (max)
  GenBitmapPara QueueOps[64];     // queued ops

*/
public:
  GenBitmap();
  ~GenBitmap();
  sU32 GetClass() { return sCID_GENBITMAP; }
  void Copy(sObject *);

  void Init(sInt x,sInt y);

  sU64 *Data;                     // the bitmap itself
#if sINTRO_X
  static const sInt XSize=256;                     // xsize
  static const sInt YSize=256;                     // ysize
  static const sInt Size=XSize*YSize;                // xsize*ysize, saves some bytes of code for common loops
#else
  sInt XSize;                     // xsize
  sInt YSize;                     // ysize
  sInt Size;                      // xsize*ysize, saves some bytes of code for common loops
#endif
  sInt Texture;                   // also available as texture (tm)
};


/****************************************************************************/
/****************************************************************************/

void __stdcall Bitmap_SetSize(sInt x,sInt y);
void __stdcall Bitmap_SetTexture(GenBitmap *bm,sInt stage);
GenBitmap * __stdcall Bitmap_MakeTexture(GenBitmap *bm);
GenBitmap * __stdcall Bitmap_Flat(sInt4 color);
GenBitmap * __stdcall Bitmap_Merge(GenBitmap *bm,GenBitmap *bb,sInt mode);
GenBitmap * __stdcall Bitmap_Color(GenBitmap *bm,sInt mode,sInt4 color);
GenBitmap * __stdcall Bitmap_GlowRect(GenBitmap *bm,sF32 cx,sF32 cy,sF32 rx,sF32 ry,sF32 sx,sF32 sy,sInt4 color,sF32 alpha,sF32 power);
GenBitmap * __stdcall Bitmap_Dots(GenBitmap *bm,sInt4 color0,sInt4 color1,sInt count,sInt seed);
GenBitmap * __stdcall Bitmap_Blur(GenBitmap *bm,sInt order,sInt sx,sInt sy,sInt amp);
GenBitmap * __stdcall Bitmap_Mask(GenBitmap *bm,GenBitmap *bb,GenBitmap *bc);
GenBitmap * __stdcall Bitmap_HSCB(GenBitmap *bm,sInt h,sInt s,sInt c,sInt b);
GenBitmap * __stdcall Bitmap_Rotate(GenBitmap *bm,sF32 angle,sF32 sx,sF32 sy,sInt tx,sInt ty,sInt border);
GenBitmap * __stdcall Bitmap_Distort(GenBitmap *bm,GenBitmap *bb,sInt dist,sInt border);
GenBitmap * __stdcall Bitmap_Normals(GenBitmap *bm,sInt dist,sInt mode);
GenBitmap * __stdcall Bitmap_Light(GenBitmap *bm,sInt subcode,sF32 px,sF32 py,sF32 pz,sF32 da,sF32 db,
                                   sInt4 diff,sInt4 ambi,sF32 outer,sF32 falloff,sF32 amp);
GenBitmap * __stdcall Bitmap_Bump(GenBitmap *bm,GenBitmap *bb,sInt subcode,sF32 px,sF32 py,sF32 pz,sF32 da,sF32 db,
                                  sInt4 diff,sInt4 ambi,sF32 outer,sF32 falloff,sF32 amp,
                                  sInt4 spec,sF32 spow,sInt samp);
GenBitmap * __stdcall Bitmap_Text(GenBitmap *bm,sInt x,sInt y,sInt width,sInt height,sInt4 col,sChar *text);
GenBitmap * __stdcall Bitmap_Perlin(sInt freq,sInt oct,sF32 fadeoff,sInt seed,sInt mode,sF32 amp,sF32 gamma,sInt4 col0,sInt4 col1);
GenBitmap * __stdcall Bitmap_Cell(sInt4 col0,sInt4 col1,sInt4 col2,sInt count,sInt seed,sF32 amp,sF32 gamma,sInt mode);
GenBitmap * __stdcall Bitmap_Wavelet(GenBitmap *bm,sInt mode,sInt count);
GenBitmap * __stdcall Bitmap_Gradient(sInt4 col0,sInt4 col1,sInt pos,sF32 angle,sF32 length);
/*
sBool Bitmap_MakeTexture(ScriptRuntime *);
sBool Bitmap_SetSize(ScriptRuntime *);
sBool Bitmap_SetTexture(ScriptRuntime *);
sBool Bitmap_Flat(ScriptRuntime *);
sBool Bitmap_Merge(ScriptRuntime *);
sBool Bitmap_Color(ScriptRuntime *);
sBool Bitmap_GlowRect(ScriptRuntime *);
sBool Bitmap_Dots(ScriptRuntime *);

sBool Bitmap_Blur(ScriptRuntime *);
sBool Bitmap_Mask(ScriptRuntime *);
sBool Bitmap_HSCB(ScriptRuntime *);
sBool Bitmap_Rotate(ScriptRuntime *);
sBool Bitmap_Distort(ScriptRuntime *);
sBool Bitmap_Normals(ScriptRuntime *);
sBool Bitmap_Light(ScriptRuntime *);
sBool Bitmap_Bump(ScriptRuntime *);
sBool Bitmap_Light(ScriptRuntime *);

sBool Bitmap_Text(ScriptRuntime *);
sBool Bitmap_Perlin(ScriptRuntime *);
sBool Bitmap_Cell(ScriptRuntime *);
sBool Bitmap_Wavelet(ScriptRuntime *);
sBool Bitmap_Gradient(ScriptRuntime *);
*/
/****************************************************************************/

#define OP_NOP                0   // do nothing
#define OP_FLUSH              1   // really write to bitmap
#define OP_MAKETEXTURE        2   // write to texture 
#define OP_FLAT               3   // fill with color
#define OP_MERGE              4   // add sub ...
#define OP_COLOR              5   // color correct ..
#define OP_GLOWRECT           6   // unified rect and glow
#define OP_DOTS               7   // spray dots
#define OP_BLUR               8   // blur
#define OP_MASK               9   // use image as alpha-mask
#define OP_HSCB               10  // Hue Saturation Contrast Brightness
#define OP_ROTATE             11  // rotzoom
#define OP_DISTORT            12  // distort by bitmap
#define OP_NORMALS            13  // generate normals 
#define OP_BUMPLIGHT          14  // light with bump
#define OP_TEXT               15  // text
#define OP_PERLIN             16  // perlin noise
#define OP_CELL               17  // cell
#define OP_WAVELET            18  // wavelet
#define OP_GRADIENT						19	// gradient

/****************************************************************************/
/****************************************************************************/


#endif

/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_CHAOS_FONT_HPP
#define FILE_WZ4FRLIB_CHAOS_FONT_HPP

#include "base/types2.hpp"
#include "wz4lib/basic_ops.hpp"
#include "wz4lib/basic.hpp"

/****************************************************************************/

struct ChaosFontLetter
{
  sU16 Char;
  sU16 StartX;
  sU16 StartY;
  sS16 Cell;
  sS16 Pre;
  sS16 Post;
};

struct ChaosFontPrint
{
  sRect Rect;
  sU32 Color;
  const sChar *Text;
  sInt TextLen;
  sF32 Scale;
};

class ChaosFont : public BitmapBase
{
  sFont2D *Font;
  sInt CursorX;
  sInt CursorY;
  sInt Safety;
  sInt Outline;
  sInt Baseline;

  sGeometry *Geo;
  sTexture2D *Tex;
  sMaterial *Mtrl;

  sArray<ChaosFontPrint> PrintBuffer;

  void Letter(sVertexSingle *vp,sInt c,sF32 &x,sF32 y,sF32 s,sU32 col,sF32 sign);
public:
  ChaosFont();
  ~ChaosFont();
  void CopyFrom(ChaosFont *src);
  template <class streamer> void Serialize_(streamer &s);
  void Serialize(sWriter &);
  void Serialize(sReader &);

  void CopyTo(sImage *dest);
  void CopyTo(sImageI16 *dest);
  void CopyTo(sImageData *dest,sInt format);

  void Init(sInt sizex,sInt sizey);
  void InitFont(const sChar *name,sInt height,sInt width,sInt style,sInt safety,sInt outline);
  void Letter(sInt c);
  void Symbol(sInt c,const sRect &r,const sImage *img,sInt adjusty);
  void Finish();


  // simple printing

  void PreparePrint();
  void Print(const sViewport &view,const sChar *text,sU32 col,sF32 scale);

  // defered printing

  void Print(sInt x,sInt y,const sChar *text,sU32 col,sF32 scale,sInt len=-1);    // safe to call with 0-ptr
  void PrintC(sInt x,sInt y,const sChar *text,sU32 col,sF32 scale,sInt len=-1);    // safe to call with 0-ptr
  void PrintR(sInt x0,sInt x1,sInt y,const sChar *text,sU32 col,sF32 scale); // safe to call with 0-ptr
  void Rect(const sRect &r,sU32 col);   // safe to call with 0-ptr
  void Paint();
  
  // data

  sImage *Image;
  sInt LineFeed;
  sArray<ChaosFontLetter> Letters;
  sInt Height;
};

/****************************************************************************/

#endif // FILE_WZ4FRLIB_CHAOS_FONT_HPP


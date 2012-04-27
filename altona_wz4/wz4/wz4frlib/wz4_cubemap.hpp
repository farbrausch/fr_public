/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) by Dierk Ohlerich                                    ***/
/***   all rights reserverd                                               ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WERKKZEUG4_WZ4_CUBEMAP_HPP
#define FILE_WERKKZEUG4_WZ4_CUBEMAP_HPP

#include "base/types.hpp"
#include "base/graphics.hpp"
#include "wz4lib/doc.hpp"
#include "wz4lib/basic.hpp"
#include "wz4_bitmap.hpp"
#include "util/image.hpp"

/****************************************************************************/

class Wz4Cubemap : public CubemapBase
{
public:
  Wz4Cubemap();
  ~Wz4Cubemap();
  void Init(sInt sizexy);
  void CopyFrom(const Wz4Cubemap *);
  void CopyTo(sImage **cubefaces);
  void MakeCube(sInt pixel,sVector30 &n) const;
  void MakeCube(sInt face,sF32 fx,sF32 fy,sVector30 &n) const;
  void Sample(const sVector30 &n,Pixel &) const;

  sInt Size;                      // must be power of two
  sInt SquareSize;              // Size*Size
  sInt CubeSize;              // Size*Size*6
  sInt Shift;
  sInt Mask;
  sF32 RSize;
  sF32 HSize;
  Pixel *Data;

  enum TernaryOp
  {
    TernaryLerp = 0,  // (1-c.r) * a + c.r * b
    TernarySelect,
  };

  enum CombineOp
  {
    // simple arithmetic
    CombineAdd = 0,   // x=saturate(a+b)
    CombineSub,       // x=saturate(a-b)
    CombineMulC,      // x=a*b
    CombineMin,       // x=min(a,b)
    CombineMax,       // x=max(a,b)
    CombineSetAlpha,  // x.rgb=a.rgb, x.a=b.r
    CombinePreAlpha,  // x.rgb=a.rgb*b.r, x.a=b.r

    CombineOver,      // x=b over a
    CombineMultiply,
    CombineScreen,
    CombineDarken,
    CombineLighten,
  };

  // Actual generator functions
  void Flat(const Pixel &col);
  void Noise(const GenTexture *grad,sInt freq,sInt oct,sF32 fadeoff,sInt seed,sInt mode);
  void Glow(const Wz4Cubemap *background,const GenTexture *grad,const sVector30 &dir,sF32 radius);

  // Filters
  void ColorMatrixTransform(const Wz4Cubemap *in,const Matrix45 &matrix,sBool clampPremult);
  void CoordMatrixTransform(const Wz4Cubemap *in,const sMatrix34 &matrix);
  void ColorRemap(const Wz4Cubemap *in,const GenTexture *mapR,const GenTexture *mapG,const GenTexture *mapB);
//  void CoordRemap(const Wz4Cubemap *in,const GenTexture *remap,sF32 strengthU,sF32 strengthV,sInt filterMode);

  // Combiners
  void Ternary(const Wz4Cubemap *in1,const Wz4Cubemap *in2,const Wz4Cubemap *in3,TernaryOp op);
  void Binary(const Wz4Cubemap *in1,const Wz4Cubemap *in2,CombineOp op);
};



/****************************************************************************/

#endif // FILE_WERKKZEUG4_WZ4_CUBEMAP_HPP


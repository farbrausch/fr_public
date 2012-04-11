/****************************************************************************/
/***                                                                      ***/
/***   Written by Fabian Giesen.                                          ***/
/***   I hereby place this code in the public domain.                     ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef __TP_GENTEXTURE_HPP_
#define __TP_GENTEXTURE_HPP_

#include "types.hpp"

// Pixel. Uses whole 16bit value range (0-65535).
// 0=>0.0, 65535=>1.0.
union Pixel
{
  struct
  {
    sU16 r,g,b,a; // OpenGL byte order
  };
  sU64 v;         // the whole value

  void Init(sU8 r,sU8 g,sU8 b,sU8 a);
  void Init(sU32 rgba); // 0xaarrggbb (D3D style)

  void Lerp(sInt t,const Pixel &x,const Pixel &y); // t=0..65536
  
  void CompositeAdd(const Pixel &b);
  void CompositeMulC(const Pixel &b);
  void CompositeROver(const Pixel &b);
  void CompositeScreen(const Pixel &b);
};

// CellCenter. 2D pair of coordinates plus a cell color.
struct CellCenter
{
  sF32 x,y;
  Pixel color;
};

// LinearInput. One input for "linear combine".
struct GenTexture;

struct LinearInput
{
  const GenTexture *Tex;    // the input texture
  sF32 Weight;              // its weight
  sF32 UShift,VShift;       // u/v translate parameter
  sInt FilterMode;          // filtering mode (as in CoordMatrixTransform)
};

// Simple 4x4 matrix type
typedef sF32 Matrix44[4][4];

// X increases from 0 (left) to 1 (right)
// Y increases from 0 (bottom) to 1 (top)

struct GenTexture
{
  Pixel *Data;    // pointer to pixel data.
  sInt XRes;      // width of texture (must be a power of 2)
  sInt YRes;      // height of texture (must be a power of 2)
  sInt NPixels;   // width*height (number of pixels)

  sInt ShiftX;    // log2(XRes)
  sInt ShiftY;    // log2(YRes)
  sInt MinX;      // (1 << 24) / (2 * XRes) = Min X for clamp to edge
  sInt MinY;      // (1 << 24) / (2 * YRes) = Min Y for clamp to edge

  GenTexture();
  GenTexture(sInt xres,sInt yres);
  GenTexture(const GenTexture &x);
  ~GenTexture();

  void Init(sInt xres,sInt yres);
  void UpdateSize();
  void Swap(GenTexture &x);

  GenTexture &operator =(const GenTexture &x);

  sBool SizeMatchesWith(const GenTexture &x) const;

  // Sampling helpers with filtering (coords are 1.7.24 fixed point)
  void SampleNearest(Pixel &result,sInt x,sInt y,sInt wrapMode) const;
  void SampleBilinear(Pixel &result,sInt x,sInt y,sInt wrapMode) const;
  void SampleFiltered(Pixel &result,sInt x,sInt y,sInt filterMode) const;
  void SampleGradient(Pixel &result,sInt x) const;

  // Ternary operations
  enum TernaryOp
  {
    TernaryLerp = 0,  // (1-c.r) * a + c.r * b
    TernarySelect,
  };

  // Derive operations
  enum DeriveOp
  {
    DeriveGradient = 0,
    DeriveNormals,
  };

  // Combine operations
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

  // Noise mode
  enum NoiseMode
  {
    NoiseDirect = 0,      // use noise(x,y) directly
    NoiseAbs = 1,         // use abs(noise(x,y))

    NoiseUnnorm = 0,      // unnormalized (no further scaling)
    NoiseNormalize = 2,   // normalized (scale so values always fall into [0,1] with no clamping)

    NoiseWhite = 0,       // white noise function
    NoiseBandlimit = 4,   // bandlimited (perlin-like) noise function
  };

  // Cell mode
  enum CellMode
  {
    CellInner = 0,        // inner (distance to cell center)
    CellOuter = 1,        // outer (distance to edge)
  };

  // Filter mode
  enum FilterMode
  {
    WrapU = 0,            // wrap in u direction
    ClampU = 1,           // clamp (to edge) in u direction
    
    WrapV = 0,            // wrap in v direction
    ClampV = 2,           // clamp (to edge) in v direction

    FilterNearest = 0,    // nearest neighbor (point sampling)
    FilterBilinear = 4,   // bilinear filtering.
  };

  // Actual generator functions
  void Noise(const GenTexture &grad,sInt freqX,sInt freqY,sInt oct,sF32 fadeoff,sInt seed,sInt mode);
  void GlowRect(const GenTexture &background,const GenTexture &grad,sF32 orgx,sF32 orgy,sF32 ux,sF32 uy,sF32 vx,sF32 vy,sF32 rectu,sF32 rectv);
  void Cells(const GenTexture &grad,const CellCenter *centers,sInt nCenters,sF32 amp,sInt mode);

  // Filters
  void ColorMatrixTransform(const GenTexture &in,const Matrix44 &matrix,sBool clampPremult);
  void CoordMatrixTransform(const GenTexture &in,const Matrix44 &matrix,sInt filterMode);
  void ColorRemap(const GenTexture &in,const GenTexture &mapR,const GenTexture &mapG,const GenTexture &mapB);
  void CoordRemap(const GenTexture &in,const GenTexture &remap,sF32 strengthU,sF32 strengthV,sInt filterMode);
  void Derive(const GenTexture &in,DeriveOp op,sF32 strength);
  void Blur(const GenTexture &in,sF32 sizex,sF32 sizey,sInt order,sInt mode);

  // Combiners
  void Ternary(const GenTexture &in1,const GenTexture &in2,const GenTexture &in3,TernaryOp op);
  void Paste(const GenTexture &background,const GenTexture &snippet,sF32 orgx,sF32 orgy,sF32 ux,sF32 uy,sF32 vx,sF32 vy,CombineOp op,sInt mode);
  void Bump(const GenTexture &surface,const GenTexture &normals,const GenTexture *specular,const GenTexture *falloff,sF32 px,sF32 py,sF32 pz,sF32 dx,sF32 dy,sF32 dz,const Pixel &ambient,const Pixel &diffuse,sBool directional);
  void LinearCombine(const Pixel &color,sF32 constWeight,const LinearInput *inputs,sInt nInputs);
};

// Initialize the generator
void InitTexgen();

#endif // __TP_GENTEXTURE_HPP_

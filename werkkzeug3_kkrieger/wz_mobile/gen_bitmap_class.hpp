// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once
#include "_types.hpp"

#if !sPLAYER
#include "doc.hpp"
#endif
#include "doc2.hpp"


/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Tile Renderer                                                      ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#define BMTILE_SIZE 16
#define BMTILE_SHIFT 4

#define BMTILE_COUNT (BMTILE_SIZE*BMTILE_SIZE)
#define BMTILE_MASK (BMTILE_SIZE-1)

void InitBitmaps();
void InitPA();
void ExitPA();
void PrintPA();

class GenBitmap : public GenObject
{  
public:
  GenBitmap();
  ~GenBitmap();
  void Copy(GenBitmap *);
  GenBitmap *Copy();

  template <class Format> void Init(sInt x,sInt y);
  void MakeTexture(sInt format=1);
  sBitmap *MakeBitmap();
  sU8 *GetAdr(sInt x,sInt y)  { return &Data[((x&BMTILE_MASK)+y*BMTILE_SIZE+(x>>BMTILE_SHIFT)*YSize*BMTILE_SIZE)*BytesPerPixel]; }

  // the bitmap 

  sU8 *Data;                      // the bitmap itself
  sInt XSize;                     // xsize
  sInt YSize;                     // ysize
  sInt Pixels;                    // xsize*ysize, saves some bytes of code for common loops
  sInt BytesPerPixel;
  sInt BytesTotal;
  sInt Valid;                     // bitmap was completly created

  // texture upload specific

#if !sMOBILE
  sInt Format;                    // format to use when creating texture
  sInt Texture;                   // also available as texture (tm)
  sInt TexMipCount;
  sInt TexMipTresh;
#endif

  class SoftImage *Soft;
};

#if !sPLAYER
GenBitmap *FindBitmapInCache(sAutoArray<GenObject> *Cache,sInt xs,sInt ys,sInt var);
GenBitmap *CalcBitmap(GenOp *op,sInt xs,sInt ys,sInt var);
#endif


template <class Format> inline  void GenBitmap::Init(sInt x,sInt y)
{
  sVERIFY(!Data);
  sVERIFY(Texture == sINVALID);

  XSize = x;
  YSize = y;
  Pixels = x*y;
  BytesPerPixel = Format::ElementSize*Format::ElementCount;
  BytesTotal = BytesPerPixel*x*y;
  Variant = Format::Variant;

  Data = new sU8[BytesTotal];
}


#if !sPLAYER
void GenBitmapTile_Calc(GenOp *op,GenBitmap *bm);
#endif
void GenBitmapTile_Calc(GenNode *node,sInt xs,sInt ys,sU32 *data);


namespace GenBitmapTile {
;

/****************************************************************************/
/***                                                                      ***/
/***   Most fixed-comma Values are 1:16:15, NOT 1:15:16                   ***/
/***                                                                      ***/
/****************************************************************************/

__forceinline sInt Mul15(sInt var_a,sInt var_b)
{
#if sPLATFORM == sPLAT_PC
  __asm
  {
    mov   eax, [var_a]
    imul  [var_b]
    shrd  eax, edx, 15
    mov   [var_a], eax
  }

  return var_a;
#else
  return (sS32)( ((sS64)var_a)*((sS64)var_b)>>15 );
#endif
}

__forceinline sInt Mul24(sInt var_a,sInt var_b)
{
#if sPLATFORM == sPLAT_PC
  __asm
  {
    mov   eax, [var_a]
    imul  [var_b]
    shrd  eax, edx, 24
    mov   [var_a], eax
  }

  return var_a;
#else
  return (sS32)( ((sS64)var_a)*((sS64)var_b)>>24 );
#endif
}

__forceinline sInt Div15(sInt var_a,sInt var_b)
{
#if sPLATFORM == sPLAT_PC
  __asm
  {
    mov   eax, [var_a]
    mov   edx, eax
    shl   eax, 15
    sar   edx, 32-15
    idiv  [var_b]
    mov   [var_a], eax
  }
  
  return var_a;
#else
  return (sS32)( (((sS64)var_a)<<15)/((sS64)var_b) );
#endif
}

sInt ISqrt15D(sU32 i);
sInt Sqrt15D(sU32 i);
sInt Sin15(sInt x);
sInt Cos15(sInt x);
void SinCos15(sInt x,sInt &s,sInt &c);
sInt Log15(sInt x);
sInt ExpClip15(sInt x);
sInt Pow15(sInt x,sInt y);

/****************************************************************************/
/***                                                                      ***/
/***   Supported Pixelformats                                             ***/
/***                                                                      ***/
/****************************************************************************/

template <class Format> class Bitmap
{
public:
  Format *Data;                   // pointer to data, tile-by-tile (swizzled that is)
  sInt SizeX;                     // size of complete bitmap (in pixels)
  sInt SizeY;
  sInt ShiftX;
  sInt ShiftY;
  sInt RefCount;

  Bitmap(sInt xs,sInt ys,sInt xss,sInt yss)         { Data=new Format[xs*ys]; SizeX=xs; SizeY=ys; ShiftX=xss; ShiftY=yss; RefCount=0; }
  ~Bitmap()                       { delete[] Data; }
  Format &Adr(sInt x,sInt y) const{ x=x&(SizeX-1); y=y&(SizeY-1); return Data[(x&BMTILE_MASK)+y*BMTILE_SIZE+((x>>BMTILE_SHIFT)<<ShiftY)*BMTILE_SIZE]; }
};

/****************************************************************************/

struct Pixel16C
{
  typedef sU16 ElementType;
  enum 
  { 
    ElementSize = sizeof(ElementType),
    ElementShift = 15,
    ElementCount = 4,
    ElementMax = (1<<ElementShift)-1,
    Variant = GVI_BITMAP_TILE16C,
  };
  ElementType p[ElementCount];

  void Set32(sU32 col)
  {
    p[0] = (col>> 0)&0xff; p[0]=(p[0]<<7)|(p[0]>>1);
    p[1] = (col>> 8)&0xff; p[1]=(p[1]<<7)|(p[1]>>1);
    p[2] = (col>>16)&0xff; p[2]=(p[2]<<7)|(p[2]>>1);
    p[3] = (col>>24)&0xff; p[3]=(p[3]<<7)|(p[3]>>1);
  }
  void Set(sInt r,sInt g,sInt b,sInt a)
  {
    p[0] = r>>(15-ElementShift);
    p[1] = g>>(15-ElementShift);
    p[2] = b>>(15-ElementShift);
    p[3] = a>>(15-ElementShift);
  }
  sU32 Get32()
  {
    return ((p[2]&0x7f80)>> 7) |
           ((p[1]&0x7f80)<< 1) |
           ((p[0]&0x7f80)<< 9) |           
           ((p[3]&0x7f80)<<17);
  }
  void Get(sInt &r,sInt &g,sInt &b,sInt &a)
  {
    r = p[0];
    g = p[1];
    b = p[2];
    a = p[3];
  }

  struct PixelAccu
  {
    sInt p[ElementCount];
    void Clear() { for(sInt i=0;i<ElementCount;i++) p[i]=0; }
  };
  __forceinline void AddAccu(PixelAccu &akku,sInt scale)
  {
    for(sInt i=0;i<ElementCount;i++)
      akku.p[i] += p[i]*scale;
  }
  __forceinline void GetAccu(PixelAccu &akku,sInt scale)
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i] = sRange<sInt>(Mul15(akku.p[i],scale),ElementMax,0);
  }
  __forceinline void GetAccu24(PixelAccu &akku,sInt scale)
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i] = sRange<sInt>(Mul24(akku.p[i],scale),ElementMax,0);
  }

  __forceinline void Scale(const Pixel16C &x,sInt s) 
  { 
    for(sInt i=0;i<ElementCount;i++)
      p[i]=sMin<sInt>((x.p[i]*s)>>15,ElementMax); 
  }
  __forceinline void Mul(const Pixel16C &x,const Pixel16C &y) 
  { 
    for(sInt i=0;i<ElementCount;i++)
    {
      p[i]=(x.p[i]*y.p[i])>>ElementShift;
    }
  }
  __forceinline void MulScale(const Pixel16C &x,const Pixel16C &y,sInt shift) 
  { 
    for(sInt i=0;i<ElementCount;i++)
      p[i]=sMin<sInt>((x.p[i]*y.p[i])>>(ElementShift-shift),ElementMax);
  }
  __forceinline void Add(const Pixel16C &x,const Pixel16C &y) 
  { 
    for(sInt i=0;i<ElementCount;i++)
      p[i]=sMin<sInt>(x.p[i]+y.p[i],ElementMax); 
  }
  __forceinline void Sub(const Pixel16C &x,const Pixel16C &y) 
  { 
    for(sInt i=0;i<ElementCount;i++)
      p[i]=sMax<sInt>(x.p[i]-y.p[i],0); 
  }
  __forceinline void Fade(const Pixel16C &x,const Pixel16C &y,sInt s) 
  { 
    sInt s0=0x8000-s;
    for(sInt i=0;i<ElementCount;i++)
      p[i]=(x.p[i]*s0+y.p[i]*s)>>15; 
  }

  __forceinline void XOr(const Pixel16C &x,const Pixel16C &y)
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i] = x.p[i] ^ y.p[i];
  }
  __forceinline void Sign(const Pixel16C &x)
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i] = x.p[i]>(ElementMax/2)?ElementMax:0;
  }

  __forceinline void Zero()
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i]=0;
  }
  __forceinline void One()
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i]=ElementMax;
  }
  __forceinline void Gray()
  {
    for(sInt i=1;i<ElementCount;i++)
      p[i]=p[0];
  }
  __forceinline sInt Intensity()
  {
    return p[0]<<(15-ElementShift);
  }
  __forceinline void Alpha(const Pixel16C &x)
  {
    if(ElementCount==4)
      p[3] = x.p[0];
  }

  __forceinline void Filter(Bitmap<Pixel16C> *in,sInt u,sInt v)
  {
    sInt ur = u&0x7fff;
    sInt vr = v&0x7fff;
    Pixel16C &c00 = in->Adr((u>>15)+0,(v>>15)+0);
    Pixel16C &c01 = in->Adr((u>>15)+1,(v>>15)+0);
    Pixel16C &c10 = in->Adr((u>>15)+0,(v>>15)+1);
    Pixel16C &c11 = in->Adr((u>>15)+1,(v>>15)+1);

    for(sInt i=0;i<ElementCount;i++)
    {
      sInt a = c00.p[i]+(((c01.p[i]-c00.p[i])*ur)>>15);
      sInt b = c10.p[i]+(((c11.p[i]-c10.p[i])*ur)>>15);
      p[i] = a+(((b-a)*vr)>>15);
    }
  }
};

/****************************************************************************/

struct Pixel16I
{
  typedef sU16 ElementType;
  enum
  {
    ElementSize = sizeof(ElementType),
    ElementShift = 15,
    ElementCount = 1,
    ElementMax = (1<<ElementShift)-1,
    Variant = GVI_BITMAP_TILE16I,
  };
  ElementType p[ElementCount];

  void Set32(sU32 col)
  {
    p[0] = (col>>16)&0xff; p[0]=(p[0]<<7)|(p[0]>>1);
  }
  void Set(sInt r,sInt g,sInt b,sInt a)
  {
    p[0] = r>>(15-ElementShift);
  }
  sU32 Get32()
  {
    return ((p[0]&0x7f80)<< 9) |
           ((p[0]&0x7f80)<< 1) |
           ((p[0]&0x7f80)>> 7) |
           0xff000000;
  }
  void Get(sInt &r,sInt &g,sInt &b,sInt &a)
  {
    r = g = b = p[0];
    a = 0x7fff;
  }

  struct PixelAccu
  {
    sInt p[ElementCount];
    void Clear() { for(sInt i=0;i<ElementCount;i++) p[i]=0; }
  };
  __forceinline void AddAccu(PixelAccu &akku,sInt scale)
  {
    for(sInt i=0;i<ElementCount;i++)
      akku.p[i] += p[i]*scale;
  }
  __forceinline void GetAccu(PixelAccu &akku,sInt scale)
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i] = sRange<sInt>(Mul15(akku.p[i],scale),ElementMax,0);
  }
  __forceinline void GetAccu24(PixelAccu &akku,sInt scale)
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i] = sRange<sInt>(Mul24(akku.p[i],scale),ElementMax,0);
  }

  __forceinline void Scale(const Pixel16I &x,sInt s) 
  { 
    for(sInt i=0;i<ElementCount;i++)
      p[i]=sMin<sInt>((x.p[i]*s)>>15,ElementMax); 
  }
  __forceinline void Mul(const Pixel16I &x,const Pixel16I &y) 
  { 
    for(sInt i=0;i<ElementCount;i++)
      p[i]=(x.p[i]*y.p[i])>>ElementShift; 
  }
  __forceinline void MulScale(const Pixel16I &x,const Pixel16I &y,sInt shift) 
  { 
    for(sInt i=0;i<ElementCount;i++)
      p[i]=sMin<sInt>((x.p[i]*y.p[i])>>(ElementShift-shift),ElementMax);
  }
  __forceinline void Add(const Pixel16I &x,const Pixel16I &y) 
  { 
    for(sInt i=0;i<ElementCount;i++)
      p[i]=sMin<sInt>(x.p[i]+y.p[i],ElementMax); 
  }
  __forceinline void Sub(const Pixel16I &x,const Pixel16I &y) 
  { 
    for(sInt i=0;i<ElementCount;i++)
      p[i]=sMax<sInt>(x.p[i]-y.p[i],0); 
  }
  __forceinline void Fade(const Pixel16I &x,const Pixel16I &y,sInt s) 
  { 
    sInt s0=0x8000-s;
    for(sInt i=0;i<ElementCount;i++)
      p[i]=(x.p[i]*s0+y.p[i]*s)>>15; 
  }

  __forceinline void XOr(const Pixel16I &x,const Pixel16I &y)
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i] = x.p[i] ^ y.p[i];
  }
  __forceinline void Sign(const Pixel16I &x)
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i] = x.p[i]>(ElementMax/2)?ElementMax:0;
  }

  __forceinline void Zero()
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i]=0;
  }
  __forceinline void One()
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i]=ElementMax;
  }
  __forceinline void Gray()
  {
    for(sInt i=1;i<ElementCount;i++)
      p[i]=p[0];
  }
  __forceinline sInt Intensity()
  {
    return p[0]<<(15-ElementShift);
  }
  __forceinline void Alpha(const Pixel16I &x)
  {
    if(ElementCount==4)
      p[3] = x.p[0];
  }

  __forceinline void Filter(Bitmap<Pixel16I> *in,sInt u,sInt v)
  {
    sInt ur = u&0x7fff;
    sInt vr = v&0x7fff;
    Pixel16I &c00 = in->Adr((u>>15)+0,(v>>15)+0);
    Pixel16I &c01 = in->Adr((u>>15)+1,(v>>15)+0);
    Pixel16I &c10 = in->Adr((u>>15)+0,(v>>15)+1);
    Pixel16I &c11 = in->Adr((u>>15)+1,(v>>15)+1);

    for(sInt i=0;i<ElementCount;i++)
    {
      sInt a = c00.p[i]+(((c01.p[i]-c00.p[i])*ur)>>15);
      sInt b = c10.p[i]+(((c11.p[i]-c10.p[i])*ur)>>15);
      p[i] = a+(((b-a)*vr)>>15);
    }
  }
};

/****************************************************************************/

struct Pixel8C
{
  typedef sU8 ElementType;
  enum 
  {
    ElementSize = sizeof(ElementType),
    ElementShift = 8,
    ElementCount = 4,
    ElementMax = (1<<ElementShift)-1,
    Variant = GVI_BITMAP_TILE8C,
  };
  ElementType p[ElementCount];

  void Set32(sU32 col)
  {
    p[0] = (col>> 0)&0xff;
    p[1] = (col>> 8)&0xff;
    p[2] = (col>>16)&0xff;
    p[3] = (col>>24)&0xff;
  };
  void Set(sInt r,sInt g,sInt b,sInt a)
  {
    p[0] = r>>(15-ElementShift);
    p[1] = g>>(15-ElementShift);
    p[2] = b>>(15-ElementShift);
    p[3] = a>>(15-ElementShift);
  }
  sU32 Get32()
  {
    return ((p[2])>> 0) |
           ((p[1])<< 8) |
           ((p[0])<<16) |           
           ((p[3])<<24);
  };
  void Get(sInt &r,sInt &g,sInt &b,sInt &a)
  {
    r = (p[0]<<7)|(p[0]>>1);
    g = (p[1]<<7)|(p[1]>>1);
    b = (p[2]<<7)|(p[2]>>1);
    a = (p[3]<<7)|(p[3]>>1);
  };

  struct PixelAccu
  {
    sInt p[ElementCount];
    void Clear() { for(sInt i=0;i<ElementCount;i++) p[i]=0; }
  };
  __forceinline void AddAccu(PixelAccu &akku,sInt scale)
  {
    for(sInt i=0;i<ElementCount;i++)
      akku.p[i] += p[i]*scale;
  }
  __forceinline void GetAccu(PixelAccu &akku,sInt scale)
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i] = sRange<sInt>(Mul15(akku.p[i],scale),ElementMax,0);
  }
  __forceinline void GetAccu24(PixelAccu &akku,sInt scale)
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i] = sRange<sInt>(Mul24(akku.p[i],scale),ElementMax,0);
  }

  __forceinline void Scale(const Pixel8C &x,sInt s) 
  { 
    for(sInt i=0;i<ElementCount;i++)
      p[i]=sMin<sInt>((x.p[i]*s)>>15,ElementMax); 
  }
  __forceinline void Mul(const Pixel8C &x,const Pixel8C &y) 
  { 
    for(sInt i=0;i<ElementCount;i++)
      p[i]=(x.p[i]*y.p[i])>>ElementShift; 
  }
  __forceinline void MulScale(const Pixel8C &x,const Pixel8C &y,sInt shift) 
  { 
    for(sInt i=0;i<ElementCount;i++)
      p[i]=sMin<sInt>((x.p[i]*y.p[i])>>(ElementShift-shift),ElementMax);
  }
  __forceinline void Add(const Pixel8C &x,const Pixel8C &y) 
  { 
    for(sInt i=0;i<ElementCount;i++)
      p[i]=sMin<sInt>(x.p[i]+y.p[i],ElementMax); 
  }
  __forceinline void Sub(const Pixel8C &x,const Pixel8C &y) 
  { 
    for(sInt i=0;i<ElementCount;i++)
      p[i]=sMax<sInt>(x.p[i]-y.p[i],0); 
  }
  __forceinline void Fade(const Pixel8C &x,const Pixel8C &y,sInt s) 
  { 
    sInt s0=0x8000-s;
    for(sInt i=0;i<ElementCount;i++)
      p[i]=(x.p[i]*s0+y.p[i]*s)>>15; 
  }

  __forceinline void XOr(const Pixel8C &x,const Pixel8C &y)
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i] = x.p[i] ^ y.p[i];
  }
  __forceinline void Sign(const Pixel8C &x)
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i] = x.p[i]>(ElementMax/2)?ElementMax:0;
  }

  __forceinline void Zero()
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i]=0;
  }
  __forceinline void One()
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i]=ElementMax;
  }
  __forceinline void Gray()
  {
    for(sInt i=1;i<ElementCount;i++)
      p[i]=p[0];
  }
  __forceinline sInt Intensity()
  {
    return p[0]<<(15-ElementShift);
  }
  __forceinline void Alpha(const Pixel8C &x)
  {
    if(ElementCount==4)
      p[3] = x.p[0];
  }

  __forceinline void Filter(Bitmap<Pixel8C> *in,sInt u,sInt v)
  {

    sInt ur = u&0x7fff;
    sInt vr = v&0x7fff;
    Pixel8C &c00 = in->Adr((u>>15)+0,(v>>15)+0);
    Pixel8C &c01 = in->Adr((u>>15)+1,(v>>15)+0);
    Pixel8C &c10 = in->Adr((u>>15)+0,(v>>15)+1);
    Pixel8C &c11 = in->Adr((u>>15)+1,(v>>15)+1);

    for(sInt i=0;i<ElementCount;i++)
    {
      sInt a = c00.p[i]+(((c01.p[i]-c00.p[i])*ur)>>15);
      sInt b = c10.p[i]+(((c11.p[i]-c10.p[i])*ur)>>15);
      p[i] = a+(((b-a)*vr)>>15);
    }
  }
};

/****************************************************************************/

struct Pixel8I
{
  typedef sU8 ElementType;
  enum 
  {
    ElementSize = sizeof(ElementType),
    ElementShift = 8,
    ElementCount = 1,
    ElementMax = (1<<ElementShift)-1,
    Variant = GVI_BITMAP_TILE8I,
  };
  ElementType p[ElementCount];

  void Set32(sU32 col)
  {
    p[0] = (col>>16)&0xff;
  };
  void Set(sInt r,sInt g,sInt b,sInt a)
  {
    p[0] = r>>(15-ElementShift);
  }
  sU32 Get32()
  {
    return ((p[0])>> 0) |
           ((p[0])<< 8) |
           ((p[0])<<16) |           
           0xff000000;
  };
  void Get(sInt &r,sInt &g,sInt &b,sInt &a)
  {
    r = g = b = (p[0]<<7)|(p[0]>>1);
    a = 0x7fff;
  };

  struct PixelAccu
  {
    sInt p[ElementCount];
    void Clear() { for(sInt i=0;i<ElementCount;i++) p[i]=0; }
  };
  __forceinline void AddAccu(PixelAccu &akku,sInt scale)
  {
    for(sInt i=0;i<ElementCount;i++)
      akku.p[i] += p[i]*scale;
  }
  __forceinline void GetAccu(PixelAccu &akku,sInt scale)
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i] = sRange<sInt>(Mul15(akku.p[i],scale),ElementMax,0);
  }
  __forceinline void GetAccu24(PixelAccu &akku,sInt scale)
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i] = sRange<sInt>(Mul24(akku.p[i],scale),ElementMax,0);
  }

  __forceinline void Scale(const Pixel8I &x,sInt s) 
  { 
    for(sInt i=0;i<ElementCount;i++)
      p[i]=sMin<sInt>((x.p[i]*s)>>15,ElementMax); 
  }
  __forceinline void Mul(const Pixel8I &x,const Pixel8I &y) 
  { 
    for(sInt i=0;i<ElementCount;i++)
      p[i]=(x.p[i]*y.p[i])>>ElementShift; 
  }
  __forceinline void MulScale(const Pixel8I &x,const Pixel8I &y,sInt shift) 
  { 
    for(sInt i=0;i<ElementCount;i++)
      p[i]=sMin<sInt>((x.p[i]*y.p[i])>>(ElementShift-shift),ElementMax);
  }
  __forceinline void Add(const Pixel8I &x,const Pixel8I &y) 
  { 
    for(sInt i=0;i<ElementCount;i++)
      p[i]=sMin<sInt>(x.p[i]+y.p[i],ElementMax); 
  }
  __forceinline void Sub(const Pixel8I &x,const Pixel8I &y) 
  { 
    for(sInt i=0;i<ElementCount;i++)
      p[i]=sMax<sInt>(x.p[i]-y.p[i],0); 
  }
  __forceinline void Fade(const Pixel8I &x,const Pixel8I &y,sInt s) 
  { 
    sInt s0=0x8000-s;
    for(sInt i=0;i<ElementCount;i++)
      p[i]=(x.p[i]*s0+y.p[i]*s)>>15; 
  }

  __forceinline void XOr(const Pixel8I &x,const Pixel8I &y)
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i] = x.p[i] ^ y.p[i];
  }
  __forceinline void Sign(const Pixel8I &x)
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i] = x.p[i]>(ElementMax/2)?ElementMax:0;
  }

  __forceinline void Zero()
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i]=0;
  }
  __forceinline void One()
  {
    for(sInt i=0;i<ElementCount;i++)
      p[i]=ElementMax;
  }
  __forceinline void Gray()
  {
    for(sInt i=1;i<ElementCount;i++)
      p[i]=p[0];
  }
  __forceinline sInt Intensity()
  {
    return p[0]<<(15-ElementShift);
  }
  __forceinline void Alpha(const Pixel8I &x)
  {
    if(ElementCount==4)
      p[3] = x.p[0];
  }

  __forceinline void Filter(Bitmap<Pixel8I> *in,sInt u,sInt v)
  {
    sInt ur = u&0x7fff;
    sInt vr = v&0x7fff;
    Pixel8I &c00 = in->Adr((u>>15)+0,(v>>15)+0);
    Pixel8I &c01 = in->Adr((u>>15)+1,(v>>15)+0);
    Pixel8I &c10 = in->Adr((u>>15)+0,(v>>15)+1);
    Pixel8I &c11 = in->Adr((u>>15)+1,(v>>15)+1);

    for(sInt i=0;i<ElementCount;i++)
    {
      sInt a = c00.p[i]+(((c01.p[i]-c00.p[i])*ur)>>15);
      sInt b = c10.p[i]+(((c11.p[i]-c10.p[i])*ur)>>15);
      p[i] = a+(((b-a)*vr)>>15);
    }
  }
};

/****************************************************************************/

enum TileFlags
{
  TF_FIRST = 0x0001,              // first tile, do some precalcs...
  TF_LAST  = 0x0002,              // last tile
};

template <class Format> struct Tile
{
  Format *Data;
  sInt Flags;                     // flags for tile
  sInt SizeX;                     // size of complete bitmap (in pixels)
  sInt SizeY;
  sInt OffsetX;                   // offset in pixels - multiple of BMTILE_SIZE
  sInt OffsetY;
  sInt ShiftX;
  sInt ShiftY;
  template <class source> void Init(Format *data,source *tile)
  {
    Data = data;
    Flags = tile->Flags;
    SizeX = tile->SizeX;
    SizeY = tile->SizeY;
    OffsetX = tile->OffsetX;
    OffsetY = tile->OffsetY;
    ShiftX = tile->ShiftX;
    ShiftY = tile->ShiftY;
  };
  void Init(Format *data,sInt xs,sInt ys,sInt flags=TF_FIRST)
  {
    Data = data;
    Flags = flags;
    SizeX = xs;
    SizeY = ys;
    OffsetX = 0;
    OffsetY = 0;
    ShiftX = sGetPower2(xs);
    ShiftY = sGetPower2(ys);
  }
};

/*
class BitmapCache : public GenCache
{
  void *Object;
  sInt Variant;
public:

  void Flush();
  BitmapCache()  { Object=0; Variant=0; }
  ~BitmapCache() { Flush(); }
  sInt GetVariant() { return Variant; }

  template <class Format> Bitmap<Format> *Get(Tile<Format> *tile);  // access existing cache
  template <class Format> Bitmap<Format> *Set(Tile<Format> *tile);  // create new cache
};
*/
/****************************************************************************/

}; // end namespace GenBitmapTile

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Traditional Renderer                                               ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#if !sPLAYER
struct GenNode *GenBitmapFlat_MakeNode(GenOp *op);
GenBitmap *GenBitmapFlat_Calc(GenNode *node);
#endif

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Initialize Bitmap Class                                            ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#if !sPLAYER
void AddBitmapClasses(GenDoc *doc);
#endif
extern GenHandlerArray GenBitmapHandlers[];

/****************************************************************************/

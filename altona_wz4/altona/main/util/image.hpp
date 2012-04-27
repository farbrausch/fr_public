/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   This file contains classes for working with images                 ***/
/***                                                                      ***/
/***   The sImageData class is used to store images that are ready for    ***/
/***   usage as texture.                                                  ***/
/***   - all texture formats are supported                                ***/
/***   - no functions for manipulating are supported                      ***/
/***   - you can only load and save the altona file format                ***/
/***   - support for mipmaps and cubemaps                                 ***/
/***                                                                      ***/
/***   The sImage class provides functionality to work with images        ***/
/***   - only one pixel format is supported: ARGB888                      ***/
/***   - it's simple to work with the images                              ***/
/***   - you can convert to sImageData for a specific texture             ***/
/***   - you can load and save different file formats                     ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef HEADER_ALTONA_UTIL_IMAGE
#define HEADER_ALTONA_UTIL_IMAGE

#ifndef __GNUC__
#pragma once
#endif


#include "base/types.hpp"
#include "base/graphics.hpp"
class sImage;
class sTextureBase;

extern sU64 sTotalImageDataMem;  // total memory in sImageData bitmaps

/****************************************************************************/

enum sImageCodecType
{
  sICT_RAW    = 0x00,             // raw data
  sICT_IM49   = 0x01,             // IM49 codec

  sICT_COUNT  = 0x10,             // current max supported # of image codecs (increase if you need more)
};

/****************************************************************************/

class sImageData
{
  sInt DataSize;                  // size for all data, including cubemaps (allocated size)
  sInt UnpackedSize;              // size of unpacked data (==DataSize if CodecType==sICT_RAW)
  sInt CubeFaceSize;              // cubemaps are stored: face0 mm0 face0 mm1 face0 mm2 face1 mm0 case1 mm1 ...
public:
  sInt Format;                    // sTEX_XXX format
  sInt SizeX;                     // Width in pixels
  sInt SizeY;                     // Height in pixels
  sInt SizeZ;                     // for volumemaps. 0 otherwise
  sInt Mipmaps;                   // number of mipmaps
  sInt BitsPerPixel;              // for size calculation
  sU8 *Data;                      // the Data itself
  sInt PaletteCount;              // in color entries (32 bit units)
  sU32 *Palette;                  // if a palette is required, here it is!
  sInt NameId;                    // Texture serialization nameid
  sInt Quality;                   // dither quality for DXT conversions
  sInt CodecType;                 // sICT_***

  sImageData();
  sImageData(sImage *,sInt flagsandformat=sTEX_2D|sTEX_ARGB8888);
  sImageData(sImage *,sInt flagsandformat,sInt ditherquality);
  ~sImageData();

//  void Init(sInt format,sInt xs,sInt ys,sInt mipmaps=1);    // obsolete
  void Init2(sInt format,sInt mipmaps,sInt xs,sInt ys,sInt zs); // new version, with volume maps
  void InitCoded(sInt codecType,sInt format,sInt mipmaps,sInt xs,sInt ys,sInt zs,const sU8 *data,sInt dataSize);

  void Copy(const sImageData *source);
  void Swap(sImageData *img);
  sInt GetByteSize() const { return UnpackedSize; }
  sInt GetDiskSize() const { return DataSize; }
  sInt GetFaceSize() const { return CubeFaceSize; }
  template <class streamer> void Serialize_(streamer &s);
  void Serialize(sWriter &s);
  void Serialize(sReader &s);

  class sTextureBase *CreateTexture() const;
  class sTextureBase *CreateCompressedTexture() const;
  void UpdateTexture(sTextureBase *) const; // this will recreate the texture if extends don't fit.
  void UpdateTexture2(sTextureBase *&) const; // this will create the texture for given null ptr and reinit given texture if extends don't fit.

  void ConvertFrom(const sImage *);
  void ConvertTo(sImage *,sInt mipmap=0) const;
  void ConvertFromCube(sImage **imgptr);
  void ConvertToCube(sImage **imgptr, sInt mipmap=0)const;

  sOBSOLETE sInt Size()const { return DataSize; } // use GetByteSize!
};

void sGenerateMipmaps(sImageData *img);
sImage *sDecompressImageData(const sImageData *src);
sImageData *sDecompressAndConvertImageData(const sImageData *src);
sTextureBase *sStreamImageAsTexture(sReader &s);

typedef sImage *(*sDecompressImageDataHandler)(const sImageData *src);
void sSetDecompressHandler(sInt codecType,sDecompressImageDataHandler handler);

/****************************************************************************/

struct sFontMapFontDesc
{
  sString<256> Name;      // name of font to search by the fontsystem of the OS
  sU32         Size;      // fontsize in pixel
  sS32         YOffset;   // offset in y-direction (positive values shift the font upwards)
  sBool        Italic;    // font in italic mode
  sBool        Bold;      // font in bold mode
};

class sFontMapInputParameter
{
  public:
    sStaticArray<sFontMapFontDesc> FontsToSearchIn;
    sU32         AntiAliasingFactor;
    sBool        Mipmaps;   // font has mitmaps
    sBool        Hinting;   // font has hinting enabled
    sBool        PMAlpha;   // font supports premultiplied alpha
    sU32         Outline;   // size of outline in pixels
    sU32         Blur;      // size of blur kernel in pixels
    const sChar *Letters;   // the letters to be contained by the fontmap
};


// there are two units for pixels
// bitmap-pixels: 
//   without multisampling factor
// multi-pixels: 
//   with multisampling multi-pixels = bitmap-pixels * Alias

struct sFontMap
{
  sU16 X,Y;                       ///< position in bitmap in bitmap-pixels
  sS16 Pre;                       ///< kerning before character in multi-pixels
  sS16 Cell;                      ///< size of bitmap-cell in bitmap-pixels
  sS16 Post;                      ///< kerning after character in multi-pixels
  sS16 Advance;                   ///< total advance in multi-pixels
  sU16 Letter;                    ///< 16-bit unicode of character
  sS16 OffsetY;                   // vertical distance between the top of the character cell and the glyph
  sU16 Height;                    // height of the glyph
  sU16 Pad;
};



class sImage
{
public:
  sBool LoadBMP(const sU8 *data,sInt size);
  sBool LoadPIC(const sU8 *data,sInt size);
  sBool LoadTGA(const sChar *name);
  sBool LoadJPG(const sU8 *data,sInt size);
  sBool LoadPNG(const sU8 *data,sInt size);

  sBool SaveBMP(const sChar *name);
  sBool SavePIC(const sChar *name);
  sBool SaveTGA(const sChar *name);
  sBool SavePNG(const sChar *name);

  sInt SaveBMP(sU8 *data, sInt size);

public:
/*
  enum IChannel     // IChannel functions removed because rarely needed 
  {                 // and not really better than shifting & masking by hand.
    ChBLUE=0,       // delete this when nobody complains...
    ChGREEN,
    ChRED,
    ChALPHA,
  };
*/
  sInt SizeX;
  sInt SizeY;
  sU32 *Data;

  sImage();
  sImage(sInt xs,sInt ys);
  ~sImage();
  void Init(sInt xs,sInt ys);
  void CopyRect(sImage *src,const sRect &destpos,sInt srcx,sInt srcy);
  template <class streamer> void Serialize_(streamer &s);
  void Serialize(sWriter &);
  void Serialize(sReader &);

  sBool HasAlpha();                 // TRUE when at least one pixel is alpha!=255 -> use for DXT1/DXT5 check
  
  void Fill(sU32 color);
//  void Fill(sU32 srcBitmask,sU32 color);
  void SwapRedBlue();
  void SwapEndian();
#if sCONFIG_BE
  void SwapIfLE()       { }
  void SwapIfBE()       { SwapEndian(); }
#else
  void SwapIfLE()       { SwapEndian(); }
  void SwapIfBE()       { }
#endif

  void Checker(sU32 col0,sU32 col1,sInt maskx=8,sInt masky=8);
  void Glow(sF32 cx,sF32 cy,sF32 rx,sF32 ry,sU32 color,sF32 alpha=1.0f,sF32 power=0.5f);
  void Rect(sF32 x0,sF32 x1,sF32 y0,sF32 y1,sU32 color);
  void Perlin(sInt start,sInt octaves,sF32 falloff,sInt mode,sInt seed,sF32 amount);

  void MakeSigned();                    // convert from 0..255 unsigned to -128..127 signed
//  void ExchangeChannels(IChannel channelX, IChannel channelY);
//  void CopyChannelFrom(IChannel channelDst, IChannel channelSrc);
//  void SetChannel(IChannel channelX,sU8 val);
  void Copy(sImage *);
  void Add(sImage *);
  void Mul(sImage *);
  void Mul(sU32 color);
  void Blend(sImage *, sBool premultiplied);
  void ContrastBrightness(sInt contrast,sInt brightness); // x = (x-0.5)*c+0.5+b;
  void BlitFrom(const sImage *src, sInt sx, sInt sy, sInt dx, sInt dy, sInt width, sInt height);
  void Outline(sInt pixelCount);
  void BlurX(sInt R);
  void BlurY(sInt R);
  void Blur(sInt R, sInt passes=3);
  void FlipXY(); // flip x/y
  void PMAlpha();
  void ClearRGB();
  void ClearAlpha();
  void AlphaFromLuminance(sImage *);
  void PinkAlpha();
  void MonoToAll(); // min(alpha,luminance) -> all channels

  void HalfTransparentRect(sInt x0,sInt y0,sInt x1,sInt y1,sU32 color);
  void HalfTransparentRect(const sRect &r,sU32 color) { HalfTransparentRect(r.x0,r.y0,r.x1,r.y1,color); }
  void HalfTransparentRectHole(const sRect &outer,const sRect &hole,sU32 color);

  sFontMap *CreateFontPage(sFontMapInputParameter &inParam, sInt &outLetterCount, sInt *outBaseLine=0);
  

  sImage *Scale(sInt xs,sInt ys) const;
  void Scale(const sImage *src, sBool filter=sFALSE);
  sImage *Half(sBool gammacorrect=sFALSE) const;
  sImage *Copy() const;

  sU32 Filter(sInt x,sInt y) const;     // 24:8 pixel, requires power 2 texture!

//  sImageData *Convert(sInt tf,sInt mip=0) const;  // obsolete
//  void Convert(sImageData *);                     // obsolete

  sBool Load(const sChar *name);
  sBool Save(const sChar *name);

  void Diff(const sImage *img0, const sImage *img1);

  void sOBSOLETE CopyRenderTarget();
};

/****************************************************************************/
/***                                                                      ***/
/***   16 bit grayscale image                                             ***/
/***                                                                      ***/
/****************************************************************************/

class sImageI16
{
public:
  sInt SizeX;
  sInt SizeY;
  sU16 *Data;

  sImageI16();
  sImageI16(sInt xs,sInt ys);
  ~sImageI16();
  void Init(sInt xs,sInt ys);
  sImage *Copy() const;
  void CopyFrom(const sImageI16 *src);
  void CopyFrom(const sImage *src);

  template <class streamer> void Serialize_(streamer &s);
  void Serialize(sWriter &);
  void Serialize(sReader &);

  void Fill(sU16 value);
  void SwapEndian();
#if sCONFIG_BE
  void SwapIfLE()       { }
  void SwapIfBE()       { SwapEndian(); }
#else
  void SwapIfLE()       { SwapEndian(); }
  void SwapIfBE()       { }
#endif

  void Add(sImageI16 *);
  void Mul(sImageI16 *);
  void Mul(sU16 value);
  void FlipXY(); 
  sInt Filter(sInt x,sInt y,sBool colwrap=0) const;     // 24:8 pixel, requires power 2 texture!
};


/****************************************************************************/
/***                                                                      ***/
/***   32 bit float image (untested)                                      ***/
/***                                                                      ***/
/****************************************************************************/

class sFloatImage
{
public:
  sInt SizeX;
  sInt SizeY;
  sInt SizeZ;                     // for cube or volume map
  sF32 *Data;
  sBool Cubemap;                  // Interpret SizeZ as cubemap, not volumemap

  sFloatImage();
  sFloatImage(sInt xs,sInt ys,sInt zs=1);
  ~sFloatImage();
  void Init(sInt xs,sInt ys,sInt zs=1);
  void CopyFrom(const sFloatImage *src);
  void CopyFrom(const sImage *src);
  void CopyFrom(const sImageData *src);
  void CopyTo(sImage *dest) const;
  void CopyTo(sImage *dest,sF32 power) const; // power optimized for 0.5f , 1.0f and 2.0f, only for RGB
  void CopyTo(sU32 *dest,sF32 power) const;   // power optimized for 0.5f , 1.0f and 2.0f, only for RGB
  

  void Fill(sF32 r,sF32 g,sF32 b,sF32 a);
  void Scale(const sFloatImage *src,sInt xs,sInt ys); // no interpolation
  void Power(sF32 p);             // power optimized for 0.5f and 2.0f, only for RGB
  void Half(sBool linear);
  void Downsample(sInt mip,const sFloatImage *src);
  void Normalize();

  void ScaleAlpha(sF32 scale);
  sF32 GetAlphaCoverage(sF32 tresh);
  void AdjustAlphaCoverage(sF32 tresh,sF32 cov,sF32 error);
};


/****************************************************************************/
/***                                                                      ***/
/***   Some sImage helpers                                                ***/
/***                                                                      ***/
/****************************************************************************/

class sTexture2D *sLoadTexture2D(const sChar *name,sInt formatandflags);
class sTexture2D *sLoadTexture2D(const sImage *img,sInt formatandflags);
class sTexture2D *sLoadTexture2D(const sImageData *img);
class sTextureCube *sLoadTextureCube(const sChar *t0,const sChar *t1,const sChar *t2,const sChar *t3,const sChar *t4,const sChar *t5,sInt formatandflags);
class sTextureCube *sLoadTextureCube(const sImageData *img);
//sImage *sReadTexImage(sU32 *&data, sU32 &texflags);
//void sReadTexImage(sU32 *&data, sU32 &texflags, sImage *img);
void sSaveRT(const sChar *filename,sTexture2D *rt=0);

sInt sDiffImage(sImage *dest, const sImage *img0, const sImage *img1, sU32 mask=0xffffffff);
void Heightmap2Normalmap(sImage *dest, const sImage *src, sF32 scale /*= 1.0f*/);
sImageData *sMergeNormalMaps(const sImageData *img0, const sImageData *img1);

// HDR compression
inline sBool sCheckMRGB(sInt format) { format &= sTEX_FORMAT; return format==sTEX_MRGB8 || format==sTEX_MRGB16; }
void sCompressMRGB(sImageData *dst_img, sInt format, const sImageData *src_img);
void sDecompressMRGB(sImageData *dst_img, const sImageData *src_img, sF32 alpha=1.0f);
void sCompressMRGB(sImageData *img, sInt format);
void sDecompressMRGB(sImageData *img, sF32 alpha=1.0f);
void sClampToARGB8(sImageData *img, sInt mm=0);
sImageData *sConvertARGB8ToHDR(const sImageData *img, sInt format);

/****************************************************************************/

// HEADER_ALTONA_UTIL_IMAGE
#endif

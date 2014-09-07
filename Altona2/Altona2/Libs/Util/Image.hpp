/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_UTIL_IMAGE_HPP
#define FILE_ALTONA2_LIBS_UTIL_IMAGE_HPP

#include "Altona2/Libs/Base/Base.hpp"

namespace Altona2 {

class sImage;
class sFImage;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

int sCalcMipmapCount(int sx,int sy,int sz,int mipmaps,uint format);
sRect sFitAspect(int fx,int fy,int ix,int iy);
sFRect sFitAspect(float fx,float fy,float ix,float iy);

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class sImageData
{
    void Constructor();
public:
    int SizeX;
    int SizeY;
    int SizeZ;
    int SizeA;
    int Mipmaps;
    int Format;
    int BitsPerPixel;

    uint8 *Data;
    uptr Size;

    void Init1D(int mipmaps,int format,int sx);
    void Init2D(int mipmaps,int format,int sx,int sy);
    void Init3D(int mipmaps,int format,int sx,int sy,int sz);
    bool LoadKTX(const char *filename);
    bool LoadPVR(const char *filename);

    sImageData();
    ~sImageData();
    template <class Ser> void Serialize(Ser &s);
    void GetResPara(sResPara &rp);
    sResource *MakeTexture(sAdapter *adapter);
};
sReader& operator| (sReader &s,sImageData &v);
sWriter& operator| (sWriter &s,sImageData &v);

/****************************************************************************/

enum sImageDownsampleFilterEnum
{
    sIDF_Point      = 1,            // sharp and aliasing
    sIDF_Box        = 2,            // fast and will never bleed
    sIDF_Lanczos    = 3,            // NOT IMPLEMENTED!! very slow and not necessarily better...
};

class sImage
{
    int SizeX;
    int SizeY;
    uint *Data;
    int Alloc;

    bool SaveBmp(const char *name);
    bool SavePng(const char *name);
    bool BoolHasAlpha;
public:
    sImage();
    sImage(int sx,int sy);
    ~sImage();
    void Init(int sx,int sy);
    bool Init(sImageData &id);
    void CopyFrom(sImage *);
    void CopyFrom(sFImage *);
    void Clear(uint col);

    void BlitFrom(const sImage *src, int sx, int sy, int dx, int dy, int width, int height);

    uint *GetData() const { return Data; }
    int GetSizeX() const { return SizeX; }
    int GetSizeY() const { return SizeY; }

    void Set8Bit(uint *);
    void Set15Bit(uint16 *);
    void SetFloat(sVector4 *);
    void BlueToAlpha(uint color=0x00ffffff);

    void Downsample(sImage *src,int dx,int dy,sImageDownsampleFilterEnum filter);

    bool Load(const char *name);
    bool Save(const char *name);

    bool HasAlpha() const { return BoolHasAlpha; }

    template <class Ser> void Serialize(Ser &s);
};
sReader& operator| (sReader &s,sFImage &v);
sWriter& operator| (sWriter &s,sFImage &v);


class sFMonoImage           // monochrome (single channel) texture
{
    int SizeX;
    int SizeY;
    float *Data;
    int Alloc;
public:
    sFMonoImage();
    sFMonoImage(int sx,int sy);
    ~sFMonoImage();
    void Init(int sx,int sy);
    bool Init(sImageData &id);
    void Clear(float col);

    float *GetData() { return Data; };
    int GetSizeX() { return SizeX; }
    int GetSizeY() { return SizeY; }

    template <class Ser> void Serialize(Ser &s);
};
sReader& operator| (sReader &s,sImageData &v);
sWriter& operator| (sWriter &s,sImageData &v);


class sFImage
{
    int SizeX;
    int SizeY;
    sVector4 *Data;
    int Alloc;

    uptr PackPixelDXT(uint8 *dest,int format);
    uptr PackPixelPVR(uint8 *dest,int bits,bool alpha,bool tiling);
public:
    sFImage();
    sFImage(int sx,int sy);
    ~sFImage();
    void Init(int sx,int sy);
    bool Init(sImageData &id);

    void CopyFrom(sImage *);
    void CopyFrom(sFImage *);
    void CopyFrom(sFMonoImage *);
    void Clear(const sVector4 &col);
    void Clear(uint col);
    void Mul(const sVector4 &col);
    void Mul(uint col);
    void Add(const sVector4 &col);
    void Add(uint col);
    sVector4 *GetData() { return Data; };
    int GetSizeX() { return SizeX; }
    int GetSizeY() { return SizeY; }

    void Set8Bit(uint *);
    void Set15Bit(uint16 *);
    void SetFloat(sVector4 *);
    void SetFloatMono(float *);

    void Filter256(int x,int y,sVector4 &value);
    void Downsample(sFImage *src,int dx,int dy,sImageDownsampleFilterEnum filter);
    void Half(sFImage *src,sImageDownsampleFilterEnum filter);
    void Copy(sFImage *src);
    uptr PackPixels(uint8 *dest,int format,bool tiling);
    void Blit(sFImage *img,const sRect &src,int dx,int dy);
    void AlphaBlend(sFImage *src);
    void AlphaToColor();

    bool Load(const char *name);
    bool Save(const char *name);
    bool LoadPFM(const char *name, float gain=1);

    template <class Ser> void Serialize(Ser &s);
};

sReader& operator| (sReader &s,sImageData &v);
sWriter& operator| (sWriter &s,sImageData &v);

/****************************************************************************/

enum sPrepareTextureFlagEnum
{
    sPTF_Gamma        = 0x0001,     // input or output are using gamma 2.2
    sPTF_Normals      = 0x0002,     // output RGB contains a normal mapped to 0..1 range that will be normalized in the pixel shader
    sPTF_Tiling       = 0x0004,     // some texture compressions need a flag to indicate tiling textures
};

class sPrepareTexture2D
{
    uint8 *FinalData;
    uptr FinalSize;
    sFImage *Original;
    bool deleteoriginal;
public:
    sPrepareTexture2D();
    ~sPrepareTexture2D();
    void Load(sFImage *);           // set input and set all parameters to sane defaults. does not take memory ownership.
    void Load(sImage *);            // set input and set all parameters to sane defaults. does not take memory ownership.
    void Load(sFMonoImage *);       // set input and set all parameters to sane defaults. does not take memory ownership.
    void Load(uint *,int sx,int sy);
    void Process();                 // actually create texture data
    const uint8 *GetData() const { return FinalData; }
    uptr GetSize() const { return FinalSize; }
    sImageData *GetImageData();
    sResource *GetTexture(sAdapter *ada);

    int Mipmaps;                   // number of mipmaps to generate
    int SizeX;                     // possibly reduced size of mipmap level 0
    int SizeY;                     // possibly reduced size of mipmap level 0
    int Format;                    // pixel format for
    int InputFlags;                // mode of the input pixels (gamma)
    int OutputFlags;               // mode of the output pixels (gamma, normals)
    sImageDownsampleFilterEnum FirstFilter;  // filter to use for the first few mipmaps
    sImageDownsampleFilterEnum SecondFilter; // filter to use on the rest
    int FirstMipmaps;              // number of mipmaps that use the first filter. downfiltering from source image to specified size is considered the first filtering even if it is skipped
};

/****************************************************************************/

static inline float sSrgbToLinear(float x)
{
    return (x <= 0.04045f) ? (x / 12.92f) : sPow((x + 0.055f) / 1.055f, 2.4f);
}

}

#endif  // FILE_ALTONA2_LIBS_UTIL_IMAGE_HPP


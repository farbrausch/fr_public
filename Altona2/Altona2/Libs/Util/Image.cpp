/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Image.hpp"

using namespace Altona2;

#define STBI_HEADER_FILE_ONLY
#include "StbImage.c"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "StbImageWrite.h"
#define STB_DXT_IMPLEMENTATION
#include "StbDxt.h"

namespace Altona2 {


/****************************************************************************/

int sCalcMipmapCount(int sx,int sy,int sz,int mipmaps,uint format)
{
    if(mipmaps==0)
    {
        if(!sIsPower2(sx?sx:0) || !sIsPower2(sy?sy:0) || !sIsPower2(sz?sz:0))
        {
            mipmaps = 1;
        }
        else
        {
            int n = sx;
            if(sy && sy<n) n = sy;
            if(sz && sz<n) n = sz;
            mipmaps = sFindLowerPower(n)+1;
            if((format & sRFB_Mask) & sRFB_Mask4x4)
                mipmaps -= 2;
        }
    }
    return mipmaps;
}

/****************************************************************************/

sRect sFitAspect(int fx,int fy,int ix,int iy)
{
    sRect r;

    // letterbox

    int n = fx*iy/ix;
    if(n < fy)
        return sRect(0,(fy-n)/2,fx,(fy-n)/2+n);

    // pillarbox

    n = fy*ix/iy;
    if(n < fx)
        return sRect((fx-n)/2,0,(fx-n)/2+n,fy);

    // exact fit

    return sRect(0,0,fx,fy);
}

sFRect sFitAspect(float fx,float fy,float ix,float iy)
{
    sRect r;

    // letterbox

    float n = fx*iy/ix;
    if(n < fy)
        return sFRect(0,(fy-n)/2,fx,(fy-n)/2+n);

    // pillarbox

    n = fy*ix/iy;
    if(n < fx)
        return sFRect((fx-n)/2,0,(fx-n)/2+n,fy);

    // exact fit

    return sFRect(0,0,fx,fy);
}

/****************************************************************************/
/***                                                                      ***/
/***   Image data ready for texture creation                              ***/
/***                                                                      ***/
/****************************************************************************/

void sImageData::Constructor()
{
    int sx = SizeX;
    int sy = SizeY ? SizeY : 1;
    int sz = SizeZ ? SizeZ : 1;
    //  int sa = SizeA ? SizeA : 1;

    Mipmaps = sCalcMipmapCount(SizeX,SizeY,SizeZ,Mipmaps,Format);

    int size = 0;
    for(int i=0;i<Mipmaps;i++)
    {
        size += (sx*sy*BitsPerPixel/8)*sz;
        sx = sMax(1,sx/2);
        sy = sMax(1,sy/2);
        sz = sMax(1,sz/2);
    }

    if(size!=Size)
    {
        Size = size;
        sDelete(Data);
        Data = new uint8[Size];
    }
}

sImageData::sImageData()
{
    SizeX = 0;
    SizeY = 0;
    SizeZ = 0;
    SizeA = 0;
    Mipmaps = 0;
    Format = 0;
    BitsPerPixel = 0;
    Data = 0;
    Size = 0;
}

sImageData::~sImageData()
{
    delete[] Data;
}

void sImageData::Init1D(int mipmaps,int format,int sx)
{
    SizeX = sx;
    SizeY = 0;
    SizeZ = 0;
    SizeA = 0;
    Mipmaps = mipmaps;
    Format = format;
    BitsPerPixel = sGetBitsPerPixel(Format);
    Constructor();
}

void sImageData::Init2D(int mipmaps,int format,int sx,int sy)
{
    SizeX = sx;
    SizeY = sy;
    SizeZ = 0;
    SizeA = 0;
    Mipmaps = mipmaps;
    Format = format;
    BitsPerPixel = sGetBitsPerPixel(Format);
    Constructor();
}

void sImageData::Init3D(int mipmaps,int format,int sx,int sy,int sz)
{
    SizeX = sx;
    SizeY = sy;
    SizeZ = sz;
    SizeA = 0;
    Mipmaps = mipmaps;
    Format = format;
    BitsPerPixel = sGetBitsPerPixel(Format);
    Constructor();
}

template <class Ser> void sImageData::Serialize(Ser &s)
{
    int version = s.Begin(sSerId::sImageData,1,1);
    if(version>0)
    {
        s.Check(1024);
        s | SizeX | SizeY | SizeZ | SizeA | Mipmaps | Format | BitsPerPixel;
        s.ValueAs32(Size);

        if(s.IsReading())
        {
            delete[] Data;
            Data = new uint8[Size];
        }
        s.LargeData(Size,Data);
        s.End();
    }
}

void sImageData::GetResPara(sResPara &rp)
{
    rp.Mode = sRBM_Shader | sRU_Static | sRM_Texture;
    rp.Format = Format;
    rp.Flags = 0;
    rp.BitsPerElement = BitsPerPixel;
    rp.SizeX = SizeX;
    rp.SizeY = SizeY;
    rp.SizeZ = SizeZ;
    rp.SizeA = SizeA;
    rp.Mipmaps = Mipmaps;
    rp.UpdatePara();
}

sResource *sImageData::MakeTexture(sAdapter *adapter)
{
    sResPara rp;
    GetResPara(rp);
    return new sResource(adapter,rp,Data,Size);
}

struct sKTXFile
{
    char Id[12];
    uint Endianness;
    uint GlType;
    uint GlTypeSize;
    uint GlFormat;
    uint GlInternalFormat;
    uint GlBaseInternalFormat;
    uint Width;
    uint Height;
    uint Depth;
    uint NbOfArrayElements;
    uint NbOfFaces;
    uint NbOfMipmapLevels;
    uint BytesOfKeyValueData;
    uint Data[1];
};

struct sPVRFile
{
    uint Version;
    uint Flags;
    uint64 Format;
    uint Colour_Space;
    uint Channel_Type;
    uint Height;
    uint Width;
    uint Depth;
    uint NbSurfaces;
    uint NbFaces;
    uint MIPMapCnt;
    uint MetaDataSize;
    uint8 Data[1];
};

bool sImageData::LoadPVR(const char *filename)
{
    bool ret = false;
    sPVRFile *pvr = (sPVRFile *)sLoadFile(filename);
    if (pvr) 
    {        
        ret = true;
        //supported by us ?
        if (pvr->Version!=0x03525650)
        {
            sLogF("util","LoadPVR wrong version");
            ret = false;
        }
        
        if (pvr->Colour_Space!=0)
        {
            sLogF("util","LoadPVR wrong color space");
            ret = false;
        }

        if ((pvr->Format&0xffffffff00000000ul)!=0)
        {
            sLogF("util","LoadPVR wrong format");
            ret = false;
        }

        if (pvr->Channel_Type!=0)
        {
            sLogF("util","LoadPVR wrong channel type");
            ret = false;
        }

        if (pvr->Depth!=1)
        {
            sLogF("util","LoadPVR wrong depth");
            ret = false;
        }

        if (pvr->NbSurfaces!=1)
        {
            sLogF("util","LoadPVR wrong nbsurfaces");
            ret = false;
        }


        switch (pvr->Format)
        {
            case 0 : //PVRTC 2bpp RGB
                Format = sRF_PVR2;
                BitsPerPixel = 2;  
            break;
            case 1 : //PVRTC 2bpp RGBA
                Format = sRF_PVR2A;
                BitsPerPixel = 2;  
            break;
            case 2 : //PVRTC 4bpp RGB
                Format = sRF_PVR4;
                BitsPerPixel = 4;  
            break;
            case 3 : //PVRTC 4bpp RGBA
                Format = sRF_PVR4A;
                BitsPerPixel = 4;  
            break;
            case 6 : //ETC1
                Format = sRF_ETC1;
                BitsPerPixel = 4;
            break;
            case 7 : //BC1
                Format = sRF_BC1;
                BitsPerPixel = 4; 
            break;
            case 9 : //BC2
                Format = sRF_BC2;
                BitsPerPixel = 8; 
            break;
            case 11 : //BC3
                Format = sRF_BC3;
                BitsPerPixel = 8; 
            break;
            case 22 : //ETC2 RGB
                Format = sRF_ETC2;
                BitsPerPixel = 4;   
            break;
            case 23 : //ETC2 RGBA
                Format = sRF_ETC2A;
                BitsPerPixel = 8; 
            break;
            default :
                sLogF("util","Unknown format");
                ret = false;
            break;
        }
        if (ret)
        {
            Mipmaps = 1;
            SizeX = pvr->Width;
            SizeY = pvr->Height;        

            uint8 *data = (&pvr->Data[0]) +  pvr->MetaDataSize;

            Size = (SizeX*SizeY*BitsPerPixel)>>3;
            Data = new uint8[Size];
            sMoveMem(Data, data, Size);
            sLogF("util","%s has been loaded",filename);
        }
        delete []((uint8 *)pvr);
    }    
    return ret;
}

bool sImageData::LoadKTX(const char *filename)
{    
    bool ret = false;
    sKTXFile *ktx = (sKTXFile *)sLoadFile(filename);
    if (ktx) 
    {
        ret = true;

        //supported by us ?
        if (ktx->Endianness==0x01020304)
        {
            sLogF("util","LoadKTX wrong Endianness");
            ret = false;
        }            
        if (ktx->NbOfFaces!=1)
        {
            sLogF("util","LoadKTX wrong NbOfFaces");
            ret = false;
        }
        if (ktx->NbOfArrayElements!=0)
        {
            sLogF("util","LoadKTX wrong NbOfArrayElements");
            ret = false;
        }

        if (ktx->Depth!=0)
        {
            sLogF("util","LoadKTX wrong Depth");
            ret = false;
        }

        switch (ktx->GlInternalFormat)
        {
            case 0x83f3 :	//COMPRESSED_RGBA_S3TC_DXT5_EXT
                Format = sRF_BC3;
                BitsPerPixel = 8;        
            break;
            case 0x83f2 :	//COMPRESSED_RGBA_S3TC_DXT3_EXT
                Format = sRF_BC2;
                BitsPerPixel = 8;        
            break;
            case 0x83f0 :	//COMPRESSED_RGB_S3TC_DXT1_EXT
                Format = sRF_BC1;
                BitsPerPixel = 4;        
            break;
//Ericsson - Optional part of OpenGL|ES 2.x   (http://www.khronos.org/registry/gles/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt)
            case 0x8d64 :   //COMPRESSED_RGB8_ETC1 / ETC1_RGB8_OES
                Format = sRF_ETC1;
                BitsPerPixel = 4;
            break;
//Ericsson ETC2 - Integral part of OpenGL|ES 3.x
            case 0x9274 :  //COMPRESSED_RGB8_ETC2
                Format = sRF_ETC2;
                BitsPerPixel = 4;        
            break;
            case 0x9278 :   //COMPRESSED_RGBA8_ETC2_EAC
                Format = sRF_ETC2A;
                BitsPerPixel = 8;        
            break;
//PowerVR (http://www.khronos.org/registry/gles/extensions/IMG/IMG_texture_compression_pvrtc.txt)
            case 0x8c00 :   //COMPRESSED_RGB_PVRTC_4BPPV1_IMG
                Format = sRF_PVR4;
                BitsPerPixel = 4;                        
            break;
            case 0x8c01 :   //COMPRESSED_RGB_PVRTC_2BPPV1_IMG
                Format = sRF_PVR2;
                BitsPerPixel = 2;                        
            break;
            case 0x8c02 :   //COMPRESSED_RGBA_PVRTC_4BPPV1_IMG
                Format = sRF_PVR4A;
                BitsPerPixel = 4;                        
            break;
            case 0x8c03 :   //COMPRESSED_RGBA_PVRTC_2BPPV1_IMG
                Format = sRF_PVR2A;
                BitsPerPixel = 2;                        
            break;
            default :
                sLogF("util","LoadKTX unknown format %d",ktx->GlInternalFormat);
                ret = false;
            break;
        }

        if (ret)
        {
            uint *data = &ktx->Data[0];


            Mipmaps = 1;
            SizeX = ktx->Width;
            SizeY = ktx->Height;        

            //skip arbitrary stuff
            data += ((ktx->BytesOfKeyValueData+3)>>2);

            Size = *data++;
            Data = new uint8[Size];
            sMoveMem(Data, data, Size);
            sLogF("util","%s has been loaded",filename);
        }
        delete []((uint8 *)ktx);
    }
    return ret;
}


sReader& operator| (sReader &s,sImageData &v)
{
    v.Serialize(s); return s;
}

sWriter& operator| (sWriter &s,sImageData &v)
{
    v.Serialize(s); return s;
}



/****************************************************************************/
/***                                                                      ***/
/***   8bit per channel argb image                                        ***/
/***                                                                      ***/
/****************************************************************************/

sImage::sImage()
{
    SizeX = 0;
    SizeY = 0;
    Alloc = 0;
    Data = 0;
    BoolHasAlpha = false;
}

sImage::sImage(int sx,int sy)
{
    SizeX = 0;
    SizeY = 0;
    Alloc = 0;
    Data = 0;
    BoolHasAlpha = false;
    Init(sx,sy);
}

sImage::~sImage()
{
    delete[] Data;
}

void sImage::Init(int sx,int sy)
{
    if(sx*sy>Alloc)
    {
        sDelete(Data);
        Alloc = sx*sy;
        Data = new uint[sx*sy];
    }
    SizeX = sx;
    SizeY = sy;
}

void sImage::CopyFrom(sImage *src)
{
    Init(src->GetSizeX(),src->GetSizeY());
    Set8Bit(src->GetData());
}

void sImage::CopyFrom(sFImage *src)
{
    Init(src->GetSizeX(),src->GetSizeY());
    SetFloat(src->GetData());
}

void sImage::Clear(uint col)
{
    for(int i=0;i<SizeX*SizeY;i++)
        Data[i] = col;
}

void sImage::Set8Bit(uint *src)
{
    sCopyMem(Data,src,SizeX*SizeY*sizeof(uint));
}

void sImage::Set15Bit(uint16 *src)
{
    uint8 *dest = (uint8 *) Data;
    for(int i=0;i<SizeX*SizeY;i++)
    {
        dest[i*4+0] = ((src[i*4+0])&0x7fff)*255/0x7fff;
        dest[i*4+1] = ((src[i*4+1])&0x7fff)*255/0x7fff;
        dest[i*4+2] = ((src[i*4+2])&0x7fff)*255/0x7fff;
        dest[i*4+3] = ((src[i*4+3])&0x7fff)*255/0x7fff;
    }
}

void sImage::SetFloat(sVector4 *src)
{
    uint8 *dest = (uint8 *) Data;
    for(int i=0;i<SizeX*SizeY;i++)
    {
        dest[i*4+0] = int(sClamp(src[i].z,0.0f,1.0f)*255.0f);
        dest[i*4+1] = int(sClamp(src[i].y,0.0f,1.0f)*255.0f);
        dest[i*4+2] = int(sClamp(src[i].x,0.0f,1.0f)*255.0f);
        dest[i*4+3] = int(sClamp(src[i].w,0.0f,1.0f)*255.0f);
    }
}

void sImage::BlueToAlpha(uint color)
{
    color &= 0x00ffffff;
    for(int i=0;i<SizeX*SizeY;i++)
        Data[i] = ((Data[i]&0xff)<<24)|color;
}

void sImage::Downsample(sImage *src,int dx,int dy,sImageDownsampleFilterEnum filter)
{
    Init(dx,dy);
    int srcx = src->GetSizeX();
    int srcy = src->GetSizeY();

    if(filter==sIDF_Point)
    {
        uint *srcd = src->GetData();
        for(int y=0;y<SizeY;y++)
        {
            for(int x=0;x<SizeX;x++)
            {
                int x0 = x*srcx/SizeX;
                int y0 = y*srcy/SizeY;
                Data[y+SizeX+x] = srcd[y0*srcx+x0];
            }
        }
    }
    else  // no lanczos yet...
    {
        uint8 *srcd = (uint8 *) src->GetData();
        uint8 *dest = (uint8 *) Data;
        for(int y=0;y<SizeY;y++)
        {
            for(int x=0;x<SizeX;x++)
            {
                int x0 =  x   *srcx/SizeX;
                int y0 =  y   *srcy/SizeY;
                int x1 = (x+1)*srcx/SizeX;
                int y1 = (y+1)*srcy/SizeY;
                uint accu[4];
                sClear(accu);

                if(x1-x0==0 || y1-y0==0)
                {
                    uint8 *srcp = srcd + (y0*srcx+x0)*4;
                    dest[0] = srcp[0];
                    dest[1] = srcp[1];
                    dest[2] = srcp[2];
                    dest[3] = srcp[3];
                }
                else
                {
                    int n = 0;
                    for(int yy=y0;yy<y1;yy++)
                    {
                        for(int xx=x0;xx<x1;xx++)
                        {
                            uint8 *srcp = srcd + (yy*srcx+xx)*4;
                            accu[0] += srcp[0];
                            accu[1] += srcp[1];
                            accu[2] += srcp[2];
                            accu[3] += srcp[3];
                            n++;
                        }
                    }
                    dest[0] = (accu[0]+(n/2))/n;
                    dest[1] = (accu[1]+(n/2))/n;
                    dest[2] = (accu[2]+(n/2))/n;
                    dest[3] = (accu[3]+(n/2))/n;
                }

                dest+=4;
            }
        }
    }
}

bool sImage::Load(const char *name)
{
    bool ok = 0;
    uptr size;
    uint8 *src = sLoadFile(name,size);
    if(src)
    {
        int sx=0;
        int sy=0;
        int comp=0;
        uint8 *idata = stbi_load_from_memory(src,int(size),&sx,&sy,&comp,4);
        BoolHasAlpha = comp==4;
        if(idata)
        {
            for(int i=0;i<sx*sy;i++)
                sSwap(idata[i*4+0],idata[i*4+2]);
            ok = 1;
            Init(sx,sy);
            Set8Bit((uint *)idata);

            stbi_image_free(idata);
        }
        delete[] src;
    }
    return ok;
}

bool sImage::Save(const char *name)
{
    const char *ext = sExtractLastSuffix(name);
    if(sCmpStringI(ext,"bmp")==0)
        return SaveBmp(name);
    else if(sCmpStringI(ext,"png")==0)
        return SavePng(name);
    else
        return 0;
}

bool sImage::SaveBmp(const char *name)
{
    struct header
    {
        uint8 Magic[4];                 // we skip the first 2 bytes when writing, to maintain natural alignment
        uint FileSize;                // header size
        uint pad0;
        uint Const54;                 // offset to data

        uint Const40;                 // header size
        uint SizeX;
        uint SizeY;
        uint16 Const1;                  // planes
        uint16 Const24;                 // bits per plane
        uint pad5;                    // compression
        uint DataSize;
        uint pad1;                    // pixels per meter
        uint pad2;
        uint pad3;                    // palette count
        uint pad4;                    // prefered palette index
    };

    int size = sizeof(header)+sAlign(SizeX*3,4)*SizeY;
    uint8 *data = new uint8[size];
    sSetMem(data,0,sizeof(header));
    header *hdr = (header *)data;
    hdr->Magic[2] = 'B';
    hdr->Magic[3] = 'M';
    hdr->FileSize = size-2;
    hdr->Const54 = 54;
    hdr->Const40 = 40;
    hdr->Const1 = 1;
    hdr->Const24 = 24;
    hdr->SizeX = SizeX;
    hdr->SizeY = SizeY;
    hdr->DataSize = sAlign(SizeX*3,4)*SizeY;

    int padding = sAlign(SizeX*3,4)-SizeX*3;
    uint8 *d = data+56;
    const uint8 *s = (const uint8 *)Data;
    for(int y=0;y<SizeY;y++)
    {
        for(int x=0;x<SizeX;x++)
        {

            *d++ = s[((SizeY-y-1)*SizeX+x)*4+0];
            *d++ = s[((SizeY-y-1)*SizeX+x)*4+1];
            *d++ = s[((SizeY-y-1)*SizeX+x)*4+2];
        }
        for(int i=0;i<padding;i++)
            *d++ = 0;
    }

    bool ok = sSaveFile(name,data+2,size-2);
    delete[] data;
    return ok;
}

bool sImage::SavePng(const char *name)
{
    bool ok = 0;
    int len;

    uint8 *data = new uint8[SizeX*SizeY*4];
    const uint8 *src = (const uint8 *) Data;
    for(int i=0;i<SizeX*SizeY;i++)
    {
        data[i*4+0] = src[i*4+2];
        data[i*4+1] = src[i*4+1];
        data[i*4+2] = src[i*4+0];
        data[i*4+3] = src[i*4+3];
    }

    unsigned char *png = stbi_write_png_to_mem((unsigned char *)data,SizeX*sizeof(uint),SizeX,SizeY,4,&len);
    if(png)
    {
        ok = sSaveFile(name,png,len);
        delete[] png;
    }
    delete[] data;
    return ok;
}

bool sImage::Init(sImageData &id)
{
    Init(id.SizeX,id.SizeY);
    uint *d32 = (uint *)id.Data;

    switch(id.Format)
    {
    case sRF_BGRA8888:
        sCopyMem(id.Data,Data,SizeX*SizeY*4);
        break;

    case sRF_RGBA8888:
        for(int i=0;i<SizeX*SizeY;i++)
            Data[i] = (d32[i]&0xff00ff00) | ((d32[i]&0x00ff0000)>>16) | ((d32[i]&0x000000ff)<<16);
        break;

    default:
        for(int i=0;i<SizeX*SizeY;i++)
            Data[i] = 0xffff0000;
        return 0;
    }
    return 1;
}

void sImage::BlitFrom(const sImage *src, int sx, int sy, int dx, int dy, int width, int height)
{
  sInt sstartx = sx;
  if (sstartx>=src->SizeX)
    return;

  sInt sstarty = sy;
  if (sstarty>=src->SizeY)
    return;

  sInt endx = sstartx+width;
  sInt endy = sstarty+height;

  if (endx>src->SizeX)
    width -= endx - src->SizeX;

  if (endy>src->SizeY)
    height -= endy - src->SizeY;

  if (sstartx<0) 
  {
    width += sstartx;
    dx    -= sstartx;
    sstartx = 0;
  }
  
  if (sstarty<0) 
  {
    height += sstarty;
    dy     -= sstarty;
    sstarty = 0;
  }

  
  sInt dstartx = dx;
  if (dstartx>=SizeX)
    return;
  
  
  sInt dstarty = dy;
  if (dstarty>=SizeY)
    return;

  endx = dstartx+width;
  endy = dstarty+height;

  if (endx>SizeX)
    width -= endx - SizeX;

  if (endy>SizeY)
    height -= endy - SizeY;


  if (dstartx<0)
  {
    width   += dstartx;
    sstartx -= dstartx;
    dstartx = 0;
  }

  if (dstarty<0)
  {
    height   += dstarty;
    sstarty  -= dstarty;
    dstarty   = 0;
  }

  if (height<0 || width <0 || sstartx<0 || sstarty<0)
    return;

  sU32 *s = src->Data + sstarty * src->SizeX + sstartx;
  sU32 *d =      Data + dstarty *      SizeX + dstartx;
  
  width *= 4;
  for (sInt i=0; i<height; i++)
  {
    sCopyMem(d,s,width);
    s += src->SizeX;
    d += SizeX;
  }
}


/****************************************************************************/

template <class Ser> 
void sImage::Serialize(Ser &s)
{
    int version = s.Begin(sSerId::sImage,1,1);
    if(version)
    {
        s.Check(8);
        s | SizeX | SizeY;
        if(s.IsReading())
            Init(SizeX,SizeY);
        s.LargeData(SizeX*SizeY*sizeof(uint),Data);
        s.End();
    }
}

sReader& operator| (sReader &s,sImage &v)  { v.Serialize(s); return s; }
sWriter& operator| (sWriter &s,sImage &v)  { v.Serialize(s); return s; }

/****************************************************************************/
/***                                                                      ***/
/***   float monochrome image                                             ***/
/***                                                                      ***/
/****************************************************************************/

sFMonoImage::sFMonoImage()
{
    SizeX = 0;
    SizeY = 0;
    Alloc = 0;
    Data = 0;
}

sFMonoImage::sFMonoImage(int sx,int sy)
{
    SizeX = 0;
    SizeY = 0;
    Alloc = 0;
    Data = 0;
    Init(sx,sy);
}

sFMonoImage::~sFMonoImage()
{
    delete[] Data;
}

void sFMonoImage::Init(int sx,int sy)
{
    if(sx*sy>Alloc)
    {
        sDelete(Data);
        Alloc = sx*sy;
        Data = new float[sx*sy];
    }
    SizeX = sx;
    SizeY = sy;
}

void sFMonoImage::Clear(float col)
{
    for(int i=0;i<SizeX*SizeY;i++)
        Data[i] = col;
}

template <class Ser> 
void sFMonoImage::Serialize(Ser &s)
{
    int version = s.Begin(sSerId::sFMonoImage,1,1);
    if(version)
    {
        s.Check(8);
        s | SizeX | SizeY;
        if(s.IsReading())
            Init(SizeX,SizeY);
        s.LargeData(SizeX*SizeY*sizeof(float),Data);
        s.End();
    }
}

sReader& operator| (sReader &s,sFMonoImage &v)  { v.Serialize(s); return s; }
sWriter& operator| (sWriter &s,sFMonoImage &v)  { v.Serialize(s); return s; }

/****************************************************************************/
/***                                                                      ***/
/***   float image                                                        ***/
/***                                                                      ***/
/****************************************************************************/

sFImage::sFImage()
{
    SizeX = 0;
    SizeY = 0;
    Alloc = 0;
    Data = 0;
}

sFImage::sFImage(int sx,int sy)
{
    SizeX = 0;
    SizeY = 0;
    Alloc = 0;
    Data = 0;
    Init(sx,sy);
}

sFImage::~sFImage()
{
    delete[] Data;
}

void sFImage::Init(int sx,int sy)
{
    if(sx*sy>Alloc)
    {
        sDelete(Data);
        Alloc = sx*sy;
        Data = new sVector4[sx*sy];
    }
    SizeX = sx;
    SizeY = sy;
}

void sFImage::CopyFrom(sImage *src)
{
    Init(src->GetSizeX(),src->GetSizeY());
    Set8Bit(src->GetData());
}

void sFImage::CopyFrom(sFImage *src)
{
    Init(src->GetSizeX(),src->GetSizeY());
    SetFloat(src->GetData());
}

void sFImage::CopyFrom(sFMonoImage *src)
{
    Init(src->GetSizeX(),src->GetSizeY());
    SetFloatMono(src->GetData());
}

void sFImage::Clear(const sVector4 &col)
{
    for(int i=0;i<SizeX*SizeY;i++)
        Data[i] = col;
}

void sFImage::Mul(const sVector4 &col)
{
    for(int i=0;i<SizeX*SizeY;i++)
        Data[i] = Data[i] * col;
}

void sFImage::Add(const sVector4 &col)
{
    for(int i=0;i<SizeX*SizeY;i++)
        Data[i] = Data[i] * col;
}

void sFImage::Clear(uint col)
{
    sVector4 c;
    c.SetColor(col);
    Clear(c);
}

void sFImage::Mul(uint col)
{
    sVector4 c;
    c.SetColor(col);
    Mul(c);
}

void sFImage::Add(uint col)
{
    sVector4 c;
    c.SetColor(col);
    Mul(c);
}

void sFImage::Set8Bit(uint *src)
{
    for(int i=0;i<SizeX*SizeY;i++)
    {
        uint col = src[i];
        Data[i].x = ((col>>16)&255)/255.0f;
        Data[i].y = ((col>> 8)&255)/255.0f;
        Data[i].z = ((col    )&255)/255.0f;
        Data[i].w = ((col>>24)&255)/255.0f;
    }
}

void sFImage::Set15Bit(uint16 *src)
{
    for(int i=0;i<SizeX*SizeY;i++)
    {
        Data[i].x = ((src[i*4+2])&0x7fff)/float(0x7fff);
        Data[i].y = ((src[i*4+1])&0x7fff)/float(0x7fff);
        Data[i].z = ((src[i*4+0])&0x7fff)/float(0x7fff);
        Data[i].w = ((src[i*4+3])&0x7fff)/float(0x7fff);
    }
}

void sFImage::SetFloat(sVector4 *src)
{
    sCopyMem(Data,src,SizeX*SizeY*sizeof(sVector4));
}

void sFImage::SetFloatMono(float *src)
{
    for(int i=0;i<SizeX*SizeY;i++)
    {
        float col = src[i];
        Data[i].x = col;
        Data[i].y = col;
        Data[i].z = col;
        Data[i].w = 1.0f;
    }
}
/****************************************************************************/

void sFImage::Filter256(int x,int y,sVector4 &value)
{
    float fx = (x&255)/256.0f;
    float fy = (y&255)/256.0f;
    int x0 = sClamp(x>>8,0,SizeX-1);
    int y0 = sClamp(y>>8,0,SizeY-1);
    int x1 = x0+1; if(x1>=SizeX) x1=0;
    int y1 = y0+1; if(y1>=SizeY) y1=0;

    //       yx
    sVector4 *data = (sVector4 *) Data;
    sVector4 v00 = data[y0*SizeX+x0];
    sVector4 v01 = data[y0*SizeX+x1];
    sVector4 v10 = data[y1*SizeX+x0];
    sVector4 v11 = data[y1*SizeX+x1];

    sVector4 v0,v1;
    v0 = sLerp(v00,v01,fx);
    v1 = sLerp(v10,v11,fx);

    value = sLerp(v0,v1,fy);
}

/****************************************************************************/

void sFImage::Downsample(sFImage *src,int dx,int dy,sImageDownsampleFilterEnum filter)
{
    Init(dx,dy);
    int srcx = src->GetSizeX();
    int srcy = src->GetSizeY();
    sVector4 *srcd = src->GetData();

    if(filter==sIDF_Point)
    {
        for(int y=0;y<SizeY;y++)
        {
            for(int x=0;x<SizeX;x++)
            {
                int x0 = x*srcx/SizeX;
                int y0 = y*srcy/SizeY;
                Data[y+SizeX+x] = srcd[y0*srcx+x0];
            }
        }
    }
    else  // no lanczos yet...
    {
        for(int y=0;y<SizeY;y++)
        {
            for(int x=0;x<SizeX;x++)
            {
                int x0 =  x   *srcx/SizeX;
                int y0 =  y   *srcy/SizeY;
                int x1 = (x+1)*srcx/SizeX;
                int y1 = (y+1)*srcy/SizeY;
                sVector4 accu(0,0,0,0);

                int n = 0;
                if(x1-x0==0 || y1-y0==0)
                {
                    accu = srcd[y0*srcx+x0];
                    n++;
                }
                else
                {
                    for(int yy=y0;yy<y1;yy++)
                    {
                        for(int xx=x0;xx<x1;xx++)
                        {
                            accu = accu + srcd[yy*srcx+xx];
                            n++;
                        }
                    }
                }

                Data[y*SizeX+x] = accu*(1.0f/n);
            }
        }
    }
}

void sFImage::Half(sFImage *src,sImageDownsampleFilterEnum filter)
{
    int srcx = src->GetSizeX();
    int srcy = src->GetSizeY();
    sVector4 *srcd = src->GetData();
    sASSERT((srcx & 1)==0);
    sASSERT((srcy & 1)==0);
    Init(srcx/2,srcy/2);

    if(filter==sIDF_Point)
    {
        for(int y=0;y<SizeY;y++)
            for(int x=0;x<SizeX;x++)
                Data[y*SizeX+x] = srcd[y*2*srcx+x*2];
    }
    else if(filter==sIDF_Lanczos)
    {
        Downsample(src,SizeX,SizeY,filter);
    }
    else
    {
        for(int y=0;y<SizeY;y++)
            for(int x=0;x<SizeX;x++)
                Data[y*SizeX+x] = (srcd[(y*2+0)*srcx+(x*2+0)] 
            + srcd[(y*2+0)*srcx+(x*2+1)] 
            + srcd[(y*2+1)*srcx+(x*2+0)] 
            + srcd[(y*2+1)*srcx+(x*2+1)] ) * 0.25f;
    }
}

void sFImage::Copy(sFImage *src)
{
    Init(src->GetSizeX(),src->GetSizeY());
    sCopyMem(Data,src->GetData(),sizeof(sVector4)*SizeX*SizeY);
}

uptr sFImage::PackPixels(uint8 *dest,int format,bool tiling)
{
    // compressed formats

    switch(format)
    {
    case sRF_BC1: case sRF_BC1_SRGB:
    case sRF_BC3: case sRF_BC3_SRGB:
        return PackPixelDXT(dest,format);

    case sRF_PVR2:
        return PackPixelPVR(dest,2,0,tiling);
    case sRF_PVR4:
        return PackPixelPVR(dest,4,0,tiling);
    case sRF_PVR2A:
        return PackPixelPVR(dest,2,1,tiling);
    case sRF_PVR4A:
        return PackPixelPVR(dest,4,1,tiling);
    }

    // everything else

    int channels = format & sRFC_Mask;
    int bits = format & sRFB_Mask;
    int type = format & sRFT_Mask;
    uint8 *start = dest;

    uint max[4];
    max[0] = max[1] = max[2] = max[3] = 1;
    switch(bits)
    {
    case sRFB_4:
        max[0] = max[1] = max[2] = max[3] = 15;
        break;
    case sRFB_8:
        max[0] = max[1] = max[2] = max[3] = 255;
        break;
    case sRFB_16:
        max[0] = max[1] = max[2] = max[3] = 0xffff;
        break;
    case sRFB_32:
        max[0] = max[1] = max[2] = max[3] = 0xffffffff;
        break;
    case sRFB_5_6_5:
        max[0] = 31;
        max[1] = 63;
        max[2] = 31;
        break;
    case sRFB_5_5_5_1:
        max[0] = 31;
        max[1] = 31;
        max[2] = 31;
        max[3] = 1;
        break;
    case sRFB_10_10_10_2:
        max[0] = 1023;
        max[1] = 1023;
        max[2] = 1023;
        max[3] = 3;
        break;
    case sRFB_24_8:
        max[0] = 0xffffff;
        max[1] = 0xff;
        break;
    default:
        sASSERT0();
    }
    int count = 0;
    switch(channels)
    {
    case sRFC_R:    count = 1; break;
    case sRFC_RG:   count = 2; break;
    case sRFC_RGB:  count = 3; break;
    case sRFC_RGBA: count = 4; break;
    case sRFC_RGBX: count = 4; break;
    case sRFC_A:    count = 1; break;
    case sRFC_I:    count = 1; break;
    case sRFC_AI:   count = 2; break;
    case sRFC_IA:   count = 2; break;
    case sRFC_BGRA: count = 4; break;
    case sRFC_BGRX: count = 4; break;
    case sRFC_D:    count = 1; break;
    case sRFC_DS:   count = 2; break;
    }

    for(int i=0;i<SizeX*SizeY;i++)
    {
        uint val[4];
        float src[4];

        switch(channels)
        {
        default:
            src[0] = Data[i].x;
            src[1] = Data[i].y;
            src[2] = Data[i].z;
            src[3] = Data[i].w;
            break;
        case sRFC_BGRA:
        case sRFC_BGRX:
            src[0] = Data[i].z;
            src[1] = Data[i].y;
            src[2] = Data[i].x;
            src[3] = Data[i].w;
            break;
        case sRFC_RG:
            src[0] = Data[i].x;
            src[1] = Data[i].y;
            break;
        case sRFC_A:
            src[0] = Data[i].w;
            break;
        case sRFC_I:
            src[0] = (Data[i].x+Data[i].y+Data[i].z)/3.0f;
            break;
        case sRFC_AI:
            src[0] = Data[i].w;
            src[1] = (Data[i].x+Data[i].y+Data[i].z)/3.0f;
            break;
        case sRFC_IA:
            src[0] = (Data[i].x+Data[i].y+Data[i].z)/3.0f;
            src[1] = Data[i].w;
            break;
        }

        switch(type)
        {
        case sRFT_UNorm: case sRFT_SRGB:
            for(int j=0;j<count;j++)
                val[j] = sClamp(uint(src[j]*max[j]+0.5f),0U,max[j]);
            break;
        case sRFT_UInt:
            for(int j=0;j<count;j++)
                val[j] = sClamp(uint(src[j]+0.5f),0U,max[j]);
            break;
        case sRFT_SNorm:
            for(int j=0;j<count;j++)
                val[j] = sClamp(uint((src[j]*0.5+0.5)*max[j]+0.5),0U,max[j])-(max[j]+1)/2;
            break;
        case sRFT_SInt:
            for(int j=0;j<count;j++)
                val[j] = sClamp(uint((src[j]*0.5+0.5)+0.5),0U,max[j])-(max[j]+1)/2;
            break;
        case sRFT_Float:
            if (bits == sRFB_32)
            {
                for (int j = 0; j<count; j++)
                    val[j] = *(uint *)(src + j);
            }
            else if (bits == sRFB_16)
            {
                for (int j = 0; j < count; j++)
                    val[j] = sFloatToHalfSafe(src[j]);
            }
            else
                sASSERT(0);
            break;
        default:
            sASSERT0();
        }

        switch(bits)
        {
        case sRFB_4:
            if(count==2)
            {
                *dest++ = (val[0]<<4)|val[1];
            }
            else if(count==4)
            {
                *dest++ = (val[0]<<4)|val[1];
                *dest++ = (val[2]<<4)|val[3];
            }
            else
            {
                sASSERT0();
            }
            break;
        case sRFB_8:
            for(int j=0;j<count;j++)
                dest[j] = val[j];
            dest+=count;
            break;
        case sRFB_16:
            for(int j=0;j<count;j++)
                ((uint16*)(dest))[j] = val[j];
            dest+=count*2;
            break;
        case sRFB_32:
            for(int j=0;j<count;j++)
                ((uint*)(dest))[j] = val[j];
            dest+=count*4;
            break;
        case sRFB_5_6_5:
            *(uint16 *)dest = (val[0])|(val[1]<<5)|(val[2]<<11);
            dest+=2;
            break;
        case sRFB_5_5_5_1:
            *(uint16 *)dest = (val[0])|(val[1]<<5)|(val[2]<<10)|(val[3]<<15);
            dest+=2;
            break;
        case sRFB_10_10_10_2:
            *(uint *)dest = (val[0])|(val[1]<<10)|(val[2]<<20)|(val[3]<<30);
            dest+=4;
            break;
        case sRFB_24_8:
            *(uint *)dest = (val[0])|(val[1]<<24);
            dest+=4;
            break;
        default:
            sASSERT0();
        }
    }

    return dest-start;
}

bool sFImage::Init(sImageData &id)
{
    Init(id.SizeX,id.SizeY);
    uint8 *s8 = (uint8 *)id.Data;
    float *df = (float *)Data;
    int sxy = id.SizeX*id.SizeY;

    float i255 = 1.0f/255.0f;

    switch(id.Format)
    {
    case sRF_BGRA8888:
        for(int i=0;i<sxy;i++)
        {
            df[i*4+0] = s8[i*4+2]*i255;
            df[i*4+1] = s8[i*4+1]*i255;
            df[i*4+2] = s8[i*4+0]*i255;
            df[i*4+3] = s8[i*4+3]*i255;
        }
        break;

    case sRF_RGBA8888:
        for(int i=0;i<sxy;i++)
        {
            df[i*4+0] = s8[i*4+0]*i255;
            df[i*4+1] = s8[i*4+1]*i255;
            df[i*4+2] = s8[i*4+2]*i255;
            df[i*4+3] = s8[i*4+3]*i255;
        }
        break;

    default:
        for(int i=0;i<sxy;i++)
        {
            df[i*4+0] = 1;
            df[i*4+1] = 0;
            df[i*4+2] = 0;
            df[i*4+3] = 1;
        }
        return 0;
    }
    return 1;
}

void sFImage::Blit(sFImage *img,const sRect &srcrect,int dx,int dy)
{
    int sy = srcrect.SizeY();
    int sx = srcrect.SizeX();
    sVector4 *dest = Data + dx + dy*SizeX;
    sVector4 *src = img->GetData() + srcrect.x0 + srcrect.y0*img->SizeX;
    for(int y=0;y<sy;y++)
        for(int x=0;x<sx;x++)
            dest[y*SizeX + x] = src[y*img->SizeX + x];
}

void sFImage::AlphaBlend(sFImage *src)
{
    sASSERT(SizeX==src->GetSizeX() && SizeY==src->GetSizeY());
    for(int i=0;i<SizeX*SizeY;i++)
    {
        float a = src->Data[i].w;
        Data[i].x = Data[i].x * (1-a) + src->Data[i].x * a;
        Data[i].y = Data[i].y * (1-a) + src->Data[i].y * a;
        Data[i].z = Data[i].z * (1-a) + src->Data[i].z * a;
    }
}

void sFImage::AlphaToColor()
{
    for(int i=0;i<SizeX*SizeY;i++)
    {
        Data[i].x = Data[i].w;
        Data[i].y = Data[i].w;
        Data[i].z = Data[i].w;
        Data[i].w = 1.0;
    }
}

bool sFImage::Load(const char *name)
{
    sImage img;
    bool ok = img.Load(name);
    CopyFrom(&img);

    return ok;
}

bool sFImage::Save(const char *name)
{
    sImage img;
    img.CopyFrom(this);
    return img.Save(name);
}

inline void sSwapEndianI(sU32 &a) { a = ((a & 0xff000000) >> 24) | ((a & 0x00ff0000) >> 8) | ((a & 0x0000ff00) << 8) | ((a & 0x000000ff) << 24); }

bool sFImage::LoadPFM(const char *name, float gain)
{
    uint pixels;
    uint pos;

    uint8 *src = 0;
    bool ok = false;

    uptr size;
    src = sLoadFile(name, size);
    if (!src)
        goto end;

    bool mono;
    uint sx, sy;
    bool BE;
    float scale;

    // "parse" header
    pos = 0;
    for (int state = 0; state < 4; state++)
    {
        char buf[32];
        uint bpos = 0;

        while (pos < size && bpos<32 && src[pos]>32)
            buf[bpos++] = src[pos++];

        if (pos >= size || bpos == 32)
            goto end;

        buf[bpos] = 0;
        pos++;
        sString<32> str = buf;
        const char *scan = &str[0];
        switch (state)
        {
        case 0:
            if (str == "PF") mono = false;
            else if (str == "Pf") mono = true;
            else goto end;
            break;
        case 1:
            if (!sScanValue(scan, sx)) goto end;
            break;
        case 2:
            if (!sScanValue(scan, sy)) goto end;
            break;
        case 3:
            if (!sScanValue(scan, scale)) goto end;
            BE = (scale > 0);
            if (!BE) scale = -scale;
            break;
        }
    }

    // load float pixels
    pixels = sx*sy;
    if ((size - pos) < (pixels*(mono ? 1 : 3) * 4))
        goto end;

    Init(sx, sy);
    gain /= scale; // or the other way around?
    for (uint y = 1; y <= sy; y++) for (uint x = 0; x < sx; x++)
    {
        sVector4 &pix = Data[sx*(sy - y) + x];
        sU32 v;
        v = *(sU32*)(src + pos); pos += 4; if (BE) sSwapEndianI(v);
        pix.x=gain**(sF32*)&v;
        if (mono)
        {
            pix.z = pix.y = pix.x;
        }
        else
        {
            v = *(sU32*)(src + pos); pos += 4; if (BE) sSwapEndianI(v);
            pix.y = gain**(sF32*)&v;
            v = *(sU32*)(src + pos); pos += 4; if (BE) sSwapEndianI(v);
            pix.z = gain**(sF32*)&v;
        }
        pix.w = 1;
    }
    ok = true;

end:
    delete src;
    return ok;
}

/****************************************************************************/

uptr sFImage::PackPixelDXT(uint8 *dest,int format)
{
    sASSERT((SizeX & 3)==0);
    sASSERT((SizeY & 3)==0);

    int sx = SizeX/4;
    int sy = SizeY/4;
    int mode = STB_DXT_HIGHQUAL;
    bool dxt1 = (format&sRFB_Mask) == sRFB_BC1;
    int alpha = dxt1 ? 0 : 1;
    int blocksize = dxt1 ? 8 : 16;

    uint8 *start = dest;
    uint8 buf[16*4];
    for(int y=0;y<sy;y++)
    {
        for(int x=0;x<sx;x++)
        {
            for(int yy=0;yy<4;yy++)
            {
                sVector4 *s = Data+(y*4+yy)*SizeX + x*4;
                for(int xx=0;xx<4;xx++)
                {
                    buf[yy*16+xx*4+0] = int(s->x*255);
                    buf[yy*16+xx*4+1] = int(s->y*255);
                    buf[yy*16+xx*4+2] = int(s->z*255);
                    buf[yy*16+xx*4+3] = int(s->w*255);
                    s++;
                }
            }
            stb_compress_dxt_block(dest,buf,alpha,mode);
            dest += blocksize;
        }
    }

    return dest - start;
}

#if sConfigPlatform != sConfigPlatformIOS && sConfigPlatform != sConfigPlatformOSX && sConfigPlatform != sConfigPlatformAndroid
class sPvrTexLibSubsystem_ : public sSubsystem
{
    sDynamicLibrary *PvrLib;
public:
    sPvrTexLibSubsystem_() : sSubsystem("pvrtexlibx",0x80)
    {
        PvrLib = 0;
    }

    void ManualInit()
    {
        if(!PvrLib)
        {
            PvrLib = new sDynamicLibrary;
            if(!PvrLib->Load("pvrtexlibx.dll"))
                sFatal("could not load <pvrtexlibx.dll>");
            PvrConvert = (PvrConvertT) PvrLib->GetSymbol("PvrConvert");
            sASSERT(PvrConvert);
        }
    }
    void Exit()
    {
        delete PvrLib;
    }

    typedef void (*PvrConvertT)(unsigned char *in,int sx,int sy,unsigned char *out,int bits,bool alpha,bool border);
    PvrConvertT PvrConvert;
} sPvrTexLibSubsystem;

uptr sFImage::PackPixelPVR(uint8 *dest,int bits,bool alpha,bool tiling)
{
    sPvrTexLibSubsystem.ManualInit();

    uint8 *data = new uint8[SizeX*SizeY*4];
    const sVector4 *src = Data;
    for(int i=0;i<SizeX*SizeY;i++)
    {
        data[i*4+0] = int(sClamp(src[i].x,0.0f,1.0f)*255.0f);
        data[i*4+1] = int(sClamp(src[i].y,0.0f,1.0f)*255.0f);
        data[i*4+2] = int(sClamp(src[i].z,0.0f,1.0f)*255.0f);
        data[i*4+3] = int(sClamp(src[i].w,0.0f,1.0f)*255.0f);
    }
    (*(sPvrTexLibSubsystem.PvrConvert))(data,SizeX,SizeY,dest,bits,alpha,!tiling);
    delete[] data;

    return (SizeX*bits/8)*SizeY;
}
#else
uptr sFImage::PackPixelPVR(uint8 *dest,int bits,bool alpha,bool border)
{
    sASSERT0();
    return (SizeX*bits/8)*SizeY;
}
#endif

/****************************************************************************/

template <class Ser> 
void sFImage::Serialize(Ser &s)
{
    int version = s.Begin(sSerId::sImage,1,1);
    if(version)
    {
        s.Check(8);
        s | SizeX | SizeY;
        if(s.IsReading())
            Init(SizeX,SizeY);
        s.LargeData(SizeX*SizeY*sizeof(sVector4),Data);
        s.End();
    }
}

sReader& operator| (sReader &s,sFImage &v)  { v.Serialize(s); return s; }
sWriter& operator| (sWriter &s,sFImage &v)  { v.Serialize(s); return s; }

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sPrepareTexture2D::sPrepareTexture2D()
{
    FinalData = 0;
    FinalSize = 0;
    Original = 0;
    deleteoriginal = 0;
}

sPrepareTexture2D::~sPrepareTexture2D()
{
    delete FinalData;
    if(deleteoriginal)
        delete Original;
}

void sPrepareTexture2D::Load(uint *data,int sx,int sy)
{
    sFImage *fi = new sFImage(sx,sy);
    fi->Set8Bit(data);
    Load(fi);
    deleteoriginal = 1;
}

void sPrepareTexture2D::Load(sImage *img)
{
    sFImage *fi = new sFImage();
    fi->CopyFrom(img);
    Load(fi);
    deleteoriginal = 1;
}

void sPrepareTexture2D::Load(sFImage *img)
{
    if(deleteoriginal)
        sDelete(Original);
    deleteoriginal = 0;
    Original = img;
    SizeX = img->GetSizeX();
    SizeY = img->GetSizeY();

    sResPara rp;
    rp.Mode = sRBM_Shader | sRM_Texture;
    rp.SizeX = SizeX;
    rp.SizeY = SizeY;
    rp.Format = Format;
    Mipmaps = rp.GetMaxMipmaps();
    Format = sRF_BGRA8888;
    InputFlags = 0;
    OutputFlags = 0;
    FirstFilter = SecondFilter = sIDF_Box;
    FirstMipmaps = Mipmaps;
}

sResource *sPrepareTexture2D::GetTexture(sAdapter *ada)
{
    return new sResource(ada,sResTexPara(Format,SizeX,SizeY,Mipmaps),GetData(),GetSize());
}

sImageData *sPrepareTexture2D::GetImageData()
{
    sImageData *img = new sImageData();
    img->Init2D(Mipmaps,Format,SizeX,SizeY);
    sASSERT(img->Size==FinalSize);
    sCopyMem(img->Data,FinalData,FinalSize);
    return img;
}

/****************************************************************************/

void sPrepareTexture2D::Process()
{
    sFImage *m0 = new sFImage;
    sFImage *m1 = new sFImage;
    Mipmaps = sCalcMipmapCount(SizeX,SizeY,0,Mipmaps,Format);

    // allocate dest buffer

    uptr size = uptr(sGetBitsPerPixel(Format))*SizeX*SizeY/8;
    FinalSize = 0;
    if(FinalData)
        delete[] FinalData;
    for(int i=0;i<Mipmaps;i++)
        FinalSize += size >> (i*2);
    FinalData = new uint8[FinalSize];
    uint8 *dest = FinalData;

    // first mipmap

    if(SizeX!=Original->GetSizeX() || SizeY!=Original->GetSizeY())
        m0->Downsample(Original,SizeX,SizeY,0<FirstMipmaps ? FirstFilter : SecondFilter);
    else
        m0->Copy(Original);

    bool tiling = (OutputFlags & sPTF_Tiling) ? 1 : 0;
    dest += m0->PackPixels(dest,Format,tiling);

    // more mipmaps

    for(int level=1;level<Mipmaps;level++)
    {
        m1->Half(m0,level<FirstMipmaps ? FirstFilter : SecondFilter);
        sSwap(m0,m1);
        dest += m0->PackPixels(dest,Format,tiling);
    }

    // done

    sASSERT(dest==FinalData+FinalSize);

    delete m0;
    delete m1;
}

/****************************************************************************/
}
 
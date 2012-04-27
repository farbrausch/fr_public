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

#include "base/types.hpp"
#if sRENDERER == sRENDER_BLANK

#if sCONFIG_COMPILER_MSC
#undef new
#endif

#if sCONFIG_SYSTEM_WINDOWS
#include <windows.h>
#endif

#if sCONFIG_COMPILER_MSC
#define new sDEFINE_NEW
#else
#endif

#include "base/system.hpp"
#include "base/math.hpp"
#include "base/graphics.hpp"
#include "base/windows.hpp"

void sInitGfxCommon();
void sExitGfxCommon();

#if sCONFIG_SYSTEM_WINDOWS
extern HWND sHWND;
extern HDC sGDIDC;
#endif

static sU8 *DummyBuffer;
static sDInt DummySize;

static sU8 *GrowDummyBuffer(sInt newsize)
{
  if(DummySize<newsize)
  {
    delete[] DummyBuffer;
    DummyBuffer = new sU8[newsize];
    DummySize = newsize;
  }
  return DummyBuffer;
}

static sScreenMode DXScreenMode;

/****************************************************************************/
/***                                                                      ***/
/***   Vertex Formats                                                     ***/
/***                                                                      ***/
/****************************************************************************/

void sVertexFormatHandle::Create()
{
  sInt i,b[sVF_STREAMMAX];
  sInt stream;

  for(i=0;i<sVF_STREAMMAX;i++)
    b[i] = 0;

  i = 0;
  while(Data[i])
  {
    stream = (Data[i]&sVF_STREAMMASK)>>sVF_STREAMSHIFT;

    sVERIFY(i<31);

    switch(Data[i]&sVF_TYPEMASK)
    {
    case sVF_F2:  b[stream]+=2*4;  break;
    case sVF_F3:  b[stream]+=3*4;  break;
    case sVF_F4:  b[stream]+=4*4;  break;
    case sVF_I4:  b[stream]+=1*4;  break;
    case sVF_C4:  b[stream]+=1*4;  break;
    case sVF_S2:  b[stream]+=1*4;  break;
    case sVF_S4:  b[stream]+=2*4;  break;
    case sVF_H2:  b[stream]+=1*4;  break;
    case sVF_H3:  b[stream]+=3*2;  break;
    case sVF_H4:  b[stream]+=2*4;  break;
    case sVF_F1:  b[stream]+=1*4;  break;
    default: sVERIFYFALSE;
    }

    AvailMask |= 1 << (Data[i]&sVF_USEMASK);
    Streams = sMax(Streams,stream+1);
    i++;
  }

  for(sInt i=0;i<sVF_STREAMMAX;i++)
    VertexSize[i] = b[i];
}

void sVertexFormatHandle::Destroy()
{
}

/****************************************************************************/
/***                                                                      ***/
/***   sGeometry                                                          ***/
/***                                                                      ***/
/****************************************************************************/


sGeoBufferPart::sGeoBufferPart()
{
  Start = 0;
  Count = 0;
  Buffer = 0;
}

sGeoBufferPart::~sGeoBufferPart()
{
}

void sGeoBufferPart::Clear()
{
}

void sGeoBufferPart::Init(sInt count,sInt size,sGeometryDuration duration,sInt buffertype)
{
  Start = 0;
  Count = count;
  Buffer = 0;
  GrowDummyBuffer(count*size);
}

void sGeoBufferPart::Lock(void **ptr)
{
  *ptr = GrowDummyBuffer(0);
}

void sGeoBufferPart::Unlock(sInt count,sInt size)
{
}

/****************************************************************************/

void sGeometry::BeginWireGrid(void **data,sInt xs,sInt ys)
{
}

void sGeometry::EndWireGrid()
{
}

void sGeometry::BeginQuadrics()
{
}

void sGeometry::EndQuadrics()
{
}

void sGeometry::BeginQuad(void **data,sInt count)
{
  *data = GrowDummyBuffer(Format->GetSize(0)*count*4);
}

void sGeometry::EndQuad()
{
}

void sGeometry::BeginGrid(void **data,sInt xs,sInt ys)
{
  *data = GrowDummyBuffer(Format->GetSize(0)*xs*ys);
}

void sGeometry::EndGrid()
{
}

/****************************************************************************/

void sGeometry::Draw()
{
}

void sGeometry::Draw(const sGeometryDrawInfo &di)
{
}

void sGeometry::Draw(sDrawRange *ir,sInt irc,sInt instancecount,sVertexOffset *)
{
}

/****************************************************************************/

sCBufferBase::sCBufferBase()
{
  DataPtr = 0;
  DataPersist = 0;
}

void sCBufferBase::SetPtr(void **dataptr,void *data)
{
  DataPtr = dataptr;
  DataPersist = data;
  if(DataPtr)
    *DataPtr = DataPersist;
}

void sCBufferBase::SetRegs()
{
}

sCBufferBase::~sCBufferBase()
{
}

/****************************************************************************/
/***                                                                      ***/
/***   Viewport                                                           ***/
/***                                                                      ***/
/****************************************************************************/


/****************************************************************************/
/***                                                                      ***/
/***   sOccQuery                                                          ***/
/***                                                                      ***/
/****************************************************************************/

sOccQuery::sOccQuery()
{
  Last = 1.0f;
  Average = 1.0f;
  Filter = 0.1f;
}

sOccQuery::~sOccQuery()
{
}

void sOccQuery::Begin(sInt pixels)
{
}

void sOccQuery::End()
{
}

void sOccQuery::Poll()
{
}

/****************************************************************************/
/***                                                                      ***/
/***   Texture                                                            ***/
/***                                                                      ***/
/****************************************************************************/

sU64 sGetAvailTextureFormats()
{
  return ~0;
}

sBool sReadTexture(sReader &s, sTextureBase *&tex)
{
  return sFALSE;
}

/****************************************************************************/

void sTexture2D::Create2(sInt flags)
{
}

void sTexture2D::Destroy2()
{
}

void sTexture2D::BeginLoad(sU8 *&data,sInt &pitch,sInt mipmap)
{
  sInt xs = SizeX;
  sInt ys = SizeY;
  pitch = xs*BitsPerPixel/8;
  switch(Flags & sTEX_FORMAT)
  {
  case sTEX_DXT1:
  case sTEX_DXT1A:
    xs /= 4;
    ys /= 4;
    pitch = 8*xs;
    break;
  case sTEX_DXT3:
  case sTEX_DXT5:
  case sTEX_DXT5N:
  case sTEX_DXT5_AYCOCG:
    xs /= 4;
    ys /= 4;
    pitch = 16*xs;
    break;
  }
  xs >>= mipmap;
  ys >>= mipmap;
  pitch >>= mipmap;

  data = GrowDummyBuffer(pitch*ys);
}

void sTexture2D::BeginLoadPartial(const sRect &rect,sU8 *&data,sInt &pitch,sInt mipmap)
{
  BeginLoad(data,pitch,mipmap);
}

void sTexture2D::EndLoad()
{
}

void *sTexture2D::BeginLoadPalette()
{
  sFatal(L"paletted textures not supported");
  return 0;
}

void sTexture2D::EndLoadPalette()
{
}

void sTexture2D::CalcOneMiplevel(const sRect &rect)
{
}

/****************************************************************************/

void sTextureCube::Create2(sInt flags)
{
}

void sTextureCube::Destroy2()
{
}

void sTextureCube::BeginLoad(sTexCubeFace cf, sU8*& data, sInt& pitch, sInt mipmap)
{
  data = GrowDummyBuffer(BitsPerPixel*SizeXY);
  pitch = 0;
}

void sTextureCube::EndLoad()
{
}

/****************************************************************************/

void sTextureProxy::Connect2()
{
}

void sTextureProxy::Disconnect2()
{
}

/****************************************************************************/

void sPackDXT(sU8 *d,sU32 *bmp,sInt xs,sInt ys,sInt format,sBool dither)
{
  sFastPackDXT(d,bmp,xs,ys,format & sTEX_FORMAT,1 | (dither ? 0x80 : 0));
}

/****************************************************************************/
/***                                                                      ***/
/***   Material                                                           ***/
/***                                                                      ***/
/****************************************************************************/

void sMaterial::Create2()
{
}

void sMaterial::Destroy2()
{
}

void sMaterial::Set(sCBufferBase **cbuffers,sInt cbcount,sBool additive)
{
}

void sMaterial::Prepare(sVertexFormatHandle *)
{
}

void sMaterial::SetVariant(sInt var)
{
}

void sMaterial::InitVariants(sInt var)
{
}

void sMaterial::DiscardVariants()
{
}

void sMaterial::SetVariantRS(sInt var, const sMaterialRS &rs)
{
}

/****************************************************************************/
/***                                                                      ***/
/***   Shaders                                                            ***/
/***                                                                      ***/
/****************************************************************************/

sShaderTypeFlag sGetShaderPlatform()
{
  return sSTF_NONE;
}

sInt sGetShaderProfile()
{
  return sSTF_NONE;
}

void sSetVSParam(sInt o, sInt count, const sVector4* vsf)
{
}

void sSetPSParam(sInt o, sInt count, const sVector4* psf)
{
}

void sSetVSBool(sU32 bits,sU32 mask)
{
}

void sSetPSBool(sU32 bits,sU32 mask)
{
}

void sCreateShader2(sShader *shader, sShaderBlob *blob)
{
}

void sDeleteShader2(sShader *shader)
{
}

/****************************************************************************/
/***                                                                      ***/
/***   Initialisation                                                     ***/
/***                                                                      ***/
/****************************************************************************/

sBool sSetOversizeScreen(sInt xs,sInt ys,sInt fsaa,sBool mayfail)
{
  return sFALSE;
}

void sGetScreenSafeArea(sF32 &xs, sF32 &ys)
{
#if sCONFIG_BUILD_DEBUG || sCONFIG_BUILD_DEBUGFAST
  xs = 0.95f;
  ys = 0.9f;
#else
  xs=ys=1;
#endif
}

void PreInitGFX(sInt &flags,sInt &xs,sInt &ys)
{
  if(!(DXScreenMode.Flags & sSM_VALID))
  {
    DXScreenMode.Flags = sSM_VALID|sSM_STENCIL;
    if(flags & sISF_FULLSCREEN)   DXScreenMode.Flags |= sSM_FULLSCREEN;
    if(flags & sISF_NOVSYNC)      DXScreenMode.Flags |= sSM_NOVSYNC;
    if(flags & sISF_FSAA)         DXScreenMode.MultiLevel = 255;
    if(flags & sISF_REFRAST)      DXScreenMode.Flags |= sSM_REFRAST;
    DXScreenMode.ScreenX = xs;
    DXScreenMode.ScreenY = ys;
    DXScreenMode.Aspect = sF32(DXScreenMode.ScreenX) / sF32(DXScreenMode.ScreenY);
  }
  else
  {
    xs = DXScreenMode.ScreenX;
    ys = DXScreenMode.ScreenY;
    if(DXScreenMode.Aspect==0)
      DXScreenMode.Aspect = sF32(DXScreenMode.ScreenX) / sF32(DXScreenMode.ScreenY);
  }
}

void InitGFX(sInt flags,sInt xs,sInt ys)
{
  GrowDummyBuffer(1024);
  DXScreenMode.ScreenX = xs;
  DXScreenMode.ScreenY = ys;
  DXScreenMode.Aspect = sF32(DXScreenMode.ScreenX) / sF32(DXScreenMode.ScreenY);
  sInitGfxCommon();
}

void ExitGFX()
{
  sExitGfxCommon();
  sDeleteArray(DummyBuffer);
  DummySize = 0;
}

void ResizeGFX(sInt xs,sInt ys)
{
  DXScreenMode.ScreenX = xs;
  DXScreenMode.ScreenY = ys;
}

/****************************************************************************/
/***                                                                      ***/
/***   ScreenModes                                                        ***/
/***                                                                      ***/
/****************************************************************************/

sInt sGetDisplayCount()
{
  return 1;
}

void sGetScreenInfo(sScreenInfo &si,sInt flags,sInt display)
{
  si.Clear();
  si.Resolutions.HintSize(1);
  si.RefreshRates.HintSize(1);
  si.AspectRatios.HintSize(1);
  si.Resolutions.AddMany(1);
  si.RefreshRates.AddMany(1);
  si.AspectRatios.AddMany(1);

  si.Resolutions[0].x = 800;
  si.Resolutions[0].y = 600;
  si.RefreshRates[0] = 60;
  si.AspectRatios[0].x = 4;
  si.AspectRatios[0].y = 3;
  si.CurrentAspect = sF32(DXScreenMode.ScreenX)/DXScreenMode.ScreenY;
  si.CurrentXSize = DXScreenMode.ScreenX;
  si.CurrentYSize = DXScreenMode.ScreenY;
}

void sGetScreenMode(sScreenMode &sm)
{
  sm = DXScreenMode;
}

sBool sSetScreenMode(const sScreenMode &sm)
{
  DXScreenMode = sm;
  return 1;
}

/****************************************************************************/
/***                                                                      ***/
/***   Rendering                                                          ***/
/***                                                                      ***/
/****************************************************************************/

void sGrabScreen(class sTexture2D *, sGrabFilterFlags filter, const sRect *dst, const sRect *src)
{
}

void sSetScreen(sTexture2D *, sGrabFilterFlags filter, const sRect *dst, const sRect *src)
{
}

void sSetRendertarget(const sRect *vrp, sInt clearflags, sU32 clearcolor)
{
}

void sSetRendertarget(const sRect *vrp,sTexture2D *tex, sInt flags, sU32 clearcolor)
{
}

void sSetRendertarget(const sRect *vrp,sInt flags,sU32 clearcolor,sTexture2D **tex,sInt count)
{
}

void sSetRendertargetCube(sTextureCube*,sTexCubeFace,sInt cf, sU32 cc)
{
}

sTexture2D *sGetCurrentFrontBuffer() { return 0; }
sTexture2D *sGetCurrentBackBuffer() { return 0; }
sTexture2D *sGetCurrentBackZBuffer() { return 0; }

void sBeginSaveRT(const sU8 *&data, sS32 &pitch, enum sTextureFlags &flags)
{
  sVERIFYFALSE;
}

void sEndSaveRT()
{
}

void sBeginReadTexture(const sU8*& data, sS32& pitch, enum sTextureFlags& flags,sTexture2D *tex)
{
  sVERIFYFALSE;
}

void sEndReadTexture()
{
}

void sRender3DEnd(sBool flip)
{
  sPreFlipHook->Call();
  sFlipMem();
  sPostFlipHook->Call();
}

void sRender3DFlush()
{
}

sBool sRender3DBegin()
{
#if sCONFIG_SYSTEM_WINDOWS
  PAINTSTRUCT ps;
  HBRUSH brush;
  InvalidateRect(sHWND,0,0);
  BeginPaint(sHWND,&ps);
  sGDIDC = ps.hdc;
  sRect client;
  GetClientRect(sHWND,(RECT *) &client);

  brush = CreateSolidBrush(0x000000);
  FillRect(sGDIDC,(const RECT *) &client,brush);
  DeleteObject(brush);


  EndPaint(sHWND,&ps);
#endif
  return 1;
}

void sSetRenderClipping(sRect *r,sInt count)
{
}

void sSetTexture(sInt stage,class sTextureBase *tex)
{
}

void sSetRenderStates(const sU32 *data, sInt count)
{
}

sInt sRenderStateTexture(sU32 *data, sInt texstage, sU32 tflags,sF32 lodbias)
{
  return 0;
}

void sEnableGraphicsStats(sBool enable)
{
}

void sGetGraphicsStats(sGraphicsStats &stat)
{
  sClear(stat);
}

void sSetDesiredFrameRate(sF32 rate)
{
}

void sSetScreenGamma(sF32 gamma)
{
}

void sGetGraphicsCaps(sGraphicsCaps &caps)
{
  sClear(caps);
}

/****************************************************************************/

#endif // sRENDERER == sRENDER_BLANK

/****************************************************************************/


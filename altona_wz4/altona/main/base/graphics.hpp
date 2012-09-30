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


#ifndef HEADER_ALTONA_UTIL_GRAPHICS
#define HEADER_ALTONA_UTIL_GRAPHICS

#ifndef __GNUC__
#pragma once
#endif

#include "base/types.hpp"
#include "base/math.hpp"

/****************************************************************************/

struct sGeoBuffer;
struct sGeoBufferPart;
struct sGeoPrim;
class sTextureBase;
class sTexture2D;
class sTexture3D;
class sTextureCube;
class sShader;                              // some kind of shader (pixel, vertex, geometry)
class sLinkedShader;                        // not yet implemented, for OpenGL ShaderObject. optional parameter to sSetShaders! 
struct sTargetSpec;
class sCBufferBase;
struct sShaderBlob;
class sCSBuffer;
  

/****************************************************************************/

// some definitions needed by the platform specific stuff

enum
{
#if sRENDERER==sRENDER_DX11
  sCBUFFER_MAXSLOT = 32,
  sCBUFFER_SHADERTYPES = 6,
  sCBUFFER_SHADERMASK = sCBUFFER_MAXSLOT*7,

#else
  // we only have pixel and vertex shader
  sCBUFFER_MAXSLOT = 32,
  sCBUFFER_SHADERTYPES = 2,
  sCBUFFER_SHADERMASK = sCBUFFER_MAXSLOT*7,
#endif

  sCBUFFER_VS = sCBUFFER_MAXSLOT*0,
  sCBUFFER_PS = sCBUFFER_MAXSLOT*1,
  // dx11
  sCBUFFER_GS = sCBUFFER_MAXSLOT*2,
  sCBUFFER_HS = sCBUFFER_MAXSLOT*3,
  sCBUFFER_DS = sCBUFFER_MAXSLOT*4,
  sCBUFFER_CS = sCBUFFER_MAXSLOT*5,
};


// the new way to insert platform specific members

#if sRENDERER==sRENDER_DX11
  #include "graphics_dx11_private.hpp"
#elif sRENDERER==sRENDER_DX9
  #include "graphics_dx9_private.hpp"
#elif sRENDERER==sRENDER_OGLES2
  #include "graphics_ogles2_private.hpp"
#else                                 // dummies for platforms that use the old way

  enum
  {
    sMTRL_MAXTEX = 16,                                  // max number of textures
    sMTRL_MAXPSTEX = 16,                                // max number of pixel shader samplers
    sMTRL_MAXVSTEX = 0,
  };

  class sGeometryPrivate {};
  struct sVertexFormatHandlePrivate {};
  class sTextureBasePrivate {};
  class sShaderPrivate {};
  class sMaterialPrivate {};
  class sOccQueryPrivate {};
  class sGfxThreadContextPrivate {};
  class sGpuToCpuPrivate {};
  class sCBufferBasePrivate 
  {
  public:
    void *DataPersist;
    void **DataPtr;
    sU64 Mask;                                // used register mask
  };
#endif

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Graph3D - interface to 3d hardware                                 ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

void sSetFullscreen(sBool enable);
sBool sGetFullscreen();

void sSetScreenResolution(sInt resX, sInt resY);
void sSetScreenResolution(sInt resX, sInt resY, sF32 aspectRatio);
void sGetScreenResolution(sInt &resX, sInt &resY);

sBool sSetOversizeScreen(sInt xs,sInt ys,sInt fsaa=0,sBool mayfail=0); // should be called BEFORE sInit() xs=ys=0 to disable
void sGetScreenSize(sInt &xs,sInt &ys);
sF32 sGetScreenAspect();
void sGetScreenSafeArea(sF32 &xs, sF32 &ys); // returns values between 0 (center) and 1 (outside)

void sSetDesiredFrameRate(sF32 rate);
void sSetScreenGamma(sF32 gamma);       // only applied in fullscreen mode

void sDbgPaintWireFrame(sBool enable);    // enable/disable wireframe fillmode for debugging

/****************************************************************************/
/***                                                                      ***/
/***   Forwards...                                                        ***/
/***                                                                      ***/
/****************************************************************************/

enum sTextureFlags 
{
  sTEX_UNKNOWN      =  0,         // choose a format.
  sTEX_ARGB8888     =  1,         // standard 32 bit texture
  sTEX_QWVU8888     =  2,         // interpreted as signed number, for bumpmaps

  sTEX_GR16         =  3,
  sTEX_ARGB16       =  4,

  sTEX_R32F         =  5,        // floating point textures
  sTEX_GR32F        =  6,
  sTEX_ARGB32F      =  7,
  sTEX_R16F         =  8,
  sTEX_GR16F        =  9,
  sTEX_ARGB16F      = 10,

  sTEX_A8           = 11,         // alpha only
  sTEX_I8           = 12,         // Intensity only (red=green=blue= 8bit)

  sTEX_DXT1         = 13,         // dxt1 without alpha
  sTEX_DXT1A        = 14,         // dxt1 with one bit alpha
  sTEX_DXT3         = 15,         // dxt3: direct 4 bit alpha
  sTEX_DXT5         = 16,         // dxt5: interpolated 3 bit alpha

  sTEX_INDEX4       = 17,
  sTEX_INDEX8       = 18,

  sTEX_MRGB8        = 19,         // 8bit scaled: HDR=M*RGB 
  sTEX_MRGB16       = 20,         // 16bit scaled: HDR=M*RGB 

  sTEX_ARGB2101010  = 21,
  sTEX_D24S8        = 22,         // depth stencil texture

  sTEX_I4           = 23,         // intensity only
  sTEX_IA4          = 24,         // intensity+alpha
  sTEX_IA8          = 25,         // intensity+alpha
  sTEX_RGB5A3       = 26,         // ARGB, depending on MSB, either 0:5:5:5 or 3:4:4:4
  sTEX_ARGB1555     = 27,         // 16 bit format
  sTEX_RGB565       = 28,
  sTEX_DXT5N        = 29,         // DXT5 for normal compression: rgba = 0g0r
  sTEX_GR8          = 30,         // green/red as 8bit (for delta-st maps)
  sTEX_ARGB4444     = 31,         // AMIGA!

  sTEX_DEPTH16NOREAD= 32,         // depth buffer, can not be read
  sTEX_DEPTH24NOREAD= 33,
  sTEX_PCF16        = 34,         // depth buffer, texture read as PCF filtered (directx9 only)
  sTEX_PCF24        = 35,
  sTEX_DXT5_AYCOCG  = 36,         // YCoCg with additional alpha DXT5 compressed (with nasty color shifts which could be reduced with YCoCg without additional alpha)
  sTEX_DEPTH16      = 37,         // depth buffer, can be bound as texture to read
  sTEX_DEPTH24      = 38,

  sTEX_8TOIA        = 39,         // single 8bit channel that gets replicated to RGBA

  sTEX_STRUCTURED   = 40,         // compute shader: structured buffer. 
  sTEX_RAW          = 41,         // compute shader: raw (byte addressed) buffer
  sTEX_UINT32       = 42,         // another format useful for compute shaders.

                                  // don't use more than 64 texture formats, some tools use an sU64 bitmask to store sets of texture formats.

  sTEX_FORMAT       = 0x000000ff, // mask for format field
  sTEX_DYNAMIC      = 0x00000100, // you want to update the texture regulary by BeginLoad() / EndLoad()
                                  // may need more memory on consoles, use sTEX_STREAM for textures which are updated every frame
  sTEX_RENDERTARGET = 0x00000200, // you want to use the texture as a rendertarget.
  sTEX_NOMIPMAPS    = 0x00000400, // no mipmaps
  sTEX_MAIN_DEPTH   = 0x00000800, // use main render target depth stencil buffer
  sTEX_NORMALIZE    = 0x00010000, // enable renomalization durng mipmap generation - useful for bumpmaps!
  sTEX_SCREENSIZE   = 0x00020000, // for rendertargets: use size of screen, update when resizing, use SizeX & SizeY as shift to create half / qarter screens.
  sTEX_SOFTWARE     = 0x00040000, // flagging sTextureSoftware type
  sTEX_SWIZZLED     = 0x00080000, // hardware dependent swizzled texture
  sTEX_PROXY        = 0x00100000, // flagging sTextureProxy type
  sTEX_AUTOMIPMAP   = 0x00200000, // generate mipmaps for rendertarget
  sTEX_STREAM       = 0x00400000, // texture is update every frame
  sTEX_FASTDXTC     = 0x00800000, // texture uses fast dxt compression

  sTEX_TILED_RT     = 0x01000000, // only render/resolve the target rectangle not the complete texture

  sTEX_INTERNAL     = 0x10000000, // used by system to initialize texture structures for backbuffer and zbuffers

  sTEX_MSAA         = 0x20000000, // multisample anti aliasing (for rendertargets)
  sTEX_NORESOLVE    = 0x40000000, // have multisampled and non multisampled buffers, but do not actually resolve (used for zbuffers)
  sTEX_AUTOMIPMAP_POINT = 0x80000000, // in addition to the automipmap flag: use point sampling, not best sampling

  // texture types

  sTEX_2D           = 0x00001000, // texture type. 
  sTEX_CUBE         = 0x00002000,
  sTEX_3D           = 0x00003000,
  sTEX_BUFFER       = 0x00004000, // compute shader: buffer
  sTEX_TYPE_MASK    = 0x0000f000, // must be within lower 16 bit for sImgBlobExtract::Serialize_

  // running out of bits - recycling console specific flags for DX11 compute shader features

  sTEX_CS_INDEX     = 0x00100000, // Will be used as an index buffer
  sTEX_CS_VERTEX    = 0x01000000, // Will be used as a vertex buffer
  sTEX_CS_WRITE     = 0x02000000, // create compute shader unordered access view for this texture
  sTEX_CS_COUNT     = 0x04000000, // add counter
  sTEX_CS_STACK     = 0x08000000, // allow append / consume
  sTEX_CS_INDIRECT  = 0x00080000, // can be used as draw indirect buffer
};


sInt sGetBitsPerPixel(sInt format);
sU64 sGetAvailTextureFormats();   // bitmask of available texture formats on this hardware

enum sTexCubeFace
{
  sTCF_POSX   = 0,
  sTCF_NEGX,
  sTCF_POSY,
  sTCF_NEGY,
  sTCF_POSZ,
  sTCF_NEGZ,

  sTCF_NONE   = 0xffffffff,
};


sBool sIsBlockCompression(sInt texflags);
sU32 sAYCoCgtoARGB(sU32 val);
sU32 sARGBtoAYCoCg(sU32 val);

/****************************************************************************/
/***                                                                      ***/
/***   General Engine Interface                                           ***/
/***                                                                      ***/
/****************************************************************************/

sBool sRender3DBegin();       // use Render3DBegin(); ..rendercommands .. Render3DEnd();
void sRender3DEnd(sBool flip=1);          // for rendering without sApp (like simple tools)
                              // otherwise render in sApp::OnPaint3D implementation
                              // don't mix sApp::OnPaint3D and sRender3DBegin/sRender3DEnd
void sRender3DFlush();        // called AFTER sRender3DEnd, makes sure that the DMA-chain finishes
                              // this does not flip the framebuffer, you may need to draw the image multiple times.
void sRender3DLock();         // multithreading, if supported, thread who calls InitGFX owns lock by default
void sRender3DUnlock();
sBool sRender3DLockOwner();

sINLINE void sRender3DSuspend() {}
sINLINE void sRender3DResume() {}

sINLINE void sRender3DLock() {}
sINLINE void sRender3DUnlock() {}
sINLINE sBool sRender3DLockOwner() { return sTRUE; }


enum sFrameRateMode
{
  sFRM_FREE,            // (default) always swap, don't force any frame rate
  sFRM_KEEP_SECOND,     // force second frame when we drop below first frame
  sFRM_FORCE_SECOND,    // always force second frame
};

void sSetFrameRateMode(sFrameRateMode mode);    // resets the current frame mode

void sClearCurrentCBuffers(); // internally called to invalidate constant buffer caching

extern sHooks *sPreFlipHook;  // called exactly before next frame is displayed
extern sHooks *sPostFlipHook;  // called exactly after next frame is displayed
extern sHooks *sGraphicsLostHook; // graphic card was lost, reload sGD_DYNAMIC

/****************************************************************************/

// new rendertarget interface

enum sSetTargetFlags
{
  sST_CLEARNONE       = 0x0000,
  sST_CLEARCOLOR      = 0x0001,
  sST_CLEARDEPTH      = 0x0002,
  sST_CLEARALL        = 0x0003,   // clearing implies sST_OVERWRITEALL

  sST_NOMSAA          = 0x0010,   // use the non-multisampled buffer for rendering.
  sST_SCISSOR         = 0x0020,   // use scissor for clearing, if supported
  sST_READZ           = 0x0040,   // you intend to actually use the zbuffer as texture later on, it can not be discarded!
  sST_OVERWRITEALL    = 0x0080,   // you will overwrite every pixel of the buffers, even if you do not clear.
  sST_NOMIPMAPGEN     = 0x0100,   // don't generate mipmaps for targets with sTEX_AUTOMIPMAP (usefull if you render into same texture with mulitple passes and switchin targets between passes)
};

struct sTargetSpec          // a little helper 
{
  union                           // the texture that you want to render to or 0 for screen
  {
    sTexture2D *Color2D;
    sTextureCube *ColorCube;
    sTextureBase *Color;
  };
  sTexture2D *Depth;              // the depth-buffer or 0 for screen
  sInt Cubeface;                  // set to -1 for cubefaces
  sRect Window;                   // always initialize
  sF32 Aspect;                    // ys/xs

  sTargetSpec();
  sTargetSpec(const sRect &r);
  sTargetSpec(sTexture2D *color,sTexture2D *depth);
  sTargetSpec(sTextureCube *color,sTexture2D *depth,sInt face);
  void Init();
  void Init(const sRect &r);
  void Init(sTexture2D *tex,sTexture2D *depth);
  void Init(sTextureCube *tex,sTexture2D *depth,sInt face);
};

struct sTargetPara
{
  sInt Flags;
  sVector4 ClearColor[4];
  sF32 ClearZ;
  sRect Window;                   // must be initialized, do not leave at 0
  sInt Cubeface;                  // set to -1 for 2d textures
  sInt Mipmap;                    // leave at 0 for largest mipmap
  sF32 Aspect;                    // must be initialized, by xs/ys for instance

  sTextureBase *Depth;            // depth buffer
  sTextureBase *Target[4];        // color buffer (or whatever)

  sTargetPara();
  sTargetPara(sInt flags,sU32 clearcol,const sTargetSpec &);
  explicit sTargetPara(sInt flags,sU32 clearcol=0,const sRect *window=0);
  sTargetPara(sInt flags,sU32 clearcol,const sRect *window,sTextureBase *colorbuffer,sTextureBase *depthbuffer=0L);
  void Init();
  void Init(sInt flags,sU32 clearcol,const sTargetSpec &);
  void Init(sInt flags,sU32 clearcol=0,const sRect *window=0);
  void Init(sInt flags,sU32 clearcol,const sRect *window,sTextureBase *colorbuffer,sTextureBase *depthbuffer);
  void SetTarget(sInt i, sTextureBase *tex, sU32 clearcol);
};

enum sCopyTextureFlags
{
  sCT_FILTER      = 0x0001,         // use linear filtering if available
  sCT_NOMIPMAPGEN = 0x0002,         // don't automatically generate mipmaps (use this if you render into copied texture anyway)
  sCT_MARKRESOLVED= 0x0004,         // indicate, that the original msaa buffer is no longer used for rendering in this frame (if we copy from it)
};

struct sCopyTexturePara
{
  sInt Flags;

  sTextureBase *Source;
  sRect SourceRect;
  sInt SourceCubeface;

  sTextureBase *Dest;
  sRect DestRect;
  sInt DestCubeface;

  sCopyTexturePara();
  sCopyTexturePara(sInt flags,sTextureBase *d,sTextureBase *s);
};
 
void sSetTarget(const sTargetPara &para);
void sSetScissor(const sRect &r);
void sResolveTarget();
void sResolveTexture(sTextureBase *tex);
void sCopyTexture(const sCopyTexturePara &para);

sTexture2D *sGetScreenColorBuffer(sInt screen=0);
sTexture2D *sGetScreenDepthBuffer(sInt screen=0);
sTexture2D *sGetRTDepthBuffer();
sInt sGetScreenCount();
void sEnlargeRTDepthBuffer(sInt x, sInt y);

/****************************************************************************/

// this method of reading textures is slow because of blocking

void sBeginReadTexture(const sU8 *&data, sS32 &pitch, enum sTextureFlags &flags,sTexture2D *tex);   // returns sTEX_??? bitmap type flag
void sEndReadTexture();

// prefer using this one:

class sGpuToCpu : public sGpuToCpuPrivate
{
public:
  sGpuToCpu(sInt flags,sInt xs,sInt ys);
  ~sGpuToCpu();

  void CopyFrom(sTexture2D *,sInt miplevel=0);
  const void *BeginRead(sDInt &pitch);
  void EndRead();
};

/****************************************************************************/

// some of these are now obsolete. will sort this out later

void sCreateZBufferRT(sInt x, sInt y);
void sEnlargeZBufferRT(sInt x, sInt y);
void sDestroyZBufferRT();

enum sSOON_OBSOLETE sRTFlags
{
  // clear flags
  sCLEAR_NONE           = 0x0000,
  sCLEAR_COLOR          = 0x0001,
  sCLEAR_ZBUFFER        = 0x0002,
  sCLEAR_ALL            = sCLEAR_COLOR|sCLEAR_ZBUFFER,
  sCLEAR_NOSCISSOR      = 0x0004,   // always clear complete render target ignoring scissor rect

  // depth buffer flags
  sRTZBUF_DEFAULT       = 0x0000,
  sRTZBUF_FORCEMAIN     = 0x0010,
  sRTZBUF_NONE          = 0x0020,
  sRTZBUF_MASK          = 0x00f0,

  // mipmap selection
  sRTF_MIPMAP           = 0x0100,   // for selecting different mipmap levels: sRT_MIPMAP*mm
  sRTF_MIPMAP_MSK       = 0xff00,
  sRTF_MIPMAP_SHIFT     =      8,

  // other platform dependent flags
  sRTF_NO_MULTISAMPLING = 0x10000,  // don't use multisampling on multisampled render target (useful for ipp passes)
  sRTF_NO_RESOLVE       = 0x20000,  // skip resolve/endtiling
  sRTF_TILED_SURFACE    = 0x40000,  // alloc only surface needed for selected target size (equivalent to set sTEX_TILED_RT texture flag)
};

void sSOON_OBSOLETE sSetRendertarget(const sRect *vrp, sInt flags=sCLEAR_ALL, sU32 clearcolor=0);
void sSOON_OBSOLETE sSetRendertarget(const sRect *vrp, sTexture2D *tex,sInt flags=sCLEAR_ALL|sRTZBUF_DEFAULT, sU32 clearcolor=0);
void sSOON_OBSOLETE sSetRendertarget(const sRect *vrp,sInt flags,sU32 clearcolor,sTexture2D **tex,sInt count);
void sSOON_OBSOLETE sSetRendertargetCube(sTextureCube* tex, sTexCubeFace cf, sInt flags=sCLEAR_ALL, sU32 clearcolor=0);

// returns current front/backbuffer textures, only supported on some consoles
// returns 0 if unsupported
sTexture2D sSOON_OBSOLETE *sGetCurrentFrontBuffer();
sTexture2D sSOON_OBSOLETE *sGetCurrentBackBuffer();
sTexture2D sSOON_OBSOLETE *sGetCurrentBackZBuffer();

enum sGrabFilterFlags
{
  sGFF_NONE,
  sGFF_POINT,
  sGFF_LINEAR,
};

void sSOON_OBSOLETE sGrabScreen(sTexture2D *, sGrabFilterFlags filter, const sRect *dst, const sRect *src);
//sINLINE void sSOON_OBSOLETE sGrabScreen(sTexture2D *tex) { sGrabScreen(tex,sGFF_NONE,0,0); }
void sSOON_OBSOLETE sSetScreen(sTexture2D *, sGrabFilterFlags filter, const sRect *dst, const sRect *src);

void sSOON_OBSOLETE sSetScreen(const sRect &rect,sU32 *data);
void sSOON_OBSOLETE sGetRendertargetSize(sInt &dx,sInt &dy);
sF32 sSOON_OBSOLETE sGetRendertargetAspect();
void sSetRenderClipping(sRect *,sInt count);

void sPackDXT(sU8 *d,sU32 *s,sInt xs,sInt ys,sInt format,sBool dither=1);
sBool sIsFormatDXT(sInt format);

// returns ptr to rendertarget data
// very slow: use only if absolute necessary (screenshots)!!!
void sSOON_OBSOLETE sBeginSaveRT(const sU8 *&data, sS32 &pitch, enum sTextureFlags &flags);   // returns sTEX_??? bitmap type flag
void sSOON_OBSOLETE sEndSaveRT();

/*
void sSetTexture(sInt stage, class sTextureBase *tex);
void sSetRenderStates(const sU32 *data, sInt count);
sInt sRenderStateTexture(sU32 *data, sInt texstage, sU32 tflags,sF32 lodbias=0);
*/

void sOBSOLETE sConvertsRGBTex(sBool e);    // to globally switch sRGB convertion on/off (hdr <-> ldr rendering)
                                  // with sConvertsRGBTex(sTRUE), texture are converted depending on sMTF_SRGB flag
sBool sOBSOLETE sConvertsRGBTex();

/****************************************************************************/

#include "base/types2.hpp"

// new screenmode interface
// * this is PC-Only, not for consoles
// * may be called BEFORE and AFTER sInit()
// * when used, overrides sInit() parameters

enum                              // flags that modify capabilities
{
  sSM_FULLSCREEN        = 0x0001, // fullscreen is default
  sSM_COLOR16BIT        = 0x0002, // use 565 or 1555 instead of 8888
  sSM_DEPTH16BIT        = 0x0004, // use a 16 bit depth format
  sSM_DEPTHREAD         = 0x0008, // use a readable depth format
  sSM_STENCIL           = 0x0010, // stencil buffer required
  sSM_OVERSIZE          = 0x0020, // use oversize buffer. clearing this does not prevent the overbuffer from beeing allocated.
  sSM_REFRAST           = 0x0040, //
  sSM_NOVSYNC           = 0x0080, //
  sSM_READZ             = 0x0100, // allow reading zbuffer as texture. may slow down rendering...
  sSM_WARP              = 0x0200, // use quick software rendering (microsoft warp engine)
  sSM_MULTISCREEN       = 0x0400, // drive all displays of the device

  sSM_VALID             = 0x8000, // always set this flag.
};


struct sScreenMode
{
  sInt Flags;                     // sSM_??? flags
  sInt Display;                   // adapter number 0..n, -1 for default
  // screen
  sInt ScreenX;                   // screen size
  sInt ScreenY;
  sInt OverX;                     // renderbuffer size, may be larger than screen (or 0)
  sInt OverY;
  sF32 Aspect;                    // x/y of whole screen
  sInt MultiLevel;                // level 0..n, NOT number of samples! -1 is off
  sInt OverMultiLevel;            // multisampling for oversized buffer
  // rendertargets
  sInt RTZBufferX;                // depth buffer for offscreen rendertarget
  sInt RTZBufferY;
  sInt Frequency;                 // Refreshrate

  sScreenMode();
  void Clear();
};

struct sScreenInfoXY
{
  sInt x,y;
  void Init(sInt x_,sInt y_) { x=x_; y=y_; }
  sBool operator==(const sScreenInfoXY& a)const { return x==a.x && y==a.y; }
};

struct sScreenInfo
{
  sInt Flags;
  sInt Display;

  sInt CurrentXSize;              // resolution of desktop
  sInt CurrentYSize;
  sF32 CurrentAspect;             // aspect of desktop, as x/y

  sStaticArray<sScreenInfoXY> Resolutions;    // Available resolutions (not for windowed)
  sStaticArray<sInt> RefreshRates;            // Available refresh rates (not for windowed)
  sInt MultisampleLevels;               // Available multisampling-levels
  sStaticArray<sScreenInfoXY> AspectRatios;   // commonly used aspect ratios
  sString<64> MonitorName;

  sScreenInfo();
  void Clear();
};

sInt sGetDisplayCount();
void sGetScreenInfo(sScreenInfo &si,sInt flags=0,sInt display=-1);
void sGetScreenMode(sScreenMode &sm);
sBool sSetScreenMode(const sScreenMode &sm);

/****************************************************************************/

enum sGraphicsCapsFlags
{
  sGCF_DEPTHTEX_RAWZ = 0x0001,   // depth texture treated as rgba -> 24bit int values (GeForce >= 6)
  sGCF_DEPTHTEX_INTZ = 0x0002,   // depth texture lookup in shader returns single float value in "r" channel (GeForce >= 8)
  sGCF_DEPTHTEX_DF24 = 0x0004,   // same as INTZ (Radeon >= X1300)  
  sGCF_DEPTHTEX_MASK = 0x0007,
  sGCF_DEPTH_RESOLVE = 0x0008,   // hardware supports msaa resolve for depth rendertargets
};

struct sGraphicsCaps
{
  sU64 Texture2D;                 // bitmask of available textureformats
  sU64 TextureCube;
  sU64 VertexTex2D;               // bitmask of available vertex texture formats
  sU64 VertexTexCube;
  sU64 TextureRT;                 // bitmask for rendertarget textureformats
  sU32 Flags;                     // supported type of depth texture, ...
  sInt MaxMultisampleLevel;
  sInt ShaderProfile;             // sSTFP_??? | sSTF_???
  sF32 UVOffset;                  // for exact pixel addressing, offset uv's by this. either  0.0 (dx11) or  0.0 (dx9).  
  sF32 XYOffset;                  // for exact pixel addressing, offset pos  by this. either  0.0 (dx11) or -0.5 (dx9).  
  sString<64> AdapterName;        // friendly name for graphics adapter
};

void sGetGraphicsCaps(sGraphicsCaps &);

/****************************************************************************/

// statistics / performance-metering

struct sGraphicsStats
{
  sInt Batches;
  sInt Splitter;
  sInt Vertices;
  sInt Indices;
  sInt Primitives;
  sInt Clears;

  sInt DynBytes;                        // bytes on DynBuffer total
  sInt QuadricsBytes;                   // bytes on DynBuffer used for quadrics
  
  sInt TexChanges[sMTRL_MAXPSTEX+sMTRL_MAXVSTEX];
  sInt AllTexChanges;
  sInt VSChanges;
  sInt PSChanges;
  sInt RTChanges;
  sInt MtrlChanges;

  sDInt StaticTextureMem;               // bytes in static textures, not counting rendertargets
  sDInt StaticVertexMem;                // bytes in static vertex buffers

  sInt GeoBuffersActive;                // dx9: static geometry buffers
};

void sEnableGraphicsStats(sBool enable);  // disable stats for debug drawing, if you want. enabled is default
void sGetGraphicsStats(sGraphicsStats &stat);

/****************************************************************************/
/***                                                                      ***/
/***   Vertex Formats                                                     ***/
/***                                                                      ***/
/****************************************************************************/

enum sVertexFlags                 // experimental way of specifying vertex formats
{                                 // (x) <- minimum number of dimensions
  sVF_POSITION      = 0x0001,     // (3) position (always set)
  sVF_NORMAL        = 0x0002,     // (3) normal
  sVF_TANGENT       = 0x0003,     // (3) tangent
  sVF_BONEINDEX     = 0x0004,     // (4) 4 indices to matrices
  sVF_BONEWEIGHT    = 0x0005,     // (4) 4 weighting factors to matrices
  sVF_BINORMAL      = 0x0006,     // (3) binormal, for certain consoles only, do not use!

  sVF_COLOR0        = 0x0008,     // (4) color
  sVF_COLOR1        = 0x0009,     // (4) color
  sVF_COLOR2        = 0x000a,     // (4) color
  sVF_COLOR3        = 0x000b,     // (4) color
  sVF_UV0           = 0x000c,     // (2) texture uv's
  sVF_UV1           = 0x000d,     // (2) texture uv's
  sVF_UV2           = 0x000e,     // (2) texture uv's
  sVF_UV3           = 0x000f,     // (2) texture uv's
  sVF_UV4           = 0x0010,     // (2) texture uv's
  sVF_UV5           = 0x0011,     // (2) texture uv's
  sVF_UV6           = 0x0012,     // (2) texture uv's
  sVF_UV7           = 0x0013,     // (2) texture uv's
  sVF_NOP           = 0x001f,

  sVF_USEMASK       = 0x001f,     // these flags are also stored in a bitmask, so it's limited to 32.

  sVF_STREAM0       = 0x0000,     // stream descriptor
  sVF_STREAM1       = 0x0040,
  sVF_STREAM2       = 0x0080,
  sVF_STREAM3       = 0x00c0,
  sVF_STREAMMASK    = 0x00c0,     // reserve for 4 streams 
  sVF_STREAMMAX     = 4,          // max number of streams
  sVF_STREAMSHIFT   = 6,          // shift to extract sream

  // optional: specify datatype. if missing, defaults are provided.
  // always available
  sVF_F2            = 0x0100,     // 2 x sF32
  sVF_F3            = 0x0200,     // 3 x sF32
  sVF_F4            = 0x0300,     // 4 x sF32
  sVF_C4            = 0x0400,     // 4 x sU8, scaled to 0..1 range
  // sometimes available
  sVF_I4            = 0x0500,     // 4 x sU8, not scaled. for bone-indices
  sVF_S2            = 0x0600,     // 2 x sS16, scaled to -1..1 range
  sVF_S4            = 0x0700,     // 4 x sS16, scaled to -1..1 range
  sVF_H2            = 0x0800,     // 2 x sF16
  sVF_H4            = 0x0900,     // 4 x sF16
  sVF_F1            = 0x0a00,     // 1 x sF32

  sVF_H3            = 0x0b00,     // 3 x sF16 (for optimized 3-vectors)
  //                  0x0c00,     // unused
  sVF_SN_11_11_10   = 0x0d00,     // Normalized, 3D signed 11 11 10 format expanded to (value/1023.0, value/1023.0, value/511.0, 1)

  sVF_TYPEMASK      = 0xff00,

  sVF_INSTANCEDATA  = 0x00010000, // DX11 needs a marker for instanced streams

  sVF_END           = 0,          // endmarker.
};

extern const sInt sVertexFormatTypeSizes[]; // contains sizes of datatypes (index=datatype>>8)

#if 0                             // some platforms require sU32 vertex colors to be swapped. this has nothing to do with endianess
inline sU32 sSWAPVC(sU32 x)  { return (x>>24)|(x<<8); }
#else
#define sSWAPVC(x) x
#endif


sINLINE sU16 sFloatToHalf(sF32 f)
{ sU32 i=sRawCast<sU32,sF32>(f); return sU16(((i&0x80000000)>>16)|(((i&0x7f800000)>>13)-(127<<10)+(15<<10))|((i&0x007fe000)>>13)); }

struct sHalfFloat // don't try to make a fully featured halffloat class, you don't want to accidentally use it...
{                 // Seee EEEE EMMM MMMM  MMMm mmmm mmmm mmmm  (32 bit, e-127)
  sU16 Val;       //                      SEEE EEMM MMMM MMMM  (16 bit, e-15)
  void Set(sF32 f) { sU32 i=sRawCast<sU32,sF32>(f); Val=sU16(((i&0x80000000)>>16)|(((i&0x7f800000)>>13)-(127<<10)+(15<<10))|((i&0x007fe000)>>13)); } // do not care about special cases :-(

  // ...but we want to be able to read halffloats (we do not handle NaN & co)
  sF32 Get() const
  { const sU32 f = ((Val&0x8000)<<16) | (((Val&0x7c00)+0x1C000)<<13) | ((Val&0x03FF)<<13);
    return sRawCast<sF32,sU32>(f);
  }
};
struct sHalfInt   // in case we are using normalized 16 bit integers instead of floats...
{
  sU16 Val;
  void Set(sF32 f) { Val = sU16(sInt((f+1)*32767.5f)-0x8000)&0xffff; }
};

struct sVertexFormatHandle : public sVertexFormatHandlePrivate
{
  friend sVertexFormatHandle *sCreateVertexFormat(const sU32 *discriptor);
  friend void sDestroyAllVertexFormats();
  friend void sStreamVertexFormat(sWriter &, const sVertexFormatHandle *vhandle);
  friend void sFlushVertexFormat(sBool flush,void *user=0);
  friend class sGeometry;

  struct OGLDecl
  {
    sInt Mode;                    // 0:END, 1:Attr, 2:Stream
    sInt Index;                   // Attr index or stream #
    sInt Size;                    // 1,2,3 or 4 scalars
    sInt Type;                    // GL_FLOAT, GL_UNSIGNED_BYTE, ...
    sInt Normalized;              // remap integers to [0..1]/[-1..1]
    sInt Offset;                  // offset to attribute in stream
  };
private:
  sInt Count;
  sU32 *Data;
  sInt UseCount;
  sInt VertexSize[sVF_STREAMMAX];
  sInt Streams;
  sU32 AvailMask;                  // used attributes
  sBool IsMemMarkSet;              // sSetMemMark() was called before allocating this
  sVertexFormatHandle *Next;

#if sRENDERER==sRENDER_OGL2
  OGLDecl *Decl;
#endif
  void Create();
  void Destroy();
public:
  sInt sOBSOLETE GetSize() const { return VertexSize[0]; }          // size in bytes
  sInt GetSize(sInt stream) const { return VertexSize[stream]; }    // size in bytes
  sInt GetCount() const { return Count; }    // element count
  sInt GetStreams() const { return Streams; }                       // highest used stream + 1. if streams 0 and 2 are used, this is 3
  sBool Has(sInt flag) const { return (AvailMask & (1<<(flag & sVF_USEMASK)))!=0; } // check if the format has a specific attribute
  sU32 GetAvailMask() const { return AvailMask; }                   // get all available attributes
  sInt GetOffset(sInt semantic_and_format);
  sInt GetDataType(sInt semantic)const;                             // slow: searches the description array

  const sU32 *GetDesc() const { return Data; }                      // if you want to know exactly...
#if sRENDERER==sRENDER_OGL2
  OGLDecl *GetDecl() { return Decl; }                               // this is only for internal use.
#endif
};


extern sVertexFormatHandle *sVertexFormatBasic;       // pos, color0
extern sVertexFormatHandle *sVertexFormatStandard;    // pos, normal, uv0
extern sVertexFormatHandle *sVertexFormatDouble;      // pos, color0, uv0, uv1
extern sVertexFormatHandle *sVertexFormatSingle;      // pos, color0, uv0
extern sVertexFormatHandle *sVertexFormatTangent;     // pos, normal, tangent, uv0
extern sVertexFormatHandle *sVertexFormatTSpace;      // pos, normal, tangent, color0, uv0, uv1
extern sVertexFormatHandle *sVertexFormatTSpace4;     // pos, normal, tangent with bitangent sign, color0, uv0, uv1
extern sVertexFormatHandle *sVertexFormatTSpace4_uv3;     // pos, normal, tangent with bitangent sign, color0, uv0, uv1, uv2
extern sVertexFormatHandle *sVertexFormatTSpace4Anim; // pos, normal, tangent with bitangent sign, indices, weights, color0, uv0, uv1
extern sVertexFormatHandle *sVertexFormatInstance;    // uv5/uv6/uv7 as sMatrix34CM in stream[1]
extern sVertexFormatHandle *sVertexFormatInstancePlus;    // + float4 in uv4

sVertexFormatHandle *sCreateVertexFormat(const sU32 *descriptor); // create a vertex format
void sDestroyAllVertexFormats();                                  // called by system... vertex formats are cheap.

void sStreamVertexFormat(sWriter &, const sVertexFormatHandle *vhandle);
void sStreamVertexFormat(sReader &, sVertexFormatHandle *&vhandle);

/****************************************************************************/

inline sU32 sMakeU32(sF32 x,sF32 y,sF32 z)
{ return ((sInt((x+0.5f)*127)&0xff)<<0)
       + ((sInt((y+0.5f)*127)&0xff)<<8)
       + ((sInt((z+0.5f)*127)&0xff)<<16); }


struct sVertexBasic               // 16 bytes
{
  sF32 px,py,pz;                  // 3 pos
  sU32 c0;                        // 1 color
  void Init(sF32 PX,sF32 PY,sF32 PZ,sU32 C0) volatile
  { px=PX; py=PY; pz=PZ; c0=sSWAPVC(C0); }
  void Init(const sVector31 &p,sU32 C0) volatile
  { px=p.x; py=p.y; pz=p.z; c0=sSWAPVC(C0); }

  static sVertexFormatHandle* VertexFormat() {return sVertexFormatBasic;}
};

struct sVertexStandard            // 32 bytes
{
  sF32 px,py,pz;                  // 3 pos
  sF32 nx,ny,nz;                  // 3 normal
  sF32 u0,v0;                     // 2 uv's
  void Init(sF32 PX,sF32 PY,sF32 PZ,sF32 NX,sF32 NY,sF32 NZ,sF32 U0,sF32 V0) volatile
  { px=PX; py=PY; pz=PZ; nx=NX; ny=NY; nz=NZ; u0=U0; v0=V0; }
  void Init(const sVector31 &p,const sVector30 &n,sF32 U0,sF32 V0) volatile
  { px=p.x; py=p.y; pz=p.z; nx=n.x; ny=n.y; nz=n.z; u0=U0; v0=V0; }

  static sVertexFormatHandle* VertexFormat() {return sVertexFormatStandard;}
};

struct sVertexTangent             // 44 bytes
{
  sF32 px,py,pz;                  // 3 position
  sF32 nx,ny,nz;                  // 3 normal
  sF32 tx,ty,tz;                  // 3 tangent
  sF32 u0,v0;                     // 2 uv's
  void Init(sF32 PX,sF32 PY,sF32 PZ,sF32 NX,sF32 NY,sF32 NZ,sF32 TX,sF32 TY,sF32 TZ,sF32 U0,sF32 V0) volatile
  { px=PX; py=PY; pz=PZ; nx=NX; ny=NY; nz=NZ; u0=U0; v0=V0; tx=ty=tz=0;}
  void Init(const sVector31 &p,const sVector30 &n,sF32 U0,sF32 V0) volatile
  { px=p.x; py=p.y; pz=p.z; nx=n.x; ny=n.y; nz=n.z; u0=U0; v0=V0; tx=ty=tz=0; }

  static sVertexFormatHandle* VertexFormat() {return sVertexFormatTangent;}
};

struct sVertexDouble              // 32 bytes
{
  sF32 px,py,pz;                  // 3 pos
  sU32 c0;                        // 1 color
  sF32 u0,v0;                     // 4 uv's
  sF32 u1,v1;

  void Init(sF32 PX,sF32 PY,sF32 PZ,sU32 C0,sF32 U0,sF32 V0,sF32 U1,sF32 V1) volatile
  { px=PX; py=PY; pz=PZ; c0=sSWAPVC(C0); u0=U0; v0=V0; u1=U1; v1=V1; }
  void Init(const sVector31 &p,sU32 C0,sF32 U0,sF32 V0,sF32 U1,sF32 V1) volatile
  { px=p.x; py=p.y; pz=p.z; c0=sSWAPVC(C0); u0=U0; v0=V0; u1=U1; v1=V1; }
  void Init(sF32 PX,sF32 PY,sF32 PZ,sU32 C0,sF32 U0,sF32 V0) volatile
  { px=PX; py=PY; pz=PZ; c0=sSWAPVC(C0); u0=U0; v0=V0; u1=0.0f; v1=0.0f; }
  void Init(const sVector31 &p,sU32 C0,sF32 U0,sF32 V0) volatile
  { px=p.x; py=p.y; pz=p.z; c0=sSWAPVC(C0); u0=U0; v0=V0; u1=0.0f; v1=0.0f; }

  static sVertexFormatHandle* VertexFormat() {return sVertexFormatDouble;}
};

struct sVertexSingle              // 24 bytes
{
  sF32 px,py,pz;                  // 3 pos
  sU32 c0;                        // 1 color
  sF32 u0,v0;                     // 2 uv's

  void Init(sF32 PX,sF32 PY,sF32 PZ,sU32 C0,sF32 U0,sF32 V0) volatile
  { px=PX; py=PY; pz=PZ; c0=sSWAPVC(C0); u0=U0; v0=V0; }
  void Init(const sVector31 &p,sU32 C0,sF32 U0,sF32 V0) volatile
  { px=p.x; py=p.y; pz=p.z; c0=sSWAPVC(C0); u0=U0; v0=V0; }
  void Init(const sVector31 &p,sU32 C0,const sVector2 &uv) volatile
  { px=p.x; py=p.y; pz=p.z; c0=sSWAPVC(C0); u0=uv.x; v0=uv.y; }

  static sVertexFormatHandle* VertexFormat() {return sVertexFormatSingle;}
};

struct sVertexTSpace              // 56 bytes, this will change a lot....
{
  sF32 px,py,pz;                  // 3 position
  sF32 nx,ny,nz;                  // 3 normal
  sF32 tx,ty,tz;                  // 3 tangent
  sU32 c0;                        // 1 color
  sF32 u0,v0;                     // 4 uv's (we WILL want a lightmap!)
  sF32 u1,v1;
  void Init(sF32 PX,sF32 PY,sF32 PZ,sF32 NX,sF32 NY,sF32 NZ,sF32 U0,sF32 V0) volatile
  { px=PX; py=PY; pz=PZ; nx=NX; ny=NY; nz=NZ; u0=U0; v0=V0; tx=ty=tz=u1=v1=0; c0=0;}
  void Init(const sVector31 &p,const sVector30 &n,sF32 U0,sF32 V0) volatile
  { px=p.x; py=p.y; pz=p.z; nx=n.x; ny=n.y; nz=n.z; u0=U0; v0=V0; tx=ty=tz=u1=v1=0; c0=0; }

  static sVertexFormatHandle* VertexFormat() {return sVertexFormatTSpace;}
};


struct sVertexTSpace4             // 56 bytes, this will change a lot....
{
  sF32 px,py,pz;                  // 3 position
  sF32 nx,ny,nz;                  // 3 normal
  sF32 tx,ty,tz,tsign;            // 4 tangent
  sU32 c0;                        // 1 color
  sF32 u0,v0;                     // 4 uv's (we WILL want a lightmap!)
  sF32 u1,v1;
/*
  void Init(sF32 PX,sF32 PY,sF32 PZ,sF32 NX,sF32 NY,sF32 NZ,sF32 U0,sF32 V0)
  { px=PX; py=PY; pz=PZ; nx=NX; ny=NY; nz=NZ; u0=U0; v0=V0; tx=ty=tz=u1=v1=0; c0=0;}
  void Init(const sVector31 &p,const sVector30 &n,sF32 U0,sF32 V0)
  { px=p.x; py=p.y; pz=p.z; nx=n.x; ny=n.y; nz=n.z; u0=U0; v0=V0; tx=ty=tz=u1=v1=0; c0=0; }
*/
  static sVertexFormatHandle* VertexFormat() {return sVertexFormatTSpace4;}
};

struct sVertexTSpace4_uv3         // 68 bytes, this will change a lot....
{
  sF32 px,py,pz;                  // 3 position
  sF32 nx,ny,nz;                  // 3 normal
  sF32 tx,ty,tz,tsign;            // 4 tangent
  sU32 c0;                        // 1 color
  sF32 u0,v0;                     // 6 uv's (we WILL want a lightmap!)
  sF32 u1,v1;
  sF32 u2,v2;

  static sVertexFormatHandle* VertexFormat() {return sVertexFormatTSpace4_uv3;}
};

struct sVertexInstance
{
  sMatrix34CM Matrix;
  static sVertexFormatHandle* VertexFormat() {return sVertexFormatInstance;}
};

struct sVertexInstancePlus
{
  sMatrix34CM Matrix;
  sVector4 Plus;
  static sVertexFormatHandle* VertexFormat() {return sVertexFormatInstancePlus;}
};

/****************************************************************************/
/***                                                                      ***/
/***   Shader Interface                                                   ***/
/***                                                                      ***/
/****************************************************************************/
  
enum sShaderTypeFlag
{
  // platform
  sSTF_NONE     = 0x0000,
  sSTF_HLSL23   = 0x0100,           // hlsl shader model 2 and 3
  sSTF_GLSL     = 0x0200,           // vertex_shader / fragment_shader
  sSTF_CG       = 0x0900,
  sSTF_HLSL45   = 0x0a00,           // hlsl shader model 4 and 5
  
  // kind
  sSTF_VERTEX   = 0x1000,
  sSTF_PIXEL    = 0x2000,
  sSTF_GEOMETRY = 0x3000,
  sSTF_HULL     = 0x4000,
  sSTF_DOMAIN   = 0x5000,
  sSTF_COMPUTE  = 0x6000,

  // dx profiles
  sSTFP_DX_20   = 0x0002,
  sSTFP_DX_30   = 0x0003,
  sSTFP_DX_40   = 0x0004,
  sSTFP_DX_30X  = 0x0006,
  sSTFP_DX_50   = 0x0007,

  sSTF_DX_20    = sSTF_HLSL23|sSTFP_DX_20,    // vs & ps
  sSTF_DX_30    = sSTF_HLSL23|sSTFP_DX_30,    // vs & ps
  sSTF_DX_40    = sSTF_HLSL45|sSTFP_DX_40,    // vs & ps
  sSTF_DX_50    = sSTF_HLSL45|sSTFP_DX_50,    // vs & ps

  // masks
  sSTF_PROFILE  = 0x00ff,
  sSTF_PLATFORM = 0x0f00,
  sSTF_KIND     = 0xf000,
};


enum sShaderCompileFlag
{
  sSCF_DEBUG          = 0x00010000,   // generating debug infos
  sSCF_AVOID_CFLOW    = 0x00020000,   // avoiding control flow
  sSCF_PREFER_CFLOW   = 0x00040000,   // prefer control flow
  sSCF_DONT_OPTIMIZE  = 0x00080000,   // skip optimization
};

/****************************************************************************/

enum sConstantBufferFlags
{
  sCBF_GPU_MEM  = 1,    // flagging cbuffer data mem as gpu mem
};

#define sFAKECBUFFER 0

#if !sFAKECBUFFER

// helper for compile time mask generation
template <sInt Start, sInt Count> struct CBMask
{
  template <sInt val, sInt MIN, sInt MAX> struct Clamp
  {
    enum { Val = val>MAX?MAX:(val<MIN?MIN:val) };
  };
  static const sU64 Mask =
    ( (((sU64(1)<<Clamp<(Count+3)/4-32,0,32>::Val)-1)<<32) |
      ((sU64(1)<<Clamp<(Count+3)/4,0,32>::Val)-1)
    )<<(Start/4);
};

/****************************************************************************/

class sCBufferBase : public sCBufferBasePrivate    // do not use this class directly!
{
  friend void sSetCBuffers(sCBufferBase **,sInt);
  friend void sSetShaders(sShader *vs,sShader *ps,sShader *gs,sLinkedShader **link,sCBufferBase **cbuffers,sInt cbcount);

public:
  sCBufferBase();                           // create data and lock
  void Modify();                            // relock new copy of buffer
  void OverridePtr(void *);                 // reload cb with data from here
  ~sCBufferBase();                          // destroy data

  sInt RegStart;                            // starting register
  sInt RegCount;                            // register count
  sS16 Slot;                                // dedicated slot & shader type
  sU16 Flags;                               // sCBF_???

  void SetCfg(sInt slot, sInt start, sInt count);
  void SetCfg(sInt slot, sInt start, sInt count, sU64 mask);
  void SetPtr(void **dataptr,void *data);   // used internally during initialization
  template <typename Type>
  sINLINE void Init(Type *data) { SetCfg(Type::Slot,Type::RegStart,Type::RegCount,CBMask<Type::RegStart,Type::RegCount>::Mask); SetPtr(0,data); }
protected:
  void SetRegs();                           // unlock and actually set constant registers
                                            // this have to be private/protected or current cbuffers have to be managed within SetRegs!
private:

};

//#if sRENDERER!=sRENDER_DX11

template <class Type>                       // you are supposed to create this as member variable or directly on stack
class sCBuffer : public sCBufferBase
{
  Type Storage;                             // sCBuffer always copy to command buffer and is valid until destruction
                                            // need other cbuffer type for direct command buffer access without copy
public:
  sCBuffer()
  {
    typedef sInt check0[(Type::RegStart&3)?-1:1];    // compile time check for register alignment
    typedef sInt check1[((sU32)Type::RegCount<(sizeof(Type)+15)/16)?-1:1];    // compile time check for data size / register count
    Mask = CBMask<Type::RegStart,Type::RegCount>::Mask;
    void *D = (void*)&Data;
    RegStart=Type::RegStart; 
    RegCount=Type::RegCount; 
    Slot=Type::Slot; 
    SetPtr((void**)D,(void*)&Storage);
    Modify();                               // a fresh cb is always "modified".
  }
  Type *Data;
};
/*
#else                                       // dx11 has real cbuffers

template <class Type>                       // you are supposed to create this as member variable or directly on stack
class sCBuffer : public sCBufferBase
{
public:
  sCBuffer()
  {
    RegStart=Type::RegStart; 
    RegCount=Type::RegCount; 
    Slot=Type::Slot; 
    SetPtr((void **)(&Data),0);
    Modify();
  }
  Type *Data;
};

#endif
*/
#else                                       // another cbuffer implementation , not used

class sCBufferBase                           // do not use this class directly!
{
  friend void sSetCBuffers(sCBufferBase **,sInt);
  friend void sSetShaders(sShader *vs,sShader *ps,sShader *gs,sLinkedShader **link,sCBufferBase **cbuffers,sInt cbcount);

public:
  sCBufferBase();
  void SetPtr(void **dataptr,void *data);
  void Modify();
  void OverridePtr(void *);
  ~sCBufferBase();

  sU8 RegStart;
  sU8 RegCount;
  sU8 Slot;
  sU8 pad;

  void SetCfg(sInt slot, sInt start, sInt count);
  void *DataPersist;
protected:
  void SetRegs();
};

template <class Type>
class sCBuffer : public sCBufferBase
{
public:
  Type *Data;
private:
  Type Storage;
public:
  sCBuffer()
  {
    RegStart=Type::RegStart; 
    RegCount=Type::RegCount; 
    Slot=Type::Slot;
    Data = &Storage;
    DataPersist = &Storage;
  }
};

#endif

/****************************************************************************/


// shader data helper
struct sShaderBlob
{
  sInt Type;
  sInt Size;
  sU8 Data[1];                              // dummy size, real size given by Bytes

  void SetNext(sInt type);
  sShaderBlob *Next();
  sShaderBlob *Get(sInt type);
  sShaderBlob *GetAny(sInt kind, sInt platform);
};

void sSerializeShaderBlob(sU8 *data, sInt &size, sReader &stream);
void sSerializeShaderBlob(const sU8 *data, sInt size, sWriter &stream);

// global functions

sShaderTypeFlag sGetShaderPlatform();
sInt sGetShaderProfile();

sShader *sCreateShader(sInt type,const sU8 *code,sInt bytes);      // create the shader from shader blob
sShader *sCreateShaderRaw(sInt type,const sU8 *code,sInt bytes);   // create the shader from "raw" data
//void sSetShaders(sShader *vs,sShader *ps,sShader *gs=0,sLinkedShader **link=0,sCBufferBase **cbuffers=0,sInt cbcount=0);
void sSetCBuffers(sCBufferBase **cbuffers,sInt cbcount);
sINLINE void sSetCBuffers(sCBufferBase *cbuffers)                 { sSetCBuffers(&cbuffers,1); }
sCBufferBase *sGetCurrentCBuffer(sInt slot);                      // needed for predicated tiling caching workaround
                                                                  // be careful: returned pointer doesn't need to point to a valid object

// the way shader parameters are set is not final. need a good idea here.

void sSOON_OBSOLETE sSetVSParam(sInt o, sInt count, const sVector4* vsf);    // VS consts are persistent between shaders
void sOBSOLETE sSetPSParam(sInt o, sInt count, const sVector4* psf);    // reset all PS constants every time you switch shaders
void sOBSOLETE sSetVSBool(sU32 bits,sU32 mask);                         // update predication
void sOBSOLETE sSetPSBool(sU32 bits,sU32 mask);

// the shader class

class sShader : public sShaderPrivate
{
public:
  union
  {
  sU8 *Data;                      // shader code
  sShaderBlob *Blob;
  };
  sInt Size;                      // size of code in bytes
  sInt Type;                      // PS/VS/GS
  sInt UseCount;                  // refcounting
  sU32 Hash;
  sShader *Link;

  ~sShader();
  sShader(sInt type,const sU8 *data,sInt length,sU32 hash,sBool raw=sFALSE);
  friend sShader *sCreateShader(sInt type,const sU8 *code,sInt bytes);
  friend sShader *sCreateShaderRaw(sInt type,const sU8 *code,sInt bytes);
  friend void sSetShaders(sShader *vs,sShader *ps,sShader *gs,sLinkedShader **link,sCBufferBase **cbuffers,sInt cbcount);
  friend void sCreateShader2(sShader *shader,sShaderBlob *blob);
  friend void sDeleteShader2(sShader *shader);
  friend void sSetShader2(sShader *shader);

  void Bind2(sVertexFormatHandle *vformat, sShader *pshader);
public:
#if sRENDERER == sRENDER_OGL2
  union
  {
    sInt GLName;                        // sRENDER_OGL2
    struct _CGprogram *cg;              // 
    struct CGBShader *cgb;              // 
  };
#endif


public:
  void AddRef();
  void Release();
  sBool CheckKind(sInt);
  const sU8 *GetCode(sInt &bytes);
  sShader *Bind(sVertexFormatHandle *vformat, sShader *pshader);

  sDNode Node;                    // privatE: list of all shaders
  sInt Temp;
};

// constant buffers
/*
class sCBuffer
{
  friend void sSetShaders(sShader *vs,sShader *ps,sShader *gs,sLinkedShader **link,sCBuffer **cbuffers,sInt cbcount);
  sInt RegStart;
  sInt RegCount;
  sInt Slot;
  sU32 *Copy;
  sU32 *Source;
public:
  sCBuffer(sInt start,sInt count,sInt slot,void *source);
  ~sCBuffer();
  void Update();
};
*/

/****************************************************************************/
/****************************************************************************/

// first obsolete shader interface

typedef sOBSOLETE class sShader *sShaderHandle; 
                                                              // always first set shader, then set constants
void sOBSOLETE sAddRefVS(sShader * vsh);
void sOBSOLETE sAddRefPS(sShader * psh);
sShader sOBSOLETE *sCreateVS(const sU32 *data,sInt count, sInt level=sSTF_DX_20);
sShader sOBSOLETE *sCreatePS(const sU32 *data,sInt count, sInt level=sSTF_DX_20);
void sOBSOLETE sDeleteVS(sShader *&);
void sOBSOLETE sDeletePS(sShader *&);
sBool sOBSOLETE sValidVS(sShader * vsh);
sBool sOBSOLETE sValidPS(sShader * psh);

// second obsolete shader interface

void sOBSOLETE sReleaseShader(sShader * sh);                      // shaders are refcounted
void sOBSOLETE sAddRefShader(sShader * sh);                       // shaders are refcounted
sBool sOBSOLETE sCheckShader(sShader * sh,sInt type);      // checks if the shader handle is a valid shader of the given type
const sOBSOLETE sU8 *sGetShaderCode(sShader * sh,sInt &bytes);    // get code from which the shader was created

/****************************************************************************/
/***                                                                      ***/
/***   Viewport                                                           ***/
/***                                                                      ***/
/****************************************************************************/

enum sViewportUpdateFlags
{
  sVUF_MODEL      = 0x0001,       // update model matrix
  sVUF_VIEW       = 0x0002,       // update view matrix
  sVUF_PROJ       = 0x0004,       // update projection matrix
  sVUF_WINDOW     = 0x0008,       // update screen rectangle

  sVUF_ALL        = 0x000f,       // update all
  sVUF_TRANSFORM  = 0x0007,       // update all transformation (not sVUF_WINDOW)
};

enum sViewportOrthogonal
{
  sVO_PROJECTIVE  = 0,            // perspective projection
  sVO_PIXELS      = 1,            // pixel addresssing
  sVO_SCREEN      = 2,            // 0..1 
  sVO_ORTHOGONAL  = 3,            // orthogonal projection, -1*zoom..+1*zoom
  sVO_PROJECTIVE_CLIPFAR = 4,     // perspective projection with projecting z on ClipFar(-Epsilon)
};

class sViewport
{
public:
  sMatrix44 Proj;                 // cache for projection matrix
  sMatrix34 View;
  sMatrix34 ModelView;            // Model * View
  sMatrix44 ModelScreen;          // Model * View * Proj

public:
  sViewport();
  void CopyFrom(const sViewport &);
  void SetTarget(sTexture2D *tex);        // set target to whole texture. (if texture is 0, use current target)
  void SetTarget(sTextureCube *tex, sTexCubeFace cf);
  void SetTarget(const sTargetSpec &spec);
  void SetTargetCurrent(const sRect *rect=0);   // takes whatever was set with sSetRendertarget()
  void SetTargetScreen(const sRect *rect=0); // set target to screen
  void SetZoom(sF32 aspect,sF32 zoom);  // zoom specifies the fov of the smaller axis. see getaspect.
  void SetZoom(sF32 zoom);        // as above, aspect is calculated from TargetSizeXY
  sF32 GetZoom() const;           // if you might want your parameter given in SetZoom(sF32) back
  void Prepare(sInt update=sVUF_ALL);
  void UpdateModelMatrix(const sMatrix34 &mat); // just change model matrix and call prepare.

  void PMatrix(const sMatrix44 &mat)  { Proj = mat; }
  const sMatrix44& PMatrix() const    { return Proj; }
  const sMatrix34& VMatrix() const    { return View; }
  const sMatrix34& MVMatrix() const   { return ModelView; }
  const sMatrix44& MVPMatrix() const  { return ModelScreen; }

  sMatrix34 Model;                // matrix for model
  sMatrix34 Camera;               // matrix for camera. View = Camera^-1

  sInt TargetSizeX;               // size of the rendertarget you want to use.
  sInt TargetSizeY;               // size of the rendertarget you want to use.
  sF32 TargetAspect;              // aspect ratio of the rendertarget you want to use.
  sRect Target;                   // portion inside the rendertarget you want to use.

  sF32 ClipNear;                  // z-clipping
  sF32 ClipFar;
//  sF32 Zoom,Aspect;
  sF32 ZoomX;                     // cot(fov_x / 2)
  sF32 ZoomY;                     // cot(fov_x / 2)*ys/xs

  sF32 CenterX;                   // center (set to 0.5 for real center), inside Window
  sF32 CenterY;
  sInt Orthogonal;                // completly changes meaning of everything: render orthogonally. Please also set CenterX and CenterY to 0

  sF32 DepthOffset;               // depth value offset
  void SetDepthOffset(sF32 cs_distance, sF32 cs_offset);  // calculating depht offset based on camerspace distance and camera space offset
  sF32 GetAspect() { return ZoomX/ZoomY; }  // aspect is X/Y, like 16/9. Previously it was defined the other way around!

  // using the frustum

  void MakeRayPixel(sInt mx,sInt my,sRay &ray) const;    // make ray, mx and my normalized to -1..1
  void MakeRay(sF32 mx,sF32 my,sRay &ray) const;    // make ray, mx and my normalized to -1..1
  sBool Transform(const sVector31 &p,sInt &ix,sInt &iy) const;
  sBool Transform(const sVector31 &p,sF32 &ix,sF32 &iy) const;
  sInt Visible(const sAABBox &box) const { return Visible(box,ClipNear,ClipFar); }    // 0=total out, 1=clip, 2=total in
  sInt Visible(const sAABBox &box, sF32 clipnear, sF32 clipfar) const;    // 0=total out, 1=clip, 2=total in
  sInt VisibleDist(const sAABBox &box,sF32 &dist) const;    // if not total out, return distance of closest point
  sInt VisibleDist2(const sAABBox &box,sF32 &near, sF32 &far) const; // if not total out, return distance range
  sBool Get2DBounds(const sAABBox &box,sFRect &bounds) const; // get 2D bounding box enclosing "box" in normalized clip coordinates. returns sFALSE if not visible.
  sBool Get2DBounds(const sVector31 points[],sInt count,sFRect &bounds) const; // same as above, 2D bbox for convex hull of "points". returns sFALSE if not visible.
};

/****************************************************************************/
/***                                                                      ***/
/***   Geometry                                                           ***/
/***                                                                      ***/
/****************************************************************************/

// old flags, dont use any more!
// remove?

//enum sGeometryFlagsOld             // obsolete names
//{
//  sGFV_STATIC            = 0x0001, // static buffer, never change once written
//  sGFV_FRAME             = 0x0002, // dynamic buffer, rewrite each frame
//  sGFV_STREAM            = 0x0003, // stremed buffer, draw immediatly
//  sGFV_MASK              = 0x0003,
//
//  sGFI_STATIC            = 0x0010, // see above
//  sGFI_FRAME             = 0x0020,
//  sGFI_STREAM            = 0x0030,
//  sGFI_MASK              = 0x0030,
//
//  sGF_STATIC             = 0x0011, // see above
//  sGF_FRAME              = 0x0022,
//  sGF_STREAM             = 0x0033,
//
//  sGFT_TRILIST           = 0x1000, // geometry types
//  sGFT_TRISTRIP          = 0x2000,
//  sGFT_LINELIST          = 0x3000,
//  sGFT_LINESTRIP         = 0x4000,
//  sGFT_QUAD              = 0x5000,
//  sGFT_SPRITE            = 0x6000,
//  sGFT_MASK              = 0xf000,
//
////  sGF_INDEX32        = 0x00010000, // use 32 bit index buffers (avoid)
//};

struct sDrawRange
{
  sInt Start;
  sInt End;
};

enum sGeomtryFlags                // flags for initializing geometries
{
  sGF_TRILIST           = 0x1000, // geometry types
  sGF_TRISTRIP          = 0x2000,
  sGF_LINELIST          = 0x3000,
  sGF_LINESTRIP         = 0x4000,
  sGF_QUADLIST          = 0x5000,
  sGF_SPRITELIST        = 0x6000,
  sGF_LINELISTADJ       = 0x7000, // .. with adjacency
  sGF_TRILISTADJ        = 0x8000,
  sGF_PATCHLIST         = 0x9000, // .. for tesselation
  sGF_QUADRICS          = 0xf000, // .. special primitive mode (quad/grid)
  sGF_PRIMMASK          = 0xf000,

  sGF_PATCHMASK         = 0x003f, // patchlist count, 1..32

  sGF_INDEXOFF      = 0x00000000, // no index buffer (avoid)
  sGF_INDEX32       = 0x00010000, // use 32 bit index buffers (avoid)
  sGF_INDEX16       = 0x00020000, // use 16 bit index buffers (do!)
  sGF_INDEXMASK     = 0x00030000, // mask index buffer bits

  sGF_INSTANCES     = 0x00040000, // use instancing API
  sGF_CPU_MEM       = 0x00080000, // use cache cpu mem for vertex and index buffers
};

enum sGeometryDuration            // state of a buffer
{
  sGD_NONE = 0,                   // uninitialized slot
  sGD_STATIC,                     // forever, unchangable
  sGD_FRAME,                      // flushed after end of frame
  sGD_STREAM,                     // flushed immediately
  sGD_CSBUFFER,                   // written by compute shaders, read by input assembly

  sGD_DYNAMIC,                    // used internally for sGeomentry::InitDyn()
};

enum sGeometryPrimitiveMode
{
  sGP_GRID = 1,
  sGP_QUAD = 2,
  sGP_WIREGRID = 3,
};

#define sMAX_VBCOUNT 0x8000       // prefered size for dynamic vertex and index buffers
#define sMAX_IBCOUNT 0x20000      // if you request more, you will get a "custom" buffer.
#define sMAX_QUADCOUNT 0x2000

#if sRENDERER!=sRENDER_DX11

struct sGeoBufferPart             // This geometry owns a part of a VB/IB
{
  sInt Start;                     // range start (this might be in bytes or elements depending on platform)
  sInt Count;                     // range size (in elements)
  sGeoBuffer *Buffer;             // buffer
  sGeoBufferPart();
  ~sGeoBufferPart();
#if sRENDERER==sRENDER_OGL2 || sRENDERER==sRENDER_BLANK
  void Init(sInt count,sInt size,sGeometryDuration duration,sInt buffertype);
  void Lock(void **);
  void Unlock(sInt count,sInt size);
  void Clear();
#endif
#if sRENDERER==sRENDER_DX9 || sRENDERER==sRENDER_DX11
  void *Init(sInt count,sInt size,sGeometryDuration duration,sInt buffertype,sInt advance=0);
  void Advance(sInt count,sInt size);
  void Clear();
  sBool IsEmpty();
  void CloneFrom(sGeoBufferPart *);
#endif
};
#endif

struct sVertexOffset
{
  sInt VOff[sVF_STREAMMAX];       // vertex offsets in stream
};

enum sGeometryDrawInfoEnum
{
  sGDI_Ranges       = 0x0001,
  sGDI_Instances    = 0x0002,
  sGDI_VertexOffset = 0x0004,
  sGDI_BlendFactor  = 0x0008,
};

class sGeometryDrawInfo
{
public:
  sInt Flags;                     // turn on more features

  sInt RangeCount;                // index ranges
  const sDrawRange *Ranges;

  sInt InstanceCount;             // instancing

  sInt VertexOffset[sVF_STREAMMAX]; // offset into vertex streams

  sU32 BlendFactor;               // override materials blend factor

  sTextureBase *Indirect;         // compute shader: buffer for indirect drawing

  sGeometryDrawInfo();
  sGeometryDrawInfo(sDrawRange *ir,sInt irc,sInt instancecount=0, sVertexOffset *off=0);
};


class sGeometry : public sGeometryPrivate    // the geometry itself
{
private:
  friend class sGeometryPrivate;
//  struct IDirect3DVertexDeclaration9 *VertexDecl;

#if sRENDERER==sRENDER_DX9
  void DrawPrim();                // specialized draw routine for prim mode.
  void DrawPrim(struct sGeoPrim *start,struct sGeoPrim *end,sInt ic,sInt vc);
#endif

#if sRENDERER==sRENDER_DX9 || sRENDERER==sRENDER_OGL2 || sRENDERER==sRENDER_BLANK
  sGeoBufferPart VertexPart[sVF_STREAMMAX];
  sGeoBufferPart IndexPart;
#endif

  sInt Flags;                     // flags at creation
  sInt IndexSize;                 // 2 or 4, derived from flags
  sBool PrimMode;                 // this buffer is loaded in prim mode (Quad/Grid)
  sGeoPrim *FirstPrim;            // first primitive in single linked list
  sGeoPrim **LastPrimPtr;         // used to append to single linked list
  sGeoPrim *CurrentPrim;          // used to unlock vertex buffer
  
  sVertexFormatHandle *Format;
  sU32 MorphTargetId;

  void InitPrivate();             // create private implementation
  void ExitPrivate();             // destroys private implementation
public:
  sGeometry();
  sGeometry(sInt flags,sVertexFormatHandle *);

  ~sGeometry();
  void Clear();
  void Serialize(sReader &s);
  sBool DebugBreak;

  // old interface
  void sOBSOLETE BeginLoad(sInt vc,sInt ic,sInt flags,sVertexFormatHandle *,void **vp,void **ip);

  // initialization and drawing

  void Init(sInt flags,sVertexFormatHandle *);
  sVertexFormatHandle *GetFormat() { return Format; }
  sInt GetFlags()const { return Flags; }
  sINLINE void SetMorphTargetId(sU32 id) { MorphTargetId=id; };
  sINLINE sU32 GetMorphTargetId() const { return MorphTargetId; };

  void Draw();
  void Draw(const sGeometryDrawInfo &di);
  void sSOON_OBSOLETE Draw(sDrawRange *ir,sInt irc,sInt instancecount=0, sVertexOffset *off=0);  // draw many small splitters

  // traditional buffer loading

  void BeginLoadIB(sInt ic,sGeometryDuration duration,void **ip);
  void BeginLoadVB(sInt vc,sGeometryDuration duration,void **vp,sInt stream=0);
  void BeginLoad(sVertexFormatHandle *,sInt flags,sGeometryDuration duration,sInt vc,sInt ic,void **vp,void **ip);

  template<typename T> void BeginLoadIB(sInt ic,sGeometryDuration duration,T **ip)
  { void **ptr = (void**)ip; BeginLoadIB(ic,duration,ptr);};
  template<typename T> void BeginLoadVB(sInt vc,sGeometryDuration duration,T **vp,sInt stream=0)
  { void **ptr = (void**)vp; BeginLoadVB(vc,duration,ptr,stream);  };
  template<typename V,typename I> void BeginLoad(sVertexFormatHandle *vh,sInt flags,sGeometryDuration duration,sInt vc,sInt ic,V **vp,I **ip)
  { void **vptr = (void**)vp; void **iptr = (void**)ip; BeginLoad(vh,flags,duration,vc,ic,vptr,iptr);  };

  void EndLoadIB(sInt ic=-1);
  void EndLoadVB(sInt vc=-1,sInt stream=0);
  void EndLoad(sInt vc=-1,sInt ic=-1);

  void SetVB(sCSBuffer *,sInt stream = 0);
  void SetIB(sCSBuffer *);

  void Merge(sGeometry *a,sGeometry *b);
  void MergeVertexStream(sInt DestStream,sGeometry *src,sInt SrcStream);

  void LoadCube(sU32 c0=0xffffffffU,sF32 sx=1,sF32 sy=1,sF32 sz=1,sGeometryDuration gd=sGD_STATIC);  // load cube (24 vertices). is quite smart in interpreting vertex formats and geometry flags
  void LoadTorus(sInt tx=48,sInt ty=12,sF32 ro=1.0f,sF32 ri=0.25f,sGeometryDuration=sGD_STATIC,sU32 c0=0xffffffffU);  // load torus

  // dynamic buffer loading, full control to update only parts of the buffer

  void InitDyn(sInt ic,sInt vc0,sInt vc1=0,sInt vc2=0,sInt vc3=0); // initialize for dynamic loading.
  void *BeginDynVB(sBool discard=0,sInt stream=0);           // Get access to buffer. using "no overwrite" flag
  void *BeginDynIB(sBool discard=0);
  void EndDynVB(sInt stream=0);                              // done acessing the buffer
  void EndDynIB();

  // quadrics interface (quads, grids and sprites)
  // the advantage: all quadrics are drawn in one or few GPU-Batches,
  // even when mixing multiple geometries. good for huds and 2d-stuff.

  void BeginQuadrics();
  void EndQuadrics();

  void BeginQuad(void **data,sInt count);
  void BeginGrid(void **data,sInt xs,sInt ys);
  void BeginWireGrid(void **data,sInt xs,sInt ys);

  template<typename T> void BeginQuad(T **data,sInt count)       { void **ptr = (void**)data; BeginQuad(ptr,count); };
  template<typename T> void BeginGrid(T **data,sInt xs,sInt ys)  { void **ptr = (void**)data; BeginGrid(ptr,xs,ys); };
  template<typename T> void BeginWireGrid(T **data,sInt xs,sInt ys)  { void **ptr = (void**)data; BeginWireGrid(ptr,xs,ys); };
  void EndQuad();
  void EndGrid();
  void EndWireGrid();

};

// macros for generating quads:
// don't template this, only sU16 & sU32 are allowed!

inline void sQuad(sU16 *&ip,sInt o,sInt a,sInt b,sInt c,sInt d)
{
  volatile sU32 *dst = (sU32*)ip;

  o = (o<<16)|o;
#if sCONFIG_BE
  *dst++ = o+((a<<16)|b);
  *dst++ = o+((c<<16)|a);
  *dst++ = o+((c<<16)|d);
#else
  *dst++ = o+((b<<16)|a);
  *dst++ = o+((a<<16)|c);
  *dst++ = o+((d<<16)|c);
#endif

  ip = (sU16*)dst;
}

inline void sQuadI(sU16 *&ip,sInt o,sInt d,sInt c,sInt b,sInt a)
{
  sQuad(ip, o, a, b, c, d);
}

inline void sQuad(sU32 *&ip,sInt o,sInt a,sInt b,sInt c,sInt d)
{
  *ip++ = o+a;
  *ip++ = o+b;
  *ip++ = o+c;
  *ip++ = o+a;
  *ip++ = o+c;
  *ip++ = o+d;
}

inline void sQuadI(sU32 *&ip,sInt o,sInt d,sInt c,sInt b,sInt a)
{
  sQuad(ip, o, a, b, c, d);
}

/****************************************************************************/
/***                                                                      ***/
/***   Occlusion Queries                                                  ***/
/***                                                                      ***/
/****************************************************************************/

class sOccQuery : public sOccQueryPrivate
{
public:
  sOccQuery();
  ~sOccQuery();
  
  sF32 Last;                      // last queried occlusion
  sF32 Average;                   // average occlusion
  sF32 Filter;                    // Average' = Average*(1-Filter) + Last*Filter. 1=hard, 0.001=soft

  void Begin(sInt pixels);        // begin sampling
  void End();                     // end sampling
  void Poll();                    // handle queries that finished, updateing Last and Average
};

/****************************************************************************/
/***                                                                      ***/
/***   Textures                                                           ***/
/***                                                                      ***/
/****************************************************************************/

// slow! only for debugging
void sCopyCubeFace(sTexture2D *dest, sTextureCube *src, sTexCubeFace cf);

class sTextureBase : public sTextureBasePrivate
{
protected:
  friend class sMaterial;
  friend class sTextureProxy;
  friend void sSetTexture(sInt, class sTextureBase*);
  friend void InitGFX(sInt,sInt,sInt);

  sInt Loading;
  sInt LockFlags;
  sU8 *LoadBuffer;                // some implementations require a buffer for loading textures

  void InitPrivate();             // create private implementation
  void ExitPrivate();             // destroys private implementation


public:
  struct sTextureProxyNode        // could link the proxies directly, but GCC does not like forward templates
  {
    sDNode Node;
    class sTextureProxy *Proxy;
  };
  sDList<sTextureProxyNode,&sTextureProxyNode::Node> Proxies; // multiple proxies may be connected to this texture. 
#if sRENDERER==sRENDER_OGL2
  sInt GLName;
#endif

  sTextureBase();
  virtual ~sTextureBase();

  // casting
  sTexture2D *CastTex2D()       { if((Flags&(sTEX_TYPE_MASK|sTEX_SOFTWARE))==sTEX_2D) return (sTexture2D*) this; return 0; }
  sTextureCube *CastTexCube()   { if((Flags&(sTEX_TYPE_MASK|sTEX_SOFTWARE))==sTEX_CUBE) return (sTextureCube*) this; return 0; }
  sTexture3D *CastTex3D()       { if((Flags&(sTEX_TYPE_MASK|sTEX_SOFTWARE))==sTEX_3D) return (sTexture3D*) this; return 0; }
  virtual void GetSize(sInt &xs,sInt &ys,sInt &zs)=0;

  sInt Temp;                      // for free use...
  sInt NameId;

  // read only!
  sInt Flags;                     // creation flags
  sInt Mipmaps;                   // number of valid mipmaps
  sInt BitsPerPixel;              // bits per pixel in this format
  sInt SizeX,SizeY,SizeZ;

  //!
  sU16 FrameRT;                   // last frame rendered into rendertarget
  sU16 SceneRT;                   // last scene rendered into rendertarget
};

// shortcut implementation for loading hardware dependant texture data
// returns NULL if not implemented or not applicable
sBool sReadTexture(sReader &s, sTextureBase *&tex); // tex==0: create new texture, otherwise reuse given texture
sINLINE sTextureBase *sReadTexture(sReader &s) { sTextureBase *tex=0; sReadTexture(s,tex); return tex; }
sInt sReadTextureSkipLevels(sInt skiplevels);     //! skipping top miplevels for saving memory, currently not available on all platforms
                                                  //! and not guarantueed to work for all textures or skipping all requested levels
                                                  //! returning number of levels which will be skipped during loading

/****************************************************************************/
/***                                                                      ***/
/***   sTexture2D                                                         ***/
/***                                                                      ***/
/****************************************************************************/

class sTexture2D : public sTextureBase
{
private:
  friend void sCopyCubeFace(sTexture2D *, sTextureCube *, sTexCubeFace);
  friend void sSetRendertarget(const sRect *vrp,sTexture2D *tex,sInt flags, sU32 clearcolor);
  friend void sSetRendertarget(const sRect *vrp,sInt flags,sU32 clearcolor,sTexture2D **tex,sInt count);
  friend void sGrabScreen(sTexture2D *tex, sGrabFilterFlags filter, const sRect *dst, const sRect *src);
  friend void sSetScreen(sTexture2D *tex, sGrabFilterFlags filter, const sRect *dst, const sRect *src);

#if sRENDERER==sRENDER_DX9
  virtual void OnLostDevice(sBool reinit=sFALSE);
#endif
#if sRENDERER==sRENDER_OGL2
  sInt GLFBName;
  sInt LoadMipmap;
#endif
  void Create2(sInt flags);
  void Destroy2();

  sInt OriginalSizeX;             // used when recreating texture after device lost. 
  sInt OriginalSizeY;             // this is required for the sTEX_SCREENSIZE flag
public:
  sTexture2D();
  sTexture2D(sInt xs,sInt ys,sU32 flags,sInt mipmaps=0);
  virtual ~sTexture2D();

  void Init(sInt xs, sInt ys, sInt flags,sInt mipmaps=0, sBool force=sFALSE);
  void ReInit(sInt xs, sInt ys, sInt flags,sInt mipmaps=0);
  void Clear();

  void BeginLoad(sU8 *&data,sInt &pitch,sInt mipmap=0);
  void BeginLoadPartial(const sRect &rect,sU8 *&data,sInt &pitch,sInt mipmap=0);
  void EndLoad();
  void LoadAllMipmaps(sU8 *data);
  void *BeginLoadPalette();       // most platforms don't support palettes
  void EndLoadPalette();

  void CalcOneMiplevel(const sRect &rect); // necessary for megatexture rendertarget. 
  void sSOON_OBSOLETE GetSize(sInt &xs,sInt &ys,sInt &zs) { xs=SizeX; ys=SizeY; zs=1; }
};

/****************************************************************************/
/***                                                                      ***/
/***   sTextureCube                                                       ***/
/***                                                                      ***/
/****************************************************************************/

class sTextureCube :
  public sTextureBase
{
private:
  friend void sCopyCubeFace(sTexture2D *, sTextureCube *, sTexCubeFace);
  friend void sSetRendertargetCube(sTextureCube* tex, sTexCubeFace cf, sInt clearflags, sU32 clearcolor);

  sTexCubeFace  LockedFace;
#if sRENDERER==sRENDER_DX9
  virtual void OnLostDevice(sBool reinit=sFALSE);
#endif
#if sRENDERER==sRENDER_OGL2
  sU32 GLFBName[6];
  sInt LoadMipmap;
  sInt LoadFace;
#endif

  void Create2(sInt flags);
  void Destroy2();

public:
  sTextureCube();
  sTextureCube(sInt dim, sInt flags, sInt mipmaps=0);
  virtual ~sTextureCube();

  void Init(sInt dim, sInt flags, sInt mipmaps=0, sBool force=sFALSE);
  void Clear();

  void BeginLoad(sTexCubeFace cf, sU8*& data, sInt& pitch, sInt mipmap=0);
  void EndLoad();
  void LoadAllMipmaps(sU8 *data);

  // read only!
  sInt sSOON_OBSOLETE SizeXY;                    // size of texture
  void sSOON_OBSOLETE GetSize(sInt &xs,sInt &ys,sInt &zs) { xs=SizeX; ys=SizeY; zs=6; }
};

/****************************************************************************/
/***                                                                      ***/
/***   sTextur3D                                                          ***/
/***                                                                      ***/
/****************************************************************************/

class sTexture3D :
  public sTextureBase
{
private:
#if sRENDERER==sRENDER_DX9
  virtual void OnLostDevice(sBool reinit=sFALSE) {}
#endif

public:
  sTexture3D(sInt xs, sInt ys, sInt zs, sU32 flags);
  virtual ~sTexture3D();

  void BeginLoad(sU8*& data, sInt& rpitch, sInt& spitch, sInt mipmap=0);
  void EndLoad();
  void Load(sU8 *src);
  void sSOON_OBSOLETE GetSize(sInt &xs,sInt &ys,sInt &zs) { xs=SizeX; ys=SizeY; zs=SizeZ; }
};


/****************************************************************************/
/***                                                                      ***/
/***   sTexturProxy                                                       ***/
/***                                                                      ***/
/****************************************************************************/

class sTextureProxy : public sTextureBase
{
  friend class sTextureBase;
protected:
  sTextureBase *Link;
public:
  sTextureProxy();
  ~sTextureProxy();

  void Connect(sTextureBase *);
  void Disconnect() { Connect(0); }

  void GetSize(sInt &xs,sInt &ys,sInt &zs);

  sTextureProxyNode Node;
  void Connect2();      // implemented by platform
  void Disconnect2();
};

/****************************************************************************/
/***                                                                      ***/
/***   Material                                                           ***/
/***                                                                      ***/
/****************************************************************************/

enum sMaterialConsts
{
  sMTRLENV_LIGHTS  = 4,
  sMTRLENV_MATRICES = 4,
  sMTRLENV_COLORS = 4,
};

sOBSOLETE class sMaterialEnv                          // additional variables for material
{
public:
  sMaterialEnv();                                     // initialize with meaningful defaults
  void Fix();                                         // check and repear ranges

  sVector30 LightDir[sMTRLENV_LIGHTS];                // 4 directional lights
  sU32 LightColor[sMTRLENV_LIGHTS];                   // color 0x00000000 disables light
  sU32 AmbientColor;                                  // ambient color.

  sMatrix44 Matrix[sMTRLENV_MATRICES];                // general purpose matrices
  sVector4 Color[sMTRLENV_COLORS];                    // general purpose pixel shader colors

  // view effect flags

  // fog parameters
  sF32 FogMax;
  sF32 FogStart;
  sF32 FogEnd;
  sF32 FogDensity;
  sU32 FogColor;

  // depth of field ipp parameters
  sF32 DOFBlurNear;                                   // near plane with maximal dof blur
  sF32 DOFFocusNear;                                  // near plane start with zero dof blur
  sF32 DOFFocusFar;                                   // far plane ending zero dof blur
  sF32 DOFBlurFar;                                    // far plane with maximal dof blur
  sF32 DOFBlurMax;                                    // maximal dof blur factor [0..1]
};

// material render state flags
struct sMaterialRS
{
  sMaterialRS();

  // don't change anything here unless you change sMaterial too
  sU32 Flags;                               // sMTRL_?? flags
  sU8 FuncFlags[4];                         // FuncFlags[sMFT_??] = sMFF_?? flags for depth test [0], alpha test [1] and stencil test [2]
  sU32 TFlags[sMTRL_MAXTEX];                // sMTF_?? flags
  sU32 BlendColor;                          // sMB?_?? flags for color
  sU32 BlendAlpha;                          // sMB?_?? flags for alpha or sMB_SAMEASCOLOR
  sU32 BlendFactor;                         // blendfactor 
  sU8 StencilOps[6];                        // sMSO_??? two sided stencil op flags for FAIL, ZFAIL, PASS (CW, CCW)
  sU8 StencilRef;														// the stencil reference value
  sU8 StencilMask;                          //
  sU8 AlphaRef;                             // reference for alpha test
  sU8 pad[3];
  sF32 LodBias[sMTRL_MAXTEX];               // mipmap lod bias. this interface is not final, it does not get saved!

  sBool operator==(const sMaterialRS& rs)const;
  sBool operator!=(const sMaterialRS& rs)const { return !((*this)==rs); }

  void SerializeOld(sReader &s);
  void SerializeOld(sWriter &s);
  void Serialize(sReader &s);
  void Serialize(sWriter &s);
};

/****************************************************************************/

class sMaterial : public sMaterialPrivate
{
protected:
#if sPLATFORM == sPLAT_WINDOWS
  sVertexFormatHandle *PreparedFormat;    // for checking correct prepare vertex format handling
public:
  sVertexFormatHandle *GetPreparedFormat()const { return PreparedFormat; }
#endif
public:
  sShader *VertexShader;
  sShader *PixelShader;
#if sRENDERER==sRENDER_DX11
  sShader *HullShader;
  sShader *DomainShader;
  sShader *GeometryShader;
  sShader *ComputeShader;
#endif

  sU32 validflag;
  bool Validate()
  { if(!VertexShader) return false;
    if(!PixelShader) return false;
    if(validflag!=0x12345678) return false;
    return true;
  }


protected:
  sInt StateVariants;
  sMaterialRS *VariantFlags;

  friend class sToolPlatform;

  void SetFixedEnv(sMaterialEnv *env);
//  void SetShader(sShader * vs,sShader * ps);
  void Create2();
  void Destroy2();

  enum {STATES_2PASS = 8};

#if sRENDERER==sRENDER_DX9 || sRENDERER==sRENDER_BLANK
public: // need access from specialized sToolPlatforms...
  void SetStates(sInt variant=0);
  void AddMtrlFlags(sU32 *&data);                     // add Flags and Blend 
  void AllocStates(const sU32 *data,sInt count,sInt var);  // optimise and copy state array from static buffer to material
protected:
#endif

#if sRENDERER==sRENDER_DX11
  void SetStates(sInt variant);
#endif

#if sRENDERER==sRENDER_OGL2
  void SetStates(sInt variant=0);
#endif

#if sRENDERER==sRENDER_OGLES2
  void SetStates(sInt variant=0);
  sInt StateVariants;
  sMaterialRS *VariantFlags;
#endif
  
public:
  sMaterial();
  virtual ~sMaterial();
  virtual void Prepare(sVertexFormatHandle *vf);              // prepare the material
  virtual void SelectShaders(sVertexFormatHandle *vf)=0; // overload this to set VertexShader and PixelShader member

  void Set(sInt variant=0)                                                  { Set(0,0,variant); }
  void Set(sCBufferBase **cbuffers,sInt cbcount,sInt variant=0);
  void Set(sCBufferBase *a)                                                 { Set(&a,1); }
  void Set(sCBufferBase *a,sCBufferBase *b)                                 { sCBufferBase *A[2] = {a,b};     Set(A,2); }
  void Set(sCBufferBase *a,sCBufferBase *b,sCBufferBase *c)                 { sCBufferBase *A[3] = {a,b,c};   Set(A,3);  }
  void Set(sCBufferBase *a,sCBufferBase *b,sCBufferBase *c,sCBufferBase *d) { sCBufferBase *A[4] = {a,b,c,d}; Set(A,4);  }
  void SetV(sCBufferBase *a,sInt variant=0)                                                 { Set(&a,1,variant); }
  void SetV(sCBufferBase *a,sCBufferBase *b,sInt variant=0)                                 { sCBufferBase *A[2] = {a,b};     Set(A,2,variant); }
  void SetV(sCBufferBase *a,sCBufferBase *b,sCBufferBase *c,sInt variant=0)                 { sCBufferBase *A[3] = {a,b,c};   Set(A,3,variant);  }
  void SetV(sCBufferBase *a,sCBufferBase *b,sCBufferBase *c,sCBufferBase *d,sInt variant=0) { sCBufferBase *A[4] = {a,b,c,d}; Set(A,4,variant);  }
  void CopyBaseFrom(const sMaterial *src);

  void InitVariants(sInt max);
  void SetVariant(sInt var);
  sInt GetVariantCount()const { return StateVariants; }

  void SetVariantRS(sInt var, const sMaterialRS &rs);
  sMaterialRS &GetVariantRS(sInt var) const;
  void DiscardVariants();                   // only for testing, prefer artist configured materials:
                                            // frees existing variants so that you can prepare the material again with different render states

  void DiscardVariantBuffers();             // special case only: discard original variant buffers after preparing to save memory

  sTextureBase *Texture[sMTRL_MAXTEX];

  sInt NameId;

  // sMaterialRS: don't change anything here unless you change sMaterialRS too
  sU32 Flags;                               // sMTRL_?? flags
  sU8 FuncFlags[4];                         // FuncFlags[sMFT_??] = sMFF_?? flags for depth test [0], alpha test [1] and stencil test [2]
  sU32 TFlags[sMTRL_MAXTEX];                // sMTF_?? flags
  sU32 BlendColor;                          // sMB?_?? flags for color
  sU32 BlendAlpha;                          // sMB?_?? flags for alpha or sMB_SAMEASCOLOR
  sU32 BlendFactor;                         // blendfactor 
  sU8 StencilOps[6];                        // sMSO_??? two sided stencil op flags for FAIL, ZFAIL, PASS (CW, CCW)
  sU8 StencilRef;														// the stencil reference value
  sU8 StencilMask;                          //
  sU8 AlphaRef;                             // reference for alpha test
  sU8 pad[3];
  sF32 LodBias[sMTRL_MAXTEX];               // mipmap lod bias. this interface is not final, it does not get saved!

  // end of sMaterialRS
#if sRENDERER==sRENDER_DX9 || sRENDERER==sRENDER_DX11                  // my console friends get angry if i make the material structure any larger...
  sU16 TBind[sMTRL_MAXTEX];                 // sMTB_?? flags. to which shader and stage is the texture to be mapped?
#endif
};

enum sMaterialFlags
{
  // these flags are converted to renderstates by graphics.cpp

  sMTRL_ZOFF          = 0x0000,             // zbuffer control
  sMTRL_ZREAD         = 0x0001,
  sMTRL_ZWRITE        = 0x0002,
  sMTRL_ZON           = 0x0003,
  sMTRL_ZMASK         = 0x0003,

  sMTRL_CULLOFF       = 0x0000,             // no culling, doublesided
  sMTRL_CULLON        = 0x0010,             // normal culling
  sMTRL_CULLINV       = 0x0020,             // inverted culling
  sMTRL_CULLMASK      = 0x0030,

  sMTRL_MSK_ALPHA     = 0x0100,             // color write mask
  sMTRL_MSK_RED       = 0x0200, 
  sMTRL_MSK_GREEN     = 0x0400,
  sMTRL_MSK_BLUE      = 0x0800,
  sMTRL_MSK_RGBA      = 0x0f00,
  sMTRL_MSK_RGB       = 0x0e00,
  sMTRL_MSK_MASK      = 0x0f00,

  sMTRL_STENCILSS     = 0x00010000,         // single sided stencil operation
  sMTRL_STENCILDS     = 0x00020000,         // double sided stencil operation
  sMTRL_SINGLESAMPLE  = 0x00040000,         // disable multisampling (FSAA)

  // these flags can be used to permuate vertex shaders

  sMTRL_LIGHTING      = 0x0080,             // enable light calculation!

  sMTRL_VC_NOLIGHT    = 0x0000,
  sMTRL_VC_LIGHT0     = 0x1000,             // vertex color0 used as vertex light
  sMTRL_VC_LIGHT1     = 0x2000,             // vertex color1 used as vertex light
  sMTRL_VC_LIGHT_MSK  = 0x3000,
  sMTRL_VC_LSHIFT     = 12,

  sMTRL_VC_NOCOLOR    = 0x0000,
  sMTRL_VC_COLOR0     = 0x4000,             // vertex color0 used as material color
  sMTRL_VC_COLOR1     = 0x8000,             // vertex color1 used as material color
  sMTRL_VC_COLOR_MSK  = 0xc000,
  sMTRL_VC_CSHIFT     = 14,

  sMTRL_VC_MSK        = 0xf000,
  sMTRL_VC_OFF        = 0x0000,

  sMTRL_NOZRENDER     = 0x01000000,         // this material is excluded from the z-only-render phase, making it invisible to DOF and SSAO
  sMTRL_LIGHTING_2SIDED = 0x02000000,       // use two sided lighting
};


enum sMaterialFunctionType
{
  sMFT_DEPTH = 0,                           // default: sMFF_LESSEQUAL
  sMFT_ALPHA = 1,                           // default: sMFF_ALWAYS. set to sMFF_GREATER to enable alpha test
  sMFT_STENCIL = 2,                         // default: sMFF_ALWAYS. set to sMFF_NOTEQUAL for shadow volumes
};

enum sMaterialFunctionFlag
{
  sMFF_NEVER          = 0,
  sMFF_LESS           = 1,
  sMFF_EQUAL          = 2,
  sMFF_LESSEQUAL      = 3,
  sMFF_GREATER        = 4,
  sMFF_NOTEQUAL       = 5,
  sMFF_GREATEREQUAL   = 6,
  sMFF_ALWAYS         = 7,
};

enum sMaterialTextureFlags
{
  // these flags are converted to renderstates by graphics.cpp

  sMTF_LEVEL0         = 0x0002,                       // point filter + point mipmap
  sMTF_LEVEL1         = 0x0003,                       // linear filter + point mipmap
  sMTF_LEVEL2         = 0x0000,                       // linear filter + linear mipmap (normal)
  sMTF_LEVEL3         = 0x0001,                       // extended quality (anisotropic filtering)
  sMTF_LEVELMASK      = 0x0003,

  // old flags
  //sMTF_TILE           = 0x0000,                       // texture addressing
  //sMTF_CLAMP          = 0x0010,
  //sMTF_MIRROR         = 0x0020,
  //sMTF_BORDER         = 0x0030,
  //sMTF_BORDER_BLACK   = 0x0030,                       // border color 0x00000000
  //sMTF_BORDER_WHITE   = 0x0040,                       // border color 0xffffffff
  //sMTF_TILEU_CLAMPV   = 0x0050,
  //sMTF_ADDRMASK       = 0x00f0,

  // texture addressing (new flags)
  sMTF_BCOLOR_WHITE   = 0x00000080,                   // use white border color for sMTF_BORDER_? address modes
  sMTF_BCOLOR_BLACK   = 0x00000000,                   // use black border color for sMTF_BORDER_? address modes

  sMTF_TILE_U         = 0x00000000,
  sMTF_TILE_V         = 0x00000000,
  sMTF_TILE_W         = 0x00000000,
  sMTF_CLAMP_U        = 0x10000000,
  sMTF_CLAMP_V        = 0x01000000,
  sMTF_CLAMP_W        = 0x00000010,
  sMTF_MIRROR_U       = 0x20000000,
  sMTF_MIRROR_V       = 0x02000000,
  sMTF_MIRROR_W       = 0x00000020,
  sMTF_BORDER_U       = 0x30000000,
  sMTF_BORDER_V       = 0x03000000,                   // select border color with sMTF_BCOLOR_???
  sMTF_BORDER_W       = 0x00000030,

  sMTF_ADDRMASK_U     = 0x30000000,
  sMTF_ADDRMASK_V     = 0x03000000,
  sMTF_ADDRMASK_W     = 0x00000030,


  sMTF_TILE           = sMTF_TILE_U|sMTF_TILE_V|sMTF_TILE_W,
  sMTF_CLAMP          = sMTF_CLAMP_U|sMTF_CLAMP_V|sMTF_CLAMP_W,
  sMTF_MIRROR         = sMTF_MIRROR_U|sMTF_MIRROR_V|sMTF_MIRROR_W,
  sMTF_BORDER_BLACK   = sMTF_BORDER_U|sMTF_BORDER_V|sMTF_BORDER_W|sMTF_BCOLOR_BLACK,    // border color 0x00000000
  sMTF_BORDER_WHITE   = sMTF_BORDER_U|sMTF_BORDER_V|sMTF_BORDER_W|sMTF_BCOLOR_WHITE,    // border color 0xffffffff
  sMTF_BORDER         = sMTF_BORDER_BLACK,
  sMTF_TILEU_CLAMPV   = sMTF_TILE_U|sMTF_CLAMP_V,
  sMTF_ADDRMASK       = sMTF_ADDRMASK_U|sMTF_ADDRMASK_V|sMTF_ADDRMASK_W,


  sMTF_SRGB           = 0x1000,                       // gamma corrected sRGB or linear texture, enable sMTF_SRGB for albedo colormaps,
                                                      // disable for lightmaps lookup textures or multiplicative blended textures
  sMTF_EXTERN         = 0x2000,                       // extern texture set by application (eg. render target)

  // these flags can be used to permuate vertex shaders

  sMTF_UVMASK         = 0x0f00,                       // OBSOLETE: these bits control uv transformation
  sMTR_UV3DMASK       = 0x0800,                       // OBSOLETE: if this is set, we need 3d calculations!
  sMTF_UV0            = 0x0000,
  sMTF_UV1            = 0x0100,
  sMTF_UV2            = 0x0200,
  sMTF_UV3            = 0x0300,
  sMTF_WORLDSPACE     = 0x0800,
  sMTF_MODELSPACE     = 0x0900,
  sMTF_CAMERASPACE    = 0x0a00,
  sMTF_REFLECTION     = 0x0b00,                       // OBSOLETE: lookup texture with reflection vector
  sMTF_SPHERICAL      = 0x0c00,                       // OBSOLETE: lookup texture with normal vector

  sMTF_SCALE          = 0x00004000,
  sMTF_TRANSFORM      = 0x00008000,         // OBSOLETE: duplicated, remove and only use matrix selector
  sMTF_MTRL_MAT       = 0x00010000,         // OBSOLETE: use sMTR_MTRL_MAT*id to select mtrl transformation matrix [1..4]
  sMTF_MTRL_MAT_MSK   = 0x000f0000,
  sMTF_ENV_MAT        = 0x00100000,         // OBSOLETE: use sMTR_ENV_MAT*id to select environment transformation matrix [1..4]
  sMTF_ENV_MAT_MSK    = 0x00f00000,

  sMTF_PCF            = 0x00000040,         // DX11: set comparison function = less in sampler for PCF
};

enum sMaterialStencilOp                     // stencil operations, is like DirectX D3DSTENCILOP_??? -1
{
  sMSO_KEEP = 0,                            // no operation
  sMSO_ZERO = 1,                            // set to zero
  sMSO_REPLACE = 2,                         // replace with ref
  sMSO_INCSAT = 3,                          // increase by one and clamp
  sMSO_DECSAT = 4,                          // decrease by one and clamp
  sMSO_INVERT = 5,                          // bit-invert stencil buffer
  sMSO_INC = 6,                             // increase by one
  sMSO_DEC = 7,                             // decrease by one
};
 
enum sMaterialStencilOpIndex
{
  sMSI_CWFAIL = 0,
  sMSI_CWZFAIL = 1,
  sMSI_CWPASS = 2,
  sMSI_CCWFAIL = 3,
  sMSI_CCWZFAIL = 4,
  sMSI_CCWPASS = 5,

  sMSI_FAIL = sMSI_CWFAIL,
  sMSI_ZFAIL = sMSI_CWZFAIL,
  sMSI_PASS = sMSI_CWPASS,
};

enum sMaterialBlend                         // define your blendmode in 11 bits...
{
  // source factor

  sMBS_0              = 0x00000001,         // 0
  sMBS_1              = 0x00000002,         // 1
  sMBS_SC             = 0x00000003,         //     source color
  sMBS_SCI            = 0x00000004,         // 1 - source color
  sMBS_SA             = 0x00000005,         //     source alpha
  sMBS_SAI            = 0x00000006,         // 1 - source alpha
  sMBS_DA             = 0x00000007,         //     destination alpha
  sMBS_DAI            = 0x00000008,         // 1 - destination alpha
  sMBS_DC             = 0x00000009,         //     destination color
  sMBS_DCI            = 0x0000000a,         // 1 - destination color
  sMBS_SA_SAT         = 0x0000000b,         // min(SA,DAI)
  sMBS_F              = 0x0000000e,         //     blend factor
  sMBS_FI             = 0x0000000f,         // 1 - blend factor
  sMBS_MASK           = 0x0000000f,

  // operation (usually add)

  sMBO_ADD            = 0x00000100,         //     sMBS_??? * Source + sMBD_??? * Dest
  sMBO_SUB            = 0x00000200,         //     sMBS_??? * Source - sMBD_??? * Dest
  sMBO_SUBR           = 0x00000300,         //   - sMBS_??? * Source + sMBD_??? * Dest
  sMBO_MIN            = 0x00000400,         //     min( Source, Dest )
  sMBO_MAX            = 0x00000500,         //     max( Source, Dest )
  sMBO_MASK           = 0x00000f00,

  // destination factor (same as source)

  sMBD_0              = 0x00010000,       
  sMBD_1              = 0x00020000,
  sMBD_SC             = 0x00030000,
  sMBD_SCI            = 0x00040000, 
  sMBD_SA             = 0x00050000,
  sMBD_SAI            = 0x00060000,
  sMBD_DA             = 0x00070000,
  sMBD_DAI            = 0x00080000,
  sMBD_DC             = 0x00090000,
  sMBD_DCI            = 0x000a0000,
  sMBD_SA_SAT         = 0x000b0000,         // for alpha channel always 1
  sMBD_F              = 0x000e0000,
  sMBD_FI             = 0x000f0000,
  sMBD_MASK           = 0x000f0000,

  // special values

  sMB_SAMEASCOLOR     = 0xffffffff,         // BlendAlpha is same as BlendColor
  sMB_OFF             = 0x00000000,         // Alphablending disabled

  // common blending equations

  sMB_ALPHA           = (sMBS_SA |sMBO_ADD |sMBD_SAI),
  sMB_ADD             = (sMBS_1  |sMBO_ADD |sMBD_1  ),
  sMB_SUB             = (sMBS_1  |sMBO_SUBR|sMBD_1  ),
  sMB_MUL             = (sMBS_DC |sMBO_ADD |sMBD_0  ),
  sMB_MUL2            = (sMBS_DC |sMBO_ADD |sMBD_SC ),
  sMB_MULINV          = (sMBS_0  |sMBO_ADD |sMBD_SCI),
  sMB_ADDSMOOTH       = (sMBS_1  |sMBO_ADD |sMBD_SCI),
  sMB_PMALPHA         = (sMBS_1  |sMBO_ADD |sMBD_SAI),
};

enum sMaterialTextureBind                   // bind texture to shader. optional, PC-Only
{
  sMTB_SAMPLERMASK= 0x00ff,                 // lower 8 bits store the texture stage
  sMTB_SHADERMASK = 0xff00,                 // upper bits store the shader to bind to
  sMTB_PS         = 0x0000,                 // Pixel Shader
  sMTB_VS         = 0x0100,                 // Vertex Shader
  sMTB_DS         = 0x0200,                 // Vertex Shader
  sMTB_HS         = 0x0300,                 // Vertex Shader
  sMTB_GS         = 0x0400,                 // Vertex Shader
  sMTB_CS         = 0x0500,                 // Vertex Shader
};

sInt sConvertOldUvFlags(sInt flags); // used for wz4 tex address mode convertion

/****************************************************************************/
/***                                                                      ***/
/***   More DX11 Specials                                                 ***/
/***                                                                      ***/
/****************************************************************************/

class sGfxThreadContext : public sGfxThreadContextPrivate
{
public:
  sGfxThreadContext();
  ~sGfxThreadContext();

  void BeginRecording();
  void EndRecording();

  void BeginUse();
  void EndUse();

  void Draw();
  void Flush();
};

/****************************************************************************/

#if sRENDERER==sRENDER_DX11

class sCSBuffer : public sTextureBase
{
public:
  sCSBuffer(sInt flags,sInt elements,sInt elementsize=0,const void *initdata=0);
  ~sCSBuffer();

  void GetSize(sInt &,sInt &,sInt &);
  void BeginLoad(void **);
  void EndLoad();
  template<class T> void BeginLoad(T **x) { BeginLoad((void **)x); }
};

class sComputeShader : private sComputeShaderPrivate
{
  sShader *Shader;

  sTextureBase *Texture[MaxTexture];
  sInt TFlags[MaxTexture];
  sTextureBase *UAV[MaxUAV];
  sInt LastTexture;
public:
  sComputeShader(sShader *);
  ~sComputeShader();
  void SetTexture(sInt n,sTextureBase *tex,sInt tflags=sMTF_LEVEL0|sMTF_TILE);
  void SetUAV(sInt n,sTextureBase *tex,sBool clear=1);
  void Prepare();
  void Draw(sInt xs,sInt ys,sInt zs,sInt cbcount,sCBufferBase **cbs);
  void Draw(sInt xs,sInt ys,sInt zs,sCBufferBase *cb0=0,sCBufferBase *cb1=0,sCBufferBase *cb2=0,sCBufferBase *cb3=0);

  sVector4 BorderColor;
};

#endif

/****************************************************************************/
/***                                                                      ***/
/***   internal use only                                                  ***/
/***                                                                      ***/
/****************************************************************************/

extern sInt sGFXRendertargetX;
extern sInt sGFXRendertargetY;
extern sF32 sGFXRendertargetAspect;
extern sALIGNED(sRect, sGFXViewRect, 16);

//extern sMatrix44 sGFXMatrices[sGM_MAX];
//extern sInt sGFXMtrlIsSet;

/*
extern sInt sGFXScreenX;
extern sInt sGFXScreenY;
extern sInt sGFXCurrentFrame;
extern sInt sGFXResetScreen;
extern sInt sGFXResetScreenX;
extern sInt sGFXResetScreenY;
*/

// Ryg's DXT compression
// input: a 4x4 pixel block, A8R8G8B8. you need to handle boundary cases
// yourself.
// alpha=sTRUE => use DXT5 (else use DXT1)
// quality: 0=normal (okay), 1=good (slower)
//    |128  to enable dithering
void sCompressDXTBlock(sU8 *dest,const sU32 *src,sBool alpha,sInt quality);
void sFastPackDXT(sU8 *d,sU32 *bmp,sInt xs,sInt ys,sInt format,sInt quality);

/****************************************************************************/

// HEADER_ALTONA_UTIL_GRAPHICS
#endif

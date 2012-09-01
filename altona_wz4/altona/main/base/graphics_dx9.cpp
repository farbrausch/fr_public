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
#if sRENDERER == sRENDER_DX9

#undef new
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#define new sDEFINE_NEW

#include "base/system.hpp"
#include "base/math.hpp"
#include "base/graphics.hpp"
#include "base/windows.hpp"

#include "base/serialize.hpp"
#include "util/image.hpp"

extern sInt sSystemFlags;

sInt sEngineHacks;

extern HWND sHWND;

#define STATS 1

void sInitGfxCommon();
void sExitGfxCommon();
void InitGraphicsCaps();

sInt sGFXCurrentFrame = 0;

/****************************************************************************/
/***                                                                      ***/
/***   Globals                                                            ***/
/***                                                                      ***/
/****************************************************************************/

enum { MAX_RENDERTARGETS = 256 };


enum sMtrlStateCfg
{
  // internal renderstate indices

  sMS_SAMPLERSHIFT = 4,                               // 20 samplers * 16 values
  sMS_SAMPLEROFFSET = (1<<sMS_SAMPLERSHIFT),
//  sMS_TEXTURESHIFT = 5,                               // 20 textures * 32 values
//  sMS_TEXTUREOFFSET = (1<<sMS_TEXTURESHIFT),

  sMS_RENDERSTATE = 0x0000,                           // renderstate
  sMS_RENDEREND = sMS_RENDERSTATE+0x0100,           
  sMS_SAMPLERSTATE = sMS_RENDEREND,          // sampler states
  sMS_SAMPLEREND = sMS_SAMPLERSTATE+(20<<sMS_SAMPLERSHIFT),
  sMS_MAX = sMS_SAMPLEREND,  // end of array
//  sMS_TEXTURESTATE = sMS_SAMPLERSTATE+(20<<sMS_SAMPLERSHIFT), // texture stage states
//  sMS_MAX = sMS_TEXTURESTATE+(20<<sMS_TEXTURESHIFT),  // end of array

};

/****************************************************************************/

static IDirect3D9 *DX9 = 0;
static UINT DX9Adapter = 0;
static D3DDEVTYPE DX9DevType;
static D3DFORMAT DX9Format;
static sScreenMode DXScreenMode;
/*static */IDirect3DDevice9 *DXDev=0;
static IDirect3DSurface9 *DXBlockSurface[2]={0,0};
#define sMAXSCREENS 16
static sInt DXScreenCount;
static sTexture2D *DXBackBuffer[sMAXSCREENS];
static HWND DXDummyWindow[sMAXSCREENS];
static sTexture2D *DXZBuffer=0;
static sTexture2D *DXZBufferRT=0;                   // render target zbuffer

static IDirect3DSurface9 *DXTargetSurf;             // current set rendertarget
static IDirect3DSurface9 *DXTargetZSurf;            // current set rendertarget

static sTextureBase *DXActiveRT=0;                  // 0=screen, 1=xlscreen, >=2 texture ptr
static sInt DXActiveRTFlags=0;
static sInt DXActiveMultiRT;                        // multiple render targets used. please diable in next sSetRenderTarget!
static sTextureBase *DXActiveZB=0;
static IDirect3DSurface9 *DXSetScreenSurf;          // buffer for sSetScreen
static IDirect3DSurface9 *DXRTSave = 0;
static IDirect3DSurface9 *DXRTSave2 = 0;
static sInt DXRTSaveXSize,DXRTSaveYSize;  // DXRTSave,DXRTSave2
static sInt DXSetScreenX,DXSetScreenY;    // DXSetScreenSurf
static sTextureBase* DXRenderTargets[MAX_RENDERTARGETS];
static sInt DXRTCount = 0;
extern sInt DXMayRestore = 1;

static sInt DXRestore=0;
static sInt DXActive=0;
static sInt DXXSIMode=0;       // special flags for D3DCreate when used within XSI
static sInt DXStreamsUsed = 0;   // number of streams used by last sGeometry::Draw(). used for caching.
static sInt DXInstanceSet = 0;    // number of streams for which instancing information was set
static sBool Render3DInProgress=0;    // flagging started scene with d3d->BeginScene
static sDInt DXTotalTextureMem = 0;

static sGeoBufferPart DXQuadIndex;
static sVertexOffset NoVertexOffset;

static sGraphicsStats Stats;
static sGraphicsStats BufferedStats;
static sGraphicsStats DisabledStats;
static sBool StatsEnable;

static sTextureBase *CurrentTexture[sMTRL_MAXVSTEX+sMTRL_MAXPSTEX];
static sShader *CurrentVS;
static sShader *CurrentPS;
static BOOL CurrentVSBools[16];
static BOOL CurrentPSBools[16];
static sVertexFormatHandle *CurrentMtrlVFormat=0;
static sBool DontCheckMtrlPrepare=sFALSE;

static D3DCAPS9 DXCaps;
static sInt DXShaderProfile;

static sInt RenderClippingFlag;
static sU32 RenderClippingData[4096];

static sBool ConvertsRGB = sTRUE;

static D3DDEVTYPE DevType = D3DDEVTYPE_HAL;

static sGraphicsCaps GraphicsCapsMaster;

static sDList2<sOccQueryNode> *FreeOccQueryNodes;
static sArray<sOccQueryNode *> *AllOccQueryNodes;

/****************************************************************************/

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE)
#define sMAXVB 1024
#define sMAXIB 4096

static sU32 sDXStates[sMS_MAX];

void ConvertFlags(sU32 flags, D3DFORMAT& d3df, D3DPOOL& pool, sInt& usage, sInt& mm, sInt& lflags);

/****************************************************************************/

struct sGeoBuffer
{
  union
  {
    struct IDirect3DIndexBuffer9 *IB;
    struct IDirect3DVertexBuffer9 *VB;
  };

  void Init();                    // preapre
  void Exit();                    // destructor
  void Reset();                   // refresh dynamic buffer after alt-tab
  void Lock();                    // lock whole buffer 
  void Unlock();                  // unlock buffer
  sInt CheckSize(sInt count,sInt size);  // return element index of next element

  sGeometryDuration Duration;     // sGD_???
  sInt BufferType;                // 0 = VB, 1 = INDEX16, 2 = INDEX32
  void *LockPtr;                  // ptr to buffer if locked

  sInt Alloc;                     // total available bytes
  sInt Used;                      // alreaedy used bytes
  sInt RefCount;                  // number of sGeoBufferPart that use this one.
  sInt Discard;                   // discard before next use!
  sInt InUse;                     // softlock 
};

#define sMAX_GEOBUFFER 512
#define sMAX_GEOPRIMS 16384

sGeoBuffer sGeoBuffers[sMAX_GEOBUFFER];
sGeoBuffer *sGeoBuffersLast[3];    // cache: last used geobuffer for each type
sInt sGeoBufferCount;
sInt sGeoBufferLocks;             // total number of locked geobuffers

void sGeoBufferInit();
void sGeoBufferExit();
void sGeoBufferFrame();
void sGeoBufferReset0();
void sGeoBufferReset1();
void sGeoBufferUnlockAll();
void GetScreenFormats(sInt display,sInt &flags,D3DFORMAT &color,D3DFORMAT &depth,D3DFORMAT &disfmt);

/****************************************************************************/

struct sGeoPrim
{
  sGeoPrim *Next;
  sGeoBufferPart VB;
  sInt Mode;
  sInt TessX;
  sInt TessY;
  sInt Vertex;
};

sGeoPrim sGeoPrims[sMAX_GEOPRIMS];
sInt sGeoPrimCount;

/****************************************************************************/
/***                                                                      ***/
/***   Windows Helper                                                     ***/
/***                                                                      ***/
/****************************************************************************/

#if sSTRIPPED
void DXError(sU32 err);

#define DXErr(hr) { sU32 err=hr; if(FAILED(err)) DXError(err); }
#else
void DXError(sU32 err,const sChar *file,sInt line,const sChar *system);

#define DXErr(hr) { sU32 err=hr; if(FAILED(err)) DXError(err,sTXT(__FILE__),__LINE__,L"d3d"); }
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Initialisation                                                     ***/
/***                                                                      ***/
/****************************************************************************/

void PreInitGFX(sInt &flags,sInt &xs,sInt &ys)
{
  DXTotalTextureMem = 0;

  // update DXScreenMode if it was not set

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
    if(DXScreenMode.Flags & sSM_FULLSCREEN)
      flags |= sISF_FULLSCREEN;
  }
}

static void MakeDX9()
{
  if(!DX9)
  {
    DX9 = Direct3DCreate9(D3D_SDK_VERSION);
    if(!DX9) sFatal(L"Could not create DirectX9");
  }
}

void InitGFX(sInt flags_,sInt xs_,sInt ys_)
{
  DontCheckMtrlPrepare = !sGetShellSwitch(L"checkmtrlprepare");

  sInt restart;

  DevType = (flags_ & sISF_REFRAST) ? D3DDEVTYPE_REF : D3DDEVTYPE_HAL;

  MakeDX9();

  // find some d3d values, check screenmode

  D3DFORMAT colormode,depthmode,displaymode;
  GetScreenFormats(DXScreenMode.Display,DXScreenMode.Flags,colormode,depthmode,displaymode);
  sInt flags2 = DXScreenMode.Flags;
  UINT adapter = DXScreenMode.Display>=0 ? DXScreenMode.Display : D3DADAPTER_DEFAULT;
  DWORD maxmultilevel;
  HRESULT hr = DX9->CheckDeviceMultiSampleType(adapter,DevType,colormode,!(flags2&sSM_FULLSCREEN),D3DMULTISAMPLE_NONMASKABLE,&maxmultilevel);
  if(FAILED(hr)) maxmultilevel = -1;

  DXErr(DX9->GetDeviceCaps( adapter, DevType, &DXCaps ))

  // check default size

  if(DXScreenMode.ScreenX==0 || DXScreenMode.ScreenY==0)
  {
    D3DDISPLAYMODE dm;
    HRESULT hr = DX9->GetAdapterDisplayMode(adapter,&dm);
    if(FAILED(hr))
      sFatal(L"DX error");
    DXScreenMode.ScreenX = dm.Width;
    DXScreenMode.ScreenY = dm.Height;
    DXScreenMode.Aspect = sF32(DXScreenMode.ScreenX) / sF32(DXScreenMode.ScreenY);
  }

  // ...

  sLogF(L"gfx",L"init d3d %d x %d\n",DXScreenMode.ScreenX,DXScreenMode.ScreenY);

  // multihead mode?

  DXScreenCount = 1;
  if(flags2 & sSM_MULTISCREEN)
  {
    sVERIFY(flags2 & sSM_FULLSCREEN);
    DXScreenCount = DXCaps.NumberOfAdaptersInGroup;
    if(DXScreenCount<1)
      DXScreenCount = 1;
    if(DXScreenCount>=sMAXSCREENS)
      sFatal(L"You graphics card can drive up to %d screens, but this application does only support %d screens in multi-screen mode.",DXScreenCount,sMAXSCREENS);
  }
 
  // Set up the structure used to create the D3DDevice

  D3DPRESENT_PARAMETERS d3dpp[sMAXSCREENS];
  sClear(d3dpp);
  if(flags2 & sSM_FULLSCREEN)
  {
    d3dpp[0].Windowed = 0;
    d3dpp[0].BackBufferWidth = DXScreenMode.ScreenX;
    d3dpp[0].BackBufferHeight = DXScreenMode.ScreenY;
    d3dpp[0].SwapEffect = D3DSWAPEFFECT_FLIP;
    d3dpp[0].BackBufferCount = 2;
    d3dpp[0].BackBufferFormat = colormode;
    d3dpp[0].PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
    d3dpp[0].FullScreen_RefreshRateInHz = DXScreenMode.Frequency;
  }
  else
  {
    RECT r;
    sU32 result = GetClientRect(sHWND,&r);
    sVERIFY(result);
    DXScreenMode.ScreenX = r.right-r.left;
    DXScreenMode.ScreenY = r.bottom-r.top;

    d3dpp[0].Windowed = 1;
    d3dpp[0].SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp[0].BackBufferCount = 1;
    d3dpp[0].BackBufferFormat = colormode;         // CHAOS: was D3DFMT_UNKNOWN. troublesome on 16 bit desktops!
    d3dpp[0].PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
  }
  if(flags2 & sSM_NOVSYNC)
  {
    d3dpp[0].PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
  }

  for(sInt i=1;i<DXScreenCount;i++)
  {
    d3dpp[i] = d3dpp[0];
    if(DXDummyWindow[i]==0)
    {
      DXDummyWindow[i]=CreateWindowW(L"EDIT",0,0,
        CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,
        HWND_MESSAGE,0,0,0);
    }
    d3dpp[i].hDeviceWindow = DXDummyWindow[i];
  }

  // create or restart

  if(DXDev==0)  // create new
  {
    DXRTCount = 0;
    sClear(DXRenderTargets);

    sVERIFY(sHWND!=0);
    timeBeginPeriod(1);
    restart = 0;

    DWORD behaviorfFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;

    // handle onboard gpus with software vertex processing correctly:
    if(!(DXCaps.DevCaps&D3DDEVCAPS_HWTRANSFORMANDLIGHT) || DXCaps.VertexShaderVersion < D3DVS_VERSION(1,1))
       behaviorfFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

    if (DXXSIMode) // recommended flags for use within XSI
      behaviorfFlags = D3DCREATE_FPU_PRESERVE | D3DCREATE_MIXED_VERTEXPROCESSING | D3DCREATE_MULTITHREADED;

    if(DXScreenCount>1)
      behaviorfFlags |= D3DCREATE_ADAPTERGROUP_DEVICE;

    DX9Adapter = adapter;
    DX9DevType = DevType;
    DX9Format = d3dpp[0].BackBufferFormat;
    InitGraphicsCaps();
    

    sCheckCapsHook->Call();

    HRESULT hr = DX9->CreateDevice(adapter,DevType,sHWND,behaviorfFlags,d3dpp,&DXDev);

    if(FAILED(hr) && DXScreenCount==1)
    {
      DXScreenMode.ScreenX = d3dpp[0].BackBufferWidth = 800;
      DXScreenMode.ScreenY = d3dpp[0].BackBufferHeight = 600;
      DXScreenMode.Aspect = sF32(DXScreenMode.ScreenX) / sF32(DXScreenMode.ScreenY);
      sLogF(L"gfx",L"retry d3d %d x %d\n",DXScreenMode.ScreenX,DXScreenMode.ScreenY);
      hr = DX9->CreateDevice(adapter,DevType,sHWND,behaviorfFlags,d3dpp,&DXDev);
    }
    if(FAILED(hr))
      sFatal(L"failed to initialize 3d graphics");

    FreeOccQueryNodes = new sDList2<sOccQueryNode>;
    AllOccQueryNodes = new sArray<sOccQueryNode *>;

    sInitGfxCommon();
  }
  else    // restart old
  {
    sOccQueryNode *qn;
    if(DXDev->TestCooperativeLevel()==D3DERR_DEVICELOST) // device is lost and can't be restored right now
      return;

    restart = 1;

    for(sInt i=0;i<sMAXSCREENS;i++)
      sDelete(DXBackBuffer[i]);
    sDelete(DXZBuffer);
    sDelete(DXZBufferRT);
    sRelease(DXBlockSurface[0]);
    sRelease(DXBlockSurface[1]);
    sRelease(DXSetScreenSurf);
    sGeoBufferReset0();
    sGraphicsLostHook->Call();
    sRelease(DXRTSave2);
    sRelease(DXRTSave);
    DXRTSaveXSize = DXRTSaveYSize = 0;
    sFORALL(*AllOccQueryNodes,qn)
    {
      DXErr(qn->Query->Release());
      qn->Query = 0;
    }

    sLogF(L"gfx",L"restoring device\n");
    for (sInt i=0; i<DXRTCount; i++)
      DXRenderTargets[i]->OnLostDevice();

    HRESULT hr = DXDev->Reset(d3dpp);
    if(hr==D3DERR_DEVICELOST)
      sLogF(L"gfx",L"device still lost - ignoring\n");
    else
      DXErr(hr);

    sInt max = DXRTCount;                         // this is tricky: create new DXRenderTargets[] 
    DXRTCount = 0;                                // directly over old DXRenderTargets[].
    for (sInt i=0; i<max; i++)
      DXRenderTargets[i]->OnLostDevice(sTRUE);
    sVERIFY(max==DXRTCount);                      // check if the trick really worked...
    sGeoBufferReset1();
    sFORALL(*AllOccQueryNodes,qn)
      DXErr(DXDev->CreateQuery(D3DQUERYTYPE_OCCLUSION,&qn->Query))
  }

  // Check support for depth textures (device must be available)  
  GraphicsCapsMaster.MaxMultisampleLevel = maxmultilevel;
  hr = DX9->CheckDeviceFormat(DX9Adapter, DX9DevType, DX9Format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, (D3DFORMAT)MAKEFOURCC('I','N','T','Z'));
  if (hr == D3D_OK)
    GraphicsCapsMaster.Flags |= sGCF_DEPTHTEX_INTZ; // Available on Geforce >= 8
  else
  {
    hr = DX9->CheckDeviceFormat(DX9Adapter, DX9DevType, DX9Format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, (D3DFORMAT)MAKEFOURCC('D','F','2','4'));
    if (hr == D3D_OK)
      GraphicsCapsMaster.Flags |= sGCF_DEPTHTEX_DF24;   // Available on Radeon >= X1300 (except X1800)
    else
    {
      hr = DX9->CheckDeviceFormat(DX9Adapter, DX9DevType, DX9Format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, (D3DFORMAT)MAKEFOURCC('R','A','W','Z'));
      if (hr == D3D_OK)
        GraphicsCapsMaster.Flags |= sGCF_DEPTHTEX_RAWZ;   // Available on Geforce >= 6      
    }
  }
  
  // Support for depth buffer msaa resolve?
  hr = DX9->CheckDeviceFormat(DX9Adapter, DX9DevType, DX9Format, D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, (D3DFORMAT)MAKEFOURCC('R','E','S','Z'));
  if (hr == D3D_OK)
    GraphicsCapsMaster.Flags |= sGCF_DEPTH_RESOLVE;   // Available on Radeon >= HD2000
  
  GraphicsCapsMaster.UVOffset =  0.0f;
  GraphicsCapsMaster.XYOffset = -0.5f;

  // get caps

  DXErr(DXDev->GetDeviceCaps(&DXCaps));
  DXShaderProfile = sSTF_HLSL23;
  const sChar *newPlatformInfix = L".pcl";
  if((DXCaps.VertexShaderVersion&0xffff)>=0x0300 && (DXCaps.PixelShaderVersion&0xffff)>=0x0300)
  {
    newPlatformInfix = L".pch";
    GraphicsCapsMaster.ShaderProfile = DXShaderProfile |= sSTFP_DX_30;
  }
  else if((DXCaps.VertexShaderVersion&0xffff)>=0x0200 && (DXCaps.PixelShaderVersion&0xffff)>=0x0200)
  {
    GraphicsCapsMaster.ShaderProfile = DXShaderProfile |= sSTFP_DX_20;
  }
  sLogF(L"gfx",L"caps.AdapterName           %s\n",GraphicsCapsMaster.AdapterName);
  sLogF(L"gfx",L"caps.Flags                 %08x\n",GraphicsCapsMaster.Flags);
  sLogF(L"gfx",L"caps.MaxMultisampleLevel   %d\n",GraphicsCapsMaster.MaxMultisampleLevel);
  sLogF(L"gfx",L"caps.ShaderProfile         %04x\n",GraphicsCapsMaster.ShaderProfile);
  sLogF(L"gfx",L"caps.Texture2D             %016x\n",GraphicsCapsMaster.Texture2D);
  sLogF(L"gfx",L"caps.TextureCube           %016x\n",GraphicsCapsMaster.TextureCube);
  sLogF(L"gfx",L"caps.TextureRT             %016x\n",GraphicsCapsMaster.TextureRT);
  sLogF(L"gfx",L"caps.VertexTex2D           %016x\n",GraphicsCapsMaster.VertexTex2D);
  sLogF(L"gfx",L"caps.VertexTexCube         %016x\n",GraphicsCapsMaster.VertexTexCube);

  if(!(GraphicsCapsMaster.Flags & sGCF_DEPTHTEX_MASK))
  {
    flags2 &= ~sSM_READZ;
    DXScreenMode.Flags &= ~sSM_READZ;
  }

  sInt xs = DXScreenMode.ScreenX;
  sInt ys = DXScreenMode.ScreenY;

  sInt tmode = sTEX_2D|sTEX_NOMIPMAPS|sTEX_RENDERTARGET;
  sInt zmode = 0;
  if(DXScreenMode.MultiLevel>=0)
    tmode |= sTEX_MSAA;
  if((flags2 & sSM_READZ) && (GraphicsCapsMaster.Flags & sGCF_DEPTHTEX_MASK)) // check, if depth textures are supported
    zmode = sTEX_DEPTH24;
  else
    zmode = sTEX_DEPTH24NOREAD;

  DXZBuffer = new sTexture2D(xs,ys,tmode|zmode|sTEX_NORESOLVE,1);
  for(sInt i=0;i<DXScreenCount;i++)
  {
    DXBackBuffer[i] = new sTexture2D(xs,ys,tmode|sTEX_ARGB8888|sTEX_INTERNAL,1);
    DXErr(DXDev->GetBackBuffer(i,0,D3DBACKBUFFER_TYPE_MONO,&DXBackBuffer[i]->Surf2D));
  }
  /*
  if(!DXBackBuffer->MultiSurf2D)
  {
    DXBackBuffer->MultiSurf2D = DXBackBuffer->Surf2D;
    DXBackBuffer->MultiSurf2D->AddRef();
  }
  */

  // locking

  DXErr(DXDev->CreateOffscreenPlainSurface(16,16,D3DFMT_A8R8G8B8,
    D3DPOOL_DEFAULT,&DXBlockSurface[0],0));
  DXErr(DXDev->CreateOffscreenPlainSurface(16,16,D3DFMT_A8R8G8B8,
    D3DPOOL_DEFAULT,&DXBlockSurface[1],0));

  // geo buffers

  if(!restart)
  {
    sU16 *ip;
    sGeoBufferInit();
    ip = (sU16 *)DXQuadIndex.Init(sMAX_QUADCOUNT*6,2,sGD_STATIC,1);
    for(sInt i=0;i<sMAX_QUADCOUNT;i++)
      sQuad(ip,i*4,0,1,2,3);
    DXQuadIndex.Advance(-1,2);
  }

  // rt-zbuffer

  sInt rtx = sMax(DXScreenMode.RTZBufferX,DXScreenMode.ScreenX);
  sInt rty = sMax(DXScreenMode.RTZBufferY,DXScreenMode.ScreenY);
  if(rtx && rty)
    DXZBufferRT = new sTexture2D(rtx,rty,sTEX_2D|sTEX_DEPTH24NOREAD|sTEX_NOMIPMAPS|sTEX_RENDERTARGET,1);

  // misc

  DXRestore = 0;
  DXActive = 1;
}

/****************************************************************************/

void ExitGFX()
{
  sOccQueryNode *qn;
  sFORALL(*AllOccQueryNodes,qn)
    qn->Query->Release();
  sDeleteAll(*AllOccQueryNodes);
  sDelete(AllOccQueryNodes);
  sDelete(FreeOccQueryNodes);

  DXActive = 0;

  DXQuadIndex.Clear();
  sGeoBufferExit();

  sExitGfxCommon();

  sDelete(DXZBuffer);
  DXActiveRT = 0;
  DXActiveRTFlags = 0;
  DXActiveZB = 0;
  DXActiveMultiRT = 0;
  for(sInt i=0;i<sMAXSCREENS;i++)
    sDelete(DXBackBuffer[i]);
  sDelete(DXZBuffer);
  sDelete(DXZBufferRT);
  sRelease(DXBlockSurface[0]);
  sRelease(DXBlockSurface[1]);
  sRelease(DXRTSave);
  sRelease(DXRTSave2);

  sRelease(DXDev);
  sRelease(DX9);

  timeEndPeriod(1);

  for(sInt i=0;i<sMAXSCREENS;i++)
  {
    if(DXDummyWindow[i])
    {
      DestroyWindow(DXDummyWindow[i]);
      DXDummyWindow[i] = 0;
    }
  }

//  sVERIFY(DXTotalTextureMem==0);
}

void ResizeGFX(sInt x,sInt y)  // this is called when the windows size changes
{
  if(x && y && (x!=DXScreenMode.ScreenX || y!=DXScreenMode.ScreenY)) 
  {
    DXScreenMode.ScreenX = x;
    DXScreenMode.ScreenY = y;
    if (!(DXScreenMode.Flags & sSM_FULLSCREEN))
      DXScreenMode.Aspect = sF32(x)/sF32(y);
    DXRestore = 1;
  }
}

void SetXSIModeD3D(sBool enable)
{
  DXXSIMode = enable;
}

void sCreateZBufferRT(sInt xs,sInt ys)
{
  if(DXDev)
  {
    sInt rtx = sMax(sMax(xs,DXScreenMode.RTZBufferX),DXScreenMode.ScreenX);
    sInt rty = sMax(sMax(ys,DXScreenMode.RTZBufferY),DXScreenMode.ScreenY);
    if(rtx!=DXScreenMode.RTZBufferX || rty!=DXScreenMode.RTZBufferY)
    {
      sDelete(DXZBufferRT);
      DXZBufferRT = new sTexture2D(rtx,rty,sTEX_2D|sTEX_DEPTH24NOREAD|sTEX_NOMIPMAPS|sTEX_RENDERTARGET,1);
      DXScreenMode.RTZBufferX = xs;
      DXScreenMode.RTZBufferY = ys;
    }
  }
  else
  {
    DXScreenMode.RTZBufferX = xs;
    DXScreenMode.RTZBufferY = ys;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   old screenmode interface                                           ***/
/***                                                                      ***/
/****************************************************************************/

sBool sSetOversizeScreen(sInt xs,sInt ys,sInt fsaa,sBool mayfail)
{
  sScreenMode sm;
  sGetScreenMode(sm);

  if(xs==0 || ys==0)
  {
    sm.OverX = 0;
    sm.OverY = 0;
    sm.Flags &= ~sSM_OVERSIZE;
    sm.OverMultiLevel = 0;
  }
  else
  {
    sm.OverX = xs;
    sm.OverY = ys;
    sm.Flags |= sSM_OVERSIZE;
    sm.OverMultiLevel = fsaa;
  }

  sSetScreenMode(sm);
  return 1;
}

void sGetScreenSafeArea(sF32 &xs, sF32 &ys)
{
  xs=ys=1;
}

/****************************************************************************/
/***                                                                      ***/
/***   new screenmode interface                                           ***/
/***                                                                      ***/
/****************************************************************************/

void GetScreenFormats(sInt display,sInt &flags,D3DFORMAT &color,D3DFORMAT &depth,D3DFORMAT &displayfmt)
{
  static const D3DFORMAT dt[8][4] =      // 16bit is just a hint!
  {
    { D3DFMT_D32,D3DFMT_D24X8,D3DFMT_D16 },
    { D3DFMT_D16,D3DFMT_D32,D3DFMT_D24X8 },
    { D3DFMT_D32F_LOCKABLE,D3DFMT_D16_LOCKABLE },
    { D3DFMT_D16_LOCKABLE,D3DFMT_D32F_LOCKABLE },
    { D3DFMT_D24S8 },
    { D3DFMT_D24S8 },
    {  },
    {  },
  };

  // assume defaults:

  if(display==-1) display = D3DADAPTER_DEFAULT;
  sBool windowed = (flags & sSM_FULLSCREEN)?0:1;
  color = D3DFMT_A8R8G8B8;
  displayfmt = D3DFMT_X8R8G8B8;
  if(FAILED(DX9->CheckDeviceType(display,DevType,displayfmt,color,windowed)))
    color = D3DFMT_X8R8G8B8;

  // check for 16bit color

  if(flags & sSM_COLOR16BIT)
  {
    if(SUCCEEDED(DX9->CheckDeviceType(display,DevType,D3DFMT_R5G6B5,D3DFMT_R5G6B5,windowed)))
      displayfmt = color = D3DFMT_R5G6B5;
    else
      flags &= ~sSM_COLOR16BIT;
  }

  // last sanity check

  DXErr(DX9->CheckDeviceType(display,DevType,displayfmt,color,windowed));

  // check for stencil buffer. only one stencil format is supported: D24S8

  for(sInt check=0;check<5;check++)
  {
    sInt id = 0;

    switch(check)
    {
    case 0: break;
    case 1: flags &= ~sSM_DEPTHREAD; break;
    case 2: flags &= ~sSM_STENCIL; break;
    }

    if(flags&sSM_DEPTH16BIT) id |= 1;
    if(flags&sSM_DEPTHREAD)  id |= 2;
    if(flags&sSM_STENCIL)    id |= 4;
    for(sInt i=0;i<4;i++)
    {
      depth = dt[id][i];
      if(depth)
        if(SUCCEEDED(DX9->CheckDeviceFormat(display,DevType,displayfmt,D3DUSAGE_DEPTHSTENCIL,D3DRTYPE_SURFACE,depth)))
          if(SUCCEEDED(DX9->CheckDepthStencilMatch(display,DevType,displayfmt,color,depth)))
            goto found;
    }
  }
  sFatal(L"can't find a fitting z-buffer");
found:;
}

sInt sGetDisplayCount()
{
  MakeDX9();
  return DX9->GetAdapterCount();
}

sInt ggt(sInt a,sInt b)
{
  while(b!=0)
  {
    sInt h = a % b;
    a = b;
    b = h;
  }
  return a;
}

void sGetScreenInfo(sScreenInfo &si,sInt flags,sInt display)
{
  MakeDX9();
  sScreenInfoXY *xy;
  sInt *ip;
  sInt x,y,max;
  sArray<sScreenInfoXY> Resolutions;
  sArray<sInt> RefreshRates;
  sArray<sScreenInfoXY> AspectRatios;

  // prepare

  si.Clear();
  si.Flags = flags;
  si.Display = display;

  // find correct adapter for multiscreen mode
  if(display>=0 && DXActive && DXScreenCount>1)
  {
    // multiscreen? enumerate correctly
    for (sInt i=0;;i++)
    {
      D3DCAPS9 caps;
      if (FAILED(DX9->GetDeviceCaps(i,DevType,&caps)))
      {
        display=-1;
        break;
      }
      if (caps.MasterAdapterOrdinal==DXCaps.AdapterOrdinal && caps.AdapterOrdinalInGroup==(UINT)display)
      {
        display=caps.AdapterOrdinal;
        break;
      }
    }
  }
  if(display==-1) display = D3DADAPTER_DEFAULT;

  // check flags

  D3DFORMAT color = D3DFMT_UNKNOWN;
  D3DFORMAT depth = D3DFMT_UNKNOWN;
  D3DFORMAT disfmt = D3DFMT_UNKNOWN;
  GetScreenFormats(display,si.Flags,color,depth,disfmt);

  // get modes

  D3DDISPLAYMODE mode;
  max = DX9->GetAdapterModeCount(display,disfmt);
  for(sInt n=0;n<max;n++)
  {
    DXErr(DX9->EnumAdapterModes(display,disfmt,n,&mode));
    sFORALL(Resolutions,xy)
      if(xy->x==sInt(mode.Width) && xy->y==sInt(mode.Height))
        goto found1;
    xy = Resolutions.AddMany(1);
    xy->x = mode.Width;
    xy->y = mode.Height;
found1:

    sFORALL(RefreshRates,ip)
      if(*ip == sInt(mode.RefreshRate))
        goto found2;
    ip = RefreshRates.AddMany(1);
    *ip = mode.RefreshRate;
found2:
    if(0)   // CHAOS: deriving possible aspect rations from screen modes is stupid.
    {
      x = mode.Width;
      y = mode.Height;
      for(;;)
      {
        sInt h = ggt(x,y);
        if(h==1)
          break;
        x /= h;
        y /= h;
      }
      sFORALL(AspectRatios,xy)
        if(xy->x==x && xy->y==y)
          goto found3;
      xy = AspectRatios.AddMany(1);
      xy->x = x;
      xy->y = y;
  found3:;
    }
  }

  // add predefined aspect ratios:

  sScreenInfoXY *as = AspectRatios.AddMany(4);
  as[0].x =  5; as[0].y = 4;
  as[1].x =  4; as[1].y = 3;
  as[2].x = 16; as[2].y = 10;
  as[3].x = 16; as[3].y = 9;

  // sort modes

  max = Resolutions.GetCount();
  for(sInt i=0;i<max-1;i++)
    for(sInt j=i+1;j<max;j++)
      if(Resolutions[i].x*Resolutions[i].y > Resolutions[j].x*Resolutions[j].y)
        Resolutions.Swap(i,j);

  max = RefreshRates.GetCount();
  for(sInt i=0;i<max-1;i++)
    for(sInt j=i+1;j<max;j++)
      if(RefreshRates[i] > RefreshRates[j])
        RefreshRates.Swap(i,j);
  
  max = AspectRatios.GetCount();
  for(sInt i=0;i<max-1;i++)
    for(sInt j=i+1;j<max;j++)
      if(sF32(AspectRatios[i].x)/AspectRatios[i].y > sF32(AspectRatios[j].x)/AspectRatios[j].y)
        AspectRatios.Swap(i,j);

  // multisampling

  DWORD mlvs=0;
  HRESULT hr=DX9->CheckDeviceMultiSampleType(display,DevType,disfmt,!(flags&sSM_FULLSCREEN),D3DMULTISAMPLE_NONMASKABLE,&mlvs);
  if (FAILED(hr) && hr!=D3DERR_NOTAVAILABLE) DXErr(hr);  si.MultisampleLevels = mlvs;

  // get current state

  DXErr(DX9->GetAdapterDisplayMode(display,&mode));
  si.CurrentXSize = mode.Width;
  si.CurrentYSize = mode.Height;
  si.CurrentAspect = sF32(si.CurrentXSize)/sF32(si.CurrentYSize); // this is wrong on some displays!

  // copy result

  si.Resolutions = Resolutions;
  si.RefreshRates = RefreshRates;
  si.AspectRatios = AspectRatios;

  // get monitor name

  HMONITOR hm=DX9->GetAdapterMonitor(display);
  MONITORINFOEX moninfo;
  sClear(moninfo);
  moninfo.cbSize=sizeof(moninfo);
  GetMonitorInfo(hm,&moninfo);
  DISPLAY_DEVICE ddev;
  sClear(ddev);
  ddev.cb=sizeof(ddev);
  EnumDisplayDevices(moninfo.szDevice,0,&ddev,1);
  sCopyString(si.MonitorName,ddev.DeviceString);
}

void sGetScreenMode(sScreenMode &sm)
{
  sm = DXScreenMode;
}

sBool sSetScreenMode(const sScreenMode &smorg)
{
  sScreenMode sm;

  sm = smorg;
  if(!(sm.Flags & sSM_FULLSCREEN) && (DXScreenMode.Flags&sSM_VALID))
  {
    sm.ScreenX = DXScreenMode.ScreenX;
    sm.ScreenY = DXScreenMode.ScreenY;
  }

  if(sCmpMem(&DXScreenMode,&sm,sizeof(sScreenMode))!=0)
  {
    sInt flags = 0;
    if(sm.Flags & sSM_FULLSCREEN) flags |= sISF_FULLSCREEN;
    if(sm.Flags & sSM_REFRAST)    flags |= sISF_REFRAST;
    if(sm.Flags & sSM_NOVSYNC)    flags |= sISF_NOVSYNC;
    if(sm.MultiLevel>=0)          flags |= sISF_FSAA;

    DXScreenMode = sm;

    DXRestore = 1;
  }
  return 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   General Engine Interface                                           ***/
/***                                                                      ***/
/****************************************************************************/

void sConvertsRGBTex(sBool e)
{
  ConvertsRGB = e;
}

sBool sConvertsRGBTex()
{
  return ConvertsRGB;
}

#if 0

void sClearRendertargetPrivate(sInt flags,sU32 color)
{
  // clear with scissor?
  sBool noscissor=(flags&sCLEAR_NOSCISSOR);
  flags &= sCLEAR_ALL;

  if(!DXActiveZB)
  {
    // don't clear zbuffer if no zbuffer buffer is active
#if !sRELEASE
    if(flags&2)
      sLogF(L"gfx",L"tried to clear NULL depth buffer\n");
#endif
    flags &= ~2;
  }
  static sInt f[4] = 
  {
    0,
    D3DCLEAR_TARGET,
                    D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,
    D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,
  };
  if((flags & 3)==0) return;
  flags = f[flags];
  if(!(DXScreenMode.Flags & sSM_STENCIL)) flags &= ~D3DCLEAR_STENCIL;

  if(sGFXViewRect.SizeX())
  {
    if(noscissor)
    {
      D3DVIEWPORT9 d3dvp;
      d3dvp.X = 0;
      d3dvp.Y = 0;
      d3dvp.Width = sGFXRendertargetX;
      d3dvp.Height = sGFXRendertargetY;
      d3dvp.MinZ = 0;
      d3dvp.MaxZ = 1;
      DXErr(DXDev->SetViewport(&d3dvp));
      DXErr(DXDev->SetRenderState(D3DRS_SCISSORTESTENABLE,0));
      DXErr(DXDev->Clear(0,0,flags,color,1.0f,0));
      DXErr(DXDev->SetRenderState(D3DRS_SCISSORTESTENABLE,1));
      d3dvp.X = sGFXViewRect.x0;
      d3dvp.Y = sGFXViewRect.y0;
      d3dvp.Width = sGFXViewRect.SizeX();
      d3dvp.Height = sGFXViewRect.SizeY();
      DXErr(DXDev->SetViewport(&d3dvp));
    }
    else
    {
      DXErr(DXDev->Clear(0,0,flags,color,1.0f,0));
    }
  }
  else
  {
    DXErr(DXDev->SetRenderState(D3DRS_SCISSORTESTENABLE,0));
    DXErr(DXDev->Clear(0,0,f[flags],color,1.0f,0));
    DXErr(DXDev->SetRenderState(D3DRS_SCISSORTESTENABLE,1));
  }

#if STATS
  Stats.Clears++;
#endif
//  sLogF(L"gfx",L"clear %08x (%d)\n",color,flags);
}

void sResolveTargetPrivate()
{
  if(DXActiveRT && DXActiveRT->MultiSurf2D 
    && DXActiveRT->MultiSurf2D!=DXActiveRT->Surf2D
    && !(DXActiveRTFlags&sRTF_NO_MULTISAMPLING)
    && !(DXActiveRT->Flags & sTEX_NORESOLVE))
  {
    IDirect3DSurface9 *dest = DXActiveRT->Surf2D;
    if(dest==0)
    {
      DXErr(DXActiveRT->Tex2D->GetSurfaceLevel(0,&dest));
    }
    else
    {
      dest->AddRef();
    }
    DXErr(DXDev->StretchRect(DXActiveRT->MultiSurf2D,0,dest,0,D3DTEXF_NONE));
    dest->Release();
  }

  if(DXActiveZB && DXActiveZB->MultiSurf2D 
    && DXActiveZB->MultiSurf2D!=DXActiveZB->Surf2D
    && !(DXActiveRTFlags&sRTF_NO_MULTISAMPLING)
    && !(DXActiveZB->Flags & sTEX_NORESOLVE))
  {
    sFatal(L"can't resove depth buffer");
  }
}

void sSetRendertargetPrivate(sTextureBase *tex,sInt xs,sInt ys,const sRect *vrp,sInt flags,IDirect3DSurface9 *cubesurface=0)
{
  // figure out zbuffer, new version
  // never use main zbuffer for rendertargets because of fsaa
  // never assume rendertargets can be as big as screen: IPP-algos need no zbuffer

  sVERIFY(Render3DInProgress);

  sTexture2D *zb = 0;
  if((flags&sRTZBUF_MASK)!=sRTZBUF_NONE)
  {
    if(tex==DXBackBuffer)
      zb = DXZBuffer;
    else
      zb = DXZBufferRT;
  }

  sBool change = 0;
  sInt miplevel = (flags&0xff00)>>8;
  sInt changemsaa = (DXActiveRTFlags ^ flags) & sRTF_NO_MULTISAMPLING;

  // color buffer

  if(DXActiveRT!=tex || changemsaa)
  {
    // done with old rendertarget

    sRelease(DXTargetSurf);

    // resolve

    sResolveTargetPrivate();

    // figure out surface

    if(cubesurface)
    {
      DXTargetSurf = cubesurface;
      DXTargetSurf->AddRef();
    }
    else
    {
      sVERIFY(tex);
      sVERIFY((tex->Flags & sTEX_TYPE_MASK)==sTEX_2D);
      if(tex->MultiSurf2D && !(flags & sRTF_NO_MULTISAMPLING))
      {
        sVERIFY(miplevel==0);
        DXTargetSurf = tex->MultiSurf2D;
        DXTargetSurf->AddRef();
      }
      else if(tex->Surf2D)
      {
        sVERIFY(miplevel==0);
        DXTargetSurf = tex->Surf2D;
        DXTargetSurf->AddRef();
      }
      else
      {
        DXErr(tex->Tex2D->GetSurfaceLevel(miplevel,&DXTargetSurf));
      }
    }

    // and set

    DXErr(DXDev->SetRenderTarget(0,DXTargetSurf));
    DXActiveRT = tex;
    change = 1;
  }

  // depth buffer

  if(DXActiveZB!=zb || changemsaa)
  {
    // done with olde one

    sRelease(DXTargetZSurf);

    // TODO: resolve

    // figure out dx9 surface

    if(zb)
    {
      sVERIFY((zb->Flags & sTEX_TYPE_MASK)==sTEX_2D);
      if(zb->MultiSurf2D && !(flags & sRTF_NO_MULTISAMPLING))
      {
        DXTargetZSurf = zb->MultiSurf2D;
        DXTargetZSurf->AddRef();
      }
      else if(zb->Surf2D)
      {
        DXTargetZSurf = zb->Surf2D;
        DXTargetZSurf->AddRef();
      }
      else
      {
        DXErr(zb->Tex2D->GetSurfaceLevel(0,&DXTargetZSurf));
      }
    }

    // and set. this may be null-ptr

    DXErr(DXDev->SetDepthStencilSurface(DXTargetZSurf));
    DXActiveZB = zb;
    change = 1;
  }
  DXActiveRTFlags = flags;

  // deactivate color targets 1..3

  if(DXActiveMultiRT)
  {
    for(sInt i=1;i<4;i++)
      DXErr(DXDev->SetRenderTarget(i,0));
    DXActiveMultiRT = 0;
    change = 1;
  }

  // window and scissor

  if(sGFXRendertargetX!=xs || sGFXRendertargetY!=ys)
  {
    sGFXRendertargetX = xs;
    sGFXRendertargetY = ys;
  }

  if(tex!=DXBackBuffer)
    sGFXRendertargetAspect = sF32(xs)/sF32(ys);
  else
    sGFXRendertargetAspect = DXScreenMode.Aspect;

  sRect vr;
  if(vrp==0)
  {
    vr.Init(0,0,xs,ys);
    vrp = &vr;
  }
  if(change || *vrp!=sGFXViewRect)
  {
    D3DVIEWPORT9 d3dvp;
    d3dvp.X = vrp->x0;
    d3dvp.Y = vrp->y0;
    d3dvp.Width = vrp->SizeX();
    d3dvp.Height = vrp->SizeY();
    d3dvp.MinZ = 0;
    d3dvp.MaxZ = 1;
    DXErr(DXDev->SetViewport(&d3dvp));

    sGFXViewRect = *vrp;
    DXErr(DXDev->SetScissorRect((RECT*)(&sGFXViewRect)));
  }
}

void sSetRendertarget(const sRect *vrp, sInt clearflags, sU32 clearcolor)
{
  sSetRendertargetPrivate(DXBackBuffer,DXScreenMode.ScreenX,DXScreenMode.ScreenY,vrp,clearflags | sRTZBUF_FORCEMAIN);

  sClearRendertargetPrivate(clearflags,clearcolor);
}

void sSetRendertarget(const sRect *vrp,sTexture2D *tex,sInt flags, sU32 clearcolor)
{
  if(tex)
  {
    if((flags & sRTZBUF_MASK)==sRTZBUF_FORCEMAIN)
      flags = (flags & ~sRTZBUF_MASK) | sRTZBUF_DEFAULT;
    sSetRendertargetPrivate(tex,tex->SizeX,tex->SizeY,vrp,flags);
    sClearRendertargetPrivate(flags,clearcolor);
  }
  else
  {
    sSetRendertarget(vrp,flags,clearcolor);
  }
}

void sSetRendertarget(const sRect *vrp,sInt flags,sU32 clearcolor,sTexture2D **tex,sInt count)
{
  sVERIFY(count>=2);
  sVERIFY(tex);
  sVERIFY(tex[0]);
  for(sInt i=1;i<count;i++)
  {
    sVERIFY(tex[0]->SizeX==tex[i]->SizeX);
    sVERIFY(tex[0]->SizeY==tex[i]->SizeY);
    sVERIFY(tex[0]->BitsPerPixel==tex[i]->BitsPerPixel);    // most old cards require this!
  }

  IDirect3DSurface9 *dest;

  sInt miplevel = (flags&0xff00)>>8;
  if((flags & sRTZBUF_MASK)==sRTZBUF_FORCEMAIN)
    flags = (flags & ~sRTZBUF_MASK) | sRTZBUF_DEFAULT;

  DXActiveMultiRT = 0;    // setting this to 0 will prevent sSetRenderTargetPrivate() from resetting the mrt's, saving some state changes.
  for(sInt i=0;i<count;i++)
  {
    if(i==0)
    {
      sSetRendertargetPrivate(tex[i],tex[i]->SizeX,tex[i]->SizeY,vrp,flags);
    }
    else
    {
      DXErr(tex[i]->Tex2D->GetSurfaceLevel(miplevel,&dest));
      DXErr(DXDev->SetRenderTarget(i,dest));
      sRelease(dest);
    }
  }
  for(sInt i=count;i<4;i++)
    DXErr(DXDev->SetRenderTarget(i,0));
  DXActiveMultiRT = 1;    // setting this to 1 will make the next sSetRendertargetPrivate() reset the mrt's

  sClearRendertargetPrivate(flags,clearcolor);
}


void sSetRendertargetCube(sTextureCube* tex, sTexCubeFace cf, sInt flags, sU32 clearcolor)
{
  IDirect3DSurface9 *dest;

  sVERIFY(tex);
    
  DXErr(tex->TexCube->GetCubeMapSurface(static_cast<D3DCUBEMAP_FACES >(cf),0,&dest));

  DXActiveRT = 0;   // caching does not care about cubemap surfaces. chaching is not that important
  DXActiveRTFlags = 0;

  sSetRendertargetPrivate(tex,tex->SizeXY,tex->SizeXY,0,flags,dest);
  sClearRendertargetPrivate(flags,clearcolor);
  sRelease(dest);
}

sTexture2D *sGetCurrentFrontBuffer() { return 0; }
sTexture2D *sGetCurrentBackBuffer() { return DXBackBuffer; }
sTexture2D *sGetCurrentBackZBuffer() { return DXZBuffer; }

void sGrabScreen(class sTexture2D *tex, sGrabFilterFlags filter, const sRect *dst, const sRect *src)
{
  IDirect3DSurface9 *surface;
  IDirect3DSurface9 *source;

  source = DXBackBuffer->Surf2D;
  if(DXBackBuffer->MultiSurf2D && !(DXActiveRTFlags & sRTF_NO_MULTISAMPLING))
    source = DXBackBuffer->MultiSurf2D;

  const sInt PointMask = (D3DPTFILTERCAPS_MINFPOINT|D3DPTFILTERCAPS_MAGFPOINT);
  const sInt LinearMask = (D3DPTFILTERCAPS_MINFPOINT|D3DPTFILTERCAPS_MAGFPOINT);

  sVERIFY((filter==sGFF_NONE) || 
          ((filter==sGFF_POINT)&&(DXCaps.StretchRectFilterCaps&PointMask)==PointMask) ||
          ((filter==sGFF_LINEAR)&&(DXCaps.StretchRectFilterCaps&LinearMask)==LinearMask));

  DXErr(tex->Tex2D->GetSurfaceLevel(0,&surface));
  DXErr(DXDev->StretchRect(source,(const RECT*)src,surface,(const RECT*)dst,(D3DTEXTUREFILTERTYPE)filter));

  surface->Release();
}

void sSetScreen(class sTexture2D *tex, sGrabFilterFlags filter, const sRect *dst, const sRect *src)
{
  IDirect3DSurface9 *surface;
  IDirect3DSurface9 *dest;

  dest = DXBackBuffer->Surf2D;

  const sInt PointMask = (D3DPTFILTERCAPS_MINFPOINT|D3DPTFILTERCAPS_MAGFPOINT);
  const sInt LinearMask = (D3DPTFILTERCAPS_MINFPOINT|D3DPTFILTERCAPS_MAGFPOINT);

  sVERIFY((filter==sGFF_NONE) || 
          ((filter==sGFF_POINT)&&(DXCaps.StretchRectFilterCaps&PointMask)==PointMask) ||
          ((filter==sGFF_LINEAR)&&(DXCaps.StretchRectFilterCaps&LinearMask)==LinearMask));

  DXErr(tex->Tex2D->GetSurfaceLevel(0,&surface));
  DXErr(DXDev->StretchRect(surface,(const RECT*)src,dest,(const RECT*)dst,(D3DTEXTUREFILTERTYPE)filter));

  sRelease(DXTargetSurf);
  sRelease(DXTargetZSurf);
  DXActiveRT = 0;
  DXActiveZB = 0;

  surface->Release();
}

void sSetScreen(const sRect &rect,sU32 *data)
{
  IDirect3DSurface9* screen;
  D3DLOCKED_RECT lock;
  D3DSURFACE_DESC desc;
  RECT sr;
  POINT dp;
  sInt xs,ys;
  sU8 *s,*d;

  // check and prepare

  screen = DXBackBuffer->Surf2D;
  DXErr(screen->GetDesc(&desc));
  sVERIFY(desc.Format==D3DFMT_X8R8G8B8);

  xs = rect.SizeX();
  ys = rect.SizeY();

  sr.left = 0;
  sr.top = 0;
  sr.right = xs;
  sr.bottom = ys;
  dp.x = rect.x0;
  dp.y = rect.y0;

  // create buffer (if required)

  if(DXSetScreenSurf==0 || DXSetScreenX!=xs || DXSetScreenY!=ys)
  {
    sRelease(DXSetScreenSurf);
    DXErr(DXDev->CreateOffscreenPlainSurface(xs,ys,D3DFMT_X8R8G8B8,D3DPOOL_SYSTEMMEM,&DXSetScreenSurf,0));
    DXSetScreenX = xs;
    DXSetScreenY = ys;
  }

  // copy to memory surface

  DXErr(DXSetScreenSurf->LockRect(&lock,&sr,0));
  s = (sU8 *)data;
  d = (sU8 *)lock.pBits;
  for(sInt y=0;y<ys;y++)
  {
    sCopyMem(d,s,xs*4);
    s += xs*4;
    d += lock.Pitch;
  }
  DXErr(DXSetScreenSurf->UnlockRect());

  // copy to screen

  DXErr(DXDev->UpdateSurface(DXSetScreenSurf,&sr,screen,&dp));
}
#endif

/****************************************************************************/
/****************************************************************************/

void sSetTarget(const sTargetPara &para)
{
  IDirect3DSurface9 *dest;
  sTextureBase *tex;
  sVERIFY(Render3DInProgress);
  sBool change = 0;

  // clear flags

  static sInt dxcf[4] = 
  {
    0,
    D3DCLEAR_TARGET,
                    D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,
    D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,
  };

  sInt cf = 0;
  if(para.Depth==0)
    cf = dxcf[para.Flags & 1];
  else
    cf = dxcf[para.Flags & 3];
  if(para.Depth && ( (para.Depth->Flags & sTEX_FORMAT)==sTEX_PCF16 || (para.Depth->Flags & sTEX_FORMAT)==sTEX_DEPTH16NOREAD ))
    cf &= ~D3DCLEAR_STENCIL;


  // count rendertargets

  sInt count = 0;
  while(para.Target[count] && count<4) count++;
  sVERIFY(count>0);

  // will we overwrite all?

  sInt oac = 1;
  sInt oad = 1;
  if(!(para.Flags & sST_OVERWRITEALL))
  {
    if(!(para.Flags & sST_CLEARCOLOR))
      oac = 0;
    if(para.Depth && !(para.Flags & sST_CLEARDEPTH))
      oad = 0;
  }

  // multiple rendertarget: check for same size

  for(sInt i=1;i<count;i++)
  {
    sVERIFY(para.Target[0]->SizeX==para.Target[i]->SizeX);
    sVERIFY(para.Target[0]->SizeY==para.Target[i]->SizeY);
    sVERIFY(para.Target[0]->BitsPerPixel==para.Target[i]->BitsPerPixel);    // most old cards require this!
  }

  // check for MSAA

  sBool msaa = 1;
  if(para.Flags & sST_NOMSAA)       // really don't want msaa
    msaa = 0;
  if(para.Flags & sST_READZ)        // can't resolve z buffer, so we have to disable msaa
    msaa = 0;
  for(sInt i=0;i<count;i++)         // do we have multisampled textures?
    if(para.Target[0]->MultiSurf2D==0)
      msaa = 0;
  if(para.Depth && para.Depth->MultiSurf2D==0)
    msaa = 0;
  if(para.Mipmap)                   // multisampled mipmaps? no way
    msaa = 0;
  if(!oac                           // we have already resolved, and we don't want to blow it up again!
  &&!(para.Target[0]->ResolveFlags & sTextureBasePrivate::RF_MultiValid)
  && (para.Target[0]->ResolveFlags & sTextureBasePrivate::RF_SingleValid))
    msaa = 0;
  if(!oad                           // same for depth buffer
  &&!(para.Depth->ResolveFlags & sTextureBasePrivate::RF_MultiValid)
  && (para.Depth->ResolveFlags & sTextureBasePrivate::RF_SingleValid))
    msaa = 0;

  // set all targets we need

  sInt i=0;
  while(i<count)
  {
    tex = para.Target[i];

    // find surface

    dest = 0;
    if(msaa)
    {
      dest = tex->MultiSurf2D;
      dest->AddRef();
    }
    else if(tex->Surf2D)
    {
      sResolveTargetPrivate(tex);
      sVERIFY(para.Mipmap==0);
      dest = tex->Surf2D;
      dest->AddRef();
    }
    else
    {
      sResolveTargetPrivate(tex);
      switch(tex->Flags & sTEX_TYPE_MASK)
      {
      case sTEX_CUBE:
        DXErr(tex->TexCube->GetCubeMapSurface(D3DCUBEMAP_FACES(para.Cubeface),para.Mipmap,&dest));
        break;
      case sTEX_2D:
        DXErr(tex->Tex2D->GetSurfaceLevel(para.Mipmap,&dest));
        break;
      default:
        sVERIFYFALSE;
      }
    }

    // caching of surface for level 0

    if(i==0)
    {
      if(DXTargetSurf!=dest)
      {
        sRelease(DXTargetSurf);
        DXTargetSurf = dest;
        DXErr(DXDev->SetRenderTarget(i,dest));
        change = 1;
      }
      else
      {
        sRelease(dest);
      }
    }
    else
    {
      DXErr(DXDev->SetRenderTarget(i,dest));
      sRelease(dest);
      change = 1;
    }

    // update stats

    if(msaa)
      tex->ResolveFlags = sTextureBasePrivate::RF_MultiValid | sTextureBasePrivate::RF_Resolve;
    else
      tex->ResolveFlags = sTextureBasePrivate::RF_SingleValid;

    // next slot

    i++;
  }
  DXActiveRT = para.Target[0];

  // clear all targets we don't need

  while(i<DXActiveMultiRT)
  {
    DXErr(DXDev->SetRenderTarget(i,0));
    i++;
  }

  // know how much we need to clear next time

  DXActiveMultiRT = count;

  // find zbuffer surface

  tex = para.Depth;

  dest = 0;
  if(tex)
  {
    if(msaa)
    {
      dest = tex->MultiSurf2D;
      dest->AddRef();
    }
    else if(tex->Surf2D)
    {
      sResolveTargetPrivate(tex);
      dest = tex->Surf2D;
      dest->AddRef();
    }
    else
    {
      sResolveTargetPrivate(tex);
      DXErr(tex->Tex2D->GetSurfaceLevel(0,&dest));
    }

    // update stats

    if(msaa)
    {
      tex->ResolveFlags = sTextureBasePrivate::RF_MultiValid;
      if(para.Flags & sST_READZ)
        tex->ResolveFlags |= sTextureBasePrivate::RF_Resolve;
    }
    else
    {
      tex->ResolveFlags = sTextureBasePrivate::RF_SingleValid;
    }
  }
  DXActiveZB = tex;

  // set zbuffer, with caching

  if(DXTargetZSurf!=dest)
  {
    sRelease(DXTargetZSurf);
    DXTargetZSurf = dest;
    DXErr(DXDev->SetDepthStencilSurface(dest));
    change = 1;
  }
  else
  {
    sRelease(dest);
  }

  // clearing without scissor

  if(cf && !(para.Flags & sST_SCISSOR))
  {
    change = 1;
    D3DVIEWPORT9 d3dvp;
    d3dvp.X = 0;
    d3dvp.Y = 0;
    d3dvp.Width = para.Target[0]->SizeX;
    d3dvp.Height = para.Target[0]->SizeY;
    d3dvp.MinZ = 0;
    d3dvp.MaxZ = 1;
    DXErr(DXDev->SetViewport(&d3dvp));
    DXErr(DXDev->SetRenderState(D3DRS_SCISSORTESTENABLE,0));
    DXErr(DXDev->Clear(0,0,cf,para.ClearColor[0].GetColor(),para.ClearZ,0));
    DXErr(DXDev->SetRenderState(D3DRS_SCISSORTESTENABLE,1));
  }

  // CHAOS: this is wrong. get rid of sGFXRendertargetAspect and the like, it will never work!

  sGFXRendertargetX = para.Target[0]->SizeX;
  sGFXRendertargetY = para.Target[0]->SizeY;

  if(para.Target[0]==sGetScreenColorBuffer())
    sGFXRendertargetAspect = sGetScreenAspect();  
  else
    sGFXRendertargetAspect = sF32(sGFXRendertargetX)/sGFXRendertargetY;

  // window and scissor

  if(change || para.Window!=sGFXViewRect)
  {
    sGFXViewRect = para.Window;

    D3DVIEWPORT9 d3dvp;
    d3dvp.X = para.Window.x0;
    d3dvp.Y = para.Window.y0;
    d3dvp.Width = para.Window.SizeX();
    d3dvp.Height = para.Window.SizeY();
    d3dvp.MinZ = 0;
    d3dvp.MaxZ = 1;
    DXErr(DXDev->SetViewport(&d3dvp));
    DXErr(DXDev->SetScissorRect((RECT*)(&para.Window)));
  }

  // clear with scissor

  if(cf && (para.Flags & sST_SCISSOR))
  {
    DXErr(DXDev->Clear(0,0,cf,para.ClearColor[0].GetColor(),para.ClearZ,0));
  }
}

void sSetScissor(const sRect &r)
{
  DXErr(DXDev->SetScissorRect((RECT*)(&r)));
}

void sResolveTargetPrivate(sTextureBase *tex)
{
  if(tex
  && (tex->ResolveFlags & sTextureBasePrivate::RF_MultiValid) 
  &&!(tex->ResolveFlags & sTextureBasePrivate::RF_SingleValid) 
  && (tex->ResolveFlags & sTextureBasePrivate::RF_Resolve))
  {
    sVERIFY(tex->MultiSurf2D);
    IDirect3DSurface9 *dest = tex->Surf2D;
    if(dest)
      dest->AddRef();
    else
      DXErr(tex->Tex2D->GetSurfaceLevel(0,&dest));

    DXErr(DXDev->StretchRect(tex->MultiSurf2D,0,dest,0,D3DTEXF_NONE));
    tex->ResolveFlags |= sTextureBasePrivate::RF_SingleValid;
    dest->Release();
  }
}

void sResolveTarget()
{
  sResolveTargetPrivate(DXActiveRT);
//  sResolveTargetPrivate(DXActiveZB);
}

void sCopyTexture(const sCopyTexturePara &para)
{
  IDirect3DSurface9 *ss,*ds;
  sTextureBase *st,*dt;

  st = para.Source;
  dt = para.Dest;

  // figure source surface

  if((st->Flags & sTEX_TYPE_MASK)==sTEX_2D)
  {
    if(st->MultiSurf2D 
    && (st->ResolveFlags & sTextureBasePrivate::RF_MultiValid)
    &&!(st->ResolveFlags & sTextureBasePrivate::RF_SingleValid))
    {
      ss = st->MultiSurf2D;
      ss->AddRef();
    }
    else if(st->Surf2D)
    {
      ss = st->Surf2D;
      ss->AddRef();
    }
    else
    {
      DXErr(st->Tex2D->GetSurfaceLevel(0,&ss));
    }
  }
  else if((st->Flags & sTEX_TYPE_MASK)==sTEX_CUBE)
  {
    DXErr(st->TexCube->GetCubeMapSurface(D3DCUBEMAP_FACES(para.SourceCubeface),0,&ss));
  }
  else
  {
    sVERIFYFALSE;
  }

  // figure dest surface

  if((dt->Flags & sTEX_TYPE_MASK)==sTEX_2D)
  {
    if(dt->Surf2D)
    {
      ds = dt->Surf2D;
      ds->AddRef();
    }
    else
    {
      DXErr(dt->Tex2D->GetSurfaceLevel(0,&ds));
    }
  }
  else if((dt->Flags & sTEX_TYPE_MASK)==sTEX_CUBE)
  {
    DXErr(dt->TexCube->GetCubeMapSurface(D3DCUBEMAP_FACES(para.SourceCubeface),0,&ds));
  }
  else
  {
    sVERIFYFALSE;
  }

  // flags

  D3DTEXTUREFILTERTYPE filter = D3DTEXF_NONE;
  if(para.Flags & sCT_FILTER)
    filter = D3DTEXF_LINEAR;

  // do it

  DXErr(DXDev->StretchRect(ss,(RECT *)&para.SourceRect,ds,(RECT *)&para.DestRect,filter));
  sRelease(ss);
  sRelease(ds);

  dt->ResolveFlags = sTextureBasePrivate::RF_SingleValid;
}

/****************************************************************************/

sGpuToCpu::sGpuToCpu(sInt flags,sInt xs,sInt ys)
{
  sVERIFY(xs>0 && ys>0);
  sVERIFY((flags & sTEX_TYPE_MASK)==sTEX_2D);

  SizeX = xs;
  SizeY = ys;
  Flags = flags;
  Dest = 0;
  Locked = 0;
  SrcTex = 0;

  D3DPOOL pool;
  D3DFORMAT fmt;
  sInt usage,mm,lflags;
  ConvertFlags(flags,fmt,pool,usage,mm,lflags);

  DXFormat = (sInt) fmt;

  DXErr(DXDev->CreateOffscreenPlainSurface(SizeX,SizeY,(D3DFORMAT)DXFormat,D3DPOOL_SYSTEMMEM,&Dest,0));
}

sGpuToCpu::~sGpuToCpu()
{
  sVERIFY(!Locked);
  sRelease(Dest);
}

void sGpuToCpu::CopyFrom(sTexture2D *tex,sInt miplevel)
{
  sVERIFY(!Locked);
  sResolveTargetPrivate(tex);

  SrcTex = tex;
  SrcMipLevel = miplevel;
  if(1)
  {
    if(tex->Surf2D)
    {
      sVERIFY(miplevel==0);
      DXErr(DXDev->GetRenderTargetData(tex->Surf2D,Dest));
    }
    else
    {
      IDirect3DSurface9 *source;
      DXErr(tex->Tex2D->GetSurfaceLevel(miplevel,&source));
  //    DXErr(DXDev->StretchRect(source,0,Dest,0,D3DTEXF_NONE));
      DXErr(DXDev->GetRenderTargetData(source,Dest));
      sRelease(source);
    }
  }
}

const void *sGpuToCpu::BeginRead(sDInt &pitch)
{
  sVERIFY(!Locked);

  if(0)
  {
    sTexture2D *tex = SrcTex;
    if(tex->Surf2D)
    {
      sVERIFY(SrcMipLevel==0);
      DXErr(DXDev->GetRenderTargetData(tex->Surf2D,Dest));
    }
    else
    {
      IDirect3DSurface9 *source;
      DXErr(tex->Tex2D->GetSurfaceLevel(SrcMipLevel,&source));
  //    DXErr(DXDev->StretchRect(source,0,Dest,0,D3DTEXF_NONE));
      DXErr(DXDev->GetRenderTargetData(source,Dest));
      sRelease(source);
    }
    SrcTex = 0;
  }

  Locked = 1;

  D3DLOCKED_RECT r;
  Dest->LockRect(&r, 0, D3DLOCK_READONLY);

  pitch = r.Pitch;

  return r.pBits;
}

void sGpuToCpu::EndRead()
{
  sVERIFY(Locked);
  Locked = 0;

  Dest->UnlockRect();
}

/****************************************************************************/

sInt sGetScreenCount()
{
  return DXScreenCount;
}

sTexture2D *sGetScreenColorBuffer(sInt screen)
{
  sVERIFY(screen>=0 && screen<DXScreenCount);
  return DXBackBuffer[screen];
}

sTexture2D *sGetScreenDepthBuffer(sInt screen)
{
  return DXZBuffer;
}

sTexture2D *sGetRTDepthBuffer()
{
  return DXZBufferRT;
}

void sEnlargeRTDepthBuffer(sInt xs,sInt ys)
{
  sInt rtx = sMax(sMax(xs,DXScreenMode.RTZBufferX),DXScreenMode.ScreenX);
  sInt rty = sMax(sMax(ys,DXScreenMode.RTZBufferY),DXScreenMode.ScreenY);
  if(DXDev)
  {
    if(rtx!=DXScreenMode.RTZBufferX || rty!=DXScreenMode.RTZBufferY)
    {
      sLogF(L"gfx",L"zbufferrt enlarged, this is very bad. %dx%d -> %dx%d\n",DXScreenMode.RTZBufferX,DXScreenMode.RTZBufferY,rtx,rty);
      sDelete(DXZBufferRT);
      DXZBufferRT = new sTexture2D(rtx,rty,sTEX_2D|sTEX_DEPTH24NOREAD|sTEX_NOMIPMAPS|sTEX_RENDERTARGET,1);
      DXScreenMode.RTZBufferX = xs;
      DXScreenMode.RTZBufferY = ys;
    }
  }
  else
  {
    DXScreenMode.RTZBufferX = rtx;
    DXScreenMode.RTZBufferY = rty;
  }
}

/****************************************************************************/
/****************************************************************************/

void sBeginSaveRTPrivate(const sU8*& data, sS32& pitch, enum sTextureFlags& flags,IDirect3DSurface9* source);

void sBeginSaveRT(const sU8*& data, sS32& pitch, enum sTextureFlags& flags)
{
  IDirect3DSurface9*  source;
  DXErr(DXDev->GetRenderTarget(0, &source));

  sBeginSaveRTPrivate(data,pitch,flags,source);
}

void sEndSaveRT()
{
  DXRTSave->UnlockRect();
}

void sBeginReadTexture(const sU8*& data, sS32& pitch, enum sTextureFlags& flags,sTexture2D *tex)
{
  sResolveTargetPrivate(tex);
  IDirect3DSurface9 *source;
  if(tex->Surf2D)
  {
    source = tex->Surf2D;
    source->AddRef();
  }
  else
  {
    DXErr(tex->Tex2D->GetSurfaceLevel(0,&source));
  }

  D3DLOCKED_RECT r;
  sInt xs,ys;
//  D3DSURFACE_DESC desc;
  xs = tex->SizeX;
  ys = tex->SizeY;
 
  if(xs!=DXRTSaveXSize || ys!=DXRTSaveYSize)
  {
    sRelease(DXRTSave);
    sRelease(DXRTSave2);
  }

  if(!DXRTSave)
  {
    DXErr(DXDev->CreateOffscreenPlainSurface(xs,ys,(D3DFORMAT)tex->DXFormat,D3DPOOL_SYSTEMMEM,&DXRTSave,0));
    DXRTSaveXSize = xs;
    DXRTSaveYSize = ys;
  }
  if(tex->MultiSurf2D)  // why? we have already resolved!
  {
    if(!DXRTSave2)
      DXErr(DXDev->CreateRenderTarget(xs,ys,(D3DFORMAT)tex->DXFormat,D3DMULTISAMPLE_NONE,0,0,&DXRTSave2,0));
    DXErr(DXDev->StretchRect(source,0,DXRTSave2,0,D3DTEXF_NONE));
    DXErr(DXDev->GetRenderTargetData(DXRTSave2,DXRTSave));
  }
  else
  {
    DXErr(DXDev->GetRenderTargetData(source,DXRTSave));
  }
  DXRTSave->LockRect(&r, 0, D3DLOCK_READONLY);

  data = (sU8*)r.pBits;
  pitch = r.Pitch;
  flags = sTextureFlags(tex->Flags&sTEX_FORMAT);

  sRelease(source);
}

void sEndReadTexture()
{
  DXRTSave->UnlockRect();
}

void sBeginSaveRTPrivate(const sU8*& data, sS32& pitch, enum sTextureFlags& flags,IDirect3DSurface9* source)
{
  D3DLOCKED_RECT      r;
  sInt xs,ys;

  D3DSURFACE_DESC desc;
  DXErr(source->GetDesc(&desc));
  xs = desc.Width;
  ys = desc.Height;

  switch(desc.Format)
  {
    case D3DFMT_X8R8G8B8:
    case D3DFMT_A8R8G8B8:       flags = sTEX_ARGB8888;break;
    case D3DFMT_Q8W8V8U8:       flags = sTEX_QWVU8888;break;
    
    case D3DFMT_G16R16:         flags = sTEX_GR16;    break;
    case D3DFMT_A16B16G16R16:   flags = sTEX_ARGB16;  break;

    case D3DFMT_R32F:           flags = sTEX_R32F;    break;
    case D3DFMT_G32R32F:        flags = sTEX_GR32F;   break;
    case D3DFMT_A32B32G32R32F:  flags = sTEX_ARGB32F; break;
    case D3DFMT_R16F:           flags = sTEX_R16F;    break;
    case D3DFMT_G16R16F:        flags = sTEX_GR16F;   break;
    case D3DFMT_A16B16G16R16F:  flags = sTEX_ARGB16F; break;

    case D3DFMT_A8:             flags = sTEX_A8;      break;
    case D3DFMT_L8:             flags = sTEX_I8;      break;
    case D3DFMT_A1R5G5B5:       flags = sTEX_ARGB1555;break;
    case D3DFMT_A4R4G4B4:       flags = sTEX_ARGB4444;break;
    case D3DFMT_R5G6B5:         flags = sTEX_RGB565;  break;
    default: sVERIFYFALSE;
  }

  if(xs!=DXRTSaveXSize || ys!=DXRTSaveYSize)
  {
    sRelease(DXRTSave);
    sRelease(DXRTSave2);
  }

  if(!DXRTSave)
  {
    DXErr(DXDev->CreateOffscreenPlainSurface(xs,ys,desc.Format,D3DPOOL_SYSTEMMEM,&DXRTSave,0));
    DXRTSaveXSize = xs;
    DXRTSaveYSize = ys;
  }
  if(desc.MultiSampleType!=D3DMULTISAMPLE_NONE)
  {
    if(!DXRTSave2)
      DXErr(DXDev->CreateRenderTarget(xs,ys,desc.Format,D3DMULTISAMPLE_NONE,0,0,&DXRTSave2,0));
    DXErr(DXDev->StretchRect(source,0,DXRTSave2,0,D3DTEXF_NONE));
    DXErr(DXDev->GetRenderTargetData(DXRTSave2,DXRTSave));
  }
  else
  {
    DXErr(DXDev->GetRenderTargetData(source,DXRTSave));
  }
  DXRTSave->LockRect(&r, 0, D3DLOCK_READONLY);

  data = (sU8*)r.pBits;
  pitch = r.Pitch;

  sRelease(source);
}

void sGetGraphicsStats(sGraphicsStats &stat)
{
  stat = BufferedStats;
}

void sEnableGraphicsStats(sBool enable)
{
  if(!enable && StatsEnable)
  {
    DisabledStats = Stats;
    StatsEnable = 0;
  }
  if(enable && !StatsEnable)
  {
    Stats = DisabledStats;
    StatsEnable = 1;
  }
}


void sSetTexture(sInt binding, class sTextureBase* tex)
{
  sInt stage = binding & sMTB_SAMPLERMASK;
  sInt cache = stage;
  if((binding & sMTB_SHADERMASK)==sMTB_VS)
  {
    stage += D3DVERTEXTEXTURESAMPLER0;
    cache += sMTRL_MAXPSTEX;
  }
  if(CurrentTexture[cache] != tex)
  {
#if STATS
    Stats.TexChanges[cache] ++;
    Stats.AllTexChanges++;
#endif
    if(tex)
    {
      DXErr(DXDev->SetTexture(stage,tex->TexBase));
    }
    else
    {
      DXErr(DXDev->SetTexture(stage,0));
    }
    CurrentTexture[cache] = tex;
  }
}

void sDbgPaintWireFrame(sBool enable)
{
  DXErr(DXDev->SetRenderState(D3DRS_FILLMODE,enable?D3DFILL_WIREFRAME:D3DFILL_SOLID));
}

void sSetRenderStates(const sU32* data, sInt count)
{
  sInt index;
  sU32 value;

  for(sInt i=0;i<count;i++)
  {
    index = *data++;
    value = *data++;

    // overwrite sRGB convertion for switching ldr <-> hdr rendering
    if (index >= sMS_SAMPLERSTATE && index<sMS_SAMPLEREND && (((index-sMS_SAMPLERSTATE)&(sMS_SAMPLEROFFSET-1))) == D3DSAMP_SRGBTEXTURE)
    {
      if (ConvertsRGB==sFALSE)
        value = 0;
    }

    if(sDXStates[index]!=value)
    {
      sDXStates[index] = value;
      if(index>=sMS_RENDERSTATE && index<sMS_RENDEREND)
      {
        DXErr(DXDev->SetRenderState((D3DRENDERSTATETYPE)index,value));
      }
      else if(index>=sMS_SAMPLERSTATE && index<sMS_SAMPLEREND)
      {
        index = index-sMS_SAMPLERSTATE;
        sInt sampler = index>>sMS_SAMPLERSHIFT;
        sInt state = index & (sMS_SAMPLEROFFSET-1);
        if(sampler>=16)
          sampler = sampler-16+D3DVERTEXTEXTURESAMPLER0;
        DXErr(DXDev->SetSamplerState(sampler,(D3DSAMPLERSTATETYPE)state,value));
      }
      /*
      else if(index>=sMS_TEXTURESTATE && index<sMS_TEXTUREMAX)
      {
        index = index-sMS_TEXTURESTATE;
        DXErr(DXDev->SetTextureStageState(index>>sMS_TEXTURESHIFT,(D3DTEXTURESTAGESTATETYPE)(index&(sMS_TEXTUREOFFSET-1)),value));
      }
      */
    }
  }
}

sInt sRenderStateTexture(sU32* data, sInt texstage, sU32 tflags,sF32 lodbias)
{
  sInt tex = sMS_SAMPLERSTATE+(texstage<<sMS_SAMPLERSHIFT);
  sU32* start = data;

  if (tflags & sMTF_SRGB)
  {
    *data++ = tex+D3DSAMP_SRGBTEXTURE; *data++ = 1;
  }
  else
  {
    *data++ = tex+D3DSAMP_SRGBTEXTURE; *data++ = 0;
  }

  switch(tflags & sMTF_LEVELMASK)
  {
  case sMTF_LEVEL0:
    *data++ = tex+D3DSAMP_MAGFILTER;    *data++ = D3DTEXF_POINT;
    *data++ = tex+D3DSAMP_MINFILTER;    *data++ = D3DTEXF_POINT;
    *data++ = tex+D3DSAMP_MIPFILTER;    *data++ = D3DTEXF_POINT;
    break;
  case sMTF_LEVEL1:
    *data++ = tex+D3DSAMP_MAGFILTER;    *data++ = D3DTEXF_LINEAR;
    *data++ = tex+D3DSAMP_MINFILTER;    *data++ = D3DTEXF_LINEAR;
    *data++ = tex+D3DSAMP_MIPFILTER;    *data++ = D3DTEXF_POINT;
    break;
  case sMTF_LEVEL2:
    *data++ = tex+D3DSAMP_MAGFILTER;    *data++ = D3DTEXF_LINEAR;
    *data++ = tex+D3DSAMP_MINFILTER;    *data++ = D3DTEXF_LINEAR;
    *data++ = tex+D3DSAMP_MIPFILTER;    *data++ = D3DTEXF_LINEAR;
    break;
  case sMTF_LEVEL3:
    if(DXCaps.TextureFilterCaps&D3DPTFILTERCAPS_MAGFANISOTROPIC)
    {
      *data++ = tex+D3DSAMP_MAGFILTER;    *data++ = D3DTEXF_ANISOTROPIC;
      *data++ = tex+D3DSAMP_MAXANISOTROPY; *data++ = DXCaps.MaxAnisotropy;
    }
    else
    {
      *data++ = tex+D3DSAMP_MAGFILTER;    *data++ = D3DTEXF_LINEAR;
    }
    if(DXCaps.TextureFilterCaps&D3DPTFILTERCAPS_MINFANISOTROPIC)
    {
      *data++ = tex+D3DSAMP_MINFILTER;    *data++ = D3DTEXF_ANISOTROPIC;
      *data++ = tex+D3DSAMP_MAXANISOTROPY; *data++ = DXCaps.MaxAnisotropy;
    }
    else
    {
      *data++ = tex+D3DSAMP_MINFILTER;    *data++ = D3DTEXF_LINEAR;
    }
    *data++ = tex+D3DSAMP_MIPFILTER;    *data++ = D3DTEXF_LINEAR;
    break;
  }
  *data++ = tex+D3DSAMP_MIPMAPLODBIAS; *(sF32 *)data = lodbias; data++;

  sU32 bcolor = (tflags&sMTF_BCOLOR_WHITE) ? 0xffffffff : 0;

  *data++ = tex+D3DSAMP_ADDRESSU;
  switch(tflags&sMTF_ADDRMASK_U)
  {
  case sMTF_TILE_U:   *data++ = D3DTADDRESS_WRAP;   break;
  case sMTF_CLAMP_U:  *data++ = D3DTADDRESS_CLAMP;  break;
  case sMTF_MIRROR_U: *data++ = D3DTADDRESS_MIRROR; break;
  case sMTF_BORDER_U: 
    *data++ = D3DTADDRESS_BORDER;
    *data++ = tex+D3DSAMP_BORDERCOLOR;  
    *data++ = bcolor;
    break;
  default:
    sVERIFYFALSE;
  }

  *data++ = tex+D3DSAMP_ADDRESSV;
  switch(tflags&sMTF_ADDRMASK_V)
  {
  case sMTF_TILE_V:   *data++ = D3DTADDRESS_WRAP;   break;
  case sMTF_CLAMP_V:  *data++ = D3DTADDRESS_CLAMP;  break;
  case sMTF_MIRROR_V: *data++ = D3DTADDRESS_MIRROR; break;
  case sMTF_BORDER_V: 
    *data++ = D3DTADDRESS_BORDER;
    *data++ = tex+D3DSAMP_BORDERCOLOR;  
    *data++ = bcolor;
    break;
  default:
    sVERIFYFALSE;
  }

  *data++ = tex+D3DSAMP_ADDRESSW;
  switch(tflags&sMTF_ADDRMASK_W)
  {
  case sMTF_TILE_W:   *data++ = D3DTADDRESS_WRAP;   break;
  case sMTF_CLAMP_W:  *data++ = D3DTADDRESS_CLAMP;  break;
  case sMTF_MIRROR_W: *data++ = D3DTADDRESS_MIRROR; break;
  case sMTF_BORDER_W: 
    *data++ = D3DTADDRESS_BORDER;
    *data++ = tex+D3DSAMP_BORDERCOLOR;  
    *data++ = bcolor;
    break;
  default:
    sVERIFYFALSE;
  }

  return (data-start);
}


/****************************************************************************/
/***                                                                      ***/
/***   Vertex Formats                                                     ***/
/***                                                                      ***/
/****************************************************************************/


void sVertexFormatHandle::Create()
{
  // create vertex declarator 
  D3DVERTEXELEMENT9 decl[32];
  sInt i,b[sVF_STREAMMAX];
  sInt stream;

  for(i=0;i<sVF_STREAMMAX;i++)
    b[i] = 0;

  sBool dontcreate=sFALSE;
  i = 0;
  sInt j = 0;
  while(Data[i])
  {
    stream = (Data[i]&sVF_STREAMMASK)>>sVF_STREAMSHIFT;

    decl[i].Stream = stream;
    decl[i].Offset = b[stream];
    decl[i].Method = 0;

    sVERIFY(i<31);

    switch(Data[i]&sVF_USEMASK)
    {
    case sVF_NOP:   break;
    case sVF_POSITION:    decl[j].Usage = D3DDECLUSAGE_POSITION;      decl[j].UsageIndex = 0;  j++; break;
    case sVF_NORMAL:      decl[j].Usage = D3DDECLUSAGE_NORMAL;        decl[j].UsageIndex = 0;  j++; break;
    case sVF_TANGENT:     decl[j].Usage = D3DDECLUSAGE_TANGENT;       decl[j].UsageIndex = 0;  j++; break;
    case sVF_BONEINDEX:   decl[j].Usage = D3DDECLUSAGE_BLENDINDICES;  decl[j].UsageIndex = 0;  j++; break;
    case sVF_BONEWEIGHT:  decl[j].Usage = D3DDECLUSAGE_BLENDWEIGHT;   decl[j].UsageIndex = 0;  j++; break;
    case sVF_BINORMAL:    decl[j].Usage = D3DDECLUSAGE_BINORMAL;      decl[j].UsageIndex = 0;  j++; break;
    case sVF_COLOR0:      decl[j].Usage = D3DDECLUSAGE_COLOR;         decl[j].UsageIndex = 0;  j++; break;
    case sVF_COLOR1:      decl[j].Usage = D3DDECLUSAGE_COLOR;         decl[j].UsageIndex = 1;  j++; break;
    case sVF_COLOR2:      decl[j].Usage = D3DDECLUSAGE_COLOR;         decl[j].UsageIndex = 2;  j++; break;
    case sVF_COLOR3:      decl[j].Usage = D3DDECLUSAGE_COLOR;         decl[j].UsageIndex = 3;  j++; break;
    case sVF_UV0:         decl[j].Usage = D3DDECLUSAGE_TEXCOORD;      decl[j].UsageIndex = 0;  j++; break;
    case sVF_UV1:         decl[j].Usage = D3DDECLUSAGE_TEXCOORD;      decl[j].UsageIndex = 1;  j++; break;
    case sVF_UV2:         decl[j].Usage = D3DDECLUSAGE_TEXCOORD;      decl[j].UsageIndex = 2;  j++; break;
    case sVF_UV3:         decl[j].Usage = D3DDECLUSAGE_TEXCOORD;      decl[j].UsageIndex = 3;  j++; break;
    case sVF_UV4:         decl[j].Usage = D3DDECLUSAGE_TEXCOORD;      decl[j].UsageIndex = 4;  j++; break;
    case sVF_UV5:         decl[j].Usage = D3DDECLUSAGE_TEXCOORD;      decl[j].UsageIndex = 5;  j++; break;
    case sVF_UV6:         decl[j].Usage = D3DDECLUSAGE_TEXCOORD;      decl[j].UsageIndex = 6;  j++; break;
    case sVF_UV7:         decl[j].Usage = D3DDECLUSAGE_TEXCOORD;      decl[j].UsageIndex = 7;  j++; break;
    default: sVERIFYFALSE;
    }

    switch(Data[i]&sVF_TYPEMASK)
    {
    case sVF_F2:  decl[i].Type = D3DDECLTYPE_FLOAT2;    b[stream]+=2*4;  break;
    case sVF_F3:  decl[i].Type = D3DDECLTYPE_FLOAT3;    b[stream]+=3*4;  break;
    case sVF_F4:  decl[i].Type = D3DDECLTYPE_FLOAT4;    b[stream]+=4*4;  break;
    case sVF_I4:  decl[i].Type = D3DDECLTYPE_UBYTE4;    b[stream]+=1*4;  break;
    case sVF_C4:  decl[i].Type = D3DDECLTYPE_D3DCOLOR;  b[stream]+=1*4;  break;
    case sVF_S2:  decl[i].Type = D3DDECLTYPE_SHORT2N;   b[stream]+=1*4;  break;
    case sVF_S4:  decl[i].Type = D3DDECLTYPE_SHORT4N;   b[stream]+=2*4;  break;
    case sVF_H2:  decl[i].Type = D3DDECLTYPE_FLOAT16_2; b[stream]+=1*4;  break;
    case sVF_H4:  decl[i].Type = D3DDECLTYPE_FLOAT16_4; b[stream]+=2*4;  break;
    case sVF_F1:  decl[i].Type = D3DDECLTYPE_FLOAT1;    b[stream]+=1*4;  break;
    default: sVERIFYFALSE;
    }

    AvailMask |= 1 << (Data[i]&sVF_USEMASK);
    Streams = sMax(Streams,stream+1);
    i++;
  }
  decl[j].Stream = 0xff;
  decl[j].Offset = 0;
  decl[j].Type = D3DDECLTYPE_UNUSED;
  decl[j].Method = 0;
  decl[j].Usage = 0;
  decl[j].UsageIndex = 0;

  for(sInt i=0;i<sVF_STREAMMAX;i++)
    VertexSize[i] = b[i];

  if(dontcreate)
    Decl = 0L;
  else
    DXErr(DXDev->CreateVertexDeclaration(decl,&Decl));
}

void sVertexFormatHandle::Destroy()
{
  if(Decl)
    Decl->Release();
}

/*
sInt sSizeofVertexFormat(sVertexFormatHandle *handle)
{
  return handle->VertexSize;
}

sInt sVertexFormatHas(sVertexFormatHandle *handle,sInt flags)
{
  return (handle->AvailMask & (1<<(flags & sVF_USEMASK))) != 0;
}
*/
/****************************************************************************/
/***                                                                      ***/
/***   Shader Interface                                                   ***/
/***                                                                      ***/
/****************************************************************************/

void sCreateShader2(sShader *shader, sShaderBlob *blob)
{
  if((blob->Type&sSTF_PLATFORM)!=sSTF_HLSL23)
  {
    sLogF(L"gfx",L"sCreateShader2 only supports hlsl shader model 2 and 3\n");
    return;
  }

  // can this be a directx shader?
  const sU32 *start = (const sU32 *) (blob->Data);
  const sU32 *end = (const sU32 *) (blob->Data+blob->Size-4);
  sVERIFY((blob->Size&3)==0);
  sVERIFY(*end==0x0000ffff);

  switch(shader->Type&sSTF_KIND)
  {
  case sSTF_VERTEX:
    sVERIFY(((*start)&0xffff0000)==0xfffe0000);
    DXErr(DXDev->CreateVertexShader((const DWORD *)blob->Data,&shader->vs));
    break;
  case sSTF_PIXEL:
    sVERIFY(((*start)&0xffff0000)==0xffff0000);
    DXErr(DXDev->CreatePixelShader((const DWORD *)blob->Data,&shader->ps));
    break;
  default:
    sVERIFYFALSE;
  }
}

void sDeleteShader2(sShader *shader)
{
  switch(shader->Type&sSTF_KIND)
  {
  case sSTF_VERTEX:
    if(shader->vs)
      shader->vs->Release();
    shader->vs = 0;
    break;
  case sSTF_PIXEL:
    if(shader->ps)
      shader->ps->Release();
    shader->ps = 0;
    break;
  default:
    sVERIFYFALSE;
  }
}

/****************************************************************************/

void sSetVSParam(sInt o, sInt count, const sVector4* vsf)
{
  DXErr(DXDev->SetVertexShaderConstantF(o,&vsf->x,count));
  sClearCurrentCBuffers();  // cbuffers are not valid anymore
}

void sSetPSParam(sInt o, sInt count, const sVector4* psf)
{
  DXErr(DXDev->SetPixelShaderConstantF (o,&psf->x,count));
  sClearCurrentCBuffers();  // cbuffers are not valid anymore
}

void sSetVSBool(sU32 bits,sU32 mask)
{
  for(sInt i=0;i<sCOUNTOF(CurrentVSBools);i++)
  {
    if(mask&(1<<i))
      CurrentVSBools[i] = (bits&(1<<i)) ? 1 : 0;
  }
  if(DXShaderProfile>=sSTF_DX_20)
    DXErr(DXDev->SetVertexShaderConstantB(0,CurrentVSBools,sCOUNTOF(CurrentVSBools)));

  sClearCurrentCBuffers();  // cbuffers are not valid anymore
}

void sSetPSBool(sU32 bits,sU32 mask)
{
  for(sInt i=0;i<sCOUNTOF(CurrentPSBools);i++)
  {
    if(mask&(1<<i))
      CurrentPSBools[i] = (bits&(1<<i)) ? 1 : 0;
  }
  if(DXShaderProfile>=sSTF_DX_30)
    DXErr(DXDev->SetPixelShaderConstantB(0,CurrentPSBools,sCOUNTOF(CurrentPSBools)));

  sClearCurrentCBuffers();  // cbuffers are not valid anymore
}

sShaderTypeFlag sGetShaderPlatform()
{
  return sSTF_HLSL23;
}

sInt sGetShaderProfile()
{
  return DXShaderProfile;
}

/****************************************************************************/


sCBufferBase::sCBufferBase()
{
  RegStart = 0;
  RegCount = 0;
  DataPtr = 0;
  DataPersist = 0;
  Slot = 0;
  Flags = 0;
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
  switch(Slot & sCBUFFER_SHADERMASK)
  {
  case sCBUFFER_VS:
    DXErr(DXDev->SetVertexShaderConstantF(RegStart,(sF32*)DataPersist,RegCount));
    break;
  case sCBUFFER_PS:
    DXErr(DXDev->SetPixelShaderConstantF(RegStart,(sF32*)DataPersist,RegCount));
    break;
  }
  // should be valid for more than one frame
  //if(DataPtr)
  //  *DataPtr = 0;
}

void sCBufferBase::OverridePtr(void *ptr)
{
  DataPtr = 0;
  DataPersist = ptr;
  Modify();
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
/***   Geometry Buffers                                                   ***/
/***                                                                      ***/
/****************************************************************************/

void sGeoBuffer::Init()
{
  sClear(*this);
}

void sGeoBuffer::Exit()
{
  sRelease(VB);
  sRelease(IB);
}

void sGeoBuffer::Reset()
{
  sVERIFY(!LockPtr);
  sRelease(VB);
  sRelease(IB);

  Duration = sGD_NONE;
  BufferType = 0;
  Alloc = 0;
  Used = 0;
//  RefCount = 0;         // do not reset refcount. the GeoBufferPart will still have a pointer to this and decrease the refcount!
  Discard = 0;
  InUse = 0;
  LockPtr = 0;
}

void sGeoBuffer::Lock()
{
  sVERIFY(LockPtr==0);
  sGeoBufferLocks++;

  sInt lock = (Duration==sGD_STATIC) ? 0 : D3DLOCK_NOOVERWRITE;
  if(Discard)
  {
    lock = D3DLOCK_DISCARD;
    Discard = 0;
  }

  switch(BufferType)
  {
  case 0:
    DXErr(VB->Lock(0,0,&LockPtr,lock));
    break;
  case 1:
  case 2:
    DXErr(IB->Lock(0,0,&LockPtr,lock));
    break;
  }
}

void sGeoBuffer::Unlock()
{
  sVERIFY(LockPtr);
  sGeoBufferLocks--;
  LockPtr = 0;
  switch(BufferType)
  {
  case 0:
    DXErr(VB->Unlock());
    break;
  case 1:
  case 2:
    DXErr(IB->Unlock());
    break;
  }
}

sInt sGeoBuffer::CheckSize(sInt count,sInt size)
{
  sInt pos = (Used+size-1)/size;
  sInt start = pos*size;
  sInt end = start + count*size;
  if(Duration == sGD_STREAM)
  {
    if(count*size<=Alloc)
    {
      if(end>Alloc)
      {
       // Unlock();
        Discard = 1;
        Used = 0;
        pos = 0;
      }
      else
      {
        Used = start;
      }
      return pos;
    }
  }
  else
  {
    if(end<=Alloc)
    {
      Used = start;
      return pos;
    }
  }
  return -1;
}

/****************************************************************************/


static void DumpGeoBuffers(sBool all=sFALSE)
{
  sLogF(L"gfx",L"===> %d <===\n",sGeoBufferCount);
  for(sInt i=0;i<sGeoBufferCount;i++)
  {
    if(sGeoBuffers[i].RefCount || all)
    {
      sLogF(L"gfx",L"%3d %s %s : used %d alloc %d refs %d\n",i,
        sGeoBuffers[i].Duration==sGD_STATIC ? L"stat" : L"dyn ",
        sGeoBuffers[i].BufferType==0 ? L"VB" : L"IB",
        sGeoBuffers[i].Used,sGeoBuffers[i].Alloc,sGeoBuffers[i].RefCount);
    }
  }
}

void sGeoBufferInit()
{
  sGeoBufferCount = 0;
  sGeoBufferLocks = 0;
  sSetMem(sGeoBuffers,0x00,sizeof(sGeoBuffers));
}

void sGeoBufferExit()
{
  //DumpGeoBuffers();
  static const sChar *s[] = { L"???",L"STATIC",L"FRAME",L"STREAM"};
  for(sInt i=0;i<sGeoBufferCount;i++)
  {
    if(sGeoBuffers[i].RefCount!=0/* && sGeoBuffers[i].Duration==sGD_STATIC*/)
      sLogF(L"gfx",L"*** GeoBuffers not freed: alloc %08x dur %d refcount %d !\n",
        sGeoBuffers[i].Alloc,s[sGeoBuffers[i].Duration],sGeoBuffers[i].RefCount);
    sGeoBuffers[i].Exit();
  }
  sGeoBufferCount = 0;
}

void sGeoBufferReset0()
{
  for(sInt i=0;i<sGeoBufferCount;i++)
  {
    sGeoBuffer *gb = &sGeoBuffers[i];
    if(gb->LockPtr && gb->Duration!=sGD_DYNAMIC)
      gb->Unlock();
    if(gb->Duration==sGD_STREAM || gb->Duration==sGD_FRAME || gb->Duration==sGD_DYNAMIC)
      gb->Reset();
  }
  sVERIFY(sGeoBufferLocks == 0);
}

sGeoBuffer *sFindFreeGeoBuffer()
{
  sGeoBuffer *gb;
  for(sInt i=0;i<sGeoBufferCount;i++)
  {
    gb = &sGeoBuffers[i];
    if(gb->Alloc==0 && gb->RefCount==0)
      return gb;
  }

#if sDEBUG
  if(sGeoBufferCount>=sCOUNTOF(sGeoBuffers))
    DumpGeoBuffers(sTRUE);
#endif
  sVERIFY(sGeoBufferCount < sCOUNTOF(sGeoBuffers));
  return &sGeoBuffers[sGeoBufferCount++];
}

void sGeoBufferReset1()
{
}

void sGeoBufferFrame()    // called in sRender3DEnd()
{
  sGeoBufferUnlockAll();
  for(sInt i=0;i<sGeoBufferCount;i++)
  {
    sGeoBuffer *gb = &sGeoBuffers[i];
    if(gb->Duration!=sGD_DYNAMIC)
    {
      sVERIFY(gb->LockPtr==0);
      if(gb->Duration==sGD_STREAM)
      {
        gb->Discard = 1;
        gb->Used = gb->Alloc;
      }
      if(gb->Duration==sGD_FRAME)
      {
        gb->Discard = 1;
        gb->Used = 0;
      }
    }
  }

  for(sInt i=0;i<sGeoPrimCount;i++)
    sGeoPrims[i].VB.Clear();
  sGeoPrimCount = 0;
  sGeoBuffersLast[0] = 0;
  sGeoBuffersLast[1] = 0;
  sGeoBuffersLast[2] = 0;
}

void sGeoBufferUnlockAll()
{
  if(sGeoBufferLocks>0)
  {
    for(sInt i=0;i<sGeoBufferCount;i++)
    {
      sGeoBuffer *gb = &sGeoBuffers[i];
      if(gb->LockPtr && gb->Duration!=sGD_DYNAMIC)
        gb->Unlock();
    }
  }
}

/****************************************************************************/

sGeoBufferPart::sGeoBufferPart()
{
  Buffer = 0;
  Start = 0;
  Count = 0;
}

sGeoBufferPart::~sGeoBufferPart()
{
  Clear();
}

void sGeoBufferPart::Clear()
{
  if(Buffer)
  {
    Buffer->RefCount--;
    sVERIFY(Buffer->RefCount>=0);
    if(Buffer->RefCount == 0 && Buffer->Duration==sGD_STATIC)
      Buffer->Used = 0;
    Buffer = 0;
  }
  Start = 0;
  Count = 0;
}

sBool sGeoBufferPart::IsEmpty()
{
  return Buffer==0;
}

void sGeoBufferPart::CloneFrom(sGeoBufferPart *src)
{
  Clear();
  Buffer = src->Buffer;
  Start = src->Start;
  Count = src->Count;
  if(src->Buffer)
    Buffer->RefCount++;
}

void *sGeoBufferPart::Init(sInt count,sInt size,sGeometryDuration duration,sInt buffertype,sInt advance)
{
  sGeoBuffer *gb;

  Clear();

  Buffer = 0;
  sInt pos = 0;

  // shortcut: does the last one fit?

  gb = sGeoBuffersLast[buffertype];
  if(gb && gb->Alloc>0 && gb->Duration==duration && !gb->InUse)
  {
    pos = gb->CheckSize(count,size);
    if(pos>=0)
      Buffer = gb;
  }

  // search all buffers

  sGeoBuffer *reuse=0;
  for(sInt i=0;i<sGeoBufferCount && Buffer==0;i++)
  {
    gb = &sGeoBuffers[i];
    if(gb->Alloc>0 && gb->Duration==duration && gb->BufferType==buffertype && !gb->InUse)
    {
      pos = gb->CheckSize(count,size);
      if(pos>=0)
        Buffer = gb;
    }
    // check for reuseable buffers
    if(!gb->RefCount && gb->BufferType==buffertype)
    {
      if(!reuse || gb->Alloc > reuse->Alloc)
        reuse = gb;
    }
  }

  // not found, create one!

  if(Buffer == 0)
  {
    D3DPOOL pool;
    D3DFORMAT format;
    DWORD usage;

    if(reuse)
    {
      if(reuse->LockPtr)      // we should unlock locked geometrybuffers without references before reusing/reseting them
        reuse->Unlock();      // this happens with geometries created and destroyd within one frame
      reuse->Reset();
      gb = reuse;
    }
    else
      gb = sFindFreeGeoBuffer();

    gb->BufferType = buffertype;
    gb->Duration = duration;

    gb->Alloc = 0;
    gb->Used = 0;  pos = 0;
    sVERIFY(gb->RefCount==0);
    gb->RefCount = 0;
    gb->Discard = 0;
    gb->LockPtr = 0;
    gb->InUse = 0;

    format = D3DFMT_UNKNOWN;
    if(duration==sGD_STATIC)
    {
      usage = D3DUSAGE_WRITEONLY;
      pool = D3DPOOL_MANAGED;
    }
    else
    {
      usage = D3DUSAGE_WRITEONLY|D3DUSAGE_DYNAMIC;
      pool = D3DPOOL_DEFAULT;
    }

    gb->Alloc = count*size;
    if(duration!=sGD_DYNAMIC)
      gb->Alloc = sMax((buffertype==0)?sMAX_VBCOUNT:sMAX_IBCOUNT,count)*size;

    switch(buffertype)
    {
    case 0:
      DXErr(DXDev->CreateVertexBuffer(gb->Alloc,usage,0,pool,&gb->VB,0));
      break;
    case 1:
      DXErr(DXDev->CreateIndexBuffer(gb->Alloc,usage,D3DFMT_INDEX16,pool,&gb->IB,0));
      break;
    case 2:
      DXErr(DXDev->CreateIndexBuffer(gb->Alloc,usage,D3DFMT_INDEX32,pool,&gb->IB,0));
      break;
    }


    Buffer = gb;
  }

  // done

  sGeoBuffersLast[buffertype] = Buffer;
  Start = pos;
  Count = count;

  if(Buffer->LockPtr==0)
    Buffer->Lock();
  sU8 *data = (((sU8*)Buffer->LockPtr) + Buffer->Used);

  if(advance)
    Buffer->Used += Count*size;
  else
    Buffer->InUse = 1;
  Buffer->RefCount++;

  return (void *) data;

}

void sGeoBufferPart::Advance(sInt count,sInt size)
{
  Buffer->InUse = 0;
  if(count!=-1)
  {
    sVERIFY(count<=Count);
    Count = count;
  }
  Buffer->Used += Count*size;
  sVERIFY(Buffer->Used <= Buffer->Alloc);
}

/****************************************************************************/
/***                                                                      ***/
/***   Geometry Begin/End interface                                       ***/
/***                                                                      ***/
/****************************************************************************/

void sGeometry::Init(sInt flags,sVertexFormatHandle *form)
{
  sVERIFY(!(flags&sGF_CPU_MEM));
  Flags = flags;
  Format = form;

  switch(Flags & sGF_INDEXMASK)
  {
    case sGF_INDEX16:  IndexSize = 2; break;
    case sGF_INDEX32:  IndexSize = 4; break;
    default: IndexSize = 0;
  }
}

void sGeometry::BeginLoadIB(sInt ic,sGeometryDuration duration,void **ip)
{
  sVERIFY(IndexSize>0)
  IndexPart.Clear();
  *ip = IndexPart.Init(ic,IndexSize,duration,(Flags & sGF_INDEX32)?2:1);
}

void sGeometry::EndLoadIB(sInt ic)
{
  IndexPart.Advance(ic,IndexSize);
}

void sGeometry::BeginLoadVB(sInt vc,sGeometryDuration duration,void **vp,sInt stream)
{
  VertexPart[stream].Clear();
  *vp = VertexPart[stream].Init(vc,Format->GetSize(stream),duration,0);
}

void sGeometry::EndLoadVB(sInt vc,sInt stream)
{
  VertexPart[stream].Advance(vc,Format->GetSize(stream));
}

void sGeometry::BeginLoad(sVertexFormatHandle *vf,sInt flags,sGeometryDuration duration,sInt vc,sInt ic,void **vp,void **ip)
{
  Init(flags,vf);
  BeginLoadVB(vc,duration,vp,0);
  if(ic)
    BeginLoadIB(ic,duration,ip);
  else
    IndexPart.Clear();
}

void sGeometry::EndLoad(sInt vc,sInt ic)
{
  VertexPart[0].Advance(vc,Format->GetSize(0));
  if(IndexPart.Buffer)
    IndexPart.Advance(ic,IndexSize);
}

// obsolete

void sGeometry::BeginLoad(sInt vc,sInt ic,sInt flags,sVertexFormatHandle *vf,void **vp,void **ip)
{
  if(!(flags & sGF_INDEX32))
    flags |= sGF_INDEX16;

  Init(flags&(sGF_PRIMMASK|sGF_INDEXMASK|sGF_INSTANCES),vf);
  BeginLoadVB(vc,sGeometryDuration(flags&3),vp,0);
  if(ic)
    BeginLoadIB(ic,sGeometryDuration((flags>>4)&3),ip);
  else
    IndexPart.Clear();
}

/****************************************************************************/

void sGeometry::Merge(sGeometry *geo0,sGeometry *geo1)
{
  sVERIFY(geo1!=this);
  if(geo0 != this)
  {
    for(sInt i=0;i<sVF_STREAMMAX;i++)
      VertexPart[i].CloneFrom(&geo0->VertexPart[i]);
    IndexPart.CloneFrom(&geo0->IndexPart);
  }

  if(geo1)
  {
    for(sInt i=0;i<sVF_STREAMMAX;i++)
      if(!geo1->VertexPart[i].IsEmpty())
        VertexPart[i].CloneFrom(&geo1->VertexPart[i]);
    if(!geo1->IndexPart.IsEmpty())
      IndexPart.CloneFrom(&geo1->IndexPart);
  }
}

void sGeometry::MergeVertexStream(sInt DestStream,sGeometry *src,sInt SrcStream)
{
  sVERIFY(src!=this);
  VertexPart[DestStream].CloneFrom(&src->VertexPart[SrcStream]);
}

/****************************************************************************/
/***                                                                      ***/
/***   Dynamic Management                                                 ***/
/***                                                                      ***/
/****************************************************************************/

void sGeometry::InitDyn(sInt ic,sInt vc0,sInt vc1,sInt vc2,sInt vc3)
{
  if(ic) { IndexPart.Init(ic,IndexSize,sGD_DYNAMIC,IndexSize==2 ? 1 : 2,0); IndexPart.Buffer->Unlock(); }
  if(vc0) { VertexPart[0].Init(vc0,Format->GetSize(0),sGD_DYNAMIC,0,0); VertexPart[0].Buffer->Unlock(); }
  if(vc1) { VertexPart[1].Init(vc1,Format->GetSize(1),sGD_DYNAMIC,0,0); VertexPart[0].Buffer->Unlock(); }
  if(vc2) { VertexPart[2].Init(vc2,Format->GetSize(2),sGD_DYNAMIC,0,0); VertexPart[0].Buffer->Unlock(); }
  if(vc3) { VertexPart[3].Init(vc3,Format->GetSize(3),sGD_DYNAMIC,0,0); VertexPart[0].Buffer->Unlock(); }
}

void *sGeometry::BeginDynVB(sBool discard,sInt stream)
{
  if(discard) VertexPart[stream].Buffer->Discard = 1;
  VertexPart[stream].Buffer->Lock();
  return VertexPart[stream].Buffer->LockPtr;
}

void *sGeometry::BeginDynIB(sBool discard)
{
  if(discard) IndexPart.Buffer->Discard = 1;
  IndexPart.Buffer->Lock();
  return IndexPart.Buffer->LockPtr;
}

void sGeometry::EndDynVB(sInt stream)
{
  VertexPart[stream].Buffer->Unlock();
}

void sGeometry::EndDynIB()
{
  IndexPart.Buffer->Unlock();
}

/****************************************************************************/
/***                                                                      ***/
/***   Geometry Prim Interface                                            ***/
/***                                                                      ***/
/****************************************************************************/

void sGeometry::BeginQuadrics()
{
  sVERIFY(!PrimMode);
  sVERIFY(Flags==sGF_QUADRICS);
  PrimMode = 1;
  FirstPrim = 0;
  CurrentPrim = 0;
  LastPrimPtr = &FirstPrim;
}

void sGeometry::EndQuadrics()
{
  sVERIFY(PrimMode);
  PrimMode = 0;
  sGeoPrim *prim = FirstPrim;
  while(prim)
  {
    if(prim->VB.Buffer->LockPtr)
    {
      prim->VB.Buffer->Unlock();
    }
    prim = prim->Next;
  }
}

void sGeometry::BeginQuad(void **data,sInt count)
{
  sVERIFY(PrimMode)
  sVERIFY(sGeoPrimCount<sMAX_GEOPRIMS);
  sVERIFY(!CurrentPrim);
  sGeoPrim *prim = &sGeoPrims[sGeoPrimCount++];
  *LastPrimPtr = prim;
  LastPrimPtr = &prim->Next;
  CurrentPrim = prim;

  prim->Next = 0;
  prim->Mode = sGP_QUAD;
  prim->TessX = count;
  prim->TessY = 0;
  prim->VB.Clear();
  *data = prim->VB.Init(count*4,Format->GetSize(0),sGD_FRAME,0,1);
  prim->Vertex = prim->VB.Start;
}

void sGeometry::EndQuad()
{
  CurrentPrim = 0;
}

void sGeometry::BeginGrid(void **data,sInt xs,sInt ys)
{
  sVERIFY(PrimMode)
  sVERIFY(sGeoPrimCount<sMAX_GEOPRIMS);
  sVERIFY(!CurrentPrim);
  sGeoPrim *prim = &sGeoPrims[sGeoPrimCount++];
  *LastPrimPtr = prim;
  LastPrimPtr = &prim->Next;
  CurrentPrim = prim;

  prim->Next = 0;
  prim->Mode = sGP_GRID;
  prim->TessX = xs;
  prim->TessY = ys;
  prim->VB.Clear();
  *data = prim->VB.Init(xs*ys,Format->GetSize(0),sGD_FRAME,0,1);
  prim->Vertex = prim->VB.Start;
}

void sGeometry::EndGrid()
{
  CurrentPrim = 0;
}


void sGeometry::BeginWireGrid(void **data,sInt xs,sInt ys)
{
  sVERIFY(PrimMode)
  sVERIFY(sGeoPrimCount<sMAX_GEOPRIMS);
  sVERIFY(!CurrentPrim);
  sGeoPrim *prim = &sGeoPrims[sGeoPrimCount++];
  *LastPrimPtr = prim;
  LastPrimPtr = &prim->Next;
  CurrentPrim = prim;

  prim->Next = 0;
  prim->Mode = sGP_WIREGRID;
  prim->TessX = xs;
  prim->TessY = ys;
  prim->VB.Clear();
  *data = prim->VB.Init(xs*ys,Format->GetSize(0),sGD_FRAME,0,1);
  prim->Vertex = prim->VB.Start;
}

void sGeometry::EndWireGrid()
{
  CurrentPrim = 0;
}

// find ranges of sGeoPrim with same DX-VertexBuffer object
void sGeometry::DrawPrim()
{
  sGeoPrim *p,*first;
  sGeoBuffer *gb;
  sInt ic,vc;
 
  p = FirstPrim;
  first = p;
  gb = 0;
  ic = 0;
  vc = 0;
 
  while(p)
  {
    if(gb==0)
      gb = p->VB.Buffer;
    else if(gb!=p->VB.Buffer) // vertex buffer changed, draw range
    {
      if(ic>0)
        DrawPrim(first,p,ic,vc);
      first = p;
      gb = p->VB.Buffer;
      ic = 0;
      vc = 0;
    }

    switch(p->Mode)           // count how many indices we will need
    {
    case sGP_QUAD:
      ic += p->TessX*6;
      vc += p->TessX*4;
      break;
    case sGP_GRID:
      ic += (p->TessX-1)*(p->TessY-1)*6;
      vc += p->TessX*p->TessY;
      break;
    case sGP_WIREGRID:
      ic += (p->TessX*(p->TessY-1) + p->TessY*(p->TessX-1))*2;
      vc += p->TessX*p->TessY;
      break;
    }

    p = p->Next;
  }
  if(first && ic>0)
    DrawPrim(first,p,ic,vc);
}

// draw one range of sGeoPrim with same DX-VertexBuffer object
void sGeometry::DrawPrim(sGeoPrim *start,sGeoPrim *end,sInt ic,sInt vc)
{
  sGeoBufferPart IB;  
  sGeoBuffer *verify;
  sGeoPrim *prim;
  sU32 *ip;
  sInt vert,xs;

  IB.Clear();
  ip = (sU32 *)IB.Init(ic,4,sGD_FRAME,2,1);
  verify = start->VB.Buffer;
  
  prim = start;
  sInt linemode = 0;
  while(prim!=end)
  {
    sVERIFY(prim->VB.Buffer==verify);
    switch(prim->Mode)
    {
    case sGP_GRID:
      vert = prim->Vertex;
      xs = prim->TessX;
      for(sInt y=0;y<prim->TessY-1;y++)
      {
        for(sInt x=0;x<prim->TessX-1;x++)
        {
          sQuad(ip,vert+y*xs+x,0,1,1+xs,0+xs);
        }
      }
      break;
    case sGP_QUAD:
      vert = prim->Vertex;
      for(sInt i=0;i<prim->TessX;i++)
      {
        sQuad(ip,vert,0,1,2,3);
        vert+=4;
      }
      break;
    case sGP_WIREGRID:
      vert = prim->Vertex;
      xs = prim->TessX;
      for(sInt y=0;y<prim->TessY;y++)
      {
        for(sInt x=0;x<prim->TessX-1;x++)
        {
          *ip++ = vert+y*xs+x;
          *ip++ = vert+y*xs+x+1;
        }
      }
      for(sInt y=0;y<prim->TessY-1;y++)
      {
        for(sInt x=0;x<prim->TessX;x++)
        {
          *ip++ = vert+y*xs+x;
          *ip++ = vert+y*xs+x+xs;
        }
      }
      linemode = 1;
      break;
    }

    prim = prim->Next;
  }

  // now draw.

  sGeoBufferUnlockAll();    // need to unlock geobuffers before drawing

  DXErr(DXDev->SetVertexDeclaration(Format->GetDecl()));

  if(DXInstanceSet)
  {
    for(sInt i=0;i<DXInstanceSet;i++)
      DXErr(DXDev->SetStreamSourceFreq(i,D3DSTREAMSOURCE_INDEXEDDATA|1))        
    DXInstanceSet = 0;
  }

  sInt size = Format->GetSize(0);
  DXErr(DXDev->SetStreamSource(0,start->VB.Buffer->VB,0,size));
  for(sInt i=1;i<DXStreamsUsed;i++)
    DXErr(DXDev->SetStreamSource(i,0,0,0));
  DXStreamsUsed = 1;

  // set index buffer (except for quad simulation)

  DXErr(DXDev->SetIndices(IB.Buffer->IB));

  if(linemode)
  {
    DXErr(DXDev->DrawIndexedPrimitive(D3DPT_LINELIST,0,0,start->VB.Buffer->Used/size,IB.Start,ic/2)); 
  }
  else
  {
    DXErr(DXDev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,start->VB.Buffer->Used/size,IB.Start,ic/3)); 
  }

  // update stats

#if STATS
  Stats.Batches++;
  Stats.Splitter += 1;
  Stats.Primitives += ic/3;
  Stats.Vertices += vc; // inaccurate
  Stats.Indices += ic;
#endif

}

/****************************************************************************/
/***                                                                      ***/
/***   Geometry Drawing                                                   ***/
/***                                                                      ***/
/****************************************************************************/

// this routine needs lots of optimisations
// - 8 different cases for permutations DP/DIP - DrawRange - MultiVertexStream

void sGeometry::Draw()
{
  Draw(sGeometryDrawInfo());
}

void sGeometry::Draw(sDrawRange *ir,sInt irc,sInt instancecount, sVertexOffset *off)
{
  sGeometryDrawInfo di(ir,irc,instancecount,off);
  Draw(di);
}

void sGeometry::Draw(const sGeometryDrawInfo &di)
{
  sVERIFY(DontCheckMtrlPrepare || CurrentMtrlVFormat==Format);    // correct prepared material used?
  sDrawRange irdata;
  D3DPRIMITIVETYPE type;
  sInt primcount,vertcount;
#if STATS
  sInt primtotal=0,verttotal=0;
#endif
  sInt start;
  sInt streamsused;
  sInt vertexoffset,vertexcount;
  sInt quads;

  if(DebugBreak) sDEBUGBREAK;
  if(!sRELEASE)
  {
    if(!(di.Flags & sGDI_Ranges))
    {
      sVERIFY(di.RangeCount==0);
      sVERIFY(di.Ranges==0);
    }
    if(!(di.Flags & sGDI_Instances))
      sVERIFY(di.InstanceCount==0);
    if(!(di.Flags & sGDI_VertexOffset))
      for(sInt i=0;i<sVF_STREAMMAX;i++)
        sVERIFY(di.VertexOffset[0]==0);
  }

  sGeoBufferUnlockAll();
  if(Flags==sGF_QUADRICS)
  {
    sVERIFY((di.Flags&(sGDI_Ranges|sGDI_Instances|sGDI_VertexOffset))==0);
    DrawPrim();
    return ;
  }

  sVERIFY(VertexPart[0].Buffer);

  // set blendfacter

  if(di.Flags & sGDI_BlendFactor)
  {
    sU32 data[2] = { sMS_RENDERSTATE+sInt(D3DRS_BLENDFACTOR),di.BlendFactor };
    sSetRenderStates(data,1);       // this cares about caching!
  }

  // preparation for primitive type

  quads = 0;
  switch(Flags & sGF_PRIMMASK)
  {
  case sGF_TRILIST:    
    type = D3DPT_TRIANGLELIST;  
    break;
  case sGF_TRISTRIP:   
    type = D3DPT_TRIANGLESTRIP;
    break;
  case sGF_LINELIST:   
    type = D3DPT_LINELIST;      
    break;
  case sGF_LINESTRIP:  
    type = D3DPT_LINESTRIP;     
    break;
  case sGF_QUADLIST:       
    type = D3DPT_TRIANGLELIST;  
    sVERIFY(IndexPart.Buffer==0);
    sVERIFY(IndexSize==0);
    quads = 1;
    DXErr(DXDev->SetIndices(DXQuadIndex.Buffer->IB));
    break;
  case sGF_SPRITELIST:
    sFatal(L"sGF_SPRITE not implemented");
    break;
  default:    
    sVERIFYFALSE;
    break;
  }

  // preparation for drawrange

  const sDrawRange *ir = di.Ranges;
  sInt irc = di.RangeCount;

  if(!quads)                    // no quads, normal processing
  {
    if(ir==0)                  // when no drawrange is specified, fake one
    {
      ir = &irdata;
      irc = 1;

      if(IndexPart.Buffer)
      {
        irdata.Start = 0;
        irdata.End = IndexPart.Count;
      }
      else
      {
        irdata.Start = 0;
        irdata.End = VertexPart[0].Count;
      }
    }
  }
  else                          // quads, special case
  {
    sInt maxquads = DXQuadIndex.Count/6;
    if(ir==0)                   // when no drawrange is specified, split VB into sMAX_QUADCOUNT quad parts
    {
      sInt quadcount = VertexPart[0].Count/4;
      irc = (quadcount + maxquads-1) / maxquads;
      sDrawRange *ir_ = sALLOCSTACK(sDrawRange,irc);
      ir = ir_;

      for(sInt i=0;i<irc-1;i++)
      {
        ir_[i].Start = i*maxquads*4;
        ir_[i].End = (i+1)*maxquads*4;
      }
      ir_[irc-1].Start = (irc-1)*maxquads*4;
      ir_[irc-1].End = quadcount*4;
    }
    else                        // there is an indexrange, split into sMAX_QUADCOUNT quad parts
    {
      sInt irc_ = 0;
      sVERIFY(sMAX_QUADCOUNT == DXQuadIndex.Count/6);
      for(sInt i=0;i<irc;i++)
        irc_ += (ir[i].End-ir[i].Start+(sMAX_QUADCOUNT*4-1))/(sMAX_QUADCOUNT*4);

      if(irc_>irc)
      {
        sDrawRange *ir_ = sALLOCSTACK(sDrawRange,irc_);
        sInt index = 0;
        for(sInt i=0;i<irc;i++)
        {
          sInt start = ir[i].Start;
          sInt end = ir[i].End;
          sInt count = (end-start+(sMAX_QUADCOUNT*4-1))/(sMAX_QUADCOUNT*4);
          for(sInt j=0;j<count-1;j++)
          {
            ir_[index].Start = start;
            start += (sMAX_QUADCOUNT*4);
            ir_[index].End = start;
            index++;
          }
          ir_[index].Start = start;
          ir_[index].End = end;
          index++;
        }
        irc = irc_;
        ir = ir_;
      }
    }
  }

  // setup vertex streams

  DXErr(DXDev->SetVertexDeclaration(Format->GetDecl()));
  streamsused = Format->GetStreams();

  if(DXInstanceSet)
  {
    for(sInt i=0;i<DXInstanceSet;i++)
      DXErr(DXDev->SetStreamSourceFreq(i,1))    // see the well hidden comment on dxdocu "Efficiently Drawing Multiple Instances of Geometry (Direct3D 9)". use dxddebug to see how important this is!
  }
  DXInstanceSet = di.InstanceCount>0 ? streamsused : 0;

  if(streamsused==1)
  {
    sVERIFY(VertexPart[0].Buffer);
    DXErr(DXDev->SetStreamSource(0,VertexPart[0].Buffer->VB,0,Format->GetSize(0)));
    vertexoffset = VertexPart[0].Start+di.VertexOffset[0];
    vertexcount = VertexPart[0].Count-di.VertexOffset[0];
  }
  else
  {
    vertexoffset = 0;
    vertexcount = VertexPart[0].Count;
    for(sInt i=0;i<streamsused;i++)
    {
      sVERIFY(VertexPart[i].Buffer)
      DXErr(DXDev->SetStreamSource(i,VertexPart[i].Buffer->VB,Format->GetSize(i)*(VertexPart[i].Start+di.VertexOffset[i]),Format->GetSize(i)));
      if(di.InstanceCount>0)
      {
        if(i==0)        
        { DXErr(DXDev->SetStreamSourceFreq(i,D3DSTREAMSOURCE_INDEXEDDATA | di.InstanceCount)); }
        else
        { DXErr(DXDev->SetStreamSourceFreq(i,D3DSTREAMSOURCE_INSTANCEDATA | 1)); }
      }
    }
  }
  if(IndexSize==2)                            // clamp vertexcount when using 16 bit indices as only up to 0x10000 vertices can be referenced
    vertexcount = sMin(vertexcount,0x10000);  // (otherwise we get debug runtime errors with large vertex buffers on nvidia hardware (MaxVertexIndex==1048575))

  // clear unused vertex streams, if they were used last draw-call

  for(sInt i=streamsused;i<DXStreamsUsed;i++)
    DXErr(DXDev->SetStreamSource(i,0,0,0));
  DXStreamsUsed = streamsused;

  // set index buffer (except for quad simulation)

  if(IndexPart.Buffer)
  {
    DXErr(DXDev->SetIndices(IndexPart.Buffer->IB));
  }
  else if(!quads)
  {
    DXErr(DXDev->SetIndices(0));
  }

  // draw the splitters

#if STATS
  primcount = 0;
  vertcount = 0;
#endif
  for(sInt i=0;i<irc;i++)
  {
    start = ir[i].Start + IndexPart.Start;
    vertcount = ir[i].End - ir[i].Start;

    switch(Flags & sGF_PRIMMASK)
    {
      case sGF_TRILIST:     primcount = vertcount/3;    break;
      case sGF_TRISTRIP:    primcount = vertcount-2;    break;
      case sGF_LINELIST:    primcount = vertcount/2;    break;
      case sGF_LINESTRIP:   primcount = vertcount-1;    break;
      case sGF_QUADLIST:    primcount = vertcount/4*2;  break;
      default:    sVERIFYFALSE;
    }
    if(primcount==0)
    {
      sLogF(L"gfx",L"Draw call with 0 primitives\n");
      continue;
    }
#if STATS
    primtotal += primcount;
    verttotal += vertcount;
#endif

    // draw

    if(!quads)
    {
      if(IndexPart.Buffer)
      { 
        DXErr(DXDev->DrawIndexedPrimitive(type,vertexoffset,0,vertexcount,start,primcount)); 
      }
      else
      { 
        DXErr(DXDev->DrawPrimitive(type,vertexoffset+start,primcount)); 
      }
    }
    else
    {
      DXErr(DXDev->DrawIndexedPrimitive(type,vertexoffset+start,0,vertcount,0,primcount));
    }
  }

  // update stats

#if STATS
  Stats.Batches++;
  Stats.Splitter += irc;
  Stats.Primitives += primtotal*sMax(di.InstanceCount,1);
  if(IndexPart.Buffer)
    Stats.Vertices += VertexPart[0].Count*irc*sMax(di.InstanceCount,1);   // inaccurate when using index ranges
  else
    Stats.Vertices += vertcount*irc*sMax(di.InstanceCount,1);
  Stats.Indices += verttotal*sMax(di.InstanceCount,1);
#endif
}

/****************************************************************************/
/***                                                                      ***/
/***   sTextureBase                                                       ***/
/***                                                                      ***/
/****************************************************************************/

void sCopyCubeFace(sTexture2D *dest, sTextureCube *src, sTexCubeFace cf)
{
  sVERIFY( dest->SizeX == dest->SizeY && dest->SizeX == src->SizeXY );
  sVERIFY( (dest->Flags&sTEX_FORMAT) == (src->Flags&sTEX_FORMAT) );
  sVERIFY(cf != sTCF_NONE);

  D3DCUBEMAP_FACES face = (D3DCUBEMAP_FACES)cf;

  if (src->Flags&sTEX_RENDERTARGET)
  {
    // copy rendertarget
    sSetRendertargetCube(src,cf,sCLEAR_NONE);
    const sU8 *data;
    sInt pitch;
    sTextureFlags rtflags;
    sBeginSaveRT(data,pitch,rtflags);

    D3DLOCKED_RECT ldest;
    DXErr(dest->Tex2D->LockRect(0, &ldest, 0, 0));
    sVERIFY(ldest.Pitch == pitch);

    for (sInt y=0; y<dest->SizeY; y++)
    {
      sU8 *src_ptr = (sU8*)data+y*pitch;
      sU8 *dst_ptr = (sU8*)ldest.pBits+y*ldest.Pitch;
      for (sInt x=0; x<dest->SizeX*dest->BitsPerPixel/8; x++)
        *dst_ptr++ = *src_ptr++;
    }

    DXErr(dest->Tex2D->UnlockRect(0));
    sSetRendertarget(0,sCLEAR_NONE);
  }
  else
  {
    // copy normal texture
    D3DLOCKED_RECT ldest;
    D3DLOCKED_RECT lsrc;

    DXErr(dest->Tex2D->LockRect(0, &ldest, 0, 0));
    DXErr(src->TexCube->LockRect(face, 0, &lsrc, 0, 0));

    sVERIFY(ldest.Pitch == lsrc.Pitch);

    for (sInt y=0; y<dest->SizeY; y++)
    {
      sU8 *src_ptr = (sU8*)lsrc.pBits+y*lsrc.Pitch;
      sU8 *dst_ptr = (sU8*)ldest.pBits+y*ldest.Pitch;
      for (sInt x=0; x<dest->SizeX*dest->BitsPerPixel/8; x++)
        *dst_ptr++ = *src_ptr++;
    }

    DXErr(dest->Tex2D->UnlockRect(0));
    DXErr(src->TexCube->UnlockRect(face,0));
  }
}

sBool sReadTexture(sReader &s, sTextureBase *&tex)
{
  return sFALSE;
}


/****************************************************************************/
/***                                                                      ***/
/***   sOccQuery                                                          ***/
/***                                                                      ***/
/****************************************************************************/

static sOccQueryNode *GetOccQueryNode()
{
  if(FreeOccQueryNodes->IsEmpty())
  {
    sOccQueryNode *qn = new sOccQueryNode;
    DXErr(DXDev->CreateQuery(D3DQUERYTYPE_OCCLUSION,&qn->Query));
    AllOccQueryNodes->AddTail(qn);
    return qn;
  }
  else
  {
    return FreeOccQueryNodes->RemTail();
  }
}

void sFlushOccQueryNodes()
{
  while(!FreeOccQueryNodes->IsEmpty())
  {
    sOccQueryNode *qn = FreeOccQueryNodes->RemHead();
    sInt id = sFindIndex(*AllOccQueryNodes,qn);
    AllOccQueryNodes->RemAt(id);
    sRelease(qn->Query);
    sDelete(qn);
  }
  sVERIFY(AllOccQueryNodes->IsEmpty());
  AllOccQueryNodes->Reset();
}

sOccQuery::sOccQuery()
{
  Last = 1.0f;
  Average = 1.0f;
  Filter = 0.1f;
  Current = 0;
}

sOccQuery::~sOccQuery()
{
  sVERIFY(Current==0);
  for(;;)
  {
    sOccQueryNode *qn = Queries.RemTail();
    if(!qn) break;
    FreeOccQueryNodes->AddTail(qn);
  }
}

void sOccQuery::Begin(sInt pixels)
{
  Poll();
  sVERIFY(Current==0);
  Current = GetOccQueryNode();
  Current->Pixels = pixels;
  Current->Query->Issue(D3DISSUE_BEGIN);
  Queries.AddTail(Current);
}

void sOccQuery::End()
{
  sVERIFY(Current);
  Current->Query->Issue(D3DISSUE_END);
  Current = 0;
}

void sOccQuery::Poll()
{
  DWORD pixels;
  sOccQueryNode *qn = Queries.GetHead();
  while(!Queries.IsEnd(qn))
  {
    sOccQueryNode *next = Queries.GetNext(qn);
    if(qn->Query->GetData(&pixels,sizeof(DWORD),0)==S_OK)
    {
      Last = sF32(pixels)/qn->Pixels;
      Average = (1-Filter)*Average + Filter*Last;
      Queries.Rem(qn);
      FreeOccQueryNodes->AddTail(qn);
    }
    qn = next;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   sTexture2D                                                         ***/
/***                                                                      ***/
/****************************************************************************/


void ConvertFlags(sU32 flags, D3DFORMAT& d3df, D3DPOOL& pool, sInt& usage, sInt& mm, sInt& lflags)
{
  sInt rt = D3DUSAGE_RENDERTARGET;

  switch(flags&sTEX_FORMAT)
  {
    case sTEX_ARGB8888: d3df = D3DFMT_A8R8G8B8;     break;
    case sTEX_QWVU8888: d3df = D3DFMT_Q8W8V8U8;     break;
    
    case sTEX_GR16:     d3df = D3DFMT_G16R16;       break;
    case sTEX_ARGB16:   d3df = D3DFMT_A16B16G16R16; break;

    case sTEX_R32F:     d3df = D3DFMT_R32F;         break;
    case sTEX_GR32F:    d3df = D3DFMT_G32R32F;      break;
    case sTEX_ARGB32F:  d3df = D3DFMT_A32B32G32R32F;break;
    case sTEX_R16F:     d3df = D3DFMT_R16F;         break;
    case sTEX_GR16F:    d3df = D3DFMT_G16R16F;      break;
    case sTEX_ARGB16F:  d3df = D3DFMT_A16B16G16R16F;break;

    case sTEX_A8:       d3df = D3DFMT_A8;           break;
    case sTEX_I8:       d3df = D3DFMT_L8;           break;
    case sTEX_IA4:      d3df = D3DFMT_A4L4;         break;
    case sTEX_DXT1:     d3df = D3DFMT_DXT1;         break;
    case sTEX_DXT1A:    d3df = D3DFMT_DXT1;         break;
    case sTEX_DXT3:     d3df = D3DFMT_DXT3;         break;
    case sTEX_DXT5:     d3df = D3DFMT_DXT5;         break;
    case sTEX_DXT5N:    d3df = D3DFMT_DXT5;         break;
    case sTEX_ARGB1555: d3df = D3DFMT_A1R5G5B5;     break;
    case sTEX_ARGB4444: d3df = D3DFMT_A4R4G4B4;     break;
    case sTEX_RGB565:   d3df = D3DFMT_R5G6B5;       break;
    case sTEX_IA8:      d3df = D3DFMT_A8L8;         break;
    
    case sTEX_MRGB8:    d3df = D3DFMT_A8R8G8B8;     break;
    case sTEX_DXT5_AYCOCG:    d3df = D3DFMT_DXT5;   break;


    case sTEX_DEPTH16:
      {
        sGraphicsCaps caps;
        sGetGraphicsCaps(caps);

        rt = D3DUSAGE_DEPTHSTENCIL;

        switch (caps.Flags & sGCF_DEPTHTEX_MASK)
        {
        case sGCF_DEPTHTEX_INTZ:  d3df = (D3DFORMAT)MAKEFOURCC('I','N','T','Z');  break;
        case sGCF_DEPTHTEX_RAWZ:  d3df = (D3DFORMAT)MAKEFOURCC('R','A','W','Z');  break;
        default:                  d3df = (D3DFORMAT)MAKEFOURCC('D','F','1','6');  break;
        }
      }          
      break;

    case sTEX_DEPTH24:  
      {
        sGraphicsCaps caps;
        sGetGraphicsCaps(caps);

        rt = D3DUSAGE_DEPTHSTENCIL;

        switch (caps.Flags & sGCF_DEPTHTEX_MASK)
        {
        case sGCF_DEPTHTEX_INTZ:  d3df = (D3DFORMAT)MAKEFOURCC('I','N','T','Z');  break;
        case sGCF_DEPTHTEX_RAWZ:  d3df = (D3DFORMAT)MAKEFOURCC('R','A','W','Z');  break;
        default:                  d3df = (D3DFORMAT)MAKEFOURCC('D','F','2','4');  break;
        }
      }          
      break;

    case sTEX_DEPTH16NOREAD:  d3df = D3DFMT_D16;    rt = D3DUSAGE_DEPTHSTENCIL; break;
    case sTEX_DEPTH24NOREAD:  d3df = D3DFMT_D24S8;  rt = D3DUSAGE_DEPTHSTENCIL; break;
    case sTEX_PCF16:          d3df = D3DFMT_D16;    rt = D3DUSAGE_DEPTHSTENCIL; break;
    case sTEX_PCF24:          d3df = D3DFMT_D24S8;  rt = D3DUSAGE_DEPTHSTENCIL; break;
    default: 
      sLogF(L"gfx",L"invalid format %d\n",flags&sTEX_FORMAT);
      sVERIFYFALSE;
  }

  pool = D3DPOOL_MANAGED;
  usage = 0;
  lflags = 0;
  if(flags & sTEX_RENDERTARGET)
  {
    sVERIFY(!(flags & sTEX_DYNAMIC));
    sVERIFY((flags & sTEX_NOMIPMAPS) || (flags & sTEX_AUTOMIPMAP) || mm != 0); 
    pool = D3DPOOL_DEFAULT;
    usage = rt;
  }
  if(flags & sTEX_DYNAMIC)
  {
    pool = D3DPOOL_DEFAULT;
    usage = D3DUSAGE_DYNAMIC;
    mm = 1;
    lflags = D3DLOCK_DISCARD;
  }
  if(flags & sTEX_AUTOMIPMAP)   // to be used with sTEX_RENDERTARGET or sTEX_DYNAMIC
  {
    usage |= D3DUSAGE_AUTOGENMIPMAP;
    mm = 0;
  }
}

sU64 sGetAvailTextureFormats()
{
  return
    (1ULL<<sTEX_ARGB8888) |
    (1ULL<<sTEX_QWVU8888) |
    (1ULL<<sTEX_GR16) |
    (1ULL<<sTEX_ARGB16) |
    (1ULL<<sTEX_R32F) |
    (1ULL<<sTEX_GR32F) |
    (1ULL<<sTEX_ARGB32F) |
    (1ULL<<sTEX_R16F) |
    (1ULL<<sTEX_GR16F) |
    (1ULL<<sTEX_ARGB16F) |
    (1ULL<<sTEX_A8) |
    (1ULL<<sTEX_I8) |
  //    (1ULL<<sTEX_IA4) |
    (1ULL<<sTEX_DXT1) |
    (1ULL<<sTEX_DXT1A) |
    (1ULL<<sTEX_DXT3) |
    (1ULL<<sTEX_DXT5) |
    (1ULL<<sTEX_DXT5N) |
    (1ULL<<sTEX_ARGB1555) |
    (1ULL<<sTEX_ARGB4444) |
  //    (1ULL<<sTEX_RGB565) |
    (1ULL<<sTEX_DEPTH16) |
    (1ULL<<sTEX_DEPTH24) |
    (1ULL<<sTEX_PCF16) |
    (1ULL<<sTEX_PCF24) |
    (1ULL<<sTEX_DXT5_AYCOCG) |
    0;  
}


void InitGraphicsCaps()
{
  sGraphicsCaps caps;
  sClear(caps);

  D3DADAPTER_IDENTIFIER9 aid;
  if (SUCCEEDED(DX9->GetAdapterIdentifier(DX9Adapter,0,&aid)))
    sCopyString(caps.AdapterName,aid.Description);
    
  sU64 avail = sGetAvailTextureFormats();
  for(sInt i=0;i<64;i++)
  {
    if((1ULL<<i)&avail)
    {
      HRESULT hr;
      D3DFORMAT d3df;
      D3DPOOL pool;
      DX9Format = D3DFMT_X8R8G8B8;
      sInt usage,mm,lflags;
      ConvertFlags(sTEX_2D|i,d3df,pool,usage,mm,lflags);

      hr = DX9->CheckDeviceFormat(DX9Adapter,DX9DevType,DX9Format,D3DUSAGE_QUERY_VERTEXTEXTURE,D3DRTYPE_TEXTURE,d3df);
      if(D3D_OK==hr)
        caps.VertexTex2D |= 1ULL<<i;

      hr = DX9->CheckDeviceFormat(DX9Adapter,DX9DevType,DX9Format,D3DUSAGE_QUERY_VERTEXTEXTURE,D3DRTYPE_CUBETEXTURE,d3df);
      if(D3D_OK==hr)
        caps.VertexTexCube |= 1ULL<<i;

      hr = DX9->CheckDeviceFormat(DX9Adapter,DX9DevType,DX9Format,D3DUSAGE_RENDERTARGET,D3DRTYPE_TEXTURE,d3df);
      if(D3D_OK==hr)
        caps.TextureRT |= 1ULL<<i;

      hr = DX9->CheckDeviceFormat(DX9Adapter,DX9DevType,DX9Format,0,D3DRTYPE_TEXTURE,d3df);
      if(D3D_OK==hr)
        caps.Texture2D |= 1ULL<<i;

      hr = DX9->CheckDeviceFormat(DX9Adapter,DX9DevType,DX9Format,0,D3DRTYPE_CUBETEXTURE,d3df);
      if(D3D_OK==hr)
        caps.TextureCube |= 1ULL<<i;
    }
  }

  GraphicsCapsMaster = caps;
}

void sGetGraphicsCaps(sGraphicsCaps &caps)
{
  caps = GraphicsCapsMaster;
}

/****************************************************************************/
/****************************************************************************/

void sPackDXT(sU8 *d,sU32 *bmp,sInt xs,sInt ys,sInt format,sBool dither)
{
  sInt formatflags = format;
  format &= sTEX_FORMAT;
  if((formatflags & sTEX_FASTDXTC) || DXDev==0)
  { 
    sFastPackDXT(d,bmp,xs,ys,format,1 | (dither ? 0x80 : 0));
    return;
  }

#if !sCONFIG_COMPILER_GCC
  IDirect3DSurface9 *surf;
  D3DFORMAT d3dformat,srcfmt;
  sU32 *bmpdel=0;
  D3DLOCKED_RECT lr;
  RECT wr;
  sU8 *s;
  sInt blocksize;

  switch(format&sTEX_FORMAT)
  {
  case sTEX_DXT1:
    d3dformat = D3DFMT_DXT1;
    srcfmt = D3DFMT_X8R8G8B8;
    blocksize = 8;
    break;
  case sTEX_DXT1A:
    d3dformat = D3DFMT_DXT1;
    srcfmt = D3DFMT_A8R8G8B8;
    blocksize = 8;
    bmpdel = new sU32[xs*ys];
    sCopyMem(bmpdel,bmp,xs*ys*4);
    bmp = bmpdel;
    for(sInt i=0;i<xs*ys;i++)
    {
      if(bmp[i]>=0x80000000) 
        bmp[i]|=0xff000000; 
      else 
        bmp[i]&=0x00ffffff;
    }
    break;
  case sTEX_DXT3:
    d3dformat = D3DFMT_DXT3;
    srcfmt = D3DFMT_A8R8G8B8;
    blocksize = 16;
    break;
  case sTEX_DXT5:
    d3dformat = D3DFMT_DXT5;
    srcfmt = D3DFMT_A8R8G8B8;
    blocksize = 16;
    break;

  case sTEX_DXT5N:
    d3dformat = D3DFMT_DXT5;
    srcfmt = D3DFMT_A8R8G8B8;
    blocksize = 16;
    bmpdel = new sU32[xs*ys];
    for(sInt i=0;i<xs*ys;i++)
      bmpdel[i] = (bmp[i]&0x0000ff00) | ((bmp[i]&0x00ff0000)<<8);
    bmp = bmpdel;
    break;

  case sTEX_DXT5_AYCOCG:
    d3dformat = D3DFMT_DXT5;
    srcfmt = D3DFMT_A8R8G8B8;
    blocksize = 16;
    bmpdel = new sU32[xs*ys];
    for(sInt i=0;i<xs*ys;i++)
      bmpdel[i] = sARGBtoAYCoCg(bmp[i]);
    bmp = bmpdel;
    break;

  default:
    d3dformat = D3DFMT_UNKNOWN;
    srcfmt = D3DFMT_UNKNOWN;
    blocksize = 0;
    sVERIFYFALSE;
  }

  wr.top = 0;
  wr.left = 0;
  wr.right = xs;
  wr.bottom = ys;

  surf = 0;
  sVERIFY(DXDev);
  DXErr(DXDev->CreateOffscreenPlainSurface(xs,ys,d3dformat,D3DPOOL_SCRATCH,&surf,0));
  DXErr(D3DXLoadSurfaceFromMemory(surf,0,0,bmp,srcfmt,xs*4,0,&wr,D3DX_FILTER_POINT|(dither ? D3DX_FILTER_DITHER : 0),0));

  DXErr(surf->LockRect(&lr,0,D3DLOCK_READONLY));

  s = (sU8 *)lr.pBits;
  for(sInt y=0;y<ys;y+=4)
  {
    sCopyMem(d,s,xs/4*blocksize);
    s += lr.Pitch;
    d += xs/4*blocksize;
  }

  DXErr(surf->UnlockRect());
  surf->Release();

  delete[] bmpdel;  
#else
  sFastPackDXT(d,bmp,xs,ys,format,1 | (dither ? 0x80 : 0));
#endif
}

sTextureBasePrivate::sTextureBasePrivate()
{
  Tex2D = 0;
  Surf2D = 0;
  MultiSurf2D = 0;
  ResolveFlags = 0;
  DXFormat = 0;
}

void sTexture2D::Create2(sInt flags)
{
  // handle stream textures as dynamic textures
  if(Flags&sTEX_STREAM)
  {
    Flags &= ~sTEX_STREAM;
    Flags |= sTEX_DYNAMIC;
    flags &= ~sTEX_STREAM;
    flags |= sTEX_DYNAMIC;
  }

  D3DFORMAT format;
  D3DPOOL pool;
  sInt usage;

  sVERIFY((flags & sTEX_TYPE_MASK)==sTEX_2D);

  // create resource

  ConvertFlags(flags, format, pool, usage, Mipmaps, LockFlags);
  DXFormat = format;

  if((flags & sTEX_FORMAT)==sTEX_DEPTH16NOREAD || (flags & sTEX_FORMAT)==sTEX_DEPTH24NOREAD)
  {
    DXErr(DXDev->CreateDepthStencilSurface(SizeX,SizeY,format,D3DMULTISAMPLE_NONE,0,0,&Surf2D,0)); // not discardable, because that will invalidate the depth buffer the moment another one is set!
  }
  else
  {
    if(!(flags & sTEX_INTERNAL))
      DXErr(DXDev->CreateTexture(SizeX,SizeY,Mipmaps,usage,format,pool,&Tex2D,0));
  }

  if(Mipmaps==0) Mipmaps=1;   // for autogenmipmap: use 0 as argument for CreateTexture, but 1 is the correct value for later because we have one accessible mipmap.

  // in case of multisampled rendertargets, we need the multisampled buffer. this is done even for sTEX_INTERNAL textures

  if(flags & sTEX_MSAA)
  {
    sVERIFY(flags & sTEX_RENDERTARGET);

    D3DMULTISAMPLE_TYPE mst = D3DMULTISAMPLE_NONMASKABLE;
    DWORD msq = sClamp<DWORD>(DXScreenMode.MultiLevel,0,GraphicsCapsMaster.MaxMultisampleLevel-1);
    if(usage==D3DUSAGE_DEPTHSTENCIL)
    {
      D3DFORMAT fmt = D3DFMT_D24S8;
      if(BitsPerPixel==16)
        fmt = D3DFMT_D16;
      DXErr(DXDev->CreateDepthStencilSurface(SizeX,SizeY,fmt,mst,msq,1,&MultiSurf2D,0));
    }
    else
    {
      DXErr(DXDev->CreateRenderTarget(SizeX,SizeY,format,mst,msq,0,&MultiSurf2D,0));
    }
  }

  // automipmap pointfilter

  if((flags & sTEX_AUTOMIPMAP) && (flags & sTEX_AUTOMIPMAP_POINT) && Tex2D)
    DXErr(Tex2D->SetAutoGenFilterType(D3DTEXF_POINT));

  // memory stats

  sInt mem = SizeX*SizeY*BitsPerPixel/8;
  for(sInt i=0;i<Mipmaps;i++)
  {
    DXTotalTextureMem += mem;
    mem /= 4;
  }

  // register rendertargets

  if((Flags&sTEX_RENDERTARGET) || (Flags&sTEX_DYNAMIC))
  {
    sVERIFY(DXRTCount < MAX_RENDERTARGETS);
    DXRenderTargets[DXRTCount++] = this;
  }
}

void sTexture2D::Destroy2()
{
  if ((Flags&sTEX_RENDERTARGET) || (Flags&sTEX_DYNAMIC))
  {
    for (sInt i=0; i<DXRTCount; i++)
    {
      if (DXRenderTargets[i] == this)
      {
        DXRenderTargets[i] = DXRenderTargets[DXRTCount-1];
        DXRTCount--;
        break;
      }
    }
  }

  sRelease(Tex2D);
  sRelease(Surf2D);
  sRelease(MultiSurf2D);

  // memory stats

  sInt mem = SizeX*SizeY*BitsPerPixel/8;
  for(sInt i=0;i<Mipmaps;i++)
  {
    DXTotalTextureMem -= mem;
    mem /= 4;
  }
}

void sTexture2D::OnLostDevice(sBool reinit/*=sFALSE*/)
{
  if ((Flags&sTEX_RENDERTARGET) || (Flags&sTEX_DYNAMIC))
  {
    sRelease(Tex2D);
    sRelease(Surf2D);
    sRelease(MultiSurf2D);
    if (reinit) 
      Init(OriginalSizeX, OriginalSizeY, Flags, Mipmaps, sTRUE);

    FrameRT = 0xffff;
    SceneRT = 0xffff;

    // update the proxies!

    sTextureProxyNode *pr;
    sFORALL_LIST(Proxies,pr)
    {
      pr->Proxy->Disconnect2();
      pr->Proxy->Connect2();
    }
  }
}

void sTexture2D::BeginLoad(sU8 *&data,sInt &pitch,sInt mipmap)
{
  D3DLOCKED_RECT lr;
  sVERIFY(Loading==-1);

  sInt lflags = LockFlags;
  if(mipmap)                      // D3DLOCK_DISCARD is allowed only on the top level
    lflags &= ~D3DLOCK_DISCARD;
  DXErr(Tex2D->LockRect(mipmap,&lr,0,LockFlags));
  data = (sU8 *)lr.pBits;
  pitch = lr.Pitch;
  Loading = mipmap;
}

void sTexture2D::BeginLoadPartial(const sRect &rect,sU8 *&data,sInt &pitch,sInt mipmap)
{
  D3DLOCKED_RECT lr;
  sVERIFY(Loading==-1);

  DXErr(Tex2D->LockRect(mipmap,&lr,(const RECT *)&rect,LockFlags&~D3DLOCK_DISCARD));  // you probably don't want to discard the entire texture/mipmap chain with a partial update
  data = (sU8 *)lr.pBits;
  pitch = lr.Pitch;
  Loading = mipmap;
}

void sTexture2D::EndLoad()
{
  sVERIFY(Loading>=0);
  DXErr(Tex2D->UnlockRect(Loading));
  Loading = -1;
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
  sVERIFY(rect.x0 <= rect.x1 && rect.y0 <= rect.y1);
  sVERIFY((Flags & sTEX_RENDERTARGET) && Mipmaps > 1);

  IDirect3DSurface9 *srcSurf,*dstSurf;
  RECT dstRect;

  dstRect.left = rect.x0/2;
  dstRect.top = rect.y0/2;
  dstRect.right = rect.x1/2;
  dstRect.bottom = rect.y1/2;

  DXErr(Tex2D->GetSurfaceLevel(0,&srcSurf));
  DXErr(Tex2D->GetSurfaceLevel(1,&dstSurf));
  DXErr(DXDev->StretchRect(srcSurf,(const RECT*) &rect,dstSurf,&dstRect,D3DTEXF_LINEAR));
  srcSurf->Release();
  dstSurf->Release();
}

/****************************************************************************/
/***                                                                      ***/
/***   sTextureCube                                                       ***/
/***                                                                      ***/
/****************************************************************************/

void sTextureCube::Create2(sInt flags)
{
  // handle stream textures as dynamic textures
  if(Flags&sTEX_STREAM)
  {
    Flags &= ~sTEX_STREAM;
    Flags |= sTEX_DYNAMIC;
    flags &= ~sTEX_STREAM;
    flags |= sTEX_DYNAMIC;
  }

  D3DFORMAT format;
  D3DPOOL pool;
  sInt usage;

  // create resource
  ConvertFlags(flags, format, pool, usage, Mipmaps, LockFlags);
  DXErr(DXDev->CreateCubeTexture(SizeXY,Mipmaps,usage,format,pool,&TexCube,0));

  // register for lost device handling
  if ((Flags&sTEX_RENDERTARGET) || (Flags&sTEX_DYNAMIC))
  {
    sVERIFY(DXRTCount < MAX_RENDERTARGETS);
    DXRenderTargets[DXRTCount++] = this;
  }
}

void sTextureCube::Destroy2()
{
  if ((Flags&sTEX_RENDERTARGET) || (Flags&sTEX_DYNAMIC))
  {
    for (sInt i=0; i<DXRTCount; i++)
      if (DXRenderTargets[i] == this)
      {
        DXRenderTargets[i] = DXRenderTargets[DXRTCount-1];
        DXRTCount--;
        break;
      }
  }
  sRelease(TexCube);
}

void sTextureCube::OnLostDevice(sBool reinit/*sFALSE*/)
{
  if ((Flags&sTEX_RENDERTARGET) || (Flags&sTEX_DYNAMIC))
  {
    sRelease(TexCube);
    if (reinit) Init(SizeXY, Flags,Mipmaps,1);

    FrameRT = 0xffff;
    SceneRT = 0xffff;
  }
}

void sTextureCube::BeginLoad(sTexCubeFace cf, sU8*& data, sInt& pitch, sInt mipmap/*=0*/)
{
  sVERIFY(cf != sTCF_NONE);

  D3DLOCKED_RECT lr;
  sVERIFY(Loading==-1);

  sInt lflags = LockFlags;
  if(cf!=sTCF_POSX||mipmap)               // D3DLOCK_DISCARD is allowed only on the top level
    lflags &= ~D3DLOCK_DISCARD;
  DXErr(TexCube->LockRect(static_cast<D3DCUBEMAP_FACES>(cf), mipmap, &lr, 0, lflags));
  data = (sU8 *)lr.pBits;
  pitch = lr.Pitch;
  Loading = mipmap;
  LockedFace = cf;
}

void sTextureCube::EndLoad()
{
  sVERIFY(Loading>=0);
  DXErr(TexCube->UnlockRect(static_cast<D3DCUBEMAP_FACES>(LockedFace),Loading));
  Loading = -1;
}

/****************************************************************************/
/***                                                                      ***/
/***   sTextureProxy                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void sTextureProxy::Connect2()
{
  Disconnect2();

  TexBase = Link->TexBase; 
  Surf2D = Link->Surf2D;
  MultiSurf2D = Link->MultiSurf2D;

  if(TexBase) TexBase->AddRef();
  if(Surf2D) Surf2D->AddRef();
  if(MultiSurf2D) MultiSurf2D->AddRef();
}

void sTextureProxy::Disconnect2()
{
  sRelease(TexBase);
  sRelease(Surf2D);
  sRelease(MultiSurf2D);
}

/****************************************************************************/
/***                                                                      ***/
/***   sTextureRaw                                                        ***/
/***                                                                      ***/
/****************************************************************************/

void sTextureBasePrivate::OnLostDevice(sBool reinit)
{
  sFatal(L"OnLostDevice not supported by this kind of texture\n");
}

/****************************************************************************/
/***                                                                      ***/
/***   sTextureBase                                                       ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/
/***                                                                      ***/
/***   sTexture3D                                                         ***/
/***                                                                      ***/
/****************************************************************************/

sTexture3D::sTexture3D(sInt xs, sInt ys, sInt zs, sU32 flags)
{
  D3DFORMAT format;
  D3DPOOL pool;
  sInt usage;

  SizeX = xs;
  SizeY = ys;
  SizeZ = zs;
  Flags = (flags&~sTEX_TYPE_MASK)|sTEX_3D;
  Mipmaps = 0;
  Tex3D = 0;
  Loading = -1;

  while(xs>=1 && ys>=1 && zs>=1)
  {
    xs = xs/2;
    ys = ys/2;
    zs = zs/2;
    Mipmaps++;
  }
  if(flags & sTEX_NOMIPMAPS)
    Mipmaps = 1;

  BitsPerPixel = sGetBitsPerPixel(flags);
  ConvertFlags(flags, format, pool, usage, Mipmaps, LockFlags);
  DXErr(DXDev->CreateVolumeTexture(SizeX, SizeY, SizeZ, Mipmaps,usage,format,pool,&Tex3D,0));

  // register rendertargets
  if((Flags&sTEX_RENDERTARGET) || (Flags&sTEX_DYNAMIC))
  {
    sVERIFY(DXRTCount < MAX_RENDERTARGETS);
    DXRenderTargets[DXRTCount++] = this;
  }
}

sTexture3D::~sTexture3D()
{
  sRelease(Tex3D);
}

void sTexture3D::BeginLoad(sU8*& data, sInt& rpitch, sInt& spitch, sInt mipmap/*=0*/)
{
  D3DLOCKED_BOX lr;
  sVERIFY(Loading==-1);

  sInt lflags = LockFlags;
  if(mipmap)               // D3DLOCK_DISCARD is allowed only on the top level
    lflags &= ~D3DLOCK_DISCARD;
  DXErr(Tex3D->LockBox(mipmap, &lr, 0, lflags));
  data = (sU8 *)lr.pBits;
  rpitch = lr.RowPitch;
  spitch = lr.SlicePitch;
  Loading = mipmap;
}

void sTexture3D::EndLoad()
{
  sVERIFY(Loading>=0);
  DXErr(Tex3D->UnlockBox(Loading));
  Loading = -1;
}

void sTexture3D::Load(sU8 *data)
{
  sVERIFYFALSE;
}

/****************************************************************************/
/***                                                                      ***/
/***   sMaterial                                                          ***/
/***                                                                      ***/
/****************************************************************************/

void sMaterial::Create2()
{
  sVERIFYSTATIC(sOFFSET(sMaterial,Flags)+sizeof(sMaterialRS)==sOFFSET(sMaterial,TBind));
}

void sMaterial::Destroy2()
{
}


void sMaterial::Prepare(sVertexFormatHandle *format)
{
  sShader *oldvs = VertexShader;   VertexShader = 0;
  sShader *oldps = PixelShader;    PixelShader = 0;

  SelectShaders(format);    // before mtrl states to copy textures into Texture[]
                            // otherwise for null textures no states are set

  if(StateVariants==0)
  {
    InitVariants(1);
    SetVariant(0);
  }

  oldvs->Release();
  oldps->Release();

  for(sInt i=0;i<sMTRL_MAXTEX;i++)
  {
    sInt sampler = TBind[i] & sMTB_SAMPLERMASK;
    sInt shader  = TBind[i] & sMTB_SHADERMASK;
    if(shader==sMTB_VS)
    {
      sVERIFY(sampler<sMTRL_MAXVSTEX);
    }
    else if(shader==sMTB_PS)
    {
      sVERIFY(sampler<sMTRL_MAXPSTEX);
    }
    else
    {
      sVERIFYFALSE;
    }
  }
  PreparedFormat = format;
}


void sMaterial::Set(sCBufferBase **cbuffers,sInt cbcount,sInt variant)
{
  CurrentMtrlVFormat = PreparedFormat;
  SetStates(variant);

  // set VS

  if (VertexShader != CurrentVS)
  {
    CurrentVS = VertexShader;
    if(VertexShader==0)
    {
      DXErr(DXDev->SetVertexShader(0));
    }
    else
    {
      sVERIFY(VertexShader->vs);
      sVERIFY(VertexShader->CheckKind(sSTF_VERTEX));
      DXErr(DXDev->SetVertexShader(VertexShader->vs));
    }
#if STATS
    Stats.VSChanges++;
#endif
  }

  // set PS

  if (PixelShader != CurrentPS)
  {
    CurrentPS = PixelShader;
    if(PixelShader==0)
    {
      DXErr(DXDev->SetPixelShader(0));
    }
    else
    {
      sVERIFY(PixelShader->ps);
      sVERIFY(PixelShader->CheckKind(sSTF_PIXEL));
      DXErr(DXDev->SetPixelShader(PixelShader->ps));
    }
#if STATS
    Stats.PSChanges++;
#endif 
  }

  // set constant buffers

  sSetCBuffers(cbuffers,cbcount);
}

void sMaterial::SetStates(sInt variant)
{
  sVERIFY(variant>=0 && variant<StateVariants);
  sVERIFY(States);
  sVERIFY(States[variant]);
  
  sSetRenderStates(States[variant],StateCount[variant]);

  sTextureBase *tps[sMTRL_MAXPSTEX];
  sTextureBase *tvs[sMTRL_MAXVSTEX];
  sClear(tps);
  sClear(tvs);
  for(sInt i=0;i<sMTRL_MAXTEX;i++)
  {
    if(Texture[i])
    {
      if(TBind[i]&sMTB_SHADERMASK)
        tvs[TBind[i]&sMTB_SAMPLERMASK] = Texture[i];
      else
        tps[TBind[i]&sMTB_SAMPLERMASK] = Texture[i];
    }
  }
  for(sInt i=0;i<sMTRL_MAXPSTEX;i++)
    sSetTexture(i|sMTB_PS, tps[i]);
  for(sInt i=0;i<sMTRL_MAXVSTEX;i++)
    sSetTexture(i|sMTB_VS, tvs[i]);

#if STATS
  Stats.MtrlChanges ++;
#endif
//  sGFXMtrlIsSet = 1;

  sVERIFY(sizeof(sMaterial)-((sU8*)&Flags-(sU8*)this)>=sizeof(sMaterialRS));
  *(sMaterialRS*)&Flags = VariantFlags[variant];
}
/*
void sMaterial::AllocStates(const sU32 *data,sInt count)
{
  sDeleteArray(States);
  StateCount = count;
  count += STATES_2PASS;
  States = new sU32[count*2];
  sCopyMem(States,data,count*8);


  // this apppends additive blending states
  // material can be used with normal blending and additive blending skipping StateOffset states
  // and including last 5 states
  sU32 blend =  BlendColor==sMB_ALPHA ? (sMBS_SA  |sMBO_ADD |sMBD_1  ) : sMB_ADD;
  sU32 *ptr = &States[StateCount*2];

  *ptr++ = D3DRS_ALPHABLENDENABLE;*ptr++ = 1;
  *ptr++ = D3DRS_SRCBLEND;        *ptr++ = (blend>> 0)&15;
  *ptr++ = D3DRS_DESTBLEND;       *ptr++ = (blend>>16)&15;
  *ptr++ = D3DRS_BLENDOP;         *ptr++ = (blend>> 8)&7;
  *ptr++ = D3DRS_SEPARATEALPHABLENDENABLE; *ptr++ = 0;

  *ptr++ = D3DRS_ZENABLE;         *ptr++ = D3DZB_TRUE;
  *ptr++ = D3DRS_ZWRITEENABLE;    *ptr++ = 0;
  *ptr++ = D3DRS_ZFUNC;           *ptr++ = (FuncFlags[0]==sMFF_ALWAYS || (Flags & sMTRL_ZMASK)==sMTRL_ZOFF ? sMFF_ALWAYS : sMFF_EQUAL)+D3DCMP_NEVER;
}
*/
void sMaterial::AddMtrlFlags(sU32 *&data)
{
  //sU32* start = data;
  if(BlendColor!=sMB_OFF)
  {
    *data++ = D3DRS_ALPHABLENDENABLE; *data++ = 1;

    *data++ = D3DRS_SRCBLEND;         *data++ = (BlendColor>> 0)&15;
    *data++ = D3DRS_DESTBLEND;        *data++ = (BlendColor>>16)&15;
    *data++ = D3DRS_BLENDOP;          *data++ = (BlendColor>> 8)&7;
    *data++ = D3DRS_BLENDFACTOR;      *data++ = BlendFactor;

    if(BlendAlpha==sMB_SAMEASCOLOR)
    {
      *data++ = D3DRS_SEPARATEALPHABLENDENABLE; *data++ = 0;
    }
    else
    {
      *data++ = D3DRS_SEPARATEALPHABLENDENABLE; *data++ = 1;

      *data++ = D3DRS_SRCBLENDALPHA;    *data++ = (BlendAlpha>> 0)&15;
      *data++ = D3DRS_DESTBLENDALPHA;   *data++ = (BlendAlpha>>16)&15;
      *data++ = D3DRS_BLENDOPALPHA;     *data++ = (BlendAlpha>> 8)&7;
    }
  }
  else
  {
    *data++ = D3DRS_ALPHABLENDENABLE; *data++ = 0;
    *data++ = D3DRS_SEPARATEALPHABLENDENABLE; *data++ = 0;
  }

  switch(Flags & sMTRL_ZMASK)
  {
  case sMTRL_ZOFF:
    *data++ = D3DRS_ZENABLE;        *data++ = D3DZB_FALSE;
    break;
  case sMTRL_ZREAD:
    *data++ = D3DRS_ZENABLE;        *data++ = D3DZB_TRUE;
    *data++ = D3DRS_ZWRITEENABLE;   *data++ = 0;
    *data++ = D3DRS_ZFUNC;          *data++ = FuncFlags[0]+D3DCMP_NEVER;
    break;
  case sMTRL_ZWRITE:
    *data++ = D3DRS_ZENABLE;        *data++ = D3DZB_TRUE;
    *data++ = D3DRS_ZWRITEENABLE;   *data++ = 1;
    *data++ = D3DRS_ZFUNC;          *data++ = D3DCMP_ALWAYS;
    break;
  case sMTRL_ZON:
    *data++ = D3DRS_ZENABLE;        *data++ = D3DZB_TRUE;
    *data++ = D3DRS_ZWRITEENABLE;   *data++ = 1;
    *data++ = D3DRS_ZFUNC;          *data++ = FuncFlags[0]+D3DCMP_NEVER;
    break;
  }

//  StateOffset = (data-start)/2;


  switch(Flags & sMTRL_CULLMASK)
  {
  case sMTRL_CULLOFF:
    *data++ = D3DRS_CULLMODE;       *data++ = D3DCULL_NONE;
    break;
  case sMTRL_CULLON:
    *data++ = D3DRS_CULLMODE;       *data++ = D3DCULL_CCW;
    break;
  case sMTRL_CULLINV:
    *data++ = D3DRS_CULLMODE;       *data++ = D3DCULL_CW;
    break;
  }

  // color write mask
  {
    sU32 mask = D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_BLUE;
    if (Flags&sMTRL_MSK_ALPHA)
      mask &= ~D3DCOLORWRITEENABLE_ALPHA;
    if (Flags&sMTRL_MSK_RED)
      mask &= ~D3DCOLORWRITEENABLE_RED;
    if (Flags&sMTRL_MSK_GREEN)
      mask &= ~D3DCOLORWRITEENABLE_GREEN;
    if (Flags&sMTRL_MSK_BLUE)
      mask &= ~D3DCOLORWRITEENABLE_BLUE;

    *data++ = D3DRS_COLORWRITEENABLE;
    *data++ = mask;
  }

  *data++ = D3DRS_MULTISAMPLEANTIALIAS;
  *data++ = (Flags&sMTRL_SINGLESAMPLE)?0:1;

  // stencil renderstates

  if (Flags & (sMTRL_STENCILSS | sMTRL_STENCILDS))
  {
    *data++ = D3DRS_STENCILENABLE; *data++ = 1; 
    *data++ = D3DRS_STENCILMASK; *data++ = StencilMask;
    *data++ = D3DRS_STENCILWRITEMASK; *data++ = 0xffffffff;
    *data++ = D3DRS_STENCILREF; *data++ = StencilRef;
    *data++ = D3DRS_STENCILFUNC; *data++ = FuncFlags[sMFT_STENCIL]+D3DCMP_NEVER;
    *data++ = D3DRS_STENCILFAIL; *data++ = StencilOps[sMSI_CWFAIL]+D3DSTENCILOP_KEEP;
    *data++ = D3DRS_STENCILZFAIL; *data++ = StencilOps[sMSI_CWZFAIL]+D3DSTENCILOP_KEEP;
    *data++ = D3DRS_STENCILPASS; *data++ = StencilOps[sMSI_CWPASS]+D3DSTENCILOP_KEEP;
    if ( Flags & sMTRL_STENCILDS ) 
    {
      *data++ = D3DRS_TWOSIDEDSTENCILMODE; *data++ = 1;
      *data++ = D3DRS_CCW_STENCILFAIL; *data++ = StencilOps[sMSI_CCWFAIL]+D3DSTENCILOP_KEEP;
      *data++ = D3DRS_CCW_STENCILZFAIL; *data++ = StencilOps[sMSI_CCWZFAIL]+D3DSTENCILOP_KEEP;
      *data++ = D3DRS_CCW_STENCILPASS; *data++ = StencilOps[sMSI_CCWPASS]+D3DSTENCILOP_KEEP;
    }
    else
    {
      *data++ = D3DRS_TWOSIDEDSTENCILMODE; *data++ = 0;
    }
  }
  else
  {
    *data++ = D3DRS_STENCILENABLE; *data++ = 0; 
    *data++ = D3DRS_STENCILWRITEMASK; *data++ = 0;
  }

  for(sInt i=0;i<sMTRL_MAXTEX;i++)
  {
    if(Texture[i] || (TFlags[i]&sMTF_EXTERN))
    {
      sInt stage = TBind[i]&sMTB_SAMPLERMASK;
      if(TBind[i]&sMTB_SHADERMASK)
        stage+=16;
      data += sRenderStateTexture(data,stage,TFlags[i],LodBias[i]);
    }
  }

  if(FuncFlags[sMFT_ALPHA]!=sMFF_ALWAYS)
  { 
    *data++ = D3DRS_ALPHATESTENABLE; *data++ = 1;
    *data++ = D3DRS_ALPHAFUNC; *data++ = FuncFlags[sMFT_ALPHA]+1;
    *data++ = D3DRS_ALPHAREF; *data++ = AlphaRef; 
  }
  else
  {
    *data++ = D3DRS_ALPHATESTENABLE; *data++ = 0;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   MainLoops                                                          ***/
/***                                                                      ***/
/****************************************************************************/

void sSetRenderClipping(sRect *r,sInt count)
{
  RGNDATAHEADER *hdr;
  sRect *rd;

  sVERIFY(count*sizeof(sRect)+sizeof(RGNDATAHEADER)<=sizeof(RenderClippingData));

  hdr = (RGNDATAHEADER *) RenderClippingData;
  rd = (sRect *) (((sU8 *)RenderClippingData)+(sizeof(RGNDATAHEADER)));

  hdr->dwSize = sizeof(RGNDATAHEADER);
  hdr->iType = RDH_RECTANGLES;
  hdr->nCount = count;
  hdr->nRgnSize = hdr->nCount*sizeof(sRect);
  hdr->rcBound.left = 0;
  hdr->rcBound.top = 0;
  hdr->rcBound.right = DXScreenMode.ScreenX;
  hdr->rcBound.bottom = DXScreenMode.ScreenY;

  sCopyMem(rd,r,count*sizeof(sRect));

  RenderClippingFlag = 1;
}

sBool sRender3DBegin()
{
  // restore system

  sVERIFY(DXActiveRT==0);
  sVERIFY(Render3DInProgress==sFALSE);

  if(!DXActive)
  {
    sLogF(L"gfx",L"sRender3DBegin() fail: not active\n");
    return 0;
  }

  if(DXRestore)
  {
    if(DXMayRestore)
    {
      InitGFX(0,0,0);
      if (DXRestore)
      {
        sSleep(10);
        return 0;
      }
    }
    else
    {
      sLogF(L"gfx",L"sRender3DBegin() fail: can not restore\n");
      return 0;
    }
  }

  if(RenderClippingFlag && ((RGNDATAHEADER *) RenderClippingData)->nCount==0)
  {
    sLogF(L"gfx",L"sRender3DBegin() fail: zero area\n");
    return 0;
  }

  Render3DInProgress = sTRUE;

  if(SUCCEEDED(DXDev->BeginScene()))
  {
    // anti-lagging

    static sInt dblock;
    D3DLOCKED_RECT lr;
    volatile sInt dummy;

    DXDev->ColorFill(DXBlockSurface[dblock],0,0xff002050);

    dblock = 1-dblock;

    if(!FAILED((DXBlockSurface[dblock]->LockRect(&lr,0,D3DLOCK_READONLY))))
    {
      dummy = *(volatile sInt *)lr.pBits;
      DXBlockSurface[dblock]->UnlockRect();
    }

    // prepare frame

    sGFXCurrentFrame++;
//    sGFXMtrlIsSet = 0;
    for(sInt i=0;i<sCOUNTOF(sDXStates);i++)
      sDXStates[i] = 0xdeadc0de;

    // default states

    D3DMATERIAL9 d3dm;
    sClear(d3dm);
    d3dm.Ambient .a=1; d3dm.Ambient .r=1; d3dm.Ambient .g=1; d3dm.Ambient .b=1;
    d3dm.Diffuse .a=1; d3dm.Diffuse .r=1; d3dm.Diffuse .g=1; d3dm.Diffuse .b=1;
    d3dm.Emissive.a=0; d3dm.Emissive.r=0; d3dm.Emissive.g=0; d3dm.Emissive.b=0;
    d3dm.Specular.a=1; d3dm.Specular.r=1; d3dm.Specular.g=1; d3dm.Specular.b=1;
    d3dm.Power = 1.0f;

    DXErr(DXDev->SetMaterial(&d3dm));
    DXErr(DXDev->SetRenderState(D3DRS_SCISSORTESTENABLE,1));

    DXErr(DXDev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE ,D3DMCS_COLOR1 ));
    DXErr(DXDev->SetRenderState(D3DRS_SPECULARMATERIALSOURCE,D3DMCS_MATERIAL ));
    DXErr(DXDev->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE ,D3DMCS_MATERIAL ));
    DXErr(DXDev->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE,D3DMCS_MATERIAL ));
//    for(sInt i=0;i<16;i++)
//      DXErr(DXDev->SetSamplerState(i,D3DSAMP_MAXANISOTROPY,8));
    // dxstates are reset to 0xffffffff, so set color 0xffffffff
    for(sInt i=0;i<16;i++)
    {
      DXErr(DXDev->SetSamplerState(i,D3DSAMP_BORDERCOLOR ,0xffffffff ));
    }


    if(DXShaderProfile>=sSTF_DX_20)
      DXErr(DXDev->SetVertexShaderConstantB(0,CurrentVSBools,sCOUNTOF(CurrentVSBools)));
    if(DXShaderProfile>=sSTF_DX_30)
      DXErr(DXDev->SetPixelShaderConstantB (0,CurrentPSBools,sCOUNTOF(CurrentPSBools)));

    // clear caching

    DXActiveRT = 0;
    DXActiveZB = 0;
    DXActiveRTFlags = 0;
    DXActiveMultiRT = 1;
    CurrentVS = 0;
    CurrentPS = 0;
    CurrentMtrlVFormat = 0;
    sClear(CurrentVSBools);
    sClear(CurrentPSBools);
    sClearCurrentCBuffers();
    for(sInt i=0;i<sCOUNTOF(CurrentTexture);i++)
      CurrentTexture[i] = (sTextureBase *) 1;     // make sure they are flushed!
    for(sInt i=0;i<sMTRL_MAXPSTEX;i++)
      sSetTexture(i|sMTB_PS, 0);
    for(sInt i=0;i<sMTRL_MAXVSTEX;i++)
      sSetTexture(i|sMTB_VS, 0);

    // set main view cfg, otherwise sViewport::SetTargetCurrent/SetTarget(0) don't
    // work as expected, when rendered to another render target at end of last frame
    sGFXRendertargetX = DXScreenMode.ScreenX;
    sGFXRendertargetY = DXScreenMode.ScreenY;
    sGFXRendertargetAspect = DXScreenMode.Aspect;
    sGFXViewRect.Init(sGFXRendertargetX,sGFXRendertargetY);
  }

  sGeoBufferUnlockAll();

  return 1;
}

void sRender3DEnd(sBool flip)
{
  if(!Render3DInProgress)
  {
    sVERIFY(DXActiveRT==0);
    return;
  }

  // pre flip hook

  if(flip)
    sPreFlipHook->Call();

  // finish up rendertarget

  for(sInt i=0;i<DXScreenCount;i++)
    sResolveTargetPrivate(sGetScreenColorBuffer(i));

  DXErr(DXDev->SetRenderTarget(0,DXBackBuffer[0]->Surf2D));
  for(sInt i=1;i<sMin<sInt>(4,DXCaps.NumSimultaneousRTs);i++)
    DXErr(DXDev->SetRenderTarget(i,0));
  DXErr(DXDev->SetDepthStencilSurface(0));
  sRelease(DXTargetSurf);
  sRelease(DXTargetZSurf);
  DXActiveRT = 0;
  DXActiveZB = 0;
  DXActiveRTFlags = 0;
  DXActiveMultiRT = 0;

  // bla bla

  Render3DInProgress = sFALSE;

  DXErr(DXDev->EndScene());

  sGeoBufferFrame();

  // perform flip

  if(flip)
  {
    HRESULT hr = DXDev->Present(0,0,0,RenderClippingFlag ? (RGNDATA *) RenderClippingData : 0);
    if(hr==D3DERR_DEVICELOST)
      DXRestore = 1;
    else if(hr==0x88760879)
      sLogF(L"gfx",L"DX Error 0x88760879: leaving this error unhandled!\n");
#if sCONFIG_OPTION_NPP
    // ignore INVALIDCALL, chances are it will work one frame later :)
    else if(hr==0x88760872)
      sLogF(L"gfx",L"DX Error 0x88760872: leaving this error unhandled!\n");
#endif
    else
      DXErr(hr);
    RenderClippingFlag=0;
  }

  // more bla bla

  BufferedStats = Stats;
  StatsEnable = 1;
  BufferedStats.StaticTextureMem = DXTotalTextureMem;
  sDInt mem = 0;
  for(sInt i=0;i<sGeoBufferCount;i++)
  {
    if(sGeoBuffers[i].Duration==sGD_STATIC)
      mem += sGeoBuffers[i].Used;
    if(sGeoBuffers[i].Used>0)
      BufferedStats.GeoBuffersActive++;
  }
  BufferedStats.StaticVertexMem = mem;


  sClear(Stats);

  sFlipMem();

  // post flip hook

  if(flip)
    sPostFlipHook->Call();

  sVERIFY(DXActiveRT==0);
}

void sRender3DFlush()
{
}

void sSetDesiredFrameRate(sF32 rate)
{
}

void sSetScreenGamma(sF32 gamma)
{
  D3DGAMMARAMP ramp;
  for(sInt i=0;i<256;i++)
  {
    ramp.red[i] = 0xffff*sFPow(i/255.0f,gamma);
    ramp.green[i] = 0xffff*sFPow(i/255.0f,gamma);
    ramp.blue[i] = 0xffff*sFPow(i/255.0f,gamma);
  }
  DXDev->SetGammaRamp(0,D3DSGR_CALIBRATE,&ramp);
}


/****************************************************************************/

#endif // sRENDERER == sRENDER_DX9


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
#if sRENDERER == sRENDER_DX11

#undef new
#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10math.h>
#include <d3dcompiler.h>
#define new sDEFINE_NEW

#include "base/system.hpp"
#include "base/math.hpp"
#include "base/graphics.hpp"
#include "base/windows.hpp"

#include "base/serialize.hpp"
#include "util/image.hpp"
#include "util/algorithms.hpp"

void sInitGfxCommon();
void sExitGfxCommon();
extern sInt sSystemFlags;

#define sDXRELEASE(x) { if(x) DXErr(x->Release()); x=0; }

extern HINSTANCE WInstance;

/****************************************************************************/
/***                                                                      ***/
/***   static                                                             ***/
/***                                                                      ***/
/****************************************************************************/

#define MAXSCREENS 16
static IDXGIFactory1 *DXGI;
static IDXGIAdapter1 *DXAdapter;
static D3D_FEATURE_LEVEL DXlevel_out;
static IDXGISwapChain *DXSwapChain[MAXSCREENS];
static sInt DXScreenSizeX[MAXSCREENS];
static sInt DXScreenSizeY[MAXSCREENS];
static HWND DXDummyWindow[MAXSCREENS];
static IDXGIOutput *DXSyncOutput;
static sU32 DXLastFrame;
static sInt DXFrameTime;
static sU64 DXFrameTimeU64;
static sU64 PerfCountFreq;
static sU64 PerfCountStart;
static IDXGISurface1 *DXGISurface;
static ID3D11Device *DXDev;
static sInt DXRendering;
static sScreenMode DXScreenMode;
static sInt DXRestore=0;
extern sInt DXMayRestore = 1;
static sInt DXActive=0;
static ID3D11Buffer *DXQuadList;

static sTexture2D *DXBackBufferTex[MAXSCREENS];
static sTexture2D *DXZBufferTex[MAXSCREENS];
static sTexture2D *DXRTZBufferTex;
static sBool DXRenderTargetActive;
static sInt DXPresentInterval;
static sInt DXScreenCount;

static sGeoBufferManager *GeoBufferManager;
static sCBufferManager *CBufferManager;

static sArray<DXGI_SAMPLE_DESC> *DXMSAALevels;

static sGraphicsCaps GraphicsCapsMaster;

static sDList2<sOccQueryNode> *FreeOccQueryNodes;
static sArray<sOccQueryNode *> *AllOccQueryNodes;

ID3D11DeviceContext *DXBaseCtx;

__declspec(thread) sGfxThreadContext *GTC = 0;
sGfxThreadContext *GTCBase = 0;

/****************************************************************************/

extern HWND sHWND;

typedef HRESULT (WINAPI *DXCompLibCompileType)(
  LPCVOID pSrcData,
  SIZE_T SrcDataSize,
  LPCSTR pSourceName,
  CONST D3D10_SHADER_MACRO *pDefines,
  LPD3D10INCLUDE pInclude,
  LPCSTR pEntrypoint,
  LPCSTR pTarget,
  UINT Flags1,
  UINT Flags2,
  LPD3D10BLOB *ppCode,
  LPD3D10BLOB *ppErrorMsgs
);
typedef HRESULT (WINAPI *DXCompLibDisassembleType)(
  LPCVOID pSrcData,
  SIZE_T SrcDataSize,
  UINT Flags,
  LPD3D10BLOB *szComments,
  LPD3D10BLOB *ppDisassembly
);

static HMODULE DXCompLib;
static DXCompLibCompileType DXCompLibCompile;
static DXCompLibDisassembleType DXCompLibDisassemble;

struct sScreenInfoX : public sScreenInfo
{
  sInt Adapter;
  sInt MonitorOfAdapter;
  sInt PosX,PosY;
};

static sScreenInfoX *DXScreenInfos[MAXSCREENS];

/****************************************************************************/
/***                                                                      ***/
/***   Windows Helper                                                     ***/
/***                                                                      ***/
/****************************************************************************/

void PrintBlob(ID3D10Blob *b)
{
  if(b)
  {
    sInt len = b->GetBufferSize();
    sChar *buf = new sChar[len+1];
    sCopyString(buf,(sChar8 *)b->GetBufferPointer(),len);
    buf[len] = 0;
    sDPrint(buf);
    delete[] buf;
  }
}

#if sSTRIPPED
void DXError(sU32 err);

#define DXErr(hr) { sU32 err=hr; if(FAILED(err)) DXError(err); }
#else
void DXError(sU32 err,const sChar *file,sInt line,const sChar *system);

#define DXErr(hr) { sU32 err=hr; if(FAILED(err)) DXError(err,sTXT(__FILE__),__LINE__,L"d3d"); }
#endif


#define STATS 1



static sGraphicsStats Stats;
static sGraphicsStats BufferedStats;
static sGraphicsStats DisabledStats;
static sBool StatsEnable;

static sInt RenderClippingFlag;
static sU32 RenderClippingData[4096];

/****************************************************************************/
/***                                                                      ***/
/***   Initialisation                                                     ***/
/***                                                                      ***/
/****************************************************************************/

void ConvertFlags(sU32 flags,sInt &mm,D3D11_BIND_FLAG &bind,D3D11_USAGE &usage,DXGI_FORMAT &fmt_res,DXGI_FORMAT &fmt_tex,DXGI_FORMAT &fmt_view,sBool &ds);

LRESULT WINAPI MsgProcMultiscreen(HWND win,UINT msg,WPARAM wparam,LPARAM lparam)
{
  switch(msg)
  {
  case WM_CLOSE:
    {
      sBool mayexit = 1;
      sAltF4Hook->Call(mayexit);
      if(mayexit)
        sExit(); 
    }
    break;
  case WM_SETCURSOR:
    if(sSystemFlags & sISF_FULLSCREEN)
      SetCursor(0);
    else
      return DefWindowProc(win,msg,wparam,lparam);
    break;
  default:
    return DefWindowProc(win,msg,wparam,lparam);
  }
  return 1;
}

void PreInitGFX(sInt &flags,sInt &xs,sInt &ys)
{
  // set screenmode

  if(!(DXScreenMode.Flags & sSM_VALID))
  {
    DXScreenMode.Clear();
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

void InitGFX(sInt flags_,sInt xs_,sInt ys_)
{

  // if we are recreating, release resources

  if(DXDev)
  {
    for(sInt i=0;i<DXScreenCount;i++)
    {
      sDelete(DXBackBufferTex[i]);
      sDelete(DXZBufferTex[i]);
    }
    sDelete(DXRTZBufferTex);
    sDXRELEASE(DXGISurface);
    sDXRELEASE(DXSyncOutput);
    DXBaseCtx->ClearState();
    for(sInt i=0;i<DXScreenCount;i++)
      DXErr(DXSwapChain[i]->ResizeBuffers(1,DXScreenMode.ScreenX,DXScreenMode.ScreenY,DXGI_FORMAT_B8G8R8A8_UNORM,DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE));
  }

  // create device

  sScreenInfo si;
  sGetScreenInfo(si,0,0);

  if(DXGI==0)
  {
    DXErr(CreateDXGIFactory1(__uuidof(IDXGIFactory1),(void**)(&DXGI)));
    DXErr(DXGI->MakeWindowAssociation(sHWND,0));
  }

  sVERIFY(DXScreenMode.Display < MAXSCREENS);
  sScreenInfoX *display = DXScreenInfos[sMax(0,DXScreenMode.Display)];

  if(DXDev==0)
  {
    DXGI_SWAP_CHAIN_DESC sd;
    const D3D_FEATURE_LEVEL levels[4] = { D3D_FEATURE_LEVEL_11_0,D3D_FEATURE_LEVEL_10_1,D3D_FEATURE_LEVEL_10_0,D3D_FEATURE_LEVEL_9_3 };
    sClear(sd);

    sd.BufferCount = 1;
    sd.BufferDesc.Width = DXScreenMode.ScreenX;
    sd.BufferDesc.Height = DXScreenMode.ScreenY;
    sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 0;
    sd.BufferDesc.RefreshRate.Denominator = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT|DXGI_USAGE_SHADER_INPUT;
    sd.OutputWindow = sHWND;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = !(DXScreenMode.Flags & sSM_FULLSCREEN);
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH|DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;


    D3D_DRIVER_TYPE dt = D3D_DRIVER_TYPE_HARDWARE;
    if(DXScreenMode.Flags & sSM_REFRAST) dt = D3D_DRIVER_TYPE_REFERENCE;
    if(DXScreenMode.Flags & sSM_WARP) dt = D3D_DRIVER_TYPE_WARP;


    if(DXScreenMode.Flags & sSM_MULTISCREEN)
    {
      WNDCLASSEXW wc;
      sClear(wc);
      wc.cbSize = sizeof(WNDCLASSEX);
      wc.style = CS_OWNDC|CS_DBLCLKS ;
      wc.lpfnWndProc = MsgProcMultiscreen;
      wc.hInstance = WInstance;
      wc.lpszClassName = L"ALTONA_MULTISCREEN";
      wc.hCursor = 0;
      wc.hIcon = LoadIcon(WInstance,MAKEINTRESOURCE(100));
      RegisterClassExW(&wc);

      DXErr(DXGI->EnumAdapters1(display->Adapter,&DXAdapter));
      DXErr(D3D11CreateDevice(DXAdapter,D3D_DRIVER_TYPE_UNKNOWN,0,0,levels,3,D3D11_SDK_VERSION,&DXDev,&DXlevel_out,&DXBaseCtx));

      GTCBase = GTC = new sGfxThreadContext;
      GTCBase->DXCtx = DXBaseCtx;

      DXScreenCount = 0;
      for(;;)
      {
        IDXGIOutput *o;
        HRESULT hr = DXAdapter->EnumOutputs(DXScreenCount,&o);
        if(hr!=S_OK) break;

        DXGI_OUTPUT_DESC od;
        DXErr(o->GetDesc(&od));
        sRect r(*(sRect *)&od.DesktopCoordinates);
        sDPrintF(L"%d\n",r);

        HWND win = sHWND;

        if(DXScreenCount>0)
        {
          win = CreateWindowW(L"ALTONA_MULTISCREEN",L"Altona Multiscreen",WS_OVERLAPPEDWINDOW,
            r.x0,r.y0,r.SizeX(),r.SizeY(),
            sHWND,0,WInstance,0);
          DXDummyWindow[DXScreenCount] = win;
          SetWindowPos(win,0,r.x0,r.y0,r.SizeX(),r.SizeY(),SW_SHOW);
          ShowWindow(win,SW_SHOW);
          UpdateWindow(win);
        }
        else
        {
          SetWindowPos(win,0,r.x0,r.y0,r.SizeX(),r.SizeY(),0);
        }
  
        sDXRELEASE(o);

        sd.OutputWindow = win;
        DXScreenSizeX[DXScreenCount] = sd.BufferDesc.Width = r.SizeX();
        DXScreenSizeY[DXScreenCount] = sd.BufferDesc.Height = r.SizeY();
        
        DXErr(DXGI->CreateSwapChain(DXDev,&sd,&DXSwapChain[DXScreenCount++]));
      }
      if(DXScreenCount>0)
      {
        DXScreenMode.ScreenX = DXScreenSizeX[0];
        DXScreenMode.ScreenY = DXScreenSizeY[0];
      }
      SetFocus(sHWND);
    }

    else
    {
      // D3D11CreateDeviceAndSwapChain() is a much safer method of creating a fullscreen swapchain,
      // my implementation avove may handle the resizing incorrectly.

      DXErr(DXGI->EnumAdapters1(display->Adapter,&DXAdapter));
      DXErr(D3D11CreateDeviceAndSwapChain(DXAdapter,D3D_DRIVER_TYPE_UNKNOWN,0,0,
            levels,3,D3D11_SDK_VERSION,&sd,
            &DXSwapChain[0],&DXDev,&DXlevel_out,&DXBaseCtx));
      DXScreenCount = 1;
      DXScreenSizeX[0] = DXScreenMode.ScreenX;
      DXScreenSizeY[0] = DXScreenMode.ScreenY;

//      DXSwapChain[0]->AddRef();
      GTCBase = GTC = new sGfxThreadContext;
      GTCBase->DXCtx = DXBaseCtx;


      if(flags_ & sISF_2D)
      {
        IDXGIDevice1 *dev;
        DXErr(DXDev->QueryInterface(__uuidof(IDXGIDevice1),(void**)(&dev)));
        dev->SetMaximumFrameLatency(1);
        sDXRELEASE(dev);
      }
    }

    DXCompLib = LoadLibrary(L"D3DCompiler_42.dll");
    if(!DXCompLib)
      sFatal(L"could not load D3DCompiler_42.dll");
    DXCompLibCompile = (DXCompLibCompileType) GetProcAddress(DXCompLib,"D3DCompile");
    sVERIFY(DXCompLibCompile);
    DXCompLibDisassemble = (DXCompLibDisassembleType) GetProcAddress(DXCompLib,"D3DDisassemble");
    sVERIFY(DXCompLibDisassemble);

    GeoBufferManager = new sGeoBufferManager;
    CBufferManager = new sCBufferManager;

    DXPresentInterval = 1;
    if((DXScreenMode.Flags & sSM_NOVSYNC) || (flags_ & sISF_2D))
      DXPresentInterval = 0;

    // index buffer for quadlist drawing

    if(1)
    {
      sU16 *ql = new sU16[0x10000/4*6];
      for(sInt i=0;i<0x10000/4;i++)
      {
        ql[i*6+0] = i*4+0;
        ql[i*6+1] = i*4+1;
        ql[i*6+2] = i*4+2;
        ql[i*6+3] = i*4+0;
        ql[i*6+4] = i*4+2;
        ql[i*6+5] = i*4+3;
      }

      D3D11_BUFFER_DESC bd;
      sClear(bd);
      bd.ByteWidth = 2*0x10000/4*6;
      bd.Usage = D3D11_USAGE_IMMUTABLE;
      bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

      D3D11_SUBRESOURCE_DATA sd;
      sClear(sd);
      sd.pSysMem = ql;
      DXErr(DXDev->CreateBuffer(&bd,&sd,&DXQuadList));
      delete[] ql;
    }

    // enumerate msaa levels

    DXMSAALevels = new sArray<DXGI_SAMPLE_DESC>;

    for(sInt i=1;i<=D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT;i++)
    {
      UINT n;
      DXErr(DXDev->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM,i,&n));
      for(sU32 j=0;j<n;j++)
      {
        if(i>1)
        {
          DXGI_SAMPLE_DESC *desc = DXMSAALevels->AddMany(1);
          desc->Count = i;
          desc->Quality = j;
        }
      }
    }

    // occlusion queries

    FreeOccQueryNodes = new sDList2<sOccQueryNode>;
    AllOccQueryNodes = new sArray<sOccQueryNode *>;

    // common init

    sInitGfxCommon();
  }

  else    // just updating existing device
  {
    sVERIFY(DXScreenCount==1);
    sInt xs = DXScreenMode.ScreenX;
    sInt ys = DXScreenMode.ScreenY;

    sDXRELEASE(DXAdapter);
    DXErr(DXGI->EnumAdapters1(display->Adapter,&DXAdapter));

    if(DXScreenMode.Flags & sSM_FULLSCREEN)
    {
      IDXGIOutput *out;
      DXErr(DXAdapter->EnumOutputs(display->MonitorOfAdapter,&out));
      DXErr(DXSwapChain[0]->SetFullscreenState(1,out));
      sDXRELEASE(out);
    }
    else
    {
      DXErr(DXSwapChain[0]->SetFullscreenState(0,0));
      RECT r;
      GetClientRect(sHWND,&r);
      xs = r.right-r.left;
      ys = r.bottom-r.top;
    }

    DXGI_MODE_DESC md;
    sClear(md);
    md.Width = xs;
    md.Height = ys;
    md.RefreshRate.Numerator = 0;
    md.RefreshRate.Denominator = 0;
    DXErr(DXSwapChain[0]->ResizeTarget(&md));
  //    DXErr(DXSwapChain[0]->ResizeBuffers(1,xs,ys,DXGI_FORMAT_B8G8R8A8_UNORM,DXGI_SWAP_EFFECT_DISCARD));
    DXScreenSizeX[0] = DXScreenMode.ScreenX = xs;
    DXScreenSizeY[0] = DXScreenMode.ScreenY = ys;
    sDPrintF(L"new mode %08x: %d x %d\n",DXScreenMode.Flags,DXScreenMode.ScreenX,DXScreenMode.ScreenY);
  }

  // figure out caps

  sGraphicsCaps caps;
  sClear(caps);
  caps.MaxMultisampleLevel = DXMSAALevels->GetCount();
  caps.Flags |= sGCF_DEPTHTEX_DF24;
  if(DXlevel_out>=D3D_FEATURE_LEVEL_11_0)
    caps.ShaderProfile = sSTFP_DX_50|sSTF_HLSL45;
  else if(DXlevel_out>=D3D_FEATURE_LEVEL_10_0)
    caps.ShaderProfile = sSTFP_DX_40|sSTF_HLSL45;
  else
    caps.ShaderProfile = sSTFP_DX_30|sSTF_HLSL45;

  caps.UVOffset = 0.0f;
  caps.XYOffset = 0.0f;

  DXGI_ADAPTER_DESC adesc;
  sClear(adesc);
  DXAdapter->GetDesc(&adesc);
  sCopyString(caps.AdapterName,adesc.Description);

  sU64 avail = sGetAvailTextureFormats();
  for(sInt i=0;i<64;i++)
  {
    if((1ULL<<i)&avail)
    {
      sInt mm;
      sBool ds;
      D3D11_BIND_FLAG bind;
      D3D11_USAGE usage;
      DXGI_FORMAT fmt_res;
      DXGI_FORMAT fmt_tex;
      DXGI_FORMAT fmt_view;
      ConvertFlags(sTEX_2D|i,mm,bind,usage,fmt_res,fmt_tex,fmt_view,ds);

      UINT s_tex,s_res,s_view;

      s_res=0;  DXDev->CheckFormatSupport(fmt_res,&s_res);
      s_tex=0;  DXDev->CheckFormatSupport(fmt_tex,&s_tex);
      s_view=0; DXDev->CheckFormatSupport(fmt_view,&s_view);

      if((s_res&s_tex) & D3D11_FORMAT_SUPPORT_TEXTURE2D)
      {
        caps.VertexTex2D |= 1ULL<<i;
        caps.VertexTexCube |= 1ULL<<i;
        caps.Texture2D |= 1ULL<<i;
        caps.TextureCube |= 1ULL<<i;
      }
      if((s_view&s_tex) & D3D11_FORMAT_SUPPORT_RENDER_TARGET)
      {
        caps.TextureRT |= 1ULL<<i;
      }
    }
  }

  D3D11_FEATURE_DATA_THREADING ThreadCaps;
  DXErr(DXDev->CheckFeatureSupport(D3D11_FEATURE_THREADING,&ThreadCaps,sizeof(ThreadCaps)));

  sLogF(L"gfx",L"caps.AdapterName           %s\n",caps.AdapterName);
  sLogF(L"gfx",L"caps.Flags                 %08x\n",caps.Flags);
  sLogF(L"gfx",L"caps.MaxMultisampleLevel   %d\n",caps.MaxMultisampleLevel);
  sLogF(L"gfx",L"caps.ShaderProfile         %04x\n",caps.ShaderProfile);
  sLogF(L"gfx",L"caps.Texture2D             %016x\n",caps.Texture2D);
  sLogF(L"gfx",L"caps.TextureCube           %016x\n",caps.TextureCube);
  sLogF(L"gfx",L"caps.TextureRT             %016x\n",caps.TextureRT);
  sLogF(L"gfx",L"caps.VertexTex2D           %016x\n",caps.VertexTex2D);
  sLogF(L"gfx",L"caps.VertexTexCube         %016x\n",caps.VertexTexCube);
  sLogF(L"gfx",L"Multithreaded Creation     %d\n",ThreadCaps.DriverConcurrentCreates);
  sLogF(L"gfx",L"Command Lists              %d\n",ThreadCaps.DriverCommandLists);

  GraphicsCapsMaster = caps;

  // aquire resources

  DXErr(DXSwapChain[0]->GetContainingOutput(&DXSyncOutput));
  LARGE_INTEGER qpf;
  QueryPerformanceFrequency(&qpf);
  PerfCountFreq = qpf.QuadPart;
  QueryPerformanceCounter(&qpf);
  PerfCountStart = qpf.QuadPart;

  if(flags_ & sISF_2D)
    DXErr(DXSwapChain[0]->GetBuffer(0,__uuidof(IDXGISurface1),(void **)&DXGISurface));

  sInt tmode = sTEX_2D|sTEX_ARGB8888|sTEX_INTERNAL|sTEX_RENDERTARGET|sTEX_NOMIPMAPS;
  sInt zmode = sTEX_DEPTH24NOREAD;
  if(DXScreenMode.Flags & sSM_READZ)
    zmode = sTEX_DEPTH24;
  if(DXScreenMode.MultiLevel>=0)
  {
    tmode |= sTEX_MSAA;
    zmode |= sTEX_MSAA;
  }

  for(sInt i=0;i<DXScreenCount;i++)
  {
    sInt xs = DXScreenSizeX[i];
    sInt ys = DXScreenSizeY[i];
    DXBackBufferTex[i] = new sTexture2D(xs,ys,tmode,1);
    DXErr(DXSwapChain[i]->GetBuffer(0,__uuidof(ID3D11Texture2D),(void **)&DXBackBufferTex[i]->DXTex2D));    
    DXErr(DXDev->CreateRenderTargetView(DXBackBufferTex[i]->DXTex2D,0,&DXBackBufferTex[i]->DXRenderView));
    DXZBufferTex[i] = new sTexture2D(xs,ys,zmode|sTEX_2D|sTEX_RENDERTARGET,1);
  }

  if(DXScreenMode.RTZBufferX * DXScreenMode.RTZBufferY > 0)
  {
    DXRTZBufferTex = new sTexture2D(DXScreenMode.RTZBufferX,DXScreenMode.RTZBufferY,sTEX_2D|sTEX_DEPTH24NOREAD|sTEX_RENDERTARGET|sTEX_NOMIPMAPS,1);
  }

  // done

  DXActive = 1;
  DXRestore = 0;
}

void ExitGFX()
{
  DXActive = 0;

  sExitGfxCommon();

  sOccQueryNode *qn;
  if(AllOccQueryNodes)
  {
    sFORALL(*AllOccQueryNodes,qn)
      qn->Query->Release();
    sDeleteAll(*AllOccQueryNodes);
  }
  sDelete(AllOccQueryNodes);
  sDelete(FreeOccQueryNodes);

  for(sInt i=0;i<DXScreenCount;i++)
  {
    sDelete(DXBackBufferTex[i]);
    sDelete(DXZBufferTex[i]);
  }
  sDelete(DXRTZBufferTex);
  sDXRELEASE(DXGISurface);

  sDXRELEASE(DXQuadList);
  for(sInt i=0;i<DXScreenCount;i++)
  {
    DXErr(DXSwapChain[i]->SetFullscreenState(0,0));
    sDXRELEASE(DXSwapChain[i]);
  }
  sVERIFY(GTCBase==GTC);
  sDelete(GTCBase);
  GTC = 0;
//  sDXRELEASE(DXBaseCtx);
  sDXRELEASE(DXDev);
  sDXRELEASE(DXSyncOutput);
  sDXRELEASE(DXAdapter);
  sDXRELEASE(DXGI);

  sDelete(GeoBufferManager);
  sDelete(CBufferManager);
  delete DXMSAALevels;

  for(sInt i=0;i<MAXSCREENS;i++)
    sDelete(DXScreenInfos[i]);
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

HDC DXGIGetDC()
{
  HDC result;
  DXErr(DXGISurface->GetDC(0,&result));
  return result;
}

void DXGIReleaseDC()
{
  DXErr(DXGISurface->ReleaseDC(0));
}


/****************************************************************************/
/***                                                                      ***/
/***   Graphics Device Context                                            ***/
/***                                                                      ***/
/****************************************************************************/

sGfxThreadContext::sGfxThreadContext()
{
  OldGTC = 0;
  DXCtx = 0;
  DXCmd = 0;
  sClear(CurrentCBs);
  sClear(CurrentTexture);

}

sGfxThreadContext::~sGfxThreadContext()
{
  sDXRELEASE(DXCtx);
  sDXRELEASE(DXCmd);
}

void sGfxThreadContext::BeginRecording()
{
  if(!DXCmd)
    DXErr(DXDev->CreateDeferredContext(0,&DXCtx));
  sDXRELEASE(DXCmd);
  Flush();
}

void sGfxThreadContext::EndRecording()
{
  DXErr(DXCtx->FinishCommandList(0,&DXCmd));
}


void sGfxThreadContext::BeginUse()
{
  OldGTC = GTC;
  GTC = this;
}

void sGfxThreadContext::EndUse()
{
  GTC = OldGTC;
}

void sGfxThreadContext::Draw()
{
  sVERIFY(GTC==GTCBase);
  sVERIFY(this!=GTCBase);

  DXBaseCtx->ExecuteCommandList(DXCmd,0);
  GTCBase->Flush();
}

void sGfxThreadContext::Flush()
{
  if(DXCtx)
    DXCtx->ClearState();

  sClear(CurrentTexture);
  sClear(CurrentCBs);
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
  if(!DXActive)
    return 0;

  if(DXRestore)
  {
    if(DXMayRestore)
      InitGFX(sSystemFlags,DXScreenMode.ScreenX,DXScreenMode.ScreenY);
    else
      return 0;
  }

  sVERIFY(DXRendering == 0);
  DXRendering = 1;

  GTC->Flush();
  GeoBufferManager->Flush();
  CBufferManager->Flush();

  return 1;
}

struct cbd
{
  static const sInt RegStart = 0;
  static const sInt RegCount = 12;
  static const sInt Slot = sCBUFFER_VS|0;
  sMatrix44 ms_ss;
};

void sRender3DEnd(sBool flip)
{
  // checking sRender3DBEgin / End nesting


  sVERIFY(DXRendering == 1);

  sResolveTarget();

  DXRendering = 0;

  DXRenderTargetActive = 0;

  
  // really flip

  if(flip)
  {
    for(sInt i=0;i<DXScreenCount;i++)
      DXErr(DXSwapChain[i]->Present(DXPresentInterval,0));

    if(sSystemFlags & sISF_FULLSCREEN)
    {
      DXGI_FRAME_STATISTICS stat;
      DXSyncOutput->GetFrameStatistics(&stat);

      while(DXLastFrame==stat.SyncRefreshCount)
      {
        DXSyncOutput->WaitForVBlank();
        DXSyncOutput->GetFrameStatistics(&stat);
      }
      DXLastFrame = stat.SyncRefreshCount;
      
      sF64 tick = sF64(stat.SyncQPCTime.QuadPart-PerfCountStart)/sF64(PerfCountFreq);
      DXFrameTimeU64 = sU64(tick*1000000);
      DXFrameTime = sInt(tick*1000);
    }
  }
  sVERIFY(GTC == GTCBase);
  GTC->Flush();

  BufferedStats = Stats;
  sClear(Stats);
  StatsEnable = 1;

  if(sSystemFlags & sISF_2D)
    GTC->DXCtx->Flush();

  sFlipMem();
}

void sRender3DFlush()
{
}

/****************************************************************************/

void sSetDesiredFrameRate(sF32 rate)
{
}


void SetXSIModeD3D(sBool enable)
{
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

sInt sGetDisplayCount()
{
  sInt n = 0;
  for(sInt i=0;i<MAXSCREENS;i++)
    if(DXScreenInfos[i]) n = i+1;
  return n;
}

sInt sGetFrameTime()
{
  if(sSystemFlags & sISF_FULLSCREEN)
    return DXFrameTime;
  else
    return sGetTime();
}

sU64 sGetFrameMicroSecond()
{
  return DXFrameTimeU64;
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
  // initialize screen info (possibly, Graphics are not initialized)

  if(DXScreenInfos[0]==0)
  {
    IDXGIFactory1 *fac;
    IDXGIAdapter1 *ada;
    IDXGIOutput *out;
    HRESULT hr;

    sInt n = 0;
    DXErr(CreateDXGIFactory1(__uuidof(IDXGIFactory1),(void**)(&fac)));
    for(sInt i=0;;i++)
    {
      hr = fac->EnumAdapters1(i,&ada);
      if(hr==DXGI_ERROR_NOT_FOUND) break;
      DXErr(hr);
  
      DXGI_ADAPTER_DESC adesc;
      DXErr(ada->GetDesc(&adesc));
      

      const D3D_FEATURE_LEVEL levels[4] = { D3D_FEATURE_LEVEL_11_0,D3D_FEATURE_LEVEL_10_1,D3D_FEATURE_LEVEL_10_0,D3D_FEATURE_LEVEL_9_3 };
      ID3D11Device *dev;
      hr = D3D11CreateDevice(ada,D3D_DRIVER_TYPE_UNKNOWN,0,0,levels,3,D3D11_SDK_VERSION,&dev,0,0);
      if(hr==DXGI_ERROR_UNSUPPORTED) break;
      DXErr(hr);

      sInt msaa = 0;
      for(sInt _i=2;_i<=D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT;_i++)
      {
        UINT _n;
        DXErr(dev->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM,_i,&_n));
        msaa += _n;
      }

      DXErr(dev->Release());

      for(sInt j=0;;j++)
      {
        hr = ada->EnumOutputs(j,&out);
        if(hr==DXGI_ERROR_NOT_FOUND) break;
        DXErr(hr);
        UINT num = 0;
        DXGI_MODE_DESC *desc = 0;

        DXErr(out->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM,DXGI_ENUM_MODES_SCALING|DXGI_ENUM_MODES_INTERLACED,&num,desc));
        if(num>0)
        {
          sScreenInfoX *info = DXScreenInfos[n++] = new sScreenInfoX;
          info->Display = n-1;
          info->Flags = 0;

          desc = new DXGI_MODE_DESC[num];
          DXErr(out->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM,DXGI_ENUM_MODES_SCALING|DXGI_ENUM_MODES_INTERLACED,&num,desc));
          info->Resolutions.HintSize(num);
          info->AspectRatios.HintSize(num);
          info->RefreshRates.HintSize(num);
          info->Adapter = i;
          info->MonitorOfAdapter = j;

          for(UINT i=0;i<num;i++)
          {
            sLogF(L"gfx",L"mode: screen[%d] %d x %d (%d:%d)\n",n-1,desc[i].Width,desc[i].Height,desc[i].RefreshRate.Numerator,desc[i].RefreshRate.Denominator);
            sScreenInfoXY xy;
            xy.Init(desc[i].Width,desc[i].Height);
            if(!sFind(info->Resolutions,xy))
              info->Resolutions.AddTail(xy);
            sInt teiler = ggt(xy.x,xy.y);
            xy.x = xy.x/teiler;
            xy.y = xy.y/teiler;
            if(!sFind(info->AspectRatios,xy))
              info->AspectRatios.AddTail(xy);
            xy.Init(desc[i].RefreshRate.Denominator,desc[i].RefreshRate.Numerator);
            sInt refresh = xy.y / xy.x;
            if(!sFind(info->RefreshRates,refresh))
              info->RefreshRates.AddTail(refresh);
          }

          sDeleteArray(desc);

          DXGI_OUTPUT_DESC od;
          DXErr(out->GetDesc(&od));
          info->PosX = od.DesktopCoordinates.left;
          info->PosY = od.DesktopCoordinates.top;
          info->CurrentXSize = od.DesktopCoordinates.right - od.DesktopCoordinates.left;
          info->CurrentYSize = od.DesktopCoordinates.bottom - od.DesktopCoordinates.top;          
          info->CurrentAspect = sF32(info->CurrentXSize) / info->CurrentYSize;
          info->MultisampleLevels = msaa;

          DXGI_OUTPUT_DESC odesc;
          sClear(odesc);
          out->GetDesc(&odesc);
          DISPLAY_DEVICE ddev;
          sClear(ddev);
          ddev.cb=sizeof(ddev);
          EnumDisplayDevices(odesc.DeviceName,0,&ddev,1);
          sCopyString(info->MonitorName,ddev.DeviceString);
        }

        DXErr(out->Release());
      }

      DXErr(ada->Release());
    }
  }

  // copy screeninfo;

  if(display<0)
    display = 0;
  if(display>=MAXSCREENS || DXScreenInfos[display]==0)
  {
    sClear(si);
  }
  else
  {
    sScreenInfo *src = DXScreenInfos[display];
    si.Flags = src->Flags;
    si.Display = src->Display;
    si.CurrentXSize = src->CurrentXSize;
    si.CurrentYSize = src->CurrentYSize;
    si.CurrentAspect = src->CurrentAspect;
    si.MultisampleLevels = src->MultisampleLevels;

    si.Resolutions = src->Resolutions;
    si.RefreshRates = src->RefreshRates;
    si.AspectRatios = src->AspectRatios;

    si.MonitorName = src->MonitorName;
  }
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
/***   General Engine Interface, new style                                ***/
/***                                                                      ***/
/****************************************************************************/

sTextureBasePrivate::sTextureBasePrivate()
{ 
  DXTex2D=0; 
  DXTexView=0; 
  DXTexUAV=0; 
  DXRenderView=0; 
  DXDepthView=0; 
  sClear(DXCubeRenderView);
  LoadPtr=0; 
  Dynamic=0; 
  LoadMipmap=-1; 
  LoadCubeFace=-1; 
  DXFormat=0;
  MultiSampled=0; 
  NeedResolve=0; 
  NeedAutomipmap=0;
  DXReadTex=0;
}

void sTextureBasePrivate::ResolvePrivate()
{ 
  if(NeedResolve)
  {
    GTC->DXCtx->ResolveSubresource(DXTex2D,0,MultiSampled->DXTex2D,0,(DXGI_FORMAT)DXFormat);
    NeedResolve = 0;
  }
  if(NeedAutomipmap)
  {
    GTC->DXCtx->GenerateMips(DXTexView);
    NeedAutomipmap = 0;
  }
}

static sTextureBase *DXResolveMe[5];    // resolve belongs to texture, not rendertarget!

void sSetTarget(const sTargetPara &para)
{
  // clear all textures, in case a target is still bound as input

  static ID3D11ShaderResourceView *tps[sMTRL_MAXPSTEX];
  static ID3D11ShaderResourceView *tvs[sMTRL_MAXVSTEX];
  GTC->DXCtx->PSSetShaderResources(0,sMTRL_MAXPSTEX,tps);
  GTC->DXCtx->VSSetShaderResources(0,sMTRL_MAXVSTEX,tvs);

  // ...

  DXRenderTargetActive = 1;

  sBool ms = 0;
  
  if(!(para.Flags & sST_NOMSAA) && !(para.Flags & sST_READZ))
  {
    ms = 1;
    if(para.Depth && !para.Depth->MultiSampled)
    {
      ms = 0;
    }
    else
    {
      for(sInt i=0;i<4;i++)
      {
        if(para.Target[i] && (!para.Target[i]->MultiSampled || (para.Target[i]->Flags&sTEX_TYPE_MASK)!=sTEX_2D))
        {
          ms = 0;
          break;
        }
      }
    }
  }
  if(ms==0)
  {
    for(sInt i=0;i<4;i++)
      if(para.Target[i]) 
        para.Target[i]->ResolvePrivate();
    if(para.Depth)
      para.Depth->ResolvePrivate();
  }
  sClear(DXResolveMe);
  for(sInt i=0;i<4;i++)
  {
    if(para.Target[i]) 
    {
      para.Target[i]->NeedResolve = ms;
      DXResolveMe[i] = para.Target[i];
      if(para.Target[i]->Flags & sTEX_AUTOMIPMAP)
        para.Target[i]->NeedAutomipmap = 1;
    }
  }
  if(para.Depth && (para.Flags & sST_READZ))
  {
    para.Depth->NeedResolve = ms;
    DXResolveMe[4] = para.Depth;
  }

  ID3D11RenderTargetView *rt[4];
  sClear(rt);
  for(sInt i=0;i<4;i++)
  {
    if(para.Target[i])
    {
      switch(para.Target[i]->Flags & sTEX_TYPE_MASK)
      {
      case sTEX_2D:
        rt[i] = ms ? para.Target[i]->MultiSampled->DXRenderView : para.Target[i]->DXRenderView;
        break;
      case sTEX_CUBE:
        sVERIFY(para.Cubeface>=0 && para.Cubeface<6);
        rt[i] = para.Target[i]->DXCubeRenderView[para.Cubeface];
        break;
      }
    }
  }

  sGFXViewRect = para.Window;
  sGFXRendertargetX = para.Window.SizeX();
  sGFXRendertargetY = para.Window.SizeY();
  if(para.Aspect==0)
    sGFXRendertargetAspect = sF32(sGFXRendertargetX)/sGFXRendertargetY;
  else
    sGFXRendertargetAspect = para.Aspect;

  ID3D11DepthStencilView *db = 0;
  if(para.Depth)
    db = ms ? para.Depth->MultiSampled->DXDepthView : para.Depth->DXDepthView;
  GTC->DXCtx->OMSetRenderTargets(4,rt,db);
  D3D11_VIEWPORT vp;

  vp.Width = sGFXRendertargetX;
  vp.Height = sGFXRendertargetY;
  vp.MinDepth = 0.0f;
  vp.MaxDepth = 1.0f;
  vp.TopLeftX = para.Window.x0;
  vp.TopLeftY = para.Window.y0;
  GTC->DXCtx->RSSetViewports(1,&vp);

  if(para.Flags & sCLEAR_COLOR)
  {
    for(sInt i=0;i<4;i++)
      if(rt[i])
        GTC->DXCtx->ClearRenderTargetView(rt[i],&para.ClearColor[i].x);
  }
  if((para.Flags & sCLEAR_ZBUFFER) && db)
    GTC->DXCtx->ClearDepthStencilView(db,D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL,para.ClearZ,0);
  GTC->DXCtx->RSSetScissorRects(1,(D3D11_RECT *)&para.Window);
}

void sResolveTarget()
{
  for(sInt i=0;i<5;i++)
    if(DXResolveMe[i])
      DXResolveMe[i]->ResolvePrivate();
  sClear(DXResolveMe);
}

void sResolveTexture(sTextureBase *tex)
{
  tex->ResolvePrivate();
}

void sSetScissor(const sRect &r)
{
  GTC->DXCtx->RSSetScissorRects(1,(RECT*)(&r));
}

/****************************************************************************/

sInt sGetScreenCount()
{
  return DXScreenCount;
}

sTexture2D *sGetScreenColorBuffer(sInt screen)
{
  return DXBackBufferTex[screen];
}

sTexture2D *sGetScreenDepthBuffer(sInt screen)
{
  return DXZBufferTex[screen];
}

sTexture2D *sGetRTDepthBuffer()
{
  return DXRTZBufferTex;
}

void sEnlargeRTDepthBuffer(sInt x, sInt y)
{
  if(DXScreenMode.RTZBufferX < x || DXScreenMode.RTZBufferY < y)
  {
    DXScreenMode.RTZBufferX = x;
    DXScreenMode.RTZBufferY = y;
    if(DXDev)
    {
      sDelete(DXRTZBufferTex);
      DXRTZBufferTex = new sTexture2D(DXScreenMode.RTZBufferX,DXScreenMode.RTZBufferY,sTEX_2D|sTEX_DEPTH24NOREAD|sTEX_RENDERTARGET|sTEX_NOMIPMAPS,1);
    }
  }
}


/****************************************************************************/

static ID3D11Texture2D *DXReadTexture;
void sBeginReadTexture(const sU8*& data, sS32& pitch, enum sTextureFlags& flags,sTexture2D *tex)
{
  sVERIFY(DXReadTexture == 0);
  sVERIFY((tex->Flags & sTEX_TYPE_MASK)==sTEX_2D); 

  if(tex->DXReadTex==0)
  {
    D3D11_TEXTURE2D_DESC td;
    sClear(td);
    tex->DXTex2D->GetDesc(&td);
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Usage = D3D11_USAGE_STAGING;
    td.BindFlags = 0;
    td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    td.MiscFlags = 0;
    DXErr(DXDev->CreateTexture2D(&td,0,&tex->DXReadTex));
  }

  DXReadTexture = tex->DXReadTex;

  tex->ResolvePrivate();

  GTC->DXCtx->CopySubresourceRegion(tex->DXReadTex,0,0,0,0,tex->DXTex2D,0,0);
  D3D11_MAPPED_SUBRESOURCE map;
  GTC->DXCtx->Map(tex->DXReadTex,0,D3D11_MAP_READ,0,&map);
  data = (sU8 *) map.pData;
  pitch = map.RowPitch;
  flags = (enum sTextureFlags) (tex->Flags & sTEX_FORMAT);
}

void sEndReadTexture()
{
  sVERIFY(DXReadTexture);
  GTC->DXCtx->Unmap(DXReadTexture,0);
  DXReadTexture = 0;
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
  
  DXGI_FORMAT fmt_res,fmt_tex,fmt_view;
  D3D11_USAGE usage;
  D3D11_BIND_FLAG bind;
  sBool ds;
  sInt mm;

  ConvertFlags(flags,mm,bind,usage,fmt_res,fmt_tex,fmt_view,ds);

  D3D11_TEXTURE2D_DESC td;
  sClear(td);
  td.Width = SizeX;
  td.Height = SizeY;
  td.MipLevels = 1;
  td.ArraySize = 1;
  td.Format = fmt_tex;
  td.SampleDesc.Count = 1;
  td.SampleDesc.Quality = 0;
  td.Usage = D3D11_USAGE_STAGING;
  td.BindFlags = 0;
  td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  td.MiscFlags = 0;
  DXErr(DXDev->CreateTexture2D(&td,0,&Dest));
}

sGpuToCpu::~sGpuToCpu()
{
  sVERIFY(!Locked);
  sDXRELEASE(Dest);
}


void sGpuToCpu::CopyFrom(sTexture2D *tex,sInt miplevel)
{
  sVERIFY(!Locked);
  tex->ResolvePrivate();
  if(tex->Mipmaps==1 && miplevel==0)
    GTC->DXCtx->CopyResource(Dest,tex->DXTex2D);
  else
    GTC->DXCtx->CopySubresourceRegion(Dest,0,0,0,0,tex->DXTex2D,miplevel,0);
}

const void *sGpuToCpu::BeginRead(sDInt &pitch)
{
  sVERIFY(!Locked);
  Locked = 1;

  D3D11_MAPPED_SUBRESOURCE map;
  GTC->DXCtx->Map(Dest,0,D3D11_MAP_READ,0,&map);
  pitch = map.RowPitch;
  return map.pData;
}

void sGpuToCpu::EndRead()
{
  sVERIFY(Locked);
  Locked = 0;
  GTC->DXCtx->Unmap(Dest,0);
}

/****************************************************************************/

void sCopyTexture(const sCopyTexturePara &para)
{
  D3D11_BOX box;
  box.left = para.SourceRect.x0;
  box.top = para.SourceRect.y0;
  box.right = para.SourceRect.x1;
  box.bottom = para.SourceRect.y1;
  box.front = 0;
  box.back = 1;

  if(para.Source) para.Source->ResolvePrivate();

  ID3D11Resource *s = 0;
  if(para.Source && (para.Source->Flags & sTEX_TYPE_MASK)==sTEX_2D)
    s = ((sTexture2D *) para.Source)->DXTex2D;
  if(para.Source && (para.Source->Flags & sTEX_TYPE_MASK)==sTEX_CUBE)
    s = ((sTextureCube *) para.Source)->DXTex2D;

  ID3D11Resource *d = 0;
  if(para.Dest && (para.Dest->Flags & sTEX_TYPE_MASK)==sTEX_2D)
    d = ((sTexture2D *) para.Dest)->DXTex2D;
  if(para.Dest && (para.Dest->Flags & sTEX_TYPE_MASK)==sTEX_CUBE)
    d = ((sTextureCube *) para.Dest)->DXTex2D;


  GTC->DXCtx->CopySubresourceRegion(d,0,para.DestRect.x0,para.DestRect.y0,0,
                               s,0,&box);

  para.Dest->NeedResolve = 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   General Engine Interface                                           ***/
/***                                                                      ***/
/****************************************************************************/

void sConvertsRGBTex(sBool e)
{
  sVERIFYFALSE;
}

sBool sConvertsRGBTex()
{
  sVERIFYFALSE;
  return sFALSE;
}

sTexture2D *sGetCurrentFrontBuffer() { return 0; }

void sSetScreen(class sTexture2D *tex, sGrabFilterFlags filter, const sRect *dst, const sRect *src)
{
  sFatal(L"sSetScreen() not implemented");
}

void sSetScreen(const sRect &rect,sU32 *data)
{
  sFatal(L"sSetScreen() not implemented");
}

void sBeginSaveRT(const sU8*& data, sS32& pitch, enum sTextureFlags& flags)
{
  sFatal(L"sBeginSaveRT() not implemented");
}

void sEndSaveRT()
{
  sFatal(L"sEndSaveRT() not implemented");
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

void sGetGraphicsCaps(sGraphicsCaps &caps)
{
  caps = GraphicsCapsMaster;
}


/****************************************************************************/
/***                                                                      ***/
/***   Vertex Formats                                                     ***/
/***                                                                      ***/
/****************************************************************************/



void sVertexFormatHandle::Create()
{
  // create vertex declarator 
  D3D11_INPUT_ELEMENT_DESC decl[32];
  sInt b[sVF_STREAMMAX];
  sInt stream;

  for(sInt i=0;i<sVF_STREAMMAX;i++)
    b[i] = 0;

  // create declaration

  sInt count = 0;
  for(sInt i=0;Data[i];i++)
  {
    stream = (Data[i]&sVF_STREAMMASK)>>sVF_STREAMSHIFT;

    decl[i].InputSlot = stream;
    decl[i].AlignedByteOffset = b[stream];
    decl[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    decl[i].InstanceDataStepRate = 0;
    if(Data[i]&sVF_INSTANCEDATA)
    {
      decl[i].InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
      decl[i].InstanceDataStepRate = 1;
    }

    sVERIFY(count<31);

    switch(Data[i]&sVF_USEMASK)
    {
    case sVF_NOP:   break;
    case sVF_POSITION:    decl[count].SemanticName = "POSITION";      decl[count].SemanticIndex = 0; break;
    case sVF_NORMAL:      decl[count].SemanticName = "NORMAL";        decl[count].SemanticIndex = 0; break;
    case sVF_TANGENT:     decl[count].SemanticName = "TANGENT";       decl[count].SemanticIndex = 0; break;
    case sVF_BONEINDEX:   decl[count].SemanticName = "BLENDINDICES";  decl[count].SemanticIndex = 0; break;
    case sVF_BONEWEIGHT:  decl[count].SemanticName = "BLENDWEIGHT";   decl[count].SemanticIndex = 0; break;
    case sVF_BINORMAL:    decl[count].SemanticName = "BINORMAL";      decl[count].SemanticIndex = 0; break;
    case sVF_COLOR0:      decl[count].SemanticName = "COLOR";         decl[count].SemanticIndex = 0; break;
    case sVF_COLOR1:      decl[count].SemanticName = "COLOR";         decl[count].SemanticIndex = 1; break;
    case sVF_COLOR2:      decl[count].SemanticName = "COLOR";         decl[count].SemanticIndex = 2; break;
    case sVF_COLOR3:      decl[count].SemanticName = "COLOR";         decl[count].SemanticIndex = 3; break;
    case sVF_UV0:         decl[count].SemanticName = "TEXCOORD";      decl[count].SemanticIndex = 0; break;
    case sVF_UV1:         decl[count].SemanticName = "TEXCOORD";      decl[count].SemanticIndex = 1; break;
    case sVF_UV2:         decl[count].SemanticName = "TEXCOORD";      decl[count].SemanticIndex = 2; break;
    case sVF_UV3:         decl[count].SemanticName = "TEXCOORD";      decl[count].SemanticIndex = 3; break;
    case sVF_UV4:         decl[count].SemanticName = "TEXCOORD";      decl[count].SemanticIndex = 4; break;
    case sVF_UV5:         decl[count].SemanticName = "TEXCOORD";      decl[count].SemanticIndex = 5; break;
    case sVF_UV6:         decl[count].SemanticName = "TEXCOORD";      decl[count].SemanticIndex = 6; break;
    case sVF_UV7:         decl[count].SemanticName = "TEXCOORD";      decl[count].SemanticIndex = 7; break;
    default: sVERIFYFALSE;
    }

    switch(Data[i]&sVF_TYPEMASK)
    {
    case sVF_F2:  decl[count].Format = DXGI_FORMAT_R32G32_FLOAT;        b[stream]+=2*4;  break;
    case sVF_F3:  decl[count].Format = DXGI_FORMAT_R32G32B32_FLOAT;     b[stream]+=3*4;  break;
    case sVF_F4:  decl[count].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;  b[stream]+=4*4;  break;
    case sVF_I4:  decl[count].Format = DXGI_FORMAT_R8G8B8A8_UINT;       b[stream]+=1*4;  break;
    case sVF_C4:  decl[count].Format = DXGI_FORMAT_B8G8R8A8_UNORM;      b[stream]+=1*4;  break;
    case sVF_S2:  decl[count].Format = DXGI_FORMAT_R16G16_SNORM;        b[stream]+=1*4;  break;
    case sVF_S4:  decl[count].Format = DXGI_FORMAT_R16G16B16A16_SNORM;  b[stream]+=2*4;  break;
    case sVF_H2:  decl[count].Format = DXGI_FORMAT_R16G16_FLOAT;        b[stream]+=1*4;  break;
    case sVF_H4:  decl[count].Format = DXGI_FORMAT_R16G16B16A16_FLOAT;  b[stream]+=2*4;  break;
    case sVF_F1:  decl[count].Format = DXGI_FORMAT_R32_FLOAT;           b[stream]+=1*4;  break;

    default: sVERIFYFALSE;
    }

    AvailMask |= 1 << (Data[i]&sVF_USEMASK);
    Streams = sMax(Streams,stream+1);
    count++;
  }

  for(sInt i=0;i<sVF_STREAMMAX;i++)
    VertexSize[i] = b[i];

  // create a dummy shader that will provide the input layout signature (i hate this)

  sTextBuffer tb;

  tb.Print(L"void main(\n");
  for(sInt i=0;Data[i];i++)
  {
    tb.Print(L"  in ");
    switch(Data[i]&sVF_TYPEMASK)
    {
    case sVF_F2:  tb.Print(L"float2 "); break;
    case sVF_F3:  tb.Print(L"float3 "); break;
    case sVF_F4:  tb.Print(L"float4 "); break;
    case sVF_I4:  tb.Print(L"uint4 "); break;
    case sVF_C4:  tb.Print(L"float4 "); break;
    case sVF_S2:  tb.Print(L"float2 "); break;
    case sVF_S4:  tb.Print(L"float4 "); break;
    case sVF_H2:  tb.Print(L"float2 "); break;
    case sVF_H4:  tb.Print(L"float4 "); break;
    case sVF_F1:  tb.Print(L"float1 "); break;
    }
    tb.PrintF(L"_%d : ",i);
    switch(Data[i]&sVF_USEMASK)
    {
    case sVF_POSITION:    tb.Print(L"POSITION"); break;
    case sVF_NORMAL:      tb.Print(L"NORMAL"); break;
    case sVF_TANGENT:     tb.Print(L"TANGENT"); break;
    case sVF_BONEINDEX:   tb.Print(L"BLENDINDICES"); break;
    case sVF_BONEWEIGHT:  tb.Print(L"BLENDWEIGHT"); break;
    case sVF_BINORMAL:    tb.Print(L"BINORMAL"); break;
    case sVF_COLOR0:      tb.Print(L"COLOR0"); break;
    case sVF_COLOR1:      tb.Print(L"COLOR1"); break;
    case sVF_COLOR2:      tb.Print(L"COLOR2"); break;
    case sVF_COLOR3:      tb.Print(L"COLOR3"); break;
    case sVF_UV0:         tb.Print(L"TEXCOORD0"); break;
    case sVF_UV1:         tb.Print(L"TEXCOORD1"); break;
    case sVF_UV2:         tb.Print(L"TEXCOORD2"); break;
    case sVF_UV3:         tb.Print(L"TEXCOORD3"); break;
    case sVF_UV4:         tb.Print(L"TEXCOORD4"); break;
    case sVF_UV5:         tb.Print(L"TEXCOORD5"); break;
    case sVF_UV6:         tb.Print(L"TEXCOORD6"); break;
    case sVF_UV7:         tb.Print(L"TEXCOORD7"); break;
    }
    tb.Print(L",\n");
  }
  tb.Print(L"  out float4 pos:SV_POSITION\n");
  tb.Print(L"){ pos=0; }\n");

  ID3D10Blob *VSD=0,*error=0;

  sInt len = tb.GetCount();
  sChar8 *buffer = new sChar8[len+1];
  sCopyString(buffer,tb.Get(),len);
  buffer[len] = 0;
  UINT f0=0,f1=0;

  if(0) sDPrint(tb.Get());

  DXErr((*DXCompLibCompile)(buffer,len,"mem",0,0,"main","vs_4_0",f0,f1,&VSD,&error));
  if(error)
    error->Release();
  delete[] buffer;

  // create

  DXErr(DXDev->CreateInputLayout(decl,count,VSD->GetBufferPointer(),VSD->GetBufferSize(),&Layout));
  VSD->Release();
}

void sVertexFormatHandle::Destroy()
{
  sDXRELEASE(Layout);
}


/****************************************************************************/
/***                                                                      ***/
/***   Shader Interface                                                   ***/
/***                                                                      ***/
/****************************************************************************/

void sCreateShader2(sShader *shader, sShaderBlob *blob)
{
  if((blob->Type&sSTF_PLATFORM)!=sSTF_HLSL45)
  {
    sLogF(L"gfx",L"sCreateShader2 only supports hlsl shader model 4 and 5\n");
    shader->vs = 0;
    return;
  }

  switch(shader->Type&sSTF_KIND)
  {
  case sSTF_VERTEX:
    DXErr(DXDev->CreateVertexShader((const DWORD *)blob->Data,blob->Size,0,&shader->vs));
    break;
  case sSTF_HULL:
    DXErr(DXDev->CreateHullShader((const DWORD *)blob->Data,blob->Size,0,&shader->hs));
    break;
  case sSTF_DOMAIN:
    DXErr(DXDev->CreateDomainShader((const DWORD *)blob->Data,blob->Size,0,&shader->ds));
    break;
  case sSTF_GEOMETRY:
    DXErr(DXDev->CreateGeometryShader((const DWORD *)blob->Data,blob->Size,0,&shader->gs));
    break;
  case sSTF_PIXEL:
    DXErr(DXDev->CreatePixelShader((const DWORD *)blob->Data,blob->Size,0,&shader->ps));
    break;
  case sSTF_COMPUTE:
    DXErr(DXDev->CreateComputeShader((const DWORD *)blob->Data,blob->Size,0,&shader->cs));
    break;
  default:
    sVERIFYFALSE;
  }
}

void sDeleteShader2(sShader *shader)
{
  sDXRELEASE(shader->vs);
}

/****************************************************************************/

void sSetVSParam(sInt o, sInt count, const sVector4* vsf)
{
  sFatal(L"sSetVSParam() not implemented");
}

void sSetPSParam(sInt o, sInt count, const sVector4* psf)
{
  sFatal(L"sSetPSParam() not implemented");
}

void sSetVSBool(sU32 bits,sU32 mask)
{
  sFatal(L"sSetVSBool() not implemented");
}

void sSetPSBool(sU32 bits,sU32 mask)
{
  sFatal(L"sSetPSBool() not implemented");
}

sShaderTypeFlag sGetShaderPlatform()
{
  return sSTF_HLSL45;
}

sInt sGetShaderProfile()
{
  return 0;
}

/****************************************************************************/
/****************************************************************************/

sCBuffer11::sCBuffer11(sInt size,sInt bin)
{
  D3D11_BUFFER_DESC bd;

  sClear(bd);
  bd.ByteWidth = size;
  bd.Usage = D3D11_USAGE_DYNAMIC;
  bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

  DXErr(DXDev->CreateBuffer(&bd,0,&DXBuffer));  

  Size = size;
  Bin = bin;
}

sCBuffer11::~sCBuffer11()
{
  sDXRELEASE(DXBuffer);  
}

/****************************************************************************/

sCBufferManager::sCBufferManager()
{

}

sCBufferManager::~sCBufferManager()
{
  sCBuffer11 *cb;
  Flush();

  if(!sRELEASE)
  {
    sLog(L"gfx",L"constant buffer resources used:\n");
    for(sInt i=1;i<sCBM_BinCount;i++)
    {
      sInt n = Bins[i].GetCount();
      if(n>0)
        sLogF(L"gfx",L"%-4d bytes: %d\n",i*sCBM_BinSize,n);
    }
    sFORALL_LIST(Bins[0],cb)
      sLogF(L"gfx",L"%-4d bytes: one\n",cb->Size);
  }

  for(sInt i=0;i<sCBM_BinCount;i++)
  {
    while(!Bins[i].IsEmpty())
    {
      cb = Bins[i].RemTail();
      delete cb;
    }
  }
  sVERIFY(Mapped.IsEmpty());
}

void sCBufferManager::Map(sCBufferMap &map,sInt size)
{
  // find one

  sInt fullsize = sAlign(size,sCBM_BinSize);
  sInt bin = fullsize/sCBM_BinSize;
  if(bin>=sCBM_BinCount)
    bin = 0;

  sCBuffer11 *cb = 0;
  if(bin==0)            // bin[0] holds the oversized cbuffers
  {
    sFORALL_LIST(Bins[0],cb)
    {
      if(cb->Size==fullsize)
      {
        Bins[0].Rem(cb);
        goto found;
      }
    }
    cb = new sCBuffer11(fullsize,0);
found:;
  }
  else
  {
    if(Bins[bin].IsEmpty())
      cb = new sCBuffer11(fullsize,bin);
    else
      cb = Bins[bin].RemTail();
  }

  // map

  D3D11_MAPPED_SUBRESOURCE mr;
  DXErr(GTC->DXCtx->Map(cb->DXBuffer,0,D3D11_MAP_WRITE_DISCARD,0,&mr));
  map.Ptr = mr.pData;
  map.Buffer = cb;

  Mapped.AddTail(cb);
  cb->IsMapped = 1;
}

void sCBufferManager::Unmap(sCBufferMap &map)
{
  sCBuffer11 *cb = map.Buffer;
  sVERIFY(cb->IsMapped);

  GTC->DXCtx->Unmap(cb->DXBuffer,0);

  Mapped.Rem(cb);
  Used.AddTail(cb);
}

void sCBufferManager::Flush()
{
  sVERIFY(Mapped.IsEmpty());

  while(!Used.IsEmpty())
  {
    sCBuffer11 *cb = Used.RemTail();
    Bins[cb->Bin].AddTail(cb);
  }
}

/****************************************************************************/

sCBufferBase::sCBufferBase()
{
  DataPtr = 0;
  DataPersist = 0;
  Slot = 0;
  Flags = 0;
  sClear(Map);
}

sCBufferBase::~sCBufferBase()
{
}

void sCBufferBase::OverridePtr(void *ptr)
{
  DataPtr = 0;
  DataPersist = ptr;
  Modify();
}

void sCBufferBase::SetPtr(void **dataptr,void *data)
{
  DataPtr = dataptr;
  DataPersist = data;
  if(DataPtr)
    *DataPtr = DataPersist;
}

void sCBufferBase::Modify()
{
  if(DataPtr)
    *DataPtr = DataPersist;

  sClear(Map);
  if(GTC->CurrentCBs[Slot]==this)
    GTC->CurrentCBs[Slot] = 0;
}

void sCBufferBase::SetCfg(sInt slot, sInt start, sInt count)
{
  Slot = slot;
  RegStart = start;
  RegCount = count;
  if(GTC->CurrentCBs[Slot]==this)
    GTC->CurrentCBs[Slot] = 0;
}

void sCBufferBase::SetRegs()
{
  if(Map.Buffer == 0)
  {
    CBufferManager->Map(Map,RegCount*16);
    sCopyMem(Map.Ptr,DataPersist,RegCount*16);
    CBufferManager->Unmap(Map);
  }
  switch(Slot & sCBUFFER_SHADERMASK)
  {
  case sCBUFFER_VS:
    GTC->DXCtx->VSSetConstantBuffers(Slot&(sCBUFFER_MAXSLOT-1),1,&Map.Buffer->DXBuffer);
    break;
  case sCBUFFER_HS:
    GTC->DXCtx->HSSetConstantBuffers(Slot&(sCBUFFER_MAXSLOT-1),1,&Map.Buffer->DXBuffer);
    break;
  case sCBUFFER_DS:
    GTC->DXCtx->DSSetConstantBuffers(Slot&(sCBUFFER_MAXSLOT-1),1,&Map.Buffer->DXBuffer);
    break;
  case sCBUFFER_GS:
    GTC->DXCtx->GSSetConstantBuffers(Slot&(sCBUFFER_MAXSLOT-1),1,&Map.Buffer->DXBuffer);
    break;
  case sCBUFFER_PS:
    GTC->DXCtx->PSSetConstantBuffers(Slot&(sCBUFFER_MAXSLOT-1),1,&Map.Buffer->DXBuffer);
    break;
  case sCBUFFER_CS:
    GTC->DXCtx->CSSetConstantBuffers(Slot&(sCBUFFER_MAXSLOT-1),1,&Map.Buffer->DXBuffer);
    break;
  default:
    sVERIFYFALSE;
    break;
  }
}


/*
sCBufferBase::sCBufferBase()
{
  DXBuffer = 0;
  Mapped = 0;
}

void sCBufferBase::SetPtr(void **data,void *)
{
  D3D11_BUFFER_DESC bd;

  bd.ByteWidth = RegCount*16;
  bd.Usage = D3D11_USAGE_DYNAMIC;
  bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  bd.MiscFlags = 0;
  bd.StructureByteStride = 0;

  DXErr(DXDev->CreateBuffer(&bd,0,&DXBuffer));

  DataPtr = data;
}

extern sCBufferBase *CurrentCBs[];

void sCBufferBase::Modify()
{
  if(!Mapped)
  {
    D3D11_MAPPED_SUBRESOURCE map;
    sClear(map);
    DXErr(GTC->DXCtx->Map(DXBuffer,0,D3D11_MAP_WRITE_DISCARD,0,&map));
    *DataPtr = map.pData;
    Mapped = 1;
  }
  if(CurrentCBs[Slot]==this)
    CurrentCBs[Slot] = 0;
}

void sCBufferBase::SetRegs()
{
  if(Mapped)
  {
    GTC->DXCtx->Unmap(DXBuffer,0);
    Mapped = 0;
  }
  switch(Slot&sCBUFFER_SHADERMASK)
  {
  case sCBUFFER_VS: GTC->DXCtx->VSSetConstantBuffers(Slot,1,&DXBuffer); break;
  case sCBUFFER_PS: GTC->DXCtx->PSSetConstantBuffers(Slot,1,&DXBuffer); break;
  case sCBUFFER_GS: GTC->DXCtx->GSSetConstantBuffers(Slot,1,&DXBuffer); break;
  default: sVERIFYFALSE;
  }
}

void sCBufferBase::OverridePtr(void *ptr)
{
  Modify();
  sCopyMem(DataPtr,ptr,RegCount*16);
}

sCBufferBase::~sCBufferBase()
{
  if(Mapped)
    GTC->DXCtx->Unmap(DXBuffer,0);
  sDXRELEASE(DXBuffer);
}
*/


void sSetCBuffers(sCBufferBase **cbuffers,sInt cbcount)
{ 
  for(sInt i=0;i<cbcount;i++)
  { 
    sCBufferBase *cb = cbuffers[i];
    if(cb && cb != GTC->CurrentCBs[cb->Slot])
    { 
      cb->SetRegs();
      GTC->CurrentCBs[cb->Slot] = cb;
    }
  }
}

void sClearCurrentCBuffers()
{
  sClear(GTC->CurrentCBs);
}

sCBufferBase *sGetCurrentCBuffer(sInt slot)
{ 
  sVERIFY(slot<sCBUFFER_MAXSLOT*sCBUFFER_SHADERTYPES);
  return GTC->CurrentCBs[slot];
}

/****************************************************************************/
/***                                                                      ***/
/***   Geometry Begin/End interface                                       ***/
/***                                                                      ***/
/****************************************************************************/

sGeoBuffer11::sGeoBuffer11(sDInt bytes)
{
  Alloc = bytes;
  Used = 0;
  Free = bytes;
  MappedCount = 0;
  MapPtr = 0;
  
  D3D11_BUFFER_DESC bd;

  sClear(bd);
  bd.ByteWidth = bytes;
  bd.Usage = D3D11_USAGE_DYNAMIC;
  bd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER;
  bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

  DXErr(DXDev->CreateBuffer(&bd,0,&DXBuffer));  
}

sGeoBuffer11::~sGeoBuffer11()
{
  sDXRELEASE(DXBuffer);
}

/****************************************************************************/

sGeoBufferManager::sGeoBufferManager()
{
  BlockSize = 1024*1024*2;        // 32bytes*64kvert
  Current = 0;
}

sGeoBufferManager::~sGeoBufferManager()
{
  if(!sRELEASE)
  {
    sGeoBuffer11 *gb;
    sLog(L"gfx",L"index/vertex buffer resources\n");
    sFORALL_LIST(Free,gb)
      sLogF(L"gfx",L"%k bytes\n",gb->Alloc);
    sFORALL_LIST(Used,gb)
      sLogF(L"gfx",L"%k bytes\n",gb->Alloc);
    if(Current)
      sLogF(L"gfx",L"%k bytes\n",Current->Alloc);
  }

  sDeleteAll(Free);
  sDeleteAll(Used);
  sDelete(Current);
}

void sGeoBufferManager::Flush()
{
  if(Current)
  {
    Used.AddTail(Current);
    Current = 0;
  }
  sGeoBuffer11 *gb;
  while(!Used.IsEmpty())
  {
    gb = Used.RemTail();
    D3D11_MAPPED_SUBRESOURCE mr;
    DXErr(GTC->DXCtx->Map(gb->DXBuffer,0,D3D11_MAP_WRITE_DISCARD,0,&mr));
    GTC->DXCtx->Unmap(gb->DXBuffer,0);
    gb->Used = 0;
    gb->Free = gb->Alloc;
    Free.AddHead(gb);
  }
}

/****************************************************************************/

void sGeoBufferManager::Map(sGeoMapHandle &map,sDInt bytes)
{
  sGeoBuffer11 *gb = 0;

  // the buffer we want is unusually large

  if(bytes>=BlockSize/2 || (Current && Current->Free<bytes))
  {
    // what is the best fit?

    sGeoBuffer11 *bestb=0;
    sInt bests = 0x7fffffff;
    sFORALL_LIST(Free,gb)
    {
      if(gb->Free>=bytes)
      {
        sInt error = gb->Free-bytes;
        if(error<bests)
        {
          bests = error;
          bestb = gb;
        }
      }
    }

    if(Current)
      Used.AddTail(Current);

    if(bestb)
    {
      Free.Rem(bestb);
      Current = bestb;
    }
    else
    {
      // no fit. create new one

      Current = new sGeoBuffer11(bytes);
    }
  }
  else  // ordinary size:
  {
    // do we need a new current block?

    while(Current==0 || Current->Free<bytes)
    {
      if(Current)
        Used.AddTail(Current);
      if(Free.IsEmpty())
        Free.AddTail(new sGeoBuffer11(BlockSize));
      Current = Free.RemTail();
    }
  }
  // use current block!

  gb = Current;

  sVERIFY(gb && gb->Free>=bytes);

  // now map

  if(gb->MappedCount==0)
  {
    D3D11_MAPPED_SUBRESOURCE mr;
    DXErr(GTC->DXCtx->Map(gb->DXBuffer,0,D3D11_MAP_WRITE_NO_OVERWRITE,0,&mr));
    gb->MapPtr = (sU8 *)mr.pData;
  }
  map.Ptr = gb->MapPtr+gb->Used;
  map.Buffer = gb;
  map.Offset = gb->Used;
  sVERIFY(gb->Used<=gb->Alloc);
  gb->Used += bytes;
  gb->Free = gb->Alloc-gb->Used;  
  gb->MappedCount++;
}

void sGeoBufferManager::Unmap(sGeoMapHandle &hnd)
{
  hnd.Buffer->MappedCount--;
  if(hnd.Buffer->MappedCount==0)
  {
    GTC->DXCtx->Unmap(hnd.Buffer->DXBuffer,0);
    hnd.Buffer->MapPtr = 0;
  }
}

/****************************************************************************/
/****************************************************************************/

void sGeometry::InitPrivate()
{
  sClear(VB);
  sClear(IB);
}

void sGeometry::ExitPrivate()
{
  sClear(VB);
  sClear(IB);
}

void sGeometry::Clear()
{
  for(sInt i=0;i<sVF_STREAMMAX;i++)
    sDXRELEASE(VB[i].DXBuffer);
  sDXRELEASE(IB.DXBuffer);
  sClear(VB);
  sClear(IB);

  Format = 0;
  Flags = 0;
  IndexSize = 0;
  DebugBreak = 0;

  DXIndexFormat = DXGI_FORMAT_UNKNOWN;

  PrimMode = 0;
  FirstPrim = 0;
  LastPrimPtr = 0;
  CurrentPrim = 0;
}

// initialization and drawing

void sGeometry::Init(sInt flags,sVertexFormatHandle *vf)
{
  Flags = flags;
  Format = vf;
  if(flags & sGF_INDEX32)
  {
    IndexSize = 4;
    DXIndexFormat = DXGI_FORMAT_R32_UINT;
  }
  if(flags & sGF_INDEX16)
  {
    IndexSize = 2;
    DXIndexFormat = DXGI_FORMAT_R16_UINT;
  }
  switch(flags & sGF_PRIMMASK)
  {
  case sGF_TRILIST:     Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
  case sGF_TRISTRIP:    Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; break; 
  case sGF_LINELIST:    Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST; break;
  case sGF_LINESTRIP:   Topology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP; break;
  case sGF_LINELISTADJ: Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST_ADJ; break;
  case sGF_TRILISTADJ:  Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ; break;
  case sGF_QUADLIST:    Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
  case sGF_PATCHLIST:   Topology = D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST
                                   - 1 + (flags&sGF_PATCHMASK); break;
  default: sVERIFYFALSE;
  }
}

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
  ID3D11Buffer *buffer[sVF_STREAMMAX];
  UINT byteoffset[sVF_STREAMMAX];
  UINT strides[sVF_STREAMMAX];

  const sDrawRange *ir = di.Ranges;
  sInt irc = di.RangeCount;
  sInt instancecount = di.InstanceCount;

  ID3D11DeviceContext *DXCtx = GTC->DXCtx;

  for(sInt i=0;i<sVF_STREAMMAX;i++)
  {
    buffer[i] = VB[i].GetBuffer();
    strides[i] = VB[i].ElementSize;
    if(di.VertexOffset[0])
      byteoffset[i] = di.VertexOffset[i]*VB[i].ElementSize;
    else
      byteoffset[i] = 0;
    byteoffset[i] += UINT(VB[i].GetOffset());
  }

  DXCtx->IASetVertexBuffers(0,sVF_STREAMMAX,buffer,strides,byteoffset);
  DXCtx->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)Topology);
  DXCtx->IASetInputLayout(Format->Layout);


  if((Flags & sGF_PRIMMASK)==sGF_QUADLIST)
  {
    sVERIFY(IndexSize==0);
    sDrawRange dr;
    if(ir==0)
    {
      ir = &dr;
      dr.Start = 0;
      dr.End = VB[0].ElementCount;
      irc = 1;
    }
    for(sInt i=0;i<irc;i++)
    {
      sInt start = ir[i].Start;
      sInt end = ir[i].End;
      sVERIFY(di.Indirect==0);
      while(start<end)
      {
        sInt batch = sMin(end-start,0x3ffc);

        DXCtx->IASetIndexBuffer(DXQuadList,DXGI_FORMAT_R16_UINT,0);
        DXCtx->DrawIndexed(batch/4*6,0,start);

        start += batch;
#if STATS
        Stats.Splitter++;
        Stats.Vertices += batch;
        Stats.Indices += batch/4*6;
        Stats.Primitives += batch/4*2;
#endif
      }
    }
  }
  else
  {
#if STATS
    sInt _vc = 0;
    sInt _ic = 0;
#endif
    if(IndexSize)
    {
      DXCtx->IASetIndexBuffer(IB.GetBuffer(),(DXGI_FORMAT) DXIndexFormat,IB.GetOffset());
      sDrawRange dr;
      if(ir==0)
      {
        ir = &dr;
        dr.Start = 0;
        dr.End = IB.ElementCount;
        irc = 1;
      }
      for(sInt i=0;i<irc;i++)
      {
        if(di.Indirect)
        {
          DXCtx->DrawIndexedInstancedIndirect(di.Indirect->DXBuffer,0);
        }
        else
        {
          if(instancecount)
            DXCtx->DrawIndexedInstanced(ir[i].End-ir[i].Start,instancecount,ir[i].Start,0,0);
          else
            DXCtx->DrawIndexed(ir[i].End-ir[i].Start,ir[i].Start,0);
        }
#if STATS
        _ic += ir[i].End-ir[i].Start;
#endif
      }
#if STATS
      _vc += VB[0].ElementCount;
#endif
    }
    else
    {
      DXCtx->IASetIndexBuffer(0,DXGI_FORMAT_UNKNOWN,0);
      sDrawRange dr;
      if(ir==0)
      {
        ir = &dr;
        dr.Start = 0;
        dr.End = VB[0].ElementCount;
        irc = 1;
      }

      for(sInt i=0;i<irc;i++)
      {
        if(di.Indirect)
        {
          DXCtx->DrawInstancedIndirect(di.Indirect->DXBuffer,0);
        }
        else
        {
          if(instancecount)
            DXCtx->DrawInstanced(ir[i].End-ir[i].Start,instancecount,ir[i].Start,0);
          else
            DXCtx->Draw(ir[i].End-ir[i].Start,ir[i].Start);
        }
#if STATS
        _vc += ir[i].End-ir[i].Start;
        _ic += ir[i].End-ir[i].Start;
#endif
      }
    }
#if STATS
    sInt infa = sMax(instancecount,1);
    Stats.Splitter += irc;
    Stats.Vertices += _vc*infa;
    Stats.Indices += _ic*infa;
    sInt ic = _ic*infa;
    switch(Flags & sGF_PRIMMASK)
    {
    case sGF_TRILIST:     Stats.Primitives += (ic/3); break;
    case sGF_TRISTRIP:    Stats.Primitives += (ic-2); break;
    case sGF_LINELIST:    Stats.Primitives += (ic/2); break;
    case sGF_LINESTRIP:   Stats.Primitives += (ic-1); break;
    case sGF_LINELISTADJ: Stats.Primitives += (ic/4); break;
    case sGF_TRILISTADJ:  Stats.Primitives += (ic/6); break;
    case sGF_PATCHLIST:   Stats.Primitives += ic/(Flags & sGF_PATCHMASK); break;
    }
#endif
  }
#if STATS
  Stats.Batches++;
#endif
}

// buffer loading

void sGeometry::BeginLoadIB(sInt ic,sGeometryDuration duration,void **ip)
{
  IB.ElementCount = ic;
  IB.ElementSize = IndexSize;
  BeginLoadPrv(&IB,duration,ip,D3D11_BIND_INDEX_BUFFER);
}

void sGeometry::BeginLoadVB(sInt vc,sGeometryDuration duration,void **vp,sInt stream)
{
  VB[stream].ElementCount = vc;
  VB[stream].ElementSize = Format->GetSize(stream);
  BeginLoadPrv(&VB[stream],duration,vp,D3D11_BIND_VERTEX_BUFFER);
}


void sGeometry::EndLoadIB(sInt ic)
{
  EndLoadPrv(&IB,ic,D3D11_BIND_INDEX_BUFFER);
}

void sGeometry::EndLoadVB(sInt vc,sInt stream)
{
  EndLoadPrv(&VB[stream],vc,D3D11_BIND_VERTEX_BUFFER);
}

void sGeometryPrivate::BeginLoadPrv(Buffer *b,sInt duration,void **ptr,sInt bindflag)
{
  sClear(b->DynMap);
  sDXRELEASE(b->DXBuffer);
  if(duration==sGD_STATIC)
  {

    b->LoadBuffer = sAllocMem(b->ElementCount*b->ElementSize,64,0);
    *ptr = b->LoadBuffer;
  }
  else
  {
    b->LoadBuffer = 0;
    GeoBufferManager->Map(b->DynMap,b->ElementCount*b->ElementSize);
    *ptr = b->DynMap.Ptr;
  }
}

void sGeometryPrivate::EndLoadPrv(Buffer *b,sInt count,sInt bindflag)
{
  if(b->LoadBuffer)
  {
    if(count!=-1)
      b->ElementCount = count;
    if(b->ElementCount>0)
    {
      D3D11_BUFFER_DESC bd;
      sClear(bd);
      bd.ByteWidth = b->ElementCount*b->ElementSize;
      bd.Usage = D3D11_USAGE_IMMUTABLE;
      bd.BindFlags = bindflag;

      D3D11_SUBRESOURCE_DATA sd;
      sClear(sd);
      sd.pSysMem = b->LoadBuffer;

      DXErr(DXDev->CreateBuffer(&bd,&sd,&b->DXBuffer));
      sDelete(b->LoadBuffer);
    }
  }
  else
  {
    if(count!=-1)
      b->ElementCount = count;
    sVERIFY(b->DynMap.Buffer);
    GeoBufferManager->Unmap(b->DynMap);
  }
}

// helper

void sGeometry::BeginLoad(sVertexFormatHandle *vf,sInt flags,sGeometryDuration duration,sInt vc,sInt ic,void **vp,void **ip)
{
  Init(flags,vf);
  BeginLoadIB(ic,duration,ip);
  BeginLoadVB(vc,duration,vp);
}

void sGeometry::EndLoad(sInt vc,sInt ic)
{
  EndLoadVB(vc);
  EndLoadIB(ic);
}

// vertex stream merging


void sGeometry::Merge(sGeometry *geo0,sGeometry *geo1)
{
  sVERIFY(geo1!=this);
  if(geo0 != this)
  {
    for(sInt i=0;i<sVF_STREAMMAX;i++)
      VB[i].CloneFrom(&geo0->VB[i]);
    IB.CloneFrom(&geo0->IB);
  }

  if(geo1)
  {
    for(sInt i=0;i<sVF_STREAMMAX;i++)
      if(!geo1->VB[i].IsEmpty())
        VB[i].CloneFrom(&geo1->VB[i]);
    if(!geo1->IB.IsEmpty())
      IB.CloneFrom(&geo1->IB);
  }
}

void sGeometry::MergeVertexStream(sInt DestStream,sGeometry *src,sInt SrcStream)
{
  sVERIFY(src!=this);
  VB[DestStream].CloneFrom(&src->VB[SrcStream]);
}

void sGeometryPrivate::Buffer::CloneFrom(sGeometryPrivate::Buffer *s)
{ 
  ElementCount=s->ElementCount; 
  ElementSize=s->ElementSize;
  DynMap = s->DynMap;
  LoadBuffer = 0;
  sDXRELEASE(DXBuffer);
  DXBuffer = s->DXBuffer;
  if(DXBuffer) DXBuffer->AddRef();
}

/****************************************************************************/
/***                                                                      ***/
/***   Dynamic Management                                                 ***/
/***                                                                      ***/
/****************************************************************************/

void sGeometry::InitDyn(sInt ic,sInt vc0,sInt vc1,sInt vc2,sInt vc3)
{
  sInt vc[4];
  vc[0] = vc0;
  vc[1] = vc1;
  vc[2] = vc2;
  vc[3] = vc3;
  D3D11_BUFFER_DESC bd;
  sClear(bd);

  bd.Usage = D3D11_USAGE_DYNAMIC;
  bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
  bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

  if(ic>0)
  {
    IB.ElementSize = IndexSize;
    IB.ElementCount = ic;
    bd.ByteWidth = ic*IndexSize;
    DXErr(DXDev->CreateBuffer(&bd,0,&IB.DXBuffer));  
  }

  bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  for(sInt i=0;i<4;i++)
  {
    if(vc[i]>0)
    {
      VB[i].ElementSize = Format->GetSize(i);
      VB[i].ElementCount = vc[i];
      bd.ByteWidth = vc[i]*Format->GetSize(i);
      DXErr(DXDev->CreateBuffer(&bd,0,&VB[i].DXBuffer));      
    }
  }
}

void *sGeometry::BeginDynVB(sBool discard,sInt stream)
{
  D3D11_MAPPED_SUBRESOURCE mr;
  DXErr(GTC->DXCtx->Map(VB[stream].DXBuffer,0,discard ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE,0,&mr));
  return mr.pData;
}

void *sGeometry::BeginDynIB(sBool discard)
{
  D3D11_MAPPED_SUBRESOURCE mr;
  DXErr(GTC->DXCtx->Map(IB.DXBuffer,0,discard ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE,0,&mr));
  return mr.pData;
}

void sGeometry::EndDynVB(sInt stream)
{
  GTC->DXCtx->Unmap(VB[stream].DXBuffer,0);
}

void sGeometry::EndDynIB()
{
  GTC->DXCtx->Unmap(IB.DXBuffer,0);
}


/****************************************************************************/
/***                                                                      ***/
/***   sTextureBase                                                       ***/
/***                                                                      ***/
/****************************************************************************/

void sCopyCubeFace(sTexture2D *dest, sTextureCube *src, sTexCubeFace cf)
{
}

sBool sReadTexture(sReader &s, sTextureBase *&tex)
{
  return sFALSE;
}

/****************************************************************************/

void ConvertFlags(sU32 flags,sInt &mm,D3D11_BIND_FLAG &bind,D3D11_USAGE &usage,DXGI_FORMAT &fmt_res,DXGI_FORMAT &fmt_tex,DXGI_FORMAT &fmt_view,sBool &ds)
{
  ds = 0;
  fmt_tex = DXGI_FORMAT_UNKNOWN;
  fmt_view = DXGI_FORMAT_UNKNOWN;
  fmt_res = DXGI_FORMAT_UNKNOWN;
  switch(flags&sTEX_FORMAT)
  {
    case sTEX_ARGB8888: fmt_res = DXGI_FORMAT_B8G8R8A8_UNORM;    break;
    case sTEX_QWVU8888: fmt_res = DXGI_FORMAT_R8G8B8A8_SNORM;    break;
    
    case sTEX_GR16:     fmt_res = DXGI_FORMAT_R16G16_UNORM;      break;
    case sTEX_ARGB16:   fmt_res = DXGI_FORMAT_R16G16B16A16_UNORM;break;

    case sTEX_R32F:     fmt_res = DXGI_FORMAT_R32_FLOAT;         break;
    case sTEX_GR32F:    fmt_res = DXGI_FORMAT_R32G32_FLOAT;      break;
    case sTEX_ARGB32F:  fmt_res = DXGI_FORMAT_R32G32B32A32_FLOAT;break;
    case sTEX_R16F:     fmt_res = DXGI_FORMAT_R16_FLOAT;         break;
    case sTEX_GR16F:    fmt_res = DXGI_FORMAT_R16G16_FLOAT;      break;
    case sTEX_ARGB16F:  fmt_res = DXGI_FORMAT_R16G16B16A16_FLOAT;break;

    case sTEX_A8:       fmt_res = DXGI_FORMAT_A8_UNORM;          break;
    case sTEX_I8:       fmt_res = DXGI_FORMAT_R8_UNORM;          break;
    case sTEX_DXT1:     fmt_res = DXGI_FORMAT_BC1_UNORM;         break;
    case sTEX_DXT1A:    fmt_res = DXGI_FORMAT_BC1_UNORM;         break;
    case sTEX_DXT3:     fmt_res = DXGI_FORMAT_BC2_UNORM;         break;
    case sTEX_DXT5:     fmt_res = DXGI_FORMAT_BC3_UNORM;         break;
    case sTEX_DXT5N:    fmt_res = DXGI_FORMAT_BC3_UNORM;         break;
    case sTEX_ARGB1555: fmt_res = DXGI_FORMAT_B5G5R5A1_UNORM;    break;
    case sTEX_RGB565:   fmt_res = DXGI_FORMAT_B5G6R5_UNORM;      break;
    
    case sTEX_MRGB8:    fmt_res = DXGI_FORMAT_B8G8R8A8_UNORM;    break;
    case sTEX_8TOIA:    fmt_res = DXGI_FORMAT_R8_UNORM;          break;

    case sTEX_PCF24:
    case sTEX_DEPTH24NOREAD:
    case sTEX_DEPTH24:  fmt_res = DXGI_FORMAT_R24G8_TYPELESS;
                        fmt_tex = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
                        fmt_view= DXGI_FORMAT_D24_UNORM_S8_UINT;
                        ds=1;  break;
    case sTEX_PCF16:
    case sTEX_DEPTH16NOREAD:
    case sTEX_DEPTH16:  fmt_res = DXGI_FORMAT_R16_TYPELESS;
                        fmt_tex = DXGI_FORMAT_R16_UNORM;
                        fmt_view= DXGI_FORMAT_D16_UNORM;
                        ds=1;  break;

    case sTEX_STRUCTURED: fmt_res = DXGI_FORMAT_UNKNOWN;         break;
    case sTEX_RAW:        fmt_res = DXGI_FORMAT_R32_UINT;        break;
    case sTEX_UINT32:     fmt_res = DXGI_FORMAT_R32_UINT;        break;

    default: 
      sLogF(L"gfx",L"invalid format %d\n",flags&sTEX_FORMAT);
      sVERIFYFALSE;
  }
  if(fmt_tex ==DXGI_FORMAT_UNKNOWN) fmt_tex  = fmt_res;
  if(fmt_view==DXGI_FORMAT_UNKNOWN) fmt_view = fmt_res;

  bind = D3D11_BIND_SHADER_RESOURCE;
  usage = D3D11_USAGE_DEFAULT;
  if(flags & sTEX_RENDERTARGET)
  {
    sVERIFY(!(flags & sTEX_DYNAMIC));
    sVERIFY((flags & sTEX_NOMIPMAPS) || (flags & sTEX_AUTOMIPMAP) || mm != 0); 
    if(ds)
      bind = D3D11_BIND_FLAG(bind | D3D11_BIND_DEPTH_STENCIL);
    else
      bind = D3D11_BIND_FLAG(bind | D3D11_BIND_RENDER_TARGET);
  }
  if(flags & sTEX_DYNAMIC)
  {
    usage = D3D11_USAGE_DYNAMIC;
    mm = 1;
  }
  if(flags & sTEX_CS_WRITE)
    bind = D3D11_BIND_FLAG(bind | D3D11_BIND_UNORDERED_ACCESS);
  if(flags & sTEX_CS_VERTEX)
    bind = D3D11_BIND_FLAG(bind | D3D11_BIND_VERTEX_BUFFER);
  if(flags & sTEX_CS_INDEX)
    bind = D3D11_BIND_FLAG(bind | D3D11_BIND_INDEX_BUFFER);
}

/****************************************************************************/

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
  //  (1ULL<<sTEX_I8) |
  //  (1ULL<<sTEX_IA4) |
    (1ULL<<sTEX_DXT1) |
    (1ULL<<sTEX_DXT1A) |
    (1ULL<<sTEX_DXT3) |
    (1ULL<<sTEX_DXT5) |
    (1ULL<<sTEX_DXT5N) |
    (1ULL<<sTEX_ARGB1555) |
  //  (1ULL<<sTEX_ARGB4444) |
    (1ULL<<sTEX_RGB565) |
    0;  
}

/****************************************************************************/

void sPackDXT(sU8 *d,sU32 *bmp,sInt xs,sInt ys,sInt format,sBool dither)
{
//  sInt formatflags = format;
  format &= sTEX_FORMAT;


  sFastPackDXT(d,bmp,xs,ys,format,1 | (dither ? 0x80 : 0));
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
    D3D11_QUERY_DESC qd;
    qd.Query = D3D11_QUERY_OCCLUSION;
    qd.MiscFlags = 0;
    DXErr(DXDev->CreateQuery(&qd,&qn->Query));
    AllOccQueryNodes->AddTail(qn);
    return qn;
  }
  else
  {
    return FreeOccQueryNodes->RemTail();
  }
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
  GTC->DXCtx->Begin(Current->Query);
  Queries.AddTail(Current);
}

void sOccQuery::End()
{
  sVERIFY(Current);
  GTC->DXCtx->End(Current->Query);
  Current = 0;
}

void sOccQuery::Poll()
{
  sU64 pixels;
  sOccQueryNode *qn = Queries.GetHead();
  while(!Queries.IsEnd(qn))
  {
    sOccQueryNode *next = Queries.GetNext(qn);
    if(GTC->DXCtx->GetData(qn->Query,&pixels,sizeof(pixels),0)==S_OK)
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

void sTexture2D::Create2(sInt flags)
{
  LoadPtr = 0;
  Dynamic = 0;
  DXTex2D = 0;
  LoadMipmap = -1;
  static sInt MultisampleHack = 0;

  // handle stream textures as dynamic textures
  if(Flags&sTEX_STREAM)
  {
    Flags &= ~sTEX_STREAM;
    Flags |= sTEX_DYNAMIC;
    flags &= ~sTEX_STREAM;
    flags |= sTEX_DYNAMIC;
  }

  // oh, we need multisampling...

  if((Flags & sTEX_MSAA) && DXScreenMode.MultiLevel>=0)
  {
    MultisampleHack = 1;
    MultiSampled = new sTexture2D(SizeX,SizeY,flags&~(sTEX_MSAA|sTEX_INTERNAL),Mipmaps);
    MultisampleHack = 0;
    Mipmaps = 1;
  }

  // what are we dealing with?

  D3D11_BIND_FLAG bind;
  D3D11_USAGE usage;
  DXGI_FORMAT fmt_res,fmt_tex,fmt_view;
  sBool ds;

  ConvertFlags(flags,Mipmaps,bind,usage,fmt_res,fmt_tex,fmt_view,ds);
  if(usage==D3D11_USAGE_DYNAMIC)
    Dynamic = 1;
  DXFormat = fmt_res;

  // create it

  D3D11_TEXTURE2D_DESC td;
  sClear(td);
  td.Width = SizeX;
  td.Height = SizeY;
  td.MipLevels = Mipmaps;
  td.ArraySize = 1;
  td.Format = fmt_res;
  td.SampleDesc.Count = 1;
  td.Usage = usage;
  td.BindFlags = bind;
  td.CPUAccessFlags = Dynamic?D3D11_CPU_ACCESS_WRITE:0;
  if(Flags & sTEX_AUTOMIPMAP)
    td.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

  if(MultisampleHack)
  {
    sInt ms = sMin(DXMSAALevels->GetCount()-1,DXScreenMode.MultiLevel);
    while(ms>=0)
    {
      UINT n;
      DXErr(DXDev->CheckMultisampleQualityLevels(fmt_res,(*DXMSAALevels)[ms].Count,&n));
      if((*DXMSAALevels)[ms].Quality<n)
        break;
      ms--;
    }
    if(ms>=0)
    {
      td.SampleDesc = (*DXMSAALevels)[ms];
    }
    if(ds)
    {
      bind = (D3D11_BIND_FLAG)(bind &~D3D11_BIND_SHADER_RESOURCE);
      td.BindFlags = bind;
    }
  }

  if(!(Flags & sTEX_INTERNAL))
  {
    DXErr(DXDev->CreateTexture2D(&td,0,&DXTex2D));
    
    if(bind & D3D11_BIND_SHADER_RESOURCE)
    {
      D3D11_SHADER_RESOURCE_VIEW_DESC vd;
      vd.Format = fmt_tex;
      if(MultisampleHack)
      {
        vd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
      }
      else
      {
        vd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        vd.Texture2D.MostDetailedMip = 0;
        vd.Texture2D.MipLevels = Mipmaps;
      }
      DXErr(DXDev->CreateShaderResourceView(DXTex2D,&vd,&DXTexView));
    }

    if(bind & D3D11_BIND_UNORDERED_ACCESS)
    {
      D3D11_UNORDERED_ACCESS_VIEW_DESC vd;
      vd.Format = fmt_tex;
      vd.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
      vd.Texture2D.MipSlice = 0;

      DXErr(DXDev->CreateUnorderedAccessView(DXTex2D,&vd,&DXTexUAV));
    }

    if(Flags & sTEX_RENDERTARGET)
    {
      if(ds)
      {
        D3D11_DEPTH_STENCIL_VIEW_DESC dd;
        sClear(dd);
        dd.Format = fmt_view;
        if(MultisampleHack)
        {
          dd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        }
        else
        {
          dd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
          dd.Flags = 0;
          dd.Texture2D.MipSlice = 0;
        }
        DXErr(DXDev->CreateDepthStencilView(DXTex2D,&dd,&DXDepthView));
      }
      else
      {
        D3D11_RENDER_TARGET_VIEW_DESC rd;
        sClear(rd);
        rd.Format = fmt_view;
        if(MultisampleHack)
        {
          rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
        }
        else
        {
          rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
          rd.Texture2D.MipSlice = 0;
        }
        DXErr(DXDev->CreateRenderTargetView(DXTex2D,&rd,&DXRenderView));
      }
    }
  }

  if(Mipmaps==0) Mipmaps=1;   // for autogenmipmap: use 0 as argument for CreateTexture, but 1 is the correct value for later because we have one accessible mipmap.
}

void sTexture2D::Destroy2()
{
  delete MultiSampled;
  sDXRELEASE(DXTex2D);
  sDXRELEASE(DXTexView);
  sDXRELEASE(DXTexUAV);
  sDXRELEASE(DXRenderView);
  sDXRELEASE(DXDepthView);
  sDXRELEASE(DXReadTex);
  sVERIFY(!LoadPtr); // forgot EndLoad()
}

void sTexture2D::BeginLoad(sU8 *&data,sInt &pitch,sInt mipmap)
{
  sVERIFY(LoadPtr==0);
  sVERIFY(LoadMipmap==-1);

  LoadMipmap = mipmap;

  if (Dynamic)
  {
    D3D11_MAPPED_SUBRESOURCE subres;
    GTC->DXCtx->Map(DXTex2D,LoadMipmap,D3D11_MAP_WRITE_DISCARD,0,&subres);
    data=(sU8*)subres.pData;
    pitch=subres.RowPitch;
  }
  else
  {
    sDInt size = (sU64(SizeX)*SizeY*BitsPerPixel)>>(3+2*mipmap);
    data = LoadPtr = new sU8[size];
    pitch = (SizeX*BitsPerPixel)>>(3+mipmap);
    if(sIsBlockCompression(Flags))
      pitch *= 4;
  }
}

void sTexture2D::BeginLoadPartial(const sRect &rect,sU8 *&data,sInt &pitch,sInt mipmap)
{
  sVERIFYFALSE;
}

void sTexture2D::EndLoad()
{
  sVERIFY(LoadMipmap>=0)

  if (Dynamic)
  {
    GTC->DXCtx->Unmap(DXTex2D,LoadMipmap);
  }
  else
  {
    sVERIFY(LoadPtr);

    sInt pitch = (SizeX*BitsPerPixel)>>(3+LoadMipmap);
    if(sIsBlockCompression(Flags))
      pitch *= 4;
    
    GTC->DXCtx->UpdateSubresource(DXTex2D,LoadMipmap,0,LoadPtr,pitch,0);
    sDelete(LoadPtr);
  }

  LoadMipmap = -1;
}

void *sTexture2D::BeginLoadPalette()
{
  sVERIFYFALSE;
  return 0;
}

void sTexture2D::EndLoadPalette()
{
}

void sTexture2D::CalcOneMiplevel(const sRect &rect)
{
  sVERIFYFALSE;
}

/****************************************************************************/
/***                                                                      ***/
/***   sTextureCube                                                       ***/
/***                                                                      ***/
/****************************************************************************/

void sTextureCube::Create2(sInt flags)
{
  LoadPtr = 0;
  Dynamic = 0;
  DXTex2D = 0;
  LoadMipmap = -1;

  // handle stream textures as dynamic textures
  if(Flags&sTEX_STREAM)
  {
    Flags &= ~sTEX_STREAM;
    Flags |= sTEX_DYNAMIC;
    flags &= ~sTEX_STREAM;
    flags |= sTEX_DYNAMIC;
  }

  // what are we dealing with?

  D3D11_BIND_FLAG bind;
  D3D11_USAGE usage;
  DXGI_FORMAT fmt_res,fmt_tex,fmt_view;
  sBool ds;

  ConvertFlags(flags,Mipmaps,bind,usage,fmt_res,fmt_tex,fmt_view,ds);
  if(usage==D3D11_USAGE_DYNAMIC)
    Dynamic = 1;
  DXFormat = fmt_res;
  sVERIFY(!ds);

  // create it

  D3D11_TEXTURE2D_DESC td;
  sClear(td);
  td.Width = SizeX;
  td.Height = SizeY;
  td.MipLevels = Mipmaps;
  td.ArraySize = 6;
  td.Format = fmt_res;
  td.SampleDesc.Count = 1;
  td.Usage = usage;
  td.BindFlags = bind;
  td.CPUAccessFlags = 0;
  td.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
  
  DXErr(DXDev->CreateTexture2D(&td,0,&DXTex2D));
  
  if(bind & D3D11_BIND_SHADER_RESOURCE)
  {
    D3D11_SHADER_RESOURCE_VIEW_DESC vd;
    vd.Format = fmt_tex;
    vd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    vd.Texture2DArray.MostDetailedMip = 0;
    vd.Texture2DArray.MipLevels = Mipmaps;
    DXErr(DXDev->CreateShaderResourceView(DXTex2D,&vd,&DXTexView));
  }

  if(Flags & sTEX_RENDERTARGET)
  {
    D3D11_RENDER_TARGET_VIEW_DESC rd;
    for(sInt i=0;i<6;i++)
    {
      sClear(rd);
      rd.Format = fmt_view;
      rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
      rd.Texture2DArray.MipSlice = 0;
      rd.Texture2DArray.FirstArraySlice = i;
      rd.Texture2DArray.ArraySize = 1;
      DXErr(DXDev->CreateRenderTargetView(DXTex2D,&rd,&DXCubeRenderView[i]));
    }
  }

  if(Mipmaps==0) Mipmaps=1;   // for autogenmipmap: use 0 as argument for CreateTexture, but 1 is the correct value for later because we have one accessible mipmap.
}

void sTextureCube::Destroy2()
{
  sDXRELEASE(DXTex2D);
  sDXRELEASE(DXTexView);
  for(sInt i=0;i<6;i++)
    sDXRELEASE(DXCubeRenderView[i]);
  sDXRELEASE(DXDepthView);
  sVERIFY(!LoadPtr); // forgot EndLoad()
}

void sTextureCube::BeginLoad(sTexCubeFace cf, sU8*& data, sInt& pitch, sInt mipmap/*=0*/)
{
  sVERIFY(LoadPtr==0);
  sVERIFY(LoadMipmap==-1);

  LoadMipmap = mipmap;
  LoadCubeFace = cf;
  sDInt size = (sU64(SizeXY)*SizeXY*BitsPerPixel)>>(3+2*mipmap);
  data = LoadPtr = new sU8[size];
  pitch = (SizeXY*BitsPerPixel)>>(3+mipmap);
}

void sTextureCube::EndLoad()
{
  sVERIFY(LoadPtr);
  sVERIFY(LoadMipmap>=0)

  GTC->DXCtx->UpdateSubresource(DXTex2D,D3D11CalcSubresource(LoadMipmap,LoadCubeFace,Mipmaps),0,LoadPtr,(SizeXY*BitsPerPixel)>>(3+LoadMipmap),0);
  LoadMipmap = -1;
  LoadCubeFace = -1;
  sDelete(LoadPtr);
}

/****************************************************************************/
/***                                                                      ***/
/***   sTexture3D                                                         ***/
/***                                                                      ***/
/****************************************************************************/

sTexture3D::sTexture3D(sInt xs, sInt ys, sInt zs, sU32 flags)
{
  SizeX = xs;
  SizeY = ys;
  SizeZ = zs;
  Flags = (flags&~sTEX_TYPE_MASK)|sTEX_3D;
  Mipmaps = 0;
  BitsPerPixel = sGetBitsPerPixel(flags);

  LoadPtr = 0;
  Dynamic = 0;
  DXTex2D = 0;
  LoadMipmap = -1;

  while(xs>=1 && ys>=1 && zs>=1)
  {
    xs = xs/2;
    ys = ys/2;
    zs = zs/2;
    Mipmaps++;
  }
  if(flags & sTEX_NOMIPMAPS)
    Mipmaps = 1;

  // handle stream textures as dynamic textures
  if(Flags&sTEX_STREAM)
  {
    Flags &= ~sTEX_STREAM;
    Flags |= sTEX_DYNAMIC;
    flags &= ~sTEX_STREAM;
    flags |= sTEX_DYNAMIC;
  }

  // what are we dealing with?

  D3D11_BIND_FLAG bind;
  D3D11_USAGE usage;
  DXGI_FORMAT fmt_res,fmt_tex,fmt_view;
  sBool ds;

  ConvertFlags(flags,Mipmaps,bind,usage,fmt_res,fmt_tex,fmt_view,ds);
  if(usage==D3D11_USAGE_DYNAMIC)
    Dynamic = 1;
  DXFormat = fmt_res;
  sVERIFY(!ds);

  // some checks

  if(Mipmaps==0) Mipmaps=1;
  sVERIFY(!(Flags & sTEX_RENDERTARGET));

  // create it

  D3D11_TEXTURE3D_DESC td;
  sClear(td);
  td.Width = SizeX;
  td.Height = SizeY;
  td.Depth = SizeZ;
  td.MipLevels = Mipmaps;
  td.Format = fmt_res;
  td.Usage = usage;
  td.BindFlags = bind;
  td.CPUAccessFlags = 0;
  
  DXErr(DXDev->CreateTexture3D(&td,0,&DXTex3D));
  
  if(bind & D3D11_BIND_SHADER_RESOURCE)
  {
    D3D11_SHADER_RESOURCE_VIEW_DESC vd;
    vd.Format = fmt_tex;
    vd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
    vd.Texture3D.MostDetailedMip = 0;
    vd.Texture3D.MipLevels = Mipmaps;
    DXErr(DXDev->CreateShaderResourceView(DXTex3D,&vd,&DXTexView));
  }
}

sTexture3D::~sTexture3D()
{
  sDXRELEASE(DXTex3D);
  sDXRELEASE(DXTexView);
  sVERIFY(!LoadPtr); // forgot EndLoad()
}

void sTexture3D::BeginLoad(sU8*& data, sInt& rpitch, sInt& spitch, sInt mipmap/*=0*/)
{
  sVERIFY(LoadPtr==0);
  sVERIFY(LoadMipmap==-1);

  LoadMipmap = mipmap;
  sDInt size = (sU64(SizeX)*SizeY*SizeZ*BitsPerPixel)>>(3+3*mipmap);
  data = LoadPtr = new sU8[size];
  rpitch = (SizeX*BitsPerPixel)>>(3+mipmap);
  spitch = (SizeX*SizeY*BitsPerPixel)>>(3+2*mipmap);
}

void sTexture3D::EndLoad()
{
  sVERIFY(LoadPtr);
  sVERIFY(LoadMipmap>=0)

  GTC->DXCtx->UpdateSubresource(DXTex3D,LoadMipmap,0,LoadPtr,(SizeX*BitsPerPixel)>>(3+LoadMipmap),(SizeX*SizeY*BitsPerPixel)>>(3+LoadMipmap+LoadMipmap));
  LoadMipmap = -1;
  sDelete(LoadPtr);
}

void sTexture3D::Load(sU8 *data)
{
  sVERIFYFALSE;
}

/****************************************************************************/
/***                                                                      ***/
/***   sTextureProxy                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void sTextureProxy::Connect2()
{
  Disconnect2();

  DXTex2D = Link->DXTex2D;           if(DXTex2D)      DXTex2D->AddRef();
  DXTexView = Link->DXTexView;       if(DXTexView)    DXTexView->AddRef();
  DXRenderView = Link->DXRenderView; if(DXRenderView) DXRenderView->AddRef();
  DXDepthView = Link->DXDepthView;   if(DXDepthView)  DXDepthView->AddRef();
  for(sInt i=0;i<6;i++)
  {
    DXCubeRenderView[i] = Link->DXCubeRenderView[i];
    if(DXCubeRenderView[i])
      DXCubeRenderView[i]->AddRef();
  }
}

void sTextureProxy::Disconnect2()
{
  sRelease(DXTex2D);
  sRelease(DXTexView);
  sRelease(DXRenderView);
  sRelease(DXDepthView);
  for(sInt i=0;i<6;i++)
    sRelease(DXCubeRenderView[i]);
}

/****************************************************************************/
/***                                                                      ***/
/***   sMaterial                                                          ***/
/***                                                                      ***/
/****************************************************************************/

void sMaterial::Create2()
{
  StateVariants = 0;
  Variants = 0;
  
  DomainShader = 0;
  HullShader = 0;
  GeometryShader = 0;
  ComputeShader = 0;
}

void sMaterial::Destroy2()
{
  if(DomainShader) 
    DomainShader->Release();
  if(HullShader) 
    HullShader->Release();
  if(GeometryShader) 
    GeometryShader->Release();
  if(ComputeShader) 
    ComputeShader->Release();

  for(sInt i=0;i<StateVariants;i++)
  {
    sDXRELEASE(Variants[i].BlendState);
    sDXRELEASE(Variants[i].DepthState);
    sDXRELEASE(Variants[i].RasterState);
    for(sInt j=0;j<sMTRL_MAXTEX;j++)
      sDXRELEASE(Variants[i].SamplerStates[j]);
  }
  delete[] Variants;
}


void sMaterial::Prepare(sVertexFormatHandle *format)
{
  sShader *shaders[6];
  
  shaders[0] = VertexShader; VertexShader = 0;
  shaders[1] = HullShader; HullShader = 0;
  shaders[2] = DomainShader; DomainShader = 0;
  shaders[3] = GeometryShader; GeometryShader = 0;
  shaders[4] = PixelShader; PixelShader = 0;
  shaders[5] = ComputeShader; ComputeShader = 0;

  SelectShaders(format);    // before mtrl states to copy textures into Texture[]
                            // otherwise for null textures no states are set

  if(StateVariants==0)
  {
    InitVariants(1);
    SetVariant(0);
  }

  for(sInt i=0;i<6;i++)
    shaders[i]->Release();

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
  SetStates(variant);
  GTC->DXCtx->VSSetShader(VertexShader->vs,0,0);
  GTC->DXCtx->DSSetShader(DomainShader?DomainShader->ds:0,0,0);
  GTC->DXCtx->HSSetShader(HullShader?HullShader->hs:0,0,0);
  GTC->DXCtx->GSSetShader(GeometryShader?GeometryShader->gs:0,0,0);
  GTC->DXCtx->PSSetShader(PixelShader->ps,0,0);
  sSetCBuffers(cbuffers,cbcount);
}

void sMaterial::SetStates(sInt var)
{
  sVERIFY(var>=0 && var<StateVariants);

  // set states

  GTC->DXCtx->OMSetBlendState(Variants[var].BlendState,&Variants[var].BlendFactor.x,0xffffffff);
  GTC->DXCtx->OMSetDepthStencilState(Variants[var].DepthState,Variants[var].StencilRef);
  GTC->DXCtx->RSSetState(Variants[var].RasterState);

  // figure out textures and samplers

  ID3D11ShaderResourceView *tps[sMTRL_MAXPSTEX];
  ID3D11ShaderResourceView *tvs[sMTRL_MAXVSTEX];
  ID3D11SamplerState *sps[sMTRL_MAXPSTEX];
  ID3D11SamplerState *svs[sMTRL_MAXVSTEX];
  sClear(tps);
  sClear(tvs);
  sClear(sps);
  sClear(svs);
  for(sInt i=0;i<sMTRL_MAXTEX;i++)
  {
    if(Texture[i])
    {
      Texture[i]->ResolvePrivate();
      sInt sampler = TBind[i]&sMTB_SAMPLERMASK;
      if(TBind[i]&sMTB_SHADERMASK)
      {
        tvs[sampler] = Texture[i]->DXTexView;
        svs[sampler] = Variants[var].SamplerStates[i];
      }
      else
      {
        tps[sampler] = Texture[i]->DXTexView;
        sps[sampler] = Variants[var].SamplerStates[i];
      }
    }
  }

  // set all texture states

  GTC->DXCtx->PSSetShaderResources(0,sMTRL_MAXPSTEX,tps);
  GTC->DXCtx->VSSetShaderResources(0,sMTRL_MAXVSTEX,tvs);
  GTC->DXCtx->PSSetSamplers(0,sMTRL_MAXPSTEX,sps);
  GTC->DXCtx->VSSetSamplers(0,sMTRL_MAXVSTEX,svs);

#if STATS
  Stats.MtrlChanges ++;
#endif
//  sGFXMtrlIsSet = 1;
}

void sMaterial::InitVariants(sInt max)
{
  sVERIFY(StateVariants == 0);
  StateVariants = max;
  Variants = new StateObjects[max];
  for(sInt i=0;i<max;i++)
    sClear(Variants[i]);
}

void sMaterial::DiscardVariants()
{
  for(sInt i=0;i<StateVariants;i++)
  {
    sDXRELEASE(Variants[i].BlendState);
    sDXRELEASE(Variants[i].DepthState);
    sDXRELEASE(Variants[i].RasterState);
    for(sInt j=0;j<sMTRL_MAXTEX;j++)
      sDXRELEASE(Variants[i].SamplerStates[j]);
  }
  sDeleteArray(Variants);
  StateVariants = 0;
}

void sMaterial::SetVariant(sInt var)
{
  sVERIFY(var>=0 && var<StateVariants);

  // get ready

  D3D11_BLEND_DESC bs;
  D3D11_DEPTH_STENCIL_DESC ds;
  D3D11_RASTERIZER_DESC rs;

  sClear(bs);
  sClear(ds);
  sClear(rs);

  // blend state

  bs.IndependentBlendEnable = 1;
  if(BlendColor!=sMB_OFF)
  {
    bs.RenderTarget[0].BlendEnable = 1;
    bs.RenderTarget[0].SrcBlend  = D3D11_BLEND((BlendColor>> 0)&0xff);
    bs.RenderTarget[0].BlendOp   = D3D11_BLEND_OP((BlendColor>> 8)&0xff);
    bs.RenderTarget[0].DestBlend = D3D11_BLEND((BlendColor>>16)&0xff);
    if(BlendAlpha==sMB_SAMEASCOLOR)
    {
      const static D3D11_BLEND color2alpha[256] = 
      {
        D3D11_BLEND(0),
        D3D11_BLEND_ZERO,
        D3D11_BLEND_ONE,
        D3D11_BLEND_SRC_ALPHA,
        D3D11_BLEND_INV_SRC_ALPHA,
        D3D11_BLEND_SRC_ALPHA,
        D3D11_BLEND_INV_SRC_ALPHA,
        D3D11_BLEND_DEST_ALPHA,
        D3D11_BLEND_INV_DEST_ALPHA,
        D3D11_BLEND_DEST_ALPHA,
        D3D11_BLEND_INV_DEST_ALPHA,
        D3D11_BLEND_SRC_ALPHA_SAT,
        D3D11_BLEND(0),  // 12
        D3D11_BLEND(0),  // 13
        D3D11_BLEND_BLEND_FACTOR,
        D3D11_BLEND_INV_BLEND_FACTOR,
        D3D11_BLEND_SRC1_ALPHA,
        D3D11_BLEND_INV_SRC1_ALPHA,
        D3D11_BLEND_SRC1_ALPHA,
        D3D11_BLEND_INV_SRC1_ALPHA,
      };
      bs.RenderTarget[0].SrcBlendAlpha  = color2alpha[(BlendColor>> 0)&0xff];
      bs.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP((BlendColor>> 8)&0xff);
      bs.RenderTarget[0].DestBlendAlpha = color2alpha[(BlendColor>>16)&0xff];
    }
    else if(BlendAlpha==sMB_OFF)
    {
      bs.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE;
      bs.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
      bs.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    }
    else
    {
      bs.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND((BlendAlpha>> 0)&0xff);
      bs.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP((BlendAlpha>> 8)&0xff);
      bs.RenderTarget[0].DestBlendAlpha = D3D11_BLEND((BlendAlpha>>16)&0xff);
    }
    Variants[var].BlendFactor.InitColor(BlendFactor);
  }
  if(!(Flags & sMTRL_MSK_RED  )) bs.RenderTarget[0].RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_RED;
  if(!(Flags & sMTRL_MSK_GREEN)) bs.RenderTarget[0].RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_GREEN;
  if(!(Flags & sMTRL_MSK_BLUE )) bs.RenderTarget[0].RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_BLUE;
  if(!(Flags & sMTRL_MSK_ALPHA)) bs.RenderTarget[0].RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_ALPHA;

  // depth

  switch(Flags & sMTRL_ZMASK)
  {
  case sMTRL_ZOFF:
    ds.DepthEnable = 0;
    break;
  case sMTRL_ZREAD:
    ds.DepthEnable = 1;
    ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    ds.DepthFunc = D3D11_COMPARISON_FUNC(FuncFlags[0]+D3D11_COMPARISON_NEVER);
    break;
  case sMTRL_ZWRITE:
    ds.DepthEnable = 1;
    ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    ds.DepthFunc = D3D11_COMPARISON_ALWAYS;
    break;
  case sMTRL_ZON:
    ds.DepthEnable = 1;
    ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    ds.DepthFunc = D3D11_COMPARISON_FUNC(FuncFlags[0]+D3D11_COMPARISON_NEVER);
    break;
  }

  if(Flags & (sMTRL_STENCILSS | sMTRL_STENCILDS))
  {
    ds.StencilEnable = 1; 
    ds.StencilReadMask = StencilMask;
    ds.StencilWriteMask = 0xff;
    ds.FrontFace.StencilFunc = D3D11_COMPARISON_FUNC(FuncFlags[sMFT_STENCIL]+D3D11_COMPARISON_NEVER);
    ds.FrontFace.StencilFailOp      = D3D11_STENCIL_OP(StencilOps[sMSI_CWFAIL ]+D3D11_STENCIL_OP_KEEP);
    ds.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP(StencilOps[sMSI_CWZFAIL]+D3D11_STENCIL_OP_KEEP);
    ds.FrontFace.StencilPassOp      = D3D11_STENCIL_OP(StencilOps[sMSI_CWPASS ]+D3D11_STENCIL_OP_KEEP);
    Variants[var].StencilRef = StencilRef;

    if ( Flags & sMTRL_STENCILDS ) 
    {
      ds.BackFace.StencilFunc = D3D11_COMPARISON_FUNC(FuncFlags[sMFT_STENCIL]+D3D11_COMPARISON_NEVER);
      ds.BackFace.StencilFailOp      = D3D11_STENCIL_OP(StencilOps[sMSI_CCWFAIL ]+D3D11_STENCIL_OP_KEEP);
      ds.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP(StencilOps[sMSI_CCWZFAIL]+D3D11_STENCIL_OP_KEEP);
      ds.BackFace.StencilPassOp      = D3D11_STENCIL_OP(StencilOps[sMSI_CCWPASS ]+D3D11_STENCIL_OP_KEEP);
    }
    else
    {
      ds.BackFace = ds.FrontFace;
    }
  }

  // raster

  rs.FillMode = D3D11_FILL_SOLID;
  switch(Flags & sMTRL_CULLMASK)
  {
  case sMTRL_CULLOFF:
    rs.CullMode = D3D11_CULL_NONE;
    break;
  case sMTRL_CULLON:
    rs.CullMode = D3D11_CULL_BACK;
    break;
  case sMTRL_CULLINV:
    rs.CullMode = D3D11_CULL_FRONT;
    break;
  }
  rs.DepthClipEnable = 1;
  rs.ScissorEnable = 1;
  rs.MultisampleEnable = 1;
  rs.AntialiasedLineEnable = 1;

  // set it all. d3d will find equal states, i need not do the caching myself

  DXErr(DXDev->CreateBlendState(&bs,&Variants[var].BlendState));
  DXErr(DXDev->CreateDepthStencilState(&ds,&Variants[var].DepthState));
  DXErr(DXDev->CreateRasterizerState(&rs,&Variants[var].RasterState));

  // and now the samplers

  for(sInt i=0;i<sMTRL_MAXTEX;i++)
  {
    if(Texture[i] || (TFlags[i]&sMTF_EXTERN))
    {
      D3D11_SAMPLER_DESC sd;
      sClear(sd);

      switch(TFlags[i] & sMTF_LEVELMASK)
      {
      case sMTF_LEVEL0: sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT; break;
      case sMTF_LEVEL1: sd.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT; break;
      case sMTF_LEVEL2: sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; break;
      case sMTF_LEVEL3: sd.Filter = D3D11_FILTER_ANISOTROPIC; break;
      }
      if(TFlags[i] & sMTF_PCF)
      {
        sd.ComparisonFunc = D3D11_COMPARISON_LESS;
        sd.Filter = D3D11_FILTER(sd.Filter | 0x80);
      }
      else
      {
        sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
      }

      switch(TFlags[i] & sMTF_ADDRMASK_U)
      {
      case sMTF_TILE_U:   sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP; break;
      case sMTF_CLAMP_U:  sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP; break;
      case sMTF_MIRROR_U: sd.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR; break;
      case sMTF_BORDER_U: sd.AddressU = D3D11_TEXTURE_ADDRESS_BORDER; break;
      }
      switch(TFlags[i] & sMTF_ADDRMASK_V)
      {
      case sMTF_TILE_V:   sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP; break;
      case sMTF_CLAMP_V:  sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP; break;
      case sMTF_MIRROR_V: sd.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR; break;
      case sMTF_BORDER_V: sd.AddressV = D3D11_TEXTURE_ADDRESS_BORDER; break;
      }
      switch(TFlags[i] & sMTF_ADDRMASK_W)
      {
      case sMTF_TILE_W:   sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP; break;
      case sMTF_CLAMP_W:  sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP; break;
      case sMTF_MIRROR_W: sd.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR; break;
      case sMTF_BORDER_W: sd.AddressW = D3D11_TEXTURE_ADDRESS_BORDER; break;
      }

      sd.MipLODBias = LodBias[i];
      sd.MaxAnisotropy = 8;
      sF32 bc = (TFlags[i] & sMTF_BCOLOR_WHITE) ? 1 : 0;
      sd.BorderColor[0] = bc;
      sd.BorderColor[1] = bc;
      sd.BorderColor[2] = bc;
      sd.BorderColor[3] = bc;
      sd.MinLOD = -FLT_MAX;
      sd.MaxLOD = FLT_MAX;

      DXErr(DXDev->CreateSamplerState(&sd,&Variants[var].SamplerStates[i]));
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Compute Shader                                                     ***/
/***                                                                      ***/
/****************************************************************************/

sCSBuffer::sCSBuffer(sInt flags,sInt elements,sInt elementsize,const void *initdata)
{
  Flags = (flags & sTEX_FORMAT) 
        | (flags & (sTEX_CS_WRITE|sTEX_CS_COUNT|sTEX_CS_STACK|sTEX_CS_INDEX|sTEX_CS_VERTEX|sTEX_CS_INDIRECT)) 
        | (flags & (sTEX_DYNAMIC))
        | sTEX_BUFFER;

  SizeX = elements;
  SizeY = 0;
  SizeZ = 0;

  D3D11_BUFFER_DESC bd;
  D3D11_SHADER_RESOURCE_VIEW_DESC rd;
  D3D11_UNORDERED_ACCESS_VIEW_DESC ud;

  sClear(bd);
  sClear(rd);
  sClear(ud);

  DXGI_FORMAT fmt_res,fmt_tex,fmt_view,fmt_uav;
  D3D11_BIND_FLAG bind;

  sInt mm_dummy;
  sBool ds_dummy;
  D3D11_USAGE usage_dummy;
  ConvertFlags(Flags,mm_dummy,bind,usage_dummy,fmt_res,fmt_tex,fmt_view,ds_dummy);
  fmt_uav = fmt_view;

  if((Flags & sTEX_FORMAT)==sTEX_STRUCTURED)
  {
    bd.StructureByteStride = elementsize;
    bd.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
  }
  else if((Flags & sTEX_FORMAT)==sTEX_RAW)
  {
    elementsize=4;
    bd.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
    ud.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_RAW; 
    fmt_uav = DXGI_FORMAT_R32_TYPELESS;
  }
  else
  {
    elementsize = sGetBitsPerPixel(Flags)/8;
  }
  sVERIFY(elementsize!=0);
  
  bd.BindFlags = bind;
  bd.ByteWidth = elements*elementsize;
  bd.Usage = D3D11_USAGE_DEFAULT;

  rd.Format = fmt_view;
  rd.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
  rd.BufferEx.NumElements = elements;

  ud.Format = fmt_uav;
  ud.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
  ud.Buffer.NumElements = elements;
  if(Flags & sTEX_CS_STACK)
    ud.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_APPEND;
  if(Flags & sTEX_CS_COUNT)
    ud.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_COUNTER; 
  if(Flags & sTEX_CS_INDIRECT)
    bd.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS; 
  if(Flags & sTEX_DYNAMIC)
  {
    bd.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
    bd.Usage = D3D11_USAGE_DYNAMIC;
  }

  if(initdata)
  {
    D3D11_SUBRESOURCE_DATA srd;
    srd.pSysMem = initdata;
    srd.SysMemPitch = 0;
    srd.SysMemSlicePitch = 0;
    DXErr(DXDev->CreateBuffer(&bd,&srd,&DXBuffer));
  }
  else
  {
    DXErr(DXDev->CreateBuffer(&bd,0,&DXBuffer));
  }
  DXErr(DXDev->CreateShaderResourceView(DXBuffer,&rd,&DXTexView));
  if(Flags & sTEX_CS_WRITE)
    DXErr(DXDev->CreateUnorderedAccessView(DXBuffer,&ud,&DXTexUAV));
}

sCSBuffer::~sCSBuffer()
{
  sDXRELEASE(DXBuffer);
  sDXRELEASE(DXTexUAV);
  sDXRELEASE(DXTexView);
}

void sCSBuffer::GetSize(sInt &xs,sInt &ys,sInt &zs)
{
  xs = SizeX;
  ys = 0;
  zs = 0;
}

void sCSBuffer::BeginLoad(void **p)
{
  sVERIFY(LoadMipmap==-1);
  LoadMipmap = 1;

  D3D11_MAPPED_SUBRESOURCE mr;
  GTC->DXCtx->Map(DXBuffer,0,D3D11_MAP_WRITE_DISCARD,0,&mr);

  *p = mr.pData;
}

void sCSBuffer::EndLoad()
{
  sVERIFY(LoadMipmap==1);
  LoadMipmap = -1;

  GTC->DXCtx->Unmap(DXBuffer,0);
}

/****************************************************************************/

void sGeometry::SetVB(sCSBuffer *cb,sInt i)
{
  VB[i].ElementSize = Format->GetSize(i);
  VB[i].ElementCount = cb->SizeX*4/Format->GetSize(i);
  VB[i].DXBuffer = cb->DXBuffer;
  VB[i].DXBuffer->AddRef();
}

void sGeometry::SetIB(sCSBuffer *cb)
{
  sVERIFY(IndexSize==4);
  IB.ElementSize = IndexSize;
  IB.ElementCount = cb->SizeX*4/IndexSize;
  IB.DXBuffer = cb->DXBuffer;
  IB.DXBuffer->AddRef();
}

/****************************************************************************/

sComputeShader::sComputeShader(sShader *sh)
{
  for(sInt i=0;i<MaxTexture;i++)
  {
    Texture[i] = 0;
    Sampler[i] = 0;
    TFlags[i] = 0;
    DXsrv[i] = 0;
  }
  for(sInt i=0;i<MaxUAV;i++)
  {
    UAV[i] = 0;
    DXuavp[i] = 0;
    DXuavc[i] = 0;
  }
  Shader = 0;
  LastTexture = 0;
  BorderColor.Init(0,0,0,0);

  Shader = sh;
}

sComputeShader::~sComputeShader()
{
  for(sInt i=0;i<MaxTexture;i++)
    sDXRELEASE(Sampler[i]);
  delete Shader;
}

void sComputeShader::Prepare()
{
  sVERIFY(LastTexture==0);
  for(sInt i=0;i<MaxTexture;i++)
  {
    if(Texture[i] || (TFlags[i]&sMTF_EXTERN))
    {
      D3D11_SAMPLER_DESC sd;
      sClear(sd);

      switch(TFlags[i] & sMTF_LEVELMASK)
      {
      case sMTF_LEVEL0: sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT; break;
      case sMTF_LEVEL1: sd.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT; break;
      case sMTF_LEVEL2: sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; break;
      case sMTF_LEVEL3: sd.Filter = D3D11_FILTER_ANISOTROPIC; break;
      }
      if(TFlags[i] & sMTF_PCF)
      {
        sd.ComparisonFunc = D3D11_COMPARISON_LESS;
        sd.Filter = D3D11_FILTER(sd.Filter | 0x80);
      }
      else
      {
        sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
      }

      switch(TFlags[i] & sMTF_ADDRMASK_U)
      {
      case sMTF_TILE_U:   sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP; break;
      case sMTF_CLAMP_U:  sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP; break;
      case sMTF_MIRROR_U: sd.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR; break;
      case sMTF_BORDER_U: sd.AddressU = D3D11_TEXTURE_ADDRESS_BORDER; break;
      }
      switch(TFlags[i] & sMTF_ADDRMASK_V)
      {
      case sMTF_TILE_V:   sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP; break;
      case sMTF_CLAMP_V:  sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP; break;
      case sMTF_MIRROR_V: sd.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR; break;
      case sMTF_BORDER_V: sd.AddressV = D3D11_TEXTURE_ADDRESS_BORDER; break;
      }
      switch(TFlags[i] & sMTF_ADDRMASK_W)
      {
      case sMTF_TILE_W:   sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP; break;
      case sMTF_CLAMP_W:  sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP; break;
      case sMTF_MIRROR_W: sd.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR; break;
      case sMTF_BORDER_W: sd.AddressW = D3D11_TEXTURE_ADDRESS_BORDER; break;
      }

      sd.MaxAnisotropy = 1;
      sd.BorderColor[0] = BorderColor.x;
      sd.BorderColor[1] = BorderColor.y;
      sd.BorderColor[2] = BorderColor.z;
      sd.BorderColor[3] = BorderColor.w;
      sd.MinLOD = -FLT_MAX;
      sd.MaxLOD = FLT_MAX;

      DXErr(DXDev->CreateSamplerState(&sd,&Sampler[i]));
      LastTexture = i+1;
    }
  }
}

void sComputeShader::SetTexture(sInt n,sTextureBase *tex,sInt tflags)
{
  Texture[n] = tex;
  DXsrv[n] = 0;
  TFlags[n] = tflags;
  if(tex)
    DXsrv[n] = tex->DXTexView;
}

void sComputeShader::SetUAV(sInt n,sTextureBase *tex,sBool clear)
{
  UAV[n] = tex;
  DXuavp[n] = 0;
  if(tex)
    DXuavp[n] = tex->DXTexUAV;
  DXuavc[n] = clear ? 0 : sU32(-1);
}

void sComputeShader::Draw(sInt xs,sInt ys,sInt zs,sCBufferBase *cb0,sCBufferBase *cb1,sCBufferBase *cb2,sCBufferBase *cb3)
{
  Draw(xs,ys,zs,4,&cb0);
}


void sComputeShader::Draw(sInt xs,sInt ys,sInt zs,sInt cbcount,sCBufferBase **cbs)
{
  sSetCBuffers(cbs,cbcount);

  GTC->DXCtx->CSSetShader(Shader->cs,0,0);
  GTC->DXCtx->CSSetSamplers(0,MaxTexture,Sampler);
  GTC->DXCtx->CSSetShaderResources(0,MaxTexture,DXsrv);
  GTC->DXCtx->CSSetUnorderedAccessViews(0,MaxUAV,DXuavp,DXuavc);

  GTC->DXCtx->Dispatch(xs,ys,zs);

  // clear it 

  ID3D11UnorderedAccessView *uavp[MaxUAV];
  sU32 uavc[MaxUAV];
  sClear(uavp);
  for(sInt i=0;i<MaxUAV;i++)
    uavc[i] = UINT(-1);
  GTC->DXCtx->CSSetUnorderedAccessViews(0,MaxUAV,uavp,uavc);
}

/****************************************************************************/

#endif // sRENDERER == sRENDER_DX11


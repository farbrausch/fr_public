/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Base/DxShaderCompiler.hpp"
#include "Altona2/Libs/Base/SystemPrivate.hpp"
#include "Altona2/Libs/Base/FixedShader.hpp"

#undef new
#include <d3d11.h>
#include <d3dcompiler.h>
#define new sDEFINE_NEW

namespace Altona2 {

/****************************************************************************/

namespace Private
{

    class sDisplayInfoEx : public sDisplayInfo
    {
    public:
        sRect DesktopRect;
        IDXGIAdapter1 *Adapter;
        IDXGIOutput *Display;

        sDisplayInfoEx();
        ~sDisplayInfoEx();
        void Init(IDXGIAdapter1 *a,IDXGIOutput *d);
    };


    IDXGIFactory1 *DXGI;

    bool TargetOn;
    bool RestartFlag;
    sTargetPara TargetPara;

    sArray<sDisplayInfoEx *> EnumeratedDisplays;
    sArray<sAdapter *> OpenAdapters;
    bool DisableRenderLoop=0;
} // namepsace

using namespace Private;



sDisplayInfoEx::sDisplayInfoEx()
{
    Adapter = 0;
    Display = 0;
}
void sDisplayInfoEx::Init(IDXGIAdapter1 *a,IDXGIOutput *d)
{
    Adapter = a; a->AddRef();
    Display = d; d->AddRef();

    DXGI_OUTPUT_DESC od;
    DXGI_ADAPTER_DESC1 ad;
    MONITORINFOEXW mix;
    sClear(mix);
    mix.cbSize = sizeof(MONITORINFOEXW);
    DXErr(Display->GetDesc(&od));
    DXErr(Adapter->GetDesc1(&ad));
    if(!GetMonitorInfoW(od.Monitor,&mix))
        sASSERT0();

    DesktopRect.x0 = od.DesktopCoordinates.left;
    DesktopRect.y0 = od.DesktopCoordinates.top;
    DesktopRect.x1 = od.DesktopCoordinates.right;
    DesktopRect.y1 = od.DesktopCoordinates.bottom;

    sUTF16toUTF8((uint16 *)&od.DeviceName[0],MonitorName);
    sUTF16toUTF8((uint16 *)&ad.Description[0],AdapterName);
    SizeX = DesktopRect.SizeX();
    SizeY = DesktopRect.SizeY();
}
sDisplayInfoEx::~sDisplayInfoEx()
{
    sRelease(Adapter);
    sRelease(Display);
}

/****************************************************************************/
/***                                                                      ***/
/***   Windows Message Loop Modifications                                 ***/
/***                                                                      ***/
/****************************************************************************/

void Private::DestroyAllWindows()
{
    for(auto i : OpenScreens)
        DestroyWindow((HWND)(i->ScreenWindow));
}


void Private::sCreateWindow(const sScreenMode &sm)
{
}

void Private::Render()
{
    if(DisableRenderLoop)
        return;

    sPreFrameHook.Call();
    {
        if(RestartFlag)
        {
            for(auto i : OpenScreens)
                i->RestartGfx();
            RestartFlag = 0;
        }
        {
            sZONE("Render",0xff004000);
            sGPU_ZONE("Render",0xff004000);

            CurrentApp->OnFrame();
            sBeginFrameHook.Call();
            CurrentApp->OnPaint();
            sEndFrameHook.Call();
            for(auto i : OpenAdapters)
                if(i)
                    i->BeforeFrame();
        }
        if(sGC) 
            sGC->CollectIfTriggered();
        for(auto i : OpenScreens)
        {
            if(!(i->ScreenMode.Flags & sSM_Headless))
            {
                uptr index = sGetIndex(OpenScreens,i);
                int swap = index==0 || (i->ScreenMode.Flags & sSM_NoVSync) ? 0 : 1;
                i->DXSwap->Present(swap,0);
            }
            else
            {
                i->DXSwap->Present(0,0);
            }
        }
        sUpdateGfxStats();
    }
}

void Private::sRestartGfx()
{
    RestartFlag = 1;
}

static void sFindFormat(int af,DXGI_FORMAT &fmt_res,DXGI_FORMAT &fmt_view,DXGI_FORMAT &fmt_tex);

/****************************************************************************/
/***                                                                      ***/
/***   Screen                                                             ***/
/***                                                                      ***/
/****************************************************************************/

sAdapterPrivate::sAdapterPrivate()
{
    DXDev = 0;
    DXCtx = 0;
    DrawIndexedInstancedIndirectMtrl = 0;
    DrawInstancedIndirectMtrl = 0;
    DispatchIndirectMtrl = 0;
    IndirectBuffer = 0;
    ImmediateContext = 0;
    IndirectIndex = 0;
    IndirectConstants = 0;
}

sAdapterPrivate::~sAdapterPrivate()
{
    sRelease(DXCtx);
    sRelease(DXDev);
    sDelete(IndirectBuffer);
    sDelete(IndirectConstants);
    sDelete(DrawIndexedInstancedIndirectMtrl);
    sDelete(DrawInstancedIndirectMtrl);
    sDelete(DispatchIndirectMtrl);
    delete ImmediateContext;
}

void sAdapterPrivate::InitPrivate(int id,bool debug)
{
    sDisplayInfoEx *di = EnumeratedDisplays[id];
    sLogF("gfx","open adapter %d (%s)",di->AdapterNumber,di->AdapterName);

    // create device

    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };

    HRESULT hr = D3D11CreateDevice(
        EnumeratedDisplays[id]->Adapter,D3D_DRIVER_TYPE_UNKNOWN,0,debug ? D3D11_CREATE_DEVICE_DEBUG : 0,levels,sCOUNTOF(levels),
        D3D11_SDK_VERSION,&DXDev,0,&DXCtx);

    if(FAILED(hr))
        sFatal("count not create dx11 device");

    ImmediateContext = new sContext();
    ImmediateContext->Init((sAdapter *)this,DXCtx);
    DXCtx->AddRef();

    auto ada = (sAdapter *) this;
    ada->Init();

    IndirectBuffer = new sResource(ada,sResPara(sRBM_Unordered|sRM_DrawIndirect|sRU_Gpu,sRFC_R|sRFB_32|sRFT_UInt,0,MaxDrawIndirectBytes/4,0,0));
    sMaterial *mtrl = 0;

    mtrl = new sMaterial(ada);
    mtrl->SetShaders(DrawIndexedInstancedIndirectShader.Get(0));
    mtrl->SetUav(sST_Compute,0,IndirectBuffer);
    DrawIndexedInstancedIndirectMtrl = mtrl;

    mtrl = new sMaterial(ada);
    mtrl->SetShaders(DrawInstancedIndirectShader.Get(0));
    mtrl->SetUav(sST_Compute,0,IndirectBuffer);
    DrawInstancedIndirectMtrl = mtrl;

    mtrl = new sMaterial(ada);
    mtrl->SetShaders(DispatchIndirectShader.Get(0));
    mtrl->SetUav(sST_Compute,0,IndirectBuffer);
    DispatchIndirectMtrl = mtrl;

    IndirectConstants = new sCBuffer<DrawIndexedInstancedIndirectShader_cbc0>(ada,sST_Compute,0);
}

void sAdapterPrivate::BeforeFrame()
{
    IndirectIndex = 0;
    DXCtx->ClearState();
}

/****************************************************************************/

sResource *sAdapter::CreateBufferRW(int count,int format,int additionalbind)
{
    return new sResource(this,sResPara(sRBM_Unordered|sRBM_Shader|sRU_Gpu|additionalbind,format,0,count,0));
}

sResource *sAdapter::CreateStructRW(int count,int bytesperelement,int additionalbind)
{
    return new sResource(this,sResBufferPara(sRBM_Unordered|sRBM_Shader|sRM_Structured|sRU_Gpu|additionalbind,bytesperelement,count));
}

sResource *sAdapter::CreateCounterRW(int count,int bytesperelement,int additionalbind)
{
    return new sResource(this,sResBufferPara(sRBM_Unordered|sRBM_Shader|sRM_Structured|sRM_Counter|sRU_Gpu|additionalbind,bytesperelement,count));
}

sResource *sAdapter::CreateAppendRW(int count,int bytesperelement,int additionalbind)
{
    return new sResource(this,sResBufferPara(sRBM_Unordered|sRBM_Shader|sRM_Structured|sRM_Append|sRU_Gpu|additionalbind,bytesperelement,count));
}

sResource *sAdapter::CreateRawRW(int bytecount,int additionalbind)
{
    return new sResource(this,sResBufferPara(sRBM_Unordered|sRBM_Shader|sRM_Raw|sRU_Gpu|additionalbind,0,bytecount/4,sRFB_32|sRFC_R|sRFT_Typeless));
}

sResource *sAdapter::CreateStruct(int count,int bytesperelement,int additionalbind,void *data)
{
    return new sResource(this,sResBufferPara(sRBM_Shader|sRM_Structured|additionalbind,bytesperelement,count),data,count*bytesperelement);
}

sResource *sAdapter::CreateBuffer(int count,int format,int additionalbind,void *data,int size)
{
    return new sResource(this,sResPara(sRBM_Shader|additionalbind,format,0,count,0),data,size);
}

/****************************************************************************/

sContextPrivate::sContextPrivate()
{
    DXCtx = 0;
    Adapter = 0;
    ComputeSyncUav = 0;

}

sContextPrivate::~sContextPrivate()
{
    sRelease(DXCtx);
    delete ComputeSyncUav;
}

void sContextPrivate::Init(sAdapter *ada,ID3D11DeviceContext *ctx)
{
    DXCtx = ctx;
    Adapter = ada;
    ComputeSyncUav = Adapter->CreateBufferRW(1,sRFB_32|sRFC_R|sRFT_UInt);
}

/****************************************************************************/

sScreenPrivate::~sScreenPrivate()
{
    sDelete(ColorBuffer);
    sDelete(DepthBuffer);
    sRelease(DXSwap);
}

sScreenPrivate::sScreenPrivate(const sScreenMode &sm_)
{
    DXSwap = 0;
    ScreenWindow = 0;
    ColorBuffer = 0;
    DepthBuffer = 0;
    Adapter = 0;

    ScreenMode = sm_;
    if(ScreenMode.ColorFormat==0)
        ScreenMode.ColorFormat = sRF_RGBA8888;
    if(ScreenMode.DepthFormat==0)
        ScreenMode.DepthFormat = sRF_D24S8;


    sDisplayInfoEx *di = EnumeratedDisplays[ScreenMode.Monitor];
    sLogF("gfx","open screen %d, which is display %d adapter %d",ScreenMode.Monitor,di->DisplayNumber,di->AdapterName);

    if(OpenAdapters[di->AdapterNumber]==0)
    {
        OpenAdapters[di->AdapterNumber] = new sAdapter();
        OpenAdapters[di->AdapterNumber]->InitPrivate(ScreenMode.Monitor,(ScreenMode.Flags & sSM_Debug)!=0);
    }
    Adapter = OpenAdapters[di->AdapterNumber];

    // create window

    DisableRenderLoop = 1;

    uint16 titlebuffer[sFormatBuffer];
    sUTF8toUTF16(ScreenMode.WindowTitle,titlebuffer,sCOUNTOF(titlebuffer));

    if(ScreenMode.Flags & sSM_Fullscreen)
    {
        int px = di->DesktopRect.x0;
        int py = di->DesktopRect.y0;
//        ScreenMode.SizeX = di->DesktopRect.SizeX();
//        ScreenMode.SizeY = di->DesktopRect.SizeY();

        ScreenWindow = CreateWindowExW(0,L"Altona2",(LPCWSTR) titlebuffer,
            WS_OVERLAPPEDWINDOW,px,py,ScreenMode.SizeX,ScreenMode.SizeY,
            GetDesktopWindow(),0,WindowClass.hInstance,0);
    }
    else if(ScreenMode.Flags & sSM_FullWindowed)
    {    
        int px = di->DesktopRect.x0;
        int py = di->DesktopRect.y0;
        ScreenMode.SizeX = di->DesktopRect.SizeX();
        ScreenMode.SizeY = di->DesktopRect.SizeY();

        ScreenWindow = CreateWindowExW(OpenScreens.GetCount()>0 ? WS_EX_TOOLWINDOW : 0,L"Altona2",(LPCWSTR) titlebuffer,
            WS_POPUP,px,py,ScreenMode.SizeX,ScreenMode.SizeY,
            GetDesktopWindow(),0,WindowClass.hInstance,0);
    }
    else
    {
        RECT r2;
        r2.left = r2.top = 0;
        r2.right = ScreenMode.SizeX; 
        r2.bottom = ScreenMode.SizeY;
        AdjustWindowRect(&r2,WS_OVERLAPPEDWINDOW,FALSE);

        ScreenWindow = CreateWindowExW(0,L"Altona2",(LPCWSTR) titlebuffer, 
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,r2.right-r2.left,r2.bottom-r2.top,
            GetDesktopWindow(), NULL, WindowClass.hInstance, NULL );
    }

    DisableRenderLoop = 0;

    // create device

    DXGI_SWAP_CHAIN_DESC sd;
    sClear(sd);
    if(ScreenMode.Flags & sSM_Fullscreen)
    {
        sd.Windowed = 0;
        sd.BufferCount = 1;
        sd.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    }
    else
    {
        sd.Windowed = 1;
        sd.BufferCount = 1;
    }

    DXGI_FORMAT fc,fs,fd;
    sFindFormat(ScreenMode.ColorFormat,fc,fs,fd);
    sd.BufferDesc.Width = ScreenMode.SizeX;
    sd.BufferDesc.Height = ScreenMode.SizeY;
    sd.BufferDesc.Format = fs;
    sd.BufferDesc.RefreshRate.Numerator = 0;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = (HWND) ScreenWindow;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    if(ScreenMode.Flags & sSM_PartialUpdate)
    {
        sd.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
        ScreenMode.BufferSequence = sd.BufferCount;
    }
    else
    {
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        ScreenMode.BufferSequence = 0;
    }

    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };

    HRESULT hr = DXGI->CreateSwapChain(Adapter->DXDev,&sd,&DXSwap);

    if(FAILED(hr))
        sFatal("count not create dx11 swap chain");

    // set frame latency

    IDXGIDevice1 *dev;
    DXErr(Adapter->DXDev->QueryInterface(__uuidof(IDXGIDevice1),(void**)(&dev)));
    dev->SetMaximumFrameLatency(1);
    sRelease(dev);

    CreateSwapBuffer();

    // done

    if(ScreenWindow && !(sm_.Flags & sSM_Headless))
        ShowWindow((HWND)ScreenWindow,SW_SHOWDEFAULT);
}

void sScreen::SetTitle(const char *title)
{
    ScreenMode.WindowTitle = title;
    uint16 titlebuffer[sFormatBuffer];
    sUTF8toUTF16(ScreenMode.WindowTitle,titlebuffer,sCOUNTOF(titlebuffer));

    SetWindowTextW((HWND)ScreenWindow,(LPCWSTR)titlebuffer);
}

void sScreenPrivate::CreateSwapBuffer()
{
    ID3D11Texture2D *DXBackBuffer;
    DXErr(DXSwap->GetBuffer(0,__uuidof(ID3D11Texture2D),(LPVOID*)&DXBackBuffer))
        sResPara rp(sRBM_ColorTarget|sRU_Gpu|sRM_Texture,ScreenMode.ColorFormat,sRES_Internal,ScreenMode.SizeX,ScreenMode.SizeY,0,0);
    if(ScreenMode.Flags & sSM_Shared)
        rp.Flags |= sRES_SharedHandle;

    ColorBuffer = new sResource(Adapter,rp);
    ColorBuffer->PrivateSetInternalResource(DXBackBuffer);

    sRelease(DXBackBuffer);
    if(ScreenMode.DepthFormat!=0)
    {
        rp.Format = ScreenMode.DepthFormat;
        rp.Mode = sRBM_DepthTarget|sRU_Gpu|sRM_Texture;
        rp.Flags &= ~sRES_Internal;
        DepthBuffer = new sResource(Adapter,rp);
    }
}

void sScreenPrivate::RestartGfx()
{
    // tear down

    sDelete(ColorBuffer);
    sDelete(DepthBuffer);

    // resize

    bool fs = (ScreenMode.Flags & sSM_Fullscreen)?1:0;
    DXErr(DXSwap->SetFullscreenState(fs,0));

    sRect r;
    GetClientRect((HWND)ScreenWindow,(RECT *)&r);
    int sx = r.SizeX();
    int sy = r.SizeY();
    sLogF("gfx","resize screen %d to %dx%d",ScreenMode.Monitor,sx,sy);
    ScreenMode.SizeX = sx;
    ScreenMode.SizeY = sy;

    DXGI_MODE_DESC md;
    sClear(md);
    md.Width = sx;
    md.Height = sy;
    md.RefreshRate.Numerator = 0;
    md.RefreshRate.Denominator = 1;
    DXErr(DXSwap->ResizeTarget(&md));
    DXErr(DXSwap->ResizeBuffers(1,sx,sy,DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_SWAP_EFFECT_DISCARD));

    // re-create

    CreateSwapBuffer();
}

void sScreen::WindowControl(WindowControlEnum mode)
{
    switch(mode)
    {
    case sWC_Hide:
        ShowWindow((HWND) ScreenWindow,SW_HIDE);
        break;
    case sWC_Normal:
        ShowWindow((HWND) ScreenWindow,SW_SHOWNORMAL);
        break;
    case sWC_Minimize:
        ShowWindow((HWND) ScreenWindow,SW_MINIMIZE);
        break;
    case sWC_Maximize:
        ShowWindow((HWND) ScreenWindow,SW_MAXIMIZE);
        break;
    }
}

/****************************************************************************/
/*
sContext *sGetCurrentContext()
{
    return sGetScreen(-1)->Adapter->ImmediateContext;
}

sAdapter *sGetCurrentAdapter()
{
    return sGetScreen(-1)->Adapter;
}
*/
int sGetDisplayCount()
{
    return EnumeratedDisplays.GetCount();
}

void sGetDisplayInfo(int display,sDisplayInfo &info)
{
    info = *(sDisplayInfo *)EnumeratedDisplays[display];
}

bool sIsSupportedFormat(int format)
{
    return format!=sRF_PVR2 &&
        format!=sRF_PVR4 && 
        format!=sRF_PVR2A && 
        format!=sRF_PVR4A &&
        format!=sRF_ETC1 && 
        format!=sRF_ETC2 && 
        format!=sRF_ETC2A;
}

bool sIsElementIndexUintSupported()
{
    return true;
}

/****************************************************************************/
/***                                                                      ***/
/***   Create Graphics Subsystems                                         ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/

class sDXGISubsystem : public sSubsystem
{
public:
    sDXGISubsystem() : sSubsystem("DXGI",0x40) {}
    void Init()
    {
        DXErr(CreateDXGIFactory1(__uuidof(IDXGIFactory1),(void**)(&DXGI)));
        //    DXErr(DXGI->MakeWindowAssociation(Window,0));

        int n=0;
        int adaptercount=0;
        while(1)
        {
            IDXGIAdapter1 *adapter = 0;
            HRESULT hr = DXGI->EnumAdapters1(adaptercount,&adapter);
            if(FAILED(hr))
                break;
            for(int di=0;1;di++)
            {
                IDXGIOutput *output = 0;
                HRESULT hr = adapter->EnumOutputs(di,&output);
                if(FAILED(hr))
                    break;
                sDisplayInfoEx *dix = new sDisplayInfoEx();
                dix->Init(adapter,output);
                dix->AdapterNumber = adaptercount;
                dix->DisplayNumber = n++;
                dix->DisplayInAdapterNumber = di;
                EnumeratedDisplays.Add(dix);
                output->Release();
            }
            adapter->Release();
            adaptercount++;
        }

        for(auto i : EnumeratedDisplays)
        {
            sLogF("gfx","display %d@%d: %dx%d - %s on adapter %s",i->DisplayNumber,i->AdapterNumber,i->SizeX,i->SizeY,i->MonitorName,i->AdapterName);
        }

        sASSERT(EnumeratedDisplays.GetCount()>0);
        OpenAdapters.SetSize(adaptercount);
        for(int i=0;i<OpenAdapters.GetCount();i++)
            OpenAdapters[i] = 0;
    }
    void Exit()
    {
        //while(!OpenScreens.IsEmpty())
        //    delete OpenScreens.RemTail();
        //OpenAdapters.DeleteAll();
        OpenAdapters.FreeMemory();
        EnumeratedDisplays.DeleteAll();
        EnumeratedDisplays.FreeMemory();
        sRelease(DXGI);
    }
} sDXGISubsystem_;

/****************************************************************************/

class sDXDev11Subsystem : public sSubsystem
{
public:
    sDXDev11Subsystem() : sSubsystem("Direct3D11",0xc0) {}
    void Init()
    {
        RestartFlag = 0;
    }
    void Exit()
    {
        while(!OpenScreens.IsEmpty())
            delete OpenScreens.RemTail();
        for(auto &i : OpenAdapters)
        {
            delete i;
            i = 0;
        }
    }
} sDXDev11Subsystem_;


/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/
/***                                                                      ***/
/***   Targets and CopyTexture                                            ***/
/***                                                                      ***/
/****************************************************************************/

void sContext::BeginTarget(const sTargetPara &target)
{
    sASSERT(!TargetOn);
    TargetOn = 1;
    TargetPara = target;

    ID3D11RenderTargetView *views[sGfxMaxTargets];
    sClear(views);

    for(int i=0;i<sGfxMaxTargets;i++)
    {
        if(!target.Target[i])
            break;
        views[i] = target.Target[i]->PrivateRTView();
    }

    if(target.Depth)
        DXCtx->OMSetRenderTargets(sGfxMaxTargets,views,target.Depth->PrivateDSView());
    else
        DXCtx->OMSetRenderTargets(sGfxMaxTargets,views,0);

    D3D11_VIEWPORT vp;
    if(target.Target[0])
    {
        vp.Width  = (FLOAT)(target.Window.SizeX());
        vp.Height = (FLOAT)(target.Window.SizeY());
        vp.TopLeftX = (FLOAT)target.Window.x0;
        vp.TopLeftY = (FLOAT)target.Window.y0;
    }
    else if(target.Depth)
    {
        vp.Width  = (FLOAT)(target.Depth->Para.SizeX);
        vp.Height = (FLOAT)(target.Depth->Para.SizeY);
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;

    }
    else
    {
        sASSERT0();
    }

    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    DXCtx->RSSetViewports(1,&vp);

    for(int i=0;i<sGfxMaxTargets;i++)
    {
        if(!target.Target[i]) break;
        if(target.Flags & sTAR_ClearColor)
            DXCtx->ClearRenderTargetView(target.Target[i]->PrivateRTView(),&target.ClearColor[i].x);
    }
    DXCtx->RSSetScissorRects(1,(const RECT *)&target.Window);

    if(target.Depth && (target.Flags & sTAR_ClearDepth))
    {
        DXCtx->ClearDepthStencilView(target.Depth->PrivateDSView(),D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL,target.ClearZ,target.ClearStencil);
    }
}

void sContext::EndTarget()
{
    sASSERT(TargetOn);
    TargetOn = 0;

    ID3D11RenderTargetView *views[sGfxMaxTargets];
    sClear(views);
    DXCtx->OMSetRenderTargets(sGfxMaxTargets,views,0);
}

void sContext::SetScissor(bool enable,const sRect &r)
{
    if(enable)
        DXCtx->RSSetScissorRects(1,(const RECT *)&r);
    else
        DXCtx->RSSetScissorRects(1,(const RECT *)&TargetPara.Window);
}

void sContext::Resolve(sResource *dest,sResource *src)
{
    DXCtx->ResolveSubresource(dest->GetDXResource(),0,src->GetDXResource(),0,src->Format_Shader);
}

void sContext::Copy(const sCopyTexturePara &cp)
{
    ID3D11Resource *dr = cp.Dest->DXResource;
    ID3D11Resource *sr = cp.Source->DXResource;
    if(cp.Source->Para.MSCount>=2 && cp.Dest->Para.MSCount<2)
    {
        DXCtx->ResolveSubresource(dr,0,sr,0,cp.Dest->Format_Shader);
    }
    else
    {
        if(cp.DestRect.x0==0 && cp.DestRect.y0==0)
            DXCtx->CopyResource(dr,sr);
        else
            DXCtx->CopySubresourceRegion(dr,0,cp.DestRect.x0,cp.DestRect.y0,0,sr,0,0);
    }
}

sResource *sScreen::GetScreenColor()
{
    return ColorBuffer;
}

sResource *sScreen::GetScreenDepth()
{
    return DepthBuffer;
}

/****************************************************************************/
/***                                                                      ***/
/***   sResource, Textures, Buffers                                       ***/
/***                                                                      ***/
/****************************************************************************/

static void sFindFormat(int af,DXGI_FORMAT &fmt_create,DXGI_FORMAT &fmt_shader,DXGI_FORMAT &fmt_depth)
{
    fmt_create = DXGI_FORMAT_UNKNOWN;
    fmt_shader = DXGI_FORMAT_UNKNOWN;
    fmt_depth = DXGI_FORMAT_UNKNOWN;
    switch(af)
    {
    case sRFC_R   |sRFB_16|sRFT_Float:   fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R16_FLOAT; break;
    case sRFC_RG  |sRFB_16|sRFT_Float:   fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R16G16_FLOAT; break;
    case sRFC_RGBA|sRFB_16|sRFT_Float:   fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
    case sRFC_R   |sRFB_32|sRFT_Float:   fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R32_FLOAT; break;
    case sRFC_RG  |sRFB_32|sRFT_Float:   fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R32G32_FLOAT; break;
    case sRFC_RGBA|sRFB_32|sRFT_Float:   fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R32G32B32A32_FLOAT; break;

    case sRFC_R   |sRFB_16|sRFT_UInt:    fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R16_UINT; break;
    case sRFC_RG  |sRFB_16|sRFT_UInt:    fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R16G16_UINT; break;
    case sRFC_RGBA|sRFB_16|sRFT_UInt:    fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R16G16B16A16_UINT; break;
    case sRFC_R   |sRFB_32|sRFT_UInt:    fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R32_UINT; break;
    case sRFC_RG  |sRFB_32|sRFT_UInt:    fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R32G32_UINT; break;
    case sRFC_RGBA|sRFB_32|sRFT_UInt:    fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R32G32B32A32_UINT; break;

    case sRFC_R   |sRFB_16|sRFT_UNorm:   fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R16_UNORM; break;
    case sRFC_RG  |sRFB_16|sRFT_UNorm:   fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R16G16_UNORM; break;
    case sRFC_RGBA|sRFB_16|sRFT_UNorm:   fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R16G16B16A16_UNORM; break;


    case sRFC_R   |sRFB_16|sRFT_SInt:    fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R16_SINT; break;
    case sRFC_RG  |sRFB_16|sRFT_SInt:    fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R16G16_SINT; break;
    case sRFC_RGBA|sRFB_16|sRFT_SInt:    fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R16G16B16A16_SINT; break;
    case sRFC_R   |sRFB_32|sRFT_SInt:    fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R32_SINT; break;
    case sRFC_RG  |sRFB_32|sRFT_SInt:    fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R32G32_SINT; break;
    case sRFC_RGBA|sRFB_32|sRFT_SInt:    fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R32G32B32A32_SINT; break;

    case sRFC_R   |sRFB_16|sRFT_SNorm:   fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R16_SNORM; break;
    case sRFC_RG  |sRFB_16|sRFT_SNorm:   fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R16G16_SNORM; break;
    case sRFC_RGBA|sRFB_16|sRFT_SNorm:   fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R16G16B16A16_SNORM; break;

    case sRFC_R   |sRFB_8|sRFT_UInt:   fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R8_UINT; break;
    case sRFC_RG  |sRFB_8|sRFT_UInt:   fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R8G8_UINT; break;
    case sRFC_RGBA|sRFB_8|sRFT_UInt:   fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R8G8B8A8_UINT; break;
    case sRFC_R   |sRFB_8|sRFT_UNorm:  fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R8_UNORM; break;
    case sRFC_RG  |sRFB_8|sRFT_UNorm:  fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R8G8_UNORM; break;
    case sRFC_RGBA|sRFB_8|sRFT_UNorm:  fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R8G8B8A8_UNORM; break;
    case sRFC_R   |sRFB_8|sRFT_SInt:   fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R8_SINT; break;
    case sRFC_RG  |sRFB_8|sRFT_SInt:   fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R8G8_SINT; break;
    case sRFC_RGBA|sRFB_8|sRFT_SInt:   fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R8G8B8A8_SINT; break;
    case sRFC_R   |sRFB_8|sRFT_SNorm:  fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R8_SNORM; break;
    case sRFC_RG  |sRFB_8|sRFT_SNorm:  fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R8G8_SNORM; break;
    case sRFC_RGBA|sRFB_8|sRFT_SNorm:  fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R8G8B8A8_SNORM; break;
    case sRFC_RGBA|sRFB_8|sRFT_SRGB:   fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; break;

    case sRFC_RGBA|sRFB_10_10_10_2|sRFT_UNorm: fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R10G10B10A2_UNORM; break;
    case sRFC_RGBA|sRFB_10_10_10_2|sRFT_UInt : fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R10G10B10A2_UINT; break;
    case sRFC_RGB |sRFB_11_11_10  |sRFT_Float: fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R11G11B10_FLOAT; break;

    case sRF_BGRA8888:      fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_B8G8R8A8_UNORM; break;
    case sRF_BGRA8888_SRGB: fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; break;
    case sRF_BGRA5551:      fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_B5G5R5A1_UNORM; break;
    case sRF_BGR565:        fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_B5G6R5_UNORM; break;
    case sRF_BC1:           fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_BC1_UNORM; break;
    case sRF_BC2:           fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_BC2_UNORM; break;
    case sRF_BC3:           fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_BC3_UNORM; break;
    case sRF_BC1_SRGB:      fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_BC1_UNORM_SRGB; break;
    case sRF_BC2_SRGB:      fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_BC2_UNORM_SRGB; break;
    case sRF_BC3_SRGB:      fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_BC3_UNORM_SRGB; break;
    case sRF_A8:            fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_A8_UNORM; break;
    case sRF_X32:           fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_R32_TYPELESS; break;

    case sRF_D16:     fmt_create = DXGI_FORMAT_R16_TYPELESS;   fmt_shader = DXGI_FORMAT_R16_UNORM;   fmt_depth = DXGI_FORMAT_D16_UNORM; break;
        //    case sRF_D24:     fmt_create = DXGI_FORMAT_R24G8_TYPELESS; fmt_shader = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; fmt_depth = DXGI_FORMAT_D24_UNORM_S8_UINT; break;
    case sRF_D24S8:   fmt_create = DXGI_FORMAT_R24G8_TYPELESS; fmt_shader = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; fmt_depth = DXGI_FORMAT_D24_UNORM_S8_UINT; break;
    case sRF_D32:     fmt_create = DXGI_FORMAT_R32_TYPELESS;   fmt_shader = DXGI_FORMAT_R32_FLOAT;   fmt_depth = DXGI_FORMAT_D32_FLOAT; break;

    case 0:  fmt_create = fmt_shader = fmt_depth = DXGI_FORMAT_UNKNOWN; break;

    default: sASSERT0(); break;
    }
}

void sAdapter::GetBestMultisampling(const sResPara &respara,int &count,int &quality)
{
    DXGI_FORMAT fc,fs,fd;
    sFindFormat(respara.Format,fc,fs,fd);
    UINT q = 0;

    for(int i=32;i>0;i=i/2)
    {
        HRESULT hr = DXDev->CheckMultisampleQualityLevels(fs,i,&q);
        if(!FAILED(hr) && q>0)
        {
            quality = q-1;
            count = i;
            return;
        }
    }

    quality = 0;
    count = 1;
}

int sAdapter::GetMultisampleQuality(const sResPara &respara,int count)
{
    if(count<2)
        return 0;
    DXGI_FORMAT fc,fs,fd;
    sFindFormat(respara.Format,fc,fs,fd);
    UINT quality = 0;
    if(FAILED(DXDev->CheckMultisampleQualityLevels(fs,count,&quality)))
        return 0;
    return quality;
}

sResource::sResource(sAdapter *adapter,const sResPara &para,const void *data,uptr size)
{
    Adapter = adapter;
    Para = para;
    ShaderResourceView = 0;
    RenderTargetView = 0;
    DepthStencilView = 0;
    UnorderedAccessView = 0;
    DXResource = 0;
    STResource = 0;
    Loading = 0;
    SharedHandle = 0;
    Format_Create = DXGI_FORMAT_UNKNOWN;
    Format_Shader = DXGI_FORMAT_UNKNOWN;
    Format_Depth = DXGI_FORMAT_UNKNOWN;
    D3D11_SUBRESOURCE_DATA *srdp=0;
    D3D11_SUBRESOURCE_DATA srd[16];
    uint bindflags = 0;
    uint cpuflags = 0;
    uint miscflags = 0;
    int bitsperpixel = 0;
    bool staging = 0;
    D3D11_USAGE usage = D3D11_USAGE_DEFAULT;

    // sanitize mipmaps

    if(Para.Mipmaps == 0)
        Para.Mipmaps = Para.GetMaxMipmaps();
    else
        Para.Mipmaps = sMin(Para.Mipmaps,Para.GetMaxMipmaps());
    if(Para.Flags & sRES_NoMipmaps)
        Para.Mipmaps = 1;

    // pixel format 

    sFindFormat(Para.Format,Format_Create,Format_Shader,Format_Depth);
    bitsperpixel = sGetBitsPerPixel(Para.Format);
    if(Para.BitsPerElement == 0) 
        Para.BitsPerElement = bitsperpixel;
    else 
        if(bitsperpixel!=0) 
            sASSERT(Para.BitsPerElement==bitsperpixel);
    int elements = Para.SizeX 
        * (Para.SizeY ? Para.SizeY : 1) 
        * (Para.SizeZ ? Para.SizeZ : 1) 
        * (Para.SizeA ? Para.SizeA : 1);
    uptr level0size = uptr((uint64(Para.BitsPerElement) * uint64(elements)) / 8 );
    if(Para.BitsPerElement==0)
        level0size = elements;
    uptr totalsize = 0;
    int dimensions = Para.GetDimensions();
    for(int i=0;i<Para.Mipmaps;i++)
        totalsize += level0size >> (i*dimensions);

    // figure out binding and other flags

    if(Para.Mode & sRBM_Shader)         { bindflags |= D3D11_BIND_SHADER_RESOURCE; }
    if(Para.Mode & sRBM_Index)          { bindflags |= D3D11_BIND_INDEX_BUFFER; }
    if(Para.Mode & sRBM_Vertex)         { bindflags |= D3D11_BIND_VERTEX_BUFFER; }
    if(Para.Mode & sRBM_ColorTarget)    { bindflags |= D3D11_BIND_RENDER_TARGET; }
    if(Para.Mode & sRBM_DepthTarget)    { bindflags |= D3D11_BIND_DEPTH_STENCIL; }
    if(Para.Mode & sRBM_Constant)       { bindflags |= D3D11_BIND_CONSTANT_BUFFER; }
    if(Para.Mode & sRBM_Unordered)      { bindflags |= D3D11_BIND_UNORDERED_ACCESS; }
    if(Para.Mode & sRM_DrawIndirect)    { miscflags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS; }
    if(Para.Mode & sRM_Raw)             { miscflags |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS; }
    if(Para.Mode & sRM_Structured)      { miscflags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED; }
    if(Para.Mode & sRM_Cubemap)         { miscflags |= D3D11_RESOURCE_MISC_TEXTURECUBE; }
    if(Para.Mode & sRM_GenerateMips)    { miscflags |= D3D11_RESOURCE_MISC_GENERATE_MIPS; }

    // check for some dx facts

    bool same = Format_Create==Format_Shader;
    if(Para.Mode & sRM_Counter)
    {
        sASSERT(Para.Mode & sRM_Structured);
        sASSERT(same && Format_Create == DXGI_FORMAT_UNKNOWN);
    }
    if(Para.Mode & sRM_Append)
    {
        sASSERT(Para.Mode & sRM_Structured);
        sASSERT(same && Format_Create == DXGI_FORMAT_UNKNOWN);
    }
    if(Para.Mode & sRM_Raw)
    {
        sASSERT(same && Format_Create == DXGI_FORMAT_R32_TYPELESS);
    }

    // access mode

    switch(Para.Mode & sRU_Mask)
    {
    case sRU_Static: 
        sASSERT(data);
        sASSERT(size == totalsize);

        usage = D3D11_USAGE_IMMUTABLE; 

        srdp = &srd[0];
        {
            const uint8 *datap = (const uint8 *) data;
            for(int i=0;i<Para.Mipmaps;i++)
            {
                int bc = Para.GetBlockSize();
                srd[i].pSysMem = datap;
                srd[i].SysMemPitch = (Para.SizeX>>i)*Para.BitsPerElement/8 * bc;
                srd[i].SysMemSlicePitch = (Para.SizeY>>i)*srdp->SysMemPitch * bc * bc;

                datap += Para.GetTextureSize(i);
            }
        }
        break;

    case sRU_Gpu: 
        sASSERT(!data);
        usage = D3D11_USAGE_DEFAULT; 
        break;

    case sRU_MapWrite:
        sASSERT(!data);
        usage = D3D11_USAGE_DYNAMIC; 
        cpuflags = D3D11_CPU_ACCESS_WRITE;
        break;

    case sRU_Update:
        usage = D3D11_USAGE_DEFAULT; 
        break;
/*
    case sRU_StageWrite:
        sASSERT(!data);
        usage = D3D11_USAGE_DYNAMIC; 
        cpuflags = D3D11_CPU_ACCESS_WRITE;
        staging = 1;
        break;
        */
    case sRU_SystemMem:
        sASSERT(!data);
        usage = D3D11_USAGE_STAGING; 
        cpuflags = D3D11_CPU_ACCESS_READ;
        break;
/*
    case sRU_MapWrite2:
        sASSERT(!data);
        usage = D3D11_USAGE_STAGING; 
        cpuflags = D3D11_CPU_ACCESS_WRITE;
        break;
        */
    default: 
        sASSERT0();
        break;
    }

    if(!(Para.Flags & sRES_Internal))
    {
        if(Para.Mode & sRM_Texture)
        {
            if(Para.SizeY==0 && Para.SizeZ==0)
            {
                D3D11_TEXTURE1D_DESC td;
                sClear(td);
                td.Width = Para.SizeX;
                td.MipLevels = Para.Mipmaps;
                td.ArraySize = Para.SizeA ? Para.SizeA : 1;
                td.Format = Format_Create;
                td.BindFlags = bindflags;
                td.CPUAccessFlags = cpuflags;
                td.MiscFlags = miscflags;
                td.Usage = usage;
                DXErr(adapter->DXDev->CreateTexture1D(&td,srdp,&DXTex1D));

                if(staging)
                {
                    td.Usage = D3D11_USAGE_STAGING;
                    td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                    td.BindFlags = 0;
                    DXErr(adapter->DXDev->CreateTexture1D(&td,0,&STTex1D));  
                }
            }
            else if(para.SizeZ==0)
            {
                if(Para.SharedHandle)
                {
                    DXErr(adapter->DXDev->OpenSharedResource((HANDLE) Para.SharedHandle,__uuidof(ID3D11Texture2D),(void **)&DXTex2D));
                    SharedHandle = Para.SharedHandle;
                }
                else
                {
                    D3D11_TEXTURE2D_DESC td;
                    sClear(td);
                    td.Width = Para.SizeX;
                    td.Height = Para.SizeY;
                    td.MipLevels = Para.Mipmaps;
                    td.ArraySize = Para.SizeA ? Para.SizeA : 1;
                    td.Format = Format_Create;
                    td.BindFlags = bindflags;
                    td.CPUAccessFlags = cpuflags;
                    td.MiscFlags = miscflags;
                    td.Usage = usage;
                    td.SampleDesc.Count = sMax(1,Para.MSCount);
                    td.SampleDesc.Quality = 0;
                    if(Para.Flags & sRES_SharedHandle)
                        td.MiscFlags |= D3D11_RESOURCE_MISC_SHARED;
                    if(Para.MSCount>=2)
                    {
                        td.SampleDesc.Quality = Para.MSQuality;
                        if(Para.MSQuality==-1)
                        {
                            td.SampleDesc.Quality = adapter->GetMultisampleQuality(Para,Para.MSCount)-1;
                            if(td.SampleDesc.Quality==-1)
                            {
                                td.SampleDesc.Count = 1;
                                td.SampleDesc.Quality = 0;
                            }
                        }
                    }
                    DXErr(adapter->DXDev->CreateTexture2D(&td,srdp,&DXTex2D));

                    if(Para.Flags & sRES_SharedHandle)
                    {
                        IDXGIResource *res;
                        DXErr(DXTex2D->QueryInterface(__uuidof(IDXGIResource),(void **)&res));
                        HANDLE handle;
                        DXErr(res->GetSharedHandle(&handle));
                        SharedHandle = (uint) handle;
                        sRelease(res);
                    }

                    if(staging)
                    {
                        td.Usage = D3D11_USAGE_STAGING;
                        td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                        td.BindFlags = 0;
                        DXErr(adapter->DXDev->CreateTexture2D(&td,0,&STTex2D));  
                    }
                }
            }
            else
            {
                D3D11_TEXTURE3D_DESC td;
                sClear(td);
                td.Width = Para.SizeX;
                td.Height = Para.SizeY;
                td.Depth = Para.SizeZ;
                td.MipLevels = Para.Mipmaps;
                td.Format = Format_Create;
                td.BindFlags = bindflags;
                td.CPUAccessFlags = cpuflags;
                td.MiscFlags = miscflags;
                td.Usage = usage;

                DXErr(adapter->DXDev->CreateTexture3D(&td,srdp,&DXTex3D));

                if(staging)
                {
                    td.Usage = D3D11_USAGE_STAGING;
                    td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                    td.BindFlags = 0;
                    DXErr(adapter->DXDev->CreateTexture3D(&td,0,&STTex3D));  
                }
            }
        }
        else
        {
            D3D11_BUFFER_DESC bd;
            sClear(bd);

            bd.ByteWidth = (UINT)totalsize;
            bd.Usage = usage;
            bd.BindFlags = bindflags;
            bd.CPUAccessFlags = cpuflags;
            bd.MiscFlags = miscflags;
            bd.StructureByteStride = para.BitsPerElement / 8;

            DXErr(adapter->DXDev->CreateBuffer(&bd,srdp,&DXBuffer));  

            if(staging)
            {
                bd.Usage = D3D11_USAGE_STAGING;
                bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                DXErr(adapter->DXDev->CreateBuffer(&bd,0,&STBuffer));  
            }
        }
    }
}

sResource::~sResource()
{
    sRelease(ShaderResourceView);
    sRelease(RenderTargetView);
    sRelease(DepthStencilView);
    sRelease(UnorderedAccessView);
    sRelease(DXResource);
    sRelease(STResource);
}


void sResourcePrivate::PrivateSetInternalResource(ID3D11Resource *res) 
{
    sRelease(DXResource);
    DXResource = res; 
    res->AddRef();
}

ID3D11ShaderResourceView *sResourcePrivate::PrivateSRView()
{
    if(ShaderResourceView==0)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC vd;
        sClear(vd);
        vd.Format = Format_Shader;
        if(Para.Mode & sRM_Texture)
        {
            if(Para.SizeZ>0)
            {
                vd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
            }
            else if(Para.SizeY>0)
            {
                vd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            }
            else
            {
                vd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
            }
            vd.Texture2D.MipLevels = Para.Mipmaps;
            vd.Texture2D.MostDetailedMip = 0;
        }
        else
        {
            D3D11_BUFFER_DESC desc;
            ((ID3D11Buffer *)DXResource)->GetDesc(&desc);
            if(Para.Mode & (sRM_Structured | sRM_Counter | sRM_Append | sRM_Raw))
            {
                vd.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
                vd.Format = DXGI_FORMAT_UNKNOWN;
                vd.BufferEx.FirstElement = 0;
                vd.BufferEx.NumElements = Para.SizeX;
                vd.BufferEx.Flags = (Para.Mode & sRM_Raw) ? D3D11_BUFFEREX_SRV_FLAG_RAW : 0;
            }
            else
            {
                vd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                vd.Format = Format_Shader!=DXGI_FORMAT_UNKNOWN ? Format_Shader : DXGI_FORMAT_R32G32B32A32_FLOAT;
                vd.Buffer.FirstElement = 0;
                vd.Buffer.NumElements = Para.SizeX;
            }
        }
        DXErr(Adapter->DXDev->CreateShaderResourceView(DXResource,&vd,&ShaderResourceView))
    }
    return ShaderResourceView;
}

ID3D11UnorderedAccessView *sResourcePrivate::PrivateUAView()
{
    if(UnorderedAccessView==0)
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC vd;
        sClear(vd);
        vd.Format = Format_Shader;
        if(Para.Mode & sRM_Texture)
        {
            if(Para.SizeZ>0)
            {
                vd.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
                vd.Texture3D.MipSlice = 0;
                vd.Texture3D.FirstWSlice = 0;
                vd.Texture3D.WSize = Para.SizeZ;
            }
            else if(Para.SizeY>0)
            {
                if(Para.SizeA>0)
                {
                    vd.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
                    vd.Texture2DArray.MipSlice = 0;
                    vd.Texture2DArray.ArraySize = Para.SizeA;
                    vd.Texture2DArray.FirstArraySlice = 0;
                }
                else
                {
                    vd.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                    vd.Texture2D.MipSlice = 0;
                }
            }
            else
            {
                if(Para.SizeA>0)
                {
                    vd.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1DARRAY;
                    vd.Texture1DArray.MipSlice = 0;
                    vd.Texture1DArray.ArraySize = Para.SizeA;
                    vd.Texture1DArray.FirstArraySlice = 0;
                }
                else
                {
                    vd.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
                    vd.Texture1D.MipSlice = 0;
                }
            }
        }
        else
        {
            vd.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
            vd.Buffer.FirstElement = 0;
            vd.Buffer.NumElements = Para.SizeX;
            vd.Buffer.Flags = 0;
            if(Para.Mode & sRM_Raw) vd.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_RAW;
            if(Para.Mode & sRM_Append) vd.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_APPEND;
            if(Para.Mode & sRM_Counter) vd.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_COUNTER;
        }
        DXErr(Adapter->DXDev->CreateUnorderedAccessView(DXResource,&vd,&UnorderedAccessView))

    }
    return UnorderedAccessView;
}

ID3D11RenderTargetView *sResourcePrivate::PrivateRTView()
{
    if(!RenderTargetView)
    {
        D3D11_RENDER_TARGET_VIEW_DESC desc;
        sClear(desc);
        desc.Format = Format_Depth;
        if(Para.MSCount>=2)
        {
            desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
        }
        else
        {
            desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipSlice = 0;
        }
        DXErr(Adapter->DXDev->CreateRenderTargetView(DXResource,&desc,&RenderTargetView));
    }
    return RenderTargetView;
}

ID3D11DepthStencilView *sResourcePrivate::PrivateDSView()
{
    if(!DepthStencilView)
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC desc;
        sClear(desc);
        desc.Format = Format_Depth;
        desc.Flags = 0;
        if(Para.MSCount>=2)
        {
            desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        }
        else
        {
            desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipSlice = 0;
        }
        DXErr(Adapter->DXDev->CreateDepthStencilView(DXResource,&desc,&DepthStencilView));
    }
    return DepthStencilView;
}

void sResource::MapBuffer0(sContext *ctx,void **data,sResourceMapMode mode)
{
    MapTexture0(ctx,0,0,data,0,0,mode);
}

void sResource::MapTexture0(sContext *ctx,int mipmap,int index,void **data,int *pitch,int *pitch2,sResourceMapMode mode_)
{
    sASSERT(!Loading);
    Loading = 1;

    D3D11_MAP mode = D3D11_MAP(0);
    ID3D11Resource *res = 0;

    int usage = (Para.Mode & sRU_Mask);
/*
    if(usage==sRU_StageWrite)
    {
        mode = D3D11_MAP_WRITE;
        res = STResource;
    }
    else*/ if(usage==sRU_MapWrite)
    {
        switch(mode_)
        {
        case sRMM_NoOverwrite: mode = D3D11_MAP_WRITE_NO_OVERWRITE; sASSERT(!(usage & sRM_Texture)); break;                       // can't do no-overwrite, also can't replace with discard.
        case sRMM_Discard: mode = D3D11_MAP_WRITE_DISCARD; break;
        case sRMM_Sync: mode = D3D11_MAP_WRITE_DISCARD; break;          // can't write without discarding!
        default: sASSERT0(); break;
        }
        res = DXResource;
    }
    else if(usage==sRU_SystemMem)
    {
        sASSERT(mode_==sRMM_Sync);
        mode = D3D11_MAP_READ;
        res = DXResource;
    }
/*    else if(usage==sRU_MapWrite2)
    {
        mode = D3D11_MAP_WRITE;
        res = DXResource;
    }*/
    else
    {
        sASSERT0();
    }

    D3D11_MAPPED_SUBRESOURCE map;
    //  DXCtx->ClearState();
    DXErr(ctx->DXCtx->Map(res,mipmap+index*Para.Mipmaps,mode,0,&map));
    *data = (uint8 *) map.pData;
    if(pitch)
        *pitch = map.RowPitch;
    if(pitch2)
        *pitch2 = map.DepthPitch;
}

void sResource::Unmap(sContext *ctx)
{
    sASSERT(Loading);
    Loading = 0;

    int usage = (Para.Mode & sRU_Mask);

/*    if(usage==sRU_StageWrite)
    {
        ctx->DXCtx->Unmap(STResource,0);
        ctx->DXCtx->CopySubresourceRegion(DXResource,0,0,0,0,STResource,0,0);
    }
    else*/ if(usage==sRU_MapWrite)
    {
        ctx->DXCtx->Unmap(DXResource,0);
    }
    else if(usage==sRU_SystemMem/* || usage==sRU_MapWrite2*/)
    {
        ctx->DXCtx->Unmap(DXResource,0);
    }
    else
    {
        sASSERT0();
    }
}

void sResource::UpdateBuffer(void *data,int startbyte,int bytesize)
{
    D3D11_BOX box;

    sASSERT((Para.Mode & sRU_Mask)==sRU_Update);

    box.left =  startbyte;
    box.right = startbyte+bytesize;
    box.top = 0;
    box.bottom = 1;
    box.front = 0;
    box.back = 1;

    Adapter->ImmediateContext->DXCtx->UpdateSubresource(DXResource,0,&box,data,Para.BitsPerElement/8,0);
}

void sResource::UpdateTexture(void *data,int miplevel,int arrayindex)
{
    D3D11_BOX box;

    sASSERT((Para.Mode & sRU_Mask)==sRU_Update);

    box.left = 0;
    box.right = Para.SizeX;
    box.top = 0;
    box.bottom = sMax(Para.SizeY,1);
    box.front = 0;
    box.back = sMax(Para.SizeZ,1);

    uint p0 = (Para.SizeX>>miplevel)*Para.BitsPerElement/8;
    uint p1 = p0 * (Para.SizeY>>miplevel);

    uint subres = D3D11CalcSubresource(miplevel,arrayindex,Para.Mipmaps);
    Adapter->ImmediateContext->DXCtx->UpdateSubresource(DXResource,subres,&box,data,p0,p1);
}

void sResource::ReadTexture(void *data,uptr size)
{
    sFatal("sResource::ReadTexture - not implemented");
}

/****************************************************************************/

// this is REALLY slow, for debugging only!

void sResource::DebugDumpBuffer(int columns,const char *type,int max,const char *msg)
{
    ID3D11Buffer *buffer = 0;

    D3D11_BUFFER_DESC bd;
    sClear(bd);

    int bytesize = (Para.BitsPerElement/ 8) * Para.SizeX;

    bd.ByteWidth = bytesize;
    bd.Usage = D3D11_USAGE_STAGING;
    bd.BindFlags = 0;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    if(Para.Mode & sRM_Structured)
        bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bd.StructureByteStride = Para.BitsPerElement / 8;

    bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    DXErr(Adapter->DXDev->CreateBuffer(&bd,0,&buffer));

    Adapter->DXCtx->CopyResource(buffer,GetDXBuffer());


    D3D11_MAPPED_SUBRESOURCE ms;
    DXErr(Adapter->DXCtx->Map(buffer,0,D3D11_MAP_READ,0,&ms));
    int mode = 0;
    if(sCmpStringI(type,"int")) mode = 1;
    if(sCmpStringI(type,"uint")) mode = 2;
    if(sCmpStringI(type,"hex")) mode = 3;

    void *data = ms.pData;

    sTextBuffer tb;
    if(msg)
        tb.PrintF("%s\n",msg);
    if(max==0)
        max = bytesize/4;
    else
        max = sMin(bytesize/4,max);
    for(int i=0;i<max;i+=columns)
    {
        tb.PrintF("%08x:",i);
        switch(mode)
        {
        case 0:
            for(int j=0;j<columns && i+j<max;j++)
                tb.PrintF(" %9f",((float *)data)[i+j]);
            break;
        case 1:
            for(int j=0;j<columns && i+j<max;j++)
                tb.PrintF(" %7d",((int *)data)[i+j]);
            break;
        case 2:
            for(int j=0;j<columns && i+j<max;j++)
                tb.PrintF(" %7d",((uint *)data)[i+j]);
            break;
        case 3:
            for(int j=0;j<columns && i+j<max;j++)
                tb.PrintF(" %08x",((uint *)data)[i+j]);
            break;
        }
        tb.PrintF("\n");
    }

    Adapter->DXCtx->Unmap(buffer,0);
    sRelease(buffer);

    sDPrint(tb.Get());
}


/****************************************************************************/
/***                                                                      ***/
/***   Vertex Formats                                                     ***/
/***                                                                      ***/
/****************************************************************************/

sVertexFormat::sVertexFormat(sAdapter *adapter,const uint *desc_,int count_,const sChecksumMD5 &hash_)
{
    Hash = hash_;
    Count = count_;
    Desc = desc_;
    AvailMask = 0;
    Streams = 0;
    Adapter = adapter;

    // create vertex declarator 
    D3D11_INPUT_ELEMENT_DESC decl[32];
    int b[sGfxMaxStream];
    int stream;

    for(int i=0;i<sGfxMaxStream;i++)
        b[i] = 0;

    // create declaration

    int count = 0;
    for(int i=0;Desc[i];i++)
    {
        stream = (Desc[i]&sVF_StreamMask)>>sVF_StreamShift;

        decl[i].InputSlot = stream;
        decl[i].AlignedByteOffset = b[stream];
        decl[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        decl[i].InstanceDataStepRate = 0;
        if(Desc[i]&sVF_InstanceData)
        {
            decl[i].InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
            decl[i].InstanceDataStepRate = 1;
        }

        sASSERT(count<31);

        switch(Desc[i]&sVF_UseMask)
        {
        case sVF_Nop:   break;
        case sVF_Position:    decl[count].SemanticName = "POSITION";      decl[count].SemanticIndex = 0; break;
        case sVF_Normal:      decl[count].SemanticName = "NORMAL";        decl[count].SemanticIndex = 0; break;
        case sVF_Tangent:     decl[count].SemanticName = "TANGENT";       decl[count].SemanticIndex = 0; break;
        case sVF_BoneIndex:   decl[count].SemanticName = "BLENDINDICES";  decl[count].SemanticIndex = 0; break;
        case sVF_BoneWeight:  decl[count].SemanticName = "BLENDWEIGHT";   decl[count].SemanticIndex = 0; break;
        case sVF_Binormal:    decl[count].SemanticName = "BINORMAL";      decl[count].SemanticIndex = 0; break;
        case sVF_Color0:      decl[count].SemanticName = "COLOR";         decl[count].SemanticIndex = 0; break;
        case sVF_Color1:      decl[count].SemanticName = "COLOR";         decl[count].SemanticIndex = 1; break;
        case sVF_Color2:      decl[count].SemanticName = "COLOR";         decl[count].SemanticIndex = 2; break;
        case sVF_Color3:      decl[count].SemanticName = "COLOR";         decl[count].SemanticIndex = 3; break;
        case sVF_UV0:         decl[count].SemanticName = "TEXCOORD";      decl[count].SemanticIndex = 0; break;
        case sVF_UV1:         decl[count].SemanticName = "TEXCOORD";      decl[count].SemanticIndex = 1; break;
        case sVF_UV2:         decl[count].SemanticName = "TEXCOORD";      decl[count].SemanticIndex = 2; break;
        case sVF_UV3:         decl[count].SemanticName = "TEXCOORD";      decl[count].SemanticIndex = 3; break;
        case sVF_UV4:         decl[count].SemanticName = "TEXCOORD";      decl[count].SemanticIndex = 4; break;
        case sVF_UV5:         decl[count].SemanticName = "TEXCOORD";      decl[count].SemanticIndex = 5; break;
        case sVF_UV6:         decl[count].SemanticName = "TEXCOORD";      decl[count].SemanticIndex = 6; break;
        case sVF_UV7:         decl[count].SemanticName = "TEXCOORD";      decl[count].SemanticIndex = 7; break;
        default: sASSERT0();
        }

        switch(Desc[i]&sVF_TypeMask)
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
        case sVF_I1:  decl[count].Format = DXGI_FORMAT_R32_UINT;            b[stream]+=1*4;  break;

        default: sASSERT0();
        }

        AvailMask |= 1 << (Desc[i]&sVF_UseMask);
        Streams = sMax(Streams,stream+1);
        count++;
    }

    for(int i=0;i<sGfxMaxStream;i++)
        VertexSize[i] = b[i];

    // create a dummy shader that will provide the input layout signature (i hate this)

    sTextLog tb;

    tb.Print("void main(\n");
    for(int i=0;Desc[i];i++)
    {
        tb.Print("  in ");
        switch(Desc[i]&sVF_TypeMask)
        {
        case sVF_F2:  tb.Print("float2 "); break;
        case sVF_F3:  tb.Print("float3 "); break;
        case sVF_F4:  tb.Print("float4 "); break;
        case sVF_I4:  tb.Print("uint4 "); break;
        case sVF_C4:  tb.Print("float4 "); break;
        case sVF_S2:  tb.Print("float2 "); break;
        case sVF_S4:  tb.Print("float4 "); break;
        case sVF_H2:  tb.Print("float2 "); break;
        case sVF_H4:  tb.Print("float4 "); break;
        case sVF_F1:  tb.Print("float1 "); break;
        case sVF_I1:  tb.Print("uint "); break;
        }
        tb.PrintF("_%d : ",i);
        switch(Desc[i]&sVF_UseMask)
        {
        case sVF_Position:    tb.Print("POSITION"); break;
        case sVF_Normal:      tb.Print("NORMAL"); break;
        case sVF_Tangent:     tb.Print("TANGENT"); break;
        case sVF_BoneIndex:   tb.Print("BLENDINDICES"); break;
        case sVF_BoneWeight:  tb.Print("BLENDWEIGHT"); break;
        case sVF_Binormal:    tb.Print("BINORMAL"); break;
        case sVF_Color0:      tb.Print("COLOR0"); break;
        case sVF_Color1:      tb.Print("COLOR1"); break;
        case sVF_Color2:      tb.Print("COLOR2"); break;
        case sVF_Color3:      tb.Print("COLOR3"); break;
        case sVF_UV0:         tb.Print("TEXCOORD0"); break;
        case sVF_UV1:         tb.Print("TEXCOORD1"); break;
        case sVF_UV2:         tb.Print("TEXCOORD2"); break;
        case sVF_UV3:         tb.Print("TEXCOORD3"); break;
        case sVF_UV4:         tb.Print("TEXCOORD4"); break;
        case sVF_UV5:         tb.Print("TEXCOORD5"); break;
        case sVF_UV6:         tb.Print("TEXCOORD6"); break;
        case sVF_UV7:         tb.Print("TEXCOORD7"); break;
        }
        tb.Print(",\n");
    }
    tb.Print("  out float4 pos:SV_POSITION\n");
    tb.Print("){ pos=0; }\n");

    sShaderBinary *bin = sCompileShaderDX(sST_Vertex,"vs_4_0",tb.Get(),0);

    // create

    DXErr(adapter->DXDev->CreateInputLayout(decl,count,bin->Data,bin->Size,&Layout));
    delete bin;

}

sVertexFormat::~sVertexFormat()
{
    delete[] Desc;
    sRelease(Layout);
}

int sVertexFormat::GetStreamCount()
{
    return Streams;
}

int sVertexFormat::GetStreamSize(int stream)
{
    return VertexSize[stream];
}

/****************************************************************************/
/***                                                                      ***/
/***   Geometry                                                           ***/
/***                                                                      ***/
/****************************************************************************/


void sGeometry::PrivateInit()
{
    DXIB = 0;
    IBFormat = DXGI_FORMAT_UNKNOWN;
    HasDynamicBuffers = 0;
    sClear(DXVB);
    if(Index)
    {
        DXIB = Index->GetDXBuffer();
        if(Index->PrivateIsDynamic())
            HasDynamicBuffers = 1;
    }
    for(int i=0;i<sGfxMaxStream;i++)
    {
        if(Vertex[i])
        {
            DXVB[i] = Vertex[i]->GetDXBuffer();
            if(Vertex[i]->PrivateIsDynamic())
                HasDynamicBuffers = 1;
        }
    }
    if(Mode & sGMF_Index16)
        IBFormat = DXGI_FORMAT_R16_UINT;
    if(Mode & sGMF_Index32)
        IBFormat = DXGI_FORMAT_R32_UINT;
  switch(Mode & sGMP_Mask)
  {
  case sGMP_Points:   Topology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST; break;
  case sGMP_Lines:    Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST; break;
  case sGMP_Tris:     Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
      //    case sGMP_Quads:    Topology = D3D11_PRIMITIVE_TOPOLOGY_LIST; break;
  case sGMP_LinesAdj: Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST_ADJ; break;
  case sGMP_TrisAdj:  Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ; break;
  case sGMP_ControlPoints: 
      Topology = D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST-1+(Mode&sGM_ControlMask);
      break;
  default: sASSERT0(); break;
  }

}

void sGeometry::PrivateExit()
{
}

/****************************************************************************/
/***                                                                      ***/
/***   Shaders                                                            ***/
/***                                                                      ***/
/****************************************************************************/

sShaderBinary *sCompileShader(sShaderTypeEnum type,const char *profile,const char *source,sTextLog *errors)
{
    return sCompileShaderDX(type,profile,source,errors);
}

sShader::sShader(sAdapter *adapter,sShaderBinary *bin)
{
    Hash = bin->Hash;
    Type = bin->Type;
    Adapter = adapter;

    switch(Type)
    {
    case sST_Vertex:   DXErr(adapter->DXDev->CreateVertexShader  (bin->Data,bin->Size,0,&VS)); break;
    case sST_Hull:     DXErr(adapter->DXDev->CreateHullShader    (bin->Data,bin->Size,0,&HS)); break;
    case sST_Domain:   DXErr(adapter->DXDev->CreateDomainShader  (bin->Data,bin->Size,0,&DS)); break;
    case sST_Geometry: DXErr(adapter->DXDev->CreateGeometryShader(bin->Data,bin->Size,0,&GS)); break;
    case sST_Pixel:    DXErr(adapter->DXDev->CreatePixelShader   (bin->Data,bin->Size,0,&PS)); break;
    case sST_Compute:  DXErr(adapter->DXDev->CreateComputeShader (bin->Data,bin->Size,0,&CS)); break;
    default: sASSERT0();
    }
}

sShader::~sShader()
{
    sRelease(VS);
}

/****************************************************************************/
/***                                                                      ***/
/***   Constant Buffers                                                   ***/
/***                                                                      ***/
/****************************************************************************/

sCBufferBase::sCBufferBase(sAdapter *adapter,int size,sShaderTypeEnum type,int slot)
{
    Size = size;
    Type = 1<<type;
    Slot = slot;
    DXBuffer = 0;
    Loading = 0;
    Adapter = adapter;

    D3D11_BUFFER_DESC bd;

    sClear(bd);
    bd.ByteWidth = size;
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    DXErr(adapter->DXDev->CreateBuffer(&bd,0,&DXBuffer));
}

sCBufferBase::sCBufferBase(sAdapter *adapter,int size,int typesmask,int slot)
{
    Size = size;
    Type = typesmask;
    Slot = slot;
    DXBuffer = 0;
    Loading = 0;
    Adapter = adapter;

    D3D11_BUFFER_DESC bd;

    sClear(bd);
    bd.ByteWidth = size;
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    DXErr(adapter->DXDev->CreateBuffer(&bd,0,&DXBuffer));
}

sCBufferBase::~sCBufferBase()
{
    sRelease(DXBuffer);
}

void sCBufferBase::Map(sContext *ctx,void **ptr)
{
    sASSERT(!Loading);
    Loading = 1;

    D3D11_MAPPED_SUBRESOURCE mr;
    //  ctx->DXCtx->ClearState();
    DXErr(ctx->DXCtx->Map(DXBuffer,0,D3D11_MAP_WRITE_DISCARD,0,&mr));
    *ptr = mr.pData;
}

void sCBufferBase::Unmap(sContext *ctx)
{
    sASSERT(Loading);
    Loading = 0;
    ctx->DXCtx->Unmap(DXBuffer,0);
}

/****************************************************************************/
/***                                                                      ***/
/***   Renderstates                                                       ***/
/***                                                                      ***/
/****************************************************************************/


sRenderState::sRenderState(sAdapter *adapter,const sRenderStatePara &para,const sChecksumMD5 &hash_)
{
    Hash = hash_;
    Adapter = adapter;
    SampleMask = para.SampleMask;
    StencilRef = para.StencilRef;

    BlendState = 0;
    DepthState = 0;
    RasterState = 0;

    D3D11_BLEND_DESC bd;
    D3D11_DEPTH_STENCIL_DESC dd;
    D3D11_RASTERIZER_DESC rd;

    sClear(bd);
    sClear(dd);
    sClear(rd);

    if(para.Flags & sMTRL_DepthRead) dd.DepthEnable = 1;
    if(para.Flags & sMTRL_DepthWrite) dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    if(para.Flags & sMTRL_Stencil) dd.StencilEnable = 1;
    rd.CullMode = D3D11_CULL_NONE;
    if((para.Flags & sMTRL_CullMask)==sMTRL_CullOn) rd.CullMode = D3D11_CULL_BACK;
    if((para.Flags & sMTRL_CullMask)==sMTRL_CullInv) rd.CullMode = D3D11_CULL_FRONT;

    if(!(para.Flags & sMTRL_ScissorDisable)) rd.ScissorEnable = 1;
    if(!(para.Flags & sMTRL_MultisampleDisable)) rd.MultisampleEnable = 1;
    if(para.Flags & sMTRL_AALine) rd.AntialiasedLineEnable = 1;
    if(para.Flags & sMTRL_AlphaToCoverage) bd.AlphaToCoverageEnable = 1;

    rd.FillMode = D3D11_FILL_SOLID;
    rd.DepthClipEnable = 1;
    int maxblend = 1;
    if(para.Flags & sMTRL_IndividualBlend)
    {
        bd.IndependentBlendEnable = 1;
        maxblend = sGfxMaxTargets;
    }
    for(int i=0;i<maxblend;i++)
    {

        int bc = para.BlendColor[i];
        int ba = para.BlendAlpha[i];
        if(bc!=0 || ba!=0)
        {
            if(bc==0) bc = sMBS_1 | sMBO_Add | sMBD_0;
            if(ba==0) ba = sMBS_1 | sMBO_Add | sMBD_0;
            bd.RenderTarget[i].BlendEnable = 1;
            bd.RenderTarget[i].SrcBlend  = D3D11_BLEND((bc>>0)&63);
            bd.RenderTarget[i].BlendOp   = D3D11_BLEND_OP((bc>>12)&15);
            bd.RenderTarget[i].DestBlend = D3D11_BLEND((bc>>6)&63);
            bd.RenderTarget[i].SrcBlendAlpha  = D3D11_BLEND((ba>>0)&63);
            bd.RenderTarget[i].BlendOpAlpha   = D3D11_BLEND_OP((bc>>12)&15);
            bd.RenderTarget[i].DestBlendAlpha = D3D11_BLEND((ba>>6)&63);
        }
        bd.RenderTarget[i].RenderTargetWriteMask = para.TargetWriteMask[i]^15;
    }

    dd.DepthFunc = D3D11_COMPARISON_FUNC(para.DepthFunc);
    if(dd.StencilEnable)
    {
        dd.StencilReadMask = para.StencilReadMask;
        dd.StencilWriteMask = para.StencilWriteMask;
        dd.FrontFace.StencilFailOp = D3D11_STENCIL_OP(para.FrontStencilFailOp);
        dd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP(para.FrontStencilDepthFailOp);
        dd.FrontFace.StencilPassOp = D3D11_STENCIL_OP(para.FrontStencilPassOp);
        dd.FrontFace.StencilFunc = D3D11_COMPARISON_FUNC(para.FrontStencilFunc);
        dd.BackFace.StencilFailOp = D3D11_STENCIL_OP(para.BackStencilFailOp);
        dd.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP(para.BackStencilDepthFailOp);
        dd.BackFace.StencilPassOp = D3D11_STENCIL_OP(para.BackStencilPassOp);
        dd.BackFace.StencilFunc = D3D11_COMPARISON_FUNC(para.BackStencilFunc);
    }

    rd.DepthBias =para.DepthBias;
    rd.DepthBiasClamp = para.DepthBiasClamp;
    rd.SlopeScaledDepthBias = para.DepthBiasSlope;

    DXErr(adapter->DXDev->CreateBlendState(&bd,&BlendState));
    DXErr(adapter->DXDev->CreateDepthStencilState(&dd,&DepthState));
    DXErr(adapter->DXDev->CreateRasterizerState(&rd,&RasterState));
}

sRenderState::~sRenderState()
{
    sRelease(BlendState);
    sRelease(DepthState);
    sRelease(RasterState);
}

/****************************************************************************/
/***                                                                      ***/
/***   Sampler States                                                     ***/
/***                                                                      ***/
/****************************************************************************/

sSamplerState::sSamplerState(sAdapter *adapter,const sSamplerStatePara &para,const sChecksumMD5 &hash_)
{
    Hash = hash_;
    Adapter = adapter;

    D3D11_SAMPLER_DESC sd;
    sClear(sd);
    SamplerState = 0;

    D3D11_FILTER filters[32] = 
    {
        D3D11_FILTER_MIN_MAG_MIP_POINT,
        D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT,
        D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
        D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,

        D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR,
        D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR,
        D3D11_FILTER_MIN_MAG_MIP_LINEAR,

        D3D11_FILTER_ANISOTROPIC,D3D11_FILTER_ANISOTROPIC,
        D3D11_FILTER_ANISOTROPIC,D3D11_FILTER_ANISOTROPIC,
        D3D11_FILTER_ANISOTROPIC,D3D11_FILTER_ANISOTROPIC,
        D3D11_FILTER_ANISOTROPIC,D3D11_FILTER_ANISOTROPIC,


        D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
        D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
        D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
        D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,

        D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
        D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
        D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,

        D3D11_FILTER_COMPARISON_ANISOTROPIC,D3D11_FILTER_COMPARISON_ANISOTROPIC,
        D3D11_FILTER_COMPARISON_ANISOTROPIC,D3D11_FILTER_COMPARISON_ANISOTROPIC,
        D3D11_FILTER_COMPARISON_ANISOTROPIC,D3D11_FILTER_COMPARISON_ANISOTROPIC,
        D3D11_FILTER_COMPARISON_ANISOTROPIC,D3D11_FILTER_COMPARISON_ANISOTROPIC,

    };
    D3D11_TEXTURE_ADDRESS_MODE addr[16] =
    {
        D3D11_TEXTURE_ADDRESS_WRAP,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_BORDER,
        D3D11_TEXTURE_ADDRESS_MIRROR,
        D3D11_TEXTURE_ADDRESS_MIRROR_ONCE,
    };


    sd.Filter = filters[para.Flags & sTF_FilterMask];
    sd.AddressU = addr[(para.Flags&sTF_UMask)>>16];
    sd.AddressV = addr[(para.Flags&sTF_VMask)>>20];
    sd.AddressW = addr[(para.Flags&sTF_WMask)>>24];
    sd.MipLODBias = para.MipLodBias;
    sd.MaxAnisotropy = para.MaxAnisotropy;
    sd.ComparisonFunc = D3D11_COMPARISON_FUNC(para.CmpFunc);
    sd.BorderColor[0] = para.Border.x;
    sd.BorderColor[1] = para.Border.y;
    sd.BorderColor[2] = para.Border.z;
    sd.BorderColor[3] = para.Border.w;
    sd.MinLOD = para.MinLod;
    sd.MaxLOD = para.MaxLod;
    DXErr(adapter->DXDev->CreateSamplerState(&sd,&SamplerState));
}

sSamplerState::~sSamplerState()
{
    sRelease(SamplerState);
}

/****************************************************************************/
/***                                                                      ***/
/***   Material                                                           ***/
/***                                                                      ***/
/****************************************************************************/

void sMaterial::PrivateSetBase(sAdapter *adapter)
{
}


void sMaterial::SetTexture(sShaderTypeEnum shader,int index,sResource *tex,sSamplerState *sam,int slicestart,int slicecount)
{
    sASSERT(slicestart==0);
    sASSERT(slicecount==1);
    ResView[int(shader)].SetAndGrow(index,tex ? tex->PrivateSRView() : 0);
    Texture[int(shader)].SetAndGrow(index,tex);
    if(sam)
        Sampler[int(shader)].SetAndGrow(index,sam->SamplerState);
}

void sMaterial::UpdateTexture(sShaderTypeEnum shader,int index,sResource *tex,int slicestart,int slicecount)
{
    sASSERT(slicestart==0);
    sASSERT(slicecount==1);
    ResView[int(shader)][index] = tex ? tex->PrivateSRView() : 0;
    Texture[int(shader)][index] = tex;
}

sResource *sMaterial::GetTexture(sShaderTypeEnum shader,int index)
{
    if(Texture[shader].IsIndex(index))
        return Texture[shader][index];
    else 
        return 0;
}

sResource *sMaterial::GetUav(sShaderTypeEnum shader,int index)
{
    if(shader==sST_Pixel && UavPixel.IsIndex(index))
        return UavPixel[index];
    if(shader==sST_Compute && UavCompute.IsIndex(index))
        return UavCompute[index];
    return 0;
}

void sMaterial::SetUav(sShaderTypeEnum shader,int index,sResource *uav)
{
    switch(shader)
    {
    case sST_Pixel:
        UavPixel.SetAndGrow(index,uav);
        UavViewPixel.SetAndGrow(index,uav ? uav->PrivateUAView() : 0);
        break;
    case sST_Compute:
        UavCompute.SetAndGrow(index,uav);
        UavViewCompute.SetAndGrow(index,uav ? uav->PrivateUAView() : 0);
        break;
    }
}

void sMaterial::ClearTextureSamplerUav()
{
    for(int i=0;i<sST_Max;i++)
    {
        Sampler[i].Clear();
        ResView[i].Clear();
    }
    UavViewPixel.Clear();
    UavViewCompute.Clear();
}

/****************************************************************************/
/***                                                                      ***/
/***  Draw it                                                             ***/
/***                                                                      ***/
/****************************************************************************/

void sContext::Draw(const sDrawPara &dp)
{
    static ID3D11UnorderedAccessView *nulluav[sGfxMaxUav];
    static UINT nolls[sGfxMaxUav];
    static ID3D11Buffer *nullbuf[sGfxMaxStream];
    static ID3D11ShaderResourceView *nullsrv[sGfxMaxTexture];

    if(!(dp.Flags & sDF_Compute))
    {
        sUpdateGfxStats(dp);
        sASSERT(Adapter==dp.Mtrl->RenderState->Adapter);
        sASSERT(dp.Geo->Index==0 || Adapter==dp.Geo->Index->Adapter);
        sASSERT(dp.Geo->Vertex[0]==0 || Adapter==dp.Geo->Vertex[0]->Adapter);

        // set material states

        sRenderState *rs = dp.Mtrl->RenderState;
        uint sm = rs->SampleMask;
        sm = (sm<<8) | sm; 
        sm = (sm<<16) | sm; 
        DXCtx->RSSetState(rs->RasterState);
        DXCtx->OMSetBlendState(rs->BlendState,&dp.BlendFactor.x,sm);
        DXCtx->OMSetDepthStencilState(rs->DepthState,rs->StencilRef);

        // vertex input

        DXCtx->IASetInputLayout(dp.Geo->Format->Layout);
        DXCtx->IASetIndexBuffer(dp.Geo->DXIB,dp.Geo->IBFormat,dp.IndexOffset);
        DXCtx->IASetVertexBuffers(0,sGfxMaxStream,dp.Geo->DXVB,(UINT *)dp.Geo->Format->VertexSize,(UINT *)dp.VertexOffset);
        DXCtx->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)(dp.Geo->Topology));
    }

    // constant buffers and shaders

    ID3D11Buffer *c[sST_Max][8];
    sClear(c);
    for(int i=0;i<dp.CBCount;i++)
    {
        sCBufferBase *cb = dp.CBs[i];
        if(cb)
        {
            for(int j=0;j<sST_Max;j++)
                if(cb->Type & (1<<j))
                    c[j][cb->Slot&7] = cb->DXBuffer;
        }
    }
    if(dp.Mtrl->Shaders[sST_Compute])
    {
        sASSERT(Adapter==dp.Mtrl->Shaders[sST_Compute]->Adapter);
        DXCtx->CSSetShader(dp.Mtrl->Shaders[sST_Compute]->CS,0,0);
        DXCtx->CSSetConstantBuffers(0,8,c[sST_Compute]);
    }
    else
    {
        sASSERT(Adapter==dp.Mtrl->Shaders[sST_Vertex]->Adapter);
        DXCtx->VSSetShader(dp.Mtrl->Shaders[sST_Vertex]->VS,0,0);
        DXCtx->VSSetConstantBuffers(0,8,c[sST_Vertex]);
        if(dp.Mtrl->Shaders[sST_Hull] && dp.Mtrl->Shaders[sST_Domain])
        {
            sASSERT(Adapter==dp.Mtrl->Shaders[sST_Hull]->Adapter);
            DXCtx->HSSetShader(dp.Mtrl->Shaders[sST_Hull]->HS,0,0);
            DXCtx->HSSetConstantBuffers(0,8,c[sST_Hull]);
            sASSERT(Adapter==dp.Mtrl->Shaders[sST_Domain]->Adapter);
            DXCtx->DSSetShader(dp.Mtrl->Shaders[sST_Domain]->DS,0,0);
            DXCtx->DSSetConstantBuffers(0,8,c[sST_Domain]);
        }
        else
        {
            DXCtx->HSSetShader(0,0,0);
            DXCtx->DSSetShader(0,0,0);
        }
        if(dp.Mtrl->Shaders[sST_Geometry])
        {
            sASSERT(Adapter==dp.Mtrl->Shaders[sST_Geometry]->Adapter);
            DXCtx->GSSetShader(dp.Mtrl->Shaders[sST_Geometry]->GS,0,0);
            DXCtx->GSSetConstantBuffers(0,8,c[sST_Geometry]);
        }
        else
        {
            DXCtx->GSSetShader(0,0,0);
        }
        sASSERT(Adapter==dp.Mtrl->Shaders[sST_Pixel]->Adapter);
        DXCtx->PSSetShader(dp.Mtrl->Shaders[sST_Pixel]->PS,0,0);
        DXCtx->PSSetConstantBuffers(0,8,c[sST_Pixel]);
    }

    // textures (shader resources)

    if(!dp.Mtrl->ResView[sST_Vertex].IsEmpty())
        DXCtx->VSSetShaderResources(0,dp.Mtrl->ResView[sST_Vertex].GetCount(),dp.Mtrl->ResView[sST_Vertex].GetData());
    if(!dp.Mtrl->ResView[sST_Hull].IsEmpty())
        DXCtx->HSSetShaderResources(0,dp.Mtrl->ResView[sST_Hull].GetCount(),dp.Mtrl->ResView[sST_Hull].GetData());
    if(!dp.Mtrl->ResView[sST_Domain].IsEmpty())
        DXCtx->DSSetShaderResources(0,dp.Mtrl->ResView[sST_Domain].GetCount(),dp.Mtrl->ResView[sST_Domain].GetData());
    if(!dp.Mtrl->ResView[sST_Geometry].IsEmpty())
        DXCtx->GSSetShaderResources(0,dp.Mtrl->ResView[sST_Geometry].GetCount(),dp.Mtrl->ResView[sST_Geometry].GetData());
    if(!dp.Mtrl->ResView[sST_Pixel].IsEmpty())
        DXCtx->PSSetShaderResources(0,dp.Mtrl->ResView[sST_Pixel].GetCount(),dp.Mtrl->ResView[sST_Pixel].GetData());
    if(!dp.Mtrl->ResView[sST_Compute].IsEmpty())
        DXCtx->CSSetShaderResources(0,dp.Mtrl->ResView[sST_Compute].GetCount(),dp.Mtrl->ResView[sST_Compute].GetData());

    if(!dp.Mtrl->Sampler[sST_Vertex].IsEmpty())
        DXCtx->VSSetSamplers(0,dp.Mtrl->Sampler[sST_Vertex].GetCount(),dp.Mtrl->Sampler[sST_Vertex].GetData());
    if(!dp.Mtrl->Sampler[sST_Hull].IsEmpty())
        DXCtx->HSSetSamplers(0,dp.Mtrl->Sampler[sST_Hull].GetCount(),dp.Mtrl->Sampler[sST_Hull].GetData());
    if(!dp.Mtrl->Sampler[sST_Domain].IsEmpty())
        DXCtx->DSSetSamplers(0,dp.Mtrl->Sampler[sST_Domain].GetCount(),dp.Mtrl->Sampler[sST_Domain].GetData());
    if(!dp.Mtrl->Sampler[sST_Geometry].IsEmpty())
        DXCtx->GSSetSamplers(0,dp.Mtrl->Sampler[sST_Geometry].GetCount(),dp.Mtrl->Sampler[sST_Geometry].GetData());
    if(!dp.Mtrl->Sampler[sST_Pixel].IsEmpty())
        DXCtx->PSSetSamplers(0,dp.Mtrl->Sampler[sST_Pixel].GetCount(),dp.Mtrl->Sampler[sST_Pixel].GetData());
    if(!dp.Mtrl->Sampler[sST_Compute].IsEmpty())
        DXCtx->CSSetSamplers(0,dp.Mtrl->Sampler[sST_Compute].GetCount(),dp.Mtrl->Sampler[sST_Compute].GetData());

    //    if(!dp.Mtrl->UavViewPixel.IsEmpty())
    //        DXCtx->PSSetUnorderedAccessViews(0,dp.Mtrl->UavViewPixel.GetCount(),dp.Mtrl->UavViewPixel.GetData());
    if(!dp.Mtrl->UavViewCompute.IsEmpty())
        DXCtx->CSSetUnorderedAccessViews(0,dp.Mtrl->UavViewCompute.GetCount(),dp.Mtrl->UavViewCompute.GetData(),dp.InitialCount);

    // indirect?

    sResource *indirect = 0;
    if(dp.Flags & sDF_Indirect)
    {
        indirect = Adapter->IndirectBuffer;
    }
    if(dp.Flags & sDF_Indirect2)
    {
        indirect = dp.IndirectBuffer;
    }



    // lets kick it!

    if(dp.Flags & sDF_Compute)
    {
        if(indirect)
            DXCtx->DispatchIndirect(indirect->DXBuffer,dp.IndirectBufferOffset);
        else
            DXCtx->Dispatch(dp.SizeX,dp.SizeY,dp.SizeZ);
    }
    else if(dp.Geo->Index==0)
    {
        sASSERT(TargetOn);
        if(dp.Flags & sDF_Instances)
        {
            if(dp.Flags & sDF_Ranges)
            {
                for(int i=0;i<dp.RangeCount;i++)
                    DXCtx->DrawInstanced(dp.Ranges[i].End-dp.Ranges[i].Start,dp.InstanceCount,dp.Geo->VertexOffset+dp.Ranges[i].Start,0);
            }
            else
            {
                if(indirect)
                    DXCtx->DrawInstancedIndirect(indirect->DXBuffer,dp.IndirectBufferOffset);
                else
                    DXCtx->DrawInstanced(dp.Geo->VertexCount,dp.InstanceCount,dp.Geo->VertexOffset,0);
            }
        }
        else
        {
            if(dp.Flags & sDF_Ranges)
            {
                for(int i=0;i<dp.RangeCount;i++)
                    DXCtx->Draw(dp.Ranges[i].End-dp.Ranges[i].Start,dp.Geo->VertexOffset+dp.Ranges[i].Start);
            }
            else
            {
                if(indirect)
                    DXCtx->DrawInstancedIndirect(indirect->DXBuffer,dp.IndirectBufferOffset);
                else
                    DXCtx->Draw(dp.Geo->VertexCount,dp.Geo->VertexOffset);
            }
        }
    }
    else
    {
        sASSERT(TargetOn);
        if(dp.Flags & sDF_Instances)
        {
            if(dp.Flags & sDF_Ranges)
            {
                for(int i=0;i<dp.RangeCount;i++)
                    DXCtx->DrawIndexedInstanced(dp.Ranges[i].End-dp.Ranges[i].Start,dp.InstanceCount,dp.Geo->IndexFirst+dp.Ranges[i].Start,dp.Geo->VertexOffset,0);
            }
            else
            {
                if(indirect)
                    DXCtx->DrawIndexedInstancedIndirect(indirect->DXBuffer,dp.IndirectBufferOffset);
                else
                    DXCtx->DrawIndexedInstanced(dp.Geo->IndexCount,dp.InstanceCount,dp.Geo->IndexFirst,dp.Geo->VertexOffset,0);
            }
        }
        else
        {
            if(dp.Flags & sDF_Ranges)
            {
                for(int i=0;i<dp.RangeCount;i++)
                    DXCtx->DrawIndexed(dp.Ranges[i].End-dp.Ranges[i].Start,dp.Geo->IndexFirst+dp.Ranges[i].Start,dp.Geo->VertexOffset);
            }
            else
            {
                if(indirect)
                    DXCtx->DrawIndexedInstancedIndirect(indirect->DXBuffer,dp.IndirectBufferOffset);
                else
                    DXCtx->DrawIndexed(dp.Geo->IndexCount,dp.Geo->IndexFirst,dp.Geo->VertexOffset);
            }
        }
    }

    // unmap dynamic resources so they can get mapped

    if(dp.Flags & sDF_Compute)
    {
        if(!dp.Mtrl->UavViewCompute.IsEmpty())
            DXCtx->CSSetUnorderedAccessViews(0,sGfxMaxUav,nulluav,nolls);
    }
    else
    {
        DXCtx->IASetIndexBuffer(0,DXGI_FORMAT_UNKNOWN,0);
        DXCtx->IASetVertexBuffers(0,sGfxMaxStream,nullbuf,(UINT *)dp.Geo->Format->VertexSize,(UINT *)dp.VertexOffset);
    }

    if(!dp.Mtrl->ResView[sST_Vertex].IsEmpty())
        DXCtx->VSSetShaderResources(0,dp.Mtrl->ResView[sST_Vertex].GetCount(),nullsrv);
    if(!dp.Mtrl->ResView[sST_Hull].IsEmpty())
        DXCtx->HSSetShaderResources(0,dp.Mtrl->ResView[sST_Hull].GetCount(),nullsrv);
    if(!dp.Mtrl->ResView[sST_Domain].IsEmpty())
        DXCtx->DSSetShaderResources(0,dp.Mtrl->ResView[sST_Domain].GetCount(),nullsrv);
    if(!dp.Mtrl->ResView[sST_Geometry].IsEmpty())
        DXCtx->GSSetShaderResources(0,dp.Mtrl->ResView[sST_Geometry].GetCount(),nullsrv);
    if(!dp.Mtrl->ResView[sST_Pixel].IsEmpty())
        DXCtx->PSSetShaderResources(0,dp.Mtrl->ResView[sST_Pixel].GetCount(),nullsrv);
    if(!dp.Mtrl->ResView[sST_Compute].IsEmpty())
        DXCtx->CSSetShaderResources(0,dp.Mtrl->ResView[sST_Compute].GetCount(),nullsrv);
}

void sContext::FillIndirectBuffer(sDrawPara &dp)
{
    if(dp.IndirectMask==0 && dp.IndirectUavMask==0)
        return;

    // gather shader resources with instance count

    int count = 0;
    int mask = dp.IndirectMask;
    int uavmask = dp.IndirectUavMask;
    sResource *ir[4];
    sClear(ir);
    if(dp.Flags & sDF_Compute)
    {
        for(int i=0;i<sGfxMaxTexture;i++)
        {
            if(mask & (1<<i))
            {
                ir[count++] = dp.Mtrl->GetTexture(sST_Compute,i);
                mask &= ~(1<<i);
                if((mask|uavmask)==0 || count==4)
                    break;
            }
            if(uavmask & (1<<i))
            {
                ir[count++] = dp.Mtrl->GetUav(sST_Compute,i);
                uavmask &= ~(1<<i);
                if((mask|uavmask)==0 || count==4)
                    break;
            }
        }
    }
    else
    {
        for(int i=0;i<sGfxMaxTexture;i++)
        {
            if(mask & (1<<i))
            {
                ir[count++] = dp.Mtrl->GetTexture(sST_Vertex,i);
                mask &= ~(1<<i);
                if(mask==0 || count==4)
                    break;
            }
        }
    }

    if(count==0)
        return;

    // update DrawPara

    if(!(dp.Flags & sDF_Instances))
    {
        dp.Flags |= sDF_Instances;
        dp.InstanceCount = 1;
    }
    dp.Flags |= sDF_Indirect;
    dp.IndirectBufferOffset = Adapter->IndirectIndex;

    // set constant buffer

    sMaterial *mtrl = 0;
    sAdapterPrivate::IndirectCBStruct *cb;

    Adapter->IndirectConstants->Map<sAdapterPrivate::IndirectCBStruct>(&cb);
    cb->counts[0] = 1;
    cb->counts[1] = 1;
    cb->counts[2] = 1;
    cb->counts[3] = 1;
    cb->Index = dp.IndirectBufferOffset/4;
    cb->InstanceAdd = dp.IndirectAdd;
    if(dp.Flags & sDF_Compute)
    {
        mtrl = Adapter->DispatchIndirectMtrl;
        cb->IndexVertexCountPerInstance = 0;
        cb->InstanceCount = 1;
        cb->StartIndexVertexLocation = 0;
        cb->StartInstanceLocation = 0;
        cb->BaseVertexLocation = 0;
        cb->ThreadGroupSize = dp.IndirectThreadGroupSize;
        Adapter->IndirectIndex += 3*4;
    }
    else
    {
        sASSERT(dp.Flags & sDF_Instances);
        sASSERT(!(dp.Flags & sDF_Arrays));
        sASSERT(!(dp.Flags & sDF_Ranges));

        cb->InstanceCount = dp.InstanceCount;
        cb->StartInstanceLocation = 0;
        cb->ThreadGroupSize = 0;
        if(dp.Geo->Index)
        {
            mtrl = Adapter->DrawIndexedInstancedIndirectMtrl;
            cb->IndexVertexCountPerInstance = dp.Geo->IndexCount;
            cb->StartIndexVertexLocation = dp.Geo->IndexFirst;
            cb->BaseVertexLocation = dp.Geo->VertexOffset;
            Adapter->IndirectIndex += 5*4;
        }
        else
        {
            mtrl = Adapter->DrawInstancedIndirectMtrl;
            cb->IndexVertexCountPerInstance = dp.Geo->VertexCount;
            cb->StartIndexVertexLocation = dp.Geo->VertexOffset;
            cb->BaseVertexLocation = 0;
            Adapter->IndirectIndex += 4*4;
        }
    }
    Adapter->IndirectConstants->Unmap();

    sASSERT(Adapter->IndirectIndex <= sAdapterPrivate::MaxDrawIndirectBytes);

    // draw

    for(int i=0;i<count;i++)
        DXCtx->CopyStructureCount(Adapter->IndirectConstants->DXBuffer,i*4,ir[i]->PrivateUAView());

    sDrawPara mydp(1,1,1,mtrl,Adapter->IndirectConstants);
    Adapter->ImmediateContext->Draw(mydp);
}

void sContext::CopyHiddenCounter(sResource *dest,sResource *src,sInt byteoffset)
{
    DXCtx->CopyStructureCount(dest->DXBuffer,byteoffset,src->PrivateUAView());
}

void sContext::CopyHiddenCounter(sCBufferBase *dest,sResource *src,sInt byteoffset)
{
    DXCtx->CopyStructureCount(dest->DXBuffer,byteoffset,src->PrivateUAView());
}

/****************************************************************************/

sDispatchContext::sDispatchContext()
{
    ResourceCount = 0;
    SamplerCount = 0;
    UavCount = 0;
    CBufferCount = 0;
    sClear(Resources);
    sClear(Samplers);
    sClear(Uavs);
    sClear(CBuffers);
    for(int i=0;i<sGfxMaxUav;i++)
        UavHiddens[i] = ~0;
}

sDispatchContext::~sDispatchContext()
{
    // we don't do refcounting for the dx resources,
    // assuming that the dispatchcontext will not be used after releasing resources.
}

void sDispatchContext::SetResource(int slot,sResource *res)
{
    ResourceCount = sMax(slot+1,ResourceCount);
    Resources[slot] = res->PrivateSRView();
}

void sDispatchContext::SetSampler(int slot, sSamplerState *ss)
{
    SamplerCount = sMax(slot+1,SamplerCount);
    Samplers[slot] = ss->SamplerState;
}

void sDispatchContext::SetUav(int slot,sResource *res,uint hidden)
{
    UavCount = sMax(slot+1,UavCount);
    Uavs[slot] = res ? res->PrivateUAView() : 0;
    UavHiddens[slot] = hidden;
}

void sDispatchContext::SetCBuffer(sCBufferBase *cb)
{
    int slot = cb->Slot;
    CBufferCount = sMax(slot+1,CBufferCount);
    CBuffers[slot] = cb->DXBuffer;
}

void sContext::BeginDispatch(sDispatchContext *dp)
{
    if(dp->ResourceCount>0)
        DXCtx->CSSetShaderResources(0,dp->ResourceCount,dp->Resources);
    if(dp->SamplerCount>0)
        DXCtx->CSSetSamplers(0,dp->SamplerCount,dp->Samplers);
    if(dp->UavCount>0)
        DXCtx->CSSetUnorderedAccessViews(0,dp->UavCount,dp->Uavs,dp->UavHiddens);
    if(dp->CBufferCount>0)
        DXCtx->CSSetConstantBuffers(0,dp->CBufferCount,dp->CBuffers);
}

void sContext::Dispatch(sShader *sh,int x,int y,int z)
{
    DXCtx->CSSetShader(sh->CS,0,0);
    DXCtx->Dispatch(x,y,z);
}

void sContext::Dispatch(sShader *sh,int x,int y,int z,const sPerfMonSection &perf,bool debug)
{
    if(debug)
    {
        sGpuPerfMon->Enter(&perf);
        ComputeSync();
    }
    DXCtx->CSSetShader(sh->CS,0,0);
    DXCtx->Dispatch(x,y,z);
    if(debug)
    {
        ComputeSync();
        sGpuPerfMon->Leave(&perf);
    }
}

void sContext::Dispatch(sMaterial *mtrl,int x,int y,int z)
{
    DXCtx->CSSetShader(mtrl->Shaders[sST_Compute]->CS,0,0);
    DXCtx->Dispatch(x,y,z);
}

void sContext::EndDispatch(sDispatchContext *dp)
{
    static ID3D11ShaderResourceView *NullResources[sGfxMaxSampler];
    static ID3D11SamplerState *NullSamplers[sGfxMaxSampler];
    static ID3D11UnorderedAccessView *NullUavs[sGfxMaxUav];
    static uint NullHiddens[sGfxMaxUav];
    static ID3D11Buffer *NullCBuffers[sGfxMaxCBs];

    if(dp->ResourceCount>0)
        DXCtx->CSSetShaderResources(0,dp->ResourceCount,NullResources);
    if(dp->SamplerCount>0)
        DXCtx->CSSetSamplers(0,dp->SamplerCount,NullSamplers);
    if(dp->UavCount>0)
        DXCtx->CSSetUnorderedAccessViews(0,dp->UavCount,NullUavs,NullHiddens);
    if(dp->CBufferCount>0)
        DXCtx->CSSetConstantBuffers(0,dp->CBufferCount,NullCBuffers);
}

void sContext::UnbindCBuffer(sDispatchContext *dp)
{
    static ID3D11Buffer *NullCBuffers[sGfxMaxCBs];
    DXCtx->CSSetConstantBuffers(0,dp->CBufferCount,NullCBuffers);
}

void sContext::RebindCBuffer(sDispatchContext *dp)
{
    DXCtx->CSSetConstantBuffers(0,dp->CBufferCount,dp->CBuffers);
}


void sContext::ClearUav(sResource *r,const uint v[4])
{
    DXCtx->ClearUnorderedAccessViewUint(r->PrivateUAView(),v);
}

void sContext::ClearUav(sResource *r,const float v[4])
{
    DXCtx->ClearUnorderedAccessViewFloat(r->PrivateUAView(),v);
}

void sContext::ComputeSync()
{
    static const uint v[4] = { 0,0,0,0 };
    ClearUav(ComputeSyncUav,v);
}

sResource *sContext::GetComputeSyncUav()
{
    return ComputeSyncUav;
}

/****************************************************************************/
/***                                                                      ***/
/***   Queries                                                            ***/
/***                                                                      ***/
/****************************************************************************/

sGpuQuery::sGpuQuery(sAdapter *adapter,sGpuQueryKind kind)
{
    Kind = kind;
    Adapter = adapter;
    Query = 0;
    D3D11_QUERY_DESC desc;
    desc.MiscFlags = 0;
    switch(Kind)
    {
    case sGQK_Timestamp:
        desc.Query = D3D11_QUERY_TIMESTAMP;
        break;
    case sGQK_Frequency:
        desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
        break;
    case sGQK_Occlusion:
        desc.Query = D3D11_QUERY_OCCLUSION;
        break;
    case sGQK_PipelineStats:
        desc.Query = D3D11_QUERY_PIPELINE_STATISTICS;
        break;
    default:
        sASSERT0();
    }
    DXErr(adapter->DXDev->CreateQuery(&desc,&Query));
}

sGpuQuery::~sGpuQuery()
{
    sRelease(Query);
}

void sGpuQuery::Begin(sContext *ctx)
{
    sASSERT(ctx->Adapter==Adapter);
    switch(Kind)
    {
    case sGQK_Occlusion:
    case sGQK_PipelineStats:
    case sGQK_Frequency:
        ctx->DXCtx->Begin(Query);
        break;
    default:
        sASSERT0();
    }
}

void sGpuQuery::End(sContext *ctx)
{
    sASSERT(ctx->Adapter==Adapter);
    switch(Kind)
    {
    case sGQK_Occlusion:
    case sGQK_PipelineStats:
    case sGQK_Frequency:
        ctx->DXCtx->End(Query);
        break;
    default:
        sASSERT0();
    }
}

void sGpuQuery::Issue(sContext *ctx)
{
    sASSERT(ctx->Adapter==Adapter);
    switch(Kind)
    {
    case sGQK_Timestamp:
        ctx->DXCtx->End(Query);
        break;
    default:
        sASSERT0();
    }
}

bool sGpuQuery::GetData(uint &data)
{
    HRESULT hr = 0;
    switch(Kind)
    {
    case sGQK_Occlusion:
        {
            uint64 d64;
            hr = Adapter->DXCtx->GetData(Query,&d64,sizeof(d64),D3D11_ASYNC_GETDATA_DONOTFLUSH);
            data = d64&0xffffffffULL;
        }
        break;
    default:
        sASSERT0();
    }
    if(hr==S_OK) return 1;
    if(hr==S_FALSE) return 0;
    DXErr(hr);
    return 0;
}

bool sGpuQuery::GetData(uint64 &data)
{
    HRESULT hr = 0;
    switch(Kind)
    {
    case sGQK_Timestamp:
    case sGQK_Occlusion:
        hr = Adapter->DXCtx->GetData(Query,&data,sizeof(data),D3D11_ASYNC_GETDATA_DONOTFLUSH);
        break;
    case sGQK_Frequency:
        {
            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT freq;
            hr = Adapter->DXCtx->GetData(Query,&freq,sizeof(freq),D3D11_ASYNC_GETDATA_DONOTFLUSH);
            data = freq.Frequency;
        }
        break;
    default:
        sASSERT0();
    }
    if(hr==S_OK) return 1;
    if(hr==S_FALSE) return 0;
    DXErr(hr);
    return 0;
}

bool sGpuQuery::GetData(sPipelineStats &data)
{
    HRESULT hr = 0;
    switch(Kind)
    {
    case sGQK_PipelineStats:
        hr = Adapter->DXCtx->GetData(Query,&data,sizeof(data),D3D11_ASYNC_GETDATA_DONOTFLUSH);
        break;
    default:
        sASSERT0();
    }
    if(hr==S_OK) return 1;
    if(hr==S_FALSE) return 0;
    DXErr(hr);
    return 0;
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/
};

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

#if sConfigDebug
#define D3D_DEBUG_INFO
#endif

#undef new
#include <d3d9.h>
#define new sDEFINE_NEW

namespace Altona2 {

Altona2::SwapCallbackType SwapCallback = 0;

/****************************************************************************/


namespace Private
{
    class sDisplayInfoEx : public sDisplayInfo
    {
    public:
        sRect DesktopRect;

        int DXMasterNumber;
        int DXScreenNumber;

#if sD3D9EX
        sDisplayInfoEx(int id,IDirect3D9Ex *d3dex);
#else
        sDisplayInfoEx(int id,IDirect3D9 *d3dex);
#endif
        ~sDisplayInfoEx();
    };

    int EnumLastAdapterOrdinal;
    int EnumCurrentAdapter;

#if sD3D9EX
    IDirect3D9Ex *DX9;
    //  IDirect3DDevice9Ex *DXDev;
#else
    IDirect3D9 *DX9;
    //  IDirect3DDevice9 *DXDev;
#endif

    bool RestartFlag;
    int LostFlag;

    sArray<sDisplayInfoEx *> EnumeratedDisplays;
    sArray<sAdapter *> OpenAdapters;

    sArray<sResource *> Resources;
    sArray<sGpuQuery *> Queries;

} // namepsace

using namespace Private;

/****************************************************************************/

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

void Private::sRestartGfx()
{
    RestartFlag = 1;
}

void Private::Render()
{
    sPreFrameHook.Call();
    {
        if(RestartFlag)
        {
            sLogF("gfx","restart");
            sBeforeResetDeviceHook.Call();
            RestartGfxExit();
            for(auto i : OpenScreens)
            {
                sRect r;
                GetClientRect((HWND)i->ScreenWindow,(RECT *)&r);
                i->Present->BackBufferWidth = i->ScreenMode.SizeX = r.SizeX();
                i->Present->BackBufferHeight = i->ScreenMode.SizeY = r.SizeY();
                HRESULT hr = i->Adapter->DXDev->Reset(i->Present);
                if(hr==D3DERR_DEVICELOST)
                    return;
                DXErr(hr);
            }
            RestartGfxInit();
            sAfterResetDeviceHook.Call();
        }

        if(1)   // this prevent driver from buffering too much into the future.
        {
            if(OpenScreens.GetCount()>0)
            {
                static int LockQueryIndex;
                auto ada = sGetScreen(0)->Adapter;
                HRESULT hr;
                do
                    hr = ada->LockQuery[LockQueryIndex]->GetData(0,0,D3DGETDATA_FLUSH);
                while(hr==S_FALSE);
                ada->LockQuery[LockQueryIndex]->Issue(D3DISSUE_END);
                LockQueryIndex = !LockQueryIndex;
            }
        }

        int failmask = 0;

        for(auto i : OpenAdapters)
            if(i)
                if(FAILED(i->DXDev->BeginScene()))
                    failmask |= 1<<sGetIndex(OpenAdapters,i);
        if(failmask==0)
        {
            sZONE("Render",0xff004000);
            sGPU_ZONE("Render",0xff004000);
            CurrentApp->OnFrame();
            sBeginFrameHook.Call();
            CurrentApp->OnPaint();
            sEndFrameHook.Call();
            if(sGC) sGC->CollectIfTriggered();
        }
        for(auto i : OpenAdapters)
        {
            if(i && !(failmask & (1<<sGetIndex(OpenAdapters,i))))
            {
                DXErr(i->DXDev->EndScene());
            }
        }

        for(auto i : OpenScreens)
        {
            if(!(i->ScreenMode.Flags & sSM_Headless))
            {
                if(SwapCallback)
                {
                    if((*SwapCallback)(i->Adapter->DXDev,i->SwapChain))
                        RestartFlag = 1;
                }
                else
                {
                    HRESULT hr = i->SwapChain->Present(0,0,0,0,0);
                    if(hr==D3DERR_DEVICELOST)
                        RestartFlag = 1;
                    else
                        DXErr(hr);
                }
            }
            else
            {
                HRESULT hr = i->Adapter->DXDev->TestCooperativeLevel();
                switch(hr)
                {
                case D3DERR_DEVICELOST:
                case D3DERR_DEVICENOTRESET:
                    RestartFlag = 1;
                    break;
                default:
                    DXErr(hr);
                    break;
                }
            }
        }

        sUpdateGfxStats();
    }
}

/****************************************************************************/

void Private::RestartGfxInit()
{
    for(auto i : OpenAdapters)
        if(i)
            i->RestartGfxInit();
    for(auto i : OpenScreens)
        i->RestartGfxInit();  

    RestartFlag = 0;

    if(LostFlag)
    {
        for(auto r : Resources)
            r->PrivateAfterReset();
        for(auto q : Queries)
            q->PrivateAfterReset();
    }
}

void Private::RestartGfxExit()
{
    for(auto i : OpenAdapters)
        if(i)
            i->RestartGfxExit();
    for(auto i : OpenScreens)
        i->RestartGfxExit();

    for(auto r : Resources)
        r->PrivateBeforeReset();
    for(auto q : Queries)
        q->PrivateBeforeReset();
    LostFlag = 1;
}

/****************************************************************************/
#if sD3D9EX
sDisplayInfoEx::sDisplayInfoEx(int id,IDirect3D9Ex *d3dex)
#else
sDisplayInfoEx::sDisplayInfoEx(int id,IDirect3D9 *d3dex)
#endif
{
    D3DADAPTER_IDENTIFIER9 adapterid;
    D3DCAPS9 caps;
    HMONITOR mon;
    MONITORINFO moninfo;

    DXErr(d3dex->GetAdapterIdentifier(id,0,&adapterid));
    DXErr(d3dex->GetDeviceCaps(id,D3DDEVTYPE_HAL,&caps));
    mon = d3dex->GetAdapterMonitor(id);
    sClear(moninfo);
    moninfo.cbSize = sizeof(moninfo);
    sASSERT(GetMonitorInfo(mon,&moninfo));

    DesktopRect = *(sRect *) (&moninfo.rcMonitor);
    DXMasterNumber = caps.MasterAdapterOrdinal;
    DXScreenNumber = id;
    DisplayNumber = id;
    if(caps.MasterAdapterOrdinal!=EnumLastAdapterOrdinal)
        EnumCurrentAdapter++;
    EnumLastAdapterOrdinal = caps.MasterAdapterOrdinal;
    AdapterNumber = EnumCurrentAdapter;
    DisplayInAdapterNumber = caps.AdapterOrdinalInGroup;
    AdapterName = adapterid.Description;
    MonitorName = adapterid.DeviceName;
    SizeX = DesktopRect.SizeX();
    SizeY = DesktopRect.SizeY();
}

sDisplayInfoEx::~sDisplayInfoEx()
{
}

/****************************************************************************/

static void sFindFormat(int af,_D3DFORMAT &fmt);

/****************************************************************************/
/***                                                                      ***/
/***   Create Graphics Subsystems                                         ***/
/***                                                                      ***/
/****************************************************************************/

class sDirect3D9Subsystem : public sSubsystem
{
public:
    sDirect3D9Subsystem() : sSubsystem("Direct3D9",0x40) {}
    void Init()
    {
#if sD3D9EX
        DXErr(Direct3DCreate9Ex(D3D_SDK_VERSION,&DX9));
#else
        DX9 = Direct3DCreate9(D3D_SDK_VERSION);
#endif
        if(!DX9) sFatal("Could not create DirectX9");

        int adaptercount = DX9->GetAdapterCount();
        EnumCurrentAdapter = 0;
        EnumLastAdapterOrdinal = 0;
        for(int i=0;i<adaptercount;i++)
            EnumeratedDisplays.AddTail(new sDisplayInfoEx(i,DX9));
        OpenAdapters.SetSize(EnumCurrentAdapter+1);
        for(sAdapter *&i:OpenAdapters) i=0;

        for(auto i:EnumeratedDisplays)
            sLogF("gfx","display %d@%d: %dx%d - %s on adapter %s",i->DisplayNumber,i->AdapterNumber,i->SizeX,i->SizeY,i->MonitorName,i->AdapterName);
    }

    void Exit()
    {
        sRelease(DX9);
        while(!OpenScreens.IsEmpty())
            delete OpenScreens.RemTail();
        OpenAdapters.DeleteAll();
        EnumeratedDisplays.DeleteAll();

        sASSERT(Resources.IsEmpty());
        sASSERT(Queries.IsEmpty());

        Resources.FreeMemory();
        Queries.FreeMemory();
        OpenScreens.FreeMemory();
        OpenAdapters.FreeMemory();
        EnumeratedDisplays.FreeMemory();
    }
} sDirect3D9Subsystem_;

/****************************************************************************/

sScreenPrivate::~sScreenPrivate()
{
    sRelease(SwapChain);
    delete Present;
}

sScreenPrivate::sScreenPrivate(const sScreenMode &sm_)
{
    ColorBuffer = 0;
    DepthBuffer = 0;
    Adapter = 0;
    Present = new D3DPRESENT_PARAMETERS;
    SwapChain = 0;

    ScreenMode = sm_;
    if(ScreenMode.ColorFormat==0)
        ScreenMode.ColorFormat = sRF_BGRA8888;
    if(ScreenMode.DepthFormat==0)
        ScreenMode.DepthFormat = sRF_D24S8;

    sDisplayInfoEx *dex = EnumeratedDisplays[sm_.Monitor];
    sLogF("gfx","open screen %d, which is display %d adapter %d",ScreenMode.Monitor,dex->DisplayNumber,dex->AdapterName);

    // create window

    uint16 titlebuffer[sFormatBuffer];
    sUTF8toUTF16(ScreenMode.WindowTitle,titlebuffer,sCOUNTOF(titlebuffer));

    if(ScreenMode.Flags & sSM_Fullscreen)
    {
        ScreenWindow = CreateWindowExW(0,L"Altona2",(LPCWSTR) titlebuffer,
            WS_POPUP,0,0,ScreenMode.SizeX,ScreenMode.SizeY,
            GetDesktopWindow(),0,WindowClass.hInstance,0);
    }
    else if(ScreenMode.Flags & sSM_FullWindowed)
    {
        int px = dex->DesktopRect.x0;
        int py = dex->DesktopRect.y0;
        ScreenMode.SizeX = dex->DesktopRect.SizeX();
        ScreenMode.SizeY = dex->DesktopRect.SizeY();

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

    sASSERT(ScreenWindow);

    // set up d3dpresent

    sClear(*Present);
    Present->BackBufferWidth = ScreenMode.SizeX;
    Present->BackBufferHeight = ScreenMode.SizeY;
    Present->AutoDepthStencilFormat = D3DFMT_UNKNOWN;
    Present->EnableAutoDepthStencil = 0;
    Present->hDeviceWindow = (HWND) ScreenWindow;
    Present->PresentationInterval = (ScreenMode.Flags & sSM_NoVSync) ? D3DPRESENT_INTERVAL_IMMEDIATE : D3DPRESENT_INTERVAL_ONE;
    if(ScreenMode.Flags & sSM_Fullscreen)
    {
        ScreenMode.BufferSequence = 2;
        Present->SwapEffect =  D3DSWAPEFFECT_FLIP;
        sFindFormat(ScreenMode.ColorFormat,Present->BackBufferFormat);
        Present->Windowed = 0;
        Present->BackBufferCount = 2;
    }
    else
    {
        ScreenMode.BufferSequence = 1;
        Present->SwapEffect = D3DSWAPEFFECT_COPY;
        Present->BackBufferFormat = D3DFMT_UNKNOWN;
        Present->Windowed = 1;
        Present->BackBufferCount = 1;
    }
    Present->hDeviceWindow = (HWND) ScreenWindow;
    if(!(ScreenMode.Flags & sSM_PartialUpdate))
    {
        if(!(ScreenMode.Flags & sSM_NoDiscard))
            Present->SwapEffect = D3DSWAPEFFECT_DISCARD;
        ScreenMode.BufferSequence = 0;
    }

    // open adapter or swap chain

    Adapter = OpenAdapters[dex->AdapterNumber];
    if(!Adapter)
    {
        OpenAdapters[dex->AdapterNumber] = new sAdapter();
        OpenAdapters[dex->AdapterNumber]->InitPrivate(Present,ScreenMode);
        OpenAdapters[dex->AdapterNumber]->Init();
        Adapter = OpenAdapters[dex->AdapterNumber];
        DXErr(Adapter->DXDev->GetSwapChain(0,&SwapChain));
    }
    else
    {
        Present->PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        DXErr(Adapter->DXDev->CreateAdditionalSwapChain(Present,&SwapChain));
    }

    // done

    RestartGfxInit();
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

sResource *sScreen::GetScreenColor()
{
    return ColorBuffer;
}

sResource *sScreen::GetScreenDepth()
{
    return DepthBuffer;
}

void sScreenPrivate::RestartGfxInit()
{
    IDirect3DSurface9 *surf=0;
    DXErr(SwapChain->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&surf));
    D3DSURFACE_DESC sd;
    DXErr(surf->GetDesc(&sd));
    ScreenMode.SizeX = sd.Width;
    ScreenMode.SizeY = sd.Height;

    ColorBuffer = new sResource(Adapter,sResPara(sRBM_ColorTarget|sRU_Gpu|sRM_Texture,ScreenMode.ColorFormat,sRES_Internal|sRES_NoMipmaps,ScreenMode.SizeX,ScreenMode.SizeY,0,0));
    ColorBuffer->PrivateSetInternalSurf(surf);
    surf->Release();

    if(ScreenMode.DepthFormat!=0)
        DepthBuffer = new sResource(Adapter,sResPara(sRBM_DepthTarget|sRU_Gpu|sRM_Texture,ScreenMode.DepthFormat,sRES_NoMipmaps,ScreenMode.SizeX,ScreenMode.SizeY,0,0));
    else
        DepthBuffer = 0;
}

void sScreenPrivate::RestartGfxExit()
{
    sDelete(DepthBuffer);
    sDelete(ColorBuffer);
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

sAdapterPrivate::sAdapterPrivate()
{
    DXDev = 0;
    sClear(LockQuery);
}

sAdapterPrivate::~sAdapterPrivate()
{
    delete ImmediateContext;
    sRelease(DXDev);
}

void sAdapterPrivate::InitPrivate(_D3DPRESENT_PARAMETERS_ *present,const sScreenMode &sm)
{
    sDisplayInfoEx *dex = EnumeratedDisplays[sm.Monitor];
    sLogF("gfx","open adapter %d (%s)",dex->AdapterNumber,dex->AdapterName);

    WindowedFlag = (sm.Flags & sSM_Fullscreen)?0:1;

    DWORD flags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    if(CurrentMode.Flags & sSM_Multithreaded)
        flags |= D3DCREATE_MULTITHREADED;
    if(sm.Flags & sSM_PreserveFpuDx9)
        flags |= D3DCREATE_FPU_PRESERVE;

    flags |= D3DCREATE_ENABLE_PRESENTSTATS;

#if sD3D9EX

    D3DDISPLAYMODEEX DM;
    DM.Size = sizeof(D3DDISPLAYMODEEX);
    DM.Width = sm.SizeX;
    DM.Height = sm.SizeY;
    DM.RefreshRate = 0;
    sFindFormat(sm.ColorFormat,DM.Format);
    DM.ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;

    HRESULT hr = DX9->CreateDeviceEx(dex->DXMasterNumber,D3DDEVTYPE_HAL,present->hDeviceWindow,
        flags,
        present,(sm.Flags & sSM_Fullscreen) ? &DM : 0,&DXDev);
#else
    HRESULT hr = DX9->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,present->hDeviceWindow,
        flags,
        present,&DXDev);
#endif
    if(FAILED(hr))
        sFatal("count not create dx9 device");

    ImmediateContext = new sContext();
    ImmediateContext->DXDev = DXDev;
    ImmediateContext->Adapter = (sAdapter *) this;

    RestartGfxInit();
}

void sAdapterPrivate::RestartGfxInit()
{
    DXErr(DXDev->CreateQuery(D3DQUERYTYPE_EVENT,&LockQuery[0]));
    DXErr(DXDev->CreateQuery(D3DQUERYTYPE_EVENT,&LockQuery[1]));
    LockQuery[0]->Issue(D3DISSUE_END);
    LockQuery[1]->Issue(D3DISSUE_END);
}

void sAdapterPrivate::RestartGfxExit()
{
    sRelease(LockQuery[0]);
    sRelease(LockQuery[1]);
}

/****************************************************************************/

sContextPrivate::sContextPrivate()
{
    TargetParaPtr = new sTargetPara;
    TargetOn = 0;
}

sContextPrivate::~sContextPrivate()
{
    delete TargetParaPtr;
}

/****************************************************************************/

class sDXDev9Subsystem : public sSubsystem
{
public:
    sDXDev9Subsystem() : sSubsystem("Direct3D9",0xc0) {}
    void Init()
    {
        RestartFlag = 0;
        LostFlag = 0;
    }
    void Exit()
    {
        RestartGfxExit();
        while(!OpenScreens.IsEmpty())
            delete OpenScreens.RemTail();
        for(int i=0;i<OpenAdapters.GetCount();i++)
            sDelete(OpenAdapters[i]);
    }
} sDXDev9Subsystem_;


/****************************************************************************/

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
/***   sResource, Textures, Buffers                                       ***/
/***                                                                      ***/
/****************************************************************************/

static void sFindFormat(int af,_D3DFORMAT &fmt)
{
    switch(af)
    {
    case sRF_RGBA8888: fmt = D3DFMT_A8B8G8R8; break;

    case sRF_BGRA8888: fmt = D3DFMT_A8R8G8B8; break;
    case sRF_BGRA5551: fmt = D3DFMT_A1R5G5B5; break;
    case sRF_BGR565:   fmt = D3DFMT_R5G6B5; break;
    case sRF_BC1:      fmt = D3DFMT_DXT1; break;
    case sRF_BC2:      fmt = D3DFMT_DXT3; break;
    case sRF_BC3:      fmt = D3DFMT_DXT5; break;


    case sRF_D16:      fmt = D3DFMT_D16; break;
    case sRF_D24:      fmt = D3DFMT_D24X8; break;
    case sRF_D24S8:    fmt = D3DFMT_D24S8; break;
    case sRF_D32:      fmt = D3DFMT_D32; break;


    case sRFC_I|sRFB_8|sRFT_UNorm:      fmt = D3DFMT_L8; break;
    case sRFC_I|sRFB_16|sRFT_UNorm:     fmt = D3DFMT_L16; break;
    case sRFC_A|sRFB_8|sRFT_UNorm:      fmt = D3DFMT_A8; break;
    case sRFC_IA|sRFB_8|sRFT_UNorm:     fmt = D3DFMT_A8L8; break;

    case sRFC_R|sRFB_8|sRFT_UNorm:      fmt = D3DFMT_L8; break;
    case sRFC_R|sRFB_16|sRFT_UNorm:     fmt = D3DFMT_L16; break;
    case sRFC_RG|sRFB_16|sRFT_UNorm:	fmt = D3DFMT_G16R16; break;
    case sRFC_RGBA|sRFB_16|sRFT_UNorm:	fmt = D3DFMT_A16B16G16R16; break;

    case sRFC_RG|sRFB_8|sRFT_SNorm:		fmt = D3DFMT_V8U8; break;
    case sRFC_RG|sRFB_16|sRFT_SNorm:	fmt = D3DFMT_V16U16; break;
    case sRFC_RGBA|sRFB_8|sRFT_SNorm:	fmt = D3DFMT_Q8W8V8U8; break;
    case sRFC_RGBA|sRFB_16|sRFT_SNorm:	fmt = D3DFMT_Q16W16V16U16; break;

    case sRFB_10_10_10_2|sRFC_BGRA|sRFT_UNorm: fmt = D3DFMT_A2R10G10B10; break;

    case sRFC_RGBA|sRFB_16|sRFT_Float:	fmt = D3DFMT_A16B16G16R16F; break;
    case sRFC_RGBA|sRFB_32|sRFT_Float:	fmt = D3DFMT_A32B32G32R32F; break;

    case sRFC_R|sRFB_32|sRFT_Float:	    fmt = D3DFMT_R32F; break;

    case 0:            fmt = D3DFMT_UNKNOWN; break;

    default: sASSERT0(); break;
    }
}

void sAdapter::GetBestMultisampling(const sResPara &para,int &count,int &quality)
{
    _D3DFORMAT fmt;
    sFindFormat(para.Format,fmt);
    DWORD q = 0;

    for(int i=16;i>0;i=i/2)
    {
        HRESULT hr = DX9->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,fmt,WindowedFlag,D3DMULTISAMPLE_TYPE(i),&q);
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

int sAdapter::GetMultisampleQuality(const sResPara &para,int count)
{
    if(count>16)
        return 0;

    _D3DFORMAT fmt;
    sFindFormat(para.Format,fmt);
    DWORD quality = 0;
    D3DMULTISAMPLE_TYPE samples;

    if(count>2)
        samples = D3DMULTISAMPLE_TYPE(count);
    else
        samples = D3DMULTISAMPLE_NONE;

    HRESULT hr = DX9->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,fmt,WindowedFlag,samples,&quality);
    if(hr==D3D_OK)
        return quality;
    else
        return 0;
}

sResource::sResource(sAdapter *adapter,const sResPara &para,const void *data,uptr size)
{
    Adapter = adapter;
    Para = para;
    DXVB = 0;
    DXIB = 0;
    DXTex2D = 0;
    DXTex3D = 0;
    DXTexCube = 0;
    DXSurf = 0;
    PrimaryResource = PR_Unknown;
    LockFace = -1;
    LockMipmap = -1;
    Locked = 0;
    SystemMemHack = 0;
    SharedHandle = 0;

    Format = D3DFMT_UNKNOWN;
    if(Para.Mode & sRM_Pinned)
    {
        SystemMemHack = (const void *)data;
        ReInit();
        sASSERT(this);
        Resources.Add(this);
        return;
    }

    ReInit();

    if(data)
    {
        sASSERT((Para.Mode & sRU_Mask)==sRU_Static);
        sASSERT(size == TotalSize);
        if(PrimaryResource==PR_VB || PrimaryResource==PR_IB)
        {
            uint8 *dest;
            MapBuffer(&dest,sRMM_Sync);
            sCopyMem(dest,data,TotalSize);
            Unmap();
        }
        else
        {
            int sx = Para.SizeX;
            int sy = Para.SizeY?Para.SizeY:1;
            int sz = Para.SizeZ?Para.SizeZ:1;
            int bpp = Para.BitsPerElement;
            const uint8 *s = (const uint8 *) data;

            int bc = Para.GetBlockSize();
            sx /= bc;
            sy /= bc;
            bpp *= bc*bc;

            for(int i=0;i<Para.Mipmaps;i++)
            {
                uint8 *dp2;
                int p;
                int p2;

                MapTexture(i,0,&dp2,&p,&p2,sRMM_Sync);

                for(int z=0;z<sz;z++)
                {
                    uint8 *dp = dp2;
                    for(int y=0;y<sy;y++)
                    {
                        sCopyMem(dp,s,sx*bpp/8);
                        dp += p;
                        s += sx*bpp/8;
                    }
                    dp2 += p2;
                }

                sx = sx/2;
                if(Para.SizeY)
                    sy = sy/2;
                if(Para.SizeZ)
                    sz = sz/2;

                Unmap();
            }
        }
    }
    sASSERT(this);
    Resources.Add(this);
}

void sResourcePrivate::ReInit()
{
    int bitsperpixel = 0;
    D3DPOOL pool = D3DPOOL_DEFAULT;
    DWORD usage = 0;

    // sanitize mipmaps

    if(Para.Mipmaps == 0)
        Para.Mipmaps = Para.GetMaxMipmaps();
    else
        Para.Mipmaps = sMin(Para.Mipmaps,Para.GetMaxMipmaps());
    if(Para.Flags & sRES_NoMipmaps)
        Para.Mipmaps = 1;

    // pixel format 

    sFindFormat(Para.Format,Format);
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
    TotalSize = 0;
    int dimensions = Para.GetDimensions();
    for(int i=0;i<Para.Mipmaps;i++)
        TotalSize += level0size >> (i*dimensions);

    // access mode

    switch(Para.Mode & sRU_Mask)
    {
    case sRU_Static: 
#if sD3D9EX
        usage = D3DUSAGE_DYNAMIC;
        if(!(Para.Mode & sRM_Texture))
            usage |= D3DUSAGE_WRITEONLY;
        pool = D3DPOOL_DEFAULT; 
#else
        pool = D3DPOOL_MANAGED; 
#endif
        break;

    case sRU_Gpu: 
        pool = D3DPOOL_DEFAULT; 
        break;

    case sRU_MapWrite:
        usage = D3DUSAGE_DYNAMIC;
        if(!(Para.Mode & sRM_Texture))
            usage |= D3DUSAGE_WRITEONLY;
        pool = D3DPOOL_DEFAULT; 
        break;
/*
    case sRU_StageWrite:
        pool = D3DPOOL_DEFAULT; 
        usage = D3DUSAGE_DYNAMIC;
        break;
        */
    case sRU_MapRead:
        pool = D3DPOOL_DEFAULT;
        usage = D3DUSAGE_DYNAMIC;
        break;

    case sRU_Update:
        if(!(Para.Mode & sRM_Texture))
            sFatal("DX9 can't update buffers, only textures. use sRU_MapWrite");
        pool = D3DPOOL_DEFAULT;
        break;

    case sRU_SystemMem:
        pool = D3DPOOL_SYSTEMMEM;
        break;

    default: 
        sASSERT0();
        break;
    }


    // create

    HANDLE SharedHandleOut = (void *)Para.SharedHandle;
    HANDLE *SharedHandlePtr = 0;
    if((Para.Flags & sRES_SharedHandle) || Para.SharedHandle)
        SharedHandlePtr = &SharedHandleOut;

    if(!(Para.Flags & sRES_Internal))
    {
        if((Para.Mode & sRBM_ColorTarget) || (Para.Mode & sRBM_DepthTarget))
        {
            D3DMULTISAMPLE_TYPE msc = D3DMULTISAMPLE_NONE;
            int msq = 0;
            if(Para.MSCount>=2 && Para.MSCount<=16) 
            {
                msc = D3DMULTISAMPLE_TYPE(Para.MSCount);
                msq = Para.MSQuality;
                if(msq==-1)
                {
                    msq = Adapter->GetMultisampleQuality(Para,Para.MSCount)-1;
                    if(msq==-1)
                    {
                        msc = D3DMULTISAMPLE_NONE;
                        msq = 0;
                    }
                }
            }
            if(Para.Mode & sRBM_ColorTarget)
            {
                if(msc==D3DMULTISAMPLE_NONE && ((Para.Mode & sRBM_Shader) || (Para.Flags & sRES_AutoMipmap)))
                {
                    uint use = D3DUSAGE_RENDERTARGET;
                    if(Para.Flags & sRES_AutoMipmap)
                        use |= D3DUSAGE_AUTOGENMIPMAP;
                    DXErr(Adapter->DXDev->CreateTexture(Para.SizeX,Para.SizeY,1,use,Format,D3DPOOL_DEFAULT,&DXTex2D,SharedHandlePtr));
                    DXErr(DXTex2D->GetSurfaceLevel(0,&DXSurf));
                }
                else
                {
                    sASSERT(Para.Mipmaps == 1);
                    if(Format==D3DFMT_A1R5G5B5)
                        Format = D3DFMT_X1R5G5B5;

                    bool lockable = (usage==D3DUSAGE_DYNAMIC);
                    DXErr(Adapter->DXDev->CreateRenderTarget(Para.SizeX,Para.SizeY,Format,msc,msq,lockable,&DXSurf,SharedHandlePtr));
                }
            }
            else
            {
                if(((Para.Mode & sRU_MapRead) || (Para.Mode & sRU_MapWrite)) && Para.Format==sRF_D32)
                {
                    DXErr(Adapter->DXDev->CreateDepthStencilSurface(Para.SizeX,Para.SizeY,D3DFMT_D32F_LOCKABLE,msc,msq,0,&DXSurf,SharedHandlePtr));
                }
                else if((Para.Mode & sRBM_Shader) && Para.Format==sRF_D24S8)
                {
                    DXErr(Adapter->DXDev->CreateTexture(Para.SizeX,Para.SizeY,1,D3DUSAGE_DEPTHSTENCIL,((D3DFORMAT)(MAKEFOURCC('I','N','T','Z'))),D3DPOOL_DEFAULT,&DXTex2D,SharedHandlePtr));
                    DXErr(DXTex2D->GetSurfaceLevel(0,&DXSurf));
                }
                else
                {
                    DXErr(Adapter->DXDev->CreateDepthStencilSurface(Para.SizeX,Para.SizeY,Format,msc,msq,1,&DXSurf,SharedHandlePtr));
                }
            }
            PrimaryResource = PR_Surf;
        }
        else if(Para.Mode & sRM_Texture)
        {
            if(Para.SizeZ>0 && (Para.Mode & sRM_Cubemap))
            {
                sASSERT(Para.Mode & sRBM_Shader);
                sASSERT(Para.SizeZ == 6);
                sASSERT(Para.SizeX == Para.SizeY);
                DXErr(Adapter->DXDev->CreateCubeTexture(Para.SizeX,Para.Mipmaps,usage,Format,pool,&DXTexCube,SharedHandlePtr));
                PrimaryResource = PR_TexCube;
            }
            else if(Para.SizeZ>0)
            {
                sASSERT(Para.Mode & sRBM_Shader);
                DXErr(Adapter->DXDev->CreateVolumeTexture(Para.SizeX,Para.SizeY,Para.SizeZ,Para.Mipmaps,usage,Format,pool,&DXTex3D,SharedHandlePtr));
                PrimaryResource = PR_Tex3D;
            }
            else
            {
                if(Para.Mode & sRBM_Shader)
                {
                    if(Para.Mode & sRM_Pinned)
                    {
                        DXErr(Adapter->DXDev->CreateTexture(Para.SizeX,Para.SizeY,Para.Mipmaps,0,Format,D3DPOOL_SYSTEMMEM,&DXTex2D,(HANDLE *)&SystemMemHack));
                    }
                    else
                    {
                        DXErr(Adapter->DXDev->CreateTexture(Para.SizeX,Para.SizeY,Para.Mipmaps,usage,Format,pool,&DXTex2D,SharedHandlePtr));
                    }
                }
                else if((Para.Mode & sRM_SystemMem2) || (Para.Mode & sRU_SystemMem))
                {
                    DXErr(Adapter->DXDev->CreateTexture(Para.SizeX,Para.SizeY,Para.Mipmaps,D3DUSAGE_DYNAMIC,Format,D3DPOOL_SYSTEMMEM,&DXTex2D,SharedHandlePtr));
                }
                else
                {
                    sASSERT0();
                }
                PrimaryResource = PR_Tex2D;
            }
        }
        else if(Para.Mode & sRBM_Index)
        {
            sASSERT(Para.Mipmaps == 1);
            sASSERT(Para.BitsPerElement == 2*8 || Para.BitsPerElement == 4*8);
            DXErr(Adapter->DXDev->CreateIndexBuffer((uint)TotalSize,usage,Para.BitsPerElement == 2*8 ? D3DFMT_INDEX16 : D3DFMT_INDEX32,pool,&DXIB,SharedHandlePtr));
            PrimaryResource = PR_IB;
        }
        else if(Para.Mode & sRBM_Vertex)
        {
            sASSERT(Para.Mipmaps == 1);
            DXErr(Adapter->DXDev->CreateVertexBuffer((uint)TotalSize,usage,0,pool,&DXVB,SharedHandlePtr));
            PrimaryResource = PR_VB;
        }
        else
        {
            sASSERT0();
        }
    }
    SharedHandle = (uint) SharedHandleOut;
}

sResource::~sResource()
{
    sASSERT(!Locked);
    sRelease(DXVB);
    sRelease(DXIB);
    sRelease(DXTexBase);
    sRelease(DXSurf);
    Resources.Rem(this);
}

IDirect3DSurface9 *sResourcePrivate::PrivateGetSurf()
{
    if(!DXSurf)
        DXErr(DXTex2D->GetSurfaceLevel(0,&DXSurf));
    return DXSurf;
}

void sResourcePrivate::PrivateBeforeReset()
{
    if((Para.Mode & sRU_Mask)!=sRU_Static)
    {
        sASSERT(!Locked);
        sRelease(DXVB);
        sRelease(DXIB);
        sRelease(DXTexBase);
        sRelease(DXSurf);
    }
}

void sResourcePrivate::PrivateAfterReset()
{
    if((Para.Mode & sRU_Mask)!=sRU_Static)
    {
        if(DXVB==0 && DXIB==0 && DXTexBase==0 && DXSurf==0)
        {
            ReInit();
        }
    }
}

void sResourcePrivate::PrivateSetInternalSurf(IDirect3DSurface9 *surf) 
{
    sRelease(DXSurf);
    DXSurf = surf; 
    surf->AddRef();
}

void sResource::MapBuffer0(sContext *ctx,void **data,sResourceMapMode mode)
{
    sASSERT(!Locked);
    Locked = 1;

    uint flags = 0;
    if((Para.Mode & sRU_Mask)==sRU_MapWrite)
    {
        switch(mode)
        {
        case sRMM_NoOverwrite: flags = D3DLOCK_NOOVERWRITE; break;
        case sRMM_Discard: flags = D3DLOCK_DISCARD; break;
        case sRMM_Sync: flags = 0; break;
        default: sASSERT0();
        }
    }
    else if((Para.Mode & sRU_Mask)==sRU_Static)
    {
        sASSERT(mode==sRMM_Sync);
        flags = 0;
    }
    else
    {
        sASSERT0();
    }

    *data = 0;
    if(PrimaryResource==PR_VB)
    {
        DXErr(DXVB->Lock(0,(uint)TotalSize,data,flags));
    }
    else if(PrimaryResource==PR_IB)
    {
        DXErr(DXIB->Lock(0,(uint)TotalSize,data,flags));
    }
    else
    {
        sASSERT0();
    }
}

void sResource::MapTexture0(sContext *ctx,int mipmap,int index,void **data,int *pitch,int *pitch2,sResourceMapMode mode)
{
    sASSERT(!Locked);
    Locked = 1;

    uint flags = 0;
    switch(Para.Mode & sRU_Mask)
    {
    case sRU_MapWrite:
        switch(mode)
        {
        case sRMM_NoOverwrite: flags = D3DLOCK_NOOVERWRITE; break;
        case sRMM_Discard: flags = D3DLOCK_DISCARD; break;
        case sRMM_Sync: flags = 0; break;
        default: sASSERT0();
        }
        break;
    case sRU_Static:
//    case sRU_StageWrite:
        sASSERT(mode==sRMM_Sync);
        flags = 0;
        break;
    case sRU_MapRead:
        sASSERT(mode==sRMM_Sync);
        flags = D3DLOCK_READONLY;
        break;
    case sRU_SystemMem:
        break;

    default:
        sASSERT0();
        break;
    }

    *data = 0;
    if(pitch) *pitch = 0;
    if(pitch2) *pitch2 = 0;
    if(PrimaryResource==PR_Tex2D)
    {
        LockMipmap = mipmap;
        D3DLOCKED_RECT lr;
        DXErr(DXTex2D->LockRect(LockMipmap,&lr,0,flags));
        *data = lr.pBits;
        if(pitch) *pitch = lr.Pitch;
    }
    else if(PrimaryResource==PR_Surf)
    {
        D3DLOCKED_RECT lr;
        DXErr(DXSurf->LockRect(&lr,0,flags));
        *data = lr.pBits;
        if(pitch) *pitch = lr.Pitch;
    }
    else if(PrimaryResource==PR_TexCube)
    {
        LockMipmap = mipmap;
        LockFace = index;
        D3DLOCKED_RECT lr;
        DXErr(DXTexCube->LockRect(D3DCUBEMAP_FACES(LockFace),LockMipmap,&lr,0,flags));
        *data = lr.pBits;
        if(pitch) *pitch = lr.Pitch;
    }
    else if(PrimaryResource==PR_Tex3D)
    {
        LockMipmap = mipmap;
        D3DLOCKED_BOX lb;
        DXErr(DXTex3D->LockBox(LockMipmap,&lb,0,flags));
        *data = lb.pBits;
        if(pitch) *pitch = lb.RowPitch;
        if(pitch2) *pitch2 = lb.SlicePitch;
    }
    else
    {
        sASSERT0();
    }
}

void sResource::Unmap(sContext *ctx)
{
    sASSERT(Locked);
    if(PrimaryResource==PR_VB)
    {
        DXErr(DXVB->Unlock());
    }
    else if(PrimaryResource==PR_IB)
    {
        DXErr(DXIB->Unlock());
    }
    else if(PrimaryResource==PR_Tex2D)
    {
        DXErr(DXTex2D->UnlockRect(LockMipmap));
    }
    else if(PrimaryResource==PR_Surf)
    {
        DXErr(DXSurf->UnlockRect());
    }
    else if(PrimaryResource==PR_TexCube)
    {
        DXErr(DXTexCube->UnlockRect(D3DCUBEMAP_FACES(LockFace),LockMipmap));
    }
    else if(PrimaryResource==PR_Tex3D)
    {
        DXErr(DXTex3D->UnlockBox(LockMipmap));
    }
    else
    {
        sASSERT0();
    }
    LockMipmap = -1;
    LockFace = -1;
    Locked = 0;
}

void sResource::UpdateBuffer(void *data,int startbyte,int bytesize)
{
    sFatal("sResource::UpdateBuffer - not implemented");
}


void sResource::UpdateTexture(void *data,int miplevel,int arrayindex)
{
    sFatal("sResource::UpdateTexture - not implemented");
}

void sResource::ReadTexture(void *data,uptr size)
{
    sFatal("sResource::ReadTexture - not implemented");
}

/****************************************************************************/
/***                                                                      ***/
/***   Vertex Formats                                                     ***/
/***                                                                      ***/
/****************************************************************************/

sVertexFormat::sVertexFormat(sAdapter *adapter,const uint *desc_,int count_,const sChecksumMD5 &hash_)
{
    Adapter = adapter;
    Hash = hash_;
    Count = count_;
    Desc = desc_;
    AvailMask = 0;
    Streams = 0;

    // create vertex declarator 
    D3DVERTEXELEMENT9 decl[32];
    int i,b[sGfxMaxStream];
    int stream;

    for(i=0;i<sGfxMaxStream;i++)
        b[i] = 0;

    bool dontcreate=0;
    i = 0;
    int j = 0;
    while(Desc[i])
    {
        stream = (Desc[i]&sVF_StreamMask)>>sVF_StreamShift;

        decl[i].Stream = stream;
        decl[i].Offset = b[stream];
        decl[i].Method = 0;

        sASSERT(i<31);

        switch(Desc[i]&sVF_UseMask)
        {
        case sVF_Position:    decl[j].Usage = D3DDECLUSAGE_POSITION;      decl[j].UsageIndex = 0;  break;
        case sVF_Normal:      decl[j].Usage = D3DDECLUSAGE_NORMAL;        decl[j].UsageIndex = 0;  break;
        case sVF_Tangent:     decl[j].Usage = D3DDECLUSAGE_TANGENT;       decl[j].UsageIndex = 0;  break;
        case sVF_BoneIndex:   decl[j].Usage = D3DDECLUSAGE_BLENDINDICES;  decl[j].UsageIndex = 0;  break;
        case sVF_BoneWeight:  decl[j].Usage = D3DDECLUSAGE_BLENDWEIGHT;   decl[j].UsageIndex = 0;  break;
        case sVF_Binormal:    decl[j].Usage = D3DDECLUSAGE_BINORMAL;      decl[j].UsageIndex = 0;  break;
        case sVF_Color0:      decl[j].Usage = D3DDECLUSAGE_COLOR;         decl[j].UsageIndex = 0;  break;
        case sVF_Color1:      decl[j].Usage = D3DDECLUSAGE_COLOR;         decl[j].UsageIndex = 1;  break;
        case sVF_Color2:      decl[j].Usage = D3DDECLUSAGE_COLOR;         decl[j].UsageIndex = 2;  break;
        case sVF_Color3:      decl[j].Usage = D3DDECLUSAGE_COLOR;         decl[j].UsageIndex = 3;  break;
        case sVF_UV0:         decl[j].Usage = D3DDECLUSAGE_TEXCOORD;      decl[j].UsageIndex = 0;  break;
        case sVF_UV1:         decl[j].Usage = D3DDECLUSAGE_TEXCOORD;      decl[j].UsageIndex = 1;  break;
        case sVF_UV2:         decl[j].Usage = D3DDECLUSAGE_TEXCOORD;      decl[j].UsageIndex = 2;  break;
        case sVF_UV3:         decl[j].Usage = D3DDECLUSAGE_TEXCOORD;      decl[j].UsageIndex = 3;  break;
        case sVF_UV4:         decl[j].Usage = D3DDECLUSAGE_TEXCOORD;      decl[j].UsageIndex = 4;  break;
        case sVF_UV5:         decl[j].Usage = D3DDECLUSAGE_TEXCOORD;      decl[j].UsageIndex = 5;  break;
        case sVF_UV6:         decl[j].Usage = D3DDECLUSAGE_TEXCOORD;      decl[j].UsageIndex = 6;  break;
        case sVF_UV7:         decl[j].Usage = D3DDECLUSAGE_TEXCOORD;      decl[j].UsageIndex = 7;  break;
        default: sASSERT0(); break;
        }

        switch(Desc[i]&sVF_TypeMask)
        {
        case sVF_F2:  decl[j].Type = D3DDECLTYPE_FLOAT2;    b[stream]+=2*4;  break;
        case sVF_F3:  decl[j].Type = D3DDECLTYPE_FLOAT3;    b[stream]+=3*4;  break;
        case sVF_F4:  decl[j].Type = D3DDECLTYPE_FLOAT4;    b[stream]+=4*4;  break;
        case sVF_I4:  decl[j].Type = D3DDECLTYPE_UBYTE4;    b[stream]+=1*4;  break;
        case sVF_C4:  decl[j].Type = D3DDECLTYPE_D3DCOLOR;  b[stream]+=1*4;  break;
        case sVF_S2:  decl[j].Type = D3DDECLTYPE_SHORT2N;   b[stream]+=1*4;  break;
        case sVF_S4:  decl[j].Type = D3DDECLTYPE_SHORT4N;   b[stream]+=2*4;  break;
        case sVF_H2:  decl[j].Type = D3DDECLTYPE_FLOAT16_2; b[stream]+=1*4;  break;
        case sVF_H4:  decl[j].Type = D3DDECLTYPE_FLOAT16_4; b[stream]+=2*4;  break;
        case sVF_F1:  decl[j].Type = D3DDECLTYPE_FLOAT1;    b[stream]+=1*4;  break;

        default: sASSERT0(); break;
        }

        AvailMask |= 1 << (Desc[i]&sVF_UseMask);
        Streams = sMax(Streams,stream+1);
        i++;
        j++;
    }
    decl[j].Stream = 0xff;
    decl[j].Offset = 0;
    decl[j].Type = D3DDECLTYPE_UNUSED;
    decl[j].Method = 0;
    decl[j].Usage = 0;
    decl[j].UsageIndex = 0;

    for(int i=0;i<sGfxMaxStream;i++)
        VertexSize[i] = b[i];

    if(dontcreate)
        Decl = 0L;
    else
        DXErr(Adapter->DXDev->CreateVertexDeclaration(decl,&Decl));
}

sVertexFormat::~sVertexFormat()
{
    delete[] Desc;
    sRelease(Decl);
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
    Topology = 0; 

    const static int top[16] = 
    {
        0,D3DPT_POINTLIST,D3DPT_LINELIST,D3DPT_TRIANGLELIST
    };

    Topology = top[(Mode & sGMP_Mask)>>sGMP_Shift];
    sASSERT(Topology!=0);
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
    Adapter = adapter;
    Type = bin->Type;
    Hash = bin->Hash;

    switch(Type)
    {
    case sST_Vertex:   DXErr(Adapter->DXDev->CreateVertexShader  ((const DWORD *)bin->Data,&VS)); break;
    case sST_Pixel:    DXErr(Adapter->DXDev->CreatePixelShader   ((const DWORD *)bin->Data,&PS)); break;
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
    Adapter = adapter;
    LoadBuffer = new uint8[size];
    Size = size;
    Type = 1<<type;
    Slot = slot;
    Loading = 0;
}

sCBufferBase::sCBufferBase(sAdapter *adapter,int size,int typemask,int slot)
{
    Adapter = adapter;
    LoadBuffer = new uint8[size];
    Size = size;
    Type = typemask;
    Slot = slot;
    Loading = 0;
}

sCBufferBase::~sCBufferBase()
{
    delete[] LoadBuffer;
}

void sCBufferBase::Map(sContext *ctx,void **ptr)
{
    sASSERT(!Loading);
    Loading = 1;
    *ptr = LoadBuffer;
}

void sCBufferBase::Unmap(sContext *ctx)
{
    sASSERT(Loading);
    Loading = 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Renderstates                                                       ***/
/***                                                                      ***/
/****************************************************************************/


sRenderState::sRenderState(sAdapter *adapter,const sRenderStatePara &para,const sChecksumMD5 &hash)
{
    Adapter = adapter;
    Hash = hash;
    StateCount = 0;
    States = 0;

    uint buffer[512];
    uint *data = buffer;

    sASSERT(!(para.Flags & sMTRL_Stencil));
    sASSERT(!(para.Flags & sMTRL_IndividualBlend));
    sASSERT(!(para.Flags & sMTRL_AlphaToCoverage));

    *data++ = D3DRS_ZENABLE;
    *data++ = (para.Flags & sMTRL_DepthRead) || (para.Flags & sMTRL_DepthWrite);
    *data++ = D3DRS_ZWRITEENABLE;
    *data++ = (para.Flags & sMTRL_DepthWrite) ? 1 : 0;
    *data++ = D3DRS_ALPHATESTENABLE;
    *data++ = (para.Flags & sMTRL_AlphaTest) ? 1 : 0;

    *data++ = D3DRS_CULLMODE;
    switch(para.Flags & sMTRL_CullMask)
    {
    default:
    case sMTRL_CullOff: *data++=D3DCULL_NONE; break;
    case sMTRL_CullOn:  *data++=D3DCULL_CCW; break;
    case sMTRL_CullInv: *data++=D3DCULL_CW; break;
    }

    *data++ = D3DRS_ZFUNC;
    *data++ = (para.Flags & sMTRL_DepthRead) ? para.DepthFunc : D3DCMP_ALWAYS;
    if(para.Flags & sMTRL_AlphaTest)
    {
        *data++ = D3DRS_ALPHAREF;
        *data++ = para.AlphaRef;
        *data++ = D3DRS_ALPHAFUNC;
        *data++ = para.AlphaFunc;
    }

    *data++ = D3DRS_COLORWRITEENABLE;
    *data++ = para.TargetWriteMask[0]^15;
    *data++ = D3DRS_SCISSORTESTENABLE;
    *data++ = (para.Flags & sMTRL_ScissorDisable)?0:1;
    *data++ = D3DRS_ANTIALIASEDLINEENABLE;
    *data++ = (para.Flags & sMTRL_AALine)?1:0;
    *data++ = D3DRS_MULTISAMPLEANTIALIAS;
    *data++ = (para.Flags & sMTRL_MultisampleDisable)?0:1;

    if(para.BlendColor==0 && para.BlendAlpha==0)
    {
        *data++ = D3DRS_ALPHABLENDENABLE;
        *data++ = 0;
    }
    else
    {
        *data++ = D3DRS_ALPHABLENDENABLE;
        *data++ = 1;
        *data++ = D3DRS_SEPARATEALPHABLENDENABLE;
        *data++ = 1;
        *data++ = D3DRS_BLENDFACTOR;
        *data++ = para.BlendFactor;

        int bc = para.BlendColor[0];
        int ba = para.BlendAlpha[0];
        if(bc==0) bc = sMBS_1 | sMBO_Add | sMBD_0;
        if(ba==0) ba = sMBS_1 | sMBO_Add | sMBD_0;


        *data++ = D3DRS_SRCBLEND;
        *data++ = D3DBLEND((bc>>0)&63);
        *data++ = D3DRS_BLENDOP;
        *data++ = D3DBLENDOP((bc>>12)&15);
        *data++ = D3DRS_DESTBLEND;
        *data++ = D3DBLEND((bc>>6)&63);

        *data++ = D3DRS_SRCBLENDALPHA;
        *data++ = D3DBLEND((ba>>0)&63);
        *data++ = D3DRS_BLENDOPALPHA;
        *data++ = D3DBLENDOP((bc>>12)&15);
        *data++ = D3DRS_DESTBLENDALPHA;
        *data++ = D3DBLEND((ba>>6)&63);
    }

    StateCount = int(data-buffer) / 2;
    uint *st = new uint[StateCount*2];
    States = st;
    sCopyMem(st,buffer,sizeof(uint)*2*StateCount);
}

sRenderState::~sRenderState()
{
    delete[] States;
}

/****************************************************************************/
/***                                                                      ***/
/***   Sampler States                                                     ***/
/***                                                                      ***/
/****************************************************************************/

sSamplerState::sSamplerState(sAdapter *adapter,const sSamplerStatePara &para,const sChecksumMD5 &hash)
{
    Adapter = adapter;
    Hash = hash;
    StateCount = 0;
    States = 0;

    uint buffer[512];
    uint *data = buffer;

    static const D3DTEXTUREADDRESS addr[16] =
    {
        D3DTADDRESS_WRAP,
        D3DTADDRESS_CLAMP,
        D3DTADDRESS_BORDER,
        D3DTADDRESS_MIRROR,
        D3DTADDRESS_MIRRORONCE,
    };

    *data++ = D3DSAMP_ADDRESSU;
    *data++ = addr[(para.Flags&sTF_UMask)>>16];
    *data++ = D3DSAMP_ADDRESSV;
    *data++ = addr[(para.Flags&sTF_VMask)>>20];
    *data++ = D3DSAMP_ADDRESSW;
    *data++ = addr[(para.Flags&sTF_WMask)>>24];
    if(((para.Flags&sTF_UMask)>>16) == sTF_Border ||
        ((para.Flags&sTF_VMask)>>20) == sTF_Border ||
        ((para.Flags&sTF_WMask)>>24) == sTF_Border )
    {
        *data++ = D3DSAMP_BORDERCOLOR;
        *data++ = para.Border.GetColor();
    }
    if((para.Flags & sTF_FilterMask) == sTF_Aniso)
    {
        *data++ = D3DSAMP_MAGFILTER;
        *data++ = D3DTEXF_ANISOTROPIC;
        *data++ = D3DSAMP_MINFILTER;
        *data++ = D3DTEXF_ANISOTROPIC;
        *data++ = D3DSAMP_MIPFILTER;
        *data++ = D3DTEXF_ANISOTROPIC;
    }
    else
    {
        *data++ = D3DSAMP_MAGFILTER;
        *data++ = (para.Flags & 2) ? D3DTEXF_LINEAR : D3DTEXF_POINT;
        *data++ = D3DSAMP_MINFILTER;
        *data++ = (para.Flags & 1) ? D3DTEXF_LINEAR : D3DTEXF_POINT;
        *data++ = D3DSAMP_MIPFILTER;
        *data++ = (para.Flags & 4) ? D3DTEXF_LINEAR : D3DTEXF_POINT;
    }
    *data++ = D3DSAMP_MIPMAPLODBIAS;
    *data++ = *(uint *)&para.MipLodBias;
    //  *data++ = D3DSAMP_MAXMIPLEVEL;
    //  *data++ = int(para.MaxLod);
    *data++ = D3DSAMP_MAXANISOTROPY;
    *data++ = para.MaxAnisotropy;
    *data++ = D3DSAMP_SRGBTEXTURE;
    *data++ = 0;

    StateCount = int(data-buffer) / 2;
    uint *st = new uint[StateCount*2];
    States = st;
    sCopyMem(st,buffer,sizeof(uint)*2*StateCount);
}

sSamplerState::~sSamplerState()
{
    delete[] States;
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
    sASSERT(sam);

    if(shader==sST_Pixel)
    {
        SamplerPS.SetAndGrow(index,sam);
        TexturePS.SetAndGrow(index,tex);
    }
    else if(shader==sST_Vertex)
    {
        SamplerVS.SetAndGrow(index,sam);
        TextureVS.SetAndGrow(index,tex);
    }
    else
    {
        sASSERT0();
    }
}

void sMaterial::UpdateTexture(sShaderTypeEnum shader,int index,sResource *tex,int slicestart,int slicecount)
{
    if(shader==sST_Pixel)
    {
        TexturePS[index] = tex;
    }
    else if(shader==sST_Vertex)
    {
        TextureVS[index] = tex;
    }
    else
    {
        sASSERT0();
    }
}

sResource *sMaterial::GetTexture(sShaderTypeEnum shader,int index)
{
    if(shader==sST_Vertex && TextureVS.IsIndex(index))
        return TextureVS[index];
    if(shader==sST_Pixel && TexturePS.IsIndex(index))
        return TexturePS[index];
    return 0;
}

void sMaterial::SetUav(sShaderTypeEnum shader,int index,sResource *uav)
{
    sASSERT0();
}

void sMaterial::ClearTextureSamplerUav()
{
    SamplerVS.Clear();
    SamplerPS.Clear();
    TextureVS.Clear();
    TexturePS.Clear();
}

/****************************************************************************/
/***                                                                      ***/
/***   Draw It!                                                           ***/
/***                                                                      ***/
/****************************************************************************/

#if sD3D9EX
void sSamplerStatePrivate::SetState(IDirect3DDevice9Ex *dev,int sampler)
#else
void sSamplerStatePrivate::SetState(IDirect3DDevice9 *dev,int sampler)
#endif
{
    const uint *data = States;
    for(int j=0;j<StateCount;j++)
    {
        DXErr(dev->SetSamplerState(sampler,D3DSAMPLERSTATETYPE(data[0]),data[1]));
        data+=2;
    }
}

void sContext::Draw(const sDrawPara &dp)
{
    sUpdateGfxStats(dp);

    // set material states

    const uint *data;

    data = dp.Mtrl->RenderState->States;
    for(int j=0;j<dp.Mtrl->RenderState->StateCount;j++)
    {
        DXErr(DXDev->SetRenderState(D3DRENDERSTATETYPE(data[0]),data[1]));
        data+=2;
    }

    for(int i=0;i<dp.Mtrl->SamplerVS.GetCount();i++)
        if(dp.Mtrl->SamplerVS[i])
            dp.Mtrl->SamplerVS[i]->SetState(DXDev,i+D3DVERTEXTEXTURESAMPLER0);

    for(int i=0;i<dp.Mtrl->SamplerPS.GetCount();i++)
        if(dp.Mtrl->SamplerPS[i])
            dp.Mtrl->SamplerPS[i]->SetState(DXDev,i);

    // vertex input

    if(!(dp.Flags & sDF_Arrays))
    {
        if(dp.Geo->Index)
        {
            DXErr(DXDev->SetIndices(dp.Geo->Index->DXIB));
        }
        else
        {
            DXErr(DXDev->SetIndices(0));
        }

        for(int i=0;i<sGfxMaxStream;i++)
        {
            if(dp.Geo->Vertex[i])
            {
                DXErr(DXDev->SetStreamSource(i,dp.Geo->Vertex[i]->DXVB,dp.VertexOffset[i],dp.Geo->Format->VertexSize[i]));
                if(dp.Flags & sDF_Instances)
                {
                    if(i==0)
                    {
                        DXErr(DXDev->SetStreamSourceFreq(i,D3DSTREAMSOURCE_INDEXEDDATA|dp.InstanceCount));
                    }
                    else
                    {
                        DXErr(DXDev->SetStreamSourceFreq(i,D3DSTREAMSOURCE_INSTANCEDATA|1));
                    }
                }
                else
                {
                    DXErr(DXDev->SetStreamSourceFreq(i,1));
                }
            }
            else
            {
                DXErr(DXDev->SetStreamSource(i,0,0,0));
                DXErr(DXDev->SetStreamSourceFreq(i,1));
            }
        }

        DXErr(DXDev->SetVertexDeclaration(dp.Geo->Format->Decl));
    }
    else
    {
        DXErr(DXDev->SetVertexDeclaration(dp.VertexArrayFormat->Decl));
    }

    // constant buffers and shaders

    for(int i=0;i<dp.CBCount;i++)
    {
        sCBufferBase *cb = dp.CBs[i];
        if(cb)
        {
            if(cb->Type & (1<<sST_Vertex))
                DXErr(DXDev->SetVertexShaderConstantF(0,(const float *)cb->LoadBuffer,(cb->Size+15)/16));
            if(cb->Type & (1<<sST_Pixel))
                DXErr(DXDev->SetPixelShaderConstantF(0,(const float *)cb->LoadBuffer,(cb->Size+15)/16));
        }
    }
    DXErr(DXDev->SetVertexShader(dp.Mtrl->Shaders[sST_Vertex]->VS));
    DXErr(DXDev->SetPixelShader(dp.Mtrl->Shaders[sST_Pixel]->PS));

    // textures (shader resources)

    for(int i=0;i<dp.Mtrl->TextureVS.GetCount();i++)
        DXErr(DXDev->SetTexture(i+D3DVERTEXTEXTURESAMPLER0,dp.Mtrl->TextureVS[i] ? dp.Mtrl->TextureVS[i]->DXTex2D : 0));

    for(int i=0;i<dp.Mtrl->TexturePS.GetCount();i++)
        DXErr(DXDev->SetTexture(i,dp.Mtrl->TexturePS[i] ? dp.Mtrl->TexturePS[i]->DXTex2D : 0));

    // lets kick it!

    if(!(dp.Flags & sDF_Arrays))
    {
        int startindex = 0;
        if(dp.Flags & sDF_IndexOffset)
        {
            switch(dp.Geo->Mode & sGMF_IndexMask)
            {
            case sGMF_Index16:
                sASSERT((dp.IndexOffset & 1)==0);
                startindex = dp.IndexOffset/2;
                break;
            case sGMF_Index32:
                sASSERT((dp.IndexOffset & 3)==0);
                startindex = dp.IndexOffset/4;
                break;
            default:
                sASSERT0();
            }
        }
        if(dp.Geo->Index)
        {
            if(dp.Flags & sDF_Ranges)
            {
                for(int i=0;i<dp.RangeCount;i++)
                    DXErr(DXDev->DrawIndexedPrimitive(D3DPRIMITIVETYPE(dp.Geo->Topology),dp.BaseVertexIndex,0,dp.Geo->VertexCount,dp.Geo->IndexFirst+dp.Ranges[i].Start+startindex,(dp.Ranges[i].End-dp.Ranges[i].Start)/dp.Geo->Divider));
            }
            else
            {
                DXErr(DXDev->DrawIndexedPrimitive(D3DPRIMITIVETYPE(dp.Geo->Topology),dp.BaseVertexIndex,0,dp.Geo->VertexCount,dp.Geo->IndexFirst+startindex,dp.Geo->IndexCount/dp.Geo->Divider));
            }
        }
        else
        {
            if(dp.Flags & sDF_Ranges)
            {
                for(int i=0;i<dp.RangeCount;i++)
                    DXErr(DXDev->DrawPrimitive(D3DPRIMITIVETYPE(dp.Geo->Topology),dp.Geo->VertexOffset+dp.Ranges[i].Start,(dp.Ranges[i].End-dp.Ranges[i].Start)/dp.Geo->Divider));
            }
            else
            {
                DXErr(DXDev->DrawPrimitive(D3DPRIMITIVETYPE(dp.Geo->Topology),dp.Geo->VertexOffset,dp.Geo->VertexCount/dp.Geo->Divider));
            }
        }
    }
    else
    {
        sASSERT(!(dp.Flags & sDF_Ranges));
        sASSERT(!(dp.Flags & sDF_Instances));
        sASSERT(!(dp.Flags & sDF_IndexOffset));
        sASSERT(!(dp.Flags & sDF_VertexOffset));
           
        if(dp.IndexArray)
        {
            DXErr(DXDev->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,0,dp.VertexArrayCount,dp.IndexArrayCount/3,dp.IndexArray,dp.IndexArraySize==2?D3DFMT_INDEX16:D3DFMT_INDEX32,dp.VertexArray,dp.VertexArrayFormat->GetStreamSize(0)));
        }
        else
        {
            DXErr(DXDev->DrawPrimitiveUP(D3DPT_TRIANGLELIST,dp.VertexArrayCount/3,dp.VertexArray,dp.VertexArrayFormat->GetStreamSize(0)));
        }
    }

    // unbind testures in case they are used as render target

    for(int i=0;i<dp.Mtrl->TextureVS.GetCount();i++)
        if(dp.Mtrl->TextureVS[i]->Para.Mode & (sRBM_ColorTarget | sRBM_DepthTarget))
            DXDev->SetTexture(i+D3DVERTEXTEXTURESAMPLER0,0);

    for(int i=0;i<dp.Mtrl->TexturePS.GetCount();i++)
        if(dp.Mtrl->TexturePS[i] && dp.Mtrl->TexturePS[i]->Para.Mode & (sRBM_ColorTarget | sRBM_DepthTarget))
            DXDev->SetTexture(i,0);

    // unmap dynamic resources so they can get mapped

}

/****************************************************************************/
/***                                                                      ***/
/***   Targets and CopyTexture                                            ***/
/***                                                                      ***/
/****************************************************************************/

void sContext::BeginTarget(const sTargetPara &tar)
{
    sASSERT(!TargetOn)
        TargetOn = 1;
    *TargetParaPtr = tar;

    for(int i=0;i<sGfxMaxTargets;i++)
    {
        if(tar.Target[i])
        {
            DXErr(DXDev->SetRenderTarget(i,tar.Target[i]->PrivateGetSurf()));
        }
        else
        {
            DXErr(DXDev->SetRenderTarget(i,0));
        }
    }
    if(tar.Depth)
    {
        DXErr(DXDev->SetDepthStencilSurface(tar.Depth->PrivateGetSurf()));
    }
    else
    {
        DXErr(DXDev->SetDepthStencilSurface(0));
    }

    DWORD dcf = 0;
    if(tar.Flags & sTAR_ClearColor)
        dcf |= D3DCLEAR_TARGET;
    if((tar.Flags & sTAR_ClearDepth) && tar.Depth)
        if (tar.Depth->Format==D3DFMT_D24S8)
            dcf |= D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL;
        else
            dcf |= D3DCLEAR_ZBUFFER;

    D3DVIEWPORT9 vp;
    vp.X = tar.Window.x0;
    vp.Y = tar.Window.y0;
    vp.Width = tar.Window.SizeX();
    vp.Height = tar.Window.SizeY();
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;

    DXDev->SetViewport(&vp);

    sRect all(0,0,tar.SizeX,tar.SizeY);
    DXErr(DXDev->SetScissorRect((const RECT *) &all));
    if(dcf)
    {
        DXErr(DXDev->Clear(0,0,dcf,tar.ClearColor[0].GetColor(),tar.ClearZ,0));
    }
    DXErr(DXDev->SetScissorRect((const RECT *) &tar.Window));
}

void sContext::EndTarget()
{
    sASSERT(TargetOn)
        TargetOn = 0;
}

void sContext::SetScissor(bool enable,const sRect &r)
{
    if(enable)
    {
        DXErr(DXDev->SetScissorRect((const RECT *) &r));
    }
    else
    {
        DXErr(DXDev->SetScissorRect((const RECT *) &TargetParaPtr->Window));
    }
}

void sContext::Resolve(sResource *dest,sResource *src)
{
    DXErr(DXDev->StretchRect(src->PrivateGetSurf(),0,dest->PrivateGetSurf(),0,D3DTEXF_NONE));
}

void sContext::Copy(const sCopyTexturePara &cp)
{
    if((cp.Source->Para.Mode & sRM_Pinned) || (cp.Source->Para.Mode & sRM_SystemMem2) || (cp.Source->Para.Mode & sRU_SystemMem))
    {
        DXErr(DXDev->UpdateSurface(cp.Source->PrivateGetSurf(),0,cp.Dest->PrivateGetSurf(),0));
    }
    else if((cp.Dest->Para.Mode & sRM_Pinned) || (cp.Dest->Para.Mode & sRM_SystemMem2) || (cp.Dest->Para.Mode & sRU_SystemMem))
    {
        DXErr(DXDev->GetRenderTargetData(cp.Source->PrivateGetSurf(),cp.Dest->PrivateGetSurf()));
    }
    else
    {
        DXErr(DXDev->StretchRect(cp.Source->PrivateGetSurf(),(const RECT *)&cp.SourceRect,cp.Dest->PrivateGetSurf(),(const RECT *)&cp.DestRect,D3DTEXF_NONE));
    }
}

/****************************************************************************/
/***                                                                      ***/
/***   Queries                                                            ***/
/***                                                                      ***/
/****************************************************************************/

sGpuQuery::sGpuQuery(sAdapter *adapter,sGpuQueryKind kind)
{
    Adapter = adapter;
    Kind_ = Kind = kind;
    Query = 0;
    PrivateAfterReset();
    Queries.Add(this);
}

sGpuQuery::~sGpuQuery()
{
    Queries.Rem(this);
    sRelease(Query);
}

void sGpuQuery::Begin(sContext *ctx)
{
    switch(Kind)
    {
    case sGQK_Occlusion:
        DXErr(Query->Issue(D3DISSUE_BEGIN));
        break;
    case sGQK_Frequency:
        break;
    default:
        sASSERT0();
    }
}

void sGpuQuery::End(sContext *ctx)
{
    switch(Kind)
    {
    case sGQK_Occlusion:
        DXErr(Query->Issue(D3DISSUE_END));
        break;
    case sGQK_Frequency:
        DXErr(Query->Issue(D3DISSUE_END));
        break;
    default:
        sASSERT0();
    }
}

void sGpuQuery::Issue(sContext *ctx)
{
    switch(Kind)
    {
    case sGQK_Timestamp:
        DXErr(Query->Issue(D3DISSUE_END));
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
        hr = Query->GetData(&data,sizeof(data),0);
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
    case sGQK_Frequency:
        hr = Query->GetData(&data,sizeof(data),0);
        break;
    case sGQK_Occlusion:
        {
            uint d32;
            hr = Query->GetData(&d32,sizeof(d32),0);
            data = d32;
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

bool sGpuQuery::GetData(sPipelineStats &)
{
    sASSERT0();
    return 1;
}

void sGpuQueryPrivate::PrivateBeforeReset()
{
    sRelease(Query);
}

void sGpuQueryPrivate::PrivateAfterReset()
{
    switch(Kind_)
    {
    case sGQK_Timestamp:
        DXErr(Adapter->DXDev->CreateQuery(D3DQUERYTYPE_TIMESTAMP,&Query));
        break;
    case sGQK_Frequency:
        DXErr(Adapter->DXDev->CreateQuery(D3DQUERYTYPE_TIMESTAMPFREQ,&Query));
        break;
    case sGQK_Occlusion:
        DXErr(Adapter->DXDev->CreateQuery(D3DQUERYTYPE_OCCLUSION,&Query));
        break;
    default:
        sASSERT0();
    }
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Fake Interfaces                                                    ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

}

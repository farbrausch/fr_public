/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Base/SystemPrivate.hpp"

namespace Altona2 {

using namespace Private;

/****************************************************************************/
/***                                                                      ***/
/***   Windows Message Loop Modifications                                 ***/
/***                                                                      ***/
/****************************************************************************/

#if sConfigPlatform == sConfigPlatformWin

void Private::sCreateWindow(const sScreenMode &sm)
{
    RECT r2;
    r2.left = r2.top = 0;
    r2.right = sm.SizeX; 
    r2.bottom = sm.SizeY;
    AdjustWindowRect(&r2,WS_OVERLAPPEDWINDOW,FALSE);

    uint16 titlebuffer[sFormatBuffer];
    sUTF8toUTF16(sm.WindowTitle,titlebuffer,sCOUNTOF(titlebuffer));

    Window = CreateWindowW(L"Altona2",(LPCWSTR) titlebuffer, 
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,r2.right-r2.left,r2.bottom-r2.top,
        GetDesktopWindow(), NULL, WindowClass.hInstance, NULL );
}

void Private::Render()
{
    sPreFrameHook.Call();
    CurrentApp->OnFrame();
    sBeginFrameHook.Call();
    CurrentApp->OnPaint();
    sEndFrameHook.Call();
    if(sGC) sGC->CollectIfTriggered();
    sUpdateGfxStats();
}

void Private::sRestartGfx()
{
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Create Fake Graphics Subsystem                                     ***/
/***                                                                      ***/
/****************************************************************************/

sResource *FakeResource;
sAdapter *DefaultAdapter;
sScreen *DefaultScreen;
sContext *DefaultContext;


class sBlankPreSubsystem : public sSubsystem
{
public:
    sBlankPreSubsystem() : sSubsystem("blank renderer preinitialization",0x40) {}
    void Init()
    {
        CurrentMode.ColorFormat = sRF_RGBA8888;
        CurrentMode.DepthFormat = sRF_D24S8;
    }
};

class sBlankSubsystem : public sSubsystem
{
public:
    sBlankSubsystem() : sSubsystem("blank renderer",0xc0) {}
    void Init()
    {
        DefaultAdapter = new sAdapter();
        DefaultScreen = new sScreen(CurrentMode);
        DefaultContext = new sContext();
        DefaultAdapter->ImmediateContext = DefaultContext;
        DefaultContext->Adapter = DefaultAdapter;
        DefaultScreen->Adapter = DefaultAdapter;
        FakeResource = new sResource(DefaultAdapter,sResTexPara(sRF_RGBA8888,16,16,0),0,0);
        DefaultScreen->ColorBuffer = FakeResource;
        DefaultScreen->DepthBuffer = FakeResource;
        DefaultAdapter->Init();
    }
    void Exit()
    {
        delete DefaultAdapter;
        delete DefaultScreen;
        delete DefaultContext;
        delete FakeResource;
        FakeResource = 0;
        while(!OpenScreens.IsEmpty())
            delete OpenScreens[0];
    }
} sBlankSubsystem_;

sScreenPrivate::sScreenPrivate(const sScreenMode &sm) 
{
    ScreenMode = sm; 
    OpenScreens.Add((sScreen *)this);
}

sScreenPrivate::~sScreenPrivate()
{
    OpenScreens.Rem((sScreen *)this);
}


/****************************************************************************/
/***                                                                      ***/
/***   Fake Interfaces                                                    ***/
/***                                                                      ***/
/****************************************************************************/

sShaderBinary *sCompileShader(sShaderTypeEnum type,const char *profile,const char *source,sTextLog *errors)
{
    sShaderBinary *sh = new sShaderBinary;

    sh->Type = type;
    sh->Profile = profile;
    sh->Hash.Calc((const uint8 *)source,int(sGetStringLen(source)));
    sh->Data = (uint8 *) "fake!";
    sh->Size = 6;

    return sh;
}

/****************************************************************************/

void sScreen::WindowControl(WindowControlEnum mode)
{
#if sConfigPlatform == sConfigPlatformWin

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
#endif
}

/****************************************************************************/

void sContext::Draw(const sDrawPara &dp)
{
    sUpdateGfxStats(dp);
}

void sContext::BeginTarget(const sTargetPara &)
{
}

void sContext::EndTarget()
{
}

void sContext::SetScissor(bool enable,const sRect &r)
{
}

void sContext::Resolve(sResource *dest,sResource *src)
{
}

void sContext::Copy(const sCopyTexturePara &cp)
{
}

/****************************************************************************/
/***                                                                      ***/
/***   Fake Classes                                                       ***/
/***                                                                      ***/
/****************************************************************************/

void sAdapter::GetBestMultisampling(const sResPara &,int &count,int &quality)
{
    count = 1;
    quality = 0;
}

int sAdapter::GetMultisampleQuality(const sResPara &,int count)
{
    return 0;
}

sResource::sResource(sAdapter *adapter,const sResPara &para,const void *data,uptr size)
{
    Adapter = adapter;
    LoadBuffer = 0;
    Para = para;
    SharedHandle = 0;
}

sResource::~sResource()
{
}

void sResource::MapBuffer0(sContext *ctx,void **data,sResourceMapMode mode)
{
    LoadBuffer = new uint8[Para.GetTextureSize(0)];
    *data = LoadBuffer;
}

void sResource::MapTexture0(sContext *ctx,int mipmap,int index,void **data,int *pitch,int *pitch2,sResourceMapMode mode)
{
    LoadBuffer = new uint8[Para.GetTextureSize(mipmap)];
    *data = LoadBuffer;
}

void sResource::Unmap(sContext *ctx)
{
    delete[] LoadBuffer;
    LoadBuffer = 0;
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

sVertexFormat::sVertexFormat(sAdapter *adapter,const uint *desc_,int count_,const sChecksumMD5 &hash_)
{
    Adapter = adapter;
    Hash = hash_;
    Count = count_;
    Desc = desc_;
}

sVertexFormat::~sVertexFormat()
{
    delete[] Desc;
}

int sVertexFormat::GetStreamCount()
{
    return 1;
}

int sVertexFormat::GetStreamSize(int stream)
{
    return 32;
}


/****************************************************************************/

void sGeometry::PrivateInit()
{
}

void sGeometry::PrivateExit()
{
}

/****************************************************************************/

sShader::sShader(sAdapter *adapter,sShaderBinary *bin)
{
    Adapter = adapter;
    Hash = bin->Hash;
    Type = bin->Type;
}

sShader::~sShader()
{
}

/****************************************************************************/

sCBufferBase::sCBufferBase(sAdapter *adapter,int size,sShaderTypeEnum type,int slot)
{
    Adapter = adapter;
    LoadBuffer = new uint8[size];
    Size = size;
    Type = 1<<type;
    Slot = slot;
}

sCBufferBase::~sCBufferBase()
{
    delete[] LoadBuffer;
}

void sCBufferBase::Map(sContext *ctx,void **ptr)
{
    *ptr = LoadBuffer;
}

void sCBufferBase::Unmap(sContext *ctx)
{
}

/****************************************************************************/

sRenderState::sRenderState(sAdapter *adapter,const sRenderStatePara &para,const sChecksumMD5 &hash)
{
    Adapter = adapter;
    Hash = hash;
}

sRenderState::~sRenderState()
{
}

/****************************************************************************/

sSamplerState::sSamplerState(sAdapter *adapter,const sSamplerStatePara &para,const sChecksumMD5 &hash)
{
    Adapter = adapter;
    Hash = hash;
}

sSamplerState::~sSamplerState()
{
}

/****************************************************************************/

void sMaterial::PrivateSetBase(sAdapter *adapter)
{
    Adapter = adapter;
}

void sMaterial::SetTexture(sShaderTypeEnum shader,int index,sResource *tex,sSamplerState *sam,int slicestart,int slicecount)
{
}

sResource *sMaterial::GetTexture(sShaderTypeEnum shader,int index)
{
    return 0;
}

void sMaterial::SetUav(sShaderTypeEnum shader,int index,sResource *uav)
{
}

void sMaterial::ClearTextureSamplerUav()
{
}

void sMaterial::UpdateTexture(sShaderTypeEnum shader,int index,sResource *tex,int slicestart,int slicecount)
{
}


/****************************************************************************/

sGpuQuery::sGpuQuery(sAdapter *adapter,sGpuQueryKind kind)
{
    Adapter = adapter;
    Kind = kind;
}

sGpuQuery::~sGpuQuery()
{
}

void sGpuQuery::Begin(sContext *)
{
}

void sGpuQuery::End(sContext *)
{
}

void sGpuQuery::Issue(sContext *)
{
}

bool sGpuQuery::GetData(uint &data)
{
    data = 1;
    return 1;
}

bool sGpuQuery::GetData(uint64 &data)
{
    data = 1;
    return 1;
}

bool sGpuQuery::GetData(sPipelineStats &stat)
{
    sASSERT0();
    return 1;
}

/****************************************************************************/

}

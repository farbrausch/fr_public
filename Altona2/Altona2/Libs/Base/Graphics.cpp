/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Base/SystemPrivate.hpp"
#include "Altona2/Libs/Base/FixedShader.hpp"

/****************************************************************************/

namespace Altona2 {

namespace Private
{
#if  sConfigRender == sConfigRenderDX9
    int DefaultRTColorFormat = sRF_BGRA8888;
#else
    int DefaultRTColorFormat = sRF_RGBA8888;
#endif    
#if sConfigPlatform == sConfigPlatformIOS || sConfigPlatform == sConfigPlatformOSX || sConfigPlatform == sConfigPlatformWin
    int DefaultRTDepthFormat = sRF_D24S8;
#if  sConfigRender == sConfigRenderDX9
    bool DefaultRTDepthIsUsableInShader = false;
#else
    bool DefaultRTDepthIsUsableInShader = true;
#endif
#else
    int DefaultRTDepthFormat = sRF_D16;
    bool DefaultRTDepthIsUsableInShader = false;
#endif  
}

using namespace Private;

/****************************************************************************/
/***                                                                      ***/
/***   Resources that have to be flushed before memleak debugging         ***/
/***                                                                      ***/
/****************************************************************************/

sHook sPreFrameHook;
sHook sBeginFrameHook;
sHook sEndFrameHook;
sHook sBuffersLostHook;
sHook sBeforeResetDeviceHook;
sHook sAfterResetDeviceHook;

void Private::ExitGfx()
{
    sPreFrameHook.FreeMemory();
    sBeginFrameHook.FreeMemory();
    sEndFrameHook.FreeMemory();
    sBuffersLostHook.FreeMemory();
    sBeforeResetDeviceHook.FreeMemory();
    sAfterResetDeviceHook.FreeMemory();
    OpenScreens.FreeMemory();
}

/****************************************************************************/
/***                                                                      ***/
/***   Screenmode Settings                                                ***/
/***                                                                      ***/
/****************************************************************************/

sScreenMode::sScreenMode()
{
    Flags = 0;
    SizeX = 0;
    SizeY = 0;
    ColorFormat = 0;
    DepthFormat = 0;
    Monitor = 0;
    Frequency = 0;
    BufferSequence = 0;
    WindowTitle = "altona application";
}

sScreenMode::sScreenMode(int flags,const char *name,int sx,int sy)
{
    Flags = flags;
    SizeX = sx;
    SizeY = sy;
    ColorFormat = 0;
    DepthFormat = 0;
    Monitor = 0;
    Frequency = 0;
    BufferSequence = 0;
    WindowTitle = name;
}


/*
void sGetScreenMode(sScreenMode &sm)
{
#if sConfigPlatform!=sConfigPlatformOSX
sm = CurrentMode;
#endif
}

void sSetScreenMode(const sScreenMode &sm)
{
#if sConfigPlatform==sConfigPlatformWin
if(sCmpString(sm.WindowTitle,CurrentMode.WindowTitle)!=0 && Window)
{
sUTF8toUTF16(sm.WindowTitle,WindowTitleBuffer,sCOUNTOF(WindowTitleBuffer));
SetWindowTextW(Window,(LPCWSTR)WindowTitleBuffer);
}
#endif
#if sConfigPlatform!=sConfigPlatformOSX
CurrentMode = sm;
#endif
}
*/
/****************************************************************************/
/***                                                                      ***/
/***   Graphics Statistics                                                ***/
/***                                                                      ***/
/****************************************************************************/

sGfxStats sGfxStatsCurrent;
sGfxStats sGfxStatsLast;
bool sGfxStatsEnable = 1;

void sGetGfxStats(sGfxStats &stat)
{
    stat = sGfxStatsLast;
    if(stat.LastFrameDuration < 1e-6f)
        stat.LastFrameDuration = 1e-6f;
    if(stat.FilteredFrameDuration < 1e-6f)
        stat.FilteredFrameDuration = 1e-6f;
}

bool sEnableGfxStats(bool enable)
{
    bool oldstats = sGfxStatsEnable;
    sGfxStatsEnable = enable;
    return oldstats;
}

void sUpdateGfxStats()
{
    const int maxhist = 64;
    static uint64 lasttime = 0;
    static int index = 0;
    static uint64 history[maxhist];

    sGfxStatsLast = sGfxStatsCurrent;
    sGfxStatsCurrent.Batches = 0;
    sGfxStatsCurrent.Instances = 0;
    sGfxStatsCurrent.Ranges = 0;
    sGfxStatsCurrent.Primitives = 0;
    sGfxStatsCurrent.Indices = 0;

    uint64 time = sGetTimeUS();
    if(lasttime==0)
    {
        lasttime = time;
        for(int i=0;i<maxhist;i++)
            history[i] = maxhist;
    }
    sGfxStatsLast.LastFrameDuration = (time-lasttime)/1000000.0f;
    lasttime = time;
    history[index] = time;
    int histuse = 63;
    sGfxStatsLast.FilteredFrameDuration = (history[index]-history[(index-(histuse)+maxhist)%maxhist])/(1000000.0f*histuse);
    if(sGfxStatsLast.FilteredFrameDuration > 0.1f)
    {
        histuse = sClamp(int(2.0/sGfxStatsLast.FilteredFrameDuration),5,63);
        sGfxStatsLast.FilteredFrameDuration = (history[index]-history[(index-(histuse)+maxhist)%maxhist])/(1000000.0f*histuse);
    }

    index = (index+1)%maxhist;

    sGfxStatsEnable  = 1;
}

void sUpdateGfxStats(const sDrawPara &dp)
{
    if(sGfxStatsEnable)
    {
        int inst = 1;
        if(dp.Flags & sDF_Instances)
        {
            inst = dp.InstanceCount;
            sGfxStatsCurrent.Instances += inst;
        }
        else
        {
            sASSERT(dp.InstanceCount==0);
        }

        int ic = 0;
        int divider = 3;
        if(!(dp.Flags & sDF_Arrays))
        {
            ic = dp.Geo->IndexCount;
            if(ic==0)
                ic = dp.Geo->VertexCount;
            divider = dp.Geo->Divider;
        }
        else
        {
            if(dp.IndexArray)
                ic = dp.IndexArrayCount;
            else
                ic = dp.VertexArrayCount;
        }
        if(dp.Flags & sDF_Ranges)
        {
            ic = 0;
            for(int i=0;i<dp.RangeCount;i++)
                ic += dp.Ranges[i].End - dp.Ranges[i].Start;
            sGfxStatsCurrent.Ranges += dp.RangeCount;
        }
        else
        {
            sASSERT(dp.RangeCount==0);
            sGfxStatsCurrent.Ranges += 1;
        }
        ic *= inst;
        int pc = ic/divider;

        sGfxStatsCurrent.Indices += ic;
        sGfxStatsCurrent.Primitives += pc;
        sGfxStatsCurrent.Batches += 1;
    }
}

/****************************************************************************/
/***                                                                      ***/
/***   Parameter Adapters                                                 ***/
/***                                                                      ***/
/****************************************************************************/

sResPara::sResPara()
{
    Mode = 0;
    Format = 0;
    Flags = 0;
    BitsPerElement = 0;
    SizeX = 0;
    SizeY = 0;
    SizeZ = 0;
    SizeA = 0;
    Mipmaps = 0;
    MSCount = 1;
    MSQuality = -1;
    SharedHandle = 0;
}

sResPara::sResPara(int mode,int format,int flags,int sx,int sy,int sz,int sa)
{
    Mode = mode;
    Format = format;
    Flags = flags;
    BitsPerElement = 0;
    SizeX = sx;
    SizeY = sy;
    SizeZ = sz;
    SizeA = sa;
    Mipmaps = 0;
    MSCount = 1;
    MSQuality = -1;
    SharedHandle = 0;

    UpdatePara();
}

int sGetBitsPerPixel(int format)
{
    int channels = 0;
    int bits = 0;

    if(format==0) return 0;

    switch(format&sRFC_Mask)
    {
    case sRFC_R:     channels = 1; break;
    case sRFC_RG:    channels = 2; break;
    case sRFC_RGB:   channels = 3; break;
    case sRFC_RGBA:  channels = 4; break;
    case sRFC_RGBX:  channels = 4; break;
    case sRFC_A:     channels = 1; break;
    case sRFC_I:     channels = 1; break;
    case sRFC_AI:    channels = 2; break;
    case sRFC_IA:    channels = 2; break;
    case sRFC_BGRA:  channels = 4; break;
    case sRFC_BGRX:  channels = 4; break;
    case sRFC_D:     channels = 1; break;
    case sRFC_DX:    channels = 2; break;
    case sRFC_DS:    channels = 2; break;
    default: sASSERT0();
    }
    switch(format&sRFB_Mask)
    {
    case sRFB_4:            bits =  4*channels; break;
    case sRFB_8:            bits =  8*channels; break;
    case sRFB_16:           bits = 16*channels; break;
    case sRFB_32:           bits = 32*channels; break;

    case sRFB_BC1:          bits =  4; break;
    case sRFB_BC2:          bits =  8; break;
    case sRFB_BC3:          bits =  8; break;
    case sRFB_BC4:          bits =  4; break;
    case sRFB_BC5:          bits =  8; break;
    case sRFB_BC6H:         bits =  8; break;
    case sRFB_BC7:          bits =  8; break;
    case sRFB_PVR2:         bits =  2; break;
    case sRFB_PVR4:         bits =  4; break;
    case sRFB_PVR2A:        bits =  2; break;
    case sRFB_PVR4A:        bits =  4; break;

    case sRFB_ETC1:         bits =  4; break;
    case sRFB_ETC2:         bits =  4; break;
    case sRFB_ETC2A:        bits =  8; break;

    case sRFB_5_6_5:        bits = 16; break;
    case sRFB_5_5_5_1:      bits = 16; break;
    case sRFB_10_10_10_2:   bits = 32; break;
    case sRFB_24_8:         bits = 32; break;
    case sRFB_11_11_10:     bits = 32; break;
    default: sASSERT0();
    }

    return bits;
}


void sResPara::UpdatePara()
{
    BitsPerElement = sGetBitsPerPixel(Format);
}

int sResPara::GetMaxMipmaps()
{
    if(!(Mode & sRM_Texture)) return 1;

    // * only power of 2 textures get mipmaps
    // * the shortest coordinate counts.

    int n = 0;
    if(SizeX)
    {
        //  if(!sIsPower2(SizeX)) return 1;
        n = SizeX;
    }
    if(SizeY)
    {
        // if(!sIsPower2(SizeY)) return 1;
        n = sMin(n,SizeY);
    }
    if(SizeZ)
    {
        // if(!sIsPower2(SizeZ)) return 1;
        n = sMin(n,SizeZ);
    }
    if(n==0) return 1;

    return sFindLowerPower(n)+1;
}

int sResPara::GetDimensions()
{
    if(SizeZ && SizeY && SizeX) return 3;
    if(SizeY && SizeX) return 2;
    if(SizeX) return 1;
    return 0;
}

uptr sResPara::GetTextureSize(int mipmap)
{
    uptr n = SizeX>>mipmap;
    if(SizeY) n *= SizeY>>mipmap;
    if(SizeZ) n *= SizeZ>>mipmap;
    if(SizeA) n *= SizeA;

    return (n*BitsPerElement+7)/8;
}

int sResPara::GetBlockSize()
{
    switch(Format)
    {
    case sRF_BC1: case sRF_BC1_SRGB:
    case sRF_BC2: case sRF_BC2_SRGB:
    case sRF_BC3: case sRF_BC3_SRGB:
        return 4;
    default:
        return 1;
    }
}

bool sResPara::operator==(const sResPara &b) const
{
    return sCmpMem(this,&b,sizeof(sResPara))==0;
}

sResTexPara::sResTexPara(int format,int sx,int sy,int mipmaps) 
    : sResPara(sRBM_Shader|sRU_Static|sRM_Texture,format,0,sx,sy)
{
    Mipmaps = mipmaps;
}

sResBufferPara::sResBufferPara(int bind,int bytesperelement,int elements,int format)
{
    Mode = bind;
    Format = format;
    BitsPerElement = bytesperelement*8;
    SizeX = elements;
}

/****************************************************************************/
/*
sResViewPara::sResViewPara()
{
Bind = sRB_Texture;
Flags = 0;
MinX = MaxX = 0;
MinY = MaxY = 0;
MinZ = MaxZ = 0;
MinA = MaxA = 0;
}
*/
/****************************************************************************/

sResUpdatePartialWindow::sResUpdatePartialWindow()
{
    MinX = MaxX = 0;
    MinY = MaxY = 0;
    MinZ = MaxZ = 0;
}

sResUpdatePartialWindow::sResUpdatePartialWindow(const sRect &r)
{
    MinX = r.x0;
    MaxX = r.x1;
    MinY = r.y0;
    MaxY = r.y1;
    MinZ = MaxZ = 0;
}

/****************************************************************************/

sRenderStatePara::sRenderStatePara()
{
    sClear(*this);

    DepthFunc = sCF_LE;
    AlphaFunc = sCF_1;
    SampleMask = 0xff;
    FrontStencilFunc = sCF_0;
    BackStencilFunc = sCF_0;
}

sRenderStatePara::sRenderStatePara(int flags,uint16 blendcolor)
{
    sClear(*this);
    Flags = flags;
    BlendColor[0] = blendcolor;

    DepthFunc = sCF_LE;
    AlphaFunc = sCF_1;
    SampleMask = 0xff;
    FrontStencilFunc = sCF_0;
    BackStencilFunc = sCF_0;
}

/****************************************************************************/

sSamplerStatePara::sSamplerStatePara()
{
    Flags = 0;
    MipLodBias = 0;
    MinLod = -3.402823466e+38F;
    MaxLod = 3.402823466e+38F;
    MaxAnisotropy = 16;
    CmpFunc = sCF_0;
    Border.x = 0;
    Border.y = 0;
    Border.z = 0;
    Border.w = 0;
}

sSamplerStatePara::sSamplerStatePara(int flags,float lodbias)
{
    Flags = flags;
    MipLodBias = lodbias;
    MinLod = -3.402823466e+38F;
    MaxLod = 3.402823466e+38F;
    MaxAnisotropy = 16;
    CmpFunc = sCF_0;
    Border.x = 0;
    Border.y = 0;
    Border.z = 0;
    Border.w = 0;
}

/****************************************************************************/

sCopyTexturePara::sCopyTexturePara()
{
    Flags = 0;
    Source = 0;
    SourceRect.Init(0,0,0,0);
    SourceCubeface = -1;
    Dest = 0;
    DestRect.Init(0,0,0,0);
    DestCubeface = -1;
}

sCopyTexturePara::sCopyTexturePara(int flags,sResource *d,sResource *s)
{
    Flags = flags;
    Source = s;
    SourceRect.Init(0,0,s->Para.SizeX,s->Para.SizeY);
    SourceCubeface = -1;
    Dest = d;
    DestRect.Init(0,0,d->Para.SizeX,d->Para.SizeY);
    DestCubeface = -1;
}

/****************************************************************************/

sTargetPara::sTargetPara()
{
    Flags = 0;
    Depth = 0;
    sClear(Target);
    Mipmap = 0;
    Window.Init(0,0,0,0);
    Aspect = 1;
    SizeX = 0;
    SizeY = 0;
    ClearZ = 1.0f;
    ClearStencil = 0;
    sClear(ClearColor);

    UpdateSize();

    Window.Init(0,0,SizeX,SizeY);
    if(SizeY==0)
        Aspect = 1;
    else
        Aspect = float(SizeX) / float(SizeY);
}

sTargetPara::sTargetPara(int flags,uint color,sScreen *screen)
{
    Flags = flags;
    Depth = screen->DepthBuffer;
    sClear(Target);
    Target[0] = screen->ColorBuffer;
    Mipmap = 0;
    Window.Init(0,0,0,0);
    Aspect = 1;
    SizeX = 0;
    SizeY = 0;
    ClearZ = 1.0f;
    ClearStencil = 0;
    sClear(ClearColor);
    ClearColor[0].SetColor(color);

    UpdateSize();

    Window.Init(0,0,SizeX,SizeY);
    Aspect = float(SizeX) / float(SizeY);
}

sTargetPara::sTargetPara(int flags,uint color,sResource *target,sResource *depth)
{
    Flags = flags;
    Depth = depth;
    sClear(Target);
    Target[0] = target;
    Mipmap = 0;
    Window.Init(0,0,0,0);
    Aspect = 1;
    SizeX = 0;
    SizeY = 0;
    ClearZ = 1.0f;
    ClearStencil = 0;
    sClear(ClearColor);
    ClearColor[0].SetColor(color);

    UpdateSize();

    Window.Init(0,0,SizeX,SizeY);
    Aspect = float(SizeX) / float(SizeY);
}

void sTargetPara::UpdateSize()
{
    int sx = 0;
    int sy = 0;
    for(int i=0;i<sGfxMaxTargets;i++)
    {
        if(Target[i])
        {
            if(sx==0) 
                sx = Target[i]->Para.SizeX;
            else
                sASSERT(sx==Target[i]->Para.SizeX);
            if(sy==0) 
                sy = Target[i]->Para.SizeY;
            else
                sASSERT(sy==Target[i]->Para.SizeY);
        }
    }
    if(Depth)
    {
        if(sx==0) 
            sx = Depth->Para.SizeX;
        else
            sASSERT(sx==Depth->Para.SizeX);
        if(sy==0) 
            sy = Depth->Para.SizeY;
        else
            sASSERT(sy==Depth->Para.SizeY);
    }

    SizeX = sx;
    SizeY = sy;
}

/****************************************************************************/

sDrawPara::sDrawPara()
{
    sClear(*this);
    for(int i=0;i<sCOUNTOF(InitialCount);i++)
        InitialCount[i] = -1;
}

sDrawPara::sDrawPara(sGeometry *geo,sMaterial *mtrl,sCBufferBase *cb0,sCBufferBase *cb1,sCBufferBase *cb2,sCBufferBase *cb3)
{
    sClear(*this);
    for(int i=0;i<sCOUNTOF(InitialCount);i++)
        InitialCount[i] = -1;

    Geo = geo;
    Mtrl = mtrl;
    CBs[0]= cb0; if(cb0) CBCount = 1;
    CBs[1]= cb1; if(cb1) CBCount = 2;
    CBs[2]= cb2; if(cb2) CBCount = 3;
    CBs[3]= cb3; if(cb3) CBCount = 4;
}

sDrawPara::sDrawPara(int xs,int ys,int zs,sMaterial *mtrl,sCBufferBase *cb0,sCBufferBase *cb1,sCBufferBase *cb2,sCBufferBase *cb3)
{
    sClear(*this);
    for(int i=0;i<sCOUNTOF(InitialCount);i++)
        InitialCount[i] = -1;

    Flags = sDF_Compute;
    SizeX = xs;
    SizeY = ys;
    SizeZ = zs;
    Mtrl = mtrl;
    CBs[0]= cb0; if(cb0) CBCount = 1;
    CBs[1]= cb1; if(cb1) CBCount = 2;
    CBs[2]= cb2; if(cb2) CBCount = 3;
    CBs[3]= cb3; if(cb3) CBCount = 4;
}

/****************************************************************************/

sDrawRangePara::sDrawRangePara()
{
    Ranges = &Range;
    RangeCount = 1;
    Flags |= sDF_Ranges;
}

sDrawRangePara::sDrawRangePara(int end,sGeometry *geo,sMaterial *mtrl,sCBufferBase *cb0,sCBufferBase *cb1,sCBufferBase *cb2,sCBufferBase *cb3)
    : sDrawPara(geo,mtrl,cb0,cb1,cb2,cb3) , Range(0,end)
{
    Ranges = &Range;
    RangeCount = 1;
    Flags |= sDF_Ranges;
}

sDrawRangePara::sDrawRangePara(int start,int end,sGeometry *geo,sMaterial *mtrl,sCBufferBase *cb0,sCBufferBase *cb1,sCBufferBase *cb2,sCBufferBase *cb3)
    : sDrawPara(geo,mtrl,cb0,cb1,cb2,cb3) , Range(start,end)
{
    Ranges = &Range;
    RangeCount = 1;
    Flags |= sDF_Ranges;
}


/****************************************************************************/
/***                                                                      ***/
/***   Gfx State Management                                               ***/
/***                                                                      ***/
/****************************************************************************/


void sAdapter::DeleteGfxStates()
{
    // can't use DeleteAll() because of protected destructor

    for(auto i : AllVertexFormats) delete i;
    for(auto i : AllShaders) delete i;
    for(auto i : AllRenderStates) delete i;
    for(auto i : AllSamplerStates) delete i;

    // avoid memory leaks

    AllVertexFormats.FreeMemory();
    AllShaders.FreeMemory();
    AllRenderStates.FreeMemory();
    AllSamplerStates.FreeMemory();
};

/****************************************************************************/

void sResource::MapBuffer0(void **data,sResourceMapMode mode)
{
    MapBuffer0(Adapter->ImmediateContext,data,mode);
}

void sResource::MapTexture0(int mipmap,int index,void **data,int *pitch,int *pitch2,sResourceMapMode mode)
{
    MapTexture0(Adapter->ImmediateContext,mipmap,index,data,pitch,pitch2,mode);
}

void sResource::Unmap()
{
    Unmap(Adapter->ImmediateContext);
}

/****************************************************************************/

sShader *sAdapter::CreateShader(sShaderBinary *bin)
{
    for(auto sh : AllShaders)
        if(sh->Hash==bin->Hash && sh->Adapter==this)
            return sh;

    sShader *sh = new sShader(this,bin);
    AllShaders.Add(sh);
    return sh;
}

sShader *sAdapter::CreateShader(const sAllShaderPara &shaders,sShaderTypeEnum shader)
{
    sShaderBinary bin;
    bin.Type = sShaderTypeEnum(shader);
    bin.Data = shaders.Data[shader];
    bin.Size = shaders.Size[shader];
    bin.CalcHash();
    return CreateShader(&bin);
}

sShaderBinary::sShaderBinary()
{
    Profile = 0;
    DeleteData = 0;
    Data = 0;
    Size = 0;
}

sShaderBinary::~sShaderBinary()
{
    if(DeleteData)
        delete[] Data;
}

void sShaderBinary::CalcHash()
{
    Hash.Calc(Data,int(Size));
}

/****************************************************************************/

void sCBufferBase::Map(void **data)
{ 
    Map(Adapter->ImmediateContext,data); 
}

void sCBufferBase::Unmap()
{ 
    Unmap(Adapter->ImmediateContext); 
}

/****************************************************************************/

sRenderState *sAdapter::CreateRenderState(const sRenderStatePara &para)
{
    sChecksumMD5 hash;
    hash.Calc((const uint8 *)&para,sizeof(para));

    for(auto rs : AllRenderStates)
        if(rs->Hash==hash && rs->Adapter==this)
            return rs;

    sRenderState *rs = new sRenderState(this,para,hash);
    AllRenderStates.Add(rs);
    return rs;
}

/****************************************************************************/

sSamplerState *sAdapter::CreateSamplerState(const sSamplerStatePara &para)
{
    sChecksumMD5 hash;
    hash.Calc((const uint8 *)&para,sizeof(para));

    for(auto ss : AllSamplerStates)
        if(ss->Hash==hash && ss->Adapter==this)
            return ss;

    sSamplerState *ss = new sSamplerState(this,para,hash);
    AllSamplerStates.Add(ss);
    return ss;
}

/****************************************************************************/
/***                                                                      ***/
/***   Platform Independent class implementation                          ***/
/***                                                                      ***/
/****************************************************************************/

sGeometry::sGeometry()
{
    Index = 0;
    DeleteIndex = 0;
    sClear(Vertex);
    sClear(DeleteVertex);
    Adapter = 0;

    Format = 0;
    Mode = sGMP_Error;
    Divider = 0;
}

sGeometry::sGeometry(sAdapter *ada)
{
    Index = 0;
    DeleteIndex = 0;
    sClear(Vertex);
    sClear(DeleteVertex);
    Adapter = ada;

    Format = 0;
    Mode = sGMP_Error;
    Divider = 0;
}

sGeometry::~sGeometry()
{
    PrivateExit();
    if(DeleteIndex)
        delete Index;
    for(int i=0;i<sGfxMaxStream;i++)
        if(DeleteVertex[i])
            delete Vertex[i];
}


void sGeometry::SetIndex(sResource *r,bool del)
{
    if(DeleteIndex) sDelete(Index);
    Index = r;
    DeleteIndex = del;
}

void sGeometry::SetVertex(sResource *r,int stream,bool del)
{
    if(DeleteVertex[stream]) sDelete(Vertex[stream]);
    Vertex[stream] = r;
    DeleteVertex[stream] = del;
}

void sGeometry::SetIndex(const sResBufferPara &para,const void *data,bool del)
{
    sASSERT(Adapter);
    SetIndex(new sResource(Adapter,para,data,para.SizeX*(para.BitsPerElement/8)),del);
}

void sGeometry::SetVertex(const sResBufferPara &para,const void *data,int stream,bool del)
{
    sASSERT(Adapter);
    SetVertex(new sResource(Adapter,para,data,para.SizeX*(para.BitsPerElement/8)),stream,del);
}

void sGeometry::Prepare(sVertexFormat *fmt,int mode,int ic,int i0,int vc,int v0)
{
    Format = fmt;
    Mode = mode;
    IndexCount = ic;
    IndexFirst = i0;
    VertexCount = vc;
    VertexOffset = v0;

    const static int div[16] = 
    {
        0,1,2,3,
        4,4,6,-1,
    };

    Divider = div[(Mode & sGMP_Mask)>>sGMP_Shift];
    if(Divider==-1)
        Divider=Mode & sGM_ControlMask;
    sASSERT(Divider);


    PrivateInit();
}

void sGeometry::Prepare(sVertexFormat *fmt,int mode)
{
    Prepare(fmt,mode,Index ? Index->Para.SizeX : 0,0,Vertex[0]->Para.SizeX,0);
}

sResource *sGeometry::IB()
{
    return Index;
}

sResource *sGeometry::VB(int stream)
{
    return Vertex[stream];
}

/****************************************************************************/

sSetTexturePara::sSetTexturePara()
{
    BindIndex = 0;
    BindShader = sST_Pixel;
    Texture = 0;
    Sampler = 0;
    SliceStart = 0;
    SliceCount = 1;
}

sSetTexturePara::sSetTexturePara(sShaderTypeEnum bindshader,int bindindex,sResource *tex,sSamplerState *sam)
{
    BindIndex = bindindex;
    BindShader = bindshader;
    Texture = tex;
    Sampler = sam;
    SliceStart = 0;
    SliceCount = 1;
}

sSetTexturePara::sSetTexturePara(sShaderTypeEnum bindshader,int bindindex,sResource *tex,sSamplerState *sam,int slicestart,int slicecount)
{
    BindIndex = bindindex;
    BindShader = bindshader;
    Texture = tex;
    Sampler = sam;
    SliceStart = slicestart;
    SliceCount = slicecount;
}


/****************************************************************************/

sMaterial::sMaterial()
{
    RenderState = 0;
    Format = 0;
    Adapter = 0;
    sClear(Shaders);
}

sMaterial::sMaterial(sAdapter *ada)
{
    RenderState = 0;
    Format = 0;
    Adapter = ada;
    sClear(Shaders);
}

sMaterial::~sMaterial()
{
}

void sMaterial::SetState(sRenderState *rs)
{
    RenderState = rs;
}

void sMaterial::SetState(const sRenderStatePara &para)
{
    sASSERT(Adapter);
    RenderState = Adapter->CreateRenderState(para);
}

void sMaterial::SetTexturePS(int index,sResource *tex,sSamplerState *sam)
{
    SetTexture(sST_Pixel,index,tex,sam);
}

void sMaterial::SetTexturePS(int index,sResource *tex,const sSamplerStatePara &sampler)
{
    SetTexture(sST_Pixel,index,tex,Adapter->CreateSamplerState(sampler));
}

void sMaterial::UpdateTexturePS(int index,sResource *tex)
{
    UpdateTexture(sST_Pixel,index,tex);
}

void sMaterial::SetShader(sShader *sh,sShaderTypeEnum type)
{
    Shaders[type] = sh;
}

void sMaterial::SetShaders(sAdapter *adapter,const sAllShaderPara &as)
{
    for(int i=0;i<sST_Max;i++)
    {
        if(as.Data[i])
        {
            sShaderBinary bin;
            bin.Type = sShaderTypeEnum(i);
            bin.Data = as.Data[i];
            bin.Size = as.Size[i];
            bin.CalcHash();
            Shaders[i] = adapter->CreateShader(&bin);
        }
    }
}

void sMaterial::SetShaders(const sAllShaderPara &as)
{
    sASSERT(Adapter);
    SetShaders(Adapter,as);
}

void sMaterial::Prepare(sAdapter *adapter,sVertexFormat *fmt)
{
    Format = fmt;
    PrivateSetBase(adapter);
}

void sMaterial::Prepare(sVertexFormat *fmt)
{
    sASSERT(Adapter);
    sASSERT(!fmt ||  Adapter==fmt->GetAdapter() );
    Format = fmt;
    PrivateSetBase(Adapter);
}

/****************************************************************************/

sFixedMaterialLightPara::sFixedMaterialLightPara()
{
    sClear(*this);
}

void sFixedMaterialLightVC::Set(const struct sViewport &vp,const sFixedMaterialLightPara &para)
{
    MS2SS = vp.MS2SS;

    sVector3 matx; matx.Set(vp.Model.i.x,vp.Model.j.x,vp.Model.k.x);
    sVector3 maty; maty.Set(vp.Model.i.y,vp.Model.j.y,vp.Model.k.y);
    sVector3 matz; matz.Set(vp.Model.i.z,vp.Model.j.z,vp.Model.k.z);

    /*
    sVector3 matx; matx.Set(vp.MS2CS.i.x,vp.MS2CS.i.y,vp.MS2CS.i.z);
    sVector3 maty; maty.Set(vp.MS2CS.j.x,vp.MS2CS.j.y,vp.MS2CS.j.z);
    sVector3 matz; matz.Set(vp.MS2CS.k.x,vp.MS2CS.k.y,vp.MS2CS.k.z);
    */
    LightDirX.x = sDot(para.LightDirWS[0],matx);
    LightDirY.x = sDot(para.LightDirWS[0],maty);
    LightDirZ.x = sDot(para.LightDirWS[0],matz);
    LightDirX.y = sDot(para.LightDirWS[1],matx);
    LightDirY.y = sDot(para.LightDirWS[1],maty);
    LightDirZ.y = sDot(para.LightDirWS[1],matz);
    LightDirX.z = sDot(para.LightDirWS[2],matx);
    LightDirY.z = sDot(para.LightDirWS[2],maty);
    LightDirZ.z = sDot(para.LightDirWS[2],matz);
    LightDirX.w = sDot(para.LightDirWS[3],matx);
    LightDirY.w = sDot(para.LightDirWS[3],maty);
    LightDirZ.w = sDot(para.LightDirWS[3],matz);

    LightColR.x = ((para.LightColor[0]>>16)&255)/255.0f;
    LightColG.x = ((para.LightColor[0]>> 8)&255)/255.0f;
    LightColB.x = ((para.LightColor[0]    )&255)/255.0f;
    LightColR.y = ((para.LightColor[1]>>16)&255)/255.0f;
    LightColG.y = ((para.LightColor[1]>> 8)&255)/255.0f;
    LightColB.y = ((para.LightColor[1]    )&255)/255.0f;
    LightColR.z = ((para.LightColor[2]>>16)&255)/255.0f;
    LightColG.z = ((para.LightColor[2]>> 8)&255)/255.0f;
    LightColB.z = ((para.LightColor[2]    )&255)/255.0f;
    LightColR.w = ((para.LightColor[3]>>16)&255)/255.0f;
    LightColG.w = ((para.LightColor[3]>> 8)&255)/255.0f;
    LightColB.w = ((para.LightColor[3]    )&255)/255.0f;
    Ambient.x = ((para.AmbientColor>>16)&255)/255.0f;
    Ambient.y = ((para.AmbientColor>> 8)&255)/255.0f;
    Ambient.z = ((para.AmbientColor    )&255)/255.0f;
}

void sFixedMaterialVC::Set(const sViewport &vp)
{
    MS2SS = vp.MS2SS;
}

/****************************************************************************/


// why don't use the include? to be able to compile the render==none builds without ASC.
#if sConfigRender!=sConfigRenderNull
extern "C" sAllShaderPermPara sFixedShaderFlat;
extern "C" sAllShaderPermPara sFixedShaderLight;
#endif

sFixedMaterial::sFixedMaterial()
{
    FMFlags = 0;
}

sFixedMaterial::sFixedMaterial(sAdapter *ada) : sMaterial(ada)
{
    FMFlags = 0;
}

void sFixedMaterial::PrivateSetBase(sAdapter *adapter)
{
    int shader = 0;
    bool hasnormal = 0;
    const uint *desc = Format->GetDesc();
    sASSERT((desc[0]&sVF_UseMask)==sVF_Position);
    if((*desc&sVF_UseMask) == sVF_Position)
    {
        if((*desc&sVF_TypeMask)==sVF_F4)
            shader |= 8;
    }
    desc++;
    if((*desc&sVF_UseMask) == sVF_Normal)
    {
        hasnormal = 1;
        desc++;
    }
    if((*desc&sVF_UseMask) == sVF_Color0)
    {
        shader |= 1;
        desc++;
    }
    if((*desc&sVF_UseMask) == sVF_UV0)
    {
        shader += (FMFlags & sFMF_TexTexMask)*2+2;
        desc++;
    }
    sASSERT(*desc==sVF_End);


#if sConfigRender!=sConfigRenderNull
    {
        if(hasnormal)
            sMaterial::SetShaders(adapter,sFixedShaderLight.Get(shader));
        else
            sMaterial::SetShaders(adapter,sFixedShaderFlat.Get(shader));
    }
#endif
    sMaterial::PrivateSetBase(adapter);
}

/****************************************************************************/
/***                                                                      ***/
/***   Screen                                                             ***/
/***                                                                      ***/
/****************************************************************************/

sScreen::sScreen(const sScreenMode &sm) : sScreenPrivate(sm)
{
    OpenScreens.Add(this);
}

sScreen::~sScreen()
{
    OpenScreens.Rem(this);
}

void sScreen::GetSize(int &x,int &y) const
{
    x = ScreenMode.SizeX;
    y = ScreenMode.SizeY;
}

sScreen *sGetScreen(int n)
{
    if(n==-1)
        n=0;
    if(OpenScreens.IsIndex(n))
        return OpenScreens[n];
    else
        return 0;
}

int sGetScreenCount()
{
    return OpenScreens.GetCount();
}

/****************************************************************************/
/***                                                                      ***/
/***   Adapter Defaults                                                   ***/
/***                                                                      ***/
/****************************************************************************/

sAdapter::sAdapter()
{
    FormatP = 0;
    FormatPT = 0;
    FormatPWT = 0;
    FormatPWCT = 0;
    FormatPC = 0;
    FormatPCT = 0;
    FormatPCTT = 0;
    FormatPN = 0;
    FormatPNT = 0;
    FormatPNTT = 0;
    FormatPTT = 0;
    DynamicGeometry = 0;
}

sAdapter::~sAdapter()
{
    delete DynamicGeometry;
    DeleteGfxStates();
}

void sAdapter::Init()
{
    uint desc_p[] =
    {
        sVF_Position,
        sVF_End,
    };
    uint desc_pt[] =
    {
        sVF_Position,
        sVF_UV0,
        sVF_End,
    };
    uint desc_pwt[] =
    {
        sVF_Position|sVF_F4,
        sVF_UV0,
        sVF_End,
    };
    uint desc_pwct[] =
    {
        sVF_Position|sVF_F4,
        sVF_Color0,
        sVF_UV0,
        sVF_End,
    };
    uint desc_pc[] =
    {
        sVF_Position,
        sVF_Color0,
        sVF_End,
    };
    uint desc_pct[] =
    {
        sVF_Position,
        sVF_Color0,
        sVF_UV0,
        sVF_End,
    };
    uint desc_pctt[] =
    {
        sVF_Position,
        sVF_Color0,
        sVF_UV0,
        sVF_UV1,
        sVF_End,
    };
    uint desc_pn[] =
    {
        sVF_Position,
        sVF_Normal,
        sVF_End,
    };
    uint desc_pnt[] =
    {
        sVF_Position,
        sVF_Normal,
        sVF_UV0,
        sVF_End,
    };
    uint desc_pntt[] =
    {
        sVF_Position,
        sVF_Normal,
        sVF_UV0,
        sVF_UV1,
        sVF_End,
    };
    uint desc_ptt[] =
    {
        sVF_Position,    
        sVF_UV0,
        sVF_UV1,
        sVF_End,
    };

    FormatP    = CreateVertexFormat(desc_p);
    FormatPT   = CreateVertexFormat(desc_pt);
    FormatPWT  = CreateVertexFormat(desc_pwt);
    FormatPWCT = CreateVertexFormat(desc_pwct);
    FormatPC   = CreateVertexFormat(desc_pc);
    FormatPCT  = CreateVertexFormat(desc_pct);
    FormatPCTT = CreateVertexFormat(desc_pctt);
    FormatPN   = CreateVertexFormat(desc_pn);
    FormatPNT  = CreateVertexFormat(desc_pnt);
    FormatPNTT = CreateVertexFormat(desc_pntt);
    FormatPTT  = CreateVertexFormat(desc_ptt);

    DynamicGeometry = new sDynamicGeometry(this);
}

sVertexFormat *sAdapter::CreateVertexFormat(const uint *desc_)
{
    static const int defaulttypes[32] = 
    {
        0     ,sVF_F3,sVF_F3,sVF_F4,
        sVF_C4,sVF_C4,0     ,0,
        sVF_C4,sVF_C4,sVF_C4,sVF_C4,
        0,0,0,0,

        sVF_F2,sVF_F2,sVF_F2,sVF_F2,
        sVF_F2,sVF_F2,sVF_F2,sVF_F2,
        0,0,0,0,
        0,0,0,0,
    };

    int count = 0;
    while(desc_[count++]) ;
    uint *desc = new uint[count];
    for(int i=0;i<count;i++)
    {
        uint d = desc_[i];
        if(d!=0 && (d&sVF_TypeMask)==0)
            d |= defaulttypes[d&sVF_UseMask];
        desc[i] = d;
    }

    sChecksumMD5 hash;
    hash.Calc((const uint8 *)desc,count*4);

    for(auto fmt : AllVertexFormats)
    {
        if(fmt->Hash==hash && fmt->Adapter==this)
        {
            delete[] desc;
            return fmt;
        }
    }

    sVertexFormat *fmt = new sVertexFormat(this,desc,count,hash);
    AllVertexFormats.Add(fmt);
    return fmt;
}


/****************************************************************************/
/***                                                                      ***/
/***   Dynamic Geometry Spender                                           ***/
/***                                                                      ***/
/****************************************************************************/

sGeometry *sDynamicGeometry::NewGeo()
{
    sResource *vb = new sResource(Adapter,sResBufferPara(sRBM_Vertex|sRU_MapWrite,32,VBAlloc/32));
    sResource *ib = new sResource(Adapter,sResBufferPara(sRBM_Index|sRU_MapWrite,2,IBAlloc/2));
    sGeometry *geo = new sGeometry();
    geo->SetVertex(vb,0,1);
    geo->SetIndex(ib,1);
    return geo;
}

sDynamicGeometry::sDynamicGeometry(sAdapter *ada)
{
    Adapter = ada;
    VBAlloc = 0x10000*32;
    IBAlloc = 0x18000*2;
#if sDG_ARRAYS
    VertexBuffer = new uint8[VBAlloc];
    IndexBuffer = new uint8[IBAlloc];
    IndexQuad = new uint8[IBAlloc];
    sU16 *ip = (sU16 *) IndexQuad;
    for(int i=0;i<IBAlloc/2/6;i++)
        sQuad(ip,i*4,0,1,2,3);
#else
    VBUsed = 0;
    IBUsed = 0;
    DiscardVB = 1;
    DiscardIB = 1;
    DiscardCount = 0;

    GeoNoIndex = 0;
    GeoIndex = 0;
    GeoQuad = 0;
    IndexBuffer = 0;
    VertexBuffer = 0;
    IndexQuad = 0;

    Quads = 0x4000;
    int ic = Quads*6;
    uint16 *ib = new uint16[ic];
    uint16 *ip = ib;
    for(int i=0;i<Quads;i++)
        sQuad(ip,i*4,0,1,2,3);
    IndexQuad = new sResource(Adapter,sResBufferPara(sRBM_Index|sRU_Static,sizeof(uint16),ic),ib,ic*sizeof(uint16));
    delete[] ib;
    VertexBuffer = new sResource(Adapter,sResBufferPara(sRBM_Vertex|sRU_MapWrite,32,0x10000));
    IndexBuffer = new sResource(Adapter,sResBufferPara(sRBM_Index|sRU_MapWrite,2,0x18000));

    GeoNoIndex = new sGeometry();
    GeoNoIndex->SetVertex(VertexBuffer,0,0);
    GeoIndex = new sGeometry();
    GeoIndex->SetIndex(IndexBuffer,0);
    GeoIndex->SetVertex(VertexBuffer,0,0);
    GeoQuad = new sGeometry();
    GeoQuad->SetIndex(IndexQuad,0);
    GeoQuad->SetVertex(VertexBuffer,0,0);
#endif
    sEndFrameHook.Add(sDelegate<void>(this,&sDynamicGeometry::EndFrame));
}

sDynamicGeometry::~sDynamicGeometry()
{
#if sDG_ARRAYS
    delete[] VertexBuffer;
    delete[] IndexBuffer;
    delete[] IndexQuad;
#else
    delete IndexBuffer;
    delete VertexBuffer;
    delete IndexQuad;
    delete GeoNoIndex;
    delete GeoIndex;
    delete GeoQuad;
#endif
    sEndFrameHook.Rem(sDelegate<void>(this,&sDynamicGeometry::EndFrame));
}

/****************************************************************************/

int sDynamicGeometry::GetMaxIndex(int minic,int indexsize)
{
#if sDG_ARRAYS
    int ic = IBAlloc/indexsize;
#else
    int ic = (IBAlloc-IBUsed)/indexsize;
#endif
    if(minic>ic)
        ic = IBAlloc/indexsize;
    sASSERT(minic<=ic);
    return ic;
}

int sDynamicGeometry::GetMaxVertex(int minvc,sVertexFormat *fmt)
{
#if sDG_ARRAYS
    int vc = VBAlloc/fmt->GetStreamSize(0);
#else
    int vc = (VBAlloc-VBUsed)/fmt->GetStreamSize(0);
#endif
    if(minvc>vc)
        vc = VBAlloc/fmt->GetStreamSize(0);
    sASSERT(minvc<=vc);
    return vc;
}

void sDynamicGeometry::Begin(sContext *ctx,sVertexFormat *fmt,int indexsize,sMaterial *mtrl,int maxvc,int maxic,int mode)
{
    Begin(ctx,fmt,mtrl,maxvc,maxic,mode);
}

void sDynamicGeometry::Begin(sContext *ctx,sVertexFormat *fmt,sMaterial *mtrl,int maxvc,int maxic,int mode)
{
    Context = ctx;
    TempFormat = fmt;
    TempMtrl = mtrl;
    TempVC = maxvc;
    TempIC = maxic;
    TempMode = mode;
    TempISize = 0;
    switch(mode & sGMF_IndexMask)
    {
    case 0:
        break;
    case sGMF_Index16:
        TempISize = 2;
        break;
    default:
        sASSERT0();
    }


    sASSERT(fmt->GetStreamCount()==1);
    int ibb = TempISize*maxic;
    int vbb = fmt->GetStreamSize(0)*maxvc;

    sASSERT(ibb<=IBAlloc && vbb<=VBAlloc);
#if !sDG_ARRAYS
    if(IBUsed+ibb>IBAlloc || VBUsed+vbb>VBAlloc)
    {
        DiscardVB = 1;
        VBUsed = 0;
        DiscardIB = 1;
        IBUsed = 0;
        DiscardCount ++;
    }
#endif
}

#if sDG_ARRAYS
void sDynamicGeometry::BeginVB(void **vp)
{
    *vp = VertexBuffer;
}

void sDynamicGeometry::EndVB(int used)
{
    if(used!=-1)
        TempVC = used;
}

void sDynamicGeometry::BeginIB(void **ip)
{
    *ip = IndexBuffer;
}

void sDynamicGeometry::EndIB(int used)
{
    if(used!=-1)
        TempIC = used;
}
#else
void sDynamicGeometry::BeginVB(void **vp)
{
    uint8 *ptr;
    VertexBuffer->MapBuffer(Context,(void **)&ptr,DiscardVB ? sRMM_Discard : sRMM_NoOverwrite);
    DiscardVB = 0;
    *vp = ptr + VBUsed;
}

void sDynamicGeometry::EndVB(int used)
{
    VertexBuffer->Unmap(Context);
    if(used!=-1)
        TempVC = used;
}

void sDynamicGeometry::BeginIB(void **ip)
{
    uint8 *ptr;
    IndexBuffer->MapBuffer(Context,(void **)&ptr,DiscardIB ? sRMM_Discard : sRMM_NoOverwrite);
    DiscardIB = 0;
    *ip = ptr + IBUsed;
}

void sDynamicGeometry::EndIB(int used)
{
    IndexBuffer->Unmap(Context);
    if(used!=-1)
        TempIC = used;
}
#endif

void sDynamicGeometry::QuadIB()
{
    sU16 *ip;
    BeginIB(&ip);
    sQuad(ip,0,0,1,2,3);
    EndIB();
}

void sDynamicGeometry::End(sCBufferBase *cb0,sCBufferBase *cb1,sCBufferBase *cb2,sCBufferBase *cb3)
{
    if(TempVC==0)
        return;
    if(TempISize!=0 && TempIC==0)
        return;

#if sDG_ARRAYS
    sDrawPara dp(0,TempMtrl,cb0,cb1,cb2,cb3);
    dp.Flags |= sDF_Arrays;
    dp.VertexArray = VertexBuffer;
    dp.VertexArrayCount = TempVC;
    dp.VertexArrayFormat = TempFormat;
    if(TempMode==sGMP_Quads)
    {
        dp.IndexArray = IndexQuad;
        dp.IndexArrayCount = TempVC/4*6;
        dp.IndexArraySize = 2;
    }
    else if(TempISize!=0)
    {
        dp.IndexArray = IndexBuffer;
        dp.IndexArrayCount = TempIC;
        dp.IndexArraySize = TempISize;
    }
    Context->Draw(dp);
#else
    sDrawRangePara dp;
    if(TempMode==sGMP_Quads)
    {
        GeoQuad->Prepare(TempFormat,sGMP_Tris|sGMF_Index16,TempVC/4*6,0,TempVC,0);
        dp = sDrawRangePara(0,TempVC/4*6,GeoQuad,TempMtrl,cb0,cb1,cb2,cb3);
    }
    else if(TempISize==0)
    {
        GeoNoIndex->Prepare(TempFormat,TempMode,0,0,TempVC,0);
        dp = sDrawRangePara(0,TempVC,GeoNoIndex,TempMtrl,cb0,cb1,cb2,cb3);
    }
    else
    {
        GeoIndex->Prepare(TempFormat,TempMode,TempIC,0,TempVC,0);
        dp = sDrawRangePara(0,TempIC,GeoIndex,TempMtrl,cb0,cb1,cb2,cb3);
        dp.Flags |= sDF_IndexOffset;
        dp.IndexOffset = IBUsed;
    }
    dp.Flags |= sDF_VertexOffset;
    dp.VertexOffset[0] = VBUsed;
    Context->Draw(dp);
    IBUsed += TempIC * TempISize;
    VBUsed += TempVC * TempFormat->GetStreamSize(0);
#endif

    Context = 0;
}

void sDynamicGeometry::Flush()
{
}

void sDynamicGeometry::EndFrame()
{
#if !sDG_ARRAYS
    //  sDPrintF("discards last frame: %d\n",DiscardCount);
    DiscardVB = 1;
    DiscardIB = 1;
    DiscardCount = 1;
    IBUsed = 0;
    VBUsed = 0;
#endif
}

/****************************************************************************/


void sDynamicGeometry::Quad(sMaterial *mtrl,const sVertexP &p0,const sVertexP &p1,sCBufferBase *cb0,sCBufferBase *cb1,sCBufferBase *cb2,sCBufferBase *cb3)
{
    Begin(Adapter->FormatP,mtrl,4,6,sGMF_Index16|sGMP_Tris);
    sVertexP *vp;
    BeginVB(&vp);
    vp[0].Set(p0.px,p0.py,p0.pz);
    vp[1].Set(p1.px,p0.py,p0.pz);
    vp[2].Set(p1.px,p1.py,p0.pz);
    vp[3].Set(p0.px,p1.py,p0.pz);
    EndVB();
    QuadIB();
    End(cb0,cb1,cb2,cb3);
}

void sDynamicGeometry::Quad(sMaterial *mtrl,const sVertexPC &p0,const sVertexPC &p1,sCBufferBase *cb0,sCBufferBase *cb1,sCBufferBase *cb2,sCBufferBase *cb3)
{
    Begin(Adapter->FormatPC,mtrl,4,6,sGMF_Index16|sGMP_Tris);
    sVertexPC *vp;
    BeginVB(&vp);
    vp[0].Set(p0.px,p0.py,p0.pz,p0.c0);
    vp[1].Set(p1.px,p0.py,p0.pz,p0.c0);
    vp[2].Set(p1.px,p1.py,p0.pz,p1.c0);
    vp[3].Set(p0.px,p1.py,p0.pz,p1.c0);
    EndVB();
    QuadIB();
    End(cb0,cb1,cb2,cb3);
}

void sDynamicGeometry::Quad(sMaterial *mtrl,const sVertexPT &p0,const sVertexPT &p1,sCBufferBase *cb0,sCBufferBase *cb1,sCBufferBase *cb2,sCBufferBase *cb3)
{
    Begin(Adapter->FormatPT,mtrl,4,6,sGMF_Index16|sGMP_Tris);
    sVertexPT *vp;
    BeginVB(&vp);
    vp[0].Set(p0.px,p0.py,p0.pz,p0.u0,p0.v0);
    vp[1].Set(p1.px,p0.py,p0.pz,p1.u0,p0.v0);
    vp[2].Set(p1.px,p1.py,p0.pz,p1.u0,p1.v0);
    vp[3].Set(p0.px,p1.py,p0.pz,p0.u0,p1.v0);
    EndVB();
    QuadIB();
    End(cb0,cb1,cb2,cb3);
}

void sDynamicGeometry::Quad(sMaterial *mtrl,const sVertexPCT &p0,const sVertexPCT &p1,sCBufferBase *cb0,sCBufferBase *cb1,sCBufferBase *cb2,sCBufferBase *cb3)
{
    Begin(Adapter->FormatPCT,mtrl,4,6,sGMF_Index16|sGMP_Tris);
    sVertexPCT *vp;
    BeginVB(&vp);
    vp[0].Set(p0.px,p0.py,p0.pz,p0.c0,p0.u0,p0.v0);
    vp[1].Set(p1.px,p0.py,p0.pz,p0.c0,p1.u0,p0.v0);
    vp[2].Set(p1.px,p1.py,p0.pz,p1.c0,p1.u0,p1.v0);
    vp[3].Set(p0.px,p1.py,p0.pz,p1.c0,p0.u0,p1.v0);
    EndVB();
    QuadIB();
    End(cb0,cb1,cb2,cb3);
}

/****************************************************************************/
/***                                                                      ***/
/***   Queries                                                            ***/
/***                                                                      ***/
/****************************************************************************/
/*
sGpuQuery *sIssueTimeQuery()
{
  sGpuQuery *q;
  if(sFreeTimeQueries.GetCount()>0)
  {
    q = sFreeTimeQueries.RemTail();
  }
  else
  {
    q = new sGpuQuery(sGQK_Timestamp);
    sAllTimeQueries(q);
  }
  return q;
}

bool sGetTimeQuery(sGpuQuery *query,float &seconds_since_frame_start)
{
  uint64 timestamp;
  if(query->GetData(timestamp))
  {
    seconds_since_frame_start = 1;
    sFreeTimeQueries(query);
    return 1;
  }
  return 0;
}
*/
/****************************************************************************/
/***                                                                      ***/
/***   Compute Dummies                                                    ***/
/***                                                                      ***/
/****************************************************************************/

#if sConfigRender==sConfigRenderDX9 || sConfigRender==sConfigRenderGL2 || sConfigRender==sConfigRenderGLES2 || sConfigRender==sConfigRenderNull


sDispatchContext::sDispatchContext()
{
}

sDispatchContext::~sDispatchContext() 
{
}

void sDispatchContext::SetResource(int slot,sResource *res)
{
}

void sDispatchContext::SetSampler(int slot, sSamplerState *ss)
{
}

void sDispatchContext::SetUav(int slot,sResource *res,uint hidden)
{
}

void sDispatchContext::SetCBuffer(sCBufferBase *cb)
{
}

/****************************************************************************/

void sContext::FillIndirectBuffer(sDrawPara &dp)
{
}

void sContext::CopyHiddenCounter(sResource *dest,sResource *src,sInt byteoffset)
{
}

void sContext::CopyHiddenCounter(sCBufferBase *dest,sResource *src,sInt byteoffset)
{
}

void sContext::BeginDispatch(sDispatchContext *dp)
{
}

void sContext::EndDispatch(sDispatchContext *dp)
{
}

void sContext::Dispatch(sShader *sh,int x,int y,int z,const sPerfMonSection &perf,bool debug)
{
}

void sContext::Dispatch(sShader *sh,int x,int y,int z)
{
}

void sContext::Dispatch(sMaterial *sh,int x,int y,int z)
{
}

void sContext::UnbindCBuffer(sDispatchContext *dp)
{
}

void sContext::RebindCBuffer(sDispatchContext *dp)
{
}

void sContext::ClearUav(sResource *r,const uint v[4])
{
}

void sContext::ClearUav(sResource *r,const float v[4])
{
}

void sContext::ComputeSync()
{
}

sResource *sContext::GetComputeSyncUav()
{
    return 0;
}

/****************************************************************************/

void sResource::DebugDumpBuffer(int columns,const char *type,int max,const char *msg)
{
}

/****************************************************************************/


sResource *sAdapter::CreateBufferRW(int count,int format,int additionalbind)
{
    sFatal("sAdapter::CreateBufferRW not implemented");
    return 0;
}

sResource *sAdapter::CreateStructRW(int count,int bytesperelement,int additionalbind)
{
    sFatal("sAdapter::CreateStructRW not implemented");
    return 0;
}

sResource *sAdapter::CreateCounterRW(int count,int bytesperelement,int additionalbind)
{
    sFatal("sAdapter::CreateCounterRW not implemented");
    return 0;
}

sResource *sAdapter::CreateAppendRW(int count,int bytesperelement,int additionalbind)
{
    sFatal("sAdapter::CreateAppendRW not implemented");
    return 0;
}

sResource *sAdapter::CreateRawRW(int bytecount,int additionalbind)
{
    sFatal("sAdapter::CreateRawRW not implemented");
    return 0;
}

sResource *sAdapter::CreateStruct(int count,int bytesperelement,int additionalbind,void *data)
{
    sFatal("sAdapter::CreateStructRW not implemented");
    return 0;
}

sResource *sAdapter::CreateBuffer(int count,int format,int additionalbind,void *data,int size)
{
    sFatal("sAdapter::CreateStructRW not implemented");
    return 0;
}

/****************************************************************************/

#endif

/****************************************************************************/

}

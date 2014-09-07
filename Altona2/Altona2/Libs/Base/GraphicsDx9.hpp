/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_BASE_GRAPHICS_DX9_HPP
#define FILE_ALTONA2_LIBS_BASE_GRAPHICS_DX9_HPP

#ifndef sD3D9EX
    #define sD3D9EX 1
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Platform dependend forwards                                        ***/
/***                                                                      ***/
/****************************************************************************/

struct IDirect3DVertexShader9;
struct IDirect3DPixelShader9;

struct IDirect3DVertexDeclaration9;
struct IDirect3DSurface9;

struct IDirect3DVertexBuffer9;
struct IDirect3DIndexBuffer9;

struct IDirect3DBaseTexture9;
struct IDirect3DVolumeTexture9;
struct IDirect3DCubeTexture9;
struct IDirect3DTexture9;

struct IDirect3DQuery9;

struct IDirect3DDevice9;
struct IDirect3DDevice9Ex;
struct _D3DPRESENT_PARAMETERS_;
struct IDirect3DSwapChain9;

enum _D3DFORMAT;

/****************************************************************************/

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***   Limits                                                             ***/
/***                                                                      ***/
/****************************************************************************/

enum sGraphicsLimits
{
    sGfxMaxTargets = 4,
    sGfxMaxCBs = 8,
    sGfxMaxStream = 4,
    sGfxMaxTexture = 16,
    sGfxMaxSampler = 16,
    sGfxMaxVSAttrib = 16,
    sGfxMaxUav = 1,
};

/****************************************************************************/
/***                                                                      ***/
/***   Private Classes                                                    ***/
/***                                                                      ***/
/****************************************************************************/

class sResourcePrivate
{
    friend class sContext;
protected:
    enum PrimaryResourceEnum
    {
        PR_Unknown  = 0,
        PR_VB,
        PR_IB,
        PR_Tex2D,
        PR_Tex3D,
        PR_TexCube,
        PR_Surf,
    };

    IDirect3DVertexBuffer9 *DXVB;
    IDirect3DIndexBuffer9 *DXIB;
    union
    { 
        IDirect3DBaseTexture9 *DXTexBase;
        IDirect3DTexture9 *DXTex2D;
        IDirect3DVolumeTexture9 *DXTex3D;
        IDirect3DCubeTexture9 *DXTexCube;
    };
    IDirect3DSurface9 *DXSurf;
    _D3DFORMAT Format;
    PrimaryResourceEnum PrimaryResource;
    uptr TotalSize;
    int LockFace;
    int LockMipmap;
    bool Locked;
    sAdapter *Adapter;
    uint SharedHandle;
    void ReInit();
public:
    sResPara Para;
    const void *SystemMemHack;

    void PrivateSetInternalSurf(IDirect3DSurface9 *surf);
    IDirect3DSurface9 *PrivateGetSurf();
    IDirect3DTexture9 *PrivateGetTex() { return DXTex2D; }
    void *PrivateGetSharedHandle() { return (void *) SharedHandle; }
    void PrivateSetTex(IDirect3DTexture9 *tex) { DXTex2D=tex; }

    void PrivateBeforeReset();
    void PrivateAfterReset();
};

class sVertexFormatPrivate
{
    friend class sContext;
protected:
    sAdapter *Adapter;
    uint AvailMask;
    int Streams;
    int VertexSize[sGfxMaxStream];
    IDirect3DVertexDeclaration9 *Decl;
};

class sShaderPrivate
{
    friend class sContext;
protected:
    sAdapter *Adapter;
    union
    {
        IDirect3DVertexShader9 *VS;
        IDirect3DPixelShader9 *PS;
    };
};

class sCBufferBasePrivate
{ 
    friend class sContext;
protected:
    sAdapter *Adapter;
    uint8 *LoadBuffer;
    bool Loading;
};

class sRenderStatePrivate
{
    friend class sContext;
protected:
    sAdapter *Adapter;
    int StateCount;
    const uint *States;
};

class sSamplerStatePrivate
{
    friend class sContext;
#if sD3D9EX
    void SetState(IDirect3DDevice9Ex *dev,int sampler);
#else
    void SetState(IDirect3DDevice9 *dev,int sampler);
#endif
protected:
    sAdapter *Adapter;
    int StateCount;
    const uint *States;
};

class sMaterialPrivate
{
    friend class sContext;
protected:
    sArray<sSamplerState *> SamplerVS;
    sArray<sSamplerState *> SamplerPS;
    sArray<sResource *> TextureVS;
    sArray<sResource *> TexturePS;
};

class sDispatchContextPrivate
{
};

class sGeometryPrivate
{
    friend class sContext;
protected:
    int Topology;
};

class sGpuQueryPrivate
{
protected:
    IDirect3DQuery9 *Query;
    int Kind_;
    sAdapter *Adapter;
public:
    void PrivateBeforeReset();
    void PrivateAfterReset();
};

/****************************************************************************/

class sAdapterPrivate
{
    friend class sResource;
    friend class sCBufferBase;
    friend class sGpuQuery;
protected:
    bool WindowedFlag;
public:
    sAdapterPrivate();
    ~sAdapterPrivate();
    void InitPrivate(_D3DPRESENT_PARAMETERS_ *,const sScreenMode &sm);
    void RestartGfxInit();
    void RestartGfxExit();

    sContext *ImmediateContext;

    // semi protected

#if sD3D9EX
    IDirect3DDevice9Ex *DXDev;
#else
    IDirect3DDevice9 *DXDev;
#endif
    IDirect3DQuery9 *LockQuery[2];
};

/****************************************************************************/

class sContextPrivate
{
    friend class sResource;
    friend class sCBufferBase;
    friend class sGpuQuery;
protected:
    bool TargetOn;
    sTargetPara *TargetParaPtr;
public:
    sContextPrivate();
    ~sContextPrivate();
    sAdapter *Adapter;

    // semi protected

#if sD3D9EX
    IDirect3DDevice9Ex *DXDev;
#else
    IDirect3DDevice9 *DXDev;
#endif
};

/****************************************************************************/

class sScreenPrivate
{
public:
    sAdapter *Adapter;

    sScreenPrivate(const sScreenMode &sm);
    ~sScreenPrivate();
    void RestartGfxInit();
    void RestartGfxExit();

    // semi protected

    sResource *ColorBuffer;
    sResource *DepthBuffer;
    _D3DPRESENT_PARAMETERS_ *Present;
    void *ScreenWindow;
    sScreenMode ScreenMode;
    IDirect3DSwapChain9 *SwapChain;
};

/****************************************************************************/

typedef bool (*SwapCallbackType)(IDirect3DDevice9Ex *dev,IDirect3DSwapChain9 *swap);

extern SwapCallbackType SwapCallback;

}

#endif  // FILE_ALTONA2_LIBS_BASE_GRAPHICS_DX9_HPP

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_BASE_GRAPHICS_DX11_HPP
#define FILE_ALTONA2_LIBS_BASE_GRAPHICS_DX11_HPP

/****************************************************************************/
/***                                                                      ***/
/***   Platform dependend forwards                                        ***/
/***                                                                      ***/
/****************************************************************************/

struct ID3D11Resource;
struct ID3D11Buffer;
struct ID3D11Texture1D;
struct ID3D11Texture2D;
struct ID3D11Texture3D;

struct ID3D11ShaderResourceView;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11UnorderedAccessView;

struct ID3D11VertexShader;
struct ID3D11HullShader;
struct ID3D11DomainShader;
struct ID3D11GeometryShader;
struct ID3D11PixelShader;
struct ID3D11ComputeShader;

struct ID3D11InputLayout;
struct ID3D11BlendState;
struct ID3D11DepthStencilState;
struct ID3D11RasterizerState;
struct ID3D11SamplerState;

struct ID3D11Query;

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;

enum DXGI_FORMAT;

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
    sGfxMaxCBs = 16,
    sGfxMaxStream = 4,
    sGfxMaxTexture = 128,
    sGfxMaxSampler = 16,
    sGfxMaxVSAttrib = 16,
    sGfxMaxUav = 8,
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
    union
    {
        ID3D11Resource *DXResource;
        ID3D11Buffer *DXBuffer;
        ID3D11Texture1D *DXTex1D;
        ID3D11Texture2D *DXTex2D;
        ID3D11Texture3D *DXTex3D;
    };
    union
    {
        ID3D11Resource *STResource;
        ID3D11Buffer *STBuffer;
        ID3D11Texture1D *STTex1D;
        ID3D11Texture2D *STTex2D;
        ID3D11Texture3D *STTex3D;
    };
    DXGI_FORMAT Format_Shader;
    DXGI_FORMAT Format_Depth;
    DXGI_FORMAT Format_Create;

    ID3D11ShaderResourceView *ShaderResourceView;
    ID3D11RenderTargetView *RenderTargetView;
    ID3D11DepthStencilView *DepthStencilView;
    ID3D11UnorderedAccessView *UnorderedAccessView;
    uint SharedHandle;

    bool Loading;
    sAdapter *Adapter;
public:
    sResPara Para;

    ID3D11Buffer *GetDXBuffer() { return DXBuffer; }
    ID3D11Resource *GetDXResource() { return DXResource; }
    bool PrivateIsDynamic() { return (Para.Mode & sRU_Mask)==sRU_MapWrite || (Para.Mode & sRU_Mask)==sRU_Update; }
    void PrivateSetInternalResource(ID3D11Resource *res);
    ID3D11ShaderResourceView *PrivateSRView();
    ID3D11RenderTargetView *PrivateRTView();
    ID3D11DepthStencilView *PrivateDSView();
    ID3D11UnorderedAccessView *PrivateUAView();
};

/****************************************************************************/

class sVertexFormatPrivate
{
    friend class sContext;
protected:
    uint AvailMask;
    int Streams;
    int VertexSize[sGfxMaxStream];
    ID3D11InputLayout *Layout;
};

/****************************************************************************/

class sShaderPrivate
{
    friend class sContext;
protected:
    union
    {
        ID3D11VertexShader *VS;
        ID3D11HullShader *HS;
        ID3D11DomainShader *DS;
        ID3D11GeometryShader *GS;
        ID3D11PixelShader *PS;
        ID3D11ComputeShader *CS;
    };
};

/****************************************************************************/

class sCBufferBasePrivate
{
    friend class sContext;
    friend class sDispatchContext;
protected:
    ID3D11Buffer *DXBuffer;
    bool Loading;
    sAdapter *Adapter;
};

/****************************************************************************/

class sRenderStatePrivate
{
protected:
    struct ID3D11BlendState *BlendState;
    struct ID3D11DepthStencilState *DepthState;
    struct ID3D11RasterizerState *RasterState;
    friend class sContext;
    uint SampleMask;
    uint StencilRef;
};

/****************************************************************************/

class sSamplerStatePrivate
{
    friend class sDispatchContext;
protected:
    struct ID3D11SamplerState *SamplerState;
    friend class sContext;
    friend class sMaterial;
};

/****************************************************************************/

class sMaterialPrivate
{
    friend class sContext;
protected:
    sArray<ID3D11SamplerState *> Sampler[sST_Max];
    sArray<sResource *> Texture[sST_Max];
    sArray<ID3D11ShaderResourceView *> ResView[sST_Max];
    sArray<sResource *> UavPixel;
    sArray<sResource *> UavCompute;
    sArray<ID3D11UnorderedAccessView *> UavViewPixel;
    sArray<ID3D11UnorderedAccessView *> UavViewCompute;

    void GrowSampler(int n);
    void GrowReView(int n);
};

/****************************************************************************/

class sDispatchContextPrivate
{
    friend class sContext;
protected:
    int ResourceCount;
    int SamplerCount;
    int UavCount;
    int CBufferCount;
    ID3D11ShaderResourceView *Resources[sGfxMaxSampler];           // we don't allow full 128 textures, because that's insane..
    ID3D11SamplerState *Samplers[sGfxMaxSampler];
    ID3D11UnorderedAccessView *Uavs[sGfxMaxUav];
    ID3D11Buffer *CBuffers[sGfxMaxCBs];
};

/****************************************************************************/

class sGeometryPrivate
{
    friend class sContext;
protected:
    ID3D11Buffer *DXIB;
    DXGI_FORMAT IBFormat;
    ID3D11Buffer *DXVB[sGfxMaxStream];
    int Topology;
    bool HasDynamicBuffers;
};

/****************************************************************************/

class sGpuQueryPrivate
{
protected:
    sAdapter *Adapter;
    struct ID3D11Query *Query;
};

/****************************************************************************/

class sAdapterPrivate
{
    friend class sResource;
    friend class sCBufferBase;
    friend class sGpuQuery;
    friend class sContext;
protected:

    struct IndirectCBStruct
    {
        uint counts[4];

        uint Index;
        uint IndexVertexCountPerInstance;
        uint InstanceCount;
        uint StartIndexVertexLocation;

        uint StartInstanceLocation;
        uint BaseVertexLocation;
        uint InstanceAdd;
        uint ThreadGroupSize;
    };

    static const int IndirectBuffers = 4;
    static const int MaxDrawIndirectBytes = 4096*32;

    int IndirectIndex;
    sResource *IndirectBuffer;
    sCBufferBase *IndirectConstants;
    sMaterial *DrawIndexedInstancedIndirectMtrl;
    sMaterial *DrawInstancedIndirectMtrl;
    sMaterial *DispatchIndirectMtrl;
public:
    sAdapterPrivate();
    ~sAdapterPrivate();
    void InitPrivate(int id,bool debug);
    void BeforeFrame();

    sContext *ImmediateContext;

    // semi-protected:

    ID3D11Device *DXDev;
    ID3D11DeviceContext *DXCtx;
};

/****************************************************************************/

class sContextPrivate
{
    friend class sResource;
    friend class sCBufferBase;
    friend class sGpuQuery;
protected:
    sResource *ComputeSyncUav;
public:

    sContextPrivate();
    ~sContextPrivate();
    void Init(sAdapter *ada,ID3D11DeviceContext *ctx);

    sAdapter *Adapter;

    // semi-protected:

    ID3D11DeviceContext *DXCtx;
};

/****************************************************************************/

class sScreenPrivate
{
    void CreateSwapBuffer();
public:
    sResource *ColorBuffer;
    sResource *DepthBuffer;
    sAdapter *Adapter;

    sScreenPrivate(const sScreenMode &sm);
    ~sScreenPrivate();
    void RestartGfx();

    // semi-protected:

    IDXGISwapChain *DXSwap;
    void *ScreenWindow;
    sScreenMode ScreenMode;
};

/****************************************************************************/

}

#endif  // FILE_ALTONA2_LIBS_BASE_GRAPHICS_DX11_HPP


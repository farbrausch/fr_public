/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_BASE_GRAPHICS_DX11_PRIVATE_HPP
#define FILE_BASE_GRAPHICS_DX11_PRIVATE_HPP

#include "base/types.hpp"

/****************************************************************************/

// not yet handled



/****************************************************************************/

// constants

enum 
{
  sMTRL_MAXTEX = 16,                                  // max number of textures
  sMTRL_MAXPSTEX = 16,                                // max number of pixel shader samplers
  sMTRL_MAXVSTEX = 4,                                 // max number of vertex shader samplers
};

/****************************************************************************/

struct sVertexFormatHandlePrivate
{
protected:
  
public:
  struct ID3D11InputLayout *Layout;
};

/****************************************************************************/


class sGeoBuffer11
{
public:
  sDNode Node;
  sGeoBuffer11(sDInt);
  ~sGeoBuffer11();

  struct ID3D11Buffer *DXBuffer;

  sDInt Alloc;
  sDInt Used;
  sDInt Free;

  sInt MappedCount;
  sU8 *MapPtr;
};

struct sGeoMapHandle
{
  sGeoBuffer11 *Buffer;
  void *Ptr;                // pointer where to write (including offset)
  sDInt Offset;             // offset for when using the buffer
};

class sGeoBufferManager
{
  sDList2<sGeoBuffer11> Free;
  sDList2<sGeoBuffer11> Used;
  sGeoBuffer11 *Current;
  sInt BlockSize;

  void NewBuffer();
public:
  sGeoBufferManager();
  ~sGeoBufferManager();

  void Map(sGeoMapHandle &hnd,sDInt bytes);
  void Unmap(sGeoMapHandle &hnd);
  void Flush();
};

class sGeometryPrivate
{
protected:
  struct Buffer
  {
    sInt ElementCount;
    sInt ElementSize;

    sGeoMapHandle DynMap;
    void *LoadBuffer;             // for static loading
    struct ID3D11Buffer *DXBuffer;
    struct ID3D11Buffer *GetBuffer()
    { return DynMap.Buffer ? DynMap.Buffer->DXBuffer : DXBuffer; }
    sDInt GetOffset()
    { return DynMap.Offset; }
    sBool IsEmpty()
    { return ElementCount==0; }
    void CloneFrom(sGeometryPrivate::Buffer *s);
  };
  Buffer VB[4];
  Buffer IB;
  sInt DXIndexFormat;
  sInt Topology;
  sInt Mapped;

  void BeginLoadPrv(Buffer *,sInt duration,void **vp,sInt bindflag);
  void EndLoadPrv(Buffer *,sInt count,sInt bindflag);
};

/****************************************************************************/

enum sCBufferManagerConsts
{
  sCBM_BinSize = 64,                 // each bin stands for a range of 64 bytes (4 regs)
  sCBM_BinCount = 32,                // there are 32 bins, so the largest cbuffer in bins is 2Kb
};

class sCBuffer11
{
public:
  sDNode Node;
  sCBuffer11(sInt size,sInt bin);
  ~sCBuffer11();

  struct ID3D11Buffer *DXBuffer;
  sInt Size;
  sInt Bin;
  sBool IsMapped;
};

struct sCBufferMap
{
  sCBuffer11 *Buffer;
  void *Ptr;
};

class sCBufferManager
{
  sDList2<sCBuffer11> Bins[sCBM_BinCount];   // bins[0] holds the extra-large cbuffers
  sDList2<sCBuffer11> Mapped;     // list of currently mapped cbuffers
  sDList2<sCBuffer11> Used;
public:
  sCBufferManager();
  ~sCBufferManager();

  void Map(sCBufferMap &map,sInt Size);
  void Unmap(sCBufferMap &map);
  void Flush();
};

class sCBufferBasePrivate
{
protected:
  void *DataPersist;
  void **DataPtr;
  sCBufferMap Map;
  sU64 Mask;
};

/****************************************************************************/

class sShaderPrivate
{
  friend class sMaterial;
  friend class sComputeShader;
protected:
  union
  {
    struct ID3D11VertexShader *vs;
    struct ID3D11HullShader *hs;
    struct ID3D11DomainShader *ds;
    struct ID3D11GeometryShader *gs;
    struct ID3D11PixelShader *ps;
    struct ID3D11ComputeShader *cs;
  };
};

/****************************************************************************/

class sTextureBasePrivate 
{
  friend void InitGFX();
  friend void ResizeGFX(sInt x,sInt y);
  friend void sSetTarget(const struct sTargetPara &para);
  friend void sCopyTexture(const struct sCopyTexturePara &para);
  friend void sBeginReadTexture(const sU8*& data, sS32& pitch, enum sTextureFlags& flags,class sTexture2D *tex);
  friend class sGpuToCpu;
  friend class sComputeShader;
  friend class sGeometry;
protected:
  union
  {
    struct ID3D11Texture2D *DXTex2D;
    struct ID3D11Texture3D *DXTex3D;
    struct ID3D11Buffer *DXBuffer;
  };
  struct ID3D11ShaderResourceView *DXTexView;
  struct ID3D11UnorderedAccessView *DXTexUAV;
  struct ID3D11RenderTargetView *DXRenderView;
  struct ID3D11DepthStencilView *DXDepthView;
  struct ID3D11RenderTargetView *DXCubeRenderView[6];
  sU8 *LoadPtr;
  sBool Dynamic;
  sInt LoadMipmap;
  sInt LoadCubeFace;
  sInt DXFormat;

  struct ID3D11Texture2D *DXReadTex;    // the first time a texture is read by CPU, a buffer gets allocated, that will not go away until the whole texture is deleted. because it is likely that the same texture is read again


  class sTexture2D *MultiSampled;   // an associated multisampled texture
  sBool NeedResolve;          // before using the non-multisampled, we need to resolve
  sBool NeedAutomipmap;

  sTextureBasePrivate();
public:
  void ResolvePrivate();
};

/****************************************************************************/

class sMaterialPrivate 
{
protected:
  struct StateObjects
  {
    struct ID3D11BlendState *BlendState;
    struct ID3D11DepthStencilState *DepthState;
    struct ID3D11RasterizerState *RasterState;
    struct ID3D11SamplerState *SamplerStates[sMTRL_MAXTEX];
    sVector4 BlendFactor;
    sInt StencilRef;
  };

  StateObjects *Variants;
};

/****************************************************************************/

struct sOccQueryNode
{
  sDNode Node;
  sInt Pixels;
  struct ID3D11Query *Query;
};

class sOccQueryPrivate 
{
protected:
  sOccQueryNode *Current;
  sDList2<sOccQueryNode> Queries;
};

/****************************************************************************/

class sGfxThreadContextPrivate
{
public:
  class sGfxThreadContext *OldGTC;
  struct ID3D11DeviceContext *DXCtx;
  struct ID3D11CommandList *DXCmd;
  class sCBufferBase *CurrentCBs[sCBUFFER_MAXSLOT*sCBUFFER_SHADERTYPES];
  class sTextureBase *CurrentTexture[sMTRL_MAXVSTEX+sMTRL_MAXPSTEX];
};

/****************************************************************************/

class sGpuToCpuPrivate 
{
protected:
  sInt SizeX,SizeY;
  sInt Flags;
  sInt DXFormat;
  sBool Locked;

  struct ID3D11Texture2D *Dest;
};

/****************************************************************************/

class sComputeShaderPrivate
{
public:
  static const sInt MaxTexture = 16;
  static const sInt MaxUAV = 4;
protected:

  struct ID3D11SamplerState *Sampler[MaxTexture];
  ID3D11ShaderResourceView *DXsrv[MaxTexture];
  ID3D11UnorderedAccessView *DXuavp[MaxUAV];
  sU32 DXuavc[MaxUAV];
};

/****************************************************************************/

#endif // FILE_BASE_GRAPHICS_DX11_PRIVATE_HPP


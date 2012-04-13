// This code is in the public domain. See LICENSE for details.

#ifndef __fvs_directx_h_
#define __fvs_directx_h_

#include "types.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d8.h>
#include "math3d_2.h"

#include "fvsconfig.h"

namespace fvs
{
  struct viewportStruct
  {
    sU32  xstart, ystart;
    sU32  xend, yend;
    sU32  width, height;
  };

  const sU32 STDVERTEX_FMT=D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX1;
  
  struct stdVertex
  {
    sF32  x, y, z, rhw;
    sU32  color;
		sU32	specular;
    sF32  u1, v1;
  };
  
  extern viewportStruct viewport;
  
  // ---- texturcode
  
  struct iFormatDesc;
  
  enum textureFlags
  {
    texfNone              = 0x0000,
    texfRGB               = 0x0001,
    texfGrayscale         = 0x0002,
    texfAlpha             = 0x0004,
    texfLowQual           = 0x0008,
    texfRTT               = 0x0010,
    texfMipMaps           = 0x0020,
    texfWrap              = 0x0040,
#ifdef FVS_SUPPORT_ANIM_TEXTURES
    texfAnimated          = 0x0080,
    texfNoLag             = 0x0100,
#endif
  };
  
  class FR_NOVTABLE baseTexture
  {
  protected:
    sBool       created;
    void        *resId;
    
    virtual void *getResID() const=0;
    
  public:
    baseTexture();
    virtual ~baseTexture();
    
    operator IDirect3DBaseTexture8 *() const;
    void     use(sU32 stage=0) const;
  };

  class texture: public baseTexture
  {
  private:
    sBool       locked;
    sU32        w, h, rw, rh;
    sF32        usc, uof, vsc, vof;
    sU32        createFlags;
    sU32        mipLevels;

#ifndef FVS_RGBA_TEXTURES_ONLY
    iFormatDesc *format;
#endif

    D3DPOOL     pool;
    virtual void *getResID() const;

#ifdef FVS_SUPPORT_ANIM_TEXTURES
    sInt        animFlags, animDisp;
    sBool       animChange;
    void        *addResIDs[2];

    friend void updateAnimatedTexturesPerFrame();
#endif

  public:
    texture();
    texture(sU32 w, sU32 h, sU32 flags);                 
    texture(const sU16 *img, sU32 w, sU32 h, sU32 flags);
    ~texture();

    operator IDirect3DTexture8 *() const;
    
    void  create(sU32 w, sU32 h, sU32 flags);
    void  destroy();

    sBool lock(sU32 level, D3DLOCKED_RECT *lr, const RECT *rect=0);
    void  unlock(sU32 level);

    sBool getDesc(sU32 level, D3DSURFACE_DESC *desc);

    IDirect3DSurface8 *getSurfaceLevel(sU32 level);

    void  upload(const sU16 *img);
    void  translate(sF32 *u, sF32 *v) const;
    void  translatePix(sF32 *u, sF32 *v) const;
    
    inline sU32 getWidth() const      { return w; }
    inline sU32 getHeight() const     { return h; }
  };
  
#ifdef FVS_SUPPORT_CUBEMAPS
  class cubeTexture: public baseTexture
  {
  private:
    sBool       locked;
    sU32        edge;
    sU32        createFlags;
    sU32        mipLevels;

#ifndef FVS_RGBA_TEXTURES_ONLY
    iFormatDesc *format;
#endif

    D3DPOOL     pool;

    virtual void *getResID() const;
    
  public:
    cubeTexture();
    cubeTexture(sU32 edge, sU32 flags);                 

    ~cubeTexture();

    operator  IDirect3DCubeTexture8 *() const;
    
    void      create(sU32 edge, sU32 flags);
    void      destroy();

    sBool     lock(sU32 face, sU32 level, D3DLOCKED_RECT *lr, const RECT *rect=0);
    void      unlock(sU32 face, sU32 level);
    
    sBool     getDesc(sU32 level, D3DSURFACE_DESC *desc);
    
    IDirect3DSurface8 *getSurfaceLevel(sU32 face, sU32 level);
    void      upload(sU32 face, const sU8 *img);
    
    inline sU32 getEdge() const   { return edge; }
  };
#endif

  // ---- enum stuff
  
	extern D3DCAPS8						deviceCaps;
  extern IDirect3D8         *d3d;
  extern IDirect3DDevice8   *d3ddev;
  
  // ---- configuration

  struct config
  {
    sS32      xRes, yRes, depth;
    D3DFORMAT fmt;
#ifdef FVS_SUPPORT_GAMMA
    sF32      gamma;
#endif
    sInt      device, mode;
#ifdef FVS_SUPPORT_WINDOWED
    sInt      windowed;
#endif
    sBool     lowtexq, notrilin, triplebuffer;
#ifdef FVS_SUPPORT_MULTITHREADING
    sBool     multithread;
#endif
  };
  
  extern config          conf;

  // ---- vertex/index buffer handling

  class FR_NOVTABLE bufSlice
  {
  protected:
    struct iD3DBuffer;

    sBool         created;
    sBool         locked;
    sBool         dynamic;
    sU32          FVF;
    sInt          streamIndex;
    sInt          nElements;
    sInt          elemBase;
    iD3DBuffer    *ibuf;

    static iD3DBuffer *d3dBuffers;

    static void globalCleanup();
    friend void closeDirectX();

    sU32 performCreate(sInt nElems, sBool isDynamic, sU32 VF, sInt streamInd, sU32 stride, sInt refSize, sU32 vto);

  public:
    bufSlice();
    ~bufSlice();

    void destroy();

    void *lock(sInt lockStart = 0, sInt lockSize = -1);
    void unlock();
    void fillFrom(const void *dataPtr, sInt count = -1);
  };

  class FR_NOVTABLE vbSlice: public bufSlice
  {
  public:
    vbSlice() { }
    vbSlice(sInt nVerts, sBool dynamic = sFALSE, sU32 VF = STDVERTEX_FMT, sInt streamInd = 0)
    {
      create(nVerts, dynamic, VF, streamInd);
    }

    void create(sInt nVerts, sBool dynamic = sFALSE, sU32 VF = STDVERTEX_FMT, sInt streamInd = 0);

    sU32 use() const;
  };

  class FR_NOVTABLE ibSlice: public bufSlice
  {
  public:
    ibSlice() { }
    ibSlice(sInt nInds, sBool dynamic = sFALSE)
    {
      create(nInds, dynamic);
    }

    void create(sInt nInds, sBool dynamic = sFALSE);

    sU32 use(sU32 startv) const;
  };
  
  // ---- dx api

  extern void initDirectX();
  extern void startupDirectX(HWND hWnd); // eigentlich HWND
  extern void closeDirectX();

  extern void updateScreen(HWND hWnd = 0, LPRECT rect = 0);
  extern void setViewport(sU32 xs, sU32 ys, sU32 w, sU32 h);

  extern void setTransform(sU32 state, const fr::matrix& matrix);

  extern void clear(sU32 mode = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, sU32 color = 0, sF32 z = 1.0f, sU32 stencil = 0);

  extern void drawPrimitive(sU32 type, sU32 startv, sU32 pcount);
  extern void drawIndexedPrimitive(sU32 type, sU32 startv, sU32 vcount, sU32 starti, sU32 pcount);

#ifdef FVS_SUPPORT_RENDER_TO_TEXTURE
  extern sU32 newRenderTarget(sU32 w, sU32 h, sBool zBuffer=sTRUE);
  extern void setRenderTarget(sU32 id, sBool force=sFALSE);
#endif
}

#endif

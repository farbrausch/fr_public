// This code is in the public domain. See LICENSE for details.

#ifndef __fvs_directx_h_
#define __fvs_directx_h_

#include "types.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
#include "pool.h"
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

  const sU32 STDVERTEX_FMT=D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX2;
  
  struct stdVertex
  {
    sF32  x, y, z, rhw;
    sU32  color;
    sF32  u1, v1;
    sF32  u2, v2;
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
    texfAnimated          = 0x0080,
    texfNoLag             = 0x0100,
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
    
    operator IDirect3DBaseTexture9 *() const;
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
    sInt        animFlags, animDisp;
    sBool       animChange;

    iFormatDesc *format;

    D3DPOOL     pool;
    void        *addResIDs[2];

    virtual void *getResID() const;

    friend void updateAnimatedTexturesPerFrame();

  public:
    texture();
    texture(sU32 w, sU32 h, sU32 flags);                 
    texture(const sU8 *img, sU32 w, sU32 h, sU32 flags);
    ~texture();

    operator IDirect3DTexture9 *() const;
    
    void  create(sU32 w, sU32 h, sU32 flags);
    void  destroy();

    sBool lock(sU32 level, D3DLOCKED_RECT *lr, const RECT *rect=0);
    void  unlock(sU32 level);

    sBool getDesc(sU32 level, D3DSURFACE_DESC *desc);

    IDirect3DSurface9 *getSurfaceLevel(sU32 level);

    void  upload(const sU8 *img);
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

    iFormatDesc *format;
    D3DPOOL     pool;

    virtual void *getResID() const;
    
  public:
    cubeTexture();
    cubeTexture(sU32 edge, sU32 flags);                 

    ~cubeTexture();

    operator  IDirect3DCubeTexture9 *() const;
    
    void      create(sU32 edge, sU32 flags);
    void      destroy();

    sBool     lock(sU32 face, sU32 level, D3DLOCKED_RECT *lr, const RECT *rect=0);
    void      unlock(sU32 face, sU32 level);
    
    sBool     getDesc(sU32 level, D3DSURFACE_DESC *desc);
    
    IDirect3DSurface9 *getSurfaceLevel(sU32 face, sU32 level);
    void      upload(sU32 face, const sU8 *img);
    
    inline sU32 getEdge() const   { return edge; }
  };
#endif

  // ---- texture pool

#ifdef FVS_SUPPORT_TEXTURE_POOL
  class texturePoolT: public fvs::pool<texture *>
  {
  protected:
    sU32  getFlags;
    
    virtual sBool rawGet(const sChar *id, texture **ptr, sInt *size);
    virtual void rawDestroy(texture *&ptr);
    
  public:
    void  setGetFlags(sU32 flags)       { getFlags=flags; }
    
    texture *load(const sChar *fileName, sU32 flags=texfWrap|texfMipMaps)
    {
      texture  *tex=0;
      
      setGetFlags(flags);
      get(fileName, &tex);
      
      return tex;
    }
  };
  
  extern texturePoolT *texturePool;
#endif
  
  // ---- enum stuff
  
  const sInt nDeviceTypes=2;
  
  struct outputInfo
  {
    D3DDEVTYPE        type;
    D3DCAPS9          caps;
  };
  
  struct deviceInfo
  {
    D3DADAPTER_IDENTIFIER9  adapterid;
    D3DDISPLAYMODE          mode;
    
    HMONITOR                monitor;
    
    sU32                    numModes;
    D3DDISPLAYMODE          *modes;
    
    outputInfo              outputs[nDeviceTypes];
    IDirect3DDevice9        *d3ddev;
  };

  extern sU32         numDevices;
  extern deviceInfo   *devices;
  extern deviceInfo   device;
  extern outputInfo   output;
  extern IDirect3D9        *d3d;
  extern IDirect3DDevice9  *d3ddev;
  
  // ---- configuration

  struct config
  {
    sS32      minXRes, minYRes;
    sS32      maxXRes, maxYRes;
    sS32      prefXRes, prefYRes, prefDepth;
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

  class FR_NOVTABLE vbSlice
  {
    struct        iVertexBuffer;
    
    sBool         created;
    sBool         locked;
    sBool         dynamic;
    sU32          FVF;
    sInt          streamIndex;
    sInt          nVertices;
    sInt          vBase;
    iVertexBuffer *ivb;
    
    static iVertexBuffer  *staticVertBuffers;
    static iVertexBuffer  *dynamicVertBuffers;
    
    static void globalCleanup();
    friend void closeDirectX();
    
  public:
    vbSlice();
    vbSlice(sInt nVerts, sBool dynamic=sFALSE, sU32 VF=STDVERTEX_FMT, sInt streamInd=0);
    ~vbSlice();
    
    void create(sInt nVerts, sBool dynamic=sFALSE, sU32 VF=STDVERTEX_FMT, sInt streamInd=0);
    void destroy();
    
    void *lock(sInt lockStart=0, sInt lockSize=-1);
    void unlock();
    
    void resize(sInt size);
    sU32 use() const;
  };
  
  class FR_NOVTABLE ibSlice
  {
    struct        iIndexBuffer;
    
    sBool         created;
    sBool         locked;
    sBool         dynamic;
    sInt          nIndices;
    sInt          iBase;
    iIndexBuffer  *iib;
    
    static iIndexBuffer   *staticIndBuffers;
    static iIndexBuffer   *dynamicIndBuffer;
    
    static void globalCleanup();
    friend void closeDirectX();
    
  public:
    ibSlice();
    ibSlice(sInt nInds, sBool dynamic=sFALSE);
    ~ibSlice();
    
    void create(sInt nInds, sBool dynamic=sFALSE);
    void destroy();
    
    void *lock(sInt lockStart=0, sInt lockSize=-1);
    void unlock();
    
    void resize(sInt size);
    sU32 use(sU32 startv=0) const;
  };
  
  // ---- misc stuff
  
  typedef void (__cdecl *idleHandler)();
  
  // ---- dx api

  extern void initDirectX();
  extern void startupDirectX(HWND hWnd); // eigentlich HWND
  extern void closeDirectX();
  
  extern void addIdleHandler(idleHandler hnd);

#ifdef FVS_SUPPORT_RESIZEABLE_BACKBUFFER
  extern void resizeBackbuffer(sInt xRes, sInt yRes);
#endif

  extern void updateScreen(HWND hWnd=0, LPRECT rect=0);
  extern void setViewport(sU32 xs, sU32 ys, sU32 w, sU32 h);

  extern void getTransform(sU32 state, fr::matrix &matrix);
  extern void setTransform(sU32 state, const fr::matrix &matrix);

  extern void clear(sU32 mode=D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, sU32 color=0, sF32 z=1.0f, sU32 stencil=0);

#ifdef FVS_SUPPORT_VERTEX_SHADERS
  extern sU32 createVertexShader(const sU32 *decl, const sU32 *func);
  extern void destroyVertexShader(sU32 id);

  extern sU32 getVertexShaderStride(sU32 id, sInt strm);
  extern void setVertexShaderStride(sU32 id, sInt strm, sU32 stride);
  extern sBool isVertexShader(sU32 id);

  extern void setVertexShaderConstant(sU32 constant, sF32 x=0.0f, sF32 y=0.0f, sF32 z=0.0f, sF32 w=1.0f);
  extern void setVertexShaderConstant(sU32 constant, const fr::vector &vector, sF32 w=1.0f);
  extern void setVertexShaderConstant(sU32 constant, const fr::matrix &matrix);
#endif  
  
#ifdef FVS_SUPPORT_USER_POINTER_DRAWING
  extern void drawLineList(const void *v, sU32 n, sU32 vertFormat=STDVERTEX_FMT);
  extern void drawLineStrip(const void *v, sU32 n, sU32 vertFormat=STDVERTEX_FMT);
  extern void drawTriList(const void *v, sU32 n, sU32 vertFormat=STDVERTEX_FMT);
  extern void drawTriFan(const void *v, sU32 n, sU32 vertFormat=STDVERTEX_FMT);
  extern void drawTriStrip(const void *v, sU32 n, sU32 vertFormat=STDVERTEX_FMT);
#endif

  extern void drawPrimitive(sU32 type, sU32 startv, sU32 pcount);
  extern void drawIndexedPrimitive(sU32 type, sU32 startv, sU32 vcount, sU32 starti, sU32 pcount);

  extern sU32 newRenderTarget(sU32 w, sU32 h, sBool zBuffer=sTRUE, sBool lockable=sFALSE);
  extern sU32 *lockRenderTarget(sU32 id);
  extern void unlockRenderTarget(sU32 id);
  extern void releaseRenderTarget(sU32 id);
  extern void getRenderTargetDimensions(sInt id, sInt *w, sInt *h, sInt *rw, sInt *rh);
  extern void setRenderTarget(sU32 id, sBool force=sFALSE);
}

#endif

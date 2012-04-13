// This code is in the public domain. See LICENSE for details.

#pragma warning (disable: 4786)

#include "types.h"
#include "debug.h"
#include "tool.h"
#include "directx.h"

#define  WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d8.h>

#pragma comment(lib, "d3d8.lib")

#ifndef FVS_TINY_ERROR_CHECKING
#pragma comment(lib, "dxerr8.lib")
#include <dxerr8.h>
#endif

#pragma warning (disable: 4390)
#pragma warning (disable: 4018)

namespace fvs
{
  struct            iZBufList;
  
  viewportStruct    viewport;
  config            conf;
	D3DCAPS8					deviceCaps;
  IDirect3D8*       d3d = 0;
  IDirect3DDevice8* d3ddev = 0;
  
  namespace
  {
    struct iRenderTarget
    {
      IDirect3DSurface8*    tgt;
      void*                 zbuf;
      texture*              texture;
      sU32                  id;
      sU32                  w, h;
      sU32                  rw, rh;
      sBool                 hasZB;
      iRenderTarget*        next;
    };

    static IDirect3DSurface8*     back;
    static HWND                   hDevWindow, hFocusWindow;
    static sBool					        inscene=sFALSE;
#ifdef FVS_SUPPORT_RENDER_TO_TEXTURE
    static iRenderTarget*         targets = 0;
		static iRenderTarget*					curtarget;
    static iZBufList*             zBuffers = 0;
#endif
    static D3DFORMAT              zbufFormat;
    static D3DPRESENT_PARAMETERS  pparm;
    
    static sBool										clearStencil=sFALSE;
    static IDirect3DVertexBuffer8*	oldVertexBuffer[8];
		static IDirect3DVertexBuffer8*	newVertexBuffer[8];
    static sU32											oldVBStride[8], newVBStride[8];
    static IDirect3DIndexBuffer8*		oldIndexBuffer;
		static IDirect3DIndexBuffer8*		newIndexBuffer;
    static sU32											oldIBStart, newIBStart;
    static sU32											oldVShader, newVShader;

    // ---- resource management

    class FR_NOVTABLE iResourceManager
    {
			struct resList
			{
				IUnknown*	id;
				resList*	next;
			};

			resList* rsrc;

    public:
      iResourceManager()
      {
        rsrc = new resList;
        rsrc->id = 0;
        rsrc->next = rsrc;
      }

      ~iResourceManager()
      {
        while (rsrc->next != rsrc)
          remove(rsrc->next->id);

        delete rsrc;
      }

      inline void* add(IUnknown* res)
      {
        resList *t = new resList;
        t->id = res;
        t->next = rsrc->next;
        rsrc->next = t;

        return res;
      }

      inline void remove(void* id)
      {
        ((IUnknown*) id)->Release();

        for (resList* t = rsrc; t->next != rsrc; t = t->next)
        {
          if (t->next->id == id)
          {
            resList* t2 = t->next;
            t->next = t2->next;
            delete t2;
          }
        }
      }

      inline IUnknown* getPtr(const void* id) const
      {
        return (IUnknown*) id;
      }

      inline void save()                        { }
      inline void kill()                        { }
      inline void recreate()                    { }
    };

    iResourceManager* resourceMgr = 0;
  }

  // ---- support stuff

#ifdef FVS_SUPPORT_RENDER_TO_TEXTURE
  static iRenderTarget *addRenderTarget(IDirect3DSurface8 *tgt)
  {
    iRenderTarget* t = new iRenderTarget;
    static sU32 IDCounter = 0;
    
    t->tgt = tgt;
    t->texture = 0;
    t->id = IDCounter++;
    t->next = targets;
    
    targets = t;
    return t;
  }
#endif
  
  static sBool checkDepthFormat(D3DFORMAT depthFormat)
  {
    if (d3d->CheckDeviceFormat(conf.device, D3DDEVTYPE_HAL, conf.fmt, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, depthFormat) != D3D_OK)
      return sFALSE;
    
    if (d3d->CheckDepthStencilMatch(conf.device, D3DDEVTYPE_HAL, conf.fmt, conf.fmt, depthFormat) != D3D_OK)
      return sFALSE;
    
    return sTRUE;
  }

  static HRESULT checkError(HRESULT hr, const sChar* str, sBool fatal = sTRUE)
  {
    if (hr != D3D_OK && fatal)
      fr::errorExit(str);

    return hr;
  }

  static sU32 getFVFSize(sU32 FVF)
  {
    sU32 size = 0, texcount;

    texcount = (FVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
  
    switch (FVF & D3DFVF_POSITION_MASK)
    {
    case D3DFVF_XYZ:    size += 12; break;
    case D3DFVF_XYZRHW: size += 16; break;
    case D3DFVF_XYZB1:  size += 16; break;
    case D3DFVF_XYZB2:  size += 20; break;
    case D3DFVF_XYZB3:  size += 24; break;
    case D3DFVF_XYZB4:  size += 28; break;
    case D3DFVF_XYZB5:  size += 32; break;
    }
  
    size += ((FVF & D3DFVF_DIFFUSE)  == D3DFVF_DIFFUSE)  ?  4 : 0;
    size += ((FVF & D3DFVF_NORMAL)   == D3DFVF_NORMAL)   ? 12 : 0;
    size += ((FVF & D3DFVF_PSIZE)    == D3DFVF_PSIZE)    ?  4 : 0;
    size += ((FVF & D3DFVF_SPECULAR) == D3DFVF_SPECULAR) ?  4 : 0;
  
    FVF >>= 16;
    for (sU32 i = 0; i < texcount; i++)
    {
      switch (FVF & 3)
      {
      case D3DFVF_TEXTUREFORMAT1: size += 4;  break;
      case D3DFVF_TEXTUREFORMAT2: size += 8;  break;
      case D3DFVF_TEXTUREFORMAT3: size += 12; break;
      case D3DFVF_TEXTUREFORMAT4: size += 16; break;
      }
    
      FVF >>= 2;
    }
  
    return size;
  }
  
  // ---- zbuffer sharing

#ifdef FVS_SUPPORT_RENDER_TO_TEXTURE
  struct iZBufList
  {
    sU32  w, h;
    void*	resid;
    sInt  refCount;

    iZBufList*	next;
  };

  static void* getZBuffer(sU32 w, sU32 h)
  {
    iZBufList* t = zBuffers;

    while (t)
    {
      if (t->w == w && t->h == h)
      {
        t->refCount++;
        return t->resid;
      }

      t = t->next;
    }

    IDirect3DSurface8* srf;
    checkError(d3ddev->CreateDepthStencilSurface(w, h, zbufFormat, D3DMULTISAMPLE_NONE, 
      &srf), "DCTX: couldn't create zbuffer surface for RTT targets");
    
    t = new iZBufList;
    t->w = w;
    t->h = h;
    t->resid = resourceMgr->add(srf);
    t->refCount = 1;
    t->next = zBuffers;
    zBuffers = t;

    return t->resid;
  }

  static void releaseZBuffer(void* ptr)
  {
    iZBufList* t = zBuffers, *t2;

    if (t->resid == ptr)
    {
      if (--t->refCount == 0)
      {
        resourceMgr->remove(ptr);
        t2 = t->next;
        delete t;
        zBuffers = t2;
      }

      return;
    }

    while (t->next)
    {
      if (t->next->resid == ptr)
      {
        if (--t->next->refCount == 0)
        {
          resourceMgr->remove(ptr);
          t2 = t->next;
          t->next = t2->next;
          delete t2;
        }

        return;
      }

      t = t->next;
    }
  }
#endif // FVS_SUPPORT_RENDER_TO_TEXTURE
  
  static void resetStateVars()
  {
    for (sInt i = 0; i < 8; i++)
    {
      oldVertexBuffer[i] = newVertexBuffer[i] = 0;
      oldVBStride[i] = newVBStride[i] = 0;
    }
    
    oldIndexBuffer = newIndexBuffer = 0;
    oldIBStart = newIBStart = 0;
    
    oldVShader = newVShader = 0;
  }

	static inline sInt getBackbufferFormatDepth(D3DFORMAT fmt)
	{
		return (fmt == D3DFMT_X8R8G8B8 || fmt == D3DFMT_A8R8G8B8) ? 32 : 16;
	}

	static D3DFORMAT backbufFormats[] = { D3DFMT_X1R5G5B5, D3DFMT_R5G6B5, D3DFMT_X8R8G8B8, D3DFMT_UNKNOWN }; // best last
	static D3DFORMAT zbufFormats[] = { D3DFMT_D15S1, D3DFMT_D16_LOCKABLE, D3DFMT_D16, D3DFMT_D24X4S4, D3DFMT_D24S8, D3DFMT_D24X8,
		D3DFMT_UNKNOWN }; // best last

	static void probeFormat(D3DFORMAT* destFormat, const D3DFORMAT* formatList, sBool depthProbe)
	{
		D3DFORMAT curFormat;

		*destFormat = D3DFMT_UNKNOWN;

		for (sInt i = 0; (curFormat = formatList[i]) != D3DFMT_UNKNOWN; i++)
		{
			if (SUCCEEDED(d3d->CheckDeviceFormat(conf.device, D3DDEVTYPE_HAL, depthProbe ? conf.fmt : curFormat,
				depthProbe ? D3DUSAGE_DEPTHSTENCIL : D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, curFormat)))
				*destFormat = curFormat;
		}

		if (*destFormat == D3DFMT_UNKNOWN)
			fr::errorExit("No suitable surface format found!");
	}

  void startupDirectX(HWND hWnd)
  {
		D3DFORMAT	modeFormat;

    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    if (!d3d)
      fr::errorExit("DirectX 8.1 required!\n");

    resourceMgr = new iResourceManager;

  	hDevWindow = 0;
    hFocusWindow = hWnd;

		probeFormat(&modeFormat, backbufFormats, sFALSE);

#ifdef FVS_SUPPORT_WINDOWED
		if (conf.windowed)
		{
			D3DDISPLAYMODE mode;
			checkError(d3d->GetAdapterDisplayMode(conf.device, &mode), "GetAdapterDisplayMode FAILED");
			modeFormat = mode.Format;
		}
#endif

		checkError(d3d->GetDeviceCaps(conf.device, D3DDEVTYPE_HAL, &deviceCaps), "GetDeviceCaps FAILED");

		conf.fmt = modeFormat;
		conf.depth = getBackbufferFormatDepth(conf.fmt);

    memset(&pparm, 0, sizeof(pparm));

#ifdef FVS_SUPPORT_WINDOWED
    if (!conf.windowed)
#endif
    {
      hDevWindow = hWnd;

      pparm.BackBufferWidth = conf.xRes;
      pparm.BackBufferHeight = conf.yRes;
      pparm.BackBufferFormat = modeFormat;
      pparm.BackBufferCount = conf.triplebuffer ? 2 : 1;

      pparm.MultiSampleType = D3DMULTISAMPLE_NONE;

      pparm.SwapEffect = D3DSWAPEFFECT_FLIP;
      pparm.hDeviceWindow = hDevWindow;
    }
#ifdef FVS_SUPPORT_WINDOWED
    else
    {
      pparm.BackBufferWidth = conf.xRes;
      pparm.BackBufferHeight = conf.yRes;
      pparm.BackBufferFormat = modeFormat;
      pparm.BackBufferCount = 1;

      pparm.MultiSampleType = D3DMULTISAMPLE_NONE;

      pparm.SwapEffect = D3DSWAPEFFECT_COPY;
      pparm.hDeviceWindow = 0;
      pparm.Windowed = TRUE;
      pparm.Flags = 0;
    }
#endif

		// zbuffer-format raussuchen
		probeFormat(&zbufFormat, zbufFormats, sTRUE);

		if (zbufFormat == D3DFMT_D24S8 || zbufFormat == D3DFMT_D15S1 || zbufFormat == D3DFMT_D24X4S4)
			clearStencil = sTRUE; // always clear stencil when we have it but won't use it anyway

		pparm.EnableAutoDepthStencil = TRUE;
		pparm.AutoDepthStencilFormat = zbufFormat;

    sU32 behavior = 0;

    if (deviceCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
      behavior |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
    else
      behavior |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;

    HRESULT hrCreate;
    sBool retried = sFALSE;
    
    do
    {
      hrCreate = d3d->CreateDevice(conf.device, D3DDEVTYPE_HAL, hWnd, behavior, &pparm, &d3ddev);

      if (hrCreate == D3DERR_OUTOFVIDEOMEMORY && !pparm.Windowed && !retried)
        retried = sTRUE;
      else
        checkError(hrCreate, "CreateDevice FAILED");
    } while (hrCreate != D3D_OK);

#ifdef FVS_SUPPORT_WINDOWED
    if (!conf.windowed)
#endif
    {
      ShowCursor(0);
      SetCursor(0);
    }

#ifdef FVS_SUPPORT_GAMMA
    D3DGAMMARAMP ramp;
    
    sF32 gv = 1.0f / conf.gamma;
    for (i = 0; i < 256; i++)
      ramp.red[i] = ramp.green[i] = ramp.blue[i] = 65535.0f * pow((sF32) i / 255.0f, gv);

    d3ddev->SetGammaRamp(D3DSGR_NO_CALIBRATION, &ramp);
#endif

    const sInt xRes = conf.xRes, yRes = conf.yRes;
    checkError(d3ddev->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back), "Couldn't get Backbuffer");
    setViewport(0, 0, xRes, yRes);

#ifdef FVS_SUPPORT_RENDER_TO_TEXTURE
    curtarget = addRenderTarget(back);
    curtarget->rw = curtarget->w = xRes;
    curtarget->rh = curtarget->h = yRes;
    curtarget->zbuf = 0;
    curtarget->hasZB = sTRUE;
#endif

    resetStateVars();
  }

  void closeDirectX()
  {
#ifdef FVS_SUPPORT_RENDER_TO_TEXTURE
    iRenderTarget* t;
		iRenderTarget* t2;

    for (t = targets; t; t = t2)
    {
      t2 = t->next;
      if (t->tgt)
        t->tgt->Release();

      delete t; // die texturen werden vom texturcode gefreed
    }

  	while (zBuffers)
      releaseZBuffer(zBuffers->resid); // hack
#endif

    bufSlice::globalCleanup();
    
    FRSAFEDELETE(resourceMgr);

		if (back)
			back->Release();

    if (d3ddev)
    {
      d3ddev->Release();
      d3ddev = 0;
    }

    if (d3d)
    {
      d3d->Release();
      d3d = 0;
    }
  }

  void doSetRenderTarget(iRenderTarget* tgt, sBool force = sFALSE);

  // -------------------------------------------------------------------------------------------

  void updateScreen(HWND hWnd, LPRECT rect)
  {
    if (!d3ddev)
      return;

    if (inscene)
    {
			d3ddev->EndScene();
			inscene = sFALSE;
    }

    d3ddev->Present(0, rect, hWnd, 0);

#ifdef FVS_SUPPORT_RENDER_TO_TEXTURE
    setRenderTarget(0, sTRUE);
#endif
#ifdef FVS_SUPPORT_ANIM_TEXTURES
    updateAnimatedTexturesPerFrame();
#endif
  }

  void setViewport(sU32 xs, sU32 ys, sU32 w, sU32 h)
  {
    viewportStruct* vp = &viewport;

    if (!d3ddev)
      return;

    D3DVIEWPORT8 dvp = { xs, ys, w, h, 0.0f, 1.0f };

		d3ddev->SetViewport(&dvp);
    vp->xstart=xs;
    vp->ystart=ys;
    vp->width=w;
    vp->height=h;
    vp->xend=xs+w;
    vp->yend=ys+h;
  }

  void setTransform(sU32 state, const fr::matrix& tr)
  {
    if (d3ddev)
      d3ddev->SetTransform((D3DTRANSFORMSTATETYPE) state, tr.d3d());
  }

  static void __forceinline setVertexBuffer(sU32 strm, IDirect3DVertexBuffer8* vbuf, sU32 stride)
  {
    newVertexBuffer[strm] = vbuf;
    newVBStride[strm] = stride;
  }

  static void __forceinline setIndexBuffer(IDirect3DIndexBuffer8* ibuf, sU32 start)
  {
    newIndexBuffer = ibuf;
    newIBStart = start;
  }

  static void __forceinline setVertexShader(sU32 shader)
  {
    newVShader = shader;
  }

  static void updateStates()
  {
    if (!d3ddev)
      return;

    if (newVShader!=oldVShader)
    {
      checkError(d3ddev->SetVertexShader(newVShader), "SetVertexShader FAILED", sFALSE);
      oldVShader = newVShader;
    }

    for (sInt i = 0; i < 8; i++)
    {
      if (newVertexBuffer[i] != oldVertexBuffer[i] || newVBStride[i] != oldVBStride[i])
      {
        checkError(d3ddev->SetStreamSource(i, newVertexBuffer[i], newVBStride[i]), "SetStreamSource FAILED", sFALSE);
        oldVertexBuffer[i] = newVertexBuffer[i];
        oldVBStride[i] = newVBStride[i];
      }
    }

    if (newIndexBuffer != oldIndexBuffer || newIBStart != oldIBStart)
    {
      checkError(d3ddev->SetIndices(newIndexBuffer, newIBStart), "SetIndices FAILED", sFALSE);
      oldIndexBuffer = newIndexBuffer;
      oldIBStart = newIBStart;
    }
  }

  void clear(sU32 mode, sU32 color, sF32 z, sU32 stencil)
  {
    if (!d3ddev)
      return;

#ifdef FVS_SUPPORT_RENDER_TO_TEXTURE
    if (!curtarget->hasZB)
      mode &= ~D3DCLEAR_ZBUFFER;
#endif

    if (clearStencil && (mode & D3DCLEAR_ZBUFFER)
#ifdef FVS_SUPPORT_RENDER_TO_TEXTURE
      && !curtarget->texture)
#else
      )
#endif
      mode|=D3DCLEAR_STENCIL;

  	if (!inscene)
  	{
			d3ddev->BeginScene();
			inscene = sTRUE;
  	}

#ifdef FVS_SUPPORT_RENDER_TO_TEXTURE
    sBool setVp = (curtarget != targets);

    D3DVIEWPORT8 tvp = {0, 0, curtarget->rw, curtarget->rh, 0.0f, 1.0f};

		if (setVp)
			d3ddev->SetViewport(&tvp);
#endif

		d3ddev->Clear(0, 0, mode, color, z, stencil);

#ifdef FVS_SUPPORT_RENDER_TO_TEXTURE
    tvp.X = viewport.xstart;
    tvp.Y = viewport.ystart;
    tvp.Width = viewport.width;
    tvp.Height = viewport.height;

		if (setVp)
			d3ddev->SetViewport(&tvp);
#endif
  }

  // ---------------------------------------------------------------------------------------------
  // vertex buffer drawing calls
  // ---------------------------------------------------------------------------------------------

  void drawPrimitive(sU32 type, sU32 startv, sU32 pcount)
  {
    if (d3ddev && pcount)
    {
      updateStates();
			d3ddev->DrawPrimitive((D3DPRIMITIVETYPE) type, startv, pcount);
    }
  }

  void drawIndexedPrimitive(sU32 type, sU32 startv, sU32 vcount, sU32 starti, sU32 pcount)
  {
    if (d3ddev && vcount && pcount)
    {
      updateStates();
			d3ddev->DrawIndexedPrimitive((D3DPRIMITIVETYPE) type, startv, vcount, starti, pcount);
    }
  }

  // ---------------------------------------------------------------------------------------------

#ifdef FVS_SUPPORT_RENDER_TO_TEXTURE
  sU32 newRenderTarget(sU32 w, sU32 h, sBool zBuffer)
  {
		IDirect3DSurface8*	srf;
    D3DSURFACE_DESC			desc;
		iRenderTarget*			rt;
    texture*						tex;

    tex = new texture(w, h, texfRGB | texfRTT);
    srf = tex->getSurfaceLevel(0);

    if (!srf)
      fr::errorExit("GetSurfaceLevel FAILED");

    rt = addRenderTarget(0);

    memset(&desc, 0, sizeof(desc));
    desc.Size = sizeof(desc);
    checkError(srf->GetDesc(&desc), "GetDesc FAILED");

    rt->texture = tex;
    rt->w = w;
    rt->h = h;
    rt->rw = desc.Width;
    rt->rh = desc.Height;

    srf->Release();

    if (zBuffer)
    {
      if (!getZBuffer(rt->rw, rt->rh))
        fr::errorExit("Couldn't get matching ZBuffer for Rendertarget %d", rt->id);

      rt->hasZB = sTRUE;
    }
    else
    {
      rt->zbuf = 0;
      rt->hasZB = sFALSE;
    }

    return rt->id;
  }

  static void doSetRenderTarget(iRenderTarget* tgt, sBool force)
  {
    IDirect3DSurface8*	target;

  	if (!d3ddev || !tgt)
  		return;

    if (curtarget != tgt || force)
    {
  		if (inscene)
  		{
				d3ddev->EndScene();
				inscene = sFALSE;
  		}

      if (tgt->tgt)
        target = tgt->tgt;
      else
        target = tgt->texture->getSurfaceLevel(0);

      IDirect3DSurface8* zb = tgt->zbuf ? (IDirect3DSurface8 *) resourceMgr->getPtr(tgt->zbuf) : zbuffer;
			if (!tgt->hasZB)
				zb = 0;
      
			d3ddev->SetRenderTarget(target, zb);
      setRenderState(D3DRS_ZENABLE, !!tgt->hasZB);

      if (!tgt->tgt)
        target->Release();

      curtarget = tgt;
    }

    if (!inscene)
  	{
			d3ddev->BeginScene();
			inscene = sTRUE;
  	}
  }

  void setRenderTarget(sU32 id, sBool force)
  {
		iRenderTarget* t;

    for (t = targets; t; t = t->next)
    {
      if (t->id == id)
        break;
    }

		doSetRenderTarget(t ? t : targets, force);
  }
#endif // FVS_SUPPORT_RENDER_TO_TEXTURE

  // ---------------------------------------------------------------------------------------------
  // texturcode
  // ---------------------------------------------------------------------------------------------

  // ---- pixel format conversion

#ifndef FVS_RGBA_TEXTURES_ONLY

	struct iFormatDesc;

  typedef void (*convertFunc)(sU8 *, const sU8 *, sU32, sInt, iFormatDesc *);

  struct iFormatDesc
  {
    D3DFORMAT     fmt;
    sU8           asl, asr, rsl, rsr, gsl, gsr, bsl, bsr;
    sInt          bits;
    convertFunc   func;
  };

  static void convertLineRGB(sU8 *dest, const sU8 *src, sU32 w, sInt bits, iFormatDesc *fd)
  {
    sU32  out;

    while (w--)
    {
      out=((src[2]>>fd->rsr)<<fd->rsl)|((src[1]>>fd->gsr)<<fd->gsl)|((src[0]>>fd->bsr)<<fd->bsl);

      *dest++=out;
      if (bits>8)   *dest++=out>>8;
      if (bits>16)  *dest++=out>>16;
      if (bits>24)  *dest++=out>>24;
      src+=3;
    }
  }

  static void convertLineLum(sU8 *dest, const sU8 *src, sU32 w, sInt bits, iFormatDesc *fd)
  {
    sU32  out;

    while (w--)
    {
      out=((*src>>fd->rsr)<<fd->rsl)|((*src>>fd->gsr)<<fd->gsl)|((*src>>fd->bsr)<<fd->bsl);
      *dest++=out;
      if (bits>8)   *dest++=out>>8;
      if (bits>16)  *dest++=out>>16;
      if (bits>24)  *dest++=out>>24;
      src++;
    }
  }

  static void convertLineAlpha(sU8 *dest, const sU8 *src, sU32 w, sInt bits, iFormatDesc *fd)
  {
    sU32  out, base;

    base=((0xff>>fd->rsr)<<fd->rsl)|((0xff>>fd->gsr)<<fd->gsl)|((0xff>>fd->bsr)<<fd->bsl);

    while (w--)
    {
      out=base|((*src>>asr)<<asl);

      *dest++=out;
      if (bits>8)   *dest++=out>>8;
      if (bits>16)  *dest++=out>>16;
      if (bits>24)  *dest++=out>>24;
      src++;
    }
  }

  static void convertLineRGBA(sU8 *dest, const sU8 *src, sU32 w, sInt bits, iFormatDesc *fd)
  {
    sU32  out;

    while (w--)
    {
      out=((src[3]>>fd->asr)<<fd->asl)|((src[2]>>fd->rsr)<<fd->rsl)|((src[1]>>fd->gsr)<<fd->gsl)|((src[0]>>fd->bsr)<<fd->bsl);

      *dest++=out;
      if (bits>8)   *dest++=out>>8;
      if (bits>16)  *dest++=out>>16;
      if (bits>24)  *dest++=out>>24;
      src+=4;
    }
  }

  static void convertLineLumA(sU8 *dest, const sU8 *src, sU32 w, sInt bits, iFormatDesc *fd)
  {
    sU32  out;

    while (w--)
    {
      out=((src[1]>>fd->asr)<<fd->asl)|((*src>>fd->rsr)<<fd->rsl)|((*src>>fd->gsr)<<fd->gsl)|((*src>>fd->bsr)<<fd->bsl);
      *dest++=out;
      if (bits>8)   *dest++=out>>8;
      if (bits>16)  *dest++=out>>16;
      if (bits>24)  *dest++=out>>24;
      src+=2;
    }
  }

	static iFormatDesc formatsINVL[]={
    D3DFMT_UNKNOWN,    0,0,  0,0, 0,0, 0,0,   0, 0
  };

  static iFormatDesc formatsRGB_[]={
    D3DFMT_R8G8B8,     0,0, 16,0, 8,0, 0,0,  24, convertLineRGB,
    D3DFMT_X8R8G8B8,   0,0, 16,0, 8,0, 0,0,  32, convertLineRGB,
    D3DFMT_A8R8G8B8,   0,0, 16,0, 8,0, 0,0,  32, convertLineRGB,
    D3DFMT_R5G6B5,     0,0, 11,3, 5,2, 0,3,  16, convertLineRGB,
    D3DFMT_X1R5G5B5,   0,0, 10,3, 5,3, 0,3,  16, convertLineRGB,
    D3DFMT_A1R5G5B5,   0,0, 10,3, 5,3, 0,3,  16, convertLineRGB,
    D3DFMT_X4R4G4B4,   0,0,  8,4, 4,4, 0,4,  16, convertLineRGB,
    D3DFMT_A4R4G4B4,   0,0,  8,4, 4,4, 0,4,  16, convertLineRGB,
    D3DFMT_R3G3B2,     0,0,  5,5, 2,5, 0,6,   8, convertLineRGB,
    D3DFMT_A8R3G3B2,   8,0,  5,5, 2,5, 0,6,  16, convertLineRGB,
    D3DFMT_UNKNOWN,    0,0,  0,0, 0,0, 0,0,   0, 0
  };

  static iFormatDesc formatsRGBL[]={
    D3DFMT_R5G6B5,     0,0, 11,3, 5,2, 0,3,  16, convertLineRGB,
    D3DFMT_X1R5G5B5,   0,0, 10,3, 5,3, 0,3,  16, convertLineRGB,
    D3DFMT_A1R5G5B5,   0,0, 10,3, 5,3, 0,3,  16, convertLineRGB,
    D3DFMT_X4R4G4B4,   0,0,  8,4, 4,4, 0,4,  16, convertLineRGB,
    D3DFMT_A4R4G4B4,   0,0,  8,4, 4,4, 0,4,  16, convertLineRGB,
    D3DFMT_R8G8B8,     0,0, 16,0, 8,0, 0,0,  24, convertLineRGB,
    D3DFMT_X8R8G8B8,   0,0, 16,0, 8,0, 0,0,  32, convertLineRGB,
    D3DFMT_A8R8G8B8,   0,0, 16,0, 8,0, 0,0,  32, convertLineRGB,
    D3DFMT_R3G3B2,     0,0,  5,5, 2,5, 0,6,   8, convertLineRGB,
    D3DFMT_A8R3G3B2,   8,0,  5,5, 2,5, 0,6,  16, convertLineRGB,
    D3DFMT_UNKNOWN,    0,0,  0,0, 0,0, 0,0,   0, 0
  };

  static iFormatDesc formatsAlph[]={
    D3DFMT_A8,         0,0,  0,8, 0,8, 0,8,   8, convertLineAlpha,
    D3DFMT_A8L8,       8,0,  0,0, 0,8, 0,8,  16, convertLineAlpha,
    D3DFMT_A8R3G3B2,   8,0,  5,5, 2,5, 0,6,  16, convertLineAlpha,
    D3DFMT_A8R8G8B8,  24,0, 16,0, 8,0, 0,0,  32, convertLineAlpha,
    D3DFMT_A4L4,       4,4,  0,4, 0,8, 0,8,   8, convertLineAlpha,
    D3DFMT_A4R4G4B4,  12,4,  8,4, 4,4, 0,4,  16, convertLineAlpha,
    D3DFMT_A1R5G5B5,  15,7, 10,3, 5,3, 0,3,  16, convertLineAlpha,
    D3DFMT_UNKNOWN,    0,0,  0,0, 0,0, 0,0,   0, 0
  };

  static iFormatDesc formatsAlpL[]={
    D3DFMT_A8,         0,0,  0,8, 0,8, 0,8,   8, convertLineAlpha,
    D3DFMT_A8L8,       8,0,  0,0, 0,8, 0,8,  16, convertLineAlpha,
    D3DFMT_A4L4,       4,4,  0,4, 0,8, 0,8,   8, convertLineAlpha,
    D3DFMT_A4R4G4B4,  12,4,  8,4, 4,4, 0,4,  16, convertLineAlpha,
    D3DFMT_A8R8G8B8,  24,0, 16,0, 8,0, 0,0,  32, convertLineAlpha,
    D3DFMT_A1R5G5B5,  15,7, 10,3, 5,3, 0,3,  16, convertLineAlpha,
    D3DFMT_UNKNOWN,    0,0,  0,0, 0,0, 0,0,   0, 0
  };

  static iFormatDesc formatsLumi[]={
    D3DFMT_L8,         0,0,  0,0, 0,8, 0,8,   8, convertLineLum,
    D3DFMT_A8L8,       0,0,  0,0, 0,8, 0,8,  16, convertLineLum,
    D3DFMT_R8G8B8,     0,0, 16,0, 8,0, 0,0,  24, convertLineLum,
    D3DFMT_X8R8G8B8,   0,0, 16,0, 8,0, 0,0,  32, convertLineLum,
    D3DFMT_A8R8G8B8,   0,0, 16,0, 8,0, 0,0,  32, convertLineLum,
    D3DFMT_X1R5G5B5,   0,0, 10,3, 5,3, 0,3,  16, convertLineLum,
    D3DFMT_A1R5G5B5,   0,0, 10,3, 5,3, 0,3,  16, convertLineLum,
    D3DFMT_R5G6B5,     0,0, 11,3, 5,2, 0,3,  16, convertLineLum,
    D3DFMT_X4R4G4B4,   0,0,  8,4, 4,4, 0,4,  16, convertLineLum,
    D3DFMT_A4R4G4B4,   0,0,  8,4, 4,4, 0,4,  16, convertLineLum,
    D3DFMT_R3G3B2,     0,0,  5,5, 2,5, 0,6,   8, convertLineLum,
    D3DFMT_A8R3G3B2,   8,0,  5,5, 2,5, 0,6,  16, convertLineLum,
    D3DFMT_UNKNOWN,    0,0,  0,0, 0,0, 0,0,   0, 0
  };

  static iFormatDesc formatsLumL[]={
    D3DFMT_L8,         0,0,  0,0, 0,8, 0,8,   8, convertLineLum,
    D3DFMT_A8L8,       0,0,  0,0, 0,8, 0,8,  16, convertLineLum,
    D3DFMT_X1R5G5B5,   0,0, 10,3, 5,3, 0,3,  16, convertLineLum,
    D3DFMT_A1R5G5B5,   0,0, 10,3, 5,3, 0,3,  16, convertLineLum,
    D3DFMT_R5G6B5,     0,0, 11,3, 5,2, 0,3,  16, convertLineLum,
    D3DFMT_X4R4G4B4,   0,0,  8,4, 4,4, 0,4,  16, convertLineLum,
    D3DFMT_A4R4G4B4,   0,0,  8,4, 4,4, 0,4,  16, convertLineLum,
    D3DFMT_R8G8B8,     0,0, 16,0, 8,0, 0,0,  24, convertLineLum,
    D3DFMT_X8R8G8B8,   0,0, 16,0, 8,0, 0,0,  32, convertLineLum,
    D3DFMT_A8R8G8B8,   0,0, 16,0, 8,0, 0,0,  32, convertLineLum,
    D3DFMT_R3G3B2,     0,0,  5,5, 2,5, 0,6,   8, convertLineLum,
    D3DFMT_A8R3G3B2,   8,0,  5,5, 2,5, 0,6,  16, convertLineLum,
    D3DFMT_UNKNOWN,    0,0,  0,0, 0,0, 0,0,   0, 0
  };

  static iFormatDesc formatsRGBA[]={
    D3DFMT_A8R8G8B8,  24,0, 16,0, 8,0, 0,0,  32, convertLineRGBA,
    D3DFMT_A4R4G4B4,  12,4,  8,4, 4,4, 0,4,  16, convertLineRGBA,
    D3DFMT_A8R3G3B2,   8,0,  5,5, 2,5, 0,6,  16, convertLineRGBA,
    D3DFMT_A1R5G5B5,  15,7, 10,3, 5,3, 0,3,  16, convertLineRGBA,
    D3DFMT_UNKNOWN,    0,0,  0,0, 0,0, 0,0,   0, 0
  };

  static iFormatDesc formatsRGAL[]={
    D3DFMT_A4R4G4B4,  12,4,  8,4, 4,4, 0,4,  16, convertLineRGBA,
    D3DFMT_A8R8G8B8,  24,0, 16,0, 8,0, 0,0,  32, convertLineRGBA,
    D3DFMT_A1R5G5B5,  15,7, 10,3, 5,3, 0,3,  16, convertLineRGBA,
    D3DFMT_A8R3G3B2,   8,0,  5,5, 2,5, 0,6,  16, convertLineRGBA,
    D3DFMT_UNKNOWN,    0,0,  0,0, 0,0, 0,0,   0, 0
  };

  static iFormatDesc formatsLumA[]={
    D3DFMT_A8L8,       0,0,  0,0, 0,8, 0,8,  16, convertLineLumA,
    D3DFMT_A8R8G8B8,   0,0, 16,0, 8,0, 0,0,  32, convertLineLumA,
    D3DFMT_A4R4G4B4,   0,0,  8,4, 4,4, 0,4,  16, convertLineLumA,
    D3DFMT_A8R3G3B2,   8,0,  5,5, 2,5, 0,6,  16, convertLineLumA,
    D3DFMT_A1R5G5B5,   0,0, 10,3, 5,3, 0,3,  16, convertLineLumA,
    D3DFMT_UNKNOWN,    0,0,  0,0, 0,0, 0,0,   0, 0
  };

  static iFormatDesc formatsLuAL[]={
    D3DFMT_A8L8,       0,0,  0,0, 0,8, 0,8,  16, convertLineLumA,
    D3DFMT_A4R4G4B4,   0,0,  8,4, 4,4, 0,4,  16, convertLineLumA,
    D3DFMT_A8R8G8B8,   0,0, 16,0, 8,0, 0,0,  32, convertLineLumA,
    D3DFMT_A8R3G3B2,   8,0,  5,5, 2,5, 0,6,  16, convertLineLumA,
    D3DFMT_A1R5G5B5,   0,0, 10,3, 5,3, 0,3,  16, convertLineLumA,
    D3DFMT_UNKNOWN,    0,0,  0,0, 0,0, 0,0,   0, 0
  };

  static iFormatDesc *formatTable[16]={
    formatsINVL, formatsRGB_, formatsLumi, formatsINVL,
    formatsAlph, formatsRGBA, formatsLumA, formatsINVL,
    formatsINVL, formatsRGBL, formatsLumL, formatsINVL,
    formatsAlpL, formatsRGAL, formatsLuAL, formatsINVL
  };

  static sChar formatSize[16]={
    0, 3, 1, 0,
    1, 4, 2, 0,
    0, 3, 1, 0,
    1, 4, 2, 0
  };
#endif

  // ---- texture image processing

  static void resampleImage(const sU8 *inp, sU32 sw, sU32 sh, sU8 *dst, sU32 dw, sU32 dh, sInt cpp)
  {
    const sU8 *srct, *src;
    sU32      bpl=cpp*sw, x, y, c, xr, yr, xs, ys;

    if (sw==dw && sh==dh)
    {
      memcpy(dst, inp, cpp*sw*sh);
      return;
    }

    srct=src=inp;
    xs=(sw<<16)/dw;
    ys=(sh<<16)/dh;

    // LAHM.

    for (y=0, yr=0; y<dh; y++, yr+=ys, srct=inp+((yr>>16)*sw*cpp))
    {
      for (x=0, xr=0; x<dw; x++, dst+=cpp, xr+=xs, src=srct+((xr>>16)*cpp))
      {
        for (c=0; c<cpp; c++)
        {
          sU8 t1, t2;

          if (xr>>16<sw-1)
          {
            t1=src[c]+(((src[c+cpp]-src[c])*(xr&0xffff))>>16);
            if (yr>>16<sh-1)
              t2=src[c+bpl]+(((src[c+bpl+cpp]-src[c+bpl])*(xr&0xffff))>>16);
            else
              t2=src[c];
          }
          else
            t1=t2=src[c];

          
          dst[c]=t1+(((t2-t1)*(yr&0xffff))>>16);
        }
      }
    }
  }

  static void generateMipMap(const sU8 *inp, sU32 sw, sU32 sh, sU8 *dst, sInt cpp)
  {
#if !(defined(FVS_RGBA_TEXTURES_ONLY) && defined(FVS_TINY_TEXTURE_CODE))
    sInt  x, y, c, lstep, lsteph, o, io;

    FRDASSERT(inp && dst && cpp);
    FRDASSERT((sw>>1) && (sh>>1));

    lstep=sw*cpp;
    lsteph=(sw>>1)*cpp;

    for (y=0, o=0, io=0; y<(sh/2); y++, io+=lstep)
    {
      for (x=0; x<(sw/2); x++, io+=cpp)
      {
        for (c=0; c<cpp; c++, o++, io++)
        {
          dst[o]=(inp[io] + inp[io+cpp] + inp[io+lstep] + inp[io+lstep+cpp] + 2) >> 2;
        }
      }
    }
#else
		static const sU64 addc = 0x0002000200020002;

		__asm
		{
			mov				esi, [inp];
			mov				edi, [dst];
			mov				ecx, [sw];
			mov				edx, [sh];
			shr				edx, 1;
			lea				ebx, [esi + ecx * 4];
			lea       edi, [edi + ecx * 2];
			lea       esi, [ebx + ecx * 4];
			emms;
			pxor			mm7, mm7;
			movq			mm4, [addc];

linelp:
			mov				eax, ecx;
			shr				eax, 1;
			neg       eax;

pixlp:
			movq			mm0, [esi + eax * 8];
			movq			mm1, [ebx + eax * 8];
			movq			mm2, mm0;
			movq			mm3, mm1;
			punpcklbw	mm0, mm7;
			punpcklbw	mm1, mm7;
			punpckhbw	mm2, mm7;
			punpckhbw	mm3, mm7;
			paddw			mm0, mm1;
			paddw			mm2, mm3;
			paddw			mm0, mm2;
			paddw			mm0, mm4;
			psrlw			mm0, 2;
			packuswb	mm0, mm7;
			movd			[edi + eax * 4], mm0;
			inc       eax;
			jnz				pixlp;

			lea       esi, [esi + ecx * 8];
			lea       ebx, [ebx + ecx * 8];
			lea       edi, [edi + ecx * 2];
			dec				edx;
			jnz				linelp;

			emms;
		}
#endif
  }
  
  // ---- support functions

  static sBool checkTextureFormat(D3DFORMAT fmt, sInt flags)
  {
		return d3d->CheckDeviceFormat(conf.device, D3DDEVTYPE_HAL, conf.fmt, (flags & texfRTT) ? D3DUSAGE_RENDERTARGET : 0,
			D3DRTYPE_TEXTURE, fmt) == D3D_OK;
  }

#ifdef FVS_SUPPORT_CUBEMAPS
  static sBool checkCubemapFormat(D3DFORMAT fmt, sInt flags)
  {
		return d3d->CheckDeviceFormat(conf.device, D3DDEVTYPE_HAL, conf.fmt, (flags & texfRTT) ? D3DUSAGE_RENDERTARGET : 0,
			D3DRTYPE_CUBETEXTURE, fmt) == D3D_OK;
  }
#endif
  
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

#ifdef FVS_SUPPORT_ANIM_TEXTURES
  // ---- support für animated textures

  struct iAnimatedTexture
  {
    texture           *tex;
    iAnimatedTexture  *next;
  };

  static iAnimatedTexture   *animTextures=0;
#endif

  // ---- fvsBaseTexture

  baseTexture::baseTexture()
  {
    created=sFALSE;
    resId=0;
  }

  baseTexture::~baseTexture()
  {
    FRDASSERT(!created);
  }

  baseTexture::operator IDirect3DBaseTexture8 *() const
  {
    if (this)
    {
      FRDASSERT(created);
      
      return (IDirect3DBaseTexture8 *) resourceMgr->getPtr(getResID());
    }
    else
      return 0;
  }

  void baseTexture::use(sU32 stage) const
  {
    if (this)
    {
      FRDASSERT(created);
      d3ddev->SetTexture(stage, (IDirect3DBaseTexture8 *) resourceMgr->getPtr(getResID()));
    }
    else
      d3ddev->SetTexture(stage, 0);
  }
  
  // ---- texture

  texture::texture()
  {
  }

  texture::texture(sU32 w, sU32 h, sU32 flags)
  {
    create(w, h, flags);
  }

  texture::texture(const sU16 *img, sU32 w, sU32 h, sU32 flags)
  {
    create(w, h, flags);
    upload(img);
  }

  texture::~texture()
  {
    if (created)
      destroy();
  }

  void *texture::getResID() const
  {
    void *res=resId;

#ifdef FVS_SUPPORT_ANIM_TEXTURES
    if (animFlags==1)
      res=addResIDs[0];
    else if (animFlags==2)
      res=addResIDs[animDisp];
#endif

    return res;
  }

  texture::operator IDirect3DTexture8 *() const
  {
    if (this)
    {
      FRDASSERT(created);

      return (IDirect3DTexture8 *) resourceMgr->getPtr(getResID());
    }
    else
      return 0;
  }

  void texture::create(sU32 w, sU32 h, sU32 flags)
  {
    IDirect3DTexture8 *tex;
    sU32              usage;
    sU32              wn, hn, i;

#ifdef FVS_SUPPORT_RENDER_TO_TEXTURE
    if (flags & texfRTT) 
      flags&=~texfMipMaps;
#endif

#ifndef FVS_TINY_TEXTURE_CODE
    if (output.caps.TextureCaps & D3DPTEXTURECAPS_POW2)
    {
      for (wn=1; wn<w; wn<<=1);
      for (hn=1; hn<h; hn<<=1);
    }
    else
    {
      wn=w;
      hn=h;
    }
    
    if (output.caps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
      if (wn>hn)
        hn=wn;
      else
        wn=hn;
      
    wn=MIN(wn, output.caps.MaxTextureWidth);
    hn=MIN(hn, output.caps.MaxTextureHeight);
#else
    wn=w;
    hn=h;
#endif

    // passendes texturformat suchen
    
#ifndef FVS_RGBA_TEXTURES_ONLY
    for (format=formatTable[flags&15]; format->fmt!=D3DFMT_UNKNOWN; format++)
    {
      if (checkTextureFormat(format->fmt, flags))
        break;
    }

		if (format->fmt==D3DFMT_UNKNOWN)
			fr::errorExit("No matching texture format found! (w=%d, h=%d, flags=%08x)", w, h, flags);
#endif
      
    if (flags & texfMipMaps) 
    {
      for (i = MIN(wn, hn), mipLevels = -1; i; i >>= 1, mipLevels++);
    }
    else
      mipLevels = 1;

    if (!(deviceCaps.TextureCaps & D3DPTEXTURECAPS_MIPMAP))
      mipLevels = 1;

#ifdef FVS_SUPPORT_RENDER_TO_TEXTURE
    usage = (flags & texfRTT)     ? D3DUSAGE_RENDERTARGET   : 0;
    pool  = (flags & texfRTT)     ? D3DPOOL_DEFAULT         : D3DPOOL_MANAGED;
#else
    usage = 0;
    pool  = D3DPOOL_MANAGED;
#endif

#ifdef FVS_SUPPORT_ANIM_TEXTURES
    if (flags & texfAnimated)
    {
      pool=D3DPOOL_SYSTEMMEM; // bei animierten texturen erst den mainbuffer allocen

      if (flags & texfNoLag)
        this->animFlags=1;
      else
        this->animFlags=2;

      this->animDisp=0;
      this->animChange=sFALSE;
    }
    else
      this->animFlags=0;
#endif
    
    // textur erzeugen
    
#ifndef FVS_RGBA_TEXTURES_ONLY
    checkError(d3ddev->CreateTexture(wn, hn, mipLevels, usage, format->fmt, pool, &tex), "CreateTexture FAILED");
#else
		checkError(d3ddev->CreateTexture(wn, hn, mipLevels, usage, D3DFMT_A8R8G8B8, pool, &tex), "CreateTexture FAILED");
#endif

#ifdef FVS_SUPPORT_ANIM_TEXTURES
    // bei animierten texturen ausserdem den update buffer allocen

    if (flags & texfAnimated)
    {
      IDirect3DTexture8 *a1, *a2;

      checkError(d3ddev->CreateTexture(wn, hn, mipLevels, usage, format->fmt, D3DPOOL_DEFAULT, &a1), "CreateTexture FAILED for anim buffer 1");
      this->addResIDs[0]=resourceMgr->add(a1);

      if (!(flags & texfNoLag)) // gebufferte texturen brauchen noch einen zusatzbuffer
      {
        checkError(d3ddev->CreateTexture(wn, hn, mipLevels, usage, format->fmt, D3DPOOL_DEFAULT, &a2), "CreateTexture FAILED for anim buffer 2");
        this->addResIDs[1]=resourceMgr->add(a2);
      }

      iAnimatedTexture *te=new iAnimatedTexture;
      te->tex=this;
      te->next=animTextures;
      animTextures=te;
    }
#endif

    this->w=w;
    this->h=h;
    this->rw=wn;
    this->rh=hn;

#ifndef FVS_TINY_TEXTURE_CODE
    if (flags & texfWrap)
    {
      this->usc=1.0f;
      this->vsc=1.0f;
    }
    else
    {
      this->usc=(sF32) w/(sF32) wn;
      this->vsc=(sF32) h/(sF32) hn;
    }
    this->uof=0.0f;
    this->vof=0.0f;
#endif

    this->createFlags=flags;
    this->resId=resourceMgr->add(tex);
    this->locked=sFALSE;
    this->created=sTRUE;
  }

  void texture::destroy()
  {
#ifdef FVS_SUPPORT_ANIM_TEXTURES
    if (animFlags && animTextures)
    {
      iAnimatedTexture *t1, *t2;

      if (animTextures->tex==this)
      {
        t1=animTextures->next;
        delete animTextures;
        animTextures=t1;
      }
      else
      {
        for (t1=animTextures; t1->next && t1->next->tex!=this; t1=t1->next);

        if (t1->next)
        {
          t2=t1->next->next;
          delete t1->next;
          t1->next=t2;
        }
      }
    }
#endif

    resourceMgr->remove(resId);
    created=sFALSE;
  }

  sBool texture::lock(sU32 level, D3DLOCKED_RECT *lr, const RECT *rect)
  {
    IDirect3DTexture8 *tex;

    FRDASSERT(created && !locked);

    tex=(IDirect3DTexture8 *) resourceMgr->getPtr(resId);
    locked=tex?(tex->LockRect(level, lr, rect, 0)==D3D_OK):sFALSE;

    return locked;
  }

  void texture::unlock(sU32 level)
  {
    IDirect3DTexture8 *tex;

    FRDASSERT(created && locked);

    tex=(IDirect3DTexture8 *) resourceMgr->getPtr(resId);
    tex->UnlockRect(level);
    locked=sFALSE;
  }

  sBool texture::getDesc(sU32 level, D3DSURFACE_DESC *dsc)
  {
    IDirect3DTexture8 *tex;
    
    FRDASSERT(created);
    
    tex=(IDirect3DTexture8 *) resourceMgr->getPtr(resId);
    return (tex->GetLevelDesc(level, dsc)==D3D_OK);
  }

  IDirect3DSurface8 *texture::getSurfaceLevel(sU32 level)
  {
    IDirect3DTexture8 *tex;
    IDirect3DSurface8 *srf;

    FRDASSERT(created);

    tex=(IDirect3DTexture8 *) resourceMgr->getPtr(resId);
    return (tex->GetSurfaceLevel(level, &srf)==D3D_OK)?srf:0;
  }

  void texture::upload(const sU16 *img)
  {
    D3DSURFACE_DESC desc;
    D3DLOCKED_RECT  rect;
    sU32            i, y;
    sU8             *src, *dst, *tmp;

    FRDASSERT(created && !locked);

    if (img)
    {
#ifndef FVS_RGBA_TEXTURES_ONLY
      // code not current
      tmp=new sU8[rw*rh*formatSize[createFlags&15]];
      
      if (w==rw && h==rh)
        memcpy(tmp, img, rw*rh*formatSize[createFlags&15]);
#else
      tmp=new sU8[rw*rh*4];
      for (i = 0; i < rw*rh*4; i++)
        tmp[i] = img[i] >> 7;
#endif
#ifndef FVS_TINY_TEXTURE_CODE
      else
      {
        if (createFlags & texfWrap || w>rw || h>rh)
#ifndef FVS_RGBA_TEXTURES_ONLY
          resampleImage(img, w, h, tmp, rw, rh, formatSize[createFlags&15]);
#else
	        resampleImage(img, w, h, tmp, rw, rh, 4);
#endif
        else
        {
#ifndef FVS_RGBA_TEXTURES_ONLY
          memset(tmp, 0, rw*rh*formatSize[createFlags&15]);
          
          sU32 lstep=rw*formatSize[createFlags&15], llen=w*formatSize[createFlags&15];
#else
          memset(tmp, 0, rw*rh*4);
          
          sU32 lstep=rw*4, llen=w*4;
#endif
          
          for (y=0; y<h; y++)
            memcpy(tmp+y*lstep, img+y*llen, llen);
        }
      }
#endif
    }
    else
      tmp=0;

    if (pool!=D3DPOOL_DEFAULT)
		{
      for (i=0; i<mipLevels; i++)
      {
        if (!lock(i, &rect))
          break;

        getDesc(i, &desc);
        
        if (tmp)
        {
          src=tmp;
          dst=(sU8 *) rect.pBits;
          
          for (y=0; y<desc.Height; y++)
          {
#ifndef FVS_RGBA_TEXTURES_ONLY
            format->func(dst, src, desc.Width, format->bits, format);
            src += formatSize[createFlags&15] * desc.Width;
#else
						memcpy(dst, src, 4 * desc.Width);
            src += 4 * desc.Width;
#endif
            dst += rect.Pitch;
          }

#ifndef FVS_RGBA_TEXTURES_ONLY
          generateMipMap(tmp, desc.Width, desc.Height, tmp, formatSize[createFlags&15]);
#else
          generateMipMap(tmp, desc.Width, desc.Height, tmp, 4);
#endif
        }
        else
          memset(rect.pBits, 0, rect.Pitch*desc.Height);

        unlock(i);
      }
		}
#ifndef FVS_TINY_ERROR_CHECKING
    else
      fr::errorExit("Cannot directly upload images to VRAM textures");
#endif

#ifdef FVS_SUPPORT_ANIM_TEXTURES
    // textur animiert?
    if (animFlags)
    {
      if (animFlags==1) // nolag, kopie im videoram updaten
        checkError(d3ddev->UpdateTexture((IDirect3DBaseTexture8 *) resourceMgr->getPtr(resId),
          (IDirect3DBaseTexture8 *) resourceMgr->getPtr(addResIDs[0])), "TEXX: UpdateTexture FAILED", sFALSE);
      else if (animFlags==2) // lagging, gerade nicht dargestellten buffer updaten
      {
        checkError(d3ddev->UpdateTexture((IDirect3DBaseTexture8 *) resourceMgr->getPtr(resId),
          (IDirect3DBaseTexture8 *) resourceMgr->getPtr(addResIDs[animDisp^1])), "TEXX: UpdateTexture FAILED", sFALSE);

        this->animChange=sTRUE;
      }
    }
#endif

    FRSAFEDELETEA(tmp);
  }

  void texture::translate(sF32 *u, sF32 *v) const
  {
#ifndef FVS_TINY_TEXTURE_CODE
    if (this)
    {
      FRDASSERT(created);

      if (u)  *u=*u*usc+uof;
      if (v)  *v=*v*vsc+vof;
    }
#endif
  }

  void texture::translatePix(sF32 *u, sF32 *v) const
  {
#ifndef FVS_TINY_TEXTURE_CODE
    if (this)
    {
      FRDASSERT(created);

      if (u)  *u=*u*usc/(sF32) w+uof;
      if (v)  *v=*v*vsc/(sF32) h+vof;
    }
#endif
  }

  // ---- cubeTexture

#ifdef FVS_SUPPORT_CUBEMAPS
  cubeTexture::cubeTexture()
  {
    created=sFALSE;
  }

  cubeTexture::cubeTexture(sU32 edge, sU32 flags)
  {
    created=sFALSE;
    create(edge, flags);
  }

  cubeTexture::~cubeTexture()
  {
    if (created)
      destroy();
  }

  void *cubeTexture::getResID() const
  {
    return resId;
  }

  cubeTexture::operator IDirect3DCubeTexture8 *() const
  {
    if (this)
    {
      FRDASSERT(created);
      
      return (IDirect3DCubeTexture8 *) resourceMgr->getPtr(getResID());
    }
    else
      return 0;
  }

  void cubeTexture::create(sU32 edge, sU32 flags)
  {
    IDirect3DCubeTexture8 *tex;
    sU32                  usage;
    sU32                  en, i;

#ifdef FVS_SUPPORT_ANIM_TEXTURES
    FRASSERT(!(flags & texfAnimated));
#endif

    if (flags & texfRTT) 
      flags&=~texfMipMaps;

    if (!(output.caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP))
      fr::errorExit("Cubemaps are not supported on your graphics adapter, but we need them");

#ifndef FVS_TINY_TEXTURE_CODE
    if (output.caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP_POW2)
      for (en=1; en<edge; en<<=1);
    else
      en=edge;

    en=MIN(en, output.caps.MaxTextureWidth);
    if (en!=edge)
      fr::errorExit("Hardware does not directly support requested cubemap edge size of %d", edge);
#endif

    // passendes texturformat suchen

#ifndef FVS_RGBA_TEXTURES_ONLY
    for (format=formatTable[flags&15]; format->fmt!=D3DFMT_UNKNOWN; format++)
    {
      if (checkCubemapFormat(format->fmt, flags))
        break;
    }
#else
    for (format=formatsRGBA; format->fmt!=D3DFMT_UNKNOWN; format++)
    {
      if (checkCubemapFormat(format->fmt, flags))
        break;
    }
#endif
      
    if (format->fmt==D3DFMT_UNKNOWN)
      fr::errorExit("No matching cubemap format found! (edge=%d, flags=%s)", edge, formatFlags(flags));

    if (flags & texfMipMaps) 
    {
      for (i=edge, mipLevels=0; i; i>>=1, mipLevels++);
      mipLevels--;
    }
    else
      mipLevels=1;
    
    if (!(output.caps.TextureCaps & D3DPTEXTURECAPS_MIPCUBEMAP))
      mipLevels=1;
    
    usage = (flags & texfRTT)     ? D3DUSAGE_RENDERTARGET   : 0;
    pool  = (flags & texfRTT)     ? D3DPOOL_DEFAULT         : D3DPOOL_MANAGED;

    // cubemap erzeugen
 
    checkError(d3ddev->CreateCubeTexture(edge, mipLevels, usage, format->fmt, pool, &tex),
      "CreateCubeTexture FAILED");

    this->edge=edge;
    this->createFlags=flags;
    this->resId=resourceMgr->add(tex);
    this->locked=sFALSE;
    this->created=sTRUE;
  }
  
  void cubeTexture::destroy()
  {
    resourceMgr->remove(resId);
    created=sFALSE;
  }
  
  sBool cubeTexture::lock(sU32 face, sU32 level, D3DLOCKED_RECT *lr, const RECT *rect)
  {
    IDirect3DCubeTexture8 *tex;
    
    FRDASSERT(created && !locked);
    FRDASSERT(face>=0 && face<=5);
    
    tex=(IDirect3DCubeTexture8 *) resourceMgr->getPtr(resId);
    locked=tex?(tex->LockRect((D3DCUBEMAP_FACES) face, level, lr, rect, 0)==D3D_OK):sFALSE;
    
    return locked;
  }
  
  void cubeTexture::unlock(sU32 face, sU32 level)
  {
    IDirect3DCubeTexture8 *tex;
    
    FRDASSERT(created && locked);
    
    tex=(IDirect3DCubeTexture8 *) resourceMgr->getPtr(resId);
    tex->UnlockRect((D3DCUBEMAP_FACES) face, level);
    locked=sFALSE;
  }
  
  sBool cubeTexture::getDesc(sU32 level, D3DSURFACE_DESC *dsc)
  {
    IDirect3DCubeTexture8 *tex;
    
    FRDASSERT(created);
    
    tex=(IDirect3DCubeTexture8 *) resourceMgr->getPtr(resId);
    return (tex->GetLevelDesc(level, dsc)==D3D_OK);
  }
  
  IDirect3DSurface8 *cubeTexture::getSurfaceLevel(sU32 face, sU32 level)
  {
    IDirect3DCubeTexture8 *tex;
    IDirect3DSurface8 *srf;
    
    FRDASSERT(created);
    FRDASSERT(face>=0 && face<=5);
    
    tex=(IDirect3DCubeTexture8 *) resourceMgr->getPtr(resId);
    return (tex->GetCubeMapSurface((D3DCUBEMAP_FACES) face, level, &srf)==D3D_OK)?srf:0;
  }
  
  void cubeTexture::upload(sU32 face, const sU8 *img)
  {
    D3DSURFACE_DESC desc;
    D3DLOCKED_RECT  rect;
    sU32            i, y;
    sU8             *src, *dst, *tmp;

    FRDASSERT(created && !locked);
    FRDASSERT(face>=0 && face<=5);

    if (img)
    {
      tmp=new sU8[edge*edge*formatSize[createFlags&15]];
      memcpy(tmp, img, edge*edge*formatSize[createFlags&15]);
    }
    else
      tmp=0;

    if (pool!=D3DPOOL_DEFAULT)
    {
      for (i=0; i<mipLevels; i++)
      {
        if (!lock(face, i, &rect))
          break;

        getDesc(i, &desc);
        
        if (tmp)
        {
          src=tmp;
          dst=(sU8 *) rect.pBits;
          
          asl=format->asl; asr=format->asr;
          rsl=format->rsl; rsr=format->rsr;
          gsl=format->gsl; gsr=format->gsr;
          bsl=format->bsl; bsr=format->bsr;
          
          for (y=0; y<desc.Height; y++)
          {
            format->func(dst, src, desc.Width, format->bits);
            
            src+=formatSize[createFlags&15]*desc.Width;
            dst+=rect.Pitch;
          }
          
          generateMipMap(tmp, desc.Width, desc.Height, tmp, formatSize[createFlags&15]);
        }
        else
          memset(rect.pBits, 0, rect.Pitch*desc.Height);
        
        unlock(face, i);
      }
    }
    else
      fr::errorExit("Cannot directly upload images to VRAM textures");
      
    FRSAFEDELETEA(tmp);
  }
#endif
  
  // ---- fvs interface
  
#ifdef FVS_SUPPORT_ANIM_TEXTURES
  void updateAnimatedTexturesPerFrame()
  {
    for (iAnimatedTexture *t=animTextures; t; t=t->next)
    {
      texture *tex=t->tex;
      
      if (tex->animFlags==2 && tex->animChange)
      {
        tex->animDisp^=1;
        tex->animChange=sFALSE;
      }
    }
  }
#endif

  // ---------------------------------------------------------------------------------------------
  // highlevel vertex/index buffer handling
  // ---------------------------------------------------------------------------------------------

  namespace
  {
    static const sInt  staticVBSize=16000;
    static const sInt  staticIBSize=16000;
    static const sInt  dynamicVBSize=49152;
    static const sInt  dynamicIBSize=65535*3;

    class iRangeManager
    {
    public:
      sU32      size, pos;
      
      iRangeManager(sU32 sz)
        : size(sz), pos(0)
      {
      }

      void add(sU32 start, sU32 len)
      {
        if (start + len > pos)
          pos = start + len;
      }

      void remove(sU32 start, sU32 len)
      {
        if (pos == start + len)
          pos = start;
      }

      sInt findSpace(sU32 len)
      {
				return (pos + len < size) ? pos : -1;
      }

      void clear()
      {
        pos = 0;
      }
    };
  }

  struct bufSlice::iD3DBuffer
  {
		void*					buf;
    iRangeManager usage;
    sU32          FVF;
    sInt          streamInd;
    sInt          pos, size;
    sU32          stride;
    sBool         isDynamic;
		iD3DBuffer*		next;

    iD3DBuffer(void* buffer, sU32 VF, sInt streamNdx, sU32 sz, sU32 strd, sBool dynamic)
      : buf(buffer), usage(isDynamic ? 0 : sz), FVF(VF), streamInd(streamNdx), size(sz), pos(0), next(0), stride(strd), isDynamic(dynamic)
    {
    }

    ~iD3DBuffer()
    {
      resourceMgr->remove(buf);
      FRSAFEDELETE(next);
    }
  };

  bufSlice::iD3DBuffer* bufSlice::d3dBuffers = 0;

  // ---- bufSlice

  bufSlice::bufSlice()
  {
    created = sFALSE;
    locked = sFALSE;
    ibuf = 0;
  }

  bufSlice::~bufSlice()
  {
    destroy();
  }

  void bufSlice::globalCleanup()
  {
    FRSAFEDELETE(d3dBuffers);
  }

  sU32 bufSlice::performCreate(sInt nElems, sBool isDynamic, sU32 VF, sInt streamInd, sU32 stride, sInt refSize, sU32 vto)
  {
    iD3DBuffer  *it;
    sInt        pos;

    FRDASSERT(!created);

    created = sTRUE;
    dynamic = isDynamic;
    FVF = VF;
    nElements = nElems;
    streamIndex = streamInd;

    if (nElems <= refSize)
    {
      for (it = d3dBuffers; it; it = it->next)
      {
        if (it->FVF == VF && it->streamInd == streamInd && it->isDynamic == isDynamic)
        {
          if (((pos = it->usage.findSpace(nElems)) != -1) || isDynamic)
          {
            elemBase = pos;
            ibuf = it;

            if (!isDynamic)
              it->usage.add(pos, nElems);

            return 0;
          }
        }
      }
    }

    sU32 sz = (nElems > refSize) ? nElems : refSize;
    IUnknown *iface;
    HRESULT hr;

		// ugly but kinda effective hack follows :)

    __asm
    {
      lea     eax, [iface];
      push    eax;            // target iface ptr

      mov     eax, [isDynamic];
      test    eax, eax;
      jz      notDynamic;

      mov     eax, D3DPOOL_DEFAULT;
      mov     ebx, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
      jmp     short madeDecision;

notDynamic:
      mov     eax, D3DPOOL_MANAGED;
      mov     ebx, D3DUSAGE_WRITEONLY;

madeDecision:
      push    eax;            // resource pool number
      push    [VF];           // FVF/index format
      push    ebx;            // usage flags

      mov     eax, [sz];
      imul    eax, [stride];
      push    eax;            // size of buffer

      mov     ecx, [d3ddev];
      push    ecx;            // this ptr
      mov     eax, [ecx];     // eax=vtbl offset
      add     eax, [vto];     // add vtable delta
      call    [eax];          // call create function

      mov     [hr], eax;      // and save result.
    }

    checkError(hr, "Buffer create failed");
    ibuf = new iD3DBuffer(resourceMgr->add(iface), VF, streamInd, sz, stride, isDynamic);
    elemBase = 0;

    if (!isDynamic)
      ibuf->usage.add(0, nElems);

    ibuf->next = d3dBuffers;
    d3dBuffers = ibuf;
  }

  void bufSlice::destroy()
  {
    FRDASSERT(created && !locked);

    if (!dynamic)
      ibuf->usage.remove(elemBase, nElements);

    created = sFALSE;
  }

  void* bufSlice::lock(sInt lockStart, sInt lockSize)
  {
    sU32 lockMode;

    if (lockSize == -1)
      lockSize = nElements;

    FRDASSERT(created && !locked);
    FRDASSERT(!dynamic || lockStart == 0);

    if (dynamic)
    {
      if (ibuf->pos + lockSize < ibuf->size)
        elemBase = ibuf->pos;
      else
        elemBase = 0;

      lockMode = (elemBase != 0) ? D3DLOCK_NOOVERWRITE : D3DLOCK_DISCARD;
      ibuf->pos = elemBase + lockSize;
    }
    else
      lockMode = 0;

    locked = sTRUE;

		// this would've been nicer if vertex/index buffers actually had a common base that defines lock/unlock.
		// but who cares.
    IDirect3DVertexBuffer8 *vb = (IDirect3DVertexBuffer8 *) resourceMgr->getPtr(ibuf->buf);
    sU8 *buf;
    checkError(vb->Lock((elemBase + lockStart) * ibuf->stride, lockSize * ibuf->stride, &buf, lockMode), "Buffer Lock failed", sFALSE);

    return (void *) buf;
  }

  void bufSlice::unlock()
  {
    FRDASSERT(created && locked);

    // refer to the comment in lock()
    IDirect3DVertexBuffer8 *vb = (IDirect3DVertexBuffer8 *) resourceMgr->getPtr(ibuf->buf);
    vb->Unlock();

    locked = sFALSE;
  }

  void bufSlice::fillFrom(const void *dataPtr, sInt count)
  {
    if (count == -1)
      count = nElements;

    memcpy(lock(0, count), dataPtr, count * ibuf->stride);
    unlock();
  }

  // ---- vbSlice

  void vbSlice::create(sInt nVerts, sBool isDynamic, sU32 VF, sInt streamInd)
  {
#ifndef FVS_ONLY_32B_VERTEX_FORMATS
    const sU32 stride = getFVFSize(VF);
#else
    const sU32 stride = 32;
#endif
    performCreate(nVerts, isDynamic, VF, streamInd, stride, isDynamic ? dynamicVBSize : staticVBSize, 92);
  }

  sU32 vbSlice::use() const
  {
    FRDASSERT(created && !locked);

    setVertexShader(FVF);
    setVertexBuffer(ibuf->streamInd, (IDirect3DVertexBuffer8 *) resourceMgr->getPtr(ibuf->buf), ibuf->stride);

    return elemBase;
  }

  // ---- ibSlice

  void ibSlice::create(sInt nInds, sBool isDynamic)
  {
    performCreate(nInds, isDynamic, D3DFMT_INDEX16, 0, 2, isDynamic ? dynamicIBSize : staticIBSize, 96);
  }

  sU32 ibSlice::use(sU32 startv) const
  {
    FRDASSERT(created && !locked);

    setIndexBuffer((IDirect3DIndexBuffer8 *) resourceMgr->getPtr(ibuf->buf), startv);
    
    return elemBase;
  }
}

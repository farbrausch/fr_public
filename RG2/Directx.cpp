// This code is in the public domain. See LICENSE for details.

#pragma warning (disable: 4786)

#include "types.h"
#include "pool.h"
#include "debug.h"
#include "tool.h"
#include "directx.h"

#define  WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>

#pragma comment(lib, "d3d9.lib")

#ifdef FVS_MINIMAL_DEBUG_OUTPUT
#pragma warning (disable: 4390)
#endif

namespace fvs
{
  struct            iZBufList;
  
  viewportStruct    viewport;
  config            conf;
  sU32              numDevices;
  deviceInfo        *devices;
  deviceInfo        device;
  outputInfo        output;
  IDirect3D9        *d3d;
  IDirect3DDevice9  *d3ddev;
  
#ifdef FVS_SUPPORT_TEXTURE_POOL
  texturePoolT      *texturePool;
#endif
  
  namespace
  {
    struct iRenderTarget
    {
      IDirect3DSurface9     *tgt;
      void                  *zbuf;
      texture               *texture;
      sU32                  id;
      sU32                  w, h;
      sU32                  rw, rh;
      sBool                 hasZB;
      IDirect3DTexture9     *sysmemcopy;
      IDirect3DSurface9     *smcsurface;
      iRenderTarget         *next;

			iRenderTarget() : tgt(0),zbuf(0),texture(0),sysmemcopy(0),smcsurface(0),next(0) {}
    };

    static IDirect3DSurface9      *back, *zbuffer;
    static HWND                   hDevWindow, hFocusWindow;
    static iRenderTarget          *targets=0, *curtarget;
    static sBool					        inscene=sFALSE;
    static iZBufList              *zBuffers=0;
    static D3DFORMAT              zbufFormat;
    static D3DDEVTYPE             devType;
    static D3DPRESENT_PARAMETERS  pparm;
    
    static sBool                  clearStencil=sFALSE;
    static IDirect3DVertexBuffer9 *oldVertexBuffer[8], *newVertexBuffer[8];
    static sU32                   oldVBStride[8], newVBStride[8];
    static IDirect3DIndexBuffer9  *oldIndexBuffer, *newIndexBuffer;
    static sU32                   oldIBStart, newIBStart;
    static sU32                   oldVShader, newVShader;
    
#ifdef FVS_SUPPORT_VERTEX_SHADERS
    static sU32                   oldVSConstants[96*4+4*4], newVSConstants[96*4+4*4];
    static sU32                   vsModStart, vsModEnd;
    static sBool                  noVertexShaders=sTRUE;
#endif

    // ---- resource management

    class FR_NOVTABLE iResourceManager
    {
#ifdef FVS_SUPPORT_RESOURCE_MANAGEMENT
    private:
      class FR_NOVTABLE resource
      {
      public:
        IDirect3DResource9  *res;
        IDirect3DSurface9   *srf;
        D3DRESOURCETYPE     type;
        D3DPOOL             pool;
        sU8                 *savedData, *createData;
        sU32                savedSize, createSize;
        void                *id;
        
        resource            *prev, *next;
        
        resource(IDirect3DResource9 *_res)
        {
          res=_res;
          srf=0;
          savedData=createData=0;
          savedSize=createSize=0;
          id=this;
          
          type=res->GetType();
          switch (type)
          {
          case D3DRTYPE_TEXTURE:
          case D3DRTYPE_CUBETEXTURE:
            {
              D3DSURFACE_DESC desc;
              
              if (type==D3DRTYPE_TEXTURE)
                ((IDirect3DTexture9 *) res)->GetLevelDesc(0, &desc);
              else
                ((IDirect3DCubeTexture9 *) res)->GetLevelDesc(0, &desc);
              
              pool=desc.Pool;
            }
            break;
            
          case D3DRTYPE_VERTEXBUFFER:
            {
              D3DVERTEXBUFFER_DESC dsc;

              ((IDirect3DVertexBuffer9 *) res)->GetDesc(&dsc);
              
              pool=dsc.Pool;
            }
            break;  
              
          case D3DRTYPE_INDEXBUFFER:
            {
              D3DINDEXBUFFER_DESC dsc;
              
              ((IDirect3DIndexBuffer9 *) res)->GetDesc(&dsc);
              
              pool=dsc.Pool;
            }
            break;

          default:
            pool=D3DPOOL_MANAGED;
          }

          next=prev=0;
        }

        resource(IDirect3DSurface9 *_srf)
        {
          res=0;
          srf=_srf;

          savedData=createData=0;
          savedSize=createSize=0;
          id=this;
          
          type=D3DRTYPE_SURFACE;
          pool=D3DPOOL_DEFAULT;
          
          next=prev=0;
        }

        ~resource()
        {
          FRSAFEDELETEA(savedData);
          FRSAFEDELETEA(createData);

          if (res)
            res->Release();

          if (srf)
            srf->Release();
        }
        
        void save()
        {
          if (pool!=D3DPOOL_DEFAULT || savedData || createData)
            return;

          switch (type)
          {
          case D3DRTYPE_SURFACE:
            {
              D3DSURFACE_DESC         dsc;
        
              srf->GetDesc(&dsc);
              createSize=sizeof(dsc);
              createData=new sU8[createSize];
        
              memcpy(createData, &dsc, sizeof(dsc));
        
              break;
            }

          case D3DRTYPE_TEXTURE:
            {
              IDirect3DTexture9       *tex=(IDirect3DTexture9 *) res;
              D3DSURFACE_DESC         dsc;

              tex->GetLevelDesc(0, &dsc);
              createSize=sizeof(dsc)+4;
              createData=new sU8[createSize];

              *((sU32 *) createData)=tex->GetLevelCount();
              memcpy(createData+4, &dsc, sizeof(dsc));

              break;
            }

          case D3DRTYPE_CUBETEXTURE:
            {
              IDirect3DCubeTexture9   *tex=(IDirect3DCubeTexture9 *) res;
              D3DSURFACE_DESC         dsc;

              tex->GetLevelDesc(0, &dsc);
              createSize=sizeof(dsc)+4;
              createData=new sU8[createSize];

              *((sU32 *) createData)=tex->GetLevelCount();
              memcpy(createData+4, &dsc, sizeof(dsc));

              break;
            }

          case D3DRTYPE_VERTEXBUFFER:
            {
              IDirect3DVertexBuffer9  *vb=(IDirect3DVertexBuffer9 *) res;
              D3DVERTEXBUFFER_DESC    dsc;
              void                    *locked;

              vb->GetDesc(&dsc);
              createSize=sizeof(dsc);
              createData=new sU8[createSize];
              memcpy(createData, &dsc, createSize);

              if (dsc.Usage & D3DUSAGE_WRITEONLY)
                break;

              if (vb->Lock(0, dsc.Size, &locked, D3DLOCK_READONLY)==D3D_OK)
              {
                savedSize=dsc.Size;
                savedData=new sU8[savedSize];
                memcpy(savedData, locked, savedSize);

                vb->Unlock();
              }

              break;
            }

          case D3DRTYPE_INDEXBUFFER:
            {
              IDirect3DIndexBuffer9   *ib=(IDirect3DIndexBuffer9 *) res;
              D3DINDEXBUFFER_DESC     dsc;
              void                    *locked;

              ib->GetDesc(&dsc);
              createSize=sizeof(dsc);
              createData=new sU8[createSize];
              memcpy(createData, &dsc, createSize);
        
              if (dsc.Usage & D3DUSAGE_WRITEONLY)
                break;

              if (ib->Lock(0, dsc.Size, &locked, D3DLOCK_READONLY)==D3D_OK)
              {
                savedSize=dsc.Size;
                savedData=new sU8[savedSize];
                memcpy(savedData, locked, savedSize);
          
                ib->Unlock();
              }
        
              break;
            }
          }
        }

        void kill()
        {
          if (pool!=D3DPOOL_DEFAULT)
            return;

          if (res)
          {
            res->Release();
            res=0;
          }
          
          if (srf)
          {
            srf->Release();
            srf=0;
          }
        }

        void recreate()
        {
          if (pool!=D3DPOOL_DEFAULT || (!savedData && !createData))
            return;

          switch (type)
          {
          case D3DRTYPE_SURFACE:
            {
              D3DSURFACE_DESC   *dsc;
        
              dsc=(D3DSURFACE_DESC *) createData;

              if (dsc->Usage & D3DUSAGE_DEPTHSTENCIL)
              {
                if (d3ddev->CreateDepthStencilSurface(dsc->Width, dsc->Height, dsc->Format,
                  dsc->MultiSampleType, dsc->MultiSampleQuality, TRUE, &srf, 0)!=D3D_OK)
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
                  fr::debugOut("RESM: surface #%08x recreation failed (non-depth-stencil)\n", id);
#else
                  ;
#endif
              }
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
              else
                fr::debugOut("RESM: surface #%08x recreation failed (non-depth-stencil)\n", id);
#endif

              break;
            }

          case D3DRTYPE_TEXTURE:
            {
              D3DSURFACE_DESC   *dsc;
              sU32              miplevels;

              miplevels=*((sU32 *) createData);
              dsc=(D3DSURFACE_DESC *) (createData+4);

              if (d3ddev->CreateTexture(dsc->Width, dsc->Height, miplevels, dsc->Usage, dsc->Format,
                dsc->Pool, (IDirect3DTexture9 **) &res, 0)!=D3D_OK)
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
                fr::debugOut("RESM: texture #%08x recreation failed\n", id);
#else
                ;
#endif

              break;
            }

          case D3DRTYPE_CUBETEXTURE:
            {
              D3DSURFACE_DESC   *dsc;
              sU32              miplevels;

              miplevels=*((sU32 *) createData);
              dsc=(D3DSURFACE_DESC *) (createData+4);

              if (d3ddev->CreateCubeTexture(dsc->Width, miplevels, dsc->Usage, dsc->Format, dsc->Pool,
                (IDirect3DCubeTexture9 **) &res, 0)!=D3D_OK)
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
                fr::debugOut("RESM: cubemap #%08x recreation failed\n", id);
#else
                ;
#endif

              break;
            }

          case D3DRTYPE_VERTEXBUFFER:
            {
              D3DVERTEXBUFFER_DESC    *dsc;
              void                    *locked;

              dsc=(D3DVERTEXBUFFER_DESC *) createData;

              if (d3ddev->CreateVertexBuffer(dsc->Size, dsc->Usage, dsc->FVF, dsc->Pool,
                (IDirect3DVertexBuffer9 **) &res, 0)!=D3D_OK)
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
                fr::debugOut("RESM: vertex buffer #%08x recreation failed\n", id);
#else
                ;
#endif

              if (!savedData)
                break;

              IDirect3DVertexBuffer9  *vb=(IDirect3DVertexBuffer9 *) res;
        
              if (vb->Lock(0, savedSize, &locked, 0)==D3D_OK)
              {
                memcpy(locked, savedData, savedSize);
                vb->Unlock();
              }
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
              else
                fr::debugOut("RESM: vertex buffer #%08x recreation lock failed\n", id);
#endif
        
              break;
            }

          case D3DRTYPE_INDEXBUFFER:
            {
              D3DINDEXBUFFER_DESC     *dsc;
              void                    *locked;

              dsc=(D3DINDEXBUFFER_DESC *) createData;
              if (d3ddev->CreateIndexBuffer(dsc->Size, dsc->Usage, dsc->Format, dsc->Pool,
                (IDirect3DIndexBuffer9 **) &res, 0)!=D3D_OK)
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
                fr::debugOut("RESM: index buffer #%08x recreation failed\n", id);
#else
                ;
#endif

              if (!savedData)
                break;
        
              IDirect3DIndexBuffer9   *ib=(IDirect3DIndexBuffer9 *) res;
        
              if (ib->Lock(0, savedSize, &locked, 0)==D3D_OK)
              {
                memcpy(locked, savedData, savedSize);
                ib->Unlock();
              }
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
              else
                fr::debugOut("RESM: index buffer #%08x recreation lock failed\n", id);
#endif
              
              break;
            }
          }
          
          FRSAFEDELETEA(savedData);
          FRSAFEDELETEA(createData);
          savedSize=0;
          createSize=0;
        }
      };
      
      resource  *resources;
      sInt      saveState;
      
    public:
      iResourceManager()
        : resources(0), saveState(0)
      {
      }
      
      ~iResourceManager()
      {
        resource  *t, *t2;
        
        for (t=resources; t; t=t2)
        {
          t2=t->next;
          delete t;
        }
      }
      
      void *add(IDirect3DResource9 *res)
      {
        resource *rsrc=new resource(res);
        
        rsrc->next=resources;
        if (resources)
          resources->prev=rsrc;
        
        resources=rsrc;
        return rsrc->id;
      }

      void *add(IDirect3DSurface9 *res)
      {
        resource *rsrc=new resource(res);
        
        rsrc->next=resources;
        if (resources)
          resources->prev=rsrc;
        
        resources=rsrc;
        return rsrc->id;
      }
      
      void remove(void *id)
      {
        if (!id)
          return;
        
        for (resource *t=resources; t; t=t->next)
        {
          if (t->id==id)
          {
            if (t->prev)
              t->prev->next=t->next;
            else
              resources=t->next;
            
            if (t->next)
              t->next->prev=t->prev;
            
            delete t;
            return;
          }
        }
        
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
        fr::debugOut("RESM: warning: no resource with id %08x found\n", id);
#endif
      }
      
      inline IUnknown *getPtr(const void *id) const
      {
        const resource  *rs=(const resource *) id;
        
        if (!rs || rs->id!=id)
          return 0;
        
        if (rs->type!=D3DRTYPE_SURFACE)
          return rs->res;
        else
          return rs->srf;
      }
      
      void save()
      {
        if (saveState!=0)
        {
          fr::debugOut("RESM: not saving; state wrong\n");
          return;
        }
        
        for (resource *t=resources; t; t=t->next)
          t->save();
        
        saveState=1;
      }

      void kill()
      {
        if (saveState!=1)
        {
          fr::debugOut("RESM: not killing, state wrong; trying save first\n");
          save();

          if (saveState!=1)
            return;
        }
        
        for (resource *t=resources; t; t=t->next)
          t->kill();
        
        saveState=2;
      }

      void recreate()
      {
        if (saveState!=2)
        {
          fr::debugOut("RESM: not recreating; state wrong\n");
          return;
        }
        
        for (resource *t=resources; t; t=t->next)
          t->recreate();
        
        saveState=0;
      }
#else
    struct resList
    {
      IDirect3DResource9  *res;
      IDirect3DSurface9   *srf;
      void                *id;
      resList             *next;
    };

    resList *rsrc;

    public:
      iResourceManager()
      {
        rsrc=0;
      }

      ~iResourceManager()
      {
        while (rsrc)
          remove(rsrc->id);
      }

      inline void *add(IDirect3DResource9 *res)
      {
        resList *t=new resList;
        t->res=res;
        t->srf=0;
        t->id=res;
        t->next=rsrc;
        rsrc=t;

        return res;
      }

      inline void *add(IDirect3DSurface9 *res)
      {
        resList *t=new resList;
        t->res=0;
        t->srf=res;
        t->id=res;
        t->next=rsrc;
        rsrc=t;

        return res;
      }

      inline void remove(void *id)
      {
        if (!rsrc)
          return;

        if (rsrc->id==id)
        {
          if (rsrc->res)
            rsrc->res->Release();

          if (rsrc->srf)
            rsrc->srf->Release();

          resList *t=rsrc->next;
          delete rsrc;
          rsrc=t;
          return;
        }

        resList *t=rsrc;

        while (t && t->next)
        {
          if (t->next->id==id)
          {
            resList *t2=t->next;

            if (t2->res)
              t2->res->Release();

            if (t2->srf)
              t2->srf->Release();

            t->next=t2->next;
            delete t2;
          }

          t=t->next;
        }
      }

      inline IUnknown *getPtr(const void *id) const
      {
        return (IUnknown *) id;
      }

      inline void save()                        { }
      inline void kill()                        { }
      inline void recreate()                    { }
#endif
    };

    iResourceManager *resourceMgr=0;
  }

  // ---- support stuff

  static LRESULT CALLBACK DeviceWindowProc(HWND hw, UINT Msg, WPARAM wParam, LPARAM lParam)
  {
    switch (Msg)
    {
    case WM_SETFOCUS:
    	SetFocus(hFocusWindow);
      return 0;
        
    case WM_SETCURSOR:
    	SetCursor(0);
      if (d3ddev)
        d3ddev->ShowCursor(FALSE);
      return TRUE;
        
    default:
      return DefWindowProc(hw, Msg, wParam, lParam);
    }
    
    return 0;
  }
  
  static iRenderTarget *addRenderTarget(IDirect3DSurface9 *tgt)
  {
    iRenderTarget *t=new iRenderTarget;
    static sU32 IDCounter=0;
    
    t->tgt=tgt;
    t->texture=0;
    t->id=IDCounter++;
    t->next=targets;
    
    targets=t;
    return t;
  }
  
  static sBool checkDepthFormat(D3DFORMAT depthFormat)
  {
    if (d3d->CheckDeviceFormat(conf.device, devType, conf.fmt, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, depthFormat)!=D3D_OK)
      return sFALSE;
    
    if (d3d->CheckDepthStencilMatch(conf.device, devType, conf.fmt, conf.fmt, depthFormat)!=D3D_OK)
      return sFALSE;
    
    return sTRUE;
  }

  static HRESULT checkError(HRESULT hr, const sChar *str, sBool fatal=sTRUE)
  {
#ifdef FVS_TINY_ERROR_CHECKING
    if (hr!=D3D_OK && fatal)
      fr::errorExit("dx error");

    return hr;
#else
    if (hr==D3D_OK)
      return hr;
    else if (hr==E_OUTOFMEMORY || hr==D3DERR_OUTOFVIDEOMEMORY)
    {
      sChar buf[4096];
      sBool tip1, tip2;
      
      fr::debugOut("TOOL: dx error: %s (#0x%08x)\n", str, hr);
      
      tip1=(conf.xRes>conf.minXRes || conf.yRes>conf.minYRes);
      tip2=(conf.depth>16);
      
      strcpy(buf, "Sorry, your graphics card has run out of memory");
      if (tip1 || tip2)
      {
        strcat(buf, ". You could try the following things:\n");
        
        if (tip1)
          strcat(buf, "- decrease the screen resolution\n");
        
        if (tip2)
          strcat(buf, "- use less bits per pixel (eg. 16)\n");
        
        strcat(buf, "This should help running this demo. Have thanks.\n");
      }
      else
      strcat(buf, ", and there's not much that can be done about it. "
				"Maybe changing your card configuration will help something, maybe not.\n");
      
      fr::errorExit(buf); // outofmemory ist *immer* fatal
    }
    else
    {
      fr::debugOut("DCTX: dx error: %s (#0x%08x)\n", str, hr);
      
      if (fatal)
        fr::errorExit("%s (#0x%08x)\n", str, hr);
    }
    
    return hr;
#endif
  }

  static sU32 getFVFSize(sU32 FVF)
  {
    sU32 size=0, texcount;

    texcount=(FVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
  
    switch (FVF & D3DFVF_POSITION_MASK)
    {
    case D3DFVF_XYZ:    size+=12; break;
    case D3DFVF_XYZRHW: size+=16; break;
    case D3DFVF_XYZB1:  size+=16; break;
    case D3DFVF_XYZB2:  size+=20; break;
    case D3DFVF_XYZB3:  size+=24; break;
    case D3DFVF_XYZB4:  size+=28; break;
    case D3DFVF_XYZB5:  size+=32; break;
    }
  
    size+=((FVF & D3DFVF_DIFFUSE)  == D3DFVF_DIFFUSE)  ?  4 : 0;
    size+=((FVF & D3DFVF_NORMAL)   == D3DFVF_NORMAL)   ? 12 : 0;
    size+=((FVF & D3DFVF_PSIZE)    == D3DFVF_PSIZE)    ?  4 : 0;
    size+=((FVF & D3DFVF_SPECULAR) == D3DFVF_SPECULAR) ?  4 : 0;
  
    FVF>>=16;
    for (sU32 i=0; i<texcount; i++)
    {
      switch (FVF & 3)
      {
      case D3DFVF_TEXTUREFORMAT1: size+=4;  break;
      case D3DFVF_TEXTUREFORMAT2: size+=8;  break;
      case D3DFVF_TEXTUREFORMAT3: size+=12; break;
      case D3DFVF_TEXTUREFORMAT4: size+=16; break;
      }
    
      FVF>>=2;
    }
  
    return size;
  }
  
  // ---- idle handling

  struct iIdleHandler
  {
    idleHandler   hnd;
    iIdleHandler  *next;
  } *idleHandlers=0;

  static void idle()
  {
    for (iIdleHandler *t=idleHandlers; t; t=t->next)
      t->hnd();
  }

  void addIdleHandler(idleHandler hnd)
  {
    iIdleHandler *t=new iIdleHandler;

    t->hnd=hnd;
    t->next=idleHandlers;
    idleHandlers=t;
  }

  static void killIdleHandlers()
  {
    iIdleHandler *t=idleHandlers, *t2;

    while (t)
    {
      t2=t->next;
      delete t;
      t=t2;
    }
  }
  
  // ---- zbuffer sharing

  struct iZBufList
  {
    sU32  w, h;
    void  *resid;
    sInt  refCount;

    iZBufList *next;
  };

  static void *getZBuffer(sU32 w, sU32 h)
  {
    iZBufList *t=zBuffers;

    while (t)
    {
      if (t->w==w && t->h==h)
      {
        t->refCount++;
        return t->resid;
      }

      t=t->next;
    }

    IDirect3DSurface9 *srf;
    checkError(d3ddev->CreateDepthStencilSurface(w, h, zbufFormat, D3DMULTISAMPLE_NONE, 
      0, TRUE, &srf, 0), "DCTX: couldn't create zbuffer surface for RTT targets");
    
    t=new iZBufList;
    t->w=w;
    t->h=h;
    t->resid=resourceMgr->add(srf);
    t->refCount=1;
    t->next=zBuffers;
    zBuffers=t;

    return t->resid;
  }

  static void releaseZBuffer(void *ptr)
  {
    iZBufList *t=zBuffers, *t2;

    if (t->resid==ptr)
    {
      if (--t->refCount==0)
      {
        resourceMgr->remove(ptr);
        t2=t->next;
        delete t;
        zBuffers=t2;
      }

      return;
    }

    while (t->next)
    {
      if (t->next->resid==ptr)
      {
        if (--t->next->refCount==0)
        {
          resourceMgr->remove(ptr);
          t2=t->next;
          t->next=t2->next;
          delete t2;
        }

        return;
      }

      t=t->next;
    }
  }
  
  static void resetStateVars()
  {
    sU32 i;
    
    for (i=0; i<8; i++)
    {
      oldVertexBuffer[i]=newVertexBuffer[i]=0;
      oldVBStride[i]=newVBStride[i]=0;
    }
    
    oldIndexBuffer=newIndexBuffer=0;
    oldIBStart=newIBStart=0;
    
    oldVShader=newVShader=0;

#ifdef FVS_SUPPORT_VERTEX_SHADERS
    for (i=0; i<96*4; i++)
      oldVSConstants[i]=newVSConstants[i]=0;
    
    vsModStart=96;
    vsModEnd=0;
#endif
  }

  static D3DFORMAT zbufFormats[]={D3DFMT_D32, D3DFMT_D24X8, D3DFMT_D24S8, D3DFMT_D24X4S4,
    D3DFMT_D16, D3DFMT_D15S1, D3DFMT_D16_LOCKABLE, D3DFMT_UNKNOWN};

  static inline sInt abs(sInt x)
  {
    return x<0?-x:x;
  }

  void initDirectX()
  {
    sU32 i, j, k, bestMatch=(sU32) -1;

    d3d=Direct3DCreate9(D3D_SDK_VERSION);
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
    fr::debugOut("ENUM: direct3d8 devices:\n");
#endif

    numDevices=d3d->GetAdapterCount();
    devices=new deviceInfo[numDevices];

    conf.mode=0;
    conf.device=D3DADAPTER_DEFAULT;

    for (i=0; i<numDevices; i++)
    {
      deviceInfo *dev=devices+i;

      // rahmendaten holen
      checkError(d3d->GetAdapterIdentifier(i, 0, &dev->adapterid),
        "GetAdapterIdentifier FAILED");
      checkError(d3d->GetAdapterDisplayMode(i, &dev->mode), "GetAdapterDisplayMode FAILED");
      dev->monitor=d3d->GetAdapterMonitor(i);

      // modes enumerieren
      dev->numModes=0;
      dev->modes=new D3DDISPLAYMODE[d3d->GetAdapterModeCount(i, D3DFMT_X8R8G8B8)];

      for (j=0; j<d3d->GetAdapterModeCount(i, D3DFMT_X8R8G8B8); j++)
      {
        D3DDISPLAYMODE *mode=dev->modes+dev->numModes;

        checkError(d3d->EnumAdapterModes(i, D3DFMT_X8R8G8B8, j, mode), "EnumAdapterModes FAILED");

        if ((mode->Width>=conf.minXRes) && (mode->Width<=conf.maxXRes) &&
          (mode->Height>=conf.minYRes) && (mode->Height<=conf.maxYRes))
        {
          for (k=0; k<dev->numModes; k++)
            if (dev->modes[k].Width==mode->Width && dev->modes[k].Height==mode->Height &&
              dev->modes[k].Format==mode->Format)
              break;

          if (k==dev->numModes)
          {
            sU32 match=abs(mode->Width-conf.prefXRes)+abs(mode->Height-conf.prefYRes);
            sU32 modeBpp=16;

            if (mode->Format==D3DFMT_A8R8G8B8 || mode->Format==D3DFMT_X8R8G8B8)
              modeBpp=32;

            if (modeBpp!=conf.prefDepth)
              match++;

            if (match<bestMatch && i==conf.device)
            {
              conf.mode=k;
              conf.xRes=mode->Width;
              conf.yRes=mode->Height;
              bestMatch=match;
            }

            dev->numModes++;
          }
        }
      }
      
      // devices durchgehen
      for (j=0; j<nDeviceTypes; j++)
      {
        outputInfo *out=dev->outputs+j;

        out->type=(D3DDEVTYPE) (j+1);

        if (d3d->GetDeviceCaps(i, (D3DDEVTYPE) (j+1), &out->caps)!=D3D_OK)
          dev->numModes=0;
      }
      
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
      fr::debugOut("ENUM: %3d. %s (%d modes)\n", i+1, dev->adapterid.Description, dev->numModes);
#endif
    }
  }
  
  static void freeEnumInfo()
  {
    for (sInt i=0; i<numDevices; i++)
      delete[] devices[i].modes;

    delete[] devices;
  }

  void startupDirectX(HWND hWnd)
  {
    D3DDISPLAYMODE  mode;
    sU32            i;

    resourceMgr=new iResourceManager;
    
    memcpy(&device, devices+conf.device, sizeof(deviceInfo));
    memcpy(&output, device.outputs+0, sizeof(outputInfo));

    devType=output.type;
  	hDevWindow=0;

    hFocusWindow=hWnd;

#ifdef FVS_SUPPORT_WINDOWED
    mode=conf.windowed?device.mode:devices->modes[conf.mode];
#else
    mode=devices->modes[conf.mode];
#endif
    conf.fmt=mode.Format;

    switch (conf.fmt)
    {
    case D3DFMT_X1R5G5B5: conf.depth=16; break;
    case D3DFMT_R5G6B5:   conf.depth=16; break;
    case D3DFMT_X8R8G8B8: conf.depth=32; break;
    case D3DFMT_A8R8G8B8: conf.depth=32; break;
    }

    memset(&pparm, 0, sizeof(pparm));

#ifdef FVS_SUPPORT_WINDOWED
    if (!conf.windowed)
#endif
    {
  		WNDCLASS  wc;
      HINSTANCE hInst=GetModuleHandle(0);
  		
  		wc.style          = CS_BYTEALIGNCLIENT;
  		wc.lpfnWndProc    = DeviceWindowProc;
  		wc.cbClsExtra     = 0;
  		wc.cbWndExtra     = 0;
  		wc.hInstance      = hInst;
  		wc.hIcon          = 0;
  		wc.hCursor        = 0;
  		wc.hbrBackground  = (HBRUSH) GetStockObject(NULL_BRUSH);
  		wc.lpszMenuName   = 0;
  		wc.lpszClassName  = "fvs32dw";

  		RegisterClass(&wc);
  		hDevWindow=CreateWindowEx(0, "fvs32dw", "fvs32dw", WS_POPUP, 0, 0, 0, 0, hWnd, 0, hInst, 0);

  		if (!hDevWindow)
        fr::errorExit("Cannot create DirectX device window");

      pparm.BackBufferWidth=conf.xRes;
      pparm.BackBufferHeight=conf.yRes;
      pparm.BackBufferFormat=mode.Format;
      pparm.BackBufferCount=conf.triplebuffer?2:1;

      pparm.MultiSampleType=D3DMULTISAMPLE_NONE;

      pparm.SwapEffect=D3DSWAPEFFECT_FLIP;
      pparm.hDeviceWindow=hDevWindow;
      pparm.Windowed=FALSE;
      pparm.EnableAutoDepthStencil=FALSE;
      pparm.Flags=0;
      pparm.PresentationInterval=D3DPRESENT_INTERVAL_DEFAULT;
    }
#ifdef FVS_SUPPORT_WINDOWED
    else
    {
      pparm.BackBufferWidth=conf.xRes;
      pparm.BackBufferHeight=conf.yRes;
      pparm.BackBufferFormat=mode.Format;
      pparm.BackBufferCount=1;

      pparm.MultiSampleType=D3DMULTISAMPLE_NONE;

      pparm.SwapEffect=D3DSWAPEFFECT_COPY;
      pparm.hDeviceWindow=0;
      pparm.Windowed=TRUE;
      pparm.EnableAutoDepthStencil=FALSE;
      pparm.Flags=0;
			pparm.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    }
#endif

    sU32 behavior=0;

#ifdef FVS_SUPPORT_MULTITHREADING
    if (conf.multithread)
      behavior|=D3DCREATE_MULTITHREADED;
#endif

    if (output.caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
    {
      behavior |= D3DCREATE_HARDWARE_VERTEXPROCESSING;

      if (output.caps.DevCaps & D3DDEVCAPS_PUREDEVICE)
        behavior |= D3DCREATE_PUREDEVICE;
    }
    else
      behavior |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;

    HRESULT hrCreate;
    
    do
    {
      const sInt bbCount=pparm.BackBufferCount;

      hrCreate=d3d->CreateDevice(conf.device, output.type, hWnd, behavior, &pparm, &d3ddev);

      if (hrCreate==D3DERR_OUTOFVIDEOMEMORY && !pparm.Windowed && bbCount>1)
      {
        // nicht genug vram, pparm enthält BackBufferCount, die gehen sollte
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
        fr::debugOut("DCTX: out of VRAM, retrying...\n");
#endif
      }
      else
        checkError(hrCreate, "CreateDevice FAILED");
    } while (hrCreate!=D3D_OK);

#ifdef FVS_SUPPORT_GAMMA
    D3DGAMMARAMP    ramp;
    
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
    fr::debugOut("DCTX: gamma is %1.3f\n", conf.gamma);
#endif

    sF32 gv=1.0f/conf.gamma;
    for (i=0; i<256; i++)
      ramp.red[i]=ramp.green[i]=ramp.blue[i]=65535.0f*pow((sF32) i/255.0f, gv);

    d3ddev->SetGammaRamp(D3DSGR_NO_CALIBRATION, &ramp);
#endif

    // zbuffer-format raussuchen
    zbufFormat=D3DFMT_UNKNOWN;
    for (i=0; zbufFormats[i]!=D3DFMT_UNKNOWN; i++)
      if (checkDepthFormat(zbufFormats[i]))
      {
        zbufFormat=zbufFormats[i];
        break;
      }

    if (zbufFormat==D3DFMT_UNKNOWN)
      fr::errorExit("No suitable Depth Buffer Format found");

    if (zbufFormat==D3DFMT_D24S8 || zbufFormat==D3DFMT_D15S1 || zbufFormat==D3DFMT_D24X4S4)
      clearStencil=sTRUE; // always clear stencil when we have it but won't use it anyway

    checkError(d3ddev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back), "Couldn't get Backbuffer");
    checkError(d3ddev->CreateDepthStencilSurface(conf.xRes, conf.yRes, zbufFormat, D3DMULTISAMPLE_NONE,
      0, TRUE, &zbuffer, 0), "ZBuffer create FAILED");

		setViewport(0, 0, conf.xRes, conf.yRes);

    curtarget=addRenderTarget(back);
    curtarget->rw=curtarget->w=conf.xRes;
    curtarget->rh=curtarget->h=conf.yRes;
    curtarget->zbuf=0;
    curtarget->hasZB=sTRUE;

    resetStateVars();

		checkError(d3ddev->SetRenderTarget(0, back), "SetRenderTarget FAILED");
		checkError(d3ddev->SetDepthStencilSurface(zbuffer), "SetDepthStencilSurface FAILED");
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
    fr::debugOut("DCTX: init\n");
#endif
  }

  void closeDirectX()
  {
    iRenderTarget *t, *t2;

    for (t=targets; t; t=t2)
    {
      t2=t->next;
			if (t->tgt) t->tgt->Release();
			if (t->sysmemcopy) t->sysmemcopy->Release();
			if (t->smcsurface) t->smcsurface->Release();
			if (t->texture) delete t->texture;
			if (t->zbuf) releaseZBuffer(t->zbuf);
      delete t; // die texturen werden vom texturcode gefreed
    }

  	while (zBuffers)
      releaseZBuffer(zBuffers->resid); // hack

    vbSlice::globalCleanup();
    ibSlice::globalCleanup();
    
    FRSAFEDELETE(resourceMgr);

    if (zbuffer)
      zbuffer->Release();

    killIdleHandlers();
    freeEnumInfo();

    d3ddev->Release();
    d3d->Release();
    
  	if (hDevWindow)
  	{
  		DestroyWindow(hDevWindow);
  		hDevWindow=0;
  	}
  }

  void doSetRenderTarget(iRenderTarget *tgt, sBool force=sFALSE);

  // -------------------------------------------------------------------------------------------

#ifdef FVS_SUPPORT_RESIZEABLE_BACKBUFFER
  void resizeBackbuffer(sInt xRes, sInt yRes)
  {
    if (!d3ddev)
      return;

    resourceMgr->save();
    resourceMgr->kill();

    if (back)
    {
      back->Release();
      back=0;
    }

    if (zbuffer)
    {
      zbuffer->Release();
      zbuffer=0;
    }

    inscene=sFALSE;

    pparm.BackBufferWidth=xRes;
    pparm.BackBufferHeight=yRes;

    for (sU32 i=0; i<output.caps.MaxTextureBlendStages; i++)
      d3ddev->SetTexture(i, 0);

    checkError(d3ddev->Reset(&pparm), "Device Reset failed in fvsResizeBackbuffer!");
    
    conf.xRes=xRes;
    conf.yRes=yRes;
    
    resourceMgr->recreate();

    checkError(d3ddev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back), "Couldn't get Backbuffer in fvs::resizeBackbuffer");
    checkError(d3ddev->CreateDepthStencilSurface(conf.xRes, conf.yRes, zbufFormat, D3DMULTISAMPLE_NONE,
      0, TRUE, &zbuffer, 0), "ZBuffer create FAILED in fvs::resizeBackbuffer");
    
    iRenderTarget *prim=targets;
    
    prim->tgt=back;
    prim->rw=prim->w=xRes;
    prim->rh=prim->h=yRes;
    doSetRenderTarget(prim, sTRUE);
    
    d3ddev->SetTexture(0, 0);
    setViewport(viewport.xstart, viewport.ystart, viewport.width, viewport.height);

    resetStateVars();
  }
#endif
  
  // -------------------------------------------------------------------------------------------

  void updateScreen(HWND hWnd, LPRECT rect)
  {
    HRESULT hr;

    if (!d3ddev)
      return;

    if ((hr=d3ddev->TestCooperativeLevel())!=D3D_OK)
    {
      while (hr==D3DERR_DEVICELOST)
      {
        idle();
        hr=d3ddev->TestCooperativeLevel();
      }
      
      if (hr==D3DERR_DEVICENOTRESET)
      {
        resourceMgr->kill();

        if (back)
        {
          back->Release();
          back=0;
        }

        if (zbuffer)
        {
          zbuffer->Release();
          zbuffer=0;
        }

        inscene=sFALSE;
        
        for (sU32 i=0; i<output.caps.MaxTextureBlendStages; i++)
          d3ddev->SetTexture(i, 0);

        checkError(d3ddev->Reset(&pparm), "Device Reset failed in fvsUpdateFrame!");
        
        resourceMgr->recreate();
        checkError(d3ddev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back), "Couldn't get Backbuffer");
        checkError(d3ddev->CreateDepthStencilSurface(conf.xRes, conf.yRes, zbufFormat, D3DMULTISAMPLE_NONE,
          0, TRUE, &zbuffer, 0), "ZBuffer create FAILED in fvsUpdateScreen");

        targets->tgt=back;
        setRenderTarget(0, sTRUE);
        d3ddev->SetTexture(0, 0);
        setViewport(viewport.xstart, viewport.ystart, viewport.width, viewport.height);

        resetStateVars();
      }
      else
        checkError(hr, "TestCooperativeLevel failed");
    }

    if (inscene)
    {
  		if (d3ddev->EndScene()!=D3D_OK)
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
        fr::debugOut("DCTX: device->EndScene() failed\n");
#else
        ;
#endif
  		else
  			inscene=sFALSE;
    }

    d3ddev->Present(0, rect, hWnd, 0);

    idle();

    setRenderTarget(0, sTRUE);
    updateAnimatedTexturesPerFrame();
  }

  void setViewport(sU32 xs, sU32 ys, sU32 w, sU32 h)
  {
    D3DVIEWPORT9 dvp={xs, ys, w, h, 0.0f, 1.0f};

    if (!d3ddev)
      return;

    if (viewport.xstart==xs && viewport.ystart==ys && viewport.width==w && viewport.height==h)
      return;

    if (d3ddev->SetViewport(&dvp)!=D3D_OK)
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
      fr::debugOut("DCTX: failed to set viewport (x: %d y: %d w: %d h: %d)\n", xs, ys, w, h);
#else
      ;
#endif
    else
    {
      viewport.xstart=xs;
      viewport.ystart=ys;
      viewport.width=w;
      viewport.height=h;
      viewport.xend=xs+w;
      viewport.yend=ys+h;
    }
  }

  void getTransform(sU32 state, fr::matrix &tr)
  {
    if (d3ddev)
      d3ddev->GetTransform((D3DTRANSFORMSTATETYPE) state, tr.d3d());
  }

  void setTransform(sU32 state, const fr::matrix &tr)
  {
    if (d3ddev)
      d3ddev->SetTransform((D3DTRANSFORMSTATETYPE) state, tr.d3d());
  }
  
  static void setVertexBuffer(sU32 strm, IDirect3DVertexBuffer9* vbuf, sU32 stride)
  {
    newVertexBuffer[strm] = vbuf;
    newVBStride[strm] = stride;
  }

  static void setIndexBuffer(IDirect3DIndexBuffer9* ibuf, sU32 start)
  {
    newIndexBuffer = ibuf;
    newIBStart = start;
  }

  static void setVertexShader(sU32 shader)
  {
    newVShader = shader;
  }

  static void doSetVertexShader(sU32 id);
  static void enableVertexShaders(sBool state);
  static sBool isVertexShader(sU32 id);

  static void updateStates()
  {
    sInt  i;

    if (!d3ddev)
      return;

    if (newVShader!=oldVShader)
    {
#ifdef FVS_SUPPORT_VERTEX_SHADERS
      if (isVertexShader(newVShader))
        doSetVertexShader(newVShader);
      else
#endif
      {
#ifdef FVS_SUPPORT_VERTEX_SHADERS
        enableVertexShaders(sFALSE);
#endif
        //checkError(d3ddev->SetVertexShader(newVShader), "SetVertexShader FAILED", sFALSE);
				// TODO: need real vertex shader support here (do i?)
				checkError(d3ddev->SetFVF(newVShader), "SetFVF FAILED", sFALSE);
      }

      oldVShader=newVShader;
    }

    for (i=0; i<8; i++)
    {
      if (newVertexBuffer[i]!=oldVertexBuffer[i] || newVBStride[i]!=oldVBStride[i])
      {
        checkError(d3ddev->SetStreamSource(i, newVertexBuffer[i], 0, newVBStride[i]), "SetStreamSource FAILED", sFALSE);
        oldVertexBuffer[i]=newVertexBuffer[i];
        oldVBStride[i]=newVBStride[i];
      }
    }

    if (newIndexBuffer!=oldIndexBuffer)
    {
      checkError(d3ddev->SetIndices(newIndexBuffer), "SetIndices FAILED", sFALSE);
      oldIndexBuffer=newIndexBuffer;
      oldIBStart=newIBStart;
    }

#ifdef FVS_SUPPORT_VERTEX_SHADERS
    if (!noVertexShaders)
      for (i=0; i<96*4; i++) // suckt, aber nur TESTWEISE
      {
        if (newVSConstants[i]!=oldVSConstants[i])
        {
          checkError(d3ddev->SetVertexShaderConstant(i>>2, &newVSConstants[i&~3], 1),
            "SetVertexShaderConstant FAILED", sFALSE);

          oldVSConstants[(i&~3)+0]=newVSConstants[(i&~3)+0];
          oldVSConstants[(i&~3)+1]=newVSConstants[(i&~3)+1];
          oldVSConstants[(i&~3)+2]=newVSConstants[(i&~3)+2];
          oldVSConstants[(i&~3)+3]=newVSConstants[(i&~3)+3];

          i=(i&~3)+3;
        }
      }

    vsModStart=96;
    vsModEnd=0;
#endif
  }

  void clear(sU32 mode, sU32 color, sF32 z, sU32 stencil)
  {
    if (!d3ddev)
      return;

    if (!curtarget->hasZB)
      mode&=~D3DCLEAR_ZBUFFER;

    if (clearStencil && (mode&D3DCLEAR_ZBUFFER) && !curtarget->texture)
      mode|=D3DCLEAR_STENCIL;

  	if (!inscene)
  	{
  	  if (d3ddev->BeginScene()!=D3D_OK)
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
        fr::debugOut("DCTX: device->BeginScene() failed\n");
#else
        ;
#endif
  		else
  			inscene=sTRUE;
  	}

    sBool setVp=(curtarget->tgt!=back);

    D3DVIEWPORT9 tvp={0, 0, curtarget->rw, curtarget->rh, 0.0f, 1.0f};

    if (setVp && d3ddev->SetViewport(&tvp)!=D3D_OK)
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
      fr::debugOut("DCTX: device->SetViewport failed in clear\n");
#else
      ;
#endif

    if (d3ddev->Clear(0, 0, mode, color, z, stencil)!=D3D_OK)
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
      fr::debugOut("DCTX: device->Clear() failed\n");
#else
      ;
#endif

    tvp.X=viewport.xstart;
    tvp.Y=viewport.ystart;
    tvp.Width=viewport.width;
    tvp.Height=viewport.height;

    if (setVp && d3ddev->SetViewport(&tvp)!=D3D_OK)
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
      fr::debugOut("DCTX: device->SetViewport failed in clear\n");
#else
      ;
#endif

	}

  // ---------------------------------------------------------------------------------------------
  // vertex shader support
  // ---------------------------------------------------------------------------------------------

#ifdef FVS_SUPPORT_VERTEX_SHADERS
  
  static void analyzeVertexShader(const sU32 *decl, sInt &minStream, sInt &maxStream, sInt *strides)
  {
    sInt    i, ip, curStream;
    sBool   end=sFALSE;
    sU32    opcode;

    minStream=16;
    maxStream=-1;

    for (i=0; i<16; i++)
      strides[i]=0;

    curStream=-1;
    ip=0;

    while (!end)
    {
      opcode=decl[ip];
      ip++;

      switch (opcode>>29)
      {
      case 0: // nop
        break;

      case 1: // stream selector
        curStream=opcode & 0xf;
        if (curStream<minStream)
          minStream=curStream;

        if (curStream>maxStream)
          maxStream=curStream;
        break;

      case 2: // vertex input register load/data skip
        if (((opcode>>28)&1)==0) // vertex input register load
        {
          switch ((opcode>>16) & 0xf) // type info
          {
          case D3DVSDT_FLOAT1:    strides[curStream]+=sizeof(sF32)*1;  break;
          case D3DVSDT_FLOAT2:    strides[curStream]+=sizeof(sF32)*2;  break;
          case D3DVSDT_FLOAT3:    strides[curStream]+=sizeof(sF32)*3;  break;
          case D3DVSDT_FLOAT4:    strides[curStream]+=sizeof(sF32)*4;  break;
          case D3DVSDT_D3DCOLOR:  strides[curStream]+=sizeof(sU32);    break;
          case D3DVSDT_UBYTE4:    strides[curStream]+=sizeof(sU8)*4;   break;
          case D3DVSDT_SHORT2:    strides[curStream]+=sizeof(sU16)*2;  break;
          case D3DVSDT_SHORT4:    strides[curStream]+=sizeof(sU16)*4;  break;
          }
        }
        else // data skip
          strides[curStream]+=((opcode>>16) & 0xf)*sizeof(sU32);
      
        break;
      
      case 3: // vertex input from tesselator
        fr::errorExit("Tesselator streams not supported yet\n");
        break;
      
      case 4: // constant memory from shader
        ip+=(opcode>>25)&0xf; // skip over addresses
        break;
      
      case 5: // extension
        ip+=(opcode>>24)&0x1f; // skip over extension dwords
        break;
      
      case 6: // reserved
        break;
      
      case 7: // end token
        end=sTRUE;
        break;
      }
    }
  }

  struct iVertexShader
  {
    sU32    id;
    sU32    handle;
    sU32    *declaration;
    sU32    *function;
    sInt    strides[16];

    void init(sU32 theID, const sU32 *decl, const sU32 *func)
    {
      sU32  decSize, funcSize;
      sInt  minStr, maxStr;

      id=theID;
      handle=0;

      for (decSize=0; decl[decSize]!=0xffffffff; decSize++);
      decSize++;

      declaration=new sU32[decSize];
      memcpy(declaration, decl, decSize*sizeof(sU32));

      analyzeVertexShader(decl, minStr, maxStr, strides);

      if (func)
      {
        for (funcSize=0; func[funcSize]!=0xffff; funcSize++);
        funcSize++;

        function=new sU32[funcSize];
        memcpy(function, func, funcSize*sizeof(sU32));
      }
      else
        function=0;
    }

    void close()
    {
      destroy();

      FRSAFEDELETEA(declaration);
      FRSAFEDELETEA(function);
    }

    void create()
    {
      sU32  usage;

      destroy();

      if (output.caps.DevCaps & (D3DDEVCAPS_HWTRANSFORMANDLIGHT | D3DDEVCAPS_PUREDEVICE)==(D3DDEVCAPS_HWTRANSFORMANDLIGHT | D3DDEVCAPS_PUREDEVICE))
        usage=0;
      else
        usage=D3DUSAGE_SOFTWAREPROCESSING;

      checkError(d3ddev->CreateVertexShader(declaration, function, &handle, usage), "CreateVertexShader FAILED", sFALSE);
    }

    void destroy()
    {
      if (handle)
      {
        checkError(d3ddev->DeleteVertexShader(handle), "DeleteVertexShader FAILED", sFALSE);
        handle=0;
      }
    }

    void use()
    {
      enableVertexShaders(sTRUE);

      if (!handle)
        create();

      checkError(d3ddev->SetVertexShader(handle), "SetVertexShader FAILED", sFALSE);
    }
  };

  typedef std::list<iVertexShader>    vshList;
  typedef vshList::iterator           vshIt;

  static vshList                      vertexShaders;
  static sU32                         vshIdCounter=1;

  static void enableVertexShaders(sBool state)
  {
    if (output.caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
    {
      if (!(output.caps.DevCaps & D3DDEVCAPS_PUREDEVICE))
      {
        checkError(d3ddev->SetRenderState(D3DRS_SOFTWAREVERTEXPROCESSING, state), "Cannot enable/disable software vertex processing");

        noVertexShaders=!state;
      }
    }
  }

  static void doSetVertexShader(sU32 id)
  {
    for (vshIt it=vertexShaders.begin(); it!=vertexShaders.end(); ++it)
    {
      if (it->id==id)
      {
        it->use();
        return;
      }
    }

    fr::errorExit("DCTX: Vertex Shader with id #%08x not available", id);
  }

  sU32 createVertexShader(const sU32 *decl, const sU32 *func)
  {
    iVertexShader   vsh;

    vsh.init(vshIdCounter, decl, func);
    vshIdCounter+=2; // ja, 2

    vertexShaders.push_back(vsh);

    return vsh.id;
  }

  void destroyVertexShader(sU32 id)
  {
    for (vshIt it=vertexShaders.begin(); it!=vertexShaders.end(); ++it)
    {
      if (it->id==id)
      {
        it->close();
        it=vertexShaders.erase(it);
        return;
      }
    }

    fr::errorExit("DCTX: Cannot delete nonexistant vertex shader #%08x\n", id);
  }

  sU32 getVertexShaderStride(sU32 id, sInt strm)
  {
    if (strm<0 || strm>=16)
      return 0;

    for (vshIt it=vertexShaders.begin(); it!=vertexShaders.end(); ++it)
      if (it->id==id)
        return it->strides[strm];

    return 0;
  }

  void setVertexShaderStride(sU32 id, sInt strm, sU32 stride)
  {
    if (strm<0 || strm>=16)
      return;

    for (vshIt it=vertexShaders.begin(); it!=vertexShaders.end(); ++it)
      if (it->id==id)
        it->strides[strm]=stride;
  }

  sBool isVertexShader(sU32 id)
  {
    if (id&1!=1)
      return sFALSE;

    for (vshIt it=vertexShaders.begin(); it!=vertexShaders.end(); ++it)
      if (it->id==id)
        return sTRUE;

    return sFALSE;
  }

  static __forceinline sU32 translateVertexShaderConstant(sU32 constant, sU32 count=1)
  {
    if (constant+count>96)
      constant=96;
    
    if (constant<vsModStart)      vsModStart=constant;
    if (constant+count>=vsModEnd) vsModEnd=constant+count-1;
    
    return constant*4;
  }
  
  void setVertexShaderConstant(sU32 constant, sF32 x, sF32 y, sF32 z, sF32 w)
  {
    constant=translateVertexShaderConstant(constant);
    newVSConstants[constant+0]=*((sU32 *) &x);
    newVSConstants[constant+1]=*((sU32 *) &y);
    newVSConstants[constant+2]=*((sU32 *) &z);
    newVSConstants[constant+3]=*((sU32 *) &w);
  }
  
  void setVertexShaderConstant(sU32 constant, const fr::vector &v, sF32 w)
  {
    constant=translateVertexShaderConstant(constant);
    newVSConstants[constant+0]=*((sU32 *) &v.x);
    newVSConstants[constant+1]=*((sU32 *) &v.y);
    newVSConstants[constant+2]=*((sU32 *) &v.z);
    newVSConstants[constant+3]=*((sU32 *) &w);
  }
  
  void setVertexShaderConstant(sU32 constant, const fr::matrix &m)
  {
    // matrix wird wenn nötig automatisch transponiert
    constant=translateVertexShaderConstant(constant, 4);
    newVSConstants[constant+ 0]=*((sU32 *) &m(0,0));
    newVSConstants[constant+ 1]=*((sU32 *) &m(1,0));
    newVSConstants[constant+ 2]=*((sU32 *) &m(2,0));
    newVSConstants[constant+ 3]=*((sU32 *) &m(3,0));
    newVSConstants[constant+ 4]=*((sU32 *) &m(0,1));
    newVSConstants[constant+ 5]=*((sU32 *) &m(1,1));
    newVSConstants[constant+ 6]=*((sU32 *) &m(2,1));
    newVSConstants[constant+ 7]=*((sU32 *) &m(3,1));
    newVSConstants[constant+ 8]=*((sU32 *) &m(0,2));
    newVSConstants[constant+ 9]=*((sU32 *) &m(1,2));
    newVSConstants[constant+10]=*((sU32 *) &m(2,2));
    newVSConstants[constant+11]=*((sU32 *) &m(3,2));
    newVSConstants[constant+12]=*((sU32 *) &m(0,3));
    newVSConstants[constant+13]=*((sU32 *) &m(1,3));
    newVSConstants[constant+14]=*((sU32 *) &m(2,3));
    newVSConstants[constant+15]=*((sU32 *) &m(3,3));
  }
  
#endif

  // ---------------------------------------------------------------------------------------------
  // user pointer drawing calls
  // ---------------------------------------------------------------------------------------------
  
#ifdef FVS_SUPPORT_USER_POINTER_DRAWING

  void drawLineList(const void *v, sU32 n, sU32 vertFormat)
  {
    if (d3ddev && n)
    {
      setVertexShader(vertFormat);
      updateStates();

      if (d3ddev->DrawPrimitiveUP(D3DPT_LINELIST, n>>1, v, getFVFSize(vertFormat))!=D3D_OK)
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
        fr::debugOut("DCTX: DrawPrimitiveUP failed\n");
#else
        ;
#endif

      oldVertexBuffer[0]=newVertexBuffer[0]=0; // UP-calls trashen stream 0
    }
  }

  void drawLineStrip(const void *v, sU32 n, sU32 vertFormat)
  {
    if (d3ddev && n)
    {
      setVertexShader(vertFormat);
      updateStates();

      if (d3ddev->DrawPrimitiveUP(D3DPT_LINESTRIP, n-1, v, getFVFSize(vertFormat))!=D3D_OK)
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
        fr::debugOut("DCTX: DrawPrimitiveUP failed\n");
#else
        ;
#endif
      
      oldVertexBuffer[0]=newVertexBuffer[0]=0; // UP-calls trashen stream 0
    }
  }

  void drawTriList(const void *v, sU32 n, sU32 vertFormat)
  {
    if (d3ddev && n)
    {
      setVertexShader(vertFormat);
      updateStates();
      
      if (d3ddev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, n/3, v, getFVFSize(vertFormat))!=D3D_OK)
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
        fr::debugOut("DCTX: DrawPrimitiveUP failed\n");
#else
        ;
#endif
      
      oldVertexBuffer[0]=newVertexBuffer[0]=0; // UP-calls trashen stream 0
    }
  }

  void drawTriFan(const void *v, sU32 n, sU32 vertFormat)
  {
    if (d3ddev && n)
    {
      setVertexShader(vertFormat);
      updateStates();
      
      if (d3ddev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, n-2, v, getFVFSize(vertFormat))!=D3D_OK)
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
        fr::debugOut("DCTX: DrawPrimitiveUP failed\n");
#else
        ;
#endif
      
      oldVertexBuffer[0]=newVertexBuffer[0]=0; // UP-calls trashen stream 0
    }
  }

  void drawTriStrip(const void *v, sU32 n, sU32 vertFormat)
  {
    if (d3ddev && n)
    {
      setVertexShader(vertFormat);
      updateStates();
      
      if (d3ddev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, n-2, v, getFVFSize(vertFormat))!=D3D_OK)
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
        fr::debugOut("DCTX: DrawPrimitiveUP failed\n");
#else
        ;
#endif
      
      oldVertexBuffer[0]=newVertexBuffer[0]=0; // UP-calls trashen stream 0
    }
  }

#endif

  // ---------------------------------------------------------------------------------------------
  // vertex buffer drawing calls
  // ---------------------------------------------------------------------------------------------

  void drawPrimitive(sU32 type, sU32 startv, sU32 pcount)
  {
    if (d3ddev && pcount)
    {
      updateStates();
      
      if (d3ddev->DrawPrimitive((D3DPRIMITIVETYPE) type, startv, pcount)!=D3D_OK)
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
        fr::debugOut("DCTX: DrawPrimitive failed\n");
#else
        ;
#endif
    }
  }

  void drawIndexedPrimitive(sU32 type, sU32 startv, sU32 vcount, sU32 starti, sU32 pcount)
  {
    if (d3ddev && vcount && pcount)
    {
      updateStates();
      
      if (d3ddev->DrawIndexedPrimitive((D3DPRIMITIVETYPE) type, newIBStart, startv, vcount, starti, pcount)!=D3D_OK)
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
        fr::debugOut("DCTX: DrawIndexedPrimitive failed\n");
#else
        ;
#endif
    }
  }

  // ---------------------------------------------------------------------------------------------

  sU32 newRenderTarget(sU32 w, sU32 h, sBool zBuffer, sBool lockable)
  {
    D3DSURFACE_DESC   desc;
    iRenderTarget     *rt;
    IDirect3DSurface9 *srf;
    texture           *tex;

    tex=new texture(w, h, texfRGB|texfRTT); ;
    srf=tex->getSurfaceLevel(0);

    if (!srf)
      fr::errorExit("GetSurfaceLevel FAILED");

    rt=addRenderTarget(0);

    memset(&desc, 0, sizeof(desc));
    checkError(srf->GetDesc(&desc), "GetDesc FAILED");

		rt->texture=tex;
    rt->w=w;
    rt->h=h;
    rt->rw=desc.Width;
    rt->rh=desc.Height;

		srf->Release();

    if (zBuffer)
    {
      if (!(rt->zbuf=getZBuffer(rt->rw, rt->rh)))
        fr::errorExit("Couldn't get matching ZBuffer for Rendertarget %d", rt->id);
      rt->hasZB=sTRUE;
    }
    else
    {
      rt->zbuf=0;
      rt->hasZB=sFALSE;
    }

		rt->sysmemcopy=0;
		if (lockable)
		{
			HRESULT hr=d3ddev->CreateTexture(rt->rw,rt->rh,1,0,desc.Format,D3DPOOL_SYSTEMMEM,&rt->sysmemcopy,0);
			if (!rt->sysmemcopy) fr::errorExit("Couldn't create sysmem copy texture for Rendertarget %d (%08x)", rt->id,hr);
			rt->smcsurface=0;
			hr=rt->sysmemcopy->GetSurfaceLevel(0,&rt->smcsurface);
			if (!rt->smcsurface) fr::errorExit("Couldn't create sysmem copy surface for Rendertarget %d (%08x)", rt->id,hr);
		}
    return rt->id;
  }

	sU32 * lockRenderTarget(sU32 id)
	{
    iRenderTarget *t;
		for (t=targets; t; t=t->next)
      if (t->id==id)
        break;

		if (t && t->sysmemcopy && t->smcsurface && t->texture)
		{
			// TODO: fix this
/*			IDirect3DSurface9 *srf=t->texture->getSurfaceLevel(0);
			setRenderTarget(0);
			RECT r;
			r.left=0;
			r.bottom=t->h;
			r.top=0;
			r.right=t->w;
			d3ddev->CopyRects(srf,&r,1,t->smcsurface,0);
			srf->Release();*/

			D3DLOCKED_RECT lockrect;
			t->smcsurface->LockRect(&lockrect,0,D3DLOCK_READONLY|D3DLOCK_NOSYSLOCK);
			return (sU32 *)lockrect.pBits;
		}
		else
			return 0;
    
	}

  void unlockRenderTarget(sU32 id)
	{
    iRenderTarget *t;
		for (t=targets; t; t=t->next)
      if (t->id==id)
        break;
	  
		if (t && t->sysmemcopy && t->smcsurface)
			t->smcsurface->UnlockRect();
	}

	void releaseRenderTarget(sU32 id)
	{
		iRenderTarget *t, **p=&targets;
    while (t=*p)
		{						
			if (t->id==id)
			{
				if (t->tgt) t->tgt->Release();
				if (t->sysmemcopy) t->sysmemcopy->Release();
				if (t->smcsurface) t->smcsurface->Release();
				if (t->texture) delete t->texture;
				if (t->zbuf) releaseZBuffer(t->zbuf);
				*p=t->next;
			}
			else
				p=&t->next;
		}

	}


	void getRenderTargetDimensions(sInt id, sInt *w, sInt *h, sInt *rw, sInt *rh)
	{
    iRenderTarget *t;
		for (t=targets; t; t=t->next)
      if (t->id==id)
        break;
	  if (t)
		{
			if (w) *w=t->w;
			if (h) *h=t->h;
			if (rw) *rw=t->rw;
			if (rh) *rh=t->rh;
		}
		else
		{
			if (w) *w=0;
			if (h) *h=0;
			if (rw) *rw=0;
			if (rh) *rh=0;
		}
	}

  static void doSetRenderTarget(iRenderTarget *tgt, sBool force)
  {
    IDirect3DSurface9   *target;

  	if (!d3ddev || !tgt)
  		return;

    if (curtarget!=tgt || force)
    {
  		if (inscene)
  		{
  			if (d3ddev->EndScene()!=D3D_OK)
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
          fr::debugOut("DCTX: d3ddev->EndScene() failed\n");
#else
          ;
#endif
  			else
  				inscene=sFALSE;
  		}

      if (tgt->tgt)
        target=tgt->tgt;
      else
        target=tgt->texture->getSurfaceLevel(0);

      IDirect3DSurface9 *zb=tgt->zbuf?(IDirect3DSurface9 *) resourceMgr->getPtr(tgt->zbuf):zbuffer;
			if (!tgt->hasZB)
				zb=0;
      
			if (d3ddev->SetRenderTarget(0, target) != D3D_OK)
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
				fr::debugOut("DCTX: d3ddev->SetRenderTarget() failed\n");
#else
				;
#endif

			if (d3ddev->SetDepthStencilSurface(zb) != D3D_OK)
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
				fr::debugOut("DCTX: d3ddev->SetDepthStencilSurface() failed\n");
#else
				;
#endif

			//TODO: fix this
      //setRenderState(D3DRS_ZENABLE, !!tgt->hasZB);

      if (!tgt->tgt)
        target->Release();

      curtarget=tgt;
    }

    if (!inscene)
  	{
      if (d3ddev->BeginScene()!=D3D_OK)
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
        fr::debugOut("DCTX: device->BeginScene() failed\n");
#else
        ;
#endif
  		else
  			inscene=sTRUE;
  	}
  }

  void setRenderTarget(sU32 id, sBool force)
  {
    iRenderTarget *t;

    for (t=targets; t; t=t->next)
    {
      if (t->id==id)
        break;
    }

    if (!t)
    {
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
      fr::debugOut("DCTX: couldn't find rendertarget %d, setting primary\n", id);
#endif
      setRenderTarget((sU32) 0, force);
    }
    else
      doSetRenderTarget(t, force);
  }

  // ---------------------------------------------------------------------------------------------
  // texturcode
  // ---------------------------------------------------------------------------------------------

  // ---- pixel format conversion

  static  sU8     asl, asr, rsl, rsr, gsl, gsr, bsl, bsr;

#ifndef FVS_RGBA_TEXTURES_ONLY
  static void convertLineRGB(sU8 *dest, const sU8 *src, sU32 w, sInt bits)
  {
    sU32  out;

    while (w--)
    {
      out=((src[2]>>rsr)<<rsl)|((src[1]>>gsr)<<gsl)|((src[0]>>bsr)<<bsl);

      *dest++=out;
      if (bits>8)   *dest++=out>>8;
      if (bits>16)  *dest++=out>>16;
      if (bits>24)  *dest++=out>>24;
      src+=3;
    }
  }

  static void convertLineLum(sU8 *dest, const sU8 *src, sU32 w, sInt bits)
  {
    sU32  out;

    while (w--)
    {
      out=((*src>>rsr)<<rsl)|((*src>>gsr)<<gsl)|((*src>>bsr)<<bsl);
      *dest++=out;
      if (bits>8)   *dest++=out>>8;
      if (bits>16)  *dest++=out>>16;
      if (bits>24)  *dest++=out>>24;
      src++;
    }
  }

  static void convertLineAlpha(sU8 *dest, const sU8 *src, sU32 w, sInt bits)
  {
    sU32  out, base;

    base=((0xff>>rsr)<<rsl)|((0xff>>gsr)<<gsl)|((0xff>>bsr)<<bsl);

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
#endif

#if !defined(FVS_RGBA_TEXTURES_ONLY) || !defined(FVS_TINY_TEXTURE_CODE)
  static void convertLineRGBA(sU8 *dest, const sU8 *src, sU32 w, sInt bits)
  {
    sU32  out;

    while (w--)
    {
      out=((src[3]>>asr)<<asl)|((src[2]>>rsr)<<rsl)|((src[1]>>gsr)<<gsl)|((src[0]>>bsr)<<bsl);

      *dest++=out;
      if (bits>8)   *dest++=out>>8;
      if (bits>16)  *dest++=out>>16;
      if (bits>24)  *dest++=out>>24;
      src+=4;
    }
  }
#endif

#ifndef FVS_RGBA_TEXTURES_ONLY
  static void convertLineLumA(sU8 *dest, const sU8 *src, sU32 w, sInt bits)
  {
    sU32  out;

    while (w--)
    {
      out=((src[1]>>asr)<<asl)|((*src>>rsr)<<rsl)|((*src>>gsr)<<gsl)|((*src>>bsr)<<bsl);
      *dest++=out;
      if (bits>8)   *dest++=out>>8;
      if (bits>16)  *dest++=out>>16;
      if (bits>24)  *dest++=out>>24;
      src+=2;
    }
  }
#endif

  typedef void (*convertFunc)(sU8 *, const sU8 *, sU32, sInt);

  // ---- pixel format definitions

  struct iFormatDesc
  {
    D3DFORMAT     fmt;
    sU8           asl, asr, rsl, rsr, gsl, gsr, bsl, bsr;
    sInt          bits;
    convertFunc   func;
  };

#ifndef FVS_RGBA_TEXTURES_ONLY
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

#endif

#if !defined(FVS_RGBA_TEXTURES_ONLY) || !defined(FVS_TINY_TEXTURE_CODE)
  static iFormatDesc formatsRGBA[]={
    D3DFMT_A8R8G8B8,  24,0, 16,0, 8,0, 0,0,  32, convertLineRGBA,
    D3DFMT_A4R4G4B4,  12,4,  8,4, 4,4, 0,4,  16, convertLineRGBA,
    D3DFMT_A8R3G3B2,   8,0,  5,5, 2,5, 0,6,  16, convertLineRGBA,
    D3DFMT_A1R5G5B5,  15,7, 10,3, 5,3, 0,3,  16, convertLineRGBA,
    D3DFMT_UNKNOWN,    0,0,  0,0, 0,0, 0,0,   0, 0
  };
#endif

#ifndef FVS_RGBA_TEXTURES_ONLY

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
      for (x=0, xr=0; x<dw; x++, dst+=cpp, xr+=xs, src=srct+((xr>>16)*cpp))
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

#if !defined(FVS_RGBA_TEXTURES_ONLY) || !defined(FVS_TINY_TEXTURE_CODE)
  static void generateMipMap(const sU8 *inp, sU32 sw, sU32 sh, sU8 *dst, sInt cpp)
  {
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
          sInt v=(inp[io]+inp[io+cpp]+inp[io+lstep]+inp[io+lstep+cpp]+2)>>2;
          if (v>255)
            v=255;
          
          dst[o]=v;
        }
			}
		}
  }
#else
	static void generateMipMap(const sU8* inp, sU32 sw, sU32 sh, sU8* dst, sInt)
	{
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
	}
#endif
  
  // ---- support functions

  static sChar *formatFlags(sInt flags)
  {
#ifndef FVS_MINIMAL_DEBUG_OUTPUT
    static sChar buf[256];

    buf[0]=0;

    if (flags & texfRGB)        strcat(buf, "rgb");
    if (flags & texfGrayscale)  strcat(buf, "grayscale");
    if (flags & texfAlpha)      strcat(buf, buf[0]?"+alpha":"alpha");
    if (flags & texfLowQual)    strcat(buf, "+low quality");
    if (flags & texfRTT)        strcat(buf, "+rendertarget");
    if (flags & texfMipMaps)    strcat(buf, "+mip maps");
    if (flags & texfWrap)       strcat(buf, "+wrap");
    if (flags & texfAnimated)   strcat(buf, "+animated");
    if (flags & texfNoLag)      strcat(buf, "+no lag");

    return buf;
#else
    return "";
#endif
  }

  static sBool checkTextureFormat(D3DFORMAT fmt, sInt flags)
  {
    if (d3d->CheckDeviceFormat(conf.device, output.type, conf.fmt,
      (flags&texfRTT)?D3DUSAGE_RENDERTARGET:0, D3DRTYPE_TEXTURE, fmt)!=D3D_OK)
      return sFALSE;

    return sTRUE;
  }

#ifdef FVS_SUPPORT_CUBEMAPS
  static sBool checkCubemapFormat(D3DFORMAT fmt, sInt flags)
  {
    if (d3d->CheckDeviceFormat(conf.device, output.type, conf.fmt,
      (flags&texfRTT)?D3DUSAGE_RENDERTARGET:0, D3DRTYPE_CUBETEXTURE, fmt)!=D3D_OK)
      return sFALSE;
    
    return sTRUE;
  }
#endif
  
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

  // ---- support für animated textures

  struct iAnimatedTexture
  {
    texture           *tex;
    iAnimatedTexture  *next;
  };

  static iAnimatedTexture   *animTextures=0;

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

  baseTexture::operator IDirect3DBaseTexture9 *() const
  {
    if (this)
    {
      FRDASSERT(created);
      
      return (IDirect3DBaseTexture9 *) resourceMgr->getPtr(getResID());
    }
    else
      return 0;
  }

  void baseTexture::use(sU32 stage) const
  {
    if (this)
    {
      FRDASSERT(created);

      if (stage<output.caps.MaxTextureBlendStages)
        d3ddev->SetTexture(stage, (IDirect3DBaseTexture9 *) resourceMgr->getPtr(getResID()));
      else
        fr::debugOut("TEXX: more texture blend stages used than available\n");
    }
    else
      if (stage<output.caps.MaxTextureBlendStages)
        d3ddev->SetTexture(stage, 0);
      else
        fr::debugOut("TEXX: more texture blend stages used than available\n");
  }
  
  // ---- texture

  texture::texture()
  {
  }

  texture::texture(sU32 w, sU32 h, sU32 flags)
  {
    create(w, h, flags);
  }

  texture::texture(const sU8 *img, sU32 w, sU32 h, sU32 flags)
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

    if (animFlags==1)
      res=addResIDs[0];
    else if (animFlags==2)
      res=addResIDs[animDisp];

    return res;
  }

  texture::operator IDirect3DTexture9 *() const
  {
    if (this)
    {
      FRDASSERT(created);

      return (IDirect3DTexture9 *) resourceMgr->getPtr(getResID());
    }
    else
      return 0;
  }

  void texture::create(sU32 w, sU32 h, sU32 flags)
  {
    IDirect3DTexture9 *tex;
    sU32              usage;
    sU32              wn, hn, i;

#ifndef FVS_MINIMAL_DEBUG_OUTPUT
    if ((flags & texfRTT) && (flags & texfAnimated))
      fr::errorExit("Is texture now animated or rendertarget?");

    if ((flags & texfNoLag) && !(flags & texfAnimated))
      fr::debugOut("TEXX: NoLag only makes sense with animated textures\n");

    if ((flags & texfAnimated) && (flags & texfMipMaps))
      fr::debugOut("TEXX: Animated textures with mipmaps could cause serious performance problems\n");
#endif

    if (flags & texfRTT) 
      flags&=~texfMipMaps;

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
#elif !defined(FVS_TINY_TEXTURE_CODE)
    for (format=formatsRGBA; format->fmt!=D3DFMT_UNKNOWN; format++)
    {
      if (checkTextureFormat(format->fmt, flags))
        break;
    }
#endif

#if !defined(FVS_RGBA_TEXTURES_ONLY) || !defined(FVS_TINY_TEXTURE_CODE)
    if (format->fmt==D3DFMT_UNKNOWN)
      fr::errorExit("No matching texture format found! (w=%d, h=%d, flags=%s)", w, h, formatFlags(flags));
#endif

    if (flags & texfMipMaps) 
    {
      for (i=MIN(wn, hn), mipLevels=0; i; i>>=1, mipLevels++);
      mipLevels--;
    }
    else
      mipLevels=1;

    if (!(output.caps.TextureCaps & D3DPTEXTURECAPS_MIPMAP))
      mipLevels=1;
    
    usage = (flags & texfRTT)     ? D3DUSAGE_RENDERTARGET   : 0;
    pool  = (flags & texfRTT)     ? D3DPOOL_DEFAULT         : D3DPOOL_MANAGED;

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
    
    // textur erzeugen
    
#if !defined(FVS_RGBA_TEXTURES_ONLY) || !defined(FVS_TINY_TEXTURE_CODE)
    checkError(d3ddev->CreateTexture(wn, hn, mipLevels, usage, format->fmt, pool, &tex, 0), "CreateTexture FAILED");
#else
		checkError(d3ddev->CreateTexture(wn, hn, mipLevels, usage, D3DFMT_A8R8G8B8, pool, &tex, 0), "CreateTexture FAILED");
#endif

    // bei animierten texturen ausserdem den update buffer allocen

    if (flags & texfAnimated)
    {
      IDirect3DTexture9 *a1, *a2;

      checkError(d3ddev->CreateTexture(wn, hn, mipLevels, usage, format->fmt, D3DPOOL_DEFAULT, &a1, 0), "CreateTexture FAILED for anim buffer 1");
      this->addResIDs[0]=resourceMgr->add(a1);

      if (!(flags & texfNoLag)) // gebufferte texturen brauchen noch einen zusatzbuffer
      {
        checkError(d3ddev->CreateTexture(wn, hn, mipLevels, usage, format->fmt, D3DPOOL_DEFAULT, &a2, 0), "CreateTexture FAILED for anim buffer 2");
        this->addResIDs[1]=resourceMgr->add(a2);
      }

      iAnimatedTexture *te=new iAnimatedTexture;
      te->tex=this;
      te->next=animTextures;
      animTextures=te;
    }

    this->w=w;
    this->h=h;
    this->rw=wn;
    this->rh=hn;
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
    this->createFlags=flags;
    this->resId=resourceMgr->add(tex);
    this->locked=sFALSE;
    this->created=sTRUE;

#ifndef FVS_MINIMAL_DEBUG_OUTPUT
    fr::debugOut("TEXX: added texture: width=%d/%d, height: %d/%d, %d miplevels, flags: %s, rid #%08x\n",
      w, rw, h, rh, mipLevels, formatFlags(flags), resId);
#endif
  }

  void texture::destroy()
  {
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

    resourceMgr->remove(resId);
    created=sFALSE;
  }

  sBool texture::lock(sU32 level, D3DLOCKED_RECT *lr, const RECT *rect)
  {
    IDirect3DTexture9 *tex;

    FRDASSERT(created && !locked);

    tex=(IDirect3DTexture9 *) resourceMgr->getPtr(resId);
    locked=tex?(tex->LockRect(level, lr, rect, 0)==D3D_OK):sFALSE;

    return locked;
  }

  void texture::unlock(sU32 level)
  {
    IDirect3DTexture9 *tex;

    FRDASSERT(created && locked);

    tex=(IDirect3DTexture9 *) resourceMgr->getPtr(resId);
    tex->UnlockRect(level);
    locked=sFALSE;
  }

  sBool texture::getDesc(sU32 level, D3DSURFACE_DESC *dsc)
  {
    IDirect3DTexture9 *tex;
    
    FRDASSERT(created);
    
    tex=(IDirect3DTexture9 *) resourceMgr->getPtr(resId);
    return (tex->GetLevelDesc(level, dsc)==D3D_OK);
  }

  IDirect3DSurface9 *texture::getSurfaceLevel(sU32 level)
  {
    IDirect3DTexture9 *tex;
    IDirect3DSurface9 *srf;

    FRDASSERT(created);

    tex=(IDirect3DTexture9 *) resourceMgr->getPtr(resId);
    return (tex->GetSurfaceLevel(level, &srf)==D3D_OK)?srf:0;
  }

  void texture::upload(const sU8 *img)
  {
    D3DSURFACE_DESC desc;
    D3DLOCKED_RECT  rect;
    sU32            i, y;
    sU8             *src, *dst, *tmp;

    FRDASSERT(created && !locked);

    if (img)
    {
#ifndef FVS_RGBA_TEXTURES_ONLY
      tmp=new sU8[rw*rh*formatSize[createFlags&15]];
      
      if (w==rw && h==rh)
        memcpy(tmp, img, rw*rh*formatSize[createFlags&15]);
#else
      tmp=new sU8[rw*rh*4];
      
      if (w==rw && h==rh)
        memcpy(tmp, img, rw*rh*4);
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
      for (i=0; i<mipLevels; i++)
      {
        if (!lock(i, &rect))
          break;

        getDesc(i, &desc);
        
        if (tmp)
        {
          src=tmp;
          dst=(sU8 *) rect.pBits;

#if !defined(FVS_RGBA_TEXTURES_ONLY) || !defined(FVS_TINY_TEXTURE_CODE)
          asl=format->asl; asr=format->asr;
          rsl=format->rsl; rsr=format->rsr;
          gsl=format->gsl; gsr=format->gsr;
          bsl=format->bsl; bsr=format->bsr;
          
          for (y=0; y<desc.Height; y++)
          {
            format->func(dst, src, desc.Width, format->bits);

#ifndef FVS_RGBA_TEXTURES_ONLY
            src+=formatSize[createFlags&15]*desc.Width;
#else
            src+=4*desc.Width;
#endif
            dst+=rect.Pitch;
          }
#else
					for (y = 0; y < desc.Height; y++)
					{
						memcpy(dst, src, desc.Width * 4);
						src += 4 * desc.Width;
						dst += rect.Pitch;
					}
#endif

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
    else
      fr::errorExit("Cannot directly upload images to VRAM textures");

    // textur animiert?
    if (animFlags)
    {
      if (animFlags==1) // nolag, kopie im videoram updaten
        checkError(d3ddev->UpdateTexture((IDirect3DBaseTexture9 *) resourceMgr->getPtr(resId),
          (IDirect3DBaseTexture9 *) resourceMgr->getPtr(addResIDs[0])), "TEXX: UpdateTexture FAILED", sFALSE);
      else if (animFlags==2) // lagging, gerade nicht dargestellten buffer updaten
      {
        checkError(d3ddev->UpdateTexture((IDirect3DBaseTexture9 *) resourceMgr->getPtr(resId),
          (IDirect3DBaseTexture9 *) resourceMgr->getPtr(addResIDs[animDisp^1])), "TEXX: UpdateTexture FAILED", sFALSE);

        this->animChange=sTRUE;
      }
    }

    FRSAFEDELETEA(tmp);
  }

  void texture::translate(sF32 *u, sF32 *v) const
  {
    if (this)
    {
      FRDASSERT(created);

      if (u)  *u=*u*usc+uof;
      if (v)  *v=*v*vsc+vof;
    }
  }

  void texture::translatePix(sF32 *u, sF32 *v) const
  {
    if (this)
    {
      FRDASSERT(created);

      if (u)  *u=*u*usc/(sF32) w+uof;
      if (v)  *v=*v*vsc/(sF32) h+vof;
    }
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

  cubeTexture::operator IDirect3DCubeTexture9 *() const
  {
    if (this)
    {
      FRDASSERT(created);
      
      return (IDirect3DCubeTexture9 *) resourceMgr->getPtr(getResID());
    }
    else
      return 0;
  }

  void cubeTexture::create(sU32 edge, sU32 flags)
  {
    IDirect3DCubeTexture8 *tex;
    sU32                  usage;
    sU32                  en, i;

    FRASSERT(!(flags & texfAnimated));

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

#ifndef FVS_MINIMAL_DEBUG_OUTPUT
    fr::debugOut("TEXX: added cubemap: edge=%d, %d miplevels, flags: %s, rid #%08x\n",
      edge, mipLevels, formatFlags(flags), resId);
#endif
  }
  
  void cubeTexture::destroy()
  {
    resourceMgr->remove(resId);
    created=sFALSE;
  }
  
  sBool cubeTexture::lock(sU32 face, sU32 level, D3DLOCKED_RECT *lr, const RECT *rect)
  {
    
    
    FRDASSERT(created && !locked);
    FRDASSERT(face>=0 && face<=5);
    
    IDirect3DCubeTexture9 *tex = (IDirect3DCubeTexture9 *) resourceMgr->getPtr(resId);
    locked=tex?(tex->LockRect((D3DCUBEMAP_FACES) face, level, lr, rect, 0)==D3D_OK):sFALSE;
    
    return locked;
  }
  
  void cubeTexture::unlock(sU32 face, sU32 level)
  {
    FRDASSERT(created && locked);
    
		IDirect3DCubeTexture9 *tex = (IDirect3DCubeTexture9 *) resourceMgr->getPtr(resId);
    tex->UnlockRect((D3DCUBEMAP_FACES) face, level);
    locked=sFALSE;
  }
  
  sBool cubeTexture::getDesc(sU32 level, D3DSURFACE_DESC *dsc)
  {
    FRDASSERT(created);
    
		IDirect3DCubeTexture9* tex = (IDirect3DCubeTexture9 *) resourceMgr->getPtr(resId);
    return (tex->GetLevelDesc(level, dsc)==D3D_OK);
  }
  
  IDirect3DSurface9 *cubeTexture::getSurfaceLevel(sU32 face, sU32 level)
  {
    IDirect3DCubeTexture9 *tex;
    IDirect3DSurface9 *srf;
    
    FRDASSERT(created);
    FRDASSERT(face>=0 && face<=5);
    
    tex=(IDirect3DCubeTexture9 *) resourceMgr->getPtr(resId);
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

  // ---------------------------------------------------------------------------------------------
  // highlevel vertex/index buffer handling
  // ---------------------------------------------------------------------------------------------

  namespace
  {
    static const sInt  staticVBSize=16000;
    static const sInt  staticIBSize=16000;
    static const sInt  dynamicVBSize=65535; // 12k
    static const sInt  dynamicIBSize=65535*3; // 32k

    class iRangeManager // non-stupid version, third attempt
    {
#ifndef FVS_STUPID_BUFFER_MANAGEMENT
    public:
      struct block
      {
        block(sU32 start, sU32 end, block *next=0)
          : m_start(start), m_end(end), m_next(next)
        {
        }

        sU32    m_start, m_end;
        block   *m_next;
      };

      sU32    size;
      block   *blockList;

      iRangeManager(sU32 sz)
      {
        size = sz;
        blockList = new block(0, sz | 0x80000000);
      }

      ~iRangeManager()
      {
        killBlockList();
      }

      void add(sU32 start, sU32 len) // find corresponding block & split
      {
        for (block *t = blockList; t; t = t->m_next)
        {
          FRDASSERT(t->m_start <= start);

          if (t->m_start == start) // we found the block!
          {
            FRDASSERT(t->m_end & 0x80000000);         // assert free
            t->m_end &= 0x7fffffff;                   // tag as allocated
            FRDASSERT(t->m_end - t->m_start >= len);  // assert enough space

            const sU32 newEnd = t->m_start + len;

            if (newEnd < t->m_end) // we have to split, create new free block & link it in
            {
              t->m_next = new block(newEnd, t->m_end | 0x80000000, t->m_next);
              t->m_end = newEnd;
            }

            break; // end search.
          }
        }
      }

      void remove(sU32 start, sU32 len) // free allocated block
      {
        for (block *t = blockList; t; t = t->m_next)
        {
          FRDASSERT(t->m_start <= start);

          if (t->m_start == start) // we found the block!
          {
            FRDASSERT(t->m_end == start + len); // assert correct size (& allocated)
            t->m_end |= 0x80000000;             // tag as free

            block *tNext = t->m_next;
            if (tNext && (tNext->m_end & 0x80000000)) // next block also free? then coalesce the two.
            {
              t->m_end = tNext->m_end;      // 1. merge ranges
              t->m_next = tNext->m_next;    // 2. unlink tNext
              delete tNext;                 // 3. delete tNext
            }

            break;
          }
        }
      }

      sInt findSpace(sU32 len) const
      {
        // this goes through the free list and tries to find the most suitable free memory block.
        // deciding criterion is closest match in size.

        sU32 bestSizeMatch((sU32) -1);
        block *bestMatch = 0;

        for (block *t = blockList; t; t = t->m_next)
        {
          if (t->m_end & 0x80000000) // only check free blocks
          {
            sU32 blockSize = (t->m_end & 0x7fffffff) - t->m_start;

            if (blockSize >= len && (blockSize - len) < bestSizeMatch)
            {
              bestSizeMatch = blockSize - len;
              bestMatch = t;
            }
          }
        }

        if (bestMatch)
          return bestMatch->m_start;
        else
          return -1;
      }

      void clear()
      {
        killBlockList();
        blockList=new block(0, size | 0x80000000);
      }

      sBool isEmpty() const
      {
        return blockList && blockList->m_start == 0 && blockList->m_end == (size | 0x80000000);
      }
    
    private:
      void killBlockList()
      {
        for (block *t = blockList, *t2 = 0; t; t = t2)
        {
          t2 = t->m_next;
          delete t;
        }

        blockList = 0;
      }

#else // the stupid version for 64ks etc :)
    public:
      sU32      size, pos;
      
      iRangeManager(sU32 sz)
        : size(sz), pos(0)
      {
      }

      void add(sU32 start, sU32 len)
      {
        if (start+len>pos)
          pos=start+len;
      }

      void remove(sU32 start, sU32 len)
      {
        if (pos==start+len)
          pos=start;
      }

      sInt findSpace(sU32 len) const
      {
        if (pos+len<size)
          return pos;
        else
          return -1;
      }

      sBool isEmpty() const
      {
        return pos == 0;
      }

      void clear()
      {
        pos=0;
      }
#endif
    };
  }

  struct vbSlice::iVertexBuffer
  {
    void          *buf;
    iRangeManager usage;
    sU32          FVF;
    sInt          streamInd;
    sInt          pos, size;
    sBool         priv;
    sU32          stride;
    iVertexBuffer *next, *prev;
    
    iVertexBuffer(void *vb, sU32 VF, sInt streamNdx, sU32 sz, sU32 strd)
      : buf (vb), usage(sz), FVF(VF), streamInd(streamNdx), size(sz), pos(0), next(0), prev(0),
      priv(sFALSE), stride(strd)
    {
    }
    
    ~iVertexBuffer()
    {
      if (buf)
        resourceMgr->remove(buf);
    }
  };

  struct ibSlice::iIndexBuffer
  {
    void           *buf;
    iRangeManager  usage;
    sInt           pos, size;
    iIndexBuffer   *next, *prev;

    iIndexBuffer()
      : buf(0), usage(dynamicIBSize), size(dynamicIBSize), pos(0), next(0), prev(0)
    {
    }

    iIndexBuffer(void *ib, sU32 sz)            
      : buf (ib), usage(sz), size(sz), pos(0), next(0), prev(0)
    {
    }

    ~iIndexBuffer()
    {
      if (buf)
        resourceMgr->remove(buf);
    }
  };

  vbSlice::iVertexBuffer *vbSlice::staticVertBuffers=0;
  vbSlice::iVertexBuffer *vbSlice::dynamicVertBuffers=0;

  ibSlice::iIndexBuffer *ibSlice::staticIndBuffers=0;
  ibSlice::iIndexBuffer *ibSlice::dynamicIndBuffer=0;

  // ---- fvsVBSlice

  vbSlice::vbSlice()
  {
    created=sFALSE;
    locked=sFALSE;
    ivb=0;
  }

  vbSlice::vbSlice(sInt nVerts, sBool dynamic, sU32 VF, sInt streamInd)
  {
    created=sFALSE;
    locked=sFALSE;
    ivb=0;

    create(nVerts, dynamic, VF, streamInd);
  }

  vbSlice::~vbSlice()
  {
    destroy();
  }

  void vbSlice::globalCleanup()
  {
    iVertexBuffer *vt1, *vt2;

    for (vt1=staticVertBuffers; vt1; vt1=vt2)
    {
      vt2=vt1->next;
      delete vt1;
    }

    for (vt1=dynamicVertBuffers; vt1; vt1=vt2)
    {
      vt2=vt1->next;
      delete vt1;
    }

    staticVertBuffers=0;
    dynamicVertBuffers=0;
  }

  void vbSlice::create(sInt nVerts, sBool isDynamic, sU32 VF, sInt streamInd)
  {
    iVertexBuffer *it;
    sInt          pos;
    sU32          stride=getFVFSize(VF);

    FRDASSERT(!created);

    if (!isDynamic)
    {
      if (nVerts<staticVBSize)
      {
        for (it=staticVertBuffers; it; it=it->next)
        {
          if (it->FVF==VF && it->streamInd==streamInd && ((pos=it->usage.findSpace(nVerts))!=-1))
          {
            created=sTRUE;
            dynamic=sFALSE;
            FVF=VF;
            nVertices=nVerts;
            vBase=pos;
            ivb=it;
            streamIndex=streamInd;

            it->usage.add(pos, nVerts);
            return;
          }
        }
      }

      sU32            sz=MAX(nVerts, staticVBSize);
      IDirect3DVertexBuffer9 *vb;

      checkError(d3ddev->CreateVertexBuffer(sz*stride, D3DUSAGE_WRITEONLY, VF,
        D3DPOOL_MANAGED, &vb, 0), "CreateVertexBuffer FAILED");

      ivb=new iVertexBuffer(resourceMgr->add(vb), VF, streamInd, sz, stride);

      created=sTRUE;
      dynamic=sFALSE;
      FVF=VF;
      nVertices=nVerts;
      vBase=0;
      streamIndex=streamInd;

      ivb->usage.add(0, nVerts);
      ivb->next=staticVertBuffers;
      if (staticVertBuffers)
        staticVertBuffers->prev=ivb;

      staticVertBuffers=ivb;
    }
    else
    {
      if (nVerts<=dynamicVBSize)
      {
        for (it=dynamicVertBuffers; it; it=it->next)
        {
          if (it->FVF==VF && it->streamInd==streamInd && !it->priv)
          {
            created=sTRUE;
            dynamic=sTRUE;
            FVF=VF;
            nVertices=nVerts;
            vBase=-1;
            streamIndex=streamInd;
            ivb=&(*it);

            return;
          }
        }

        IDirect3DVertexBuffer9 *vb;
        
        checkError(d3ddev->CreateVertexBuffer(dynamicVBSize*stride, D3DUSAGE_WRITEONLY|D3DUSAGE_DYNAMIC, VF,
          D3DPOOL_DEFAULT, &vb, 0), "CreateVertexBuffer FAILED");

        ivb=new iVertexBuffer(resourceMgr->add(vb), VF, streamInd, dynamicVBSize, stride);
        
        created=sTRUE;
        dynamic=sTRUE;
        FVF=VF;
        nVertices=nVerts;
        vBase=-1;
        streamIndex=streamInd;

        ivb->next=dynamicVertBuffers;
        dynamicVertBuffers=ivb;
      }
      else
      {
        IDirect3DVertexBuffer9 *vb;
        
        checkError(d3ddev->CreateVertexBuffer(nVerts*stride, D3DUSAGE_WRITEONLY|D3DUSAGE_DYNAMIC, VF,
          D3DPOOL_DEFAULT, &vb, 0), "CreateVertexBuffer FAILED");
        
        ivb=new iVertexBuffer(resourceMgr->add(vb), VF, streamInd, nVerts, stride);
        ivb->priv=sTRUE;

        created=sTRUE;
        dynamic=sTRUE;
        FVF=VF;
        nVertices=nVerts;
        vBase=-1;
        streamIndex=streamInd;

        ivb->next=dynamicVertBuffers;
        dynamicVertBuffers=ivb;
      }
    }
  }

  void vbSlice::destroy()
  {
    FRDASSERT(created && !locked);

    if (!dynamic)
    {
      ivb->usage.remove(vBase, nVertices);
      if (ivb->usage.isEmpty()) // destroy vertex buffer if it isn't occupied anymore
      {
        iVertexBuffer *iv=ivb;

        // unlink & delete it
        if (ivb->prev)
          ivb->prev->next=ivb->next;
        else
          staticVertBuffers=ivb->next;

        if (ivb->next)
          ivb->next->prev=ivb->prev;

        delete iv;
      }
    }

    created=sFALSE;
  }

  void *vbSlice::lock(sInt lockStart, sInt lockSize)
  {
    sU32 lockMode;

    if (lockSize==-1)
      lockSize=nVertices;

    FRDASSERT(created && !locked);
    FRDASSERT(!dynamic || (lockStart==0 && lockSize==nVertices));

    if (dynamic)
    {
      if (ivb->pos+lockSize<ivb->size)
      {
        vBase=ivb->pos;
        lockMode=D3DLOCK_NOOVERWRITE;
      }
      else
      {
        vBase=0;
        lockMode=D3DLOCK_DISCARD;
      }

      ivb->pos=vBase+lockSize;
    }
    else
      lockMode=0;
    
    locked=sTRUE;
    
    IDirect3DVertexBuffer9 *vb=(IDirect3DVertexBuffer9 *) resourceMgr->getPtr(ivb->buf);

    void *buf;
    checkError(vb->Lock((vBase+lockStart)*ivb->stride, lockSize*ivb->stride, &buf, lockMode),
      "VB Lock failed", sFALSE);

    return (void *) buf;
  }

  void vbSlice::unlock()
  {
    FRDASSERT(created && locked);

    IDirect3DVertexBuffer9 *vb=(IDirect3DVertexBuffer9 *) resourceMgr->getPtr(ivb->buf);
    vb->Unlock();

    locked=sFALSE;
  }

  void vbSlice::resize(sInt size)
  {
    FRDASSERT(created && !locked);
    FRDASSERT(dynamic && nVertices<=dynamicVBSize && size<=dynamicVBSize);

    nVertices=size;
  }

  sU32 vbSlice::use() const
  {
    FRDASSERT(created && !locked);

    setVertexShader(FVF);
    setVertexBuffer(ivb->streamInd, (IDirect3DVertexBuffer9 *) resourceMgr->getPtr(ivb->buf), ivb->stride);

    return vBase;
  }

  // ---- fvsIBSlice

  ibSlice::ibSlice()
  {
    if (!dynamicIndBuffer)
      dynamicIndBuffer=new iIndexBuffer;

    created=sFALSE;
    locked=sFALSE;
  }

  ibSlice::ibSlice(sInt nInds, sBool dynamic)
  {
    if (!dynamicIndBuffer)
      dynamicIndBuffer=new iIndexBuffer;
    
    created=sFALSE;
    locked=sFALSE;

    create(nInds, dynamic);
  }

  ibSlice::~ibSlice()
  {
    destroy();
  }

  void ibSlice::globalCleanup()
  {
    for (iIndexBuffer *it1=staticIndBuffers, *it2=0; it1; it1=it2)
    {
      it2=it1->next;
      delete it1;
    }

    staticIndBuffers=0;

    FRSAFEDELETE(dynamicIndBuffer);
  }

  void ibSlice::create(sInt nInds, sBool isDynamic)
  {
    iIndexBuffer  *it;
    sInt          pos;

    FRDASSERT(!created);
    FRDASSERT(!isDynamic || nInds<=dynamicIBSize);

    if (!isDynamic)
    {
      if (nInds<staticIBSize)
      {
        for (it=staticIndBuffers; it; it=it->next)
        {
          if ((pos=it->usage.findSpace(nInds))!=-1)
          {
            created=sTRUE;
            dynamic=sFALSE;
            nIndices=nInds;
            iBase=pos;
            iib=it;

            it->usage.add(pos, nInds);
            return;
          }
        }
      }

      sU32            sz=MAX(nInds, staticIBSize);
      IDirect3DIndexBuffer9 *ib;

      checkError(d3ddev->CreateIndexBuffer(sz*2, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16,
        D3DPOOL_MANAGED, &ib, 0), "CreateIndexBuffer FAILED");

      iib=new iIndexBuffer(resourceMgr->add(ib), sz);

      created=sTRUE;
      dynamic=sFALSE;
      nIndices=nInds;
      iBase=0;

      iib->usage.add(0, nInds);
      iib->next=staticIndBuffers;
      if (staticIndBuffers)
        staticIndBuffers->prev=iib;

      staticIndBuffers=iib;
    }
    else
    {
      if (dynamicIndBuffer->buf)
      {
        created=sTRUE;
        dynamic=sTRUE;
        nIndices=nInds;
        iBase=-1;
        iib=dynamicIndBuffer;
        return;
      }

      IDirect3DIndexBuffer9 *ib;
      
      checkError(d3ddev->CreateIndexBuffer(dynamicIBSize*2, D3DUSAGE_WRITEONLY|D3DUSAGE_DYNAMIC, D3DFMT_INDEX16,
        D3DPOOL_DEFAULT, &ib, 0), "CreateIndexBuffer FAILED");

      dynamicIndBuffer->buf=resourceMgr->add(ib);

      created=sTRUE;
      dynamic=sTRUE;
      nIndices=nInds;
      iBase=-1;

      iib=dynamicIndBuffer;
    }
  }

  void ibSlice::destroy()
  {
    FRDASSERT(created && !locked);

    if (!dynamic)
    {
      iib->usage.remove(iBase, nIndices);

      if (iib->usage.isEmpty())
      {
        iIndexBuffer *ib=iib;

        // unlink & delete index buffer if empty
        if (iib->prev)
          iib->prev->next=iib->next;
        else
          staticIndBuffers=iib->next;

        if (iib->next)
          iib->next->prev=iib->prev;

        delete ib;
      }
    }

    created=sFALSE;
  }

  void *ibSlice::lock(sInt lockStart, sInt lockSize)
  {
    sU32 lockMode;

    if (lockSize==-1)
      lockSize=nIndices;

    FRDASSERT(created && !locked);
    FRDASSERT(!dynamic || (lockStart==0 && lockSize==nIndices));

    if (dynamic)
    {
      if (iib->pos+lockSize<iib->size)
      {
        iBase=iib->pos;
        lockMode=D3DLOCK_NOOVERWRITE;
      }
      else
      {
        iBase=0;
        lockMode=D3DLOCK_DISCARD;
      }
      
      iib->pos=iBase+lockSize;
    }
    else
      lockMode=0;
    
    locked=sTRUE;

    IDirect3DIndexBuffer9 *ib=(IDirect3DIndexBuffer9 *) resourceMgr->getPtr(iib->buf);
    
    void *data;
    checkError(ib->Lock((iBase+lockStart)*2, lockSize*2, &data, lockMode), "IB Lock FAILED", sFALSE);
    
    return (void *) data;
  }
  
  void ibSlice::unlock()
  {
    FRDASSERT(created && locked);
    
    IDirect3DIndexBuffer9 *ib=(IDirect3DIndexBuffer9 *) resourceMgr->getPtr(iib->buf);
    ib->Unlock();
    locked=sFALSE;
  }
  
  void ibSlice::resize(sInt size)
  {
    FRDASSERT(created && !locked);
    FRDASSERT(dynamic && size<=dynamicIBSize);
    
    nIndices=size;
  }
  
  sU32 ibSlice::use(sU32 startv) const
  {
    FRDASSERT(created && !locked);

    setIndexBuffer((IDirect3DIndexBuffer9 *) resourceMgr->getPtr(iib->buf), startv);
    
    return iBase;
  }
}



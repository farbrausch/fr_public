/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#define sPEDANTIC_OBSOLETE 1
//#define sPEDANTIC_WARN 1

#include "base/graphics.hpp"
#include "base/system.hpp"
#include "util/shaders.hpp"
#include "util/ipp.hpp"

extern sInt sExitFlag;

/****************************************************************************/

sRenderTargetManager_::sRenderTargetManager_()
{
  ScreenProxy = 0;
  ScreenProxyDirty = 0;
  ScreenX = 0;
  ScreenY = 0;
  ToTexture = 0;
  GeoDouble = new sGeometry(sGF_QUADLIST,sVertexFormatDouble);
  Refs.HintSize(4);
}

sRenderTargetManager_::~sRenderTargetManager_()
{
  delete GeoDouble;
  Flush();
}

sRenderTargetManager_::Target *sRenderTargetManager_::Find(sTextureBase *tex)
{
  Target *t;

  sFORALL(Targets,t)
    if(t->Texture == tex)
      return t;

  return 0;
}


sTexture2D *sRenderTargetManager_::Acquire(sInt x,sInt y,sInt format)
{
  Target *t;
  format = (format & sTEX_FORMAT) | sTEX_2D | sTEX_RENDERTARGET;
  sFORALL(Targets,t)
  {
    if(t->RefCount==0 && t->SizeX==x && t->SizeY==y && t->Format==format && t->Proxy==0)
    {
      t->RefCount = 1;
      return (sTexture2D *)t->Texture;
    }
  }
  t = Targets.AddMany(1);
  t->SizeX = x;
  t->SizeY = y;
  t->Format = format;
  t->RefCount = 1;
  t->Proxy = 0;
  t->Texture = new sTexture2D(x,y,format,1);
  sLogF(L"RTM",L"create rendertarget %dx%d %08x\n",x,y,format);
  return (sTexture2D *)t->Texture;
}

sTextureBase *sRenderTargetManager_::AcquireProxy(sTexture2D *tex)
{
  Target *t;
  sFORALL(Targets,t)
  {
    if(t->RefCount==0 && t->Proxy==1)
    {
      t->RefCount = 1;
      ((sTextureProxy*)(t->Texture))->Connect(tex);
      return (sTexture2D *)t->Texture;
    }
  }
  t = Targets.AddMany(1);
  t->Proxy = 1;
  t->RefCount = 1;
  t->Texture = new sTextureProxy();
  ((sTextureProxy*)(t->Texture))->Connect(tex);
  return t->Texture;
}

sTextureCube *sRenderTargetManager_::AcquireCube(sInt edge,sInt format)
{
  Target *t;
  format = (format & sTEX_FORMAT) | sTEX_CUBE | sTEX_RENDERTARGET;
  sFORALL(Targets,t)
  {
    if(t->RefCount==0 && t->SizeX==edge && t->Format==format)
    {
      t->RefCount = 1;
      return (sTextureCube *)t->Texture;
    }
  }
  t = Targets.AddMany(1);
  t->SizeX = edge;
  t->SizeY = 0;
  t->Format = format;
  t->RefCount = 1;
  t->Texture = new sTextureCube(edge,format,1);
  sLogF(L"wz4",L"create cube rendertarget edge=%d %08x\n",edge,format);
  return (sTextureCube *)t->Texture;
}

void sRenderTargetManager_::AddRef(sTextureBase *tex)
{
  if(tex)
  {
    Target *t = Find(tex);
    if(t)
      t->RefCount++;
    else
      sFatal(L"sRenderTargetManager_: texture %08x not aquired\n",sDInt(tex));
  }
}

void sRenderTargetManager_::Release(sTextureBase *tex)
{
  if(tex)
  {
    Target *t = Find(tex);
    if(t)
      t->RefCount--;
    else
      sFatal(L"sRenderTargetManager_: texture %08x not aquired\n",sDInt(tex));
  }
}

void sRenderTargetManager_::Flush()
{
  Target *t;
  sFORALL(Targets,t)
  {
    if (t->RefCount != 0)
    {
      if (sExitFlag)
        sLogF(L"RTM", L"Flush: texture->RefCount != 0 released\n");  // ignore this on shutdown, we don't use the texture anymore (emergency shutdown may happen in a discipline while a render target is in use)
      else
        sVERIFY(t->RefCount==0)
    }

    delete t->Texture;
  }
  Targets.Clear();
  Refs.Clear();

  // reset to avoid memleaks
  Targets.Reset();
  Refs.Reset();
}

void sRenderTargetManager_::ResolutionCheck(void *ref,sInt xs,sInt ys)
{
  Reference *r = sFind(Refs,&Reference::Ref,ref);
  if(r)
  {
    if(r->SizeX!=xs || r->SizeY!=ys)
    {
      Flush();
      sLogF(L"wz4",L"flush rendertargets\n");
      r = 0;
    }
  }
  if(!r)
  {
    r = Refs.AddMany(1);
    r->Ref = ref;
    r->SizeX = xs;
    r->SizeY = ys;
  }
}

/****************************************************************************/

void sRenderTargetManager_::SetScreen(const sRect *window)
{
  Window = window;
  ScreenProxy = 0;
  ScreenProxyDirty = 0;
  Release(ToTexture);
  ToTexture = 0;
  
  if(window)
  {
    ScreenX = window->SizeX();
    ScreenY = window->SizeY();
  }
  else
  {
    sGetScreenSize(ScreenX,ScreenY);
  }
}

void sRenderTargetManager_::SetScreen(const sTargetSpec &spec)
{
  ScreenProxy = 0;
  ScreenProxyDirty = 0;
  Release(ToTexture);
  ToTexture = 0;
  if(spec.Color2D!=sGetScreenColorBuffer())
  {
    Target *t;
    ToTexture = 0;
    sFORALL(Targets,t)
    {
      if(t->Texture==spec.Color2D)
      {
        ToTexture = spec.Color2D;
        AddRef(ToTexture);
      }
    }
    if(ToTexture==0)
      ToTexture = (sTexture2D *)AcquireProxy(spec.Color2D);
  }

  if(spec.Window.IsEmpty())
  {
    Window = 0;
    sInt z;
    spec.Color->GetSize(ScreenX,ScreenY,z);
  }
  else
  {
    Window = &spec.Window;
    ScreenX = Window->SizeX();
    ScreenY = Window->SizeY();
  }
}

sTexture2D *sRenderTargetManager_::ReadScreen()
{
  if(!ScreenProxy)
  {
    if(ToTexture)
    {
      AddRef(ToTexture);    // this will silently fail if the texture is not managed by sRTMan. good.
      return ToTexture;
    }
    ScreenProxy = Acquire(ScreenX,ScreenY);
    sRect dr(0,0,ScreenX,ScreenY);

    sCopyTexturePara ctp;
    ctp.Source = sGetScreenColorBuffer();
    ctp.SourceRect = Window ? *Window : dr;
    ctp.Dest = ScreenProxy;
    ctp.DestRect = dr;
    ctp.Flags = 0;
    sCopyTexture(ctp);
  }
  AddRef(ScreenProxy);
  return ScreenProxy;
}

sTexture2D *sRenderTargetManager_::WriteScreen(sBool finish)
{
  if(ToTexture && finish)
  {
    AddRef(ToTexture);
    return ToTexture;
  }

  if(ScreenProxy)
  {
    Release(ScreenProxy);
    ScreenProxy = 0;
    ScreenProxyDirty = 0;
  }
  if(!finish)
  {
    ScreenProxy = Acquire(ScreenX,ScreenY);
    AddRef(ScreenProxy);
    ScreenProxyDirty = 1;
  }
  return ScreenProxy;
}

void sRenderTargetManager_::SetTarget(sTexture2D *tex,sInt clrflags,sU32 clrcol,sTexture2D *dep)
{
  if(tex)
    sSetTarget(sTargetPara((clrflags&3)|sST_NOMSAA,clrcol,0,tex,dep));
  else
    sSetTarget(sTargetPara((clrflags&3)|sST_NOMSAA,clrcol,Window,sGetScreenColorBuffer(),dep));
}

void sRenderTargetManager_::FinishScreen()
{
#if sPLATFORM==sPLAT_WINDOWS  // Until sSetScreen is available on all platforms
  if(ScreenProxy && ScreenProxyDirty)
  {
    const sRect sr(0,0,ScreenX,ScreenY);
    sInt z;
    sCopyTexturePara ctp;
    ctp.Source = ScreenProxy;
    ctp.SourceRect = sr;
    ctp.Dest = ToTexture ? ToTexture : sGetScreenColorBuffer();
    if(ScreenProxy)
      ctp.DestRect = *Window;
    else
      ctp.Dest->GetSize(ctp.DestRect.x1,ctp.DestRect.y1,z);

    sCopyTexture(ctp);
//    sSetScreen(ScreenProxy,sGFF_NONE,Window,);
    Release(ScreenProxy);
    ScreenProxy = 0;
    ScreenProxyDirty = 0;
  }
  Release(ToTexture);
  ToTexture = 0;
#endif
}

/****************************************************************************/

sRenderTargetManager_ *sRTMan;
void sInitRenderTargetManager() { sRTMan = new sRenderTargetManager_; }
void sExitRenderTargetManager() { delete sRTMan; sRTMan = 0; }
sADDSUBSYSTEM(sRTMan,0xc0,sInitRenderTargetManager,sExitRenderTargetManager);

/****************************************************************************/


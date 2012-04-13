// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#include "_start.hpp"
#include "rtmanager.hpp"

/****************************************************************************/

void RenderTargetManager_::GetRealResolution(const Target *tgt,sInt &xRes,sInt &yRes) const
{
  xRes = tgt->XRes ? tgt->XRes : MasterVP.Window.XSize();
  yRes = tgt->YRes ? tgt->YRes : MasterVP.Window.YSize();
}

void RenderTargetManager_::ResizeInternal(Target *tgt,sInt xRes,sInt yRes)
{
  sInt useXRes,useYRes;

  tgt->XRes = xRes;
  tgt->YRes = yRes;
  GetRealResolution(tgt,useXRes,useYRes);

  sInt texXRes = sMin(sMakePower2(useXRes),2048);
  sInt texYRes = sMin(sMakePower2(useYRes),2048);

  // re-allocate texture if necessary
  if(texXRes > tgt->TexXRes || texYRes > tgt->TexYRes)
  {
    if(tgt->Handle != sINVALID)
      sSystem->RemTexture(tgt->Handle);

    tgt->TexXRes = texXRes;
    tgt->TexYRes = texYRes;
    tgt->Handle = sSystem->AddTexture(tgt->TexXRes,tgt->TexYRes,sTF_A8R8G8B8,0);

#if !sINTRO
    sDPrintF("RTManager: target %08x resized to %dx%d\n",tgt->Id,tgt->TexXRes,tgt->TexYRes);
#endif
  }
}

RenderTargetManager_::RenderTargetManager_()
{
  Targets.Init();
  MasterVP.Init();
}

RenderTargetManager_::~RenderTargetManager_()
{
  for(sInt i=0;i<Targets.Count;i++)
    sSystem->RemTexture(Targets[i].Handle);

  Targets.Exit();
}

void RenderTargetManager_::GetMasterViewport(sViewport &vp)
{
  vp = MasterVP;
}

void RenderTargetManager_::SetMasterViewport(const sViewport &vp)
{
  MasterVP = vp;
  if(MasterVP.RenderTarget != -1 && vp.Window.XSize() == 0)
  {
    sInt w,h;

    sSystem->GetTextureSize(MasterVP.RenderTarget,w,h);
    MasterVP.Window.Init(0,0,w,h);
  }

  // go through all fullscreen targets, resizing them if necessary
  for(sInt i=0;i<Targets.Count;i++)
  {
    Target *tgt = &Targets[i];

    if(!tgt->XRes && !tgt->YRes)
      ResizeInternal(tgt,0,0);
  }
}

void RenderTargetManager_::Add(sU32 id,sInt xRes,sInt yRes)
{
  sF32 us,vs;
  if(Get(id,us,vs) != sINVALID) // only add if not yet added
    return;

  Target *tgt = Targets.Add();
  tgt->Id = id;
  tgt->TexXRes = 0;
  tgt->TexYRes = 0;
  tgt->Handle = sINVALID;
  ResizeInternal(tgt,xRes,yRes);
}

void RenderTargetManager_::Resize(sU32 id,sInt xRes,sInt yRes)
{
  for(sInt i=0;i<Targets.Count;i++)
  {
    Target *tgt = &Targets[i];

    if(tgt->Id == id)
      ResizeInternal(tgt,xRes,yRes);
  }
}

sInt RenderTargetManager_::Get(sU32 id,sF32 &uScale,sF32 &vScale) const
{
  for(sInt i=0;i<Targets.Count;i++)
  {
    const Target *tgt = &Targets[i];

    if(tgt->Id == id)
    {
      sInt useXRes,useYRes;
      GetRealResolution(tgt,useXRes,useYRes);

      if(useXRes <= tgt->TexXRes && useYRes <= tgt->TexYRes)
      {
        uScale = 1.0f * useXRes / tgt->TexXRes;
        vScale = 1.0f * useYRes / tgt->TexYRes;
      }
      else
        uScale = vScale = 1.0f;

      return tgt->Handle;
    }
  }

  return sINVALID;
}

void RenderTargetManager_::SetAsTarget(sU32 id) const
{
  for(sInt i=0;i<Targets.Count;i++)
  {
    const Target *tgt = &Targets[i];

    if(tgt->Id == id)
    {
      sViewport vp;

      vp.InitTex(tgt->Handle);
      vp.Window.x0 = 0;
      vp.Window.y0 = 0;
      GetRealResolution(tgt,vp.Window.x1,vp.Window.y1);

      if(vp.Window.x1 > tgt->TexXRes || vp.Window.y1 > tgt->TexYRes)
        vp.Window.Init(0,0,tgt->TexXRes,tgt->TexYRes);

      sSystem->SetViewport(vp);
    }
  }
}

void RenderTargetManager_::GrabToTarget(sU32 targetId,const sRect *window) const
{
  if(!window)
    window = &MasterVP.Window;

  for(sInt i=0;i<Targets.Count;i++)
  {
    const Target *tgt = &Targets[i];

    if(tgt->Id == targetId && tgt->Handle != sINVALID)
    {
      sInt useXRes,useYRes;
      GetRealResolution(tgt,useXRes,useYRes);

      sBool stretch = useXRes > tgt->TexXRes || useYRes > tgt->TexYRes;
      sSystem->GrabScreen(MasterVP.RenderTarget,*window,tgt->Handle,stretch);

      break;
    }
  }
}

/****************************************************************************/

RenderTargetManager_ *RenderTargetManager = 0;

/****************************************************************************/

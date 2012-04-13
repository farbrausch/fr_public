// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#include "engine.hpp"
#include "geneffectipp.hpp"
#include "genoverlay.hpp"
#include "materialdirect.hpp"
#include "rtmanager.hpp"

/****************************************************************************/

KObject * __stdcall Init_Effect_LineShuffle(sInt pass)
{
  GenEffect *fx = new GenEffect;
  fx->NeedCurrentRender = sTRUE;
  fx->Pass = pass;
  fx->Usage = ENGU_BASE;

  return fx;
}

void __stdcall Exec_Effect_LineShuffle(KOp *op,KEnvironment *kenv,sInt pass)
{
  sMaterialEnv env;
  sVertexStandard *vp;
  sF32 uScale,vScale;
  static sInt permute[1024];

  env.Init();
  env.Orthogonal = sMEO_PIXELS;
  env.CameraSpace.Init();
  env.ModelSpace.Init();
  sSystem->SetViewProject(&env);

  sInt tex = RenderTargetManager->Get(0x00000000,uScale,vScale);
  GenOverlayManager->Mtrl[GENOVER_TEX1]->SetTex(0,tex);
  GenOverlayManager->Mtrl[GENOVER_TEX1]->Color[0] = 0xffffffff;
  GenOverlayManager->Mtrl[GENOVER_TEX1]->Set(env);

  sInt lines = sSystem->ViewportY;
  sInt handle = sSystem->GeoAdd(sFVF_STANDARD,sGEO_LINE|sGEO_DYNAMIC);

  sSystem->GeoBegin(handle,lines*2,0,(sF32 **) &vp,0);
  for(sInt i=0;i<lines;i++)
    permute[i] = i;

  for(sInt i=0;i<lines;i++)
  {
    sInt j = sRange<sInt>(i + (sGetRnd(lines/32) - lines/64),lines-1,0);
    sSwap(permute[i],permute[j]);
  }

  for(sInt i=0;i<lines;i++)
  {
    sInt j = permute[i];
    sInt uShift = sGetRnd(lines/32) - lines/64;
    sF32 uOffs = uShift * uScale / lines;

    vp->x = 0;
    vp->y = i;
    vp->z = 0;
    vp->u = uOffs;
    vp->v = vScale * j / lines;
    vp++;

    vp->x = sSystem->ViewportX;
    vp->y = i;
    vp->z = 0;
    vp->u = uOffs + uScale;
    vp->v = vScale * j / lines;
    vp++;
  }

  sSystem->GeoEnd(handle);
  sSystem->GeoDraw(handle);
  sSystem->GeoRem(handle);
}

/****************************************************************************/

static void updateJPEGFilterKernel(sF32 strength,sInt mode)
{
  sU16 k0Data[16*4],k1Data[16*4];
  static const sF32 filters[4][4] = {
     1.12500f, -0.12500f,  0.12500f, -0.12500f,
     0.25000f,  1.25000f, -0.25000f, -0.25000f,
    -0.25000f, -0.25000f,  1.25000f,  0.25000f,
    -0.12500f,  0.12500f, -0.12500f,  1.12500f,
  };

  for(sInt i=0;i<16;i++)
  {
    sF32 buffer[7];

    sInt row = ((i/4) + i) & 3;

    for(sInt j=0;j<7;j++)
      buffer[j] = 0;

    for(sInt j=0;j<4;j++)
      buffer[j+3-row] = (row == j) + (filters[row][j] - (row == j)) * strength;

    k0Data[i*4+2] = sRange<sInt>(buffer[0] * 0x2000 + 0x4000,0x7fff,0);
    k0Data[i*4+1] = sRange<sInt>(buffer[1] * 0x2000 + 0x4000,0x7fff,0);
    k0Data[i*4+0] = sRange<sInt>(buffer[2] * 0x2000 + 0x4000,0x7fff,0);
    k0Data[i*4+3] = sRange<sInt>(buffer[3] * 0x2000 + 0x4000,0x7fff,0);
    k1Data[i*4+2] = sRange<sInt>(buffer[4] * 0x2000 + 0x4000,0x7fff,0);
    k1Data[i*4+1] = sRange<sInt>(buffer[5] * 0x2000 + 0x4000,0x7fff,0);
    k1Data[i*4+0] = sRange<sInt>(buffer[6] * 0x2000 + 0x4000,0x7fff,0);
    k1Data[i*4+3] = 0x4000;

    if(mode)
    {
      sSwap(k0Data[i*4+0],k0Data[i*4+1]);
      sSwap(k0Data[i*4+2],k0Data[i*4+3]);
    }
  }

  sSystem->UpdateTexture(GenOverlayManager->JPEGKernel0,k0Data);
  sSystem->UpdateTexture(GenOverlayManager->JPEGKernel1,k1Data);
}

KObject * __stdcall Init_Effect_JPEG(sInt pass,sF32 ratio,sF32 strength,sInt mode,sInt iters)
{
  GenEffect *fx = new GenEffect;
  fx->NeedCurrentRender = sTRUE;
  fx->Pass = pass;
  fx->Usage = ENGU_BASE;

  return fx;
}

void __stdcall Exec_Effect_JPEG(KOp *op,KEnvironment *kenv,sInt pass,sF32 ratio,sF32 strength,sInt mode,sInt iters)
{
  sViewport realView,smallView;

  // save real viewport and calculate position of small viewport
  realView = sSystem->CurrentViewport;
  smallView = realView;
  smallView.Window.x1 = smallView.Window.x1 - sInt((smallView.Window.x1 - smallView.Window.x0) * (ratio - 1.0f) / ratio);
  smallView.Window.y1 = smallView.Window.y1 - sInt((smallView.Window.y1 - smallView.Window.y0) * (ratio - 1.0f) / ratio);

  // render to small viewport and grab it
  sF32 scaleUV[2],scaleUV2[2];

  sInt tex = RenderTargetManager->Get(0,scaleUV[0],scaleUV[1]);
  GenOverlayManager->Mtrl[GENOVER_TEX1]->SetTex(0,tex);
  GenOverlayManager->Mtrl[GENOVER_TEX1]->Color[0] = 0xffffffff;

  sSystem->SetViewport(smallView);
  GenOverlayManager->FXQuad(GENOVER_TEX1,scaleUV);

  RenderTargetManager->GrabToTarget(0,&smallView.Window);
  scaleUV[0] /= ratio;
  scaleUV[1] /= ratio;

  // set up material instance for jpeg filter
  sMaterialInstance instance;
  sVector vc[5],pc[1];

  updateJPEGFilterKernel(strength,mode);

  instance.NumTextures = 6;
  instance.NumPSConstants = 1;
  instance.NumVSConstants = 5;
  for(sInt i=0;i<4;i++)
    instance.Textures[i] = tex;

  instance.Textures[4] = GenOverlayManager->JPEGKernel0;
  instance.Textures[5] = GenOverlayManager->JPEGKernel1;
  instance.PSConstants = pc;
  instance.VSConstants = vc;

  for(sInt i=0;i<iters;i++)
  {
    // the amazing jpeg filter of death
    for(sInt dir=0;dir<2;dir++)
    {
      pc[0].x = (dir == 0) ? scaleUV[0] / sSystem->ViewportX : 0.0f;
      pc[0].y = (dir == 1) ? scaleUV[1] / sSystem->ViewportY : 0.0f;

      vc[4].Scale4(pc[0],(mode == 1) ? 3.0f : 0.0f);

      scaleUV2[0] = (dir == 0) ? sSystem->ViewportX / 4.0f : 0.0f;
      scaleUV2[1] = (dir == 1) ? sSystem->ViewportY / 4.0f : 0.0f;

      GenOverlayManager->JPEGMaterial->SetInstance(instance);
      GenOverlayManager->FXQuad(GenOverlayManager->JPEGMaterial,scaleUV,0,scaleUV2);

      RenderTargetManager->GrabToTarget(0,&smallView.Window);
    }
  }

  // blit back to big viewport
  sSystem->SetViewport(realView);
  GenOverlayManager->FXQuad(GENOVER_TEX1,scaleUV);
}

/****************************************************************************/

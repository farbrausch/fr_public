// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#include "engine.hpp"
#include "geneffectipp.hpp"
#include "genoverlay.hpp"
#include "materialdirect.hpp"
#include "rtmanager.hpp"
#include "_startdx.hpp"

#include "effect_glarevs.hpp"
#include "effect_glareps.hpp"
#include "effect_glare2vs.hpp"
#include "effect_glare2ps.hpp"
#include "effect_glare3vs.hpp"
#include "effect_glare3ps.hpp"
#include "effect_colorcorrectvs.hpp"
#include "effect_colorcorrectps.hpp"

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
/***                                                                      ***/
/***   Glare                                                              ***/
/***                                                                      ***/
/****************************************************************************/

static const sU32 EffectGlareStates[] = {
  sD3DRS_ALPHATESTENABLE,               0,
  sD3DRS_ZENABLE,                       sD3DZB_FALSE,
  sD3DRS_ZWRITEENABLE,                  0,
  sD3DRS_ZFUNC,                         sD3DCMP_ALWAYS,
  sD3DRS_CULLMODE,                      sD3DCULL_NONE,
  sD3DRS_COLORWRITEENABLE,              15,
  sD3DRS_SLOPESCALEDEPTHBIAS,           0,
  sD3DRS_DEPTHBIAS,                     0,
  sD3DRS_FOGENABLE,                     0,
  sD3DRS_STENCILENABLE,                 0,
  sD3DRS_ALPHABLENDENABLE,              0,

  sD3DSAMP_0|sD3DSAMP_MAGFILTER,        sD3DTEXF_LINEAR,
  sD3DSAMP_0|sD3DSAMP_MINFILTER,        sD3DTEXF_LINEAR,
  sD3DSAMP_0|sD3DSAMP_MIPFILTER,        sD3DTEXF_NONE,
  sD3DSAMP_0|sD3DSAMP_ADDRESSU,         sD3DTADDRESS_CLAMP,
  sD3DSAMP_0|sD3DSAMP_ADDRESSV,         sD3DTADDRESS_CLAMP,

  sD3DSAMP_1|sD3DSAMP_MAGFILTER,        sD3DTEXF_LINEAR,
  sD3DSAMP_1|sD3DSAMP_MINFILTER,        sD3DTEXF_LINEAR,
  sD3DSAMP_1|sD3DSAMP_MIPFILTER,        sD3DTEXF_NONE,
  sD3DSAMP_1|sD3DSAMP_ADDRESSU,         sD3DTADDRESS_CLAMP,
  sD3DSAMP_1|sD3DSAMP_ADDRESSV,         sD3DTADDRESS_CLAMP,

  sD3DSAMP_2|sD3DSAMP_MAGFILTER,        sD3DTEXF_LINEAR,
  sD3DSAMP_2|sD3DSAMP_MINFILTER,        sD3DTEXF_LINEAR,
  sD3DSAMP_2|sD3DSAMP_MIPFILTER,        sD3DTEXF_NONE,
  sD3DSAMP_2|sD3DSAMP_ADDRESSU,         sD3DTADDRESS_CLAMP,
  sD3DSAMP_2|sD3DSAMP_ADDRESSV,         sD3DTADDRESS_CLAMP,

  sD3DSAMP_3|sD3DSAMP_MAGFILTER,        sD3DTEXF_LINEAR,
  sD3DSAMP_3|sD3DSAMP_MINFILTER,        sD3DTEXF_LINEAR,
  sD3DSAMP_3|sD3DSAMP_MIPFILTER,        sD3DTEXF_NONE,
  sD3DSAMP_3|sD3DSAMP_ADDRESSU,         sD3DTADDRESS_CLAMP,
  sD3DSAMP_3|sD3DSAMP_ADDRESSV,         sD3DTADDRESS_CLAMP,

  sD3DSAMP_4|sD3DSAMP_MAGFILTER,        sD3DTEXF_LINEAR,
  sD3DSAMP_4|sD3DSAMP_MINFILTER,        sD3DTEXF_LINEAR,
  sD3DSAMP_4|sD3DSAMP_MIPFILTER,        sD3DTEXF_NONE,
  sD3DSAMP_4|sD3DSAMP_ADDRESSU,         sD3DTADDRESS_CLAMP,
  sD3DSAMP_4|sD3DSAMP_ADDRESSV,         sD3DTADDRESS_CLAMP,

  ~0U,                                  ~0U,
};


KObject * __stdcall Init_Effect_Glare(sInt pass,sInt downsample,sU32 tresh,sU32 cb,sU32 co,sF32 blur0,sF32 blur1,sF32 amp0,sF32 amp1,sF32 grayf,sF32 addsmoothf)
{
  GenEffect *fx = new GenEffect;
  fx->NeedCurrentRender = 0;//sTRUE;
  fx->Pass = pass;
  fx->Usage = ENGU_BASE;

  // we use material 2.0s buffer render target, because it's going to
  // be unused during post.
  RenderTargetManager->Add(0x00010000,0,0);
  RenderTargetManager->Add(0x00020010,512,256);
  RenderTargetManager->Add(0x00020011,256,128);

  if(!GenOverlayManager->GlareMaterial)
    GenOverlayManager->GlareMaterial = new sMaterialDirect(g_effect_glare_vsh,g_effect_glare_psh,EffectGlareStates,0);
  if(!GenOverlayManager->Glare2Material)
    GenOverlayManager->Glare2Material = new sMaterialDirect(g_effect_glare2_vsh,g_effect_glare2_psh,EffectGlareStates,0);
  if(!GenOverlayManager->Glare3Material)
    GenOverlayManager->Glare3Material = new sMaterialDirect(g_effect_glare3_vsh,g_effect_glare3_psh,EffectGlareStates,0);

  return fx;
}

void __stdcall Exec_Effect_Glare(KOp *op,KEnvironment *kenv,sInt pass,sInt flags,sU32 tresh,sU32 cb,sU32 co,sF32 blur0,sF32 blur1,sF32 amp0,sF32 amp1,sF32 grayf,sF32 addsmoothf)
{
  // downscale filter kernel:
  // the actual filter kernel is (1,4,6,4,1)/16
  // use bilinear interpolation to get some samples for free
  // offset is in texels
//  static const sF32 filterTap[3] = { 0.0f/16.0f,16.0f/16.0f,0.0f/16.0f };
  static const sF32 filterTap3[3] = { 5.0f/16.0f,6.0f/16.0f,5.0f/16.0f };
  static const sF32 FilterTap9[9] = 
  { 
    1.0f/25.0f,
    2.0f/25.0f,
    3.0f/25.0f,
    4.0f/25.0f,
    5.0f/25.0f,
    4.0f/25.0f,
    3.0f/25.0f,
    2.0f/25.0f,
    1.0f/25.0f,
  };
  static const sF32 filterOff[3] = { -1.2f,-0.0f,1.2f };

  sMaterialInstance inst;
  sVector vc[9],pc[3];
  sViewport realView,smallView;
  sInt si,di;
  sMaterialInstance inst2;
  sVector vc2[4],pc2[2];
  sMaterialInstance inst3;
  sVector vc3[4],pc3[3];
  sBool down[2];


  struct texinfo
  {
    sF32 uv[2];
    sInt xs;
    sInt ys;
    sInt id;
    sInt tex;
    void Init(sInt id_)
    {
      id = id_;
      tex = RenderTargetManager->Get(id_,uv[0],uv[1]);
      sSystem->GetTextureSize(tex,xs,ys);
    }
  } ti[4];

  // preparation

  switch(flags&3)
  {
  default:
    down[0] = 1;
    down[1] = 0;
    break;
  case 1:
    down[0] = 0;
    down[1] = 1;
    break;
  case 2:
    down[0] = 1;
    down[1] = 1;
    break;
  }

  realView = sSystem->CurrentViewport;
  smallView = realView;

  ti[0].Init(0x00000);
  ti[1].Init(0x10000);
  ti[2].Init(0x20010);
  ti[3].Init(0x20011);

  inst.NumVSConstants = 9;
  inst.VSConstants = vc;
  inst.NumPSConstants = 3;
  inst.PSConstants = pc;
  inst.NumTextures = 5;

  vc[8].z = vc[8].w = 0.0f;
  pc[0].x = pc[0].y = pc[0].z = 0.0f;

  // grab original

  sSystem->ClearTarget(ti[0].tex,0xff000000);
  RenderTargetManager->GrabToTarget(ti[0].id,&realView.Window);

  // downscale filter

  si = 0;
  for(sInt j=0;j<2;j++)
  {
    if(down[j])
    {
      di = 2+j;

      smallView.Window.x0 = 0;
      smallView.Window.y0 = 0;
      smallView.Window.x1 = ti[di].xs;
      smallView.Window.y1 = ti[di].ys;

      if(flags&4)
      {
        sMaterialEnv env;
        env.Init();
        env.Orthogonal = sMEO_PIXELS;
        env.CameraSpace.Init();
        env.ModelSpace.Init();

        sSystem->SetViewport(smallView);
        GenOverlayManager->Mtrl[GENOVER_TEX1]->SetTex(0,ti[si].tex);
        GenOverlayManager->Mtrl[GENOVER_TEX1]->Color[0] = 0xffffffff;
        GenOverlayManager->Mtrl[GENOVER_TEX1]->Set(env);
        GenOverlayManager->FXQuad(GENOVER_TEX1,ti[si].uv);
      }
      else
      {
        for(sInt i=0;i<9;i++)
        {
          sInt yo = i/3,xo=i%3;
          sF32 *vcp = &vc[4].x + i*2;

          vcp[0] = filterOff[xo] / ti[si].xs;
          vcp[1] = filterOff[yo] / ti[si].ys;
          (&pc[0].w)[i] = filterTap3[xo] * filterTap3[yo];
        }  

        for(sInt i=0;i<5;i++)
          inst.Textures[i] = ti[si].tex;

        sSystem->SetViewport(smallView);
        GenOverlayManager->GlareMaterial->SetInstance(inst);
        GenOverlayManager->FXQuad(GenOverlayManager->GlareMaterial,ti[si].uv);
      }
      RenderTargetManager->GrabToTarget(ti[di].id,&smallView.Window);
      si = di;
    }
  }

  // tonemapping (sub + dot3)

  inst2.NumVSConstants = 4;
  inst2.VSConstants = vc2;
  inst2.NumPSConstants = 2;
  inst2.PSConstants = pc2;
  inst2.NumTextures = 1;
  pc2[0].InitColor(tresh);
  pc2[0].w = 1.0f/((1-pc2[0].x)+(1-pc2[0].y)+(1-pc2[0].z));
  pc2[1].Init(grayf,1-grayf,0,0);
  inst2.Textures[0] = ti[si].tex;
  GenOverlayManager->Glare2Material->SetInstance(inst2);
  GenOverlayManager->FXQuad(GenOverlayManager->Glare2Material,ti[si].uv);
  RenderTargetManager->GrabToTarget(ti[si].id,&smallView.Window);


  // the glare itself

  sF32 aspect = 1.0f*ti[si].xs/ti[si].ys;

  for(sInt i=0;i<5;i++)
    inst.Textures[i] = ti[si].tex;
  for(sInt j=0;j<2;j++)
  {
    if(blur0>0)
    {
      for(sInt i=0;i<9;i++)
      {
        sF32 *vcp = &vc[4].x + i*2;

        vcp[0] = (i-4)*blur0;
        vcp[1] = 0;
        (&pc[0].w)[i] = FilterTap9[i];
      }
      GenOverlayManager->GlareMaterial->SetInstance(inst);
      GenOverlayManager->FXQuad(GenOverlayManager->GlareMaterial,ti[si].uv);
      RenderTargetManager->GrabToTarget(ti[di].id,&smallView.Window);

      for(sInt i=0;i<9;i++)
      {
        sF32 *vcp = &vc[4].x + i*2;

        vcp[0] = 0;
        vcp[1] = (i-4)*blur0*aspect;
        (&pc[0].w)[i] = FilterTap9[i]*amp0;
      }
      GenOverlayManager->GlareMaterial->SetInstance(inst);
      GenOverlayManager->FXQuad(GenOverlayManager->GlareMaterial,ti[si].uv);
      RenderTargetManager->GrabToTarget(ti[di].id,&smallView.Window);
    }
    blur0 = blur1;
    amp0 = amp1;
  }

  // copy back

  pc3[0].InitColor(cb);
  pc3[1].InitColor(co);
  pc3[2].Init(addsmoothf,0,0,0);
  inst3.NumVSConstants = 4;
  inst3.VSConstants = vc3;
  inst3.NumPSConstants = 3;
  inst3.PSConstants = pc3;
  inst3.NumTextures = 2;
  inst3.Textures[0] = ti[di].tex;
  inst3.Textures[1] = ti[0].tex;

  sSystem->SetViewport(realView);
  GenOverlayManager->Glare3Material->SetInstance(inst3);
  GenOverlayManager->FXQuad(GenOverlayManager->Glare3Material,ti[di].uv,0,ti[0].uv,0);
}

/****************************************************************************/
/***                                                                      ***/
/***   Color correction                                                   ***/
/***                                                                      ***/
/****************************************************************************/

static const sU32 EffectColorCorrectStates[] = {
  sD3DRS_ALPHATESTENABLE,               0,
  sD3DRS_ZENABLE,                       sD3DZB_TRUE,
  sD3DRS_ZWRITEENABLE,                  0,
  sD3DRS_ZFUNC,                         sD3DCMP_ALWAYS,
  sD3DRS_CULLMODE,                      sD3DCULL_NONE,
  sD3DRS_COLORWRITEENABLE,              15,
  sD3DRS_SLOPESCALEDEPTHBIAS,           0,
  sD3DRS_DEPTHBIAS,                     0,
  sD3DRS_FOGENABLE,                     0,
  sD3DRS_STENCILENABLE,                 0,
  sD3DRS_ALPHABLENDENABLE,              0,

  sD3DSAMP_0|sD3DSAMP_MAGFILTER,        sD3DTEXF_LINEAR,
  sD3DSAMP_0|sD3DSAMP_MINFILTER,        sD3DTEXF_LINEAR,
  sD3DSAMP_0|sD3DSAMP_MIPFILTER,        sD3DTEXF_NONE,
  sD3DSAMP_0|sD3DSAMP_ADDRESSU,         sD3DTADDRESS_CLAMP,
  sD3DSAMP_0|sD3DSAMP_ADDRESSV,         sD3DTADDRESS_CLAMP,

  ~0U,                                  ~0U,
};


KObject * __stdcall Init_Effect_ColorCorrection(sInt pass,sF32 t0,sF32 t1,sU32 c0,sU32 c1,
                                                sU32 color,sU32 gamma,sInt flags,sU32 c0g,sU32 c1g,sF32 amp)
{
  GenEffect *fx = new GenEffect;
  fx->NeedCurrentRender = sTRUE;
  fx->Pass = pass;
  fx->Usage = ENGU_BASE;

  if(!GenOverlayManager->ColorCorrectMaterial)
    GenOverlayManager->ColorCorrectMaterial = new sMaterialDirect(g_effect_colorcorrect_vsh,g_effect_colorcorrect_psh,EffectColorCorrectStates,0);

  return fx;
}

void __stdcall Exec_Effect_ColorCorrection(KOp *op,KEnvironment *kenv,sInt pass,sF32 t0,sF32 t1,sU32 c0,sU32 c1,
                                           sU32 color,sU32 gamma,sInt flags,sU32 c0g,sU32 c1g,sF32 amp)
{
  sF32 scaleUV[2];

  sInt tex = RenderTargetManager->Get(0,scaleUV[0],scaleUV[1]);

  sMaterialInstance inst;
  sVector vc[4],pc[7];

  inst.NumVSConstants = 4;
  inst.VSConstants = vc;
  inst.NumPSConstants = 7;
  inst.PSConstants = pc;
  inst.NumTextures = 1;
  inst.Textures[0] = tex;

  if(flags & 1)       // fake mode: show only range
  {
    pc[0].Init(0,0,0,0);
    pc[1].Init(0,0,0,0);
    pc[2].Init(1,1,1,1);
    pc[3].Init(0,0,0,0);
    pc[4].Init(-t0/(t1-t0),1/(t1-t0),1,0);
    pc[5].Init(0,0,0,0);
    pc[6].Init(0,0,0,0);
  }
  else                // normal mode
  {
    pc[0].InitColor(c0);
    pc[1].InitColor(c1);
    pc[2].InitColor(color);
    pc[2].Scale4(amp);
    pc[3].InitColor(gamma);
    pc[4].Init(-t0/(t1-t0),1/(t1-t0),0,0);
    pc[5].InitColor(c0g);
    pc[6].InitColor(c1g);
  }  

  sSystem->SetViewport(sSystem->CurrentViewport);
  GenOverlayManager->ColorCorrectMaterial->SetInstance(inst);
  GenOverlayManager->FXQuad(GenOverlayManager->ColorCorrectMaterial,scaleUV);
}

/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#define sPEDANTIC_WARN 1
#include "screens4fx.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Siegmeister                                                        ***/
/***                                                                      ***/
/****************************************************************************/

RNSiegmeister::RNSiegmeister()
{
  Geo = new sGeometry(sGF_QUADLIST,sVertexFormatSingle);

  Mtrl = new sSimpleMaterial;
  Mtrl->BlendColor = sMB_PMALPHA;

  Mtrl->Flags = sMTRL_CULLOFF|sMTRL_ZOFF;
  Mtrl->TFlags[0] = sMTF_CLAMP|sMTF_UV0;
  
  Spread = 0.5;
  Fade = 1;
  DoBlink = sTRUE;
  Color = 0xffffffff;
  BlinkColor1 = 0xffffffff;
  BlinkColor2 = 0xff000000;
  Alpha = 0.5;
  Bars.HintSize(16);
  Bars.AddTail(sFRect(0.05,0.3,0.95,0.4));

  Anim.Init(Wz4RenderType->Script);
}

RNSiegmeister::~RNSiegmeister()
{
  delete Geo;
  delete Mtrl;
  sRelease(Texture);
}

void RNSiegmeister::Init()
{
  if (Texture) Mtrl->Texture[0] = Texture->Texture;
  Mtrl->Prepare(sVertexFormatSingle);
}

void RNSiegmeister::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
 
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);
}

void RNSiegmeister::Prepare(Wz4RenderContext *ctx)
{
  sVertexSingle *vp;

  sInt sx = ctx->RTSpec.Window.SizeX();
  sInt sy = ctx->RTSpec.Window.SizeY();
  sF32 scrnaspect=sF32(sx)/sF32(sy);
  sF32 arr=scrnaspect*sF32(Para.AspDen)/sF32(Para.AspNum);
  sF32 w=0.5f, h=0.5f;
  if (arr>1) w/=arr; else h*=arr;
  
  sFRect rect(sx*(0.5f-w)+0.5f,sy*(0.5f-h)+0.5f,sx*(0.5f+w)+0.5f,sy*(0.5f+h)+0.5f);
  sF32 time = ctx->GetTime();

  sInt bars = Bars.GetCount();
  Geo->BeginLoadVB(4*bars,sGD_FRAME,&vp);
  for (sInt i=0; i<bars; i++)
  {
    sFRect brect = Bars[i];
    sU32 color = (DoBlink && i<3) ? sColorFade(BlinkColor1,BlinkColor2,0.5f*sFCos(Para.BlinkSpeed*time)+0.5f) : Color;
    color = sColorFade(0,color,Alpha);
        
    sF32 phase = sFMod(brect.x1*10000,sPI2F);
    sF32 t = Fade + 0.25f*Spread*sFSin(phase)*(4*(-Fade*Fade+Fade));
    sF32 w = sClamp(t, 0.0f, brect.x1-brect.x0);
    sF32 u0 = brect.x0;
    sF32 u1 = brect.x1 = brect.x0+w;

    brect.x0 = rect.x0+brect.x0*(rect.x1-rect.x0);
    brect.y0 = rect.y0+brect.y0*(rect.y1-rect.y0);
    brect.x1 = rect.x0+brect.x1*(rect.x1-rect.x0);
    brect.y1 = rect.y0+brect.y1*(rect.y1-rect.y0);
        
    (*vp++).Init(brect.x0,brect.y0,0.5,color,u0,0);
    (*vp++).Init(brect.x1,brect.y0,0.5,color,u1,0);
    (*vp++).Init(brect.x1,brect.y1,0.5,color,u1,1);
    (*vp++).Init(brect.x0,brect.y1,0.5,color,u0,1);
  }
  Geo->EndLoadVB();
}

void RNSiegmeister::Render(Wz4RenderContext *ctx)
{
  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sViewport view;
    view.Orthogonal = sVO_PIXELS;
    view.Prepare();

    sCBuffer<sSimpleMaterialPara> cb;
    cb.Data->Set(view);

    Mtrl->Set(&cb);
    Geo->Draw();
  }
}



/****************************************************************************/


/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4frlib/wz4_ipp.hpp"
#include "util/ipp.hpp"
#include "base/graphics.hpp"
#include "wz4frlib/wz4_ipp_shader.hpp"

/****************************************************************************/

IppHelper2::IppHelper2()
{
  GeoDouble = new sGeometry(sGF_QUADLIST,sVertexFormatDouble);
  MtrlCopy = new Wz4IppCopy();
  MtrlCopy->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  MtrlCopy->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  MtrlCopy->Prepare(sVertexFormatSingle);

  MtrlSampleLine9 = new Wz4IppSampleLine9;
  MtrlSampleLine9->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  MtrlSampleLine9->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  MtrlSampleLine9->Prepare(sVertexFormatDouble);

  MtrlSampleRect9 = new Wz4IppSampleRect9;
  MtrlSampleRect9->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  MtrlSampleRect9->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  MtrlSampleRect9->Prepare(sVertexFormatDouble);

  MtrlSampleRect16 = new Wz4IppSampleRect16;
  MtrlSampleRect16->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  MtrlSampleRect16->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  MtrlSampleRect16->Prepare(sVertexFormatDouble);

  Wz4IppSampleRect16 *s16aw = new Wz4IppSampleRect16;
  MtrlSampleRect16AW = s16aw;
  s16aw->AlphaWeighted = sTRUE;
  MtrlSampleRect16AW->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  MtrlSampleRect16AW->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  MtrlSampleRect16AW->Prepare(sVertexFormatDouble);

  sGraphicsCaps caps;
  sGetGraphicsCaps(caps);
  XYOffset = caps.XYOffset;
  UVOffset = caps.UVOffset;
}

IppHelper2::~IppHelper2()
{
  delete GeoDouble;
  delete MtrlCopy;
  delete MtrlSampleLine9;
  delete MtrlSampleRect9;
  delete MtrlSampleRect16;
  delete MtrlSampleRect16AW;
}

/****************************************************************************/

void IppHelper2::GetTargetInfo(sMatrix44 &mvp)
{
  sViewport view;
  view.Orthogonal = sVO_PIXELS;
  view.SetTargetCurrent();
  view.Prepare();
  mvp = view.ModelScreen;
}

void IppHelper2::DrawQuad(sTexture2D *dest,sTexture2D *s0,sTexture2D *s1,sU32 col,sInt flags)
{
  sF32 u0,v0,u1,v1,u2,v2,u3,v3;
  sInt sx,sy;

  u0=v0=u2=v2=0;
  u1=v1=u3=v3=1;

  sF32 uv = UVOffset;
  sF32 xy = XYOffset;

  if(flags & IPPH_BLURRY)
    uv += 0.5f;

  if(s0)
  {
    u0 = 0+uv/s0->SizeX;
    v0 = 0+uv/s0->SizeY;
    u1 = 1+uv/s0->SizeX;
    v1 = 1+uv/s0->SizeY;
  }
  if(s1)
  {
    u2 = 0+uv/s1->SizeX;
    v2 = 0+uv/s1->SizeY;
    u3 = 1+uv/s1->SizeX;
    v3 = 1+uv/s1->SizeY;
  }
  if(dest)
  {
    sx = dest->SizeX;
    sy = dest->SizeY;
  }
  else
  {
    sGetRendertargetSize(sx,sy);
  }

  sVertexDouble *vp;
  GeoDouble->BeginLoadVB(4,sGD_STREAM,&vp);
  vp[0].Init(   xy,   xy, 0,col ,u0,v0 ,u2,v2);
  vp[1].Init(sx+xy,   xy, 0,col ,u1,v0 ,u3,v2);
  vp[2].Init(sx+xy,sy+xy, 0,col ,u1,v1 ,u3,v3);
  vp[3].Init(   xy,sy+xy, 0,col ,u0,v1 ,u2,v3);
  GeoDouble->EndLoadVB();
  GeoDouble->Draw();
}

/****************************************************************************/

void IppHelper2::BlurFirst(sTexture2D *dest,sTexture2D *src,sF32 radius,sF32 amp,sBool alphaWeighted)
{
  sCBuffer<Wz4IppVSPara> cbv;
  sCBuffer<Wz4IppSampleRect16PS> cbp;
  sF32 f;
  sInt tx,ty;

  // calculate filter kernel

  sF32 ampg[8][8];
  sClear(ampg);

  sF32 sumg=0;
  for(sInt y=0;y<7;y++)
  {
    for(sInt x=0;x<7;x++)
    {
      sF32 dx = (3-x);
      sF32 dy = (3-y);
      ampg[y][x] = sMax<sF32>(0,radius-sSqrt(dx*dx+dy*dy));

      sumg += ampg[y][x];
    }
  }
  for(sInt y=0;y<7;y++)
    for(sInt x=0;x<7;x++)
      ampg[y][x] *= 1/sumg;

  // calculate pixel offsets & colors

  sF32 offx[4][4];
  sF32 offy[4][4];
  sF32 ampp[4][4];

  tx = src->SizeX; 
  ty = src->SizeY; 
  sF32 sump = 1;
  for(sInt y=0;y<4;y++)
  {
    for(sInt x=0;x<4;x++)
    {
      sF32 dx = x*2-2.5f;
      sF32 dy = y*2-2.5f;

      sF32 i00 = ampg[y*2+0][x*2+0];
      sF32 i01 = ampg[y*2+0][x*2+1];
      sF32 i10 = ampg[y*2+1][x*2+0];
      sF32 i11 = ampg[y*2+1][x*2+1];
      sF32 ito = i00+i01+i10+i11;

      ampp[y][x] = (ito);
      dx += (i01+i11)/ito;
      dy += (i10+i11)/ito;

      offx[y][x] = dx/tx;
      offy[y][x] = dy/ty;
    }
  }

  // blur

  sRTMan->SetTarget(dest);
  GetTargetInfo(cbv.Data->mvp);
  cbv.Modify();
  f = amp/sump;
  for(sInt i=0;i<4;i++)
  {
    cbp.Data->Color[i].Init(ampp[i][0]*f,ampp[i][1]*f,ampp[i][2]*f,ampp[i][3]*f);
    cbp.Data->Offa[i].Init(offx[i][0],offy[i][0],offx[i][1],offy[i][1]);
    cbp.Data->Offb[i].Init(offx[i][2],offy[i][2],offx[i][3],offy[i][3]);
  }
  cbp.Modify();

  sMaterial *mtrl = alphaWeighted ? MtrlSampleRect16AW : MtrlSampleRect16;

  mtrl->Texture[0] = src;
  mtrl->Set(&cbv,&cbp);
  DrawQuad(dest,0);
  mtrl->Texture[0] = 0;

  sRTMan->Release(src);
}

void IppHelper2::BlurLine(sTexture2D *dest,sTexture2D *src,sF32 radiusx,sF32 radiusy,sF32 amp)
{
  sCBuffer<Wz4IppVSPara> cbv;
  sCBuffer<Wz4IppPSPara> cbp;  
  sF32 f;
  sF32 fx,fy;

  // blit down

  sRTMan->SetTarget(dest);
  GetTargetInfo(cbv.Data->mvp);
  cbv.Modify();
  f = amp/25;

  fx = radiusx/(src->SizeX*4);
  fy = radiusy/(src->SizeY*4);

  cbp.Data->Color0.Init(f  ,f*2,f*3,f*4);
  cbp.Data->Color1.Init(f*5,f*4,f*3,f*2);
  cbp.Data->Color2.Init(f  ,0  ,fx  ,fy);
  cbp.Modify();

  MtrlSampleLine9->Texture[0] = src;
  MtrlSampleLine9->Set(&cbv,&cbp);
  DrawQuad(dest,src);
  MtrlSampleLine9->Texture[0] = 0;

  sRTMan->Release(src);
}

void IppHelper2::BlurNext(sTexture2D *dest,sTexture2D *src,sF32 radius,sF32 amp)
{
  sTexture2D *mid = sRTMan->Acquire(src->SizeX,src->SizeY,src->Flags);

  BlurLine(mid,src,radius,0,sSqrt(amp));
  BlurLine(dest,mid,0,radius,sSqrt(amp));
}


void IppHelper2::Blur(sTexture2D *dest,sTexture2D *src,sF32 radius,sF32 amp)
{
  sVERIFY(src!=dest);
  if(radius<4.0f)
  {
    BlurFirst(dest,src,radius,amp,sFALSE);
  }
  else
  {
    sTexture2D *mid1 = sRTMan->Acquire(src->SizeX,src->SizeY,src->Flags);
    BlurFirst(mid1,src,4,amp,sFALSE);

    if(radius<12.0f)
    {
      BlurNext(dest,mid1,radius-4,1);
    }
    else
    {
      sTexture2D *mid2 = sRTMan->Acquire(src->SizeX,src->SizeY,src->Flags);
      BlurNext(mid2,mid1,12-4,1);

      if(radius<60.0f)
      {
        BlurNext(dest,mid2,radius-12,1);
      }
      else
      {
        sTexture2D *mid3 = sRTMan->Acquire(src->SizeX,src->SizeY,src->Flags);
        BlurNext(mid3,mid2,60-12,1);
        BlurNext(dest,mid3,radius-60,1);
      }
    }
  }
}

void IppHelper2::Copy(sTexture2D *dest,sTexture2D *src)
{
  sCBuffer<Wz4IppVSPara> cbv;
  sCBuffer<Wz4IppPSPara> cbp;

  sRTMan->SetTarget(dest);
  GetTargetInfo(cbv.Data->mvp);
  cbv.Modify();
  cbp.Data->Color0.Init(1,1,1,1);
  cbp.Data->Color1.Init(0,0,0,0);
  cbp.Modify();

  MtrlCopy->Texture[0] = src;
  MtrlCopy->Set(&cbv,&cbp);
  DrawQuad(dest,src);
  MtrlCopy->Texture[0] = 0;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

static const sVector30 LumWeights(0.3086f,0.6094f,0.0820f);

/****************************************************************************/

RNColorCorrect::RNColorCorrect()
{
  Anim.Init(Wz4RenderType->Script);
  Mtrl = new Wz4IppCopy();
  Mtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  Mtrl->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  Mtrl->Prepare(sVertexFormatBasic);
}

RNColorCorrect::~RNColorCorrect()
{
  delete Mtrl;
}

void RNColorCorrect::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  SimulateChilds(ctx);
}

void RNColorCorrect::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sCBuffer<Wz4IppVSPara> cbv;
    sCBuffer<Wz4IppPSPara> cbp;

    sTexture2D *src = sRTMan->ReadScreen();
    sTexture2D *dest = sRTMan->WriteScreen();
    sRTMan->SetTarget(dest);
    ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);
    cbp.Data->Color0.InitColor(Para.Color0);
    cbp.Data->Color1.InitColor(Para.Color1);

    Mtrl->Texture[0] = src;
    Mtrl->Set(&cbv,&cbp);
    ctx->IppHelper->DrawQuad(dest,src);
    Mtrl->Texture[0] = 0;

    sRTMan->Release(src);
    sRTMan->Release(dest);
  }
}

/****************************************************************************/

RNDebugZ::RNDebugZ()
{
  Anim.Init(Wz4RenderType->Script);
  Mtrl = 0;
}

RNDebugZ::~RNDebugZ()
{
  delete Mtrl;
}

void RNDebugZ::Init()
{
  switch(Para.Channel)
  {
  default:
  case 0:
    Mtrl = new Wz4IppDebugZ();
    break;
  case 1:
    Mtrl = new Wz4IppDebugNormal();
    break;
  case 2:
    Mtrl = new Wz4IppDebugAlpha();
    break;
  }

  Mtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  Mtrl->TFlags[0] = sMTF_LEVEL2 | sMTF_TILE | sMTF_EXTERN;
  Mtrl->Prepare(sVertexFormatSingle);
}

void RNDebugZ::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  SimulateChilds(ctx);

  switch(Para.Channel)
  {
  default:
  case 0:
    ctx->RenderFlags |= wRF_RenderZ;
    break;
  case 1:
    ctx->RenderFlags |= wRF_RenderZ;
    break;
  case 2:
    break;
  }
}

void RNDebugZ::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sCBuffer<Wz4IppVSPara> cbv;
    sCBuffer<Wz4IppPSPara> cbp;


    if(Para.Channel==2)
    {
      Mtrl->Texture[0] = sRTMan->ReadScreen();
    }
    else
    {
      Mtrl->Texture[0] = ctx->ZTarget;
    }

    sTexture2D *dest = sRTMan->WriteScreen();
    sRTMan->SetTarget(dest);
    ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);

    cbp.Data->Color0.Init(1,16,256,0);


    Mtrl->Set(&cbv,&cbp);
    ctx->IppHelper->DrawQuad(dest,ctx->ZTarget);
    if(Para.Channel==2)
      sRTMan->Release(Mtrl->Texture[0]);
    Mtrl->Texture[0] = 0;
    sRTMan->Release(dest);
  }
}

/****************************************************************************/

RNBlur::RNBlur()
{
  Anim.Init(Wz4RenderType->Script);
  Mtrl = new Wz4IppSharpen();
  Mtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  Mtrl->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  Mtrl->TFlags[1] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  Mtrl->Prepare(sVertexFormatBasic);
}

RNBlur::~RNBlur()
{
  delete Mtrl;
}

void RNBlur::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  SimulateChilds(ctx);
}

void RNBlur::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    // blur
    sTexture2D *org = 0;
    if (Para.Radius>0)
    {
      sTexture2D *src = sRTMan->ReadScreen();
      sTexture2D *dest = sRTMan->WriteScreen();
      if(Para.SharpenFlag)
      {
        org = src; 
        sRTMan->AddRef(org);
      }

      sF32 radius = (Para.Unit&1) ? Para.Radius*src->SizeY/1080 : Para.Radius;

      ctx->IppHelper->Blur(dest,src,radius,Para.Amp);
      sRTMan->Release(dest);
    }

    // sharpen
    if(Para.SharpenFlag)
    {
      sTexture2D *src = sRTMan->ReadScreen();
      sTexture2D *dest = sRTMan->WriteScreen();
      sRTMan->SetTarget(dest);

      sCBuffer<Wz4IppVSPara> cbv;
      sCBuffer<Wz4IppPSPara> cbp;
      ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);

      if(Para.SharpenFlag==2)
      {
        sVector4 s0,s1;
        s0.InitColor(Para.Sharpen0);
        s1.InitColor(Para.Sharpen1);
        s0 *= Para.Sharpen0Amp;
        s1 *= Para.Sharpen1Amp;
        cbp.Data->Color0 = s0;
        cbp.Data->Color1 = s1;
      }
      else
      {
        sF32 s0 = Para.Sharpen;
        sF32 s1 = 1/(1-s0);
        cbp.Data->Color0.Init(s0,s0,s0,s0);
        cbp.Data->Color1.Init(s1,s1,s1,s1);
      }

      Mtrl->Texture[0] = org;
      Mtrl->Texture[1] = src;
      Mtrl->Set(&cbv,&cbp);

      ctx->IppHelper->DrawQuad(dest,src,0);
      Mtrl->Texture[0] = 0;
      Mtrl->Texture[1] = 0;

      sRTMan->Release(src);
      sRTMan->Release(org);
      sRTMan->Release(dest);
    }
  }
}

/****************************************************************************/

RNDof::RNDof()
{
  Anim.Init(Wz4RenderType->Script);
  MtrlCopy = new Wz4IppCopy();
  MtrlCopy->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  MtrlCopy->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  MtrlCopy->Prepare(sVertexFormatSingle);
  MtrlSampleRect9 = new Wz4IppSampleRect9();
  MtrlSampleRect9->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  MtrlSampleRect9->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  MtrlSampleRect9->Prepare(sVertexFormatDouble);
  MtrlDof = 0;
}

void RNDof::Init()
{
  Para = ParaBase;

  Wz4IppSampleDof *mtrl = new Wz4IppSampleDof();

  MtrlDof = mtrl;
  mtrl->Debug = Para.Debug;
  mtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  mtrl->TFlags[0] = sMTF_LEVEL0 | sMTF_CLAMP | sMTF_EXTERN;
  mtrl->TFlags[1] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  mtrl->TFlags[2] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  mtrl->TFlags[3] = sMTF_LEVEL0 | sMTF_CLAMP | sMTF_EXTERN;
  mtrl->Prepare(sVertexFormatDouble);
}

RNDof::~RNDof()
{
  delete MtrlSampleRect9;
  delete MtrlCopy;
  delete MtrlDof;
}

void RNDof::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  SimulateChilds(ctx);

  ctx->RenderFlags |= wRF_RenderZ;
}

void RNDof::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sCBuffer<Wz4IppVSPara> cbv;
    sCBuffer<Wz4IppPSPara> cbp;
    sF32 f,fx,fy;

    // blit down

    sTexture2D *screen = sRTMan->ReadScreen();
    sTexture2D *blur0 = sRTMan->Acquire(screen->SizeX/2,screen->SizeY/2,sTEX_2D|sTEX_NOMIPMAPS|sTEX_ARGB8888);
    sRTMan->SetTarget(blur0);
    ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);
    cbv.Modify();

    fx = Para.BlurRadius[0]/screen->SizeX;
    fy = Para.BlurRadius[0]/screen->SizeY;

    f = 1.0f/9;
    cbp.Data->Color0.Init(f,f,f,f);
    cbp.Data->Color1.Init(f,f,f,f);
    cbp.Data->Color2.Init(f,0,fx,fy);
    cbp.Modify();

    MtrlSampleRect9->Texture[0] = screen;
    MtrlSampleRect9->Set(&cbv,&cbp);
    ctx->IppHelper->DrawQuad(blur0,screen);
    MtrlSampleRect9->Texture[0] = 0;

    // blit down

    sTexture2D *blur1 = sRTMan->Acquire(screen->SizeX/4,screen->SizeY/4,sTEX_2D|sTEX_NOMIPMAPS|sTEX_ARGB8888);
    sRTMan->SetTarget(blur1);
    ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);
    cbv.Modify();

    fx = Para.BlurRadius[1]/screen->SizeX;
    fy = Para.BlurRadius[1]/screen->SizeY;

    f = 1.0f/9;
    cbp.Data->Color0.Init(f,f,f,f);
    cbp.Data->Color1.Init(f,f,f,f);
    cbp.Data->Color2.Init(f,0,fx,fy);
    cbp.Modify();

    MtrlSampleRect9->Texture[0] = blur0;
    MtrlSampleRect9->Set(&cbv,&cbp);
    ctx->IppHelper->DrawQuad(blur1,blur0);
    MtrlSampleRect9->Texture[0] = 0;

    // blit back

    sTexture2D *dest = sRTMan->WriteScreen();
    sRTMan->SetTarget(dest);
    ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);
    cbv.Modify();
    {
      sF32 d0 = (Para.Depth[0]                )/ctx->View.ClipFar;
      sF32 d1 = (Para.Depth[1]-Para.CenterSize)/ctx->View.ClipFar;
      sF32 d2 = (Para.Depth[1]+Para.CenterSize)/ctx->View.ClipFar;
      sF32 d3 = (Para.Depth[2]                )/ctx->View.ClipFar;
      sF32 c0 = Para.Blur[0];
      sF32 c1 = Para.Blur[1];
      sF32 c2 = Para.Blur[1];
      sF32 c3 = Para.Blur[2];
      cbp.Data->Color0.Init(d0,d1,d2,d3);
      cbp.Data->Color1.Init(c0,c1,c2,c3);
      cbp.Data->Color2.Init((c1-c0),1/(d1-d0),(c3-c2),1/(d3-d2));
    }
    cbp.Data->Color3.Init(0.5f/blur0->SizeX,0.5f/blur0->SizeY,0.5f/blur1->SizeX,0.5f/blur1->SizeY);
    cbp.Modify();

    MtrlDof->Texture[0] = screen;
    MtrlDof->Texture[1] = blur0;
    MtrlDof->Texture[2] = blur1;
    MtrlDof->Texture[3] = ctx->ZTarget;
    MtrlDof->Set(&cbv,&cbp);
    ctx->IppHelper->DrawQuad(dest,screen,0);
    MtrlDof->Texture[0] = 0;
    MtrlDof->Texture[1] = 0;
    MtrlDof->Texture[2] = 0;
    MtrlDof->Texture[3] = 0;

    sRTMan->Release(screen);
    sRTMan->Release(blur0);
    sRTMan->Release(blur1);
    sRTMan->Release(dest);
  }
}

/****************************************************************************/

RNDof2::RNDof2()
{
  Anim.Init(Wz4RenderType->Script);
  MtrlDown = 0;
  MtrlCoC = 0;
  MtrlDof = 0;
}

RNDof2::~RNDof2()
{
  delete MtrlDown;
  delete MtrlCoC;
  delete MtrlDof;
}

void RNDof2::Init()
{
  Para = ParaBase;

  Wz4IppCoCDof2 *mtrlCoc = new Wz4IppCoCDof2();
  MtrlCoC = mtrlCoc;
  mtrlCoc->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  mtrlCoc->TFlags[0] = sMTF_LEVEL0 | sMTF_CLAMP | sMTF_EXTERN;
  mtrlCoc->TFlags[1] = sMTF_LEVEL0 | sMTF_CLAMP | sMTF_EXTERN;
  mtrlCoc->Prepare(sVertexFormatDouble);

  Wz4IppSampleDof2 *mtrl = new Wz4IppSampleDof2();

  MtrlDof = mtrl;
  mtrl->Debug = Para.Debug;
  mtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  mtrl->TFlags[0] = sMTF_LEVEL1 | sMTF_CLAMP | sMTF_EXTERN;
  mtrl->TFlags[1] = sMTF_LEVEL1 | sMTF_CLAMP | sMTF_EXTERN;
  mtrl->TFlags[2] = sMTF_LEVEL1 | sMTF_CLAMP | sMTF_EXTERN;
  mtrl->TFlags[3] = sMTF_LEVEL1 | sMTF_CLAMP | sMTF_EXTERN;
  mtrl->TFlags[4] = sMTF_LEVEL1 | sMTF_CLAMP | sMTF_EXTERN;
  mtrl->Prepare(sVertexFormatDouble);
}

void RNDof2::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);

  SimulateChilds(ctx);

  ctx->RenderFlags |= wRF_RenderZ | wRF_RenderZLowres;
}

void RNDof2::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sCBuffer<Wz4IppVSPara> cbv;
    sCBuffer<Wz4IppPSPara> cbp;

    sTexture2D *screen = sRTMan->ReadScreen();

    sInt sizeX = screen->SizeX, sizeY = screen->SizeY;
    sTexture2D *down4 = sRTMan->Acquire(sizeX/4,sizeY/4);
    sTexture2D *down4blur = sRTMan->Acquire(sizeX/4,sizeY/4);
    sTexture2D *depth4blur = sRTMan->Acquire(sizeX/4,sizeY/4,sTEX_2D|sTEX_NOMIPMAPS|sTEX_ARGB32F);
    sTexture2D *dest = sRTMan->WriteScreen();

    // set up parameters
    cbp.Data->Color0.x = ctx->View.Proj.k.z;
    cbp.Data->Color0.y = ctx->View.Proj.l.z;
    cbp.Data->Color0.z = Para.FocalDepth / ctx->View.ClipFar;
    cbp.Data->Color0.w = Para.FocusRange / ctx->View.ClipFar;
    cbp.Data->Color1.x = ctx->View.ClipFar / Para.UnfocusRange;
    cbp.Data->Color2.Init( 0.5f/sizeX,-1.5f/sizeY,-1.5f/sizeX,-0.5f/sizeY);
    cbp.Data->Color3.Init(-0.5f/sizeX, 1.5f/sizeY, 1.5f/sizeX, 0.5f/sizeY);
    cbp.Modify();

    // medium blur: circle of confusion pass
    sTexture2D *medblurin = sRTMan->Acquire(sizeX/2,sizeY/2);
    sRTMan->SetTarget(medblurin);
    ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);
    cbv.Modify();

    MtrlCoC->Texture[0] = screen;
    MtrlCoC->Texture[1] = ctx->ZTarget;
    MtrlCoC->Set(&cbv,&cbp);
    ctx->IppHelper->DrawQuad(medblurin,screen);
    MtrlCoC->Texture[0] = 0;
    MtrlCoC->Texture[1] = 0;

    // medium blur: actual blur pass
    sTexture2D *medblur = sRTMan->Acquire(sizeX/2,sizeY/2);
    ctx->IppHelper->BlurFirst(medblur,medblurin,Para.MediumBlur/2.0f,1.0f,sTRUE);

    // downsample
    ctx->IppHelper->Copy(down4,medblur);

    // blur downsampled image
    ctx->IppHelper->BlurFirst(down4blur,down4,Para.LargeBlur/4.0f,1.0f,sTRUE);

    // blur depth
    sRTMan->AddRef(ctx->ZTargetLowRes);
    ctx->IppHelper->BlurFirst(depth4blur,ctx->ZTargetLowRes,Para.LargeBlur/4.0f,1.0f,sFALSE);

    // DOF
    sRTMan->SetTarget(dest);
    ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);
    cbv.Modify();

    MtrlDof->Texture[0] = screen;
    MtrlDof->Texture[1] = medblur;
    MtrlDof->Texture[2] = down4blur;
    MtrlDof->Texture[3] = ctx->ZTarget;
    MtrlDof->Texture[4] = depth4blur;
    MtrlDof->Set(&cbv,&cbp);
    ctx->IppHelper->DrawQuad(dest,screen,down4blur);
    MtrlDof->Texture[0] = 0;
    MtrlDof->Texture[1] = 0;
    MtrlDof->Texture[2] = 0;
    MtrlDof->Texture[3] = 0;
    MtrlDof->Texture[4] = 0;

    sRTMan->Release(screen);
    sRTMan->Release(down4blur);
    sRTMan->Release(medblur);
    sRTMan->Release(depth4blur);
    sRTMan->Release(dest);
  }
}

/****************************************************************************/

RNSSAO::RNSSAO()
{
  Anim.Init(Wz4RenderType->Script);
  MtrlSSAO = 0;
  MtrlFinish = 0;
}

RNSSAO::~RNSSAO()
{
  delete TexRandom;
  delete MtrlSSAO;
  delete MtrlFinish;
}

void RNSSAO::Init()
{
  Para = ParaBase;

  sRandomMT mt;
  mt.Seed(1234);

  // texture
  static const sInt RandomSize = 4; // match shader!
  TexRandom = new sTexture2D(RandomSize,RandomSize,sTEX_2D|sTEX_ARGB16F|sTEX_NOMIPMAPS);

  sU8 *ptr;
  sInt pitch;
  TexRandom->BeginLoad(ptr,pitch);
  for(sInt y=0;y<RandomSize;y++)
  {
    sHalfFloat *d = (sHalfFloat*) (ptr+y*pitch);
    for(sInt x=0;x<RandomSize;x++)
    {
      sVector30 dir;
      dir.InitRandom(mt);
      dir.Unit();
      d[x*4+0].Set(dir.x);
      d[x*4+1].Set(dir.y);
      d[x*4+2].Set(dir.z);
      d[x*4+3].Set(0.0f);
    }
  }
  TexRandom->EndLoad();

  // material
  Wz4IppSSAOMain *mtrlSSAO = new Wz4IppSSAOMain;
  MtrlSSAO = mtrlSSAO;
  mtrlSSAO->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  mtrlSSAO->TFlags[0] = sMTF_LEVEL1|sMTF_BORDER_WHITE|sMTF_EXTERN;
  mtrlSSAO->TFlags[1] = sMTF_LEVEL1|sMTF_CLAMP|sMTF_EXTERN;
  mtrlSSAO->TFlags[2] = sMTF_LEVEL0|sMTF_EXTERN;
  mtrlSSAO->Texture[2] = TexRandom;
  mtrlSSAO->SampleLevel = sMin(Para.Quality,Doc->EditOptions.ExpensiveIPPQuality);
  mtrlSSAO->FogMode = Para.Fog;
  mtrlSSAO->Prepare(sVertexFormatDouble);

  Wz4IppSSAOFinish *mtrlFinish = new Wz4IppSSAOFinish;
  MtrlFinish = mtrlFinish;
  mtrlFinish->Output = Para.Output;
  mtrlFinish->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  mtrlFinish->TFlags[0] = sMTF_LEVEL0|sMTF_CLAMP|sMTF_EXTERN;
  mtrlFinish->TFlags[1] = sMTF_LEVEL1|sMTF_CLAMP|sMTF_EXTERN;
  mtrlFinish->Prepare(sVertexFormatDouble);

  for(sInt i=0;i<sCOUNTOF(Randoms);i++)
  {
    Randoms[i].InitRandom(mt);
    Randoms[i].Unit();
    Randoms[i] *= (mt.Float(0.5f) + 0.5f);
  }
}

void RNSSAO::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  SimulateChilds(ctx);

  ctx->RenderFlags |= wRF_RenderZ | wRF_RenderZLowres;
}

void RNSSAO::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sCBuffer<Wz4IppVSPara> cbv;
    sCBuffer<Wz4IppSSAOPSPara> cbp;
    sCBuffer<Wz4IppSSAOFinishPSPara> cbfp;

    sTexture2D *screen = sRTMan->ReadScreen();
    sInt sizeX = screen->SizeX, sizeY = screen->SizeY;
    sTexture2D *ssao = sRTMan->Acquire(sizeX,sizeY);

    sRTMan->SetTarget(ssao);
    ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);
    cbv.Modify();

    // pixel parameters
    cbp.Data->Set(ctx->View);
    cbp.Data->occludeAndRadius.Init(Para.MinOcclude,Para.MaxOcclude,Para.SampleRadius,0);
    cbp.Data->uvStep.Init(1.0f/ctx->ZTarget->SizeX,1.0f/ctx->ZTarget->SizeY,0.0f,0.0f);
    cbp.Data->FogMinMaxDens.x = Para.FogFar;
    cbp.Data->FogMinMaxDens.y = -1.0f/(Para.FogFar-Para.FogNear);
    cbp.Data->FogMinMaxDens.z = Para.Intensity;
    cbp.Data->FogMinMaxDens.w = Para.FogDensity;
    switch(Para.Fog)
    {
    case 1:
      cbp.Data->FogCenter.Init(0,0,0,0);
      break;
    case 2:
      cbp.Data->FogCenter = Para.FogCenter*ctx->View.View;
      break;
    }
    for(sInt i=0;i<24;i++)
      cbp.Data->sampleVec[i] = Randoms[i];
    cbp.Modify();

    MtrlSSAO->Texture[0] = ctx->ZTargetLowRes;
    MtrlSSAO->Texture[1] = ctx->ZTarget;
    MtrlSSAO->Set(&cbv,&cbp);
    ctx->IppHelper->DrawQuad(ssao,ctx->ZTargetLowRes,ctx->ZTarget);
    MtrlSSAO->Texture[0] = 0;
    MtrlSSAO->Texture[1] = 0;

    // calculate filter kernel
    static const sF32 radius = 4.0f;
    sF32 ampg[8][8];
    sClear(ampg);

    sF32 sumg=0;
    for(sInt y=0;y<7;y++)
    {
      for(sInt x=0;x<7;x++)
      {
        sF32 dx = 3.0f - x;
        sF32 dy = 3.0f - y;
        ampg[y][x] = sMax<sF32>(0,radius-sSqrt(dx*dx+dy*dy));

        sumg += ampg[y][x];
      }
    }
    for(sInt y=0;y<7;y++)
      for(sInt x=0;x<7;x++)
        ampg[y][x] *= 1/sumg;

    // calculate pixel offsets & colors
    sF32 offx[4][4];
    sF32 offy[4][4];
    sF32 ampp[4][4];

    sF32 sump = 1;
    for(sInt y=0;y<4;y++)
    {
      for(sInt x=0;x<4;x++)
      {
        sF32 dx = x*2-2.0f;
        sF32 dy = y*2-2.0f;

        sF32 i00 = ampg[y*2+0][x*2+0];
        sF32 i01 = ampg[y*2+0][x*2+1];
        sF32 i10 = ampg[y*2+1][x*2+0];
        sF32 i11 = ampg[y*2+1][x*2+1];
        sF32 ito = i00+i01+i10+i11;

        ampp[y][x] = (ito);
        dx += (i01+i11)/ito;
        dy += (i10+i11)/ito;

        offx[y][x] = dx/sizeX;
        offy[y][x] = dy/sizeY;
      }
    }

    //// blur me beautiful
    //sTexture2D *dest = sRTMan->WriteScreen();
    //ctx->IppHelper->BlurFirst(dest,ssao,4.0f,1.0f,sFALSE);

    // finish him!
    sTexture2D *dest = sRTMan->WriteScreen();
    sRTMan->SetTarget(dest);
    ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);
    cbv.Modify();

    sF32 f = 1.0f/sump;
    for(sInt i=0;i<4;i++)
    {
      cbfp.Data->Color[i].Init(ampp[i][0]*f,ampp[i][1]*f,ampp[i][2]*f,ampp[i][3]*f);
      cbfp.Data->Offa[i].Init(offx[i][0],offy[i][0],offx[i][1],offy[i][1]);
      cbfp.Data->Offb[i].Init(offx[i][2],offy[i][2],offx[i][3],offy[i][3]);
    }

    sF32 nt = Para.NormalThreshold;
    cbfp.Data->Tweak.Init(nt,1.0f/(1.0f-nt),1.0f-1.0f/(1.0f-nt),0.0f);
    cbfp.Data->ScrColor.InitColor(Para.Color);
    cbfp.Modify();

    MtrlFinish->Texture[0] = screen;
    MtrlFinish->Texture[1] = ssao;
    MtrlFinish->Set(&cbv,&cbfp);
    ctx->IppHelper->DrawQuad(dest,screen);
    MtrlFinish->Texture[0] = 0;
    MtrlFinish->Texture[1] = 0;

    // cleanup
    sRTMan->Release(screen);
    sRTMan->Release(ssao);
    //sRTMan->Release(blurred);
    sRTMan->Release(dest);
  }
}

/****************************************************************************/

RNColorBalance::RNColorBalance()
{
  Anim.Init(Wz4RenderType->Script);
  Mtrl = new Wz4IppColorBalance();
  Mtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  Mtrl->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  Mtrl->Prepare(sVertexFormatBasic);
}

RNColorBalance::~RNColorBalance()
{
  delete Mtrl;
}

void RNColorBalance::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  SimulateChilds(ctx);
}

void RNColorBalance::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sCBuffer<Wz4IppVSPara> cbv;
    sCBuffer<Wz4IppPSPara> cbp;

    sTexture2D *src = sRTMan->ReadScreen();
    sTexture2D *dest = sRTMan->WriteScreen();
    sRTMan->SetTarget(dest);
    ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);

    static const sF32 sc = 100.0f / 255.0f;

    for(sInt i=0;i<3;i++)
    {
      sF32 sha = Para.Shadows[i];
      sF32 mid = Para.Midtones[i];
      sF32 hil = Para.Highlights[i];
      
      sF32 p = sFPow(0.5f,sha * 0.5f + mid + hil * 0.5f);
      sF32 min = -sMin(sha,0.0f) * sc;
      sF32 max = 1.0f  - sMax(hil,0.0f) * sc;
      sF32 msc = 1.0f / (max - min);

      cbp.Data->Color0[i] = msc;
      cbp.Data->Color1[i] = -min * msc;
      cbp.Data->Color2[i] = p;
    }

    cbp.Data->Color0.w = 1.0f;
    cbp.Data->Color1.w = 0.0f;
    cbp.Data->Color2.w = 1.0f;

    Mtrl->Texture[0] = src;
    Mtrl->Set(&cbv,&cbp);
    ctx->IppHelper->DrawQuad(dest,src);
    Mtrl->Texture[0] = 0;

    sRTMan->Release(src);
    sRTMan->Release(dest);
  }
}

/****************************************************************************/

RNColorSaw::RNColorSaw()
{
  Anim.Init(Wz4RenderType->Script);
  Mtrl = new Wz4IppColorSaw();
  Mtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  Mtrl->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  Mtrl->Prepare(sVertexFormatBasic);
}

RNColorSaw::~RNColorSaw()
{
  delete Mtrl;
}

void RNColorSaw::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);
}

void RNColorSaw::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sCBuffer<Wz4IppVSPara> cbv;
    sCBuffer<Wz4IppPSPara> cbp;

    sTexture2D *src = sRTMan->ReadScreen();
    sTexture2D *dest = sRTMan->WriteScreen();
    sRTMan->SetTarget(dest);
    ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);

    const sF32 d = 0.001f;
 
    cbp.Data->Color0.Init(Para.Repeat[0]-d,Para.Repeat[1]-d,Para.Repeat[2]-d,1);
    cbp.Data->Color1.Init(Para.Gamma[0],Para.Gamma[1],Para.Gamma[2],1);

    sVector30 scale;
    sVector30 add;

    add = Para.Threshold;
    scale.x = 1.0f/sMax(d,Para.Softness.x);
    scale.y = 1.0f/sMax(d,Para.Softness.y);
    scale.z = 1.0f/sMax(d,Para.Softness.z);

    cbp.Data->Color2 = add;
    cbp.Data->Color3 = scale;

    Mtrl->Texture[0] = src;
    Mtrl->Set(&cbv,&cbp);
    ctx->IppHelper->DrawQuad(dest,src);
    Mtrl->Texture[0] = 0;

    sRTMan->Release(src);
    sRTMan->Release(dest);
  }
}

/****************************************************************************/

RNColorGrade::RNColorGrade()
{
  Anim.Init(Wz4RenderType->Script);
  Mtrl = new Wz4IppColorGrade();
  Mtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  Mtrl->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  Mtrl->Prepare(sVertexFormatBasic);

  sMatrix34 mat;

  // rotate grey vector to Z+
  const sF32 rot1=sFRSqrt(2.0f);
  const sF32 rot2=sFRSqrt(3.0f);
  const sF32 rot3=rot2/rot1;
  mat.Init();
  mat.i.x=mat.k.z=rot3; //rc
  mat.i.z=-(mat.k.x=rot2); // rs
  HueMatPre*=mat;
  mat.Init();
  mat.j.y=mat.k.z=rot1;  // rc
  mat.j.z=-(mat.k.y=-rot1); // -rs
  HueMatPre*=mat;

  // shear luminance plane to Z
  sVector30 lumpoint = LumWeights*HueMatPre;
  mat.Init();
  mat.i.Init(1,0,lumpoint.x/lumpoint.z);
  mat.j.Init(0,1,lumpoint.y/lumpoint.z);
  HueMatPre=mat*HueMatPre;

  HueMatPost=HueMatPre;
  HueMatPost.Invert34();
}

RNColorGrade::~RNColorGrade()
{
  delete Mtrl;
}

void RNColorGrade::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);
}

sMatrix34 RNColorGrade::GetContrastMatrix(sF32 contrast)
{
  sMatrix34 out;
  out.Scale(contrast);
  out.l=sVector31(0.5f*(1-contrast));
  return out;
}

sMatrix34 RNColorGrade::GetSaturationMatrix(sF32 sat)
{
  sMatrix34 out, mat;

  /*
  out = HueMatPre;
  mat.RotateAxis(sVector30(0,0,1),sPIF*hue);
  out = mat*out;
  out = HueMatPost*mat;
  */

  out.i=sFade(sat,sVector30(LumWeights.x),out.i);
  out.j=sFade(sat,sVector30(LumWeights.y),out.j);
  out.k=sFade(sat,sVector30(LumWeights.z),out.k);
  return out;
}

sMatrix34 RNColorGrade::GetBalanceMatrix(sF32 hue, sF32 str)
{
  hue=sFMod(hue,1);
  sF32 hx=sFMod(hue+0.333333f,1);
  sVector30 color;
  color.x=sClamp(2.0f-2.0f*sFAbs((hx-0.333333f)*3),0.0f,1.0f);
  color.y=sClamp(2.0f-2.0f*sFAbs((hue-0.333333f)*3),0.0f,1.0f);
  color.z=sClamp(2.0f-2.0f*sFAbs((hue-0.666667f)*3),0.0f,1.0f);

  str=1.0f-str;
  sMatrix34 out;
  out.i=sFade(str,color*sVector30(LumWeights.x),out.i);
  out.j=sFade(str,color*sVector30(LumWeights.y),out.j);
  out.k=sFade(str,color*sVector30(LumWeights.z),out.k);
  return out;
}

void RNColorGrade::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sCBuffer<Wz4IppVSPara> cbv;
    sCBuffer<Wz4IppColorGradePSPara> cbp;

    sTexture2D *src = sRTMan->ReadScreen();
    sTexture2D *dest = sRTMan->WriteScreen();
    sRTMan->SetTarget(dest);
    ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);

    cbp.Data->LumWeights=LumWeights;

    sMatrix34 mat;
    sVector4 col;

    // 1. hue

    // 2. saturation
    mat *= GetSaturationMatrix(Para.MidSaturation);

    // 3. balance
    mat *= GetBalanceMatrix(Para.MidBalHue,Para.MidBalStrength);
    
    // 4. gain/contrast
    mat.Scale(Para.MidGain.x,Para.MidGain.y,Para.MidGain.z);
    mat *= GetContrastMatrix(Para.MidContrast);

    // 5. brightness
    mat.l+=sVector30(Para.MidBrightness);

    // 6. colorize
    col.InitColor(Para.MidColor);
    mat.i*=1.0f-col.w;
    mat.j*=1.0f-col.w;
    mat.k*=1.0f-col.w;
    mat.l=sFade(col.w,mat.l,sVector31(col));
    cbp.Data->MatrixMr.Init(mat.i.x,mat.j.x,mat.k.x,mat.l.x);
    cbp.Data->MatrixMg.Init(mat.i.y,mat.j.y,mat.k.y,mat.l.y);
    cbp.Data->MatrixMb.Init(mat.i.z,mat.j.z,mat.k.z,mat.l.z);

    if (Para.Flags & 1)
    {
      mat.Init();
      cbp.Data->Params1.x = 1.0f/Para.ShadWidth;
      mat *= GetSaturationMatrix(Para.ShadSaturation);
      mat *= GetBalanceMatrix(Para.ShadBalHue,Para.ShadBalStrength);
      mat.Scale(Para.ShadGain.x,Para.ShadGain.y,Para.ShadGain.z);
      mat *= GetContrastMatrix(Para.ShadContrast);
      mat.l+=sVector30(Para.ShadBrightness);
      col.InitColor(Para.ShadColor);
      mat.i*=1.0f-col.w;
      mat.j*=1.0f-col.w;
      mat.k*=1.0f-col.w;
      mat.l=sFade(col.w,mat.l,sVector31(col));
      cbp.Data->MatrixSr.Init(mat.i.x,mat.j.x,mat.k.x,mat.l.x);
      cbp.Data->MatrixSg.Init(mat.i.y,mat.j.y,mat.k.y,mat.l.y);
      cbp.Data->MatrixSb.Init(mat.i.z,mat.j.z,mat.k.z,mat.l.z);

      mat.Init();
      cbp.Data->Params1.y = 1.0f/Para.HighWidth;
      mat *= GetSaturationMatrix(Para.HighSaturation);
      mat *= GetBalanceMatrix(Para.HighBalHue,Para.HighBalStrength);
      mat.Scale(Para.HighGain.x,Para.HighGain.y,Para.HighGain.z);
      mat *= GetContrastMatrix(Para.HighContrast);
      mat.l+=sVector30(Para.HighBrightness);
      col.InitColor(Para.HighColor);
      mat.i*=1.0f-col.w;
      mat.j*=1.0f-col.w;
      mat.k*=1.0f-col.w;
      mat.l=sFade(col.w,mat.l,sVector31(col));
      cbp.Data->MatrixHr.Init(mat.i.x,mat.j.x,mat.k.x,mat.l.x);
      cbp.Data->MatrixHg.Init(mat.i.y,mat.j.y,mat.k.y,mat.l.y);
      cbp.Data->MatrixHb.Init(mat.i.z,mat.j.z,mat.k.z,mat.l.z);

      ((Wz4IppColorGrade*)Mtrl)->UseSMH=sTRUE;
    }
    else
      ((Wz4IppColorGrade*)Mtrl)->UseSMH=sFALSE;

    cbp.Data->VignetteMatrix.Init(4,0,0,4);
    cbp.Data->VignetteShift.Init(4*-0.5,4*-0.5,0,0);

    cbp.Data->Gamma = Para.Gamma*Para.Gamma.w;
    ((Wz4IppColorGrade*)Mtrl)->UseGamma = cbp.Data->Gamma.x!=1.0f || cbp.Data->Gamma.y!=1.0f || cbp.Data->Gamma.z!=1.0f;

    Mtrl->Texture[0] = src;
    Mtrl->Set(&cbv,&cbp);
    ctx->IppHelper->DrawQuad(dest,src);
    Mtrl->Texture[0] = 0;
    Mtrl->Prepare(sVertexFormatSingle);

    sRTMan->Release(src);
    sRTMan->Release(dest);
  }
}

/****************************************************************************/

RNCrashZoom::RNCrashZoom()
{
  Anim.Init(Wz4RenderType->Script);
  Mtrl = new Wz4IppCrashZoom();
  Mtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  Mtrl->TFlags[0] = sMTF_LEVEL2 | sMTF_CLAMP | sMTF_EXTERN;
  Mtrl->Prepare(sVertexFormatBasic);
}

RNCrashZoom::~RNCrashZoom()
{
  delete Mtrl;
}

void RNCrashZoom::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);
}

void RNCrashZoom::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sCBuffer<Wz4IppVSPara> cbv;
    sCBuffer<Wz4IppPSPara> cbp;

    sTexture2D *tex[5];
    sInt n = 0;

    sF32 cx,cy;
    sInt steps = Para.Flags & 15;

    if(Para.Flags & 16)
    {
      ctx->View.Transform(Para.Center3D,cx,cy);
    }
    else
    {
      cx = Para.Center2D[0];
      cy = Para.Center2D[1];
    }

    tex[n++] = sRTMan->ReadScreen();
    for(sInt i=1;i<steps;i++)
      tex[n++] = sRTMan->Acquire(tex[0]->SizeX,tex[0]->SizeY);
    tex[n++] = sRTMan->WriteScreen();

    sF32 amp = Para.Amplify/8;

    for(sInt i=0;i<steps;i++)
    {
      sRTMan->SetTarget(tex[i+1]);
      ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);

      sF32 amount = Para.Amount[i];
      cbp.Modify();
      cbp.Data->Color0.Init(amp,amp,amp,0);
      cbp.Data->Color1.Init(cx,cy,amount,amount);
      amount = amount/4;
      amp = 1.0f/8;

      Mtrl->Texture[0] = tex[i];
      Mtrl->Set(&cbv,&cbp);
      ctx->IppHelper->DrawQuad(tex[i+1],tex[i]);
      Mtrl->Texture[0] = 0;
    }

    for(sInt i=0;i<steps+1;i++)
      sRTMan->Release(tex[i]);
  }
}

/****************************************************************************/

RNGrabScreen::RNGrabScreen()
{
  Anim.Init(Wz4RenderType->Script);
}

RNGrabScreen::~RNGrabScreen()
{
}

void RNGrabScreen::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);
}

void RNGrabScreen::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sTextureBase *s = sRTMan->ReadScreen();
    Target->Acquire(s->SizeX,s->SizeY);
    sCopyTexturePara cp(0,Target->Texture,s);
    sCopyTexture(cp);
    sRTMan->Release(s);
  }
}

/****************************************************************************/

RNCrackFixer::RNCrackFixer()
{
  Anim.Init(Wz4RenderType->Script);
  Mtrl = new Wz4IppCrackFixer();
  Mtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  Mtrl->TFlags[0] = sMTF_LEVEL0 | sMTF_CLAMP | sMTF_EXTERN;
  Mtrl->Prepare(sVertexFormatBasic);
}

RNCrackFixer::~RNCrackFixer()
{
  delete Mtrl;
}

void RNCrackFixer::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);
}

void RNCrackFixer::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sCBuffer<Wz4IppVSPara> cbv;
    sCBuffer<Wz4IppPSPara> cbp;

    sTexture2D *src = sRTMan->ReadScreen();
    sTexture2D *dest = sRTMan->WriteScreen();
    sRTMan->SetTarget(dest);
    ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);

    cbp.Data->Color0.Init(Para.AlphaDistance,Para.OffCounts,0,1);
    cbp.Data->Color1.Init(1.0f/src->SizeX,1.0f/src->SizeY,0,0);

    Mtrl->Texture[0] = src;
    Mtrl->Set(&cbv,&cbp);
    ctx->IppHelper->DrawQuad(dest,src);
    Mtrl->Texture[0] = 0;

    sRTMan->Release(src);
    sRTMan->Release(dest);
  }
}

/****************************************************************************/

RNFXAA3::RNFXAA3()
{
  MtrlPre = new Wz4IppFXAA3Prepare();
  MtrlPre->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  MtrlPre->TFlags[0] = sMTF_LEVEL1 | sMTF_CLAMP | sMTF_EXTERN;
  MtrlPre->Prepare(sVertexFormatBasic);

  MtrlAA = new Wz4IppFXAA3();
  MtrlAA->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  MtrlAA->TFlags[0] = sMTF_LEVEL1 | sMTF_CLAMP | sMTF_EXTERN;
  MtrlAA->Prepare(sVertexFormatBasic);
}

RNFXAA3::~RNFXAA3()
{
  delete MtrlPre;
  delete MtrlAA;
}


void RNFXAA3::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sCBuffer<Wz4IppVSPara> cbv;
    sCBuffer<Wz4IppFXAA3Para> cbp;

    sTexture2D *src = sRTMan->ReadScreen();
    sTexture2D *pre = sRTMan->Acquire(src->SizeX, src->SizeY, src->Flags);
    sTexture2D *dest = sRTMan->WriteScreen();

    sRTMan->SetTarget(pre);
    ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);
    MtrlPre->Texture[0] = src;
    MtrlPre->Set(&cbv);
    ctx->IppHelper->DrawQuad(pre,src);
    MtrlPre->Texture[0] = 0;

    sRTMan->SetTarget(dest);
    cbp.Data->rcpFrame.Init(1.0f/pre->SizeX,1.0f/pre->SizeY,0,0);
    MtrlAA->Texture[0] = pre;
    MtrlAA->Set(&cbv,&cbp);
    ctx->IppHelper->DrawQuad(dest,pre);
    MtrlAA->Texture[0] = 0;

    sRTMan->Release(src);
    sRTMan->Release(pre);
    sRTMan->Release(dest);
  }
}

/****************************************************************************/

RNColorMath::RNColorMath()
{
  Mtrl = new Wz4IppColorMath();
  Mtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  Mtrl->TFlags[0] = sMTF_LEVEL1 | sMTF_CLAMP | sMTF_EXTERN;
  Mtrl->TFlags[1] = sMTF_LEVEL1 | sMTF_CLAMP | sMTF_EXTERN;
  Mtrl->Prepare(sVertexFormatBasic);
}

RNColorMath::~RNColorMath()
{
  delete Mtrl;
}

void RNColorMath::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    Childs[0]->Render(ctx);
    sTexture2D *src = sRTMan->ReadScreen();
    sTexture2D *dest = sRTMan->WriteScreen();

    Mtrl->Texture[0] = src;

    sCBuffer<Wz4IppVSPara> cbv;
    sCBuffer<Wz4IppPSPara> cbp;

    sVector4 c0,c1;
    c0.InitColor(Para.Add);
    c0 = c0 * Para.AddFactor;
    c1.InitColor(Para.Mul);
    c1 = c1 * Para.MulFactor;

    cbp.Data->Color0 = c0;
    cbp.Data->Color1 = c1;
    sRTMan->SetTarget(dest);
    ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);
    Mtrl->Set(&cbv,&cbp);
    ctx->IppHelper->DrawQuad(dest,src);

    Mtrl->Texture[0] = 0;
    
    sRTMan->Release(src);
    sRTMan->Release(dest);
  }


}

/****************************************************************************/

RNCustomIPP::RNCustomIPP()
{
  Anim.Init(Wz4RenderType->Script);
  Mtrl = new Wz4IppCustom();
  Mtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
}

RNCustomIPP::~RNCustomIPP()
{
  delete Mtrl;
}

void RNCustomIPP::Init()
{
  Mtrl->TFlags[0] = sMTF_LEVEL2 | sConvertOldUvFlags(Para.Filter) | sMTF_EXTERN;
  Mtrl->Prepare(sVertexFormatBasic);
}

#include "shadercomp\shadercomp.hpp"
#include "shadercomp\shaderdis.hpp"
sShader *RNCustomIPP::CompileShader(sInt shadertype, const sChar *source)
{
  sString<16> profile;
  switch (shadertype)
  {
    case sSTF_PIXEL|sSTF_HLSL23: profile=L"ps_3_0"; break;
    case sSTF_VERTEX|sSTF_HLSL23: profile=L"vs_3_0"; break;
    default: Log.PrintF(L"unknown shader type %x\n",shadertype); return 0;
  }

  sTextBuffer src2;
  sTextBuffer error;
  sU8 *data = 0;
  sInt size = 0;
  sShader *sh = 0;

  src2.Print(L"uniform float4x4 mvp : register(c0);\n");
  src2.Print(L"uniform float4x4 mv : register(c4);\n");
  src2.Print(L"uniform float3 eye : register(c8);\n");

  switch (shadertype)
  {
      // links to customIPP variables
      // note : these definitions are not implanted (commented here) to give the possibility, in the shader code,
      // to cast the float4 to another type at declaration or define another variable name, ex :
      // uniform float4 ps_var0 : register(c9);   =>    uniform float2 myvar : register(c9);
      // However, dispay these comments lets remember these statements (copy/past) in the debug shader code text field.

    case sSTF_PIXEL|sSTF_HLSL23: profile=L"ps_3_0";
      src2.Print(L"//uniform float4 ps_var0 : register(c9);     // link to ps_var0 (needs to be redefined)\n");
      src2.Print(L"//uniform float4 ps_var1 : register(c10);    // link to ps_var1 (needs to be redefined)\n");
      src2.Print(L"//uniform float4 ps_var2 : register(c11);    // link to ps_var2 (needs to be redefined)\n");
      src2.Print(L"//uniform float4 ps_var3 : register(c12);    // link to ps_var3 (needs to be redefined)\n");
      src2.Print(L"//uniform float4 ps_var4 : register(c13);    // link to ps_var4 (needs to be redefined)\n");
      src2.Print(L"uniform float2 resolution : register(c14);   // screen resolution\n");
      break;

    case sSTF_VERTEX|sSTF_HLSL23: profile=L"vs_3_0";
      src2.Print(L"//uniform float4 vs_var0 : register(c9);     // link to vs_var0 (needs to be redefined)\n");
      src2.Print(L"//uniform float4 vs_var1 : register(c10);    // link to vs_var1 (needs to be redefined)\n");
      src2.Print(L"//uniform float4 vs_var2 : register(c11);    // link to vs_var2 (needs to be redefined)\n");
      src2.Print(L"//uniform float4 vs_var3 : register(c12);    // link to vs_var3 (needs to be redefined)\n");
      src2.Print(L"//uniform float4 vs_var4 : register(c13);    // link to vs_var4 (needs to be redefined)\n");
      break;
  }

  src2.Print(L"\n");
  src2.Print(source);

  if(sShaderCompileDX(src2.Get(),profile,L"main",data,size,sSCF_PREFER_CFLOW|sSCF_DONT_OPTIMIZE,&error))
  {
    Log.PrintF(L"dx: %q\n",error.Get());
    Log.PrintListing(src2.Get());
    Log.Print(L"/****************************************************************************/\n");
#if sRENDERER==sRENDER_DX9
    sPrintShader(Log,(const sU32 *) data,sPSF_NOCOMMENTS);
#endif
    sh = sCreateShaderRaw(shadertype,data,size);
  }
  else
  {
    Log.PrintF(L"dx: %q\n",error.Get());
    Log.PrintListing(src2.Get());
    Log.Print(L"/****************************************************************************/\n");
    sDPrint(Log.Get());
  }
  delete[] data;

  return sh;
}

void RNCustomIPP::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
  SimulateChilds(ctx);
}

void RNCustomIPP::Render(Wz4RenderContext *ctx)
{
  RenderChilds(ctx);

  if((ctx->RenderMode & sRF_TARGET_MASK)==sRF_TARGET_MAIN)
  {
    sCBuffer<Wz4IppVSCustomPara> cbv;
    sCBuffer<Wz4IppPSCustomPara> cbp;

    sTexture2D *src = sRTMan->ReadScreen();
    sTexture2D *dest = sRTMan->WriteScreen();
    sRTMan->SetTarget(dest);

    ctx->IppHelper->GetTargetInfo(cbv.Data->mvp);
    cbv.Data->mvp.Trans4();
    cbv.Data->mv = ctx->View.ModelView;
    cbv.Data->mv.Trans4();
    cbv.Data->eye = ctx->View.Camera.l;

    cbp.Data->mvp = ctx->View.ModelScreen;
    cbp.Data->mvp.Trans4();
    cbp.Data->mv = ctx->View.ModelView;
    cbp.Data->mv.Trans4();
    cbp.Data->eye = ctx->View.Camera.l;

    cbp.Data->resolution.Init(ctx->ScreenX, ctx->ScreenY, 0, 0);

     // bind gui parameters for vertex shader
    cbv.Data->vs_var0.Init(Para.vs_var0[0], Para.vs_var0[1], Para.vs_var0[2], Para.vs_var0[3]);
    cbv.Data->vs_var1.Init(Para.vs_var1[0], Para.vs_var1[1], Para.vs_var1[2], Para.vs_var1[3]);
    cbv.Data->vs_var2.Init(Para.vs_var2[0], Para.vs_var2[1], Para.vs_var2[2], Para.vs_var3[3]);
    cbv.Data->vs_var3.Init(Para.vs_var3[0], Para.vs_var3[1], Para.vs_var3[2], Para.vs_var3[3]);
    cbv.Data->vs_var4.Init(Para.vs_var4[0], Para.vs_var4[1], Para.vs_var4[2], Para.vs_var4[3]);
    cbv.Modify();

    // bind gui parameters for pixel shader
    cbp.Data->ps_var0.Init(Para.ps_var0[0], Para.ps_var0[1], Para.ps_var0[2], Para.ps_var0[3]);
    cbp.Data->ps_var1.Init(Para.ps_var1[0], Para.ps_var1[1], Para.ps_var1[2], Para.ps_var1[3]);
    cbp.Data->ps_var2.Init(Para.ps_var2[0], Para.ps_var2[1], Para.ps_var2[2], Para.ps_var2[3]);
    cbp.Data->ps_var3.Init(Para.ps_var3[0], Para.ps_var3[1], Para.ps_var3[2], Para.ps_var3[3]);
    cbp.Data->ps_var4.Init(Para.ps_var4[0], Para.ps_var4[1], Para.ps_var4[2], Para.ps_var4[3]);
    cbp.Modify();

    Mtrl->Texture[0] = src;
    Mtrl->Set(&cbv,&cbp);
    ctx->IppHelper->DrawQuad(dest,src);
    Mtrl->Texture[0] = 0;

    sRTMan->Release(src);
    sRTMan->Release(dest);
  }
}

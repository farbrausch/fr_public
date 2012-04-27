/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4frlib/wz4_modmtrl.hpp"
#include "wz4frlib/wz4_modmtrlsc.hpp"
#include "wz4frlib/wz4_modmtrl_ops.hpp"
#include "wz4frlib/wz4_modmtrlmod.hpp"
#include "base/graphics.hpp"
#include "shadercomp/shadercomp.hpp"
#include "shadercomp/shaderdis.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Material Type                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void ModMtrlType_::Init()
{
  for(sInt i=0;i<sMAX_LIGHTENV;i++)
  {
    LightEnv[i] = new ModLightEnv;
    LightEnv[i]->Ambient.InitColor(0xff204060);
  }
  RenderShadowsForLightEnv = -1;

  Wz4MtrlType->RegisterMtrl(this);
  sc=new ShaderCreator;

  // generate dummy textures

  DummyTex2D = new Texture2D;
  DummyTexCube = new TextureCube;
  sImage img(8,8);
  img.Fill(0xffff0000);
  DummyTex2D->ConvertFrom(&img,sTEX_ARGB8888|sTEX_2D);
  DummyTexCube->InitDummy();

  // generate random angle sin/cos texture (for rotated disc texture filter sampling)

  sRandomKISS rnd;
  rnd.Seed(3);
  sImage imgsc(64,64);
  for(sInt i=0;i<imgsc.SizeX*imgsc.SizeY;i++)
  {
    sF32 a = rnd.Float(sPI2F);
    sF32 sf = (sSin(a)+1)*127;
    sF32 cf = (sCos(a)+1)*127;
    sInt s = sClamp<sInt>(sInt(sf),0,255);
    sInt c = sClamp<sInt>(sInt(cf),0,255);
    imgsc.Data[i] = (c<<8) | (s<<16); 
  }
  SinCosTex = new Texture2D;
  SinCosTex->ConvertFrom(&imgsc,sTEX_2D|sTEX_ARGB8888|sTEX_NOMIPMAPS);

  // register all parameter.

  sc->RegPara(L"FogMul",SCT_FLOAT,sOFFSET(ModLightEnv,FogMul));
  sc->RegPara(L"FogAdd",SCT_FLOAT,sOFFSET(ModLightEnv,FogAdd));
  sc->RegPara(L"FogDensity",SCT_FLOAT,sOFFSET(ModLightEnv,FogDensity));
  sc->RegPara(L"FogColor",SCT_FLOAT3,sOFFSET(ModLightEnv,FogColor));

  sc->RegPara(L"GroundFogMul",SCT_FLOAT,sOFFSET(ModLightEnv,GroundFogMul));
  sc->RegPara(L"GroundFogAdd",SCT_FLOAT,sOFFSET(ModLightEnv,GroundFogAdd));
  sc->RegPara(L"GroundFogDensity",SCT_FLOAT,sOFFSET(ModLightEnv,GroundFogDensity));
  sc->RegPara(L"GroundFogColor",SCT_FLOAT3,sOFFSET(ModLightEnv,GroundFogColor));
  sc->RegPara(L"ws_GroundFogPlane",SCT_FLOAT4,sOFFSET(ModLightEnv,ws_GroundFogPlane));
  sc->RegPara(L"Clip0",SCT_FLOAT4,sOFFSET(ModLightEnv,Clip0));
  sc->RegPara(L"Clip1",SCT_FLOAT4,sOFFSET(ModLightEnv,Clip1));
  sc->RegPara(L"Clip2",SCT_FLOAT4,sOFFSET(ModLightEnv,Clip2));
  sc->RegPara(L"Clip3",SCT_FLOAT4,sOFFSET(ModLightEnv,Clip3));
  sc->RegPara(L"cs_GroundFogPlane",SCT_FLOAT4,sOFFSET(ModLightEnv,cs_GroundFogPlane));
  sc->RegPara(L"cs_Clip0",SCT_FLOAT4,sOFFSET(ModLightEnv,cs_Clip0));
  sc->RegPara(L"cs_Clip1",SCT_FLOAT4,sOFFSET(ModLightEnv,cs_Clip1));
  sc->RegPara(L"cs_Clip2",SCT_FLOAT4,sOFFSET(ModLightEnv,cs_Clip2));
  sc->RegPara(L"cs_Clip3",SCT_FLOAT4,sOFFSET(ModLightEnv,cs_Clip3));

  // light

  sc->RegPara(L"Ambient",SCT_FLOAT3,sOFFSET(ModLightEnv,Ambient));
  for(sInt i=0;i<MM_MaxLight;i++)
  {
    sc->RegPara(sPoolF(L"LightColFront%d",i),SCT_FLOAT3,sOFFSET(ModLightEnv,Lights[i].ColFront));
    sc->RegPara(sPoolF(L"LightColMiddle%d",i),SCT_FLOAT3,sOFFSET(ModLightEnv,Lights[i].ColMiddle));
    sc->RegPara(sPoolF(L"LightColBack%d",i),SCT_FLOAT3,sOFFSET(ModLightEnv,Lights[i].ColBack));
    sc->RegPara(sPoolF(L"ws_LightPos%d",i),SCT_FLOAT3,sOFFSET(ModLightEnv,Lights[i].ws_Pos));
    sc->RegPara(sPoolF(L"ws_LightDir%d",i),SCT_FLOAT3,sOFFSET(ModLightEnv,Lights[i].ws_Dir));

    sc->RegPara(sPoolF(L"LightRange%d",i),SCT_FLOAT,sOFFSET(ModLightEnv,Lights[i].Range));
    sc->RegPara(sPoolF(L"LightInner%d",i),SCT_FLOAT,sOFFSET(ModLightEnv,Lights[i].Inner));
    sc->RegPara(sPoolF(L"LightOuter%d",i),SCT_FLOAT,sOFFSET(ModLightEnv,Lights[i].Outer));
    sc->RegPara(sPoolF(L"LightFalloff%d",i),SCT_FLOAT,sOFFSET(ModLightEnv,Lights[i].Falloff));
    sc->RegPara(sPoolF(L"SM_BaseBias%d",i),SCT_FLOAT,sOFFSET(ModLightEnv,Lights[i].SM_BaseBias));
    sc->RegPara(sPoolF(L"SM_Filter%d",i),SCT_FLOAT,sOFFSET(ModLightEnv,Lights[i].SM_Filter));
    sc->RegPara(sPoolF(L"SM_SlopeBias%d",i),SCT_FLOAT,sOFFSET(ModLightEnv,Lights[i].SM_SlopeBias));
    sc->RegPara(sPoolF(L"SM_BaseBiasOverClipFar%d",i),SCT_FLOAT,sOFFSET(ModLightEnv,Lights[i].SM_BaseBiasOverClipFar));
    sc->RegPara(sPoolF(L"SM_SlopeBiasOverClipFar%d",i),SCT_FLOAT,sOFFSET(ModLightEnv,Lights[i].SM_SlopeBiasOverClipFar));
  }
  for(sInt i=0;i<8;i++)
  {
    sc->RegPara(sPoolF(L"Color%d",i),SCT_FLOAT3,sOFFSET(ModLightEnv,Color[i]));
    sc->RegPara(sPoolF(L"Vector%d",i),SCT_FLOAT4,sOFFSET(ModLightEnv,Vector[i]));
  }
  for(sInt i=0;i<4;i++)
  {
    sc->RegPara(sPoolF(L"Matrix%d",i),SCT_FLOAT3x4,sOFFSET(ModLightEnv,Matrix[i]));
  }
  for(sInt i=0;i<sMTRL_MAXTEX;i++)
  {
    sc->RegPara(sPoolF(L"LightProj%d",i),SCT_FLOAT4x4,sOFFSET(ModLightEnv,LightProj[i]));
    sc->RegPara(sPoolF(L"LightMatZ%d",i),SCT_FLOAT4,sOFFSET(ModLightEnv,LightMatZ[i]));
    sc->RegPara(sPoolF(L"LightFarR%d",i),SCT_FLOAT,sOFFSET(ModLightEnv,LightFarR[i]));
  }
  
  // internal

  sc->RegPara(L"ws_ss",SCT_FLOAT4x4,sOFFSET(ModLightEnv,ws_ss));
  sc->RegPara(L"ws_cs",SCT_FLOAT3x4,sOFFSET(ModLightEnv,ws_cs));
  sc->RegPara(L"ws_campos",SCT_FLOAT3,sOFFSET(ModLightEnv,ws_campos));
  sc->RegPara(L"ClipFarR",SCT_FLOAT,sOFFSET(ModLightEnv,ClipFarR));
  sc->RegPara(L"ZShaderSlopeBias",SCT_FLOAT,sOFFSET(ModLightEnv,ZShaderSlopeBias));
  sc->RegPara(L"ZShaderBaseBiasOverClipFar",SCT_FLOAT,sOFFSET(ModLightEnv,ZShaderBaseBiasOverClipFar));
  sc->RegPara(L"ZShaderProjA",SCT_FLOAT,sOFFSET(ModLightEnv,ZShaderProjA));
  sc->RegPara(L"ZShaderProjB",SCT_FLOAT,sOFFSET(ModLightEnv,ZShaderProjB));
  sc->RegPara(L"RandomDisc",SCT_FLOAT4A4,sOFFSET(ModLightEnv,RandomDisc));
  sc->RegPara(L"ShellExtrude",SCT_FLOAT,sOFFSET(ModLightEnv,ShellExtrude));
}

void ModMtrlType_::Exit()
{
  for(sInt i=0;i<sMAX_LIGHTENV;i++)
    delete LightEnv[i];
  sRelease(DummyTex2D);
  sRelease(DummyTexCube);
  sRelease(SinCosTex);

  delete sc;
}

void ModMtrlType_::PrepareViewR(sViewport &view)
{
  for(sInt i=0;i<sMAX_LIGHTENV;i++)
  {
    ModLightEnv *le = LightEnv[i];
    le->Calc(view);
    le->cbv.Modify();
    le->cbp.Modify();
  }
}

/****************************************************************************/

void ModMtrlType_::PrepareRenderR(Wz4RenderContext *ctx)
{
  for(sInt i=0;i<sMAX_LIGHTENV;i++)
  {
    LightEnv[i]->RequireShadow = 0;
    for(sInt j=0;j<MM_MaxLight;j++)
      LightEnv[i]->Lights[j].ShadowReceiver.Clear();
    LightEnv[i]->ShadowCaster.Clear();
  }

  RenderShadowsForLightEnv = -1;
  ShadowCaster.Clear();
  ViewFrustum = ctx->Frustum;
}

void ModMtrlType_::BeginRenderR(Wz4RenderContext *ctx)
{
  for(sInt i=0;i<sMAX_LIGHTENV;i++)
  {
    RenderShadowsForLightEnv = i;
    ModLightEnv *le = LightEnv[i];
    for(sInt j=0;j<MM_MaxLight;j++)
    {
      ModLightEnv::Light *l = &le->Lights[j];
      sInt bit = 1<<j;
      if((le->Shadow & bit) && (le->RequireShadow & bit))
      {
        sTexture2D *colormap = 0;
        if(le->PointLight & bit)
        {
          // allocate cubemap

          sInt sxy = 1<<(l->SM_Size&15);

          l->ShadowCube = sRTMan->AcquireCube(sxy,sTEX_R16F);
          colormap = sRTMan->Acquire(sxy,sxy,sTEX_DEPTH16NOREAD);       // colormap is the depth buffer. sorry for the naming

          for(sInt i=0;i<6;i++)
          {
            // set rendertarget

            sTargetPara para(sST_CLEARALL,0xffffffff,0,l->ShadowCube,colormap);
            para.Cubeface = i;
            if(!ctx->DisableShadowFlag)
              sSetTarget(para);

            // calculate viewport

            sViewport view;
            view.SetTargetCurrent();
            view.Camera.CubeFace(i);
            view.Camera.l = l->ws_Pos;
            view.ClipNear = 1.0f/16;
            view.ClipFar = l->Range;
            view.ZoomX = view.ZoomY = 1;
            view.Prepare();


            le->ZShaderSlopeBias = l->SM_SlopeBias;
            le->ZShaderBaseBiasOverClipFar = l->SM_BaseBias / l->Range;
            // render

            if(!ctx->DisableShadowFlag)
            {
              ctx->PrepareView(view);
              ctx->RenderMode = sRF_TARGET_DIST|sRF_HINT_POINTSHADOW;
              ctx->Root->Render(ctx);
              ctx->NextRecMask();
            }
          }

          sMatrix44 mat;
          mat.i.x = 0.5f;
          mat.j.y = -0.5f;
          mat.k.z = 1.0f;
          mat.l.Init(0.5f,0.5f,-l->SM_BaseBias*0.001,1.0f);

          l->LightFarR = 1.0f / l->Range;
        }
        else if(le->SpotLight & bit)
        {

          // set rendertarget

          sInt sxy = 1<<(l->SM_Size&15);
          l->ShadowMap = sRTMan->Acquire(sxy,sxy,sTEX_PCF16);
          colormap = sRTMan->Acquire(sxy,sxy,sTEX_R16F);
          if(!ctx->DisableShadowFlag)
            sSetTarget(sTargetPara(sST_CLEARALL,0,0,colormap,l->ShadowMap));

          // calculate viewport

          sViewport view;
          view.SetTargetCurrent();
          view.Camera.LookAt(l->ws_Pos-l->ws_Dir,l->ws_Pos);
          view.ClipNear = 1.0f/16;
          view.ClipFar = l->Range;
          view.ZoomX = view.ZoomY = sTan(sPIF*0.5-sACos(l->Outer));
          view.Prepare();

          // adjust for uv 

          sMatrix44 mat;
          mat.i.x = 0.5f;
          mat.j.y = -0.5f;
          mat.k.z = 1.0f;
          mat.l.Init(0.5f+0.5f/sxy,0.5f+0.5f/sxy,-l->SM_BaseBias*0.001,1.0f);
          l->LightProj = view.ModelScreen*mat;
/*
          l->LightFarR = 1.0f / view.ClipFar;
          l->LightMatZ.x = view.View.i.z;
          l->LightMatZ.y = view.View.j.z;
          l->LightMatZ.z = view.View.k.z;
          l->LightMatZ.w = view.View.l.z;
          l->LightMatZ *= l->LightFarR;
          le->ZShaderSlopeBias = 0;
          le->ZShaderBaseBiasOverClipFar = 0;
*/
          // render

          if(!ctx->DisableShadowFlag)
          {
            ctx->PrepareView(view);
            ctx->RenderMode = sRF_TARGET_ZONLY|sRF_HINT_POINTSHADOW;
            ctx->Root->Render(ctx);
            ctx->NextRecMask();
          }
        }
        else if(le->DirLight & bit)
        {
          sMatrix34 rot,roti;
          sVector31 v[8];
          sAABBox bounds,sum;

          // project bbox to right direction

          rot.LookAt(sVector31(l->ws_Dir*-16),sVector31(0,0,0));
          roti = rot;
          roti.InvertOrthogonal();

          // shadow receivers determine size of area

          bounds = l->ShadowReceiver;
          if(le->LimitShadow & bit)
          {
            sAABBox limit;
            limit.Min = le->LimitShadowCenter-le->LimitShadowRadius;
            limit.Max = le->LimitShadowCenter+le->LimitShadowRadius;
            bounds.And(limit);
          }
          bounds.MakePoints(v);
          sum.Clear();
          for(sInt i=0;i<8;i++)
            sum.Add(v[i]*roti);

          sF32 _zoomx = sum.Max.x - sum.Min.x;
          sF32 _zoomy = sum.Max.y - sum.Min.y;
          sF32 _maxz = sum.Max.z;
          sVector31 _center = sum.Center();

          // shadow casters determine how far back we need the camera

          bounds.Add(ModMtrlType->ShadowCaster);
          bounds.Add(le->ShadowCaster);
          bounds.MakePoints(v);
          sum.Clear();
          for(sInt i=0;i<8;i++)
            sum.Add(v[i]*roti);

          sF32 _minz = sum.Min.z;
          sF32 _dist = _center.z - _minz + 1;
          sF32 _depth = _maxz - _minz + 2;

          // set rendertarget

          sInt sxy = 1<<(l->SM_Size&15);
          l->ShadowMap = sRTMan->Acquire(sxy,sxy,sTEX_PCF16);
          colormap = sRTMan->Acquire(sxy,sxy,sTEX_R16F);
          if(!ctx->DisableShadowFlag)
            sSetTarget(sTargetPara(sST_CLEARALL,0,0,colormap,l->ShadowMap));

          // set up the right matrix         

          sViewport view;
          view.Camera.LookAt(_center*rot,_center*rot+l->ws_Dir*_dist);
          view.Orthogonal = sVO_ORTHOGONAL;
          view.ClipNear = 0;
          view.ClipFar = _depth;
          view.SetTargetCurrent();
          view.ZoomX = 2/_zoomx;
          view.ZoomY = 2/_zoomy;
          view.Prepare();

          // adjust for uv 

          sMatrix44 mat;
          mat.i.x = 0.5f;
          mat.j.y = -0.5f;
          mat.k.z = 1.0f;
          mat.l.Init(0.5f+0.5f/sxy,0.5f+0.5f/sxy,-l->SM_BaseBias/view.ClipFar,1.0f);
          l->LightProj = view.ModelScreen*mat;
/*
          l->LightFarR = 1.0f / view.ClipFar;
          l->LightMatZ.x = view.View.i.z;
          l->LightMatZ.y = view.View.j.z;
          l->LightMatZ.z = view.View.k.z;
          l->LightMatZ.w = view.View.l.z;
          l->LightMatZ *= l->LightFarR;
          le->ZShaderSlopeBias = 0;
          le->ZShaderBaseBiasOverClipFar = 0;
*/
          // render

          if(!ctx->DisableShadowFlag)
          {
            ctx->PrepareView(view);
            ctx->RenderMode = sRF_TARGET_ZONLY|sRF_HINT_DIRSHADOW;
            ctx->Root->Render(ctx);
            ctx->NextRecMask();
          }
        }
        else 
        {
          sVERIFYFALSE;
        }
        sRTMan->Release(colormap);
      }
    }
  }
  RenderShadowsForLightEnv = -1;
}

void ModMtrlType_::EndRenderR(Wz4RenderContext *ctx)
{
  for(sInt i=0;i<sMAX_LIGHTENV;i++)
  {
    for(sInt j=0;j<MM_MaxLight;j++)
    {
      sRTMan->Release(LightEnv[i]->Lights[j].ShadowMap);
      LightEnv[i]->Lights[j].ShadowMap = 0;
      sRTMan->Release(LightEnv[i]->Lights[j].ShadowCube);
      LightEnv[i]->Lights[j].ShadowCube = 0;
    }
  }
}

void ModMtrlType_::BeginShow(wPaintInfo &pi)
{
  if(pi.View)   // sometimes, there is no viewport!
  {
    for(sInt i=0;i<sMAX_LIGHTENV;i++)
    {
      LightEnv[i]->Init();
  //    if(pi.CamOverride)
        LightEnv[i]->Lights[0].ws_Dir = -pi.View->Camera.k;
    }
  }
}

/****************************************************************************/

ModLightEnv::ModLightEnv()
{
  Init();

  sRandomKISS rnd;
  rnd.Seed(6);
  sF32 *dest = &RandomDisc[0].x;
  sF32 xsum=0,ysum=0;
  for(sInt i=0;i<8*2;i++)
  {
    sF32 x,y;
    do
    {
      x = rnd.Float(2)-1;
      y = rnd.Float(2)-1;
    }
    while(x*x+y*y<1.0f);
    xsum += x;
    ysum += y;
    dest[0+i*2] = x;
    dest[1+i*2] = y;
  }
  for(sInt i=0;i<8;i++)
  {
    dest[0+i*2] -= xsum/2;
    dest[1+i*2] -= ysum/2;
  }

}

// initialize everything again that is not initialized by Calc()

void ModLightEnv::Init()        
{
  FogAdd = 0;
  FogMul = 0;
  FogDensity = 0;
  FogColor.Init(1,1,1);

  GroundFogAdd = 0;
  GroundFogMul = 0;
  GroundFogDensity = 0;
  GroundFogColor.Init(1,1,1);
  ws_GroundFogPlane.Init(0,0,0,1);

  Clip0.Init(0,0,0,1);
  Clip1.Init(0,0,0,1);
  Clip2.Init(0,0,0,1);
  Clip3.Init(0,0,0,1);

  Ambient.Init(0,0,0);

  for(sInt i=0;i<MM_MaxLight;i++)
  {
    Lights[i].ColFront.Init(0,0,0);
    Lights[i].ColMiddle.Init(0,0,0);
    Lights[i].ColBack.Init(0,0,0);
    Lights[i].ws_Pos.Init(0,0,0);
    Lights[i].ws_Dir.Init(0,1,0);
    Lights[i].Range = 0;
    Lights[i].Inner = 0;
    Lights[i].Outer = 0;
    Lights[i].Falloff = 0;
    Lights[i].SM_Size = 8;
    Lights[i].SM_BaseBias = 0;
    Lights[i].SM_Filter = 0;
    Lights[i].SM_SlopeBias = 0;
    Lights[i].SM_BaseBiasOverClipFar = 0;
    Lights[i].SM_SlopeBiasOverClipFar = 0;
    Lights[i].ShadowMap = 0;
    Lights[i].ShadowCube = 0;
    Lights[i].LightProj.Init();
    Lights[i].LightMatZ.Init(0,0,0,0);
    Lights[i].LightFarR = 0;
    Lights[i].ShadowReceiver.Clear();
    Lights[i].Mode = 0;
  }
  Lights[0].ColFront.Init(1,1,1);
  Lights[0].Mode = 1;

  Features = 0;
  DirLight = 1;
  PointLight = 0;
  SpotLight = 0;
  Shadow = 0;
  ShadowOrd = 0;
  ShadowRand = 0;
  SpotInner = 0;
  SpotFalloff = 0;
  RequireShadow = 0;
  LimitShadow = 0;
  LimitShadowFlags = 0;

  for(sInt i=0;i<8;i++)
  {
    Color[i].Init(0,0,0);
    Vector[i].Init(0,0,0,0);
    Matrix[i].Init();
  }

  for(sInt i=0;i<sMTRL_MAXTEX;i++)
  {
    LightProj[i].Init();
    LightMatZ[i].Init(0,0,0,0);
    LightFarR[i] = 0;
  }

  ZShaderSlopeBias = 0;
  ZShaderBaseBiasOverClipFar = 0;
  ZShaderProjA = 0;
  ZShaderProjB = 0;
  ClipFarR = 0;
  ShellExtrude = 0;
}

void ModLightEnv::Calc(sViewport &view)
{
  ws_ss = view.ModelScreen;
  ws_cs = view.View;
  cs_ws = view.Camera;
  ws_campos = view.Camera.l;

  ClipFarR = 1.0f/view.ClipFar;
  for(sInt i=0;i<MM_MaxLight;i++)
  {
    Lights[i].SM_BaseBiasOverClipFar = Lights[i].SM_BaseBias * ClipFarR;
    Lights[i].SM_SlopeBiasOverClipFar = Lights[i].SM_SlopeBias * ClipFarR;
    if(Lights[i].Mode & 0x80)
    {
      Lights[i].ws_Dir = Lights[i].ws_Dir_ * cs_ws;
      Lights[i].ws_Pos = Lights[i].ws_Pos_ * cs_ws;
    }
  }

  sMatrix44 mat;
  mat = view.Camera;
  mat.Trans4();

  cs_GroundFogPlane = ws_GroundFogPlane*mat; 
  cs_Clip0 = Clip0*mat;
  cs_Clip1 = Clip1*mat;
  cs_Clip2 = Clip2*mat;
  cs_Clip3 = Clip3*mat;

  ZShaderProjA = view.Proj.k.z;
  ZShaderProjB = view.Proj.l.z/view.ClipFar;
}

/****************************************************************************/
/***                                                                      ***/
/***   Graphics Material                                                  ***/
/***                                                                      ***/
/****************************************************************************/

class ModMtrl2 : public sMaterial
{
public:
  void SelectShaders(sVertexFormatHandle *vf)
  {
  }
};

/****************************************************************************/

CachedModInfo::CachedModInfo()
{
  sClear(*this);
}

CachedModInfo::CachedModInfo(sInt renderflags,class ModMtrl *mtrl,sInt lightenv)
{
  sClear(*this);
  ModLightEnv *le = ModMtrlType->LightEnv[lightenv];

  MatrixMode = renderflags & sRF_MATRIX_MASK;
  RenderTarget = renderflags & sRF_TARGET_MASK;
  LightEnv = lightenv;
  FeatureFlags = mtrl->FeatureFlags;

  Features = le->Features        & mtrl->KillFeatures;
  DirLight = le->DirLight        & mtrl->KillLight;
  PointLight = le->PointLight    & mtrl->KillLight;
  SpotLight = le->SpotLight      & mtrl->KillLight;
  Shadow = le->Shadow            & mtrl->KillLight & mtrl->KillShadow; 
  ShadowOrd = le->ShadowOrd      & Shadow;
  ShadowRand = le->ShadowRand    & Shadow;
  SpotInner = le->SpotInner      & mtrl->KillLight;
  SpotFalloff = le->SpotFalloff  & mtrl->KillLight;
}

bool operator == (const CachedModInfo &a,const CachedModInfo &b)
{
  return sCmpMem(&a,&b,sizeof(CachedModInfo))==0;
}

/****************************************************************************/

CachedModShader::CachedModShader()
{
  Shader = 0;
  Format = 0;
  sClear(UpdateTex);
}

CachedModShader::~CachedModShader()
{
  delete Shader;
}

/****************************************************************************/
/***                                                                      ***/
/***   Parameter Assignment                                               ***/
/***                                                                      ***/
/****************************************************************************/

ModMtrlParaAssign::ModMtrlParaAssign()
{
  Alloc = 0;
  MapEnd = 0;
  MapMax = 0;
  Error = 0;
}

ModMtrlParaAssign::~ModMtrlParaAssign()
{
  delete[] Alloc;
}

void ModMtrlParaAssign::Init(sInt max)
{
  delete[] Alloc;
  Alloc = new sInt[max];
  for(sInt i=0;i<max;i++)
    Alloc[i] = 0;
  MapMax = max;
  MapEnd = 0;
  Error = 0;
  CopyLoop.Clear();
}

sInt ModMtrlParaAssign::Assign(sInt count,sInt index)
{
  ModPara *p;
//  sVERIFY(count<=16 && count>=1);
  p = CopyLoop.AddMany(1);
  p->SourceOffset = index;
  p->Count = count;

  if(count<4)
  {
    // find perfect fit

    for(sInt i=0;i<MapEnd;i++)
    {
      if(Alloc[i]+count==4)
      {
        p->DestOffset = i*4+Alloc[i];
        Alloc[i]+=count;
        return p->DestOffset;
      }
    }

    // find any fit

    for(sInt i=0;i<MapEnd;i++)
    {
      if(Alloc[i]+count<=4)
      {
        p->DestOffset= i*4+Alloc[i];
        Alloc[i]+=count;
        return p->DestOffset;
      }
    }
  }

  // add to end

  if(MapEnd==MapMax)
  {
    Error = 1;
    MapEnd--;
  }

  p->DestOffset = MapEnd*4;
  if(count>4)
  {
    for(sInt i=0;i<count;i+=4)
      Alloc[MapEnd++] = 4;
  }
  else
  {
    Alloc[MapEnd++] = count;
  }
  return p->DestOffset;
}

void ModMtrlParaAssign::Copy(sU32 *src,sU32 *dest)
{
  ModPara *p;
  sFORALL(CopyLoop,p)
    for(sInt i=0;i<p->Count;i++)
      dest[p->DestOffset+i] = src[p->SourceOffset+i];
}

/****************************************************************************/
/***                                                                      ***/
/***   Base Shader                                                        ***/
/***                                                                      ***/
/****************************************************************************/

ModMtrl::ModMtrl()
{
  Type = ModMtrlType;
  Flags = 0;
  Error = 0;

  BlendColor = 0;
  BlendAlpha = 0;
  AlphaTest = 7;
  AlphaRef = 0;

  KillLight = 0;
  KillShadow = 0;
  KillFeatures = 0;
  FeatureFlags = 0;

  PageX = 0;
  PageY = 0;
  PageName = L"unknown";
}

ModMtrl::~ModMtrl()
{
  sReleaseAll(ModulesUser);
  sDeleteAll(Cache);
}

void ModMtrl::BeforeFrame(sInt lightenv,sInt boxcount,const sAABBoxC *boxes,sInt matcount=0,const sMatrix34CM *mats=0)
{
  ModLightEnv *le = ModMtrlType->LightEnv[lightenv];
  le->RequireShadow |= KillShadow;
  sFrustum fr;

  sInt bits = (le->Shadow&KillShadow);
  for(sInt i=0;i<MM_MaxLight && bits;i++)
  {
    if(bits & (1<<i))
    {
      for(sInt m=0;m<matcount;m++)
      {
        fr.Transform(ModMtrlType->ViewFrustum,mats[m]);
        for(sInt b=0;b<boxcount;b++)
          if(fr.IsInside(boxes[b]))
            le->Lights[i].ShadowReceiver.Add(boxes[b],mats[m]);
      }
    }

    bits &= ~(1<<i);
  }
  if((FeatureFlags & MMFF_ShadowCastOff)==0)
  {
    if((FeatureFlags & MMFF_ShadowCastToAll))               // cast shadows in all light environemnts
    {
      for(sInt m=0;m<matcount;m++)
      {
        fr.Transform(ModMtrlType->ViewFrustum,mats[m]);
        for(sInt b=0;b<boxcount;b++)
          ModMtrlType->ShadowCaster.Add(boxes[b],mats[m]);
      }
    }
    else                                                    // cast shadows in my own light environment
    {
      for(sInt m=0;m<matcount;m++)
      {
        fr.Transform(ModMtrlType->ViewFrustum,mats[m]);
        for(sInt b=0;b<boxcount;b++)
          le->ShadowCaster.Add(boxes[b],mats[m]);
      }
    }
  }
}

void ModMtrl::Set(sInt flags,sInt index,const sMatrix34CM *mat,sInt SkinMatCount,const sMatrix34CM *SkinMats,sInt *SkinMatMap)
{  
  switch(flags & sRF_TARGET_MASK)
  {
  case sRF_TARGET_WIRE:
    Wz4MtrlType->SetDefaultShader(flags|sRF_MATRIX_ONE,mat,0,SkinMatCount,SkinMats,SkinMatMap);
    break;
/*
  case sRF_TARGET_ZNORMAL:
    {
      sInt variant = 0;
      sF32 det = 1;
      if(mat)
        det = mat->Determinant3x3();
      switch(Flags & sMTRL_CULLMASK)
      {
      case sMTRL_CULLON:  variant = det<0 ? 2 : 1; break;
      case sMTRL_CULLINV: variant = det<0 ? 1 : 2; break;
      }

      Wz4MtrlType->SetDefaultShader(flags,mat,variant,SkinMatCount,SkinMats,SkinMatMap);
    }
    break;
*/
  case sRF_TARGET_MAIN:
  case sRF_TARGET_DIST:
  case sRF_TARGET_ZONLY:
  case sRF_TARGET_ZNORMAL:
    if(Error)
    {
      sInt variant = 0;
      sF32 det = 1;
      if(mat)
        det = mat->Determinant3x3();
      switch(Flags & sMTRL_CULLMASK)
      {
      case sMTRL_CULLON:  variant = det<0 ? 2 : 1; break;
      case sMTRL_CULLINV: variant = det<0 ? 1 : 2; break;
      }

      Wz4MtrlType->SetDefaultShader(flags,mat,variant,SkinMatCount,SkinMats,SkinMatMap);
    }
    else
    {
      ModLightEnv *env = ModMtrlType->LightEnv[index];

      env->ShellExtrude = ShellExtrude;

      sCBufferBase *modelcb;
      if(mat)
      {
        env->cbm.Data->m[0] = mat->x;
        env->cbm.Data->m[1] = mat->y;
        env->cbm.Data->m[2] = mat->z;
        env->cbm.Modify();
        modelcb = &env->cbm;
      }
      else
      {
        modelcb = &Wz4MtrlType->IdentCBModel;
      }

      CachedModInfo info(flags,this,index);
      CachedModShader *cm = FindShader(info);
      ModMtrl2 *shader = cm->Shader; 


      for(sInt i=0;i<sMTRL_MAXTEX;i++)
      {
        if(cm->UpdateTex[i].Enable)
        {
          ModLightEnv::Light *l = &env->Lights[cm->UpdateTex[i].Light];
          env->LightProj[i] = l->LightProj;
          env->LightMatZ[i] = l->LightMatZ;
          env->LightFarR[i] = l->LightFarR;
        }
      }

      sCBufferBase *cbptr[4];
      cbptr[0] = &env->cbv;
      cbptr[1] = modelcb;
      cbptr[2] = &env->cbp;
      cbptr[3] = Wz4MtrlType->MakeSkinCB(SkinMatCount,SkinMats,SkinMatMap,2);

      sF32 det = 1;
      if(mat)
        det = mat->Determinant3x3();
      sInt variant = det>0 ? 0 : 1;

      cm->PSPara.Copy((sU32 *)env,(sU32 *)env->cbp.Data);
      cm->VSPara.Copy((sU32 *)env,(sU32 *)env->cbv.Data);
      env->cbp.Modify();
      env->cbv.Modify();


      for(sInt i=0;i<sMTRL_MAXTEX;i++)
      {
        if(cm->UpdateTex[i].Enable)
        {
          ModLightEnv::Light *l = &env->Lights[cm->UpdateTex[i].Light];
          cm->UpdateTex[i].Save = shader->Texture[i];
          if(l->ShadowMap)
            shader->Texture[i] = l->ShadowMap;
          else 
            shader->Texture[i] = l->ShadowCube;
        }
      }

      shader->Set(cbptr,4,variant);

      for(sInt i=0;i<sMTRL_MAXTEX;i++)
      {
        if(cm->UpdateTex[i].Enable)
        {
          shader->Texture[i] = cm->UpdateTex[i].Save;
          cm->UpdateTex[i].Save = 0;
        }
      }
    }
    break;
  }
}

void ModMtrl::SetMtrl(sInt flags,sU32 blendc,sU32 blenda)
{
  Flags = flags;
  BlendColor = blendc;
  BlendAlpha = blenda;
}

void ModMtrl::SetAlphaTest(sInt test,sInt ref)
{
  AlphaTest = test;
  AlphaRef = ref;
}

void ModMtrl::Prepare()
{
  // prepare modules

  MtrlModule *mod;

  sArray<MtrlModule *> tmod;
  tmod.HintSize(ModulesUser.GetCount());

  sFORALL(ModulesUser,mod)
    mod->Temp = 0;
  sFORALL(ModulesUser,mod)
  {
    if(mod->Temp==0)
      tmod.AddTail(mod);
    mod->Temp++;
  }
  sAddRefAll(tmod);
  sReleaseAll(ModulesUser);
  ModulesUser.Add(tmod);

  sSortUp(ModulesUser,&MtrlModule::Phase);
}

sVertexFormatHandle *ModMtrl::GetFormatHandle(sInt flags)
{
  switch(flags & sRF_TARGET_MASK)
  {
  case sRF_TARGET_MAIN:
  case sRF_TARGET_DIST:
  case sRF_TARGET_ZONLY:
  case sRF_TARGET_ZNORMAL:
    {
      CachedModInfo info(flags,this,0);
      return FindShader(info)->Format;
    }
  default:
  case sRF_TARGET_WIRE:
    return Wz4MtrlType->GetDefaultFormat(flags);
  }
}

sBool ModMtrl::SkipPhase(sInt flags,sInt lightenv)
{
  if( ( (flags & sRF_TARGET_MASK)==sRF_TARGET_ZONLY || 
        (flags & sRF_TARGET_MASK)==sRF_TARGET_ZNORMAL || 
        (flags & sRF_TARGET_MASK)==sRF_TARGET_DIST
      ) && (Flags & sMTRL_NOZRENDER)) return 1;
  if( ( (flags & sRF_TARGET_MASK)==sRF_TARGET_ZONLY ||
        (flags & sRF_TARGET_MASK)==sRF_TARGET_DIST
      ) ) 
  {
    if(FeatureFlags & MMFF_ShadowCastOff)
      return 1;
    if(!(FeatureFlags & MMFF_ShadowCastToAll) && lightenv!=ModMtrlType->RenderShadowsForLightEnv)
      return 1;
      
  }
  return 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Codegen                                                            ***/
/***                                                                      ***/
/****************************************************************************/

CachedModShader *ModMtrl::FindShader(const CachedModInfo &info)
{
  CachedModShader *mod;
  mod = sFind(Cache,&CachedModShader::Info,info);
  if(mod==0)
  {
    mod = CreateShader(info);
    Cache.AddTail(mod);
  }
  return mod;
}

CachedModShader *ModMtrl::CreateShader(const CachedModInfo &info)
{
  ShaderCreator *sc = ModMtrlType->sc;
  sArray<MtrlModule *> ModulesNew;
  sInt ShaderLogEnd = ShaderLog.GetCount();

  if(info.DirLight)
  {
    MM_DirLight *mod = new MM_DirLight;
    mod->Select = info.DirLight;
    ModulesNew.AddTail(mod);
  }
  if(info.PointLight)
  {
    MM_PointLight *mod = new MM_PointLight;
    mod->Select = info.PointLight;
    ModulesNew.AddTail(mod);
  }
  if(info.SpotLight)
  {
    MM_SpotLight *mod = new MM_SpotLight;
    mod->Select = info.SpotLight;
    mod->Falloff = info.SpotFalloff;
    mod->Inner = info.SpotInner;
    ModulesNew.AddTail(mod);
  }
  if(info.Shadow)
  {
    MM_Shadow *shadow2d = 0;
    MM_ShadowCube *shadowcube = 0;
    for(sInt i=0;i<MM_MaxLight;i++)
    {
      if(info.Shadow & info.DirLight & (1<<i))
      {
        if(shadow2d==0) shadow2d = new MM_Shadow;
        shadow2d->Mode[i] = MMRTM_DirShadow;
        shadow2d->Shadow |= 1<<i;
        shadow2d->ShadowOrd = info.ShadowOrd;
        shadow2d->ShadowRand = info.ShadowRand;
      }
      else if(info.Shadow & info.PointLight & (1<<i))
      {
        if(shadowcube==0) shadowcube = new MM_ShadowCube;
        shadowcube->Mode[i] = MMRTM_PointShadow;
      }
      else if(info.Shadow & info.SpotLight & (1<<i))
      {
        if(shadow2d==0) shadow2d = new MM_Shadow;
        shadow2d->Mode[i] = MMRTM_SpotShadow;
        shadow2d->Shadow |= 1<<i;
        shadow2d->ShadowOrd = info.ShadowOrd;
        shadow2d->ShadowRand = info.ShadowRand;
      }
    }
    if(shadow2d)
      ModulesNew.AddTail(shadow2d);
    if(shadowcube)
      ModulesNew.AddTail(shadowcube);

  }
  if(info.Features & MMF_Fog)
  {
    MM_Fog *mod = new MM_Fog;
    mod->FeatureFlags = info.FeatureFlags;
    ModulesNew.AddTail(mod);
  }
  if(info.Features & MMF_GroundFog)
  {
    MM_GroundFog *mod = new MM_GroundFog;
    mod->FeatureFlags = info.FeatureFlags;
    if((info.FeatureFlags & MMFF_SwapFog) && (info.Features & MMF_Fog))
    {
      ModulesNew.AddBefore(mod,ModulesNew.GetCount()-1);
    }
    else
    {
      ModulesNew.AddTail(mod);
    }
  }
  if(info.Features & MMF_ClipPlanes)
  {
    MM_ClipPlanes *mod = new MM_ClipPlanes;
    ModulesNew.AddTail(mod);
  }
  ModulesTotal = ModulesUser;
  ModulesTotal.Add(ModulesNew);
  sSortUp(ModulesTotal,&MtrlModule::Phase);

  CachedModShader *cm = new CachedModShader;
  cm->Info = info;

  ModMtrlParaAssign InputPara;

  ShaderLog.PrintF(L"Operator: %q x %d y %d\n",PageName,PageX,PageY);

  ShaderLog.Print(L"/****************************************************************************/\n");
  ShaderLog.Print(L"/***                                                                      ***/\n");
  switch(info.MatrixMode)
  {
  case sRF_MATRIX_ONE:
    ShaderLog.Print(L"/***   matrix: standard                                                   ***/\n");
    break;
  case sRF_MATRIX_BONE:
    ShaderLog.Print(L"/***   matrix: skinned                                                    ***/\n");
    break;
  case sRF_MATRIX_INSTANCE:
    ShaderLog.Print(L"/***   matrix: instanced                                                  ***/\n");
    break;
  case sRF_MATRIX_INSTPLUS:
    ShaderLog.Print(L"/***   matrix: instanced plus color                                       ***/\n");
    break;
  case sRF_MATRIX_BONEINST:
    ShaderLog.Print(L"/***   matrix: boneinst                                                   ***/\n");
    break;
  }

  sInt shadermask = 3;
  switch(info.RenderTarget)
  {
  case sRF_TARGET_MAIN:
    ShaderLog.Print(L"/***   target: main                                                       ***/\n");
    break;
  case sRF_TARGET_DIST:
    ShaderLog.Print(L"/***   target: dist                                                       ***/\n");
    shadermask = 1;
    break;
  case sRF_TARGET_ZONLY:
    ShaderLog.Print(L"/***   target: zonly                                                      ***/\n");
    shadermask = 1;
    break;
  case sRF_TARGET_ZNORMAL:
    ShaderLog.Print(L"/***   target: znormal                                                    ***/\n");
    shadermask = 1;
    break;
  default:
    ShaderLog.Print(L"/***   target: unknown                                                    ***/\n");
    break;
  }
  ShaderLog.Print(L"/***                                                                      ***/\n");
  ShaderLog.Print(L"/****************************************************************************/\n");
  ShaderLog.Print(L"/***                                                                      ***/\n");
  ShaderLog.Print(L"/***   Pixel Shader                                                       ***/\n");
  ShaderLog.Print(L"/***                                                                      ***/\n");
  ShaderLog.Print(L"/****************************************************************************/\n");

  sShader *vs=0;
  sShader *ps=0;
  sVertexFormatHandle *vf=0;
  MtrlModule *mod;
  sInt index;

  ModMtrl2 *mtrl = new ModMtrl2;


  // create pixel shader

  cm->PSPara.Init(32);
  InputPara.Init(MM_MaxT);
  sc->Init(&cm->PSPara,&InputPara,SCS_PS,cm->UpdateTex);
  sc->LightMask = 0;
  sc->Mtrl = this;

  sFORALL(ModulesTotal,mod)
    mod->Start(sc);


  sc->Output(L"o_color",SCT_FLOAT4,SCB_TARGET);

  sc->FragBegin(L"setup");
  sc->TB.Print(L"  float3 col_light = 0;\n");
  sc->FragFirst(L"col_light");
  sc->FragEnd();

  sc->FragBegin(L"setup");
  sc->TB.Print(L"  float3 col_diffuse = 1;\n");
  sc->FragFirst(L"col_diffuse");
  sc->FragEnd();

  sc->FragBegin(L"setup");
  sc->TB.Print(L"  float3 col_emissive = 0;\n");
  sc->FragFirst(L"col_emissive");
  sc->FragEnd();

  sc->FragBegin(L"setup");
  sc->TB.Print(L"  float3 col_gloss = 1;\n");
  sc->FragFirst(L"col_gloss");
  sc->FragEnd();

  sc->FragBegin(L"setup");
  sc->TB.Print(L"  float3 col_specular = 0;\n");
  sc->FragFirst(L"col_specular");
  sc->FragEnd();

  sc->FragBegin(L"setup");
  sc->TB.Print(L"  float3 col_aux = 0;\n");
  sc->FragFirst(L"col_aux");
  sc->FragEnd();

  sc->FragBegin(L"setup");
  sc->TB.Print(L"  float3 col_fog = 0;\n");
  sc->TB.Print(L"  float fogfactor = 0;\n");
  sc->FragFirst(L"col_fog");
  sc->FragFirst(L"fogfactor");
  sc->FragEnd();

  sc->FragBegin(L"setup");
  sc->TB.Print(L"  float3 alpha = 1;\n");
  sc->FragFirst(L"alpha");
  sc->FragEnd();

  sc->FragBegin(L"setup");
  sc->TB.Print(L"  float3 ws_lightdir[8];\n");
  sc->FragFirst(L"ws_lightdir");
  sc->FragEnd();

  sc->FragBegin(L"setup");
  sc->TB.Print(L"  float lighti[8];\n");
  sc->FragFirst(L"lighti");
  sc->FragEnd();

  sc->FragBegin(L"setup");
  sc->TB.Print(L"  float lightshadow[8] = { 1,1,1,1 ,1,1,1,1 };\n");
  sc->FragFirst(L"lightshadow");
  sc->FragEnd();

  sFORALL(ModulesTotal,mod)
    if(mod->Shaders & shadermask)
      mod->PS(sc);
  sFORALL(ModulesTotal,mod)
    if(mod->Shaders & shadermask)
      mod->Post(sc);

  switch(info.RenderTarget)
  {
  case sRF_TARGET_MAIN:
  default:
    sc->FragBegin(L"output");
    sc->TB.Print(L"  o_color.xyz = col_specular*col_gloss + col_diffuse*col_light;\n");

    if(info.FeatureFlags & MMFF_EmissiveNoRim)
    {
      sc->InputPS(L"cs_reflect_z",SCT_FLOAT);
      sc->TB.Print(L"  if(cs_reflect_z>=0) col_emissive = 0;\n");
    }
    if(info.FeatureFlags & MMFF_EmissiveNoCenter)
    {
      sc->InputPS(L"cs_reflect_z",SCT_FLOAT);
      sc->TB.Print(L"  if(cs_reflect_z<=0) col_emissive = 0;\n");
    }

    if(info.FeatureFlags & MMFF_EmissiveScreen)
      sc->TB.Print(L"  o_color.xyz = (col_emissive + o_color.xyz) - (col_emissive * o_color.xyz);\n");
    else
      sc->TB.Print(L"  o_color.xyz = col_emissive + o_color.xyz;\n");
    if(info.FeatureFlags & MMFF_FogIsBlack)
      sc->TB.Print(L"  o_color.xyz = o_color.xyz*(1-fogfactor);\n");
    else
      sc->TB.Print(L"  o_color.xyz = o_color.xyz*(1-fogfactor) + col_fog;\n");
    if(info.FeatureFlags & MMFF_FogAlpha)
      sc->TB.Print(L"  o_color.w = alpha.r*(1-fogfactor);\n");
    else
      sc->TB.Print(L"  o_color.w = alpha.r;\n");
    sc->FragRead(L"col_emissive");
    sc->FragRead(L"col_specular");
    sc->FragRead(L"col_gloss");
    sc->FragRead(L"col_diffuse");
    sc->FragRead(L"col_light");
    sc->FragRead(L"col_fog");
    sc->FragRead(L"fogfactor");
    sc->FragRead(L"alpha");
    sc->FragFirst(L"o_color");
    sc->FragEnd();
    break;

  case sRF_TARGET_DIST:
    sc->FragBegin(L"output");
    sc->FragFirst(L"o_color");
    sc->InputPS(L"cs_pos_z",SCT_FLOAT);
    sc->InputPS(L"ws_pos",SCT_FLOAT3);
    sc->InputPS(L"ws_campos",SCT_FLOAT3);
    sc->Para(L"ClipFarR");
    sc->Para(L"ZShaderSlopeBias");
    sc->Para(L"ZShaderBaseBiasOverClipFar");
    sc->TB.PrintF(L"  float bias = ZShaderBaseBiasOverClipFar + ZShaderSlopeBias*(abs(ddx(cs_pos_z))+abs(ddy(cs_pos_z)));\n");
    sc->TB.Print(L"  o_color.xyzw = length(ws_pos-ws_campos)*ClipFarR + bias;\n");
    sc->FragEnd();
    break;

  case sRF_TARGET_ZONLY:
    sc->FragBegin(L"output");
    sc->FragFirst(L"o_color");

    // this is only needed for the non-pcf shadows used by baked shadow
    sc->InputPS(L"cs_pos_z",SCT_FLOAT);
    sc->Para(L"ZShaderSlopeBias");
    sc->Para(L"ZShaderBaseBiasOverClipFar");
    sc->TB.PrintF(L"  float bias = ZShaderBaseBiasOverClipFar + ZShaderSlopeBias*(abs(ddx(cs_pos_z))+abs(ddy(cs_pos_z)));\n");
    sc->TB.Print(L"  o_color.xyzw = cs_pos_z+bias;\n");

//    sc->TB.Print(L"  o_color.xyzw = 1;\n");
    sc->FragEnd();
    break;

  case sRF_TARGET_ZNORMAL:      // this is WRONG! the freaky scaling is missing!
    sc->FragBegin(L"output");
    sc->FragFirst(L"o_color");
    sc->InputPS(L"ss_pos",SCT_FLOAT4);
    sc->InputPS(L"cs_normal",SCT_FLOAT3);
    sc->Para(L"ZShaderProjA");
    sc->Para(L"ZShaderProjB");
    sc->TB.Print(L"  o_color.yzw = normalize(cs_normal.xyz);\n");
    sc->TB.Print(L"  o_color.x = ZShaderProjB/(ss_pos.z/ss_pos.w-ZShaderProjA);\n");
    sc->FragEnd();
    break;
  }


  ps = sc->Compile(ShaderLog);
  if(ps==0)
    sDPrintF(L"compilation failed!\n");
  sc->SetTextures(mtrl);

  // give a nice error message if input parameters run over

  if(InputPara.GetError())
  {
    static const sChar *masks[] = { L"----",L"x---",L"xy--",L"xyz-",L"xyzw" };
    sDPrintF(L"Error: input parameter overflow (vs->ps)\n");
    for(sInt i=0;i<InputPara.GetSize();i++)
      sDPrintF(L"%2d:%s\n",i,masks[InputPara.GetAlloc(i)]);
  }


  // create vertex shader

  ShaderLog.Print(L"/****************************************************************************/\n");
  ShaderLog.Print(L"/***                                                                      ***/\n");
  ShaderLog.Print(L"/***   Vertex Shader                                                      ***/\n");
  ShaderLog.Print(L"/***                                                                      ***/\n");
  ShaderLog.Print(L"/****************************************************************************/\n");

  cm->VSPara.Init(28);
  sc->Shift(&cm->VSPara,0,SCS_VS);

  for(sInt i=0;i<InputPara.GetSize();i++)
    sc->Output(sPoolF(L"t%d",i),SCT_FLOAT4,SCB_UV,i);
  sc->Output(L"o_ss_pos",SCT_FLOAT4,SCB_POS);
  sc->BindVS(L"ms_posvert",SCT_FLOAT3,SCB_POSVERT);

  sc->FragBegin(L"original vertex");
  sc->FragRead(L"ms_posvert");
  sc->FragFirst(L"always there");
  sc->TB.Print(L"  float3 ms_pos = ms_posvert;\n");
  sc->TB.PrintF(L"  float4 instplus=1;\n");
  sc->FragEnd();

  sc->Para(L"ws_ss");
  if(WZ4ONLYONEGEO)
  {
    sc->FragBegin(L"fake normal for ONLYONEGEO");
    sc->Require(L"ms_normal",SCT_FLOAT3);
    sc->FragEnd();
  }
  if(info.MatrixMode!=sRF_MATRIX_INSTANCE && info.MatrixMode!=sRF_MATRIX_INSTPLUS && info.MatrixMode!=sRF_MATRIX_BONEINST)
  {
    sc->Uniform(L"instmat",SCT_FLOAT3x4,28*4);
  }
  else
  {
    sc->FragBegin(L"instancing");
    sc->BindVS(L"instmat0",SCT_FLOAT4,SCB_UV,5);
    sc->BindVS(L"instmat1",SCT_FLOAT4,SCB_UV,6);
    sc->BindVS(L"instmat2",SCT_FLOAT4,SCB_UV,7);
    sc->TB.PrintF(L"  float3x4 instmat = float3x4(instmat0,instmat1,instmat2);\n");
    if(info.MatrixMode==sRF_MATRIX_INSTPLUS)
    {
      sc->BindVS(L"instplus_pre",SCT_FLOAT4,SCB_UV,4);
      sc->TB.PrintF(L"  instplus=instplus_pre;\n");
    }
    sc->FragEnd();
  }
  if(info.MatrixMode==sRF_MATRIX_BONE || info.MatrixMode==sRF_MATRIX_BONEINST)
  {
    sc->FragBegin(L"skinning");

    sc->BindVS(L"blendindices",SCT_UINT4,SCB_BLENDINDICES);
    sc->BindVS(L"blendweight",SCT_UINT4,SCB_BLENDWEIGHT);
    sc->Uniform(L"Skinning",SCT_SKINNING,32*4);
    sc->Require(L"ms_normal",SCT_FLOAT3);
    sc->FragModify(L"ms_pos");

    sc->TB.PrintF(L"  {\n");
    sc->TB.PrintF(L"    float4 sm0,sm1,sm2,n;\n");
    sc->TB.PrintF(L"    float4 weight = (float4(blendweight)-127)/127;\n");
    sc->TB.PrintF(L"    sm0  = weight.x * Skinning[blendindices.x+0];\n");
    sc->TB.PrintF(L"    sm1  = weight.x * Skinning[blendindices.x+1];\n");
    sc->TB.PrintF(L"    sm2  = weight.x * Skinning[blendindices.x+2];\n");
    sc->TB.PrintF(L"    sm0 += weight.y * Skinning[blendindices.y+0];\n");
    sc->TB.PrintF(L"    sm1 += weight.y * Skinning[blendindices.y+1];\n");
    sc->TB.PrintF(L"    sm2 += weight.y * Skinning[blendindices.y+2];\n");
    sc->TB.PrintF(L"    sm0 += weight.z * Skinning[blendindices.z+0];\n");
    sc->TB.PrintF(L"    sm1 += weight.z * Skinning[blendindices.z+1];\n");
    sc->TB.PrintF(L"    sm2 += weight.z * Skinning[blendindices.z+2];\n");
    sc->TB.PrintF(L"    sm0 += weight.w * Skinning[blendindices.w+0];\n");
    sc->TB.PrintF(L"    sm1 += weight.w * Skinning[blendindices.w+1];\n");
    sc->TB.PrintF(L"    sm2 += weight.w * Skinning[blendindices.w+2];\n");
    sc->TB.PrintF(L"    n = float4(ms_pos.xyz,1);\n");
    sc->TB.PrintF(L"    ms_pos.x  = dot(n,sm0);\n");
    sc->TB.PrintF(L"    ms_pos.y  = dot(n,sm1);\n");
    sc->TB.PrintF(L"    ms_pos.z  = dot(n,sm2);\n");
    sc->TB.PrintF(L"    n = float4(ms_normal.xyz,0);\n");
    sc->TB.PrintF(L"    ms_normal.x = dot(n,sm0);\n");
    sc->TB.PrintF(L"    ms_normal.y = dot(n,sm1);\n");
    sc->TB.PrintF(L"    ms_normal.z = dot(n,sm2);\n");
    sc->TB.PrintF(L"  }\n");

    sc->FragEnd();
  }

  sFORALL(ModulesTotal,mod)
    if(mod->Shaders & shadermask)
      mod->VS(sc);
  sFORALL(ModulesTotal,mod)
    if(mod->Shaders & shadermask)
      mod->Post(sc);


  switch(info.RenderTarget)
  {
  case sRF_TARGET_MAIN:
  default:
    break;

  case sRF_TARGET_DIST:
  case sRF_TARGET_ZONLY:
  case sRF_TARGET_ZNORMAL:      
    sc->Require(L"ws_normal",SCT_FLOAT3);    // make sure all three formats have same vertex buffer
    break;
  }

  if(sc->Requires(L"cs_reflect_z",index))
  {
    sc->FragBegin(L"reflection (eye|normal)");
    sc->FragFirst(L"cs_reflect_z");
    sc->Require(L"cs_reflect",SCT_FLOAT3);

    sc->TB.PrintF(L"  float1 cs_reflect_z = cs_reflect.z;\n");
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = cs_reflect_z;\n",index/4,sc->swizzle[index&3][1]);
    sc->FragEnd();
  }
  if(sc->Requires(L"cs_reflect",index))
  {
    sc->FragBegin(L"reflection (eye|normal)");
    sc->FragFirst(L"cs_reflect");
    sc->Require(L"ws_reflect",SCT_FLOAT3);

    sc->TB.PrintF(L"  float3 cs_reflect = mul(ws_cs,float4(ws_reflect,0));\n");
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = cs_reflect;\n",index/4,sc->swizzle[index&3][3]);
    sc->FragEnd();
  }
  if(sc->Requires(L"cs_eye",index))
  {
    sc->FragBegin(L"input");
    sc->FragFirst(L"cs_eye");
    sc->Require(L"ws_eye",SCT_FLOAT3);

    sc->TB.PrintF(L"  float3 cs_eye = mul(ws_cs,float4(ws_eye,0));\n");
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = cs_eye;\n",index/4,sc->swizzle[index&3][3]);
    sc->FragEnd();
  }
  if(sc->Requires(L"ws_reflect",index))
  {
    sc->FragBegin(L"reflection (eye|normal)");
    sc->FragFirst(L"ws_reflect");
    sc->Require(L"ws_normal",SCT_FLOAT3);
    sc->Require(L"ws_eye",SCT_FLOAT3);

    sc->TB.PrintF(L"  float3 ws_reflect = reflect(normalize(ws_eye),normalize(ws_normal));\n");
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = ws_reflect;\n",index/4,sc->swizzle[index&3][3]);
    sc->FragEnd();
  }
  if(sc->Requires(L"ws_eye",index))
  {
    sc->FragBegin(L"inputs");
    sc->Para(L"ws_campos");
    sc->TB.PrintF(L"  float3 ws_eye = ws_campos-ws_pos;\n");
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = ws_eye;\n",index/4,sc->swizzle[index&3][3]);
    sc->FragFirst(L"ws_eye");
    sc->FragRead(L"ws_pos");
    sc->FragEnd();
  }
  if(sc->Requires(L"ws_pos",index))
  {
    sc->FragBegin(L"inputs");
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = ws_pos;\n",index/4,sc->swizzle[index&3][3]);
    sc->FragRead(L"ws_pos");
    sc->FragEnd();
  }
  if(sc->Requires(L"ws_campos",index))
  {
    sc->FragBegin(L"inputs");
    sc->Para(L"ws_campos");
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = ws_campos;\n",index/4,sc->swizzle[index&3][3]);
    sc->FragEnd();
  }
  if(sc->Requires(L"ms_pos",index))
  {
    sc->FragBegin(L"inputs");
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = ms_pos;\n",index/4,sc->swizzle[index&3][3]);
    sc->FragRead(L"ws_pos");
    sc->FragEnd();
  }
  if(sc->Requires(L"ms_posvert",index))
  {
    sc->FragBegin(L"inputs");
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = ms_posvert;\n",index/4,sc->swizzle[index&3][3]);
    sc->FragRead(L"ws_posvert");
    sc->FragEnd();
  }
  if(sc->Requires(L"cs_pos",index))
  {
    sc->FragBegin(L"inputs");
    sc->FragFirst(L"cs_pos");
    sc->FragRead(L"ws_pos");

    sc->Para(L"ws_cs");

    sc->TB.PrintF(L"  float3 cs_pos = mul(ws_cs,float4(ws_pos,1));\n");
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = cs_pos;\n",index/4,sc->swizzle[index&3][3]);
    sc->FragEnd();
  }
  if(sc->Requires(L"cs_pos_z",index))
  {
    sc->FragBegin(L"inputs");
    sc->FragFirst(L"cs_pos_z");
    sc->FragRead(L"ws_pos");

    sc->Para(L"ws_cs");
    sc->Para(L"ClipFarR");

    sc->TB.PrintF(L"  float cs_pos_z = mul(ws_cs,float4(ws_pos,1)).z*ClipFarR;\n");
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = cs_pos_z;\n",index/4,sc->swizzle[index&3][1]);
    sc->FragEnd();
  }
  if(sc->Requires(L"cs_normal",index))
  {
    sc->FragBegin(L"inputs");
    sc->FragFirst(L"cs_normal");
    sc->Para(L"ws_cs");
    sc->Require(L"ws_normal",SCT_FLOAT3);

    sc->TB.PrintF(L"  float3 cs_normal = mul(ws_cs,float4(ws_normal,0));\n");
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = cs_normal;\n",index/4,sc->swizzle[index&3][3]);
    sc->FragEnd();
  }
  if(sc->Requires(L"ws_bitangent",index))
  {
    sc->FragBegin(L"inputs");
    sc->FragFirst(L"ws_bitangent");
    sc->Require(L"ws_tangent",SCT_FLOAT3);
    sc->Require(L"ws_normal",SCT_FLOAT3);

    sc->TB.PrintF(L"  float3 ws_bitangent = cross(ws_tangent,ws_normal);\n");
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = ws_bitangent;\n",index/4,sc->swizzle[index&3][3]);
    sc->FragEnd();
  }

  if(sc->Requires(L"ws_normal",index))
  {
    sc->FragBegin(L"inputs");
    sc->FragFirst(L"ws_normal");
    sc->Require(L"ms_normal",SCT_FLOAT3);

    sc->TB.PrintF(L"  float3 ws_normal = mul(instmat,float4(ms_normal,0));\n");
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = ws_normal;\n",index/4,sc->swizzle[index&3][3]);
    sc->FragEnd();
  }
  if(sc->Requires(L"ws_tangent",index))
  {
    sc->FragBegin(L"inputs");
    sc->FragFirst(L"ws_tangent");
    sc->Require(L"ms_tangent",SCT_FLOAT3);

    sc->TB.PrintF(L"  float3 ws_tangent = mul(instmat,float4(ms_tangent,0));\n");
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = ws_tangent;\n",index/4,sc->swizzle[index&3][3]);
    sc->FragEnd();
  }

  if(sc->Requires(L"ms_normal",index))
    sc->Require(L"ms_normalvert",SCT_FLOAT3);

  if(sc->Requires(L"ms_normalvert",index))
  {
    sc->FragBegin(L"inputs");
    sc->FragFirst(L"ms_normalvert");
    sc->BindVS(L"ms_normalvert_pre",SCT_FLOAT3,SCB_NORMAL);
    sc->TB.PrintF(L"  float3 ms_normalvert = ms_normalvert_pre.xyz;\n");
    if(FeatureFlags & MMFF_NormalI4)
      sc->TB.PrintF(L"  ms_normalvert = normalize(ms_normalvert/127.0-1);\n",index/4,sc->swizzle[index&3][3]);

    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = ms_normalvert;\n",index/4,sc->swizzle[index&3][3]);
    sc->FragEnd();
  }

  if(sc->Requires(L"ms_normal",index))
  {
    sc->FragBegin(L"inputs");
//    sc->BindVS(L"ms_normal",SCT_FLOAT3,SCB_NORMAL);
    sc->FragFirst(L"ms_normal");
    sc->Require(L"ms_normalvert",SCT_FLOAT3);
    sc->TB.Print(L"  float3 ms_normal = ms_normalvert;\n");
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = ms_normal;\n",index/4,sc->swizzle[index&3][3]);
    sc->FragEnd();
  }

  if(sc->Requires(L"ms_tangent",index))
  {
    sc->FragBegin(L"inputs");
    sc->BindVS(L"ms_tangent",SCT_FLOAT3,SCB_TANGENT);
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = ms_tangent;\n",index/4,sc->swizzle[index&3][3]);
    sc->FragEnd();
  }
  if(sc->Requires(L"uv0",index))
  {
    sc->FragBegin(L"inputs");
    sc->BindVS(L"uv0",SCT_FLOAT2,SCB_UV,0);
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = uv0;\n",index/4,sc->swizzle[index&3][2]);
    sc->FragEnd();
  }
  if(sc->Requires(L"uv1",index))
  {
    sc->FragBegin(L"inputs");
    sc->BindVS(L"uv1",SCT_FLOAT2,SCB_UV,1);
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = uv1;\n",index/4,sc->swizzle[index&3][2]);
    sc->FragEnd();
  }

  if(sc->Requires(L"color0",index))
  {
    sc->FragBegin(L"inputs");
    sc->BindVS(L"color0",SCT_FLOAT4,SCB_COLOR,0);
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = color0;\n",index/4,sc->swizzle[index&3][4]);
    sc->FragEnd();
  }  
  if(sc->Requires(L"color1",index))
  {
    sc->FragBegin(L"inputs");
    sc->BindVS(L"color1",SCT_FLOAT4,SCB_COLOR,1);
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = color1;\n",index/4,sc->swizzle[index&3][4]);
    sc->FragEnd();
  }
  if(sc->Requires(L"instplus",index))
  {
    sc->FragBegin(L"inputs");
    if(index>=0)
    {
      sc->TB.PrintF(L"  t%d.%s = instplus;\n",index/4,sc->swizzle[index&3][4]);
    }
    sc->FragEnd();
  }

  sc->FragBegin(L"inputs");
  sc->FragFirst(L"ws_pos");
  sc->FragRead(L"ms_pos");
  sc->FragRead(L"instmat");
  sc->TB.PrintF(L"  float3 ws_pos = mul(instmat,float4(ms_pos,1));\n",L"instance matrix");
  sc->FragEnd();

  sc->FragBegin(L"inputs");
  sc->FragFirst(L"o_ss_pos");
  sc->FragRead(L"ws_pos");
  sc->FragRead(L"ws_ss");
  sc->TB.PrintF(L"  o_ss_pos = mul(float4(ws_pos,1),ws_ss);\n",L"projection");
  if(sc->Requires(L"ss_pos",index))
    if(index>=0)
      sc->TB.PrintF(L"  t%d.%s = o_ss_pos;\n",index/4,sc->swizzle[index&3][4]);
  sc->FragEnd();


  if(0)     // not really required, but may be useful later
  {
    sc->FragBegin(L"reset tx outputs");
    for(sInt i=0;i<InputPara.GetSize();i++)
    {
      if(InputPara.GetAlloc(i)==1)
        sc->TB.PrintF(L"  t%d.yzw = 0;\n",i);
      if(InputPara.GetAlloc(i)==2)
        sc->TB.PrintF(L"  t%d.zw = 0;\n",i);
      if(InputPara.GetAlloc(i)==3)
        sc->TB.PrintF(L"  t%d.w = 0;\n",i);
    }
    sc->FragEnd();
  }

  sc->SortBinds();

  vs = sc->Compile(ShaderLog);
  sc->SetTextures(mtrl);
  if(vs==0)
    sDPrintF(L"vs compilation failed!\n");


  // create vertex format

  sc->Shift(0,0,0);

  sU32 descdata[32],*desc=descdata;
  *desc++ = sVF_POSITION|sVF_F3;
  if(sc->Requires(L"ms_normalvert_pre",index))
  {
    if(FeatureFlags & MMFF_NormalI4)
      *desc++ = sVF_NORMAL|sVF_I4;
    else
      *desc++ = sVF_NORMAL|sVF_F3;
  }
  if(sc->Requires(L"ms_tangent",index))
    *desc++ = sVF_TANGENT|sVF_F3;
  if(sc->Requires(L"ms_bitangent",index))
    *desc++ = sVF_BINORMAL|sVF_F3;
  if(sc->Requires(L"blendindices",index))
    *desc++ = sVF_BONEINDEX|sVF_I4;
  if(sc->Requires(L"blendweight",index))
    *desc++ = sVF_BONEWEIGHT|sVF_I4;
  if(sc->Requires(L"uv0",index))
    *desc++ = sVF_UV0|sVF_F2;
  if(sc->Requires(L"uv1",index))
    *desc++ = sVF_UV1|sVF_F2;
  if(sc->Requires(L"instmat0",index))
    *desc++ = sVF_UV5|sVF_F4|sVF_STREAM1|sVF_INSTANCEDATA;
  if(sc->Requires(L"instmat1",index))
    *desc++ = sVF_UV6|sVF_F4|sVF_STREAM1|sVF_INSTANCEDATA;
  if(sc->Requires(L"instmat2",index))
    *desc++ = sVF_UV7|sVF_F4|sVF_STREAM1|sVF_INSTANCEDATA;
  if(sc->Requires(L"instplus_pre",index))
    *desc++ = sVF_UV4|sVF_F4|sVF_STREAM1|sVF_INSTANCEDATA;
  *desc = 0;
  vf = sCreateVertexFormat(descdata);

  // initialize materials

  mtrl->InitVariants(2);

  mtrl->Flags = Flags;
  if((AlphaTest&0xff)!=7)
  {
    mtrl->FuncFlags[sMFT_ALPHA] = AlphaTest;
    mtrl->AlphaRef = AlphaRef;
  }
  mtrl->BlendFactor = (AlphaRef<<24)|(AlphaRef<<16)|(AlphaRef<<8)|(AlphaRef);
  mtrl->BlendColor = BlendColor;
  mtrl->BlendAlpha = BlendAlpha;

  mtrl->SetVariant(0);
  sInt cull = mtrl->Flags & sMTRL_CULLMASK;
  if(cull==sMTRL_CULLON || cull==sMTRL_CULLINV)
    mtrl->Flags ^= sMTRL_CULLON ^ sMTRL_CULLINV;
  mtrl->SetVariant(1);
  mtrl->Prepare(vf);
  mtrl->VertexShader = vs;
  mtrl->PixelShader = ps;

  // done

  if(!vs || !ps || !vf || cm->VSPara.GetError() || cm->PSPara.GetError() || InputPara.GetError())
    Error = 1;

  cm->Code.Print(ShaderLog.Get()+ShaderLogEnd);
  cm->Shader = mtrl;
  cm->Format = vf;

  sReleaseAll(ModulesNew);

  return cm;
}


/****************************************************************************/
/***                                                                      ***/
/***   Modules                                                            ***/
/***                                                                      ***/
/****************************************************************************/

ModShader::ModShader()
{
  Type = ModShaderType;
}

ModShaderSampler::ModShaderSampler() 
{
  Type = ModShaderSamplerType; 
}

ModShader::~ModShader()
{
  sReleaseAll(Modules);
}

void ModShader::Add(ModShader *sh)
{
  if(sh)
  {
    sAddRefAll(sh->Modules);
    Modules.Add(sh->Modules);
  }
}

void ModShader::Add(MtrlModule *mod)
{
  Modules.AddTail(mod);
}

/****************************************************************************/
/***                                                                      ***/
/***   Environment Render Node                                            ***/
/***                                                                      ***/
/****************************************************************************/

RNModLight::RNModLight()
{
  Anim.Init(Wz4RenderType->Script);
}

RNModLight::~RNModLight()
{
}

void RNModLight::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  SimulateChilds(ctx);
  sMatrix34 trans;
  if(Para.TransformEnable & 1)
  {
    sSRT srt;
    srt.Init(&Para.Scale.x);
    srt.MakeMatrix(trans);
  }

  ModLightEnv *env = ModMtrlType->LightEnv[Para.Index];
  env->Ambient.InitColor(Para.Ambient);

  struct fake
  {
    sInt Mode;
    sU32 Front;
    sF32 FrontAmp;
    sU32 Middle;
    sF32 MiddleAmp;
    sU32 Back;
    sF32 BackAmp;
    sVector31 Pos;
    sVector30 Dir;
    sF32 Range;
    sF32 Inner;
    sF32 Outer;
    sF32 Falloff;
    sInt SM_Size;
    sF32 SM_BaseBias;
    sF32 SM_Filter;
    sF32 SM_SlopeBias;
    sF32 dummy[7];
  };



  fake *fp = (fake *) &Para.Mode0;

  env->DirLight = 0;
  env->PointLight = 0;
  env->SpotLight = 0;
  env->Shadow = 0;
  env->ShadowOrd = 0;
  env->ShadowRand = 0;
  env->SpotInner = 0;
  env->SpotFalloff = 0;

  for(sInt i=0;i<8;i++)
  {
    env->Lights[i].ColFront.InitColor(fp->Front,fp->FrontAmp);
    env->Lights[i].ColMiddle.InitColor(fp->Middle,fp->MiddleAmp);
    env->Lights[i].ColBack.InitColor(fp->Back,fp->BackAmp);
    env->Lights[i].ws_Pos = fp->Pos;
    env->Lights[i].ws_Dir = -fp->Dir;   // lighting need direction from surface to lightsource, user wants to enter direction of light shining.
    env->Lights[i].ws_Dir.Unit();
    env->Lights[i].Range = fp->Range;
    env->Lights[i].Inner = sMax(fp->Inner,fp->Outer);
    env->Lights[i].Outer = fp->Outer;
    env->Lights[i].Falloff = fp->Falloff;
    env->Lights[i].SM_Size = fp->SM_Size;
    env->Lights[i].SM_BaseBias = fp->SM_BaseBias;
    env->Lights[i].SM_Filter = fp->SM_Filter/(1<<(fp->SM_Size&15));
    env->Lights[i].SM_SlopeBias = fp->SM_SlopeBias;

    if((fp->Mode&15)==1)
      env->DirLight |= 1<<i;
    if((fp->Mode&15)==2)
      env->PointLight |= 1<<i;
    if((fp->Mode&15)==3)
      env->SpotLight |= 1<<i;
    if((fp->Mode&16))
      env->Shadow |= 1<<i;
    if((fp->Mode&32))
      env->SpotInner |= 1<<i;
    if((fp->Mode&64))
      env->SpotFalloff |= 1<<i;
    if((fp->Mode&128))
    {
      env->Lights[i].ws_Pos = env->Lights[i].ws_Pos;
      env->Lights[i].ws_Dir = env->Lights[i].ws_Dir;
    }
    else
    {
      env->Lights[i].ws_Pos = env->Lights[i].ws_Pos * trans;
      env->Lights[i].ws_Dir = env->Lights[i].ws_Dir * trans;
    }
    env->Lights[i].ws_Pos_ = env->Lights[i].ws_Pos;
    env->Lights[i].ws_Dir_ = env->Lights[i].ws_Dir;
    env->Lights[i].Mode = fp->Mode;

    if((fp->SM_Size & 0x30000)==0x10000)
      env->ShadowOrd |= 1<<i;
    if((fp->SM_Size & 0x30000)==0x20000)
      env->ShadowRand |= 1<<i;

    env->Shadow &= env->DirLight | env->PointLight | env->SpotLight;
    env->ShadowOrd &= env->Shadow;
    env->ShadowRand &= env->Shadow;
    fp++;
  }

  env->LimitShadow = Para.LimitShadows & 255;
  env->LimitShadowFlags = (Para.LimitShadows>>8) & 255;
  env->LimitShadowCenter = Para.LimitCenter;
  env->LimitShadowRadius = Para.LimitRadius;

}


void RNModLight::Render(Wz4RenderContext *ctx)
{
}

/****************************************************************************/

RNModMisc::RNModMisc()
{
  Anim.Init(Wz4RenderType->Script);
}

RNModMisc::~RNModMisc()
{
}

void RNModMisc::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  SimulateChilds(ctx);

  ModLightEnv *env = ModMtrlType->LightEnv[Para.Index];

  for(sInt i=0;i<8;i++)
  {
    env->Vector[i] = (&Para.Vector0)[i];
  }
  env->Color[0].InitColor(Para.Color0,Para.Amp0);
  env->Color[1].InitColor(Para.Color1,Para.Amp1);
  env->Color[2].InitColor(Para.Color2,Para.Amp2);
  env->Color[3].InitColor(Para.Color3,Para.Amp3);
  env->Color[4].InitColor(Para.Color4,Para.Amp4);
  env->Color[5].InitColor(Para.Color5,Para.Amp5);
  env->Color[6].InitColor(Para.Color6,Para.Amp6);
  env->Color[7].InitColor(Para.Color7,Para.Amp7);

  struct fake
  {
    sVector31 s;
    sVector30 r;
    sVector31 t;
  };
  fake *fp = (fake *) (&Para.Scale0.x);
  for(sInt i=0;i<4;i++)
  {
    sSRT srt;
    srt.Scale     = fp->s;
    srt.Rotate    = fp->r;
    srt.Translate = fp->t;
    fp++;
    sMatrix34 mat;
    srt.MakeMatrix(mat);
    env->Matrix[i] = mat;
  }
}

void RNModMisc::Render(Wz4RenderContext *ctx)
{
}

/****************************************************************************/

RNModLightEnv::RNModLightEnv()
{
  Anim.Init(Wz4RenderType->Script);
}

RNModLightEnv::~RNModLightEnv()
{
}

void RNModLightEnv::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  SimulateChilds(ctx);

  // initialize lightenvironment

  sVector30 n;
  ModLightEnv *env = ModMtrlType->LightEnv[Para.Index];

  env->Features = Para.Features;

  // fogging

  env->FogAdd = -Para.FogNear;
  env->FogMul = 1/sMax(0.001f,Para.FogFar);
  env->FogDensity = Para.FogDensity;
  env->FogColor.InitColor(Para.FogColor);

  // depth fog

  env->GroundFogAdd = -Para.GroundFogNear;
  env->GroundFogMul = 1/sMax(0.001f,Para.GroundFogFar-Para.GroundFogNear);
  env->GroundFogDensity = Para.GroundFogDensity;
  env->GroundFogColor.InitColor(Para.GroundFogColor);

  n.Init(Para.GroundFogPlane.x,Para.GroundFogPlane.y,Para.GroundFogPlane.z);
  n.Unit();
  env->ws_GroundFogPlane.Init(n.x,n.y,n.z,Para.GroundFogPlane.w);

  // clip planes

  env->Clip0.Init(0,0,0,1);
  env->Clip1.Init(0,0,0,1);
  env->Clip2.Init(0,0,0,1);
  env->Clip3.Init(0,0,0,1);
  if(Para.ClipEnable)
  {
    if(Para.ClipEnable & 1)
    {
      n.Init(Para.Clip0.x,Para.Clip0.y,Para.Clip0.z);
      n.Unit();
      env->Clip0.Init(n.x,n.y,n.z,Para.Clip0.w);
    }
    if(Para.ClipEnable & 2)
    {
      n.Init(Para.Clip1.x,Para.Clip1.y,Para.Clip1.z);
      n.Unit();
      env->Clip1.Init(n.x,n.y,n.z,Para.Clip1.w);
    }
    if(Para.ClipEnable & 4)
    {
      n.Init(Para.Clip2.x,Para.Clip2.y,Para.Clip2.z);
      n.Unit();
      env->Clip2.Init(n.x,n.y,n.z,Para.Clip2.w);
    }
    if(Para.ClipEnable & 8)
    {
      n.Init(Para.Clip3.x,Para.Clip3.y,Para.Clip3.z);
      n.Unit();
      env->Clip3.Init(n.x,n.y,n.z,Para.Clip3.w);
    }
  }
}

void RNModLightEnv::Render(Wz4RenderContext *ctx)
{
}

/****************************************************************************/

RNModClipTwister::RNModClipTwister()
{
  Anim.Init(Wz4RenderType->Script);
}

RNModClipTwister::~RNModClipTwister()
{
}

const sVector4 rotateplane(const sVector4 &plane,const sMatrix34 &mat)
{
  sVector30 n(plane.x,plane.y,plane.z);
  sVector31 p(-plane.x*plane.w,-plane.y*plane.w,-plane.z*plane.w);
  n = n*mat;
  p = p*mat;
  sVector4 newplane;
  newplane.InitPlane(p,n);
  return newplane;
}

void RNModClipTwister::Simulate(Wz4RenderContext *ctx)
{
  Para = ParaBase;
  Anim.Bind(ctx->Script,&Para);
  SimulateCalc(ctx);
//  Anim.UnBind(ctx->Script,&Para);

  SimulateChilds(ctx);

  // initialize lightenvironment

  sVector30 n;
  ModLightEnv *env = ModMtrlType->LightEnv[Para.Index];

  env->Features |= MMF_ClipPlanes;

  // calculate matrix

  sSRT srt;
  sMatrix34 mat;

  srt.Init(&Para.Scale.x);
  srt.MakeMatrix(mat);

  // clip planes

  env->Clip0.Init(0,0,0,1);
  env->Clip1.Init(0,0,0,1);
  env->Clip2.Init(0,0,0,1);
  env->Clip3.Init(0,0,0,1);
  if(Para.ClipEnable)
  {
    if(Para.ClipEnable & 1)
    {
      n.Init(Para.Clip0.x,Para.Clip0.y,Para.Clip0.z);
      n.Unit();
      env->Clip0.Init(n.x,n.y,n.z,Para.Clip0.w);
      env->Clip0 = rotateplane(env->Clip0,mat);
    }
    if(Para.ClipEnable & 2)
    {
      n.Init(Para.Clip1.x,Para.Clip1.y,Para.Clip1.z);
      n.Unit();
      env->Clip1.Init(n.x,n.y,n.z,Para.Clip1.w);
      env->Clip1 = rotateplane(env->Clip1,mat);
    }
    if(Para.ClipEnable & 4)
    {
      n.Init(Para.Clip2.x,Para.Clip2.y,Para.Clip2.z);
      n.Unit();
      env->Clip2.Init(n.x,n.y,n.z,Para.Clip2.w);
      env->Clip2 = rotateplane(env->Clip2,mat);
    }
    if(Para.ClipEnable & 8)
    {
      n.Init(Para.Clip3.x,Para.Clip3.y,Para.Clip3.z);
      n.Unit();
      env->Clip3.Init(n.x,n.y,n.z,Para.Clip3.w);
      env->Clip3 = rotateplane(env->Clip3,mat);
    }
  }
}

void RNModClipTwister::Render(Wz4RenderContext *ctx)
{
}

/****************************************************************************/

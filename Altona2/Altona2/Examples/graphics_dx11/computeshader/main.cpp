/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"
#include "shader.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Altona2::Main()
{
//  sRunApp(new App,sScreenMode(sSM_Fullscreen,"Blur (with compute shader)",1920,1080));
  sRunApp(new App,sScreenMode(0,"Compute Shader",1280,720));
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

App::App()
{
  Mtrl = 0;
  Tex = 0;
  Geo = 0;
  DPaint = 0;
  cbv0 = 0;

  RtSizeX = 0;
  RtSizeY = 0;
  RtDepth = 0;
  RtColor = 0;

  for(sInt i=0;i<sCOUNTOF(RtWork);i++)
    RtWork[i] = 0;

}

App::~App()
{
  delete RtDepth;
  delete RtColor;
  for(sInt i=0;i<sCOUNTOF(RtWork);i++)
    delete RtWork[i];
}

void App::OnInit()
{
  Screen = sGetScreen();
  Adapter = Screen->Adapter;
  Context = Adapter->ImmediateContext;

  DPaint = new sDebugPainter(Adapter);

  static const sU32 vfdesc[] =
  {
    sVF_Position,
    sVF_Normal,
    sVF_UV0,
    sVF_End,
  };

  Format = Adapter->CreateVertexFormat(vfdesc);

  const sInt sx=64;
  const sInt sy=64;
  sU32 tex[sy][sx];
  for(sInt y=0;y<sy;y++)
    for(sInt x=0;x<sx;x++)
      tex[y][x] = ((x^y)&8) ? 0xffff8080 : 0xff80ff80;
  Tex = new sResource(Adapter,sResTexPara(sRF_BGRA8888,sx,sy,1),&tex[0][0],sizeof(tex));


  Mtrl = new sFixedMaterial(Adapter);
  Mtrl->SetTexturePS(0,Tex,sSamplerStatePara(sTF_Linear|sTF_Clamp,0));
  Mtrl->SetState(sRenderStatePara(sMTRL_DepthOn|sMTRL_CullOn,sMB_Off));
  Mtrl->Prepare(Format);


  const sInt ic = 6*6;
  const sInt vc = 24;
  static const sU16 ib[ic] =
  {
     0, 1, 2, 0, 2, 3,
     4, 5, 6, 4, 6, 7,
     8, 9,10, 8,10,11,

    12,13,14,12,14,15,
    16,17,18,16,18,19,
    20,21,22,20,22,23,
  };
  struct vert
  {
    sF32 px,py,pz;
    sF32 nx,ny,nz;
    sF32 u0,v0;
  };
  static const vert vb[vc] =
  {
    { -1, 1,-1,   0, 0,-1, 0,0, },
    {  1, 1,-1,   0, 0,-1, 1,0, },
    {  1,-1,-1,   0, 0,-1, 1,1, },
    { -1,-1,-1,   0, 0,-1, 0,1, },

    { -1,-1, 1,   0, 0, 1, 0,0, },
    {  1,-1, 1,   0, 0, 1, 1,0, },
    {  1, 1, 1,   0, 0, 1, 1,1, },
    { -1, 1, 1,   0, 0, 1, 0,1, },

    { -1,-1,-1,   0,-1, 0, 0,0, },
    {  1,-1,-1,   0,-1, 0, 1,0, },
    {  1,-1, 1,   0,-1, 0, 1,1, },
    { -1,-1, 1,   0,-1, 0, 0,1, },

    {  1,-1,-1,   1, 0, 0, 0,0, },
    {  1, 1,-1,   1, 0, 0, 1,0, },
    {  1, 1, 1,   1, 0, 0, 1,1, },
    {  1,-1, 1,   1, 0, 0, 0,1, },

    {  1, 1,-1,   0, 1, 0, 0,0, },
    { -1, 1,-1,   0, 1, 0, 1,0, },
    { -1, 1, 1,   0, 1, 0, 1,1, },
    {  1, 1, 1,   0, 1, 0, 0,1, },

    { -1, 1,-1,  -1, 0, 0, 0,0, },
    { -1,-1,-1,  -1, 0, 0, 1,0, },
    { -1,-1, 1,  -1, 0, 0, 1,1, },
    { -1, 1, 1,  -1, 0, 0, 0,1, },
  };

  Geo = new sGeometry(Adapter);
  Geo->SetIndex(sResBufferPara(sRBM_Index|sRU_Static,sizeof(sU16),ic),ib);
  Geo->SetVertex(sResBufferPara(sRBM_Vertex|sRU_Static,sizeof(vert),vc),vb);
  Geo->Prepare(Format,sGMP_Tris|sGMF_Index16);

  cbv0 = new sCBuffer<sFixedMaterialLightVC>(Adapter,sST_Vertex,0);

  // blur

  ComputeMtrl = new sMaterial(Adapter);
  ComputeMtrl->SetShaders(ComputeShader.Get(0));
}

void App::OnExit()
{
  delete Mtrl;
  delete ComputeMtrl;
  delete Tex;
  delete Geo;
  delete cbv0;
  delete DPaint;
}

void App::OnFrame()
{
}

void App::OnPaint()
{
  sScreenMode mode;
  Screen->GetMode(mode);

  if(mode.SizeX*mode.SizeY==0)
    return;

  if(mode.SizeX!=RtSizeX || mode.SizeY!=RtSizeY)
  {
    delete RtDepth;
    delete RtColor;
    for(sInt i=0;i<sCOUNTOF(RtWork);i++)
      delete RtWork[i];

    sResPara rp; 
    rp = sResPara(sRBM_ColorTarget|sRU_Gpu|sRBM_Shader|sRBM_Unordered|sRM_Texture,mode.ColorFormat,sRES_NoMipmaps,mode.SizeX,mode.SizeY);

    for(sInt i=0;i<sCOUNTOF(RtWork);i++)
      RtWork[i] = new sResource(Adapter,rp);
    rp.Mode = sRBM_ColorTarget|sRU_Gpu|sRM_Texture;
    Adapter->GetBestMultisampling(rp,rp.MSCount,rp.MSQuality);
    RtColor = new sResource(Adapter,rp);
    rp.Format = mode.DepthFormat;
    rp.Mode = sRBM_DepthTarget|sRU_Gpu|sRM_Texture,mode.ColorFormat;
    RtDepth = new sResource(Adapter,rp);
    RtSizeX = mode.SizeX;
    RtSizeY = mode.SizeY;
  }

  sF32 time = sGetTimeMS()*0.001f;
  sTargetPara tp(sTAR_ClearAll,0xff405060,RtColor,RtDepth);
  sViewport view;

  {
    sGPU_ZONE("Cube",0xff00ff00);
    Context->BeginTarget(tp);

    view.Camera.k.w = -4;
    view.Model = sEulerXYZ(time*0.11f,time*0.13f,time*0.15f);
    view.ZoomX = 1/tp.Aspect;
    view.ZoomY = 1;
    view.Prepare(tp);

    sFixedMaterialLightPara lp;
    lp.LightDirWS[0].Set(0,0,-1);
    lp.LightColor[0] = 0xffffff;
    lp.AmbientColor = 0x202020;

    cbv0->Map();
    cbv0->Data->Set(view,lp);
    cbv0->Unmap();

    Context->Draw(sDrawPara(Geo,Mtrl,cbv0));

    Context->EndTarget();
  }

  {
    sGPU_ZONE("Resolve",0xff00c0c0);
    Context->Copy(sCopyTexturePara(0,RtWork[0],RtColor));
  }

  {
    sGPU_ZONE("Compute Shader",0xffc0c000);
    ComputeMtrl->SetTexture(sST_Compute,0,RtWork[0],0);
    ComputeMtrl->SetUav(sST_Compute,0,RtWork[1]);
    ComputeMtrl->Prepare(0);
    sDrawPara dp(RtSizeX/8,RtSizeY/8,1,ComputeMtrl);
    Context->Draw(dp);
  }

  {
    sGPU_ZONE("Back Blit",0xff00c0c0);
    Context->Copy(sCopyTexturePara(0,Screen->GetScreenColor(),RtWork[1]));
  }

  {
    sGPU_ZONE("Hud",0xff404040);
    sTargetPara tps(0,0,Screen->GetScreenColor(),0);
    Context->BeginTarget(tps);

    DPaint->PrintPerfMon();
    DPaint->PrintFPS();
    DPaint->PrintStats();
    DPaint->Draw(tps);

    Context->EndTarget();
  }
  
}

void App::OnKey(const sKeyData &kd)
{
  if((kd.Key&(sKEYQ_Mask|sKEYQ_Break))==27)
    sExit();
}

void App::OnDrag(const sDragData &dd)
{
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/


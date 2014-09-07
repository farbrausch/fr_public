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
  sRunApp(new App,sScreenMode(0,"Blur (with compute shader)",1280,720));
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
  cbc0 = 0;

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
  Mtrl->Prepare(Adapter->FormatPNT);


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

  static const sVertexPNT vb[vc] =
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
  Geo->SetVertex(sResBufferPara(sRBM_Vertex|sRU_Static,sizeof(sVertexPNT),vc),vb);
  Geo->Prepare(Adapter->FormatPNT,sGMP_Tris|sGMF_Index16);

  cbv0 = new sCBuffer<sFixedMaterialLightVC>(Adapter,sST_Vertex,0);
  cbc0 = new sCBuffer<BlurXShader_cbc0>(Adapter,sST_Compute,0);

  // blur

  CsSampler = Adapter->CreateSamplerState(sSamplerStatePara(sTF_Linear|sTF_Clamp,0));
  BlurX = new sMaterial(Adapter);
  BlurX->SetShaders(BlurXShader.Get(0));
  BlurY = new sMaterial(Adapter);
  BlurY->SetShaders(BlurYShader.Get(0));

  // debug
  //sEnablePerfMon(1);
  //sGpuPerfMon = new sPerfMonGpu(Adapter, "GPU");
}

void App::OnExit()
{
  delete Mtrl;
  delete BlurX;
  delete BlurY;
  delete Tex;
  delete Geo;
  delete cbv0;
  delete cbc0;
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
    rp = sResPara(sRBM_ColorTarget|sRU_Gpu|sRM_Texture|sRBM_Shader|sRBM_Unordered,mode.ColorFormat,sRES_NoMipmaps,mode.SizeX,mode.SizeY);

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

  for(sInt i=0;i<1;i++)
  {
    {
      sGPU_ZONE("Cube",0xff00ff00);
      Context->BeginTarget(tp);

      view.Camera.k.w = -3;
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

    int curRT = 0;


    {
      sGPU_ZONE("Blur",0xffa0e000);

      sF32 rad = 100 * (sin(time) + 1);
      rad *= rad / 100;

      cbc0->Map();
      cbc0->Data->TextureSize[0] = RtSizeX;
      cbc0->Data->TextureSize[1] = RtSizeY;
      cbc0->Data->BlurRadius.x = rad;
      cbc0->Data->BlurRadius.y = rad;
      cbc0->Unmap();

      int passes = 3;

      for (int i = 0; i < passes; i++)
      {
          BlurX->SetTexture(sST_Compute, 0, RtWork[curRT], CsSampler);
          BlurX->SetUav(sST_Compute, 0, RtWork[1-curRT]);
          BlurX->Prepare(0);
          Context->Draw(sDrawPara((RtSizeX+255) / 256, 1, 1, BlurX, cbc0));
          curRT = 1 - curRT;
      }

      for (int j = 0; j < passes; j++)
      {
          BlurY->SetTexture(sST_Compute, 0, RtWork[curRT], CsSampler);
          BlurY->SetUav(sST_Compute, 0, RtWork[1-curRT]);
          BlurY->Prepare(0);
          Context->Draw(sDrawPara(1, (RtSizeY+255) / 256, 1, BlurY, cbc0));
          curRT = 1 - curRT;
      }

    }

    {
      sGPU_ZONE("Back Blit",0xff00c0c0);
      Context->Copy(sCopyTexturePara(0,Screen->GetScreenColor(),RtWork[curRT]));
    }
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


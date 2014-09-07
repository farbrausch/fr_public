/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"
#include "shader.hpp"
#include "altona2/libs/util/graphicshelper.hpp"
#include "altona2/libs/util/image.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Altona2::Main()
{
  sRunApp(new App,sScreenMode(sSM_Fullscreen*0,"graphics helper",1280,720));
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

App::App()
{
  Mtrl = 0;
  MtrlBack = 0;
  Tex = 0;
  cbv0 = 0;
  DPaint = 0;
}

App::~App()
{
}

void App::OnInit()
{
  Screen = sGetScreen();
  Adapter = Screen->Adapter;
  Context = Adapter->ImmediateContext;

  DPaint = new sDebugPainter(Adapter);

  Tex = sLoadTexture(Adapter,"../../../exampledata/test256.png");

  Mtrl = new sMaterial(Adapter);
  Mtrl->SetShaders(CubeShader.Get(0));
  Mtrl->SetTexturePS(0,Tex,sSamplerStatePara(sTF_Linear|sTF_Clamp,0));
  Mtrl->SetState(sRenderStatePara(sMTRL_DepthOn|sMTRL_CullOff,sMB_Off));
  Mtrl->Prepare(Adapter->FormatPT);

  MtrlBack = new sFixedMaterial(Adapter);
  MtrlBack->SetState(sRenderStatePara(sMTRL_DepthOn|sMTRL_CullOff,sMB_Off));
  MtrlBack->Prepare(Adapter->FormatPC);

  cbv0 = new sCBuffer<CubeShader_cbvs>(Adapter,sST_Vertex,0);
}

void App::OnExit()
{
  delete Mtrl;
  delete MtrlBack;
  delete Tex;
  delete cbv0;
  delete DPaint;
}

void App::OnFrame()
{
}

void App::OnPaint()
{
  sF32 time = sGetTimeMS()*0.001f;
  sTargetPara tp(sTAR_ClearAll,0xff405060,Screen);
  sViewport view;

  Context->BeginTarget(tp);

  // render quads

  view.Camera = sEulerXYZ(time*0.11f,time*0.13f,time*0.15f);
  sVector40 v = sBaseZ(view.Camera);
  view.Camera.SetTrans(sVector41(v*-2.5f));
  view.ZoomX = 1/tp.Aspect;
  view.ZoomY = 1;
  view.Prepare(tp);

  cbv0->Map();
  cbv0->Data->ms2ss = view.MS2SS;
  cbv0->Data->tx = sVector3(view.MS2CS.i.x,view.MS2CS.i.y,view.MS2CS.i.z)*0.05f;
  cbv0->Data->ty = sVector3(view.MS2CS.j.x,view.MS2CS.j.y,view.MS2CS.j.z)*0.05f;
  cbv0->Unmap();

  sVertexPT *vp;
  sDynamicGeometry *dyn = Adapter->DynamicGeometry;
  dyn->Begin(Adapter->FormatPT,2,Mtrl,PartCount*4,0,sGMP_Quads);
  dyn->BeginVB(&vp);
  for(sInt i=0;i<PartCount;i++)
  {
    sF32 lx,ly,lz;

    lx = sSin(time*0.1f+s2Pi*i/sF32(PartCount)*3);
    ly = sSin(time*0.1f+s2Pi*i/sF32(PartCount)*4+0.3f);
    lz = sSin(time*0.1f+s2Pi*i/sF32(PartCount)*5+0.4f);

    vp[i*4+0].Set(lx,ly,lz,0,0);
    vp[i*4+1].Set(lx,ly,lz,0,1);
    vp[i*4+2].Set(lx,ly,lz,1,1);
    vp[i*4+3].Set(lx,ly,lz,1,0);
  }
  dyn->EndVB();
  dyn->End(cbv0);

  // render background

  sVertexPC *vp2;

  cbv0->Map();
  cbv0->Data->ms2ss.SetViewportScreen();
  cbv0->Unmap();

  dyn->Begin(Adapter->FormatPC,2,MtrlBack,4,0,sGMP_Quads);
  dyn->BeginVB(&vp2);
  vp2[0].Set(0,0,1,0xff6080a0);
  vp2[1].Set(1,0,1,0xff6080a0);
  vp2[2].Set(1,1,1,0xff204060);
  vp2[3].Set(0,1,1,0xff204060);
  dyn->EndVB();
  dyn->End(cbv0);

  // finish it

  dyn->Flush();

  DPaint->PrintFPS();
  DPaint->PrintStats();
  DPaint->Draw(tp);

  Context->EndTarget();
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


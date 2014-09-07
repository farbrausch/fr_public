/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"
#include "shader.hpp"

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Altona2::Main()
{
  sRunApp(new App,sScreenMode(sSM_Fullscreen*0,"dynamic vertices, using arrays",1280,720));
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
  cbv0 = 0;
  DPaint = 0;
  VertexBuffer = 0;
  VertexCount = 0;
  IndexBuffer = 0;
  IndexCount = 0;
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

  const sInt sx=64;
  const sInt sy=64;
  sU32 tex[sy][sx];
  for(sInt y=0;y<sy;y++)
    for(sInt x=0;x<sx;x++)
      tex[y][x] = ((x^y)&8) ? 0xffff8080 : 0xff80ff80;
  Tex = new sResource(Adapter,sResTexPara(sRF_BGRA8888,sx,sy,1),&tex[0][0],sizeof(tex));


  Mtrl = new sMaterial(Adapter);
  Mtrl->SetShaders(CubeShader.Get(0));
  Mtrl->SetTexturePS(0,Tex,sSamplerStatePara(sTF_Linear|sTF_Clamp,0));
  Mtrl->SetState(sRenderStatePara(sMTRL_DepthOn|sMTRL_CullOff,sMB_Off));
  Mtrl->Prepare(Adapter->FormatPT);

  IndexCount = PartCount*6;
  VertexCount = PartCount*4;
  IndexBuffer = new sU16[IndexCount];
  for(sInt i=0;i<PartCount;i++)
  {
    IndexBuffer[i*6+0] = i*4+0;
    IndexBuffer[i*6+1] = i*4+1;
    IndexBuffer[i*6+2] = i*4+2;
    IndexBuffer[i*6+3] = i*4+0;
    IndexBuffer[i*6+4] = i*4+2;
    IndexBuffer[i*6+5] = i*4+3;
  }

  VertexBuffer = new sVertexPT[VertexCount];

  cbv0 = new sCBuffer<CubeShader_cbvs>(Adapter,sST_Vertex,0);
}

void App::OnExit()
{
  delete Mtrl;
  delete Tex;
  delete cbv0;
  delete DPaint;
  delete[] VertexBuffer;
  delete[] IndexBuffer;
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

  view.Camera = sEulerXYZ(time*0.11f,time*0.13f,time*0.15f);
  sVector40 v = sBaseZ(view.Camera);
  view.Camera.SetTrans(sVector41(v*-2.5f));
  view.ZoomX = 1/tp.Aspect;
  view.ZoomY = 1;
  view.Prepare(tp);

  const sF32 f = 0.05f;
  cbv0->Map();
  cbv0->Data->ms2ss = view.MS2SS;
  cbv0->Data->tx = sVector3(view.MS2CS.i.x,view.MS2CS.i.y,view.MS2CS.i.z)*f;
  cbv0->Data->ty = sVector3(view.MS2CS.j.x,view.MS2CS.j.y,view.MS2CS.j.z)*f;
  cbv0->Unmap();


  sVertexPT *vp = VertexBuffer;
  for(sInt i=0;i<PartCount;i++)
  {
    sF32 lx,ly,lz;
    sF32 f = 0.0f;

    lx = sSin(time*0.1f+s2Pi*i/sF32(PartCount)*3);
    ly = sSin(time*0.1f+s2Pi*i/sF32(PartCount)*4+0.3f);
    lz = sSin(time*0.1f+s2Pi*i/sF32(PartCount)*5+0.4f);

    vp[i*4+0].Set(lx+f,ly+f,lz,0,0);
    vp[i*4+1].Set(lx+f,ly-f,lz,0,1);
    vp[i*4+2].Set(lx-f,ly-f,lz,1,1);
    vp[i*4+3].Set(lx-f,ly+f,lz,1,0);
  }

  sDrawPara dp(0,Mtrl,cbv0);
  dp.Flags |= sDF_Arrays;
  dp.VertexArray = VertexBuffer;
  dp.VertexArrayFormat = Adapter->FormatPT;
  dp.VertexArrayCount = VertexCount;
  dp.IndexArray = IndexBuffer;
  dp.IndexArraySize = sizeof(sU16);
  dp.IndexArrayCount = IndexCount;
  Context->Draw(dp);

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


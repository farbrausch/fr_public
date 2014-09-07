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
  sRunApp(new App,sScreenMode(sSM_Fullscreen*0,"dynamic vertices, using update",1280,720));
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

  const sInt ic = PartCount*6;
  const sInt vc = PartCount*4;
  sU16 ib[ic];
  for(sInt i=0;i<PartCount;i++)
  {
    ib[i*6+0] = i*4+0;
    ib[i*6+1] = i*4+1;
    ib[i*6+2] = i*4+2;
    ib[i*6+3] = i*4+0;
    ib[i*6+4] = i*4+2;
    ib[i*6+5] = i*4+3;
  }

  Geo = new sGeometry(Adapter);
  Geo->SetIndex(sResBufferPara(sRBM_Index|sRU_Static,sizeof(sU16),ic),ib);
  Geo->SetVertex(sResBufferPara(sRBM_Vertex|sRU_Update,sizeof(sVertexPT),vc),0);
  Geo->Prepare(Adapter->FormatPT,sGMP_Tris|sGMF_Index16);

  VertexBuffer = new sVertexPT[PartCount*4];

  cbv0 = new sCBuffer<CubeShader_cbvs>(Adapter,sST_Vertex,0);
}

void App::OnExit()
{
  delete Mtrl;
  delete Tex;
  delete Geo;
  delete cbv0;
  delete DPaint;
  delete[] VertexBuffer;
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
//  Geo->VB(0)->MapBuffer(&vp);
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
//  Geo->VB(0)->Unmap();
  Geo->VB(0)->UpdateBuffer(VertexBuffer,0,sizeof(sVertexPT)*PartCount*4);


  Context->Draw(sDrawPara(Geo,Mtrl,cbv0));

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


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
  sRunApp(new App,sScreenMode(sSM_Fullscreen*0,"instances",1280,720));
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
}

App::~App()
{
}

void App::OnInit()
{
  Screen = sGetScreen();
  Adapter = Screen->Adapter;
  Context = Adapter->ImmediateContext;

  static const sU32 vfdesc[] =
  {
    sVF_Stream1|sVF_Position|sVF_InstanceData,
    sVF_Stream0|sVF_UV0,
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


  Mtrl = new sMaterial(Adapter);
  Mtrl->SetShaders(CubeShader.Get(0));
  Mtrl->SetTexturePS(0,Tex,sSamplerStatePara(sTF_Linear|sTF_Clamp,0));
  Mtrl->SetState(sRenderStatePara(sMTRL_DepthOn|sMTRL_CullOff,sMB_Off));
  Mtrl->Prepare(Format);

  const sInt ic = 6;
  sU16 ib[ic];
  vert0 vb[4];
  for(sInt i=0;i<1;i++)
  {
    ib[i*6+0] = i*4+0;
    ib[i*6+1] = i*4+1;
    ib[i*6+2] = i*4+2;
    ib[i*6+3] = i*4+0;
    ib[i*6+4] = i*4+2;
    ib[i*6+5] = i*4+3;
  }
  vb[0].Set(0,0);
  vb[1].Set(0,1);
  vb[2].Set(1,1);
  vb[3].Set(1,0);

  Geo = new sGeometry(Adapter);
  Geo->SetIndex(sResBufferPara(sRBM_Index|sRU_Static,sizeof(sU16),ic),ib);
  Geo->SetVertex(sResBufferPara(sRBM_Vertex|sRU_Static,sizeof(vert0),4),vb,0,1);
  Geo->SetVertex(sResBufferPara(sRBM_Vertex|sRU_MapWrite,sizeof(vert1),PartCount),0,1,1);
  Geo->Prepare(Format,sGMP_Tris|sGMF_Index16,ic,0,4,0);

  cbv0 = new sCBuffer<CubeShader_cbvs>(Adapter,sST_Vertex,0);

  DPaint = new sDebugPainter(Adapter);
}

void App::OnExit()
{
  delete DPaint;
  delete Mtrl;
  delete Tex;
  delete Geo;
  delete cbv0;
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

  vert1 *vp;
  Geo->VB(1)->MapBuffer(&vp);
  for(sInt i=0;i<PartCount;i++)
  {
    sF32 lx,ly,lz;
    sF32 f = 0.0f;

    lx = sSin(time*0.1f+s2Pi*i/sF32(PartCount)*3);
    ly = sSin(time*0.1f+s2Pi*i/sF32(PartCount)*4+0.3f);
    lz = sSin(time*0.1f+s2Pi*i/sF32(PartCount)*5+0.4f);

    vp[i].Set(lx,ly,lz);
  }
  Geo->VB(1)->Unmap();


  sDrawPara dp(Geo,Mtrl,cbv0);
  dp.Flags |= sDF_Instances;
  dp.InstanceCount = PartCount;
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


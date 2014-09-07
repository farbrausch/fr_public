/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"
#include "altona2/libs/util/image.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Altona2::Main()
{
  sInt flags = 0;
//  flags |= sSM_Fullscreen;
  sRunApp(new App,sScreenMode(flags,"mipmaps",1280,720));
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

  DPaint= new sDebugPainter(Adapter);

  const sInt sx=64;
  const sInt sy=64;
  sU32 tex[sy*sx+sy*sx/4+sy*sx/16];
  for(sInt y=0;y<sy;y++)
    for(sInt x=0;x<sx;x++)
      tex[y*sx+x] = ((x^y)&8) ? 0xffff8080 : 0xff80ff80;

  sPrepareTexture2D pt;
  pt.Load(tex,sx,sy);
  pt.Process();

  Tex = new sResource(Adapter,sResTexPara(sRF_BGRA8888,sx,sy,pt.Mipmaps),pt.GetData(),pt.GetSize());

  Mtrl = new sFixedMaterial(Adapter);
  Mtrl->SetTexturePS(0,Tex,sSamplerStatePara(sTF_Linear|sTF_Tile,0));
  Mtrl->SetState(sRenderStatePara(sMTRL_DepthOn|sMTRL_CullOff,sMB_Off));
  Mtrl->Prepare(Adapter->FormatPT);


  const sInt ic = 1*6;
  const sInt vc = 1*4;
  static const sU16 ib[ic] =
  {
     0, 1, 2, 0, 2, 3,
  };
  struct vert
  {
    sF32 px,py,pz;
    sF32 u0,v0;
  };
  const sF32 f=16;
  static const vert vb[vc] =
  {

    { -1*f,0,-1*f,   0*f,0*f, },
    {  1*f,0,-1*f,   1*f,0*f, },
    {  1*f,0, 1*f,   1*f,1*f, },
    { -1*f,0, 1*f,   0*f,1*f, },

  };

  Geo = new sGeometry(Adapter);
  Geo->SetIndex(sResBufferPara(sRBM_Index|sRU_Static,sizeof(sU16),ic),ib,1);
  Geo->SetVertex(sResBufferPara(sRBM_Vertex|sRU_Static,sizeof(vert),vc),vb);
  Geo->Prepare(Adapter->FormatPT,sGMP_Tris|sGMF_Index16);

  cbv0 = new sCBuffer<sFixedMaterialVC>(Adapter,sST_Vertex,0);
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

  view.Camera.SetTrans(sVector41(0,0.5f,-5.0f));
  view.Model = sEulerXYZ(0,time*0.13f,0);
  view.ZoomX = 1/tp.Aspect;
  view.ZoomY = 1;
  view.Prepare(tp);

  cbv0->Map();
  cbv0->Data->Set(view);
  cbv0->Unmap();

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


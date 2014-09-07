/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"
#include "altona2/libs/util/graphicshelper.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Altona2::Main()
{
  sScreenMode sm(sSM_NoInitialScreen,0,0,0);
  sm.Monitor = 0;
  sRunApp(new App,sm);
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

AdapterInfo::AdapterInfo(sAdapter *ada)
{
  Mtrl = 0;
  Tex = 0;
  Geo = 0;
  DPaint = 0;
  cbv0 = 0;

  Adapter = ada;
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
}

AdapterInfo::~AdapterInfo()
{
  delete Mtrl;
  delete Tex;
  delete Geo;
  delete cbv0;
  delete DPaint;
}

void AdapterInfo::Draw(sTargetPara &tp,float time)
{
  sViewport view;

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

  DPaint->PrintFPS();
  DPaint->PrintStats();
  DPaint->Draw(tp);

  Context->EndTarget();
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

App::App()
{
  Screen = 0;
}

App::~App()
{
}

void App::OnInit()
{
  for(sInt i=0;i<sGetDisplayCount();i++)
  {
    sScreenMode sm(sSM_FullWindowed,"MultiMon",1280,720);
    sm.Monitor = i;
    Screens.Add(new sScreen(sm));
  }

  for(auto scr : Screens)
  {
    AdapterInfo *info = AdapterInfos.Find([&scr](const AdapterInfo *info){return info->Adapter==scr->Adapter;});
    if(!info)
    {
      AdapterInfos.Add(new AdapterInfo(scr->Adapter));
    }
  }
}

void App::OnExit()
{
  AdapterInfos.DeleteAll();
}

void App::OnFrame()
{
}

void App::OnPaint()
{
  for(auto &scr : Screens)
  {
    AdapterInfo *info = AdapterInfos.Find([scr](const AdapterInfo *info){return info->Adapter==scr->Adapter;});
    sF32 time = sGetTimeMS()*0.01f;
    sTargetPara tp(sTAR_ClearAll,0xff405060,scr);

    info->Draw(tp,time*(1.0f+sGetIndex(Screens,scr)*0.2f));
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


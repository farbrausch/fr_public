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

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Altona2::Main()
{
  sInt flags = 0;
//  flags |= sSM_Fullscreen;
  sRunApp(new App,sScreenMode(flags,"tesselation",1280,720));
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

App::App()
{
  Mtrl[0] = 0;
  Mtrl[1] = 0;
  Geo = 0;
  Wireframe = 1;
  Quality = 1;
  FreeCam = 0;
}

App::~App()
{
}

void App::OnInit()
{
  Screen = sGetScreen();
  Adapter = Screen->Adapter;
  Context = Adapter->ImmediateContext;

  for(sInt i=0;i<2;i++)
  {
    Mtrl[i] = new sMaterial(Adapter);
    Mtrl[i]->SetShaders(TesselateShader.Get(i));
    Mtrl[i]->SetState(sRenderStatePara(sMTRL_DepthOn|sMTRL_CullOn,sMB_Off));
    Mtrl[i]->Prepare(Adapter->FormatPNT);
  }

  const sInt tx = 20;
  const sInt ty = 8;

  const sInt ic = tx*ty*6;
  const sInt vc = (tx+1)*(ty+1);
  sU16 ib[ic];
  sVertexPNT vb[vc];
  sF32 ri = 0.50f;
  sF32 ro = 1.25f;

  sVertexPNT *vp = vb;
  for(sInt y=0;y<=ty;y++)
  {
    sF32 fy = sF32(y)/ty*s2Pi;
    sF32 py = y!=ty ? fy : 0;
    for(sInt x=0;x<=tx;x++)
    {
      sF32 fx = sF32(x)/tx*s2Pi;
      sF32 px = x!=tx ? fx : 0;
      vp->px = -sCos(px)*(ro+sCos(py)*ri);
      vp->py = -             sSin(py)*ri;
      vp->pz =  sSin(px)*(ro+sCos(py)*ri);
      vp->nx = -sCos(px)*(   sCos(py));
      vp->ny = -             sSin(py);
      vp->nz =  sSin(px)*(   sCos(py));
      vp->u0 = sF32(x)/tx;
      vp->v0 = sF32(y)/ty;
      vp++;
    }
  }
  sU16 *ip = ib;
  for(sInt y=0;y<ty;y++)
  {
    for(sInt x=0;x<tx;x++)
    {
      sQuad(ip,0,(x+0) + (y+0)*(tx+1),
                 (x+0) + (y+1)*(tx+1),
                 (x+1) + (y+1)*(tx+1),
                 (x+1) + (y+0)*(tx+1));
    }
  }

  Geo = new sGeometry(Adapter);
  Geo->SetIndex(sResBufferPara(sRBM_Index|sRU_Static,sizeof(sU16),ic),ib);
  Geo->SetVertex(sResBufferPara(sRBM_Vertex|sRU_Static,sizeof(sVertexPNT),vc),vb);
  Geo->Prepare(Adapter->FormatPNT,sGMP_ControlPoints|3|sGMF_Index16);

  cbh0 = new sCBuffer<TesselateShader_cbh0>(Adapter,sST_Hull,0);
  cbd0 = new sCBuffer<TesselateShader_cbd0>(Adapter,sST_Domain,0);

  DPaint = new sDebugPainter(Adapter);
}

void App::OnExit()
{
  delete DPaint;
  delete Mtrl[0];
  delete Mtrl[1];
  delete Geo;
  delete cbh0;
  delete cbd0;
}

void App::OnFrame()
{
}

void App::OnPaint()
{
  sU32 utime = sGetTimeMS();
  sF32 time = utime*0.01f;
  sTargetPara tp(sTAR_ClearAll,0xff405060,Screen);
  sViewport view;

  Context->BeginTarget(tp);

  if(FreeCam)
  {
    sInt x,y,b;
    sGetMouse(x,y,b);
    view.Model = sEulerXYZ(y*0.01f,x*0.01f,0);
  }
  else
  {
    view.Model = sEulerXYZ(time*0.031f,time*0.043f,time*0.055f);
  }

  view.Camera.k.w = -2.0f;
  view.ZoomX = 0.5f/tp.Aspect;
  view.ZoomY = 0.5f;
  view.Prepare(tp);

  sScreenMode mode;
  Screen->GetMode(mode);
  static const sF32 quality[3] = { 0.35f,0.7f,1.5f };

  cbh0->Map();
  cbh0->Data->MS2SS = view.MS2SS;
  cbh0->Data->txx.Set(view.MS2CS.i*-1);
  cbh0->Data->Quality = mode.SizeX * quality[Quality]*0.05f;
  cbh0->Unmap();

  cbd0->Map();
  cbd0->Data->MS2SS = view.MS2SS;
  cbd0->Data->ldir.Set(view.MS2CS.k*-1);
  cbd0->Unmap();

  Context->Draw(sDrawPara(Geo,Mtrl[Wireframe],cbh0,cbd0));

  DPaint->PrintFPS();
  DPaint->PrintStats();
  DPaint->FramePrint("^ for wireframe\n");
  DPaint->FramePrint("f for free camera\n");
  DPaint->FramePrint("1 2 3 for quality\n");
  DPaint->Draw(tp);

  Context->EndTarget();
}

void App::OnKey(const sKeyData &kd)
{
  if((kd.Key&(sKEYQ_Mask|sKEYQ_Break))==27)
    sExit();


  if(kd.Key=='^') Wireframe = !Wireframe;
  if(kd.Key=='f') FreeCam = !FreeCam;
  if(kd.Key>='1' && kd.Key<='3') Quality = kd.Key-'1';
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


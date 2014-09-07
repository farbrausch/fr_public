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

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Altona2::Main()
{
  sInt flags = 0;
//  flags |= sSM_Fullscreen;
  sRunApp(new App,sScreenMode(flags,"pointsprites (using gs)",1280,720));
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

  const sInt sx=64;
  const sInt sy=64;
  sU32 tex[sy][sx];
  for(sInt y=0;y<sy;y++)
    for(sInt x=0;x<sx;x++)
      tex[y][x] = ((x^y)&8) ? 0xffff8080 : 0xff80ff80;
  Tex = new sResource(Adapter,sResTexPara(sRF_BGRA8888,sx,sy,1),&tex[0][0],sizeof(tex));


  Mtrl = new sMaterial(Adapter);
  Mtrl->SetShaders(PointSpritesShader.Get(0));
  Mtrl->SetTexturePS(0,Tex,sSamplerStatePara(sTF_Linear|sTF_Clamp,0));
  Mtrl->SetState(sRenderStatePara(sMTRL_DepthOn|sMTRL_CullOn,sMB_Off));
  Mtrl->Prepare(Adapter->FormatPNT);


  const sInt tx = 16;
  const sInt ty = 8;

  const sInt vc = (tx+1)*(ty+1);
  sVertexPNT vb[vc];
  sF32 ri = 0.25f;
  sF32 ro = 1.00f;

  sVertexPNT *vp = vb;
  for(sInt y=0;y<ty;y++)
  {
    sF32 fy = sF32(y)/ty*s2Pi;
    sF32 py = y!=ty ? fy : 0;
    for(sInt x=0;x<tx;x++)
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

  Geo = new sGeometry(Adapter);
  Geo->SetVertex(sResBufferPara(sRBM_Vertex|sRU_Static,sizeof(sVertexPNT),vc),vb);
  Geo->Prepare(Adapter->FormatPNT,sGMP_Points,0,0,vc,0);

  cbg0 = new sCBuffer<PointSpritesShader_cbgs>(Adapter,sST_Geometry,0);

  DPaint = new sDebugPainter(Adapter);
}

void App::OnExit()
{
  delete DPaint;
  delete Mtrl;
  delete Tex;
  delete Geo;
  delete cbg0;
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

  view.Camera.k.w = -2;
  view.Model = sEulerXYZ(time*0.031f,time*0.043f,time*0.055f);
  view.ZoomX = 1/tp.Aspect;
  view.ZoomY = 1;
  view.Prepare(tp);

  cbg0->Map();
  cbg0->Data->mat = view.MS2SS;
  cbg0->Data->ttx = sVector3(-view.MS2CS.i.x,-view.MS2CS.i.y,-view.MS2CS.i.z)*0.05f;
  cbg0->Data->tty = sVector3(-view.MS2CS.j.x,-view.MS2CS.j.y,-view.MS2CS.j.z)*0.05f;
//  cbg0->Data->ldir.Set(-view.MS2CS.k.x,-view.MS2CS.k.y,-view.MS2CS.k.z);
  cbg0->Unmap();

  Context->Draw(sDrawPara(Geo,Mtrl,cbg0));

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


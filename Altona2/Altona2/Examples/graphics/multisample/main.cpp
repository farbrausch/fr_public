/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"
#include "altona2/libs/util/graphicshelper.hpp"

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Altona2::Main()
{
  sScreenMode sm(sSM_Fullscreen*0,"multisampling",1280,720);
  sm.DepthFormat = 0;
  sRunApp(new App,sm);
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

  TestMtrl = new sFixedMaterial(Adapter);
  TestMtrl->SetState(sRenderStatePara(sMTRL_DepthOff|sMTRL_CullOff,sMB_Off));
  TestMtrl->Prepare(Adapter->FormatPC);

  const sInt count=16;
  const sInt size=4;
  const sInt tvc = count*count*size*size*4;
  const sInt tic = count*count*size*size*6;
  sVertexPC tvb[tvc];
  sU16 tib[tic];
  sU16 *tip = tib;
  sInt n = 0;
  const sF32 f = 1.0f/count;
  for(sInt y0=0;y0<count;y0++)
  {
    for(sInt x0=0;x0<count;x0++)
    {
      for(sInt y1=0;y1<size;y1++)
      {
        for(sInt x1=0;x1<size;x1++)
        {
          sFRect r;
          r.x0 = sF32(x0*size+x1)+(x0+0)*f+10;
          r.y0 = sF32(y0*size+y1)+(y0+0)*f+100;
          r.x1 = sF32(x0*size+x1)+(x0+1)*f+10;
          r.y1 = sF32(y0*size+y1)+(y0+1)*f+100;
          tvb[n*4+0].Set(r.x0,r.y0,0,0xffffffff);
          tvb[n*4+1].Set(r.x1,r.y0,0,0xffffffff);
          tvb[n*4+2].Set(r.x1,r.y1,0,0xffffffff);
          tvb[n*4+3].Set(r.x0,r.y1,0,0xffffffff);
          sQuad(tip,n*4,0,1,2,3);
          n++;
        } 
      }
    }
  }

  TestGeo = new sGeometry(Adapter);
  TestGeo->SetIndex(sResBufferPara(sRBM_Index|sRU_Static,sizeof(sU16),tic),tib);
  TestGeo->SetVertex(sResBufferPara(sRBM_Vertex|sRU_Static,sizeof(sVertexPC),tvc),tvb);
  TestGeo->Prepare(Adapter->FormatPC,sGMP_Tris|sGMF_Index16);
  
  RtDepth = 0;
  RtColor = 0;
  RtSizeX = -1;
  RtSizeY = -1;

  Cursor = 0;
  MSCount = 0;
  MSQuality = 0;
}

void App::OnExit()
{
  delete Mtrl;
  delete Tex;
  delete Geo;
  delete TestMtrl;
  delete TestGeo;
  delete cbv0;
  delete DPaint;
  delete RtDepth;
  delete RtColor;
}

void App::OnFrame()
{
}

void App::OnPaint()
{
  sScreenMode mode;
  Screen->GetMode(mode);
  sInt sx = mode.SizeX;
  sInt sy = mode.SizeY;
  if(sx*sy==0)
    return;
  if(sx!=RtSizeX || sy!=RtSizeY)
  {
    sDelete(RtColor);
    sDelete(RtDepth);
    RtSizeX = sx;
    RtSizeY = sy;
    sResPara pc(sRBM_ColorTarget|sRU_Gpu|sRM_Texture,mode.ColorFormat,sRES_NoMipmaps,sx,sy);
    sResPara pd(sRBM_DepthTarget|sRU_Gpu|sRM_Texture,sRF_D24S8,sRES_NoMipmaps,sx,sy);
    sInt bestmscount;
    sInt bestmsquality;
    Adapter->GetBestMultisampling(pc,bestmscount,bestmsquality);
    MSCount = sClamp(MSCount,0,sFindLowerPower(bestmscount));
    MSQuality = sClamp(MSQuality,0,Adapter->GetMultisampleQuality(pc,1<<MSCount)-1);
    pc.MSCount = pd.MSCount = 1<<MSCount;
    pc.MSQuality = pd.MSQuality = MSQuality;
    RtColor = new sResource(Adapter,pc);
    RtDepth = new sResource(Adapter,pd);
  }


  sF32 time = sGetTimeMS()*0.01f;
  sTargetPara tp(sTAR_ClearAll,0xff405060,RtColor,RtDepth);

  Context->BeginTarget(tp);

  sViewport view;
  view.Camera.k.w = -4;
  view.Model = sEulerXYZ(time*0.011f,time*0.013f,time*0.015f);
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

  sViewport view2;
  view2.Mode = sVM_Pixels;
  view2.Prepare(tp);

  cbv0->Map();
  cbv0->Data->Set(view2,lp);
  cbv0->Unmap();

  Context->Draw(sDrawPara(TestGeo,TestMtrl,cbv0));


  DPaint->PrintFPS();
  DPaint->PrintStats();
  DPaint->FramePrintF("\n\n");
  DPaint->FramePrintF("%s Sample Count: %d\n",Cursor==0?"-->":"   ",1<<MSCount);
  DPaint->FramePrintF("%s Sample Quality: %d\n",Cursor==1?"-->":"   ",MSQuality);
  DPaint->Draw(tp);

  Context->EndTarget();

  Context->Copy(sCopyTexturePara(0,Screen->GetScreenColor(),RtColor));
}

void App::OnKey(const sKeyData &kd)
{
  if((kd.Key&(sKEYQ_Mask|sKEYQ_Break))==27)
    sExit();
  switch(kd.Key)
  {
  case sKEY_Up:
    Cursor = sClamp(Cursor-1,0,1);
    break;
  case sKEY_Down:
    Cursor = sClamp(Cursor+1,0,1);
    break;
  case sKEY_Left:
    if(Cursor==0)
      MSCount = sClamp(MSCount-1,0,5);
    else
      MSQuality = sClamp(MSQuality-1,0,128);
    RtSizeX = -1;
    break;
  case sKEY_Right:
    if(Cursor==0)
      MSCount = sClamp(MSCount+1,0,5);
    else
      MSQuality = sClamp(MSQuality+1,0,128);
    RtSizeX = -1;
    break;
  }
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


/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Altona2::Main()
{
    sRunApp(new App,sScreenMode(sSM_Fullscreen*0,"log input",1280,720));
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

#if sConfigPlatform==sConfigPlatformWin
    sInitWacom();
#endif

  DPaint = new sDebugPainter(Adapter);

  static const sU32 vfdesc[] =
  {
    sVF_Position,
    sVF_Normal,
    sVF_UV0,
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


  Mtrl = new sFixedMaterial(Adapter);
  Mtrl->SetTexturePS(0,Tex,sSamplerStatePara(sTF_Linear|sTF_Clamp,0));
  Mtrl->SetState(sRenderStatePara(sMTRL_DepthOn|sMTRL_CullOn,sMB_Off));
  Mtrl->Prepare(Format);


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
  struct vert
  {
    sF32 px,py,pz;
    sF32 nx,ny,nz;
    sF32 u0,v0;
  };
  static const vert vb[vc] =
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
  Geo->SetVertex(sResBufferPara(sRBM_Vertex|sRU_Static,sizeof(vert),vc),vb);
  Geo->Prepare(Format,sGMP_Tris|sGMF_Index16,ic,0,vc,0);

  cbv0 = new sCBuffer<sFixedMaterialLightVC>(Adapter,sST_Vertex,0);
}

void App::OnExit()
{
  delete Mtrl;
  delete Tex;
  delete Geo;
  delete cbv0;
  delete DPaint;
}

void App::OnFrame()
{
}

void App::OnPaint()
{
  sF32 time = sGetTimeMS()*0.01f;
  sTargetPara tp(sTAR_ClearAll,0xff405060,Screen);
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
  DPaint->FramePrintF("qual: %08x\n",sGetKeyQualifier());
  DPaint->Draw(tp);

  Context->EndTarget();
}

void App::OnKey(const sKeyData &kd)
{
  sU32 mkey = kd.Key & sKEYQ_Mask;
  if(mkey==27)
    sExit();
  if(mkey>=0x20 && mkey<0xff)
    DPaint->ConsolePrintF("%08x key %08x '%c'\n",kd.Timestamp,kd.Key,mkey&sKEYQ_Mask);
  else
    DPaint->ConsolePrintF("%08x key %08x\n",kd.Timestamp,kd.Key);
}

void App::OnDrag(const sDragData &dd)
{
  static const sChar *modes[] = { "hover","start","drag","stop" };
  if(sConfigPlatform != sConfigPlatformIOS)
  {
    DPaint->ConsolePrintF("%08x mouse %-5s : %5d %5d %1x %5f\n",dd.Timestamp,modes[dd.Mode],dd.PosX,dd.PosY,dd.Buttons,dd.Pressure);
  }
  else // multitouch
  {
    DPaint->ConsolePrintF("%08x mouse %-5s : %5d %5d %1x",dd.Timestamp,modes[dd.Mode],dd.PosX,dd.PosY,dd.Buttons);  
    for(sInt i=0;i<dd.TouchCount;i++)
    {
      if(dd.Touches[i].Mode!=sDM_Hover)
        DPaint->ConsolePrintF(" --- %04d %04d %-5s",dd.Touches[i].PosX,dd.Touches[i].PosY,modes[dd.Touches[i].Mode]);
      else
        DPaint->ConsolePrintF(" ---                ");
    }
    DPaint->ConsolePrintF("\n");
  }
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/


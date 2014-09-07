/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"
#include "altona2/libs/util/image.hpp"

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Altona2::Main()
{
  sRunApp(new App,sScreenMode(sSM_Fullscreen*0,"painter",1280,720));
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
  Painter = 0;
  cbv0 = 0;
  PImg[0] = 0;
  PImg[1] = 0;
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
  Painter = new sPainter(Adapter);

  if(0)
  {
    /*
#define PREFIX "../../../exampledata/"
    
    sImage img;
    sBool ok;

    PImg[0] = new sPainterImage(Adapter,PREFIX "frame32.bmp",0);
    if(1)
    {
      ok = img.Load(PREFIX "test256.png");
      sASSERT(ok);
      img.Save(PREFIX "save_test.png");
      img.Save(PREFIX "save_test.bmp");

      sSaveObject(&img,PREFIX "save_test.bin");
      img.Clear(0xffff0000);
      img.Init(1,1);
      PFont = new sPainterFont(Adapter,sSFI_SansSerif,0,16);
      sSaveObject(PFont,PREFIX "save_font.pfont");
      delete PFont; 
    }

    sLoadObject(&img,PREFIX "save_test.bin");

    PImg[1] = new sPainterImage(Adapter,&img,0);

    PFont = new sPainterFont(Adapter);
    sLoadObject<sPainterFont>(PFont,PREFIX "save_font.pfont");
    */
  }
  else
  {
    sAddPackfile("packfile.pak");
    PFont = new sPainterFont(Adapter);
#if sConfigPlatform!=sConfigPlatformWin
    sLoadObject(PFont,"save_font.pfont");
#else
    PFont = new sPainterFont(Adapter,"georgia",0,20);
    TFont = Painter->GetFont(sFontPara("Arial",16,sSFF_DistanceField));
#endif
    PImg[0] = new sPainterImage(Adapter,"frame32.bmp",0);
    PImg[1] = new sPainterImage(Adapter,"test256.png",0);
  }

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
  Geo->Prepare(Adapter->FormatPNT,sGMP_Tris|sGMF_Index16,ic,0,vc,0);

  cbv0 = new sCBuffer<sFixedMaterialLightVC>(Adapter,sST_Vertex,0);
}

void App::OnExit()
{
  delete Mtrl;
  delete Tex;
  delete Geo;
  delete cbv0;
  delete DPaint;
  delete Painter;

  delete PFont;
  delete PImg[0];
  delete PImg[1];
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

  //

  Painter->Begin(tp);

  if(!ClipRect.IsEmpty())
    Painter->SetClip(ClipRect);

  Painter->Rect (0xffffffff,sFRect(110,110,190,190));
    // Painter->Line (0xffffffff,sFRect(110,310,190,390));
  Painter->Frame(0xffffffff,sFRect(110,210,190,290));
  Painter->Image(PImg[0],0xffffffff,210,110);
  Painter->Image(PImg[1],0xffffffff,210,210);
  Painter->Image(PFont->GetImage( ),0xffffffff,190-128,410);
#if sConfigPlatform!=sConfigPlatformWin
  Painter->SetFont(sPainterFontPara(PFont));
#else
  Painter->SetFont(TFont);
#endif
  Painter->SetLayer(2);
  Painter->Text(sPPF_Top,0xffffffff,sFRect(210,210,210+256,210+256),"hello world");
  Painter->Text(sPPF_Bottom,0xffffffff,sFRect(210,210,210+256,210+256),"hello world");
  Painter->Text(sPPF_Top|sPPF_Downwards,0xffffffff,sFRect(210,210,210+256,210+256),"hello world");
  Painter->Text(sPPF_Bottom|sPPF_Upwards,0xffffffff,sFRect(210,210,210+256,210+256),"hello world");
  for(sInt i=0;i<5;i++)
  {
    static const sInt flags[5] = { 0,sPPF_Left,sPPF_Right,sPPF_Top,sPPF_Bottom };
#if sConfigPlatform==sConfigPlatformIOS || 1
    sFRect r(500.0f,110.0f+i*50,760.0f,140.0f+i*50);
#else
    sFRect r(900.0f,110.0f+i*50,1200.0f,140.0f+i*50);
#endif
    Painter->SetLayer(1);
    Painter->Text (flags[i],0xff000000,r,"hello, world!");
    r.Extend(1);
    Painter->SetLayer(0);
    Painter->Rect(0xffffffff,r);
    Painter->SetLayer(1);
    Painter->Frame(0xff808080,r);
  }
  if(!ClipRect.IsEmpty())
  {
    Painter->ResetClip();
    sRect r(ClipRect);
    r.Extend(2);
    Painter->Frame(0xffff0000,r);
  }

  Painter->End(Context);

  //

  DPaint->PrintFPS();
  DPaint->PrintStats();
  DPaint->FramePrint("press LMB to test clipping\n");
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
  if(dd.Mode==sDM_Drag)
    ClipRect.Set(dd.PosX-50,dd.PosY-50,dd.PosX+50,dd.PosY+50);
  else
    ClipRect.Set();
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/


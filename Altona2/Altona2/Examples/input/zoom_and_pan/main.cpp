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
  sRunApp(new App,sScreenMode(sSM_Fullscreen*0,"zoom and pan",768,1024));
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

App::App()
{
  DPaint = 0;
  Painter = 0;
  PImg[0] = 0;
  PImg[1] = 0;

  PosX0 = 40;
  PosX1 = 60;
  PosY = 0;

  Round = 1;
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

  sAddPackfile("packfile.pak");
  PFont = new sPainterFont(Adapter);
  sLoadObject(PFont,"save_font.pfont");
  PImg[0] = new sPainterImage(Adapter,"frame32.bmp",0);
  PImg[1] = new sPainterImage(Adapter,"test256.png",0);
}

void App::OnExit()
{
  delete PFont;
  delete PImg[0];
  delete PImg[1];

  delete DPaint;
  delete Painter;
}

void App::OnFrame()
{
}

void App::OnPaint()
{
  sF32 time = sF32(sGetTimeMS())*0.01f;
  sTargetPara tp(sTAR_ClearAll,0xffffffff,Screen);

  Context->BeginTarget(tp);

  // physics

  if(DragMode==DM_ScrollX || DragMode==DM_ScrollXY || DragMode==DM_Zoom || DragMode==DM_ZoomRMB)
  {
    SpeedX0 = (PosX0 - OldX0[1])*0.5f;
    SpeedX1 = (PosX1 - OldX1[1])*0.5f;
    SpeedY  = (PosY  - OldY [1])*0.5f;
  }

  OldX0[1] = OldX0[0]; OldX0[0] = PosX0;
  OldX1[1] = OldX1[0]; OldX1[0] = PosX1;
  OldY [1] = OldY [0]; OldY [0] = PosY ;
  if(DragMode==DM_Off)
  {
    const sF32 minspeedx = 0.125f/ScreenX*(PosX1-PosX0);
    PosX0 += SpeedX0; SpeedX0 *= 0.90f; if(sAbs(SpeedX0)<minspeedx) SpeedX0 = 0;
    PosX1 += SpeedX1; SpeedX1 *= 0.90f; if(sAbs(SpeedX1)<minspeedx) SpeedX1 = 0;
    const sF32 minspeedy = 0.125f;
    PosY  += SpeedY ; SpeedY  *= 0.90f; if(sAbs(SpeedY )<minspeedy) SpeedY  = 0;
  }

  if(PosX0+2>PosX1)
  {
    sF32 f = (PosX0+PosX1)*0.5f;
    PosX0 = f-1.0f;
    PosX1 = f+1.0f;
    SpeedX0 = SpeedX1 = 0;
  }
//  PosX0 = sClamp<sF32>(PosX0,0,1000);
//  PosX1 = sClamp<sF32>(PosX1,0,1000);
  PosY = sClamp<sF32>(PosY,0,2000);

  // painting

  Painter->Begin(tp);
  ScreenX = (sF32)tp.SizeX;
  ScreenY = (sF32)tp.SizeY;
  Painter->SetFont(sPainterFontPara(PFont));

  PaintGrid();

  Painter->End(Context);

  //

  DPaint->SetPrintMode(0xff000000,0xff404040,1.0f);
  DPaint->FramePrint("\n\n");
  DPaint->PrintFPS();
  DPaint->PrintStats();
  DPaint->Draw(tp);

  Context->EndTarget();
}

void App::OnKey(const sKeyData &kd)
{
  switch(kd.Key&(sKEYQ_Mask|sKEYQ_Break))
  {
  case sKEY_Escape:
    sExit();
    break;
  case ' ':
    Round = !Round;
    break;
  }
}

void App::OnDrag(const sDragData &dd)
{
  if(dd.Mode==sDM_Start)
  {
    if(dd.Buttons==2)
      DragMode = DM_ZoomRMB;
    else
      DragMode = DM_ScrollX;
    DragPosX0 = PosX0;
    DragPosX1 = PosX1;
    DragPosY = PosY;
    DragMax = 0;
  }
  else if(dd.Mode==sDM_Drag)
  {
    sF32 f;

    switch(DragMode)
    {
    case DM_ScrollX:
      DragMax = sMax(DragMax,sAbs(sF32(dd.DeltaX)));
      f = -(dd.DeltaX) * (DragPosX1-DragPosX0) / ScreenX;
      PosX0 = DragPosX0 + f;
      PosX1 = DragPosX1 + f;
      if(sAbs(dd.DeltaY) > sAbs(dd.DeltaX)*2 && DragMax<10)
        DragMode = DM_ScrollXY;
      if(dd.TouchCount==2)
        DragMode = DM_Zoom;
      break;
    case DM_ScrollXY:
      f = -(dd.DeltaX) * (DragPosX1-DragPosX0) / ScreenX;
      PosX0 = DragPosX0 + f;
      PosX1 = DragPosX1 + f;
      PosY = DragPosY - dd.DeltaY;
      if(dd.TouchCount==2)
        DragMode = DM_Zoom;
      break;
    case DM_ZoomRMB:
      f = -(dd.DeltaX) * (DragPosX1-DragPosX0) / ScreenX;
      PosX0 = DragPosX0 + f;
      PosX1 = DragPosX1 - f;
      if(dd.TouchCount==2)
        DragMode = DM_Zoom;
      break;
    case DM_Zoom:
      if(dd.TouchCount==2 && dd.Touches[0].Mode!=sDM_Hover && dd.Touches[1].Mode!=sDM_Hover)
      {
        sF32 c =  (dd.Touches[0].StartX+dd.Touches[1].StartX) * 0.5f;
        sF32 f = ((dd.Touches[0].PosX+dd.Touches[1].PosX) - (dd.Touches[0].StartX+dd.Touches[1].StartX))*0.5f;
        sF32 s = sF32(dd.Touches[0].StartX-dd.Touches[1].StartX) / sF32(dd.Touches[0].PosX-dd.Touches[1].PosX);

        f = -f * (DragPosX1-DragPosX0) / ScreenX;
        c =  DragPosX0 + (c * (DragPosX1-DragPosX0) / ScreenX);
//        DPaint->ConsolePrintF("scale %f center %f scroll %f pos %f %f\n",s,c,f,PosX0,PosX1);

        PosX0 = (DragPosX0-c+f)*s+c;
        PosX1 = (DragPosX1-c+f)*s+c;
      }
      else
      {
        DragMode = DM_Off;
      }
      break;
    default:
      break;
    }
  }
  else if(dd.Mode==sDM_Stop)
  {
    DragMode = DM_Off;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void App::PaintGrid()
{
  sString<64> buffer;
  sF32 step = 1;
  if(PosX1-PosX0>21)
    step = 10;
  if(PosX1-PosX0>201)
    step = 100;

  for(sF32 f=0;f<1000;f+=step)
  {
    if(f>PosX0-step && f<PosX1)
    {
      sF32 x = CalcX(f);
      if(Round)
        x = sRoundDown(x);
      Painter->Line(0xffc0c0c0,sFRect(x,0,x,ScreenY));
      buffer.PrintF("%d",sInt(f+0.5f));
      Painter->Text(sPPF_Left|sPPF_Space|sPPF_Top,0xff000000,x,0.0f,buffer);
    }
  }
  for(sF32 f=0;f<10;f++)
  {
    sF32 y = CalcY(f*200);
    if(Round)
      y = sRoundDown(y);
    Painter->Line(0xffc0c0c0,sFRect(0,y,ScreenX,y));
  }
}


/****************************************************************************/


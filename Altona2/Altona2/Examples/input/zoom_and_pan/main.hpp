/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_EXAMPLES_GRAPHICS_CUBE_MAIN_HPP
#define FILE_ALTONA2_EXAMPLES_GRAPHICS_CUBE_MAIN_HPP

#include "altona2/libs/base/base.hpp"
#include "altona2/libs/util/debugpainter.hpp"
#include "altona2/libs/util/painter.hpp"

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

enum AppDragMode
{
  DM_Off = 0,
  DM_ScrollX,
  DM_ScrollXY,
  DM_ZoomRMB,
  DM_Zoom,
};

class App : public sApp
{
  sAdapter *Adapter;
  sContext *Context;
  sScreen *Screen;

  sDebugPainter *DPaint;
  sPainter *Painter;
  sPainterImage *PImg[2];
  sPainterFont *PFont;

  // current state

  sF32 PosX0;
  sF32 PosX1;
  sF32 PosY;
  sF32 ScreenX;
  sF32 ScreenY;

  // dragging

  AppDragMode DragMode;
  sF32 DragPosX0;
  sF32 DragPosX1;
  sF32 DragPosY;
  sF32 DragMax;

  // speed calculation

  sF32 OldX0[2];
  sF32 OldX1[2];
  sF32 OldY[2];
  sF32 SpeedX0;
  sF32 SpeedX1;
  sF32 SpeedY;

  // ...

  sBool Round;

  void PaintGrid();
  sF32 CalcX(sF32 p) { return (p-PosX0)*ScreenX/(PosX1-PosX0); }
  sF32 CalcY(sF32 p) { return (p-PosY); }
public:
  App();
  ~App();

  void OnInit();
  void OnExit();
  void OnFrame();
  void OnPaint();
  void OnKey(const sKeyData &kd);
  void OnDrag(const sDragData &dd);
};

/****************************************************************************/

#endif  // FILE_ALTONA2_EXAMPLES_GRAPHICS_CUBE_MAIN_HPP


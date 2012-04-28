/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_CUBE_MAIN_HPP
#define FILE_CUBE_MAIN_HPP

#include "base/system.hpp"
#include "base/graphics.hpp"
#include "util/painter.hpp"
#include "util/shaders.hpp"

/****************************************************************************/

enum Lines
{
  LINE_DISPLAY,
  LINE_WINDOW,
  LINE_SCREEN,
  LINE_REFRESH,
  LINE_FSAA,
  LINE_ASPECT,
  LINE_RTSIZE,
  LINE_OVERSIZE,
  LINE_OVERFSAA,
  LINE_MAX,
};

struct Resolution
{
  sString<64> Name;
  sInt XSize,YSize;

  void Init(sInt x,sInt y,const sChar *name) { XSize=x; YSize=y; Name.PrintF(name,x,y); }
};

class MyApp : public sApp
{
  sPainter *Painter;
  sTexture2D *Tex;
  sMaterial *Mtrl;
  sMaterial *White;
  sGeometry *Geo;
  sGeometry *GeoQuad;

  sMaterialEnv Env;

  sArray<Resolution> ScreenSizes;
  sArray<Resolution> ScreenAspects;
  sArray<Resolution> RefreshRates;
  sArray<Resolution> MultisampleModes;
  sArray<Resolution> OverSizes;
  sArray<Resolution> RTSizes;
  sScreenMode LastMode;

  sInt Cursor;
  sInt SetMax[LINE_MAX];
  sInt SetStrobe[LINE_MAX];
  sString<128> SetString[LINE_MAX];
  sChar *SetLabel[LINE_MAX];

  sTiming Timer;

  void PaintRT(sInt time);
  void PaintCube(sInt time);
  void PaintSamplePattern(sInt time);
public:
  MyApp();
  ~MyApp();
  void OnPaint3D();
  void OnPrepareFrame();
  void OnInput(const sInput2Event &ie);

  void InitGui();
  void ExitGui();
  void PrintGui();
  void UpdateGui();
};

/****************************************************************************/

#endif // FILE_CUBE_MAIN_HPP

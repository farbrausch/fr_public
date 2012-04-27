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
#include "util/image.hpp"

/****************************************************************************/

struct Entry
{
  sPoolString Name;
  sImage Image;
  sTexture2D *Texture;
};

class MyApp : public sApp
{
  sPainter *Painter;
  sTexture2D *Tex;
  sTexture2D *TexOld;
  sFRect GeoPos;
  sFRect GeoPosOld;
  sMaterial *Mtrl;
  sMaterial *CursorMtrl;
  sGeometry *Geo;

  sViewport View;
  sMaterialEnv Env;

  sTiming Timer;

  sInt ImageId;
  sArray<Entry *> Entries;

  sInt MouseX;
  sInt MouseY;
  sInt MouseB;
  sInt MouseHide;
  sFRect Pos;

  sF32 Zoom;
  sF32 ZoomOld;
  sF32 ZoomTarget;
  sF32 ScrollSpeedX;
  sF32 ScrollSpeedY;

  sF32 ScreenX;
  sF32 ScreenY;
  sF32 ImageX;
  sF32 ImageY;

  sBool Help;
  sBool Linear;
  sBool FixZoom;
  sBool LoadMode;

  sInt FadeTimer;
  sInt FadeMax;

  sArray<sDirEntry> dir;
  sPoolString Name;
public:
  MyApp();
  ~MyApp();


  void LoadEntries();
  void LoadOne();
  void GoImage(sInt id);
  void CmdReset();

  void OnPaint3D();
  void OnInput(const sInput2Event &ie);
};

/****************************************************************************/

#endif // FILE_CUBE_MAIN_HPP

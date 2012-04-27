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
#include "extra/freecam.hpp"

/****************************************************************************/


class MyApp : public sApp
{
  sPainter *Painter;
  sTexture2D *Tex;
  sMaterial *Mtrl;
  sTextBuffer tb;
  class sStsWorkload *Workload;

  sViewport View;
  sFreeflightCamera Cam;
  sInt ToggleNew;
  sInt FogFactor;

  sTiming Timer;

  void AddGeoBuffer();
  void EndGeoBuffer();
  void AddSpace(sInt x,sInt y,sInt z,sInt div,sInt n);

  void MakeSpace();

  class OctNode *Root;
public:
  MyApp();
  ~MyApp();
  void OnPaint3D();
  void OnInput(const sInput2Event &ie);
};

/****************************************************************************/

#endif // FILE_CUBE_MAIN_HPP

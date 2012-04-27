/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_SIMPLETASK_MAIN_HPP
#define FILE_SIMPLETASK_MAIN_HPP

#include "base/types.hpp"
#include "base/system.hpp"
#include "base/graphics.hpp"
#include "util/painter.hpp"
#include "util/shaders.hpp"
#include "extra/mcubes.hpp"
#include "extra/freecam.hpp"

/****************************************************************************/

class MyApp : public sApp
{
  sPainter *Painter;
  sTexture2D *Tex;
  sMaterial *Mtrl;
  sFreeflightCamera Cam;

  sViewport View;

  sTiming Timer;
  MarchingCubes *MC;
  sArray<sVector31> MCParts;
  sMaterialEnv Env;

  class WaterFX *Water;
  class WaterFX *Drop;
public:
  MyApp();
  ~MyApp();
  void OnPaint3D();
  void OnInput(const sInput2Event &ie);
};


/****************************************************************************/

#endif // FILE_SIMPLETASK_MAIN_HPP


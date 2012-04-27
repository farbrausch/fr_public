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

#pragma once

#include "base/system.hpp"
#include "util/painter.hpp"
#include "base/graphics.hpp"

/****************************************************************************/

class MyApp : public sApp
{
  sPainter *Painter;
  sMaterial *Mtrl;
  sGeometry *Geo;

  sViewport View;
  sMaterialEnv Env;

  sTiming Timer;

  void PaintCube();
  void PrintInput();
  void PrintInput2Device(class sInput2Device *device, sInt &y);
public:
  MyApp();
  ~MyApp();
  void OnPaint3D();
  void OnInput(const sInput2Event &ie);
};

/****************************************************************************/

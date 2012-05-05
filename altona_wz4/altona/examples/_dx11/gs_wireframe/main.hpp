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
#include "shader.hpp"

/****************************************************************************/

class MyApp : public sApp
{
  sPainter *Painter;
  class TestMtrl *Mtrl;
  sTexture2D *Tex;
  sGeometry *Geo;
  
  sVertexFormatHandle *MyFormat;

  sViewport View;
  sMaterialEnv Env;

  sTiming Timer;
public:
  MyApp();
  ~MyApp();
  void OnPaint3D();
  void OnInput(const sInput2Event &ie);
};

/****************************************************************************/

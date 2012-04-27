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
#include "instshader.hpp"

/****************************************************************************/

class MyApp : public sApp
{
  sPainter *Painter;
  sTexture2D *Tex;

  sMaterial *WaterMtrl;
  sGeometry *WaterGeo;
  sVertexFormatHandle *WaterFormat;

  TorusShader *TorusMtrl;
  sGeometry *TorusGeo;
  sVertexFormatHandle *TorusFormat;

  sGeometry *InstGeo;
  sGeometry *MergeGeo;

  sGeometry *QuadGeo;

  sViewport View;
  sMaterialEnv Env;

  sTiming Timer;

  void CreateWater();
  void CreateTorus();
public:
  MyApp();
  ~MyApp();
  void OnPaint3D();
  void OnInput(const sInput2Event &ie);
};

/****************************************************************************/

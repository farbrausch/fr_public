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
  sViewport View;
  sMaterialEnv Env;
  sTiming Timer;
  sInt time;

  sGeometry *TorusGeo;
  sMaterial *TorusMtrl;
  sTextureCube *TorusCubemap;
  sTexture2D *TorusDepth;

  sGeometry *CubeGeo;
  sMaterial *CubeMtrl;
  sTexture2D *CubeTex;
  sArray<sMatrix34> CubeMats;

  void RenderCubemap();
  void RenderScene();
  void PaintCubes();
public:
  MyApp();
  ~MyApp();
  void OnPaint3D();
  void OnInput(const sInput2Event &ie);
};

/****************************************************************************/

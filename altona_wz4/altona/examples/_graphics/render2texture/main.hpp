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

class MyApp : public sApp
{
  sPainter *Painter;
  sTexture2D *Tex;
  sTexture2D *TexRT;
  sTexture2D *TexRTZ;
  sMaterial *Mtrl;
  sMaterial *MtrlRT;
  sGeometry *Geo;

  sViewport View;
  sMaterialEnv Env;

  sTexture2D *DummyTex;

  sTiming Timer;
public:
  MyApp();
  ~MyApp();
  void OnPaint3D();
  void OnInput(const sInput2Event &ie);

  void RenderCube(sMaterial *mtrl);
  void Blit(sTexture2D *d,sTexture2D *s,sMaterial *mtrl);
};

/****************************************************************************/

#endif // FILE_CUBE_MAIN_HPP

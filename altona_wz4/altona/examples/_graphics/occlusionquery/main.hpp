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

#define SOURCECOUNT 100


struct Sprite
{
  sVector31 Pos;
  sOccQuery *Occ;
};

class MyApp : public sApp
{
  sPainter *Painter;
  sTexture2D *CubeTex;
  sMaterial *CubeMtrl;
  sGeometry *CubeGeo;

  sTexture2D *SrcTex;
  sMaterial *SrcMtrl;
  sGeometry *SrcGeo;
  sTexture2D *GlareTex;
  sMaterial *GlareMtrl;
  sGeometry *GlareGeo;

  sMaterial *CalibMtrl;
  sGeometry *CalibGeo;
  sOccQuery *CalibQuery;

  sViewport View;
  sViewport ViewOrtho;
  sTiming Timer;

  Sprite Sprites[SOURCECOUNT];

public:
  MyApp();
  ~MyApp();
  void OnPaint3D();
  void OnInput(const sInput2Event &ie);
};

/****************************************************************************/

#endif // FILE_CUBE_MAIN_HPP

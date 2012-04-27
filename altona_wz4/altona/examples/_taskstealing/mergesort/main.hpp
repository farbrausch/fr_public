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

/****************************************************************************/

class MyApp : public sApp
{
  sPainter *Painter;
  sTexture2D *Tex;
  sMaterial *Mtrl;
  sGeometry *Geo;

  sViewport View;
  sMaterialEnv Env;

  sTiming Timer;
  sTextBuffer tb;
  static const sInt SortCount = 1024*512;//1024*256;

  sArray<sU32> SortSource;
  sArray<sU32> SortDestHS;
  sArray<sU32> SortDestST;
  sArray<sU32> SortDestMT;

  void Update(sTexture2D *);
public:
  MyApp();
  ~MyApp();
  void OnPaint3D();
  void OnInput(const sInput2Event &ie);
};


/****************************************************************************/

#endif // FILE_SIMPLETASK_MAIN_HPP


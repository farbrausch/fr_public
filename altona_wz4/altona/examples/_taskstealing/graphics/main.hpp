/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_TITAN_MAIN_HPP
#define FILE_TITAN_MAIN_HPP

#include "base/system.hpp"
#include "util/painter.hpp"
#include "base/graphics.hpp"
#include "shaders.hpp"
#include "extra/freecam.hpp"

/****************************************************************************/

struct Element
{
  sVector31 Pos;
  sVector30 Rot;
  sVector30 RotSpeed;
  sMatrix34 Matrix;
  sTextureCube *CubeTex;
};

struct ThreadDataType
{
  sGfxThreadContext *Ctx;
};

class MyApp : public sApp
{
  sPainter *Painter;
  class MaterialFlat *Mtrl;
  sVertexFormatHandle *Format;
  sTexture2D *Tex;
  sGeometry *Geo;

  sViewport View;
  sMaterialEnv Env;

  sTiming Timer;
  sCBuffer<MaterialFlatPara> MtrlPara;
  sInt ThreadCount;
  ThreadDataType *ThreadDatas;

  sInt Mode;
  sInt Granularity;
  sInt EndGame;

public:
  MyApp();
  ~MyApp();
  void OnPaint3D();
  void OnInput(const sInput2Event &ie);

  void ThreadCode(class sStsManager *man,class sStsThread *th,sInt start,sInt count);

  sArray<Element> Elements;
  sFreeflightCamera Cam;
};

/****************************************************************************/

#endif // FILE_TITAN_MAIN_HPP


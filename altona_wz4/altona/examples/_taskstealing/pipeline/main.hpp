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
#include "util/taskscheduler.hpp"

/****************************************************************************/

struct Element
{
  sVector31 Pos;
  sVector30 Rot;
  sVector30 RotSpeed;
  sMatrix34 Matrix;
  sTextureCube *CubeTex;
};

class GeoBuffer
{
public:
  GeoBuffer();
  ~GeoBuffer();
  void Begin();
  void End();
  void Draw(sGeometry *geo);

  sGeometry *Geo;
  sMatrix34CM *vp;
  sInt Alloc;
  sInt Used;

  sDNode Node;
};

struct ThreadDataType
{
  GeoBuffer *GB;
};

class MyApp : public sApp
{
  sPainter *Painter;
  class MaterialFlat *Mtrl;
  sTexture2D *Tex;
  sGeometry *Geo;

  sViewport View;
  sMaterialEnv Env;

  sTiming Timer;
  sCBuffer<MaterialFlatPara> MtrlPara;
  sInt ThreadCount;
  ThreadDataType *ThreadDatas;
  sStsSync *FrameSync;

  sInt EndGame;
  sInt Granularity;
  sInt Mode;

  sDList2<GeoBuffer> BufferFree;
  sDList2<GeoBuffer> BufferFull;
  GeoBuffer *GetBuffer();
  sStsWorkload *WLA;
  sStsWorkload *WLB;

public:
  MyApp();
  ~MyApp();
  void OnPaint3D();
  void OnInput(const sInput2Event &ie);

  void ThreadCode0(class sStsManager *man,class sStsThread *th,sInt start,sInt count);
  void ThreadCode1(class sStsManager *man,class sStsThread *th,sInt start,sInt count);

  sArray<Element> Elements;
  sFreeflightCamera Cam;

  sVertexFormatHandle *Format;
  sThreadLock BufferLock;
};

extern MyApp *App;

/****************************************************************************/

#endif // FILE_TITAN_MAIN_HPP


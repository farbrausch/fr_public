/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_EXAMPLES_GRAPHICS_CUBE_MAIN_HPP
#define FILE_ALTONA2_EXAMPLES_GRAPHICS_CUBE_MAIN_HPP

#include "altona2/libs/base/base.hpp"
#include "altona2/libs/util/debugpainter.hpp"
#include "shader.hpp"

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class App : public sApp
{
  sMaterial *CubeMtrl;
  sMaterial *FlatMtrl;
  sResource *Tex;
  sGeometry *CubeGeo;
  sGeometry *DynGeo;
  sGeometry *StatGeo;
  sCBuffer<CubeShader_cbvs> *cbv0;
  sDebugPainter *DPaint;

  sInt Benchmark;
  sInt BenchValue;

  sInt StatGeoBenchmark;
  sInt StatGeoBenchValue;
  
  enum Enums
  {
    MaxBench = 12,
    MaxValue = 10,
  };
  sRect BenchRect[MaxBench];
  sRect ValueRect[MaxValue];

  sAdapter *Adapter;
  sContext *Context;
  sScreen *Screen;
public:
  App();
  ~App();

  void OnInit();
  void OnExit();
  void OnFrame();
  void OnPaint();
  void OnKey(const sKeyData &kd);
  void OnDrag(const sDragData &dd);

  void Bench1(const sTargetPara &tp);
  void Bench2_3(const sTargetPara &tp,sInt stat);
  void Bench4(const sTargetPara &tp);
};

/****************************************************************************/

#endif  // FILE_ALTONA2_EXAMPLES_GRAPHICS_CUBE_MAIN_HPP


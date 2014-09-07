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

enum AppConst
{
  PartCount = 128,
};

struct vert0
{
  sF32 u0,v0;
  void Set(sF32 u,sF32 v) { u0=u; v0=v; }
};

struct vert1
{
  sF32 px,py,pz;
  void Set(sF32 x,sF32 y,sF32 z) { px=x; py=y; pz=z; }
};

class App : public sApp
{
  sAdapter *Adapter;
  sContext *Context;
  sScreen *Screen;

  sMaterial *Mtrl;
  sVertexFormat *Format;
  sResource *Tex;
  sGeometry *Geo;
  sCBuffer<CubeShader_cbvs> *cbv0;
  sDebugPainter *DPaint;
public:
  App();
  ~App();

  void OnInit();
  void OnExit();
  void OnFrame();
  void OnPaint();
  void OnKey(const sKeyData &kd);
  void OnDrag(const sDragData &dd);
};

/****************************************************************************/

#endif  // FILE_ALTONA2_EXAMPLES_GRAPHICS_CUBE_MAIN_HPP


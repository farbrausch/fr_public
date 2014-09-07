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
  sAdapter *Adapter;
  sContext *Context;
  sScreen *Screen;

  sMaterial *Mtrl;
  sMaterial *BlurX;
  sMaterial *BlurY;
  sResource *Tex;
  sGeometry *Geo;
  sCBuffer<sFixedMaterialLightVC> *cbv0;
  sCBuffer<BlurXShader_cbc0> *cbc0;
  sDebugPainter *DPaint;
  sSamplerState *CsSampler;

  sInt RtSizeX;
  sInt RtSizeY;
  sResource *RtDepth;
  sResource *RtColor;

  sResource *RtWork[3];

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


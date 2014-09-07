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

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class AdapterInfo
{
public:
  AdapterInfo(sAdapter *ada);
  ~AdapterInfo();
  void Draw(sTargetPara &tp,float time);

  sAdapter *Adapter;
  sContext *Context;

  sMaterial *Mtrl;
  sResource *Tex;
  sGeometry *Geo;
  sCBuffer<sFixedMaterialLightVC> *cbv0;
  sDebugPainter *DPaint;
};

class App : public sApp
{
  sArray<AdapterInfo *> AdapterInfos;
  sArray<sScreen *> Screens;

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
};

/****************************************************************************/

#endif  // FILE_ALTONA2_EXAMPLES_GRAPHICS_CUBE_MAIN_HPP


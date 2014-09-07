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

struct Click
{
  sInt Make;
  sInt ScreenX;
  sInt ScreenY;
  sInt Time;
  sU32 Color;
};

struct Vertex
{
  sF32 px,py,pz;
  sU32 c0;

  void Init(sF32 x,sF32 y,sF32 z,sU32 c) { px=x; py=y; pz=z; c0=c; }
};

class App : public sApp
{
  sAdapter *Adapter;
  sContext *Context;
  sScreen *Screen;

  sMaterial *Mtrl;
  sVertexFormat *Format;
  sGeometry *Geo;
  sCBuffer<sFixedMaterialVC> *cbv0;
  sDebugPainter *DPaint;


  sArray<Click> Clicks;
  sInt DragActive;
  sInt StartX,StartY,EndX,EndY;

  sInt MaxVertex;
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


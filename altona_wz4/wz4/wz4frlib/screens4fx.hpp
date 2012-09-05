/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_SCREENS4FX_HPP
#define FILE_WZ4FRLIB_SCREENS4FX_HPP

#include "base/types.hpp"
#include "wz4frlib/wz4_demo2.hpp"
#include "wz4frlib/wz4_demo2_ops.hpp"
#include "wz4frlib/wz4_mesh.hpp"
#include "wz4frlib/wz4_mtrl2.hpp"

#include "wz4frlib/screens4_ops.hpp"

/****************************************************************************/
/****************************************************************************/

class RNSiegmeister : public Wz4RenderNode
{
  sGeometry *Geo;
  sSimpleMaterial *Mtrl;

public:
  RNSiegmeister();
  ~RNSiegmeister();

  Texture2D *Texture;

  sF32  Fade;
  sBool DoBlink;
  sU32  Color;
  sU32  BlinkColor1;
  sU32  BlinkColor2;
  sF32  Alpha;
  sF32  Spread;
  sStaticArray<sFRect> Bars;

  Wz4RenderParaSiegmeister ParaBase,Para;
  Wz4RenderAnimSiegmeister Anim;

  void Init();
  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);
};

/****************************************************************************/

#endif // FILE_WZ4FRLIB_SCREENS4FX_HPP


/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_SOLOPONG_MAIN_HPP
#define FILE_SOLOPONG_MAIN_HPP

#include "base/types.hpp"

/****************************************************************************/

struct Joypad
{
  sU32 Buttons;
  sF32 Analog[4];
};

void DrawRect(const sFRect &r,sU32 col);
void DrawRect(sF32 x0,sF32 y0,sF32 x1,sF32 y1,sU32 col);
void GetJoypad(struct Joypad &);

/****************************************************************************/

#include "base/system.hpp"
#include "base/graphics.hpp"
#include "util/painter.hpp"
#include "util/shaders.hpp"

/****************************************************************************/

class MyApp : public sApp
{
  sPainter *Painter;
  sMaterial *Mtrl;
  sGeometry *Geo;

  sViewport View;

  sTiming Timer;

  class SoloPong *Game;
public:
  MyApp();
  ~MyApp();
  void OnPaint3D();
  void OnInput(const sInput2Event &ie);
};

/****************************************************************************/

#endif // FILE_SOLOPONG_MAIN_HPP


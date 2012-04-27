/****************************************************************************/

#ifndef FILE_DXTDECOMP_MAIN_HPP
#define FILE_DXTDECOMP_MAIN_HPP

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
public:
  MyApp();
  ~MyApp();
  void OnPaint3D();
  void OnInput(const sInput2Event &ie);
};

/****************************************************************************/

#endif // FILE_DXTDECOMP_MAIN_HPP


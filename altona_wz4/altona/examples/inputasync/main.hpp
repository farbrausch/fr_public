/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#pragma once

#include "base/system.hpp"
#include "util/painter.hpp"
#include "base/graphics.hpp"

/****************************************************************************/

#define MAX_STEPS 512
#define MAX_CHANNELS 8

class MyApp : public sApp
{
  sPainter *Painter;
  sMaterial *Mtrl;
  sGeometry *Geo;
  sInt Load;

  sViewport View;
  sMaterialEnv Env;

  sTiming Timer;

  void GetInput();
  void PaintCube();
  void PrintInput();
  void PrintGraph();

  sInt Buffer[MAX_CHANNELS][MAX_STEPS];
  sInt State[MAX_CHANNELS];
  sInt Index;
  sInt TimeStamp;
  sInt ChannelMask;
public:
  MyApp();
  ~MyApp();
  void OnPaint3D();
  void OnInput(const sInput2Event &ie);
};

/****************************************************************************/

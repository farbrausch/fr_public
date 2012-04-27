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

#ifndef FILE_CUBE_MAIN_HPP
#define FILE_CUBE_MAIN_HPP

#include "base/system.hpp"
#include "base/graphics.hpp"
#include "util/painter.hpp"
#include "util/shaders.hpp"
#include "extra/freecam.hpp"

/****************************************************************************/

class MyApp : public sApp
{
  sPainter *Painter;
  sTexture2D *Tex;
  sCSBuffer *MCMap;
  sMaterial *Mtrl;
  sGeometry *Geo;
  sVertexFormatHandle *Format;

  sViewport View;
  sMaterialEnv Env;

  sTiming Timer;
  sComputeShader *CS;
  sComputeShader *CSWC;
  sCSBuffer *CSVB;
  sCSBuffer *CSIB;
  sCSBuffer *CSCountVertex;
  sCSBuffer *CSCountIndex;
  sCSBuffer *CSIndirect;
  sCSBuffer *NodeBuffer;
  sFreeflightCamera Cam;

  // geometry

  sF32 MyApp::func(const sVector31 &c);

  sF32 FineScale;
  sF32 Scale;
  sVector30 Pos0;

  // build nodes
  struct nodeinfo
  {
    sU32 x,y,z,w;
  } *ni;
  void BuildNode(sInt x,sInt y,sInt z,sInt d);
  nodeinfo *Nodes;
  sInt NodeIndex;
public:
  MyApp();
  ~MyApp();
  void OnPaint3D();
  void OnInput(const sInput2Event &ie);
};

/****************************************************************************/

#endif // FILE_CUBE_MAIN_HPP

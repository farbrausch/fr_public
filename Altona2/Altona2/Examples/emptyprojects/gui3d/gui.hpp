/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_TOOLS_FONTTOOL_GUI_HPP
#define FILE_ALTONA2_TOOLS_FONTTOOL_GUI_HPP

#include "altona2/libs/base/base.hpp"
#include "altona2/libs/gui/gui.hpp"
#include "shaders.hpp"

using namespace Altona2;

class ShaderWindow;
class RenderWindow;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class MainWindow : public sPassthroughFrame
{
  friend class ShaderWindow;
  sImageWindow *GlyphWin;
  sImageWindow *SDFWin;
  sGridFrame *ParaWin;
  RenderWindow *RenderWin;

  sPainterImage *ShaderImage;
  sPainterImage *ShaderImage2;

  sImage GlyphImg;

  sString<16> Glyph;

  void SetPara();
  void InitFont();
  void RenderGlyph();
  void SetShader();
public:
  MainWindow();
  void OnInit();
  ~MainWindow();

  void CmdSetPara();
  void CmdUpdate();
  void CmdEdit();
};

class RenderWindow : public s3dWindow
{
  sMaterial *Mtrl;
  sResource *Tex;
  sGeometry *Geo;
  sCBuffer<CubeShader_cbvs> *cbv0;

  sU32 LastTime;

public:
  RenderWindow();
  void OnInit();
  ~RenderWindow();
  void Render(const sTargetPara &tp);

  sVector3 Rot;
  sVector3 Speed;
};

/****************************************************************************/

#endif  // FILE_ALTONA2_TOOLS_FONTTOOL_GUI_HPP


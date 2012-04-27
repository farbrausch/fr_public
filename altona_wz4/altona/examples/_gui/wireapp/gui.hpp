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
#include "base/types2.hpp"
#include "gui/gui.hpp"
#include "gui/3dwindow.hpp"
#include "base/graphics.hpp"

class WinCube;
/****************************************************************************/

class MainWindow : public sWireClientWindow
{
public:
  sCLASSNAME(MainWindow);
  MainWindow();
  ~MainWindow();
  void Tag();

  sWindow *BlaWin;
  WinCube *Cube1Win;
  WinCube *Cube2Win;
  void ResetWindows();

  void CmdNew();
  void CmdOpen();
  void CmdOpen2();
  void CmdSaveAs();
  void CmdSave();
  void CmdExit();
};

extern MainWindow *App;

/****************************************************************************/

class WinCube : public s3DWindow
{
  sMaterial *Mtrl;
  sGeometry *Geo;
  sPainter *Painter;
  sTiming Timer;
  sMaterialEnv Env;
public:
  sCLASSNAME(WinCube);
  WinCube();
  ~WinCube();
  void Paint(sViewport &view,const sTargetSpec &spec);
};

/****************************************************************************/

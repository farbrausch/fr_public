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

#include "base/system.hpp"
#include "gui/gui.hpp"

/****************************************************************************/

class MainWindow : public sWindow
{
  sString<64> Screen[16];
  sInt ScreenIndex;
  void Print(const sChar *);
  void CmdMidi();
public:
  MainWindow();
  ~MainWindow();
  void OnPaint2D();
  sBool OnKey(sU32 key);
};

/****************************************************************************/

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

class MyWindow : public sWindow
{
public:
  MyWindow();
  void OnPaint2D();
  sBool BackBuffer;
};

class MyParentWin : public sHSplitFrame
{
  sTextBuffer Text;
public:
  MyParentWin();
};


/****************************************************************************/

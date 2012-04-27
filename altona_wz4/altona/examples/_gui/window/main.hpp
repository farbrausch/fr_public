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
  void OnPaint2D();
};

class MyParentWin : public sHSplitFrame
{
  sTextBuffer Text;
public:
  MyParentWin();
  void MakeMenu();
};


/****************************************************************************/

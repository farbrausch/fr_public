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
 
#define sPEDANTIC_OBSOLETE 1
//#define sPEDANTIC_WARN 1

#include "main.hpp"
#include "base/system.hpp"
#include "gui/gui.hpp"
#include "gui.hpp"

/****************************************************************************/

void sMain()
{
  sInit(sISF_2D|sISF_3D,800,600);
  sInitGui();
  sGui->AddBackWindow(new MainWindow);
}

/****************************************************************************/


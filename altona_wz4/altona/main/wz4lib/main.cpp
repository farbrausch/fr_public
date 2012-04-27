/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) by Dierk Ohlerich                                    ***/
/***   all rights reserverd                                               ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "main.hpp"
#include "gui.hpp"
#include "gui/gui.hpp"
#include "base/windows.hpp"

/****************************************************************************/


void sMain()
{
  sInit(sISF_2D|sISF_3D,1024,768);
  sInitGui();
  sGui->AddBackWindow(new MainWindow);
  sSetWindowName(L"werkkzeug4");
}

/****************************************************************************/


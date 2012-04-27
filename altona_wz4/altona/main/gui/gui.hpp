/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_GUI_GUI_HPP
#define FILE_GUI_GUI_HPP

#ifndef __GNUC__
#pragma once
#endif

// base

#include "base/types.hpp"
#include "base/windows.hpp"
#include "base/types2.hpp"

// window system

#include "gui/window.hpp"
#include "gui/manager.hpp"
#include "gui/wire.hpp"

// general windows

#include "gui/frames.hpp"
#include "gui/controls.hpp"
#include "gui/borders.hpp"
#include "gui/dialog.hpp"

// special windows: not included by default

#if 0
#include "gui/textwindow.hpp"
#include "gui/listwindow.hpp"
#include "gui/3dwindow.hpp"
#include "gui/overlapped.hpp"
#endif

/****************************************************************************/

/* example for a good starting point:

#include "base/system.hpp"
#include "gui/gui.hpp"


class MyWindow : public sWindow
{
public:
  void OnPaint2D()
  {
    sRect2D(Client,sGC_BACK);
    sLine2D(Client.x0,Client.y0,Client.x1-1,Client.y1-1,sGC_DRAW);
    sLine2D(Client.x0,Client.y1-1,Client.x1-1,Client.y0,sGC_DRAW);
  }
};


void sMain()
{
  sInit(sISF_2D,800,600);
  sInitGui();

  sGui->AddBackWindow(new MyWindow);
}

*/

#endif

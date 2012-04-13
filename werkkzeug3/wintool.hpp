// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __WINTOOL_HPP__
#define __WINTOOL_HPP__

#include "_gui.hpp"

#if sLINK_GUI

/****************************************************************************/

class WinPerfMon : public sGuiWindow
{
  sInt Mode;
  sInt LastPaintTime;
public:
  WinPerfMon();
  void OnPaint();
  void OnKey(sU32 key);
};

/****************************************************************************/

#endif
#endif

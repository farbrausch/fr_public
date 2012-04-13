// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __WINCALC_HPP__
#define __WINCALC_HPP__

#include "_gui.hpp"

#if sLINK_GUI

/****************************************************************************/

class WinCalc : public sGuiWindow
{
  sInt Mode;
  sInt LastPaintTime;
  class WerkkzeugApp *App;
public:
  WinCalc(class WerkkzeugApp *app);
  void OnPaint();
  void OnKey(sU32 key);
};

/****************************************************************************/

#endif
#endif

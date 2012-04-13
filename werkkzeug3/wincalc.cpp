// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_start.hpp"
#include "_gui.hpp"
#include "_util.hpp"
#include "wincalc.hpp"
#include "werkkzeug.hpp"

#if sLINK_GUI

/****************************************************************************/

WinCalc::WinCalc(WerkkzeugApp *app)
{
  App = app;
  AddScrolling(0,1);
}

void WinCalc::OnPaint()
{
  sPainter->Paint(sGui->FlatMat,Client,0xffff0000);//sGui->Palette[sGC_BACK]);
}

void WinCalc::OnKey(sU32 key)
{
  switch(key & 0x8001ffff)
  {
  case 'i':
    break;
  }
}

/****************************************************************************/

#endif // sLINK_GUI

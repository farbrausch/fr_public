// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __APP_PREFS_HPP__
#define __APP_PREFS_HPP__

#include "_util.hpp"
#include "_gui.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   windows                                                            ***/
/***                                                                      ***/
/****************************************************************************/

class sPrefsApp : public sDummyFrame
{
  sInt Colors[sGC_MAX*3];
public:
  sPrefsApp();
  void OnKey(sU32 key);
  sBool OnCommand(sU32 cmd);
};

/****************************************************************************/

#endif

// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __APP_TEXT_HPP__
#define __APP_TEXT_HPP__

#include "_util.hpp"
#include "_gui.hpp"
#include "appbrowser.hpp"

/****************************************************************************/

class sTextControl2 : public sTextControl
{
public:
  sBool LoadFile(sChar *name);
  sChar PathName[sDI_PATHSIZE];
};

/****************************************************************************/
/***                                                                      ***/
/***   windows                                                            ***/
/***                                                                      ***/
/****************************************************************************/

#define sTCC_COPY     0x0101
#define sTCC_PASTE    0x0102
#define sTCC_CUT      0x0103
#define sTCC_BLOCK    0x0104
#define sTCC_CLEAR    0x0105
#define sTCC_OPEN     0x0106
#define sTCC_SAVE     0x0107
#define sTCC_SAVEAS   0x0108

#define sTCC_EXIT     0x0109

class sTextApp : public sDummyFrame
{
  sTextControl2 *Edit;
  sStatusBorder *Status;
  sInt Changed;

  sChar StatusName[256];
  sChar StatusLine[32];
  sChar StatusColumn[32];
  sChar StatusChanged[32];
public:
  sTextApp();
  sBool OnCommand(sU32 cmd);
  void OnKey(sU32 key);
  void UpdateStatus();

  sFileWindow *File;
};

/****************************************************************************/

#endif

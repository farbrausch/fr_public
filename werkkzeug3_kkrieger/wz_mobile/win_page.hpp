// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once

#include "GuiOpPage.hpp"
#include "doc.hpp"

/****************************************************************************/

class PageWin_ : public sGuiOpPage<GenOp>
{
  sToolBorder *Tools;
  sInt LastAdd;

  sString<1024> RenameMsg;
  GenName RenameOld;
  GenName RenameNew;
public:
  PageWin_();
  ~PageWin_();
  void PaintOp(const GenOp *op);
  sBool OnCommand(sU32 cmd);
  void OnPaint();
  void AddOpMenu(sInt type);
  void AddOp(sInt id);
  void Exchange(GenOp *op);

  enum PageWinCmd
  {
    CMD_PAGE_ADDOP    = 0x2000,       // requirex 0x1??? !!!
    CMD_PAGE_SHOW0    = 0x1100,
    CMD_PAGE_SHOW1,
    CMD_PAGE_FLUSHCACHE,
    CMD_PAGE_GOTO,
    CMD_PAGE_EXCHANGE,
    CMD_PAGE_BYPASS,
    CMD_PAGE_HIDE,
    CMD_PAGE_ADD0,
    CMD_PAGE_ADD1,
    CMD_PAGE_ADD2,
    CMD_PAGE_LOG,
    CMD_PAGE_RENAME0,
    CMD_PAGE_RENAME1,
  };

};

/****************************************************************************/

// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once

#include "_types2.hpp"
#include "doc.hpp"
#include "GuiSingleList.hpp"


class PageListWin_ : public sGuiSingleList<GenPage>
{
  sToolBorder *Tools;
public:
  enum CommandEnum
  {
    CMD_PAGELIST_ADD = 0x1000,
    CMD_PAGELIST_RENAME,
    CMD_PAGELIST_DELETE,
    CMD_PAGELIST_MENU,
    CMD_PAGELIST_ADD2,
    CMD_PAGELIST_RENAME2,
  };
  PageListWin_();
  void OnKey(sU32 key);
  sBool OnCommand(sU32 cmd);
  sString<32> NewName;
  GenPage *GetPage();
  void SetPage(GenPage *);
};

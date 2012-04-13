// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "win_pagelist.hpp"
#include "main_mobile.hpp"

PageListWin_::PageListWin_()
{
  SetList(Doc->Pages);
  Select(0);
  LeftCmd = CMD_MAIN_CHANGEPAGE;

  static sGuiMenuList ml[] = 
  {
    { sGML_COMMAND  ,'a'                    ,CMD_PAGELIST_ADD     ,0,"Add Page" },
    { sGML_COMMAND  ,'r'                    ,CMD_PAGELIST_RENAME  ,0,"Rename Page" },
    { sGML_COMMAND  ,sKEY_DELETE            ,CMD_PAGELIST_DELETE  ,0,"Delete Page" },
    { sGML_SPACER   },
    { sGML_COMMAND  ,sKEY_UP                ,CMD_LIST_UP          ,0,"Up" },
    { sGML_COMMAND  ,sKEY_DOWN              ,CMD_LIST_DOWN        ,0,"Down" },
    { sGML_COMMAND  ,sKEY_UP    |sKEYQ_CTRL ,CMD_LIST_MOVEUP      ,0,"Move up" },
    { sGML_COMMAND  ,sKEY_DOWN  |sKEYQ_CTRL ,CMD_LIST_MOVEDOWN    ,0,"Move down" },
    { 0 },
  };
  SetMenuList(ml);
  Tools = new sToolBorder;
  Tools->AddLabel(".page library");
  Tools->AddContextMenu(CMD_PAGELIST_MENU);
  AddBorder(Tools);
  AddScrolling(0,1);
}

void PageListWin_::OnKey(sU32 key)
{
  MenuListKey(key);
  __super::OnKey(key);
}

sBool PageListWin_::OnCommand(sU32 cmd)
{
  sDialogWindow *dlg;
  GenPage *page;

  switch(cmd)
  {
  case CMD_PAGELIST_MENU:
    PopupMenuList();
    return 1;

  case CMD_PAGELIST_ADD:
    NewName = "<< new page >>";
    dlg = new sDialogWindow;
    dlg->InitString(NewName,NewName.Size());
    dlg->InitOkCancel(this,"add new page","enter name",CMD_PAGELIST_ADD2,0);
    return 1;

  case CMD_PAGELIST_ADD2:
    Doc->Pages->Add(new GenPage(NewName));
    return 1;

  case CMD_PAGELIST_RENAME:
    page = GetPage();
    NewName = (sChar *)page->Name;
    dlg = new sDialogWindow;
    dlg->InitString(NewName,NewName.Size());
    dlg->InitOkCancel(this,"rename page","enter new name",CMD_PAGELIST_RENAME2,0);
    return 1;

  case CMD_PAGELIST_RENAME2:
    page = GetPage();
    page->Name = (sChar *)NewName;
    return 1;

  case CMD_PAGELIST_DELETE:
    page = GetPage();
    if(page)
      Doc->Pages->RemOrder(page);
    return 1;

  default:
    return __super::OnCommand(cmd);
  }
}

GenPage *PageListWin_::GetPage()
{
  sInt i = GetSelect();
  if(i>=0 && i<Doc->Pages->GetCount())
    return Doc->Pages->Get(i);
  else
    return 0;
}

void PageListWin_::SetPage(GenPage *page)
{
  GenPage *gp;
  sFORLIST(Doc->Pages,gp)
  {
    if(gp == page)
    {
      Select(_i);
      break;
    }
  }
}

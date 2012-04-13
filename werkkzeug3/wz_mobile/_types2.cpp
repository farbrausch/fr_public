// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types2.hpp"


/****************************************************************************/
/****************************************************************************/

void sGuiWindow2::SetMenuList(sGuiMenuList *l)
{
  MenuList = l;
}

sBool sGuiWindow2::MenuListKey(sU32 key)
{
  if(MenuList==0) return sFALSE;

  if(key&sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
  if(key&sKEYQ_CTRL ) key |= sKEYQ_CTRL ;

  for(sInt i=0;MenuList[i].Kind!=sGML_END;i++)
  {
    if((key&(0x8001ffff|sKEYQ_SHIFT|sKEYQ_CTRL|sKEYQ_ALT))==MenuList[i].Shortcut)
    {      
      OnCommand(MenuList[i].Command);
      return sTRUE;
    }
  }
  if(key==sKEY_APPPOPUP)
    PopupMenuList(1);
  return sFALSE;
}

void sGuiWindow2::PopupMenuList(sBool popup)
{
  sMenuFrame *mf;
  sGuiMenuList *e;
  sInt state;

  if(MenuList==0) return;

  mf = new sMenuFrame;
  for(sInt i=0;MenuList[i].Kind;i++)
  {
    e = &MenuList[i];
    switch(e->Kind)
    {
    case sGML_COMMAND:
      if(e->Command && e->Name)
        mf->AddMenu(e->Name,e->Command,e->Shortcut);
      break;
    case sGML_CHECKMARK:
      if(e->Command && e->Name)
      {
        state = *(sInt *)(  ( ((sU8 *)this) +e->Offset)  );
        mf->AddCheck(e->Name,e->Command,e->Shortcut,state);
      }
      break;
    case sGML_SPACER:
      mf->AddSpacer();
      break;
    }
  }
  mf->AddBorder(new sNiceBorder);
  mf->SendTo = this;
  if(popup)
    sGui->AddPopup(mf);
  else
    sGui->AddPulldown(mf);
}

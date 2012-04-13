// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once

#include "_types2.hpp"

enum sGuiSingleListCmd
{
  CMD_LIST_MOVEUP     = 0x0100,
  CMD_LIST_MOVEDOWN   = 0x0101,
  CMD_LIST_UP         = 0x0102,
  CMD_LIST_DOWN       = 0x0103,
};

template <class Type> class sGuiSingleList : public sGuiWindow2
{
  sList<Type> *List;
  sInt Height;
  sInt Selected;
  sInt DragStartX;
  sInt DragStartY;

public:

  sInt LeftCmd;
  sInt MenuCmd;
  sInt DoubleCmd;

  sGuiSingleList()
  {
    List = 0;
    Selected = 0;
    LeftCmd = 0;
    MenuCmd = 0;
    DoubleCmd = 0;
  }

  ~sGuiSingleList() 
  {
  }

  void SetList(sList<Type> *x)
  {
    List = x;
  }

  void Tag()
  {
    sGuiWindow2::Tag();
    sBroker->Need(List);
  }

  void OnCalcSize()
  {
    Height = PrintHeight()+2;
    SizeX = 75;
    SizeY = List->GetCount()*Height;
  }

  void OnPaint()
  {
    sRect r;
    Type *e;

    ClearBack();
    r = Client;
    r.y1 = r.y0+Height;
    sInt i = 0;
    sFORLIST(List,e)
    {
      sU32 col;
      if(i==Selected)
      {
        col = sGC_SELECT;
        Paint(r,sGC_SELBACK);
      }
      else
      {
        col = sGC_TEXT;
      }
      r.x0 += 4;
      Print(r,sFA_LEFT,e->Name,col);
      r.x0 -= 4;
      r.y0 += Height;
      r.y1 += Height;
      i++;
    }
  }

  void OnDrag(sDragData &dd)
  {
    sInt nr = (dd.MouseY-Client.y0)/Height;
    if(nr<0 || nr>=List->GetCount())
      nr = -1;
    if(dd.Mode==sDD_START || dd.Mode==sDD_DRAG || dd.Mode==sDD_STOP)
    {
      if(Selected!=nr)
      {
        Selected = nr;
        Post(LeftCmd);
      }
    }

    if(dd.Mode==sDD_START)
    {
      if((dd.Buttons&4) && dd.Flags&sDDF_DOUBLE)
      {
        Post(MenuCmd);
        return;
      }
      if((dd.Buttons&1) && dd.Flags&sDDF_DOUBLE)
      {
        Post(DoubleCmd);
        return;
      }
    }
    if(MMBScrolling(dd,DragStartX,DragStartY)) return ;
  }

  void OnKey(sU32 key)
  {
  }

  sBool OnCommand(sU32 cmd)
  {
    switch(cmd)
    {
    case CMD_LIST_MOVEUP:
      if(Selected>=1 && Selected<List->GetCount())
      {
        List->Swap(Selected-1,Selected);
        Selected--;
      }
      return 1;

    case CMD_LIST_MOVEDOWN:
      if(Selected>=0 && Selected<List->GetCount()-1)
      {
        List->Swap(Selected,Selected+1);
        Selected++;
      }
      return 1;

    case CMD_LIST_UP:
      if(Selected>=1 && Selected<List->GetCount())
        Selected--;
      return 1;

    case CMD_LIST_DOWN:
      if(Selected>=0 && Selected<List->GetCount()-1)
        Selected++;
      return 1;
    }
    return 0;
  }

  Type *Get()
  {
    if(Selected<0 || Selected>=List->GetCount())
      return 0;
    else
      return List->Get(Selected);
  }

  sInt GetSelect()
  {
    return Selected;
  }

  void Select(sInt i)
  {
    Selected = i;
  }

  void UnSelect()
  {
    Selected = -1;
  }
};

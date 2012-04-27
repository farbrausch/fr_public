/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) 2005-2006 by Dierk Ohlerich                          ***/
/***   all rights reserved                                                ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_GUI_TABS_HPP
#define FILE_GUI_TABS_HPP

#include "base/types2.hpp"
#include "gui/gui.hpp"

/****************************************************************************/

class sTabBorderBase : public sWindow     // this is a border!
{
  struct Info
  {
    sRect Client;
    sRect Kill;
  };
  sRect Rect;
  sInt Height;
  sArray<Info> Rects;
  sInt HoverTab;
  sInt HoverKill;
  sInt ActiveTab;
  sInt MoveBefore;
  sInt ScrollTo;

  sInt ScrollX0;
  sInt ScrollX1;
  sInt ScrollWidth;
  sInt Scroll;
  sInt DragScroll;
public:
  sTabBorderBase();
  ~sTabBorderBase();

  void OnPaint2D();
  void OnLayout();
  void OnCalcSize();
  sBool OnKey(sU32 key);
  void OnDrag(const sWindowDrag &dd);

  sInt GetTab();
  void SetTab(sInt tab);
  void DelTab(sInt tab);

  sMessage ChangeMsg;

  virtual sInt GetTabCount()=0;
  virtual const sChar *GetTabName(sInt n)=0;
  virtual void DeleteTab(sInt n) {}
  virtual void MoveTab(sInt from,sInt to) {}
};


template <class Type> class sTabBorder : public sTabBorderBase
{
  sArray<Type> *Tabs;
public:
  sTabBorder(sArray<Type>&t) { Tabs = &t; }
  void Tag() { sTabBorderBase::Tag(); sNeed(*Tabs); }
  sInt GetTabCount() { return Tabs ? Tabs->GetCount() : 0; }
  const sChar *GetTabName(sInt n) { return (*Tabs)[n]->Label; }
  void DeleteTab(sInt n) { Tabs->RemAtOrder(n); }
  void MoveTab(sInt from,sInt to) { Type e=(*Tabs)[from]; Tabs->RemAtOrder(from); Tabs->AddBefore(e,to); }
};

/****************************************************************************/

#endif // FILE_GUI_TABS_HPP


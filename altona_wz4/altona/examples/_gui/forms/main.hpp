/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "base/system.hpp"
#include "gui/gui.hpp"
#include "gui/listwindow.hpp"
#include "gui/textwindow.hpp"
#include "gui/color.hpp"

/****************************************************************************/

class ListItem : public sObject
{
public:
  sInt Select;
  sInt Select2;
  sInt Select3;
  sString<64> String;
  sInt Value;
  sU32 Color;
  sInt Choice;
  const sChar *ConstString;
  sPoolString PoolString;
  sColorGradient *Gradient;

  sListWindowTreeInfo<ListItem *> TreeInfo;

  sCLASSNAME_NONEW(ListItem);
  ListItem(const sChar *str,sInt val,sU32 col);
  ListItem();
 
  const sChar *GetName();
  void Tag();
//  void OnPaintColumn(sInt column,const sRect &client,sBool select);
  void OnAddNotify(sWindow *w) { w->AddNotify(this,sizeof(*this)); }
  void AddGui(sGridFrameHelper &gh);
};

/****************************************************************************/

enum MainWindowCommand
{
  CMD_UPDATEDONE = 0x1001,
  CMD_UPDATECHANGE = 0x1002,
};

class MainWindow : public sWindow
{
  class WinFrame *FrameWin;
  class WinText *TextWin;
  sMultiListWindow<ListItem> *ListWin;
  sMultiTreeWindow<ListItem> *TreeWin;
  sGridFrame *ItemWin;

public:
  sCLASSNAME(MainWindow);
  MainWindow();
  ~MainWindow();
  void Tag();
 
  void UpdateText(sDInt mode);
  void UpdateItemList();
  void UpdateItemTree();
  void AddListItem(sDInt pos);
  void CmdDialog();
  void CmdComplicated();

  sArray<ListItem *> Items;

  sInt ints[16];
  sString<8> strings[1];
  sF32 floats[16];
};

/****************************************************************************/

class WinFrame : public sWireClientWindow
{
  sGridFrame *Grid;
public:
  sCLASSNAME(WinFrame);
  WinFrame();
  ~WinFrame();

  void InitWire(const sChar *name);
  void Reset(sDInt);
  sInt Default;
};

class WinText : public sWireClientWindow
{
  sTextBuffer TextBuffer;
public:
  sCLASSNAME(WinText);
  sTextWindow *TextWin;
  WinText();
  void InitWire(const sChar *name);

  void UpdateText(sDInt mode);
};

/****************************************************************************/

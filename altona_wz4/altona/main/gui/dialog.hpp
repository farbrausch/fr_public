/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_GUI_DIALOG_HPP
#define FILE_GUI_DIALOG_HPP

#ifndef __GNUC__
#pragma once
#endif

#include "gui/window.hpp"
#include "gui/manager.hpp"
#include "base/windows.hpp"

/****************************************************************************/

void sOpenFileDialog(const sChar *label,const sChar *extensions,sInt flags,const sStringDesc &buffer,const sMessage &ok,const sMessage &cancel,sObject *tagme=0);
void sOpenFileDialog(const sChar *label,const sChar *extensions,sInt flags,sPoolString &ps,const sMessage &ok,const sMessage &cancel,sObject *tagme=0);
void sErrorDialog(const sMessage &msg,const sChar *text);
void sOkDialog(const sMessage &ok,const sChar *text);
void sOkCancelDialog(const sMessage &ok,const sMessage &cancel,const sChar *text);
void sYesNoDialog(const sMessage &yes,const sMessage &no,const sChar *text);

void sStringDialog(const sStringDesc &string,const sMessage &ok,const sMessage &cancel,const sChar *text);
void sStringDialog(sPoolString *string,const sMessage &ok,const sMessage &cancel,const sChar *text);

void sFindDialog(const sChar *headline,const sStringDesc &buffer,const sMessage &ok,void *ref,void (*inc)(sArray<sPoolString> &,const sChar *,void *ref));

sPRINTING1(sErrorDialogF   , sString<1024> tmp; sFormatStringBuffer buf=sFormatStringBase(tmp,format);buf,sErrorDialog   (arg1,     (const sChar*)tmp);,const sMessage &)
sPRINTING1(sOkDialogF      , sString<1024> tmp; sFormatStringBuffer buf=sFormatStringBase(tmp,format);buf,sOkDialog      (arg1,     (const sChar*)tmp);,const sMessage &)
sPRINTING2(sOkCancelDialogF, sString<1024> tmp; sFormatStringBuffer buf=sFormatStringBase(tmp,format);buf,sOkCancelDialog(arg1,arg2,(const sChar*)tmp);,const sMessage &,const sMessage &)
sPRINTING2(sYesNoDialogF   , sString<1024> tmp; sFormatStringBuffer buf=sFormatStringBase(tmp,format);buf,sYesNoDialog   (arg1,arg2,(const sChar*)tmp);,const sMessage &,const sMessage &)

// open file flags, defined in base/windows:

//  sSOF_LOAD = 1,
//  sSOF_SAVE = 2,
//  sSOF_DIR = 3,


/****************************************************************************/

class sMultipleChoiceDialog : public sWindow
{
private:
  struct Item
  {
    sPoolString Text;
    sMessage Cmd;
    sU32 Shortcut;
    sRect Rect;
  };
  sArray<Item> Items;
  sPoolString Title;
  sPoolString Text;
  sInt PressedItem;
  sInt PressedOk;

public:
  sMultipleChoiceDialog(const sChar *title,const sChar *text);
  ~sMultipleChoiceDialog();
  void OnPaint2D();
  void OnCalcSize();
  sBool OnKey(sU32 key);
  void OnDrag(const sWindowDrag &dd);

  void AddItem(const sChar *text,const sMessage &,sU32 shortcut);
  void Start();
};

void sQuitReallyDialog(const sMessage &quit,const sMessage &savequit);
void sContinueReallyDialog(const sMessage &cont,const sMessage &savecont);

/****************************************************************************/

// There can only be one of these at any given moment in time, and it is
// stored in sProgressDialog::Instance.
class sProgressDialog
{
  struct Bracket
  {
    sF32 Start,End;

    Bracket() {}
    Bracket(sF32 st,sF32 en) : Start(st), End(en) {}
  };

  sString<256> Title;
  sString<256> Text;
  sF32 Percentage;
  sInt LastUpdate;

  sRect WindowRect;
  sBool CancelFlag;

  sArray<Bracket> BracketStack;
  
  static sProgressDialog *Instance;

  sProgressDialog(const sChar *title,const sChar *text);
  ~sProgressDialog();

  void OnCancel();

  void Render();

public:
  static void Open(const sChar *title,const sChar *text);
  static sBool Close(); // returns sFALSE if user pressed cancel

  static void SetText(const sChar *message,sBool forceUpdate=sTRUE);
  static sBool SetProgress(sF32 percentage); // returns sFALSE if user pressed cancel

  // Use PushLevel when starting a sub-operation that would by itself call SetProgress();
  // you can "nest" progress bars this way. After push level, a progress of [0,1] will map to
  // [start,end] relative to the parent.
  static void PushLevel(sF32 start,sF32 end);

  // Same as PushLevel, for the common case where you're iterating over a collection of stuff.
  static void PushLevelCounter(sInt current,sInt max);
  
  // For every push, there's a matching pop.
  static void PopLevel();

  static void ChangeLevelCounter(sInt current,sInt max); // first push, then pop
};

struct sProgressDialogScope
{
  sProgressDialogScope(const sChar *text,sInt current,sInt max);
  ~sProgressDialogScope();
};

/****************************************************************************/

void sOpenGuiThemeDialog(sGuiTheme *theme, sBool live=sTRUE);

/****************************************************************************/

#endif

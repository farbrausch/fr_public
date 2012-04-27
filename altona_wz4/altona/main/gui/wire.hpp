/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_GUI_WIRE_HPP
#define FILE_GUI_WIRE_HPP

#ifndef __GNUC__
#pragma once
#endif

#include "gui/window.hpp"
#include "gui/manager.hpp"
#include "gui/frames.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   WireMaster                                                         ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   This is similar to LayoutFrame, but much more capable...           ***/
/***                                                                      ***/
/****************************************************************************/

namespace sWireNamespace
{
  struct FormInstance;
  struct Form;
  struct Screen;
  struct Layout;
  struct CommandBase;
  struct CommandMenu;
  struct CommandTool;
  struct Shortcut;
  struct RawShortcut;
}

class sWireMasterWindow : public sWindow
{
public:
  typedef void(sWindow::*DragFuncType)(const sWindowDrag &dd,sDInt);
  typedef void(sWindow::*CmdFuncType)(sDInt);
private:

  // document

  sArray<sWireNamespace::Form *> Classes;
  sArray<sObject *> Forms;                  // registered forms, some of them being windows
  sArray<sWindow *> Windows;                // registered child windows.
  sArray<sWireNamespace::Screen *> Screens;
  sArray<sWireNamespace::Screen *> Popups;
  sArray<sWireNamespace::CommandMenu *> Menus;
  sArray<sWireNamespace::CommandBase *> Garbage;   // various intermediate commands
  sArray<sLayoutFrameWindow *> ScreenSwitchAdd;     // add screenswitched to these toolbars...
  sArray<sWireNamespace::Shortcut *> GlobalKeys;

  class sLayoutFrame *LayoutFrame;
  class sLayoutFrame *PopupFrame;
  sWireNamespace::Form *FindClass(sPoolString name);
  sWireNamespace::Form *MakeClass(sPoolString name);
  sWireNamespace::FormInstance *FindInstance(sObject *obj);
  sWindow *FindWindow(sPoolString name,sInt index=0);

  sBool ProcessEndWasCalled;
  sInt CurrentScreen;
  sWindow *FullscreenWindow;
  const sChar *CurrentToolName;

  // the parser

  class sScanner *Scan;
  const sChar *Filename;

//  void Error(const sChar *str);
  sWireNamespace::FormInstance *_FormName(sBool window);
  void _Key(sU32 &key,sInt &qual,sInt &hit);

  void _ScreenAlign(sLayoutFrameWindow *lfw);
  void _ScreenLevel(sLayoutFrameWindow *parent);
  void _OnSwitch(sLayoutFrameWindow *parent);
  void _Screen();
  void _Popup();
  sWireNamespace::CommandBase *_Command(sWireNamespace::Form *wc);

  void _Add0(sWireNamespace::RawShortcut &sc);
  void _Add(sWireNamespace::RawShortcut &sc);
  sWireNamespace::Shortcut *MakeShortcut(sWireNamespace::Form *wc,sInt type,sWireNamespace::RawShortcut &raw);

  sWireNamespace::CommandMenu *_Menu(sWireNamespace::Form *);
  void _Sets(sArray<struct sWireNamespace::Form *> &windows);
  void _WindowCmd(sArray<struct sWireNamespace::Form *> &windows,sInt sets);
  void _Window();
  void _Global();

  // private stuff

  void AddForm(const sChar *classname,sObject *win,const sChar *instancename,sBool iswindow);
  void DoLayout();
  sMessage DragMessage;
  sWindow *DragModeFocus;
  sInt DragModeQual;
  sWindow *MainWindow;
  sU32 QualScope;

  sBool IsGlobal(sU32 m);
  sBool CanBeGlobal(sU32 m);
public:
  sMenuFrame *CurrentMenu;                    // menu creation. strictly intern
private:

  enum sWireMasterParaType
  {
    sWPT_INT = 1,
    sWPT_FLOAT,
    sWPT_CHOICE,
    sWPT_ARGB,
    sWPT_RGB,
  };

  // gui stubs

  void CmdMakeMenu(struct sWireNamespace::CommandMenu *menu);
  void CmdMakePulldown(sDInt menuptr);
  void CmdMakePopup(sDInt menuptr);

  void FormChangeTool(sWireNamespace::Form *form,sWireNamespace::CommandTool *newtool);

public:
  sCLASSNAME(sWireMasterWindow);
  sWireMasterWindow();
  ~sWireMasterWindow();
  void Tag();

  // windows handling

  sBool HandleKey(sWindow *win,sU32 key);
  sBool HandleCommand(sWindow *win,sInt cmd);
  sBool HandleShortcut(sWindow *win,sU32 key) { return 0; }
  sBool HandleDrag(sWindow *win,const sWindowDrag &dd,sInt hit);
  sBool OnShortcut(sU32 key);

  // registration

  void AddForm(const sChar *classname,sObject *win,const sChar *instancename=0);
  void AddWindow(const sChar *classname,sWindow *win,const sChar *instancename=0);

  void AddKey(const sChar *classname,const sChar *commandname,const sMessage &msg);
  void AddTool(const sChar *classname,const sChar *commandname,const sMessage &msg);
  void AddDrag(const sChar *classname,const sChar *commandname,const sMessage &msg);
  void AddCallback(const sChar *classname,const sChar *commandname,const sMessage &msg);
  void AddPara1(const sChar *classname,const sChar *commandname,const sMessage &msg,sInt type,sDInt offset,const sChar *choices,sF32 min,sF32 max,sF32 step);
  void AddChoice(const sChar *classname,const sChar *commandname,const sMessage &msg,sInt *ptr,const sChar *choices);

  void OverrideCmd(const sChar *classname, const sChar *commandname,const sMessage &msg);
 
  template<class T> void AddParaInt   (const sChar *classname,const sChar *commandname,const sMessage &msg,sInt T::*ptr,sF32 min,sF32 max,sF32 step) { AddPara1(classname,commandname,msg,sWPT_INT   ,(sDInt)(&(((T*)0)->*ptr)),0,min,max,step); }
  template<class T> void AddParaFloat (const sChar *classname,const sChar *commandname,const sMessage &msg,sF32 T::*ptr,sF32 min,sF32 max,sF32 step) { AddPara1(classname,commandname,msg,sWPT_FLOAT ,(sDInt)(&(((T*)0)->*ptr)),0,min,max,step); }
  template<class T> void AddParaChoice(const sChar *classname,const sChar *commandname,const sMessage &msg,sInt T::*ptr,const sChar *choices)        { AddPara1(classname,commandname,msg,sWPT_CHOICE,(sDInt)(&(((T*)0)->*ptr)),choices,0,0,0); }
  template<class T> void AddParaBool  (const sChar *classname,const sChar *commandname,const sMessage &msg,sInt T::*ptr)                             { AddPara1(classname,commandname,msg,sWPT_CHOICE,(sDInt)(&(((T*)0)->*ptr)),L"off|on",0,0,0); }
//  template<class T> void AddParaColor (const sChar *classname,const sChar *commandname,const sMessage &msg,sU32 T::*ptr,sBool alpha)                 { AddPara1(classname,commandname,msg,sInt(alpha?sWPT_ARGB:sWPT_RGB),(sDInt)(&(((T*)0)->*ptr))); }
  void ProcessFile(const sChar *name);
  void ProcessText(const sChar *text,const sChar *reffilename=0);   // filename is for error messages
  void ProcessText(const sChar8 *text,const sChar *reffilename=0);
  void ProcessEnd();

  // control

  void SwitchScreen(sInt scr) { CmdSwitchScreen(scr); }
  void SwitchScope(sInt scope);
  void SetSubSwitch(sPoolString name,sInt nr);
  sInt GetSubSwitch(sPoolString name);
  sInt GetScreen() { return LayoutFrame->GetSwitch(); }
  void SetFullscreen(sWindow *,sBool toggle=1);
  void Popup(const sChar *name);
  void CmdClosePopup();
  void CmdSwitchScreen(sDInt screen);
  sMessage ToolChangedMsg;
  void ChangeCurrentTool(sWindow *window,const sChar *toolSymbolName); // 0 to disable tool
  const sChar *GetCurrentToolName() { return CurrentToolName; }
  void Help(sWindow *window);
};

extern sWireMasterWindow *sWire;

/****************************************************************************/
/***                                                                      ***/
/***   WireClient                                                         ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***  you do not need to use this class, you only need to call the        ***/
/***  Handle routines of the WireMaster as outlined here.                 ***/
/***                                                                      ***/
/****************************************************************************/

class sWireClientWindow : public sWindow
{
public:
  sCLASSNAME(sWireClientWindow);
  sWireClientWindow();
  ~sWireClientWindow();
  virtual void InitWire(const sChar *name);

  virtual sBool OnCheckHit(const sWindowDrag &dd) { return 0; }
  sBool OnKey(sU32 key) { return sWire->HandleKey(this,key); }
  sBool OnCommand(sInt cmd) { return sWire->HandleCommand(this,cmd); }
  sBool OnShortcut(sU32 key) { return sWire->HandleShortcut(this,key); }
  void OnDrag(const sWindowDrag &dd) { sWire->HandleDrag(this,dd,OnCheckHit(dd)); }

  // common functions

  void DragScroll(const sWindowDrag &dd,sDInt);
  void CmdMaximize() { sWire->SetFullscreen(this,1); }     // maximise this windows
  void CmdHelp();
  void OpenCode(const sChar *);
};

class sWireDummyWindow : public sWireClientWindow
{
public:
  sCLASSNAME(sWireDummyWindow);
  sWireDummyWindow();
  const sChar *Label;
  void OnPaint2D();
};

class sWireGridFrame : public sGridFrame
{
public:
  sBool OnKey(sU32 key) { if(sGridFrame::OnKey(key)) return 1; return sWire->HandleKey(this,key); }
  sBool OnCommand(sInt cmd) { return sWire->HandleCommand(this,cmd); }
  sBool OnShortcut(sU32 key) { return sWire->HandleShortcut(this,key); }
  void OnDrag(const sWindowDrag &dd) { sWire->HandleDrag(this,dd,0); }

  void InitWire(const sChar *name);
  void DragScroll(const sWindowDrag &dd,sDInt);
  void CmdMaximize() { sWire->SetFullscreen(this,1); }     // maximise this windows
};

/****************************************************************************/

#endif

// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once

#include "_types2.hpp"
#include "GuiSingleList.hpp"
#include "doc.hpp"

#define BACKUPA "backup/"
#define BACKUPQ "backup/"



enum MainCommands
{
  CMD_MAIN_CHANGEPAGE = 0x1000,
  CMD_MAIN_FILE,
  CMD_MAIN_EDIT,  

  CMD_MAIN_FILE_NEW,
  CMD_MAIN_FILE_OPENAS,
  CMD_MAIN_FILE_OPEN,
  CMD_MAIN_FILE_SAVEAS,
  CMD_MAIN_FILE_SAVE,
  CMD_MAIN_FILE_EXPORTMOBILE,
  CMD_MAIN_FILE_EXPORTBITMAPS,
  CMD_MAIN_FILE_AUTOSAVE,

  CMD_MAIN_EDIT_SMALLFONTS,
  CMD_MAIN_EDIT_SKIN2005,
  CMD_MAIN_EDIT_SWAPVIEWS,
};

/****************************************************************************/

enum Wz3TexConsts
{
  MAX_VIEWS = 2,
};

class Wz3Tex : public sDummyFrame
{
  class PageListWin_ *PageListWin;
  class PageWin_ *PageWin;
  class ParaWin_ *ParaWin;
  class sLogWindow *LogWin;

  class ViewBitmapWin_ *ViewBitmapWin[MAX_VIEWS];
  class ViewMeshWin_ *ViewMeshWin[MAX_VIEWS];
  sSwitchFrame *ViewSwitch[MAX_VIEWS];
  sSwitchFrame *ParaSwitch;

  sInt FinalDestruction;        
  sString<4096> DocPath;
public:

  Wz3Tex();
  ~Wz3Tex();
  sBool OnCommand(sU32);
  void OnPaint();
  void OnFrame();
  void Tag();
  sBool OnShortcut(sU32 key);

  sStatusBorder *StatusBorder;              // link to statusborder
  sString<128> StatusMouse;         // current mousebutton meaning
  sString<128> StatusWindow;        // general status of the current Window
  sString<128> StatusObject;        // general status of the current Object
  sString<128> StatusLog;           // error and other log messages

  sInt AutosaveTimer;
  sInt AutosaveRequired;
  sInt SwapViews;                   // change 's' <-> 'S' key in page-windows
  sGuiWindow *CurrentViewWin;      // last viewport that was activated with ShowOp()

  // interface between the editor windows

  void ShowOp(GenOp *op,sInt viewport);
  void EditOp(GenOp *op);
  void UnsetOp(GenOp *op);
  void UnsetPage(GenPage *page);
  sBool IsShowed(const GenOp *op);
  sBool IsEdited(const GenOp *op);
  GenOp *GetEditOp();
  void SetPage(GenPage *page);
  void SetPage(GenOp *op);
  void GotoOp(GenOp *op);
  void Change();

  void ShowPara();
  void ShowLog();
  void ClearLog();
  void Log(const sChar *bla);
  void LogF(const sChar *format,...);

  void SaveConfig();
  void LoadConfig();
};

extern class Wz3Tex *App;

/****************************************************************************/

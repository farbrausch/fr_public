// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __APP_BROWSER_HPP__
#define __APP_BROWSER_HPP__

#include "_util.hpp"
#include "_gui.hpp"
#include "_diskitem.hpp"

#if sLINK_DISKITEM

/****************************************************************************/
/***                                                                      ***/
/***   windows                                                            ***/
/***                                                                      ***/
/****************************************************************************/

class sBrowserApp : public sGuiWindow
{
  sDiskItem *Root;
  class sBrowserTree *Tree;
  class sBrowserList *List;
  sGuiWindow *CustomWindow;
  sVSplitFrame *Split;
  sInt TreeFocusDelay;
public:
  sBrowserApp();
  void Tag();
  void OnCalcSize();
  void OnLayout();
  void OnPaint();
  void OnKey(sU32 key);
  void OnDrag(sDragData &);
  void OnFrame();
  sBool OnCommand(sU32 cmd);
  void SetPath(sChar *path);
  void GetFileName(sChar *buffer,sInt size);
  void GetDirName(sChar *buffer,sInt size);
  void GetPath(sChar *buffer,sInt size);
  void SetRoot(sDiskItem *root);
  void TreeFocus();
  void GoParent();
  void Reload();

  sInt DirCmd;
  sInt FileCmd;
  sInt DoubleCmd;
  sInt ExitCmd;
  sGuiWindow *SendTo;
};

class sFileWindow : public sGuiWindow
{
  sBrowserApp *Browser;
  sControl *PathCon;
  sControl *NameCon;
  sControl *OkCon;
  sControl *CancelCon;
  sControl *ParentCon;
  sControl *ReloadCon;
  sChar Path[2048];
  sChar Name[128];
  sInt Height;
//  sInt DragStartX,DragStartY;
public:
  sFileWindow();
  void OnLayout();
  void OnPaint();
//  void OnDrag(sDragData &dd);
  sBool OnCommand(sU32);
  void GetPath(sChar *buffer,sInt size);
  void GetFile(sChar *buffer,sInt size);
  void SetPath(sChar *path);
  void SetFocus(sBool save);
  sBool ChangeExtension(sChar *path,sChar *newext);

  sGuiWindow *SendTo;
  sInt OkCmd;
  sInt CancelCmd;
};

// implements Load/Save/New/Exit with sGuiWindow::FileMenu()

sBool sImplementFileMenu(sU32 cmd,sGuiWindow *,sObject *obj,sFileWindow *file,sBool dirty,sInt maxsave=128*1024*1024);

#define sCMDLS_MERGEIT     0x00ef
#define sCMDLS_NEW        0x00f0  // from menu
#define sCMDLS_OPEN       0x00f1  // from menu
#define sCMDLS_SAVEAS     0x00f2  // from menu
#define sCMDLS_SAVE       0x00f3  // from menu
#define sCMDLS_BROWSER    0x00f4  // from menu
#define sCMDLS_EXIT       0x00f5  // from menu
#define sCMDLS_CLEAR      0x00f6  // overload if obj->Clear() does not work
#define sCMDLS_READ       0x00f7  // overload if obj->Read() does not work
#define sCMDLS_WRITE      0x00f8  // overload if obj->Write() does not work
#define sCMDLS_WRITEb     0x00f9  // internal
#define sCMDLS_QUICKSAVE  0x00fa  // from menu
#define sCMDLS_AUTOSAVE   0x00fb  // from menu
#define sCMDLS_MERGE      0x00fc  // overload if obj->Merge() does not work
#define sCMDLS_EXITx      0x00fd
#define sCMDLS_CANCEL     0x00fe
#define sCMDLS_READx      0x00ff


#define sBTCMD_SELDIR   0x10101
#define sBLCMD_SELECT   0x10201
#define sBLCMD_POPUP    0x10202
#define sBLCMD_DOUBLE   0x10203
#define sBLCMD_PARENT   0x10204
#define sBWCMD_FOCTREE  0x10301
#define sBWCMD_FOCLIST  0x10302
#define sBWCMD_EXIT     0x10303


/****************************************************************************/

#endif
#endif

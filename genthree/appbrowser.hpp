// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __APP_BROWSER_HPP__
#define __APP_BROWSER_HPP__

#include "_util.hpp"
#include "_gui.hpp"
#include "_diskitem.hpp"

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

  sInt DirCmd;
  sInt FileCmd;
  sInt DoubleCmd;
  sGuiWindow *SendTo;
};

class sFileWindow : public sGuiWindow
{
  sBrowserApp *Browser;
  sControl *PathCon;
  sControl *NameCon;
  sControl *OkCon;
  sControl *CancelCon;
  sChar Path[2048];
  sChar Name[128];
  sInt Height;
public:
  sFileWindow();
  void OnLayout();
  void OnPaint();
  sBool OnCommand(sU32);
  void GetPath(sChar *buffer,sInt size);
  void SetPath(sChar *path);
  sBool ChangeExtension(sChar *path,sChar *newext);

  sGuiWindow *SendTo;
  sInt OkCmd;
  sInt CancelCmd;
};

// implements Load/Save/New/Exit with sGuiWindow::FileMenu()

sBool sImplementFileMenu(sU32 cmd,sGuiWindow *,sObject *obj,sFileWindow *file,sBool dirty,sInt maxsave=32*1024*1024);

#define sCMDLS_NEW      0x00f0    // from menu
#define sCMDLS_OPEN     0x00f1    // from menu
#define sCMDLS_SAVEAS   0x00f2    // from menu
#define sCMDLS_SAVE     0x00f3    // from menu
#define sCMDLS_BROWSER  0x00f4    // from menu
#define sCMDLS_EXIT     0x00f5    // from menu
#define sCMDLS_CLEAR    0x00f6    // overload if obj->Clear() does not work
#define sCMDLS_READ     0x00f7    // overload if obj->Read() does not work
#define sCMDLS_WRITE    0x00f8    // overload if obj->Write() does not work
#define sCMDLS_RESERVED 0x00ff    // please reserve up to this

/****************************************************************************/

#endif

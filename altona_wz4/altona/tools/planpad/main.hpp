/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_PLANPAD_MAIN_HPP
#define FILE_PLANPAD_MAIN_HPP

#include "base/types2.hpp"
#include "gui/gui.hpp"
#include "gui/textwindow.hpp"

#include "wiki/layoutwindow.hpp"
#include "wiki/markup.hpp"
#include "gui/tabs.hpp"

/****************************************************************************/
/*
class sWireTextWindow : public sTextWindow
{
public:
  sBool OnKey(sU32 key) { if(sWire->HandleKey(this,key)) return 1; else return sTextWindow::OnKey(key); }
  sBool OnShortcut(sU32 key) { return sWire->HandleShortcut(this,key); }
};
*/
struct TabHistory
{
  sPoolString Dir;
  sPoolString File;
};

class Tab : public sObject
{
public:
  sCLASSNAME(Tab);
  sPoolString Label;
  sPoolString Dir;                // only directory
  sPoolString Filename;           // only filename
  sPoolString Path;               // dir + filename
  sArray<TabHistory> History;
  Tab() {}
  Tab(const sChar *);
};

class MainWindow : public sWireClientWindow
{
  sTextBuffer Text;
  sTextBuffer Undo;

  sWireTextWindow *TextWin;
  sLayoutWindow *ViewWin;
  sStringControl *FindStringWin;
  sChoiceControl *FindDirWin;
  sButtonControl *BackWin;
  sTabBorder<Tab *> *TabWin;

  sInt FontSize;
  sInt EditFontSize;
  sBool DebugUndo;
  sBool DebugTimer;
  sBool RightBorder;
  sBool SaveAsUTF8;

  sString<sMAXPATH> OriginalPath;
  sString<256> FindString;
  sInt FindDir;
  Markup *Parser;
  sInt ClickViewHasHit;
  sFontResource *TextEditFont;
  sString<sMAXPATH> WindowTitle;

  struct FileCacheItem
  {
    FileCacheItem() { Text = 0; }
    ~FileCacheItem() { delete Text; }
    sPoolString Name;
    sTextBuffer *Text;
  };
  sArray<FileCacheItem *> FileCache;
  void ClearFileCache();
  sTextBuffer *LoadFileCache(sPoolString filename);
  const sChar *GetCurrentFile();
  Tab *GetTab();
  Tab *CurrentTab;
  void Unprotect();
  void Protect(const sChar *path);

//  sArray<sPoolString> FileHistory;
  sString<sMAXPATH> ConfigFilename;
  sArray<sDirEntry> DictDir;
public:
  MainWindow();
  void Finalize();
  ~MainWindow();
  void Tag();
  void OnPaint2D();

  void UpdateDoc(const sChar *gotoheadline=0);
  sBool OnCommand(sInt cmd);
  void UpdateWindowTitle();
  void LoadConfig();
  void SaveConfig();

  void FakeScript_FindChapter(sTextBuffer *tb,const sChar *&start,const sChar *&end,sBool section,sPoolString chapter);
  sTextBuffer *FakeScript_Inline(sScanner &scan,sTextBuffer *output,sPoolString *chapter);
  void FakeScript(const sChar *s,sTextBuffer *output);
 
  void Load(const sChar *dir,const sChar *file,const sChar *gotoheadline);
  void CmdOpen();
  void CmdOpenInclude();
  void CmdOpenInclude2(sDInt);
  void CmdOpenDict();
  void CmdOpenDict2(sDInt);
  void CmdLoadDict();
  void CmdSave();
  void CmdSaveAs();
  void CmdClear();
  void CmdRemWriteProtect();
  void CmdTab();
  void CmdNewTab();

  void CmdSwitch(sDInt n);
  void CmdToggle();
  void CmdToggleBoth();
  void CmdUndo();

  void CmdExit();
  void CmdExitDiscard();
  void CmdExitDiscard2();

  void CmdIndex();
  void CmdStartIndex();
  void CmdHtml();
  void CmdPdf();
  void CmdFontSize();
  void CmdEditFontSize();
  void CmdPageMode();
  void CmdChapterFormfeed();
  void CmdDebugBoxes();
  void CmdDocChange();
  void CmdCursorChange();
  void CmdCursorFlash();
  void CmdFindIncremental();
  void CmdFindDone();
  void CmdFind(sDInt);
  void CmdMark(sDInt);
  void CmdMarkRed();
  void CmdRemoveMark();
  void CmdBack();

  void DragClickView(const sWindowDrag &dd);
  void DragEditView(const sWindowDrag &dd);

  sBool DocChanged;
  sPoolString ProtectedFile;
  sArray<Tab *> Tabs;
};

extern MainWindow *App;

/****************************************************************************/

#endif // FILE_PLANPAD_MAIN_HPP


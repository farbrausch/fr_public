/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) by Dierk Ohlerich                                    ***/
/***   all rights reserverd                                               ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WERKKZEUG4_GUI_HPP
#define FILE_WERKKZEUG4_GUI_HPP

#ifndef __GNUC__
#pragma once
#endif

#include "base/types.hpp"
#include "gui/gui.hpp"
#include "gui/3dwindow.hpp"
#include "doc.hpp"

/****************************************************************************/

struct StoreBrowserItem
{
  wOp *Op;
  sString<64> Name;
  sListWindowTreeInfo<StoreBrowserItem *> TreeInfo;
};

enum MainWindowTabs
{
  STATUS_FILENAME = 0,
  STATUS_TOOL = 1,
  STATUS_MESSAGE = 2,
  STATUS_MEMORY = 3,
};

class MainWindow : public sWireClientWindow
{
  sMessage AddPopupMessage;
  sInt AddPopupX,AddPopupY;
  sString<sMAXPATH> ProgramDir;
  sString<sMAXPATH> BackupDir;
  sString<sMAXPATH> BackupFilename;
  sInt BackupIndex;
  sInt AutosaveTimer;
  wEditOptions EditOptionsBuffer;
  wDocOptions DocOptionsBuffer;
  wTestOptions TestOptionsBuffer;

  sArray<wOp *> GotoStack;
  sArray<wOp *> GotoStackRedo;
  sBool GotoOpBase(wOp *,sBool blink = 1);

public:
  sInt SamplePos;
  sInt SampleOffset;
  sF32 BPM;           // just for editing BPM
  sS16 *MusicData;
  sInt MusicSize;

private:
  wDocName RenameTo;
  wDocName RenameFrom;
  wDocName FindBuffer;
  sString<1024> RenameTitle;
  sString<1024> MergeFilename;

  sMessageTimer *BlinkTimer;
  void CheckAfterLoading();
protected:
  virtual void ExtendWireRegister() {}
  virtual void ExtendWireProcess() {}
  virtual void OnDocOptionsChanged() {};
public:
  sCLASSNAME(MainWindow);
  MainWindow();
  ~MainWindow();
  void MainInit();
  void Finalize();
  void Tag();
  sBool OnCommand(sInt cmd);

  void ChangeDoc();
  void UpdateTool();
  void UpdateStatus();
  void UpdateWindows();
  void UpdateImportantOp();
  void ResetWindows();
  void CmdBlinkTimer();
  void PrepareBackup();

  class WinPageList *PageListWin;
  class WinTreeView *TreeViewWin;
  class WinPara *ParaWin[2];
  class WinView *ViewWin[3];
  class WinStack *StackWin;
  class WinStoreBrowser *StoreBrowserWin;
  class WinOpList *OpListWin;
  class WinTimeline *TimelineWin;
  class sStatusBorder *Status;
  class WinCurveOpsList *CurveOpsWin;
  class WinClipOpsList *ClipOpsWin;
  class WinCustom *CustomWin;
  class sGridFrame *DocOptionsWin;
  class sGridFrame *EditOptionsWin;
  class sGridFrame *TestOptionsWin;
  class sGridFrame *ShellSwitchesWin;

  sIntControl *VolumeWin;
  sButtonControl *UnblockChangeWin;
  sChoiceControl *CamSpeedWin;
  sButtonControl *ParaPresetWin;
  class WinPopupText *PopupTextWin;
  sTextBuffer PopupTextBuffer;

  class WikiHelper *Wiki;

  sArray<wOp *> AnimCurveOps;
  sArray<wOp *> AnimClipOps;
  void AnimOpR1(wOp *);
  void AnimOpR2(wOp *);
  void CmdSetClip();
  void CmdSetCurve();

  void GotoOp(wOp *op,sBool blink = 1);
  void GotoUndo();
  void GotoRedo();
  void GotoClear();
  void GotoPush(wOp *op=0); // 0 pushes current edit op

  void EditOp(wOp *op,sInt n);
  void ShowOp(wOp *op,sInt n);
  void AnimOp(wOp *op);
  void EditOpReloadAll();
  wOp *GetEditOp();
  sBool IsShown(wOp *op);
  sBool IsEdited(wOp *op);
  void ClearOp(wOp *op);
  void UpdateViews();
  void DeselectHandles();

  void CalcWasDone();
  sInt GetTimeBeats();
  sInt GetTimeMilliseconds();
  void ScratchTime();
  void SoundHandler(sS16 *samples,sInt count);

  void CmdAddType(sDInt nr);
  void AddPopup(sMessage msg,sBool sec,sInt typekey=0);
  void AddPopup2(sInt typekey=0);
  void AddConversionPopup(sMessage msg);

  void SwitchPage(sInt mode);         // force stack / tree / custom / wiki
  void SwitchPageToggle(sInt mode);   // toggle stack / tree / custom / wiki
  void StartCustomEditor(wCustomEditor *);
  sInt WikiToggleFlag;

  void CmdNew();
  void CmdNew2();
  void CmdOpen();
  void CmdOpen2();
  void CmdOpen3();
  void CmdMerge();
  void CmdMerge2();
  void CmdOpenBackup();
  void CmdOpenBackup2();
  void CmdSaveAs();
  void CmdSave();
  void CmdSave2();
  void CmdAutoSave();
  void CmdExit();
  void CmdExit2();
  void CmdSaveQuit();
  void CmdSaveNew();
  void CmdSaveOpen();
  void CmdPanic();
  void CmdFlushCache();
  void CmdFlushCache2();
  void CmdChargeCache();
  void CmdUnblockChange();
  void CmdEditOptions();
  void CmdEditOptionsOk(sDInt ok);
  void CmdSetDefaultCamSpeed();
  void CmdDocOptions();
  void CmdDocOptionsOk(sDInt ok);
  void CmdTestOptions();
  void CmdTestOptionsOk(sDInt ok);
  void CmdShellSwitches();
  void SetShellSwitches();
  void RemScriptDefine(sDInt n);
  void AddScriptDefine();
  void CmdShellSwitchesOk();
  void CmdShellSwitchChange();

  void CmdBpmChange();
  void CmdAddBpm(sDInt i);
  void CmdRemBpm(sDInt i);
  void CmdAddHiddenPart(sDInt i);
  void CmdRemHiddenPart(sDInt i);
  void CmdUpdateVarBpm();
  void CmdChangeTimeline();
  void CmdMusicDialog();
  void CmdReloadMusic();
  void CmdMusicLength();
  void CmdSetAmbient(sDInt);
  void CmdUpdateTheme();
  void CmdEditTheme();

  void CmdAddInclude();
  void CmdRemInclude(sDInt);
  void CmdCreateInclude(sDInt);
  void CmdLoadInclude(sDInt);
  void CmdLoadInclude2(sDInt);

  void CmdLoadConfig();
  void CmdSaveConfig();
  void CmdUpdateMultisample();
  void CmdSwitchScreen(sDInt);

  void CmdStartTime();
  void CmdLoopTime();
  void CmdRestartTime();

  void CmdRenameAll();
  void CmdRenameAll2();
  void CmdAppChangeFromCustom();
  void CmdScrollToArrayLine(sDInt n);
  void CmdReferences();
  void CmdFindClass();
  void CmdFindClass2();
  void CmdFindStore();
  void CmdFindStore2();
  void CmdFindString();
  void CmdFindString2();
  void CmdFindObsolete();
  void CmdKillClips();
  void CmdSelUnconnected();
  void CmdDelUnconnected();
  void CmdDelUnconnected2();
  void CmdGotoShortcut(sDInt num);
  void CmdGotoRoot();
  void CmdBrowser();
  void CmdBrowser2(sDInt opo);

  class sPainter *Painter;
  wType *AddType;
  sBool AddTypeSec;
  sArray<StoreBrowserItem *> StoreTree;
  void UpdateStoreTree(wType *filter);
  void StartStoreBrowser(const sMessage &cmd,const sChar *oldname,wType *filter);
  void PopupText(const sChar *);

  const sChar *WikiPath;
  const sChar *UnitTestPath;
  const sChar *WikiCheckout;

  void MakePresetPath(const sStringDesc &path,wOp *op,const sChar *name);
  void ExtractPresetName(const sStringDesc &name,const sChar *path);
  void LoadOpPreset(wOp *op,const sChar *name);
  void SaveOpPreset(wOp *op,const sChar *name);
};

extern MainWindow *App;


/****************************************************************************/

class WinPageList : public sSingleTreeWindow<wPage>
{
  wPage *SetIncludePage;
  void CmdSetInclude(sDInt);
public:
  sCLASSNAME(WinPageList);
  WinPageList();
  ~WinPageList();
  void Tag();
  sBool OnPaintField(const sRect &client,sListWindow2Field *field,sObject *obj,sInt line,sInt select);
  sBool OnEditField(const sRect &client,sListWindow2Field *field,sObject *obj,sInt line);
  void InitWire(const sChar *name);
  sBool OnCommand(sInt cmd);

  void CmdAdd(sDInt);
  void CmdRename();
  void CmdSetPage();
  void CmdDeleteSave();
  void CmdDelete2(sDInt);
  void CmdCut();
  void CmdCopy();
  void CmdPaste();
};


class WinClipOpsList : public sSingleListWindow<wOp>
{
public:
  sCLASSNAME_NONEW(WinClipOpsList);
  WinClipOpsList(sArray<wOp *> *);
  void InitWire(const sChar *name);
  sBool OnPaintField(const sRect &client,sListWindow2Field *field,sObject *,sInt line,sInt select);
};

class WinCurveOpsList : public sSingleListWindow<wOp>
{
public:
  sCLASSNAME_NONEW(WinCurveOpsList);
  WinCurveOpsList(sArray<wOp *> *);
  void InitWire(const sChar *name);
  sBool OnPaintField(const sRect &client,sListWindow2Field *field,sObject *,sInt line,sInt select);
};

/****************************************************************************/

class WinTreeView : public sMultiTreeWindow<wTreeOp>
{
  sBool HasBlink;
  sBool BlinkToggle;

public:
  sCLASSNAME(WinTreeView);
  WinTreeView();
  ~WinTreeView();
  void Tag();
  void InitWire(const sChar *name);
  void SetPage(wPage *);
  void GotoOp(wOp *op,sBool blink = 1);

  sArray<wTreeOp *> *Tree;
  wPage *Page;

  sInt OnBackColor(sInt line,sInt select,sObject *obj);
  void OnPaint2D();
  sBool OnPaintField(const sRect &client,sListWindow2Field *field,sObject *obj,sInt line,sInt select);

  void CmdAdd(sDInt sec);
  void CmdAddType(sDInt type);
  void CmdAddConversion();
  void CmdAdd2(sDInt n);
  void CmdChange();
  void CmdShow(sDInt n);
  void CmdShowRoot(sDInt n);
  void CmdEdit(sDInt n);

  void CmdCut();
  void CmdCopy();
  void CmdPaste();

  void CmdBlinkTimer();
};

/****************************************************************************/

class WinPara : public sWireGridFrame
{
  sString<sMAXPATH> FileDialogBuffer;
  sInt CurrentLinkId;
  const sChar *CurrentDisplayedError;
  sStatusBorder *Status;

  wOpData Undo;
  wOpData Redo;
  sButtonControl *UndoRedoButton;
  sButtonControl *ScriptButton;
  class sTextWindow *ScriptShowControl;
  sBool UndoIsRedo;
  sInt WikiToggleFlag;
  sInt PresetKeepArrayFlag;

  sArray<sDirEntry> PresetDir;
  wDocName PresetName;

public:
  sCLASSNAME(WinPara);
  WinPara();
  ~WinPara();
  void Tag();
  wOp *Op;

  void OnBeforePaint();
  void SetOp(wOp *);
  void AddArray(sDInt pos);
  void RemArray(sDInt pos);
  void RemArrayGroup(sDInt pos);
  void CmdArrayClearAll();
  void UpdateUndoRedoButton();

  void FileLoadDialog(sDInt offset);
  void FileReload();
  void FileSaveDialog(sDInt offset);
  void FileInDialogScript(sDInt offset);
  void FileOutDialogScript(sDInt offset);
  void FileDialogScript(sDInt offset,sInt mode);
  void CmdAction(sDInt code);
  void CmdCustomEd();
  void CmdLinkPopup(sDInt offset);
  void CmdLinkPopup2(sDInt offset);
  void CmdLinkGoto(sDInt offset);
  void CmdLinkAnim(sDInt offset);

  void CmdConnectLayout();
  void CmdConnect();
  void CmdLayout();
  void CmdChanged();
  void CmdChangeScript();
  void CmdLinkBrowser(sDInt n);
  void CmdLinkBrowser2();
  void CmdDocChange();

  void CmdUndoRedo();
  void CmdWiki();

  void CmdPreset();
  void CmdPresetAdd();
  void CmdPresetAdd2();
  void CmdPresetDelete();
  void CmdPresetDelete2(sDInt);
  void CmdPresetOver();
  void CmdPresetOver2(sDInt);
  void CmdPresetDefault();
  void CmdPresetSet(sDInt i);
  void LoadPreset(const sChar *filename);
};

inline sMessage sOpActionMessage(sDInt id) { return sMessage(App->ParaWin[0],&WinPara::CmdAction,id); }

/****************************************************************************/

class WinStoreBrowser : public sSingleTreeWindow<StoreBrowserItem>
{
public:
  sCLASSNAME(WinStoreBrowser);
  WinStoreBrowser();
  ~WinStoreBrowser();
  void Tag();
  void InitWire(const sChar *name);
  void Goto(const sChar *name);

  void CmdSelectOp();
  void CmdSelectDone();
  void CmdSelectClose();
  sMessage OpMsg;
  wDocName StoreName;
};

/****************************************************************************/

class WinOpList : public sSingleListWindow<wOp>
{
  sArray<wOp *> Ops;
  sWindow *Header;
public:
  sCLASSNAME_NONEW(WinOpRefs);
  WinOpList();
  ~WinOpList();
  void Tag();
  void InitWire(const sChar *name);

  void References(const sChar *name);
  void FindClass(const sChar *name);
  void FindString(const sChar *name);
  void FindObsolete();
  void OnCalcSize();
  sBool OnPaintField(const sRect &client,sListWindow2Field *field,sObject *obj,sInt line,sInt select);

  void CmdSelectClose();
  void CmdSelect();
};

/****************************************************************************/

class WinStack : public sWireClientWindow
{
  wPage *Page;
  wStackOp *LastOp;
  wStackOp *HoverOp;

  sInt HasBlink;
  sInt BlinkToggle;
	sInt MaxBlinkTimer;

  sInt OpXS;                      // size of operator in pixels
  sInt OpYS;
  sInt CursorX;
  sInt CursorY;

  sRect DragRect;
  sBool DragRectMode;
  sInt DragMoveX;
  sInt DragMoveY;
  sInt DragMoveW;
  sInt DragMoveH;
  sInt DragMoveMode;
  sInt DragBirdX;
  sInt DragBirdY;
  sInt ShowArrows; 

  sBool BirdseyeEnable;
  sRect BirdsOuter;
  sRect BirdsInner;

  void MakeRect(sRect &r,wStackOp *op,sInt dx=0,sInt dy=0,sInt dw=0,sInt dh=0);
  const sChar *MakeOpName(wStackOp *op);
  wStackOp *GetHit(sInt mx,sInt my);
  void CursorStatus();

public:
  sCLASSNAME(WinStack);
  WinStack();
  ~WinStack();
  void Tag();
  void InitWire(const sChar *name);
  void SetPage(wPage *page,sBool nogotopush=0);
  wPage *GetPage() { return Page; }
  void SetLastOp(wOp *);

  sBool OnCheckHit(const sWindowDrag &dd);
  void OnPaint2D();
  void OnDrag(const sWindowDrag &dd);
  void OnCalcSize();
  void CmdBlinkTimer();

  void ToolBirdseye(sDInt enable);
  void ToolArrows(sDInt enable);

  void GotoOp(wOp *op,sBool blink = 1); // should be on active page

  void DragSelect(const sWindowDrag &dd,sDInt mode);
  void DragFrame(const sWindowDrag &dd,sDInt mode);
  void DragMove(const sWindowDrag &dd,sDInt mode);  
  void DragBirdScroll(const sWindowDrag &dd); 

  void CmdCursor(sDInt udlr);
  void CmdDel();
  void CmdAddPopup(sDInt sec);
  void CmdAddType(sDInt type);
  void CmdAddConversion();
  void CmdAdd(sDInt nr);
  void CmdCut();
  void CmdCopy();
  void CmdPaste();
  void CmdSelectAll();
  void CmdSelectTree();
  void CmdShowOp(sDInt n);
  void CmdShowRoot(sDInt win);
  void CmdGoto();
  void CmdBrowser();
  void CmdBrowser2(sDInt);
  void CmdExchange();
  void CmdBypass();
  void CmdHide();
  void CmdUnCache();
};


/****************************************************************************/

class WinTimeline : public sWireClientWindow
{
  sInt Height;
  sInt DragLoop0;
  sInt DragLoop1;
public:
  WinTimeline();
  ~WinTimeline();
  void InitWire(const sChar *name);
  void OnPaint2D();
  void OnCalcSize();
  void OnLayout();
  void UpdateSample(sInt sample);
  sInt PosToBeat(sInt x);
  sInt BeatToPos(sInt beat);

  void ScratchDragStart();
  sBool ScratchDrag(sF32 t);
  void ScratchDragStop();

  sInt Start;
  sInt End;
  sInt LoopStart;
  sInt LoopEnd;
  sInt LoopEnable;
  sInt Time;                      // time in beats
  sInt TimeMin;                   // when jumping to a time, set this to. Otherwise the time will "jump back" due to latency.
  sInt Pause;
  sBool ScratchMode;
  sInt ScratchStart;
  
  sMessage ScratchMsg;
  sMessage ChangeMsg;

  void SetRange(sInt start,sInt end);

  void DragScratch(const sWindowDrag &dd);
  void DragScratchIndirect(const sWindowDrag &dd,sDInt speed);
  void DragLoop(const sWindowDrag &dd);
  void CmdToogleLoop();
};

/****************************************************************************/

class WinCustom : public sWindow
{
  class wCustomEditor *Child;
public:
  WinCustom();
  ~WinCustom();
  void Tag();
  void Set(class wCustomEditor *);

  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();
  sBool OnKey(sU32 key);
  void OnDrag(const sWindowDrag &dd);
  void OnTime(sInt time);
  void ChangeOp();
};

/****************************************************************************/

#endif // FILE_WERKKZEUG4_GUI_HPP


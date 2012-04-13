// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once
#include "werkkzeug.hpp"

/****************************************************************************/

class WinTimeline2 : public sGuiWindow
{
  sInt BeatZoom;                  // pixel = (Beat * BeatZoom)>>32
  sInt BeatQuant;                 // 0x10000 = quant to beat
  sInt DragStartX,DragStartY,DragStartZ;
  sInt DragMoveX,DragMoveY,DragMoveZ;
  sRect DragFrame;
  sInt DragKey;
  sInt DragMode;
  sInt Level;
  sInt LastLine;
  sInt LastTime;

  sList<WerkEvent2> *Clipboard;
public:
  WerkkzeugApp *App;

  WinTimeline2();
  ~WinTimeline2();
  void Tag();
  sBool MakeRect(WerkEvent2 *top,sRect &r);
  sBool MakeRect(sInt line,sInt start,sInt ende,sRect &r);
  sBool MakeRectMove(WerkEvent2 *top,sRect &r);
  void ClipCut();
  void ClipDeselect();
  void ClipCopy();
  void ClipPaste();
  void ClipDelete();
  void Quant();


  void OnPaint();
  void OnKey(sU32 key);
  void OnDrag(sDragData &dd);
  sBool OnCommand(sU32 cmd);
  void OnCalcSize();

  void EditEvent(WerkEvent2 *we);
};

/****************************************************************************/


class WinEventPara2 : public sDummyFrame
{
  sInt StartCourse;
  sInt StartFine;
  sInt LengthCourse;
  sInt LengthFine;
  sU32 AddAnim_Id;
  sInt AddAnim_Type;
  const sF32 *AddAnim_Ptr;
  const sChar *AddAnim_SceneName;
//  sArray<WinEventParaInfo2> ParaInfo;

  sInt Line;
  const sChar *GroupName;
  void HandleGroup();
  void Label(const sChar *);
  void AddAnimGui(sInt animtype,sInt id,sInt scripttype,const sF32 *data,const sChar *name);
public:
  WerkkzeugApp *App;
  WerkEvent2 *Event;
  sGridFrame *Grid;

  WinEventPara2();
  ~WinEventPara2();
  void Tag();

  sBool OnCommand(sU32 cmd);
  void SetEvent(WerkEvent2 *);
  void ControlToEvent();
  void EventToControl();

  void Group(const sChar *name);
};

/****************************************************************************/

class WinEventScript2 : public sDummyFrame
{
  sTextControl *Text;
  sText *DummyScript;
  sText *Output;
  sText *Disassembly;

  sInt Mode;
  sInt RecompileDelay;

  void SetMode(sInt mode);
  void ListSymbols();
public:
  WinEventScript2();
  ~WinEventScript2();
  void Tag();
  sBool OnCommand(sU32 cmd);
  void OnFrame();
  void SetEvent(WerkEvent2 *event);

  WerkkzeugApp *App;
  WerkEvent2 *Event;
};

/****************************************************************************/

// struct sMatrixEdit is in werkkzeug.hpp

class sMatrixEditor : public sDummyFrame
{
  sGridFrame *Grid;
  sMatrixEdit *MatEd;
  sMatrix *Mat;
  const sChar *Name;
  sInt RelayoutCmd;

  void ReadMatrix();
  void WriteMatrix();
  sInt AddControl();
public:
  sMatrixEditor();
  ~sMatrixEditor();
  sInt Init(sMatrixEdit *,sMatrix *,const sChar *name,sInt relayoutcmd);

  sBool OnCommand(sU32 cmd);
};

/****************************************************************************/

class WinSceneList : public sGuiWindow
{
  sListControl *DocList;
public:
  WinSceneList();
  ~WinSceneList();
  void Tag();
  void OnCalcSize();
  void OnLayout();
  sBool OnCommand(sU32);
  void OnPaint();
  sBool OnShortcut(sU32 key);

  void UpdateList();

  WerkkzeugApp *App;
  void SetScene(WerkScene2 *scene);
};

class WinSceneNode : public sGuiWindow
{
  sListControl *DocList;
  WerkScene2 *Scene;
public:
  WinSceneNode();
  ~WinSceneNode();
  void Tag();
  void OnCalcSize();
  void OnLayout();
  sBool OnCommand(sU32);
  void OnPaint();
  sBool OnShortcut(sU32 key);

  void UpdateList();
  void SetScene(WerkScene2 *scene);
  WerkkzeugApp *App;
};

class WinSceneAnim : public sGuiWindow
{
public:
  WinSceneAnim();
  ~WinSceneAnim();
  void OnPaint();
  WerkkzeugApp *App;
};

class WinScenePara : public sDummyFrame
{
  sGridFrame *Grid;
  WerkSceneNode2 *Node;
  WerkScene2 *Scene;
public:
  WinScenePara();
  ~WinScenePara();
  void Tag();
  sBool OnCommand(sU32 cmd);

  void SetNode(WerkScene2 *scene,WerkSceneNode2 *node);
  WerkkzeugApp *App;
};

/****************************************************************************/

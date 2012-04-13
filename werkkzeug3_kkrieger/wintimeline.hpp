// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once
#include "werkkzeug.hpp"

/****************************************************************************/
/****************************************************************************/

class WinTimeline : public sGuiWindow
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

  sList<WerkEvent> *Clipboard;
  void MoveSelected(sInt dx,sInt dy,sInt dragmode);
public:
  WerkkzeugApp *App;
  WerkEvent *Current;

  WinTimeline();
  ~WinTimeline();
  void Tag();
  sBool MakeRect(WerkEvent *top,sRect &r);
  sBool MakeRectMove(WerkEvent *top,sRect &r);
  void ClipCut();
  void ClipDeselect();
  void ClipCopy();
  void ClipPaste();
  void ClipDelete();
  void Quant();
  void MarkTime();


  void OnPaint();
  void OnKey(sU32 key);
  void OnDrag(sDragData &dd);
  sBool OnCommand(sU32 cmd);
  void OnCalcSize();

  void EditEvent(WerkEvent *we);
};

/****************************************************************************/

class WinEvent : public sDummyFrame
{
  sInt Line;
  WerkPage *CurrentPage;
  sInt StartCourse;
  sInt StartFine;
  sInt LengthCourse;
  sInt LengthFine;
public:
  class WerkkzeugApp *App;
  WinEvent();
  void Reset();
  void Tag();
  sGridFrame *Grid;
  WerkEvent *Event;

  void EventToControl();
  void ControlToEvent();
  void SetEvent(WerkEvent *e);
  sBool OnCommand(sU32 cmd);
  void Label(sChar *label);
  void Space() {Line++;}
  void AddBox(sU32 cmd,sInt pos,sInt offset,sChar *name);
};

/****************************************************************************/

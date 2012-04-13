// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once
#include "werkkzeug.hpp"

/****************************************************************************/

class WinSplineList : public sGuiWindow
{
  sInt Height;
  sInt DragX,DragY;
  sInt Line;

  WerkSpline *Clipboard;
public:
  class WerkkzeugApp *App;

  WinSplineList();
  ~WinSplineList();
  void Tag();

  void OnCalcSize();
  void OnPaint();
  void OnKey(sU32 key);
  void OnDrag(sDragData &dd);
  sBool OnCommand(sU32 cmd);  
  void ToggleChannel(sInt mask);
  sInt GetLine() { return Line; }
  WerkSpline *GetSpline();
  void SetSpline(WerkSpline *);
  sList<WerkSpline> *GetSplineList();
};

/****************************************************************************/

class WinSpline : public sGuiWindow
{
  sInt DragMode;
  sF32 ValMid,ValAmp;
  sInt TimeZoom;
  sF32 LastTime;
  sInt DragKey;

  sF32 DragStartX;
  sF32 DragStartY;
  sF32 DragEndX;
  sF32 DragEndY;
  sF32 DragStartTime;
  sInt DragStartPos;

  void Time2Pos(sF32 &x,sF32 &y);
  void Pos2Time(sF32 &x,sF32 &y);
  void DPos2Time(sF32 &x,sF32 &y);
  sBool GetTime(sF32 &timef);
  void SetTime(sF32 timef);
  void MakeDragRect(sRect &r);
  void MoveSelected(sF32 dx,sF32 dy);
public:
  class WerkkzeugApp *App;

  WinSpline();
  ~WinSpline();
  void Tag();

  void OnPaint();
  void OnKey(sU32 key);
  void OnDrag(sDragData &dd);
  sBool OnCommand(sU32 cmd);

  void GetEvent(sInt &start,sInt &ende);
  void ClipDeleteKey();
  void Align(sU32 mask);
  void AllOfTime();
  void AddKeys(sF32 time);
  void SelectAll();
  void LinkEdit(sF32 *srt,sBool spline_to_srt);
};

/****************************************************************************/

class WinSplinePara : public sDummyFrame
{
  sInt Line;
  sInt NewCount;
public:
  class WerkkzeugApp *App;
  WinSplinePara();
  void OnKey(sU32 key);
  void Reset();
  void Tag();
  sGridFrame *Grid;
  WerkSpline *Spline;

  void SetSpline(WerkSpline *s);
  void Update() { SetSpline(Spline); }
  sBool OnCommand(sU32 cmd);
  void Label(sChar *label);
  void Space() {Line++;}
  void AddBox(sU32 cmd,sInt pos,sInt offset,sChar *name);
};

/****************************************************************************/

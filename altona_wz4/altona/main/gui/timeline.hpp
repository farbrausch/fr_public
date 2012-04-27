/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#pragma once
#include "gui/gui.hpp"
#include "util/musicplayer.hpp"

/****************************************************************************/
/****************************************************************************/

// timeline measures time in an artifical unit (here called "beat").
// it is used to interface a timeline and a timetable window with an
// application. it allows looping.
// it is assummend the the timeline window regulary calls OnFrame()
// this interface is purely virtual because synchronisation with a
// music replay may require a locking mechanism

class sTimelineInterface
{
public:
  // maintainance
  virtual ~sTimelineInterface() {}
  virtual void OnFrame()=0;         // update the timeline
  virtual void AddNotify(sWindow *)=0;

  // initialisation
  virtual void SetTimeline(sInt end,sInt speed)=0;
  virtual void SetLoop(sInt start,sInt end)=0;

  // getters
  virtual sInt GetEnd()=0;
  virtual sInt GetSpeed()=0;
  virtual sInt GetBeat()=0;
  virtual void GetLoop(sInt &start,sInt &end)=0;
  virtual sBool GetPlaying()=0;
  virtual sBool GetLooping()=0;
  virtual sBool GetScratching()=0;

  // playing
  virtual void EnableLoop(sBool loop)=0;
  virtual void EnablePlaying(sBool play)=0;
  // "Scratching" means that someone is scratching the timeline right now, *not* that
  // scratching is (dis)allowed! That depends only on whether the relevant messages are
  // wired (or not).
  virtual void EnableScratching(sBool scratch)=0;
  virtual void SeekBeat(sInt beat)=0;
};


class sTimerTimeline : public sTimelineInterface
{
  sInt BeatTime;                  // 16:16 current time
  sInt BeatEnd;                   // 16:16 end of timeline
  sInt LoopStart;                 // 16:16 start of loop
  sInt LoopEnd;                   // 16:16 end of loop. the loop is set if start<end
  sInt Speed;                     // 16:16 time_beat = time_seconds * Speed / 0x10000
  sBool LoopEnable;               // activate looping.
  sBool Playing;                  // handled by Start()/Stop()
  sBool Scratching;               // special mode: like play, but with time frozen

  sInt LastTime;
  sInt Time;
public:
  sTimerTimeline();
  ~sTimerTimeline();
  void OnFrame();
  void AddNotify(sWindow *);
  void SetTimeline(sInt end,sInt speed);
  void SetLoop(sInt start,sInt end);
  sInt GetEnd();
  sInt GetBeat();
  sInt GetSpeed();
  void GetLoop(sInt &start,sInt &end);
  sBool GetPlaying();
  sBool GetLooping();
  sBool GetScratching();
  void EnableLoop(sBool loop);
  void EnablePlaying(sBool play);
  void EnableScratching(sBool scratch);
  void SeekBeat(sInt beat);
};

class sMusicTimeline : public sTimelineInterface
{
  sInt BeatTime;                  // 16:16 current time
  sInt BeatEnd;                   // 16:16 end of timeline
  sInt LoopStart;                 // 16:16 start of loop
  sInt LoopEnd;                   // 16:16 end of loop. the loop is set if start<end
  sInt Speed;                     // 16:16 time_beat = time_seconds * Speed / 0x10000
  sBool LoopEnable;               // activate looping.
  sBool Playing;                  // handled by Start()/Stop()
  sBool Scratching;               // special mode: like play, but with time frozen

  sInt LastTime;
  sInt Time;

  sMusicPlayer *Music;
  sInt TotalSamples;
  sInt MusicSamples;


  friend void sMusicTimelineSoundHandler(sS16 *samples,sInt count);
  void SoundHandler(sS16 *samples,sInt count);
public:
  sMusicTimeline(sMusicPlayer *music);
  ~sMusicTimeline();
  void OnFrame();
  void AddNotify(sWindow *);
  void SetTimeline(sInt end,sInt speed);
  void SetLoop(sInt start,sInt end);
  sInt GetEnd();
  sInt GetBeat();
  sInt GetSpeed();
  void GetLoop(sInt &start,sInt &end);
  sBool GetPlaying();
  sBool GetLooping();
  sBool GetScratching();
  void EnableLoop(sBool loop);
  void EnablePlaying(sBool play);
  void EnableScratching(sBool scratch);
  void SeekBeat(sInt beat);
};

/****************************************************************************/

// a timetable arranges events along a timeline. 

class sTimeTableClip : public sObject
{
public:
  sCLASSNAME(sTimeTableClip);
  sTimeTableClip();
  sTimeTableClip(sInt start,sInt end,sInt line,const sChar *name);

  void AddNotify(sWindow *);

  sString<64> Name;
  sInt Start;
  sInt Length;
  sInt Line;
  sInt Selected;

  sInt DragStartX;
  sInt DragStartY;
};

class sTimetable : public sObject
{
public:
  sCLASSNAME(sTimetable);
  sTimetable();
  void Tag();
  
  sInt MaxLines;
  sInt LinesUsed;

  sArray<sTimeTableClip *> Clips;

  void Sort();

  // customization
  virtual sTimeTableClip *NewEntry();
  virtual sTimeTableClip *Duplicate(sTimeTableClip *source);
};

/****************************************************************************/
/****************************************************************************/

class sWinTimeline : public sWireClientWindow
{
protected:
  sInt Height;
  sTimelineInterface *Timeline;
  sBool DecimalTimebase; // false: use powers of 2 (for music); true: use powers of 10 (seconds/frames etc.)

public:
  sCLASSNAME_NONEW(sWinTimeline);
  sWinTimeline(sTimelineInterface *);
  void InitWire(const sChar *name);

  void OnCalcSize();
  void OnLayout();
  void OnPaint2D();

  void CmdStart();
  void CmdPause();
  void CmdLoop();
  void DragScratch(const sWindowDrag &dd);
  void DragMark(const sWindowDrag &dd);

  void SetDecimalTimebase(sBool enable);
};

/****************************************************************************/

enum DragLockFlag
{
  DLF_NONE=0,
  DLF_HORIZONTAL=1,
  DLF_VERTICAL=2,
};

class sWinTimetable : public sWireClientWindow
{
protected:
  sTimetable *Timetable;
  sTimelineInterface *Timeline;
  sInt Height;
  sInt Zoom;
  sRect DragRect;
  sInt DragMode;
  sInt DragStartX;
  sInt DragZoomX;
  sInt DragCenterX;
  DragLockFlag DragLockFlags;

  void EntryToScreen(const sTimeTableClip *ent,sRect &rect);
public:
  sCLASSNAME_NONEW(sWinTimetable);
  sWinTimetable(sTimetable *tt,sTimelineInterface *tl);
  ~sWinTimetable();
  void Tag();
  void InitWire(const sChar *name);
  void SetDragLockFlags(DragLockFlag flags);

  sBool OnCheckHit(const sWindowDrag &dd);
  void OnCalcSize();
  void OnPaint2D();

  sMessage SelectMsg;

  void CmdDelete();
  void CmdAdd();
  void DragScratch(const sWindowDrag &dd);
  void DragMove(const sWindowDrag &dd);
  void DragDuplicate(const sWindowDrag &dd);
  void DragWidth(const sWindowDrag &dd);
  void DragFrame(const sWindowDrag &dd);
  void DragZoom(const sWindowDrag &dd);
};

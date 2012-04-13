// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __WINTIME_HPP__
#define __WINTIME_HPP__

#include "_util.hpp"
#include "_gui.hpp"
#include "apptool.hpp"
#include "apptext.hpp"
#include "genmisc.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Timeline                                                           ***/
/***                                                                      ***/
/****************************************************************************/

class TimeOp : public sObject
{
public:
  TimeOp();
  ~TimeOp();
  void Clear();
  void Tag();
  sU32 GetClass() {return sCID_TOOL_TIMEOP;}
  sBool Inside(TimeOp *op);
  void Copy(sObject *o);


  sInt x0,x1;                     // time beat 16:16
  sInt y0;
  sInt Type;                      // TOT_???
  sInt Select;                    // current selection

  sInt Velo;                      // event parameter
  sInt Mod;
  sF32 SRT[9];
  sChar Text[64];

//  sVector S,R,T;

  class AnimDoc *Anim;            // link to animation
  sInt Temp;                      // used as temporary mark
};

#define TOT_CAMERA      1
#define TOT_MESH        2
#define TOT_EVENT       3

/****************************************************************************/
/****************************************************************************/

class TimeDoc : public ToolDoc
{
public:
  TimeDoc();
  ~TimeDoc();
  void Clear();
  void Tag();
  sU32 GetClass() {return sCID_TOOL_TIMEDOC;}

  sBool Write(sU32 *&);
  sBool Read(sU32 *&);

  sList<TimeOp> *Ops;
  sInt BeatMax;
  void SortOps();
};

/****************************************************************************/

class TimeWin : public ToolWindow
{
  TimeDoc *Doc;
  TimeOp *Current;
  sInt BeatZoom;                  // pixel = (Beat * BeatZoom)>>32
  sInt BeatQuant;                 // 0x10000 = quant to beat
  sInt DragStartX,DragStartY;
  sInt DragMoveX,DragMoveY,DragMoveZ;
  sRect DragFrame;
  sInt DragKey;
  sInt StickyKey;
  sInt DragMode;
  sInt Level;
  sInt LastLine;
  sInt LastTime;
public:
  TimeWin();
  ~TimeWin();
  void Tag();
  sU32 GetClass() { return sCID_TOOL_TIMEWIN; }
  void SetDoc(ToolDoc *doc);
  sBool MakeRect(TimeOp *top,sRect &r);
  sBool MakeRectMove(TimeOp *top,sRect &r);
  void ClipCut();
  void ClipCopy();
  void ClipPaste();
  void ClipDelete();
  void Quant();


  void OnPaint();
  void OnKey(sU32 key);
  void OnDrag(sDragData &dd);
  sBool OnCommand(sU32 cmd);
};

/****************************************************************************/
/***                                                                      ***/
/***   Animation                                                          ***/
/***                                                                      ***/
/****************************************************************************/

struct AnimKey
{ 
  sInt Time;                      // time of key 0..0x10000
  sInt Channel;                   // TimeAnim
  sInt Value[4];                  // value 16:16
  sBool Select[4];                // selected ?

  void Init();
};

struct AnimChannel
{
  sChar Name[32];                 // target name
  sInt Offset;                    // offset to animate
  sInt Count;                     // number of dimensions
  sInt Mode;                      // select input (time, velo, modulation,...)
  sInt Select[4];                 // select if spline is shown
  sObject *Target;                // object to animate
  sInt Temp;                      // numerating splines 
  sInt Tension;                   // spline parameters
  sInt Continuity;
  sInt Bias;

  void Init();
};


class AnimDoc : public ToolDoc
{
//  AnimDoc *Doc;
//  sInt SaveId;
public:
  AnimDoc();
  ~AnimDoc();
  void Tag();
  sU32 GetClass() { return sCID_TOOL_ANIMDOC; }
  void Update();

  sArray<AnimChannel> Channels;
  sArray<AnimKey> Keys;
  sInt Temp;
  sChar RootName[32];
  ToolObject *Root;                 // Object Pointer
  sInt PaintRoot;

  void AddChannel();
  void RemChannel(sInt channel);

  void AddKey(sInt time);
};

class AnimWin : public ToolWindow
{
  TimeOp *TOp;
  AnimDoc *Doc;
  sInt DragKey;
  sInt StickyKey;
  sInt DragMode;
  sInt ValMid,ValAmp;
  sInt TimeZoom;
  sInt LastTime;
  GenSpline *Spline;

  sInt DragStartX,DragStartY;

  void Time2Pos(sInt &x,sInt &y);
  void Pos2Time(sInt &x,sInt &y);
  void DPos2Time(sInt &x,sInt &y);
public:
  AnimWin();
  ~AnimWin();
  void Tag();
  sU32 GetClass() { return sCID_TOOL_ANIMWIN; }
  void SetDoc(ToolDoc *doc);
  void SetOp(TimeOp *top);

  void OnPaint();
  void OnKey(sU32 key);
  void OnDrag(sDragData &dd);
  sBool OnCommand(sU32 cmd);

  void ClipCopyKey();
  void ClipPasteKey();
  void ClipDeleteKey();
};

/****************************************************************************/

#endif

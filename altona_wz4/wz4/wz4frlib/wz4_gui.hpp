/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_WZ4_GUI_HPP
#define FILE_WZ4FRLIB_WZ4_GUI_HPP

#include "base/types.hpp"
#include "wz4lib/doc.hpp"
#include "wz4lib/gui.hpp"
#include "wz4frlib/wz4_demo2.hpp"

struct Wz4RenderArraySpline;
struct Wz4RenderParaSpline;

/****************************************************************************/

class sMathPaper
{
  sFRect DragStart;               // range when drag was started
public:
  sMathPaper();
  ~sMathPaper();

  sFRect Range;                   // range of values
  sRect Client;                   // please update the client rect regulary

  sInt XToS(sF32);                // to screen
  sInt YToS(sF32);
  sF32 XToV(sInt);                // to value
  sF32 YToV(sInt);

  sMessage ScrollMsg;
  void Paint2D();
  void DragScroll(const sWindowDrag &dd);
  void DragZoom(const sWindowDrag &dd,sDInt mode=3);
};

class Wz4SplineCed : public wCustomEditor
{
  wOp *Op;
  sMathPaper Grid;

  static sFRect SaveRange;
  static sBool RangeSaved;

  sInt DragMode;
  sRect DragRect;
  sInt LastTimePixel;
  sInt Timeline0;
  sInt Timeline1;

  sInt SplitRequestX;
  sInt SplitRequestY;
  sBool SplitRequest;
  sF32 SplitResultTime;
  sInt SplitResultCurve;

  sInt ToolMode;
  sInt ChannelMask;
  sInt ChannelMax;

  sRect Client;
  enum Consts
  {
    DM_SCROLL = 1,
    DM_ZOOM,
    DM_HANDLE,
    DM_HANDLEX,
    DM_HANDLEY,
    DM_SCALE,
    DM_SCALEX,
    DM_SCALEY,
    DM_FRAME,
    DM_FRAMEADD,
    DM_INSERT,
    DM_COPYCAM,
    DM_SCRATCHSLOW,
    DM_SCRATCHFAST,
    DM_TIME,
    DM_SCISSORVIS,
    DM_SCISSORALL,

    CHANNELS = 8,
  };

  sU32 Color[CHANNELS];
  sArray<Wz4RenderArraySpline *> Curves[CHANNELS];
  
  struct DragItem
  {
    Wz4RenderArraySpline *Key;
    sF32 Time;
    sF32 Value;
  };
  sArray<DragItem> DragItems;
  sF32 DragCenterX;
  sF32 DragCenterY;
  sInt GrabCam(sF32 *data);

public:
  Wz4SplineCed(wOp *op);
  ~Wz4SplineCed();
  void Tag();
  void UpdateInfo();
  void MakeHandle(Wz4RenderArraySpline *key,sRect &r);
  Wz4RenderArraySpline *Hit(sInt mx,sInt my,sInt &curve);
  void Sort();
  void LineSplit(sInt x0,sInt y0,sInt x1,sInt y1,sInt curve);

  void OnCalcSize(sInt &xs,sInt &ys);
  void OnLayout(const sRect &Client);
  void OnPaint2D(const sRect &Client);
  sBool OnKey(sU32 key);
  void OnDrag(const sWindowDrag &dd,const sRect &Client);
  void OnChangeOp();
  void OnTime(sInt time);

  void CmdPopup();
  void CmdScroll();
  void CmdReset();
  void CmdFrame();
  void CmdDelete();
  void CmdExit();
  void CmdSelectSameTime();
  void CmdQuantizeTime();
  void CmdZeroValue();
  void CmdAlignTime();
  void CmdToggleChannel(sDInt i);
  void CmdAllChannels(sDInt i);
  void CmdUpdateCam();
  void CmdTimeFromKey();
  void CmdReverseTime();

  void DragScroll(const sWindowDrag &dd);
  void DragZoom(const sWindowDrag &dd);
  void DragHandle(const sWindowDrag &dd,sDInt axis=3);
  void DragScale(const sWindowDrag &dd,sDInt axis=3);
  void DragFrame(const sWindowDrag &dd,sDInt mode=0);
  void DragInsert(const sWindowDrag &dd);
  void DragCopyCam(const sWindowDrag &dd);
  void DragTime(const sWindowDrag &dd);
  void DragScissor(const sWindowDrag &dd,sDInt mode);
};

/****************************************************************************/
/****************************************************************************/

class Wz4TimelineCed : public wCustomEditor
{
  sRect Inner;
  sRect Client;
  sInt Height;

  sInt DragMode;
  sInt ToolMode;
  sRect DragRect;

  static sInt Time0;
  static sInt Time1;
  static sInt ScrollY;

  sInt DragStart0;
  sInt DragStart1;
  sInt DragStartY;
  sInt LastTimePixel;

  enum Consts
  {
    DM_SCROLL = 1,
    DM_ZOOM,
    DM_HANDLE,
    DM_HANDLELEN,
    DM_FRAME,
    DM_FRAMEADD,
    DM_SCRATCHSLOW,
    DM_SCRATCHFAST,
    DM_TIME,

    DT_SCRATCH = 1,
  };

  struct Clip
  {
    wOp *Op;
    sInt Start;
    sInt Length;
    sInt Line;
    sInt Select;
    sInt Index;
    sInt Enable;
    sInt Color;

    sInt DragStart;
    sInt DragLength;
    sInt DragLine;

    sPoolString Name;
    sRect Rect;
  };
  sArray<Clip> Clips;

public:
  Wz4TimelineCed(wOp *op);
  ~Wz4TimelineCed();
  void Tag();
  void UpdateInfo();
  void MakeRect(Clip *,sRect &r);
  Clip *Hit(sInt x,sInt y);
  sInt XToS(sInt x);
  sInt XToV(sInt x);
  Clip *FindClip(wOp *op);
  void ScrollTo(Clip *clip);
  void SelectClip(Clip *clip);

  void OnPaint2D(const sRect &Client);
  void OnDrag(const sWindowDrag &dd,const sRect &client);
  void OnChangeOp();
  sBool OnKey(sU32 key);
  void OnTime(sInt time);

  void DragScroll(const sWindowDrag &dd);
  void DragZoom(const sWindowDrag &dd);
  void DragHandle(const sWindowDrag &dd,sDInt mode);
  void DragFrame(const sWindowDrag &dd,sDInt mode);
  void DragTime(const sWindowDrag &dd);

  void CmdPopup();
  void CmdReset();
  void CmdExit();
  void CmdGoto();
  void CmdMarkTime();
  void CmdEnable();
  void CmdDeleteMulticlip();
  void CmdShow();
};

/****************************************************************************/
/****************************************************************************/

class Wz4BeatCed : public wCustomEditor
{
  sRect WaveRect;
  sRect SplineRect;
  sRect BeatRect;
  sRect Client;
  sInt DragMode;
  sInt ToolMode;
  sRect DragRect;
  sInt DragStart0;
  sInt DragStart1;
  sInt DragStartY;
  sInt LastTimePixel;

  sInt WaveHeight;
  sInt WaveTime0;
  sInt WaveTime1;
  sInt WaveBS;
  sInt WaveBPS;
  sImage2D *WaveImg;

  static sInt Time0;
  static sInt Time1;
  static sInt ScrollY;

  enum Consts
  {
    DM_SCROLL = 1,
    DM_ZOOM,
    DM_FRAME,
    DM_FRAMEADD,
    DM_SCRATCHSLOW,
    DM_SCRATCHFAST,
    DM_TIME,

    DM_SELECT,
    DM_SET,
    DM_ACCENT,

    DT_SCRATCH = 1,
  };

  struct Beat
  {
    wOp *Op;
    struct Wz4RenderParaBeat *Para;
    sInt Steps;
    sRect Client;
    sInt *Lookup;

    Beat(wOp *op);
    ~Beat();

    sU8 GetStep(sInt i);
    void SetStep(sInt i,sU8 byte);
    sU8 GetStepAbs(sInt i);
    void SetStepAbs(sInt i,sU8 byte);
    void UpdateLoop();
  };
  sArray<Beat *> Beats;

  Beat *DragBeat;
  sInt DragBeatPos;
  sInt DragBeatStart;
  sInt DragSubMode;
  sInt Cursor;
  sInt CursorLength;

  class RNBeat *BeatNode;
  wOp *EditOp;
  Beat *EditBeat;

  sArray<sU8> Clipboard;

public:
  Wz4BeatCed(wOp *op);
  ~Wz4BeatCed();
  void Tag();

  sInt XToS(sInt x);
  sInt XToV(sInt x);
  void UpdateInfo();
  void UpdateSpline();

  void OnPaint2D(const sRect &Client);
  void PaintWave();
  void PaintSpline();
  void OnDrag(const sWindowDrag &dd,const sRect &client);
  void OnChangeOp();
  sBool OnKey(sU32 key);
  void OnTime(sInt time);

  void DragScroll(const sWindowDrag &dd);
  void DragZoom(const sWindowDrag &dd);
//  void DragHandle(const sWindowDrag &dd,sDInt mode);
  void DragFrame(const sWindowDrag &dd,sDInt mode);
  void DragTime(const sWindowDrag &dd);
  void DragSelect(const sWindowDrag &dd,sDInt mode);

  void CmdPopup();
  void CmdReset();
  void CmdExit();
  void CmdLoop(sDInt n);

  void CmdCut();
  void CmdCopy();
  void CmdPaste();
  void CmdCursor(sDInt dx);
  void CmdSetLevel(sDInt vel);
};

/****************************************************************************/

#endif // FILE_WZ4FRLIB_WZ4_GUI_HPP


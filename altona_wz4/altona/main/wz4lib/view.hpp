/****************************************************************************/

#ifndef FILE_WZ4LIB_VIEW_HPP
#define FILE_WZ4LIB_VIEW_HPP

#ifndef __GNUC__
#pragma once
#endif

#include "base/types2.hpp"
#include "base/graphics.hpp"
#include "gui/gui.hpp"
#include "doc.hpp"
#include "gui/3dwindow.hpp"

/****************************************************************************/

class WinView : public s3DWindow
{
  sBool WireMode;
  sBool GridMode;
  /*
  sF32 DragFX;
  sF32 DragFY;
  sF32 DragFZ;
  */
  wOp *DragRayOp;
  sInt DragRayId;
  sInt Drag3DOffsetX;
  sInt Drag3DOffsetY;
  sVector31 DragStart;
  sInt CalcSlowOps;
  sInt Letterbox;                 // 0=off, 1=best fit
  sViewport DragView;

  wObject *Calc(wOp *op);
  void SetPaintInfo(sBool render3D,sViewport *view=0,sMaterialEnv *env=0);
  sImage *Image;                  // image-cache for textures
  sImage *AlphaImage;             // image-cache for textures
  sRect LetterboxRect;

  wHandle *HitHandle(sInt mx,sInt my);

  wObject *Object;
  wObject *CamObject;

  wOp *TeleportOp;    // just used for compares, no implied ownership
  sInt TeleportHandle;
  sBool TeleportStarted;

  sTextBuffer Log;

  void CalcLetterBox(const sTargetSpec &spec);
public:
  sCLASSNAME(WinView);
  WinView();
  ~WinView();
  void Tag();
  void InitWire(const sChar *name);
  wOp *Op;
  wOp *CamOp;
  sBool FreeCamFlag;
  sBool FpsFlag;
  sBool MTMFlag;
  wPaintInfo pi;

  void StartTeleportHandle(wOp *op,sInt handleid);

  void OnBeforePaint();
  void OnPaint2D();
  void Paint(sViewport &view,const sTargetSpec &spec);
  sBool OnCheckHit(const sWindowDrag &dd);
  void OnDrag(const sWindowDrag &dd);
  
  void SetOp(wOp *);
  void SetCamOp(wOp *);
  void CmdHandles(sDInt enable);
  void CmdCalcSlowOps();
  void ToolSelect(sDInt n);
  void DragEdit(const sWindowDrag &dd,sDInt para);
  void DragSelectFrame(const sWindowDrag &dd,sDInt mode);
  void DragSelectHandle(const sWindowDrag &dd,sDInt mode);
  void DragMoveSelection(const sWindowDrag &dd);
  void DragDupSelection(const sWindowDrag &dd);
  void DragSpecial(const sWindowDrag &dd,sDInt mode);
  void CmdSpecial(sDInt mode);
  void CmdSpecialTool(sDInt mode);
  void CmdHandlesMode(sDInt mode);
  void DragTeleportHandle(const sWindowDrag &dd,sDInt mode);
  void CmdTeleportAbort(sDInt mode);
  void CmdAddHandle(sDInt before_or_after);

  // 3d helpers

  void CmdReset3D();
  void CmdColor();
  void CmdToggleWire();
  void CmdToggleGrid();
  void CmdToggleLetterbox();
  void CmdLockCam();
  void CmdFreeCam();
  void CmdFps();
  void CmdMTM();
  void CmdFocusHandle();
  void CmdMaximize2();
  void CmdDeleteHandles();
  void CmdQuantizeCamera();

  // 2d helpers

  sInt BitmapPosX;        // pxiel offset
  sInt BitmapPosY;
  sInt BitmapPosXStart;
  sInt BitmapPosYStart;
  sInt BitmapZoom;        // 16 = normal
  sInt BitmapTile;
  sInt BitmapAlpha;
  void CmdReset2D();
  void CmdTile();
  void CmdAlpha();
  void CmdZoom(sDInt inc=1);
  void DragScrollBitmap(const sWindowDrag &dd,sDInt speed=1);
  void DragZoomBitmap(const sWindowDrag &dd);
};

/****************************************************************************/

#endif // FILE_WZ4LIB_VIEW_HPP


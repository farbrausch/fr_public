/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#pragma once
#include "gui/gui.hpp"
#include "base/graphics.hpp"
#include "util/shaders.hpp"

/****************************************************************************/


class s3DWindow : public sWireClientWindow
{
  sBool Enable;
  sBool ScreenshotFlag;
  sF32 DragDist,DragRotX,DragRotY,DragRotZ;
  sMatrix34 DragPos;

  sInt QuakeTime;
  sBool QuakeMode;
  sInt QuakeMask;
  sVector30 QuakeSpeed;
protected:
  sSimpleMaterial *WireMtrl;
  sSimpleMaterial *WireMtrlNoZ;
  sGeometry *WireGeo;
  void PaintGrid();

  sRay DragRay;
  sInt OldMouseHardX;
  sInt OldMouseHardY;

#if sRENDERER==sRENDER_DX11
  sInt RTMultiLevel;
  sTexture2D *ColorRT;
  sTexture2D *DepthRT;
#endif

public:
  sCLASSNAME_NONEW(s3DWindow);
  s3DWindow();
  ~s3DWindow();
  void Tag();
  void InitWire(const sChar *name);

  void SetEnable(sBool enable);

  void OnPaint2D();
  void OnPaint3D();

  virtual void SetLight() {}
  virtual void Paint(sViewport &view) {}
  virtual void PaintWire(sViewport &view) {}
  // old style. overload old style or new style, but not both
  virtual void Paint(sViewport &view,const sTargetSpec &spec) {}
  virtual void PaintWire(sViewport &view,const sTargetSpec &spec) {}

  void Lines(sVertexBasic *vp,sInt linecount,sBool zOn=sTRUE);
  void Circle(const sVector31 &center, const sVector30 &normal, sF32 radius, sU32 color=0xffffff00, sBool zon=sTRUE, sInt segs=32);

  sRay MakeRay(sInt x, sInt y); // makes ray from position within client window
  void SetCam(const sMatrix34 &mat,sF32 zoom); // move camera to position

  sF32 Zoom;
  sVector31 Focus;
  sMatrix34 Pos;
  sF32 RotX;
  sF32 RotY;
  sF32 RotZ;
  sU32 GridColor;
  sBool Grid;
  sBool Continuous;
  sF32 GridUnit;

  sF32 SideSpeed,ForeSpeed;     // quakecam speed, default 0.000020f
  sF32 SpeedDamping;
  sInt GearShift;               // infinite speed settings through mousewheel. from -40 to +40 in sqrt(4) steps
  sInt GearShiftDisplay;        // display speed factor if(GerShiftDisplay>sGetTime());
  sF32 GearSpeed;               // current gear speed

  sViewport View;
  void PrepareView();

  void SetFocus(const sAABBox &bounds,const sVector31 &center);
  sBool OnKey(sU32 key);
  void OnDrag(const sWindowDrag &dd);
  void QuakeCam();
  void PrintGear(sPainter *p,sInt x,sInt &y);


  void CmdReset();
  void CmdResetTilt();
  void CmdGrid();
  void CmdQuakeCam();
  void CmdScreenshot();
  void CmdQuakeForwToggle(sDInt);
  void CmdQuakeBackToggle(sDInt);
  void CmdQuakeLeftToggle(sDInt);
  void CmdQuakeRightToggle(sDInt);
  void CmdQuakeUpToggle(sDInt);
  void CmdQuakeDownToggle(sDInt);
  void CmdGearShift(sDInt);
  void DragOrbit(const sWindowDrag &dd);
  void DragRotate(const sWindowDrag &dd);
  void DragMove(const sWindowDrag &dd);
  void DragZoom(const sWindowDrag &dd);
  void DragDolly(const sWindowDrag &dd);
  void DragTilt(const sWindowDrag &dd);
};


/****************************************************************************/


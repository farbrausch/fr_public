/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef HEADER_ALTONA_UTIL_CAMERA
#define HEADER_ALTONA_UTIL_CAMERA

#ifndef __GNUC__
#pragma once
#endif

#include "base/types.hpp"
#include "base/math.hpp"
#include "base/graphics.hpp"
#include "base/system.hpp"
#include "base/input2.hpp"

/****************************************************************************/
class sCamera
{
  sInt LastTime;
protected:
  enum { INPUT_X, INPUT_Y, INPUT_MBL, INPUT_MBR, INPUT_LX, INPUT_LY, INPUT_RX, INPUT_RY, };
  sInput2Scheme Scheme;
  sBool MouseControl;
  sBool JoypadControl;

public:
  sCamera();
  virtual ~sCamera();
  virtual void OnFrame(sInt delta,sViewport &vp)=0;
  virtual void OnKey(sU32 key);
  virtual void SetMouseControl(sBool mouseControl) { MouseControl = mouseControl; }
  virtual void SetJoypadControl(sBool joypadControl) { JoypadControl = joypadControl; }

  void Handle(sViewport &vp);
};

#if 0
/****************************************************************************/

class sTumbleCamera : public sCamera
{
  sInt Time;
public:
  sTumbleCamera();

  sF32 Dist;
  sVector30 Speed;
  void OnFrame(sInt delta,sViewport &vp);
};

/****************************************************************************/

class sOrbitCamera : public sCamera
{
  sVector30 Angles;
  sF32 Dist;
public:
  sOrbitCamera();
  void OnFrame(sInt delta,sViewport &vp);
  void OnDrag(const sMouseData &md);
};


/****************************************************************************/
#endif

class sFreeCamera : public sCamera
{
  sInt KeyMask;
  sF32 Moving;
  sF32 AnalogX;
  sF32 AnalogY;

  void PrintPos();
public:
  sFreeCamera();
  void OnFrame(sInt delta,sViewport &vp);
  void OnKey(sU32 key);

  void Stop();

  sF32 Speed;
  sF32 LookSpeed;
  sF32 Damping;

  sVector31 Pos;
  sF32 DirLook,DirPitch,DirRoll;
  sVector30 SpeedV;
};

/****************************************************************************/
#if 0

class sCameraXSI : public sCamera
{
  enum XSICamMode
  {
    XSI_DOLLY           = 0x0001,
    XSI_ORBIT           = 0x0002,
    XSI_TRANS_CAMPLANE  = 0x0004,
    XSI_SELECT          = 0x0008,
  };
  
  sInt KeyMaskCurrent;
  sInt KeyMaskSticky;
  

  sF32 DollyStep;
  sBool DollyActive;


  public:
    sCameraXSI();
    void OnFrame(sInt delta, sViewport &vp);
    void OnKey(sU32 key);
    void OnDrag(const sMouseData &md);
    void Init();

    
    sVector31 Target;
    sVector30 EulerRot;
    sVector30 Scale;
    sF32      Distance;
    sBool     StickyMode;
};

/****************************************************************************/
#endif
// HEADER_ALTONA_UTIL_CAMERA
#endif

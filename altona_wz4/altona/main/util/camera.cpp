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

#include "util/camera.hpp"

/****************************************************************************/

sCamera::sCamera()
{
  LastTime = sGetTime();
  Scheme.Init(10);
  sInput2Device* deviceMouse = sFindInput2Device(sINPUT2_TYPE_MOUSE, 0);
  sInput2Device* deviceJoypadXBox = sFindInput2Device(sINPUT2_TYPE_JOYPADXBOX, 0);
  Scheme.Bind(INPUT_X, deviceMouse, sINPUT2_MOUSE_RAW_X);
  Scheme.Bind(INPUT_Y, deviceMouse, sINPUT2_MOUSE_RAW_Y);
  Scheme.Bind(INPUT_MBL, deviceMouse, sINPUT2_MOUSE_LMB);
  Scheme.Bind(INPUT_MBR, deviceMouse, sINPUT2_MOUSE_RMB);
  Scheme.Bind(INPUT_LX, deviceJoypadXBox, sINPUT2_JOYPADXBOX_LEFT_X);
  Scheme.Bind(INPUT_LY, deviceJoypadXBox, sINPUT2_JOYPADXBOX_LEFT_Y);
  Scheme.Bind(INPUT_RX, deviceJoypadXBox, sINPUT2_JOYPADXBOX_RIGHT_X);
  Scheme.Bind(INPUT_RY, deviceJoypadXBox, sINPUT2_JOYPADXBOX_RIGHT_Y);
}

sCamera::~sCamera()
{
}

void sCamera::OnKey(sU32)
{
}

void sCamera::Handle(sViewport &vp)
{
  sVERIFYFALSE;
/*  sMouseData md;
  sInt time,delta;

  sGetMouse(md,1);
  OnDrag(md);

  time = sGetTime();
  delta = time-LastTime;
  LastTime = time;
  if(delta>200) delta = 200;
  OnFrame(delta,vp);*/
}

/****************************************************************************/
#if 0

sTumbleCamera::sTumbleCamera()
{
  Time = 0;
  Dist = 5;
  Speed.Init(0.0011f,0.0012f,0.0013f);
}

void sTumbleCamera::OnFrame(sInt delta,sViewport &vp)
{
  Time += delta;
  vp.Camera.EulerXYZ(Time*Speed.x,Time*Speed.y,Time*Speed.z);
  vp.Camera.l = sVector31(vp.Camera.k * (-Dist));
}

/****************************************************************************/

sOrbitCamera::sOrbitCamera()
{
  Angles.Init(0,0,0);
  Dist = 2.25f;
}

void sOrbitCamera::OnFrame(sInt /*delta*/,sViewport &vp)
{
  vp.Camera.EulerXYZ(Angles.x,Angles.y,0);
  vp.Camera.l = sVector31(vp.Camera.k * (-Dist*Dist));
}

void sOrbitCamera::OnDrag(const sMouseData &md)
{
  if(md.MouseButtons & 1)
  {
    Angles.y += md.DeltaX * 0.002f;
    Angles.x += md.DeltaY * 0.002f;
  }
  if(md.MouseButtons & 2)
  {
    Dist += md.DeltaY * 0.1f;
    if(Dist<0) Dist=0;
  }
}

/****************************************************************************/
#endif

sFreeCamera::sFreeCamera()
{
  Pos.Init(0,0,0);
  AnalogX = 0.0f;
  AnalogY = 0.0f;
  SpeedV.Init(0,0,0);
  KeyMask = 0;
  DirLook = 0;
  DirPitch = 0;
  DirRoll = 0;
  Speed = 0.01f;
  LookSpeed = 0.002f;
  Damping = 0.98f;
  Moving = 0;
  MouseControl = sTRUE;
  JoypadControl = sTRUE;
}

void sFreeCamera::OnFrame(sInt delta,sViewport &vp)
{
  // joypad
  if (JoypadControl) {
    sF32 x = Scheme.Analog(INPUT_LX);
    sF32 y = Scheme.Analog(INPUT_LY);
    sF32 x2 = Scheme.Analog(INPUT_RX);
    sF32 y2 = Scheme.Analog(INPUT_RY);

    DirLook += 0.04f*sFPow(x2,5);
    DirPitch += 0.04f*sFPow(y2,5);
    AnalogX = 0.2f*sFPow(x,5);
    AnalogY = 0.2f*sFPow(y,5);
  }

  // mouse
  if (MouseControl) {
    DirLook += Scheme.Relative(INPUT_X) * 0.002f;
    DirPitch += Scheme.Relative(INPUT_Y) * 0.002f;
    if(Scheme.Pressed(INPUT_MBL)) Moving += 1;
    if(Scheme.Pressed(INPUT_MBR)) Moving -= 1;
  }


  vp.Camera.EulerXYZ(DirPitch,DirLook,0);
  sMatrix34 rmat;
  if (DirRoll != 0.0f)
  {
    rmat.EulerXYZ(0,0,DirRoll);
    vp.Camera = rmat * vp.Camera;
  }

  if(KeyMask & 0x01)  SpeedV = SpeedV - vp.Camera.i * (delta*Speed);
  if(KeyMask & 0x02)  SpeedV = SpeedV + vp.Camera.i * (delta*Speed);
  if(KeyMask & 0x04)  SpeedV = SpeedV + vp.Camera.k * (delta*Speed);
  if(KeyMask & 0x08)  SpeedV = SpeedV - vp.Camera.k * (delta*Speed);
  //if(KeyMask & 0x10)  DirRoll = DirRoll + (delta*Speed);
  //if(KeyMask & 0x20)  DirRoll = DirRoll - (delta*Speed);
  SpeedV += AnalogX*vp.Camera.i*delta*Speed;
  SpeedV -= AnalogY*vp.Camera.k*delta*Speed;
  
  if(Damping>0.f)
  { Moving *= Damping;
    SpeedV = SpeedV * Damping;
    Pos = Pos + SpeedV + vp.Camera.k*(Moving*Speed*delta);
  } else
  { Pos = Pos + SpeedV*100.f;// + vp.Camera.k*SpeedV*100.f;
    SpeedV.Init(0,0,0);
  }

  vp.Camera.l = Pos;
}

void sFreeCamera::OnKey(sU32 key)
{
  switch(key)
  {
    case 'a':             KeyMask |= 0x01; break;
    case 'a'|sKEYQ_BREAK: KeyMask &=~0x01; break;
    case 'd':             KeyMask |= 0x02; break;
    case 'd'|sKEYQ_BREAK: KeyMask &=~0x02; break;
    case 'w':             KeyMask |= 0x04; break;
    case 'w'|sKEYQ_BREAK: KeyMask &=~0x04; break;
    case 's':             KeyMask |= 0x08; break;
    case 's'|sKEYQ_BREAK: KeyMask &=~0x08; break;
    case 'q':             KeyMask |= 0x10; break;
    case 'q'|sKEYQ_BREAK: KeyMask &=~0x10; break;
    case 'e':             KeyMask |= 0x20; break;
    case 'e'|sKEYQ_BREAK: KeyMask &=~0x20; break;


    case ' ':  PrintPos(); break;
  }
}

void sFreeCamera::Stop()
{
  SpeedV.Init(0,0,0);
  Moving = 0.0f;
}

/****************************************************************************/

void sFreeCamera::PrintPos()
{
  sMatrix34 mat;
  sQuaternion q;

  mat.Init();
  mat.EulerXYZ(DirPitch,DirLook,0);
  
  q.Init(mat);
  
  sDPrintF(L"  { %08.3ff,%08.3ff,%08.3ff, %05.3ff,%05.3ff,%05.3ff,%05.3ff },\n",Pos.x,Pos.y,Pos.z,q.i,q.j,q.k,q.r);
}

/****************************************************************************/


#if 0
sCameraXSI::sCameraXSI()
{
  StickyMode = sTRUE;
  Init();
}

/****************************************************************************/

void sCameraXSI::Init()
{
  KeyMaskCurrent = 0;
  KeyMaskSticky = 0;

  Target.Init(0,0,0);
  EulerRot.Init(0,0,0);
  Scale.Init(1.0f,1.0f,1.0f);
  Distance = 10;
  DollyActive = sFALSE;
  DollyStep   = 0.0f;
}

/****************************************************************************/

void sCameraXSI::OnFrame(sInt /*delta*/, sViewport &vp)
{
  vp.Camera.EulerXYZ(EulerRot.x,EulerRot.y,EulerRot.z);

  vp.Camera.l = Target - vp.Camera.k * Distance;

  vp.Camera.i *= Scale.x;
  vp.Camera.j *= Scale.y;
  vp.Camera.k *= Scale.z;

}

/****************************************************************************/

void sCameraXSI::OnDrag(const sMouseData &md)
{
  sMatrix34 mtx;
  mtx.EulerXYZ(EulerRot.x,EulerRot.y,EulerRot.z);
  mtx.l -= mtx.k * Distance;


  sU32 KeyMask = KeyMaskCurrent;
  
  if (StickyMode)
  {
    if (!KeyMask)
      KeyMask = KeyMaskSticky;
    else
      KeyMaskSticky = KeyMaskCurrent; 
    
    if (!(md.MouseButtons&7))
      KeyMaskSticky = KeyMaskCurrent;
  }
  
  // dolly
  if (KeyMask&XSI_DOLLY)
  {
    if (md.MouseButtons&7)
    {
      if (!DollyActive)
      {
        DollyStep = Distance/200.0f;
        DollyActive = sTRUE;
      }

      sF32 speed = md.MouseButtons==4 ? 0.5f :
                   md.MouseButtons==2 ? 2.0f : 1.0f;

      Distance = Distance - DollyStep * md.DeltaY * speed;
      if (Distance<0.1f)
        Distance = 0.1f;
    }
    else
    {
      DollyActive = sFALSE;
    }
  }
  if ((KeyMask&XSI_SELECT) && (md.MouseButtons&sMB_MIDDLE))
  {
    if (!DollyActive)
    {
      DollyStep = Distance/200.0f;
      DollyActive = sTRUE;
    }

    sF32 speed = 1.0f;

    Distance = Distance - DollyStep * md.DeltaY * speed;
    if (Distance<0.1f)
      Distance = 0.1f;
  }

  // translation in camera-plane
  if (KeyMask&XSI_TRANS_CAMPLANE)
  {
    if (md.MouseButtons&1)
    {
      sVector30 vec;
      vec = (mtx.i * (-md.DeltaX) + mtx.j * md.DeltaY) * Distance * 0.002f;
      Target += vec;
    }

    if (md.MouseButtons&6)
    {
      Scale.x *= (md.MouseButtons&2 ? 0.99f : 1.0f/0.99f);
      Scale.y *= (md.MouseButtons&2 ? 0.99f : 1.0f/0.99f);
      if (Scale.x<0.01f)
        Scale.x = 0.01f;
      if (Scale.y<0.01f)
        Scale.y = 0.01f;
    }
  }
  if ((KeyMask&XSI_SELECT) && (md.MouseButtons&sMB_LEFT))
  {
    sVector30 vec;
    vec = (mtx.i * (-md.DeltaX) + mtx.j * md.DeltaY) * Distance * 0.002f;
    Target += vec;
  }

  // orbit
  if (KeyMask&XSI_ORBIT)
  {
    if (md.MouseButtons&3)
      EulerRot.x += md.DeltaY*0.01f;
    if (md.MouseButtons&5)
      EulerRot.y += md.DeltaX*0.01f;
  }
  if ((KeyMask&XSI_SELECT) && (md.MouseButtons&sMB_RIGHT))
  {
      EulerRot.x += md.DeltaY*0.01f;
      EulerRot.y += md.DeltaX*0.01f;
  }
  
}

/****************************************************************************/

void sCameraXSI::OnKey(sU32 key)
{
  switch(key)
  {
    case 'p':             KeyMaskCurrent |= XSI_DOLLY; break;
    case 'p'|sKEYQ_BREAK: KeyMaskCurrent &=~XSI_DOLLY; break;
    case 'o':             KeyMaskCurrent |= XSI_ORBIT; break;
    case 'o'|sKEYQ_BREAK: KeyMaskCurrent &=~XSI_ORBIT; break;
    case 'z':             KeyMaskCurrent |= XSI_TRANS_CAMPLANE; break;
    case 'z'|sKEYQ_BREAK: KeyMaskCurrent &=~XSI_TRANS_CAMPLANE; break;
    case 'h':             Init(); break;
    case 's':             KeyMaskCurrent |= XSI_SELECT; break;
    case 's'|sKEYQ_BREAK: KeyMaskCurrent &=~XSI_SELECT; break;
      break;
    default:;
  }
}
#endif
/****************************************************************************/


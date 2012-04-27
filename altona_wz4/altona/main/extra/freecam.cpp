/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "freecam.hpp"
#include "base/system.hpp"

/****************************************************************************/

sFreeflightCamera::sFreeflightCamera()
{
  Pos.Init(0,0,-5);
  Center.Init(0,0,0);
  RotX = 0;
  RotY = 0;
  Zoom = 2.0f;
  SpeedGrade = 0;
  KeyMask = 0;
  Spaceship = 0;
  LMB = 0;
  RMB = 0;
  MouseHardX = 0;
  MouseHardY = 0;
}

void sFreeflightCamera::OnInput(const sInput2Event &ie)
{
  switch(ie.Key & (sKEYQ_BREAK|sKEYQ_MASK))
  {
  case 'a':             KeyMask |= 0x01; break;
  case 'a'|sKEYQ_BREAK: KeyMask &=~0x01; break;
  case 'd':             KeyMask |= 0x02; break;
  case 'd'|sKEYQ_BREAK: KeyMask &=~0x02; break;
  case 'w':             KeyMask |= 0x04; break;
  case 'w'|sKEYQ_BREAK: KeyMask &=~0x04; break;
  case 's':             KeyMask |= 0x08; break;
  case 's'|sKEYQ_BREAK: KeyMask &=~0x08; break;

  case 'r':
    Pos.Init(0,0,-5);
    RotX = 0;
    RotY = 0;
    Zoom = 1.6f;
    SpeedGrade = 0;
    KeyMask = 0;
    LMB = 0;
    RMB = 0;
    break;

  case sKEY_MOUSEHARD:
    MouseHardX += sS16(ie.Payload >> 16);
    MouseHardY += sS16(ie.Payload & 0xffff);
    break;

  case sKEY_LMB:              LMB = 1; break;
  case sKEY_LMB|sKEYQ_BREAK:  LMB = 0; break;
  case sKEY_RMB:              RMB = 1; break;
  case sKEY_RMB|sKEYQ_BREAK:  RMB = 0; break;
  case sKEY_WHEELUP:          SpeedGrade = sClamp<sF32>(SpeedGrade + 1,-40,20); break;
  case sKEY_WHEELDOWN:        SpeedGrade = sClamp<sF32>(SpeedGrade - 1,-40,20); break;
  }

//  sDPrintF(L"%d %08x %08x\n",ie.Instance,ie.Key,ie.Payload);
}

void sFreeflightCamera::OnFrame(sInt ticks,sF32 jitter)
{
  sInt x = MouseHardX;
  sInt y = MouseHardY;
  static sInt xo,yo;
  static sInt first = 1;

  sU32 qual = sGetKeyQualifier();
  if(first || ticks>100)
  {
    first = 0;
    xo = x;
    yo = y;
  }
  if(ticks>10)
    ticks = 10;
  sF32 Speed = sPow(sSqrt(2),SpeedGrade)*0.1f*ticks;

  if(LMB)
  {
    RotX += (y-yo)*0.01f;
    RotY += (x-xo)*0.01f;
  }
  if(RMB)
  {
    RotZ -= (x-xo)*0.01f;
  }

  if(Spaceship)
  {
    sMatrix34 mat;
    mat.EulerXYZ(RotX,RotY,RotZ);
    Matrix.l.Init(0,0,0);
    Matrix = mat*Matrix;
    RotX = 0;
    RotY = 0;
    RotZ = 0;
  }
  else
  {
    Matrix.EulerXYZ(RotX,RotY,0);
  }
  sF32 sx = 0;
  sF32 sz = 0; 
  if(KeyMask & 0x01) sx = -1;
  if(KeyMask & 0x02) sx =  1;
  if(KeyMask & 0x04) sz =  1;
  if(KeyMask & 0x08) sz = -1;
  Pos += (Matrix.i * sx + Matrix.k * sz)*(Speed);
  if(qual & sKEYQ_ALT)
    Pos = sVector31(0,0,0) + Matrix.k * -(sVector30(Pos).Length());
  else
    Pos += (Matrix.i * sx + Matrix.k * sz)*(Speed);
  Matrix.l = Pos;

  xo = x;
  yo = y;
}

void sFreeflightCamera::Set(sMatrix34 mat,sF32 zoom)
{
  Zoom = zoom;
  Pos = mat.l;
  mat.l.Init(0,0,0);
  if(Spaceship)
  {
    Matrix = mat;
    RotX = RotY = RotZ = 0;
  }
  else
  {
    mat.FindEulerXYZ2(RotX,RotY,RotZ);
    RotZ = 0;
  }
}


void sFreeflightCamera::MakeViewport(sViewport &view)
{
  view.SetTargetCurrent();
  view.SetZoom(Zoom);
  view.Model.Init();
  view.Camera = Matrix;
  view.Prepare();
}

void sFreeflightCamera::SetCenter(const sVector31 &center)
{
  Center = center;
}

/****************************************************************************/


/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_EXTRA_FREECAM_HPP
#define FILE_EXTRA_FREECAM_HPP

#include "base/types.hpp"
#include "base/math.hpp"
#include "base/graphics.hpp"

/****************************************************************************/


class sFreeflightCamera
{
  sVector31 Pos;
  sVector31 Center;
  sF32 RotX;
  sF32 RotY;
  sF32 RotZ;
  sF32 Zoom;
  sInt KeyMask;

  sInt SpeedGrade;
  sInt Spaceship;
  sMatrix34 Matrix;

  sInt MouseHardX;
  sInt MouseHardY;

  sInt LMB;
  sInt RMB;

public:
  sFreeflightCamera();

  void OnInput(const sInput2Event &ie);
  void OnFrame(sInt ticks,sF32 jitter);
  void MakeViewport(sViewport &view);
  void SetCenter(const sVector31 &center);
  void SetPos(const sVector31 &pos) { Pos = pos; }
  void Set(sMatrix34 mat,sF32 zoom);
  void SetSpeed(sInt n) { SpeedGrade = n; }
  sInt GetSpeed() { return SpeedGrade; }
  void SpaceshipMode(sBool b) { Spaceship = b; }
};

/****************************************************************************/

#endif // FILE_EXTRA_FREECAM_HPP


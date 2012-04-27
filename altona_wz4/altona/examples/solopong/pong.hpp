/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_SOLOPONG_PONG_HPP
#define FILE_SOLOPONG_PONG_HPP

#include "base/types.hpp"
#include "main.hpp"

/****************************************************************************/

class SoloPong
{
  sF32 BallX;
  sF32 BallY;
  sF32 SpeedX;
  sF32 SpeedY;
  sF32 PaddleA;
  sF32 PaddleB;
  sF32 PaddleSize;
  sF32 Speedup;
  sInt Lives;
  sInt DeathTimer;
  sInt Score;
public:
  SoloPong();
  ~SoloPong();
  void Reset();
  void OnTick();
  void OnPaint();
  
  sString<256> ScoreString;
};

/****************************************************************************/

#endif // FILE_SOLOPONG_PONG_HPP


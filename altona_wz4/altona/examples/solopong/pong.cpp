/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "pong.hpp"

/****************************************************************************/

const sF32 speed = 0.01f;

SoloPong::SoloPong()
{
  Reset();
}

void SoloPong::Reset()
{
  SpeedX = -1;
  SpeedY = 0.1f;
  BallX = 0;
  BallY = 0;
  PaddleA = 0;
  PaddleB = 0;

  Speedup = 0.1f;
  PaddleSize = 0.2f;
  Lives = 3;
  DeathTimer = 0;
  Score = 0;
}

SoloPong::~SoloPong()
{
}

/****************************************************************************/

void SoloPong::OnTick()
{
  sF32 dist;
  BallX += SpeedX * speed;
  BallY += SpeedY * speed;

  dist = (PaddleA-BallY)/PaddleSize;
  if(BallX<=-0.86f && SpeedX<0 && dist>=-1 && dist<1)
  {
    SpeedX =  sFAbs(SpeedX)+Speedup;
    SpeedY -= dist*1.0f;
    Score++;
  }

  dist = (PaddleB-BallY)/PaddleSize;
  if(BallX>=0.86f && SpeedX>0 && dist>=-1 && dist<1)
  {
    SpeedX = -sFAbs(SpeedX)-Speedup;
    SpeedY -= dist*1.0f;
    Score++;
  }

  sBool die = 0;

  if(BallY<-1) { SpeedY =  sFAbs(SpeedY); }
  if(BallY> 1) { SpeedY = -sFAbs(SpeedY); }
  if(BallX<-1) { SpeedX =  sFAbs(SpeedX); die = 1; }
  if(BallX> 1) { SpeedX = -sFAbs(SpeedX); die = 1; }

  if(die)
  {
    DeathTimer = 200;
    Lives--;
    if(Lives==0)
    {
      SpeedX = 0;
      SpeedY = 0;
      BallX = 0;
      BallY = 2;
      SpeedX -= sSign(SpeedX)*0.3f;
      DeathTimer = 2000;
    }
  }
  if(Lives==0 && DeathTimer == 0)
    Reset();

  DeathTimer = sMax(DeathTimer-10,0);
}

void SoloPong::OnPaint()
{
  Joypad pad;
  GetJoypad(pad);
  const sF32 sb = 0.02f;
  const sF32 sx = 0.04f;
  const sF32 sy = PaddleSize;

  PaddleA = sClamp<sF32>(pad.Analog[1]*1.1f,-1,1)*(1-PaddleSize);
  PaddleB = sClamp<sF32>(pad.Analog[3]*1.1f,-1,1)*(1-PaddleSize);

  sU32 col = 0xff000000;
  col |= sClamp(DeathTimer,0,255)<<16;

  DrawRect(-1,-1,1,1,col);
  DrawRect(-0.9f-sx,PaddleA-sy,-0.9f+sx,PaddleA+sy,0xffffffff);
  DrawRect( 0.9f-sx,PaddleB-sy, 0.9f+sx,PaddleB+sy,0xffffffff);

  DrawRect(BallX-sb,BallY-sb,BallX+sb,BallY+sb,0xffffffff);
  
  ScoreString.PrintF(L"SCORE: %03d  [%c] [%c] [%c]",Score
    ,Lives>=1?'X':' '
    ,Lives>=2?'X':' '
    ,Lives>=3?'X':' '
    );
}

/****************************************************************************/


/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "pong.hpp"

/****************************************************************************/

SoloPong::SoloPong()
{
  FlashTimer = 0;
}

SoloPong::~SoloPong()
{
}

/****************************************************************************/

void SoloPong::OnTick()
{
  // decrement flashtimer

  FlashTimer = sMax(0,FlashTimer-10);
}

void SoloPong::OnPaint()
{

  // clear background

  sInt flash = sClamp(FlashTimer,0,255);
  sU32 color = 0xff000000 | (flash<<16);
  DrawRect(-1,-1,1,1,color);

  // query joypad

  Joypad pad;
  GetJoypad(pad);

  // print button status on screen

  ScoreString.PrintF(L"%04x : %5.2f %5.2f : %5.2f %5.2f",pad.Buttons,
    pad.Analog[0],pad.Analog[1],pad.Analog[2],pad.Analog[3]);

  // draw analog buttons

  const sF32 s = 0.01f;
  DrawRect(pad.Analog[0]-s,pad.Analog[1]-s,pad.Analog[0]+s,pad.Analog[1]+s,0xffff0000);
  DrawRect(pad.Analog[2]-s,pad.Analog[3]-s,pad.Analog[2]+s,pad.Analog[3]+s,0xffff0000);

  // flash when button is pressed

  if((pad.Buttons&15) && FlashTimer<=0)
    FlashTimer = 255;

}

/****************************************************************************/


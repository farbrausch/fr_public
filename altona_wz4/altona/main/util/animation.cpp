/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "animation.hpp"

/****************************************************************************/

/****************************************************************************/
/***                                                                      ***/
/***   FadeData                                                           ***/
/***                                                                      ***/
/****************************************************************************/
#if 1
sF32 sFadeData::GetFactor(sU32 type)
{
  switch (type)
  {
  default:
  case LINEAR:
    return sF32(Time)/Length;
  case SMOOTH:
    {
    sF32 x = sF32(Time)/Length;
    return -2*x*x*x+3*x*x;
    }
  case SMOOTH_IN:
    {
    sF32 x = sF32(Time)/Length;
    return -1*x*x*x+2*x*x;
    }
  case SMOOTH_OUT:
    {
    sF32 x = 1-sF32(Time)/Length;
    return 1-(-1*x*x*x+2*x*x);
    }
  case SWITCH:
    {
    return (sF32)((int)(sF32(Time)/Length));
    }
  }
}

/****************************************************************************/

void sFadeData::Fade (sInt delta, sFadeData::FadeDirection fade)
 {
   if      (fade==IN)  FadeIn(delta); 
   else if (fade==OUT) FadeOut(delta); 
   else if (Dir!=NONE) Fade(delta,Dir); 
 }
#endif
/****************************************************************************/

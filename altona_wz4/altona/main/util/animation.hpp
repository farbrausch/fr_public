/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_UTIL_ANIMATION_HPP
#define FILE_UTIL_ANIMATION_HPP

#include "base/types.hpp"
#if 1
/****************************************************************************/

/****************************************************************************/
/***                                                                      ***/
/***   Fade Data                                                          ***/
/***                                                                      ***/
/****************************************************************************/

class sFadeData
{
public:
  enum FadeDirection { NONE, IN, OUT };
  enum FadeType { LINEAR, SMOOTH, SMOOTH_IN, SMOOTH_OUT, SWITCH };
  enum { IMMEDIATE = 9999999 };
  
  void Init (sInt length) {Length=length; Time=0; Dir=NONE;}
  void Fade (sInt delta, FadeDirection fade=NONE);
  void FadeIn (sInt delta=0) {Time = sMin(Length, Time+delta); Dir=IN;}
  void FadeOut (sInt delta=0) {Time = sMax(0, Time-delta); Dir=OUT;}
  sInt GetDirection() { return (Dir==IN ? 1 : (Dir==OUT ? -1 : 0)); }
  sF32 GetFactor(sU32 type=SMOOTH);
  sInt GetTime() { return Time; }
  sInt GetLength() { return Length; }
  
  sInt Time, Length;
  FadeDirection Dir;
};

/****************************************************************************/
#endif
#endif // FILE_UTIL_ANIMATION_HPP


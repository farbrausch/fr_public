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

#ifndef FILE_BASE_SOUND_HPP
#define FILE_BASE_SOUND_HPP

#include "base/types.hpp"

/****************************************************************************/

typedef void (*sSoundHandler)(sS16 *data,sInt count);      // 1 count = left+right = 4 bytes

// sound out

sBool sSetSoundHandler(sInt freq,sSoundHandler,sInt latency,sInt flags=0);
void sClearSoundHandler();
sInt sGetCurrentSample();
void sClearSoundBuffer();

void sSoundHandlerNull(sS16 *data,sInt count);
void sSoundHandlerTest(sS16 *data,sInt count);

enum sSoundOutFlags
{
  sSOF_GLOBALFOCUS = 1,
  sSOF_5POINT1 = 2,     // 6-channel 5.1 output instead of stereo
};

// sound in

sBool sSetSoundInHandler(sInt freq,sSoundHandler,sInt latency);
void sSoundInput();
sInt sGetCurrentInSample();

void sLockSound();
void sUnlockSound();

/****************************************************************************/

#endif

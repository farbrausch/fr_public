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

#pragma once

#include "util/musicplayer.hpp"

/****************************************************************************/

class sOGGDecoder : public sMusicPlayer
{
  sInt Size;                      // how many samples were decoded in the buffer
  sInt Index;                     // how much of the buffer was read (in samples)
  sInt Restart;                   // this is a restart, do some cleanup

//  struct sOGGStub *Stub;
  struct stb_vorbis *dec;

public:
  sOGGDecoder();
  ~sOGGDecoder();

  sInt ZeroStart;                 // put some zeros before start

  sBool Init(sInt songnr);
  sInt Render(sS16 *stream,sInt samples);
  sInt GetTuneLength();
};

/****************************************************************************/

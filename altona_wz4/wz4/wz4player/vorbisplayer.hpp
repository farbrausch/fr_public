/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_OGG_VORBISPLAYER_HPP
#define FILE_OGG_VORBISPLAYER_HPP

#include "base/types.hpp"

/****************************************************************************/

// Bob Fungus presents: Bob Fungus's Bob's Vorbis Player

class bMusicPlayer
{
public:

  explicit bMusicPlayer(sBool seekable=sFALSE);
  ~bMusicPlayer();

  void Init(const sChar *filename);
  //void Init(void *ptr, sDInt size); disabled, needs file extension now
  void Exit();

  sBool IsLoaded();

  void Play();
  void Pause();
  void Seek(sF32 time); // in seconds, only when seekable==sTRUE;
  void Skip(sInt samples); // in samples, only when not seekable

  void SetVolume(sF32 vol, sF32 fadeduration=0);
  void SetLoop(sBool loop);

  sBool IsPlaying(); // returns if still playing
  sF32 GetPosition(); // returns exact position in seconds
                      // WARNING: starts with negative values because of latency!

private:

  struct VerySecret;
  VerySecret *p;

};

/****************************************************************************/

#endif // FILE_OGG_VORBISPLAYER_HPP


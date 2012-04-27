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

#ifndef HEADER_ALTONA_UTIL_MUSICPLAYER
#define HEADER_ALTONA_UTIL_MUSICPLAYER

#ifndef __GNUC__
#pragma once
#endif

#include "base/types.hpp"

/****************************************************************************/

class sMusicPlayer
{
private:
  sS16 *RewindBuffer;               // remember all rendered samples
  sInt RewindSize;                  // max size in samples
  sInt RewindPos;                   // up to this sample the buffer has been rendered
protected:
  sU8 *Stream;                      // the loaded data
  sInt StreamSize;
  sBool StreamDelete;

public:
  sInt Status;                      // 0=error, 1=ready, 2=pause, 3=playing
  sInt PlayPos;                     // current position for continuing rendering

  sMusicPlayer();
  virtual ~sMusicPlayer();
  sBool Load(const sChar *name);    // load from file, memory is deleted automatically
  sBool Load(sU8 *data,sInt size);  // load from buffer, memory is not deleted automatically
  sBool LoadAndCache(const sChar *name);
  sInt GetCacheSamples() { return RewindSize; }
  sS16 *GetCacheData() { return RewindBuffer; }
  sBool IsPlaying() { return Status==3; }
  void InstallHandler();            // install a soundhandler.
  void AllocRewind(sInt bytes);     // allocate a rewindbuffer
  sBool Start(sInt songnr);         // initialize and start playing
  void Stop();                      // stop playing
  sBool Handler(sS16 *buffer,sInt samples,sInt Volume=256); // return sFALSE if all following calls to handler will just return silence

  virtual sBool Init(sInt songnr)=0;
  virtual sInt Render(sS16 *buffer,sInt samples)=0;   // return number of samples actually rendered, 0 for eof
  virtual sInt GetTuneLength() = 0; // in samples
};

/****************************************************************************/

// HEADER_ALTONA_UTIL_MUSICPLAYER
#endif

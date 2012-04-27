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

#include "util/musicplayer.hpp"
#include "base/system.hpp"
#include "base/sound.hpp"
static sMusicPlayer *sMusicPlayerPtr;

/****************************************************************************/
/***                                                                      ***/
/***   MusicPlayers                                                       ***/
/***                                                                      ***/
/****************************************************************************/

sMusicPlayer::sMusicPlayer()
{
  Stream = 0;
  StreamSize = 0;
  StreamDelete = 0;
  Status = 0;
  RewindBuffer = 0;
  RewindSize = 0;
  RewindPos = 0;
  PlayPos = 0;
}

sMusicPlayer::~sMusicPlayer()
{
  if(sMusicPlayerPtr==this)
    sClearSoundHandler();
  if(StreamDelete)
    delete[] Stream;
  if(RewindBuffer)
    delete[] RewindBuffer;
}

sBool sMusicPlayer::LoadAndCache(const sChar *name)
{
  sString<4096> buffer;
  sDInt size;
  sU8 *mem;

  buffer = name;
  buffer.Add(L".raw");

  mem = sLoadFile(buffer,size);
  if(mem)
  {
    RewindBuffer = (sS16 *)mem;
    RewindSize = size/4;
    RewindPos = size/4;
    sVERIFY(Stream==0);
    Status = 3;
    return sTRUE;
  }
  else
  {
    if(Load(name))
    {
      Start(0);
      size = GetTuneLength() * 5;     // 25% safety for unrelyable GetTuneLength()
      mem = new sU8[size];
      size = Render((sS16 *)mem,size/4)*4;
      RewindBuffer = (sS16 *)mem;
      RewindSize = size/4;
      RewindPos = size/4;
      sSaveFile(buffer,mem,size);
      return sTRUE;
    }
    else
    {
      return sFALSE;
    }
  }
}

sBool sMusicPlayer::Load(const sChar *name)
{
  sDInt size;

  if(StreamDelete)
    delete[] Stream;
  Stream = sLoadFile(name,size);
  StreamSize = size;
  StreamDelete = sFALSE;
  if(Stream)
  {
    StreamDelete = sTRUE;
    Status = 1;
    return sTRUE;
  }
  return sFALSE;
}

sBool sMusicPlayer::Load(sU8 *data,sInt size)
{
  if(StreamDelete)
    delete[] Stream;
  Stream = data;
  StreamSize = size;
  StreamDelete = sFALSE;
  Status = 1;
  return sTRUE;
}

void sMusicPlayer::AllocRewind(sInt bytes)
{
  RewindSize = bytes/4;
  RewindBuffer = new sS16[bytes/2];
  RewindPos = 0;
}

sBool sMusicPlayer::Start(sInt songnr)
{
  if(Status!=0)
  {
    if(Init(songnr))
      Status = 3;
    else
      Status = 0;
  }
  return Status==3;
}

void sMusicPlayer::Stop()
{
  if(Status == 3)
    Status = 1;
}

static void sMusicPlayerSoundHandler(sS16 *samples,sInt count)
{
  if(sMusicPlayerPtr)
    sMusicPlayerPtr->Handler(samples,count,0x100);
}

void sMusicPlayer::InstallHandler()
{
  sVERIFY(sMusicPlayerPtr==0);
  sMusicPlayerPtr = this;
  sSetSoundHandler(44100,sMusicPlayerSoundHandler,44100);
}

sBool sMusicPlayer::Handler(sS16 *buffer,sInt samples,sInt vol)
{
  sInt diff,size;
  sInt result;
  sInt i;

  result = 1;
  if(Status==3)
  {
    if(RewindBuffer)
    {
      if(PlayPos+samples < RewindSize)
      {
        while(RewindPos<PlayPos+samples && result)
        {
          diff = PlayPos+samples-RewindPos;
          size = Render(RewindBuffer+RewindPos*2,diff);
          if(size==0) 
            result = 0;
          RewindPos+=size;
        }
        size = samples;
        if(PlayPos+size>RewindSize)
          size = PlayPos-RewindPos;
        if(size>0)
        {
          for(i=0;i<size*2;i++)
            buffer[i] = (RewindBuffer[PlayPos*2+i]*vol)>>8;
//          sCopyMem(buffer,RewindBuffer+PlayPos*2,size*4);
          buffer+=size*2;
          PlayPos += size;
          samples -= size;
        }
      }
      if(samples>0)
      {
        sSetMem(buffer,0,samples*4);
        Status = 1;
      }
    }
    else
    {
      while(samples>0 && result)
      {
        size = Render(buffer,samples);
        if(size==0)
        {
          result = 0;
          Status = 1;
        }
        buffer+=size*2;
        PlayPos += size;
        samples-=size;
      }
    }
  }
  else
  {
    sSetMem(buffer,0,samples*4);
  }
  return result;
}


/****************************************************************************/

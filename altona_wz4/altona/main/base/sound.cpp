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

#include "base/types.hpp"

#if sCONFIG_SYSTEM_WINDOWS

#include "base/sound.hpp"
#include "base/system.hpp"
#include <windows.h>
#include <dsound.h>
#include <audiodefs.h>

// thread

static sThread *DXSThread;
static sThreadLock *DXSLock;
static sThreadEvent *DXSEvent;
//static sInt DXSRun;

// generic

#define DXSAlign 64

// output

static IDirectSound8  *DXSO;
static IDirectSoundBuffer *DXSOPrimary;
static IDirectSoundBuffer8 *DXSOBuffer;
static sSoundHandler DXSOHandler;

static volatile sInt DXSOReadOffset;
static volatile sInt DXSOReadStart;
static volatile sInt DXSOTime;
static volatile sInt DXSOIndex;
static volatile sInt DXSOTotalSamples;
static volatile sInt DXSOFirstFrames;
static sInt DXSOSamples=1000;
static sInt DXSORate=44100;
static sInt DXSOChannels=2;

// input

static IDirectSoundCapture8 *DXSI;
static IDirectSoundCaptureBuffer8 *DXSIBuffer;
static sSoundHandler DXSIHandler;

static sInt DXSIIndex;
static sInt DXSISamples=0xc000;
static sInt DXSIRate=44100;

/****************************************************************************/

extern HWND sHWND;
void sClearSoundHandler();
void sClearSoundInHandler();
static void SoundThreadCode(sThread *, void *);

/****************************************************************************/

void sInitSound()
{
  DXSOReadOffset = 0;
  DXSOReadStart = 0;
  DXSOTime = 0;
  DXSOIndex = 0;
  DXSOTotalSamples = 0;
  DXSOFirstFrames = 0;

  DXSLock = new sThreadLock();      // create soundlock in any case.
  if(!(sGetSystemFlags()&sISF_NOSOUND))
  {
    DXSOTime = 0;

    DXSEvent = new sThreadEvent();
    DXSThread = new sThread(SoundThreadCode,1);
  }
}

void sExitSound()
{
  sClearSoundHandler();
  sClearSoundInHandler();
  sDelete(DXSThread);
  sDelete(DXSEvent);
  sDelete(DXSLock);
  DXSOTime = 0;
}

sADDSUBSYSTEM(Sound,0xb0,sInitSound,sExitSound);

void sLockSound()
{
  if(DXSLock)
    DXSLock->Lock();
}

void sUnlockSound()
{
  if(DXSLock)
    DXSLock->Unlock();
}

void sPingSound()
{
  if(DXSEvent)
    DXSEvent->Signal();

  if (!DXSOTime) // has thread run yet?
    return;

  sLockSound();
  sInt sample = DXSOReadStart + DXSOReadOffset + sMulDiv(timeGetTime()-DXSOTime,DXSORate,1000) - DXSOSamples;
//  sDPrintF(L"delta = %d %08x %08x %s\n",sample-DXSOTotalSamples,DXSOReadStart,DXSOTotalSamples,sample<DXSOTotalSamples?L" !!!!!!!!!":L"");
  if(DXSOFirstFrames<5 || sample<0)
    DXSOTotalSamples = 0;
  else if(sample>DXSOTotalSamples)
    DXSOTotalSamples = sample;
  if(DXSOFirstFrames<5)
    DXSOFirstFrames++;
  sUnlockSound();
}

/****************************************************************************/

void sClearSoundBuffer()
{
  HRESULT hr;
  void *p1,*p2;
  DWORD c1,c2;

  sLockSound();

  hr = DXSOBuffer->Lock(0,0,&p1,&c1,&p2,&c2,DSBLOCK_ENTIREBUFFER);
  if(!FAILED(hr))
  {
    sSetMem(p1,0,c1);
    sSetMem(p2,0,c2);
    DXSOBuffer->Unlock(p1,c1,p2,c2);
  }

  sUnlockSound();
}

static void SoundThreadCode(sThread *thread, void *userdata)
{
  HRESULT hr;
  DWORD count1,count2;
  void *pos1,*pos2;

  while(thread->CheckTerminate())
  {
    sInt timeout = (DXSOSamples*1000/DXSORate)/3;
    DXSEvent->Wait(timeout);

    // output

    sLockSound();
    if(DXSOBuffer)
    {
      const sInt SAMPLESIZE = 2*DXSOChannels;

      sInt play,dummy;
      sInt size,pplay;

      hr = DXSOBuffer->GetCurrentPosition((DWORD*)&play,(DWORD*)&dummy);
      if(!FAILED(hr))
      {
        pplay = play;
        play = play/SAMPLESIZE;
        DXSOReadOffset = play;
        DXSOTime = timeGetTime();
        if(DXSOIndex>play)
          play+=DXSOSamples;
        size = play-DXSOIndex;
        size = (size)&(~(DXSAlign-1));
        if(size>0)
        {
          count1 = 0;
          count2 = 0;
          hr = DXSOBuffer->Lock(DXSOIndex*SAMPLESIZE,size*SAMPLESIZE,&pos1,&count1,&pos2,&count2,0);
          if(!FAILED(hr))
          {
            DXSOIndex += size;
            if(DXSOIndex>=DXSOSamples)
            {
              DXSOIndex-=DXSOSamples;
              DXSOReadStart += DXSOSamples;
            }
            sVERIFY((sInt)(count1+count2)==(size*SAMPLESIZE));
          
            if(DXSOHandler)
            {
              if(count1>0)
                (*DXSOHandler)((sS16 *)pos1,count1/SAMPLESIZE);
              if(count2>0)
                (*DXSOHandler)((sS16 *)pos2,count2/SAMPLESIZE);
            }
            DXSOBuffer->Unlock(pos1,count1,pos2,count2);
          }
        }   
      }
    }
    sUnlockSound();

    // input

  }
}

/****************************************************************************/
/****************************************************************************/

sBool sSetSoundHandler(sInt freq,sSoundHandler handler,sInt latency,sInt flags)
{
  HRESULT hr;
  DSBUFFERDESC dsbd;
  IDirectSoundBuffer *buffer = 0;
  void *p1,*p2;
  DWORD c1,c2;

  WAVEFORMATEXTENSIBLE sformat = { { WAVE_FORMAT_PCM,2,freq,freq*4,4,16,0 }, {0}, {3}, {0} };

  sClearSoundHandler();
  if(!DXSThread) return 0;
  sLockSound();
  DXSOSamples = latency;
  DXSORate = freq;
  DXSOChannels = 2;

  if (flags & sSOF_5POINT1)
  {
    DXSOChannels=6;
    sformat.Format.wFormatTag=WAVE_FORMAT_EXTENSIBLE;
    sformat.Format.cbSize=22;
    sformat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
  }

  sformat.Format.nChannels=DXSOChannels;
  sformat.Format.nBlockAlign=2*DXSOChannels;
  sformat.Format.nAvgBytesPerSec=freq*sformat.Format.nBlockAlign;
  sformat.Samples.wValidBitsPerSample=16;
  sformat.dwChannelMask=(1<<DXSOChannels)-1; // very space opera

  // open device

  hr = DirectSoundCreate8(0,&DXSO,0);
  if(FAILED(hr)) goto error;

  hr = DXSO->SetCooperativeLevel(sHWND,DSSCL_PRIORITY);
  if(FAILED(hr)) goto error;

  // set primary buffer sample rate

  sSetMem(&dsbd,0,sizeof(dsbd));
  dsbd.dwSize = sizeof(dsbd);
  dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
  dsbd.dwBufferBytes = 0;
  dsbd.lpwfxFormat = 0;
  hr = DXSO->CreateSoundBuffer(&dsbd,&DXSOPrimary,0);
  if(SUCCEEDED(hr)) 
    DXSOPrimary->SetFormat(&sformat.Format);

  // create streaming buffer

  sSetMem(&dsbd,0,sizeof(dsbd));
  dsbd.dwSize = sizeof(dsbd);
  dsbd.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
  if(flags & sSOF_GLOBALFOCUS)
    dsbd.dwFlags |= DSBCAPS_GLOBALFOCUS;
  dsbd.dwBufferBytes = latency*DXSOChannels*2;
  dsbd.lpwfxFormat = &sformat.Format;
  hr = DXSO->CreateSoundBuffer(&dsbd,&buffer,0);
  if(FAILED(hr)) goto error;
  hr = buffer->QueryInterface(IID_IDirectSoundBuffer8,(void**)&DXSOBuffer);
  if(FAILED(hr)) goto error;
  buffer->Release(); buffer = 0;

  hr = DXSOBuffer->Lock(0,0,&p1,&c1,&p2,&c2,DSBLOCK_ENTIREBUFFER);
  if(!FAILED(hr))
  {
    if (p1) sSetMem(p1,0,c1);
    if (p2) sSetMem(p2,0,c2);
    DXSOBuffer->Unlock(p1,c1,p2,c2);
  }
  DXSOHandler = handler;

  DXSOBuffer->Play(0,0,DSBPLAY_LOOPING);

  sUnlockSound();
  sPingSound();
  return sTRUE;
error:
  sClearSoundHandler();
  sUnlockSound();
  return sFALSE;
}

void sClearSoundHandler()
{
  sLockSound();
  if(DXSOBuffer) DXSOBuffer->Stop();
  sRelease(DXSOBuffer);
  sRelease(DXSOPrimary);
  sRelease(DXSO);
  sUnlockSound();
}

sInt sGetCurrentSample()
{
  return DXSOTotalSamples;
}

/****************************************************************************/
/****************************************************************************/

void sClearSoundInHandler()
{
  sLockSound();
  if(DXSIBuffer) DXSIBuffer->Stop();
  sRelease(DXSIBuffer);
  sRelease(DXSI);
  sUnlockSound();
}

sBool sSetSoundInHandler(sInt freq,sSoundHandler handler,sInt latency)
{
  HRESULT hr;
  DSCBUFFERDESC desc;
  IDirectSoundCaptureBuffer *cbuf;
  static WAVEFORMATEX wfx = { WAVE_FORMAT_PCM,2,freq,freq*4,4,16,0};


  // los gehts

  sClearSoundInHandler();

  sLockSound();
  cbuf = 0;
  DXSIRate = freq;
  DXSISamples = latency;
  DXSIHandler = handler;

  // create device

  hr = DirectSoundCaptureCreate8(0,&DXSI,0);
  if(FAILED(hr)) goto error;

  // create buffer

  sSetMem(&desc,0,sizeof(desc));
  desc.dwSize = sizeof(desc);
  desc.dwBufferBytes = latency*4;
  desc.lpwfxFormat = &wfx;
 
  hr = DXSI->CreateCaptureBuffer(&desc,&cbuf,0);
  if(FAILED(hr)) goto error;

  hr = cbuf->QueryInterface(IID_IDirectSoundCaptureBuffer8, (LPVOID*)&DXSIBuffer);
  sRelease(cbuf);
  if(FAILED(hr)) goto error;

  // kick it!

  hr = DXSIBuffer->Start(DSCBSTART_LOOPING);
  if(FAILED(hr)) goto error;
  sUnlockSound();


  return 1;

error:
  sRelease(DXSIBuffer);
  sRelease(DXSI);
  sUnlockSound();
  return 0;
}

void sSoundInput()
{
  HRESULT hr;
  const sInt SAMPLESIZE = 4;
  DWORD count1,count2;
  void *pos1,*pos2;

  sLockSound();
  if(DXSIBuffer)
  {
    sInt play,pplay,size,dummy;

    hr = DXSIBuffer->GetCurrentPosition((DWORD *)&dummy,(DWORD *)&play);
    if(!FAILED(hr))
    {

      pplay = play;
      play = play/SAMPLESIZE;
//      DXSIReadOffset = play;
//      DXSITime = timeGetTime();
      if(DXSIIndex>play)
        play+=DXSISamples;
      size = play-DXSIIndex;
      size = (size)&(~(DXSAlign-1));

      if(size>0)
      {
        count1 = 0;
        count2 = 0;
        hr = DXSIBuffer->Lock(DXSIIndex*SAMPLESIZE,size*SAMPLESIZE,&pos1,&count1,&pos2,&count2,0);
        if(!FAILED(hr))
        {
          DXSIIndex += size;
          if(DXSIIndex>=DXSISamples)
          {
            DXSIIndex-=DXSISamples;
//            DXSIReadStart += DXSISamples;
          }
          sVERIFY((sInt)(count1+count2)==(size*4));
        
          if(DXSIHandler)
          {
            if(count1>0)
              (*DXSIHandler)((sS16 *)pos1,count1/SAMPLESIZE);
            if(count2>0)
              (*DXSIHandler)((sS16 *)pos2,count2/SAMPLESIZE);
          }
          DXSIBuffer->Unlock(pos1,count1,pos2,count2);
        }
      }   
    }
  }
  sUnlockSound();
}

sInt sGetCurrentInSample()
{
  return 0;
}

/****************************************************************************/
/****************************************************************************/

#endif  // windows platforms

/****************************************************************************/

void sSoundHandlerNull(sS16 *data,sInt count)
{
  for(sInt i=0;i<count;i++)
  {
    data[0] = 0;
    data[1] = 0;
    data+=2;
  }
}

static sInt phase0,phase1;
static sInt mod=0x4000;

void sSoundHandlerTest(sS16 *data,sInt count)
{
  for(sInt i=0;i<count;i++)
  {
    data[0] = sInt(sFSin(((phase0)&0xffff)/65536.0f*sPI2F)*12000);
    data[1] = sInt(sFSin(((phase1)&0xffff)/65536.0f*sPI2F)*12000);

    phase0 += (mod+(mod/50))/16;
    phase1 += (mod+(mod/50))/16;
//    mod = mod+4;
    if(mod>0x40000) mod = 0x1000;

    data+=2;
  }
}

/****************************************************************************/

/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "base/system.hpp"
#include "vorbisplayer.hpp"
#include "util/taskscheduler.hpp"

/****************************************************************************/

#define STB_VORBIS_NO_CRT
#define STB_VORBIS_NO_PUSHDATA_API
#define STB_VORBIS_HEADER_ONLY
#include "wz4lib/stb_vorbis.inl"

#include "libv2/v2mplayer.h"

/****************************************************************************/

class bRenderer
{
public:
  virtual ~bRenderer() {} 
  virtual void Render(sF32 *buffer, sInt samples)=0;
};

/****************************************************************************/

#if sPLATFORM==sPLAT_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmreg.h>
#include <dsound.h>
extern HWND sHWND; // lol windows

class bSoundOutput
{
public:

  bSoundOutput()
  {
    Thread=0;
    DS=0;
    PrimBuffer=0;
    Buffer=0;
    RenderBuffer=0;
  }

  ~bSoundOutput()
  {
    Exit();
  }

  sBool Init(sInt rate, sInt channels)
  {
#ifndef KSDATAFORMAT_SUBTYPE_PCM
		const static GUID KSDATAFORMAT_SUBTYPE_PCM = {0x00000001,0x0000,0x0010,{0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71}};
#endif

    if FAILED(DirectSoundCreate8(0,&DS,0))
      return sFALSE;

    if FAILED(DS->SetCooperativeLevel(sHWND,DSSCL_PRIORITY))
      return sFALSE;

    // this is our output format.
    WAVEFORMATPCMEX wf;
    wf.Format.cbSize=22;
    wf.Format.wFormatTag=WAVE_FORMAT_EXTENSIBLE;
    wf.Format.nChannels=channels;
    wf.Format.wBitsPerSample=16;
    wf.Format.nSamplesPerSec=rate;
    wf.Format.nBlockAlign=channels*((wf.Format.wBitsPerSample+7)/8);
    wf.Format.nAvgBytesPerSec=wf.Format.nBlockAlign*rate;
    wf.dwChannelMask=(1<<channels)-1; // very space opera
    wf.Samples.wValidBitsPerSample=16;
    wf.SubFormat=KSDATAFORMAT_SUBTYPE_PCM;

    // set up some stuff while we're at it
    BufferSize = sAlign(rate*LATENCY_MS/1000,16);
    BytesPerSample= wf.Format.nBlockAlign;
    RenderBuffer = new sF32[channels*BufferSize];
    LastPlayPos=0;
    Channels=channels;
    Rate=rate;

    // create primary buffer...
    DSBUFFERDESC dsbd;
    sSetMem(&dsbd,0,sizeof(dsbd));
    dsbd.dwSize = sizeof(dsbd);
    dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
    dsbd.dwBufferBytes = 0;
    dsbd.lpwfxFormat = 0;
    if FAILED(DS->CreateSoundBuffer(&dsbd,&PrimBuffer,0))
      return sFALSE;

    // set primary buffer format to prepare DirectSound for what's coming (may fail)
    PrimBuffer->SetFormat((WAVEFORMATEX*)&wf);

    // create secondary buffer
    sSetMem(&dsbd,0,sizeof(dsbd));
    dsbd.dwSize = sizeof(dsbd);
    dsbd.dwFlags = DSBCAPS_GLOBALFOCUS|DSBCAPS_STICKYFOCUS|DSBCAPS_GETCURRENTPOSITION2;
    dsbd.dwBufferBytes = BufferSize*BytesPerSample;
    dsbd.lpwfxFormat = (WAVEFORMATEX*)&wf;
    if FAILED(DS->CreateSoundBuffer(&dsbd,&Buffer,0))
      return sFALSE;

    // now that everything seemed to work, let's switch over to the rendering thread
    Renderers.HintSize(8);
    Thread = new sThread(ThreadProxy,1,0,this);
    return sTRUE;
  }

  void Exit()
  {
    sDelete(Thread);
    sRelease(Buffer);
    sRelease(PrimBuffer);
    sRelease(DS);
    sDeleteArray(RenderBuffer);
    Renderers.Reset();
  }

  void Register(bRenderer* r)
  {
    Lock();
    Renderers.AddTail(r);
    Unlock();
  }

  void Unregister(bRenderer* r)
  {
    Lock();
    Renderers.RemOrder(r);
    Unlock();
  }

  void Lock() { ThreadLock.Lock(); }
  void Unlock() { ThreadLock.Unlock(); }

  sInt GetRemainder()
  {
    Lock();
    DWORD playpos;
    Buffer->GetCurrentPosition(&playpos,0);
    playpos/=BytesPerSample;
    Unlock();
    return (BufferSize+playpos-LastPlayPos)%BufferSize;
  }

  sInt GetLatency() const { return BufferSize; }
  sInt GetChannels() const { return Channels; }
  sInt GetRate() const { return Rate; }

private:

  static const int MAX_RENDERERS=8;
  static const int LATENCY_MS = 500;

  static void ThreadProxy(sThread*,void*p) { reinterpret_cast<bSoundOutput*>(p)->ThreadFunc(); }

  void ThreadFunc()
  {
    sDPrintF(L"sound thread start\n");

    HRESULT hr;
    void *cptr1, *cptr2;
    DWORD clen1, clen2;

    // clear and start the buffer
    Lock();
    hr=Buffer->Lock(0,BufferSize*BytesPerSample,&cptr1,&clen1,&cptr2,&clen2,0);
    if (clen1) sSetMem(cptr1,0,clen1);
    if (clen2) sSetMem(cptr2,0,clen2);
    Buffer->Unlock(cptr1,clen1,cptr2,clen2);
    hr=Buffer->Play(0,0,DSBPLAY_LOOPING);
    Unlock();

    for (;;)
    {
      TickEv.Wait(LATENCY_MS/3);
      if (!Thread->CheckTerminate()) break;

      Lock();

      DWORD playpos;
      hr=Buffer->GetCurrentPosition(&playpos,0);
      playpos/=BytesPerSample;

      sInt todo = (BufferSize+playpos-LastPlayPos)%BufferSize;
      if (todo>=32)
      {
        // render
        sSetMem(RenderBuffer,0,4*Channels*todo);
        bRenderer *r;
        sFORALL(Renderers,r)
        {
          r->Render(RenderBuffer,todo);
        }

        // convert to 16bit and clip to DSound buffer
        hr=Buffer->Lock(BytesPerSample*LastPlayPos,BytesPerSample*todo,&cptr1,&clen1,&cptr2,&clen2,0);
        if (clen1) ClipTo(cptr1,RenderBuffer,clen1/BytesPerSample);
        if (clen2) ClipTo(cptr2,RenderBuffer+Channels*(clen1/BytesPerSample),clen2/BytesPerSample);
        Buffer->Unlock(cptr1,clen1,cptr2,clen2);
      }

      LastPlayPos=playpos;
      Unlock();
    }

    Lock();
    Buffer->Stop();
    Unlock();

    sDPrintF(L"sound thread end\n");
  }

  void ClipTo(void *dest, sF32 *src, sInt samples)
  {
    sS16 *dst=(sS16*)dest;
    const sInt todo=samples*Channels;

    for (sInt i=0; i<todo; i++)
      *dst++=sS16(sClamp(32760.0f**src++,-32760.0f,+32760.0f));  
  }

  sStaticArray<bRenderer*> Renderers;

  sThread *Thread;
  sThreadEvent TickEv;
  sThreadLock ThreadLock;

  struct IDirectSound8 *DS;
  struct IDirectSoundBuffer *PrimBuffer;
  struct IDirectSoundBuffer *Buffer;
  sInt BufferSize; // in samples
  sInt BytesPerSample;
  sF32 *RenderBuffer;
  sInt LastPlayPos;
  sInt Channels;
  sInt Rate;
};


#endif // Windows


/****************************************************************************/

struct bMusicPlayer::VerySecret : protected bRenderer
{
  const sBool Seekable;
  bSoundOutput Output;

  static const sInt VORBIS_MEMSIZE=200*1024; // should do
  static const sInt SEEKCHUNK=1000*1000; // samples

  sU8 VorbisMem[VORBIS_MEMSIZE];
  stb_vorbis_alloc VorbisAlloc;
  stb_vorbis *Vorbis;
  stb_vorbis_info VorbisInfo;

  V2MPlayer *V2;
  sInt V2Length;
  sInt V2Decoded;

  sU8 *Buffer;
  sDInt BufSize;
  sFile *File;

  sBool Playing;
  sBool Loop;
  sInt  SamplePos;
  sInt  ZeroCount;
  volatile sU32 SkipSamples;

  sF32  CurVol;
  sF32  DestVol;
  sF32  VolDelta;

  struct SeekChunk
  {
    sF32 *Buffer; // samples*channels;
    sInt Samples; // max SEEKCHUNK
    SeekChunk *Next;
  };

  SeekChunk SeekBuffer, *CurBuffer;
  sInt      SeekBufferLen;
  sInt      CurBufferPos;

  VerySecret(sBool seekable) : Seekable(seekable), Buffer(0), File(0), Vorbis(0), V2(0), Playing(sFALSE), Loop(sFALSE)
  { 
    sClear(SeekBuffer);
    CurBuffer=0;
  }

  ~VerySecret() 
  { 
    Exit(); 
  }

  void Init(const sChar *filename)
  {
    Exit();
    File = sCreateFile(filename);
    
    const sChar *ext = sFindFileExtension(filename);
    if (!sCmpStringI(ext,L"ogg"))
      InitVorbis(File->MapAll(),File->GetSize());
    else if (!sCmpStringI(ext,L"v2m"))
      InitV2(File->MapAll(),File->GetSize());
  }

  sBool InitCommon(void *ptr, sDInt size)
  {
    if (!ptr || !size) return sFALSE;

    Buffer=(sU8*)ptr;
    BufSize=size;
    
    Playing=sFALSE;
   
    CurVol=1.0f;
    DestVol=1.0f;
    VolDelta=0.0f;
    ZeroCount=0;
    SamplePos=0;
    SkipSamples = 0;
    return sTRUE;
  }

  void InitVorbis(void *ptr, sDInt size)
  {
    if (!InitCommon(ptr,size)) return;

    InitDecoder();

    if (Seekable)
    {
      SeekBuffer.Buffer=0;
      SeekBuffer.Samples=0;
      SeekBuffer.Next=0;
      SeekBufferLen=0;
      CurBuffer=&SeekBuffer;
      CurBufferPos=0;

      sDPrintF(L"rendering vorbis file... ");
      sF32 t1=sF32(sGetTime());
      SeekChunk *cur=&SeekBuffer;
      do
      {
        cur->Next = new SeekChunk;
        cur=cur->Next;
        cur->Buffer = new sF32[SEEKCHUNK*VorbisInfo.channels];
        cur->Samples=stb_vorbis_get_samples_float_interleaved(Vorbis,VorbisInfo.channels,cur->Buffer,VorbisInfo.channels*SEEKCHUNK);
        cur->Next=0;
        SeekBufferLen+=cur->Samples;
      } while (cur->Samples==SEEKCHUNK);
      sDPrintF(L"done, %d samples (%fs) in %f seconds\n",SeekBufferLen,sF32(SeekBufferLen)/sF32(VorbisInfo.sample_rate),(sF32(sGetTime())-t1)/1000.0f);

      stb_vorbis_close(Vorbis);
      Vorbis=0;

      sDelete(File);
    }

    Output.Init(VorbisInfo.sample_rate,VorbisInfo.channels);
    Output.Register(this);
  }


  void InitV2(void *ptr, sDInt size)
  {
    if (!InitCommon(ptr,size)) return;
    
    if (Seekable)
    {
      sDPrintF(L"can't seek in V2M files yet\n");
      Exit();
      return;
    }

    const sInt rate=44100;

    V2 = new V2MPlayer;
    V2->Init();
    
    if (!V2->Open(ptr,rate))
    {
      sDPrintF(L"could not open V2M file\n");
      Exit();
      return;
    }

    sS32 *posarray=0;
    sInt npos=V2->CalcPositions(&posarray);
    if (posarray)
    {
      V2Length = sMulDiv(posarray[2*(npos-1)]+3000,rate,1000);
      delete[] posarray;
    }
    else
      V2Length=0;
    V2Decoded=0;

    V2->Play();

    Output.Init(rate,2);
    Output.Register(this);
  }


  void InitDecoder()
  {
    if (Vorbis)
    {
      stb_vorbis_close(Vorbis);
      Vorbis=0;
    }

    VorbisAlloc.alloc_buffer=(char*)VorbisMem;
    VorbisAlloc.alloc_buffer_length_in_bytes=VORBIS_MEMSIZE;

    sInt error=0;
    Vorbis = stb_vorbis_open_memory(Buffer,BufSize,&error,&VorbisAlloc);
    if (error)
    {
      sDPrintF(L"STB isn't as good as Bob, code: %d\n",error);
      Exit();
      return;
    }

    VorbisInfo = stb_vorbis_get_info(Vorbis);
  }

  void Exit()
  {
    if (!Buffer) return;

    Output.Unregister(this);
    Output.Exit();

    if (Vorbis)
    {
      stb_vorbis_close(Vorbis);
      Vorbis=0;
    }

    if (V2)
    {
      V2->Close();
      sDelete(V2);
    }

    Buffer=0;
    sDelete(File);

    SeekChunk *cur=SeekBuffer.Next;
    while (cur)
    {
      SeekChunk *chunk = cur;
      cur = cur->Next;
      sDelete(chunk->Buffer);
      sDelete(chunk);
    }
  }


  void Play()
  {
    if (!Buffer) return;
    Playing=sTRUE;
  }


  void Pause()
  {
    if (!Buffer) return;
    Playing=sFALSE;
  }

  void RestartDecoder()
  {
      if (Vorbis)
        InitDecoder();
      else if (V2)
      {
        V2->Stop();
        V2->Play();
        V2Decoded=0;
      }
  }

  void Seek(sF32 time)
  {
    if (!Buffer) return;

    Output.Lock();
    if (Seekable)
    {
      sInt pos=sMin(sInt(time*VorbisInfo.sample_rate),SeekBufferLen);
      SamplePos=pos;
      CurBuffer=&SeekBuffer;

      while (pos>CurBuffer->Samples)
      {
        pos-=CurBuffer->Samples;
        CurBuffer=CurBuffer->Next;
      }
      CurBufferPos=pos;
    }
    else if (time<=0.0f)
    {
      RestartDecoder();
      ZeroCount=0;
      SamplePos=0;
    }
    //ZeroCount=0;

    Output.Unlock();
  }


  void SetVolume(sF32 vol, sF32 fadeduration)
  {
    if (!Buffer) return;
    Output.Lock();
    DestVol=vol;
    if (fadeduration>=0.001f)
      VolDelta=(DestVol-CurVol)/(fadeduration*Output.GetRate());
    else
      CurVol=DestVol;
    Output.Unlock();
  }

  sBool IsPlaying()
  {
    return Playing;
  }

  sF32 GetPosition()
  {
    if (!Buffer) return 0;
    sInt samplepos;
    Output.Lock();
    samplepos=SamplePos+ZeroCount+(Playing?Output.GetRemainder():0)-Output.GetLatency();
    Output.Unlock();
    return sF32(sF64(samplepos)/sF32(Output.GetRate()));
  }


  sInt GetSamples(sF32 *buffer, sInt nFloats)
  {
    if (Vorbis)
      return stb_vorbis_get_samples_float_interleaved(Vorbis,VorbisInfo.channels,buffer,nFloats);
    else if (V2)
    {
      if (V2Length && V2Decoded>=V2Length) return 0;
      V2->Render(buffer,nFloats/2);
      V2Decoded+=nFloats/2;
      return nFloats/2;
    }
    else
      return 0;
  }


  void Render(sF32 *buffer, sInt len)
  {
    sInt olen=len;
    sInt rendered=0;
    const sInt ch=Output.GetChannels();
    if (Playing)
    {
      if (Seekable)
      {
        len=sMin(len,SeekBufferLen-SamplePos);
        while (len)
        {
          sInt todo=sMin(len,CurBuffer->Samples-CurBufferPos);
          sCopyMem(buffer+ch*rendered,CurBuffer->Buffer+ch*CurBufferPos,ch*todo*4);
          CurBufferPos+=todo;
          SamplePos+=todo;
          if (CurBufferPos==CurBuffer->Samples)
            if (CurBuffer->Next)
            {
              CurBuffer=CurBuffer->Next;
              CurBufferPos=0;
            }
            else
              break;
          len-=todo;
          rendered+=todo;
        }
      }
      else
      {
        sInt skip = SkipSamples;
        if(skip>0)
        {
          sAtomicAdd(&SkipSamples,-skip);
          static sF32 skipbuffer[4096];
          while(skip>0)
          {
            rendered=GetSamples(skipbuffer,sMin(4096,skip*ch));
            if (!rendered) rendered=skip; // end
            skip -= rendered;
            SamplePos+=rendered;
          }
        }

        rendered=GetSamples(buffer,ch*len);
        if (!rendered && Loop)
        {
          RestartDecoder();
          rendered=GetSamples(buffer,ch*len);
        }
        SamplePos+=rendered;
      }
    }

    const register sF32 dv=DestVol, vd=VolDelta;
    register sF32 cv=CurVol;
    if (cv!=1.0f || dv!=1.0f)
    {
      register sF32 *bp=buffer;
      for (sInt i=0; i<rendered; i++)
      {
        for (sInt c=0; c<ch; c++) *bp++*=cv;
        if (dv>cv) cv=sMin(dv,cv+vd);
        else if (dv<cv) cv=sMax(dv,cv+vd);
      }
      CurVol=cv;
    }

    if (rendered) ZeroCount=0;
    if (Playing) ZeroCount+=olen-rendered;
    //if (ZeroCount>=Output.GetLatency()) Playing=sFALSE;
  }

};

/****************************************************************************/


bMusicPlayer::bMusicPlayer(sBool seekable) { p=new VerySecret(seekable); }
bMusicPlayer::~bMusicPlayer() { delete p; }
void bMusicPlayer::Init(const sChar *filename) { p->Init(filename); }
//void bMusicPlayer::Init(void *ptr, sDInt size) { p->Init(ptr,size);  }
void bMusicPlayer::Exit() { p->Exit(); }
void bMusicPlayer::Play() { p->Play(); }
void bMusicPlayer::Pause() { p->Pause(); }
void bMusicPlayer::Seek(sF32 time) { p->Seek(time); }
void bMusicPlayer::Skip(sInt samples) { sAtomicAdd(&p->SkipSamples,samples); }
void bMusicPlayer::SetVolume(sF32 vol, sF32 fadeduration) { p->SetVolume(vol,fadeduration); }
void bMusicPlayer::SetLoop(sBool loop) { p->Loop=loop; }
sBool bMusicPlayer::IsPlaying() { return p->IsPlaying(); }
sF32 bMusicPlayer::GetPosition() { return p->GetPosition(); }
sBool bMusicPlayer::IsLoaded() { return p->Buffer!=0; }

/****************************************************************************/

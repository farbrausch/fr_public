/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) 2011 Tammo Hinrichs                                  ***/
/***   all rights reserved                                                ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "base/types.hpp"
#include "base/system.hpp"
#include "util/movieplayer.hpp"
#include "util/shaders.hpp"

// require Windows 7
#ifdef WINVER
#undef WINVER
#endif
#define WINVER _WIN32_WINNT_WIN7
#define WIN32_LEAN_AND_MEAN

// This requires Windows SDK v7.0 
// In case of trouble installing it (eg. with Visual C++ Express), check the following registry key:
// HKEY_CURRENT_USER\Software\Microsoft\Microsoft SDKs\Windows
// both CurrentInstallFolder and CurrentVersion should read at least v7.0 - VC++ takes the include dir from there.

#include <mfapi.h>
#if MF_SDK_VERSION < 2
#error This source file requires Windows SDK Ver. 7.0 or higher!
#endif

#include <dsound.h>
extern HWND sHWND; // lol windows

#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <propvarutil.h>
#include <avrt.h>
#pragma comment(lib,"mfplat.lib")
#pragma comment(lib,"mfuuid.lib")
#pragma comment(lib,"mfreadwrite.lib")
#pragma comment(lib,"propsys.lib")
#pragma comment(lib,"avrt.lib")

/****************************************************************************/

// quick hack to make a class support IUnknown if you need to implement a COM interface
template<class Interface> class sFakeCOMObject : public Interface
{
  long __stdcall QueryInterface(const GUID &riid, void **ppvObject)
  {
    *ppvObject=0;
    if (riid==__uuidof(IUnknown) || riid==__uuidof(Interface))
    {
      *ppvObject=this;
      return S_OK;
    }
    return E_NOINTERFACE;
  }

  // do I look like I care?
  unsigned long __stdcall AddRef() { return 1; } 
  unsigned long __stdcall Release(void) { return 1; }
};

static void InitMF();


class sMPSoundOutput
{
public:

  sMPSoundOutput()
  {
    DS=0;
    PrimBuffer=0;
    Buffer=0;
    Format = 0;
    Playing = sFALSE;
    Empty = sTRUE;
  }

  ~sMPSoundOutput()
  {
    Exit();
  }

  sBool Init(const WAVEFORMATEX *wf)
  {
    Format = wf;

    if FAILED(DirectSoundCreate8(0,&DS,0))
      return sFALSE;

    if FAILED(DS->SetCooperativeLevel(sHWND,DSSCL_PRIORITY))
      return sFALSE;

    // set up some stuff while we're at it
    BufferSize = sAlign(Format->nSamplesPerSec*LATENCY_MS/1000,16)*Format->nBlockAlign;
    FillPos=0;
    RefPos=0;
    Playing = sFALSE;
    Empty = sTRUE;

    // set primary buffer format to prepare DirectSound for what's coming (may fail)
    DSBUFFERDESC dsbd;
    sSetMem(&dsbd,0,sizeof(dsbd));
    dsbd.dwSize = sizeof(dsbd);
    dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
    dsbd.dwBufferBytes = 0;
    dsbd.lpwfxFormat = 0;
    if SUCCEEDED(DS->CreateSoundBuffer(&dsbd,&PrimBuffer,0))
    {
      PrimBuffer->SetFormat(Format);
    }

    // create secondary buffer
    sSetMem(&dsbd,0,sizeof(dsbd));
    dsbd.dwSize = sizeof(dsbd);
    dsbd.dwFlags = DSBCAPS_GLOBALFOCUS|DSBCAPS_STICKYFOCUS|
      DSBCAPS_GETCURRENTPOSITION2|DSBCAPS_CTRLVOLUME;
    dsbd.dwBufferBytes = BufferSize;
    dsbd.lpwfxFormat = (WAVEFORMATEX*)Format;
    if FAILED(DS->CreateSoundBuffer(&dsbd,&Buffer,0))
      return sFALSE;

    void *cptr1=0, *cptr2=0;
    DWORD clen1=0, clen2=0;
    Buffer->Lock(0,BufferSize,&cptr1,&clen1,&cptr2,&clen2,0);
    if (clen1) sSetMem(cptr1,0,clen1);
    if (clen2) sSetMem(cptr2,0,clen2);
    Buffer->Unlock(cptr1,clen1,cptr2,clen2);

    return sTRUE;
  }

  void Exit()
  {
    sRelease(Buffer);
    sRelease(PrimBuffer);
    sRelease(DS);
    Format = 0;
  }

  void Lock() { ThreadLock.Lock(); }
  void Unlock() { ThreadLock.Unlock(); }

  void Reset()
  {
    Lock();
    Buffer->Stop();
    Buffer->SetCurrentPosition(0);
    RefPos=0;
    FillPos=0;
    Empty=sTRUE;
    Unlock();
  }

  void Stop()
  {
    Lock();
    Buffer->Stop();
    Playing = sFALSE;
    Unlock();
  }

  void Play()
  {
    Lock();
    Playing = sTRUE;
    if (!Empty)
      Buffer->Play(0,0,DSBPLAY_LOOPING);
    Unlock();
  }

  void Push(const sU8 *data, sInt nBytes, sS64 timeStampSmp)
  {
    if (nBytes<=0) return;

    //sDPrintF(L"p %d %d\n",FillPos/4,timeStampSmp);

    sBool ret=sTRUE;
    Lock();

    RefPos = FillPos;
    RefTimeStamp = timeStampSmp;

    void *cptr1=0, *cptr2=0;
    DWORD clen1=0, clen2=0;
    Buffer->Lock(FillPos,nBytes,&cptr1,&clen1,&cptr2,&clen2,0);
    if (clen1) sCopyMem(cptr1, data, clen1);
    if (clen2) sCopyMem(cptr2, data+clen1, clen2);
    Buffer->Unlock(cptr1,clen1,cptr2,clen2);

    FillPos = (FillPos+nBytes)%BufferSize;
    if (Empty)
    {
      Empty = sFALSE;
      if (Playing)
        Buffer->Play(0,0,DSBPLAY_LOOPING);
    }

    Unlock();
  }

  sF32 GetFill()
  {
    Lock();
    DWORD playpos;
    Buffer->GetCurrentPosition(&playpos,0);
    sF32 fill = float((BufferSize+FillPos-playpos)%BufferSize)/float(BufferSize);
    Unlock();
    return fill<0.75?fill:fill-1;
  }

  sInt GetTimeMS()
  {
    Lock();
    DWORD playpos;
    Buffer->GetCurrentPosition(&playpos,0);
    sInt advance = (BufferSize+playpos-RefPos)%BufferSize;
    Unlock();
    if (advance>=(BufferSize/4)) advance-=BufferSize;
    return sMulDiv(RefTimeStamp+advance/Format->nBlockAlign,1000,Format->nSamplesPerSec);
  }

  sInt GetLatency() const { return BufferSize; }
  sInt GetChannels() const { return Format->nChannels; }
  sInt GetRate() const { return Format->nSamplesPerSec; }

  void SetVolume(sF32 v) 
  { 
    sF32 db = DSBVOLUME_MIN;
    if (v>0.001)
      db = 2000.0f*sFLog(v)/sFLog(10);
    Buffer->SetVolume(sClamp<LONG>(db,DSBVOLUME_MIN,DSBVOLUME_MAX));
  }


private:

  static const int MAX_RENDERERS=8;
  static const int LATENCY_MS = 500;

  sThreadLock ThreadLock;

  struct IDirectSound8 *DS;
  struct IDirectSoundBuffer *PrimBuffer;
  struct IDirectSoundBuffer *Buffer;
  sInt BufferSize; // in bytes
  sInt FillPos;
  sInt RefPos;
  sS64 RefTimeStamp; // in samples
  sBool Playing;
  sBool Empty;

  const WAVEFORMATEX *Format;
};



class sMoviePlayerMF : public sMoviePlayer
{
public:

  sInt Flags; // sMOF_*

  enum MPState
  {
    ST_OFF,
    ST_INITING,
    ST_MFINITED,
    ST_ERROR,
    ST_OK,
  } State;

  sString<sMAXPATH> Filename;
  IMFSourceReader *SourceReader;
  IMFMediaType *VideoType;
  GUID VideoFormat;

  sBool Playing;

  sMovieInfo Info;
  sInt RawStride;

  sMaterial *Mtrl;
  sFRect UVR;

  sMPSoundOutput *Audio;
  IMFMediaType *AudioType;
  WAVEFORMATEX *AudioFormat;
  volatile sBool GettingAudio;
  sF32 Volume;

  IMFSample *NextVideoSample;
  sInt CurSampleTime;  // negative: nothing decoded yet
  sInt NextSampleTime; // only if NextVideoSample!=0

  sInt LastTime;
  sInt CurTime;

  sF32 SeekTo;  // negative: no seek request

  // callback object for catching the SourceReader's async notifications
  struct Callbacks : public sFakeCOMObject<IMFSourceReaderCallback>
  {
    sMoviePlayerMF *Owner;

    Callbacks() : Owner(0) {}

    // called when a frame was decoded
    HRESULT STDMETHODCALLTYPE OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags,
      LONGLONG llTimestamp, IMFSample *pSample)
    {
      if (SUCCEEDED(hrStatus) && pSample)
      {
        IMFMediaType *type=0;
        GUID typeguid;
        Owner->SourceReader->GetCurrentMediaType(dwStreamIndex, &type);
        type->GetGUID(MF_MT_MAJOR_TYPE,&typeguid);
        sRelease(type);
        if (typeguid == MFMediaType_Video)
        {
          Owner->NextSampleTime=llTimestamp/10000;
          Owner->NextVideoSample=pSample;
          Owner->NextVideoSample->AddRef();
        }
        else if (Owner->Audio)
        {
          BYTE *pAudioData = NULL;
          IMFMediaBuffer *pBuffer = NULL;
          DWORD cbBuffer = 0;
          HRESULT hr;

          hr = pSample->ConvertToContiguousBuffer(&pBuffer);
          hr = pBuffer->Lock(&pAudioData, NULL, &cbBuffer);

          sS64 audiotime = llTimestamp*(sS64)Owner->Audio->GetRate()/10000000L;
          Owner->Audio->Push(pAudioData, cbBuffer, audiotime);

          hr = pBuffer->Unlock();
          sRelease(pBuffer);
          Owner->GettingAudio = sFALSE;
        }

        if (Owner->Audio && !Owner->GettingAudio && Owner->Audio->GetFill()<0.5f)
        {
          Owner->DecodeNextAudio();
        }
      }
      else
      {
        Owner->GettingAudio = sFALSE;
        if (SUCCEEDED(hrStatus) && (dwStreamFlags & MF_SOURCE_READERF_ENDOFSTREAM)) // end?
        {
          if (Owner->Flags & sMOF_LOOP)
            Owner->SeekTo=0;
          else
            Owner->Pause();
        }
        else if (FAILED(hrStatus) || (dwStreamFlags & MF_SOURCE_READERF_ERROR))
        {
          // pause on error
          Owner->Pause();
        }
      }
      return S_OK;
    }

    // not used yet
    HRESULT STDMETHODCALLTYPE OnFlush(DWORD dwStreamIndex)
    {
      return S_OK;
    }

    // not used yet
    HRESULT STDMETHODCALLTYPE OnEvent(DWORD dwStreamIndex, IMFMediaEvent *pEvent)
    {
      return S_OK;
    }
  };

  Callbacks CB;

  sMoviePlayerMF() 
  {
    State=ST_OFF;
    SourceReader = 0;
    VideoType = 0;
    AudioType = 0;
    AudioFormat = 0;
    Mtrl = 0;
    CB.Owner=this;
    NextVideoSample=0;
    Playing=sFALSE;
    SeekTo=-1;
    Audio = 0;
    GettingAudio = sFALSE;
    Volume = 1.0f;
  }

  ~sMoviePlayerMF() 
  {
    Exit();
  }

  /****************************************************************************/   
  /***                                                                      ***/
  /*** Simple API (guaranteed to be there)                                  ***/
  /***                                                                      ***/
  /****************************************************************************/   

  // starts playing from the beginning
  void Play()
  {
    if (Playing) SeekTo=0;
    Playing=sTRUE;
    if (Audio) Audio->Play();
  }

  // returns if movie is (still) playing
  sBool IsPlaying()
  {
    return Playing;
  }

  // sets volume. parameter is linear between 0 and 1
  void SetVolume(sF32 volume)
  {
    Volume = sClamp(volume,0.0f,1.0f);
    if (Audio)
      Audio->SetVolume(volume);
  }

  // close and delete the player
  void Release()
  {
    delete this;
  }

  /****************************************************************************/   
  /***                                                                      ***/
  /*** Advanced API (not on all platforms)                                  ***/
  /***                                                                      ***/
  /****************************************************************************/   

  // starts playing or seeks (if applicable), time is in seconds
  void Play(sF32 time) 
  { 
    SeekTo=sClamp(time,0.0f,Info.Length);
    Playing = sTRUE;
    if (Audio) Audio->Play();
  }

  // returns information about the movie
  sMovieInfo GetInfo() 
  { 
    sMovieInfo mi; 
    sClear(mi); 
    if (State==ST_OK) 
      mi = Info;
    else if (State==ST_ERROR)
      mi.XSize=mi.YSize=-1;
    return mi; 
  }

  // returns current movie position in seconds
  sF32 GetTime()
  {
    if (State!=ST_OK) return 0;
    return CurTime/1000.0f;
  }

  // pauses playback (if applicable)
  void Pause()
  {
    Playing=sFALSE;
    if (Audio) Audio->Stop();
  }

  // overrides movie's aspect ratio in case you know better
  void OverrideAspectRatio(sF32 aspect)
  {
    Info.Aspect=aspect;
  }

  /****************************************************************************/   
  /****************************************************************************/   

  // copy data to a texture
  void Copy(sTextureBase *tex, sU8 *&src, sInt spitch, sInt w, sInt h)
  {
    sTexture2D *t2d=tex->CastTex2D();
    sVERIFY(t2d);
    sU8 *dest;
    sInt dpitch;
    t2d->BeginLoad(dest,dpitch);

    if (dpitch==w && spitch==w)
    {
      sCopyMem(dest,src,w*h);
      src+=w*h;
    }
    else
    {
      for (sInt y=0; y<h; y++)
      {
        sCopyMem(dest,src,w);
        src+=spitch;
        dest+=dpitch;
      }
    }

    t2d->EndLoad();
  }


  // fill texture with constant value
  void Fill(sTextureBase *tex, sU8 v, sInt w, sInt h)
  {
    sTexture2D *t2d=tex->CastTex2D();
    sVERIFY(t2d);
    sU8 *dest;
    sInt dpitch;
    t2d->BeginLoad(dest,dpitch);

    if (dpitch==w)
    {
      sSetMem(dest,v,w*h);
    }
    else
    {
      for (sInt y=0; y<h; y++)
      {
        sSetMem(dest,v,w);
        dest+=dpitch;
      }
    }
    t2d->EndLoad();
  }

  // start decoding of next frame
  void DecodeNextVideo()
  {
    sRelease(NextVideoSample);
    HRESULT hr=SourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM,0,0,0,0,0);
    sVERIFY(SUCCEEDED(hr));
  }

  // start decoding of next frame
  void DecodeNextAudio()
  {
    if (!Audio) return;
    sVERIFY(!GettingAudio);

    GettingAudio=true;
    HRESULT hr=SourceReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM,0,0,0,0,0);

    sVERIFY(SUCCEEDED(hr));
  }

  // start decoding of first frame (after open or seek)
  void KickOff()
  {
    CurSampleTime=-1;
    DecodeNextAudio();
    DecodeNextVideo();
  }

  // blit decoded sample into texture(s)
  void BlitSample()
  {
    sVERIFY(NextVideoSample);

    DWORD bc=0;
    NextVideoSample->GetBufferCount(&bc);
    sVERIFY(bc>=1);

    IMFMediaBuffer *buffer=0;
    NextVideoSample->GetBufferByIndex(0,&buffer);
    sVERIFY(buffer);

    BYTE *data;
    DWORD size;
    buffer->Lock(&data,0,&size);

    if (VideoFormat == MFVideoFormat_YV12)
    {
      // if buffer is bigger than expected (1.5* number of pixels) the height
      // was probably padded to 16 lines (h.264 limitation)
      sInt stride = RawStride?RawStride:Info.XSize;
      sInt extralines = ((size*2/3)/stride)-Info.YSize;
      if (extralines>0)
      {
        stride = sAlign(stride,16); // actually, the stride we get from MF is a lie.
        extralines = ((size*2/3)/stride)-Info.YSize;
      }

      // copy to y,v,u texture
      sTexture2D *ytex=Mtrl->Texture[0]->CastTex2D();
      Copy(Mtrl->Texture[0],data,stride,Info.XSize,Info.YSize);
      data+=stride*extralines;
      Copy(Mtrl->Texture[2],data,stride/2,Info.XSize/2,Info.YSize/2);
      data+=stride*extralines/4;
      Copy(Mtrl->Texture[1],data,stride/2,Info.XSize/2,Info.YSize/2);

      UVR.Init(sF32(Info.XSize)/sF32(ytex->SizeX),sF32(Info.YSize)/sF32(ytex->SizeY));
    }
    else if (VideoFormat == MFVideoFormat_RGB32)
    {
      sInt stride = RawStride?RawStride:(Info.XSize*4);

      // copy to RGB texture
      sTexture2D *tex=Mtrl->Texture[0]->CastTex2D();
      Copy(Mtrl->Texture[0],data,stride,Info.XSize*4,Info.YSize);

      UVR.Init(sF32(Info.XSize)/sF32(tex->SizeX),sF32(Info.YSize)/sF32(tex->SizeY));
    }

    buffer->Unlock();
    sRelease(buffer);
    sRelease(NextVideoSample);
  }


  // returns material for painting the current frame yourself. 
  sMaterial * GetFrame(sFRect &uvrect)  
  { 
    switch (State)
    {
    case ST_OFF: 
      return 0;
    case ST_INITING: 
      return 0;
    case ST_ERROR: 
      return 0;
    case ST_MFINITED:
      InitGFX();
      State=ST_OK;
      break;
    default: break;
    }

    sVERIFY(SourceReader);
    sInt time = sGetTime();
    if (!LastTime) LastTime=sGetTime();
    //sDPrintF(L"%5d %5d %5d (%3d)\n",CurTime,CurSampleTime,NextSampleTime,time-LastTime);

    // seek? try if the decoder reacts :)
    if (SeekTo>=0)
    {
      CurTime = 1000*SeekTo;
      PROPVARIANT var;
      InitPropVariantFromInt64((sS64)(SeekTo*10000000.0f),&var);
      HRESULT hr=SourceReader->SetCurrentPosition(GUID_NULL,var);
      if (hr!=MF_E_INVALIDREQUEST)
      {
        if (Audio) Audio->Reset();
        KickOff();
        LastTime=0;
        SeekTo=-1;
      }
    }

    // next frame arrived and time to display?
    if (NextVideoSample && (NextSampleTime<=CurTime || CurSampleTime<0))
    {
      BlitSample();
      if (CurSampleTime<0)
      {
        // on first frame, set real time to first sample time
        CurTime = NextSampleTime+(sInt)(250*(1.0/Info.FPS));
      }
      CurSampleTime=NextSampleTime;
      if (SeekTo<0) DecodeNextVideo();
    }

    // advance time
    if (Playing && CurSampleTime>=0)
    {
      if (Audio)
        CurTime = Audio->GetTimeMS();
      else
        CurTime += time-LastTime;
    }
    LastTime=time;

    uvrect=UVR;
    return Mtrl;
  };

  /****************************************************************************/
  /****************************************************************************/

  sBool InitMF()
  {
    // we want async
    IMFAttributes *attrs=0;
    HRESULT hr=MFCreateAttributes(&attrs,4);
    if (FAILED(hr)) return sFALSE;
    attrs->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK,&CB);
    attrs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS,TRUE);

    // open Source Reader
    // TODO: Wrap sFile into IMFByteStream for packfile goodness :)
    if (FAILED(hr=MFCreateSourceReaderFromURL(Filename, attrs, &SourceReader)))
    {
      sLogF(L"movie",L"can't open source reader for %s (%08x)\n",Filename,(sU32)hr);
      sRelease(attrs);
      return sFALSE;
    }
    sRelease(attrs);

    // select only first video stream (discard audio for now)
    SourceReader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS,FALSE);
    if (FAILED(hr=SourceReader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM,TRUE)))
    {
      sLogF(L"movie",L"no video stream found in %s (%08x)\n",Filename,(sU32)hr);
      return sFALSE;
    }

    sBool hasAudio = !(Flags&sMOF_NOSOUND) && SUCCEEDED(hr=SourceReader->SetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM,TRUE));

    sLogF(L"movie",L"opening %s\n",Filename);

    static const GUID supported[] = 
    {
      MFVideoFormat_YV12,
      MFVideoFormat_RGB32,
      //      MFVideoFormat_IYUV,
      //      MFVideoFormat_YUY2,
      //      MFVideoFormat_UYVY,
      GUID_NULL,
    };

    static const sChar *fmtstr[] = {L"YV12",L"RGB32",L"IYUV",L"YUY2",L"UYVY",};

    // find the first supported video format
    hr = MFCreateMediaType(&VideoType);
    sVERIFY(SUCCEEDED(hr));
    hr = VideoType->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Video);
    sVERIFY(SUCCEEDED(hr));

    const sChar *fmt=0;
    for (sInt i=0;;i++)
    {
      if (supported[i]==GUID_NULL)
      {
        sLogF(L"movie",L"can't find suitable format for %s (%08x)\n",Filename,(sU32)hr);
        return sFALSE;
      }

      VideoFormat = supported[i];
      hr = VideoType->SetGUID(MF_MT_SUBTYPE,VideoFormat);
      sVERIFY(SUCCEEDED(hr));
      if (SUCCEEDED(SourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM,0,VideoType)))
      {
        fmt=fmtstr[i];
        break;      
      }
    }

    // get actual video format and data (size, frame rate, etc)
    sRelease(VideoType);
    SourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM,&VideoType);
    VideoType->GetGUID(MF_MT_SUBTYPE,&VideoFormat);

    UINT32 w,h;
    MFGetAttributeSize(VideoType,MF_MT_FRAME_SIZE,&w,&h);
    Info.XSize=w; Info.YSize=h;

    UINT32 num, den;
    MFGetAttributeRatio(VideoType,MF_MT_PIXEL_ASPECT_RATIO,&num,&den);
    Info.Aspect = (sF32(num)*sF32(w))/(sF32(den)*sF32(h));

    MFGetAttributeRatio(VideoType,MF_MT_FRAME_RATE,&num,&den);
    Info.FPS = sF32(num)/sF32(den);

    RawStride=MFGetAttributeUINT32(VideoType,MF_MT_DEFAULT_STRIDE,0);

    // determine audio format
    if (hasAudio)
    {
      // find the first supported video format
      hr = MFCreateMediaType(&AudioType);
      sVERIFY(SUCCEEDED(hr));
      hr = AudioType->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Audio);
      sVERIFY(SUCCEEDED(hr));
      hr = AudioType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);

      if (FAILED(hr=SourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, AudioType)))
      {
        sRelease(AudioType);
        sLogF(L"movie",L"Non-PCM audio stream in %s (%08x)\n",Filename,(sU32)hr);
        Audio = 0;
        SourceReader->SetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM,FALSE);
      }
      else
      {
        sRelease(AudioType);
        hr=SourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &AudioType);
        sVERIFY(SUCCEEDED(hr));
        UINT32 cbFormat;
        hr = MFCreateWaveFormatExFromMFMediaType(AudioType, &AudioFormat, &cbFormat);
        sVERIFY(SUCCEEDED(hr));
        Audio = new sMPSoundOutput;
        if (!Audio->Init(AudioFormat))
        {
          sLogF(L"movie",L"can't open audio out for %s (%08x)\n",Filename);
          sDelete(Audio);
          SourceReader->SetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM,FALSE);
        }
        if (Audio)
        {
          Audio->SetVolume(Volume);
          if (Playing) Audio->Play();
        }
        GettingAudio = sFALSE;
      }
    }



    Info.Length=0;
    PROPVARIANT var;
    hr=SourceReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE,MF_PD_DURATION,&var);
    if (SUCCEEDED(hr))
    {
      LONGLONG duration;
      PropVariantToInt64(var,&duration);
      PropVariantClear(&var);
      Info.Length=duration/10000000.0f;
    }

    sLogF(L"movie",L"- %s, %dx%d at %f fps, %f seconds\n",fmt,Info.XSize,Info.YSize,Info.FPS,Info.Length); 
    return sTRUE;
  }

  void InitGFX()
  {
    const sInt w=Info.XSize;
    const sInt h=Info.YSize;

    // now create the material (of death)
    if (VideoFormat == MFVideoFormat_YV12)
    {
      // YV12: Y, U and V in three 8bit textures
      Mtrl = new sYUVMaterial;

      Mtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF|sMTRL_VC_COLOR0;
      Mtrl->BlendColor = sMB_ALPHA;
      Mtrl->Texture[0] = new sTexture2D(w,h,sTEX_2D|sTEX_I8|sTEX_NOMIPMAPS|sTEX_DYNAMIC);
      Mtrl->Texture[1] = new sTexture2D(w/2,h/2,sTEX_2D|sTEX_I8|sTEX_NOMIPMAPS|sTEX_DYNAMIC);
      Mtrl->Texture[2] = new sTexture2D(w/2,h/2,sTEX_2D|sTEX_I8|sTEX_NOMIPMAPS|sTEX_DYNAMIC);
      Mtrl->TFlags[0]=Mtrl->TFlags[1]=Mtrl->TFlags[2]=sMTF_CLAMP|sMTF_UV0;
      Mtrl->Prepare(sVertexFormatSingle);

      Fill(Mtrl->Texture[0],16,w,h);
      Fill(Mtrl->Texture[1],128,w/2,h/2);
      Fill(Mtrl->Texture[2],128,w/2,h/2);
    }
    else if (VideoFormat == MFVideoFormat_RGB32)
    {
      // RGB32: simple ARGB8888 texture \o/
      Mtrl = new sSimpleMaterial;

      Mtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF|sMTRL_VC_COLOR0;
      Mtrl->Texture[0] = new sTexture2D(w,h,sTEX_2D|sTEX_ARGB8888|sTEX_NOMIPMAPS|sTEX_DYNAMIC);
      Mtrl->TFlags[0]=sMTF_CLAMP|sMTF_UV0;
      Mtrl->Prepare(sVertexFormatSingle);

      Fill(Mtrl->Texture[0],0,w*4,h);
    }

    State=ST_OK;

    // kick off first frame
    KickOff();
    CurTime = (sInt)(250*(1.0/Info.FPS)); // a bit into the first frame to stabilize timing
  }


  sBool Init(const sChar *filename, sInt flags, sTextureBase *alphatexture)
  {
    Exit();
    Filename = filename;
    Flags = flags;

    State=ST_INITING;
    Playing=(Flags&sMOF_DONTSTART)?0:1;
    LastTime=0;
    SeekTo=-1;

    if (InitMF())
      State=ST_MFINITED;
    else
      State=ST_ERROR;

    return State==ST_MFINITED;
  }


  void Exit()
  {
    State=ST_OFF;
    Playing=0;
    if (AudioFormat) CoTaskMemFree(AudioFormat);
    sRelease(NextVideoSample);
    sRelease(VideoType);
    sRelease(AudioType);
    sRelease(SourceReader);
    if (Mtrl) for (sInt i=0; i<sMTRL_MAXTEX; i++) sDelete(Mtrl->Texture[i]);
    sDelete(Mtrl);
    sDelete(Audio);
  }

};

/****************************************************************************/

sMoviePlayer * sCreateMoviePlayer(const sChar *filename, sInt flags, sTextureBase *alphatexture)
{
  InitMF();
  sMoviePlayerMF *mp = new sMoviePlayerMF;
  if (mp && !mp->Init(filename,flags,alphatexture)) sDelete(mp);
  return mp;
}

static HANDLE AvTaskHandle=INVALID_HANDLE_VALUE;
static bool MFActive = 0;

static void InitMF()
{
  if (!MFActive)
  {
    MFStartup(MF_VERSION);
    MFActive = sTRUE;
  }
}

static void ExitMF()
{
  if (MFActive)
  {
    MFShutdown();
    MFActive = sFALSE;
  }
}

static void DeviceLostHook(void*)
{
  ExitMF();
}

static void InitMFSys()
{
  // while we're at it, let the scheduler know we're doing something important :)
  DWORD taskindex = 0;
  AvTaskHandle = AvSetMmThreadCharacteristics(L"Playback", &taskindex);

  sGraphicsLostHook->Add(DeviceLostHook);
  InitMF();
}

static void ExitMFSys()
{
  sGraphicsLostHook->Rem(DeviceLostHook);
  ExitMF();

  if (AvTaskHandle != INVALID_HANDLE_VALUE)
    AvRevertMmThreadCharacteristics(AvTaskHandle);
}

sADDSUBSYSTEM(MediaFoundation,0xe0,InitMF,ExitMF);

/****************************************************************************/

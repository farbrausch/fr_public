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

// This requires Windows SDK v7.0 
// In case of trouble installing it (eg. with Visual C++ Express), check the following registry key:
// HKEY_CURRENT_USER\Software\Microsoft\Microsoft SDKs\Windows
// both CurrentInstallFolder and CurrentVersion should read at least v7.0 - VC++ takes the include dir from there.

#include <mfapi.h>
#if MF_SDK_VERSION < 2
#error This source file requires Windows SDK Ver. 7.0 or higher!
#endif

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

  IMFSample *NextSample;
  sInt CurSampleTime;  // negative: nothing decoded yet
  sInt NextSampleTime; // only if NextSample!=0

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
        Owner->NextSampleTime=llTimestamp/10000;
        Owner->NextSample=pSample;
        Owner->NextSample->AddRef();
      }
      else
      {
        if (SUCCEEDED(hrStatus)) // end?
        {
          if (Owner->Flags & sMOF_LOOP)
            Owner->SeekTo=0;
          else
            Owner->Playing=sFALSE;
        }
        else
        {
          // pause on error
          Owner->Playing=sFALSE;
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
    Mtrl = 0;
    CB.Owner=this;
    NextSample=0;
    Playing=sFALSE;
    SeekTo=-1;
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
  }

  // returns if movie is (still) playing
  sBool IsPlaying()
  {
    return Playing;
  }

  // sets volume. parameter is linear between 0 and 1
  void SetVolume(sF32 volume)
  {
    // someone should implement audio :)
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
  void DecodeNext()
  {
    sRelease(NextSample);
    HRESULT hr=SourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM,0,0,0,0,0);
    sVERIFY(SUCCEEDED(hr));
  }

  // start decoding of first frame (after open or seek)
  void KickOff()
  {
    CurSampleTime=-1;
    DecodeNext();
  }

  // blit decoded sample into texture(s)
  void BlitSample()
  {
    sVERIFY(NextSample);
    
    DWORD bc=0;
    NextSample->GetBufferCount(&bc);
    sVERIFY(bc>=1);

    IMFMediaBuffer *buffer=0;
    NextSample->GetBufferByIndex(0,&buffer);
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
    sRelease(NextSample);
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
        KickOff();
        LastTime=0;
        SeekTo=-1;
      }
    }

    // next frame arrived and time to display?
    if (NextSample && (NextSampleTime<=CurTime || CurSampleTime<0))
    {
      BlitSample();
      if (CurSampleTime<0)
      {
        // on first frame, set real time to first sample time
        CurTime = NextSampleTime+(sInt)(250*(1.0/Info.FPS));
      }
      CurSampleTime=NextSampleTime;
      if (SeekTo<0) DecodeNext();
    }

    // advance time
    if (Playing && CurSampleTime>=0)
      CurTime += time-LastTime;
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
    sRelease(NextSample);
    sRelease(VideoType);
    sRelease(SourceReader);
    if (Mtrl) for (sInt i=0; i<sMTRL_MAXTEX; i++) sDelete(Mtrl->Texture[i]);
    sDelete(Mtrl);
  }

};

/****************************************************************************/

sMoviePlayer * sCreateMoviePlayer(const sChar *filename, sInt flags, sTextureBase *alphatexture)
{
  sMoviePlayerMF *mp = new sMoviePlayerMF;
  if (mp && !mp->Init(filename,flags,alphatexture)) sDelete(mp);
  return mp;
}

static HANDLE AvTaskHandle=INVALID_HANDLE_VALUE;

static void InitMF()
{
  // while we're at it, let the scheduler know we're doing something important :)
  DWORD taskindex=0;
  AvTaskHandle=AvSetMmThreadCharacteristics(L"Playback",&taskindex);

  MFStartup(MF_VERSION);
}

static void ExitMF()
{
  if (AvTaskHandle != INVALID_HANDLE_VALUE)
    AvRevertMmThreadCharacteristics(AvTaskHandle);

  MFShutdown();
}

sADDSUBSYSTEM(MediaFoundation,0xe0,InitMF,ExitMF);

/****************************************************************************/

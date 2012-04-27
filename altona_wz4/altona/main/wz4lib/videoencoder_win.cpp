/****************************************************************************/

#include "videoencoder.hpp"
#if sPLATFORM==sPLAT_WINDOWS

#include "base/system.hpp"
#include "base/graphics.hpp"
#include "util/image.hpp"

/****************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vfw.h>


#pragma comment(lib, "vfw32.lib")

class AVIVideoEncoderVFW : public sVideoEncoder
{
  sThreadLock *Lock;

  sBool OK;

  PAVIFILE File;
  PAVISTREAM Vid,VidC;
  PAVISTREAM Aud;
  WAVEFORMATEX targetFormat;

  CodecInfo *Params;
  sBool MyParams;

  sString<512> BaseName;
  int Segment;
  LONG OverflowCounter;

  bool Initialized;
  bool FormatSet;  
  int SizeX,SizeY;
  int Frame;
  int AudioSample,AudioBytesPerSample;
  int FPSNum,FPSDenom;

  void Init();
  void Cleanup();
  void StartEncode();
  void StartAudioEncode();
  void WriteFrame(const sU8 *buffer);
  void WriteFrame(const sU32 *buffer, sInt sx, sInt sy, sInt pitch);

public:
  AVIVideoEncoderVFW(const sChar *name,int FPSNum,int FPSDenom,CodecInfo *info);
  ~AVIVideoEncoderVFW();

  sBool IsOK() const { return Initialized; }

  void SetSize(int SizeX,int SizeY);
  void WriteFrame(const sImage *img);
  void WriteFrame(sTexture2D *RT);

  void SetAudioFormat(sInt bits, sInt channels, sInt rate);
  void WriteAudioFrame(const void *buffer,int samples);
};


void AVIVideoEncoderVFW::Init()
{
  AVISTREAMINFO asi;
  AVICOMPRESSOPTIONS aco;
  bool error = true;

  AVIFileInit();
  OverflowCounter = 0;

  // create avi File
  sString<512> name;
  name.PrintF(L"%s.%02d.avi",BaseName,Segment);

  if(AVIFileOpen(&File,name,OF_CREATE|OF_WRITE,NULL) != AVIERR_OK)
  {
    sDPrintF(L"avi_vfw: AVIFileOpen failed\n");
    goto cleanup;
  }

  // initialize video stream header
  ZeroMemory(&asi,sizeof(asi));
  asi.fccType               = streamtypeVIDEO;
  asi.dwScale               = FPSDenom;
  asi.dwRate                = FPSNum;
  asi.dwSuggestedBufferSize = SizeX * SizeY * 3;
  SetRect(&asi.rcFrame,0,0,SizeX,SizeY);
  sCopyString(asi.szName,"Video",63);

  // create video stream
  if(AVIFileCreateStream(File,&Vid,&asi) != AVIERR_OK)
  {
    sDPrintF(L"avi_vfw: AVIFileCreateStream (video) failed\n");
    goto cleanup;
  }

  // create compressed stream
  unsigned long Codec = Params->FourCC;
  if(!Codec)
    Codec = mmioFOURCC('D','I','B',' '); // uncompressed frames

  ZeroMemory(&aco,sizeof(aco));
  aco.fccType = streamtypeVIDEO;
  aco.fccHandler = Codec;
  aco.dwQuality = Params->Quality;
  if (!Params->CodecData.IsEmpty())
  {
    aco.lpParms = &Params->CodecData[0];
    aco.cbParms = Params->CodecData.GetCount();
  }

  if(AVIMakeCompressedStream(&VidC,Vid,&aco,0) != AVIERR_OK)
  {
    sDPrintF(L"avi_vfw: AVIMakeCompressedStream (video) failed\n");
    goto cleanup;
  }

  error = false;
  Initialized = true;

cleanup:
  if(error)
  {
    OK=sFALSE;
    Cleanup();
  }
}

void AVIVideoEncoderVFW::Cleanup()
{
  sScopeLock lock(Lock);

  if(Initialized)
  {
    sDPrintF(L"avi_vfw: stopped recording\n");

    if(Aud)
    {
      AVIStreamRelease(Aud);
      Aud = 0;
    }

    if(VidC)
    {
      AVIStreamRelease(VidC);
      VidC = 0;
    }

    if(Vid)
    {
      AVIStreamRelease(Vid);
      Vid = 0;
    }

    if(File)
    {
      AVIFileRelease(File);
      File = 0;
    }

    AVIFileExit();
    sDPrintF(L"avi_vfw: avifile shutdown complete\n");
    Initialized = false;
  }
}

void AVIVideoEncoderVFW::StartEncode()
{
  BITMAPINFOHEADER bmi;
  bool error = true;

  if(!File)
    return;

  {
    sScopeLock lock(Lock);

    // set stream format
    ZeroMemory(&bmi,sizeof(bmi));
    bmi.biSize        = sizeof(bmi);
    bmi.biWidth       = SizeX;
    bmi.biHeight      = SizeY;
    bmi.biPlanes      = 1;
    bmi.biBitCount    = 24;
    bmi.biCompression = BI_RGB;
    bmi.biSizeImage   = SizeX * SizeY * 3;
    if(AVIStreamSetFormat(VidC,0,&bmi,sizeof(bmi)) == AVIERR_OK)
    {
      error = false;
      sDPrintF(L"avi_vfw: opened video stream at %.3f fps (%d/%d)\n",1.0f*FPSNum/FPSDenom,FPSNum,FPSDenom);
      Frame = 0;
      FormatSet = true;
    }
    else
      sDPrintF(L"avi_vfw: AVIStreamSetFormat (video) failed\n");
  }

  if(error)
    Cleanup();
}

void AVIVideoEncoderVFW::StartAudioEncode()
{
  AVISTREAMINFO asi;
  bool error = true;

  if(!File)
    return;

  sScopeLock lock(Lock);

  // initialize stream info
  ZeroMemory(&asi,sizeof(asi));
  asi.fccType               = streamtypeAUDIO;
  asi.dwScale               = 1;
  asi.dwRate                = targetFormat.nSamplesPerSec;
  asi.dwSuggestedBufferSize = targetFormat.nAvgBytesPerSec;
  asi.dwSampleSize          = targetFormat.nBlockAlign;
  sCopyString(asi.szName,L"Audio",64);

  // create the stream
  if(AVIFileCreateStream(File,&Aud,&asi) != AVIERR_OK)
  {
    sDPrintF(L"avi_vfw: AVIFileCreateStream (audio) failed\n");
    goto cleanup;
  }

  // set format
  if(AVIStreamSetFormat(Aud,0,(LPVOID) &targetFormat,sizeof(WAVEFORMATEX)+targetFormat.cbSize) != AVIERR_OK)
  {
    sDPrintF(L"avi_vfw: AVIStreamSetFormat (audio) failed\n");
    goto cleanup;
  }

  error = false;
  sDPrintF(L"avi_vfw: opened audio stream at %d hz, %d channels, %d bits\n",
    (sU32)targetFormat.nSamplesPerSec, (sU32)targetFormat.nChannels, (sU32)targetFormat.wBitsPerSample);
  AudioSample = 0;
  AudioBytesPerSample = targetFormat.nBlockAlign;

  // fill already written frames with no sound
  int fillBytesSample = targetFormat.nBlockAlign;
  sU8 *buffer = new sU8[fillBytesSample * 1024];
  int sampleFill = MulDiv(Frame,targetFormat.nSamplesPerSec * FPSDenom,FPSNum);

  memset(buffer,0,fillBytesSample * 1024);
  for(int samplePos=0;samplePos<sampleFill;samplePos+=1024)
    WriteAudioFrame(buffer,min(sampleFill-samplePos,1024));

  delete[] buffer;

cleanup:

  if(error)
    Cleanup();
}

AVIVideoEncoderVFW::AVIVideoEncoderVFW(const sChar *name,int _fpsNum,int _fpsDenom,CodecInfo *info)
{
  Initialized = false;
  FormatSet = false;

  Lock = new sThreadLock();

  SizeX = SizeY = 0;
  Frame = 0;
  AudioSample = 0;
  FPSNum = _fpsNum;
  FPSDenom = _fpsDenom;

  File = 0;
  Vid = 0;
  VidC = 0;
  Aud = 0;
  
  if (info)
  {
    Params=info;
    MyParams=sFALSE;
  }
  else
  {
    Params = new CodecInfo;
    MyParams = sTRUE;
    sChooseVideoCodec(*Params);
    if (!Params->FourCC) return;
  }

  // determine File base name
  BaseName=name;
  for(int i=(int) sGetStringLen(BaseName)-1;i>=0;i--)
  {
    if(BaseName[i] == '/' || BaseName[i] == '\\' || BaseName[i] == ':')
      break;
    else if(BaseName[i] == '.')
    {
      BaseName[i] = 0;
      break;
    }
  }
  Segment = 1;

  sClear(targetFormat);

  Init();
}

AVIVideoEncoderVFW::~AVIVideoEncoderVFW()
{
  Cleanup();

  sDelete(Lock);
  if (MyParams) sDelete(Params);
}

void AVIVideoEncoderVFW::SetSize(int _xRes,int _yRes)
{
  SizeX = _xRes;
  SizeY = _yRes;
}

void AVIVideoEncoderVFW::WriteFrame(const sU8 *buffer)
{
  // encode the Frame
  sScopeLock lock(Lock);

  if(!Frame && !FormatSet && SizeX && SizeY)
    StartEncode();

  if(FormatSet && VidC)
  {
    if(OverflowCounter >= 1024*1024*1800)
    {
      bool gotAudio = Aud != 0;

      sDPrintF(L"avi_vfw: Segment %d reached maximum size, creating next Segment.\n",Segment);
      Cleanup();
      Segment++;
      Init();

      StartEncode();
      if(gotAudio)
        StartAudioEncode();
    }

    LONG written = 0;
    AVIStreamWrite(VidC,Frame,1,(void *)buffer,3*SizeX*SizeY,0,0,&written);
    Frame++;
    OverflowCounter += written;
  }
}

void AVIVideoEncoderVFW::WriteFrame(const sU32 *buffer, sInt sx, sInt sy, sInt pitch)
{  
  sInt bpr=sAlign(24*sx/8,4);
  pitch/=4;

  sU8 *dest=(sU8*)sAllocMem(bpr*sy,16,sAMF_ALT);

  sU8 *ptr=dest;
  sU8 *src=0;
  for(sInt y=sy-1;y>=0;y--)
  {
    src=(sU8*)(buffer+y*pitch);
    for(sInt x=0;x<sx;x++)
    {
      ptr[x*3+0] = src[0];
      ptr[x*3+1] = src[1];
      ptr[x*3+2] = src[2];
      src+=4;
    }
    ptr+=bpr;
  }

  SetSize(sx,sy);
  WriteFrame(dest);

  sFreeMem(dest);
}

void AVIVideoEncoderVFW::WriteFrame(const sImage *img)
{
  sVERIFY(img);
  WriteFrame(img->Data,img->SizeX,img->SizeY,img->SizeX*4);
}


void AVIVideoEncoderVFW::WriteFrame(sTexture2D *rt)
{
  const sU8 *data;
  sS32 pitch;
  sTextureFlags flags;
  sInt xs,ys;

  xs = rt->SizeX;
  ys = rt->SizeY;

  sBeginReadTexture(data,pitch,flags,rt);
  if(flags==sTEX_ARGB8888)
  {
    WriteFrame((const sU32*)data,xs,ys,pitch);
  }
  else
    sDPrintF(L"AVI write: wrong RT format (%d)\n",flags&sTEX_FORMAT);

  sEndReadTexture();
}


void AVIVideoEncoderVFW::SetAudioFormat(sInt bits, sInt channels, sInt rate)
{
  WAVEFORMATEX wfx;

  sClear(wfx);
  wfx.wFormatTag=WAVE_FORMAT_PCM;
  wfx.wBitsPerSample=bits;
  wfx.nChannels=channels;
  wfx.nSamplesPerSec=rate;
  wfx.nBlockAlign=wfx.wBitsPerSample*wfx.nChannels/8;
  wfx.nAvgBytesPerSec=wfx.nSamplesPerSec*wfx.nBlockAlign;

  sBool reinit=Aud && sCmpMem(&wfx,&targetFormat,sizeof(WAVEFORMATEX));

  if (reinit)
  {
    Cleanup();
    Segment++;
  }

  targetFormat=wfx;

  if (reinit)
  {
    Init();
  }
}

void AVIVideoEncoderVFW::WriteAudioFrame(const void *buffer,int samples)
{
  sScopeLock lock(Lock);

  if(!Aud)
    StartAudioEncode();

  if(Aud)
  {
    if(samples)
    {
      LONG written = 0;
      AVIStreamWrite(Aud,AudioSample,samples,(LPVOID)buffer,
        samples*AudioBytesPerSample,0,0,&written);
      AudioSample += samples;
      OverflowCounter += written;
    }
  }
}

/****************************************************************************/

extern HWND sHWND;

void sChooseVideoCodec(sVideoEncoder::CodecInfo &info, sU32 basefourcc)
{
  COMPVARS cv;

  info.Quality=0;
  info.FourCC=0;
  info.CodecData.Reset();

  sClear(cv);

  cv.cbSize = sizeof(cv);
  cv.dwFlags = ICMF_COMPVARS_VALID;
  cv.fccType = ICTYPE_VIDEO;
  cv.fccHandler = basefourcc;
  cv.lQ = 0;
  sPreventPaint();
  if(ICCompressorChoose(sHWND,0,0,0,&cv,0))
  {
    info.FourCC=cv.fccHandler;
    info.Quality=cv.lQ;

    if (cv.cbState)
    {
      info.CodecData.HintSize(cv.cbState);
      sU8 *data=info.CodecData.AddMany(cv.cbState);
      sCopyMem(data,cv.lpState,cv.cbState);
    }

    ICCompressorFree(&cv);
  }
}

sVideoEncoder *sCreateVideoEncoder(const sChar *filename, sF32 fps, sVideoEncoder::CodecInfo *info)
{
  AVIVideoEncoderVFW *enc=new AVIVideoEncoderVFW(filename,sInt(fps*1000),1000,info);
  if (enc->IsOK())
    return enc;
  delete enc;
  return 0;
}


#endif
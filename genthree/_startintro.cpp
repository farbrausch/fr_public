// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "_types.hpp"
#include "_start.hpp"
#include "_startdx.hpp"
#if sLINK_UTIL
#include "_util.hpp"                  // for sPerfMon->Flip()
#endif
#include "genplayer.hpp"
#if sINTRO

#define WINVER 0x500
#define DIRECTINPUT_VERSION 0x0800

#include <windows.h>
#include <d3d9.h>
#include <dinput.h>
#include <olectl.h>
#include <dsound.h>
#include <crtdbg.h>
#include <malloc.h>
#include <float.h>

#define WINZERO(x) {sSetMem(&x,0,sizeof(x));}
#define WINSET(x) {sSetMem(&x,0,sizeof(x));x.dwSize = sizeof(x);}
#define RELEASE(x) {if(x)x->Release();x=0;}
#if sRELEASE
#define DXERROR(hr) hr;
#define DXERRHARD(hr) {if(FAILED(hr))sSystem->Abort(0);}
#else
#define DXERROR(hr) {if(FAILED(hr))sFatal("dxerror");}
#define DXERRHARD(hr) {if(FAILED(hr))sFatal("dxerror");}
#endif
#define THEFVF (D3DFVF_XYZ|D3DFVF_COLOR|D3DFVF_TEX2)

#undef DeleteFile
#undef GetCurrentDirectory

#pragma comment(lib,"opengl32.lib")
#pragma comment(lib,"winmm.lib")

sMAKEZONE(FlipLock,"FlipLock",0xffff0000);

#define sZBUFFERXS 1024
#define sZBUFFERYS 1024

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Globals                                                            ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

class sBroker_ *sBroker;

HMODULE d3dlib; 
HMODULE dslib;

typedef IDirect3D9* (WINAPI * Direct3DCreate9T)(UINT SDKVersion);
typedef HRESULT (WINAPI * DirectSoundCreate8T)(LPCGUID lpcGuidDevice,LPDIRECTSOUND8 * ppDS8,LPUNKNOWN pUnkOuter);

static IDirectSoundBuffer8 *DXSBuffer;
static sInt Initialized;

sSystem_ *sSystem;

/****************************************************************************/

#define MAX_KEYQUEUE    16
#define MAX_MOUSEQUEUE  64

static sU32 KeyBuffer[MAX_KEYBUFFER];
static sInt KeyIndex;
static sU32 KeyQual;
static sU32 MouseX;
static sU32 MouseY;
static sU32 MouseZ;
static sU32 MouseButtons;
static sU32 MouseButtonsSave;

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Memory                                                             ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#if !_DEBUG
void * __cdecl malloc(unsigned int size)
{
	return HeapAlloc(GetProcessHeap(),HEAP_NO_SERIALIZE,size);
}

void __cdecl free(void *ptr)
{
	HeapFree(GetProcessHeap(),HEAP_NO_SERIALIZE,ptr);
}
#endif

void * __cdecl operator new(unsigned int size)
{
	return malloc(size);
}

void * __cdecl operator new[](unsigned int size)
{
	return malloc(size);
}

void __cdecl operator delete(void *ptr)
{
	if(ptr)
		free(ptr);
}

void __cdecl operator delete[](void *ptr)
{
	if(ptr)
		free(ptr);
}

int __cdecl _purecall()
{
	return 0;
}

#if !_DEBUG
extern "C" int _fltused;
int _fltused;
#endif

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   System Initialisation                                              ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

static LRESULT WINAPI MainWndProc(HWND win,UINT msg,WPARAM wparam,LPARAM lparam)
{
  static sInt DXKillCursor;

  switch(msg)
  {
  case WM_PAINT:
    if(!DXKillCursor)
    {
      ShowCursor(0);
      SetCursorPos(0,0);
      DXKillCursor = 1;
    }
    if(Initialized)
      sSystem->Render();
    return 0;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  case WM_CHAR:
    if(wparam!=27)
      return 0;
  case WM_CLOSE:
    if(DXSBuffer)
      DXSBuffer->Stop();
    DestroyWindow(win);
    return 0;
  default:
    return DefWindowProc(win,msg,wparam,lparam);
  }
}

/****************************************************************************/

#if _DEBUG
int APIENTRY WinMain(HINSTANCE inst,HINSTANCE prev,LPSTR cmdline,int show)
{
#else
void WinMainCRTStartup()
{
#endif

  sSystem = new sSystem_;
  sSetMem(sSystem,0,sizeof(sSystem_));
  sSystem->Run();
  delete sSystem;

#if _DEBUG
  return 0;
#else
	ExitProcess(0);
#endif
}

/****************************************************************************/

void sSystem_::Run()
{
  HINSTANCE WInst;
  WNDCLASS wc;
  MSG msg;

// set up memory checking

#if !sRELEASE
  _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)|_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF);
  sChar *test = new sChar[16];
  sCopyString(test,"TestLeak",16);
#endif

// find dlls

  d3dlib = ::LoadLibraryA("d3d9.dll");
  dslib = ::LoadLibraryA("dsound.dll");

  if(d3dlib==0 || dslib==0)
  {
    MessageBox(0,"directx 9 needed",0,MB_OK);
    return;
  }
  timeBeginPeriod(1);

// set up some more stuff

  sBroker = new sBroker_;

// create window class

  WInst = GetModuleHandle(0);

#if !sRELEASE
  RECT wr;

  wc.lpszClassName    = "kk";
  wc.lpfnWndProc      = MainWndProc;
  wc.style            = CS_OWNDC|CS_VREDRAW|CS_HREDRAW;
  wc.hInstance        = WInst;
  wc.hIcon            = LoadIcon(WInst,MAKEINTRESOURCE(101));
  wc.hCursor          = LoadCursor(0,IDC_ARROW);
  wc.hbrBackground    = 0;
  wc.lpszMenuName     = 0;
  wc.cbClsExtra       = 0;
  wc.cbWndExtra       = 0;
  RegisterClass(&wc);

  wr.left = 0;
  wr.right = sSCREENX;
  wr.top = 0;
  wr.bottom = sSCREENY;
  AdjustWindowRectEx(&wr,WS_OVERLAPPEDWINDOW|WS_VISIBLE,0,0);

  Screen.Window = (sU32) CreateWindowEx(
    0,
    "kk",
    sAPPNAME " v" sVERSION,
    WS_OVERLAPPEDWINDOW|WS_VISIBLE,
    0,0,wr.right-wr.left,wr.bottom-wr.top,
    0,
    0,
    WInst,
    0);
#else
  sSetMem(&wc,0,sizeof(wc));
  wc.lpszClassName    = "kk";
  wc.lpfnWndProc      = MainWndProc;
  wc.hInstance        = WInst;
  wc.hbrBackground    = (HBRUSH) GetStockObject(NULL_BRUSH);
  RegisterClass(&wc);

  Screen.Window = (sU32) CreateWindowEx(
    WS_EX_TOPMOST,
    "kk",
    "kk",
    WS_POPUP,
    0,0,sSCREENX,sSCREENY,
    0,
    0,
    WInst,
    0);
#endif
// init

  InitScreens();

#if !sINTRO
  _control87(_PC_24|_RC_DOWN,MCW_PC|MCW_RC);
#else
	__asm
	{
		push		0143fh;
		fldcw		[esp];
		pop			eax;
	}
#endif

  sAppHandler(sAPPCODE_INIT,0);

#if sUSE_DIRECTSOUND
  InitDS();
#endif

  // main loop

  Initialized = 1;

  while(GetMessage(&msg,NULL,0,0)) 
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
/*
  msg.message = WM_QUIT;
  do
  {
    gotmsg = (PeekMessage(&msg,0,0,0,PM_REMOVE)!=0);     // !=0) not needed

    if(gotmsg)
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    else
    {
      for(i=0;i<KeyIndex;i++)
        sAppHandler(sAPPCODE_KEY,KeyBuffer[i]);
      KeyIndex = 0;

      Render();
#if sUSE_DIRECTSOUND
      FillDS();
#endif
    }
  }
  while(msg.message!=WM_QUIT);
*/


// cleanup

#if sUSE_DIRECTSOUND
  ExitDS();
#endif
  sAppHandler(sAPPCODE_EXIT,0);
  ExitScreens();

#if sLINK_UTIL
  sBroker->RemRoot(sPerfMon);
#endif
  sBroker->Free();
  delete sBroker;
  sBroker = 0;

#if !sRELEASE
  FreeLibrary(d3dlib);
#if sUSE_DIRECTSOUND
  FreeLibrary(dslib);
#endif
#endif
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   System Implementation                                              ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/***                                                                      ***/
/***   Init/Exit/Debug                                                    ***/
/***                                                                      ***/
/****************************************************************************/

void sSystem_::Log(sChar *s)
{
#if !sINTRO || !sRELEASE
  if(!sAppHandler(sAPPCODE_DEBUGPRINT,(sU32) s))
    OutputDebugString(s);
#endif
}

/****************************************************************************/

sNORETURN void sSystem_::Abort(sChar *msg)
{
#if sINTRO
  if(!sAppHandler(sAPPCODE_DEBUGPRINT,(sU32) "test\n"))   // nimm diese zeile raus und der code funktioniert nicht mehr mit DX9 Debug Libs / Maximum Validation.
    OutputDebugString("test\n");
  ExitProcess(0);
#else
  WAborting = sTRUE;
  if(DXD)
    DXD->Release();
  _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)&~(_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF));
  if(msg)
    MessageBox(0,msg,"Fatal Error",MB_OK|MB_TASKMODAL|MB_SETFOREGROUND|MB_TOPMOST|MB_ICONERROR);
  ExitProcess(0);
//  exit(0);
#endif
}

/****************************************************************************/

void sSystem_::Exit()
{
  PostQuitMessage(0);
}

#if !sRELEASE

sU32 sSystem_::GetTime()
{
  return timeGetTime();
}

#endif

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Sound                                                              ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#if sUSE_DIRECTSOUND

static IDirectSound8  *DXS;
//static IDirectSoundBuffer8 *DXSBuffer;
static IDirectSoundBuffer *DXSPrimary;
static sInt DXSHandlerSample;
static sInt DXSLastFrameTime;
static sInt DXSOldTime,DXSOldSample;

#define DXSSAMPLES 0x18000  // 2 sekunden

static sInt DXSWriteSample;
static sInt DXSReadSample;
static sInt DXSLastTotalSample;

static HANDLE DXSThread;
static ULONG DXSThreadId;
static sInt volatile DXSRun;
static CRITICAL_SECTION DXSLock;
static HANDLE DXSEvent;
static sInt DXSCurrentSample;

unsigned long __stdcall ThreadCode(void *);
void IntroRenderHandler(sS16 *stream,sInt samples);

/****************************************************************************/

#if sINTRO
static const GUID IID_IDirectSoundBuffer8 = { 0x6825a449, 0x7524, 0x4d82, 0x92, 0x0f, 0x50, 0xe3, 0x6a, 0xb3, 0xab, 0x1e };
#endif


/****************************************************************************/
/***                                                                      ***/
/***   Sound                                                              ***/
/***                                                                      ***/
/****************************************************************************/

unsigned long __stdcall ThreadCode(void *);

void sSystem_::InitDS()
{
  DWORD count1,count2;
  void *pos1,*pos2;
  HRESULT hr;
  DSBUFFERDESC dsbd;
  WAVEFORMATEX format;
  IDirectSoundBuffer *sbuffer;
  DirectSoundCreate8T DirectSoundCreate8P;

// init DXS

  DirectSoundCreate8P = (DirectSoundCreate8T) GetProcAddress(dslib,"DirectSoundCreate8");
  sVERIFY(DirectSoundCreate8P)

  hr = (*DirectSoundCreate8P)(0,&DXS,0);
  sVERIFY(!FAILED(hr));

  hr = DXS->SetCooperativeLevel((HWND)sSystem->Screen.Window,DSSCL_PRIORITY);
  sVERIFY(!FAILED(hr));


  WINSET(dsbd);
  dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
  dsbd.dwBufferBytes = 0;
  dsbd.lpwfxFormat = 0;
  hr = DXS->CreateSoundBuffer(&dsbd,&DXSPrimary,0);
  sVERIFY(!FAILED(hr));

  WINZERO(format);
  format.nAvgBytesPerSec = 44100*4;
  format.nBlockAlign = 4;
  format.nChannels = 2;
  format.nSamplesPerSec = 44100;
  format.wBitsPerSample = 16;
  format.wFormatTag = WAVE_FORMAT_PCM;
  DXSPrimary->SetFormat(&format);


  WINSET(dsbd);
  dsbd.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS/* | DSBCAPS_STICKYFOCUS*/;
  dsbd.dwBufferBytes = DXSSAMPLES*4;
  dsbd.lpwfxFormat = &format;

  hr = DXS->CreateSoundBuffer(&dsbd,&sbuffer,0);
  sVERIFY(!FAILED(hr));
  hr = sbuffer->QueryInterface(IID_IDirectSoundBuffer8,(void**)&DXSBuffer);
  sbuffer->Release();
  sVERIFY(!FAILED(hr));

  hr = DXSBuffer->Lock(0,DXSSAMPLES*4,&pos1,&count1,&pos2,&count2,0);
  if(!FAILED(hr))
  {
    sSetMem(pos1,0,count1);
    sSetMem(pos2,0,count2);
    DXSBuffer->Unlock(pos1,count1,pos2,count2);
  }

  DXSBuffer->Play(0,0,DSBPLAY_LOOPING);
}

/****************************************************************************/

void sSystem_::ExitDS()
{
  if(DXSBuffer)
    DXSBuffer->Stop();
#if !sINTROX
  if(DXSBuffer)
    DXSBuffer->Release();
  if(DXSPrimary)
    DXSPrimary->Release();
#endif
  if(DXS)
    DXS->Release();
  DXSBuffer = 0;
  DXS = 0;
}

/****************************************************************************/
/*
#if sLINK_GUI
void sSystem_::SetVolume(sInt vol)
{
  DXSBuffer->SetVolume(-sMulShift(0x10000-vol,10000));
}
#endif
*/
/****************************************************************************/

void DSTime()
{
  DWORD play,write;
  HRESULT hr;
  sInt delta,sample;

  hr = DXSBuffer->GetCurrentPosition(&play,&write);
  if(!FAILED(hr))
  {
    delta = play-DXSReadSample;
    if(delta<0)
      delta+=DXSSAMPLES*4;
    if(delta>0 /*&& delta<0x10000*/)                           // 0x10000 = 3680ms = 2.7 fps
      DXSLastTotalSample =DXSWriteSample + delta/4;
  }

  if(DXSOldTime==0)
  {
    DXSOldTime = timeGetTime();
    DXSOldSample = DXSLastTotalSample-DXSSAMPLES;
    sample = 0;
  }
  else
  {
    sample = (timeGetTime()-DXSOldTime);
    sample = sample*44 + sample/10;
    if(sample+DXSOldSample-10 > DXSLastTotalSample-DXSSAMPLES)
      DXSOldSample--;
    if(sample+DXSOldSample+10 < DXSLastTotalSample-DXSSAMPLES)
      DXSOldSample++;
  }
  DXSCurrentSample = sample + DXSOldSample;//DXSLastTotalSample-DXSSAMPLES;
}

/****************************************************************************/

void sSystem_::FillDS()
{
  HRESULT hr;
  sInt play,write;
  DWORD count1,count2;
  void *pos1,*pos2;
  static sInt index=0;
//  sSoundStreamInfo info;
  const sInt SAMPLESIZE = 4;
  sInt size,pplay;

#if !sINTRO 
//  if(!SoundRoot)
//    return;
#endif

  if(!DXSBuffer)
    return;
  hr = DXSBuffer->GetCurrentPosition((DWORD*)&play,(DWORD*)&write);
//  sDPrintF("samples:  hr %08x i:%06x p:%06x w:%06x \n",hr,index,play,write);
  if(!FAILED(hr))
  {
    pplay = play;
    play = play/SAMPLESIZE;
    write = write/SAMPLESIZE;
    if(index>play)
      play+=DXSSAMPLES;
    size = play-index;


    size = (size)&(~63);
//    sDPrintF("samples i:%06x p:%06x w:%06x -> %04x : ",index,play,write,size);
    if(size>0)
    {
      count1 = 0;
      count2 = 0;
      hr = DXSBuffer->Lock(index*SAMPLESIZE,size*SAMPLESIZE,&pos1,&count1,&pos2,&count2,0);
      if(!FAILED(hr))
      {
        index += size;
        if(index>=DXSSAMPLES)
          index-=DXSSAMPLES;

        DXSWriteSample += size;
        DXSReadSample = pplay&(~63);
        sVERIFY((sInt)(count1+count2)==(size*4));
      
        if(count1>0)
          IntroRenderHandler((sS16 *)pos1,count1/SAMPLESIZE);
        if(count2>0)
          IntroRenderHandler((sS16 *)pos2,count2/SAMPLESIZE);

        DXSBuffer->Unlock(pos1,count1,pos2,count2);
      }
    }   
  }
}

/****************************************************************************/
/*
unsigned long __stdcall ThreadCode(void *)
{
  HRESULT hr;
  sInt play,write;
  DWORD count1,count2;
  void *pos1,*pos2;
  static sInt index=0;
//  sSoundStreamInfo info;
  const sInt SAMPLESIZE = 4;
  sInt size,pplay;

#if !sINTRO 
//  if(!SoundRoot)
//    return;
#endif

  while(DXSRun==0)
  {
    WaitForSingleObject(DXSEvent,(DXSSAMPLES*1000/44100)/4);
    EnterCriticalSection(&DXSLock);
    hr = DXSBuffer->GetCurrentPosition((DWORD*)&play,(DWORD*)&write);
    if(!FAILED(hr))
    {
      pplay = play;
      play = play/SAMPLESIZE;
      write = write/SAMPLESIZE;
      if(index>play)
        play+=DXSSAMPLES;
      size = play-index;


      size = (size)&(~63);
  //    sDPrintF("samples i:%06x p:%06x w:%06x -> %04x : ",index,play,write,size);
      if(size>0)
      {
        count1 = 0;
        count2 = 0;
        hr = DXSBuffer->Lock(index*SAMPLESIZE,size*SAMPLESIZE,&pos1,&count1,&pos2,&count2,0);
        if(!FAILED(hr))
        {
          index += size;
          if(index>=DXSSAMPLES)
            index-=DXSSAMPLES;

          DXSWriteSample += size;
          DXSReadSample = pplay&(~63);
          sVERIFY((sInt)(count1+count2)==(size*4));
        

          if(SoundRootFilter)
          {
            if(count1>0)
              SoundRootFilter->Handler(pos1,count1/SAMPLESIZE,0);
            if(count2>0)
              SoundRootFilter->Handler(pos2,count2/SAMPLESIZE,1);
          }
          DXSBuffer->Unlock(pos1,count1,pos2,count2);
        }
      }   
    }
    LeaveCriticalSection(&DXSLock);
  }
  DXSRun = sFALSE;
  return 0;
}
*/

//**** old sound system


/*

void sSystem_::InitDS()
{
  DWORD count1,count2;
  void *pos1,*pos2;
  DSBUFFERDESC dsbd;
  WAVEFORMATEX format;
  IDirectSoundBuffer *sbuffer;
  DirectSoundCreate8T DirectSoundCreate8P;

// init DXS

  DirectSoundCreate8P = (DirectSoundCreate8T) GetProcAddress(dslib,"DirectSoundCreate8");
  sVERIFY(DirectSoundCreate8P);
  DXERROR((*DirectSoundCreate8P)(0,&DXS,0));

  DXERROR(DXS->SetCooperativeLevel((HWND)Screen.Window,DSSCL_PRIORITY));

  WINSET(dsbd);
  dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
  DXERRHARD(DXS->CreateSoundBuffer(&dsbd,&DXSPrimary,0));

  format.wFormatTag = WAVE_FORMAT_PCM;
  format.nChannels = 2;
  format.nSamplesPerSec = 44100;
  format.nAvgBytesPerSec = 44100*4;
  format.nBlockAlign = 4;
  format.wBitsPerSample = 16;
  DXSPrimary->SetFormat(&format);

  dsbd.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
  dsbd.dwBufferBytes = DXSSAMPLES*4;
  dsbd.lpwfxFormat = &format;
  DXERRHARD(DXS->CreateSoundBuffer(&dsbd,&sbuffer,0));

  DXERROR(sbuffer->QueryInterface(IID_IDirectSoundBuffer8,(void**)&DXSBuffer));
  sbuffer->Release();
  sbuffer = DXSBuffer;

  DXERROR(sbuffer->Lock(0,DXSSAMPLES*4,&pos1,&count1,&pos2,&count2,0));
  sSetMem(pos1,0,count1);
  sSetMem(pos2,0,count2);
  sbuffer->Unlock(pos1,count1,pos2,count2);

// initialize multithreading

  InitializeCriticalSection(&DXSLock);
  DXSEvent = CreateEvent(0,0,0,0);
  DXSRun = 0;
  DXSThread = CreateThread(0,16384,ThreadCode,0,0,&DXSThreadId);

  sVERIFY(DXSEvent)
  sVERIFY(DXSThread)

// start buffer

  sbuffer->Play(0,0,DSBPLAY_LOOPING);
}

// ***************************************************************************

void sSystem_::ExitDS()
{
//  if(DXSBuffer)
    DXSBuffer->Stop();

//  if(DXSThread)
  {
    DXSRun = 1;
    while(DXSRun==1) 
      Sleep(10);

#if !sRELEASE
    CloseHandle(DXSThread);
    CloseHandle(DXSEvent);
    DeleteCriticalSection(&DXSLock);
#endif
  }

//  if(DXSBuffer)
    DXSBuffer->Release();
//  if(DXSPrimary)
    DXSPrimary->Release();
//  if(DXS)
    DXS->Release();
}

// ****************************************************************************

void sSystem_::FillDS()
{
  DWORD play,write;
  HRESULT hr;
  sInt delta;

  SetEvent(DXSEvent);

  EnterCriticalSection(&DXSLock);
  hr = DXSBuffer->GetCurrentPosition(&play,&write);
  if(!FAILED(hr))
  {
    delta = play-DXSReadSample;
    if(delta<0)
      delta+=DXSSAMPLES*4;
    if(delta>0) // && delta<0x10000)                           // 0x10000 = 3680ms = 2.7 fps
      DXSLastTotalSample = DXSWriteSample + delta/4;
  }
  LeaveCriticalSection(&DXSLock);
//  sDPrintF("%08x:%08x=%08x+%d\n",GetTime(),DXSLastTotalSample,DXSWriteSample,delta);

  DXSLastTotalSample-=DXSSAMPLES;
}

// ****************************************************************************

void IntroRenderHandler(sS16 *stream,sInt samples);
extern GenPlayer *CurrentGenPlayer;

unsigned long __stdcall ThreadCode(void *)
{
  HRESULT hr;
  sInt play,write;
  DWORD count1,count2;
  void *pos1,*pos2;
  static sInt index=0;
  const sInt SAMPLESIZE = 4;
  sInt size,pplay;

  while(DXSRun==0)
  {
    WaitForSingleObject(DXSEvent,(DXSSAMPLES*1000/44100)/4);
    EnterCriticalSection(&DXSLock);

    hr = DXSBuffer->GetCurrentPosition((DWORD*)&play,(DWORD*)&write);
    if(!FAILED(hr))
    {
      pplay = play;
      play = play/SAMPLESIZE;
      write = write/SAMPLESIZE;
      if(index>play)
        play+=DXSSAMPLES;
      size = play-index;

      size = (size)&(~(64-1));
//      sDPrintF("samples i:%06x p:%06x w:%06x -> %04x : \n",index,play,write,size);
      if(size>0)
      {
        count1 = 0;
        count2 = 0;
        hr = DXSBuffer->Lock(index*SAMPLESIZE,size*SAMPLESIZE,&pos1,&count1,&pos2,&count2,0);
        if(!FAILED(hr))
        {
          index += size;
          if(index>=DXSSAMPLES)
            index-=DXSSAMPLES;

          DXSWriteSample += size;
          DXSReadSample = pplay&(~(64-1));
          sVERIFY((sInt)(count1+count2)==(size*4));

          if(count1>0)
            IntroRenderHandler((sS16 *)pos1,count1/SAMPLESIZE);
          if(count2>0)
            IntroRenderHandler((sS16 *)pos2,count2/SAMPLESIZE);

          DXSBuffer->Unlock(pos1,count1,pos2,count2);
        }
      }   
    }
    LeaveCriticalSection(&DXSLock);
  }
  DXSRun = sFALSE;
  return 0;
}
*/


#endif

/****************************************************************************/
/****************************************************************************/

sInt sSystem_::GetCurrentSample()
{
  return DXSCurrentSample;
}

/****************************************************************************/
/***                                                                      ***/
/***   Host Font                                                          ***/
/***                                                                      ***/
/****************************************************************************/

sU32 *sSystem_::RenderFont(sInt bmx,sInt bmy,sChar *fontname,sChar *text,sInt x,sInt y,sInt width,sInt height)
{
  HDC GDIScreenDC;
  HDC GDIDC;
  BITMAPINFO bmi;
  HBITMAP hbm;
  LOGFONT lf;
  HFONT font;
  sU32 *data;

// init structures

  data = new sU32[bmx*bmy];
  sSetMem(data,0,bmx*bmy*4);

  sSetMem(&bmi,0,sizeof(bmi));
  sSetMem(&lf,0,sizeof(lf));

  bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
  bmi.bmiHeader.biWidth = bmx;
  bmi.bmiHeader.biHeight = -bmy;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  lf.lfHeight         = -height; 
  lf.lfWidth          = width; 
  lf.lfCharSet        = DEFAULT_CHARSET; 
  lf.lfOutPrecision   = OUT_TT_PRECIS; 
  lf.lfQuality        = ANTIALIASED_QUALITY;
  sCopyMem(lf.lfFaceName,fontname,32);

// Init

  GDIScreenDC = GetDC(0);
  GDIDC = CreateCompatibleDC(GDIScreenDC);

  hbm = CreateCompatibleBitmap(GDIScreenDC,bmx,bmy);
  SelectObject(GDIDC,hbm);

  font = CreateFontIndirect(&lf);
  SetBkMode(GDIDC,TRANSPARENT);
  SelectObject(GDIDC,font);
  SetTextColor(GDIDC,0xffffff);

// draw

  SetDIBits(GDIDC,hbm,0,bmy,data,&bmi,DIB_RGB_COLORS);
  ExtTextOut(GDIDC,x,y,0,0,text,sGetStringLen(text),0);
  GetDIBits(GDIDC,hbm,0,bmy,data,&bmi,DIB_RGB_COLORS);

// exit gdi

  DeleteObject(font);
  DeleteDC(GDIDC);
  ReleaseDC(0,GDIScreenDC);

  return data;
}

/****************************************************************************/
/***                                                                      ***/
/***   Graphics                                                           ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/


sInt sSystem_::AddTexture(const sTexInfo &ti)
{
  sHardTex *tex;
  sInt i;

  i = NextTexture++;
  sVERIFY(i<MAX_TEXTURE);

  tex = &Textures[i];

  tex->XSize = ti.XSize;
  tex->YSize = ti.YSize;
  tex->Flags = ti.Flags|sTIF_ALLOCATED;
  tex->Format = ti.Format;
  tex->Tex = 0;

	if(tex->Flags & sTIF_RENDERTARGET)
	{
    if(tex->XSize==0)
    {
      tex->XSize = Screen.XSize;
      tex->YSize = Screen.YSize;
    }
    sVERIFY(tex->XSize<=sZBUFFERXS);
    sVERIFY(tex->YSize<=sZBUFFERYS);
#if LOGGING
    sDPrintF("Create Rendertarget %dx%d\n",tex->XSize,tex->YSize);
#endif
		DXERRHARD(DXDev->CreateTexture(tex->XSize,tex->YSize,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&tex->Tex,0));
	}
	else
	{
#if LOGGING
    sDPrintF("Create Texture %dx%d\n",tex->XSize,tex->YSize);
#endif
		DXERRHARD(DXDev->CreateTexture(tex->XSize,tex->YSize,0,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&tex->Tex,0));
    UpdateTexture(i,ti.Bitmap);
	}
  return i;
}

/****************************************************************************/

void sSystem_::UpdateTexture(sInt handle,sBitmap *bm)
{
  sHardTex *tex;
  sInt xs,ys,level;
  D3DLOCKED_RECT dxlock;

  sInt x,y;
  sInt bpr,oxs;
  sU32 *d,*s,*data;
  sU32 *db,*sb;


  tex = &Textures[handle];

  sVERIFY(bm->XSize == tex->XSize);
  sVERIFY(bm->YSize == tex->YSize);

	if(!(tex->Flags & sTIF_RENDERTARGET))
	{
		xs = tex->XSize;
		ys = tex->YSize;
		oxs = xs;
		level = 0;

		data = new sU32[xs*ys];
		sCopyMem4(data,bm->Data,xs*ys);

		for(;;)
		{
      DXERROR(tex->Tex->LockRect(level,&dxlock,0,0));
      bpr = dxlock.Pitch;
      d = (sU32 *)dxlock.pBits;
			s = data; 
			for(y=0;y<ys;y++)
			{
				sCopyMem4(d,s,xs);
				d+=bpr/4;
				s+=xs;
			}
      DXERROR(tex->Tex->UnlockRect(level));

			if(xs<=1 || ys<=1)
				break;
	    
			xs=xs>>1;
			ys=ys>>1;
			sb = data;
			db = data;
			for(y=0;y<ys;y++)
			{
				for(x=0;x<xs;x++)
				{
//          *db = *sb;

          *db = ((sb[     0]&0xfcfcfcfc)>>2)+
                ((sb[     1]&0xfcfcfcfc)>>2)+
                ((sb[xs*2+0]&0xfcfcfcfc)>>2)+
                ((sb[xs*2+1]&0xfcfcfcfc)>>2);
					db+=1;
					sb+=2;
				}
				sb+=xs*2;
			}

			level++;
		}

	  delete[] data;
	}
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Helper Structures                                                  ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

// assume all FVF's are 32 bytes

sBool sGeoBuffer::AllocXB(sInt count,sInt shift,sInt &firstp)
{
  count = count<<shift;
  if(Used+count>Size)
    return sFALSE;
  firstp = Used;
  Used+=count;
  UserCount++;
  return sTRUE;
}

void sTexInfo::Init(sBitmap *bm,sInt format,sU32 flags)
{
  XSize = bm->XSize;
  YSize = bm->YSize;
  Bitmap = bm;
  Flags = flags;
  Format = format;
}

/****************************************************************************/
/***                                                                      ***/
/***   Viewport                                                           ***/
/***                                                                      ***/
/****************************************************************************/

void sViewport::Init(sInt screen)
{
  Screen = 0;
  RenderTarget = -1;
  Window.Init(0,0,sSystem->Screen.XSize,sSystem->Screen.YSize);
  ClearFlags = sVCF_ALL;
  ClearColor.Color = 0xff000000;
}

/****************************************************************************/
/*
void sViewport::InitTex(sInt handle)
{
  Screen = -1;
  RenderTarget = handle;
  Window.Init();
  ClearFlags = sVCF_ALL;
  ClearColor.Color = 0xff000000;
}
*/
/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   DirectX Startup                                                    ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

void sSystem_::InitScreens()
{
  HRESULT hr;
  D3DPRESENT_PARAMETERS d3dpp;
  sInt i;
  sU32 create;
  sU16 *iblock;
  struct IDirect3D9 *DXD;
  Direct3DCreate9T Direct3DCreate9P;

  sREGZONE(FlipLock);

  WINZERO(d3dpp);
  d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
//  d3dpp.EnableAutoDepthStencil = sFALSE;
//  d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
  d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
  d3dpp.BackBufferCount = 1;
  d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
  d3dpp.Flags = 0;
  d3dpp.hDeviceWindow = (HWND) Screen.Window;

#if sRELEASE
  d3dpp.Windowed = FALSE;
  d3dpp.BackBufferWidth = sSCREENX;
  d3dpp.BackBufferHeight = sSCREENY;
  Screen.XSize = sSCREENX;
  Screen.YSize = sSCREENY;
#else
  RECT r;
  d3dpp.Windowed = TRUE;
  GetClientRect((HWND) Screen.Window,&r);
  Screen.XSize = r.right-r.left;
  Screen.YSize = r.bottom-r.top;
#endif

  Direct3DCreate9P = (Direct3DCreate9T) GetProcAddress(d3dlib,"Direct3DCreate9");
  DXD = (*Direct3DCreate9P)(D3D_SDK_VERSION);
  sVERIFY(DXD);
  create = D3DCREATE_HARDWARE_VERTEXPROCESSING;
  do
  {
    hr = DXD->CreateDevice(0,D3DDEVTYPE_HAL,(HWND) Screen.Window,create,&d3dpp,&DXDev);
    if(FAILED(hr) && create==D3DCREATE_SOFTWARE_VERTEXPROCESSING)
      Abort(0);//sFatal("could not create screen");
    create = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
  }
  while(FAILED(hr)); 
  DXD->Release();

	DXERROR(DXDev->CreateDepthStencilSurface(sZBUFFERXS,sZBUFFERYS,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,TRUE,&ZBuffer,0));
  DXERROR(DXDev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&BackBuffer));
	DXERROR(DXDev->SetDepthStencilSurface(ZBuffer));
  DXDev->SetRenderState(D3DRS_ZENABLE,D3DZB_TRUE);
  DXERROR(DXDev->CreateOffscreenPlainSurface(16,16,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&DXBlockSurface[0],0));
  DXERROR(DXDev->CreateOffscreenPlainSurface(16,16,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&DXBlockSurface[1],0));

// init buffer management

  CreateGeoBuffer(0,1,1);
  CreateGeoBuffer(1,1,0);
  CreateGeoBuffer(2,0,0);
  GeoThisVertex = 3;
  CreateGeoBuffer(GeoThisVertex,0,1);
  GeoThisIndex = MAX_GEOBUFFER-1;
  CreateGeoBuffer(GeoThisIndex,0,0);
  DXERROR(GeoBuffer[2].IB->Lock(0,MAX_DYNIBSIZE,(void **) &iblock,0));
  for(i=0;i<MAX_DYNIBSIZE/12;i++)
    sQuad(iblock,i*4+0,i*4+1,i*4+2,i*4+3);
  DXERROR(GeoBuffer[2].IB->Unlock());

// set some defaults
/*
  StdTexTransMat.Init();
  StdTexTransMat.i.x = 0.5f;
  StdTexTransMat.j.y = 0.5f;
  StdTexTransMat.k.z = 0.5f;
  StdTexTransMat.l.x = 0.5f;
  StdTexTransMat.l.y = 0.5f;
  StdTexTransMat.l.z = 0.5f;*/
  ShowCursor(0);
  NextTexture = 0;
}

/****************************************************************************/

void sSystem_::ExitScreens()
{
  sInt i;

  for(i=0;i<MAX_TEXTURE;i++)
    if(Textures[i].Tex)
      Textures[i].Tex->Release();

  for(i=0;i<MAX_GEOBUFFER;i++)
    if(GeoBuffer[i].VB)
      GeoBuffer[i].VB->Release(); 

  DXDev->Release();
}

/****************************************************************************/

void sSystem_::Render()
{
  HRESULT hr;
  sInt i;

  RecalcTransform = sTRUE;
  LastUnit.Init();
  LastCamera.Init();
  LastTransform.Init();
  LastMatrix.Init();

//  for(i=0;i<8;i++)
//    StdTexTransMatSet[i] = 0;

  FillDS();

  sAppHandler(sAPPCODE_PAINT,0);

  {
    static sInt dblock;
    D3DLOCKED_RECT lr;
    volatile sInt dummy;

    DXERROR(DXDev->ColorFill(DXBlockSurface[dblock],0,0xff002050));

    dblock = 1-dblock;

    DXERROR(DXBlockSurface[dblock]->LockRect(&lr,0,D3DLOCK_READONLY));
    dummy = *(sInt *)lr.pBits;
    DXERROR(DXBlockSurface[dblock]->UnlockRect());
	}

  hr = DXDev->Present(0,0,0,0);

  DSTime();

  if(hr==D3DERR_DEVICELOST || sSystem->GetCurrentSample() > ((0x1af*60+15)*44100/132))
    PostQuitMessage(0);

  i = 0;
  do
  {
    GeoHandle[i].DiscardCount = 0;
    i++;
  }
  while(i<MAX_GEOHANDLE); 

  DiscardCount = 1;
  GeoBuffer[0].Used = GeoBuffer[0].Size;
  GeoBuffer[1].Used = GeoBuffer[1].Size;
}

/****************************************************************************/

void ProgressBar(sInt done)
{
  sSystem->Progress(done);
}

static sInt progress;
void sSystem_::Progress(sInt done)
{
  static sInt lasttime;
  const int maxprog = 1913;
  const int step = 3;
  sInt time;
  sRect r;
  sInt key;

  progress++;
  time = timeGetTime();
  if(time > lasttime+250 || done)
  {
    lasttime = time;
    DXDev->Clear(0,0,D3DCLEAR_TARGET,0xff000000,0,0);

    r.Init(20+step*0,sSCREENY/2-20+step*0,sSCREENX-20-step*0,sSCREENY/2+20-step*0);
    DXDev->ColorFill(BackBuffer,(RECT *)&r,0xffffffff);
    r.Init(20+step*1,sSCREENY/2-20+step*1,sSCREENX-20-step*1,sSCREENY/2+20-step*1);
    DXDev->ColorFill(BackBuffer,(RECT *)&r,0xff000000);
    r.Init(20+step*2,sSCREENY/2-20+step*2,sSCREENX-20-step*2,sSCREENY/2+20-step*2);
    r.x1 = r.x0 + 1 + sMin(maxprog,progress)*(r.x1-r.x0-1)/maxprog;
    DXDev->ColorFill(BackBuffer,(RECT *)&r,0xffffffff);

    DXDev->Present(0,0,0,0);

    key = GetAsyncKeyState(VK_ESCAPE);
    if(key&0x8000)
    {
      DXDev->Release(); 
      ExitProcess(0);
    }
  }
}



/****************************************************************************/
/****************************************************************************/

void sSystem_::BeginViewport(const sViewport &vp)
{
  sInt dxs,dys;
	sHardTex *tex;
  IDirect3DSurface9 *backbuffer;
  const static sU8 clear[4] = 
  { 
    0,
    D3DCLEAR_TARGET,
    D3DCLEAR_ZBUFFER/*|D3DCLEAR_STENCIL*/,
    D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER/*|D3DCLEAR_STENCIL*/
  };

	if(vp.Screen!=-1)
	{
		dxs = Screen.XSize;
		dys = Screen.YSize;
    backbuffer = BackBuffer;
    backbuffer->AddRef();
	}
	else
	{
		sVERIFY(vp.RenderTarget >= 0);
		tex = &Textures[vp.RenderTarget];
		sVERIFY(tex->Flags & sTIF_RENDERTARGET);

		dxs = tex->XSize;
		dys = tex->YSize;

		DXERROR(tex->Tex->GetSurfaceLevel(0,&backbuffer));
	}
  DXERROR(DXDev->SetRenderTarget(0,backbuffer));
	backbuffer->Release();

  DView.Width = dxs;
	DView.Height = dys;
	DView.MaxZ = 1.0f;

	DXDev->SetViewport((D3DVIEWPORT9 *)&DView);
  
  if(vp.ClearFlags)
    DXDev->Clear(0,0,clear[vp.ClearFlags],vp.ClearColor.Color,1.0f,0);

  DXERRHARD(DXDev->BeginScene());
}

void sSystem_::EndViewport()
{
  DXERROR(DXDev->EndScene());
  ClearLights();
}

/****************************************************************************/

void sSystem_::SetCamera(const sCamera &cam)
{
  sF32 ratio,q;
  sMatrix mat;

  ratio = cam.AspectX/cam.AspectY;
  q = cam.FarClip/(cam.FarClip-cam.NearClip);
  LastCamera = cam.CamPos;
//  mat = cam.CamPos;
//  mat.TransR();
  LastCamera.TransR();// = mat;
  DXDev->SetTransform(D3DTS_VIEW,(D3DMATRIX*)&LastCamera);

  mat.Init();
  mat.i.x = cam.ZoomX;
  mat.j.y = cam.ZoomY*ratio;
  mat.k.x = cam.CenterX;
  mat.k.y = cam.CenterY;
  mat.k.z = q;
  mat.k.w = 1;
  mat.l.z = -q*cam.NearClip;
  mat.l.w = 0;
  /*
  mat.i.Init(cam.ZoomX    ,0              , 0             ,0);
  mat.j.Init(0            ,cam.ZoomY*ratio, 0             ,0);
  mat.k.Init(cam.CenterX  ,cam.CenterY    , q             ,1);
  mat.l.Init(0            ,0              ,-q*cam.NearClip,0);
  */
  DXDev->SetTransform(D3DTS_PROJECTION,(D3DMATRIX*)&mat);
}

void sSystem_::SetCameraOrtho()
{
  sMatrix mat;

  LastCamera.Init();
  LastMatrix.Init();
  LastTransform.Init();
  DXDev->SetTransform(D3DTS_VIEW,(D3DMATRIX*)&LastTransform);
  DXDev->SetTransform(D3DTS_WORLD,(D3DMATRIX *)&LastTransform);

  mat.Init();
  mat.i.x = 2.0f/DView.Width;
  mat.j.y = -2.0f/DView.Height;
  mat.l.x = -1;
  mat.l.y = 1;
  /*
  mat.i.Init(2.0f/DView.Width ,0                  ,0 ,0);
  mat.j.Init(0                ,-2.0f/DView.Height ,0 ,0);
  mat.k.Init(0                ,0                  ,1 ,0);
  mat.l.Init(-1               ,1                  ,0 ,1);

  */
  DXDev->SetTransform(D3DTS_PROJECTION,(D3DMATRIX*)&mat);
}

void sSystem_::SetMatrix(sMatrix &mat)
{
  LastMatrix = mat;
  LastTransform.Mul4(LastMatrix,LastCamera);
  DXDev->SetTransform(D3DTS_WORLD,(D3DMATRIX *) &mat);
}

void sSystem_::GetTransform(sInt mode,sMatrix &mat)
{
  sVERIFY(mode>=0 && mode<4);
  mat = (&LastUnit)[mode];
}


/****************************************************************************/
/****************************************************************************/

void sSystem_::SetTexture(sInt stage,sInt handle,sMatrix *mat)
{
  sHardTex *tex;
  IDirect3DTexture9 *mst;

  if(handle==sINVALID)
  {
    DXDev->SetTexture(stage,0);
    return;
  }

  tex = &Textures[handle];
  mst = tex->Tex;
  DXDev->SetTexture(stage,mst);
  if(mat)
    DXDev->SetTransform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0+stage),(D3DMATRIX *) mat);
}

/****************************************************************************/
/****************************************************************************/

void sSystem_::SetStates(sU32 *stream)
{
//  sVERIFY(*stream!=0xffffffff);
  while(*stream!=0xffffffff)
  {
    SetState(stream[0],stream[1]);
    stream+=2;
  }

/*
  sVERIFY(*stream!=0xffffffff);
  do
  {
    SetState(stream[0],stream[1]);
    stream+=2;
  }
  while(*stream!=0xffffffff);
*/
}

/****************************************************************************/

void sSystem_::SetState(sU32 token,sU32 value)
{
  sVERIFY(token<0x300);
  if(token<0x0100)
  {
    DXDev->SetRenderState((enum _D3DRENDERSTATETYPE)token,value);
  }
  else if(token<0x0200)
  {
    DXDev->SetTextureStageState(((token>>5)&7),(enum _D3DTEXTURESTAGESTATETYPE)(token&31),value);
  }
  else /* if(token<0x0300)*/
  {
    DXDev->SetSamplerState(((token>>4)&15),(enum _D3DSAMPLERSTATETYPE)(token&15),value);
  }/*
  else if(token<0x0310)
  {
    DXDev->SetSamplerState(D3DDMAPSAMPLER,(enum _D3DSAMPLERSTATETYPE)(token&15),value);
  }*/
}

/****************************************************************************/
/****************************************************************************/

void sSystem_::CreateGeoBuffer(sInt i,sInt dyn,sInt vertex)
{
  sInt usage;
  sInt size;
  sGeoBuffer *gb;

  sVERIFY(dyn==0 || dyn==1);
  sVERIFY(vertex==0 || vertex==1);

  gb = &GeoBuffer[i];

  usage = D3DUSAGE_WRITEONLY;
  if(dyn)
    usage |= D3DUSAGE_DYNAMIC;

  if(vertex)
  {
    size = MAX_DYNVBSIZE;
    DXERRHARD(DXDev->CreateVertexBuffer(size,usage,0,D3DPOOL_DEFAULT,&gb->VB,0));
  }
  else
  {
    size = MAX_DYNIBSIZE;
    DXERRHARD(DXDev->CreateIndexBuffer(size,usage,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&gb->IB,0));  
  }
  gb->Size = size;
  gb->Type = vertex+1;
  gb->Used = 0;
  gb->UserCount = 0;
}

sInt sSystem_::GeoAdd(sInt fvf,sInt prim)
{
  sInt i;
  sGeoHandle *gh;

  i = 3;
  for(;;)
  {
    sVERIFY(i<MAX_GEOHANDLE);
    gh = &GeoHandle[i];
    if(gh->Mode==0)
    {
      sSetMem(gh,0,sizeof(*gh));
      sVERIFY(fvf==(sFVF_STANDARD) || fvf==(sFVF_DOUBLE));

      gh->Mode = prim;
      gh->FVF = fvf;
      return i;
    }
    i++;
  }
}

void sSystem_::GeoRem(sInt handle)
{
  sVERIFY(handle>=1 && handle<MAX_GEOHANDLE);
  sVERIFY(GeoHandle[handle].Mode!=0);

  GeoHandle[handle].Mode = 0;
}

void sSystem_::GeoFlush(sInt handle)
{
  sVERIFY(handle>=1 && handle<MAX_GEOHANDLE);
  GeoHandle[handle].DiscardCount = 0;
}

sBool sSystem_::GeoDraw(sInt &handle)
{
  sGeoHandle *gh;
  sInt count;
  sInt ib,is;
  sU32 dxfvf;

// buffer correctly loaded?

  sVERIFY(handle>=1 && handle<MAX_GEOHANDLE);
  sVERIFY(GeoHandle[handle].Mode!=0);
  gh = &GeoHandle[handle];
  sVERIFY(gh->Mode==sGEO_TRI || gh->Mode==sGEO_QUAD || gh->Mode==(sGEO_TRI|sGEO_STATIC));

  if(gh->VertexCount==0)
    return sTRUE;
  if(gh->VertexBuffer==0 && gh->DiscardCount!=DiscardCount)
    return sTRUE;

// fake quadbuffers)

  ib = gh->IndexBuffer;
  is = gh->IndexStart>>1;
  count = gh->IndexCount;
  if(count==0)
    count = gh->VertexCount;

  if(gh->Mode == sGEO_QUAD)
  {
    sVERIFY(gh->IndexCount==0);
    ib = 2;
    is = 0;
    count = (count * 3)>>1;
  }
  else
  {
    sVERIFY(gh->IndexCount!=0 || ib==0)
  }
  count = count/3;

// set up buffers

  if(gh->FVF==sFVF_STANDARD)
    dxfvf = D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1;
  else
    dxfvf = D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX2;

  DXDev->SetFVF(dxfvf);
  DXDev->SetStreamSource(0,GeoBuffer[gh->VertexBuffer].VB,0,32);
  if(ib)
  {
    DXDev->SetIndices(GeoBuffer[ib].IB);
    DXDev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,gh->VertexStart>>5,0,gh->VertexCount,is,count);
  }
  else
  {
    DXDev->DrawPrimitive(D3DPT_TRIANGLELIST,gh->VertexStart>>5,count);
  }

  return sFALSE;
}

void sSystem_::GeoBegin(sInt handle,sInt vc,sInt ic,sF32 **fp,sU16 **ip)
{
  sGeoHandle *gh;
  sInt ok;
  sInt flags;

  sVERIFY(handle>=1 && handle<MAX_GEOHANDLE);
  sVERIFY(GeoHandle[handle].Mode!=0);
  sVERIFY(vc*32<=MAX_DYNVBSIZE); 
  sVERIFY(ic*2<=MAX_DYNIBSIZE);

  gh = &GeoHandle[handle];

  gh->VertexCount = vc;
  gh->IndexCount = ic;

  if(gh->Mode&sGEO_STATIC)
  {
    while(!GeoBuffer[GeoThisVertex].AllocXB(vc,5,gh->VertexStart))
    {
      GeoThisVertex++;
      sVERIFY(GeoThisVertex<GeoThisIndex);
      CreateGeoBuffer(GeoThisVertex,0,1);
    }
    gh->VertexBuffer = GeoThisVertex;
    GeoBuffer[gh->VertexBuffer].VB->Lock(gh->VertexStart,vc*32,(void **)fp,0);

    if(ic)
    {
      while(!GeoBuffer[GeoThisIndex].AllocXB(ic,1,gh->IndexStart))
      {
        GeoThisIndex--;
        sVERIFY(GeoThisVertex<GeoThisIndex);
        CreateGeoBuffer(GeoThisIndex,0,0);
      }
      gh->IndexBuffer = GeoThisIndex;
      GeoBuffer[gh->IndexBuffer].IB->Lock(gh->IndexStart,ic*2,(void **)ip,0);
    }
  }
  else
  {
    flags = D3DLOCK_NOOVERWRITE;
retry:
    ok = GeoBuffer[0].AllocXB(vc,5,gh->VertexStart);
    gh->VertexBuffer = 0;
    if(ic)
    {
      gh->IndexBuffer = 1;
      ok &= GeoBuffer[1].AllocXB(ic,1,gh->IndexStart);
    }
    if(!ok)
    {
      GeoBuffer[0].Used = 0;
      GeoBuffer[1].Used = 0;
      flags = D3DLOCK_DISCARD;
      DiscardCount++;
      goto retry;
    }
    gh->DiscardCount = DiscardCount;

    GeoBuffer[gh->VertexBuffer].VB->Lock(gh->VertexStart,vc*32,(void **)fp,flags);
    if(ic)
      GeoBuffer[gh->IndexBuffer].IB->Lock(gh->IndexStart,ic*2,(void **)ip,flags);
  }
}

void sSystem_::GeoEnd(sInt handle,sInt vc,sInt ic)
{
  sGeoHandle *gh;

  sVERIFY(handle>=1 && handle<MAX_GEOHANDLE);
  sVERIFY(GeoHandle[handle].Mode!=0);

  gh = &GeoHandle[handle];

  GeoBuffer[gh->VertexBuffer].VB->Unlock();
  if(gh->IndexBuffer)
    GeoBuffer[gh->IndexBuffer].IB->Unlock();

  if(vc!=-1)
  {
    gh->VertexCount = vc;
    gh->IndexCount = ic;
  }

//  GeoDraw(handle);
}

/****************************************************************************/
/****************************************************************************/

void sSystem_::StateBase(sU32 *&datap,sU32 flags,sInt texmode,sU32 color)
{
  sInt i;
  sU32 *data;

  static sS16 states[] = 
  {
    sD3DRS_CULLMODE                ,sD3DCULL_CCW,         // 0
    sD3DRS_NORMALIZENORMALS        ,0,                    // 1
    sD3DRS_LIGHTING                ,0,                    // 2

    sD3DRS_ZWRITEENABLE            ,0,                    // 3
    sD3DRS_ZFUNC                   ,sD3DCMP_ALWAYS,       // 4
    D3DRS_COLORWRITEENABLE         ,15,                   // 5

    sD3DRS_ALPHABLENDENABLE,1,                            // 6
    sD3DRS_SRCBLEND   ,    sD3DBLEND_ONE     ,            // 7
    sD3DRS_DESTBLEND  ,    sD3DBLEND_ONE     ,            // 8

    sD3DTSS_0|sD3DTSS_COLOROP,sD3DTOP_MODULATE,           // 9   0
    sD3DTSS_0|sD3DTSS_ALPHAOP,sD3DTOP_MODULATE,           // 10  1
	  sD3DTSS_1|sD3DTSS_COLOROP,sD3DTOP_DISABLE,            // 11  2
	  sD3DTSS_1|sD3DTSS_ALPHAOP,sD3DTOP_DISABLE,            // 12  3
    sD3DTSS_2|sD3DTSS_COLOROP,sD3DTOP_DISABLE,            // 13  4
    sD3DTSS_0|sD3DTSS_COLORARG1,sD3DTA_TEXTURE,           // 14  5
    sD3DTSS_0|sD3DTSS_COLORARG2,sD3DTA_DIFFUSE,           // 15  6
    sD3DTSS_1|sD3DTSS_COLORARG1,sD3DTA_TEXTURE,           // 16  7
    sD3DTSS_1|sD3DTSS_COLORARG2,sD3DTA_CURRENT,           // 17  8
    sD3DTSS_0|sD3DTSS_ALPHAARG1,sD3DTA_TEXTURE,           // 18  9
    sD3DTSS_0|sD3DTSS_ALPHAARG2,sD3DTA_DIFFUSE,           // 19  10

    sD3DRS_ALPHATESTENABLE         ,0,
    sD3DRS_ALPHAFUNC               ,sD3DCMP_GREATER,
    sD3DRS_ALPHAREF                ,4,
    sD3DRS_STENCILENABLE           ,0,

    sD3DRS_DIFFUSEMATERIALSOURCE   ,sD3DMCS_COLOR1,
    sD3DRS_AMBIENTMATERIALSOURCE   ,sD3DMCS_MATERIAL,
    sD3DRS_SPECULARMATERIALSOURCE  ,sD3DMCS_MATERIAL,
    sD3DRS_EMISSIVEMATERIALSOURCE  ,sD3DMCS_MATERIAL,
    sD3DRS_SPECULARENABLE          ,0,

    sD3DRS_BLENDOP    ,    sD3DBLENDOP_ADD   ,
  };

  data = datap;
  i=0;
  do
  {
    data[i] = states[i]; i++;
  }
  while(i<30*2);

  if(flags & sMBF_INVERTCULL)       data[0*2+1] = sD3DCULL_CW;
  if(flags & sMBF_DOUBLESIDED)      data[0*2+1] = sD3DCULL_NONE;
  if(flags & sMBF_RENORMALIZE)      data[1*2+1] = 1;
  if(flags & sMBF_LIGHT)            data[2*2+1] = 1;
  if(flags & sMBF_ZWRITE)           data[3*2+1] = 1;
  if(flags & sMBF_ZREAD)            data[4*2+1] = sD3DCMP_LESSEQUAL;
  if(flags & sMBF_ZONLY)            data[5*2+1] = 0;

  switch((flags>>12)&7)
  {
  default:
  case sMBF_BLENDOFF>>12:
    data[6*2+1] = 0;
    break;
  case sMBF_BLENDADD>>12:
    break;
  case sMBF_BLENDMUL>>12:
    data[7*2+1] = sD3DBLEND_ZERO;
    data[8*2+1] = sD3DBLEND_SRCCOLOR;
    break;
  case sMBF_BLENDALPHA>>12:
    data[7*2+1] = sD3DBLEND_SRCALPHA;
    data[8*2+1] = sD3DBLEND_INVSRCALPHA;
    break;
  case sMBF_BLENDSMOOTH>>12:
    data[8*2+1] = sD3DBLEND_INVSRCCOLOR;
    break;
  }

  data+=9*2+1;


/*
  switch(texmode)
  {
  default:
  case sMBM_FLAT:
    data[1*2] = sD3DTOP_SELECTARG2;
    data[0*2] = sD3DTOP_SELECTARG2;
    break;
  case sMBM_TEX:
	case sMBM_MULCOL:
    break;
  case sMBM_ADD:
    data[2*2] = sD3DTOP_ADD;
    break;
  case sMBM_MUL:
    data[2*2] = sD3DTOP_MODULATE;
    break;
	case sMBM_BLEND:
    data[0*2] = sD3DTOP_SELECTARG1;
    data[1*2] = sD3DTOP_SELECTARG1;
    data[2*2] = sD3DTOP_BLENDDIFFUSEALPHA;
		break;
  case sMBM_SUB:
    data[2*2] = sD3DTOP_SUBTRACT;
    break;
  case sMBM_ADDSMOOTH:
    data[2*2] = sD3DTOP_ADDSMOOTH;
    break;
	case sMBM_ADDCOL:
    data[0*2] = sD3DTOP_ADD;
		break;
	case sMBM_BLENDCOL:
    data[5*2] = sD3DTA_DIFFUSE;
    data[6*2] = sD3DTA_TEXTURE;
    data[1*2] = sD3DTOP_SELECTARG1;
    data[0*2] = sD3DTOP_BLENDDIFFUSEALPHA;
		break;
	case sMBM_SUBCOL:
    data[0*2] = sD3DTOP_SUBTRACT;
		break;
  case sMBM_ADDSMOOTHCOL:
    data[0*2] = sD3DTOP_ADDSMOOTH;
    break;
  }
*/
  data+=(30-9)*2-1;



  switch(texmode)
  {
  case sMBM_FLAT:
    *data++ = sD3DTSS_0|sD3DTSS_COLOROP;    *data++ = sD3DTOP_SELECTARG2;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_COLOROP;    *data++ = sD3DTOP_DISABLE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_SELECTARG2;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_DISABLE;
    break;
  case sMBM_TEX:
    *data++ = sD3DTSS_0|sD3DTSS_COLOROP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_COLOROP;    *data++ = sD3DTOP_DISABLE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_DISABLE;
    break;
  case sMBM_ADD:
    *data++ = sD3DTSS_0|sD3DTSS_COLOROP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_COLOROP;    *data++ = sD3DTOP_ADD;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_COLOROP;    *data++ = sD3DTOP_DISABLE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_SELECTARG2;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_DISABLE;
    break;
  case sMBM_MUL:
    *data++ = sD3DTSS_0|sD3DTSS_COLOROP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_COLOROP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_COLOROP;    *data++ = sD3DTOP_DISABLE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_SELECTARG2;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_DISABLE;
    break;
	case sMBM_BLEND:
    *data++ = sD3DTSS_0|sD3DTSS_COLOROP;    *data++ = sD3DTOP_SELECTARG1;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_COLOROP;    *data++ = sD3DTOP_BLENDDIFFUSEALPHA;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_COLOROP;    *data++ = sD3DTOP_DISABLE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_SELECTARG1;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_SELECTARG2;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_DISABLE;
		break;
  case sMBM_SUB:
    *data++ = sD3DTSS_0|sD3DTSS_COLOROP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_COLOROP;    *data++ = sD3DTOP_SUBTRACT;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_2|sD3DTSS_COLOROP;    *data++ = sD3DTOP_DISABLE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_SELECTARG2;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_DISABLE;
    break;
  case sMBM_ADDSMOOTH:
    *data++ = sD3DTSS_0|sD3DTSS_COLOROP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_COLOROP;    *data++ = sD3DTOP_ADDSMOOTH;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_COLOROP;    *data++ = sD3DTOP_DISABLE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_SELECTARG2;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_DISABLE;
    break;
	case sMBM_ADDCOL:
		*data++ = sD3DTSS_0|sD3DTSS_COLOROP;		*data++ = sD3DTOP_ADD;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++ = sD3DTSS_1|sD3DTSS_COLOROP;		*data++ = sD3DTOP_DISABLE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_MODULATE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++	= sD3DTSS_1|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_DISABLE;
		break;
	case sMBM_MULCOL:
		*data++ = sD3DTSS_0|sD3DTSS_COLOROP;		*data++ = sD3DTOP_MODULATE;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++ = sD3DTSS_1|sD3DTSS_COLOROP;		*data++ = sD3DTOP_DISABLE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_MODULATE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++	= sD3DTSS_1|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_DISABLE;
		break;
	case sMBM_BLENDCOL:
		*data++ = sD3DTSS_0|sD3DTSS_COLOROP;		*data++ = sD3DTOP_BLENDDIFFUSEALPHA;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG1;	*data++ = sD3DTA_DIFFUSE;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG2;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_1|sD3DTSS_COLOROP;		*data++ = sD3DTOP_DISABLE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_SELECTARG1;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;	*data++ = sD3DTA_TEXTURE;
		*data++	= sD3DTSS_1|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_DISABLE;
		break;
	case sMBM_SUBCOL:
		*data++ = sD3DTSS_0|sD3DTSS_COLOROP;		*data++ = sD3DTOP_SUBTRACT;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++ = sD3DTSS_1|sD3DTSS_COLOROP;		*data++ = sD3DTOP_DISABLE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_MODULATE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++	= sD3DTSS_1|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_DISABLE;
		break;
	case sMBM_ADDSMOOTHCOL:
		*data++ = sD3DTSS_0|sD3DTSS_COLOROP;		*data++ = sD3DTOP_ADDSMOOTH;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++ = sD3DTSS_1|sD3DTSS_COLOROP;		*data++ = sD3DTOP_DISABLE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_MODULATE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++	= sD3DTSS_1|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_DISABLE;
		break;
  }


  if(flags & sMBF_COLOR)
  {
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG2;	*data++ = sD3DTA_TFACTOR;
    *data++ = sD3DRS_TEXTUREFACTOR;         *data++ = color;
  }

  datap = data;
}


/*
void sSystem_::StateBase(sU32 *&data,sU32 flags,sInt texmode,sU32 color)
{
  *data++ = sD3DRS_ALPHATESTENABLE;         *data++ = 0;
  *data++ = sD3DRS_ALPHAFUNC;               *data++ = sD3DCMP_GREATER;
  *data++ = sD3DRS_ALPHAREF;                *data++ = 4;
  *data++ = sD3DRS_STENCILENABLE;           *data++ = 0;

  *data++ = sD3DMAT_DIFFUSE;                *data++ = 0xffffffff;
  *data++ = sD3DMAT_AMBIENT;                *data++ = 0xffffffff;
  *data++ = sD3DMAT_SPECULAR;               *data++ = 0xffffffff;
  *data++ = sD3DMAT_EMISSIVE;               *data++ = 0x00000000;
  *data++ = sD3DMAT_POWER;                  *(sF32 *)data++ = 1.0f;
  *data++ = sD3DRS_DIFFUSEMATERIALSOURCE;   *data++ = sD3DMCS_COLOR1;
  *data++ = sD3DRS_AMBIENTMATERIALSOURCE;   *data++ = sD3DMCS_MATERIAL;
  *data++ = sD3DRS_SPECULARMATERIALSOURCE;  *data++ = sD3DMCS_MATERIAL;
  *data++ = sD3DRS_EMISSIVEMATERIALSOURCE;  *data++ = sD3DMCS_MATERIAL;
  *data++ = sD3DRS_SPECULARENABLE;          *data++ = 0;

  *data++ = sD3DRS_ZENABLE;
  *data++ = sD3DZB_TRUE;

  *data++ = sD3DRS_ZWRITEENABLE;
  *data++ = (flags & sMBF_ZWRITE) ? 1 : 0;

  *data++ = sD3DRS_ZFUNC;
  *data++ = (flags & sMBF_ZREAD) ? sD3DCMP_LESSEQUAL : sD3DCMP_ALWAYS;

  *data++ = sD3DRS_CULLMODE;
  *data++ = (flags & sMBF_DOUBLESIDED) ? sD3DCULL_NONE : (flags & sMBF_INVERTCULL?sD3DCULL_CW:sD3DCULL_CCW);

  *data++ = D3DRS_COLORWRITEENABLE;
  *data++ = (flags & sMBF_ZONLY) ? 0 : 15;

  *data++ = sD3DRS_FOGENABLE;
  *data++ = 0;

  *data++ = sD3DRS_CLIPPING;
  *data++ = 1;
  
  *data++ = sD3DRS_LIGHTING;
  *data++ = (flags & sMBF_LIGHT) ? 1 : 0;

  *data++ = sD3DRS_NORMALIZENORMALS;
  *data++ = (flags & sMBF_RENORMALIZE) ? 1 : 0;

  switch(flags&sMBF_BLENDMASK)
  {
  case sMBF_BLENDOFF:
    *data++ = sD3DRS_ALPHABLENDENABLE;
    *data++ = 0;
    break;
  case sMBF_BLENDADD:
    *data++ = sD3DRS_ALPHABLENDENABLE;
    *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;
    *data++ = sD3DBLEND_ONE;
    *data++ = sD3DRS_DESTBLEND;
    *data++ = sD3DBLEND_ONE;
    *data++ = sD3DRS_BLENDOP;
    *data++ = sD3DBLENDOP_ADD;
    break;
  case sMBF_BLENDMUL:
    *data++ = sD3DRS_ALPHABLENDENABLE;
    *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;
    *data++ = sD3DBLEND_ZERO;
    *data++ = sD3DRS_DESTBLEND;
    *data++ = sD3DBLEND_SRCCOLOR;
    *data++ = sD3DRS_BLENDOP;
    *data++ = sD3DBLENDOP_ADD;
    break;
  case sMBF_BLENDALPHA:
    *data++ = sD3DRS_ALPHABLENDENABLE;
    *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;
    *data++ = sD3DBLEND_SRCALPHA;
    *data++ = sD3DRS_DESTBLEND;
    *data++ = sD3DBLEND_INVSRCALPHA;
    *data++ = sD3DRS_BLENDOP;
    *data++ = sD3DBLENDOP_ADD;
    break;
  case sMBF_BLENDSMOOTH:
    *data++ = sD3DRS_ALPHABLENDENABLE;
    *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;
    *data++ = sD3DBLEND_ONE;
    *data++ = sD3DRS_DESTBLEND;
    *data++ = sD3DBLEND_INVSRCCOLOR;
    *data++ = sD3DRS_BLENDOP;
    *data++ = sD3DBLENDOP_ADD;
    break;
  }

  switch(texmode)
  {
  case sMBM_FLAT:
    *data++ = sD3DTSS_0|sD3DTSS_COLOROP;    *data++ = sD3DTOP_SELECTARG2;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_COLOROP;    *data++ = sD3DTOP_DISABLE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_SELECTARG2;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_DISABLE;
    break;
  case sMBM_TEX:
    *data++ = sD3DTSS_0|sD3DTSS_COLOROP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_COLOROP;    *data++ = sD3DTOP_DISABLE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_DISABLE;
    break;
  case sMBM_ADD:
    *data++ = sD3DTSS_0|sD3DTSS_COLOROP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_COLOROP;    *data++ = sD3DTOP_ADD;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_COLOROP;    *data++ = sD3DTOP_DISABLE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_SELECTARG2;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_DISABLE;
    break;
  case sMBM_MUL:
    *data++ = sD3DTSS_0|sD3DTSS_COLOROP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_COLOROP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_COLOROP;    *data++ = sD3DTOP_DISABLE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_SELECTARG2;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_DISABLE;
    break;
	case sMBM_BLEND:
    *data++ = sD3DTSS_0|sD3DTSS_COLOROP;    *data++ = sD3DTOP_SELECTARG1;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_COLOROP;    *data++ = sD3DTOP_BLENDDIFFUSEALPHA;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_COLOROP;    *data++ = sD3DTOP_DISABLE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_SELECTARG1;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_SELECTARG2;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_DISABLE;
		break;
  case sMBM_SUB:
    *data++ = sD3DTSS_0|sD3DTSS_COLOROP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_COLOROP;    *data++ = sD3DTOP_SUBTRACT;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_2|sD3DTSS_COLOROP;    *data++ = sD3DTOP_DISABLE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_SELECTARG2;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_DISABLE;
    break;
  case sMBM_ADDSMOOTH:
    *data++ = sD3DTSS_0|sD3DTSS_COLOROP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_COLORARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_COLOROP;    *data++ = sD3DTOP_ADDSMOOTH;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_COLORARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_COLOROP;    *data++ = sD3DTOP_DISABLE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_MODULATE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_DIFFUSE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_SELECTARG2;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG1;  *data++ = sD3DTA_TEXTURE;
    *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG2;  *data++ = sD3DTA_CURRENT;
    *data++ = sD3DTSS_2|sD3DTSS_ALPHAOP;    *data++ = sD3DTOP_DISABLE;
    break;
	case sMBM_ADDCOL:
		*data++ = sD3DTSS_0|sD3DTSS_COLOROP;		*data++ = sD3DTOP_ADD;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++ = sD3DTSS_1|sD3DTSS_COLOROP;		*data++ = sD3DTOP_DISABLE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_MODULATE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++	= sD3DTSS_1|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_DISABLE;
		break;
	case sMBM_MULCOL:
		*data++ = sD3DTSS_0|sD3DTSS_COLOROP;		*data++ = sD3DTOP_MODULATE;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++ = sD3DTSS_1|sD3DTSS_COLOROP;		*data++ = sD3DTOP_DISABLE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_MODULATE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++	= sD3DTSS_1|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_DISABLE;
		break;
	case sMBM_BLENDCOL:
		*data++ = sD3DTSS_0|sD3DTSS_COLOROP;		*data++ = sD3DTOP_BLENDDIFFUSEALPHA;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG1;	*data++ = sD3DTA_DIFFUSE;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG2;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_1|sD3DTSS_COLOROP;		*data++ = sD3DTOP_DISABLE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_SELECTARG1;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;	*data++ = sD3DTA_TEXTURE;
		*data++	= sD3DTSS_1|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_DISABLE;
		break;
	case sMBM_SUBCOL:
		*data++ = sD3DTSS_0|sD3DTSS_COLOROP;		*data++ = sD3DTOP_SUBTRACT;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++ = sD3DTSS_1|sD3DTSS_COLOROP;		*data++ = sD3DTOP_DISABLE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_MODULATE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++	= sD3DTSS_1|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_DISABLE;
		break;
	case sMBM_ADDSMOOTHCOL:
		*data++ = sD3DTSS_0|sD3DTSS_COLOROP;		*data++ = sD3DTOP_ADDSMOOTH;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++ = sD3DTSS_1|sD3DTSS_COLOROP;		*data++ = sD3DTOP_DISABLE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_MODULATE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;	*data++ = sD3DTA_TEXTURE;
		*data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;	*data++ = sD3DTA_DIFFUSE;
		*data++	= sD3DTSS_1|sD3DTSS_ALPHAOP;		*data++ = sD3DTOP_DISABLE;
		break;
  }

  if(flags & sMBF_COLOR)
  {
		*data++ = sD3DTSS_0|sD3DTSS_COLORARG2;	*data++ = sD3DTA_TFACTOR;
    *data++ = sD3DRS_TEXTUREFACTOR;         *data++ = color;
  }
}
*/

/****************************************************************************/
/*
void sSystem_::StateTex(sU32 *&datap,sInt nr,sU32 flags)
{
  sInt tex,sam;
  static sMatrix envimat;
  sU32 *data;
  sInt i;

  static sU8 states[] =
  {
    0xff&sD3DSAMP_MAGFILTER,sD3DTEXF_POINT,
    0xff&sD3DSAMP_MINFILTER,sD3DTEXF_POINT,
    0xff&sD3DSAMP_MIPFILTER,sD3DTEXF_NONE,
    0xff&sD3DSAMP_ADDRESSU,sD3DTADDRESS_WRAP,
    0xff&sD3DSAMP_ADDRESSV,sD3DTADDRESS_WRAP,
  };
  static sU8 envi[16] = 
  {
    0,0,0,0,0,0,0,0,
    D3DTSS_TCI_CAMERASPACENORMAL>>16,
    D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR>>16,
    D3DTSS_TCI_CAMERASPACEPOSITION>>16,
  };


  tex = (nr*sD3DTSS_1);
  sam = (nr+0x200/sD3DSAMP_1)*sD3DSAMP_1;
  data = datap;

  i=0;
  do
  {
    data[i] = states[i]|sam; i++;
    data[i] = states[i]; i++;
  }
  while(i<10);

  if(flags & sMTF_FILTERMAG)
    data[0*2+1] = sD3DTEXF_LINEAR;
  if(flags & sMTF_FILTERMIN)
    data[1*2+1] = sD3DTEXF_LINEAR;
  if(flags & sMTF_MIPMAP)
    data[2*2+1] = sD3DTEXF_LINEAR;
  if(flags & sMTF_CLAMP)
  {
    data[3*2+1] = sD3DTADDRESS_CLAMP;
    data[4*2+1] = sD3DTADDRESS_CLAMP;
  }
  data+=10;

  *data++ = tex+sD3DTSS_TEXTURETRANSFORMFLAGS;
  *data++ = (flags & (sMTF_UVTRANS | sMTF_ENVIMASK)) ? 2 : 0;

  *data++ = tex+sD3DTSS_TEXCOORDINDEX;
  *data++ = envi[(flags & sMTF_UVMASK)>>sMTF_UVSHIFT]<<16;
 
  datap = data;
}
*/


void sSystem_::StateTex(sU32 *&data,sInt nr,sU32 flags)
{
  sInt tex,sam;
  static sMatrix envimat;

  tex = nr*sD3DTSS_1;
  sam = nr*sD3DSAMP_1;

  *data++ = sam+sD3DSAMP_MAGFILTER;
  *data++ = flags & sMTF_FILTERMAG ? sD3DTEXF_LINEAR : sD3DTEXF_POINT;

  *data++ = sam+sD3DSAMP_MINFILTER;
  *data++ = flags & sMTF_FILTERMIN ? sD3DTEXF_LINEAR : sD3DTEXF_POINT;

  *data++ = sam+sD3DSAMP_MIPFILTER;
  *data++ = flags & sMTF_MIPMAP ? sD3DTEXF_LINEAR : sD3DTEXF_NONE;

  *data++ = sam+sD3DSAMP_ADDRESSU;
  *data++ = flags & sMTF_CLAMP ? sD3DTADDRESS_CLAMP : sD3DTADDRESS_WRAP;

  *data++ = sam+sD3DSAMP_ADDRESSV;
  *data++ = flags & sMTF_CLAMP ? sD3DTADDRESS_CLAMP : sD3DTADDRESS_WRAP;

  *data++ = tex+sD3DTSS_TEXTURETRANSFORMFLAGS;
  *data++ = ((flags & sMTF_UVTRANS)||(flags & sMTF_ENVIMASK)) ? 2 : 0;

  *data++ = tex+sD3DTSS_TEXCOORDINDEX;
  switch(flags & sMTF_UVMASK)
  {
    case sMTF_UVENVI: *data++ = D3DTSS_TCI_CAMERASPACENORMAL;           break;
    case sMTF_UVSPEC: *data++ = D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR; break;
    case sMTF_UVPOS:  *data++ = D3DTSS_TCI_CAMERASPACEPOSITION;         break;
    default:          *data++ = (flags>>sMTF_UVSHIFT)&7;                break;
  }
}

/****************************************************************************/

void sSystem_::StateEnd(sU32 *&datap,sU32 *buffer,sInt size)
{
  sU32 *data; 
  data = datap;
  data[0] = ~0;
  data[1] = ~0;
  datap = data+2;
  sVERIFY(data <= buffer+size/4);
}

void sSystem_::SetMatLight(const sD3DMaterial &ml)
{
  DXDev->SetMaterial((D3DMATERIAL9 *)&ml);
}

/****************************************************************************/

sInt sSystem_::AddLight(sD3DLight &li,sU32 mask)
{
  sVERIFY(li.Type!=sLI_AMBIENT) 
  sVERIFY(LightCount<MAX_LIGHTS);

  DXDev->SetLight(LightCount,(D3DLIGHT9 *)&li);
  LightMask[LightCount] = mask;
  return LightCount++;
}

void sSystem_::EnableLights(sU32 mask)
{
  sInt i;

  i = 0;
  do
  {
    DXDev->LightEnable(i,(LightMask[i]&mask)?1:0);
    i++;
  }
  while(i<LightCount);

  __asm finit; // driver bugs SUCK SUCK SUCK SUCK SUCK SUCK
}

/****************************************************************************/
/****************************************************************************/


#endif

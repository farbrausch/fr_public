// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "_types.hpp"
#include "_start.hpp"
#include "_startdx.hpp"
#if sLINK_UTIL
#include "_util.hpp"                  // for sPerfMon->Flip()
#endif

#if !sINTRO

#define WINVER 0x500
#define DIRECTINPUT_VERSION 0x0800

#define INITGUID // to get rid of dxguid.lib

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
#define DXERROR(hr) {if(FAILED(hr))sFatal("%s(%d) : directx error %08x (%d)",__FILE__,__LINE__,hr,hr&0x3fff);}

#undef DeleteFile
#undef GetCurrentDirectory
#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"opengl32.lib")
#pragma comment(lib,"dinput8.lib")
#if sUSE_SHADERS
#pragma comment(lib,"d3dx9.lib")
#endif

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   System Initialisation                                              ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

class sBroker_ *sBroker;

HMODULE d3dlib; 
HMODULE dilib;
HMODULE dslib;

typedef HRESULT (WINAPI * DirectInput8CreateT)(HINSTANCE hinst,DWORD dwVersion,REFIID riidltf,LPVOID* ppvOut,LPUNKNOWN punkOuter);
typedef IDirect3D9* (WINAPI * Direct3DCreate9T)(UINT SDKVersion);
typedef HRESULT (WINAPI * DirectSoundCreate8T)(LPCGUID lpcGuidDevice,LPDIRECTSOUND8 * ppDS8,LPUNKNOWN pUnkOuter);

DirectInput8CreateT DirectInput8CreateP;
Direct3DCreate9T Direct3DCreate9P;
DirectSoundCreate8T DirectSoundCreate8P;

HINSTANCE WInst;
HWND MsgWin;

sSystem_ *sSystem;
sU32 sSystem_::CpuMask;
sU32 sSystem_::GfxSystem;

static sU64 sPerfFrame;
static sF64 sPerfKalibFactor = 1.0f/1000;
static sU64 sPerfKalibRDTSC;
static sU64 sPerfKalibQuery;

/****************************************************************************/

static HDC GDIScreenDC;
static HDC GDIDC;
static HBITMAP GDIHBM;

/****************************************************************************/

sBool InitGDI();
void ExitGDI();

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

sInt MemoryUsedCount;

#if !sRELEASE
#if !sINTRO
#undef new

void * __cdecl operator new(unsigned int size)
{
  void *p;
  p = _malloc_dbg(size,_NORMAL_BLOCK,"unknown",1);
  MemoryUsedCount+=_msize(p); 
  return p;
}

void * __cdecl operator new(unsigned int size,const char *file,int line)
{
  void *p;
  p = _malloc_dbg(size,_NORMAL_BLOCK,file,line);
  MemoryUsedCount+=_msize(p); 
  return p;
}

void operator delete(void *p)
{
	if(p)
	{
		MemoryUsedCount-=_msize(p); 
		_free_dbg(p,_NORMAL_BLOCK);
	}
}

#define new new(__FILE__,__LINE__)
#endif
#endif

#if sINTRO
#if sRELEASE
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

#if sRELEASE
extern "C" int _fltused;
int _fltused;
#endif

#endif

/****************************************************************************/
/****************************************************************************/

static void MakeCpuMask()
{
#if !sINTRO
  sU32 result;
  sU32 vendor[4];
#endif

// start with nothing supportet

  sSystem_::CpuMask = 0;

#if !sINTRO
// check cpuid command, and check avaiability of standard function #1

  __try
  {
    __asm
    {
      pushad
      xor     eax,eax
      CPUID
      mov     result,eax
      mov     vendor,ebx
      mov     vendor+4,edx
      mov     vendor+8,ecx
      xor     eax,eax
      mov     vendor+12,eax
      popad
    }
  }
  __except(1)
  {
    return;
  }

  if(result==0)
    return;

// check standard features

  __asm
  {
    pushad
    mov     eax,1
    CPUID
    mov     result,edx  
    popad
  }
  if(result&(1<< 4)) sSystem_::CpuMask |= sCPU_RDTSC;
  if(result&(1<<15)) sSystem_::CpuMask |= sCPU_CMOVE;  
  if(result&(1<<23)) sSystem_::CpuMask |= sCPU_MMX;
  if(result&(1<<25)) sSystem_::CpuMask |= sCPU_SSE|sCPU_MMX2;
  if(result&(1<<26)) sSystem_::CpuMask |= sCPU_SSE2;

// check extended features 

  __asm
  {
    pushad
    mov     eax,0x80000000
    CPUID
    mov     result,eax
    popad
  }
  if(result>=0x80000001)
  {
    __asm
    {
      pushad
      mov     eax,0x80000001
      CPUID
      mov     result,edx
      popad
    }
    if(result&(1<<31)) sSystem_::CpuMask |= sCPU_3DNOW;

// check AMD specific features

    if(sCmpMem(vendor,"AuthenticAMD",12)==0)
    {
      if(result&(1<<22)) sSystem_::CpuMask |= sCPU_MMX2;  
      if(result&(1<<30)) sSystem_::CpuMask |= sCPU_3DNOW2;  
    }
  }

// is the SSE support complete and supported by the operating system?
  
  if(sSystem_::CpuMask & sCPU_SSE)
  {
    __try
    {
      __asm
      {
        orps xmm0,xmm1
      }
    }
    __except(1)
    {
      sSystem_::CpuMask &= ~(sCPU_SSE|sCPU_SSE2);
    }
  }
#endif
}

/****************************************************************************/

static LRESULT WINAPI MainWndProc(HWND win,UINT msg,WPARAM wparam,LPARAM lparam)
{
  MsgWin = win;
  return sSystem->Msg(msg,wparam,lparam);
}

/****************************************************************************/

/****************************************************************************/

#if sCODECOVERMODE==1
sU8 sCodeCoverString[512];
#endif

#if !sINTRO || !sRELEASE
int APIENTRY WinMain(HINSTANCE inst,HINSTANCE prev,LPSTR cmdline,int show)
{
  WInst = inst;
#else
void WinMainCRTStartup()
{
	WInst = GetModuleHandle(0);
#endif

#if sCODECOVERMODE==1
  sSetMem(sCodeCoverString,0,sizeof(sCodeCoverString));
#endif

  MakeCpuMask();
  if(!sAppHandler(sAPPCODE_CONFIG,0))
    sSetConfig(sSF_DIRECT3D);
  sSystem->InitX();

#if sCODECOVERMODE==1
  sChar Buffer[sizeof(sCodeCoverString)*4+88],*bufp;
  sInt i;
  
  sCopyString(Buffer,"#define sCODECOVERSTRING \"",256);
  bufp = Buffer+sGetStringLen(Buffer);
  for(i=0;i<sizeof(sCodeCoverString);i++)
  {
    *bufp++ = '\\';
    *bufp++ = 'x';
    *bufp++ = "0123456789abcdef"[sCodeCoverString[i]>>4];
    *bufp++ = "0123456789abcdef"[sCodeCoverString[i]&15];
  }
  *bufp++='"';
  *bufp++='\n';

  sSystem->SaveFile("codecoverage.hpp",(sU8 *)Buffer,bufp-Buffer);
#endif

  delete sSystem;

#if !sINTRO
  sDPrintF("Memory Left: %d bytes\n",MemoryUsedCount);
  return 0;
#else
	ExitProcess(0);
#endif
}

/****************************************************************************/

void sSetConfig(sU32 flags,sInt xs,sInt ys)
{
#if !sUSE_OPENGL && !sUSE_DIRECT3D
  sFatal("need either OpenGl or Direct3D");
#elif sUSE_OPENGL && sUSE_DIRECT3D
  if(flags&sSF_OPENGL)
  {
    sSystem = new sSystemGL;
    sSystem_::GfxSystem = sSF_OPENGL;
  }
  else
  {
    sSystem = new sSystemDX;
    sSystem_::GfxSystem = sSF_DIRECT3D;
  }
#elif !sUSE_OPENGL
  flags &= ~sSF_OPENGL;
  sSystem = new sSystemDX;
#elif !sUSE_DIRECT3D
  flags |= sSF_OPENGL;
  sSystem = new sSystemGL;
#endif

  sSetMem(((sU8 *)sSystem)+4,0,sizeof(sSystem_)-4);
  sSystem->ConfigFlags = flags;
  sSystem->ConfigX = xs;
  sSystem->ConfigY = ys;
}

/****************************************************************************/

BOOL CALLBACK MonitorEnumProc(HMONITOR handle,HDC hdc,LPRECT rect,LPARAM user)
{
  HWND win;
  HWND pwin;
  RECT r;

#if !sINTRO
  sDPrintF("found monitor <%s> at %d %d %d %d\n","xx",rect->left,rect->top,rect->right,rect->bottom);
#endif
  sSetMem(&r,0,sizeof(RECT));
  if(sSystem->WScreenCount==0)
  {
    if(sSystem->ConfigX==0 && sSystem->ConfigY==0)
    {
      if(sSystem->ConfigFlags & sSF_FULLSCREEN)
      {
        sSystem->ConfigX = rect->right - rect->left;
        sSystem->ConfigY = rect->bottom - rect->top;
      }
      else
      {
        SystemParametersInfo(SPI_GETWORKAREA,0,&r,0);
        sSystem->ConfigX = r.right-r.left;
        sSystem->ConfigY = r.bottom-r.top;
      }
    }
  }

  if(sSystem->WScreenCount<MAX_SCREEN)
  {
    if(sSystem->WScreenCount > 0)
      pwin = (HWND)sSystem->Screen[0].Window;
    else
      pwin = 0;

    win = CreateWindowEx(0,"kk",sAPPNAME " v" sVERSION,WS_OVERLAPPEDWINDOW|WS_VISIBLE,r.left,r.top,sSystem->ConfigX,sSystem->ConfigY,pwin,0,WInst,0);

    sSystem->WWindowedStyle = WS_OVERLAPPEDWINDOW|WS_VISIBLE;
    sSystem->Screen[sSystem->WScreenCount].Window = (sU32) win;
    sSystem->WScreenCount++;

    if(sSystem->ConfigFlags & sSF_MULTISCREEN)
      return sTRUE;
  }

  return sFALSE;
}

/****************************************************************************/

void sSystemPrivate::InitX()
{
  WNDCLASS wc;
  MSG msg;
  sBool gotmsg;
  sInt i;

// set up memory checking

#if !sINTRO
  _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)|_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF);
  sChar *test = new sChar[16];
  sCopyString(test,"TestLeak",16);
#ifdef sSETROOTDIR
  sChar *root = sSETROOTDIR;
  if(root[0]!=0 && SetCurrentDirectory(root))
    sDPrintF("changed dir to " sSETROOTDIR "\n");
  else
    sDPrintF("could not change dir");
#endif
#endif

// find dlls

  d3dlib = ::LoadLibraryA("d3d9.dll");
  dilib = ::LoadLibraryA("dinput8.dll");
  dslib = ::LoadLibraryA("dsound.dll");

  if(d3dlib==0 || dilib==0 || dslib==0)
  {
    sFatal("you need directx 9 (or better)\nto run this program.\ntry downloading it at\nwww.microsoft.com");
  }

  Direct3DCreate9P = (Direct3DCreate9T) GetProcAddress(d3dlib,"Direct3DCreate9");
  sVERIFY(Direct3DCreate9P);

  DirectInput8CreateP = (DirectInput8CreateT) GetProcAddress(dilib,"DirectInput8Create");
  sVERIFY(DirectInput8CreateP);

  DirectSoundCreate8P = (DirectSoundCreate8T) GetProcAddress(dslib,"DirectSoundCreate8");
  sVERIFY(DirectSoundCreate8P);

// set up some more stuff

  timeBeginPeriod(1);
  WStartTime = timeGetTime();
  sSetRndSeed(timeGetTime()&0x7fffffff);
  if(!InitGDI())
    sFatal("could not initialise GDI for Font conversions!");
  sBroker = new sBroker_;
#if sLINK_UTIL
  sPerfMon = new sPerfMon_;
  sBroker->AddRoot(sPerfMon);
#endif

  // sInitTypes();

// create window class

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

// init

  WDeviceLost = 0;
  WActiveCount = 0;
  WActiveMsg = 1;
  WContinuous = 1;
  WSinglestep = 0;
  WFullscreen = 0;
  WWindowedStyle = 0;
  WMinimized = 0;
  WMaximized = 0;
  WAborting = sFALSE;
  WResponse = 0;

  EnumDisplayMonitors(0,0,MonitorEnumProc,0);
  if(WScreenCount<2)
    ConfigFlags &= ~sSF_MULTISCREEN;
  if(ConfigFlags & sSF_MULTISCREEN)
    ConfigFlags |= sSF_FULLSCREEN;
  sVERIFY(WScreenCount>0);
  sSetMem(GeoBuffer,0,sizeof(GeoBuffer));
  sSetMem(GeoHandle,0,sizeof(GeoHandle));

#if sUSE_DIRECTINPUT
  if(!InitDI())
    sFatal("could not initialise Direct Input!");
#endif

	ZBufXSize = ZBufYSize = 0;
	ZBufFormat = 0;
	ZBuffer = 0;
  for(i=0;i<MAX_TEXTURE;i++)
    Textures[i].Flags = 0;
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
  
#if sUSE_DIRECTSOUND
  if(!InitDS())
    sFatal("could not initialize DirectSound");
#endif

  // main loop


  sAppHandler(sAPPCODE_INIT,0);

  msg.message = WM_NULL;
  PeekMessage(&msg,0,0,0,PM_NOREMOVE);
  while(msg.message!=WM_QUIT)
  {
    if(WActiveCount==0)
      gotmsg = (PeekMessage(&msg,0,0,0,PM_REMOVE)!=0);     // !=0) not needed
    else
      gotmsg = (GetMessage(&msg,0,0,0)!=0);

    if(gotmsg)
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    else
    {
      if(WActiveCount==0)
      {
#if sUSE_DIRECTINPUT
        PollDI();
#endif
        for(i=0;i<KeyIndex;i++)
          sAppHandler(sAPPCODE_KEY,KeyBuffer[i]);
        KeyIndex = 0;

        Render();
#if sUSE_DIRECTSOUND
        FillDS();
#endif
#if sLINK_UTIL
        sPerfMon->Flip();
#elif !sINTRO
        sSystem->PerfKalib();
#endif
      }
#if sUSE_DIRECTSOUND
      MarkDS();
#endif
    }
  }
/*
  while(GetMessage(&msg,NULL,0,0)) 
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
*/
  sAppHandler(sAPPCODE_EXIT,0);

// cleanup

#if sUSE_DIRECTSOUND
  ExitDS();
#endif
  ExitScreens();

  for(i=0;i<MAX_TEXTURE;i++)
    Textures[i].Flags=0;

#if sUSE_DIRECTINPUT
  ExitDI();
#endif
  ExitGDI();

#if sLINK_UTIL
  sBroker->RemRoot(sPerfMon);
#endif
  sBroker->Free();
  sBroker->Dump();
  delete sBroker;
  sBroker = 0;

  FreeLibrary(d3dlib);
  FreeLibrary(dilib);
  FreeLibrary(dslib);
}

/****************************************************************************/

sInt sSystemPrivate::Msg(sU32 msg,sU32 wparam,sU32 lparam)
{
  HWND win;
  sInt result;
  sInt nr;
  sInt i;

  nr = -1;
  for(i=0;i<WScreenCount && nr==-1;i++)
  {
    if(MsgWin==(HWND) Screen[i].Window)
      nr = i;
  }

  win = MsgWin;
  if(WAborting || nr==-1)
    return DefWindowProc(win,msg,wparam,lparam);

  result = 0;

  switch(msg)
  {
  case WM_PAINT:

    if(WActiveCount!=0 && !WFullscreen)
      Render();
    break;

  case WM_ENTERSIZEMOVE:
    WActiveCount++;
    break;
  case WM_SIZE:
    if(!WFullscreen)
      WWindowedStyle = GetWindowLong(win,GWL_STYLE);
    if(wparam == SIZE_MINIMIZED)
    {
      WActiveCount++;
      WMinimized = 1;
      WMaximized = 0;
    }
    else if(wparam == SIZE_MAXIMIZED)
    {
      if(WMinimized)
        WActiveCount--;
      WMinimized = 0;
      WMaximized = 1;
    }
    else if(wparam == SIZE_RESTORED)
    {
      if(WMinimized)
        WActiveCount--;
      WMinimized = 0;
      WMaximized = 0;
    }
    if((wparam==SIZE_MAXIMIZED || wparam==SIZE_RESTORED) && WActiveCount==0)
    {
      InitScreens();
      if(KeyIndex < MAX_KEYBUFFER)
        KeyBuffer[KeyIndex++] = sKEY_MODECHANGE;
    }
    break;
  case WM_EXITSIZEMOVE:
    WActiveCount--;
    if(WActiveCount==0)
    {
      InitScreens();
      if(KeyIndex < MAX_KEYBUFFER)
        KeyBuffer[KeyIndex++] = sKEY_MODECHANGE;
    }
    break;
  case WM_SETCURSOR:
    if(WFullscreen)
    {
      SetCursor(0);
      return 0;
    }
    break;
  case WM_ENTERMENULOOP:
    WActiveCount++;
    break;
  case WM_EXITMENULOOP:
    WActiveCount--;
    break;
  case WM_NCHITTEST:
    if(WFullscreen)
      return HTCLIENT;
    break;
  case WM_SYSCOMMAND:
    switch(wparam)
    {
    case SC_MOVE:
    case SC_SIZE:
    case SC_MAXIMIZE:
    case SC_KEYMENU:
    case SC_MONITORPOWER:
      if(WFullscreen)
        return 1;
    }
    break;
  case WM_SYSKEYUP:
  case WM_SYSKEYDOWN:
    if(WFullscreen)
      return 0;
    break;
    /*
  case WM_ACTIVATE:
    switch(wparam&0xffff)
    {
    case WA_ACTIVE:
    case WA_CLICKACTIVE:
      if(!WActiveMsg)
      {
        WActiveCount--;
        WActiveMsg = 1;
      }
      break;

    case WA_INACTIVE:
      if(WActiveMsg)
      {
        WActiveCount++;
        WActiveMsg = 0;
      }
      break;
    }
    break;
*/

  case WM_CLOSE:
    if(KeyIndex<MAX_KEYBUFFER)
      KeyBuffer[KeyIndex++] = sKEY_CLOSE;
    else
      KeyBuffer[KeyIndex-1] = sKEY_CLOSE;
    break;
  case WM_LBUTTONDOWN:
    MouseButtons |= 1;
    MouseButtonsSave |= 1;
    KeyQual |= sKEYQ_MOUSEL;
    if(KeyIndex < MAX_KEYBUFFER)
      KeyBuffer[KeyIndex++] = (sKEY_MOUSEL)|KeyQual;
    SetCapture(win);
    break;
  case WM_RBUTTONDOWN:
    MouseButtons |= 2;
    MouseButtonsSave |= 2;
    KeyQual |= sKEYQ_MOUSER;
    if(KeyIndex < MAX_KEYBUFFER)
      KeyBuffer[KeyIndex++] = (sKEY_MOUSER)|KeyQual;
    SetCapture(win);
    break;
  case WM_MBUTTONDOWN:
    MouseButtons |= 4;
    MouseButtonsSave |= 4;
    KeyQual |= sKEYQ_MOUSEM;
    if(KeyIndex < MAX_KEYBUFFER)
      KeyBuffer[KeyIndex++] = (sKEY_MOUSEM)|KeyQual;
    SetCapture(win);
    break;
  case WM_LBUTTONUP:
    MouseButtons &= ~1;
    KeyQual &= ~sKEYQ_MOUSEL;
    if(KeyIndex < MAX_KEYBUFFER)
      KeyBuffer[KeyIndex++] = (sKEY_MOUSEL)|sKEYQ_BREAK|KeyQual;
    ReleaseCapture();
    break;
  case WM_RBUTTONUP:
    MouseButtons &= ~2;
    KeyQual &= ~sKEYQ_MOUSER;
    if(KeyIndex < MAX_KEYBUFFER)
      KeyBuffer[KeyIndex++] = (sKEY_MOUSER)|sKEYQ_BREAK|KeyQual;
    ReleaseCapture();
    break;
  case WM_MBUTTONUP:
    MouseButtons &= ~4;
    KeyQual &= ~sKEYQ_MOUSEM;
    if(KeyIndex < MAX_KEYBUFFER)
      KeyBuffer[KeyIndex++] = (sKEY_MOUSEM)|sKEYQ_BREAK|KeyQual;
    ReleaseCapture();
    break;
  case WM_MOUSEMOVE:
    WMouseX = (sS16)LOWORD(lparam);
    WMouseY = (sS16)HIWORD(lparam);
#if !sUSE_DIRECTINPUT
    MouseX = WMouseX;
    MouseY = WMouseY;
#endif
    break;
#if !sUSE_DIRECTINPUT
  case WM_CHAR:
    if(KeyIndex < MAX_KEYBUFFER-1)
    {
      KeyBuffer[KeyIndex++] = wparam;
      KeyBuffer[KeyIndex++] = wparam|sKEYQ_BREAK;
    }
    break;
#endif
  }

  return DefWindowProc(win,msg,wparam,lparam);
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
  if(!sAppHandler(sAPPCODE_DEBUGPRINT,(sU32) s))
    OutputDebugString(s);
}

/****************************************************************************/

sNORETURN void sSystem_::Abort(sChar *msg)
{
#if sINTRO
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

void sSystem_::Init(sU32 flags,sInt xs,sInt ys)
{
  ConfigX = xs;
  ConfigY = ys;
  ConfigFlags = flags;
}

/****************************************************************************/

void sSystem_::Reset(sU32 flags,sInt x,sInt y,sInt x2,sInt y2)
{
  ConfigFlags = flags;
  ConfigX = x;
  ConfigY = y;
//  ConfigX2 = x2;
//  CondifY2 = y2;

  InitScreens();

  if(!(flags & (sSF_FULLSCREEN|sSF_MULTISCREEN)))
  {
    MoveWindow((HWND)Screen[0].Window,0,0,x,y,TRUE);
    ShowWindow((HWND)Screen[0].Window,SW_RESTORE);
    InitScreens();
  }

  if(KeyIndex < MAX_KEYBUFFER)
    KeyBuffer[KeyIndex++] = sKEY_MODECHANGE;
}

/****************************************************************************/

void sSystem_::Exit()
{
  PostQuitMessage(0);
}

/****************************************************************************/

sInt sSystem_::MemoryUsed()
{
  return MemoryUsedCount;
}

/****************************************************************************/

void sSystem_::Tag()
{
}

/****************************************************************************/

void *sSystem_::FindFunc(sChar *name)
{
  return GetProcAddress(0,name);
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Input                                                              ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#if sUSE_DIRECTINPUT

static IDirectInput8 *DXI; 
static IDirectInputDevice8 *DXIKey;
static IDirectInputDevice8 *DXIMouse;
static sInt WMouseX;
static sInt WMouseY;
static sInt DXIKeyFocus;
static sInt DXIMouseFocus;

#define MAX_KEYQUEUE    16
#define MAX_MOUSEQUEUE  64

//static sU32 KeyBuffer[MAX_KEYBUFFER];
//static sInt KeyIndex;
//static sU32 KeyQual;
static sU32 KeyMaps[3][256];
static sU32 KeyRepeat;
static sU32 KeyRepeatTimer;
static sU32 KeyRepeatDelay;
static sU32 KeyRepeatRate;
static sU32 KeyStickyMouseX;
static sU32 KeyStickyMouseY;
static sBool KeySticky;
static sInt KeyStickyTime;
//static sU32 MouseX;
//static sU32 MouseY;
//static sU32 MouseZ;
//static sU32 MouseButtons;
//static sU32 MouseButtonsSave;

/****************************************************************************/
/****************************************************************************/

void sSystemPrivate::AddAKey(sU32 *Scans,sInt ascii)
{
  sInt j;
  sU32 scan,vkey;

  vkey = VkKeyScan(ascii);
  if(vkey!=-1)
  {
    scan = MapVirtualKey(vkey&0xff,0);
    for(j=0;j<256;j++)
    {
      if(Scans[j]==scan)
      {
//        sDPrintF("ASCII '%c' %02x -> VKey %04x -> Scan %02x -> DX %02x\n",ascii,ascii,vkey,scan,j);
        switch(vkey&0xff00)
        {
        case 0x0000:
          KeyMaps[0][j] = ascii;      // normal
          break;
        case 0x0100:
          KeyMaps[1][j] = ascii;      // shift
          break;
        case 0x0600:
          KeyMaps[2][j] = ascii;      // alt-gr
          break;
        }
        break;
      }
    }
  }
}

/****************************************************************************/

sBool sSystemPrivate::InitDI()
{
  HRESULT hr;
  DIPROPDWORD  prop; 
  sU32 Scans[256];
  sInt i;
  static sInt dkeys[][2] = 
  {
    { DIK_BACK    ,sKEY_BACKSPACE   },
    { DIK_TAB     ,sKEY_TAB         },
    { DIK_RETURN  ,sKEY_ENTER       },
    { DIK_ESCAPE  ,sKEY_ESCAPE      },

    { DIK_UP      ,sKEY_UP          },
    { DIK_DOWN    ,sKEY_DOWN        },
    { DIK_LEFT    ,sKEY_LEFT        },
    { DIK_RIGHT   ,sKEY_RIGHT       },
    { DIK_PRIOR   ,sKEY_PAGEUP      },
    { DIK_NEXT    ,sKEY_PAGEDOWN    },
    { DIK_HOME    ,sKEY_HOME        },
    { DIK_END     ,sKEY_END         },
    { DIK_INSERT  ,sKEY_INSERT      },
    { DIK_DELETE  ,sKEY_DELETE      },

    { DIK_PAUSE   ,sKEY_PAUSE       },
    { DIK_SCROLL  ,sKEY_SCROLL      },
    { DIK_LWIN    ,sKEY_WINL        },
    { DIK_RWIN    ,sKEY_WINR        },
    { DIK_APPS    ,sKEY_APPS        },

    { DIK_LSHIFT  ,sKEY_SHIFTL      },
    { DIK_RSHIFT  ,sKEY_SHIFTR      },
    { DIK_CAPITAL ,sKEY_CAPS        },
    { DIK_NUMLOCK ,sKEY_NUMLOCK     },
    { DIK_LCONTROL,sKEY_CTRLL       },
    { DIK_RCONTROL,sKEY_CTRLR       },
    { DIK_LMENU   ,sKEY_ALT         },
    { DIK_RMENU   ,sKEY_ALTGR       },

    { DIK_F1      ,sKEY_F1          },
    { DIK_F2      ,sKEY_F2          },
    { DIK_F3      ,sKEY_F3          },
    { DIK_F4      ,sKEY_F4          },
    { DIK_F5      ,sKEY_F5          },
    { DIK_F6      ,sKEY_F6          },
    { DIK_F7      ,sKEY_F7          },
    { DIK_F8      ,sKEY_F8          },
    { DIK_F9      ,sKEY_F9          },
    { DIK_F10     ,sKEY_F10         },
    { DIK_F11     ,sKEY_F11         },
    { DIK_F12     ,sKEY_F12         },

    { 0,0 }
  };

// set up direct input


  hr = (*DirectInput8CreateP)(WInst,DIRECTINPUT_VERSION,IID_IDirectInput8,(void**)&DXI,0);
  if(FAILED(hr)) return sFALSE;

// set up keyboard

  hr = DXI->CreateDevice(GUID_SysKeyboard,&DXIKey,0);
  if(FAILED(hr)) return sFALSE;

  hr = DXIKey->SetDataFormat(&c_dfDIKeyboard); 
  if(FAILED(hr)) return sFALSE;

  hr = DXIKey->SetCooperativeLevel((HWND)Screen[0].Window,DISCL_FOREGROUND|DISCL_EXCLUSIVE);
  if(FAILED(hr)) return sFALSE;

  prop.diph.dwSize = sizeof(DIPROPDWORD); 
  prop.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
  prop.diph.dwObj = 0; 
  prop.diph.dwHow = DIPH_DEVICE; 
  prop.dwData = MAX_KEYQUEUE; 
  hr = DXIKey->SetProperty(DIPROP_BUFFERSIZE,&prop.diph); 
  if(FAILED(hr)) return sFALSE;

// load keyboard mapping

  for(i=0;i<256;i++)
  {
    prop.diph.dwSize = sizeof(DIPROPDWORD); 
    prop.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
    prop.diph.dwObj = i; 
    prop.diph.dwHow = DIPH_BYOFFSET; 
    prop.dwData = 0;
    DXIKey->GetProperty(DIPROP_SCANCODE,&prop.diph);
    Scans[i] = prop.dwData;
  }
  for(i=0;dkeys[i][0];i++)
    KeyMaps[0][dkeys[i][0]] = dkeys[i][1];
  for(i=32;i<127;i++)
    AddAKey(Scans,i);
  for(i=160;i<256;i++)
    AddAKey(Scans,i);

// init key tables

  KeyIndex = 0;
  KeyQual = 0;
  KeyRepeat = 0;
  KeyRepeatTimer = 0;
  KeyRepeatDelay = 200;
  KeyRepeatRate = 20;

// start the keyboard

  hr = DXIKey->Acquire();
  DXIKeyFocus = 1;
//  if(FAILED(hr)) return sFALSE;

// create mouse 

  hr = DXI->CreateDevice(GUID_SysMouse,&DXIMouse,0);
  if(FAILED(hr)) return sFALSE;

  hr = DXIMouse->SetDataFormat(&c_dfDIMouse); 
  if(FAILED(hr)) return sFALSE;

  hr = DXIMouse->SetCooperativeLevel((HWND)Screen[0].Window,DISCL_FOREGROUND|DISCL_NONEXCLUSIVE);
  if(FAILED(hr)) return sFALSE;

  prop.diph.dwSize = sizeof(DIPROPDWORD); 
  prop.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
  prop.diph.dwObj = 0; 
  prop.diph.dwHow = DIPH_DEVICE; 
  prop.dwData = MAX_MOUSEQUEUE; 
  hr = DXIMouse->SetProperty(DIPROP_BUFFERSIZE,&prop.diph); 
  if(FAILED(hr)) return sFALSE;

// init mouse tables

  MouseX = 0;
  MouseY = 0;
  MouseZ = 0;
  MouseButtons = 0;
  MouseButtonsSave = 0;

// start mouse

  hr = DXIMouse->Acquire(); 
  DXIMouseFocus = 1;
//  if(FAILED(hr)) return sFALSE;

  return TRUE; 
}

/****************************************************************************/

void sSystemPrivate::ExitDI()
{
  if(DXIKey)
  {
    DXIKey->Unacquire();
    DXIKey->Release();
    DXIKey = 0;
  }
  if(DXIMouse)
  {
    DXIMouse->Unacquire();
    DXIMouse->Release();
    DXIMouse = 0;
  }
  RELEASE(DXI);
}

/****************************************************************************/

void sSystemPrivate::PollDI()
{
  DIDEVICEOBJECTDATA data[MAX_MOUSEQUEUE];
  DWORD count; 
  HRESULT hr;
  sInt i;
  sU32 code;
  sU32 key;
  sU32 qual;
  sInt time;

// check for window

  if(WActiveCount!=0)
    return;

// get keys

  if(!DXIKeyFocus)
  {
    hr = DXIKey->Acquire();
    if(!FAILED(hr))
      DXIKeyFocus = 1;
  }
  if(DXIKeyFocus)
  {
    count = MAX_KEYQUEUE;
    hr = DXIKey->GetDeviceData(sizeof(DIDEVICEOBJECTDATA),data,&count,0);
    if(hr==DIERR_INPUTLOST || hr==DIERR_NOTACQUIRED)
    {
      DXIKeyFocus = 0;
    }
  }
  time = sSystem->GetTime();
  if(DXIKeyFocus)
  {
    if(hr==DI_BUFFEROVERFLOW)
    {
      KeyQual=0;
      sDPrintF("Direct Input Buffer Overflow\n");
    }
    if(!FAILED(hr))
    {

// done

      for(i=0;i<(sInt)count;i++)
      {
        if(i<(sInt)count-1)   // direct input bugs
        {
          if((data[i].dwData&0x80) && (data[i+1].dwData&0x80))
          {
            if(data[i].dwOfs==0x1d && data[i+1].dwOfs==0xb8)    // altgr always comes with control
              continue;
          }
          /*
          if(!(data[i].dwData&0x80) && (data[i+1].dwData&0x80) && (data[i].dwOfs==0x36))
          {
            if(data[i+1].dwOfs==0xc7 || data[i+1].dwOfs==0xc8 || data[i+1].dwOfs==0xc9 || data[i+1].dwOfs==0xcb || 
              data[i+1].dwOfs==0xcd || data[i+1].dwOfs==0xcf || data[i+1].dwOfs==0xd0 || data[i+1].dwOfs==0xd1 || 
              data[i+1].dwOfs==0xd2 || data[i+1].dwOfs==0xd3)
            {
              continue;
            }
          }
          */
        }

        
        code = data[i].dwOfs&0xff;
        qual = sKEYQ_BREAK;
        if(data[i].dwData&0x80)
          qual = 0;

        key = KeyMaps[0][code];
        if(key>=sKEY_SHIFTL && key<=sKEY_ALTGR)
        {
          if(qual&sKEYQ_BREAK)
            KeyQual &= ~(sKEYQ_SHIFTL<<(key-sKEY_SHIFTL));
          else
            KeyQual |=  (sKEYQ_SHIFTL<<(key-sKEY_SHIFTL));          
        }
  //      sDPrintF("%02x %02x -> %08x\n",data[i].dwOfs,data[i].dwData,KeyQual);

        key = 0;
        if(KeyQual & sKEYQ_ALTGR)
          key = KeyMaps[2][code];
        if(key == 0 && (KeyQual & sKEYQ_SHIFT))
          key = KeyMaps[1][code];
        if(key == 0)
          key = KeyMaps[0][code];
        key |= KeyQual;
        key |= qual;

        if(key&sKEYQ_BREAK)
        {
          KeyRepeat = 0;
          KeyRepeatTimer = 0;
          if(sSystem->GetTime() - KeyStickyTime > 250)
            key |= sKEYQ_STICKYBREAK;
        }
        else
        {
          KeyRepeat = key;
          KeyRepeatTimer = time+KeyRepeatDelay;
          KeyStickyMouseX = MouseX;
          KeyStickyMouseY = MouseY;
          KeyStickyTime = sSystem->GetTime();
          KeySticky = sTRUE;
        }
        if(KeyIndex<MAX_KEYBUFFER)
          KeyBuffer[KeyIndex++] = key;

  //      sDPrintF("key %02x -> %08x '%c'\n",code,key,(key&0xff)>=32 ? key&0xff : '?');
      }
    }
  }
  if(sAbs(KeyStickyMouseX-MouseX)+sAbs(KeyStickyMouseY-MouseY)>4)
    KeySticky = sFALSE;

  if(KeyRepeat!=0 && time>=(sInt)KeyRepeatTimer && KeyIndex<MAX_KEYBUFFER)
  {
    KeyBuffer[KeyIndex++] = KeyRepeat|sKEYQ_REPEAT;
    KeyRepeatTimer += KeyRepeatRate;
    if((sInt)KeyRepeatTimer<time)
      KeyRepeatTimer=time;
  }

// get mouse

  if(!DXIMouseFocus)
  {
    hr = DXIMouse->Acquire();
    if(!FAILED(hr))
      DXIMouseFocus = 1;
  }
  if(DXIMouseFocus)
  {
    count = MAX_MOUSEQUEUE;
    hr = DXIMouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA),data,&count,0);
    if(hr==DIERR_INPUTLOST || hr==DIERR_NOTACQUIRED)
      DXIMouseFocus = 0;
  }
  if(DXIMouseFocus)
  {
    if(!FAILED(hr))
    {
      for(i=0;i<(sInt)count;i++)
      {
        switch(data[i].dwOfs)
        {
        case DIMOFS_X:
          MouseX += data[i].dwData;
          break;
        case DIMOFS_Y:
          MouseY += data[i].dwData;
          break;
        case DIMOFS_Z:
          MouseZ += data[i].dwData;
          break;
  /*
        case DIMOFS_BUTTON0:
        case DIMOFS_BUTTON1:
        case DIMOFS_BUTTON2:
        case DIMOFS_BUTTON3:
        case DIMOFS_BUTTON4:
        case DIMOFS_BUTTON5:
        case DIMOFS_BUTTON6:
        case DIMOFS_BUTTON7:
          key = (data[i].dwOfs-DIMOFS_BUTTON0);
          if(data[i].dwData&0x80)
          {
            MouseButtons |=  (1<<key);
            MouseButtonsSave |=  (1<<key);
            KeyQual |= (sKEYQ_MOUSEL<<key);
            if(KeyIndex < MAX_KEYBUFFER)
              KeyBuffer[KeyIndex++] = (sKEY_MOUSEL+key)|KeyQual;
          }
          else
          {
            MouseButtons &= ~( (1<<key) | ((MouseButtons&8)?4:0) ); // buggy mouse driver fix
            KeyQual &= ~(sKEYQ_MOUSEL<<key);
            if(KeyIndex < MAX_KEYBUFFER)
              KeyBuffer[KeyIndex++] = (sKEY_MOUSEL+key)|KeyQual|sKEYQ_BREAK;
          }
          break;
  */
        }
      }
    }
  }
} 

#endif

/****************************************************************************/
/****************************************************************************/

void sSystem_::GetInput(sInt id,sInputData &data)
{
  data.Type = sIDT_NONE;
  data.AnalogCount = 0;
  data.DigitalCount = 0;
  data.pad = 0;
  data.Analog[0] = 0;
  data.Analog[1] = 0;
  data.Analog[2] = 0;
  data.Analog[3] = 0;
  data.Digital = 0;
  if(id==0)
  {
    data.Type = sIDT_MOUSE;
    data.AnalogCount = 3;
    data.DigitalCount = 3;
    data.Analog[0] += MouseX;
    data.Analog[1] += MouseY;
    data.Analog[2] += MouseZ;
    data.Analog[3] = 0;
    data.Digital = MouseButtons | MouseButtonsSave;
    MouseButtonsSave = 0;
  }
}

/****************************************************************************/

sU32 sSystem_::GetKeyboardShiftState()
{
  return (KeyQual&0x7ffe0000);
}

/****************************************************************************/

void sSystem_::GetKeyName(sChar *buffer,sInt size,sU32 key)
{
#if !sINTRO
  sU32 pure;
  sChar c2[2];
  static sChar *names[] = 
  {
    "UP","DOWN","LEFT","RIGHT",
    "PAGEUP","PAGEDOWN","HOME","END",
    "INSERT","DELETE",0,0,
    0,0,0,0,

    "PAUSE","SCROLL","NUMLOCK","WINL",
    "WINR","APPS","CLOSE","SIZE",
    0,0,0,0,
    0,0,0,0,

    0,"SHIFTL","SHIFTR","CAPS",
    "CTRLL","CTRLR","ALT","ALTGR",
    "MOUSEL","MOUSER","MOUSEM",0,
    0,0,0,0,

    "F1","F2","F3","F4",
    "F5","F6","F7","F8",
    "F9","F10","F11","F12",
    0,0,0,0,
  };

  pure = key & 0x0001ffff;
  buffer[0] = 0;

  if(key & sKEYQ_SHIFT)
    sAppendString(buffer,"SHIFT+",size);
  if(key & sKEYQ_CTRL)
    sAppendString(buffer,"CTRL+",size);
  if(key & sKEYQ_ALTGR)
    sAppendString(buffer,"ALTGR+",size);
  if(key & sKEYQ_ALT)
    sAppendString(buffer,"ALT+",size);

  if((pure>=0x20 && pure<=0x7f) || (pure>=0xa0 && pure<0xff))
  {
    c2[0] = pure;
    c2[1] = 0;
    sAppendString(buffer,c2,size);
  }
  else if(pure>=0x00010000 && pure<0x00010040 && names[pure&0x3f])
    sAppendString(buffer,names[pure&0x3f],size);
  else if(pure==sKEY_BACKSPACE)
    sAppendString(buffer,"BACKSPACE",size);
  else if(pure==sKEY_TAB)
    sAppendString(buffer,"TAB",size);
  else if(pure==sKEY_ENTER)
    sAppendString(buffer,"ENTER",size);
  else if(pure==sKEY_ESCAPE)
    sAppendString(buffer,"ESCAPE",size);
  else
    sAppendString(buffer,"???",size);
#endif
}

/****************************************************************************/

sInt sSystem_::GetTime()
{
  return timeGetTime()-WStartTime;
}

#if !sINTRO

sInt sSystem_::GetTimeOfDay()
{
  static SYSTEMTIME st;
  static sInt cache;
  sInt time;

  time = GetTime();
  if(time>cache+500)
  {
    GetLocalTime(&st);
    cache = time;
  }

  return st.wSecond+st.wMinute*60+st.wHour*60*60;
}

sU32 sSystem_::PerfTime()
{
  sU64 time;
  _asm
  {
    push eax
    push edx
    rdtsc
    mov DWORD PTR time,eax
    mov DWORD PTR time+4,edx
    pop edx
    pop eax
  }
  return (time-sPerfFrame)*sPerfKalibFactor;
}

void sSystem_::PerfKalib()
{
  sU64 query;
  sU64 freq;
  sU64 time;

  _asm
  {
    push eax
    push edx
    rdtsc
    mov DWORD PTR time,eax
    mov DWORD PTR time+4,edx
    pop edx
    pop eax
  }
  sPerfFrame = time;

  if(time-sPerfKalibRDTSC>1000*1000*1000)
  {
    QueryPerformanceCounter((LARGE_INTEGER *)&query);
    QueryPerformanceFrequency((LARGE_INTEGER *)&freq);

    sPerfKalibFactor = (((sF64)(query-sPerfKalibQuery))/freq) / ((sF64)(time-sPerfKalibRDTSC)) * 1000000;      
    sPerfKalibRDTSC=0;
  }
  if(sPerfKalibRDTSC==0)
  {
    sPerfKalibRDTSC = time;
    QueryPerformanceCounter((LARGE_INTEGER *)&query);
    sPerfKalibQuery = query;
  }
}

#endif

/****************************************************************************/
/****************************************************************************/

sBool sSystem_::GetWinMouse(sInt &x,sInt &y)
{
  x = WMouseX;
  y = WMouseY;
  return sTRUE;
}

/****************************************************************************/

void sSystem_::SetWinMouse(sInt x,sInt y)
{
  SetCursorPos(x,y);
}

/****************************************************************************/

void sSystem_::SetWinTitle(sChar *name)
{
  SetWindowText((HWND)Screen[0].Window,name);
}

/****************************************************************************/

void sSystem_::HideWinMouse(sBool hide)
{
  if(hide)
    while(ShowCursor(0)>0);
  else
    ShowCursor(1);
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Sound                                                              ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#if sUSE_DIRECTSOUND

static IDirectSound8  *DXS;
static IDirectSoundBuffer8 *DXSBuffer;
static IDirectSoundBuffer *DXSPrimary;
static volatile sSoundHandler DXSHandler;
static sInt DXSHandlerAlign;
static sInt DXSHandlerSample;
static volatile void *DXSHandlerUser;
static sInt DXSLastFrameTime;

#define DXSSAMPLES 0x2000

static sInt DXSWriteSample;
static sInt DXSReadSample;
static sInt DXSLastTotalSample;

static HANDLE DXSThread;
static ULONG DXSThreadId;
static sInt volatile DXSRun;
static CRITICAL_SECTION DXSLock;
static HANDLE DXSEvent;

unsigned long __stdcall ThreadCode(void *);

/****************************************************************************/

#if sINTRO
static const GUID IID_IDirectSoundBuffer8 = { 0x6825a449, 0x7524, 0x4d82, 0x92, 0x0f, 0x50, 0xe3, 0x6a, 0xb3, 0xab, 0x1e };
#endif

sBool sSystemPrivate::InitDS()
{
  DWORD count1,count2;
  void *pos1,*pos2;
  HRESULT hr;
  DSBUFFERDESC dsbd;
  WAVEFORMATEX format;
  IDirectSoundBuffer *sbuffer;

  hr = (*DirectSoundCreate8P)(0,&DXS,0);
  if(FAILED(hr)) return sFALSE;

  hr = DXS->SetCooperativeLevel((HWND)Screen[0].Window,DSSCL_PRIORITY);
  if(FAILED(hr)) return sFALSE;


  WINSET(dsbd);
  dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
  dsbd.dwBufferBytes = 0;
  dsbd.lpwfxFormat = 0;
  hr = DXS->CreateSoundBuffer(&dsbd,&DXSPrimary,0);
  if(FAILED(hr)) return sFALSE;

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
  if(FAILED(hr)) return sFALSE;
  hr = sbuffer->QueryInterface(IID_IDirectSoundBuffer8,(void**)&DXSBuffer);
  sbuffer->Release();
  if(FAILED(hr)) return sFALSE;

  hr = DXSBuffer->Lock(0,DXSSAMPLES*4,&pos1,&count1,&pos2,&count2,0);
  if(!FAILED(hr))
  {
    sSetMem(pos1,0,count1);
    sSetMem(pos2,0,count2);
    DXSBuffer->Unlock(pos1,count1,pos2,count2);
  }

//  DXSBuffer->SetFrequency(44100);
//  DXSBuffer->SetVolume(0);
//  DXSBuffer->SetPan(0);

  DXSEvent = CreateEvent(0,0,0,0);
  if(!DXSEvent)
    return sFALSE;

  InitializeCriticalSection(&DXSLock);

  DXSBuffer->Play(0,0,DSBPLAY_LOOPING);

  DXSRun = 0;
  DXSThread = CreateThread(0,16384,ThreadCode,0,0,&DXSThreadId);
  if(!DXSThread)
    return sFALSE;

//  SetThreadPriority(DXSThread,THREAD_PRIORITY_TIME_CRITICAL);
//  SetPriorityClass(DXSThread,HIGH_PRIORITY_CLASS);
  return sTRUE;
}

/****************************************************************************/

void sSystemPrivate::ExitDS()
{
  if(DXSBuffer)
    DXSBuffer->Stop();

  if(DXSThread)
  {
    DXSRun = 1;
    while(DXSRun==1) 
      Sleep(10);

    CloseHandle(DXSThread);
    CloseHandle(DXSEvent);
    DeleteCriticalSection(&DXSLock);
  }

  if(DXSBuffer)
    DXSBuffer->Release();
  if(DXSPrimary)
    DXSPrimary->Release();
  if(DXS)
    DXS->Release();
}

/****************************************************************************/

void sSystemPrivate::MarkDS()
{
  DWORD play,write;
  HRESULT hr;
  sInt delta;

  EnterCriticalSection(&DXSLock);
  hr = DXSBuffer->GetCurrentPosition(&play,&write);
  if(!FAILED(hr))
  {
    delta = play-DXSReadSample;
    if(delta<0)
      delta+=DXSSAMPLES*4;
    if(delta>0 /*&& delta<0x10000*/)                           // 0x10000 = 3680ms = 2.7 fps
      DXSLastTotalSample = DXSWriteSample + delta/4;
  }
  LeaveCriticalSection(&DXSLock);
//  sDPrintF("%08x:%08x=%08x+%d\n",GetTime(),DXSLastTotalSample,DXSWriteSample,delta);

  DXSLastTotalSample-=DXSSAMPLES;
}

/****************************************************************************/

void sSystemPrivate::FillDS()
{
  SetEvent(DXSEvent);
/*
  HRESULT hr;
  sInt play,write;
  DWORD count1,count2;
  void *pos1,*pos2;
  static sInt index=0;
  const sInt SAMPLESIZE = 4;
  sInt size,pplay;

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
      

        if(DXSHandler)
        {
          if(count1>0)
            (*DXSHandler)((sS16 *)pos1,count1/SAMPLESIZE,DXSHandlerUser);
          if(count2>0)
            (*DXSHandler)((sS16 *)pos2,count2/SAMPLESIZE,DXSHandlerUser);
        }
        DXSBuffer->Unlock(pos1,count1,pos2,count2);
      }
    }   
  }

  LeaveCriticalSection(&DXSLock);
*/
}

/****************************************************************************/

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


      size = (size)&(~(DXSHandlerAlign-1));
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
          DXSReadSample = pplay&(~(DXSHandlerAlign-1));
          sVERIFY((sInt)(count1+count2)==(size*4));
        
#if sLINK_UTIL
          if(sPerfMon && sPerfMon->IndexSound<0xfc)
          {
            sPerfMon->SoundRec[sPerfMon->DBuffer][sPerfMon->IndexSound] = sSystem->PerfTime();
          }
#endif
          if(DXSHandler)
          {
            if(count1>0)
              (*DXSHandler)((sS16 *)pos1,count1/SAMPLESIZE,(void *)DXSHandlerUser);
            if(count2>0)
              (*DXSHandler)((sS16 *)pos2,count2/SAMPLESIZE,(void *)DXSHandlerUser);
          }
          DXSBuffer->Unlock(pos1,count1,pos2,count2);

#if sLINK_UTIL
          if(sPerfMon && sPerfMon->IndexSound<0xfc)
          {
            sPerfMon->SoundRec[sPerfMon->DBuffer][sPerfMon->IndexSound+1] = sSystem->PerfTime();
            sPerfMon->IndexSound += 2;
          }
#endif
        }
      }   
    }
    LeaveCriticalSection(&DXSLock);
  }
  DXSRun = sFALSE;
  return 0;
}

#endif

/****************************************************************************/
/****************************************************************************/

static void DSNullHandler(sS16 *data,sInt samples,void *user)
{
  static sInt p0,p1;

//  sSetMem4((sU32 *)data,0x00000000,samples);

  while(samples>0)
  {
    *data++ = sFSin(p0/65536.0f)*0x4000; p0+=5010;
    *data++ = sFSin(p1/65536.0f)*0x4000; p1+=5050;
    samples--;
  }
}

/****************************************************************************/

void sSystem_::SetSoundHandler(sSoundHandler hnd,sInt align,void *user)
{
#if sUSE_DIRECTSOUND
  EnterCriticalSection(&DXSLock);
  if(hnd)
    DXSHandler = hnd;
  else
    DXSHandler = DSNullHandler;
  if(align<0)
    align=64;
  DXSHandlerAlign = align;
  DXSHandlerSample = 0;
  DXSHandlerUser = user;
  LeaveCriticalSection(&DXSLock);
#endif
}

/****************************************************************************/

sInt sSystem_::GetCurrentSample()
{
#if sUSE_DIRECTSOUND
  return DXSLastTotalSample;//DXSLastFrameTime;
#else
  return 0;
#endif
}

/****************************************************************************/
/***                                                                      ***/
/***   File                                                               ***/
/***                                                                      ***/
/****************************************************************************/

sU8 *sSystem_::LoadFile(sChar *name,sInt &size)
{
  sInt result;
  HANDLE handle;
  DWORD test;
  sU8 *mem;
   
  mem = 0;
  result = sFALSE;
  handle = CreateFile(name,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
  if(handle != INVALID_HANDLE_VALUE)
  {
    size = GetFileSize(handle,&test);
    if(test==0)
    {
      mem = new sU8[size];
      if(ReadFile(handle,mem,size,&test,0))
        result = sTRUE;
      if(size!=(sInt)test)
        result = sFALSE;
    }
    CloseHandle(handle);
  }

  if(!result)
  {
    if(mem)
      delete[] mem;
    mem = 0;
  }

  return mem;
}

/****************************************************************************/

sU8 *sSystem_::LoadFile(sChar *name)
{
  sInt dummy;
  return LoadFile(name,dummy);
}

/****************************************************************************/

sChar *sSystem_::LoadText(sChar *name)
{
  sInt result;
  HANDLE handle;
  DWORD test;
  sU8 *mem;
  sInt size;
   
  mem = 0;
  result = sFALSE;
  handle = CreateFile(name,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
  if(handle != INVALID_HANDLE_VALUE)
  {
    size = GetFileSize(handle,&test);
    if(test==0)
    {
      mem = new sU8[size+1];
      if(ReadFile(handle,mem,size,&test,0))
        result = sTRUE;
      if(size!=(sInt)test)
        result = sFALSE;
      mem[size]=0;
    }
    CloseHandle(handle);
  }

  if(!result)
  {
    delete[] mem;
    mem = 0;
  }

  return (sChar *)mem;}

/****************************************************************************/

sBool sSystem_::SaveFile(sChar *name,sU8 *data,sInt size)
{
  sInt result;
  HANDLE handle;
  DWORD test;

  result = sFALSE;
  handle = CreateFile(name,GENERIC_WRITE,FILE_SHARE_WRITE,0,CREATE_NEW,0,0);  
  if(handle == INVALID_HANDLE_VALUE)
    handle = CreateFile(name,GENERIC_WRITE,FILE_SHARE_WRITE,0,TRUNCATE_EXISTING,0,0);  
  if(handle != INVALID_HANDLE_VALUE)
  {
    if(WriteFile(handle,data,size,&test,0))
      result = sTRUE;
    if(size!=(sInt)test)
      result = sFALSE;
    CloseHandle(handle);
  }

  return result;
}

/****************************************************************************/
/****************************************************************************/

#define sMAX_PATHNAME 4096
sDirEntry *sSystem_::LoadDir(sChar *name)
{
#if !sINTRO
  static sChar buffer[sMAX_PATHNAME];
  HANDLE handle;
  sInt len;
  sInt i,j;
  WIN32_FIND_DATA dir;
  sDirEntry *de;
  sInt max;

  de = 0;

  sCopyString(buffer,name,sMAX_PATHNAME);
  len = sGetStringLen(buffer);
  if(name[len-1]!='/' && name[len-1]!='\\')
    sAppendString(buffer,"/",sMAX_PATHNAME);
  sAppendString(buffer,"*.*",sMAX_PATHNAME);

  handle = FindFirstFile(buffer,&dir);
  if(handle!=INVALID_HANDLE_VALUE)
  {
    max = 1024;
    i=0;
    de = new sDirEntry[max];
    do
    {
      if(sCmpString(dir.cFileName,".")==0)
        continue;
      if(sCmpString(dir.cFileName,"..")==0)
        continue;
      de[i].IsDir = (dir.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
      de[i].Size = dir.nFileSizeLow;
      sCopyString(de[i].Name,dir.cFileName,sizeof(de[i].Name));
      de[i].pad[0]=0;
      de[i].pad[1]=0;
      de[i].pad[2]=0;
      i++;
    }
    while(FindNextFile(handle,&dir) && i<max-1);
    FindClose(handle);
    sSetMem(&de[i],0,sizeof(sDirEntry));

    max = i;

    for(i=0;i<max-1;i++)
      for(j=i+1;j<max;j++)
        if(de[i].IsDir<de[j].IsDir || (de[i].IsDir==de[j].IsDir && sCmpStringI(de[i].Name,de[j].Name)>0))
          sSwap(de[i],de[j]);
  }

  return de;
#else
	return 0;
#endif
}

/****************************************************************************/

sBool sSystem_::MakeDir(sChar *name)
{
#if !sINTRO
  return CreateDirectory(name,0) != 0;
#else
	return sFALSE;
#endif
}

/****************************************************************************/

sBool sSystem_::CheckDir(sChar *name)
{
#if !sINTRO
  sBool result;
  static sChar buffer[sMAX_PATHNAME];
  HANDLE handle;
  sInt len;
  WIN32_FIND_DATA dir;

  sCopyString(buffer,name,sMAX_PATHNAME);
  len = sGetStringLen(buffer);
  if(name[len-1]!='/' && name[len-1]!='\\')
    sAppendString(buffer,"/",sMAX_PATHNAME);
  sAppendString(buffer,"*.*",sMAX_PATHNAME);

  handle = FindFirstFile(buffer,&dir);
  result = (handle!=INVALID_HANDLE_VALUE);
  FindClose(handle);

  return result;
#else
	return sFALSE;
#endif
}
#undef sMAX_PATHNAME

/****************************************************************************/

sBool sSystem_::RenameFile(sChar *oldname,sChar *newname)
{
  sBool result;

  result = sFALSE;
//  if(CheckFileName(name))
  {
    result = MoveFile(oldname,newname);
  }

  return result;
}

/****************************************************************************/

sBool sSystem_::DeleteFile(sChar *name)
{
  return ::DeleteFileA(name);
}

/****************************************************************************/

void sSystem_::GetCurrentDir(sChar *buffer,sInt size)
{
#if !sINTRO
  sChar *s;
  if(::GetCurrentDirectoryA(size,buffer))
  {
    s = buffer;
    while(*s)
    {
      if(*s=='\\')
        *s = '/';
      s++;
    }
  }
  else
  {
    sCopyString(buffer,"c:/",size);
  }
#endif
}

/****************************************************************************/

sU32 sSystem_::GetDriveMask()
{
  return GetLogicalDrives();
}

/****************************************************************************/
/***                                                                      ***/
/***   Graphics                                                           ***/
/***                                                                      ***/
/****************************************************************************/

sBool sSystem_::GetScreenInfo(sInt i,sScreenInfo &info)
{
  if(i<0||i>=WScreenCount)
  {
    info.XSize = 0;
    info.YSize = 0;
    info.FullScreen = 0;
    info.SwapVertexColor = 0;
    info.PixelRatio = 1;
    return sFALSE;
  }
  else
  {
    info.XSize = Screen[i].XSize;
    info.YSize = Screen[i].YSize;
    info.FullScreen = WFullscreen;
    info.SwapVertexColor = (ConfigFlags & sSF_OPENGL)?1:0;
    info.PixelRatio = 1.0f*Screen[i].XSize/Screen[i].YSize;
    return sTRUE;
  }
}

sInt sSystem_::GetScreenCount()
{
  return WScreenCount;
}

sBool sSystem_::GetFullscreen()
{
  return WFullscreen;
}

/****************************************************************************/
/****************************************************************************/

void sSystem_::FlushTexture(sInt handle)
{
  sHardTex *tex;

  sVERIFY(handle>=0 && handle<MAX_TEXTURE);
  tex = &Textures[handle];
  sVERIFY(tex->Flags);

  if(tex->Tex)
  {
    tex->Tex->Release();
    tex->Tex = 0;
  }
}

/****************************************************************************/
/****************************************************************************/

void sSystem_::UpdateTexture(sInt handle,sBitmap *bm)
{
  sHardTex *tex;
  sBitmapLock lock;
  sInt xs,ys,level;

  sInt x,y;
  sInt bpr,oxs;
  sU32 *d,*s,*data;
  sU8 *db,*sb;

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
			StreamTextureStart(handle,level,lock);
			bpr = lock.BPR;
			d = (sU32 *)lock.Data;
			s = data;
			for(y=0;y<ys;y++)
			{
				sCopyMem4(d,s,xs);
				d+=bpr/4;
				s+=oxs;
			}
			StreamTextureEnd();

			if(xs<=1 || ys<=1)
				break;
	    
			s = data;
			d = data;
			for(y=0;y<ys/2;y++)
			{
				sb = (sU8 *) s;
				db = (sU8 *) d;
				s+=oxs*2;
				d+=oxs;
				for(x=0;x<xs/2;x++)
				{
					db[0] = (sb[0]+sb[4]+sb[oxs*4+0]+sb[oxs*4+4]+2)>>2;
					db[1] = (sb[1]+sb[5]+sb[oxs*4+1]+sb[oxs*4+5]+2)>>2;
					db[2] = (sb[2]+sb[6]+sb[oxs*4+2]+sb[oxs*4+6]+2)>>2;
					db[3] = (sb[3]+sb[7]+sb[oxs*4+3]+sb[oxs*4+7]+2)>>2;
					db+=4;
					sb+=8;
				}
			}

			xs=xs>>1;
			ys=ys>>1;
			level++;
		}

	  delete[] data;
	}
}

/****************************************************************************/

void sSystem_::UpdateTexture(sInt handle,sU16 *source)
{
  sHardTex *tex;
  sBitmapLock lock;
  sInt xs,ys,level;

  sInt x,y;
  sInt bpr,oxs;
  sU32 *d,*s,*data;
  sU16 *d16,*s16;
  sU8 *d8;

  tex = &Textures[handle];

	if(!(tex->Flags & sTIF_RENDERTARGET))
	{
		xs = tex->XSize;
		ys = tex->YSize;
		oxs = xs;
		level = 0;

		data = new sU32[xs*ys*2];
		sCopyMem4(data,(sU32 *)source,xs*ys*2);

		for(;;)
		{
			StreamTextureStart(handle,level,lock);
			bpr = lock.BPR;
			d = (sU32 *)lock.Data;
			s = data;
			for(y=0;y<ys;y++)
			{
        switch(tex->Format)
        {
        case sTF_A8R8G8B8:
          s16 = (sU16 *)s;
          for(x=0;x<xs;x++)
          {
            d[x] = (((s16[0]>>7)&0xff)    ) | 
                   (((s16[1]>>7)&0xff)<< 8) | 
                   (((s16[2]>>7)&0xff)<<16) | 
                   (((s16[3]>>7)&0xff)<<24);
            s16+=4;
          }
          break;

        case sTF_A8:
          s16 = (sU16 *)s;
          d8 = (sU8 *)d;

          for(x=0;x<xs;x++)
          {
            d8[x] = (((s16[1]>>7)&0xff)    );
            s16+=4;
          }
          break;

        case sTF_Q8W8V8U8:
          s16 = (sU16 *)s;
          for(x=0;x<xs;x++)
          {
            d[x] = (((((sInt)s16[2]-0x4000)>>7)&0xff)    ) | 
                   (((((sInt)s16[1]-0x4000)>>7)&0xff)<< 8) | 
                   (((((sInt)s16[0]-0x4000)>>7)&0xff)<<16) | 
                   (((((sInt)s16[3]-0x4000)>>7)&0xff)<<24);
            s16+=4;
          }
          break;
        }
				d+=bpr/4;
				s+=oxs*2;
			}
			StreamTextureEnd();

			if(xs<=1 || ys<=1)
				break;
	    
			s = data;
			d = data;
			for(y=0;y<ys/2;y++)
			{
				s16 = (sU16 *) s;
				d16 = (sU16 *) d;
				s+=oxs*4;
				d+=oxs*2;
				for(x=0;x<xs/2;x++)
				{
					d16[0] = (s16[0]+s16[4]+s16[oxs*4+0]+s16[oxs*4+4]+2)>>2;
					d16[1] = (s16[1]+s16[5]+s16[oxs*4+1]+s16[oxs*4+5]+2)>>2;
					d16[2] = (s16[2]+s16[6]+s16[oxs*4+2]+s16[oxs*4+6]+2)>>2;
					d16[3] = (s16[3]+s16[7]+s16[oxs*4+3]+s16[oxs*4+7]+2)>>2;
					d16+=4;
					s16+=8;
				}
			}

			xs=xs>>1;
			ys=ys>>1;
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

void sGeoBuffer::Init()
{
  Type = 0;
  Size = 0;
  Used = 0;
  VB = 0;
  UserCount = 0;
}

sBool sGeoBuffer::AllocVB(sInt count,sInt size,sInt &firstp)
{
  sInt pos;
  sInt first;

  if(Type!=2) 
    return sFALSE;

  pos = Used;
  first = (pos+size-1)/size;
  pos = first*size;
  pos += count*size;

  if(pos>Size)
    return sFALSE;

  Used = pos;
  firstp = first;
  UserCount++;
  return sTRUE;
}

sBool sGeoBuffer::AllocIB(sInt count,sInt &firstp)
{
  if(Type!=1)
    return sFALSE;
  if(Used+count*2>Size)
    return sFALSE;
  firstp = Used/2;
  Used+=count*2;
  UserCount++;
  return sTRUE;
}

void sGeoBuffer::Free()
{
  sVERIFY(UserCount>0);
  UserCount--;
  if(UserCount==0)
    Used = 0;
}

void sTexInfo::Init(sBitmap *bm,sInt format,sU32 flags)
{
  XSize = bm->XSize;
  YSize = bm->YSize;
  Bitmap = bm;
  Flags = flags;
  Format = format;
}

void sTexInfo::InitRT(sInt xs,sInt ys)
{
  XSize = xs;
  YSize = ys;
  Flags = sTIF_RENDERTARGET;
  Format = 0;
}

/*
void sLightEnv::Init()
{
  Diffuse = 0xffffffff;
  Ambient = 0xff000000;
  Specular = 0xffffffff;
  Emissive = 0x00000000;
  Power = 0.0f;
}
*/
void sLightInfo::Init()
{
  Type = sLI_DIR;
  Pos.Init(0,0,0,1);
  Dir.Init(0,0,0,1);
  Range = 16.0f;
  RangeCut = 0.1f;

  Diffuse = 0xffffffff;
  Ambient = 0xff000000;
  Specular = 0xffffffff;
  Amplify = 1.0f;
  
  Gamma = 0.5f;
  Inner = 0.0f;
  Outer = sPIF/2;
}

/****************************************************************************/
/***                                                                      ***/
/***   Camera                                                             ***/
/***                                                                      ***/
/****************************************************************************/

void sCamera::Init()
{
  /*
  View.Init();
  Project.Init();
  Final.Init();
*/
  Matrix.Init();

  CamPos.Init();
  ZoomX = 1.0f;
  ZoomY = 1.0f;
  CenterX = 0.0f;
  CenterY = 0.0f;
  AspectX = 1.0f;
  AspectY = 1.0f;
  NearClip = 0.125f;
  FarClip = 256.0f;
}

/****************************************************************************/

void sCamera::Init(const sViewport &vp)
{
  sInt i;
  Init();
  i = vp.Window.XSize(); if(i!=0) AspectX = i;
  i = vp.Window.YSize(); if(i!=0) AspectY = i;
}

void sCamera::Calc()
{
  sF32 ratio;
  sF32 q;
  sMatrix mat,cam;

  ratio = AspectX/AspectY;
  q = FarClip/(FarClip-NearClip);
  mat.i.Init(ZoomX    ,0          , 0         ,0);
  mat.j.Init(0        ,ZoomY*ratio, 0         ,0);
  mat.k.Init(CenterX  ,CenterY    , q         ,1);
  mat.l.Init(0        ,0          ,-q*NearClip,0);

  cam = CamPos;
  cam.TransR();
  Matrix.Mul4(cam,mat);
}

void sCamera::CalcTS()
{
  sF32 ratio;
  sF32 q;
  sMatrix mat,cam;

  ratio = AspectX/AspectY;
  q = FarClip/(FarClip-NearClip);
  mat.i.Init(ZoomX*0.5    ,0          , 0         ,0);
  mat.j.Init(0        ,-ZoomY*ratio*0.5, 0         ,0);
  mat.k.Init(CenterX+0.5  ,CenterY+0.5, q         ,1);
  mat.l.Init(0        ,0          ,-q*NearClip,0);

  cam = CamPos;
  cam.TransR();
  Matrix.Mul4(cam,mat);
}


/****************************************************************************/
/*

void sCamera::SetMatrix(const sMatrix &)
{
}

void sCamera::SetMatrix2d()
{
}
*/
/****************************************************************************/
/****************************************************************************/
/*
void sCamera::Transform3d(const sVector *in,sVector *out,sInt count)
{
}

sInt sCamera::Transform2d(const sVector *in,sVector *out,sInt count)
{
  return 0;
}

void sCamera::TransformScreen(const sVector *in,sVector *out,sInt count)
{
}
*/
/****************************************************************************/
/***                                                                      ***/
/***   Viewport                                                           ***/
/***                                                                      ***/
/****************************************************************************/

void sViewport::Init(sInt screen)
{
  Screen = screen;
  RenderTarget = -1;
  Window.Init(0,0,sSystem->Screen[screen].XSize,sSystem->Screen[screen].YSize);
  ClearFlags = sVCF_ALL;
  ClearColor.Color = 0xff000000;
}

/****************************************************************************/

void sViewport::InitTex(sInt handle)
{
  Screen = -1;
  RenderTarget = handle;
  ClearFlags = sVCF_ALL;
  ClearColor.Color = 0xff000000;
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Host Font                                                          ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

struct sHostFontPrivate
{
//  HDC screendc;
  BITMAPINFO bmi;
//  HDC hdc;
  HBITMAP hbm;
  LOGFONT lf;
  HFONT font;
};

/****************************************************************************/

sBool InitGDI()
{
  GDIScreenDC = GetDC(0);
  GDIDC = CreateCompatibleDC(GDIScreenDC);
  GDIHBM = CreateCompatibleBitmap(GDIScreenDC,16,16);
  SelectObject(GDIDC,GDIHBM);

  return sTRUE;
}

/****************************************************************************/

void ExitGDI()
{
  DeleteDC(GDIDC);
  DeleteObject(GDIHBM);
  ReleaseDC(0,GDIScreenDC);
}
/****************************************************************************/

sHostFont::sHostFont()
{
  prv = new sHostFontPrivate;
  prv->font = 0;
}

/****************************************************************************/

sHostFont::~sHostFont()
{
  if(prv->font)
    DeleteObject(prv->font);

  delete[] prv;
}

/****************************************************************************/
/****************************************************************************/

void sHostFont::Begin(sBitmap *bm)
{
  prv->hbm = CreateCompatibleBitmap(GDIScreenDC,bm->XSize,bm->YSize);
  SelectObject(GDIDC,prv->hbm);

  prv->bmi.bmiHeader.biSize = sizeof(prv->bmi.bmiHeader);
  prv->bmi.bmiHeader.biWidth = bm->XSize;
  prv->bmi.bmiHeader.biHeight = -bm->YSize;
  prv->bmi.bmiHeader.biPlanes = 1;
  prv->bmi.bmiHeader.biBitCount = 32;
  prv->bmi.bmiHeader.biCompression = BI_RGB;
  SetDIBits(GDIDC,prv->hbm,0,bm->YSize,bm->Data,&prv->bmi,DIB_RGB_COLORS);
}

/****************************************************************************/

void sHostFont::Print(sChar *text,sInt x,sInt y,sU32 c0,sInt len)
{
  sInt i;

  c0 = ((c0&0xff0000)>>16)|(c0&0x00ff00)|((c0&0x0000ff)<<16);
  SetTextColor(GDIDC,c0);

//  if(len==-1)
//    len = 0x7fffffff;
  len &= 0x7fffffff;
  for(i=0;text[i]!=0 && i<len;i++);
  ExtTextOut(GDIDC,x,y,0,0,text,i,0);
}

/****************************************************************************/

void sHostFont::End(sBitmap *bm)
{
  sInt i;
  prv->bmi.bmiHeader.biSize = sizeof(prv->bmi.bmiHeader);
  prv->bmi.bmiHeader.biWidth = bm->XSize;
  prv->bmi.bmiHeader.biHeight = -bm->YSize;
  prv->bmi.bmiHeader.biPlanes = 1;
  prv->bmi.bmiHeader.biBitCount = 32;
  prv->bmi.bmiHeader.biCompression = BI_RGB;
  GetDIBits(GDIDC,prv->hbm,0,bm->YSize,bm->Data,&prv->bmi,DIB_RGB_COLORS);
  SelectObject(GDIDC,GDIHBM);
  DeleteObject(prv->hbm);
  for(i=0;i<bm->XSize*bm->YSize;i++)
    bm->Data[i] |= 0xff000000;
}

/****************************************************************************/
/****************************************************************************/

void sHostFont::Init(sChar *name,sInt height,sInt width,sInt style)
{
  LOGFONT lf;
  TEXTMETRIC met;

  sVERIFY(prv->font==0);

  if(name[0]==0)
    name = "arial";

  lf.lfHeight         = -height; 
  lf.lfWidth          = width; 
  lf.lfEscapement     = 0; 
  lf.lfOrientation    = 0; 
  lf.lfWeight         = (style&2) ? FW_BOLD : FW_NORMAL; 
  lf.lfItalic         = (style&1) ? 1 : 0; 
  lf.lfUnderline      = 0; 
  lf.lfStrikeOut      = 0; 
  lf.lfCharSet        = DEFAULT_CHARSET; 
  lf.lfOutPrecision   = OUT_TT_PRECIS; 
  lf.lfClipPrecision  = CLIP_DEFAULT_PRECIS; 
  lf.lfQuality        = (style&4) ? ANTIALIASED_QUALITY : NONANTIALIASED_QUALITY;
  lf.lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;
  sCopyMem(lf.lfFaceName,name,32);

  prv->font = CreateFontIndirect(&lf);
  SetBkMode(GDIDC,TRANSPARENT);
  SelectObject(GDIDC,prv->font);
  SelectObject(GDIDC,GDIHBM);

  GetTextMetrics(GDIDC,&met);

  Height = met.tmHeight;
  Advance = met.tmHeight+met.tmExternalLeading;
  Baseline = met.tmHeight-met.tmDescent;
}

/****************************************************************************/

void sHostFont::Print(sBitmap *bm,sChar *text,sInt x,sInt y,sU32 color,sInt len)
{
  Begin(bm);
  Print(text,x,y,color,len);
  End(bm);
}

/****************************************************************************/

sBool sHostFont::Prepare(sBitmap *bm,sChar *letters,sHostFontLetter *met)
{
  sInt letter;
  sInt border;
  sInt x,y,xs,ys,h,w,a;
  SIZE size;
  sChar buffer[2];
  sBool result;
  ABC abc;

  Begin(bm);

  h = Height;
  border = 2;
  x = border;
  y = border;
  xs = bm->XSize;
  ys = bm->YSize;
  sSetMem(met,0,sizeof(sHostFontLetter)*256);

  result = sTRUE;
  while(*letters)
  {
    letter = (*letters++)&0xff;

    buffer[0]=letter;
    buffer[1]=0;
    if(GetCharABCWidths(GDIDC,letter,letter,&abc))
    {
      w = abc.abcB;
      a = 0;
      if(abc.abcA>0)
        w += abc.abcA;
      else
        a = -abc.abcA;
      if(abc.abcC>0)
        w += abc.abcC;
    }
    else
    {
      GetTextExtentPoint32(GDIDC,buffer,1,&size);
      w = size.cx;
      a = 0;
    }
    if(x+w>xs-border)
    {
      y+=h+border;
      x=border;
      if(y+h>ys-border)
      {
        result = sFALSE;
        break;
      }
    }

    Print(buffer,x+a,y,0xffffffff);

    met[letter].Location.x0 = x;
    met[letter].Location.y0 = y;
    met[letter].Location.x1 = x+w;
    met[letter].Location.y1 = y+h;
    met[letter].Pre = 0;
    met[letter].Post = 0;
    met[letter].Top = 0;

    x = x + w + border;
  }
  End(bm);

  return result;
}

/****************************************************************************/

sBool sHostFont::PrepareCheck(sInt xs,sInt ys,sChar *letters)
{
  sInt letter;
  sInt border;
  sInt x,y,h,w;
  SIZE size;
  sChar buffer[2];
  sBool result;


  h = Height;
  border = 2;
  x = border;
  y = border;

  result = sTRUE;
  while(*letters)
  {
    letter = (*letters++)&0xff;

    buffer[0]=letter;
    buffer[1]=0;
    GetTextExtentPoint32(GDIDC,buffer,1,&size);
    w = size.cx;
    if(x+w>xs-border)
    {
      y+=h+border;
      x=border;
      if(y+h>ys-border)
      {
        result = sFALSE;
        break;
      }
    }

    x = x + w + border;
  }

//  sDPrintF("letter %d\n",letter);
  return result;
}

/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/

#endif

// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#include "_start.hpp"
#if sLINK_ENGINE
#include "_startdx.hpp"
#endif
#if sLINK_UTIL
#include "_util.hpp"                  // for sPerfMon->Flip()
#endif

#define WINVER 0x500
#define _WIN32_WINNT 0x0500
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
#define DXERROR(hr) {if(FAILED(hr))sFatal("%s(%d) : directx error %08x (%d)",__FILE__,__LINE__,hr,hr&0x3fff);}

#undef DeleteFile
#undef GetCurrentDirectory
#undef LoadBitmap
#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"opengl32.lib")
#pragma comment(lib,"dinput.lib")
#if !sINTRO
#pragma comment(lib,"dxguid.lib")
#endif

#define LOGGING 0                 // log all create and release calls
#define WINZERO(x) {sSetMem(&x,0,sizeof(x));}
#define WINSET(x) {sSetMem(&x,0,sizeof(x));x.dwSize = sizeof(x);}
#define RELEASE(x) {if(x)x->Release();x=0;}
#define DXERROR(hr) {if(FAILED(hr))sFatal("%s(%d) : directx error %08x (%d)",__FILE__,__LINE__,hr,hr&0x3fff);}
sMAKEZONE(FlipLock,"FlipLock",0xffff0000);

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   System Initialisation                                              ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

//#if !sINTRO
class sBroker_ *sBroker;
//#endif

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
sChar *WCmdLine;
HWND MsgWin;

sSystem_ *sSystem;

static sU64 sPerfFrame;
static sF64 sPerfKalibFactor = 1.0f/1000;
static sU64 sPerfKalibRDTSC;
static sU64 sPerfKalibQuery;

/****************************************************************************/


/****************************************************************************/

HDC GDIScreenDC;
HDC GDIDC;
HBITMAP GDIHBM;
BITMAPINFO FontBMI;
HBITMAP FontHBM;
HFONT FontHandle;

sBool InitGDI();
void ExitGDI();

/****************************************************************************/
/*
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
*/
sInt sFatality;

/****************************************************************************/
/***                                                                      ***/
/***   FVF Codes                                                          ***/
/***                                                                      ***/
/****************************************************************************/

struct FVFTableStruct
{
  sInt Size;                      // size in bytes!
  D3DVERTEXELEMENT9 *Info;        // dx declaration
  IDirect3DVertexDeclaration9 *Decl;    // declaration object
};

static D3DVERTEXELEMENT9 DeclDefault[] = 
{
  { 0, 0,D3DDECLTYPE_FLOAT3    ,0,D3DDECLUSAGE_POSITION  ,0 },
  { 0,12,D3DDECLTYPE_FLOAT3    ,0,D3DDECLUSAGE_NORMAL    ,0 },
  { 0,24,D3DDECLTYPE_FLOAT2    ,0,D3DDECLUSAGE_TEXCOORD  ,0 },
  D3DDECL_END()
};
static D3DVERTEXELEMENT9 DeclDouble[] = 
{
  { 0, 0,D3DDECLTYPE_FLOAT3    ,0,D3DDECLUSAGE_POSITION  ,0 },
  { 0,12,D3DDECLTYPE_D3DCOLOR  ,0,D3DDECLUSAGE_COLOR     ,0 },
  { 0,16,D3DDECLTYPE_FLOAT2    ,0,D3DDECLUSAGE_TEXCOORD  ,0 },
  { 0,24,D3DDECLTYPE_FLOAT2    ,0,D3DDECLUSAGE_TEXCOORD  ,1 },
  D3DDECL_END()
};
static D3DVERTEXELEMENT9 DeclTSpace[] = 
{
  { 0, 0,D3DDECLTYPE_FLOAT3    ,0,D3DDECLUSAGE_POSITION  ,0 },
  { 0,12,D3DDECLTYPE_FLOAT3    ,0,D3DDECLUSAGE_NORMAL    ,0 },
  { 0,24,D3DDECLTYPE_D3DCOLOR  ,0,D3DDECLUSAGE_COLOR     ,0 },
  { 0,28,D3DDECLTYPE_D3DCOLOR  ,0,D3DDECLUSAGE_COLOR     ,1 },
  { 0,32,D3DDECLTYPE_FLOAT2    ,0,D3DDECLUSAGE_TEXCOORD  ,0 },
  { 0,40,D3DDECLTYPE_FLOAT3    ,0,D3DDECLUSAGE_TANGENT   ,0 },
  { 0,52,D3DDECLTYPE_FLOAT3    ,0,D3DDECLUSAGE_BINORMAL  ,0 },
  D3DDECL_END()
};
static D3DVERTEXELEMENT9 DeclCompact[] = 
{
  { 0, 0,D3DDECLTYPE_FLOAT3    ,0,D3DDECLUSAGE_POSITION  ,0 },
  { 0,12,D3DDECLTYPE_D3DCOLOR  ,0,D3DDECLUSAGE_COLOR     ,0 },
  D3DDECL_END()
};
static D3DVERTEXELEMENT9 DeclTSpace3[] = 
{
  { 0, 0,D3DDECLTYPE_FLOAT3    ,0,D3DDECLUSAGE_POSITION  ,0 },
  { 0,12,D3DDECLTYPE_SHORT4    ,0,D3DDECLUSAGE_NORMAL    ,0 },
  { 0,20,D3DDECLTYPE_SHORT2    ,0,D3DDECLUSAGE_TANGENT   ,0 },
  { 0,24,D3DDECLTYPE_FLOAT2    ,0,D3DDECLUSAGE_TEXCOORD  ,0 },
  D3DDECL_END()
};

static D3DVERTEXELEMENT9 DeclXYZW[] = 
{
  { 0, 0,D3DDECLTYPE_FLOAT4    ,0,D3DDECLUSAGE_POSITION  ,0 },
  D3DDECL_END()
};

static FVFTableStruct FVFTable[] = 
{
  {  0,0           },
  { 32,DeclDefault },
  { 32,DeclDouble  },
  { 64,DeclTSpace  },
  { 16,DeclCompact },
  { 16,DeclXYZW    },
  { 32,DeclTSpace3 },
};

#define FVFMax (sizeof(FVFTable)/sizeof(FVFTableStruct))

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
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Keyboard table for non-DI                                          ***/
/***                                                                      ***/
/****************************************************************************/

sInt VKTable[256] =
{
  0,0,0,0,0,0,0,0,                          // 0x00
  0,0,0,0,0,sKEY_ENTER,0,0,
  0,0,0,0,0,0,0,0,                          // 0x10
  0,0,0,sKEY_ESCAPE,0,0,0,0,
  sKEY_SPACE,0,0,0,0,sKEY_LEFT,sKEY_UP,sKEY_RIGHT,  // 0x20
  sKEY_DOWN,0,0,0,0,0,0,0,
  1,1,1,1,1,1,1,1,                          // 0x30
  1,1,0,0,0,0,0,0,
  1,1,1,1,1,1,1,1,                          // 0x40
  1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,                          // 0x50
  1,1,1,0,0,0,0,0,
  0,0,0,0,0,0,0,0,                          // 0x60
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,                          // 0x70
  0,0,0,0,0,0,0,0,

  0,0,0,0,0,0,0,0,                          // 0x80
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,                          // 0x90
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,                          // 0xa0
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,                          // 0xb0
  0,0,0,0,0,0,0,0,

  0,0,0,0,0,0,0,0,                          // 0xc0
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,                          // 0xd0
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,                          // 0xe0
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,                          // 0xf0
  0,0,0,0,0,0,0,0,
};

/****************************************************************************/
/***                                                                      ***/
/***   Heap Memory Management                                             ***/
/***                                                                      ***/
/****************************************************************************/

sInt MemoryUsedCount;

#if !sINTRO
#undef new

void * __cdecl operator new(unsigned int size)
{
  void *p;
#if !sRELEASE
  p = _malloc_dbg(size,_NORMAL_BLOCK,"unknown",1);
#else
  p = malloc(size);
#endif
  if(p==0) sFatal("ran out of virtual memory...");
//  sSetMem(p,0x12,size);
//  sDPrintF("%10d : (%d)\n",MemoryUsedCount,size);
  MemoryUsedCount+=_msize(p); 
  return p;
}

void * __cdecl operator new(unsigned int size,const char *file,int line)
{
  void *p;
#if !sRELEASE
  p = _malloc_dbg(size,_NORMAL_BLOCK,file,line);
#else
  p = malloc(size);
#endif
  if(p==0) sFatal("ran out of virtual memory...");
//  sSetMem(p,0x12,size);
  MemoryUsedCount+=_msize(p); 
//  sDPrintF("%10d : %d\n",MemoryUsedCount,size);
//  if(size>1024*1024)
//    sDPrintF("geht garnicht!\n");
  return p;
}

void __cdecl operator delete(void *p)
{
	if(p)
	{
		MemoryUsedCount-=_msize(p); 
#if !sRELEASE
		_free_dbg(p,_NORMAL_BLOCK);
#else
    free(p);
#endif
	}
}

#define new new(__FILE__,__LINE__)
#endif

#if sINTRO

void * __cdecl operator new(unsigned int size)
{
	return HeapAlloc(GetProcessHeap(),HEAP_NO_SERIALIZE,size);
}

void * __cdecl operator new[](unsigned int size)
{
	return HeapAlloc(GetProcessHeap(),HEAP_NO_SERIALIZE,size);
}

void __cdecl operator delete(void *ptr)
{
	if(ptr)
  	HeapFree(GetProcessHeap(),HEAP_NO_SERIALIZE,ptr);
}

void __cdecl operator delete[](void *ptr)
{
	if(ptr)
  	HeapFree(GetProcessHeap(),HEAP_NO_SERIALIZE,ptr);
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
/***                                                                      ***/
/***   Startup, Commandline                                               ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#if sCOMMANDLINE

#include <stdio.h>

void sPrint(sChar *text)
{
  printf("%s",text);
  sDPrint(text);
}

void __cdecl sPrintF(sChar *format,...)
{
  static sChar buffer[1024];

  sFormatString(buffer,sizeof(buffer),format,&format);
  sPrint(buffer);
}

int main(int argc,char **argv)
{
  sSystem = new sSystem_;
  sSetMem(((sU8 *)sSystem)+4,0,sizeof(sSystem_)-4);
  sSystem->InitX();
  sAppHandler(sAPPCODE_INIT,0);
  sAppHandler(sAPPCODE_EXIT,0);
}

void sSystem_::InitX()
{
  timeBeginPeriod(1);
  WStartTime = timeGetTime();
  sSetRndSeed(timeGetTime()&0x7fffffff);
  sBroker = new sBroker_;

  sAppHandler(sAPPCODE_FRAME,0);

  sBroker->Free();
  sBroker->Dump();
  delete sBroker;
  sBroker = 0;

#if !sRELEASE
  getchar();
#endif
}

/*
void sSystem_::Exit()
{
}

void sSystem_::Tag()
{
}

sNORETURN void sSystem_::Abort(sChar *msg)
{
  WAborting = sTRUE;
  _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)&~(_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF));
  if(msg)
    MessageBox(0,msg,"Fatal Error",MB_OK|MB_TASKMODAL|MB_SETFOREGROUND|MB_TOPMOST|MB_ICONERROR);
  ExitProcess(0);
}

void sSystem_::Log(sChar *s)
{
  if(!sFatality && !sAppHandler(sAPPCODE_DEBUGPRINT,(sU32) s))
    OutputDebugString(s);
}
*/
#endif

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Startup, without intro                                             ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#if !sCOMMANDLINE
#if !sINTRO

static void MakeCpuMask()
{
  sU32 result;
  sU32 vendor[4];

// start with nothing supportet

  sSystem->CpuMask = 0;

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
  if(result&(1<< 4)) sSystem->CpuMask |= sCPU_RDTSC;
  if(result&(1<<15)) sSystem->CpuMask |= sCPU_CMOVE;  
  if(result&(1<<23)) sSystem->CpuMask |= sCPU_MMX;
  if(result&(1<<25)) sSystem->CpuMask |= sCPU_SSE|sCPU_MMX2;
  if(result&(1<<26)) sSystem->CpuMask |= sCPU_SSE2;

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
    if(result&(1<<31)) sSystem->CpuMask |= sCPU_3DNOW;

// check AMD specific features

    if(sCmpMem(vendor,"AuthenticAMD",12)==0)
    {
      if(result&(1<<22)) sSystem->CpuMask |= sCPU_MMX2;  
      if(result&(1<<30)) sSystem->CpuMask |= sCPU_3DNOW2;  
    }
  }

// is the SSE support complete and supported by the operating system?
  
  if(sSystem->CpuMask & sCPU_SSE)
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
      sSystem->CpuMask &= ~(sCPU_SSE|sCPU_SSE2);
    }
  }
}

/****************************************************************************/

static LRESULT WINAPI MainWndProc(HWND win,UINT msg,WPARAM wparam,LPARAM lparam)
{
  MsgWin = win;
  return sSystem->Msg(msg,wparam,lparam);
}

/****************************************************************************/

int APIENTRY WinMain(HINSTANCE inst,HINSTANCE prev,LPSTR cmdline,int show)
{
  WInst = inst;
  WCmdLine = cmdline;

  if(!sAppHandler(sAPPCODE_CONFIG,0))
    sSetConfig(sSF_DIRECT3D);
  sSystem->InitX();

  delete sSystem;

  sDPrintF("Memory Left: %d bytes\n",MemoryUsedCount);
  return 0;
}

/****************************************************************************/

void sSetConfig(sU32 flags,sInt xs,sInt ys)
{
  sSystem = new sSystem_;
  
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

  sDPrintF("found monitor <%s> at %d %d %d %d\n","xx",rect->left,rect->top,rect->right,rect->bottom);
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

void sSystem_::InitX()
{
  WNDCLASS wc;
  MSG msg;
  sBool gotmsg;
  sInt i;

// set up memory checking
  MakeCpuMask();

  _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)|_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF);
  sChar *test = new sChar[16];
  sCopyString(test,"TestLeak",16);

// find dlls

  d3dlib = ::LoadLibraryA("d3d9.dll");
  if(d3dlib==0)
    sFatal("you need directx 9 (or better)\nto run this program.\ntry downloading it at\nwww.microsoft.com");
  Direct3DCreate9P = (Direct3DCreate9T) GetProcAddress(d3dlib,"Direct3DCreate9");
  sVERIFY(Direct3DCreate9P);

  dslib = ::LoadLibraryA("dsound.dll");
  DirectSoundCreate8P = (DirectSoundCreate8T) GetProcAddress(dslib,"DirectSoundCreate8");
  sVERIFY(DirectSoundCreate8P);

#if sUSE_DIRECTINPUT
  dilib = ::LoadLibraryA("dinput8.dll");
  DirectInput8CreateP = (DirectInput8CreateT) GetProcAddress(dilib,"DirectInput8Create");
  sVERIFY(DirectInput8CreateP);
#endif

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
  wc.style            = CS_CLASSDC;///*CS_OWNDC|*/CS_VREDRAW|CS_HREDRAW;
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
  WConstantUpdate = sTRUE;
  WAbortKeyFlag = 0;
  WFocus = 1;

  CmdLowQuality = 0;
  CmdLowRes = 0;
  CmdShaderLevel = sPS_20;

  CmdWindowed = 0;
  CmdFullscreen = 0;
  sChar *cmdline = WCmdLine;
  while(*cmdline)
  {
    while(*cmdline==' ')
      cmdline++;
    if(sCmpMem(cmdline,"low",3)==0)
    {
      CmdLowQuality = 1;
      cmdline+=3;
    }
    else if(sCmpMem(cmdline,"lowres",6)==0)
    {
      CmdLowRes = 1;
      cmdline+=6;
    }
    else if(sCmpMem(cmdline,"full",4)==0)
    {
      ConfigFlags |= sSF_FULLSCREEN;
      if(ConfigX==0) ConfigX = 1024;
      if(ConfigY==0) ConfigY = 768;
      cmdline+=4;
    }
    else if(sCmpMem(cmdline,"win",4)==0)
    {
      ConfigFlags &= ~sSF_FULLSCREEN;
      cmdline+=3;
    }
    else if(sCmpMem(cmdline,"ps00",4)==0)
    {
      CmdShaderLevel = sPS_00;
      cmdline+=4;
    }
    else if(sCmpMem(cmdline,"ps13",4)==0)
    {
      CmdShaderLevel = sPS_13;
      cmdline+=4;
    }
    else if(sCmpMem(cmdline,"ps14",4)==0)
    {
      CmdShaderLevel = sPS_14;
      cmdline+=4;
    }
    else if(sCmpMem(cmdline,"ps20",4)==0)
    {
      CmdShaderLevel = sPS_20;
      cmdline+=4;
    }
    else
    {
      break;
    }
  }

  EnumDisplayMonitors(0,0,MonitorEnumProc,0);
  if(WScreenCount<2)
    ConfigFlags &= ~sSF_MULTISCREEN;
  if(ConfigFlags & sSF_MULTISCREEN)
    ConfigFlags |= sSF_FULLSCREEN;
  sVERIFY(WScreenCount>0);
  sSetMem(GeoBuffer,0,sizeof(GeoBuffer));
  sSetMem(GeoHandle,0,sizeof(GeoHandle));

  sSetMem(&PerfThis,0,sizeof(PerfThis));
  sSetMem(&PerfLast,0,sizeof(PerfLast));
  PerfThis.Time = sSystem->PerfTime();

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

  _control87(_PC_24|_RC_NEAR,MCW_PC|MCW_RC);
  
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
      if(WActiveCount==0 && WFocus)
      {
#if sUSE_DIRECTINPUT
        PollDI();
#endif
        for(i=0;i<KeyIndex;i++)
          sAppHandler(sAPPCODE_KEY,KeyBuffer[i]);
        KeyIndex = 0;

        Render();

        PerfLast.TimeFiltered = (PerfLast.TimeFiltered*7+PerfLast.Time)/8;
        PerfLast.Line     = PerfThis.Line;
        PerfLast.Triangle = PerfThis.Triangle;
        PerfLast.Vertex   = PerfThis.Vertex;
        PerfLast.Material = PerfThis.Material;
        PerfThis.Line     = 0;
        PerfThis.Triangle = 0;
        PerfThis.Vertex   = 0;
        PerfThis.Material = 0;
#if sLINK_UTIL
        sPerfMon->Flip();
#elif !sINTRO
        sSystem->PerfKalib();
#endif
        Sleep(0);
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

#if sLINK_UTIL
  sBroker->RemRoot(sPerfMon);
#endif

#if sUSE_DIRECTSOUND
  ExitDS();
#endif
  sBroker->Free();      // some objects may still hold resources, like Geometries
  sBroker->Dump();
  ExitScreens();

  for(i=0;i<MAX_TEXTURE;i++)
    Textures[i].Flags=0;

#if sUSE_DIRECTINPUT
  ExitDI();
#endif
  ExitGDI();

  sBroker->Free();
  sBroker->Dump();
  delete sBroker;
  sBroker = 0;

  FreeLibrary(d3dlib);
  FreeLibrary(dilib);
  FreeLibrary(dslib);
}

/****************************************************************************/

sInt sSystem_::Msg(sU32 msg,sU32 wparam,sU32 lparam)
{
  PAINTSTRUCT ps; 
  HDC hdc; 
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
    hdc = BeginPaint(win,&ps); 
    EndPaint(win,&ps); 
    if(/*WActiveCount!=0 &&*/ !WFullscreen)
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
  case WM_SETFOCUS:
    WFocus = 1;
    break;
  case WM_KILLFOCUS:
    WFocus = 0;
    break;
  case WM_CLOSE:
    if(KeyIndex<MAX_KEYBUFFER)
      KeyBuffer[KeyIndex++] = sKEY_CLOSE;
    else
      KeyBuffer[KeyIndex-1] = sKEY_CLOSE;
    return 0;

  case WM_LBUTTONDOWN:
    MouseButtons |= 1;
    MouseButtonsSave |= 1;
    KeyQual |= sKEYQ_MOUSEL;
    if(KeyIndex < MAX_KEYBUFFER)
      KeyBuffer[KeyIndex++] = (sKEY_MOUSEL)|KeyQual;
    SetCapture(win);
    break;
  case WM_LBUTTONUP:
    MouseButtons &= ~1;
    KeyQual &= ~sKEYQ_MOUSEL;
    if(KeyIndex < MAX_KEYBUFFER)
      KeyBuffer[KeyIndex++] = (sKEY_MOUSEL)|sKEYQ_BREAK|KeyQual;
    ReleaseCapture();
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
  case WM_MOUSEWHEEL:
    i = (sS16)(wparam>>16);
    while(i>=WHEEL_DELTA && KeyIndex < MAX_KEYBUFFER)
    {
      KeyBuffer[KeyIndex++] = sKEY_WHEELUP;
      i -= WHEEL_DELTA;
    }
    while(i<=-WHEEL_DELTA && KeyIndex < MAX_KEYBUFFER)
    {
      KeyBuffer[KeyIndex++] = sKEY_WHEELDOWN;
      i += WHEEL_DELTA;
    }
    break;

#if !sUSE_DIRECTINPUT
  case WM_KEYDOWN:
    if(KeyIndex < MAX_KEYBUFFER-1 && wparam<256)
    {
      i = VKTable[wparam];
      if(i==1) i=wparam;
      KeyBuffer[KeyIndex++] = i;
    }
    break;
  case WM_KEYUP:
    if(KeyIndex < MAX_KEYBUFFER-1 && wparam<256)
    {
      i = VKTable[wparam];
      if(i==1) i=wparam;
      KeyBuffer[KeyIndex++] = i|sKEYQ_BREAK;
    }
    break;
#endif
  }

  return DefWindowProc(win,msg,wparam,lparam);
}

void sSystem_::InitScreens()
{
  HRESULT hr;
  D3DPRESENT_PARAMETERS d3dpp[8];
  RECT r;
  sInt nr,i;
  sU32 create;
  sU16 *iblock;
  sHardTex *tex;
  D3DCAPS9 caps;

  sVERIFY(WScreenCount<8);

  sREGZONE(FlipLock);

  if(!DXD)
  {
    DXD = (*Direct3DCreate9P)(D3D_SDK_VERSION);
    sVERIFY(DXD);
  }

	ZBufFormat=D3DFMT_D24S8;

  for(nr=0;nr<WScreenCount;nr++)
  {
// determine n

    Screen[nr].SFormat=D3DFMT_A8R8G8B8;
    Screen[nr].ZFormat=ZBufFormat;

    WINZERO(d3dpp[nr]);
    d3dpp[nr].BackBufferFormat = (enum _D3DFORMAT) Screen[nr].SFormat;
    d3dpp[nr].EnableAutoDepthStencil = sFALSE;
    d3dpp[nr].SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp[nr].BackBufferCount = 1;
    d3dpp[nr].PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    d3dpp[nr].Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;// | D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
    d3dpp[nr].hDeviceWindow = (HWND) Screen[nr].Window;
    if(ConfigFlags & sSF_FULLSCREEN)
    {
      d3dpp[nr].BackBufferWidth = ConfigX;
      d3dpp[nr].BackBufferHeight = ConfigY;
      Screen[nr].XSize = ConfigX;
      Screen[nr].YSize = ConfigY;
      WFullscreen = sTRUE;
      SetWindowLong((HWND)Screen[nr].Window,GWL_STYLE,WS_POPUP|WS_VISIBLE);
    }
    else
    {
      d3dpp[nr].Windowed = TRUE;
      GetClientRect((HWND) Screen[nr].Window,&r);
      Screen[nr].XSize = r.right-r.left;
      Screen[nr].YSize = r.bottom-r.top;
      WFullscreen = sFALSE;
      SetWindowLong((HWND)Screen[nr].Window,GWL_STYLE,WWindowedStyle);
    }
  }

  hr=DXD->GetDeviceCaps(0,D3DDEVTYPE_HAL,&caps);
  sVERIFY(!FAILED(hr));

  if(!DXDev)
  {
#if LOGGING
    sDPrintF("Create Device\n");
#endif

    if((caps.PixelShaderVersion&0xffff)==0x0000)
      goto dxdevfailed;

    create = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    if(ConfigFlags & sSF_MULTISCREEN) create |= D3DCREATE_ADAPTERGROUP_DEVICE;

    hr = DXD->CreateDevice(0,D3DDEVTYPE_HAL,(HWND) Screen[0].Window,create,d3dpp,&DXDev);
    if(FAILED(hr))
    {
dxdevfailed:;
      create = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
      if(ConfigFlags & sSF_MULTISCREEN) create |= D3DCREATE_ADAPTERGROUP_DEVICE;
      hr = DXD->CreateDevice(0,D3DDEVTYPE_HAL,(HWND) Screen[0].Window,create,d3dpp,&DXDev);
      if(FAILED(hr))
        sFatal("could not create screen");
    }
    for(i=1;i<FVFMax;i++)
      DXDev->CreateVertexDeclaration(FVFTable[i].Info,&FVFTable[i].Decl);
  }
  else 
  {
    if(DXNormalCube)
    {
      DXNormalCube->Release();
      DXNormalCube = 0;
    }
		if(ZBuffer)
		{
#if LOGGING
      sDPrintF("Release ZBuffer\n");
#endif
			ZBuffer->Release();
			ZBuffer = 0;
		}
    for(i=0;i<MAX_GEOHANDLE;i++)
    {
      if(GeoHandle[i].Mode!=0)
      {
        if(GeoHandle[i].VertexBuffer>=3)
          GeoBuffer[GeoHandle[i].VertexBuffer].Free();
        if(GeoHandle[i].IndexBuffer>=3)
          GeoBuffer[GeoHandle[i].IndexBuffer].Free();
      }
      GeoHandle[i].VertexCount = 0;
      GeoHandle[i].VertexBuffer = 0;
      GeoHandle[i].IndexCount = 0;
      GeoHandle[i].IndexBuffer = 0;
    }
    for(i=0;i<MAX_GEOBUFFER;i++)
    {
      if(i>=3)
        sVERIFY(GeoBuffer[i].UserCount==0);
      if(GeoBuffer[i].VB)
      {
        GeoBuffer[i].VB->Release();   // VB & IB are the same
#if LOGGING
        sDPrintF("Release vertex/indexbuffer %d bytes\n",GeoBuffer[i].Size);
#endif
      }
      GeoBuffer[i].Init();
    }
		for(i=0;i<MAX_TEXTURE;i++)
		{
			if(Textures[i].Flags & sTIF_RENDERTARGET)
			{
#if LOGGING
        sDPrintF("Release Rendertarget\n");
#endif
				Textures[i].Tex->Release();
				Textures[i].Tex=0;
			}
		}

    hr = DXDev->Reset(d3dpp);

    if(FAILED(hr))
    {
      WDeviceLost = 1;
      return;
    }
  }

// initialize gpumask
  if(caps.StencilCaps & D3DSTENCILCAPS_TWOSIDED) GpuMask |= sGPU_TWOSIDESTENCIL;

// init buffer management

  CreateGeoBuffer(0,1,1,MAX_DYNVBSIZE);
  CreateGeoBuffer(1,1,0,MAX_DYNIBSIZE);
  CreateGeoBuffer(2,0,0,2*0x8000/4*6);
  DXERROR(GeoBuffer[2].IB->Lock(0,2*0x8000/4*6,(void **) &iblock,0));
  for(i=0;i<0x8000/6;i++)
    sQuad(iblock,i*4+0,i*4+1,i*4+2,i*4+3);
  DXERROR(GeoBuffer[2].IB->Unlock());
  if((caps.PixelShaderVersion&0xffff)!=0x0000)
  {
    MakeCubeNormalizer();
//    MakeSpecularLookupTex();
  }
  else
  {
//    SpecularLookupTex = sINVALID;
//    NullBumpTex = sINVALID;
  }

// set some defaults

  DXDev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
  DXDev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
  DXDev->SetRenderState(D3DRS_LIGHTING,0);

  WDeviceLost = 0;
  if(WFullscreen)
  {
    GetWindowRect((HWND)Screen[0].Window,&r);
    ClipCursor(&r);
    ShowCursor(0);
  }
  else
  {
    ClipCursor(0);
    ShowCursor(1);
  }

	ReCreateZBuffer();
	for(i=0;i<MAX_TEXTURE;i++)
	{
		if(Textures[i].Flags & sTIF_RENDERTARGET)
    {
      tex = &Textures[i];
	    DXERROR(DXDev->CreateTexture(tex->XSize,tex->YSize,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&tex->Tex,0));
    }
	}
}

/****************************************************************************/

void sSystem_::ReCreateZBuffer()
{
	sInt i,xs,ys;
	
	// determine required size
	xs=ys=0;
	for(i=0;i<WScreenCount;i++)
	{
		xs=sMax<sInt>(xs,Screen[i].XSize);
		ys=sMax<sInt>(ys,Screen[i].YSize);
	}

	for(i=0;i<MAX_TEXTURE;i++)
	{
		if(Textures[i].Flags & sTIF_RENDERTARGET)
		{
			xs=sMax<sInt>(xs,Textures[i].XSize);
			ys=sMax<sInt>(ys,Textures[i].YSize);
		}
	}

	// create the zbuffer
	if(xs!=ZBufXSize || ys!=ZBufYSize || !ZBuffer)
	{
		DXERROR(DXDev->SetDepthStencilSurface(0));

		if(ZBuffer)
		{
			ZBuffer->Release();
			ZBuffer=0;
		}

    #if LOGGING
    sDPrintF("Create ZBuffer %dx%d\n",xs,ys);
    #endif
		DXERROR(DXDev->CreateDepthStencilSurface(xs,ys,(D3DFORMAT) ZBufFormat,D3DMULTISAMPLE_NONE,0,FALSE,&ZBuffer,0));
		ZBufXSize=xs;
		ZBufYSize=ys;

		DXERROR(DXDev->SetDepthStencilSurface(ZBuffer));
	}
}

/****************************************************************************/

void sSystem_::ExitScreens()
{
  sInt i;

  for(i=0;i<MAX_TEXTURE;i++)
  {
    if(Textures[i].Tex)
    {
      Textures[i].Tex->Release();
      Textures[i].Tex=0;
    }
  }
  if(DXNormalCube)
  {
    DXNormalCube->Release();
    DXNormalCube = 0;
  }
//  if(SpecularLookupTex != sINVALID)
//    RemTexture(SpecularLookupTex);
//  if(NullBumpTex != sINVALID)
//    RemTexture(NullBumpTex);
	if(ZBuffer)
	{
		ZBuffer->Release();
		ZBuffer = 0;
	}
  if(DXReadTex)
    DXReadTex->Release();
  for(i=1;i<MAX_GEOHANDLE;i++)
  {
    /*if(GeoHandle[i].Mode != 0)
      sFatal("GeoHandle leak in %s(%d) #%d",GeoHandle[i].File,GeoHandle[i].Line,GeoHandle[i].AllocId);*/
    //sVERIFY(GeoHandle[i].Mode==0);
  }
  for(i=0;i<MAX_GEOBUFFER;i++)
  {
    /*if(i>=3)
      sVERIFY(GeoBuffer[i].UserCount==0);*/
    if(GeoBuffer[i].VB)
      GeoBuffer[i].VB->Release();   // VB & IB are the same
    GeoBuffer[i].Init();
  }
#if sUSE_SHADERS
  for(i=0;i<MAX_SHADERS;i++)
  {
    if(Shaders[i].ShaderData)
    {
      delete[] Shaders[i].ShaderData;
      Shaders[i].ShaderData = 0;
    }
    sVERIFY(Shaders[i].VS==0);
    sVERIFY(Shaders[i].PS==0);
  }
#endif
//  for(i=0;i<MAX_MATERIALS;i++)
//    sVERIFY(Materials[i].RefCount==0);
  for(i=1;i<FVFMax;i++)
    if(FVFTable[i].Decl)
      FVFTable[i].Decl->Release();
  DXDev->Release();
  DXDev = 0;
  DXD->Release();
  DXD = 0;
}
#endif
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Startup, with sINTRO                                               ***/
/***                                                                      ***/
/****************************************************************************/

#if !sCOMMANDLINE
#if sINTRO

/****************************************************************************/

static LRESULT WINAPI MainWndProc(HWND win,UINT msg,WPARAM wparam,LPARAM lparam)
{
  MsgWin = win;
  PAINTSTRUCT ps; 
  HDC hdc; 
  sInt i;

  switch(msg)
  {
  case WM_PAINT:
    hdc = BeginPaint(win,&ps); 
    EndPaint(win,&ps); 
//    if(/*WActiveCount!=0 &&*/ !sSystem->WFullscreen)
//      sSystem->Render();
    break;

  case WM_SETCURSOR:
    SetCursor(0);
    return 0;
  case WM_LBUTTONDOWN:
    sSystem->MouseButtons |= 1;
//    sSystem->MouseButtonsSave |= 1;
//    KeyQual |= sKEYQ_MOUSEL;
    if(sSystem->KeyIndex < MAX_KEYBUFFER)
      sSystem->KeyBuffer[sSystem->KeyIndex++] = sKEY_MOUSEL;
    break;
  case WM_LBUTTONUP:
    sSystem->MouseButtons &= ~1;
//    sSystem->KeyQual &= ~sKEYQ_MOUSEL;
    if(sSystem->KeyIndex < MAX_KEYBUFFER)
      sSystem->KeyBuffer[sSystem->KeyIndex++] = (sKEY_MOUSEL)|sKEYQ_BREAK;
    break;

  case WM_CLOSE:
    sSystem->Exit();
    break;

#if !sUSE_DIRECTINPUT
  case WM_KEYDOWN:
    if(sSystem->KeyIndex < MAX_KEYBUFFER-1 && wparam<256)
    {
      i = VKTable[wparam];
      if(i==1) i=wparam;
      sSystem->KeyBuffer[sSystem->KeyIndex++] = i;
    }
    break;
  case WM_KEYUP:
    if(sSystem->KeyIndex < MAX_KEYBUFFER-1 && wparam<256)
    {
      i = VKTable[wparam];
      if(i==1) i=wparam;
      sSystem->KeyBuffer[sSystem->KeyIndex++] = i|sKEYQ_BREAK;
    }
    break;
#endif
  }

  return DefWindowProc(win,msg,wparam,lparam);
}

/****************************************************************************/

#if !sINTRO || !sRELEASE
int APIENTRY WinMain(HINSTANCE inst,HINSTANCE prev,LPSTR cmdline,int show)
{
  WInst = inst;
  WCmdLine = cmdline;
#else
void WinMainCRTStartup()
{
	WInst = GetModuleHandle(0);
#endif

  sSystem = new sSystem_;
  sSetMem(((sU8 *)sSystem)+4,0,sizeof(sSystem_)-4);

  sSystem->InitX();

  delete sSystem;

	ExitProcess(0);
}

/****************************************************************************/

void sSystem_::InitX()
{
  WNDCLASS wc;
  MSG msg;
  sBool gotmsg;
  sInt i;

// init system

  sSystem->ConfigFlags = sSF_DIRECT3D;
  sSystem->ConfigX = 800;
  sSystem->ConfigY = 600;

// find dlls

  d3dlib = ::LoadLibraryA("d3d9.dll");
  if(d3dlib==0)
    sFatal("you need directx 9 (or better)\nto run this program.\ntry downloading it at\nwww.microsoft.com");
  Direct3DCreate9P = (Direct3DCreate9T) GetProcAddress(d3dlib,"Direct3DCreate9");
  sVERIFY(Direct3DCreate9P);

  dslib = ::LoadLibraryA("dsound.dll");
  DirectSoundCreate8P = (DirectSoundCreate8T) GetProcAddress(dslib,"DirectSoundCreate8");
  sVERIFY(DirectSoundCreate8P);

#if sUSE_DIRECTINPUT
  dilib = ::LoadLibraryA("dinput8.dll");
  DirectInput8CreateP = (DirectInput8CreateT) GetProcAddress(dilib,"DirectInput8Create");
  sVERIFY(DirectInput8CreateP);
#endif

// set up some more stuff

  timeBeginPeriod(1);
  WStartTime = timeGetTime();
  sSetRndSeed(timeGetTime()&0x7fffffff);

  GDIScreenDC = GetDC(0);
  GDIDC = CreateCompatibleDC(GDIScreenDC);
  GDIHBM = CreateCompatibleBitmap(GDIScreenDC,16,16);
  SelectObject(GDIDC,GDIHBM);

//  sBroker = new sBroker_;

// create window class

  wc.lpszClassName    = "kk";
  wc.lpfnWndProc      = MainWndProc;
  wc.style            = CS_CLASSDC;///*CS_OWNDC|*/CS_VREDRAW|CS_HREDRAW;
  wc.hInstance        = WInst;
  wc.hIcon            = LoadIcon(WInst,MAKEINTRESOURCE(101));
  wc.hCursor          = LoadCursor(0,IDC_ARROW);
  wc.hbrBackground    = 0;
  wc.lpszMenuName     = 0;
  wc.cbClsExtra       = 0;
  wc.cbWndExtra       = 0;
  RegisterClass(&wc);

// init

//  WActiveMsg = 1;
//  WContinuous = 1;
//  WConstantUpdate = sTRUE;
//  WFocus = 1;

  Screen[0].Window = (sU32) CreateWindowEx(
    0,
    "kk",
    "kk",
#if sFULLSCREEN
    WS_POPUP|WS_VISIBLE,
#else
    WS_OVERLAPPEDWINDOW|WS_VISIBLE,
#endif
    0,0,
    ConfigX,ConfigY,
    0,0,
    WInst,0);
  sSystem->WScreenCount = 1;
  sSetMem(GeoBuffer,0,sizeof(GeoBuffer));
  sSetMem(GeoHandle,0,sizeof(GeoHandle));

#if sUSE_DIRECTINPUT
  if(!InitDI())
    sFatal("could not initialise Direct Input!");
#endif

	ZBufFormat = 0;
	ZBuffer = 0;
  for(i=0;i<MAX_TEXTURE;i++)
    Textures[i].Flags = 0;
  InitScreens();

  sFloatFix();
  
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
#if sUSE_DIRECTSOUND
  if(DXSBuffer)
    DXSBuffer->Stop();
//  ExitDS();
#endif
  ExitProcess(0);
}

/****************************************************************************/

void sSystem_::InitScreens()
{
  HRESULT hr;
  D3DPRESENT_PARAMETERS d3dpp;
  RECT r;
  sInt i;
  sU32 create;
  sU16 *iblock;
  sHardTex *tex;
  D3DCAPS9 caps;

  sREGZONE(FlipLock);

  if(!DXD)
  {
    DXD = (*Direct3DCreate9P)(D3D_SDK_VERSION);
    sVERIFY(DXD);
  }

	ZBufFormat=D3DFMT_D24S8;

  Screen[0].SFormat=D3DFMT_A8R8G8B8;
  Screen[0].ZFormat=ZBufFormat;

  WINZERO(d3dpp);
  d3dpp.BackBufferFormat = (enum _D3DFORMAT) Screen[0].SFormat;
  d3dpp.EnableAutoDepthStencil = sFALSE;
  d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  d3dpp.BackBufferCount = 1;
  d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
  d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;// | D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
  d3dpp.hDeviceWindow = (HWND) Screen[0].Window;

#if sFULLSCREEN
  d3dpp.Windowed = FALSE;
  d3dpp.BackBufferWidth = ConfigX;
  d3dpp.BackBufferHeight = ConfigY;
  Screen[0].XSize = ConfigX;
  Screen[0].YSize = ConfigY;
#else
  d3dpp.Windowed = TRUE;
  GetClientRect((HWND) Screen[0].Window,&r);
  Screen[0].XSize = r.right-r.left;
  Screen[0].YSize = r.bottom-r.top;
#endif

  hr=DXD->GetDeviceCaps(0,D3DDEVTYPE_HAL,&caps);
  sVERIFY(!FAILED(hr));

  if(!DXDev)
  {
    if((caps.PixelShaderVersion&0xffff)==0x0000)
      sFatal("pixel shaders required");

    create = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    if(ConfigFlags & sSF_MULTISCREEN) create |= D3DCREATE_ADAPTERGROUP_DEVICE;

    hr = DXD->CreateDevice(0,D3DDEVTYPE_HAL,(HWND) Screen[0].Window,create,&d3dpp,&DXDev);
    if(FAILED(hr))
      sFatal("could not create screen");
    for(i=1;i<FVFMax;i++)
      DXDev->CreateVertexDeclaration(FVFTable[i].Info,&FVFTable[i].Decl);
  }
  else 
  {
    if(DXNormalCube)
    {
      DXNormalCube->Release();
      DXNormalCube = 0;
    }
		if(ZBuffer)
		{
			ZBuffer->Release();
			ZBuffer = 0;
		}
    for(i=0;i<MAX_GEOHANDLE;i++)
    {
      if(GeoHandle[i].Mode!=0)
      {
        if(GeoHandle[i].VertexBuffer>=3)
          GeoBuffer[GeoHandle[i].VertexBuffer].Free();
        if(GeoHandle[i].IndexBuffer>=3)
          GeoBuffer[GeoHandle[i].IndexBuffer].Free();
      }
      GeoHandle[i].VertexCount = 0;
      GeoHandle[i].VertexBuffer = 0;
      GeoHandle[i].IndexCount = 0;
      GeoHandle[i].IndexBuffer = 0;
    }
    for(i=0;i<MAX_GEOBUFFER;i++)
    {
      if(i>=3)
        sVERIFY(GeoBuffer[i].UserCount==0);
      if(GeoBuffer[i].VB)
      {
        GeoBuffer[i].VB->Release();   // VB & IB are the same
      }
      GeoBuffer[i].Init();
    }
		for(i=0;i<MAX_TEXTURE;i++)
		{
			if(Textures[i].Flags & sTIF_RENDERTARGET)
			{
#if LOGGING
        sDPrintF("Release Rendertarget\n");
#endif
				Textures[i].Tex->Release();
				Textures[i].Tex=0;
			}
		}

    DXERROR(DXDev->Reset(&d3dpp));
  }

// zbuffer

	DXERROR(DXDev->CreateDepthStencilSurface(1024,600,(D3DFORMAT) ZBufFormat,D3DMULTISAMPLE_NONE,0,FALSE,&ZBuffer,0));
	DXERROR(DXDev->SetDepthStencilSurface(ZBuffer));

// initialize gpumask

  if(caps.StencilCaps & D3DSTENCILCAPS_TWOSIDED) GpuMask |= sGPU_TWOSIDESTENCIL;

// init buffer management

  CreateGeoBuffer(0,1,1,MAX_DYNVBSIZE);
  CreateGeoBuffer(1,1,0,MAX_DYNIBSIZE);
  CreateGeoBuffer(2,0,0,2*0x8000/4*6);
  DXERROR(GeoBuffer[2].IB->Lock(0,2*0x8000/4*6,(void **) &iblock,0));
  for(i=0;i<0x8000/6;i++)
    sQuad(iblock,i*4+0,i*4+1,i*4+2,i*4+3);
  DXERROR(GeoBuffer[2].IB->Unlock());
  MakeCubeNormalizer();
//  MakeSpecularLookupTex();

// set some defaults

  DXDev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
  DXDev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
  DXDev->SetRenderState(D3DRS_LIGHTING,0);

  WDeviceLost = 0;

  for(i=0;i<MAX_TEXTURE;i++)
	{
		if(Textures[i].Flags & sTIF_RENDERTARGET)
    {
      tex = &Textures[i];
	    DXERROR(DXDev->CreateTexture(tex->XSize,tex->YSize,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&tex->Tex,0));
    }
	}
}

#endif
#endif

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

void sSystem_::Exit()
{
  PostQuitMessage(0);
}

void sSystem_::Tag()
{
}

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

void sSystem_::Log(sChar *s)
{
  if(!sFatality && !sAppHandler(sAPPCODE_DEBUGPRINT,(sU32) s))
    OutputDebugString(s);
}

/****************************************************************************/

#if !sINTRO

void sSystem_::Init(sU32 flags,sInt xs,sInt ys)
{
  ConfigX = xs;
  ConfigY = ys;
  ConfigFlags = flags;
}

#if !sCOMMANDLINE
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
#endif

sInt sSystem_::MemoryUsed()
{
  return MemoryUsedCount;
}

void sSystem_::CheckMem()
{
  _CrtCheckMemory();
}

/****************************************************************************/

void *sSystem_::FindFunc(sChar *name)
{
  return GetProcAddress(0,name);
}

#endif
/****************************************************************************/
/***                                                                      ***/
/***   Render                                                             ***/
/***                                                                      ***/
/****************************************************************************/

#if sLINK_ENGINE
#if sPLAYER

void sSystem_::Progress(sInt done,sInt max)
{
  static sInt lasttime;
  const int step = 3;
  sInt time;
  sRect r;
  sInt key;
  IDirect3DSurface9 *backbuffer;
  IDirect3DSwapChain9 *swapchain;

  time = timeGetTime();
  if(time > lasttime+450 || done==max)
  {
    lasttime = time;
    DXDev->Clear(0,0,D3DCLEAR_TARGET,0xff000000,0,0);

    sZONE(FlipLock);

    DXDev->GetSwapChain(0,&swapchain);
    swapchain->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&backbuffer);
    r.Init(20+step*0,sSCREENY/2-20+step*0,sSCREENX-20-step*0,sSCREENY/2+20-step*0);
    DXDev->ColorFill(backbuffer,(RECT *)&r,0xffffffff);
    r.Init(20+step*1,sSCREENY/2-20+step*1,sSCREENX-20-step*1,sSCREENY/2+20-step*1);
    DXDev->ColorFill(backbuffer,(RECT *)&r,0xff000000);
    r.Init(20+step*2,sSCREENY/2-20+step*2,sSCREENX-20-step*2,sSCREENY/2+20-step*2);
    r.x1 = r.x0 + 1 + sMin(max,done)*(r.x1-r.x0-1)/max;
    DXDev->ColorFill(backbuffer,(RECT *)&r,0xffffffff);
    swapchain->Release();
    backbuffer->Release();

    DXDev->Present(0,0,0,0);

    key = GetAsyncKeyState(VK_ESCAPE);
    if(key&0x8000)
    {
      DXDev->Release(); 
      ExitProcess(0);
    }
  }
}

#endif

/****************************************************************************/

void sSystem_::Render()
{
  static sInt LastTime=-1;
  sInt time,ticks;
  HRESULT hr;
  sInt i;


  RecalcTransform = sTRUE;
  LastCamera.Init();
  LastMatrix.Init();
  LastTransform.Init();
  StdTexTransMat.i.Init(0.5f,0.0f,0.0f,0.0f);
  StdTexTransMat.j.Init(0.0f,0.5f,0.0f,0.0f);
  StdTexTransMat.k.Init(0.0f,0.0f,0.5f,0.0f);
  StdTexTransMat.l.Init(0.5f,0.5f,0.5f,1.0f);
  StdTexTransMatSet[0] = 0;
  for(i=0;i<8;i++)
    StdTexTransMatSet[i] = 0;

  if(WDeviceLost)
  {
    Sleep(100);
    hr = DXDev->TestCooperativeLevel();
    if(hr!=D3DERR_DEVICENOTRESET)
      return;
    InitScreens();
    if(WDeviceLost)
      return;
  }

  time = sSystem->GetTime();
  ticks = 0;
  if(LastTime==-1)
    LastTime = time;
  while(time>LastTime)
  {
    LastTime+=10;
    ticks++;
  }
  if(ticks>10)
    ticks=10;

  sAppHandler(sAPPCODE_FRAME,0);
  sAppHandler(sAPPCODE_TICK,ticks);
  sAppHandler(sAPPCODE_PAINT,0);

#if !sPLAYER
#if sLINK_UTIL
  sPerfMon->Marker(1);
#endif
#endif

#if !sINTRO
  if(WResponse)
#endif
  {
    IDirect3DSurface9 *backbuffer;
    IDirect3DSwapChain9 *swapchain;
    D3DLOCKED_RECT lr;
    sZONE(FlipLock);

    DXERROR(DXDev->GetSwapChain(0,&swapchain));
    DXERROR(swapchain->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&backbuffer));
    DXERROR(backbuffer->LockRect(&lr,0,D3DLOCK_READONLY));
    backbuffer->UnlockRect();
    swapchain->Release();
    backbuffer->Release();
  }

  hr = DXDev->Present(0,0,0,0);
  if(hr==D3DERR_DEVICELOST)
    WDeviceLost = 1;

  for(i=1;i<MAX_GEOHANDLE;i++)
  {
    GeoHandle[i].VBDiscardCount = 0;
    GeoHandle[i].IBDiscardCount = 0;
  }
  VBDiscardCount = 1;
  IBDiscardCount = 1;
  GeoBuffer[0].Used = GeoBuffer[0].Size;
  GeoBuffer[1].Used = GeoBuffer[1].Size;
}

void sSystem_::SetGamma(sF32 gamma)
{
  D3DGAMMARAMP ramp;

  sInt i;
  for(i=0;i<256;i++)
    ramp.red[i] = ramp.green[i] = ramp.blue[i] = sFPow(i/255.0f,1.0f/gamma)*0xffff;
  DXDev->SetGammaRamp(0,D3DSGR_NO_CALIBRATION,&ramp);
}

/****************************************************************************/
/****************************************************************************/

void sSystem_::BeginViewport(const sViewport &vp)
{
  D3DVIEWPORT9 dview,fview;
  sInt nr;
  sInt xs,ys,dxs,dys;
  sU32 stencilClear;
	sHardTex *tex;
  IDirect3DSurface9 *backbuffer;
  IDirect3DSwapChain9 *swapchain;

  CurrentViewport = vp;

  nr = vp.Screen;
	if(vp.RenderTarget==sINVALID)
	{
		ScreenNr = nr;
		scr = &Screen[nr];
		sVERIFY(nr>=0 && nr<WScreenCount);

		dxs = Screen[nr].XSize;
		dys = Screen[nr].YSize;
	}
	else
	{
		sVERIFY(vp.RenderTarget >= 0);
		tex = &Textures[vp.RenderTarget];
		sVERIFY(tex->Flags & sTIF_RENDERTARGET);

		dxs = tex->XSize;
		dys = tex->YSize;
	}

	xs = vp.Window.XSize();
	ys = vp.Window.YSize();
	if(xs==0 || ys==0)
	{
		dview.X = 0;
		dview.Y = 0;
		dview.Width = dxs;
		dview.Height = dys;
    CurrentViewport.Window.x0 = 0;
    CurrentViewport.Window.y0 = 0;
    CurrentViewport.Window.x1 = dxs;
    CurrentViewport.Window.y1 = dys;
	}
	else
	{
		dview.X = vp.Window.x0;
		dview.Y = vp.Window.y0;
		dview.Width = vp.Window.x1-vp.Window.x0;
		dview.Height = vp.Window.y1-vp.Window.y0;
	}
	ViewportX = dview.Width;
	ViewportY = dview.Height;
	dview.MinZ = 0.0f;
	dview.MaxZ = 1.0f;

	if(vp.RenderTarget!=sINVALID)
	{
		DXERROR(tex->Tex->GetSurfaceLevel(0,&backbuffer));
	}
	else
	{
		DXERROR(DXDev->GetSwapChain(nr,&swapchain));
		DXERROR(swapchain->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&backbuffer));
		swapchain->Release();
	}

	DXERROR(DXDev->SetRenderTarget(0,backbuffer));
	backbuffer->Release();

	if(nr!=-1 || (vp.ClearFlags & sVCF_PARTIAL))
		DXDev->SetViewport(&dview);
	else
	{
		fview.X = 0; // always clear full surface for textures
		fview.Y = 0;
		fview.Width = tex->XSize;
		fview.Height = tex->YSize;
		fview.MinZ = 0.0f;
		fview.MaxZ = 1.0f;
		DXDev->SetViewport(&fview);
	}

  stencilClear = (ZBufFormat==D3DFMT_D24S8 || ZBufFormat==D3DFMT_D15S1) ? D3DCLEAR_STENCIL : 0;

  switch(vp.ClearFlags&3)
  {
  case sVCF_COLOR:
    DXDev->Clear(0,0,D3DCLEAR_TARGET,vp.ClearColor.Color,1.0f,0);
    break;
  case sVCF_Z:
    DXDev->Clear(0,0,D3DCLEAR_ZBUFFER|stencilClear,vp.ClearColor.Color,1.0f,0);
    break;
  case sVCF_ALL:
    DXDev->Clear(0,0,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|stencilClear,vp.ClearColor.Color,1.0f,0);
    break;
  }

	if(nr==-1)
		DXDev->SetViewport(&dview);

  DXERROR(DXDev->BeginScene());
}

void sSystem_::EndViewport()
{
  DXERROR(DXDev->EndScene());
}


void sSystem_::GetTransform(sInt mode,sMatrix &mat)
{
  switch(mode)
  {
  case sGT_UNIT:
    mat.Init();
    break;
  case sGT_MODELVIEW:
    mat = LastTransform;
    break;

  case sGT_MODEL:
    mat = LastMatrix;
    break;

  case sGT_VIEW:
    mat = LastCamera;
    break;

  case sGT_PROJECT:
    mat = LastProjection;
    break;

  default:
    mat.Init();
    break;
  }
}


/****************************************************************************/
/****************************************************************************/
/*
void sSystem_::SetTexture(sInt stage,sInt handle,sMatrix *mat)
{
  sHardTex *tex;

  if(handle==sINVALID)
  {
    DXDev->SetTexture(stage,0);
    return;
  }

  tex = &Textures[handle];
  DXDev->SetTexture(stage,tex->Tex);
  if(mat)
  {
    DXDev->SetTransform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0+stage),(D3DMATRIX *) mat);
    StdTexTransMatSet[stage] = 0;
  }
  else
  {
    if(!StdTexTransMatSet[stage])
    {
      DXDev->SetTransform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0+stage),(D3DMATRIX *) &StdTexTransMat);
      StdTexTransMatSet[stage] = 1;
    }
  }
}
*/
/****************************************************************************/

sInt sSystem_::AddTexture(const sTexInfo &ti)
{
  sInt i;
  sHardTex *tex;

  tex = Textures;
  for(i=0;i<MAX_TEXTURE;i++)
  {
    if(tex->Flags==0)
    {
      tex->RefCount = 1;
      tex->XSize = ti.XSize;
      tex->YSize = ti.YSize;
      tex->Flags = ti.Flags|sTIF_ALLOCATED;
      tex->Format = sTF_A8R8G8B8;
      tex->Tex = 0;
      tex->TexGL = 0;

	    if(tex->Flags & sTIF_RENDERTARGET)
	    {
        if(tex->XSize==0)
        {
          tex->XSize = Screen[0].XSize;
          tex->YSize = Screen[0].YSize;
        }
//        sVERIFY(tex->XSize<=Screen[0].XSize);
//        sVERIFY(tex->YSize<=Screen[0].YSize);
        #if LOGGING
        sDPrintF("Create Rendertarget %dx%d\n",tex->XSize,tex->YSize);
        #endif
		    DXERROR(DXDev->CreateTexture(tex->XSize,tex->YSize,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&tex->Tex,0));
#if !sINTRO
        ReCreateZBuffer();
#endif
	    }
	    else
	    {
        #if LOGGING
        sDPrintF("Create Texture %dx%d\n",tex->XSize,tex->YSize);
        #endif
		    DXERROR(DXDev->CreateTexture(tex->XSize,tex->YSize,0,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&tex->Tex,0));
        UpdateTexture(i,ti.Bitmap);
	    }

      return i;
    }
    tex++;
  }

  return sINVALID;
}

sInt sSystem_::AddTexture(sInt xs,sInt ys,sInt format,sU16 *data,sInt mipcount,sInt miptresh)
{
  sInt i;
  sHardTex *tex;
#if sUSE_SHADERS
  static D3DFORMAT formats[] = 
  {
    D3DFMT_UNKNOWN,
    D3DFMT_A8R8G8B8,
    D3DFMT_A8,
    D3DFMT_R16F,
    D3DFMT_A2R10G10B10,
    D3DFMT_Q8W8V8U8,
    D3DFMT_A2W10V10U10,
  };
#else
  static D3DFORMAT formats[] = 
  {
    D3DFMT_UNKNOWN,
    D3DFMT_A8R8G8B8,
    D3DFMT_A8,
    D3DFMT_A8R8G8B8,
    D3DFMT_A8R8G8B8,
    D3DFMT_A8R8G8B8,
    D3DFMT_A8R8G8B8,
  };
#endif

  sVERIFY(format>0 && format<sTF_MAX);

  tex = Textures;
  for(i=0;i<MAX_TEXTURE;i++)
  {
    if(tex->Flags==0)
    {
      tex->RefCount = 1;
      tex->XSize = xs;
      tex->YSize = ys;
      tex->Flags = sTIF_ALLOCATED;
      tex->Format = format;
      tex->Tex = 0;
      tex->TexGL = 0;

	    if(data==0)
	    {
        if(tex->XSize==0)
        {
          tex->XSize = Screen[0].XSize;
          tex->YSize = Screen[0].YSize;
        }
        tex->Flags |= sTIF_RENDERTARGET;
//        sVERIFY(tex->XSize<=Screen[0].XSize);
//        sVERIFY(tex->YSize<=Screen[0].YSize);
        #if LOGGING
        sDPrintF("Create Rendertarget %dx%d\n",tex->XSize,tex->YSize);
        #endif
		    DXERROR(DXDev->CreateTexture(tex->XSize,tex->YSize,1,D3DUSAGE_RENDERTARGET,formats[format],D3DPOOL_DEFAULT,&tex->Tex,0));
#if !sINTRO
        ReCreateZBuffer();
#endif
	    }
	    else
	    {
        #if LOGGING
        sDPrintF("Create Texture %dx%d\n",tex->XSize,tex->YSize);
        #endif
		    DXERROR(DXDev->CreateTexture(tex->XSize,tex->YSize,mipcount,0,formats[format],D3DPOOL_MANAGED,&tex->Tex,0));
        UpdateTexture(i,data,miptresh);
	    }

      return i;
    }
    tex++;
  }
  return sINVALID;
}

/****************************************************************************/

void sSystem_::AddRefTexture(sInt handle)
{
//  sHardTex *tex;

  if(handle!=sINVALID)
  {
    sVERIFY(handle>=0 && handle<MAX_TEXTURE);
    sVERIFY(Textures[handle].RefCount>=0)
    Textures[handle].RefCount++;
  }
}

/****************************************************************************/

void sSystem_::RemTexture(sInt handle)
{
  sHardTex *tex;

  if(handle!=sINVALID)
  {
    sVERIFY(handle>=0 && handle<MAX_TEXTURE);
    tex = &Textures[handle];
    sVERIFY(tex->RefCount>=1)
    tex->RefCount--;
    if(tex->RefCount==0)
    {
      if(tex->Tex)
        tex->Tex->Release();

      tex->Flags = 0;
      tex->Tex = 0;
    }
  }
}

/****************************************************************************/

void sSystem_::MakeCubeNormalizer()
{
  const sInt size = 64;
  sInt i,x,y;
  D3DLOCKED_RECT lr;
  sU8 *p;
  sVector v;

  static sVector faces[6][2] = 
  {
    {{ 0, 0,-1},{ 0, 1, 0}},
    {{ 0, 0, 1},{ 0, 1, 0}},
    {{ 1, 0, 0},{ 0, 0,-1}},
    {{ 1, 0, 0},{ 0, 0, 1}},
    {{ 1, 0, 0},{ 0, 1, 0}},
    {{-1, 0, 0},{ 0, 1, 0}}
  };

	DXERROR(DXDev->CreateCubeTexture(size,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&DXNormalCube,0));
  for(i=0;i<6;i++)
  {
    DXERROR(DXNormalCube->LockRect((D3DCUBEMAP_FACES)i,0,&lr,0,0));
    for(y=0;y<size;y++)
    {
      p = ((sU8*)lr.pBits)+y*lr.Pitch;
      for(x=0;x<size;x++)
      {
        v.Cross3(faces[i][0],faces[i][1]);
        v.Scale3((size-1)*0.5f);
        v.AddScale3(faces[i][0],(x-(size-1)*0.5f));
        v.AddScale3(faces[i][1],(-y+(size-1)*0.5f));
        v.Unit3();
        p[0] = sFtol(128.0f+v.z*127);
        p[1] = sFtol(128.0f+v.y*127);
        p[2] = sFtol(128.0f+v.x*127);
        p[3] = 0;
        p+=4;
      }
    }
    DXERROR(DXNormalCube->UnlockRect((D3DCUBEMAP_FACES)i,0));
  }
}

/****************************************************************************/

/*
void sSystem_::MakeSpecularLookupTex()
{
  sU16 lookup[256*4],*lp;
  sInt i,v;
  sF32 x;

  lp = lookup;
  for(i=0;i<256;i++)
  {
    x = i / 255.0f;
    v = 32767 * sFPow(x,32.0f); // specular power is 32
    *lp++ = v;
    *lp++ = v;
    *lp++ = v;
    *lp++ = v;
  }

  SpecularLookupTex = AddTexture(256,1,sTF_A8R8G8B8,lookup);

  lp = lookup;
  for(i=0;i<256;i++)
  {
    *lp++ = 0x4000;
    *lp++ = 0x4000;
    *lp++ = 0x8000;
    *lp++ = 0x4000;
  }

  NullBumpTex = AddTexture(16,16,sTF_Q8W8V8U8,lookup);
}
*/
/****************************************************************************/

sBool sSystem_::StreamTextureStart(sInt handle,sInt level,sBitmapLock &lock)
{
  sHardTex *tex;
  IDirect3DTexture9 *mst;
  D3DLOCKED_RECT dxlock;

  tex = &Textures[handle];
  mst = tex->Tex;

  sVERIFY(!DXStreamTexture);
  DXStreamTexture = mst;
  DXStreamLevel = level;

  if(FAILED(mst->LockRect(level,&dxlock,0,0)))
    return sFALSE;

  lock.Data = (sU8 *)dxlock.pBits;
  lock.XSize = tex->XSize>>level;
  lock.YSize = tex->YSize>>level;
  lock.Kind = tex->Format;
  lock.BPR = dxlock.Pitch;

  return sTRUE;
}

/****************************************************************************/

void sSystem_::StreamTextureEnd()
{
  sVERIFY(DXStreamTexture);
  DXStreamTexture->UnlockRect(DXStreamLevel);
  DXStreamTexture=0;
}

/****************************************************************************/

void sSystem_::ReadTexture(sInt handle,sU32 *data)
{
  sHardTex *tex;
  IDirect3DSurface9 *rs;
  IDirect3DSurface9 *ms;
  D3DLOCKED_RECT lr;
  sInt y;
  sU8 *s;
  
  tex = &Textures[handle];
  sVERIFY(tex->Format==sTF_A8R8G8B8);

  if(DXReadTexXS!=tex->XSize || DXReadTexYS!=tex->YSize)
  {
    if(DXReadTex)
      DXReadTex->Release();
    DXReadTex = 0;
    DXReadTexXS = tex->XSize;
    DXReadTexYS = tex->YSize;
    DXERROR(DXDev->CreateOffscreenPlainSurface(tex->XSize,tex->YSize,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&DXReadTex,0));
  }
  ms = DXReadTex;
  DXERROR(tex->Tex->GetSurfaceLevel(0,&rs));

  DXERROR(DXDev->GetRenderTargetData(rs,ms));
  DXERROR(ms->LockRect(&lr,0,D3DLOCK_READONLY));
  s = (sU8 *)lr.pBits;
  for(y=0;y<tex->YSize;y++)
  {
    sCopyMem(data,s,tex->XSize*4);
    data+=tex->XSize;
    s+=lr.Pitch;
  }
  DXERROR(ms->UnlockRect());

  rs->Release();
}

/****************************************************************************/
/****************************************************************************/

void sSystem_::SetStates(sU32 *stream)
{
//  DXDev->SetPixelShader(0);
//  DXDev->SetVertexShader(0);
  while(*stream!=0xffffffff)
  {
    SetState(stream[0],stream[1]);
    stream+=2;
  }
}

/****************************************************************************/

void sSystem_::SetState(sU32 token,sU32 value)
{
  if(token<0x0310)
  {
    if(token<0x0100)
    {
      DXDev->SetRenderState((enum _D3DRENDERSTATETYPE)token,value);
    }
    else if(token<0x0200)
    {
      DXDev->SetTextureStageState(((token>>5)&7),(enum _D3DTEXTURESTAGESTATETYPE)(token&31),value);
    }
    else if(token<0x0300)
    {
      DXDev->SetSamplerState(((token>>4)&15),(enum _D3DSAMPLERSTATETYPE)(token&15),value);
    }
    else if(token<0x0310)
    {
      DXDev->SetSamplerState(D3DDMAPSAMPLER,(enum _D3DSAMPLERSTATETYPE)(token&15),value);
    }
  }
}

/****************************************************************************/

void sSystem_::SetScissor(const sFRect *scissor)
{
  RECT rc;

  if(!scissor)
    SetState(sD3DRS_SCISSORTESTENABLE,0);
  else
  {
    rc.left   = CurrentViewport.Window.x0 + (CurrentViewport.Window.x1 - CurrentViewport.Window.x0) * (1.0f + scissor->x0) * 0.5f;
    rc.right  = CurrentViewport.Window.x0 + (CurrentViewport.Window.x1 - CurrentViewport.Window.x0) * (1.0f + scissor->x1) * 0.5f;
    rc.top    = CurrentViewport.Window.y0 + (CurrentViewport.Window.y1 - CurrentViewport.Window.y0) * (1.0f - scissor->y1) * 0.5f;
    rc.bottom = CurrentViewport.Window.y0 + (CurrentViewport.Window.y1 - CurrentViewport.Window.y0) * (1.0f - scissor->y0) * 0.5f;

    DXDev->SetScissorRect(&rc);
    SetState(sD3DRS_SCISSORTESTENABLE,1);
  }
}

/****************************************************************************/
/****************************************************************************/

void sSystem_::CreateGeoBuffer(sInt i,sInt dyn,sInt vertex,sInt size)
{
  sInt usage;
  D3DPOOL pool;

  sVERIFY(dyn==0 || dyn==1);
  sVERIFY(vertex==0 || vertex==1);

  GeoBuffer[i].Type = 1+vertex;
  GeoBuffer[i].Size = size;
  GeoBuffer[i].Used = 0;
  GeoBuffer[i].UserCount = 0;
  GeoBuffer[i].VB = 0;

  usage = D3DUSAGE_WRITEONLY;
  pool = D3DPOOL_MANAGED;
  if(dyn)
  {
    usage |= D3DUSAGE_DYNAMIC;
    pool = D3DPOOL_DEFAULT;
  }

  if(vertex)
  {
    DXERROR(DXDev->CreateVertexBuffer(size,usage,0,pool,&GeoBuffer[i].VB,0));
  }
  else
  {
    DXERROR(DXDev->CreateIndexBuffer(size,usage,D3DFMT_INDEX16,pool,&GeoBuffer[i].IB,0));  
  }
#if LOGGING
  sDPrintF("Create %s %s-Buffer (%d bytes)\n",dyn?"dynamic":"static",vertex?"vertex":"index",size);
#endif
}

#undef GeoAdd
sInt sSystem_::GeoAdd(sInt fvf,sInt prim)
{
  sInt i;
  sGeoHandle *gh;

  sVERIFY(fvf>=1 && fvf<FVFMax);
  for(i=1;i<MAX_GEOHANDLE;i++)
  {
    gh = &GeoHandle[i];
    if(gh->Mode==0)
    {
      sSetMem(gh,0,sizeof(*gh));
      gh->VertexSize = FVFTable[fvf].Size;
      gh->FVF = fvf;
      gh->Mode = prim;
      gh->Locked = 0;
      gh->File = "";
      gh->Line = 0;
      return i;
    }
  }
  sFatal("GeoAdd() ran out of handles");
	return 0;
}

#if !sINTRO
sInt sSystem_::GeoAdd(sInt fvf,sInt prim,const sChar *file,sInt line)
{
  sInt i;
  sGeoHandle *gh;
  static sInt AllocId=0;

  sVERIFY(fvf>=1 && fvf<FVFMax);
  for(i=1;i<MAX_GEOHANDLE;i++)
  {
    gh = &GeoHandle[i];
    if(gh->Mode==0)
    {
      sSetMem(gh,0,sizeof(*gh));
      gh->VertexSize = FVFTable[fvf].Size;
      gh->FVF = fvf;
      gh->Mode = prim;
      gh->Locked = 0;
      gh->File = file;
      gh->Line = line;
      gh->AllocId = AllocId++;
      if(gh->AllocId == -1)
        __asm int 3;
      return i;
    }
  }
  sFatal("GeoAdd() ran out of handles");
	return 0;
}
#define GeoAdd(f,p) GeoAdd(f,p,__FILE__,__LINE__)
#endif

void sSystem_::GeoRem(sInt handle)
{
  sVERIFY(handle>=1 && handle<MAX_GEOHANDLE);
  sVERIFY(GeoHandle[handle].Mode!=0);

  if(GeoHandle[handle].VertexBuffer>=3)
    GeoBuffer[GeoHandle[handle].VertexBuffer].Free();
  if(GeoHandle[handle].IndexBuffer>=3)
    GeoBuffer[GeoHandle[handle].IndexBuffer].Free();
  GeoHandle[handle].Mode = 0;
}

void sSystem_::GeoFlush()
{
  sInt i;

  for(i=1;i<MAX_GEOHANDLE;i++)
    GeoHandle[i].VertexCount = 0;
  for(i=3;i<MAX_GEOBUFFER;i++)
  {
    if(GeoBuffer[i].VB)
      GeoBuffer[i].VB->Release();
    GeoBuffer[i].Init();
  }
}

void sSystem_::GeoFlush(sInt handle,sInt what)
{
  sVERIFY(handle>=1 && handle<MAX_GEOHANDLE);
  if(what & sGEO_VERTEX)  GeoHandle[handle].VBDiscardCount = 0;
  if(what & sGEO_INDEX)   GeoHandle[handle].IBDiscardCount = 0;
}

sInt sSystem_::GeoDraw(sInt &handle)
{
  sGeoHandle *gh;
  enum _D3DPRIMITIVETYPE mode;
  sInt count;
  sInt update;

// buffer correctly loaded?

  sVERIFY(handle>=1 && handle<MAX_GEOHANDLE);
  sVERIFY(GeoHandle[handle].Mode!=0);

  gh = &GeoHandle[handle];
  update = 0;

  if(gh->VertexCount == 0)
    update |= sGEO_VERTEX | sGEO_INDEX;
  if(gh->VertexBuffer == 0 && gh->VBDiscardCount != VBDiscardCount)
    update |= sGEO_VERTEX;
  if(gh->IndexBuffer == 1 && gh->IBDiscardCount != IBDiscardCount)
    update |= sGEO_INDEX;

  if(update)
    return update;

// set up buffers

  DXDev->SetVertexDeclaration(FVFTable[gh->FVF].Decl);
  if(gh->IndexCount)
  {
    sVERIFY(gh->IndexBuffer>=0 && gh->IndexBuffer<MAX_GEOBUFFER);
    sVERIFY(GeoBuffer[gh->IndexBuffer].Type == 1);
    sVERIFY(GeoBuffer[gh->IndexBuffer].IB);
    DXDev->SetIndices(GeoBuffer[gh->IndexBuffer].IB);
  }
  sVERIFY(gh->VertexBuffer>=0 && gh->VertexBuffer<MAX_GEOBUFFER);
  sVERIFY(GeoBuffer[gh->VertexBuffer].Type == 2);
  sVERIFY(GeoBuffer[gh->VertexBuffer].VB);
  DXDev->SetStreamSource(0,GeoBuffer[gh->VertexBuffer].VB,0,gh->VertexSize);

// start drawing

  count = gh->IndexCount;
  if(count==0)
    count = gh->VertexCount;
  PerfThis.Vertex += gh->VertexCount;
  PerfThis.Batches++;

  switch(gh->Mode&7)
  {
  case sGEO_POINT:
    mode = D3DPT_POINTLIST;
    count = count;
    sVERIFY(gh->IndexCount==0);
    break;
  case sGEO_LINE:
    mode = D3DPT_LINELIST;
    count = count/2;
    PerfThis.Line += count;
    break;
  case sGEO_LINESTRIP:
    mode = D3DPT_LINESTRIP;
    count = count-1;
    PerfThis.Line += count;
    break;
  case sGEO_TRI:
    mode = D3DPT_TRIANGLELIST;
    count = count/3;
    PerfThis.Triangle += count;
    break;
  case sGEO_TRISTRIP:
    mode = D3DPT_TRIANGLESTRIP;
    count = count-2;
    PerfThis.Triangle += count;
    break;
  case sGEO_QUAD:
    sVERIFY(gh->IndexCount==0);
    PerfThis.Triangle += count/2;
    DXDev->SetIndices(GeoBuffer[2].IB);
    DXDev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,gh->VertexStart,0,gh->VertexCount,0,count/4*2);
    return sFALSE;
  default:
    sVERIFYFALSE;
  }
  if(gh->IndexCount)
    DXDev->DrawIndexedPrimitive(mode,gh->VertexStart,0,gh->VertexCount,gh->IndexStart,count);
  else
    DXDev->DrawPrimitive(mode,gh->VertexStart,count);

  return sFALSE;
}

void sSystem_::GeoBegin(sInt handle,sInt vc,sInt ic,sF32 **fp,sU16 **ip,sInt upd)
{
  sGeoHandle *gh;
  sInt i,lockf;
  sInt ok;

  sVERIFY(handle>=1 && handle<MAX_GEOHANDLE);
  sVERIFY(GeoHandle[handle].Mode!=0);
  sVERIFY(vc*GeoHandle[handle].VertexSize<=MAX_DYNVBSIZE); 
  sVERIFY(ic*2<=MAX_DYNIBSIZE);

  gh = &GeoHandle[handle];

  *fp = 0;
  if(ip)
    *ip = 0;

  if(upd & sGEO_VERTEX)
  {
    if(gh->VertexBuffer>=3)
      GeoBuffer[gh->VertexBuffer].Free();

    gh->VertexBuffer = 0;
    gh->VertexStart = 0;
    gh->VertexCount = vc;
  
    if(gh->Mode & sGEO_STATVB) // static vertex buffer
    {
      for(i=3;i<MAX_GEOBUFFER;i++)
        if(GeoBuffer[i].AllocVB(vc,gh->VertexSize,gh->VertexStart))
          break;

      if(i == MAX_GEOBUFFER)
      {
        for(i=3;i<MAX_GEOBUFFER;i++)
        {
          if(GeoBuffer[i].Type == 0)
          {
            CreateGeoBuffer(i,0,1,MAX_DYNVBSIZE);
            ok = GeoBuffer[i].AllocVB(vc,gh->VertexSize,gh->VertexStart);
            sVERIFY(ok);
            break;
          }
        }

        if(i == MAX_GEOBUFFER)
          sFatal("GeoBegin(): no free static vertex buffer");
      }

      gh->VertexBuffer = i;
      lockf = 0;
    }
    else // dynamic vertex buffer
    {
      ok = GeoBuffer[0].AllocVB(vc,gh->VertexSize,gh->VertexStart);
      gh->VertexBuffer = 0;

      if(!ok)
      {
        GeoBuffer[0].Used = 0;
        VBDiscardCount++;
        ok = GeoBuffer[0].AllocVB(vc,gh->VertexSize,gh->VertexStart);
        sVERIFY(ok);
      }

      gh->VBDiscardCount = VBDiscardCount;
      lockf = gh->VertexStart ? D3DLOCK_NOOVERWRITE : D3DLOCK_DISCARD;
    }
    DXERROR(GeoBuffer[gh->VertexBuffer].VB->Lock(gh->VertexStart*gh->VertexSize,vc*gh->VertexSize,(void **)fp,lockf));
    gh->Locked |= sGEO_VERTEX;
  }

  if(upd & sGEO_INDEX)
  {
    if(gh->IndexBuffer>=3)
      GeoBuffer[gh->IndexBuffer].Free();

    gh->IndexBuffer = 0;
    gh->IndexStart = 0;
    gh->IndexCount = ic;

    if(ic)
    {
      if(gh->Mode & sGEO_STATIB) // static index buffer
      {
        for(i=3;i<MAX_GEOBUFFER;i++)
          if(GeoBuffer[i].AllocIB(ic,gh->IndexStart))
            break;

        if(i == MAX_GEOBUFFER)
        {
          for(i=3;i<MAX_GEOBUFFER;i++)
          {
            if(GeoBuffer[i].Type == 0)
            {
              CreateGeoBuffer(i,0,0,MAX_DYNIBSIZE);
              ok = GeoBuffer[i].AllocIB(ic,gh->IndexStart);
              sVERIFY(ok);
              break;
            }
          }

          if(i == MAX_GEOBUFFER)
            sFatal("GeoBegin(): no free static index buffer");
        }

        gh->IndexBuffer = i;
        lockf = 0;
      }
      else // dynamic index buffer
      {
        gh->IndexBuffer = 1;
        ok = GeoBuffer[1].AllocIB(ic,gh->IndexStart);

        if(!ok)
        {
          GeoBuffer[1].Used = 0;
          IBDiscardCount++;

          ok = GeoBuffer[1].AllocIB(ic,gh->IndexStart);
          sVERIFY(ok);
        }

        gh->IBDiscardCount = IBDiscardCount;
        lockf = gh->IndexStart ? D3DLOCK_NOOVERWRITE : D3DLOCK_DISCARD;
      }

      DXERROR(GeoBuffer[gh->IndexBuffer].IB->Lock(gh->IndexStart*2,ic*2,(void **)ip,lockf));
      gh->Locked |= sGEO_INDEX;
    }
  }
}

void sSystem_::GeoEnd(sInt handle,sInt vc,sInt ic)
{
  sGeoHandle *gh;
//  sBool load;

  sVERIFY(handle>=1 && handle<MAX_GEOHANDLE);
  sVERIFY(GeoHandle[handle].Mode!=0);

  gh = &GeoHandle[handle];

  if(gh->Locked & sGEO_VERTEX)
  {
    GeoBuffer[gh->VertexBuffer].VB->Unlock();
    gh->Locked &= ~sGEO_VERTEX;
  }

  if(gh->Locked & sGEO_INDEX)
  {
    GeoBuffer[gh->IndexBuffer].IB->Unlock();
    gh->Locked &= ~sGEO_INDEX;
  }

  if(vc!=-1)
    gh->VertexCount = vc;
  if(ic!=-1)
    gh->IndexCount = ic;
}


/****************************************************************************/
/****************************************************************************/

#if !sINTRO
sBool sSystem_::GetScreenInfo(sInt i,sScreenInfo &info)
{
  if(i<0||i>=WScreenCount)
  {
    info.XSize = 0;
    info.YSize = 0;
    info.FullScreen = 0;
    info.SwapVertexColor = 0;
    info.ShaderLevel = sPS_00;
    info.LowQuality = 0;
    info.PixelRatio = 1;
    return sFALSE;
  }
  else
  {
    info.XSize = Screen[i].XSize;
    info.YSize = Screen[i].YSize;
    info.FullScreen = WFullscreen;
    info.SwapVertexColor = (ConfigFlags & sSF_OPENGL)?1:0;
    info.ShaderLevel = CmdShaderLevel;
    info.LowQuality = CmdLowQuality;
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
#endif

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

void sSystem_::GetTexSize(sInt handle,sInt &xs,sInt &ys)
{
  sHardTex *tex;

  sVERIFY(handle>=0 && handle<MAX_TEXTURE);
  tex = &Textures[handle];
  sVERIFY(tex->Flags);

  xs = tex->XSize;
  ys = tex->YSize;
}

/****************************************************************************/

void sSystem_::UpdateTexture(sInt handle,sBitmap *bm)
{
  sHardTex *tex;
  sInt xs,ys,level;
  sInt miptresh;
  sInt mipdir;
  sInt filter;
  IDirect3DTexture9 *mst;
  D3DLOCKED_RECT dxlock;

  sInt x,y;
  sInt bpr,oxs;
  sU32 *d,*s,*data;
  sU8 *db,*sb;

  miptresh = 0;
  tex = &Textures[handle];
  mst = tex->Tex;
  mipdir = miptresh & 16;
  miptresh = miptresh & 15;

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

    {    
      for(;;)
		  {
        if(FAILED(mst->LockRect(level,&dxlock,0,0)))
          break;
        bpr = dxlock.Pitch;
			  d = (sU32 *)dxlock.pBits;
			  s = data;
			  for(y=0;y<ys;y++)
			  {
				  sCopyMem4(d,s,xs);
				  d+=bpr/4;
				  s+=oxs;
			  }
        mst->UnlockRect(level);
			  if(xs<=1 || ys<=1)
				  break;
  	    
        filter = (miptresh <= level);
        if(mipdir) filter = !filter;

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
            if(filter)
            {
					    db[0] = (sb[0]+sb[4]+sb[oxs*4+0]+sb[oxs*4+4]+2)>>2;
					    db[1] = (sb[1]+sb[5]+sb[oxs*4+1]+sb[oxs*4+5]+2)>>2;
					    db[2] = (sb[2]+sb[6]+sb[oxs*4+2]+sb[oxs*4+6]+2)>>2;
					    db[3] = (sb[3]+sb[7]+sb[oxs*4+3]+sb[oxs*4+7]+2)>>2;
            }
            else
            {
					    db[0] = sb[0];
					    db[1] = sb[1];
					    db[2] = sb[2];
					    db[3] = sb[3];
            }
					  db+=4;
					  sb+=8;
				  }
			  }

			  xs=xs>>1;
			  ys=ys>>1;
			  level++;
		  }
    }
	  delete[] data;
	}
}

/****************************************************************************/

void sSystem_::UpdateTexture(sInt handle,sU16 *source,sInt miptresh)
{
  sHardTex *tex;
  IDirect3DTexture9 *mst;
  D3DLOCKED_RECT dxlock;
  sInt xs,ys,level;
  sInt mipdir;
  sInt filter;
  sInt alpha;

  sInt x,y;
  sInt bpr,oxs;
  sU32 *d,*s,*data;
  sU16 *d16,*s16;
//  sU8 *d8;

  mipdir = miptresh & 16;
  alpha = miptresh & 32;
  miptresh = miptresh & 15;
 
  tex = &Textures[handle];
  mst = tex->Tex;

	if(!(tex->Flags & sTIF_RENDERTARGET))
	{
		xs = tex->XSize;
		ys = tex->YSize;
		oxs = xs;
		level = 0;

		data = new sU32[xs*ys*2];
		sCopyMem4(data,(sU32 *)source,xs*ys*2);

    {
      for(;;)
		  {
        if(FAILED(mst->LockRect(level,&dxlock,0,0)))
          break;
        bpr = dxlock.Pitch;
			  d = (sU32 *)dxlock.pBits;
			  s = data;
			  for(y=0;y<ys;y++)
			  {
          if(tex->Format==sTF_Q8W8V8U8)
          {
            s16 = (sU16 *)s;
            for(x=0;x<xs;x++)
            {
              d[x] = (((((sInt)s16[2]-0x4000)>>7)&0xff)    ) | 
                    (((((sInt)s16[1]-0x4000)>>7)&0xff)<< 8) | 
                    (((((sInt)s16[0]-0x4000)>>7)&0xff)<<16) | 
                    (((((sInt)s16[3]-0x4000)>>7)&0xff)<<24);
              s16+=4;
            }
          }
          else
          {
            s16 = (sU16 *)s;
            for(x=0;x<xs;x++)
            {
              d[x] = (((s16[0]>>7)&0xff)    ) | 
                      (((s16[1]>>7)&0xff)<< 8) | 
                      (((s16[2]>>7)&0xff)<<16) | 
                      (((s16[3]>>7)&0xff)<<24);
              s16+=4;
            }
          }

          /*
          switch(tex->Format+alpha)
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

          case sTF_A8R8G8B8+32:
            s16 = (sU16 *)s;
            for(x=0;x<xs;x++)
            {
              d[x] = (((s16[0]>>7)&0xff)    ) | 
                     (((s16[1]>>7)&0xff)<< 8) | 
                     (((s16[2]>>7)&0xff)<<16) | 
                     (s16[3]>0x4000?0xff000000:0);
              s16+=4;
            }
            break;

          case sTF_A8:
            s16 = (sU16 *)s;
            d8 = (sU8 *)d;

            for(x=0;x<xs;x++)
            {
              d8[x] = (((s16[3]>>7)&0xff)    );
              s16+=4;
            }
            break;

          case sTF_A8+32:
            s16 = (sU16 *)s;
            d8 = (sU8 *)d;

            for(x=0;x<xs;x++)
            {
              d8[x] = s16[3]>0x4000?0xff:0;
              s16+=4;
            }
            break;

          case sTF_Q8W8V8U8:
          case sTF_Q8W8V8U8+32:
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
*/
				  d+=bpr/4;
				  s+=oxs*2;
			  }
        mst->UnlockRect(level);

        if(xs<=1 || ys<=1)
				  break;

        filter = (miptresh <= level);
        if(mipdir) filter = !filter;


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
            if(filter)
            {
					    d16[0] = (s16[0]+s16[4]+s16[oxs*4+0]+s16[oxs*4+4]+2)>>2;
					    d16[1] = (s16[1]+s16[5]+s16[oxs*4+1]+s16[oxs*4+5]+2)>>2;
					    d16[2] = (s16[2]+s16[6]+s16[oxs*4+2]+s16[oxs*4+6]+2)>>2;
					    d16[3] = (s16[3]+s16[7]+s16[oxs*4+3]+s16[oxs*4+7]+2)>>2;
            }
            else
            {
              d16[0] = s16[0];
					    d16[1] = s16[1];
					    d16[2] = s16[2];
					    d16[3] = s16[3];
            }
					  d16+=4;
					  s16+=8;
				  }
			  }

			  xs=xs>>1;
			  ys=ys>>1;
			  level++;
		  }
    }
	  delete[] data;
	}
}

#endif

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
static sU32 MouseX;
static sU32 MouseY;
static sU32 MouseZ;
//static sU32 MouseButtons;
//static sU32 MouseButtonsSave;

/****************************************************************************/
/****************************************************************************/

void sSystem_::AddAKey(sU32 *Scans,sInt ascii)
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

sBool sSystem_::InitDI()
{
  HRESULT hr;
  DIPROPDWORD  prop; 
  sU32 Scans[256];
  sInt i;
  static dkeys[][2] = 
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
    { DIK_APPS    ,sKEY_APPPOPUP    },

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

  hr = DXIKey->SetCooperativeLevel((HWND)Screen[0].Window,DISCL_FOREGROUND|DISCL_NONEXCLUSIVE);
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

void sSystem_::ExitDI()
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

void sSystem_::PollDI()
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
        */
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
        }
      }
    }
  }
} 

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Misc Input/Timing stuff                                            ***/
/***                                                                      ***/
/****************************************************************************/

#if !sCOMMANDLINE
void sSystem_::GetInput(sInt id,sInputData &data)
{
#if sINTRO
  POINT pt;
  static sInt x,y;
  GetCursorPos(&pt);
  x += pt.x-400;
  y += pt.y-300;
  data.Analog[0]=x;
  data.Analog[1]=y;
  SetCursorPos(400,300);
#else
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
#endif
}
#endif

sInt sSystem_::GetTime()
{
  return timeGetTime()-WStartTime;
}

/****************************************************************************/
/****************************************************************************/

#if !sINTRO

sU32 sSystem_::GetKeyboardShiftState()
{
  return (KeyQual&0x7ffe0000);
}

/****************************************************************************/

void sSystem_::GetKeyName(sChar *buffer,sInt size,sU32 key)
{
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
}

/****************************************************************************/

sBool sSystem_::GetAbortKey()
{
  if(GetAsyncKeyState(VK_PAUSE)&1)
    WAbortKeyFlag = 1;
  return WAbortKeyFlag;
}

void sSystem_::ClearAbortKey()
{
  GetAbortKey();
  WAbortKeyFlag = 0;
}

/****************************************************************************/

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
  PerfLast.Time = (time-sPerfFrame)*sPerfKalibFactor;
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

void sSystem_::GetPerf(sPerfInfo &info,sInt mode)
{
  switch(mode)
  {
  case sPIM_LAST:
    info = PerfLast;
    break;
  case sPIM_BEGIN:
    info = PerfThis;
    info.Time = PerfLast.Time;
    info.TimeFiltered = PerfLast.TimeFiltered;
    break;
  case sPIM_END:
    info.Line     = PerfThis.Line     - info.Line;
    info.Triangle = PerfThis.Triangle - info.Triangle;
    info.Vertex   = PerfThis.Vertex   - info.Vertex;
    info.Material = PerfThis.Material - info.Material;
    info.Batches  = PerfThis.Batches  - info.Batches;
    info.Time = PerfLast.Time;
    info.TimeFiltered = PerfLast.TimeFiltered;
    break;
  case sPIM_PAUSE:
    info.Line     -= PerfThis.Line;
    info.Triangle -= PerfThis.Triangle;
    info.Vertex   -= PerfThis.Vertex;
    info.Material -= PerfThis.Material;
    info.Batches  -= PerfThis.Batches;
    break;
  case sPIM_CONTINUE:
    info.Line     += PerfThis.Line;
    info.Triangle += PerfThis.Triangle;
    info.Vertex   += PerfThis.Vertex;
    info.Material += PerfThis.Material;
    info.Batches  += PerfThis.Batches;
    break;
  default:
    sFatal("sSystem_::GetPerf() unknown mode");
    break;
  }
}

/****************************************************************************/

sBool sSystem_::GetWinMouse(sInt &x,sInt &y)
{
  x = WMouseX;
  y = WMouseY;
  return sTRUE;
}

void sSystem_::SetWinMouse(sInt x,sInt y)
{
  SetCursorPos(x,y);
}

void sSystem_::SetWinTitle(sChar *name)
{
  SetWindowText((HWND)Screen[0].Window,name);
}

void sSystem_::HideWinMouse(sBool hide)
{
  if(hide)
    while(ShowCursor(0)>0);
  else
    ShowCursor(1);
}

/****************************************************************************/

#endif

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Sound                                                              ***/
/***                                                                      ***/
/****************************************************************************/

#if sUSE_DIRECTSOUND
#if sINTRO
static const GUID IID_IDirectSoundBuffer8 = { 0x6825a449, 0x7524, 0x4d82, 0x92, 0x0f, 0x50, 0xe3, 0x6a, 0xb3, 0xab, 0x1e };
#endif

sBool sSystem_::InitDS()
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

void sSystem_::ExitDS()
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

  SampleRemAll();

  if(DXSBuffer)
    DXSBuffer->Release();
  if(DXSPrimary)
    DXSPrimary->Release();
  if(DXS)
    DXS->Release();
}

/****************************************************************************/

void sSystem_::MarkDS()
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
    if(delta>0 /*&& delta<0x10000*/)                           // 0x10000 = 3680ms = 2.7 fps
      DXSLastTotalSample = DXSWriteSample + delta/4;
  }
  LeaveCriticalSection(&DXSLock);
//  sDPrintF("%08x:%08x=%08x+%d\n",GetTime(),DXSLastTotalSample,DXSWriteSample,delta);

//  DXSLastTotalSample-=DXSSAMPLES;
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
#if !sINTRO
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
#endif
/****************************************************************************/

sInt sSystem_::GetCurrentSample()
{
#if sUSE_DIRECTSOUND
  return DXSLastTotalSample-DXSSAMPLES;//DXSLastFrameTime;
#else
  return 0;
#endif
}

/****************************************************************************/
/***                                                                      ***/
/***   Sample Player                                                      ***/
/***                                                                      ***/
/****************************************************************************/

#if sUSE_DIRECTSOUND
sInt sSystem_::SampleAdd(sS16 *data,sInt size,sInt buffers,sInt handle)
{
  sInt i,j;
  IDirectSoundBuffer *dsb;
  DSBUFFERDESC dsbd;
  WAVEFORMATEX format;
  HRESULT hr;
  DWORD len;
  LPVOID ptr;

  sVERIFY(buffers>=1);
  if(handle==sINVALID)
  {
    for(i=0;i<sMAXSAMPLEHANDLE;i++)
    {
      if(Sample[i].Count==0)
      {
        handle = i;
        break;
      }
    }
  }
  else
  {
    sVERIFY(handle>=0 && handle<sMAXSAMPLEHANDLE);
    if(Sample[handle].Count!=0)
      handle = sINVALID;
  }

  if(handle!=sINVALID)
  {
    WINZERO(format);
    WINSET(dsbd);
    format.wFormatTag = WAVE_FORMAT_PCM; 
    format.nChannels = 2; 
    format.nSamplesPerSec = 44100; 
    format.nBlockAlign = 4; 
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign; 
    format.wBitsPerSample = 16; 
    dsbd.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY;
    dsbd.dwBufferBytes = size*4;
    dsbd.lpwfxFormat = &format;

    hr = DXS->CreateSoundBuffer(&dsbd,&dsb,0);
    if(FAILED(hr)) return sINVALID;

    hr = dsb->Lock(0,0,&ptr,&len,0,0,DSBLOCK_ENTIREBUFFER);
    if(FAILED(hr)) return sINVALID;
    sVERIFY(len==size*4);

    sCopyMem(ptr,data,size*4);

    hr = dsb->Unlock(ptr,len,0,0);
    if(FAILED(hr)) return sINVALID;

    Sample[handle].Buffers[0] = dsb;
    for(j=1;j<buffers;j++)
    {
      hr = DXS->DuplicateSoundBuffer(dsb,&Sample[handle].Buffers[j]);
      if(FAILED(hr)) return sINVALID;
    }
    Sample[handle].LRU = 0;
    Sample[handle].Count = buffers;
  }
  return handle;
}


void sSystem_::SampleRem(sInt handle)
{
  sInt i;
  sVERIFY(handle>=0 && handle<sMAXSAMPLEHANDLE);
  for(i=0;i<Sample[handle].Count;i++)
    Sample[handle].Buffers[i]->Release();
  Sample[handle].Count = 0;
  Sample[handle].LRU = 0;
}

void sSystem_::SampleRemAll()
{
  sInt i;
  for(i=0;i<sMAXSAMPLEHANDLE;i++)
    SampleRem(i);
}

void sSystem_::SamplePlay(sInt handle,sInt volume,sInt pan,sInt freq)
{
  struct sSample *sam;
  HRESULT hr;

  sVERIFY(handle>=0 && handle<sMAXSAMPLEHANDLE);
  sam = &Sample[handle];
  if(sam->Count>0)
  {
    sam->LRU = (sam->LRU+1)%sam->Count;
    sam->Buffers[sam->LRU]->Stop();
    sam->Buffers[sam->LRU]->SetCurrentPosition(0);
    sam->Buffers[sam->LRU]->SetVolume(-volume);
    hr = sam->Buffers[sam->LRU]->Play(0,0,0);
  }
}
#endif

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
    sDPrintF("*** Loading <%s> failed\n",name);
  }
  else
  {
    sDPrintF("*** Loading <%s> (%d bytes)\n",name,size);
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

  if(result)
    sDPrintF("*** Saving <%s> (%d bytes)\n",name,size);
  else
    sDPrintF("*** Saving <%s> failed\n",name);

  return result;
}

/****************************************************************************/
/****************************************************************************/

#define sMAX_PATHNAME 4096
sDirEntry *sSystem_::LoadDir(sChar *name)
{
  static sChar buffer[sMAX_PATHNAME];
  HANDLE handle;
  sInt len;
  sInt i,j;
  WIN32_FIND_DATA dir;
  sDirEntry *de,*nde;
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
    max = 256;
    i=0;
    de = new sDirEntry[max];
    do
    {
      if(i == max-1)
      {
        max *= 2;
        nde = new sDirEntry[max];
        sCopyMem(nde,de,sizeof(sDirEntry)*i);
        delete[] de;
        de = nde;
      }

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
    while(FindNextFile(handle,&dir));
    FindClose(handle);
    sSetMem(&de[i],0,sizeof(sDirEntry));

    max = i;

    for(i=0;i<max-1;i++)
      for(j=i+1;j<max;j++)
        if(de[i].IsDir<de[j].IsDir || (de[i].IsDir==de[j].IsDir && sCmpStringI(de[i].Name,de[j].Name)>0))
          sSwap(de[i],de[j]);
  }

  return de;
}

/****************************************************************************/

sBool sSystem_::MakeDir(sChar *name)
{
  return CreateDirectory(name,0) != 0;
}

/****************************************************************************/

sBool sSystem_::CheckDir(sChar *name)
{
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
}

sBool sSystem_::CheckFile(sChar *name)
{
  HANDLE handle;

  handle = CreateFile(name,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
  if(handle != INVALID_HANDLE_VALUE)
  {
    CloseHandle(handle);
    return sTRUE;
  }
  return sFALSE;
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
}

/****************************************************************************/

sU32 sSystem_::GetDriveMask()
{
  return GetLogicalDrives();
}

/****************************************************************************/
/****************************************************************************/

#define HIMETRIC_PER_INCH         2540
#define MAP_PIX_TO_LOGHIM(x,ppli) ((HIMETRIC_PER_INCH*(x)+((ppli)>>1))/(ppli))
#define MAP_LOGHIM_TO_PIX(x,ppli) (((ppli)*(x)+HIMETRIC_PER_INCH/2)/HIMETRIC_PER_INCH)

class mystream : public IStream
{
};

sBitmap *sSystem_::LoadBitmap(sU8 *data,sInt size)
{
  HDC screendc;
  HDC hdc;
  HDC hdc2,hdcold;
  HBITMAP hbm,hbm2;
  BITMAPINFO bmi;
  sInt i;
  HRESULT hr;
  IPicture *pic;
  IStream *str;
  DWORD dummy;
  POINT wpl,wpp;
  sInt xs,ys;
  sBitmap *bm;

  union _ULARGE_INTEGER usize;
  union _LARGE_INTEGER useek;
  union _ULARGE_INTEGER udummy;

  bm = 0;
  screendc = GetDC(0);
  hr = CreateStreamOnHGlobal(0,TRUE,&str);
  if(!FAILED(hr))
  {
    usize.QuadPart = size;
    str->SetSize(usize);
    str->Write(data,size,&dummy);
    useek.QuadPart = 0;
    str->Seek(useek,STREAM_SEEK_SET,&udummy);
    hr = OleLoadPicture(str,size,TRUE,IID_IPicture,(void**)&pic);

    if(!FAILED(hr))
    {
      pic->get_Width(&wpl.x); 
      pic->get_Height(&wpl.y); 
      wpp = wpl;
  	  sInt px=GetDeviceCaps(screendc,LOGPIXELSX);
  	  sInt py=GetDeviceCaps(screendc,LOGPIXELSY);
  	  wpp.x=MAP_LOGHIM_TO_PIX(wpl.x,px);
  	  wpp.y=MAP_LOGHIM_TO_PIX(-wpl.y,py);

      hdc = CreateCompatibleDC(screendc);
      hdc2 = CreateCompatibleDC(screendc);

      xs = sMakePower2(wpp.x);
      ys = sMakePower2(-wpp.y);

      bm = new sBitmap;
      bm->Init(xs,ys);
      hbm = CreateCompatibleBitmap(screendc,xs,ys);
      SelectObject(hdc,hbm);

      sSetMem4(bm->Data,0xffff0000,bm->XSize*bm->YSize);
      bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
      bmi.bmiHeader.biWidth = bm->XSize;
      bmi.bmiHeader.biHeight = -bm->YSize;
      bmi.bmiHeader.biPlanes = 1;
      bmi.bmiHeader.biBitCount = 32;
      bmi.bmiHeader.biCompression = BI_RGB;

      hr = pic->SelectPicture(hdc2,&hdcold,(unsigned int *)&hbm2);
      if(!FAILED(hr))
      {
        SetMapMode(hdc2,MM_TEXT);
        StretchBlt(hdc,0,0,bm->XSize,bm->YSize,hdc2,0,0,wpp.x,-wpp.y,SRCCOPY);
        GetDIBits(hdc,hbm,0,bm->YSize,bm->Data,&bmi,DIB_RGB_COLORS);
        pic->SelectPicture(hdcold,0,(unsigned int *)&hbm2);
      }
      pic->Release();

      DeleteDC(hdc2);
      DeleteDC(hdc);
      DeleteObject(hbm);
      for(i=0;i<bm->XSize*bm->YSize;i++)
        bm->Data[i] |= 0xff000000;
    }
    str->Release();
  }
  ReleaseDC(0,screendc);

  return bm;
}

/****************************************************************************/
/***                                                                      ***/
/***   Host Font Interface                                                ***/
/***                                                                      ***/
/****************************************************************************/

sInt sSystem_::FontBegin(sInt pagex,sInt pagey,sChar *name,sInt xs,sInt ys,sInt style)
{
  LOGFONT lf;
  TEXTMETRIC met;

  FontPageX = pagex;
  FontPageY = pagey;
  FontMem = new sU32[pagex*pagey];
  sSetMem(FontMem,0,pagex*pagey*4);
  FontHBM = CreateCompatibleBitmap(GDIScreenDC,pagex,pagey);
  SelectObject(GDIDC,FontHBM);

  FontBMI.bmiHeader.biSize = sizeof(FontBMI.bmiHeader);
  FontBMI.bmiHeader.biWidth = pagex;
  FontBMI.bmiHeader.biHeight = -pagey;
  FontBMI.bmiHeader.biPlanes = 1;
  FontBMI.bmiHeader.biBitCount = 32;
  FontBMI.bmiHeader.biCompression = BI_RGB;
  SetDIBits(GDIDC,FontHBM,0,pagey,FontMem,&FontBMI,DIB_RGB_COLORS);


  if(name[0]==0) name = "arial";

  lf.lfHeight         = -ys; 
  lf.lfWidth          = xs; 
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

  FontHandle = CreateFontIndirect(&lf);
  SetBkMode(GDIDC,TRANSPARENT);
  SelectObject(GDIScreenDC,FontHandle);    // win98: first select font into screen dc
  SelectObject(GDIDC,FontHandle);
  SelectObject(GDIDC,FontHBM);
  SetTextColor(GDIDC,0xffffff);

  GetTextMetrics(GDIDC,&met);

  return met.tmHeight;
//  Advance = met.tmHeight+met.tmExternalLeading;
//  Baseline = met.tmHeight-met.tmDescent;
}

sInt sSystem_::FontWidth(sChar *string,sInt len)
{
  SIZE p;

  if(len==-1)
  {
    len++;
    while(string[len]) len++;
  }
  GetTextExtentPoint32(GDIDC,string,len,&p);
  return p.cx;
}

void sSystem_::FontPrint(sInt x,sInt y,sChar *string,sInt len)
{
  if(len==-1)
  {
    len++;
    while(string[len]) len++;
  }
  ExtTextOut(GDIDC,x,y,0,0,string,len,0);
}

sU32 *sSystem_::FontEnd()
{
  FontBMI.bmiHeader.biSize = sizeof(FontBMI.bmiHeader);
  FontBMI.bmiHeader.biWidth = FontPageX;
  FontBMI.bmiHeader.biHeight = -FontPageY;
  FontBMI.bmiHeader.biPlanes = 1;
  FontBMI.bmiHeader.biBitCount = 32;
  FontBMI.bmiHeader.biCompression = BI_RGB;
  GetDIBits(GDIDC,FontHBM,0,FontPageY,FontMem,&FontBMI,DIB_RGB_COLORS);
  SelectObject(GDIDC,GDIHBM);
  DeleteObject(FontHBM);

  return FontMem;
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Helper Structures                                                  ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#if sLINK_ENGINE

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

void sMaterialInfo::Init()
{
  sInt i;

  sSetMem(this,0,sizeof(*this));

  BaseFlags = smBF_ZON;
  Program = 1;
  Size = 1.0f;
  Aspect = 1.0f;
  Handle = sINVALID;

  for(i=0;i<4;i++)
  {
    TFlags[i] = smTF_MIPMAPS; 
    Color[i] = ~0;
    TScale[i] = 0.0f;
    Tex[i] = sINVALID;
  }
}

void sMaterialEnv::Init()
{
  sSetMem(this,0,sizeof(*this));

  LightRange = 5.0f;
  LightAmplify = 1.0f;
  LightDiffuse = ~0;
  LightAmbient = 0xff000000;
  ModelSpace.Init();
  CameraSpace.Init();
  NearClip = 0.125f;
  FarClip = 4096.0f;
  ZoomX = 1.0f;
  ZoomY = 1.0f;
  CenterX = 0.0f;
  CenterY = 0.0f;
  FogStart = 0;
  FogEnd = FarClip;
  FogColor = 0xff808080;
}

void sMaterialEnv::MakeProjectionMatrix(sMatrix &pmat) const
{
  sF32 q;

  switch(Orthogonal)
  {
  case sMEO_PIXELS: // 0 .. screen_max
    pmat.i.Init(2.0f/sSystem->ViewportX,0                       ,0 ,0);
    pmat.j.Init(0                      ,-2.0f/sSystem->ViewportY,0 ,0);
    pmat.k.Init(0                      ,0                       ,1 ,0);
    pmat.l.Init(-1                     ,1                       ,0 ,1);
    break;
  case sMEO_NORMALISED: // -1 .. 1
    pmat.i.Init(ZoomX                  ,0                       ,0 ,0);
    pmat.j.Init(0                      ,ZoomY                   ,0 ,0);
    pmat.k.Init(0                      ,0                       ,1.0f/FarClip,0);
    pmat.l.Init(0                      ,0                       ,0 ,1);
    break;
  case sMEO_PERSPECTIVE:
    //q = FarClip/(FarClip-NearClip);
    q = 1.0f;
    pmat.i.Init(ZoomX         ,0               , 0              ,0);
    pmat.j.Init(0             ,ZoomY           , 0              ,0);
    pmat.k.Init(CenterX       ,CenterY         , q              ,1);
    pmat.l.Init(0             ,0               ,-q*NearClip     ,0);
    break;
  }
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

void sMaterialInfo::DefaultCombiner(sBool *tex)
{
  sBool mytex[4];
  sInt i;
  sU32 light;

// find out which textures are set

  if(tex==0)
  {
    tex = mytex;
    for(i=0;i<4;i++)
      tex[i] = (Tex[i]!=sINVALID);
  }

// prepare

  for(i=0;i<sMCS_MAX;i++)
    Combiner[i] = 0;

  light = LightFlags&sMLF_MODEMASK;

// set textures

  if(tex[0] && !(SpecialFlags&sMSF_BUMPDETAIL))
    Combiner[sMCS_TEX0] = sMCOA_MUL;
  if(tex[1] && light<sMLF_BUMP)
    Combiner[sMCS_TEX1] = sMCOA_MUL;
  if(tex[2])
    Combiner[sMCS_TEX2] = sMCOA_ADD;
  if(tex[3])
    Combiner[sMCS_TEX3] = sMCOA_ADD;

// set light

  if(light)
  {
    Combiner[sMCS_LIGHT] = sMCOA_MUL;
    Combiner[sMCS_COLOR0] = sMCOA_MUL;
    Combiner[sMCS_COLOR1] = sMCOA_ADD;
  }

// set first combiner to mul

  for(i=0;i<sMCS_MAX;i++)
  {
    if(Combiner[i])
    {
      Combiner[i] = sMCOA_SET;
      goto set;
    }
  }
  Combiner[sMCS_COLOR0] = sMCOA_SET;  // no texture, set color
set:;
}

void sMaterialInfo::AddRefElements()
{
  sInt i;
  for(i=0;i<4;i++)
    sSystem->AddRefTexture(Tex[i]);
  sSystem->MtrlAddRef(Handle);
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Viewport                                                           ***/
/***                                                                      ***/
/****************************************************************************/

#if sLINK_ENGINE

void sViewport::Init(sInt screen)
{
  Screen = screen;
  RenderTarget = -1;
  Window.Init(0,0,sSystem->Screen[screen].XSize,sSystem->Screen[screen].YSize);
  ClearFlags = sVCF_ALL;
  ClearColor.Color = 0xff00000;
}

/****************************************************************************/

void sViewport::InitTex(sInt handle)
{
  Screen = -1;
  RenderTarget = handle;
  ClearFlags = sVCF_ALL;
  ClearColor.Color = 0xff000000;
  Window.Init();
}

#endif

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Host Font                                                          ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#if sLINK_ENGINE

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

sInt sHostFont::GetWidth(sChar *text,sInt len)
{
  SIZE p;

  if(len==-1)
    len = sGetStringLen(text);
  GetTextExtentPoint32(GDIDC,text,len,&p);
  return p.cx;
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
  SelectObject(GDIScreenDC,prv->font);    // win98: first select font into screen dc
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

#endif

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Shader                                                             ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#if sLINK_ENGINE
#if !sINTRO
static void PrintReg(sU32 reg,sInt version)
{
  sInt type,num;
  const sChar *name;
  static const sChar *psnames[32] = 
  {
    "r","v","c","t",
    "?","?","?","?",
    "oC","oDepth","s","?",
    "?","?","?","?",

    "?","?","?","?",
    "?","?","?","?",
    "?","?","?","?",
    "?","?","?","?",
  };
  static const sChar *vsnames[32] = 
  {
    "r","v","c","a",
    "?","oD","oT","?",
    "?","?","?","?",
    "?","?","?","?",

    "?","?","?","?",
    "?","?","?","?",
    "?","?","?","?",
    "?","?","?","?",
  };
  static const sChar *rastout[4] = 
  {
    "oPos","oFog","oPsize","???"
  };


  type = ((reg>>28)&7) + ((reg>>11)&3)*8;
  num = reg&0x7ff;
  if(version<0xffff0000)
  {
    name = vsnames[type];
    if(type==4)
      name = rastout[sMin(num,3)];
  }
  else
  {
    name = psnames[type];
  }
  sDPrintF("%s%d",name,num);
}
static void PrintShader(sU32 *data,sInt size)
{
  static const sChar *opcodes[0x100] = 
  {
    "nop"   ,"mov"   ,"add"   ,"sub"     ,"mad"   ,"mul"   ,"rcp"   ,"rsq"   ,
    "dp3"   ,"dp4"   ,"min"   ,"max"     ,"slt"   ,"sge"   ,"exp"   ,"log"   ,
    "lit"   ,"dst"   ,"lrp"   ,"frc"     ,"m4x4"  ,"m4x3"  ,"m3x4"  ,"m3x3"  ,
    "m3x2"  ,"call"  ,"callnz","loop"    ,"ret"   ,"endloop","label","dcl"   ,

    "pow"   ,"crs"   ,"sgn"   ,"abs"     ,"nrm"   ,"sincos","rep"   ,"endrep",
    "if"    ,"ifc"   ,"else"  ,"endif"   ,"break" ,"breakc","mova"  ,"defb"  ,
    "defi"  ,0       ,0       ,0         ,0       ,0       ,0       ,0       ,
    0       ,0       ,0       ,0         ,0       ,0       ,0       ,0       ,

    "texcoord","texkill","tex","texbem"  ,"texbeml","texreg2ar","texreg2bg","texm3x2pad",
    "texm3x2tex","texm3x3pad","texm3x3tex",0,"texm3x3spec","texm3x3vspec","expp","logp",
    "cnd"   ,"def"   ,"texreg2rgb","texdp3tex","texm3x2depth","texdp3","texm3x3","texdepth",
    "cmp"   ,"bem"   ,"dp2add","dsx"     ,"dsy"   ,"texldd"  ,"setp"  ,"texldl",
    
    "breaklp",0      ,0       ,0         ,0       ,0       ,0       ,0       ,
    0       ,0       ,0       ,0         ,0       ,0       ,0       ,0       ,
    0       ,0       ,0       ,0         ,0       ,0       ,0       ,0       ,
    0       ,0       ,0       ,0         ,0       ,0       ,0       ,0       ,


    0       ,0       ,0       ,0         ,0       ,0       ,0       ,0       ,
    0       ,0       ,0       ,0         ,0       ,0       ,0       ,0       ,
    0       ,0       ,0       ,0         ,0       ,0       ,0       ,0       ,
    0       ,0       ,0       ,0         ,0       ,0       ,0       ,0       ,

    0       ,0       ,0       ,0         ,0       ,0       ,0       ,0       ,
    0       ,0       ,0       ,0         ,0       ,0       ,0       ,0       ,
    0       ,0       ,0       ,0         ,0       ,0       ,0       ,0       ,
    0       ,0       ,0       ,0         ,0       ,0       ,0       ,0       ,

    0       ,0       ,0       ,0         ,0       ,0       ,0       ,0       ,
    0       ,0       ,0       ,0         ,0       ,0       ,0       ,0       ,
    0       ,0       ,0       ,0         ,0       ,0       ,0       ,0       ,
    0       ,0       ,0       ,0         ,0       ,0       ,0       ,0       ,

    0       ,0       ,0       ,0         ,0       ,0       ,0       ,0       ,
    0       ,0       ,0       ,0         ,0       ,0       ,0       ,0       ,
    0       ,0       ,0       ,0         ,0       ,0       ,0       ,0       ,
    "vs"    ,"ps"    ,0       ,0         ,0       ,"phase" ,"comment","end"  ,
  };

  static const sU8 outin[0x100]=
  {
    0x00,0x11,0x12,0x12  ,0x13,0x12,0x11,0x11,
    0x12,0x12,0x12,0x12  ,0x12,0x12,0x11,0x11,
    0x11,0x12,0x13,0x11  ,0x12,0x12,0x12,0x12,    // 0x80 = label
    0x12,0x80,0x80,0x80  ,0x00,0x00,0x80,0x90,    // 0x90 = dcl

    0x12,0x12,0x11,0x11  ,0x11,0x20,0x00,0x00,
    0x01,0x02,0x01,0x01  ,0x00,0x02,0x11,0x90,
    0x90,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,

    0x10,0x01,0x10,0x11  ,0x11,0x11,0x11,0x12,
    0x12,0x12,0x12,0x00  ,0x13,0x12,0x11,0x11,
    0x13,0x12,0x13,0x11  ,0x11,0x14,0x12,0x12,
    0x13,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
    
    0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,


    0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,

    0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,

    0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,

    0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
    0xa0,0xa0,0x00,0x00  ,0x00,0x00,0x00,0x00,    // 0xa0 = version (faked!)
  };
  static const sChar *usage[16] =
  {
    "position","blendweights","blendindices","normal",
    "psize","texcoord","tangent","binormal",
    "tessfactor","positiont","color","fog",
    "depth","sample","???","???",
  };
  static const sChar *texkind[8] = 
  {
    "(reg)","???","2d","cube","volume","???","???","???"
  };
  static const sChar rz[] = "xyzw";
  static const sChar *sourcepre[16] = 
  {
    ""    ,"-"    ,"b_"   ,"-b_"  ,
    "~"   ,"~-"   ,"?"    ,"2*"   ,
    "-2*" ,"dz_"  ,"dw_"  ,"abs_" ,
    "-abs_","!"   ,"?"    ,"?"
  };

  sInt code,in,out,dcl,label;
  sInt i,len,j;
  sInt version;
  sU32 val;
  sInt komma;
  sInt line;

  line = 1;
  while(size)
  {
    val = *data;
    switch(val&0xffff0000)
    {
    case 0xffff0000:
      code = 0xf9; 
      version = val;
      break;
    case 0xfffe0000:
      code = 0xf8; 
      version = val;
      break;
    default:
      code = val&0x00ff;
      break;
    }
    in = out = dcl = label = komma = 0;
    switch(outin[code]&0xf0)
    {
    case 0x80:
      label = 1;
      break;
    case 0x90:
      dcl = 1;
      out = 1;
      break;
    case 0xa0:
      break;
    default:
      in = outin[code]&0x0f;
      out = (outin[code]&0x70)>>4;
      break;
    }
    if(code==0x42 && version==0xffff0200)
      in = 2, out =1;

    len = 1+in+out+dcl+label;

    sDPrintF("%04d  ",line++);
    for(i=0;i<5;i++)
    {
      if(i<len)
        sDPrintF("%08x ",data[i]);
      else
        sDPrintF("         ");
    }
    data++; size--;

    if((val & 0x40000000) && ((val >> 16) != 0xffff))
      sDPrintF("+");
    else
      sDPrintF(" ");
    sDPrintF("%-08s",opcodes[code]);
    if((outin[code]&0xf0)==0xa0)
      sDPrintF("%04x",version);

    for(i=0;i<label;i++)
    {
      if(komma) sDPrintF(","); komma=1;
      sDPrintF("label");
      data++; size--;
    }
    for(i=0;i<dcl;i++)
    {
      if(komma) sDPrintF(","); komma=1;
      val = *data++; size--;
      if(version<0xffff0000) 
        sDPrintF("%s%d",usage[val&0x0f],(val&0xf0000)>>16);
      else
        sDPrintF("%s",texkind[(val>>27)&7]);
    }
    for(i=0;i<out;i++)
    {
      if(komma) sDPrintF(","); komma=1;
      val = *data++; size--;
      PrintReg(val,version);
      if(val&0x00100000) sDPrintF("_sat");
      if(val&0x00200000) sDPrintF("_pp");
      if(val&0x00400000) sDPrintF("_msamp");
      j = (val&0xf0000)>>16;
      if(j!=15)
      {
        sDPrintF(".");
        if(j&1) sDPrintF("x");
        if(j&2) sDPrintF("y");
        if(j&4) sDPrintF("z");
        if(j&8) sDPrintF("w");
      }
    }
    for(i=0;i<in;i++)
    {
      if(komma) sDPrintF(","); komma=1;
      val = *data++; size--;
      sDPrintF("%s",sourcepre[(val>>24)&15]);
      PrintReg(val,version);
      if(val&0x00002000) sDPrintF("[a]");
      j = (val&0x00ff0000)>>16;
      if(j!=0xe4)
        sDPrintF(".%c%c%c%c",rz[(j>>0)&3],rz[(j>>2)&3],rz[(j>>4)&3],rz[(j>>6)&3]);
    }
    sDPrintF("\n");
  }
  sDPrintF(".\n");
}
#endif
/****************************************************************************/

#define CONST_HALF  (XS_X|X_C|6)
#define CONST_HMH   (XSALL|X_C|6)
#define CONST_ZERO  (XS_Z|X_C|6)
#define CONST_ONE   (XS_W|X_C|6)

static void cothread(sU32 *&ps,sU32 *data,sU32 co)
{
  sInt i,max;
  max = *data++;
  *ps++ = *data++|co;
  for(i=1;i<max;i++)
    *ps++ = *data++;
}

static void vs_nrm(sU32 *&p,sInt dreg,sU32 s)
{
  sU32 *vs;

  vs = p;

  *vs++ = XO_DP3;
  *vs++ = XD_X|X_R|dreg;
  *vs++ = s;
  *vs++ = s;

  *vs++ = XO_RSQ;
  *vs++ = XD_X|X_R|dreg;
  *vs++ = XS_X|X_R|dreg;

  *vs++ = XO_MUL;
  *vs++ = XD_XYZ|X_R|dreg;
  *vs++ = XS_X|X_R|dreg;
  *vs++ = s;

  p = vs;
}

#if !sINTRO

sInt sSystem_::MtrlAdd(sMaterialInfo *pinfo)
{
  enum RFileIndex                 // register file indices (which Tx register?)
  {
    RTex0=0,                      // uv transformed for Texture 0
    RTex1,                        // uv transformed for Texture 1
    RTex2,                        // uv transformed for Texture 2
    RTex3,                        // uv transformed for Texture 3
    REyeNormal,                   // normal in eye space
    REyePos,                      // position in eye space
    REyeBinormal,
    REyeTangent,
    RTexPos,                      // texture space position
    RTexLight,                    // texture space light
    RHalfway,                     // halfway vector, either eye or tex space
    RMAX                          // count of possible registers
  };
  static const sU32 psatoc[16] =
  {
    0,1<<sMCS_LIGHT,(1<<sMCS_SPECULAR)|(1<<sMCS_LIGHT),1<<sMCS_LIGHT,
    1<<sMCS_TEX0,1<<sMCS_TEX1,1<<sMCS_TEX2,1<<sMCS_TEX3,
    1<<sMCS_COLOR0,1<<sMCS_COLOR1,1<<sMCS_COLOR2,1<<sMCS_COLOR3,
    0,0,0,0,
  };
  static const sU32 psttoc[4] = 
  {
    1<<sMCS_TEX0,1<<sMCS_TEX1,1<<sMCS_TEX2,1<<sMCS_TEX3,
  };
  static const sU32 vsswizzles[4] = 
  {
    XS_X,XS_Y,XS_Z,XS_W,
  };

  sMaterial *mtrl;
  sMaterialInfo infocopy;
  sMaterialInfo info;
  sU32 *data;
  sInt mid;
  sInt i,j;
  sInt texuv1;
  sInt tex,sam,val;
  sF32 f;
  sU32 flags;
  static sU32 psb[512];
  static sU32 vsb[512];
  sU32 *ps;
  sU32 *vs;
  HRESULT hr;
  sInt f0,f1;
  sInt combinermask;
  sInt error;
#if !sINTRO
  sInt op;
  sInt normalreg;
#endif
  sInt ps00_combine;
  sU32 lightmode;
  sInt decltspace;

  sInt RFile[RMAX];               // the register file. -1 = unused
  sInt TFile[4];                  // sMCS_TEX -> d3d_texture
  sInt RCount;                    // count of T-Registers
  sInt TCount;

// modify material

  info = *pinfo;
  ps00_combine = sMCOA_NOP;
  lightmode = (info.LightFlags & sMLF_MODEMASK);
  switch(info.ShaderLevel)
  {
#if !sINTRO
  case sPS_00:
    switch(lightmode)
    {
    case sMLF_BUMP:
    case sMLF_BUMPENV:
      info.Tex[1] = sINVALID;
      info.LightFlags = (info.LightFlags&~sMLF_MODEMASK)|sMLF_POINT;
      break;
    }
    if(info.SpecialFlags & sMSF_BUMPDETAIL)
    {
      info.Tex[0] = sINVALID;
      info.SpecialFlags &= ~sMSF_BUMPDETAIL;
    }

    if(info.Tex[0]==sINVALID)
    {
      info.Tex[1] = info.Tex[2] = info.Tex[3] = sINVALID;
    }
    else
    {
      if(info.Tex[2]!=sINVALID)
      {
        ps00_combine = info.Combiner[sMCS_TEX2];
        info.Tex[1] = info.Tex[3] = sINVALID;
      }
      else if(info.Tex[3]!=sINVALID)
      {
        ps00_combine = info.Combiner[sMCS_TEX3];
        info.Tex[1] = sINVALID;
      }
      else if(info.Tex[1]!=sINVALID)
      {
        ps00_combine = info.Combiner[sMCS_TEX1];
      }
    }
    break;
#endif
  case sPS_13:
  case sPS_14:
    switch(lightmode)
    {
    case sMLF_BUMPENV:
    case sMLF_BUMP:
      info.Tex[2] = sINVALID;
      info.LightFlags = (info.LightFlags&~sMLF_MODEMASK)|sMLF_BUMP;
      break;
    case sMLF_BUMPXSPEC:
      if(info.SpecPower < 8.5f)
        info.SpecPower = 8.0f;
      else if(info.SpecPower < 16.5f)
        info.SpecPower = 16.0f;
      else
        info.SpecPower = 32.0f;
      break;
    }
    if(info.SpecialFlags & sMSF_BUMPDETAIL)
    {
      info.Tex[0] = sINVALID;
      info.SpecialFlags &= ~sMSF_BUMPDETAIL;
    }
    break;
  case sPS_20:
    break;
  }
  lightmode = (info.LightFlags & sMLF_MODEMASK);

// find similar material

  infocopy = info;
  for(i=0;i<4;i++)
    infocopy.Tex[i] = infocopy.Tex[i]==sINVALID?sINVALID:sINVALID-1;
  sSetMem(infocopy.TScale,0,16*4); // TScale .. SRT2
  infocopy.Handle = 0;
  infocopy.Usage = 0;
  infocopy.Pass = 0;
  infocopy.Size = 0;
  infocopy.Aspect = 0;
  if(info.ShaderLevel == sPS_13 || info.ShaderLevel == sPS_14)
    infocopy.SpecPower = info.SpecPower;

  for(mid=0;mid<MAX_MATERIALS;mid++)
  {
    if(Materials[mid].RefCount>0)
    {
      if(sCmpMem(&Materials[mid].Info,&infocopy,sizeof(sMaterialInfo))==0)
      {
        Materials[mid].RefCount++;
        return mid;
      }
    }
  }

//*** find free material

  mid = sINVALID;
  mtrl = 0;
  for(mid=0;mid<MAX_MATERIALS;mid++)
  {
    if(Materials[mid].RefCount==0)
    {
      mtrl = &Materials[mid];
      break;
    }
  }
  if(!mtrl) return sINVALID;
  error = 0;

//*** initialize to zero

  mtrl->RefCount = 1;
  mtrl->Info = infocopy;
  mtrl->PS = 0;
  mtrl->VS = 0;
  mtrl->TexUse[0] = -1;
  mtrl->TexUse[1] = -1;
  mtrl->TexUse[2] = -1;
  mtrl->TexUse[3] = -1;

// initialize our state

  data = mtrl->States;
  vs = vsb;
  ps = psb;
  for(i=0;i<RMAX;i++) RFile[i] = -1;
  RCount = 0;
  TCount = 0;
  combinermask = 0;
  decltspace = 0;

//*** base flags

  flags = info.BaseFlags;
  f = 0;
  if(flags & smBF_ZBIASBACK) f=0.00005f;
  if(flags & smBF_ZBIASFORE) f=-0.00005f;

  *data++ = sD3DRS_ALPHATESTENABLE;             *data++ = (flags & smBF_ALPHATEST) ? 1 : 0;
  *data++ = sD3DRS_ALPHAFUNC;                   *data++ = (flags & smBF_ALPHATESTINV) ? sD3DCMP_LESSEQUAL : sD3DCMP_GREATER;
  *data++ = sD3DRS_ALPHAREF;                    *data++ = 128/*4*/;
  *data++ = sD3DRS_ZENABLE;                     *data++ = sD3DZB_TRUE;
  *data++ = sD3DRS_ZWRITEENABLE;                *data++ = (flags & smBF_ZWRITE) ? 1 : 0;
  *data++ = sD3DRS_ZFUNC;                       
  *data++ = (flags & smBF_ZREAD) ? 
    ((flags & smBF_SHADOWMASK)?sD3DCMP_LESS:
    sD3DCMP_LESSEQUAL) : sD3DCMP_ALWAYS;
  *data++ = sD3DRS_CULLMODE;                    *data++ = (flags & smBF_DOUBLESIDED) ? sD3DCULL_NONE : (flags & smBF_INVERTCULL?sD3DCULL_CW:sD3DCULL_CCW);
  *data++ = sD3DRS_COLORWRITEENABLE;            *data++ = (flags & smBF_ZONLY) ? 0 : 15;
  *data++ = sD3DRS_SLOPESCALEDEPTHBIAS;         *data++ = *(sU32 *)&f;
  *data++ = sD3DRS_DEPTHBIAS;                   *data++ = *(sU32 *)&f;
  *data++ = sD3DRS_FOGENABLE;                   *data++ = 0;
  *data++ = sD3DRS_CLIPPING;                    *data++ = 1;
  *data++ = sD3DTSS_TEXCOORDINDEX|sD3DTSS_0;    *data++ = 0;
  *data++ = sD3DTSS_TEXCOORDINDEX|sD3DTSS_1;    *data++ = 1;
  *data++ = sD3DTSS_TEXCOORDINDEX|sD3DTSS_2;    *data++ = 2;
  *data++ = sD3DTSS_TEXCOORDINDEX|sD3DTSS_3;    *data++ = 3;

  switch(flags&smBF_BLENDMASK)
  {
  case smBF_BLENDOFF:
    *data++ = sD3DRS_ALPHABLENDENABLE;  *data++ = 0;
    break;
  case smBF_BLENDADD:
    *data++ = sD3DRS_ALPHABLENDENABLE;  *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;          *data++ = sD3DBLEND_ONE;
    *data++ = sD3DRS_DESTBLEND;         *data++ = sD3DBLEND_ONE;
    *data++ = sD3DRS_BLENDOP;           *data++ = sD3DBLENDOP_ADD;
    break;
  case smBF_BLENDMUL:
    *data++ = sD3DRS_ALPHABLENDENABLE;  *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;          *data++ = sD3DBLEND_ZERO;
    *data++ = sD3DRS_DESTBLEND;         *data++ = sD3DBLEND_SRCCOLOR;
    *data++ = sD3DRS_BLENDOP;           *data++ = sD3DBLENDOP_ADD;
    break;
  case smBF_BLENDALPHA:
    *data++ = sD3DRS_ALPHABLENDENABLE;  *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;          *data++ = sD3DBLEND_SRCALPHA;
    *data++ = sD3DRS_DESTBLEND;         *data++ = sD3DBLEND_INVSRCALPHA;
    *data++ = sD3DRS_BLENDOP;           *data++ = sD3DBLENDOP_ADD;
    break;
  case smBF_BLENDSMOOTH:
    *data++ = sD3DRS_ALPHABLENDENABLE;  *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;          *data++ = sD3DBLEND_ONE;
    *data++ = sD3DRS_DESTBLEND;         *data++ = sD3DBLEND_INVSRCCOLOR;
    *data++ = sD3DRS_BLENDOP;           *data++ = sD3DBLENDOP_ADD;
    break;
  case smBF_BLENDSUB:
    *data++ = sD3DRS_ALPHABLENDENABLE;  *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;          *data++ = sD3DBLEND_ONE;
    *data++ = sD3DRS_DESTBLEND;         *data++ = sD3DBLEND_ONE;
    *data++ = sD3DRS_BLENDOP;           *data++ = sD3DBLENDOP_REVSUBTRACT;
    break;
  case smBF_BLENDMUL2:
    *data++ = sD3DRS_ALPHABLENDENABLE;  *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;          *data++ = sD3DBLEND_DESTCOLOR;
    *data++ = sD3DRS_DESTBLEND;         *data++ = sD3DBLEND_SRCCOLOR;
    *data++ = sD3DRS_BLENDOP;           *data++ = sD3DBLENDOP_ADD;
    break;
  case smBF_BLENDDESTA:
    *data++ = sD3DRS_ALPHABLENDENABLE;  *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;          *data++ = sD3DBLEND_DESTALPHA;
    *data++ = sD3DRS_DESTBLEND;         *data++ = sD3DBLEND_ONE;
    *data++ = sD3DRS_BLENDOP;           *data++ = sD3DBLENDOP_ADD;
    break;
  case smBF_BLENDADDSA:
    *data++ = sD3DRS_ALPHABLENDENABLE;  *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;          *data++ = sD3DBLEND_SRCALPHA;
    *data++ = sD3DRS_DESTBLEND;         *data++ = sD3DBLEND_ONE;
    *data++ = sD3DRS_BLENDOP;           *data++ = sD3DBLENDOP_ADD;
    break;
  }

  switch(flags & smBF_STENCILMASK)
  {
  default:
    *data++ = D3DRS_STENCILENABLE;      *data++ = 0;
    *data++ = D3DRS_TWOSIDEDSTENCILMODE; *data++ = 0;
    *data++ = D3DRS_STENCILFUNC;        *data++ = D3DCMP_ALWAYS;
    break;
  case smBF_STENCILINC:
  case smBF_STENCILINC|smBF_STENCILZFAIL:
    *data++ = D3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = D3DRS_TWOSIDEDSTENCILMODE; *data++ = 0;
    *data++ = D3DRS_STENCILFUNC;        *data++ = D3DCMP_ALWAYS;
    *data++ = D3DRS_STENCILFAIL;        *data++ = D3DSTENCILOP_KEEP;
    if(flags & smBF_STENCILZFAIL)
    {
      *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_INCR;
      *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_KEEP;
    }
    else
    {
      *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_KEEP;
      *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_INCR;
    }
    break;
  case smBF_STENCILDEC:
  case smBF_STENCILDEC|smBF_STENCILZFAIL:
    *data++ = D3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = D3DRS_TWOSIDEDSTENCILMODE; *data++ = 0;
    *data++ = D3DRS_STENCILFUNC;        *data++ = D3DCMP_ALWAYS;
    *data++ = D3DRS_STENCILFAIL;        *data++ = D3DSTENCILOP_KEEP;
    if(flags & smBF_STENCILZFAIL)
    {
      *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_DECR;
      *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_KEEP;
    }
    else
    {
      *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_KEEP;
      *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_DECR;
    }
    break;
  case smBF_STENCILTEST:
    *data++ = D3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = D3DRS_TWOSIDEDSTENCILMODE; *data++ = 0;
    *data++ = D3DRS_STENCILFUNC;        *data++ = D3DCMP_EQUAL;
    *data++ = D3DRS_STENCILFAIL;        *data++ = D3DSTENCILOP_KEEP;
    *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_KEEP;
    *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_KEEP;
    break;
  case smBF_STENCILCLR:
    *data++ = D3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = D3DRS_TWOSIDEDSTENCILMODE; *data++ = 0;
    *data++ = D3DRS_STENCILFUNC;        *data++ = D3DCMP_ALWAYS;
    *data++ = D3DRS_STENCILFAIL;        *data++ = D3DSTENCILOP_ZERO;
    *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_ZERO;
    *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_ZERO;
    break;
  case smBF_STENCILINDE:
  case smBF_STENCILINDE|smBF_STENCILZFAIL:
    *data++ = D3DRS_CULLMODE;           *data++ = D3DCULL_NONE;
    *data++ = D3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = D3DRS_TWOSIDEDSTENCILMODE; *data++ = 1;
    *data++ = D3DRS_STENCILFUNC;        *data++ = D3DCMP_ALWAYS;
    *data++ = D3DRS_CCW_STENCILFUNC;    *data++ = D3DCMP_ALWAYS;
    *data++ = D3DRS_STENCILFAIL;        *data++ = D3DSTENCILOP_KEEP;
    *data++ = D3DRS_CCW_STENCILFAIL;    *data++ = D3DSTENCILOP_KEEP;
    if(flags & smBF_STENCILZFAIL)
    {
      *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_DECR;
      *data++ = D3DRS_CCW_STENCILZFAIL;   *data++ = D3DSTENCILOP_INCR;
      *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_KEEP;
      *data++ = D3DRS_CCW_STENCILPASS;    *data++ = D3DSTENCILOP_KEEP;
    }
    else
    {
      *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_KEEP;
      *data++ = D3DRS_CCW_STENCILZFAIL;   *data++ = D3DSTENCILOP_KEEP;
      *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_INCR;
      *data++ = D3DRS_CCW_STENCILPASS;    *data++ = D3DSTENCILOP_DECR;
    }
    break;
  case smBF_STENCILRENDER:
    *data++ = D3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = D3DRS_TWOSIDEDSTENCILMODE; *data++ = 0;
    *data++ = D3DRS_STENCILFUNC;        *data++ = D3DCMP_EQUAL;
    *data++ = D3DRS_STENCILFAIL;        *data++ = D3DSTENCILOP_ZERO;
    *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_ZERO;
    *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_ZERO;
    break;
  }

  texuv1 = 0;
  for(i=0;i<4;i++)
  {
    static const sU32 filters[8][4] = 
    {
      { sD3DTEXF_POINT,sD3DTEXF_POINT,sD3DTEXF_NONE },
      { sD3DTEXF_LINEAR,sD3DTEXF_LINEAR,sD3DTEXF_NONE },
      { sD3DTEXF_LINEAR,sD3DTEXF_LINEAR,sD3DTEXF_LINEAR },
      { sD3DTEXF_LINEAR,sD3DTEXF_ANISOTROPIC,sD3DTEXF_LINEAR },
      { sD3DTEXF_POINT,sD3DTEXF_POINT,sD3DTEXF_POINT },
      { sD3DTEXF_POINT,sD3DTEXF_POINT,sD3DTEXF_NONE },
      { sD3DTEXF_POINT,sD3DTEXF_POINT,sD3DTEXF_NONE },
      { sD3DTEXF_POINT,sD3DTEXF_POINT,sD3DTEXF_NONE },
    };
    TFile[i] = -1;
    if(info.Tex[i]!=sINVALID)
    {
      flags = info.TFlags[i];
      tex = TCount*sD3DTSS_1;
      sam = TCount*sD3DSAMP_1;
      mtrl->TexUse[TCount] = i;
      TFile[i]=TCount++;

      *data++ = sam+sD3DSAMP_MAGFILTER;    *data++ = filters[flags&7][0];
      *data++ = sam+sD3DSAMP_MINFILTER;    *data++ = filters[flags&7][1];
      *data++ = sam+sD3DSAMP_MIPFILTER;    *data++ = filters[flags&7][2];
      *data++ = sam+sD3DSAMP_ADDRESSU;     *data++ = (flags&smTF_CLAMP) ? sD3DTADDRESS_CLAMP : sD3DTADDRESS_WRAP;
      *data++ = sam+sD3DSAMP_ADDRESSV;     *data++ = (flags&smTF_CLAMP) ? sD3DTADDRESS_CLAMP : sD3DTADDRESS_WRAP;

      if(flags & smTF_UV1)
        texuv1 = 1;

      if(info.ShaderLevel <= sPS_14)
      {
        *data++ = tex+sD3DTSS_TEXTURETRANSFORMFLAGS;
        *data++ = (flags & smTF_PROJECTIVE) ? sD3DTTFF_PROJECTED : 0;
      }
    }
  }

// what do we need?

  for(i=0;i<sMCS_MAX;i++)
  {
    flags = info.Combiner[i];
    combinermask |= psatoc[flags&15];
    if(flags & 0xff0)
      combinermask |= 1<<i;
  }
  flags = info.AlphaCombiner;
  combinermask |= psatoc[flags&15];
  combinermask |= psatoc[(flags>>4)&15];


  // Register Allocation

  switch(info.ShaderLevel)
  {
#if !sINTRO
  case sPS_20:
    switch(lightmode)
    {
    case sMLF_DIR:
      RFile[REyeNormal] = RCount++;
      break;
    case sMLF_BUMPENV:
      combinermask |= 1<<sMCS_TEX1;
      RFile[REyeTangent] = RCount++;
      RFile[REyeBinormal] = RCount++;
      decltspace = 1;
      if(info.TFlags[1]&smTF_TRANS1) 
        error = 1;
      /* nobreak */
    case sMLF_POINT:
      RFile[REyeNormal] = RCount++;
      RFile[REyePos] = RCount++;
      break;
    case sMLF_BUMP:
      combinermask |= 1<<sMCS_TEX1;
      RFile[RTexLight] = RCount++;
      RFile[RTexPos] = RCount++;
      decltspace = 1;
      if(info.TFlags[1]&smTF_TRANS1) 
        error = 1;
      break;
    }
    if(info.SpecialFlags&sMSF_BUMPDETAIL)
    {
      combinermask |= 1<<sMCS_TEX0;
      if(info.TFlags[0]&smTF_TRANS1) 
        error = 1;
    }
    switch(info.SpecialFlags & sMSF_ENVIMASK)
    {
    case sMSF_ENVIREFLECT:
      if(RFile[REyePos]==-1)
        RFile[REyePos] = RCount++;
      /* nobreak */
    case sMSF_ENVISPHERE:
      if(RFile[REyeNormal]==-1)
        RFile[REyeNormal] = RCount++;
      break;
    }

    if(combinermask & (1<<sMCS_SPECULAR))
      RFile[RHalfway] = RCount++;
    break;
#endif
  case sPS_13:
  case sPS_14:
    switch(lightmode)
    {
    case sMLF_BUMP:
    case sMLF_BUMPX:
    case sMLF_BUMPXSPEC:
      if(lightmode == sMLF_BUMP || TFile[1] != -1)
        combinermask |= 1<<sMCS_TEX1;
    case sMLF_POINT:
      decltspace = 1;
      break;
    }
    break;
  }

// *** Vertex Shader Startup

  *vs++ = XO_VS0101;
  *vs++ = XO_DCL; *vs++ = XD_POSITION;   *vs++ = XDALL|X_V|0;

  if(info.BaseFlags & smBF_SHADOWMASK)
  {
    // calc position
    *vs++ = XO_MUL;
    *vs++ = XDALL|X_R|3;
    *vs++ = XSALL|X_V|0;
    *vs++ = XS_W |X_V|0;

    *vs++ = XO_ADD;
    *vs++ = XDALL|X_R|0;
    *vs++ = XS_W |X_C|20;
    *vs++ = XS_W |XS_NEG|X_V|0;

    *vs++ = XO_ADD;
    *vs++ = XDALL|X_R|1;
    *vs++ = XSALL|X_V|0;
    *vs++ = XSALL|XS_NEG|X_C|24;

    *vs++ = XO_MAD;
    *vs++ = XDALL|X_R|3;
    *vs++ = XSALL|X_R|1;
    *vs++ = XSALL|X_R|0;
    *vs++ = XSALL|X_R|3;
  }
  else
  {
    i = 1;
    if(!(info.BaseFlags & smBF_NOTEXTURE))
    {
      i = 2;
      *vs++ = XO_DCL; *vs++ = XD_TEXCOORD0;  *vs++ = XDALL|X_V|1;
    }
    if(!(info.BaseFlags & smBF_NONORMAL))
    {
      decltspace = 1;

      if(decltspace)
      {
        i = 4;
        *vs++ = XO_DCL; *vs++ = XD_NORMAL;    *vs++ = XDALL|X_V|2;
        *vs++ = XO_DCL; *vs++ = XD_TANGENT;   *vs++ = XDALL|X_V|3;
      }
      else
      {
        i = 3;
        *vs++ = XO_DCL; *vs++ = XD_NORMAL;     *vs++ = XDALL|X_V|2;
      }
    }
    if(texuv1)
    {
      *vs++ = XO_DCL; *vs++ = XD_TEXCOORD1;  *vs++ = XDALL|X_V|i;
      texuv1 = i;
      i++;
    }
    if((combinermask & (1<<sMCS_VERTEX)))
    {
      if(info.SpecialFlags & sMSF_LONGCOLOR)
      {
        *vs++ = XO_DCL; *vs++ = XD_NORMAL;     *vs++ = XDALL|X_V|i;
        *vs++ = XO_MUL;
        *vs++ = XDALL|X_OCOLOR|0;
        *vs++ = XSALL|X_V|i;
        *vs++ = XS_W|X_C|19;
      }
      else
      {
        *vs++ = XO_DCL; *vs++ = XD_COLOR0;     *vs++ = XDALL|X_V|i;
        *vs++ = XO_MOV;
        *vs++ = XDALL|X_OCOLOR|0;
        *vs++ = XSALL|X_V|i;
      }
    }
    *vs++ = XO_MOV;
    *vs++ = XDALL|X_R|3;
    *vs++ = XSALL|X_V|0;
  }

  *vs++ = XO_M4x4;
  *vs++ = XDALL|X_OPOS;
  *vs++ = XSALL|X_R|3;
  *vs++ = XSALL|X_C|8;

  if(!(info.BaseFlags & smBF_NONORMAL) && decltspace)
  {
    // unpack packed normal/tangent and calculate binormal
    *vs++ = XO_MUL;
    *vs++ = XD_XYZ|X_R|8;
    *vs++ = XSALL|X_V|2;
    *vs++ = XS_W|X_C|19;

    *vs++ = XO_MUL;
    *vs++ = XD_Z|X_R|10;
    *vs++ = XS_W|X_V|2;
    *vs++ = XS_W|X_C|19;

    *vs++ = XO_MUL;
    *vs++ = XD_XY|X_R|10;
    *vs++ = XSALL|X_V|3;
    *vs++ = XS_W|X_C|19;

    *vs++ = XO_MUL;
    *vs++ = XD_XYZ|X_R|9;
    *vs++ = XS_YZXW|X_R|8;
    *vs++ = XS_ZXYW|X_R|10;
    
    *vs++ = XO_MAD;
    *vs++ = XD_XYZ|X_R|9;
    *vs++ = XS_ZXYW|X_R|8|XS_NEG;
    *vs++ = XS_YZXW|X_R|10;
    *vs++ = XSALL|X_R|9;
  }

// vertex shader: texture coordinates

  for(i=0;i<4;i++)
  {
    if(combinermask&psttoc[i])
    {
      RFile[RTex0+i] = RCount++;
      j = XSALL|X_V|1;
      if(info.TFlags[i] & smTF_UV1)
        j = XSALL|X_V|texuv1;
      if(i==2)
      {
        switch(info.SpecialFlags&sMSF_ENVIMASK)
        {
        case sMSF_ENVIREFLECT:
          *vs++ = XO_M4x3;         // wasted calculation !!!
          *vs++ = XD_XYZ|X_R|1;     // r0 = e
          *vs++ = XSALL|X_R|3;
          *vs++ = XSALL|X_C|4;
          vs_nrm(vs,0,XSALL|X_R|1);

          *vs++ = XO_M3x3;        // wasted calculation !!!
          *vs++ = XD_XYZ|X_R|1;   // r1 = n
          *vs++ = XSALL|X_R|8;
          *vs++ = XSALL|X_C|4;

          *vs++ = XO_DP3;         // r2 = n*e
          *vs++ = XD_W|X_R|2;
          *vs++ = XSALL|X_R|0;
          *vs++ = XSALL|X_R|1;

          *vs++ = XO_ADD;         // r2 = 2*(n*e)
          *vs++ = XD_W|X_R|2;
          *vs++ = XS_W|X_R|2;
          *vs++ = XS_W|X_R|2;

          *vs++ = XO_MAD;         // r2 = 2*(n*e)*n-e
          *vs++ = XD_XYZ|X_R|2;
          *vs++ = XS_W|X_R|2;
          *vs++ = XSALL|X_R|1;
          *vs++ = XSALL|X_R|0|XS_NEG;

          vs_nrm(vs,1,XSALL|X_R|2);
          goto ENVI1;

        case sMSF_ENVISPHERE:
          *vs++ = XO_M3x2;        // wasted calculation !!!
          *vs++ = XD_XY|X_R|1;
          *vs++ = XSALL|X_R|8;
          *vs++ = XSALL|X_C|4;

        ENVI1:
          *vs++ = XO_MAD;
          *vs++ = XD_XY|X_R|1;
          *vs++ = XSALL|X_R|1;
          *vs++ = XSALL|X_C|20;
          *vs++ = XS_X|X_C|20;

          j = XSALL|X_R|1;
          break;
        }
      }
      if(i==3)
      {
        switch(info.SpecialFlags&sMSF_PROJMASK)
        {
        case sMSF_PROJMODEL:
          j = XSALL|X_R|3;
          break;
        case sMSF_PROJWORLD:
          j = XSALL|X_R|1;
          *vs++ = XO_M4x3;
          *vs++ = XD_XYZ|X_R|1;
          *vs++ = XSALL|X_R|3;
          *vs++ = XSALL|X_C|0;
          break;
        case sMSF_PROJEYE:
          j = XSALL|X_R|1;
          *vs++ = XO_M4x3;
          *vs++ = XD_XYZ|X_R|1;
          *vs++ = XSALL|X_R|3;
          *vs++ = XSALL|X_C|4;
          break;
        case sMSF_PROJSCREEN:
          j = XSALL|X_R|1;
          *vs++ = XO_M4x4;
          *vs++ = XDALL|X_R|1;
          *vs++ = XSALL|X_R|3;
          *vs++ = XSALL|X_C|28;
          break;
        }
      }
      switch(info.TFlags[i]&smTF_TRANSMASK)
      {
      case smTF_DIRECT:
        *vs++ = XO_MOV;
        *vs++ = ((info.TFlags[i]&smTF_PROJECTIVE)?XDALL:XD_XY)|X_OUV|RFile[RTex0+i];
        *vs++ = j;
        break;
      case smTF_SCALE:
        *vs++ = XO_MUL;
        *vs++ = XD_XY|X_OUV|RFile[RTex0+i];
        *vs++ = j;
        *vs++ = vsswizzles[i]|X_C|13;
        break;
      case smTF_TRANS1:
        *vs++ = XO_DP4;
        *vs++ = XD_X|X_R|0;
        *vs++ = j;
        *vs++ = XSALL|X_C|16;
        *vs++ = XO_DP4;
        *vs++ = XD_Y|X_R|0;
        *vs++ = j;
        *vs++ = XSALL|X_C|17;
        *vs++ = XO_MOV;
        *vs++ = XD_XY|X_OUV|RFile[RTex0+i];
        *vs++ = XSALL|X_R|0;
        break;
      case smTF_TRANS2:
        *vs++ = XO_DP4;
        *vs++ = XD_X|X_R|0;
        *vs++ = j;
        *vs++ = XSALL|X_C|14;
        *vs++ = XO_DP4;
        *vs++ = XD_Y|X_R|0;
        *vs++ = j;
        *vs++ = XSALL|X_C|15;
        *vs++ = XO_MOV;
        *vs++ = XD_XY|X_OUV|RFile[RTex0+i];
        *vs++ = XSALL|X_R|0;
        break;
      }
    }
  }

//*** Different code paths for different shaders-levels

  switch(info.ShaderLevel)
  {

//*** Pixel Shader 0.0 , no pixel shader available

#if !sINTRO
  case sPS_00:
    {
      sInt hascolor;

      *data++ = sD3DTSS_0|sD3DTSS_COLORARG1; *data++ = sD3DTA_TEXTURE;
      *data++ = sD3DTSS_0|sD3DTSS_COLORARG2; *data++ = sD3DTA_DIFFUSE;
      *data++ = sD3DTSS_1|sD3DTSS_COLORARG1; *data++ = sD3DTA_TEXTURE;
      *data++ = sD3DTSS_1|sD3DTSS_COLORARG2; *data++ = sD3DTA_CURRENT;
      *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG1; *data++ = sD3DTA_TEXTURE;
      *data++ = sD3DTSS_0|sD3DTSS_ALPHAARG2; *data++ = sD3DTA_DIFFUSE;
      *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG1; *data++ = sD3DTA_TEXTURE;
      *data++ = sD3DTSS_1|sD3DTSS_ALPHAARG2; *data++ = sD3DTA_CURRENT;
      *data++ = sD3DTSS_2|sD3DTSS_COLOROP;   *data++ = sD3DTOP_DISABLE;
      *data++ = sD3DTSS_2|sD3DTSS_ALPHAOP;   *data++ = sD3DTOP_DISABLE;

      hascolor = 0;
      if(info.Combiner[sMCS_LIGHT]&0x0f0)
      {
        switch(lightmode)
        {
        case sMLF_DIR:
          hascolor = 1;
          *vs++ = XO_DP3;
          *vs++ = XDALL|X_R|0;
          *vs++ = XSALL|X_V|2;
          *vs++ = XSALL|X_C|21;
          break;
        case sMLF_POINT:
          hascolor = 1;
          *vs++ = XO_ADD;               // r0 = dist
          *vs++ = XD_XYZ|X_R|0;
          *vs++ = XSALL|X_V|0|XS_NEG;
          *vs++ = XSALL|X_C|12;

          *vs++ = XO_DP3;               // r1.w = dist*dist
          *vs++ = XD_W|X_R|1;
          *vs++ = XSALL|X_R|0;
          *vs++ = XSALL|X_R|0;

          *vs++ = XO_RSQ;               // r0.w = 1/sqrt(dist*dist)
          *vs++ = XD_W|X_R|1;
          *vs++ = XS_W|X_R|1;

          *vs++ = XO_MUL;               // r1 = dir
          *vs++ = XD_XYZ|X_R|1;
          *vs++ = XSALL|X_R|0;
          *vs++ = XS_W|X_R|1;

          *vs++ = XO_DP3;               // r0 = light
          *vs++ = XDALL|X_R|0;
          *vs++ = XSALL|X_V|2;
          *vs++ = XSALL|X_R|1;

          *vs++ = XO_RCP;               // r1.w = dist
          *vs++ = XD_W|X_R|1;
          *vs++ = XS_W|X_R|1;
            
          *vs++ = XO_MUL;               // r1.w = dist/range
          *vs++ = XD_W|X_R|1;
          *vs++ = XS_W|X_R|1;
          *vs++ = XS_W|X_C|19;

          *vs++ = XO_ADD;               // scale & sat
          *vs++ = XD_W|X_R|1;
          *vs++ = XS_W|X_C|20;
          *vs++ = XS_W|X_R|1|XS_NEG;

          *vs++ = XO_MAX;
          *vs++ = XD_W|X_R|1;
          *vs++ = XS_W|X_R|1;
          *vs++ = XS_Z|X_C|20;

          *vs++ = XO_MIN;
          *vs++ = XD_W|X_R|1;
          *vs++ = XS_W|X_R|1;
          *vs++ = XS_W|X_C|20;

          *vs++ = XO_MUL;               // apply attenuation
          *vs++ = XD_XYZ|X_R|0;
          *vs++ = XSALL|X_R|0;
          *vs++ = XS_W|X_R|1;
          break;
        }
      }
      switch(info.Combiner[sMCS_COLOR0]&0x0f0)
      {
      case sMCOA_SET:
        hascolor = 1;
        *vs++ = XO_MOV;
        *vs++ = XDALL|X_R|0;
        *vs++ = XSALL|X_C|22;
        break;
      case sMCOA_ADD:
        if(!hascolor) error = 1;
        hascolor = 1;
        *vs++ = XO_ADD;
        *vs++ = XDALL|X_R|0;
        *vs++ = XSALL|X_R|0;
        *vs++ = XSALL|X_C|22;
        break;
      case sMCOA_SUB:
        if(!hascolor) error = 1;
        hascolor = 1;
        *vs++ = XO_SUB;
        *vs++ = XDALL|X_R|0;
        *vs++ = XSALL|X_R|0;
        *vs++ = XSALL|X_C|22;
        break;
      case sMCOA_MUL:
        if(!hascolor) error = 1;
        hascolor = 1;
        *vs++ = XO_MUL;
        *vs++ = XDALL|X_R|0;
        *vs++ = XSALL|X_R|0;
        *vs++ = XSALL|X_C|22;
        break;
      }
      switch(info.Combiner[sMCS_COLOR1]&0x0f0)
      {
      case sMCOA_SET:
        hascolor = 1;
        *vs++ = XO_MOV;
        *vs++ = XDALL|X_R|0;
        *vs++ = XSALL|X_C|23;
        break;
      case sMCOA_ADD:
        if(!hascolor) error = 1;
        hascolor = 1;
        *vs++ = XO_ADD;
        *vs++ = XDALL|X_R|0;
        *vs++ = XSALL|X_R|0;
        *vs++ = XSALL|X_C|23;
        break;
      case sMCOA_SUB:
        if(!hascolor) error = 1;
        hascolor = 1;
        *vs++ = XO_SUB;
        *vs++ = XDALL|X_R|0;
        *vs++ = XSALL|X_R|0;
        *vs++ = XSALL|X_C|23;
        break;
      case sMCOA_MUL:
        if(!hascolor) error = 1;
        hascolor = 1;
        *vs++ = XO_MUL;
        *vs++ = XDALL|X_R|0;
        *vs++ = XSALL|X_R|0;
        *vs++ = XSALL|X_C|23;
        break;
      }

      if(hascolor)
      {
        *vs++ = XO_MOV;
        *vs++ = XDALL|X_OCOLOR|0;
        *vs++ = XSALL|X_R|0;
      }

      if(info.Tex[0]==sINVALID)
      {
        *data++ = sD3DTSS_0|sD3DTSS_COLOROP; *data++ = sD3DTOP_SELECTARG2;
        *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP; *data++ = sD3DTOP_SELECTARG2;
        *data++ = sD3DTSS_1|sD3DTSS_COLOROP; *data++ = sD3DTOP_DISABLE;
        *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP; *data++ = sD3DTOP_DISABLE;
      }
      else
      {
        i = info.Combiner[sMCS_TEX0];
        if(info.Combiner[sMCS_VERTEX])
        {
          hascolor = 1;
          i = info.Combiner[sMCS_VERTEX];
        }
        switch(i&0x0f0)
        {
        default:
          *data++ = sD3DTSS_0|sD3DTSS_COLOROP; *data++ = sD3DTOP_SELECTARG1;
          break;
        case sMCOA_ADD:
          if(!hascolor) error = 1;
          *data++ = sD3DTSS_0|sD3DTSS_COLOROP; *data++ = sD3DTOP_ADD;
          break;
        case sMCOA_SUB:
          if(!hascolor) error = 1;
          *data++ = sD3DTSS_0|sD3DTSS_COLORARG1; *data++ = sD3DTA_DIFFUSE;
          *data++ = sD3DTSS_0|sD3DTSS_COLORARG2; *data++ = sD3DTA_TEXTURE;
          *data++ = sD3DTSS_0|sD3DTSS_COLOROP; *data++ = sD3DTOP_SUBTRACT;
          break;
        case sMCOA_MUL:
          if(!hascolor) error = 1;
          *data++ = sD3DTSS_0|sD3DTSS_COLOROP; *data++ = sD3DTOP_MODULATE;
          break;
        case sMCOA_MUL2:
          if(!hascolor) error = 1;
          *data++ = sD3DTSS_0|sD3DTSS_COLOROP; *data++ = sD3DTOP_MODULATE2X;
          break;
        case sMCOA_SMOOTH:
          if(!hascolor) error = 1;
          *data++ = sD3DTSS_0|sD3DTSS_COLOROP; *data++ = sD3DTOP_ADDSMOOTH;
          break;
        case sMCOA_RSUB:
          if(!hascolor) error = 1;
          *data++ = sD3DTSS_0|sD3DTSS_COLOROP; *data++ = sD3DTOP_SUBTRACT;
          break;
        }
        *data++ = sD3DTSS_0|sD3DTSS_ALPHAOP; *data++ = sD3DTOP_MODULATE;
        switch(ps00_combine)
        {
        case sMCOA_SET:
          *data++ = sD3DTSS_1|sD3DTSS_COLOROP; *data++ = sD3DTOP_SELECTARG1;
          *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP; *data++ = sD3DTOP_SELECTARG2;
          break;
        case sMCOA_ADD:
          *data++ = sD3DTSS_1|sD3DTSS_COLOROP; *data++ = sD3DTOP_ADD;
          *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP; *data++ = sD3DTOP_SELECTARG2;
          break;
        case sMCOA_SUB:
          *data++ = sD3DTSS_1|sD3DTSS_COLORARG1; *data++ = sD3DTA_CURRENT;
          *data++ = sD3DTSS_1|sD3DTSS_COLORARG2; *data++ = sD3DTA_TEXTURE;
          *data++ = sD3DTSS_1|sD3DTSS_COLOROP; *data++ = sD3DTOP_SUBTRACT;
          *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP; *data++ = sD3DTOP_SELECTARG2;
          break;
        case sMCOA_MUL:
          *data++ = sD3DTSS_1|sD3DTSS_COLOROP; *data++ = sD3DTOP_MODULATE;
          *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP; *data++ = sD3DTOP_SELECTARG2;
          break;
        case sMCOA_MUL2:
          *data++ = sD3DTSS_1|sD3DTSS_COLOROP; *data++ = sD3DTOP_MODULATE2X;
          *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP; *data++ = sD3DTOP_SELECTARG2;
          break;
        case sMCOA_MUL4:
          *data++ = sD3DTSS_1|sD3DTSS_COLOROP; *data++ = sD3DTOP_MODULATE4X;
          *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP; *data++ = sD3DTOP_SELECTARG2;
          break;
        case sMCOA_RSUB:
          *data++ = sD3DTSS_1|sD3DTSS_COLOROP; *data++ = sD3DTOP_SUBTRACT;
          *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP; *data++ = sD3DTOP_SELECTARG2;
          break;
        default:
          *data++ = sD3DTSS_1|sD3DTSS_COLOROP; *data++ = sD3DTOP_DISABLE;
          *data++ = sD3DTSS_1|sD3DTSS_ALPHAOP; *data++ = sD3DTOP_DISABLE;
          break;
        }
      }
    }
    break;
#endif

//*** Pixel Shader 1.3 (1.4 has no own path)

  case sPS_14:
  case sPS_13:
    {
      sU32 newreg[sMCS_MAX] =
      {
        XSALL|X_R|1,  // light
        XSALL|X_C|0,  // col0: diffuse
        XSALL|X_C|1,  // col1: ambient
        ~0,           // tex0
        ~0,           // tex1
        ~0,           // merge
        XS_W|X_R|1,   // spec
        XSALL|X_C|2,  // col2
        XSALL|X_C|3,  // col3
        ~0,           // tex2
        ~0,           // tex3
        XSALL|X_V|0,  // vertex color
        ~0,           // final
      };
      static const sU32 psalpha13[16] =
      {
        XS_W|X_C|6, XS_Z|X_R|1, XS_W|X_R|1, XS_W|X_C|6,
        XS_W|X_T|0, XS_W|X_T|1, XS_W|X_T|2, XS_W|X_T|3,
        XS_W|X_C|0, XS_W|X_C|1, XS_W|X_C|2, XS_W|X_C|3,
        XS_W|X_C|7, XS_Z|X_C|6,         ~0,         ~0,
      };
      sInt oldreg;
      sInt hasspec;
      sU32 cocmd[8][8];   // coissued alpha cmd's
      sInt cocount;       // write index
      sInt coindex;       // read index
      sInt reg;

      // start pixel shader and sample all textures

      newreg[sMCS_TEX0] = XSALL|X_T|TFile[0];
      newreg[sMCS_TEX1] = XSALL|X_T|TFile[1];
      newreg[sMCS_TEX2] = XSALL|X_T|TFile[2];
      newreg[sMCS_TEX3] = XSALL|X_T|TFile[3];

      cocount = 0;
      coindex = 0;
      hasspec = combinermask & (1<<sMCS_SPECULAR);
      *ps++ = XO_PS0103;
      for(i=0;i<4;i++)
      {
        if(combinermask & psttoc[i])
        {
          if(info.Tex[i]==sINVALID) 
            error = 1;
          if(TFile[i]==-1)
            error = 1;
          *ps++ = XO_TEX13;
          *ps++ = XDALL|X_T|TFile[i];
        }
      }

      // if there is light, then we need a normal

      if(lightmode)
      {
        if(TCount>=4)
          error = 1;
        *ps++ = XO_TEX13;
        *ps++ = XDALL|X_T|TCount;
        mtrl->TexUse[TCount] = -2;
        sInt sam = TCount*sD3DSAMP_1;
        *data++ = sam+sD3DSAMP_MAGFILTER;    *data++ = sD3DTEXF_LINEAR;
        *data++ = sam+sD3DSAMP_MINFILTER;    *data++ = sD3DTEXF_LINEAR;
        *data++ = sam+sD3DSAMP_MIPFILTER;    *data++ = sD3DTEXF_NONE;
        *data++ = sam+sD3DSAMP_ADDRESSU;     *data++ = (lightmode == sMLF_BUMPXSPEC) ? sD3DTADDRESS_CLAMP : sD3DTADDRESS_WRAP;
        *data++ = sam+sD3DSAMP_ADDRESSV;     *data++ = sD3DTADDRESS_WRAP;
        *data++ = TCount*sD3DTSS_1+sD3DTSS_TEXTURETRANSFORMFLAGS; *data++ = 0;
        if(hasspec)
        {
          if(TCount+1>=4)
            error = 1;
          *ps++ = XO_TEX13;
          *ps++ = XDALL|X_T|(TCount+1);
          mtrl->TexUse[TCount+1] = -2;
          sInt sam = (TCount+1)*sD3DSAMP_1;
          *data++ = sam+sD3DSAMP_MAGFILTER;    *data++ = sD3DTEXF_LINEAR;
          *data++ = sam+sD3DSAMP_MINFILTER;    *data++ = sD3DTEXF_LINEAR;
          *data++ = sam+sD3DSAMP_MIPFILTER;    *data++ = sD3DTEXF_NONE;
          *data++ = sam+sD3DSAMP_ADDRESSU;     *data++ = sD3DTADDRESS_WRAP;
          *data++ = sam+sD3DSAMP_ADDRESSV;     *data++ = sD3DTADDRESS_WRAP;
          *data++ = (TCount+1)*sD3DTSS_1+sD3DTSS_TEXTURETRANSFORMFLAGS; *data++ = 0;
        }
      }
      oldreg = 0;
      j = 0;

      // light calculation
/*
      if(lightmode==sMLF_BUMP||lightmode==sMLF_BUMPENV)
      {
        if(info.Combiner[sMCS_TEX1]&0x00f)
        {
          *ps++ = XO_MUL;
          *ps++ = XD_XYZ|X_T|TFile[1];
          *ps++ = XSALL|X_T|TFile[1];
          *ps++ = psalpha13[info.Combiner[sMCS_TEX1]&0x00f];
        }
      }
*/
      switch(lightmode)
      {
#if !sINTRO
      case sMLF_DIR:
        break;
      case sMLF_POINT:
      case sMLF_BUMP:
        *vs++ = XO_ADD;         // distance
        *vs++ = XD_XYZ|X_R|0;
        *vs++ = XSALL|X_C|12;
        *vs++ = XSALL|X_V|0|XS_NEG;

        *vs++ = XO_DP3;         // normalize
        *vs++ = XD_W|X_R|1;
        *vs++ = XSALL|X_R|0;
        *vs++ = XSALL|X_R|0;

        *vs++ = XO_RSQ;
        *vs++ = XD_W|X_R|1;
        *vs++ = XS_W|X_R|1;

        *vs++ = XO_MUL;       // r0 = light dir, normalized, model space
        *vs++ = XD_XYZ|X_R|0;
        *vs++ = XSALL|X_R|0;
        *vs++ = XS_W|X_R|1;

        *vs++ = XO_RCP;       // attenuate
        *vs++ = XD_W|X_R|1;
        *vs++ = XS_W|X_R|1;

        *vs++ = XO_MUL;
        *vs++ = XDALL|X_R|1;
        *vs++ = XS_W|X_R|1;
        *vs++ = XS_W|X_C|19;

        *vs++ = XO_ADD;
        *vs++ = XDALL|X_R|1;
        *vs++ = XS_W|X_R|1|XS_NEG;
        *vs++ = XS_W|X_C|20;

        if(lightmode!=sMLF_BUMP)
        {
          *vs++ = XO_MOV;       // normal vector is (attenuate,0,0,0)
          *vs++ = XD_Y|XD_Z|XD_W|X_R|1;
          *vs++ = XS_Z|X_C|20;

          *vs++ = XO_MOV;       // store in color, automatically saturates
          *vs++ = XDALL|X_OCOLOR|0;
          *vs++ = XSALL|X_R|1;

          *vs++ = XO_M3x3;      // light dir to texture space
          *vs++ = XD_XYZ|X_OUV+TCount;
          *vs++ = XSALL|X_R|0;
          *vs++ = XSALL|X_R|8;

          *ps++ = XO_DP3;         // pixel shader
          *ps++ = XD_XYZ|X_R|1;
          *ps++ = XSALL|X_T|TCount|XS_SIGN;
          *ps++ = XSALL|X_V|0;
        }
        else
        {
          *vs++ = XO_MAX;       // saturate attenuation
          *vs++ = XD_W|X_R|1;
          *vs++ = XSALL|X_R|1;
          *vs++ = XS_Z|X_C|20;

          *vs++ = XO_M3x3;      // light dir to texture space
          *vs++ = XD_XYZ|X_OUV+TCount;
          *vs++ = XSALL|X_R|0;
          *vs++ = XSALL|X_R|8;

          *vs++ = XO_MOV;       // store attenuation as color
          *vs++ = XDALL|X_OCOLOR|0;
          *vs++ = XS_W|X_R|1;

          if(hasspec)         // specular!
          {
            *vs++ = XO_ADD;         // camera direction: distance
            *vs++ = XD_XYZ|X_R|3;
            *vs++ = XSALL|X_C|19;
            *vs++ = XSALL|X_V|0|XS_NEG;

            vs_nrm(vs,2,XSALL|X_R|3); // r2 = camera dir, normalized, model space

            *vs++ = XO_ADD;       // halfway = lightdir + camdir (no need to normalize
            *vs++ = XD_XYZ|X_R|2;
            *vs++ = XSALL|X_R|0;
            *vs++ = XSALL|X_R|2;

            *vs++ = XO_M3x3;      // halfvector to texture space
            *vs++ = XD_XYZ|X_OUV+(TCount+1);
            *vs++ = XSALL|X_R|2;
            *vs++ = XSALL|X_R|8;
                                  // pixel shader
            *ps++ = XO_DP3;       // r0: H*N 
            *ps++ = XD_XYZW|X_R|0|XD_SAT;
            *ps++ = XSALL|X_T|(TCount+1)|XS_SIGN;
            *ps++ = XSALL|X_T|TFile[1];

            *ps++ = XO_DP3;       // r1: L*N
            *ps++ = XD_XYZ|X_R|1|XD_SAT;
            *ps++ = XSALL|X_T|TCount|XS_SIGN;
            *ps++ = XSALL|X_T|TFile[1];

            *ps++ = XO_MUL|XO_CO; // spec^2
            *ps++ = XD_W|X_R|1|XD_SAT;
            *ps++ = XS_W|X_R|0|XS_SIGN;
            *ps++ = XS_W|X_R|0;

            *ps++ = XO_MUL;       // r1: attenuate
            *ps++ = XD_XYZ|X_R|1;
            *ps++ = XSALL|X_R|1;
            *ps++ = XSALL|X_V|0;

            *ps++ = XO_MUL|XO_CO; // spec^4
            *ps++ = XD_W|X_R|1|XD_SAT;
            *ps++ = XSALL|X_R|1|XS_SIGN;
            *ps++ = XSALL|X_R|1;

            cocmd[cocount][0] = 4;  // spec^16
            cocmd[cocount][1] = XO_MUL;     
            cocmd[cocount][2] = XD_W|X_R|1|XD_SAT;
            cocmd[cocount][3] = XS_W|X_R|1;  
            cocmd[cocount][4] = XS_W|X_R|1;
            cocount++;
          }
          else
          {
            *ps++ = XO_DP3;         // pixel shader
            *ps++ = XD_XYZ|X_R|1|XD_SAT;
            *ps++ = XSALL|X_T|TCount|XS_SIGN;
            *ps++ = XSALL|X_T|TFile[1];
           
            *ps++ = XO_MUL;
            *ps++ = XD_XYZ|X_R|1;
            *ps++ = XSALL|X_R|1;
            *ps++ = XSALL|X_V|0;
          }
        }
        break;
#endif
      case sMLF_BUMPX:
      case sMLF_BUMPXSPEC:
        if(TFile[3] != -1) // we need the shadow texture
        {
          static sU32 attenreg[4*2] =
          {
            XS_X|X_C|26, XS_Y|X_C|26,
            XS_Z|X_C|26, XS_W|X_C|26,
            XS_X|X_C|27, XS_Y|X_C|27,
            XS_Z|X_C|27, XS_W|X_C|27,
          };

          if(lightmode == sMLF_BUMPXSPEC) // need camera direction
          {
            *vs++ = XO_ADD;         // camera direction: distance
            *vs++ = XD_XYZ|X_R|3;
            *vs++ = XSALL|X_C|19;
            *vs++ = XSALL|X_V|0|XS_NEG;

            vs_nrm(vs,7,XSALL|X_R|3); // r7 = camera dir, normalized, model space
          }

          for(i=0;i<4;i++) // for all 4 lights
          {
            *vs++ = XO_ADD;         // distance
            *vs++ = XD_XYZ|X_R|i;
            *vs++ = XSALL|X_C|21+i;
            *vs++ = XSALL|X_V|0|XS_NEG;

            *vs++ = XO_DP3;         // normalize
            *vs++ = XD_W|X_R|i;
            *vs++ = XSALL|X_R|i;
            *vs++ = XSALL|X_R|i;

            *vs++ = XO_RSQ;
            *vs++ = XD_W|X_R|4;
            *vs++ = XS_W|X_R|i;

            *vs++ = XO_MUL;         // ri = light dir, normalized, model space
            *vs++ = XDALL|X_R|i;
            *vs++ = XSALL|X_R|i;
            *vs++ = XS_W|X_R|4;

            *vs++ = XO_MAD;         // attenuation
            *vs++ = (XD_X<<i)|X_R|6;
            *vs++ = XS_W|X_R|i;
            *vs++ = attenreg[i*2+0];
            *vs++ = attenreg[i*2+1];

            if(lightmode == sMLF_BUMPXSPEC)
            {
              *vs++ = XO_ADD;       // halfway vector
              *vs++ = XD_XYZ|X_R|i;
              *vs++ = XSALL|X_R|i;
              *vs++ = XSALL|X_R|7;

              *vs++ = XO_DP3;       // normalize
              *vs++ = XD_W|X_R|i;
              *vs++ = XSALL|X_R|i;
              *vs++ = XSALL|X_R|i;

              *vs++ = XO_RSQ;
              *vs++ = XD_W|X_R|i;
              *vs++ = XS_W|X_R|i;

              *vs++ = XO_MUL;
              *vs++ = XDALL|X_R|i;
              *vs++ = XSALL|X_R|i;
              *vs++ = XS_W|X_R|i;
            }

            *vs++ = XO_DP3;         // N.L (or N.H for specular)=light
            *vs++ = (XD_X<<i)|X_R|5;
            *vs++ = XSALL|X_R|i;
            *vs++ = XSALL|X_R|8;

            // diffuse: 6 instrs/light=24 for 4 lights
            // specular: 10 instrs/light=40 for 4 lights
          }

          *vs++ = XO_MAD;           // rescale N.L contributions
          *vs++ = XDALL|X_R|5;
          *vs++ = XSALL|X_R|5;
          *vs++ = XS_X|X_C|20;
          *vs++ = XS_X|X_C|20;

          *vs++ = XO_MAX;           // clamp attenuation values
          *vs++ = XDALL|X_R|6;
          *vs++ = XSALL|X_R|6;
          *vs++ = XS_Z|X_C|20;

          *vs++ = XO_MUL;           // (N.L*attenuation)=intensity
          *vs++ = XDALL|X_R|5;
          *vs++ = XSALL|X_R|5;
          *vs++ = XSALL|X_R|6;

          *vs++ = XO_MUL;           // intensity*colorbright=weights
          *vs++ = XDALL|X_R|5;
          *vs++ = XSALL|X_R|5;
          *vs++ = XSALL|X_C|25;

          *vs++ = XO_MUL;           // calc weighted L vector
          *vs++ = XD_XYZ|X_R|0;
          *vs++ = XSALL|X_R|0;
          *vs++ = XS_X|X_R|5;

          for(i=1;i<4;i++)
          {
            *vs++ = XO_MAD;
            *vs++ = XD_XYZ|X_R|0;
            *vs++ = XSALL|X_R|i;
            *vs++ = (XS_Y*i)|X_R|5;
            *vs++ = XSALL|X_R|0;
          }

          *vs++ = XO_DP3;         // normalize weighted L vector
          *vs++ = XD_W|X_R|0;
          *vs++ = XSALL|X_R|0;
          *vs++ = XSALL|X_R|0;

          *vs++ = XO_RSQ;
          *vs++ = XD_W|X_R|0;
          *vs++ = XS_W|X_R|0;

          *vs++ = XO_MUL;
          *vs++ = XDALL|X_R|0;
          *vs++ = XSALL|X_R|0;
          *vs++ = XS_W|X_R|0;

          *vs++ = XO_M3x3;        // to texture space with it
          *vs++ = XD_XYZ|X_OUV+TCount;
          *vs++ = XSALL|X_R|0;
          *vs++ = XSALL|X_R|8;

          *vs++ = XO_MOV;         // set shadow light intensities
          *vs++ = XDALL|X_OCOLOR|0;
          *vs++ = XS_WZYX|X_R|6;

          *vs++ = XO_MUL;         // calc nonshadow light intensity
          *vs++ = XDALL|X_OCOLOR|1;
          *vs++ = XS_W|X_R|6;
          *vs++ = XSALL|X_C|18;

          // diffuse: 40 slots
          // specular: 60 slots
          // ---

          // pixel shader
          if(lightmode == sMLF_BUMPX)
          {
            *ps++ = XO_MOV;       
            *ps++ = XD_XYZ|X_R|0;
            *ps++ = XSALL|X_V|1;

            *ps++ = XO_MUL|XO_CO;
            *ps++ = XD_W|X_R|0;
            *ps++ = XSALL|X_T|TFile[3];
            *ps++ = XSALL|X_V|0;

            *ps++ = XO_MUL;
            *ps++ = XD_XYZ|X_R|1;
            *ps++ = XSALL|X_T|TFile[3];
            *ps++ = XSALL|X_V|0;

            *ps++ = XO_MUL|XO_CO;
            *ps++ = XD_W|X_R|1;
            *ps++ = XS_Z|X_T|TFile[3];
            *ps++ = XS_Z|X_V|0;

            *ps++ = XO_MAD;
            *ps++ = XD_XYZ|X_R|0;
            *ps++ = XSALL|X_C|0;
            *ps++ = XS_W|X_R|0;
            *ps++ = XSALL|X_R|0;

            *ps++ = XO_DP3;
            *ps++ = XD_XYZ|X_R|1;
            *ps++ = XSALL|X_R|1;
            *ps++ = XSALL|X_C|4;

            *ps++ = XO_MAD;
            *ps++ = XD_XYZ|X_R|0;
            *ps++ = XSALL|X_C|1;
            *ps++ = XS_W|X_R|1;
            *ps++ = XSALL|X_R|0;

            *ps++ = XO_MOV|XO_CO;
            *ps++ = XD_W|X_R|0;
            *ps++ = XS_Z|X_R|1;

            *ps++ = XO_MAD;
            *ps++ = XD_XYZ|X_R|0;
            *ps++ = XSALL|X_C|2;
            *ps++ = XS_W|X_R|0;
            *ps++ = XSALL|X_R|0;

            *ps++ = XO_DP3;
            *ps++ = XD_XYZ|X_R|1|XD_SAT;
            if(TFile[1] != -1)
              *ps++ = XSALL|X_T|TFile[1];
            else
              *ps++ = XSALL|X_C|3;
            *ps++ = XSALL|X_T|TCount|XS_SIGN;
            
            *ps++ = XO_MUL;
            *ps++ = XD_XYZ|X_R|0;
            *ps++ = XSALL|X_R|0;
            *ps++ = XSALL|X_R|1;
          }
          else // specular path
          {
            *ps++ = XO_DP3;       // specular dot
            *ps++ = XD_XYZ|X_R|0|XD_SAT;
            if(TFile[1] != -1)
              *ps++ = XSALL|X_T|TFile[1];
            else
              *ps++ = XSALL|X_C|3;
            *ps++ = XSALL|X_T|TCount|XS_SIGN;

            *ps++ = XO_MUL|XO_CO; // lights*shadow (light4)
            *ps++ = XD_W|X_R|1;
            *ps++ = XSALL|X_T|TFile[3];
            *ps++ = XSALL|X_V|0;

            *ps++ = XO_MUL;       // lights*shadow
            *ps++ = XD_XYZ|X_R|1;
            *ps++ = XSALL|X_T|TFile[3];
            *ps++ = XSALL|X_V|0;

            *ps++ = XO_MUL|XO_CO; // dot^2
            *ps++ = XD_W|X_R|0;
            *ps++ = XS_Z|X_R|0;
            *ps++ = XS_Z|X_R|0;

            *ps++ = XO_DP3;       // first 3 lights intensity
            *ps++ = XD_XYZ|X_R|0;
            *ps++ = XSALL|X_R|1;
            *ps++ = XSALL|X_C|0;

            if(info.SpecPower > 8.0f)
            {
              *ps++ = XO_MUL|XO_CO; // dot^4
              *ps++ = XD_W|X_R|0;
              *ps++ = XSALL|X_R|0;
              *ps++ = XSALL|X_R|0;
            }

            *ps++ = XO_MUL;       // dot^8
            *ps++ = XD_XYZ|X_R|1;
            *ps++ = XS_W|X_R|0;
            *ps++ = XS_W|X_R|0;

            *ps++ = XO_MAD|XO_CO; // all 4 lights intensity
            *ps++ = XD_W|X_R|1;
            *ps++ = XSALL|X_R|1;
            *ps++ = XSALL|X_C|0;
            *ps++ = XS_Z|X_R|0;

            *ps++ = XO_MUL;       // dot^16
            *ps++ = XD_XYZ|X_R|1;
            *ps++ = XSALL|X_R|1;
            *ps++ = XSALL|X_R|1;

            if(TFile[1] != -1 && (info.SpecialFlags & sMSF_SPECMAP))
            {
              *ps++ = XO_MUL|XO_CO; // specularity*intensity
              *ps++ = XD_W|X_R|1;
              *ps++ = XSALL|X_R|1;
              *ps++ = XSALL|X_T|TFile[1];
            }

            if(info.SpecPower > 16.0f)
            {
              *ps++ = XO_MUL;       // dot^32
              *ps++ = XD_XYZ|X_R|0;
              *ps++ = XSALL|X_R|1;
              *ps++ = XSALL|X_R|1;
            }
            else
            {
              *ps++ = XO_MOV;       // dot^16
              *ps++ = XD_XYZ|X_R|0;
              *ps++ = XSALL|X_R|1;
            }

            *ps++ = XO_MUL;       // specularity*intensity*dot^32
            *ps++ = XD_XYZ|X_R|0;
            *ps++ = XSALL|X_R|0;
            *ps++ = XS_W|X_R|1;
          }
        }
        break;
      }

      // alpha to co-buffer
      val = info.AlphaCombiner;
      f0 = val&15;
      f0 = psalpha13[f0];
      if(val&sMCA_INVERTA)
        f0 |= XS_COMP;
      f1 = (val>>4)&15;
      if(f1)
      {
        f1 = psalpha13[f1];
        if(val&sMCA_INVERTB)
          f1 |= XS_COMP;
        cocmd[cocount][0] = 4;
        cocmd[cocount][1] = XO_MUL;     
        cocmd[cocount][2] = XD_W|X_R+0;       
        cocmd[cocount][3] = f0;          
        cocmd[cocount][4] = f1;
        cocount++;
      }
      else
      {
        cocmd[cocount][0] = 3;
        cocmd[cocount][1] = XO_MOV;     
        cocmd[cocount][2] = XD_W|X_R+0;       
        cocmd[cocount][3] = f0;          
        cocount++;
      }      

      if(lightmode != sMLF_BUMPX && lightmode != sMLF_BUMPXSPEC)
      {
        // combiner (half of it, at least)

        for(i=0;i<sMCS_MAX && j<8;i++)
        {
          reg = newreg[i];
          if(reg==~0) continue; // skip unsupported entries
          if((info.Combiner[i]&0x00f) && (info.Combiner[i]&0xf0)!=sMCOA_LERP)
          {
            reg = 1;
            if(oldreg==(XSALL|X_R|1))
              reg = 0;
            *ps++ = XO_MUL;
            *ps++ = XD_XYZ|X_R|reg;
            *ps++ = newreg[i];
            *ps++ = psalpha13[info.Combiner[i]&0x00f];
            reg = (XSALL|X_R|reg);
          }

          switch(info.Combiner[i]&0x0f0)
          {
          default:                    // skip unsupported modes silently...
            break;
          case sMCOA_SET:
            oldreg = reg;
            break;
          case sMCOA_ADD:
            *ps++ = XO_ADD;
            *ps++ = XD_XYZ|X_R|0;
            *ps++ = oldreg;
            *ps++ = reg;
            oldreg = XSALL|X_R|0;
            j++;
            if(coindex<cocount) cothread(ps,cocmd[coindex++],XO_CO);
            break;
          case sMCOA_SUB:
            *ps++ = XO_SUB;
            *ps++ = XD_XYZ|X_R|0;
            *ps++ = oldreg;
            *ps++ = reg;
            oldreg = XSALL|X_R|0;
            j++;
            if(coindex<cocount) cothread(ps,cocmd[coindex++],XO_CO);
            break;
          case sMCOA_MUL:
          case sMCOA_MUL2:
          case sMCOA_MUL4:
            *ps++ = XO_MUL;
            *ps++ = XD_XYZ|X_R|0|(XD_2X*(((info.Combiner[i]&0x0f0)-sMCOA_MUL)>>4));
            *ps++ = oldreg;
            *ps++ = reg;
            oldreg = XSALL|X_R|0;
            j++;
            if(coindex<cocount) cothread(ps,cocmd[coindex++],XO_CO);
            break;
          case sMCOA_MUL8:
            *ps++ = XO_MUL;
            *ps++ = XD_XYZ|X_R|0|XD_4X;
            *ps++ = oldreg;
            *ps++ = reg;
            oldreg = XSALL|X_R|0;
            j++;
            if(coindex<cocount) cothread(ps,cocmd[coindex++],XO_CO);
            *ps++ = XO_ADD;
            *ps++ = XD_XYZ|X_R|0;
            *ps++ = XSALL|X_R|0;
            *ps++ = XSALL|X_R|0;
            break;
          case sMCOA_SMOOTH:
            *ps++ = XO_MAD;
            *ps++ = XD_XYZ|X_R|0;
            *ps++ = oldreg;
            *ps++ = reg|XS_COMP;
            *ps++ = reg;
            oldreg = XSALL|X_R|0;
            j++;
            if(coindex<cocount) cothread(ps,cocmd[coindex++],XO_CO);
            break;
          case sMCOA_LERP:
            if(!(info.Combiner[i]&0x00f)) error = 1;
            *ps++ = XO_LRP;
            *ps++ = XD_XYZ|X_R|0;
            *ps++ = psalpha13[info.Combiner[i]&0x00f];
            if(info.Combiner[i] & sMCA_INVERTA)
              ps[-1] |= XS_COMP;
            *ps++ = oldreg;
            *ps++ = reg;
            oldreg = XSALL|X_R|0;
            j++;
            if(coindex<cocount) cothread(ps,cocmd[coindex++],XO_CO);
            break;
          case sMCOA_RSUB:
            *ps++ = XO_SUB;
            *ps++ = XD_XYZ|X_R|0;
            *ps++ = reg;
            *ps++ = oldreg;
            oldreg = XSALL|X_R|0;
            j++;
            if(coindex<cocount) cothread(ps,cocmd[coindex++],XO_CO);
            break;
          case sMCOA_DP3:
            *ps++ = XO_DP3;
            *ps++ = XD_XYZ|X_R|0;
            *ps++ = reg;
            *ps++ = oldreg;
            oldreg = XSALL|X_R|0;
            j++;
            if(coindex<cocount) cothread(ps,cocmd[coindex++],XO_CO);
            break;
          }
        }

        if(oldreg!=(XSALL|X_R|0))
        {
          *ps++ = XO_MOV;
          *ps++ = XD_XYZ|X_R|0;
          *ps++ = oldreg;
          if(coindex<cocount) cothread(ps,cocmd[coindex++],XO_CO);
        }

        while(coindex<cocount) cothread(ps,cocmd[coindex++],0); // remaining alpha-ops without co-issue
      }
      else
      {
        if(coindex<cocount) cothread(ps,cocmd[coindex++],XO_CO);
        if(coindex<cocount) error = 1;
      }
    }
    break;

//*** Pixel Shader 2.0 (the real thing)
#if !sINTRO
  case sPS_20:
    {
      static const sU32 psregs[sMCS_MAX] =
      {
        X_R|2, X_C|0, X_C|1, X_R|4, X_R|5, X_R|1,
        X_R|3, X_C|2, X_C|3, X_R|6, X_R|7, X_V|0, X_R|1, 
      };
      static const sU32 psops[16] = 
      {
        0, XO_MOV, XO_ADD, XO_ADD, XO_MUL, XO_MUL, XO_MUL, XO_MUL, XO_MAD, XO_LRP, XO_ADD, XO_DP3
      };
      static const sU32 psalpha20[16] =
      {
        XS_W|X_C|6, XS_X|X_R|2, XS_X|X_R|3, XS_W|X_R|3,
        XS_W|X_R|4, XS_W|X_R|5, XS_W|X_R|6, XS_W|X_R|7,
        XS_W|X_C|0, XS_W|X_C|1, XS_W|X_C|2, XS_W|X_C|3,
        XS_X|X_C|6, XS_Z|X_C|6,         ~0,         ~0,
      };
      static const sU32 pstmask[RMAX] =
      {
        XD_XY,XD_XY,XD_XY,XD_XY,                // tex?, uv?
        XD_XYZ,XD_XYZ,XD_XYZ,XD_XYZ,            // eye space
        XD_XYZ,XD_XYZ,                          // texture space 
        XD_XYZ                                  // halfway vector
      };

    //*** vertex shader

        
      if(RCount>8) 
        error = 1;

      if(RFile[REyePos]!=-1)
      {
        *vs++ = XO_M4x3;
        *vs++ = XD_XYZ|X_OUV+RFile[REyePos];
        *vs++ = XSALL|X_R|3;
        *vs++ = XSALL|X_C|4;
      }
      if(RFile[REyeNormal]!=-1)
      {
        *vs++ = XO_M3x3;
        *vs++ = XD_XYZ|X_OUV+RFile[REyeNormal];
        *vs++ = XSALL|X_V|2;
        *vs++ = XSALL|X_C|4;
      }
      if(RFile[REyeBinormal]!=-1)
      {
        *vs++ = XO_M3x3;
        *vs++ = XD_XYZ|X_OUV+RFile[REyeBinormal];
        *vs++ = XSALL|X_V|3;
        *vs++ = XSALL|X_C|4;
      }
      if(RFile[REyeTangent]!=-1)
      {
        *vs++ = XO_M3x3;
        *vs++ = XD_XYZ|X_OUV+RFile[REyeTangent];
        *vs++ = XSALL|X_V|4;
        *vs++ = XSALL|X_C|4;
      }

      if(RFile[RTexPos]!=-1)
      {
        *vs++ = XO_MOV;
        *vs++ = XDALL|X_R|0;
        *vs++ = XSALL|X_R|3;

        *vs++ = XO_M3x3;
        *vs++ = XD_XYZ|X_OUV+RFile[RTexPos];
        *vs++ = XSALL|X_R|0;
        *vs++ = XSALL|X_V|2;
      }
      if(RFile[RTexLight]!=-1)
      {
        *vs++ = XO_M3x3;
        *vs++ = XD_XYZ|X_OUV+RFile[RTexLight];
        *vs++ = XSALL|X_C|12;
        *vs++ = XSALL|X_V|2;
      }
      if(RFile[RHalfway]!=-1)
      {
        if(lightmode==sMLF_BUMP) // model space halfway
        {
          *vs++ = XO_ADD;           // pos-cam
          *vs++ = XD_XYZ|X_R|0;
          *vs++ = XSALL|X_R|3;
          *vs++ = XSALL|X_C|19|XS_NEG;

          *vs++ = XO_ADD;           // pos-light
          *vs++ = XD_XYZ|X_R|2;
          *vs++ = XSALL|X_R|3;
          *vs++ = XSALL|X_C|12|XS_NEG;
        }
        else                        // eye space halfway
        {
          *vs++ = XO_M4x3;          // eye space position = pos - cam
          *vs++ = XD_XYZ|X_R|0;     // may be calculated twice, optimise!
          *vs++ = XSALL|X_R|3;
          *vs++ = XSALL|X_C|4;

          *vs++ = XO_ADD;           // pos-light
          *vs++ = XD_XYZ|X_R|2;
          *vs++ = XSALL|X_R|0;
          *vs++ = XSALL|X_C|18|XS_NEG;
        }
        vs_nrm(vs,1,XSALL|X_R|0);           // nrm(pos-cam)
        vs_nrm(vs,0,XSALL|X_R|2);           // nrm(pos-light)
/*
        *vs++ = XO_NRM;           // nrm(pos-cam)
        *vs++ = XD_XYZ|X_R|1;
        *vs++ = XSALL|X_R|0;

        *vs++ = XO_NRM;           // nrm(pos-light)
        *vs++ = XD_XYZ|X_R|0;
        *vs++ = XSALL|X_R|2;
*/
        if(lightmode==sMLF_BUMP) // texture space
        {
          *vs++ = XO_ADD;           // nrm(pos-cam)+nrm(pos-ligh)
          *vs++ = XD_XYZ|X_R|0;
          *vs++ = XSALL|X_R|1;
          *vs++ = XSALL|X_R|0;

          *vs++ = XO_M3x3;
          *vs++ = XD_XYZ|X_OUV+RFile[RHalfway];
          *vs++ = XSALL|X_R|0;
          *vs++ = XSALL|X_V|2;
        }
        else
        {
          *vs++ = XO_ADD;           // nrm(pos-cam)+nrm(pos-ligh)
          *vs++ = XD_XYZ|X_OUV+RFile[RHalfway];
          *vs++ = XSALL|X_R|1;
          *vs++ = XSALL|X_R|0;
        }
      }

    //*** pixel shader
      // r0 = left
      // r1 = right
      // r2 = light
      // r3 = specular
      // r4 = tex0
      // r5 = tex1
      // r6 = tex2
      // r7 = tex3
      // c0 = color0
      // c1 = color1
      // c2 = color2 | 1.0
      // c3 = color3 | 0.5
      // c4 = eye light direction | specpower
      // c5 = eye light position  | light range

      *ps++ = XO_PS0200;

      // declatations

      for(i=0;i<RMAX;i++)
      {
        if(RFile[i]!=-1)
        {
          *ps++ = XO_DCL;
          *ps++ = XD_REG;
          if(i > RTex3 || !(info.TFlags[i] & smTF_PROJECTIVE))
            *ps++ = pstmask[i]|X_T|RFile[i];
          else
            *ps++ = XDALL|X_T|RFile[i];
        }
      }
      if(combinermask & (1<<sMCS_VERTEX))
      {
        *ps++ = XO_DCL;
        *ps++ = XD_REG;
        *ps++ = XDALL|X_V|0;
      }

      for(i=0;i<4;i++)
      {
        if(combinermask & psttoc[i])
        {
          if(info.Tex[i]==sINVALID)
            error = 1;
          *ps++ = XO_DCL;     
          *ps++ = XD_TEX2D;         
          *ps++ = XDALL|X_S|TFile[i];
        }
      }

      // read textures

      if(combinermask & (1<<sMCS_TEX0))
      {
        *ps++ = (info.TFlags[0] & smTF_PROJECTIVE) ? XO_TEXLDP : XO_TEXLD;
        *ps++ = XDALL|X_R|4;
        *ps++ = XSALL|X_T|RFile[RTex0];
        *ps++ = XSALL|X_S|TFile[0];
      }
      if(combinermask & (1<<sMCS_TEX1))
      {
        *ps++ = (info.TFlags[1] & smTF_PROJECTIVE) ? XO_TEXLDP : XO_TEXLD;
        *ps++ = XDALL|X_R|5;
        *ps++ = XSALL|X_T|RFile[RTex1];
        *ps++ = XSALL|X_S|TFile[1];
        i = 5;
        if(info.SpecialFlags&sMSF_BUMPDETAIL)
        {
          *ps++ = XO_ADD;
          *ps++ = XD_XYZ|X_R|6;
          *ps++ = XSALL|X_R|5;
          *ps++ = XSALL|X_R|4;
          i = 6;
        }
        if(lightmode==sMLF_BUMP||lightmode==sMLF_BUMPENV)
        {
          if(info.Combiner[sMCS_TEX1]&0x00f)
          {
            *ps++ = XO_LRP;
            *ps++ = XD_XYZ|X_R|i+1;
            *ps++ = psalpha20[info.Combiner[sMCS_TEX1]&0x00f];
            *ps++ = XSALL|X_R|i;
            *ps++ = XSALL|X_C|7;
            i++;
          }
        }
        if(i!=5)
        {
          *ps++ = XO_NRM;
          *ps++ = XD_XYZ|X_R|5;
          *ps++ = XSALL|X_R|6;
        }
      }
      
      // light

      normalreg = -1;
      if(combinermask & (1<<sMCS_LIGHT))
      {
        switch(lightmode)
        {
        case sMLF_DIR:
          *ps++ = XO_NRM;
          *ps++ = XD_XYZ|X_R|6;
          *ps++ = XSALL|X_T|RFile[REyeNormal];

          *ps++ = XO_DP3;
          *ps++ = XDALL|X_R|2;
          *ps++ = XSALL|X_R|6;
          *ps++ = XSALL|X_C|4;

          normalreg = XSALL|X_R|6;
          break;

        case sMLF_BUMP:
          *ps++ = XO_MOV;
          *ps++ = XD_XYZ|X_R|6;
          *ps++ = XSALL|X_T|RFile[RTexLight];

          *ps++ = XO_ADD;             // light-pos   (use vertexshader !)
          *ps++ = XD_XYZ|X_R|2;
          *ps++ = XSALL|X_R|6;
          *ps++ = XSALL|X_T|RFile[RTexPos]|XS_NEG;

          normalreg = XSALL|X_R|5;
          goto BUMP2;

        case sMLF_BUMPENV:
          *ps++ = XO_MUL;
          *ps++ = XD_XYZ|X_R|6;
          *ps++ = XSALL|X_T|RFile[REyeNormal];
          *ps++ = XS_X|X_R|5;

          *ps++ = XO_MAD;
          *ps++ = XD_XYZ|X_R|6;
          *ps++ = XSALL|X_T|RFile[REyeBinormal];
          *ps++ = XS_Y|X_R|5;
          *ps++ = XSALL|X_R|6;

          *ps++ = XO_MAD;
          *ps++ = XD_XYZ|X_R|6;
          *ps++ = XSALL|X_T|RFile[REyeTangent];
          *ps++ = XS_Z|X_R|5;
          *ps++ = XSALL|X_R|6;

          normalreg = XSALL|X_R|6;
          goto BUMP1;

        case sMLF_POINT:
          *ps++ = XO_NRM;
          *ps++ = XD_XYZ|X_R|6;
          *ps++ = XSALL|X_T|RFile[REyeNormal];
          normalreg = XSALL|X_R|6;

        BUMP1:
          *ps++ = XO_ADD;             // light-pos   (use vertexshader !)
          *ps++ = XD_XYZ|X_R|2;
          *ps++ = XSALL|X_C|5;
          *ps++ = XSALL|X_T|RFile[REyePos]|XS_NEG;


        BUMP2: 
          *ps++ = XO_DP3;             // normalize
          *ps++ = XD_W|X_R|3;         // r2 = normalized vector
          *ps++ = XSALL|X_R|2;        // r3 = distance scalar
          *ps++ = XSALL|X_R|2;

          *ps++ = XO_RSQ;
          *ps++ = XD_W|X_R|3;
          *ps++ = XS_W|X_R|3;

          *ps++ = XO_MUL;
          *ps++ = XD_XYZ|X_R|2;
          *ps++ = XSALL|X_R|2;
          *ps++ = XS_W|X_R|3;

          *ps++ = XO_RCP;
          *ps++ = XD_W|X_R|3;
          *ps++ = XS_W|X_R|3;

          *ps++ = XO_DP3;             // dot-product it!
          *ps++ = XDALL|X_R|2|XD_SAT;
          *ps++ = normalreg;
          *ps++ = XSALL|X_R|2;

          *ps++ = XO_MUL;             // attenuation
          *ps++ = XD_W|X_R|3;
          *ps++ = XS_W|X_R|3;
          *ps++ = XS_W|X_C|5;

          *ps++ = XO_ADD;
          *ps++ = XD_W|X_R|3|XD_SAT;
          *ps++ = CONST_ONE;
          *ps++ = XS_W|X_R|3|XS_NEG;

          *ps++ = XO_MUL;
          *ps++ = XD_XYZ|X_R|2;
          *ps++ = XSALL|X_R|2;
          *ps++ = XS_W|X_R|3;
          break;
        }
      }
      if(combinermask & (1<<sMCS_SPECULAR))
      {
        *ps++ = XO_NRM;
        *ps++ = XD_XYZ|X_R|7;
        *ps++ = XSALL|X_T|RFile[RHalfway]|XS_NEG;

        *ps++ = XO_DP3;
        *ps++ = XD_W|X_R|7;
        *ps++ = normalreg;
        *ps++ = XSALL|X_R|7;

        *ps++ = XO_POW;
        *ps++ = XD_W|X_R|6;
        *ps++ = XS_W|X_R|7;
        *ps++ = XS_W|X_C|4;

        *ps++ = XO_MUL;
        *ps++ = XD_W|X_R|6|XD_SAT;
        *ps++ = XS_W|X_R|7;
        *ps++ = XS_W|X_R|6;

        *ps++ = XO_MUL;
        *ps++ = XD_XYZ|X_R|3;
        *ps++ = XS_W|X_R|6;
        *ps++ = XS_W|X_R|3;
        if(combinermask & (1<<sMCS_LIGHT))
        {
          *ps++ = XO_CMP;
          *ps++ = XD_XYZ|X_R|3;
          *ps++ = XSALL|X_R|2|XS_NEG;
          *ps++ = CONST_ZERO;
          *ps++ = XSALL|X_R|3;
        }
      }

      // read more textures

      if(combinermask & (1<<sMCS_TEX2))
      {
        if((info.SpecialFlags & sMSF_ENVIMASK) && lightmode==sMLF_BUMPENV)
        {
          *ps++ = XO_MUL;
          *ps++ = XD_XY|X_R|6;
          *ps++ = CONST_HMH;
          *ps++ = normalreg;

          *ps++ = XO_ADD;
          *ps++ = XD_XY|X_R|6;
          *ps++ = CONST_HALF;
          *ps++ = XSALL|X_R|6;

          *ps++ = XO_TEXLD;
          *ps++ = XDALL|X_R|6;
          *ps++ = XSALL|X_R|6;
          *ps++ = XSALL|X_S|TFile[2];
        }
        else
        {
          *ps++ = (info.TFlags[2] & smTF_PROJECTIVE) ? XO_TEXLDP : XO_TEXLD;
          *ps++ = XDALL|X_R|6;
          *ps++ = XSALL|X_T|RFile[RTex2];
          *ps++ = XSALL|X_S|TFile[2];
        }
      }
      if(combinermask & (1<<sMCS_TEX3))
      {
        *ps++ = (info.TFlags[3] & smTF_PROJECTIVE) ? XO_TEXLDP : XO_TEXLD;
        *ps++ = XDALL|X_R|7;
        *ps++ = XSALL|X_T|RFile[RTex3];
        *ps++ = XSALL|X_S|TFile[3];
      }

      // combiner

      sInt firstone[2];
      firstone[0] = 1;
      firstone[1] = 1;
      for(i=0;i<sMCS_MAX;i++)
      {
        if(combinermask & (1<<i))
        {
          flags = info.Combiner[i];
          val = flags;
          if((flags&15)!=0 && (flags&0xf0)!=sMCOA_LERP && (flags&0xf00)!=sMCOB_LERP)
          {
            *ps++ = XO_MUL; 
            *ps++ = XD_XYZ|psregs[i]; 
            *ps++ = XSALL|psregs[i]; 
            *ps++ = psalpha20[flags&15];
          }
          for(j=0;j<2;j++)
          {
            val = val>>4;
            op = val&15;
            if(op)
            {
              if(firstone[j])
                op = sMCOA_SET>>4;
              if((i==sMCS_MERGE || i==sMCS_FINAL) && (firstone[0] || firstone[1]))
                op = sMCOA_NOP>>4;
            }
            if(op)
              firstone[j] = 0;

            *ps++ = psops[op];
            *ps++ = XD_XYZ|X_R|j;
            *ps++ = XSALL|psregs[i];
            *ps++ = XSALL|X_R|j;
            switch(op)
            {
            case sMCOA_NOP>>4:
              ps-=4;
              break;
            case sMCOA_SET>>4:
              ps-=1;
              break;
            case sMCOA_ADD>>4:
            case sMCOA_MUL>>4:
              break;
            case sMCOA_SUB>>4:
              ps[-1] |= XS_NEG;
              break;
            case sMCOA_MUL8>>4:
              *ps++ = XO_ADD;  *ps++ = XD_XYZ|X_R|j;  *ps++ = XSALL|X_R|j;  *ps++ = XSALL|X_R|j;
            case sMCOA_MUL4>>4:
              *ps++ = XO_ADD;  *ps++ = XD_XYZ|X_R|j;  *ps++ = XSALL|X_R|j;  *ps++ = XSALL|X_R|j;
            case sMCOA_MUL2>>4:
              *ps++ = XO_ADD;  *ps++ = XD_XYZ|X_R|j;  *ps++ = XSALL|X_R|j;  *ps++ = XSALL|X_R|j;
              break;
            case sMCOA_SMOOTH>>4:
              ps[-1] |= XS_NEG;
              *ps++ = XSALL|X_R|j;
              *ps++ = XO_ADD;
              *ps++ = XD_XYZ|X_R|j;
              *ps++ = XSALL|X_R|j;
              *ps++ = XSALL|psregs[i];
              break;
            case sMCOA_LERP>>4:
              ps[-2] = psalpha20[flags&15];
              ps[-1] = XSALL|X_R|j;
              *ps++ = XSALL|psregs[i];
              break;
            case sMCOA_RSUB:
              ps[-2] |= XS_NEG;
              break;
            }
          }
        }
      } 

      if(firstone[0])
        error = 1;

      // alpha

      val = info.AlphaCombiner;
      f0 = val&15;
      f0 = psalpha20[f0];
      if(val&sMCA_INVERTA)
      {
        *ps++ = XO_MOV;  *ps++ = XD_W|X_R|0; *ps++ = CONST_ONE;
        *ps++ = XO_ADD;  *ps++ = XD_W|X_R|0; *ps++ = f0|XS_NEG; *ps++ = XS_W|X_R|0;
        f0 = XS_W|X_R|0;
      }
      f1 = (val>>4)&15;
      if(f1)
      {
        f1 = psalpha20[f1];
        if(val&sMCA_INVERTB)
        {
          *ps++ = XO_MOV;  *ps++ = XD_W|X_R|8; *ps++ = CONST_ONE;
          *ps++ = XO_ADD;  *ps++ = XD_W|X_R|8; *ps++ = f1|XS_NEG; *ps++ = XS_W|X_R|8;
          f1 = XS_W|X_R|8;
        }
        *ps++ = XO_MUL;     *ps++ = XD_W|X_R+0;       *ps++ = f0;          *ps++ = f1;
      }
      else
      {
        *ps++ = XO_MOV;     *ps++ = XD_W|X_R+0;       *ps++ = f0;  
      }
      // end

      *ps++ = XO_MOV;     *ps++ = XDALL|X_COLOR+0;  *ps++ = XSALL|X_R+0;
    }
    break;
#endif
  default:
    sFatal("unknown shader level");
    break;
  }

//*** finish shaders

  *data++ = ~0;
  *data++ = ~0;
  mtrl->VS = 0;
  mtrl->PS = 0;
#if !sPLAYER
  pinfo->PSOps = 0;
  pinfo->VSOps = 0;
  pinfo->ShaderLevelUsed = info.ShaderLevel;
  for(sU32 *p=vsb+1;p<vs;p++)           // count VS ops
    if(!(*p&0x80000000))
      pinfo->VSOps++;
  for(sU32 *p=psb+1;p<ps;p++)           // count PS ops
    if((*p&0xc0000000)==0x00000000)     // (skip coissue)
      pinfo->PSOps++;
#endif
  if(!error)
  {
    if(vs!=vsb)
    {
      *vs++ = XO_END;
      if(*vsb<XO_VS0200)
        for(sU32 *p=vsb+1;p<vs;p++)     // remove instruction-length for VS0101
          if(!(*p&0x80000000))
            *p &= 0xf0ffffff;
      hr = DXDev->CreateVertexShader((DWORD *)vsb,&mtrl->VS);
      if(FAILED(hr))
      {
#if !sINTRO
        PrintShader(vsb,vs-vsb);
#endif
        error = 1;
      }

//      PrintShader(vsb,vs-vsb);
    }
    if(ps!=psb)
    {
      *ps++ = XO_END;
      if(*psb<XO_PS0200)
        for(sU32 *p=psb+1;p<ps;p++)     // remove instruction-length for PS0101
          if(!(*p&0x80000000))
            *p &= 0xf0ffffff;
      hr = DXDev->CreatePixelShader((DWORD *)psb,&mtrl->PS);
      if(FAILED(hr))
      {
#if !sINTRO
        PrintShader(psb,ps-psb);
#endif
        error = 1;
      }

//      PrintShader(psb,ps-psb);
    }
  }
  if(error)
  {
    if(mtrl->VS)
      mtrl->VS->Release();
    if(mtrl->PS)
      mtrl->PS->Release();
    mtrl->Info.Init();
    mid = sINVALID;
  }

  return mid;
}

#else

sInt sSystem_::MtrlAdd(sMaterialInfo *pinfo)
{
  enum RFileIndex                 // register file indices (which Tx register?)
  {
    RTex0=0,                      // uv transformed for Texture 0
    RTex1,                        // uv transformed for Texture 1
    RTex2,                        // uv transformed for Texture 2
    RTex3,                        // uv transformed for Texture 3
    REyeNormal,                   // normal in eye space
    REyePos,                      // position in eye space
    REyeBinormal,
    REyeTangent,
    RTexPos,                      // texture space position
    RTexLight,                    // texture space light
    RHalfway,                     // halfway vector, either eye or tex space
    RMAX                          // count of possible registers
  };
  static const sU32 psatoc[16] =
  {
    0,1<<sMCS_LIGHT,(1<<sMCS_SPECULAR)|(1<<sMCS_LIGHT),1<<sMCS_LIGHT,
    1<<sMCS_TEX0,1<<sMCS_TEX1,1<<sMCS_TEX2,1<<sMCS_TEX3,
    1<<sMCS_COLOR0,1<<sMCS_COLOR1,1<<sMCS_COLOR2,1<<sMCS_COLOR3,
    0,0,0,0,
  };
  static const sU32 psttoc[4] = 
  {
    1<<sMCS_TEX0,1<<sMCS_TEX1,1<<sMCS_TEX2,1<<sMCS_TEX3,
  };
  static const sU32 vsswizzles[4] = 
  {
    XS_X,XS_Y,XS_Z,XS_W,
  };

  sMaterial *mtrl;
  sMaterialInfo infocopy;
  sMaterialInfo info;
  sU32 *data;
  sInt mid;
  sInt i,j;
  sInt texuv1;
  sInt tex,sam,val;
  sF32 f;
  sU32 flags;
  static sU32 psb[512];
  static sU32 vsb[512];
  sU32 *ps;
  sU32 *vs;
  HRESULT hr;
  sInt f0,f1;
  sInt combinermask;
  sInt error;
  sInt ps00_combine;
  sU32 lightmode;

  sInt RFile[RMAX];               // the register file. -1 = unused
  sInt TFile[4];                  // sMCS_TEX -> d3d_texture
  sInt RCount;                    // count of T-Registers
  sInt TCount;

// modify material

  info = *pinfo;
  ps00_combine = sMCOA_NOP;
  lightmode = (info.LightFlags & sMLF_MODEMASK);
  //sVERIFY(info.ShaderLevel == sPS_13 || info.ShaderLevel == sPS_14);

  switch(lightmode)
  {
  case sMLF_BUMPENV:
  case sMLF_BUMP:
    info.Tex[2] = sINVALID;
    info.LightFlags = (info.LightFlags&~sMLF_MODEMASK)|sMLF_BUMP;
    break;
  case sMLF_BUMPXSPEC:
    if(info.SpecPower < 8.5f)
      info.SpecPower = 8.0f;
    else if(info.SpecPower < 16.5f)
      info.SpecPower = 16.0f;
    else
      info.SpecPower = 32.0f;
    break;
  }
  if(info.SpecialFlags & sMSF_BUMPDETAIL)
  {
    info.Tex[0] = sINVALID;
    info.SpecialFlags &= ~sMSF_BUMPDETAIL;
  }

// find similar material

  infocopy = info;
  for(i=0;i<4;i++)
    infocopy.Tex[i] = infocopy.Tex[i]==sINVALID?sINVALID:sINVALID-1;
  sSetMem(infocopy.TScale,0,16*4); // TScale .. SRT2
  infocopy.Handle = 0;
  infocopy.Usage = 0;
  infocopy.Pass = 0;
  infocopy.Size = 0;
  infocopy.Aspect = 0;
  infocopy.SpecPower = info.SpecPower;

  for(mid=0;mid<MAX_MATERIALS;mid++)
  {
    if(Materials[mid].RefCount>0)
    {
      if(sCmpMem(&Materials[mid].Info,&infocopy,sizeof(sMaterialInfo))==0)
      {
        Materials[mid].RefCount++;
        return mid;
      }
    }
  }

//*** find free material

  mid = sINVALID;
  mtrl = 0;
  for(mid=0;mid<MAX_MATERIALS;mid++)
  {
    if(Materials[mid].RefCount==0)
    {
      mtrl = &Materials[mid];
      break;
    }
  }
  if(!mtrl) return sINVALID;
  error = 0;

//*** initialize to zero

  mtrl->RefCount = 1;
  mtrl->Info = infocopy;
  mtrl->PS = 0;
  mtrl->VS = 0;
  mtrl->TexUse[0] = -1;
  mtrl->TexUse[1] = -1;
  mtrl->TexUse[2] = -1;
  mtrl->TexUse[3] = -1;

// initialize our state

  data = mtrl->States;
  vs = vsb;
  ps = psb;
  for(i=0;i<RMAX;i++) RFile[i] = -1;
  RCount = 0;
  TCount = 0;
  combinermask = 0;

//*** base flags

  flags = info.BaseFlags;
  f = 0;
  if(flags & smBF_ZBIASBACK) f=0.00005f;
  if(flags & smBF_ZBIASFORE) f=-0.00005f;

  *data++ = sD3DRS_ALPHATESTENABLE;             *data++ = (flags & smBF_ALPHATEST) ? 1 : 0;
  *data++ = sD3DRS_ALPHAFUNC;                   *data++ = (flags & smBF_ALPHATESTINV) ? sD3DCMP_LESSEQUAL : sD3DCMP_GREATER;
  *data++ = sD3DRS_ALPHAREF;                    *data++ = 128;
  *data++ = sD3DRS_ZENABLE;                     *data++ = sD3DZB_TRUE;
  *data++ = sD3DRS_ZWRITEENABLE;                *data++ = (flags & smBF_ZWRITE) ? 1 : 0;
  *data++ = sD3DRS_ZFUNC;                       
  *data++ = (flags & smBF_ZREAD) ? 
    ((flags & smBF_SHADOWMASK)?sD3DCMP_LESS:
    sD3DCMP_LESSEQUAL) : sD3DCMP_ALWAYS;
  *data++ = sD3DRS_CULLMODE;                    *data++ = (flags & smBF_DOUBLESIDED) ? sD3DCULL_NONE : (flags & smBF_INVERTCULL?sD3DCULL_CW:sD3DCULL_CCW);
  *data++ = sD3DRS_COLORWRITEENABLE;            *data++ = (flags & smBF_ZONLY) ? 0 : 15;
  *data++ = sD3DRS_SLOPESCALEDEPTHBIAS;         *data++ = *(sU32 *)&f;
  *data++ = sD3DRS_DEPTHBIAS;                   *data++ = *(sU32 *)&f;
  *data++ = sD3DRS_FOGENABLE;                   *data++ = 0;
  *data++ = sD3DRS_CLIPPING;                    *data++ = 1;
  *data++ = sD3DTSS_TEXCOORDINDEX|sD3DTSS_0;    *data++ = 0;
  *data++ = sD3DTSS_TEXCOORDINDEX|sD3DTSS_1;    *data++ = 1;
  *data++ = sD3DTSS_TEXCOORDINDEX|sD3DTSS_2;    *data++ = 2;
  *data++ = sD3DTSS_TEXCOORDINDEX|sD3DTSS_3;    *data++ = 3;

  switch(flags&smBF_BLENDMASK)
  {
  case smBF_BLENDOFF:
    *data++ = sD3DRS_ALPHABLENDENABLE;  *data++ = 0;
    break;
  case smBF_BLENDADD:
    *data++ = sD3DRS_ALPHABLENDENABLE;  *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;          *data++ = sD3DBLEND_ONE;
    *data++ = sD3DRS_DESTBLEND;         *data++ = sD3DBLEND_ONE;
    *data++ = sD3DRS_BLENDOP;           *data++ = sD3DBLENDOP_ADD;
    break;
  case smBF_BLENDMUL:
    *data++ = sD3DRS_ALPHABLENDENABLE;  *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;          *data++ = sD3DBLEND_ZERO;
    *data++ = sD3DRS_DESTBLEND;         *data++ = sD3DBLEND_SRCCOLOR;
    *data++ = sD3DRS_BLENDOP;           *data++ = sD3DBLENDOP_ADD;
    break;
  case smBF_BLENDALPHA:
    *data++ = sD3DRS_ALPHABLENDENABLE;  *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;          *data++ = sD3DBLEND_SRCALPHA;
    *data++ = sD3DRS_DESTBLEND;         *data++ = sD3DBLEND_INVSRCALPHA;
    *data++ = sD3DRS_BLENDOP;           *data++ = sD3DBLENDOP_ADD;
    break;
  case smBF_BLENDSMOOTH:
    *data++ = sD3DRS_ALPHABLENDENABLE;  *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;          *data++ = sD3DBLEND_ONE;
    *data++ = sD3DRS_DESTBLEND;         *data++ = sD3DBLEND_INVSRCCOLOR;
    *data++ = sD3DRS_BLENDOP;           *data++ = sD3DBLENDOP_ADD;
    break;
  case smBF_BLENDSUB:
    *data++ = sD3DRS_ALPHABLENDENABLE;  *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;          *data++ = sD3DBLEND_ONE;
    *data++ = sD3DRS_DESTBLEND;         *data++ = sD3DBLEND_ONE;
    *data++ = sD3DRS_BLENDOP;           *data++ = sD3DBLENDOP_REVSUBTRACT;
    break;
  case smBF_BLENDMUL2:
    *data++ = sD3DRS_ALPHABLENDENABLE;  *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;          *data++ = sD3DBLEND_DESTCOLOR;
    *data++ = sD3DRS_DESTBLEND;         *data++ = sD3DBLEND_SRCCOLOR;
    *data++ = sD3DRS_BLENDOP;           *data++ = sD3DBLENDOP_ADD;
    break;
  case smBF_BLENDDESTA:
    *data++ = sD3DRS_ALPHABLENDENABLE;  *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;          *data++ = sD3DBLEND_DESTALPHA;
    *data++ = sD3DRS_DESTBLEND;         *data++ = sD3DBLEND_ONE;
    *data++ = sD3DRS_BLENDOP;           *data++ = sD3DBLENDOP_ADD;
    break;
  case smBF_BLENDADDSA:
    *data++ = sD3DRS_ALPHABLENDENABLE;  *data++ = 1;
    *data++ = sD3DRS_SRCBLEND;          *data++ = sD3DBLEND_SRCALPHA;
    *data++ = sD3DRS_DESTBLEND;         *data++ = sD3DBLEND_ONE;
    *data++ = sD3DRS_BLENDOP;           *data++ = sD3DBLENDOP_ADD;
    break;
  }

  switch(flags & smBF_STENCILMASK)
  {
  default:
    *data++ = D3DRS_STENCILENABLE;      *data++ = 0;
    *data++ = D3DRS_TWOSIDEDSTENCILMODE; *data++ = 0;
    *data++ = D3DRS_STENCILFUNC;        *data++ = D3DCMP_ALWAYS;
    break;
  case smBF_STENCILINC:
  case smBF_STENCILINC|smBF_STENCILZFAIL:
    *data++ = D3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = D3DRS_TWOSIDEDSTENCILMODE; *data++ = 0;
    *data++ = D3DRS_STENCILFUNC;        *data++ = D3DCMP_ALWAYS;
    *data++ = D3DRS_STENCILFAIL;        *data++ = D3DSTENCILOP_KEEP;
    if(flags & smBF_STENCILZFAIL)
    {
      *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_INCR;
      *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_KEEP;
    }
    else
    {
      *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_KEEP;
      *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_INCR;
    }
    break;
  case smBF_STENCILDEC:
  case smBF_STENCILDEC|smBF_STENCILZFAIL:
    *data++ = D3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = D3DRS_TWOSIDEDSTENCILMODE; *data++ = 0;
    *data++ = D3DRS_STENCILFUNC;        *data++ = D3DCMP_ALWAYS;
    *data++ = D3DRS_STENCILFAIL;        *data++ = D3DSTENCILOP_KEEP;
    if(flags & smBF_STENCILZFAIL)
    {
      *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_DECR;
      *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_KEEP;
    }
    else
    {
      *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_KEEP;
      *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_DECR;
    }
    break;
  case smBF_STENCILTEST:
    *data++ = D3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = D3DRS_TWOSIDEDSTENCILMODE; *data++ = 0;
    *data++ = D3DRS_STENCILFUNC;        *data++ = D3DCMP_EQUAL;
    *data++ = D3DRS_STENCILFAIL;        *data++ = D3DSTENCILOP_KEEP;
    *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_KEEP;
    *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_KEEP;
    break;
  case smBF_STENCILCLR:
    *data++ = D3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = D3DRS_TWOSIDEDSTENCILMODE; *data++ = 0;
    *data++ = D3DRS_STENCILFUNC;        *data++ = D3DCMP_ALWAYS;
    *data++ = D3DRS_STENCILFAIL;        *data++ = D3DSTENCILOP_ZERO;
    *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_ZERO;
    *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_ZERO;
    break;
  case smBF_STENCILINDE:
  case smBF_STENCILINDE|smBF_STENCILZFAIL:
    *data++ = D3DRS_CULLMODE;           *data++ = D3DCULL_NONE;
    *data++ = D3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = D3DRS_TWOSIDEDSTENCILMODE; *data++ = 1;
    *data++ = D3DRS_STENCILFUNC;        *data++ = D3DCMP_ALWAYS;
    *data++ = D3DRS_CCW_STENCILFUNC;    *data++ = D3DCMP_ALWAYS;
    *data++ = D3DRS_STENCILFAIL;        *data++ = D3DSTENCILOP_KEEP;
    *data++ = D3DRS_CCW_STENCILFAIL;    *data++ = D3DSTENCILOP_KEEP;
    if(flags & smBF_STENCILZFAIL)
    {
      *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_DECR;
      *data++ = D3DRS_CCW_STENCILZFAIL;   *data++ = D3DSTENCILOP_INCR;
      *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_KEEP;
      *data++ = D3DRS_CCW_STENCILPASS;    *data++ = D3DSTENCILOP_KEEP;
    }
    else
    {
      *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_KEEP;
      *data++ = D3DRS_CCW_STENCILZFAIL;   *data++ = D3DSTENCILOP_KEEP;
      *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_INCR;
      *data++ = D3DRS_CCW_STENCILPASS;    *data++ = D3DSTENCILOP_DECR;
    }
    break;
  case smBF_STENCILRENDER:
    *data++ = D3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = D3DRS_TWOSIDEDSTENCILMODE; *data++ = 0;
    *data++ = D3DRS_STENCILFUNC;        *data++ = D3DCMP_EQUAL;
    *data++ = D3DRS_STENCILFAIL;        *data++ = D3DSTENCILOP_ZERO;
    *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_ZERO;
    *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_ZERO;
    break;
  }

  texuv1 = 0;
  for(i=0;i<4;i++)
  {
    static const sU32 filters[8][4] = 
    {
      { sD3DTEXF_POINT,sD3DTEXF_POINT,sD3DTEXF_NONE },
      { sD3DTEXF_LINEAR,sD3DTEXF_LINEAR,sD3DTEXF_NONE },
      { sD3DTEXF_LINEAR,sD3DTEXF_LINEAR,sD3DTEXF_LINEAR },
      { sD3DTEXF_LINEAR,sD3DTEXF_ANISOTROPIC,sD3DTEXF_LINEAR },
      { sD3DTEXF_POINT,sD3DTEXF_POINT,sD3DTEXF_POINT },
      { sD3DTEXF_POINT,sD3DTEXF_POINT,sD3DTEXF_NONE },
      { sD3DTEXF_POINT,sD3DTEXF_POINT,sD3DTEXF_NONE },
      { sD3DTEXF_POINT,sD3DTEXF_POINT,sD3DTEXF_NONE },
    };
    TFile[i] = -1;
    if(info.Tex[i]!=sINVALID)
    {
      flags = info.TFlags[i];
      tex = TCount*sD3DTSS_1;
      sam = TCount*sD3DSAMP_1;
      mtrl->TexUse[TCount] = i;
      TFile[i]=TCount++;

      *data++ = sam+sD3DSAMP_MAGFILTER;    *data++ = filters[flags&7][0];
      *data++ = sam+sD3DSAMP_MINFILTER;    *data++ = filters[flags&7][1];
      *data++ = sam+sD3DSAMP_MIPFILTER;    *data++ = filters[flags&7][2];
      *data++ = sam+sD3DSAMP_ADDRESSU;     *data++ = (flags&smTF_CLAMP) ? sD3DTADDRESS_CLAMP : sD3DTADDRESS_WRAP;
      *data++ = sam+sD3DSAMP_ADDRESSV;     *data++ = (flags&smTF_CLAMP) ? sD3DTADDRESS_CLAMP : sD3DTADDRESS_WRAP;

      if(flags & smTF_UV1)
        texuv1 = 1;

      if(info.ShaderLevel <= sPS_14)
      {
        *data++ = tex+sD3DTSS_TEXTURETRANSFORMFLAGS;
        *data++ = (flags & smTF_PROJECTIVE) ? sD3DTTFF_PROJECTED : 0;
      }
    }
  }

// what do we need?

  for(i=0;i<sMCS_MAX;i++)
  {
    flags = info.Combiner[i];
    combinermask |= psatoc[flags&15];
    if(flags & 0xff0)
      combinermask |= 1<<i;
  }
  flags = info.AlphaCombiner;
  combinermask |= psatoc[flags&15];
  combinermask |= psatoc[(flags>>4)&15];

  // Register Allocation

  switch(lightmode)
  {
  case sMLF_BUMP:
  case sMLF_BUMPX:
  case sMLF_BUMPXSPEC:
    if(lightmode == sMLF_BUMP || TFile[1] != -1)
      combinermask |= 1<<sMCS_TEX1;
  }

// *** Vertex Shader Startup

  *vs++ = XO_VS0101;
  *vs++ = XO_DCL; *vs++ = XD_POSITION;   *vs++ = XDALL|X_V|0;

  if(info.BaseFlags & smBF_SHADOWMASK)
  {
    // calc position
    *vs++ = XO_MUL;
    *vs++ = XDALL|X_R|3;
    *vs++ = XSALL|X_V|0;
    *vs++ = XS_W |X_V|0;

    *vs++ = XO_ADD;
    *vs++ = XDALL|X_R|0;
    *vs++ = XS_W |X_C|20;
    *vs++ = XS_W |XS_NEG|X_V|0;

    *vs++ = XO_ADD;
    *vs++ = XDALL|X_R|1;
    *vs++ = XSALL|X_V|0;
    *vs++ = XSALL|XS_NEG|X_C|24;

    *vs++ = XO_MAD;
    *vs++ = XDALL|X_R|3;
    *vs++ = XSALL|X_R|1;
    *vs++ = XSALL|X_R|0;
    *vs++ = XSALL|X_R|3;
  }
  else
  {
    i = 1;
    if(!(info.BaseFlags & smBF_NOTEXTURE))
    {
      i = 2;
      *vs++ = XO_DCL; *vs++ = XD_TEXCOORD0;  *vs++ = XDALL|X_V|1;
    }
    if(!(info.BaseFlags & smBF_NONORMAL))
    {
      i = 4;
      *vs++ = XO_DCL; *vs++ = XD_NORMAL;    *vs++ = XDALL|X_V|2;
      *vs++ = XO_DCL; *vs++ = XD_TANGENT;   *vs++ = XDALL|X_V|3;
    }
    if(texuv1)
    {
      *vs++ = XO_DCL; *vs++ = XD_TEXCOORD1;  *vs++ = XDALL|X_V|i;
      texuv1 = i;
      i++;
    }
    if((combinermask & (1<<sMCS_VERTEX)))
    {
      if(info.SpecialFlags & sMSF_LONGCOLOR)
      {
        *vs++ = XO_DCL; *vs++ = XD_NORMAL;     *vs++ = XDALL|X_V|i;
        *vs++ = XO_MUL;
        *vs++ = XDALL|X_OCOLOR|0;
        *vs++ = XSALL|X_V|i;
        *vs++ = XS_W|X_C|19;
      }
      else
      {
        *vs++ = XO_DCL; *vs++ = XD_COLOR0;     *vs++ = XDALL|X_V|i;
        *vs++ = XO_MOV;
        *vs++ = XDALL|X_OCOLOR|0;
        *vs++ = XSALL|X_V|i;
      }
    }
    *vs++ = XO_MOV;
    *vs++ = XDALL|X_R|3;
    *vs++ = XSALL|X_V|0;
  }

  *vs++ = XO_M4x4;
  *vs++ = XDALL|X_OPOS;
  *vs++ = XSALL|X_R|3;
  *vs++ = XSALL|X_C|8;

  if(!(info.BaseFlags & smBF_NONORMAL))
  {
    // unpack packed normal/tangent and calculate binormal
    *vs++ = XO_MUL;
    *vs++ = XD_XYZ|X_R|8;
    *vs++ = XSALL|X_V|2;
    *vs++ = XS_W|X_C|19;

    *vs++ = XO_MUL;
    *vs++ = XD_Z|X_R|10;
    *vs++ = XS_W|X_V|2;
    *vs++ = XS_W|X_C|19;

    *vs++ = XO_MUL;
    *vs++ = XD_XY|X_R|10;
    *vs++ = XSALL|X_V|3;
    *vs++ = XS_W|X_C|19;

    *vs++ = XO_MUL;
    *vs++ = XD_XYZ|X_R|9;
    *vs++ = XS_YZXW|X_R|8;
    *vs++ = XS_ZXYW|X_R|10;
    
    *vs++ = XO_MAD;
    *vs++ = XD_XYZ|X_R|9;
    *vs++ = XS_ZXYW|X_R|8|XS_NEG;
    *vs++ = XS_YZXW|X_R|10;
    *vs++ = XSALL|X_R|9;
  }

// vertex shader: texture coordinates

  for(i=0;i<4;i++)
  {
    if(combinermask&psttoc[i])
    {
      RFile[RTex0+i] = RCount++;
      j = XSALL|X_V|1;
      if(info.TFlags[i] & smTF_UV1)
        j = XSALL|X_V|texuv1;
      if(i==2)
      {
        switch(info.SpecialFlags&sMSF_ENVIMASK)
        {
        case sMSF_ENVIREFLECT:
          {
            *vs++ = XO_M4x3;         // wasted calculation !!!
            *vs++ = XD_XYZ|X_R|1;     // r0 = e
            *vs++ = XSALL|X_R|3;
            *vs++ = XSALL|X_C|4;
            vs_nrm(vs,0,XSALL|X_R|1);

            *vs++ = XO_M3x3;        // wasted calculation !!!
            *vs++ = XD_XYZ|X_R|1;   // r1 = n
            *vs++ = XSALL|X_R|8;
            *vs++ = XSALL|X_C|4;

            *vs++ = XO_DP3;         // r2 = n*e
            *vs++ = XD_W|X_R|2;
            *vs++ = XSALL|X_R|0;
            *vs++ = XSALL|X_R|1;

            *vs++ = XO_ADD;         // r2 = 2*(n*e)
            *vs++ = XD_W|X_R|2;
            *vs++ = XS_W|X_R|2;
            *vs++ = XS_W|X_R|2;

            *vs++ = XO_MAD;         // r2 = 2*(n*e)*n-e
            *vs++ = XD_XYZ|X_R|2;
            *vs++ = XS_W|X_R|2;
            *vs++ = XSALL|X_R|1;
            *vs++ = XSALL|X_R|0|XS_NEG;
          
            vs_nrm(vs,1,XSALL|X_R|2);
          }
          goto ENVI1;

        case sMSF_ENVISPHERE:
          *vs++ = XO_M3x2;        // wasted calculation !!!
          *vs++ = XD_XY|X_R|1;
          *vs++ = XSALL|X_R|8;
          *vs++ = XSALL|X_C|4;

        ENVI1:
          *vs++ = XO_MAD;
          *vs++ = XD_XY|X_R|1;
          *vs++ = XSALL|X_R|1;
          *vs++ = XSALL|X_C|20;
          *vs++ = XS_X|X_C|20;

          j = XSALL|X_R|1;
          break;
        }
      }
      if(i==3)
      {
        switch(info.SpecialFlags&sMSF_PROJMASK)
        {
        case sMSF_PROJMODEL:
          j = XSALL|X_R|3;
          break;
        case sMSF_PROJWORLD:
          j = XSALL|X_R|1;
          *vs++ = XO_M4x3;
          *vs++ = XD_XYZ|X_R|1;
          *vs++ = XSALL|X_R|3;
          *vs++ = XSALL|X_C|0;
          break;
        case sMSF_PROJEYE:
          j = XSALL|X_R|1;
          *vs++ = XO_M4x3;
          *vs++ = XD_XYZ|X_R|1;
          *vs++ = XSALL|X_R|3;
          *vs++ = XSALL|X_C|4;
          break;
        case sMSF_PROJSCREEN:
          j = XSALL|X_R|1;
          *vs++ = XO_M4x4;
          *vs++ = XDALL|X_R|1;
          *vs++ = XSALL|X_R|3;
          *vs++ = XSALL|X_C|28;
          break;
        }
      }
      switch(info.TFlags[i]&smTF_TRANSMASK)
      {
      case smTF_DIRECT:
        *vs++ = XO_MOV;
        *vs++ = ((info.TFlags[i]&smTF_PROJECTIVE)?XDALL:XD_XY)|X_OUV|RFile[RTex0+i];
        *vs++ = j;
        break;
      case smTF_SCALE:
        *vs++ = XO_MUL;
        *vs++ = XD_XY|X_OUV|RFile[RTex0+i];
        *vs++ = j;
        *vs++ = vsswizzles[i]|X_C|13;
        break;
      case smTF_TRANS1:
        *vs++ = XO_DP4;
        *vs++ = XD_X|X_R|0;
        *vs++ = j;
        *vs++ = XSALL|X_C|16;
        *vs++ = XO_DP4;
        *vs++ = XD_Y|X_R|0;
        *vs++ = j;
        *vs++ = XSALL|X_C|17;
        *vs++ = XO_MOV;
        *vs++ = XD_XY|X_OUV|RFile[RTex0+i];
        *vs++ = XSALL|X_R|0;
        break;
      case smTF_TRANS2:
        *vs++ = XO_DP4;
        *vs++ = XD_X|X_R|0;
        *vs++ = j;
        *vs++ = XSALL|X_C|14;
        *vs++ = XO_DP4;
        *vs++ = XD_Y|X_R|0;
        *vs++ = j;
        *vs++ = XSALL|X_C|15;
        *vs++ = XO_MOV;
        *vs++ = XD_XY|X_OUV|RFile[RTex0+i];
        *vs++ = XSALL|X_R|0;
        break;
      }
    }
  }

  static sU32 newreg[sMCS_MAX] =
  {
    XSALL|X_R|1,  // light
    XSALL|X_C|0,  // col0: diffuse
    XSALL|X_C|1,  // col1: ambient
    ~0,           // tex0
    ~0,           // tex1
    ~0,           // merge
    XS_W|X_R|1,   // spec
    XSALL|X_C|2,  // col2
    XSALL|X_C|3,  // col3
    ~0,           // tex2
    ~0,           // tex3
    XSALL|X_V|0,  // vertex color
    ~0,           // final
  };
  static const sU32 psalpha13[16] =
  {
    XS_W|X_C|6, XS_Z|X_R|1, XS_W|X_R|1, XS_W|X_C|6,
    XS_W|X_T|0, XS_W|X_T|1, XS_W|X_T|2, XS_W|X_T|3,
    XS_W|X_C|0, XS_W|X_C|1, XS_W|X_C|2, XS_W|X_C|3,
    XS_W|X_C|7, XS_Z|X_C|6,         ~0,         ~0,
  };
  sInt oldreg;
  sU32 cocmd[8][8];   // coissued alpha cmd's
  sInt cocount;       // write index
  sInt coindex;       // read index
  sInt reg;

  // start pixel shader and sample all textures

  newreg[sMCS_TEX0] = XSALL|X_T|TFile[0];
  newreg[sMCS_TEX1] = XSALL|X_T|TFile[1];
  newreg[sMCS_TEX2] = XSALL|X_T|TFile[2];
  newreg[sMCS_TEX3] = XSALL|X_T|TFile[3];

  cocount = 0;
  coindex = 0;
  *ps++ = XO_PS0103;
  for(i=0;i<4;i++)
  {
    if(combinermask & psttoc[i])
    {
      if(info.Tex[i]==sINVALID) 
        error = 1;
      if(TFile[i]==-1)
        error = 1;
      *ps++ = XO_TEX13;
      *ps++ = XDALL|X_T|TFile[i];
    }
  }

  // if there is light, then we need a normal

  if(lightmode)
  {
    if(TCount>=4)
      error = 1;
    *ps++ = XO_TEX13;
    *ps++ = XDALL|X_T|TCount;
    mtrl->TexUse[TCount] = -2;
    sInt sam = TCount*sD3DSAMP_1;
    *data++ = sam+sD3DSAMP_MAGFILTER;    *data++ = sD3DTEXF_LINEAR;
    *data++ = sam+sD3DSAMP_MINFILTER;    *data++ = sD3DTEXF_LINEAR;
    *data++ = sam+sD3DSAMP_MIPFILTER;    *data++ = sD3DTEXF_NONE;
    *data++ = sam+sD3DSAMP_ADDRESSU;     *data++ = (lightmode == sMLF_BUMPXSPEC) ? sD3DTADDRESS_CLAMP : sD3DTADDRESS_WRAP;
    *data++ = sam+sD3DSAMP_ADDRESSV;     *data++ = sD3DTADDRESS_WRAP;
    *data++ = TCount*sD3DTSS_1+sD3DTSS_TEXTURETRANSFORMFLAGS; *data++ = 0;
  }
  oldreg = 0;
  j = 0;

  switch(lightmode)
  {
  case sMLF_BUMPX:
  case sMLF_BUMPXSPEC:
    if(TFile[3] != -1) // we need the shadow texture
    {
      static sU32 attenreg[4*2] =
      {
        XS_X|X_C|26, XS_Y|X_C|26,
        XS_Z|X_C|26, XS_W|X_C|26,
        XS_X|X_C|27, XS_Y|X_C|27,
        XS_Z|X_C|27, XS_W|X_C|27,
      };

      if(lightmode == sMLF_BUMPXSPEC) // need camera direction
      {
        *vs++ = XO_ADD;         // camera direction: distance
        *vs++ = XD_XYZ|X_R|3;
        *vs++ = XSALL|X_C|19;
        *vs++ = XSALL|X_V|0|XS_NEG;

        vs_nrm(vs,7,XSALL|X_R|3); // r7 = camera dir, normalized, model space
      }

      for(i=0;i<4;i++) // for all 4 lights
      {
        *vs++ = XO_ADD;         // distance
        *vs++ = XD_XYZ|X_R|i;
        *vs++ = XSALL|X_C|21+i;
        *vs++ = XSALL|X_V|0|XS_NEG;

        *vs++ = XO_DP3;         // normalize
        *vs++ = XD_W|X_R|i;
        *vs++ = XSALL|X_R|i;
        *vs++ = XSALL|X_R|i;

        *vs++ = XO_RSQ;
        *vs++ = XD_W|X_R|4;
        *vs++ = XS_W|X_R|i;

        *vs++ = XO_MUL;         // ri = light dir, normalized, model space
        *vs++ = XDALL|X_R|i;
        *vs++ = XSALL|X_R|i;
        *vs++ = XS_W|X_R|4;

        *vs++ = XO_MAD;         // attenuation
        *vs++ = (XD_X<<i)|X_R|6;
        *vs++ = XS_W|X_R|i;
        *vs++ = attenreg[i*2+0];
        *vs++ = attenreg[i*2+1];

        if(lightmode == sMLF_BUMPXSPEC)
        {
          *vs++ = XO_ADD;       // halfway vector
          *vs++ = XD_XYZ|X_R|i;
          *vs++ = XSALL|X_R|i;
          *vs++ = XSALL|X_R|7;

          *vs++ = XO_DP3;       // normalize
          *vs++ = XD_W|X_R|i;
          *vs++ = XSALL|X_R|i;
          *vs++ = XSALL|X_R|i;

          *vs++ = XO_RSQ;
          *vs++ = XD_W|X_R|i;
          *vs++ = XS_W|X_R|i;

          *vs++ = XO_MUL;
          *vs++ = XDALL|X_R|i;
          *vs++ = XSALL|X_R|i;
          *vs++ = XS_W|X_R|i;
        }

        *vs++ = XO_DP3;         // N.L (or N.H for specular)=light
        *vs++ = (XD_X<<i)|X_R|5;
        *vs++ = XSALL|X_R|i;
        *vs++ = XSALL|X_R|8;

        // diffuse: 6 instrs/light=24 for 4 lights
        // specular: 10 instrs/light=40 for 4 lights
      }

      *vs++ = XO_MAD;           // rescale N.L contributions
      *vs++ = XDALL|X_R|5;
      *vs++ = XSALL|X_R|5;
      *vs++ = XS_X|X_C|20;
      *vs++ = XS_X|X_C|20;

      *vs++ = XO_MAX;           // clamp attenuation values
      *vs++ = XDALL|X_R|6;
      *vs++ = XSALL|X_R|6;
      *vs++ = XS_Z|X_C|20;

      *vs++ = XO_MUL;           // (N.L*attenuation)=intensity
      *vs++ = XDALL|X_R|5;
      *vs++ = XSALL|X_R|5;
      *vs++ = XSALL|X_R|6;

      *vs++ = XO_MUL;           // intensity*colorbright=weights
      *vs++ = XDALL|X_R|5;
      *vs++ = XSALL|X_R|5;
      *vs++ = XSALL|X_C|25;

      *vs++ = XO_MUL;           // calc weighted L vector
      *vs++ = XD_XYZ|X_R|0;
      *vs++ = XSALL|X_R|0;
      *vs++ = XS_X|X_R|5;

      for(i=1;i<4;i++)
      {
        *vs++ = XO_MAD;
        *vs++ = XD_XYZ|X_R|0;
        *vs++ = XSALL|X_R|i;
        *vs++ = (XS_Y*i)|X_R|5;
        *vs++ = XSALL|X_R|0;
      }

      *vs++ = XO_DP3;         // normalize weighted L vector
      *vs++ = XD_W|X_R|0;
      *vs++ = XSALL|X_R|0;
      *vs++ = XSALL|X_R|0;

      *vs++ = XO_RSQ;
      *vs++ = XD_W|X_R|0;
      *vs++ = XS_W|X_R|0;

      *vs++ = XO_MUL;
      *vs++ = XDALL|X_R|0;
      *vs++ = XSALL|X_R|0;
      *vs++ = XS_W|X_R|0;

      *vs++ = XO_M3x3;        // to texture space with it
      *vs++ = XD_XYZ|X_OUV+TCount;
      *vs++ = XSALL|X_R|0;
      *vs++ = XSALL|X_R|8;

      *vs++ = XO_MOV;         // set shadow light intensities
      *vs++ = XDALL|X_OCOLOR|0;
      *vs++ = XS_WZYX|X_R|6;

      *vs++ = XO_MUL;         // calc nonshadow light intensity
      *vs++ = XDALL|X_OCOLOR|1;
      *vs++ = XS_W|X_R|6;
      *vs++ = XSALL|X_C|18;

      // diffuse: 40 slots
      // specular: 60 slots
      // ---

      // pixel shader
      if(lightmode == sMLF_BUMPX)
      {
        *ps++ = XO_MOV;       
        *ps++ = XD_XYZ|X_R|0;
        *ps++ = XSALL|X_V|1;

        *ps++ = XO_MUL|XO_CO;
        *ps++ = XD_W|X_R|0;
        *ps++ = XSALL|X_T|TFile[3];
        *ps++ = XSALL|X_V|0;

        *ps++ = XO_MUL;
        *ps++ = XD_XYZ|X_R|1;
        *ps++ = XSALL|X_T|TFile[3];
        *ps++ = XSALL|X_V|0;

        *ps++ = XO_MUL|XO_CO;
        *ps++ = XD_W|X_R|1;
        *ps++ = XS_Z|X_T|TFile[3];
        *ps++ = XS_Z|X_V|0;

        *ps++ = XO_MAD;
        *ps++ = XD_XYZ|X_R|0;
        *ps++ = XSALL|X_C|0;
        *ps++ = XS_W|X_R|0;
        *ps++ = XSALL|X_R|0;

        *ps++ = XO_DP3;
        *ps++ = XD_XYZ|X_R|1;
        *ps++ = XSALL|X_R|1;
        *ps++ = XSALL|X_C|4;

        *ps++ = XO_MAD;
        *ps++ = XD_XYZ|X_R|0;
        *ps++ = XSALL|X_C|1;
        *ps++ = XS_W|X_R|1;
        *ps++ = XSALL|X_R|0;

        *ps++ = XO_MOV|XO_CO;
        *ps++ = XD_W|X_R|0;
        *ps++ = XS_Z|X_R|1;

        *ps++ = XO_MAD;
        *ps++ = XD_XYZ|X_R|0;
        *ps++ = XSALL|X_C|2;
        *ps++ = XS_W|X_R|0;
        *ps++ = XSALL|X_R|0;

        *ps++ = XO_DP3;
        *ps++ = XD_XYZ|X_R|1|XD_SAT;
        if(TFile[1] != -1)
          *ps++ = XSALL|X_T|TFile[1];
        else
          *ps++ = XSALL|X_C|3;
        *ps++ = XSALL|X_T|TCount|XS_SIGN;
        
        *ps++ = XO_MUL;
        *ps++ = XD_XYZ|X_R|0;
        *ps++ = XSALL|X_R|0;
        *ps++ = XSALL|X_R|1;
      }
      else // specular path
      {
        *ps++ = XO_DP3;       // specular dot
        *ps++ = XD_XYZ|X_R|0|XD_SAT;
        if(TFile[1] != -1)
          *ps++ = XSALL|X_T|TFile[1];
        else
          *ps++ = XSALL|X_C|3;
        *ps++ = XSALL|X_T|TCount|XS_SIGN;

        *ps++ = XO_MUL|XO_CO; // lights*shadow (light4)
        *ps++ = XD_W|X_R|1;
        *ps++ = XSALL|X_T|TFile[3];
        *ps++ = XSALL|X_V|0;

        *ps++ = XO_MUL;       // lights*shadow
        *ps++ = XD_XYZ|X_R|1;
        *ps++ = XSALL|X_T|TFile[3];
        *ps++ = XSALL|X_V|0;

        *ps++ = XO_MUL|XO_CO; // dot^2
        *ps++ = XD_W|X_R|0;
        *ps++ = XS_Z|X_R|0;
        *ps++ = XS_Z|X_R|0;

        *ps++ = XO_DP3;       // first 3 lights intensity
        *ps++ = XD_XYZ|X_R|0;
        *ps++ = XSALL|X_R|1;
        *ps++ = XSALL|X_C|0;

        if(info.SpecPower > 8.0f)
        {
          *ps++ = XO_MUL|XO_CO; // dot^4
          *ps++ = XD_W|X_R|0;
          *ps++ = XSALL|X_R|0;
          *ps++ = XSALL|X_R|0;
        }

        *ps++ = XO_MUL;       // dot^8
        *ps++ = XD_XYZ|X_R|1;
        *ps++ = XS_W|X_R|0;
        *ps++ = XS_W|X_R|0;

        *ps++ = XO_MAD|XO_CO; // all 4 lights intensity
        *ps++ = XD_W|X_R|1;
        *ps++ = XSALL|X_R|1;
        *ps++ = XSALL|X_C|0;
        *ps++ = XS_Z|X_R|0;

        *ps++ = XO_MUL;       // dot^16
        *ps++ = XD_XYZ|X_R|1;
        *ps++ = XSALL|X_R|1;
        *ps++ = XSALL|X_R|1;

        if(TFile[1] != -1 && (info.SpecialFlags & sMSF_SPECMAP))
        {
          *ps++ = XO_MUL|XO_CO; // specularity*intensity
          *ps++ = XD_W|X_R|1;
          *ps++ = XSALL|X_R|1;
          *ps++ = XSALL|X_T|TFile[1];
        }

        if(info.SpecPower > 16.0f)
        {
          *ps++ = XO_MUL;       // dot^32
          *ps++ = XD_XYZ|X_R|0;
          *ps++ = XSALL|X_R|1;
          *ps++ = XSALL|X_R|1;
        }
        else
        {
          *ps++ = XO_MOV;       // dot^16
          *ps++ = XD_XYZ|X_R|0;
          *ps++ = XSALL|X_R|1;
        }

        *ps++ = XO_MUL;       // specularity*intensity*dot^32
        *ps++ = XD_XYZ|X_R|0;
        *ps++ = XSALL|X_R|0;
        *ps++ = XS_W|X_R|1;
      }
    }
    break;
  }

  // alpha to co-buffer
  val = info.AlphaCombiner;
  f0 = val&15;
  f0 = psalpha13[f0];
  if(val&sMCA_INVERTA)
    f0 |= XS_COMP;
  f1 = (val>>4)&15;
  if(f1)
  {
    f1 = psalpha13[f1];
    if(val&sMCA_INVERTB)
      f1 |= XS_COMP;
    cocmd[cocount][0] = 4;
    cocmd[cocount][1] = XO_MUL;     
    cocmd[cocount][2] = XD_W|X_R+0;       
    cocmd[cocount][3] = f0;          
    cocmd[cocount][4] = f1;
    cocount++;
  }
  else
  {
    cocmd[cocount][0] = 3;
    cocmd[cocount][1] = XO_MOV;     
    cocmd[cocount][2] = XD_W|X_R+0;       
    cocmd[cocount][3] = f0;          
    cocount++;
  }      

  if(lightmode != sMLF_BUMPX && lightmode != sMLF_BUMPXSPEC)
  {
    // combiner (half of it, at least)

    for(i=0;i<sMCS_MAX && j<8;i++)
    {
      reg = newreg[i];
      if(reg==~0) continue; // skip unsupported entries
      if((info.Combiner[i]&0x00f) && (info.Combiner[i]&0xf0)!=sMCOA_LERP)
      {
        reg = 1;
        if(oldreg==(XSALL|X_R|1))
          reg = 0;
        *ps++ = XO_MUL;
        *ps++ = XD_XYZ|X_R|reg;
        *ps++ = newreg[i];
        *ps++ = psalpha13[info.Combiner[i]&0x00f];
        reg = (XSALL|X_R|reg);
      }

      switch(info.Combiner[i]&0x0f0)
      {
      default:                    // skip unsupported modes silently...
        break;
      case sMCOA_SET:
        oldreg = reg;
        break;
      case sMCOA_ADD:
        *ps++ = XO_ADD;
        *ps++ = XD_XYZ|X_R|0;
        *ps++ = oldreg;
        *ps++ = reg;
        oldreg = XSALL|X_R|0;
        j++;
        if(coindex<cocount) cothread(ps,cocmd[coindex++],XO_CO);
        break;
      case sMCOA_SUB:
        *ps++ = XO_SUB;
        *ps++ = XD_XYZ|X_R|0;
        *ps++ = oldreg;
        *ps++ = reg;
        oldreg = XSALL|X_R|0;
        j++;
        if(coindex<cocount) cothread(ps,cocmd[coindex++],XO_CO);
        break;
      case sMCOA_MUL:
      case sMCOA_MUL2:
      case sMCOA_MUL4:
        *ps++ = XO_MUL;
        *ps++ = XD_XYZ|X_R|0|(XD_2X*(((info.Combiner[i]&0x0f0)-sMCOA_MUL)>>4));
        *ps++ = oldreg;
        *ps++ = reg;
        oldreg = XSALL|X_R|0;
        j++;
        if(coindex<cocount) cothread(ps,cocmd[coindex++],XO_CO);
        break;
      case sMCOA_MUL8:
        *ps++ = XO_MUL;
        *ps++ = XD_XYZ|X_R|0|XD_4X;
        *ps++ = oldreg;
        *ps++ = reg;
        oldreg = XSALL|X_R|0;
        j++;
        if(coindex<cocount) cothread(ps,cocmd[coindex++],XO_CO);
        *ps++ = XO_ADD;
        *ps++ = XD_XYZ|X_R|0;
        *ps++ = XSALL|X_R|0;
        *ps++ = XSALL|X_R|0;
        break;
      case sMCOA_SMOOTH:
        *ps++ = XO_MAD;
        *ps++ = XD_XYZ|X_R|0;
        *ps++ = oldreg;
        *ps++ = reg|XS_COMP;
        *ps++ = reg;
        oldreg = XSALL|X_R|0;
        j++;
        if(coindex<cocount) cothread(ps,cocmd[coindex++],XO_CO);
        break;
      case sMCOA_LERP:
        if(!(info.Combiner[i]&0x00f)) error = 1;
        *ps++ = XO_LRP;
        *ps++ = XD_XYZ|X_R|0;
        *ps++ = psalpha13[info.Combiner[i]&0x00f];
        if(info.Combiner[i] & sMCA_INVERTA)
          ps[-1] |= XS_COMP;
        *ps++ = oldreg;
        *ps++ = reg;
        oldreg = XSALL|X_R|0;
        j++;
        if(coindex<cocount) cothread(ps,cocmd[coindex++],XO_CO);
        break;
      case sMCOA_RSUB:
        *ps++ = XO_SUB;
        *ps++ = XD_XYZ|X_R|0;
        *ps++ = reg;
        *ps++ = oldreg;
        oldreg = XSALL|X_R|0;
        j++;
        if(coindex<cocount) cothread(ps,cocmd[coindex++],XO_CO);
        break;
      case sMCOA_DP3:
        *ps++ = XO_DP3;
        *ps++ = XD_XYZ|X_R|0;
        *ps++ = reg;
        *ps++ = oldreg;
        oldreg = XSALL|X_R|0;
        j++;
        if(coindex<cocount) cothread(ps,cocmd[coindex++],XO_CO);
        break;
      }
    }

    if(oldreg!=(XSALL|X_R|0))
    {
      *ps++ = XO_MOV;
      *ps++ = XD_XYZ|X_R|0;
      *ps++ = oldreg;
      if(coindex<cocount) cothread(ps,cocmd[coindex++],XO_CO);
    }

    while(coindex<cocount) cothread(ps,cocmd[coindex++],0); // remaining alpha-ops without co-issue
  }
  else
  {
    if(coindex<cocount) cothread(ps,cocmd[coindex++],XO_CO);
    if(coindex<cocount) error = 1;
  }

//*** finish shaders

  *data++ = ~0;
  *data++ = ~0;
  mtrl->VS = 0;
  mtrl->PS = 0;
  if(!error)
  {
    if(vs!=vsb)
    {
      *vs++ = XO_END;
      hr = DXDev->CreateVertexShader((DWORD *)vsb,&mtrl->VS);
      if(FAILED(hr))
        error = 1;
    }
    if(ps!=psb)
    {
      *ps++ = XO_END;
      hr = DXDev->CreatePixelShader((DWORD *)psb,&mtrl->PS);
      if(FAILED(hr))
        error = 1;
    }
  }
  if(error)
  {
    if(mtrl->VS)
      mtrl->VS->Release();
    if(mtrl->PS)
      mtrl->PS->Release();
    mtrl->Info.Init();
    mid = sINVALID;
  }

  return mid;
}

#endif

void sSystem_::MtrlAddRef(sInt mid)
{
  sMaterial *mtrl;
  if(mid==sINVALID) return;
  sVERIFY(mid>=0 && mid<MAX_MATERIALS);
  mtrl = &Materials[mid];
  sVERIFY(mtrl->RefCount>0);
  mtrl->RefCount++;
}

void sSystem_::MtrlRem(sInt mid)
{
  sMaterial *mtrl;
  HRESULT hr;

  sVERIFY(mid>=0 && mid<MAX_MATERIALS);
  mtrl = &Materials[mid];
  sVERIFY(mtrl->RefCount>0);
  mtrl->RefCount--;
  if(mtrl->RefCount==0)
  {
    if(mtrl->VS)
    {
      hr = mtrl->VS->Release();
      DXERROR(hr);
    }
    if(mtrl->PS)
    {
      mtrl->PS->Release();
      DXERROR(hr);
    }
  }
}

void sSystem_::MtrlSet(sInt mid,sMaterialInfo *info,const sMaterialEnv *env)
{
  sMaterial *mtrl;
  sInt i,j,lightmode;
  const sFastLight *light;
  HRESULT hr;
  sMatrix vc[8],tmp;
  sVector pc[8];
  sVector v;
  sF32 fc,fs;

  // set environment

  if(env)
  {
    sMatrix cmat,pmat,mat;

    env->MakeProjectionMatrix(pmat);
    cmat = env->CameraSpace;
    cmat.TransR();
    mat = env->ModelSpace;
    mat.TransR();

    LastProjection = pmat;
    LastCamera = cmat;
    LastMatrix = mat;
    LastEnv = *env;
    LastTransform.Mul4(LastMatrix,LastCamera);
  }
  else
    env = &LastEnv;

  // check mateterial

  if(mid==sINVALID)    // just set env!
    return;

  sVERIFY(mid>=0 && mid<MAX_MATERIALS);
  mtrl = &Materials[mid];
  sVERIFY(mtrl->RefCount>0);

  // set shader constants

  fs = sFSin(info->SRT2[2]);
  fc = sFCos(info->SRT2[2]);
  sSetMem(vc,0,sizeof(vc));
  sSetMem(pc,0,sizeof(pc));

  lightmode = info->LightFlags & sMLF_MODEMASK;

  vc[0] = env->ModelSpace;            // c0: model -> world
  vc[1].Mul4(env->ModelSpace,LastCamera);   // c4: model -> eye
  vc[2].Mul4(vc[1],LastProjection);             // c8: model -> screen
  vc[3].i = env->LightPos;            // c12: light pos (model space)
  vc[3].i.Rotate34(LastMatrix);
  vc[3].j.Init(info->TScale[0],info->TScale[1],info->TScale[2],info->TScale[3]);    // c13: all Texture Scales
  vc[3].k.Init( info->SRT2[0]*fc,info->SRT2[1]*fs,0,info->SRT2[3]);   // c14: SRT 2(x)
  vc[3].l.Init(-info->SRT2[0]*fs,info->SRT2[1]*fc,0,info->SRT2[4]);   // c15: SRT 2(y)
  vc[4].InitSRT(info->SRT1);          // c16: SRT1
  vc[4].Trans4();
  vc[4].k = env->LightPos;            // c18: light pos (eye space)
  vc[4].k.Rotate34(LastCamera);
  vc[4].l = env->CameraSpace.l;       // c19: camera pos (model space)
  vc[4].l.Rotate34(LastMatrix);
  vc[4].l.w = 1.0f / 32767.0f;
  vc[5].i.Init(0.5f,-0.5f,0,1);       // c20: envi scaler
  vc[5].j = env->LightPos;            // c21: light direction (model space)
  vc[5].j.Unit3();
  vc[5].j.Rotate3(LastMatrix);
  vc[5].k.InitColor(info->Color[0]);  // c22: diffuse, for vs-light
  vc[5].l.InitColor(info->Color[1]);  // c23: ambient, for vs-light
  vc[6].i = env->ModelLightPos;       // c24: light pos (model space)
  vc[6].i.w = 0;
  tmp.Init();
  tmp.i.x = 0.5f;
  tmp.j.y = -0.5f;
  tmp.l.x = 0.5f;
  tmp.l.y = 0.5f;
  vc[7].Mul4(vc[2],tmp);              // c28: model -> screen [0;1]

  if(lightmode == sMLF_BUMPX || lightmode == sMLF_BUMPXSPEC)
  {
    for(i=0;i<4;i++)
    {
      light = &env->FastLights[i];

      (&vc[5].j)[i] = light->Pos;       // c21-c24: positions (model space)
      (&vc[5].j)[i].Rotate34(LastMatrix);
      if(light->Range > 0.001f)         // c26/c27: attenuation
      {
        (&vc[6].k.x)[i*2+0] = -light->Amplify / light->Range;
        (&vc[6].k.x)[i*2+1] = light->Amplify;
      }
      else
      {
        (&vc[6].k.x)[i*2+0] = 0.0f;
        (&vc[6].k.x)[i*2+1] = light->Amplify;
      }
      if(lightmode == sMLF_BUMPX)       // c25: colorbright
        (&vc[6].j.x)[i] = sMax(light->Color.x,sMax(light->Color.y,light->Color.z));
      else
        (&vc[6].j.x)[i] = light->Color.w;
    }

    v.InitColor(info->Color[0]);
    vc[4].k.Mul4(env->FastLights[3].Color,v); // c18: fastlight color
  }

  if(lightmode != sMLF_BUMPX && lightmode != sMLF_BUMPXSPEC)
  {
    pc[0].InitColor(info->Color[0]);
    pc[1].InitColor(info->Color[1]);
    pc[2].InitColor(info->Color[2]);
    pc[3].InitColor(info->Color[3]);
    pc[4] = env->LightPos;          // c4: light direction (eye space)
    pc[4].Unit3();
    pc[4].Rotate3(LastCamera);
    pc[4].w = info->SpecPower;      // c4.w: spec power
    pc[5] = env->LightPos;          // c5: light position (eye space)
    pc[5].Rotate4(LastCamera);
    pc[5].w = 1.0f/env->LightRange; // c5.w: light range
    pc[6].Init(0.5,-0.5,0,1);       // c6: constantes
    pc[7].Init(1,0,0,0.5);          // c7: 0.5 for PS13 (which can't really swizzle) ; 1 0 0 vector for ps20 bump fade

    if(info->LightFlags&sMLF_DIFFUSE)
    {
      v.InitColor(env->LightDiffuse);
      v.Scale3(env->LightAmplify);
      pc[0].Mul4(v);
      vc[5].k.Mul4(v);
    }
  }
  else
  {
    v.InitColor(info->Color[0]);
    pc[0].Mul4(env->FastLights[0].Color,v);
    pc[1].Mul4(env->FastLights[1].Color,v);
    pc[2].Mul4(env->FastLights[2].Color,v);
    if(lightmode == sMLF_BUMPXSPEC)
    {
      pc[0].z = pc[1].w;
      pc[0].y = pc[2].w;
      //pc[0].Init(0,0,0,0.1f);
    }

    pc[3].Init(1,0,0,0); // positive x-axis (unperturbed tangent-space normal)
    pc[4].Init(0,1,0,0); // dotproduct mask
    pc[6].Init(0.5f,-0.5f,0,1);
    pc[7].Init(1,0,0,0.5f);
  }

  vc[0].Trans4();                 // transpose for shader
  vc[1].Trans4();
  vc[2].Trans4();
  vc[7].Trans4();

  // set textures

  for(i=0;i<4;i++)
  {
    IDirect3DBaseTexture9 *tex;
    
    tex = 0;
    switch(mtrl->TexUse[i])
    {
    case 0:
    case 1:
    case 2:
    case 3:
      j = info->Tex[mtrl->TexUse[i]];
      if(j!=sINVALID)
      {
        sVERIFY(j>=0 && j<MAX_TEXTURE && Textures[j].Tex);
        tex = Textures[j].Tex;
        /*hr = DXDev->SetTexture(i,Textures[j].Tex);
        DXRROR(hr);*/
      }
      break;
    case -2:
      tex = DXNormalCube;
      //DXERROR(DXDev->SetTexture(i,DXNormalCube));
      break;
/*
    case -3:
      tex = Textures[SpecularLookupTex].Tex;
      //DXERROR(DXDev->SetTexture(i,Textures[SpecularLookupTex].Tex));
      break;
*/
    }

    if(tex)
      DXERROR(DXDev->SetTexture(i,tex));
  }

  // set the shaders & states

//  DXDev->SetTransform(D3DTS_VIEW,(D3DMATRIX*)&cmat);
//  DXDev->SetTransform(D3DTS_PROJECTION,(D3DMATRIX*)&pmat);
//  DXDev->SetTransform(D3DTS_WORLD,(D3DMATRIX*)&env->ModelSpace);
  DXDev->SetVertexShaderConstantF(0,(sF32 *)vc,(info->SpecialFlags & sMSF_PROJSCREEN) ? 32 : 26);
  DXDev->SetPixelShaderConstantF(0,(sF32 *)pc,8);

  SetStates(mtrl->States);
  hr = DXDev->SetVertexShader(mtrl->VS);
  sVERIFY(!FAILED(hr));
  hr = DXDev->SetPixelShader(mtrl->PS);
  sVERIFY(!FAILED(hr));

  ShaderOn = sTRUE;
}

/****************************************************************************/

void sSystem_::MtrlShadowStates(sU32 flags)
{
  sU32 states[256],*data=states;

  *data++ = sD3DRS_CULLMODE;                    *data++ = (flags & smBF_DOUBLESIDED) ? sD3DCULL_NONE : (flags & smBF_INVERTCULL?sD3DCULL_CW:sD3DCULL_CCW);
  
  switch(flags & smBF_STENCILMASK)
  {
  default:
    *data++ = D3DRS_STENCILENABLE;      *data++ = 0;
    *data++ = D3DRS_STENCILFUNC;        *data++ = D3DCMP_ALWAYS;
    break;
  case smBF_STENCILINC:
  case smBF_STENCILINC|smBF_STENCILZFAIL:
    *data++ = D3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = D3DRS_STENCILFUNC;        *data++ = D3DCMP_ALWAYS;
    *data++ = D3DRS_STENCILFAIL;        *data++ = D3DSTENCILOP_KEEP;
    if(flags & smBF_STENCILZFAIL)
    {
      *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_DECR;
      *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_KEEP;
    }
    else
    {
      *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_KEEP;
      *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_INCR;
    }
    break;
  case smBF_STENCILDEC:
  case smBF_STENCILDEC|smBF_STENCILZFAIL:
    *data++ = D3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = D3DRS_STENCILFUNC;        *data++ = D3DCMP_ALWAYS;
    *data++ = D3DRS_STENCILFAIL;        *data++ = D3DSTENCILOP_KEEP;
    if(flags & smBF_STENCILZFAIL)
    {
      *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_INCR;
      *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_KEEP;
    }
    else
    {
      *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_KEEP;
      *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_DECR;
    }
    break;
  case smBF_STENCILTEST:
    *data++ = D3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = D3DRS_STENCILFUNC;        *data++ = D3DCMP_EQUAL;
    *data++ = D3DRS_STENCILFAIL;        *data++ = D3DSTENCILOP_KEEP;
    *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_KEEP;
    *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_KEEP;
    break;
  case smBF_STENCILCLR:
    *data++ = D3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = D3DRS_STENCILFUNC;        *data++ = D3DCMP_ALWAYS;
    *data++ = D3DRS_STENCILFAIL;        *data++ = D3DSTENCILOP_ZERO;
    *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_ZERO;
    *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_ZERO;
    break;
  case smBF_STENCILINDE:
  case smBF_STENCILINDE|smBF_STENCILZFAIL:
    *data++ = D3DRS_CULLMODE;           *data++ = D3DCULL_NONE;
    *data++ = D3DRS_STENCILENABLE;      *data++ = 1;
    *data++ = D3DRS_TWOSIDEDSTENCILMODE; *data++ = 1;
    *data++ = D3DRS_STENCILFUNC;        *data++ = D3DCMP_ALWAYS;
    *data++ = D3DRS_CCW_STENCILFUNC;    *data++ = D3DCMP_ALWAYS;
    *data++ = D3DRS_STENCILFAIL;        *data++ = D3DSTENCILOP_KEEP;
    *data++ = D3DRS_CCW_STENCILFAIL;    *data++ = D3DSTENCILOP_KEEP;
    if(flags & smBF_STENCILZFAIL)
    {
      *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_DECR;
      *data++ = D3DRS_CCW_STENCILZFAIL;   *data++ = D3DSTENCILOP_INCR;
      *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_KEEP;
      *data++ = D3DRS_CCW_STENCILPASS;    *data++ = D3DSTENCILOP_KEEP;
    }
    else
    {
      *data++ = D3DRS_STENCILZFAIL;       *data++ = D3DSTENCILOP_KEEP;
      *data++ = D3DRS_CCW_STENCILZFAIL;   *data++ = D3DSTENCILOP_KEEP;
      *data++ = D3DRS_STENCILPASS;        *data++ = D3DSTENCILOP_INCR;
      *data++ = D3DRS_CCW_STENCILPASS;    *data++ = D3DSTENCILOP_DECR;
    }
    break;
  }

  *data++ = ~0;
  *data++ = ~0;
  SetStates(states);
}

/****************************************************************************/

#endif

/****************************************************************************/
/****************************************************************************/

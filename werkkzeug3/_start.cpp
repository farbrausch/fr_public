// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#include "_start.hpp"
#include "_startdx.hpp"
#include "material11.hpp"
#if sLINK_UTIL
#include "_util.hpp"                  // for sPerfMon->Flip()
#endif
#if sLINK_RYGDXT
#include "_rygdxt.hpp"
#endif

#define WINVER 0x500
#define _WIN32_WINNT 0x0500
#define DIRECTINPUT_VERSION 0x0800
#define DIRECTSOUND_VERSION 0x0800

#define sPLAYER_SCREENX     1024
#define sPLAYER_SCREENY     768
#define sPLAYER_FULLSCREEN  !sDEBUG
#define sPLAYER_DIALOG      1

#define sINTRO_NO_ALT_TAB   0 // setting this to 1 saves ~190 bytes

#if !sINTRO
const sChar *sWindowTitle=".theprodukkt";
#else
const sChar *sWindowTitle="fr-052: platinum";
#endif

#define D3D_DEBUG_INFO
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

#if sINTRO
#define DXERROR(hr) {if(FAILED(hr))sFatal("%s(%d) : directx error %08x (%d)",__FILE__,__LINE__,hr,hr&0x3fff);}
#else
#define DXERROR(hr) {HRESULT res=hr;if(FAILED(res)) ReportDXError(__FILE__,__LINE__,hr);}
#endif


#undef DeleteFile
#undef GetCurrentDirectory
#undef LoadBitmap
#undef GetCommandLine
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"comdlg32.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"olepro32.lib")
#pragma comment(lib,"oleaut32.lib")
#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"opengl32.lib")
#pragma comment(lib,"dinput8.lib")
#if sINTRO && !defined(_DEBUG)
#pragma comment(linker,"/nodefaultlib")
#endif
#if !sPLAYER
#pragma comment(lib,"shell32.lib")
#endif

#if !sPLAYER
#include <mmsystem.h>
#include <vfw.h>
#pragma comment(lib,"vfw32.lib")
#endif

#define LOGGING 1                 // log all create and release calls
//#define DXERROR(hr) {OutputDebugString(#hr "\n");if(FAILED(hr))sFatal("%s(%d) : directx error %08x (%d)",__FILE__,__LINE__,hr,hr&0x3fff);}

sMAKEZONE(FlipLock,"FlipLock",0xffff0000);
sMAKEZONE(MtrlSet,"MtrlSet",0xff404040);
sMAKEZONE(MtrlSetSetup,"MtrlSetSetup",0xff203040);
sMAKEZONE(Sound3D,"Sound3D",0xff804040);

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
sChar *WCmdLine;
sChar WFilename[2048];
HWND MsgWin;
static sBool SingleCore;

sSystem_ *sSystem;

static sU64 sPerfFrame;
static sF64 sPerfKalibFactor = 1.0f/1000;
static sU64 sPerfKalibRDTSC;
static sU64 sPerfKalibQuery;

// dies sollten meistens konstanten sein, damit ein paar if's wegoptimieren
// (auch ohne lekktor)
// aber manche builds haben einen konfigurationsdialog...

#if sCONFIGDIALOG
sInt IntroScreenX=sPLAYER_SCREENX;
sInt IntroScreenY=sPLAYER_SCREENY;
sInt IntroFlags = sPLAYER_FULLSCREEN ? sSF_FULLSCREEN : 0;
sInt IntroTargetAspect = 0;
sInt IntroTexQuality = 1;
sBool IntroShadows = sTRUE;
#else
const sInt IntroScreenX=sPLAYER_SCREENX;
const sInt IntroScreenY=sPLAYER_SCREENY;
const sInt IntroFlags = sPLAYER_FULLSCREEN ? sSF_FULLSCREEN : 0;
sInt IntroTargetAspect = 0;
sInt IntroTexQuality = 1;
sBool IntroShadows = sTRUE;
#endif
sInt IntroLoop;
sBool IntroStereo3D = sTRUE;
static D3DMULTISAMPLE_TYPE MultiSampleType = D3DMULTISAMPLE_NONE;
static sInt MultiSampleQuality = 0;

/****************************************************************************/

/****************************************************************************/

sInt WShow;
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
  { 0,12,D3DDECLTYPE_D3DCOLOR  ,0,D3DDECLUSAGE_NORMAL    ,0 },
  { 0,16,D3DDECLTYPE_D3DCOLOR  ,0,D3DDECLUSAGE_TANGENT   ,0 },
  { 0,20,D3DDECLTYPE_D3DCOLOR  ,0,D3DDECLUSAGE_COLOR     ,0 },
  { 0,24,D3DDECLTYPE_FLOAT2    ,0,D3DDECLUSAGE_TEXCOORD  ,0 },
  D3DDECL_END()
};
static D3DVERTEXELEMENT9 DeclTSpace3Big[] =
{
  { 0, 0,D3DDECLTYPE_FLOAT3    ,0,D3DDECLUSAGE_POSITION  ,0 },
  /*{ 0,12,D3DDECLTYPE_D3DCOLOR  ,0,D3DDECLUSAGE_NORMAL    ,0 },
  { 0,16,D3DDECLTYPE_D3DCOLOR  ,0,D3DDECLUSAGE_TANGENT   ,0 },
  { 0,20,D3DDECLTYPE_D3DCOLOR  ,0,D3DDECLUSAGE_COLOR     ,0 },
  { 0,24,D3DDECLTYPE_FLOAT2    ,0,D3DDECLUSAGE_TEXCOORD  ,0 },*/
  { 0,12,D3DDECLTYPE_FLOAT3    ,0,D3DDECLUSAGE_NORMAL    ,0 },
  { 0,24,D3DDECLTYPE_FLOAT3    ,0,D3DDECLUSAGE_TANGENT   ,0 },
  { 0,36,D3DDECLTYPE_D3DCOLOR  ,0,D3DDECLUSAGE_COLOR     ,0 },
  { 0,40,D3DDECLTYPE_FLOAT2    ,0,D3DDECLUSAGE_TEXCOORD  ,0 },
  D3DDECL_END()
};
static D3DVERTEXELEMENT9 DeclXYZW[] = 
{
  { 0, 0,D3DDECLTYPE_FLOAT4    ,0,D3DDECLUSAGE_POSITION  ,0 },
  D3DDECL_END()
};
static D3DVERTEXELEMENT9 DeclCubeSpecial[] = 
{
  { 0, 0,D3DDECLTYPE_UBYTE4    ,0,D3DDECLUSAGE_POSITION  ,0 },
  { 0, 4,D3DDECLTYPE_D3DCOLOR  ,0,D3DDECLUSAGE_COLOR     ,0 },
  D3DDECL_END()
};

static FVFTableStruct FVFTable[] = 
{
  {  0,0                },
  { 32,DeclDefault      },
  { 32,DeclDouble       },
  { 64,DeclTSpace       },
  { 16,DeclCompact      },
  { 16,DeclXYZW         },
  { 32,DeclTSpace3      },
  { 48,DeclTSpace3Big   },
  {  8,DeclCubeSpecial  },
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

#if sPLAYER
#define DXSSAMPLES 0x20000
#else
#define DXSSAMPLES 0x2000
#endif

static volatile sInt DXSLastTotalSample;

static volatile sInt DXSReadStart;
static volatile sInt DXSReadOffset;
static volatile sInt DXSTime;
static volatile sInt DXSIndex;

static HANDLE DXSThread;
static ULONG DXSThreadId;
static sInt volatile DXSRun;
static CRITICAL_SECTION DXSLock;
static HANDLE DXSEvent;

unsigned long __stdcall ThreadCode(void *);

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
/***   DX Error reporting                                                 ***/
/***                                                                      ***/
/****************************************************************************/

static void ReportDXError(const sChar *file,sInt line,HRESULT hr)
{
#if !sRELEASE
  __asm int 3;
#endif
#if sUSE_DIRECTSOUND
  sSystem->ExitDS();
#endif
#if sUSE_DIRECTINPUT
  sSystem->ExitDI();
#endif
  sSystem->ExitScreens();

  switch(hr)
  {
  case D3DERR_OUTOFVIDEOMEMORY:
    sFatal("Out of video memory (in %s:%d).\n\n"
      "Try using a lower resolution or lower antialiasing settings.",
      file,line);
    break;

  case E_OUTOFMEMORY:
    sFatal("DirectX ran out of memory (in %s:%d).",file,line);
    break;

  default:
    sFatal("DirectX error %08x occured in %s:%d.",hr,file,line);
    break;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Heap Memory Management                                             ***/
/***                                                                      ***/
/****************************************************************************/

sDInt MemoryUsedCount;

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
  if(p==0)
  {
#if sDEBUG
    //_CrtDumpMemoryLeaks();
#endif
    sFatal("ran out of virtual memory...");
  }

  //sSetMem(p,0x12,size);
  //sDPrintF("%10d : (%d)\n",MemoryUsedCount,size);

#if !sPLAYER
  MemoryUsedCount+=_msize(p);
#endif

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
  if(p==0)
  {
#if sDEBUG
    //_CrtDumpMemoryLeaks();
#endif
    sFatal("ran out of virtual memory...");
  }

  //sSetMem(p,0x12,size);
  //sDPrintF("%10d : %d\n",MemoryUsedCount,size);

#if !sPLAYER
  MemoryUsedCount+=_msize(p); 
#endif

  return p;
}

void __cdecl operator delete(void *p)
{
  if(p)
	{
#if !sPLAYER
		MemoryUsedCount-=_msize(p);
#endif

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

void checkHeap()
{
  HeapValidate(GetProcessHeap(),0,0);
}

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
/***   Dialog                                                             ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

sInt AspectRatioList[][2] = {
  { 4,3 }, { 5,4 }, { 16,9 }, { 16,10 }, { 2,1 }, { 3,2 },
  { 3,4 }, { 4,5 }, { 9,16 }, { 10,16 }, { 1,2 }, { 2,3 }, // <-- hochkant
};

#if sCONFIGDIALOG
#include "resource.h"

struct DisplayMode
{
  sInt XRes;
  sInt YRes;

  bool operator == (const DisplayMode &b) const
  {
    return XRes == b.XRes && YRes == b.YRes;
  }

  bool operator < (const DisplayMode &b) const
  {
    return XRes < b.XRes || (XRes == b.XRes && YRes < b.YRes);
  }
};

static sInt MatchAspectRatio(const DisplayMode &mode)
{
  sF32 ratio = 1.0f * mode.XRes / mode.YRes;
  sInt bestMatch = 0;
  sF32 bestDist = 1e+20f;

  for(sInt i=0;i<sCOUNTOF(AspectRatioList);i++)
  {
    sF32 targetRatio = 1.0f*AspectRatioList[i][0]/AspectRatioList[i][1];
    sF32 dist = sFAbs(ratio - targetRatio);

    if(dist < bestDist)
    {
      bestDist = dist;
      bestMatch = i;
    }
  }

  return bestMatch;
}

BOOL CALLBACK sDialogProc(HWND win,UINT msg,WPARAM wparam,LPARAM lparam) 
{
  static const sInt defaultXRes = 1024;
  static const sInt defaultYRes = 768;

  static DisplayMode *Modes=0;
  static sInt ModeCount;
  static D3DMULTISAMPLE_TYPE MultiTypes[17];
  static sInt MultiCount;

  switch(msg)
  {
  case WM_INITDIALOG: 
    {
      if(sWindowTitle)
        SetWindowText(win,sWindowTitle);

      // enumerate video modes
      IDirect3D9 *d3d9 = sSystem->DXD;
      D3DFORMAT fmt = D3DFMT_X8R8G8B8;

      sInt numModes = d3d9->GetAdapterModeCount(D3DADAPTER_DEFAULT,fmt);
      Modes = new DisplayMode[numModes];
      ModeCount = 0;
      for(sInt i=0;i<numModes;i++)
      {
        D3DDISPLAYMODE mode;
        if(d3d9->EnumAdapterModes(D3DADAPTER_DEFAULT,fmt,i,&mode) == D3D_OK)
        {
          DisplayMode newm;

          newm.XRes = mode.Width;
          newm.YRes = mode.Height;

          // find insertion point
          sInt i;
          for(i=ModeCount;i>0 && !(Modes[i-1] < newm);i--);

          // check for dupes
          if(i<ModeCount && Modes[i] == newm)
            continue;

          // insert
          for(sInt j=ModeCount;j>i;j--)
            Modes[j] = Modes[j-1];

          Modes[i] = newm;
          ModeCount++;
        }
      }

      // add video mode list to dialog
      sInt bestMatch = 0;
      sInt bestDist = 0x7fffffff;

      for(sInt i=0;i<ModeCount;i++)
      {
        sChar buffer[32];

        sInt dist = sAbs(Modes[i].XRes - defaultXRes) + sAbs(Modes[i].YRes - defaultYRes);
        if(dist<bestDist)
        {
          bestDist = dist;
          bestMatch=i;
        }
        
        sSPrintF(buffer,sizeof(buffer),"%dx%d",Modes[i].XRes,Modes[i].YRes);
        SendDlgItemMessage(win,IDC_RESOLUTION,CB_ADDSTRING,0,(LPARAM) buffer);
      }
      
      SendDlgItemMessage(win,IDC_RESOLUTION,CB_SETCURSEL,bestMatch,0);

      /*SendDlgItemMessage(win,IDC_RESOLUTION,CB_ADDSTRING,0,(LPARAM)"640x480");
      SendDlgItemMessage(win,IDC_RESOLUTION,CB_ADDSTRING,0,(LPARAM)"800x600");
      SendDlgItemMessage(win,IDC_RESOLUTION,CB_ADDSTRING,0,(LPARAM)"1024x768");
      SendDlgItemMessage(win,IDC_RESOLUTION,CB_SETCURSEL,1,0);*/

      // aspect ratios
      for(sInt i=0;i<sCOUNTOF(AspectRatioList);i++)
      {
        sChar buffer[32];

        sSPrintF(buffer,sizeof(buffer),"%d:%d",AspectRatioList[i][0],AspectRatioList[i][1]);
        SendDlgItemMessage(win,IDC_ASPECT,CB_ADDSTRING,0,(LPARAM) buffer);
      }
      SendDlgItemMessage(win,IDC_ASPECT,CB_SETCURSEL,
        MatchAspectRatio(Modes[bestMatch]),0);

      // multisampling quality
      MultiCount = 0;

      for(sInt i=0;i<17;i++)
      {
        D3DMULTISAMPLE_TYPE mst = (D3DMULTISAMPLE_TYPE) i;
        if(mst == D3DMULTISAMPLE_NONMASKABLE)
          continue;

        if(SUCCEEDED(d3d9->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT,
          D3DDEVTYPE_HAL,fmt,FALSE,mst,0)))
        {
          sChar buffer[32];

          MultiTypes[MultiCount++] = mst;
          if(i == 0)
            SendDlgItemMessage(win,IDC_MULTISAMPLE,CB_ADDSTRING,0,(LPARAM) "None");
          else
          {
            sSPrintF(buffer,sizeof(buffer),"%dx",i);
            SendDlgItemMessage(win,IDC_MULTISAMPLE,CB_ADDSTRING,0,(LPARAM) buffer);
          }
        }
      }

      SendDlgItemMessage(win,IDC_MULTISAMPLE,CB_SETCURSEL,0,0);

      /*SendDlgItemMessage(win,IDC_TEXQUAL,CB_ADDSTRING,0,(LPARAM)"Normal");
      SendDlgItemMessage(win,IDC_TEXQUAL,CB_ADDSTRING,0,(LPARAM)"High");
      SendDlgItemMessage(win,IDC_TEXQUAL,CB_ADDSTRING,0,(LPARAM)"Ultra");
      SendDlgItemMessage(win,IDC_TEXQUAL,CB_SETCURSEL,1,0);*/

#if sRELEASE
      CheckDlgButton(win,IDC_FULLSCREEN,BST_CHECKED);
#endif
      //CheckDlgButton(win,IDC_SHADOWS,BST_CHECKED);
      CheckDlgButton(win,IDC_VSYNC,BST_CHECKED);
      //CheckDlgButton(win,IDC_STEREO3D,BST_CHECKED);
    }
    return sTRUE;

  case WM_CLOSE:
    delete[] Modes;
    EndDialog(win,102);
    return sTRUE;

  case WM_COMMAND:
    switch(wparam&0xffff)
    {
    case IDC_RESOLUTION:
      // update aspect ratio on resolution change
      if((wparam >> 16) == CBN_SELCHANGE)
      {
        sInt item = SendDlgItemMessage(win,IDC_RESOLUTION,CB_GETCURSEL,0,0);
        if(item != CB_ERR)
          SendDlgItemMessage(win,IDC_ASPECT,CB_SETCURSEL,MatchAspectRatio(Modes[item]),0);
      }
      break;

    case IDOK:
      {
        sInt res = SendDlgItemMessage(win,IDC_RESOLUTION,CB_GETCURSEL,0,0);
        res = sRange(res,ModeCount-1,0);

        sU32 flags = sSF_DIRECT3D;
        if(IsDlgButtonChecked(win,IDC_FULLSCREEN) == BST_CHECKED)
          flags |= sSF_FULLSCREEN;
        if(IsDlgButtonChecked(win,IDC_VSYNC) == BST_CHECKED)
          flags |= sSF_WAITVSYNC;

#if !sINTRO
        sSetConfig(flags,Modes[res].XRes,Modes[res].YRes);
#else
        IntroScreenX = Modes[res].XRes;
        IntroScreenY = Modes[res].YRes;
        IntroFlags = flags;
#endif

        IntroTargetAspect = SendDlgItemMessage(win,IDC_ASPECT,CB_GETCURSEL,0,0);
        //IntroTexQuality = SendDlgItemMessage(win,IDC_TEXQUAL,CB_GETCURSEL,0,0);
        IntroTexQuality = 2;

        IntroLoop = (IsDlgButtonChecked(win,IDC_LOOP)==BST_CHECKED);
        IntroStereo3D = sFALSE;
        IntroShadows = sFALSE;
        /*IntroStereo3D = (IsDlgButtonChecked(win,IDC_STEREO3D) == BST_CHECKED);
        IntroShadows = (IsDlgButtonChecked(win,IDC_SHADOWS) == BST_CHECKED) ? sTRUE : sFALSE;*/
        MultiSampleType = MultiTypes[SendDlgItemMessage(win,IDC_MULTISAMPLE,CB_GETCURSEL,0,0)];
      }

      delete[] Modes;
      EndDialog(win,wparam);
      break;
    case IDCANCEL:
      delete[] Modes;
      EndDialog(win,wparam);
      break;
    }
    return sTRUE;
  }
  return sFALSE;
}

// please note that the resource-file is excluded from build
// you have to compile the resource manually, and then this
// code is used to extract the dialog from the resource file.

sBool ConfigDialog(sInt nr,const sChar *title)
{
  if(title)
    sWindowTitle = title;

  sInt ret = DialogBox(WInst,MAKEINTRESOURCE(nr),0,(DLGPROC) sDialogProc);
  return ret == -1 ? 0 : ret;
}
#endif

static sBool MessagePump()
{
  MSG msg;
  sBool ok = sTRUE;

  while(PeekMessage(&msg,0,0,0,PM_NOREMOVE))
  {
    if(!GetMessage(&msg,0,0,0))
      ok = sFALSE;

    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return ok;
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Startup, without intro                                             ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#if !sINTRO

static void MakeCpuMask()
{
  sU32 result;
  sU32 vendor[4];

// start with nothing supportet

  sSystem->CpuMask = 0;

// check cpuid command, and check avaiability of standard function #1

#if !sNOCRT
  __try
  {
#endif
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
#if !sNOCRT
  }
  __except(1)
  {
    return;
  }
#endif

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
  
#if !sNOCRT
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
#endif
}

/****************************************************************************/

static LRESULT WINAPI MainWndProc(HWND win,UINT msg,WPARAM wparam,LPARAM lparam)
{
  MsgWin = win;
  sVERIFY(sSystem);
  return sSystem->Msg(msg,wparam,lparam);
}

/****************************************************************************/

int APIENTRY WinMain(HINSTANCE inst,HINSTANCE prev,LPSTR cmdline,int show)
{
  WInst = GetModuleHandle(0);
  WCmdLine = GetCommandLineA();
  if(*WCmdLine=='/"')
  {
    WCmdLine++;
    while(*WCmdLine && *WCmdLine!='/"')
      WCmdLine++;
    if(*WCmdLine=='/"')
      WCmdLine++;
  }
  else
  {
    while(*WCmdLine && *WCmdLine!=' ')
      WCmdLine++;
  }
  while(*WCmdLine==' ')
    WCmdLine++;


#if sUSE_LEKKTOR
  sLekktorInit();
#endif

  sSystem = new sSystem_;
  sSetMem(((sU8 *)sSystem)+4,0,sizeof(sSystem_)-4); // argh
  
  // load d3d dll
  d3dlib = ::LoadLibraryA("d3d9.dll");
  if(d3dlib==0)
    sFatal("You need DirectX 9 (or better) to run this program.");
  Direct3DCreate9P = (Direct3DCreate9T) GetProcAddress(d3dlib,"Direct3DCreate9");
  sVERIFY(Direct3DCreate9P);

  // initialize d3d
  sSystem->DXD = (*Direct3DCreate9P)(D3D_SDK_VERSION);
  if(!sSystem->DXD)
    sFatal("DirectX 9.0c not present.");

  // config
  if(sAppHandler(sAPPCODE_CONFIG,0))
    sSystem->InitX();

  sRelease(sSystem->DXD);
  FreeLibrary(d3dlib);
  delete sSystem;

  sDPrintF("Memory Left: %d bytes\n",MemoryUsedCount);
  if(MemoryUsedCount!=16)
  {
    sDPrintF("************************************************\n");
    sDPrintF("\n               MEMORY LEAK\n\n");
    sDPrintF("************************************************\n");
  }

  return 0;
}

/****************************************************************************/

void sSetConfig(sU32 flags,sInt xs,sInt ys)
{
  sSystem->ConfigFlags = flags;
  sSystem->ConfigX = xs;
  sSystem->ConfigY = ys;
}

/****************************************************************************/

BOOL CALLBACK MonitorEnumProc(HMONITOR handle,HDC hdc,LPRECT rect,LPARAM user)
{
  HWND win;
  HWND pwin;
  RECT r,r2;
  sBool createMaximized=sFALSE;

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
        r2.left = r2.top = 0;
        r2.right = r2.bottom = 1;
        AdjustWindowRect(&r2,WS_OVERLAPPEDWINDOW,FALSE);
        sSystem->ConfigX = r.right-r.left-(r2.right-r2.left);
        sSystem->ConfigY = r.bottom-r.top-(r2.bottom-r2.top);
        createMaximized = sTRUE;
      }
    }
  }

  if(sSystem->WScreenCount<MAX_SCREEN)
  {
    if(sSystem->WScreenCount > 0)
      pwin = (HWND)sSystem->Screen[0].Window;
    else
      pwin = 0;

    r2.left = r2.top = 0;
    r2.right = sSystem->ConfigX; 
    r2.bottom = sSystem->ConfigY;
    AdjustWindowRect(&r2,WS_OVERLAPPEDWINDOW,FALSE);

    sInt style = WS_OVERLAPPEDWINDOW;
    if(sSystem->ConfigFlags & sSF_MINIMAL)
      style = WS_POPUP|WS_THICKFRAME;
    if(createMaximized)
      style |= WS_MAXIMIZE;
    style |= WS_VISIBLE;

    win = CreateWindowEx(0,"kk",sWindowTitle,style,r.left,r.top,r2.right-r2.left,r2.bottom-r2.top,pwin,0,WInst,0);

    sSystem->WWindowedStyle = style;
    sSystem->Screen[sSystem->WScreenCount].Window = (sDInt) win;
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
  sInitTypes();
#if sLINK_RYGDXT
  sInitDXT();
#endif

  _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)|_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF);
  sChar *test = new sChar[16];
  sCopyString(test,"TestLeak",16);

// find dlls

  dslib = ::LoadLibraryA("dsound.dll");
  DirectSoundCreate8P = (DirectSoundCreate8T) GetProcAddress(dslib,"DirectSoundCreate8");
  sVERIFY(DirectSoundCreate8P);

#if sUSE_DIRECTINPUT
  dilib = ::LoadLibraryA("dinput8.dll");
  DirectInput8CreateP = (DirectInput8CreateT) GetProcAddress(dilib,"DirectInput8Create");
  sVERIFY(DirectInput8CreateP);
#endif

// set up some more stuff
  DWORD procAffinity,sysAffinity;
  if(GetProcessAffinityMask(GetCurrentProcess(),&procAffinity,&sysAffinity))
    SingleCore = sysAffinity == 1;
  else
    SingleCore = sTRUE;

  sDPrintF("*** configuring system for %s-core operation.\n",SingleCore ? "single" : "multi");

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
  wc.hIcon            = LoadIcon(WInst,MAKEINTRESOURCE(23));
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
  CmdNoRt = sFALSE;

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
    else if(sCmpMem(cmdline,"ps11",4)==0)
    {
      CmdShaderLevel = sPS_11;
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
    else if(sCmpMem(cmdline,"nort",4)==0)
    {
      CmdNoRt = sTRUE;
      cmdline += 4;
    }
    else if(cmdline[0]=='"')
    {
      cmdline++;
      sChar *ptr = WFilename;
      sInt left = sCOUNTOF(WFilename);
      while(*cmdline!='"' && *cmdline && left>0)
      {
        *ptr++ = *cmdline++;
      }
      *ptr++ = 0;
      if(*cmdline=='"')
        cmdline++;
    }
    else
    {
      break;
    }
  }

  WCmdLine = cmdline;

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

#ifdef _DOPE
  TexMemAlloc = 0;
#endif

	ZBufXSize = ZBufYSize = 0;
	ZBufFormat = 0;
	ZBuffer = 0;
  RTBuffer = 0;
  CurrentRT = 0;
  SyncQuery = 0;
  
  for(i=0;i<MAX_TEXTURE;i++)
    Textures[i].Flags = 0;

  sSetMem(Setups,0,sizeof(Setups));
  sSetMem(Shaders,0,sizeof(Shaders));
  MtrlClearCaches();
  InitScreens();
  if(CmdShaderLevel>GetShaderLevel())
    CmdShaderLevel = GetShaderLevel();

#if !sNOCRT
  _control87(_PC_24|_RC_NEAR,MCW_PC|MCW_RC);
#endif
  
#if sUSE_DIRECTSOUND
  if(!InitDS())
    sFatal("could not initialize DirectSound");
#endif

  // main loop

  DXDev->BeginScene();
  sAppHandler(sAPPCODE_INIT,0);
  DXDev->EndScene();
  WShow = -1;

  msg.message = WM_NULL;
  PeekMessage(&msg,0,0,0,PM_NOREMOVE);
  while(msg.message!=WM_QUIT)
  {
    if(WShow!=-1)
    {
      ::ShowWindow((HWND)sSystem->Screen[0].Window,WShow);
      WShow = -1;
    }
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
      else
        Sleep(20);

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

  sExitTypes();

  sBroker->Free();
  sBroker->Dump();
  delete sBroker;
  sBroker = 0;

  FreeLibrary(dilib);
  FreeLibrary(dslib);
}

/****************************************************************************/

sInt sSystem_::Msg(sU32 msg,sU32 wparam,sU32 lparam)
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

//  sDPrintF("%04x %08x %08x\n",msg,wparam,lparam);
  win = MsgWin;
  if(WAborting || nr==-1)
    return DefWindowProc(win,msg,wparam,lparam);

  result = 0;

  switch(msg)
  {
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;

  case WM_PAINT:
#if !sPLAYER
    if(!WFullscreen)
      Render();
#endif
    ValidateRect(win,0);
#if 0
    if(/*WActiveCount!=0 &&*/ !WFullscreen)
      Render();
#endif
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
    //if(WFullscreen)
      return 0;
    break;
  case WM_ACTIVATE:
    WFocus = wparam != WA_INACTIVE;
    break;
  case WM_ACTIVATEAPP:
    WFocus = wparam != 0;
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
  sREGZONE(MtrlSet);
  sREGZONE(MtrlSetSetup);
  sREGZONE(Sound3D);

  ScreenFormat=CmdNoRt ? D3DFMT_R5G6B5 : D3DFMT_A8R8G8B8;
  ZBufFormat=CmdNoRt ? D3DFMT_D16 : D3DFMT_D24S8;

  for(nr=0;nr<WScreenCount;nr++)
  {
// determine n

    Screen[nr].SFormat=ScreenFormat;
    Screen[nr].ZFormat=ZBufFormat;

    WINZERO(d3dpp[nr]);
    d3dpp[nr].BackBufferFormat = (D3DFORMAT) Screen[nr].SFormat;
    d3dpp[nr].EnableAutoDepthStencil = sFALSE;
    d3dpp[nr].SwapEffect = D3DSWAPEFFECT_DISCARD;
    if((ConfigFlags & (sSF_FULLSCREEN | sSF_WAITVSYNC)) == (sSF_FULLSCREEN | sSF_WAITVSYNC))
      d3dpp[nr].BackBufferCount = 2;
    else
      d3dpp[nr].BackBufferCount = 1;
    d3dpp[nr].PresentationInterval = (ConfigFlags & sSF_WAITVSYNC) ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
    d3dpp[nr].Flags = 0;
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

    d3dpp[nr].MultiSampleType = MultiSampleType;
    d3dpp[nr].MultiSampleQuality = MultiSampleQuality;
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
      {
        ::MoveWindow((HWND)Screen[0].Window,0,0,2,2,1);
        sFatal("could not create screen");
      }
    }
    for(i=1;i<FVFMax;i++)
      DXDev->CreateVertexDeclaration(FVFTable[i].Info,&FVFTable[i].Decl);

    // create dummy texture
    sU16 bmp[4*4*4];
    sSetMem(bmp,0,sizeof(bmp));
    AddTexture(4,4,sTF_A8R8G8B8,bmp,1);
  }
  else 
  {
    sRelease(DXNormalCube);
    sRelease(DXTinyNormalCube);
    sRelease(DXAttenuationVolume);
    sRelease(ZBuffer);
    sRelease(RTBuffer);
    sRelease(SyncQuery);
    /*for(i=0;i<MAX_GEOHANDLE;i++)
    {
      if(GeoHandle[i].Mode!=0)
      {
        if(GeoHandle[i].Vertex.Buffer>=3)
          GeoBuffer[GeoHandle[i].Vertex.Buffer].Free();
        
        if(GeoHandle[i].Index.Buffer>=3)
          GeoBuffer[GeoHandle[i].Index.Buffer].Free();
      }
      GeoHandle[i].Vertex.Count = 0;
      GeoHandle[i].Vertex.Buffer = 0;
      GeoHandle[i].Index.Count = 0;
      GeoHandle[i].Index.Buffer = 0;
    }*/
    for(i=0;i</*MAX_GEOBUFFER*/3;i++)
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
        sRelease(Textures[i].Tex);
        sRelease(Textures[i].Shadow);
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

#ifdef _DOPE
  BufferMemAlloc = 0;
#endif

	ReCreateZBuffer();
  SyncQuery = 0;
  /*DXERROR*/(DXDev->CreateQuery(D3DQUERYTYPE_EVENT,&SyncQuery));  // allow this to fail

  CreateGeoBuffer(0,1,0,MAX_DYNVBSIZE);
  CreateGeoBuffer(1,1,1,MAX_DYNIBSIZE);
  CreateGeoBuffer(2,1,2,MAX_DYNIBSIZE*2);
  CreateGeoBuffer(3,0,1,2*0x8000/4*6);
  DXERROR(GeoBuffer[3].IB->Lock(0,2*0x8000/4*6,(void **) &iblock,0));
  for(i=0;i<0x8000/6;i++)
    sQuad(iblock,i*4+0,i*4+1,i*4+2,i*4+3);
  DXERROR(GeoBuffer[3].IB->Unlock());
  
  HardwareShaderLevel = sPS_00;
  if((caps.PixelShaderVersion&0xffff)!=0x0000)
  {
    MakeCubeNormalizer();
    MakeAttenuationVolume();
    HardwareShaderLevel = sPS_11;
    if((caps.PixelShaderVersion&0xffff)>=0x0104)
    {
      HardwareShaderLevel = sPS_14;
      if((caps.PixelShaderVersion&0xffff)>=0x0200)
      {
        HardwareShaderLevel = sPS_20;
      }
    }
  }

// set some defaults
  DXDev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
  DXDev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
  DXDev->SetRenderState(D3DRS_LIGHTING,0);

  if(CmdNoRt)
    DXDev->SetRenderState(D3DRS_DITHERENABLE,1);

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

	for(i=0;i<MAX_TEXTURE;i++)
	{
		if(Textures[i].Flags & sTIF_RENDERTARGET)
    {
      tex = &Textures[i];
	    DXERROR(DXDev->CreateTexture(tex->XSize,tex->YSize,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&tex->Tex,0));

      if(Textures[i].Flags & sTIF_NEEDEXTRAZ)
        DXERROR(DXDev->CreateDepthStencilSurface(tex->XSize,tex->YSize,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,TRUE,&tex->Shadow,0));
      /*if(MultiSampleType)
      {
	      DXERROR(DXDev->CreateRenderTarget(tex->XSize,tex->YSize,D3DFMT_A8R8G8B8,MultiSampleType,MultiSampleQuality,FALSE,&tex->Shadow,0));
      }*/
    }
	}

  DiscardCount[0] = DiscardCount[1] = DiscardCount[2] = 1;

  LastDecl = -1;
  LastVB = -1;
  LastVSize = 0;
  LastIB = -1;
  MtrlReset = sTRUE;
  MtrlClearCaches();

  CurrentTarget = -1;
  NeedFinishBlit = sFALSE;
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

  // handle default rendertarget texture sizes
  xs=sMax<sInt>(xs,1024);
  ys=sMax<sInt>(ys,1024/*512*/);

	for(i=0;i<MAX_TEXTURE;i++)
	{
    sInt flags = Textures[i].Flags;

		if((flags & sTIF_RENDERTARGET) && !(flags & sTIF_NEEDEXTRAZ))
		{
			xs=sMax<sInt>(xs,Textures[i].XSize);
			ys=sMax<sInt>(ys,Textures[i].YSize);
		}
	}

	// create the zbuffer
	if(xs!=ZBufXSize || ys!=ZBufYSize || !ZBuffer)
	{
		DXERROR(DXDev->SetDepthStencilSurface(0));

    sRelease(ZBuffer);
    sRelease(RTBuffer);

#if LOGGING
    sDPrintF("Create ZBuffer %dx%d\n",xs,ys);
#endif

		DXERROR(DXDev->SetDepthStencilSurface(0));    // only for chaos laptop...
		DXERROR(DXDev->CreateDepthStencilSurface(xs,ys,(D3DFORMAT) ZBufFormat,MultiSampleType,MultiSampleQuality,FALSE,&ZBuffer,0));

#if !sPLAYER || 1
    if(MultiSampleType)
    {
      DXERROR(DXDev->CreateRenderTarget(xs,ys,(D3DFORMAT) ScreenFormat,MultiSampleType,MultiSampleQuality,FALSE,&RTBuffer,0));
    }
#endif

		ZBufXSize=xs;
		ZBufYSize=ys;

		DXERROR(DXDev->SetDepthStencilSurface(ZBuffer));
	}
}

/****************************************************************************/

void sSystem_::ExitScreens()
{
  sInt i;

#if !sPLAYER
  VideoEnd();
#endif

  for(i=0;i<MAX_SETUPS;i++)
  {
    if(Setups[i].RefCount)
      Setups[i].Cleanup();
  }

  for(i=0;i<MAX_SHADERS;i++)
  {
    if(Shaders[i].RefCount)
      Shaders[i].Cleanup();
  }

  for(i=0;i<MAX_TEXTURE;i++)
  {
    sRelease(Textures[i].Tex);
    sRelease(Textures[i].Shadow);
  }
  
  sRelease(DXNormalCube);
  sRelease(DXTinyNormalCube);
  sRelease(DXAttenuationVolume);
  sRelease(SyncQuery);
  sRelease(ZBuffer);
  sRelease(RTBuffer);

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

//  for(i=0;i<MAX_MATERIALS;i++)
//    sVERIFY(Materials[i].RefCount==0);
  for(i=1;i<FVFMax;i++)
    if(FVFTable[i].Decl)
      FVFTable[i].Decl->Release();

  sRelease(DXDev);
}
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Startup, with sINTRO                                               ***/
/***                                                                      ***/
/****************************************************************************/

#if sINTRO

/****************************************************************************/

static LRESULT WINAPI MainWndProc(HWND win,UINT msg,WPARAM wparam,LPARAM lparam)
{
  MsgWin = win;
  sInt i;

  switch(msg)
  {
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;

  case WM_PAINT:
    ValidateRect(win,0);
    break;

  case WM_SETCURSOR:
    if(IntroFlags & sSF_FULLSCREEN)
    {
      SetCursor(0);
      return 0;
    }
    break;

  case WM_LBUTTONDOWN:
    sSystem->MouseButtons |= 1;
    if(sSystem->KeyIndex < MAX_KEYBUFFER)
      sSystem->KeyBuffer[sSystem->KeyIndex++] = sKEY_MOUSEL;
    break;
  case WM_LBUTTONUP:
    sSystem->MouseButtons &= ~1;
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

#pragma lekktor(off)

#if !sINTRO || !sRELEASE
int APIENTRY WinMain(HINSTANCE inst,HINSTANCE prev,LPSTR cmdline,int show)
{
  WInst = inst;
  WCmdLine = cmdline;

#if sUSE_LEKKTOR
  sLekktorInit();
#endif

  sSystem = new sSystem_;
  sSetMem(((sU8 *)sSystem)+4,0,sizeof(sSystem_)-4);

  // load d3d dll
  d3dlib = ::LoadLibraryA("d3d9.dll");
  if(d3dlib==0)
    sFatal("You need DirectX 9 (or better) to run this program.");
  Direct3DCreate9P = (Direct3DCreate9T) GetProcAddress(d3dlib,"Direct3DCreate9");
  sVERIFY(Direct3DCreate9P);

  // initialize d3d
  sSystem->DXD = (*Direct3DCreate9P)(D3D_SDK_VERSION);
  if(!sSystem->DXD)
    sFatal("DirectX 9.0c not present.");

#if !sCONFIGDIALOG
  sSystem->InitX();
#else
  if(sAppHandler(sAPPCODE_CONFIG,0))
    sSystem->InitX();
#endif

  sRelease(sSystem->DXD);
  FreeLibrary(d3dlib);
  delete sSystem;

	ExitProcess(0);
}
#else
void WinMainCRTStartup()
{
	WInst = GetModuleHandle(0);
  WCmdLine = GetCommandLineA();
  while(*WCmdLine!=0 && *WCmdLine!=' ') WCmdLine++;
  while(*WCmdLine==' ') WCmdLine++;

#if sUSE_LEKKTOR
  sLekktorInit();
#endif

  sSystem = new sSystem_;
  sSetMem(((sU8 *)sSystem)+4,0,sizeof(sSystem_)-4);

  // load d3d dll
  d3dlib = ::LoadLibraryA("d3d9.dll");
  if(d3dlib==0)
    sFatal("You need DirectX 9 (or better) to run this program.");
  Direct3DCreate9P = (Direct3DCreate9T) GetProcAddress(d3dlib,"Direct3DCreate9");
  sVERIFY(Direct3DCreate9P);

  // initialize d3d
  sSystem->DXD = (*Direct3DCreate9P)(D3D_SDK_VERSION);
  if(!sSystem->DXD)
    sFatal("DirectX 9.0c not present.");

#if !sCONFIGDIALOG
  sSystem->InitX();
#else
  if(sAppHandler(sAPPCODE_CONFIG,0))
    sSystem->InitX();
#endif

  sRelease(sSystem->DXD);
  FreeLibrary(d3dlib);
  delete sSystem;

	ExitProcess(0);
}
#endif

/****************************************************************************/

void sSystem_::InitX()
{
  WNDCLASS wc;
  MSG msg;
  sBool quitmsg;
  sInt i;

// init system

  sSystem->ConfigFlags = sSF_DIRECT3D;
  sSystem->ConfigX = IntroScreenX;
  sSystem->ConfigY = IntroScreenY;

// find dlls

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

  wc.lpszClassName    = ".thepd00";
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

  sU32 windowstyle;
  sInt windowx;
  sInt windowy;
  if(IntroFlags & sSF_FULLSCREEN) // IntroFullscreen ist normalerweise eine konstante!
  {
    windowstyle = WS_POPUP|WS_VISIBLE;
    windowx = IntroScreenX;
    windowy = IntroScreenY;
  }
  else
  {
    windowstyle = WS_OVERLAPPEDWINDOW|WS_VISIBLE;
    
    RECT rc;

    rc.left = rc.top = 0;
    rc.right = IntroScreenX;
    rc.bottom = IntroScreenY;
    AdjustWindowRect(&rc,windowstyle,FALSE);

    windowx = rc.right - rc.left;
    windowy = rc.bottom - rc.top;
  }

  Screen[0].Window = (sU32) CreateWindowEx(
    0,
    ".thepd00",
    sWindowTitle,
    windowstyle,
    0,0,
    windowx,windowy,
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
  sSetMem(Setups,0,sizeof(Setups));
  MtrlClearCaches();
  InitScreens();

//  ShowWindow((HWND)Screen[0].Window,SW_SHOW);

  sFloatFix();
  
#if sUSE_DIRECTSOUND
  if(!InitDS())
    sFatal("could not initialize DirectSound");
#endif

  // main loop

  DXDev->BeginScene();
  sAppHandler(sAPPCODE_INIT,0);
  DXDev->EndScene();

  msg.message = WM_NULL;
  PeekMessage(&msg,0,0,0,PM_NOREMOVE);
  quitmsg = 0;

  do
  {
    if(!MessagePump())
      quitmsg = 1;

    for(i=0;i<KeyIndex;i++)
      sAppHandler(sAPPCODE_KEY,KeyBuffer[i]);
    KeyIndex = 0;

    Render();
#if sUSE_DIRECTSOUND
    MarkDS();
#endif
  }
  while(!quitmsg);

  sAppHandler(sAPPCODE_EXIT,0);

#if sUSE_DIRECTSOUND
  if(DXSBuffer)
    DXSBuffer->Stop();
  ExitDS();
#endif
  ExitScreens();
  sExitTypes();
}

/****************************************************************************/

void sSystem_::InitScreens()
{
  HRESULT hr;
  D3DPRESENT_PARAMETERS d3dpp;
  sInt i;
  sU32 create;
  sU16 *iblock;
  D3DCAPS9 caps;

  sREGZONE(FlipLock);

	ZBufFormat=D3DFMT_D24S8;

  Screen[0].SFormat=D3DFMT_A8R8G8B8;
  Screen[0].ZFormat=ZBufFormat;

  WINZERO(d3dpp);
  d3dpp.BackBufferFormat = (D3DFORMAT) Screen[0].SFormat;
  d3dpp.EnableAutoDepthStencil = sFALSE;
  d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  if(IntroFlags & sSF_WAITVSYNC)
  {
    d3dpp.BackBufferCount = 2;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
  }
  else
  {
    d3dpp.BackBufferCount = 1;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
  }

  d3dpp.Flags = 0;
  d3dpp.hDeviceWindow = (HWND) Screen[0].Window;

  if(IntroFlags & sSF_FULLSCREEN)
  {
    d3dpp.Windowed = FALSE;
    d3dpp.BackBufferWidth = ConfigX;
    d3dpp.BackBufferHeight = ConfigY;
    Screen[0].XSize = ConfigX;
    Screen[0].YSize = ConfigY;
  }
  else
  {
    RECT r;

    d3dpp.Windowed = TRUE;
    GetClientRect((HWND) Screen[0].Window,&r);
    Screen[0].XSize = r.right-r.left;
    Screen[0].YSize = r.bottom-r.top;
  }

  hr=DXD->GetDeviceCaps(0,D3DDEVTYPE_HAL,&caps);
  sVERIFY(!FAILED(hr));

  if(!DXDev)
  {
    if((caps.PixelShaderVersion&0xffff)<0x0100)
      sFatal("Pixel shaders required.");

    create = D3DCREATE_HARDWARE_VERTEXPROCESSING;

    hr = DXD->CreateDevice(0,D3DDEVTYPE_HAL,(HWND) Screen[0].Window,create,&d3dpp,&DXDev);
    if(FAILED(hr))
    {
      create = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

      hr = DXD->CreateDevice(0,D3DDEVTYPE_HAL,(HWND) Screen[0].Window,create,&d3dpp,&DXDev);
      if(FAILED(hr))
      {
#if sPLAYER && !sINTRO
        ::MoveWindow((HWND)Screen[0].Window,0,0,2,2,1);
        ConfigDialog(IDD_DIALOG3,0);
        ExitProcess(0);
#else
        sFatal("CreateDevice failed");
#endif
      }
    }
    for(i=1;i<FVFMax;i++)
      DXDev->CreateVertexDeclaration(FVFTable[i].Info,&FVFTable[i].Decl);

    // create dummy texture
    sU16 bmp[4*4*4];
    sSetMem(bmp,0,sizeof(bmp));
    AddTexture(4,4,sTF_A8R8G8B8,bmp,1);
  }
  else 
  {
#if !sINTRO_NO_ALT_TAB
    if(DXNormalCube)
    {
      DXNormalCube->Release();
      DXNormalCube = 0;
    }
    sRelease(DXTinyNormalCube);
		if(ZBuffer)
		{
			ZBuffer->Release();
			ZBuffer = 0;
		}
    for(i=0;i<2;i++)
    {
      DXBlockSurface[i]->Release();
      DXBlockSurface[i] = 0;
    }

    for(i=0;i<3;i++)
    {
      if(GeoBuffer[i].VB)
        GeoBuffer[i].VB->Release();
      
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
#endif
  }

// zbuffer
  sInt zBufX = sMax(ConfigX,1024);
  sInt zBufY = sMax(ConfigY, 512);

	DXERROR(DXDev->CreateDepthStencilSurface(zBufX,zBufY,(D3DFORMAT) ZBufFormat,D3DMULTISAMPLE_NONE,0,FALSE,&ZBuffer,0));
	DXERROR(DXDev->SetDepthStencilSurface(ZBuffer));

// initialize gpumask

  if(caps.StencilCaps & D3DSTENCILCAPS_TWOSIDED) GpuMask |= sGPU_TWOSIDESTENCIL;

// init buffer management

  CreateGeoBuffer(0,1,0,MAX_DYNVBSIZE);
  CreateGeoBuffer(1,1,1,MAX_DYNIBSIZE);
  CreateGeoBuffer(2,1,2,MAX_DYNIBSIZE*2);

// set some defaults
  WDeviceLost = 0;

#if !sINTRO_NO_ALT_TAB
  for(i=0;i<MAX_TEXTURE;i++)
	{
		if(Textures[i].Flags & sTIF_RENDERTARGET)
    {
      sHardTex *tex = &Textures[i];
	    DXERROR(DXDev->CreateTexture(tex->XSize,tex->YSize,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&tex->Tex,0));
    }
	}
#endif

  for(i=0;i<2;i++)
    DXDev->CreateOffscreenPlainSurface(16,16,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&DXBlockSurface[i],0);

// create managed resources
  CreateGeoBuffer(3,0,1,2*0x8000/4*6);
  DXERROR(GeoBuffer[3].IB->Lock(0,2*0x8000/4*6,(void **) &iblock,0));
  for(i=0;i<0x8000/6;i++)
    sQuad(iblock,i*4+0,i*4+1,i*4+2,i*4+3);
  DXERROR(GeoBuffer[3].IB->Unlock());
  MakeCubeNormalizer();
  MakeAttenuationVolume();

  DiscardCount[0] = DiscardCount[1] = DiscardCount[2] = 1;

  LastDecl = -1;
  LastVB = -1;
  LastVSize = 0;
  LastIB = -1;
  MtrlReset = sTRUE;

  CurrentTarget = -1;
}

/****************************************************************************/

void sSystem_::ExitScreens()
{
  sInt i;

  for(i=0;i<MAX_SETUPS;i++)
  {
    if(Setups[i].RefCount)
      Setups[i].Cleanup();
  }

  for(i=0;i<MAX_SHADERS;i++)
  {
    if(Shaders[i].RefCount)
      Shaders[i].Cleanup();
  }

  for(i=0;i<MAX_TEXTURE;i++)
  {
    sRelease(Textures[i].Tex);
    sRelease(Textures[i].Shadow);
  }
  
  sRelease(DXNormalCube);
  sRelease(DXTinyNormalCube);
  sRelease(DXAttenuationVolume);
  sRelease(SyncQuery);
  sRelease(ZBuffer);
  sRelease(RTBuffer);

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

//  for(i=0;i<MAX_MATERIALS;i++)
//    sVERIFY(Materials[i].RefCount==0);
  for(i=1;i<FVFMax;i++)
    if(FVFTable[i].Decl)
      FVFTable[i].Decl->Release();

  sRelease(DXDev);
}
#endif

#pragma lekktor(on)

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

#pragma lekktor(off)

void sSystem_::Exit()
{
#if sUSE_LEKKTOR
  sLekktorExit();
#endif
   DestroyWindow ((HWND)Screen[0].Window);
//  PostQuitMessage(0);
}

void sSystem_::Tag()
{
}

/*sNORETURN*/ void sSystem_::Abort(sChar *msg)
{
#if sINTRO
  if(DXD)
    DXD->Release();

  if(msg)
    MessageBox(0,msg,"Fatal Error",MB_OK|MB_TASKMODAL|MB_SETFOREGROUND|MB_TOPMOST|MB_ICONERROR);

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
  if(!sFatality && !sAppHandler(sAPPCODE_DEBUGPRINT,(sDInt) s))
  {
    OutputDebugString(s);
  }
}

#pragma lekktor(on)

/****************************************************************************/

#if !sINTRO

void sSystem_::Init(sU32 flags,sInt xs,sInt ys)
{
  ConfigX = xs;
  ConfigY = ys;
  ConfigFlags = flags;
}

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
    ::MoveWindow((HWND)Screen[0].Window,0,0,x,y,TRUE);
    ShowWindow((HWND)Screen[0].Window,SW_RESTORE);
    InitScreens();
  }

  if(KeyIndex < MAX_KEYBUFFER)
    KeyBuffer[KeyIndex++] = sKEY_MODECHANGE;
}

sInt sSystem_::MemoryUsed()
{
  return MemoryUsedCount;
}

void sSystem_::CheckMem()
{
  _CrtCheckMemory();
}
#endif

sChar *sSystem_::GetCmdLine()
{
  return WCmdLine;
}

/****************************************************************************/
/***                                                                      ***/
/***   Render                                                             ***/
/***                                                                      ***/
/****************************************************************************/

#if sPLAYER

#pragma lekktor(off)

#ifdef _DOPE
extern void DopeProgress(sInt done,sInt max);
#endif

void sSystem_::Progress(sInt done,sInt max)
{
  static sInt lasttime = 0;
  const int step = 3;
  sInt time;
  sRect r[3];

#ifndef _DOPE
  if(lasttime==0)
    GetAsyncKeyState(VK_ESCAPE);

  time = timeGetTime();
  if(time > lasttime+450 || done==max)
  {
    lasttime = time;

    sViewport vp;
    vp.Init();
    SetViewport(vp);
    Clear(sVCF_COLOR);

    sZONE(FlipLock);

    static const sU32 colors[3] = { 0xffffffff,0xff000000,0xffffffff };
    r[0].Init(20+step*0,ConfigY/2-20+step*0,ConfigX-20-step*0,ConfigY/2+20-step*0);
    r[1].Init(20+step*1,ConfigY/2-20+step*1,ConfigX-20-step*1,ConfigY/2+20-step*1);
    r[2].Init(20+step*2,ConfigY/2-20+step*2,ConfigX-20-step*2,ConfigY/2+20-step*2);
    r[2].x1 = r[2].x0 + 1 + sMin(max,done)*(r[2].x1-r[2].x0-1)/max;
    ColorFill(3,r,colors);

    DXDev->EndScene();
    DXDev->Present(0,0,0,0);
    if(!MessagePump())
      ExitProcess(0);
    DXDev->BeginScene();

#if !sLINK_KKRIEGER
    // ganz schlechte idee, den progress-bar mit escape abzubrechen 
    // wenn man den 2. progressbar mit escape startet.

    sInt key;
    key = GetAsyncKeyState(VK_ESCAPE);
    if(key&0x0001)
    {
      DXDev->Release(); 
      ExitProcess(0);
    }
#endif

  }
#else
  time = timeGetTime();
  if(time > lasttime+80 || done==max)
  {
    lasttime = time;

#if sUSE_DIRECTINPUT
    PollDI();
#endif

    for(sInt i=0;i<KeyIndex;i++)
      sAppHandler(sAPPCODE_KEY,KeyBuffer[i]);
    KeyIndex = 0;

    MessagePump();

    DopeProgress(done,max);
    DXDev->Present(0,0,0,0);
  }
#endif
}



void sSystem_::WaitForKey()
{
//  static sInt lasttime;
//  const int step = 3;
//  sInt time;

  while(1)
  {
    sViewport vp;
    vp.Init();
    SetViewport(vp);
    Clear(sVCF_COLOR,0xff000000);

    sZONE(FlipLock);

    DXDev->EndScene();
    DXDev->Present(0,0,0,0);
    DXDev->BeginScene();

    sInt key;
    key = GetAsyncKeyState(VK_SPACE);
    if(key&0x8000)
      break;

    if(!MessagePump())
      ExitProcess(0);
  }
}

#pragma lekktor(on)

#endif

/****************************************************************************/

void sSystem_::Render()
{
  static sInt LastTime=-1;
  static sInt Reentrant = 0;
  sInt time,ticks;
  HRESULT hr;

  LastCamera.Init();
  LastMatrix.Init();
  LastViewProject.Init();

#pragma lekktor(off)
  if(WDeviceLost)
  {
#if !sINTRO_NO_ALT_TAB
    Sleep(100);
    hr = DXDev->TestCooperativeLevel();
    if(hr!=D3DERR_DEVICENOTRESET)
      return;
    InitScreens();
    if(WDeviceLost)
      return;
#else
    ExitProcess(0);
#endif
  }
#pragma lekktor(on)

  time = sSystem->GetTime();
  ticks = 0;
  if(LastTime==-1)
    LastTime = time;

  sAppHandler(sAPPCODE_FRAME,0);

#if !sINTRO
  while(time>LastTime)
  {
    LastTime+=10;
    ticks++;
  }
  if(ticks>10)
    ticks=10;

  sAppHandler(sAPPCODE_TICK,ticks);
#endif

  if(Reentrant) 
    DXERROR(DXDev->EndScene());
  Reentrant++;
  DXERROR(DXDev->BeginScene());
  sAppHandler(sAPPCODE_PAINT,0);
  DXERROR(DXDev->EndScene());
  Reentrant--;

#if sPROFILE
  sPerfMon->Marker(1);
#endif

#if !sINTRO
  if(WResponse)
  {
    sZONE(FlipLock);

    if(SyncQuery)
    {
      SyncQuery->Issue(D3DISSUE_END);
      while(S_FALSE == SyncQuery->GetData(0,0,D3DGETDATA_FLUSH)); 
    }
  }
#else
  {
    static sInt dblock = 0;
    D3DLOCKED_RECT lr;

    DXDev->ColorFill(DXBlockSurface[dblock],0,0);
    dblock = 1-dblock;

    DXERROR(DXBlockSurface[dblock]->LockRect(&lr,0,D3DLOCK_READONLY));
    DXBlockSurface[dblock]->UnlockRect();
  }
#endif

  hr = DXDev->Present(0,0,0,0);
  if(hr==D3DERR_DEVICELOST)
    WDeviceLost = 1;
  DiscardCount[0]++;  DiscardCount[1]++;  DiscardCount[2]++;
  
  if(Reentrant) 
    DXERROR(DXDev->BeginScene());
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

void sSystem_::ColorFill(sInt nRects,const sRect *rects,const sU32 *colors)
{
  IDirect3DSurface9 *backbuffer=0;
  IDirect3DSwapChain9 *swapchain=0;

  if(nRects && SUCCEEDED(DXDev->GetSwapChain(0,&swapchain))
    && SUCCEEDED(swapchain->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&backbuffer)))
  {
    for(sInt i=0;i<nRects;i++)
      DXDev->ColorFill(backbuffer,(RECT *) &rects[i],colors[i]);
  }

  sRelease(swapchain);
  sRelease(backbuffer);
}

/****************************************************************************/

void sSystem_::Clear(sU32 flags,sU32 color)
{
  sU32 d3dClear = 0;

  if(flags & sVCF_COLOR)    d3dClear |= D3DCLEAR_TARGET;
  if(flags & sVCF_Z)        d3dClear |= D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL;
  if(flags & sVCF_STENCIL)  d3dClear |= D3DCLEAR_STENCIL;

  if(d3dClear)
    DXDev->Clear(0,0,d3dClear,color,1.0f,0);
}

void sSystem_::SetViewport(const sViewport &vp)
{
  IDirect3DSurface9 *target;
  sInt txs,tys,tgtid;

  CurrentViewport = vp;

  // get target surface and its extents
  if(vp.RenderTarget == sINVALID)
  {
    // get screen extents and target surface
    sInt nr = vp.Screen;

    sVERIFY(nr >= 0 && nr < WScreenCount);
    txs = Screen[nr].XSize;
    tys = Screen[nr].YSize;
    tgtid = -nr - 1;
  }
  else
  {
    // get the target texture and verify that it's a rendertarget
    sVERIFY(vp.RenderTarget >= 0);
    sHardTex *tex = &Textures[vp.RenderTarget];
    sVERIFY(tex->Flags & sTIF_RENDERTARGET);

    // get extents and surface
    txs = tex->XSize;
    tys = tex->YSize;
    tgtid = vp.RenderTarget;
  }

  // if the given window has zero extents, use full surface
  if(!vp.Window.XSize() || !vp.Window.YSize())
  {
    CurrentViewport.Window.x0 = 0;
    CurrentViewport.Window.y0 = 0;
    CurrentViewport.Window.x1 = txs;
    CurrentViewport.Window.y1 = tys;
  }

  ViewportX = CurrentViewport.Window.XSize();
  ViewportY = CurrentViewport.Window.YSize();

  // prepare d3d viewport
  D3DVIEWPORT9 d3dvp;

  d3dvp.X = CurrentViewport.Window.x0;
  d3dvp.Y = CurrentViewport.Window.y0;
  d3dvp.Width = ViewportX;
  d3dvp.Height = ViewportY;
  d3dvp.MinZ = 0.0f;
  d3dvp.MaxZ = 1.0f;

  // set render target and viewport
  if(CurrentTarget != tgtid)
  {
    // finishing blit if multisampling is on
    FinishMSBlit();

    IDirect3DSurface9 *zbuf = ZBuffer;

    if(vp.RenderTarget == sINVALID)
    {
      IDirect3DSwapChain9 *swapchain;
      DXERROR(DXDev->GetSwapChain(vp.Screen,&swapchain));
      DXERROR(swapchain->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&target));
      swapchain->Release();
    }
    else if(vp.Screen == -2 && MultiSampleType)
    {
#if sPLAYER || 1
      sHardTex *tex = &Textures[vp.RenderTarget];

      if(tex->XSize <= Screen[0].XSize && tex->YSize <= Screen[0].YSize)
      {
        IDirect3DSwapChain9 *swapchain;
        DXERROR(DXDev->GetSwapChain(0,&swapchain));
        DXERROR(swapchain->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&target));
        swapchain->Release();
      }
      else
      {
        target = RTBuffer;
        target->AddRef();
      }

      NeedFinishBlit = sTRUE;
#else
      target=RTBuffer;
      target->AddRef();
#endif
    }
    else
    {
      DXERROR(Textures[vp.RenderTarget].Tex->GetSurfaceLevel(0,&target));
      NeedFinishBlit = sFALSE;

      if(Textures[vp.RenderTarget].Flags & sTIF_NEEDEXTRAZ)
        zbuf = Textures[vp.RenderTarget].Shadow;
    }

    DXDev->SetRenderTarget(0,target);
    DXDev->SetDepthStencilSurface(zbuf);
    target->Release();

    CurrentTarget = tgtid;
  }

  DXDev->SetViewport(&d3dvp);
}

void sSystem_::FinishMSBlit()
{
  if(!NeedFinishBlit)
    return;

  if(CurrentTarget >= 0 && MultiSampleType)
  {
    RECT rc;
    sHardTex *tex = &Textures[CurrentTarget];
    IDirect3DSurface9 *source,*target;

    rc.left = 0;
    rc.top = 0;

#if sPLAYER || 1
    if(tex->XSize <= Screen[0].XSize && tex->YSize <= Screen[0].YSize)
    {
      IDirect3DSwapChain9 *swapchain;
      DXERROR(DXDev->GetSwapChain(0,&swapchain));
      DXERROR(swapchain->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&source));
      swapchain->Release();
    }
    else
    {
      source = RTBuffer;
      source->AddRef();
    }

    rc.right = tex->XSize;
    rc.bottom = tex->YSize;
#else
    source = RTBuffer;
    source->AddRef();

    rc.right = tex->XSize;
    rc.bottom = tex->YSize;
#endif

    DXERROR(tex->Tex->GetSurfaceLevel(0,&target));
    DXERROR(DXDev->StretchRect(source,&rc,target,0,D3DTEXF_NONE));
    target->Release();
    source->Release();
  }

  //NeedFinishBlit = sFALSE;
}

void sSystem_::CopyRT(sInt dest,const sRect *destRect,sInt source,const sRect *srcRect)
{
  IDirect3DSurface9 *srcsurf,*dstsurf;

  sVERIFY(dest > -1 && Textures[dest].Tex);
  sVERIFY(source > -1 && Textures[source].Tex);

  DXERROR(Textures[dest].Tex->GetSurfaceLevel(0,&dstsurf));
  DXERROR(Textures[source].Tex->GetSurfaceLevel(0,&srcsurf));
  DXERROR(DXDev->StretchRect(srcsurf,(const RECT *) srcRect,dstsurf,(const RECT *) destRect,D3DTEXF_NONE));
  dstsurf->Release();
  srcsurf->Release();
}

void sSystem_::ClearTarget(sInt rt,sU32 color)
{
  IDirect3DSurface9 *texbuffer;
  DXERROR(Textures[rt].Tex->GetSurfaceLevel(0,&texbuffer));
  DXERROR(DXDev->ColorFill(texbuffer,0,color));
  texbuffer->Release();
}

void sSystem_::GrabScreen(sInt sourceRT,const sRect &win,sInt tex,sBool stretch)
{
  IDirect3DSurface9 *backbuffer;
  IDirect3DSwapChain9 *swapchain;
  IDirect3DSurface9 *texbuffer;
  sHardTex *ht;

  ht = &Textures[tex];
  if(sourceRT == -1)
  {
	  DXERROR(DXDev->GetSwapChain(0,&swapchain));
	  DXERROR(swapchain->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&backbuffer));
  }
  else
  {
    DXERROR(Textures[sourceRT].Tex->GetSurfaceLevel(0,&backbuffer));
  }

	DXERROR(ht->Tex->GetSurfaceLevel(0,&texbuffer));

  RECT tgtRect;
  tgtRect.left = 0;
  tgtRect.top = 0;
  tgtRect.right = win.XSize();
  tgtRect.bottom = win.YSize();

//  if(!sRELEASE)
//    DXERROR(DXDev->ColorFill(texbuffer,0,0xffff0000));
  DXERROR(DXDev->StretchRect(backbuffer,(const RECT *)&win,texbuffer,stretch ? 0 : &tgtRect,D3DTEXF_NONE));

  if(sourceRT == -1)
	  swapchain->Release();

  backbuffer->Release();
  texbuffer->Release();
}

void sSystem_::Begin2D(sU32 **buffer,sInt &stride)
{
  IDirect3DSurface9 *backbuffer;
  IDirect3DSwapChain9 *swapchain;
  D3DLOCKED_RECT lr;

	DXERROR(DXDev->GetSwapChain(0,&swapchain));
	DXERROR(swapchain->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&backbuffer));
  DXERROR(backbuffer->LockRect(&lr,0,0));

	swapchain->Release();
  backbuffer->Release();

  *buffer = (sU32 *) lr.pBits;
  stride = lr.Pitch;
}

void sSystem_::End2D()
{
  IDirect3DSurface9 *backbuffer;
  IDirect3DSwapChain9 *swapchain;

  DXERROR(DXDev->GetSwapChain(0,&swapchain));
	DXERROR(swapchain->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&backbuffer));
  DXERROR(backbuffer->UnlockRect());

	swapchain->Release();
  backbuffer->Release();
}

void sSystem_::GetTransform(sInt mode,sMatrix &mat)
{
  switch(mode)
  {
  case sGT_VIEW:
    mat = LastCamera;
    break;

  case sGT_MODELVIEW:
    mat = LastMatrix;
    mat.TransR();
    mat.MulA(mat,LastCamera);
    break;

  case sGT_MODEL:
    mat = LastMatrix;
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
      tex->Shadow = 0;

	    if(tex->Flags & sTIF_RENDERTARGET)
	    {
        if(tex->XSize==0)
        {
          tex->XSize = Screen[0].XSize;
          tex->YSize = Screen[0].YSize;
        }

#if LOGGING
        sDPrintF("Create Rendertarget %dx%d\n",tex->XSize,tex->YSize);
#endif
#ifdef _DOPE
        TexMemAlloc += tex->XSize * tex->YSize * 4;
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
#ifdef _DOPE
        TexMemAlloc += tex->XSize * tex->YSize * 4 * 4 / 3;
#endif
        sInt mipCount = 0;

        // non-power-of-2
        if((tex->XSize & (tex->XSize-1)) || (tex->YSize & (tex->YSize-1)))
          mipCount = 1;

		    DXERROR(DXDev->CreateTexture(tex->XSize,tex->YSize,mipCount,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&tex->Tex,0));
        UpdateTexture(i,ti.Bitmap);
	    }

      return i;
    }
    tex++;
  }

  return sINVALID;
}

sInt sSystem_::AddTexture(sInt xs,sInt ys,sInt formatParm,sU16 *data,sInt mipcount,sInt miptresh)
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
    D3DFMT_A1R5G5B5,
    D3DFMT_DXT1,
    D3DFMT_DXT5,
    D3DFMT_R32F,
    D3DFMT_L8,
    D3DFMT_A8L8,
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
    D3DFMT_A1R5G5B5,
    D3DFMT_DXT1,
    D3DFMT_DXT5,
    D3DFMT_R32F,
    D3DFMT_L8,
    D3DFMT_A8L8,
  };
#endif

  sInt format = formatParm & 0xff;
  sVERIFY(format>0 && format<sTF_MAX);

  tex = Textures;
  for(i=0;i<MAX_TEXTURE;i++)
  {
    if(tex->Flags==0)
    {
      mipcount = sMin(mipcount,sGetPower2(xs));
      mipcount = sMin(mipcount,sGetPower2(ys));
      if(xs & (xs-1) || ys & (ys-1)) // xs/ys not a power of 2? no mips.
        mipcount = 1;

      tex->RefCount = 1;
      tex->XSize = xs;
      tex->YSize = ys;
      tex->Flags = sTIF_ALLOCATED;
      tex->Format = format;
      tex->MipLevels = mipcount;
      tex->Tex = 0;
      tex->Shadow = 0;

      if(formatParm & sTF_NEEDEXTRAZ)
        tex->Flags |= sTIF_NEEDEXTRAZ;

	    if(data==0)
	    {
        if(tex->XSize==0)
        {
          tex->XSize = Screen[0].XSize;
          tex->YSize = Screen[0].YSize;
        }

        if(CmdNoRt)
          tex->XSize = tex->YSize = 4;

        tex->Flags |= sTIF_RENDERTARGET;

#if LOGGINGT
        sDPrintF("Create Rendertarget %dx%d\n",tex->XSize,tex->YSize);
#endif
#ifdef _DOPE
        TexMemAlloc += tex->XSize * tex->YSize * 4;
#endif

        DXDev->EvictManagedResources();
		    DXERROR(DXDev->CreateTexture(tex->XSize,tex->YSize,1,D3DUSAGE_RENDERTARGET,formats[format],D3DPOOL_DEFAULT,&tex->Tex,0));
        if(tex->Flags & sTIF_NEEDEXTRAZ)
          DXERROR(DXDev->CreateDepthStencilSurface(tex->XSize,tex->YSize,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,TRUE,&tex->Shadow,0));

        /*if(MultiSampleType)
        {
  		    DXERROR(DXDev->CreateRenderTarget(tex->XSize,tex->YSize,formats[format],MultiSampleType,MultiSampleQuality,FALSE,&tex->Shadow,0));
        }*/
#if !sINTRO
        ReCreateZBuffer();
#endif
	    }
	    else
	    {
#if LOGGING
        sDPrintF("Create Texture %dx%d\n",tex->XSize,tex->YSize);
#endif
#ifdef _DOPE
        TexMemAlloc += tex->XSize * tex->YSize * 4 * 4 / 3;
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
    sVERIFY(Textures[handle].RefCount>0);
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
    sVERIFY(tex->RefCount>=1);
    tex->RefCount--;
    if(tex->RefCount==0)
    {
      // if we delete a texture that was the current rendertarget,
      // make sure we reset the viewport
      if(CurrentTarget == handle)
      {
        sViewport vp;
        vp.Init();
        SetViewport(vp);
      }

      sRelease(tex->Tex);
      sRelease(tex->Shadow);

      tex->Flags = 0;
    }
  }
}

/****************************************************************************/

void sSystem_::GetTextureSize(sInt handle,sInt &w,sInt &h)
{
  if(handle != sINVALID)
  {
    sVERIFY(handle>=0 && handle<MAX_TEXTURE);
    sHardTex *tex = &Textures[handle];
    sVERIFY(tex->RefCount >= 1);

    w = tex->XSize;
    h = tex->YSize;
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

  DXERROR(DXDev->CreateCubeTexture(1,1,0,D3DFMT_Q8W8V8U8,D3DPOOL_MANAGED,&DXTinyNormalCube,0));
  for(i=0;i<6;i++)
  {
    DXERROR(DXTinyNormalCube->LockRect((D3DCUBEMAP_FACES)i,0,&lr,0,0));
    
    p = (sU8 *) lr.pBits;
    p[0] = p[1] = p[2] = 0;
    p[3] = 127;
    p[i/2] = (i&1) ? 127 : -128;
    
    DXERROR(DXTinyNormalCube->UnlockRect((D3DCUBEMAP_FACES)i,0));
  }
}

/****************************************************************************/

void sSystem_::MakeAttenuationVolume()
{
  const sInt size = 32;
  const sF32 scale = 2.0f / (size - 2.0f);
  const sF32 mid = size / 2.0f;
  D3DLOCKED_BOX lb;
  sInt x,y,z;
  sVector v;
  sF32 attn;
  sU8 *p;

  DXERROR(DXDev->CreateVolumeTexture(size,size,size,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&DXAttenuationVolume,0));
  DXERROR(DXAttenuationVolume->LockBox(0,&lb,0,0));

  for(z=0;z<size;z++)
  {
    v.z = (z - mid) * scale;

    for(y=0;y<size;y++)
    {
      v.y = (y - mid) * scale;
      p = ((sU8 *)lb.pBits)+z*lb.SlicePitch+y*lb.RowPitch;

      for(x=0;x<size;x++)
      {
        v.x = (x - mid) * scale;
        attn = sMax(1.0f - v.Dot3(v),0.0f);
        p[0] = p[1] = p[2] = p[3] = sFtol(attn * 255);
        p += 4;
      }
    }
  }

  DXERROR(DXAttenuationVolume->UnlockBox(0));
}

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
/****************************************************************************/

void sSystem_::SetStates(sU32 *stream)
{
//  DXDev->SetPixelShader(0);
//  DXDev->SetVertexShader(0);
  while(*stream!=0xffffffff)
  {
    if(CurrentStates[stream[0]] != stream[1])
    {
      SetState(stream[0],stream[1]);
      CurrentStates[stream[0]] = stream[1];
    }

    stream+=2;
  }
}

/****************************************************************************/

void sSystem_::SetState(sU32 token,sU32 value)
{
  if(CurrentStates[token] == value)
    return;

  if(token<0x0310)
  {
    if(token<0x0100)
    {
      DXDev->SetRenderState((D3DRENDERSTATETYPE)token,value);
    }
    else if(token<0x0200)
    {
      DXDev->SetTextureStageState(((token>>5)&7),(D3DTEXTURESTAGESTATETYPE)(token&31),value);
    }
    else if(token<0x0300)
    {
      DXDev->SetSamplerState(((token>>4)&15),(D3DSAMPLERSTATETYPE)(token&15),value);
    }
    else if(token<0x0310)
    {
      DXDev->SetSamplerState(D3DDMAPSAMPLER,(D3DSAMPLERSTATETYPE)(token&15),value);
    }
  }

  CurrentStates[token] = value;
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

void sSystem_::CreateGeoBuffer(sInt i,sInt dyn,sInt index,sInt size)
{
  sInt usage;
  D3DPOOL pool;

  sVERIFY(dyn==0 || dyn==1);
  sVERIFY(index>=0 || index<=2);

  GeoBuffer[i].Type = 1+index;
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

  if(!index)
  {
    DXERROR(DXDev->CreateVertexBuffer(size,usage,0,pool,&GeoBuffer[i].VB,0));
  }
  else
  {
    DXERROR(DXDev->CreateIndexBuffer(size,usage,index == 1 ? D3DFMT_INDEX16 : D3DFMT_INDEX32,pool,&GeoBuffer[i].IB,0));
  }
#if LOGGING
  sDPrintF("Create %s %s-Buffer (%d bytes) id %d\n",dyn?"dynamic":"static",index?"index":"vertex",size,i);
#endif
#ifdef _DOPE
  BufferMemAlloc += size;
#endif
}

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
      return i;
    }
  }
  sFatal("GeoAdd() ran out of handles");
	return 0;
}

void sSystem_::GeoRem(sInt handle)
{
  sVERIFY(handle>=1 && handle<MAX_GEOHANDLE);
  sVERIFY(GeoHandle[handle].Mode!=0);

  if(GeoHandle[handle].Vertex.Buffer>=4)
    GeoBuffer[GeoHandle[handle].Vertex.Buffer].Free();
  if(GeoHandle[handle].Index.Buffer>=4)
    GeoBuffer[GeoHandle[handle].Index.Buffer].Free();
  GeoHandle[handle].Mode = 0;
}

void sSystem_::GeoFlush()
{
  sInt i;

  for(i=1;i<MAX_GEOHANDLE;i++)
    GeoHandle[i].Vertex.Count = 0;
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
  if(what & sGEO_VERTEX)  GeoHandle[handle].Vertex.DiscardCount = 0;
  if(what & sGEO_INDEX)   GeoHandle[handle].Index.DiscardCount = 0;
}

sInt sSystem_::GeoDraw(sInt &handle,sInt drawCount)
{
  sGeoHandle *gh;
  D3DPRIMITIVETYPE mode;
  sInt count;
  sInt update;

// buffer correctly loaded?

  sVERIFY(handle>=1 && handle<MAX_GEOHANDLE);
  sVERIFY(GeoHandle[handle].Mode!=0);

  gh = &GeoHandle[handle];
  update = 0;

  if(gh->Vertex.Count == 0)
    update |= sGEO_VERTEX | sGEO_INDEX;
  if(gh->Vertex.Buffer == 0 && gh->Vertex.DiscardCount != DiscardCount[0])
    update |= sGEO_VERTEX;
  if(gh->Index.Buffer == 1 && gh->Index.DiscardCount != DiscardCount[1])
    update |= sGEO_INDEX;
  if(gh->Index.Buffer == 2 && gh->Index.DiscardCount != DiscardCount[2])
    update |= sGEO_INDEX;

  if(update)
    return update;

// set up buffers
  if(gh->FVF != LastDecl)
  {
    DXDev->SetVertexDeclaration(FVFTable[gh->FVF].Decl);
    LastDecl = gh->FVF;
  }

  if(gh->Index.Count && LastIB != gh->Index.Buffer)
  {
    sInt ib = gh->Index.Buffer;

    sVERIFY(ib>=0 && ib<MAX_GEOBUFFER);
    sVERIFY(GeoBuffer[ib].Type >= 2 && GeoBuffer[ib].Type <= 3);
    sVERIFY(GeoBuffer[ib].IB);
    DXDev->SetIndices(GeoBuffer[ib].IB);
    LastIB = ib;
  }
  if(LastVB != gh->Vertex.Buffer || LastVSize != gh->VertexSize)
  {
    sInt vb = gh->Vertex.Buffer;

    sVERIFY(vb>=0 && vb<MAX_GEOBUFFER);
    sVERIFY(GeoBuffer[vb].Type == 1);
    sVERIFY(GeoBuffer[vb].VB);
    DXDev->SetStreamSource(0,GeoBuffer[vb].VB,0,gh->VertexSize);
    LastVB = vb;
    LastVSize = gh->VertexSize;
  }

// start drawing

  count = drawCount ? drawCount : gh->Index.Count;
  if(count == 0)
    count = gh->Vertex.Count;

#if !sINTRO
  PerfThis.Vertex += gh->Vertex.Count;
  PerfThis.Batches++;
#endif

  switch(gh->Mode&7)
  {
  case sGEO_POINT:
    mode = D3DPT_POINTLIST;
    count = count;
    sVERIFY(gh->Index.Count==0);
    break;
  case sGEO_LINE:
    mode = D3DPT_LINELIST;
    count = count/2;
#if !sINTRO
    PerfThis.Line += count;
#endif
    break;
  case sGEO_LINESTRIP:
    mode = D3DPT_LINESTRIP;
    count = count-1;
#if !sINTRO
    PerfThis.Line += count;
#endif
    break;
  case sGEO_TRI:
    mode = D3DPT_TRIANGLELIST;
    count = count/3;
#if !sINTRO
    PerfThis.Triangle += count;
#endif
    break;
  case sGEO_TRISTRIP:
    mode = D3DPT_TRIANGLESTRIP;
    count = count-2;
#if !sINTRO
    PerfThis.Triangle += count;
#endif
    break;
  case sGEO_QUAD:
    sVERIFY(gh->Index.Count==0);
#if !sINTRO
    PerfThis.Triangle += count/2;
#endif
    LastIB = 3;
    DXDev->SetIndices(GeoBuffer[3].IB);
    DXDev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,gh->Vertex.Start,0,gh->Vertex.Count,0,count/4*2);
    return sFALSE;
  default:
    sVERIFYFALSE;
  }
  
  if(gh->Index.Count)
    DXDev->DrawIndexedPrimitive(mode,gh->Vertex.Start,0,gh->Vertex.Count,gh->Index.Start,count);
  else if(count)
    DXDev->DrawPrimitive(mode,gh->Vertex.Start,count);

  return sFALSE;
}

void sSystem_::AllocBufferInternal(sGeoBufferRef &ref,sInt size,sBool stat,sInt index,void **ptr)
{
  sInt i,lockf;
  sBool ok;

  if(stat) // static buffer
  {
    for(i=4;i<MAX_GEOBUFFER;i++)
      if(GeoBuffer[i].Alloc(ref.Count,size,ref.Start,index+1))
        break;

    if(i == MAX_GEOBUFFER)
    {
      for(i=4;i<MAX_GEOBUFFER;i++)
      {
#pragma lekktor(off)
        if(GeoBuffer[i].Type == 0)
        {
          sInt bufSize = index ? MAX_DYNIBSIZE * index : MAX_DYNVBSIZE;
          if(bufSize < ref.Count * size)
            bufSize = ref.Count * size;

          CreateGeoBuffer(i,0,index,bufSize);
          ok = GeoBuffer[i].Alloc(ref.Count,size,ref.Start,index+1);
          sVERIFY(ok);
          break;
#pragma lekktor(on)
        }
      }

      if(i == MAX_GEOBUFFER)
        sFatal("AllocInternal(): out of static geobuffers");
    }

    ref.Buffer = i;
    lockf = 0;
  }
  else // dynamic buffer
  {
    ok = GeoBuffer[index].Alloc(ref.Count,size,ref.Start,index+1);
    ref.Buffer = index;

    if(!ok)
    {
#pragma lekktor(off)
      GeoBuffer[index].Used = 0;
      DiscardCount[index]++;
      ok = GeoBuffer[index].Alloc(ref.Count,size,ref.Start,index+1);
      sVERIFY(ok);
#pragma lekktor(on)
    }

    ref.DiscardCount = DiscardCount[index];
    lockf = ref.Start ? D3DLOCK_NOOVERWRITE : D3DLOCK_DISCARD;
  }
  
  DXERROR(GeoBuffer[ref.Buffer].VB->Lock(ref.Start*size,ref.Count*size,(void **)ptr,lockf));
}

// Compare two shaders, return length if equal.
static sInt checkShadersEqual(const sU32 *sh1,const sU32 *sh2)
{
  if(!sh1 && !sh2)
    return 1;

  if(!sh1 || !sh2)
    return 0;

  sInt i;
  for(i=0;sh1[i] == sh2[i] && sh1[i] != XO_END;i++);

  return sh1[i] == sh2[i] ? i + 1 : 0;
}

sInt sSystem_::GetShader(const sU32 *bytecode)
{
  // search for matching shader
  sInt holePos = -1;

  for(sInt i=0;i<MAX_SHADERS;i++)
  {
    if(Shaders[i].RefCount)
    {
      if(checkShadersEqual(bytecode,Shaders[i].Code))
      {
        Shaders[i].AddRef();
        return i;
      }
    }
    else if(holePos == -1)
      holePos = i;
  }

  // need to add a new one
  sVERIFY(holePos != -1);
  sInt i = holePos;
  sInt len = checkShadersEqual(bytecode,bytecode);

  Shaders[i].RefCount = 1;
  if(bytecode)
  {
    Shaders[i].Code = new sU32[len];
    sCopyMem(Shaders[i].Code,bytecode,len*4);
  }
  else
    Shaders[i].Code = 0;

  Shaders[i].Shader = 0;
  if(bytecode)
  {
    HRESULT hr;
    sU32 type = bytecode[0] >> 16;

    if(type == 0xfffe) // vertex shader
      hr = DXDev->CreateVertexShader((DWORD *) bytecode,(IDirect3DVertexShader9 **) &Shaders[i].Shader);
    else if(type == 0xffff) // pixel shader
      hr = DXDev->CreatePixelShader((DWORD *) bytecode,(IDirect3DPixelShader9 **) &Shaders[i].Shader);
    else
      sVERIFYFALSE; // invalid shader bytecode!

#if !sPLAYER
    if(FAILED(hr))
      PrintShader(bytecode);
#endif
  }

  return i;
}

void sSystem_::GeoBegin(sInt handle,sInt vc,sInt ic,sF32 **fp,void **ip,sInt upd)
{
  sGeoHandle *gh;

  sVERIFY(handle>=1 && handle<MAX_GEOHANDLE);
  sVERIFY(GeoHandle[handle].Mode!=0);
  sVERIFY(ic*2<=MAX_DYNIBSIZE);

  gh = &GeoHandle[handle];

  if(fp)
    *fp = 0;
  if(ip)
    *ip = 0;

  if(upd & sGEO_VERTEX)
  {
    if(gh->Vertex.Buffer>=4)
      GeoBuffer[gh->Vertex.Buffer].Free();

    gh->Vertex.Buffer = 0;
    gh->Vertex.Start = 0;
    gh->Vertex.Count = vc;

    AllocBufferInternal(gh->Vertex,gh->VertexSize,gh->Mode & sGEO_STATVB,0,(void **)fp);
    gh->Locked |= sGEO_VERTEX;

    sVERIFY(vc*GeoHandle[handle].VertexSize <= GeoBuffer[gh->Vertex.Buffer].Size);
  }

  if(upd & sGEO_INDEX)
  {
    if(gh->Index.Buffer>=4)
      GeoBuffer[gh->Index.Buffer].Free();

    gh->Index.Buffer = 0;
    gh->Index.Start = 0;
    gh->Index.Count = ic;

    if(ic)
    {
      sBool is32b = (gh->Mode & sGEO_IND32B);

      AllocBufferInternal(gh->Index,is32b ? 4 : 2,gh->Mode & sGEO_STATIB,is32b ? 2 : 1,(void **)ip);
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
    GeoBuffer[gh->Vertex.Buffer].VB->Unlock();
    gh->Locked &= ~sGEO_VERTEX;
  }

  if(gh->Locked & sGEO_INDEX)
  {
    GeoBuffer[gh->Index.Buffer].IB->Unlock();
    gh->Locked &= ~sGEO_INDEX;
  }

  if(vc!=-1)
    gh->Vertex.Count = vc;
  if(ic!=-1)
    gh->Index.Count = ic;
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

  sRelease(tex->Tex);
  sRelease(tex->Shadow);
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
			  if(xs<=1 || ys<=1 || (xs&(xs-1)) || (ys&(ys-1)))
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
  sInt dxtquality;

  sInt x,y;
  sInt bpr,oxs;
  sU32 *d,*s,*data;
  sU16 *d16,*s16;
//  sU8 *d8;

  mipdir = miptresh & 16;
  alpha = miptresh & 32;
  dxtquality = miptresh >> 6;
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
#if sLINK_RYGDXT
        if(tex->Format==sTF_DXT1 || tex->Format==sTF_DXT5)
        {
          sU8 pixels[16*4];
          sU32 block[4];

          sInt shiftX = sGetPower2(oxs*4);

          s16 = (sU16 *) s;
          for(y=0;y<ys;y+=4)
          {
            for(x=0;x<xs;x+=4)
            {
              sU8 *dp = pixels;

              for(sInt yb=0;yb<4;yb++)
              {
                sU16 *sp = s16 + (((y+yb) & (ys-1))<<shiftX);

                for(sInt xb=0;xb<4;xb++)
                {
                  sU16 *sr = sp + ((x+xb) & (xs-1))*4;

                  dp[0] = (sr[0]>>7) & 0xff;
                  dp[1] = (sr[1]>>7) & 0xff;
                  dp[2] = (sr[2]>>7) & 0xff;
                  dp[3] = (sr[3]>>7) & 0xff;
                  dp += 4;
                }
              }

              sCompressDXTBlock((sU8 *) block,(sU32 *) pixels,tex->Format==sTF_DXT5,dxtquality);
              d[0] = block[0];
              d[1] = block[1];
              if(tex->Format == sTF_DXT1)
                d += 2;
              else
              {
                d[2] = block[2];
                d[3] = block[3];
                d += 4;
              }
            }
          }
        }
        else 

#endif
        if(tex->Format==sTF_A8)
        {
          sU8 *d8 = (sU8 *)d;
			    for(y=0;y<ys;y++)
			    {
            s16 = (sU16 *)s;
            for(x=0;x<xs;x++)
            {
              d8[x] = (s16[x*4+3]>>7)&0xff;
            }
				    d8+=bpr;
				    s+=oxs*2;
          }
        }
        else if(tex->Format==sTF_I8)
        {
          sU8 *d8 = (sU8 *)d;
			    for(y=0;y<ys;y++)
			    {
            s16 = (sU16 *)s;
            for(x=0;x<xs;x++)
            {
              d8[x] = (s16[x*4+1]>>7)&0xff;
            }
				    d8+=bpr;
				    s+=oxs*2;
          }
        }
        else if(tex->Format==sTF_A8I8)
        {
          sU8 *d8 = (sU8 *)d;
			    for(y=0;y<ys;y++)
			    {
            s16 = (sU16 *)s;
            for(x=0;x<xs;x++)
            {
              d8[x*2+0] = (s16[x*4+1]>>7)&0xff;
              d8[x*2+1] = (s16[x*4+3]>>7)&0xff;
            }
				    d8+=bpr;
				    s+=oxs*2;
          }
        }
        else
        {
			    for(y=0;y<ys;y++)
			    {
            if(tex->Format==sTF_Q8W8V8U8)
            {
              s16 = (sU16 *)s;
              for(x=0;x<xs;x++)
              {
#if sINTRO
                d[x] = (((((sInt)s16[2]-0x4000)>>7)&0xff)    ) | 
                      (((((sInt)s16[1]-0x4000)>>7)&0xff)<< 8) | 
                      (((((sInt)s16[0]-0x4000)>>7)&0xff)<<16) | 
                      (((((sInt)s16[3]-0x4000)>>7)&0xff)<<24);
#else // more exact but bigger (and slower) version
                sVector v;
                v.x = s16[2] - 0x4000;
                v.y = s16[1] - 0x4000;
                v.z = s16[0] - 0x4000;
                v.UnitSafe3();

                d[x] = ((sInt(v.x * 127.0f) & 0xff)<< 0) |
                       ((sInt(v.y * 127.0f) & 0xff)<< 8) |
                       ((sInt(v.z * 127.0f) & 0xff)<<16) |
                       (((((sInt)s16[3]-0x4000)>>7)&0xff)<<24);
#endif

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

				    d+=bpr/4;
				    s+=oxs*2;
			    }
        }
        mst->UnlockRect(level);

        if(xs<=1 || ys<=1 || (tex->MipLevels && level+1>=tex->MipLevels))
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

void sSystem_::ReadTexture(sInt handle,sU16 *dest)
{
  sHardTex *tex;
  IDirect3DTexture9 *mst;
  IDirect3DSurface9 *surf;
  IDirect3DSurface9 *rsurf;
  IDirect3DSurface9 *tsurf;
  D3DLOCKED_RECT dxlock;
  sInt xs,ys;

  sInt x,y;
  sInt bpr;
  sU32 *s;
 
  tex = &Textures[handle];
  mst = tex->Tex;
  DXERROR(mst->GetSurfaceLevel(0,&tsurf));
  DXERROR(DXDev->CreateRenderTarget(tex->XSize,tex->YSize,D3DFMT_A8R8G8B8,D3DMULTISAMPLE_NONE,0,0,&rsurf,0));
  DXERROR(DXDev->CreateOffscreenPlainSurface(tex->XSize,tex->YSize,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&surf,0));
  DXERROR(DXDev->StretchRect(tsurf,0,rsurf,0,D3DTEXF_NONE));
  DXERROR(DXDev->GetRenderTargetData(rsurf,surf));

  xs = tex->XSize;
  ys = tex->YSize;

  DXERROR(surf->LockRect(&dxlock,0,0));

  bpr = dxlock.Pitch;
  s = (sU32 *)dxlock.pBits;
  for(y=0;y<ys;y++)
  {
    for(x=0;x<xs;x++)
    {
      sU32 val = s[x];
      for(sInt i=0;i<4;i++)
      {
        sU32 bits = ((val>>(i*8))&255);
        dest[i] = (bits<<7) | (bits>>1);
      }
      dest+=4;
    }
	  s+=bpr/4;
  }
  surf->UnlockRect();
  sRelease(surf);
  sRelease(rsurf);
  sRelease(tsurf);
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
static sU32 KeyDown[MAX_KEYQUEUE];
static sInt KeyDownIndex;
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

    { DIK_NUMPAD0       ,'0'         },
    { DIK_NUMPAD1       ,'1'         },
    { DIK_NUMPAD2       ,'2'         },
    { DIK_NUMPAD3       ,'3'         },
    { DIK_NUMPAD4       ,'4'         },
    { DIK_NUMPAD5       ,'5'         },
    { DIK_NUMPAD6       ,'6'         },
    { DIK_NUMPAD7       ,'7'         },
    { DIK_NUMPAD8       ,'8'         },
    { DIK_NUMPAD9       ,'9'         },
    { DIK_NUMPADENTER   ,sKEY_ENTER  },
    { DIK_DECIMAL       ,'.'         },
    { DIK_NUMPADCOMMA   ,','         },
    { DIK_DIVIDE        ,'/'         },
    { DIK_MULTIPLY      ,'*'         },  
    { DIK_SUBTRACT      ,'-'         },
    { DIK_ADD           ,'+'         },
    { DIK_NUMPADEQUALS  ,'='         },

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
  KeyDownIndex = 0;
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

#pragma lekktor(off)

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
      for(i=0;i<KeyDownIndex;i++)
      {
        if(KeyIndex<MAX_KEYBUFFER)
          KeyBuffer[KeyIndex++] = KeyDown[i] | sKEYQ_BREAK;
      }

      KeyDownIndex = 0;
      KeyQual = 0;
      KeyRepeat = 0;
      KeyRepeatTimer = 0;
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

          for(sInt j=0;j<KeyDownIndex;j++)
          {
            if(KeyDown[j] == (key & ~(sKEYQ_BREAK | sKEYQ_STICKYBREAK)))
            {
              KeyDown[j] = KeyDown[--KeyDownIndex];
              break;
            }
          }
        }
        else
        {
          KeyRepeat = key;
          KeyRepeatTimer = time+KeyRepeatDelay;
          KeyStickyMouseX = MouseX;
          KeyStickyMouseY = MouseY;
          KeyStickyTime = sSystem->GetTime();
          KeySticky = sTRUE;

          if(KeyDownIndex<MAX_KEYQUEUE)
            KeyDown[KeyDownIndex++] = key;
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

#pragma lekktor(on)

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Windows User Interface Access                                      ***/
/***                                                                      ***/
/****************************************************************************/

#if !sPLAYER

const sChar *sSystem_::GetCommandLine()
{
  return WFilename[0] ? WFilename : 0;
}

sBool sSystem_::FileRequester(sChar *buffer,sInt size,sU32 flags,const sChar *ext)
{
  sChar oldpath[2048];
  sChar extstring[2048];
  OPENFILENAME ofn;
  sInt result=0;

  if(ext==0)
  {
    sCopyString(extstring,"All\0*.*\0",sCOUNTOF(extstring));
  }
  else
  {
    sChar *d = extstring;
    const sChar *s = ext;
    while(*s)
    {
      while(*s && *s!=':')
        *d++ = *s++;
      *d++ = 0;
      if(*s==':')
      {
        sBool comma = 0;
        while(*s==':' || *s==',')
        {
          s++;
          if(comma)
            *d++ = ';';
          *d++ = '*';
          *d++ = '.';
          while(*s && *s!=',' && *s!=';')
            *d++ = *s++;
          comma = 1;
        }
        if(*s==';')
          s++;
      }
      else
      {
        *d++ = '*';
        *d++ = '.';
        *d++ = '*';
      }
      *d++ = 0;
    }
    *d++ = 0;
  }

  sSetMem(&ofn,0,sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = (HWND) Screen[0].Window;
  ofn.lpstrFile = buffer;
  ofn.nMaxFile = size;
  ofn.lpstrFilter = extstring;
  ofn.nFilterIndex = 1;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  if(!CheckFile(buffer) && !CheckDir(buffer))
    buffer[0] = 0;

  GetCurrentDirectoryA(sCOUNTOF(oldpath),oldpath);

  switch(flags & sFRF_MODEMASK)
  {
  case sFRF_OPEN:
    result = GetOpenFileName(&ofn);
    break;
  case sFRF_SAVE:
    result = GetSaveFileName(&ofn);
    break;
  case sFRF_PATH:
    sVERIFYFALSE;
    break;
  }

  SetCurrentDirectoryA(oldpath);

  if(result)
  {
    sInt len = sGetStringLen(oldpath);
    if(sCmpMemI(oldpath,buffer,len)==0)
    {
      sChar b[2048];
      sCopyString(b,".\\",2048);
      if(buffer[len]=='/' || buffer[len]=='\\')
        len++;
      sAppendString(b,buffer+len,2048);
      sCopyString(buffer,b,2048);
    }
  }

  return result;
}

sBool sSystem_::ExecuteShell(sChar *verb,sChar *file)
{
  HINSTANCE inst = ShellExecute(0,verb,file,0,0,SW_SHOWNORMAL);
  return sDInt(inst)>32;
}
  
void sSystem_::SetClipboard(const sChar *text,sInt len)
{
  OpenClipboard((HWND)Screen[0].Window);
  EmptyClipboard();

  if(len==-1)
    len = sGetStringLen(text);
  sInt size = len+1;

  for(sInt i=0;text[i];i++)
    if(text[i]=='\n')
      size++;

  HANDLE hmem = GlobalAlloc(GMEM_MOVEABLE,size);

  sChar *d = (sChar *) GlobalLock(hmem);
  for(sInt i=0;i<len;i++)
  {
    if(text[i]=='\n')
      *d++ = '\r';
    *d++ = text[i];
  }
  *d++ = 0;
  GlobalUnlock(hmem);

  SetClipboardData(CF_TEXT,hmem);
  CloseClipboard();
}

sChar *sSystem_::GetClipboard()
{
  sChar *result=0;

  OpenClipboard((HWND)Screen[0].Window);

  HANDLE hmem = GetClipboardData(CF_TEXT);
  if(hmem)
  {
    sChar *s = (sChar *)GlobalLock(hmem);
    sInt size = sGetStringLen(s)+1;
    result = new sChar[size];
    sInt i = 0;

    while(*s)
    {
      if(*s!='\r')
        result[i++] = *s;
      s++;
    }
    result[i++] = 0;

    GlobalUnlock(hmem);
  }
  else
  {
    result = new sChar[1];
    result[0] = 0;
  }

  CloseClipboard();

  return result;
}

void sSystem_::SetClipboard(sBitmap *bm)
{
  OpenClipboard((HWND)Screen[0].Window);
  EmptyClipboard();


  HBITMAP hbm = CreateBitmap(bm->XSize,bm->YSize,1,32,bm->Data);

  SetClipboardData(CF_BITMAP,hbm);
  CloseClipboard();
}



static sInt ProgressGDIMax;
static sInt ProgressGDITime;
static sInt ProgressGDIValue;
static sInt ProgressGDIActive;
static sInt ProgressGDIAbort;

void sSystem_::ProgressGDIStart(sInt max)
{
  ProgressGDIMax = max;
  ProgressGDITime = GetTime()+500;
  ProgressGDIValue = 0;
  ProgressGDIActive = 0;
  ProgressGDIAbort = 0;
  ClearAbortKey();
}

void sSystem_::ProgressGDIStop()
{
  /*if(ProgressGDIValue!=0 || ProgressGDIMax!=0)
    sDPrintF("progress %d/%d\n",ProgressGDIValue,ProgressGDIMax);*/
  if(ProgressGDIActive)
  {
    ProgressGDIValue = ProgressGDIMax;
    ProgressGDI(0);
  }
  ProgressGDIMax = 0;
}

sBool sSystem_::ProgressGDI(sInt increment)
{
  ProgressGDIValue += increment;
  sInt max = ProgressGDIMax;
  if(max>3)
  {
    sInt value = ProgressGDIValue;
    sInt time = GetTime();

    if(time>ProgressGDITime)
    {
      ProgressGDIAbort |= GetAbortKey();
      HDC dc = GetDC((HWND)Screen[0].Window);
      HBRUSH br0 = CreateSolidBrush(RGB(0,0,0));
      HBRUSH br1 = CreateSolidBrush(RGB(255,255,255));
      sScreenInfo si;
      ProgressGDITime = time+500;
      ProgressGDIActive = 1;

      GetScreenInfo(0,si);

      sRect r,rr;
      sInt h;
      sInt w = 500; if(si.XSize<500) w = si.XSize;
      r.x0 = si.XSize/2-w/2;
      r.y0 = si.YSize/2-14;
      r.x1 = si.XSize/2+w/2;
      r.y1 = si.YSize/2+14;

      h = 2;
      rr.Init(r.x0,r.y0,r.x1,r.y0+h);
      FillRect(dc,(const RECT *)&rr,br0);
      rr.Init(r.x0,r.y1-h,r.x1,r.y1);
      FillRect(dc,(const RECT *)&rr,br0);
      rr.Init(r.x0,r.y0+h,r.x0+h,r.y1-h);
      FillRect(dc,(const RECT *)&rr,br0);
      rr.Init(r.x1-h,r.y0+h,r.x1,r.y1-h);
      FillRect(dc,(const RECT *)&rr,br0);
      r.Extend(-h);

      h = 2;
      rr.Init(r.x0,r.y0,r.x1,r.y0+h);
      FillRect(dc,(const RECT *)&rr,br1);
      rr.Init(r.x0,r.y1-h,r.x1,r.y1);
      FillRect(dc,(const RECT *)&rr,br1);
      rr.Init(r.x0,r.y0+h,r.x0+h,r.y1-h);
      FillRect(dc,(const RECT *)&rr,br1);
      rr.Init(r.x1-h,r.y0+h,r.x1,r.y1-h);
      FillRect(dc,(const RECT *)&rr,br1);
      r.Extend(-h);

      h = 2;
      rr.Init(r.x0,r.y0,r.x1,r.y0+h);
      FillRect(dc,(const RECT *)&rr,br0);
      rr.Init(r.x0,r.y1-h,r.x1,r.y1);
      FillRect(dc,(const RECT *)&rr,br0);
      rr.Init(r.x0,r.y0+h,r.x0+h,r.y1-h);
      FillRect(dc,(const RECT *)&rr,br0);
      rr.Init(r.x1-h,r.y0+h,r.x1,r.y1-h);
      FillRect(dc,(const RECT *)&rr,br0);
      r.Extend(-h);

      sInt x =  r.x0 + sMulDiv(r.XSize(),value,max);
      rr = r; rr.x0 = x;
      FillRect(dc,(const RECT *)&rr,br0);
      rr = r; rr.x1 = x;
      FillRect(dc,(const RECT *)&rr,br1);

      DeleteObject(br0);
      DeleteObject(br1);
      ReleaseDC((HWND)Screen[0].Window,dc);
    }
  }

  return ProgressGDIAbort;
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Misc Input/Timing stuff                                            ***/
/***                                                                      ***/
/****************************************************************************/

void sSystem_::GetInput(sInt id,sInputData &data)
{
#if !sUSE_DIRECTINPUT
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

#pragma lekktor(off)
  if(key & sKEYQ_SHIFT)
    sAppendString(buffer,"SHIFT+",size);
  if(key & sKEYQ_CTRL)
    sAppendString(buffer,"CTRL+",size);
  if(key & sKEYQ_ALTGR)
    sAppendString(buffer,"ALTGR+",size);
  if(key & sKEYQ_ALT)
    sAppendString(buffer,"ALT+",size);
#pragma lekktor(on)

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
#pragma lekktor(off)
  if(GetAsyncKeyState(VK_PAUSE)&1)
    WAbortKeyFlag = 1;
#pragma lekktor(on)
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

static sU64 GetPreciseTimer()
{
  sU64 time;

  if(SingleCore)
  {
    __asm
    {
      rdtsc;
      mov   dword ptr [time], eax;
      mov   dword ptr [time+4], edx;
    }
  }
  else
    QueryPerformanceCounter((LARGE_INTEGER *) &time);

  return time;
}

sU32 sSystem_::PerfTime()
{
  return (GetPreciseTimer()-sPerfFrame)*sPerfKalibFactor;
}

void sSystem_::PerfKalib()
{
  sU64 query;
  sU64 freq;
  sU64 time = GetPreciseTimer();

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

sBool sSystem_::GetWinMouseAbs(sInt &x,sInt &y)
{
  POINT pt;
  GetCursorPos(&pt);
  x = pt.x;
  y = pt.y;
  return sTRUE;
}

void sSystem_::SetWinMouse(sInt x,sInt y)
{
  RECT rc;
  GetWindowRect((HWND)Screen[0].Window,&rc);
  SetCursorPos(x+rc.left,y+rc.top);
}

void sSystem_::SetWinTitle(sChar *name)
{
  SetWindowText((HWND)Screen[0].Window,name);
}

void sSystem_::MoveWindow(sInt dx,sInt dy)
{
  RECT r;
  GetWindowRect((HWND)Screen[0].Window,&r);
  ::MoveWindow((HWND)Screen[0].Window,r.left+dx,r.top+dy,r.right-r.left,r.bottom-r.top,1);
}

void sSystem_::SetWinMode(sInt mode)
{
  switch(mode)
  {
  case 0:
    WShow = SW_RESTORE;
//    ShowWindow((HWND)Screen[0].Window,SW_RESTORE);
    break;
  case 1:
    WShow = SW_MAXIMIZE;
//    ShowWindow((HWND)Screen[0].Window,SW_MAXIMIZE);
    break;
  case 2:
    WShow = SW_MINIMIZE;
//    ShowWindow((HWND)Screen[0].Window,SW_MINIMIZE);
    break;
  }
}

sInt sSystem_::GetWinMode()
{
  if(IsIconic((HWND)Screen[0].Window)) return 2;
  if(IsZoomed((HWND)Screen[0].Window)) return 1;
  return 0;
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

#pragma lekktor(off)

static WAVEFORMATEX soundFormat = {
  WAVE_FORMAT_PCM,
  2,
  44100,
  44100*4,
  4,
  16,
  0
};

sBool sSystem_::InitDS()
{
  DWORD count1,count2;
  void *pos1,*pos2;
  HRESULT hr;
  DSBUFFERDESC dsbd;
  IDirectSoundBuffer *sbuffer;

  timeBeginPeriod(1);

  hr = (*DirectSoundCreate8P)(0,&DXS,0);
  if(FAILED(hr)) return sFALSE;

  hr = DXS->SetCooperativeLevel((HWND)Screen[0].Window,DSSCL_PRIORITY);
  if(FAILED(hr)) return sFALSE;

#if !sINTRO
  WINSET(dsbd);
  dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRL3D;
  dsbd.dwBufferBytes = 0;
  dsbd.lpwfxFormat = 0;
  hr = DXS->CreateSoundBuffer(&dsbd,&DXSPrimary,0);
  if(FAILED(hr)) return sFALSE;

  hr = DXSPrimary->QueryInterface(IID_IDirectSound3DListener,(void **)&Listener);
  if(FAILED(hr)) return sFALSE;
#else // no dsound3d support
  WINSET(dsbd);
  dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
  dsbd.dwBufferBytes = 0;
  dsbd.lpwfxFormat = 0;
  hr = DXS->CreateSoundBuffer(&dsbd,&DXSPrimary,0);
  if(FAILED(hr)) return sFALSE;
#endif

  DXSPrimary->SetFormat(&soundFormat);

  WINSET(dsbd);
  dsbd.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;

  dsbd.dwBufferBytes = DXSSAMPLES*4;
  dsbd.lpwfxFormat = &soundFormat;

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

  DXSEvent = CreateEvent(0,0,0,0);
  if(!DXSEvent)
    return sFALSE;

  InitializeCriticalSection(&DXSLock);

  DXSBuffer->Play(0,0,DSBPLAY_LOOPING);

  DXSRun = 0;
  DXSThread = CreateThread(0,16384,ThreadCode,0,0,&DXSThreadId);
  if(!DXSThread)
    return sFALSE;

  SoundTime = 1;
  DXSTime = timeGetTime();

  return sTRUE;
}

#pragma lekktor(on)

/****************************************************************************/

void sSystem_::ExitDS()
{
  if(Listener)
    Listener->Release();

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

#if sLINK_KKRIEGER
  SampleRemAll();
#endif

  if(DXSBuffer)
    DXSBuffer->Release();
  if(DXSPrimary)
    DXSPrimary->Release();
  if(DXS)
    DXS->Release();
}

/****************************************************************************/

#pragma lekktor(off)

void sSystem_::MarkDS()
{
  SetEvent(DXSEvent);

  EnterCriticalSection(&DXSLock);

  DXSLastTotalSample = DXSReadStart + DXSReadOffset + sMulDiv(timeGetTime()-DXSTime,44100,1000);

  LeaveCriticalSection(&DXSLock);
}

#pragma lekktor(on)

/****************************************************************************/

#pragma lekktor(off)

unsigned long __stdcall ThreadCode(void *)
{
  HRESULT hr;
  sInt play,dummy;
  DWORD count1,count2;
  void *pos1,*pos2;
  const sInt SAMPLESIZE = 4;
  sInt size,pplay;

  while(DXSRun==0)
  {
    WaitForSingleObject(DXSEvent,20/*(DXSSAMPLES*1000/44100)/(SAMPLESIZE*2)*/);
    EnterCriticalSection(&DXSLock);

    hr = DXSBuffer->GetCurrentPosition((DWORD*)&play,(DWORD*)&dummy);
    if(!FAILED(hr))
    {
      pplay = play;
      play = play/SAMPLESIZE;
      DXSReadOffset = play;
      DXSTime = timeGetTime();
      if(DXSIndex>play)
        play+=DXSSAMPLES;
      size = play-DXSIndex;


      size = (size)&(~(DXSHandlerAlign-1));
      if(size>0)
      {
        count1 = 0;
        count2 = 0;
        hr = DXSBuffer->Lock(DXSIndex*SAMPLESIZE,size*SAMPLESIZE,&pos1,&count1,&pos2,&count2,0);
        if(!FAILED(hr))
        {
          DXSIndex += size;
          if(DXSIndex>=DXSSAMPLES)
          {
            DXSIndex-=DXSSAMPLES;
            DXSReadStart += DXSSAMPLES;
          }
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

#pragma lekktor(on)

#endif

/****************************************************************************/
/****************************************************************************/

static void DSNullHandler(sS16 *data,sInt samples,void *user)
{
  static sInt p0,p1;

  sSetMem4((sU32 *)data,0x00000000,samples);
}

extern void DSNullHandler(sS16 *data,sInt samples,void *user);

/****************************************************************************/

void sSystem_::SetSoundHandler(sSoundHandler hnd,sInt align,void *user)
{
#if sUSE_DIRECTSOUND
  EnterCriticalSection(&DXSLock);

  if(hnd)
  {
    DXSHandler = hnd;
    DXSBuffer->Play(0,0,DSBPLAY_LOOPING);
  }
  else
  {
    DXSHandler = DSNullHandler;
    DXSBuffer->Stop();

#if !sINTRO
    HRESULT hr;
    DWORD count1,count2;
    void *pos1,*pos2;

    hr = DXSBuffer->Lock(0,DXSSAMPLES*4,&pos1,&count1,&pos2,&count2,0); // clear buffer, so that replay stops
    if(!FAILED(hr))
    {
      sSetMem(pos1,0,count1);
      sSetMem(pos2,0,count2);
      DXSBuffer->Unlock(pos1,count1,pos2,count2);
    }
#endif
  }
  if(align<0)
    align=64;
  DXSHandlerAlign = align;
  DXSHandlerSample = 0;
  DXSHandlerUser = user;

  DXSLastTotalSample = 0;
  DXSIndex = 0;
  DXSReadStart = -DXSSAMPLES;
  DXSReadOffset = 0;
  DXSTime = timeGetTime();
  
  LeaveCriticalSection(&DXSLock);
#endif
}
/****************************************************************************/

sInt sSystem_::GetCurrentSample()
{
#if sUSE_DIRECTSOUND
  return DXSLastTotalSample;
#else
  return 0;
#endif
}

/****************************************************************************/
/***                                                                      ***/
/***   Sample Player                                                      ***/
/***                                                                      ***/
/****************************************************************************/

#if sLINK_KKRIEGER

sInt sSystem_::SampleAdd(sS16 *data,sInt size,sInt buffers,sInt handle,sBool is3d)
{
  sInt i;
  IDirectSoundBuffer *dsb;
  DSBUFFERDESC dsbd;
  WAVEFORMATEX format;
  HRESULT hr;
  DWORD len;
  sSample *sam;
  sS16 *ptr;

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
    format.nChannels = is3d ? 1 : 2; 
    format.nSamplesPerSec = 44100; 
    format.nBlockAlign = is3d ? 2 : 4; 
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign; 
    format.wBitsPerSample = 16; 
    dsbd.dwFlags = (is3d ? DSBCAPS_CTRL3D : DSBCAPS_CTRLPAN) | DSBCAPS_CTRLVOLUME/* | DSBCAPS_CTRLFREQUENCY*/;
    dsbd.dwBufferBytes = size * format.nBlockAlign;
    dsbd.lpwfxFormat = &format;

#pragma lekktor(off)
    hr = DXS->CreateSoundBuffer(&dsbd,&dsb,0);
    if(FAILED(hr)) return sINVALID;

    hr = dsb->Lock(0,0,(void **)&ptr,&len,0,0,DSBLOCK_ENTIREBUFFER);
    if(FAILED(hr)) return sINVALID;
    sVERIFY(len==size*format.nBlockAlign);

    if(!is3d) // we can upload stereo samples
      sCopyMem(ptr,data,size*format.nBlockAlign);
    else // do mono downmix
      for(i=0;i<size;i++)
        ptr[i] = (data[i*2+0] + data[i*2+1]) >> 1;

    hr = dsb->Unlock(ptr,len,0,0);
    if(FAILED(hr)) return sINVALID;

    sam = &Sample[handle];
    sam->Buffers[0].Buffer = dsb;

    for(i=0;i<buffers;i++)
    {
      sam->Buffers[i].Buf3D = 0;
      sam->Buffers[i].PlayTime = 0;
    }

    for(i=1;i<buffers;i++)
    {
      hr = DXS->DuplicateSoundBuffer(dsb,&sam->Buffers[i].Buffer);
      if(FAILED(hr)) return sINVALID;
    }

    if(is3d)
    {
      for(i=0;i<buffers;i++)
      {
        if(!FAILED(sam->Buffers[i].Buffer->QueryInterface(IID_IDirectSound3DBuffer,(void **)&sam->Buffers[i].Buf3D)))
          sam->Buffers[i].Buf3D->SetMode(DS3DMODE_DISABLE,DS3D_DEFERRED);
      }
    }

    sam->LRU = 0;
    sam->Count = buffers;
    sam->Uses = 0;
    sam->Is3D = is3d;
    sam->LenMs = sMulDiv(len,1000,format.nAvgBytesPerSec) + 10;
#pragma lekktor(on)
  }
  return handle;
}


void sSystem_::SampleRem(sInt handle)
{
  sInt i;
  sSample *sam;
  sVERIFY(handle>=0 && handle<sMAXSAMPLEHANDLE);

  sam = &Sample[handle];

  for(i=0;i<sam->Count;i++)
  {
    sam->Buffers[i].Buffer->Release();
    if(sam->Buffers[i].Buf3D)
      sam->Buffers[i].Buf3D->Release();
  }

  sam->Count = 0;
  sam->LRU = 0;
}

void sSystem_::SampleRemAll()
{
  sInt i;
  for(i=0;i<sMAXSAMPLEHANDLE;i++)
    SampleRem(i);
}

static sInt lin2d3d(sF32 volume)
{
  volume = sRange(volume,1.0f,1e-20f);
  return sFLog(volume)*20*100;
}

sInt sSystem_::SamplePlay(sInt handle,sF32 volume,sF32 pan,sInt freq)
{
  sSample *sam;
  sSampleBuffer *buf;
  HRESULT hr;
  sInt d3dvol,d3dpan;

  sVERIFY(handle>=0 && handle<sMAXSAMPLEHANDLE);
  sam = &Sample[handle];
  if(sam->Count>0)
  {
    d3dvol = lin2d3d(volume);
    d3dpan = sRange<sInt>(pan*100,10000,-10000);

    sam->LRU = (sam->LRU+1)%sam->Count;
    buf = &sam->Buffers[sam->LRU];

    buf->Buffer->Stop();
    buf->Buffer->SetCurrentPosition(0);
    buf->Buffer->SetVolume(d3dvol);
    if(!sam->Is3D)
      buf->Buffer->SetPan(d3dpan);
    hr = buf->Buffer->Play(0,0,0);

    buf->PlayTime = SoundTime;

    return ++sam->Uses;
  }
  else
    return -1;
}

void sSystem_::Sample3DParam(sInt handle,sInt bufnum,const sVector &pos,const sVector &vel,sF32 minDist,sF32 maxDist)
{
  sSample *sam;
  DS3DBUFFER params;

  sVERIFY(handle>=0 && handle<sMAXSAMPLEHANDLE);
  sam = &Sample[handle];

  // get 3d buffer
  if(sam->Is3D && sam->Uses - bufnum < sam->Count) // if buffer has not been reallocated yet
  {
    bufnum %= sam->Count;
    if(sam->Buffers[bufnum].PlayTime) // if buffer hasn't been stopped yet
    {
      // set up parameters
      WINSET(params);
      
      params.vPosition.x = pos.x;
      params.vPosition.y = pos.y;
      params.vPosition.z = pos.z;
      
      params.vVelocity.x = vel.x;
      params.vVelocity.y = vel.y;
      params.vVelocity.z = vel.z;

      params.dwInsideConeAngle = 360; // in degrees... right.
      params.dwOutsideConeAngle = 360;
      params.vConeOrientation.z = 1.0f;
      params.lConeOutsideVolume = DS3D_DEFAULTCONEOUTSIDEVOLUME;
      params.flMinDistance = minDist;
      params.flMaxDistance = maxDist;
      params.dwMode = DS3DMODE_NORMAL;

      // set the parameters
      sam->Buffers[bufnum].Buf3D->SetAllParameters(&params,DS3D_IMMEDIATE);
    }
  }
}

void sSystem_::Sample3DListener(const sVector &pos,const sVector &vel,const sVector &up,const sVector &fwd,sF32 doppler,sF32 rolloff)
{
  DS3DLISTENER params;

  WINSET(params);
  params.vPosition.x = pos.x;
  params.vPosition.y = pos.y;
  params.vPosition.z = pos.z;
  params.vVelocity.x = vel.x;
  params.vVelocity.y = vel.y;
  params.vVelocity.z = vel.z;
  params.vOrientFront.x = fwd.x;
  params.vOrientFront.y = fwd.y;
  params.vOrientFront.z = fwd.z;
  params.vOrientTop.x = up.x;
  params.vOrientTop.y = up.y;
  params.vOrientTop.z = up.z;
  params.flDistanceFactor = 1.0f;
  params.flRolloffFactor = rolloff;
  params.flDopplerFactor = doppler;

  Listener->SetAllParameters(&params,DS3D_DEFERRED);
}

void sSystem_::Sample3DCommit()
{
  sInt i,j;
  sSample *sam;
  sSampleBuffer *buf;

  sZONE(Sound3D);

  // disable hanging 3d sounds
  for(i=0;i<sMAXSAMPLEHANDLE;i++)
  {
    sam = &Sample[i];
    if(sam->Is3D)
    {
      for(j=0;j<sam->Count;j++)
      {
        buf = &sam->Buffers[j];
        
        if(buf->PlayTime && buf->PlayTime + sam->LenMs < SoundTime)
        {
          buf->PlayTime = 0;
          buf->Buf3D->SetMode(DS3DMODE_DISABLE,DS3D_DEFERRED);
        }
      }
    }
  }

  // commit settings
  Listener->CommitDeferredSettings();
  SoundTime = sSystem->GetTime();
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   File                                                               ***/
/***                                                                      ***/
/****************************************************************************/

sU8 *sSystem_::LoadFile(const sChar *name,sInt &size)
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

sU8 *sSystem_::LoadFile(const sChar *name)
{
  sInt dummy;
  return LoadFile(name,dummy);
}

/****************************************************************************/


#if !sINTRO
//----------------------------------------------------------------------------
// get base name of current exe
//----------------------------------------------------------------------------
void sSystem_::GetModuleBaseName(sChar *buffer,sInt size)
{
  // Retrieves the full path for the file that contains the specified module
  char str[MAX_PATH];
  GetModuleFileName(NULL, str, MAX_PATH);

  // strip path
  sCopyString(buffer,sFileFromPathString(str),size);

  // remove extension (if present) to just keep base name
  sChar *ext = sFileExtensionString(buffer);
  if (ext)
    *ext = 0;
}
#endif

/****************************************************************************/

sU8 *sSystem_::LoadFileIfNewerThan(const sChar *name,const sChar *other,sInt &size)
{
  WIN32_FILE_ATTRIBUTE_DATA fa1,fa2;

  if(GetFileAttributesEx(name,GetFileExInfoStandard,&fa1) &&
    GetFileAttributesEx(other,GetFileExInfoStandard,&fa2) &&
    CompareFileTime(&fa1.ftLastWriteTime,&fa2.ftLastWriteTime) > 0)
    return LoadFile(name,size);
  else
    return 0;
}

/****************************************************************************/

sChar *sSystem_::LoadText(const sChar *name)
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

sBool sSystem_::SaveFile(const sChar *name,const sU8 *data,sInt size)
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

sU64 sSystem_::GetFileStamp(const sChar *name)
{
  WIN32_FILE_ATTRIBUTE_DATA data;

  if(GetFileAttributesEx(name,GetFileExInfoStandard,&data))
  {
    return *(sU64 *)&data.ftLastWriteTime;
  }
  else
  {
    return 0;
  }
}

/****************************************************************************/
/****************************************************************************/

#define sMAX_PATHNAME 4096
sDirEntry *sSystem_::LoadDir(const sChar *name)
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

sBool sSystem_::MakeDir(const sChar *name)
{
  return CreateDirectory(name,0) != 0;
}

/****************************************************************************/

sBool sSystem_::CheckDir(const sChar *name)
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

sBool sSystem_::CheckFile(const sChar *name)
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

sBool sSystem_::RenameFile(const sChar *oldname,const sChar *newname)
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

sBool sSystem_::DeleteFile(const sChar *name)
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

#if sLINK_PNG
#include "pngloader/loadpng.hpp"
#endif

/****************************************************************************/

#define HIMETRIC_PER_INCH         2540
#define MAP_PIX_TO_LOGHIM(x,ppli) ((HIMETRIC_PER_INCH*(x)+((ppli)>>1))/(ppli))
#define MAP_LOGHIM_TO_PIX(x,ppli) (((ppli)*(x)+HIMETRIC_PER_INCH/2)/HIMETRIC_PER_INCH)

sBitmap *sSystem_::LoadBitmap(const sU8 *data,sInt size)
{
  sInt x,y;
  sU8 *pic;
  sBitmap *bm;

  if(LoadBitmapCore(data,size,x,y,pic))
  {
    bm = new sBitmap;
    bm->Init(x,y);

    sCopyMem(bm->Data,pic,x*y*4);
    delete[] pic;

    return bm;
  }
  else
  {
    return 0;
  }
}

sBool sSystem_::LoadBitmapCore(const sU8 *data,sInt size,sInt &xout,sInt &yout,sU8 *&dataout)
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
//  sBitmap *bm;

  union _ULARGE_INTEGER usize;
  union _LARGE_INTEGER useek;
  union _ULARGE_INTEGER udummy;

  xout = 0;
  yout = 0;
  dataout = 0;

#if sLINK_PNG
  if(LoadPNG(data,size,xout,yout,dataout))
    return sTRUE;
#endif
#if sTEXTUREONLY
  return sFALSE;
#else
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

      xout = xs = sMakePower2(wpp.x);
      yout = ys = sMakePower2(-wpp.y);

      dataout = new sU8[xs*ys*4];
      hbm = CreateCompatibleBitmap(screendc,xs,ys);
      SelectObject(hdc,hbm);

      sSetMem4((sU32 *)dataout,0xffff0000,xs*ys);
      bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
      bmi.bmiHeader.biWidth = xs;
      bmi.bmiHeader.biHeight = -ys;
      bmi.bmiHeader.biPlanes = 1;
      bmi.bmiHeader.biBitCount = 32;
      bmi.bmiHeader.biCompression = BI_RGB;

      hr = pic->SelectPicture(hdc2,&hdcold,(unsigned int *)&hbm2);
      if(!FAILED(hr))
      {
        SetMapMode(hdc2,MM_TEXT);
        StretchBlt(hdc,0,0,xs,ys,hdc2,0,0,wpp.x,-wpp.y,SRCCOPY);
        GetDIBits(hdc,hbm,0,ys,dataout,&bmi,DIB_RGB_COLORS);
        pic->SelectPicture(hdcold,0,(unsigned int *)&hbm2);
      }
      pic->Release();

      DeleteDC(hdc2);
      DeleteDC(hdc);
      DeleteObject(hbm);
      for(i=0;i<xs*ys;i++)
        ((sU32 *)dataout)[i] |= 0xff000000;
    }
    str->Release();
  }
  ReleaseDC(0,screendc);

  return dataout!=0;
#endif
}

/****************************************************************************/
/***                                                                      ***/
/***   Host Font Interface                                                ***/
/***                                                                      ***/
/****************************************************************************/

sInt sSystem_::FontBegin(sInt pagex,sInt pagey,const sChar *name,sInt xs,sInt ys,sInt style)
{
  //LOGFONT lf;
  TEXTMETRIC met;

  FontBMI.bmiHeader.biSize = sizeof(FontBMI.bmiHeader);
  FontBMI.bmiHeader.biWidth = pagex;
  FontBMI.bmiHeader.biHeight = -pagey;
  FontBMI.bmiHeader.biPlanes = 1;
  FontBMI.bmiHeader.biBitCount = 32;
  FontBMI.bmiHeader.biCompression = BI_RGB;
  FontHBM = CreateDIBSection(GDIScreenDC,&FontBMI,DIB_RGB_COLORS,
    (void **) &FontMem,0,0);
  SelectObject(GDIDC,FontHBM);

  if(name[0]==0) name = "arial";

  /*lf.lfHeight         = -ys; 
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

  FontHandle = CreateFontIndirect(&lf);*/

  FontHandle = CreateFont(-ys,xs,0,0,(style&2) ? FW_BOLD : FW_NORMAL,
    (style&1) ? 1 : 0,0,0,DEFAULT_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,
    PROOF_QUALITY,
    //(style&4) ? ANTIALIASED_QUALITY : NONANTIALIASED_QUALITY,
    DEFAULT_PITCH|FF_DONTCARE,name);

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

sInt sSystem_::FontWidth(const sChar *string,sInt len)
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

void sSystem_::FontCharWidth(sInt ch,sInt *widths)
{
  ABC abc;

  GetCharABCWidths(GDIDC,ch,ch,&abc);

  widths[0] = abc.abcA;
  widths[1] = abc.abcB;
  widths[2] = abc.abcC;
}

void sSystem_::FontPrint(sInt x,sInt y,const sChar *string,sInt len)
{
  if(len==-1)
  {
    len++;
    while(string[len]) len++;
  }
  ExtTextOut(GDIDC,x,y,0,0,string,len,0);
}

void sSystem_::FontPrint(sInt x,sInt y,const sWChar *string,sInt len)
{
  if(len==-1)
  {
    len++;
    while(string[len]) len++;
  }
  ExtTextOutW(GDIDC,x,y,0,0,string,len,0);
}

void sSystem_::FontEnd()
{
  SelectObject(GDIDC,GDIHBM);
  DeleteObject(FontHBM);
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

sBool sGeoBuffer::Alloc(sInt count,sInt size,sInt &firstp,sInt type)
{
  sInt pos,first;

  if(Type!=type)
    return sFALSE;

  first = (Used+size-1)/size;
  pos = (first+count)*size;
  if(pos>Size)
    return sFALSE;

  Used = pos;
  firstp = first;
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

void sMaterialEnv::Init()
{
  sSetMem(this,0,sizeof(*this));

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
  sF32 shiftX = 1.0f / sSystem->ViewportX;
  sF32 shiftY = 1.0f / sSystem->ViewportY;

  switch(Orthogonal)
  {
#if !sINTRO
  case sMEO_PIXELS: // 0 .. screen_max
    pmat.i.Init(2.0f/sSystem->ViewportX,0                       ,0 ,0);
    pmat.j.Init(0                      ,-2.0f/sSystem->ViewportY,0 ,0);
    pmat.k.Init(0                      ,0                       ,1 ,0);
    pmat.l.Init(-1 - (2*CenterX+1)*shiftX,1 + (2*CenterY+1)*shiftY,0 ,1);
    break;
#endif

  case sMEO_NORMALISED: // -1 .. 1
    pmat.i.Init(ZoomX                  ,0                       ,0 ,0);
    pmat.j.Init(0                      ,ZoomY                   ,0 ,0);
    pmat.k.Init(0                      ,0                       ,1.0f/FarClip,0);
    pmat.l.Init(-shiftX                ,shiftY                  ,0 ,1);
    break;

  case sMEO_PERSPECTIVE:
    //q = FarClip/(FarClip-NearClip);
    q = 1.0f;
    pmat.i.Init(ZoomX         ,0               , 0              ,0);
    pmat.j.Init(0             ,ZoomY           , 0              ,0);
    pmat.k.Init(CenterX       ,CenterY         , q              ,1);
    pmat.l.Init(-shiftX       ,shiftY          ,-q*NearClip     ,0);
    break;
  }
}

/****************************************************************************/

sMaterial::~sMaterial()
{
}

/****************************************************************************/

void sShader::Cleanup()
{
  delete[] Code;
  Code = 0;
  sRelease(Shader);
}

/****************************************************************************/

void sMaterialSetup::Cleanup()
{
  delete[] States;
  States = 0;

  sSystem->Shaders[VSId].Release();
  sSystem->Shaders[PSId].Release();
  VSId = PSId = sINVALID;
  VS = 0;
  PS = 0;
}

sMaterialInstance sMaterialInstance::Null = { 0 };

/****************************************************************************/

sSimpleMaterial::sSimpleMaterial(sInt tex,sU32 flags,sU32 flags2,sU32 color)
{
  sU32 states[64],*st = states;
  static const sU32 vShaderTex[] =
  {
    0xfffe0101,                                         // vs.1.1
    0x0000001f, 0x80000000, 0x900f0000,                 // dcl_position v0
    0x0000001f, 0x80000005, 0x900f0001,                 // dcl_texcoord v1
    0x0000001f, 0x8000000a, 0x900f0002,                 // dcl_color v2
    0x00000014, 0xc00f0000, 0x90e40000, 0xa0e40000,     // m4x4 oPos,v0,c0
    0x00000001, 0xe00f0000, 0x90e40001,                 // mov oT0,v1
    0x00000001, 0xd00f0000, 0x90e40002,                 // mov oD0,v2
    0x0000ffff,                                         // end
  };
  static const sU32 vShaderNoTex[] =
  {
    0xfffe0101,                                         // vs.1.1
    0x0000001f, 0x80000000, 0x900f0000,                 // dcl_position v0
    0x0000001f, 0x8000000a, 0x900f0001,                 // dcl_color v1
    0x00000014, 0xc00f0000, 0x90e40000, 0xa0e40000,     // m4x4 oPos,v0,c0
    0x00000001, 0xd00f0000, 0x90e40001,                 // mov oD0,v1
    0x0000ffff,                                         // end
  };

  sInt blendMode = flags & sMBF_BLENDMASK;

  // zbias calc
  sF32 zBias = 0.0f;
  if(flags & sMBF_ZBIASBACK) zBias =  1.0f / 65536.0f;
  if(flags & sMBF_ZBIASFORE) zBias = -1.0f / 65536.0f;

  // render state setup
  *st++ = sD3DRS_ALPHATESTENABLE;     *st++ = 0;
  *st++ = sD3DRS_ZENABLE;             *st++ = sD3DZB_TRUE;
  *st++ = sD3DRS_ZWRITEENABLE;        *st++ = (flags & sMBF_ZWRITE) ? 1 : 0;
  *st++ = sD3DRS_ZFUNC;               *st++ = (flags & sMBF_ZREAD) ? sD3DCMP_LESS : sD3DCMP_ALWAYS;
  *st++ = sD3DRS_CULLMODE;            *st++ = (flags & sMBF_DOUBLESIDED) ? sD3DCULL_NONE : (flags & sMBF_INVERTCULL ? sD3DCULL_CW : sD3DCULL_CCW);
  *st++ = sD3DRS_COLORWRITEENABLE;    *st++ = 15;
  *st++ = sD3DRS_SLOPESCALEDEPTHBIAS; *st++ = *(sU32 *) &zBias;
  *st++ = sD3DRS_DEPTHBIAS;           *st++ = *(sU32 *) &zBias;
  *st++ = sD3DRS_FOGENABLE;           *st++ = 0;
  *st++ = sD3DRS_STENCILENABLE;       *st++ = 0;

  switch(flags & sMBF_BLENDMASK)
  {
  case sMBF_BLENDADD:
    *st++ = sD3DRS_ALPHABLENDENABLE;  *st++ = 1;
    *st++ = sD3DRS_SRCBLEND;          *st++ = sD3DBLEND_ONE;
    *st++ = sD3DRS_DESTBLEND;         *st++ = sD3DBLEND_ONE;
    *st++ = sD3DRS_BLENDOP;           *st++ = sD3DBLENDOP_ADD;
    break;

  case sMBF_BLENDALPHA:
    *st++ = sD3DRS_ALPHABLENDENABLE;  *st++ = 1;
    *st++ = sD3DRS_SRCBLEND;          *st++ = sD3DBLEND_SRCALPHA;
    *st++ = sD3DRS_DESTBLEND;         *st++ = sD3DBLEND_INVSRCALPHA;
    *st++ = sD3DRS_BLENDOP;           *st++ = sD3DBLENDOP_ADD;
    break;

  default:
    *st++ = sD3DRS_ALPHABLENDENABLE;  *st++ = 0;
    break;
  }

  // texture stages
  *st++ = sD3DTSS_0|sD3DTSS_TEXCOORDINDEX;          *st++ = 0;
  *st++ = sD3DTSS_0|sD3DTSS_TEXTURETRANSFORMFLAGS;  *st++ = 0;

  sU32 colorSource = (flags2 & 1) ? sD3DTA_TFACTOR : sD3DTA_DIFFUSE;

  if(tex != sINVALID)
  {
    *st++ = sD3DTSS_0|sD3DTSS_COLORARG1;              *st++ = sD3DTA_TEXTURE;
    *st++ = sD3DTSS_0|sD3DTSS_COLORARG2;              *st++ = colorSource;
    *st++ = sD3DTSS_0|sD3DTSS_COLOROP;                *st++ = sD3DTOP_MODULATE;
    *st++ = sD3DTSS_0|sD3DTSS_ALPHAARG1;              *st++ = sD3DTA_TEXTURE;
    *st++ = sD3DTSS_0|sD3DTSS_ALPHAOP;                *st++ = sD3DTOP_SELECTARG1;
  }
  else
  {
    *st++ = sD3DTSS_0|sD3DTSS_COLORARG2;              *st++ = colorSource;
    *st++ = sD3DTSS_0|sD3DTSS_COLOROP;                *st++ = sD3DTOP_SELECTARG2;
    *st++ = sD3DTSS_0|sD3DTSS_ALPHAARG2;              *st++ = colorSource;
    *st++ = sD3DTSS_0|sD3DTSS_ALPHAOP;                *st++ = sD3DTOP_SELECTARG2;
  }
  *st++ = sD3DTSS_1|sD3DTSS_COLOROP;                *st++ = sD3DTOP_DISABLE;
  *st++ = sD3DTSS_1|sD3DTSS_ALPHAOP;                *st++ = sD3DTOP_DISABLE;
  
  // sampler
  sU32 addressMode = (flags2 & 2) ? sD3DTADDRESS_CLAMP : sD3DTADDRESS_WRAP;

  *st++ = sD3DSAMP_0|sD3DSAMP_MAGFILTER;            *st++ = sD3DTEXF_LINEAR;
  *st++ = sD3DSAMP_0|sD3DSAMP_MINFILTER;            *st++ = sD3DTEXF_LINEAR;
  *st++ = sD3DSAMP_0|sD3DSAMP_MIPFILTER;            *st++ = (flags2 & 4) ? sD3DTEXF_NONE : sD3DTEXF_LINEAR;
  *st++ = sD3DSAMP_0|sD3DSAMP_ADDRESSU;             *st++ = addressMode;
  *st++ = sD3DSAMP_0|sD3DSAMP_ADDRESSV;             *st++ = addressMode;

  // terminator
  *st++ = ~0U;                                      *st++ = ~0U;

  // other stuff
  Color = color;
  Tex = sINVALID;
  SetTex(tex);

  // create setup
  Setup = sSystem->MtrlAddSetup(states,(tex == sINVALID) ? vShaderNoTex : vShaderTex,0);
}

sSimpleMaterial::~sSimpleMaterial()
{
  SetTex(sINVALID);
  sSystem->MtrlRemSetup(Setup);
}

void sSimpleMaterial::SetTex(sInt tex)
{
  if(tex != sINVALID)
    sSystem->AddRefTexture(tex);

  if(Tex != sINVALID)
    sSystem->RemTexture(Tex);

  Tex = tex;
}

void sSimpleMaterial::Set(const sMaterialEnv &env)
{
  sMaterialInstance inst;
  sMatrix mat;

  inst.NumTextures = 1;
  inst.Textures[0] = Tex;
  inst.NumVSConstants = 4;
  inst.VSConstants = &mat.i;
  inst.NumPSConstants = 0;

  mat.Mul4(env.ModelSpace,sSystem->LastViewProject);
  mat.Trans4();

  sSystem->MtrlSetSetup(Setup);
  sSystem->MtrlSetInstance(inst);
  sSystem->SetState(sD3DRS_TEXTUREFACTOR,Color);
}

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
}

/****************************************************************************/

void sViewport::InitTex(sInt handle)
{
  Screen = -1;
  RenderTarget = handle;
  Window.x0 = 0;
  Window.y0 = 0;
  sSystem->GetTextureSize(handle,Window.x1,Window.y1);
}

/****************************************************************************/

void sViewport::InitTexMS(sInt handle)
{
  Screen = -2;
  RenderTarget = handle;
  Window.x0 = 0;
  Window.y0 = 0;
  sSystem->GetTextureSize(handle,Window.x1,Window.y1);
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

/****************************************************************************/

void sSystem_::SetViewProject(const sMaterialEnv *env)
{
  sVERIFY(env);

  sFloatDouble();

  env->MakeProjectionMatrix(LastProjection);
  LastCamera = env->CameraSpace;
  LastCamera.TransR();

  LastViewProject.Mul4(LastCamera,LastProjection);
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Shader                                                             ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#if !sPLAYER
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
void sSystem_::PrintShader(const sU32 *data)
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
    0x12,0x12,0x12,0x12  ,0x12,0x12,0x11,0x11,    // 0x80 = label
    0x11,0x12,0x13,0x11  ,0x12,0x12,0x12,0x12,    // 0x90 = dcl
    0x12,0x80,0x80,0x80  ,0x00,0x00,0x80,0x90,    // 0xb0 = def

    0x12,0x12,0x11,0x11  ,0x11,0x20,0x00,0x00,
    0x01,0x02,0x01,0x01  ,0x00,0x02,0x11,0x90,
    0x90,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,

    0x10,0x01,0x10,0x11  ,0x11,0x11,0x11,0x12,
    0x12,0x12,0x12,0x00  ,0x13,0x12,0x11,0x11,
    0x13,0xb0,0x13,0x11  ,0x11,0x14,0x12,0x12,
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

  sInt code,in,out,def,dcl,label;
  sInt i,len,j;
  sInt version;
  sU32 val;
  sInt komma;
  sInt line;
  sBool end = sFALSE;

  line = 1;
  while(!end)
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
    if(val == 0xffff)
      end = sTRUE;
    in = out = def = dcl = label = komma = 0;
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
    case 0xb0:
      def = 4;
      out = 1;
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
    data++;

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
      data++;
    }
    for(i=0;i<dcl;i++)
    {
      if(komma) sDPrintF(","); komma=1;
      val = *data++;
      if(version<0xffff0000) 
        sDPrintF("%s%d",usage[val&0x0f],(val&0xf0000)>>16);
      else
        sDPrintF("%s",texkind[(val>>27)&7]);
    }
    for(i=0;i<out;i++)
    {
      if(komma) sDPrintF(","); komma=1;
      val = *data++;
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
      val = *data++;
      sDPrintF("%s",sourcepre[(val>>24)&15]);
      PrintReg(val,version);
      if(val&0x00002000) sDPrintF("[a]");
      j = (val&0x00ff0000)>>16;
      if(j!=0xe4)
        sDPrintF(".%c%c%c%c",rz[(j>>0)&3],rz[(j>>2)&3],rz[(j>>4)&3],rz[(j>>6)&3]);
    }
    for(i=0;i<def;i++)
    {
      if(komma) sDPrintF(","); komma=1;
      val = *data++;
      sF32 valf = *((sF32 *) &val);
      sDPrintF("%7.3f",valf);
    }
    sDPrintF("\n");
  }
  sDPrintF(".\n");
}
#endif

/****************************************************************************/

// Compare two state lists.
static sBool checkStatesEqual(const sU32 *st1,const sU32 *st2)
{
  sInt i;
  for(i=0;st1[i] == st2[i] && ((i&1) || st1[i] != ~0U);i++);

  return st1[i] == st2[i];
}

sInt sSystem_::MtrlAddSetup(const sU32 *states,const sU32 *vs,const sU32 *ps)
{
  // calculate length of state array
  sInt stateLen;
  for(stateLen=0;states[stateLen] != ~0U;stateLen+=2);
  stateLen += 2;

#if !sINTRO // full-featured version
  // create copy of state array + insertion sort by state index
  sU32 *stateCopy = new sU32[stateLen];
  for(sInt i=0;i<stateLen;i+=2)
  {
    sInt j;
    sU32 key = states[i];

    for(j=i-2;j>=0 && stateCopy[j] > key;j-=2)
    {
      stateCopy[j+2] = stateCopy[j+0];
      stateCopy[j+3] = stateCopy[j+1];
    }

    stateCopy[j+2] = states[i+0];
    stateCopy[j+3] = states[i+1];
  }

  // check whether this shader is unique
  sInt firstFree = -1;
  sInt vsId = GetShader(vs);
  sInt psId = GetShader(ps);

  for(sInt i=0;i<MAX_SETUPS;i++)
  {
    if(!Setups[i].RefCount)
    {
      if(firstFree == -1)
        firstFree = i;

      continue;
    }
    else if(checkStatesEqual(stateCopy,Setups[i].States) // identical with this shader?
      && vsId == Setups[i].VSId && psId == Setups[i].PSId)
    {
      Shaders[vsId].Release();
      Shaders[psId].Release();
      Setups[i].AddRef();
      delete[] stateCopy;

      return i;
    }
  }

  if(firstFree == -1) // no more setup slots left
  {
#if !sINTRO
    sDPrintF("out of material setup slots!\n");
#endif
    return sINVALID;
  }

  // create new shader setup
  sMaterialSetup *setup = &Setups[firstFree];

  // copy source data and we're set
  setup->RefCount = 1;
  setup->States = stateCopy;
  setup->VSId = vsId;
  setup->PSId = psId;
  setup->VS = (IDirect3DVertexShader9 *) Shaders[vsId].Shader;
  setup->PS = (IDirect3DPixelShader9 *) Shaders[psId].Shader;
#else // bare-bone version for intros without equality check or anything
  sInt firstFree;
  HRESULT hr;

  for(firstFree=0;firstFree<MAX_SETUPS;firstFree++)
  {
    if(!Setups[firstFree].RefCount)
      break;
  }

  if(firstFree == MAX_SETUPS)
    return sINVALID;

  sMaterialSetup *setup = &Setups[firstFree];
  hr = DXDev->CreateVertexShader((DWORD *) vs,&setup->VS);
  if(FAILED(hr))
    return sINVALID;

  hr = DXDev->CreatePixelShader((DWORD *) ps,&setup->PS);
  if(FAILED(hr))
    return sINVALID;

  setup->RefCount = 1;
  setup->States = new sU32[stateLen];
  setup->VSId = setup->PSId = sINVALID;
  sCopyMem4(setup->States,states,stateLen);
#endif

  return firstFree;
}

void sSystem_::MtrlAddRefSetup(sInt sid)
{
  // sanity checks
  sVERIFY(sid >= 0 && sid < MAX_SETUPS);
  sVERIFY(Setups[sid].RefCount);

  Setups[sid].AddRef();
}

void sSystem_::MtrlRemSetup(sInt sid)
{
  // sanity checks
  sVERIFY(sid >= 0 && sid < MAX_SETUPS);
  sVERIFY(Setups[sid].RefCount);

  Setups[sid].Release();
}

void sSystem_::MtrlClearCaches()
{
  CurrentSetupId = sINVALID;
  CurrentVS = 0;
  CurrentPS = 0;

  for(sInt i=0;i<MAX_TEXSTAGE;i++)
    CurrentTextures[i] = sINVALID;

  for(sInt i=0;i<0x310;i++)
    CurrentStates[i] = 0xfefefefe; // let's hope we never actually want this value
}

void sSystem_::MtrlSetSetup(sInt sid)
{
  if(sid == CurrentSetupId)
    return;

  sZONE(MtrlSetSetup);
  
  // sanity checks
  sVERIFY(sid >= 0 && sid < MAX_SETUPS);
  sMaterialSetup *setup = &Setups[sid];
  sVERIFY(setup->RefCount > 0);

  if(CurrentSetupId != sINVALID)
    Setups[CurrentSetupId].Release();

  setup->AddRef();

  // set states
  for(sU32 *st = setup->States;st[0] != ~0U;st += 2)
  {
    sU32 state = st[0]; 
    sVERIFY(state < 0x310);

    if(st[1] != CurrentStates[state])
    {
      SetState(state,st[1]);
      CurrentStates[state] = st[1];
    }
  }

  // set vertex shader
  if(setup->VS != CurrentVS)
  {
    HRESULT hr = DXDev->SetVertexShader(setup->VS);
    sVERIFY(SUCCEEDED(hr));
    CurrentVS = setup->VS;
  }

  // set pixel shader
  if(setup->PS != CurrentPS)
  {
    HRESULT hr = DXDev->SetPixelShader(setup->PS);
    sVERIFY(SUCCEEDED(hr));
    CurrentPS = setup->PS;
  }

  CurrentSetupId = sid;
}

void sSystem_::MtrlSetInstance(const sMaterialInstance &inst)
{
  // set textures
  for(sInt i=0;i<inst.NumTextures;i++)
  {
    sInt texIndex = inst.Textures[i];

    if(texIndex != CurrentTextures[i])
    {
      IDirect3DBaseTexture9 *tex = 0;
      if(texIndex >= 0 && texIndex < MAX_TEXTURE)
        tex = Textures[texIndex].Tex;
      else if(texIndex == -2)
        tex = DXNormalCube;
      else if(texIndex == -3)
        tex = DXAttenuationVolume;
      else if(texIndex == -4)
        tex = DXTinyNormalCube;

      HRESULT hr = DXDev->SetTexture(i,tex);
      sVERIFY(SUCCEEDED(hr));
      CurrentTextures[i] = texIndex;
    }
  }

  // vertex+pixel shader constants (no special caching)
  if(inst.NumVSConstants)
    DXDev->SetVertexShaderConstantF(0,(sF32 *) inst.VSConstants,inst.NumVSConstants);

  if(inst.NumPSConstants)
    DXDev->SetPixelShaderConstantF(0,(sF32 *) inst.PSConstants,inst.NumPSConstants);
}

void sSystem_::MtrlSetVSConstants(sInt first,const sVector *constants,sInt count)
{
  if(count)
    DXDev->SetVertexShaderConstantF(first,&constants->x,count);
}

void sSystem_::MtrlSetPSConstants(sInt first,const sVector *constants,sInt count)
{
  if(count)
    DXDev->SetPixelShaderConstantF(first,&constants->x,count);
}

/****************************************************************************/

#if !sPLAYER

static PAVIFILE aviFile = 0;
static PAVISTREAM aviVid = 0,aviVidC = 0;
static sInt aviXRes,aviYRes;
static IDirect3DSurface9 *aviSurface = 0;
static sInt aviTarget = sINVALID;
static sU8 *aviBuffer = 0;
static sInt aviFrameCount;

sBool sSystem_::VideoStart(sChar *filename,sF32 fpsrate,sInt xRes,sInt yRes)
{
  AVISTREAMINFO asi;
  AVICOMPRESSOPTIONS aco;
  AVICOMPRESSOPTIONS FAR *optlist[1];
  BITMAPINFOHEADER bmi;
  sBool gotOptions = sFALSE;
  sBool error = sTRUE;

  VideoEnd();

  // clamp resolution
  if(xRes > 1024)
  {
    xRes = 1024;
    yRes = sMulDiv(yRes,1024,xRes);
  }

  if(yRes > 512)
  {
    yRes = 512;
    xRes = sMulDiv(xRes,512,yRes);
  }

  AVIFileInit();
  
  // create avi file
  if(AVIFileOpen(&aviFile,filename,OF_WRITE|OF_CREATE,NULL) != AVIERR_OK)
  {
    sDPrintF("AVIFileOpen failed\n");
    goto cleanup;
  }

  // initialize video stream header
  WINZERO(asi);
  asi.fccType               = streamtypeVIDEO;
  asi.dwScale               = 10000;
  asi.dwRate                = 10000*fpsrate + 0.5f;
  asi.dwSuggestedBufferSize = xRes * yRes * 3;
  SetRect(&asi.rcFrame,0,0,xRes,yRes);
  sCopyString(asi.szName,"Video",sizeof(asi.szName));

  // create video stream
  if(AVIFileCreateStream(aviFile,&aviVid,&asi) != AVIERR_OK)
  {
    sDPrintF("AVIFileCreateStream failed\n");
    goto cleanup;
  }

  // get coding options
  WINZERO(aco);
  optlist[0] = &aco;

  gotOptions = sTRUE;
  if(!AVISaveOptions((HWND) Screen[0].Window,0,1,&aviVid,optlist))
  {
    sDPrintF("AVISaveOptions failed\n");
    goto cleanup;
  }

  // create compressed stream
  if(AVIMakeCompressedStream(&aviVidC,aviVid,&aco,0) != AVIERR_OK)
  {
    sDPrintF("AVIMakeCompressedStream failed\n");
    goto cleanup;
  }

  // set stream format
  WINZERO(bmi);
  bmi.biSize        = sizeof(bmi);
  bmi.biWidth       = xRes;
  bmi.biHeight      = yRes;
  bmi.biPlanes      = 1;
  bmi.biBitCount    = 24;
  bmi.biCompression = BI_RGB;
  AVIStreamSetFormat(aviVidC,0,&bmi,sizeof(bmi));

  if(FAILED(DXDev->CreateOffscreenPlainSurface(1024,512,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&aviSurface,0)))
  {
    sDPrintF("CreateOffscreenPlainSurface failed\n");
    goto cleanup;
  }

  aviTarget = AddTexture(1024,512,sTF_A8R8G8B8,0);
  if(aviTarget == sINVALID)
  {
    sDPrintF("Couldn't create AVI rendertarget\n");
    goto cleanup;
  }

  error = sFALSE;
  sDPrintF("Video recording started.\n");
  aviXRes = xRes;
  aviYRes = yRes;
  aviBuffer = new sU8[xRes * yRes * 3];
  aviFrameCount = 0;

cleanup:
  if(gotOptions)
    AVISaveOptionsFree(1,optlist);

  if(error)
    VideoEnd();

  return !error;
}

void sSystem_::VideoViewport(sViewport &vp)
{
  vp.InitTex(aviTarget);
  vp.Window.Init(0,0,aviXRes,aviYRes);
}

sInt sSystem_::VideoWriteFrame(sF32 &u1,sF32 &v1)
{
  D3DLOCKED_RECT lr;
  IDirect3DSurface9 *surface = 0;
  sBool error = sTRUE;

  if(aviTarget == sINVALID)
    return sINVALID;

  u1 = 0.0f;
  v1 = 0.0f;

  // first, get rendertarget surface
  if(FAILED(Textures[aviTarget].Tex->GetSurfaceLevel(0,&surface)))
  {
    sDPrintF("couldn't get surface level!\n");
    goto cleanup;
  }

  // copy rendertarget data
  if(FAILED(DXDev->GetRenderTargetData(surface,aviSurface)))
  {
    sDPrintF("couldn't get render target data!\n");
    goto cleanup;
  }

  // lock
  if(FAILED(aviSurface->LockRect(&lr,0,D3DLOCK_READONLY)))
  {
    sDPrintF("couldn't lock.\n");
    goto cleanup;
  }

  // read data and convert in the process
  for(sInt y=0;y<aviYRes;y++)
  {
    sU32 *src = (sU32 *) ((sU8 *) lr.pBits + lr.Pitch * y);
    sU8 *dst = aviBuffer + (aviYRes - 1 - y) * aviXRes * 3;

    for(sInt x=0;x<aviXRes;x++)
    {
      sU32 srcv = *src++;
      *dst++ = (srcv >>  0) & 0xff; // blue
      *dst++ = (srcv >>  8) & 0xff; // green
      *dst++ = (srcv >> 16) & 0xff; // red
    }
  }

  // unlock + encode
  aviSurface->UnlockRect();
  AVIStreamWrite(aviVidC,aviFrameCount,1,aviBuffer,3*aviXRes*aviYRes,0,0,0);

  error = sFALSE;
  aviFrameCount++;

  u1 = aviXRes / 1024.0f;
  v1 = aviYRes / 512.0f;

cleanup:
  if(surface)
    surface->Release();

  if(error)
    return sINVALID;
  else
    return aviTarget;
}

void sSystem_::VideoEnd()
{
  delete[] aviBuffer;
  aviBuffer = 0;

  if(aviTarget != sINVALID)
  {
    RemTexture(aviTarget);
    aviTarget = sINVALID;
  }

  if(aviSurface)
  {
    aviSurface->Release();
    aviSurface = 0;
  }

  if(aviVidC)
  {
    AVIStreamRelease(aviVidC);
    aviVidC = 0;
  }

  if(aviVid)
  {
    AVIStreamRelease(aviVid);
    aviVid = 0;
  }

  if(aviFile)
  {
    AVIFileRelease(aviFile);
    aviFile = 0;
  }

  AVIFileExit();
}
#endif

/****************************************************************************/
/****************************************************************************/

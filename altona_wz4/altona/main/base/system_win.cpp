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

/****************************************************************************/

#include "base/types2.hpp"
#include "base/system.hpp"
#include "base/math.hpp"
#include "base/graphics.hpp"
#include "base/windows.hpp"
#include "base/input2.hpp"

#include <windows.h>
#include <mmsystem.h>
#include <dinput.h> 
#include <xinput.h>
#include <vfw.h>
#include <dbt.h>            // DBT_DEVNODES_CHANGED
#include <dxerr.h>
#if sCONFIG_COMPILER_MSC
#undef new
#define _MFC_OVERRIDES_NEW
#include <crtdbg.h>
#include <malloc.h>
#endif
#include <setjmp.h>
#define new sDEFINE_NEW

#define sCRASHDUMP (sCONFIG_OPTION_XSI || sCONFIG_OPTION_AGEINGTEST)  // save crashdumps in ageing tests always

// exit==sTRUE: application exit, forces collection of all objects
// iterates collection until all objects are destroyed or gives warning if not all roots are removed
// without this flag in the case of sRemRoot in collected objects or in root object dtors
// objects are not collected before shutdown
void sCollector(sBool exit=sFALSE);

void sPingSound();

void sInitEmergencyThread();


/****************************************************************************/
/***                                                                      ***/
/***   Globals                                                            ***/
/***                                                                      ***/
/****************************************************************************/

extern sInt sSystemFlags;
extern sInt sExitFlag;
extern sApp *sAppPtr;
extern sBool sAllocMemEnabled;
extern sInt DXMayRestore;
static sBool sTrialVersion=sFALSE;

sInt sFatalFlag = 0;

HDC sGDIDC = 0;
HDC sGDIDCOffscreen = 0;
HWND sHWND = 0;
HWND sExternalWindow = 0;

static sInt sInPaint = 0;

static sU32 sKeyTable[3][256];    // norm/shift/alt
sU32 sKeyQual;

HINSTANCE WInstance;
static sInt sMouseWheelAkku;

static sInt WError;
static sInt TimerEventTime;

static HANDLE WConOut = 0;        // console output file
static HANDLE WConIn  = 0;        // console input file


// interfaces for user32.dll parts, which are not in every 
// system version available.
typedef UINT  (sSTDCALL *GetRawInputDataProc)(HRAWINPUT hRawInput,UINT uiCommand,LPVOID pData,PUINT pcbSize,UINT cbSizeHeader);
typedef BOOL  (sSTDCALL *RegisterRawInputDevicesProc)(PCRAWINPUTDEVICE pRawInputDevices,UINT uiNumDevices,UINT cbSize);

static RegisterRawInputDevicesProc  RegisterRawInputDevicesPtr=0L;
static GetRawInputDataProc          GetRawInputDataPtr=0L;

/****************************************************************************/
/***                                                                      ***/
/***   Input                                                              ***/
/***                                                                      ***/
/****************************************************************************/

static sThreadLock* InputThreadLock;

class sKeyboardData {
public:
  sKeyboardData(sInt num) 
   : Device(sINPUT2_TYPE_KEYBOARD, num) 
  { 
    keys.HintSize(256);
    keys.AddManyInit(256, sFALSE);
    sInput2RegisterDevice(&Device);
  }

  ~sKeyboardData()
  {
    sInput2RemoveDevice(&Device);
  }

  void Poll()
  {
    InputThreadLock->Lock();
    sInput2DeviceImpl<sINPUT2_KEYBOARD_MAX>::Value_ v;
    for (int i=0; i<255; i++) {
      v.Value[i] = keys[i] ? 1.0f : 0.0f;
    }
    v.Timestamp = sGetTime();
    v.Status = 0;
    Device.addValues(v);
    InputThreadLock->Unlock();
  }

  sStaticArray<sBool> keys;

private:
  sInput2DeviceImpl<sINPUT2_KEYBOARD_MAX> Device;
};

class sMouseData {
public:
  enum
  {
    sMB_LEFT = 1,                   // left mousebutton
    sMB_RIGHT = 2,                  // right mousebutton
    sMB_MIDDLE = 4,                 // middle mousebutton
    sMB_X1 = 8,                     // 4th mousebutton
    sMB_X2 = 16,                    // 5th mousebutton
  };

  sMouseData(sInt num) 
   : Device(sINPUT2_TYPE_MOUSE, num) 
  {
    X = Y = Z = RawX = RawY = Buttons = 0;
    sInput2RegisterDevice(&Device);
  }

  ~sMouseData()
  {
    sInput2RemoveDevice(&Device);
  }

  void Poll() 
  {
    InputThreadLock->Lock();
    sInput2DeviceImpl<sINPUT2_MOUSE_MAX>::Value_ v;
    v.Value[sINPUT2_MOUSE_X] = X;
    v.Value[sINPUT2_MOUSE_Y] = Y;
    v.Value[sINPUT2_MOUSE_RAW_X] = RawX;
    v.Value[sINPUT2_MOUSE_RAW_Y] = RawY;
    v.Value[sINPUT2_MOUSE_WHEEL] = Z;
    v.Value[sINPUT2_MOUSE_LMB] = Buttons & sMB_LEFT;
    v.Value[sINPUT2_MOUSE_RMB] = Buttons & sMB_RIGHT;
    v.Value[sINPUT2_MOUSE_MMB] = Buttons & sMB_MIDDLE;
    v.Value[sINPUT2_MOUSE_X1] = Buttons & sMB_X1;
    v.Value[sINPUT2_MOUSE_X2] = Buttons & sMB_X2;
    v.Value[sINPUT2_MOUSE_VALID] = 1.0f; // TODO
    v.Timestamp = sGetTime();
    v.Status = 0;
    Device.addValues(v);
    InputThreadLock->Unlock();
  }

  sInt X;
  sInt Y;
  sInt Z;
  sInt RawX;
  sInt RawY;
  sInt Buttons;

private:
  sInput2DeviceImpl<sINPUT2_MOUSE_MAX> Device;
};

bool XInputPresent=false;
static sThread* InputThread;
static sKeyboardData* Keyboard;
static sMouseData* Mouse;

void sPollInput(sThread *thread, void *data)
{
  while(thread->CheckTerminate())
  {
    // mouse
    Mouse->Poll();

    // keyboard
    Keyboard->Poll();

    sSleep(5);
  }
}

void sInitInput()
{
  Mouse = new sMouseData(0);
  Keyboard = new sKeyboardData(0);
  InputThreadLock = new sThreadLock();
  InputThread = new sThread(sPollInput);
}
void sExitInput()
{
  sDelete(InputThread);
  sDelete(Keyboard);
  sDelete(Mouse);
  sDelete(InputThreadLock);
}
sADDSUBSYSTEM(Input, 0x30, sInitInput, sExitInput);

void sSetMouseCenter()
{
  RECT r;
  GetClientRect(sHWND,&r);
  sSetMouse((r.left+r.right)/2,(r.top+r.bottom)/2);
}

void sSetMouse(sInt x,sInt y)
{
  if (!Mouse)
    return;
  POINT p;
  Mouse->X = x;
  Mouse->Y = y;
  p.x = x;
  p.y = y;
  ClientToScreen(sHWND,&p);
  SetCursorPos(p.x,p.y);
}

//static sPackfile *Packfiles[16];
//static sInt PackfileCount;

/****************************************************************************/
/***                                                                      ***/
/***   External Modules                                                   ***/
/***                                                                      ***/
/****************************************************************************/

void Render3D();            // render 3d scene with sApp::OnPaint3D

void PreInitGFX(sInt &flags,sInt &xs,sInt &ys);
void InitGFX(sInt flags,sInt xs,sInt ys);
void ExitGFX();
void ResizeGFX(sInt x,sInt y);
void SetXSIModeD3D(sBool enable);

/****************************************************************************/
/***                                                                      ***/
/***   Windows Helper                                                     ***/
/***                                                                      ***/
/****************************************************************************/

#if sSTRIPPED
void DXError(sU32 err)
{
  if(FAILED(err))
  {
    sString<256> buffer;

    sSPrintF(buffer,L"dx error %08x (%d)",err,err&0x3fff);
    sFatal(buffer);
  }
}
#define DIErr(hr) { sU32 err=hr; if(FAILED(err)) DXError(err); }

#else
#pragma comment(lib,"dxerr.lib")
void DXError(sU32 err,const sChar *file,sInt line,const sChar *system)
{
  if(FAILED(err))
  {
    sString<1024> buffer;

    sSPrintF(buffer,L"%s(%d): %s error %08x (%d): %s (%s)",file,line,system,err,err&0x3fff,
      DXGetErrorStringW(err),DXGetErrorDescriptionW(err));
    sFatal(buffer);
  }
}
#define DIErr(hr) { sU32 err=hr; if(FAILED(err)) DXError(err,sTXT(__FILE__),__LINE__,L"dinput"); }
#endif

#ifndef sCRASHDUMP
#define sCRASHDUMP 0
#endif

#if sCRASHDUMP
#include <windows.h>
#include <dbghelp.h>
#include <shellapi.h>
#include <shlobj.h>

typedef BOOL (WINAPI * MINIDUMP_WRITE_DUMP)(
  HANDLE hProcess,
  DWORD ProcessId,
  HANDLE hFile,
  MINIDUMP_TYPE DumpType,
  CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
  PVOID UserStreamParam,
  PVOID CallbackParam
  );

int GenerateDump(EXCEPTION_POINTERS* pExceptionPointers)
{
  // don't write a crashdump if a debugger is running
  if (IsDebuggerPresent()) return EXCEPTION_CONTINUE_SEARCH;

  HMODULE hDbgHelp = LoadLibraryW(L"DBGHELP.DLL");
  MINIDUMP_WRITE_DUMP MiniDumpWriteDump_ = (MINIDUMP_WRITE_DUMP)GetProcAddress(hDbgHelp, 
                        "MiniDumpWriteDump");

  if(MiniDumpWriteDump_)
  {
    BOOL bMiniDumpSuccessful;
    HANDLE hDumpFile;
    MINIDUMP_EXCEPTION_INFORMATION ExpParam;

    hDumpFile = CreateFileW(L"crash.dmp", GENERIC_READ|GENERIC_WRITE, 
                FILE_SHARE_WRITE|FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

    ExpParam.ThreadId = GetCurrentThreadId();
    ExpParam.ExceptionPointers = pExceptionPointers;
    ExpParam.ClientPointers = TRUE;

    bMiniDumpSuccessful = MiniDumpWriteDump_(GetCurrentProcess(), GetCurrentProcessId(), 
                    hDumpFile, (MINIDUMP_TYPE)(MiniDumpNormal), &ExpParam, NULL, NULL);
  }

  return EXCEPTION_EXECUTE_HANDLER;
}

void CrashDump()
{
  __try
  {
      int *pBadPtr = NULL;
      *pBadPtr = 0;
  }
  __except(GenerateDump(GetExceptionInformation()))
  {
  }
}
#endif // sCRASHDUMP

static sDateAndTime TimeFromSystemTime(const SYSTEMTIME &time)
{
  sDateAndTime r;
  r.Year = time.wYear;
  r.Month = time.wMonth;
  r.Day = time.wDay;
  r.DayOfWeek = time.wDayOfWeek;
  r.Hour = time.wHour;
  r.Minute = time.wMinute;
  r.Second = time.wSecond;
  return r;
}

static void TimeToSystemTime(SYSTEMTIME &st,const sDateAndTime &time)
{
  st.wYear = time.Year;
  st.wMonth = time.Month;
  st.wDay = time.Day;
  st.wHour = time.Hour;
  st.wMinute = time.Minute;
  st.wSecond = time.Second;
  st.wMilliseconds = 0;
}

static sDateAndTime TimeFromFileTime(const FILETIME &time)
{
  SYSTEMTIME utctime,loctime;

  FileTimeToSystemTime(&time,&loctime);
  SystemTimeToTzSpecificLocalTime(0,&utctime,&loctime);
  return TimeFromSystemTime(loctime);
}

static void TimeToFileTime(FILETIME &ft,const sDateAndTime &time)
{
  SYSTEMTIME utctime,loctime;

  TimeToSystemTime(loctime,time);
  TzSpecificLocalTimeToSystemTime(0,&loctime,&utctime);
  SystemTimeToFileTime(&utctime,&ft);
}

/****************************************************************************/
/***                                                                      ***/
/***   Debugging (declared in types.hpp!)                                 ***/
/***                                                                      ***/
/****************************************************************************/

//void __cdecl sFatal(const sChar *format,...)
//{
//  static sString<1024> buffer;
//
////  sFatality = 1;
//  sFormatString(buffer,format,&format);
//
//  OutputDebugStringW(buffer);
//  OutputDebugStringW(sTXT("\n"));
//
//  MessageBoxW(0,buffer,L"altona",MB_OK);
//
//  _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)&~(_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF));
//  TerminateProcess(GetCurrentProcess(),1);
//}

#if sCONFIG_OPTION_XSI
extern jmp_buf XSICDHFailure;
extern LPCWSTR XSICDHFailureMessage;
#endif

void __cdecl sFatalImplTrace(const sChar *str,const sChar *traceMsg)
{
  sString<1024> error = str;      // copy error message
  if(traceMsg)
  {
    sAppendString(error,L"\nStack trace:\n");
    sAppendString(error,traceMsg);
  }

#if sCONFIG_OPTION_XSI
  // message box call invalidates longjump marker, so show message box after jump
  XSICDHFailureMessage  = str;
  longjmp(XSICDHFailure,1);     // don't terminate XSI cdh
#endif


  sCrashHook->Call(error);

  sExit();                        // this will cause debug prints to override our error message!  
  sFatalFlag = 1;
  OutputDebugStringW(error);
  OutputDebugStringW(sTXT("\n"));

#if !sRELEASE
  sDEBUGBREAK;
#endif


  if(sCONFIG_OPTION_SHELL)
  {
    sPrintError(L"Fatal Error\n");
    sPrintError(error);
    sPrintError(L"\n");

    if(!sRELEASE)
    {
      sString<64> buffer;\
      DWORD dummy;
      sPrint(L"<<< press enter >>>\n");

      ReadConsoleW(WConIn,(sChar *)buffer,64,&dummy,0);
    }
  }
  else
  {
    MSG msg;
    if (sHWND)
    {
      DestroyWindow(sHWND);
      while(GetMessage(&msg,0,0,0)) 
      { 
        TranslateMessage(&msg); 
        DispatchMessage(&msg); 
      }
      sHWND=0;
    }

    const sChar *title = sGetWindowName();
    MessageBoxW(0,error,title,MB_OK|MB_ICONERROR);
  }


#if sCONFIG_COMPILER_MSC
  _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)&~(_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF));
#endif

#if sCRASHDUMP
  CrashDump();
#endif

  TerminateProcess(GetCurrentProcess(),1);
}

void __cdecl sFatalImpl(const sChar *str)
{
  sString<1024> trace;
  const sChar *traceMsg = 0;

  if(IsDebuggerPresent() && !(sSystemFlags & sISF_FULLSCREEN))
  { // wir wollen die message aber trotzdem sehen...
    if(str!=0L)
    {
      OutputDebugStringW(str);
      OutputDebugStringW(L"\n");
    }
    sDEBUGBREAK;
  }

  if(sStackTrace(trace,2))
    traceMsg = trace;

  sFatalImplTrace(str,traceMsg);
}

#if sENABLE_DPRINT
static sBool sDPrintFlag=sFALSE;


void sDPrint(const sChar *text)
{
  sThreadContext *tc=sGetThreadContext();
  if (tc->Flags & sTHF_NODEBUG) return;
#if !sSTRIPPED
  if (sDebugOutHook && !(tc->Flags&sTHF_NONETDEBUG))
    sDebugOutHook->Call(text);
#endif

#if sCONFIG_SYSTEM_LINUX
  sPrint(text);                   // no debugger with linux
#else
  OutputDebugStringW(text);
#endif
  sDPrintFlag=sTRUE;
}
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Initialisation                                                     ***/
/***                                                                      ***/
/****************************************************************************/

void sInitKeys()
{
  static const sU32 keys[][2] =
  {
    { sKEY_ENTER    , VK_RETURN   },
    { sKEY_UP       , VK_UP       },
    { sKEY_DOWN     , VK_DOWN     },
    { sKEY_LEFT     , VK_LEFT     },
    { sKEY_RIGHT    , VK_RIGHT    },
    { sKEY_PAGEUP   , VK_PRIOR    },
    { sKEY_PAGEDOWN , VK_NEXT     },
    { sKEY_HOME     , VK_HOME     },
    { sKEY_END      , VK_END      },
    { sKEY_INSERT   , VK_INSERT   },
    { sKEY_DELETE   , VK_DELETE   },
    { sKEY_BACKSPACE, VK_BACK     },
    { sKEY_PAUSE    , VK_PAUSE    },
    { sKEY_SCROLL   , VK_SCROLL   },
    { sKEY_NUMLOCK  , VK_NUMLOCK  },
    { sKEY_PRINT    , VK_SNAPSHOT },
    { sKEY_WINL     , VK_LWIN     },
    { sKEY_WINR     , VK_RWIN     },
    { sKEY_WINM     , VK_APPS     },
    { sKEY_SHIFT    , VK_SHIFT    },
    { sKEY_CTRL     , VK_CONTROL  },
    { sKEY_CAPS     , VK_CAPITAL  },
    { sKEY_F1       , VK_F1       },
    { sKEY_F2       , VK_F2       },
    { sKEY_F3       , VK_F3       },
    { sKEY_F4       , VK_F4       },
    { sKEY_F5       , VK_F5       },
    { sKEY_F6       , VK_F6       },
    { sKEY_F7       , VK_F7       },
    { sKEY_F8       , VK_F8       },
    { sKEY_F9       , VK_F9       },
    { sKEY_F10      , VK_F10      },
    { sKEY_F11      , VK_F11      },
    { sKEY_F12      , VK_F12      },

    { '0'           , VK_NUMPAD0  },
    { '1'           , VK_NUMPAD1  },
    { '2'           , VK_NUMPAD2  },
    { '3'           , VK_NUMPAD3  },
    { '4'           , VK_NUMPAD4  },
    { '5'           , VK_NUMPAD5  },
    { '6'           , VK_NUMPAD6  },
    { '7'           , VK_NUMPAD7  },
    { '8'           , VK_NUMPAD8  },
    { '9'           , VK_NUMPAD9  },
    { '*'           , VK_MULTIPLY },
    { '+'           , VK_ADD      },
    { '.'           , VK_DECIMAL  },
    { '/'           , VK_DIVIDE   },
    { '-'           , VK_SUBTRACT },
    { 0 }
  };
  static const sChar symbols[] = L"€";


    
  // ranges for unicode codepages (end is -EXCLUSIVE-!)
  // see http://www.unicode.org/charts
  static const sInt ranges[][2] =
  {
    { 0x000, 0x0ff},       // asciitable
    { 0x400, 0x500},       // cyrillic
  };

  sClear(sKeyTable);

  for (sInt k=0; k<sCOUNTOF(ranges); k++)
  {
    for(sInt i=ranges[k][0];i<ranges[k][1];i++)
    {
      sInt r = VkKeyScanW(i);
      if((r&255)!=255)
      {
        sInt t = 0;
        if(r&0x100)
          t = 1;
        if(r&0x200)
          t = -1;
        if(r&0x400)
          t = 2;

        if(t>=0)
          sKeyTable[t][r&255] = i;
      }
    }
  }

  for(sInt j=0;symbols[j];j++)
  {
    sInt i = symbols[j];
    sInt r = VkKeyScanW(i);
    if((r&255)!=255)
    {
      sInt t = 0;
      if(r&0x100)
        t = 1;
      if(r&0x400)
        t = 2;

      sKeyTable[t][r&255] = i;
    }
  }

  for(sInt i=0;keys[i][0];i++)
  {
    sKeyTable[0][keys[i][1]] = keys[i][0];
    sKeyTable[1][keys[i][1]] = keys[i][0];
    sKeyTable[2][keys[i][1]] = keys[i][0];
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   File Operation                                                     ***/
/***                                                                      ***/
/****************************************************************************/

#if !sSTRIPPED
static sInt sFileSeekTimeEmulation_ = 0;
static sInt sFileSeekTimeNext_ = 0;
sInt sFileSeekTimeEmulation(sInt seektime_in_ms/*=-1*/)
{
  if(seektime_in_ms>-1)
    sFileSeekTimeEmulation_ = seektime_in_ms;
  return sFileSeekTimeEmulation_;
}
#endif


class sRootFileHandler : public sFileHandler
{
  friend class sRootFile;

  static const sInt MAX_READENTRIES=128;

  struct ReadEntry
  {
    sU8 *Buffer;
    sDInt Size;
    OVERLAPPED Ovl;
    sBool Active;
    sInt Next;
  };

  sThreadLock *Lock;
  sStaticArray<ReadEntry> ReadEntries;
  sInt FirstFreeEntry;

  sInt AllocReadHandle();
  void FreeReadHandle(sInt h);

  void DumpFreeHandles();
  void DumpFreeHandles(const sStringDesc &fillMe);
public:

  sFile *Create(const sChar *name,sFileAccess access);
  sBool Exists(const sChar *name);

  sRootFileHandler();
  ~sRootFileHandler();
};

class sRootFile : public sFile
{
  HANDLE File;
  sFileAccess Access;
  sS64 Size;
  sS64 Offset;
  sBool Ok;

  sS64 MapOffset;
  sDInt MapSize;
  sU8 *MapPtr;             // 0 = mapping not active
  HANDLE MapHandle;
  sBool MapFailed;          // 1 = we tryed once to map and it didn't work, do don't try again
  sRootFileHandler *Handler;

  sBool IsOverlapped;
  HANDLE OvlEvent;

#if !sSTRIPPED
  sInt SeekDoneTime;
#endif

public:
  sRootFile(HANDLE file,sFileAccess access,sRootFileHandler *h,sBool isovl);
  ~sRootFile();
  sBool Close();
  sBool Read(void *data,sDInt size); 
  sBool Write(const void *data,sDInt size);
  sU8 *Map(sS64 offset,sDInt size); 
  sBool SetOffset(sS64 offset);      
  sS64 GetOffset();                  
  sBool SetSize(sS64);               
  sS64 GetSize();                    

  sFileReadHandle BeginRead(sS64 offset,sDInt size,void *destbuffer, sFilePriorityFlags prio);  // begin reading
  sBool DataAvailable(sFileReadHandle handle);  // data valid?
  void *GetData(sFileReadHandle handle);  // access data (only valid if you didn't specify the buffer yourself!)
  void EndRead(sFileReadHandle handle);   // the data buffer may be reused now
};

/*static*/ void sAddRootFilesystem()
{
  sAddFileHandler(new sRootFileHandler);
}

sADDSUBSYSTEM(RootFileHandler,0x28,sAddRootFilesystem,0);

/****************************************************************************/

sRootFileHandler::sRootFileHandler()
{
  ReadEntries.HintSize(MAX_READENTRIES);
  ReadEntries.AddMany(MAX_READENTRIES);
  for (sInt i=0; i<MAX_READENTRIES-1; i++)
    ReadEntries[i].Next=i+1;
  ReadEntries[MAX_READENTRIES-1].Next=-1;
  FirstFreeEntry=0;
  Lock = new sThreadLock;
}

sRootFileHandler::~sRootFileHandler()
{
  sDelete(Lock);
}

sInt sRootFileHandler::AllocReadHandle()
{
  sScopeLock sl(Lock);
  //sDPrintF(L"alloc:"); DumpFreeHandles();

  sInt e=FirstFreeEntry;
  if (e<0) sFatal(L"sRootFileHandler: out of async read entries!\n");
  FirstFreeEntry=ReadEntries[e].Next;
  ReadEntries[e].Next=e;
  return e;
}

void sRootFileHandler::FreeReadHandle(sInt rh)
{
  sVERIFY(ReadEntries[rh].Next==rh); // avoid double deletes
  ReadEntries[rh].Next=FirstFreeEntry;
  FirstFreeEntry=rh;
  
  //sDPrintF(L"free:"); DumpFreeHandles();
}

void sRootFileHandler::DumpFreeHandles()
{
  sString<1024> s;

  DumpFreeHandles(s);

  sLogF(L"sys",s);
}

void sRootFileHandler::DumpFreeHandles(const sStringDesc &fillMe)
{
  sInt freeHandleCount = 0;
  sInt currentFreeEntry = FirstFreeEntry;

  while (currentFreeEntry>=0)
  {
    currentFreeEntry = ReadEntries[currentFreeEntry].Next;
    freeHandleCount++;
  }

  sSPrintF(fillMe, L"Free async sRootFileHandler entries left: %d\n", freeHandleCount);
}

sBool sRootFileHandler::Exists(const sChar *name)
{
  WIN32_FIND_DATA wfd;
  HANDLE hndl;
  hndl=FindFirstFile(name,&wfd);
  if (hndl==INVALID_HANDLE_VALUE)
    return sFALSE;
  FindClose(hndl);
  return sTRUE;
}

sFile *sRootFileHandler::Create(const sChar *name,sFileAccess access)
{
  HANDLE handle = INVALID_HANDLE_VALUE;
  static const sChar *mode[] =  {  0,L"read",L"readrandom",L"write",L"writeappend",L"readwrite"  };

  sBool isovl=sFALSE;

  switch(access)
  {
  case sFA_READ:
    handle = CreateFileW(name,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN|FILE_FLAG_OVERLAPPED,0);
    isovl=sTRUE;
    break;
  case sFA_READRANDOM:
    handle = CreateFileW(name,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,0);
    isovl=sTRUE;
    break;
  case sFA_WRITE:
    handle = CreateFileW(name,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_FLAG_SEQUENTIAL_SCAN,0);
    break;
  case sFA_WRITEAPPEND:
    handle = CreateFileW(name,GENERIC_WRITE,0,0,OPEN_ALWAYS,FILE_FLAG_SEQUENTIAL_SCAN,0);
    break;
  case sFA_READWRITE:
    handle = CreateFileW(name,GENERIC_WRITE|GENERIC_READ,0,0,OPEN_ALWAYS,0,0);
    break;
  default: sVERIFYFALSE;
  }

  if(handle!=INVALID_HANDLE_VALUE)
  {
    sLogF(L"file",L"open file <%s> for %s\n",name,mode[access]);
    sFile *file = new sRootFile(handle,access,this,isovl);
    return file;
  }
  else 
  {
    sLogF(L"file",L"failed to open file <%s> for %s\n",name,mode[access]);
    return 0;
  }
}

/****************************************************************************/

sRootFile::sRootFile(HANDLE file,sFileAccess access,sRootFileHandler *h, sBool isovl)
{
  File = file;
  Access = access;
  Handler=h;
  Offset = 0;
  Ok = 1;
  MapOffset = 0;
  MapSize = 0;
  MapPtr = 0;
  MapHandle = INVALID_HANDLE_VALUE;
  MapFailed = 0;

  IsOverlapped=isovl;
  if (isovl)
    OvlEvent=CreateEvent(0,0,0,0);
  else
    OvlEvent=INVALID_HANDLE_VALUE;

  // get file size

  LARGE_INTEGER s;
  Ok = GetFileSizeEx(File,&s);
  Size = s.QuadPart;
#if !sSTRIPPED
  SeekDoneTime=0;
#endif
}

sRootFile::~sRootFile()
{
  if(File!=INVALID_HANDLE_VALUE)
    Close();
  if(OvlEvent!=INVALID_HANDLE_VALUE)
    CloseHandle(OvlEvent);
}

sBool sRootFile::Close()
{
  sVERIFY(File!=INVALID_HANDLE_VALUE);

  if(MapPtr)
    if(!UnmapViewOfFile(MapPtr))
      Ok = 0;
  if(MapHandle!=INVALID_HANDLE_VALUE)
    if(!CloseHandle(MapHandle))
      Ok = 0;
  if(!CloseHandle(File))
    Ok = 0;
  File = INVALID_HANDLE_VALUE;
  return Ok;
}

sBool sRootFile::Read(void *data,sDInt size)
{
  sVERIFY(File!=INVALID_HANDLE_VALUE)
  DWORD read=0;
  sVERIFY(size<=0x7fffffff);

  sBool result;
  if (IsOverlapped)
  {
    OVERLAPPED ovl;
    sClear(ovl);
    ovl.Offset=sU32(Offset);
    ovl.OffsetHigh=sU32(Offset>>32);
    ovl.hEvent=OvlEvent;

    result=ReadFile(File,data,size,&read,&ovl);
    while(!result)
    {
      sU32 error = GetLastError();
      if(ERROR_NO_SYSTEM_RESOURCES==error)
      {
        // try smaller chunks
        sU8 *dest = (sU8*)data;
        sDInt left = size;
        result = 1;
        while(left && result)
        {
          sInt chunk=sMin(left,sDInt(1024*1024));
          result=ReadFile(File,dest,chunk,&read,&ovl);
          if(!result && ERROR_IO_PENDING==GetLastError())
            result=GetOverlappedResult(File,&ovl,&read,TRUE);

          dest += read;
          left -= read;

          sU64 offset = (sU64(ovl.OffsetHigh)<<32)|ovl.Offset;
          offset += read;
          ovl.Offset=sU32(offset);
          ovl.OffsetHigh=sU32(offset>>32);
        }
        read = dest-(sU8*)data;
      }
      else if (error==ERROR_IO_PENDING)
        result=GetOverlappedResult(File,&ovl,&read,TRUE);
      else
      {
        sLogF(L"file",L"read error: %08x\n",error);
        break; // error
      }
    }
  }
  else
  {
    result = ReadFile(File,data,size,&read,0);
  }

  if(read!=sU32(size))
    result = 0;
  if(!result) Ok = 0;
  Offset+=read;
  return result;
}

sBool sRootFile::Write(const void *data,sDInt size)
{
  sVERIFY(!IsOverlapped)
  sVERIFY(File!=INVALID_HANDLE_VALUE)

  // do not write more than 16MB at a time.
  // there is a limit for network files at 33524976 bytes (!)  
  // there seems to be no such limit for reads

  const sInt chunk = 16*1024*1024; 
  if(size<=chunk)
  {
    DWORD written;
    sVERIFY(size<=0x7fffffff);
    sBool result = WriteFile(File,data,size,&written,0);
    if(written!=sU32(size))
      result = 0;
    if(!result) Ok = 0;
    Offset+=written;
    return result;
  }
  else
  {
    const sU8 *ptr = (const sU8 *) data;
    while(size>chunk)
    {
      if(!Write(ptr,chunk))
        return 0;
      ptr += chunk;
      size -= chunk;
    }
    sVERIFY(size>0);
    return Write(ptr,size);
  }
}

sU8 *sRootFile::Map(sS64 offset,sDInt size)
{
  // try mapping only once

  if(MapFailed) return 0;

  // prepare mapping

  if(MapHandle==INVALID_HANDLE_VALUE)
  {
    if(Access!=sFA_READ && Access!=sFA_READRANDOM)    // only supported for reading
    {
      MapFailed = 1;
      return 0;
    }
    MapHandle = CreateFileMapping(File,0,PAGE_READONLY,0,0,0);
    if(MapHandle==INVALID_HANDLE_VALUE)
    {
      MapFailed = 1;
      return 0;
    }
  }

  // unmap current view

  if(MapPtr)
    UnmapViewOfFile(MapPtr);
  MapPtr = 0;

  // map new view

  if(size>=0x7fffffff) return 0;
  MapPtr = (sU8 *)MapViewOfFile(MapHandle,FILE_MAP_READ,offset>>32,offset&0xffffffff,size);
  if(MapPtr)
  {
    MapOffset = offset;
    MapSize = size;
  }
  else
  {
    MapOffset = 0;
    MapSize = 0;
  }
  return MapPtr;
}

sBool sRootFile::SetOffset(sS64 offset)
{
  sVERIFY(File!=INVALID_HANDLE_VALUE)

  Offset = offset;

  if (!IsOverlapped)
  {
    LARGE_INTEGER o;
    o.QuadPart = offset;
    sBool result = SetFilePointerEx(File,o,0,FILE_BEGIN);
    if(!result) Ok = 0;
    return result;
  }

  return Offset>=0 && Offset<=Size;
}

sS64 sRootFile::GetOffset()
{
  return Offset;
}

sBool sRootFile::SetSize(sS64 size)
{
  sVERIFY(File!=INVALID_HANDLE_VALUE)
  sVERIFY(!IsOverlapped)
  if(Ok)
  {
    Size = size;
    Ok &= SetOffset(size);
    Ok &= (SetEndOfFile(File)!=0);
    Ok &= SetOffset(sMin(Size,Offset));
  }
  return Ok;
}

sS64 sRootFile::GetSize()
{
  return Size;
}

/****************************************************************************/

sFileReadHandle sRootFile::BeginRead(sS64 offset,sDInt size,void *destbuffer, sFilePriorityFlags prio)
{
  sInt handle=Handler->AllocReadHandle();
  sRootFileHandler::ReadEntry &e=Handler->ReadEntries[handle];
  e.Size=size;
  e.Buffer=0;
  sClear(e.Ovl);

  if (!destbuffer)
    destbuffer = e.Buffer = new sU8[size];

  sVERIFY(offset + size <= Size);

  e.Ovl.Offset=sU32(offset);
  e.Ovl.OffsetHigh=sU32(offset>>32);
  ReadFile(File,destbuffer,size,0,&e.Ovl);
  e.Active=sTRUE;

#if !sSTRIPPED
  if(sFileSeekTimeEmulation_>0)
  {
    sInt time = sGetTime();
    sFileSeekTimeNext_ = sMax(sFileSeekTimeNext_+sFileSeekTimeEmulation_,time+sFileSeekTimeEmulation_);
    //sDPrintF(L"BeginRead delay %d ms\n",sFileSeekTimeNext_-time);
    SeekDoneTime = sFileSeekTimeNext_;
  }
#endif

  return handle;
}

sBool sRootFile::DataAvailable(sFileReadHandle handle)
{
#if !sSTRIPPED
  sInt time = sGetTime();
  if(sFileSeekTimeEmulation_>0 && time<SeekDoneTime)
    return sFALSE;
#endif

  sVERIFY(handle>=0 && handle<sRootFileHandler::MAX_READENTRIES);
  sRootFileHandler::ReadEntry &e=Handler->ReadEntries[handle];

  DWORD read;
  if (GetOverlappedResult(File,&e.Ovl,&read,FALSE))
  {
    sVERIFY((sDInt)read==e.Size);
    e.Active=sFALSE;
    return sTRUE;
  }

  DWORD error=GetLastError();
  if (error!=ERROR_IO_INCOMPLETE)
    sFatal(L"sRootFile: read error during async io!\n");
  
  return sFALSE;
}

void *sRootFile::GetData(sFileReadHandle handle)
{
  sVERIFY(handle>=0 && handle<sRootFileHandler::MAX_READENTRIES);
  sRootFileHandler::ReadEntry &e=Handler->ReadEntries[handle];
  if (e.Active) return 0;
  return e.Buffer;
}

void sRootFile::EndRead(sFileReadHandle handle)
{
#if !sSTRIPPED
  sInt time = sGetTime();
  if(sFileSeekTimeEmulation_>0 && time<SeekDoneTime)
  {
    //sDPrintF(L"EndRead block %d ms\n",SeekDoneTime-time);
    sSleep(SeekDoneTime-time);
  }
#endif

  sVERIFY(handle>=0 && handle<sRootFileHandler::MAX_READENTRIES);
  sRootFileHandler::ReadEntry &e=Handler->ReadEntries[handle];
  if (e.Active)
  {
    DWORD read;
    GetOverlappedResult(File,&e.Ovl,&read,TRUE);
  }
  sDeleteArray(e.Buffer);
  Handler->FreeReadHandle(handle);
}

/****************************************************************************/
/***                                                                      ***/
/***   File Operation                                                     ***/
/***                                                                      ***/
/****************************************************************************/

sBool sCopyFile(const sChar *source,const sChar *dest,sBool failifexists)
{
  sLogF(L"file",L"copy from <%s>\n",source);
  sLogF(L"file",L"copy to   <%s>\n",dest);
  if (CopyFileW(source,dest,failifexists))
  {
    SetFileAttributes(dest,GetFileAttributes(dest)&~FILE_ATTRIBUTE_READONLY);
    return sTRUE;
  }
  else
  {
    sLogF(L"file",L"copy failed to <%s>\n",dest);
  }
  return sFALSE;
}

sBool sRenameFile(const sChar *source,const sChar *dest, sBool overwrite/*=sFALSE*/)
{
  sLogF(L"file",L"rename from <%s>\n",source);
  sLogF(L"file",L"rename to   <%s>\n",dest);

  if(MoveFileExW(source,dest,overwrite?MOVEFILE_REPLACE_EXISTING:0))
  {
    return 1;
  }
  else
  {
    sLogF(L"file",L"rename failed to <%s>\n",dest);
    return 0;;
  }
}

// the array is highly inefficient in this case, because of large reallocations!
// a linked list would be a siutable container

sBool sCheckDir(const sChar *name)
{
  HANDLE file = CreateFileW(name,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,0);
  if(file!=INVALID_HANDLE_VALUE)
    CloseHandle(file);
  return file!=INVALID_HANDLE_VALUE;
}

sBool sLoadDir(sArray<sDirEntry> &list,const sChar *path,const sChar *pattern)
{
  const sInt sMAX_PATHNAME = 2048;
  sString<sMAX_PATHNAME> buffer;
  HANDLE handle;
  sInt len;
  WIN32_FIND_DATAW dir;
  sDirEntry *de;

  if (!pattern) pattern=L"*.*";

  list.Clear();

  buffer = path;
  len = sGetStringLen(buffer);
  if(path[len-1]!='/' && path[len-1]!='\\')
    buffer.Add(L"\\");
  buffer.Add(pattern);
  len=sGetStringLen(buffer);
  if(buffer[len-1]=='/' || buffer[len-1]=='\\')
    buffer[len-1]=0;

  handle = FindFirstFileW(buffer,&dir);
  if(handle==INVALID_HANDLE_VALUE)
    return 0;

  do
  {
    if(sCmpString(dir.cFileName,L".")==0)
      continue;
    if(sCmpString(dir.cFileName,L"..")==0)
      continue;

    de = list.AddMany(1);
    de->Flags = sDEF_EXISTS;
    if(dir.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      de->Flags |= sDEF_DIR;
    if(dir.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
      de->Flags |= sDEF_WRITEPROTECT;

    de->Size = dir.nFileSizeLow;
    if(de->Size<0 || dir.nFileSizeHigh)
      de->Size = 0x7fffffff;
    de->Name = (sChar *)dir.cFileName;
    de->LastWriteTime = dir.ftLastWriteTime.dwLowDateTime + (((sU64) dir.ftLastWriteTime.dwHighDateTime)<<32);
  }
  while(FindNextFileW(handle,&dir));

  FindClose(handle);

/*
  sInt i,j;
  sInt max;
  max = ;

  for(i=0;i<max-1;i++)
    for(j=i+1;j<max;j++)
      if(de[i].IsDir<de[j].IsDir || (de[i].IsDir==de[j].IsDir && sCmpStringI(de[i].Name,de[j].Name)>0))
        sSwap(de[i],de[j]);
*/

  return 1;
}

sDateAndTime sFromFileTime(sU64 lastWriteTime)
{
  FILETIME ft;

  ft.dwLowDateTime = lastWriteTime & 0xffffffff;
  ft.dwHighDateTime = lastWriteTime >> 32;
  return TimeFromFileTime(ft);
}

sU64 sToFileTime(sDateAndTime time)
{
  FILETIME ft;
  TimeToFileTime(ft,time);
  return (sU64(ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
}

sBool sFindFile(sChar *foundname, sInt foundnamesize, const sChar *path, const sChar *pattern)
{
  const sInt sMAX_PATHNAME = 2048;
  sString<sMAX_PATHNAME> buffer;
  HANDLE handle;
  sInt len;
  WIN32_FIND_DATAW dir;

  if (!pattern) pattern=L"*.*";

  buffer = path;
  len = sGetStringLen(buffer);
  if(path[len-1]!='/' && path[len-1]!='\\')
    buffer.Add(L"\\");
  buffer.Add(pattern);
  len=sGetStringLen(buffer);
  if(buffer[len-1]=='/' || buffer[len-1]=='\\')
    buffer[len-1]=0;

  handle = FindFirstFileW(buffer,&dir);
  if(handle==INVALID_HANDLE_VALUE)
    return 0;

  sCopyString(foundname, (sChar *)dir.cFileName, foundnamesize);

  FindClose(handle);

  return 1;
}

/****************************************************************************/

void sSetProjectDir(const sChar *name)
{
  const sChar *path[] =
  {
    L"c:/nxn/",
    L"e:/nxn/",
    L"g:/nxn/",
    L"../../",
  };
  sString<2048> buf;

  for(sInt i=0;i<sCOUNTOF(path);i++)
  {
    buf = path[i];
    buf.Add(name);
    if(sChangeDir(buf))
      return;
  }
  sLogF(L"file",L"sSetProjectDir could not find project <%s>\n",name);

//  sFatal(L"could not find project directory"); silently fail!
}


sBool sChangeDir(const sChar *dir)
{
  sBool ok = SetCurrentDirectoryW(dir)!=0;
  if(ok) sLogF(L"file",L"sChangeDir <%s>\n",dir);
  return ok;
}

void sGetCurrentDir(const sStringDesc &str)
{
  GetCurrentDirectoryW(str.Size,str.Buffer);
}

void sGetTempDir(const sStringDesc &str)
{
  GetTempPath(str.Size,str.Buffer);
  sInt pos;
  while ((pos=sFindFirstChar(str.Buffer,'\\'))>=0) str.Buffer[pos]='/';
}

sBool sMakeDir(const sChar *name)
{
  return CreateDirectoryW(name,0);
}

sBool sDeleteFile(const sChar *name)
{
  return DeleteFileW(name);
}

sBool sDeleteDir(const sChar *name)
{
  return RemoveDirectoryW(name);
}

sBool sGetFileWriteProtect(const sChar *name,sBool &prot)
{
  prot = 0;
  DWORD attr = GetFileAttributes(name);
  if(attr==INVALID_FILE_ATTRIBUTES)
    return 0;
  if(attr & FILE_ATTRIBUTE_READONLY)
    prot = 1;
  sLogF(L"file",L"get attribute <%s> %d\n",name,prot);
  return 1;
}

sBool sSetFileWriteProtect(const sChar *name,sBool prot)
{
  DWORD attr = GetFileAttributes(name);
  if(attr==INVALID_FILE_ATTRIBUTES)
    return 0;
  if(prot)
    attr |= FILE_ATTRIBUTE_READONLY;
  else
    attr &= ~FILE_ATTRIBUTE_READONLY;
  sLogF(L"file",L"set attribute <%s> %d\n",name,prot);
  return SetFileAttributes(name,attr)!=0;
}

sBool sIsBelowCurrentDir(const sChar *relativePath)
{
  sString<4096> currentDir;
  sString<4096> normalizedDir;

  sGetCurrentDir(currentDir);
  GetFullPathNameW(relativePath,normalizedDir.Size(),normalizedDir,0);
  return sCheckPrefix(normalizedDir,currentDir);
}

sBool sGetFileInfo(const sChar *name,sDirEntry *de)
{
  HANDLE hnd;
  FILETIME time;
  DWORD sizehigh;
  DWORD attr;
  sBool ok = 0;

  // clear infgo

  de->Name = sFindFileWithoutPath(name);
  de->Flags = 0;
  de->Size = 0;
  de->LastWriteTime = 0;

  // open file

  hnd = CreateFileW(name,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
  if(hnd!=INVALID_HANDLE_VALUE)
  {
    de->Flags = sDEF_EXISTS;

    // size

    de->Size = GetFileSize(hnd,&sizehigh);
    if(sizehigh!=0 || de->Size<0)
      de->Size = 0x7fffffff;

    // time

    GetFileTime(hnd,0,0,&time);
    de->LastWriteTime = time.dwLowDateTime + (((sU64) time.dwHighDateTime)<<32);

    // attributes

    attr = GetFileAttributesW(name);
    if(attr!=INVALID_FILE_ATTRIBUTES)
    {
      if(attr & FILE_ATTRIBUTE_DIRECTORY)
        de->Flags |= sDEF_DIR;
      if(attr & FILE_ATTRIBUTE_READONLY)
        de->Flags |= sDEF_WRITEPROTECT;

      ok = 1;
    }
  }

  // done

  CloseHandle(hnd);
  
  return ok;
}


sBool sGetDiskSizeInfo(const sChar *path, sS64 &availablesize, sS64 &totalsize)
{
  ULARGE_INTEGER available;
  ULARGE_INTEGER total;
  BOOL result = GetDiskFreeSpaceExW(path,&available,&total,0);
  availablesize = available.QuadPart;
  totalsize = total.QuadPart;
  return result;
}


sBool sSetFileTime(const sChar *name, sU64 lastwritetime)
{
  sBool ok = 0;
  HANDLE hnd = CreateFileW(name,GENERIC_WRITE,0,0,OPEN_EXISTING,0,0);
  if(hnd!=INVALID_HANDLE_VALUE)
  {
    FILETIME time;
    time.dwHighDateTime = (lastwritetime>>32)&0xffffffff;
    time.dwLowDateTime = lastwritetime&0xffffffff;
    BOOL result = SetFileTime(hnd,0,0,&time);
    ok = (result!=0);
  }
  CloseHandle(hnd);
  return ok;
}

/****************************************************************************/
/***                                                                      ***/
/***   Strack trace code                                                  ***/
/***                                                                      ***/
/****************************************************************************/

#include <dbghelp.h>

#if sCONFIG_32BIT
#define CHOOSE64(a,b) a
#else
#define CHOOSE64(a,b) b
#endif

typedef void (__stdcall *PRTLCAPTURECONTEXT)(PCONTEXT);
typedef BOOL (__stdcall *PSYMINITIALIZE)(HANDLE,char*,BOOL);
typedef BOOL (__stdcall *PSYMCLEANUP)(HANDLE);
typedef DWORD (__stdcall *PSYMSETOPTIONS)(DWORD);
typedef BOOL (__stdcall *PSTACKWALK64)(DWORD,HANDLE,HANDLE,LPSTACKFRAME64,PVOID,PREAD_PROCESS_MEMORY_ROUTINE64,PFUNCTION_TABLE_ACCESS_ROUTINE64,PGET_MODULE_BASE_ROUTINE64,PTRANSLATE_ADDRESS_ROUTINE64);
typedef DWORD64 (__stdcall *PSYMGETMODULEBASE64)(HANDLE,DWORD64);
typedef PVOID (__stdcall *PSYMFUNCTIONTABLEACCESS64)(HANDLE,DWORD64);
typedef BOOL (__stdcall *PSYMGETLINEFROMADDR64)(HANDLE,DWORD64,PDWORD,PIMAGEHLP_LINE64);
typedef BOOL (__stdcall *PSYMFROMADDR)(HANDLE,DWORD64,PDWORD64,PSYMBOL_INFO);

#define DEBUGHELP_STAYS_IN_MEMORY 1

static HMODULE hDbgHelp = sNULL;
static sBool DbgHelpInited = sFALSE;

static sBool sStackTraceFromContext(const sStringDesc &tgt,sInt skipCount,sInt maxCount,CONTEXT &ctx)
{
#if !sCONFIG_BUILD_STRIPPED
  HANDLE hProcess = GetCurrentProcess();
  HANDLE hThread = GetCurrentThread();

  // load debughelp dll+required functions if not loaded already
  if (!hDbgHelp)
  {
    hDbgHelp = LoadLibrary(L"dbghelp.dll");
    DbgHelpInited=sFALSE;
  }
  if(!hDbgHelp)
    return sFALSE;

  PSYMINITIALIZE SymInitialize = (PSYMINITIALIZE) GetProcAddress(hDbgHelp,"SymInitialize");
  PSYMCLEANUP SymCleanup = (PSYMCLEANUP) GetProcAddress(hDbgHelp,"SymCleanup");
  PSYMSETOPTIONS SymSetOptions = (PSYMSETOPTIONS) GetProcAddress(hDbgHelp,"SymSetOptions");
  PSTACKWALK64 StackWalk64 = (PSTACKWALK64) GetProcAddress(hDbgHelp,"StackWalk64");
  PSYMGETMODULEBASE64 SymGetModuleBase64 = (PSYMGETMODULEBASE64) GetProcAddress(hDbgHelp,"SymGetModuleBase64");
  PSYMFUNCTIONTABLEACCESS64 SymFunctionTableAccess64 = (PSYMFUNCTIONTABLEACCESS64) GetProcAddress(hDbgHelp,"SymFunctionTableAccess64");
  PSYMGETLINEFROMADDR64 SymGetLineFromAddr64 = (PSYMGETLINEFROMADDR64) GetProcAddress(hDbgHelp,"SymGetLineFromAddr64");
  PSYMFROMADDR SymFromAddr = (PSYMFROMADDR) GetProcAddress(hDbgHelp,"SymFromAddr");

  if (!DbgHelpInited)
  {

    if(!SymInitialize || !SymCleanup || !SymSetOptions || !StackWalk64
      || !SymGetModuleBase64 || !SymFunctionTableAccess64 || !SymFromAddr)
    {
      FreeLibrary(hDbgHelp);
      hDbgHelp = sNULL;
      return FALSE;
    }

    // determine symbol path: start with module directory
    sString<2048> symPath,temp;
    if(GetModuleFileNameW(0,&temp[0],temp.Size()))
    {
      temp[temp.Size()-1] = 0;
      sExtractPath(temp,symPath);

      sInt len = symPath.Count();
      if(len && (symPath[len-1]=='/' || symPath[len-1]=='\\'))
        symPath[len-1] = 0;
    }

    // also add some other environment vars to search path
    static const sChar *envs[] = { L"windir",L"_NT_SYMBOL_PATH",L"_NT_ALTERNATE_SYMBOL_PATH" };

    for(sInt i=0;i<sCOUNTOF(envs);i++)
    {
      if(sGetEnvironmentVariable(temp,envs[i]))
      {
        symPath.Add(L";");
        symPath.Add(temp);
      }
    }

    // translate to ANSI
    sChar8 symPathMB[2048];
    WideCharToMultiByte(CP_ACP,0,symPath,symPath.Count(),symPathMB,sCOUNTOF(symPathMB),0,FALSE);
    symPathMB[sCOUNTOF(symPathMB)-1] = 0;

    // actually initialize symbol resolution
    SymSetOptions(SYMOPT_LOAD_LINES|SYMOPT_UNDNAME);
    if(!SymInitialize(hProcess,symPathMB,TRUE))
    {
      FreeLibrary(hDbgHelp);
      hDbgHelp = sNULL;

      return sFALSE;
    }

    DbgHelpInited=sTRUE;
  }

  // prepare stack walk
  STACKFRAME64 frame;
  sClear(frame);
  frame.AddrPC.Mode = AddrModeFlat;
  frame.AddrPC.Offset = CHOOSE64(ctx.Eip,ctx.Rip);
  frame.AddrFrame.Mode = AddrModeFlat;
  frame.AddrFrame.Offset = CHOOSE64(ctx.Ebp,ctx.Rbp);
  frame.AddrStack.Mode = AddrModeFlat;
  frame.AddrStack.Offset = CHOOSE64(ctx.Esp,ctx.Rsp);
  DWORD machineType = CHOOSE64(IMAGE_FILE_MACHINE_I386,IMAGE_FILE_MACHINE_AMD64);

  sInt frameCounter = 0;

  do
  {
    if(frameCounter >= skipCount && frameCounter < skipCount+maxCount) // skip first frame (our own)
    {
      static const sInt MaxNameLen = 1024;

      // found a stack frame! try to find the function and line number.
      IMAGEHLP_LINE64 line;
      sInt symMemAmount = sizeof(SYMBOL_INFO) + (MaxNameLen-1)*sizeof(sChar8);
      sU8 *symMem = new sU8[symMemAmount];
      SYMBOL_INFO *sym = (SYMBOL_INFO *) symMem;

      sSetMem(symMem,0,symMemAmount);
      sym->SizeOfStruct = sizeof(SYMBOL_INFO);
      sym->MaxNameLen = MaxNameLen;

      sClear(line);
      line.SizeOfStruct = sizeof(line);

      DWORD disp;
      DWORD64 disp64;
      sString<512> buffer;
      sInt index = frameCounter - skipCount + 1;

      BOOL gotSym = SymFromAddr(hProcess,frame.AddrPC.Offset,&disp64,sym);
      BOOL gotLine = SymGetLineFromAddr64(hProcess,frame.AddrPC.Offset,&disp,&line);

      if(gotSym && gotLine)
      {
        sString<256> fileName,funcName;
        sCopyString(fileName,line.FileName);
        sCopyString(funcName,sym->Name);

        const sChar *fileNameShort = sFindFileWithoutPath(fileName);

        sSPrintF(buffer,L"%2d. %s (%s:%d)\n",index,funcName,fileNameShort,(sInt)line.LineNumber);
      }
      else
        sSPrintF(buffer,L"%2d. ??? (EIP=%08x)\n",index,frame.AddrPC.Offset);

      sAppendString(tgt,buffer);

      delete[] symMem;
    }

    frameCounter++;
  } while(StackWalk64(machineType,hProcess,hThread,&frame,&ctx,0,SymFunctionTableAccess64,SymGetModuleBase64,0));

  // if stacktrace is active in sStaticArray we better not unload the library
#if !DEBUGHELP_STAYS_IN_MEMORY
  SymCleanup(hProcess);
  FreeLibrary(hDbgHelp);
  hDbgHelp = sNULL;
#endif

  return sTRUE;
#else
  return sFALSE; // stripped builds don't implement this.
#endif
}

sBool sStackTrace(const sStringDesc &tgt,sInt skipCount,sInt maxCount)
{
#if !sCONFIG_BUILD_STRIPPED
  // grab the current thread context

#if sCONFIG_32BIT

  CONTEXT ctx;
  __asm
  {
    lea   eax, [ctx];
    lea   ecx, eip_ref;
    mov   [eax + CONTEXT.Eip], ecx;
    mov   [eax + CONTEXT.Ebp], ebp;
    mov   [eax + CONTEXT.Esp], esp;
eip_ref:
  }

  skipCount++;

#else

  HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");
  if(!hKernel)
    return sFALSE; // really, this should never happen.

  PRTLCAPTURECONTEXT RtlCaptureContext = (PRTLCAPTURECONTEXT) GetProcAddress(hKernel,"RtlCaptureContext");
  if(!RtlCaptureContext)
    return sFALSE; // this function needs win xp.

  CONTEXT ctx;
  RtlCaptureContext(&ctx);
  skipCount++;

#endif

  return sStackTraceFromContext(tgt,skipCount,maxCount,ctx);

#else
  return sFALSE; // stripped builds don't implement this.
#endif
}

#undef CHOOSE64

#if sALLOW_MEMDEBUG_STACKTRACE

sBool sGetMemTagged() { return sGetThreadContext()->TagMemFile != 0; }

void sGetCaller(const sStringDesc &filename, sInt &line)
{
  sStackTrace(filename, 4, 1);
  line = 0;

  // the format we assume is
  //   n. classname::methodname (filename:linenumber)
  // so we should first scan for the brackets
  sInt pos_A = sFindFirstChar(filename.Buffer,L'(');
  sInt pos_B = sFindFirstChar(filename.Buffer,L')');

  if (pos_A>=0 && pos_B>=0)
  {
    sString<256> s = filename.Buffer;
    
    s.CutRight(s.Count() - pos_B);
    s.CutLeft(pos_A+1);

    // get linenumber which should be behind last ':'
    pos_B = sFindLastChar(s, L':');
    if (pos_B>=0)
    {
      const sChar *ptr = ((const sChar *)s) + pos_B + 1;
      sScanInt(ptr, line);
    }

    // get filename which should be right before first ':'
    pos_A = sFindFirstChar(s, L':');
    if (pos_A>=0)
      s[pos_A] = 0;

    sCopyString(filename, s);
  }


}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   MainLoops                                                          ***/
/***                                                                      ***/
/****************************************************************************/


/****************************************************************************/
/***                                                                      ***/
/***   windows handling                                                   ***/
/***                                                                      ***/
/****************************************************************************/

static LONG MakeFatal(LPEXCEPTION_POINTERS except)
{
  if(IsDebuggerPresent())
  {
#if sCONFIG_64BIT
    // this will recurse for a while and then crash to the debugger with a stack overflow.
    // calling UnhandledExceptionFilter won't work with 64 bits for some reason.
    except->ExceptionRecord->ExceptionFlags |= EXCEPTION_NONCONTINUABLE;
    return EXCEPTION_CONTINUE_EXECUTION;
#else
    return UnhandledExceptionFilter(except);
#endif
  }
  else
    return EXCEPTION_EXECUTE_HANDLER;
}

namespace sWin32
{
  extern sBool ModalDialogActive;
}

#define INPUT2_EVENT_QUEUE_SIZE 64
extern sLockQueue<sInput2Event, INPUT2_EVENT_QUEUE_SIZE>* sInput2EventQueue;

LRESULT WINAPI MsgProc(HWND win,UINT msg,WPARAM wparam,LPARAM lparam)
{
  LPEXCEPTION_POINTERS exceptInfo = 0;

  __try {

#if sCRASHDUMP
  __try
  {
#endif
//  sDPrintF(L"msg(%0x8) %08x %08x %08x\n",sGetTime(),msg,wparam,lparam);

  sInt i,t;
  //sBool mouse=0;

  switch(msg)
  {

// system messages
  case WM_SYSCOMMAND:
    if(sSystemFlags & sISF_NOSCREENSAVE)
    {
      if(wparam == SC_SCREENSAVE || wparam == SC_MONITORPOWER)
        return 0; // forbit starting screensaver
    }
    return DefWindowProc(win,msg,wparam,lparam);

  case WM_CLOSE:
    {
      sBool mayexit = 1;
      sAltF4Hook->Call(mayexit);
      if(mayexit)
        sExit(); 
    }
    break;

  case WM_DESTROY:
    if(!sFatalFlag)
    {
      sDelete(sAppPtr);             // destroy application before reducing run level
                                    // otherwise gives problems when application uses subsystems
                                    // which are shutdown with sSetRunlevel
      DeleteDC(sGDIDCOffscreen);
      sGDIDCOffscreen = 0;
      sCollect();
      sCollector(sTRUE);
      sSetRunlevel(0x80);

      if(sSystemFlags & sISF_3D)
        ExitGFX();
    }
    PostQuitMessage(0);
    break;

  case WM_CREATE:
    {
      HDC hdc;
      hdc = GetDC(sExternalWindow ? sExternalWindow : win);    // use external window if available
      sGDIDCOffscreen = CreateCompatibleDC(hdc);
      ReleaseDC(sExternalWindow ? sExternalWindow : win,hdc);
    }
    break;

  case WM_PAINT:
    {
      if(sGetRunlevel() < 0x100) // not fully initialized yet. just paint a black rect!
      {
        PAINTSTRUCT ps;
        BeginPaint(win,&ps);
        FillRect(ps.hdc,&ps.rcPaint,(HBRUSH) GetStockObject(BLACK_BRUSH));
        EndPaint(win,&ps);
        return 0;
      }

      if (sInPaint>1) break;
      if (sInPaint) 
        sDPrintF(L"warning: recursive paint detected\n");
      sInPaint++;

      win = sExternalWindow ? sExternalWindow : win;              // use external window if available
      sFrameHook->Call();
      if (sInput2EventQueue) {
        sInput2Event event;
        while (sInput2PopEvent(event, sGetTime())) {
#if !sSTRIPPED
          sBool skip = sFALSE;
          sInputHook->Call(event, skip);
          if (!skip)
#endif
            sAppPtr->OnInput(event);
        }
      }

      sBool app_fullpaint = sFALSE;
#if sRENDERER==sRENDER_DX9 || sRENDERER==sRENDER_DX11
      DXMayRestore = 1;
      if(sGetApp())
        app_fullpaint = sGetApp()->OnPaint();
      DXMayRestore = 0;
#endif

      if(!app_fullpaint && sGetApp())
        sGetApp()->OnPrepareFrame();

#if sRENDERER==sRENDER_DX11   // mixed GDI & DX11. complicated.
      if((sSystemFlags & sISF_2D) && (sSystemFlags & sISF_3D) && !sExitFlag && !app_fullpaint && sAppPtr)
      {
        PAINTSTRUCT ps;
        BeginPaint(win,&ps);
        sRect update(ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom);
        sRect client;
        GetClientRect(win,(RECT *) &client);
        EndPaint(win,&ps);

        DXMayRestore = 1;
        if(sRender3DBegin())
        {
          if(!client.IsEmpty())
          {
            sRender2DBegin();
            sAppPtr->OnPaint2D(client,update);
            sRender2DEnd();
          }
          if(sGetApp())
            sGetApp()->OnPaint3D();
          
          sRender3DEnd();
        }
        DXMayRestore = 0;
        sCollector();
      }
      else
#endif
      {
        if((sSystemFlags & sISF_2D) && !sExitFlag)  // sExitFlag is checked in case of windows crash during destruction
        {
          PAINTSTRUCT ps;

          BeginPaint(win,&ps);
          sGDIDC = ps.hdc;
          sRect update(ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom);
          sRect client;
          GetClientRect(win,(RECT *) &client);
          if(sAppPtr && !sFatalFlag)
            sAppPtr->OnPaint2D(client,update);
          EndPaint(win,&ps);
          sGDIDC = 0;
    //      sPingSound();
          sCollector();
        }
        else
        {
          ValidateRect(win,0);
        }
        if(!app_fullpaint && (sSystemFlags & sISF_3D))
        {
  #if sRENDERER==sRENDER_DX9 || sRENDERER==sRENDER_DX11
          DXMayRestore = 1;
          Render3D();
          DXMayRestore = 0;
  #else
          Render3D();
  #endif
        }
      }
      sPingSound();
#if !sCONFIG_OPTION_NPP
      if((sSystemFlags & sISF_CONTINUOUS) && !sWin32::ModalDialogActive || sExternalWindow)
      {
        InvalidateRect(win,0,0);
      }
#endif

      sInPaint=0;
    }
    break;

  case WM_TIMER:
    if(!sExitFlag && !sFatalFlag)
    {
#if sCONFIG_OPTION_NPP
      if (wparam==0x1234)
      {
        InvalidateRect(win,0,0);
        break;
      }
#endif
      sPingSound();
      sAppPtr->OnEvent(sAE_TIMER);
      if(TimerEventTime)
        SetTimer(win,1,TimerEventTime,0);
    }
    break;

  case WM_COMMAND:
    if((wparam & 0x8000) && !sFatalFlag)
      sAppPtr->OnEvent(wparam&0x7fff);
    break;

  case WM_DEVICECHANGE:
    if(wparam==DBT_DEVNODES_CHANGED)
    {
      sNewDeviceHook->Call();
    }
    break;

  case WM_ACTIVATE:
    sInput2ClearQueue();
    if(sSystemFlags & sISF_FULLSCREEN)
      sActivateHook->Call((wparam&0xffff)!=WA_INACTIVE);
    else
      sActivateHook->Call(!(wparam&0xffff0000));
    return DefWindowProc(win,msg,wparam,lparam);

  case WM_SETCURSOR:
    if(sSystemFlags & sISF_FULLSCREEN)
      SetCursor(0);
    else
      return DefWindowProc(win,msg,wparam,lparam);
    break;

  case WM_SIZE:
    if(wparam == SIZE_MINIMIZED && sActivateHook)
    {
      // We only need to send a deactivate event,
      // because activation is handled in WM_ACTIVATE.
      sActivateHook->Call(sFALSE);
    }

    if(!(sSystemFlags&sISF_FULLSCREEN))
    {
      sInt x = sInt(lparam&0xffff);
      sInt y = sInt(lparam>>16);
      ResizeGFX(x,y);
    }
    InvalidateRect(win,0,0);
    break;

// input messages

  case WM_INPUT: // complete mouse handling, does not work before windows xp
    if(GetRawInputDataPtr)
    {
      InputThreadLock->Lock();
      RAWINPUT mouse[2];      // just some data, 
      UINT size,bytes;
      bytes = GetRawInputDataPtr((HRAWINPUT)lparam,RID_INPUT,0,&size,sizeof(RAWINPUTHEADER));
      if(size>0 && size<=sizeof(mouse))
      {
        bytes = GetRawInputDataPtr((HRAWINPUT)lparam,RID_INPUT,(void *) mouse,&size,sizeof(RAWINPUTHEADER));
        if(bytes>0 && size<=sizeof(mouse))
        {
          RAWMOUSE *rm = &mouse[0].data.mouse;

/*          if (rm->usFlags & MOUSE_MOVE_ABSOLUTE)
          {
            // this device does not deliver relative but absolute coordinates!
            //   (happens in VMWare virtual machines)

            // MSDN: Special Requirements for Absolute Pointing Devices
            //   In addition to dividing the device input value by the maximum 
            //   capability of the device, the driver scales the device input value by 0xFFFF:
            //     LastX = ((device input x value) * 0xFFFF ) / (Maximum x capability of the device)
            //     LastY = ((device input y value) * 0xFFFF ) / (Maximum y capability of the device)
            sInt maxCapX = GetSystemMetrics(SM_CXVIRTUALSCREEN);
            sInt maxCapY = GetSystemMetrics(SM_CYVIRTUALSCREEN);
            sInt mx = (rm->lLastX * maxCapX) / 0xffff;
            sInt my = (rm->lLastY * maxCapY) / 0xffff;

            Mouse[0].RawX += mx-Mouse[0].X;
            Mouse[0].RawY += my-Mouse[0].Y;
            Mouse[0].X = mx;
            Mouse[0].Y = my;
            sInput2SendEvent(sInput2Event(sKEY_MOUSEHARD, 0, (sU16(sS16(mx)) << 16) | sU16(sS16(my))));
          }
          else*/
          {
            Mouse[0].RawX += rm->lLastX;
            Mouse[0].RawY += rm->lLastY;
            sInput2SendEvent(sInput2Event(sKEY_MOUSEHARD, 0, (sU16(sS16(rm->lLastX)) << 16) | sU16(sS16(rm->lLastY))));
          }
        }
      }
      InputThreadLock->Unlock();
    }
    return 0;

  case WM_KEYUP:
  case WM_SYSKEYUP:
    i = sInt(lparam & 0x01000000);
    if(wparam==VK_SHIFT && !i)    sKeyQual &= ~sKEYQ_SHIFTL;
    if(wparam==VK_SHIFT &&  i)    sKeyQual &= ~sKEYQ_SHIFTR;
    if(wparam==VK_CONTROL && !i)  sKeyQual &= ~sKEYQ_CTRLL;
    if(wparam==VK_CONTROL &&  i)  sKeyQual &= ~sKEYQ_CTRLR;
    if(wparam==VK_MENU && !i)     sKeyQual &= ~sKEYQ_ALT;
    if(wparam==VK_MENU &&  i)     sKeyQual &= ~sKEYQ_ALTGR;

    t=0;
    if((sKeyQual&sKEYQ_SHIFT) || (sKeyQual&sKEYQ_CAPS)) t = 1;
    if(sKeyQual&sKEYQ_ALTGR) { t = 2; sKeyQual&=~sKEYQ_CTRLL; }
    i = sKeyTable[t][wparam&255];
    if(i==0) i = sKeyTable[0][wparam&255];
    if(i)
    {
      i |= sKEYQ_BREAK;
      sInput2SendEvent(sInput2Event(i | sKeyQual));
    }
    InputThreadLock->Lock();
    Keyboard->keys[(lparam >> 16) & 0xff] = sFALSE;
    InputThreadLock->Unlock();
    break;

  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
    i = sInt(lparam & 0x01000000);

    if(wparam==VK_SHIFT && !i)    sKeyQual |= sKEYQ_SHIFTL;
    if(wparam==VK_SHIFT &&  i)    sKeyQual |= sKEYQ_SHIFTR;
    if(wparam==VK_CONTROL && !i)  sKeyQual |= sKEYQ_CTRLL;
    if(wparam==VK_CONTROL &&  i)  sKeyQual |= sKEYQ_CTRLR;
    if(wparam==VK_MENU && !i)     sKeyQual |= sKEYQ_ALT;
    if(wparam==VK_MENU &&  i)     sKeyQual |= sKEYQ_ALTGR;

    {
      sInt repeat = (lparam & 0x40000000)?sInt(sKEYQ_REPEAT) : 0;
      if(wparam==VK_CAPITAL && !i && !repeat)  sKeyQual ^= sKEYQ_CAPS;

      t=0;
      if((sKeyQual&sKEYQ_SHIFT) || (sKeyQual&sKEYQ_CAPS)) t = 1;
      if(sKeyQual&sKEYQ_ALTGR) { t = 2; sKeyQual&=~sKEYQ_CTRLL; }
      i = sKeyTable[t][wparam];
      if(i==0) i = sKeyTable[0][wparam];
      if(i)
        sInput2SendEvent(sInput2Event(i | sKeyQual | repeat));
      InputThreadLock->Lock();
      Keyboard->keys[(lparam >> 16) & 0xff] = sTRUE;
      InputThreadLock->Unlock();
    }
    break;

  case WM_MOUSEMOVE:
    Mouse[0].X = sS16(lparam&0xffff);
    Mouse[0].Y = sS16(lparam>>16);
    sInput2SendEvent(sInput2Event(sKEY_MOUSEMOVE, 0, (Mouse[0].X << 16) | Mouse[0].Y));
    break;
  case WM_MOUSEWHEEL:
    sMouseWheelAkku += sS16((wparam>>16)&0xffff);
    while(sMouseWheelAkku>=120)
    {
      Mouse[0].Z += 1.0f;
      sMouseWheelAkku-=120;
      sInput2SendEvent(sInput2Event(sKEY_WHEELUP));
    }
    while(sMouseWheelAkku<=-120)
    {
      Mouse[0].Z -= 1.0f;
      sMouseWheelAkku+=120;
      sInput2SendEvent(sInput2Event(sKEY_WHEELDOWN));
    }
    break;

  case WM_LBUTTONDOWN:
    Mouse[0].Buttons |= sMouseData::sMB_LEFT;
    sInput2SendEvent(sInput2Event(sKEY_LMB));
    SetCapture(win);
    break;
  case WM_RBUTTONDOWN:
    Mouse[0].Buttons |= sMouseData::sMB_RIGHT;
    sInput2SendEvent(sInput2Event(sKEY_RMB));
    SetCapture(win);
    break;
  case WM_MBUTTONDOWN:
    Mouse[0].Buttons |= sMouseData::sMB_MIDDLE;
    sInput2SendEvent(sInput2Event(sKEY_MMB));
    SetCapture(win);
    break;
  case WM_XBUTTONDOWN:
    SetCapture(win);
    switch(HIWORD(wparam))
    {
    case XBUTTON1: 
      Mouse[0].Buttons |= sMouseData::sMB_X1;
      sInput2SendEvent(sInput2Event(sKEY_X1MB));
      break;
    case XBUTTON2: 
      Mouse[0].Buttons |= sMouseData::sMB_X2;
      sInput2SendEvent(sInput2Event(sKEY_X2MB));
      break;
    }
    break;

  case WM_LBUTTONDBLCLK:
    Mouse[0].Buttons |= sMouseData::sMB_LEFT;
    sInput2SendEvent(sInput2Event(sKEY_LMB|sKEYQ_REPEAT));
    SetCapture(win);
    break;
  case WM_RBUTTONDBLCLK:
    Mouse[0].Buttons |= sMouseData::sMB_RIGHT;
    sInput2SendEvent(sInput2Event(sKEY_RMB|sKEYQ_REPEAT));
    SetCapture(win);
    break;
  case WM_MBUTTONDBLCLK:
    Mouse[0].Buttons |= sMouseData::sMB_MIDDLE;
    sInput2SendEvent(sInput2Event(sKEY_MMB|sKEYQ_REPEAT));
    SetCapture(win);
    break;
  case WM_XBUTTONDBLCLK:
    SetCapture(win);
    switch(HIWORD(wparam))
    {
    case XBUTTON1: 
      Mouse[0].Buttons |= sMouseData::sMB_X1;
      sInput2SendEvent(sInput2Event(sKEY_X1MB|sKEYQ_REPEAT));
      break;
    case XBUTTON2: 
      Mouse[0].Buttons |= sMouseData::sMB_X2;
      sInput2SendEvent(sInput2Event(sKEY_X2MB|sKEYQ_REPEAT));
      break;
    }
    break;


  case WM_LBUTTONUP:
    Mouse[0].Buttons &= ~sMouseData::sMB_LEFT;
    sInput2SendEvent(sInput2Event(sKEY_LMB | sKEYQ_BREAK));
    if(Mouse[0].Buttons==0) ReleaseCapture();
    break;
  case WM_RBUTTONUP:
    Mouse[0].Buttons &= ~sMouseData::sMB_RIGHT;
    sInput2SendEvent(sInput2Event(sKEY_RMB | sKEYQ_BREAK));
    if(Mouse[0].Buttons==0) ReleaseCapture();
    break;
  case WM_MBUTTONUP:
    Mouse[0].Buttons &= ~sMouseData::sMB_MIDDLE;
    sInput2SendEvent(sInput2Event(sKEY_MMB | sKEYQ_BREAK));
    if(Mouse[0].Buttons==0) ReleaseCapture();
    break;
  case WM_XBUTTONUP:
    if(Mouse[0].Buttons==0) ReleaseCapture();
    switch(HIWORD(wparam))
    {
    case XBUTTON1: 
      Mouse[0].Buttons &= ~sMouseData::sMB_X1;
      sInput2SendEvent(sInput2Event(sKEY_X1MB | sKEYQ_BREAK));
      break;
    case XBUTTON2: 
      Mouse[0].Buttons &= ~sMouseData::sMB_X2;
      sInput2SendEvent(sInput2Event(sKEY_X2MB | sKEYQ_BREAK));
      break;
    }
    break;
  case WM_DROPFILES:
    {
      extern void sSetDragDropFile(const sChar *name);

      HDROP drop = (HDROP) wparam;
      sChar buffer[2048];
      DragQueryFileW(drop,0,buffer,sCOUNTOF(buffer));
      DragFinish(drop);
      sSetDragDropFile(buffer);
      sInput2SendEvent(sInput2Event(sKEY_DROPFILES));
    }
    return 0;
// default processing

  default:
    return DefWindowProc(win,msg,wparam,lparam);
  }

#if sCRASHDUMP
  }
  __except(GenerateDump(GetExceptionInformation()))
  {
    ExitProcess(1);
  }
#endif

  }
  __except(MakeFatal(exceptInfo = GetExceptionInformation()))
  {
    sChar msg[1024],trace[1024];
    const sChar *traceMsg = 0;

    if(sStackTraceFromContext(sStringDesc(trace,sCOUNTOF(trace)),0,16,*exceptInfo->ContextRecord))
      traceMsg = trace;

    sSPrintF(sStringDesc(msg,sCOUNTOF(msg)),
      L"Exception occured in Windows Message Loop.\nCode = %08x, Address = %016x.",
      sU32(exceptInfo->ExceptionRecord->ExceptionCode),sPtr(exceptInfo->ExceptionRecord->ExceptionAddress));

    sFatalImplTrace(msg,traceMsg);
  }

  return 1;
}

/****************************************************************************/

//
// external access start
//

// init/exit is called several times, so we don't wanna adding up the test leaks
#if sCONFIG_COMPILER_MSC
static sU8* ExternTestLeak = 0;
#endif

void sExternMainInit(void *p_instance, void *p_hwnd)
{
  sDPrint(L"sExternMainInit\n");
  WInstance = *(HINSTANCE*)p_instance;
  sExternalWindow = *(HWND*)p_hwnd;
//  sGDIDC = GetDC(sExternalWindow);        // use external window for gui
  
  // check offscreensurface   it may not be active because XSI eats up the wm_create-call
  if (!sGDIDCOffscreen)
  {
    HDC hdc;
    hdc = GetDC(sExternalWindow);    // use external window if available
    sGDIDCOffscreen = CreateCompatibleDC(hdc);
    ReleaseDC(sExternalWindow, hdc);
  }

  sVERIFY(sizeof(sChar)==2);

  // initialize
  CoInitialize(NULL);

  WConOut = GetStdHandle(STD_OUTPUT_HANDLE);
  WConIn  = GetStdHandle(STD_INPUT_HANDLE);
  sInitMem0();
  sFrameHook = new sHooks;
  sNewDeviceHook = new sHooks;
  sActivateHook = new sHooks1<sBool>;
  sAltF4Hook = new sHooks1<sBool &>;
  sCrashHook = new sHooks1<sStringDesc>;
  sCheckCapsHook = new sHooks;
#if !sSTRIPPED
  sInputHook = new sHooks2<const sInput2Event &,sBool &>;
  sDebugOutHook = new sHooks1<const sChar*>;
#endif

  sInitKeys();

  sSetRunlevel(0x80);

  // don't initialize joypad for xsi!
#if sCONFIG_OPTION_XSI
  SetXSIModeD3D(sTRUE);
#endif
  sMain();
}

void sExternMainExit()
{
  sDelete(sAppPtr);

  if (sHWND)
  {
    MsgProc(sHWND,WM_DESTROY,0,0);
    if(sHWND) UnregisterClassW(L"EXTERN_ALTONA",WInstance);
  }

//  for(sInt i=0;i<PackfileCount;i++)
//    sDelete(Packfiles[i]);
  
  CoUninitialize();

  // don't exit joypad for xsi!

  // check memory usage

  sSetRunlevel(0x00);
  sDelete(sFrameHook);
  sDelete(sCheckCapsHook);
  sDelete(sNewDeviceHook);

  sHWND = sNULL; 

  sExitMem0();
}

struct sMessageLoopData
{
  HWND hWnd; 
  UINT message;
  WPARAM wParam;
  LPARAM lParam;
  LRESULT result;
};

int sExternMessageLoopGetSize()
{
  return sizeof(sMessageLoopData);
}

void sExternMessageLoop(void *p_messageLoopData)
{
  /*sInt message = ((sMessageLoopData*)p_messageLoopData)->message;
  if (message!=WM_TIMER && message!=WM_PAINT)
  {
    // sDPrintF crashes if threading is not initialized, so better use sSPrintF and sDPrint
    sString<64> buffer;
    sSPrintF(buffer,L"%d: %d\n",sGetTime(),message);
    sDPrint(buffer);
  }*/

  if (sHWND)
  {
    sMessageLoopData *mdata = (sMessageLoopData*)p_messageLoopData;

    mdata->result = MsgProc(mdata->hWnd,mdata->message,mdata->wParam,mdata->lParam);
  }
}

//
// external access end
//


void sInit(sInt flags,sInt xs,sInt ys)
{
  if(flags & sISF_3D)
    PreInitGFX(flags,xs,ys);

  const sChar *caption = sGetWindowName();

  if (flags&sISF_NODWM)
  {
    HMODULE dwmapi=LoadLibrary(L"dwmapi.dll");
    if (dwmapi)
    {
      typedef HRESULT (WINAPI *DECtype)(UINT);
      DECtype DwmEnableComposition=(DECtype)GetProcAddress(dwmapi,"DwmEnableComposition");
      if (DwmEnableComposition) DwmEnableComposition(FALSE);
      FreeLibrary(dwmapi);
    }
  }

  // is external window present?
  //if((flags & sISF_2D) || sCOMMANDLINE==0)    // don't create window if we don't need one!
  {                                             // some commandline tools (like converteng) need d3d and therefore a window
    if (sExternalWindow)
    {
      sHWND = sExternalWindow;
      RECT rect;
      GetWindowRect(sHWND,&rect);
      xs = sAbs(rect.right-rect.left);
      ys = sAbs(rect.top-rect.bottom);
    }
    else
    {
      // window needs to be created
      WNDCLASSEXW wc;

      // Register the window class

      sClear(wc);
      wc.cbSize = sizeof(WNDCLASSEX);
      wc.style = CS_OWNDC|CS_DBLCLKS ;
      wc.lpfnWndProc = MsgProc;
      wc.hInstance = WInstance;
      wc.lpszClassName = L"ALTONA";
      wc.hCursor = 0;
      wc.hIcon = LoadIcon(WInstance,MAKEINTRESOURCE(100));
      RegisterClassExW(&wc);

      // Create the application's window

      if(flags&sISF_FULLSCREEN)
      {
        sHWND = CreateWindowW(L"ALTONA",caption,
          WS_POPUP,
          CW_USEDEFAULT,CW_USEDEFAULT,xs,ys,
          0,0,wc.hInstance,0);
      }
      else
      {
        RECT r2;
        r2.left = r2.top = 0;
        r2.right = xs; 
        r2.bottom = ys;
        AdjustWindowRect(&r2,WS_OVERLAPPEDWINDOW,FALSE);

        sHWND = CreateWindowW(L"ALTONA",caption,
          WS_OVERLAPPEDWINDOW,
          CW_USEDEFAULT,CW_USEDEFAULT,r2.right-r2.left,r2.bottom-r2.top,
          0,0,wc.hInstance,0);
      }
      if(sCOMMANDLINE==0)                   // don't show window for commandline tools
        ShowWindow(sHWND,SW_SHOWDEFAULT);
    }
  }

  sSystemFlags = flags;
  if(flags & sISF_3D)
    InitGFX(flags,xs,ys);
  else
    ResizeGFX(xs,ys);

  // register raw input, XSI builds use direct input!

#if !sCONFIG_OPTION_XSI
  HMODULE module=GetModuleHandle(L"USER32.dll");
  if(module)
  {
    RegisterRawInputDevicesPtr = (RegisterRawInputDevicesProc)GetProcAddress(module,"RegisterRawInputDevices");
    GetRawInputDataPtr =(GetRawInputDataProc)        GetProcAddress(module,"GetRawInputData");

    if(RegisterRawInputDevicesPtr)
    {
      RAWINPUTDEVICE rid;
      rid.usUsagePage = 1;
      rid.usUsage = 2;
      rid.dwFlags = 0;
      rid.hwndTarget = 0;
//#include <winuser.h>

      BOOL regres = RegisterRawInputDevicesPtr(&rid,1,sizeof(rid));    // may fail
      sLogF(L"dev",L"register raw input: %d\n",regres);
    } else
    { 
      sLogF(L"dev",L"RegisterRawInputDevices not found in USER32.dll.(windows 2000 maybe?)");
    }
  }
#endif

  // run the rest
  sTrialVersion = sGetShellSwitch(L"trialversion") || sGetShellSwitch(L"trial");
  sSetRunlevel(0x100);
  InvalidateRect(sHWND,0,FALSE);
}



int main()
{
  return WinMain(GetModuleHandle(0),0,GetCommandLineA(),0);
} 

const sChar *sGetCommandLine()
{
  return GetCommandLineW();
}


INT WINAPI WinMain(HINSTANCE inst,HINSTANCE,LPSTR,INT)
{
#if sCRASHDUMP
  #pragma warning(push)
  #pragma warning(disable : 4509)   // we don't care if dtors are not called when an exception happes
  __try
  {
#endif

  MSG msg;

  WInstance = inst;

  sVERIFY(sizeof(sChar)==2);
  sInitEmergencyThread();

  // initialize

  WConOut = GetStdHandle(STD_OUTPUT_HANDLE);
  WConIn  = GetStdHandle(STD_INPUT_HANDLE);
  
  sParseCmdLine(sGetCommandLine());
restart:
  sInitMem0();

  CoInitialize(NULL);
  sFrameHook = new sHooks;
  sNewDeviceHook = new sHooks;
  sActivateHook = new sHooks1<sBool>;
  sCrashHook = new sHooks1<sStringDesc>;
  sAltF4Hook = new sHooks1<sBool &>;
  sCheckCapsHook = new sHooks;
#if !sSTRIPPED
  sInputHook = new sHooks2<const sInput2Event &,sBool &>;
  sDebugOutHook = new sHooks1<const sChar*>;
#endif

  sInitKeys();
  sSetRunlevel(0x80);

  sMain();

  // if window, do the message loop

  if(sAppPtr)
  {
    if(sHWND)
    {

      // Show the window

      UpdateWindow(sHWND);

      // message loop

      GetMessage(&msg,0,0,0);
      while(msg.message!=WM_QUIT)
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if(sExitFlag)
        {
          // this gives a chance to shut down system resources
          // important for async threads like sound
          // gui uses it to delete all windows.

          if(sAppPtr)
            sAppPtr->OnEvent(sAE_EXIT);
          sCollect();
          sCollector(sTRUE);

          // now it's really going down.

          DestroyWindow(sHWND);
          sHWND=0;
//          sExitFlag = 0;
          break;
        }

        GetMessage(&msg,0,0,0);
      }
    }

    // done;

    sDelete(sAppPtr);
  }
  else  // do some destruction that is normally done in WM_DESTROY
  {
    sGDIDCOffscreen = 0;
    sCollect();
    sCollector(sTRUE);
    sSetRunlevel(0x80);

    if(sSystemFlags & sISF_3D)
      ExitGFX();
  }

  // remaining exit code

//  for(sInt i=0;i<PackfileCount;i++)
//    sDelete(Packfiles[i]);

  if(sHWND) UnregisterClassW(L"fcoe",inst);
  CoUninitialize();

  // some shell interaction before finish

  if(sCONFIG_OPTION_SHELL && !sRELEASE)
  {
    sString<64> buffer;
    DWORD dummy;
    sPrint(L"<<< press enter >>>\n");

    ReadConsoleW(WConIn,(sChar *)buffer,64,&dummy,0);
  }

  // check memory usage

  sSetRunlevel(0x00);
  sDelete(sFrameHook);
  sDelete(sNewDeviceHook);
  sDelete(sActivateHook);
  sDelete(sCrashHook);
  sDelete(sAltF4Hook);
  sDelete(sCheckCapsHook);
#if !sSTRIPPED
  sDelete(sDebugOutHook);
  sDelete(sInputHook);
#endif
  sExitMem0();

  if(sExitFlag==2)
  {
    sExitFlag = 0;
    goto restart;
  }

#if sCRASHDUMP
  }
  __except(GenerateDump(GetExceptionInformation()))
  {
    ExitProcess(1);
  }
  #pragma warning(pop)
#endif

  sLogF(L"sys",L"error code %08x (%s)\n",WError,WError ? L"fail" : L"ok");

  return WError;
}

/****************************************************************************/

sInt sGetTime()
{
  return timeGetTime();
}

sU64 sGetTimeUS()
{
  LARGE_INTEGER pc, pf;
  QueryPerformanceCounter(&pc);
  QueryPerformanceFrequency(&pf);
  return pc.QuadPart*1000000UL/pf.QuadPart;
}

sDateAndTime sGetDateAndTime()
{
  SYSTEMTIME time;
  GetLocalTime(&time);
  return TimeFromSystemTime(time);
}

sDateAndTime sAddLocalTime(sDateAndTime origin,sInt seconds)
{
  SYSTEMTIME system;
  union
  {
    FILETIME fileLocal;
    ULARGE_INTEGER intVal;
  } u;

  TimeToSystemTime(system,origin);
  SystemTimeToFileTime(&system,&u.fileLocal);

  u.intVal.QuadPart += seconds * 10000000ll;

  FileTimeToSystemTime(&u.fileLocal,&system);
  return TimeFromSystemTime(system);
}

sS64 sDiffLocalTime(sDateAndTime a,sDateAndTime b)
{
  SYSTEMTIME sa,sb;
  union
  {
    FILETIME file;
    ULARGE_INTEGER intVal;
  } fa,fb;

  TimeToSystemTime(sa,a);
  SystemTimeToFileTime(&sa,&fa.file);

  TimeToSystemTime(sb,b);
  SystemTimeToFileTime(&sb,&fb.file);

  return sS64(fa.intVal.QuadPart - fb.intVal.QuadPart) / 10000000ll;
}

sU8 sGetFirstDayOfWeek()
{
  WCHAR buffer[4];
  sInt  firstDayOfWeek = 1; // set default first day of week to monday

  // The actual result is stored in buffer[0] and is in range 0..6, where 6 equals sunday
  if(GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK, buffer, sCOUNTOF(buffer)) > 0)
  {
    sInt firstDay = ((buffer[0] - L'0') + 1) % 7; // Convert result, so Sunday=0, Monday=1, ...
    if(firstDay >= 0 && firstDay <= 6) // Make sure it make sense, otherwise use default
      firstDayOfWeek = firstDay;
  }

  return firstDayOfWeek;
}

void sSysLogInit(const sChar *appname)
{
}

void sSysLog(const sChar *module,const sChar *text)
{
  sLog(module,text);
}

sBool sDaemonize()
{
  return sFALSE;
}

sInt sGetRandomSeed()
{
  SYSTEMTIME time;
  sU32 val;
  static sRandom rnd;

#define Step(val,in) { val=(val<<7)|(val>>(32-7)); val ^= in^(in<<16)^(in>>16); }

  GetSystemTime(&time);
  val=timeGetTime();

  Step(val,sInt(time.wYear));
  Step(val,sInt(time.wYear));
  Step(val,sInt(time.wMonth));  
  Step(val,sInt(time.wDayOfWeek)); 
  Step(val,sInt(time.wDay)); 
  Step(val,sInt(time.wHour));  
  Step(val,sInt(time.wMinute));  
  Step(val,sInt(time.wSecond));  
  Step(val,sInt(time.wMilliseconds));

  return val^rnd.Int32();
}

sBool sGetEnvironmentVariable(const sStringDesc &dst,const sChar *var)
{
  DWORD result = GetEnvironmentVariable(var,dst.Buffer,dst.Size);
  
  if(result && result<=sU32(dst.Size))
    return sTRUE;
  sVERIFY(!result && GetLastError()==ERROR_ENVVAR_NOT_FOUND);
  return sFALSE;
}

sBool sExecuteOpen(const sChar *file)
{
  return sDInt(ShellExecuteW(0,L"open",file,0,L"",SW_SHOW))>=32;
}


sBool sExecuteShell(const sChar *cmdline)
{
  sString<2048> buffer;
  buffer = cmdline;
  sInt result = 0;
  DWORD code = 1;
  PROCESS_INFORMATION pi;
  STARTUPINFOW si;
  
  sClear(pi);
  sClear(si);
  si.cb = sizeof(si);
  if(!CreateProcessW(0,buffer,0,0,1,0,0,0,&si,&pi))
    return 0;
  
  WaitForSingleObject(pi.hProcess,INFINITE);
  if(GetExitCodeProcess(pi.hProcess,&code))
  {
    if(code==0)
      result = 1;
  }
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return result;
}


sBool sExecuteShellDetached(const sChar *cmdline)
{
  sString<2048> buffer;
  buffer = cmdline;
  PROCESS_INFORMATION pi;
  STARTUPINFOW si;
  
  sClear(pi);
  sClear(si);
  si.cb = sizeof(si);
  if(!CreateProcessW(0,buffer,0,0,0,CREATE_NEW_PROCESS_GROUP,0,0,&si,&pi))
    return 0;
  
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return 1;
}


// warning!
// this is exactly as the sample implementation from microsoft
// "Creating a Child Process with Redirected Input and Output"
// the slightest change will break the code in mysterious ways!

sBool sExecuteShell(const sChar *cmdline,sTextBuffer *tb)
{
  sString<2048> buffer;
  //buffer = cmdline;
  buffer = L"cmd.exe /C ";
  buffer.Add(cmdline);

  sInt result = 0;
  DWORD code = 1;
  PROCESS_INFORMATION pi;
  STARTUPINFOW si;
  HANDLE readpipe;
  HANDLE writepipe;
  DWORD bytesread;
  SECURITY_ATTRIBUTES sec; 

  // prepare the pipe for stdout
  // read: not inhereted
  // write: ihrereted

  sClear(sec);
  sec.nLength = sizeof(SECURITY_ATTRIBUTES); 
  sec.bInheritHandle = TRUE; 
   
  HANDLE readpipe_1;
  if(!CreatePipe(&readpipe_1,&writepipe,&sec,0))
    sVERIFYFALSE;
  if(!DuplicateHandle(GetCurrentProcess(),readpipe_1,GetCurrentProcess(),&readpipe,0,0,DUPLICATE_SAME_ACCESS))
    sVERIFYFALSE;
  CloseHandle(readpipe_1);

  // create process with redirected pipe

  sClear(pi);
  sClear(si);
  si.cb = sizeof(si);

  si.hStdOutput = writepipe;
  si.hStdError = GetStdHandle(STD_ERROR_HANDLE); 
  si.hStdInput = GetStdHandle(STD_INPUT_HANDLE); 
  si.dwFlags = STARTF_USESTDHANDLES;

  sString<2048> env;
  GetEnvironmentVariableW(L"PATH",&env[0],env.Size());

  // [kb 09-09-07] added CREATE_NO_WINDOW flag to suppress the empty console window
  if(CreateProcessW(0,buffer,0,0,1,CREATE_NO_WINDOW,0,0,&si,&pi))
  {
    // close write port. the child process has the last handle, and
    // that will be closed soon. 
    // this allows read to terminate when no more input can come.

    if(!CloseHandle(writepipe))
      sVERIFYFALSE;

    // read and convert to unicode in chunks

    const sInt block = 4096;
    sU8 ascii[block];
    sChar uni[block];

    for(;;)
    {
      if(!ReadFile(readpipe,ascii,block,&bytesread,0) || bytesread==0)
        break;
      for(sU32 i=0;i<bytesread;i++)
        uni[i] = ascii[i];
      tb->Print(uni,bytesread);
    }

    CloseHandle(readpipe);

    // terminate process and get termination code

    WaitForSingleObject(pi.hProcess,INFINITE);
    if(GetExitCodeProcess(pi.hProcess,&code))
    {
      if(code==0)
        result = 1;
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
  }
  else  // failed to create process
  {
    CloseHandle(writepipe);
    CloseHandle(readpipe);
    result = 1;
  }

  return result;
}

static volatile sBool sCtrlCFlag;
extern "C" 
{
  BOOL WINAPI sCtrlCHandler(DWORD event)
  {
    if(event==CTRL_C_EVENT)
    {
      sCtrlCFlag = 1;
      return TRUE;
    }
    else
    {
      return FALSE;
    }
  }
}

void sCatchCtrlC(sBool enable)
{
  SetConsoleCtrlHandler(sCtrlCHandler,enable!=0);
}

sBool sGotCtrlC()
{
  return sCtrlCFlag;
}

sBool sCheckBreakKey()
{
  sU32 n = GetAsyncKeyState(VK_PAUSE);
  return (n!=0) ? 1 : 0;
}

void sConsoleWindowClear()
{
  HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
  COORD dwCursorPosition;
  dwCursorPosition.X = 0;
  dwCursorPosition.Y = 0;
  SetConsoleCursorPosition(hConsoleOutput,dwCursorPosition);
}



void sSetErrorCode(sInt i)
{
  WError = i;
}

sU32 sGetKeyQualifier()
{
  return sKeyQual;
}

sInt sMakeUnshiftedKey(sInt ascii)
{
  ascii &= sKEYQ_MASK;
  if(ascii>0x20)
  {
    sInt scan = VkKeyScan(ascii)&0xff;
    if(scan>=0x30 && scan<=0x39)
      return scan-0x30+'0';
    if(scan>=0x41 && scan<=0x5a)
      return scan-0x41+'a';
  }
  return ascii;
}

/****************************************************************************/

void sSetTimerEvent(sInt time,sBool loop)
{
  SetTimer(sHWND,1,time,0);
  if(loop)
    TimerEventTime = time;
  else
    TimerEventTime = 0;
}

void sTriggerEvent(sInt event)
{
  sVERIFY(event>=0 && event<0x8000);
  PostMessage(sHWND,WM_COMMAND,event|0x8000,0);
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Direct Input / Joypads                                             ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

struct sJoypadData
{
  sInt Connected;                 // 0: disconnected, 1: connected
  sInt ButtonMask;                // which buttons are available? usually continuous
  sInt AnalogMask;                // which axes are available? usually NOT continous
  sInt PovMask;                   // which POV's are available? usually the first
  sU32 Buttons;                   // all buttons states
  sU32 Povs;                      // 8 pov states @ 4 bits
  sU16 Analog[16];                // 16 analog channel states, 0 .. 0xffff
  sU8 Pressure[32];               // pressure of the buttons
  sU8 JoypadType;                   // one of sJOYPAD_* in enum sJoypadType
};

void sPollJoypads(void *user)
{
//  sDIJoypad *pad = sNULL;
//TODO:    pad->Poll();
}

static void sInitJoypadManager()
{
  sFrameHook->Add(sPollJoypads);
}

static void sExitJoypadManager()
{
  sFrameHook->Rem(sPollJoypads);
/*  while(!Joypads->IsEmpty())
    delete Joypads->RemHead();
  delete Joypads;*/
}

/****************************************************************************/
/***                                                                      ***/
/***   Direct Input                                                       ***/
/***                                                                      ***/
/****************************************************************************/

static IDirectInput8W *DID;

void sInitDInput()
{
  DirectInput8Create(WInstance,DIRECTINPUT_VERSION,IID_IDirectInput8W,(void **)&DID,0);
}

void sExitDInput()
{
  sRelease(DID);
}

/****************************************************************************/
/***                                                                      ***/
/***   Direct Input Joypads                                               ***/
/***                                                                      ***/
/****************************************************************************/

class sDIJoypad
{
  friend BOOL CALLBACK EnumJoystickObjectCallback(const DIDEVICEOBJECTINSTANCEW *doi,void *obj);

  IDirectInputDevice8W *DIDev;
  sString<128> Name;
  GUID Guid;
  DWORD LastTimestamp;
  void Event(sInt ofs,sInt value,sInt timestamp);
public:
  sDIJoypad(IDirectInputDevice8W *,const sChar *name,GUID guid);
  ~sDIJoypad();
  void GetName(const sStringDesc &name); 
  void SetMotor(sInt slow,sInt fast);

  void Poll();
};

/****************************************************************************/

static BOOL CALLBACK EnumJoystickObjectCallback(const DIDEVICEOBJECTINSTANCEW *doi,void *obj)
{
  sDIJoypad *dev = (sDIJoypad *)obj;
  sInt type = doi->dwType & 0xff0000ff;
  sInt num = (doi->dwType & 0x00ffff00)>>8;

//  sDPrintF(L"  %04x %08x %04x  <%s>\n",sInt(doi->dwOfs),sInt(doi->dwType),sInt(doi->dwFlags),doi->tszName);

  if (type & DIDFT_ABSAXIS)
  {
    if(num>=0 && num<16)
    {
      //dev->State.AnalogMask |= (1<<num);

      DIPROPRANGE diprg; 
      diprg.diph.dwSize       = sizeof(DIPROPRANGE); 
      diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
      diprg.diph.dwHow        = DIPH_BYID; 
      diprg.diph.dwObj        = doi->dwType; // Specify the enumerated axis
      diprg.lMin              = 0; 
      diprg.lMax              = 0x10000; 
    
      DIErr(dev->DIDev->SetProperty(DIPROP_RANGE,&diprg.diph));
    }
  }
  else if (type & DIDFT_PSHBUTTON)
  {
    //if(num>=0 && num<32)
    //  dev->State.ButtonMask |= (1<<num);
  }
  else if (type & DIDFT_POV)
  {
    //if(num>=0 && num<8)
    //  dev->State.PovMask |= (1<<num);
  }

  return DIENUM_CONTINUE;
}

static BOOL CALLBACK EnumInput(LPCDIDEVICEINSTANCEW ddi,LPVOID)
{
  IDirectInputDevice8W *didev;
  DIDEVCAPS caps;

  // do we know this device already?

/*  if(sIdentifyJoypad(ddi->guidInstance))
    return DIENUM_CONTINUE;*/

  // no, it's new!

  DIErr(DID->CreateDevice(ddi->guidInstance,&didev,0));
  caps.dwSize = sizeof(DIDEVCAPS);
  DIErr(didev->GetCapabilities(&caps));

  //sDPrintF(L"dxinput: %08x:%08x|%04x|%04x:",sInt(caps.dwFlags),sInt(ddi->dwDevType),sInt(ddi->wUsage),sInt(ddi->wUsagePage));
//  sDPrintF(L"%3d %3d %3d:<%s> <%s>\n",sInt(caps.dwAxes),sInt(caps.dwButtons),sInt(caps.dwPOVs),(ddi->tszInstanceName),(ddi->tszProductName));


  switch(ddi->dwDevType&0xff)
  {
  case DI8DEVTYPE_JOYSTICK:
  case DI8DEVTYPE_GAMEPAD:
  case DI8DEVTYPE_DRIVING:
  case DI8DEVTYPE_FLIGHT:
  case DI8DEVTYPE_1STPERSON:

    if(XInputPresent && sFindString(ddi->tszProductName,L"XBOX 360 For Windows")>=0)
    {
      sLogF(L"dev",L"input device <%s> rejected because it is handled by XInput\n",ddi->tszProductName);
    }
    else if(sFindString(ddi->tszProductName,L"PLAYSTATION(R)3")>=0)
    {
      sLogF(L"dev",L"input device <%s> rejected because it is handled by libpad_win\n",ddi->tszProductName);
    }
    else
    {
      //sAddJoypad(new sDIJoypad(didev,ddi->tszInstanceName,ddi->guidInstance));
      sResetMemChecksum();
    }
    break;

  default:
    didev->Release();
    break;
  }

  return DIENUM_CONTINUE;
}

/****************************************************************************/

sDIJoypad::sDIJoypad(IDirectInputDevice8W *dev,const sChar *name,GUID guid)
{
  DIPROPDWORD dipdw;

  Name = name;
  DIDev = dev;
  Guid = guid;
  LastTimestamp = 0;
  for (sInt i=0; i<16; i++)
    ;//State.Analog[i]=0x8000;

  DIErr(DIDev->SetDataFormat(&c_dfDIJoystick2));

  HRESULT hr = DIDev->SetCooperativeLevel(sHWND,DISCL_FOREGROUND|DISCL_NONEXCLUSIVE);
  if(!FAILED(hr))
  {
    dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipdw.diph.dwObj        = 0;
    dipdw.diph.dwHow        = DIPH_DEVICE;
    dipdw.dwData            = 256;
    DIErr(DIDev->SetProperty(DIPROP_BUFFERSIZE,&dipdw.diph));

    DIDev->EnumObjects(EnumJoystickObjectCallback,this,DIDFT_ALL);

    DIDev->Acquire();

    //sLogF(L"inp", L"joypad: buttons %08x axes %04x povs %02x name <%s>\n",State.ButtonMask,State.AnalogMask,State.PovMask,Name);
  }
}

sDIJoypad::~sDIJoypad()
{
  sRelease(DIDev);
}

void sDIJoypad::SetMotor(sInt slow,sInt fast)
{
}

void sDIJoypad::Poll()
{
  HRESULT hr;
  DWORD count;
  static DIDEVICEOBJECTDATA DIData[64];

  hr = DIDev->Poll();
  if(hr==DIERR_INPUTLOST || hr==DIERR_NOTACQUIRED)
    DIDev->Acquire();

  count = sCOUNTOF(DIData);
  sBool sendevent = 1;
getmore:
  hr = DIDev->GetDeviceData(sizeof(DIDEVICEOBJECTDATA),DIData,&count,0);
  if(hr==DIERR_INPUTLOST || hr==DIERR_NOTACQUIRED)
  {
    hr = DIDev->Acquire();
    //if(hr==DIERR_UNPLUGGED)
      //State.Connected = 0;
  }
  else if(!FAILED(hr))
  {
    //State.Connected = 1;
    for(sInt j=0;j<(sInt)count;j++)
    {
      Event(DIData[j].dwOfs,DIData[j].dwData,DIData[j].dwTimeStamp);
      sendevent = 1;
      LastTimestamp = DIData[j].dwTimeStamp;
    }
    if(count>0) goto getmore;
  }
}

void sDIJoypad::Event(sInt ofs,sInt value,sInt timestamp)
{
  sInt i;

  if(ofs>=0x00 && ofs<0x20 && (ofs&3)==0)   // analog channel
  {
    i = ofs/4;
    //State.Analog[i] = sClamp(value,0,0xffff);
  }
  else if(ofs>=0x20 && ofs<0x30 && (ofs&3)==0)   // POV knop
  {
    i = (ofs-0x20)/4;
    sInt bits = 0;
    //sInt oldbits = 0;//State.Povs>>(i*4);
    if((value&0xffff)!=0xffff)
    {
      static sInt table[8] = { 1,3,2,6,4,12,8,9 };
      bits = table[(value*8/36000)&7];
    }
    //State.Povs &= ~(15<<(i*4));
    //State.Povs |= bits<<(i*4);
    for(sInt j=0;j<4;j++)
    {
      //if((bits&(1<<j)) && !(oldbits&(1<<j)))
        //sQueueInput(sIED_JOYPAD,GetId(),sPAD_POV|(i*4)|j,timestamp);
      //if(!(bits&(1<<j)) && (oldbits&(1<<j)))
        //sQueueInput(sIED_JOYPAD,GetId(),sPAD_POV|(i*4)|j|sKEYQ_BREAK,timestamp);
    }
  }
  else if(ofs>=0x30 && ofs<0x50)                 // button
  {
    i = ofs-0x30;
    if(value)
    {
      //State.Pressure[i]=0xff;
      //State.Buttons |= (1<<i);
      //sQueueInput(sIED_JOYPAD,GetId(),sPAD_BUTTON|i,timestamp);
    }
    else
    {
      //State.Pressure[i]=0;
      //State.Buttons &= ~(1<<i);
      //sQueueInput(sIED_JOYPAD,GetId(),sPAD_BUTTON|i|sKEYQ_BREAK,timestamp);
    }
  }
}

/****************************************************************************/

static void sScanDIJoypad(void *user=0)
{
  DID->EnumDevices(DI8DEVCLASS_ALL,EnumInput,0,DIEDFL_ALLDEVICES|DIEDFL_ATTACHEDONLY);  
}

static void sInitDIJoypad()
{
  sScanDIJoypad();

  sNewDeviceHook->Add(sScanDIJoypad);
}

static void sExitDIJoypad()
{
  sNewDeviceHook->Rem(sScanDIJoypad);
}


/****************************************************************************/
/****************************************************************************/

sADDSUBSYSTEM(JoypadManager,0xa0,sInitJoypadManager,sExitJoypadManager);
#if !sCONFIG_OPTION_XSI
sADDSUBSYSTEM(DirectInput,0xa0,sInitDInput,sExitDInput);
sADDSUBSYSTEM(DirectInputJoypad,0xa1,sInitDIJoypad,sExitDIJoypad);
#endif

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Multithreading                                                     ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

__declspec(thread) sThreadContext* sTlsContext;
sThreadContext sEmergencyThreadContext;            // the context of the mainthread

/****************************************************************************/

sInt sGetCPUCount()
{
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return info.dwNumberOfProcessors;
}

#if sCONFIG_OPTION_XSI
sThreadContext *sGetThreadContext()
{
  return &sEmergencyThreadContext;
}
#else
sThreadContext *sGetThreadContext()
{
  if(sTlsContext==0)
    return &sEmergencyThreadContext;
  else 
    return sTlsContext;
}
#endif

/****************************************************************************/

void sSleep(sInt ms)
{
  Sleep(ms);
}

unsigned long sSTDCALL sThreadTrunk(void *ptr)
{
  sThread *th = (sThread *) ptr;

#if !sCONFIG_OPTION_XSI
  sTlsContext = th->Context;
#endif

  th->Code(th,th->Userdata);

  return 0;
}

/****************************************************************************/

sThread::sThread(void (*code)(sThread *,void *),sInt pri,sInt stacksize,void *userdata, sInt flags/*=0*/)
{
  sVERIFY(sizeof(ULONG)==sizeof(sU32));

  TerminateFlag = 0;
  Code = code;
  Userdata = userdata;
  Stack = 0;
  Context = sCreateThreadContext(this);

  ThreadHandle = CreateThread(0,stacksize,sThreadTrunk,this,0,(ULONG *)&ThreadId);
  if(pri>0)
  {
    SetPriorityClass(ThreadHandle,HIGH_PRIORITY_CLASS);
    SetThreadPriority(ThreadHandle,THREAD_PRIORITY_TIME_CRITICAL);
  }
  if(pri==0)
  {
//    SetThreadPriorityBoost(ThreadHandle,TRUE);
  }
  if(pri<0)
  {
    SetThreadPriority(ThreadHandle,THREAD_PRIORITY_BELOW_NORMAL);
  }
}

sThread::~sThread()
{
  if(TerminateFlag==0)
    Terminate();
  if(TerminateFlag==1)
    WaitForSingleObject(ThreadHandle,INFINITE);
  CloseHandle(ThreadHandle);
  delete Context;
}

void sInitEmergencyThread()
{
  sClear(sEmergencyThreadContext);
  sEmergencyThreadContext.ThreadName = L"MainThread";
  sEmergencyThreadContext.MemTypeStack[0] = sAMF_HEAP;
  sTlsContext = &sEmergencyThreadContext;
}

void sThread::SetHomeCore(sInt core)
{
  if(this==0)
    SetThreadIdealProcessor(GetCurrentThread(),core);
  else
    SetThreadIdealProcessor(ThreadHandle,core);
}

/****************************************************************************/

sThreadLock::sThreadLock()
{
  CriticalSection = new CRITICAL_SECTION;
  InitializeCriticalSection((CRITICAL_SECTION*)CriticalSection);
}

sThreadLock::~sThreadLock()
{
  DeleteCriticalSection((CRITICAL_SECTION*)CriticalSection);
  delete CriticalSection;
}


void sThreadLock::Lock()
{
  EnterCriticalSection((CRITICAL_SECTION*)CriticalSection);
}

sBool sThreadLock::TryLock()
{
  return TryEnterCriticalSection((CRITICAL_SECTION*)CriticalSection);
}

void sThreadLock::Unlock()
{
  LeaveCriticalSection((CRITICAL_SECTION*)CriticalSection);
}

/****************************************************************************/

sThreadEvent::sThreadEvent(sBool manual/*=sFALSE*/)
{
  EventHandle = CreateEvent(0,manual?TRUE:0,0,0);
}

sThreadEvent::~sThreadEvent()
{
  CloseHandle(EventHandle);
}

sBool sThreadEvent::Wait(sInt timeout)
{
  return WaitForSingleObject(EventHandle,timeout)==WAIT_OBJECT_0;
}

void sThreadEvent::Signal()
{
  SetEvent(EventHandle);
}

void sThreadEvent::Reset()
{
  ResetEvent(EventHandle);
}

/****************************************************************************/

void sPrintError(const sChar *text)
{
  if(sRELEASE!=0)
    sDPrint(text);    // in release mode we want info as debug output, too

  CONSOLE_SCREEN_BUFFER_INFO info={0};
  sBool restore_color = sFALSE;
  if(WConOut)
  {
    if(GetConsoleScreenBufferInfo(WConOut,&info))
    {
      restore_color = sTRUE;
      SetConsoleTextAttribute ( WConOut, FOREGROUND_RED|FOREGROUND_INTENSITY );
    }
  }
  sPrint(text);
  if(WConOut&&restore_color)
    SetConsoleTextAttribute ( WConOut, info.wAttributes );
}

void sPrintWarning(const sChar *text)
{
  if(sRELEASE!=0)
    sDPrint(text);    // in release mode we want info as debug output, too

  CONSOLE_SCREEN_BUFFER_INFO info={0};
  sBool restore_color = sFALSE;
  if(WConOut)
  {
    if(GetConsoleScreenBufferInfo(WConOut,&info))
    {
      restore_color = sTRUE;
      SetConsoleTextAttribute ( WConOut, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY );
    }
  }
  sPrint(text);
  if(WConOut&&restore_color)
    SetConsoleTextAttribute ( WConOut, info.wAttributes );
}

void sPrint(const sChar* t)
{
  if(sRedirectStdOut)
  {
    (*sRedirectStdOut)(t);
    return;
  }
  DWORD dummy;
  if(WConOut)     // everyone can do unicode - only the visual studio build system can't!
  {
    //  WriteConsoleW(WConOut,t,sGetStringLen(t),&dummy,0);

    sChar8 buffer[1024];
    sInt left = sGetStringLen(t);
    while(left)
    {
      sInt count = sMin<sInt>(sCOUNTOF(buffer),left);
      left -= count;
      for(sInt i=0;i<count;i++)
        buffer[i] = *t++;
      WriteFile(WConOut,buffer,count,&dummy,0);
    }
  }
  if(sRELEASE==0)
    OutputDebugStringW(t);
}

//#endif // sCONFIG_SYSTEM_WINDOWS

/****************************************************************************/
/***                                                                      ***/
/***   Platform Dependend Memory Code                                     ***/
/***                                                                      ***/
/****************************************************************************/

sU8 *sMainHeapBase;
sU8 *sDebugHeapBase;
class sMemoryHeap sMainHeap;
class sMemoryHeap sDebugHeap;

class sVSHeapBase : public sMemoryHandler
{

  void GetFreeStats(sCONFIG_SIZET &total,  sCONFIG_SIZET &largest)
  {
    total=0;
    largest=0;

    HANDLE hproc = GetCurrentProcess();
    MEMORY_BASIC_INFORMATION mbi;
    const sU8 *ptr = 0;

    while (VirtualQueryEx(hproc,ptr,&mbi,sizeof(mbi)))
    {
      ptr+=mbi.RegionSize;
      if (mbi.State==MEM_FREE)
      {
        total+=mbi.RegionSize;
        largest=sMax<sCONFIG_SIZET>(largest,mbi.RegionSize);
      }
    }

  }

  sCONFIG_SIZET GetFree() { sCONFIG_SIZET total,largest; GetFreeStats(total,largest); return total; }
  sCONFIG_SIZET GetLargestFree() { sCONFIG_SIZET total,largest; GetFreeStats(total,largest); return largest; }
};

class sVSReleaseHeap_ : public sVSHeapBase
{
public:
  void *Alloc(sPtr size,sInt align,sInt flags)
  {
    void *mem = _aligned_malloc(size,align);
    if(mem==0) return 0;
    sAtomicAdd(&sMemoryUsed, (sDInt)_aligned_msize(mem,0,0));
    return mem;
  }
  sBool Free(void *ptr)
  {
    sAtomicAdd(&sMemoryUsed,-(sDInt)_aligned_msize(ptr,0,0));
    _aligned_free(ptr);
    return 1;
  }

  sPtr MemSize(void *memblock)
  {
    uintptr_t ptr;

    ptr = (uintptr_t)memblock;

    // ptr points to the pointer to starting of the memory block
    ptr = (ptr & ~(sizeof(void *) -1)) - sizeof(void *);

    // ptr is the pointer to the start of memory block
    ptr = *((uintptr_t *)ptr);

    return _msize((void*)ptr);
  }

} sVSReleaseHeap;

class sVSDebugHeap_ : public sVSHeapBase
{
public:
  void *Alloc(sPtr size,sInt align,sInt flags)
  {
    size += sizeof(sPtr)+align-1;
    sThreadContext *tx=sGetThreadContext(); tx; // "tx;" to fix unused local variable warning
    void *po = _malloc_dbg(size,_NORMAL_BLOCK,tx->TagMemFile?tx->TagMemFile : "unknown",tx->TagMemLine);
    if(po==0) return 0;
    void *ptr = (void *)((sPtr(po)+sizeof(sPtr)+align-1)&~(align-1));
    *((sPtr*) (sPtr(ptr)-sizeof(sPtr))) = sPtr(po);
    sAtomicAdd(&sMemoryUsed,(sDInt)_msize_dbg(po,_NORMAL_BLOCK));

    return ptr;
  }
  sBool Free(void *ptr)
  {
    void *po = (void*)((sPtr*)ptr)[-1];
    sAtomicAdd(&sMemoryUsed,-(sDInt)_msize_dbg(po,_NORMAL_BLOCK));
    _free_dbg(po,_NORMAL_BLOCK);
    return 1;
  }

  sPtr MemSize(void *ptr)
  {
    void *po = (void*)((sPtr*)ptr)[-1];
    return _msize_dbg(po,_NORMAL_BLOCK);
  }

  void Validate()
  {
    if(!_CrtCheckMemory())
      sFatal(L"sVSDebugHeap_ validate failed\n");
  }
} sVSDebugHeap, sVSDebugHeapD;

void sInitMem2(sPtr gfx)
{
}

void sInitMem1()
{
  sMainHeapBase = 0;
  sDebugHeapBase = 0;
  
  sInt flags = sMemoryInitFlags;

  if(flags & sIMF_DEBUG)
  {
    if (flags & sIMF_NORTL)
    {
      sInt size = sMemoryInitDebugSize;
      sDebugHeapBase = (sU8 *)VirtualAlloc(0,size,MEM_COMMIT,PAGE_READWRITE);
      sDebugHeap.Init(sDebugHeapBase,size);
      sRegisterMemHandler(sAMF_DEBUG,&sDebugHeap);
    }
    else
      sRegisterMemHandler(sAMF_DEBUG,&sVSDebugHeapD);
  }
  if((flags & sIMF_NORTL) && sMemoryInitSize>0)
  {
    sMainHeapBase = (sU8 *)VirtualAlloc(0,sMemoryInitSize,MEM_COMMIT,PAGE_READWRITE);
    if(flags & sIMF_CLEAR)
      sSetMem(sMainHeapBase,0x77,sMemoryInitSize);
    sMainHeap.Init(sMainHeapBase,sMemoryInitSize);
    sMainHeap.SetDebug((flags & sIMF_CLEAR)!=0,0);
    sRegisterMemHandler(sAMF_HEAP,&sMainHeap);
  }
  else
  {
    if(flags & sIMF_DEBUG)
    {
      sRegisterMemHandler(sAMF_HEAP,&sVSDebugHeap);
      _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)|_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF);
    }
    else
    {
      sRegisterMemHandler(sAMF_HEAP,&sVSReleaseHeap);
    }
  }
}

void sExitMem1()
{
  if(sDebugHeapBase) VirtualFree(sDebugHeapBase,0,MEM_RELEASE);
  if(sMainHeapBase) VirtualFree(sMainHeapBase,0,MEM_RELEASE);

  sUnregisterMemHandler(sAMF_DEBUG);
  sUnregisterMemHandler(sAMF_HEAP);
}

/****************************************************************************/

sLanguage sGetLanguage()
{
  sInt lang = GetUserDefaultLangID();
  switch(lang&0x3ff)
  {
  case LANG_GERMAN:     return sLANG_DE;
  case LANG_SPANISH:    return sLANG_ES;
  case LANG_ITALIAN:    return sLANG_IT;
  case LANG_FRENCH:     return sLANG_FR;
  case LANG_ENGLISH:    return sLANG_EN;
  case LANG_PORTUGUESE: return sLANG_PT;
  case LANG_DUTCH:      return sLANG_NL; 
  default: break;
  }

  return sLANG_UNKNOWN;
}

#if !sCONFIG_OPTION_DEMO
sBool sIsTrialVersion()
{
  return sTrialVersion;
}
#endif

#if sCONFIG_OPTION_DEMO
sU32 sGetDemoTimeOut()
{
#if !sSTRIPPED
  static const sU32 TimeOut = sGetShellParameterInt(L"timeout",0,0);
  return TimeOut;
#else
  return 0;
#endif
}
#endif

void sPreventPaint()
{
  if (sInPaint==1) sInPaint++;
}

/****************************************************************************/
/***                                                                      ***/
/***   Video writer                                                       ***/
/***                                                                      ***/
/****************************************************************************/

#pragma comment(lib,"vfw32.lib")

class sVideoWriterWin32 : public sVideoWriter
{
  IAVIFile *File;
  IAVIStream *VidRaw,*Video;

  sString<sMAXPATH> BaseName;
  sInt Segment;
  sU32 Codec;
  sU32 OverflowCounter;
  sF32 FPS;

  sU8 *ConversionBuffer;
  sInt XRes,YRes;
  sInt Frame;

public:
  sVideoWriterWin32();
  ~sVideoWriterWin32();

  sBool Init(const sChar *filename,const sChar *codec,sF32 fps,sInt xRes,sInt yRes);
  void Exit();

  sBool AVIInit();
  void AVICleanup();

  sBool WriteFrame(const sU32 *data);
};

sVideoWriterWin32::sVideoWriterWin32()
{
  File = 0;
  VidRaw = Video = 0;
  ConversionBuffer = 0;
}

sVideoWriterWin32::~sVideoWriterWin32()
{
  Exit();
}

sBool sVideoWriterWin32::Init(const sChar *filename,const sChar *codec,sF32 fps,sInt xRes,sInt yRes)
{
  Exit();

  XRes = xRes;
  YRes = yRes;
  Frame = 0;
  FPS = fps;
  
  sCopyString(BaseName,filename);
  sChar *ext = sFindFileExtension(BaseName);
  if(ext)
    ext[-1] = 0;

  if(codec && sGetStringLen(codec) >= 4)
    Codec = MAKEFOURCC(codec[0],codec[1],codec[2],codec[3]);
  else
    Codec = 0;

  Segment = 1;
  ConversionBuffer = new sU8[xRes*yRes*3];

  return AVIInit();
}

void sVideoWriterWin32::Exit()
{
  AVICleanup();
  sDeleteArray(ConversionBuffer);
}

sBool sVideoWriterWin32::AVIInit()
{
  AVISTREAMINFOW asi;
  AVICOMPRESSOPTIONS aco;
  BITMAPINFOHEADER bmi;
  sBool error = sTRUE;

  AVIFileInit();
  OverflowCounter = 0;

  // create the file
  sString<sMAXPATH> name;
  name.PrintF(L"%s.%02d.avi",BaseName,Segment);
  if(AVIFileOpenW(&File,name,OF_CREATE|OF_WRITE,0) != AVIERR_OK)
  {
    sDPrintF(L"sVideoWriterWin32: AVIFileOpen failed.\n");
    goto cleanup;
  }

  // video stream
  sClear(asi);
  asi.fccType = streamtypeVIDEO;
  asi.dwScale = 10000;
  asi.dwRate = (DWORD) (asi.dwScale * FPS + 0.5f);
  asi.dwSuggestedBufferSize = XRes * YRes * 3;
  SetRect(&asi.rcFrame,0,0,XRes,YRes);
  sCopyString(asi.szName,L"Video",sCOUNTOF(asi.szName));

  if(AVIFileCreateStreamW(File,&VidRaw,&asi) != AVIERR_OK)
  {
    sDPrintF(L"sVideoWriterWin32: AVIFileCreateStream failed.\n");
    goto cleanup;
  }

  sU32 codec = Codec ? Codec : MAKEFOURCC('D','I','B',' ');
  sClear(aco);
  aco.fccType = streamtypeVIDEO;
  aco.fccHandler = codec;
  aco.dwQuality = 100;

  if(AVIMakeCompressedStream(&Video,VidRaw,&aco,0) != AVIERR_OK)
  {
    sDPrintF(L"sVideoWriterWin32: AVIMakeCompressedStream failed - invalid codec FOURCC set?\n");
    goto cleanup;
  }

  sClear(bmi);
  bmi.biSize = sizeof(bmi);
  bmi.biWidth = XRes;
  bmi.biHeight = YRes;
  bmi.biPlanes = 1;
  bmi.biBitCount = 24;
  bmi.biCompression = BI_RGB;
  bmi.biSizeImage = XRes * YRes * 3;
  if(AVIStreamSetFormat(Video,0,&bmi,sizeof(bmi)) != AVIERR_OK)
  {
    sDPrintF(L"sVideoWriterWin32: AVIStreamSetFormat failed.\n");
    goto cleanup;
  }

  error = sFALSE;
  Frame = 0;

cleanup:
  if(error)
    AVICleanup();

  return !error;
}

void sVideoWriterWin32::AVICleanup()
{
  sRelease(Video);
  sRelease(VidRaw);
  sRelease(File);

  AVIFileExit();
}

sBool sVideoWriterWin32::WriteFrame(const sU32 *data)
{
  sBool error = sTRUE;

  if(Video)
  {
    if(OverflowCounter >= 1024*1024*1800)
    {
      AVICleanup();
      Segment++;
      AVIInit();
    }

    // convert from ARGB8888 to RGB888
    for(sInt y=0;y<YRes;y++)
    {
      const sU8 *src = (const sU8 *) data + (YRes-1-y) * XRes * 4;
      sU8 *dst = ConversionBuffer + y * XRes * 3;

      for(sInt x=0;x<XRes;x++)
      {
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        src++;
      }
    }

    LONG written = 0;
    AVIStreamWrite(Video,Frame,1,(void *)ConversionBuffer,3*XRes*YRes,0,0,&written);
    Frame++;
    OverflowCounter += written;

    error = (written == 0);
  }

  return !error;
}

sVideoWriter *sCreateVideoWriter(const sChar *filename,const sChar *codec,sF32 fps,sInt xRes,sInt yRes)
{
  sVideoWriterWin32 *writer = new sVideoWriterWin32;
  if(!writer->Init(filename,codec,fps,xRes,yRes))
  {
    delete writer;
    writer = 0;
  }

  return writer;
}

/****************************************************************************/
#define SECURITY_WIN32
#include <wininet.h>
#include <security.h>
#pragma comment (lib,"advapi32.lib") // for GetUserNameW
#pragma comment (lib,"Secur32.lib")  // for GetUserNameExW
#pragma comment (lib,"wininet.lib")  // for InternetGetConnectedState
#undef SECURITY_WIN32

sBool sGetUserName(const sStringDesc &dest, sInt joypadId)
{
  if(joypadId < 0 || joypadId >= sJOYPAD_COUNT)
    return sFALSE;

  sChar buffer[128];
  DWORD count = sCOUNTOF(buffer);

  // GetUserNameExW returns the display name, set in user properties of windows
  if(GetUserNameExW(NameDisplay, buffer, &count) != 0 && buffer[0]) // sometimes it seems to return an empty string for NameDisplay
  {
    sCopyString(dest, buffer);
    return sTRUE;
  }
  else
  {
    // if no display name is set, this should always return the account name
    if(GetUserNameW(buffer, &count) != 0)
    {
      sCopyString(dest, buffer);
      return sTRUE;
    }
  }

  return sFALSE;
}

sBool sGetOnlineUserName(const sStringDesc &dest, sInt joypadId)
{
  return sGetUserName(dest, joypadId);
}

sBool sIsUserOnline(sInt joypadId)
{
  sBool online = InternetGetConnectedState(sNULL, 0);
  return online;
}

sBool sGetOnlineUserId(sU64 &dest, sInt joypadId)
{
  sString<128> userName;
  sString<128> computerName;

  if(!sGetOnlineUserName(userName, joypadId))
    return sFALSE;

  DWORD size=computerName.Size();
  if(!GetComputerNameW(computerName, &size))
    return sFALSE;

  //Take the processId into account, to generate different IDs 
  //in case the game runs twice on the same machine (debugging on localhost)
  DWORD processId = 0;
  if(sGetShellSwitch(L"uniqueonlineid"))
    processId = GetCurrentProcessId();

  // UserName@ComputerName.ProcessId
  sString<272> ident;
  sSPrintF(ident, L"%s@%s.%d", userName, computerName, (sU64)processId);

  dest = sHashStringFNV(ident);
  return sTRUE;
}

/****************************************************************************/

#define PSAPI_VERSION 1
#include <psapi.h>
#pragma comment (lib,"Psapi.lib")

sInt sGetNumInstances()
{
  // Get the list of process ids.
  DWORD processes[1024], bytesUsed, numReturned;
  if (!EnumProcesses( processes, sizeof(processes), &bytesUsed ))
    return 1;

  numReturned = bytesUsed / sizeof(DWORD);

  sChar ownName[128];
  GetModuleBaseName(GetCurrentProcess(), sNULL, ownName, sCOUNTOF(ownName));

  sInt numInstances = 0;
  // check the name
  for (DWORD i = 0; i < numReturned; ++i)
  {
    if (processes[i] != 0)
    {
      sChar processName[128];

      HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processes[i]);

      if (hProcess)
      {
        HMODULE hMod;
        DWORD cbNeeded;

        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
        {
          GetModuleBaseName(hProcess, hMod, processName, sCOUNTOF(processName));
          if (sCmpString(ownName, processName) == 0)
            numInstances++;
        }
      }

      CloseHandle(hProcess);
    }
  }

  return numInstances;
}


/****************************************************************************/
/****************************************************************************/

#endif // sCONFIG_SYSTEM_WINDOWS

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

#ifndef HEADER_ALTONA_BASE_SYSTEM
#define HEADER_ALTONA_BASE_SYSTEM

#ifndef __GNUC__
#pragma once
#endif

#include "base/types.hpp"

#if sPLATFORM==sPLAT_WINDOWS || sPLATFORM==sPLAT_LINUX 
template<typename T> class sArray;
#endif

/****************************************************************************/
/***                                                                      ***/
/***   initialization                                                     ***/
/***                                                                      ***/
/****************************************************************************/

void sInit(sInt flags,sInt xs=0,sInt ys=0); // this function should also have parameters to specify the window position
void sExit();
void sRestart();
sInt sGetSystemFlags();
void sSetErrorCode(sInt i=1);           // set the commandline error code. 0=OK, 1=ERROR

/****************************************************************************/

enum sInitSystemFlags
{
  sISF_FULLSCREEN       = 0x0001,     // create window in fullscreen
  sISF_3D               = 0x0002,     // allow 3d rendering (DirectX)
  sISF_2D               = 0x0004,     // allow 2d rendering (GDI)
  sISF_CONTINUOUS       = 0x0008,     // update automatically
  sISF_NOVSYNC          = 0x0100,     // disable wait for VSync, for benchmarks!
  sISF_REFRAST          = 0x0200,     // use reference rasterizer (sloooow)
  sISF_NOBUSYWAIT       = 0x0400,     // no busy wait during FLIP, so you get some more multithreading  
  sISF_NOSOUND          = 0x0800,     // do not initialize sound
  sISF_MINIMAL          = 0x1000,     // omit platform/OS integration components, for testing only!
  sISF_FSAA             = 0x2000,     // enable full scene anti aliasing
  sISF_NODWM            = 0x4000,     // disable Vista/Win7 Desktop Window Manager
  sISF_NOSCREENSAVE     = 0x8000,     // disable screensaver and monitor power saving

  sISF_SHADER_BEST      = 0x0000,     // use best available shader model
  sISF_SHADER_00        = 0x0010,     // revert to fixed function pipeline
  sISF_SHADER_13        = 0x0020,     // revert to shader model 1.3
  sISF_SHADER_20        = 0x0030,     // revert to shader model 2.0
  sISF_SHADERMASK       = 0x0070, 


#if sRELEASE || sSTRIPPED
  sISF_FULLSCREENIFRELEASE = sISF_FULLSCREEN,
#else
  sISF_FULLSCREENIFRELEASE = 0,
#endif
};

/****************************************************************************/

sBool sIsSubsystemRunning(const sChar *name);
void sAddSubsystem(const sChar *name,sInt priority,void (*init)(),void (*exit)());
void sSetRunlevel(sInt priority);
sInt sGetRunlevel();
#define sADDSUBSYSTEM(n,p,i,e) namespace sSubSystems { struct n##_type { n##_type() {sAddSubsystem(sTXT(#n),p,i,e);} } n##_global; }

/****************************************************************************/

// additional init if called from outside (winmain is not possible)
void sExternMainInit(void *p_instance, void *p_hwnd);
void sExternMessageLoop(void *p_messageLoopData);
int  sExternMessageLoopGetSize(); // just to be sure to have the same struct
void sExternMainExit();

/****************************************************************************/
/***                                                                      ***/
/***   Misc                                                               ***/
/***                                                                      ***/
/****************************************************************************/

struct sDateAndTime
{
  sU16 Year;        // Woops! Y0x10000 problem!
  sU8  Month;       // 1-12 (January=1)
  sU8  Day;         // Day of month 1-31
  sU8  DayOfWeek;   // 0-6 (0=Sunday)
  sU8  Hour;        // 0-23
  sU8  Minute;      // 0-59
  sU8  Second;      // 0-60 (! - leap seconds!)

  sDateAndTime();
  sInt Compare(const sDateAndTime &x) const;

  bool operator ==(const sDateAndTime &x) const   { return Compare(x) == 0; }
  bool operator !=(const sDateAndTime &x) const   { return Compare(x) != 0; }
  bool operator < (const sDateAndTime &x) const   { return Compare(x) <  0; }
  bool operator <=(const sDateAndTime &x) const   { return Compare(x) <= 0; }
  bool operator > (const sDateAndTime &x) const   { return Compare(x) >  0; }
  bool operator >=(const sDateAndTime &x) const   { return Compare(x) >= 0; }
};

sBool sExecuteShell(const sChar *cmdline);
sBool sExecuteShellDetached(const sChar *cmdline);
sBool sExecuteShell(const sChar *cmdline,class sTextBuffer *tb);
sBool sExecuteOpen(const sChar *file);
sBool sGetEnvironmentVariable(const sStringDesc &dst,const sChar *var);

void sConsoleWindowClear();
sInt sGetTime();
sU64 sGetTimeUS(); // microseconds, resolution as good as possible

sDateAndTime sGetDateAndTime(); // local time
sDateAndTime sAddLocalTime(sDateAndTime origin,sInt seconds);
sS64 sDiffLocalTime(sDateAndTime a,sDateAndTime b); // a-b (returns difference in seconds)
sU8 sGetFirstDayOfWeek(); //! Gets first day of week, depending on users locale setting. (0=Sunday)
sBool sIsLeapYear(sU16 year);
sBool sIsDateValid(sU16 year, sU8 month, sU8 day);
sBool sIsDateValid(const sDateAndTime &date);
sInt sGetDaysInMonth(sU16 year, sU8 month);
sU8 sGetDayOfWeek(const sDateAndTime &date);

void sCatchCtrlC(sBool enable=1);
sBool sGotCtrlC();

#if sPLATFORM==sPLAT_WINDOWS || sPLATFORM==sPLAT_LINUX 
// system logger: writes to syslog on linux/unix (normal sLog on win)
void sSysLogInit(const sChar *appname);
void sSysLog(const sChar *module,const sChar *text);
sPRINTING1(sSysLogF,sFormatStringBuffer buf; sFormatStringBaseCtx(buf,format);buf,sSysLog(arg1,buf.Get());,const sChar *)

sBool sDaemonize(); // turns the process into a daemon (if successful)

sInt sGetNumInstances(); // returns the number of times, the application is currently running
#endif

sInt sGetRandomSeed();
const sChar *sGetCommandLine();

extern sHooks *sFrameHook;        // called every frame
extern sHooks *sNewDeviceHook;    // called when new io devices where attached
extern sHooks1<sBool &> *sAltF4Hook;      // os wants to terminate app. set bool to 0 to prevent
extern sHooks1<sBool> *sActivateHook;     // called when app is activated/deactivated
extern sHooks1<sStringDesc> *sCrashHook;  // called when app is crashed (after stacktrace is generated)
extern sHooks *sCheckCapsHook;    // this is your last chance to check device caps...
extern sHooks2<const sInput2Event &,sBool &> *sInputHook; // called on input (for debugging)
                                                          // when setting bool param to true in hook input
                                                          // is not send to application




sBool sStackTrace(const sStringDesc &tgt,sInt skipCount=1,sInt maxCount=8); // call stack trace for sFatal etc. return sFALSE if not implemented.

// if called within App::OnPaint, prevent recursive painting this once.
void sPreventPaint();

// timing helper for easy scope profiling
struct sTimeHelper
{
  sInt Start;
  const sChar *Msg;
  sU32 *TotalTime;
  sTimeHelper(const sChar *msg):Msg(msg) { TotalTime = 0; Start = sGetTime(); }
  sTimeHelper(const sChar *msg, sU32 &totaltime):Msg(msg) { TotalTime = &totaltime; Start = sGetTime(); }
  ~sTimeHelper()
  {
#if !sSTRIPPED
    sInt End = sGetTime(); sInt time = (End-Start);
    if(TotalTime)
    {
      *TotalTime += time;
      if(time>10000)
        sDPrintF(L"%s %f sec (total %f sec)\n",Msg,time/1000.0f,*TotalTime/1000.0f);
      else
        sDPrintF(L"%s %d ms (total %f sec)\n",Msg,time,*TotalTime/1000.0f);
    }
    else
    {
      if(time>10000)
        sDPrintF(L"%s %f sec\n",Msg,time/1000.0f);
      else
        sDPrintF(L"%s %d ms\n",Msg,time);
    }
#endif
  }
};

#define sGetScopeTime(msg) sTimeHelper gettime##__LINE__(msg);
#define sGetScopeTotalTime(msg,total) sTimeHelper gettime##__LINE__(msg,total);


/****************************************************************************/

enum sLanguage // ISO 3166-1-alpha-2 code elements
{
  sLANG_UNKNOWN = 0,
  sLANG_EN,   // englisch

  sLANG_US = sLANG_EN,
  sLANG_GB,

  sLANG_DE,   // german
  sLANG_FR,   // french
  sLANG_ES,   // spanish
  sLANG_IT,   // italian
  sLANG_NL,   // dutch
  sLANG_PT,   // portuguese
  sLANG_RU,   // russian
  sLANG_JP,   // japanese
};

sLanguage sGetLanguage();

/****************************************************************************/

#if sCONFIG_OPTION_DEMO
// A demo is a separate game with limited functionality.
// It is not possible to upgrade a demo to a full title by
// obtaining a license for example, this is only possible with a trial version.
// This function is inlined, because the compiler is able to optimize/remove surrounding code,
// so it is harder "patch" the demo to a full title.
sINLINE sBool sIsTrialVersion() { return sTRUE; }
sU32 sGetDemoTimeOut();   // 0 : never, otherwise timeout time in ms
#else
// A trial version is a full title, with limited functionality only.
// It actually contains the whole content and logic of the full title.
// To unlock all features, the user has to obtain a license.
sBool sIsTrialVersion();
sINLINE sU32 sGetDemoTimeOut() { return 0; }   // 0 : never, otherwise timeout time in ms
#endif

/****************************************************************************/

// system memory info
enum sSysMemKind
{
  sMEMKIND_CPU  = 0x01,
  sMEMKIND_GPU  = 0x02,
};

struct sSysMemInfo
{
  sInt Kind;
  sCONFIG_SIZET PhysicalTotal;
  sCONFIG_SIZET PhysicalAvailable;
  sCONFIG_SIZET VirtualTotal;
  sCONFIG_SIZET VirtualAvailable;
};

// returns number of different memory types, fillst up to count info entries with mem infos
sInt sGetSystemMemInfo(sSysMemInfo* info=0, sInt count=0);

sS64 sGetUsedMainMemory();          // negative result: memory free
void sEnableMemoryAllocation(sBool enable);

/****************************************************************************/
/***                                                                      ***/
/***   Multithreading                                                     ***/
/***                                                                      ***/
/****************************************************************************/

enum sThreadConsts
{
  sMAX_THREADCOUNT = 16,
  sMAX_APP_TLS     = 64,  // maximal bytes for thread local storage for sAllocTls
};

enum sThreadFlags
{
  // thread assignment
  sTHREAD_ASSIGN_ANY  = 0x00000,
  sTHREAD_ASSIGN_CPU0 = 0x10000,
  sTHREAD_ASSIGN_CPU1 = 0x20000,
  sTHREAD_ASSIGN_CPU2 = 0x30000,
  sTHREAD_ASSIGN_CPU3 = 0x40000,
  sTHREAD_ASSIGN_CPU4 = 0x50000,
  sTHREAD_ASSIGN_CPU5 = 0x60000,
  sTHREAD_ASSIGN_MSK  = 0xf0000,
};

enum sThreadContextFlags
{
  sTHF_NODEBUG  = 0x00000001,       // in this thread sDPrintF is not allowed to print at all
  sTHF_NONETDEBUG  = 0x00000002,    // in this thread sDPrintF is not allowed to print debuginfo over the net
};

enum sThreadPriority
{
  sTHREAD_PRIORITY_LOW    = -1,
  sTHREAD_PRIORITY_NORMAL = 0,
  sTHREAD_PRIORITY_HIGH   = 1,
};

typedef void (sThreadFunc)(class sThread *, void *);
//typedef sBool (sThreadFuncBool)(sThread *, void *);

/****************************************************************************/

struct sThreadContext
{
  static sPtr TlsOffset;
  sU32 Flags;                       // some flags for the thread
  sChar PrintBuffer[1024];        // the buffer for the threadsafe print-functions
  sString<64> ThreadName;           // a name for the thread
  //  void *UserData;                   // free for your own business
  class sThread *Thread;            // thread linked to context

  sInt MemTypeStack[16];
  sInt MemTypeStackIndex;

#if sCONFIG_DEBUGMEM
  sInt TagMemLine;
  const char *TagMemFile;
  const char *TagLastFile;

  sChar MemLeakDescBuffer[sMemoryLeakDesc::STRINGLEN];
  sChar MemLeakDescBuffer2[sMemoryLeakDesc::STRINGLEN]; 
  sInt MLDBPos;
  sU32 MLDBCRC;

#endif

#if !sSTRIPPED
  void *PerfData; // for PerfMon
#endif

  void *GetTls(sPtr offset) { if(offset<TlsOffset) return (void *)(((sU8 *)this)+offset); else return 0; }

  // used by sAllocFrame
#if sCONFIG_FRAMEMEM_MT
  sU64 FrameFrame;                // check if data needs to be reset
  sPtr FrameCurrent;              // primary buffer for allocation (borrowed from global frame mem)
  sPtr FrameEnd;
  sPtr FrameAltCurrent;           // alternate buffer for allocation (borrowed from global frame mem)
  sPtr FrameAltEnd;
  sPtr FrameBorrow;               // sAllovFrameBegin/sAllocFrameEnd to primary buffer
#endif

  // used by sAllocDma
  sPtr DmaCurrent;                // primary buffer for allocation (borrowed from global frame mem)
  sPtr DmaEnd;
  sPtr DmaAltCurrent;             // alternate buffer for allocation (borrowed from global frame mem)
  sPtr DmaAltEnd;
  sPtr DmaBorrow;                 // sAllovFrameBegin/sAllocFrameEnd to primary buffer
  sU64 DmaFrame;                  // check if data needs to be reset

  // sAllocTls memory
  sU8 Mem[sMAX_APP_TLS];          // memory for application thread context allocation (sAllocTls)
};

/****************************************************************************/

class sThread
{
  friend unsigned long sSTDCALL sThreadTrunk(void *ptr);
  friend void * sSTDCALL sThreadTrunk_pthread(void *ptr);
  friend void sThreadTrunk(sThread *);
  void *ThreadHandle;
#if sPLATFORM==sPLAT_LINUX
  sU64 ThreadId;
#else
  sU32 ThreadId;
#endif

  volatile sBool TerminateFlag;
  sThreadFunc *Code;
  void *Userdata;
  sU8 *Stack;
  sThreadContext *Context;


public:
  sThread(sThreadFunc *threadFunc,sInt pri=0,sInt stacksize=0x4000, void *userdata=sNULL, sInt flags=0);
  ~sThread();

  void Terminate()          { TerminateFlag = 1; }
  sBool CheckTerminate()     { return !TerminateFlag; }
  sThreadContext *GetContext() { return Context; } 
  void SetHomeCore(sInt core);
};

/****************************************************************************/

void sSleep(sInt ms);
sInt sGetCPUCount();
sThreadContext *sGetThreadContext();
sThreadContext *sCreateThreadContext(sThread *);
sPtr sAllocTls(sPtr bytes,sInt align);
inline void *sGetTls(sPtr offset) { return sGetThreadContext()->GetTls(offset); }
template <typename T> sPtr sAllocTls(T*& ptr, sInt align=4) { sPtr offset=sAllocTls(sizeof(T),align); ptr = sGetTls(offset); return offset; }
template <typename T> inline  T *sGetTls(sPtr offset) { return (T *) sGetThreadContext()->GetTls(offset); }

/****************************************************************************/

class sThreadLock
{
  void *CriticalSection;
public:
  sThreadLock();
  ~sThreadLock();

  void Lock();
  sBool TryLock();
  void Unlock();
};

/****************************************************************************/

class sThreadEvent
{
#if sPLATFORM==sPLAT_LINUX || sPLATFORM==sPLAT_IOS
  volatile sU32 Signaled;
  sBool ManualReset;
#else
  void *EventHandle;
#endif
public:
  sThreadEvent(sBool manual=sFALSE);    // with manual == sFALSE the signal gets deactivated be first waiting thread
  // use manual == sTRUE to disable event only by Reset
  // (can be usefull to synchronising multiple threads)
  ~sThreadEvent();

  sBool Wait(sInt timeout=-1);    // 0 for immediate return, -1 for infinite wait
  void Signal();                  // the signal is consumed by the first thread for automatic events
  void Reset();                   // go into nonsignaled state
};

/****************************************************************************/


class sScopeLock                  // scope lock helper
{
private:
  sThreadLock *Lock;
public:
  sScopeLock(sThreadLock *l):Lock(l) { sVERIFY(l); Lock->Lock(); }
  ~sScopeLock()                      { Lock->Unlock(); }
};


// locks by simply disabling interrupts - only avaliable on single-CPU platforms!
class sIRQScopeLock
{
  sInt OldIRQ;
public:
  sIRQScopeLock(sInt *dummy);
  ~sIRQScopeLock();
};

/****************************************************************************/
/***                                                                      ***/
/***   Lockless Programming                                               ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   Lockless Programming, especially for more than one CPU             ***/
/***   architecture, is very dangerous and difficult. Do not change       ***/
/***   a single line of this unless you know exactly what you are doing   ***/
/***   and you do extensive testing on all platforms!                     ***/
/***                                                                      ***/
/****************************************************************************/

#if sCONFIG_COMPILER_MSC

extern "C" long _InterlockedIncrement(long volatile *);
extern "C" long _InterlockedDecrement(long volatile *);
extern "C" long _InterlockedExchangeAdd(long volatile *,long);
extern "C" long _InterlockedExchange(long volatile *,long);

extern "C" __int64 _InterlockedIncrement64(__int64 volatile *);
extern "C" __int64 _InterlockedDecrement64(__int64 volatile *);
extern "C" __int64 _InterlockedExchangeAdd64(__int64 volatile *,__int64);

extern "C" void _ReadBarrier(void);
extern "C" void _WriteBarrier(void);
#pragma intrinsic (_WriteBarrier,_ReadBarrier,_InterlockedExchangeAdd,_InterlockedIncrement,_InterlockedDecrement)


// sWriteBarrier/sReadBarrier are not full memory barriers (_WriteBarrier/_ReadBarrier are only compiler barriers and DO NOT prevent CPU reordering)
// practically this means that still reads can moved ahead of write by the CPU
inline void sWriteBarrier() { _WriteBarrier(); }
inline void sReadBarrier() { _ReadBarrier(); }

// these functions return the NEW value after the operation was done on the memory address

inline sU32 sAtomicAdd(volatile sU32 *p,sInt i) { sU32 e = _InterlockedExchangeAdd((volatile long *)p,i); return e+i; }
inline sU32 sAtomicInc(volatile sU32 *p) { return _InterlockedIncrement((long *)p); }
inline sU32 sAtomicDec(volatile sU32 *p) { return _InterlockedDecrement((long *)p); }
inline sU64 sAtomicAdd(volatile sU64 *p,sInt i) { sU64 e = _InterlockedExchangeAdd64((volatile __int64 *)p,i); return e+i; }
inline sU64 sAtomicInc(volatile sU64 *p) { return _InterlockedIncrement64((__int64 *)p); }
inline sU64 sAtomicDec(volatile sU64 *p) { return _InterlockedDecrement64((__int64 *)p); }
inline sU32 sAtomicSwap(volatile sU32 *p, sU32 i) { return _InterlockedExchange((long*)p,i); }

#endif

#if sCONFIG_COMPILER_GCC

inline void sWriteBarrier() { /*__sync_syncronize();*/ }
inline void sReadBarrier() { /*__sync_syncronize();*/ }
inline sU32 sAtomicAdd(volatile sU32 *p,sU32 i) { return __sync_add_and_fetch(p,i); }
inline sU32 sAtomicInc(volatile sU32 *p) { return __sync_add_and_fetch(p,1); }
inline sU32 sAtomicDec(volatile sU32 *p) { return __sync_add_and_fetch(p,-1); }
inline sU64 sAtomicAdd(volatile sU64 *p,sU64 i) { return __sync_add_and_fetch(p,i); }
inline sU64 sAtomicInc(volatile sU64 *p) { return __sync_add_and_fetch(p,1); }
inline sU64 sAtomicDec(volatile sU64 *p) { return __sync_add_and_fetch(p,-1); }
inline sU32 sAtomicSwap(volatile sU32 *p, sU32 val) { return __sync_lock_test_and_set(p,val); }   // full swap supported?

#endif



#if sCONFIG_COMPILER_CWCC

inline void sWriteBarrier() {}
inline void sReadBarrier() {}

sU32 sAtomicAdd(volatile sU32 *p,sU32 i);
sU32 sAtomicInc(volatile sU32 *p);
sU32 sAtomicDec(volatile sU32 *p);
sU64 sAtomicAdd(volatile sU64 *p,sU64 i);
sU64 sAtomicInc(volatile sU64 *p);
sU64 sAtomicDec(volatile sU64 *p);
sU32 sAtomicSwap(volatile sU32 *p,sU32 val);

#endif


#if sCONFIG_COMPILER_SNC

inline void sWriteBarrier() { __builtin_fence(); }
inline void sReadBarrier() { __builtin_fence(); }
inline sU32 sAtomicAdd(volatile sU32 *p,sU32 i) { return __builtin_cellAtomicAdd32((sU32*)p,i) + i; }
inline sU32 sAtomicInc(volatile sU32 *p) { return __builtin_cellAtomicAdd32((sU32*)p,1) + 1; }
inline sU32 sAtomicDec(volatile sU32 *p) { return __builtin_cellAtomicAdd32((sU32*)p,~0u) - 1; }
inline sU64 sAtomicAdd(volatile sU64 *p,sU64 i) { return __builtin_cellAtomicAdd64((sU64*)p,i) + i; }
inline sU64 sAtomicInc(volatile sU64 *p) { return __builtin_cellAtomicAdd64((sU64*)p,1) + 1; }
inline sU64 sAtomicDec(volatile sU64 *p) { return __builtin_cellAtomicAdd64((sU64*)p,~0u) - 1; }

static inline sU32 sAtomicSwap(volatile sU32 *p, sU32 val)
{
  sU32 prev;
  do prev = __builtin_cellAtomicLockLine32((sU32*)p); while(!__builtin_cellAtomicStoreConditional32((sU32*)p,val));
  return prev;
}

#endif

#if sCONFIG_COMPILER_ARM

// technically, 3ds is a dualcore. But since we can only use one, we don't really need atomics

inline void sWriteBarrier() { __force_stores(); }
inline void sReadBarrier() { __memory_changed(); }
inline sU32 sAtomicAdd(volatile sU32 *p,sU32 i) { return *p+i; }
inline sU32 sAtomicInc(volatile sU32 *p) { return *p+1; }
inline sU32 sAtomicDec(volatile sU32 *p) { return *p-1; }
inline sU64 sAtomicAdd(volatile sU64 *p,sU64 i) { return *p+i; }
inline sU64 sAtomicInc(volatile sU64 *p) { return *p+1; }
inline sU64 sAtomicDec(volatile sU64 *p) { return *p-1; }
inline sU32 sAtomicSwap(volatile sU32 *p, sU32 val) { sU32 i = *p; *p = val; return i; }   // full swap supported?

#endif

/****************************************************************************/

// there may be only one cosumer thread and one producer thread

template <class T,int max> class sLocklessQueue
{
  T * volatile Data[max];
  volatile sInt Write;
  volatile sInt Read;
public:
  sLocklessQueue()   { Write=0;Read=0;sVERIFY(sIsPower2(max)); }
  ~sLocklessQueue()  {}

  // may be called by producer

  sBool IsFull()      { return Write >= Read+max; }
  void AddTail(T *e)  { sInt i=Write; sVERIFY(i < Read+max); Data[i&(max-1)] = e; sWriteBarrier(); Write=i+1; }

  // may be called by consumer

  sBool IsEmpty()     { return Read >= Write; }
  T *RemHead()        { sInt i=Read; if(i>=Write) return 0; T *e=Data[i&(max-1)]; sWriteBarrier(); Read=i+1; return e; }

  // this is an approximation!

  sInt GetUsed()      { return Write-Read; }
};

/****************************************************************************/

// and here is a locking queue, with no restrictions, does not work on pointers, but values!


template <class T,int max> class sLockQueue
{
  T Data[max];
  sInt Write;
  sInt Read;

  typedef sScopeLock ScopeLock;
  sThreadLock Lock;

public:
  sLockQueue()   { Write=0;Read=0;sVERIFY(sIsPower2(max)); }
  ~sLockQueue()  {}

  // may be called by producer
  sBool IsFull()      { ScopeLock s(&Lock); return Write >= Read+max; }
  sBool AddTail(const T &e)  { ScopeLock s(&Lock); if(Write >= Read+max) return 0; Data[Write&(max-1)] = e; Write++; return 1; }

  // may be called by consumer

  sBool IsEmpty()     { ScopeLock s(&Lock); return Read >= Write; }
  sBool RemHead(T &e) { ScopeLock s(&Lock); if(Read>=Write) return 0; e=Data[Read&(max-1)]; Read++; return 1; }
  const T& Front()    { ScopeLock s(&Lock); sVERIFY(!IsEmpty()); return Data[Read&(max-1)]; } // WARNING: may break when class is used by more than one consumer!

  // this is an approximation!

  sInt GetUsed()      { ScopeLock s(&Lock); return Write-Read; }
};

/****************************************************************************/
/***                                                                      ***/
/***   All button codes                                                   ***/
/***                                                                      ***/
/****************************************************************************/

enum sInputKeys
{
  // keyboard

  sKEY_BACKSPACE    = 8,              // traditional ascii
  sKEY_TAB          = 9,
  sKEY_ENTER        = 10,
  sKEY_ESCAPE       = 27,
  sKEY_SPACE        = 32,

  sKEY_UP           = 0x0000e000,     // cursor block
  sKEY_DOWN         = 0x0000e001,
  sKEY_LEFT         = 0x0000e002,
  sKEY_RIGHT        = 0x0000e003,
  sKEY_PAGEUP       = 0x0000e004,
  sKEY_PAGEDOWN     = 0x0000e005,
  sKEY_HOME         = 0x0000e006,
  sKEY_END          = 0x0000e007,
  sKEY_INSERT       = 0x0000e008,
  sKEY_DELETE       = 0x0000e009,

  sKEY_PAUSE        = 0x0000e010,      // Pause/Break key
  sKEY_SCROLL       = 0x0000e011,      // ScrollLock key
  sKEY_NUMLOCK      = 0x0000e012,      // NumLock key
  sKEY_PRINT        = 0x0000e013,
  sKEY_WINL         = 0x0000e014,      // left windows key
  sKEY_WINR         = 0x0000e015,      // right windows key
  sKEY_WINM         = 0x0000e016,      // windows menu key
  sKEY_SHIFT        = 0x0000e017,      // a shift key was pressed
  sKEY_CTRL         = 0x0000e018,      // a ctrl key was pressed
  sKEY_CAPS         = 0x0000e019,      // caps lock key was pressed

  sKEY_F1           = 0x0000e020,      // function keys
  sKEY_F2           = 0x0000e021,
  sKEY_F3           = 0x0000e022,
  sKEY_F4           = 0x0000e023,
  sKEY_F5           = 0x0000e024,
  sKEY_F6           = 0x0000e025,
  sKEY_F7           = 0x0000e026,
  sKEY_F8           = 0x0000e027,
  sKEY_F9           = 0x0000e028,
  sKEY_F10          = 0x0000e029,
  sKEY_F11          = 0x0000e02a,
  sKEY_F12          = 0x0000e02b,

  sKEY_KEYBOARD_MAX = 0x0000e0ff,      // there must be no real keys beyond this point in page 0x0000e000

  // mouse

  sKEY_LMB          = 0x0000e100,
  sKEY_RMB          = 0x0000e101,
  sKEY_MMB          = 0x0000e102,
  sKEY_X1MB         = 0x0000e103,
  sKEY_X2MB         = 0x0000e104,
  sKEY_LASTMB       = 0x0000e107,
  sKEY_HOVER        = 0x0000e108,
  sKEY_MOUSEMOVE    = 0x0000e180,
  sKEY_WHEELUP      = 0x0000e181,
  sKEY_WHEELDOWN    = 0x0000e182,
  sKEY_MOUSETWIST   = 0x0000e183,
  sKEY_MOUSEHARD    = 0x0000e185,

  sKEY_DROPFILES    = 0x0000e184,       // user drag&dropped a file ?? keycode ??

  sKEY_JOYPADXBOX_A       = 0x0000e200,
  sKEY_JOYPADXBOX_B       = 0x0000e201,
  sKEY_JOYPADXBOX_X       = 0x0000e202,
  sKEY_JOYPADXBOX_Y       = 0x0000e203,
  sKEY_JOYPADXBOX_LT      = 0x0000e204,
  sKEY_JOYPADXBOX_LB      = 0x0000e205,
  sKEY_JOYPADXBOX_THUMBL  = 0x0000e206,
  sKEY_JOYPADXBOX_RT      = 0x0000e207,
  sKEY_JOYPADXBOX_RB      = 0x0000e208,
  sKEY_JOYPADXBOX_THUMBR  = 0x0000e209,
  sKEY_JOYPADXBOX_LEFT    = 0x0000e20a,
  sKEY_JOYPADXBOX_RIGHT   = 0x0000e20b,
  sKEY_JOYPADXBOX_UP      = 0x0000e20c,
  sKEY_JOYPADXBOX_DOWN    = 0x0000e20d,
  sKEY_JOYPADXBOX_START   = 0x0000e20e,
  sKEY_JOYPADXBOX_BACK    = 0x0000e20f,

  // input2_devices
  sKEY_INPUT2STATUS = 0x0000e400,
  
  // qualifiers

  sKEYQ_SHIFTL      = 0x00010000, // qualifiers
  sKEYQ_SHIFTR      = 0x00020000,
  sKEYQ_CAPS        = 0x00040000,
  sKEYQ_CTRLL       = 0x00080000,
  sKEYQ_CTRLR       = 0x00100000,
  sKEYQ_ALT         = 0x00200000,
  sKEYQ_ALTGR       = 0x00400000,
  sKEYQ_NUM         = 0x20000000, // from numeric keypad
  sKEYQ_REPEAT      = 0x40000000, // this is a repeated key
  sKEYQ_BREAK       = 0x80000000, // this is a break-key

  sKEYQ_SHIFT       = 0x00030000, // do not treat caps as a shift key!
  sKEYQ_CTRL        = 0x00180000,
  sKEYQ_MASK        = 0x0000ffff,
};

enum
{
  sJOYPAD_COUNT = 8,
};


// for easier platform-coding of the joypads
enum
{
  sKEY_JOYPAD_A  = sKEY_JOYPADXBOX_A,
  sKEY_JOYPAD_B  = sKEY_JOYPADXBOX_B,
  sKEY_JOYPAD_X  = sKEY_JOYPADXBOX_X,
  sKEY_JOYPAD_Y  = sKEY_JOYPADXBOX_Y,
  sKEY_JOYPAD_L1 = sKEY_JOYPADXBOX_LT,
  sKEY_JOYPAD_L2 = sKEY_JOYPADXBOX_LB,
  sKEY_JOYPAD_L3 = sKEY_JOYPADXBOX_THUMBL,
  sKEY_JOYPAD_R1 = sKEY_JOYPADXBOX_RT,
  sKEY_JOYPAD_R2 = sKEY_JOYPADXBOX_RB,
  sKEY_JOYPAD_R3 = sKEY_JOYPADXBOX_THUMBR,
  sKEY_JOYPAD_LEFT = sKEY_JOYPADXBOX_LEFT,
  sKEY_JOYPAD_RIGHT = sKEY_JOYPADXBOX_RIGHT,
  sKEY_JOYPAD_UP = sKEY_JOYPADXBOX_UP,
  sKEY_JOYPAD_DOWN = sKEY_JOYPADXBOX_DOWN,
  sKEY_JOYPAD_START = sKEY_JOYPADXBOX_START,
  sKEY_JOYPAD_SELECT = sKEY_JOYPADXBOX_BACK,
};

sU32 sGetKeyQualifier();                    // get shift-state sKEYQ
void sInput2ClearQueue();
void sInput2SendEvent(const sInput2Event& event);
sBool sInput2PopEvent(sInput2Event& event, sInt time);
sBool sInput2PeekEvent(sInput2Event& event, sInt time);

sInt sMakeUnshiftedKey(sInt ascii);
void sSetMouse(sInt x,sInt y);
void sSetMouseCenter();
sBool sCheckBreakKey();

sBool sInput2IsKeyboardKey(sInt key);

/****************************************************************************/
/***                                                                      ***/
/***   File abstraction                                                   ***/
/***                                                                      ***/
/****************************************************************************/

enum sFileAccess
{
  sFA_READ = 1,                   // read mostly sequential
  sFA_READRANDOM,                 // read in random order
  sFA_WRITE,                      // create new file, possibly overwriting old data
  sFA_WRITEAPPEND,                // append to existing file or create new file
  sFA_READWRITE,                  // open file for read and write
  sFA_READDRM,                    // open encrypted DRM file for read
};

enum sFilePriorityFlags
{
  sFP_BACKGROUND = 0,
  sFP_NORMAL = 1,
  sFP_REALTIME = 2,
};

typedef sInt sFileReadHandle;

class sFile
{
protected:
  sU8 *MapFakeMem;                            // memory buffer for fake file mapping in MapAll()
public:
  sFile();
  virtual ~sFile();
  virtual sBool Close();                      // only needed to get error code. the error code is accumulative!
  virtual sBool Read(void *data,sDInt size);  // read bytes. may change mapping.
  virtual sBool Write(const void *data,sDInt size); // write bytes, may change mapping.
  virtual sU8 *Map(sS64 offset,sDInt size);  // map file (independent from SetOffset()) (may fail)  
  virtual sBool SetOffset(sS64 offset);       // seek to offset
  virtual sS64 GetOffset();                   // get offset
  virtual sBool SetSize(sS64);                // change size of file on disk
  virtual sS64 GetSize();                     // get size

  // asynchronous interface. expect this to be unimplemented for certain file handlers
  // normal on disk files and uncompressed files from a pack file should work tho.
  virtual sFileReadHandle BeginRead(sS64 offset,sDInt size,void *destbuffer=0, sFilePriorityFlags prio=sFP_NORMAL);  // begin reading
  virtual sBool DataAvailable(sFileReadHandle handle);  // data valid?
  virtual void *GetData(sFileReadHandle handle);  // access data (only valid if you didn't specify the buffer yourself!)
  virtual void EndRead(sFileReadHandle handle);   // the data buffer may be reused now

  // PLATFORM DEPENDENT: get physical position of part of file.
  virtual sU64 GetPhysicalPosition(sS64 offset);   

  // helper functions
  sU8 *MapAll();                              // map whole file. if mapping is not supported, copy by hand.
  sInt CopyFrom(sFile *f);                    // copies from f into itself
  sInt CopyFrom(sFile *f, sS64 max);          // copies max bytes from f into itself
};

class sFailsafeFile : public sFile // file that stores to a different name and renames when finished. only for writing!
{
  sFile *Host;
  sString<2048> CurrentName;
  sString<2048> TargetName;

public:
  sFailsafeFile(sFile *host,const sChar *curName,const sChar *tgtName);
  virtual ~sFailsafeFile();
  virtual sBool Close();
  virtual sBool Read(void *data,sDInt size)                 { sVERIFYFALSE; return 0; }
  virtual sBool Write(const void *data,sDInt size)          { return Host->Write(data,size); }
  virtual sU8 *Map(sS64 offset,sDInt size)                  { return Host->Map(offset,size); }
  virtual sBool SetOffset(sS64 offset)                      { return Host->SetOffset(offset); }
  virtual sS64 GetOffset()                                  { return Host->GetOffset(); }
  virtual sBool SetSize(sS64 newSize)                       { return Host->SetSize(newSize); }
  virtual sS64 GetSize()                                    { return Host->GetSize(); }
};

class sFileHandler
{
public:
  virtual ~sFileHandler();
  virtual sFile *Create(const sChar *name,sFileAccess access);
  virtual sBool Exists(const sChar *name);
};

void sAddFileHandler(sFileHandler *);
void sRemFileHandler(sFileHandler *);

class sFile *sCreateFile(const sChar *name,sFileAccess access=sFA_READ);
class sFile *sCreateFailsafeFile(const sChar *name,sFileAccess access=sFA_READ);

class sFile *sCreateMemFile(const void *data,sDInt size,sBool owndata=1,sInt blocksize=1);  // readonly
class sFile *sCreateMemFile(void *data,sDInt size,sBool owndata=1,sInt blocksize=1,sFileAccess access=sFA_READ);
class sFile *sCreateMemFile(const sChar *name);

class sFile *sCreateGrowMemFile();

/****************************************************************************/
// for calculating md5 of serialization without writing it to hardisk
class sCalcMD5File : public sFile
{
  sU8 TempBuf[64];
  sInt TempCount;
  sInt TotalCount;
public:
  sChecksumMD5 Checksum;

  sCalcMD5File();
  ~sCalcMD5File();
  sBool Close();
  sBool Read(void *data,sDInt size);
  sBool Write(const void *data,sDInt size);
  sU8 *Map(sS64 offset,sDInt size);
  sBool SetOffset(sS64 offset);
  sS64 GetOffset();
  sBool SetSize(sS64);
  sS64 GetSize();
};

/****************************************************************************/
/***                                                                      ***/
/***   File Operation                                                     ***/
/***                                                                      ***/
/****************************************************************************/

sBool sCheckFile(const sChar *name);
sU8 *sLoadFile(const sChar *name);
sU8 *sLoadFile(const sChar *name,sDInt &size);
sBool sSaveFile(const sChar *name,const void *data,sDInt bytes);
sBool sSaveFileFailsafe(const sChar *name,const void *data,sDInt bytes);
sChar *sLoadText(const sChar *name);
//sStream *sOpenText(const sChar *name);
sBool sSaveTextAnsi(const sChar *name,const sChar *data);
sBool sSaveTextUnicode(const sChar *name,const sChar *data);
sBool sSaveTextUTF8(const sChar *name,const sChar *data);
sBool sSaveTextAnsiIfDifferent(const sChar *name,const sChar *data); // for ASC/makeproject etc.
sBool sBackupFile(const sChar *name);

sBool sFilesEqual(const sChar *name1,const sChar *name2);
sBool sFileCalcMD5(const sChar *name, sChecksumMD5 &md5);

#if !sSTRIPPED && sPLATFORM==sPLAT_WINDOWS
// enable a rough emulation of seek time for async file io with given seek time seektime_in_ms>0
// seektime_in_ms==0 : disables seek time emulation
// always returns the set seek time (or last set, when not changing seek time with seektime_in_ms<0)
// (dvd drives have about 120ms average seek time)
sInt sFileSeekTimeEmulation(sInt seektime_in_ms=-1);
#else
sINLINE sInt sFileSeekTimeEmulation(sInt seektime_in_ms=-1) { return 0; }
#endif

/****************************************************************************/

void sSetProjectDir(const sChar *name);

struct sDirEntry
{
  sString<256> Name;
  sInt Size;
  sInt Flags;
  sU64 LastWriteTime;
};

enum sDirEntryFlags
{
  sDEF_DIR = 0x0001,
  sDEF_WRITEPROTECT = 0x0002,
  sDEF_EXISTS = 0x0004,
};

sBool sCopyFile(const sChar *source,const sChar *dest,sBool failifexists=0);
sBool sCopyFileFailsafe(const sChar *source,const sChar *dest,sBool failifexists=0);
sBool sRenameFile(const sChar *source,const sChar *dest, sBool overwrite=sFALSE);
sBool sFindFile(sChar *foundname, sInt foundnamesize, const sChar *path,const sChar *pattern = 0);
#if sPLATFORM==sPLAT_WINDOWS || sPLATFORM==sPLAT_LINUX 
sBool sLoadDir(sArray<sDirEntry> &list,const sChar *path,const sChar *pattern=0);
sDateAndTime sFromFileTime(sU64 fileTime); // OS specific times, e.g. LastWriteTime
sU64 sToFileTime(sDateAndTime time);
sBool sSetFileTime(const sChar *name, sU64 lastwritetime);
#endif
sBool sChangeDir(const sChar *name);
void  sGetCurrentDir(const sStringDesc &str);
void  sGetTempDir(const sStringDesc &str);
void  sGetAppDataDir(const sStringDesc &str);
sBool sGetFileInfo(const sChar *name,sDirEntry *);
sBool sGetDiskSizeInfo(const sChar *path, sS64 &availablesize, sS64 &totalsize);
sBool sMakeDir(const sChar *);          // make one directory
sBool sMakeDirAll(const sChar *);       // make all directories. may fail with strage paths.
sBool sCheckDir(const sChar *);
sBool sDeleteFile(const sChar *);
sBool sDeleteDir(const sChar *name);
sBool sGetFileWriteProtect(const sChar *,sBool &prot);
sBool sSetFileWriteProtect(const sChar *,sBool prot);
sBool sIsBelowCurrentDir(const sChar *relativePath);

/****************************************************************************/
/***                                                                      ***/
/***   Video writer                                                       ***/
/***                                                                      ***/
/****************************************************************************/

// This is more of a "util" thing, really, but the preferred way to write
// videos is system dependent, so here goes.
class sVideoWriter
{
public:
  virtual ~sVideoWriter();

  // Writes a frame of video (ARGB8888)
  virtual sBool WriteFrame(const sU32 *data) = 0;
};

sVideoWriter *sCreateVideoWriter(const sChar *filename,const sChar *codec,sF32 fps,sInt xRes,sInt yRes);

/****************************************************************************/
/***                                                                      ***/
/***   User Management                                                    ***/
/***                                                                      ***/
/****************************************************************************/

sBool sGetUserName(const sStringDesc &dest, sInt joypadId);

/****************************************************************************/

#endif   //ALTONA_BASE_SYSTEM



/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) 2005-2006 by Dierk Ohlerich                          ***/
/***   all rights reserverd                                               ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "base/types.hpp"
#include "base/types2.hpp"
#include "base/system.hpp"
#include "base/system_ios.hpp"
#include "base/graphics.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>

//#include "gltest.h"

static const sInt DebugHeapSize = 16*1024*1024;

void sInitThread();
void sExitThread();
void PreInitGFX(sInt &flags,sInt &xs,sInt &ys);
void InitGFX(sInt flags,sInt xs,sInt ys);
void ExitGFX();

static sInt IOS_TimeMs;
static sU64 IOS_TimeUs;

IOS_TouchData IOS_Touches[IOS_MaxTouches];
int IOS_TouchCount;

/****************************************************************************/

void IOSInit1()
{
//  setlocale(LC_ALL,"");
  setbuf(stdout,0);
  
  sDPrint(L"***** A\n");
  sInitThread();
  sInitMem0();
  sSetRunlevel(0x80);
}

void IOSInit2()
{
  sDPrint(L"***** B\n");
}

void IOSInit3()
{
  sMain();
  sSetRunlevel(0x100);
}

void IOSExit()
{
  sDPrint(L"***** X\n");
  sSetRunlevel(0x00);
  sDPrint(L"***** Y\n");
  fflush(stdout);
  sExitThread();
}

void IOSRender(int ms,unsigned long long us)
{
  IOS_TimeMs = ms;
  IOS_TimeUs = us;
  if(sRender3DBegin())
  {
    sGetApp()->OnPaint3D();
    sRender3DEnd(1);
  }
  
  sDPrintF(L"touch");
  for(sInt i=0;i<IOS_TouchCount;i++)
    sDPrintF(L" | %3d %3d %1d",IOS_Touches[i].x,IOS_Touches[i].y,IOS_Touches[i].c);
  sDPrintF(L"\n");
}

void sInit(sInt flags,sInt xs,sInt ys)
{
  PreInitGFX(flags,xs,ys);
  InitGFX(flags,xs,ys);
}

sBool sHasWindowFocus()
{
  return 1;
}

void sSetWindowName(const sChar *name)
{
}

/****************************************************************************/

void sDPrint(const sChar *str)
{
  char buffer[1024];
  sInt i=0;
  while(*str)
  {
    while(*str && i<1023)
      buffer[i++] = *str++;
    buffer[i++] = 0;
    fwrite(buffer,i,1,stdout);
  }
}

void sPrint(const sChar *str)
{
  char buffer[1024];
  sInt i=0;
  while(*str)
  {
    while(*str && i<1023)
      buffer[i++] = *str++;
    buffer[i++] = 0;
    fwrite(buffer,i,1,stdout);
  }
}

void sFatalImpl(const sChar *str)
{
  sDPrint(str);
  sDPrint(L"\n");
  exit(1);
}

void sTriggerEvent(sInt e)
{
}

/****************************************************************************/
/***                                                                      ***/
/***   Heap                                                               ***/
/***                                                                      ***/
/****************************************************************************/

sU8 *sMainHeapBase;
sU8 *sDebugHeapBase;
class sMemoryHeap sMainHeap;
class sMemoryHeap sDebugHeap;

class sLibcHeap_ : public sMemoryHandler
{
public:
  void *Alloc(sPtr size,sInt align,sInt flags)
  {
    //    sAtomicAdd(&sMemoryUsed, (sDInt)size);
    void *ptr;
//    align = sMax<sInt>(align,sizeof(void*));
//    sVERIFY(align<=4);
    ptr = malloc(size);
    
    return ptr;
  }
  sBool Free(void *ptr)
  {
    //    sAtomicAdd(&sMemoryUsed, -(sDInt)size);
    free(ptr);
    return 1;
  }
} sLibcHeap;


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
      sInt size = DebugHeapSize;
      sDebugHeapBase = (sU8 *) malloc(size);
      sVERIFY(sDebugHeapBase);
      sDebugHeap.Init(sDebugHeapBase,size);
      sRegisterMemHandler(sAMF_DEBUG,&sDebugHeap);
    }
    else
      sRegisterMemHandler(sAMF_DEBUG,&sLibcHeap);
  }
  if((flags & sIMF_NORTL) && sMemoryInitSize>0)
  {
    sMainHeapBase = (sU8 *) malloc(sMemoryInitSize);
    sVERIFY(sMainHeapBase);
    if(flags & sIMF_CLEAR)
      sSetMem(sMainHeapBase,0x77,sMemoryInitSize);
    sMainHeap.Init(sMainHeapBase,sMemoryInitSize);
    sMainHeap.SetDebug((flags & sIMF_CLEAR)!=0,0);
    sRegisterMemHandler(sAMF_HEAP,&sMainHeap);
  }
  else
  {
    sRegisterMemHandler(sAMF_HEAP,&sLibcHeap);
  }
}

void sExitMem1()
{
  if(sDebugHeapBase)  free(sDebugHeapBase);
  if(sMainHeapBase)   free(sMainHeapBase);
  
  sUnregisterMemHandler(sAMF_DEBUG);
  sUnregisterMemHandler(sAMF_HEAP);
}

/****************************************************************************/
/***                                                                      ***/
/***   Multithreading (ripped from linux)                                 ***/
/***                                                                      ***/
/****************************************************************************/

static sBool sThreadInitialized = sFALSE;
static pthread_key_t sThreadKey, sContextKey;
static sThreadContext sEmergencyThreadContext;

/****************************************************************************/

sThread *sGetCurrentThread()
{
  return (sThread *) pthread_getspecific(sThreadKey);
}
/*
sInt sGetCurrentThreadId()
{
  return pthread_self();
}
*/
/****************************************************************************/


sThreadContext *sGetThreadContext()
{
  sThreadContext *ctx = 0;
  if(sThreadInitialized)
    ctx = (sThreadContext*) pthread_getspecific(sContextKey);
  if(!ctx)
    ctx = &sEmergencyThreadContext;
  
  return ctx;
}

/****************************************************************************/

void sSleep(sInt ms)
{
  usleep(ms*1000); //uSleep need microseconds
}

sInt sGetCPUCount()
{
  return 1;// 	sysconf(_SC_NPROCESSORS_CONF);
}

/****************************************************************************/


void * sSTDCALL sThreadTrunk_pthread(void *ptr)
{
  sThread *th = (sThread *) ptr;
  
  pthread_setspecific(sThreadKey, th);
  pthread_setspecific(sContextKey, th->Context);
  
  // wait for the ThreadId to be set in the mainthread
  while (!th->ThreadId)
    sSleep(10);
  
  pthread_t self = pthread_self();
  sLogF(L"sys",L"New sThread started. 0x%x, id is 0x%x\n", self, th->ThreadId);
  
  sVERIFY( pthread_equal( self,(pthread_t) th->ThreadId ) );
  
  th->Code(th,th->Userdata);
    
  return 0;
}

/****************************************************************************/

sThread::sThread(void (*code)(sThread *,void *),sInt pri,sInt stacksize,void *userdata, sInt flags/*=0*/)
{
  sVERIFY(sizeof(pthread_t)==sCONFIG_PTHREAD_T_SIZE);
  
  
  TerminateFlag = 0;
  Code = code;
  Userdata = userdata;
  Stack = 0;
  Context = sCreateThreadContext(this);
  ThreadHandle = new pthread_t;
  ThreadId     = 0;
  
  // windows: ThreadHandle = CreateThread(0,stacksize,sThreadTrunk,this,0,(ULONG *)&ThreadId);
  
  int result = pthread_create((pthread_t*)ThreadHandle, sNULL, sThreadTrunk_pthread, this);
  sVERIFY(result==0);
  
  ThreadId = sU64(*(pthread_t*)ThreadHandle);
  
  // clone(sThreadTrunk, StackMemory + stacksize - 1,  CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_VM|CLONE_THREAD ,this);
  
  //  if(pri>0)
  //  {
  //    SetPriorityClass(ThreadHandle,HIGH_PRIORITY_CLASS);
  //    SetThreadPriority(ThreadHandle,THREAD_PRIORITY_TIME_CRITICAL);
  //  }
  //  if(pri<0)
  //  {
  ////    SetPriorityClass(ThreadHandle,BELOW_NORMAL_PRIORITY_CLASS);
  //    SetThreadPriority(ThreadHandle,THREAD_PRIORITY_BELOW_NORMAL);
  //  }
}

/****************************************************************************/

sThread::~sThread()
{
  if(TerminateFlag==0)
    Terminate();
  if(TerminateFlag==1)
    pthread_join(*(pthread_t*)ThreadHandle, sNULL);
  
  delete (pthread_t*)ThreadHandle;
  delete Context;
}

/****************************************************************************/

void sInitThread()
{
  pthread_key_create(&sThreadKey, 0);
  pthread_key_create(&sContextKey, 0);
  
  sClear(sEmergencyThreadContext);
  sEmergencyThreadContext.ThreadName = L"MainThread";
  sEmergencyThreadContext.MemTypeStack[0] = sAMF_HEAP;
  pthread_setspecific(sContextKey, &sEmergencyThreadContext);
  
  sThreadInitialized = sTRUE;
}

/****************************************************************************/

void sExitThread()
{
  pthread_key_delete(sThreadKey);
  pthread_key_delete(sContextKey);
}

/****************************************************************************/

sThreadLock::sThreadLock()
{
  CriticalSection = new pthread_mutex_t;
  
  pthread_mutexattr_t mattr;
  pthread_mutexattr_init(&mattr);
  pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init((pthread_mutex_t*) CriticalSection, &mattr);  
}

/****************************************************************************/

sThreadLock::~sThreadLock()
{
  delete (pthread_mutex_t*)CriticalSection;
}

/****************************************************************************/

void sThreadLock::Lock()
{
  pthread_mutex_lock((pthread_mutex_t*) CriticalSection);
}

/****************************************************************************/

sBool sThreadLock::TryLock()
{
  return pthread_mutex_trylock((pthread_mutex_t*) CriticalSection) == 0;
}

/****************************************************************************/

void sThreadLock::Unlock()
{
  pthread_mutex_unlock((pthread_mutex_t*) CriticalSection);
  
}

/****************************************************************************/

// THIS IS UNTESTED, MAY CONTAIN BUGS AND IS DEFINITELY INEFFICIENT.
// It's also outright dangerous on anything that's not x86!

sThreadEvent::sThreadEvent(sBool manual)
{
  Signaled = 0;
  ManualReset = manual;
  
  sLogF(L"system",L"WARNING! This program uses sThreadEvents on Linux. They are currently UNTESTED, probably DANGEROUS and most likely also INEFFICIENT.\n");
}

sThreadEvent::~sThreadEvent()
{
}

sBool sThreadEvent::Wait(sInt timeout)
{
  if(ManualReset) // manual reset
  {
    if(timeout == -1) // okay, just wait forever
    {
      while(!Signaled)
        sched_yield();
      
      return sTRUE;
    }
    
    sInt start = sGetTime();
    sU32 tDiff;
    sBool okay = sFALSE;
    do
    {
      okay = Signaled;
      tDiff = sU32(sGetTime()-start);
      if(!okay && sU32(timeout) > tDiff) // not signaled, not yet timed out
        sched_yield();
    }
    while(!okay && sU32(timeout) > tDiff);
    
    return okay;
  }
  else // automatic reset
  {
    if(timeout == -1) // okay, just wait forever
    {
      sU32 gotit;
      while((gotit = sAtomicSwap(&Signaled,0)) == 0)
        sched_yield();
      
      return sTRUE;
    }
    
    sInt start = sGetTime();
    sU32 tDiff,gotit;
    do
    {
      gotit = sAtomicSwap(&Signaled,0);
      tDiff = sU32(sGetTime()-start);
      if(!gotit && sU32(timeout) > tDiff) // haven't got it, not timed out
        sched_yield();
    }
    while(!gotit && sU32(timeout) > tDiff);
    
    return gotit == 1;
  }
}

void sThreadEvent::Signal()
{
  Signaled = 1;
}

void sThreadEvent::Reset()
{
  Signaled = 0;
}



/****************************************************************************/
/***                                                                      ***/
/***   Not Implemented                                                    ***/
/***                                                                      ***/
/****************************************************************************/

sU64 sGetTimeUS()
{
  return IOS_TimeUs;
}

sInt sGetTime()
{
  return IOS_TimeMs;
}

sInt sGetJoypadCount()
{
  sFatal(L"not implemented");
  return 0;
}

sJoypad *sGetJoypad(sInt id)
{
  sFatal(L"not implemented");
  return 0;
}

void sGetMouse(sMouseData&,int,int)
{
}

/****************************************************************************/
/***                                                                      ***/
/***   File Operation                                                     ***/
/***                                                                      ***/
/****************************************************************************/
/*
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
*/
/****************************************************************************/

void sSetProjectDir(const sChar *name) 
{ sFatal(L"not implemented"); }
sBool sCopyFile(const sChar *source,const sChar *dest,sBool failifexists)
{ sFatal(L"not implemented"); return 0; }
sBool sRenameFile(const sChar *source,const sChar *dest, sBool overwrite)
{ sFatal(L"not implemented"); return 0; }
sBool sFindFile(sChar *foundname, sInt foundnamesize, const sChar *path,const sChar *pattern)
{ sFatal(L"not implemented"); return 0; }
sBool sLoadDir(sArray<sDirEntry> &list,const sChar *path,const sChar *pattern)
{ sFatal(L"not implemented"); return 0; }
sDateAndTime sFromFileTime(sU64 fileTime) // OS specific times, e.g. LastWriteTime
{ sFatal(L"not implemented"); return sDateAndTime(); }
sU64 sToFileTime(sDateAndTime time)
{ sFatal(L"not implemented"); return 0; }
sBool sSetFileTime(const sChar *name, sU64 lastwritetime)
{ sFatal(L"not implemented"); return 0; }
sBool sChangeDir(const sChar *name)
{ sFatal(L"not implemented"); return 0; }
void  sGetCurrentDir(const sStringDesc &str)
{ sFatal(L"not implemented"); }
void  sGetTempDir(const sStringDesc &str)
{ sFatal(L"not implemented"); }
sBool sGetFileInfo(const sChar *name,sDirEntry *)
{ sFatal(L"not implemented"); return 0; }
sBool sGetDiskSizeInfo(const sChar *path, sS64 &availablesize, sS64 &totalsize)
{ sFatal(L"not implemented"); return 0; }
sBool sMakeDir(const sChar *)          // make one directory
{ sFatal(L"not implemented"); return 0; }
sBool sCheckDir(const sChar *)
{ sFatal(L"not implemented"); return 0; }
sBool sDeleteFile(const sChar *)
{ sFatal(L"not implemented"); return 0; }
sBool sDeleteDir(const sChar *name)
{ sFatal(L"not implemented"); return 0; }
sBool sGetFileWriteProtect(const sChar *,sBool &prot)
{ sFatal(L"not implemented"); return 0; }
sBool sSetFileWriteProtect(const sChar *,sBool prot)
{ sFatal(L"not implemented"); return 0; }
sBool sIsBelowCurrentDir(const sChar *relativePath)
{ sFatal(L"not implemented"); return 0; }


/****************************************************************************/
/***                                                                      ***/
/***   Graphics                                                           ***/
/***                                                                      ***/
/****************************************************************************/


// Written by Fabian "ryg" Giesen.
// I hereby place this code in the public domain.

#include "_types.hpp"
#include "_startconsole.hpp"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <crtdbg.h>
#include <malloc.h>

/****************************************************************************/
/***                                                                      ***/
/***   System Initialisation                                              ***/
/***                                                                      ***/
/****************************************************************************/

sSystem_ *sSystem;
class sBroker_ *sBroker;

/****************************************************************************/
/****************************************************************************/

sInt MemoryUsedCount;

#if _DEBUG
#if !sINTRO
#undef new
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

#endif

/****************************************************************************/

sInt main(sInt argc,sChar **argv)
{
  sInt ret;

  sSystem = new sSystem_;
  ret = sAppMain(argc,argv);
  delete sSystem;

  return ret;
}

/****************************************************************************/
/***                                                                      ***/
/***   Init/Exit/Debug                                                    ***/
/***                                                                      ***/
/****************************************************************************/

void sSystem_::Log(sChar *s)
{
  OutputDebugString(s);
}

/****************************************************************************/

sNORETURN void sSystem_::Abort(sChar *msg)
{
  _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)&~(_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF));
  if(msg)
    PrintF("\afatal error: %s\n",msg);
  ExitProcess(0);
}

/****************************************************************************/

void sSystem_::Tag()
{
}

/****************************************************************************/
/***                                                                      ***/
/***   Console IO                                                         ***/
/***                                                        B              ***/
/****************************************************************************/

void sSystem_::PrintF(sChar *format,...)
{
  sChar buffer[2048];
  DWORD written;

  sFormatString(buffer,2048,format,&format);
  WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),buffer,
    sGetStringLen(buffer),&written,0);
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

sInt sSystem_::GetTime()
{
  return GetTickCount();
}

/****************************************************************************/

// This code is in the public domain. See LICENSE for details.

#ifndef __fr_debug_h_
#define __fr_debug_h_

#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"

namespace fr
{
	namespace debug
	{
		typedef void (__cdecl *exitProc)(void);
		typedef void (__stdcall *reportErrorProc)(const sChar *errMsg);

#ifdef _DEBUG
		enum allocType
		{
			allocTypeNew=1,
			allocTypeNewA,
			allocTypeMalloc
		};

		struct allocUnit
		{
		  sU32        size, realSize;
		  void        *addr, *realAddr;
		  sChar       file[64];
		  sU32        line;
			allocType		type;
		  sBool       watchDealloc, watchRealloc;
		  allocUnit		*next, *prev;
		};

		struct memStats
		{
		  sU32	totalAlloc, totalRealAlloc;
		  sU32  peakAlloc, peakRealAlloc;
		  sU32  curAlloc, curRealAlloc;
		  sU32  totalAllocUnits, peakAllocUnits, curAllocUnits;
		};

		extern void memSetOwner(const sChar *file, const sU32 line);
		extern void *memAllocate(const sChar *file, const sU32 line, const allocType type, const sU32 size);
		extern void *memReAllocate(const sChar *file, const sU32 line, const allocType type, const sU32 size, void *addr);
		extern void memDeAllocate(const sChar *file, const sU32 line, const allocType type, void *addr);
		extern sU32 getAllocatedMem();
		extern sU32 memSize(void *addr);
#endif
	}

	extern void __cdecl errorExit(const sChar *format, ...);
	extern void __cdecl debugOut(const sChar *format, ...);
	extern void atExit(debug::exitProc exit);
	extern void setReportError(debug::reportErrorProc error);
}

#ifdef _DEBUG

#define FRDASSERT(x)	if(!(x)){__asm{int 3}fr::errorExit("%s(%d) : Debug Assertion failed!\nExpression: %s",__FILE__,__LINE__,#x);}

#else

#define memSize(p)    _msize(p)
#define FRDASSERT(x)	/**/

#endif

#define FRASSERT(x)		if(!(x)){__asm{int 3}fr::errorExit("%s(%d) : Assertion failed!\nExpression: %s",__FILE__,__LINE__,#x);}

#pragma warning (disable: 4291)

#endif

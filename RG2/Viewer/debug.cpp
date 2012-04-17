// This code is in the public domain. See LICENSE for details.

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "debug.h"

struct iExitList
{
	fr::debug::exitProc exit;
  iExitList						*next;
};

static iExitList        *exits=0;

#ifdef _DEBUG

static sChar *atypes[]={"error", "new", "new[]", "malloc"};
static sChar *dtypes[]={"error", "delete", "delete[]", "free"};

#ifndef STRESSTEST
#define ALLOCHASHBITS   12
#define ALLOCHASHSIZE   (1<<ALLOCHASHBITS)
#define ALLOCHASHMASK   (ALLOCHASHSIZE-1)
#define PADDINGSIZE     4
#define RANDOMWIPE      sFALSE
#undef  NULLPOINTERS
#else
#define ALLOCHASHBITS   12
#define ALLOCHASHSIZE   (1<<ALLOCHASHBITS)
#define ALLOCHASHMASK   (ALLOCHASHSIZE-1)
#define PADDINGSIZE     32
#define RANDOMWIPE      sTRUE
#define NULLPOINTERS
#endif

#define MAXALLOC        256L*1048576L

#undef  new
#undef  delete
#undef  malloc
#undef  calloc
#undef  realloc
#undef  free

#define PREFIXPATTERN   0xbaadf00d
#define POSTFIXPATTERN  0xdeadc0de
#define UNUSEDPATTERN   0xfeedface
#define RELEASEDPATTERN 0xdeadbeef

namespace fr
{
	namespace debug
	{
		static allocUnit  *allocHash[ALLOCHASHSIZE];
		static allocUnit  *reservoir=0, **reservoirBuffer=0;
		static sU32       reservoirSize=0;
		static memStats   stats;
		static sChar      *sourceFile;
		static sU32       sourceLine;
		static sU32       randomPattern=0x14f1fea;

#ifdef HOTSPOTS
		struct hotSpot
		{
		  sChar   file[64];
		  sU32    line;
		  sU32    allocs, allocedmem;
		  hotSpot	*next;
		};

		static hotSpot *hotSpots[64];
#endif

		static const sChar *stripFilename(const sChar *name)
		{
		  const sChar *ptr;

		  ptr=strrchr(name, '\\');
		  if (ptr)
		    return ptr+1;

		  ptr=strrchr(name, '/');
		  if (ptr)
		    return ptr+1;

		  return name;
		}

		static const sChar *formatOwner(const sChar *file, const sU32 line)
		{
		  static sChar buf[128];

		  wsprintf(buf, "%s(%d)", file, line);
		  return buf;
		}

		static const sChar *formatSize(const sU32 size)
		{
		  static sChar buf[128];

		  if (size>=1048576)
		    sprintf(buf, "%10d (%.2fmb)", size, (sF32) size/1048576.0f);
		  else if (size>=1024)
		    sprintf(buf, "%10d (%.2fkb)", size, (sF32) size/1024.0f);
		  else
		    sprintf(buf, "%10d", size);

		  return buf;
		}

		static allocUnit *findUnit(void *addr)
		{
		  allocUnit *u;
		  sU32      hashIndex;
		  
		  hashIndex=((sU32) addr>>4) & ALLOCHASHMASK;
		  for (u=allocHash[hashIndex]; u; u=u->next)
		    if (u->addr==addr)
		      return u;

		  return 0;
		}

		static sU32 calcRealSize(const sU32 size)
		{
		  return size+PADDINGSIZE*8;
		}

		static sU32 calcSize(const sU32 realSize)
		{
		  return realSize-PADDINGSIZE*8;
		}

		static void *calcRealAddr(const void *addr)
		{
		  return addr?(void *) ((sChar *) addr-PADDINGSIZE*4):0;
		}

		static void *calcAddr(const void *addr)
		{
		  return addr?(void *) ((sChar *) addr+PADDINGSIZE*4):0;
		}

		static void dwordFill(void *p, sU32 count, sU32 fill)
		{
		  __asm {
		    mov   edi, p
		    mov   ecx, count
		    mov   eax, fill
		    push  ecx
		    cld
		    shr   ecx, 2
		    rep   stosd
		    pop   ecx
		    and   ecx, 3
		restlp:
		    or    ecx, ecx
		    jz    end
		    stosb
		    shr   eax, 8
		    dec   ecx
		    jmp   restlp
		end:
		  }
		}

		static void wipeUnit(allocUnit *u)
		{
      if (u->addr && u->realAddr)
      {
#if RANDOMWIPE==sTRUE
		    randomPattern=(randomPattern*0x31337303)+0x3857337a;

		    dwordFill(u->realAddr, u->realSize, randomPattern);
#else
		    sU32  *pre=(sU32 *) u->realAddr;
		    sU32  *post=(sU32 *) ((sChar *) u->realAddr+u->realSize-PADDINGSIZE*4);
		    sU32  i;

		    for (i=0; i<PADDINGSIZE; i++)
		    {
		      *pre++=PREFIXPATTERN;
		      *post++=POSTFIXPATTERN;
		    }

		    dwordFill(u->addr, u->size, 0);
#endif
      }
		}

#ifdef HOTSPOTS
		static sU32 computeHotspotHash(const sChar *file, const sU32 line)
		{
		  sU32  hash=(line+1)*0xda679fa2;

		  for (sInt i=0; i<4 && file[i]; i++)
		    hash^=(hash*247)^file[i];

		  return (hash&63);
		}

		static void updateHotspot(const sChar *file, const sU32 line, const sU32 alloced)
		{
		  hotSpot *hs;
		  sU32    h;

		  h=computeHotspotHash(file, line);
		  for (hs=hotSpots[h]; hs; hs=hs->next)
		    if (!strncmp(hs->file, file, 63) && hs->line==line)
		      goto found;

		  hs=(hotSpot *) malloc(sizeof(hotSpot));
		  strncpy(hs->file, file, 63); hs->file[63]=0;
		  hs->line=line;
		  hs->allocs=0;
      hs->allocedmem=0;
      hs->next=hotSpots[h];
      hotSpots[h]=hs;
      
found:
      hs->allocs++;
      hs->allocedmem+=alloced;
    }
    
    static void dumpHotspots()
    {
      hotSpot	*hs;
      sU32    i;
      
      fr::debugOut("-<hotspot analysis>---------------------------+-------------+----------------\n");
      fr::debugOut("  Who                                         | Alloc count | Alloc size     \n");
      
      for (i=0; i<64; i++)
        for (hs=hotSpots[i]; hs; hs=hs->next)
          fr::debugOut("%-44s  | %11d | %15d\n", formatOwner(hs->file, hs->line), hs->allocs, hs->allocedmem);
    }
#endif

		void memSetOwner(const sChar *file, const sU32 line)
		{
		  sourceFile=(sChar *) file;
		  sourceLine=line;
		}

		static void dumpAllocs()
		{
		  allocUnit *au;
		  sU32      i;

			fr::debugOut("----------------------------------------------+----------+--------+----------\n");
			fr::debugOut("  Who                                         | Where    |  What  | How Much \n");

		  for (i=0; i<ALLOCHASHSIZE; i++)
		    for (au=allocHash[i]; au; au=au->next)
					fr::debugOut("%-44s :| %p | %-6s | %8d\n", formatOwner(au->file, au->line), au->addr, atypes[au->type], au->size);
		}

		void *memAllocate(const sChar *file, const sU32 line, const allocType type, const sU32 size)
		{
		  allocUnit **temp;
		  allocUnit *au;
		  sU32      i, hashIndex;

		  FRASSERT(size<MAXALLOC);

		  if (!reservoir)
		  {
		    reservoir=(allocUnit *) malloc(sizeof(allocUnit)*256);
		    if (!reservoir)
					fr::errorExit("memory manager panic");

		    memset(reservoir, 0, sizeof(allocUnit)*256);
		    for (i=0; i<255; i++)
		      reservoir[i].next=&reservoir[i+1];

		    if (reservoirBuffer)
		      temp=(allocUnit **) realloc(reservoirBuffer, (reservoirSize+1)*sizeof(allocUnit *));
		    else
		      temp=(allocUnit **) malloc(sizeof(allocUnit *));

		    if (!temp)
					fr::errorExit("memory manager panic");

		    reservoirBuffer=temp;
		    reservoirBuffer[reservoirSize++]=reservoir;
		  }

		  au=reservoir;
		  reservoir=au->next;

		  memset(au, 0, sizeof(allocUnit));
		  au->realSize=calcRealSize(size);
		  au->realAddr=malloc(au->realSize);
		  au->size=size;
		  au->addr=calcAddr(au->realAddr);
		  au->type=type;
		  au->line=line;
		  if (file)
		    strncpy(au->file, file, 63);
		  else
		    strcpy(au->file, "???");

		  hashIndex=((sU32) au->addr>>4) & ALLOCHASHMASK;
		  if (allocHash[hashIndex])
		    allocHash[hashIndex]->prev=au;

		  au->next=allocHash[hashIndex];
		  au->prev=0;
		  allocHash[hashIndex]=au;

#ifdef HOTSPOTS
		  updateHotspot(au->file?au->file:"???", au->line, au->size);
#endif

		  stats.totalAlloc+=au->size;
		  stats.totalRealAlloc+=au->realSize;
		  stats.totalAllocUnits++;
		  stats.curAlloc+=au->size;
		  stats.curRealAlloc+=au->realSize;
		  stats.curAllocUnits++;
		  
		  if (stats.curAlloc>stats.peakAlloc)
		    stats.peakAlloc=stats.curAlloc;

		  if (stats.curRealAlloc>stats.peakRealAlloc)
		    stats.peakRealAlloc=stats.curRealAlloc;

		  if (stats.curAllocUnits>stats.peakAllocUnits)
		    stats.peakAllocUnits=stats.curAllocUnits;

		  wipeUnit(au);
		  sourceFile="include_debug_h_everywhere_please.cpp";
		  sourceLine=303;

		  if (!au->addr)
				fr::debugOut("MDBG: warning, allocation of %lu bytes FAILED\n", au->size);

		  return au->addr;
		}

		void *memReAllocate(const sChar *file, const sU32 line, const allocType type, const sU32 size, void *addr)
		{
		  allocUnit *au;
		  sU32      hashIndex, oldSize, newRealSize;
		  void      *oldAddr, *newRealAddr;

		  FRASSERT(size<MAXALLOC);

		  if (!addr)
		    return memAllocate(file, line, type, size);

		  au=findUnit(addr);
		  if (!au)
		  {
				fr::debugOut("%s : trying to realloc nonallocated memory\n", formatOwner(file, line));
		    sourceFile="include_debug_h_everywhere_please.cpp";
		    sourceLine=303;
		    return 0;
		  }

		  if (type!=au->type)
				fr::debugOut("%s : allocation and reallocation types mismatch (%s vs %s)\n", formatOwner(file, line), atypes[au->type], atypes[type]);

		  oldSize=au->size;
		  oldAddr=au->addr;
		  newRealSize=calcRealSize(size);
		  newRealAddr=realloc(au->realAddr, newRealSize);

		  stats.curAlloc-=au->size;
		  stats.curRealAlloc-=au->realSize;

		  au->realSize=newRealSize;
		  au->realAddr=newRealAddr;
		  au->size=calcSize(newRealSize);
		  au->addr=calcAddr(newRealAddr);
		  au->type=type;
		  au->line=line;
		  if (file)
		    strncpy(au->file, file, 63);
		  else
		    strcpy(au->file, "???");

		  hashIndex=0xffffffff;
		  if (oldAddr!=au->addr)
		  {
		    hashIndex=((sU32) oldAddr>>4) & ALLOCHASHMASK;

		    if (allocHash[hashIndex]==au)
		      allocHash[hashIndex]=au->next;
		    else
		    {
		      if (au->prev) au->prev->next=au->next;
		      if (au->next) au->next->prev=au->prev;
		    }

		    hashIndex=((sU32) au->addr>>4) & ALLOCHASHMASK;
		    if (allocHash[hashIndex])
		      allocHash[hashIndex]->prev=au;

		    au->next=allocHash[hashIndex];
		    au->prev=0;
		    allocHash[hashIndex]=au;
		  }

#ifdef HOTSPOTS
		  updateHotspot(au->file?au->file:"???", au->line, au->size);
#endif
		  
		  stats.curAlloc+=au->size;
		  stats.curRealAlloc+=au->realSize;
		  
		  if (stats.curAlloc>stats.peakAlloc)
		    stats.peakAlloc=stats.curAlloc;

		  if (stats.curRealAlloc>stats.peakRealAlloc)
		    stats.peakRealAlloc=stats.curRealAlloc;

		  sourceFile="include_debug_h_everywhere_please.cpp";
		  sourceLine=303;

		  return au->addr;
		}

		void memDeAllocate(const sChar *file, const sU32 line, const allocType type, void *addr)
		{
		  allocUnit	*au;
		  sU32      hashIndex;

		  if (!addr)
		    return;

		  au=findUnit(addr);
		  if (!au)
		  {
				fr::debugOut("%s : trying to dealloc nonallocated memory\n", formatOwner(file, line));
		    sourceFile="include_debug_h_everywhere_please.cpp";
		    sourceLine=303;
		    return;
		  }

		  if (type!=au->type)
				fr::debugOut("%s : allocation and deallocation types mismatch (%s vs %s), orig alloc in %s:%d\n", formatOwner(file, line), atypes[au->type], dtypes[type], au->file, au->line);

		  free(au->realAddr);

		  hashIndex=((sU32) addr>>4) & ALLOCHASHMASK;
		  if (allocHash[hashIndex]==au)
		    allocHash[hashIndex]=au->next;
		  else
		  {
		    if (au->prev) au->prev->next=au->next;
		    if (au->next) au->next->prev=au->prev;
		  }

		  stats.curAlloc-=au->size;
		  stats.curRealAlloc-=au->realSize;
		  stats.curAllocUnits--;

		  memset(au, 0, sizeof(allocUnit));
		  au->next=reservoir;
		  reservoir=au;

		  sourceFile="include_debug_h_everywhere_please.cpp";
		  sourceLine=303;
		}

		static void dumpLeaks()
		{
			fr::debugOut("\n----------------------------------------------------------- MEMORY STATISTICS\n\n");

			fr::debugOut("Memory allocations (sum):       %10d\n", stats.totalAllocUnits);
			fr::debugOut("Memory allocations (peak):      %10d\n", stats.peakAllocUnits);
			fr::debugOut("Memory allocated by app (sum):  %s\n", formatSize(stats.totalAlloc));
			fr::debugOut("Memory allocated by app (peak): %s\n", formatSize(stats.peakAlloc));
			fr::debugOut("Memory allocated by app (now):  %s\n", formatSize(stats.curAlloc));
			fr::debugOut("Memory really allocated (sum):  %s\n", formatSize(stats.totalRealAlloc));
			fr::debugOut("Memory really allocated (peak): %s\n", formatSize(stats.peakRealAlloc));
			fr::debugOut("Memory really allocated (now):  %s\n", formatSize(stats.curRealAlloc));
			fr::debugOut("\n");

			fr::debugOut("the memory leak counter of horror sez: %d\n\n", stats.curAllocUnits);
		  
		  if (reservoirBuffer)
		  {
		    free(reservoirBuffer);
		    reservoirBuffer=0;
		    reservoirSize=0;
		    reservoir=0;
		  }

		  if (stats.curAllocUnits)
		    dumpAllocs();

#ifdef HOTSPOTS
		  dumpHotspots();
#endif

			fr::debugOut("-----------------------------------------------------------------------------\n");
		}

		sU32 getAllocatedMem()
		{
		  return stats.curAlloc;
		}

		sU32 memSize(void *addr)
		{
		  return findUnit(addr)->size;
		}

		void checkAllBlocks()
		{
#ifndef _DEBUG
			fr::debugOut("FATAL heap corruption only in debug builds!\n");
#else
#if RANDOMWIPE==sTRUE
			fr::debugOut("FATAL cannot check corruption with random wipe enabled. disable it.\n");
#else
		  for (sU32 i=0; i<ALLOCHASHSIZE; i++)
		  {
		    allocUnit *au=allocHash[i];
		    
		    while (au)
		    {
		      sU32 *pref=(sU32 *) (au->realAddr);
		      sU32 *post=(sU32 *) ((sU8 *) au->realAddr+au->realSize-4*PADDINGSIZE);
		      sBool pre=sFALSE, pos=sFALSE;
		      
		      for (sU32 i=0; i<PADDINGSIZE; i++)
		      {
		        pre|=(pref[i]!=PREFIXPATTERN);
		        pos|=(post[i]!=POSTFIXPATTERN);
		      }
		      
		      if (pre || pos)
						fr::debugOut("%s : block: memoverwrite detected pre: %d post: %d\n", formatOwner(au->file, au->line), pre, pos);
		      
		      au=au->next;
		    }
		  }
#endif
#endif
			fr::debugOut("----- end of heap corruption check  ----\n");
		}
	}
}

#if 0
void * __cdecl operator new(unsigned int size)
{
  if (!size)
  {
		fr::debugOut("%s : WARNING: zero size allocation\n", fr::debug::formatOwner(fr::debug::sourceFile, fr::debug::sourceLine));
    size++;
  }

	return fr::debug::memAllocate(fr::debug::sourceFile, fr::debug::sourceLine, fr::debug::allocTypeNew, size); 
}

void * __cdecl operator new[](unsigned int size)
{
  if (!size)
  {
		fr::debugOut("%s : WARNING: zero size allocation\n", fr::debug::formatOwner(fr::debug::sourceFile, fr::debug::sourceLine));
    size++;
  }

	return fr::debug::memAllocate(fr::debug::sourceFile, fr::debug::sourceLine, fr::debug::allocTypeNewA, size); 
}

void * __cdecl operator new(unsigned int size, const sChar *sourceFile, const sU32 sourceLine)
{
  if (!size)
  {
		fr::debugOut("%s : WARNING: zero size allocation\n", fr::debug::formatOwner(sourceFile, sourceLine));
    size++;
  }

	return fr::debug::memAllocate(sourceFile, sourceLine, fr::debug::allocTypeNew, size); 
}

void * __cdecl operator new[](unsigned int size, const sChar *sourceFile, const sU32 sourceLine)
{
  if (!size)
  {
		fr::debugOut("%s : WARNING: zero size allocation\n", fr::debug::formatOwner(sourceFile, sourceLine));
    size++;
  }

	return fr::debug::memAllocate(sourceFile, sourceLine, fr::debug::allocTypeNewA, size); 
}

void __cdecl operator delete(void *addr)
{
#ifdef NULLPOINTERS
  if (!addr)
		fr::debugOut("%s : WARNING: null pointer deleted\n", fr::debug::formatOwner(fr::debug::sourceFile, fr::debug::sourceLine));
  else
#endif
		fr::debug::memDeAllocate(fr::debug::sourceFile, fr::debug::sourceLine, fr::debug::allocTypeNew, addr);
}

void __cdecl operator delete[](void *addr)
{
#ifdef NULLPOINTERS
  if (!addr)
		fr::debugOut("%s : WARNING: null pointer deleted\n", fr::debug::formatOwner(fr::debug::sourceFile, fr::debug::sourceLine));
  else
#endif
		fr::debug::memDeAllocate(fr::debug::sourceFile, fr::debug::sourceLine, fr::debug::allocTypeNewA, addr);
}

#endif

#endif

static void processExitProcs()
{
  iExitList *t, *t2;

  for (t=exits; t; t=t2)
  {
    t2=t->next;

#ifdef _DEBUG
    if (!IsBadCodePtr((FARPROC) t->exit))
#endif
      t->exit();

    delete t;
  }
}

static LONG __stdcall unhandledExceptionFilter(EXCEPTION_POINTERS *exceptionInfo)
{
#ifdef _DEBUG
  sChar *msg;

  switch (exceptionInfo->ExceptionRecord->ExceptionCode)
  {
  case EXCEPTION_BREAKPOINT:
    if (!IsBadReadPtr((const void *) exceptionInfo->ContextRecord->Eip, 2))
    {
      const sU8 *exPtr=(const sU8 *) exceptionInfo->ContextRecord->Eip;

      if (exPtr[0]==0xcc) // int3
        exceptionInfo->ContextRecord->Eip++;
      else if (exPtr[0]==0xcd && exPtr[1]==0x03) // int 3
        exceptionInfo->ContextRecord->Eip+=2;
    }
    return EXCEPTION_CONTINUE_EXECUTION;

  case EXCEPTION_ACCESS_VIOLATION:    msg="Access Violation";         break;
  case EXCEPTION_ILLEGAL_INSTRUCTION: msg="Invalid Opcode";           break;
  case EXCEPTION_INT_DIVIDE_BY_ZERO:  msg="Integer Division by Zero"; break;
  case EXCEPTION_INT_OVERFLOW:        msg="Integer Overflow";         break;
  case EXCEPTION_STACK_OVERFLOW:      msg="Stack Overflow";           break;

  default:
    return EXCEPTION_CONTINUE_SEARCH;
  }

  SetErrorMode(SEM_NOGPFAULTERRORBOX);

  fr::errorExit("An exception occured: %s at %04x:%08x\n", msg,
    exceptionInfo->ContextRecord->SegCs, exceptionInfo->ContextRecord->Eip);
#endif

  return EXCEPTION_EXECUTE_HANDLER;
}

namespace fr
{
	void __stdcall defaultReportError(const sChar *errMsg)
	{
		MessageBox(0, errMsg, "Error", MB_ICONERROR|MB_OK);
	}

	static HANDLE dbgout=INVALID_HANDLE_VALUE;
	static debug::reportErrorProc errorProc=defaultReportError;

	void __cdecl errorExit(const sChar *format, ...)
	{
	  va_list    arg;
    sChar      errbuf[4096];
	  
	  va_start(arg, format);
	  wvsprintf(errbuf, format, arg);
	  va_end(arg);

//		debugOut("\nDBUG: error exit. reason: %s\n", errbuf);

		errorProc(errbuf);
	  processExitProcs();

	  if (dbgout!=INVALID_HANDLE_VALUE)
	    CloseHandle(dbgout);

	  ExitProcess(0);
	}

#ifdef _DEBUG
  void __cdecl debugOut(const sChar *format, ...)
	{
#ifndef FRDEBUG_NODEBUGOUT
	  va_list arg;
    sChar   buf[4096];
    DWORD   nwr;
	  
	  va_start(arg, format);
	  vsprintf(buf, format, arg);
	  va_end(arg);

	  if (dbgout!=INVALID_HANDLE_VALUE)
	    WriteFile(dbgout, buf, strlen(buf), &nwr, 0);

	  OutputDebugString(buf);
#endif
	}
#endif

	void atExit(debug::exitProc exit)
	{
	  iExitList *lst=new iExitList;

	  lst->exit=exit;
	  lst->next=exits;
	  exits=lst;
	}

	void setReportError(debug::reportErrorProc error)
	{
		errorProc=error;
	}
}

void __cdecl autoDebugInit(void)
{
	fr::dbgout=CreateFile("fvsdebug.txt", GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
  SetUnhandledExceptionFilter(unhandledExceptionFilter);

}

void __cdecl autoDebugClose(void)
{
  processExitProcs();

#ifdef _DEBUG
	fr::debug::dumpLeaks();
#endif

	if (fr::dbgout!=INVALID_HANDLE_VALUE)
		CloseHandle(fr::dbgout);
}

// kids, don't try this at home, we're trained professionals here

#ifdef _DEBUG

typedef void (__cdecl *_PVFV)(void);

#pragma data_seg(".CRT$XCY")
static _PVFV __xc_y[]={&autoDebugInit};
#pragma data_seg(".CRT$XPY")
static _PVFV __xp_y[]={&autoDebugClose};
#pragma data_seg()

#endif

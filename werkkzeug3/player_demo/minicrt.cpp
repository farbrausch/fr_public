// This file is distributed under a BSD license. See LICENSE.txt for details.

// mini c-runtime library

#include "_types.hpp"

#if sNOCRT

#define WIN32_LEAN_AND_MEAN
#include "windows.h"

#pragma function (memcpy,memset)

static HANDLE hProcessHeap;
sInt MallocSize;

extern "C"
{
  // ---- library support
  sInt _fltused;

  int __cdecl _purecall()
  {
    sFatal("pure virtual function called");
    return 0;
  }

  // ---- memory allocation
  void * __cdecl malloc(size_t size)
  {
    MallocSize += size;
  	return HeapAlloc(hProcessHeap,0,size);
  }

  void __cdecl free(void *ptr)
  {
    MallocSize -= HeapSize(hProcessHeap,0,ptr);
    HeapFree(hProcessHeap,0,ptr);
  }

  void * __cdecl _aligned_malloc(size_t size,size_t align)
  {
    // align must be a power of 2
    sVERIFY((align & (align - 1)) == 0);
    align = sMax(align,sizeof(void*)) - 1;

    uintptr_t mem = (uintptr_t) malloc(size + sizeof(void*) + align);
    if(!mem)
      return 0;

    uintptr_t memalign = (mem + sizeof(void*) + align) & ~align;
    ((uintptr_t *) memalign)[-1] = mem;

    return (void *) memalign;
  }

  void __cdecl _aligned_free(void *ptr)
  {
    uintptr_t *ptru = (uintptr_t *) ptr;
    if(ptru)
      free((void *) ptru[-1]);
  }

  // ---- string functions

  __declspec(naked) void * __cdecl memcpy(void *dst,const void *src,size_t size)
  {
    __asm
    {
      push    esi;
      push    edi;

      mov     edi, [esp+12];
      mov     eax, edi;
      mov     esi, [esp+16];
      mov     ecx, [esp+20];
      shr     ecx, 2;
      jz      memcpy_tail;
      rep     movsd;

memcpy_tail:
      mov     ecx, [esp+20];
      and     ecx, 3;
      rep     movsb;
      
      pop     edi;
      pop     esi;
      ret;
    }
  }

  __declspec(naked) void * __cdecl memset(void *dst,int c,size_t count)
  {
    __asm
    {
      push    edi;

      mov     edi, [esp+8];
      movzx   eax, byte ptr [esp+12];
      mov     edx, eax;
      shl     edx, 8;
      or      eax, edx;
      mov     edx, eax;
      shl     edx, 16;
      or      eax, edx;
      mov     ecx, [esp+16];
      shr     ecx, 2;
      jz      memset_tail;
      rep     stosd;

memset_tail:
      mov     ecx, [esp+16];
      and     ecx, 3;
      rep     stosb;

      mov     eax, [esp+8];
      pop     edi;
      ret;
    }
  }

  __declspec(naked) sInt _ftol2_sse(sF64 value)
  {
    __asm
    {
		  push				ebp;
      mov					ebp, esp;
		  sub					esp, 20h;
		  and					esp, -16;
		  fld					st;
		  fst					dword ptr [esp+18h];
		  fistp				qword ptr [esp+10h];
		  fild				qword ptr [esp+10h];
		  mov					edx, dword ptr [esp+18h];
		  mov					eax, dword ptr [esp+10h];
		  test				eax, eax;
		  je					int_qnan_or_zero;

not_int_qnan:		
		  fsubp				st(1), st;
		  test				edx, edx;
		  jns					positive;
		  fstp				dword ptr [esp];
		  mov					ecx, [esp];
		  xor					ecx, 80000000h;
		  add					ecx, 7fffffffh;
		  adc					eax, 0;
		  mov					edx, dword ptr [esp+14h];
		  adc					edx, 0;
  		jmp					short exit;
		
positive:
		  fstp				dword ptr [esp];
		  mov					ecx, dword ptr [esp];
		  add					ecx, 7fffffffh;
		  sbb					eax, 0;
		  mov					edx, dword ptr [esp+14h];
		  sbb					edx, 0;
		  jmp					short exit;
		
int_qnan_or_zero:
		  mov					edx, dword ptr [esp+14h];
		  test				edx, 7fffffffh;
		  jne					not_int_qnan;
		  fstp				dword ptr [esp+18h];
		  fstp				dword ptr [esp+18h];
		
exit:
		  leave;
		  ret;
    }
  }

  // ---- startup/initializers

  typedef void (__cdecl *_PVFV)(void);

  #pragma data_seg(".CRT$XCA")
  _PVFV __xc_a[] = { NULL };
  #pragma data_seg(".CRT$XCZ")
  _PVFV __xc_z[] = { NULL };
  #pragma data_seg(".CRT$XPA")
  _PVFV __xp_a[] = { NULL };
  #pragma data_seg(".CRT$XPZ")
  _PVFV __xp_z[] = { NULL };
  #pragma data_seg(".CRT$XTA")
  _PVFV __xt_a[] = { NULL };
  #pragma data_seg(".CRT$XTZ")
  _PVFV __xt_z[] = { NULL };
  #pragma data_seg()

  #pragma comment(linker,"/merge:.CRT=.rdata")

  static void initterm(_PVFV *start,_PVFV *end)
  {
    while(start < end)
    {
      if(*start)
        (**start)();

      start++;
    }
  }

  void __stdcall WinMainCRTStartup()
  {
    hProcessHeap = GetProcessHeap();

    // run initializers
    initterm(__xc_a,__xc_z);

    // call winmain (no commandline parsing yet)
    int ret = WinMain(GetModuleHandle(0),0,"",SW_SHOWDEFAULT);

    // run pre-terminators
    initterm(__xp_a,__xp_z);

    // run terminators
    initterm(__xt_a,__xt_z);

    ExitProcess(ret);
  }
}

#endif

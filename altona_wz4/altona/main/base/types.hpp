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

#ifndef HEADER_ALTONA_BASE_TYPES
#define HEADER_ALTONA_BASE_TYPES

#ifndef __GNUC__
#pragma once
#endif

#ifndef NN_COMPILER_RVCT
; // if this semicolon causes an error, then something is wrong BEFORE
  // you include types.hpp (or anything else)
  // but don't do this check for 3ds, since it generates a warning.
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Configuration checking                                             ***/
/***                                                                      ***/
/****************************************************************************/

// configuration enumerations.

// sPLATFORM = one of...

#define sPLAT_NONE      0         // for: no selection made
#define sPLAT_WINDOWS   1
#define sPLAT_LINUX     2
#define sPLAT_PS3       x x x
#define sPLAT_XBOX360   x x x
#define sPLAT_PS2       x x x
#define sPLAT_WII       x x x 
#define sPLAT_APPLE     x x x     // not implemented. intendet for MacOS X
#define sPLAT_IOS       8
#define sPLAT_3DS       x x x

// sRENDERER = one of

#define sRENDER_BLANK     0       // blank screen, no rendering
#define sRENDER_DX9       1       // dx9 rendering
#define sRENDER_OGL2      2       // opengl 2.0 rendering
#define sRENDER_DX11      3       // dx11 rendering (vista required)
#define sRENDER_PS2       x x x   // ps2 ee/gs engine
#define sRENDER_WII       x x x   // wii GX
#define sRENDER_XBOX360   x x x
#define sRENDER_PS3       x x x   // ps3
#define sRENDER_OGLES2    10      // ios  
#define sRENDER_3DS       x x x

/****************************************************************************/

// include the installation dependent configuration file
// if this file is missing, rename "altona/altona_config_sample.hpp" to 
// "altona/altona_config.hpp". 

#include "../altona_config.hpp"

/****************************************************************************/

#ifndef sPEDANTIC_OBSOLETE
#define sPEDANTIC_OBSOLETE 0
#endif

#ifndef sPEDANTIC_WARN
#define sPEDANTIC_WARN 0
#endif

#ifdef sCONFIG_BUILD_DEBUG
#undef sCONFIG_BUILD_DEBUG
#define sCONFIG_BUILD_DEBUG 1
#else
#define sCONFIG_BUILD_DEBUG 0
#endif

#ifdef sCONFIG_BUILD_DEBUGFAST
#undef sCONFIG_BUILD_DEBUGFAST
#define sCONFIG_BUILD_DEBUGFAST 1
#else
#define sCONFIG_BUILD_DEBUGFAST 0
#endif

#ifdef sCONFIG_BUILD_RELEASE
#undef sCONFIG_BUILD_RELEASE
#define sCONFIG_BUILD_RELEASE 1
#else
#define sCONFIG_BUILD_RELEASE 0
#endif

#ifdef sCONFIG_BUILD_STRIPPED
#undef sCONFIG_BUILD_STRIPPED
#define sCONFIG_BUILD_STRIPPED 1
#else
#define sCONFIG_BUILD_STRIPPED 0
#endif


#ifdef sCONFIG_OPTION_XSI
#undef sCONFIG_OPTION_XSI
#define sCONFIG_OPTION_XSI 1
#else
#define sCONFIG_OPTION_XSI 0
#endif

#ifdef sCONFIG_OPTION_NPP
#undef sCONFIG_OPTION_NPP
#define sCONFIG_OPTION_NPP 1
#else
#define sCONFIG_OPTION_NPP 0
#endif

#ifdef sCONFIG_OPTION_NOSYSTEM
#undef sCONFIG_OPTION_NOSYSTEM
#define sCONFIG_OPTION_NOSYSTEM 1
#else
#define sCONFIG_OPTION_NOSYSTEM 0
#endif

#ifdef sCONFIG_OPTION_LOCAKIT
#undef sCONFIG_OPTION_LOCAKIT
#define sCONFIG_OPTION_LOCAKIT 1
#else
#define sCONFIG_OPTION_LOCAKIT 0
#endif

#ifdef sCONFIG_OPTION_AGEINGTEST
#undef sCONFIG_OPTION_AGEINGTEST
#define sCONFIG_OPTION_AGEINGTEST 1
#else
#define sCONFIG_OPTION_AGEINGTEST 0
#endif

#ifdef sCONFIG_OPTION_DEMO

#undef sCONFIG_OPTION_DEMO
#define sCONFIG_OPTION_DEMO 1
#else
#define sCONFIG_OPTION_DEMO 0
#endif

#ifdef sCONFIG_OPTION_SHELL
#undef sCONFIG_OPTION_SHELL
#define sCONFIG_OPTION_SHELL 1
#else
#define sCONFIG_OPTION_SHELL 0
#endif

#ifdef sCONFIG_OPTION_GAMEPAD
#undef sCONFIG_OPTION_GAMEPAD
#define sCONFIG_OPTION_GAMEPAD 1
#else
#define sCONFIG_OPTION_GAMEPAD 0
#endif

#ifdef sCONFIG_RENDER_BLANK
#undef sCONFIG_RENDER_BLANK
#define sCONFIG_RENDER_BLANK 1
#define sRENDERER sRENDER_BLANK
#define sCONFIG_QUADRICS 0
#else
#define sCONFIG_RENDER_BLANK 0
#endif

#ifdef sCONFIG_RENDER_DX9
#undef sCONFIG_RENDER_DX9
#define sCONFIG_RENDER_DX9 1
#define sRENDERER sRENDER_DX9
#define sCONFIG_QUADRICS 0
#else
#define sCONFIG_RENDER_DX9 0
#endif

#ifdef sCONFIG_RENDER_DX11
#undef sCONFIG_RENDER_DX11
#define sCONFIG_RENDER_DX11 1
#define sRENDERER sRENDER_DX11
#define sCONFIG_QUADRICS 0
#else
#define sCONFIG_RENDER_DX11 0
#endif

#ifdef sCONFIG_RENDER_OGL2
#undef sCONFIG_RENDER_OGL2
#define sCONFIG_RENDER_OGL2 1
#define sRENDERER sRENDER_OGL2
#define sCONFIG_QUADRICS 0
#else
#define sCONFIG_RENDER_OGL2 0
#endif

#ifdef sCONFIG_RENDER_OGLES2
#undef sCONFIG_RENDER_OGLES2
#define sCONFIG_RENDER_OGLES2 1
#define sRENDERER sRENDER_OGLES2
#define sCONFIG_QUADRICS 0
#else
#define sCONFIG_RENDER_OGLES2 0
#endif

// find out used platform

#if defined(_WIN32)
#define sCONFIG_SYSTEM_WINDOWS 1
#define sPLATFORM sPLAT_WINDOWS
#define sCONFIG_FRAMEMEM_MT 1
#else


#ifdef sCONFIG_SYSTEM_IOS
#define sCONFIG_SYSTEM_IOS 1
#define sPLATFORM sPLAT_IOS
#else

#define sCONFIG_SYSTEM_LINUX 1
#define sPLATFORM sPLAT_LINUX
#define sCONFIG_FRAMEMEM_MT 1

#endif
#endif

// fill out rest of platforms
#define sCONFIG_SYSTEM_PS2 x x x
#define sCONFIG_SYSTEM_WII x x x
#define sCONFIG_SYSTEM_XBOX360 x x x
#define sCONFIG_SYSTEM_PS3 x x x
#define sCONFIG_SYSTEM_3DS x x x

#ifndef sCONFIG_SYSTEM_WINDOWS
#define sCONFIG_SYSTEM_WINDOWS 0
#endif
#ifndef sCONFIG_SYSTEM_IOS
#define sCONFIG_SYSTEM_IOS 0
#endif
#ifndef sCONFIG_SYSTEM_LINUX
#define sCONFIG_SYSTEM_LINUX 0
#endif


// fix coderoot

#ifndef sCONFIG_CODEROOT_WINDOWS
#define sCONFIG_CODEROOT_WINDOWS sCONFIG_CODEROOT
#endif

#ifndef sCONFIG_CODEROOT_LINUX
#define sCONFIG_CODEROOT_LINUX sCONFIG_CODEROOT
#endif

#ifndef sCONFIG_CODEROOT
#if sCONFIG_SYSTEM_LINUX
#define sCONFIG_CODEROOT sCONFIG_CODEROOT_LINUX
#else
#define sCONFIG_CODEROOT sCONFIG_CODEROOT_WINDOWS
#endif
#endif


/****************************************************************************/
/****************************************************************************/

#ifdef _MSC_VER                             // microsoft C++

// configuration switching

#define sCONFIG_COMPILER_MSC      1         // compiled with Microsoft Visuacl C++
#define sCONFIG_COMPILER_GCC      0         // compiled with Gnu C++
#define sCONFIG_COMPILER_CWCC     0         // compiled with CodeWarrior C++
#define sCONFIG_COMPILER_SNC      0         // compiled with SN Systems C++
#define sCONFIG_COMPILER_ARM      0         // compiled with ARM RVCT (3ds)
#define sCONFIG_32BIT             1         // Pointers are 32 bit
#define sCONFIG_64BIT             0         // Pointers are 64 bit
#define sCONFIG_ASM_MSC_IA32      1         // use 80386 instruction set with MSC syntax
#define sCONFIG_ASM_87            1         // when using IA32, use 8087 fpu
#define sCONFIG_ASM_SSE           0         // when using IA32, use SSE fpu

// type switching

#define sCONFIG_NATIVEINT         int _w64                // sDInt: an int of the same size as a pointer
#define sCONFIG_INT64             __int64                 // sS64, sU64: a 64 bit int
#define sCONFIG_UNICODETYPE       wchar_t                 // sChar: an unicode character
#define sCONFIG_SIZET             size_t
#define sINLINE                   __forceinline           // use this to inline
#define sNOINLINE                 __declspec(noinline)    // forbid inlining. usefull to hint compiler errors
#define sNORETURN                 __declspec(noreturn)    // use this for dead end funtions
#define sDEBUGBREAK               __debugbreak();
#define sCOMPILEERROR(x)          y y y y x 
#define sCDECL                    __cdecl
#define sOBSOLETE                 __declspec(deprecated)
#define sALIGNED(type, name, al)  __declspec( align (al) ) type name
#define sRESTRICT                 __restrict
#define sUNUSED                                           // possibly unused variable
#define sOVERRIDE                 override
#define sALIGNOF                  __alignof

// compiler dependent settings

#pragma warning (disable : 4244) 
void __debugbreak();
#pragma intrinsic (__debugbreak)
// variations

#if sCONFIG_OPTION_XSI
#undef sCONFIG_UNICODETYPE
#define sCONFIG_UNICODETYPE       unsigned short 
#endif

#ifdef _WIN64
#undef sCONFIG_NATIVEINT
#undef sCONFIG_INT64
#undef sCONFIG_SIZET
#undef sCONFIG_32BIT
#undef sCONFIG_64BIT
#undef sCONFIG_ASM_MSC_IA32
#undef sCONFIG_ASM_87
#undef sCONFIG_ASM_SSE
#define sCONFIG_NATIVEINT         __int64 _w64
#define sCONFIG_INT64             __int64 _w64
#define sCONFIG_SIZET             unsigned __int64
#define sCONFIG_32BIT             0
#define sCONFIG_64BIT             1
#define sCONFIG_ASM_MSC_IA32      0
#define sCONFIG_ASM_87            0
#define sCONFIG_ASM_SSE           0
#endif

#define sCONFIG_ALLOCA(a)         _alloca(a)
extern "C" void *_alloca(sCONFIG_SIZET size);
#pragma intrinsic (_alloca)

// done

#endif

/****************************************************************************/

// don't comment options here, that would duplicate.

#if (defined(__GNUC__) || defined(__GCC__)) && !defined(__SNC__) && !defined(NN_COMPILER_RVCT) // GNU C++

#include <stddef.h>
#include <stdlib.h>

// configuration switching

#define sCONFIG_COMPILER_MSC      0
#define sCONFIG_COMPILER_GCC      1
#define sCONFIG_COMPILER_CWCC     0
#define sCONFIG_COMPILER_SNC      0
#define sCONFIG_COMPILER_ARM      0

#ifndef __LP64__

#define sCONFIG_32BIT             1
#define sCONFIG_64BIT             0
#define sCONFIG_NATIVEINT         int
#define sCONFIG_PTHREAD_T_SIZE    (sizeof(sU32))

#else

#define sCONFIG_32BIT             0
#define sCONFIG_64BIT             1
#define sCONFIG_NATIVEINT         long long
#define sCONFIG_PTHREAD_T_SIZE    (sizeof(sU64))

#endif

// type switching

#define sCONFIG_INT64             long long
#define sCONFIG_UNICODETYPE       wchar_t
#define sCONFIG_SIZET             size_t
#define sINLINE                   inline
#define sNOINLINE
#define sNORETURN
#define sDEBUGBREAK               {};//__asm__("int $3");
#define sCOMPILEERROR(x)          y y y y x 
#define sCDECL
//#define sOBSOLETE                  __attribute__ ((deprecated))
#define sOBSOLETE                 
#define sALIGNED(type, name, al)  type name __attribute__ ((aligned (al)))
#define sRESTRICT                 __restrict
#define sCONFIG_ALLOCA(a)         __builtin_alloca(a)
#define sUNUSED                   __attribute__((unused))    // possibly unused variable
#define sOVERRIDE                 
#define sALIGNOF                  __alignof

// compiler dependent settings

void __debugbreak();

// done

#endif

#ifdef __CWCC__                             // CodeWarrior C++

// compiler behaviour
#pragma ipa file


// configuration switching

#define sCONFIG_COMPILER_MSC      0
#define sCONFIG_COMPILER_GCC      0
#define sCONFIG_COMPILER_CWCC     1
#define sCONFIG_COMPILER_SNC      0
#define sCONFIG_COMPILER_ARM      0
#define sCONFIG_32BIT             1
#define sCONFIG_64BIT             0

// type switching

#define sCONFIG_NATIVEINT         int
#define sCONFIG_INT64             long long
#define sCONFIG_UNICODETYPE       wchar_t
#define sCONFIG_SIZET             unsigned long
#define sINLINE                   __inline__
#define sNOINLINE                 __attribute__((never_inline))
#define sNORETURN
#define sDEBUGBREAK               {};
#define sCOMPILEERROR(x)          y y y y x 
#define sCDECL
#define sOBSOLETE                 
#define sALIGNED(type, name, al)  type name __attribute__ ((aligned (al)))
#define sCONFIG_ALLOCA(a)         __alloca(a)
#define sUNUSED                   __attribute__((unused))
#define sRESTRICT                 __restrict
#define sOVERRIDE                 
#define sALIGNOF(T)               ((sizeof(T)<4)?4:sizeof(T))

// compiler dependent settings

void __debugbreak();

// done

#endif

#if defined(__SNC__)       // SN systems compiler

#include <alloca.h>
#include <stddef.h>
#include <stdlib.h>

// configuration switching

#define sCONFIG_COMPILER_MSC      0
#define sCONFIG_COMPILER_GCC      0
#define sCONFIG_COMPILER_CWCC     0
#define sCONFIG_COMPILER_SNC      1
#define sCONFIG_COMPILER_ARM      0

#define sCONFIG_32BIT             1
#define sCONFIG_64BIT             0
#define sCONFIG_NATIVEINT         int
#define sCONFIG_PTHREAD_T_SIZE    (sizeof(sU32))

// type switching

#define sCONFIG_INT64             long long
#define sCONFIG_UNICODETYPE       wchar_t
#define sCONFIG_SIZET             size_t
#define sINLINE                   inline
#define sNOINLINE
#define sNORETURN
#define sDEBUGBREAK               {};//__asm__("int $3");
#define sCOMPILEERROR(x)          y y y y x 
#define sCDECL
//#define sOBSOLETE                  __attribute__ ((deprecated))
#define sOBSOLETE                 
#define sALIGNED(type, name, al)  type name __attribute__ ((aligned (al)))
#define sRESTRICT                 __restrict
#define sCONFIG_ALLOCA(a)         alloca(a)
#define sUNUSED                   __attribute__((unused))    // possibly unused variable
#define sOVERRIDE                 
#define sALIGNOF                  __alignof

// compiler dependent settings

//#pragma diag_suppress=613,1011,1669, // get rid of the more annoying warnings
#pragma diag_suppress=613,1011,1669,872,186,1421,112 // this is a more forgiving setting

void __debugbreak();

// done

#endif

#if defined(NN_COMPILER_RVCT)       // ARM RVCT


// configuration switching

#define sCONFIG_COMPILER_MSC      0
#define sCONFIG_COMPILER_GCC      0
#define sCONFIG_COMPILER_CWCC     0
#define sCONFIG_COMPILER_SNC      0
#define sCONFIG_COMPILER_ARM      1

#define sCONFIG_32BIT             1
#define sCONFIG_64BIT             0
#define sCONFIG_NATIVEINT         int
#define sCONFIG_PTHREAD_T_SIZE    (sizeof(sU64))

// type switching

#define sCONFIG_INT64             long long
#define sCONFIG_UNICODETYPE       wchar_t
#define sCONFIG_SIZET             unsigned int
#define sINLINE                   inline
#define sNOINLINE                 __declspec(noinline)
#define sNORETURN                 __declspec(noreturn)
#define sDEBUGBREAK               __breakpoint(0);
#define sCOMPILEERROR(x)          y y y y x 
#define sCDECL
//#define sOBSOLETE                  __attribute__ ((deprecated))
#define sOBSOLETE                 
#define sALIGNED(type, name, al)  type name __attribute__ ((aligned (al)))
#define sRESTRICT                 __restrict
#define sCONFIG_ALLOCA(a)         __builtin_alloca(a)
#define sUNUSED                   __attribute__((unused))    // possibly unused variable
#define sOVERRIDE                 
#define sALIGNOF                  __ALIGNOF__

void __debugbreak();

#pragma diag_suppress 381,228,1300,826,479,831,2817    // stupid warnings
// these wunderful warnings should stay on. but i don't have the time!
#pragma diag_suppress 1301        // structure padding inside
#pragma diag_suppress 2530        // structure padding at end
#pragma diag_suppress 1517        // declaration hides variable
#pragma diag_suppress 177         // function declared but never referenced
#pragma diag_suppress 1299        // members and base-classes will be initialized in declaration order, not in member initialisation list order
#pragma diag_suppress 550         // variable was set but never used


#endif



// platform dependent settings

#if sCONFIG_SYSTEM_WINDOWS
#define D3D_DEBUG_INFO 1
#define WINVER 0x0501           // require windows xp!
#define _WIN32_WINNT 0x0501
#define DIRECTINPUT_VERSION 0x0800
#undef UNICODE
#undef _UNICODE
#define UNICODE
#define _UNICODE

#define _INC_MALLOC
#include "xmmintrin.h"
#include "emmintrin.h"
#undef _INC_MALLOC
#define sCONFIG_SSE               __m128
#define sSTDCALL                  __stdcall
#endif


#if sCONFIG_SYSTEM_LINUX
#define sSTDCALL

#ifndef __SSE__
#define sCONFIG_SSE               void // mainly for initial build
#else
#include "xmmintrin.h"
#define sCONFIG_SSE               __m128
#endif
#endif

#define sCONFIG_LE 1
#define sCONFIG_BE 0
#define sCONFIG_UNALIGNEDCRASH 0

#if sCONFIG_SYSTEM_IOS
#define sSTDCALL
#define sCONFIG_SSE               void
#define sCONFIG_GUID              { 0 }
#endif

/****************************************************************************/

// use these macros instead of the sCONFIG_xxx_xxx ones.

#if !sCONFIG_BUILD_STRIPPED
#define sDEBUG        1                     // include sVERIFY
#else
#define sDEBUG        0
#endif
#define sINTRO        0                     // code size optimisations 
#define sRELEASE      (sCONFIG_BUILD_RELEASE||sCONFIG_BUILD_STRIPPED)      // ommit time consuming checks
#define sSTRIPPED     sCONFIG_BUILD_STRIPPED // no cheats, no debugs.
#define sCOMMANDLINE  sCONFIG_OPTION_SHELL  // commandline build - don't open window

/****************************************************************************/

// pedantism

#if sPEDANTIC_OBSOLETE == 1                 // use sSOON_OBSOLETE in a period of transition.
#define sSOON_OBSOLETE sOBSOLETE            // one source after another can convert to pedantism,
#else                                       // and then you can move to sOBSOLETE
#define sSOON_OBSOLETE                      // yet, there is so much altona code, it may take
#endif                                      // quite a while to go from sOBSOLETE to compile errors.

#if sPEDANTIC_WARN == 1                     // more warnings
#ifdef _MSC_VER
  #pragma warning (3 : 4244)          // enable float to int warnings
#endif
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Basic Types and Functions                                          ***/
/***                                                                      ***/
/****************************************************************************/

typedef unsigned char             sU8;      // for packed arrays
typedef unsigned short            sU16;     // for packed arrays
typedef unsigned int              sU32;     // for packed arrays and bitfields
typedef unsigned sCONFIG_INT64    sU64;     // use as needed
typedef signed char               sS8;      // for packed arrays
typedef short                     sS16;     // for packed arrays
typedef int                       sS32;     // for packed arrays
typedef signed sCONFIG_INT64      sS64;     // use as needed
typedef float                     sF32;     // basic floatingpoint
typedef double                    sF64;     // use as needed
typedef int                       sInt;     // use this most!
typedef unsigned sCONFIG_NATIVEINT sPtr;    // type for pointer
typedef signed sCONFIG_NATIVEINT  sDInt;    // type for pointer diff
typedef sCONFIG_UNICODETYPE       sChar;    // type for strings (UNICODE!)
typedef sS64                      sSize;    // size of a file = something possibly larger than memory

typedef char                      sChar8;   // only use where the API requires ascii
typedef int                       sBool;    // use for boolean function results

typedef sCONFIG_SSE               sSSE ;   // 4 x sF32

/****************************************************************************/

#define sMAX_U8    0xff
#define sMAX_U16   0xffff
#define sMAX_U32   0xffffffff
#define sMAX_U64   0xffffffffffffffff

#define sMAX_S8    0x7f
#define sMIN_S8    (-sMAX_S8-1)
#define sMAX_S16   0x7fff
#define sMIN_S16   (-sMAX_S16-1)
#define sMAX_S32   0x7fffffff
#define sMIN_S32   (-sMAX_S32-1)
#define sMAX_S64   0x7fffffffffffffff
#define sMAX_INT   (~(1<<(8*sizeof(sInt)-1)))
#define sMAX_F32   (1E+37)
#define sMIN_F32   (1E-37)

#define sVARARG(p,i) (((sInt *)(p))[(i)+1])               // don't use varargs at all if possible
#define sVARARGF(p,i) (*(sF64*)&(((sInt *)(p))[(i)+1]))
#define sALLOCSTACK(type,count)  ((type *) sCONFIG_ALLOCA((count)*sizeof(type)))   // this has to be a macro because is MUST be inlined!
#define sALLOCALIGNEDSTACK(type,count,ba)  ((type *) ((sDInt(sCONFIG_ALLOCA((count)*sizeof(type)+ba-1))+ba-1)&~(ba-1)))   // this has to be a macro because is MUST be inlined!
#define sMAKE2(a,b) (((b)<<16)|(a))
#define sMAKE4(a,b,c,d) (((d)<<24)|((c)<<16)|((b)<<8)|(a))
#define sCOUNTOF(x) sDInt(sizeof(x)/sizeof(*(x)))    // #elements of array, usefull for unicode strings
#define sINDEXVALID(index,arrayVar) (((unsigned) (index)) < ((unsigned) sCOUNTOF(arrayVar))) 

#define sTXT2(y) L ## y
#define sTXT(x) sTXT2(x)              // make long string from short string, like sTXT(__FILE__)

#if sPLATFORM==sPLAT_WINDOWS || sPLATFORM==sPLAT_LINUX
#define sMAXPATH    4096              // use this for path-strings. 
#else
#define sMAXPATH  512
#endif

/****************************************************************************/

#if sCONFIG_COMPILER_GCC
  #if __GNUC__ >= 4
    #define sOFFSET(TYPE, MEMBER) __builtin_offsetof(TYPE,MEMBER)
  #else
    #define sOFFSET(TYPE, MEMBER)					\
    (__offsetof__ (reinterpret_cast <sDInt>			\
                 (&reinterpret_cast <const volatile char &>	\
                  (static_cast<TYPE *> (0)->MEMBER))))
  #endif
#else
  #define sOFFSET(t,m) (((sDInt)(&((t*)64)->m))-64)
#endif

#define sOFFSETO(o,m) ((sDInt)(((sU8 *)(&(o)->m))-((sU8 *)(o))))    // offset from object
#define sOFFSETOMP(o,m) ((sDInt)(((sU8 *)(&((o)->*(m))))-((sU8 *)(o))))  // offset from object, member pointer

// experimental!
// in the presence of virtual functions, i can't cast pointer to member
// variables in a relyable way. unless multiple inheritence is used, this 
// hack should work on all current platforms
//
// mp = sMemberPtr<membertype>(sOFFSET(classtype,member));
template<typename ElementType> struct sMemberPtr
{
  sDInt Offset;
  sMemberPtr() { Offset=0; }
  sMemberPtr(sDInt o) { Offset=o; }
  typedef ElementType Type;
  ElementType &Ref(void *obj) { return *(ElementType *)(((sU8 *)obj)+Offset); }
};
// abstract version: cast as you like
struct sVoidMemberPtr
{
  sDInt Offset;
  sVoidMemberPtr() { Offset=0; }
  sVoidMemberPtr(sDInt o) { Offset=o; }
  template<typename ElementType> ElementType &Ref(void *obj) { return *(ElementType *)(((sU8 *)obj)+Offset); }
};


template<typename MemberType,typename ClassType> 
sMemberPtr<MemberType> sMakeMemberPtr(MemberType ClassType::*,sDInt o) 
{ return sMemberPtr<MemberType>(o); }

#define sMEMBERPTR(classtype,membername) sMakeMemberPtr(&classtype::membername,sOFFSET(classtype,membername))

/****************************************************************************/

#define sTRUE   (!0)
#define sFALSE  0
#define sNULL   0
#define sPI     3.1415926535897932384626433832795
#define sPI2    6.28318530717958647692528676655901
#define sPIF    3.1415926535897932384626433832795f
#define sPI2F   6.28318530717958647692528676655901f
#define sSQRT2  1.4142135623730950488016887242097
#define sSQRT2F 1.4142135623730950488016887242097f

// degrees to radians conversion
#define sRAD2DEG(x) ((x)*180.0f/sPIF)
#define sDEG2RAD(x) ((x)*sPIF/180.0f)

/****************************************************************************/

template <class Type> sINLINE Type sMin(Type a,Type b)              {return (a<b) ? a : b;}
template <class Type> sINLINE Type sMax(Type a,Type b)              {return (a>b) ? a : b;}
template <class Type> sINLINE Type sMin3(Type a,Type b,Type c)      {return sMin(sMin(a,b),c);}
template <class Type> sINLINE Type sMax3(Type a,Type b,Type c)      {return sMax(sMax(a,b),c);}
template <class Type> sINLINE Type sSign(Type a)                    {return (a==0) ? Type(0) : (a>0) ? Type(1) : Type(-1);}
template <class Type> sINLINE Type sCompare(Type a,Type b)          {return (a>b) ? 1 : ((a==b) ? 0 : -1); }
template <class Type> sINLINE Type sClamp(Type a,Type min,Type max) {return (a>=max) ? max : (a<=min) ? min : a;}
template <class Type> sINLINE sBool sIsInRangeIncl(Type a,Type min,Type max) {return (min<=a) && (a<=max);}
template <class Type> sINLINE sBool sIsInRangeExcl(Type a,Type min,Type max) {return (min<=a) && (a<max);}
template <class Type> sINLINE void sSwap(Type &a,Type &b)           {Type s=a; a=b; b=s;}
template <class Type> sINLINE Type sAlign(Type a,sInt b)            {typedef sInt check[(sizeof(Type)<=sizeof(sDInt))?1:-1]; return (Type)((((sDInt)a)+b-1)&(~(b-1)));} // doesn't work correctly if sizeof(sDInt)<sizeof(Type)
sINLINE sS64 sAlign(sS64 a,sInt b)                                  {return ((a+b-1)&(~(b-1)));}
sINLINE sU64 sAlign(sU64 a,sInt b)                                  {return ((a+b-1)&(~(b-1)));}


template <class Type> sINLINE Type sSquare(Type a)                  {return a*a;}
sInt sFindLowerPower(sInt x);
sInt sFindHigherPower(sInt x);
sU32 sMakeMask(sU32 max);
sU64 sMakeMask(sU64 max);
inline sBool sIsPower2(sInt x)                                      {return (x&(x-1)) == 0;}
inline sInt sDivDown(sInt a,sInt b)                                 {return a>0?a/b:-(-a/b);}

template <class Type> sINLINE void sDelete(Type &a)                 {if(a) delete a; a=0;}
template <class Type> sINLINE void sDeleteArray(Type &a)            {if(a) delete[] a; a=0;}
template <class Type> sINLINE void sRelease(Type &a)                {if(a) a->Release(); a=0;}
template <class Type> sINLINE void sAddRef(Type &a)                 {if(a) a->AddRef();}

template <class Type> sINLINE Type sFade(sF32 f,Type a,Type b)      {return (Type)(a+(b-a)*f);}
template <class Type> sINLINE Type sFadeBilin(sF32 u,sF32 v,Type x00,Type x01,Type x10,Type x11) { return sFade(v,sFade(u,x00,x01),sFade(u,x10,x11)); }

// the endian API will change - do not use yet!
// it also needs inline assembly
// LE assumes IA32 processor, where unaligned loads/stores do not crash

inline sU32 sSwapEndian(sU32 a)                                     {return ((a&0xff000000)>>24)|((a&0x00ff0000)>>8)|((a&0x0000ff00)<<8)|((a&0x000000ff)<<24); }
inline sU16 sSwapEndian(sU16 a)                                     {return ((a&0xff00)>>8)|((a&0x00ff)<<8); }
inline sU64 sSwapEndian(sU64 a)
{
  return ((a>>56)&0xff)|(((a>>48)&0xff)<<8)|(((a>>40)&0xff)<<16)|(((a>>32)&0xff)<<24)|(((a>>24)&0xff)<<32)|(((a>>16)&0xff)<<40)|(((a>>8 )&0xff)<<48)|((a&0xff)<<56);
}
inline void sSwapEndianI(sU32 &a)                                   {a= ((a&0xff000000)>>24)|((a&0x00ff0000)>>8)|((a&0x0000ff00)<<8)|((a&0x000000ff)<<24); }
inline void sSwapEndianI(sU16 &a)                                   {a= ((a&0xff00)>>8)|((a&0x00ff)<<8); }
inline void sSwapEndianI(sU64 &a)                                   {a= sSwapEndian(a); }
inline sInt sSwapEndian(sInt a)                                     {sU32 tmp = *(sU32*)&a; sSwapEndianI(tmp); a = *(sInt*)&tmp; return a;}
inline sF32 sSwapEndian(sF32 a)                                     {sU32 tmp = *(sU32*)&a; sSwapEndianI(tmp); a = *(sF32*)&tmp; return a;}
#if sCONFIG_BE
inline sU32 sSwapIfBE(sU32 a)                                       {return ((a&0xff000000)>>24)|((a&0x00ff0000)>>8)|((a&0x0000ff00)<<8)|((a&0x000000ff)<<24); }
inline sU32 sSwapIfLE(sU32 a)                                       {return a;}
inline sU16 sSwapIfBE(sU16 a)                                       {return ((a&0xff00)>>8)|((a&0x00ff)<<8); }
inline sU16 sSwapIfLE(sU16 a)                                       {return a;}
inline sU64 sSwapIfBE(sU64 a)                                       {return sU64(sSwapEndian(sU32(a)))<<32 | sU64(sSwapEndian(sU32(a>>32))); }
inline sU64 sSwapIfLE(sU64 a)                                       {return a;}
inline sInt sSwapIfBE(sInt a)                                       {return sSwapEndian(a);}
inline sInt sSwapIfLE(sInt a)                                       {return a;}
inline sF32 sSwapIfBE(sF32 a)                                       {return sSwapEndian(a);}
inline sF32 sSwapIfLE(sF32 a)                                       {return a;}

inline void sUnalignedLittleEndianStore16(sU8 *p,sU16 v)            { p[0]=sU8(v); p[1]=sU8(v>>8);}
inline void sUnalignedLittleEndianStore32(sU8 *p,sU32 v)            { p[0]=sU8(v); p[1]=sU8(v>>8); p[2]=sU8(v>>16); p[3]=sU8(v>>24);}
inline void sUnalignedLittleEndianStore64(sU8 *p,sU64 v)            { p[0]=sU8(v); p[1]=sU8(v>>8); p[2]=sU8(v>>16); p[3]=sU8(v>>24); p[0]=sU8(v>>32); p[1]=sU8(v>>40); p[2]=sU8(v>>48); p[3]=sU8(v>>56);}
inline void sUnalignedLittleEndianLoad16(const sU8 *p,sU16 &v)      { sU8 *s=(sU8*)&v;  s[0]=p[1]; s[1]=p[0]; }
inline void sUnalignedLittleEndianLoad32(const sU8 *p,sU32 &v)      { sU8 *s=(sU8*)&v;  s[0]=p[3]; s[1]=p[2];  s[2]=p[1];  s[3]=p[0]; }
inline void sUnalignedLittleEndianLoad64(const sU8 *p,sU64 &v)      { sU8 *s=(sU8*)&v;  s[0]=p[7]; s[1]=p[6];  s[2]=p[5];  s[3]=p[4];  s[4]=p[3]; s[5]=p[2];  s[6]=p[1];  s[7]=p[0]; }

#else
inline sU32 sSwapIfBE(sU32 a)                                       {return a; }
inline sU32 sSwapIfLE(sU32 a)                                       {return ((a&0xff000000)>>24)|((a&0x00ff0000)>>8)|((a&0x0000ff00)<<8)|((a&0x000000ff)<<24); }
inline sU16 sSwapIfBE(sU16 a)                                       {return a;}
inline sU16 sSwapIfLE(sU16 a)                                       {return ((a&0xff00)>>8)|((a&0x00ff)<<8); }
inline sU64 sSwapIfBE(sU64 a)                                       {return a;}
inline sU64 sSwapIfLE(sU64 a)                                       {return sU64(sSwapEndian(sU32(a)))<<32 | sU64(sSwapEndian(sU32(a>>32))); }
inline sInt sSwapIfBE(sInt a)                                       {return a;}
inline sInt sSwapIfLE(sInt a)                                       {return sSwapEndian(a);}
inline sF32 sSwapIfBE(sF32 a)                                       {return a;}
inline sF32 sSwapIfLE(sF32 a)                                       {return sSwapEndian(a);}
inline void sUnalignedLittleEndianStore16(sU8 *p,sU16 v)            {*(sU16 *)p = v;}
inline void sUnalignedLittleEndianStore32(sU8 *p,sU32 v)            {*(sU32 *)p = v;}
inline void sUnalignedLittleEndianLoad16(const sU8 *p,sU16 &v)      {v = *(sU16 *)p;}
inline void sUnalignedLittleEndianLoad32(const sU8 *p,sU32 &v)      {v = *(sU32 *)p;}
#if sCONFIG_UNALIGNEDCRASH
inline void sUnalignedLittleEndianStore64(sU8 *p,sU64 v)            {((sU32 *)p)[0]=v; ((sU32 *)p)[1]=v>>32;}
inline void sUnalignedLittleEndianLoad64(const sU8 *p,sU64 &v)      { sU32 a=((sU32*)p)[0],b=((sU32*)p)[1]; v = a+(sU64(b)<<32); }
#else
inline void sUnalignedLittleEndianStore64(sU8 *p,sU64 v)            {*(sU64 *)p = v;}
inline void sUnalignedLittleEndianLoad64(const sU8 *p,sU64 &v)      {v = *(sU64 *)p;}
#endif
#endif


// raw cast
template <typename T, sInt Bytes> struct sSwapHelper;
template <typename T0, typename T1> struct sRawCastHelper
{
  sRawCastHelper(T1 v1):V1(v1) { }
  union
  {
    T0 V0;
    T1 V1;
  };
};
template <typename T0, typename T1> T0 raw_cast(T1 v1) { sRawCastHelper<T0,T1> tmp(v1); return tmp.V0; }

// template endian swapper
// if this work on all compilers explicit swap permutations can be removed
template <typename T, sInt S> struct sSwapEndianHelper;
template <typename T> struct sSwapEndianHelper<T,1> { static T Swap(T a) { return a; } };
template <typename T> struct sSwapEndianHelper<T,2> { static T Swap(T a) { return raw_cast<T>(sSwapEndian(raw_cast<sU16>(a))); } };
template <typename T> struct sSwapEndianHelper<T,4> { static T Swap(T a) { return raw_cast<T>(sSwapEndian(raw_cast<sU32>(a))); } };
template <typename T> struct sSwapEndianHelper<T,8> { static T Swap(T a) { return raw_cast<T>(sSwapEndian(raw_cast<sU64>(a))); } };

#if sCONFIG_BE
template <typename T> T sSwapIfLE(T a) { return a; }
template <typename T> T sSwapIfBE(T a) { return sSwapEndianHelper<T,sizeof(T)>::Swap(a); }
#else
template <typename T> T sSwapIfLE(T a) { return sSwapEndianHelper<T,sizeof(T)>::Swap(a); }
template <typename T> T sSwapIfBE(T a) { return a; }
#endif

template<typename TO,typename FROM>TO &sRawCast(const FROM &src) { void *ptr = (void*)&src;  return *(TO*)ptr; }


/****************************************************************************/
/***                                                                      ***/
/***   Arithmetic Functions                                               ***/
/***                                                                      ***/
/****************************************************************************/

// int

sInt sMulDiv(sInt a,sInt b,sInt c);
sU32 sMulDivU(sU32 a,sU32 b,sU32 c);
sInt sMulShift(sInt var_a,sInt var_b);
sU32 sMulShiftU(sU32 var_a,sU32 var_b);
sInt sDivShift(sInt var_a,sInt var_b);
//sInt sDivMod(sInt over,sInt under,sInt &div,sInt &mod); // |over| = div*under+mod

// float fiddling

sF32 sAbs(sF32);                            // specialization for float
sF32 sMinF(sF32,sF32);                      // specialization for float
sF32 sMaxF(sF32,sF32);                      // specialization for float
void sDivMod(sF32 over,sF32 under,sF32 &div,sF32 &mod); // |over| = div*under+mod
sF32 sMod(sF32 over,sF32 under);
sF32 sAbsMod(sF32 over,sF32 under);         // values below zero will be modded to 0>=x<under as well.
sInt sAbsMod(sInt over,sInt under);         // values below zero will be modded to 0>=x<under as well.
void sSetFloat();                           // set floating point unit into default precision/rounding
void sFrac(sF32 val,sF32 &frac,sF32 &full); // val = frac+full; 0>=frac>1; full is int
void sFrac(sF32 val,sF32 &frac,sInt &full);
sF32 sFrac(sF32);
sF32 sRoundDown(sF32);                      // round towards minus infinity
sF32 sRoundUp(sF32);                        // round towards plus infinity
sF32 sRoundZero(sF32);                      // round towards zero
sF32 sRoundNear(sF32);                      // round towards nearest integer, in case of tie round to nearest even integer
sInt sRoundDownInt(sF32);
sInt sRoundUpInt(sF32);
sInt sRoundZeroInt(sF32);
sInt sRoundNearInt(sF32);
sF32 sSelectEQ(sF32 a,sF32 b,sF32 t,sF32 f);// if (a==b) then x=t else x=f;
sF32 sSelectNE(sF32 a,sF32 b,sF32 t,sF32 f);// if (a!=b) then x=t else x=f;
sF32 sSelectGT(sF32 a,sF32 b,sF32 t,sF32 f);// if (a>=b) then x=t else x=f;
sF32 sSelectGE(sF32 a,sF32 b,sF32 t,sF32 f);// if (a> b) then x=t else x=f;
sF32 sSelectLT(sF32 a,sF32 b,sF32 t,sF32 f);// if (a<=b) then x=t else x=f;
sF32 sSelectLE(sF32 a,sF32 b,sF32 t,sF32 f);// if (a< b) then x=t else x=f;
sF32 sSelectEQ(sF32 a,sF32 b,sF32 t);       // if (a==b) then x=t else x=0;
sF32 sSelectNE(sF32 a,sF32 b,sF32 t);       // if (a!=b) then x=t else x=0;
sF32 sSelectGT(sF32 a,sF32 b,sF32 t);       // if (a>=b) then x=t else x=0;
sF32 sSelectGE(sF32 a,sF32 b,sF32 t);       // if (a> b) then x=t else x=0;
sF32 sSelectLT(sF32 a,sF32 b,sF32 t);       // if (a<=b) then x=t else x=0;
sF32 sSelectLE(sF32 a,sF32 b,sF32 t);       // if (a< b) then x=t else x=0;

// non-trivial float

sF32 sSqrt(sF32);
sF32 sRSqrt(sF32);
sF32 sLog(sF32);
sF32 sLog2(sF32);
sF32 sLog10(sF32);
sF32 sExp(sF32);
sF32 sPow(sF32,sF32);

sF32 sSin(sF32);
sF32 sCos(sF32);
sF32 sTan(sF32);
void sSinCos(sF32,sF32 &s,sF32 &c);
sF32 sASin(sF32);
sF32 sACos(sF32);
sF32 sATan(sF32);
sF32 sATan2(sF32 over,sF32 under);

// non-trivial float, fast

sF32 sFSqrt(sF32);
sF32 sFRSqrt(sF32);
sF32 sFLog(sF32);
sF32 sFLog2(sF32);
sF32 sFLog10(sF32);
sF32 sFExp(sF32);
sF32 sFPow(sF32,sF32);

sF32 sFSin(sF32);
sF32 sFCos(sF32);
sF32 sFTan(sF32);
void sFSinCos(sF32,sF32 &s,sF32 &c);
sF32 sFASin(sF32);
sF32 sFACos(sF32);
sF32 sFATan(sF32);
sF32 sFATan2(sF32 over,sF32 under);

// some obsolete variants. obsolence not yet confirmed...

sINLINE sF32 /*sOBSOLETE*/ sFMod(sF32 a,sF32 b) { return sMod(a,b); }
sINLINE sF32 /*sOBSOLETE*/ sFFloor(sF32 a) { return sRoundDown(a); }
sINLINE sF32 /*sOBSOLETE*/ sFCeil(sF32 a) { return sRoundUp(a); }
sINLINE sF32 /*sOBSOLETE*/ sFAbs(sF32 a) { return sAbs(a); }
sINLINE sF32 /*sOBSOLETE*/ sFInvSqrt(sF32 a) { return sRSqrt(a); }

// int(floor(log2(abs(x)))) for a float x. Works on all machines with IEEE floating point
// arithmetic. Significantly faster than a "real" log2 followed by a float->int
// conversion on all platforms (even those with Load-Hit-Store penalties).
// If x is 0 or denormal, -127 is returned.
sINLINE sInt sFloorLog2(sF32 x) { union { sF32 f; sU32 i; } u; u.f = x; return sInt(((u.i >> 23) & 0xff) - 127); }

//sINLINE sInt sCountBits(sU32 bits) { sInt count = 0; while(bits) { if(bits&1) count++; bits >>=1; } return count; }
// improved version: clear least significant set bit each iteration
sINLINE sInt sCountBits(sU32 bits) { sInt count = 0; while(bits) { bits &= bits-1; count++; } return count; }

// Bit interleaving helpers
sU32 sPart1By1(sU32 x);     // "inserts" a 0 bit between each of the low 16 bits of x
sU32 sPart1By2(sU32 x);     // "inserts" two 0 bits between each of the low 10 bits of x
sU32 sCompact1By1(sU32 x);  // inverse of sPart1By1 - "delete" all odd-indexed bits
sU32 sCompact1By2(sU32 x);  // inverse of sPart1By2 - "delete" two bits after every bit

// Morton numbers: Bit-interleave two (or three) integer coordinates
sINLINE sU32 sEncodeMorton2(sU32 x,sU32 y)        { return (sPart1By1(y) << 1) + sPart1By1(x); }
sINLINE sU32 sEncodeMorton3(sU32 x,sU32 y,sU32 z) { return (sPart1By2(z) << 2) + (sPart1By2(y) << 1) + sPart1By2(x); }

sINLINE sU32 sDecodeMorton2X(sU32 index)          { return sCompact1By1(index >> 0); }
sINLINE sU32 sDecodeMorton2Y(sU32 index)          { return sCompact1By1(index >> 1); }
sINLINE sU32 sDecodeMorton2(sU32 index,sInt axis) { return sCompact1By1(index >> axis); }
sINLINE sU32 sDecodeMorton3X(sU32 index)          { return sCompact1By2(index >> 0); }
sINLINE sU32 sDecodeMorton3Y(sU32 index)          { return sCompact1By2(index >> 1); }
sINLINE sU32 sDecodeMorton3Z(sU32 index)          { return sCompact1By2(index >> 2); }
sINLINE sU32 sDecodeMorton3(sU32 index,sInt axis) { return sCompact1By2(index >> axis); }

/****************************************************************************/
/***                                                                      ***/
/***   Strings and Memory                                                 ***/
/***                                                                      ***/
/****************************************************************************/

// memory

void sSetMem(void *dd,sInt s,sInt c);
void sCopyMem(void *dd,const void *ss,sInt c);
void sMoveMem(void *dd,const void *ss,sInt c); // use for overlapping memory regions
sInt sCmpMem(const void *dd,const void *ss,sInt c);
template <class Type> void sClear(Type &p) { sSetMem(&p,0,sizeof(p)); }

struct sStringDesc                // this is always used as CONST, otherwise it won't work correctly.
{
  sChar *Buffer;
  sInt Size;
  sStringDesc(sChar *b,sInt s) : Buffer(b),Size(s) {}
  sStringDesc(const sStringDesc &s) : Buffer(s.Buffer),Size(s.Size) {}
  sStringDesc& operator=(const sStringDesc &s) { Buffer=s.Buffer; Size=s.Size; return*this;}
};

void sCopyString(sChar *d,const sChar *s,sInt size);  // Copy s to d, size is the destination buffer size (in sChars, not bytes)
inline void sCopyString(const sStringDesc &d, const sChar *s) { sCopyString(d.Buffer,s,d.Size); }

// use only if API functions don't support unicode
void sCopyString(sChar8* d, const sChar *s,sInt c);
void sCopyString(sChar* d, const sChar8* s,sInt c);
inline void sCopyString(const sStringDesc &d, const sChar8 *s) { sCopyString(d.Buffer,s,d.Size); }
void sCopyString(sChar8* d, const sChar8* s,sInt c);


void sCopyStringToUTF8(sChar8 *d, const sChar *s, sInt size);   //Encodes the 2byte unicode string into UTF-8 byte array
void sCopyStringFromUTF8(sChar *d, const sChar8 *s, sInt size); //Decodes the 0 terminated UTF-8 byte array into an 2byte unicode string. 'size' is the size of 'd'. (max. 3 byte utf-8 sequences supported!)

sChar sUpperChar(sChar c); // returns the uppercase character for c
sChar sLowerChar(sChar c); // returns the lowercase character for c
sChar *sMakeUpper(sChar *s); // converts s in place to uppercase and returns s
sChar *sMakeLower(sChar *s); // converts s in place to lowercase and returns s

sINLINE sInt sGetStringLen(const sChar *s)    { sInt i; for(i=0;s[i];i++) {} return i; }
void sAppendString(sChar *d,const sChar *s,sInt size);
inline void sAppendString(const sStringDesc &d,const sChar *s) { sAppendString(d.Buffer,s,d.Size); }
void sAppendString2(sChar *d,const sChar *s,sInt size,sInt len);
void sAppendPath(sChar *d,const sChar *s,sInt size);          // append strings, have exactly one '/' between them, handle absolute paths
inline void sAppendPath(const sStringDesc &d,const sChar *s) { sAppendPath(d.Buffer,s,d.Size); }
sInt sCmpString(const sChar *a,const sChar *b);               // normal string compare
sInt sCmpStringI(const sChar *a,const sChar *b);              // case insensitive 
sInt sCmpStringP(const sChar *a,const sChar *b);              // filename paths ('/'=='\', case insensitive)
sInt sCmpStringLen(const sChar *a,const sChar *b,sInt len);   // first characters match
sInt sCmpStringILen(const sChar *a,const sChar *b,sInt len);
sInt sCmpStringPLen(const sChar *a,const sChar *b,sInt len);
const sChar *sFindFileWithoutPath(const sChar *a);
sBool sIsAbsolutePath(const sChar *a);
void sExtractPath(const sChar *a, const sStringDesc &path);
void sExtractPath(const sChar *a, const sStringDesc &path, const sStringDesc &filename);
void sMakeRelativePath(const sStringDesc &path, const sChar *relTo); // make first path relative to second one if possible
sBool sExtractPathDrive(const sChar *path, const sStringDesc &drive);

sInt sFindString(const sChar *f, const sChar *s); // returns index of first occurrence of s in f or -1
sInt sFindStringI(const sChar *f, const sChar *s); // returns index of first case insensitive occurrence of s in f or -1
sInt sFindLastChar(const sChar *f,sInt c);
sInt sFindFirstChar(const sChar *f,sInt c);
sInt sFindNthChar(const sChar *f,sInt c, sInt n);
sInt sFindChar(const sChar *f,sChar character, sInt startPos=0); // returns index of first occurrence of character after startPos
const sChar *sFindFileExtension(const sChar *a);              // "ab.cd.ef" -> "ef"
sBool sExtractFileExtension(const sStringDesc &d, const sChar *a, sInt nr=0);  // "ab.cd.ef" -nr==0-> "ef", "ab.cd.ef" -nr==1-> "cd"
sChar *sFindFileExtension(sChar *a);
sBool sCheckFileExtension(const sChar *name,const sChar *ext); // example: ext = "mp.txt"
sBool sMatchWildcard(const sChar *wild,const sChar *text,sBool casesensitive=0,sBool pathsensitive=1);    // one recursion per '*'
sInt sCountChar(const sChar *str,sChar c);                    // useful as sCountChar(text,'\n');
sInt sFindChoice(const sChar *str,const sChar *choices);
sBool sFindFlag(const sChar *str,const sChar *choices,sInt &mask_,sInt &value_);
//sChar *sMakeChoice(sChar * choice, const sChar * choices, sInt index);
sInt sCountChoice(const sChar *choices);
sBool sCheckPrefix(const sChar *string,const sChar *prefix);  // ("1234","12") -> true
sBool sCheckSuffix(const sChar *string,const sChar *suffix);  // ("1234","34") -> true
sInt sReplaceChar(sChar *string, sChar from, sChar to);

void sReadString(sU32 *&data,sChar *buffer,sInt size);
inline void sReadString(sU32 *&data,const sStringDesc &desc) { sReadString(data,desc.Buffer,desc.Size); }
void sWriteString(sU32 *&data,const sChar *buffer);

inline sBool sIsDigit(sInt i) { return (i>='0' && i<='9'); }
inline sBool sIsLetter(sInt i) { return (i>='a' && i<='z') || (i>='A' && i<='Z') || i=='_'; }
inline sBool sIsSpace(sInt i) { return i==' ' || i=='\t' || i=='\r' || i=='\n'; }
inline sBool sIsHex(sInt i) { return (i>='a' && i<='f') || (i>='A' && i<='F') || (i>='0' && i<='9'); }
sBool sIsName(const sChar *);   // c conventions: letter followed by letter or digit
sU32 sHashString(const sChar *string);
sU32 sHashString(const sChar *string,sInt len);
sBool sScanInt(const sChar *&scan,sInt &val);
#if sCONFIG_64BIT
sBool sScanInt(const sChar *&scan,sDInt &val);
#endif
sBool sScanFloat(const sChar *&scan,sF32 &val);
sBool sScanHex(const sChar *&scan,sInt &val, sInt maxlen=0);
sBool sScanMatch(const sChar *&scan, const sChar *match);
sBool sScanGUID(const sChar *&str, struct sGUID &guid);
inline void sScanSpace(const sChar *&scan) { while(sIsSpace(*scan)) scan++; }

void sMakeChoice(const sStringDesc &,const sChar *choices,sInt index);

// get a stringdesc which will append to given string
sStringDesc sGetAppendDesc(const sStringDesc &sd);

/****************************************************************************/

// float / ascii conversion

struct sFloatInfo
{
  sChar Digits[20];               // zero-terminated digits, without decimal 
  sInt Exponent;                  // 1.0e0 -> Exponent = 0;
  sInt NaN;                       // if NaN, this holds all the NaN bits
  sBool Negative;                 // sign (1 or 0)
  sBool Denormal;                 // denormal detected during float->ascii conversion
  sBool Infinite;                 // infinite (1 or 0)

  void PrintE(const sStringDesc &desc); // print number in 1.2345e6 notation
  void PrintF(const sStringDesc &desc,sInt fractions); // print number in 0.0001234 notation

  // float -> ascii -> float roundtrips reproduce bitpattern 100% (at min 9 digits)

  void FloatToAscii(sU32 f,sInt digits=9);   
  sU32 AsciiToFloat();

  // double -> ascii -> double roundtips have a maximum error of 2ulp's (at min 17 digits)

  void DoubleToAscii(sU64 f,sInt digits=17); 
  sU64 AsciiToDouble();
};

/****************************************************************************/

sBool sFormatString(const sStringDesc &,const sChar *format,const sChar **fp); // return sFALSE if text is truncated!

/****************************************************************************/

struct sFormatStringInfo      // parses %n.mf, like %12.6f 
{
  sInt Format;                // f, f
  sInt Field;                 // n, 12
  sInt Fraction;              // m, 6 (-1 if not specified)
  sInt Minus;                 // starts with negative sign -> left aligned
  sInt Null;                  // starts with null character -> pad with zero
  sInt Truncate;              // starts with exclamation mark -> truncate output to field length if bigger
};

struct sFormatStringBuffer
{
  sChar *Start;
  sChar *Dest;
  sChar *End;
  const sChar *Format;

  sBool Fill();
  void GetInfo(sFormatStringInfo &);
  void Add(const sFormatStringInfo &info,const sChar *buffer,sBool sign);
  void Print(const sChar *str);
  template<typename Type> void PrintInt(const sFormatStringInfo &info,Type v,sBool sign);
  void PrintFloat(const sFormatStringInfo &info,sF32 v);
  void PrintFloat(const sFormatStringInfo &info,sF64 v);
  const sChar *Get() { return Start; }
};

sFormatStringBuffer sFormatStringBase(const sStringDesc &buffer,const sChar *format);
void sFormatStringBaseCtx(sFormatStringBuffer &buf,const sChar *format); // size of buffer is sPRINTBUFFERSIZE
sFormatStringBuffer& operator% (sFormatStringBuffer &,sInt);
sFormatStringBuffer& operator% (sFormatStringBuffer &,sDInt);
sFormatStringBuffer& operator% (sFormatStringBuffer &,sU32);
sFormatStringBuffer& operator% (sFormatStringBuffer &,sU64);
sFormatStringBuffer& operator% (sFormatStringBuffer &,sS64);
sFormatStringBuffer& operator% (sFormatStringBuffer &,void *);
//inline sFormatStringBuffer& operator% (sFormatStringBuffer & buf,sU32 v)     { return buf%(sInt)v; }
sFormatStringBuffer& operator% (sFormatStringBuffer &,sF32);
sFormatStringBuffer& operator% (sFormatStringBuffer &,sF64);
sFormatStringBuffer& operator% (sFormatStringBuffer &,const sChar *);

// we want to get rid of the variable arg thing and have a typesafe solution
// three examples:
//   sDPrintF(format,...)
//   sSPrintF(buffer,format,...)
//   sTextBuffer::PrintF(format,...)
// this has to be extended to templates (up to ten variable args):
//   template<typename A> sDPrintF(format,A a);
//   template<typename A,typename B> sDPrintF(format,A a,B b);
//   template<typename A,typename B,typename C> sDPrintF(format,A a,B b,C c);
// we don't want to type this everytime, especially since the template syntax is so lengthy
// that's what the sPRINTING macros are for:
//   sDPrintF(format,...) -> sPRINTING0
//   sSPrintF(buffer,format,...) -> sPRINTING1
//   sTextBuffer::PrintF(format,...) -> sPRINTING0
// how do we implement that?
//   #define sPRINTING0(Symbol,PreImpl,PostImpl)
//   #define sPRINTING1(Symbol,PreImpl,PostImpl,ArgType1)
//   #define sPRINTING2(Symbol,PreImpl,PostImpl,ArgType1,ArgType2)

#define sPRINTING0(Symbol,PreImpl,PostImpl) \
template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H,typename I,typename J> inline void Symbol(const sChar *format,A a,B b,C c,D d,E e,F f,G g,H h,I i,J j) { PreImpl%a%b%c%d%e%f%g%h%i%j; PostImpl } \
template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H,typename I           > inline void Symbol(const sChar *format,A a,B b,C c,D d,E e,F f,G g,H h,I i    ) { PreImpl%a%b%c%d%e%f%g%h%i  ; PostImpl } \
template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H                      > inline void Symbol(const sChar *format,A a,B b,C c,D d,E e,F f,G g,H h        ) { PreImpl%a%b%c%d%e%f%g%h    ; PostImpl } \
template<typename A,typename B,typename C,typename D,typename E,typename F,typename G                                 > inline void Symbol(const sChar *format,A a,B b,C c,D d,E e,F f,G g            ) { PreImpl%a%b%c%d%e%f%g      ; PostImpl } \
template<typename A,typename B,typename C,typename D,typename E,typename F                                            > inline void Symbol(const sChar *format,A a,B b,C c,D d,E e,F f                ) { PreImpl%a%b%c%d%e%f        ; PostImpl } \
template<typename A,typename B,typename C,typename D,typename E                                                       > inline void Symbol(const sChar *format,A a,B b,C c,D d,E e                    ) { PreImpl%a%b%c%d%e          ; PostImpl } \
template<typename A,typename B,typename C,typename D                                                                  > inline void Symbol(const sChar *format,A a,B b,C c,D d                        ) { PreImpl%a%b%c%d            ; PostImpl } \
template<typename A,typename B,typename C                                                                             > inline void Symbol(const sChar *format,A a,B b,C c                            ) { PreImpl%a%b%c              ; PostImpl } \
template<typename A,typename B                                                                                        > inline void Symbol(const sChar *format,A a,B b                                ) { PreImpl%a%b                ; PostImpl } \
template<typename A                                                                                                   > inline void Symbol(const sChar *format,A a                                    ) { PreImpl%a                  ; PostImpl } \
/*                                                                                                                   */ inline void Symbol(const sChar *format                                        ) { PreImpl%0                  ; PostImpl }

#define sPRINTING0R(rtype,Symbol,PreImpl,PostImpl) \
template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H,typename I,typename J> inline rtype Symbol(const sChar *format,A a,B b,C c,D d,E e,F f,G g,H h,I i,J j) { PreImpl%a%b%c%d%e%f%g%h%i%j; PostImpl } \
template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H,typename I           > inline rtype Symbol(const sChar *format,A a,B b,C c,D d,E e,F f,G g,H h,I i    ) { PreImpl%a%b%c%d%e%f%g%h%i  ; PostImpl } \
template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H                      > inline rtype Symbol(const sChar *format,A a,B b,C c,D d,E e,F f,G g,H h        ) { PreImpl%a%b%c%d%e%f%g%h    ; PostImpl } \
template<typename A,typename B,typename C,typename D,typename E,typename F,typename G                                 > inline rtype Symbol(const sChar *format,A a,B b,C c,D d,E e,F f,G g            ) { PreImpl%a%b%c%d%e%f%g      ; PostImpl } \
template<typename A,typename B,typename C,typename D,typename E,typename F                                            > inline rtype Symbol(const sChar *format,A a,B b,C c,D d,E e,F f                ) { PreImpl%a%b%c%d%e%f        ; PostImpl } \
template<typename A,typename B,typename C,typename D,typename E                                                       > inline rtype Symbol(const sChar *format,A a,B b,C c,D d,E e                    ) { PreImpl%a%b%c%d%e          ; PostImpl } \
template<typename A,typename B,typename C,typename D                                                                  > inline rtype Symbol(const sChar *format,A a,B b,C c,D d                        ) { PreImpl%a%b%c%d            ; PostImpl } \
template<typename A,typename B,typename C                                                                             > inline rtype Symbol(const sChar *format,A a,B b,C c                            ) { PreImpl%a%b%c              ; PostImpl } \
template<typename A,typename B                                                                                        > inline rtype Symbol(const sChar *format,A a,B b                                ) { PreImpl%a%b                ; PostImpl } \
template<typename A                                                                                                   > inline rtype Symbol(const sChar *format,A a                                    ) { PreImpl%a                  ; PostImpl } \
/*                                                                                                                   */ inline rtype Symbol(const sChar *format                                        ) { PreImpl%0                  ; PostImpl }

#define sPRINTING1(Symbol,PreImpl,PostImpl,ArgType1) \
template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H,typename I,typename J> inline void Symbol(ArgType1 arg1,const sChar *format,A a,B b,C c,D d,E e,F f,G g,H h,I i,J j) { PreImpl%a%b%c%d%e%f%g%h%i%j; PostImpl } \
template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H,typename I           > inline void Symbol(ArgType1 arg1,const sChar *format,A a,B b,C c,D d,E e,F f,G g,H h,I i    ) { PreImpl%a%b%c%d%e%f%g%h%i  ; PostImpl } \
template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H                      > inline void Symbol(ArgType1 arg1,const sChar *format,A a,B b,C c,D d,E e,F f,G g,H h        ) { PreImpl%a%b%c%d%e%f%g%h    ; PostImpl } \
template<typename A,typename B,typename C,typename D,typename E,typename F,typename G                                 > inline void Symbol(ArgType1 arg1,const sChar *format,A a,B b,C c,D d,E e,F f,G g            ) { PreImpl%a%b%c%d%e%f%g      ; PostImpl } \
template<typename A,typename B,typename C,typename D,typename E,typename F                                            > inline void Symbol(ArgType1 arg1,const sChar *format,A a,B b,C c,D d,E e,F f                ) { PreImpl%a%b%c%d%e%f        ; PostImpl } \
template<typename A,typename B,typename C,typename D,typename E                                                       > inline void Symbol(ArgType1 arg1,const sChar *format,A a,B b,C c,D d,E e                    ) { PreImpl%a%b%c%d%e          ; PostImpl } \
template<typename A,typename B,typename C,typename D                                                                  > inline void Symbol(ArgType1 arg1,const sChar *format,A a,B b,C c,D d                        ) { PreImpl%a%b%c%d            ; PostImpl } \
template<typename A,typename B,typename C                                                                             > inline void Symbol(ArgType1 arg1,const sChar *format,A a,B b,C c                            ) { PreImpl%a%b%c              ; PostImpl } \
template<typename A,typename B                                                                                        > inline void Symbol(ArgType1 arg1,const sChar *format,A a,B b                                ) { PreImpl%a%b                ; PostImpl } \
template<typename A                                                                                                   > inline void Symbol(ArgType1 arg1,const sChar *format,A a                                    ) { PreImpl%a                  ; PostImpl } \
/*                                                                                                                   */ inline void Symbol(ArgType1 arg1,const sChar *format                                        ) { PreImpl%0                 ; PostImpl }

#define sPRINTING2(Symbol,PreImpl,PostImpl,ArgType1,ArgType2) \
template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H,typename I,typename J> inline void Symbol(ArgType1 arg1,ArgType2 arg2,const sChar *format,A a,B b,C c,D d,E e,F f,G g,H h,I i,J j) { PreImpl%a%b%c%d%e%f%g%h%i%j; PostImpl } \
template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H,typename I           > inline void Symbol(ArgType1 arg1,ArgType2 arg2,const sChar *format,A a,B b,C c,D d,E e,F f,G g,H h,I i    ) { PreImpl%a%b%c%d%e%f%g%h%i  ; PostImpl } \
template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H                      > inline void Symbol(ArgType1 arg1,ArgType2 arg2,const sChar *format,A a,B b,C c,D d,E e,F f,G g,H h        ) { PreImpl%a%b%c%d%e%f%g%h    ; PostImpl } \
template<typename A,typename B,typename C,typename D,typename E,typename F,typename G                                 > inline void Symbol(ArgType1 arg1,ArgType2 arg2,const sChar *format,A a,B b,C c,D d,E e,F f,G g            ) { PreImpl%a%b%c%d%e%f%g      ; PostImpl } \
template<typename A,typename B,typename C,typename D,typename E,typename F                                            > inline void Symbol(ArgType1 arg1,ArgType2 arg2,const sChar *format,A a,B b,C c,D d,E e,F f                ) { PreImpl%a%b%c%d%e%f        ; PostImpl } \
template<typename A,typename B,typename C,typename D,typename E                                                       > inline void Symbol(ArgType1 arg1,ArgType2 arg2,const sChar *format,A a,B b,C c,D d,E e                    ) { PreImpl%a%b%c%d%e          ; PostImpl } \
template<typename A,typename B,typename C,typename D                                                                  > inline void Symbol(ArgType1 arg1,ArgType2 arg2,const sChar *format,A a,B b,C c,D d                        ) { PreImpl%a%b%c%d            ; PostImpl } \
template<typename A,typename B,typename C                                                                             > inline void Symbol(ArgType1 arg1,ArgType2 arg2,const sChar *format,A a,B b,C c                            ) { PreImpl%a%b%c              ; PostImpl } \
template<typename A,typename B                                                                                        > inline void Symbol(ArgType1 arg1,ArgType2 arg2,const sChar *format,A a,B b                                ) { PreImpl%a%b                ; PostImpl } \
template<typename A                                                                                                   > inline void Symbol(ArgType1 arg1,ArgType2 arg2,const sChar *format,A a                                    ) { PreImpl%a                  ; PostImpl } \
/*                                                                                                                   */ inline void Symbol(ArgType1 arg1,ArgType2 arg2,const sChar *format                                        ) { PreImpl%0                  ; PostImpl }

/****************************************************************************/

struct sPoolString;

template<int size> struct sString
{
protected:
  sChar Buffer[size];
public:
  sString()                    { Buffer[0] = 0; }
  ~sString()                    { Buffer[0] = 1; }
  sString(const sChar *s)      { if (s) sCopyString(Buffer,s,size); else Buffer[0]=0; }
  sString(const sChar *s,sInt len) { if (s) sCopyString(Buffer,s,sMin(size,len)); else Buffer[0]=0; }
  sString(const sString &s)    { sCopyString(Buffer,s.Buffer,size); }
  template<int n> sString(const sString<n> &s) { sCopyString(Buffer,s,size); }
  sString(const sPoolString &s);
  sString &operator=(const sChar *s) { if (s) sCopyString(Buffer,s,size); else Buffer[0]=0; return *this; }
  template<int n> sString &operator=(const sString<n> &s) { if(s) sCopyString(Buffer,s,size); else Buffer[0]=0; return *this; }
  sString &operator=(const struct sPoolString &s);
  operator sChar*()            { return Buffer; }
  operator const sChar*() const { return Buffer; }
  operator sStringDesc()       { return sStringDesc(Buffer,size); }
  sBool operator==(const sChar *s) const { return sCmpString(Buffer,s)==0; }
  sBool operator==(const sString &s) const { return sCmpString(Buffer,s.Buffer)==0; }
  sBool operator==(const sPoolString &s) const;
  sBool operator!=(const sChar *s) const { return sCmpString(Buffer,s)!=0; }
  sBool operator!=(const sString &s) const { return sCmpString(Buffer,s.Buffer)!=0; }
  sBool operator!=(const sPoolString &s) const;
  sBool operator>(const sChar *s) const { return sCmpString(Buffer,s)>0; }
  sBool operator>(const sString &s) const { return sCmpString(Buffer,s.Buffer)>0; }
  sBool operator>(const sPoolString &s) const;
  sBool operator<(const sChar *s) const { return sCmpString(Buffer,s)<0; }
  sBool operator<(const sString &s) const { return sCmpString(Buffer,s.Buffer)<0; }
  sBool operator<(const sPoolString &s) const;
  const sChar &operator[](sInt i) const { return Buffer[i]; }
  sChar &operator[](sInt i)    { return Buffer[i]; }
  sInt Count() const           { return sGetStringLen(Buffer); }
  sBool IsEmpty() const        { return Buffer[0]==0; }
  sInt Size() const            { return size; }
  void Clear()                 { Buffer[0]=0; }

  void Init(const sChar *a)    { sInt n = sMin(size-1,sGetStringLen(a)); sCopyMem(Buffer,a,n*sizeof(sChar)); Buffer[n]=0; }
  void Init(const sChar *a,sInt len) { sInt n = sMin(size-1,len); sCopyMem(Buffer,a,n*sizeof(sChar)); Buffer[n]=0; }
  sString &operator+=(const sChar *a) { sAppendString(Buffer,a,size); return *this; }
  void Add(const sChar *a)     { sAppendString(Buffer,a,size); }
  void Add(const sChar *a,sInt len) { sAppendString2(Buffer,a,size,len); }
  void Add(const sChar *a,const sChar *b) { sAppendString(Buffer,a,b,size); }
  void Add(sInt ch) { sInt len = Count(); if(len+1<size) Buffer[len]=(sChar)ch; Buffer[len+1]=0; }
  void AddPath(const sChar *a) { sAppendPath(Buffer,a,size); }

  void PadRight(sInt length, sChar c=L' ') { sInt l=Count(); while (l<sMin(length, size)) Buffer[l++]=c; Buffer[l]=0; };
  void PadLeft(sInt length, sChar c=L' ')  { sInt l=sMin(length,size); if (Count()<l) { sInt o=l-Count(); for (sInt i=l; i>=0; i--) Buffer[i] = ((i-o >= 0) ? Buffer[i-o] : c); } }
  void StripRight(const sChar * chars=L" \n\t")  { sInt l=Count(); while (l && sFindFirstChar(chars, Buffer[l-1]) >= 0) { Buffer[l-1] = 0; l--; } }
  void StripLeft(const sChar * chars=L" \n\t")   { sInt s=0; while (sFindFirstChar(chars, Buffer[s]) >= 0) s++; if (s > 0) CutLeft(s); }
  void Strip(const sChar * chars=L" \n\t")       { StripLeft(chars); StripRight(chars); }
  void CutLeft(sInt length)                { CutRange(0,length); }
  void CutRight(sInt length)               { if (length>0) Buffer[sMax(0,Count()-length)]=0; }
  void CutRange(sInt start, sInt length)   { sInt end=start+length; if (end<start) sSwap(end,start); sInt l=Count(); sInt i=sClamp(start,0,l); sInt n=sClamp(end,0,l); while (Buffer[n]) Buffer[i++]=Buffer[n++]; Buffer[i]=0; }

  sPRINTING0(PrintF,sFormatStringBuffer buf=sFormatStringBase(sStringDesc(Buffer,size),format);buf,;);
  sPRINTING0(PrintAddF,sFormatStringBuffer buf=sFormatStringBase(sStringDesc(Buffer+sGetStringLen(Buffer),size-sGetStringLen(Buffer)),format);buf,;);
};

/****************************************************************************/
/***                                                                      ***/
/***   Debugging                                                          ***/
/***                                                                      ***/
/****************************************************************************/

sNORETURN void sVerifyFalse(const sChar *file,sInt line);
sNORETURN void sVerifyFalse2(const sChar *file,sInt line,const sChar *expr,const sChar *desc);

void sPrint(const sChar *text);
void sPrintError(const sChar *text);        // colored output
void sPrintWarning(const sChar *text);      // colored output
extern void (*sRedirectStdOut)(const sChar *text);  // sPrintF will be send here instead
void sHexDump(const void *data,sInt bytes); // dumps data as 32bit words
void sHexByteDump(const void *data,sInt bytes); // dumps data as bytes
void sEnableLogFilter();
sBool sCheckLogFilter(const sChar *module);

sPRINTING1(sSPrintF,sFormatStringBuffer buf=sFormatStringBase(arg1,format);buf,;,const sStringDesc &)

sPRINTING0(sPrintF,sFormatStringBuffer buf; sFormatStringBaseCtx(buf,format);buf,sPrint(buf.Get());)
sPRINTING0(sPrintErrorF,sFormatStringBuffer buf; sFormatStringBaseCtx(buf,format);buf,sPrintError(buf.Get());)
sPRINTING0(sPrintWarningF,sFormatStringBuffer buf; sFormatStringBaseCtx(buf,format);buf,sPrintWarning(buf.Get());)

#define sENABLE_DPRINT !sSTRIPPED
//#define sENABLE_DPRINT 1

#if sENABLE_DPRINT
void sDPrint(const sChar *text);
sPRINTING0(sDPrintF,sFormatStringBuffer buf; sFormatStringBaseCtx(buf,format);buf,sDPrint(buf.Get());)
#else
#define sDPrintF
#define sDPrint
#endif

#if sENABLE_DPRINT
extern void (*sPrintScreenCB)(const sChar *text);
void sPrintScreen(const sChar *text);
sPRINTING0(sPrintScreenF,sFormatStringBuffer buf; if(format) sFormatStringBaseCtx(buf,format);if(format) buf,sPrintScreen(format?buf.Get():0);)
#else
#define sPrintScreenF
#define sPrintScreen
#endif

// set to sTRUE to log to console instead of debug output. useful when
// writing test cases.
void sEnableLogConsole(sBool enable);

// cant define away because of sLogF(float);

#if sENABLE_DPRINT
void sLog(const sChar *module,const sChar *text);
sPRINTING1(sLogF,sFormatStringBuffer buf; sFormatStringBaseCtx(buf,format);buf,sLog(arg1,buf.Get());,const sChar *)
#else
inline void sLog(const sChar *m,const sChar *n) {}
template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H,typename I,typename J> inline void sLogF(const sChar *arg1,const sChar *format,A a,B b,C c,D d,E e,F f,G g,H h,I i,J j) {}
template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H,typename I           > inline void sLogF(const sChar *arg1,const sChar *format,A a,B b,C c,D d,E e,F f,G g,H h,I i    ) {}
template<typename A,typename B,typename C,typename D,typename E,typename F,typename G,typename H                      > inline void sLogF(const sChar *arg1,const sChar *format,A a,B b,C c,D d,E e,F f,G g,H h        ) {}
template<typename A,typename B,typename C,typename D,typename E,typename F,typename G                                 > inline void sLogF(const sChar *arg1,const sChar *format,A a,B b,C c,D d,E e,F f,G g            ) {}
template<typename A,typename B,typename C,typename D,typename E,typename F                                            > inline void sLogF(const sChar *arg1,const sChar *format,A a,B b,C c,D d,E e,F f                ) {}
template<typename A,typename B,typename C,typename D,typename E                                                       > inline void sLogF(const sChar *arg1,const sChar *format,A a,B b,C c,D d,E e                    ) {}
template<typename A,typename B,typename C,typename D                                                                  > inline void sLogF(const sChar *arg1,const sChar *format,A a,B b,C c,D d                        ) {}
template<typename A,typename B,typename C                                                                             > inline void sLogF(const sChar *arg1,const sChar *format,A a,B b,C c                            ) {}
template<typename A,typename B                                                                                        > inline void sLogF(const sChar *arg1,const sChar *format,A a,B b                                ) {}
template<typename A                                                                                                   > inline void sLogF(const sChar *arg1,const sChar *format,A a                                    ) {}
/*                                                                                                                   */ inline void sLogF(const sChar *arg1,const sChar *format                                        ) {}
#endif

sNORETURN void sCDECL sFatalImpl(const sChar *str);
sPRINTING0(sFatal,sFormatStringBuffer buf; sFormatStringBaseCtx(buf,format);buf,sFatalImpl(buf.Get()););

#if sCONFIG_COMPILER_CWCC
#define sVERIFYSTATIC(x) { bool STATIC_ASSERTION_[(x)?1:-1]; }
#elif sCONFIG_COMPILER_SNC
#define sVERIFYSTATIC(x) {  }
#else
#define sVERIFYSTATIC(x) { struct STATIC_ASSERTION {bool STATIC_ASSERTION_[(x)?1:-1]; }; }
#endif

#define sTOSTRING8_I(x) #x
#define sTOSTRING8(x) sTOSTRING8_I(x)


#if sDEBUG

#if sRELEASE
#define sVERIFY(x) {if(!(x))sVerifyFalse2(sTXT(__FILE__),__LINE__,sTXT(sTOSTRING8(x)),L"");}
#define sVERIFY2(x,desc) {if(!(x))sVerifyFalse2(sTXT(__FILE__),__LINE__,sTXT(sTOSTRING8(x)),desc);}
#define sVERIFYFALSE {sVerifyFalse(sTXT(__FILE__),__LINE__);}
#else
#define sVERIFY(x) {if(!(x)){ sDEBUGBREAK ;sVerifyFalse2(sTXT(__FILE__),__LINE__,sTXT(sTOSTRING8(x)),L"");}}
#define sVERIFY2(x,desc) {if(!(x)){ sDEBUGBREAK ;sVerifyFalse2(sTXT(__FILE__),__LINE__,sTXT(sTOSTRING8(x)),desc);}}
#define sVERIFYFALSE { sDEBUGBREAK ;sVerifyFalse(sTXT(__FILE__),__LINE__);}
#endif

#else
#define sVERIFY(x) {(void)sizeof(x); }
#define sVERIFY2(x,desc) { (void)sizeof(x); }
#define sVERIFYFALSE {;}
#endif


#if sRELEASE
#define sVERIFYRELEASE(x) {if(!(x))sVerifyFalse(sTXT(__FILE__),__LINE__);}
#else
#define sVERIFYRELEASE(x) {if(!(x)){ sDEBUGBREAK ;sVerifyFalse(sTXT(__FILE__),__LINE__);}}
#endif



#if defined(sTRACEON)
#define sTRACE() sDPrintF(L"%s(%d):Trace\n",sTXT(__FILE__),__LINE__);
#else
#define sTRACE() ;
#endif

// helpers for managing not implemented stuff, useful for porting
#if sCONFIG_COMPILER_MSC

#define sNOT_IMPLEMENTED()    \
  __pragma (message(__FILE__"("sTOSTRING8(__LINE__)"): TODO implement "__FUNCTION__))    \
  sDPrintF(sTXT(__FILE__)L"(%d): TODO implement "sTXT(__FUNCTION__)L"\n",__LINE__);

#define sNOT_IMPLEMENTED_FAIL()    \
  __pragma (message(__FILE__"("sTOSTRING8(__LINE__)"): TODO implement "__FUNCTION__))    \
  sVERIFY(0);
#ifndef NO_TODO
#define sTODO(msg)  \
  __pragma (message(__FILE__"("sTOSTRING8(__LINE__)"): TODO "msg))
#else
#define sTODO(msg)
#endif

#else
#define sNOT_IMPLEMENTED()  \
  sDPrintF(sTXT(__FILE__)L"(%d): TODO implement "sTXT(__FUNCTION__)L"\n",__LINE__);
#define sNOT_IMPLEMENTED_FAIL()    \
  sDPrintF(sTXT(__FILE__)L"(%d): TODO implement "sTXT(__FUNCTION__)L"\n",__LINE__);   \
  sVERIFY(0);
#define sTODO(msg)
#endif

/****************************************************************************/
/***                                                                      ***/
/***   NaN and inf traps for sF32                                         ***/
/***                                                                      ***/
/****************************************************************************/

// only use these traps to check for infinites or nans during a debug session.
// never write code that "fixes" those nans on the fly. write code that
// does not produce nans or infinites.

#define sF32U32(x) ((*(sU32*)(&(x))))

inline sBool sIsInfNan(sF32 x) { return ((sF32U32(x)>>23)&0xff) == 0xff; }
inline sBool sIsNan(sF32 x) { return ((sF32U32(x)>>23)&0xff) == 0xff && (sF32U32(x) & 0x7fffff)!=0; }
inline sBool sIsInf(sF32 x) { return ((sF32U32(x)>>23)&0xff) == 0xff && (sF32U32(x) & 0x7fffff)==0; }

#if sDEBUG
#define sTRAP_INFNAN(x) sVERIFY(!sIsInfNan(x))
#define sTRAP_NAN(x) sVERIFY(!sIsNan(x))
#define sTRAP_INF(x) sVERIFY(!sIsInf(x))
#else
#define sTRAP_INFNAN(x)
#define sTRAP_NAN(x)
#define sTRAP_INF(x)
#endif

// returns ptr to broken float
#if sDEBUG
const sF32 *sCheckFloatArray(const sF32 *ptr, sInt count);
#else
  sINLINE const sF32 *sCheckFloatArray(const sF32 *, sInt ) { return sNULL; }
#endif

template<typename T>
const sF32 *sCheckFloats(const T &t)
{
#if sDEBUG
  return sCheckFloatArray((const sF32 *)&t, sizeof(T)/sizeof(sF32));
#else
  return sNULL;
#endif
}

template<typename T>
inline void sCheckFloatsFatal(const T &t, const sChar *message=sNULL)
{
#if sDEBUG
  const sF32 *src = (const sF32 *)&t;
  const sF32 *ptr = sCheckFloatArray(src, sizeof(T)/sizeof(sF32));
  if (ptr)
  {
    sString<64> messageDef;
    sInt index = (sInt)(ptr - src);
    messageDef.PrintF(L"Inf/NaN in data. (float[%d] at 0x%x (+0x%x) )\n", index, (sPtr)src, index * (sInt)sizeof(sF32));
    sLogF(L"chkflt", messageDef);
    sFatal(message ? message : (const sChar*)messageDef);
  }
#endif
}

/****************************************************************************/
/***                                                                      ***/
/***   Memory Management                                                  ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   all allocations go through sAllocMem_(size,align,flags).           ***/
/***                                                                      ***/
/***   operator new is overloaded to take the flags parameter, and        ***/
/***   possibly __FILE__ and __LINE__.                                    ***/
/***                                                                      ***/
/***   sInitMem() is optional, sAllocMem_() works before sInitMem().      ***/
/***   it can be used to specify debugging options.                       ***/
/***                                                                      ***/
/***   on PC, the RTL heap is used by default and sInitMem() can be used  ***/
/***   to switch to altona's heap.                                        ***/
/***                                                                      ***/
/****************************************************************************/

#if sSTRIPPED
#define sCONFIG_DEBUGMEM 0
#elif sRELEASE && !(sPLATFORM==sPLAT_LINUX)
#define sCONFIG_DEBUGMEM 1
#else
#define sCONFIG_DEBUGMEM 1
#endif

enum sAllocMemFlags               // sAllocMem() flags
{
  sAMF_DEFAULT    = 0x0000,       // default memory type, set by sPushMemType() / sPopMemType()
  sAMF_HEAP       = 0x0001,       // normal heap memory
  sAMF_DEBUG      = 0x0002,       // debug memory (not available in stripped builds)
  sAMF_GFX        = 0x0003,       // platform dependent behaviour.
  sAMF_NETWORK    = 0x0004,       // used by network thread

  sAMF_XTRA1      = 0x0005,       // 
  sAMF_XTRA2      = 0x0006,       // 

  sAMF_MASK       = 0x000f,
  sAMF_ALT        = 0x0100,       // from end of heap, not beginning (alternative direction)
  sAMF_NOLEAK     = 0x0200,       // exclude from leak tracking. used by leaktracker itself.
  sAMF_MAYFAIL    = 0x0400,       // return 0 on out-of-mem. usually, out-of-mem will assert
};

enum sInitMemFlags                // sInitMem() flags
{
  sIMF_DEBUG        = 0x0001,     // use devkit-extra memory as debug memory (ignored in stripped builds)
  sIMF_LARGE        = 0x0002,     // use devkit-extra memory as heap memory (ignored in stripped builds)
  sIMF_NORTL        = 0x0004,     // pc only: use altona heap instead of RTL heap.
  sIMF_CLEAR        = 0x0008,     // debugging: clear memory on allocation and freeing
  sIMF_NOLEAKTRACK  = 0x0010,     // prevent initialization of leaktracker
};

class sMemoryHandler              // used by system to register memory handlers
{
  sBool ThreadSafe;               // do locking (done by sAllocMem(), ..)
  class sThreadLock *Lock_;
public:
  sPtr Start;                     // for Free(): range of memory this handler reacts on
  sPtr End;
  sPtr MinTotalFree;              // lowest total free memory
  struct sThreadContext *Owner;   // only this context may use the handler (checked by sAllocMem(), ..)
  sBool IncludeInSnapshot;        // some heaps may change between snapshots, like network and debug
  sBool MayFail;                  // out of mem condition is passed to app instead of failing
  sBool NoLeaks;                  // disable all memory leak tracking

  sMemoryHandler();
  virtual ~sMemoryHandler();

  // allocation
  virtual void *Alloc(sPtr size,sInt align,sInt flags)=0;   // alloc mem
  virtual sBool Free(void *)=0;   // free mem
  virtual sPtr MemSize(void *) { return 0; } // exact size of allocation
  virtual void Validate() {}      // check integrity of memory lists
  virtual sU32 MakeSnapshot() { return 0; }

  // statistics
  virtual sCONFIG_SIZET GetSize() { return End-Start; }
  virtual sCONFIG_SIZET GetFree() { return 0; }        // returns total free memory (or always 0 if not applicable)
  virtual sCONFIG_SIZET GetLargestFree() { return 0; } // returns largest possible allocation block (or always 0 if not applicable)

  sBool Contains(void *ptr) { return sPtr(ptr)>=Start&&sPtr(ptr)<End; }

  // thread safety
  void MakeThreadSafe();          // set up threadsafe locking
  virtual void MakeThreadUnsafe();// sometimes you need always thread safe memory
                                  // therefore this method is virtual to define empty implementation
                                  // (dtor and sExitMem directly call sMemoryHandler::MakeThreadUnsafe)
  sBool IsThreadSafe() { return ThreadSafe; }
  void Lock();
  sBool TryLock();
  void Unlock();
};

/****************************************************************************/

extern sPtr sMemoryUsed;
extern sInt sMemoryInitFlags;
extern sPtr sMemoryInitSize;
extern sInt sMemoryInitCount;
extern sPtr sMemoryInitDebugSize;

#define sINITMEM(f,s) struct sInitMem_ { sInitMem_(sInt flags,sPtr size){ sMemoryInitFlags=flags; sMemoryInitSize=size; }} sInitMem__(f,s); 
#define sINITDEBUGMEM(s) struct sInitDebugMem_ { sInitDebugMem_(sPtr size){ sMemoryInitFlags|=sIMF_DEBUG; sMemoryInitDebugSize=size; }} sInitDebugMem__(s);

void sInitMem0();                 // called automatically to initialize basic heap
void sExitMem0();                 // called automatically to shut down and check heap
void sInitMem1();                 // platform dependent implementation
void sInitMem2(sPtr gfx);         // platform dependent implementation
void sExitMem1();                 // platform dependent implementation
void sRegisterMemHandler(sInt slot,sMemoryHandler *);
void sUnregisterMemHandler(sInt slot);
sMemoryHandler *sGetMemHandler(sInt slot);

void *sAllocMem_(sPtr size,sInt align,sInt flags=0);  // malloc()
void sFreeMem(void *ptr);         // free()
sPtr sMemSize(void *ptr);         // returns size of allocated memory
sBool sIsMemTypeAvailable(sInt);  // check if mem of a certain type is available at all
sCONFIG_SIZET sGetFreeMem(sInt);  // get free memory for type (0 if not available)
void sPushMemType(sInt);          // set default memtype (sAMF_DEFAULT)
void sPopMemType(sInt);           // restore old default memtype. use same argument as in sPushMemType().
inline void sBeginAlt() { sPushMemType(sAMF_HEAP|sAMF_ALT); }
inline void sEndAlt() { sPopMemType(sAMF_HEAP|sAMF_ALT); }

void sSetInfoForAlloc(void *ptr, const sChar *infostring);


void sMemMark(sBool fatal=sTRUE);  // first call: set memmark. subsequent call: restore memmark
void sResetMemChecksum();          // reset the checksum for memory management. 
void sMemMarkPush(sInt id=-1);   // pushes the current memmark on the memmarkstack
void sMemMarkPop();             // pops a memmark from the stack copies it to the current memmark (but saves the reset-flag)
sInt sMemMarkGetId();
sBool sIsMemMarkSet();
void sAddMemMarkCallback(void (*cb)(sBool flush,void *user),void *user=0);

void sBreakOnAllocation(sInt n);   // one id for all 
sInt sGetMemoryAllocId();          // usefull for monitoring allocations per frame
void sCheckMem_();                 // check memory lists for inconsistencies 
void sDumpMemoryMap();             // print a summary of all memory handlers
void sSetAllocFailTest(sF32 probability); // set probability that allocs from "may fail" heaps fail randomly.
sF32 sGetAllocFailTest();           // return fail probabality

//! memory heaps assert when freeing locked memory ranges 
//! enable this with sCFG_MEMDBG_LOCKING define in types.cpp for heap corruption debugging
//! eg. async file reading into freed memory
void sMemDbgLock(sPtr start, sPtr size);
void sMemDbgUnlock(sPtr start, sPtr size);
sBool sMemDbgCheckLock(void *ptr);

#if sSTRIPPED
#define sScopeNoMemFailTest()
#else
struct sScopeNoMemFailTest_
{
  sF32 Probality;
  sScopeNoMemFailTest_() { Probality = sGetAllocFailTest(); sSetAllocFailTest(0.0f); }
  ~sScopeNoMemFailTest_() { sSetAllocFailTest(Probality); }
};
#define sScopeNoMemFailTest() sScopeNoMemFailTest_ noscopememfailtest##__LINE__;
#endif

struct sScopeMem_
{
  sInt Flags;

  sScopeMem_(sInt flags) { Flags = flags; sPushMemType(flags); }
  ~sScopeMem_() { sPopMemType(Flags); }
};

#define sScopeMem(flags) sScopeMem_ usemem##__LINE__(flags);

/****************************************************************************/

// memory debugging 

#if sCONFIG_DEBUGMEM
void sTagMem(const char *file,sInt line);
void sTagMemLast();
#else
inline void sTagMem(const char *file,sInt line) {  }
inline void sTagMemLast() { }
#endif

#if sCONFIG_DEBUGMEM
inline void *sAllocMem_(sPtr size,sInt align,sInt flags,const char *file,sInt line) { sTagMem(file,line); return sAllocMem_(size,align,flags); }
inline void sCheckMem(const char *file,sInt line) { sTagMem(file,line); sCheckMem_(); }
#define sAllocMem(size,align,flags)   sAllocMem_(size,align,flags,__FILE__,__LINE__)
#define sCheckMem()                   { sTagMem(__FILE__,__LINE__); sCheckMem_(); }
#else
#define sAllocMem(size,align,flags)   sAllocMem_(size,align,flags) 
#define sCheckMem()                   sCheckMem_()
#endif


#define sALLOCFRAMEALIGN 16

// own new/delete using altona memory manager

#if sPLATFORM!=sPLAT_IOS
#if !sCONFIG_OPTION_XSI
#if sCONFIG_DEBUGMEM
inline void *sCDECL operator new  (sCONFIG_SIZET size)throw() { return sAllocMem_(size,16,0); }
inline void *sCDECL operator new[](sCONFIG_SIZET size)throw() { return sAllocMem_(size,16,0); }
inline void *sCDECL operator new  (sCONFIG_SIZET size,sInt flags)throw() { return sAllocMem_(size,16,flags); }
inline void *sCDECL operator new[](sCONFIG_SIZET size,sInt flags)throw() { return sAllocMem_(size,16,flags); }
inline void sCDECL operator delete  (void* p)throw() { sFreeMem(p); }
inline void sCDECL operator delete[](void* p)throw() { sFreeMem(p); }
inline void * sCDECL operator new  (sCONFIG_SIZET size,const char *file,int line)throw() { sTagMem(file,line); return sAllocMem_(size,16,0); }
inline void * sCDECL operator new[](sCONFIG_SIZET size,const char *file,int line)throw() { sTagMem(file,line); return sAllocMem_(size,16,0); }
inline void * sCDECL operator new  (sCONFIG_SIZET size,sInt flags,const char *file,int line)throw() { sTagMem(file,line); return sAllocMem_(size,16,flags); }
inline void * sCDECL operator new[](sCONFIG_SIZET size,sInt flags,const char *file,int line)throw() { sTagMem(file,line); return sAllocMem_(size,16,flags); }
inline void sCDECL operator delete  (void *p, const char *, int)throw() { sFreeMem(p); }
inline void sCDECL operator delete[](void *p, const char *, int)throw() { sFreeMem(p); }
#else
inline void *sCDECL operator new  (sCONFIG_SIZET size)throw() { return sAllocMem_(size,16,0); }
inline void *sCDECL operator new[](sCONFIG_SIZET size)throw() { return sAllocMem_(size,16,0); }
inline void *sCDECL operator new  (sCONFIG_SIZET size,sInt flags)throw() { return sAllocMem_(size,16,flags); }
inline void *sCDECL operator new[](sCONFIG_SIZET size,sInt flags)throw() { return sAllocMem_(size,16,flags); }
inline void sCDECL operator delete  (void* p) { sFreeMem(p); }
inline void sCDECL operator delete[](void* p) { sFreeMem(p); }
#endif // sCONFIG_DEBUGMEM
#endif // !sCONFIG_OPTION_XSI

inline void *operator new(sCONFIG_SIZET, void *ptr)throw() { return ptr; }
inline void *operator new[](sCONFIG_SIZET, void *ptr)throw() { return ptr; }

#undef sNEW_ALLOCATOR

#if sCONFIG_DEBUGMEM && !sCONFIG_OPTION_XSI
#define sDEFINE_NEW new(__FILE__,__LINE__)
#else
#define sDEFINE_NEW new
#endif

#endif  // !IOS 

#if sPLATFORM==sPLAT_IOS
inline void *operator new(sCONFIG_SIZET, void *ptr)throw() { return ptr; }
inline void *operator new[](sCONFIG_SIZET, void *ptr)throw() { return ptr; }
#define sDEFINE_NEW new
#endif // IOS

template<class T> 
T* sPlacementNew(void *ptr){ return new (ptr) T; }
template<class T, typename ARG0> 
T* sPlacementNew(void *ptr, ARG0 arg0){ return new (ptr) T(arg0); }
template<class T, typename ARG0, typename ARG1> 
T* sPlacementNew(void *ptr, ARG0 arg0, ARG1 arg1){ return new (ptr) T(arg0, arg1); }
template<class T, typename ARG0, typename ARG1, typename ARG2> 
T* sPlacementNew(void *ptr, ARG0 arg0, ARG1 arg1, ARG2 arg2){ return new (ptr) T(arg0, arg1, arg2); }

#define new sDEFINE_NEW

/****************************************************************************/
/***                                                                      ***/
/***   Automatic Memory pools: AllocFrame, AllocDma                        ***/
/***                                                                      ***/
/****************************************************************************/

void sPartitionMemory(sPtr frame=0,sPtr dma=0,sPtr gfx=0);    // allocate buffers for sAllocFrame and sAllocDma, and partition gpu/cpu memory for unified architectures
void sFrameMemDoubleBuffer(sBool enable);                     // change frame mem double buffering (useful for multithreading), results in sFlipMem
void *sAllocFrame(sPtr size,sInt align=16);   // cpu accessable memory, not double buffered
void *sAllocFrameBegin(sInt max,sInt align);
void sAllocFrameEnd(void *);
sBool sIsFrameMem(void *);
sBool sIsDmaMem(void *);
void *sAllocDma(sPtr size,sInt align=16);     // gpu accessable memory (cpu is write only), double buffered
void *sAllocDmaBegin(sInt size, sInt align=16);
void sAllocDmaEnd(void *);
void sFlipMem();                    // perform double buffering of the dma buffers

sU32 sAllocDmaEstimateAvailable (void); // Return an estimate of the available DMA memory.
sU32 sDMAMemSize(void); 

template <typename T> sINLINE T* sAllocFrameNew(sInt count=1, sInt align=sALLOCFRAMEALIGN)
{
  void *ptr = sAllocFrame(sizeof(T)*count,align);
  T* tmp = (T*)ptr;
  for(sInt i=0;i<count;i++)
  { sPlacementNew<T>((T*)&tmp[i]);
  }
  return tmp;
}
template <typename T> sINLINE void sAllocFrameNew(T *&ptr, sInt count=1, sInt align=sALLOCFRAMEALIGN) 
{ ptr = sAllocFrameNew<T>(count,align); }

// only raw memory, no constructor called

template <typename T> sINLINE T* sAllocFrameRaw(sInt count=1, sInt align=sALLOCFRAMEALIGN)
{ void *ptr = sAllocFrame(sizeof(T)*count,align); return (T*)ptr; }

template <typename T> sINLINE void sAllocFrameRaw(T *&ptr, sInt count=1, sInt align=sALLOCFRAMEALIGN)
{ ptr = sAllocFrameRaw<T>(count,align); }

template <typename T> sINLINE void sAllocFrameBegin(T *&ptr, sInt max, sInt align=sALLOCFRAMEALIGN)
{ ptr = (T*)sAllocFrameBegin(sizeof(T)*max,align); }

/****************************************************************************/
/***                                                                      ***/
/***   Small Structures                                                   ***/
/***                                                                      ***/
/****************************************************************************/

template <class Type> struct sBaseRect
{
  Type x0,y0,x1,y1;
  sBaseRect() { x0=y0=x1=y1=0; }
  sBaseRect(Type XS,Type YS) { x0=0;y0=0;x1=XS;y1=YS; }
  sBaseRect(Type X0,Type Y0,Type X1,Type Y1) { x0=X0;y0=Y0;x1=X1;y1=Y1; }
  sBaseRect(const sBaseRect &r) { *this=r; }
  template <typename T2> explicit sBaseRect(const sBaseRect<T2> &r) { x0 = Type(r.x0); y0 = Type(r.y0); x1 = Type(r.x1); y1 = Type(r.y1); }

  void Init() { x0=y0=x1=y1=0; }
  void Init(Type XS,Type YS) { x0=0;y0=0;x1=XS;y1=YS; }
  void Init(Type X0,Type Y0,Type X1,Type Y1) { x0=X0;y0=Y0;x1=X1;y1=Y1; }

  Type SizeX() const { return x1-x0; }
  Type SizeY() const { return y1-y0; }
  Type CenterX() const { return (x0+x1)/2; }
  Type CenterY() const { return (y0+y1)/2; }
  Type Area() const    { return SizeX()*SizeY(); }
  sBool IsInside(const sBaseRect &r) const { return (r.x0<x1 && r.x1>x0 && r.y0<y1 && r.y1>y0); }   // this is inside r
  sBool IsEmpty() const { return x0==x1 || y0==y1; }
  sBool IsValid() const { return x0<x1 && y0<y1; }

  void Extend(Type e) { x0-=e;y0-=e;x1+=e;y1+=e; }
  void Extend(const sBaseRect &r) { x0-=r.x0;y0-=r.y0;x1+=r.x1;y1+=r.y1; }
  void Add(const sBaseRect &a,const sBaseRect &b) { if(b.IsEmpty()) { *this=a; } else if(a.IsEmpty()) { *this=b; } else { x0=sMin(a.x0,b.x0); y0=sMin(a.y0,b.y0); x1=sMax(a.x1,b.x1); y1=sMax(a.y1,b.y1); } } 
  void Add(const sBaseRect &a)                    { if(  IsEmpty()) { *this=a; } else if(!a.IsEmpty())                  { x0=sMin(a.x0,  x0); y0=sMin(a.y0,  y0); x1=sMax(a.x1,  x1); y1=sMax(a.y1,  y1); } } 
  void And(const sBaseRect &a,const sBaseRect &b) { x0=sMax(a.x0,b.x0); y0=sMax(a.y0,b.y0); x1=sMin(a.x1,b.x1); y1=sMin(a.y1,b.y1); if(x0>=x1||y0>=y1) Init(); } 
  void And(const sBaseRect &a)                    { x0=sMax(a.x0,  x0); y0=sMax(a.y0,  y0); x1=sMin(a.x1,  x1); y1=sMin(a.y1,  y1); if(x0>=x1||y0>=y1) Init(); } 
  void Move(Type x, Type y)                       { x0+=x; x1+=x; y0+=y; y1+=y; }
  void Scale(Type s)                              { x0*=s; x1*=s; y0*=s; y1*=s; }
  void Scale(Type x, Type y)                      { x0*=x; x1*=x; y0*=y; y1*=y; }
  void ScaleCenter(sF32 x, sF32 y)                { Type sx = SizeX()/2; Type sy = SizeY()/2; Type cx = CenterX(); Type cy = CenterY(); x0 = Type(cx-sx*x); y0 = Type(cy-sy*y); x1 = Type(cx+sx*x); y1 = Type(cy+sy*y); }
  void ScaleCenter(sF32 s)                        { ScaleCenter(s,s); }
  void MoveCenter(Type cx, Type cy)               { Type sx = SizeX()/2; Type sy = SizeY()/2; x0=cx-sx;x1=cx+sx;y0=cy-sy;y1=cy+sy; }
  void MoveCenterX(Type cx)                       { Type sx = SizeX()/2; x0=cx-sx;x1=cx+sx; }
  void MoveCenterY(Type cy)                       { Type sy = SizeY()/2; y0=cy-sy;y1=cy+sy; }
  void SetSize(Type sx, Type sy)                  { x1 = x0 + sx; y1 = y0 + sy; }
  
  int operator !=(const sBaseRect<Type> &r) const { return x0!=r.x0 || x1!=r.x1 || y0!=r.y0 || y1!=r.y1; }
  int operator ==(const sBaseRect<Type> &r) const { return x0==r.x0 && x1==r.x1 && y0==r.y0 && y1==r.y1; }
  sBaseRect<Type> & operator = (const sBaseRect<Type> &r) { x0=r.x0; x1=r.x1; y0=r.y0; y1=r.y1; return *this; }
  sBool Hit(Type x,Type y) const { return x>=x0 && x<x1 && y>=y0 && y<y1; }
  void Sort() { if(x0>x1) sSwap(x0,x1); if(y0>y1) sSwap(y0,y1); }
  void Inset(Type o) { InsetX(o); InsetY(o); } 
  void Inset(const sBaseRect &r) { x0+=r.x0;y0+=r.y0;x1-=r.x1;y1-=r.y1; }
  void InsetX(Type o) { Type mx=sMin(o,SizeX()/2); x0+=mx; x1-=mx; } 
  void InsetY(Type o) { Type my=sMin(o,SizeY()/2); y0+=my; y1-=my; }

  void SwapX() { sSwap(x0, x1); }
  void SwapY() { sSwap(y0, y1); }
  void Swap() { SwapX(); SwapY(); }
  void Fix() { if (x1 < x0) SwapX(); if (y1 < y0) SwapY(); }

  void Blend(const sBaseRect &l, const sBaseRect &r, sF32 n) { sF32 m=1-n; x0 = l.x0 * m + r.x0 * n; x1 = l.x1 * m + r.x1 * n; y0 = l.y0 * m + r.y0 * n; y1 = l.y1 * m + r.y1 * n; }
  void Clip(const sBaseRect &c)   { x0 = sClamp(x0,c.x0,c.x1); y0 = sClamp(y0,c.y0,c.y1); x1 = sClamp(x1,c.x0,c.x1); y1 = sClamp(y1,c.y0,c.y1); }
  void Div(Type x, Type y) { x0 /= x; y0 /= y; x1 /= x; y1 /= y; }
};

typedef sBaseRect<sInt> sRect;
typedef sBaseRect<sF32> sFRect;

sFRect sRectToFRect(const sRect &src);
sRect sFRectToRect(const sFRect &src);

sFormatStringBuffer& operator% (sFormatStringBuffer &f, const sRect &r);
sFormatStringBuffer& operator% (sFormatStringBuffer &f, const sFRect &r);

/****************************************************************************/

// Linear congruential generator (mul add)

class sRandom                               
{
private:
  sU32 kern;
  sU32 Step();
public:
  sRandom() { Seed(0); }
  sRandom(sU32 seed) { Seed(seed); }
  void Init() { Seed(0); }
  void Seed(sU32 seed);
  sInt Int(sInt max);
  sInt Int16() { return Step()&0xffff; }
  sU32 Int32();
  sF32 Float(sF32 max);
  inline sF32 FloatSigned(sF32 max) { return Float(2.0f*max)-max; }
};

//  Mersenne twister (pretty good and fast)

class sRandomMT                             
{
private:
  enum Enums
  {
    Count = 624,
    Period = 397,
  };
  sU32 State[Count];
  sInt Index;

  sU32 Step();
  void Reload();
public:
  sRandomMT() { Seed(0); }
  sRandomMT(sU32 seed) { Seed(seed); }
  void Init() { Seed(0); }
  void Seed(sU32 seed);
  sInt Int(sInt max);
  sInt Int16() { return Step()&0xffff; }
  sU32 Int32() { return Step(); }
  sF32 Float(sF32 max);
  inline sF32 FloatSigned(sF32 max) { return Float(2.0f*max)-max; }
};


//  KISS (smaller period than MT, but still long enough for games)

class sRandomKISS                           
{
private:
  sU32 x,y,z,c;
  sU32 Step();
public:
  sRandomKISS() { Seed(0); }
  sRandomKISS(sU32 seed) { Seed(seed); }
  void Init() { Seed(0); }
  void Seed(sU32 seed);
  sInt Int(sInt max);
  sInt Int16() { return Step()&0xffff; }
  sU32 Int32() { return Step(); }
  sF32 Float(sF32 max);
  inline sF32 FloatSigned(sF32 max) { return Float(2.0f*max)-max; }
};

/****************************************************************************/

class sTiming
{
  sInt TotalTime;
  sInt LastTime;
  sInt FirstFrame;
  sInt Slices;
  sInt TimeSlice;
  sInt Delta;
  sInt Jitter;
  sInt Timestamp;

  sInt History[64];                         // buffer for building average time.
  sInt HistoryIndex;                        // round-robin pointer to next sample
  sInt HistoryCount;                        // how many samples have been taken?
  sInt HistorySkip;                         // don't measure the first few frames
  sInt HistoryMax;                          // how many fields of the history are used?
public:
  sTiming();                                // initialize structures
  void SetTimeSlice(sInt ts=10);            // set duration of timeslice
  sInt GetTime() const { return TotalTime; }     // get time of frame, in ms
  sInt GetDelta() const { return Delta; }        // get time delta from last frame
  sInt GetSlices() const { return Slices; }      // get number of slices for this frame
  sInt GetJitter() const { return Jitter; }      // get remaining milliseconds in next slice
  sInt GetTimeSlice() const { return TimeSlice; }
  sF32 GetAverageDelta() const;                   // averaged milliseconds of last 64 frames
  void SetHistoryMax(sInt m) { HistoryMax = sMin(m,(sInt)sCOUNTOF(History)); HistoryCount=0; HistoryIndex=0; }
  

  void OnFrame(sInt time);                  // call this every frame
  
  void StartMeasure(sInt time);             // alternative timing interface
  void StopMeasure(sInt time);

  // timestamp methods
  void SetTimestamp(sInt time) { Timestamp = time; } 
  sInt GetTimestamp()          { return Timestamp; } 
  sInt GetTimestampDelta(sInt time) { return time-Timestamp; }
};

/****************************************************************************/

sU64 sHashStringFNV(const sChar *text);

sU32 sChecksumAdler32(const sU8 *data,sInt size);
sU32 sChecksumCRC32(const sU8 *data,sInt size);
sU32 sChecksumRandomByte(const sU8 *data,sInt size);
sU32 sChecksumMurMur(const sU32 *data,sInt words);

void sChecksumAdler32Begin();
void sChecksumAdler32Add(const sU8 *data,sInt size);
template <typename T> sINLINE void sChecksumAdler32Add(const T &data) { sChecksumAdler32Add((const sU8*)&data,sizeof(T)); }
sU32 sChecksumAdler32End();


struct sChecksumMD5
{
private:
  inline void f(sU32 &a,sU32 b,sU32 c,sU32 d,sInt k,sInt s,sU32 t,sU32 *data) { a += (d^(b&(c^d))) + data[k] + t;  a = ((a<<s)|(a>>(32-s))) + b; }
  inline void g(sU32 &a,sU32 b,sU32 c,sU32 d,sInt k,sInt s,sU32 t,sU32 *data) { a += (c^(d&(b^c))) + data[k] + t;  a = ((a<<s)|(a>>(32-s))) + b; }
  inline void h(sU32 &a,sU32 b,sU32 c,sU32 d,sInt k,sInt s,sU32 t,sU32 *data) { a += (b^c^d)       + data[k] + t;  a = ((a<<s)|(a>>(32-s))) + b; }
  inline void i(sU32 &a,sU32 b,sU32 c,sU32 d,sInt k,sInt s,sU32 t,sU32 *data) { a += (c^(b|~d))    + data[k] + t;  a = ((a<<s)|(a>>(32-s))) + b; }
  void Block(const sU8 *data);
public:
  sU32 Hash[4];

  void CalcBegin();
  sInt CalcAdd(const sU8 *data, sInt size);   // calculate 64 byte blocks, returns consumed bytes
  void CalcEnd(const sU8 *data, sInt size, sInt sizeall);

  void Calc(const sU8 *data,sInt size);
  sBool Check(const sU8 *data,sInt size);
  sBool operator== (const sChecksumMD5 &)const;
  sBool operator!= (const sChecksumMD5 &md5)const    { return !(*this==md5); }
};

sFormatStringBuffer& operator% (sFormatStringBuffer &f, const sChecksumMD5 &md5);
/****************************************************************************/

sU32 sScaleColor(sU32 a,sInt scale);          // a*b, scale = 0..0x10000...
sU32 sScaleColorFast(sU32 a,sInt scale);    // a*scale, scale = 0..0x100
sU32 sMulColor(sU32 a,sU32 b);              // a*b
sU32 sAddColor(sU32 a,sU32 b);              // a+b
sU32 sSubColor(sU32 a,sU32 b);              // a-b
sU32 sMadColor(sU32 mul1,sU32 mul2,sU32 add); // mul1*mul2+add
sU32 sFadeColor(sInt fade,sU32 a,sU32 b);   // fade=0..0x10000

sU32 sColorFade (sU32 a, sU32 b, sF32 f);   // fade=0.0f..1.0f
sU32 sAlphaFade (sU32 c, sF32 f);           // fade=0.0f..1.0f
sU32 sPMAlphaFade (sU32 c, sF32 f);         // fade=0.0f..1.0f
sF32 sScaleFade(sF32 a, sF32 b, sF32 f);    // fade=0.0f..1.0f

// a-b, angles=0..c, -count/2 > result > count/2
sInt sAngleDiff(sInt alpha, sInt beta, sInt count); 

// a-b, angles=0..1, -0.5 > result > 0.5
sF32 sAngleDiff(sF32 alpha, sF32 beta); 

// a-b, angles=2*pi, -pi > result > pi
sF32 sRadianDiff(sF32 alpha, sF32 beta); 

// a-b, angles=180, -180 > result > 180
sF32 sDegreeDiff(sF32 alpha, sF32 beta); 

/****************************************************************************/
/***                                                                      ***/
/***   Application Interface                                              ***/
/***                                                                      ***/
/****************************************************************************/
extern sInt sGetTime();
struct sInput2Event {
  sInput2Event(sU32 key = 0, sInt instance = 0, sDInt payload = 0, sInt timestamp = sGetTime()) { Key = key; Instance = instance; Payload = payload; Timestamp = timestamp; }
  sU32 Key;          // one of sKEY_*. defines the semantic of Payload
  sInt Instance;     // zero-based device Instance
  sDInt Payload;     // some value. semantic is defined bei Key
  sInt Timestamp;    // time when event was generated
};

class sApp
{
public:
  virtual ~sApp();

  virtual void OnInit();                   // to init stuff which cannot be done by constructor
  
  virtual void OnInput(const sInput2Event &ie);
  virtual void OnPaint2D(const sRect &client,const sRect &update);
  virtual void OnPrepareFrame();           // is called just before paint3d but not between renderbegin and renderend
  virtual void OnPaint3D();
  virtual sBool OnPaint();                // return TRUE to do all render begin/end, prepare/paint in OnPaint and OnPrepareFrame/OnPaint3D are not called

  virtual void OnEvent(sInt id); 
//  virtual void OnMouse(sInt buttons,sInt x,sInt y);   // obsolete
//  virtual void OnKey(sU32 key);     // obsolete

  virtual void GetCaption(const sStringDesc &sd) { sCopyString(sd,L"sApp"); }
};

void sMain();
void sSetApp(sApp *);
sApp *sGetApp();

// events not tested!

enum sAppEvents
{
  sAE_TIMER = 1,
  sAE_TRIGGER,
  sAE_EXIT,

  sAE_USER = 0x1000,
};

void sSetTimerEvent(sInt time,sBool loop = 1);
void sTriggerEvent(sInt event=sAE_TRIGGER);

// command line parsing
void sParseCmdLine(const sChar *cmd,sBool skipcmd=1);
sBool sAddShellParameter(const sChar *opt, const sChar *val);

// omit the '-', like sGetShellSwitch(L"x");
const sChar *sGetShellParameter(const sChar *opt,sInt n);
sInt sGetShellParameterInt(const sChar *opt,sInt n,sInt def);
sF32 sGetShellParameterFloat(const sChar *opt,sInt n,sF32 def);
sU32 sGetShellParameterHex(const sChar *opt,sInt n,sU32 def);
sBool sGetShellSwitch(const sChar *opt);

// for short and long option, like sGetShellSwitch(L"n",L"-name","dierk2);
const sChar *sGetShellString(const sChar *optshort,const sChar *optlong,const sChar *def=0);
sInt sGetShellInt(const sChar *optshort,const sChar *optlong,sInt def=0);

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   GUIDs                                                              ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

struct sGUID
{
  sU32 Data32;
  sU16 Data16[3];
  sU8  Data8[6];

  bool operator == (const sGUID &g) const { return !sCmpMem(this,&g,sizeof(sGUID)); }
  bool operator != (const sGUID &g) const { return sCmpMem(this,&g,sizeof(sGUID))!=0; }
  bool operator <  (const sGUID &g) const { return sCmpMem(this,&g,sizeof(sGUID))<0; }

};

// get project GUID. Take extreme caution if you're using this from within library code!
static sINLINE sGUID sGetProjectGUID() { sGUID g=sCONFIG_GUID; return g; }
sGUID sGetGUIDFromString(const sChar *str);

sFormatStringBuffer& operator% (sFormatStringBuffer &f, const sGUID &guid);

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Containers with explicitly static memory management                ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/


/****************************************************************************/
/***                                                                      ***/
/***   sStaticArray                                                       ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   Base of all Arrays. You state the size ONCE and never change it.   ***/
/***   No Dynamic allocation occours.                                     ***/
/***                                                                      ***/
/***   To set the Size, use HintSize() or AddMany(). HintSize() will      ***/
/***   allocate memory, but the Array is still empty, you will need to    ***/
/***   use AddXXX() to put something into. AddMany() will add multiple    ***/
/***   items, and if this is your first call you can allocate and fill    ***/
/***   the Array at the same time                                         ***/
/***                                                                      ***/
/***   sArray in types2.hpp is the same, just with dynamic reallocation.  ***/
/***                                                                      ***/
/****************************************************************************/

// used to get the base type of a pointer type,
// which can then be used like this:
//
// sStaticArray<T>::ElementType *element;
// sFORALL(Array, element) {}

template<typename T>
struct sRemovePtrType { typedef T Type; };
template<typename T>
struct sRemovePtrType<T*> { typedef T Type; };
template<typename T>
struct sRemovePtrType<T**> { typedef T Type; };

#define sALLOW_MEMDEBUG_STACKTRACE 0 && (sDEBUG && !sRELEASE && sPLATFORM==sPLAT_WINDOWS)

#if sALLOW_MEMDEBUG_STACKTRACE

sBool sGetMemTagged();
void sGetCaller(const sStringDesc &filename, sInt &line);

#define sTAG_CALLER() \
{ \
  sMemoryLeakTracker *mlt = sGetMemoryLeakTracker(); \
  if (mlt && !sGetMemTagged()) \
  { \
    sString<255> traceFile=L"*"; \
    sInt         traceLine; \
    sGetCaller(sGetAppendDesc(traceFile), traceLine); \
    sTagMem(mlt->GetPersistentString8(traceFile), traceLine); \
  } \
} \

#else

#define sTAG_CALLER()

#endif

#define TRACK_ARRAY_USAGE sDEBUG

/************************************************************************/ /*! 

\ingroup altona_base_types_arrays

Array that will not grow automatically. 
- call HintSize() to set initial size before using
- You can also use the constructor to set initial size
- You may resize an sStaticArray manually after calling Reset().

*/ /*************************************************************************/

template <class Type> class sStaticArray
{
protected:

  Type *Data;
  sInt Used;
  sInt Alloc;

// prepare tracking of array-usage
#if TRACK_ARRAY_USAGE
  sInt UsedMaximum;
  void UsageTrackingInit()   { UsedMaximum = Used; }
  void UsageTrackingUpdate() { if (UsedMaximum<Used) UsedMaximum = Used; }
public:
  sInt GetMaximumUsed() const { return UsedMaximum; }
#else
  void UsageTrackingInit()   {}
  void UsageTrackingUpdate() {}
public:
  sInt GetMaximumUsed() const { return -1; }
#endif
protected:

  virtual void ReAlloc(sInt max)          { 
                                            if(max>Alloc) 
                                            { 
#if sDEBUG
                                              if (Data)
                                                sDPrintF(L"ReAlloc: Alloc=%d, max=%d, sizeof(Type)=%d\n", Alloc, max, sInt(sizeof(Type)));
#endif
                                              sVERIFY(!Data);
                                              sTAG_CALLER();
                                              Data = new Type[max]; 
                                              Alloc= Data?max:0;
                                            } 
                                          }

  virtual void Grow(sInt add)             { 
#if sDEBUG
                                            if(Used+add>Alloc)
                                              sFatal(L"Overflow! Alloc: %d Used: %d Added: %d\n",Alloc, Used, add);
#endif
                                            sVERIFYRELEASE(Used+add<=Alloc);
                                          } 

public:
  typedef Type BaseType;
  typedef typename sRemovePtrType<Type>::Type ElementType;
  typedef sInt Iterator;

  //! Construct and allocate some space. The array will be considered empty.
  explicit sStaticArray(sInt max)       { Data = 0; Used = 0; Alloc = 0; UsageTrackingInit(); sTagMemLast(); ReAlloc(max); }
  //! Construct with no space allocated
  sStaticArray()                        { Data = 0; Used = 0; Alloc = 0; UsageTrackingInit(); }
  //! Construct and initialize with preallocated space. The array will be considered empty.
  sStaticArray(Type *data, sInt max)    { Data = data; Used = 0; Alloc = max; UsageTrackingInit(); }
  //! Deallocate memory.
  virtual ~sStaticArray()               { delete[] Data; 
                                          //// generate a message if memory is wasted
                                          //{
                                          //  sInt mu = GetMaximumUsed();
                                          //  if (mu>0)
                                          //  {
                                          //    sInt wastedBytes = (GetSize()-mu) * sizeof(BaseType);
                                          //    if (wastedBytes>100*1024)
                                          //      sLogF(L"sys", L"sStaticArray destroyed. %d bytes of mem never used.\n", wastedBytes);
                                          //  }
                                          //}
                                        }
  //! Create a copy of all elements.
  void Copy(const sStaticArray &a)      { sTAG_CALLER(); Resize(a.GetCount()); for(sInt i=0;i<a.GetCount();i++) Data[i] = a.Data[i]; }
  //! Create a copy of all elements.
  sStaticArray(const sStaticArray &a)   { sTAG_CALLER(); Data = 0; Used = 0; Alloc = 0; UsageTrackingInit(); Copy(a); }
  //! Create a copy of all elements.
  sStaticArray &operator=(const sStaticArray &a)  { sTAG_CALLER(); if (this != &a) Copy(a); return *this; }
  //! reset deallocates memory and resets container,
  //! - use Clear instead if you want to reuse your container afterwards
  //! - this is the only way to Resize an sStaticArray
  virtual void Reset()                  { sDeleteArray(Data); Used = 0; Alloc = 0;}

  //! return number of elements used
  sInt GetCount() const                 { return Used; }
  //! return number of elements allocated (the potential size of the array)
  sInt GetSize() const                  { return Alloc; }
  //! get pointer to the actual array. Be aware that some classes derived from sStaticArray may change this pointer as the array grows
  Type *GetData() const                 { return Data; }
  //! return true if the used counter is zero
  sBool IsEmpty() const                 { return Used==0; }
  //! return true if potential size is used up. Some classes derived from sStaticArray can grow automatically.
  sBool IsFull() const                  { return Used==Alloc; }
  //! return true if the index provided will index a element.
  sBool IsIndexValid(sInt p) const      { return ((unsigned) p) < ((unsigned)Used); }
  //! Hints the array to allow grow to at least the specified size.
  //! - sStaticArray can only be initialized once and will not grow automatically
  void HintSize(sInt max)               { ReAlloc(max); }
  void HintSize_(sInt max,const char *file,sInt line)  { sTagMem(file,line); ReAlloc(max); }

  //! Set the array count. Grow array if required.
  Type *Resize(sInt count)              { if (count>Alloc) ReAlloc(count); Used = count; UsageTrackingUpdate(); return Data; }
  //! Set the array count to 0 without actually deallocating the memory.
  void Clear()                          { Used = 0; }

//  void Add(const Type &e)               { sVERIFY(&e<Data || &e>=Data+Alloc); Grow(1); Data[Used++]=e; }
//  Type *Add()                           { Grow(1); return &Data[Used++]; }
  //! Add all elements from another array
  void Add(const sStaticArray<Type> &a) { Grow(a.Used); for(sInt i=0;i<a.Used;i++) Data[Used++]=a.Data[i]; UsageTrackingUpdate(); }
  //! Advance the Used counter, increasing the array count. The returned pointer points to the first of the new elements
  Type *AddMany(sInt count)             { Grow(count); Type *r=Data+Used; Used+=count; UsageTrackingUpdate(); return r; }
  Type *AddManyInit(sInt count, const Type *prototype = sNULL)  { Type *r=AddMany(count); 
                                                                      if (prototype) for (sInt i=0;i<count;i++) r[i] = *prototype;
                                                                      else           for (sInt i=0;i<count;i++) r[i] = Type();
                                                                      return r; }  
  Type *AddFull()                       { return AddMany(GetSize()-GetCount()); }
  Type *AddFullInit(const Type *prototype = sNULL) { return AddManyInit(GetSize()-GetCount(), prototype); }
  //! Insert element into array, preserving order (requires copying elements around)
  void AddBefore(const Type &e,sInt p)  { sVERIFY(&e<Data || &e>=Data+Alloc); sVERIFY(p>=0 && p<=Used); Grow(1); for(sInt i=Used;i>p;i--) Data[i]=Data[i-1]; Data[p]=e; Used++; UsageTrackingUpdate(); }
  //! Insert element into array, preserving order (requires copying elements around)
  void AddAfter(const Type &e,sInt p)   { sVERIFY(&e<Data || &e>=Data+Alloc); AddBefore(e,p+1); UsageTrackingUpdate(); }
  //! Insert element into array, preserving order (requires copying elements around)
  void AddHead(const Type &e)           { sVERIFY(&e<Data || &e>=Data+Alloc); AddBefore(e,0); UsageTrackingUpdate(); }
  //! Insert element into array, preserving order (which is trivial)
  void AddTail(const Type &e)           { sVERIFY(&e<Data || &e>=Data+Alloc); Grow(1); Data[Used++]=e; UsageTrackingUpdate(); }

  Type &GetTail() const                 { sVERIFY(!IsEmpty()); return Data[Used-1]; }

  //! Removing element, NOT preserving order
  void RemAt(sInt p)                    { sVERIFY(IsIndexValid(p)); Data[p] = Data[--Used]; }
  //! Removing element, preserving order (requires copying elements around)
  void RemAtOrder(sInt p)               { sVERIFY(IsIndexValid(p)); Used--; while(p<Used) {Data[p]=Data[p+1]; p++;} }
  //! Removing last element, preserving order (which is trivial)
  Type RemTail()                        { sVERIFY(Used>0); Used--; return Data[Used]; }
  //! Removing element, NOT preserving order (requires a linear search for the element)
  sBool Rem(const Type e)               { sBool result = sFALSE; for(sInt i=0;i<Used;) if(Data[i]==e) { result = sTRUE; RemAt(i); } else i++; return result; }
  //! Removing element, preserving order (requires a linear search for the element) (requires copying elements around)
  sBool RemOrder(const Type e)          { sBool result = sFALSE; for(sInt i=0;i<Used;) if(Data[i]==e) { result = sTRUE; RemAtOrder(i); } else i++; return result; }

  //! Indexing
  sINLINE const Type &operator[](sInt p) const  { sVERIFY(IsIndexValid(p)); return Data[p]; }
  //! Indexing
  sINLINE Type &operator[](sInt p)              { sVERIFY(IsIndexValid(p)); return Data[p]; }

  //! Swap the whole array with another one. This is quick.
  void Swap(sStaticArray &a)            { sSwap(Data,a.Data); sSwap(Used,a.Used); sSwap(Alloc,a.Alloc); }
  //! Swap two elements of this array
  void Swap(sInt i,sInt j)              { sSwap(Data[i],Data[j]); }

  //! Fisher Yates Shuffle ; works in-place, time complexity: O(n)
  template <class RandomClass> void Shuffle(RandomClass &rnd_gen) { if (Used==0) return; for (sInt i=Used-1; i>0; i--) Swap(rnd_gen.Int(i+1), i); }

  // Don't use IndexOf, use sFindIndex instead!
  sOBSOLETE sInt IndexOf(const Type &e)            { for(sInt i = 0; i < GetCount(); i++) if (Data[i] == e) return i; return -1; }
};

// remove unused macros
#undef TRACK_ARRAY_USAGE

/****************************************************************************/

//! \ingroup altona_base_types_arrays
//! Array class with storage on stack
//!
//! This is a modified version of the sStaticArray that gets it's 
//! data allocation automatically by the compiler:
//! - When used as a local variable the data is allocated on the stack. (Therefore it's name.)
//! - When used as a global variable the data is allocated on the heap.
//! - When used as a member the data is allocated like the instance of the containing datastructure.
//!
//! The Alloc counter must be specified as template parameter, further 
//! capacity changes are not possible.

template <class Type,int Max_> class sStackArray : public sStaticArray<Type>
{
protected:

  void ReAlloc(sInt max)                { sVERIFY(max<=Max_); }
  void Grow(sInt add)                   { 
#if !sRELEASE
                                            if (sStaticArray<Type>::Used+add>Max_)
                                            {
                                              sString<128> s;
                                              s.PrintF(L"Overflow! Alloc: %d Used: %d Added: %d\n",sStaticArray<Type>::Alloc,sStaticArray<Type>::Used, add);
                                              sFatal(s);
                                            }
#endif
                                            sVERIFY(sStaticArray<Type>::Used+add<=Max_); 
                                        } 
  Type Storage[Max_];
public:                                 // das ganze rumgescope ist für den PS3-compiler...
  typedef typename sRemovePtrType<Type>::Type ElementType;
  enum { SIZE = Max_ };
  sStackArray()                         { sStaticArray<Type>::Data = Storage; sStaticArray<Type>::Used = 0; sStaticArray<Type>::Alloc = Max_; }
  explicit sStackArray(sInt initcount)  { sStaticArray<Type>::Data = Storage; sStaticArray<Type>::Used = 0; sStaticArray<Type>::Alloc = Max_; sStaticArray<Type>::AddMany(initcount); }
  sStackArray(const sStackArray<Type,Max_> &other) { sStaticArray<Type>::Data = Storage; sStaticArray<Type>::Used = 0; sStaticArray<Type>::Alloc = Max_; Copy(other); }
  ~sStackArray()                        { sStaticArray<Type>::Data = 0; }
  void Reset()                          { sStaticArray<Type>::Used = 0; sStaticArray<Type>::Alloc = Max_; }
};

/****************************************************************************/
/***                                                                      ***/
/***   sExternalArray                                                     ***/
/***                                                                      ***/
/***   for checked access to external arrays                              ***/
/***                                                                      ***/
/****************************************************************************/

/*

THIS DOESN'T WORK WITH CODEWARRIOR:
if you try to use this class with a const type, it fails because the compiler tries to
compile sStaticArray<const T>::ReAlloc() and fails because CWCC doesn't let you allocate
arrays of const types without an initializer (which makes perfect sense).

Only sensible (as in "not a total hack") solution: Create a sensible array base class
already (with an empty ReAlloc()) and derive sStaticArray from it. Of course this would 
take some major changes throughout the whole codebase but it's the only clean thing to do.

template <typename T> class sExternalArray : public sStaticArray<T>
{
protected:
  virtual void ReAlloc(sInt max) { sVERIFY(max<=sStaticArray<T>::Alloc); }
  virtual void Reset() { sStaticArray<T>::Clear(); }
public:
  template <sDInt SIZE> sExternalArray(T (&array)[SIZE]) { sStaticArray<T>::Data = array; sStaticArray<T>::Alloc = SIZE; sStaticArray<T>::Used = SIZE; }
  sExternalArray(T* data, sInt count) { sStaticArray<T>::Data = data; sStaticArray<T>::Alloc = count; sStaticArray<T>::Used = count; }
  virtual ~sExternalArray() { sStaticArray<T>::Data = 0; }
};

*/

/****************************************************************************/
/***                                                                      ***/
/***   FORALL macro                                                       ***/
/***                                                                      ***/
/****************************************************************************/

// adapter functions

template <template <typename> class ArrayType,class BaseType> 
sINLINE BaseType *sGetPtr(ArrayType<BaseType *> &a,sInt i)
{
  return a[i];
}

template <template <typename> class ArrayType,class BaseType> 
sINLINE BaseType *sGetPtr(ArrayType<BaseType> &a,sInt i)
{
  return &a[i];
}

template <template <typename> class ArrayType,class BaseType> 
sINLINE const BaseType *sGetPtr(const ArrayType<BaseType *> &a,sInt i)
{
  return a[i];
}

template <template <typename> class ArrayType,class BaseType> 
sINLINE const BaseType *sGetPtr(const ArrayType<BaseType> &a,sInt i)
{
  return &a[i];
}

// GCC needs these versions for sStackArray<class type,int max>

template <template <typename,int> class ArrayType,class BaseType,int count> 
sINLINE BaseType *sGetPtr(ArrayType<BaseType *,count> &a,sInt i)
{
  return a[i];
}

template <template <typename,int> class ArrayType,class BaseType,int count> 
sINLINE BaseType *sGetPtr(ArrayType<BaseType,count> &a,sInt i)
{
  return &a[i];
}

template <template <typename,int> class ArrayType,class BaseType,int count> 
sINLINE const BaseType *sGetPtr(const ArrayType<BaseType *,count> &a,sInt i)
{
  return a[i];
}

template <template <typename,int> class ArrayType,class BaseType,int count> 
sINLINE const BaseType *sGetPtr(const ArrayType<BaseType,count> &a,sInt i)
{
  return &a[i];
}


// the forall macro itself
/************************************************************************/ /*! 

\ingroup altona_base_types_arrays

This macro iterates sStaticArray and it's subclasses

@param l the array
@param e elements

- You will have to declare the iterator variable 'e' outside befor the macro
- 'e' should be a pointer to the element type. If the element type is already
  a pointer, the type of 'e' should be the element type.

*/ /*************************************************************************/

#define sFORALL(l,e) for(sInt _i=0;_i<(l).GetCount()?((e)=sGetPtr((l),_i)),1:0;_i++)

/************************************************************************/ /*! 

\ingroup altona_base_types_arrays

This macro iterates sStaticArray and it's subclasses in reverse order

@param l the array
@param e elements
 
see sFORALL for details

*/ /*************************************************************************/

#define sFORALLREVERSE(l,e) for(sInt _i=(l).GetCount()-1;_i>=0?((e)=sGetPtr((l),_i)),1:0;_i--)

/****************************************************************************/
/***                                                                      ***/
/***   Sorting                                                            ***/
/***                                                                      ***/
/****************************************************************************/

//! sort up, using '>' operator (ineffecient but compact)
//! \ingroup altona_base_types_arrays
template <class ArrayType> 
void sSortUp(ArrayType &a)
{
  sInt max = a.GetCount();
  for(sInt i=0;i<max-1;i++)
    for(sInt j=i+1;j<max;j++)
      if(a[i] > a[j])
        sSwap(a[i],a[j]);
}

//! sort down, using '<' operator (ineffecient but compact)
//! \ingroup altona_base_types_arrays
template <class ArrayType> 
void sSortDown(ArrayType &a)
{
  sInt max = a.GetCount();
  for(sInt i=0;i<max-1;i++)
    for(sInt j=i+1;j<max;j++)
      if(a[i] < a[j])
        sSwap(a[i],a[j]);
}

//! sort up using functor (ineffecient but compact)
//! \ingroup altona_base_types_arrays
template <class ArrayType, class CmpType> 
void sCmpSortUp(ArrayType &a, sInt (*cmp) (CmpType, CmpType))
{
  sInt max = a.GetCount();
  for(sInt i=0;i<max-1;i++)
    for(sInt j=i+1;j<max;j++)
      if(cmp(a[i], a[j]) > 0)
        sSwap(a[i],a[j]);
}

//! sort down using functor (ineffecient but compact)
//! \ingroup altona_base_types_arrays
template <class ArrayType, class CmpType> 
void sCmpSortDown(ArrayType &a, sInt (*cmp) (CmpType, CmpType))
{
  sInt max = a.GetCount();
  for(sInt i=0;i<max-1;i++)
    for(sInt j=i+1;j<max;j++)
      if(cmp(a[i], a[j]) < 0)
        sSwap(a[i],a[j]);
}

//! sort up using '>' operator on a member variable (ineffecient but compact)
//! \ingroup altona_base_types_arrays
template <class ArrayType,class BaseType,class MemberType> 
void sSortUp(ArrayType &a,MemberType BaseType::*o)
{
  sInt max = a.GetCount();
  for(sInt i=0;i<max-1;i++)
    for(sInt j=i+1;j<max;j++)
      if(sGetPtr(a,i)->*o > sGetPtr(a,j)->*o)
        sSwap(a[i],a[j]);
}

//! sort down using '<' operator on a member variable (ineffecient but compact)
//! \ingroup altona_base_types_arrays
template <class ArrayType,class BaseType,class MemberType> 
void sSortDown(ArrayType &a,MemberType BaseType::*o)
{
  sInt max = a.GetCount();
  for(sInt i=0;i<max-1;i++)
    for(sInt j=i+1;j<max;j++)
      if(sGetPtr(a,i)->*o < sGetPtr(a,j)->*o)
        sSwap(a[i],a[j]);
}


//! sort up using '<' operator (efficient)
//! \ingroup altona_base_types_arrays
template <class ArrayType> 
void sHeapSortUp(ArrayType &a)
{
  sInt count=a.GetCount();

  // pass 1: heapify (sift up)
  for (sInt i=1; i<count; i++)
  {
    sInt child=i;
    while (child>0)
    {
      sInt root=(child-1)/2;
      if (a[root]<a[child])
      {
        sSwap(a[root],a[child]);
        child=root;
      }
      else
        break;
    }
  }

  // pass 2: sort (sift down)
  while (--count>0)
  {
    sSwap(a[0],a[count]);
    sInt root=0;
    sInt child;
    while ((child=root*2+1)<count)
    {
      if (child<count-1 && a[child]<a[child+1])
        child++;
      if (a[root]<a[child])
      {
        sSwap(a[root],a[child]);
        root=child;
      }
      else
        break;
    }
  }
}


//! sort up using '<' operator on member variable (efficient)
//! \ingroup altona_base_types_arrays

template <class ArrayType,class BaseType,class MemberType> 
void sHeapSortUp(ArrayType &a,MemberType BaseType::*o)
{
  sInt count=a.GetCount();

  // pass 1: heapify (sift up)
  for (sInt i=1; i<count; i++)
  {
    sInt child=i;
    while (child>0)
    {
      sInt root=(child-1)/2;
      if (sGetPtr(a,root)->*o < sGetPtr(a,child)->*o)
      {
        sSwap(a[root],a[child]);
        child=root;
      }
      else
        break;
    }
  }

  // pass 2: sort (sift down)
  while (--count>0)
  {    
    sSwap(a[0],a[count]);
    sInt root=0;
    sInt child;
    while ((child=root*2+1)<count)
    {
      if (child<count-1 && sGetPtr(a,child)->*o < sGetPtr(a,child+1)->*o)
        child++;
      if (sGetPtr(a,root)->*o<sGetPtr(a,child)->*o)
      {
        sSwap(a[root],a[child]);
        root=child;
      }
      else
        break;
    }
  }
}

template <class ArrayType> 
void sGetSortPermutation(ArrayType &a, sStaticArray<sInt> &perm)
{
  sInt max = a.GetCount();
  perm.Resize(max);
  for (sInt i=0; i<max; i++) perm[i]=i;
  for(sInt i=0;i<max-1;i++)
    for(sInt j=i+1;j<max;j++)
      if(a[perm[j]] < a[perm[i]])
        sSwap(perm[i],perm[j]);
}


template <class ArrayType, class CmpType> 
void sGetSortPermutation(ArrayType &a, sInt (*cmp) (CmpType, CmpType), sStaticArray<sInt> &perm)
{
  sInt max = a.GetCount();
  perm.Resize(max);
  for (sInt i=0; i<max; i++) perm[i]=i;
  for(sInt i=0;i<max-1;i++)
    for(sInt j=i+1;j<max;j++)
      if(cmp(a[perm[i]], a[perm[j]]) > 0)
        sSwap(perm[i],perm[j]);
}

template <class ArrayType,class BaseType,class MemberType> 
void sGetSortPermutation(ArrayType &a,MemberType BaseType::*o, sStaticArray<sInt> &perm)
{
  sInt max = a.GetCount();
  perm.Resize(max);
  for (sInt i=0; i<max; i++) perm[i]=i;
  for(sInt i=0;i<max-1;i++)
    for(sInt j=i+1;j<max;j++)
      if(sGetPtr(a,perm[j])->*o < sGetPtr(a,perm[i])->*o)
        sSwap(perm[i],perm[j]);
}

// one should use some kind of function template or somethig...

//template <class Type> struct sCompareGreater { static sBool f(const Type a,const Type b) { return a>b; } };
//template <class Type> struct sCompareLesser  { static sBool f(const Type a,const Type b) { return a<b; } };

/****************************************************************************/
/***                                                                      ***/
/***   Remove and Delete by boolean test                                  ***/
/***                                                                      ***/
/****************************************************************************/

//! remove all elements with an attribute beeing true from list, without keeping order
//! \ingroup altona_base_types_arrays
template <class ArrayType,class BaseType,class MemberType> 
void sRemTrue(ArrayType &a,MemberType BaseType::*o)
{
  for(sInt i=0;i<a.GetCount();)
  {
    if(sGetPtr(a,i)->*o)
      a.RemAt(i);
    else
      i++;
  }
}

//! remove all elements with an attribute beeing false from list, without keeping order
//! \ingroup altona_base_types_arrays
template <class ArrayType,class BaseType,class MemberType> 
void sRemFalse(ArrayType &a,MemberType BaseType::*o)
{
  for(sInt i=0;i<a.GetCount();)
  {
    if(!(sGetPtr(a,i)->*o))
      a.RemAt(i);
    else
      i++;
  }
}

//! remove all elements with an attribute beeing true from list, maintaining order
//! \ingroup altona_base_types_arrays
template <class ArrayType,class BaseType,class MemberType> 
void sRemOrderTrue(ArrayType &a,MemberType BaseType::*o)
{
  sInt max = a.GetCount();
  sInt used = 0;
  for(sInt i=0;i<max;i++)
    if(!(sGetPtr(a,i)->*o))
      a[used++] = a[i];
  a.Resize(used);
}

//! remove all elements with an attribute beeing false from list, maintaining order
//! \ingroup altona_base_types_arrays
template <class ArrayType,class BaseType,class MemberType> 
void sRemOrderFalse(ArrayType &a,MemberType BaseType::*o)
{
  sInt max = a.GetCount();
  sInt used = 0;
  for(sInt i=0;i<max;i++)
    if(sGetPtr(a,i)->*o)
      a[used++] = a[i];
  a.Resize(used);
}

//! remove elements with an attribute equals a compare value from list, not keeping order
//! \ingroup altona_base_types_arrays
template <class ArrayType,class BaseType,class MemberType> 
void sRemEqual(ArrayType &a,MemberType BaseType::*o,const MemberType &e)
{
  for(sInt i=0;i<a.GetCount();)
  {
    if(sGetPtr(a,i)->*o == e)
      a.RemAt(i);
    else
      i++;
  }
}

//! remove all elements with an attribute beeing true from list, without keeping order, then deleting it
//! \ingroup altona_base_types_arrays
template <class ArrayType,class BaseType,class MemberType> 
void sDeleteTrue(ArrayType &a,MemberType BaseType::*o)
{
  for(sInt i=0;i<a.GetCount();)
  {
    if(sGetPtr(a,i)->*o)
    {
      delete sGetPtr(a,i);
      a.RemAt(i);
    }
    else
    {
      i++;
    }
  }
}

//! remove all elements with an attribute beeing false from list, without keeping order, then deleting it
//! \ingroup altona_base_types_arrays
template <class ArrayType,class BaseType,class MemberType> 
void sDeleteFalse(ArrayType &a,MemberType BaseType::*o)
{
  for(sInt i=0;i<a.GetCount();)
  {
    if(!(sGetPtr(a,i)->*o))
    {
      delete sGetPtr(a,i);
      a.RemAt(i);
    }
    else
    {
      i++;
    }
  }
}

//! remove and delete all elements from an array
//! \ingroup altona_base_types_arrays
template <class ArrayType> 
void sDeleteAll(ArrayType &a)
{
  while(!a.IsEmpty())
    delete a.RemTail();
}

//! for all elements in an array, remove it and call Release()
//! \ingroup altona_base_types_arrays
template <class ArrayType> 
void sReleaseAll(ArrayType &a)
{
  while(!a.IsEmpty())
    a.RemTail()->Release();
}

//! for all elements in an array, call AddRef()
//! \ingroup altona_base_types_arrays
template <template <typename> class ArrayType,class BaseType> 
void sAddRefAll(ArrayType<BaseType> &a)
{
  BaseType e;
  sFORALL(a,e)
    e->AddRef();
}


/****************************************************************************/
/***                                                                      ***/
/***   Finding                                                            ***/
/***                                                                      ***/
/****************************************************************************/

// const versions

//! Find first element by comparing a member.
template <class ArrayType,class BaseType,class MemberType,class CompareType> 
const BaseType *sFind(const ArrayType &a,MemberType BaseType::*o,const CompareType i)
{
  const BaseType *e;
  sFORALL(a,e)
    if(e->*o == i)
      return e;
  return 0;
}

//! Find first element by comparing a member and return the index.
template <class ArrayType,class BaseType,class MemberType,class CompareType> 
sInt sFindIndex(const ArrayType &a,MemberType BaseType::*o,const CompareType i)
{
  const BaseType *e;
  sFORALL(a,e)
    if(e->*o == i)
      return _i;
  return -1;
}

//! Continue searching for an element by comparing a member and return the next index.
template <class ArrayType,class BaseType,class MemberType,class CompareType> 
sInt sFindNextIndex(const ArrayType &a, sInt index,MemberType BaseType::*o,const CompareType i)
{
  const BaseType *e;
  sInt _i=index+1;
  for (; _i<a.GetCount()?(e=sGetPtr((a),_i)),1:0;_i++)
    if(e->*o == i)
      return _i;
  return -1;
}

//! Find first element by comparing the whole value.
template <class ArrayType,class BaseType> 
const BaseType *sFind(const ArrayType &a,const BaseType &v)
{
  const BaseType *e;
  sFORALL(a,e)
    if(*e == v)
      return e;
  return 0;
}

//! Check if a certain element is in an array by comparing the whole value
template <class ArrayType,class BaseType> 
sBool sFindPtr(const ArrayType &a,const BaseType *v)
{
  sInt max = a.GetCount();
  for (sInt i=0; i<max; i++)
    if(a[i] == v)
      return 1;
  return 0;
}

//! Find first element by comparing the whole value and return the index.
template <class ArrayType,class BaseType> 
sInt sFindIndex(const ArrayType &a,const BaseType &v)
{
  sInt max = a.GetCount();
  for (sInt i=0; i<max; i++)
    if(a[i] == v)
      return i;
  return -1;
}

//! Continue searching for an element by comparing the whole value and return the next index.
template <class ArrayType,class BaseType> 
sInt sFindNextIndex(const ArrayType &a, sInt index,const BaseType &v)
{  
  for (sInt i=index+1; i<a.GetCount(); i++)
    if(a[i] == v)
      return i;
  return -1;
}

template <class ArrayType,class BaseType> 
sBool sContains(const ArrayType &a,const BaseType &v)
{
  return sFindIndex(a,v)!=-1;
}



template <class ArrayType,class BaseType> 
sInt sFindIndexPtr(const ArrayType &a,const BaseType *v)
{
  // this can be more efficient implemented using pointerarithmetics
  sInt max = a.GetCount();
  for (sInt i=0; i<max; i++)
    if(&a[i] == v)
      return i;
  return -1;
}

//! Find first element by comparing a member and return the index.
template <class ArrayType,class BaseType,class MemberType> 
sInt sFindIndex(const ArrayType &a, MemberType BaseType::*o, const MemberType v)
{
  sInt max = a.GetCount();
  for(sInt i=0;i<max;i++)
      if(sGetPtr(a,i)->*o == v)
        return i;
  return -1;
}

//! Find first element with a certain member being \b true.
template <class ArrayType,class BaseType,class MemberType> 
const BaseType *sFindTrue(const ArrayType &a,MemberType BaseType::*o)
{
  const BaseType *e;
  sFORALL(a,e)
    if(e->*o)
      return e;
  return 0;
}

//! Find first element with a certain member being \b false.
template <class ArrayType,class BaseType,class MemberType> 
const BaseType *sFindFalse(const ArrayType &a,MemberType BaseType::*o)
{
  const BaseType *e;
  sFORALL(a,e)
    if(!e->*o)
      return e;
  return 0;
}

/****************************************************************************/

// non-const versions

template <class ArrayType,class BaseType,class MemberType,class CompareType> 
BaseType *sFind(ArrayType &a,MemberType BaseType::*o,const CompareType i)
{
  BaseType *e;
  sFORALL(a,e)
    if(e->*o == i)
      return e;
  return 0;
}

template <class ArrayType,class BaseType> 
BaseType *sFind(ArrayType &a,const BaseType &v)
{
  BaseType *e;
  sFORALL(a,e)
    if(*e == v)
      return e;
  return 0;
}

template <class ArrayType,class BaseType,class MemberType> 
BaseType *sFindTrue(ArrayType &a,MemberType BaseType::*o)
{
  BaseType *e;
  sFORALL(a,e)
    if(e->*o)
      return e;
  return 0;
}

template <class ArrayType,class BaseType,class MemberType> 
BaseType *sFindFalse(ArrayType &a,MemberType BaseType::*o)
{
  BaseType *e;
  sFORALL(a,e)
    if(!e->*o)
      return e;
  return 0;
}

//! Find first element with a certain member by using case insensitive string compares (sCmpStringI())
template <class ArrayType,class BaseType,class MemberType,class CompareType> 
BaseType *sFindStringI(ArrayType &a,MemberType BaseType::*o,const CompareType i)
{
  BaseType *e;
  sFORALL(a,e)
    if(sCmpStringI(e->*o,i)==0)
      return e;
  return 0;
}

//! Find first element with a certain member by using file-path insensitive string compares (sCmpStringP())
template <class ArrayType,class BaseType,class MemberType,class CompareType> 
BaseType *sFindStringP(ArrayType &a,MemberType BaseType::*o,const CompareType i)
{
  BaseType *e;
  sFORALL(a,e)
    if(sCmpStringP(e->*o,i)==0)
      return e;
  return 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Other Stuff                                                        ***/
/***                                                                      ***/
/****************************************************************************/

// we need template<typename> here for the acse that BaseType is not a pointer
//! Reverse elements in an array.
template <template <typename> class ArrayType,class BaseType> 
void sReverse(ArrayType<BaseType> &a)
{
  sInt max = a.GetCount();
  for(sInt i=0;i<max/2;i++)
    a.Swap(i,max-i-1);
}

//! \ingroup altona_base_types_arrays
//! Node for sDList and sDList2

class sDNode
{
  template <class Type,sDNode Type::*Offset> friend class sDList;
  template <class Type,sDNode Type::*Offset> friend class sDListRange;
  template <class Type> friend class sDList2;
  sDNode *Next;
  sDNode *Prev;
  void Reset() { Next = Prev = 0; }

public:
  //! constructor
  sDNode() { Next = Prev = 0; }
  //! node is part of a list
  sBool IsValid() const { return Next&&Prev; }
  //! remove node from list
  void Rem()   { Next->Prev = Prev; Prev->Next = Next; Reset(); }
};

//! \ingroup altona_base_types_arrays
//! Double linked list with anchor nodes at start and end.
//! Put sDNode members in your node class.

template <class Type,sDNode Type::*Offset> class sDList
{
  sDNode First;
  sDNode Last;

  const sDNode *GetNode(const Type* e) const   { return &(e->*Offset); } //{ return (sDNode *)(((sU8*)e)+Offset); }
  const Type* GetType(const sDNode *n) const   { return (const Type*) (((sU8*)n)-sOFFSETOMP((Type *)0,Offset)); }

  sDNode *GetNode(Type* e)                     { return &(e->*Offset); } // (sDNode *)(((sU8*)e)+Offset); }
  Type* GetType(sDNode *n)                     { return (Type*) (((sU8*)n)-sOFFSETOMP((Type *)0,Offset)); }

public:
  //! Element Type
  typedef typename sRemovePtrType<Type>::Type ElementType;

  //! Make list empty
  void Clear()                    { First.Next=&Last; First.Prev=0; Last.Next=0; Last.Prev=&First; }
  //! Constructor
  sDList()                        { Clear(); }
  //! Iterate through list and count elements
  int GetCount() const            { int count=0; for (const Type* iter = GetHead(); !IsEnd(iter); iter=GetNext(iter), ++count) {} return count;}
  //! Check if list is empty
  sBool IsEmpty() const           { return (First.Next==&Last); }

  //! get next element in list
  const Type* GetNext(const Type* e) const            { return GetType(GetNode(e)->Next); } // { return (Type*) (((sU8*)(GetNode(e)->Next))-MO(Offset)); }
  //! get prev element in list
  const Type* GetPrev(const Type* e) const            { return GetType(GetNode(e)->Prev); } // { return (Type*) (((sU8*)(GetNode(e)->Prev))-MO(Offset)); }
  //! get next element in list
  Type* GetNext(Type* e)                              { return GetType(GetNode(e)->Next); } // { return (Type*) (((sU8*)(GetNode(e)->Next))-MO(Offset)); }
  //! get prev element in list
  Type* GetPrev(Type* e)                              { return GetType(GetNode(e)->Prev); } // { return (Type*) (((sU8*)(GetNode(e)->Prev))-MO(Offset)); }
  //! check if first element in list
  sBool IsFirst(const Type* e) const     { return (GetNode(e)->Prev->Prev==0); }
  //! check if last element in list
  sBool IsLast(const Type* e) const      { return (GetNode(e)->Next->Next==0); }
  //! If you call GetNext() on the last element of a list, you will get a pointer to an anchor node. You may use this pointer only to call IsEnd(), which will then return true.
  sBool IsEnd(const Type* e) const       { return (GetNode(e)->Next==0); }
  //! If you call GetPrev() on the first element of a list, you will get a pointer to an anchor node. You may use this pointer only to call IsFirst(), which will then return true.
  sBool IsStart(const Type* e) const     { return (GetNode(e)->Prev==0); }

  //! get first element of list, or the end anchor if the list is empty.
  Type* GetHead()                  { return GetType(First.Next); }
  //! get last element of list, or the start anchor if the list is empty.
  Type* GetTail()                  { return GetType(Last.Prev); }
  //! get first element of list, or the end anchor if the list is empty.
  const Type* GetHead() const      { return GetType(First.Next); }
  //! get last element of list, or the start anchor if the list is empty.
  const Type* GetTail() const      { return GetType(Last.Prev); }
  //! Iterate through the list and return an element.
  Type* GetSlow(sInt n)            { Type* e=GetHead(); for(;;) { if(IsEnd(e)) return 0; if(n==0) return e; e=GetNext(e); n--; } }
  //! Iterate through the list searching for an element and return its index.
  sInt GetIndex(Type* e) const     { const Type* h=GetType(First.Next); for(sInt i=0;!IsEnd(h);i++, h=GetNext(h)) if (e==h) return i; return -1; }

  //! Insert at beginning of list.
  void AddHead(Type* e)            { GetNode(e)->Next = First.Next; GetNode(e)->Prev=&First; First.Next->Prev = GetNode(e); First.Next=GetNode(e); }
  //! Insert at end of list.
  void AddTail(Type* e)            { GetNode(e)->Prev = Last .Prev; GetNode(e)->Next=&Last ; Last .Prev->Next = GetNode(e); Last .Prev=GetNode(e); }
  //! Remove element from list
  void Rem(Type* e)                { GetNode(e)->Next->Prev = GetNode(e)->Prev; GetNode(e)->Prev->Next = GetNode(e)->Next; GetNode(e)->Reset(); }
  //! Remove first element from list and return it. Return 0 if the list is empty.
  Type* RemHead()                  { if(IsEmpty()) return 0; Type* e=GetHead(); Rem(e); return e; }
  //! Remove last element from list and return it. Return 0 if the list is empty.
  Type* RemTail()                  { if(IsEmpty()) return 0; Type* e=GetTail(); Rem(e); return e; }
  //! Add element before reference element.
  void AddBefore(Type* e,Type* ref) { GetNode(e)->Prev=GetNode(ref)->Prev; GetNode(e)->Next=GetNode(ref); GetNode(ref)->Prev->Next=GetNode(e); GetNode(ref)->Prev=GetNode(e); }
  //! Add element after reference element.
  void AddAfter(Type* e,Type* ref)  { GetNode(e)->Next=GetNode(ref)->Next; GetNode(e)->Prev=GetNode(ref); GetNode(ref)->Next->Prev=GetNode(e); GetNode(ref)->Next=GetNode(e); }

  //! Insert another list at the beginning of this list. The other list will be empty afterwards.
  void AddHead(sDList<Type,Offset> &l)
  {
    l.Last.Prev->Next = First.Next;
    First.Next->Prev = l.Last.Prev; 
    First.Next = l.First.Next;
    l.First.Next->Prev = &First; 
    l.Clear(); 
  }
  //! Insert another list at the end of this list. The other list will be empty afterwards.
  void AddTail(sDList<Type,Offset> &l)    
  {
    l.First.Next->Prev = Last.Prev;
    Last.Prev->Next = l.First.Next; 
    Last.Prev = l.Last.Prev;
    l.Last.Prev->Next = &Last; 
    l.Clear(); 
  }
};

//! Delete all elements of a list
template <class Type,sDNode Type::*Offset> sINLINE void sDeleteAll (sDList<Type,Offset> &list)
{
  while (!list.IsEmpty()) delete list.RemHead();
}

//! \ingroup altona_base_types_arrays
//! Double linked list. 
//! Put an sDNode member called \b Node in your node class.
//! see sDList for member documentation

template <class Type> class sDList2
{
  sDNode First;
  sDNode Last;

  const sDNode *GetNode(const Type* e) const   { return &(e->Node); } 
  const Type* GetType(const sDNode *n) const   { return (const Type*) (((sU8*)n)-sOFFSETOMP((Type *)0,&Type::Node)); }

  sDNode *GetNode(Type* e)                     { return &(e->Node); } 
  Type* GetType(sDNode *n)                     { return (Type*) (((sU8*)n)-sOFFSETOMP((Type *)0,&Type::Node)); }

public:
  typedef typename sRemovePtrType<Type>::Type ElementType;

  void Clear()                    { First.Next=&Last; First.Prev=0; Last.Next=0; Last.Prev=&First; }
  sDList2()                       { Clear(); }
  int GetCount() const            { int count=0; for (const Type* iter = GetHead(); !IsEnd(iter); iter=GetNext(iter), ++count) {} return count;}
  sBool IsEmpty() const           { return (First.Next==&Last); }

  const Type* GetNext(const Type* e) const            { return GetType(GetNode(e)->Next); } // { return (Type*) (((sU8*)(GetNode(e)->Next))-MO(Offset)); }
  const Type* GetPrev(const Type* e) const            { return GetType(GetNode(e)->Prev); } // { return (Type*) (((sU8*)(GetNode(e)->Prev))-MO(Offset)); }
  Type* GetNext(Type* e)                              { return GetType(GetNode(e)->Next); } // { return (Type*) (((sU8*)(GetNode(e)->Next))-MO(Offset)); }
  Type* GetPrev(Type* e)                              { return GetType(GetNode(e)->Prev); } // { return (Type*) (((sU8*)(GetNode(e)->Prev))-MO(Offset)); }
  sBool IsFirst(const Type* e) const     { return (GetNode(e)->Prev->Prev==0); }
  sBool IsLast(const Type* e) const      { return (GetNode(e)->Next->Next==0); }
  sBool IsEnd(const Type* e) const       { return (GetNode(e)->Next==0); }
  sBool IsStart(const Type* e) const     { return (GetNode(e)->Prev==0); }

  Type* GetHead()                  { return GetType(First.Next); }
  Type* GetTail()                  { return GetType(Last.Prev); }
  const Type* GetHead() const      { return GetType(First.Next); }
  const Type* GetTail() const      { return GetType(Last.Prev); }
  Type* GetSlow(sInt n)            { Type* e=GetHead(); for(;;) { if(IsEnd(e)) return 0; if(n==0) return e; e=GetNext(e); n--; } }
  sInt GetIndex(Type* e) const     { const Type* h=GetType(First.Next); for(sInt i=0;!IsEnd(h);i++, h=GetNext(h)) if (e==h) return i; return -1; }

  void AddHead(Type* e)            { GetNode(e)->Next = First.Next; GetNode(e)->Prev=&First; First.Next->Prev = GetNode(e); First.Next=GetNode(e); }
  void AddTail(Type* e)            { GetNode(e)->Prev = Last .Prev; GetNode(e)->Next=&Last ; Last .Prev->Next = GetNode(e); Last .Prev=GetNode(e); }
  void Rem(Type* e)                { GetNode(e)->Next->Prev = GetNode(e)->Prev; GetNode(e)->Prev->Next = GetNode(e)->Next;  GetNode(e)->Reset(); }
  Type* RemHead()                  { if(IsEmpty()) return 0; Type* e=GetHead(); Rem(e); return e; }
  Type* RemTail()                  { if(IsEmpty()) return 0; Type* e=GetTail(); Rem(e); return e; }
  void AddBefore(Type* e,Type* ref) { GetNode(e)->Prev=GetNode(ref)->Prev; GetNode(e)->Next=GetNode(ref); GetNode(ref)->Prev->Next=GetNode(e); GetNode(ref)->Prev=GetNode(e); }
  void AddAfter(Type* e,Type* ref)  { GetNode(e)->Next=GetNode(ref)->Next; GetNode(e)->Prev=GetNode(ref); GetNode(ref)->Next->Prev=GetNode(e); GetNode(ref)->Next=GetNode(e); }

  void AddHead(sDList2<Type> &l)
  {
    l.Last.Prev->Next = First.Next;
    First.Next->Prev = l.Last.Prev; 
    First.Next = l.First.Next;
    l.First.Next->Prev = &First; 
    l.Clear(); 
  }
  void AddTail(sDList2<Type> &l)    
  {
    l.First.Next->Prev = Last.Prev;
    Last.Prev->Next = l.First.Next; 
    Last.Prev = l.Last.Prev;
    l.Last.Prev->Next = &Last; 
    l.Clear(); 
  }};

//! Delete all elements of a list
template <class Type> sINLINE void sDeleteAll (sDList2<Type> &list)
{
  while (!list.IsEmpty()) delete list.RemHead();
}

//! iterate through sDList or sDList2
#define sFORALL_LIST(l,p)         for((p)=(l).GetHead();!(l).IsEnd(p)  ;(p)=(l).GetNext(p) )
//! iterate through sDList or sDList2 in reverse order.
#define sFORALL_LIST_REVERSE(l,p) for((p)=(l).GetTail();!(l).IsStart(p);(p)=(l).GetPrev(p) )


/****************************************************************************/
/***                                                                      ***/
/***   Blob                                                               ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   A blob is a refcounted binary block. There are no copy semantics,  ***/
/***   so a copy of a blob will point to the same area of memory,         ***/
/***   providing for efficient handling of binary buffers and their       ***/
/***   ownership issues.                                                  ***/
/***                                                                      ***/
/****************************************************************************/

class sBlob
{
  sU32 *Ptr;
  void AddRef() { if (Ptr) Ptr[1]++; }

public:

  // construction
  sBlob() { Ptr=0; }
  explicit sBlob(sInt bytes) { Ptr=0; Create(bytes); }
  sBlob(const sBlob &blob) { Ptr=blob.Ptr; AddRef(); }

  // c++ stuff
  sBlob & operator = (const sBlob &blob) { sU32 *oldPtr = Ptr; Ptr=blob.Ptr; AddRef(); Ptr=oldPtr; Release(); Ptr=blob.Ptr; return *this; }
  ~sBlob() { Release(); }

  // creation/deletion
  void Create(sInt bytes) { Release(); Ptr = new sU32[(bytes+sizeof(sU32)-1)/sizeof(sU32)+2]; Ptr[0]=bytes; Ptr[1]=1; }
  void Release() { if (Ptr && !--Ptr[1]) { delete[] Ptr; Ptr=0; } }

  // resizing (only smaller and memory usage will stay the same)
  void Resize(sInt newsize) { sVERIFY(Ptr); if (Ptr) { sInt &size=*(sInt*)Ptr; sVERIFY(newsize>=0 && newsize<=size); size=newsize; }}

  // data retrieval
  sBool Valid() const { return Ptr!=0; }
  sInt GetSize() const { sVERIFY(Ptr); if (Ptr) return sInt(Ptr[0]); else return 0; } 
  template <typename T> T * GetPtr() { sVERIFY(Ptr); if (Ptr) return reinterpret_cast<T*>(Ptr+2); else return 0; }
  template <typename T> const T * GetPtr() const { sVERIFY(Ptr); if (Ptr) return reinterpret_cast<const T*>(Ptr+2); else return 0; }
};


/****************************************************************************/
/***                                                                      ***/
/***   memory heap                                                        ***/
/***                                                                      ***/
/****************************************************************************/

struct sMemoryHeapFreeNode
{
  sDNode Node;
  sPtr Size;
};

class sMemoryHeap : public sMemoryHandler
{
protected:
  sPtr TotalFree;                // amount of memory that SHOULD be free
  sInt Clear;                     // clear memory on allocation and freeing

  sDList<sMemoryHeapFreeNode,&sMemoryHeapFreeNode::Node> FreeList;
  sMemoryHeapFreeNode *LastFreeNode;

public:
  sMemoryHeap();
  void Init(sU8 *start,sPtr size);
  void SetDebug(sBool clear,sInt memwall);

  sCONFIG_SIZET GetFree();
  sCONFIG_SIZET GetLargestFree();
  sPtr GetUsed();
  void DumpStats(sInt verbose=0);
  sBool IsFree(const void *ptr) const;
  void ClearFree();               // clear free memory. for debugging

  void *Alloc(sPtr bytes,sInt align,sInt reverse=0);
  sBool Free(void *);
  sPtr MemSize(void *);
  void Validate();
  sU32 MakeSnapshot();
};

/****************************************************************************/
/***                                                                      ***/
/***   memory heap that stores it's free list and alloc info OUTSIDE      ***/
/***   usefull for gpu-memory scenarious (slow read access)               ***/
/***                                                                      ***/
/****************************************************************************/

struct sGpuHeapFreeNode
{
  sPtr Data;
  sPtr Size;
  sDNode Node;
};

struct sGpuHeapUsedNode
{
  sPtr Data;
  sPtr Size;
  sGpuHeapUsedNode *Next;
  sPtr pad;
};


class sGpuHeap : public sMemoryHandler
{
protected:
  sGpuHeapUsedNode **HashTable;
  sInt HashSize;
  sInt HashMask;
  sInt HashShift;
  sPtr TotalFree;

  sBool Clear;

  sDList<sGpuHeapFreeNode,&sGpuHeapFreeNode::Node> FreeList;
  sDList<sGpuHeapFreeNode,&sGpuHeapFreeNode::Node> UnusedNodes;
  sGpuHeapFreeNode *NodeMem;

  void AddNode(sGpuHeapUsedNode *l);                 // add to hashtable
  sGpuHeapUsedNode *FindNode(sPtr data,sBool remove); // find/remove from hashtable
public:
  sGpuHeap();
  ~sGpuHeap();
  void Init(sU8 *start,sPtr size,sInt maxallocations=0x1000);
  void Exit();

  void SetDebug(sBool clear,sInt memwall);
  sCONFIG_SIZET GetFree();
  sCONFIG_SIZET GetLargestFree();
  sPtr GetUsed();

  void Validate();
  void *Alloc(sPtr bytes,sInt align,sInt flags=0);
  sBool Free(void *);
  sPtr MemSize(void *);
  sU32 MakeSnapshot();
};

/****************************************************************************/
/***                                                                      ***/
/***   sSimpleMemPool                                                     ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   This is a pool from where you can cheaply allocate small objects.  ***/
/***   - does not grow                                                    ***/
/***   - constructors / destructors are not called                        ***/
/***   + everything will be correctly aligned                             ***/
/***                                                                      ***/
/****************************************************************************/

class sSimpleMemPool
{
  sBool DeleteMe;
  sPtr Start;
  sPtr Current;
  sPtr End;
public:

  sSimpleMemPool(sInt defaultsize = 65536);
  sSimpleMemPool(sU8 *data,sInt size);
  ~sSimpleMemPool();
  sU8 *Alloc(sInt size,sInt align);
  void Reset();     // frees all reserved memory after the first default size block
  template <typename T> T *Alloc(sInt n=1) { return (T*) Alloc(sizeof(T)*n,sALIGNOF(T)); }
};


/****************************************************************************/
/***                                                                      ***/
/***   MemoryPool                                                         ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   This is a pool from where you can cheaply allocate small objects.  ***/
/***   - unlike an array you can not enumerate them                       ***/
/***   + it grows more efficiently than an array                          ***/
/***   + it is more efficient in time and space than many new's           ***/
/***   - you can only delete all objects in one flush                     ***/
/***   - constructors / destructors are not called                        ***/
/***   + everything will be correctly aligned                             ***/
/***   + minimal template usage                                           ***/
/***                                                                      ***/
/****************************************************************************/

class sMemoryPool
{
  sPtr DefaultSize;
  sInt AllocFlags;
  struct sMemoryPoolBuffer
  {
    sU8 *Start;
    sU8 *Current;
    sU8 *End;
    sMemoryPoolBuffer *Next;
    sMemoryPoolBuffer(sPtr size,sInt flags);
    ~sMemoryPoolBuffer();
  };
  sMemoryPoolBuffer *Current;
  sMemoryPoolBuffer *First;
  sMemoryPoolBuffer *Last;

  struct sStackItem
  {
    sMemoryPoolBuffer *CBuffer;
    sU8 *CPtr;
  };
  sStaticArray<sStackItem> Stack;
public:

  sMemoryPool(sPtr defaultsize = 65536,sInt allocflags=sAMF_HEAP, sInt maxstacksize=16);
  ~sMemoryPool();
  sU8 *Alloc(sPtr size,sInt align);

  sPtr BytesAllocated() const;
  sPtr BytesReserved() const;

  void Reset();     // frees all reserved memory after the first default size block
  void Push();      // pushes the current allocation cfg to internal stack
  void Pop();       // everything allocated after last push is freed

  template <typename T> T *Alloc(sInt n=1) { return (T*) Alloc(sizeof(T)*n,sALIGNOF(T)); }
  sChar *AllocString(const sChar *);
  sChar *AllocString(const sChar *,sInt len);
};

/****************************************************************************/
/***                                                                      ***/
/***  Compression algorithms                                              ***/
/***                                                                      ***/
/****************************************************************************/

sBool sCompES(sU8 *s, sU8 *d, sInt ssize, sInt &dsize, sInt scan);
sBool sDeCompES(sU8 *s, sU8 *d, sInt ssize, sInt &dsize, sBool verify);

/****************************************************************************/
/***                                                                      ***/
/***  Hooks                                                               ***/
/***                                                                      ***/
/****************************************************************************/

// you may call hook->Add(), hook->Rem(), hook->Call() and delete hook
// even when the pointer hook is NULL.
//
// this uses a static array, assuming that there are rarely large amounts of
// hooks required. this is just to be able to put hooks in types, not types2.
// sDList would be more versatile, but sStaticArray is a lot leaner.

class sHooksBase
{
protected:
  struct HookInfo
  {
    void *Hook;
    void *User;
  };
  sStaticArray<HookInfo> *Hooks;

  void _Add(void *hook,void *user);
  void _Rem(void *hook,void *user);
public:
  sHooksBase(sInt max=16);
  ~sHooksBase();
};

class sHooks : public sHooksBase
{
  typedef void (*hfp)(void *user);
public:
  void Add(hfp hook,void *user=0) { _Add((void *)hook,user); }
  void Rem(hfp hook,void *user=0) { _Rem((void *)hook,user); }
  void Call() { if(this) { HookInfo *h; sFORALL(*Hooks,h) (*hfp(h->Hook))(h->User); } }
};

template <typename A>
class sHooks1 : public sHooksBase
{
  typedef void (*hfp)(A a,void *user);
public:
  void Add(hfp hook,void *user=0) { _Add((void *)hook,user); }
  void Rem(hfp hook,void *user=0) { _Rem((void *)hook,user); }
  void Call(A a) { if(this) { HookInfo *h; sFORALL(*Hooks,h) (*hfp(h->Hook))(a,h->User); } }
};
template <typename A,typename B>
class sHooks2 : public sHooksBase
{
  typedef void (*hfp)(A a,B b,void *user);
public:
  void Add(hfp hook,void *user=0) { _Add((void *)hook,user); }
  void Rem(hfp hook,void *user=0) { _Rem((void *)hook,user); }
  void Call(A a,B b) { if(this) { HookInfo *h; sFORALL(*Hooks,h) (*hfp(h->Hook))(a,b,h->User); } }
};

#if !sSTRIPPED
extern sHooks1<const sChar *> * sDebugOutHook;
#endif

/****************************************************************************/
/***                                                                      ***/
/***   memory debug helper                                                ***/
/***                                                                      ***/
/****************************************************************************/

#if !sRELEASE
/*
struct sMemDebugInfo
{
private:
  struct Item
  {
    Item():Ptr(0),File(0),Size(0),Line(0xffff),Next(0xffff) {}
    sPtr Ptr;
    const sChar8 *File;
    sInt Size;
    sU16 Line;
    sU16 Next;

    void Init(sDInt ptr, sInt size, const sChar8 *file, sU16 line, sU16 next);
  };

  Item ItemPool[sMEMSEG_DBGINFOMAX];
  sInt ItemCount;
  sU16 FreeList;
public:

  sU16 Add(sU16 id, sPtr ptr, sInt size, const sChar8 *file, sU16 line);
  sU16 Free(sU16 id, sPtr ptr);
  Item *Get(sU16 id);

  void FlushDump(sU16 id);
  void StatisticDump(sU16 id=0xffff);   // choose valid id for dumping only selected segment
  sInt GetAllocSize(sU16 id);

  sMemDebugInfo();
  void Init();
  void Exit();
};

void sDumpMemUsage();     // dumps allocated memory statistics
*/
#endif // !sRELEASE



/****************************************************************************/
/***                                                                      ***/
/***   Memory Leak Tracker.                                               ***/
/***                                                                      ***/
/****************************************************************************/
/*
class sMemoryLeakTracker1   // this is used internally to track memory leaks
{
public:
  struct LeakCheck
  {
    void *Ptr;
    sPtr Size;
    const sChar8 *File;
    sU16 Line;
    sU8 HeapId;
    sU8 pad;
    sInt AllocId;

    sInt Duplicates;      // temporary
    sPtr TotalBytes;

    sU32 Hash() const { return Line; }
    bool operator==(const LeakCheck &b) const
    { if(Line!=b.Line) return 0; 
      for(sInt i=0;File[i] || b.File[i];i++) 
        if(File[i]!=b.File[i]) return 0; 
      return 1; }

  };
  
private: 

  LeakCheck *LeakData;

  sInt LeakAlloc;
  sInt LeakUsed;
  sInt AllocFlags;
  class sThreadLock *Lock;

public:
  sMemoryLeakTracker1();
#if sPLATFORM==sPLAT_WINDOWS
  void Init(sInt listsize=100000,sInt allocflags=sAMF_DEBUG);
#else
  void Init(sInt listsize=10000,sInt allocflags=sAMF_DEBUG);
#endif
  void Exit();

  void DumpLeaks(const sChar *heapname,sInt minallocid=0);

  void AddLeak(void *ptr,sPtr size,const char *file,sInt line,sInt allocid,sInt heapid);
  void RemLeak(void *ptr);
  sBool HasLeak() const { return LeakUsed!=0; }
  sInt GetLeakCount() const { return LeakUsed; }

  sInt BeginGetLeaks(const LeakCheck *&leaks);
  void EndGetLeaks();
};
*/
/****************************************************************************/

struct sMemoryLeakDesc
{
  enum { STRINGLEN=256, };

  sString<STRINGLEN> String;

  sMemoryLeakDesc *NextHash;
  sInt AllocId;
  sInt Hash;
  sInt RefCount;
  sInt Size[sAMF_MASK+1]; // size per mem type
  sInt Count[sAMF_MASK+1];
  sDNode Node;
};


struct sMemoryLeakInfo      // combine all leaks at one location
{
  const sChar8 *File;           // file of location
  sInt Line;                    // line of location
  sInt HeapId;                  // heap id of first leak
  sInt Count;                   // total count
  sInt AllocId;                 // alloc id of first leak
  sInt TotalBytes;              // total size
  sInt FirstSize;               // size of first leak
  sBool ExtendedInfoAvailable;  // some of the contained leaks have extended informations to offer
};

struct sMemoryLeakInformationStrings
{
  sChar8 *InformationString;
  sMemoryLeakInformationStrings *NextInfo;
};

class sMemoryLeakTracker2  // this is used internally to track memory leaks
{
public:
  struct Leak
  {
    void *Ptr;
    sPtr Size;
    sInt AllocId;
    sInt HeapId;
    sMemoryLeakDesc *Desc;
    Leak *NextHash;
    sDNode Node;
  };
  struct LeakLoc
  {
    const sChar8 *File;
    sInt Line;
    sInt HeapId;
    sDList<Leak,&Leak::Node> Leaks;
    LeakLoc *NextHash;
  };


private: 

  static const sInt LocHashSize=0x0400;
  static const sInt LeakHashSize=0x4000;
  static const sInt DescHashSize=0x0100;

  sMemoryPool *Pool;
  LeakLoc *LocHash[LocHashSize];
  Leak *LeakHash[LeakHashSize];
  sMemoryLeakDesc *DescHash[DescHashSize];

  sInt LeakCount;
  sInt AllocFlags;
  class sThreadLock *Lock;
  sDList<Leak,&Leak::Node> FreeLeaks;

  sDList<sMemoryLeakDesc,&sMemoryLeakDesc::Node> Descs;
  sDList<sMemoryLeakDesc,&sMemoryLeakDesc::Node> FreeDescs;

  sMemoryLeakInformationStrings *InformationStrings;            // linear list of attached informationstrings

  sInt CmpString8(const sChar *a, const sChar8 *b);

public:
  sMemoryLeakTracker2();
  ~sMemoryLeakTracker2();
  void Init2(sInt allocflags=sAMF_DEBUG);
  void Exit();

  void DumpLeaks(const sChar *message,sInt minallocid=0, sInt heapid=0);
  
  void AddLeak(void *ptr,sPtr size,const char *file,sInt line,sInt allocid,sInt heapid, const sChar *desc, sU32 descCRC);
  void RemLeak(void *ptr);
  sBool HasLeak() const { return LeakCount!=0; }
  sInt GetLeakCount() const { return LeakCount; }

  void GetLeaks(sStaticArray<sMemoryLeakInfo> &,sInt minallocid);
  void GetLeakDescriptions(sStaticArray<sMemoryLeakDesc> &, sInt minallocid);  

  sChar8 *GetPersistentString8(const sChar *informationString);  // returns an equal string located in the list above
};

// memory leak description handlers
#if sCONFIG_DEBUGMEM

void sPushMemLeakDesc(const sChar *desc);
void sPopMemLeakDesc();

struct sMemLeakDescScope
{
  inline explicit sMemLeakDescScope(const sChar *str) { sPushMemLeakDesc(str); }
  inline ~sMemLeakDescScope() { sPopMemLeakDesc(); }
};

#else

#define sPushMemLeakDesc(x)
#define sPopMemLeakDesc()

struct sMemLeakDescScope
{
  inline explicit sMemLeakDescScope(const sChar *str) {}
  inline ~sMemLeakDescScope() { sPopMemLeakDesc(); }
};

#endif

#define sMEMLEAKDESC__(name,line) sMemLeakDescScope _mlddummy##line(name)
#define sMEMLEAKDESC_(name,line) sMEMLEAKDESC__(name,line)
#define sMEMLEAKDESC(name) sMEMLEAKDESC_(name,__LINE__)

/****************************************************************************/

typedef sMemoryLeakTracker2 sMemoryLeakTracker;

// For debugging purposes only. May and will return zero in eg. stripped builds
sMemoryLeakTracker * sGetMemoryLeakTracker();

/****************************************************************************/
/***                                                                      ***/
/***   Intrinsics MSC (Windows)                                           ***/
/***                                                                      ***/
/****************************************************************************/

#if sCONFIG_COMPILER_MSC

#include <math.h>

extern "C"
{
  int __cdecl abs(int);
  void * __cdecl memset( void *dest, int c, sCONFIG_SIZET count );
  void * __cdecl memcpy( void *dest, const void *src, sCONFIG_SIZET count );
  int __cdecl memcmp( const void *buf1, const void *buf2, sCONFIG_SIZET count );
  sCONFIG_SIZET __cdecl strlen( const char *string );
  __int64 __cdecl __emul(int a,int b);
  unsigned __int64 __cdecl __emulu(unsigned int a,unsigned int b);
}

#pragma intrinsic (abs,__emul,__emulu)                        // int intrinsic
#pragma intrinsic (memset,memcpy,memcmp,strlen)               // memory intrinsic
#pragma intrinsic (fabs)                                      // float intrinsic

sINLINE sInt sAbs(sInt i)                                  { return abs(i); }
sINLINE void sSetMem(void *dd,sInt s,sInt c)               { memset(dd,s,c); }
sINLINE void sCopyMem(void *dd,const void *ss,sInt c)      { memcpy(dd,ss,c); }
sINLINE sInt sCmpMem(const void *dd,const void *ss,sInt c) { return (sInt)memcmp(dd,ss,c); }

sINLINE sInt sMulDiv(sInt a,sInt b,sInt c)      { return sInt(__emul(a,b)/c); }
sINLINE sU32 sMulDivU(sU32 a,sU32 b,sU32 c)     { return sU32(__emulu(a,b)/c); }
sINLINE sInt sMulShift(sInt a,sInt b)           { return sInt(__emul(a,b)>>16); }
sINLINE sU32 sMulShiftU(sU32 a,sU32 b)           { return sU32(__emulu(a,b)>>16); }

sINLINE sInt sDivShift(sInt a,sInt b)           { return sInt((sS64(a)<<16)/b); }
sINLINE sF32 sAbs(sF32 f)                       { return fabs(f); }

#define sHASINTRINSIC_ABSI
#define sHASINTRINSIC_SETMEM
#define sHASINTRINSIC_COPYMEM
#define sHASINTRINSIC_CMPMEM
#define sHASINTRINSIC_MULDIV
#define sHASINTRINSIC_MULDIVU
#define sHASINTRINSIC_MULSHIFT
#define sHASINTRINSIC_MULSHIFTU
#define sHASINTRINSIC_DIVSHIFT
#define sHASINTRINSIC_ABSF

#pragma intrinsic (atan,atan2,cos,exp,log,log10,sin,sqrt,tan)

sINLINE sF32 sATan(sF32 f)                      { return (sF32)atan(f); }
sINLINE sF32 sATan2(sF32 over,sF32 under)       { return (sF32)atan2(over,under); }
sINLINE sF32 sCos(sF32 f)                       { return (sF32)cos(f); }
sINLINE sF32 sExp(sF32 f)                       { return (sF32)exp(f); }
sINLINE sF32 sLog(sF32 f)                       { return (sF32)log(f); }
sINLINE sF32 sLog10(sF32 f)                     { return (sF32)log10(f); }
sINLINE sF32 sSin(sF32 f)                       { return (sF32)sin(f); }
sINLINE sF32 sSqrt(sF32 f)                      { return (sF32)sqrtf(f); }
sINLINE sF32 sRSqrt(sF32 f)                     { return 1.0f/(sF32)sqrt(f); }
sINLINE sF32 sTan(sF32 f)                       { return (sF32)tan(f); }

sINLINE sF32 sFATan(sF32 f)                      { return (sF32)atan(f); }
sINLINE sF32 sFATan2(sF32 over,sF32 under)       { return (sF32)atan2(over,under); }
sINLINE sF32 sFCos(sF32 f)                       { return (sF32)cos(f); }
sINLINE sF32 sFExp(sF32 f)                       { return (sF32)exp(f); }
sINLINE sF32 sFLog(sF32 f)                       { return (sF32)log(f); }
sINLINE sF32 sFLog10(sF32 f)                     { return (sF32)log10(f); }
sINLINE sF32 sFSin(sF32 f)                       { return (sF32)sin(f); }
sINLINE sF32 sFTan(sF32 f)                       { return (sF32)tan(f); }

// Intel SSE seems to be too unpreceise which leads to jittering errors in animation code.
#if sRELEASE && 0
//sINLINE sF32 sFSqrt(sF32 f)                      { sF32 x; _mm_store_ss(&x,_mm_sqrt_ss(_mm_load_ss(&f))); return x; }
//sINLINE sF32 sFRSqrt(sF32 f)                     { sF32 x; _mm_store_ss(&x,_mm_rsqrt_ss(_mm_load_ss(&f))); return x; }
#else
// VS2005 has a bug where SSE intrinsics break debugging now and then. therefore...
sINLINE sF32 sFSqrt(sF32 f)                      { return (sF32)sqrtf(f); }
sINLINE sF32 sFRSqrt(sF32 f)                     { return 1.0f/(sF32)sqrt(f); }
#endif

#define sHASINTRINSIC_ATAN
#define sHASINTRINSIC_ATAN2
#define sHASINTRINSIC_COS
#define sHASINTRINSIC_EXP
#define sHASINTRINSIC_LOG
#define sHASINTRINSIC_LOG10
#define sHASINTRINSIC_SIN
#define sHASINTRINSIC_SQRT
#define sHASINTRINSIC_RSQRT
#define sHASINTRINSIC_TAN

#define sHASINTRINSIC_FATAN
#define sHASINTRINSIC_FATAN2
#define sHASINTRINSIC_FCOS
#define sHASINTRINSIC_FEXP
#define sHASINTRINSIC_FLOG
#define sHASINTRINSIC_FLOG10
#define sHASINTRINSIC_FSIN
#define sHASINTRINSIC_FSQRT
#define sHASINTRINSIC_FRSQRT
#define sHASINTRINSIC_FTAN

#endif    // compiler

/****************************************************************************/
/***                                                                      ***/
/***   Intrinsics GCC                                                     ***/
/***                                                                      ***/
/****************************************************************************/

#if sCONFIG_COMPILER_GCC           // no intrinsics: USE __builtin__

#include <math.h>


sINLINE sInt sAbs(sInt i)                                  { return __builtin_abs(i); }
sINLINE void sSetMem(void *dd,sInt s,sInt c)               { __builtin_memset(dd,s,c); }
sINLINE void sCopyMem(void *dd,const void *ss,sInt c)      { __builtin_memcpy(dd,ss,c); }
sINLINE sInt sCmpMem(const void *aa,const void *bb,sInt c) { return __builtin_memcmp(aa,bb,c); }
sINLINE sInt sMulDiv(sInt a,sInt b,sInt c)      { return sS64(a)*b/c; }
sINLINE sU32 sMulDivU(sU32 a,sU32 b,sU32 c)      { return sU64(a)*b/c; }
sINLINE sInt sMulShift(sInt a,sInt b)           { return (sS64(a)*b)>>16; }
sINLINE sU32 sMulShiftU(sU32 a,sU32 b)           { return (sU64(a)*b)>>16; }
sINLINE sInt sDivShift(sInt a,sInt b)           { return (sS64(a)<<16)/b; }
sINLINE sF32 sAbs(sF32 f)                       { return __builtin_fabsf(f); }

#define sHASINTRINSIC_ABSI
#define sHASINTRINSIC_SETMEM
#define sHASINTRINSIC_COPYMEM
#define sHASINTRINSIC_CMPMEM
#define sHASINTRINSIC_MULDIV
#define sHASINTRINSIC_MULDIVU
#define sHASINTRINSIC_MULSHIFT
#define sHASINTRINSIC_MULSHIFTU
#define sHASINTRINSIC_DIVSHIFT
#define sHASINTRINSIC_ABSF

sINLINE sF32 sCos(sF32 f)                       { return __builtin_cosf(f); }
sINLINE sF32 sSin(sF32 f)                       { return __builtin_sinf(f); }
sINLINE sF32 sSqrt(sF32 f)                      { return __builtin_sqrtf(f); }
sINLINE sF32 sFSqrt(sF32 f)                     { return __builtin_sqrtf(f); }
sINLINE sF32 sRSqrt(sF32 f)                     { return 1.0f/__builtin_sqrtf(f); }
sINLINE sF32 sFRSqrt(sF32 f)                    { return 1.0f/__builtin_sqrtf(f); }

#define sHASINTRINSIC_COS
#define sHASINTRINSIC_SIN
#define sHASINTRINSIC_SQRT
#define sHASINTRINSIC_FSQRT
#define sHASINTRINSIC_RSQRT
#define sHASINTRINSIC_FRSQRT

sINLINE sF32 sLog(sF32 x)                       { return __builtin_logf(x); }
sINLINE sF32 sExp(sF32 x)                       { return __builtin_expf(x); }

sINLINE sF32 sFLog(sF32 x)                       { return __builtin_logf(x); }
sINLINE sF32 sFExp(sF32 x)                       { return __builtin_expf(x); }

#define sHASINTRINSIC_LOG
#define sHASINTRINSIC_EXP

#define sHASINTRINSIC_FLOG
#define sHASINTRINSIC_FEXP

#endif // compiler


/****************************************************************************/
/****************************************************************************/

class sTextStreamer
{
public:
  virtual ~sTextStreamer() {}
  
  virtual const sStringDesc GetPrintBuffer() = 0;
  virtual void Print() = 0;

  sPRINTING0(PrintF,sFormatStringBuffer buf=sFormatStringBase(GetPrintBuffer(),format);buf,Print(););
};

/****************************************************************************/

class sTextStreamerString : public sTextStreamer
{
public:
  sTextStreamerString(const sStringDesc &sd) : StringDescriptor(sd) {}
  
  const sStringDesc GetPrintBuffer() { return TextBuffer; }
  void Print() {sAppendString(StringDescriptor, TextBuffer); TextBuffer.Clear(); }

private:
  const sStringDesc StringDescriptor;
  sString<1024> TextBuffer;
};

/****************************************************************************/
/***                                                                      ***/
/***   Pairs                                                              ***/
/***                                                                      ***/
/****************************************************************************/

// sBasePair can be used as array initializer.
template<class KeyType,class ValueType>
struct sBasePair
{
  KeyType Key;
  ValueType Value;
};

// sPair cannot be used as array initializer.
// Use sBasePair for this instead.
template<class KeyType,class ValueType>
struct sPair
{
  sPair() {}
  sPair(const sBasePair<KeyType,ValueType> &x) : Key(x.Key), Value(x.Value) {}
  sPair(const KeyType &key, const ValueType &value) : Key(key), Value(value) {}

  KeyType Key;
  ValueType Value;
};


/****************************************************************************/
/***                                                                      ***/
/***   BitArray                                                           ***/
/***                                                                      ***/
/****************************************************************************/


/************************************************************************/ /*! 
\ingroup altona_base_types_arrays

A simple array of bits with a predefined size.

Use this for bitmasks larger than 64 bits. Otherwise, simply use an sU64 
or sInt.

*/ /*************************************************************************/

template<int bits>
class sStaticBitArray
{
  static const sInt bytes = (bits+7)/8;
  sU8 Data[bytes];
public:
  //! constructor. initalize full array with zero
  sStaticBitArray() { ClearAll(); }
  
  //! set a bit to 1
  void Set(sInt i)              { sVERIFY(i>=0 && i<bits); Data[i/8] |= 1<<(i&7); }
  //! clear a bit to 0
  void Clear(sInt i)            { sVERIFY(i>=0 && i<bits); Data[i/8] &= ~(1<<(i&7)); }
  //! set a bit to a value 0 or 1
  void Assign(sInt i,sInt v)    { sVERIFY(i>=0 && i<bits); if(v) Set(i); else Clear(i); }
  //! get a bit. return 0 or 1
  sInt Get(sInt i) const        { sVERIFY(i>=0 && i<bits); return (Data[i/8]>>(i&7))&1; }
  //! clear all bits
  void ClearAll()               { for(sInt i=0;i<bytes;i++) Data[i] = 0x00; }
  //! set all bits
  void SetAll()                 { for(sInt i=0;i<bytes;i++) Data[i] = 0xff; }

  //! return the size (in bits) of the array.
  sInt Size() const             { return bits; }

  //! index the bits like an array. read only.
  sInt operator[](sInt i) const { return Get(i); }
};

/****************************************************************************/
/****************************************************************************/

// HEADER_ALTONA_BASE_TYPES
#endif


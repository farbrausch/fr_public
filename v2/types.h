#ifndef TYPES_H_
#define TYPES_H_

#define _CRT_SECURE_NO_DEPRECATE
#define V2TYPES

#include <stdint.h>
#include <stddef.h>

#ifndef _WIN32
  #ifndef __stdcall
    #define __stdcall
  #endif
  #ifndef __cdecl
    #define __cdecl
  #endif
#endif

typedef int               sInt;
typedef unsigned int      sUInt;
typedef sInt              sBool;
typedef char              sChar;

typedef int8_t            sS8;
typedef int16_t           sS16;
typedef int32_t           sS32;
typedef int64_t           sS64;

typedef uint8_t           sU8;
typedef uint16_t          sU16;
typedef uint32_t          sU32;
typedef uint64_t          sU64;

typedef float             sF32;
typedef double            sF64;

#define sTRUE             1
#define sFALSE            0

//
#ifdef _DEBUG
extern void __cdecl printf2(const char *format, ...);
#else
#define printf2(...)
#endif

template<class T> inline T sMin(const T a, const T b) { return (a<b)?a:b;  }
template<class T> inline T sMax(const T a, const T b) { return (a>b)?a:b;  }
template<class T> inline T sClamp(const T x, const T min, const T max) { return sMax(min,sMin(max,x)); }

#endif
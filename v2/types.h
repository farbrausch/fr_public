#ifndef TYPES_H_
#define TYPES_H_

#define _CRT_SECURE_NO_DEPRECATE
#define V2TYPES

typedef int               sInt;
typedef unsigned int      sUInt;
typedef sInt              sBool;
typedef char              sChar;

typedef signed   char     sS8;
typedef signed   short    sS16;
typedef signed   long     sS32;
typedef signed   __int64  sS64;

typedef unsigned char     sU8;
typedef unsigned short    sU16;
typedef unsigned long     sU32;
typedef unsigned __int64  sU64;

typedef float             sF32;
typedef double            sF64;

#define sTRUE             1
#define sFALSE            0

//
#ifdef _DEBUG
extern void __cdecl printf2(const char *format, ...);
#else
#define printf2
#endif

template<class T> inline T sMin(const T a, const T b) { return (a<b)?a:b;  }
template<class T> inline T sMax(const T a, const T b) { return (a>b)?a:b;  }
template<class T> inline T sClamp(const T x, const T min, const T max) { return sMax(min,sMin(max,x)); }

#endif
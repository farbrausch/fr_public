// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once

#ifndef __TYPES_HPP__
#define __TYPES_HPP__

/****************************************************************************/
/***                                                                      ***/
/***   Compiler                                                           ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef sNEWCONFIG
#ifdef _WIN32_WCE
#include "player_mobile/player_mobile_config.hpp" // ugly hack 
#else
#include "base/base_config.hpp"     // read default config
//#include "xmmintrin.h"
#endif
#endif

#ifndef _WIN32_WCE
#include <stddef.h>
#include <string.h>
//#include "xmmintrin.h"
#endif

#define sPLAT_PC      1
#define sPLAT_PDA     2

#if _WIN32_WCE
#define sPLATFORM     sPLAT_PDA
#include "Cmnintrin.h"
#else
#define sPLATFORM     sPLAT_PC
#endif


/****************************************************************************/

#pragma warning (disable : 4068)    // unknown pragma (for lekktor)
#pragma warning (disable : 4244)    // 'double' to 'float'
//#pragma warning (disable : 4018)  // signed/unsigned mismatch
#pragma warning (disable : 4251)    // dll-interfaces needed for baseclass (template problem)

#pragma inline_recursion (on)       // get full inline power
#pragma inline_depth (255)

#define sSTRINGIZE_(x) #x
#define sSTRINGIZE(x) sSTRINGIZE_(x)
#define sWARNING(x)  message (__FILE__ "(" sSTRINGIZE(__LINE__) "):" x)

#ifdef _WIN32_WCE
#define __w64
#endif

/****************************************************************************/

#if _DEBUG                          // switch on debugs in debug build
#undef sDEBUG
#define sDEBUG 1
#define sRELEASE 0
#else
#define sRELEASE 1
#endif

#ifndef sUSE_SHADERS
#pragma message("please set sUSE_SHADERS!")
#define sUSE_SHADERS 0
#endif
#ifndef sUNICODE
#pragma message("please set sUNICODE!")
#define sUNICODE 0
#endif
#ifndef sMOBILE
#pragma message("please set sMOBILE!")
#define sMOBILE 0
#endif

/****************************************************************************/

//#if _DEBUG                          // fix memory management
#if !sINTRO && sPLATFORM!=sPLAT_PDA
#define _MFC_OVERRIDES_NEW
void *  __cdecl operator new(unsigned int);
void *  __cdecl operator new(unsigned int,const char *,int);
inline void __cdecl operator delete(void *p, const char *, int s) { ::operator delete(p); }
#define new new(__FILE__,__LINE__)
#endif
//#endif

#define sNORETURN __declspec(noreturn)  // use this for dead end funtions

/****************************************************************************/
/***                                                                      ***/
/***   Init/Exit                                                          ***/
/***                                                                      ***/
/****************************************************************************/

void sInitTypes();
void sExitTypes();

/****************************************************************************/
/***                                                                      ***/
/***   Forwards                                                           ***/
/***                                                                      ***/
/****************************************************************************/

struct sRect;
struct sFRect;

struct sVector;
struct sMatrix;

class sObject;
class sBroker_;
class sBitmap;

/****************************************************************************/
/***                                                                      ***/
/***   Class ID's                                                         ***/
/***                                                                      ***/
/****************************************************************************/

#define sCID_INVALID        0x00000000  // no object or garbage
#define sCID_UNKNOWN        0x00000001  // valid object of unknown type
#define sCID_OBJECT         0x00100001
#define sCID_LIST           0x00100002
#define sCID_BITMAP         0x00100003
//#define sCID_MESH           0x00100004
#define sCID_DYNAMIC        0x00100005
#define sCID_GUIPAINTER     0x00100006
#define sCID_GUIMANAGER     0x00100007
#define sCID_TEXT           0x00100008
#define sCID_GUIWINDOW      0x00100009
#define sCID_DISKITEM       0x0010000a
#define sCID_DIROOT         0x0010000b
#define sCID_DIFILE         0x0010000c
#define sCID_DIDIR          0x0010000d
#define sCID_DIAPP          0x0010000e
#define sCID_DIFOLDER       0x0010000f
#define sCID_DISYSTEMINFO   0x00100010
#define sCID_CONTROL        0x00100011
#define sCID_VSPLITFRAME    0x00100012
#define sCID_HSPLITFRAME    0x00100013
#define sCID_SWITCHFRAME    0x00100014
#define sCID_SIZEBORDER     0x00100015
#define sCID_DIALOGWINDOW   0x00100016

#define sCID_MINIED         0x01000001  // minimal text (ascii) editor
#define sCID_MINIPAINT      0x01000002  // minimal 2d-painter
#define sCID_DESKMANAGER    0x01000003
#define sCID_DESKINDEX      0x01000004  // only used for save files
#define sCID_TEXGEN         0x01000005  // 16 bit per color gun texture generator
#define sCID_MESHGEN        0x01000006  // winged edge mesh generator

#define sCID_GENPLAYER      0x01000007
#define sCID_GENMATERIAL    0x01000008
#define sCID_GENBITMAP      0x01000009
#define sCID_GENCAMERA      0x0100000a
#define sCID_GENMESH        0x0100000b
#define sCID_GENSCENE       0x0100000c
#define sCID_GENMATPASS     0x0100000d
#define sCID_GENFXCHAIN			0x0100000e
#define sCID_GENMATRIX      0x0100000f
#define sCID_GENSPLINE      0x01000010
#define sCID_GENMESHANIM    0x01000011
#define sCID_GENLIGHT       0x01000012
#define sCID_GENPARTICLES   0x01000013
#define sCID_WERKK3CONFIG   0x01000014  // config file for werkkzeug3
#define sCID_WZMOBILE_DOC   0x01000015  // document file for wz_mobile

// allocation of id's;
//
// lower 20 bits: running number
// middle 8 bits: computer id;
// upper 4 bits: usage id
//
// the computer id depends on the usage bits.
//
// usage $0: class id 
// computer id: globally assigned by chaos
//   00  illegal
//   01  system code
//   02  temporary 
//   10  chaos personally
//   11  ...
//
//
// usage $8-$f: object id
// computer id: extended to 11 bit, globally assigned by chaos
// fist $100 id's identical to coder id's
// last $100 id's (with $Fxxrrrrr) can be used at will in closed comunities
//
// CID's are also used for save-files. the standard save-file header is
// (in this example a sCID_MINIED document
//
// 0x????????   '>>>>'
// 0x00000001   version
// 0x00000000   flags
// 0x01000001   sCID_MINIED
// ....
// 0x????????   '<<<<'

/****************************************************************************/
/***                                                                      ***/
/***   Base Types and Funcs                                               ***/
/***                                                                      ***/
/****************************************************************************/

typedef unsigned char             sU8;      // for packed arrays
typedef unsigned short            sU16;     // for packed arrays
typedef unsigned int              sU32;     // for packed arrays and bitfields
typedef unsigned __int64          sU64;     // use as needed
typedef signed char               sS8;      // for packed arrays
typedef short                     sS16;     // for packed arrays
typedef int                       sS32;     // for packed arrays
typedef signed __int64            sS64;     // use as needed
typedef float                     sF32;     // basic floatingpoint
typedef double                    sF64;     // use as needed
typedef int                       sInt;     // use this most!
typedef int __w64                 sDInt;    // type for pointer diff
#if sUNICODE
typedef wchar_t                   sChar;    // type for strings
#else
typedef char                      sChar;    // type for strings
#endif
typedef wchar_t                   sWChar;   // type for unicode strings
typedef signed char               sBool;    // not so usefull, don't use
typedef void*                     sPtr;     // not so usefull, don't use
typedef const void*               sCPtr;    // not so usefull, don't use

/****************************************************************************/

#define sVARARG(p,i) (((sInt *)(p))[(i)+1])
#define sVARARGF(p,i) (*(sF64*)&(((sInt *)(p))[(i)+1]))
#define sOFFSET(t,m) ((sDInt)(&((t*)0)->m))
#define sMAKECID(u,s,c) (((u)<<16)|((s)<<8)|(c))
#define sMAKE2(a,b) (((b)<<16)|(a))
#define sMAKE4(a,b,c,d) (((d)<<24)|((c)<<16)|((b)<<8)|(a))
#define sCOUNTOF(x) (sizeof(x)/sizeof(*(x)))    // #elements of array, usefull for unicode strings

#if sUNICODE
#define sTXT2(y) L ## y
#define sTXT(x) sTXT2(x)
#else
#define sTXT(x) x
#endif

/****************************************************************************/

#define sTRUE   (!0)
#define sFALSE  0
#if !sMOBILE
#define sPI     3.1415926535897932384626433832795
#define sPI2    6.28318530717958647692528676655901
#define sPIF    3.1415926535897932384626433832795f
#define sPI2F   6.28318530717958647692528676655901f
#endif

/****************************************************************************/

template <class Type> __forceinline Type sMin(Type a,Type b)            {return (a<b) ? a : b;}
template <class Type> __forceinline Type sMax(Type a,Type b)            {return (a>b) ? a : b;}
template <class Type> __forceinline Type sSign(Type a)                  {return (a==0) ? 0 : (a>0) ? 1 : -1;}
template <class Type> __forceinline Type sRange(Type a,Type max,Type min) {return (a>=max) ? max : (a<=min) ? min : a;}
template <class Type> __forceinline void sSwap(Type &a,Type &b)         {Type s; s=a; a=b; b=s;}
template <class Type> __forceinline Type sAlign(Type a,sInt b)          {return (Type)((((sInt)a)+b-1)&(~(b-1)));}

template <class Type> __forceinline Type sSquare(Type a)                {return a*a;}

template <class Type> __forceinline void sDelete(Type &a)               {if(a) delete a; a=0;}
template <class Type> __forceinline void sDeleteArray(Type &a)          {if(a) delete[] a; a=0;}
template <class Type> __forceinline void sRelease(Type &a)              {if(a) a->Release(); a=0;}

sU32 sGetRnd();
sU32 sGetRnd(sU32 max);
void sSetRndSeed(sInt seed);
sInt sFindLowerPower(sInt x);
sInt sMakePower2(sInt val);
sInt sGetPower2(sInt val);

#if !sMOBILE
sF32 sFGetRnd();
sF32 sFGetRnd(sF32 max);
sU32 sDec3(sF32 x,sF32 y,sF32 z);
void sHermite(sF32 *d,sF32 *p0,sF32 *p1,sF32 *p2,sF32 *p3,sInt count,sF32 fade,sF32 t,sF32 c,sF32 b,sBool ignoretime=0);
void sHermiteD(sF32 *d,sF32 *dd,sF32 *p0,sF32 *p1,sF32 *p2,sF32 *p3,sInt count,sF32 fade,sF32 t,sF32 c,sF32 b,sBool ignoretime=0);
__forceinline sF32 sFade(sF32 a,sF32 b,sF32 fade)												{return a+(b-a)*fade;}
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Intrinsics                                                         ***/
/***                                                                      ***/
/****************************************************************************/

// integer

typedef unsigned int size_t;

extern "C"
{
  int __cdecl abs(int);
  /*void * __cdecl memset( void *dest, int c, size_t count );
  void * __cdecl memcpy( void *dest, const void *src, size_t count );*/
  int __cdecl memcmp( const void *buf1, const void *buf2, size_t count );
  //size_t __cdecl strlen( const char *string );
}

#pragma intrinsic (abs)                                       // int intrinsic
#pragma intrinsic (memset,memcpy,memcmp,strlen)               // memory intrinsic

__forceinline sInt sAbs(sInt i)                                 { return abs(i); }
__forceinline void sSetMem(sPtr dd,sInt s,sInt c)               { memset(dd,s,c); }
__forceinline void sCopyMem(sPtr dd,const void *ss,sInt c)      { memcpy(dd,ss,c); }
__forceinline sInt sCmpMem(const void *dd,const void *ss,sInt c) { return (sInt)memcmp(dd,ss,c); }
#if !sUNICODE
__forceinline sInt sGetStringLen(const sChar *s)                { return (sInt)strlen(s); }
#else
__forceinline sInt sGetStringLen(const sChar *s)                { sInt i;for(i=0;s[i];i++); return i; }
#endif

/****************************************************************************/

// float

#if !sMOBILE

extern "C"
{

  double __cdecl atan(double);
  double __cdecl atan2(double,double);
  double __cdecl cos(double);
  double __cdecl exp(double);
  double __cdecl fabs(double);
  double __cdecl log(double);
  double __cdecl log10(double);
  double __cdecl sin(double);
  double __cdecl sqrt(double);
  double __cdecl tan(double); 

  double __cdecl acos(double);
  double __cdecl asin(double);
  double __cdecl cosh(double);
  double __cdecl fmod(double,double);
  double __cdecl pow(double,double);
  double __cdecl sinh(double);
  double __cdecl tanh(double);
}

#pragma intrinsic (atan,atan2,cos,exp,log,log10,sin,sqrt,tan,fabs) // true intrinsic
#pragma intrinsic (acos,asin,cosh,fmod,pow,sinh,tanh)         // fake intrinsic

__forceinline sF64 sFATan(sF64 f)         { return atan(f); }
__forceinline sF64 sFATan2(sF64 a,sF64 b) { return atan2(a,b); }
__forceinline sF64 sFCos(sF64 f)          { return cos(f); }
__forceinline sF64 sFAbs(sF64 f)          { return fabs(f); }
__forceinline sF64 sFLog(sF64 f)          { return log(f); }
__forceinline sF64 sFLog10(sF64 f)        { return log10(f); }
__forceinline sF64 sFSin(sF64 f)          { return sin(f); }
__forceinline sF64 sFSqrt(sF64 f)         { return sqrt(f); }

__forceinline sF64 sFASin(sF64 f)         { return asin(f); }
__forceinline sF64 sFCosH(sF64 f)         { return cosh(f); }
__forceinline sF64 sFSinH(sF64 f)         { return sinh(f); }
__forceinline sF64 sFTanH(sF64 f)         { return tanh(f); }

__forceinline sF64 sFInvSqrt(sF64 f)      { return 1.0/sqrt(f); }

#if !sINTRO && !sNOCRT
__forceinline sF64 sFACos(sF64 f)         { return acos(f); }
__forceinline sF64 sFMod(sF64 a,sF64 b)   { return fmod(a,b); }
__forceinline sF64 sFExp(sF64 f)          { return exp(f); }
__forceinline sF64 sFPow(sF64 a,sF64 b)   { return pow(a,b); }
#else
sF64 sFACos(sF64 f);
sF64 sFPow(sF64 a,sF64 b);
sF64 sFMod(sF64 a,sF64 b);
sF64 sFExp(sF64 f);
#endif

// Calculate real roots of polynomials with given degrees:
// p(x) = coeffs[0]*x^0 + coeffs[1]*x^1 + ...
// Returns number of n real roots found, and the respective coordinates
// in roots [0..n-1]
sInt sQuadraticRoots(const sF32 *coeffs,sF32 *roots);

#endif

/****************************************************************************/
/***                                                                      ***/
/***   asm                                                                ***/
/***                                                                      ***/
/****************************************************************************/

#if !sMOBILE

#pragma warning (disable : 4035) 

__forceinline void sFloatFix()
{
  __asm
	{
    fclex;
		push		0103fh; // round to nearest even + single precision
		fldcw		[esp];
		pop			eax;
	}
}

__forceinline void sFloatDouble()
{
  __asm
  {
    fclex;
    push    0123fh; // round to nearest even + double precision
    fldcw   [esp];
    pop     eax;
  }
}

__forceinline void sFloatDen1()
{
  __asm
	{
    fclex;
		push		0141fh;
		fldcw		[esp];
		pop			eax;
	}
}
__forceinline void sFloatDen0()
{
  __asm
	{
    fclex;
		push		0143fh;
		fldcw		[esp];
		pop			eax;
	}
}

__forceinline sInt sFtol (const float f)
{
  __asm 
  {
    fld f
    push eax
    fistp dword ptr [esp]
    pop eax
  }
}

__forceinline sF32 sFRound (const float f)
{
  __asm 
  {
    fld f
    frndint
  }
}

__forceinline void sFSinCos(const float x,sF32 &sine,sF32 &cosine)
{
  __asm
  {
    fld x;
    fsincos;
    mov eax,[cosine];
    fstp dword ptr [eax];
    mov eax,[sine];
    fstp dword ptr [eax];
  }
}

__forceinline sInt sMulDiv(sInt var_a,sInt var_b,sInt var_c)
{
  __asm
  {
    mov eax,var_a
    imul var_b
    idiv var_c
  }
}

__forceinline sInt sMulShift(sInt var_a,sInt var_b)
{
  __asm
  {
    mov eax, var_a
    imul var_b
    shr eax, 16
    shl edx, 16
    add eax, edx
    //shrd eax, edx, 16
  }
}

__forceinline sInt sMulShift30(sInt var_a,sInt var_b)
{
  __asm
  {
    mov eax, var_a
    imul var_b
    shrd eax, edx, 30
  }
}

__forceinline sInt sDivShift(sInt var_a,sInt var_b)
{
  __asm
  {
    mov eax,var_a
    mov edx,eax
    shl eax,16
    sar edx,16
    idiv var_b
  }
}

__forceinline sInt sDivShift30(sInt var_a,sInt var_b)
{
  __asm
  {
    mov eax,var_a
    mov edx,eax
    shl eax,30
    sar edx,2
    idiv var_b
  }
}

sBool sNormalFloat(sF32 value);

#pragma warning (default : 4035) 

#else

__forceinline sInt sMulDiv(sInt var_a,sInt var_b,sInt var_c)
{
  return (sS32)( ((sS64)var_a)*((sS64)var_b)/var_c );
}

__forceinline sInt sMulShift(sInt var_a,sInt var_b)
{
  return (sS32)( ((sS64)var_a)*((sS64)var_b)>>16 );
}

__forceinline sInt sMulShift30(sInt var_a,sInt var_b)
{
  return (sS32)( ((sS64)var_a)*((sS64)var_b)>>30 );
}

__forceinline sInt sDivShift(sInt var_a,sInt var_b)
{
  return (sS32)( (((sS64)var_a)<<16)/((sS64)var_b) );
}

__forceinline sInt sDivShift30(sInt var_a,sInt var_b)
{
  return (sS32)( (((sS64)var_a)<<30)/((sS64)var_b) );
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Fixed Point Math (for mobile devices)                              ***/
/***                                                                      ***/
/****************************************************************************/

#if !sINTRO

sInt sISinOld(sInt);
sInt sICosOld(sInt);

#endif 

/****************************************************************************/
/***                                                                      ***/
/***   Memory and String                                                  ***/
/***                                                                      ***/
/****************************************************************************/

__forceinline void sSetMem1(sU8  *d,sInt s,sInt c)    {while(c--)*d++=s;}
__forceinline void sSetMem2(sU16 *d,sU16 s,sInt c)    {while(c--)*d++=s;}
__forceinline void sSetMem4(sU32 *d,sU32 s,sInt c)    {while(c--)*d++=s;}
__forceinline void sSetMem8(sU64 *d,sU64 s,sInt c)    {while(c--)*d++=s;}
void sCopyMem4(sU32 *d,const sU32 *s,sInt c);
void sCopyMem8(sU64 *d,const sU64 *s,sInt c);

sChar *sDupString(sChar *s,sInt minsize=8);
void sAppendString(sChar *d,const sChar *s,sInt size);
void sParentString(sChar *path);
sChar *sFileFromPathString(sChar *path);
sChar *sFileExtensionString(sChar *path);
sInt sCmpStringI(const sChar *a,const sChar *b);
sInt sCmpMemI(const sChar *a,const sChar *b,sInt len);
const sChar *sFindString(const sChar *s,const sChar *f);
const sChar *sFindStringI(const sChar *s,const sChar *f);
sInt sAtoi(const sChar *s);

void __cdecl sSPrintF(sChar *buffer,sInt size,const sChar *format,...); 

sInt sScanInt(const sChar *&scan);
sInt sScanInt(sChar *&scan);
sInt sScanHex(const sChar *&scan);
sInt sScanHex(sChar *&scan);
#if !sMOBILE
sF32 sScanFloat(const sChar *&scan);
sF32 sScanFloat(sChar *&scan);
#endif
sBool sScanString(const sChar *&scan,sChar *buffer,sInt size);
sBool sScanString(sChar *&scan,sChar *buffer,sInt size);
sBool sScanName(const sChar *&scan,sChar *buffer,sInt size);
sBool sScanName(sChar *&scan,sChar *buffer,sInt size);
void sScanSpace(const sChar *&scan);
void sScanSpace(sChar *&scan);
sBool sScanCycle(const sChar *cycle,sInt index,sInt &start,sInt &len);

void sCopyString(sChar *d,const sChar *s,sInt size);
sInt sCmpString(const sChar *a,const sChar *b);
sBool sFormatString(sChar *buffer,sInt size,const sChar *format,const sChar **fp); // return sFALSE if text is truncated!

/****************************************************************************/
/***                                                                      ***/
/***   Memory Stack Allocator                                             ***/
/***                                                                      ***/
/****************************************************************************/

#if !sMOBILE

struct sMemStack
{
private:
  sInt Size;
  sInt Used;
  sU8 *Mem;
  sBool Delete;
public:
  void Init();
  void Init(sInt size);
  void Init(sInt size,sU8 *mem);
  void Exit();
  void Flush();
  
  sU8 *Alloc(sInt size);
  template <class type> type *Alloc(sInt count=1) {return (type *)Alloc(sizeof(type)*count);}
};

struct sGrowableMemStack
{
private:
  struct Block
  {
    sU8 *Mem;
    sInt Used;
    Block *Next;
  };
  Block *Blocks;
  sInt BlockSize;
  sInt TotalUsed;

  void AddNewBlock();
  void FreeAllBlocks();

public:
  void Init(sInt size);
  void Exit();
  void Flush();

  sU8 *Alloc(sInt size);
  template <class type> type *Alloc(sInt count=1) {return (type *)Alloc(sizeof(type)*count);}
};

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Perlin Util                                                         ***/
/***                                                                      ***/
/****************************************************************************/

#if !sMOBILE

extern sVector sPerlinGradient3D[16];
extern sF32 sPerlinRandom[256][2];
extern sU8 sPerlinPermute[512];

void sInitPerlin();
void sPerlin3D(const sVector &pos,sVector &out);

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Compact value encodings                                            ***/
/***                                                                      ***/
/****************************************************************************/

#if !sMOBILE

extern void sWriteShort(sU8 *&data,sInt value); // 0<=value<=32767, 1..2 bytes
extern sInt sReadShort(const sU8 *&data);
extern void sWriteX16(sU8 *&data,sF32 value);   // 2 bytes
extern sF32 sReadX16(const sU8 *&data);
extern void sWriteF16(sU8 *&data,sF32 value);   // 2 bytes
extern sF32 sReadF16(const sU8 *&data);
extern void sWriteF24(sU8 *&data,sF32 value);   // 1/3 bytes
extern sF32 sReadF24(const sU8 *&data);

#endif

/****************************************************************************/
/***                                                                      ***/
/***   File Reading and Writing                                           ***/
/***                                                                      ***/
/****************************************************************************/

// unified file format:
//
// simply Write() each object with *p++
// each object has a header and a footer. other objects are embedded.
//
// sU32 sMAGIC_BEGIN        // '<<<<'
// sU32 CID                 // class id. may be sCID_UNKNOWN, but NOT for first object of file!
// sU32 Version             // starting with 1
// sU32 Size                // size of data in sU32 units, add to ptr to skip
// ... data ...             // sU32 aligned data
// sU32 sMAGIC_END          // '>>>>', not included in size

/****************************************************************************/

#define sMAGIC_BEGIN    0x3c3c3c3c          // '<<<<'
#define sMAGIC_END      0x3e3e3e3e          // '>>>>'

void sWriteString(sU32 *&,sChar *);
sBool sReadString(sU32 *&,sChar *,sInt max);
sU32 *sWriteBegin(sU32 *&,sU32 cid,sInt version);
void sWriteEnd(sU32 *&,sU32 *header);
sInt sReadBegin(sU32 *&,sU32 cid);
sBool sReadEnd(sU32 *&);

/****************************************************************************/
/***                                                                      ***/
/***   Debugging                                                          ***/
/***                                                                      ***/
/****************************************************************************/

sNORETURN void sVerifyFalse(const sChar *file,sInt line);

#if !sINTRO || !sRELEASE 

void sDPrint(sChar *text);
void __cdecl sDPrintF(const sChar *format,...);
sNORETURN void __cdecl sFatal(const sChar *format,...);

#else

inline void sDPrint(const sChar *text) {}
inline void __cdecl sDPrintF(const sChar *format,...) {}
sNORETURN void __cdecl sFatal(char *format,...);

#endif

#if sDEBUG
#if sRELEASE
#define sVERIFY(x) {if(!(x))sVerifyFalse(sTXT(__FILE__),__LINE__);}
#define sVERIFYFALSE {sVerifyFalse(sTXT(__FILE__),__LINE__);}
#else
#define sVERIFY(x) {if(!(x)){__asm { int 3 };sVerifyFalse(sTXT(__FILE__),__LINE__);}}
#define sVERIFYFALSE {__asm{ int 3 };sVerifyFalse(sTXT(__FILE__),__LINE__);}
#endif
#else
#define sVERIFY(x) {;}
#define sVERIFYFALSE {;}
#endif

/****************************************************************************/
/***                                                                       ***/
/***   Simple Structs                                                     ***/
/***                                                                      ***/
/****************************************************************************/

struct sInt2
{
  sInt x,y;
  void Init(sInt X,sInt Y) { x=X;y=Y; }
};

struct sInt3 
{
  sInt x,y,z;
  void Init(sInt X,sInt Y,sInt Z) { x=X;y=Y;z=Z; }
};

struct sInt4
{
  sInt x,y,z,w;
  void Init(sInt X,sInt Y,sInt Z,sInt W) { x=X;y=Y;z=Z;w=W; }
};

/****************************************************************************/

#if !sMOBILE

struct sF322
{
  sF32 x,y;
  void Init(sF32 X,sF32 Y) { x=X;y=Y; }

  const sF32 operator[](sInt ind) const                             {return (&x)[ind];}
  sF32 &operator [](sInt ind)                                       {return (&x)[ind];}
};

struct sF323
{
  sF32 x,y,z;
  void Init(sF32 X,sF32 Y,sF32 Z) { x=X; y=Y; z=Z; }

  const sF32 operator[](sInt ind) const                             {return (&x)[ind];}
  sF32 &operator [](sInt ind)                                       {return (&x)[ind];}
};

struct sF324
{
  sF32 x,y,z,w;
  void Init(sF32 X,sF32 Y,sF32 Z,sF32 W) { x=X;y=Y;z=Z;w=W; }

  const sF32 operator[](sInt ind) const                             {return (&x)[ind];}
  sF32 &operator [](sInt ind)                                       {return (&x)[ind];}
};

union sFSRT
{
  struct
  {
    sF323 s;
    sF323 r;
    sF323 t;
  };
  sF32 v[9];
};

#endif

/****************************************************************************/

struct sRect
{
  sInt x0,y0,x1,y1;
  __forceinline void Init() {x0=y0=x1=y1=0;}
  __forceinline void Init(sInt X0,sInt Y0,sInt X1,sInt Y1) {x0=X0;y0=Y0;x1=X1;y1=Y1;}
  __forceinline sInt XSize() const {return x1-x0;}
  __forceinline sInt YSize() const {return y1-y0;}
    void Init(const struct sFRect &r);
  sBool Hit(sInt x,sInt y);
  sBool Hit(const sRect &);
  sBool Inside(const sRect &);
  sBool IsInside(const sRect &r) const { return (r.x0<x1 && r.x1>x0 && r.y0<y1 && r.y1>y0); }   // this is inside r
  void And(const sRect &r);
  void Or(const sRect &r);
  void Sort();
  void Extend(sInt i);
};

/****************************************************************************/

#if !sMOBILE

struct sFRect
{
  sF32 x0,y0,x1,y1;
  __forceinline void Init() {x0=y0=x1=y1=0;}
  __forceinline void Init(sF32 X0,sF32 Y0,sF32 X1,sF32 Y1) {x0=X0;y0=Y0;x1=X1;y1=Y1;}
  __forceinline sF32 XSize() const {return x1-x0;}
  __forceinline sF32 YSize() const {return y1-y0;}
  void Init(const struct sRect &r);
  sBool Hit(sF32 x,sF32 y);
  sBool Hit(const sFRect &);
  void And(const sFRect &r);
  void Or(const sFRect &r);
  void Sort();
  void Extend(sF32 i);
};

#endif

/****************************************************************************/

struct sRandom
{
private:
  sU32 kern;
public:
  void Init();
  void Seed(sInt seed);
  sInt Int(sInt max);
  sInt Int16();
  sU32 Int32();
  sF32 Float(sF32 max);
};

/****************************************************************************/
/***                                                                      ***/
/***   Vector and Matrix                                                  ***/
/***                                                                      ***/
/****************************************************************************/

#if !sMOBILE

#define x_forceinline(a,b) a b
#define y_forceinline(a,b) a;

/****************************************************************************/

struct sVector
{
  sF32 x,y,z,w;

  __forceinline void Init()                                         {x=0; y=0; z=0; w=0;}
  __forceinline void Init(sF32 xx,sF32 yy,sF32 zz)                  {x=xx; y=yy; z=zz; w=0;}
  __forceinline void Init(sF32 xx,sF32 yy,sF32 zz,sF32 ww)          {x=xx; y=yy; z=zz; w=ww;}
  __forceinline void InitS(sF32 s)                                  {x=y=z=w=s;}
  __forceinline void Init3()                                        {x=0; y=0; z=0;}                            
  __forceinline void Init3(sF32 xx,sF32 yy,sF32 zz)                 {x=xx; y=yy; z=zz;}                         
  __forceinline void Init4()                                        {x=0; y=0; z=0; w=0;}                            
  __forceinline void Init4(sF32 xx,sF32 yy,sF32 zz,sF32 ww)         {x=xx; y=yy; z=zz; w=ww;}
  __forceinline void Init(const sF323 &v)                           {x=v.x; y=v.y; z=v.z; w=0;}
  __forceinline void Init(const sF324 &v)                           {x=v.x; y=v.y; z=v.z; w=v.w;}

  y_forceinline(void Init(sInt3 &v)                                 ,{x=v.x/65536.0f; y=v.y/65536.0f; z=v.z/65536.0f; w=0.0f;})
  y_forceinline(void Init(sInt4 &v)																	,{x=v.x/65536.0f; y=v.y/65536.0f; z=v.z/65536.0f; w=v.w/65536.0f;})
  y_forceinline(void InitColor(sU32 col)                            ,{x=((col>>16)&0xff)/255.0f;y=((col>>8)&0xff)/255.0f;z=((col>>0)&0xff)/255.0f;w=((col>>24)&0xff)/255.0f;})
  y_forceinline(void InitColorI(sU32 col)                           ,{x=((col>>0)&0xff)/255.0f;y=((col>>8)&0xff)/255.0f;z=((col>>16)&0xff)/255.0f;w=((col>>24)&0xff)/255.0f;})
  y_forceinline(sU32 GetColor()                                     ,{return sRange<sInt>(z*255,255,0)|(sRange<sInt>(y*255,255,0)<<8)|(sRange<sInt>(x*255,255,0)<<16)|(sRange<sInt>(w*255,255,0)<<24);})
  y_forceinline(sU32 GetColorI()                                    ,{return sRange<sInt>(x*255,255,0)|(sRange<sInt>(y*255,255,0)<<8)|(sRange<sInt>(z*255,255,0)<<16)|(sRange<sInt>(w*255,255,0)<<24);})

  const sF32 operator[](sInt ind) const                             {return (&x)[ind];}
  sF32 &operator [](sInt ind)                                       {return (&x)[ind];}

  bool operator == (const sVector &a) const                         {return x == a.x && y == a.y && z == a.z && w == a.w;}
  bool operator != (const sVector &a) const                         {return x != a.x || y != a.y || z != a.z || w != a.w;}

  sF32 Distance(const sVector &a) const                             {sVector v; v.Sub3(*this,a); return v.Abs3();}

  void InitRnd();
  void Write(sU32 *&p);
  void Write3(sU32 *&p);
  void Write3U(sU32 *&p);
  void Read(sU32 *&p);
  void Read3(sU32 *&p);
  void Read3U(sU32 *&p);

  y_forceinline(void Add3(const sVector &a,const sVector &b)        ,{x=a.x+b.x; y=a.y+b.y; z=a.z+b.z;}          )
  y_forceinline(void Sub3(const sVector &a,const sVector &b)        ,{x=a.x-b.x; y=a.y-b.y; z=a.z-b.z;}          )
  y_forceinline(void Mul3(const sVector &a,const sVector &b)        ,{x=a.x*b.x; y=a.y*b.y; z=a.z*b.z;}          )
  y_forceinline(void Neg3(const sVector &a)                         ,{x=-a.x; y=-a.y; z=-a.z;}                   )
  y_forceinline(void Add3(const sVector &a)                         ,{x+=a.x; y+=a.y; z+=a.z;}                   )
  y_forceinline(void Sub3(const sVector &a)                         ,{x-=a.x; y-=a.y; z-=a.z;}                   )
  y_forceinline(void Mul3(const sVector &a)                         ,{x*=a.x; y*=a.y; z*=a.z;}                   )
  y_forceinline(void Neg3()                                         ,{x=-x; y=-y; z=-z;}                         )
  y_forceinline(sF32 Dot3(const sVector &a) const                   ,{return x*a.x+y*a.y+z*a.z;}                 )
  y_forceinline(void AddMul3(const sVector &a,const sVector &b)     ,{x+=a.x*b.x; y+=a.y*b.y; z+=a.z*b.z;}       )
  y_forceinline(void Scale3(sF32 s)                                 ,{x*=s; y*=s; z*=s;}                         )
  y_forceinline(void Scale3(const sVector &a,sF32 s)                ,{x=a.x*s; y=a.y*s; z=a.z*s;}                )
  y_forceinline(void AddScale3(const sVector &a,sF32 s)             ,{x+=a.x*s; y+=a.y*s; z+=a.z*s;}             )
	void               Lin3(const sVector &a,const sVector &b,sF32 t) ;
                sF32 Abs3() const                                   ;
                void Unit3()                                        ;
                sF32 UnitAbs3()                                     ;
                void UnitSafe3()                                    ;
                void Rotate3(const sMatrix &m,const sVector &v)     ;
                void Rotate3(const sMatrix &m);                     ;
                void RotateT3(const sMatrix &m,const sVector &v)    ;
                void RotateT3(const sMatrix &m);                    ;
                void Cross3(const sVector &a,const sVector &b)      ;
                sInt Classify();

  y_forceinline(void Add4(const sVector &a,const sVector &b)        ,{x=a.x+b.x; y=a.y+b.y; z=a.z+b.z; w=a.w+b.w;}    )
  y_forceinline(void Sub4(const sVector &a,const sVector &b)        ,{x=a.x-b.x; y=a.y-b.y; z=a.z-b.z; w=a.w-b.w;}    )
  y_forceinline(void Mul4(const sVector &a,const sVector &b)        ,{x=a.x*b.x; y=a.y*b.y; z=a.z*b.z; w=a.w*b.w;}    )
  y_forceinline(void Neg4(const sVector &a)                         ,{x=-a.x; y=-a.y; z=-a.z; w=-a.w;}                )
  y_forceinline(void Add4(const sVector &a)                         ,{x+=a.x; y+=a.y; z+=a.z; w+=a.w;}                )
  y_forceinline(void Sub4(const sVector &a)                         ,{x-=a.x; y-=a.y; z-=a.z; w-=a.w;}                )
  y_forceinline(void Mul4(const sVector &a)                         ,{x*=a.x; y*=a.y; z*=a.z; w*=a.w;}                )
  y_forceinline(void Neg4()                                         ,{x=-x; y=-y; z=-z; w=-w;}                        )
  y_forceinline(void AddMul4(const sVector &a,const sVector &b)     ,{x+=a.x*b.x; y+=a.y*b.y; z+=a.z*b.z; w+=a.w*b.w;})
  y_forceinline(sF32 Dot4(const sVector &a) const                   ,{return x*a.x + y*a.y + z*a.z + w*a.w;}          )
  y_forceinline(void Scale4(sF32 s)                                 ,{x*=s; y*=s; z*=s; w*=s;}                        )
  y_forceinline(void Scale4(const sVector &a,sF32 s)                ,{x=a.x*s; y=a.y*s; z=a.z*s; w=a.w*s;}            )
  y_forceinline(void AddScale4(const sVector &a,sF32 s)             ,{x+=a.x*s; y+=a.y*s; z+=a.z*s; w+=a.w*s;}        )

	void               Lin4(const sVector &a,const sVector &b,sF32 t)	;
                sF32 Abs4() const                                   ;
                void Unit4()                                        ;
                sF32 UnitAbs4()                                     ;
                void UnitSafe4()                                    ;
                void Rotate4(const sMatrix &m,const sVector &v)     ;
                void Rotate4(const sMatrix &m);                     ;
                void Rotate34(const sMatrix &m,const sVector &v)    ;
                void Rotate34(const sMatrix &m);                    ;
                void RotateT4(const sMatrix &m,const sVector &v)    ;
                void RotateT4(const sMatrix &m);                    ;
                void RotatePl(const sMatrix &m,const sVector &v)    ;
                void RotatePl(const sMatrix &m)                     ;
                void Cross4(const sVector &a,const sVector &b)      ; // just set w=0
                void Plane(const sVector &a,const sVector &b,const sVector &c);

                void Verify()                                       { sVERIFY(sNormalFloat(x) && sNormalFloat(y) && sNormalFloat(z) && sNormalFloat(w)); }
};

/****************************************************************************/

void SSE_SinCos4(sF32 a[4],sF32 s[4],sF32 c[4]);

struct sMatrix
{
  sVector i,j,k,l;

  void Init();
  void InitRot(const sVector &);
  void InitRot(const sVector &v,sF32 angle);
  void InitDir(const sVector &);
  void InitEuler(sF32 a,sF32 b,sF32 c);
  void InitEulerPI2(const sF32 *a);
  void InitSRT(const sF32 *);
  void InitRandomSRT(const sF32 *);
  void InitSRTInv(const sF32 *);
  void InitClassify(sInt i);
  void PivotTransform(sF32 x,sF32 y,sF32 z);
  __forceinline void PivotTransform(const sVector &t) { PivotTransform(t.x,t.y,t.z); }
	
	void FindEuler(sF32 &a,sF32 &b,sF32 &c);

  const sVector &operator[](sInt ind) const                         {return (&i)[ind];}
  sVector &operator [](sInt ind)                                    {return (&i)[ind];}

  void Write(sU32 *&f);
  void WriteR(sU32 *&f);
  void Write33(sU32 *&f);
  void Write34(sU32 *&f);
  void Write33R(sU32 *&f);
  void Read(sU32 *&f);
  void ReadR(sU32 *&f);
  void Read33(sU32 *&f);
  void Read34(sU32 *&f);
  void Read33R(sU32 *&f);

  void Trans3();
  void InvCheap3();
  void Scale3(sF32 s);

  void Trans4();
  void InvCheap4();
  void Scale4(sF32 s);
  void Lin4(sMatrix &m0,sMatrix &m1,sF32 fade);

  void TransR();
  void Invert(const sMatrix &a);

  void Mul3(const sMatrix &a,const sMatrix &b);
  void Mul4(const sMatrix &a,const sMatrix &b); // a==*this allowed!
  void MulA(const sMatrix &a,const sMatrix &b); // affine, a==*this allowed
  void Mul3(const sMatrix &b)                                       {sMatrix a;a=*this;Mul3(a,b);}
  void Mul4(const sMatrix &b)                                       {Mul4(*this,b);}
  void MulA(const sMatrix &b)                                       {MulA(*this,b);}

  void Verify()                                                     {i.Verify();j.Verify();k.Verify();l.Verify();}
};

/****************************************************************************/

struct sQuaternion
{
  // Strange component order (here and in Init()) to keep up with standard
  // notation, which is real (scalar) part first, imaginary (vector) part
  // after
  sF32 w,x,y,z;

  __forceinline void Init()                                         {w=0; x=0; y=0; z=0;}
  __forceinline void Init(sF32 ww,sF32 xx,sF32 yy,sF32 zz)          {w=ww; x=xx; y=yy; z=zz;}
  void InitAxisAngle(const sVector &axis,sF32 angle);
  
  const sF32 operator[](sInt ind) const                             {return (&w)[ind];}
  sF32 &operator [](sInt ind)                                       {return (&w)[ind];}

  // Vector operations
  void Add(const sQuaternion &a,const sQuaternion &b)               {w=a.w+b.w; x=a.x+b.x; y=a.y+b.y; z=a.z+b.z;}
  void Sub(const sQuaternion &a,const sQuaternion &b)               {w=a.w-b.w; x=a.x-b.x; y=a.y-b.y; z=a.z-b.z;}
  void Scale(const sQuaternion &a,sF32 s)                           {w=s*a.w; x=s*a.x; y=s*a.y; z=s*a.z;} 
  void AddScale(const sQuaternion &a,const sQuaternion &b,sF32 s)   {w=a.w+s*b.w; x=a.x+s*b.x; y=a.y+s*b.y; z=a.z+s*b.z;}
  
  void Add(const sQuaternion &a)                                    {w+=a.w; x+=a.x; y+=a.y; z+=a.z;}
  void Sub(const sQuaternion &a)                                    {w-=a.w; x-=a.x; y-=a.y; z-=a.z;}
  void Scale(sF32 s)                                                {w*=s; x*=s; y*=s; z*=s;}
  void AddScale(const sQuaternion &a,sF32 s)                        {w+=s*a.w; x+=s*a.x; y+=s*a.y; z+=s*a.z;}

  // Don't let anyone tell you that linear interpolation makes no sense for quats
  void Lin(const sQuaternion &a,const sQuaternion &b,sF32 t);

  // Euclidean vector space operations
  sF32 Dot(const sVector &a) const                                  {return w*a.w + x*a.x + y*a.y + z*a.z;}
  sF32 Abs() const;
  void Unit();
  void UnitSafe();

  // Quaternion-specific operations
  void Mul(const sQuaternion &a,const sQuaternion &b);
  void MulR(const sQuaternion &a)                                   {Mul(*this,a);}
  void MulL(const sQuaternion &a);

  void ToMatrix(sMatrix &m) const;
  void FromMatrix(const sMatrix &m);
};

/****************************************************************************/

struct sAABox
{
  sVector Min;          // xmin,ymin,zmin
  sVector Max;          // xmax,ymax,zmax

  void Rotate34(const sMatrix &m)                                   {Rotate34(*this,m);}
  void Rotate34(const sAABox &a,const sMatrix &m);
  void Or(const sAABox &b);
  void And(const sAABox &b);
  sBool Intersects(const sAABox &b);
  sBool IntersectsSphere(const sVector &center,sF32 radius);
};

/****************************************************************************/

struct sCullPlane
{
  sVector Plane;
  sInt NVert;

  void LinearComb(const sVector &a,sF32 wa,const sVector &b,sF32 wb);
  void From3Points(const sVector &a,const sVector &b,const sVector &c);
  void CalcNVert();
  void Normalize();
  sF32 Distance(const sVector &x) const;
};

/****************************************************************************/

struct sFrustum
{
  sCullPlane Planes[6];   // typically left,right,bottom,top,near,far
  sInt NPlanes;           // to support degenerate frustums efficiently

  void FromViewProject(const sMatrix &viewProject,const sFRect &viewport);
  void ZFailVolume(const sVector &light,const sMatrix &view,sF32 znear,sF32 zoom[2],const sFRect &viewport);
  
  void EnlargeToInclude(const sVector &point);
  void Normalize();
};


/****************************************************************************/

sBool sCullBBox(const sAABox &box,const sCullPlane *planes,sInt planeCount);

/****************************************************************************/

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Static Strings                                                     ***/
/***                                                                      ***/
/****************************************************************************/

struct sStringDesc
{
  sChar *Buffer;
  sInt Size;
  sStringDesc(sChar *b,sInt s) : Buffer(b),Size(s) {}
  sStringDesc(const sStringDesc &s) : Buffer(s.Buffer),Size(s.Size) {}
  sStringDesc& operator=(const sStringDesc &s) { Buffer=s.Buffer; Size=s.Size; return*this;}
};

template<int size> struct sString
{
protected:
  sChar Buffer[size];
public:
  sString()                    { Buffer[0] = 0; }
  sString(const sChar *s)      { sCopyString(Buffer,s,size); }
  sString(const sString &s)    { sCopyString(Buffer,s.Buffer,size); }
  operator sChar*()            { return Buffer; }
  operator const sChar*() const { return Buffer; }
  operator sStringDesc()       { return sStringDesc(Buffer,size); }
  sInt operator==(const sChar *s) const { return sCmpString(Buffer,s)==0; }
  sInt operator==(const sString &s) const { return sCmpString(Buffer,s.Buffer)==0; }
  sInt operator!=(const sChar *s) const { return sCmpString(Buffer,s)!=0; }
  sInt operator!=(const sString &s) const { return sCmpString(Buffer,s.Buffer)!=0; }
  const sChar &operator[](sInt i) const { return Buffer[i]; }
  sChar &operator[](sInt i)    { return Buffer[i]; }
  sInt Count() const           { return sGetStringLen(Buffer); }
  sBool IsEmpty() const        { return Buffer[0]==0; }
  sInt Size() const            { return size; }
  void Write(sU32 *&data)      { sWriteString(data,Buffer); }
  void Read(sU32 *&data)       { sReadString(data,Buffer,size); }

  void Add(const sChar *a)     { sAppendString(Buffer,a,size); }
  void Add(const sChar *a,const sChar *b) { sAppendString(Buffer,a,b,size); }
};

void sSPrintF(const sStringDesc &,const sChar *,...);



/****************************************************************************/
/***                                                                      ***/
/***   Objects                                                            ***/
/***                                                                      ***/
/****************************************************************************/

#if !sMOBILE

// broker for garbage collection.
// 
// you MAY call the destructor if you are absolutly shure that's safe.
// but it's better to add the root of your data structure to the broker and
// then rely on garbage collection. if you fell that some deallocation should
// take place, just call sBroker->Free(). Now all roots will be tagged, and
// the sObject::Tag() will recursivly tag everything that is accessed by a 
// root. 
//
// sBroker::Free() may only be called from the mainloop, otherwise temporary
// objects might get "lost".
//
// never mess with the tag-value, it stores the index to the object table and
// other stuff!

class sBroker_
{
  sObject **Objects;
  sInt ObjectCount;
  sInt ObjectAlloc;
  sObject **Roots;
  sInt RootCount;
  sInt RootAlloc;
public:
  sBroker_();
  ~sBroker_();

  void NewObject(sObject *);      // automatically called  
  void DeleteObject(sObject *);   // automatically called  
  void AddRoot(sObject *);        // may be called only ONCE
  void RemRoot(sObject *);        // don't forget this!
  void Dump();                    // dump out all remaining objects
  void Stats(sInt &oc,sInt &rc) { oc = ObjectCount; rc = RootCount; }
  void Need(sObject *);           // should be called by sObject::Tag()
  void Free();                    // starts garbage collection
};

extern class sBroker_ *sBroker;

// the object overhead adds a virtual function table and the "TagVal", thats
// louse lightweight 8 bytes.
//
// since the order with which objects are deleted during garbage collection is
// random, you should NOT delete child objects in your destructor. only delete
// things that are not derived from sObject!

class sObject
{
public:
  sObject()                       { sBroker->NewObject(this); }
  virtual ~sObject()              { sBroker->DeleteObject(this); }
  virtual sU32 GetClass()         { return sCID_OBJECT; }
  virtual void Tag();
  virtual sBool Write(sU32 *&);
  virtual sBool Read(sU32 *&);
  virtual sBool Merge(sU32 *&);
  virtual void Clear();
  virtual void Copy(sObject *);
  sU32 TagVal;
  sU32 _Label;
};

#define sTAGVAL_USED    0x80000000
#define sTAGVAL_ROOT    0x40000000
#define sTAGVAL_INDEX   0x3fffffff

typedef sObject *(*sNewHandler)();

#endif

/****************************************************************************/
/***                                                                      ***/
/***   List                                                               ***/
/***                                                                      ***/
/****************************************************************************/

#define sFORALL(l,e) for(sInt _i=0;_i<(l).GetCount()?((e)=(l).GetFOR(_i)),1:0;_i++)
#define sFORLIST(l,e) for(sInt _i=0;_i<(l)->GetCount()?((e)=(l)->GetFOR(_i)),1:0;_i++)

#if !sMOBILE

// old fashoned list of broker-objects. use:
//
// in class: sList<xx> *mylist;
// in constructor: mylist = new sList<xx>;
// in Tag: sBroker->Need(mylist)

template <class Type> class sList : public sObject
{
  Type **Array;
  sInt Count;
  sInt Alloc;
public:
  sList(sInt i);
  sList();
  ~sList();
  sU32 GetClass()                 { return sCID_LIST; }
  void Tag();
  void Clear();
  void Copy(sObject *);
  Type *Get(sInt i)               { sVERIFY(i>=0 && i<Count); return Array[i]; }
  const Type *Get(sInt i) const   { sVERIFY(i>=0 && i<Count); return Array[i]; }
  Type *GetFOR(sInt i)            { return Get(i); }
  const Type *GetFOR(sInt i) const { return Get(i); }
  void Add(Type *o);
  void AddPos(Type *o,sInt i);
  void Rem(sInt i);
  void Rem(Type *o);
  void RemOrder(Type *o);
  void RemOrder(sInt i);
  void AddHead(Type *o);
  sInt GetCount()                 { return Count; }
  void Swap(sInt i);
  void Swap(sInt i,sInt j);
  sBool IsInside(Type *o);
};


// list of broker-objects, that is not an sObject itself.
// this allows it beeing a member variable instead of a pointer. use:
//
// in class: sList<xx> mylist;
// in Tag: mylist->Need();

template <class Type> class sObjList
{
  Type **Array;
  sInt Count;
  sInt Alloc;
public:
  sObjList();
  ~sObjList();
  void Clear()                    { Count = 0; }
  void Need()                     { Type *obj; sFORALL(*this,obj) sBroker->Need(obj); }   // internal compiler error ..
  Type *Get(sInt i)               { sVERIFY(i>=0 && i<Count); return Array[i]; }
  const Type *Get(sInt i) const   { sVERIFY(i>=0 && i<Count); return Array[i]; }
  Type *GetFOR(sInt i)            { return Get(i); }
  const Type *GetFOR(sInt i) const   { return Get(i); }
  void Add(Type *o);
  void AddPos(Type *o,sInt i);
  void Rem(sInt i);
  void Rem(Type *o);
  void RemOrder(Type *o);
  void RemOrder(sInt i);
  void AddHead(Type *o);
  sInt GetCount()                 { return Count; }
  void Swap(sInt i);
  void Swap(sInt i,sInt j);
  sBool IsInside(Type *o);
};

/****************************************************************************/

template <class Type> sList<Type>::sList(sInt i)                        
{ 
  Alloc=i;
  Count=0; 
  Array=new Type*[i]; 
}

template <class Type> sList<Type>::sList()                         
{ 
  /*sInt i;
  i = 16;
  Alloc=i;
  Count=0; 
  Array=new Type*[i];*/
  Alloc = Count = 0;
  Array = 0;
}

template <class Type> sList<Type>::~sList()                        
{ 
  delete[] Array; 
}

template <class Type> void sList<Type>::Tag()                      
{ 
  for(sInt i=0;i<Count;i++) 
    sBroker->Need(Array[i]); 
}

template <class Type> void sList<Type>::Clear()
{
  Count = 0;
}

template <class Type> void sList<Type>::Copy(sObject *o)
{
  sInt i;
  sList<Type> *ol;

  sVERIFY(o->GetClass()==sCID_LIST);
  ol = (sList<Type> *) o;

  for(i=0;i<ol->GetCount();i++)
    Add(ol->Get(i));
}

template <class Type> void sList<Type>::Add(Type *o)               
{ 
  if(Count>=Alloc) 
  { 
    sInt n=Alloc ? Alloc*2 : 16;
    Type **a=new Type*[n]; 
    sCopyMem(a,Array,Count*4); 
    delete[] Array;
    Array=a; 
    Alloc=n; 
  } 
  Array[Count++] = o; 
}

template <class Type> void sList<Type>::AddPos(Type *o,sInt pos)               
{ 
  sInt i;
  Add(o);

  for(i=Count-1;i>pos;i--)
    Array[i] = Array[i-1];
  Array[pos] = o;
}

template <class Type> void sList<Type>::Rem(sInt i)                
{ 
  sVERIFY(i>=0 && i<Count); 
  Array[i] = Array[--Count]; 
}

template <class Type> void sList<Type>::Rem(Type *o)               
{ 
  for(sInt i=0;i<Count;i++) 
  {
    if(Array[i]==o) 
    { 
      Array[i] = Array[--Count]; 
      return; 
    } 
  }
//  sFatal("could not remove %08x from list %08x\n",o,this);
}

template <class Type> void sList<Type>::RemOrder(sInt i)                
{ 
  sVERIFY(i>=0 && i<Count); 
  Count--;
  for(;i<Count;i++)
    Array[i] = Array[i+1];
}

template <class Type> void sList<Type>::RemOrder(Type *o)               
{ 
  for(sInt i=0;i<Count;i++) 
  {
    if(Array[i]==o) 
    {
      RemOrder(i);
      return; 
    } 
  }
  sFatal("could not remove %08x from list %08x\n",o,this);
}

template <class Type> void sList<Type>::AddHead(Type *o)               
{ 
  sInt i;
  Add(o);

  for(i=Count-1;i>=0;i--)
    Array[i+1] = Array[i];
  Array[0] = o;
}

template <class Type> void sList<Type>::Swap(sInt i)
{
  sVERIFY(i>=0 && i<Count-1)

  sSwap(Array[i],Array[i+1]);
}

template <class Type> void sList<Type>::Swap(sInt i,sInt j)
{
  sVERIFY(i>=0 && i<Count)
  sVERIFY(j>=0 && j<Count)

  sSwap(Array[i],Array[j]);
}

template <class Type> sBool sList<Type>::IsInside(Type *o)               
{ 
  for(sInt i=0;i<Count;i++) 
    if(Array[i]==o) 
      return sTRUE; 

  return sFALSE;
}

/****************************************************************************/


template <class Type> sObjList<Type>::sObjList()                         
{ 
  Alloc = Count = 0;
  Array = 0;
}

template <class Type> sObjList<Type>::~sObjList()                        
{ 
  delete[] Array; 
}

/*
template <class Type> sObjList<Type>::Need()
{
  Type *obj;
  sFORALL(*this,obj)
    sBroker->Need(obj);
}
    */

template <class Type> void sObjList<Type>::Add(Type *o)               
{ 
  if(Count>=Alloc) 
  { 
    sInt n=Alloc ? Alloc*2 : 16;
    Type **a=new Type*[n]; 
    sCopyMem(a,Array,Count*4); 
    delete[] Array;
    Array=a; 
    Alloc=n; 
  } 
  Array[Count++] = o; 
}

template <class Type> void sObjList<Type>::AddPos(Type *o,sInt pos)               
{ 
  sInt i;
  Add(o);

  for(i=Count-1;i>pos;i--)
    Array[i] = Array[i-1];
  Array[pos] = o;
}

template <class Type> void sObjList<Type>::Rem(sInt i)                
{ 
  sVERIFY(i>=0 && i<Count); 
  Array[i] = Array[--Count]; 
}

template <class Type> void sObjList<Type>::Rem(Type *o)               
{ 
  for(sInt i=0;i<Count;i++) 
  {
    if(Array[i]==o) 
    { 
      Array[i] = Array[--Count]; 
      return; 
    } 
  }
//  sFatal("could not remove %08x from list %08x\n",o,this);
}

template <class Type> void sObjList<Type>::RemOrder(sInt i)                
{ 
  sVERIFY(i>=0 && i<Count); 
  Count--;
  for(;i<Count;i++)
    Array[i] = Array[i+1];
}

template <class Type> void sObjList<Type>::RemOrder(Type *o)               
{ 
  for(sInt i=0;i<Count;i++) 
  {
    if(Array[i]==o) 
    {
      RemOrder(i);
      return; 
    } 
  }
  sFatal("could not remove %08x from list %08x\n",o,this);
}

template <class Type> void sObjList<Type>::AddHead(Type *o)               
{ 
  sInt i;
  Add(o);

  for(i=Count-1;i>=0;i--)
    Array[i+1] = Array[i];
  Array[0] = o;
}

template <class Type> void sObjList<Type>::Swap(sInt i)
{
  sVERIFY(i>=0 && i<Count-1)

  sSwap(Array[i],Array[i+1]);
}

template <class Type> void sObjList<Type>::Swap(sInt i,sInt j)
{
  sVERIFY(i>=0 && i<Count)
  sVERIFY(j>=0 && j<Count)

  sSwap(Array[i],Array[j]);
}

template <class Type> sBool sObjList<Type>::IsInside(Type *o)               
{ 
  for(sInt i=0;i<Count;i++) 
    if(Array[i]==o) 
      return sTRUE; 

  return sFALSE;
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Dynamic Arrays                                                     ***/
/***                                                                      ***/
/****************************************************************************/

struct sArrayBase
{
  sU8 *Array;
  sInt Count;
  sInt Alloc;
  sInt TypeSize;
};

void sArrayInit(sArrayBase *,sInt s,sInt c);
void sArraySetMax(sArrayBase *,sInt c);
void sArrayAtLeast(sArrayBase *,sInt c);
void sArrayCopy(sArrayBase *dest,const sArrayBase *src);
sU8 *sArrayAdd(sArrayBase *);

template <class Type> struct sArray
{
  Type *Array;
  sInt Count;
  sInt Alloc;
  sInt TypeSize;

  __forceinline void Init()                     { sArrayInit((sArrayBase *)this,sizeof(Type),0); }
  __forceinline void Init(sInt c)               { sArrayInit((sArrayBase *)this,sizeof(Type),c); }
  __forceinline void Exit()                     { if(Array){delete[] Array; Array=0;} }
  __forceinline void SetMax(sInt c)             { sArraySetMax((sArrayBase *)this,c); }
  __forceinline void AtLeast(sInt c)            { sArrayAtLeast((sArrayBase *)this,c); }
  __forceinline void Resize(sInt c)             { AtLeast(c); Count=c; }
  __forceinline Type Get(sInt i)                { return Array[i]; }
  __forceinline void Set(sInt i,const Type &o)  { Array[i] = o; }
  __forceinline const Type &operator[](sInt i) const { return Array[i]; }
  __forceinline Type &operator[](sInt i)        { return Array[i]; }
  __forceinline void Copy(const sArray<Type> &a){ sArrayCopy((sArrayBase *)this,(sArrayBase *)&a); }
  __forceinline Type *Add()                     { return (Type *)sArrayAdd((sArrayBase *)this); }
  __forceinline void Compact()                  { SetMax(Count); }

  __forceinline void Swap(sArray<Type> &a)      { sSwap(Array,a.Array); sSwap(Count,a.Count); sSwap(Alloc,a.Alloc); }
};


/****************************************************************************/
/***                                                                      ***/
/***   Bunch of Containers.                                               ***/
/***                                                                      ***/
/****************************************************************************/

template <class Type> class sAutoPtr        // automatically delete ptr when going out of scope
{
  Type *Ptr;
public:
  sAutoPtr()                                { Ptr = 0; }
  sAutoPtr(Type *x) : Ptr(x)                {}
  ~sAutoPtr()                               { delete Ptr }
  sAutoPtr(const sAutoPtr &x) : Ptr(x)      { x=0; }
  sAutoPtr & operator= (const sAutoPtr &x)  { delete Ptr; Ptr=x; x=0; }

  Type & operator->()                       { return *Ptr; }
};

template <class Type> class sArray2 : public sArray<Type> // add support for sFORALL() and constructor/destructor to sArray 
{
public:
  sArray2()                                 { Init(); }
  ~sArray2()                                { Exit(); }
  sArray2(const sArray2 &x)                 { Init(); Copy(x); }
  sArray2 & operator= (const sArray2 &x)    { Copy(x); }
  Type *Add()                               { return sArray<Type>::Add(); }
  void Add(const Type &x)                   { *sArray<Type>::Add()=x; }
  sInt GetCount() const                     { return Count; }
  Type *GetFOR(sInt i)                      { return &Array[i]; }
  const Type *GetFOR(sInt i) const          { return &Array[i]; }
  void Swap(sInt i,sInt j)                  { sSwap(Array[i],Array[j]); }
  void Clear()                              { Count = 0; }
};

template <class Type> class sAutoArray       // Array of pointers, deleted when array gets deleted
{
  sArray<Type*> Array;
public:

  sAutoArray()                               { Array.Init(); }
  ~sAutoArray()                              { Clear(); Array.Exit(); }
  sAutoArray(const sAutoArray &x)             { Array.Init(); Array.Copy(x.Array); }
//  sAutoArray & operator= (const sAutoArray &x) { Array.Copy(x.Array); return *this; }

  Type *Get(sInt i)                         { sVERIFY(i>=0 && i<Array.Count); return Array[i]; }
  const Type *Get(sInt i) const             { sVERIFY(i>=0 && i<Array.Count); return Array[i]; }
  Type *GetFOR(sInt i)                      { return Get(i); }
  const Type *GetFOR(sInt i) const          { return Get(i); }
  void Add(Type *o)                         { *Array.Add()=o; }
  sInt GetCount() const                     { return Array.Count; }
  Type *Rem(sInt i)                         { Type *x=Array[i]; Array[i] = Array[--Array.Count]; return x; }
  void Clear()                              { Type*e; sFORLIST(this,e) delete e; Array.Count=0; }
  void Swap(sInt i,sInt j)                  { sSwap(Array[i],Array[j]); }
/*
  void AddPos(Type *o,sInt i);
  void Rem(Type *o);
  void RemOrder(Type *o);
  void RemOrder(sInt i);
  void AddHead(Type *o);
  void Swap(sInt i);
  void Swap(sInt i,sInt j);
  sBool IsInside(Type *o);
*/
};

template <class Type> class sPtrArray        // Array of pointers, NOT deleted when array gets deleted
{
  sArray<Type*> Array;
public:

  sPtrArray()                               { Array.Init(); }
  ~sPtrArray()                              { Clear(); Array.Exit(); }
  sPtrArray(const sPtrArray &x)             { Array.Init(); Array.Copy(x.Array); }
  sPtrArray & operator= (const sPtrArray &x) { Array.Copy(x.Array); return *this; }

  Type *Get(sInt i)                         { sVERIFY(i>=0 && i<Array.Count); return Array[i]; }
  const Type *Get(sInt i) const             { sVERIFY(i>=0 && i<Array.Count); return Array[i]; }
  Type *GetFOR(sInt i)                      { return Get(i); }
  const Type *GetFOR(sInt i) const          { return Get(i); }
  void Add(Type *o)                         { *Array.Add()=o; }
  sInt GetCount() const                     { return Array.Count; }
  Type *Rem(sInt i)                         { Type *x=Array[i]; Array[i] = Array[--Array.Count]; return x; }
  void Clear()                              { Array.Count=0; }
  void Swap(sInt i,sInt j)                  { sSwap(Array[i],Array[j]); }
/*
  void AddPos(Type *o,sInt i);
  void Rem(Type *o);
  void RemOrder(Type *o);
  void RemOrder(sInt i);
  void AddHead(Type *o);
  void Swap(sInt i);
  void Swap(sInt i,sInt j);
  sBool IsInside(Type *o);
*/
};


/****************************************************************************/
/***                                                                      ***/
/***   Bitmap                                                             ***/
/***                                                                      ***/
/****************************************************************************/

#if !sMOBILE

class sBitmap : public sObject
{
public:
  sBitmap();
  ~sBitmap();
  sU32 GetClass() { return sCID_BITMAP; }

  sU32 *Data;
  sInt XSize;
  sInt YSize;

  void Init(sInt xs,sInt ys);
  void Copy(sObject *);
  void Clear();
  sBool Write(sU32 *&);
  sBool Read(sU32 *&);
};

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Text (no, this is not a string class, this is a wrapper)           ***/
/***                                                                      ***/
/****************************************************************************/

#if !sINTRO
#if !sMOBILE

class sText : public sObject
{
public:
  sText(sInt size=256);
  sText(const sChar *);
  ~sText();
  sU32 GetClass() { return sCID_TEXT; }

  sChar *Text;
  sInt Alloc;
  sInt Used;                            // this counter may be wrong!

  void Init(const sChar *,sInt len=-1);
  sBool Realloc(sInt size);             // reallocate to exactly this size
  sBool Grow(sInt size=0);              // reallocate to a large chunk, at least of this size
  void Clear();
  void Copy(sObject *);
  sBool Write(sU32 *&);
  sBool Read(sU32 *&);

  void FixUsed();
  void Print(const sChar *);                 // these require correct Used-counter!
  void PrintF(const sChar *format,...);
  void PrintArg(const sChar *format,const sChar **fp);
};

#endif
#endif

/****************************************************************************/
/***                                                                      ***/
/***   sPerfmon Support (use it even where util.hpp is not included)      ***/
/***                                                                      ***/
/****************************************************************************/

#if !sMOBILE

#if sPROFILE && sLINK_UTIL
#define sZONE(x) sPerfMonZone __usezone_##x(__zone_##x);
#define sMAKEZONE(x,name,col) sPerfMonToken __zone_##x={name,col,-1};
#define sREGZONE(x)  sPerfMon->Register(__zone_##x); 
#else
#define sZONE(x)
#define sMAKEZONE(x,name,col)
#define sREGZONE(x)
#endif

#endif

/****************************************************************************/
/***                                                                      ***/
/***   MusicPlayers                                                       ***/
/***                                                                      ***/
/****************************************************************************/

#if !sMOBILE

class sMusicPlayer : public sObject
{
private:
  sS16 *RewindBuffer;               // remember all rendered samples
  sInt RewindSize;                  // max size in samples
  sInt RewindPos;                   // up to this sample the buffer has been rendered
protected:
  sU8 *Stream;                      // the loaded data
  sInt StreamSize;
  sBool StreamDelete;

public:
  sInt Status;                      // 0=error, 1=ready, 2=pause, 3=playing
  sInt PlayPos;                     // current position for continuing rendering

  sMusicPlayer();
  virtual ~sMusicPlayer();
  sBool Load(sChar *name);          // load from file, memory is deleted automatically
  sBool Load(sU8 *data,sInt size,sBool ownMem=sFALSE);  // load from buffer, memory is deleted automatically if ownMem is set
  sBool LoadCache(sChar *name);     // loads cached render buffer for "name"
  sBool LoadAndCache(sChar *name);  // load with caching
  sBool LoadAndCache(sChar *name,sU8 *data,sInt size,sBool ownMem=sFALSE); // load with caching from mem
  void AllocRewind(sInt bytes);     // allocate a rewindbuffer
  sBool Start(sInt songnr);         // initialize and start playing
  void Stop();                      // stop playing
  sBool Handler(sS16 *buffer,sInt samples,sInt Volume=256); // return sFALSE if all following calls to handler will just return silence

  virtual sBool Init(sInt songnr)=0;
  virtual sInt Render(sS16 *buffer,sInt samples)=0;   // return number of samples actually rendered, 0 for eof
  virtual sInt GetTuneLength() = 0; // in samples
};

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Matrix Stack                                                       ***/
/***                                                                      ***/
/****************************************************************************/

#if !sMOBILE

class sMatrixStack
{
  sArray<sMatrix> Stack;

public:
  void Init();
  void Exit();

  const sMatrix &Top() const          { return Stack[Stack.Count-1]; }
  sMatrix &Top()                      { return Stack[Stack.Count-1]; }

  void Push(const sMatrix &mtx)       { Stack.AtLeast(Stack.Count+1); Stack[Stack.Count++] = mtx; }
  void PushIdentity()                 { Stack.AtLeast(Stack.Count+1); Stack[Stack.Count++].Init(); }
  void PushMul(const sMatrix &mtx)    { Stack.AtLeast(Stack.Count+1); Stack[Stack.Count].MulA(mtx,Stack[Stack.Count-1]); Stack.Count++; }
  void PushMul4(const sMatrix &mtx)   { Stack.AtLeast(Stack.Count+1); Stack[Stack.Count].Mul4(mtx,Stack[Stack.Count-1]); Stack.Count++; }
  void Dup()                          { Stack.AtLeast(Stack.Count+1); Stack[Stack.Count] = Stack[Stack.Count-1]; Stack.Count++; }
  void Pop()                          { if(Stack.Count>1) Stack.Count--; }
  void PopAll()                       { Stack.Count = 1; }
};

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Tick Timer                                                         ***/
/***                                                                      ***/
/****************************************************************************/

#if !sMOBILE
class sTickTimer
{
  sU64 Total;
  sU64 LastStart;

  static sU64 Tick()                  { __asm { rdtsc } }

public:
  sTickTimer()                        { Reset(); }

  void Reset()                        { Total = LastStart = 0; }
  void Start()                        { LastStart = Tick(); }
  void Stop()                         { Total += Tick() - LastStart; }

  sU64 GetTotal() const               { return Total; }
};
#endif

/****************************************************************************/

#endif

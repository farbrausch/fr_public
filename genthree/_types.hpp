// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __TYPES_HPP__
#define __TYPES_HPP__

/****************************************************************************/
/***                                                                      ***/
/***   Compiler                                                           ***/
/***                                                                      ***/
/****************************************************************************/

#include "_config.hpp"              // read system config

/****************************************************************************/

#pragma warning (disable : 4244)    // 'double' to 'float'
//#pragma warning (disable : 4018)  // signed/unsigned mismatch
#pragma warning (disable : 4251)    // dll-interfaces needed for baseclass (template problem)

#pragma inline_recursion (on)       // get full inline power
#pragma inline_depth (255)

/****************************************************************************/

#if _DEBUG                          // switch on debugs in debug build
#undef sDEBUG
#define sDEBUG 1
#define sRELEASE 0
#else
#define sRELEASE 1
#endif

#define sDLLSYSTEM                   // we have no dll support right now

#ifndef sUSE_SHADERS
#pragma message("please set sUSE_SHADERS!")
#define sUSE_SHADERS 0
#endif

/****************************************************************************/

#if _DEBUG                          // fix memory management
#if !sINTRO
#define _MFC_OVERRIDES_NEW
void * sDLLSYSTEM __cdecl operator new(unsigned int);
void * sDLLSYSTEM __cdecl operator new(unsigned int,const char *,int);
inline void __cdecl operator delete(void *p, const char *, int s) { ::operator delete(p); }
#define new new(__FILE__,__LINE__)
#endif
#endif

#define sNORETURN __declspec(noreturn)  // use this for dead end funtions

/****************************************************************************/
/***                                                                      ***/
/***   Init/Exit                                                          ***/
/***                                                                      ***/
/****************************************************************************/

void sDLLSYSTEM sInitTypes();
void sDLLSYSTEM sExitTypes();

/****************************************************************************/
/***                                                                      ***/
/***   Forwards                                                           ***/
/***                                                                      ***/
/****************************************************************************/

struct sRect;
struct sFRect;
struct sColor;

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
// 0x214f4344   sMAGIC
// 0x00000001   version
// 0x00000000   flags
// 0x01000001   sCID_MINIED
// ....

/****************************************************************************/
/***                                                                      ***/
/***   Base Types and Funcs                                               ***/
/***                                                                      ***/
/****************************************************************************/

typedef unsigned char             sU8;
typedef signed char               sS8;
typedef unsigned short            sU16;
typedef short                     sS16;
typedef unsigned int              sU32;
typedef int                       sS32;
typedef float                     sF32;
typedef unsigned __int64          sU64;
typedef signed __int64            sS64;
typedef double                    sF64;
typedef int                       sInt;
typedef char                      sChar;
typedef signed char               sBool;
typedef void*                     sPtr;

/****************************************************************************/

#define sVARARG(p,i) (((sInt *)(p))[(i)+1])
#define sVARARGF(p,i) (*(sF64*)&(((sInt *)(p))[(i)+1]))
#define sOFFSET(t,m) ((sInt)(&((t*)0)->m))
#define sMAKECID(u,s,c) (((u)<<16)|((s)<<8)|(c))
#define sMAKE2(a,b) (((b)<<16)|(a))
#define sMAKE4(a,b,c,d) (((d)<<24)|((c)<<16)|((b)<<8)|(a))

/****************************************************************************/

#define sTRUE   (!0)
#define sFALSE  0
#define sPI     3.1415926535897932384626433832795
#define sPI2    6.28318530717958647692528676655901
#define sPIF    3.1415926535897932384626433832795f
#define sPI2F   6.28318530717958647692528676655901f

/****************************************************************************/

template <class Type> __forceinline Type sMin(Type a,Type b)            {return (a<b) ? a : b;}
template <class Type> __forceinline Type sMax(Type a,Type b)            {return (a>b) ? a : b;}
template <class Type> __forceinline Type sSign(Type a)                  {return (a==0) ? 0 : (a>0) ? 1 : -1;}
template <class Type> __forceinline Type sRange(Type a,Type b,Type c)   {return (a>=b) ? b : (a<=c) ? c : a;}
template <class Type> __forceinline void sSwap(Type &a,Type &b)         {Type s; s=a; a=b; b=s;}
template <class Type> __forceinline Type sAlign(Type &a,sInt b)         {return (Type)((((sInt)a)+b-1)&(~(b-1)));}
__forceinline sF32 sFade(sF32 a,sF32 b,sF32 fade)												{return a+(b-a)*fade;}

sU32 sDLLSYSTEM sGetRnd();
sU32 sDLLSYSTEM sGetRnd(sU32 max);
sF32 sDLLSYSTEM sFGetRnd();
sF32 sDLLSYSTEM sFGetRnd(sF32 max);
void sDLLSYSTEM sSetRndSeed(sInt seed);
sInt sDLLSYSTEM sMakePower2(sInt val);
sInt sDLLSYSTEM sGetPower2(sInt val);
sU32 sDec3(sF32 x,sF32 y,sF32 z);
void sHermite(sF32 *d,sF32 *p0,sF32 *p1,sF32 *p2,sF32 *p3,sInt count,sF32 fade,sF32 t,sF32 c,sF32 b);
/*
sInt sRangeInt(sInt a,sInt b,sInt c);
sF32 sRangeF32(sF32 a,sF32 b,sF32 c);
*/
/****************************************************************************/
/***                                                                      ***/
/***   Intrinsics                                                         ***/
/***                                                                      ***/
/****************************************************************************/


typedef unsigned int size_t;
extern "C"
{
  int __cdecl abs(int);

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

  void * __cdecl memset( void *dest, int c, size_t count );
  void * __cdecl memcpy( void *dest, const void *src, size_t count );
  int __cdecl memcmp( const void *buf1, const void *buf2, size_t count );
  size_t __cdecl strlen( const char *string );
}

#pragma intrinsic (abs)                                       // int intrinsic
#pragma intrinsic (memset,memcpy,memcmp,strlen)               // memory intrinsic
#pragma intrinsic (atan,atan2,cos,exp,log,log10,sin,sqrt,tan,fabs) // true intrinsic
#pragma intrinsic (acos,asin,cosh,fmod,pow,sinh,tanh)         // fake intrinsic

__forceinline sInt sAbs(sInt i)           { return abs(i); }
__forceinline void sSetMem(sPtr dd,sInt s,sInt c)        { memset(dd,s,c); }
__forceinline void sCopyMem(sPtr dd,sPtr ss,sInt c)      { memcpy(dd,ss,c); }
__forceinline sInt sCmpMem(sPtr dd,sPtr ss,sInt c)       { return (sInt)memcmp(dd,ss,c); }
__forceinline sInt sGetStringLen(sChar *s)               { return (sInt)strlen(s); }


__forceinline sF64 sFATan(sF64 f)         { return atan(f); }
__forceinline sF64 sFATan2(sF64 a,sF64 b) { return atan2(a,b); }
__forceinline sF64 sFCos(sF64 f)          { return cos(f); }
__forceinline sF64 sFAbs(sF64 f)          { return fabs(f); }
__forceinline sF64 sFLog(sF64 f)          { return log(f); }
__forceinline sF64 sFLog10(sF64 f)        { return log10(f); }
__forceinline sF64 sFSin(sF64 f)          { return sin(f); }
__forceinline sF64 sFSqrt(sF64 f)         { return sqrt(f); }

__forceinline sF64 sFACos(sF64 f)         { return acos(f); }
__forceinline sF64 sFASin(sF64 f)         { return asin(f); }
__forceinline sF64 sFCosH(sF64 f)         { return cosh(f); }
__forceinline sF64 sFSinH(sF64 f)         { return sinh(f); }
__forceinline sF64 sFTanH(sF64 f)         { return tanh(f); }

__forceinline sF64 sFInvSqrt(sF64 f)      { return 1.0/sqrt(f); }

#if !sINTRO 
__forceinline sF64 sFMod(sF64 a,sF64 b)   { return fmod(a,b); }
__forceinline sF64 sFExp(sF64 f)          { return exp(f); }
__forceinline sF64 sFPow(sF64 a,sF64 b)   { return pow(a,b); }
#endif

#if sINTRO
sF64 sFPow(sF64 a,sF64 b);
sF64 sFMod(sF64 a,sF64 b);
sF64 sFExp(sF64 f);
#endif

/****************************************************************************/
/***                                                                      ***/
/***   asm                                                                ***/
/***                                                                      ***/
/****************************************************************************/

#pragma warning (disable : 4035) 

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

__forceinline sInt sMulDiv(sInt var_a,sInt var_b,sInt var_c)
{
  __asm
  {
    mov eax,var_a
    mov ebx,var_b
    imul ebx
    mov ecx,var_c
    idiv ecx
  }
}

__forceinline sInt sMulShift(sInt var_a,sInt var_b)
{
  __asm
  {
    mov eax, var_a
    imul var_b
    shrd eax, edx, 16
  }
}

__forceinline sInt sDivShift(sInt var_a,sInt var_b)
{
  __asm
  {
    mov eax,var_a
    mov ebx,var_b
    mov edx,eax
    shl eax,16
    sar edx,16
    idiv ebx
  }
}

#pragma warning (default : 4035) 


/****************************************************************************/
/***                                                                      ***/
/***   Memory and String                                                  ***/
/***                                                                      ***/
/****************************************************************************/


__forceinline void sSetMem1(sU8  *d,sInt s,sInt c)    {while(c--)*d++=s;}
__forceinline void sSetMem2(sU16 *d,sU16 s,sInt c)    {while(c--)*d++=s;}
__forceinline void sSetMem4(sU32 *d,sU32 s,sInt c)    {while(c--)*d++=s;}
__forceinline void sSetMem8(sU64 *d,sU64 s,sInt c)    {while(c--)*d++=s;}
void sCopyMem4(sU32 *d,sU32 *s,sInt c);
void sCopyMem8(sU64 *d,sU64 *s,sInt c);


#if !sINTRO

sDLLSYSTEM sChar *sDupString(sChar *s,sInt minsize=8);
sDLLSYSTEM void sAppendString(sChar *d,sChar *s,sInt size);
sDLLSYSTEM void sParentString(sChar *path);
sDLLSYSTEM sInt sCmpStringI(sChar *a,sChar *b);
sDLLSYSTEM sChar *sFindString(sChar *s,sChar *f);
sDLLSYSTEM sChar *sFindStringI(sChar *s,sChar *f);

sDLLSYSTEM void __cdecl sSPrintF(sChar *buffer,sInt size,sChar *format,...); 

sDLLSYSTEM sInt sScanInt(sChar *&scan);
sDLLSYSTEM sInt sScanHex(sChar *&scan);
sDLLSYSTEM sF32 sScanFloat(sChar *&scan);
sDLLSYSTEM sBool sScanString(sChar *&scan,sChar *buffer,sInt size);
sDLLSYSTEM sBool sScanName(sChar *&scan,sChar *buffer,sInt size);
sDLLSYSTEM void sScanSpace(sChar *&scan);
sDLLSYSTEM sBool sScanCycle(sChar *cycle,sInt index,sInt &start,sInt &len);

#endif

sDLLSYSTEM void sCopyString(sChar *d,sChar *s,sInt size);
sDLLSYSTEM sInt sCmpString(sChar *a,sChar *b);

#if !sINTRO || !sRELEASE
sDLLSYSTEM void sFormatString(sChar *buffer,sInt size,sChar *format,sChar **fp);
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Perlin Util                                                         ***/
/***                                                                      ***/
/****************************************************************************/

extern sVector sPerlinGradient3D[16];
extern sF32 sPerlinRandom[256][2];
extern sU8 sPerlinPermute[512];

extern void sInitPerlin();

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

sDLLSYSTEM sNORETURN void sVerifyFalse(sChar *file,sInt line);
#if !sINTRO || !sRELEASE
sDLLSYSTEM void sDPrint(sChar *text);
sDLLSYSTEM void __cdecl sDPrintF(sChar *format,...);
sDLLSYSTEM sNORETURN void __cdecl sFatal(sChar *format,...);
#endif

#if sDEBUG
#define sVERIFY(x) {if(!(x))sVerifyFalse(__FILE__,__LINE__);}
#define sVERIFYFALSE {sVerifyFalse(__FILE__,__LINE__);}
#else
#if !sINTRO
#define sVERIFY(x) ;
#define sVERIFYFALSE ;
#else
#define sVERIFY(x) __assume(x);
#define sVERIFYFALSE __assume(0);
#endif
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

struct sF322
{
  sF32 x,y;
  void Init(sF32 X,sF32 Y) { x=X;y=Y; }
};

struct sF323
{
  sF32 x,y,z;
  void Init(sF32 X,sF32 Y,sF32 Z) { x=X; y=Y; z=Z; }
};

struct sF324
{
  sF32 x,y,z,w;
  void Init(sF32 X,sF32 Y,sF32 Z,sF32 W) { x=X;y=Y;z=Z;w=W; }
};

/****************************************************************************/

struct sDLLSYSTEM sRect
{
  sInt x0,y0,x1,y1;
  __forceinline void Init() {x0=y0=x1=y1=0;}
  __forceinline void Init(sInt X0,sInt Y0,sInt X1,sInt Y1) {x0=X0;y0=Y0;x1=X1;y1=Y1;}
  __forceinline sInt XSize() const {return x1-x0;}
  __forceinline sInt YSize() const {return y1-y0;}
    void Init(struct sFRect &r);
#if !sINTRO
  sBool Hit(sInt x,sInt y);
  sBool Hit(sRect &);
  sBool Inside(sRect &);
  void And(sRect &r);
  void Or(sRect &r);
  void Sort();
  void Extend(sInt i);
#endif
};

/****************************************************************************/

struct sDLLSYSTEM sFRect
{
  sF32 x0,y0,x1,y1;
  __forceinline void Init() {x0=y0=x1=y1=0;}
  __forceinline void Init(sF32 X0,sF32 Y0,sF32 X1,sF32 Y1) {x0=X0;y0=Y0;x1=X1;y1=Y1;}
  __forceinline sF32 XSize() const {return x1-x0;}
  __forceinline sF32 YSize() const {return y1-y0;}
  void Init(struct sRect &r);
#if !sINTRO
  sBool Hit(sF32 x,sF32 y);
  sBool Hit(sFRect &);
  void And(sFRect &r);
  void Or(sFRect &r);
  void Sort();
  void Extend(sF32 i);
#endif
};

/****************************************************************************/

struct sDLLSYSTEM sColor
{
  union
  {
    sU32 Color;
    struct
    {
      sU8 b,g,r,a;
    };
    sU8 bgra[4];
  };
  __forceinline sColor(sU32 c) { Color = c; }
  __forceinline sColor() {}
  __forceinline operator unsigned long () { return Color; }
  __forceinline void Init(sU8 R,sU8 G,sU8 B) { Color = B|(G<<8)|(R<<16)|0xff000000; };
  __forceinline void Init(sU8 R,sU8 G,sU8 B,sU32 A) { Color = B|(G<<8)|(R<<16)|(A<<24); };
  void Fade(sF32 fade,sColor c0,sColor c1);
  void Fade(sInt fade,sColor c0,sColor c1);
};

#if !sINTRO

/****************************************************************************/

struct sDLLSYSTEM sDate
{
  sU16 Day;                       // 0..364 / 365 day of year
  sU16 Year;                      // 1..65535 year AD
  sU16 Minute;                    // 0..1439 minute of day
  sU16 Millisecond;               // 0..59999 millisecond of minute

  void RoundDay();                // clear minute and millisecond to zero
  void Sub(sDate &a,sDate &b);    // get difference of two dates
  void Add(sDate &a,sDate &b);    // add difference b to date a
  sInt DaysOfYear();              // gets days per year. 
  void SetMonth(sInt m,sInt d);   // m=0..11, d=0..27/30  sets by month and day of month
  void GetMonth(sInt &m,sInt &d); // m=0..11, d=0..27/30  gets month and day of month
  void PrintDate(sChar *);        // 2002-dec-31
  void PrintTime(sChar *);        // 23:59
  void ReadDate(sChar *);         // 2002-dec-31
  void ReadTime(sChar *);         // 23:59
};

/****************************************************************************/

struct sPlacer                    // places gui elements in a grid
{
  sInt XPos;                      // rect to place the elements in
  sInt YPos;
  sInt XSize;
  sInt YSize;
  sInt XDiv;                      // number of divisions in x-axxis
  sInt YDiv;                      // number of divisions in y-axxis
  void Init(sRect &r,sInt xdiv,sInt ydiv);
  void Div(sRect &r,sInt x,sInt y,sInt xs=1,sInt ys=1);
};

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Vector and Matrix                                                  ***/
/***                                                                      ***/
/****************************************************************************/

struct sDLLSYSTEM sVector
{
  sF32 x,y,z,w;

  __forceinline void Init()                                         {x=0; y=0; z=0; w=0;}
  __forceinline void Init(sInt3 &v)                                 {x=v.x/65536.0f; y=v.y/65536.0f; z=v.z/65536.0f; w=0.0f;}
	__forceinline void Init(sInt4 &v)																	{x=v.x/65536.0f; y=v.y/65536.0f; z=v.z/65536.0f; w=v.w/65536.0f;}
  __forceinline void Init(sF32 xx,sF32 yy,sF32 zz)                  {x=xx; y=yy; z=zz; w=0;}
  __forceinline void Init(sF32 xx,sF32 yy,sF32 zz,sF32 ww)          {x=xx; y=yy; z=zz; w=ww;}
  __forceinline void InitColorWithBug(sU32 col)                     {x=((col>>0)&0xff)/255.0f;y=((col>>8)&0xff)/255.0f;z=((col>>16)&0xff)/255.0f;w=((col>>24)&0xff)/255.0f;}
  __forceinline void InitColor(sU32 col)                            {x=((col>>16)&0xff)/255.0f;y=((col>>8)&0xff)/255.0f;z=((col>>0)&0xff)/255.0f;w=((col>>24)&0xff)/255.0f;}
  __forceinline void InitColorI(sU32 col)                           {x=((col>>0)&0xff)/255.0f;y=((col>>8)&0xff)/255.0f;z=((col>>16)&0xff)/255.0f;w=((col>>24)&0xff)/255.0f;}
  __forceinline sU32 GetColor()                                     {return sRange<sInt>(x*255,255,0)|(sRange<sInt>(y*255,255,0)<<8)|(sRange<sInt>(z*255,255,0)<<16)|(sRange<sInt>(w*255,255,0)<<24);}
  void InitRnd();

  void Write(sU32 *&p);
  void Write3(sU32 *&p);
  void Write3U(sU32 *&p);
  void Read(sU32 *&p);
  void Read3(sU32 *&p);
  void Read3U(sU32 *&p);

  __forceinline void Init3()                                        {x=0; y=0; z=0;}
  __forceinline void Init3(sF32 xx,sF32 yy,sF32 zz)                 {x=xx; y=yy; z=zz;}
  __forceinline void Add3(const sVector &a,const sVector &b)        {x=a.x+b.x; y=a.y+b.y; z=a.z+b.z;}
  __forceinline void Sub3(const sVector &a,const sVector &b)        {x=a.x-b.x; y=a.y-b.y; z=a.z-b.z;}
  __forceinline void Mul3(const sVector &a,const sVector &b)        {x=a.x*b.x; y=a.y*b.y; z=a.z*b.z;}
  __forceinline void Neg3(const sVector &a)                         {x=-a.x; y=-a.y; z=-a.z;}
  __forceinline void Add3(const sVector &a)                         {x+=a.x; y+=a.y; z+=a.z;}
  __forceinline void Sub3(const sVector &a)                         {x-=a.x; y-=a.y; z-=a.z;}
  __forceinline void Mul3(const sVector &a)                         {x*=a.x; y*=a.y; z*=a.z;}
  __forceinline sF32 Dot3(const sVector &a) const                   {return x*a.x+y*a.y+z*a.z;}
  __forceinline void Neg3()                                         {x=-x; y=-y; z=-z;}
  __forceinline void AddMul3(const sVector &a,const sVector &b)     {x+=a.x*b.x; y+=a.y*b.y; z+=a.z*b.z;}
  __forceinline void Scale3(sF32 s)                                 {x*=s; y*=s; z*=s;}
  __forceinline void Scale3(const sVector &a,sF32 s)                {x=a.x*s; y=a.y*s; z=a.z*s;}
  __forceinline void AddScale3(const sVector &a,sF32 s)             {x+=a.x*s; y+=a.y*s; z+=a.z*s;}
	void               Lin3(const sVector &a,const sVector &b,sF32 t) ;
                sF32 Abs3() const                                   ;
                void Unit3()                                        ;
                void UnitSafe3()                                    ;
                void Rotate3(const sMatrix &m,const sVector &v)     ;
                void Rotate3(const sMatrix &m);                     ;
                void RotateT3(const sMatrix &m,const sVector &v)    ;
                void RotateT3(const sMatrix &m);                    ;
                void Cross3(const sVector &a,const sVector &b)      ;

  __forceinline void Init4()                                        {x=0; y=0; z=0; w=0;}
  __forceinline void Init4(sF32 xx,sF32 yy,sF32 zz,sF32 ww)         {x=xx; y=yy; z=zz; w=ww;}
  __forceinline void Add4(const sVector &a,const sVector &b)        {x=a.x+b.x; y=a.y+b.y; z=a.z+b.z; w=a.w+b.w;}
  __forceinline void Sub4(const sVector &a,const sVector &b)        {x=a.x-b.x; y=a.y-b.y; z=a.z-b.z; w=a.w-b.w;}
  __forceinline void Mul4(const sVector &a,const sVector &b)        {x=a.x*b.x; y=a.y*b.y; z=a.z*b.z; w=a.w*b.w;}
  __forceinline void Neg4(const sVector &a)                         {x=-a.x; y=-a.y; z=-a.z; w=-a.w;}
  __forceinline void Add4(const sVector &a)                         {x+=a.x; y+=a.y; z+=a.z; w+=a.w;}
  __forceinline void Sub4(const sVector &a)                         {x-=a.x; y-=a.y; z-=a.z; w-=a.w;}
  __forceinline void Mul4(const sVector &a)                         {x*=a.x; y*=a.y; z*=a.z; w*=a.w;}
  __forceinline void Neg4()                                         {x=-x; y=-y; z=-z; w=-w;}
  __forceinline void AddMul4(const sVector &a,const sVector &b)     {x+=a.x*b.x; y+=a.y*b.y; z+=a.z*b.z; w+=a.w*b.w;}
  __forceinline sF32 Dot4(const sVector &a) const                   {return x*a.x + y*a.y + z*a.z * w*a.w;}
  __forceinline void Scale4(sF32 s)                                 {x*=s; y*=s; z*=s; w*=s;}
  __forceinline void Scale4(const sVector &a,sF32 s)                {x=a.x*s; y=a.y*s; z=a.z*s; w=a.w*s;}
  __forceinline void AddScale4(const sVector &a,sF32 s)             {x+=a.x*s; y+=a.y*s; z+=a.z*s; w+=a.w*s;}
	void               Lin4(const sVector &a,const sVector &b,sF32 t)	;
                sF32 Abs4() const                                   ;
                void Unit4()                                        ;
                void UnitSafe4()                                    ;
                void Rotate4(const sMatrix &m,const sVector &v)     ;
                void Rotate4(const sMatrix &m);                     ;
                void Rotate34(const sMatrix &m,const sVector &v)    ;
                void Rotate34(const sMatrix &m);                    ;
                void RotateT4(const sMatrix &m,const sVector &v)    ;
                void RotateT4(const sMatrix &m);                    ;

};

/****************************************************************************/

struct sDLLSYSTEM sMatrix
{
  sVector i,j,k,l;

  void Init();
  void InitRot(const sVector &);
  void InitDir(const sVector &);
  void InitEuler(sF32 a,sF32 b,sF32 c);
  void InitEulerPI2(sF32 *a);
  void InitSRT(sF32 *);
  void InitSRTInv(sF32 *);
	
#if !sINTRO
	void FindEuler(sF32 &a,sF32 &b,sF32 &c);
#endif

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

  void TransR();

  void Mul3(const sMatrix &a,const sMatrix &b);
  void Mul4(const sMatrix &a,const sMatrix &b);
  void MulR(const sMatrix &a,const sMatrix &b);
  void Mul3(const sMatrix &b)                                       {sMatrix a;a=*this;Mul3(a,b);}
  void Mul4(const sMatrix &b)                                       {sMatrix a;a=*this;Mul4(a,b);}
  void MulR(const sMatrix &b)                                       {sMatrix a;a=*this;MulR(a,b);}
};
/*
__forceinline sF32 sVector::Abs3() const                                   {return sFSqrt(Dot3(*this));}
__forceinline void sVector::Unit3()                                        {Scale3(sFInvSqrt(Dot3(*this)));}
__forceinline void sVector::UnitSafe3()                                    {sF32 e=Dot3(*this); if(e>0.00001) Scale3(sFInvSqrt(e)); else Init3(1,0,0);}

__forceinline sF32 sVector::Abs4() const                                   {return Dot4(*this);}
__forceinline void sVector::Unit4()                                        {Scale4(sFInvSqrt(Dot4(*this)));}
__forceinline void sVector::UnitSafe4()                                    {sF32 e=Dot4(*this); if(e>0.00001) Scale4(sFInvSqrt(e)); else Init4(1,0,0,0);}
*/
/****************************************************************************/
/***                                                                      ***/
/***   Objects                                                            ***/
/***                                                                      ***/
/****************************************************************************/

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
#if !sINTRO
  sObject **Roots;
  sInt RootCount;
  sInt RootAlloc;
#endif
public:
  sBroker_();
  ~sBroker_();

  void NewObject(sObject *);      // automatically called  
  void DeleteObject(sObject *);   // automatically called  
#if !sINTRO
  void AddRoot(sObject *);        // may be called only ONCE
  void RemRoot(sObject *);        // don't forget this!
  void Dump();                    // dump out all remaining objects
  void Stats(sInt &oc,sInt &rc) { oc = ObjectCount; rc = RootCount; }
#endif
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
#if !sINTRO
  virtual sBool Write(sU32 *&);
  virtual sBool Read(sU32 *&);
#endif
#if !sINTRO
  virtual void Clear();
  virtual void Copy(sObject *);
#endif
  sU32 TagVal;
  sU32 _Label;
};

#define sTAGVAL_USED    0x80000000
#define sTAGVAL_ROOT    0x40000000
#define sTAGVAL_INDEX   0x3fffffff

typedef sObject *(*sNewHandler)();

/****************************************************************************/
/***                                                                      ***/
/***   List                                                               ***/
/***                                                                      ***/
/****************************************************************************/

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
  void Add(Type *o);
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
  sInt i;
  i = 16;
  Alloc=i;
  Count=0; 
  Array=new Type*[i]; 
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
    sInt n=Alloc*2; 
    Type **a=new Type*[n]; 
    sCopyMem(a,Array,Count*4); 
    delete[] Array;
    Array=a; 
    Alloc=n; 
  } 
  Array[Count++] = o; 
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
void sArrayCopy(sArrayBase *dest,sArrayBase *src);
sU8 *sArrayAdd(sArrayBase *);

template <class Type> struct sArray
{
  Type *Array;
  sInt Count;
  sInt Alloc;
  sInt TypeSize;

  __forceinline void Init()                     { sArrayInit((sArrayBase *)this,sizeof(Type),0); }
  __forceinline void Init(sInt c)               { sArrayInit((sArrayBase *)this,sizeof(Type),c); }
  __forceinline void Exit()                     { delete[] Array; }
  __forceinline void SetMax(sInt c)             { sArraySetMax((sArrayBase *)this,c); }
  __forceinline void AtLeast(sInt c)            { sArrayAtLeast((sArrayBase *)this,c); }
  __forceinline Type Get(sInt i)                { return Array[i]; }
  __forceinline void Set(sInt i,Type o)         { Array[i] = o; }
  __forceinline Type &operator[](sInt i)        { return Array[i]; }
  __forceinline void Copy(sArray<Type> &a)      { sArrayCopy((sArrayBase *)this,(sArrayBase *)&a); }
  __forceinline Type *Add()                     { return (Type *)sArrayAdd((sArrayBase *)this); }
};

/****************************************************************************/
/*
  void AtLeast(sInt c)            { if(c>Alloc) SetMax(sMax(c,Alloc*2)); }
  Type Get(sInt i)                { return Array[i]; }
  void Set(sInt i,Type o)         { Array[i] = o; }
  Type &operator[](sInt i)        { return Array[i]; }
  void Copy(sArray<Type> &);
  Type *Add()                     { AtLeast(Count+1); return &Array[Count++]; }

template <class Type> void inline sArray<Type>::Init()                     
{ 
  Count=0; 
  Alloc=0;
  Array=0;
}

template <class Type> void inline sArray<Type>::Init(sInt c)               
{ 
  Count=0; 
  Alloc=c; 
  Array=new Type[c]; 
}

template <class Type> void inline sArray<Type>::Exit()                     
{ 
  if(Array) 
    delete[] Array; 
}

template <class Type> void inline sArray<Type>::SetMax(sInt c)             
{ 
  Type *n; 
  sVERIFY(c > Alloc);
  n=new Type[c]; 
  if(Array)
  {
    sCopyMem(n,Array,sizeof(Type)*Count); 
    delete[] Array; 
  }
  Count=sMin(Count,c); 
  Array=n; 
  Alloc=c; 
}


template <class Type> void inline sArray<Type>::Copy(sArray<Type> &c)
{
  if(c.Count > Alloc)
    SetMax(c.Count);
  sCopyMem(Array,c.Array,sizeof(Type)*c.Count);
  Count = c.Count;
}
*/
/****************************************************************************/
/***                                                                      ***/
/***   Bitmap                                                             ***/
/***                                                                      ***/
/****************************************************************************/

class sDLLSYSTEM sBitmap : public sObject
{
public:
  sBitmap();
  ~sBitmap();
  sU32 GetClass() { return sCID_BITMAP; }

  sU32 *Data;
  sInt XSize;
  sInt YSize;

  void Init(sInt xs,sInt ys);
#if !sINTRO
  void Copy(sObject *);
  void Clear();
  sBool Write(sU32 *&);
  sBool Read(sU32 *&);
#endif
};

/****************************************************************************/
/***                                                                      ***/
/***   StringSet                                                          ***/
/***                                                                      ***/
/****************************************************************************/

#if !sINTRO

struct sStringSetEntry
{
  sChar *String;
  sObject *Object;
};

struct sStringSet
{
private:
  sInt FindNr(sChar *string);
public:
  sArray<sStringSetEntry>  Set;

  void Init();                              // create
  void Exit();                              // destroy
  void Clear();
  sObject *Find(sChar *string);             // find object set for string
  sBool Add(sChar *string,sObject *);       // add object/string pair. return sTRUE if already in list. overwites
};

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Text (no, this is not a string class, this is a wrapper)           ***/
/***                                                                      ***/
/****************************************************************************/

#if !sINTRO
class sDLLSYSTEM sText : public sObject
{
  static sChar *DummyString;
public:
  sText();
  ~sText();
  sU32 GetClass() { return sCID_TEXT; }

  sChar *Text;
  sInt Alloc;

  void Init(sChar *,sInt len=-1);
  void Clear();
  void Copy(sObject *);
  sBool Write(sU32 *&);
  sBool Read(sU32 *&);
};
#endif

/****************************************************************************/
/***                                                                      ***/
/***   sPerfmon Support (use it even where util.hpp is not included)      ***/
/***                                                                      ***/
/****************************************************************************/


#if sLINK_UTIL
#define sZONE(x) sPerfMonZone __usezone_##x(__zone_##x);
#define sMAKEZONE(x,name,col) sPerfMonToken __zone_##x={name,col,-1}; 
#define sREGZONE(x)  sPerfMon->Register(__zone_##x); 
#else
#define sZONE(x)
#define sMAKEZONE(x,name,col)
#define sREGZONE(x)
#endif

/****************************************************************************/
/***                                                                      ***/
/***   MusicPlayers                                                       ***/
/***                                                                      ***/
/****************************************************************************/

#if sINTRO
struct sMusicPlayer
{
public:
  sU8 *Stream;                      // the loaded data
};
#else
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
  ~sMusicPlayer();
  sBool Load(sChar *name);          // load from file, memory is deleted automatically
  sBool Load(sU8 *data,sInt size);  // load from buffer, memory is not deleted automatically
  void AllocRewind(sInt bytes);     // allocate a rewindbuffer
  sBool Start(sInt songnr);         // initialize and start playing
  sBool Handler(sS16 *buffer,sInt samples); // return sFALSE if all following calls to handler will just return silence

  virtual sBool Init(sInt songnr)=0;
  virtual sInt Render(sS16 *buffer,sInt samples)=0;   // return number of samples actually rendered, 0 for eof
};
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Code Coverage                                                      ***/
/***                                                                      ***/
/****************************************************************************/

#if sCODECOVERMODE==0 // disable
# define sCODECOVER(id) 1
#elif sCODECOVERMODE==1 // analyze
  extern sU8 sCodeCoverString[];
# define sCODECOVER(id) (sCodeCoverString[id]=1)
#else // optimize
#include "codecoverage.hpp"
# define sCODECOVER(id) (sCODECOVERSTRING[id])
#endif

/****************************************************************************/
/***                                                                      ***/
/***   RangeCoder                                                         ***/
/***                                                                      ***/
/****************************************************************************/

class RangeCoder
{
  sU32 Low;
  sU32 Code;
  sU32 Range;
  sU32 Bytes;
  sU8 *Buffer;

public:
#if !sPLAYER
  void InitEncode(sU8 *buffer);
  void FinishEncode();
  void Encode(sU32 cumfreq,sU32 freq,sU32 totfreq);
  void EncodePlain(sU32 value,sU32 max);
  void EncodeGamma(sU32 value);
#endif

  void InitDecode(sU8 *buffer);
  sU32 GetFreq(sU32 totfreq);
  void Decode(sU32 cumfreq,sU32 freq);
  sU32 DecodePlain(sU32 max);
  sU32 DecodeGamma();

  sU32 GetBytes()             { return Bytes; }
};

/****************************************************************************/

class RangeModel
{
  sInt *Freq;
  sInt Symbols;
  sInt TotFreq;

  void Update(sInt symbol);

public:
  void Init(sInt symbols);
  void Exit()                 { delete[] Freq; }

#if !sPLAYER
  void Encode(RangeCoder &coder,sInt symbol);
#endif
  sInt Decode(RangeCoder &coder);
};

/****************************************************************************/

#endif

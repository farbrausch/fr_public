/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_BASE_MACHINE_HPP
#define FILE_ALTONA2_LIBS_BASE_MACHINE_HPP

; // if this semicolon causes an error, then something is wrong BEFORE
  // you include types.hpp (or anything else)

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***   Determine Configuration                                            ***/
/***                                                                      ***/
/****************************************************************************/

// platform / operating system

#define sConfigPlatformWin     11 
#define sConfigPlatformLinux   12
#define sConfigPlatformOSX     13
#define sConfigPlatformIOS     14
#define sConfigPlatformAndroid 15

// rendering backends

#define sConfigRenderNull     20
#define sConfigRenderDX9      21
#define sConfigRenderDX11     22
#define sConfigRenderGL2      23
#define sConfigRenderGLES2    24
//#define sConfigRenderGL4      25
//#define sConfigRenderGLES1    26

// compiler frontend

#define sConfigCompilerMsc    31
#define sConfigCompilerGcc    32
#define sConfigCompilerLlvm   33

// build

#define sConfigBuildDebug 1
#define sConfigBuildDFast 2
#define sConfigBuildOptim 3
#define sConfigBuildFinal 4

/****************************************************************************/

#ifdef sDEFINE_DX9
#define sConfigRender         sConfigRenderDX9
#elif sDEFINE_DX11
#define sConfigRender         sConfigRenderDX11
#elif sDEFINE_GL2
#define sConfigRender         sConfigRenderGL2
#elif sDEFINE_GLES2
#define sConfigRender         sConfigRenderGLES2
#else
#define sConfigRender         sConfigRenderNull
#endif

#if defined(sDEFINE_LINUX)
#define sConfigPlatform       sConfigPlatformLinux
#elif defined(sDEFINE_OSX)
#define sConfigPlatform       sConfigPlatformOSX
#elif defined(sDEFINE_IOS)
#define sConfigPlatform       sConfigPlatformIOS
#elif defined(sDEFINE_ANDROID)
#define sConfigPlatform       sConfigPlatformAndroid
#else
#define sConfigPlatform       sConfigPlatformWin
#endif

#if defined(sDEFINE_O0)
#define sConfigBuild          sConfigBuildDebug
#elif defined(sDEFINE_O1)
#define sConfigBuild          sConfigBuildDFast
#elif defined(sDEFINE_O2)
#define sConfigBuild          sConfigBuildOptim
#elif defined(sDEFINE_O3)
#define sConfigBuild          sConfigBuildFinal
#endif

/****************************************************************************/


#if defined(sDEFINE_LLVM)

#define sConfigCompiler       sConfigCompilerLlvm
#if __SIZEOF_POINTER__ == 8
#define sConfig64Bit          1
#else
#define sConfig64Bit          0
#endif

#elif defined(__GNUC__)
#define sConfigCompiler       sConfigCompilerGcc
#if __SIZEOF_POINTER__ == 8
#define sConfig64Bit          1
#else
#define sConfig64Bit          0
#endif

#else
#define sConfigCompiler       sConfigCompilerMsc
#ifdef _M_AMD64
#define sConfig64Bit          1
#else
#define sConfig64Bit          0
#endif

#endif

/****************************************************************************/

#ifdef sDEFINE_FINAL
#define sConfigLogging        0
#else
#define sConfigLogging        1
#endif

#ifdef sDEFINE_DEBUG
#define sConfigDebug          1
#else
#define sConfigDebug          0
#endif

#ifdef sDEFINE_SHELL
#define sConfigShell          1
#else
#define sConfigShell          0
#endif

#ifdef sDEFINE_NOSETUP
#define sConfigNoSetup        1
#else
#define sConfigNoSetup        0
#endif
    
#ifdef sDEFINE_FINAL
#define sConfigAssert         0
#else
#define sConfigAssert         1
#endif

#if defined(sDEFINE_FINAL) || sConfigPlatform==sConfigPlatformIOS
#define sConfigDebugMem       0
#else
#define sConfigDebugMem       1
#endif

#if sConfigPlatform == sConfigPlatformIOS
#define sConfigStrictAlign    1
#else
#define sConfigStrictAlign    0
#endif

#ifdef sDEFINE_PLAYER
#define sConfigPlayer         1
#else
#define sConfigPlayer         0
#endif

/****************************************************************************/
/***                                                                      ***/
/***    Basic Variables                                                   ***/
/***                                                                      ***/
/****************************************************************************/

typedef int                 sS32;
typedef unsigned int        sU32;
typedef short               sS16;
typedef unsigned short      sU16;
typedef signed char         sS8;
typedef unsigned char       sU8;
typedef float               sF32;
typedef double              sF64;
typedef long long           sS64;
typedef unsigned long long  sU64;

typedef int                 sInt;         // 32 bit signed int
typedef char                sChar;        // signed or unsigned, depending on os
typedef bool                sBool;        // special c++ semantics
#if sConfig64Bit
typedef long long           sSPtr;        // signed pointer size integert
typedef unsigned long long  sPtr;         // unsigned pointer size integer
#else
typedef int                 sSPtr;        // signed pointer size integert
typedef unsigned int        sPtr;         // unsigned pointer size integer
#endif

// new style, where you can use bool, char, int float & double

typedef signed char         int8;
typedef unsigned char       uint8;
//                          char
typedef short               int16;
typedef unsigned short      uint16;
typedef wchar_t             wchar;
//                          int;
typedef unsigned int        uint;
//                          float;
typedef long long           int64;
typedef unsigned long long  uint64;
//                          double;
#if sConfig64Bit
typedef long long           sptr;         // signed pointer size integert
typedef unsigned long long  uptr;         // unsigned pointer size integer
#else
typedef int                 sptr;         // signed pointer size integert
typedef unsigned int        uptr;         // unsigned pointer size integer
#endif
//                          bool;         // implementation dependen size!!!!

/****************************************************************************/
/***                                                                      ***/
/***    Compiler Specifics                                                ***/
/***                                                                      ***/
/****************************************************************************/

#if sConfigCompiler == sConfigCompilerMsc

#define sFORCEINLINE      __forceinline           // use only when you think you need force...
#define sNOINLINE         __declspec(noinline)    // forbid inlining. usefull to hint compiler errors
#define sNORETURN         __declspec(noreturn)    // use this for dead end funtions
#define sOBSOLETE         __declspec(deprecated)  // do not use any more, will be removed soon
#define sALIGNOF(x)       __alignof(x)
inline void sDebugBreak() { __debugbreak(); }
#define sALIGNED(type, name, al)  __declspec( align (al) ) type name
#define sTHREADLOCALSTORAGE __declspec(thread)
//#else
//#define sTHREADLOCALSTORAGE __thread
#endif


#if sConfigCompiler == sConfigCompilerGcc || sConfigCompiler == sConfigCompilerLlvm

#define sFORCEINLINE      
#define sNOINLINE         
#define sNORETURN         
#define sOBSOLETE
#define sALIGNOF(x)       __alignof__(x)
#if (sConfigPlatform==sConfigPlatformAndroid)
extern "C" { void Android_DebugBreak(); };

inline void sDebugBreak() { Android_DebugBreak(); }
#else
inline void sDebugBreak() {  }
#endif
#define sALIGNED(type, name, al)  type name __attribute__ ((aligned (al)))

#define sTHREADLOCALSTORAGE
    
}
#if sConfig64Bit
typedef long unsigned int size_t;
#else
#if sConfigCompiler == sConfigCompilerLlvm
typedef long unsigned int size_t;
#else
typedef unsigned int size_t;
#endif
#endif
namespace Altona2 {
#endif
/****************************************************************************/
/***                                                                      ***/
/***   Useful constants                                                   ***/
/***                                                                      ***/
/****************************************************************************/

const float sPi            = 3.14159265359f;
const float s2Pi           = 6.28318530718f;
const float sTau           = 6.28318530718f;        // Tau = 2 Pi
const float sSqrt2         = 1.41421356237f;

const double sPiDouble      = 3.1415926535897932385;
const double s2PiDouble     = 6.2831853071795864769;
const double sSqrt2Double   = 1.4142135623730950488;

const int sMaxPath       = 2048;   // no real limit, just a useful tradeoff
const int sFormatBuffer  = 2048;   // size of temp buffer used by printf() style functions

/****************************************************************************/

const uint sMaxU8         = 0xffU;
const uint sMaxU16        = 0xffffU;
const uint sMaxU32        = 0xffffffffUL;
const uint64 sMaxU64        = 0xffffffffffffffffULL;

const int sMaxS8         =  0x7f;
const int sMinS8         = -0x80;
const int sMaxS16        =  0x7fff;
const int sMinS16        = -0x8000;
const int sMaxS32        =  0x7fffffff;
const int sMinS32        = -0x80000000LL;
const int64 sMaxS64        =  0x7fffffffffffffffLL;
const int64 sMinS64        = -0x8000000000000000LL;
const float sMaxF32        = 1E+37f;   // actually approximate
const float sMinF32        = 1E-37f;   // actually approximate smallest integer >0 !=0

/****************************************************************************/
/***                                                                      ***/
/***   memory                                                             ***/
/***                                                                      ***/
/****************************************************************************/

void *sAllocMem_(uptr size,int align,int flags=0);
void *sAllocMem_(uptr size,int align,int flags,const char *file,int line);
void sFreeMem(void *mem);
uptr sMemSize(void *mem);

void sBreakOnAllocId(int id);
int sGetAllocId();
uptr sGetTotalMemUsed();
bool sDumpMemory();
void sCheckMemory();
void sDebugMemory(bool enable);

/****************************************************************************/

}

inline void *operator new  (size_t size)throw() { return Altona2::sAllocMem_(size,16,0); }
inline void *operator new[](size_t size)throw() { return Altona2::sAllocMem_(size,16,0); }
inline void *operator new  (size_t size,int flags)throw() { return Altona2::sAllocMem_(size,16,flags); }
inline void *operator new[](size_t size,int flags)throw() { return Altona2::sAllocMem_(size,16,flags); }
inline void operator delete  (void* p)throw() { Altona2::sFreeMem(p); }
inline void operator delete[](void* p)throw() { Altona2::sFreeMem(p); }
inline void *operator new  (size_t, void *ptr)throw() { return ptr; }
inline void *operator new[](size_t, void *ptr)throw() { return ptr; }
inline void operator delete  (void* p,void* q)throw() { }
inline void operator delete[](void* p,void* q)throw() { }
#define __PLACEMENT_NEW_INLINE
#define __PLACEMENT_VEC_NEW_INLINE

namespace Altona2 {

template<class T> 
T* sPlacementNew(void *ptr){ return new (ptr) T; }
template<class T, typename ARG0> 
T* sPlacementNewConst(void *ptr, const ARG0 &arg0){ return new (ptr) T(arg0); }
template<class T, typename ARG0> 
T* sPlacementNew(void *ptr, ARG0 arg0){ return new (ptr) T(arg0); }
template<class T, typename ARG0, typename ARG1> 
T* sPlacementNew(void *ptr, ARG0 arg0, ARG1 arg1){ return new (ptr) T(arg0, arg1); }
template<class T, typename ARG0, typename ARG1, typename ARG2> 
T* sPlacementNew(void *ptr, ARG0 arg0, ARG1 arg1, ARG2 arg2){ return new (ptr) T(arg0, arg1, arg2); }

/****************************************************************************/

#if sConfigDebugMem

}

inline void *operator new  (size_t size,const char *file,int line)throw() { return Altona2::sAllocMem_(size,16,0,file,line); }
inline void *operator new[](size_t size,const char *file,int line)throw() { return Altona2::sAllocMem_(size,16,0,file,line); }
inline void *operator new  (size_t size,int flags,const char *file,int line)throw() { return Altona2::sAllocMem_(size,16,flags,file,line); }
inline void *operator new[](size_t size,int flags,const char *file,int line)throw() { return Altona2::sAllocMem_(size,16,flags,file,line); }
inline void operator delete  (void *p, const char *, int)throw() { Altona2::sFreeMem(p); }
inline void operator delete[](void *p, const char *, int)throw() { Altona2::sFreeMem(p); }

namespace Altona2 {

#define _MFC_OVERRIDES_NEW
#define sDEFINE_NEW new(__FILE__,__LINE__)
#define sAllocMem(size,align,flags)  sAllocMem_(size,align,flags,__FILE__,__LINE__)

#else

#define sDEFINE_NEW new
#define sAllocMem(size,align,flags)  sAllocMem_(size,align,flags)

#endif

#define new sDEFINE_NEW

/****************************************************************************/
/***                                                                      ***/
/***   use macros and debugging                                           ***/
/***                                                                      ***/
/****************************************************************************/

void sAssertImpl(const char *file,int line);

#if sConfigAssert
#define sASSERT(x) {if(!(x)){ sDebugBreak(); ;sAssertImpl(__FILE__,__LINE__);}}
#define sASSERT0() { sDebugBreak(); ;sAssertImpl(__FILE__,__LINE__);}
#else
#define sASSERT(x) {(x);}
#define sASSERT0() {}
#endif
#define sASSERTSTATIC(x) { struct STATIC_ASSERTION {bool STATIC_ASSERTION_[(x)?1:-1]; }; }

/****************************************************************************/
/***                                                                      ***/
/***   program startup and exit                                           ***/
/***                                                                      ***/
/****************************************************************************/

void Main();
void sSetExitCode(int);
sNORETURN void sFatal(const char *);
const char *sGetCommandline();
void sWaitForKey();

/****************************************************************************/

class sSubsystem
{
private:
    static sSubsystem *First;
    static int Runlevel;
    const char *Name;
    int Priority;
    sSubsystem *Next;
public:  
    static void SetRunlevel(int);
    sSubsystem(const char *name,int pri);
    sSubsystem();
    void Register(const char *name,int pri);
    virtual ~sSubsystem();
    virtual void Init();
    virtual void Exit();
    static int GetRunlevel() { return Runlevel; }
};

/****************************************************************************/
/***                                                                      ***/
/***   logging                                                            ***/
/***                                                                      ***/
/****************************************************************************/

void sPrint(const char *);
#if sConfigLogging
void sDPrint(const char *);
void sLog(const char *system,const char *msg);
#else
inline void sDPrint(const char *) {}
inline void sLog(const char *system,const char *msg) {}
#endif

/****************************************************************************/
/***                                                                      ***/
/***   floating point control                                             ***/
/***                                                                      ***/
/****************************************************************************/

enum sFloatControlFlags
{
    sFC_GeneralException    = 0x0001,
    sFC_DenormalException   = 0x0002,
    sFC_InexcactException   = 0x0004,
    sFC_DenormalDisable     = 0x0100,

    sFC_PrecisionMask       = 0x3000,
    sFC_DefaultPrecision    = 0x0000,
    sFC_SinglePrecision     = 0x1000,
    sFC_DoublePrecision     = 0x2000,
    sFC_ExtendedPrecision   = 0x3000,
};

void sFpuControl(int flags);

/****************************************************************************/
/***                                                                      ***/
/***   intrinsics (and things that should be intrinsic)                   ***/
/***                                                                      ***/
/****************************************************************************/

extern uint sQuietNan_;
inline float sQuietNan() { return *(float *)&sQuietNan_; } // ffffffff
inline bool sIsNan(float x) { uint i = *(uint *)(&x); return (i&0xff800000)==0xff800000 && (i&0x007fffff)!=0; }

/****************************************************************************/

#if sConfigCompiler == sConfigCompilerMsc

}

#include <intrin.h>
#include <math.h>
#include <memory.h>
#include <string.h>

namespace Altona2 {

// memory

inline uptr sGetStringLen(const char *s)               { return strlen(s); }
inline void sCopyMem(void *d,const void *s,uptr c)      { memcpy(d,s,c); }
inline void sSetMem(void *d,int s,uptr c)              { memset(d,s,c); }
inline int sCmpMem(const void *a,const void *b,uptr c) { return memcmp(a,b,c); }
void sMoveMem(void *d,const void *s,uptr c);

// integer

inline int sAbs(int a)                      { return abs(a); }
//inline int sSign(int a)                     { return sign(a); }

// large integer

inline int sMulDiv(int a,int b,int c)     { return int(__emul(a,b)/c); }
inline uint sMulDivU(uint a,uint b,uint c)    { return uint(__emulu(a,b)/c); }
inline int sMulShift(int a,int b)          { return int(__emul(a,b)>>16); }
inline uint sMulShiftU(uint a,uint b)         { return uint(__emulu(a,b)>>16); }
inline int sDivShift(int a,int b)          { return int((int64(a)<<16)/b); }
inline uint sDivShift(uint a,uint b)          { return int((uint64(a)<<16)/b); }

// float math

inline float sAbs(float f)                      { return fabsf(f); }
inline float sATan(float f)                     { return (float)atanf(f); }
inline float sATan2(float over,float under)      { return (float)atan2f(over,under); }
inline float sCos(float f)                      { return (float)cosf(f); }
inline float sACos(float f)                     { return (float)acosf(f); }  
inline float sExp(float f)                      { return (float)expf(f); }
inline float sPow(float b,float e)               { return (float)powf(b,e); }
inline float sLog(float f)                      { return (float)logf(f); }
inline float sLog10(float f)                    { return (float)log10f(f); }
inline float sSin(float f)                      { return (float)sinf(f); }
inline float sSqrt(float f)                     { return (float)sqrtf(f); }
inline float sRSqrt(float f)                    { return 1.0f/(float)sqrtf(f); }
inline float sTan(float f)                      { return (float)tanf(f); }
inline void sSinCos(float f,float &s,float &c)   { s = sinf(f); c = cosf(f); }
inline float sFMod(float a,float b)              { return fmodf(a,b); }

inline float sRoundDown(float f)                { return floorf(f); }
inline float sRoundUp(float f)                  { return ceilf(f); }

// double math

inline double sAbs(double f)                      { return fabs(f); }
inline double sATan(double f)                     { return atan(f); }
inline double sATan2(double over,double under)      { return atan2(over,under); }
inline double sCos(double f)                      { return cos(f); }
inline double sExp(double f)                      { return exp(f); }
inline double sPow(double b,double e)               { return pow(b,e); }
inline double sLog(double f)                      { return log(f); }
inline double sLog10(double f)                    { return log10(f); }
inline double sSin(double f)                      { return sin(f); }
inline double sSqrt(double f)                     { return sqrt(f); }
inline double sRSqrt(double f)                    { return 1.0/sqrt(f); }
inline double sTan(double f)                      { return tan(f); }
inline void sSinCos(double f,double &s,double &c)   { s = sin(f); c = cos(f); }
inline double sFMod(double a,double b)              { return fmod(a,b); }

inline double sRoundDown(double f)                { return floor(f); }
inline double sRoundUp(double f)                  { return ceil(f); }

#endif

/****************************************************************************/

#if sConfigCompiler == sConfigCompilerGcc

// memory

inline uptr sGetStringLen(const char *s)               { return __builtin_strlen(s); }
inline void sCopyMem(void *d,const void *s,uptr c)      { __builtin_memcpy(d,s,c); }
inline void sSetMem(void *d,int s,uptr c)              { __builtin_memset(d,s,c); }
inline int sCmpMem(const void *a,const void *b,uptr c) { return __builtin_memcmp(a,b,c); }
void sMoveMem(void *d,const void *s,uptr c);

// integer

inline int sAbs(int a)                      { return __builtin_abs(a); }
//inline int sSign(int a)                     { return __builtin_sign(a); }

// large integer

inline int sMulDiv(int a,int b,int c)     { return int(int64(a)*b/c); }
inline uint sMulDivU(uint a,uint b,uint c)    { return uint(uint64(a)*b/c); }
inline int sMulShift(int a,int b)          { return (int64(a)*b)>>16; }
inline uint sMulShiftU(uint a,uint b)         { return (uint64(a)*b)>>16; }
inline int sDivShift(int a,int b)          { return int((int64(a)<<16)/b); }
inline uint sDivShift(uint a,uint b)          { return int((uint64(a)<<16)/b); }

// float math

inline float sAbs(float f)                      { return __builtin_fabsf(f); }
inline float sATan(float f)                     { return (float)__builtin_atanf(f); }
inline float sATan2(float over,float under)      { return (float)__builtin_atan2f(over,under); }
inline float sCos(float f)                      { return (float)__builtin_cosf(f); }
inline float sACos(float f)                     { return (float)__builtin_acosf(f); }  
inline float sExp(float f)                      { return (float)__builtin_expf(f); }
inline float sPow(float b,float e)               { return (float)__builtin_powf(b,e); }
inline float sLog(float f)                      { return (float)__builtin_logf(f); }
inline float sLog10(float f)                    { return (float)__builtin_log10f(f); }
inline float sSin(float f)                      { return (float)__builtin_sinf(f); }
inline float sSqrt(float f)                     { return (float)__builtin_sqrtf(f); }
inline float sRSqrt(float f)                    { return 1.0f/(float)__builtin_sqrtf(f); }
inline float sTan(float f)                      { return (float)__builtin_tanf(f); }

#if sConfigPlatform==sConfigPlatformAndroid
inline void sSinCos(float f,float &s,float &c)   { s = __builtin_sinf(f); c = __builtin_cosf(f); }
#else
inline void sSinCos(float f,float &s,float &c)   { __builtin_sincosf(f,&s,&c); }
#endif

inline float sFMod(float a,float b)              { return (float)__builtin_fmodf(a,b); }

inline float sRoundDown(float f)                { return __builtin_floorf(f); }
inline float sRoundUp(float f)                  { return __builtin_ceilf(f); }

// double math

inline double sAbs(double f)                      { return __builtin_fabs(f); }
inline double sATan(double f)                     { return __builtin_atan(f); }
inline double sATan2(double over,double under)      { return __builtin_atan2(over,under); }
inline double sCos(double f)                      { return __builtin_cos(f); }
inline double sExp(double f)                      { return __builtin_exp(f); }
inline double sPow(double b,double e)               { return __builtin_pow(b,e); }
inline double sLog(double f)                      { return __builtin_log(f); }
inline double sLog10(double f)                    { return __builtin_log10(f); }
inline double sSin(double f)                      { return __builtin_sin(f); }
inline double sSqrt(double f)                     { return __builtin_sqrt(f); }
inline double sRSqrt(double f)                    { return 1.0/__builtin_sqrt(f); }
inline double sTan(double f)                      { return __builtin_tan(f); }
inline void sSinCos(double f,double &s,double &c)   { s = __builtin_sin(f); c = __builtin_cos(f); }
inline double sFMod(double a,double b)              { return __builtin_fmod(a,b); }

inline double sRoundDown(double f)                { return __builtin_floor(f); }
inline double sRoundUp(double f)                  { return __builtin_ceil(f); }

#endif


/****************************************************************************/

#if sConfigCompiler == sConfigCompilerLlvm

// memory

inline uptr sGetStringLen(const char *s)               { return __builtin_strlen(s); }
inline void sCopyMem(void *d,const void *s,uptr c)      { __builtin_memcpy(d,s,c); }
inline void sSetMem(void *d,int s,uptr c)              { __builtin_memset(d,s,c); }
inline int sCmpMem(const void *a,const void *b,uptr c) { return __builtin_memcmp(a,b,c); }
void sMoveMem(void *d,const void *s,uptr c);

// integer

inline int sAbs(int a)                      { return __builtin_abs(a); }
//inline int sSign(int a)                     { return __builtin_sign(a); }

// large integer

inline int sMulDiv(int a,int b,int c)     { return int(int64(a)*b/c); }
inline uint sMulDivU(uint a,uint b,uint c)    { return uint(uint64(a)*b/c); }
inline int sMulShift(int a,int b)          { return (int)((int64(a)*b)>>16); }
inline uint sMulShiftU(uint a,uint b)         { return (uint)((uint64(a)*b)>>16); }
inline int sDivShift(int a,int b)          { return int((int64(a)<<16)/b); }
inline uint sDivShift(uint a,uint b)          { return int((uint64(a)<<16)/b); }
  
// float math

inline float sAbs(float f)                      { return __builtin_fabsf(f); }
inline float sATan(float f)                     { return (float)__builtin_atanf(f); }
inline float sATan2(float over,float under)      { return (float)__builtin_atan2f(over,under); }
inline float sCos(float f)                      { return (float)__builtin_cosf(f); }
inline float sACos(float f)                     { return (float)__builtin_acosf(f); }
inline float sExp(float f)                      { return (float)__builtin_expf(f); }
inline float sPow(float b,float e)               { return (float)__builtin_powf(b,e); }
inline float sLog(float f)                      { return (float)__builtin_logf(f); }
inline float sLog10(float f)                    { return (float)__builtin_log10f(f); }
inline float sSin(float f)                      { return (float)__builtin_sinf(f); }
inline float sSqrt(float f)                     { return (float)__builtin_sqrtf(f); }
inline float sRSqrt(float f)                    { return 1.0f/(float)__builtin_sqrtf(f); }
inline float sTan(float f)                      { return (float)__builtin_tanf(f); }
inline void sSinCos(float f,float &s,float &c)   { s = sSin(f); c = sCos(f); }
inline float sFMod(float a,float b)              { return (float)__builtin_fmodf(a,b); }

inline float sRoundDown(float f)                { return __builtin_floorf(f); }
inline float sRoundUp(float f)                  { return __builtin_ceilf(f); }

// double math

inline double sAbs(double f)                      { return __builtin_fabs(f); }
inline double sATan(double f)                     { return __builtin_atan(f); }
inline double sATan2(double over,double under)      { return __builtin_atan2(over,under); }
inline double sCos(double f)                      { return __builtin_cos(f); }
inline double sExp(double f)                      { return __builtin_exp(f); }
inline double sPow(double b,double e)               { return __builtin_pow(b,e); }
inline double sLog(double f)                      { return __builtin_log(f); }
inline double sLog10(double f)                    { return __builtin_log10(f); }
inline double sSin(double f)                      { return __builtin_sin(f); }
inline double sSqrt(double f)                     { return __builtin_sqrt(f); }
inline double sRSqrt(double f)                    { return 1.0/__builtin_sqrt(f); }
inline double sTan(double f)                      { return __builtin_tan(f); }
inline void sSinCos(double f,double &s,double &c)   { s = __builtin_sin(f); c = __builtin_cos(f); }
inline double sFMod(double a,double b)              { return __builtin_fmod(a,b); }

inline double sRoundDown(double f)                { return __builtin_floor(f); }
inline double sRoundUp(double f)                  { return __builtin_ceil(f); }

#endif

/****************************************************************************/
/***                                                                      ***/
/***   lockless programming                                               ***/
/***                                                                      ***/
/****************************************************************************/

#if sConfigCompiler == sConfigCompilerMsc
} // namespace
extern "C"
{
    long _InterlockedIncrement(long volatile *); long _InterlockedDecrement(long volatile *);
    long _InterlockedExchangeAdd(long volatile *,long); long _InterlockedExchange(long volatile *,long);

    __int64 _InterlockedIncrement64(__int64 volatile *); __int64 _InterlockedDecrement64(__int64 volatile *);
    __int64 _InterlockedExchangeAdd64(__int64 volatile *,__int64);
    void _ReadBarrier(void); void _WriteBarrier(void);
}
namespace Altona2 {
#pragma intrinsic (_WriteBarrier,_ReadBarrier,_InterlockedExchangeAdd,_InterlockedIncrement,_InterlockedDecrement)

// sWriteBarrier/sReadBarrier are not full memory barriers 
// (_WriteBarrier/_ReadBarrier are only compiler barriers and DO NOT prevent CPU reordering)
// practically this means that still reads can moved ahead of write by the CPU

inline void sWriteBarrier() { _WriteBarrier(); }
inline void sReadBarrier() { _ReadBarrier(); }

// these functions return the NEW value after the operation was done on the memory address

inline uint sAtomicAdd(volatile uint *p,int i) { uint e = _InterlockedExchangeAdd((volatile long *)p,i); return e+i; }
inline uint sAtomicInc(volatile uint *p) { return _InterlockedIncrement((long *)p); }
inline uint sAtomicDec(volatile uint *p) { return _InterlockedDecrement((long *)p); }
inline uint64 sAtomicAdd(volatile uint64 *p,int64 i) { uint64 e = _InterlockedExchangeAdd64((volatile __int64 *)p,i); return e+i; }
inline uint64 sAtomicInc(volatile uint64 *p) { return _InterlockedIncrement64((__int64 *)p); }
inline uint64 sAtomicDec(volatile uint64 *p) { return _InterlockedDecrement64((__int64 *)p); }
inline uint sAtomicSwap(volatile uint *p, uint i) { return _InterlockedExchange((long*)p,i); }

#endif

/****************************************************************************/

#if sConfigCompiler == sConfigCompilerGcc || sConfigCompiler == sConfigCompilerLlvm

inline void sWriteBarrier() {  }
inline void sReadBarrier() {  }

// these functions return the NEW value after the operation was done on the memory address

inline uint sAtomicAdd(volatile uint *p,int i)   { return __sync_add_and_fetch(p,i); } 
inline uint sAtomicInc(volatile uint *p)          { return __sync_add_and_fetch(p,1); }
inline uint sAtomicDec(volatile uint *p)          { return __sync_add_and_fetch(p,-1); }
inline uint64 sAtomicAdd(volatile uint64 *p,int64 i)   { return __sync_add_and_fetch(p,i); } 
inline uint64 sAtomicInc(volatile uint64 *p)          { return __sync_add_and_fetch(p,1); }
inline uint64 sAtomicDec(volatile uint64 *p)          { return __sync_add_and_fetch(p,-1); }
inline uint sAtomicSwap(volatile uint *p, uint i) { return __sync_lock_test_and_set(p,i); }

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Simple Functions (on float and int)                                ***/
/***                                                                      ***/
/****************************************************************************/

template <class Type> inline Type sMin(Type a,Type b)               {return (a<b) ? a : b;}
template <class Type> inline Type sMax(Type a,Type b)               {return (a>b) ? a : b;}
template <class Type> inline Type sMin3(Type a,Type b,Type c)       {return sMin(sMin(a,b),c);}
template <class Type> inline Type sMax3(Type a,Type b,Type c)       {return sMax(sMax(a,b),c);}
template <class Type> inline Type sSign(Type a)                     {return (a==0) ? Type(0) : (a>0) ? Type(1) : Type(-1);}
template <class Type> inline int sCmp(Type a,Type b)               {return (a==b) ? 0 : (a>b) ? 1 : -1;}
template <class Type> inline Type sCompare(Type a,Type b)           {return (a>b) ? 1 : ((a==b) ? 0 : -1);}
template <class Type> inline Type sClamp(Type a,Type min,Type max)  {return (a>=max) ? max : (a<=min) ? min : a;}
template <class Type> inline void sSwap(Type &a,Type &b)            {Type s=a; a=b; b=s;}

/****************************************************************************/
/***                                                                      ***/
/***   Simple Functions (int)                                             ***/
/***                                                                      ***/
/****************************************************************************/

// alignment does not work if sizeof(a)>sizeof(uptr)
inline int sAlign(int a,int b)                                   {return ((a+b-1)&(~(b-1)));}
inline uint sAlign(uint a,int b)                                   {return ((a+b-1)&(~(b-1)));}
inline int64 sAlign(int64 a,int b)                                   {return ((a+b-1)&(~(b-1)));}
inline uint64 sAlign(uint64 a,int b)                                   {return ((a+b-1)&(~(b-1)));}
int sFindLowerPower(int x);
int sFindHigherPower(int x);
inline bool sIsPower2(int x)                                      {return (x&(x-1)) == 0;}
uint sMakeMask(uint max);
uint64 sMakeMask(uint64 max);
inline int sDivRoundDown(int a,int b)                            { return a>=0 ? a/b : (a-(b-1))/b; }
inline int64 sDivRoundDown(int64 a,int64 b)                            { return a>=0 ? a/b : (a-(b-1))/b; }

/****************************************************************************/
/***                                                                      ***/
/***   Simple Functions (pointerfrackers)                                 ***/
/***                                                                      ***/
/****************************************************************************/

template<typename TO,typename FROM>
inline TO &sRawCast(const FROM &src)
{ void *ptr = (void*)&src;  return *(TO*)ptr; }

#define sCOUNTOF(x) uptr(sizeof(x)/sizeof(*(x)))    // #elements of array, usefull for unicode strings

template <class Type> void sClear(Type &p) { sSetMem(&p,0,sizeof(p)); }

template<class t> void sRelease(t &x) { if(x) x->Release(); x=0; }
template<class t> void sDelete(t &x) { delete x; x=0; }
template<class t> void sDeleteArray(t &x) { delete[] x; x=0; }

template <typename RA,typename RB,typename SA,typename SB>
RB SA::*sAddMemberPtr(RA SA::*a,RB SB::*b)
{
    sASSERT(sizeof(a)==sizeof(uptr));
    RB SA::*x = 0;
    *(uptr *)&x = *(uptr *)&a + *(uptr *)&b;
    return x;
}

/****************************************************************************/

} // namespace Altona2

#endif  // FILE_ALTONA2_LIBS_BASE_MACHINE_HPP

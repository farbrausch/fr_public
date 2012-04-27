/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_UTIL_SIMD_FLOAT_HPP
#define FILE_UTIL_SIMD_FLOAT_HPP

#include "base/types.hpp"

/****************************************************************************/


#if sPLATFORM == sPLAT_WINDOWS || sPLATFORM == sPLAT_LINUX

#define sSIMD_INTRINSICS 1

#if sCONFIG_64BIT // 64bit builds always support SSE2 and don't support the MMX instrs we need without it
#undef sSIMD_SSE2
#define sSIMD_SSE2 1
#endif

#include <xmmintrin.h>
#if sSIMD_SSE2
#include <emmintrin.h>
#endif

// ---- Compatibility stuff

// x86 apparently can't pass more than 3 constant SSE parameters as value types, so we have
// to use const references where possible. PPC doesn't have this problem.
typedef const sSSE &sSSEPara;

// ---- Data movement

static sINLINE sSSE sVecZero()                          { return _mm_setzero_ps(); }

static sINLINE sSSE sVecLoad(const void *ptr)           { return _mm_load_ps((sF32*) ptr); }
static sINLINE sSSE sVecLoadU(const void *ptr)          { return _mm_loadu_ps((sF32*) ptr); }
static sINLINE sSSE sVecLoadScalar(sF32 x)              { return _mm_set1_ps(x); }
static sINLINE void sVecStore(sSSE vec,void *ptr)       { _mm_store_ps((sF32*) ptr,vec); }
static sINLINE void sVecStoreU(sSSE vec,void *ptr)      { _mm_storeu_ps((sF32*) ptr,vec); }
#define sVecStoreElem(v,ptr,i)                          (_mm_store_ss((sF32*) (ptr),_mm_shuffle_ps(v,v,(i)*0x55)))

// ---- Shuffling

static sINLINE sSSE sVecMergeH(sSSE a,sSSE b)           { return _mm_unpacklo_ps(a,b); }
static sINLINE sSSE sVecMergeL(sSSE a,sSSE b)           { return _mm_unpackhi_ps(a,b); }
#define sVecSplat(v,i)                                  _mm_shuffle_ps(v,v,(i)*0x55)
#define sVecShuffle(v,a,b,c,d)                          _mm_shuffle_ps(v,v,_MM_SHUFFLE(d,c,b,a))
static sINLINE sSSE sVecSwapHiLo(sSSE a)                { return sVecShuffle(a,2,3,0,1); }

// ---- Logical operations

static sINLINE sSSE sVecAnd(sSSE a,sSSE b)              { return _mm_and_ps(a,b); }
static sINLINE sSSE sVecAndC(sSSE a,sSSE b)             { return _mm_andnot_ps(b,a); }
static sINLINE sSSE sVecOr(sSSE a,sSSE b)               { return _mm_or_ps(a,b); }
static sINLINE sSSE sVecXor(sSSE a,sSSE b)              { return _mm_xor_ps(a,b); }
static sINLINE sSSE sVecSel(sSSE a,sSSE b,sSSE mask)    { return _mm_or_ps(_mm_and_ps(b,mask),_mm_andnot_ps(mask,a)); }

// ---- Arithmetic

static sINLINE sSSE sVecAdd(sSSE a,sSSE b)              { return _mm_add_ps(a,b); }
static sINLINE sSSE sVecSub(sSSE a,sSSE b)              { return _mm_sub_ps(a,b); }
static sINLINE sSSE sVecMul(sSSE a,sSSE b)              { return _mm_mul_ps(a,b); }
static sINLINE sSSE sVecMAdd(sSSE a,sSSE b,sSSE c)      { return _mm_add_ps(_mm_mul_ps(a,b),c); }
static sINLINE sSSE sVecNMSub(sSSE a,sSSE b,sSSE c)     { return _mm_sub_ps(c,_mm_mul_ps(a,b)); }
static sINLINE sSSE sVecRcp(sSSE a)                     { return _mm_rcp_ps(a); }
static sINLINE sSSE sVecRSqrt(sSSE a)                   { return _mm_rsqrt_ps(a); }
static sINLINE sSSE sVecMax(sSSE a,sSSE b)              { return _mm_max_ps(a,b); }
static sINLINE sSSE sVecMin(sSSE a,sSSE b)              { return _mm_min_ps(a,b); }

// ---- Dot products

static sINLINE sSSE sVecDot3(sSSE a,sSSE b)             { sSSE p = sVecMul(a,b); return sVecAdd(sVecAdd(sVecSplat(p,0),sVecSplat(p,1)),sVecSplat(p,2)); }
static sINLINE sSSE sVecDot4(sSSE a,sSSE b)
{
  sSSE p = sVecMul(a,b);              // componentwise products
  sSSE shuf = sVecShuffle(p,2,3,0,1); // p.z,p.w,p.x,p.y
  sSSE sum = sVecAdd(p,shuf);         // p.x+p.z,p.y+p.w,p.z+p.x,p.w+p.y
  return sVecAdd(sVecSplat(sum,0),sVecSplat(sum,1));
}

// ---- Comparison operators

static sINLINE sSSE sVecCmpEQ(sSSE a,sSSE b)            { return _mm_cmpeq_ps(a,b); }
static sINLINE sSSE sVecCmpGT(sSSE a,sSSE b)            { return _mm_cmpgt_ps(a,b); }
static sINLINE sSSE sVecCmpGE(sSSE a,sSSE b)            { return _mm_cmpge_ps(a,b); }
static sINLINE sSSE sVecCmpLT(sSSE a,sSSE b)            { return _mm_cmpgt_ps(b,a); }
static sINLINE sSSE sVecCmpLE(sSSE a,sSSE b)            { return _mm_cmpge_ps(b,a); }

static sINLINE sBool sVecAnyEQ(sSSE a,sSSE b)           { return _mm_movemask_ps(_mm_cmpeq_ps(a,b)) != 0; }
static sINLINE sBool sVecAnyGT(sSSE a,sSSE b)           { return _mm_movemask_ps(_mm_cmpgt_ps(a,b)) != 0; }
static sINLINE sBool sVecAnyGE(sSSE a,sSSE b)           { return _mm_movemask_ps(_mm_cmpge_ps(a,b)) != 0; }
static sINLINE sBool sVecAnyLT(sSSE a,sSSE b)           { return _mm_movemask_ps(_mm_cmpgt_ps(b,a)) != 0; }
static sINLINE sBool sVecAnyLE(sSSE a,sSSE b)           { return _mm_movemask_ps(_mm_cmpge_ps(b,a)) != 0; }
                                             
static sINLINE sBool sVecAllEQ(sSSE a,sSSE b)           { return _mm_movemask_ps(_mm_cmpeq_ps(a,b)) == 0xf; }
static sINLINE sBool sVecAllGT(sSSE a,sSSE b)           { return _mm_movemask_ps(_mm_cmpgt_ps(a,b)) == 0xf; }
static sINLINE sBool sVecAllGE(sSSE a,sSSE b)           { return _mm_movemask_ps(_mm_cmpge_ps(a,b)) == 0xf; }
static sINLINE sBool sVecAllLT(sSSE a,sSSE b)           { return _mm_movemask_ps(_mm_cmpgt_ps(b,a)) == 0xf; }
static sINLINE sBool sVecAllLE(sSSE a,sSSE b)           { return _mm_movemask_ps(_mm_cmpge_ps(b,a)) == 0xf; }

// ---- Conversion

static sINLINE sSSE sVecInt2Float(sSSE a)
{
#if sSIMD_SSE2
  // SSE2 makes this easy
  return _mm_cvtepi32_ps(*(__m128i*) &a);
#else
  // SSE1, on the other hand, is pretty retarded in this regard
  __m64 t0,t1;
  _mm_storel_pi(&t0,a);
  _mm_storeh_pi(&t1,a);
  sSSE s0 = _mm_cvt_pi2ps(_mm_setzero_ps(),t0);
  sSSE s1 = _mm_cvt_pi2ps(_mm_setzero_ps(),t1);
  sSSE res = _mm_movelh_ps(s0,s1);
  _mm_empty();
  return res;
#endif
}

static sINLINE sSSE sVecFloat2Int(sSSE a)
{
#if sSIMD_SSE2
  __m128i ai = _mm_cvtps_epi32(a);
  return *(sSSE*) &ai;
#else
  __m64 t0 = _mm_cvttps_pi32(a);
  __m64 t1 = _mm_cvttps_pi32(_mm_movehl_ps(a,a));

  a = _mm_loadl_pi(a,&t0);
  a = _mm_loadh_pi(a,&t1);
  _mm_empty();
  return a;
#endif
}

#define sVecInt2FloatScale(a,exp)                       _mm_mul_ps(sVecInt2Float(a),_mm_set1_ps(1.0f / (1<<(exp))))
#define sVecFloat2IntScale(a,exp)                       sVecFloat2Int(_mm_mul_ps(a,_mm_set1_ps((sF32) (1<<(exp)))))

// ---- Mask building

static sINLINE sInt sVecMask(sSSE v)                    { return _mm_movemask_ps(v); }

#else

#define sSIMD_INTRINSICS 0

#endif

/****************************************************************************/

#endif // FILE_UTIL_SIMD_FLOAT_HPP


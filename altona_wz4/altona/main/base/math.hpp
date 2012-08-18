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

#ifndef HEADER_ALTONA_BASE_MATH
#define HEADER_ALTONA_BASE_MATH

#ifndef __GNUC__
#pragma once
#endif

#include "base/types.hpp"
#include "base/serialize.hpp"

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   3d graphics primitives                                             ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

class sVector2;
class sVector30;
class sVector31;
class sVector4;
class sMatrix34;
class sMatrix34CM;
class sMatrix44;
class sQuaternion;
class sAABBox;
class sAABBoxC;
class sRay;
class sComplex;
class sFrustum;

struct sDontInitializeT {};
extern sDontInitializeT sDontInitialize;

const sF32 sEPSILON = 0.000000001f;

enum
{
  sTEST_OUT  = 0,
  sTEST_CLIP = 1,
  sTEST_IN   = 2, 
};

#if 0                             // on some platforms passing by value is faster than passing by reference....
  typedef sVector30 sVector30Arg;
  typedef sVector31 sVector31Arg;
  typedef sVector4 sVector4Arg;
  typedef sVector2 sVector2Arg;
  typedef sQuaternion sQuaternionArg;
#else
  typedef const sVector30 &sVector30Arg;
  typedef const sVector31 &sVector31Arg;
  typedef const sVector4 &sVector4Arg;
  typedef const sVector2 &sVector2Arg;
  typedef const sQuaternion &sQuaternionArg;
#endif 

/****************************************************************************/
/***                                                                      ***/
/***   Equation solving                                                   ***/
/***                                                                      ***/
/****************************************************************************/

// Finds real roots (=Nullstellen) of at² + bt + c, puts them into t,
// and returns how many there are. Not very fast, but handles all
// degenerate cases and is quite accurate.
sInt sSolveQuadratic(sF32 t[],sF32 a,sF32 b,sF32 c);

/****************************************************************************/
/***                                                                      ***/
/***   Numerical integration                                              ***/
/***                                                                      ***/
/****************************************************************************/

typedef sF64 (*sIntegrand)(sF64 x,void *user);

// Integrate f using Romberg's method. You specify the interval [a,b]
// over which integration is to take place, as well as a maximum order
// (every additional order doubles the maximum number of function
// evaluations) and a desired maximum error.
//
// f should be reasonably smooth or convergence will be slow.
//
// Warning: This implementation is not necessarily safe to use with
// certain periodic functions; it may severaly underestimate the error sometimes.
sF64 sRombergIntegral(sIntegrand f,void *user,sF64 a,sF64 b,sInt maxOrder,sF64 maxError);

/****************************************************************************/
/***                                                                      ***/
/***   Vector                                                             ***/
/***                                                                      ***/
/****************************************************************************/

class sVector2
{
public:
  enum { Dimensions = 2 };

  sF32 x,y;

  sVector2()                                      { x=y=0.0f; }
  explicit sVector2(const sDontInitializeT&)      {}
  sVector2(sF32 xx,sF32 yy)                       { x=xx;y=yy; }
  sVector2(const sFRect &rect)                    { x = rect.SizeX(); y = rect.SizeY(); }
  void Init(sF32 xx=0,sF32 yy=0)                  { x=xx;y=yy; }
  sVector2(const sVector2 &a)                         { x=a.x; y=a.y; }
  sVector2& operator= (sVector2Arg a)             { x=a.x; y=a.y; return *this; }

  void Average(sVector2Arg a,sVector2Arg b)       { x=(a.x+b.x)*0.5f; y=(a.y+b.y)*0.5f; }
  void Fade(sF32 f,sVector2Arg a,sVector2Arg b)   { x=a.x+(b.x-a.x)*f; y=a.y+(b.y-a.y)*f; }
  void Neg()                                      { x = -x; y = -y; }

  void UnitPrecise()                              { sF32 e=sRSqrt(x*x+y*y); x*=e; y*=e; }
  sOBSOLETE void UnitSlow()                       { sF32 e=sRSqrt(x*x+y*y); x*=e; y*=e; }   // UnitPrecise sounds better the UnitSlow
  void UnitFast()                                 { sF32 e=sFRSqrt(x*x+y*y); x*=e; y*=e; }

  sF32 &operator[](sInt i)                        { return (&x)[i]; }
  const sF32 &operator[](sInt i) const            { return (&x)[i]; }

  sF32 Length() const                             { return (sF32)sFSqrt(x*x+y*y); }
  sF32 LengthPrecise() const                      { return (sF32)sSqrt(x*x+y*y); }  
  sF32 LengthSq() const                           { return x*x+y*y; }

  operator sFRect() const                         { return sFRect(x,y); }
  inline sBool operator==(sVector2Arg v) const    { return x==v.x && y==v.y; }
  inline sBool operator!=(sVector2Arg v) const    { return x!=v.x || y!=v.y; }
};

class sVector31
{
public:
  enum { Dimensions = 3 };

  sF32 x,y,z;

  sVector31()                                       { x=y=z=0; }
  explicit sVector31(const sDontInitializeT&)       {}
  explicit sVector31(sF32 xyz)                      { x=y=z=xyz; }
  sVector31(sF32 xx,sF32 yy,sF32 zz)                { x=xx;y=yy;z=zz; }
  void Init(sF32 xx,sF32 yy,sF32 zz)                { x=xx;y=yy;z=zz; }
  sVector31(const sVector31 &a)                     { x=a.x; y=a.y; z=a.z; }
  explicit sVector31(const sVector30 &a);
  explicit sVector31(const sVector4 &a);
  inline sVector31& operator= (sVector31Arg a)      { x=a.x; y=a.y; z=a.z; return *this; }

  void InitColor(sU32 col);
  void InitColor(sU32 col,sF32 amp);
  sU32 GetColor() const;
  inline void Fade(sF32 f,sVector31Arg a,sVector31Arg b)  { x=a.x+(b.x-a.x)*f; y=a.y+(b.y-a.y)*f; z=a.z+(b.z-a.z)*f; }
  inline void Average(sVector31Arg a,sVector31Arg b)      { x=(a.x+b.x)*0.5f; y=(a.y+b.y)*0.5f; z=(a.z+b.z)*0.5f;  }
  template <class RND> void InitRandom(RND &r)            { do { x=r.Float(2)-1; y=r.Float(2)-1; z=r.Float(2)-1; } while(x*x+y*y+z*z>1); }
  inline void Neg()                                       { x = -x; y = -y, z = -z; }

  inline sF32 &operator[](sInt i)                   { return (&x)[i]; }
  inline const sF32 &operator[](sInt i) const       { return (&x)[i]; }
  inline sBool operator==(sVector31Arg v)const	    { return x==v.x && y==v.y && z==v.z; }
  inline sBool operator!=(sVector31Arg v)const	    { return x!=v.x || y!=v.y || z!=v.z; }
};

class sVector30
{
public:
  enum { Dimensions = 3 };

  sF32 x,y,z;

  sVector30()                                       { x=y=z=0; }
  explicit sVector30(const sDontInitializeT&)       {}
  explicit sVector30(sF32 xyz)                      { x=y=z=xyz; }
  sVector30(sF32 xx,sF32 yy,sF32 zz)                { x=xx;y=yy;z=zz; }
  void Init(sF32 xx,sF32 yy,sF32 zz)                { x=xx;y=yy;z=zz; }
  sVector30(const sVector30 &a)                     { x=a.x; y=a.y; z=a.z; }
  sVector30& operator= (sVector30Arg a)             { x=a.x; y=a.y; z=a.z; return *this; }
  explicit sVector30(const sVector31 &a)            { x=a.x; y=a.y; z=a.z; }
  explicit sVector30(const sVector4 &a);

  void InitColor(sU32 col);
  void InitColor(sU32 col,sF32 amp);
  sU32 GetColor() const;
  inline void Fade(sF32 f,sVector30Arg a,sVector30Arg b)  { x=a.x+(b.x-a.x)*f; y=a.y+(b.y-a.y)*f; z=a.z+(b.z-a.z)*f; }
  inline void Average(sVector30Arg a,sVector30Arg b)      { x=(a.x+b.x)*0.5f; y=(a.y+b.y)*0.5f; z=(a.z+b.z)*0.5f;  }
  template <class RND> void InitRandom(RND &r)            { do { x=r.Float(2)-1; y=r.Float(2)-1; z=r.Float(2)-1; } while(x*x+y*y+z*z>1); }
  inline sF32 Length() const                              { return (sF32)sFSqrt(x*x+y*y+z*z); }
  inline sF32 LengthPrecise() const                       { return (sF32)sSqrt(x*x+y*y+z*z); }  
  inline sF32 LengthSq() const                            { return x*x+y*y+z*z; }
  sF32 Angle(sVector30Arg a) const;

  void Cross(sVector30Arg a,sVector30Arg b);
  // returns cross product of this cross v
  sVector30 Cross(sVector30Arg v) const { return sVector30(y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x); }
  void Rotate(const sMatrix34 &m,sVector30Arg v);

  void Unit();
  inline void UnitFast()                            { sF32 e=sFRSqrt(x*x+y*y+z*z); x*=e; y*=e; z*=e; } 
  void UnitPrecise();
  // Returns vector normalised (<1,0,0> when length<<1)
  inline sVector30 GetUnit() const                  { sVector30 n = *this; n.Unit(); return n; }
  inline void Neg()                                 { x = -x; y = -y, z = -z; }
  inline sInt MaxAxisAbs() const                    { sF32 ax = sFAbs(x); sF32 ay = sFAbs(y); sF32 az = sFAbs(z); return (ax>ay) ? ((ax>az) ? 0 : 2) : ((ay>az) ? 1 : 2); }
  // Componentwise reciprocal (useful for e.g. ray-plane or ray-box tests)
  void Reciprocal(sVector30Arg a)                 { x = 1.0f / a.x; y = 1.0f / a.y; z = 1.0f / a.z; }

  // Componentwise reciprocal that never divides by zero (1/0 returns 0)
  void ReciprocalKeepZero(sVector30Arg a)         { x = a.x ? (1.0f / a.x) : 0.0f; y = a.y ? (1.0f / a.y) : 0.0f; z = a.z ? (1.0f / a.z) : 0.0f; }
  
  inline sF32 &operator[](sInt i)                 { return (&x)[i]; }
  inline const sF32 &operator[](sInt i) const     { return (&x)[i]; }
  inline sBool operator==(sVector30Arg v)const	  { return x==v.x && y==v.y && z==v.z; }
  inline sBool operator!=(sVector30Arg v)const	  { return x!=v.x || y!=v.y || z!=v.z; }
};

class sVector4
{
public:
  enum { Dimensions = 4 };

  sF32 x,y,z,w;

  sVector4()                                        { x=y=z=w=0; }
  explicit sVector4(const sDontInitializeT&)        {}
  sVector4(sF32 xx,sF32 yy,sF32 zz,sF32 ww)         { x=xx; y=yy; z=zz; w=ww; }
  void Init(sF32 xx,sF32 yy,sF32 zz,sF32 ww)        { x=xx; y=yy; z=zz; w=ww; }
  sVector4(const sVector30 &a)                      { x=a.x; y=a.y; z=a.z; w=0; }
  sVector4(const sVector31 &a)                      { x=a.x; y=a.y; z=a.z; w=1; }
  sVector4(const sVector4 &a)                       { x=a.x; y=a.y; z=a.z; w=a.w; }
  sVector4& operator= (sVector30Arg a)              { x=a.x; y=a.y; z=a.z; w=0; return *this; }
  sVector4& operator= (sVector31Arg a)              { x=a.x; y=a.y; z=a.z; w=1; return *this; }
  sVector4& operator= (sVector4Arg a)               { x=a.x; y=a.y; z=a.z; w=a.w; return *this; }

  void InitColor(sU32 col);
  void InitColor(sU32 col,sF32 amp);
  sU32 GetColor() const;
  inline void Average(sVector4Arg a,sVector4Arg b) { x=(a.x+b.x)*0.5f; y=(a.y+b.y)*0.5f; z=(a.z+b.z)*0.5f; w=(a.w+b.w)*0.5f;  }

  void InitMRGB8(sU32 col);
  sU32 GetMRGB8()const;
  void InitMRGB16(sU64 col);
  sU64 GetMRGB16()const;

  void InitPlane(sVector31Arg pos,sVector30Arg norm);
  void InitPlane(sVector31Arg p0,sVector31Arg p1,sVector31Arg p2);
  inline void Neg0()                                        { x = -x; y = -y, z = -z; }
  inline void Neg1()                                        { x = -x; y = -y, z = -z; w=-w;}
  void Unit4();
  void Unit4Precise();

  inline sF32 &operator[](sInt i)                 { return (&x)[i]; }
  inline const sF32 &operator[](sInt i) const     { return (&x)[i]; }

  sBool operator==(sVector4Arg v)const;
  inline sBool operator!=(sVector4Arg v)const     { return x!=v.x||y!=v.y||z!=v.z; }
};

inline sU32 sMakeColorRGBA(sF32 *p) { sVector4 c(p[0],p[1],p[2],p[3]); return c.GetColor(); }

inline sVector2  operator+ (sVector2Arg a,sVector2Arg b)              { return sVector2 (a.x+b.x,a.y+b.y); }
inline sVector30 operator+ (sVector30Arg a,sVector30Arg b)            { return sVector30(a.x+b.x,a.y+b.y,a.z+b.z); }
inline sVector31 operator+ (sVector31Arg a,sVector30Arg b)            { return sVector31(a.x+b.x,a.y+b.y,a.z+b.z); }
inline sVector31 operator+ (sVector30Arg a,sVector31Arg b)            { return sVector31(a.x+b.x,a.y+b.y,a.z+b.z); }
inline sVector4  operator+ (const sVector4  &a,const sVector4  &b)    { return sVector4 (a.x+b.x,a.y+b.y,a.z+b.z,a.w+b.w); }

inline sVector2  operator- (sVector2Arg a,sVector2Arg b)              { return sVector2 (a.x-b.x,a.y-b.y); }
inline sVector30 operator- (sVector30Arg a,sVector30Arg b)            { return sVector30(a.x-b.x,a.y-b.y,a.z-b.z); }
inline sVector31 operator- (sVector31Arg a,sVector30Arg b)            { return sVector31(a.x-b.x,a.y-b.y,a.z-b.z); }
inline sVector30 operator- (sVector31Arg a,sVector31Arg b)            { return sVector30(a.x-b.x,a.y-b.y,a.z-b.z); }
inline sVector4  operator- (const sVector4  &a,const sVector4  &b)    { return sVector4 (a.x-b.x,a.y-b.y,a.z-b.z,a.w-b.w); }

inline sVector2  operator- (sVector2Arg a)                            { return sVector2(-a.x,-a.y); }
inline sVector30 operator- (sVector30Arg a)                           { return sVector30(-a.x,-a.y,-a.z); }
inline sVector31 operator- (sVector31Arg a)                           { return sVector31(-a.x,-a.y,-a.z); }
inline sVector4  operator- (const sVector4  &a)                       { return sVector4(-a.x,-a.y,-a.z,-a.w); }

inline sVector2  operator* (const sVector2  &a,sF32 s)                { return sVector2 (a.x*s,a.y*s); }
inline sVector30 operator* (sVector30Arg a,sF32 s)                    { return sVector30(a.x*s,a.y*s,a.z*s); }
inline sVector4  operator* (const sVector4  &a,sF32 s)                { return sVector4 (a.x*s,a.y*s,a.z*s,a.w*s); }

inline sVector2  operator* (sF32 s, sVector2Arg a)                    { return sVector2 (s*a.x,s*a.y); }
inline sVector30 operator* (sF32 s, sVector30Arg a)                   { return sVector30(a.x*s,a.y*s,a.z*s); }
inline sVector4  operator* (sF32 s, const sVector4  &a)               { return sVector4 (a.x*s,a.y*s,a.z*s,a.w*s); }
inline sVector30 operator* (sVector30Arg a, sVector30Arg b)           { return sVector30(a.x*b.x,a.y*b.y,a.z*b.z); }
inline sVector30 operator* (sVector30Arg a, sVector31Arg b)           { return sVector30(a.x*b.x,a.y*b.y,a.z*b.z); }
inline sVector30 operator* (sVector31Arg a, sVector30Arg b)           { return sVector30(a.x*b.x,a.y*b.y,a.z*b.z); }
inline sVector31 operator* (sVector31Arg a, sVector31Arg b)           { return sVector31(a.x*b.x,a.y*b.y,a.z*b.z); }
inline sVector4  operator* (const sVector4  &a, const sVector4  &b)   { return sVector4 (a.x*b.x,a.y*b.y,a.z*b.z,a.w*b.w); }

inline sVector2  operator/ (sVector2Arg a, sF32 s)                    { s = 1.0f/s; return sVector2 (a.x*s,a.y*s); }
inline sVector30 operator/ (sVector30Arg a,sF32 s)                    { s = 1.0f/s; return sVector30(a.x*s,a.y*s,a.z*s); }
inline sVector4  operator/ (const sVector4  &a,sF32 s)                { s = 1.0f/s; return sVector4 (a.x*s,a.y*s,a.z*s,a.w*s); }

inline sVector30 operator% (sVector30Arg a, sVector30Arg b)     { sVector30 r; r.Cross(a,b); return r;}

inline sF32 operator^ (sVector2Arg a,sVector2Arg b)         { return a.x*b.x + a.y*b.y; }
inline sF32 operator^ (sVector30Arg a,sVector30Arg b)       { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline sF32 operator^ (sVector30Arg a,sVector31Arg b)       { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline sF32 operator^ (sVector31Arg a,sVector30Arg b)       { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline sF32 operator^ (sVector30Arg a,sVector4Arg b)        { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline sF32 operator^ (sVector31Arg a,sVector4Arg b)        { return a.x*b.x + a.y*b.y + a.z*b.z + b.w; }
inline sF32 operator^ (sVector4Arg a,sVector30Arg b)        { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline sF32 operator^ (sVector4Arg a,sVector31Arg b)        { return a.x*b.x + a.y*b.y + a.z*b.z + a.w; }
inline sF32 operator^ (sVector4Arg a,sVector4Arg b)         { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }

inline sVector2&  operator+= (sVector2 &a,sVector2Arg b)        { a = a + b; return a; }
inline sVector30& operator+= (sVector30 &a,sVector30Arg b)      { a = a + b; return a; }
inline sVector31& operator+= (sVector31 &a,sVector30Arg b)      { a = a + b; return a; }
inline sVector4&  operator+= (sVector4  &a,const sVector4  &b)  { a = a + b; return a; }
inline sVector2&  operator-= (sVector2 &a,sVector2Arg b)        { a = a - b; return a; }
inline sVector30& operator-= (sVector30 &a,sVector30Arg b)      { a = a - b; return a; }
inline sVector31& operator-= (sVector31 &a,sVector30Arg b)      { a = a - b; return a; }
inline sVector4&  operator-= (sVector4  &a,const sVector4  &b)  { a = a - b; return a; }
inline sVector2&  operator*= (sVector2 &a,sF32 s)               { a = a * s; return a; }
inline sVector30& operator*= (sVector30 &a,sF32 s)              { a = a * s; return a; }
inline sVector4&  operator*= (sVector4  &a,sF32 s)              { a = a * s; return a; }
inline sVector4&  operator*= (sVector4  &a,const sVector4  &b)  { a = a * b; return a; }
inline sVector2&  operator/= (sVector2  &a,sF32 s)              { a = a / s; return a; }
inline sVector30& operator/= (sVector30 &a,sF32 s)              { a = a / s; return a; }
inline sVector4&  operator/= (sVector4  &a,sF32 s)              { a = a / s; return a; }


inline sF32 sOBSOLETE dot(sVector30Arg a,sVector30Arg b)          { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline sF32 sOBSOLETE dot(sVector30Arg a,sVector31Arg b)          { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline sF32 sOBSOLETE dot(sVector31Arg a,sVector30Arg b)          { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline sF32 sOBSOLETE dot(sVector4Arg a,sVector31Arg b)           { return a.x*b.x + a.y*b.y + a.z*b.z + a.w; }
inline sF32 sOBSOLETE dot(sVector31Arg a,sVector4Arg b)           { return a.x*b.x + a.y*b.y + a.z*b.z + b.w; }
inline sF32 sOBSOLETE dot(sVector4Arg a,sVector30Arg b)           { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline sF32 sOBSOLETE dot(sVector30Arg a,sVector4Arg b)           { return a.x*b.x + a.y*b.y + a.z*b.z; }

inline sVector30::sVector30(const sVector4 &a)  { x=a.x; y=a.y; z=a.z; }
inline sVector31::sVector31(const sVector30 &a) { x=a.x; y=a.y; z=a.z; }
inline sVector31::sVector31(const sVector4 &a)  { x=a.x; y=a.y; z=a.z; }

sFormatStringBuffer& operator% (sFormatStringBuffer &,sVector2Arg );
sFormatStringBuffer& operator% (sFormatStringBuffer &,const sVector30 &);
sFormatStringBuffer& operator% (sFormatStringBuffer &,const sVector31 &);
sFormatStringBuffer& operator% (sFormatStringBuffer &,const sVector4 &);

inline sVector2  sAverage(sVector2Arg a,sVector2Arg b)    { return sVector2((a.x+b.x)*0.5f,(a.y+b.y)*0.5f); }
inline sVector30 sAverage(sVector30Arg a,sVector30Arg b)  { return sVector30((a.x+b.x)*0.5f,(a.y+b.y)*0.5f,(a.z+b.z)*0.5f); }
inline sVector31 sAverage(sVector31Arg a,sVector31Arg b)  { return sVector31((a.x+b.x)*0.5f,(a.y+b.y)*0.5f,(a.z+b.z)*0.5f); }

inline sVector2  sUnitFast(sVector2Arg v)  { return v*sFRSqrt(v^v); }
inline sVector30 sUnitFast(sVector30Arg v) { return v*sFRSqrt(v^v); }
inline sVector4  sUnitFast(sVector4Arg v)  { return v*sFRSqrt(v^v); }

inline sWriter& operator| (sWriter &s,sVector2Arg a)  { s | a.x | a.y;              return s; }
inline sReader& operator| (sReader &s,      sVector2 &a)  { s | a.x | a.y;              return s; }
inline sWriter& operator| (sWriter &s,sVector30Arg a) { s | a.x | a.y | a.z;        return s; }
inline sReader& operator| (sReader &s,      sVector30 &a) { s | a.x | a.y | a.z;        return s; }
inline sWriter& operator| (sWriter &s,sVector31Arg a) { s | a.x | a.y | a.z;        return s; }
inline sReader& operator| (sReader &s,      sVector31 &a) { s | a.x | a.y | a.z;        return s; }
inline sWriter& operator| (sWriter &s,sVector4Arg a) { s | a.x | a.y | a.z | a.w;  return s; }
inline sReader& operator| (sReader &s,      sVector4  &a) { s | a.x | a.y | a.z | a.w;  return s; }

/****************************************************************************/
/***                                                                      ***/
/***   Quaternion                                                         ***/
/***                                                                      ***/
/****************************************************************************/

class sQuaternion
{
public:
  sF32 r,i,j,k;

  sQuaternion()                              { i=j=k=0; r=1; }
  sQuaternion(sF32 R,sF32 I,sF32 J,sF32 K)   { r=R;i=I;j=J;k=K; }
  sQuaternion(sVector30Arg v, sF32 a)        { sF32 si; sFSinCos(a/2,si,r); i=si*v.x; j=si*v.y; k=si*v.z; } // rotation around axis v, v must be unit length, angle in radians
  void Init(sF32 R,sF32 I,sF32 J,sF32 K)     { r=R;i=I;j=J;k=K; }

  void Unit();

  void Lerp(sF32 fade,sQuaternionArg a,sQuaternionArg b);
  sOBSOLETE void Slerp(sF32 fade,sQuaternionArg a,sQuaternionArg b);
  void FastSlerp(sF32 fade,sQuaternionArg a,sQuaternionArg b); // spline approximated slerp
  inline void Fade(sF32 fade,sQuaternionArg a,sQuaternionArg b) { Lerp(fade,a,b); Unit(); } // also called "nlerp"

  void Init(const sMatrix34 &);
  void Init(sVector30Arg axis, sF32 angle); // axis must be unit length

  void Euler(sF32 h, sF32 p, sF32 b);
  inline void Euler(sVector30Arg v) { Euler(v.x,v.y,v.z); }
  sVector30 GetEuler() const;
  void GetAxisAngle(sVector30 &axis, sF32 &angle);

  void MakeRotation(sVector30Arg v1, sVector30Arg v2); // creates a minimal rotation that
                                                               // maps v1 to v2 
  void Invert();
  void Invert(sQuaternionArg from);
  void InvertNorm();      // fast invert for unit quaternions

  inline sQuaternion Conjugate() const { return sQuaternion(r,-i,-j,-k); }

  inline sF32 GetNorm() const { return sSqrt(GetNormSq()); }
  inline sF32 GetNormSq() const { return r*r + i*i + j*j + k*k; }

  inline sF32 &operator[](sInt i)                 { return (&r)[i]; }
  inline const sF32 &operator[](sInt i) const     { return (&r)[i]; }
};

sQuaternion operator* (sQuaternionArg a,sQuaternionArg b);
sVector30 operator* (sVector30Arg a,sQuaternionArg b);
sVector31 operator* (sVector31Arg a,sQuaternionArg b);

inline sQuaternion& operator*= (sQuaternion &a,sQuaternionArg b)  { a=a*b; return a; }
inline sVector30& operator*= (sVector30 &a,sQuaternionArg b)      { a=a*b; return a; }
inline sVector31& operator*= (sVector31 &a,sQuaternionArg b)      { a=a*b; return a; }
inline sQuaternion operator* (sF32 s,sQuaternionArg q) { return sQuaternion(s*q.r,s*q.i,s*q.j,s*q.k); }
inline sQuaternion operator* (sQuaternionArg q,sF32 s) { return sQuaternion(s*q.r,s*q.i,s*q.j,s*q.k); }
inline sQuaternion operator+ (sQuaternionArg a,sQuaternionArg b)  { return sQuaternion(a.r+b.r,a.i+b.i,a.j+b.j,a.k+b.k); }
inline sQuaternion& operator+= (sQuaternion &a,sQuaternionArg b)  { a=a+b; return a; }

inline sQuaternion operator~ (sQuaternionArg q) { sQuaternion res(q); res.Invert(); return res; }

inline sF32 dot(sQuaternionArg a,sQuaternionArg b)          { return a.r*b.r + a.i*b.i + a.j*b.j + a.k*b.k; }
inline sF32 operator^ (sQuaternionArg a, sQuaternionArg b)  { return a.r*b.r + a.i*b.i + a.j*b.j + a.k*b.k; }

sFormatStringBuffer& operator% (sFormatStringBuffer &, sQuaternionArg );

inline sQuaternion sUnitFast(sQuaternionArg q)  { return q*sFRSqrt(q^q); }


inline sWriter& operator| (sWriter &s,sQuaternionArg a) { s | a.r | a.i | a.j | a.k;  return s; }
inline sReader& operator| (sReader &s,      sQuaternion &a) { s | a.r | a.i | a.j | a.k;  return s; }

/****************************************************************************/
/***                                                                      ***/
/***   Matrix                                                             ***/
/***                                                                      ***/
/****************************************************************************/

class sMatrix34
{
public:
  sVector30 i;
  sVector30 j;
  sVector30 k;
  sVector31 l;

  sINLINE sMatrix34() : i(1,0,0),j(0,1,0),k(0,0,1),l(0,0,0) {}
  explicit sMatrix34(const sDontInitializeT&) : i(sDontInitialize),j(sDontInitialize),k(sDontInitialize),l(sDontInitialize) {}
  sMatrix34(sVector30Arg _i, sVector30Arg _j, sVector30Arg _k, sVector31Arg _l) : i(_i), j(_j), k(_k), l(_l) {}
  explicit sMatrix34(const sMatrix44 &m);
  explicit sMatrix34(const sMatrix34CM &m);

  explicit sMatrix34(sQuaternionArg q) { Init(q); }
  explicit sMatrix34(sQuaternionArg q, sVector31Arg v) { Init(q, v); }

  sMatrix34 &operator=(const sMatrix34CM &m);

  // initialize matrix

  void Init();
  void sOBSOLETE Euler(sF32 a,sF32 b,sF32 c);
  void EulerXYZ(sF32 x,sF32 y,sF32 z);
  void EulerXYZ(sVector30Arg xyz) { EulerXYZ(xyz.x, xyz.y, xyz.z); }
  void FindEulerXYZ(sF32 &x,sF32 &y,sF32 &z) const;
  void FindEulerXYZ2(sF32 &x,sF32 &y,sF32 &z) const;
  void CubeFace(sInt face);         // set up camera matrix for rendering to cube face
  void Look(sVector30Arg v);
  void Look(sVector30Arg dir, sVector30Arg up);  // this is wrong. is it used anywhere? -> (it is not wrong but less stable, and currently used a lot)
  void LookPrecise(sVector30Arg v);
  void LookPrecise(sVector30Arg dir, sVector30Arg up);  // this is wrong. is it used anywhere? -> (it is not wrong but less stable, and currently used a lot)
  void Look_(sVector30Arg dir, sVector30Arg up); // corrected version.
  void LookAt(sVector31Arg dest, sVector31Arg pos);
  void LookAt(sVector31Arg dest, sVector31Arg pos, sVector30Arg up);
  void Cross(sVector30Arg v);
  void FadeOrthonormal(sF32 f,const sMatrix34 &mat0,const sMatrix34 &mat1);
  void FadeOrthonormalPrecise(sF32 f,const sMatrix34 &mat0,const sMatrix34 &mat1);
  void Fade(sF32 f,const sMatrix34 &mat0,const sMatrix34 &mat1);

  // manipulate matrix

  void RotateAxis(sVector30Arg v,sF32 angle); // this should initialize L. check summergames.
  void Trans3();
  void TransR();            // works as a full inverse for rigid body transforms (rotate+translate only)
  void InvertOrthogonal();  // invert a matrix that is Orthogonal but not Orthonormal
  sF32 Determinant3x3() const;    // calculate determinant of 3x3 part of matrix

  sBool Invert3();
  sBool Invert34();

  void Orthonormalize();    // Gram-Schmidt orthonormalization, doesn't touch l
  void Init(sQuaternionArg , sVector31Arg );
  void Init(sQuaternionArg );
  void Scale(sF32 x, sF32 y, sF32 z);
  void Scale(sF32 s) { Scale(s,s,s); }
  void ThreePoint(const sVector31 &p0,const sVector31 & p1,const sVector31 & p2,const sVector30 & tweak);  // buggy
  void ThreePoint_(const sVector31 &p0,const sVector31 &p1,const sVector31 &p2,const sVector30 &tweak); // better

  void RotateAroundPivot(sQuaternionArg rot,sVector31Arg pivot);
  void RotateAroundPivot(sVector30Arg axis,sF32 angle,sVector31Arg pivot);

  // convenience stuff. don't use in time-critical code!
  sMatrix34 Transpose3() const  { sMatrix34 ret = *this; ret.Trans3(); return ret; }
  sMatrix34 TransposeR() const  { sMatrix34 ret = *this; ret.TransR(); return ret; }
  sMatrix34 Inverse3() const    { sMatrix34 ret = *this; ret.Invert3(); return ret; }
  sMatrix34 Inverse() const     { sMatrix34 ret = *this; ret.Invert34(); return ret; }
};

// column major stored sMatrix34 matrix for shaders
// use only for storage! don't define operations, use sMatrix34 instead
class sMatrix34CM
{
public:
  sVector4 x,y,z;  // each sVector4 represents the x or y or z of i and j and k and l

  sINLINE sMatrix34CM() {}
  explicit sMatrix34CM(const sMatrix34 &m);

  sMatrix34CM &operator=(const sMatrix34 &m);
  void Ident();
  sINLINE void Init() { Ident(); }

  void ConvertTo(sMatrix34 &m)const;

  sF32 Determinant3x3() const;    // calculate determinant of 3x3 part of matrix
};

class sMatrix44
{
public:
  sVector4 i;
  sVector4 j;
  sVector4 k;
  sVector4 l;

  void Init();
  sMatrix44() : i(1,0,0,0),j(0,1,0,0),k(0,0,1,0),l(0,0,0,1) {}
  explicit sMatrix44(const sDontInitializeT&) : i(sDontInitialize), j(sDontInitialize), k(sDontInitialize), l(sDontInitialize) {}
  sMatrix44(const sMatrix34 &m) : i(m.i),j(m.j),k(m.k),l(m.l) {}
  sMatrix44(const sMatrix34CM &m) : i(m.x.x,m.y.x,m.z.x,0),j(m.x.y,m.y.y,m.z.y,0),k(m.x.z,m.y.z,m.z.z,0),l(m.x.w,m.y.w,m.z.w,1) {}
  sMatrix44(sVector4Arg _i, sVector4Arg _j, sVector4Arg _k, sVector4Arg _l) : i(_i), j(_j), k(_k), l(_l) {}
  void Trans4(const sMatrix44 &mat);
  void Trans4();
  void Invert(const sMatrix44 &mat);
  void Invert()                                         { sMatrix44 mat(*this); Invert(mat); }
  sF32 Determinant() const;

  sBool operator==(const sMatrix44 &mat) const;
  sBool operator!=(const sMatrix44 &mat) const          { return !(*this == mat); }

  void Scale(sF32 x, sF32 y, sF32 z);
  void Orthonormalize();    // Gram-Schmidt orthonormalization

  void Perspective(sF32 left, sF32 right, sF32 top, sF32 bottom, sF32 front, sF32 back);

  sVector4 &operator[](sInt a)                          { return (&i)[a]; }
  sVector4Arg operator[](sInt a) const                  { return (&i)[a]; }
};

inline sMatrix34::sMatrix34(const sMatrix44 &m) : i(sVector30(m.i)),j(sVector30(m.j)),k(sVector30(m.k)),l(sVector31(m.l)) {}

sFormatStringBuffer& operator% (sFormatStringBuffer &,const sMatrix34 &);
sFormatStringBuffer& operator% (sFormatStringBuffer &,const sMatrix44 &);
sFormatStringBuffer& operator% (sFormatStringBuffer &,const sMatrix34CM &);

/****************************************************************************/

sMatrix44 operator* (const sMatrix44 &a,const sMatrix44 &b);
sMatrix34 operator* (const sMatrix34 &a,const sMatrix34 &b);
sMatrix34CM operator* (const sMatrix34CM &a,const sMatrix34CM &b);

sVector30 operator* (sVector30Arg v,const sMatrix34 &m);
sVector31 operator* (sVector31Arg v,const sMatrix34 &m);
sVector4  operator* (sVector4Arg v,const sMatrix34 &m);

sVector30 operator* (sVector30Arg v,const sMatrix34CM &m);
sVector31 operator* (sVector31Arg v,const sMatrix34CM &m);
sVector4  operator* (sVector4Arg v,const sMatrix34CM &m);

sVector4  operator* (sVector30Arg v,const sMatrix44 &m);
sVector4  operator* (sVector31Arg v,const sMatrix44 &m);
sVector4  operator* (sVector4Arg v,const sMatrix44 &m);

sMatrix34 operator* (const sMatrix34 &a,sF32 b);
sMatrix44 operator* (const sMatrix44 &a,sF32 b);

sMatrix34 operator+ (const sMatrix34 &a, const sMatrix34 &b);
sMatrix44 operator+ (const sMatrix44 &a, const sMatrix44 &b);
sMatrix34 operator- (const sMatrix34 &a, const sMatrix34 &b);
sMatrix44 operator- (const sMatrix44 &a, const sMatrix44 &b);

#ifndef sHASINTRINSIC_MATRIXMUL
inline sMatrix44& operator*= (sMatrix44 &a,const sMatrix44 &b) { a = a*b; return a; }
inline sMatrix34& operator*= (sMatrix34 &a,const sMatrix34 &b) { a = a*b; return a; }
#else
sMatrix44& operator*= (sMatrix44 &a,const sMatrix44 &b);
sMatrix34& operator*= (sMatrix34 &a,const sMatrix34 &b);
#endif

#ifndef sHASINTRINSIC_VECTORMATRIXMUL
inline sVector30& operator*= (sVector30 &v,const sMatrix34 &m) { v = v*m; return v; }
inline sVector31& operator*= (sVector31 &v,const sMatrix34 &m) { v = v*m; return v; }
inline sVector4&  operator*= (sVector4  &v,const sMatrix34 &m) { v = v*m; return v; }
inline sVector4&  operator*= (sVector4  &v,const sMatrix44 &m) { v = v*m; return v; }
#else
sVector30& operator*= (sVector30 &v,const sMatrix34 &m);
sVector31& operator*= (sVector31 &v,const sMatrix34 &m);
sVector4&  operator*= (sVector4  &v,const sMatrix34 &m);
sVector4&  operator*= (sVector4  &v,const sMatrix44 &m);
#endif

sMatrix34& operator*= (sMatrix34 &a, sF32 b);
sMatrix44& operator*= (sMatrix44 &a, sF32 b);

sMatrix34& operator+= (sMatrix34 &a, const sMatrix34 &b);
sMatrix44& operator+= (sMatrix44 &a, const sMatrix44 &b);
sMatrix34& operator-= (sMatrix34 &a, const sMatrix34 &b);
sMatrix44& operator-= (sMatrix44 &a, const sMatrix44 &b);

inline sWriter& operator| (sWriter &s,const sMatrix34 &a) { s | a.i | a.j | a.k | a.l;  return s; }
inline sReader& operator| (sReader &s,      sMatrix34 &a) { s | a.i | a.j | a.k | a.l;  return s; }
inline sWriter& operator| (sWriter &s,const sMatrix44 &a) { s | a.i | a.j | a.k | a.l;  return s; }
inline sReader& operator| (sReader &s,      sMatrix44 &a) { s | a.i | a.j | a.k | a.l;  return s; }

/****************************************************************************/
/***                                                                      ***/
/***   Complex numbers. Quite rudimentary.                                ***/
/***                                                                      ***/
/****************************************************************************/

class sComplex
{
public:
  sF32 r,i; // real/imaginary part

  sComplex()                                {}
  sComplex(sF32 s) : r(s),i(0.0f)           {}
  sComplex(sF32 re,sF32 im) : r(re),i(im)   {}

  void Init(sF32 re,sF32 im)                { r=re; i=im; }
  void InitUnit(sF32 re,sF32 im)            { sF32 il = sFRSqrt(re*re + im*im); r = il*re; i = il*im; }

  sComplex &operator =(sF32 s)              { r = s; i = 0.0f; return *this; }

  sComplex &operator +=(const sComplex &b)  { r += b.r; i += b.i; return *this; }
  sComplex &operator *=(sF32 s)             { r *= s; i *= s; return *this; }
  sComplex &operator *=(const sComplex &b)  { sF32 re = r*b.r - i*b.i; sF32 im = r*b.i + i*b.r; Init(re,im); return *this; }

  sF32 LengthSq() const                     { return r*r + i*i; }
  sF32 Length() const                       { return sFSqrt(r*r + i*i); }

  void Unit()                               { *this *= sFRSqrt(r*r + i*i); }
};

inline sComplex operator +(const sComplex &a,const sComplex &b)
{
  return sComplex(a.r+b.r, a.i+b.i);
}

inline sComplex operator *(const sComplex &a,const sComplex &b)
{
  return sComplex(a.r*b.r - a.i*b.i,a.r*b.i + a.i*b.r);
}

inline sComplex operator *(sF32 s,const sComplex &a)   { return sComplex(s*a.r,s*a.i); }
inline sComplex operator *(const sComplex &a,sF32 s)   { return sComplex(s*a.r,s*a.i); }

inline sComplex operator -(const sComplex &a)          { return sComplex(-a.r,-a.i); }
inline sComplex operator ~(const sComplex &a)          { return sComplex(a.r,-a.i); } // conjugate

/****************************************************************************/
/***                                                                      ***/
/***   Axis Aligned Bounding Box                                          ***/
/***                                                                      ***/
/****************************************************************************/

class sAABBoxC;
class sAABBox                     // axis aligned bounding box
{
public:

  sVector31 Min;
  sVector31 Max;

  sAABBox();
  sAABBox(const sDontInitializeT&) : Min(sDontInitialize), Max(sDontInitialize) {}
  void Init(const sAABBoxC &);
  void Clear();

  sBool HitPoint(sVector31Arg p) const;
  sBool HitRay(sF32 &dist,const sRay &ray) const;
  sBool HitInvRay(sF32 &dist,sVector31Arg origin,sVector30Arg invDir) const;
  void Add(const sAABBox &box);
  void Add(sVector31Arg p);
  void Add(const sAABBox &box,const sMatrix34 &mat);
  void Add(const sAABBoxC &box,const sMatrix34CM &mat);
  void And(const sAABBox &box);
  sVector31 Center() const { sVector31 p((Min.x+Max.x)/2,(Min.y+Max.y)/2,(Min.z+Max.z)/2); return p; }
  sVector30 Size() const { sVector30 p(Max.x-Min.x,Max.y-Min.y,Max.z-Min.z); return p; }
  void MakePoints(sVector31 *points) const;
  sF32 CalcArea()const;
  sF32 CalcVolume()const;
  sBool IsEmpty() const;
  sBool IsValid() const;    // check if min<=max (empty bboxes are valid)

  sBool Intersects(const sAABBox &b) const;
  sBool IntersectsXZ(const sAABBox &b) const;
  sBool IntersectsMovingBox(const sAABBox &box,sVector30Arg v,sF32 tMin,sF32 tMax) const;
  sBool IntersectsMovingBoxInv(const sAABBox &box,sVector30Arg invV,sF32 tMin,sF32 tMax) const;
  sF32 DistanceToSq(sVector31Arg p) const;
  sF32 DistanceTo(sVector31Arg p) const { return sFSqrt(DistanceToSq(p)); }
  sInt Classify(sVector30Arg n, sF32 d);

  sAABBox &operator*=(const sMatrix34 &m);
  sBool operator==(const sAABBox& b)const { return Min==b.Min&&Max==b.Max; }
};

inline sAABBox operator* (const sAABBox &bbox,const sMatrix34 &m)  { sAABBox r=bbox; r*=m; return r; }


inline sWriter& operator| (sWriter &s,const sAABBox &a) { s | a.Min | a.Max;  return s; }
inline sReader& operator| (sReader &s,      sAABBox &a) { s | a.Min | a.Max;  return s; }

sFormatStringBuffer& operator% (sFormatStringBuffer &,const sAABBox &);

// a different representation

class sAABBoxC
{
public:
  sVector31 Center;               // center of the box
  sVector30 Radius;               // half the diameter in each axis

  sINLINE void Init(const sAABBox &s)
  {
    Radius = 0.5f * (s.Max - s.Min);
    Center = sVector31(0.5f * (sVector30(s.Max) + sVector30(s.Min)));
  }

  void Clear();
  void Add(const sAABBoxC &);
};

/****************************************************************************/
/***                                                                      ***/
/***   sOBBox - oriented bounding box                                     ***/
/***                                                                      ***/
/****************************************************************************/

struct sOBBox
{
  sQuaternion BoxToWorld;           // rotation from box to world coordinate system
  sVector31 Center;                 // center (in world space)
  sVector30 HalfExtents;            // box half-extents (in bbox coordinate system)

  sOBBox();
  void Init(const sAABBox &box);
  void Init(const sAABBoxC &box);
};

inline sWriter& operator |(sWriter &s,const sOBBox &b) { return s | b.BoxToWorld | b.Center | b.HalfExtents; }
inline sReader& operator |(sReader &s,      sOBBox &b) { return s | b.BoxToWorld | b.Center | b.HalfExtents; }

/****************************************************************************/
/***                                                                      ***/
/***   Ray with Start and direction                                       ***/
/***                                                                      ***/
/****************************************************************************/

class sRay
{
public:
  sVector31 Start;
  sVector30 Dir;
  sRay() {}
  sRay(sVector31Arg s,sVector30Arg d) : Start(s),Dir(d) {}

  sBool HitTriangle(sF32 &dist,sVector31Arg p0,sVector31Arg p1,sVector31Arg p2) const;
  sBool HitTriangleDoubleSided(sF32 &dist,sVector31Arg p0,sVector31Arg p1,sVector31Arg p2) const;
  sBool HitTriangleBary(sF32 &dist,sVector31Arg p0,sVector31Arg p1,sVector31Arg p2,sF32 &u0,sF32 &u1,sF32 &u2) const;
  sBool HitPlane(sVector31 &intersect,sVector4Arg plane) const;
  sBool HitPlaneDoubleSided(sVector31 &intersect,sVector4Arg plane) const;
  sBool HitSphere(sF32 *dist, sVector31Arg center, sF32 radius) const;
  sBool HitAABB(sF32 &min, sF32 &max, const sVector31& bbmin,const sVector31& bbmax)const;
  sBool HitBilinearPatch(sF32 &dist,const sVector31 &p00,const sVector31 &p01,const sVector31 &p10,const sVector31 &p11,sF32 *uOut,sF32 *vOut) const;
  sInt IntersectPlane(sF32 &t, sVector4Arg plane)const;  // 0 not intersection, 1 normal intersection, 2 ray in plane

  // warning: (out)dist is NOT ACCURATE, but about in the area of where it should be.
  //sBool HitCappedCylinder(sVector31Arg cylstart, sVector31Arg cylend, sF32 radius, sF32 *dist=0) const; 
};

inline sRay operator* (const sRay &r,const sMatrix34 &m) { sRay x(r.Start*m,r.Dir*m); return x; }

// ird : 1 / ray dir
sBool sIntersectRayAABB(sF32 &min, sF32 &max, const sVector31 &ro, const sVector30 &ird, const sVector31 &bbmin, const sVector31 &bbmax);

inline sWriter& operator| (sWriter &s,const sRay &a) { s | a.Start | a.Dir;  return s; }
inline sReader& operator| (sReader &s,      sRay &a) { s | a.Start | a.Dir;  return s; }

/****************************************************************************/
/***                                                                      ***/
/***   Frustum                                                            ***/
/***                                                                      ***/
/****************************************************************************/
/*
enum sFrustumPlanes
{
  sFP_Left = 0,
  sFP_Right,
  sFP_Bottom,
  sFP_Top,
  sFP_Near,
  sFP_Far,
};
*/

class sFrustum
{
public:
  sVector4 Planes[6];
  sVector4 AbsPlanes[6];

  // Full frustum
  inline void Init(const sMatrix44 &mat)
  {
    Init(mat,-1.0f,1.0f,-1.0f,1.0f,0.0f,1.0f);
  }

  // Frustum for a subset (in normalized clip coordinates)
  inline void Init(const sMatrix44 &mat,const sFRect &r,sF32 zMin,sF32 zMax)
  {
    Init(mat,r.x0,r.x1,r.y0,r.y1,zMin,zMax);
  }

  void Init(const sMatrix44 &mat,sF32 xMin,sF32 xMax,sF32 yMin,sF32 yMax,sF32 zMin,sF32 zMax);

  // Full tri-state clipping test (inside, intersecting, outside)
  sInt IsInside(const sAABBoxC &b) const;   // sTEST_???
  
  sINLINE sInt IsInside(const sAABBox &b) const // slower than above variant
  {
    sAABBoxC box;
    box.Init(b);
    return IsInside(box);
  }

  sInt IsInside(const sOBBox &b) const; // more work than for AABBs!

  // Just the rejection test (fully outside yes/no)
  sINLINE sBool IsOutside(const sAABBoxC &b) const
  {
    for(sInt i=0;i<6;i++)
    {
      sF32 m = b.Center ^ Planes[i];
      sF32 n = b.Radius ^ AbsPlanes[i];
      if(m+n<0.0f) return sTRUE;
    }

    return sFALSE;
  }

  sINLINE sBool IsOutside(const sAABBox &b) const
  {
    sAABBoxC box;
    box.Init(b);
    return IsOutside(box);
  }

  sBool IsOutside(const sOBBox &b) const; // more work than for AABBs!

  void Transform(const sFrustum &src,const sMatrix34 &mat);
  void Transform(const sFrustum &src,const sMatrix34CM &mat);
};

/****************************************************************************/
/***                                                                      ***/
/***   Other Geometry helpers                                             ***/
/***                                                                      ***/
/****************************************************************************/

// Squared distance between (line) segment ab and point p
sF32 sDistSegmentToPointSq(sVector2Arg a,sVector2Arg b,sVector2Arg p);
sF32 sDistSegmentToPointSq(const sVector31 &a,const sVector31 &b,const sVector31 &p);

// same, non-squared
sINLINE sF32 sDistSegmentToPoint(sVector2Arg a,sVector2Arg b,sVector2Arg p)
{
  return sFSqrt(sDistSegmentToPointSq(a,b,p));
}

sINLINE sF32 sDistSegmentToPoint(const sVector31 &a,const sVector31 &b,const sVector31 &p)
{
  return sFSqrt(sDistSegmentToPointSq(a,b,p));
}

// Squared distance between (line) segment ab and point p, returning t for closest point
sF32 sDistSegmentToPointSq(sVector2Arg a,sVector2Arg b,sVector2Arg p,sF32 &t);
sF32 sDistSegmentToPointSq(const sVector31 &a,const sVector31 &b,const sVector31 &p,sF32 &t);

// And again, non-squared version
sINLINE sF32 sDistSegmentToPoint(sVector2Arg a,sVector2Arg b,sVector2Arg p,sF32 &t)
{
  return sFSqrt(sDistSegmentToPointSq(a,b,p,t));
}

sINLINE sF32 sDistSegmentToPoint(const sVector31 &a,const sVector31 &b,const sVector31 &p,sF32 &t)
{
  return sFSqrt(sDistSegmentToPointSq(a,b,p,t));
}

/****************************************************************************/
/***                                                                      ***/
/***   Simple Rect in 3D Coordinates                                      ***/
/***                                                                      ***/
/****************************************************************************/

struct s3DRect
{
  sVector31 Center;
  sVector30 Up, Right;

  void Init() { Center.Init(0,0,0); Up.Init(0,1,0); Right.Init(1,0,0); }
  void InitBorder(sVector31Arg  bottomLeft, sVector30Arg  widthVector, sVector30Arg  heightVector) { Right = widthVector*0.5f; Up = heightVector*0.5f; Center = bottomLeft+Up+Right; }
  void InitCenter(sVector31Arg  center, sVector30Arg  upVector, sVector30Arg  rightVector) { Center = center; Up = upVector; Right = rightVector; }

  sVector31 BottomLeft()  const { return Center-Up-Right; }
  sVector31 TopRight()    const { return Center+Up+Right; }
  sVector31 TopLeft()     const { return Center+Up-Right; }
  sVector31 BottomRight() const { return Center-Up+Right; }
};

/****************************************************************************/
/***                                                                      ***/
/***   Scale-Rotate-Translate                                             ***/
/***                                                                      ***/
/****************************************************************************/

class sSRT
{
public:
  sVector31 Scale;                // scale before rotate
  sVector30 Rotate;               // 0..1 is full circle
  sVector31 Translate;            // translate after rotate

  sSRT();
  void Init(sF32 *);
  void MakeMatrix(sMatrix34 &mat) const;
  void MakeMatrixInv(sMatrix34 &mat) const;
  void Invert(); // warning: sSRTs are not closed under invert!

  void ToString(const sStringDesc &outStr) const;
  sBool FromString(const sChar *str);
};

inline sWriter& operator| (sWriter &s,const sSRT &a) { s | a.Scale | a.Rotate | a.Translate; return s; }
inline sReader& operator| (sReader &s,      sSRT &a) { s | a.Scale | a.Rotate | a.Translate; return s; }

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Other Applications                                                 ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/


/****************************************************************************/
/***                                                                      ***/
/***   Spline Basics                                                      ***/
/***                                                                      ***/
/****************************************************************************/

// calculate interpolation factors:
//
// t0 = t[1]-t[0]
// t1 = t[2]-t[1]
// t2 = t[3]-t[2]
// f = s[-1]*c0 + s[0]*c1 + s[1]*c2 + s[2]*c3

void sHermite(const sF32 x1, sF32 &c0, sF32 &c1, sF32 &c2, sF32 &c3);
void sHermiteD(const sF32 x1, sF32 &c0, sF32 &c1, sF32 &c2, sF32 &c3);
void sHermiteUniform(const sF32 x1,sF32 t0,sF32 t1,sF32 t2,sF32 &c0,sF32 &c1,sF32 &c2,sF32 &c3);
void sHermiteUniformD(const sF32 x1,sF32 t0,sF32 t1,sF32 t2,sF32 &c0,sF32 &c1,sF32 &c2,sF32 &c3);

// use to weight segments with (some function) of their lengths
// (gives better curves). l0="length" of segment from s[-1] to s[0],
// l1="length" of segment from s[0] to s[1], l2 from s[1] to s[2].
// normal (euclidean) distance or sqrt of euclidean distance work well.
// l0=l1=l2 corresponds to normal sHermite.
// obsolete
void sHermiteWeighted(const sF32 x1, sF32 l0, sF32 l1, sF32 l2, sF32 &c0, sF32 &c1, sF32 &c2, sF32 &c3);

// uniform cubic b-spline. approximating, not interpolating (curve doesn't pass
// through the control points!). very smooth.
void sUniformBSpline(const sF32 x1, sF32 &c0, sF32 &c1, sF32 &c2, sF32 &c3);
void sUniformBSplineD(const sF32 x1, sF32 &c0, sF32 &c1, sF32 &c2, sF32 &c3);

// cubic bezier curve weights.
void sCubicBezier(sF32 x1, sF32 &c0, sF32 &c1, sF32 &c2, sF32 &c3);

/****************************************************************************/
/***                                                                      ***/
/***   Smooth Function                                                    ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   y = f(x,s)                                                         ***/
/***   y = n/2x^2 + (4-n)x^3 + (-3+n/2)x^4                                ***/
/***   map x 0..1 to smooth y 0..1                                        ***/
/***                                                                      ***/
/***   at x=0 and x=1, slope (y') is 0                                    ***/
/***   at x=0, curvation (y'') is s                                       ***/
/***                                                                      ***/
/***   parameter s=6 makes it symetrical                                  ***/
/***   n=6  ->  y = 3x^2 - 2x^3    (also called smoothstep)               ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   good choice for smoothing geometry:                                ***/
/***                                                                      ***/
/***   plateau range 0..1 default 1 (no plateau)                          ***/
/***   smooth range 0..64 default 1 (smoothstep)                          ***/
/***                                                                      ***/
/***   y = sSmooth(x/plateau,12-smooth*6);                                ***/
/***                                                                      ***/
/****************************************************************************/

sF32 sSmooth(sF32 x,sF32 s=6);
inline sF32 sSmoothStep(sF32 x) { x=sClamp<sF32>(x,0,1); return (x*x)*3 - (x*x)*x*2; }
inline sF32 sSmoothStep(sF32 min, sF32 max, sF32 x) { return sSmoothStep((x-min)/(max-min)); }

/****************************************************************************/
/***                                                                      ***/
/***   Damping timeslice syncronisation                                   ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   if you use X' = X*DAMP+FORCE for a simple fake physic, you need    ***/
/***   to adjust DAMP and FORCE when you change the timeslice             ***/
/***                                                                      ***/
/***   if D and F are the factors for a timeslice of 1, then the factors  ***/
/***   for larger timeslices are:                                         ***/
/***                                                                      ***/
/***   X'  = X*D   +F             = XD+F                                  ***/
/***   X'2 = X*DD  +FD  +F        = XDD+(D+1)F                            ***/
/***   X'2 = X*DDD +FDD +FD +F    = XDDD+(DD+D+1)F                        ***/
/***   X'2 = X*DDDD+FDDD+FDD+FD+F = XDDDD+(DDD+DD+D+1)F                   ***/
/***                                                                      ***/
/***   When changing from Timeslice 1 to Timeslice N, this function       ***/
/***   calclulates D' and F' for use:                                     ***/
/***                                                                      ***/
/***   X' = X*DAMP+FORCE          // timeslice 1                          ***/
/***   X''= X*D'*DAMP + F'*FORCE  // timeslice N                          ***/
/***                                                                      ***/
/****************************************************************************/

void sMakeDamp(sInt timeslice,sF32 damp,sF32 &d,sF32 &f);

/****************************************************************************/
/***                                                                      ***/
/***   Filter (2Pole IIR)                                                 ***/
/***                                                                      ***/
/****************************************************************************/

struct sFilter2PoleTemp
{
  sF64 x1,x2;
  sF64 y1,y2;
  void Init() { sClear(*this); }
};

struct sFilter2Pole
{
  sF64 a0,a1,a2;
  sF64 b1,b2;

  void Init();
  void Scale(sF32 s);
  void Band(sF32 f,sF32 bw);      // bw = 0 .. 0.5
  void Low(sF32 f,sF32 reso);     // reso = 0 .. 1 (1 is dangerous!)
  void High(sF32 f,sF32 reso);    // reso = 0 .. 1 (1 is dangerous!)

  sF32 Filter(sFilter2PoleTemp &temp,sF32);
  void Filter(sFilter2PoleTemp &temp,sF32 *in,sF32 *out,sInt count);
  void FilterStereo(sFilter2PoleTemp *temp,sF32 *in,sF32 *out,sInt count);
};


/****************************************************************************/
/***                                                                      ***/
/***   Perlin Noise                                                       ***/
/***                                                                      ***/
/****************************************************************************/

// x,y = 8:16 (größere zahlen loopen)

sF32 sPerlin2D(sInt x,sInt y,sInt mask=255,sInt seed=0);  
sF32 sPerlin3D(sInt x,sInt y,sInt z,sInt mask=255,sInt seed=0);  
void sPerlinDerive3D(sInt x,sInt y,sInt z,sInt mask,sInt seed,sF32 &value,sVector30 &dir);

// x,y = 0..256 (größere zahlen loopen)
// mode = 0: x
// mode = 1: abs(x)
// mode = 2: sin(x*sPI)
// mode = 3: sin(abs(x)*sPI)

sF32 sPerlin2D(sF32 x,sF32 y,sInt octaves=1,sF32 falloff=1.0f,sInt mode=0,sInt seed=0);

/****************************************************************************/
/***                                                                      ***/
/***  Intersection                                                        ***/
/***                                                                      ***/
/****************************************************************************/

sBool sGetIntersection(sVector2Arg u1, sVector2Arg u2, sVector2Arg v1, sVector2Arg v2, sF32 *s=sNULL, sF32 *t=sNULL, sVector2 *p=sNULL);
sBool sGetIntersection(sVector2Arg u1, sVector2Arg u2, const sFRect &rect, sF32 *s=sNULL, sVector2 *p=sNULL);

// check two line segments for intersection, returns 0 : no intersection, 1 : intersection, 2 : coincident lines with overlapping segments
// line segment0 : [ls0,ls0+ld0], line segment1 : [ls1,ls1+ld1]
// intersection point ls0+dist0*ld0 resp. ls1+dist1*ld1, coincident lines with intersecting segment [ls0+dist0*ld0,ls0+dist1*ld0]
sInt sGetLineSegmentIntersection(sF32 &dist0, sF32 &dist1, sVector2Arg ls0, sVector2Arg ld0, sVector2Arg ls1, sVector2Arg ld1);

/****************************************************************************/
/***                                                                      ***/
/***   Bezier Splines                                                     ***/
/***                                                                      ***/
/****************************************************************************/

// n = 0..1, returns 4 factors to be multiplied with four points each.
sVector30 sGetBezierSplineFactors3(sF32 n);
sVector4 sGetBezierSplineFactors4(sF32 n);
void sGetBezierSplinePoint(sF32 *out, sF32 n, const sF32 *v1, const sF32 *v2, const sF32 *v3, const sU32 d);
void sGetBezierSplinePoint(sF32 *out, sF32 n, const sF32 *v1, const sF32 *v2, const sF32 *v3, const sF32 *v4, const sU32 d);
template<typename T>
inline T sGetBezierSplinePoint(sF32 n, const T& v1, const T& v2, const T& v3) { T out; sGetBezierSplinePoint(&out[0], n, &v1[0], &v2[0], &v3[0], T::Dimensions); return out; }
template<typename T>
inline T sGetBezierSplinePoint(sF32 n, const T& v1, const T& v2, const T& v3, const T& v4) { T out; sGetBezierSplinePoint(&out[0], n, &v1[0], &v2[0], &v3[0], &v4[0], T::Dimensions); return out; }

/****************************************************************************/
/***                                                                      ***/
/***   RGB <-> HSL color space conversion                                 ***/
/***                                                                      ***/
/****************************************************************************/

void sRgbToHsl(sF32 r, sF32 g, sF32 b, sF32 &h, sF32 &s, sF32 &l);
void sHslToRgb(sF32 h, sF32 s, sF32 l, sF32 &r, sF32 &g, sF32 &b);
void sRgbToHsl(sVector4Arg in, sVector4 &out);
sU32 sHslToColor(sF32 h, sF32 s, sF32 l, sF32 a=1.0f);


/************************************************************************/
//! \ingroup altona_base_math
//! Pseudo Random Generator
//! - Never generates the same value two times in a row.
//! - Values get more probable if not picked.
//! - Warning: This algorithm is only efficient for small result ranges!
//! - Complexity is between O(n) and O(2n).
/*************************************************************************/

template <class RndClass> class MarkovRandom
{
public:
  MarkovRandom(
    sInt elements /*!< [in] defines the range of generated values (0 .. elements-1) */, 
    sU32 seed /*!< [in] internal random generator seed */)
  {
    Rnd.Seed(seed);

    Probabilities.HintSize(elements);
    Probabilities.AddFull();
    Temp.HintSize(elements);
    Temp.AddFull();

    Reset();
  };

  ~MarkovRandom() {};

  //! Reset the history/probabilities to initial state.
  void Reset()
  {
    for (sInt i=0; i<Probabilities.GetCount(); i++) Probabilities[i]=1;
  }

  //! Get the next value. Will be different from the previous value and in the range [0..elements-1].
  sInt GetNextRandom()
  {
    // if we have an empty result range, just return zero (which is formally incorrect, but shouldn't be a problem)
    if (Probabilities.GetCount()==0) return 0;

    // prepare summed up temp
    Temp[0]=Probabilities[0];
    for (sInt i=1; i<Temp.GetCount(); i++) Temp[i] = Temp[i-1] + Probabilities[i];

    // generate random index
    sInt sum = Temp[Temp.GetCount()-1];
    sInt r = Rnd.Int(sMax(sum,1));

    // find weighted element
    sInt idx=0;
    while ( (idx<Temp.GetCount()) && (Temp[idx]<=r) ) idx++;

    // apply 'history' info to Probabilities for next run
    for (sInt i=0; i<Probabilities.GetCount(); i++)
    {
      if (i==idx)
      {
        Probabilities[i] = 0;
      } else {
        Probabilities[i]++;
      }
    }

    return sClamp(idx, 0, Probabilities.GetCount()-1);
  }

protected:
  sStaticArray<sInt> Probabilities;
  sStaticArray<sInt> Temp;
  RndClass Rnd;
};

/****************************************************************************/

// HEADER_ALTONA_BASE_MATH
#endif

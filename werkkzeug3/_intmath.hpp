// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once
#include "_types.hpp"

/****************************************************************************/

#define sFIXONE  0x4000
#define sFIXHALF 0x2000
#define sFIXSHIFT 14
void sInitIntMath2();

struct sIVector3;
struct sSVector3;
struct sISRT;
struct sIMatrix34;

/****************************************************************************/

#if sPLATFORM==sPLAT_PDA && sMOBILE
inline sInt sCountLeadingZeroes(sU32 x) { return _CountLeadingZeros(x); }
#else
sInt sCountLeadingZeroes(sU32 x);
#endif

inline sInt sIMul(sInt a,sInt b) { return (((sS64)a)*b)>>sFIXSHIFT; }
sInt sIntSqrt15D(sU32 r);

sInt sISin(sInt a);                         // 0..sFIXONE -> -sFIXONE..sFIXONE
sInt sICos(sInt a);                         // 0..sFIXONE -> -sFIXONE..sFIXONE

/****************************************************************************/

struct sIVector3                            // integer vector, usually 1:15:16
{
  sInt x,y,z;
  void Init(sInt X,sInt Y,sInt Z) { x=X; y=Y; z=Z; }
  void Rotate34(const sIMatrix34 &mat);

  void Sub(const sIVector3 &a,const sIVector3 &b) { x=a.x-b.x; y=a.y-b.y; z=a.z-b.z; }
  void Add(const sIVector3 &a,const sIVector3 &b) { x=a.x+b.x; y=a.y+b.y; z=a.z+b.z; }
  void Sub(const sIVector3 &b) { x=x-b.x; y=y-b.y; z=z-b.z; }
  void Add(const sIVector3 &b) { x=x+b.x; y=y+b.y; z=z+b.z; }
  void AddScale(const sIVector3 &b,sInt s) { x+=sIMul(b.x,s); y+=sIMul(b.y,s); z+=sIMul(b.z,s); }
  void Unit();
  void Cross(const sIVector3 &a,const sIVector3 &b);
};

struct sSVector3                            // short vector, usually 1:1:14
{
  sS16 x,y,z;
  void Init(sInt X,sInt Y,sInt Z) { x=X; y=Y; z=Z; }
  void Init(const sIVector3 &a) { x=a.x; y=a.y; z=a.z; }
};

struct sISRT                                // srt values to initialize a matrix
{
  sIVector3 s;
  sIVector3 r;
  sIVector3 t;

  void Init();
  void Fix14();
};


struct sIMatrix34                           // a 3d matrix
{
  sIVector3 i;
  sIVector3 j;
  sIVector3 k;
  sIVector3 l;

  void Init();
  void InitEuler(sInt a,sInt b,sInt c);
  void InitSRT(sISRT &srt);
  void Mul(const sIMatrix34 &a,const sIMatrix34 &b);
  void TransR();
};


/****************************************************************************/
/****************************************************************************/

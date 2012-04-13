// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __GENMISC_HPP__
#define __GENMISC_HPP__

#include "_start.hpp"

class ScriptRuntime;


/****************************************************************************/
/****************************************************************************/

sInt __stdcall Sys_PrintDez(sInt i);
sInt __stdcall Sys_PrintHex(sInt i);
sInt __stdcall Sys_Print(sChar *str);

sBool Sys_Show(ScriptRuntime *sr);            // obsolete
sObject *__stdcall Sys_Copy(sObject *obj);
void __stdcall Sys_LoadSound(class GenPlayer *player,sChar *name,sInt mode);
void __stdcall Sys_PlaySound(class GenPlayer *player,sInt song);
sObject * __stdcall Sys_FindLabel(sObject *obj,sInt label);
sInt __stdcall Sys_Pow(sInt a,sInt b);

/****************************************************************************/

/*
void __stdcall Global_CamFrustrum(sInt near_,sInt far_);
void __stdcall Global_CamPos(sInt3 t,sInt3 r);
void __stdcall Global_CamSet(void);
void __stdcall Global_CamMat(class GenMatrix *m);

void __stdcall Global_ObjPos(sInt3 s,sInt3 r,sInt3 t);
void __stdcall Global_ObjSet(void);
void __stdcall Global_ObjMat(class GenMatrix *m);
*/

/****************************************************************************/

class GenMatrix : public sObject
{
public:
  sU32 GetClass() { return sCID_GENMATRIX; }
#if !sINTRO
  GenMatrix();
  void Copy(sObject *);
#endif

  sF323 s,r,t;
  sInt Mode;

  GenMatrix *Parent;

  void Calc(sMatrix &);
};

GenMatrix * __stdcall Matrix_Generate(sF323 s,sF323 r,sF323 t,sInt mode);
GenMatrix * __stdcall Matrix_Modify(GenMatrix *,sF323 s,sF323 r,sF323 t,sInt mode);
GenMatrix * __stdcall Matrix_Label(GenMatrix *,sInt label);

#define MM_INIT       0
#define MM_ORBIT      1
#define MM_LOCKAT     2
#define MM_MUL        3
#define MM_TRANSPOSE  4

/****************************************************************************/

struct GenSplineKey
{
  sF32 Time;
  sF32 Value[4];
};

class GenSpline : public sObject
{
public:
  GenSpline();
  ~GenSpline();
  sU32 GetClass() { return sCID_GENSPLINE; }
#if !sINTRO
  void Clear();
  void Copy(sObject *);
#endif

  sInt Dimensions;
  sF32 Tension;
  sF32 Continuity;
  sF32 Bias;
  sArray<GenSplineKey> Keys;

  void Sort();
  void Calc(sF32 time,sF32 *);
};

GenSpline * __stdcall Spline_New(sInt dimensions,sInt tension,sInt continuity);
GenSpline * __stdcall Spline_Sort(GenSpline *spline);
GenSpline * __stdcall Spline_AddKey1(GenSpline *spline,sInt time,sInt val);
GenSpline * __stdcall Spline_AddKey2(GenSpline *spline,sInt time,sInt2 val);
GenSpline * __stdcall Spline_AddKey3(GenSpline *spline,sInt time,sInt3 val);
GenSpline * __stdcall Spline_AddKey4(GenSpline *spline,sInt time,sInt4 val);

void __stdcall Spline_Calc1(GenSpline *spline,sInt time);
void __stdcall Spline_Calc2(GenSpline *spline,sInt time);
void __stdcall Spline_Calc3(GenSpline *spline,sInt time);
void __stdcall Spline_Calc4(GenSpline *spline,sInt time);

/****************************************************************************/
/****************************************************************************/

#endif

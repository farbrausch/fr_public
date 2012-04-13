// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once
#include "_types.hpp"
#include "kdoc.hpp"

/****************************************************************************/

struct BlobSplineKey
{
  sInt Select;
  sF32 Time;
  sF32 px,py,pz;
  sF32 rx,ry,rz;
  sF32 Zoom;
  void Init() { sSetMem(this,0,sizeof(*this)); Zoom=1.0f; }
};

#pragma warning (disable : 4200) 

struct BlobSpline
{
  sInt Count;
  sInt Version;
  sInt Select;
  sInt Mode;
  sVector Target;
  sF32 Continuity;
  sF32 Tension;
  sInt Uniform;
  sU32 pad[5];
  BlobSplineKey Keys[0];

  BlobSpline *Copy();
  void CopyPara(BlobSpline *from);
  void Sort();
  sF32 Normalize();
  void SwapTargetCam();
  void SetSelect(sInt i);
  void SetTarget();

  void Calc(sF32 time,sMatrix &mat,sF32 &zoom);
  void CalcKey(sF32 time,BlobSplineKey &key);
};

#pragma warning (default : 4200) 

BlobSpline *MakeBlobSpline(sInt *out_bytes,sInt keys);
BlobSpline *MakeDummyBlobSpline(sInt *out_bytes);

/****************************************************************************/

struct BlobPipeKey
{
  sF32 PosX;
  sF32 PosY;
  sF32 PosZ;
  sF32 Radius;
  sInt Flags;
  sU32 pad[3];
};  

#pragma warning (disable : 4200) 

struct BlobPipe
{
  sInt Count;
  sInt Version;
  sInt Mode;
  sF32 Tension;
  sF32 StartX,StartY,StartZ;
  sU32 pad[1+8];

  BlobPipeKey Keys[0];

  void CopyPara(BlobPipe *from);
  BlobPipe *Copy();
};

#pragma warning (default : 4200) 

BlobPipe *MakeBlobPipe(sInt *out_bytes,sInt keys);
BlobPipe *MakeDummyBlobPipe(sInt *out_bytes);

/****************************************************************************/

class GenSpline : public KObject
{
public:
  GenSpline();
  virtual struct BlobSpline *GetBlobSpline() { return 0; }
  virtual struct BlobPipe *GetBlobPipe() { return 0; }
  virtual void Eval(sF32 time,sF32 phase,sMatrix &mat,sF32 &zoom)=0;
};

class GenSplineSpline : public GenSpline
{
public:
  GenSplineSpline();
  ~GenSplineSpline();
  void Eval(sF32 time,sF32 phase,sMatrix &mat,sF32 &zoom);
  void Init(BlobSpline *data);

  BlobSpline *Spline;
  BlobPipe *Pipe;
  struct BlobSpline *GetBlobSpline() { return Spline; }
  struct BlobPipe *GetBlobPipe() { return Pipe; }
};

class GenSplineShaker : public GenSpline
{
public:
  GenSplineShaker();
  ~GenSplineShaker();
  void Eval(sF32 time,sF32 phase,sMatrix &mat,sF32 &zoom);
  GenSpline *Parent;
  KOp *OpLink;

  sInt Mode;
  sVector TranslateAmp;
  sVector RotateAmp;
  sVector TranslateFreq;
  sVector RotateFreq;
  sVector Center;
};

class GenSplineMeta : public GenSpline
{
public:
  GenSplineMeta();
  ~GenSplineMeta();
  void Eval(sF32 time,sF32 phase,sMatrix &mat,sF32 &zoom);
  void Sort();

  sF32 Times[8];
  GenSpline *Splines[8];
  sInt Count;
};

/****************************************************************************/

//void __stdcall Exec_Misc_Spline(KOp *op,KEnvironment *kenv);
//void __stdcall Exec_Misc_Shaker(KOp *op,KEnvironment *kenv,sInt mode,sF323 ta,sF323 ra,sF323 tf,sF323 rf,sF32 time,sF323 center);
//void __stdcall Exec_Misc_PipeSpline(KOp *op,KEnvironment *kenv);
KObject * __stdcall Init_Misc_Spline(KOp *op);
KObject * __stdcall Init_Misc_Shaker(KOp *op,GenSpline *gs,sInt mode,sF323 ta,sF323 ra,sF323 tf,sF323 rf,sF32 time,sF323 center);
KObject * __stdcall Init_Misc_PipeSpline(KOp *op);

struct Init_Misc_MetaSplinePara
{
  GenSpline *sp[8];
  sF32 time[8];
};
KObject * __stdcall Init_Misc_MetaSpline(Init_Misc_MetaSplinePara);

/****************************************************************************/

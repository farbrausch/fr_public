// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __GENEFFECT_HPP__
#define __GENEFFECT_HPP__

#include "_types.hpp"
#include "_start.hpp"
#include "kdoc.hpp"

/****************************************************************************/

class GenEffect : public KObject
{
public:
  GenEffect();
  ~GenEffect();
  void Copy(KObject *);
  KObject *Copy();

  class GenMaterial *Material;
  sInt Pass;
  sInt Usage;
  sBool NeedCurrentRender;
  void *EffectData;
  KOp *Op;
};

GenEffect *MakeEffect(class GenMaterial *mtrl);      // new GenEffect, set Pass from material, release material.
void SetVert(sF32 *fp,sF32 x,sF32 y,sF32 z,sF32 u,sF32 v,sU64 col=~0);   // set tspace vertex for simple effects.


/****************************************************************************/

KObject * __stdcall Init_Effect_Dummy(class GenMaterial *mtrl,sF323 size);
void __stdcall Exec_Effect_Dummy(KOp *op,KEnvironment *kenv,sF323 size);

/****************************************************************************/

KObject * __stdcall Init_Effect_Print(class GenMaterial *mtrl,sInt flags,sF32 sizex,sF32 sizey,sF32 spacex,sF32 spacey,sF32 radius,sChar *text);
void __stdcall Exec_Effect_Print(KOp *op,KEnvironment *kenv,sInt flags,sF32 sizex,sF32 sizey,sF32 spacex,sF32 spacey,sF32 radius,sChar *text);

/****************************************************************************/

//KObject * __stdcall Init_Effect_Particles(class GenMaterial *mtrl,sInt mode,sInt count,sF32 time,sF32 rate,sF324 speed,sU32 col0,sU32 col1,sU32 col2,sF323 size,sF32 middle,sF32 fade,sF324 rpos,sF324 rspeed);
//void __stdcall Exec_Effect_Particles(KOp *op,KEnvironment *kenv,sInt mode,sInt count,sF32 time,sF32 rate,sF324 speed,sU32 col0,sU32 col1,sU32 col2,sF323 size,sF32 middle,sF32 fade,sF324 rpos,sF324 rspeed);

/****************************************************************************/

KObject * __stdcall Init_Effect_PartEmitter(
  sInt slot,sInt mode,sInt count,sF32 time,sF32 rate,
  sF324 speed,sF324 rpos,sF324 rspeed);
void __stdcall Exec_Effect_PartEmitter(KOp *op,KEnvironment *kenv,
  sInt slot,sInt mode,sInt count,sF32 time,sF32 rate,
  sF324 speed,sF324 rpos,sF324 rspeed);

/****************************************************************************/

KObject * __stdcall Init_Effect_PartSystem(class GenMaterial *mtrl,
  sInt slot,sInt count,sU32 col0,sU32 col1,sU32 col2,sF323 size,sF32 middle,sF32 fade,sF32 gravity,sInt flags);
void __stdcall Exec_Effect_PartSystem(KOp *op,KEnvironment *kenv,
  sInt slot,sInt count,sU32 col0,sU32 col1,sU32 col2,sF323 size,sF32 middle,sF32 fade,sF32 gravity,sInt flags);
void FreeParticles();

/****************************************************************************/

KObject * __stdcall Init_Effect_ImageLayer(KOp *op,class GenBitmap *alpha,sInt pass,sInt quality,sF32 fade,sInt flags,sChar *filename);
void __stdcall Exec_Effect_ImageLayer(KOp *op,KEnvironment *kenv,sInt pass,sInt quality,sF32 fade,sInt flags,sChar *filename);

/****************************************************************************/

struct BillboardOTElem
{
  BillboardOTElem *Next;
  sF32 x,y,z;
  sF32 size;
  sInt uvval;
};

void PaintBillboards(KEnvironment *kenv,sInt count,BillboardOTElem **OT,sInt MAXOT,sF32 size,sF32 aspect,sInt mode);

KObject * __stdcall Init_Effect_Billboards(class GenMinMesh *mesh,class GenMaterial *mtrl,sF32 size,sF32 aspect,sF32 maxdist,sInt mode);
void __stdcall Exec_Effect_Billboards(KOp *op,KEnvironment *kenv,sF32 size,sF32 aspect,sF32 maxdist,sInt mode);

/****************************************************************************/

#endif

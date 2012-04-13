// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __GENEFFECTEX_HPP__
#define __GENEFFECTEX_HPP__

#include "_types.hpp"
#include "_start.hpp"
#include "kdoc.hpp"
#include "geneffect.hpp"

/****************************************************************************/

KObject * __stdcall Init_Effect_Chaos1(class GenMaterial *mtrl,sF323 a,sF323 b,sF323 c,sInt flags,sInt effect,sInt seed,sInt maxcount);
void __stdcall Exec_Effect_Chaos1(KOp *op,KEnvironment *kenv,sF323 a,sF323 b,sF323 c,sInt flags,sInt effect,sInt seed,sInt maxcount);


KObject * __stdcall Init_Effect_Tourque(class GenMaterial *mtrl,class GenMesh *mesh,sInt Seed,sInt MaxCount,sInt Rate,sInt Flags,sF32 SizeF,sF32 SizeS,sF32 Speed,sF32 Damp,sF323 Pos,sF323 Range,sInt Slot);
void __stdcall   Exec_Effect_Tourque(KOp *op,KEnvironment *kenv,sInt Seed,sInt MaxCount,sInt Rate,sInt Flags,sF32 SizeF,sF32 SizeS,sF32 Speed,sF32 Damp,sF323 Pos,sF323 Range,sInt Slot);

struct StreamPara
{
  sInt Seed;
  sInt Count;
  sInt Rate;
  sInt Flags;
  sInt Slot;
  sF32 SizeF,SizeS;
  sF32 Speed;
  sVector Pos[4];
  sF32 StuffX;
  sF32 StuffY;
  sF32 StuffZ;
  sF32 StuffW;
  sVector Light[2];
};

KObject * __stdcall Init_Effect_Stream(class GenMaterial *mtrl,StreamPara para);
void __stdcall   Exec_Effect_Stream(KOp *op,KEnvironment *kenv,StreamPara para);

/****************************************************************************/

KObject * __stdcall Init_Effect_BP06Spirit(class GenMaterial *mtrl,class GenMaterial *mtrl2,sF323 pos,sF32 radius,sF32 corerad,sF32 perlfreq,sF32 perlamp,sF32 perlanim,sU32 coli,sU32 colo,sU32 colc,sInt partcount,sF32 partrad,sF32 partspeed,sF32 partthick,sF32 partanim,sInt segments);
void __stdcall   Exec_Effect_BP06Spirit(KOp *op,KEnvironment *kenv,sF323 pos,sF32 radius,sF32 corerad,sF32 perlfreq,sF32 perlamp,sF32 perlanim,sU32 coli,sU32 colo,sU32 colc,sInt partcount,sF32 partrad,sF32 partspeed,sF32 partthick,sF32 partanim,sInt segments);

KObject * __stdcall Init_Effect_BP06Jungle(class GenMaterial *mtrl,sInt segments,sInt slices,sF32 thickness,sF32 thickscale,sF32 length,sF32 lenscale,sF32 sangle,sF32 carfreq,sF32 caramp,sF32 modfreq,sF32 modamp,sF32 modphase);
void __stdcall Exec_Effect_BP06Jungle(KOp *op,KEnvironment *kenv,sInt segments,sInt slices,sF32 thickness,sF32 thickscale,sF32 length,sF32 lenscale,sF32 sangle,sF32 carfreq,sF32 caramp,sF32 modfreq,sF32 modamp,sF32 modphase);

/****************************************************************************/

#endif

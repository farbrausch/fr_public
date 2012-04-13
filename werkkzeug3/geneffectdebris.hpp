// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __GENEFFECTDEBRIS_HPP__
#define __GENEFFECTDEBRIS_HPP__

#include "_types.hpp"
#include "_start.hpp"
#include "kdoc.hpp"
#include "geneffect.hpp"

/****************************************************************************/

KObject * __stdcall Init_Effect_WalkTheSpline(class GenMinMesh *mesh,class GenSpline *spline,sF32 animpos,sF32 animwalk,sInt count,sInt seed,sF32 side,sF32 middle);
void __stdcall Exec_Effect_WalkTheSpline(KOp *op,KEnvironment *kenv,sF32 animpos,sF32 animwalk,sInt count,sInt seed,sF32 side,sF32 middle);

/****************************************************************************/

KObject * __stdcall Init_Effect_ChainLine(GenMaterial *mtrl,
                                          sF323 posa,sF323 posb,sInt marka,sInt markb,sF32 dist,sF32 thick,sF32 gravity,sF32 damp,sF32 spring,
                                          sF32 rip,sF32 windspeed,sF32 windforce,sInt flags,sF323 basewind,sInt rippoint);
void __stdcall Exec_Effect_ChainLine(KOp *op,KEnvironment *kenv,
                                     sF323 posa,sF323 posb,sInt marka,sInt markb,sF32 dist,sF32 thick,sF32 gravity,sF32 damp,sF32 spring,
                                     sF32 rip,sF32 windspeed,sF32 windforce,sInt flags,sF323 basewind,sInt rippoint);

/****************************************************************************/

KObject * __stdcall Init_Effect_BillboardField(class GenMaterial *mtrl,sF32 size,sF32 aspect,sF32 tweak,sInt mode,sF323 r0,sF323 r1,sF32 anim,sInt count,sInt seed,sF32 heightrnd,sF32 mindist);
void __stdcall Exec_Effect_BillboardField(KOp *op,KEnvironment *kenv,sF32 size,sF32 aspect,sF32 tweak,sInt mode,sF323 r0,sF323 r1,sF32 anim,sInt count,sInt seed,sF32 heightrnd,sF32 mindist);

/****************************************************************************/

#endif

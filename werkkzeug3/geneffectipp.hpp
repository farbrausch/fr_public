// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __GENEFFECTIPP_HPP__
#define __GENEFFECTIPP_HPP__

#include "_types.hpp"
#include "geneffect.hpp"

/****************************************************************************/

KObject * __stdcall Init_Effect_LineShuffle(sInt pass);
void __stdcall Exec_Effect_LineShuffle(KOp *op,KEnvironment *kenv,sInt pass);

KObject * __stdcall Init_Effect_JPEG(sInt pass,sF32 ratio,sF32 strength,sInt mode,sInt iters);
void __stdcall Exec_Effect_JPEG(KOp *op,KEnvironment *kenv,sInt pass,sF32 ratio,sF32 strength,sInt mode,sInt iters);

KObject * __stdcall Init_Effect_Glare(sInt pass,sInt downsample,sU32 tresh,sU32 cb,sU32 co,sF32 blur0,sF32 blur1,sF32 amp0,sF32 amp1,sF32 grayf,sF32 addsmoothf);
void __stdcall Exec_Effect_Glare(KOp *op,KEnvironment *kenv,sInt pass,sInt downsample,sU32 tresh,sU32 cb,sU32 co,sF32 blur0,sF32 blur1,sF32 amp0,sF32 amp1,sF32 grayf,sF32 addsmoothf);

KObject * __stdcall Init_Effect_ColorCorrection(sInt pass,sF32 t0,sF32 t1,sU32 c1,sU32 c0,sU32 color,sU32 gamma,sInt flags,sU32 c1g,sU32 c0g,sF32 amp);
void __stdcall Exec_Effect_ColorCorrection(KOp *op,KEnvironment *kenv,sInt pass,sF32 t0,sF32 t1,sU32 c1,sU32 c0,sU32 color,sU32 gamma,sInt flags,sU32 c1g,sU32 c0g,sF32 amp);

/****************************************************************************/

#endif

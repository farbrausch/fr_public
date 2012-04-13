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

/****************************************************************************/

#endif

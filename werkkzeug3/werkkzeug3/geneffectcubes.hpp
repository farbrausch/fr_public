// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __GENEFFECTCUBES_HPP__
#define __GENEFFECTCUBES_HPP__

#include "_types.hpp"
#include "geneffect.hpp"

/****************************************************************************/

extern sVector __declspec(align(16)) CubeBuffer[];
extern sInt CubeCount;

/****************************************************************************/

KObject * __stdcall Init_Effect_Cubes(KOp *op);
void __stdcall Exec_Effect_Cubes(KOp *op,KEnvironment *kenv);

KObject * __stdcall Init_Effect_BounceWorm(KOp *op);
void __stdcall Exec_Effect_BounceWorm(KOp *op,KEnvironment *kenv,sInt,sF32,sF32,sF32,sF32,sInt,sInt);

KObject * __stdcall Init_Effect_Wirbeln(KOp *op);
void __stdcall Exec_Effect_Wirbeln(KOp *op,KEnvironment *kenv,sF32,sF32,sF32,sF32,sF32,sF32,sInt,sInt, sF32,sF32,sF32,sInt );

KObject * __stdcall Init_Effect_XSI2Cube(KOp *op);
void __stdcall Exec_Effect_XSI2Cube(KOp *op,KEnvironment *kenv);

KObject * __stdcall Init_Effect_CubeMorpher(KOp *op);
void __stdcall Exec_Effect_CubeMorpher(KOp *op,KEnvironment *kenv);

/****************************************************************************/

#endif

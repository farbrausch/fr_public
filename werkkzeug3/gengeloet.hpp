// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __GENGELOET__
#define __GENGELOET__

#include "_types.hpp"

class GenBitmap;

/****************************************************************************/

// generators
GenBitmap * __stdcall Bitmap_Geloet(sInt xs,sInt ys,sInt flags,sInt pointsPerIt,sInt edgesPerIt,sInt iters,sInt maxLen,sInt seed,sF32 lineDiam,sF32 radMiddle,sF32 radDisc);

// effects
KObject * __stdcall Init_Effect_Geloet2D(class GenMaterial *mtrl,class GenMaterial *mtrl2,sF32 animate,sInt gw,sInt gh,sInt flags,sInt ptIter,sInt edgeIter,sInt iters,sInt maxLen,sInt rndSeed,sF32 lineWidth,sF32 ptWidth,const sChar *text);
void __stdcall Exec_Effect_Geloet2D(KOp *op,KEnvironment *kenv,sF32 animate,sInt gw,sInt gh,sInt flags,sInt ptIter,sInt edgeIter,sInt iters,sInt maxLen,sInt rndSeed,sF32 lineWidth,sF32 ptWidth,const sChar *text);

KObject * __stdcall Init_Effect_GeloetMesh(class GenMinMesh *msh,class GenMaterial *mtrl,class GenMaterial *mtrl2,sF32 animate,sInt ptIter,sInt edgeIter,sInt iters,sInt maxLen,sInt rndSeed,sF32 lineWidth,sF32 ptWidth);
void __stdcall Exec_Effect_GeloetMesh(KOp *op,KEnvironment *kenv,sF32 animate,sInt ptIter,sInt edgeIter,sInt iters,sInt maxLen,sInt rndSeed,sF32 lineWidth,sF32 ptWidth);

/****************************************************************************/

#endif

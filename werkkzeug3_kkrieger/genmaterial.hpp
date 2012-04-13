// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __GENMATERIAL__
#define __GENMATERIAL__

#include "_types.hpp"
#include "kdoc.hpp"

class GenBitmap;
class GenMesh;

/****************************************************************************/

struct GenMaterialPass
{
  sMaterial *Mtrl;                // actual material
  sInt Usage;                     // material usage
  sInt Program;                   // finalizer
  sInt Pass;                      // render pass
  sF32 Size;                      // parameter to finalizer
  sF32 Aspect;                    // parameter to finalizer
};

class GenMaterial : public KObject
{
public:
  GenMaterial();
  ~GenMaterial();
  void Copy(KObject *);
  KObject *Copy();

  // doesn't change mtrl reference count
  void AddPass(sMaterial *mtrl,sInt use,sInt program,sInt pass=0,sF32 size=1.0f,sF32 aspect=1.0f);

  sArray<GenMaterialPass> Passes;
  class EngMaterialInsert *Insert;
};

/****************************************************************************/

GenMaterial * __stdcall Init_Material_Material(KOp *op);
void __stdcall Exec_Material_Material(KOp *op,KEnvironment *kenv);
GenMesh * __stdcall Mesh_MatLink(GenMesh *mesh,GenMaterial *mtrl,sInt mask,sInt pass);
GenMaterial * __stdcall Material_Add(sInt count,GenMaterial *m0,...);

GenMaterial * __stdcall Init_Material_Material20(KOp *op);

/****************************************************************************/
/****************************************************************************/

#endif

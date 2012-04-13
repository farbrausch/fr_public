// This code is in the public domain. See LICENSE for details.

#ifndef __engine_h_
#define __engine_h_

#include "types.h"
#include "rtlib.h"

namespace fr
{
  struct matrix;
}

class frGeometryOperator;
class frMaterial;

extern void engineInit();
extern void engineClose();
extern void enginePaint(frGeometryOperator* plg, const fr::matrix& cam, sF32 w, sF32 h);
extern void engineSetMaterial(const frMaterial* material);

#endif

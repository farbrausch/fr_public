// This code is in the public domain. See LICENSE for details.

#ifndef __rg2_exporttool_h_
#define __rg2_exporttool_h_

#include "types.h"

namespace fr
{
  class stream;
}

struct fr3Float;
struct frColor;

extern void putToterFloat(fr::stream& f, sF32 val);
extern void putPackedFloat(fr::stream& f, sF32 val);
extern void putPackedVector(fr::stream& f, fr3Float val, sBool delta = sFALSE, sInt killMask = 0);
extern void putSmallInt(fr::stream& f, sU32 val);
extern sInt colorClass(const frColor& col);
extern void putColor(fr::stream& f, const frColor& col);

#endif

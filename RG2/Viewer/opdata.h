// This code is in the public domain. See LICENSE for details.

#ifndef __opdata_h_
#define __opdata_h_

#include "types.h"

namespace fr
{
  struct vector;
}

extern sInt getSmallInt(const sU8*& strm);
extern sF32 getPackedFloat(const sU8*& strm);
extern sF32 getToterFloat(const sU8*& strm);
extern void getPackedVec(fr::vector& vec, const sU8*& strm);

#endif

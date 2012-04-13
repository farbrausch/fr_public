// This code is in the public domain. See LICENSE for details.

// runtime library header

#ifndef __rtlib_h_
#define __rtlib_h_

#define  PI    3.1415926535897932384626433832795f
#define  SQRT2 1.4142135623730950488016887242097f

#pragma warning (disable: 4244)

#include "math.h"
#include "string.h"

#pragma intrinsic (sin, cos, atan2, sqrt, memcpy, memset, fabs, pow, exp)

extern "C" size_t __cdecl   strlen(const char *s);

#endif

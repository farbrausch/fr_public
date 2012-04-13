// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __PLRUTIL_HPP_
#define __PLRUTIL_HPP_

#include "_types.hpp"

/****************************************************************************/
/****************************************************************************/

extern sVector PerlinGradient3D[16];
extern sF32 PerlinRandom[256][2];
extern sU8 PerlinPermute[512];

/****************************************************************************/

extern void InitPerlin();

#endif

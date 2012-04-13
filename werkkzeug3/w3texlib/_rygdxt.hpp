// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __RYGDXT_HPP__
#define __RYGDXT_HPP__

#include "_types.hpp"

/****************************************************************************/

namespace Werkk3TexLib
{

// initialize DXT codec. only needs to be called once.
void sInitDXT();

// input: a 4x4 pixel block, A8R8G8B8. you need to handle boundary cases
// yourself.
// alpha=sTRUE => use DXT5 (else use DXT1)
// quality: 0=fastest (no dither), 1=medium (dither)
void sCompressDXTBlock(sU8 *dest,const sU32 *src,sBool alpha,sInt quality);

}

/****************************************************************************/

#endif

// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __LOADPNG_HPP__
#define __LOADPNG_HPP__

#include "_types.hpp"

// this, well, loads PNGs.
sBool LoadPNG(const sU8 *data,sInt size,sInt &xout,sInt &yout,sU8 *&dataout);

// and this one saves them (ARGB8888, little endian)
sBool SavePNG(const sChar *filename,const sU8 *data,sInt xSize,sInt ySize);

#endif

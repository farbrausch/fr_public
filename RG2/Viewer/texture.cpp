// This code is in the public domain. See LICENSE for details.

#include "types.h"
#include "texture.h"

// ---- frBitmap

frBitmap::frBitmap(sU32 nw, sU32 nh)
  : w(nw), h(nh), size(nw*nh)
{
  data = new sU16[w * h * 4];
}

frBitmap::~frBitmap()
{
  delete[] data;
}

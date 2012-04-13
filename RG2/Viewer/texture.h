// This code is in the public domain. See LICENSE for details.

// rg2 intro runtime texture base system

#ifndef __texture_h__
#define __texture_h__

#include "types.h"

namespace fvs
{
  class texture;
}

class frTextureOperator;

struct frBitmap
{
  sU32          w, h;
  sU32          size;
  sU16*         data;

  frBitmap(sU32 nw, sU32 nh);
  ~frBitmap();
};

#endif

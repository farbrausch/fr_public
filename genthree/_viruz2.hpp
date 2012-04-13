// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef ___VIRUZ2_HPP__
#define ___VIRUZ2_HPP__

#include "_types.hpp"
#include "_start.hpp"

/****************************************************************************/

#if sLINK_VIRUZ2

class sViruz2 : public sMusicPlayer
{
  sF32 *Buffer;
public:
#if !sINTRO
  sViruz2();
  ~sViruz2();
#endif
  sBool Init(sInt songnr);
  sInt Render(sS16 *stream,sInt samples);
};
#endif

/****************************************************************************/

#endif

// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef ___OGG_HPP__
#define ___OGG_HPP__

#include "_types.hpp"
#include "_start.hpp"

#if sLINK_OGG

/****************************************************************************/

class sOGGDecoder : public sMusicPlayer
{
  sInt Size;                      // how many samples were decoded in the buffer
  sInt Index;                     // how much of the buffer was read (in samples)

  struct sOGGStub *Stub;

public:
  sOGGDecoder();
  ~sOGGDecoder();

  sInt ZeroStart;                 // put some zeros before start

  sBool Init(sInt songnr);
  sInt Render(sS16 *stream,sInt samples);
  sInt GetTuneLength();
};

/****************************************************************************/

#endif
#endif

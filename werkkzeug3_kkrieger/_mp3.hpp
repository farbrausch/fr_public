// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef ___MP3_HPP__
#define ___MP3_HPP__

#include "_types.hpp"
#include "_start.hpp"

#if sLINK_MP3

/****************************************************************************/

class sMP3Decoder : public sMusicPlayer
{
  sU32 *Buffer;                   // stores one decoded frame
  sInt Size;                      // how many samples were decoded in the buffer
  sInt Index;                     // how much of the buffer was read (in samples)

  class ampegdecoder *dec;
  sInt DecodeBlock();
public:
  sMP3Decoder();
  ~sMP3Decoder();

  sBool Init(sInt songnr);
  sInt Render(sS16 *stream,sInt samples);
};

/****************************************************************************/

#endif
#endif

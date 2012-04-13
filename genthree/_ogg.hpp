// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef ___OGG_HPP__
#define ___OGG_HPP__

#include "_types.hpp"
#include "_start.hpp"

#if sLINK_OGG

/****************************************************************************/

#define sCID_OGGDECODER           sMAKECID(1,3,6)

class sOGGDecoder : public sSoundFilter
{
  sInt Size;                      // how many samples were decoded in the buffer
  sInt Index;                     // how much of the buffer was read (in samples)

  sBool DeleteStream;

  struct sOGGStub *Stub;

public:
  sTYPEINFOH();
  sOGGDecoder();
  ~sOGGDecoder();

  sInt ZeroStart;                 // put some zeros before start

  void Load(sU8 *data,sInt size,sBool del=sFALSE);
  sBool Load(sChar *name);
  void Handler(void *stream,sInt samples,sU32 time);
  sPtr Render(sInt &size);
};

/****************************************************************************/

#endif
#endif

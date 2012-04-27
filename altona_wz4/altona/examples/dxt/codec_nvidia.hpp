/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#pragma once
#include "config.hpp"
#if LINK_NVIDIA

/****************************************************************************/

#include "base/types.hpp"
#include "doc.hpp"

/****************************************************************************/

class CodecNVIDIA : public DocCodec
{
public:
  CodecNVIDIA();
  ~CodecNVIDIA();

  const sChar *GetName();
  void Pack(sImage *bmp,sImageData *dxt,sInt level=1);
  void Unpack(sImage *bmp,sImageData *dxt,sInt level=1);
};

/****************************************************************************/

#endif

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#pragma once
#include "config.hpp"
#if LINK_ATI

/****************************************************************************/

#include "base/types.hpp"
#include "doc.hpp"
#include "ATI_Compress.h" // ATI dxt

/****************************************************************************/

class CodecATI : public DocCodec
{             
public:
  CodecATI();
  ~CodecATI();

  const sChar *GetName();
  void Pack(sImage *bmp,sImageData *dxt,sInt level=1);
  void Unpack(sImage *bmp,sImageData *dxt,sInt level=1);
};

/****************************************************************************/

#endif
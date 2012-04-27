/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#pragma once
#include "base/types2.hpp"
#include "doc.hpp"

/****************************************************************************/

class CodecTest : public DocCodec
{             
public:
  CodecTest();
  ~CodecTest();

  const sChar *GetName();
  void Pack(sImage *bmp,sImageData *dxt,sInt level=1);
  void Unpack(sImage *bmp,sImageData *dxt,sInt level=1);
};

/****************************************************************************/


/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "codec_altona.hpp"
#include "base/graphics.hpp"

/****************************************************************************/
/****************************************************************************/

CodecAltona::CodecAltona() 
{
}

CodecAltona::~CodecAltona() 
{
}

const sChar *CodecAltona::GetName()
{
  return L"altona";
}

/****************************************************************************/

void CodecAltona::Pack(sImage *bmp,sImageData *dxt,sInt level)      // dummy!
{
  sVERIFY((dxt->Format&sTEX_FORMAT) == level);
  dxt->ConvertFrom(bmp);
}

void CodecAltona::Unpack(sImage *bmp,sImageData *dxt,sInt level)
{
  sVERIFY((dxt->Format&sTEX_FORMAT) == level);
  dxt->ConvertTo(bmp);
}

/****************************************************************************/
/****************************************************************************/


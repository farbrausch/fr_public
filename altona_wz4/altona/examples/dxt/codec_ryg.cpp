/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) 2005-2006 by Dierk Ohlerich                          ***/
/***   all rights reserved                                                ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "codec_ryg.hpp"

/****************************************************************************/

const sChar *CodecRyg::GetName()
{
  return L"ryg (fastdxt)";
}

void CodecRyg::Pack(sImage *bmp,sImageData *dxt,sInt level)
{
  sVERIFY((dxt->Format&sTEX_FORMAT) == level);

  const sImage *img=bmp;
  sImage *other;

  sU8 *d = dxt->Data;
  sInt m = 0;

  for(;;)
  {
    sInt pc = img->SizeX*img->SizeY;
    switch(dxt->Format&sTEX_FORMAT)
    {
    case sTEX_DXT1: // no DXT1A suppot with fastdxt
      sFastPackDXT(d,img->Data,img->SizeX,img->SizeY,dxt->Format&sTEX_FORMAT,1);
      d += pc/2;
      break;

    case sTEX_DXT3:
    case sTEX_DXT5:
    case sTEX_DXT5N:
      sFastPackDXT(d,img->Data,img->SizeY,img->SizeY,dxt->Format&sTEX_FORMAT,1);
      d += pc/2;
      break;

    default:
      sVERIFYFALSE;
    }

    m++;
    if(m >= dxt->Mipmaps)
    {
      if(img != bmp) delete img;
      break;
    }

    other = img->Half();
    if(img!=bmp) delete img;
    img = other;
  }
}

void CodecRyg::Unpack(sImage *bmp,sImageData *dxt,sInt level)
{
  sVERIFY((dxt->Format&sTEX_FORMAT) == level);
  dxt->ConvertTo(bmp);
}

/****************************************************************************/


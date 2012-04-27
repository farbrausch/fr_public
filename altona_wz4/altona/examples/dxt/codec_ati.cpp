/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "config.hpp"
#if LINK_ATI

/****************************************************************************/

#include "codec_ati.hpp"
#include "base/graphics.hpp"

/****************************************************************************/
/****************************************************************************/

CodecATI::CodecATI() 
{
}

CodecATI::~CodecATI() 
{
}

const sChar *CodecATI::GetName()
{
  return L"ATI";
}

/****************************************************************************/

void CodecATI::Pack(sImage *bmp, sImageData *dxt, sInt level)
{ 
  sImage *bmpdel=0;
  bmpdel = bmp->Copy();
  bmp = bmpdel;
  if(level == sTEX_DXT1A)
  {
    for(sInt i=0;i<bmp->SizeX*bmp->SizeY;i++)
    {
      if(bmp->Data[i]>=0x80000000) 
        bmp->Data[i]|=0xff000000; 
      else 
        bmp->Data[i]&=0x00ffffff;
    }
  }

  ATI_TC_Texture texture, texture2;

  texture.dwSize = sizeof(texture);
  texture.dwHeight = bmp->SizeY;
  texture.dwWidth = bmp->SizeX;
  texture.dwPitch = bmp->SizeX*sizeof(bmp->Data);
  texture.dwDataSize = bmp->SizeX*bmp->SizeY*sizeof(bmp->Data);
  texture.pData = (unsigned char *)bmp->Data;

  texture2.dwSize = sizeof(texture2);
  texture2.dwHeight = dxt->SizeY;
  texture2.dwWidth = dxt->SizeX;
  texture2.dwPitch = dxt->SizeX*sizeof(dxt->BitsPerPixel);
  texture2.dwDataSize = dxt->SizeX*dxt->SizeY*sizeof(dxt->BitsPerPixel);
  texture2.pData = (unsigned char *)dxt->Data;
  
  texture.format = ATI_TC_FORMAT_ARGB_8888;

  switch(level)
  {
  case sTEX_DXT1:
    texture2.format = ATI_TC_FORMAT_DXT1;
    break;
  case sTEX_DXT1A:
    texture2.format = ATI_TC_FORMAT_DXT1;
    break;
  case sTEX_DXT3:
    texture2.format = ATI_TC_FORMAT_DXT3;
    break;
  case sTEX_DXT5:
    texture2.format = ATI_TC_FORMAT_DXT5;
    break;
  default:
    return;
  }

  ATI_TC_ConvertTexture(&texture, &texture2, NULL, NULL, NULL, NULL);
}

void CodecATI::Unpack(sImage *bmp,sImageData *dxt,sInt level)
{

}

/****************************************************************************/
/****************************************************************************/

#endif

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "config.hpp"
#if LINK_NVIDIA

/****************************************************************************/

#include "codec_nvidia.hpp"
#include "base/graphics.hpp"
#include "base/system.hpp"
#include <stdio.h>

/****************************************************************************/
/****************************************************************************/

CodecNVIDIA::CodecNVIDIA() 
{
}

CodecNVIDIA::~CodecNVIDIA() 
{
}

const sChar *CodecNVIDIA::GetName()
{
  return L"nVIDIA";
}

/****************************************************************************/

void CodecNVIDIA::Pack(sImage *bmp,sImageData *dxt,sInt level)
{
  bmp->Save(L"test.tga");

  switch(level)
  {
  case sTEX_DXT1:
    sExecuteShell(L"nvdxt.exe -file test.tga -quick -dxt1c");
    break;
  case sTEX_DXT1A:
    sExecuteShell(L"nvdxt.exe -file test.tga -quick -dxt1a");
    break;
  case sTEX_DXT3:
    sExecuteShell(L"nvdxt.exe -file test.tga -quick -dxt3");
    break;
  case sTEX_DXT5:
    sExecuteShell(L"nvdxt.exe -file test.tga -quick -dxt5");
    break;
  default:
    return;
  }

  sImage temp;
  temp.Copy(bmp);

  temp.Load(L"test.tga");

  remove("test.tga");
  remove("test.dds");

  dxt->ConvertFrom(&temp);
}

void CodecNVIDIA::Unpack(sImage *bmp,sImageData *dxt,sInt level)
{

}

/****************************************************************************/
/****************************************************************************/

#endif


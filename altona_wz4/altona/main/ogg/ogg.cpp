/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "ogg.hpp"
#define STB_VORBIS_NO_STDIO
#include "stb_vorbis.h"

/****************************************************************************/

sOGGDecoder::sOGGDecoder() 
{
  dec = 0;
}

sOGGDecoder::~sOGGDecoder() 
{
  if(dec)
    stb_vorbis_close(dec);
}

sBool sOGGDecoder::Init(sInt songnr) 
{
  if(dec)
  {
    stb_vorbis_close(dec);
    dec = 0;
  }
  int error = 0;
  dec = stb_vorbis_open_memory(Stream,StreamSize,&error,0);  
  return dec!=0;
}

sInt sOGGDecoder::Render(sS16 *stream,sInt samples) 
{
  return stb_vorbis_get_samples_short_interleaved(dec,2,stream,samples*2);
}

sInt sOGGDecoder::GetTuneLength() 
{
  return stb_vorbis_stream_length_in_samples(dec);
}

/****************************************************************************/

/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_WZ4_AUDIO_HPP
#define FILE_WZ4FRLIB_WZ4_AUDIO_HPP

#include "base/types.hpp"

class sImage;

/****************************************************************************/

namespace Wz4Audiolyzer
{
  sBool AudioAvailable();
  void RMSImage(sImage *out,sInt width,sInt height,sInt chunkSize);
  void Spectogram(sImage *out,sInt width,sInt fftSize);
}

/****************************************************************************/

#endif // FILE_WZ4FRLIB_WZ4_AUDIO_HPP


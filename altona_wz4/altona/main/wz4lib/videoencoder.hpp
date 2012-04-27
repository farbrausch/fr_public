/****************************************************************************/

#ifndef FILE_WZ4LIB_VIDEOENCODER_HPP
#define FILE_WZ4LIB_VIDEOENCODER_HPP

#include "base/types.hpp"

/****************************************************************************/

class sImage;
class sTexture2D;

class sVideoEncoder
{
public:
  struct CodecInfo
  {
    sU32 FourCC;
    sInt Quality;
    sStaticArray<sU8> CodecData;
  };

  virtual ~sVideoEncoder() {}

  virtual void SetSize(sInt xRes,sInt yRes) = 0;
  
  virtual void WriteFrame(const sImage *img) = 0;
  virtual void WriteFrame(sTexture2D *RT=0) = 0;

  virtual void SetAudioFormat(sInt bits, sInt channels, sInt rate) = 0;
  virtual void WriteAudioFrame(const void *buffer,int samples) = 0;
};


sBool sGetVideoCodecName(sU32 codec, const sStringDesc &str);
void  sChooseVideoCodec(sVideoEncoder::CodecInfo &info, sU32 basefourcc=0);
sVideoEncoder *sCreateVideoEncoder(const sChar *filename, sF32 fps, sVideoEncoder::CodecInfo *info=0);

/****************************************************************************/

#endif // FILE_WZ4LIB_VIDEOENCODER_HPP

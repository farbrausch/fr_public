// This file is distributed under a BSD license. See LICENSE.txt for details.

// FRIED
// internal interface (everything that shouldn't be visible to the public)

#ifndef __FRIED_INTERNAL_HPP__
#define __FRIED_INTERNAL_HPP__

#include "_types.hpp"

namespace FRIED
{
  enum ChannelType
  {
    CHANNEL_NONE  = 0,
    CHANNEL_Y     = 1,
    CHANNEL_CO    = 2,
    CHANNEL_CG    = 3,
    CHANNEL_ALPHA = 4,
    // just allocate other channel types as required
  };

  // channel header
  struct ChannelHeader
  {
    sInt Type;                      // channel type tag
    sInt Quantizer;                 // quantizer factor
    sInt StripeOffset;              // offset in stripe data
    sInt ChunkOffset;               // offset in chunk data
  };

  // file header
  struct FileHeader
  {
    sChar Signature[8];             // FRIEDxxx (xxx=version number)
    sInt XRes;                      // width of image
    sInt YRes;                      // height of image
    sInt VirtualXRes;               // virtual width of image
    sInt ChunkWidth;                // chunk width
    sInt Channels;                  // # of channels used (max 16)
  };

  // encode context
  struct EncodeContext
  {
    FileHeader FH;
    ChannelHeader Chans[16];
    sInt XResPadded;
    sInt YResPadded;
    sInt *SB;                       // stripe buffer (32 lines)
    sInt *QB;                       // quantized buffer (16 lines)
    sInt *CK;                       // chunk (unquantized) buffer

    sU8 *Bits;                      // packed buffer
    sInt BitsLength;                // length of packed buffer

    const sU8 *Image;               // source image pointer
    sInt Flags;                     // encoding flags
  };

  // decode context
  struct DecodeContext
  {
    FileHeader FH;
    ChannelHeader Chans[16];
    sInt XResPadded;
    sInt YResPadded;
    sS16 *SB;                       // stripe buffer (32 lines)
    sInt *QB;                       // quantized buffer (16 lines)
    sS16 *CK;                       // chunk (unquantized) buffer

    sU8 *Image;                     // destination image pointer
    sInt ChannelSetup;              // channel setup number
  };

  // entropy coding
  sInt rlgrenc(sU8 *bits,sInt nbmax,sInt *x,sInt n,sInt xminit);
  sInt rlgrdec(const sU8 *bits,sInt nbmax,sS16 *y,sInt n,sInt xminit);

  // quantization
  sInt newQuantize(sInt qs,sInt *x,sInt npts,sInt cwidth);
  void newDequantize(sInt qs,sS16 *x,sInt npts,sInt cwidth);

  // transforms
  void ndct42D(sInt *x0,sInt *x1,sInt *x2,sInt *x3);
  void indct42D(sS16 *x0,sS16 *x1,sS16 *x2,sS16 *x3);
  
  void ndct42D_MB(sInt *x0,sInt *x,sInt *x2,sInt *x3);
  void indct42D_MB(sS16 *x0,sS16 *x1,sS16 *x2,sS16 *x3);

  void lbt4pre2x4(sInt *x0,sInt *x1);
  void lbt4post2x4(sS16 *x0,sS16 *x1);
  void lbt4pre4x2(sInt *x0,sInt *x1,sInt *x2,sInt *x3);
  void lbt4post4x2(sS16 *x0,sS16 *x1,sS16 *x2,sS16 *x3);
  void lbt4pre4x4(sInt *x0,sInt *x1,sInt *x2,sInt *x3);
  void lbt4post4x4(sS16 *x0,sS16 *x1,sS16 *x2,sS16 *x3);

  // pixel processing
  void gray_alpha_convert_dir(sInt cols,sInt colsPad,const sU8 *src,sInt *dst);
  void gray_alpha_convert_inv(sInt cols,sInt colsPad,const sS16 *src,sU8 *dst);
  void gray_x_convert_dir(sInt cols,sInt colsPad,const sU8 *src,sInt *dst);
  void gray_x_convert_inv(sInt cols,sInt colsPad,const sS16 *src,sU8 *dst);
  void color_alpha_convert_dir(sInt cols,sInt colsPad,const sU8 *src,sInt *dst);
  void color_alpha_convert_inv(sInt cols,sInt colsPad,const sS16 *src,sU8 *dst);
  void color_x_convert_dir(sInt cols,sInt colsPad,const sU8 *src,sInt *dst);
  void color_x_convert_inv(sInt cols,sInt colsPad,const sS16 *src,sU8 *dst);
}

#endif

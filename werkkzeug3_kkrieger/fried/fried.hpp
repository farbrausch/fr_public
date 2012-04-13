// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __FRIED_HPP__
#define __FRIED_HPP__

// Farbrausch Image Encoder/Decoder

// Save options
#define FRIED_DEFAULT         0x0000
#define FRIED_GRAYSCALE       0x0001
#define FRIED_SAVEALPHA       0x0002
#define FRIED_CHROMASUBSAMPLE 0x0004 // not implemented yet

// Loading/saving
sBool LoadFRIED(const sU8 *data,sInt size,sInt &xout,sInt &yout,sU8 *&dataout);
sU8 *SaveFRIED(const sU8 *image,sInt xsize,sInt ysize,sInt flags,sInt quality,sInt &outsize);

#endif

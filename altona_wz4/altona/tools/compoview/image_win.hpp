/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) 2005-2006 by Dierk Ohlerich                          ***/
/***   all rights reserved                                                ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_CUBE_IMAGE_WIN_HPP
#define FILE_CUBE_IMAGE_WIN_HPP

#include "base/types.hpp"

/****************************************************************************/

class sImage;
class sFile;

sImage *sLoadImageWin32(sFile *file);
sImage *sLoadImageWin32(const sChar *name);
sBool  sLoadImageWin32(sFile *file, sImage &image);
sBool  sLoadImageWin32(const sChar *name, sImage &image);

/****************************************************************************/

#endif // FILE_CUBE_IMAGE_WIN_HPP


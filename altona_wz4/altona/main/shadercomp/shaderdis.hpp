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

#ifndef HEADER_ALTONA_UTIL_SHADERDIS
#define HEADER_ALTONA_UTIL_SHADERDIS

#ifndef __GNUC__
#pragma once
#endif


#include "base/types.hpp"

void sPrintShader(const sU32 *shader,sInt flags=0);
void sPrintShader(class sTextBuffer& tb, const sU32 *shader,sInt flags=0);
void sPrintShader(class sTextBuffer& tb, struct sShaderBlob *blob, sInt flags=0);

enum sPrintShaderFlags
{
  sPSF_CARRAY = 0x0001,           // print in the style of a C++ array
  sPSF_NOCOMMENTS = 0x0002,        // skip comments. can be used with sPSF_CARRAY to strip comments
};

/****************************************************************************/

// HEADER_ALTONA_UTIL_SHADERDIS
#endif

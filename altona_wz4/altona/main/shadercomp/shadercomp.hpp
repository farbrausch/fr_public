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

#ifndef HEADER_ALTONA_UTIL_SHADERS
#define HEADER_ALTONA_UTIL_SHADERS

#ifndef __GNUC__
#pragma once
#endif

#include "base/types2.hpp"
#include "base/graphics.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   The individual real compilers                                      ***/
/***                                                                      ***/
/****************************************************************************/

sBool sShaderCompileDX     (const sChar *source,const sChar *profile,const sChar *main,sU8 *&data,sInt &size,sInt flags=0,sTextBuffer *errors=0);
sBool sShaderCompileCG     (const sChar *source,const sChar *profile,const sChar *main,sU8 *&data,sInt &size,sInt flags=0,sTextBuffer *errors=0);

/****************************************************************************/
/***                                                                      ***/
/***   Shader compiler                                                    ***/
/***                                                                      ***/
/****************************************************************************/

// external compiler stuff.

struct sExternCompileBuffer
{
  sExternCompileBuffer(sInt size0, sInt size1) { Buffer = new sU8[size0]; BufferSize = size0; ResultSize = 0; Message = new sChar8[size1]; Message[0]=0; MessageSize = size1; }
  ~sExternCompileBuffer()            { sDeleteArray(Buffer); sDeleteArray(Message); }

  sU8 *Buffer;
  sInt BufferSize;
  sInt ResultSize;
  sChar8 *Message;
  sInt MessageSize;

  void SetShader(const sU8 *data, sInt size)  { for(sInt i=0;i<sMin(size,BufferSize);i++) Buffer[i]=data[i]; ResultSize = size; }
  void SetMessage(const sChar8 *msg)          { sInt i=0; for(;i<MessageSize-1&&*msg;i++) Message[i] = *msg++; Message[i] = 0; }
};

typedef sBool (*sCompileCallback)(sExternCompileBuffer *buffer, sInt stype, sInt dtype, sInt flags, const sChar8 *source, sInt len, const sChar8 *name);
//sBool sCompileExtern(sCompileCallback cb, sCompileResult &result, sInt stype, sInt dtype, sInt flags, const sChar8 *source, sInt len, const sChar8 *name);

/****************************************************************************/

#endif

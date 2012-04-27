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

#include "base/graphics.hpp"

#if sCONFIG_SDK_CG

#include "Cg/cg.h"
#pragma comment(lib,"cg.lib")

/****************************************************************************/

static CGcontext CGC;

static void CGInit()
{
  CGC = cgCreateContext();  
}

static void CGExit()
{
  cgDestroyContext(CGC);
}

static sBool CGError(sPoolString &errors)
{
  sString<0x4000> buffer;
  CGerror error;
  const sChar8 *string = cgGetLastErrorString(&error);

  if(error==CG_NO_ERROR) return 0;
  if(error==CG_COMPILER_ERROR)
  {
    const sChar8* ll = (const sChar8 *)cgGetLastListing(CGC);
    if(ll)
    {
      sCopyString(buffer,ll,sCOUNTOF(buffer));
      sPrint(buffer);
    }
  }

  sCopyString(buffer,string,sCOUNTOF(buffer));
  errors = buffer;
  return 1;
}

static sBool CGCompile(const sChar *source,const sChar *profile,sPoolString &result,sPoolString &errors)
{
  sChar8 *source8=0;
  const sChar8 *out8;
  sChar *out=0;
  sInt size;
  sChar8 prof8[64];
  sCopyString(prof8,profile,sCOUNTOF(prof8));

  sDPrintF(L"\n-------- <%s> --------------------\n",profile);
  sDPrint(source);
  sDPrint(L"\n----------------------------\n");

  CGprofile prof = cgGetProfile(prof8);
  size = sGetStringLen(source)+1;
  source8 = new sChar8[size];
  sCopyString(source8,source,size);
  CGprogram program = cgCreateProgram(CGC,CG_SOURCE,source8,prof,0,0);
  if(CGError(errors)) goto error;
  delete[] source8;
  cgCompileProgram(program);

  out8 = cgGetProgramString(program,CG_COMPILED_PROGRAM);
  if(CGError(errors)) goto error;
  size=0; while(out8[size]) size++;
  size++;
  out = new sChar[size];
  sCopyString(out,out8,size);
  result = out;
  delete[] out;

  errors=L"ok";
  return 1;
error:
  delete[] out;
  delete[] source8;
  return 0;
}

sBool sShaderCompileCG(const sChar *source,const sChar *profile,const sChar *main,sU8 *&data_,sInt &size_,sInt flags,sTextBuffer *errors)
{
  sChar8 *source8=0;
  const sChar8 *out8 = 0;
  sInt insize;
  sPoolString errstr;
  sChar8 prof8[64];
  sCopyString(prof8,profile,sCOUNTOF(prof8));
  sChar8 entry[64];
  sCopyString(entry,main,sCOUNTOF(entry));
  CGInit();

//  sDPrintF(L"\n----------<%s>------------------\n",profile);
//  sDPrint(source);
//  sDPrint(L"\n----------------------------\n");


  CGprofile prof = cgGetProfile(prof8);
  insize = sGetStringLen(source)+1;
  source8 = new sChar8[insize];
  sCopyString(source8,source,insize);
  CGprogram program = cgCreateProgram(CGC,CG_SOURCE,source8,prof,entry,0);

  if(CGError(errstr))
  {
    if(errors)
      errors->PrintF(L"%s (CG_PC)\n",errstr);
    goto error;
  }
  cgCompileProgram(program);

  out8 = cgGetProgramString(program,CG_COMPILED_PROGRAM);
  if(CGError(errstr))
  {
    if(errors)
      errors->PrintF(L"%s (CG_PC)\n",errstr);
    goto error;
  }
  size_=0; while(out8[size_]) size_++;
  size_++;
  data_ = new sU8[size_];
  sCopyMem(data_,out8,size_);

  delete[] source8;
  CGExit();
  return 1;
error:
  delete[] source8;  
  CGExit();
  return 0;
}

#else

sBool sShaderCompileCG(const sChar *source,const sChar *profile,const sChar *main,sU8 *&data_,sInt &size_,sInt flags,sTextBuffer *errors)
{
  return 0;
}

#endif

/****************************************************************************/


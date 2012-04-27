/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "base/types.hpp"
#include "shadercomp.hpp"

#if sPLATFORM==sPLAT_WINDOWS

#undef new
#include <d3dx9.h>
#include <d3dx11.h>
#include <d3dcompiler.h>

#pragma comment(lib,"d3dx9.lib")
#pragma comment(lib,"d3dcompiler.lib")
#define new sDEFINE_NEW


sBool sShaderCompileDX(const sChar *source,const sChar *profile,const sChar *main,sU8 *&data,sInt &size,sInt flags,sTextBuffer *errors)
{
  ID3D10Blob *bytecode;
  ID3D10Blob *dxerrors;
  sBool result;

  result = 0;
  data = 0;
  size = 0;


  // unicode -> ansi conversion (cheap)

  sChar8 profile8[16];
  sCopyString(profile8,profile,sCOUNTOF(profile8));

  sChar8 main8[256];
  sCopyString(main8,main,sCOUNTOF(main8));

  sInt len = sGetStringLen(source);
  sChar8 *src8 = new sChar8[len+1];
  sCopyString(src8,source,len+1);

  // figure out flags

  UINT flags1 = 0;
  UINT flags2 = 0;
  if(flags&sSCF_DEBUG) flags1 |= D3D10_SHADER_DEBUG;
  if(flags&sSCF_AVOID_CFLOW) flags1 |= D3D10_SHADER_AVOID_FLOW_CONTROL;
  if(flags&sSCF_PREFER_CFLOW) flags1 |= D3D10_SHADER_PREFER_FLOW_CONTROL;
  if(flags&sSCF_DONT_OPTIMIZE) flags1 |= D3D10_SHADER_SKIP_OPTIMIZATION;

  HRESULT hr = D3DCompile(src8,len,0,0,0,main8,profile8,flags1,flags2,&bytecode,&dxerrors);

  // print errors and warnings

  if(dxerrors)
  {
    sInt elen = dxerrors->GetBufferSize();
    const sChar8 *estr = (const sChar8*)dxerrors->GetBufferPointer();
    for(sInt i=0;i<elen;i++)
      if(estr[i])
        errors->PrintChar(estr[i]);
    dxerrors->Release();
  }

  // get bytecode

  if(bytecode && !FAILED(hr))
  {
    size = bytecode->GetBufferSize();
    data = new sU8[size];
    sCopyMem(data,bytecode->GetBufferPointer(),size);
    bytecode->Release();

    result = 1;
  }

  // done

  delete[] src8;
  return result;
}

#else

sBool sShaderCompileDX(const sChar *source,const sChar *profile,const sChar *main,sU8 *&data,sInt &size,sInt flags,sTextBuffer *errors)
{
  return 0;
}

#endif

/****************************************************************************/


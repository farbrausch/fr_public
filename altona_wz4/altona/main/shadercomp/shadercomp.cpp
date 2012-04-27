/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "shadercomp/shadercomp.hpp"
#include "base/types.hpp"
#include "base/system.hpp"
#include "shadercomp/shaderdis.hpp"

#if sCONFIG_SDK_TC3
#include "util_tc3/util.hpp"
#endif

#if sCONFIG_SDK_CG && (sPLATFORM == sPLAT_WINDOWS || sPLATFORM == sPLAT_LINUX) /*&& !sCONFIG_64BIT*/    // we want cg compiler in 64bit mode (64bit xsi shader plugin!)
#define sCOMP_CG_ENABLE 1
#else
#define sCOMP_CG_ENABLE 0
#endif

#if sCONFIG_SDK_DX9 && sPLATFORM == sPLAT_WINDOWS
#define sCOMP_DX9_ENABLE 1
#else
#define sCOMP_DX9_ENABLE 0
#endif

#if sCONFIG_SDK_DX11 && sPLATFORM == sPLAT_WINDOWS
#define sCOMP_DX11_ENABLE 1
#else
#define sCOMP_DX11_ENABLE 0
#endif

/****************************************************************************/

#if sCOMP_DX9_ENABLE

#undef new
#include <d3dx9.h>
#include <d3dx11.h>
#include <d3dcompiler.h>

#pragma comment(lib,"d3dx9.lib")
#pragma comment(lib,"d3dcompiler.lib")
#define new sDEFINE_NEW

#endif

/****************************************************************************/
/*
#undef new
#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10math.h>
#define new sDEFINE_NEW

typedef HRESULT (WINAPI *DX11CompileType)(
  LPCVOID pSrcData,
  SIZE_T SrcDataSize,
  LPCSTR pSourceName,
  CONST D3D10_SHADER_MACRO *pDefines,
  LPD3D10INCLUDE pInclude,
  LPCSTR pEntrypoint,
  LPCSTR pTarget,
  UINT Flags1,
  UINT Flags2,
  LPD3D10BLOB *ppCode,
  LPD3D10BLOB *ppErrorMsgs
);
typedef HRESULT (WINAPI *DX11DisassembleType)(
  LPCVOID pSrcData,
  SIZE_T SrcDataSize,
  UINT Flags,
  LPD3D10BLOB *szComments,
  LPD3D10BLOB *ppDisassembly
);

static HMODULE DX11Lib;
static DX11CompileType DX11Compile;
static DX11DisassembleType DX11Disassemble;

#endif
*/
/****************************************************************************/

#if sCOMP_CG_ENABLE

#include "cg/cg.h"
#pragma comment(lib,"cg.lib")
static CGcontext CGC=0;

#endif

/****************************************************************************/

void sPrintBlob(sTextBuffer &tb, sArray<sU8> &blob);

/****************************************************************************/
/***                                                                      ***/
/***   Shader compiler                                                    ***/
/***                                                                      ***/
/****************************************************************************/

sCompileResult::sCompileResult()
{
  Valid = sFALSE;
  Errors = L"";
#if sCOMP_DX9_ENABLE
  sSetMem(&D3D9,0,sizeof(D3D9));
#endif
#if sCOMP_CG_ENABLE
  sSetMem(&Cg,0,sizeof(Cg));
#endif
}

sCompileResult::~sCompileResult()
{
  Reset();
}

void sCompileResult::Reset()
{
#if sCOMP_DX9_ENABLE
  sRelease(D3D9.CTable);
  sRelease(D3D9.Errors);
#endif // sCOMP_DX9_ENABLE
#if sCOMP_CG_ENABLE
  if(Cg.Program)
    cgDestroyProgram(Cg.Program);
#endif // sCOMP_CG_ENABLE
  ShaderBlobs.Reset();
  Valid = sFALSE;
  Errors = L"";
}


sShaderBlob *sCompileResult::GetBlob()
{ 
  if(ShaderBlobs.GetCount())
    return (sShaderBlob*)&ShaderBlobs[0];
  return 0;
}

/****************************************************************************/

static const sInt MaxCompilerCount = 32;
static struct sShaderCompiler
{
  sCompilerFunc Func;
  sInt SrcType;
  sInt DstType;
} ShaderCompiler[MaxCompilerCount];
static sInt CompilerCount=0;


sBool sShaderCompile(sCompileResult &result, sInt stype, sInt dtype, const sChar *source, sInt len, sInt flags /*=0*/, const sChar *name/*=0*/)
{
  for(sInt i=0;i<CompilerCount;i++)
  {
    if((ShaderCompiler[i].SrcType&(sSTF_PLATFORM|sSTF_PROFILE)) == (stype&(sSTF_PLATFORM|sSTF_PROFILE)) && (ShaderCompiler[i].DstType&(sSTF_PLATFORM|sSTF_PROFILE)) == (dtype&(sSTF_PLATFORM|sSTF_PROFILE)))
    {
      sChar8 *buffer = new sChar8[len];
      sCopyString(buffer,source,len);
      if(!name) name = L"main";
      sChar8 name8[64];
      sCopyString(name8,name,64);
      sBool check = (*ShaderCompiler[i].Func)(result,stype,dtype,flags,buffer,len,name8);
      sDeleteArray(buffer);
      return check;
    }
  }
  sPrintF(L"no shader compiler found for 0x%08x -> 0x%08x\n",stype,dtype);
  return sFALSE;
}

void sRegisterCompiler(sInt stype, sInt dtype, sCompilerFunc func)
{
  sVERIFY(CompilerCount<MaxCompilerCount);
  sShaderCompiler *c = &ShaderCompiler[CompilerCount++];
  c->Func = func;
  c->SrcType = stype;
  c->DstType = dtype;
}

const sChar8* GetProfile(sInt type)
{
  switch(type&sSTF_KIND)
  {
  case sSTF_VERTEX:
    switch(type&(sSTF_PLATFORM|sSTF_PROFILE))
    {
//    case sSTF_DX_00:    return "";
//    case sSTF_DX_11:    return "vs_1_1";
//    case sSTF_DX_13:    return "vs_1_3";
    case sSTF_DX_20:    return "vs_2_0";
    case sSTF_DX_30:    return "vs_3_0";
    case sSTF_DX_40:    return "vs_4_0";
    case sSTF_DX_50:    return "vs_5_0";

    case sSTF_NVIDIA:   return "vp40";
    }
    sVERIFY(0);
    break;
  case sSTF_PIXEL:
    switch(type&(sSTF_PLATFORM|sSTF_PROFILE))
    {
//    case sSTF_DX_00:    return "";
//    case sSTF_DX_11:    return "ps_1_1";
//    case sSTF_DX_13:    return "ps_1_3";
    case sSTF_DX_20:    return "ps_2_0";
    case sSTF_DX_30:    return "ps_3_0";
    case sSTF_DX_40:    return "ps_4_0";
    case sSTF_DX_50:    return "ps_5_0";

    case sSTF_NVIDIA:   return "fp40";
    }
    sVERIFY(0);
    break;
  case sSTF_GEOMETRY:
    sVERIFY(0);
    break;
  }
  return 0;
}


static sBool sCompileDX9(sCompileResult &result, sInt stype, sInt dtype, sInt flags, const sChar8 *src, sInt len, const sChar8 *name)
{
#if sCOMP_DX9_ENABLE
  ID3DXBuffer *bytecode;

  sRelease(result.D3D9.CTable);
  sRelease(result.D3D9.Errors);

  sU32 d3dflags = 0;
  if(flags&sSCF_DEBUG)
    d3dflags |= D3DXSHADER_DEBUG;
  //if (sGetShellSwitch(L"n"))
  //  flags |= D3DXSHADER_SKIPOPTIMIZATION; // only use in case of emergency

  const sChar8 *profile8 = GetProfile(dtype);
  sChar profile[16];
  sCopyString(profile,profile8,16);

  // use old compiler for generating shader model 1_* code
#ifdef D3DXSHADER_USE_LEGACY_D3DX9_31_DLL
  if(sMatchWildcard(L"ps_1_*",profile))
    d3dflags |= D3DXSHADER_USE_LEGACY_D3DX9_31_DLL;
#endif

  if(D3DXCompileShader(src,len,0,0,name,profile8,d3dflags,&bytecode,&result.D3D9.Errors,&result.D3D9.CTable)!=D3D_OK)
    result.Valid = sFALSE;
  else
    result.Valid = sTRUE;

  // print errors and warnings
  if(result.D3D9.Errors)
  {
    ID3DXBuffer *buffer = result.D3D9.Errors;
    //sInt size = buffer->GetBufferSize();
    sCopyString(result.Errors,(sChar8*)buffer->GetBufferPointer(),result.Errors.Size());
  }

  if(!result.Valid)
  {
    sRelease(bytecode);
    return sFALSE;
  }

  // get source code
  sAddShaderBlob(result.ShaderBlobs,dtype,bytecode->GetBufferSize(),(const sU8*)bytecode->GetBufferPointer());
  return result.Valid;
#else
  return sFALSE;
#endif // sCOMP_DX9_ENABLE
}

/****************************************************************************/


/****************************************************************************/

static sBool GetCgError(const sStringDesc &errors)
{
#if sCOMP_CG_ENABLE
  CGerror error;
  const sChar8 *string = cgGetLastErrorString(&error);

  if(error==CG_NO_ERROR) return sFALSE;

  sErrorString temp0;
  sErrorString temp1;
  sCopyString(temp0,string,0x4000);

  if(error==CG_COMPILER_ERROR)
  {
    const sChar8 *msg = cgGetLastListing(CGC);
    if(msg)
      sCopyString(temp1,msg,0x4000);
  }
  sSPrintF(errors,L"%s\n%s",temp0,temp1);
  return sTRUE;
#else
  return sTRUE;
#endif
}

static sBool sCompileCg(sCompileResult &result, sInt stype, sInt dtype, sInt flags, const sChar8 *src, sInt len, const sChar8 *name)
{
#if sCOMP_CG_ENABLE
  if(result.Cg.Program)
    cgDestroyProgram(result.Cg.Program);

  sChar8 *src8 = new sChar8[len];
  sCopyString(src8,src,len);

  CGprofile prof = cgGetProfile(GetProfile(dtype));
  CGprogram program = cgCreateProgram(CGC,CG_SOURCE,src8,prof,name,0);

  if(GetCgError(result.Errors)) goto error;

  cgCompileProgram(program);
  if(GetCgError(result.Errors)) goto error;
  const sChar8 *out8 = cgGetProgramString(program,CG_COMPILED_PROGRAM);
  if(GetCgError(result.Errors)) goto error;
  sInt size = 0;
  while(out8[size]) size++;

  sAddShaderBlob(result.ShaderBlobs,dtype,size,(const sU8*)out8);

  result.Valid = sTRUE;
  sDeleteArray(src8);
  return sTRUE;

error:

  result.Valid = sFALSE;
  sDeleteArray(src8);
  return sFALSE;
#else
  sLogF(L"asc",L"sCOMP_CG_ENABLE == 0 no cg compiler available\n");
  return sFALSE;
#endif // sCOMP_CG_ENABLE
}


static sBool sCompileDummy(sCompileResult &result, sInt stype, sInt dtype, sInt flags, const sChar8 *src, sInt len, const sChar8 *name)
{
  sAddShaderBlob(result.ShaderBlobs,dtype,4,(const sU8 *)"dumy");
  result.Valid = sTRUE;
  return sTRUE;
}

/****************************************************************************/


void sShaderCompilerInit()
{
#if sCOMP_CG_ENABLE
  CGC = cgCreateContext();
#endif
//  sRegisterCompiler(sSTF_HLSL,sSTF_DX_00,sCompileDX9);
//  sRegisterCompiler(sSTF_HLSL,sSTF_DX_11,sCompileDX9);
//  sRegisterCompiler(sSTF_HLSL,sSTF_DX_13,sCompileDX9);
  sRegisterCompiler(sSTF_HLSL23,sSTF_DX_20,sCompileDX9);
  sRegisterCompiler(sSTF_HLSL23,sSTF_DX_30,sCompileDX9);
//  sRegisterCompiler(sSTF_HLSL45,sSTF_DX_20,sCompileDX11);
//  sRegisterCompiler(sSTF_HLSL45,sSTF_DX_30,sCompileDX11);
//  sRegisterCompiler(sSTF_CG,sSTF_DX_00,sCompileCg);
//  sRegisterCompiler(sSTF_CG,sSTF_DX_13,sCompileCg);
  sRegisterCompiler(sSTF_CG,sSTF_DX_20,sCompileCg);
  sRegisterCompiler(sSTF_CG,sSTF_DX_30,sCompileCg);
  sRegisterCompiler(sSTF_CG,sSTF_NVIDIA,sCompileCg);
  sRegisterCompiler(sSTF_HLSL23,sSTF_NVIDIA,sCompileCg);
}

void sShaderCompilerExit()
{
#if sCOMP_CG_ENABLE
  cgDestroyContext(CGC);
  CGC = 0;
#endif
  CompilerCount = 0;
}

void sAddShaderBlob(sArray<sU8> &blob, sInt type, sInt bytes, const sU8 *data)
{
  if(blob.IsEmpty())
  {
    sInt* dst = (sInt*) blob.AddMany(4);
    *dst = sSTF_NONE;
  }

  sInt count = (bytes+3)&~3;
  sShaderBlob *b = (sShaderBlob*)(blob.AddMany(8+count)-4);
  b->Type = type;
  b->Size = bytes;
  sCopyMem(b->Data,data,bytes);
  while(bytes&3) b->Data[bytes++] = 0;  // pad with zeroes
  b->SetNext(sSTF_NONE);  
}

void sPrintBlob(sTextBuffer &tb, sArray<sU8> &blob)
{
  tb.Print(L"  ");
  for(sInt i=0;i<blob.GetCount();i++)
  {
    tb.PrintF(L"0x%02x,",blob[i]);
    if((i%16)==15)
      tb.Print(L"\n  ");
  }
}

/****************************************************************************/

/****************************************************************************/

sBool sShaderCompileDX(const sChar *source,const sChar *profile,const sChar *main,sU8 *&data,sInt &size,sInt flags,sTextBuffer *errors)
{
#if sCOMP_DX9_ENABLE
  ID3D10Blob *bytecode;
  ID3D10Blob *dxerrors;
  sBool result;

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
  if(flags&sSCF_DEBUG)
    flags1 |= D3D10_SHADER_DEBUG;

//#ifdef D3DXSHADER_USE_LEGACY_D3DX9_31_DLL
//  if(sGetShellSwitch(L"-d3dold") || sMatchWildcard(L"ps_1_*",profile))
//    flags1 |= D3DXSHADER_USE_LEGACY_D3DX9_31_DLL;
//#endif

//  result = (D3DXCompileShader(src8,len,0,0,main8,profile8,d3dflags,&bytecode,&dxerrors,0)==D3D_OK);
  result = !FAILED(D3DCompile(src8,len,0,0,0,main8,profile8,flags1,flags2,&bytecode,&dxerrors));

  // print errors and warnings

//  errors->Clear();    // do not clear, this will overwrite old errors
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

  if(bytecode)
  {
    size = bytecode->GetBufferSize();
    data = new sU8[size];
    sCopyMem(data,bytecode->GetBufferPointer(),size);
    bytecode->Release();
  }
  else
  {
    result = 0;
    data = 0;
    size = 0;
  }

  // done

  delete[] src8;
  return result;
#else
  return sFALSE;
#endif // sCOMP_DX9_ENABLE
}

/****************************************************************************/
/***                                                                      ***/
/***   external shader compilers                                          ***/
/***                                                                      ***/
/****************************************************************************/
#if sPLATFORM == sPLAT_WINDOWS

sBool sCompileExtern(sCompileCallback cb, sCompileResult &result, sInt stype, sInt dtype, sInt flags, const sChar8 *source, sInt len, const sChar8 *name)
{
  if(cb)
  {
    sExternCompileBuffer buffer(128*1024,4*1024);
    result.Valid = (*cb)(&buffer,stype,dtype,flags,source,len,name);
    if(result.Valid)
      sAddShaderBlob(result.ShaderBlobs,dtype,buffer.ResultSize,buffer.Buffer);
    else
    {
      sChar error[4096];
      sCopyString(error,buffer.Message,4096);
      sLogF(L"asc",L"extern compiler error:\n%s\n",error);
      result.Errors = error;
    }
    return result.Valid;
  }
  return sFALSE;
}

#endif // sPLATFORM == sPLAT_WINDOWS

/****************************************************************************/

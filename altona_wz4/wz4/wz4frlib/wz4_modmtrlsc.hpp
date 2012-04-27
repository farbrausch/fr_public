/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_WZ4_MODMTRLSC_HPP
#define FILE_WZ4FRLIB_WZ4_MODMTRLSC_HPP

#include "base/types.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Code Generator                                                     ***/
/***                                                                      ***/
/****************************************************************************/

enum ShaderCreatorEnum
{
  SCT_FLOAT = 1,                  // types
  SCT_FLOAT2,
  SCT_FLOAT3,
  SCT_FLOAT4,
  SCT_INT,
  SCT_INT2,
  SCT_INT3,
  SCT_INT4,
  SCT_UINT,
  SCT_UINT2,
  SCT_UINT3,
  SCT_UINT4,
  SCT_BOOL,
  SCT_FLOAT3x4,                   // row major 3x4 will fill 3 float4's , ready for DP3
  SCT_FLOAT4x3,                   // this is useless
  SCT_FLOAT4x4,                   // full matrix
  SCT_SKINNING,                   // all the skinning matrices (uniform only)
  SCT_FLOAT4A8,                   // array
  SCT_FLOAT4A4,                   // array
  SCT_MAX,

  SCB_POS = 1,                    // position from vertex shader to pixel shader
  SCB_POSVERT,                    // position from vertex buffer to vertex shader
  SCB_NORMAL,
  SCB_TANGENT,
  SCB_COLOR,
  SCB_BLENDINDICES,
  SCB_BLENDWEIGHT,
  SCB_VPOS,
  SCB_UV,
  SCB_MAX,
  SCB_TARGET,
  SCB_VFACE,
  SCB_SV_ISFRONTFACE,

  SCS_VS = 1,
  SCS_PS = 2,
};

class ShaderCreator
{
  struct Reg
  {
    sPoolString Name;
    sInt Type;
    sInt Bind;
    sInt Index;
    sInt Temp;
  };

  struct Tex
  {
    Texture2D *Tex2D;
    TextureCube *TexCube;
    sInt Slot;
    sInt Flags;
  };

  struct CompiledShaderInfo
  {
    sU32 Hash;
    sInt ShaderType;
    sShader *Shader;
    sChar *Source;
  };

  sArray<Reg> ParaReg;
  sArray<Reg> Outputs;
  sArray<Reg> Inputs;
  sArray<Reg> Binds;
  sArray<Reg> Paras;
  sArray<Reg> Uniforms;
  sArray<Reg> Reqs;
  sArray<Tex> Texs;

  ModMtrlParaAssign *Assign;
  ModMtrlParaAssign *AssignInput;

  sString<1024> TempString;
  sChar *TempNames[64];
  sInt NextTemp;

  sArray<CompiledShaderInfo> CompiledShaders;

public:
  ShaderCreator();
  ~ShaderCreator();

  const sChar *types[SCT_MAX];
  const sChar *binds[SCB_MAX];
  const sChar *typeswizzle[SCT_MAX];
  sInt typesize[SCT_MAX];
  const static sChar *swizzle[4][5];
  const sChar *GetTemp();
  sInt ShaderType;                          // SCS_PS or SCS_VS
  sInt LightMask;                           // it is convenient to have this here.
  ModMtrl *Mtrl;                            // it is convenient to have this here.

  void Init(ModMtrlParaAssign *ass,ModMtrlParaAssign *assi,sInt scs_shadertype,UpdateTexInfo *uptex);
  void Shift(ModMtrlParaAssign *ass,ModMtrlParaAssign *assi,sInt scs_shadertype);
  void RegPara(sPoolString name,sInt type,sInt offset);
  void SortBinds();
  sShader *Compile(sTextBuffer &log);
  void SetTextures(sMaterial *mtrl);

  void Output(sPoolString name,sInt type,sInt bind,sInt bindindex=0);
  void BindVS(sPoolString name,sInt type,sInt bind,sInt bindindex=0);
  void BindPS(sPoolString name,sInt type,sInt bind,sInt bindindex=0);
  void InputPS(sPoolString name,sInt type,sBool modify = 0);
  void Para(sPoolString name);
  void Uniform(sPoolString name,sInt type,sInt bindindex);
  sInt Texture(Texture2D *tex,sInt flags);
  sInt Texture(TextureCube *tex,sInt flags);
  void Require(sPoolString name,sInt type); // late requiries
  sBool Requires(sPoolString text,sInt &index);

  void DoubleSided(sPoolString name);

  // fragment linker

  struct Value;
  struct Fragment;
  struct Value
  {
    Value();
    ~Value();
    sPoolString Name;                       // name of the resource
    Fragment *First;                        // first use (input)
    sArray<Fragment *> Modify;              // middle uses that modify
    sArray<Fragment *> Read;                // middle uses readonly, will be put last
  };
  struct Fragment
  {
    Fragment();
    ~Fragment();
    sInt KillMe;
    const sChar *Code;                      // code fragment
    const sChar *Comment;
    sArray<Fragment *> Depend;              // dependencies, automatically created
  };

  sTextBuffer TB;                           // print together your fragment here

  sArray<Fragment *> FragVS;                // fragments for vertex shader
  sArray<Fragment *> FragPS;                // fragments for pixel shader
  sArray<Value *> ValueVS;
  sArray<Value *> ValuePS;
  sArray<Fragment *> *Frags;
  sArray<Value *> *Values;

  Fragment *Frag;
  UpdateTexInfo *UpdateTex;

  void MakeDepend();
  sBool ResolveDepend(sTextBuffer &tb);
  void DumpDepend();
  Value *GetValue(sPoolString name);        // get value, create if not available
  Fragment *MakeFragment();                 // FragBegin() + FragEnd()
  Fragment *FragBegin(const sChar *c=0);    // prepare fragment for generation
  Fragment *FragEnd();                      // but TB into frag, and add to list
  void FragsInit();                         // prepare for fragment use
  void FragsSetShader();
  void FragsExit();                         // clear all values and fragments
  void FragFirst(sPoolString name);         // add for current frag as first dependency
  void FragModify(sPoolString name);        // add for current frag as modify dependency
  void FragRead(sPoolString name);          // add for current frag as read dependency
};


/****************************************************************************/

// helpers

const sChar *Tex2D(sInt n,const sChar *uv,const sChar *swizzle,Texture2D *tex);
const sChar *Tex2DLod0(sInt n,const sChar *uv,const sChar *swizzle,Texture2D *tex);
const sChar *Tex2DProj(sInt n,const sChar *xy,const sChar *z,const sChar *w,const sChar *swizzle,Texture2D *tex);
const sChar *Tex2DProjCmp(sInt n,const sChar *xy,const sChar *z,const sChar *w,const sChar *swizzle,Texture2D *tex);

const sChar *Tex2D(sInt n,const sChar *uv,const sChar *swizzle,Texture2D *tex,ShaderCreator *sc);

/****************************************************************************/

#endif // FILE_WZ4FRLIB_WZ4_MODMTRLSC_HPP


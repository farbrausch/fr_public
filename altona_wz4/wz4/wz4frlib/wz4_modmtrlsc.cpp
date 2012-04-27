/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4frlib/wz4_modmtrl.hpp"
#include "wz4frlib/wz4_modmtrlsc.hpp"
#include "wz4frlib/wz4_modmtrl_ops.hpp"
#include "base/graphics.hpp"
#include "shadercomp/shadercomp.hpp"
#include "shadercomp/shaderdis.hpp"

const sChar *ShaderCreator::swizzle[4][5]=
{
  { L"?",L"x",L"xy",L"xyz",L"xyzw", },
  { L"?",L"y",L"yz",L"yzw",L"yzw?", },
  { L"?",L"z",L"zw",L"zw?",L"zw??", },
  { L"?",L"w",L"w?",L"w??",L"w???", },
};
/****************************************************************************/
/***                                                                      ***/
/***   Code Generator                                                     ***/
/***                                                                      ***/
/****************************************************************************/

ShaderCreator::ShaderCreator()
{
  NextTemp = 0;
  ShaderType = 0;
  Assign = 0;
  AssignInput = 0;
  LightMask = 0;
  Frag = 0;


  types[0]            = L"???";
  types[SCT_FLOAT]    = L"float";
  types[SCT_FLOAT2]   = L"float2";
  types[SCT_FLOAT3]   = L"float3";
  types[SCT_FLOAT4]   = L"float4";
  types[SCT_INT]      = L"int";
  types[SCT_INT2]     = L"int2";
  types[SCT_INT3]     = L"int3";
  types[SCT_INT4]     = L"int4";
#if sRENDERER==sRENDER_DX11
  types[SCT_UINT]     = L"uint";
  types[SCT_UINT2]    = L"uint2";
  types[SCT_UINT3]    = L"uint3";
  types[SCT_UINT4]    = L"uint4";
  types[SCT_BOOL]     = L"bool";
#else
  types[SCT_UINT]     = L"int";
  types[SCT_UINT2]    = L"int2";
  types[SCT_UINT3]    = L"int3";
  types[SCT_UINT4]    = L"int4";
  types[SCT_BOOL]     = L"int";
#endif
  types[SCT_FLOAT3x4] = L"row_major float3x4";
  types[SCT_FLOAT4x3] = L"row_major float4x3";
  types[SCT_FLOAT4x4] = L"row_major float4x4";
  types[SCT_SKINNING] = L"???";
  types[SCT_FLOAT4A8] = L"???";
  types[SCT_FLOAT4A4] = L"???";

  typesize[0]             = 0;
  typesize[SCT_FLOAT]     = 1;
  typesize[SCT_FLOAT2]    = 2;
  typesize[SCT_FLOAT3]    = 3;
  typesize[SCT_FLOAT4]    = 4;
  typesize[SCT_INT]       = 1;
  typesize[SCT_INT2]      = 2;
  typesize[SCT_INT3]      = 3;
  typesize[SCT_INT4]      = 4;
  typesize[SCT_UINT]      = 1;
  typesize[SCT_UINT2]     = 2;
  typesize[SCT_UINT3]     = 3;
  typesize[SCT_UINT4]     = 4;
  typesize[SCT_BOOL]      = 1;
  typesize[SCT_FLOAT3x4]  = 12;
  typesize[SCT_FLOAT4x3]  = 12;
  typesize[SCT_FLOAT4x4]  = 16;
  typesize[SCT_SKINNING]  = 4*74;
  typesize[SCT_FLOAT4A8]  = 4*8;
  typesize[SCT_FLOAT4A4]  = 4*4;

  sClear(typeswizzle);
  typeswizzle[SCT_FLOAT]  = L"x";
  typeswizzle[SCT_FLOAT2] = L"xy";
  typeswizzle[SCT_FLOAT3] = L"xyz";
  typeswizzle[SCT_FLOAT4] = L"xyzw";
  typeswizzle[SCT_INT]    = L"x";
  typeswizzle[SCT_INT2]   = L"xy";
  typeswizzle[SCT_INT3]   = L"xyz";
  typeswizzle[SCT_INT4]   = L"xyzw";
  typeswizzle[SCT_UINT]   = L"x";
  typeswizzle[SCT_UINT2]  = L"xy";
  typeswizzle[SCT_UINT3]  = L"xyz";
  typeswizzle[SCT_UINT4]  = L"xyzw";
  typeswizzle[SCT_BOOL]   = L"x";

  binds[0]                = L"???";
  binds[SCB_POS]          = L"position";
  binds[SCB_POSVERT]      = L"position";
  binds[SCB_NORMAL]       = L"normal";
  binds[SCB_TANGENT]      = L"tangent";
  binds[SCB_UV]           = L"texcoord";
  binds[SCB_COLOR]        = L"color";
  binds[SCB_TARGET]       = L"color";
  binds[SCB_BLENDINDICES] = L"blendindices";
  binds[SCB_BLENDWEIGHT]  = L"blendweight";
  binds[SCB_VPOS]         = L"vpos";
  binds[SCB_VFACE]        = L"vface";
  binds[SCB_SV_ISFRONTFACE]  = L"SV_IsFrontFace";

#if sRENDERER==sRENDER_DX11
  binds[SCB_TARGET]       = L"SV_Target";
  binds[SCB_POS]          = L"SV_position";
  binds[SCB_VPOS]         = L"SV_position";
#endif

  for(sInt i=0;i<sCOUNTOF(TempNames);i++)
  {
    TempNames[i] = new sChar[8];
    TempNames[i][0] = 't';
    TempNames[i][1] = 'e';
    TempNames[i][2] = 'm';
    TempNames[i][3] = 'p';
    TempNames[i][4] = '0'+i/10;
    TempNames[i][5] = '0'+i%10;
    TempNames[i][6] = 0;
  }
}
  
ShaderCreator::~ShaderCreator()
{
  CompiledShaderInfo *cs;
  sFORALL(CompiledShaders,cs)
  {
    cs->Shader->Release();
    delete[] cs->Source;
  }
  for(sInt i=0;i<sCOUNTOF(TempNames);i++)
    delete[] TempNames[i];
  FragsExit();
  sVERIFY(Frag==0);
}

/****************************************************************************/

void ShaderCreator::Init(ModMtrlParaAssign *ass,ModMtrlParaAssign *assi,sInt shadertype,UpdateTexInfo *uptex)
{
  Outputs.Clear();
  Inputs.Clear();
  Binds.Clear();
  Paras.Clear();
  Reqs.Clear();
  Uniforms.Clear();
  Texs.Clear();

  Assign = ass;
  AssignInput = assi;
  UpdateTex = uptex;

  NextTemp = 0;
  ShaderType = shadertype;
  FragsInit();
  FragsSetShader();
}

void ShaderCreator::Shift(ModMtrlParaAssign *ass,ModMtrlParaAssign *assi,sInt shadertype)
{
  if(shadertype==0)               // shift from VS to IA
    Reqs = Binds; 
  else                            // shfit from PS to VS
    Reqs = Inputs;
  Outputs.Clear();
  Inputs.Clear();
  Binds.Clear();
  Paras.Clear();
  Uniforms.Clear();
  Texs.Clear();

  Assign = ass;
  AssignInput = assi;
  ShaderType = shadertype;
  FragsSetShader();
}

const sChar *ShaderCreator::GetTemp()
{
  sVERIFY(NextTemp<sCOUNTOF(TempNames));
  return TempNames[NextTemp++];
}

void ShaderCreator::RegPara(sPoolString name,sInt type,sInt offset)
{
  Reg *r = ParaReg.AddMany(1);
  r->Name = name;
  r->Type = type;
  r->Bind = 0;
  r->Index = offset/4;
}

void ShaderCreator::SortBinds()
{
  Reg *r;
  sFORALL(Binds,r)
    r->Temp = (r->Bind<<16)+r->Index;
  sSortUp(Binds,&Reg::Temp);
}

sShader *ShaderCreator::Compile(sTextBuffer &log)
{
  sTextBuffer tb;
  Reg *r;
  Tex *t;
  const sChar *profile = L"???";
  sInt shadertype = 0;

#if sRENDERER==sRENDER_DX9
  const sChar *psp = L"ps_3_0";
  const sChar *vsp = L"vs_3_0";
  shadertype = sSTF_HLSL23;
#endif
#if sRENDERER==sRENDER_DX11
  const sChar *psp = L"ps_4_0";
  const sChar *vsp = L"vs_4_0";
  shadertype = sSTF_HLSL45;
#endif

  switch(ShaderType)
  {
  case SCS_VS:
    profile = vsp;
    shadertype |= sSTF_VERTEX;
    break;
  case SCS_PS:
    profile = psp;
    shadertype |= sSTF_PIXEL;
    break;
  }


#if sRENDERER==sRENDER_DX9
  sFORALL(Texs,t)
  {
    if(t->Tex2D)
      tb.PrintF(L"sampler2D tex%d : register(s%d);\n",_i,_i);
    if(t->TexCube)
      tb.PrintF(L"samplerCUBE tex%d : register(s%d);\n",_i,_i);
  }
#endif

#if sRENDERER==sRENDER_DX11

  sFORALL(Texs,t)
  {
    if(t->Tex2D)
    {
      if(t->Flags & sMTF_PCF)
        tb.PrintF(L"SamplerComparisonState sam%dcmp : register(s%d);\n",_i,_i);
      else
        tb.PrintF(L"SamplerState sam%d : register(s%d);\n",_i,_i);
      tb.PrintF(L"Texture2D tex%d : register(t%d);\n",_i,_i);
    }
    if(t->TexCube)
    {
      tb.PrintF(L"SamplerState sam%d : register(s%d);\n",_i,_i);
      tb.PrintF(L"TextureCube tex%d : register(t%d);\n",_i,_i);
    }
  }

  if(1)
  {
    sSortUp(Uniforms,&Reg::Index);
    sInt cbuffer = -1;

//    tb.PrintF(L"cbuffer cb0 : register(b0)\n{\n  uniform float4 para[%d];\n}\n",sMax(1,Assign->GetSize()));
    sFORALL(Uniforms,r)
    {
      sInt nextcbuffer = 0;
      sInt startreg = 0;

      sInt index = r->Index/4;
      if(ShaderType==SCS_VS)
      {
        if(index>=32)
        {
          nextcbuffer = 2;
          startreg = 32;
        }
        else if(index>=28)
        {
          nextcbuffer = 1;
          startreg = 28;
        }
      }

      if(cbuffer!=nextcbuffer)
      {
        if(cbuffer>=0)
          tb.PrintF(L"}\n");
        cbuffer = nextcbuffer;
        tb.PrintF(L"cbuffer cb%d : register(b%d)\n",cbuffer,cbuffer);
        tb.PrintF(L"{\n");
      }
      if(r->Type==SCT_SKINNING)
        tb.PrintF(L"  uniform float4 %s[3] : packoffset(c%d);\n",r->Name,index-startreg);
      else if(r->Type==SCT_FLOAT4A8)
        tb.PrintF(L"  uniform float4 %s[8] : packoffset(c%d);\n",r->Name,index-startreg);
      else if(r->Type==SCT_FLOAT4A4)
        tb.PrintF(L"  uniform float4 %s[4] : packoffset(c%d);\n",r->Name,index-startreg);
      else
        tb.PrintF(L"  uniform %s %s : packoffset(c%d);\n",types[r->Type],r->Name,index-startreg);
    }
    if(cbuffer>=0)
      tb.PrintF(L"}\n");

  }
#endif


  tb.Print(L"void main(\n");
  if(AssignInput)
  {
    for(sInt i=0;i<AssignInput->GetSize();i++)
      tb.PrintF(L"  in float4 t%d : texcoord%d,\n",i,i);
  }
  sFORALL(Binds,r)
    tb.PrintF(L"  in %s %s : %s%d,\n",types[r->Type],r->Name,binds[r->Bind],r->Index);
  sFORALL(Outputs,r)
    tb.PrintF(L"  out %s %s : %s%d,\n",types[r->Type],r->Name,binds[r->Bind],r->Index);
#if sRENDERER==sRENDER_DX9
  sFORALL(Uniforms,r)
  {
    if(r->Type==SCT_SKINNING)
      tb.PrintF(L"  uniform float4 %s[74] : register(c%d),\n",r->Name,r->Index/4);
    else if(r->Type==SCT_FLOAT4A8)
      tb.PrintF(L"  uniform float4 %s[8] : register(c%d),\n",r->Name,r->Index/4);
    else if(r->Type==SCT_FLOAT4A4)
      tb.PrintF(L"  uniform float4 %s[4] : register(c%d),\n",r->Name,r->Index/4);
    else
      tb.PrintF(L"  uniform %s %s : register(c%d),\n",types[r->Type],r->Name,r->Index/4);
  }
#endif
  tb.PrintF(L"  uniform float4 para[%d] : register(c0)\n",sMax(1,Assign->GetSize()));
  tb.PrintF(L")\n{\n");

  // pre-segment

  Fragment *frag;

  if(AssignInput)
  {
    sFORALL(Inputs,r)
    {
      frag = FragBegin(L"inputs");
      switch(r->Type)
      {
      case SCT_FLOAT4x3:
        TB.PrintF(L"  %s %s = float4x3(t%d,t%d,t%d);\n",
          types[r->Type],r->Name,r->Index/4,r->Index/4+1,r->Index/4+2);
        break;
      case SCT_FLOAT3x4:
        TB.PrintF(L"  %s %s = float3x4(t%d,t%d,t%d);\n",
          types[r->Type],r->Name,r->Index/4,r->Index/4+1,r->Index/4+2);
        break;
      case SCT_FLOAT4x4:
        TB.PrintF(L"  %s %s = float4x4(t%d,t%d,t%d,t%d);\n",
          types[r->Type],r->Name,r->Index/4,r->Index/4+1,r->Index/4+2,r->Index/4+3);
        break;
      case SCT_FLOAT:
      case SCT_FLOAT2:
      case SCT_FLOAT3:
      case SCT_FLOAT4:
      case SCT_INT:
      case SCT_INT2:
      case SCT_INT3:
      case SCT_INT4:
      case SCT_UINT:
      case SCT_UINT2:
      case SCT_UINT3:
      case SCT_UINT4:
      case SCT_BOOL:
        TB.PrintF(L"  %s %s = t%d.%s;\n",types[r->Type],r->Name,r->Index/4,swizzle[r->Index&3][typesize[r->Type]]);
        break;
      }
      FragFirst(r->Name);
      FragEnd();
    }
  }


  sFORALL(Paras,r)
  {
    frag = FragBegin(L"constants");
    switch(r->Type)
    {
    case SCT_FLOAT4x3:
      TB.PrintF(L"  %s %s = float4x3(para[%d],para[%d],para[%d]);\n",
        types[r->Type],r->Name,r->Index/4,r->Index/4+1,r->Index/4+2);
      break;
    case SCT_FLOAT3x4:
      TB.PrintF(L"  %s %s = float3x4(para[%d],para[%d],para[%d]);\n",
        types[r->Type],r->Name,r->Index/4,r->Index/4+1,r->Index/4+2);
      break;
    case SCT_FLOAT4x4:
      TB.PrintF(L"  %s %s = float4x4(para[%d],para[%d],para[%d],para[%d]);\n",
        types[r->Type],r->Name,r->Index/4,r->Index/4+1,r->Index/4+2,r->Index/4+3);
      break;
    case SCT_FLOAT:
    case SCT_FLOAT2:
    case SCT_FLOAT3:
    case SCT_FLOAT4:
    case SCT_INT:
    case SCT_INT2:
    case SCT_INT3:
    case SCT_INT4:
    case SCT_UINT:
    case SCT_UINT2:
    case SCT_UINT3:
    case SCT_UINT4:
    case SCT_BOOL:
      TB.PrintF(L"  %s %s = para[%d].%s;\n",types[r->Type],r->Name,r->Index/4,swizzle[r->Index&3][typesize[r->Type]]);
      break;
    case SCT_FLOAT4A8:
      TB.PrintF(L"  float4 %s[8] = { para[%d],para[%d],para[%d],para[%d] ,para[%d],para[%d],para[%d],para[%d] };\n"
        ,r->Name, r->Index/4+0,r->Index/4+1,r->Index/4+2,r->Index/4+3, r->Index/4+4,r->Index/4+5,r->Index/4+6,r->Index/4+7);
      break;
    case SCT_FLOAT4A4:
      TB.PrintF(L"  float4 %s[4] = { para[%d],para[%d],para[%d],para[%d] };\n"
        ,r->Name, r->Index/4+0,r->Index/4+1,r->Index/4+2,r->Index/4+3);
      break;
    }   
    FragFirst(r->Name);
    FragEnd();
  }

  MakeDepend();
//  DumpDepend();
  ResolveDepend(tb);

  tb.PrintF(L"}\n");

  // compile

  sInt len = sGetStringLen(tb.Get());
  sU32 hash = sChecksumMurMur((sU32 *)tb.Get(),len/2);      // this is safe because of the trailing 0.
  CompiledShaderInfo *cs;
  sFORALL(CompiledShaders,cs)
  {
    if(cs->Hash == hash && cs->ShaderType==ShaderType && sCmpString(cs->Source,tb.Get())==0)
    {
      cs->Shader->AddRef();
      return cs->Shader;
    }
  }

  sTextBuffer error;
  sU8 *data = 0;
  sInt size = 0;
  sShader *sh = 0;
  if(sShaderCompileDX(tb.Get(),profile,L"main",data,size,sSCF_AVOID_CFLOW,&error))
  {
    log.PrintF(L"dx: %q\n",error.Get());
    log.PrintListing(tb.Get());
    log.Print(L"/****************************************************************************/\n");
#if sRENDERER==sRENDER_DX9
    sPrintShader(log,(const sU32 *) data,sPSF_NOCOMMENTS);
#endif
    sh = sCreateShaderRaw(shadertype,data,size);
  }
  else
  {
    log.PrintF(L"dx: %q\n",error.Get());
    log.PrintListing(tb.Get());
    log.Print(L"/****************************************************************************/\n");

    sDPrint(log.Get());
  }
  delete[] data;

  cs = CompiledShaders.AddMany(1);
  cs->Hash = hash;
  cs->ShaderType = ShaderType;
  cs->Shader = sh;
  cs->Source = new sChar[len];
  sCopyMem(cs->Source,tb.Get(),(len+1)*sizeof(sChar));
  sh->AddRef();

  return sh;
}

void ShaderCreator::SetTextures(sMaterial *mtrl)
{
  Tex *t;
  sInt slot=0;

  sInt mtb = 0;
  switch(ShaderType)
  {
  case SCS_VS: mtb = sMTB_VS; break;
  case SCS_PS: mtb = sMTB_PS; break;
  }

  sFORALL(Texs,t)
  {
    sTextureBase *tex = 0;
    if(t->Tex2D)
      tex = t->Tex2D->Texture;
    if(t->TexCube)
      tex = t->TexCube->Texture;

    if(tex)
    {
      while(mtrl->Texture[slot]!=0 && slot<sMTRL_MAXTEX) slot++;
      sVERIFY(mtrl->Texture[slot]==0);
      mtrl->Texture[slot] = tex;
      mtrl->TFlags[slot] = t->Flags;
      mtrl->TBind[slot] = mtb | t->Slot;
      slot++;
    }
  }
}

/****************************************************************************/

void ShaderCreator::Output(sPoolString name,sInt type,sInt bind,sInt bindindex)
{
  Reg *r = Outputs.AddMany(1);
  r->Name = name;
  r->Type = type;
  r->Bind = bind;
  r->Index = bindindex;
}

void ShaderCreator::BindVS(sPoolString name,sInt type,sInt bind,sInt bindindex)
{
  Reg *r;

  if(Frag) FragRead(name);

  sFORALL(Binds,r)
  {
    if(r->Name==name)
    {
      sVERIFY(type == r->Type);
      sVERIFY(bind == r->Bind);
      sVERIFY(bindindex == r->Index);
      return;
    }
  }

  r = Binds.AddMany(1);
  r->Name = name;
  r->Type = type;
  r->Bind = bind;
  r->Index = bindindex;
}

void ShaderCreator::BindPS(sPoolString name,sInt type,sInt bind,sInt bindindex)
{
  Reg *r;

  if(bind==SCB_VPOS)      // hack for vpos
  {
    if(sRENDERER==sRENDER_DX11)
      type = SCT_FLOAT4;
  }

  if(Frag) FragRead(name);

  sFORALL(Binds,r)
  {
    if(r->Name==name)
    {
      sVERIFY(type == r->Type);
      sVERIFY(bind == r->Bind);
      sVERIFY(bindindex == r->Index);
      return;
    }
  }

  r = Binds.AddMany(1);
  r->Name = name;
  r->Type = type;
  r->Bind = bind;
  r->Index = bindindex;
}

void ShaderCreator::InputPS(sPoolString name,sInt type,sBool modify)
{
  Reg *r;

  if(Frag) 
  {
    if(modify)
      FragModify(name);
    else
      FragRead(name);
  }

  sFORALL(Inputs,r)
  {
    if(r->Name==name)
    {
      sVERIFY(type == r->Type);
      return;
    }
  }

  r = Inputs.AddMany(1);
  r->Name = name;
  r->Type = type;
  r->Bind = SCB_UV;
  r->Index = AssignInput->Assign(typesize[type],0);
}

void ShaderCreator::Para(sPoolString name)
{
  Reg *r;

  if(Frag) FragRead(name);

  sFORALL(Paras,r)
    if(r->Name==name)
      return;

  Reg *rr = sFind(ParaReg,&Reg::Name,name);
  sVERIFY(rr);
  r = Paras.AddMany(1);
  r->Name = name;
  r->Type = rr->Type;
  r->Bind = 0;
  r->Index = Assign->Assign(typesize[r->Type],rr->Index);
}

void ShaderCreator::Uniform(sPoolString name,sInt type,sInt bindindex)
{
  if(Frag) FragRead(name);

  Reg *r = Uniforms.AddMany(1);
  r->Name = name;
  r->Type = type;
  r->Bind = 0;
  r->Index = bindindex;
}

sInt ShaderCreator::Texture(Texture2D *tex,sInt flags)
{
  Tex *t;
  if(tex!=ModMtrlType->DummyTex2D)
  {
    sFORALL(Texs,t)
    {
      if(t->Tex2D==tex && t->TexCube==0 && t->Flags==flags)
        return t->Slot;
    }
  }
  sInt slot = Texs.GetCount();
  t = Texs.AddMany(1);
  t->Tex2D = tex;
  t->TexCube = 0;
  t->Flags = flags;
  t->Slot = slot;

  return slot;
};

sInt ShaderCreator::Texture(TextureCube *tex,sInt flags)
{
  Tex *t;
  if(tex!=ModMtrlType->DummyTexCube)
  {
    sFORALL(Texs,t)
    {
      if(t->Tex2D==0 && t->TexCube==tex && t->Flags==flags)
        return t->Slot;
    }
  }
  sInt slot = Texs.GetCount();
  t = Texs.AddMany(1);
  t->Tex2D = 0;
  t->TexCube = tex;
  t->Flags = flags;
  t->Slot = slot;

  return slot;
};

void ShaderCreator::Require(sPoolString name,sInt type)
{
  if(Frag) FragRead(name);

  Reg *r;
  sFORALL(Reqs,r)
  {
    if(r->Name==name)
    {
      sVERIFY(type == r->Type);
      return;
    }
  }

  r = Reqs.AddMany(1);
  r->Name = name;
  r->Type = type;
  r->Bind = 0;
  r->Index = -1;
}

sBool ShaderCreator::Requires(sPoolString name,sInt &index)
{
  Reg *r;
  index = -1;
  sFORALL(Reqs,r)
  {
    if(r->Name==name)
    {
      index = r->Index;
      return 1;
    }
  }
  return 0;
}

/****************************************************************************/

void ShaderCreator::DoubleSided(sPoolString name)
{
  if(sRENDERER==sRENDER_DX11)
  {
    if(Mtrl->FeatureFlags & MMFF_DoubleSidedLight)
    {
      BindPS(L"faceflag",SCT_BOOL,SCB_SV_ISFRONTFACE);
      TB.PrintF(L"  if(!faceflag) %s = -%s;\n",name,name);
    }
  }
  else
  {
    if(Mtrl->FeatureFlags & MMFF_DoubleSidedLight)
    {
      BindPS(L"faceflag",SCT_FLOAT,SCB_VFACE);
      TB.PrintF(L"  if(faceflag<0) %s = -%s;\n",name,name);
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Fragment Linker SHit                                               ***/
/***                                                                      ***/
/****************************************************************************/

ShaderCreator::Value::Value()
{
  First = 0;
}

ShaderCreator::Value::~Value()
{
}

ShaderCreator::Fragment::Fragment()
{
  Code = 0;
  KillMe = 0;
}

ShaderCreator::Fragment::~Fragment()
{
  delete[] Code;
}

/****************************************************************************/

void ShaderCreator::MakeDepend()
{
  Fragment *f,*fl;
  Value *v;
  sFORALL(*Values,v)
  {
    fl = v->First;

    sFORALL(v->Modify,f)
    {
      if(fl)
        f->Depend.AddTail(fl);
      fl = f;
    }
    sFORALL(v->Read,f)
    {
      if(fl)
        f->Depend.AddTail(fl);
    }
  }
}

void ShaderCreator::DumpDepend()
{
  Fragment *f,*d;
  Value *v;

  sDPrintF(L"---- values\n");

  sFORALL(*Values,v)
  {
    sDPrintF(L"%s ",v->Name);
    if(v->First)
      sDPrintF(L"FIRST %s ",v->First->Comment);
    if(v->Modify.GetCount())
    {
      sDPrintF(L"MODIFY ");
      sFORALL(v->Modify,f)
       sDPrintF(L"%s ",f->Comment);
    }
    if(v->Read.GetCount())
    {
      sDPrintF(L"READ ");
      sFORALL(v->Read,f)
       sDPrintF(L"%s ",f->Comment);
    }
    sDPrintF(L"\n");
  }

  sDPrintF(L"---- fragments\n");

  sFORALL(*Frags,f)
  {
    sDPrintF(L"%s <-",f->Comment);
    sFORALL(f->Depend,d)
      sDPrintF(L" %s",d->Comment);
    sDPrint(L"\n");
  }
}


sBool ShaderCreator::ResolveDepend(sTextBuffer &tb)
{
  Fragment *f;
  sFORALL(*Frags,f)
    f->KillMe = 0;
  sInt active = 1;

  const sChar *comment=0;

  while(active>0)
  {
    sInt hits = 0;
    active = 0;

    sFORALL(*Frags,f)
    {
      if(!f->KillMe)
      {
        active++;
        sRemTrue(f->Depend,&Fragment::KillMe);    // remove all fragments that were already written
        if(f->Depend.GetCount()==0)               // if no dependencies, write out
        {
          hits = 1;                               // and mark this for removal
          f->KillMe = 1;

          if(f->Comment!=comment)                 // pretty print fragment header
          {
            tb.Print(L"\n");
            comment = f->Comment;
            if(comment)
              tb.PrintF(L"// %s\n\n",comment);
          }
          tb.Print(f->Code);                      // print code
        }
      }
    }

    if(hits==0 && active>0)
    {
      sDPrintF(L"could not resolve dependency");
      return 0;
    }
  }
  return 1;
}

/****************************************************************************/

ShaderCreator::Value *ShaderCreator::GetValue(sPoolString name)
{
  Value *v;

  sFORALL(*Values,v)
  {
    if(v->Name==name)
      return v;
  }

  v = new Value;
  v->Name = name;
  Values->AddTail(v);
  return v;
}

ShaderCreator::Fragment *ShaderCreator::MakeFragment()
{
  sVERIFY(Frag==0);
  Frag = new Fragment;
  return FragEnd();
}

ShaderCreator::Fragment *ShaderCreator::FragBegin(const sChar *comment)
{
  sVERIFY(Frag==0);
  Frag = new Fragment;
  Frag->Comment = comment;
  Frag->Code = 0;
  TB.Clear();
  return Frag;
}

ShaderCreator::Fragment *ShaderCreator::FragEnd()
{
  sInt size = TB.GetCount()+1;
  sChar *code = new sChar[size];
  sCopyMem(code,TB.Get(),size*sizeof(sChar));
  Frag->Code = code;
  Frags->AddTail(Frag);
  TB.Clear();

  Fragment *f = Frag;
  Frag = 0;
  return f;
}

void ShaderCreator::FragsInit()
{
  FragsExit();
  TB.Clear();
}

void ShaderCreator::FragsSetShader()
{
  if(ShaderType==SCS_PS)
  {
    Frags = &FragPS;
    Values = &ValuePS;
  }
  else
  {
    Frags = &FragVS;
    Values = &ValueVS;
  }
}

void ShaderCreator::FragsExit()
{
  sDeleteAll(FragVS);
  sDeleteAll(FragPS);
  sDeleteAll(ValueVS);
  sDeleteAll(ValuePS);
  Frags = 0;
  Values = 0;
}

void ShaderCreator::FragFirst(sPoolString name)
{
  Value *v = GetValue(name);
  sVERIFY(v->First == 0);
  v->First = Frag;
}

void ShaderCreator::FragModify(sPoolString name)
{
  Value *v = GetValue(name);
  v->Modify.AddTail(Frag);
}

void ShaderCreator::FragRead(sPoolString name)
{
  Value *v = GetValue(name);
  v->Read.AddTail(Frag);
}

/****************************************************************************/

const sChar *Tex2D(sInt n,const sChar *uv,const sChar *swizzle,Texture2D *tex)
{
  static sChar buffer[128];
#if sRENDERER==sRENDER_DX11
  if(tex && tex->Texture && (tex->Texture->Flags & sTEX_FORMAT)==sTEX_I8)
    sSPrintF(sStringDesc(buffer,sCOUNTOF(buffer)),L"float4(tex%d.Sample(sam%d,(%s).xy).xxx,1).%s",n,n,uv,swizzle);
  else if(tex && tex->Texture && (tex->Texture->Flags & sTEX_FORMAT)==sTEX_A8)
    sSPrintF(sStringDesc(buffer,sCOUNTOF(buffer)),L"float4(0,0,0,tex%d.Sample(sam%d,(%s).xy)).%s",n,n,uv,swizzle);
  else if(tex && tex->Texture && (tex->Texture->Flags & sTEX_FORMAT)==sTEX_IA8)
    sSPrintF(sStringDesc(buffer,sCOUNTOF(buffer)),L"(tex%d.Sample(sam%d,(%s).xy).xxxy).%s",n,n,uv,swizzle);
  else
    sSPrintF(sStringDesc(buffer,sCOUNTOF(buffer)),L"tex%d.Sample(sam%d,(%s).xy).%s",n,n,uv,swizzle);
#else
  sSPrintF(sStringDesc(buffer,sCOUNTOF(buffer)),L"tex2D(tex%d,(%s).xy).%s",n,uv,swizzle);  
#endif
  return buffer;
}

const sChar *Tex2DLod0(sInt n,const sChar *uv,const sChar *swizzle,Texture2D *tex)
{
  static sChar buffer[128];
#if sRENDERER==sRENDER_DX11
  if(tex && tex->Texture && (tex->Texture->Flags & sTEX_FORMAT)==sTEX_I8)
    sSPrintF(sStringDesc(buffer,sCOUNTOF(buffer)),L"float4(tex%d.SampleLevel(sam%d,(%s).xy,0).xxx,1).%s",n,n,uv,swizzle);
  else if(tex && tex->Texture && (tex->Texture->Flags & sTEX_FORMAT)==sTEX_A8)
    sSPrintF(sStringDesc(buffer,sCOUNTOF(buffer)),L"float4(0,0,0,tex%d.SampleLevel(sam%d,(%s).xy,0)).%s",n,n,uv,swizzle);
  else if(tex && tex->Texture && (tex->Texture->Flags & sTEX_FORMAT)==sTEX_IA8)
    sSPrintF(sStringDesc(buffer,sCOUNTOF(buffer)),L"(tex%d.SampleLevel(sam%d,(%s).xy,0).xxxy).%s",n,n,uv,swizzle);
  else
    sSPrintF(sStringDesc(buffer,sCOUNTOF(buffer)),L"tex%d.SampleLevel(sam%d,(%s).xy,0).%s",n,n,uv,swizzle);
#else
  sSPrintF(sStringDesc(buffer,sCOUNTOF(buffer)),L"tex2Dlod(tex%d,float4((%s).xy,0,0)).%s",n,uv,swizzle);  
#endif
  return buffer;
}

const sChar *Tex2DProj(sInt n,const sChar *xy,const sChar *z,const sChar *w,const sChar *swizzle,Texture2D *tex)
{
  static sChar buffer[128];
#if sRENDERER==sRENDER_DX11
  sSPrintF(sStringDesc(buffer,sCOUNTOF(buffer)),L"tex%d.Sample(sam%d,(%s)/(%s)).%s",n,n,xy,w,swizzle);
#else
  sSPrintF(sStringDesc(buffer,sCOUNTOF(buffer)),L"tex2Dproj(tex%d,float4(%s,%s,%s)).%s",n,xy,z,w,swizzle);  
#endif
  return buffer;
}

const sChar *Tex2DProjCmp(sInt n,const sChar *xy,const sChar *z,const sChar *w,const sChar *swizzle,Texture2D *tex)
{
  static sChar buffer[128];
#if sRENDERER==sRENDER_DX11
//  sSPrintF(sStringDesc(buffer,sCOUNTOF(buffer)),L"tex%d.Sample(sam%d,(%s)/(%s)).%s<(%s/%s)",n,n,xy,w,swizzle,z,w);
  sSPrintF(sStringDesc(buffer,sCOUNTOF(buffer)),L"tex%d.SampleCmpLevelZero(sam%dcmp,(%s)/(%s),%s/%s).%s",n,n,xy,w,z,w,swizzle);
#else
  sSPrintF(sStringDesc(buffer,sCOUNTOF(buffer)),L"tex2Dproj(tex%d,float4(%s,%s,%s)).%s",n,xy,z,w,swizzle);  
#endif
  return buffer;
}


const sChar *Tex2D(sInt n,const sChar *uv,const sChar *swizzle,Texture2D *tex,ShaderCreator *sc)
{
  if(sc->ShaderType==SCS_VS)
    return Tex2DLod0(n,uv,swizzle,tex);
  else
    return Tex2D(n,uv,swizzle,tex);
}

/****************************************************************************/

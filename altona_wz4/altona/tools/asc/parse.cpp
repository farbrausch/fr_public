/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"
#include "base/system.hpp"
#include "base/graphics.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Compiler::_Code(sPoolString &code)
{
  sTextBuffer tb;
  tb.PrintF(L"#line %d \"%p\"\n",Scan.TokenLine,Scan.TokenFile);

  if (!Scan.ScanRaw(tb, '{', '}'))
    return;

  code.Init(tb.Get());
}


void Compiler::_CodeNoLine(sPoolString &code,sInt lineoffset)
{
  sTextBuffer tb;

  if (!Scan.ScanRaw(tb, '{', '}'))
    return;

  code.Init(tb.Get());
}

void Compiler::_NewShaderCodeStatement(NewShader *ns)
{
  if(Scan.IfName(L"hlsl"))
  {
    if(!ns->HlslSource.IsEmpty())
      Scan.Error(L"multiple DX shaders specified");
    Scan.ScanName(ns->Profile);
    _Code(ns->HlslSource);
  }
  else if(Scan.IfName(L"asc"))
  {
    if(!ns->AscSource.IsEmpty())
      Scan.Error(L"multiple ASC shaders specified");
    Scan.ScanName(ns->Profile);
    _Code(ns->AscSource);
  }
  else if(Scan.IfName(L"gl"))
  {
    if(!ns->GlslSource.IsEmpty())
      Scan.Error(L"multiple GLSL shaders specified");
    _Code(ns->GlslSource);
  }
  else if(Scan.IfName(L"cg"))
  {
    if(!ns->CgSource.IsEmpty())
      Scan.Error(L"multiple CG shaders specified");
    _Code(ns->CgSource);
  }
  else if(Scan.IfName(L"cflags"))
  {
    while (!Scan.Errors && !Scan.IfToken(';'))
    {
      if (Scan.IfName(L"debug"))
        ns->CompileFlags|=sSCF_DEBUG;
      else if (Scan.IfName(L"dont_optimize"))
        ns->CompileFlags|=sSCF_DONT_OPTIMIZE;
      else if (Scan.IfName(L"avoid_cflow"))
        ns->CompileFlags|=sSCF_AVOID_CFLOW;
      else if (Scan.IfName(L"prefer_cflow"))
        ns->CompileFlags|=sSCF_PREFER_CFLOW;
      else
        Scan.Error(L"unknown compiler flag");
      Scan.IfToken(',');
    }
  }
  else 
  {
    Scan.Error(L"syntax Error");
  }
}

void Compiler::_NewShaderCode(NewMaterial *nm,NewShader *ns)
{
  if(Scan.IfToken('{'))
  {
    while(!Scan.Errors && Scan.Token!='}')
      _NewShaderCodeStatement(ns);
    Scan.Match('}');
  }
  else
    _NewShaderCodeStatement(ns);

  ns->AscSource.Add(nm->AscHeader,ns->AscSource);
}

void Compiler::_Material()
{
  sPoolString cmd;

  Scan.ScanName(cmd);
  NewMaterial *nm=0, *im;
  sFORALL(NewMtrls,im)
  {
    if (im->Name==cmd)
    {
      nm=im;
      break;
    }
  }
  if (!nm)
  {
    nm = new NewMaterial;
    NewMtrls.AddTail(nm);
    nm->Name=cmd;
    nm->BaseName = L"sMaterial";
  }

  if (Scan.Token == ':')
  {
    Scan.Scan();
    Scan.ScanName(nm->BaseName);
  }

  Scan.Match('{');
  while(Scan.Token!='}' && !Scan.Errors)
  {
    if(Scan.IfName(L"new"))
    {
      _Code(nm->New);
    }
    else if(Scan.IfName(L"header"))
    {
      _Code(nm->Header);
    }
    else if(Scan.IfName(L"code"))
    {
      _Code(nm->Code);
    }
    else if(Scan.IfName(L"prepare"))
    {
      _Code(nm->Prepare);
    }
    else if(Scan.IfName(L"asc"))
    {
      _Code(nm->AscHeader);
    }

    else if(Scan.IfName(L"vs"))
    {
      _Shader(nm,_VS_,sSTF_VERTEX);
    }
    else if(Scan.IfName(L"hs"))
    {
      if(!Level11) Scan.Error(L"hs only in level11");
      _Shader(nm,_HS_,sSTF_HULL);
    }
    else if(Scan.IfName(L"ds"))
    {
      if(!Level11) Scan.Error(L"ds only in level11");
      _Shader(nm,_DS_,sSTF_DOMAIN);
    }
    else if(Scan.IfName(L"gs"))
    {
      if(!Level11) Scan.Error(L"gs only in level11");
      _Shader(nm,_GS_,sSTF_GEOMETRY);
    }
    else if(Scan.IfName(L"ps"))
    {
      _Shader(nm,_PS_,sSTF_PIXEL);
    }
    else if(Scan.IfName(L"cs"))
    {
      if(!Level11) Scan.Error(L"cs only in level11");
      _Shader(nm,_CS_,sSTF_COMPUTE);
    }
    else
    {
      Scan.Error(L"syntax error");
    }
  }

  Scan.Match('}');
  Scan.Match(';');

  if(nm->Shaders[_CS_]==0)
  {
    if(nm->Shaders[_PS_]==0)
      Scan.Error(L"no pixel shader");
    if(nm->Shaders[_VS_]==0)
      Scan.Error(L"no vertex shader");
  }
}

void Compiler::_Shader(NewMaterial *nm,sInt ascshaderkind,sInt altonashaderkind)
{
  if (!nm->Shaders[ascshaderkind])
  {
    nm->Shaders[ascshaderkind]=new NewShader;
    nm->Shaders[ascshaderkind]->ShaderType=altonashaderkind;
  }
  _NewShaderCode(nm,nm->Shaders[ascshaderkind]);
}

void Compiler::_Asc()           // top level asc block, for defining permutations and cbuffers.
{
  sPoolString code;
  _Code(code);
  if(!Scan.Errors)
    if(!ASC->Parse(code))
      Scan.Error(L"error parsing ASC");
}

void Compiler::_Include()
{
  sPoolString filename;

  Scan.ScanString(filename);
  Scan.Match(';');

  Includes.AddTail(filename);
}

void Compiler::_Global()
{
  sPoolString cmd;

  while(Scan.Token!=sTOK_END && !Scan.Errors)
  {
    if(Scan.IfName(L"include"))
    {
      _Include();
    }
    else if(Scan.IfName(L"level11"))
    {
      Scan.Match(';');
      Level11 = 1;
    }
    else if(Scan.IfName(L"pconly"))
    {
      Scan.Match(';');
      PcOnly = 1;
    }
    else if(Scan.IfName(L"material"))
    {
      _Material();
    }
    else if(Scan.IfName(L"asc"))
    {
      _Asc();
    }
    else if(Scan.IfName(L"include_asc"))
    {
      sPoolString filename;
      sBool mayfail = Scan.IfName(L"mayfail");
      Scan.ScanString(filename);
      Scan.Match(';');
      if(!Scan.Errors)
      {
        if(!Scan.IncludeFile(filename) && !mayfail)
          Scan.Error(L"could not include file %q",filename);
      }
    }
    else if(Scan.IfName(L"cs"))
    {
      NewMaterial *nm = new NewMaterial;
      Scan.ScanName(nm->Name);

      if(!Level11) Scan.Error(L"cs only in level11");
      _Shader(nm,_CS_,sSTF_COMPUTE);

      ComputeShaders.AddTail(nm);
    }
    else
    {
      Scan.Error(L"syntax error");
    }
  }
}

/****************************************************************************/


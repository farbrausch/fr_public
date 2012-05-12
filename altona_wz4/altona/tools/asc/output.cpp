/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"
#include "base/system.hpp"
#include "base/graphics.hpp"
#include "shadercomp/shaderdis.hpp"
#include "shadercomp/shadercomp.hpp"
#include "shadercomp/asc_doc.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

static const sChar *shadername[] = { L"VS",L"HS",L"DS",L"GS",L"PS",L"CS" };
static const sChar *fullshadername[] = { L"VertexShader",L"HullShader",L"DomainShader",L"GeometryShader",L"PixelShader",L"ComputeShader" };

/****************************************************************************/

void PrintTextShader(class sTextBuffer& tb, const sChar *shader,sBool skipcomment=1)
{
  tb.Print(L"  \"");
  sInt whitespace = 0;
  sInt newline = 0;
  sInt empty = 1;
  sInt c;
  while((c = *shader++) != 0)
  {
    switch(c)
    {
    case '\n':
    case '\r':
      whitespace = 1;
      newline = 1;
      if(*shader=='#' && skipcomment)   // assembler comments
      {
        while(*shader!=0 && *shader!='\n')
          shader++;
      }
      break;
    case ' ':
    case '\t':
      whitespace = 1;
      break;
    case ';':
      tb.PrintChar(';');
      empty = 0;
      newline = 1;
      whitespace = 0;
      break;
    case '"':
      tb.PrintChar('\\');
      tb.PrintChar('"');
      break;
      /*
    case '/':                           // cpp comments
      if(skipcomment && *shader=='/')
      {
        while(*shader!=0 && *shader!='\n')
          shader++;
        break;
      }
      */
    default:
      if(whitespace && !newline)
        tb.PrintChar(' ');
      if(newline && !empty)
        tb.Print(L"\\n\"\n  \"");
      tb.PrintChar(c);
      whitespace = 0;
      newline = 0;
      empty = 0;
    }
  }
  tb.Print(L"\\n\";\n");
}

void PrintTextShader(class sTextBuffer& tb, const sChar8 *shader,sBool skipcomment=1)
{
  tb.Print(L"  \"");
  sInt whitespace = 0;
  sInt newline = 0;
  sInt empty = 1;
  sInt c;
  while((c = *shader++) != 0)
  {
    switch(c)
    {
    case '\n':
    case '\r':
      whitespace = 1;
      newline = 1;
      if(*shader=='#' && skipcomment)   // assembler comments
      {
        while(*shader!=0 && *shader!='\n')
          shader++;
      }
      break;
    case ' ':
    case '\t':
      whitespace = 1;
      break;
    case ';':
      tb.PrintChar(';');
      empty = 0;
      newline = 1;
      whitespace = 0;
      break;
      /*
    case '/':                           // cpp comments
      if(skipcomment && *shader=='/')
      {
        while(*shader!=0 && *shader!='\n')
          shader++;
        break;
      }
      */
    default:
      if(whitespace && !newline)
        tb.PrintChar(' ');
      if(newline && !empty)
        tb.Print(L"\\n\"\n  \"");
      if(c == '"')
        tb.PrintChar('\\');
      tb.PrintChar(c);
      if(c == '\\')
        tb.PrintChar('\\');
      whitespace = 0;
      newline = 0;
      empty = 0;
    }
  }
  tb.Print(L"\\n\";\n");
}

/****************************************************************************/

void PrintShaderU8(class sTextBuffer& tb, const sU8 *shader,sInt size)
{
  for (sInt i=0; i<size; i++)
  {
    if (i && (i&0x0f)==0x00) tb.Print(L"\n");
    if ((i&0x0f)==0x00) tb.Print(L"    ");
    tb.PrintF(L"0x%02x,",shader[i]);
  }
  tb.Print(L"\n");
}

void PrintShaderU32(class sTextBuffer& tb, const sU32 *shader,sInt size)
{
  for (sInt i=0; i<(size+3)/4; i++)
  {
    if (i && (i&0x07)==0x00) tb.Print(L"\n");
    if ((i&0x07)==0x00) tb.Print(L"    ");
    tb.PrintF(L"0x%08x,",shader[i]);
  }
  tb.Print(L"\n");
}

/****************************************************************************/

void PrintCreateShader(sTextBuffer &CPP,sInt type,const sChar *name,const sChar *result)
{
  const sChar *types=0;
  switch(type&sSTF_KIND)
  {
  case sSTF_VERTEX:
    types = L"sSTF_VERTEX";
    break;
  case sSTF_PIXEL:
    types = L"sSTF_PIXEL";
    break;
  case sSTF_GEOMETRY:
    types = L"sSTF_GEOMETRY";
    break;
  case sSTF_HULL:
    types = L"sSTF_HULL";
    break;
  case sSTF_DOMAIN:
    types = L"sSTF_DOMAIN";
    break;
  case sSTF_COMPUTE:
    types = L"sSTF_COMPUTE";
    break;
  default:
    sVERIFYFALSE;
  }

  CPP.Print(L"#if sRENDERER==sRENDER_DX9 || sRENDERER==sRENDER_DX11\n");
  CPP.PrintF(L"  %s sCreateShaderRaw(%s,(const sU8 *)%s,sizeof(%s));\n",result,types,name,name);
  CPP.Print(L"#endif\n");
  CPP.Print(L"#if sRENDERER==sRENDER_OGL2\n");
  CPP.PrintF(L"  %s sCreateShaderRaw(%s,(const sU8 *)%s,sizeof(%s));\n",result,types,name,name);
  CPP.Print(L"#endif\n");
  CPP.Print(L"#if sRENDERER==sRENDER_BLANK\n");
  CPP.PrintF(L"  %s sCreateShaderRaw(%s,0,0);\n",result,types);
  CPP.Print(L"#endif\n");
}

/****************************************************************************/

void PrintCode(class sTextBuffer &tb,const sChar *code)
{
  while(*code)
  {
    if(*code!='\r')           // remove all 0x0d from source
      tb.PrintChar(*code);    // to avoid mixed line endings
    code++;
  }
  tb.PrintChar('\n');
}

/****************************************************************************/
/****************************************************************************/


static void sPrintNewShaderData(sTextBuffer &CPP,const sChar *name,NewCode *code,sStringDesc ifdef)
{
  sBool ifor=sFALSE;
  if(code->DX9Code)
  {
    CPP.PrintF(L"#if sRENDERER==sRENDER_DX9\n");
    CPP.PrintF(L"  static const sU32 %s[] = \n",name);
    CPP.PrintF(L"  {\n");
    sPrintShader(CPP,(const sU32 *)code->DX9Code,sPSF_CARRAY|sPSF_NOCOMMENTS);
    CPP.PrintF(L"  };\n");
    CPP.PrintF(L"#endif\n");
    if (ifor) sAppendString(ifdef,L" ||");
    sAppendString(ifdef,L" (sRENDERER==sRENDER_DX9)");
    ifor=sTRUE;
  }
  if(code->DX11Code)
  {
    CPP.PrintF(L"#if sRENDERER==sRENDER_DX11\n");
    CPP.PrintF(L"  static const sU8 %s[] = \n",name);
    CPP.PrintF(L"  {\n");
    PrintShaderU8(CPP,code->DX11Code,code->DX11Size);
    CPP.PrintF(L"  };\n");
    CPP.PrintF(L"#endif\n");
    if (ifor) sAppendString(ifdef,L" ||");
    sAppendString(ifdef,L" (sRENDERER==sRENDER_DX11)");
    ifor=sTRUE;
  }
  if(code->GLSLCode)
  {
    CPP.PrintF(L"#if sRENDERER==sRENDER_OGL2\n");
    CPP.PrintF(L"  static const sU8 %s[] = \n",name);
    PrintTextShader(CPP,(const sChar8 *)code->GLSLCode,1);
    CPP.PrintF(L"#endif\n");
    if (ifor) sAppendString(ifdef,L" ||");
    sAppendString(ifdef,L" sRENDERER==sRENDER_OGL2");
    ifor=sTRUE;
  }
}

void sPrintNewShaderCode(sTextBuffer &CPP,const sChar *name,const sChar *size,NewShader *ns)
{
  sInt shadertype=ns->ShaderType;

  const sChar *st=L"???";
  switch(shadertype&sSTF_KIND)
  {
    case sSTF_VERTEX:   st = L"sSTF_VERTEX"; break;
    case sSTF_PIXEL:    st = L"sSTF_PIXEL"; break;
    case sSTF_GEOMETRY: st = L"sSTF_GEOMETRY"; break;
    case sSTF_HULL:     st = L"sSTF_HULL"; break;
    case sSTF_DOMAIN:   st = L"sSTF_DOMAIN"; break;
    case sSTF_COMPUTE:  st = L"sSTF_COMPUTE"; break;
  }

  CPP.PrintF(L"#if sRENDERER==sRENDER_DX11\n");
  CPP.PrintF(L"  return sCreateShaderRaw(%s|sSTF_HLSL45,(const sU8 *)%s,%s);\n",st,name,size);
  if(!Level11)
  {
    CPP.PrintF(L"#elif sRENDERER==sRENDER_DX9\n");
    CPP.PrintF(L"  return sCreateShaderRaw(%s|sSTF_HLSL23,(const sU8 *)%s,%s);\n",st,name,size);
    if(sCONFIG_SDK_CG && !PcOnly)
    {
      CPP.PrintF(L"#elif sRENDERER==sRENDER_OGL2\n");
      CPP.PrintF(L"  return sCreateShaderRaw(%s|sSTF_NVIDIA,(const sU8 *)%s,%s);\n",st,name,size);
    }
  }
  CPP.PrintF(L"#else\n");
  CPP.PrintF(L"  return sCreateShaderRaw(%s,0,0);\n",st);
  CPP.PrintF(L"#endif\n");
}

/****************************************************************************/
/****************************************************************************/

void Compiler::OutputHPP()
{
  sPoolString *name;
  ACType *type;
  ACVar *member;
  ACExtern *ext;
  ACPermute *perm;
  ACPermuteMember *pmem;

  sString<256> smallpath;
  sString<256> projectpath;
  {
    const sChar *s;
    sChar *d;
    d = smallpath;
    s = Filename;
    while(*s)
    {
      if(sIsLetter(*s) || sIsDigit(*s))
        *d++ = *s;
      s++;
    }
    *d++ = 0;
  }
  {
    sString<sMAXPATH> path;
    sGetCurrentDir(path);
    sInt n = sFindLastChar(path,'/');
    if(n<0)
      n = sFindLastChar(path,'\\');
    if(n>=0)
    {
      const sChar *s = path+n;
      sChar *d = projectpath;
      while(*s)
      {
        if(sIsLetter(*s) || sIsDigit(*s))
          *d++ = *s;
        s++;
      }
      *d++ = 0;
    }
  }

  HPP.Print(L"/****************************************************************************/\n");
  HPP.Print(L"/***                                                                      ***/\n");
  HPP.PrintF(L"/***   header file generated by ASC %1d.%2d - altona shader compiler.        ***/\n",VERSION,REVISION);
  HPP.Print(L"/***                                                                      ***/\n");
  HPP.Print(L"/****************************************************************************/\n");
  HPP.Print(L"\n");
  HPP.PrintF(L"#ifndef HEADER_%C_%C_ASC\n",projectpath,smallpath);
  HPP.PrintF(L"#define HEADER_%C_%C_ASC\n",projectpath,smallpath);
  HPP.Print(L"\n");
  HPP.Print(L"#include \"base/graphics.hpp\"\n");

  sFORALL(Includes,name)
    HPP.PrintF(L"#include \"%s\"\n",*name);

  HPP.Print(L"\n");
  HPP.Print(L"/****************************************************************************/\n");
  HPP.Print(L"\n");

  // permutes


  sFORALL(ASC->Permutes,perm)
  {
    HPP.PrintF(L"enum %s\n",perm->Name);
    HPP.Print(L"{\n");
    sFORALL(perm->Members,pmem)
    {
      //sInt val = pmem->Group ? pmem->Mask : pmem->Value;
      sInt spaces = 40 - sGetStringLen(perm->Name) - sGetStringLen(pmem->Name);
      if(pmem->Group)
        HPP.PrintF(L"  %sMask_%s%_ = 0x%04x,\n",perm->Name,pmem->Name,spaces-4,pmem->Mask<<pmem->Shift);
      else
        HPP.PrintF(L"  %s_%s%_ = 0x%04x,\n",perm->Name,pmem->Name,spaces,pmem->Value<<pmem->Shift);
    }
    HPP.Print(L"};\n");
  }

  HPP.Print(L"\n");
  HPP.Print(L"/****************************************************************************/\n");
  HPP.Print(L"\n");

  // constant buffer

  sFORALL(ASC->UserTypes,type)
  {
    if(type->Type==ACT_CBUFFER)
    {
      HPP.PrintF(L"struct %s\n",type->Name);
      HPP.PrintF(L"{\n");
      HPP.Print(L"public:\n");
      sFORALL(type->Members,member)
      {
        ACType *mbt = member->Type;
        sInt ac = 1;
        sInt r,c;
        mbt->SizeOf(c,r);
        if(member->Type->Type==ACT_ARRAY)
        {
          mbt = member->Type->BaseType;
          if(member->Type->ArrayCount==0)
            Scan.Error(L"could not determine array count");
          ac = member->Type->ArrayCount;
          mbt->SizeOf(c,r);
        }
        else
        {
          if(member->Usages & ACF_ROWMAJOR)
          {
            if(c==4&&r==3)
              sFatal(L"don't use <row_major float3x4>");
            sSwap(r,c);
          }
        }
        const sChar *name = L"sVector4";
        if(c==4&&r==4) name = L"sMatrix44";
        else if(c==3&&r==4) name = L"sMatrix34CM"; // column major stored sMatrix34
        else ac *= c;
        HPP.PrintF(L"  %s %s",name,member->Name);
        if(ac>1)
          HPP.PrintF(L"[%d]",ac);
        HPP.PrintF(L";\n"); 
      }
      static const sChar *shaderconst[] = { L"sCBUFFER_VS",L"sCBUFFER_PS",L"sCBUFFER_GS",L"sCBUFFER_HS",L"sCBUFFER_DS",L"sCBUFFER_CS" };
      HPP.PrintF(L"  static const sInt RegStart = %d;\n",type->CRegStart);
      HPP.PrintF(L"  static const sInt RegCount = %d;\n",type->CRegCount);
      HPP.PrintF(L"  static const sInt Slot = %s|%d;\n",shaderconst[(type->CSlot & sCBUFFER_SHADERMASK)/sCBUFFER_MAXSLOT],type->CSlot%sCBUFFER_MAXSLOT);
      sFORALL(type->Externs,ext)
      {
        HPP.PrintF(L"#line %d \"%p\"\n",ext->ParaLine,ext->File);
        HPP.PrintF(L"  %s %s(%s);\n",ext->Result,ext->Name,ext->Para);
      }
      HPP.PrintF(L"#line %d \"%phpp\"\n",sCountChar(HPP.Get(),'\n')+2,Filename);
      HPP.Print(L"};\n");
      HPP.Print(L"\n");
    }
  }

  HPP.Print(L"\n");
  HPP.Print(L"/****************************************************************************/\n");
  HPP.Print(L"\n");

  // new shaders

  NewShader *ns;
  NewMaterial *nm;

  sFORALL(NewShaders,ns)
  {
    if(ns->PermuteCount)
      HPP.PrintF(L"sShader *%s(sInt perm);\n",ns->Name);
    else
      HPP.PrintF(L"sShader *%s();\n",ns->Name);
  }

  sFORALL(NewMtrls,nm)
  {
    if(sCmpString(nm->BaseName,L""))
      HPP.PrintF(L"class %s : public %s\n",nm->Name,nm->BaseName);
    else
      HPP.PrintF(L"class %s\n",nm->Name);

    HPP.PrintF(L"{\n");
    for(sInt i=0;i<MAXSHADER;i++)
    {
      if(nm->Shaders[i])
      {
        if(nm->Shaders[i]->PermuteCount)
          HPP.PrintF(L"  static sShader *%s(sInt);\n",shadername[i]);
        else
          HPP.PrintF(L"  static sShader *%s();\n",shadername[i]);
      }
    }

    HPP.PrintF(L"public:\n");
    if(!nm->New.IsEmpty()) 
      HPP.PrintF(L"  %s();\n",nm->Name);
    HPP.PrintF(L"  void SelectShaders(sVertexFormatHandle *);\n");
    if(!nm->Header.IsEmpty())
    {
      PrintCode(HPP,nm->Header);
      HPP.PrintF(L"#line %d \"%pcpp\"\n",sCountChar(HPP.Get(),'\n')+2,Filename);
    }
    HPP.PrintF(L"};\n");
  }

  sFORALL(ComputeShaders,nm)
  {
    if(nm->Shaders[_CS_])
    {
      if(nm->Shaders[_CS_]->PermuteCount)
        HPP.PrintF(L"sShader *%s(sInt);\n",nm->Name);
      else 
        HPP.PrintF(L"sShader *%s();\n",nm->Name);
    }
  }

  HPP.Print(L"\n");
  HPP.Print(L"/****************************************************************************/\n");
  HPP.Print(L"\n");

  HPP.Print(L"#endif\n");
  HPP.Print(L"\n");
}

/****************************************************************************/

void Compiler::OutputShader(NewShader *ns)
{
  sString<256> buffer;
  sString<256> ifdef;

  if(ns->PermuteCount)
  {
    CPP.PrintF(L"sShader *%s(sInt i)\n",ns->Name);
    CPP.Print(L"{\n");
    for(sInt i=0;i<ns->PermuteCount;i++)
    {
      if(ns->Permutes[i])
      {
        ifdef[0]=0;
        sSPrintF(buffer,L"data_%04x",i);
        sPrintNewShaderData(CPP,buffer,ns->Permutes[i],ifdef);
      }
    }
    if(ifdef.IsEmpty())
      sAppendString(ifdef,L" 0");
    CPP.PrintF(L"#if%s\n",ifdef);
    CPP.PrintF(L"  static const sU8 *codes[] =\n");
    CPP.Print(L"  {\n");
    for(sInt i=0;i<ns->PermuteCount;i++)
    {
      if(ns->Permutes[i])
      {
        sSPrintF(buffer,L"data_%04x",i);
        CPP.PrintF(L"    (const sU8 *)%s,\n",buffer);
      }
      else
      {
        CPP.Print(L"    0,\n");
      }
    }
    CPP.Print(L"  };\n");
    CPP.PrintF(L"  static sDInt sizes[] =\n");
    CPP.Print(L"  {\n");
    for(sInt i=0;i<ns->PermuteCount;i++)
    {
      if(ns->Permutes[i])
      {
        sSPrintF(buffer,L"data_%04x",i);
        CPP.PrintF(L"    sizeof(%s),\n",buffer);
      }
      else
      {
        CPP.Print(L"    0,\n");
      }
    }
    CPP.Print(L"  };\n");
    CPP.Print(L"  sVERIFY(codes[i])\n");  
    CPP.PrintF(L"#endif\n");
    CPP.Print(L"\n");

    sPrintNewShaderCode(CPP,L"codes[i]",L"sizes[i]",ns);
    CPP.Print(L"}\n");
    CPP.Print(L"\n");
    //CPP.PrintF(L"sShader *%s(sInt perm);\n",ns->Name);
  }
  else if(ns->ByteCode)
  {
    CPP.PrintF(L"sShader *%s()\n",ns->Name);
    CPP.Print(L"{\n");
    sPrintNewShaderData(CPP,L"data",ns->ByteCode,ifdef);
    sPrintNewShaderCode(CPP,L"data",L"sizeof(data)",ns);
    CPP.Print(L"}\n");
    CPP.Print(L"\n");
  }
}

/****************************************************************************/

void Compiler::OutputCPP()
{
  sTextBuffer tb;
  ACType *type;
  ACExtern *ext;
  sString<256> buffer;
  NewShader *ns;
  NewMaterial *nm;


  CPP.Print(L"/****************************************************************************/\n");
  CPP.Print(L"/***                                                                      ***/\n");
  CPP.PrintF(L"/***   source file generated by ASC %1d.%2d - altona shader compiler.        ***/\n",VERSION,REVISION);
  CPP.Print(L"/***                                                                      ***/\n");
  CPP.Print(L"/****************************************************************************/\n");
  CPP.Print(L"\n");
  CPP.PrintF(L"#include \"%phpp\"\n",Filename);

  CPP.Print(L"\n");
  CPP.Print(L"/****************************************************************************/\n");
  CPP.Print(L"\n");

  // cbuffers

  sFORALL(ASC->UserTypes,type)
  {
    if(type->Type==ACT_CBUFFER)
    {
      sFORALL(type->Externs,ext)
      {
        CPP.PrintF(L"#line %d \"%p\"\n",ext->ParaLine,ext->File);
        CPP.PrintF(L"%s %s::%s(%s)\n",ext->Result,type->Name,ext->Name,ext->Para);
        CPP.PrintF(L"#line %d \"%p\"\n",ext->BodyLine,ext->File);
        CPP.PrintF(L"{%s}\n",ext->Body);
      }
      CPP.PrintF(L"#line %d \"%pcpp\"\n",sCountChar(CPP.Get(),'\n')+2,Filename);
    }
  }

  CPP.Print(L"\n");
  CPP.Print(L"/****************************************************************************/\n");
  CPP.Print(L"\n");

  // new shaders

  sFORALL(NewShaders,ns)
  {
    OutputShader(ns);
  }

  CPP.Print(L"\n");
  CPP.Print(L"/****************************************************************************/\n");
  CPP.Print(L"\n");

  sFORALL(NewMtrls,nm)
  {
    for(sInt i=0;i<MAXSHADER;i++)
    {
      if(nm->Shaders[i])
      {
        sSPrintF(buffer,L"%s::%s",nm->Name,shadername[i]);
        nm->Shaders[i]->Name = buffer;
        OutputShader(nm->Shaders[i]);
      }
    }


    if(!nm->New.IsEmpty())
    {
      CPP.PrintF(L"%s::%s()\n",nm->Name,nm->Name);
      CPP.Print(L"{\n");
      PrintCode(CPP,nm->New);
      CPP.PrintF(L"#line %d \"%pcpp\"\n",sCountChar(CPP.Get(),'\n')+2,Filename);
      CPP.Print(L"}\n");
    }

    if(!nm->Code.IsEmpty())
    {
      PrintCode(CPP,nm->Code);
      CPP.PrintF(L"#line %d \"%pcpp\"\n",sCountChar(CPP.Get(),'\n')+2,Filename);
    }

    CPP.PrintF(L"void %s::SelectShaders(sVertexFormatHandle *format)\n",nm->Name);
    CPP.Print(L"{\n");
    if(!nm->Prepare.IsEmpty())
    {
      PrintCode(CPP,nm->Prepare);
      CPP.PrintF(L"#line %d \"%pcpp\"\n",sCountChar(CPP.Get(),'\n')+2,Filename);
    }
    else
    {
      for(sInt i=0;i<MAXSHADER;i++)
        if(nm->Shaders[i])
          CPP.PrintF(L"  %s = %s();\n",fullshadername[i],nm->Shaders[i]->Name);
    }
    CPP.Print(L"}\n");
    CPP.Print(L"\n");
  }
  CPP.Print(L"\n");
  CPP.Print(L"/****************************************************************************/\n");
  CPP.Print(L"\n");

  sFORALL(ComputeShaders,nm)
  {
    if(nm->Shaders[_CS_])
    {
      nm->Shaders[_CS_]->Name = nm->Name;
      OutputShader(nm->Shaders[_CS_]);
    }
  }


  CPP.Print(L"\n");
  CPP.Print(L"/****************************************************************************/\n");
  CPP.Print(L"\n");
}

/****************************************************************************/

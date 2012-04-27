/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "shadercomp/asc_doc.hpp"
#include "base/graphics.hpp"
/****************************************************************************/

void ACDoc::Print(const sChar *str) 
{ 
  Out.Print(str); 
  while(*str)
    if(*str++=='\n')
      CurrentLine++;
}

void ACDoc::PrintLine(sInt line,const sChar *file)
{
  Print(L"\n");
  if(CurrentLine!=line || CurrentFile!=file)
  {
    CurrentLine = line;
    CurrentFile = file;
    Out.PrintF(L"#line %d \"%p\"\n",CurrentLine,CurrentFile);
  }
}

/****************************************************************************/

void ACDoc::Output(sInt language,sInt renderer)
{
  ACGlobal *gl;
  ACFunc *func;
  ACVar *member;
  OutputLanguage = language;
  OutputRender = renderer;

  Out.Clear();
  CurrentLine = 0;
  CurrentFile = 0;
   
  sFORALL(Program,gl)
  {
    PrintLine(gl->SourceLine,gl->SourceFile);
    switch(gl->Kind)
    {
    case ACG_VARIABLE:
      if(gl->Var->Permute==0 || ConstFoldInt(gl->Var->Permute))
      {
        if(((this->OutputLanguage & sSTF_PLATFORM)==sSTF_HLSL45) && gl->Var && gl->Var->Type && gl->Var->Type->Type==ACT_SAMPLER)
        {
          const sChar *dim=L"XX";
          switch(gl->Var->Type->Rows)
          {
          case 1: dim = L"1D"; break;
          case 2: dim = L"2D"; break;
          case 3: dim = L"3D"; break;
          case 4: dim = L"Cube"; break;
          }
          PrintF(L"Texture%s %s : register(t%d);\n",dim,gl->Var->Name,gl->Var->RegisterNum);
          PrintF(L"SamplerState %s_ : register(s%d);\n",gl->Var->Name,gl->Var->RegisterNum);
        }
        else
        {
          OutVar(gl->Var,gl->Init);
          Print(L";");
        }
      }
      break;

    case ACG_FUNC:
      func = (ACFunc *) gl->Var;
      
      OutVar(func,0);
      OutBlock(func->FuncBody,0);
      break;

    case ACG_ATTRIBUTE:
      if(gl->Permute==0 || ConstFoldInt(gl->Permute))
        PrintF(L"[%s]\n",gl->Attribute);
      break;

    case ACG_PRAGMA:
      if(gl->Permute==0 || ConstFoldInt(gl->Permute))
        PrintF(L"#pragma %s\n",gl->Attribute);
      break;

    case ACG_TYPEDEF:
      if(gl->Type->Type==ACT_STRUCT)
      {
        PrintF(L"struct %s",gl->Type->Name);
        OutStruct(gl->Type);
        Print(L";");
      }
      else
      {
        Print(L"typedef ");
        OutTypeDecoration(gl->Type,gl->Type->Name);
      }
      break;

    case ACG_USE:
      if((OutputLanguage & sSTF_PLATFORM)==sSTF_HLSL45)
      {
        if(gl->Type->Members.GetCount()>0)
        {
          PrintF(L"cbuffer __%d : register(b%d)\n{\n",gl->Type->CSlot,gl->Type->CSlot&(sCBUFFER_MAXSLOT)-1);
          sInt base = gl->Type->Members[0]->RegisterNum;
          sFORALL(gl->Type->Members,member)
          {
            PrintF(L"  ");
            member->RegisterNum -= base;
            OutVar(member,0);
            member->RegisterNum += base;
            PrintF(L";\n");
          }
          Print(L"}");
        }
      }
      else
      {
        sFORALL(gl->Type->Members,member)
        {
          Print(L"\n");
          OutVar(member,0);
          Print(L";");
        }
      }
      break;
    }
  }
  Print(L"\n");

  if(!sRELEASE)
  {
    const sChar *shad=L"???";
    const sChar *prof=L"???";
    const sChar *plat=L"???";

    switch(language&sSTF_KIND)
    {
    case sSTF_VERTEX: shad = L"vertex"; break;
    case sSTF_PIXEL: shad = L"pixel"; break;
    case sSTF_GEOMETRY: shad = L"geometry"; break;
    case sSTF_HULL: shad = L"hull"; break;
    case sSTF_DOMAIN: shad = L"domain"; break;
    case sSTF_COMPUTE: shad = L"compute"; break;
    }
    switch(language&sSTF_PLATFORM)
    {
    case sSTF_HLSL23: prof = L"hlsl23"; break;
    case sSTF_GLSL: prof = L"glsl"; break;
    case sSTF_CG: prof = L"cg"; break;
    case sSTF_HLSL45: prof = L"hlsl45"; break;
    }
    switch(renderer)
    {
    case sRENDER_DX9: plat=L"dx9"; break;
    case sRENDER_OGL2: plat=L"ogl2"; break;
    case sRENDER_DX11: plat=L"dx11"; break;
    }
    sDPrintF(L"\n--------- %s %s %s -------------\n\n",shad,prof,plat);
    sDPrint(Out.Get());
  }
}

void ACDoc::OutStruct(ACType *type)
{
  ACVar *member;
  Print(L"\n{");
  sFORALL(type->Members,member)
  {
    Print(L"\n  ");
    OutVar(member,0);
    Print(L";");
  }
  Print(L"\n}");
}

void ACDoc::OutTypeDecoration(ACType *type,sPoolString name)
{
  const sChar *tname = type->Name;
  switch(type->Type)
  {
  case ACT_UINT:
    if((OutputLanguage&sSTF_PLATFORM)==sSTF_HLSL23)
    {
      sVERIFY(tname[0]=='u');
      tname++;
    }
    // no break
  case ACT_VOID:
  case ACT_BOOL:
  case ACT_INT:
  case ACT_FLOAT:
  case ACT_DOUBLE:
  case ACT_HALF:
  case ACT_STRUCT:
  case ACT_TEX:
  case ACT_SAMPLERSTATE:
  case ACT_SAMPLER:
  case ACT_STREAM:
    if(type->Template && type->TemplateCount)
      PrintF(L"%s<%s,%d> ",tname,type->Template->Name,type->TemplateCount);
    else if(type->Template)
      PrintF(L"%s<%s> ",tname,type->Template->Name);
    else
      PrintF(L"%s ",tname);
    break;

  case ACT_ARRAY:
    // array is handled by printing base type, then dimension
    break;

  default:
    // unknown/non-printable type is a bug
    sVERIFYFALSE;

/*
    switch(type->Rows)
    {
    case 1: Print(L"sampler1D "); break;
    case 2: Print(L"sampler2D "); break;
    case 3: Print(L"sampler3D "); break;
    case 4: Print(L"samplerCUBE "); break;
    }
    break;
    */
  }
  if(type->BaseType==0)
    Print(name);
  else
    OutTypeDecoration(type->BaseType,name);
  switch(type->Type)
  {
  case ACT_ARRAY:
    PrintF(L"[");
    OutExpr(type->ArrayCountExpr);
    PrintF(L"]");
    break;
  }
}

void ACDoc::OutVar(ACVar *var,ACExpression *init)
{
  if((var->Usages & ACF_INOUT)==ACF_IN) Print(L"in ");
  if((var->Usages & ACF_INOUT)==ACF_OUT) Print(L"out ");
  if((var->Usages & ACF_INOUT)==ACF_INOUT) Print(L"inout ");
  if(var->Usages & ACF_UNIFORM) Print(L"uniform ");
  if(var->Usages & ACF_ROWMAJOR) Print(L"row_major ");
  if(var->Usages & ACF_COLMAJOR) Print(L"column_major ");
  if(var->Usages & ACF_CONST) Print(L"const ");
  if(var->Usages & ACF_INLINE) Print(L"inline ");
  if(var->Usages & ACF_STATIC) Print(L"static ");

  if(var->Usages & ACF_POINT) Print(L"point ");
  if(var->Usages & ACF_LINE) Print(L"line ");
  if(var->Usages & ACF_TRIANGLE) Print(L"triangle ");
  if(var->Usages & ACF_LINEADJ) Print(L"lineadj ");
  if(var->Usages & ACF_TRIANGLEADJ) Print(L"triangleadj ");
  if(var->Usages & ACF_GROUPSHARED) Print(L"groupshared ");


  OutTypeDecoration(var->Type,var->Name);
  if(init)
  {
    Print(L" = ");
    OutExpr(init);
  }
  if(var->Function)
  {
    ACFunc *func = (ACFunc *) var;
    ACVar *para;
    Print(L"\n(");
    sBool first=1;
    sFORALL(func->Locals[SCOPE_PARA],para)
    {
      if(para->Permute==0 || ConstFoldInt(para->Permute))
      {
        if(!first)
          Print(L",");
        first = 0;
        Print(L"\n  ");
        OutVar(para,0);
      }
    }
    Print(L"\n)");
  }


  if(!var->Semantic.IsEmpty())
  {
    const sChar *semantic = var->Semantic;

    // convert normal -> attrib2 etc.
    // don't check for usage OUT, since functions have output semantic but this flag is not set!
    
    if(OutputLanguage==(sSTF_HLSL45|sSTF_GEOMETRY))
    {
      if(sCmpStringI(semantic,L"position")==0) semantic = L"SV_POSITION";
    }
    if(OutputLanguage==(sSTF_HLSL45|sSTF_VERTEX))
    {
      if(var->Usages==ACF_OUT && sCmpStringI(semantic,L"position")==0) semantic = L"SV_POSITION";
    }
    if(OutputLanguage==(sSTF_HLSL45|sSTF_PIXEL))
    {
      sBool in = (var->Usages==ACF_IN);
      sBool out = ((var->Usages==ACF_OUT)||(var->Function));
      if(in  && sCmpStringI(semantic,L"vpos")==0) semantic = L"SV_POSITION";
      if(in  && sCmpStringI(semantic,L"position")==0) semantic = L"SV_POSITION";
      if(out && sCmpStringI(semantic,L"color")==0) semantic = L"SV_TARGET0";
      if(out && sCmpStringI(semantic,L"color0")==0) semantic = L"SV_TARGET0";
      if(out && sCmpStringI(semantic,L"color1")==0) semantic = L"SV_TARGET1";
      if(out && sCmpStringI(semantic,L"color2")==0) semantic = L"SV_TARGET2";
      if(out && sCmpStringI(semantic,L"color3")==0) semantic = L"SV_TARGET3";
      if(out && sCmpStringI(semantic,L"depth")==0) semantic = L"SV_DEPTH";
    }
    
    if((OutputLanguage==(sSTF_CG|sSTF_VERTEX)) && (var->Usages & ACF_IN))
    {
           if(sCmpStringI(semantic,L"position")==0)  semantic = L"ATTR0";
      else if(sCmpStringI(semantic,L"normal")==0)    semantic = L"ATTR2";
      else if(sCmpStringI(semantic,L"color")==0)     semantic = L"ATTR3";
      else if(sCmpStringI(semantic,L"color0")==0)    semantic = L"ATTR3";
      else if(sCmpStringI(semantic,L"color1")==0)    semantic = L"ATTR4";
      else if(sCmpStringI(semantic,L"fogcoord")==0)  semantic = L"ATTR5";
      else if(sCmpStringI(semantic,L"texcoord")==0)  semantic = L"ATTR8";
      else if(sCmpStringI(semantic,L"texcoord0")==0) semantic = L"ATTR8";
      else if(sCmpStringI(semantic,L"texcoord1")==0) semantic = L"ATTR9";
      else if(sCmpStringI(semantic,L"texcoord2")==0) semantic = L"ATTR10";
      else if(sCmpStringI(semantic,L"texcoord3")==0) semantic = L"ATTR11";
      else if(sCmpStringI(semantic,L"texcoord4")==0) semantic = L"ATTR12";
      else if(sCmpStringI(semantic,L"texcoord5")==0) semantic = L"ATTR13";
      else if(sCmpStringI(semantic,L"texcoord6")==0) semantic = L"ATTR14";
      else if(sCmpStringI(semantic,L"texcoord7")==0) semantic = L"ATTR15";
    }
    if((OutputLanguage==(sSTF_CG|sSTF_PIXEL)) && (var->Usages & ACF_IN))
    {
           if(sCmpStringI(semantic,L"vpos")==0)      semantic = L"WPOS";
      else if(sCmpStringI(semantic,L"texcoord_centroid")==0)  semantic = L"TEXCOORD0_CENTROID";
      else if(sCmpStringI(semantic,L"texcoord0_centroid")==0) semantic = L"TEXCOORD0_CENTROID";
      else if(sCmpStringI(semantic,L"texcoord1_centroid")==0) semantic = L"TEXCOORD1_CENTROID";
      else if(sCmpStringI(semantic,L"texcoord2_centroid")==0) semantic = L"TEXCOORD2_CENTROID";
      else if(sCmpStringI(semantic,L"texcoord3_centroid")==0) semantic = L"TEXCOORD3_CENTROID";
      else if(sCmpStringI(semantic,L"texcoord4_centroid")==0) semantic = L"TEXCOORD4_CENTROID";
      else if(sCmpStringI(semantic,L"texcoord5_centroid")==0) semantic = L"TEXCOORD5_CENTROID";
      else if(sCmpStringI(semantic,L"texcoord6_centroid")==0) semantic = L"TEXCOORD6_CENTROID";
      else if(sCmpStringI(semantic,L"texcoord7_centroid")==0) semantic = L"TEXCOORD7_CENTROID";
    }

    PrintF(L" : %s",semantic);
  }
  if(var->RegisterType)
    PrintF(L" : register(%c%d)",var->RegisterType,var->RegisterNum);
}

void ACDoc::OutBlock(ACStatement *stat,sInt indent)
{
  sBool braces = (indent==0) || stat->Next || stat->Op==ACS_VARDECL;
  if(braces)
    PrintF(L"\n%_{",indent);
  while(stat)
  {
    OutStat(stat,indent+2);
    stat = stat->Next;
  }
  if(braces)
    PrintF(L"\n%_}",indent);
}

void ACDoc::OutStat(ACStatement *stat,sInt indent)
{
  if(stat->Op!=ACS_BLOCK)
    PrintLine(stat->SourceLine,stat->SourceFile);
  else
    Print(L"\n");
  switch(stat->Op)
  {
  default:
    PrintF(L"%_;",indent);
    break;
  case ACS_WHILE:
    PrintF(L"%_while(",indent);
    OutExpr(stat->Cond);
    PrintF(L")");
    OutBlock(stat->Body,indent);
    break;
  case ACS_FOR:
    PrintF(L"%_for(",indent);
    if(stat->Else->Op==ACS_EXPRESSION)
      OutExpr(stat->Else->Expr);
    else if(stat->Else->Op==ACS_VARDECL)
      OutVar(stat->Else->Var,stat->Else->Expr);
    Print(L";");
    OutExpr(stat->Cond);
    Print(L";");
    OutExpr(stat->Expr);
    Print(L")");
    OutBlock(stat->Body,indent);
    break;
  case ACS_IF:
    PrintF(L"%_if(",indent);
    OutExpr(stat->Cond);
    PrintF(L")");
    OutBlock(stat->Body,indent);
    if(stat->Else)
    {
      PrintF(L"\n%_else",indent);
      OutBlock(stat->Else,indent);
    }
    break;
  case ACS_PIF:
    if(ConstFoldInt(stat->Cond))
      OutBlock(stat->Body,indent);
    else if(stat->Else)
      OutBlock(stat->Else,indent);
    break;
  case ACS_EXPRESSION:
    PrintF(L"%_",indent);
    OutExpr(stat->Expr);
    PrintF(L";");
    break;
  case ACS_RETURN:
    PrintF(L"%_return ",indent);
    if(stat->Expr)
      OutExpr(stat->Expr);
    PrintF(L";");
    break;
  case ACS_BREAK:
    PrintF(L"%_break;",indent);
    break;
  case ACS_CONTINUE:
    PrintF(L"%_continue;",indent);
    break;
  case ACS_ASM:
    PrintF(L"%_asm {\n",indent);
    Print(stat->Expr->Literal->Value);
    PrintF(L"\n%_};\n",indent);
    break;
  case ACS_ATTRIBUTE:
    PrintF(L"%_[%s]\n",indent,stat->Expr->Literal->Value);
    break;
  case ACS_DO:
    PrintF(L"%_do",indent);
    OutBlock(stat->Body,indent);
    PrintF(L"\n%_while(",indent);
    OutExpr(stat->Cond);
    PrintF(L");");
    break;
  case ACS_BLOCK:
    OutBlock(stat->Body,indent);
    break;
  case ACS_TYPEDECL:
    break;
  case ACS_VARDECL:
    if(stat->Var->Permute==0 || ConstFoldInt(stat->Var->Permute))
    {
      PrintF(L"%_",indent);
      OutVar(stat->Var,stat->Expr);
      PrintF(L";");
    }
    break;
  }
}

void ACDoc::OutBinary(ACExpression *left,ACExpression *right,const sChar *name,sBool brackets)
{
  if(brackets)
    Print(L"(");
  if(left)
    OutExpr(left,1); 
  Print(name); 
  if(right)
    OutExpr(right,1);
  if(brackets)
    Print(L")");
}

void ACDoc::OutExpr(ACExpression *expr,sBool brackets)
{
  switch(expr->Op)
  {
  // binary:
  case ACE_ADD:         OutBinary(expr->Left,expr->Right,L"+",brackets); break;
  case ACE_SUB:         OutBinary(expr->Left,expr->Right,L"-",brackets); break;
  case ACE_MUL:         OutBinary(expr->Left,expr->Right,L"*",brackets); break;
  case ACE_DIV:         OutBinary(expr->Left,expr->Right,L"/",brackets); break;
  case ACE_MOD:         OutBinary(expr->Left,expr->Right,L"%",brackets); break;
  case ACE_ASSIGN:      OutBinary(expr->Left,expr->Right,brackets ? L"=" : L" = ",brackets); break;
  case ACE_ASSIGN_ADD:  OutBinary(expr->Left,expr->Right,brackets ? L"+=" : L" += ",brackets); break;
  case ACE_ASSIGN_SUB:  OutBinary(expr->Left,expr->Right,brackets ? L"-=" : L" -= ",brackets); break;
  case ACE_ASSIGN_MUL:  OutBinary(expr->Left,expr->Right,brackets ? L"*=" : L" *= ",brackets); break;
  case ACE_ASSIGN_DIV:  OutBinary(expr->Left,expr->Right,brackets ? L"/=" : L" /= ",brackets); break;
  case ACE_AND:         OutBinary(expr->Left,expr->Right,L"&&",brackets); break;
  case ACE_OR:          OutBinary(expr->Left,expr->Right,L"||",brackets); break;
  case ACE_BAND:        OutBinary(expr->Left,expr->Right,L"&",brackets); break;
  case ACE_BXOR:        OutBinary(expr->Left,expr->Right,L"^",brackets); break;
  case ACE_BOR:         OutBinary(expr->Left,expr->Right,L"|",brackets); break;
  case ACE_SHIFTL:      OutBinary(expr->Left,expr->Right,L"<<",brackets); break;
  case ACE_SHIFTR:      OutBinary(expr->Left,expr->Right,L">>",brackets); break;
  case ACE_COND1:       OutBinary(expr->Left,expr->Right,L"?",brackets); break;
  case ACE_COND2:       OutBinary(expr->Left,expr->Right,L":",0); break;
  case ACE_GT:          OutBinary(expr->Left,expr->Right,L">",brackets); break;
  case ACE_LT:          OutBinary(expr->Left,expr->Right,L"<",brackets); break;
  case ACE_EQ:          OutBinary(expr->Left,expr->Right,L"==",brackets); break;
  case ACE_NE:          OutBinary(expr->Left,expr->Right,L"!=",brackets); break;
  case ACE_GE:          OutBinary(expr->Left,expr->Right,L">=",brackets); break;
  case ACE_LE:          OutBinary(expr->Left,expr->Right,L"<=",brackets); break;

  // unary

  case ACE_PREINC:      OutBinary(0,expr->Left,L"++",brackets); break;
  case ACE_PREDEC:      OutBinary(0,expr->Left,L"--",brackets); break;
  case ACE_NOT:         OutBinary(0,expr->Left,L"!",brackets); break;
  case ACE_POSITIVE:    OutBinary(0,expr->Left,L"+",brackets); break;
  case ACE_NEGATIVE:    OutBinary(0,expr->Left,L"-",brackets); break;
  case ACE_COMPLEMENT:  OutBinary(0,expr->Left,L"~",brackets); break;

  case ACE_POSTINC:     OutBinary(expr->Left,0,L"++",brackets); break;
  case ACE_POSTDEC:     OutBinary(expr->Left,0,L"--",brackets); break;

  // special

  case ACE_TRUE:
    Print(L"true");
    break;

  case ACE_FALSE:
    Print(L"false");
    break;

  case ACE_DOT:
    Print(L"dot(");
    OutExpr(expr->Left,0); 
    Print(L",");
    OutExpr(expr->Right,0); 
    Print(L")");
    break;

  case ACE_VAR:
    Print(expr->Variable->Name);
    break;

  case ACE_CONSTRUCT:
    PrintF(L"%s(",expr->CastType->Name);
    {
      ACExpression *para = expr->Left;
      sBool first = 1;
      while(para)
      {
        if(!first)
          Print(L",");
        first = 0;
        sVERIFY(para->Op==ACE_COMMA);
        OutExpr(para->Left,1); 
        para = para->Right;
      }
    }
    Print(L")");
    break;

  case ACE_CAST:
    PrintF(L"(%s)",expr->CastType->Name);
    OutExpr(expr->Left,1); 
    break;

  case ACE_LITERAL:
    if(expr->Literal->Next==0 && expr->Literal->Child==0)
      OutLitVal(expr->Literal);
    else
      OutLiteral(expr->Literal);
    break;

  case ACE_RENDER:
    PrintF(L"%d",(OutputRender==expr->Literal->ValueI));
    break;

  case ACE_INDEX:
    OutExpr(expr->Left);
    Print(L"[");
    OutExpr(expr->Right); 
    Print(L"]");
    break;

  case ACE_CALL:
    {
      sBool handled = 0;
      if(((this->OutputLanguage & sSTF_PLATFORM)==sSTF_HLSL45) && expr->Variable)
      {
        const sChar *dim = 0;
        sInt mode = 0;
        if(expr->Variable->Name==L"tex1D") dim = L"1D";
        if(expr->Variable->Name==L"tex2D") dim = L"2D";
        if(expr->Variable->Name==L"tex3D") dim = L"3D";
        if(expr->Variable->Name==L"texCUBE") dim = L"CUBE";
        if(expr->Variable->Name==L"tex2Dlod") { dim = L"2D"; mode = 1; }
        if(dim)
        {
          if(expr->Left && expr->Left->Left && expr->Left->Right->Left && !expr->Left->Right->Right)
          {
            ACExpression *sampler = expr->Left->Left;
            ACExpression *uv = expr->Left->Right->Left;
            handled = 1;

            OutExpr(sampler);
            if(mode==1)
              Print(L".SampleLevel(");
            else
              Print(L".Sample(");
            OutExpr(sampler);
            Print(L"_,");
            if(mode==1)         //  s.Sample(s_,uv.xy,uv.w);
            {
              OutExpr(uv);
              Print(L".xy,");
              OutExpr(uv);
              Print(L".w)");
            }
            else                //  s.Sample(s_,uv);
            {
              OutExpr(uv);
              Print(L")");
            }
          }
        }
      }
      

      if(!handled)
      {
        ACExpression *para = expr->Left;
        if(expr->Variable)
          Print(expr->Variable->Name);
        Print(L"(");
        sBool first = 1;
        while(para)
        {
          if(!first)
            Print(L",");
          first = 0;
          sVERIFY(para->Op==ACE_COMMA);
          OutExpr(para->Left);
          para = para->Right;
        }
        Print(L")");
      }
    }
    break;

  case ACE_MEMBER:
    OutExpr(expr->Left);
    PrintF(L".%s",expr->Member);
    if(expr->Right)           // this would be function call after member
      OutExpr(expr->Right);
    break;

  case ACE_PERMUTE:
    PrintF(L"%d",expr->Permute->Value);
    break;

  default:
    Print(L"???");
    break;
  }
}

void ACDoc::OutLiteral(ACLiteral *lit)
{
  Print(L"{");
  OutLitVal(lit);
  lit = lit->Next;
  while(lit)
  {
    Print(L",");
    OutLitVal(lit);
    lit = lit->Next;
  }
  Print(L"}");
}

void ACDoc::OutLitVal(ACLiteral *lit)
{
  if(lit->Child)
    OutLiteral(lit->Child);
  else
  {
    if(lit->Expr)
      OutExpr(lit->Expr);
    else
      Print(lit->Value);
  }
}

/****************************************************************************/


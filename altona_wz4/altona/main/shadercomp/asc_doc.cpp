/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "shadercomp/asc_doc.hpp"

/****************************************************************************/

ACType::ACType()
{
  Name = L"";
  Type = 0;
  Rows = 0;
  Columns = 0;
  BaseType = 0;
  ArrayCountExpr = 0;
  ArrayCount = 0;

  CRegStart = 0;
  CRegCount = 0;
  CSlot = 0;
  Template = 0;
  TemplateCount = 0;
}

ACType::~ACType()
{
  sDeleteAll(Externs);
}

void ACType::SizeOf(sInt &c,sInt &r)
{
  sInt cc,rr;
  ACVar *member;

  switch(Type)
  {
  case ACT_VOID:                  // illegal items
  case ACT_FUNC:
  case ACT_SAMPLER:
    c = r = 0;
    break;
  case ACT_INT:                   // normal types
  case ACT_UINT:
  case ACT_BOOL:
  case ACT_FLOAT:
  case ACT_DOUBLE:
  case ACT_HALF:
    c = Columns; if(c==0) c = 1;
    r = Rows; if(r==0) r = 1;
    break;
  case ACT_STRUCT:                // sum all members
  case ACT_CBUFFER:
    c = 0;
    r = 4;
    sFORALL(Members,member)
    {
      member->Type->SizeOf(cc,rr);
      if((member->Usages & ACF_ROWMAJOR) && cc>1 && rr>1)
        sSwap(cc,rr);
      c += cc;
    }
    break;
  case ACT_ARRAY:                 // array -> multiply
    BaseType->SizeOf(c,r);
    r = 4;
    if(ArrayCount==0)
      sFatal(L"could not determine array count");
    c *= ArrayCount;
    break;
  }
}

/****************************************************************************/

ACVar::ACVar()
{
  Name = L"";
  Type = 0;
  Semantic = L"";
  RegisterType = 0;
  RegisterNum = -1;
  Usages = 0;
  Function = 0;
  Permute = 0;
}

ACVar::~ACVar()
{
}

void ACVar::CopyFrom(ACVar *src)
{
  Name = src->Name;
  Type = src->Type;
  Semantic = src->Semantic;
  RegisterType = src->RegisterType;
  Usages = src->Usages;
  Function = src->Function;
}

/****************************************************************************/

ACExtern::ACExtern()
{
  File = 0;
  ParaLine = 0;
  BodyLine = 0;
}

/****************************************************************************/

ACFunc::ACFunc()
{
  Function = 1;
}

ACFunc::~ACFunc()
{
}

/****************************************************************************/
/****************************************************************************/

sBool ACDoc::IsValidPermutation(ACPermute *perm,sInt n)
{
  ACPermuteMember *mem;
  ACExpression *ass;

  sInt count = 1<<perm->MaxShift;
  if(n<0 || n>=count) return 0;

  sFORALL(perm->Members,mem)
  {
    if(mem->Group)
    {
      mem->Value = ((n>>mem->Shift) & mem->Mask);
      if( mem->Value >= mem->Max)
        return 0;
    }
  }
  sFORALL(perm->Asserts,ass)
  {
    if(!ConstFoldInt(ass))
      return 0;
  }
  return 1;
}



/****************************************************************************/
/****************************************************************************/

sInt ACDoc::ConstFoldInt(ACExpression *expr)
{
  switch(expr->Op)
  {
  case ACE_ADD:      return ConstFoldInt(expr->Left) +  ConstFoldInt(expr->Right);
  case ACE_SUB:      return ConstFoldInt(expr->Left) -  ConstFoldInt(expr->Right);
  case ACE_MUL:      return ConstFoldInt(expr->Left) *  ConstFoldInt(expr->Right);
  case ACE_DIV:      return ConstFoldInt(expr->Left) /  ConstFoldInt(expr->Right);
  case ACE_MOD:      return ConstFoldInt(expr->Left) %  ConstFoldInt(expr->Right);

  case ACE_AND:      return ConstFoldInt(expr->Left) && ConstFoldInt(expr->Right);
  case ACE_OR:       return ConstFoldInt(expr->Left) || ConstFoldInt(expr->Right);
  case ACE_BAND:     return ConstFoldInt(expr->Left) & ConstFoldInt(expr->Right);
  case ACE_BXOR:     return ConstFoldInt(expr->Left) ^ ConstFoldInt(expr->Right);
  case ACE_BOR:      return ConstFoldInt(expr->Left) | ConstFoldInt(expr->Right);

  case ACE_GT:       return ConstFoldInt(expr->Left) >  ConstFoldInt(expr->Right);
  case ACE_LT:       return ConstFoldInt(expr->Left) <  ConstFoldInt(expr->Right);
  case ACE_GE:       return ConstFoldInt(expr->Left) >= ConstFoldInt(expr->Right);
  case ACE_LE:       return ConstFoldInt(expr->Left) <= ConstFoldInt(expr->Right);
  case ACE_EQ:       return ConstFoldInt(expr->Left) == ConstFoldInt(expr->Right);
  case ACE_NE:       return ConstFoldInt(expr->Left) != ConstFoldInt(expr->Right);

  case ACE_NOT:      return !ConstFoldInt(expr->Left);
  case ACE_POSITIVE: return  ConstFoldInt(expr->Left);
  case ACE_NEGATIVE: return -ConstFoldInt(expr->Left);
  case ACE_COMPLEMENT: return ~ConstFoldInt(expr->Left);

  case ACE_LITERAL:
    if(expr->Literal->Next==0 && expr->Literal->Child==0 && expr->Literal->Token==sTOK_INT)
      return expr->Literal->ValueI;
    break;

  case ACE_RENDER:
    return expr->Literal->ValueI == OutputRender;

  case ACE_PERMUTE:
    return expr->Permute->Value;
  }
  Scan.Error(L"constant expression expected");
  return 0;
}

/****************************************************************************/

ACType *ACDoc::FindType(sPoolString name)
{
  ACType *type;

  type = sFind(DefaultTypes,&ACType::Name,name);
  if(type) return type;
  type = sFind(UserTypes,&ACType::Name,name);
  return type;
}

ACFunc *ACDoc::FindFunc(sPoolString name)
{
  ACFunc *func;

  func = (ACFunc *)sFind(Intrinsics,&ACFunc::Name,name);
  if(func) return func;
  func = (ACFunc *)sFind(Functions,&ACFunc::Name,name);
  return func;
}

ACVar *ACDoc::FindVar(sPoolString name)
{
  ACVar *var;
  if(Function)
  {
    for(sInt i=Scope;i>=0;i--)
    {
      var = sFind(Function->Locals[i],&ACVar::Name,name);
      if(var) return var;
    }

    var = sFind(Variables,&ACVar::Name,name);               // search globals
    if(var)
      Function->Locals[SCOPE_GLOBAL].AddTail(var);          // if found, add to used globals. we need to filter globals for automatic register distribution
    return var;
  }
  else
  {
    var = sFind(Variables,&ACVar::Name,name);
    return var;    
  }
}

void ACDoc::AddDefTypes(const sChar *name,sInt type,sBool many)
{
  sString<16> buffer;
  AddDefType(name,type,0,0);
  if(many)
  {
    for(sInt i=1;i<=4;i++)
    {
      sSPrintF(buffer,L"%s%d",name,i);
      AddDefType(buffer,type,i,1);
    }
    for(sInt i=1;i<=4;i++)
    {
      for(sInt j=1;j<=4;j++)
      {
        sSPrintF(buffer,L"%s%dx%d",name,i,j);
        AddDefType(buffer,type,i,j);
      }
    }
  }
}

void ACDoc::AddDefType(const sChar *name,sInt kind,sInt row,sInt col)
{
  ACType *type = new ACType;
  type->Name = name;
  type->Rows = row;
  type->Columns = col;
  type->Type = kind;
  DefaultTypes.AddTail(type);
}

void ACDoc::AddDefFunc(const sChar *name)
{
  ACFunc *func= new ACFunc;
  func->Name = name;
  Intrinsics.AddTail(func);
}

ACExpression *ACDoc::NewExpr(sInt op,ACExpression *a,ACExpression *b)
{
  ACExpression *e = Pool->Alloc<ACExpression>();
  sClear(*e);
  e->Op = op;
  e->Left = a;
  e->Right = b;
  return e;
}

/****************************************************************************/

ACDoc::ACDoc()
{
  Pool = new sMemoryPool;
  Function = 0;
  UsePermute = 0;
  Scan.Init();
  Scan.Flags |= sSF_TYPEDNUMBERS;
  Scan.DefaultTokens();
  Scan.AddTokens(L"°");

  Scan.AddToken(L"#line",AC_TOK_LINE);
  Scan.AddToken(L"+=",AC_TOK_ASSIGNADD);
  Scan.AddToken(L"-=",AC_TOK_ASSIGNSUB);
  Scan.AddToken(L"*=",AC_TOK_ASSIGNMUL);
  Scan.AddToken(L"/=",AC_TOK_ASSIGNDIV);
  Scan.AddToken(L"&&",AC_TOK_ANDAND);
  Scan.AddToken(L"||",AC_TOK_OROR);
  Scan.AddToken(L"++",AC_TOK_INC);
  Scan.AddToken(L"--",AC_TOK_DEC);
  Scan.AddToken(L"true",AC_TOK_TRUE);
  Scan.AddToken(L"false",AC_TOK_FALSE);
  Scan.AddToken(L"implies",AC_TOK_IMPLIES);

  Scan.AddToken(L"if",AC_TOK_IF);
  Scan.AddToken(L"else",AC_TOK_ELSE);
  Scan.AddToken(L"while",AC_TOK_WHILE);
  Scan.AddToken(L"do",AC_TOK_DO);
  Scan.AddToken(L"for",AC_TOK_FOR);
  Scan.AddToken(L"return",AC_TOK_RETURN);
  Scan.AddToken(L"break",AC_TOK_BREAK);
  Scan.AddToken(L"continue",AC_TOK_CONTINUE);
  Scan.AddToken(L"asm",AC_TOK_ASM);
  Scan.AddToken(L"struct",AC_TOK_STRUCT);
  Scan.AddToken(L"cbuffer",AC_TOK_CBUFFER);
  Scan.AddToken(L"permute",AC_TOK_PERMUTE);
  Scan.AddToken(L"use",AC_TOK_USE);
  Scan.AddToken(L"extern",AC_TOK_EXTERN);
  Scan.AddToken(L"pif",AC_TOK_PIF);
  Scan.AddToken(L"pelse",AC_TOK_PELSE);
  Scan.AddToken(L"assert",AC_TOK_ASSERT);
  Scan.AddToken(L"register",AC_TOK_REGISTER);
  Scan.AddToken(L"pragma",AC_TOK_PRAGMA);

  Scan.AddToken(L"in",AC_TOK_IN);
  Scan.AddToken(L"inout",AC_TOK_INOUT);
  Scan.AddToken(L"out",AC_TOK_OUT);
  Scan.AddToken(L"uniform",AC_TOK_UNIFORM);
  Scan.AddToken(L"row_major",AC_TOK_ROWMAJOR);
  Scan.AddToken(L"column_major",AC_TOK_COLMAJOR);
  Scan.AddToken(L"const",AC_TOK_CONST);
  Scan.AddToken(L"inline",AC_TOK_INLINE);
  Scan.AddToken(L"static",AC_TOK_STATIC);
  Scan.AddToken(L"groupshared",AC_TOK_GROUPSHARED);

  AddDefType(L"void",ACT_VOID,0,0);
  AddDefType(L"bool",ACT_BOOL,0,0);

  AddDefTypes(L"float",ACT_FLOAT,1);
  AddDefTypes(L"half",ACT_HALF,1);
  AddDefTypes(L"double",ACT_DOUBLE,1);
  AddDefTypes(L"int",ACT_INT,1);
  AddDefTypes(L"uint",ACT_UINT,1);
  AddDefTypes(L"bool",ACT_BOOL,1);

  AddDefType(L"sampler1D",ACT_SAMPLER,1,0);
  AddDefType(L"sampler2D",ACT_SAMPLER,2,0);
  AddDefType(L"sampler3D",ACT_SAMPLER,3,0);
  AddDefType(L"samplerCUBE",ACT_SAMPLER,4,0);

  AddDefType(L"Texture1D",ACT_TEX,1,0);
  AddDefType(L"Texture2D",ACT_TEX,2,0);
  AddDefType(L"Texture3D",ACT_TEX,3,0);
  AddDefType(L"TextureCUBE",ACT_TEX,4,0);

  // dx11 types...

  AddDefType(L"AppendStructuredBuffer",ACT_TEX,0,0);
  AddDefType(L"Buffer",ACT_TEX,0,0);
  AddDefType(L"ByteAddressBuffer",ACT_TEX,0,0);
  AddDefType(L"ConsumeStructuredBuffer",ACT_TEX,0,0);
  AddDefType(L"RWBuffer",ACT_TEX,0,0);
  AddDefType(L"RWByteAddressBuffer",ACT_TEX,0,0);
  AddDefType(L"RWStructuredBuffer",ACT_TEX,0,0);
  AddDefType(L"RWTexture1D",ACT_TEX,0,0);
  AddDefType(L"RWTexture1DArray",ACT_TEX,0,0);
  AddDefType(L"RWTexture2D",ACT_TEX,0,0);
  AddDefType(L"RWTexture2DArray",ACT_TEX,0,0);
  AddDefType(L"RWTexture2D",ACT_TEX,0,0);
  AddDefType(L"StructuredBuffer",ACT_TEX,0,0);
  AddDefType(L"Texture1DArray",ACT_TEX,0,0);
  AddDefType(L"Texture2DArray",ACT_TEX,0,0);
  AddDefType(L"Texture2DMS",ACT_TEX,0,0);
  AddDefType(L"Texture2DMSArray",ACT_TEX,0,0);

  AddDefType(L"SamplerState",ACT_SAMPLERSTATE,1,0);
  AddDefType(L"PointStream",ACT_STREAM,0,0);
  AddDefType(L"LineStream",ACT_STREAM,0,0);
  AddDefType(L"TriangleStream",ACT_STREAM,0,0);
  AddDefType(L"InputPatch",ACT_STREAM,0,0);
  AddDefType(L"OutputPatch",ACT_STREAM,0,0);

  AddDefFunc(L"abort");
  AddDefFunc(L"abs");
  AddDefFunc(L"acos");
  AddDefFunc(L"all");
  AddDefFunc(L"AllMemoryBarrer");
  AddDefFunc(L"AllMemoryBarrierWithGroupSync");
  AddDefFunc(L"any");
  AddDefFunc(L"asdouble");
  AddDefFunc(L"asfloat");
  AddDefFunc(L"asin");
  AddDefFunc(L"asint");
  AddDefFunc(L"asuint");
  AddDefFunc(L"atan");
  AddDefFunc(L"atan2");
  AddDefFunc(L"ceil");
  AddDefFunc(L"clamp");
  AddDefFunc(L"clip");
  AddDefFunc(L"cos");
  AddDefFunc(L"cosh");
  AddDefFunc(L"countbits");
  AddDefFunc(L"cross");
  AddDefFunc(L"D3DCOLORtoUBYTE4");
  AddDefFunc(L"ddx");
  AddDefFunc(L"ddx_coarse");
  AddDefFunc(L"ddx_fine");
  AddDefFunc(L"ddy");
  AddDefFunc(L"ddy_coarse");
  AddDefFunc(L"ddy_fine");
  AddDefFunc(L"degrees");
  AddDefFunc(L"determinant");
  AddDefFunc(L"DeviceMemoryBarrier");
  AddDefFunc(L"DeviceMemoryBarrierWithGroupSync");
  AddDefFunc(L"distance");
  AddDefFunc(L"dot");
  AddDefFunc(L"dst");
  AddDefFunc(L"errorf");
  AddDefFunc(L"EvaluateAttributeAtCentriod");
  AddDefFunc(L"EvaluateAttributeAtSample");
  AddDefFunc(L"EvaluateAttributeSnapped");
  AddDefFunc(L"exp");
  AddDefFunc(L"exp2");
  AddDefFunc(L"f16tof32");
  AddDefFunc(L"f32tof16");
  AddDefFunc(L"faceforward");
  AddDefFunc(L"firstbithigh");
  AddDefFunc(L"firstbitlow");
  AddDefFunc(L"floor");
  AddDefFunc(L"fmod");
  AddDefFunc(L"frac");
  AddDefFunc(L"frexp");
  AddDefFunc(L"fwidth");
  AddDefFunc(L"GetRenderTargetSampleCount");
  AddDefFunc(L"GetRenderTargetSamplePosition");
  AddDefFunc(L"GroupMemoryBarrier");
  AddDefFunc(L"GroupMemoryBarrierWithGroupSync");
  AddDefFunc(L"InterlockedAdd");
  AddDefFunc(L"InterlockedAnd");
  AddDefFunc(L"InterlockedCompareExchange");
  AddDefFunc(L"InterlockedCompareStore");
  AddDefFunc(L"InterlockedExchange");
  AddDefFunc(L"InterlockedMax");
  AddDefFunc(L"InterlockedMin");
  AddDefFunc(L"InterlockedOr");
  AddDefFunc(L"InterlockedXor");
  AddDefFunc(L"isfinite");
  AddDefFunc(L"isinf");
  AddDefFunc(L"isnan");
  AddDefFunc(L"ldexp");
  AddDefFunc(L"length");
  AddDefFunc(L"lerp");
  AddDefFunc(L"lit");
  AddDefFunc(L"log");
  AddDefFunc(L"log2");
  AddDefFunc(L"log10");
  AddDefFunc(L"Mad");
  AddDefFunc(L"max");
  AddDefFunc(L"min");
  AddDefFunc(L"modf");
  AddDefFunc(L"mul");
  AddDefFunc(L"noise");
  AddDefFunc(L"normalize");
  AddDefFunc(L"pow");
  AddDefFunc(L"printf");
  AddDefFunc(L"Process2DQuadTessFactorsAvg");
  AddDefFunc(L"Process2DQuadTessFactorsMax");
  AddDefFunc(L"Process2DQuadTessFactorsMin");
  AddDefFunc(L"ProcessIsolineTessFactors");
  AddDefFunc(L"ProcessQuadTessFactorsAvg");
  AddDefFunc(L"ProcessQuadTessFactorsMax");
  AddDefFunc(L"ProcessQuadTessFactorsMin");
  AddDefFunc(L"ProcessTriTessFactorsAvg");
  AddDefFunc(L"ProcessTriTessFactorsMax");
  AddDefFunc(L"ProcessTriTessFactorsMin");
  AddDefFunc(L"radians");
  AddDefFunc(L"Rcp");
  AddDefFunc(L"reflect");
  AddDefFunc(L"refract");
  AddDefFunc(L"reversebits");
  AddDefFunc(L"round");
  AddDefFunc(L"rsqrt");
  AddDefFunc(L"saturate");
  AddDefFunc(L"sign");
  AddDefFunc(L"sin");
  AddDefFunc(L"sincos");
  AddDefFunc(L"sinh");
  AddDefFunc(L"smoothstep");
  AddDefFunc(L"sqrt");
  AddDefFunc(L"step");
  AddDefFunc(L"tan");
  AddDefFunc(L"tanh");
  AddDefFunc(L"tex1D");
  AddDefFunc(L"tex1Dbias");
  AddDefFunc(L"tex1Dgrad");
  AddDefFunc(L"tex1Dlod");
  AddDefFunc(L"tex1Dproj");
  AddDefFunc(L"tex2D");
  AddDefFunc(L"tex2Dbias");
  AddDefFunc(L"tex2Dgrad");
  AddDefFunc(L"tex2Dlod");
  AddDefFunc(L"tex2Dproj");
  AddDefFunc(L"tex3D");
  AddDefFunc(L"tex3Dbias");
  AddDefFunc(L"tex3Dgrad");
  AddDefFunc(L"tex3Dlod");
  AddDefFunc(L"tex3Dproj");
  AddDefFunc(L"texCUBE");
  AddDefFunc(L"texCUBEbias");
  AddDefFunc(L"texCUBEgrad");
  AddDefFunc(L"texCUBElod");
  AddDefFunc(L"texCUBEproj");
  AddDefFunc(L"transpose");
  AddDefFunc(L"trunc");
}


ACDoc::~ACDoc()
{
  delete Pool;
  sDeleteAll(DefaultTypes);
  sDeleteAll(Intrinsics);
  sDeleteAll(Permutes);

  sDeleteAll(AllVariables);
  sDeleteAll(UserTypes);
  sDeleteAll(Functions);
}

sBool ACDoc::Parse(const sChar *source)
{
  Scan.Start(source);

  sDeleteAll(Functions);
  Variables.Clear();
  Program.Clear();
  UsePermute = 0;
  Function = 0;

  _Global();

  return Scan.Errors==0;
}

void ACDoc::Void()
{
  sDeleteAll(Functions);
  Variables.Clear();
  Program.Clear();
  UsePermute = 0;
  Function = 0;

  // remove structs, but not cbuffers or arrays

  ACType *t;
  sFORALL(UserTypes,t)
    t->Temp = (t->Type!=ACT_CBUFFER && t->Type!=ACT_ARRAY);
  sDeleteTrue(UserTypes,&ACType::Temp);
}

/****************************************************************************/


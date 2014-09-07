/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "altona2/libs/base/graphics.hpp"
#include "altona2/tools/AscOld/asc.hpp"
#include "altona2/libs/util/scanner.hpp"

using namespace AltonaShaderLanguage;


/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

wMaterialOut::wMaterialOut(sInt permmax,sPoolString name)
{
  Name = name;
  PermMax = permmax;
  VS = new sShaderBinary*[PermMax];  sSetMem(VS,0,PermMax*sizeof(sShaderBinary *));
  HS = new sShaderBinary*[PermMax];  sSetMem(HS,0,PermMax*sizeof(sShaderBinary *));
  DS = new sShaderBinary*[PermMax];  sSetMem(DS,0,PermMax*sizeof(sShaderBinary *));
  GS = new sShaderBinary*[PermMax];  sSetMem(GS,0,PermMax*sizeof(sShaderBinary *));
  PS = new sShaderBinary*[PermMax];  sSetMem(PS,0,PermMax*sizeof(sShaderBinary *));
  CS = new sShaderBinary*[PermMax];  sSetMem(CS,0,PermMax*sizeof(sShaderBinary *));
}

wMaterialOut::~wMaterialOut()
{
  delete[] VS;
  delete[] HS;
  delete[] DS;
  delete[] GS;
  delete[] PS;
  delete[] CS;
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sInt Document::GetRegCount(wMember *mem)
{
  switch(mem->Type->Kind)
  {
  case TypeBase:
    return 1;
  case TypeArray:
    if(mem->Type->Base->Kind==TypeBase)
      return ConstInt(mem->Type->Count);
  case TypeVector:
    return 1;
  case TypeMatrix:
    if(mem->Flags & MemRowMajor)
      return mem->Type->Rows;
    else if(mem->Flags & MemColumnMajor)
      return mem->Type->Columns;
    else
      return mem->Type->Columns;
  default:
    Scan.Error("cant store type in registers");
    return 1;
  }
}

/****************************************************************************/

wScope *Document::MakeScope()
{
  wScope *x = Pool->Alloc<wScope>();
  sClear(*x);
  x->Childs.Init();
  x->Symbols.Init();

  return x;
}

wSymbol *Document::MakeSym(wSymbolKind kind,sPoolString name,wType *type)
{
  wSymbol *x = Pool->Alloc<wSymbol>();
  sClear(*x);
  x->Surrogate = "";

  x->Kind = kind;
  x->Name = name;
  x->Type = type;

  return x;
}

wLit *Document::MakeLit(wLitKind kind)
{
  wLit *x = Pool->Alloc<wLit>();
  sClear(*x);
  x->Members.Init();

  x->Kind = kind;

  return x;
}

wType *Document::MakeTypeBase(sInt token)
{
  wType *t = BaseTypes.FindMember(&wType::Token,token);
  if(!t)
  {
    t = MakeType(TypeBase,0);
    t->Token = token;
    t->Loc = Scan.LastLoc;
    BaseTypes.Add(t);
  }
  return t;
}


wType *Document::MakeType(wTypeKind kind,wType *base)
{
  wType *x = Pool->Alloc<wType>();
  sClear(*x);
  x->Members.Init();

  x->Kind = kind;
  x->Base = base;
  x->DebugName = "";

  return x;
}

wExpr *Document::MakeExpr(wExprKind kind,wExpr *a,wExpr *b,wExpr *c)
{
  wExpr *x = Pool->Alloc<wExpr>();
  sClear(*x);
  x->Args.Init();

  x->Scope = CScope;
  x->Kind = kind;
  x->a = a;
  x->b = b;
  x->c = c;
  x->Loc = Scan.LastLoc;

  return x;
}

wStmt *Document::MakeStmt(wStmtKind kind)
{
  wStmt *x = Pool->Alloc<wStmt>();
  sClear(*x);

  x->Kind = kind;
  x->Loc = Scan.LastLoc;
  x->Scope = CScope;

  return x;
}

void Document::PushScope()
{
  wScope *s = Pool->Alloc<wScope>();
  sClear(*s);
  s->Childs.Init();
  s->Symbols.Init();

  s->Parent = CScope;
  if(s->Parent)
    s->Parent->Childs.Add(s);
  CScope = s;
}

void Document::PopScope()
{
  CScope = CScope->Parent;
}

/****************************************************************************/

sInt Document::ConstInt(wExpr *e)
{
  wSymbol *sym;

  switch(e->Kind)
  {
  case ExprMul:         return ConstInt(e->a) *  ConstInt(e->b); break;
  case ExprDiv:         return ConstInt(e->a) /  ConstInt(e->b); break;
  case ExprMod:         return ConstInt(e->a) %  ConstInt(e->b); break;
  case ExprAdd:         return ConstInt(e->a) +  ConstInt(e->b); break;
  case ExprSub:         return ConstInt(e->a) -  ConstInt(e->b); break;
  case ExprShiftL:      return ConstInt(e->a) << ConstInt(e->b); break;
  case ExprShiftR:      return ConstInt(e->a) >> ConstInt(e->b); break;
  case ExprGT:          return ConstInt(e->a) >  ConstInt(e->b); break;
  case ExprLT:          return ConstInt(e->a) <  ConstInt(e->b); break;
  case ExprGE:          return ConstInt(e->a) >= ConstInt(e->b); break;
  case ExprLE:          return ConstInt(e->a) <= ConstInt(e->b); break;
  case ExprEQ:          return ConstInt(e->a) == ConstInt(e->b); break;
  case ExprNE:          return ConstInt(e->a) != ConstInt(e->b); break;
  case ExprAnd:         return ConstInt(e->a) &  ConstInt(e->b); break;
  case ExprOr:          return ConstInt(e->a) |  ConstInt(e->b); break;
  case ExprEor:         return ConstInt(e->a) ^  ConstInt(e->b); break;
  case ExprLAnd:        return ConstInt(e->a) && ConstInt(e->b); break;
  case ExprLOr:         return ConstInt(e->a) || ConstInt(e->b); break;

  case ExprNot:         return !ConstInt(e->a); break;
  case ExprNeg:         return -ConstInt(e->a); break;
  case ExprComplement:  return ~ConstInt(e->a); break;
  case ExprCond:        return ConstInt(e->c) ? ConstInt(e->b) : ConstInt(e->a); break;

  case ExprLit:
    if(e->Literal->Kind == LitInt)
      return e->Literal->Int;
    break;

  case ExprSymbol:
    sym = FindSymbol(e->Scope,e->Name);
    if(sym && sym->Kind==SymPermute)
      return (sym->Permute->Mask & Perm);
    break;
  case ExprMember:
    if(e->a->Kind==ExprSymbol)
    {
      sym = FindSymbol(e->a->Scope,e->a->Name);
      if(sym->Kind==SymPermute)
      {
        wPermuteOption *opt;
        sFORALL(sym->Permute->Options,opt)
        {
          if(opt->Name==e->Name)
            return opt->Value;
        }
      }
    }
    break;
  }

  Scan.Error(e->Loc,"could not evaluate expression as constant int");
  return 0;
}

void Document::AssignPermute()
{
  sInt bit = 0;
  wPermute *per;
  wPermuteOption *opt;

  sFORALL(Material->Permutes,per)
  {
    sInt n = per->Options.GetCount();
    if(n==0)
    {
      per->Shift = bit;
      per->Mask = (1<<bit);
      per->Count = 2;
      bit++;
    }
    else
    {
      sInt bits = sFindHigherPower(n);
      per->Shift = bit;
      per->Count = n;
      per->Mask = ((1<<bits)-1)<<bit;

      sInt i = 0;
      sFORALL(per->Options,opt)
      {
        opt->Value = (i++)<<bit;
      }
      bit += bits;
    }
  }

  MaxPerm = 1<<bit;
}

sBool Document::IsPermutationValid(sInt perm)
{
  wPermute *per;
  sFORALL(Material->Permutes,per)
  {
    sInt v = (perm & per->Mask)>>per->Shift;
    if(v>=per->Count)
      return 0;
  }
  return 1;
}


wSymbol *Document::FindSymbol(sPoolString name)
{
  return FindSymbol(CScope,name);
}

wSymbol *Document::FindSymbol(wScope *scope,sPoolString name)
{
  wSymbol *sym;
  while(scope)
  {
    sFORALL(scope->Symbols,sym)
    {
      if(sym->Name==name)
        return sym;
    }

    scope = scope->Parent;
  }
  return 0;
}

sBool Document::IsSymbolUnique(sPoolString name)
{
  wSymbol *sym;

  sFORALL(CScope->Symbols,sym)
  {
    if(sym->Name==name)
      return 0;
  }

  return 1;
}

void Document::ClearObjectSymbols(wScope *scope)
{
  wScope *c;
  sFORALL(scope->Childs,c)
    ClearObjectSymbols(c);

  wSymbol *node;
  node = scope->Symbols.GetFirst();
  scope->Symbols.Init();
  while(node)
  {
    wSymbol *next = node->Next;
    if(node->Kind!=SymObject)
      scope->Symbols.Add(node);
    node = next;
  }
}

wType *Document::FuncType(const sChar *name)
{
  wSymbol *sym = FindSymbol(name);
  if(sym && sym->Kind==SymType)
    return sym->Type;
  if(sCmpString(name,"bool")==0) return MakeTypeBase(TokBool);
  if(sCmpString(name,"int")==0) return MakeTypeBase(TokFloat);
  if(sCmpString(name,"uint")==0) return MakeTypeBase(TokUInt);
  if(sCmpString(name,"half")==0) return MakeTypeBase(TokHalf);
  if(sCmpString(name,"float")==0) return MakeTypeBase(TokFloat);
  if(sCmpString(name,"double")==0) return MakeTypeBase(TokDouble);
  if(sCmpString(name,"Texture1D")==0) return MakeTypeBase(TokTexture1D);
  if(sCmpString(name,"Texture2D")==0) return MakeTypeBase(TokTexture2D);
  if(sCmpString(name,"Texture2DMS")==0) return MakeTypeBase(TokTexture2DMS);
  if(sCmpString(name,"Texture3D")==0) return MakeTypeBase(TokTexture3D);
  if(sCmpString(name,"TextureCube")==0) return MakeTypeBase(TokTextureCube);
  if(sCmpString(name,"Texture1DArray")==0) return MakeTypeBase(TokTexture1DArray);
  if(sCmpString(name,"Texture2DArray")==0) return MakeTypeBase(TokTexture2DArray);
  if(sCmpString(name,"Texture2DMSArray")==0) return MakeTypeBase(TokTexture2DMSArray);
  if(sCmpString(name,"TextureCubeArray")==0) return MakeTypeBase(TokTextureCubeArray);

  if(sCmpString(name,"Sampler")==0) return MakeTypeBase(TokSampler);
  if(sCmpString(name,"Sampler1D")==0) return MakeTypeBase(TokSampler1D);
  if(sCmpString(name,"Sampler2D")==0) return MakeTypeBase(TokSampler2D);
  if(sCmpString(name,"Sampler3D")==0) return MakeTypeBase(TokSampler3D);
  if(sCmpString(name,"SamplerCube")==0) return MakeTypeBase(TokSamplerCube);
  if(sCmpString(name,"SamplerState")==0) return MakeTypeBase(TokSamplerState);
  if(sCmpString(name,"SamplerComparisonState")==0) return MakeTypeBase(TokSamplercomparisonState);

  sASSERT0();
  return 0;
}

wIntrinsic *Document::Func(const sChar *result,const sChar *cl,sPoolString name,const sChar *arg0,const sChar *arg1,const sChar *arg2,const sChar *arg3)
{
//  sDPrintF("%s %s(%s,%s,%s)\n",result,name,arg0,arg1,arg2,arg3);
  wIntrinsic *in = Pool->Alloc<wIntrinsic>();
  sClear(*in);

  in->Result = FuncType(result);
  if(cl)
    in->Class = FuncType(cl);
  in->Name = name;
  in->ArgCount = 0;
  if(arg0)
  {
    in->Arg[0] = FuncType(arg0);
    in->ArgCount = 1;
    if(arg1)
    {
      in->Arg[1] = FuncType(arg1);
      in->ArgCount = 2;
      if(arg2)
      {
        in->Arg[2] = FuncType(arg2);
        in->ArgCount = 3;
        if(arg3)
        {
          in->Arg[3] = FuncType(arg3);
          in->ArgCount = 4;
        }
      }
    }
  }

  Intrinsics.Add(in);
  return in;
}

wType *Document::GetLiteralType(wLit *lit)
{
  switch(lit->Kind)
  {
  case LitInt:
    return MakeTypeBase(TokInt);
  case LitBool:
    return MakeTypeBase(TokBool);
  case LitFloat:
    return MakeTypeBase(TokFloat);
  default:
    sASSERT0();
    return ErrorType;
  }
}

sBool Document::Same(const sScannerLoc &loc,wType *a,wType *b)
{
  if(a==b) return 1;
  if(a->Kind==TypeError || b->Kind==TypeError) return 1;

  if(a->Kind==TypeBase && b->Kind==TypeVector && b->Columns==1 && b->Base->Kind==TypeBase && a->Token==b->Base->Token)
    return 1;
  if(b->Kind==TypeBase && a->Kind==TypeVector && a->Columns==1 && a->Base->Kind==TypeBase && b->Token==a->Base->Token)
    return 1;

  if(a->Kind!=b->Kind) return 0;

  switch(a->Kind)
  {
  case TypeArray:
    if(ConstInt(a->Count)==ConstInt(b->Count))
      return 1;
    break;
  case TypeVector:
    if(a->Columns==b->Columns && a->Rows==b->Rows)
      return 1;
    break;
  case TypeMatrix:
    if(a->Columns==b->Columns && a->Rows==b->Rows)
      return 1;
    break;
  }
  return 0;
}


wType *Document::Compatible(const sScannerLoc &loc,wType *a,wType *b,sBool noerror)
{
  if(Same(loc,a,b))
    return a;

  if(!noerror)
    Scan.Error(loc,"types not compatible");
    
  return 0;
}

sInt GetBaseKind(wType *t)
{
  if(t->Kind==TypeBase)
    return t->Token;
  if(t->Kind==TypeVector && t->Base->Kind==TypeBase)
    return t->Base->Token;
  if(t->Kind==TypeMatrix && t->Base->Kind==TypeBase)
    return t->Base->Token;
  return 0;
}

wType *Document::MakeCompatible(const sScannerLoc &loc,wExpr **exprptr,wType *b,sBool noerror)
{
  wExpr *expr = *exprptr;
  wType *a = expr->ResultType;
  sInt abase = GetBaseKind(a);

  sInt bbase = GetBaseKind(b);
  if(abase!=0 && bbase!=0 && abase!=bbase)
  {
    sBool ok = 0;
    if(abase==TokBool  && bbase==TokInt   ) ok = 1;
    if(abase==TokInt   && bbase==TokBool  ) ok = 1;
    if(abase==TokUInt  && bbase==TokInt   ) ok = 1;

    if(abase==TokInt   && bbase==TokFloat ) ok = 1;
    if(abase==TokUInt  && bbase==TokFloat ) ok = 1;
    if(abase==TokHalf  && bbase==TokFloat ) ok = 1;

    if(abase==TokInt   && bbase==TokDouble) ok = 1;
    if(abase==TokUInt  && bbase==TokDouble) ok = 1;
    if(abase==TokHalf  && bbase==TokDouble) ok = 1;
    if(abase==TokFloat && bbase==TokDouble) ok = 1;

    if(ok)
    {
      wType *nt = 0;
      if(b->Kind==TypeBase)
        nt = b;
      else if((b->Kind==TypeVector || b->Kind==TypeMatrix) && b->Base->Kind==TypeBase)
        nt = b->Base;
      else
        sASSERT0();

      if(a->Kind==TypeBase)
      {
        // already ok
      }
      else if((a->Kind==TypeVector || a->Kind==TypeMatrix) && a->Base->Kind==TypeBase)
      {
        nt = MakeType(a->Kind,nt);
        nt->Columns = a->Columns;
        nt->Rows = a->Rows;
      }
      else
      {
        sASSERT0();
      }

      wExpr *nx = MakeExpr(ExprCast);
      nx->Loc = expr->Loc;
      nx->Type = nt;
      nx->ResultType = nt;
      nx->Args.Add(expr);

      *exprptr = nx;

      // get new A

      expr = *exprptr;
      a = expr->ResultType;
      abase = GetBaseKind(a);
    }
  }


  if((a->Kind==TypeBase                    && b->Kind==TypeVector && b->Base->Kind==TypeBase && abase==bbase)
  || (a->Kind==TypeVector && a->Columns==1 && b->Kind==TypeVector && b->Base->Kind==TypeBase && abase==bbase)) 
  {
    if(b->Columns>1)   // scalar to vector expansion
    {
      if(Platform==sConfigRenderGL2 || Platform==sConfigRenderGLES2)
      {
        *exprptr = MakeExpr(ExprCast);
        (*exprptr)->Loc = expr->Loc;
        (*exprptr)->Type = b;
        (*exprptr)->ResultType = b;
        (*exprptr)->Args.Add(expr);
      }
      // hlsl has automatic conversion.
    }

    // get new A

    expr = *exprptr;
    a = b;
    abase = GetBaseKind(a);
  }      

  if(Same(loc,a,b))
    return b;

  if(!noerror)
    Scan.Error(loc,"types not compatible");

  return 0;
}

void Document::WriteAsm()
{
  sShaderBinary *bin;
  wMaterialOut *mo;
  sString<128> HashString;
  const sChar *prefix = "";
  if(sConfigPlatform==sConfigPlatformWin || sConfigPlatform==sConfigPlatformOSX || sConfigPlatform==sConfigPlatformIOS)
    prefix = "_";

  sFORALL(AllShaders,bin)
  {
    Asm.PrintF("_Shader_%x:",bin->Hash);
    const sU8 *data = bin->Data;
    const sU8 *end = data+bin->Size;
    sInt n = 0;
    while(end-data>3)
    {
      if((n&31)==0)
        Asm.Print("\n        dd      ");
      else
        Asm.Print(",");
      Asm.PrintF("0x%08x",*(sU32 *)data);
      n+=4;
      data+=4;
    }
    if(end-data>0)
    {
      Asm.PrintF("\n        db      0x%02x",*data++);
      for(sInt i=0;i<3;i++)
      {
        if(data<end)
          Asm.PrintF(",0x%02x",*data++);
        else
          Asm.PrintF(",0x%02x",0xaa);
      }
    }
    Asm.Print("\n\n");

    Cpp.PrintF("unsigned char _Shader_%x[] = {",bin->Hash);
    data = bin->Data;
    n = 0;
    while(data<end)
    {
      if(((n++)&31)==0)
        Cpp.Print("\n  ");
      Cpp.PrintF("%3d,",*data++);
    }
    Cpp.Print("\n\n};\n");


  }
  sFORALL(MtrlOuts,mo)
  {
    Asm.PrintF("        global  %s%sArray\n",prefix,mo->Name);
    Asm.PrintF("%s%sArray:\n",prefix,mo->Name);
    for(sInt i=0;i<mo->PermMax;i++)
    {
      if(mo->VS[i])
        Asm.PrintF("        dd        _Shader_%x\n",mo->VS[i]->Hash);
      else
        Asm.Print("        dd        0\n");
      if(mo->HS[i])
        Asm.PrintF("        dd        _Shader_%x\n",mo->HS[i]->Hash);
      else
        Asm.Print("        dd        0\n");
      if(mo->DS[i])
        Asm.PrintF("        dd        _Shader_%x\n",mo->DS[i]->Hash);
      else
        Asm.Print("        dd        0\n");
      if(mo->GS[i])
        Asm.PrintF("        dd        _Shader_%x\n",mo->GS[i]->Hash);
      else
        Asm.Print("        dd        0\n");
      if(mo->PS[i])
        Asm.PrintF("        dd        _Shader_%x\n",mo->PS[i]->Hash);
      else
        Asm.Print("        dd        0\n");
      if(mo->CS[i])
        Asm.PrintF("        dd        _Shader_%x\n",mo->CS[i]->Hash);
      else
        Asm.Print("        dd        0\n");

      Asm.PrintF("        dd        0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x\n",
        mo->VS[i] ? mo->VS[i]->Size : 0,
        mo->HS[i] ? mo->HS[i]->Size : 0,
        mo->DS[i] ? mo->DS[i]->Size : 0,
        mo->GS[i] ? mo->GS[i]->Size : 0,
        mo->PS[i] ? mo->PS[i]->Size : 0,
        mo->CS[i] ? mo->CS[i]->Size : 0);
    }
    Asm.Print("\n");
    
    Asm.PrintF("        global  %s%s\n",prefix,mo->Name);
    Asm.PrintF("%s%s:\n",prefix,mo->Name);
    Asm.PrintF("        dd      0x%x\n",mo->PermMax);
    Asm.PrintF("        dd      %s%sArray\n",prefix,mo->Name);
    Asm.Print("\n\n");

    Hpp.PrintF("extern \"C\" sAllShaderPermPara %s;\n",mo->Name);

    Cpp.PrintF("#include \"altona2/libs/base/base.hpp\"\n");
    Cpp.PrintF("extern \"C\" sAllShaderPara %s_Array[];\n",mo->Name);
    Cpp.PrintF("extern \"C\" sAllShaderPermPara %s;\n",mo->Name);
    Cpp.PrintF("sAllShaderPara %s_Array[] = {\n",mo->Name);
    for(sInt i=0;i<mo->PermMax;i++)
    {
      Cpp.PrintF("  {\n");
      Cpp.PrintF("    {\n");
      if(mo->VS[i])
        Cpp.PrintF("      _Shader_%x,\n",mo->VS[i]->Hash);
      else
        Cpp.Print("      0,\n");
      if(mo->HS[i])
        Cpp.PrintF("      _Shader_%x,\n",mo->HS[i]->Hash);
      else
        Cpp.Print("      0,\n");
      if(mo->DS[i])
        Cpp.PrintF("      _Shader_%x,\n",mo->DS[i]->Hash);
      else
        Cpp.Print("      0,\n");
      if(mo->GS[i])
        Cpp.PrintF("      _Shader_%x,\n",mo->GS[i]->Hash);
      else
        Cpp.Print("      0,\n");
      if(mo->PS[i])
        Cpp.PrintF("      _Shader_%x,\n",mo->PS[i]->Hash);
      else
        Cpp.Print("      0,\n");
      if(mo->CS[i])
        Cpp.PrintF("      _Shader_%x,\n",mo->CS[i]->Hash);
      else
        Cpp.Print("      0,\n");
      Cpp.Print("    },\n");

      Cpp.PrintF("    { 0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x },\n",
        mo->VS[i] ? mo->VS[i]->Size : 0,
        mo->HS[i] ? mo->HS[i]->Size : 0,
        mo->DS[i] ? mo->DS[i]->Size : 0,
        mo->GS[i] ? mo->GS[i]->Size : 0,
        mo->PS[i] ? mo->PS[i]->Size : 0,
        mo->CS[i] ? mo->CS[i]->Size : 0);
      Cpp.PrintF("  },\n");
    }
    Cpp.PrintF("};\n\n");
    Cpp.PrintF("sAllShaderPermPara %s = {\n",mo->Name);
    Cpp.PrintF("  %d,%s_Array\n",mo->PermMax,mo->Name);
    Cpp.PrintF("};\n\n");
  }
  Hpp.Print("\n");
  Hpp.Print("/****************************************************************************/\n");
  Hpp.Print("\n");
  Hpp.Print("#endif\n");
  Hpp.Print("\n");
}

/****************************************************************************/

void Document::_MtrlMakeTypes(wType *base,const sChar *name)
{
  sString<64> buffer;
  wType *t;
  for(sInt c=1;c<=4;c++)
  {
    buffer.PrintF("%s%d",name,c);
    t = MakeType(TypeVector,base);
    t->Columns = c;
    t->DebugName = buffer.Get();
    CScope->Add(MakeSym(SymType,buffer.Get(),t));
    for(sInt r=1;r<=4;r++)
    {
      buffer.PrintF("%s%dx%d",name,r,c);
      t = MakeType(TypeMatrix,base);
      t->Columns = c;
      t->Rows = r;
      t->DebugName = buffer.Get();
      CScope->Add(MakeSym(SymType,buffer.Get(),t));
    }
  } 
}


void Document::_Mtrl()
{
  // reset

  CScope = 0;
  GScope = 0;
  Material = 0;
  PifCount = 0;
  Pool->Reset();
  BaseTypes.Clear();
  Intrinsics.Clear();
  TempCount = 0;

  // setup

  PushScope();
  GScope = CScope;

  if(Platform==sConfigRenderGL2 || Platform==sConfigRenderGLES2)
  {
    _MtrlMakeTypes(MakeTypeBase(TokBool)  ,"bool");
    _MtrlMakeTypes(MakeTypeBase(TokInt)   ,"int");
    _MtrlMakeTypes(MakeTypeBase(TokFloat) ,"float");
  }
  else
  {
    _MtrlMakeTypes(MakeTypeBase(TokBool)  ,"bool");
    _MtrlMakeTypes(MakeTypeBase(TokInt)   ,"int");
    _MtrlMakeTypes(MakeTypeBase(TokUInt)  ,"uint");
    _MtrlMakeTypes(MakeTypeBase(TokHalf)  ,"half");
    _MtrlMakeTypes(MakeTypeBase(TokFloat) ,"float");
    _MtrlMakeTypes(MakeTypeBase(TokDouble),"double");
  }

  ErrorType = MakeType(TypeError,0);

  PushScope();

  Material = Pool->Alloc<wMtrl>();
  sClear(*Material);
  Material->Permutes.Init();
  Material->Invariants.Init();
  Material->CBuffers.Init();

  // intrinsics

  wIntrinsic *f;
  const sChar *bt = "float";
  sString<64> r,a,b,m;

  for(sInt i=0;i<=4;i++)
  {
    if(i>=1)
    {
      r.PrintF("%s%d",bt,i);
      for(sInt j=1;j<=4;j++)
      {
        b.PrintF("%s%d",bt,j);
        m.PrintF("%s%dx%d",bt,i,j);
        Func(r,0,"mul",m,b)->HandleGL2="m";

        b.PrintF("%s%d",bt,j);
        m.PrintF("%s%dx%d",bt,j,i);
        Func(r,0,"mul",b,m)->HandleGL2="m";

        for(sInt k=1;k<=4;k++)
        {
          m.PrintF("%s%dx%d",bt,i,k);
          a.PrintF("%s%dx%d",bt,k,j);
          b.PrintF("%s%dx%d",bt,i,j);
          Func(m,0,"mul",a,b)->HandleGL2="m";
        }
      }
    }

    if(i==0)
      r.PrintF("%s",bt);
    else
      r.PrintF("%s%d",bt,i);
    b = a = r;

    Func(r,0,"normalize",a);      // vector math operators
    Func(bt,0,"length",a);
    Func(r,0,"reflect",a,a);
    Func(r,0,"refract",a,a);
    Func(r,0,"cross",a);

    Func(bt,0,"dot",a,a);         // simple math operators
    Func(r,0,"lerp",a,a,a);
    Func(r,0,"mad",a,a,a);
    Func(r,0,"min",a,a);
    Func(r,0,"max",a,a);
    Func(r,0,"saturate",a)->HandleGL2="s";
    Func(r,0,"sign",a);
    Func(r,0,"step",a);
    Func(r,0,"abs",a);
    Func(r,0,"smoothstep",a,a,a);

    Func(r,0,"sin",a);            // transcendental
    Func(r,0,"cos",a);
    Func(r,0,"tan",a);
    Func(r,0,"atan",a);
    Func(r,0,"atan2",a,a);
//    Func("void",0,"sincos",a,a,a);
    Func(r,0,"sinh",a);
    Func(r,0,"cosh",a);
    Func(r,0,"tanh",a);

    Func(r,0,"fmod",a,a);         // special math operators
    Func(r,0,"frac",a);
    Func(r,0,"floor",a);
    Func(r,0,"ceil",a);
    Func(r,0,"exp",a);
    Func(r,0,"exp2",a);
    Func(r,0,"log",a);
    Func(r,0,"log2",a);
    Func(r,0,"log10",a);
    Func(r,0,"pow",a,a);
    Func(r,0,"rcp",a);
    Func(r,0,"rsqrt",a);
    Func(r,0,"sqrt",a);

    Func(r,0,"ddx",a);            // derivatives
    Func(r,0,"ddx_coarse",a);
    Func(r,0,"ddx_fine",a);
    Func(r,0,"ddy",a);
    Func(r,0,"ddy_coarse",a);
    Func(r,0,"ddy_fine",a);
    Func(r,0,"fwidth",a);
/*
  f = Func("float4","sampler2d","sample","float2");
    f->HandleDX9 = "1tex2D";
    f->HandleDX11 = "2";
    */
  }

  bt = r = a = "float";

  Func(bt,0,"dot",a,a);           // simle math operators
  Func(r,0,"lerp",a,a,a);
  Func(r,0,"mad",a,a,a);
  Func(r,0,"min",a,a);
  Func(r,0,"max",a,a);
  Func(r,0,"saturate",a);
  Func(r,0,"sign",a);
  Func(r,0,"step",a);
  Func(r,0,"abs",a);
  Func(r,0,"smoothstep",a,a,a);

  Func(r,0,"sin",a);              // transcendental
  Func(r,0,"cos",a);
  Func(r,0,"tan",a);
  Func(r,0,"atan",a);
  Func(r,0,"atan2",a,a);
//    Func("void",0,"sincos",a,a,a);
  Func(r,0,"sinh",a);
  Func(r,0,"cosh",a);
  Func(r,0,"tanh",a);

  Func(r,0,"fmod",a,a);           // Special math operators
  Func(r,0,"frac",a);
  Func(r,0,"floor",a);
  Func(r,0,"ceil",a);
  Func(r,0,"exp",a);
  Func(r,0,"exp2",a);
  Func(r,0,"log",a);
  Func(r,0,"log2",a);
  Func(r,0,"log10",a);
  Func(r,0,"pow",a,a);
  Func(r,0,"rcp",a);
  Func(r,0,"rsqrt",a);
  Func(r,0,"sqrt",a);

  Func(r,0,"ddx",a);              // derivatives
  Func(r,0,"ddx_coarse",a);
  Func(r,0,"ddx_fine",a);
  Func(r,0,"ddy",a);
  Func(r,0,"ddy_coarse",a);
  Func(r,0,"ddy_fine",a);
  Func(r,0,"fwidth",a);

  for(sInt i=0;i<4;i++)
  {
    const sChar *sampler;
    const sChar *coord;
    switch(i)
    {
    case 0:
      sampler = "Texture1D";
      coord = "float";
      break;
    case 1:
      sampler = "Texture1DArray";
      coord = "float2";
      break;
    case 2:
      sampler = "Texture2D";
      coord = "float2";
      break;
    case 3:
      sampler = "Texture2DArray";
      coord = "float3";
      break;
    case 4:
      sampler = "Texture3D";
      coord = "float3";
      break;
    case 5:
      sampler = "TextureCube";
      coord = "float3";
      break;
    case 6:
      sampler = "TextureCubeArray";
      coord = "float3";
      break;
    }
    f = Func("float4",sampler,"Sample","SamplerState",coord);
    if(i==2)
    {
      f->HandleDX9 = "1tex2D";
      f->HandleGL2 = "2texture2D";
    }
    f = Func("float4",sampler,"SampleBias","SamplerState",coord,"float");
    if(i!=4)
    {
      f = Func("float4",sampler,"SampleCmp","SamplerState",coord,"float");
      f = Func("float4",sampler,"SampleCmpLevelZero","SamplerState",coord,"float");
    }
    f = Func("float4",sampler,"SampleGrad","SamplerState",coord,coord,coord);
    f = Func("float4",sampler,"SampleLevel","SamplerState",coord,"float");
  }

    
    // parse

  Scan.ScanName(Material->Name);
  _MtrlBlock(0);
  AssignPermute();

  // do something

  if(Flags & 1)
  {
    DumpMtrl();
  }

  // output shaders

  wMaterialOut *mo = new wMaterialOut(MaxPerm,Material->Name);
  MtrlOuts.AddTail(mo);

  for(Perm = 0; Perm<MaxPerm; Perm++)
  {
    if(!IsPermutationValid(Perm))
      continue;
    if(Material->Shaders[sST_Vertex])
      mo->VS[Perm] = OutShader(sST_Vertex,Material->Shaders[sST_Vertex]);
    if(Material->Shaders[sST_Geometry])
      mo->GS[Perm] = OutShader(sST_Geometry,Material->Shaders[sST_Geometry]);
    if(Material->Shaders[sST_Pixel])
      mo->PS[Perm] = OutShader(sST_Pixel,Material->Shaders[sST_Pixel]);
  }

  // output header

  wCBuffer *cb;
  sFORALL(Material->CBuffers,cb)
  {
    wMember *mem;

    Hpp.PrintF("struct %s_%s\n",Material->Name,cb->Name);
    Hpp.PrintF("{\n");
    sInt regscalar = 0;
    sFORALL(cb->Members,mem)
    {
      const sChar *tname = "";
      wType *type = mem->Type;
      wType *base = type;
      
      
      if(base->Kind==TypeVector || base->Kind==TypeMatrix || base->Kind==TypeArray)
        base = type->Base;
      if(base->Kind==TypeBase && !(mem->Name[0]=='_' && mem->Name[1]=='_'))
      {
        // pad to start of new member
        while(regscalar<mem->AssignedScalar)
        {
          Hpp.PrintF("  sF32 __%04dtemp;\n",TempCount++);
          regscalar ++;
        }
        sASSERT(regscalar==mem->AssignedScalar)

        // sanitze type

        switch(base->Token)
        {
        case TokFloat: if(type->Kind==TypeBase) tname = "sF32"; else tname = "sVector"; break;
        case TokInt: tname = "sS32"; break;
        case TokUInt: tname = "sU32"; break;
        default: tname = "sInt"; break;
        }
        
        // output scalar / vector / matrix
        
        if(mem->Type->Kind==TypeBase)
        {
          Hpp.PrintF("  %s %s;\n",tname,mem->Name);
          regscalar++;
        }
        else if(mem->Type->Kind==TypeVector)
        {
          sASSERT(mem->Type->Columns+(regscalar%4)<=4)
          Hpp.PrintF("  %s%d %s;\n",tname,mem->Type->Columns,mem->Name);
          regscalar += mem->Type->Columns;
        }
        else if(mem->Type->Kind==TypeMatrix)
        {
          sASSERT((regscalar%4)==0)
          if(mem->Type->Columns==4 && mem->Type->Rows==4)
          {
            Hpp.PrintF("  sMatrix44 %s;\n",mem->Name);
          }
          else if(mem->Type->Columns==4 && mem->Type->Rows==3)
          {
            Hpp.PrintF("  sMatrix44A %s;\n",mem->Name);
          }
          else
          {
            for(sInt i=0;i<mem->Type->Rows;i++)
            {
              Hpp.PrintF("  %s%d %s_%c;\n",tname,mem->Type->Columns,mem->Name,'i'+i);
              regscalar += mem->Type->Columns;
              while(regscalar%4)
              {
                Hpp.PrintF("  sF32 __%04dtemp;\n",TempCount++);
                regscalar++;
              }
            }
          }
          regscalar += mem->Type->Rows * 4;
        }
      }
    }
    
    // pad to full register
    
    while(regscalar%4)
    {
      Hpp.PrintF("  sF32 __%04dtemp;\n",TempCount++);
      regscalar++;
    }
    Hpp.PrintF("};\n");
  }


  wPermute *per;
  if(Material->Permutes.GetCount())
  {
    Hpp.PrintF("enum %s_PermuteEnum\n",Material->Name);
    Hpp.PrintF("{\n");
    sFORALL(Material->Permutes,per)
    {
      wPermuteOption *opt;
      Hpp.PrintF("  %s_%s = 0x%08x,\n",Material->Name,per->Name,per->Mask);
      sFORALL(per->Options,opt)
        Hpp.PrintF("  %s_%s_%s = 0x%08x,\n",Material->Name,per->Name,opt->Name,opt->Value);
    }
    Hpp.PrintF("};\n");
  }
  Hpp.Print("\n");

  // reset again

  PopScope();
  sASSERT(GScope==CScope);
  PopScope();
  sASSERT(CScope == 0);

  CScope = 0;
  GScope = 0;
  Material = 0;
  Pool->Reset();
}

void Document::_Global()
{
  while(!Scan.Errors && Scan.Token!=sTOK_End)
  {
    if(Scan.IfToken(TokMtrl))
    {
      _Mtrl();
    }
    else
    {
      Scan.Error("syntax");
    }
  }
}

/****************************************************************************/

Document::Document()
{
  Pool = new sMemoryPool();
}

Document::~Document()
{
  delete Pool;
  AllShaders.DeleteAll();
  MtrlOuts.DeleteAll();
}

sBool Document::Parse(const sChar *filename,sInt platform,sInt flags)
{
  Flags = flags;
  Platform = platform;
  Pool->Reset();
  Scan.Init(sSF_CppComment|sSF_EscapeCodes|sSF_MergeStrings|sSF_Cpp);
  Scan.AddDefaultTokens();

  // asc tokens

  Scan.AddToken("mtrl",TokMtrl);
  Scan.AddToken("permute",TokPermute);
  Scan.AddToken("pif",TokPif);
  Scan.AddToken("pelse",TokPelse);
  Scan.AddToken("vs",TokVS);
  Scan.AddToken("hs",TokHS);
  Scan.AddToken("ds",TokDS);
  Scan.AddToken("gs",TokGS);
  Scan.AddToken("ps",TokPS);
  Scan.AddToken("cs",TokCS);
  Scan.AddToken("ia2vs",TokIA2VS);
  Scan.AddToken("vs2rs",TokVS2RS);
  Scan.AddToken("vs2ps",TokVS2PS);
  Scan.AddToken("ps2om",TokPS2OM);
  Scan.AddToken("vs2gs",TokVS2GS);
  Scan.AddToken("gs2ps",TokGS2PS);
  Scan.AddToken("gs2rs",TokGS2RS);

  // hlsl tokens (without effects)

  Scan.AddToken("++",TokInc);
  Scan.AddToken("--",TokDec);
  Scan.AddToken("+=",TokAssignAdd);
  Scan.AddToken("-=",TokAssignSub);
  Scan.AddToken("*=",TokAssignMul);
  Scan.AddToken("/=",TokAssignDiv);
  Scan.AddToken("%=",TokAssignMod);
  Scan.AddToken("&&",TokLAnd);
  Scan.AddToken("||",TokLOr);

  Scan.AddToken("bool",TokBool);
  Scan.AddToken("break",TokBreak);
  Scan.AddToken("buffer",TokBuffer);
  Scan.AddToken("cbuffer",TokCBuffer);
  Scan.AddToken("class",TokClass);
  Scan.AddToken("column_major",TokColumnMajor);
  Scan.AddToken("const",TokConst);
  Scan.AddToken("continue",TokContinue);
  Scan.AddToken("discard",TokDiscard);
  Scan.AddToken("do",TokDo);
  Scan.AddToken("double",TokDouble);
  Scan.AddToken("else",TokElse);
  Scan.AddToken("extern",TokExtern);
  Scan.AddToken("false",TokFalse);
  Scan.AddToken("float",TokFloat);
  Scan.AddToken("for",TokFor);
  Scan.AddToken("half",TokHalf);
  Scan.AddToken("if",TokIf);
  Scan.AddToken("in",TokIn);
  Scan.AddToken("inout",TokInout);
  Scan.AddToken("int",TokInt);
  Scan.AddToken("matrix",TokMatrix);
  Scan.AddToken("namespace",TokNamespace);
  Scan.AddToken("nointerpolation",TokNointerpolation);
  Scan.AddToken("out",TokOut);
  Scan.AddToken("pass",TokPass);
  Scan.AddToken("precise",TokPrecise);
  Scan.AddToken("return",TokReturn);
  Scan.AddToken("register",TokRegister);
  Scan.AddToken("row_major",TokRowMajor);
  Scan.AddToken("Sampler",TokSampler);
  Scan.AddToken("Sampler1D",TokSampler1D);
  Scan.AddToken("Sampler2D",TokSampler2D);
  Scan.AddToken("Sampler3D",TokSampler3D);
  Scan.AddToken("SamplerCube",TokSamplerCube);
  Scan.AddToken("SamplerState",TokSamplerState);
  Scan.AddToken("SamplerComparisonState",TokSamplercomparisonState);
  Scan.AddToken("slot",TokSlot);
  Scan.AddToken("static",TokStatic);
  Scan.AddToken("struct",TokStruct);
  Scan.AddToken("switch",TokSwitch);
  Scan.AddToken("Texture",TokTexture);
  Scan.AddToken("Texture1D",TokTexture1D);
  Scan.AddToken("Texture2D",TokTexture2D);
  Scan.AddToken("Texture3D",TokTexture3D);
  Scan.AddToken("Texture2DMS",TokTexture2DMS);
  Scan.AddToken("TextureCube",TokTextureCube);
  Scan.AddToken("Texture1DArray",TokTexture1DArray);
  Scan.AddToken("texture2DArray",TokTexture2DArray);
  Scan.AddToken("Texture2DMSArray",TokTexture2DMSArray);
  Scan.AddToken("TextureCubeArray",TokTextureCubeArray);
  Scan.AddToken("true",TokTrue);
  Scan.AddToken("typedef",TokTypedef);
  Scan.AddToken("uint",TokUInt);
  Scan.AddToken("uniform",TokUniform);
  Scan.AddToken("vector",TokVector);
  Scan.AddToken("void",TokVoid);
  Scan.AddToken("volatile",TokVolatile);
  Scan.AddToken("while",TokWhile);

  Scan.AddToken("lowp",TokLowP);
  Scan.AddToken("mediump",TokMediumP);
  Scan.AddToken("highp",TokHighP);

  // header

  sString<sMaxPath> HeaderProtectName;
  {
    HeaderProtectName = filename;
    sPtr size=0;
    sU8 *data = sLoadFile(filename,size);
    if(data)
    {
      sChecksumMD5 md;
      md.Calc(data,size);
      HeaderProtectName.PrintF("%X",md);
    }
    delete[] data;
  }


  Hpp.Print("/****************************************************************************/\n");
  Hpp.Print("/***                                                                      ***/\n");
  Hpp.Print("/***   this file is automatically generated by asc.exe.                   ***/\n");
  Hpp.Print("/***   please do not edit.                                                ***/\n");
  Hpp.Print("/***                                                                      ***/\n");
  Hpp.Print("/****************************************************************************/\n");
  Hpp.Print("\n");
  Hpp.PrintF("#ifndef FILE_%s_HPP\n",HeaderProtectName);
  Hpp.PrintF("#define FILE_%s_HPP\n",HeaderProtectName);
  Hpp.Print("\n");
  Hpp.Print("#include \"altona2/libs/base/base.hpp\"\n");
  Hpp.Print("\n");
  Hpp.Print("/****************************************************************************/\n");
  Hpp.Print("\n");

  Asm.Print("        section .text\n");
  Asm.Print("        bits    32\n");
  Asm.PrintF("        align   4,db 0\n");
  Asm.Print("\n");

  // parse

  Scan.StartFile(filename);

  _Global();

  if(!Scan.Errors)
  {
    WriteAsm();
  }
  else
  {
    sDPrint(Scan.ErrorLog.Get());
  }
  return Scan.Errors==0;
}

/****************************************************************************/


/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "altona2/tools/AscOld/asc.hpp"
#include "altona2/libs/util/scanner.hpp"

using namespace AltonaShaderLanguage;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Document::_Struct()
{
  Scan.Error("struct not implemented");
}

wType *Document::_Type()
{
  wType *type = 0;
  wSymbol *sym = 0;
  sPoolString name;

  switch(Scan.Token)
  {
  case TokVector:
    Scan.Match(TokVector);
    Scan.Match('<');
    type = MakeType(TypeVector,_Type());
    Scan.Match(',');
    type->Columns = Scan.ScanInt();
    Scan.Match('>');
    break;

  case TokMatrix:
    Scan.Match(TokMatrix);
    Scan.Match('<');
    type = MakeType(TypeMatrix,_Type());
    Scan.Match(',');
    type->Rows = Scan.ScanInt();
    Scan.Match(',');
    type->Columns = Scan.ScanInt();
    Scan.Match('>');
    break;

  case TokBool:  
  case TokUInt:  
  case TokInt:   
  case TokHalf:  
  case TokFloat: 
  case TokDouble:

  case TokSampler:
  case TokSampler1D:
  case TokSampler2D:
  case TokSampler3D:
  case TokSamplerCube:
  case TokSamplerState:
  case TokSamplercomparisonState:
  case TokTexture:
  case TokTexture1D:
  case TokTexture1DArray:
  case TokTexture2D:
  case TokTexture2DArray:
  case TokTexture2DMS:
  case TokTexture2DMSArray:
  case TokTexture3D:
  case TokTextureCube:
  case TokTextureCubeArray:
  case TokBuffer:
    type = MakeTypeBase(Scan.Token);    
    Scan.Scan();
    break;

  case sTOK_Name:
    Scan.ScanName(name);
    sym = FindSymbol(name);
    type = MakeType(TypeError,0);
    if(sym==0)
      Scan.Error("unknown type %s",name);
    else if(sym->Kind!=SymType)
      Scan.Error("symbol %s is expected to be a type",name);
    else
      type = sym->Type;

    break;

  default:
    Scan.Error("type expected");
    type = MakeType(TypeError,0);
  }

  return type;
}

sInt Document::_MemberFlags()
{
  sInt f = 0;
  for(;;)
  {
    if(Scan.IfToken(TokExtern)) 
    {
      f |= MemExtern;
    }
    else if(Scan.IfToken(TokNointerpolation))
    {
      f |= MemNointerpolation;
    }
    else if(Scan.IfToken(TokPrecise))
    {
      f |= MemPrecise;
    }
    else if(Scan.IfToken(TokGroupShared))
    {
      f |= MemGroupShared;
    }
    else if(Scan.IfToken(TokStatic))
    {
      f |= MemStatic;
    }
    else if(Scan.IfToken(TokUniform))
    {
      f |= MemUniform;
    }
    else if(Scan.IfToken(TokVolatile))
    {
      f |= MemVolatile;
    }

    else if(Scan.IfToken(TokLowP))
    {
      f |= MemLowP;
    }
    else if(Scan.IfToken(TokMediumP))
    {
      f |= MemMediumP;
    }
    else if(Scan.IfToken(TokHighP))
    {
      f |= MemHighP;
    }
    else
    {
      break;
    }
  }
  return f;
}

wMember *Document::_Decl()
{
  wMember *mem = Pool->Alloc<wMember>();
  sClear(*mem);
  mem->Register = -1;
  mem->Semantics = "";
  mem->Surrogate = "";

  // storage class

  mem->Flags = _MemberFlags();

  // type modifier

  for(;;)
  {
    if(Scan.IfToken(TokConst)) 
    {
      mem->Flags |= MemConst;
    }
    else if(Scan.IfToken(TokColumnMajor))
    {
      mem->Flags |= MemColumnMajor;
    }
    else if(Scan.IfToken(TokRowMajor))
    {
      mem->Flags |= MemRowMajor;
    }
    else
    {
      break;
    }
  }

  mem->Type = _Type();

  Scan.ScanName(mem->Name);
  if(Scan.IfToken('['))
  {
    mem->Type = MakeType(TypeArray,mem->Type);
    mem->Type->Count = _Expr();
    Scan.Match(']');
  }
  while(Scan.IfToken(':'))
  {
    if(Scan.IfToken(TokRegister))
    {
      Scan.Match('(');
      mem->Register = Scan.ScanInt();
      Scan.Match(')');
    }
    else if(Scan.Token==sTOK_Name)
    {
      Scan.ScanName(mem->Semantics);
    }
    else
    {
      Scan.Error("syntax in semantics");
    }
  }
  if(Scan.IfToken('='))
  {
    mem->Initializer = _Expr();
  }
  Scan.Match(';');

  return mem;
}

void Document::_Args(wExpr *call)
{
  Scan.Match('(');
  while(!Scan.Errors)
  {
    if(Scan.IfToken(')')) break;
    call->Args.Add(_Expr());
    if(Scan.IfToken(')')) break;
    Scan.Match(',');
  }
}

wLit *Document::_Lit()
{
  wLit *lit = 0;
  switch(Scan.Token)
  {
  case sTOK_Int:
    lit = MakeLit(LitInt);
    lit->Int = Scan.ValI;
    Scan.Scan();
    break;
  case sTOK_Float:
    lit = MakeLit(LitFloat);
    lit->Float = Scan.ValF;
    Scan.Scan();
    break;
  case TokTrue:
    lit = MakeLit(LitBool);
    lit->Int = 1;
    Scan.Scan();
    break;
  case TokFalse:
    lit = MakeLit(LitBool);
    lit->Int = 0;
    Scan.Scan();
    break;
  case '{':
    lit = MakeLit(LitCompound);
    Scan.Match('{');
    while(!Scan.Errors)
    {
      lit->Members.Add(_Lit());
      if(Scan.IfToken('}')) break;
      Scan.Match(',');
      if(Scan.IfToken('}')) break;
    }
    break;
  default:
    Scan.Error("literal expeceted");
    lit = MakeLit(LitError);
    break;
  }
  return lit;
}

wExpr *Document::_Value()
{
  sPoolString name;
  wExpr *expr = 0;

  // prefix

  if(Scan.IfToken('-'))
    return MakeExpr(ExprNeg,_Expr());
  if(Scan.IfToken('+'))
    return _Expr();
  if(Scan.IfToken('!'))
    return MakeExpr(ExprNot,_Expr());
  if(Scan.IfToken(TokInc))
    return MakeExpr(ExprPreInc,_Expr());
  if(Scan.IfToken(TokDec))
    return MakeExpr(ExprPreDec,_Expr());

  // value

  wSymbol *sym;
  switch(Scan.Token)
  {
  case sTOK_Int:
  case sTOK_Float:
    expr = MakeExpr(ExprLit);
    expr->Loc = Scan.TokenLoc;
    expr->Literal = _Lit();
    break;
  case sTOK_Name:
    Scan.ScanName(name);
    sym = FindSymbol(name);
    if(sym && sym->Kind==SymType)
    {
      expr = MakeExpr(ExprCast);
      expr->Type = sym->Type;
      _Args(expr);
    }
    else
    {
      expr = MakeExpr(ExprSymbol);
      expr->Name = name;
    }
    break;
  case '(':
    Scan.Match('(');
    expr = _Expr();
    Scan.Match(')');
    break;
  default:
    Scan.Error("value expected");
    expr = MakeExpr(ExprError);
    break;
  }

  sASSERT(expr);

  // postfix

morepostfix:
  if(Scan.IfToken('.'))
  {
    expr = MakeExpr(ExprMember,expr);
    Scan.ScanName(expr->Name);
    goto morepostfix;
  }
  if(Scan.IfToken('['))
  {
    expr = MakeExpr(ExprIndex,expr,_Expr());
    Scan.Match(']');
    goto morepostfix;
  }
  if(Scan.Token=='(')     // function call
  {
    expr = MakeExpr(ExprCall,expr);
    _Args(expr);
    goto morepostfix;
  }
  if(Scan.IfToken(TokInc))
  {
    expr = MakeExpr(ExprPostInc,expr);
    goto morepostfix;
  }
  if(Scan.IfToken(TokDec))
  {
    expr = MakeExpr(ExprPostDec,expr);
    goto morepostfix;
  }

  return expr;
}

wExpr *Document::_ExprR(sInt maxlevel)
{
  sInt pri,mode,level;
  wExprKind kind;
  wExpr *expr; 

  expr = _Value();

  for(level=1;level<=maxlevel;level++)
  {
    while(!Scan.Errors)
    {
      kind = ExprError;
      pri = 0;
      mode = 0;
      switch(Scan.Token)
      {
      case '*':          pri= 1; mode=0x0000; kind=ExprMul;    break;
      case '/':          pri= 1; mode=0x0000; kind=ExprDiv;    break;
      case '%':          pri= 1; mode=0x0000; kind=ExprMod;    break;

      case '+':          pri= 2; mode=0x0000; kind=ExprAdd;    break;
      case '-':          pri= 2; mode=0x0000; kind=ExprSub;    break;

      case sTOK_ShiftL:  pri= 3; mode=0x0000; kind=ExprShiftL; break;
      case sTOK_ShiftR:  pri= 3; mode=0x0000; kind=ExprShiftR; break;

      case '>':          pri= 4; mode=0x0000; kind=ExprGT;     break;
      case '<':          pri= 4; mode=0x0000; kind=ExprLT;     break;
      case sTOK_GE:      pri= 4; mode=0x0000; kind=ExprGE;     break;
      case sTOK_LE:      pri= 4; mode=0x0000; kind=ExprLE;     break;

      case sTOK_EQ:      pri= 5; mode=0x0000; kind=ExprEQ;     break;
      case sTOK_NE:      pri= 5; mode=0x0000; kind=ExprNE;     break;

      case '&':          pri= 6; mode=0x0000; kind=ExprAnd;    break;
      case '|':          pri= 7; mode=0x0000; kind=ExprOr;     break;
      case '^':          pri= 8; mode=0x0000; kind=ExprEor;    break;

      case TokLAnd:      pri= 9; mode=0x0000; kind=ExprLAnd;   break;
      case TokLOr:       pri=10; mode=0x0000; kind=ExprLOr;    break;

      case '?':          pri=11; mode=0x0000; kind=ExprCond;   break;
      case '=':          pri=12; mode=0x0000; kind=ExprAssign; break;
      case TokAssignAdd: pri=12; mode=0x0000; kind=ExprAssignAdd; break;
      case TokAssignSub: pri=12; mode=0x0000; kind=ExprAssignSub; break;
      case TokAssignMul: pri=12; mode=0x0000; kind=ExprAssignMul; break;
      case TokAssignDiv: pri=12; mode=0x0000; kind=ExprAssignDiv; break;
      case TokAssignMod: pri=12; mode=0x0000; kind=ExprAssignMod; break;
      }

      if(kind==ExprError || pri!=level)
        break;
      Scan.Scan();

      if(kind==ExprAssign)
      {
        expr = MakeExpr(kind,expr,_ExprR(level));    // (a = (b = c))
      }
      else
      {
        expr = MakeExpr(kind,expr,_ExprR(level-1),0);
        if(kind==ExprCond)
        {
          expr->c = expr->b;
          expr->b = expr->a;
          Scan.Match(':');
          expr->a = _ExprR(level-1);
        }
      }
    }
  }

  return expr;
}

wExpr *Document::_Expr()
{
  return _ExprR(12);
}

wStmt **Document::_Stmt(wStmt **n)
{
  wStmt *s = 0;
  wType *type = 0;
  wSymbol *sym = 0;
  sPoolString name;
  switch(Scan.Token)
  {
  case TokBreak:
    Scan.Match(TokBreak);
    s = MakeStmt(StmtBreak);
    Scan.Match(';');
    break;

  case TokContinue:
    Scan.Match(TokContinue);
    s = MakeStmt(StmtContinue);
    Scan.Match(';');
    break;

  case TokDiscard:
    Scan.Match(TokDiscard);
    s = MakeStmt(StmtDiscard);
    Scan.Match(';');
    break;

  case TokDo:
    Scan.Match(TokDo);
    s = MakeStmt(StmtDo);
    s->Childs = _Block();
    Scan.Match(TokWhile);
    Scan.Match('(');
    s->Expr = _Expr();
    Scan.Match(')');
    Scan.Match(';');
    break;

  case TokFor:
    Scan.Match(TokFor);
    s = MakeStmt(StmtFor);
    s->Expr = MakeExpr(ExprError);
    Scan.Match('(');
    if(Scan.Token!=';')
      s->Expr->a = _Expr();
    Scan.Match(';');
    if(Scan.Token!=';')
      s->Expr->b = _Expr();
    Scan.Match(';');
    if(Scan.Token!=')')
      s->Expr->c = _Expr();
    Scan.Match(')');
    s->Childs = _Block();
    break;

  case TokIf:
    Scan.Match(TokIf);
    s = MakeStmt(StmtIf);
    Scan.Match('(');
    s->Expr = _Expr();
    Scan.Match(')');
    s->Childs = _Block();
    if(Scan.IfToken(TokElse))
      s->Alt = _Block();
    break;

  case TokStruct:
    _Struct();
    break;

  case TokSwitch:
    Scan.Match(TokSwitch);
    Scan.Error("switch not implemented");
    break;

  case TokTypedef:
    type = _Type();
    do
    {
      Scan.ScanName(name);
      if(IsSymbolUnique(name))
      {
        CScope->Add(MakeSym(SymType,name,type));
      }
      else
      {
        Scan.Error("name %s already defined in this scope",name);
      }
      s = MakeStmt(StmtTypedef);
      s->Expr = MakeExpr(ExprError);
      s->Expr->Name = name;
      s->Expr->Type = type;

      *n = s;
      n = &s->Next;
      s = 0;
    }
    while(Scan.IfToken(','));
    Scan.Match(';');
    break;

  case TokPif:
    ++PifCount;
    Scan.Match(TokPif);
    s = MakeStmt(StmtPif);
    Scan.Match('(');
    s->Expr = _Expr();
    Scan.Match(')');
    s->Childs = _Block(1);
    if(Scan.IfToken(TokPelse))
      s->Alt = _Block(1);
    --PifCount;
    break;

  case TokReturn:
    Scan.Match(TokReturn);
    s = MakeStmt(StmtReturn);
    if(Scan.Token!=';')
      s->Expr = _Expr();
    Scan.Match(';');
    break;

  case TokWhile:
    s = MakeStmt(StmtWhile);
    Scan.Match('(');
    s->Expr = _Expr();
    Scan.Match(')');
    s->Childs = _Block();
    break;

  case sTOK_Name:
    sym = FindSymbol(Scan.Name);
    if(sym && sym->Kind==SymType)
    {
      s = MakeStmt(StmtObject);
      s->Object = _Decl();
    }
    else
    {
      s = MakeStmt(StmtExpr);
      s->Expr = _Expr();
      Scan.Match(';');
    }
    break;

  default:
    s = MakeStmt(StmtObject);
    s->Object = _Decl();
    break;
  }

  if(s)
  {
    *n = s;
    n = &s->Next;
  }
  
  return n;
}

wStmt *Document::_Block(sBool noscope)
{
  wStmt *s = MakeStmt(StmtBlock);
  wStmt **n = &s->Childs;

  if(!noscope)
    PushScope();
  if(Scan.IfToken('{'))
  {
    while(!Scan.Errors && !Scan.IfToken('}'))
      n = _Stmt(n);
  }
  else
  {
    n = _Stmt(n);
  }
  if(!noscope)
    PopScope();

  return s;
}

/****************************************************************************/

wStmt *Document::_Shader()
{
  Scan.Scan();    // vs, ps, ..

  return _Block();
}

void Document::_A2B(wExpr *cond)
{
  wInvariant *inv = Pool->Alloc<wInvariant>();
  sClear(*inv);
  Material->Invariants.Add(inv);
  inv->Condition = cond;
  inv->Semantic = "";
  inv->Name = "";
  inv->Slot = -1;

  inv->MemberFlags = _MemberFlags();
  inv->Type = _Type();

  Scan.ScanName(inv->Name);

  while(Scan.IfToken(':'))
  {
    switch(Scan.Token)
    {
    case TokIA2VS:  Scan.Scan(); inv->KindMask |= InvIA2VS; break;
    case TokVS2RS:  Scan.Scan(); inv->KindMask |= InvVS2RS; break;
    case TokVS2PS:  Scan.Scan(); inv->KindMask |= InvVS2PS; break;
    case TokPS2OM:  Scan.Scan(); inv->KindMask |= InvPS2OM; break;
    case TokVS2GS:  Scan.Scan(); inv->KindMask |= InvVS2GS; break;
    case TokGS2PS:  Scan.Scan(); inv->KindMask |= InvGS2PS; break;
    case TokGS2RS:  Scan.Scan(); inv->KindMask |= InvGS2RS; break;
    case sTOK_Name:
      if(!inv->Semantic.IsEmpty())
        Scan.Error("double semantics not allowed");
      Scan.ScanName(inv->Semantic);
      break;
    case TokVS:
      Scan.Scan();
      if(inv->Slot>=0)
        Scan.Error("can bind only to one shader");
      Scan.Match('(');
      inv->Slot = Scan.ScanInt();
      Scan.Match(')');
      inv->KindMask |= InvVS;
      break;
    case TokHS:
      Scan.Scan();
      if(inv->Slot>=0)
        Scan.Error("can bind only to one shader");
      Scan.Match('(');
      inv->Slot = Scan.ScanInt();
      Scan.Match(')');
      inv->KindMask |= InvHS;
      break;
    case TokDS:
      Scan.Scan();
      if(inv->Slot>=0)
        Scan.Error("can bind only to one shader");
      Scan.Match('(');
      inv->Slot = Scan.ScanInt();
      Scan.Match(')');
      inv->KindMask |= InvDS;
      break;
    case TokGS:
      Scan.Scan();
      if(inv->Slot>=0)
        Scan.Error("can bind only to one shader");
      Scan.Match('(');
      inv->Slot = Scan.ScanInt();
      Scan.Match(')');
      inv->KindMask |= InvGS;
      break;
    case TokPS:
      Scan.Scan();
      if(inv->Slot>=0)
        Scan.Error("can bind only to one shader");
      Scan.Match('(');
      inv->Slot = Scan.ScanInt();
      Scan.Match(')');
      inv->KindMask |= InvPS;
      break;
    case TokCS:
      Scan.Scan();
      if(inv->Slot>=0)
        Scan.Error("can bind only to one shader");
      Scan.Match('(');
      inv->Slot = Scan.ScanInt();
      Scan.Match(')');
      inv->KindMask |= InvCS;
      break;
    default:
      Scan.Error("unknown semantic");
      break;
    }
  }

  Scan.Match(';');
}

void Document::_Permute()
{
  Scan.Match(TokPermute);
  sPoolString name,key;

  wPermute *pm = Pool->Alloc<wPermute>();
  sClear(*pm);
  pm->Options.Init();
  Scan.ScanName(pm->Name);
  Material->Permutes.AddTail(pm);

  if(!IsSymbolUnique(pm->Name))
    Scan.Error("name %s already defined in this scope",pm->Name);
  wSymbol *sym = MakeSym(SymPermute,pm->Name);
  sym->Permute = pm;
  CScope->Add(sym);

  if(Scan.IfToken('{'))
  {

    while(!Scan.Errors)
    {
      if(Scan.IfToken('}')) break;
      wPermuteOption *pk = Pool->Alloc<wPermuteOption>();
      sClear(*pk);
      Scan.ScanName(pk->Name);
      pm->Options.AddTail(pk);
      if(Scan.IfToken('}')) break;
      Scan.Match(',');
    }
  }
  Scan.Match(';');
}

void Document::_CBuffer()
{
  wCBuffer *cb = Pool->Alloc<wCBuffer>();
  sClear(*cb);
  cb->Members.Init();
  Material->CBuffers.Add(cb);
  Scan.Match(TokCBuffer);

  Scan.ScanName(cb->Name);

  while(Scan.IfToken(':'))
  {
    if(Scan.IfToken(TokRegister))
    {
      Scan.Match('(');
      cb->Register = Scan.ScanInt();
      Scan.Match(')');
    }
    else if(Scan.IfToken(TokVS)) 
    {
      cb->Shader=sST_Vertex;
      Scan.Match('(');
      cb->Slot = Scan.ScanInt();
      Scan.Match(')');
    }
    else if(Scan.IfToken(TokGS)) 
    {
      cb->Shader=sST_Geometry;
      Scan.Match('(');
      cb->Slot = Scan.ScanInt();
      Scan.Match(')');
    }
    else if(Scan.IfToken(TokPS)) 
    {
      cb->Shader=sST_Pixel;
      Scan.Match('(');
      cb->Slot = Scan.ScanInt();
      Scan.Match(')');
    }
    else
    {
      Scan.Error("unknown semantic");
    }

  }
  Scan.Match('{');
  sInt reg = cb->Register;
  sInt regscalar = 0;
  sInt regtemp = 0;
  wType *regtype = 0;
  wType *base = 0;
  static const sChar shaders[] = "vhdgpc";
  sString<sFormatBuffer> buffer;

  while(!Scan.Errors && !Scan.IfToken('}'))
  {
    wMember *mem = _Decl();

    base = mem->Type;
    if(mem->Type->Kind==TypeVector || mem->Type->Kind==TypeMatrix || mem->Type->Kind==TypeArray)
      base = base->Base;
    if(base->Kind!=TypeBase || (base->Token!=TokInt && base->Token!=TokFloat && base->Token!=TokUInt))
      Scan.Error("can not have type in a constant buffer (yet)");

    if(mem->Type->Kind==TypeVector || mem->Type->Kind==TypeBase)
    {
      sInt v = 1;
      if(mem->Type->Kind==TypeVector)
        v = mem->Type->Columns;
      if(v+regscalar>4 || regtype==0 || regtype!=base)   // start new register
      {
        regtype = base;
        regtemp = TempCount++;
        regscalar = 0;
        wMember *mr = Pool->Alloc<wMember>();
        sClear(*mr);
        mr->Register = reg++;
        mr->Type = MakeType(TypeVector,base);
        mr->Type->Columns = 4;
        buffer.PrintF("__temp%04d",regtemp);
        mr->Name = buffer;
        mr->Surrogate = "";
        mr->Semantics = "";
        cb->Members.Add(mr);
      }
      if(Platform==sConfigRenderGL2 || Platform==sConfigRenderGLES2)
      {
        buffer.PrintF("%cc%d[%d].",shaders[cb->Shader],cb->Slot,reg-1);
      }
      else
      {
        buffer.PrintF("__temp%04d.",regtemp);
      }
      mem->AssignedScalar = (reg-1)*4+regscalar;
      for(sInt i=0;i<v;i++)
        buffer.Add("xyzw"+(regscalar+i),1);
      regscalar += v;
      mem->Surrogate = buffer;
      mem->Register = -1;
      cb->Members.Add(mem);
    }
    else if(mem->Type->Kind==TypeMatrix || mem->Type->Kind==TypeArray)
    {
      mem->AssignedScalar = reg*4;
      mem->Register = reg;
      reg += GetRegCount(mem);
      cb->Members.Add(mem);

      if(Platform==sConfigRenderGL2 || Platform==sConfigRenderGLES2)
      {
        static const sChar *swizzles[] = { "","x","xy","xyz","xyzw" };
        if(Platform==sConfigRenderGLES2 && mem->Type->Rows==mem->Type->Columns)
          buffer.PrintF("mat%d(",mem->Type->Rows);
        else
          buffer.PrintF("mat%dx%d(",mem->Type->Rows,mem->Type->Columns);
        for(sInt i=0;i<mem->Type->Rows;i++)
        {
          if(i!=0) buffer.Add(",");
//          buffer.AddF("%cc%d[%d].%s",shaders[cb->Shader],cb->Slot,mem->Register+i,swizzles[mem->Type->Columns]);
 //         buffer.AddF("%cc%d[%d].%s",shaders[cb->Shader],cb->Slot,mem->Register+i,swizzles[mem->Type->Columns]);
          buffer.AddF("vec%d(",mem->Type->Columns);
          for(sInt j=0;j<mem->Type->Columns;j++)
          {
            if(j!=0) buffer.Add(",");
            buffer.AddF("%cc%d[%d].%c",shaders[cb->Shader],cb->Slot,mem->Register+j,("xyzw")[i]);
          }
          buffer.Add(")");
        }
        buffer.AddF(")");
        mem->Surrogate = buffer;
      }

      regscalar = 0;
      regtype = 0;
      regtemp = 0;
    }
  }
  cb->RegSize = reg-cb->Register;
  Scan.Match(';');
}

void Document::_MtrlBlock(wExpr *cond)
{
  if(Scan.IfToken('{'))
    while(!Scan.Errors && !Scan.IfToken('}'))
      _MtrlStmt(cond);
  else
    _MtrlStmt(cond);
}

void Document::_MtrlStmt(wExpr *cond)
{
  wExpr *pifcond;
  switch(Scan.Token)
  {
  case TokPermute:
    if(cond)
      Scan.Error("permute inside pif is not allowed");
    _Permute();
    break;
  case TokPif:
    Scan.Match(TokPif);
    Scan.Match('(');
    if(cond)
      pifcond = MakeExpr(ExprLAnd,cond,_Expr());
    else
      pifcond = _Expr();
    Scan.Match(')');
    _MtrlBlock(pifcond);
    if(Scan.IfToken(TokPelse))
      _MtrlBlock(MakeExpr(ExprNot,pifcond));
    break;

  case TokCBuffer:
    if(cond)
      Scan.Error("cbuffer inside pif is not allowed");
    _CBuffer();
    break;

  case TokVS:
    if(cond)
      Scan.Error("shader inside pif is not allowed");
    Material->Shaders[sST_Vertex] = _Shader();
    break;
  case TokGS:
    if(cond)
      Scan.Error("shader inside pif is not allowed");
    Scan.Scan();
attr:
    if(Scan.IfName("maxvertexcount"))
    {
      Scan.Match('=');
      Material->GeoMaxVert = Scan.ScanInt();
      Scan.Match(';');
      goto attr;
    }
    if(Scan.IfName("type"))
    {
      Scan.Match('=');
      if(Scan.IfName("point")) Material->GeoType = 1;
      else if(Scan.IfName("line")) Material->GeoType = 2;
      else if(Scan.IfName("triangle")) Material->GeoType = 3;
      else if(Scan.IfName("lineadj")) Material->GeoType = 4;
      else if(Scan.IfName("triangleadj")) Material->GeoType = 5;
      else Scan.Error("unknown geometry shader input type");
      Scan.Match(';');
      goto attr;
    }
    Material->Shaders[sST_Geometry] = _Block();
    break;
  case TokPS:
    if(cond)
      Scan.Error("shader inside pif is not allowed");
    Material->Shaders[sST_Pixel] = _Shader();
    break;

  default:
    _A2B(cond);
    break;
  }
}

/****************************************************************************/


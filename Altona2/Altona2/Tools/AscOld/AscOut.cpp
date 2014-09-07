/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "altona2/tools/AscOld/asc.hpp"
#include "altona2/libs/util/scanner.hpp"
#include "altona2/libs/base/dxshadercompiler.hpp"

using namespace AltonaShaderLanguage;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Document::OutMemberFlags(sInt flags)
{
  if(Platform==sConfigRenderGLES2)
  {
    if((flags & MemPrecisionMask)==0)
    {
      if(ShaderType==sST_Pixel)
        Out.Print("mediump ");
    }
    else
    {
      if(flags & MemLowP)       Out.Print("lowp ");
      if(flags & MemMediumP)    Out.Print("mediump ");
      if(flags & MemHighP)      Out.Print("highp ");
    }
  }

  if(flags & MemExtern)         Out.Print("extern ");
  if(flags & MemNointerpolation)Out.Print("nointerpolation ");
  if(flags & MemPrecise)        Out.Print("precise ");
  if(flags & MemGroupShared)    Out.Print("groupshared ");
  if(flags & MemStatic)         Out.Print("static ");
  if(flags & MemUniform)        Out.Print("uniform ");
  if(flags & MemVolatile)       Out.Print("volatile ");
  if(flags & MemConst)          Out.Print("const ");
  if(flags & MemColumnMajor)    Out.Print("column_major ");
  if(flags & MemRowMajor)       Out.Print("row_major ");
}

void Document::OutMember(wMember *mem,sInt indent,sInt flags)
{
  flags |= mem->Flags;

  Out.PrintF("%_",indent*2);

  OutMemberFlags(flags);
  OutType(mem->Type);
  Out.PrintF(" %s",mem->Name);
  if(!mem->Semantics.IsEmpty())
  {
    const sChar *sema = mem->Semantics;
    if(Platform == sConfigRenderDX9)
    {
      if(sCmpStringI(sema,"SV_Position")==0) sema = "POSITION";
      if(sCmpStringI(sema,"SV_Target")==0) sema = "COLOR";
      if(sCmpStringI(sema,"SV_Target0")==0) sema = "COLOR0";
      if(sCmpStringI(sema,"SV_Target1")==0) sema = "COLOR1";
      if(sCmpStringI(sema,"SV_Target2")==0) sema = "COLOR2";
      if(sCmpStringI(sema,"SV_Target3")==0) sema = "COLOR3";
    }
    Out.PrintF(" : %s",mem->Semantics);
  }
  if(mem->Register>=0)
    Out.PrintF(" : register(c%d)",mem->Register);

  if(mem->Initializer)
  {
    Out.PrintF(" = ");
    OutExpr(mem->Initializer);
  }
  Out.PrintF(";\n");
}

void Document::OutLit(wLit *l)
{
  wLit *c;
  switch(l->Kind)
  {
  case LitError:
    Out.Print("LitError");
    break;
  case LitBool:
  case LitInt:
    Out.PrintF("%d",l->Int);
    break;
  case LitFloat:
    Out.PrintF("%f",l->Float);
    break;
  case LitCompound:
    Out.Print("{");
    sFORALL(l->Members,c)
    {
      if(c!=l->Members.GetFirst())
        Out.Print(",");
      OutLit(c);
    }
    Out.Print("}");
    break;
  }
}


void Document::OutType(wType *t)
{
  if(Platform==sConfigRenderGL2 || Platform==sConfigRenderGLES2)
  {
    switch(t->Kind)
    {
    case TypeError:
      Out.Print("TypeError");
      break;
    case TypeVoid:
      Out.Print("void");
      break;
    case TypeBase:
      switch(t->Token)
      {
      default:
        Scan.Error("GL2: illegal type");
        break;

      case TokBool:             Out.Print("bool"); break;
      case TokFloat:            Out.Print("float"); break;
      case TokInt:              Out.Print("int"); break;
      case TokTexture2D:        Out.Print("sampler2D"); break;
      case TokSampler2D:        Out.Print("Sampler2D"); break;
      case TokSamplerCube:      Out.Print("SamplerCube"); break;
      case TokSamplerState:     Out.Print("SamplerState"); break;
      }
      break;
    case TypeVector:
      sASSERT(t->Base->Kind==TypeBase);

      if(t->Base->Token==TokFloat) 
        ; 
      else if(t->Base->Token==TokInt)
        Out.Print("i");
      else if(t->Base->Token==TokBool)
        Out.Print("b");
      else
        Scan.Error("GL2: illegal type");

      Out.PrintF("vec%d",t->Columns);
      break;
    case TypeMatrix:
      if(!(t->Base->Kind==TypeBase || t->Base->Token==TokFloat))
        Scan.Error("GL2: illegal type");
      if(Platform==sConfigRenderGLES2 && t->Rows==t->Columns)
        Out.PrintF("mat%d",t->Rows);
      else
        Out.PrintF("mat%dx%d",t->Rows,t->Columns);
      break;
    case TypeArray:
      OutType(t->Base);
      Out.PrintF("[%d]",ConstInt(t->Count));
      break;
    default:
      sASSERT0();
    }
  }
  else
  {
    if(!t->DebugName.IsEmpty())
    {
      Out.Print(t->DebugName);
      return;
    }

    switch(t->Kind)
    {
    case TypeError:
      Out.Print("TypeError");
      break;
    case TypeVoid:
      Out.Print("void");
      break;
    case TypeBase:
      switch(t->Token)
      {
      default:
        sASSERT0();
        break;

      case TokBool:             Out.Print("bool"); break;
      case TokFloat:            Out.Print("float"); break;
      case TokInt:              Out.Print("int"); break;
      case TokUInt:             Out.Print("uint"); break;
      case TokSampler:          Out.Print("Sampler"); break;
      case TokSampler1D:        Out.Print("Sampler1D"); break;
      case TokSampler2D:        Out.Print("Sampler2D"); break;
      case TokSampler3D:        Out.Print("Sampler3D"); break;
      case TokSamplerCube:      Out.Print("SamplerCube"); break;
      case TokSamplerState:     Out.Print("SamplerState"); break;
      case TokSamplercomparisonState: 
                                Out.Print("SamplerComparisonState"); break;
      case TokTexture:          Out.Print("Texture"); break;
      case TokTexture1D:        Out.Print("Texture1D"); break;
      case TokTexture1DArray:   Out.Print("Texture1DArray"); break;
      case TokTexture2D:        Out.Print("Texture2D"); break;
      case TokTexture2DArray:   Out.Print("Texture2DArray"); break;
      case TokTexture2DMS:      Out.Print("Texture2DMS"); break;
      case TokTexture2DMSArray: Out.Print("Texture2DMSArray"); break;
      case TokTexture3D:        Out.Print("Texture3D"); break;
      case TokTextureCube:      Out.Print("TextureCube"); break;
      case TokTextureCubeArray: Out.Print("TextureCubeArray"); break;

      }
      break;
    case TypeVector:
      Out.Print("vector<");
      OutType(t->Base);
      Out.PrintF(",%d>",t->Columns);
      break;
    case TypeMatrix:
      Out.Print("matrix<");
      OutType(t->Base);
      Out.PrintF(",%d,%d>",t->Rows,t->Columns);
      break;
    case TypeArray:
      OutType(t->Base);
      Out.PrintF("[%d]",ConstInt(t->Count));
      break;
    default:
      sASSERT0();
    }
  }
}

void Document::OutExprTypeBin(const sChar *op,wExpr *e,sInt typemode)
{
  OutExprType(e->a);
  OutExprType(e->b);

  if(typemode/* && e->ResultType==0*/)   // try both direction
  {
    e->ResultType = MakeCompatible(e->Loc,&e->a,e->b->ResultType,1);
    if(!e->ResultType)
      e->ResultType = MakeCompatible(e->Loc,&e->b,e->a->ResultType);
  }
  else    // as in  a = b : modify by so that it fits to a. from b to a.
  {
    e->ResultType = MakeCompatible(e->Loc,&e->b,e->a->ResultType);
  }
  if(e->ResultType==0) e->ResultType = ErrorType;
  if(typemode==2)
    e->ResultType = MakeTypeBase(TokBool);
}

void Document::OutExprType(wExpr *e)
{
  wExpr *arg;
  wSymbol *sym;

  e->ResultType = ErrorType;

  switch(e->Kind)
  {

  case ExprMul:       OutExprTypeBin("*" ,e,1); break;
  case ExprDiv:       OutExprTypeBin("/" ,e,1); break;
  case ExprMod:       OutExprTypeBin("%" ,e,1); break;
  case ExprAdd:       OutExprTypeBin("+" ,e,1); break;
  case ExprSub:       OutExprTypeBin("-" ,e,1); break;
  case ExprShiftL:    OutExprTypeBin("<<",e,1); break;
  case ExprShiftR:    OutExprTypeBin(">>",e,1); break;
  case ExprGT:        OutExprTypeBin(">" ,e,2); break;
  case ExprLT:        OutExprTypeBin("<" ,e,2); break;
  case ExprGE:        OutExprTypeBin(">=",e,2); break;
  case ExprLE:        OutExprTypeBin("<=",e,2); break;
  case ExprEQ:        OutExprTypeBin("==",e,2); break;
  case ExprNE:        OutExprTypeBin("!=",e,2); break;
  case ExprAnd:       OutExprTypeBin("&" ,e,1); break;
  case ExprOr:        OutExprTypeBin("|" ,e,1); break;
  case ExprEor:       OutExprTypeBin("^" ,e,1); break;
  case ExprLAnd:      OutExprTypeBin("&&",e,1); break;
  case ExprLOr:       OutExprTypeBin("||",e,1); break;
  case ExprAssign:    OutExprTypeBin("=" ,e,0); break;
  case ExprAssignAdd: OutExprTypeBin("+=",e,0); break;
  case ExprAssignSub: OutExprTypeBin("-=",e,0); break;
  case ExprAssignMul: OutExprTypeBin("*=",e,0); break;
  case ExprAssignDiv: OutExprTypeBin("/=",e,0); break;
  case ExprAssignMod: OutExprTypeBin("%=",e,0); break;
    break;

  case ExprError: 
    e->ResultType = ErrorType;
    break;

  case ExprCond:
    OutExprType(e->a);
    OutExprType(e->b);
    OutExprType(e->c);
    Same(e->Loc,e->a->ResultType,e->b->ResultType);
    e->ResultType = e->a->ResultType;
    if(e->ResultType==0) e->ResultType = ErrorType;
    MakeCompatible(e->Loc,&e->c,MakeTypeBase(TokBool));
    break;

  case ExprNot:
  case ExprNeg:
  case ExprComplement:
  case ExprPreInc:
  case ExprPreDec:
  case ExprPostInc:
  case ExprPostDec:
    OutExprType(e->a);
    e->ResultType = e->a->ResultType;
    break;
  case ExprLit:
    e->ResultType = GetLiteralType(e->Literal);
    break;
  case ExprIndex:
    OutExprType(e->a);
    if(e->a->ResultType->Kind!=TypeArray)
      Scan.Error("array type expected for indexing");
    else
      e->ResultType = e->a->Type->Base;
    if(e->b->ResultType!=MakeTypeBase(TokInt))
      Scan.Error("int type expected as index");
    break;
  case ExprSymbol:
    sym = FindSymbol(e->Scope,e->Name);
    if(!sym)
      Scan.Error(e->Loc,"symbol %d not defined (perm 0x%08x)",e->Name,Perm);
    else
      e->ResultType = sym->Type;
    break;
  case ExprCast:
    e->ResultType = e->Type;
    sFORALL(e->Args,arg)
      OutExprType(arg);
    break;
  case ExprCall:
    {
      sBool funcok = 0;
      sPoolString funcname;
      wType *funcclass = 0;

      sFORALL(e->Args,arg)
        OutExprType(arg);

      
      if(e->a->Kind==ExprSymbol)
      {
        funcname = e->a->Name;
        sym = FindSymbol(e->a->Scope,e->a->Name);
        if(!sym)
        {
          funcok = 1;
        }
      }
      else if(e->a->Kind==ExprMember && e->a->a->Kind==ExprSymbol)
      {
        funcname = e->a->Name;
        sym = FindSymbol(e->a->a->Scope,e->a->a->Name);
        if(sym)
        {
          funcclass = sym->Type;
          funcok = 1;
        }
        else
        {
          funcok = 0;
          Scan.Error(e->Loc,"symbol %d not defined (perm 0x%08x)",e->a->a->Name,Perm);
        }
      }

      if(funcok)
      {
        // check for a fitting function. and find out result type

        wIntrinsic *in = 0;
        wIntrinsic *ini;
        sInt argcnt = e->Args.GetCount();
        sFORALL(Intrinsics,ini)
        {
          if(ini->ArgCount == argcnt && ini->Name==e->a->Name && ini->Class==funcclass)
          {
            sInt i = 0;
            sBool ok = 1;
            sFORALL(e->Args,arg)
              if(!Compatible(e->Loc,arg->ResultType,ini->Arg[i++],1))
                ok = 0;
            if(ok)
            {
              in = ini;
              break;
            }
          }
        }
        e->Intrinsic = in;
        if(in)
          e->ResultType = in->Result;
        else
          funcok = 0;

      }
      if(!funcok)
      {
        Scan.Error(e->Loc,"unrecognised call");
      }
    }
    break;

  case ExprMember:
    OutExprType(e->a);
    if(e->a->ResultType->Kind==TypeVector)
    {
      e->ResultType = MakeType(TypeVector,e->a->ResultType->Base);
      e->ResultType->Columns = sInt(sGetStringLen(e->Name));
    }
    else
    {
      Scan.Error(e->Loc,"swizzle needs vector type");
    }
    break;

  default:
    sASSERT0();
  }
}


void Document::OutExprPrintBin(const sChar *op,wExpr *e,sInt typemode)
{
  OutExprPrint(e->a,1);
  Out.Print(op);
  OutExprPrint(e->b,1);
}

void Document::OutExprPrint(wExpr *e,sBool brace)
{
  wExpr *arg;
  wSymbol *sym;
  sInt i=0;
  if(e->Kind==ExprLit || e->Kind==ExprSymbol || e->Kind==ExprCall || e->Kind==ExprCast)
    brace = 0;
  if(brace)
    Out.Print("(");

  switch(e->Kind)
  {

  case ExprMul:       OutExprPrintBin("*" ,e,1); break;
  case ExprDiv:       OutExprPrintBin("/" ,e,1); break;
  case ExprMod:       OutExprPrintBin("%" ,e,1); break;
  case ExprAdd:       OutExprPrintBin("+" ,e,1); break;
  case ExprSub:       OutExprPrintBin("-" ,e,1); break;
  case ExprShiftL:    OutExprPrintBin("<<",e,1); break;
  case ExprShiftR:    OutExprPrintBin(">>",e,1); break;
  case ExprGT:        OutExprPrintBin(">" ,e,2); break;
  case ExprLT:        OutExprPrintBin("<" ,e,2); break;
  case ExprGE:        OutExprPrintBin(">=",e,2); break;
  case ExprLE:        OutExprPrintBin("<=",e,2); break;
  case ExprEQ:        OutExprPrintBin("==",e,2); break;
  case ExprNE:        OutExprPrintBin("!=",e,2); break;
  case ExprAnd:       OutExprPrintBin("&" ,e,1); break;
  case ExprOr:        OutExprPrintBin("|" ,e,1); break;
  case ExprEor:       OutExprPrintBin("^" ,e,1); break;
  case ExprLAnd:      OutExprPrintBin("&&",e,1); break;
  case ExprLOr:       OutExprPrintBin("||",e,1); break;
  case ExprAssign:    OutExprPrintBin("=" ,e,0); break;
  case ExprAssignAdd: OutExprPrintBin("+=",e,0); break;
  case ExprAssignSub: OutExprPrintBin("-=",e,0); break;
  case ExprAssignMul: OutExprPrintBin("*=",e,0); break;
  case ExprAssignDiv: OutExprPrintBin("/=",e,0); break;
  case ExprAssignMod: OutExprPrintBin("%=",e,0); break;
    break;

  case ExprError: 
    Out.Print("ExprError"); 
    break;

  case ExprCond:
    OutExprPrint(e->c,1);
    Out.Print("?");
    OutExprPrint(e->b,1);
    Out.Print(":");
    OutExprPrint(e->a,1);
    break;

  case ExprNot:
    Out.Print("!");
    OutExprPrint(e->a,1);
    break;
  case ExprNeg:
    Out.Print("-");
    OutExprPrint(e->a,1);
    break;
  case ExprComplement:
    Out.Print("~");
    OutExprPrint(e->a,1);
    break;
  case ExprPreInc:
    Out.Print("++");
    OutExprPrint(e->a,1);
    break;
  case ExprPreDec:
    Out.Print("--");
    OutExprPrint(e->a,1);
    break;
  case ExprPostInc:
    OutExprPrint(e->a,1);
    Out.Print("++");
    break;
  case ExprPostDec:
    OutExprPrint(e->a,1);
    Out.Print("--");
    break;
  case ExprLit:
    OutLit(e->Literal);
    break;
  case ExprIndex:
    OutExprPrint(e->a,1);
    Out.Print("[");
    OutExprPrint(e->b,0);
    Out.Print("]");
    break;
  case ExprSymbol:
    sym = FindSymbol(e->Scope,e->Name);
    if(sym)
    {
      if(!sym->Surrogate.IsEmpty())
        Out.Print(sym->Surrogate);
      else
        Out.Print(sym->Name);
    }
    break;
  case ExprCast:
    OutType(e->Type);
    Out.Print("(");
    sFORALL(e->Args,arg)
    {
      if(i)
        Out.Print(",");
      i = 1;
      OutExprPrint(arg,0);
    }
    Out.Print(")");
    break;
  case ExprCall:
    {
      wIntrinsic *in = e->Intrinsic;
      if(in)
      {

        const sChar *handle = 0;
        if(Platform==sConfigRenderDX9) handle = in->HandleDX9;
        if(Platform==sConfigRenderDX11) handle = in->HandleDX11;
        if(Platform==sConfigRenderGL2) handle = in->HandleGL2;
        if(Platform==sConfigRenderGLES2) handle = in->HandleGL2;

        if(handle)
        {
          if(handle[0]=='1')      // dx9 special case
          {
            Out.Print(handle+1);
            Out.Print("(");
            i = 0;
            sFORALL(e->Args,arg)
            {
              if(i!=0)
                Out.Print(",");
              i = 1;
              OutExprPrint(arg,0);
            }
            Out.Print(")");
          }
          else if(handle[0]=='2')      // same as above for gl
          {
            Out.Print(handle+1);
            Out.Print("(");
            i = 0;
            OutExprPrint(e->a->a,0);
            sFORALL(e->Args,arg)
            {
              if(i!=0)
              {
                Out.Print(",");
                OutExprPrint(arg,0);
              }
              i = 1;
            }
            Out.Print(")");
          }
          else if(handle[0]=='!')
          {
            Scan.Error(e->Loc,"texture sampling method not available on this platform");
          }
          else if(handle[0]=='m' && handle[1]==0)
          {
            arg = e->Args.GetFirst();
            Out.Print(" ( ");
            OutExprPrint(arg,1);
            Out.Print(" * ");
            OutExprPrint(arg->Next,1);
            Out.Print(" ) ");
          }
          else if(handle[0]=='s' && handle[1]==0)
          {
            Out.Print("clamp(");
            OutExprPrint(e->Args.GetFirst(),0);
            Out.Print(",0.0,1.0)");
          }
          else                     // other special cases
          {
            sASSERT0();
          }
        }
        else                              // normal case (like dx11 with sampler and texture)
        {
          if(in->Class)
          {
            OutExprPrint(e->a->a,1);
            Out.Print(".");
          }

          Out.PrintF("%s(",in->Name);
          i = 0;
          sFORALL(e->Args,arg)
          {
            if(i)
              Out.Print(",");
            i = 1;
            OutExprPrint(arg,0);
          }
          Out.Print(")");
        }
      }
    }
    break;

  case ExprMember:
    OutExprPrint(e->a,1);
    Out.PrintF(".%s",e->Name);
    break;

  default:
    sASSERT0();
  }

  if(brace)
    Out.Print(")");
}

void Document::OutExpr(wExpr *e)
{
  OutExprType(e);
  OutExprPrint(e,0);
}


void Document::OutStmt(wStmt *s,sInt indent,sBool pif)
{
  wStmt *n;
  sBool br=0;
  switch(s->Kind)
  {
  case StmtError:
    Out.PrintF("%_StmtError\n",indent*2);
    break;
  case StmtNop:
    break;
  case StmtExpr:
    Out.PrintF("%_",indent*2);
    OutExpr(s->Expr);
    Out.Print(";\n");
    break;
  case StmtBlock:
    n = s->Childs;    

    if(pif)
    {
      while(n)
      {
        OutStmt(n,indent);
        n = n->Next;
      }
    }
    else
    {
      br = (n==0 || n->Next!=0 || indent==0);
      if(br)
        Out.PrintF("%_{\n",indent*2);
      while(n)
      {
        OutStmt(n,indent+1);
        n = n->Next;
      }
      if(br)
        Out.PrintF("%_}\n",indent*2);
    }
    break;
  case StmtObject:
    OutMember(s->Object,indent,0);
    s->Scope->Add(MakeSym(SymObject,s->Object->Name,s->Object->Type));
    break;

  case StmtBreak:
    Out.PrintF("%_break;\n",indent*2);
    break;
  case StmtContinue:
    Out.PrintF("%_continue;\n",indent*2);
    break;
  case StmtDiscard:
    Out.PrintF("%_discard;\n",indent*2);
    break;
  case StmtDo:
    Out.PrintF("%_do\n",indent*2);
    OutStmt(s->Childs,indent);
    break;
  case StmtFor:
    Out.PrintF("%_for(",indent*2);
    OutExpr(s->Expr->a);
    Out.Print(";");
    OutExpr(s->Expr->b);
    Out.Print(";");
    OutExpr(s->Expr->c);
    Out.Print(")\n");
    break;
  case StmtIf:
    Out.PrintF("%_if(",indent*2);
    OutExpr(s->Expr);
    Out.Print(")\n");
    OutStmt(s->Childs,indent);
    if(s->Alt)
    {
      Out.PrintF("%_else\n",indent*2);
      OutStmt(s->Alt,indent);
    }
    break;
  case StmtPif:
    if(ConstInt(s->Expr))
      OutStmt(s->Childs,indent,1);
    else if(s->Alt)
      OutStmt(s->Alt,indent,1);
    break;
  case StmtReturn:
    Out.PrintF("%_return",indent*2);
    if(s->Expr)
    {
      Out.Print(" ");
      OutExpr(s->Expr);
    }
    Out.Print("\n");
    break;
  case StmtTypedef:
    Out.PrintF("%_typedef ",indent*2);
    OutType(s->Expr->Type);
    Out.PrintF(" %s;\n",s->Expr->Name);
    break;
  case StmtWhile:
    Out.PrintF("%_while(",indent*2);
    OutExpr(s->Expr);
    Out.Print(")\n");
    OutStmt(s->Childs,indent);
    break;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sShaderBinary *Document::OutShader(sShaderTypeEnum skind,wStmt *root)
{
  wInvariant *inv;
  wCBuffer *cb;
  wMember *mem;
  Out.Clear();
  sInt dmask = 0;
  sInt imask = 0;
  sInt omask = 0;
  const sChar *shadername = "??";
  sString<256> buffer;
  wSymbol *sym;

  ShaderType = skind;
  ClearObjectSymbols(GScope);

  switch(skind)
  {
  case sST_Vertex:
    dmask = InvVS;
    imask = InvIA2VS;
    omask = InvVS2RS | InvVS2PS | InvVS2GS;
    shadername = "vertex";
    break;
  case sST_Geometry:
    dmask = InvGS;
    imask = InvVS2GS;
    omask = InvGS2PS|InvGS2RS;
    shadername = "geometry";
    break;
  case sST_Pixel:
    dmask = InvPS;
    imask = InvVS2PS|InvGS2PS;
    omask = InvPS2OM;
    shadername = "pixel";
    break;
  default:
    sASSERT0();
  }

  if(Platform==sConfigRenderGL2)
  {
    Out.Print("#version 120\n");
  }
  if(Platform==sConfigRenderGLES2)
  {
    Out.Print("#version 100\n");
  }

  // constant buffers

  sFORALL(Material->CBuffers,cb)
  {
    if(cb->Shader==skind)
    {
      if(Platform==sConfigRenderGL2 || Platform==sConfigRenderGLES2)
      {
        static const sChar shaders[] = "vhdgpc";
        Out.PrintF("uniform vec4 %cc%d[%d];\n",shaders[cb->Shader],cb->Slot,cb->RegSize);
        sFORALL(cb->Members,mem)
        {
          sym = MakeSym(SymObject,mem->Name,mem->Type);
          sym->Surrogate = mem->Surrogate;
          CScope->Add(sym);
        }
      }
      else
      {
        sFORALL(cb->Members,mem)
        {
          if(mem->Surrogate.IsEmpty())
            OutMember(mem,0,MemUniform);
          sym = MakeSym(SymObject,mem->Name,mem->Type);
          sym->Surrogate = mem->Surrogate;
          CScope->Add(sym);
        }
      }
    }
  }

  // textures and samplers

  sFORALL(Material->Invariants,inv)
  {
    if(inv->KindMask & dmask)
    {
      if(inv->Condition)
      {
        sInt n = ConstInt(inv->Condition);
        if(n==0)
          continue;
      }

      if(Platform==sConfigRenderGL2 || Platform==sConfigRenderGLES2)
      {
        if(inv->Type->IsTextureType())
        {
          Out.Print("uniform ");
          OutType(inv->Type);
          Out.PrintF(" %s_%d;\n",inv->Name,inv->Slot);
          sym = MakeSym(SymObject,inv->Name,inv->Type);
          buffer.PrintF("%s_%d",inv->Name,inv->Slot);
          sym->Surrogate = buffer;
          CScope->Add(sym);
        }
        else
        {
          CScope->Add(MakeSym(SymObject,inv->Name,inv->Type));
        }
      }
      else 
      {
        if(inv->Type->IsTextureType() && Platform==sConfigRenderDX9)
        {
          // on dx9, just skip the textures.
        }
        else
        {
          OutType(inv->Type);
          Out.PrintF(" %s",inv->Name);

          if(inv->Type->IsTextureType())
            Out.PrintF(" : register(t%d)",inv->Slot);
          if(inv->Type->IsSamplerType())
            Out.PrintF(" : register(s%d)",inv->Slot);

          Out.Print(";\n");
        }
        CScope->Add(MakeSym(SymObject,inv->Name,inv->Type));
      }
    }
  }

  // main and invariants

  if(Platform==sConfigRenderGL2 || Platform==sConfigRenderGLES2)
  {
    sInt attrcount = 0;
    sFORALL(Material->Invariants,inv)
    {
      if(inv->Condition)
      {
        sInt n = ConstInt(inv->Condition);
        if(n==0)
          continue;
      }

      if(inv->KindMask & imask)
      {
        if(skind==sST_Vertex)
        {
          Out.Print("attribute ");
          OutMemberFlags(inv->MemberFlags);
          OutType(inv->Type);
          Out.PrintF(" %s_%d;\n",inv->Name,attrcount);
          sym = MakeSym(SymObject,inv->Name,inv->Type);
          buffer.PrintF("%s_%d",inv->Name,attrcount);
          sym->Surrogate = buffer;
          inv->InSurrogate = buffer;
          CScope->Add(sym);
          attrcount++;
        }
        else
        {
          Out.Print("varying ");
          OutMemberFlags(inv->MemberFlags);
          OutType(inv->Type);
          Out.PrintF(" %s;\n",inv->Semantic);
          sym = MakeSym(SymObject,inv->Name,inv->Type);
          sym->Surrogate = inv->Semantic;
          CScope->Add(sym);
        }
      }
      if(inv->KindMask & omask)
      {
        if(skind==sST_Vertex)
        {
          if(inv->Semantic=="SV_Position")
          {
            sym = MakeSym(SymObject,inv->Name,inv->Type);
            sym->Surrogate = "gl_Position";
            CScope->Add(sym);
          }
          else
          {
            Out.Print("varying ");
            OutMemberFlags(inv->MemberFlags);
            OutType(inv->Type);
            Out.PrintF(" %s;\n",inv->Semantic);
            sym = MakeSym(SymObject,inv->Name,inv->Type);
            sym->Surrogate = inv->Semantic;
            CScope->Add(sym);
          }
        }
        else
        {
          if(inv->Semantic=="SV_Target")
          {
            sym = MakeSym(SymObject,inv->Name,inv->Type);
            sym->Surrogate = "gl_FragColor";
            CScope->Add(sym);
          }
          else
          {
            Scan.Error("GL2: unknown sematic %s",inv->Semantic);
          }
        }
      }
    }
    Out.Print("\nvoid main()\n");
  }
  else
  {
    Out.Print("\nvoid main\n");
    Out.Print("(");
    sInt komma = 0;
    sFORALL(Material->Invariants,inv)
    {
      if(inv->Condition)
      {
        sInt n = ConstInt(inv->Condition);
        if(n==0)
          continue;
      }

      const sChar *mode = 0;
      if((inv->KindMask & imask) && (inv->KindMask & omask))
        mode = "inout";
      else if(inv->KindMask & imask)
        mode = "in";
      else if(inv->KindMask & omask)
        mode = "out";
        
      if(mode!=0)
      {
        if(komma)
          Out.Print(",\n");
        else
          Out.Print("\n");

        Out.PrintF("  %s ",mode);
        OutType(inv->Type);
        Out.PrintF(" %s",inv->Name);
        if(!inv->Semantic.IsEmpty())
          Out.PrintF(" : %s",inv->Semantic);
        komma = 1;

        CScope->Add(MakeSym(SymObject,inv->Name,inv->Type));
      }
    }
    Out.Print("\n)\n");
  }

  // code

  if(Platform==sConfigRenderGL2 || Platform==sConfigRenderGLES2)
  {
    Out.Print("{\n");

    sFORALL(Material->Invariants,inv)
    {
      if(inv->Condition)
      {
        sInt n = ConstInt(inv->Condition);
        if(n==0) continue;
      }
      if((inv->KindMask & imask) && (inv->KindMask & omask))
      {
        Out.PrintF("  %s = %s;\n",inv->Semantic,inv->InSurrogate);
      }
    }

    OutStmt(root,1);
    Out.Print("}\n");
  }
  else
  {
    OutStmt(root,0);
  }

  if(Scan.Errors)
  {
    Dump.Print(Scan.ErrorLog.Get());
    return 0;
  }

  // dump code

  if(Flags & 1)
  {
    Dump.PrintF("--- %s shader permutation 0x%08x\n",shadername,Perm);
    Dump.Print(Out.Get());
    Dump.Print("\n");
  }

  // compile shader

  sTextLog errors;
  sShaderBinary *bin=0;
  if(Platform==sConfigRenderDX9)
  {
    static const sChar *profiles[] = { "vs_3_0","??","??","??","ps_3_0","??" };
    bin = sCompileShaderDX(skind,profiles[skind],Out.Get(),&errors);
  }
  else if(Platform==sConfigRenderDX11)
  {
    static const sChar *profiles[] = { "vs_5_0","hs_5_0","ds_5_0","gs_5_0","ps_5_0","cs_5_0" };
    bin = sCompileShaderDX(skind,profiles[skind],Out.Get(),&errors);
  }
  else if(Platform==sConfigRenderNull)
  {
    sU32 *data = new sU32;
    *data = 0xdeadc0de;

    bin = new sShaderBinary();

    bin->Data = (const sU8 *)data;
    bin->Size = 4;
    bin->DeleteData = 1;
    bin->Profile = "null";
    bin->Type = skind;
    bin->CalcHash();
  }
  else if(Platform==sConfigRenderGL2 || Platform==sConfigRenderGLES2)
  {
    const sChar *codesrc = Out.Get();
    sInt codesize = sInt(sGetStringLen(codesrc)+1);
    sU8 *codedest = new sU8[codesize];
    sCopyMem(codedest,codesrc,codesize);


    bin = new sShaderBinary();
    bin->Data = codedest;
    bin->Size = codesize;
    bin->DeleteData = 1;
    bin->Profile = "gl2";
    bin->Type = skind;
    bin->CalcHash();
  }
  else
  {
    Scan.Error(root->Loc,"platform not supported");
  }
  if(bin==0)
  {
    Scan.Error(root->Loc,"shader compiler errors");
    sPrint(errors.Get());
    if(Flags & 1)
    {
      Dump.Print("Shader compiler errors\n");
      Dump.Print(errors.Get());
    }
    return 0;
  }

  // dump binary

  if((Flags & 1) && bin)
  {
    if(Platform==sConfigRenderDX9 || Platform==sConfigRenderDX11)
    {
      sDisassembleShaderDX(bin,&Dump);
      Dump.HexDump32(0,(const sU32 *)bin->Data,bin->Size);
      Dump.Print("\n");
    }
  }

  // every shader only once!

  sShaderBinary *hbin = AllShaders.FindMember(&sShaderBinary::Hash,bin->Hash);
  if(hbin)
  {
    delete bin;
    bin = hbin;
    Dump.Print("shader is a duplicate\n");
  }
  else
  {
    AllShaders.Add(bin);
  }

  // done

  return bin;
}


/****************************************************************************/


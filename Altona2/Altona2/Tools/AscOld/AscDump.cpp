/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "altona2/tools/AscOld/asc.hpp"

using namespace AltonaShaderLanguage;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void Document::DumpLit(wLit *l)
{
  wLit *c;
  switch(l->Kind)
  {
  case LitError:
    Dump.Print("LitError");
    break;
  case LitBool:
  case LitInt:
    Dump.PrintF("%d",l->Int);
    break;
  case LitFloat:
    Dump.PrintF("%f",l->Float);
    break;
  case LitCompound:
    Dump.Print("{");
    sFORALL(l->Members,c)
    {
      if(c!=l->Members.GetFirst())
        Dump.Print(",");
      DumpLit(c);
    }
    Dump.Print("}");
    break;
  }
}


void Document::DumpType(wType *t)
{
  if(!t->DebugName.IsEmpty())
  {
    Dump.Print(t->DebugName);
    return;
  }

  switch(t->Kind)
  {
  case TypeError:
    Dump.Print("TypeError");
    break;
  case TypeVoid:
    Dump.Print("void");
    break;
  case TypeBase:
    switch(t->Token)
    {
    default:
      sASSERT0();
      break;

    case TokBool:             Dump.Print("bool"); break;
    case TokFloat:            Dump.Print("float"); break;
    case TokInt:              Dump.Print("int"); break;
    case TokUInt:             Dump.Print("uint"); break;
    case TokSampler:          Dump.Print("Sampler"); break;
    case TokSampler1D:        Dump.Print("Sampler1D"); break;
    case TokSampler2D:        Dump.Print("Sampler2D"); break;
    case TokSampler3D:        Dump.Print("Sampler3D"); break;
    case TokSamplerCube:      Dump.Print("SamplerCube"); break;
    case TokSamplerState:     Dump.Print("SamplerState"); break;
    case TokSamplercomparisonState: 
                              Dump.Print("SamplerComparisonState"); break;
    case TokTexture:          Dump.Print("Texture"); break;
    case TokTexture1D:        Dump.Print("Texture1D"); break;
    case TokTexture1DArray:   Dump.Print("Texture1DArray"); break;
    case TokTexture2D:        Dump.Print("Texture2D"); break;
    case TokTexture2DArray:   Dump.Print("Texture2Darray"); break;
    case TokTexture2DMS:      Dump.Print("Texture2DMS"); break;
    case TokTexture2DMSArray: Dump.Print("Texture2DMSArray"); break;
    case TokTexture3D:        Dump.Print("Texture3D"); break;
    case TokTextureCube:      Dump.Print("TextureCube"); break;
    case TokTextureCubeArray: Dump.Print("TextureCubeArray"); break;
    }
    break;
  case TypeVector:
    Dump.Print("vector<");
    DumpType(t->Base);
    Dump.PrintF(",%d>",t->Columns);
    break;
  case TypeMatrix:
    Dump.Print("matrix<");
    DumpType(t->Base);
    Dump.PrintF(",%d,%d>",t->Columns,t->Rows);
    break;
  case TypeArray:
    DumpType(t->Base);
    Dump.Print("[");
    DumpExpr(t->Count,0);
    Dump.Print("]");
    break;
  default:
    sASSERT0();
  }
}

void Document::DumpExprBin(const sChar *op,wExpr *a,wExpr *b)
{
  DumpExpr(a,1);
  Dump.Print(op);
  DumpExpr(b,1);
}

void Document::DumpExpr(wExpr *e,sBool brace)
{
  wExpr *arg;
  sInt i=0;
  if(e->Kind==ExprLit || e->Kind==ExprSymbol || e->Kind==ExprCall || e->Kind==ExprCast)
    brace = 0;
  if(brace)
    Dump.Print("(");

  switch(e->Kind)
  {

  case ExprMul:       DumpExprBin("*" ,e->a,e->b); break;
  case ExprDiv:       DumpExprBin("/" ,e->a,e->b); break;
  case ExprMod:       DumpExprBin("%" ,e->a,e->b); break;
  case ExprAdd:       DumpExprBin("+" ,e->a,e->b); break;
  case ExprSub:       DumpExprBin("-" ,e->a,e->b); break;
  case ExprShiftL:    DumpExprBin("<<",e->a,e->b); break;
  case ExprShiftR:    DumpExprBin(">>",e->a,e->b); break;
  case ExprGT:        DumpExprBin(">" ,e->a,e->b); break;
  case ExprLT:        DumpExprBin("<" ,e->a,e->b); break;
  case ExprGE:        DumpExprBin(">=",e->a,e->b); break;
  case ExprLE:        DumpExprBin("<=",e->a,e->b); break;
  case ExprEQ:        DumpExprBin("==",e->a,e->b); break;
  case ExprNE:        DumpExprBin("!=",e->a,e->b); break;
  case ExprAnd:       DumpExprBin("&" ,e->a,e->b); break;
  case ExprOr:        DumpExprBin("|" ,e->a,e->b); break;
  case ExprEor:       DumpExprBin("^" ,e->a,e->b); break;
  case ExprLAnd:      DumpExprBin("&&",e->a,e->b); break;
  case ExprLOr:       DumpExprBin("||",e->a,e->b); break;
  case ExprAssign:    DumpExprBin("=" ,e->a,e->b); break;
  case ExprAssignAdd: DumpExprBin("+=",e->a,e->b); break;
  case ExprAssignSub: DumpExprBin("-=",e->a,e->b); break;
  case ExprAssignMul: DumpExprBin("*=",e->a,e->b); break;
  case ExprAssignDiv: DumpExprBin("/=",e->a,e->b); break;
  case ExprAssignMod: DumpExprBin("%=",e->a,e->b); break;
    break;

  case ExprError: 
    Dump.Print("ExprError"); 
    break;

  case ExprCond:      
    DumpExpr(e->c,1);
    Dump.Print("?");
    DumpExpr(e->b,1);
    Dump.Print(":");
    DumpExpr(e->a,1);
    break;

  case ExprNot:
    Dump.Print("!");
    DumpExpr(e->a,1);
    break;
  case ExprNeg:
    Dump.Print("-");
    DumpExpr(e->a,1);
    break;
  case ExprComplement:
    Dump.Print("~");
    DumpExpr(e->a,1);
    break;
  case ExprPreInc:
    Dump.Print("++");
    DumpExpr(e->a,1);
    break;
  case ExprPreDec:
    Dump.Print("--");
    DumpExpr(e->a,1);
    break;
  case ExprPostInc:
    DumpExpr(e->a,1);
    Dump.Print("++");
    break;
  case ExprPostDec:
    DumpExpr(e->a,1);
    Dump.Print("--");
    break;
  case ExprLit:
    DumpLit(e->Literal);
    break;
  case ExprIndex:
    DumpExpr(e->a,1);
    Dump.Print("[");
    DumpExpr(e->b,0);
    Dump.Print("]");
    break;
  case ExprSymbol:
    Dump.Print(e->Name);
    break;
  case ExprMember:
    DumpExpr(e->a,1);
    Dump.PrintF(".%s",e->Name);
    break;
  case ExprCast:
    DumpType(e->Type);
    Dump.Print("(");
    sFORALL(e->Args,arg)
    {
      if(i)
        Dump.Print(",");
      i = 1;
      DumpExpr(arg,0);
    }
    Dump.Print(")");
    break;
  case ExprCall:
    DumpExpr(e->a,1);
    Dump.Print("(");
    sFORALL(e->Args,arg)
    {
      if(i)
        Dump.Print(",");
      i = 1;
      DumpExpr(arg,0);
    }
    Dump.Print(")");
    break;
  }

  if(brace)
    Dump.Print(")");
}

void Document::DumpStmt(wStmt *s,sInt indent)
{
  wStmt *n;
  sBool br=0;
  switch(s->Kind)
  {
  case StmtError:
    Dump.PrintF("%_StmtError\n",indent*2);
    break;
  case StmtNop:
    break;
  case StmtExpr:
    Dump.PrintF("%_",indent*2);
    DumpExpr(s->Expr,0);
    Dump.Print(";\n");
    break;
  case StmtObject:
    Dump.PrintF("%_",indent*2);
    DumpType(s->Object->Type);
    Dump.PrintF(" %s",s->Object->Name);
    if(s->Object->Initializer)
    {
      Dump.PrintF(" = ");
      DumpExpr(s->Object->Initializer,0);
    }
    Dump.Print(";\n");
    break;
  case StmtBlock:
    n = s->Childs;    
    br = (n==0 || n->Next!=0 || indent==0);
    if(br)
      Dump.PrintF("%_{\n",indent*2);
    while(n)
    {
      DumpStmt(n,indent+1);
      n = n->Next;
    }
    if(br)
      Dump.PrintF("%_}\n",indent*2);
    break;

  case StmtBreak:
    Dump.PrintF("%_break;\n",indent*2);
    break;
  case StmtContinue:
    Dump.PrintF("%_continue;\n",indent*2);
    break;
  case StmtDiscard:
    Dump.PrintF("%_discard;\n",indent*2);
    break;
  case StmtDo:
    Dump.PrintF("%_do\n",indent*2);
    DumpStmt(s->Childs,indent);
    break;
  case StmtFor:
    Dump.PrintF("%_for(",indent*2);
    DumpExpr(s->Expr->a,0);
    Dump.Print(";");
    DumpExpr(s->Expr->b,0);
    Dump.Print(";");
    DumpExpr(s->Expr->c,0);
    Dump.Print(")\n");
    break;
  case StmtIf:
    Dump.PrintF("%_if(",indent*2);
    DumpExpr(s->Expr,0);
    Dump.Print(")\n");
    DumpStmt(s->Childs,indent);
    if(s->Alt)
    {
      Dump.PrintF("%_else\n",indent*2);
      DumpStmt(s->Alt,indent);
    }
    break;
  case StmtPif:
    Dump.PrintF("%_pif(",indent*2);
    DumpExpr(s->Expr,0);
    Dump.Print(")\n");
    DumpStmt(s->Childs,indent);
    if(s->Alt)
    {
      Dump.PrintF("%_else\n",indent*2);
      DumpStmt(s->Alt,indent);
    }
    break;
  case StmtReturn:
    Dump.PrintF("%_return",indent*2);
    if(s->Expr)
    {
      Dump.Print(" ");
      DumpExpr(s->Expr,0);
    }
    Dump.Print("\n");
    break;
  case StmtTypedef:
    Dump.PrintF("%_typedef ",indent*2);
    DumpType(s->Expr->Type);
    Dump.PrintF(" %s;\n",s->Expr->Name);
    break;
  case StmtWhile:
    Dump.PrintF("%_while(",indent*2);
    DumpExpr(s->Expr,0);
    Dump.Print(")\n");
    DumpStmt(s->Childs,indent);
    break;
  }
}

void Document::DumpMtrl()
{
  wPermute *per;
  wPermuteOption *opt;
  wInvariant *inv;
  sInt n,i;

  Dump.Print("/****************************************************************************/\n");
  Dump.PrintF("Material %s\n",Material->Name);
  Dump.Print("Permutations:\n");
  
  n = 1;
  sFORALL(Material->Permutes,per)
  {
    Dump.PrintF("  %s : %08x\n",per->Name,per->Mask);
    i = 0;
    sFORALL(per->Options,opt)
    {
      i++;
      Dump.PrintF("    %s : %08x\n",opt->Name,opt->Value);
    }
    if(i==0)
      n*=2;
    else
      n*=i;
  }
  Dump.PrintF("total permutations: %d\n\n",n);
  sFORALL(Material->Invariants,inv)
  {
    if(inv->Condition)
    {
      Dump.Print("pif(");
      DumpExpr(inv->Condition,0);
      Dump.Print(") ");
    }

    if(inv->KindMask & InvIA2VS) Dump.Print("ia2vs ");
    if(inv->KindMask & InvVS2RS) Dump.Print("vs2rs ");
    if(inv->KindMask & InvVS2PS) Dump.Print("vs2ps ");
    if(inv->KindMask & InvPS2OM) Dump.Print("ps2om ");
    if(inv->KindMask & InvVS2GS) Dump.Print("vs2gs ");
    if(inv->KindMask & InvGS2PS) Dump.Print("gs2ps ");
    if(inv->KindMask & InvGS2RS) Dump.Print("gs2rs ");
    DumpType(inv->Type);
    Dump.PrintF(" %s",inv->Name);
    if(!inv->Semantic.IsEmpty())
      Dump.PrintF(" : %s",inv->Semantic);
    Dump.Print(";\n");
  }
  Dump.Print("\n");
  for(sInt i=0;i<sST_Max;i++)
  {
    if(Material->Shaders[i])
    {
      static const sChar *ShaderNames[] =
      { "vs","hs","ds","gs","ps","cs" };
      Dump.PrintF("%s\n",ShaderNames[i]);
      DumpStmt(Material->Shaders[i],0);
      Dump.Print("\n");
    }
  }
}

/****************************************************************************/


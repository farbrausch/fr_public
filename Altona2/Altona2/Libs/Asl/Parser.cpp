/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Parser.hpp"

using namespace Altona2;
using namespace Asl;

enum ParseTokens
{
    TOK_If = 0x100,
    TOK_Else,
    TOK_While,
    TOK_Do,
    TOK_For,
    TOK_Switch,
    TOK_Case,
    TOK_Default,
    TOK_Return,
    TOK_Break,
    TOK_Continue,
    TOK_Discard,
    TOK_Inject,
    TOK_Cif,

    TOK_True,
    TOK_False,
    TOK_Inc,
    TOK_Dec,
    TOK_AssignAdd,
    TOK_AssignSub,
    TOK_AssignMul,
    TOK_AssignDiv,
    TOK_AssignMod,
    TOK_AssignShiftL,
    TOK_AssignShiftR,
    TOK_AssignBAnd,
    TOK_AssignBOr,
    TOK_AssignBEor,


    TOK_Func,
    TOK_DotProduct,
};

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

bool sParse::IsType()
{
    if(Scan->Token==sTOK_Name)
    {
        if(sCmpString(Scan->Name,"row_major")==0) return true;
        if(sCmpString(Scan->Name,"column_major")==0) return true;
        if(sCmpString(Scan->Name,"lowp")==0) return true;
        if(sCmpString(Scan->Name,"mediump")==0) return true;
        if(sCmpString(Scan->Name,"highp")==0) return true;
        if(sCmpString(Scan->Name,"const")==0) return true;
        if(sCmpString(Scan->Name,"groupshared")==0) return true;
        if(sCmpString(Scan->Name,"static")==0) return true;
        if(sCmpString(Scan->Name,"precise")==0) return true;
        if(sCmpString(Scan->Name,"in")==0) return true;
        if(sCmpString(Scan->Name,"out")==0) return true;
        if(sCmpString(Scan->Name,"linear")==0) return true;
        if(sCmpString(Scan->Name,"nointerpolation")==0) return true;
        if(sCmpString(Scan->Name,"centroid")==0) return true;
        if(sCmpString(Scan->Name,"noperspective")==0) return true;
        if(sCmpString(Scan->Name,"sample")==0) return true;
        if(sCmpString(Scan->Name,"import")==0) return true;
        if(sCmpString(Scan->Name,"export")==0) return true;
        if(sCmpString(Scan->Name,"numthreads")==0) return true;
    }

    if(Scan->Token==sTOK_Name)
        return Tree->FindType(Scan->Name)!=0;
    return false;
}

sType *sParse::_Type()
{
    sPoolString name = Scan->ScanName();

    auto type = Tree->FindType(name);
    if(!type)
    {
        Scan->Error("can't find type %q",name);
        type = Tree->MakeType(sTK_Error);
    }
    return type;
}

sLiteral *sParse::_Literal()
{
    sLiteral *lit = 0;
    switch(Scan->Token)
    {
    case TOK_True:
        Scan->Scan();
        lit = Tree->MakeLiteral(Tree->MakeType(sTK_Bool));
        *lit->DataU = 1;
        break;
    case TOK_False:
        Scan->Scan();
        lit = Tree->MakeLiteral(Tree->MakeType(sTK_Bool));
        *lit->DataU = 0;
        break;
    case sTOK_Int:
        lit = Tree->MakeLiteral(Tree->MakeType(sTK_Int));
        *lit->DataU = Scan->ScanInt();
        break;
    case sTOK_Float:
        lit = Tree->MakeLiteral(Tree->MakeType(sTK_Float));
        *lit->DataF = Scan->ScanFloat();
        break;
    default:
        Scan->Error("literal expected");
    }
    return lit;
}

sExpr *sParse::_Intrinsic(sIntrinsic *in)
{
    auto e = Tree->MakeExpr(sEX_Intrinsic);
    e->Intrinsic = in;

    Scan->Match('(');
    while(!Scan->Errors && Scan->Token!=')')
    {
        e->Args.Add(_Expr());
        if(!Scan->IfToken(','))
            break;
    }
    Scan->Match(')');

    if(e->Args.GetCount()<in->MinArgCount || e->Args.GetCount()>in->ArgCount)
    {
        Scan->Error("%q requires %d to %d paramters",in->Name,in->MinArgCount,in->ArgCount);
        return 0;
    }

    if(Scan->Errors) return 0;

    // use typemask matching

    if(in->Flags & sTAF_ArgAnyType)
    {
        sTypeKind typekind = sTK_Error;

             if(in->TypeMask & sBT_Float) typekind = sTK_Float;
        else if(in->TypeMask & sBT_Uint ) typekind = sTK_Uint ;
        else if(in->TypeMask & sBT_Int  ) typekind = sTK_Int  ;
        else if(in->TypeMask & sBT_Bool ) typekind = sTK_Bool ;
        else sASSERT0();

        if(e->Args[0]->Type->Kind==sTK_Vector)
            e->Type = Tree->MakeVector(typekind,e->Args[0]->Type->SizeX);
        else if(e->Args[0]->Type->Kind==sTK_Matrix)
            e->Type = Tree->MakeMatrix(typekind,e->Args[0]->Type->SizeX,e->Args[0]->Type->SizeY);
        else if(e->Args[0]->Type->IsBaseType())
            e->Type = Tree->MakeType(typekind);
        else
            Scan->Error("illegal type for conversion operator");
    }
    else
    {
        if(in->TypeMask && e->Args.GetCount()>0)
        {
            if(in->TypeMask & sBT_VectorPromote && e->Args.GetCount()==2)
            {
                Tree->ExpandVector(e->Args[0],e->Args[1]);
                Tree->ExpandVector(e->Args[1],e->Args[0]);
            }

            auto type = e->Args[0]->Type;
            sTypeKind basetype = type->Kind;
            if(basetype==sTK_Vector) basetype = type->Base->Kind;
            if(basetype==sTK_Matrix) basetype = type->Base->Kind;

            if(basetype==sTK_Float  && !(in->TypeMask & sBT_Float )) Scan->Error("float not allowed as argument for %q",in->Name);
            if(basetype==sTK_Int    && !(in->TypeMask & sBT_Int   )) Scan->Error("int not allowed as argument for %q",in->Name);
            if(basetype==sTK_Uint   && !(in->TypeMask & sBT_Uint  )) Scan->Error("uint not allowed as argument for %q",in->Name);
            if(basetype==sTK_Bool   && !(in->TypeMask & sBT_Bool  )) Scan->Error("bool not allowed as argument for %q",in->Name);
            if(basetype==sTK_Double && !(in->TypeMask & sBT_Double)) Scan->Error("double not allowed as argument for %q",in->Name);

            if(Scan->Errors) return 0;

            // assume here, we are either scalar, sTK_Vector or sTK_Matrix

            bool ok = false;

            if((in->TypeMask & sBT_Float)        && type->Kind==sTK_Float)
                ok = true;
            if((in->TypeMask & sBT_Uint)         && type->Kind==sTK_Uint)
                ok = true;
            if((in->TypeMask & sBT_Int)          && type->Kind==sTK_Int)
                ok = true;
            if((in->TypeMask & sBT_Bool)         && type->Kind==sTK_Bool)
                ok = true;
            if((in->TypeMask & sBT_Vector)       && type->Kind==sTK_Vector)
                ok = true;
            if((in->TypeMask & sBT_Vector3)      && type->Kind==sTK_Vector && type->SizeX==3)
                ok = true;
            if((in->TypeMask & sBT_Matrix)       && type->Kind==sTK_Matrix)
                ok = true;
            if((in->TypeMask & sBT_SquareMatrix) && type->Kind==sTK_Matrix && type->SizeX==type->SizeY)
                ok = true;
            if((in->TypeMask & sBT_Scalar)       && type->Kind!=sTK_Vector && type->Kind!=sTK_Matrix)
                ok = true;

            if(!ok)
            {
                Scan->Error("type incompatible for arguments in %q",in->Name);
                return 0;
            }
        }

        for(int i=0;i<e->Args.GetCount();i++)
        {
            sType *cmptype = in->ArgType[i] ? in->ArgType[i] : e->Args[0]->Type;
            Tree->ConvertIntLit(e->Args[i],cmptype);
            Tree->ExpandVector(e->Args[i],cmptype);
            if(!e->Args[i]->Type->Compatible(cmptype))
            {
                Scan->Error("argument %s has wrong type",i+1);
                return 0;
            }
        }

        e->Type = in->ResultType;
        if(in->Flags & sTAF_ResultVoid)
            e->Type = Tree->FindType("void");
        if(!e->Type)
            e->Type = e->Args[0]->Type;

        if(in->Flags & sTAF_ResultBool)
            e->Type = Tree->FindType("bool");
        if((in->Flags & sTAF_ResultScalar) && (e->Type->Kind==sTK_Vector || e->Type->Kind==sTK_Matrix))
            e->Type = e->Type->Base;
        if(in->Flags & sTAF_MulSpecial)
            sASSERT0();
        sASSERT(e->Type);
    }


    return e;
}

sExpr *sParse::_NameExpr()
{
    sPoolString name = Scan->ScanName();

    // search in scope / variables

    auto scope = Scope;
    sVariable *var = 0;
    while(scope && !var)
    {
        var = scope->Vars.Find([=](const sVariable *v){ return v->Name==name; });
        scope = scope->ParentScope;
    }

    if(!var)
        var = Tree->Vars.Find([=](const sVariable *v){ return v->Name==name; });
    if(!var && Tree->CurrentModule)
        var = Tree->CurrentModule->Vars.Find([=](const sVariable *v){ return v->Name==name; });

    if(var)
    {
        sExpr *e = Tree->MakeExpr(sEX_Variable);
        e->Variable = var;
        e->Type = var->Type;
        return e;
    }

    // search in constant buffers

    for(auto type : Tree->Types)
    {
        if(type->Kind==sTK_CBuffer || type->Kind==sTK_TBuffer)
        {
            auto m = Tree->FindMember(name,type);
            if(m)
            {
                if(m->Name==name)
                {
                    auto e = Tree->MakeExpr(sEX_Member);
                    e->Type = m->Type;
                    e->Member = m;
                    return e;
                }
            }
        }
    }

    // intrinsic functions

    for(auto in : Tree->Intrinsics)
    {
        if(in->Name==name)// && !(in->Flags & sTAF_ResultVoid))
        {
            auto e = _Intrinsic(in);
            if(e)
                return e;
        }
    }

    // explicit function
    
    for(auto func : Tree->Funcs)
    {
        if(func->Name==name && func->Result->Kind!=sTK_Void)
        {
            auto e = Tree->MakeExpr(sEX_Call);
            e->FuncName = func->Name;
            e->Type = func->Result;
            Scan->Match('(');
            if(Scan->Token!=')')
            {
                do
                {
                    e->Args.Add(_Expr());
                }
                while(Scan->IfToken(','));
            }
            Scan->Match(')');
            return e;
        }
    }

    // type constructur

    auto type = Tree->FindType(name);
    if(type && (type->Kind==sTK_Vector || type->Kind==sTK_Matrix))
    {
        auto e = Tree->MakeExpr(sEX_Construct);
        e->Type = type;

        Scan->Match('(');
        while(!Scan->Errors && Scan->Token!=')')
        {
            e->Args.Add(_Expr());
            if(!Scan->IfToken(','))
                break;
        }
        Scan->Match(')');

        if(type->Kind==sTK_Vector)
        {
            int n = 0;
            for(auto a : e->Args)
            {
                Tree->ConvertIntLit(a,type->Base);
                if(a->Type->Kind==type->Base->Kind)
                {
                    n++;
                }
                else if(a->Type->Kind==sTK_Vector && a->Type->Base->Kind==type->Base->Kind)
                {
                    n += a->Type->SizeX;
                }
                else
                {
                    Scan->Error("type mismatch");
                }
            }
            if(n!=type->SizeX)
                Scan->Error("vector size %d expected, %d found",type->SizeX,n);
        }
        if(type->Kind==sTK_Matrix)
        {
            int n = 0;
            for(auto a : e->Args)
            {
                if(a->Type->Kind==sTK_Vector && a->Type->Base->Kind==type->Base->Kind)
                {
                    if(a->Type->SizeX!=type->SizeX)
                        Scan->Error("vector size does not match matrix size");
                    n++;
                }
                else
                {
                    Scan->Error("type mismatch");
                }
            }
            if(n!=type->SizeY)
                Scan->Error("matrix size %d expected, %d found",type->SizeY,n);
        }
        return e;
    }

    // cif condition

    if(Tree->CurrentModule)
    {
        sPoolString member;
        if(Scan->IfToken('.'))
            member = Scan->ScanName();
        for(auto &c : Tree->CurrentModule->Conditions)
        {
            if(c.Name == name && c.Member==member)
            {
                sExpr *e = Tree->MakeExpr(sEX_Condition);
                e->CondName = name;
                e->CondMember = member;
                switch(c.TypeKind)
                {
                case sTK_Bool: e->Type = Tree->FindType("bool"); break;
                case sTK_Int: e->Type = Tree->FindType("int"); break;
                case sTK_Uint: e->Type = Tree->FindType("uint"); break;
                default: sASSERT0();
                }
                return e;
            }
        }

        // here we have parsed '.', so we must FAIL!!!
    }

    // done

    Scan->Error("can't find name %q",name);

    return Tree->ErrorExpr;
}

sExpr *sParse::_Value()
{
    sExpr *e = 0;
    if(Scan->IfToken('-'))
    {
        e = Tree->MakeExpr(sEX_Neg,_Value());
        if(!Scan->Errors)
            if(!e->Args[0]->Type->CheckBaseType(sBT_Int|sBT_Uint|sBT_Float|sBT_Double))
                Scan->Error("illegal type");
        return e;
    }
    if(Scan->IfToken('+'))
    {
        e = Tree->MakeExpr(sEX_Pos,_Value());
        if(!Scan->Errors)
            if(!e->Args[0]->Type->CheckBaseType(sBT_Int|sBT_Uint|sBT_Float|sBT_Double))
                Scan->Error("illegal type");
        return e;
    }
    if(Scan->IfToken('!'))
    {
        e = Tree->MakeExpr(sEX_Not,_Value());
        if(!Scan->Errors)
            if(!e->Args[0]->Type->CheckBaseType(sBT_Int|sBT_Uint|sBT_Float|sBT_Double|sBT_Bool))
                Scan->Error("illegal type");
        return e;
    }
    if(Scan->IfToken('~'))
    {
        e = Tree->MakeExpr(sEX_Complement,_Value());
        if(!Scan->Errors)
            if(!e->Args[0]->Type->CheckBaseType(sBT_Int|sBT_Uint|sBT_Float|sBT_Double))
                Scan->Error("illegal type");
        return e;
    }
    if(Scan->IfToken(TOK_Inc))
    {
        e = Tree->MakeExpr(sEX_PreInc,_Value());
        if(!Scan->Errors)
            if(!e->Args[0]->Type->CheckBaseType(sBT_Int|sBT_Uint|sBT_Float|sBT_Double))
                Scan->Error("illegal type");
        return e;
    }
    if(Scan->IfToken(TOK_Dec))
    {
        e = Tree->MakeExpr(sEX_PreDec,_Value());
        if(!Scan->Errors)
            if(!e->Args[0]->Type->CheckBaseType(sBT_Int|sBT_Uint|sBT_Float|sBT_Double))
                Scan->Error("illegal type");
        return e;
    }

    switch(Scan->Token)
    {
    case sTOK_Int:
    case sTOK_Float:
    case TOK_True:
    case TOK_False:
        e = Tree->MakeExpr(sEX_Literal);
        e->Literal = _Literal();
        e->Type = e->Literal ? e->Literal->Type : 0;
        break;

    case sTOK_Name:
        e = _NameExpr();
        break;

    case '(':
        Scan->Match('(');
        if(IsType())            // cast
        {
            e = Tree->MakeExpr(sEX_Cast);
            e->Type = _Type();
            Scan->Match(')');
            e->Args.Add(_Value());          // cast has high priority, so use _Value(), not _Expr().
        }
        else
        {
            e = _Expr();
            Scan->Match(')');
        }
        break;

    default:
        Scan->Error("value expected");
        e = Tree->MakeExpr(sEX_Error);
        break;
    }

more:


    if(Scan->IfToken('['))
    {
        e = Tree->MakeExpr(sEX_Index,e,_Expr());
        if(e->Args[1]->Type->Kind!=sTK_Int && e->Args[1]->Type->Kind!=sTK_Uint)
            Scan->Error("index type must be int or uint");
        e->Type = e->Args[0]->Type->TemplateType ? e->Args[0]->Type->TemplateType : e->Args[0]->Type->Base;
        Scan->Match(']');
        goto more;
    }

    while(Scan->Token=='.' && !e->Type->Members.IsEmpty())
    {
        Scan->Scan();
        sPoolString mn = Scan->ScanName();
        auto member = Tree->FindMember(mn,e->Type);
        if(!member)
        {
            Scan->Error("can't find member %q",mn);
        }
        else
        {
            e = Tree->MakeExpr(sEX_Member,e);
            e->Member = member;
            e->Type = member->Type;
        }
        goto more;
    }

    if(Scan->Token=='.' && !e->Type->Intrinsics.IsEmpty())
    {
        Scan->Scan();
        sPoolString name = Scan->ScanName();
        auto in = e->Type->Intrinsics.Find([=](const sIntrinsic *i) { return i->Name==name; });
        if(in)
        {
            auto ie = _Intrinsic(in);
            if(ie)
            {
                e = Tree->MakeExpr(sEX_Member,e,ie);
                e->Type = ie->Type;
            }
            goto more;
        }
        Scan->Error("Intrinsic .%s not found",name);
    }

    if(Scan->Token=='.' && e->Type->Kind==sTK_Vector)
    {
        Scan->Scan();
        sString<128> swizzle = Scan->ScanName().Get();
        const char *sw = swizzle;
        uint mask = 0;
        int index = 0;
        int maxindex = 0;

        while(sw[index])
        {
            switch(sw[index])
            {
                case 'x': mask |= 1 << (8*index); break;
                case 'y': mask |= 2 << (8*index); maxindex = sMax(maxindex,1); break;
                case 'z': mask |= 3 << (8*index); maxindex = sMax(maxindex,2); break;
                case 'w': mask |= 4 << (8*index); maxindex = sMax(maxindex,3); break;
                case '0': mask |= 5 << (8*index); break;
                case '1': mask |= 6 << (8*index); break;
                default: Scan->Error("illegal swizzle %q",sw); break;
            }
            index++;
        }
        if(index>4)
            Scan->Error("illegal swizzle %q",sw);
        if(maxindex > e->Type->SizeX)
            Scan->Error("swizzle %q out of range",sw);
        auto type = e->Type;
        e = Tree->MakeExpr(sEX_Swizzle,e);
        e->Type = index==1 ? type->Base : Tree->MakeVector(type->Base->Kind,index);
        e->Swizzle = mask;
        goto more;
    }

    if(Scan->IfToken(TOK_Inc))
    {
        e = Tree->MakeExpr(sEX_PostInc,e);
        goto more;
    }
    if(Scan->IfToken(TOK_Dec))
    {
        e = Tree->MakeExpr(sEX_PostDec,e);
        goto more;
    }

    return e;
}

sExpr *sParse::_ExprR(int maxlevel)
{
    sInt pri,mode,level;
    sExprKind kind;
    sExpr *expr; 

    expr = _Value();

    for(level=1;level<=maxlevel;level++)
    {
        while(!Scan->Errors)
        {
            kind = sEX_Error;
            pri = 0;
            mode = 0;
            switch(Scan->Token)
            {
            case '*':               pri= 1; mode=0x06; kind=sEX_Mul;    break;
            case '/':               pri= 1; mode=0x00; kind=sEX_Div;    break;
            case '%':               pri= 1; mode=0x00; kind=sEX_Mod;    break;
            case TOK_DotProduct:    pri= 1; mode=0x07; kind=sEX_Dot;    break;

            case '+':               pri= 2; mode=0x00; kind=sEX_Add;    break;
            case '-':               pri= 2; mode=0x00; kind=sEX_Sub;    break;

            case sTOK_ShiftL:       pri= 3; mode=0x03; kind=sEX_ShiftL; break;
            case sTOK_ShiftR:       pri= 3; mode=0x03; kind=sEX_ShiftR; break;

            case '>':               pri= 4; mode=0x01; kind=sEX_GT;     break;
            case '<':               pri= 4; mode=0x01; kind=sEX_LT;     break;
            case sTOK_GE:           pri= 4; mode=0x01; kind=sEX_GE;     break;
            case sTOK_LE:           pri= 4; mode=0x01; kind=sEX_LE;     break;

            case sTOK_EQ:           pri= 5; mode=0x02; kind=sEX_EQ;     break;
            case sTOK_NE:           pri= 5; mode=0x02; kind=sEX_NE;     break;

            case '&':               pri= 6; mode=0x03; kind=sEX_BAnd;    break;
            case '|':               pri= 7; mode=0x03; kind=sEX_BOr;     break;
            case '^':               pri= 8; mode=0x03; kind=sEX_BEor;    break;

            case sTOK_DoubleAnd:    pri= 9; mode=0x04; kind=sEX_LAnd;   break;
            case sTOK_DoubleOr:     pri=10; mode=0x04; kind=sEX_LOr;    break;

            case '?':               pri=11; mode=0x00; kind=sEX_Cond;         break;
            case '=':               pri=12; mode=0x05; kind=sEX_Assign;       break;
            case TOK_AssignAdd:     pri=12; mode=0x05; kind=sEX_AssignAdd;    break;
            case TOK_AssignSub:     pri=12; mode=0x05; kind=sEX_AssignSub;    break;
            case TOK_AssignMul:     pri=12; mode=0x05; kind=sEX_AssignMul;    break;
            case TOK_AssignDiv:     pri=12; mode=0x05; kind=sEX_AssignDiv;    break;
            case TOK_AssignMod:     pri=12; mode=0x05; kind=sEX_AssignMod;    break;
            case TOK_AssignShiftL:  pri=12; mode=0x05; kind=sEX_AssignShiftL; break;
            case TOK_AssignShiftR:  pri=12; mode=0x05; kind=sEX_AssignShiftR; break;
            case TOK_AssignBAnd:    pri=12; mode=0x05; kind=sEX_AssignBAnd;   break;
            case TOK_AssignBOr:     pri=12; mode=0x05; kind=sEX_AssignBOr;    break;
            case TOK_AssignBEor:    pri=12; mode=0x05; kind=sEX_AssignBEor;   break;
            }

            if(kind==sEX_Error || pri!=level)
                break;
            Scan->Scan();

            if(!Scan->Errors)
            {
                if(mode==0x05)
                {
                    expr = Tree->MakeExpr(kind,expr,_ExprR(level));    // (a = (b = c))
                }
                else
                {
                    expr = Tree->MakeExpr(kind,expr,_ExprR(level-1));
                    if(kind==sEX_Cond)
                    {
                        Scan->Match(':');
                        expr->Args.Add(_ExprR(level-1));

                        // x ? a : b -> abx

                                                // xab
                        expr->Args.Swap(0,2);   // bax
                        expr->Args.Swap(0,1);   // abx

                        if(!expr->Args[2]->Type->CheckBaseType(sBT_Bool))
                            Scan->Error("first argument of ?: operator must be bool");
                        expr->Type = expr->Args[0]->Type;
                    }
                }
            }
            if(!Scan->Errors)
            {
                sType *ta = 0;
                sType *tb = 0;
                switch(mode)
                {
                default:
                    sASSERT0();

                case 0x00:      // all types
                    Tree->ExpandVector(expr->Args[0],expr->Args[1]);
                    Tree->ExpandVector(expr->Args[1],expr->Args[0]);
                    ta = expr->Args[0]->Type;
                    tb = expr->Args[1]->Type;
                    if(!ta->CheckBaseType(sBT_Float|sBT_Int|sBT_Uint|sBT_Bool|sBT_Double))
                        Scan->Error("type mismatch");
                    if(!ta->Compatible(tb))
                        Scan->Error("type mismatch");
                    expr->Type = ta;
                    break;
                case 0x01:      // input non bool, output bool
                    Tree->ExpandVector(expr->Args[0],expr->Args[1]);
                    Tree->ExpandVector(expr->Args[1],expr->Args[0]);
                    ta = expr->Args[0]->Type;
                    tb = expr->Args[1]->Type;
                    if(!ta->CheckBaseType(sBT_Float|sBT_Double|sBT_Uint|sBT_Int))
                        Scan->Error("type mismatch");
                    if(!ta->Compatible(tb))
                        Scan->Error("type mismatch");
                    expr->Type = Tree->SwitchBaseType(ta,sTK_Bool);
                    break;
                case 0x02:      // all types, output bool
                    Tree->ExpandVector(expr->Args[0],expr->Args[1]);
                    Tree->ExpandVector(expr->Args[1],expr->Args[0]);
                    ta = expr->Args[0]->Type;
                    tb = expr->Args[1]->Type;
                    if(!ta->CheckBaseType(sBT_Float|sBT_Int|sBT_Uint|sBT_Bool|sBT_Double))
                        Scan->Error("type mismatch");
                    if(!ta->Compatible(tb))
                        Scan->Error("type mismatch");
                    expr->Type = Tree->SwitchBaseType(ta,sTK_Bool);
                    break;
                case 0x03:      // uint && in
                    Tree->ExpandVector(expr->Args[0],expr->Args[1]);
                    Tree->ExpandVector(expr->Args[1],expr->Args[0]);
                    ta = expr->Args[0]->Type;
                    tb = expr->Args[1]->Type;
                    if(!ta->CheckBaseType(sBT_Uint|sBT_Int))
                        Scan->Error("type mismatch");
                    if(!ta->Compatible(tb))
                        Scan->Error("type mismatch");
                    expr->Type = ta;
                    break;
                case 0x04:      // bool
                    Tree->ExpandVector(expr->Args[0],expr->Args[1]);
                    Tree->ExpandVector(expr->Args[1],expr->Args[0]);
                    ta = expr->Args[0]->Type;
                    tb = expr->Args[1]->Type;
                    if(!ta->CheckBaseType(sBT_Bool))
                        Scan->Error("type mismatch");
                    if(!ta->Compatible(tb))
                        Scan->Error("type mismatch");
                    expr->Type = ta;
                    break;
                case 0x05:      // assign
                    Tree->ExpandVector(expr->Args[1],expr->Args[0]);
                    ta = expr->Args[0]->Type;
                    tb = expr->Args[1]->Type;
                    if(!ta->Compatible(tb))
                        Scan->Error("type mismatch");
                    expr->Type = ta;
                    break;
                case 0x06:      // mul special
                    ta = expr->Args[0]->Type;
                    tb = expr->Args[1]->Type;
                    if(ta->Kind==sTK_Matrix && tb->Kind==sTK_Matrix)
                    {
                        if(ta->Base->Kind!=tb->Base->Kind)
                            Scan->Error("type mismatch");
                        if(ta->SizeX!=tb->SizeY)
                            Scan->Error("type mismatch");
                        expr->Type = Tree->MakeMatrix(ta->Base->Kind,tb->SizeX,ta->SizeY);
                    }
                    else if(ta->Kind==sTK_Vector && tb->Kind==sTK_Matrix)
                    {
                        if(ta->Base->Kind!=tb->Base->Kind)
                            Scan->Error("type mismatch");
                        if(ta->SizeX!=tb->SizeY)
                            Scan->Error("type mismatch");
                        expr->Type = Tree->MakeVector(ta->Base->Kind,ta->SizeX);
                    }
                    else if(ta->Kind==sTK_Matrix && tb->Kind==sTK_Vector)
                    {
                        if(ta->Base->Kind!=tb->Base->Kind)
                            Scan->Error("type mismatch");
                        if(ta->SizeX!=tb->SizeX)
                            Scan->Error("type mismatch");
                        expr->Type = Tree->MakeVector(ta->Base->Kind,ta->SizeY);
                    }
                    else
                    {
                        Tree->ExpandVector(expr->Args[0],expr->Args[1]);
                        Tree->ExpandVector(expr->Args[1],expr->Args[0]);
                        ta = expr->Args[0]->Type;
                        tb = expr->Args[1]->Type;
                        if(!expr->Args[0]->Type->Compatible(expr->Args[1]->Type))
                            Scan->Error("type mismatch");
                        expr->Type = ta;
                    }
                    break;
                case 0x07:      // dot
                    ta = expr->Args[0]->Type;
                    tb = expr->Args[1]->Type;
                    if(!ta->CheckBaseType(sBT_Float))
                        Scan->Error("type mismatch");
                    if(!ta->Compatible(tb))
                        Scan->Error("type mismatch");
                    if(ta->Kind!=sTK_Vector || tb->Kind!=sTK_Vector)
                        Scan->Error("type mismatch");
                    else
                        expr->Type = ta->Base;
                    break;
                }
            }
        }
    }

    return expr;
}

sExpr *sParse::_Expr()
{
    return _ExprR(12);
}

sVariable *sParse::_Decl(bool initializer,bool local)
{
    sStorage store = _Storage();
    auto type = _Type();

    if(type && type->HasTemplate)
    {
        auto oldtype = type;
        type = Tree->MakeType(oldtype->Kind);
        type->Base = oldtype->Base;
        type->Name = oldtype->Name;
        type->HasTemplate = oldtype->HasTemplate;
        Scan->Match('<');
        type->TemplateType = _Type();
        Scan->Match('>');
    }

    sPoolString name = Scan->ScanName();

    sArray<sExpr *> arrays;
    while(Scan->IfToken('['))
    {
        arrays.Add(_Expr());
        Scan->Match(']');
    }
    for(int i=arrays.GetCount()-1;i>=0;i--)
        type = Tree->MakeArray(type,arrays[i]);


    sSemantic sem = _Semantic();

    auto var = Tree->MakeVar(type,name,store,local,sem);

    if(initializer && Scan->IfToken('='))
    {
        var->Initializer = _Expr(); 
        Tree->ExpandVector(var->Initializer,var->Type);
        if(!var->Type->Compatible(var->Initializer->Type))
            Scan->Error("type mismatch");
    }

    return var;
}


sStmt *sParse::_Stmt()
{
    sStmt *stmt = 0;

    if(IsType())
    {
        auto var = _Decl(false,true);
        auto oldvar = Scope->Vars.Find([&](const sVariable *v){ return v->Name==var->Name; });
        if(oldvar)
        {
            if(!oldvar->Type->IsSame(var->Type) || oldvar->Storage.Flags != var->Storage.Flags || oldvar->Shaders != var->Shaders)
                Scan->Error("variable %q declared twice with different type and flags. Declaring twice is ok if declared in different modules",var->Name);
        }
        else
        {
            Scope->Vars.Add(var);
        }
        stmt = Tree->MakeStmt(sST_Decl);
        stmt->Vars.Add(var);
        if(Scan->IfToken('='))
        {
            stmt->Expr = _Expr(); 
            Tree->ExpandVector(stmt->Expr,var->Type);
            if(!var->Type->Compatible(stmt->Expr->Type))
                Scan->Error("type mismatch");
        }
        Scan->Match(';');
    }
    else
    {
        switch(Scan->Token)
        {
        case '{':
            stmt = _Block();
            break;

        case ';':
            Scan->Scan();
            stmt = Tree->MakeStmt(sST_Block);
            Scan->Match(';');
            break;

        case TOK_Break:
            Scan->Scan();
            stmt = Tree->MakeStmt(sST_Break);
            Scan->Match(';');
            break;

        case TOK_Continue:
            Scan->Scan();
            stmt = Tree->MakeStmt(sST_Continue);
            Scan->Match(';');
            break;

        case TOK_Discard:
            Scan->Scan();
            stmt = Tree->MakeStmt(sST_Discard);
            Scan->Match(';');
            break;

        case TOK_Return:
            Scan->Scan();
            stmt = Tree->MakeStmt(sST_Return);
            if(Scan->Token!=';')
                stmt->Expr = _Expr();
            Scan->Match(';');
            break;
            
        case TOK_If:
            Scan->Scan();
            stmt = Tree->MakeStmt(sST_If);
            stmt->ParentScope = Scope;
            Scope = stmt;
            Scan->Match('(');
            stmt->Expr = _Expr();
            Scan->Match(')');
            stmt->Block.Add(_Stmt());
            if(Scan->IfToken(TOK_Else))
                stmt->Block.Add(_Stmt());
            Scope = stmt->ParentScope;
            break;

        case TOK_Cif:
            Scan->Scan();
            stmt = Tree->MakeStmt(sST_Cif);
            Scan->Match('(');
            stmt->Expr = _Expr();
            Scan->Match(')');
            stmt->Block.Add(_BlockNoBrackets());
            if(Scan->IfToken(TOK_Else))
                stmt->Block.Add(_BlockNoBrackets());
            break;

        case TOK_For:
            Scan->Scan();
            stmt = Tree->MakeStmt(sST_For);
            if(Scan->IfName("unroll")) stmt->StatementFlags |= sSTF_Unroll;
            else if(Scan->IfName("loop")) stmt->StatementFlags |= sSTF_Loop;
            Scan->Match('(');
            stmt->Block.Add(_Stmt());
            stmt->Expr = _Expr();
            Scan->Match(';');
            stmt->IncExpr = _Expr();
            Scan->Match(')');
            stmt->Block.Add(_Stmt());
            break;

        case TOK_While:
            Scan->Scan();
            stmt = Tree->MakeStmt(sST_While);
            if(Scan->IfName("unroll")) stmt->StatementFlags |= sSTF_Unroll;
            else if(Scan->IfName("loop")) stmt->StatementFlags |= sSTF_Loop;
            Scan->Match('(');
            stmt->Expr = _Expr();
            Scan->Match(')');
            stmt->Block.Add(_Stmt());
            break;

        case TOK_Do:
            Scan->Scan();
            stmt = Tree->MakeStmt(sST_Do);
            if(Scan->IfName("unroll")) stmt->StatementFlags |= sSTF_Unroll;
            else if(Scan->IfName("loop")) stmt->StatementFlags |= sSTF_Loop;
            stmt->Block.Add(_Stmt());
            Scan->Match(TOK_While);
            Scan->Match('(');
            stmt->Expr = _Expr();
            Scan->Match(')');
            Scan->Match(';');
            break;

        case TOK_Switch:
            Scan->Scan();
            stmt = Tree->MakeStmt(sST_Switch);
            Scan->Match('(');
            stmt->Expr = _Expr();
            Scan->Match(')');
            stmt->Block.Add(_Block(true));
            break;

        case TOK_Inject:
            Scan->Scan();
            stmt = Tree->MakeStmt(sST_Inject);
            stmt->Inject = Tree->MakeInject();
            stmt->Inject->Shaders = Tree->CurrentShaders;
            stmt->Inject->Kind = sIK_Code;
            stmt->Inject->Name = Scan->ScanName();
            stmt->Inject->Scope = Scope;
            Tree->Injects.Add(stmt->Inject);
            Scan->Match(';');
            break;

        default:
            stmt = Tree->MakeStmt(sST_Expr);
            stmt->Expr = _Expr();
            Scan->Match(';');
            break;
        }
    }
    return stmt;
}

sStmt *sParse::_Block(bool swtch)
{
    sStmt *stmt = Tree->MakeStmt(sST_Block);
    stmt->ParentScope = Scope;
    Scope = stmt;

    Scan->Match('{');
    while(!Scan->Errors && !Scan->IfToken('}'))
    {
        if(swtch && Scan->IfToken(TOK_Case))
        {
            auto s = Tree->MakeStmt(sST_Case);
            s->Expr = _Expr();
            Scan->Match(':');
            stmt->Block.Add(s);
        }
        else if(swtch && Scan->IfToken(TOK_Default))
        {
            auto s = Tree->MakeStmt(sST_Default);
            Scan->Match(':');
            stmt->Block.Add(s);
        }
        else
        {
            stmt->Block.Add(_Stmt());
        }
    }

    Scope = stmt->ParentScope;
    return stmt;
}

sStmt *sParse::_BlockNoBrackets(bool swtch)
{
    sStmt *stmt = Tree->MakeStmt(sST_BlockNoBrackets);

    if(Scan->IfToken('{'))
    {
        while(!Scan->Errors && !Scan->IfToken('}'))
        {
            stmt->Block.Add(_Stmt());
        }
    }
    else
    {
        stmt->Block.Add(_Stmt());
    }

    return stmt;
}

/****************************************************************************/

void sParse::_Func()
{
    Scan->Match(TOK_Func);

    auto func = Tree->MakeFunction();

    if(Scan->IfName("numthreads"))
    {
        Scan->Match('(');
        func->NumThreadsX = _Expr();
        Scan->Match(',');
        func->NumThreadsY = _Expr();
        Scan->Match(',');
        func->NumThreadsZ = _Expr();
        Scan->Match(')');
    }

    func->Result = _Type();
    func->Name = Scan->ScanName();
    func->Root = Tree->MakeStmt(sST_Block);

    Scan->Match('(');
    while(!Scan->Errors && Scan->Token!=')')
    {
        sStorage store = _Storage();
        sSemantic sem = sSEM_None;
        auto type = _Type();
        sPoolString name = Scan->ScanName();

        sem = _Semantic();

        func->Root->Vars.Add(Tree->MakeVar(type,name,store,true,sem));

        if(!Scan->IfToken(','))
            break;
    }
    Scan->Match(')');

    Scope = func->Root;
    func->Root->Block.Add(_Block());
    Scope = 0;

    Tree->Funcs.Add(func);
}

sStorage sParse::_Storage()
{
    int oldstore = 0;
    sStorage store = 0;
    bool hit = false;
    do
    {
        oldstore = store.Flags;
        hit = false;

        if(Scan->IfName("row_major")) store.Flags |= sST_RowMajor;
        if(Scan->IfName("column_major")) store.Flags |= sST_ColumnMajor;
        if(Scan->IfName("lowp")) store.Flags |= sST_LowP;
        if(Scan->IfName("mediump")) store.Flags |= sST_MedP;
        if(Scan->IfName("highp")) store.Flags |= sST_HighP;
        if(Scan->IfName("const")) store.Flags |= sST_Const;
        if(Scan->IfName("groupshared")) store.Flags |= sST_GroupShared;
        if(Scan->IfName("static")) store.Flags |= sST_Static;
        if(Scan->IfName("precise")) store.Flags |= sST_Precise;
        if(Scan->IfName("in")) store.Flags |= sST_In;
        if(Scan->IfName("out")) store.Flags |= sST_Out;
        if(Scan->IfName("import")) store.Flags |= sST_Import;
        if(Scan->IfName("export")) store.Flags |= sST_Export;

        if(Scan->IfName("linear")) store.Flags |= sST_Linear;
        if(Scan->IfName("nointerpolation")) store.Flags |= sST_NoInterpolation;
        if(Scan->IfName("centroid")) store.Flags |= sST_Centroid;
        if(Scan->IfName("noperspective")) store.Flags |= sST_NoPerspective;
        if(Scan->IfName("sample")) store.Flags |= sST_Sample;
    }
    while(oldstore!=store.Flags || hit);

    return store;
}

sSemantic sParse::_Semantic()
{
    int sem = sSEM_None;
    int index = 0;

    if(Scan->IfToken(':'))
    {
        sString<128> name;
        Scan->ScanName(name);
        uptr len = sGetStringLen(name);
        int dec = 1;
        while(len>0 && sIsDigit(name[len-1]))
        {
            len--;
            index += dec * (name[len]-'0');
            dec = dec*10;
            name[len] = 0;
        }
        sPoolString ps = sPoolString(name);

        if(ps=="Position") sem = sSEM_Position;
        if(ps=="Normal") sem = sSEM_Normal;
        if(ps=="Tangent") sem = sSEM_Tangent;
        if(ps=="Binormal") sem = sSEM_Binormal;
        if(ps=="BlendIndex") sem = sSEM_BlendIndex;
        if(ps=="BlendWeight") sem = sSEM_BlendWeight;
        if(ps=="Color") sem = sSEM_Color;
        if(ps=="Tex") sem = sSEM_Tex;

        if(ps=="InstanceId") sem = sSEM_InstanceId;
        if(ps=="VertexId") sem = sSEM_VertexId;

        if(ps=="PositionT") sem = sSEM_PositionT;

        if(ps=="VFace") sem = sSEM_VFace;
        if(ps=="VPos") sem = sSEM_VPos;
        if(ps=="Target") sem = sSEM_Target;
        if(ps=="Depth") sem = sSEM_Depth;
        if(ps=="Coverage") sem = sSEM_Coverage;
        if(ps=="IsFrontFace") sem = sSEM_IsFrontFace;
        if(ps=="SampleIndex") sem = sSEM_SampleIndex;
        if(ps=="PrimitiveId") sem = sSEM_PrimitiveId;

        if(ps=="GsInstanceId") sem = sSEM_GSInstanceId;
        if(ps=="RenderTargetArrayIndex") sem = sSEM_RenderTargetArrayIndex;
        if(ps=="ViewportArrayIndex") sem = sSEM_ViewportArrayIndex;

        if(ps=="DomainLocation") sem = sSEM_DomainLocation;
        if(ps=="InsideTessFactor") sem = sSEM_InsideTessFactor;
        if(ps=="OutputControlPointId") sem = sSEM_OutputControlPointId;
        if(ps=="TessFactor") sem = sSEM_TessFactor;

        if(ps=="DispatchThreadId") sem = sSEM_DispatchThreadId;
        if(ps=="GroupId") sem = sSEM_GroupId;
        if(ps=="GroupIndex") sem = sSEM_GroupIndex;
        if(ps=="GroupThreadId") sem = sSEM_GroupThreadId;

        if(ps=="Vs") sem = sSEM_Vs;
        if(ps=="Hs") sem = sSEM_Hs;
        if(ps=="Ds") sem = sSEM_Ds;
        if(ps=="Gs") sem = sSEM_Gs;
        if(ps=="Ps") sem = sSEM_Ps;
        if(ps=="Cs") sem = sSEM_Cs;
        if(ps=="Slot") sem = sSEM_Slot;
        if(ps=="SlotAuto") sem = sSEM_SlotAuto;
        if(ps=="Uav") sem = sSEM_Uav;

        if(sem==sSEM_None)
            Scan->Error("Semantic Expected");
    }
    return sSemantic(sem | (index<<sSEM_IndexShift));
}

void sParse::_Members(sType *str,sInject *inj,sModule *mod)
{
    bool align = inj!=0;
    bool injecting = false;
    Scan->Match('{');
    while(!Scan->Errors && !Scan->IfToken('}'))
    {
        if(Scan->IfToken(TOK_Inject))
        {
            if(inj)
                Scan->Error("cant nest injects");
            auto inject = Tree->MakeInject();
            if(str->Kind==sTK_CBuffer || str->Kind==sTK_TBuffer)
                inject->Kind = sIK_Const;
            else if(str->Kind==sTK_Struct)
                inject->Kind = sIK_Struct;
            else
                Scan->Error("inject only allowed for tbuffer and cbuffer");
            inject->Name = Scan->ScanName();
            inject->Shaders = Tree->CurrentShaders;
            inject->Type = str;
            Tree->Injects.Add(inject);
            Scan->Match(';');
            injecting = true;
        }
        else
        {
            if(injecting)
                Scan->Error("inject must be at the end of the buffer");

            sExpr *cond = 0;
            if(Scan->IfToken(TOK_Cif))
            {
                Scan->Match('(');
                cond = _Expr();
                Scan->Match(')');
            }

            sStorage store = _Storage();
            sSemantic sem = sSEM_None;
            auto type = _Type();
            sPoolString name = Scan->ScanName();
            sem = _Semantic();

            sArray<sExpr *> arrays;
            while(Scan->IfToken('['))
            {
                arrays.Add(_Expr());
                Scan->Match(']');
            }
            for(int i=arrays.GetCount()-1;i>=0;i--)
                type = Tree->MakeArray(type,arrays[i]);


            Scan->Match(';');
            auto mem = Tree->AddMember(str,type,name,store,sem);
            mem->Active = inj==0;
            mem->Align = align; align = false;
            mem->Module = mod;
            mem->Condition = cond;
            if(inj)
                inj->Members.Add(mem);
        }
    }
    str->AssignOffsets(Tree);
}

void sParse::_GDecl(sTypeKind tk)
{
    auto type = Tree->MakeType(tk);
    type->Name = Scan->Token==sTOK_Name ? Scan->ScanName() : "";
    
    type->Semantic = _Semantic();
    _Members(type);

    if(!Scan->Errors)
    {
        if(Tree->Types.Find([&](const sType *t){ return t->Name==type->Name; }))
        {
            Scan->Error("type %q defined twice",type->Name);
        }
        else
        {
            Tree->Types.Add(type);
            Tree->TypeDefs.Add(Tree->MakeTypeDef(type));
        }
    }
}

void sParse::_Shader(int shader)
{
    if(Tree->CurrentShaders!=0)
        Scan->Error("can't nest shaders");
    Scan->Match('{');
    Tree->CurrentShaders = 1<<shader;
    Tree->UsedShaders |= Tree->CurrentShaders;
    _Global();
    Tree->CurrentShaders = 0;
    Scan->Match('}');
}

void sParse::_ModuleShader(sModule *mod,int shader)
{
    Tree->CurrentShaders = 1<<shader;
    Scan->Match('{');
    while(!Scan->Errors && Scan->Token!=sTOK_End && Scan->Token!='}')
    {
        if(IsType())
        {
            auto var = _Decl(true,false);
            if(var)
            {
                if(mod->Vars.Find([&](const sVariable *v){ return v->Name==var->Name; }))
                    Scan->Error("variable %q defined twice in this module",var->Name);
                mod->Vars.Add(var);
            }
            Scan->Match(';');
        }
        else if(Scan->Token == TOK_Func)
        {
            _Func();        // just put functions in global scope
        }
        else
        {
            Scan->Error("syntax");
        }
    }
    Scan->Match('}');
    Tree->CurrentShaders = 0;
}

void sParse::_ModuleDepends(sModule *mod,sExpr *cond)
{
    do
    {
        sPoolString name = Scan->ScanName();
        auto omod = Tree->Modules.Find([=](const sModule *m){ return m->Name==name; });
        if(!omod)
            Scan->Error("module %q not found",name);
        else if(!omod->Once)
            Scan->Error("dependend module %q must have 'once' flag set",name);
        else
            mod->Depends.Add(sModuleDependency(omod,cond));
    }
    while(Scan->IfToken(','));
}

void sParse::_Module()
{
    auto mod = Tree->MakeModule();
    mod->Name = Scan->ScanName();
    Tree->CurrentModule = mod;
    Scan->Match('{');
    while(!Scan->Errors && !Scan->IfToken('}'))
    {
        if(Scan->IfToken(TOK_Inject))
        {
            sPoolString name = Scan->ScanName();
            auto snip = Tree->MakeSniplet();
            snip->Inject = Tree->Injects.Find([=](const sInject *i){ return i->Name==name; });
            if(!snip->Inject)
            {
                Scan->Error("unknown injection point %q",name);
            }
            else
            {
                Scope = snip->Inject->Scope;
                Tree->CurrentShaders = snip->Inject->Shaders;
                switch(snip->Inject->Kind)
                {
                case sIK_Code:
                    snip->Code = _BlockNoBrackets();
                    break;
                case sIK_Const:
                case sIK_Struct:
                    _Members(snip->Inject->Type,snip->Inject,mod);
                    break;
                default:
                    sASSERT(0);
                }
                mod->Sniplets.Add(snip);
            }
            Tree->CurrentShaders = 0;
            Scope = 0;
        }
        else if(Scan->IfName("once"))
        {
            if(mod->Once)
                Scan->Error("module option 'once' stated multiple times");
            mod->Once = true;
            Scan->Match(';');
        }
        else if(Scan->IfToken(TOK_Cif))
        {
            Scan->Match('(');
            sExpr *expr = _Expr();
            Scan->Match(')');
            if(Scan->IfToken('{'))
            {
                while(!Scan->Errors && !Scan->IfToken('}'))
                {
                    Scan->MatchName("depend");
                    _ModuleDepends(mod,expr);
                    Scan->Match(';');
                }
            }
            else
            {
                Scan->MatchName("depend");
                _ModuleDepends(mod,expr);
                Scan->Match(';');
            }
        }
        else if(Scan->IfName("depend"))
        {
            _ModuleDepends(mod,0);
            Scan->Match(';');
        }
        else if(Scan->IfName("vs"))
        {
            _ModuleShader(mod,sST_Vertex);
        }
        else if(Scan->IfName("hs"))
        {
            _ModuleShader(mod,sST_Hull);
        }
        else if(Scan->IfName("ds"))
        {
            _ModuleShader(mod,sST_Domain);
        }
        else if(Scan->IfName("gs"))
        {
            _ModuleShader(mod,sST_Geometry);
        }
        else if(Scan->IfName("ps"))
        {
            _ModuleShader(mod,sST_Pixel);
        }
        else if(Scan->IfName("cs"))
        {
            _ModuleShader(mod,sST_Compute);
        }
        else if(Scan->IfName("cond"))
        {
            sTypeKind tk = sTK_Bool;
            if(Scan->IfName("int")) tk = sTK_Int;
            else if(Scan->IfName("uint")) tk = sTK_Uint;
            sPoolString name = Scan->ScanName();
            sCondition *c = Tree->Pool.Alloc<sCondition>();
            mod->Conditions.AddMany(1)->Set(tk,name);
            if(Scan->IfToken('{'))
            {
                do
                {
                    mod->Conditions.AddMany(1)->Set(tk,name,Scan->ScanName());
                }
                while(!Scan->Errors && Scan->IfToken(','));
                Scan->Match('}');
            }
            Scan->Match(';');
        }
        else
        {
            Scan->Error("syntax");
        }
    }
    Tree->Modules.Add(mod);
    Tree->CurrentModule = 0;
}

void sParse::_VarBool()
{
    auto var = Tree->MakeVariant();
    Tree->Variants.Add(var);
    
    var->Name = Scan->ScanName();
    var->Mask = 1<<Tree->VariantBit;
    var->Bool = true;
    Tree->VariantBit++;
    if(Scan->IfToken('?'))
    {
        auto m1 = Tree->MakeVariantMember();
        sPoolString name = Scan->ScanName();
        m1->Value = var->Mask;
        m1->Module = Tree->FindModule(name);
        if(!m1->Module)
            Scan->Error("unknwon module %q",name);
        Scan->Match(':');
        auto m0 = Tree->MakeVariantMember();
        name = Scan->ScanName();
        m0->Module = Tree->FindModule(name);
        if(!m0->Module)
            Scan->Error("unknwon module %q",name);

        var->Members.Add(m0);
        var->Members.Add(m1);
    }
    else
    {
        auto m1 = Tree->MakeVariantMember();
        m1->Name = var->Name;
        m1->Value = var->Mask;
        sPoolString name = m1->Name;
        if(Scan->IfToken('='))
            name = Scan->ScanName();
        m1->Module = Tree->FindModule(name);
        if(!m1->Module)
            Scan->Error("unknwon module %q",m1->Name);
        var->Members.Add(m1);
    }

    Scan->Match(';');
}

void sParse::_Var()
{
    auto var = Tree->MakeVariant();
    Tree->Variants.Add(var);
    
    var->Name = Scan->ScanName();
    int n = 0;
    Scan->Match('{');
    do
    {
        auto mem = Tree->MakeVariantMember();
        var->Members.Add(mem);
        mem->Name = Scan->ScanName();
        mem->Value = n<<Tree->VariantBit;
        sPoolString name = mem->Name;
        if(Scan->IfToken('='))
        {
            if(Scan->Token==sTOK_Name)
            {
                name = Scan->ScanName();
            }
            else
            {
                name.Clear();
                if(Scan->ScanInt()!=0)
                    Scan->Error("name or 0 expected");
            }
        }
        if(!name.IsEmpty())
        {
            mem->Module = Tree->FindModule(name);
            if(!mem->Module)
                Scan->Error("unknwon module %q",mem->Name);
        }
        n++;
    }
    while(Scan->IfToken(','));
    Scan->Match('}');
    Scan->Match(';');

    int bits = sFindHigherPower(var->Members.GetCount());
    var->Mask = ((1<<bits)-1)<<Tree->VariantBit;
    Tree->VariantBit += bits;
}

void sParse::_VarUse()
{
    auto var = Tree->MakeVariant();
    Tree->Variants.Add(var);
    auto mem = Tree->MakeVariantMember();
    var->Members.Add(mem);
    
    mem->Name = Scan->ScanName();
    mem->Module = Tree->FindModule(mem->Name);
    if(!mem->Module)
        Scan->Error("unknwon module %q",mem->Name);
    var->Mask = 0;
    Scan->Match(';');
}

void sParse::_Global()
{
    while(!Scan->Errors && Scan->Token!=sTOK_End && Scan->Token!='}')
    {
        if(Scan->Token==TOK_Func)
        {
            _Func();
        }
        else if(Scan->IfName("struct"))
        {
            _GDecl(sTK_Struct);
        }
        else if(Scan->IfName("cbuffer"))
        {
            _GDecl(sTK_CBuffer);
        }
        else if(Scan->IfName("tbuffer"))
        {
            _GDecl(sTK_CBuffer);
        }
        else if(Scan->IfName("vs"))
        {
            _Shader(sST_Vertex);
        }
        else if(Scan->IfName("hs"))
        {
            _Shader(sST_Hull);
        }
        else if(Scan->IfName("ds"))
        {
            _Shader(sST_Domain);
        }
        else if(Scan->IfName("gs"))
        {
            _Shader(sST_Geometry);
        }
        else if(Scan->IfName("ps"))
        {
            _Shader(sST_Pixel);
        }
        else if(Scan->IfName("cs"))
        {
            _Shader(sST_Compute);
        }
        else if(Scan->IfName("module"))
        {
            _Module();
        }
        else if(Scan->IfName("var"))
        {
            _Var();
        }
        else if(Scan->IfName("varbool"))
        {
            _VarBool();
        }
        else if(Scan->IfName("use"))
        {
            _VarUse();
        }
        else if(Scan->IfToken(';'))
        {
        }
        else if(IsType())
        {
            auto var = _Decl(true,false);
            if(var)
            {
                if(Tree->Vars.Find([&](const sVariable *v){ return v->Name==var->Name; }))
                    Scan->Error("global variable %q defined twice",var->Name);
                Tree->Vars.Add(var);
            }
        }
        else
        {
            Scan->Error("Syntax");
        }
    }
}

/****************************************************************************/

sParse::sParse()
{
    Tree = 0;
    Scope = 0;
    Scan = 0;
}

sParse::~sParse()
{
    delete Tree;
    sASSERT(Scan==0);
}

bool sParse::Parse(sScanner *scan,bool needEof,sTree *tree)
{
    Scan = scan;
    Tree = tree;

    Scope = 0;
    Tree = tree;

    _Global();
    if(Scan->Token!=sTOK_End && needEof)
        Scan->Error("end of file expected");

    Tree = 0;
    if(Scan->Errors)
    {
        Scan = 0;
        return false;
    }
    Scan = 0;
    return true;
}

void sParse::InitScanner(sScanner *scan)
{
    scan->Init(sSF_CppComment | sSF_EscapeCodes | sSF_MergeStrings | sSF_Cpp);
    scan->AddDefaultTokens();
    scan->AddToken("if",TOK_If);
    scan->AddToken("cif",TOK_Cif);
    scan->AddToken("else",TOK_Else);
    scan->AddToken("while",TOK_While);
    scan->AddToken("do",TOK_Do);
    scan->AddToken("for",TOK_For);
    scan->AddToken("switch",TOK_Switch);
    scan->AddToken("case",TOK_Case);
    scan->AddToken("default",TOK_Default);
    scan->AddToken("return",TOK_Return);
    scan->AddToken("break",TOK_Break);
    scan->AddToken("continue",TOK_Continue);
    scan->AddToken("discard",TOK_Discard);
    scan->AddToken("inject",TOK_Inject);

    scan->AddToken("true",TOK_True);
    scan->AddToken("false",TOK_False);
    scan->AddToken("++",TOK_Inc);
    scan->AddToken("--",TOK_Dec);

    scan->AddToken("+=",TOK_AssignAdd);
    scan->AddToken("-=",TOK_AssignSub);
    scan->AddToken("*=",TOK_AssignMul);
    scan->AddToken("/=",TOK_AssignDiv);
    scan->AddToken("%=",TOK_AssignMod);
    scan->AddToken("<<=",TOK_AssignShiftL);
    scan->AddToken(">>=",TOK_AssignShiftR);
    scan->AddToken("&=",TOK_AssignBAnd);
    scan->AddToken("|=",TOK_AssignBOr);
    scan->AddToken("^=",TOK_AssignBEor);

    scan->AddToken("func",TOK_Func);
    scan->AddToken("**",TOK_DotProduct);
}

/****************************************************************************/


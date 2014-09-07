/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Doc.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

wPermute::wPermute()
{
    Doc->Permutes.Add(this);
    Count = 0;
    Shift = 0;
}

/****************************************************************************/

wPredicate::wPredicate()
{
    Doc->Predicates.Add(this);
    Op = wPO_Error;
    a = 0;
    b = 0;
    Permute = 0;
    Literal = 0;
}

wPredicate::wPredicate(wPredicateOp op,wPredicate *a_,wPredicate *b_)
{
    Doc->Predicates.Add(this);
    Op = op;
    a = a_;
    b = b_;
    Permute = 0;
    Literal = 0;
}

wPredicate::wPredicate(sInt lit)
{
    Doc->Predicates.Add(this);
    Op = wPO_Literal;
    a = 0;
    b = 0;
    Permute = 0;
    Literal = lit;
}

wPredicate::wPredicate(wPermute *perm)
{
    Doc->Predicates.Add(this);
    Op = wPO_Permute;
    a = 0;
    b = 0;
    Permute = perm;
    Literal = 0;
}

sInt wPredicate::Evaluate(wMaterial *mtrl,sU32 perm)
{
    switch(Op)
    {
    case wPO_Permute:
        return (perm>>Permute->Shift)&Permute->Mask;
    case wPO_Literal:
        return Literal;
    case wPO_And:
        return a->Evaluate(mtrl,perm) && b->Evaluate(mtrl,perm);
    case wPO_Or:
        return a->Evaluate(mtrl,perm) || b->Evaluate(mtrl,perm);
    case wPO_Not:
        return !a->Evaluate(mtrl,perm);
    case wPO_Equal:
        return a->Evaluate(mtrl,perm) == b->Evaluate(mtrl,perm);
    case wPO_NotEqual:
        return a->Evaluate(mtrl,perm) != b->Evaluate(mtrl,perm);
    }
    sASSERT0();
    return 0;
}

sU32 wPredicate::UsedPermutations()
{
    if(wPO_Permute)
        return Permute->Mask << Permute->Shift;
    sU32 perm = 0;
    if(a) perm |= a->UsedPermutations();
    if(b) perm |= b->UsedPermutations();
    return perm;
}

/****************************************************************************/

wAscType::wAscType()
{
    Doc->AscTypes.Add(this);

    Base = wTB_Error;
    Columns = 0;
    Rows = 0;
    Array = 0;
    ColumnMajor = 0;
}

sInt wAscType::GetWordSize()
{
    if(Rows==0 && Array==0)
        return Columns;
    else
        return 4*Rows*Array;
}

sInt wAscType::GetRegisterSize()
{
    if(Rows==0 && Array==0)
        return 1;
    else
        return sMax(1,Rows)*sMax(1,Array);
}

/****************************************************************************/

wAscMember::wAscMember()
{
    Doc->AscMembers.Add(this);
    Type = 0;
    Predicate = 0;
    Flags = 0;
}

wAscBuffer::wAscBuffer()
{
    Doc->AscBuffers.Add(this);
    Slot = -1;
    Register = -1;
    Kind = wBK_Error;
    Shader = sST_Max;
}

/****************************************************************************/

wMaterial::wMaterial()
{
    PermuteShift = 0;
    Doc->Materials.Add(this);
}

sBool wMaterial::PermutationIsValid(sU32 p)
{
    for(auto perm : Permutes)
    {
        if(((p>>perm->Shift)&perm->Mask)>=(sU32)perm->Count)
            return 0;
    }
    return 1;
}

/****************************************************************************/

wShader::wShader()
{
    Size = 0;
    Data = 0;
}

wShader::wShader(sPtr size,const sU8 *data)
{
    Size = size;
    Data = new sU8[size];
    sCopyMem(Data,data,size);
}

wShader::~wShader()
{
    delete[] Data;
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

wPredicate *wDocument::Negate(wPredicate *a)
{
    return new wPredicate(wPO_Not,a);
}

wPredicate *wDocument::Combine(wPredicate *a,wPredicate *b)
{
    if(b==0)
        return a;

    return new wPredicate(wPO_And,a,b);
}

wPredicate *wDocument::_PredValue()
{
    if(Scan->IfToken('!'))
    {
        return new wPredicate(wPO_Not,_PredValue());
    }

    wPredicate *pred = 0;

    if(Scan->IfToken('('))
    {
        pred = _Predicate();
        Scan->Match(')');
    }
    else if(Scan->Token==sTOK_Int)
    {
        pred = new wPredicate(Scan->ScanInt());
    }
    else if(Scan->Token==sTOK_Name)
    {
        sPoolString name;
        Scan->ScanName(name);
        wPermute *permute = Material->Permutes.Find([=](wPermute *i){return i->Name==name;});
        if(!permute)
        {
            Scan->Error("permute %q not found",name);
        }
        else
        {
            if(Scan->IfToken('.'))
            {
                sPoolString optname = Scan->ScanName();
                wPermuteOption *op = permute->Options.FindValue([=](wPermuteOption &i){return i.Name==optname;});
                if(!op)
                {
                    Scan->Error("permute option %q.%q not found",name,optname);
                }
                else
                {
                    pred = new wPredicate(op->Value);
                }
            }
            else
            {
                pred = new wPredicate(permute);
            }
        }
    }
    else
    {
        Scan->Error("unknown token in expression");
        Scan->Scan();
    }

    if(pred==0)
        pred = new wPredicate();

    return pred;
}

wPredicate *wDocument::_PredAnd()
{
    wPredicate *a = _PredValue();
    while(Scan->IfToken(sTOK_DoubleAnd))
        a = new wPredicate(wPO_And,a,_PredValue());
    return a;
}

wPredicate *wDocument::_PredOr()
{
    wPredicate *a = _PredAnd();
    while(Scan->IfToken(sTOK_DoubleOr))
        a = new wPredicate(wPO_Or,a,_PredAnd());
    return a;
}

wPredicate *wDocument::_PredCmp()
{
    wPredicate *a = _PredOr();
    while(Scan->Token==sTOK_EQ || Scan->Token==sTOK_NE)
    {
        sInt token = Scan->Token;
        Scan->Scan();
        a = new wPredicate(token==sTOK_EQ ? wPO_Equal : wPO_NotEqual,a,_PredOr());
    }
    return a;
}

wPredicate *wDocument::_Predicate()
{
    return _PredCmp();
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void wDocument::AscShader(sShaderTypeEnum st)
{
    wSourceLanguage sl;
    if(Scan->IfName("hlsl3"))
    {
        sl = wSL_Hlsl3;
    }
    else if(Scan->IfName("hlsl5"))
    {
        sl = wSL_Hlsl5;
    }
    else if(Scan->IfName("glsl1"))
    {
        sl = wSL_Glsl1;
    }
    else
    {
        sl = wSL_Glsl1;
        Scan->Error("shader language expected");
    }


    if(!Material->Sources[sl][st].Source.IsEmpty())
        Scan->Error("shader defined twice");
    if(Predicate)
        Scan->Error("shaders can not be defined in pif-blocks");
    Material->Sources[sl][st].Line = Scan->GetLine()+1;
    Scan->ScanRaw(Material->Sources[sl][st].Source,'{','}');
    Scan->Match(';');
}

void wDocument::AscPermute()
{
    if(Predicate)
        Scan->Error("permutations can not be declared in pif-blocks");

    wPermute *perm = new wPermute();

    perm->Name = Scan->ScanName();
    if(Scan->IfToken('{'))
    {
        sInt n=0;
        while(Scan->Token!=sTOK_End && !Scan->IfToken('}'))
        {
            if(n!=0)
                Scan->Match(',');
            wPermuteOption opt;
            opt.Name = Scan->ScanName();
            opt.Value = n++;
            perm->Options.Add(opt);
        }
        perm->Count = n;
        if(n<2)
            Scan->Error("at least two options must be given when manually specifying options");
    }
    else
    {
        perm->Count = 2;
    }
    sInt bits = sFindHigherPower(perm->Count);
    perm->Mask = (1<<bits)-1;
    perm->Shift = Material->PermuteShift;
    Material->PermuteShift += bits;
    if(Material->PermuteShift>30)
        Scan->Error("more than 30 bits for permutations used (this is insane!)");
    Material->Permutes.Add(perm);

    Scan->Match(';');
}

void wDocument::AscMemberSingle(wAscBuffer *buffer,sBool allowpif)
{
    if(Scan->IfName("pif"))
    {
        if(!allowpif)
            Scan->Error("pifs are not allowed in this type");
        Scan->Match('(');
        wPredicate *pred = _Predicate();
        Scan->Match(')');
        wPredicate *old = Predicate;
        Predicate = Combine(pred,old);
        AscMemberBlock(buffer,allowpif);
        if(Scan->IfName("pelse"))
        {
            Predicate = Combine(Negate(pred),old);
            AscMemberBlock(buffer,allowpif);
        }
        Predicate = old;
    }
    else
    {
        wAscMember *mem = new wAscMember();
        wAscType *type = new wAscType();

moreflags:
        if(Scan->Token==TokExtern)
        {
            Scan->Scan();
            mem->Flags |= wMF_Extern;
            goto moreflags;
        }
        if(Scan->Token==TokStatic)
        {
            Scan->Scan();
            mem->Flags |= wMF_Static;
            goto moreflags;
        }
        if(Scan->Token==TokUniform)
        {
            Scan->Scan();
            mem->Flags |= wMF_Uniform;
            goto moreflags;
        }
        if(Scan->Token==TokVolatile)
        {
            Scan->Scan();
            mem->Flags |= wMF_Volatile;
            goto moreflags;
        }
        if(Scan->Token==sTOK_Name && Scan->Name=="nointerpolation")
        {
            Scan->Scan();
            mem->Flags |= wMF_NoInterpolation;
            goto moreflags;
        }
        if(Scan->Token==sTOK_Name && Scan->Name=="precise")
        {
            Scan->Scan();
            mem->Flags |= wMF_Precise;
            goto moreflags;
        }
        if(Scan->Token==sTOK_Name && Scan->Name=="groupshared")
        {
            Scan->Scan();
            mem->Flags |= wMF_GroupShared;
            goto moreflags;
        }
        if(Scan->Token==sTOK_Name && Scan->Name=="shared")
        {
            Scan->Scan();
            mem->Flags |= wMF_Shared;
            goto moreflags;
        }


        if(Scan->IfName("row_major"))
        {
        }
        else if(Scan->IfName("column_major"))
        {
            type->ColumnMajor = 1;
        }

        sString<64> tname;
        Scan->ScanName(tname);

        sInt len = sGetStringLen(tname);
        if(len>0 && tname[len-1]>='1' && tname[len-1]<='4')
        {
            type->Columns = tname[len-1]-'0';
            len--;
            if(len>1 && tname[len-1]=='x' && tname[len-2]>='1' && tname[len-2]<='4')
            {
                type->Rows = tname[len-2]-'0';
                len-=2;
            }
            tname[len] = 0;
        }

        if(tname=="bool") type->Base = wTB_Bool;
        else if(tname=="int") type->Base = wTB_Int;
        else if(tname=="uint") type->Base = wTB_UInt;
        else if(tname=="float") type->Base = wTB_Float;
        else if(tname=="double") type->Base = wTB_Double;
        else if(tname=="min16float") type->Base = wTB_Min16Float;
        else if(tname=="min10float") type->Base = wTB_Min10Float;
        else if(tname=="min16int") type->Base = wTB_Min16Int;
        else if(tname=="min12int") type->Base = wTB_Min12Int;
        else if(tname=="min16uint") type->Base = wTB_Min16UInt;
        else Scan->Error("unknown type %q",tname);

        mem->Name = Scan->ScanName();
        mem->Type = type;
        mem->Predicate = Predicate;

        if(Scan->IfToken('['))
        {
            type->Array = Scan->ScanInt();
            Scan->Match(']');
        }

        if(Scan->IfToken(':'))
            mem->Binding = Scan->ScanName();

        Scan->Match(';');

        buffer->Members.AddTail(mem);
    }
}

void wDocument::AscMemberBlock(wAscBuffer *buffer,sBool allowpif)
{
    if(Scan->IfToken('{'))
    {
        while(Scan->Token!=sTOK_End && !Scan->IfToken('}'))
            AscMemberSingle(buffer,allowpif);
    }
    else
    {
        AscMemberSingle(buffer,allowpif);
    }
}

void wDocument::AscCBuffer()
{
    wAscBuffer *buffer = new wAscBuffer;
    buffer->Name = Scan->ScanName();
    buffer->Kind = wBK_Constants;
    Material->Buffers.Add(buffer);

    while(Scan->IfToken(':'))
    {
        sPoolString name = Scan->ScanName();
        Scan->Match('(');
        sInt num = Scan->ScanInt();
        Scan->Match(')');

        if(name=="vs")
        {
            buffer->Shader = sST_Vertex;
            buffer->Slot = num;
        }
        else if(name=="hs")
        {
            buffer->Shader = sST_Hull;
            buffer->Slot = num;
        }
        else if(name=="ds")
        {
            buffer->Shader = sST_Domain;
            buffer->Slot = num;
        }
        else if(name=="gs")
        {
            buffer->Shader = sST_Geometry;
            buffer->Slot = num;
        }
        else if(name=="ps")
        {
            buffer->Shader = sST_Pixel;
            buffer->Slot = num;
        }
        else if(name=="cs")
        {
            buffer->Shader = sST_Compute;
            buffer->Slot = num;
        }
        else if(name=="register")
        {
            buffer->Register = num;
        }
        else
        {
            Scan->Error("unknown semantic");
        }
    }
    AscMemberBlock(buffer,1);

    Scan->Match(';');
}

void wDocument::AscStruct()
{
    wAscBuffer *buffer = new wAscBuffer;
    buffer->Name = Scan->ScanName();
    buffer->Kind = wBK_Struct;
    Material->Buffers.Add(buffer);
    AscMemberBlock(buffer,1);
    Scan->Match(';');
}

/****************************************************************************/

void wDocument::AscBlock()
{
    if(Scan->IfToken('{'))
    {
        while(Scan->Token!=sTOK_End && !Scan->IfToken('}'))
            AscStmt();
    }
    else
    {
        AscStmt();
    }
}

void wDocument::AscStmt()
{
    if(Scan->IfName("permute"))
    {
        AscPermute();
    }
    else if(Scan->IfName("struct"))
    {
        AscStruct();
    }
    else if(Scan->IfName("cbuffer"))
    {
        AscCBuffer();
    }
    else if(Scan->IfName("vs"))
    {
        AscShader(sST_Vertex);
    }
    else if(Scan->IfName("ds"))
    {
        AscShader(sST_Domain);
    }
    else if(Scan->IfName("hs"))
    {
        AscShader(sST_Hull);
    }
    else if(Scan->IfName("gs"))
    {
        AscShader(sST_Geometry);
    }
    else if(Scan->IfName("ps"))
    {
        AscShader(sST_Pixel);
    }
    else if(Scan->IfName("cs"))
    {
        AscShader(sST_Compute);
    }
    else if(Scan->IfName("pif"))
    {
        Scan->Match('(');
        wPredicate *pred = _Predicate();
        Scan->Match(')');
        wPredicate *old = Predicate;
        Predicate = Combine(pred,old);
        AscBlock();
        if(Scan->IfName("pelse"))
        {
            Predicate = Combine(Negate(pred),old);
            AscBlock();
        }
        Predicate = old;
    }
    else
    {
        Scan->Error("syntax");
        Scan->Scan();
    }
}

void wDocument::AscMaterial()
{
    Material = new wMaterial;
    Material->Name = Scan->ScanName();
    Predicate = 0;
    AscBlock();
}

sBool wDocument::AscParse(const sChar *filename)
{
    Scan->Init(sSF_CppComment|sSF_EscapeCodes|sSF_MergeStrings);
    Scan->AddDefaultTokens();
    Scan->StartFile(filename);

    while(Scan->Token!=sTOK_End)
    {
        if(Scan->IfName("mtrl"))
        {
            AscMaterial();
        }
        else if(Scan->IfName("namespace"))
        {
            if(!NameSpace.IsEmpty())
                Scan->Error("namespace directive used twice");
            do
            {
                NameSpace.Add(Scan->ScanName());
            }
            while(Scan->IfToken('.'));
            Scan->Match(';');
        }
        else
        {
            Scan->Error("syntax");
            Scan->Scan();
        }
    }

    sDPrintF(Scan->ErrorLog.Get());
    if(Scan->Errors)
        ErrorFlag = 1;
    return Scan->Errors==0;
}

/****************************************************************************/


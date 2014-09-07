/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Doc.hpp"

// missing features:
// * correct for(int i=0,j=1;;)
// * structs
// * typedef
// * in variable declarations: packoffset
// * pelse at global scope
// denied features:
// * comma operator

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sBool wDocument::HlslIsStorageClass()
{
    if(Scan->Token==TokExtern) return 1;
    if(Scan->Token==TokStatic) return 1;
    if(Scan->Token==TokUniform) return 1;
    if(Scan->Token==TokVolatile) return 1;
    if(Scan->Token==sTOK_Name && Scan->Name=="nointerpolation") return 1;
    if(Scan->Token==sTOK_Name && Scan->Name=="precise") return 1;
    if(Scan->Token==sTOK_Name && Scan->Name=="groupshared") return 1;
    if(Scan->Token==sTOK_Name && Scan->Name=="shared") return 1;
    return 0;
}

sBool wDocument::HlslIsTypeModifer()
{
    if(Scan->Token==TokColumnMajor) return 1;
    if(Scan->Token==TokRowMajor) return 1;
    if(Scan->Token==TokConst) return 1;
    return 0;
}

sBool wDocument::HlslType()
{
    if(Scan->Token!=sTOK_Name)
        return 0;

    // already known type?

    for(auto &pstr : Typenames)
    {
        if(pstr == Scan->Name)
        {
            OutPtr->PrintF("%s",Scan->Name);
            Scan->Scan();
            return 1;
        }
    }

    if(Scan->IfToken(TokMatrix))
    {
        Scan->Match('<');
        OutPtr->Print("matrix<");
        HlslType();
        Scan->Match(',');
        OutPtr->PrintF(",%d",Scan->ScanInt());
        Scan->Match(',');
        OutPtr->PrintF(",%d>",Scan->ScanInt());
        Scan->Match('>');
        return 1;
    }

    if(Scan->IfToken(TokVector))
    {
        Scan->Match('<');
        OutPtr->Print("vector");
        HlslType();
        Scan->Match(',');
        OutPtr->PrintF(",%d>",Scan->ScanInt());
        Scan->Match('>');
        return 1;
    }

    // parse normal type. 

    sBool ok = 0;
    if(Scan->IfName("snorm"))
    {
        ok = 1;
        OutPtr->Print("snorm ");
    }
    else if(Scan->IfName("unorm"))
    {
        ok = 1;
        OutPtr->Print("unorm ");
    }

    if(Scan->Token!=sTOK_Name)
        return ok;


    sString<64> buffer = (const sChar *) Scan->Name;
    sInt len = sGetStringLen(buffer);
    if(len>1 && buffer[len-1]>='1' && buffer[len-1]<='4')
    {
        len--;
        if(len>2 && buffer[len-1]=='x' && buffer[len-2]>='1' && buffer[len-2]<='4')
            len-=2;
        buffer[len] = 0;
    }
    const sChar *typenames[] = 
    {
        "void","bool","int","uint","float","double",
        "min16float","min10float","min16int","min12int","min16uint",
        "TriangleStream","LineStream","PointStream","InputPatch","OutputPatch",
    };


    sBool found = 0;
    for(sInt i=0;i<sCOUNTOF(typenames) && !found;i++)
        if(sCmpString(typenames[i],buffer)==0)
            found = 1;

    if(!found) 
        return ok;

    OutPtr->Print(Scan->Name);
    Scan->Scan();
    HlslTemplateArgs();
    return 1;
}

void wDocument::HlslValue()
{
more:
    if(Scan->IfToken('-'))
    {
        OutPtr->Print("-");
        goto more;
    }
    if(Scan->IfToken('+'))
    {
        OutPtr->Print("+");
        goto more;
    }
    if(Scan->IfToken('!'))
    {
        OutPtr->Print("!");
        goto more;
    }
    if(Scan->IfToken('~'))
    {
        OutPtr->Print("~");
        goto more;
    }
    if(Scan->IfToken(TokInc))
    {
        OutPtr->Print("++");
        goto more;
    }
    if(Scan->IfToken(TokDec))
    {
        OutPtr->Print("--");
        goto more;
    }

    if(Scan->Token==sTOK_Int || Scan->Token==sTOK_Float)
    {
        OutPtr->Print(Scan->ValueString);
        Scan->Scan();
    }
    else if(Scan->Token==sTOK_String)     // for attributes
    {
        OutPtr->PrintF("%q",Scan->ScanString());
    }
    else if(Scan->IfToken('('))
    {
        OutPtr->Print("(");
        if(!HlslType())
            HlslExpr();
        Scan->Match(')');
        OutPtr->Print(")");
    }
    else if(Scan->Token==sTOK_Name)
    {
        OutPtr->Print(Scan->ScanName());
    }
    else if(Scan->IfToken(TokNull))
    {
        OutPtr->Print("null");
    }
    else
    {
        Scan->Error("syntax");
        Scan->Scan();
    }

more2:
    if(Scan->IfToken('['))
    {
        OutPtr->Print("[");
        HlslExpr();
        Scan->Match(']');
        OutPtr->Print("]");
        goto more2;
    }
    if(Scan->IfToken('.'))
    {
        OutPtr->Print(".");
        OutPtr->Print(Scan->ScanName());
        goto more2;
    }
    if(Scan->IfToken(TokInc))
    {
        OutPtr->Print("++");
        goto more2;
    }
    if(Scan->IfToken(TokDec))
    {
        OutPtr->Print("--");
        goto more2;
    }
    if(Scan->IfToken('('))
    {
        OutPtr->Print("(");
        if(Scan->Token!=')')
        {
            for(;;)
            {
                HlslExpr();
                if(Scan->IfToken(','))
                    OutPtr->Print(",");
                else
                    break;
            }
        }
        Scan->Match(')');
        OutPtr->Print(")");
        goto more2;
    }
}

void wDocument::HlslExpr()
{
    HlslValue();
    const sChar *op;
    do
    {
        op = 0;
        sBool assign = 0;
        if(Scan->IfToken('?'))
        {
            OutPtr->Print(" ? ");
            HlslExpr();
            Scan->Match(':');
            OutPtr->Print(" : ");
            HlslExpr();
        }
        else
        {
            switch(Scan->Token)
            {
            case '+': op = "+"; assign=1; break;
            case '-': op = "-"; assign=1; break;
            case '*': op = "*"; assign=1; break;
            case '/': op = "/"; assign=1; break;
            case '%': op = "%"; assign=1; break;
            case '=': op = "="; break;
            case sTOK_ShiftL: op = "<<"; assign=1; break;
            case sTOK_ShiftR: op = ">>"; assign=1; break;
            case '&': op = "&"; assign=1; break;
            case '|': op = "|"; assign=1; break;
            case '^': op = "^"; assign=1; break;
            case sTOK_DoubleAnd: op = "&&"; break;
            case sTOK_DoubleOr: op = "||"; break;

            case '<': op = "<"; assign=1; break;
            case '>': op = ">"; assign=1; break;
            case sTOK_GE: op = ">="; assign=1; break;
            case sTOK_LE: op = "<="; assign=1; break;
            case sTOK_EQ: op = "=="; break;
            case sTOK_NE: op = "!="; break;
            }
            if(op)
            {
                Scan->Scan();
                OutPtr->Print(op);
                if(assign)
                    if(Scan->IfToken('='))
                        OutPtr->Print("=");

                HlslValue();
            }
        }
    }
    while(op);
}

void wDocument::HlslStmt(sInt indent)
{
    while(Scan->IfToken('['))
    {
        OutPtr->Print("[");
        OutPtr->Print(Scan->ScanName());
        Scan->Match(']');
        OutPtr->Print("]");
    }

    if(Scan->IfToken(TokBreak))
    {
        OutPtr->PrintF("%_break;",indent);
        Scan->Match(';');
    }
    else if(Scan->IfToken(TokContinue))
    {
        OutPtr->PrintF("%_continue;",indent);
        Scan->Match(';');
    }
    else if(Scan->IfToken(TokDiscard))
    {
        OutPtr->PrintF("%_discard;",indent);
        Scan->Match(';');
    }
    else if(Scan->IfToken(TokDo))
    {
        OutPtr->PrintF("%_do(",indent);
        HlslExpr();
        OutPtr->Print(")\n");
        HlslBlock(indent);
        Scan->Match(TokWhile);
        OutPtr->PrintF("%_while(",indent);
        HlslExpr();
        OutPtr->Print(");\n");
    }
    else if(Scan->IfToken(TokFor))
    {
        OutPtr->PrintF("%_for(",indent);
        Scan->Match('(');
        if(HlslType())                // this cant parse "for(int i=0,j=1;;)"
            OutPtr->Print(" ");
        HlslExpr();
        Scan->Match(';');
        OutPtr->Print(";");
        HlslExpr();
        Scan->Match(';');
        OutPtr->Print(";");
        HlslExpr();
        Scan->Match(')');
        OutPtr->Print(")\n");
        HlslBlock(indent);
    }
    else if(Scan->IfToken(TokIf))
    {
        OutPtr->PrintF("%_if(",indent);
        HlslExpr();
        OutPtr->Print(")\n");
        HlslBlock(indent);
        if(Scan->IfToken(TokElse))
        {
            OutPtr->PrintF("%_else\n",indent);
            HlslBlock(indent);
        }
    }
    else if(Scan->IfToken(TokSwitch))
    {
        OutPtr->PrintF("%_switch(",indent);
        HlslExpr();
        OutPtr->Print(")\n");
        HlslBlock(indent);
    }
    else if(Scan->IfToken(TokCase))
    {
        OutPtr->PrintF("%_case ",indent-2);
        HlslExpr();
        OutPtr->Print(":\n");
    }
    else if(Scan->IfToken(TokDefault))
    {
        OutPtr->PrintF("%_default:\n",indent-2);
    }
    else if(Scan->IfToken(TokWhile))
    {
        OutPtr->PrintF("%_while(",indent);
        HlslExpr();
        OutPtr->Print(")\n");
        HlslBlock(indent);
    }
    else if(Scan->IfToken(TokPif))
    {
        Scan->Match('(');
        sTextBuffer *old = OutPtr;
        sInt pred = _Predicate()->Evaluate(Material,Permutation);
        if(!pred)
            OutPtr = &OutAlt;
        Scan->Match(')');
        HlslBlock(indent,1);
        OutPtr = old;
        if(Scan->IfToken(TokPelse))
        {
            if(pred)
                OutPtr = &OutAlt;
            HlslBlock(indent,1);
            OutPtr = old;
        }
    }
    else if(Scan->IfToken(TokReturn))
    {
        OutPtr->PrintF("%_return ",indent);
        if(Scan->Token!=';')
            HlslExpr();
        Scan->Match(';');
        OutPtr->Print(";\n");
    }
    else if(Scan->Token=='{')
    {
        HlslBlock(indent);
    }
    else  // expression or declaration
    {
        sBool simple = HlslIsStorageClass() || HlslIsTypeModifer();
        OutPtr->PrintF("%_",indent);
        if(simple || HlslType())
        {
            while(HlslIsStorageClass())
            {
                Scan->PrintToken(*OutPtr);
                Scan->Scan();
            }
            while(HlslIsTypeModifer())
            {
                Scan->PrintToken(*OutPtr);
                Scan->Scan();
            }
            if(simple)
                HlslType();

            for(;;)
            {
                OutPtr->PrintF(" %s",Scan->ScanName());

                if(Scan->IfToken('['))
                {
                    OutPtr->Print("[");
                    HlslExpr();
                    OutPtr->Print("]");
                    Scan->Match(']');
                }
                if(Scan->IfToken('='))
                {
                    OutPtr->Print(" = ");
                    HlslExpr();
                }
                if(!Scan->IfToken(','))
                    break;
                OutPtr->Print(",");
            }
            Scan->Match(';');
        }
        else
        {
            HlslExpr();
            Scan->Match(';');
        }
        OutPtr->Print(";\n");
    }
}

void wDocument::HlslBlock(sInt indent,sBool pif)
{
    if(Scan->IfToken('{'))
    {
        if(!pif)
            OutPtr->PrintF("%_{\n",indent);
        while(Scan->Token!=sTOK_End && !Scan->IfToken('}'))
            HlslStmt(indent+(pif?0:2));
        if(!pif)
            OutPtr->PrintF("%_}\n",indent);
    }
    else
    {
        HlslStmt(indent+(pif?0:2));
    }
}

void wDocument::HlslTemplateArgs()
{
    if(Scan->IfToken('<'))
    {
        OutPtr->Print("<");
        while(1)
        {
            if(!HlslType())
                HlslValue();
            if(!Scan->IfToken(','))
                break;
            OutPtr->Print(",");
        }
        OutPtr->Print(">");
        Scan->Match('>');
    }
}

void wDocument::HlslDecl()
{
    while(Scan->IfToken('['))
    {
        OutPtr->Print("[");
        for(;;)
        {
            OutPtr->Print(Scan->ScanName());
            while(Scan->IfToken('.'))
                OutPtr->PrintF(".%s",Scan->ScanName());
            if(Scan->IfToken('('))
            {
                OutPtr->Print("(");
                while(1)
                {
                    HlslExpr();
                    if(!Scan->IfToken(','))
                        break;
                    OutPtr->Print(",");
                }
                OutPtr->Print(")");
                Scan->Match(')');
            }
            if(Scan->IfToken(','))
                OutPtr->Print(",");
            else
                break;
        }
        OutPtr->Print("]\n");
        Scan->Match(']');
    }
    while(HlslIsStorageClass())
    {
        Scan->PrintToken(*OutPtr);
        Scan->Scan();
    }
    while(HlslIsTypeModifer())
    {
        Scan->PrintToken(*OutPtr);
        Scan->Scan();
    }
    if(Scan->IfName("precise "))
    {
        OutPtr->Print("precise ");
    }

    sBool found = 0;
    if(Scan->Token==sTOK_Name)
    {
        static const sChar *names[] =
        {
            "SamplerState",
            "Sampler1D",
            "Sampler2D",
            "Sampler3D",
            "SamplerCube",
            "Sampler1DArray",
            "Sampler2DArray",
            "Sampler3DArray",
            "SamplerCubeArray",
            "SamplerComparisonState",
            "Texture1D",
            "Texture1DArray",
            "Texture2D",
            "Texture2DArray",
            "Texture2DMS",
            "Texture2DMSArray",
            "Texture3D",
            "TextureCube",
            "TextureCubeArray",

            "AppendStructuredBuffer",
            "Buffer",
            "ByteAddressBuffer",
            "ConsumeStructuredBuffer",
            "StructuredBuffer",
            "RWBuffer",
            "RWByteAddressBuffer",
            "RWStructuredBuffer",
            "RWTexture1D",
            "RWTexture1DArray",
            "RWTexture2D",
            "RWTexture2DArray",
            "RWTexture3D",
            "RWTextureCube",
            "RWTextureCubeArray",
        };

        for(sInt i=0;i<sCOUNTOF(names) && !found;i++)
        {
            if(sCmpString(names[i],Scan->Name)==0)
                found = 1;
        }
        if(found)
        {
            OutPtr->PrintF("%s",Scan->ScanName());
        }

        HlslTemplateArgs();
    }
    if(!found)
        HlslType();

    sPoolString name = Scan->ScanName();
    OutPtr->PrintF(" %s ",name);

    sBool func = Scan->IfToken('(');
    if(func)
    {
        OutPtr->Print("(");
        if(Scan->Token!=')')
        {
            for(;;)
            {
                while(Scan->Token==TokIn || Scan->Token==TokOut || Scan->Token==TokInOut || Scan->Token==TokUniform || 
                    (Scan->Token==sTOK_Name && (Scan->Name=="point" || Scan->Name=="line" || Scan->Name=="triangle" || Scan->Name=="lineadj" || Scan->Name=="triangleadj")))
                {
                    Scan->PrintToken(*OutPtr);
                    Scan->Scan();
                }

                if(!HlslType())
                    Scan->Error("type expected");

                OutPtr->PrintF(" %s",Scan->ScanName());

                if(Scan->IfToken('['))
                {
                    OutPtr->Print("[");
                    HlslExpr();
                    OutPtr->Print("]");
                    Scan->Match(']');
                }

                if(Scan->IfToken(':'))
                    OutPtr->PrintF(" : %s",Scan->ScanName());

                if(Scan->IfName("linear")) 
                    OutPtr->Print("linear ");
                else if(Scan->IfName("centroid")) 
                    OutPtr->Print("centroid ");
                else if(Scan->IfName("nointerpolation")) 
                    OutPtr->Print("nointerpolation ");
                else if(Scan->IfName("noperspective")) 
                    OutPtr->Print("noperspective ");
                else if(Scan->IfName("sample")) 
                    OutPtr->Print("sample ");

                if(Scan->IfToken('='))
                {
                    OutPtr->Print(" = ");
                    HlslExpr();
                }

                if(Scan->IfToken(','))
                    OutPtr->Print(",");
                else
                    break;
            }
        }
        Scan->Match(')');
        OutPtr->Print(")");
    }

    while(Scan->IfToken('['))
    {
        OutPtr->Print("[");
        HlslExpr();
        OutPtr->Print("]");
        Scan->Match(']');
    }

    if(Scan->IfToken('='))
    {
        OutPtr->Print("=");
        HlslExpr();
    }

    if(Scan->IfToken(':'))
    {
        Scan->Match(TokRegister);
        Scan->Match('(');
        OutPtr->PrintF(" : register(%s)",Scan->ScanName());
        Scan->Match(')');
    }

    if(func)
    {
        if(Scan->Token!='{')
            Scan->Error("'{' expected");
        OutPtr->Print("\n");
        HlslBlock(0);
    }
    else
    {
        Scan->Match(';');
        OutPtr->Print(";\n");
    }
    OutPtr->Print("\n");
}

void wDocument::HlslGlobal()
{
    if(Scan->IfToken(TokPif))
    {
        Scan->Match('(');
        sTextBuffer *old = OutPtr;
        sInt pred = _Predicate()->Evaluate(Material,Permutation);
        if(!pred)
            OutPtr = &OutAlt;
        Scan->Match(')');
        if(Scan->IfToken('{'))
        {
            while(Scan->Token!=sTOK_End && !Scan->IfToken('}'))
                HlslGlobal();
        }
        else
        {
            HlslGlobal();
        }
        OutPtr = old;
    }
    else if(Scan->IfToken(';'))
    {
    }
    else
    {
        HlslDecl();
    }
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void wDocument::HlslPrintMember(wAscMember *mem)
{
    if(mem->Flags & wMF_Extern) OutPtr->Print("extern ");
    if(mem->Flags & wMF_NoInterpolation) OutPtr->Print("nointerpolation ");
    if(mem->Flags & wMF_Precise) OutPtr->Print("precise ");
    if(mem->Flags & wMF_Shared) OutPtr->Print("shared ");
    if(mem->Flags & wMF_GroupShared) OutPtr->Print("groupshared ");
    if(mem->Flags & wMF_Static) OutPtr->Print("static ");
    if(mem->Flags & wMF_Uniform) OutPtr->Print("uniform ");
    if(mem->Flags & wMF_Volatile) OutPtr->Print("volatile ");
    if(mem->Type->Rows>0)
    {
        if(mem->Type->ColumnMajor)
            OutPtr->Print("column_major ");
        else
            OutPtr->Print("row_major ");
    }
    static const sChar *typenames[] =
    { 
        "???","bool","int","uint","float","double",
        "float","float","int","int","uint"
    };
    OutPtr->Print(typenames[mem->Type->Base]);
    if(mem->Type->Rows>0)
        OutPtr->PrintF("%dx",mem->Type->Rows);
    if(mem->Type->Columns>0)
        OutPtr->PrintF("%d",mem->Type->Columns);
    OutPtr->PrintF(" %s",mem->Name);
    if(mem->Type->Array)
        OutPtr->PrintF("[%d]",mem->Type->Array);
}

/****************************************************************************/

sBool wDocument::HlslParse(sInt stage,const sChar *source,sInt line)
{
    Predicate = 0;

    Scan->Init(sSF_CppComment|sSF_EscapeCodes|sSF_MergeStrings);
    Scan->AddDefaultTokens();
    //  Scan->AddToken("",Tok);
    Scan->AddToken("break",TokBreak);
    Scan->AddToken("case",TokCase);
    Scan->AddToken("column_major",TokColumnMajor);
    Scan->AddToken("const",TokConst);
    Scan->AddToken("continue",TokContinue);
    Scan->AddToken("do",TokDo);
    Scan->AddToken("default",TokDefault);
    Scan->AddToken("discard",TokDiscard);
    Scan->AddToken("else",TokElse);
    Scan->AddToken("extern",TokExtern);
    Scan->AddToken("false",TokFalse);
    Scan->AddToken("for",TokFor);
    Scan->AddToken("if",TokIf);
    Scan->AddToken("in",TokIn);
    Scan->AddToken("inout",TokInOut);
    Scan->AddToken("matrix",TokMatrix);
    Scan->AddToken("null",TokNull);
    Scan->AddToken("out",TokOut);
    Scan->AddToken("return",TokReturn);
    Scan->AddToken("register",TokRegister);
    Scan->AddToken("row_major",TokRowMajor);
    Scan->AddToken("static",TokStatic);
    Scan->AddToken("struct",TokStruct);
    Scan->AddToken("switch",TokSwitch);
    Scan->AddToken("typedef",TokTypedef);
    Scan->AddToken("uniform",TokUniform);
    Scan->AddToken("vector",TokVector);
    Scan->AddToken("volatile",TokVolatile);
    Scan->AddToken("while",TokWhile);

    Scan->AddToken("pif",TokPif);
    Scan->AddToken("pelse",TokPelse);

    Scan->AddToken("--",TokDec);
    Scan->AddToken("++",TokInc);

    Scan->Start(source);
    Scan->SetFilename(InputFilename);
    Scan->SetLine(line);

    Out.Clear();
    OutAlt.Clear();
    OutPtr = &Out;
    Typenames.Clear();

    for(auto buf : Material->Buffers)
    {
        if(buf->Kind==wBK_Constants && buf->Shader==stage)
        {
            if(Platform==sConfigRenderDX11)
            {
                sInt reg = 0;
                OutPtr->PrintF("cbuffer cb%d : register(b%d)\n",buf->Slot,buf->Slot);
                OutPtr->Print("{\n");
                for(auto mem : buf->Members)
                {
                    OutPtr->Print("  ");
                    HlslPrintMember(mem);
                    OutPtr->PrintF(" : packoffset(c%d);\n",reg);

                    reg += mem->Type->GetRegisterSize();
                }
                OutPtr->Print("}\n");
            }
            else
            {
                sInt reg = buf->Register;
                for(auto mem : buf->Members)
                {
                    OutPtr->Print("uniform ");
                    HlslPrintMember(mem);
                    OutPtr->PrintF(" : register(c%d);\n",reg);

                    reg += mem->Type->GetRegisterSize();
                }
            }
            OutPtr->Print("\n");
        }
    }
    for(auto buf : Material->Buffers)
    {
        if(buf->Kind==wBK_Struct)
        {
            OutPtr->PrintF("struct %s\n",buf->Name);
            OutPtr->Print("{\n");

            for(auto mem : buf->Members)
            {
                if(mem->Predicate==0 || mem->Predicate->Evaluate(Material,Permutation)!=0)
                {
                    OutPtr->Print("  ");
                    HlslPrintMember(mem);
                    if(!mem->Binding.IsEmpty())
                        OutPtr->PrintF(" : %s",mem->Binding);
                    OutPtr->Print(";\n");
                }
            }

            OutPtr->Print("};\n");
            OutPtr->Print("\n");
            Typenames.Add(buf->Name);
        }
    }

    while(Scan->Token!=sTOK_End)
    {
        HlslGlobal();
    }

    sDPrintF(Scan->ErrorLog.Get());

    if(EnableDump)
        Dump.Print(Out.Get());


    if(Scan->Errors)
        ErrorFlag = 0;
    return !Scan->Errors;
}

/****************************************************************************/


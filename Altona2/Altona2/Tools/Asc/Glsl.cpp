/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Doc.hpp"

// this is generally a mess, i don't really care that much about OGL

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sBool wDocument::GlslIsTypeModifer()
{
    if(Scan->Token==TokColumnMajor) return 1;
    if(Scan->Token==TokRowMajor) return 1;
    if(Scan->Token==TokConst) return 1;
    if(Scan->Token==TokIn) return 1;
    if(Scan->Token==TokOut) return 1;
    if(Scan->Token==TokUniform) return 1;
    if(Scan->Token==TokInOut) return 1;
    if(Scan->Token==sTOK_Name)
    {
        if(Scan->Name=="attribute") return 1;
        if(Scan->Name=="varying") return 1;
        if(Scan->Name=="centroid") return 1;
        if(Scan->Name=="patch") return 1;
        if(Scan->Name=="sample") return 1;
        if(Scan->Name=="smooth") return 1;
        if(Scan->Name=="flat") return 1;
        if(Scan->Name=="noperspective") return 1;
        if(Scan->Name=="invariant") return 1;
    } 
    return 0;
}

sBool wDocument::GlslType()
{
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
        GlslType();
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
        OutPtr->Print("vector<");
        GlslType();
        Scan->Match(',');
        OutPtr->PrintF(",%d>",Scan->ScanInt());
        Scan->Match('>');
        return 1;
    }

    // parse normal type. 

    sBool ok = 0;
    if(Scan->IfName("highp"))
    {
        ok = 1;
        if(Platform==sConfigRenderGLES2)
          OutPtr->Print("highp ");
    }
    else if(Scan->IfName("lowp"))
    {
        ok = 1;
        if(Platform==sConfigRenderGLES2)
          OutPtr->Print("lowp ");
    }
    else if(Scan->IfName("mediump"))
    {
        ok = 1;
        if(Platform==sConfigRenderGLES2)
          OutPtr->Print("mediump ");
    }

    if(Scan->Token!=sTOK_Name)
        return ok;


    sString<64> buffer = (const sChar *) Scan->Name;
    sInt len = sGetStringLen(buffer);

    const sChar *typenames[] = 
    {
        "void","bool","int","uint","float","double",0
    };
    const sChar *vectornames[] = 
    {
        "vec","ivec","uvec","dvec","bvec",0
    };
    const sChar *matrixnames[] = 
    {
        "mat","dmat",0
    };

    const sChar **names = typenames;

    sInt cl = 0;
    if(len>1 && buffer[len-1]>='1' && buffer[len-1]<='4')
    {
        names = vectornames;
        len--;
        if(len>2 && buffer[len-1]=='x' && buffer[len-2]>='1' && buffer[len-2]<='4')
        {
            names = matrixnames;
            len-=2;      
        }
        buffer[len] = 0;
    }

    sBool found = 0;
    for(sInt i=0;names[i] && !found;i++)
        if(sCmpString(names[i],buffer)==0)
            found = 1;

    if(!found) 
        return ok;

    OutPtr->Print(Scan->Name);
    Scan->Scan();
    return 1;
}

void wDocument::GlslValue()
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
        if(!GlslType())
            GlslExpr();
        Scan->Match(')');
        OutPtr->Print(")");
    }
    else if(Scan->Token==sTOK_Name)
    {
        sPoolString name = Scan->ScanName();
        for(auto &r : GlslReplacements)
        {
            if(r.From==name)
            {
                name = r.To;
                break;
            }
        }
        OutPtr->Print(name);
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
        GlslExpr();
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
                GlslExpr();
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

void wDocument::GlslExpr()
{
    GlslValue();
    const sChar *op;
    do
    {
        op = 0;
        sBool assign = 0;
        if(Scan->IfToken('?'))
        {
            OutPtr->Print(" ? ");
            GlslExpr();
            Scan->Match(':');
            OutPtr->Print(" : ");
            GlslExpr();
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

                GlslValue();
            }
        }
    }
    while(op);
}

void wDocument::GlslStmt(sInt indent)
{
    if(Scan->IfToken(TokBreak))
    {
        OutPtr->PrintF("%_break;",indent);
    }
    else if(Scan->IfToken(TokContinue))
    {
        OutPtr->PrintF("%_continue;",indent);
    }
    else if(Scan->IfToken(TokDiscard))
    {
        OutPtr->PrintF("%_discard;",indent);
    }
    else if(Scan->IfToken(TokDo))
    {
        OutPtr->PrintF("%_do(",indent);
        GlslExpr();
        OutPtr->Print(")\n");
        GlslBlock(indent);
        Scan->Match(TokWhile);
        OutPtr->PrintF("%_while(",indent);
        GlslExpr();
        OutPtr->Print(");\n");
    }
    else if(Scan->IfToken(TokFor))
    {
        OutPtr->PrintF("%_for(;",indent);
        Scan->Match('(');
        if(GlslType())                // this cant parse "for(int i=0,j=1;;)"
            OutPtr->Print(" ");
        GlslExpr();
        OutPtr->Print(";");
        GlslExpr();
        OutPtr->Print(";");
        GlslExpr();
        Scan->Match(')');
        OutPtr->Print(")\n");
        GlslBlock(indent);
    }
    else if(Scan->IfToken(TokIf))
    {
        OutPtr->PrintF("%_if(",indent);
        GlslExpr();
        OutPtr->Print(")\n");
        GlslBlock(indent);
        if(Scan->IfToken(TokElse))
        {
            OutPtr->PrintF("%_else\n",indent);
            GlslBlock(indent);
        }
    }
    else if(Scan->IfToken(TokSwitch))
    {
        OutPtr->PrintF("%_switch(",indent);
        GlslExpr();
        OutPtr->Print(")\n");
        GlslBlock(indent);
    }
    else if(Scan->IfToken(TokCase))
    {
        OutPtr->PrintF("%_case ",indent-2);
        GlslExpr();
        OutPtr->Print(":\n");
    }
    else if(Scan->IfToken(TokDefault))
    {
        OutPtr->PrintF("%_default:\n",indent-2);
    }
    else if(Scan->IfToken(TokWhile))
    {
        OutPtr->PrintF("%_while(",indent);
        GlslExpr();
        OutPtr->Print(")\n");
        GlslBlock(indent);
    }
    else if(Scan->IfToken(TokPif))
    {
        Scan->Match('(');
        sTextBuffer *old = OutPtr;
        sInt pred = _Predicate()->Evaluate(Material,Permutation);
        if(!pred)
            OutPtr = &OutAlt;
        Scan->Match(')');
        GlslBlock(indent,1);
        OutPtr = old;
        if(Scan->IfToken(TokPelse))
        {
            if(pred)
                OutPtr = &OutAlt;
            GlslBlock(indent,1);
            OutPtr = old;
        }
    }
    else if(Scan->IfToken(TokReturn))
    {
        OutPtr->PrintF("%_return ",indent);
        HlslExpr();
        Scan->Match(';');
        OutPtr->Print(";\n");
    }
    else  // expression or declaration
    {
        sBool simple = GlslIsTypeModifer();
        OutPtr->PrintF("%_",indent);
        if(simple || GlslType())
        {
            while(GlslIsTypeModifer())
            {
                Scan->PrintToken(*OutPtr);
                Scan->Scan();
            }
            if(simple)
                GlslType();

            for(;;)
            {
                OutPtr->PrintF(" %s",Scan->ScanName());
                if(Scan->IfToken('='))
                {
                    OutPtr->Print(" = ");
                    GlslExpr();
                }
                if(!Scan->IfToken(','))
                    break;
                OutPtr->Print(",");
            }
            Scan->Match(';');
        }
        else
        {
            GlslExpr();
            Scan->Match(';');
        }
        OutPtr->Print(";\n");
    }
}

void wDocument::GlslBlock(sInt indent,sBool pif)
{
    if(Scan->IfToken('{'))
    {
        if(!pif)
            OutPtr->PrintF("%_{\n",indent);
        while(Scan->Token!=sTOK_End && !Scan->IfToken('}'))
            GlslStmt(indent+(pif?0:2));
        if(!pif)
            OutPtr->PrintF("%_}\n",indent);
    }
    else
    {
        GlslStmt(indent+(pif?0:2));
    }
}

void wDocument::GlslDecl()
{
    while(GlslIsTypeModifer())
    {
        Scan->PrintToken(*OutPtr);
        Scan->Scan();
    }

    sBool found = 0;
    if(Scan->Token==sTOK_Name)
    {
        static const sChar *names[] =
        {
            "sampler1D",
            "sampler2D",
            "sampler3D",
            "samplerCube",
            "sampler2DRect",
            "sampler1DShadow",
            "sampler2DShadow",
            "sampler2DRectShadow",
            "sampler1DArray",
            "sampler2DArray",
            "sampler1DArrayShadow",
            "sampler2DArrayShadow",
            "samplerBuffer",
            "sampler2DMS",
            "sampler2DMSArray",
            "samplerCubeShadow",
            "samplerCubeArray",
            "samplerCubeArrayShadow",

            "isampler1D",
            "isampler2D",
            "isampler3D",
            "isamplerCube",
            "isampler2DRect",
            "isampler1DArray",
            "isampler2DArray",
            "isamplerBuffer",
            "isampler2DMS",
            "isampler2DMSArray",
            "isamplerCubeArray",

            "usampler1D",
            "usampler2D",
            "usampler3D",
            "usamplerCube",
            "usampler2DRect",
            "usampler1DArray",
            "usampler2DArray",
            "usamplerBuffer",
            "usampler2DMS",
            "usampler2DMSArray",
            "usamplerCubeArray",
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
    }
    if(!found)
        GlslType();

    sPoolString name = Scan->ScanName();

    if(found) // this is a sampler -> rename
    {
        sString<64> to;
        to.PrintF("%s_%d",name,GlslSamplerCount++);
        GlslReplacements.AddMany(1)->Set(name,to);
        name = to;
    }

    OutPtr->PrintF(" %s ",name);

    sBool func = Scan->IfToken('(');
    if(func)
    {
        OutPtr->Print("(");
        if(Scan->Token!=')')
        {
            for(;;)
            {
                while(Scan->Token==TokIn || Scan->Token==TokOut || Scan->Token==TokInOut || Scan->Token==TokUniform)
                {
                    Scan->PrintToken(*OutPtr);
                    Scan->Scan();
                }
                if(!GlslType())
                    Scan->Error("type expected");
                OutPtr->PrintF(" %s",Scan->ScanName());
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
                    GlslExpr();
                }

                if(Scan->IfToken(','))
                    OutPtr->Print(",");
                else
                    break;
            }
        }
        OutPtr->Print(")");
        Scan->Match(')');
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
        GlslBlock(0);
    }
    else
    {
        Scan->Match(';');
        OutPtr->Print(";\n");
    }
    OutPtr->Print("\n");
}

void wDocument::GlslGlobal()
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
                GlslGlobal();
        }
        else
        {
            GlslGlobal();
        }
        OutPtr = old;
    }
    else if(Scan->IfToken(';'))
    {
    }
    else
    {
        GlslDecl();
    }
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void wDocument::GlslPrintType(wAscType *type)
{
  static const sChar *typenames_gles[] =
  {
    "???","bool","int","uint","float","double",
    "mediump float","lowp float","mediump int","lowp int","mediump uint"
  };
  
  static const sChar *typenames_gl[] =
  {
    "???","bool","int","uint","float","double",
    "float","float","int","int","uint"
  };

  static const sChar *vectornames_gles[] =
  {
    "???","bvec","ivec","uvec","vec","dvec",
    "mediump vec","lowp vec","mediump ivec","lowp ivec","mediump uvec"
  };
  
  static const sChar *vectornames_gl[] =
  {
    "???","bvec","ivec","uvec","vec","dvec",
    "vec","vec","ivec","ivec","uvec"
  };
  
  static const sChar *matrixnames_gles[] =
  {
    "???","???","???","???","mat","dmat",
    "mat","mat","???","???","???"
  };

  static const sChar *matrixnames_gl[] =
  {
    "???","???","???","???","mat","dmat",
    "mat","mat","???","???","???"
  };
  
  const sChar **typenames = typenames_gl;
  const sChar **vectornames = vectornames_gl;
  const sChar **matrixnames = matrixnames_gl;

  if(Platform==sConfigRenderGLES2)
  {
    typenames = typenames_gles;
    vectornames = vectornames_gles;
    matrixnames = matrixnames_gles;
  }
  
  
  if(type->Rows>1)
  {
    if(type->ColumnMajor)
      OutPtr->PrintF("column_major %s%dx%d",matrixnames[type->Base],type->Columns,type->Rows);
    else
      OutPtr->PrintF("row_major %s%dx%d",matrixnames[type->Base],type->Columns,type->Rows);
  }
  else if(type->Columns>1)
  {
    OutPtr->PrintF("%s%d",vectornames[type->Base],type->Columns);
  }
  else
  {
    OutPtr->PrintF("%s",typenames[type->Base]);
  }
}

/****************************************************************************/

sBool wDocument::GlslParse(sInt stage,const sChar *source,sInt line)
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

    GlslSamplerCount = 0;
    GlslReplacements.Clear();
    Out.Clear();
    OutAlt.Clear();
    OutPtr = &Out;
    Typenames.Clear();

    if(Platform==sConfigRenderGLES2)
        Out.Print("#version 100\n");
    else
        Out.Print("#version 120\n");

    sString<64> from;
    sString<1024> to;
    sChar shaderletter = stage==sST_Vertex ? 'v' : 'p';
    const sChar *swizzle[5] = { "x","x","xy","xyz","xyzw" };
    for(auto buf : Material->Buffers)
    {
        if(buf->Kind==wBK_Constants && buf->Shader==stage)
        {
            sInt size = 0;
            for(auto mem : buf->Members)
            {
                to.Clear();

                if(mem->Type->Array<2 && mem->Type->Base==wTB_Float)
                {
                    if(mem->Type->Rows<2)
                    {
                        to.PrintF("%cc%d[%d].%s",shaderletter,buf->Slot,size,swizzle[mem->Type->Columns]);
                    }
                    else
                    {
                        if(mem->Type->Columns==mem->Type->Rows)
                            to.PrintF("mat%d(",mem->Type->Columns);
                        else
                            to.PrintF("mat%dx%d(",mem->Type->Columns,mem->Type->Rows);
                        for(sInt i=0;i<mem->Type->Columns;i++)
                        {
                            if(i>0)
                                to.Add(",");
                            if(mem->Type->ColumnMajor)
                            {
                                to.AddF("%cc%d[%d].%s",shaderletter,buf->Slot,size+i,swizzle[mem->Type->Columns]);
                            }
                            else
                            {
                                to.AddF("vec%d(",mem->Type->Rows);
                                for(sInt j=0;j<mem->Type->Rows;j++)
                                {
                                    if(j>0)
                                        to.Add(",");
                                    to.AddF("%cc%d[%d].%c",shaderletter,buf->Slot,size+j,("xyzw")[i]);
                                }
                                to.AddF(")");
                            }
                        }
                        to.AddF(")");
                    }
                }

                if(to.IsEmpty())
                {
                    ErrorFlag = 1;
                    sDPrintF("can not convert type %s%dx%d[%d] to glsl\n",mem->Type->ColumnMajor?"column_major ":"",mem->Type->Columns,mem->Type->Rows,mem->Type->Array);
                }
                else
                {
                    GlslReplacements.AddMany(1)->Set(mem->Name,to);
                }

                size += mem->Type->GetRegisterSize();
            }
            if(Platform==sConfigRenderGLES2)
            {
                if (stage==sST_Vertex)
                    OutPtr->PrintF("uniform vec4 %cc%d[%d];\n",shaderletter,buf->Slot,size);
                else
                    OutPtr->PrintF("uniform mediump vec4 %cc%d[%d];\n",shaderletter,buf->Slot,size);
            }
            else
            {
                OutPtr->PrintF("uniform vec4 %cc%d[%d];\n",shaderletter,buf->Slot,size);
            }
        }
    }
    for(auto buf : Material->Buffers)
    {
        if(buf->Kind==wBK_Struct)
        {
            if(stage==sST_Vertex && buf->Name=="ia2vs")
            {
                sInt n = 0;
                for(auto mem : buf->Members)
                {
                    if(mem->Predicate==0 || mem->Predicate->Evaluate(Material,Permutation))
                    {
                        OutPtr->PrintF("attribute ");
                        GlslPrintType(mem->Type);
                        from.PrintF("i_%s",mem->Name);
                        to.PrintF("%s_%d",mem->Name,n++);
                        OutPtr->PrintF(" %s;\n",to);
                        GlslReplacements.AddMany(1)->Set(from,to);
                    }
                }
            }
            else if(buf->Name=="vs2ps")
            {
                for(auto mem : buf->Members)
                {
                    if(mem->Predicate==0 || mem->Predicate->Evaluate(Material,Permutation))
                    {
                        OutPtr->PrintF("varying ");
                        GlslPrintType(mem->Type);
                        OutPtr->PrintF(" %s;\n",mem->Binding);
                        from.PrintF("%c_%s",stage==sST_Vertex ? 'o' : 'i',mem->Name);
                        GlslReplacements.AddMany(1)->Set(from,mem->Binding);
                    }
                }
            }
        }
    }

    while(Scan->Token!=sTOK_End)
    {
        GlslGlobal();
    }

    sDPrintF(Scan->ErrorLog.Get());

    if(EnableDump)
        Dump.Print(Out.Get());


    if(Scan->Errors)
        ErrorFlag = 0;
    return !Scan->Errors;
}

/****************************************************************************/


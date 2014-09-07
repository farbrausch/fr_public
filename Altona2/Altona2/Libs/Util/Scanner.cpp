/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Util/Scanner.hpp"

namespace Altona2 {

/****************************************************************************/

sScanner::sScannerSource::sScannerSource()
{
    ScanPtr = 0;
    Line = 1;
    Filename = "";
    BeginOfFile = 1;
}

/****************************************************************************/

sScanner::sScannerSourceText::sScannerSourceText(const char *buffer,bool deleteme)
{
    Start = buffer;
    ScanPtr = Start;
    DeleteMe = deleteme;
}

sScanner::sScannerSourceText::~sScannerSourceText()
{
    if(DeleteMe)
        delete[] Start;
}

/****************************************************************************/
/****************************************************************************/

sScanner::sScanner()
{
    Stream = 0;
    ErrorCallback = 0;
    ErrorCallbackPtr = 0;
    Init(sSF_CppComment);
}

sScanner::~sScanner()
{
    delete Stream;
}

void sScanner::Init(int flags)
{
    Stop();
    Flags = flags;
    TokensSym.Clear();
    TokensName.Clear();

};

void sScanner::Stop()
{
    Token = 0;
    ValI = 0;
    ValF = 0;
    Name = "";
    String = "";
    ValueString = "";
    ValueSuffix = "";
    Notes = 0;
    Warnings = 0;
    Errors = 0;
    ErrorLog.Clear();
    Includes.DeleteAll();
    delete Stream; Stream = 0;
}

void sScanner::SortTokens()
{
    //  sSortDown(TokensSym,&ScannerToken::Length);
    //  sHeapSort(sAll(TokensSym),sMemberGreater(&ScannerToken::Length));
    TokensSym.QuickSort([](const ScannerToken &a,const ScannerToken &b){return a.Length>b.Length;});
}

void sScanner::Start(const char *text)
{
    Stop();
    Stream = new sScannerSourceText(text,0);
    SortTokens();
    TokenLoc.File = LastLoc.File = Stream->Filename;
    TokenLoc.Line = LastLoc.Line = Stream->Line;
    Scan();
}


bool sScanner::StartFile(const char *filename)
{
    char *buffer = sLoadText(filename);
    if(buffer==0)
        return 0;

    Stop();
    Stream = new sScannerSourceText(buffer,1);
    Stream->Filename = filename;
    SortTokens();
    Scan();
    return 1;
}

bool sScanner::IncludeFile(const char *filename)
{
    char *buffer = sLoadText(filename);
    if(buffer==0)
        return 0;

    Stream->ScanPtr = TokenScan;
    Includes.AddTail(Stream);
    Stream = new sScannerSourceText(buffer,1);
    Stream->Filename = filename;
    Scan();
    return 1;
}

void sScanner::AddToken(const char *name,int token)
{
    ScannerToken tok;
    tok.Name = name;
    tok.Token = token;
    tok.Length = sGetStringLen(name);

    bool IsName = 1;
    if(!sIsLetter(*name++))
        IsName=0;
    while(*name && IsName)
    {
        if(!sIsLetter(*name) && !sIsDigit(*name))
            IsName = 0;
        name++;
    }

    if(IsName)
        TokensName.AddTail(tok);
    else
        TokensSym.AddTail(tok);
}

void sScanner::RemoveToken(const char *name)
{
    for(auto &tok : TokensName)
    {
        if (sCmpString(name, tok.Name) == 0)
        {
            TokensName.Rem(tok);
            return;
        }
    }
    for(auto &tok : TokensSym)
    {
        if (sCmpString(name, tok.Name) == 0)
        {
            TokensSym.Rem(tok);
            return;
        }
    }
}

void sScanner::AddTokens(const char *SingleCharacterTokens)
{
    char buffer[2];
    for(int i=0;SingleCharacterTokens[i];i++)
    {
        buffer[0] = SingleCharacterTokens[i];
        buffer[1] = 0;
        AddToken(buffer,SingleCharacterTokens[i]);
    }
}

void sScanner::AddDefaultTokens()
{
    AddToken(">>",sTOK_ShiftR);
    AddToken("<<",sTOK_ShiftL);
    AddToken("==",sTOK_EQ);
    AddToken("!=",sTOK_NE);
    AddToken("<=",sTOK_LE);
    AddToken(">=",sTOK_GE);
    AddToken("..",sTOK_Ellipses);
    AddToken("::",sTOK_Scope);
    AddToken("&&",sTOK_DoubleAnd);
    AddToken("||",sTOK_DoubleOr);

    AddTokens("+-*/%^~,.;:!?=<>()[]{}&|");
}

void sScanner::LogMsg(const sScannerLoc &loc,const char *name,const char *sev)
{
    sString<sFormatBuffer> msg;

    if(ErrorCallback)
        (*ErrorCallback)(ErrorCallbackPtr,loc.File,loc.Line,name);

    if(!loc.File.IsEmpty())
        msg.PrintF("%s(%d): %s: %s\n",loc.File,loc.Line,sev,name);
    else
        msg.PrintF("line %d: %s: %s\n",loc.Line,sev,name);

    ErrorLog.Print(msg);

    if(!(Flags & sSF_Quiet))
    {
        if(sConfigShell)
            sPrint(msg);
        else
            sDPrint(msg);
    }
}

void sScanner::Error0(const char *name)
{
    LogMsg(GetLoc(),name,"ERROR");

    Errors++;
}
void sScanner::Warn0(const char *name)
{
    LogMsg(GetLoc(),name,"WARNING");
    Warnings++;
}
void sScanner::Note0(const char *name)
{
    LogMsg(GetLoc(),name,"NOTE");
    Notes++;
}

void sScanner::Error0(const sScannerLoc &loc,const char *name)
{
    LogMsg(loc,name,"ERROR");

    Errors++;
}
void sScanner::Warn0(const sScannerLoc &loc,const char *name)
{
    LogMsg(loc,name,"WARNING");
    Warnings++;
}
void sScanner::Note0(const sScannerLoc &loc,const char *name)
{
    LogMsg(loc,name,"NOTE");
    Notes++;
}

/****************************************************************************/

char sScanner::DirtyPeek()
{
    Stream->Refill();
    const char *s = Stream->ScanPtr;

morespace:
    // skip whitespace
    while(*s==' ' || *s=='\n' || *s=='\r' || *s=='\t')
        s++;

    // check end of file
    if(*s==0)
        return 0;

    // check comments
    if(Flags & sSF_CppComment)
    {
        if(s[0]=='/' && s[1]=='*')
        {
            s+=2;
            while(*s && !(s[0]=='*' && s[1]=='/'))
                s++;
            if(*s==0)
                return 0;
            s+=2;
            goto morespace;
        }
        if(s[0]=='/' && s[1]=='/')
        {
            s+=2;
            while(*s!=0 && *s!='\n' && *s!='\r')
                s++;
            goto morespace;
        }
    }
    if( ((Flags & sSF_SemiComment) && *s==';') || ((Flags & sSF_NumberComment) && *s=='#') )
    {
        s++;
        while(*s!=0 && *s!='\n' && *s!='\r')
            s++;
        goto morespace;
    }

    return *s;
}

int sScanner::Scan()
{
    static const int MaxValLen = sCOUNTOF(ValueString);

    int newline;
    sString<256> &bp = ValueString;
    int bi=0;

    Token = sTOK_Error;
    ValI = 0;
    ValF = 0;
    newline = 0;
    LastLoc = TokenLoc;
    TokenLoc.File = Stream->Filename;
    TokenLoc.Line = Stream->Line;


morespace:

    Stream->Refill();

    // save position for token (before whitespace, for sTOK_END)

    TokenLine = Stream->Line;
    TokenFile = Stream->Filename;
    TokenScan = Stream->ScanPtr;

    // skip whitespace
    const char * __restrict s = Stream->ScanPtr;

    while(*s==' ' || *s=='\n' || *s=='\r' || *s=='\t')
    {
        if(*s=='\n')
        {
            Stream->Line++;
            newline = 1;
        }
        s++;
    }

    // check end of file
    if(*s==0)
    {
        if(Includes.GetCount()>0)
        {
            Stream->ScanPtr = s;
            delete Stream;
            Stream = Includes.RemTail();
            goto morespace;
        }
        else
        {
            Token = sTOK_End;
            goto ende;
        }    
    }

    // check comments
    if(Flags & sSF_CppComment)
    {
        if(s[0]=='/' && s[1]=='*')
        {
            s+=2;
            while(s[0] && !(s[0]=='*' && s[1]=='/'))
            {
                if(*s=='\n')
                    Stream->Line++;
                s++;
            }
            if(*s==0)
            {
                Error("eol reached in /* */ comment");
                goto ende;
            }
            else
            {
                s+=2;
            }
            Stream->ScanPtr = s;
            goto morespace;
        }
        if(s[0]=='/' && s[1]=='/')
        {
            s+=2;
            while(*s!=0 && *s!='\n' && *s!='\r')
                s++;
            newline = 1;
            Stream->ScanPtr = s;
            goto morespace;
        }
    }

    // simple preprocessor stuff

    if((Flags & sSF_Cpp) && (newline|Stream->BeginOfFile) && *s=='#')
    {
        if(sCmpStringLen(s,"#line ",6)==0)
        {
            s += 6;
            while(*s==' ') s++;
            int line;

            if(sScanValue((const char *&) s,line))
            {
                Stream->Line = line;
                while(*s==' ') s++;
                if(*s=='"')
                {
                    s++;
                    char buffer[sMaxPath];
                    int i=0;
                    while(s[i]!='"' && s[i]!=0 && i<255)
                    {
                        buffer[i] = s[i];
                        i++;
                    }
                    s+=i;
                    if(*s=='"')
                    {
                        s++;
                        buffer[i] =0 ;
                        Stream->Filename = sPoolString(buffer);

                        while(*s==' ') s++;
                        if(*s=='\n' || *s==0)
                        {
                            Stream->ScanPtr = s;
                            goto morespace;
                        }
                    }
                }
            }
            Error("error in #line directive");
            goto ende;
        }
        if(sCmpStringLen(s,"#error ",7)==0)
        {
            Error("#error directive");
            goto ende;
        }    
    }

    // full line comments.

    if( ((Flags & sSF_SemiComment) && *s==';') || ((Flags & sSF_NumberComment) && *s=='#') )
    {
        s+=1;
        while(*s!=0 && *s!='\n' && *s!='\r')
            s++;
        newline = 1;
        Stream->ScanPtr = s;
        goto morespace;
    }

    // save position for token (again, after whitespace)

    Stream->BeginOfFile = 0;
    TokenLine = Stream->Line;
    TokenFile = Stream->Filename;
    TokenScan = Stream->ScanPtr;

    // send newline tokens
    if(newline && (Flags & sSF_Newline))
    {
        Token = sTOK_Newline;
        goto ende;
    }

    // token?

    if(!(s[0]=='.' && sIsDigit(s[1])))
    {
        for(auto &st : TokensSym)           // this is a performance hotspot! make it table driven!
        {
            if(sCmpMem(s,st.Name,st.Length*sizeof(char))==0)
            {
                Token = st.Token;
                s += st.Length;
                goto ende;
            }
        }
    }

    // strings

    if(*s=='"')
    {
        s++;
        int i=0;
        while(*s!='"' && *s)
        {
            if(*s=='\n')
                Stream->Line++;
            int c = *s;
            if(c=='\\' && (Flags&sSF_EscapeCodes))
            {
                s++;
                c = *s;

                switch(c)
                {
                case 'a': c = '\a'; break;    // BELL
                case 'b': c = '\b'; break;    // backspace
                case 'f': c = '\f'; break;    // formfeed
                case 'n': c = '\n'; break;    // newline
                case 'r': c = '\r'; break;    // CR
                case 't': c = '\t'; break;    // tab
                case 'v': c = '\v'; break;    // vertical tab
                case '\'': c = '\''; break;
                case '\"': c = '\"'; break;
                case '\\': c = '\\'; break;
                case '\?': c = '\?'; break;
                case 0: goto ende;
                default:
                    {
                        if(s[0]>='0' && s[0]<='7' && s[1]>='0' && s[1]<='7' && s[2]>='0' && s[2]<='7')
                        {
                            c = (s[0]&7)*64 + (s[1]&7)*8 + (s[0]&7);
                            s+=2;
                        }
                        else if((s[0]=='x'||s[0]=='u') && sIsHex(s[1]) && sIsHex(s[2]) && sIsHex(s[3]) && sIsHex(s[4]))
                        {
                            char b[5];
                            b[0] = s[1];
                            b[1] = s[2];
                            b[2] = s[3];
                            b[3] = s[4];
                            b[4] = 0;
                            const char *bb = b;
                            sScanHex(bb,c);
                            s+=4;
                        }
                        else if((s[0]=='x'||s[0]=='u') && sIsHex(s[1]) && sIsHex(s[2]))
                        {
                            char b[5];
                            b[0] = s[1];
                            b[1] = s[2];
                            b[2] = 0;
                            const char *bb = b;
                            sScanHex(bb,c);
                            s+=2;
                        }
                    }
                    break;
                }
            }
            if(c!='\r')
            {
                TempString[i]=c;
                i++;
            }
            s++;
        }
        String.Init(TempString,i);

        if(*s=='"')
        {
            Token=sTOK_String;
            s++;
        }
        goto ende;
    }

    // char?

    if(*s=='\'')
    {
        s++;
        if(*s=='\\' && (Flags&sSF_EscapeCodes))
        {
            s++;
            switch(*s)
            {
            case 'n': ValI = '\n'; break;
            case 'r': ValI = '\r'; break;
            case 'f': ValI = '\f'; break;
            case 't': ValI = '\t'; break;
            case '"': ValI = '"'; break;
            case '\\': ValI = '\\'; break;
            case '0': ValI = '\0'; break;
            case 0: goto ende;
            default: ValI = *s;
            }
            s++;
        }
        else if(*s!=0)
        {
            ValI = *s;
            s++;
        }
        else
        {
            goto ende;
        }
        if(*s!='\'')
            goto ende;
        s++;
        Token = sTOK_Char;
        goto ende;
    }

    // names

    if((*s>='a' && *s<='z') || 
        (*s>='A' && *s<='Z') || 
        (*s>=0xa0 && *s<=0xff && (Flags&sSF_Umlauts)) ||
        *s=='_')
    {
        int i = 0;
        while((*s>='a' && *s<='z') || 
            (*s>='A' && *s<='Z') ||
            (*s>='0' && *s<='9') || 
            (*s>=0xa0 && *s<=0xff && (Flags&sSF_Umlauts)) ||
            *s=='_')
        {
            TempString[i++]=*s++;
        }
        Name.Init(TempString,i);
        Token = sTOK_Name;

        for(auto &st : TokensName)
        {
            if(sCmpString(Name,st.Name)==0)
            {
                Token = st.Token;
                break;
            }
        }
        goto ende;
    }

    // hex numbers

    if(s[0]=='0' && (s[1]=='x' || s[1]=='X'))
    {
        bp[bi++]=s[0];
        bp[bi++]=s[1];

        s+=2;
        while((*s>='0' && *s<='9') ||
            (*s>='a' && *s<='f') ||
            (*s>='A' && *s<='F'))
        {
            if(bi<MaxValLen-2) bp[bi++]=*s;
            ValI = ValI*16;
            if(*s<='9') ValI += (*s&15);
            else ValI += 10+((*s-'a')&15);
            s++;
        }
        bp[bi++]=0;

        Token = sTOK_Int;
        goto ende;
    }

    if(s[0]=='0' && (s[1]=='b' || s[1]=='B'))
    {
        bp[bi++]=s[0];
        bp[bi++]=s[1];

        s+=2;
        while(*s>='0' && *s<='1')
        {
            if(bi<MaxValLen-2) bp[bi++]=*s;
            ValI = ValI*2;
            ValI += (*s&1);
            s++;
        }
        bp[bi++]=0;

        Token = sTOK_Int;
        goto ende;
    }

    // hashcolors

    if(*s=='#' && (Flags & sSF_HashColors))
    {
        bp[bi++]=s[0];

        int digits = 0;
        s+=1;
        while((*s>='0' && *s<='9') ||
            (*s>='a' && *s<='f') ||
            (*s>='A' && *s<='F'))
        {
            if(bi<MaxValLen-2) bp[bi++]=*s;
            ValI = ValI*16;
            if(*s<='9') ValI += (*s&15);
            else ValI += 10+((*s-'a')&15);
            s++;
            digits++;
        }
        bp[bi++]=0;
        int argb[4];
        argb[3] = 0xf;
        switch(digits)
        {
        case 4:
            argb[3] = (ValI>>12)&0xf;
            break;
        case 3:
            argb[2] = (ValI>>8)&0xf;
            argb[1] = (ValI>>4)&0xf;
            argb[0] = (ValI>>0)&0xf;
            ValI = (argb[3]<<24) | (argb[2]<<16) | (argb[1]<<8) | (argb[0]<<0);
            ValI = (ValI << 4) | ValI;
            break;
        case 6:
            ValI = ValI | 0xff000000;
            break;
        case 8:
            break;
        default:
            Error("illegal number of hex digits in hashcolor");
            break;
        }

        Token = sTOK_HashColor;
        goto ende;
    }

    // numbers

    if((*s>='0' && *s<='9') || s[0]=='.' && sIsDigit(s[1]))
    {
        Token = sTOK_Int;
        while(*s>='0' && *s<='9')
        {
            ValI = ValI*10+(*s-'0');
            ValF = ValF*10+(*s-'0');
            if(bi<MaxValLen-2) bp[bi++]=*s;
            s++;
        }

        if(s[0]=='.' && s[1]!='.'/*&& ((s[1]>='0' && s[1]<='9') || s[1]=='#')*/)
        {
            Token = sTOK_Float;
            if(bi<MaxValLen-2) bp[bi++]=*s;
            s++;
            double dec=1.0f;
            if (*s == '#')
            {
                // catch float infinity 
                if(bi<MaxValLen-2) bp[bi++]=*s;
                s++;
                if (s[0] == 'I' && s[1] == 'N' && s[2] == 'F')
                {
                    if(bi<MaxValLen-2) bp[bi++]='I';
                    if(bi<MaxValLen-2) bp[bi++]='N';
                    if(bi<MaxValLen-2) bp[bi++]='F';
                    s+=3;
                    while(*s>='0' && *s<='9') s++;
                    ValF = sRawCast<float,uint>(0x7f800000);
                }
                else
                    Token = sTOK_Error;     // invalid infinity float
            }
            else
            {
                while(*s>='0' && *s<='9')
                {
                    dec = dec/10;
                    ValF += float(dec*(*s-'0'));
                    if(bi<MaxValLen-2) bp[bi++]=*s;
                    s++;
                }
            }
        }
        // float with exponent
        if (*s=='e')
        {
            Token = sTOK_Float;
            if(bi<MaxValLen-2) bp[bi++]=*s;
            s++;
            int exp = 0;
            int sign = 1;
            // sign
            if (*s=='-')
            {
                if(bi<MaxValLen-2) bp[bi++]=*s;
                sign = -1;
                s++;
            }
            else if (*s=='+')
            {
                if(bi<MaxValLen-2) bp[bi++]=*s;
                s++;
            }
            // exponent
            while (*s>='0' && *s<='9')
            {
                exp = exp*10 + (*s -'0');
                if(bi<MaxValLen-2) bp[bi++]=*s;
                s++;
            }
            exp *= sign;
            ValF *= sPow(10, float(exp));
        }

        char *suffix = bp+bi;
        if(Flags & sSF_TypedNumbers)
        {
            while(sIsLetter(*s))
            {
                if(bi<MaxValLen-2) bp[bi++]=*s;
                s++;
            }
        }
        bp[bi++] = 0;
        ValueSuffix = suffix;
    }

    if(Token==sTOK_Error)
    {
        Error("illegal character in input");
        s++;
    }
ende:
    Stream->ScanPtr = s;
    return Token;
}

void sScanner::PrintToken(sTextLog &tb)
{
    const ScannerToken *st;
    switch(Token)
    {
    case sTOK_End:
        tb.Print("[END] ");
        break;
    case sTOK_Error:
        tb.Print("[ERROR] ");
        break;
    case sTOK_Int:
        tb.PrintF("%d ",ValI);
        break;
    case sTOK_Float:
        tb.PrintF("%f ",ValF);
        break;
    case sTOK_Name:
        tb.PrintF("%s ",Name);
        break;
    case sTOK_String:
        tb.PrintF("\"%s\" ",String);
        break;
    case sTOK_Char:
        tb.PrintF("%d ",ValI);
        break;
    default:
        {
            int tok = Token;
            st = TokensSym.FindValue([=](const ScannerToken &a){return a.Token==tok;});
            if(!st)
                st = TokensName.FindValue([=](const ScannerToken &a){return a.Token==tok;});
            tb.PrintF("%s ",st ? (const char*)st->Name : "???");
        }
        break;
    }
}

/****************************************************************************/

int sScanner::ScanInt()
{
    int r;
    int sign = 1;
    if(IfToken('-'))
        sign = -1;
    if(Token==sTOK_Int || Token==sTOK_Char)
    {
        r = ValI;
        Scan();
    }
    else
    {
        r = 0;
        Error("integer expected");
    }
    return r*sign;
}

float sScanner::ScanFloat()
{
    float r;
    float sign = 1;
    if(IfToken('-')) 
        sign = -1;
    if(Token==sTOK_Int)   
    {
        r = float(ValI);
        Scan();
    }
    else if(Token==sTOK_Float)
    {
        r = ValF;
        Scan();
    }
    else
    {
        r = 0;
        Error("float expected");
    }
    return r*sign;
}

int sScanner::ScanChar()
{
    int r;
    if(Token==sTOK_Char)
    {
        r = ValI;
    }
    else
    {
        r = 0;
        Error("character literal expected");
    }
    Scan();
    return r;
}

bool sScanner::ScanRaw(sPoolString &ps, int opentoken, int closetoken)
{
    sTextLog tb;
    bool result = ScanRaw(tb, opentoken, closetoken);
    ps = tb.Get();
    return result;
}

bool sScanner::ScanRaw(sTextLog &tb, int opentoken, int closetoken)
{
    int count;
    int startLine = Stream->Line;

    if(Token!=opentoken)
    {
        Error("scan raw: open token missing");
        return 0;
    }

    count = 1;
    char c;
    const char * __restrict s = Stream->ScanPtr;

    while((c=*s)!=0)
    { 
        if (c==char(closetoken)) 
        {
            count--;
            if (count==0) break;
        }
        else if (c==char(opentoken)) 
        {
            count++;
        }

        // skip comments

        if (Flags & sSF_CppComment)
        {
            char next =s[1];
            if (c=='/' && next=='*')
            {
                tb.PrintChar(c); s++; c = *s;
                tb.PrintChar(c); s++; 
                while ((c=*s) != 0 && !(c=='*' && s[1]=='/'))
                {
                    if (c=='\n')
                        Stream->Line++;

                    tb.PrintChar(c); s++; 
                }
                if(c==0)
                {
                    Error("eof reached in /* */ comment");
                    return 0;
                }
                else
                {
                    tb.PrintChar(c); s++; c = *s;
                    tb.PrintChar(c); s++; 
                }
            }
            else if (c=='/' && next=='/')
            {
                tb.PrintChar(c); s++; c = *s;
                tb.PrintChar(c); s++; 
                while ((c=*s) != 0 && c != '\n' && c != '\r')
                {
                    tb.PrintChar(c); s++;
                }
            }
        }

        // end of comment skipping

        // skip strings

        if (c=='\"')
        {
            char last = c;
            tb.PrintChar(c); s++; 
            while ((c=*s) != 0 && (c != '\"' || last == '\\'))
            {
                if (c == '\n') Stream->Line++;
                last = c;
                tb.PrintChar(c); s++;
            }
        }
        if (c=='\'')
        {
            char last = c;
            tb.PrintChar(c); s++; 
            while ((c=*s) != 0 && (c != '\'' || last == '\\'))
            {
                if (c == '\n') Stream->Line++;
                last = c;
                tb.PrintChar(c); s++;
            }
        }

        // end of string skipping

        if (*s=='\n') 
            Stream->Line++;
        tb.PrintChar(*s);
        s++;
    }

    Stream->ScanPtr = s;

    if (count != 0)
    {
        sString<64> msg; sSPrintF(msg, "scan raw: %d %s token(s) missing, starting at line %d", count, (count > 0) ? "close" : "open", startLine);
        Error(msg);
    }

    Match(opentoken);
    Match(closetoken);

    return 1;
}

bool sScanner::ScanLine (sTextLog &tb)
{
    char c;
    while((c=*(Stream->ScanPtr))!=0)
    { 
        if (c=='\n')
        {
            Scan();
            return 1;
        }
        tb.PrintChar(c);
        Stream->ScanPtr++;
    }

    sString<64> msg; sSPrintF(msg, "scan line: premature end of file reached");
    Error(msg);

    return 0;
}

bool sScanner::ScanName(sPoolString &ps)
{
    bool r;
    if(Token==sTOK_Name)
    {
        ps = Name;
        r = 1;
    }
    else
    {
        ps = "";
        Error("name expected");
        r = 0;
    }
    Scan();
    return r;
}

bool sScanner::ScanName(const sStringDesc &ps)
{
    bool r;
    if(Token==sTOK_Name)
    {
        sCopyString(ps,Name);
        r = 1;
    }
    else
    {
        sCopyString(ps,"");
        Error("name expected");
        r = 0;
    }
    Scan();
    return r;
}

bool sScanner::ScanString(sPoolString &ps)
{
    bool r;
    if(Token==sTOK_String)
    {
        ps = String;
        r = 1;
        Scan();

        while(Token==sTOK_String && (Flags & sSF_MergeStrings))
        {
            TempString = ps;
            TempString.Add(String);
            ps = TempString;

            Scan();
        }
    }
    else
    {
        ps = "";
        Error("string expected");
        r = 0;
        Scan();
    }
    return r;
}

bool sScanner::ScanString(const sStringDesc &ps)
{
    bool r;
    if(Token==sTOK_String)
    {
        sCopyString(ps,String);
        r = 1;
        Scan();

        while(Token==sTOK_String && (Flags & sSF_MergeStrings))
        {
            TempString = ps.Buffer;
            TempString.Add(String);
            sCopyString(ps,TempString);

            Scan();
        }
    }
    else
    {
        sCopyString(ps,"");
        Error("string expected");
        r = 0;
        Scan();
    }
    return r;
}

bool sScanner::ScanNameOrString(sPoolString &ps)
{
    if(Token==sTOK_Name)
        return ScanName(ps);
    else
        return ScanString(ps);
}

bool sScanner::ScanNameOrString(const sStringDesc &ps)
{
    if(Token==sTOK_Name)
        return ScanName(ps);
    else
        return ScanString(ps);
}

sPoolString sScanner::ScanName()
{
    sPoolString name;
    ScanName(name);
    return name;
}

sPoolString sScanner::ScanString()
{
    sPoolString name;
    ScanString(name);
    return name;
}

sPoolString sScanner::ScanNameOrString()
{
    sPoolString name;
    ScanNameOrString(name);
    return name;
}

void sScanner::Match(int token)
{
    if(token==Token) 
        Scan();
    else
    {
        for(auto &t : TokensSym)
        {
            if(t.Token==token)
            {
                Error("syntax error: `%s' expected",t.Name);
                return;
            }
        }
        for(auto &t : TokensName)
        {
            if(t.Token==token)
            {
                Error("syntax error: `%s' expected",t.Name);
                return;
            }
        }
        Error("syntax error");
    }

    // this is broken: user tokens start with 32 and doesn't need to equal their printable ascii character
    // so just do the slow search over all tokens for a correct error message
    //if(token>=32 && token<256) // printable ascii character
    //  Error("syntax error: `%c' expected",token);
    //else
    //  Error("syntax error");
}

void sScanner::MatchName(const char *name)
{
    if(!IfName(name))
        Error("%s expected",name);
}

void sScanner::MatchString(const char *name)
{
    if(!IfString(name))
        Error("string %q expected",name);
}

void sScanner::MatchInt(int n)
{
    if(Token==sTOK_Int && ValI==n)
        Scan();
    else
        Error("integer %d expected",n);
}

/****************************************************************************/

bool sScanner::IfToken(int tok)
{
    if(Token==tok)
    {
        Scan();
        return 1;
    }
    else
    {
        return 0;
    }
}

bool sScanner::IfName(const sPoolString name)
{
    if(Token==sTOK_Name && Name == name)
    {
        Scan();
        return 1;
    }
    else
    {
        return 0;
    }
}
bool sScanner::IfString(const sPoolString name)
{
    if(Token==sTOK_String && String == name)
    {
        Scan();
        return 1;
    }
    else
    {
        return 0;
    }
}


/**************************************************************************+*/
/**************************************************************************+*/
}

/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "base/types.hpp"
#include "scanner.hpp"
#include "base/system.hpp"

/****************************************************************************/

sScannerSource::sScannerSource()
{
  ScanPtr = 0;
  Line = 1;
  Filename = 0;
  BeginOfFile = 1;
}

/****************************************************************************/

sScannerSourceText::sScannerSourceText(const sChar *buffer,sBool deleteme)
{
  Start = buffer;
  ScanPtr = Start;
  DeleteMe = deleteme;
}

sScannerSourceText::~sScannerSourceText()
{
  if(DeleteMe)
    delete[] Start;
}

/****************************************************************************/

sScannerSourceFile::sScannerSourceFile()
{
  File = 0;
  ScanBuffer = 0;
  UnicodeConversion = 0;
}

sScannerSourceFile::~sScannerSourceFile()
{
  delete File;
  delete ScanBuffer;
}

sBool sScannerSourceFile::Load(const sChar *filename)
{
  File = sCreateFile(filename,sFA_READ);
  if(!File)
    return 0;

  ScanBuffer = new sChar[sScanner::BufferSize+1];
  ScanBuffer[sScanner::BufferSize] = 0;

  // handles big and little endian

  union
  {
    sChar header;
    sChar8 headerbytes[2];
  } u;

  if(!File->Read(&u.header,2))
    return 0;

  CharsLeftInFile = File->GetSize();
  switch(u.header)
  {
  case 0xfeff:      // file endian == cpu endian
    CharsLeftInFile = CharsLeftInFile/2-1;
    UnicodeConversion = 0;
    ScanPtr = ScanBuffer + sScanner::BufferSize;
    break;

  case 0xfffe:      // file endian != cpu endian
    CharsLeftInFile = CharsLeftInFile/2-1;
    UnicodeConversion = 2;
    ScanPtr = ScanBuffer + sScanner::BufferSize;
    break;

  default:          // 8bit -> 16 bit
    UnicodeConversion = 1;
    ScanPtr = ScanBuffer + sScanner::BufferSize - 2;
    ScanBuffer[sScanner::BufferSize-2] = u.headerbytes[0];
    ScanBuffer[sScanner::BufferSize-1] = u.headerbytes[1];
    CharsLeftInFile -= 2;
    break;
  }
  
  Filename = filename;
  return 1;
}

sBool sScannerSourceFile::Refill()
{
  sInt left = ScanBuffer+sScanner::BufferSize-ScanPtr;
  if(left<sScanner::LineSize)
  {
    //  calculate copy

    sInt pos = ScanPtr-ScanBuffer;
    pos = pos&~(sScanner::Alignment-1);
    sInt count = sScanner::BufferSize-pos;

    // perform copy

    for(sInt i=0;i<count;i++)
      ScanBuffer[i] = ScanBuffer[i+pos];
    ScanPtr -= pos;

    // calculate load

    sInt loadchars = pos;
    sChar *dest = ScanBuffer+count;
    if(loadchars>CharsLeftInFile)
      loadchars = CharsLeftInFile;

    // perform load

    sBool ok=0;
    if(loadchars>0)
    {
      switch(UnicodeConversion)
      {
      case 0:
        ok = File->Read(dest,loadchars*sizeof(sChar));
        dest+=loadchars;
        break;
      case 1:   // 8bit -> 16bit (or whatever a widechar on this system is)
        {
          sChar8 *ascii = (sChar8 *) dest;
          ascii+=loadchars * (sizeof(sChar)-1);
          ok = File->Read(ascii,loadchars);
          for(sInt i=0;i<loadchars;i++)
            *dest++ = (*ascii++)&0xff;
        }
        break;
      case 2:   // swap endian
        ok = File->Read(dest,loadchars*sizeof(sChar));
        for(sInt i=0;i<loadchars;i++)
          dest[i] = ((dest[i]>>8) | (dest[i]<<8))&0xffff;
        dest += loadchars;
        break;
      }

      CharsLeftInFile -= loadchars;
    }
    *dest = 0;
    if(!ok)
    {
      ScanBuffer[count] = 0;
      return 0;
    }
  }
  return 1;
}

/****************************************************************************/
/****************************************************************************/

sScanner::sScanner()
{
  Stream = 0;
  ErrorCallback = 0;
  ErrorCallbackPtr = 0;
  Init();
}

sScanner::~sScanner()
{
  delete Stream;
}

void sScanner::Init()
{
  Stop();
  Flags = 0;
  Flags = sSF_CPPCOMMENT;
  TokensSym.Clear();
  TokensName.Clear();
};

void sScanner::Stop()
{
  Token = 0;
  ValI = 0;
  ValF = 0;
  Name = L"";
  String = L"";
  ValueString = L"";
  ValueSuffix = L"";
  Errors = 0;
  sDeleteAll(Includes);
  sDelete(Stream);
}

void sScanner::SortTokens()
{
  sSortDown(TokensSym,&ScannerToken::Length);
}

void sScanner::Start(const sChar *text)
{
  Stop();
  Stream = new sScannerSourceText(text,0);

  SortTokens();
  Scan();
}


sBool sScanner::StartFile(const sChar *filename, sBool reporterror)
{
  Stop();

  sScannerSourceFile *s = new sScannerSourceFile();
  if(s->Load(filename))
  {
    Stream = s;
    SortTokens();
    Scan();
    return 1;
  }
  else
  {
    delete s;
    Stream = new sScannerSourceText(L"");
    if(reporterror)
      Error(L"could not load file %q",filename);
    return 0;
  }
}

sBool sScanner::IncludeFile(const sChar *filename)
{
  sScannerSourceFile *s = new sScannerSourceFile();
  if(s->Load(sPoolString(filename)))
  {
    Stream->ScanPtr = TokenScan;
    Includes.AddTail(Stream);
    Stream = s;
    Scan();
    return 1;
  }
  else
  {
    delete s;
    return 0;
  }
}

void sScanner::AddToken(const sChar *name,sInt token)
{
  ScannerToken tok;
  tok.Name = name;
  tok.Token = token;
  tok.Length = sGetStringLen(name);

  sBool IsName = 1;
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

void sScanner::RemoveToken(const sChar *name)
{
  ScannerToken *tok;
  sFORALL(TokensName, tok)
  {
    if (sCmpString(name, tok->Name) == 0)
    {
      TokensName.Rem(*tok);
      return;
    }
  }
  sFORALL(TokensSym, tok)
  {
    if (sCmpString(name, tok->Name) == 0)
    {
      TokensSym.Rem(*tok);
      return;
    }
  }
}

void sScanner::AddTokens(const sChar *SingleCharacterTokens)
{
  sChar buffer[2];
  for(sInt i=0;SingleCharacterTokens[i];i++)
  {
    buffer[0] = SingleCharacterTokens[i];
    buffer[1] = 0;
    AddToken(buffer,SingleCharacterTokens[i]);
  }
}

void sScanner::DefaultTokens()
{
  AddToken(L">>",sTOK_SHIFTR);
  AddToken(L"<<",sTOK_SHIFTL);
  AddToken(L"==",sTOK_EQ);
  AddToken(L"!=",sTOK_NE);
  AddToken(L"<=",sTOK_LE);
  AddToken(L">=",sTOK_GE);
  AddToken(L"..",sTOK_ELLIPSES);
  AddToken(L"::",sTOK_SCOPE);

  AddTokens(L"+-*/%^~,.;:!?=<>()[]{}&|");
}

sInt sScanner::Error0(const sChar *name)
{
  if(ErrorCallback)
  {
    (*ErrorCallback)(ErrorCallbackPtr,Stream->Filename,Stream->Line,name);
  }
  else if(Errors==0)
  {
    if(Stream)
    {
      if(Stream->Filename)
        ErrorMsg.PrintF(L"%s(%d): %s\n",Stream->Filename,Stream->Line,name);
      else
        ErrorMsg.PrintF(L"line %d: %s\n",Stream->Line,name);
    }
    else
    {
      ErrorMsg.PrintF(L"%s\n",name);
    }
    if(!(Flags & sSF_QUIET))
    {
      if(sCONFIG_OPTION_SHELL)
        sPrintErrorF(ErrorMsg);
      else
        sDPrintF(ErrorMsg);
    }
  }
  Errors++;

  return 0;
}

sInt sScanner::Warn0(const sChar *name)
{
  if(Stream->Filename)
    ErrorMsg.PrintF(L"%s(%d): %s\n",Stream->Filename,Stream->Line,name);
  else
    ErrorMsg.PrintF(L"line %d: %s\n",Stream->Line,name);
  if(!(Flags & sSF_QUIET))
  {
    if(sCONFIG_OPTION_SHELL)
      sPrintWarningF(ErrorMsg);
    else
      sDPrintF(ErrorMsg);
  }
  return 0;
}

/****************************************************************************/


sChar sScanner::DirtyPeek()
{
  Stream->Refill();
  const sChar *s = Stream->ScanPtr;

morespace:
  // skip whitespace
  while(*s==' ' || *s=='\n' || *s=='\r' || *s=='\t')
    s++;

  // check end of file
  if(*s==0)
    return 0;

  // check comments
  if(Flags & sSF_CPPCOMMENT)
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
  if( ((Flags & sSF_SEMICOMMENT) && *s==';') || ((Flags & sSF_NUMBERCOMMENT) && *s=='#') )
  {
    s++;
    while(*s!=0 && *s!='\n' && *s!='\r')
      s++;
    goto morespace;
  }

  return *s;
}

sInt sScanner::Scan()
{
  static const sInt MaxValLen = sCOUNTOF(ValueString);

  sInt newline;
  ScannerToken *st;
  sString<256> &bp = ValueString;
  sInt bi=0;

  Token = sTOK_ERROR;
  ValI = 0;
  ValF = 0;
  newline = 0;

morespace:

  Stream->Refill();

  // save position for token (before whitespace, for sTOK_END)

  TokenLine = Stream->Line;
  TokenFile = Stream->Filename;
  TokenScan = Stream->ScanPtr;

  // skip whitespace
  const sChar * sRESTRICT s = Stream->ScanPtr;

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
      Token = sTOK_END;
      goto ende;
    }    
  }

  // check comments
  if(Flags & sSF_CPPCOMMENT)
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
        Error(L"eol reached in /* */ comment");
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

  if((Flags & sSF_CPP) && (newline|Stream->BeginOfFile) && *s=='#')
  {
    if(sCmpStringLen(s,L"#line ",6)==0)
    {
      s += 6;
      while(*s==' ') s++;
      sInt line;

      if(sScanInt((const sChar *&) s,line))
      {
        Stream->Line = line;
        while(*s==' ') s++;
        if(*s=='"')
        {
          s++;
          sChar buffer[sMAXPATH];
          sInt i=0;
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
      Error(L"error in #line directive");
      goto ende;
    }
    if(sCmpStringLen(s,L"#error ",7)==0)
    {
      Error(L"#error directive");
      goto ende;
    }    
  }

  // full line comments.

  if( ((Flags & sSF_SEMICOMMENT) && *s==';') || ((Flags & sSF_NUMBERCOMMENT) && *s=='#') )
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
  if(newline && (Flags & sSF_NEWLINE))
  {
    Token = sTOK_NEWLINE;
    goto ende;
  }

  // token?

  sFORALL(TokensSym,st)           // this is a performance hotspot! make it table driven!
  {
    if(sCmpMem(s,st->Name,st->Length*sizeof(sChar))==0)
    {
      Token = st->Token;
      s += st->Length;
      goto ende;
    }
  }

  // strings

  if(*s=='"')
  {
    s++;
    sInt i=0;
    while(*s!='"' && *s)
    {
      if(*s=='\n')
        Stream->Line++;
      sInt c = *s;
      if(c=='\\' && (Flags&sSF_ESCAPECODES))
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
              sChar b[5];
              b[0] = s[1];
              b[1] = s[2];
              b[2] = s[3];
              b[3] = s[4];
              b[4] = 0;
              const sChar *bb = b;
              sScanHex(bb,c);
              s+=4;
            }
            else if((s[0]=='x'||s[0]=='u') && sIsHex(s[1]) && sIsHex(s[2]))
            {
              sChar b[5];
              b[0] = s[1];
              b[1] = s[2];
              b[2] = 0;
              const sChar *bb = b;
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
      Token=sTOK_STRING;
      s++;
    }
    goto ende;
  }

  // char?

  if(*s=='\'')
  {
    s++;
    if(*s=='\\' && (Flags&sSF_ESCAPECODES))
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
    Token = sTOK_CHAR;
    goto ende;
  }

  // names

  if((*s>='a' && *s<='z') || 
     (*s>='A' && *s<='Z') || 
     (*s>=0xa0 && *s<=0xff && (Flags&sSF_UMLAUTS)) ||
     *s=='_')
  {
    sInt i = 0;
    while((*s>='a' && *s<='z') || 
          (*s>='A' && *s<='Z') ||
          (*s>='0' && *s<='9') || 
          (*s>=0xa0 && *s<=0xff && (Flags&sSF_UMLAUTS)) ||
          *s=='_')
    {
      TempString[i++]=*s++;
    }
    Name.Init(TempString,i);
    Token = sTOK_NAME;

    sFORALL(TokensName,st)
    {
      if(sCmpString(Name,st->Name)==0)
      {
        Token = st->Token;
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

    Token = sTOK_INT;
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

    Token = sTOK_INT;
    goto ende;
  }

  // numbers
  if(*s>='0' && *s<='9')
  {
    Token = sTOK_INT;
    while(*s>='0' && *s<='9')
    {
      ValI = ValI*10+(*s-'0');
      ValF = ValF*10+(*s-'0');
      if(bi<MaxValLen-2) bp[bi++]=*s;
      s++;
    }

    if(s[0]=='.' && ((s[1]>='0' && s[1]<='9') || s[1]=='#'))
    {
      Token = sTOK_FLOAT;
      if(bi<MaxValLen-2) bp[bi++]=*s;
      s++;
      sF64 dec=1.0f;
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
          ValF = sRawCast<sF32,sU32>(0x7f800000);
        }
        else
          Token = sTOK_ERROR;     // invalid infinity float
      }
      else
      {
        while(*s>='0' && *s<='9')
        {
          dec = dec/10;
          ValF += sF32(dec*(*s-'0'));
          if(bi<MaxValLen-2) bp[bi++]=*s;
          s++;
        }
      }
    }
    // float with exponent
    if (*s=='e')
    {
      Token = sTOK_FLOAT;
      if(bi<MaxValLen-2) bp[bi++]=*s;
      s++;
      sInt exp = 0;
      sInt sign = 1;
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
      ValF *= sFPow(10, exp);
    }

    sChar *suffix = bp+bi;
    if(Flags & sSF_TYPEDNUMBERS)
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

  if(Token==sTOK_ERROR)
  {
    Error(L"illegal character in input");
    s++;
  }
ende:
  Stream->ScanPtr = s;
  return Token;
}

void sScanner::Print(sTextBuffer &tb)
{
  ScannerToken *st;
  switch(Token)
  {
  case sTOK_END:
    tb.Print(L"[END] ");
    break;
  case sTOK_ERROR:
    tb.Print(L"[ERROR] ");
    break;
  case sTOK_INT:
    tb.PrintF(L"%d ",ValI);
    break;
  case sTOK_FLOAT:
    tb.PrintF(L"%f ",ValF);
    break;
  case sTOK_NAME:
    tb.PrintF(L"%s ",Name);
    break;
  case sTOK_STRING:
    tb.PrintF(L"\"%s\" ",String);
    break;
  case sTOK_CHAR:
    tb.PrintF(L"%d ",ValI);
    break;
  default:
    st = sFind(TokensSym,&ScannerToken::Token,Token);
    if(!st)
      st = sFind(TokensName,&ScannerToken::Token,Token);
    tb.PrintF(L"%s ",st ? (const sChar*)st->Name : L"???");
    break;
  }
}

/****************************************************************************/

sInt sScanner::ScanInt()
{
  sInt r;
  sInt sign = 1;
  if(IfToken('-'))
    sign = -1;
  if(Token==sTOK_INT || Token==sTOK_CHAR)
  {
    r = ValI;
    Scan();
  }
  else
  {
    r = 0;
    Error(L"integer expected");
  }
  return r*sign;
}

sF32 sScanner::ScanFloat()
{
  sF32 r;
  sF32 sign = 1;
  if(IfToken('-')) 
    sign = -1;
  if(Token==sTOK_INT)   
  {
    r = ValI;
    Scan();
  }
  else if(Token==sTOK_FLOAT)
  {
    r = ValF;
    Scan();
  }
  else
  {
    r = 0;
    Error(L"float expected");
  }
  return r*sign;
}

sInt sScanner::ScanChar()
{
  sInt r;
  if(Token==sTOK_CHAR)
  {
    r = ValI;
  }
  else
  {
    r = 0;
    Error(L"character literal expected");
  }
  Scan();
  return r;
}

sBool sScanner::ScanRaw(sPoolString &ps, sInt opentoken, sInt closetoken)
{
  sTextBuffer tb;
  sBool result = ScanRaw(tb, opentoken, closetoken);
  ps = tb.Get();
  return result;
}

sBool sScanner::ScanRaw(sTextBuffer &tb, sInt opentoken, sInt closetoken)
{
  sInt count;
  sInt startLine = Stream->Line;

  if(Token!=opentoken)
  {
    Error(L"scan raw: open token missing");
    return sFALSE;
  }

  count = 1;
  sChar c;
  const sChar * sRESTRICT s = Stream->ScanPtr;

  while((c=*s)!=0)
  { 
    if (c==sChar(closetoken)) 
    {
      count--;
      if (count==0) break;
    }
    else if (c==sChar(opentoken)) 
    {
      count++;
    }
    else if (c=='\n') 
    {
      Stream->Line++;
    }
    
    // skip comments

    if (Flags & sSF_CPPCOMMENT)
    {
      sChar next =s[1];
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
          Error(L"eol reached in /* */ comment");
          return Token;
        }
        else
        {
          tb.PrintChar(c); s++; c = *s;
          tb.PrintChar(c); s++; 
        }
      }
      if (c=='/' && next=='/')
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
      sChar last = c;
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
      sChar last = c;
      tb.PrintChar(c); s++; 
      while ((c=*s) != 0 && (c != '\'' || last == '\\'))
      {
        if (c == '\n') Stream->Line++;
        last = c;
        tb.PrintChar(c); s++;
      }
    }

    // end of string skipping

    tb.PrintChar(*s);
    s++;
  }

  Stream->ScanPtr = s;

  if (count != 0)
  {
    sString<64> msg; sSPrintF(msg, L"scan raw: %d %s token(s) missing, starting at line %d", count, (count > 0) ? L"close" : L"open", startLine);
    Error(msg);
  }

  Match(opentoken);
  Match(closetoken);

  return sTRUE;
}

sBool sScanner::ScanLine (sTextBuffer &tb)
{
  sChar c;
  while((c=*(Stream->ScanPtr))!=0)
  { 
    if (c=='\n')
    {
      Scan();
      return sTRUE;
    }
    tb.PrintChar(c);
    Stream->ScanPtr++;
  }

  sString<64> msg; sSPrintF(msg, L"scan line: premature end of file reached");
  Error(msg);

  return sFALSE;
}

sBool sScanner::ScanName(sPoolString &ps)
{
  sBool r;
  if(Token==sTOK_NAME)
  {
    ps = Name;
    r = 1;
  }
  else
  {
    ps = L"";
    Error(L"name expected");
    r = 0;
  }
  Scan();
  return r;
}

sBool sScanner::ScanString(sPoolString &ps)
{
  sBool r;
  if(Token==sTOK_STRING)
  {
    ps = String;
    r = 1;
    Scan();

    while(Token==sTOK_STRING && (Flags & sSF_MERGESTRINGS))
    {
      sPoolString b = String;
      sPoolString a = ps;
      Scan();
      ps.Add(a,b);
    }
  }
  else
  {
    ps = L"";
    Error(L"string expected");
    r = 0;
    Scan();
  }
  return r;
}

sBool sScanner::ScanNameOrString(sPoolString &ps)
{
  sBool r;
  if(Token==sTOK_NAME)
  {
    ps = Name;
    r = 1;
  }
  else if(Token==sTOK_STRING)
  {
    ps = String;
    r = 1;
  }
  else
  {
    ps = L"";
    Error(L"name expected");
    r = 0;
  }
  Scan();
  return r;
}


void sScanner::Match(sInt token)
{
  if(token==Token) 
    Scan();
  else
  {
    ScannerToken *t;
    sFORALL(TokensSym,t)
    {
      if(t->Token==token)
      {
        Error(L"syntax error: `%s' expected",t->Name);
        return;
      }
    }
    sFORALL(TokensName,t)
    {
      if(t->Token==token)
      {
        Error(L"syntax error: `%s' expected",t->Name);
        return;
      }
    }
    Error(L"syntax error");
  }
   
  // this is broken: user tokens start with 32 and doesn't need to equal their printable ascii character
  // so just do the slow search over all tokens for a correct error message
  //if(token>=32 && token<256) // printable ascii character
  //  Error(L"syntax error: `%c' expected",token);
  //else
  //  Error(L"syntax error");
}

void sScanner::MatchName(const sChar *name)
{
  if(!IfName(name))
    Error(L"%s expected",name);
}

void sScanner::MatchString(const sChar *name)
{
  if(!IfString(name))
    Error(L"string %q expected",name);
}

void sScanner::MatchInt(sInt n)
{
  if(Token==sTOK_INT && ValI==n)
    Scan();
  else
    Error(L"integer %d expected",n);
}

/****************************************************************************/

sBool sScanner::IfToken(sInt tok)
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

sBool sScanner::IfName(const sPoolString name)
{
  if(Token==sTOK_NAME && Name == name)
  {
    Scan();
    return 1;
  }
  else
  {
    return 0;
  }
}
sBool sScanner::IfString(const sPoolString name)
{
  if(Token==sTOK_STRING && String == name)
  {
    Scan();
    return 1;
  }
  else
  {
    return 0;
  }
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Scanner utils                                                      ***/
/***                                                                      ***/
/**************************************************************************+*/
/****************************************************************************/

namespace sScannerUtil
{
  void Unexpected(sScanner *scan)
  {
    switch(scan->Token)
    {
    case sTOK_NAME:   scan->Error(L"unexpected identifier \"%s\"",scan->Name); break;
    case sTOK_STRING: scan->Error(L"unexpected string\n"); break;
    default:          scan->Error(L"unexpected token\n"); break;
    }
  }

  sBool StringProp(sScanner *scan,const sChar *name,sPoolString &tgt)
  {
    if(scan->IfName(name))
    {
      scan->Match('=');
      scan->ScanString(tgt);
      scan->Match(';');

      return sTRUE;
    }

    return sFALSE;
  }

  sBool StringProp(sScanner *scan,const sChar *name,const sStringDesc &tgt)
  {
    sPoolString str;
    if(StringProp(scan,name,str))
    {
      sCopyString(tgt,str);
      return sTRUE;
    }
    else
      return sFALSE;
  }

  sBool IntProp(sScanner *scan,const sChar *name,sInt &tgt)
  {
    if(scan->IfName(name))
    {
      scan->Match('=');
      tgt = scan->ScanInt();
      scan->Match(';');

      return sTRUE;
    }
    
    return sFALSE;
  }

  sBool FloatProp(sScanner *scan,const sChar *name,sF32 &tgt)
  {
    if(scan->IfName(name))
    {
      scan->Match('=');
      tgt = scan->ScanFloat();
      scan->Match(';');

      return sTRUE;
    }
    
    return sFALSE;
  }

  sBool GUIDProp(sScanner *scan,const sChar *name,sGUID &tgt)
  {
    sPoolString str;
    if(StringProp(scan,name,str))
    {
      const sChar *p = str;
      if(!sScanGUID(p,tgt) || *p != 0)
        scan->Error(L"malformed GUID");
      return sTRUE;
    }
    else
      return sFALSE;
  }

  sBool OptionalBoolProp(sScanner *scan,const sChar *name,sBool &tgt)
  {
    if(scan->IfName(name))
    {
      scan->Match(';');
      tgt = sTRUE;
      return sTRUE;
    }

    return sFALSE;
  }

  sBool EnumProp(sScanner *scan,const sChar *name,const sChar *choices,sInt &tgt)
  {
    if(scan->IfName(name))
    {
      scan->Match('=');

      sPoolString value;
      if(scan->ScanName(value))
      {
        tgt = sFindChoice(value,choices);
        if(tgt == -1)
          scan->Error(L"%q is not a member of this enumeration",value);
      }

      scan->Match(';');
      return sTRUE;
    }

    return sFALSE;
  }

  sBool FlagsProp(sScanner *scan,const sChar *name,const sChar *pattern,sInt &tgt)
  {
    if(scan->IfName(name))
    {
      scan->Match('=');

      tgt = 0;
      while(scan->Errors==0 && scan->Token!=';')
      {
        sPoolString flag;
        if(scan->ScanName(flag))
        {
          sInt mask,value;
          if(sFindFlag(flag,pattern,mask,value))
            tgt = (tgt & ~mask) | value;
          else
            scan->Error(L"%q is not a valid flag",flag);
        }

        if(scan->Token!=';')
          scan->Match('|');
      }

      scan->Match(';');
      return sTRUE;
    }

    return sFALSE;
  }
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Regular Expression Matching                                        ***/
/***                                                                      ***/
/**************************************************************************+*/
/****************************************************************************/

#define sREGEX_MAXGROUP 10

struct sRegexState                 // state in regular expression
{
  sRegexState();
  sInt Temp;
  sArray<sRegexTrans *> Output;   // outgoing transitions
  sArray<sRegexTrans *> Input;    // incomming transitions
};

struct sRegexTrans                // possible transition between states
{
  sRegexTrans();
  ~sRegexTrans();
  
  sRegexState *To;
  sRegexState *From;
  sU32 BitGroup[8];               // 0x0001..0x00ff
  const sChar *HighGroup;         // 0x0100..0xffff
  sBool Invert;                   // invert the groups
  sBool Empty;                    // transition taken without consuming
  sU32 StartCaptureBits;          // start capturing for group
  sU32 StopCaptureBits;            // end capturing for group

  void SetBit(sChar);
  void SetChar(sChar);            // 'a'
  void SetGroup(const sChar *);   // "[a-z]"
  sBool Hit(sChar c);
};

struct sRegexMarker               // a marker on the state. each character
{
  sRegexMarker();
  void CopyFrom(const sRegexMarker *src);
  void Goto(sRegexTrans *t,const sChar *str);

  sRegexState *State;
  const sChar *CaptureStart[sREGEX_MAXGROUP];
  const sChar *CaptureEnd[sREGEX_MAXGROUP];
};

/****************************************************************************/


/****************************************************************************/


sRegexState::sRegexState()
{
}

sRegexTrans::sRegexTrans()
{
  From = 0;
  To = 0;
  sClear(BitGroup);
  HighGroup = 0;
  Invert = 0;
  Empty = 0;
  StartCaptureBits = 0;
  StopCaptureBits = 0;
}

sRegexTrans::~sRegexTrans()
{
  delete[] HighGroup;
}

void sRegexTrans::SetBit(sChar c)
{
  BitGroup[(c>>5)&7] |= 1<<(c&31);
}

void sRegexTrans::SetChar(sChar c)
{
  if(c>=0 && c<=0xff)
  {
    SetBit(c);
  }

  else
  {
    sChar *h = new sChar[2];
    HighGroup = h;
    h[0] = c;
    h[1] = 0;
  }
}

void sRegexTrans::SetGroup(const sChar *c)
{
  sVERIFY(*c=='[');
  c++;
  if(*c=='^')
  {
    Invert = 1;
    c++;
  }
  sInt h0=0,h1=0;
  const sChar *e = c;
  sChar *h = 0;
  while(*e!=0 && *e!=']')
  {
    if(!(*e>=0 && *e<=0xff))
      h0++;
    e++;
  }
  if(h0>0)
  {
    h = new sChar[h0+1];
    HighGroup = h;
  }

  while(c<e)
  {
    if(*c>=0 && *c<=0xff)
    {
      if(c[0]=='\\' && c[1]!=0)
      {
        c++;
        switch(*c)
        {
        case 'n':
          SetBit(10);
          break;
        case 'r':
          SetBit(13);
          break;
        case 't':
          SetBit('\t');
          break;
        case '\\':
          SetBit('\\');
          break;
        case '-':
          SetBit('-');
          break;
        case 's':
          SetBit(10);
          SetBit(13);
          SetBit(' ');
          SetBit('\t');
          break;
        case 'd':
          for(sInt i='0';i<='9';i++)
            SetBit(i);
          break;
        case 'w':
          for(sInt i='a';i<='z';i++)
            SetBit(i);
          for(sInt i='A';i<='Z';i++)
            SetBit(i);
          break;
        }
      }
      else if(c[1]=='-' && c[2]!=0 && c[2]!=']' && c[2]!='\\' && c[0]!='\\' &&
        c[0]>=32 && c[0]<=0xff && c[2]>=32 && c[2]<=0xff )
      {
        sInt c0 = c[0];
        sInt c1 = c[2];
        c+=2;
        for(sInt i=c0;i<=c1;i++)
          SetBit(i);
      }
      else
      {
        SetBit(*c);
      }
    }
    else
    {
      sBool found = 0;
      sVERIFY(h);
      for(sInt i=0;i<h1 && !found;i++)
        if(h[i]==*c)
          found = 1;
      if(!found)
        h[h1++] = *c;
    }
    c++;
  }
  sVERIFY(h1<=h0);
  if(h)
    h[h1++] = 0;
}

sBool sRegexTrans::Hit(sChar c)
{
  if(c>=0 && c<=0xff)
  {
    return (BitGroup[c>>5] & (1<<(c&31))) ? !Invert : Invert;
  }
  else
  {
    const sChar *h = HighGroup;
    while(*h)
      if(*h++==c)
        return !Invert;
    return Invert;
  }
}

sRegexMarker::sRegexMarker()
{
  State = 0;
  sClear(CaptureStart);
  sClear(CaptureEnd);
}

void sRegexMarker::CopyFrom(const sRegexMarker *src)
{
  State = src->State;
  for(sInt i=0;i<sCOUNTOF(CaptureStart);i++)
  {
    CaptureStart[i] = src->CaptureStart[i];
    CaptureEnd[i] = src->CaptureEnd[i];
  }
}

void sRegexMarker::Goto(sRegexTrans *t,const sChar *str)
{
  State = t->To;
  if(t->StartCaptureBits)
  {
    for(sInt i=0;i<sREGEX_MAXGROUP;i++)
    {
      if((t->StartCaptureBits & (1<<i)) && CaptureStart[i]==0)
        CaptureStart[i] = str;
    }
  }
  if(t->StopCaptureBits)
  {
    for(sInt i=0;i<sREGEX_MAXGROUP;i++)
    {
      if((t->StopCaptureBits & (1<<i)) && CaptureEnd[i]==0)
        CaptureEnd[i] = str;
    }
  }
}

/****************************************************************************/

void sRegex::Flush()
{
  sDeleteAll(Machine);
  sDeleteAll(Markers);
  sDeleteAll(OldMarkers);
  sDeleteAll(Trans);
  Start = 0;
  End = 0;
  Valid = 0;
  Scan = 0;
  GroupId = 0;
}

sRegexState *sRegex::MakeState()
{
  sRegexState *s = new sRegexState();
  Machine.AddTail(s);
  return s;
}

sRegexMarker *sRegex::MakeMarker()
{
  if(OldMarkers.GetCount()>0)
    return OldMarkers.RemTail();
  return new sRegexMarker();
}

sRegexTrans *sRegex::MakeTrans()
{
  sRegexTrans *t = new sRegexTrans;
  Trans.AddTail(t);
  return t;
}

sRegexTrans *sRegex::MakeTrans(sRegexState *from,sRegexState *to)
{
  sRegexTrans *t = MakeTrans();
  t->From = from;
  t->To = to;
  to->Input.AddTail(t);
  from->Output.AddTail(t);
  return t;
}

sRegexTrans *sRegex::MakeTransCopy(sRegexTrans *src,sRegexState *from,sRegexState *to)
{
  sRegexTrans *t = MakeTrans(from,to);

  for(sInt i=0;i<sCOUNTOF(t->BitGroup);i++)
    t->BitGroup[i] = src->BitGroup[i];
  t->Invert = src->Invert;
  t->Empty = src->Empty;
  t->StartCaptureBits = src->StartCaptureBits;
  t->StopCaptureBits = src->StopCaptureBits;

  if(src->HighGroup)
  {
    sInt len = sGetStringLen(src->HighGroup);
    sChar *d = new sChar[len+1];
    t->HighGroup = d;
    sCopyString(d,src->HighGroup,len+1);
  }

  return t;
}

/****************************************************************************/

sRegex::sRegex()
{
}

sRegex::~sRegex()
{
  Flush();
}

sRegexState *sRegex::_Value(sRegexState *o)
{
  sRegexState *n;
  sRegexTrans *t;
  sInt id,mask;
  const sChar *e0 = 0;

  n=0;
  switch(*Scan)
  {
  case '[':
    e0 = Scan;
    while(*Scan!=']' && *Scan!=0)
      Scan++;
    if(*Scan==']')
      Scan++;
    else
      return 0;

    n = MakeState();
    t = MakeTrans(o,n);
    t->SetGroup(e0);
    break;

  case '.':
    Scan++;
    n = MakeState();
    t = MakeTrans(o,n);
    t->Invert = 1;
    break;

  case '(':
    Scan++;
    id = GroupId++;
    mask = 0;
    if(id<sREGEX_MAXGROUP)
      mask = 1<<id;

    n = MakeState();
    t = MakeTrans(o,n);
    t->Empty = 1;
    t->StartCaptureBits = mask;
    o = n;

    n = _Expr(o);
    if(n==0)
      return 0;
    if(*Scan!=')')
      return 0;
    o = n;

    n = MakeState();
    t = MakeTrans(o,n);
    t->Empty = 1;
    t->StopCaptureBits = mask;
    o = n;

    Scan++;
    break;

  case '*':
  case '+':
  case '?':
  case ')':
  case '{':
  case '|':
  case '$':
    n = 0;
    break;

  case '\\':
    Scan++;
    n = MakeState();
    t = MakeTrans(o,n);
    switch(*Scan)
    {
    case '.':
    case '(':
    case '[':
    case '*':
    case '+':
    case '?':
    case ')':
    case '{':
    case '|':
    case '$':
    case '\\':
      t->SetBit(*Scan);
      break;

    case 'n':
      t->SetBit(10);
      break;
    case 'r':
      t->SetBit(13);
      break;
    case 't':
      t->SetBit('\t');
      break;

    case 'W':
      t->Invert = 1;
    case 'w':
      for(sInt i='a';i<='z';i++)
        t->SetBit(i);
      for(sInt i='A';i<='Z';i++)
        t->SetBit(i);
      break;

    case 'S':
      t->Invert = 1;
    case 's':
      t->SetBit(10);
      t->SetBit(13);
      t->SetBit(' ');
      t->SetBit('\t');
      break;

    case 'D':
      t->Invert = 1;
    case 'd':
      for(sInt i='0';i<='9';i++)
        t->SetBit(i);
      break;
      //return 0;//warning 112: statement is unreachable
    }
    Scan++;
    break;

  default:
    n = MakeState();
    t = MakeTrans(o,n);
    t->SetChar(*Scan++);
    break;
  }
  
  return n;
}

sRegexState *sRegex::_Repeat(sRegexState *o)
{
  sRegexState *n = _Value(o);
  sRegexTrans *t;
  if(*Scan=='+')                  // one or more repeats
  {
    Scan++;
    t = MakeTrans(n,o);
    t->Empty = 1;
  }
  else if(*Scan=='*')             // zero or more repeats
  {                               // careful: dont make empty transisiton
    Scan++;                       // from o->n and n->o. instead: 
    sRegexState *m = MakeState(); // o->Value->n
    t = MakeTrans(n,o);           // o<--------n
    t->Empty = 1;                 

    t = MakeTrans(o,m);           // o------------> m
    t->Empty = 1;

    n = m;                        // and m is the target!
  }
  else if(*Scan=='?')             // zero or one "repeat"
  {
    Scan++;
    t = MakeTrans(o,n);
    t->Empty = 1;
  }
  return n;
}

sRegexState *sRegex::_Cat(sRegexState *o)
{
  do
  {
    o = _Repeat(o);
  }
  while(*Scan!='|' && *Scan!='$' && *Scan!='+' && *Scan!='*' && *Scan!='{' && *Scan!=')' && *Scan!=0);
  return o;
}

sRegexState *sRegex::_Expr(sRegexState *o)
{
  sRegexTrans *t;
  sRegexState *p=0L,*p0;
  while(*Scan!=0 && *Scan!=')')
  {
    p = _Cat(o);
    if(p==0)
      return 0;

    if(*Scan=='|')
    {
      p0 = p;
      p = MakeState();
      t = MakeTrans(p0,p);
      t->Empty = 1;
      
      while(*Scan=='|')
      {
        Scan++;
        p0 = _Cat(o);
        if(p0==0)
          return 0;
        t = MakeTrans(p0,p);
        t->Empty = 1;
      }
    }
  }

  return p;
}

sBool sRegex::PrepareAll(const sChar *expr)
{
  Flush();                        // initial state
  Scan = expr;
  Start = MakeState();
  OriginalPattern = expr;

  sRegexState *s0 = MakeState();  // have one virtual step at the beginning
  sRegexTrans *t = MakeTrans(Start,s0);
  t->SetChar(0);

  End = _Expr(s0);                // expression parser

  if(*Scan!=0 || End==0)          // check correct termination
  {
    Scan = 0;
    return 0;
  }

  Scan = 0;                       // confirm successful parsing
  Valid = 1;

//  Validate();                     
  RemoveEmptyTrans();             // get rid of empty nodes
//  Validate();
  RemoveEmptyState();             // optimize away deadends in graph.
//  Validate();

  return 1;
}

sBool sRegex::SetPattern(const sChar *expr)
{
  const sChar *s = expr;
  sInt len = sGetStringLen(s);
  sChar *buffer = new sChar[len+5];
  sChar *d = buffer;

  if(s[0]=='^')
  {
    s++;
    len--;
  }
  else
  {
    *d++ = '.';
    *d++ = '*';
  }
  if(len>0 && s[len-1]=='$')
  {
    len--;
    *d++ = '(';
    while(len-->0)
      *d++ = *s++;
    *d++ = ')';
  }
  else
  {
    *d++ = '(';
    while(len-->0)
      *d++ = *s++;
    *d++ = ')';
    *d++ = '.';
    *d++ = '*';
  }
  *d++ = 0;

  sBool ok = PrepareAll(buffer);

  delete[] buffer;
  OriginalPattern = expr;
  return ok;
}

void sRegex::Validate()           // check if everythins is sane
{
  sRegexState *s;
  sRegexTrans *t;

  sFORALL(Machine,s)
  {
    sFORALL(s->Input,t)
      if(t->To!=s) 
        sFatal(L"sRegex::Validate() failed (1)");
    sFORALL(s->Output,t)
      if(t->From!=s) 
        sFatal(L"sRegex::Validate() failed (2)");
  }

  sFORALL(Trans,t)
  {
    if(sFindPtr(t->To->Input,t)==0) 
      sFatal(L"sRegex::Validate() failed (3)");
    if(sFindPtr(t->From->Output,t)==0)
      sFatal(L"sRegex::Validate() failed (4)");
  }
  return;
}

void sRegex::RemoveEmptyTrans()
{
  sRegexTrans *t,*t0,*t1;
  for(sInt i=0;i<Trans.GetCount();)
  {
    t = Trans[i];
    if(t->Empty)
    {
      Trans.RemAt(i);
      t->From->Output.Rem(t);
      t->To->Input.Rem(t);

      sFORALL(t->From->Input,t0)
      {
        t1 = MakeTransCopy(t0,t0->From,t->To);
        t1->StartCaptureBits |= t->StartCaptureBits;
        t1->StopCaptureBits  |= t->StopCaptureBits;
      }

      delete t;
    }
    else
    {
      i++;
    }
  }
}

void MarkR(sRegexState *s)
{
  sRegexTrans *t;
  s->Temp=1;
  sFORALL(s->Input,t)
    if(t->From->Temp==0)
      MarkR(t->From);
}

void sRegex::RemoveEmptyState()
{
  sRegexState *s;
  sRegexTrans *t;

  sFORALL(Machine,s)              // mark all states that can reach the end
    s->Temp = 0;
  MarkR(End);

  for(sInt i=0;i<Trans.GetCount();) // remove transitions to unmarked states
  {
    t = Trans[i];
    if(t->To->Temp==0)
    {
      t->To->Input.Rem(t);
      t->From->Output.Rem(t);
      Trans.RemAt(i);
      delete t;
    }
    else
    {
      i++;
    }
  }

  sFORALL(Machine,s)              // make sure empty states are really empty
  {
    if(s->Temp==0)
    {
      if(s->Input.GetCount()>0) sFatal(L"sRegex::RemoveEmptyState() state not empty (1)");
      if(s->Output.GetCount()>0) sFatal(L"sRegex::RemoveEmptyState() state not empty (1)");
    }
  }

  sDeleteFalse(Machine,&sRegexState::Temp);  // remove 
}

/****************************************************************************/

sBool sRegex::MatchPattern(const sChar *string)
{
  sVERIFY(Valid);

  sDeleteAll(Markers);
  sRegexTrans *t;
  sRegexMarker *m = MakeMarker();
  sRegexMarker *m1;
  m->State = Start;
  Markers.AddTail(m);
  sArray<sRegexTrans *> newstates;
  sArray<sRegexMarker *> newmarkers;

  sBool first = 1;

  while(*string || first)
  {
    newmarkers.Clear();

    sChar c = 0;                // in the first step, we insert a 0
    if(first)
      first = 0;
    else
      c = *string++;

    sFORALL(Markers,m)
    {
      newstates.Clear();

      sFORALL(m->State->Output,t)
      {
        if(t->Hit(c))
          newstates.AddTail(t);
      }

      if(newstates.GetCount()==0)
      {
        OldMarkers.AddTail(m);
      }
      else
      {
        if(newstates.GetCount()>1)
        {
          for(sInt i=1;i<newstates.GetCount();i++)
          {
            m1 = MakeMarker();
            m1->CopyFrom(m);
            m1->Goto(newstates[i],string);
            newmarkers.AddTail(m1);
          }
        }
        m->Goto(newstates[0],string);
        newmarkers.AddTail(m);
      }
    }
    Markers = newmarkers;
  }

  Success.Clear();
  sFORALL(Markers,m)
  {
    if(m->State==End)
      Success.AddTail(m);
  }
  return Success.GetCount()>0;
}

sBool sRegex::GetGroup(sInt n,const sStringDesc &desc,sInt match)
{
  if(match<0 || match>=Success.GetCount() || n<0 || n>=sREGEX_MAXGROUP)
  {
    sCopyString(desc,L"");
    return 0;
  }
  else
  {
    sRegexMarker *m = Success[match];
    const sChar *c0 = m->CaptureStart[n];
    const sChar *c1 = m->CaptureEnd[n];
    if(c0==0 || c1==0 || c0==c1)
    {
      desc.Buffer[0] = 0;
      return 0;
    }
    else
    {
      sInt len = sMin((sInt)(c1-c0),desc.Size-1);
      sVERIFY(len>=0);
      sCopyMem(desc.Buffer,c0,len*sizeof(sChar));
      desc.Buffer[len] = 0;
      return 1;
    }
  }
}

void sRegex::DebugDump()
{
  sString<1024> buffer;
  sRegexTrans *t;
  sRegexState *s;
  sLogF(L"util",L"dump regex %q\n",OriginalPattern);

  sFORALL(Machine,s)
    s->Temp = _i;
  sFORALL(Trans,t)
  {
    sInt n = 0;
    for(sInt i=0;i<sCOUNTOF(t->BitGroup);i++)
    {
      if(t->BitGroup[i])
      {
        for(sInt j=0;j<32;j++)
        {
          if(t->BitGroup[i] & (1<<j))
          {
            if(i==0)
            {
              buffer[n++] = '\\';
              buffer[n++] = j/8+'0';
              buffer[n++] = j&7+'0';
            }
            else
            {
              buffer[n++] = i*32+j;
            }
          }
        }
      }
    }
    buffer[n++] = 0;
    if(t->HighGroup)
      buffer.Add(t->HighGroup);
    sLogF(L"util",L"%3d -> %3d (%02x %02x) %s[%s]%s\n",t->From->Temp,t->To->Temp,t->StartCaptureBits,t->StopCaptureBits
      ,t->Invert?L"!":L"",buffer,t->To==End?L" END":L"");
  }
}

/**************************************************************************+*/
/**************************************************************************+*/

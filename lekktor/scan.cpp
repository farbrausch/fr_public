// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "main.hpp"
#include "scan.hpp"

/****************************************************************************/
/****************************************************************************/

sInt TokenPush;
sChar ValuePush[256];
sChar *ScanPtrPush;


sInt AToken;
sChar AValue[256];
sChar *AScanPtr;

sInt Token;
sChar Value[256];
sChar *ScanPtr;

sChar TypeMem[65536];
sChar *TypeList[4096];
sChar *TypePtr;
sInt TypeCount;

/****************************************************************************/

const sChar *Keywords[] =
{
  "+=",  "-=",  "*=",  "/=",
  "%=",  "^=",  "&=",  "|=",
  ">>=", "<<=", "...", "::",
  "",    "",    "",    "",

  "++",  "--",  ">=",  "<=",
  "==",  "!=",  ">>",  "<<",
  "&&",  "||",  "->",  "",
  ",",   ".",   ";",   ":",

  "+",  "-",  "*",  "/",
  "%",  "^",  "&",  "|",
  "!",  "~",  "=",  "",
  "{",  "}",  "(",  ")",

  "[",  "]",  "<",  ">",
  "?",  "",   "",   "",
  "",   "",   "",   "",
  "",   "",   "",   "",

  "while",
  "for",
  "if",
  "else",

  "switch",
  "case",
  "return",
  "new",

  "delete[]",
  "delete",
  "break",
  "static",

  "extern",
  "auto",
  "register",
  "int",


  "float",
  "long",
  "short",
  "char",

  "double",
  "const",
  "__asm",
  "__stdcall",

  "do",
  "sizeof",
  "goto",
  "struct",

  "class",
  "union",
  "public",
  "protected",

  "private",
  "default",

  "__cdecl",
  "__fastcall",
  "WINAPI",
  
  "typedef",
  "volatile",
  "signed",
  "unsigned",
  "operator",
  "__try",
  "__except",
  "APIENTRY",
  "CALLBACK",
  "enum",
  "namespace",
  "inline",
  "__forceinline",
  "using",
  0
};

/****************************************************************************/
/****************************************************************************/

void StartScan(sChar *txt)
{
  AScanPtr = txt;
  Scan();
}

void AddType(sChar *name)
{
  sInt size;

  size = sGetStringLen(name)+1;
  TypeList[TypeCount++] = TypePtr;
  sCopyMem(TypePtr,name,size);
  TypePtr += size;
}

void LoadTypeList(sChar *txt)
{
  StartScan(txt);
  Scan();
  TypeCount = 0;
  TypePtr = TypeMem;
  while(Token == TOK_NAME)
  {
    AddType(Value);
    Scan();
  }
}

/****************************************************************************/

sBool IsLetter(sInt c)
{
  return c=='_' || (c>='a' && c<='z') || (c>='A' && c<='Z');
}

sBool IsWhitespace(sInt c)
{
  return c==' ' || c=='\n' || c=='\r' || c=='\t';
}

sInt AScan()
{
  sChar *s;
  sChar c;
  sInt i,j;

  AToken = TOK_ERROR;
  AValue[0] = 0;
  s = AValue;

// pre-init

whitespace:
  while(IsWhitespace(*AScanPtr))
    AScanPtr++;
  if(AScanPtr[0]=='/' && AScanPtr[1]=='*')
  {
    AScanPtr+=2;
    while(!(AScanPtr[0]=='*' && AScanPtr[1]=='/'))
    {
      if(AScanPtr[0]==0)
        return AToken;
      AScanPtr++;
    }
    AScanPtr+=2;
    goto whitespace;
  }
  if(AScanPtr[0]=='/' && AScanPtr[1]=='/')
  {
    AScanPtr+=2;
    while(AScanPtr[0]!='\n' && AScanPtr[0]!=0)
    {
      AScanPtr++;
    }
    goto whitespace;
  }

  if(*AScanPtr==0)
  {
    AToken = TOK_EOF;
    return AToken;
  }

// preprocessor

  if(AScanPtr[0]=='#')
  {
    while(*AScanPtr!='\n' && *AScanPtr!='\r' && *AScanPtr!=0)
    {
      *s++ = *AScanPtr++;
    }
    *s++ = 0;

    // check for lekktor pragmas
    if(!sCmpMem(AValue,"#pragma",7))
    {
      s = AValue + 7;
      while(IsWhitespace(*s))
        s++;

      if(!sCmpMem(s,"lekktor",7))
      {
        s += 7;
        while(IsWhitespace(*s))
          s++;

        if(*s++ == '(')
        {
          while(IsWhitespace(*s))
            s++;

          if(!sCmpMem(s,"on",2))
          {
            i = RealMode;
            s += 2;
          }
          else if(!sCmpMem(s,"off",3))
          {
            i = -1;
            s += 3;
          }

          while(IsWhitespace(*s))
            s++;

          if(*s == ')') // well-formed pragma, so change mode
            Mode = i;
        }
      }
    }

    AToken = TOK_PRE;
    return AToken;
//    goto whitespace;
  }

// keywords

  for(i=0;Keywords[i];i++)
  {
    if(Keywords[i][0]!=0)
    {
      for(j=0;j<Keywords[i][j];j++)
      {
        if(Keywords[i][j]!=AScanPtr[j])
          goto nextkey;
      }
      if(i<0x40 || !IsLetter(AScanPtr[j]))
      {
        AScanPtr+=j;
        AToken = i+0x80;
        sCopyString(AValue,Keywords[i],sizeof(AValue));
        return AToken;
      }
    }
    nextkey:;
  }

// names

  c = *AScanPtr;
  if(c=='_' || (c>='a' && c<='z') || (c>='A' && c <='Z'))
  {
    AToken = TOK_NAME;
    do
    {
      AScanPtr++;
      *s++ = c;
      c = *AScanPtr;
    }
    while(c=='_' || (c>='a' && c<='z') || (c>='A' && c <='Z') || (c>='0' && c<='9'));
    *s++ = 0;
    for(i=0;i<TypeCount;i++)
    {
      if(sCmpString(AValue,TypeList[i])==0)
      {
        AToken = TOK_TYPE;
        return AToken;
      }
    }
    return AToken;
  }

// values

  if(*AScanPtr=='\'')
  {
    *s++ = *AScanPtr++;
    if(*AScanPtr=='\\')
      *s++ = *AScanPtr++;
    *s++ = *AScanPtr++;
    if(*AScanPtr!='\'')
      Error("char literal");
    *s++ = *AScanPtr++;
    *s++ = 0;
    AToken=TOK_VALUE;
    return AToken;
  }

  if(AScanPtr[0]=='0' && AScanPtr[1]=='x')
  {
    *s++ = *AScanPtr++;
    *s++ = *AScanPtr++;
    c=*AScanPtr;
    while((c>='a' && c<='f') || (c>='A' && c <='F') || (c>='0' && c<='9'))
    {
      *s++ = *AScanPtr++;
      c=*AScanPtr;
    }
    *s++ = 0;
    while(c=='u' || c=='U' || c=='l' || c=='L')
    {
      *s++ = *AScanPtr++;
      c = *AScanPtr;
    }
    AToken = TOK_VALUE;
    return AToken;
  }

  if(AScanPtr[0]>='0' && AScanPtr[0]<='9')
  {
    c=*AScanPtr;
    while((c>='0' && c<='9'))
    {
      *s++ = *AScanPtr++;
      c=*AScanPtr;
    }
    if(c=='.')
    {
      *s++ = *AScanPtr++;
      c = *AScanPtr;
      while((c>='0' && c<='9'))
      {
        *s++ = *AScanPtr++;
        c=*AScanPtr;
      }
    }
    if(c=='e')
    {
      *s++ = *AScanPtr++;
      c=*AScanPtr;
      if(c=='-')
      {
        *s++ = *AScanPtr++;
        c=*AScanPtr;
      }
      while((c>='0' && c<='9'))
      {
        *s++ = *AScanPtr++;
        c=*AScanPtr;
      }
    }
    if(c=='f' || c=='F')
    {
      *s++ = *AScanPtr++;
      c = *AScanPtr;
    }
    else if(c=='l' || c=='L')
    {
      *s++ = *AScanPtr++;
      c = *AScanPtr;
    }
    else if(c=='u' || c=='U')
    {
      *s++ = *AScanPtr++;
      c = *AScanPtr;
    }
    if(c=='l' || c=='L')
    {
      *s++ = *AScanPtr++;
      c = *AScanPtr;
    }
    *s++ = 0;
    AToken = TOK_VALUE;
    return AToken;
  }

  if(AScanPtr[0]=='"')
  {
    *s++ = *AScanPtr++;
    while(*AScanPtr!='"')
    {
      if(*AScanPtr==0)
      {
        AToken = TOK_ERROR;
        return AToken;
      }
      if(*AScanPtr=='\\' && AScanPtr[1]!=0)
      {
        *s++ = *AScanPtr++;
      }
      *s++ = *AScanPtr++;
    }
    *s++ = *AScanPtr++;
    *s++ = 0;
    AToken = TOK_VALUE;
    return AToken;
  }

// default

  AToken = TOK_KEYWORD;
  *s++ = *AScanPtr++;
  *s++ = 0;
  return AToken;
}


sInt Scan()
{
  static condmode;
  static sChar buffer[256];
/*
  while(AToken==TOK_PRE)
  {
    Out("\n");
    Out(AValue);
    AScan();
  }
  */
  sCopyString(Value,AValue,sizeof(Value));
  Token=AToken;
  ScanPtr = AScanPtr;
  AScan();
  if(Token==TOK_COND)
    condmode = 1;
  /*if(Token==TOK_NAME && AToken==TOK_COLON && condmode==0)
  {
    Token=TOK_LABEL;
    sAppendString(Value,":",sizeof(Value));
    AScan();
  }*/
  if(Token==TOK_TYPE && AToken==TOK_SCOPE)
  {
    sCopyString(buffer,Value,sizeof(buffer));
    sAppendString(Value,"::",sizeof(Value));
    while(AToken==TOK_SCOPE)
    {
      AScan();
      if(AToken==TOK_NEG)
      {
        sAppendString(Value,"~",sizeof(Value));
        AScan();
      }
      if(AToken==TOK_NAME)
      {
        sAppendString(Value,AValue,sizeof(Value));
        Token = TOK_NAME;
        AScan();
        break;
      }
      if(AToken==TOK_TYPE)
      {
        sAppendString(Value,AValue,sizeof(Value));
        AScan();
      }
    }
  }
  if(Token==TOK_COLON)
    condmode = 0;
  return Token;
}

sInt Peek()
{
  return AToken;
}

/*
void ScanSafe()
{
  TokenPush = Token;
  ScanPtrPush = AScanPtr;
  sCopyString(ValuePush,Value,sizeof(ValuePush));
}

void ScanRestore()
{
  Token = TokenPush;
  AScanPtr = ScanPtrPush;
  sCopyString(Value,ValuePush,sizeof(ValuePush));
}

void PrintToken()
{
  switch(Token)
  {
  case TOK_NAME:
  case TOK_VALUE:
  case TOK_PRE:
    sPrintF("<%s>\n",Value);
    break;
  case TOK_KEYWORD:
    sPrintF("<%s>",Value);
    break;
  case TOK_EOF:
    sPrintF("EOF",Value);
    break;
  case TOK_ERROR:
    sPrintF("ERROR",Value);
    break;
  default:
    sPrintF("%s\n",Value);
    break;
  }
}
*/
void InlineAssembly()
{
  sChar buffer[256];
  sChar *ptr;
  sInt label;

  if(Token!=TOK_BOPEN)
  {
    Error("inline assembler");
    return;
  }

  NewLine();
  Out("{");
  Indent++;
  while(*ScanPtr!='}')
  {
    while(IsWhitespace(*ScanPtr))
      ScanPtr++;
    ptr = buffer;
    label = 0;
    while(*ScanPtr!='\n' && *ScanPtr!='\r' && *ScanPtr!='}')
    {
      if(*ScanPtr==0)
      {
        Token=TOK_EOF;
        Error("inline assembler");
        return;
      }
      if(*ScanPtr==':')
        label=1;
      *ptr++ = *ScanPtr++;
    }
    *ptr++ = 0;
    if(buffer[0])
    {
      if(label)
        Out("\n");
      else
        NewLine();
      Out(buffer);
    }
  }
  AScanPtr = ScanPtr;
  AScan();
  Scan();
  if(Token!=TOK_BCLOSE)
  {
    Error("inline assembler");
    return;
  }
  Scan();
  Indent--;
  NewLine();
  Out("}");
}

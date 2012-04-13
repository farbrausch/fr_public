// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "cslce.hpp"

#define DEBUGLEVEL 0

/****************************************************************************/
/****************************************************************************/

sChar *ScriptCompiler::MakeString(sChar *string)
{
  sInt i;
  sChar *str;

  for(i=0;i<StringCount;i++)
    if(sCmpString(Strings[i],string)==0)
      return Strings[i];
  str = StringBuffer+StringNext;
  Strings[StringCount++] = str;
  while(*string)
    StringBuffer[StringNext++] = *string++;
  StringBuffer[StringNext++] = 0;

  return str;
}

/****************************************************************************/

sU8 *ScriptCompiler::Alloc(sInt size)
{
  sU8 *mem;

  sVERIFY(size<100);
  sVERIFY(MemNext+size <= SC_MAXWORKSPACE);
  size = sAlign(size,4);
  mem = MemBuffer+MemNext;
  MemNext+=size;
  sSetMem(mem,0,size);

  return mem;
}

/****************************************************************************/

void ScriptCompiler::Error(sChar *text)
{
  if(!IsError)
  {
    sCopyString(ErrorText,text,sizeof(ErrorText));
    sCopyString(ErrorFile,File,sizeof(ErrorFile));
    ErrorLine = Line;
    ErrorSymbolPos = Token0-TextStart;
    ErrorSymbolWidth = Token1-Token0;
    sSPrintF(ErrorBuf,sizeof(ErrorBuf),"\n%s(%d) : %s",File,Line,text);
    sDPrintF("%s\n",ErrorBuf);
  }
  IsError = 1;
}

/****************************************************************************/
/****************************************************************************/

#define LETTER      0x01
#define DIGIT       0x02
#define HEX         0x04
#define WHITESPACE  0x08
#define OCT         0x10

static sU8 ScanTable[] =
{
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x08,0x08,0x00,0x00,0x08,0x00,0x00, // 0x00
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 0x10
  0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 0x20
  0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16, 0x06,0x06,0x00,0x00,0x00,0x00,0x00,0x00, // 0x30

  0x00,0x05,0x05,0x05,0x05,0x05,0x05,0x01, 0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01, // 0x40
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01, 0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x01, // 0x50
  0x00,0x05,0x05,0x05,0x05,0x05,0x05,0x01, 0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01, // 0x40
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01, 0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00, // 0x50

  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 0x80
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 0x90
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 0xa0
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 0xb0

  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 0xc0
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 0xd0
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 0xe0
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 0xf0
};

static sU8 BinOpLookup[0x200] =
{
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // 0x00
  0,0,0,0, 0,0,0,4, 4,5,5,5, 0,0,0,0, // 0x10
  0,2,2,2, 2,2,2,1, 0,0,0,0, 0,0,0,0, // 0x20
  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,3,3,0, // 0x30
};

/****************************************************************************/

void ScriptCompiler::AddToken(sInt tok,sChar *name)
{
  Tokens[tok] = MakeString(name);
}

/****************************************************************************/

void ScriptCompiler::Scan()
{
  ScanX();
#if DEBUGLEVEL&1
  PrintToken();
#endif
}

void ScriptCompiler::ScanX()
{
  sInt flags;
  sInt value;
  sInt frac,dec;
  sChar buffer[2048];
  sChar *s;
  sInt i;

morewhitespaces:
  while(ScanTable[*Text]&WHITESPACE)
  {
    if(Text[0]=='\n')
      Line++;
    Text++;
  }
  if(Text[0]=='/' && Text[1]=='*')
  {
    Text+=2;
    while(Text[0]!='*' || Text[1]!='/')
    {
      if(Text[0]=='\n')
        Line++;
      if(Text[0]==0)
      {
        Error("end of file in comment");
        Token=TOK_END;
        return;
      }
      Text++;
    }
    Text+=2;
    goto morewhitespaces;
  }
  if(Text[0]=='/' && Text[1]=='/')
  {
    Text+=2;
    while(Text[0]!='\n')
    {
      if(Text[0]==0)
      {
        Error("end of file in comment");
        Token=TOK_END;
        return;
      }
      Text++;
    }
    Text++;
    Line++;
    goto morewhitespaces;
  }


  flags = ScanTable[*Text];

  Token = TOK_ERROR;
  TokValue = 0;
  TokName = 0;
  TokFloatUsed = 0;
  Token1 = Token0 = Text;

  if(*Text==0)
  {
    Token = TOK_END;
    Token1 = Text;
    return;
  }
  if(*Text=='"')
  {
    s = buffer;
    Text++;
    while(*Text!='"' && *Text>=0x20)
    {
      if(*Text=='\n')
        Line++;
      if(Text[0]=='\\' && Text[1]!=0)
      {
        Text++;
        if(*Text=='n') *s++ = '\n';
        if(*Text=='t') *s++ = '\t';
        if(*Text=='r') *s++ = '\r';
        if(*Text=='f') *s++ = '\f';
        if(*Text=='"') *s++ = '"';
        if(*Text=='\'') *s++ = '\'';
        if(*Text=='\\') *s++ = '\\';
        Text++;
      }
      else
        *s++ = *Text++;
    }
    if(*Text!='"')
    {
      Token1 = Text;
      return;
    }
    Token=TOK_STRINGLIT;
    *s++ = 0;
    Text++;
    TokName = MakeString(buffer);
    Token1 = Text;
    return;
  }
  if(*Text=='\'')
  {
    Text++;
    value = 0;
    if(*Text=='\n' || *Text==0)
      Error("newline in character constant");
    else
    {
      if(Text[0]=='\\' && Text[1]!=0)
      {
        Text++;
        if(*Text=='n') value = '\n';
        if(*Text=='t') value = '\t';
        if(*Text=='r') value = '\r';
        if(*Text=='f') value = '\f';
        if(*Text=='"') value = '"';
        if(*Text=='\'') value = '\'';
        if(*Text=='\\') value = '\\';
        Text++;
      }
      else
        value = *Text++;
    }
    if(*Text!='\'')
      Error("character constant too long");
    else
      Text++;

    Token=TOK_INTLIT;
    TokValue = value<<16;
    Token1 = Text;
    return;
  }
  if(Text[0]=='0' && (Text[1]=='x' || Text[1]=='X'))
  {
    Text+=2;
    value = 0;
    dec = 0x10000;
    while(ScanTable[*Text]&HEX)
    {
      if(*Text<='9')
        value = value*16 + (((*Text++)-'0')*dec);
      else
        value = value*16 + ((((*Text++)&0x0f)+9)*dec);
    }
    if(*Text=='.' && Text[1]!='.')
    {
      TokFloatUsed = 1;
      Text++;
      while(ScanTable[*Text]&HEX)
      {
        dec = dec/16;
        if(*Text<='9')
          value = value + dec * ((*Text++)-'0');
        else
          value = value + dec * (((*Text++)&0x0f)+9);
      }
    }
    Token = TOK_INTLIT;
    TokValue = value;
    Token1 = Text;
    return;
  }
  if((flags&DIGIT) || (*Text=='.' && (ScanTable[Text[1]]&DIGIT)))
  {
    value = 0;
    while(ScanTable[*Text]&DIGIT)
    {
      value = value*10 + (((*Text++)-'0')<<16);
    }
    if(*Text=='.' && Text[1]!='.')
    {
      TokFloatUsed = 1;
      Text++;
      frac = 0;
      dec = 1;

      while(ScanTable[*Text]&DIGIT)
      {
        frac = frac*10 + (((*Text++)-'0'));
        dec = dec*10;
      }

      Token = TOK_INTLIT;
      TokValue = value | (frac*0x10000/dec);
    }
    else
    {
      Token = TOK_INTLIT;
      TokValue = value;
    }
    Token1 = Text;
    return;
  }
  if(*Text=='#')
  {
    Text++;
    value = 0;
    dec = 1;
    i = 0;
    while(ScanTable[*Text]&HEX)
    {
      if(*Text<='9')
        value = value*16 + ((*Text++)-'0');
      else
        value = value*16 + (((*Text++)&0x0f)+9);
      dec = dec*16;
      i++;
    }
    if(i<=6)
      value|=0xff000000;
    value = ((value&0xff00ff00))
          | ((value&0x00ff0000)>>16)
          | ((value&0x000000ff)<<16);
    Token = TOK_COLORLIT;
    TokValue = value;
    Token1 = Text;
    return;
  }
  if(flags&LETTER)
  {
    s = buffer;
    while(ScanTable[*Text]&(LETTER|DIGIT))
      *s++ = *Text++;
    *s++ = 0;
    Token = TOK_NAME;
    TokName = MakeString(buffer);
  } 
  else
  {
    buffer[0] = *Text++;
    buffer[1] = 0;
    if(*Text=='=' && (buffer[0]=='<' || buffer[0]=='>' || buffer[0]=='!' || buffer[0]=='='))
    {
      buffer[1] = *Text++;
      buffer[2] = 0;
    }
    if(*Text=='.' && buffer[0]=='.')
    {
      buffer[1] = *Text++;
      buffer[2] = 0;
    }
    TokName = MakeString(buffer);
  }
  for(i=0;i<256;i++)
    if(Tokens[i])
      if(sCmpString(TokName,Tokens[i])==0)
        Token = i;

  if(Token==TOK_ERROR)
    IsError = 1;
  Token1 = Text;
}

/****************************************************************************/

void ScriptCompiler::Match(sInt tok)
{
  if(tok!=Token)
    Error("syntax error");
  else
    Scan();
}

/****************************************************************************/

sChar *ScriptCompiler::MatchName()
{
  sChar *name;

  if(Token!=TOK_NAME)
  {
    Error("name expected");
    name = "???";
  }
  else
  {
    name = TokName;
    Scan();
  }

  return name;
}

/****************************************************************************/

sInt ScriptCompiler::MatchInt()
{
  sInt value;

  if(Token!=TOK_INTLIT)
  {
    Error("value expected");
    value = -1;
  }
  else
  {
    value = TokValue;
    Scan();
  }

  return value;
}

/****************************************************************************/

void ScriptCompiler::PrintToken()
{
  switch(Token)
  {
  case TOK_ERROR:
    sDPrintF("ERROR");
    break;
  case TOK_INTLIT:
    sDPrintF("%08x",TokValue);
    break;
  case TOK_STRINGLIT:
    sDPrintF("\"%s\"",TokName);
    break;
  case TOK_NAME:
    sDPrintF("<%s>",TokName);
    break;
  case TOK_END:
    sDPrintF("END");
    break;
  default:
    if(Tokens[Token])
      sDPrintF("%s",Tokens[Token]);
    else
      sDPrintF("???");
    break;
  }

  if(Token==TOK_SEMI)
    sDPrintF("\n");
  else
    sDPrintF(" ");
}

/****************************************************************************/
/****************************************************************************/

SCType *ScriptCompiler::AddType(sChar *name,sInt type,sInt count)
{
  SCType *t;

  t = NewType();
  t->Kind = SCS_TYPE;
  t->Name = MakeString(name);
  t->Base = type;
  t->Count = count;
  AddSymbol(t);
  return t;
}

/****************************************************************************/

void ScriptCompiler::AddSymbol(SCSymbol *sym,SCSymbol **list)
{
  if(list==0)
  {
    if(CurrentFunc)
    {
      AddSymbol(sym,&CurrentFunc->Locals);
    }
    else
    {
      AddSymbol(sym,&Symbols);
    }
  }
  else
  {
    if(FindSymbol(sym->Name,list))
      Error("symbol declared twice");
    sym->Next = *list;
    *list = sym;
  }
}

/****************************************************************************/

void ScriptCompiler::ConvTypeLeft(SCStatement *st,SCType *type)
{
  ConvType(st->Left,type);
}

void ScriptCompiler::ConvTypeRight(SCStatement *st,SCType *type)
{
  ConvType(st->Right,type);
}

void ScriptCompiler::ConvType(SCStatement *&st,SCType *type)
{
  SCStatement *conv;

  if(st->Type->Count!=type->Count)
    Error("type count mismatch");
  if(st->Type->Base!=type->Base)
  {
    if(st->Type->Base==SCT_INT && type->Base==SCT_FLOAT)
    {
      conv = NewStatement();
      conv->Left = st;
      conv->Source0 = st->Source0;
      conv->Source1 = st->Source1;
      conv->Token = OP_I2F;
      conv->Type = type;
      st = conv;
    }
    else if(st->Type->Base==SCT_FLOAT && type->Base==SCT_INT)
    {
      conv = NewStatement();
      conv->Left = st;
      conv->Source0 = st->Source0;
      conv->Source1 = st->Source1;
      conv->Token = OP_F2I;
      conv->Type = type;
      st = conv;
    }
    else
      Error("type base mismatch");
  }
}

/****************************************************************************/

SCSymbol *ScriptCompiler::FindSymbol(sChar *name,SCSymbol **list)
{
  SCSymbol *sym;
  if(list==0)
  {
    if(CurrentFunc && CurrentFunc->Locals)
    {
      sym = FindSymbol(name,&CurrentFunc->Locals);
      if(sym)
        return sym;
    }
    return FindSymbol(name,&Symbols);
  }
  
  sym = *list;
  while(sym)
  {
    if(sym->Name == name)
      return sym;
    sym = sym->Next;
  }
  return 0;
}

/****************************************************************************/
/****************************************************************************/

void ScriptCompiler::_Programm()
{
  while(!IsError && Token!=TOK_END)
  {
    _Statement();
  }
}

/****************************************************************************/

sU32 ScriptCompiler::ParseModifierString(sChar *scan)
{
  sU32 flags;
  sInt i,len,found;

  static sChar *tokens[] = { "copy","link","mod","nop","copy2","link2","mod2","nop2","multi","rowa","rowb","rowc","fxalloc" };

  flags = 0;
  while(*scan && !IsError)
  {
    found = 0;
    for(i=0;i<13 && !found;i++)
    {
      len = sGetStringLen(tokens[i]);
      if(sCmpMem(scan,tokens[i],len)==0 && (scan[len]==' '||scan[len]==0))
      {
        flags |= 1<<i;
        found = 1;
        scan+=len;
      }
    }
    if(!found)
      Error("Illegal Function Modifier String");
    
    while(*scan==' ') scan++;
  }


  return flags;
}

void ScriptCompiler::_Func(SCType *type,sChar *name,SCStatement *clause)
{
  SCFunction *func;
  SCFuncArg *arg;
  SCVar *var;
  sChar *source;

  source = Token0;
  func = (SCFunction *)FindSymbol(name);
  if(func!=0 && func->Kind!=SCS_FUNC)
  {
    Error("nonfunction redeclared as function");
    func = 0;
  }
  if(!func)
  {
    if(CurrentFunc)
      Error("local function");
    func = NewFunction();
    func->Kind = SCS_FUNC;
    func->Result = type;
    func->Name = name;
    AddSymbol(func);
  }
  _VarList(func);
  if(Token==TOK_OPEN)
    _OStack(func);
  if(Token==TOK_EQUAL)
  {
    Scan();
    if(func->Index!=0)
      Error("function number set twice");
    func->Index = _ConstExpr()>>16;
  }
  if(Token==TOK_COMMA)
  {
    Scan();
    if(Token==TOK_STRINGLIT)
      func->Flags = ParseModifierString(TokName);
    Match(TOK_STRINGLIT);
  }
  if(Token==TOK_COMMA)
  {
    Scan();
    func->Shortcut = _ConstExpr()>>16;
  }
  if(func->Index==0)
    func->Index = NextFunction++;

  if(Token==TOK_SEMI)
  {
    Scan();
  }
  else
  {
    NextLocal = 0;
    CurrentFunc = func;
    if(func->Block!=0)
      Error("function body set twice");
    arg = func->Args;
    while(arg)
    {
      var = NewVar();
      var->Type = arg->Type;
      var->Kind = SCS_VAR;
      var->Name = arg->Name;
      var->Scope = SCS_LOCAL;
      var->Index = NextLocal;
      NextLocal += var->Type->Count;

      AddSymbol(var,0);
      arg->Var = var;
      arg = arg->Next;
    }
    func->Block = _Block();
    func->MaxLocal = NextLocal;
    func->Block->Source0 = source;
    func->Clause = clause;
    CurrentFunc = 0;
  }

}

/****************************************************************************/

void ScriptCompiler::_VarList(SCFunction *func)
{
  SCFuncArg **arg;
  sInt offset;

  Match(TOK_OPEN);

  offset = 0;
  arg = &func->Args;

  while(!IsError && Token!=TOK_CLOSE)
  {
    *arg = _VarEntry(offset);
    arg = &(*arg)->Next;
    if(Token==TOK_COMMA)
      Scan();
    else
      break;
  }

  Match(TOK_CLOSE);
}

/****************************************************************************/

SCFuncArg *ScriptCompiler::_VarEntry(sInt &offset)
{
  SCFuncArg *arg;
  SCType *ct;
  arg = NewFuncArg();
  arg->Type = _Type();
  arg->Name = MatchName();
  arg->Offset = offset;

  offset += arg->Type->Count;
  if(Token==TOK_EQUAL)
  {
    Scan();
    ct = arg->Type;
    if(arg->Type->Base==SCT_FLOAT)
      ct = IntTypes[ct->Count];

    if(_ConstExprVector(arg->Default,ct))
      arg->Flags |= SCFA_FLOAT;
    arg->Flags |= SCFA_DEFAULT;
    if(Token==TOK_SOPEN)
    {
      Scan();
      if(Token==TOK_STRINGLIT)
      {
        arg->GuiSpecial = TokName;
        arg->Flags |= SCFA_SPECIAL;
        Scan();
      }
      if(Token!=TOK_SCLOSE && Token!=TOK_STAR)
      {
        _ConstExprVector(arg->GuiMin,ct);
        arg->Flags |= SCFA_MIN;
        if(Token==TOK_DOTDOT)
        {
          Match(TOK_DOTDOT);
          _ConstExprVector(arg->GuiMax,ct);
          arg->Flags |= SCFA_MAX;
        }
      }
      if(Token==TOK_STAR)
      {
        Scan();
        arg->GuiSpeed = _ConstExpr();
        arg->Flags |= SCFA_SPEED;
      }
      Match(TOK_SCLOSE);
    }
  }
  return arg;
}

/****************************************************************************/

void ScriptCompiler::_OStack(SCFunction *func)
{
  SCType *type;
  sInt i;

  Scan();
  i = 0;
  while(Token==TOK_NAME && !IsError)
  {
    type = (SCType *)FindSymbol(TokName);
    if(type==0 || type->Kind!=SCS_TYPE || type->Base!=SCT_OBJECT)
      Error("non-class in ostack");
    if(i<4)
      func->OStackIn[i++] = type->Cid;
    Scan();
  }

  Match(TOK_MINUS); 
  Match(TOK_MINUS); 
  i = 0;
  while(Token==TOK_NAME && !IsError)
  {
    type = (SCType *)FindSymbol(TokName);
    if(type==0 || type->Kind!=SCS_TYPE || type->Base!=SCT_OBJECT)
      Error("non-class in ostack");
    if(i<4)
      func->OStackOut[i++] = type->Cid;
    Scan();
  }
  Match(TOK_CLOSE); 
}

/****************************************************************************/

SCStatement *ScriptCompiler::_Block()
{
  SCStatement *st,*base,**stp;
  sChar *start;
  if(Token==TOK_COPEN)
  {
    start = Token0;
    Scan();
    base = 0;
    stp = &base;
    while(Token!=TOK_CCLOSE && !IsError)
    {
      st = NewStatement();
      st->Source0 = start;
      st->Token = OP_GLUE;
      *stp = st;
      st->Left = _Statement();
      start = st->Source1 = Token0;
      stp = &st->Right;
    }
    if(base!=0)
    {
      base->Source1 = Token1;
    }
    Scan();

    if(base==0)
    {
      base = NewStatement();
      base->Source0 = Token0;
      base->Source1 = Token0;
      base->Token = OP_GLUE;
    }
  }
  else
  {
    base =  _Statement();
  }
  return base;
}

/****************************************************************************/

SCStatement *ScriptCompiler::_Statement()
{
  SCStatement *st,*st2;
  SCVar *var;
  SCSymbol *sym;
  SCConst *con;

  st = NewStatement();
  if(IsError) return st;
  st->Token = Token;
  st->Source0 = Token0;
  switch(Token)
  {
  case TOK_NAME:
    sym = FindSymbol(TokName);
    if(sym==0)
      Error("unknown symbol");
    else if(sym->Kind==SCS_TYPE)
    {
      Scan();
      var = NewVar();
      var->Type = (SCType *)sym;
      var->Kind = SCS_VAR;
      var->Name = MatchName();
      if(Token==TOK_OPEN)
      {
        _Func(var->Type,var->Name,st);
        st->Source1 = Token0;
      }
      else
      {
        while(!IsError)
        {
          if(CurrentFunc)
          {
            var->Index = NextLocal;
            var->Scope = SCS_LOCAL;
            NextLocal += var->Type->Count;
          }
          else
          {
            var->Index = NextGlobal;
            var->Scope = SCS_GLOBAL;
            NextGlobal += var->Type->Count;
          }
          if(!IsError)
          {
            AddSymbol(var,0);
          }
          st->Source1 = Token1;

          if(Token==TOK_SEMI)
            break;
          Match(TOK_COMMA);

          var = NewVar();
          var->Type = (SCType *)sym;
          var->Kind = SCS_VAR;
          var->Name = MatchName();
        }
        Match(TOK_SEMI);
      }
      st->Token = OP_GLUE;
    }
    else
    {
      goto expr;
    }
    break;

  case TOK_CONST:
    Scan();
    con = NewConst();
    con->Type = _Type();
    con->Name = MatchName();
    con->Kind = SCS_CONST;
    Match(TOK_EQUAL);
    con->Value[0] = _ConstExpr();
    if(con->Type!=IntTypes[1])
      Error("const must be int");
    if(!IsError)
      AddSymbol(con,0);
    break;

  case TOK_CLASS:
    Scan();
    _Class();
    break;
     
  case TOK_RETURN:
    if(CurrentFunc==0) Error("not in global scope");
    Scan();
    st->Right = _Expr();
    ConvTypeRight(st,CurrentFunc->Result);
//    if(CurrentFunc->Result!=st->Right->Type)
//      Error("result type mismatch");
    st->Source1 = Token1;
    Match(TOK_SEMI);
    break;

  case TOK_IF:
    Scan();
    st2 = NewStatement();
    st2->Token = OP_GLUE;
    st2->Source0 = st2->Source0;

    Match(TOK_OPEN);
    st->Left = _Expr();
    ConvTypeLeft(st,IntTypes[1]);
    Match(TOK_CLOSE);
    st->Right = st2;

    st2->Left = _Block();
    if(Token==TOK_ELSE)
    {
      Scan();
      st2->Right = _Block();
    }
    st2->Source0 = Token0;
    st->Source1 = Token0;
    break;

  case TOK_WHILE:
    Scan();
    Match(TOK_OPEN);
    st->Left = _Expr();
    ConvTypeLeft(st,IntTypes[1]);
    Match(TOK_CLOSE);
    st->Right = _Block();
    st->Source1 = Token0;
    break;

  case TOK_DO:
    Scan();
    st->Left = _Block();
    Match(TOK_WHILE);
    Match(TOK_OPEN);
    st->Right = _Expr();
    ConvTypeRight(st,IntTypes[1]);
    st->Source1 = Token1;
    Match(TOK_CLOSE);
    break;

  case TOK_LOAD:
    if(CurrentFunc==0) Error("not in global scope");
    Scan();
    st->Left = _Var();
    if(IsError) 
      break;
    if(st->Left && st->Left->Var && st->Left->Var->Type->Base!=SCT_OBJECT)
      Error("load only for objects");
    st->Source1 = Token1;
    Match(TOK_SEMI);
    break;

  case TOK_STORE:
    if(CurrentFunc==0) Error("not in global scope");
    st->Source0 = Token0;
    Scan();
    st->Left = _Var();
    if(IsError) 
      break;
    if(st->Left && st->Left->Var->Type->Base!=SCT_OBJECT)
      Error("store only for objects");
    st->Source1 = Token1;
    Match(TOK_SEMI);
    break;

  case TOK_DROP:
    if(CurrentFunc==0) Error("not in global scope");
    st->Source0 = Token0;
    Scan();
    st->Source1 = Token1;
    Match(TOK_SEMI);
    break;

  case TOK_SEMI:
    Scan();
    break;

  default:
  expr:
    if(CurrentFunc==0) Error("not in global scope");
    st2 = NewStatement();
    st2->Source0 = Token0;
    st2->Left = _Expr();
    st2->Source1 = Token1;
    st2->Token = OP_FINISH;
    st->Token = OP_GLUE;
    st->Left = st2;
    st->Source1 = Token1;
    Match(TOK_SEMI);
    break;
  }

  return st;
}

void ScriptCompiler::_Class()
{
  sChar *name;
  SCType *t;
  sU32 cid;
  SCFuncArg *mem,**mp;
  sInt offset;

  name = MatchName();
  Match(TOK_EQUAL);
  cid = _ConstExpr();

  if(IsError)
    return;

  t = NewType();
  t->Kind = SCS_TYPE;
  t->Name = name;
  t->Count = 1;
  t->Base = SCT_OBJECT;
  t->Cid = cid;
  mp = &t->Members;
  offset = 0;

  if(Token!=TOK_SEMI)
  {
    Match(TOK_COPEN);
    while(Token!=TOK_CCLOSE && !IsError)
    {
      mem = _VarEntry(offset);
      *mp = mem;
      mp = &mem->Next;
      Match(TOK_SEMI);
    }
    Match(TOK_CCLOSE);
  }
  Match(TOK_SEMI);

  AddSymbol(t);
}

/****************************************************************************/

SCStatement *ScriptCompiler::_Expr()
{
  return _CExpr(); 
}

/****************************************************************************/

SCStatement *ScriptCompiler::_CExpr()
{
  return _BExpr(1); 
}

/****************************************************************************/

SCStatement *ScriptCompiler::_BExpr(sInt level)
{
  SCStatement *st,*base;
  sInt i;

  base = _UExpr();
  for(i=BinOpLookup[Token];i>=level;i--)
  {
    while(BinOpLookup[Token]==i)
    {
      st = base;
      if(BinOpLookup[Token]!=0)
      {
        base = NewStatement();
        base->Source0 = st->Source0;
        base->Token = Token;
        Scan();
        base->Left = st;
        base->Right = _BExpr(i+1);
        if(!IsError)
        {
          sVERIFY(base->Left);
          sVERIFY(base->Right);

          if(base->Token==TOK_EQUAL)
          {
            if(base->Left->Type->Base==SCT_FLOAT && base->Right->Type->Base==SCT_INT)
              ConvTypeRight(base,FloatTypes[base->Right->Type->Count]);
            if(base->Left->Type->Base==SCT_INT && base->Right->Type->Base==SCT_FLOAT)
              ConvTypeRight(base,IntTypes[base->Right->Type->Count]);
          }
          else
          {
            if(base->Left->Type->Base==SCT_FLOAT)
              ConvTypeLeft(base,IntTypes[base->Left->Type->Count]);
            if(base->Right->Type->Base==SCT_FLOAT)
              ConvTypeRight(base,IntTypes[base->Right->Type->Count]);
          }
          if(base->Left->Type!=base->Right->Type)
          {
            if(base->Token==TOK_STAR && base->Left->Type->Base == SCT_INT && base->Right->Type->Base==SCT_INT && base->Left->Type->Count>1 && base->Right->Type->Count==1)
              base->Token = OP_SCALE;
            else
              Error("type mismatch in binary expression");
          }
        }

        base->Type = base->Left->Type;
        base->Source0 = st->Source0;
        base->Source1 = Token0;
      }
    }
  }
  return base; 
}

/****************************************************************************/


SCStatement *ScriptCompiler::_UExpr()
{
  SCStatement *st,*st2,**stp;
  sInt i;
  sChar *name;
  SCSymbol *sym;

// unary

  switch(Token)
  {
  case TOK_OPEN:
    Scan();
    st = _Expr();
    Match(TOK_CLOSE);
    break;
  case TOK_INTLIT:
    st = NewStatement();
    st->Token = TOK_INTLIT;
    st->Value = TokValue;
    st->Type = IntTypes[1];
    st->Source0 = Token0;
    st->Source1 = Token1;
    Scan();
    break;

  case TOK_STRINGLIT:
    st = NewStatement();
    st->Token = TOK_STRINGLIT;
    st->Value = DataNext;
    st->Type = StringType;
    st->Source0 = Token0;
    st->Source1 = Token1;
    i = sGetStringLen(TokName)+1;
    sCopyMem(DataBuffer+DataNext,TokName,i);
    DataNext += i;
    Scan();
    break;

  case TOK_COLORLIT:
    st = NewStatement();
    st->Token = TOK_COLORLIT;
    st->Value = TokValue;
    st->Type = IntTypes[4];
    st->Source0 = Token0;
    st->Source1 = Token1;
    Scan();
    break;

  case TOK_SOPEN:
    st= NewStatement();
    st->Token = OP_COMPOUND;
    stp = &st->Right;
    st->Source0 = Token0;
    i = 0;
    Scan();
    for(;;)
    {
      st2 = NewStatement();
      st2->Source0 = Token0;
      st2->Left = _Expr();
      st2->Source1 = Token0;
      st2->Token = OP_GLUE;
      if(st2->Left->Type->Base!=SCT_INT)
        Error("only int in []");
      i+=st2->Left->Type->Count;
      *stp = st2;
      stp = &st2->Right;
      if(Token!=TOK_COMMA)
        break;
      Scan();
    }
    if(i>4)
    {
      Error("too many ints in []");
      i=4;
    }
    st->Type = IntTypes[i];
    st->Source1 = Token1;
    Match(TOK_SCLOSE);
    break;
    
  case TOK_NAME:
    sym = FindSymbol(TokName);
    if(sym==0)
    {
      Error("unknown symbol");
      st = 0;
    }
    else
    {
      if(sym->Kind==SCS_FUNC)
      {
        st = _Call();
      }
      else
      {
        st = _Var();
      }
    }
    break;

  case TOK_MINUS:
    st = NewStatement();
    st->Source0 = Token0;
    Scan();
    st->Left = NewStatement();
    st->Left->Token = TOK_INTLIT;
    st->Left->Value = 0;
    st->Left->Type = IntTypes[1];
    st->Right = _UExpr();
    st->Source1 = Token0;
    st->Token = TOK_MINUS;
    st->Type = IntTypes[1];
    ConvTypeLeft(st,st->Type);
//    if(st->Left->Type!=st->Right->Type)
//      Error("type mismatch in binary expression");
   
    break;

  default:
    st = 0;
    Error("error in unary");
    break;
  }

  if(st==0)
  {
    st = NewStatement();
    st->Token = TOK_INTLIT;
    st->Value = 0;
    st->Type = IntTypes[1];
    st->Source0 = Token0;
    st->Source1 = Token1;
  }

  while(Token==TOK_DOT)
  {
    st2 = NewStatement();
    st2->Source0 = Token0;
    st->Source0 = Token0;
    Scan();
    st->Source1 = Token1;
    name = MatchName();
    st2->Token = TOK_DOT;
    st2->Left = st;
    if(!IsError)
    {
      switch(st->Type->Base)
      {
      case SCT_OBJECT:
        SCFuncArg *member;
        member = st->Type->Members;
        while(member)
        {
          if(member->Name==name)
            break;
          member = member->Next;
        }
        if(member)
        {
          st2->Type = member->Type;
          st2->Value = member->Offset;
        }
        else
        {
          Error("unknown object member");
        }
        break;

      case SCT_INT:
        st2->Type = IntTypes[1];
        goto skipint;

      case SCT_FLOAT:
        st2->Type = FloatTypes[1];
skipint:
        st2->Value = -1;
        if(name[1]==0)
        {
          if(name[0]=='x') st2->Value = 0;
          if(name[0]=='y') st2->Value = 1;
          if(name[0]=='z') st2->Value = 2;
          if(name[0]=='w') st2->Value = 3;
          if(name[0]=='r') st2->Value = 0;
          if(name[0]=='g') st2->Value = 1;
          if(name[0]=='b') st2->Value = 2;
          if(name[0]=='a') st2->Value = 3;
        }
        if(st2->Value==-1)
          Error("illegal name after dot");
        else if(st->Type->Count<=st2->Value)
          Error("dot out of range");
        break;

      default:
        Error("no void for dot");
        break;
      }
    }
    st = st2;
  }

  return st; 
}

/****************************************************************************/

SCType *ScriptCompiler::_Type()
{
  sChar *name;
  SCType *type;

  name = MatchName();
  type = (SCType *)FindSymbol(name);
  if(type==0 || type->Kind!=SCS_TYPE)
  {
    Error("could not find type");
    type = IntTypes[1];
  }

  return type; 
}

/****************************************************************************/

SCStatement *ScriptCompiler::_Var()
{
  sChar *name;
  SCStatement *st;
  SCVar *var;
  SCConst *con;


  st = NewStatement();
  st->Source0 = Token0;
  st->Source1 = Token1;

  name = MatchName();
  if(!IsError)
  {
    var = (SCVar *)FindSymbol(name);
    if(var && var->Kind==SCS_VAR)
    {
      st->Token = OP_VAR;
      st->Var = var;
      st->Type = var->Type;
    }
    else if(var && var->Kind==SCS_CONST)
    {
      con = (SCConst *)var;
      st->Token = TOK_INTLIT;
      st->Type = con->Type;
      st->Value = con->Value[0];
    }
    else
    {
      Error("variable not found");
    }
  }

  return st; 
}

/****************************************************************************/

sBool ScriptCompiler::_ConstExprVector(sInt *vector,SCType *type)
{
  sBool floatused;
  sInt i;

  floatused = 0;
  if(type->Base==SCT_INT)
  {
    if(type->Count==1)
    {
      vector[0] = _ConstExpr(&floatused);
    }
    else if(type->Count==4 && Token==TOK_COLORLIT)
    {
      vector[0] = (TokValue    )&0xff; vector[0] = vector[0]<<8;
      vector[1] = (TokValue>> 8)&0xff; vector[1] = vector[1]<<8;
      vector[2] = (TokValue>>16)&0xff; vector[2] = vector[2]<<8;
      vector[3] = (TokValue>>24)&0xff; vector[3] = vector[3]<<8;
      floatused = 1;
      Scan();
    }
    else
    {
      Match(TOK_SOPEN);
      vector[0] = _ConstExpr(&floatused);
      for(i=1;i<type->Count;i++)
      {
        Match(TOK_COMMA);
        vector[i] = _ConstExpr(&floatused);
      }
      Match(TOK_SCLOSE);
    }
  }
  else
  {
    Error("wrong type for constant expression");
  }

  return floatused;
}

sInt ScriptCompiler::_ConstExpr(sBool *floatused)
{
  sInt value;
  sInt sign;
  sign = 1;
  value = 0;
  if(Token==TOK_MINUS)
  {
    sign = -1;
    Scan();
  }
  if(floatused) *floatused |= TokFloatUsed;
  value = MatchInt(); 
  return value*sign;
}

/****************************************************************************/

SCStatement *ScriptCompiler::_Call()
{
  SCStatement *st,*base,**stp;
  SCStatement *args[32];
  SCFunction *func;
  SCFuncArg *tc;
  sInt i,max;

  base = NewStatement();
  base->Source0 = Token0;
  func = (SCFunction *)FindSymbol(MatchName());
  sVERIFY(func!=0 && func->Kind==SCS_FUNC);
  base->Token = OP_CALL;
  base->Value = func->Index;
  base->Type = func->Result;

  Match(TOK_OPEN);  i = 0;
  while(!IsError && Token!=TOK_CLOSE)
  {
    args[i++] =  _Expr();
    sVERIFY((sInt)args[i-1]!=0xcccccccc);
    if(Token==TOK_COMMA)
      Scan();
    else
      break;
  }
  base->Source1 = Token1;
  Match(TOK_CLOSE);
  max = i;

  stp = &base->Left;
  tc = func->Args;
  for(i=0;i<max;i++)
  {
    st = NewStatement();
    *stp = st;
    stp = &st->Right;
    st->Left = args[i];
    st->Token = OP_ARG;
    if(tc==0)
      Error("too many args");
    if(!IsError)
      ConvTypeLeft(st,tc->Type);
//    else if(tc->Type!=st->Left->Type)
//      Error("arg type mismatch");
    if(tc)            // possible of by one bug!
      tc=tc->Next;
  }
  if(tc)
    Error("too few args");

  return base;
}

/****************************************************************************/

#if DEBUGLEVEL&2
void LogPrint(sChar *s)
{
  sDPrintF(s);
}
#else
#define LogPrint(x) 
#endif

/****************************************************************************/

void ScriptCompiler::Emit(sU32 code)
{
#if DEBUGLEVEL&4
  sDPrintF("[%08x@%04x] ",code,CodePtr-CodeBuffer);
#endif
  *CodePtr++ = code;
}

/****************************************************************************/

void ScriptCompiler::PrintArgs(SCFuncArg *arg)
{
  sInt i;
  if(arg)
  {
    PrintArgs(arg->Next);
    
    for(i=arg->Type->Count-1;i>=0;i--)
    {
      sVERIFY(arg->Var);
      PrintVar(arg->Var,RTC_STORE,i);
    }
  }
}

void ScriptCompiler::PrintVar(SCStatement *st,sU32 code,sInt loop)
{
  if(st->Token==OP_VAR)
  {
    PrintVar(st->Var,code,loop);
  }
  else if(st->Token==TOK_DOT)
  {
    if(st->Left->Type->Base==SCT_OBJECT)
    {
      PrintStatement(st->Left,0);
      if(code==RTC_STORE)
      {
#if DEBUGLEVEL&2
        sDPrintF("storeobj(%d+%d) ",st->Value,loop);
#endif
        code = RTC_STOREMEMI+st->Value+loop;
      }
      else
      {
#if DEBUGLEVEL&2
        sDPrintF("loadobj(%d+%d) ",st->Value,loop);
#endif
        code = RTC_FETCHMEMI+st->Value+loop;
      }
      if(st->Type->Base==SCT_INT || st->Type->Base==SCT_FLOAT)
        code |= 0x00000000;
      else if(st->Type->Base==SCT_OBJECT)
        code |= 0x01000000;
      else
        Error("var must not be void");
      Emit(code);
    }
    else
    {
      loop += st->Value;
      PrintVar(st->Left,code,loop);
    }
  }
  else
  {
    Error("lvalue expected");
  }
}

void ScriptCompiler::PrintVar(SCVar *var,sU32 code,sInt loop)
{
  sChar *ls;
  
  ls = (code==RTC_STORE?"store":"load");
  if(loop==-1)
    Error("loop error");
  code |= var->Index+loop;
  if(var->Scope==SCS_LOCAL)
  {
#if DEBUGLEVEL&2
    sDPrintF("%s<%s>l:%d+%d ",ls,var->Name,var->Index,loop);
#endif
    code |= 0x00000000;
  }
  else
  {
#if DEBUGLEVEL&2
    sDPrintF("%s<%s>g:%d+%d ",ls,var->Name,var->Index,loop);
#endif
    code |= 0x04000000;
  }

  if(var->Type->Base==SCT_INT || var->Type->Base==SCT_FLOAT)
    code |= 0x00000000;
  else if(var->Type->Base==SCT_OBJECT)
    code |= 0x01000000;
  else
    Error("var must not be void");

  Emit(code);
}

void ScriptCompiler::PrintStatement(SCStatement *st,sInt loop)
{
  sInt i,max;
  SCOpPara *op;
  SCOpStatement *os;
  sInt code;

  if(st==0)
  {
    LogPrint("###NULL### ");
    Error("null statement");
    return;
  }
  switch(st->Token)
  {
  case OP_GLUE:
    if(st->Left)
      PrintStatement(st->Left,loop);
    if(st->Right)
      PrintStatement(st->Right,loop);
    break;
  case OP_FINISH:
    PrintStatement(st->Left,loop);
    break;
  case TOK_EQUAL:
    if(loop!=-1)
      Error("loop error");
    for(i=0;i<st->Type->Count;i++)
    {
      PrintStatement(st->Right,i);
      PrintVar(st->Left,RTC_STORE,i);
      LogPrint("\n    ");
    }
    break;
  case TOK_PLUS:
    code = RTC_ADD;
    goto binary;
  case TOK_MINUS:
    code = RTC_SUB;
    goto binary;
  case TOK_STAR:
    code = RTC_MUL;
    goto binary;
  case TOK_SLASH:
    code = RTC_DIV;
    goto binary;
  case TOK_PERCENT:
    code = RTC_MOD;
    goto binary;
  case TOK_AND:
    code = RTC_AND;
    goto binary;
  case TOK_OR:
    code = RTC_OR;
    goto binary;
  case TOK_GE:
    code = RTC_GE;
    goto binary;
  case TOK_LE:
    code = RTC_LE;
    goto binary;
  case TOK_GT:
    code = RTC_GT;
    goto binary;
  case TOK_LT:
    code = RTC_LT;
    goto binary;
  case TOK_EQ:
    code = RTC_EQ;
    goto binary;
  case TOK_NE:
    code = RTC_NE;
//    goto binary;

  binary:
    if(loop==-1)
      Error("loop error");
    PrintStatement(st->Left,loop);
    PrintStatement(st->Right,loop);
#if DEBUGLEVEL&2
    sDPrintF("%s ",Tokens[st->Token]);
#endif
    Emit(code);
    break;
  case OP_SCALE:
    PrintStatement(st->Right,0);
    PrintStatement(st->Left,loop);
    Emit(RTC_MUL); LogPrint("* ");
    break;

  case OP_F2I:
    if(loop==-1)
      Error("loop error");    
    PrintStatement(st->Left,loop);
    Emit(RTC_F2I); LogPrint("F2I ");
    break;

  case OP_I2F:
    if(loop==-1)
      Error("loop error");    
    PrintStatement(st->Left,loop);
    Emit(RTC_I2F); LogPrint("I2F ");
    break;

  case TOK_IF:
    if(loop!=-1)
      Error("loop error");    
    PrintStatement(st->Left,0);
    Emit(RTC_IF); LogPrint("if ");
    PrintStatement(st->Right->Left,loop);
    if(st->Right->Right)
    {
      Emit(RTC_ELSE); LogPrint("else ");
      PrintStatement(st->Right->Right,loop);
    }
    Emit(RTC_THEN); LogPrint("then ");
    break;

  case TOK_WHILE:
    if(loop!=-1)
      Error("loop error");
    Emit(RTC_DO); LogPrint("do ");
    PrintStatement(st->Left,0);
    Emit(RTC_WHILE); LogPrint("while ");
    PrintStatement(st->Right,loop);
    Emit(RTC_REPEAT); LogPrint("repeat ");
    break;

  case TOK_DO:
    if(loop!=-1)
      Error("loop error");
    Emit(RTC_DO); LogPrint("do ");
    PrintStatement(st->Left,loop);
    PrintStatement(st->Right,0);
    Emit(RTC_WHILE); LogPrint("while ");
    Emit(RTC_REPEAT); LogPrint("repeat ");
    break;

  case TOK_RETURN:
    if(loop!=-1)
      Error("loop error");
    for(i=0;i<st->Right->Type->Count;i++)
      PrintStatement(st->Right,i);
    Emit(RTC_RTS);
    LogPrint(" return\n    ");
    break;

  case TOK_LOAD:
    if(st->Left->Token!=OP_VAR)
    {
      Error("load works only for vars");
    }
    else
    {
      if(OpStatements)
      {
        os = &OpStatements[OpStatementCount++];
        os->Function = 0x10000+TOK_LOAD;
        os->ParaStart = OpParaCount;
        os->ParaCount = st->Left ? 1 : 0;
        os->Source0 = st->Source0;
        os->Source1 = st->Source1;
        os->Code0 = CodePtr;
      }
      if(OpParas && st->Left)
      {
        op = &OpParas[OpParaCount++];
        op->Count = st->Left->Type->Count;
        op->Type = 1;
        op->Source0 = st->Left->Source0;
        op->Source1 = st->Left->Source1;
      }
      if(st->Left->Var->Type->Base!=SCT_OBJECT)
        Error("load works only for object vars");
      else
        PrintVar(st->Left->Var,RTC_FETCH,0);
      if(OpStatements)
        os->Code1 = CodePtr;
    }
    LogPrint("\n    ");
    break;

  case TOK_STORE:
    if(st->Left->Token!=OP_VAR)
    {
      Error("store works only for vars");
    }
    else
    {
      if(OpStatements)
      {
        os = &OpStatements[OpStatementCount++];
        os->Function = 0x10000+TOK_STORE;
        os->ParaStart = OpParaCount;
        os->ParaCount = st->Left ? 1 : 0;
        os->Source0 = st->Source0;
        os->Source1 = st->Source1;
        os->Code0 = CodePtr;
      }
      if(OpParas && st->Left)
      {
        op = &OpParas[OpParaCount++];
        op->Count = st->Left->Type->Count;
        op->Type = 1;
        op->Source0 = st->Left->Source0;
        op->Source1 = st->Left->Source1;
      }
      if(st->Left->Var->Type->Base!=SCT_OBJECT)
        Error("store works only for object vars");
      else
        PrintVar(st->Left->Var,RTC_STORE,0);
      if(OpStatements)
        os->Code1 = CodePtr;
    }
    LogPrint("\n    ");
    break;

  case TOK_DROP:
    LogPrint("odrop ");
    if(OpStatements)
    {
      os = &OpStatements[OpStatementCount++];
      os->Function = 0x10000+TOK_DROP;
      os->ParaStart = OpParaCount;
      os->ParaCount = st->Left ? 1 : 0;
      os->Source0 = st->Source0;
      os->Source1 = st->Source1;
      os->Code0 = CodePtr;
    }
    Emit(RTC_STOREGLOO|(RT_MAXGLOBAL-1));
    if(OpStatements)
      os->Code1 = CodePtr;
    break;

  case OP_VAR:
  case TOK_DOT:
    if(loop==-1)
      Error("loop error");
    PrintVar(st,RTC_FETCH,loop);
    break;

  case OP_CALL:
    if(loop==-1 || loop==0)
    {
      if(OpStatements)
      {
        os = &OpStatements[OpStatementCount++];
        os->Function = st->Value;
        os->ParaStart = OpParaCount;
        os->Source0 = st->Source0;
        os->Source1 = st->Source1;
        os->Code0 = CodePtr;
      }
      if(st->Left)
        PrintStatement(st->Left,loop);
      if(OpStatements)
      {
        os->ParaCount = OpParaCount-OpStatements[OpStatementCount].ParaStart;
        os->Code1 = CodePtr;
      }
#if DEBUGLEVEL&2
      sDPrintF("call(%d) ",st->Value);
#endif
      if(st->Value<0)
        Emit(RTC_CODE|(-st->Value));
      else
        Emit(RTC_USER|st->Value);
      LogPrint("\n    ");

      for(i=st->Type->Count-1;i>=1;i--)
      {
        Emit(RTC_STORELOCI|(this->NextLocal++));
      }
    }
    if(loop>=1)
    {
      Emit(RTC_FETCHLOCI|(--this->NextLocal));
    }
    break;

  case OP_ARG:
    if(OpParas)
    {
      op = &OpParas[OpParaCount++];
      op->Count = st->Left->Type->Count;
      op->Type = 1;
      op->Source0 = st->Left->Source0;
      op->Source1 = st->Left->Source1;
    }
    for(i=0;i<st->Left->Type->Count;i++)
    {
      if(OpParas)
        op->Code0[i] = CodePtr;
      PrintStatement(st->Left,i);
      LogPrint("arg ");   /* emit nothing! */
      if(OpParas)
        op->Code1[i] = CodePtr;
    }
    if(st->Right)
      PrintStatement(st->Right,loop);
    break;

  case OP_COMPOUND:
    if(loop==-1)
      Error("loop error");
    i=0;
    max = st->Type->Count;
    for(i=0;i<max;)
    {
      st = st->Right;
      if(st && loop>=i && loop < i+st->Left->Type->Count)
      {
         PrintStatement(st->Left,loop-i);
      }  
      i+=st->Left->Type->Count;
    }
    break;

  case TOK_INTLIT:
    if(loop==-1)
      Error("loop error");
#if DEBUGLEVEL&2
    sDPrintF("%08x ",st->Value);
#endif
    Emit(RTC_IMMMASK|(st->Value&0x7fffffff));
    if(st->Value<-0x40000000 || st->Value>0x3fffffff)
      Emit(RTC_I31);
//    else if(OpParas)
//      Emit(RTC_NOP);
    break;
  case TOK_STRINGLIT:
    if(loop==-1)
      Error("loop error");
#if DEBUGLEVEL&2
    sDPrintF("string ",st->Value);
#endif
    Emit(RTC_STRING|st->Value);
    break;
  case TOK_COLORLIT:
    if(loop==-1)
      Error("loop error");
    i = (st->Value>>(loop*8))&0xff;
    i = /*i|*/(i<<8);
    Emit(RTC_IMMMASK|i);
//    if(OpParas)
//      Emit(RTC_NOP);
#if DEBUGLEVEL&2
    sDPrintF("%08x ",i);
#endif
    break;
  default:
#if DEBUGLEVEL&2
    if(Tokens[st->Token])
      sDPrintF("###%s### ",Tokens[st->Token]);
    else
      sDPrintF("###%04x### ",st->Token);
#endif
    Error("unknown token in parsetree");
    break;
  }
}

/****************************************************************************/
/****************************************************************************/

ScriptCompiler::ScriptCompiler()
{
  Strings = new sChar*[SC_MAXSTRINGCOUNT];
  StringBuffer = new sChar[SC_MAXSTRINGSIZE];
  MemBuffer = new sU8[SC_MAXWORKSPACE];
  CodeBuffer = new sU32[SC_MAXCODE];
  DataBuffer = new sU8[SC_MAXDATA];

  OpParas = 0;
  OpStatements = 0;
  OpFunctions = 0;

  Reset();
}

/****************************************************************************/

ScriptCompiler::~ScriptCompiler()
{
  delete[] Strings;
  delete[] StringBuffer;
  delete[] MemBuffer;
  delete[] CodeBuffer;
  delete[] DataBuffer;
  if(OpParas) delete[] OpParas;
  if(OpStatements) delete[] OpStatements;
  if(OpFunctions) delete[] OpFunctions;
}

/****************************************************************************/

void ScriptCompiler::InitOps()
{
  OpParas = new SCOpPara[SC_MAXOPPARA];
  OpStatements = new SCOpStatement[SC_MAXOPSTATEMENT];
  OpFunctions = new SCOpFunction[SC_MAXOPFUNCTION];
}

/****************************************************************************/

void ScriptCompiler::Reset()
{
  sInt i;

  for(i=0;i<256;i++)
    Tokens[i] = 0;

  StringCount = 0;
  StringNext = 0;
  MemNext = 0;
  DataNext = 0;
  CodePtr = CodeBuffer;

  Token = 0;
  TokValue = 0;
  TokName = 0;
  TokFloatUsed = 0;

  Symbols = 0;
  NextGlobal = 0;
  NextFunction = 0x100;
  CurrentFunc = 0;
  NextLocal = 0;

  OpParaCount = 0;
  OpStatementCount = 0;
  OpFunctionCount = 0;

  ErrorText[0] = 0;
  ErrorFile[0] = 0;
  ErrorLine = 0;
  IsError = 0;
  Line = 1;
  Text = 0;
  TextStart = 0;
  File = 0;
  ErrorBuf[0] = 0;

  AddToken(TOK_OPEN    ,"(");
  AddToken(TOK_CLOSE   ,")");
  AddToken(TOK_COPEN   ,"{");
  AddToken(TOK_CCLOSE  ,"}");
  AddToken(TOK_SOPEN   ,"[");
  AddToken(TOK_SCLOSE  ,"]");
  AddToken(TOK_DOTDOT  ,"..");
  AddToken(TOK_PLUS    ,"+");
  AddToken(TOK_MINUS   ,"-");
  AddToken(TOK_STAR    ,"*");
  AddToken(TOK_SLASH   ,"/");
  AddToken(TOK_PERCENT ,"%");
  AddToken(TOK_DOT     ,".");
  AddToken(TOK_COLON   ,":");
  AddToken(TOK_SEMI    ,";");
  AddToken(TOK_QUESTION,"?");
  AddToken(TOK_COMMA   ,",");

  AddToken(TOK_GE      ,">=");
  AddToken(TOK_LE      ,"<=");
  AddToken(TOK_GT      ,">");
  AddToken(TOK_LT      ,"<");
  AddToken(TOK_EQ      ,"==");
  AddToken(TOK_NE      ,"!=");
  AddToken(TOK_EQUAL   ,"=");


  AddToken(TOK_CONST   ,"const");
  AddToken(TOK_CLASS   ,"class");
//  AddToken(TOK_OSTACK  ,"ostack");
//  AddToken(TOK_TO      ,"to");
//  AddToken(TOK_VAR     ,"var");
  AddToken(TOK_IF      ,"if");
  AddToken(TOK_ELSE    ,"else");
  AddToken(TOK_WHILE   ,"while");
  AddToken(TOK_DO      ,"do");
  AddToken(TOK_RETURN  ,"return");
  AddToken(TOK_LOAD    ,"load");
  AddToken(TOK_STORE   ,"store");
  AddToken(TOK_DROP    ,"drop");
  AddToken(TOK_AND     ,"&");
  AddToken(TOK_OR      ,"|");

  IntTypes[0] = AddType("void",SCT_VOID,0);
  IntTypes[1] = AddType("int",SCT_INT,1);
  IntTypes[2] = AddType("int2",SCT_INT,2);
  IntTypes[3] = AddType("int3",SCT_INT,3);
  IntTypes[4] = AddType("int4",SCT_INT,4);
  ObjectType = AddType("object",SCT_OBJECT,1);
  StringType = AddType("string",SCT_INT,1);
  FloatTypes[0] = 0;
  FloatTypes[1] = AddType("float",SCT_FLOAT,1);
  FloatTypes[2] = AddType("float2",SCT_FLOAT,2);
  FloatTypes[3] = AddType("float3",SCT_FLOAT,3);
  FloatTypes[4] = AddType("float4",SCT_FLOAT,4);
/*
  AddObject("bitmap",sCID_GENBITMAP);
  AddObject("mesh",sCID_GENMESH);
  AddObject("material",sCID_GENMATERIAL);
  AddObject("scene",sCID_GENSCENE);
  AddObject("matpass",sCID_GENMATPASS);
  AddObject("matrix",sCID_GENMATRIX);
  */
}

/****************************************************************************/

sU32 *ScriptCompiler::Compile(sChar *text,sChar *file)
{
  ErrorText[0] = 0;
  ErrorFile[0] = 0;
  ErrorLine = 0;
  if(Parse(text,file))
  {
    Optimise();
    return Generate();
  }
  else
  {
    return 0;
  }
}


sBool ScriptCompiler::Parse(sChar *text,sChar *file)
{
  Text = text;
  TextStart = text;
  Line = 1;
  File = file;
  IsError = 0;
  CurrentFunc = 0;

  Scan();
//  sDPrintF("COMPILER: PARSE\n");
  _Programm();
  if(!IsError)
  {
//    sDPrintF("COMPILER: NO ERRORS\n");
    return sTRUE;
  }
  else
  {
    sDPrintF("COMPILER: ERRORS!\n");
    return sFALSE;
  }
}

sU32 *ScriptCompiler::Generate()
{
//  sDPrintF("COMPILER: GENERATE\n");

  File = "post phase";
  Line = 0;
  IsError = 0;

  sVERIFY(CodePtr == CodeBuffer);
  PrintParseTrees();
  Emit(DataNext);
  sCopyMem(CodePtr,DataBuffer,DataNext);
  CodePtr += (DataNext+3)/4;
  
  if(!IsError)
  {
//    sDPrintF("COMPILER: NO ERRORS\n");
    return CodeBuffer;
  }
  else
  {
    sDPrintF("COMPILER: ERRORS!\n");
    return 0;
  }
}

/****************************************************************************/

sChar *ScriptCompiler::GetErrorPos()
{
  return ErrorBuf;
}

/****************************************************************************/

void ScriptCompiler::Optimise()
{
  SCFunction *func;

  func = (SCFunction *)Symbols;
  while(func)
  {
    if(func->Kind==SCS_FUNC && func->Block != 0)
      Optimise(func->Block);
    func = (SCFunction *)func->Next;
  }
}

void ScriptCompiler::Optimise(SCStatement *st)
{
  sInt val,vala,valb;
  sInt fold;
  if(st->Left)
    Optimise(st->Left);
  if(st->Right)
    Optimise(st->Right);

  if(st->Left && st->Right && st->Left->Token==TOK_INTLIT && st->Right->Token==TOK_INTLIT && st->Type==IntTypes[1])
  {
    vala = st->Left->Value;
    valb = st->Right->Value;
    fold = sTRUE;
    switch(st->Token)
    {
    case TOK_PLUS:
      val = vala + valb;
      break;
    case TOK_MINUS:
      val = vala - valb;
      break;
    case TOK_STAR:
      val = sMulShift(vala,valb);
      break;
    case TOK_SLASH:
      if(valb==0)
        fold = 0;
      else
        val = sDivShift(vala,valb);
      break;
    case TOK_PERCENT:
      if(valb==0)
        fold = 0;
      else
        val = vala%(valb>>16);
      break;
    case TOK_AND:
      val = vala & valb;
      break;
    case TOK_OR:
      val = vala | valb;
      break;
    default:
      fold = sFALSE;
      break;
    }
    if(fold)
    {
      st->Token = TOK_INTLIT;
      st->Left = 0;
      st->Right = 0;
      st->Value = val;
    }
  }
}

/****************************************************************************/

void ScriptCompiler::PrintParseTrees()
{
  SCFunction *func;
  SCFuncArg *arg;
  SCSymbol *sym;
  SCVar *var;

/*
  SCVar *var;
  var = Globals;
  sDPrintF("\nGlobals\n");
  while(var)
  {
    sDPrintF("  %s (%s @ %d)\n",var->Name,var->Type->Name,var->Index);
    var = var->Next;
  }

  sDPrintF("\nExtern Functions\n");
  func = Functions;
  while(func)
  {
    if(func->Block == 0)
    {
      sDPrintF("  %s %s(",func->Result->Name,func->Name);
      arg =func->Args;
      while(arg)
      {
        if(arg!=func->Args) sDPrintF(",");
        sDPrintF("%s %s",arg->Type->Name,arg->Name);
        arg = arg->Next;
      }
      sDPrintF(") = %d\n",func->Index);
    }
    func = func->Next;
  }
*/

  OpParaCount = 0;
  OpStatementCount = 0;
  OpFunctionCount = 0;

  if(OpParas)
    sSetMem(OpParas,0,sizeof(SCOpPara)*SC_MAXOPPARA);
  if(OpStatements)
    sSetMem(OpStatements,0,sizeof(SCOpStatement)*SC_MAXOPSTATEMENT);
  if(OpFunctions)
    sSetMem(OpFunctions,0,sizeof(SCOpFunction)*SC_MAXOPFUNCTION);

#if DEBUGLEVEL&2
  sDPrintF("\nDefined Functions\n");
#endif
  Emit(RTC_NOP);
  func = (SCFunction *)Symbols;
  while(func)
  {
    if(func->Kind==SCS_FUNC && func->Block != 0)
    {
#if DEBUGLEVEL&2
      sDPrintF("  %s %s(",func->Result->Name,func->Name);
#endif
      arg =func->Args;
      while(arg)
      {
#if DEBUGLEVEL&2
        if(arg!=func->Args) sDPrintF(",");
        sDPrintF("%s %s",arg->Type->Name,arg->Name);
#endif
        arg = arg->Next;
      }
#if DEBUGLEVEL&2
      sDPrintF(") = %d\n  {\n    ",func->Index);
#endif
      if(OpFunctions && func->Block && func->Clause)
      {
        OpFunctions[OpFunctionCount].Name = func->Name;
        OpFunctions[OpFunctionCount].StatementStart = OpStatementCount;
        OpFunctions[OpFunctionCount].Source0 = func->Clause->Source0;
        OpFunctions[OpFunctionCount].Source1 = func->Block->Source1;
        OpFunctions[OpFunctionCount].Code0 = CodePtr;
      }

      Emit(RTC_FUNC|func->Index);
      NextLocal = func->MaxLocal;
      PrintArgs(func->Args);
      PrintStatement(func->Block,-1);
      if(NextLocal!=func->MaxLocal)
        Error("temporary locals mismatch");
      Emit(RTC_RTS);

      if(OpFunctions)
      {
        OpFunctions[OpFunctionCount].StatementCount = OpStatementCount-OpFunctions[OpFunctionCount].StatementStart;
        OpFunctions[OpFunctionCount].Code1 = CodePtr;
        OpFunctionCount++;
      }
#if DEBUGLEVEL&2
      sDPrintF("\n  }\n");
#endif
    }
    func = (SCFunction *)func->Next;
  }
  Emit(RTC_EOF); 
#if DEBUGLEVEL&2
  sDPrintF("\n");
#endif

  sym = Symbols;
  while(sym)
  {
    if(sym->Kind==SCS_VAR)
    {
      var = (SCVar *) sym;
      if(var->Scope == SCS_GLOBAL && var->Type->Base==SCT_OBJECT)
      {
        Emit(var->Index);
      }
    }
    sym = sym->Next;
  }
  Emit(~0);

#if DEBUGLEVEL&2
  sDPrintF("\n");
#endif
}

/****************************************************************************/
/****************************************************************************/

sChar *ScriptCompiler::GetClassName(sU32 cid)
{
  SCType *type;

  type = (SCType *) Symbols;
  while(type)
  {
    if(type->Kind==SCS_TYPE && type->Base==SCT_OBJECT && type->Cid==cid)
      return type->Name;
    type = (SCType *) type->Next;
  }

  return "object";
}

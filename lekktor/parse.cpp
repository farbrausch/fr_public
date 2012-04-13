// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "main.hpp"
#include "scan.hpp"
#include "parse.hpp"

/****************************************************************************/
/****************************************************************************/

void ParseBlock();
void ParseCondBlock(sInt *elsemod=0,sBool useelse=sFALSE);
void ParseSwitchBlock();
void ParseExpr();
void ParseType();
void ParseTypeAndName();
void ParseFunctionPtrType();
sBool CheckType();
sBool ACheckType();

/****************************************************************************/
/****************************************************************************/

static sBool intypedef,gottype;
static sInt ModeStack[1024],ModeStackPos=0;

void Match(sInt tok)
{
  if(Token!=tok)
    Error("Match Error");
  Out(Value);
  Scan();
}

void OutBOpen()
{
  NewLine();
  Out("{");
  Indent++;
}

void OutBClose()
{
  Indent--;
  sVERIFY(Indent >= 0);
  NewLine();
  Out("}");
}

void MatchBOpen()
{
  NewLine();
  Indent++;
  Match(TOK_BOPEN);
}

void MatchBClose()
{
  Indent--;
  sVERIFY(Indent >= 0);
  NewLine();
  Match(TOK_BCLOSE);
}

void Match()
{
  Match(Token);
}

/****************************************************************************/

void Pre()
{
  while(Token==TOK_PRE)
  {
    Out("\n");
    Match();
  }
}

void ParseSizeofExpr()
{
  if(CheckType())
    ParseType();
  else
    ParseExpr();
}

sBool IsScopedName()
{
  return(Token==TOK_SCOPE && (Peek()==TOK_NAME || Peek()==TOK_TYPE)
    || Token==TOK_NAME || Token==TOK_TYPE);
}

void ParseScopedName()
{
  if(Token==TOK_SCOPE && (Peek()==TOK_NAME || Peek()==TOK_TYPE))
  {
    Match();
    Match();
  }
  else if(Token==TOK_NAME || Token==TOK_TYPE)
    Match();

  while(Token==TOK_SCOPE)
  {
    Match();
    if(Token==TOK_NAME || Token==TOK_TYPE)
      Match();
    else
      Error("scoped name error");
  }
}

void ParseUnary()
{
  while(Token==TOK_SUB || Token==TOK_NOT || Token==TOK_NEG || 
        Token==TOK_MUL || Token==TOK_AND ||
        Token==TOK_INC || Token==TOK_DEC)
  {
    Match();
    Out(" ");
  }

  if(IsScopedName())
    ParseScopedName();
  else
  {
    switch(Token)
    {
    case TOK_VALUE:
      Match();
      break;
    case TOK_OPEN:      
      Match();
      if(CheckType())       // cast
      {
        ParseFunctionPtrType();
        //ParseType();
        Match(TOK_CLOSE);
        ParseExpr();
      }
      else                      // subexpression
      {
        ParseExpr();
        Match(TOK_CLOSE);
      }
      break;
    case TOK_SIZEOF:
      Match();
      if(Token==TOK_OPEN)
      {
        Match(TOK_OPEN);
        ParseSizeofExpr();
        Match(TOK_CLOSE);
      }
      else
      {
        Out("(");
        ParseSizeofExpr();
        Out(")");
      }
      break;
    case TOK_NEW:
      Match();
      Out(" ");
      ParseType();
      break;
    default:
      Error("expression");
      break;
    }
  }
/*
  if(Token==TOK_LT)
  {
    ScanSafe();
    Scan();
    if(CheckType())
    {
      Out("<");
      ParseType();
      Match(TOK_GT);
    }
    else
    {
      ScanRestore();
    }
  }
*/
  if(Token==TOK_LT && ACheckType())
  {
    Match(TOK_LT);
    ParseType();
    Match(TOK_GT);
  }
  while(Token==TOK_INC || Token==TOK_DEC || Token==TOK_DOT || Token==TOK_OPEN || Token==TOK_SOPEN || (Token==TOK_LT && Peek()==TOK_TYPE))
  {
    switch(Token)
    {
    case TOK_LT:
      Match(TOK_LT);
      Match(TOK_TYPE);
      Match(TOK_GT);
      break;
    case TOK_INC:
    case TOK_DEC:
      Match();
      Out(" ");
      break;
    case TOK_DOT:
      Match(TOK_DOT);
      Match(TOK_NAME);
      break;
    case TOK_OPEN:
      Match();
      if(Token!=TOK_CLOSE)
      {
        ParseExpr();
        while(Token!=TOK_CLOSE)
        {
          Match(TOK_COMMA);
          ParseExpr();
        }
      }
      Match();
      break;
    case TOK_SOPEN:
      Match(TOK_SOPEN);
      ParseExpr();
      Match(TOK_SCLOSE);
      break;
    }
  }
}

void ParseExprB()
{
loop:
  ParseUnary();

  if(Token==TOK_ADD || Token==TOK_SUB || Token==TOK_MUL || Token==TOK_DIV || Token==TOK_MOD ||
     Token==TOK_GE || Token==TOK_GT || Token==TOK_LE || Token==TOK_LT || Token==TOK_EQ || Token==TOK_NE ||
     Token==TOK_AND || Token==TOK_OR || Token==TOK_EOR || Token==TOK_ANDAND || Token==TOK_OROR || Token==TOK_LEFT || Token==TOK_RIGHT || 
     Token==TOK_ASSADD || Token==TOK_ASSSUB || Token==TOK_ASSMUL || Token==TOK_ASSDIV || Token==TOK_ASSMOD ||
     Token==TOK_ASSAND || Token==TOK_ASSOR || Token==TOK_ASSEOR || Token==TOK_ASSLEFT || Token==TOK_ASSRIGHT ||
     Token==TOK_PTR || Token==TOK_ASSIGN)
  {
    Match();
    goto loop;
  }
}

void ParseExprA()
{
loop:
  ParseUnary();

  if(Token==TOK_ADD || Token==TOK_SUB || Token==TOK_MUL || Token==TOK_DIV || Token==TOK_MOD ||
     Token==TOK_GE || Token==TOK_GT || Token==TOK_LE || Token==TOK_LT || Token==TOK_EQ || Token==TOK_NE ||
     Token==TOK_AND || Token==TOK_OR || Token==TOK_EOR || Token==TOK_ANDAND || Token==TOK_OROR || Token==TOK_LEFT || Token==TOK_RIGHT || 
     Token==TOK_ASSADD || Token==TOK_ASSSUB || Token==TOK_ASSMUL || Token==TOK_ASSDIV || Token==TOK_ASSMOD ||
     Token==TOK_ASSAND || Token==TOK_ASSOR || Token==TOK_ASSEOR || Token==TOK_ASSLEFT || Token==TOK_ASSRIGHT ||
     Token==TOK_PTR || Token==TOK_ASSIGN/* || Token==TOK_COMMA*/)
  {
    Match();
    goto loop;
  }
}

void ParseExpr()
{
  ParseExprA();
  if(Token==TOK_COND)
  {
    Match();
    ParseExprA();
    Match(TOK_COLON);
    ParseExpr();
  }
}

void ParseAssignExpr()
{
  ParseExpr();
  while(Token==TOK_COMMA)
  {
    Match();
    ParseExpr();
  }
}


// variable declaration: var
//
// bla
// bla = blub
// *bla
// 

void ParseInitialiser()
{
  if(Token!=TOK_BCLOSE)
  {
    if(Token==TOK_BOPEN)
    {
      MatchBOpen();
      ParseInitialiser();
      while(Token==TOK_COMMA)
      {
        Match(TOK_COMMA);
        ParseInitialiser();
      }
      MatchBClose();
    }
    else
    {
      ParseExpr();
    }
  }
}

void ParseVarDecl()
{
  while(Token==TOK_MUL || Token==TOK_CONST || Token==TOK_VOLATILE)
  {
    Match();
    Out(" ");
  }
  if(Token==TOK_STDCALL || Token==TOK_CDECL || Token==TOK_FASTCALL || Token==TOK_WINAPI || Token==TOK_APIENTRY || Token==TOK_CALLBACK)
  {
    Match();
    Out(" ");
  }
  while(Token==TOK_MUL || Token==TOK_CONST || Token==TOK_VOLATILE)
  {
    Match();
    Out(" ");
  }
  /*
  while(Token==TOK_TYPE)
  {
    Match(TOK_TYPE);
    Match(TOK_SCOPE);
  }
  */
  if(Token==TOK_NAME)     // sometimes, we have no name!
  {
    if(intypedef)
    {
      AddType(Value);
      gottype=sTRUE;
    }
    Match(TOK_NAME);
  }
  else if(Token==TOK_OPERATOR) // operator overload
  {
    Match();
    Out(" ");
    if(Token != TOK_SOPEN)
      Match();
    else
    {
      Match();
      Match(TOK_SCLOSE);
    }
  }
  while(Token==TOK_SOPEN)
  {
    Match();
    if(Token!=TOK_SCLOSE)
      ParseExpr();
    Match(TOK_SCLOSE);
  }
  if(Token==TOK_ASSIGN)
  {
    Match(TOK_ASSIGN);
    ParseInitialiser();
  }
}

void ParseParamList()
{
  if(Token!=TOK_CLOSE)
  {
    ParseTypeAndName();
    while(Token==TOK_COMMA)
    {
      Match(TOK_COMMA);
      if(Token==TOK_ELIPSIS)
      {
        Match();
        break;
      }
      else
      {
        ParseTypeAndName();
      }
    }
  }
}

// variable declaration: type name

sBool CheckType1(sInt i)
{
  return (i==TOK_TYPE||i==TOK_CONST||i==TOK_VOLATILE||
         i==TOK_STATIC||i==TOK_EXTERN||i==TOK_AUTO||i==TOK_REGISTER||
         i==TOK_INT||i==TOK_FLOAT||i==TOK_LONG||i==TOK_SHORT||i==TOK_CHAR||
         i==TOK_SIGNED||i==TOK_UNSIGNED||
         i==TOK_STRUCT||i==TOK_CLASS||i==TOK_UNION||i==TOK_ENUM||
         i==TOK_INLINE||i==TOK_FINLINE);
}

sBool CheckType()
{
  return CheckType1(Token);
}
sBool ACheckType()
{
  return CheckType1(AToken);
}

void ParseType1(sInt withname)
{
  while(Token==TOK_STATIC||Token==TOK_EXTERN||Token==TOK_AUTO||Token==TOK_REGISTER)
  {
    if(Token!=TOK_EXTERN)
    {
      Match();
      Out(" ");
    }
    else
    {
      Match();
      Out(" ");
      if(Token==TOK_VALUE)
      {
        Match();
        Out(" ");
        if(Token==TOK_BOPEN)
          ParseBlock();
      }
    }
  }
  if(Token==TOK_CONST||Token==TOK_VOLATILE)
  {
    Match();
    Out(" ");
  }

  if(Token==TOK_STRUCT||Token==TOK_CLASS||Token==TOK_UNION)
  {
    Match();
    Out(" ");
    if(Token==TOK_NAME)
    {
      AddType(Value);
      Match(TOK_NAME);
    }
    else
    {
      Match(TOK_TYPE);
    }
    if(Token==TOK_COLON)
    {
      Match();
      if(Token==TOK_PROTECTED||Token==TOK_PRIVATE||Token==TOK_PUBLIC)
      {
        Match();
        Out(" ");
      }
      /*else
        Match(TOK_PUBLIC); // provoke error*/
      Match(TOK_TYPE);
    }
    if(Token==TOK_BOPEN)
    {
      MatchBOpen();
      while(Token!=TOK_BCLOSE)
      {
        switch(Token)
        {
        case TOK_PROTECTED:
        case TOK_PRIVATE:
        case TOK_PUBLIC:
          Out("\n");
          Match();
          Match(TOK_COLON);
          break;
        case TOK_PRE:
          Out("\n");
          Match();
          break;
        default:
          NewLine();
          ParseDecl();
          break;
        }
      }
      MatchBClose();
    }
  }
  else if(Token==TOK_ENUM)
  {
    Match();
    Out(" ");
    if(Token==TOK_NAME)
    {
      AddType(Value);
      Match(TOK_NAME);
    }
    else
    {
      Match(TOK_TYPE);
    }
    if(Token==TOK_BOPEN)
    {
      MatchBOpen();
      ParseInitialiser();
      while(Token==TOK_COMMA)
      {
        Match(TOK_COMMA);
        ParseInitialiser();
      }
      MatchBClose();
    }
  }
  else
  {
    sInt gotspec=0;
    while(Token==TOK_LONG || Token==TOK_SHORT || Token==TOK_SIGNED || Token==TOK_UNSIGNED)
    {
      Match();
      Out(" ");
      gotspec=1;
    }
    if(Token==TOK_INT||Token==TOK_FLOAT||Token==TOK_CHAR||Token==TOK_FLOAT||Token==TOK_DOUBLE||Token==TOK_TYPE)
    {
      Match();
    }
    else if(!gotspec)
      Error("type error");
  }

  if(withname)
    Out(" ");

  while(Token==TOK_MUL)
    Match();
  if(Token==TOK_AND)
    Match();

  if(withname==1)
  {
    ParseVarDecl();
  }

  while(Token==TOK_SOPEN)
  {
    Match();
    if(Token!=TOK_SCLOSE)
      ParseExpr();
    Match(TOK_SCLOSE);
  }

  if(withname==2 && Token==TOK_OPEN)
  {
    Match();
    ParseVarDecl();
    Match(TOK_CLOSE);
    Match(TOK_OPEN);
    ParseParamList();
    Match(TOK_CLOSE);
  }
}

void ParseFunctionPtrType()
{
  ParseType1(2);
}

void ParseTypeAndName()
{
  ParseType1(1);
}

void ParseType()
{
  ParseType1(0);
}

void ParseInitList()
{
  while(1)
  {
    if(Token==TOK_NAME || Token==TOK_TYPE)
    {
      Match();
      Match(TOK_OPEN);
      ParseExpr();
      Match(TOK_CLOSE);
      if(Token==TOK_COMMA)
        Match();
    }
    else if(Token==TOK_BOPEN)
      break;
  }
}

// variable declaration
// 
// type var;
// type var,var;
// type var(type var,type var);
// type var(type var,type var) { statements } 
// struct bla

void ParseDecl()
{
  if(Token==TOK_INLINE || Token==TOK_FINLINE)
  {
    Match();
    Out(" ");
  }
  ParseType();
/*
  if(Token==TOK_SCOPE)    // must be constrcutor...
  {
    Match();
    if(Token==TOK_NEG)
      Match();
    Match(TOK_TYPE);
  }
  else*/
  {
    Out(" ");
    ParseVarDecl();
  }
    
  if(Token==TOK_OPEN)
  {
    Match(TOK_OPEN);
    ParseParamList();
    Match(TOK_CLOSE);
    if(Token==TOK_CONST)
    {
      Match();
      Out(" ");
    }
    if(Token==TOK_BOPEN)
    {
      //ParseBlock();
      ParseCondBlock();
      NewLine();
    }
    else if(Token==TOK_ASSIGN)
    {
      Match(TOK_ASSIGN);
      Match(TOK_VALUE);
    }
    else if(Token==TOK_COLON)
    {
      Match();
      ParseInitList();
    }
    else
    {          
      Match(TOK_SEMI);
    }
  }
  else
  {
    while(Token==TOK_COMMA)
    {
      Match(TOK_COMMA);
      ParseVarDecl();
    }
    Match(TOK_SEMI);
  }
}

/****************************************************************************/

void ParseStatement()
{
  sInt elsemod;

  if(CheckType())
  {
    NewLine();
    ParseDecl();
    return;
  }
  switch(Token)
  {
  case TOK_PRE:
    Out("\n");
    Match();
    break;
  case TOK_WHILE:
    NewLine();
    Match();
    Match(TOK_OPEN);
    ParseAssignExpr();
    Match(TOK_CLOSE);
    ParseCondBlock();
    //ParseStatement();
    break;

  case TOK_SWITCH:
    NewLine();
    Match();
    Match(TOK_OPEN);
    ParseAssignExpr();
    Match(TOK_CLOSE);
    ParseSwitchBlock();
    break;
    /*
  case TOK_CASE:
    NewLine(-1);
    Match();
    Out(" ");
    if(Token==TOK_VALUE)
    {
      Match();
      Match(TOK_COLON);
    }
    else if(Token==TOK_LABEL)
    {
      Match();
    }
    else
    {
      Error("case label");
    }
    break;
    */

  case TOK_BREAK:
    NewLine();
    Match();
    Match(TOK_SEMI);
    break;
  case TOK_RETURN:
    NewLine();
    Match();
    Out(" ");
    if(Token!=TOK_SEMI)
      ParseAssignExpr();
    Match(TOK_SEMI);
    break;
  case TOK_IF:
    NewLine();
    Match(TOK_IF);
    Match(TOK_OPEN);
    ParseAssignExpr();
    Match(TOK_CLOSE);
    ParseCondBlock(&elsemod,sFALSE);
    if(Token==TOK_ELSE)
    {
      NewLine();
      Match(TOK_ELSE);
      ParseCondBlock(&elsemod,sTRUE);
    }
    else
    {
      if(Mode == 1)
      {
        NewLine();
        OutF("else");
        OutBOpen();
        NewLine();
        OutF("sLekktor_%s.Set(%d);",LekktorName,elsemod);
        OutBClose();
      }
    }
    break;

  case TOK_ASM:
    NewLine();
    Match(TOK_ASM);
    InlineAssembly();
    if(Token==TOK_SEMI)
      Match();
    break;

  case TOK_FOR:
    NewLine();
    Match(TOK_FOR);
    Match(TOK_OPEN);
    if(Token!=TOK_SEMI)
    {
      if(CheckType())
      {
        ParseDecl();
      }
      else
      {
        ParseAssignExpr();
        Match(TOK_SEMI);
      }
    }
    else
      Match();
    if(Token!=TOK_SEMI)
      ParseAssignExpr();
    Match(TOK_SEMI);
    if(Token!=TOK_CLOSE)
      ParseAssignExpr();
    Match(TOK_CLOSE);
    ParseCondBlock();
    break;
  case TOK_DO:
    NewLine();
    Match(TOK_DO);
    ParseCondBlock();
    NewLine();
    Match(TOK_WHILE);
    Match(TOK_OPEN);
    ParseAssignExpr();
    Match(TOK_CLOSE);
    Match(TOK_SEMI);
    break;
  case TOK_BOPEN:
    ParseBlock();
    break;
  case TOK_DELETEA:
  case TOK_DELETE:
    NewLine();
    Match();
    Out(" ");
    ParseExpr();
    Match(TOK_SEMI);
    break;
  case TOK_GOTO:
    NewLine();
    Match(TOK_GOTO);
    Out(" ");    
    Match(TOK_NAME);
    Match(TOK_SEMI);
    break;
  case TOK_SEMI:
    NewLine();
    Match();
    break;
  case TOK_TYPEDEF:
    NewLine();
    Match();
    Out(" ");
    intypedef = sTRUE;
    gottype = sFALSE;
    ParseFunctionPtrType();
    intypedef = sFALSE;
    Out(" ");
    if(Token == TOK_NAME)
    {
      AddType(Value);
      Match();
    }
    else if(!gottype)
      Error("typedef error");
    Match(TOK_SEMI);
    break;
  case TOK_NAMESPACE:
    NewLine();
    Match();
    Out(" ");
    if(Token==TOK_NAME) // name may be omitted
      Match(TOK_NAME);
    ParseBlock();
    break;
  case TOK_USING:
    NewLine();
    Match();
    Out(" ");
    if(Token==TOK_NAMESPACE)
    {
      Match();
      Out(" ");
    }
    ParseScopedName();
    Match(TOK_SEMI);
    break;
  case TOK_VCTRY:
    NewLine();
    Match();
    ParseStatement();
    if(Token == TOK_VCEXCEPT)
    {
      NewLine();
      Match();
      Match(TOK_OPEN);
      ParseExpr();
      Match(TOK_CLOSE);
      ParseStatement();
    }
    break;
  case TOK_NAME:
    if(Peek() == TOK_COLON)
    {
      NewLine();
      Match();
      Match();
      break;
    }
    // fall-through
  /*case TOK_LABEL:
    NewLine();
    Match();
    break;*/
  default:
    NewLine();
    ParseAssignExpr();
    Match(TOK_SEMI);
    break;
  }
}

void ParseBlock()
{
  MatchBOpen();
  while(Token!=TOK_BCLOSE)
  {
    ParseStatement();
  }
  MatchBClose();
}

void BeginIf(sInt mod)
{
  if(Mode==1)
  {
    NewLine();
    OutF("sLekktor_%s.Set(%d);",LekktorName,mod);
  }

  if(Mode==2 && ModArray[mod]==0)
  {
    NewLine();
    Out("if(0)");
    OutBOpen();
  }

  ModeStack[ModeStackPos++] = Mode;
}

void EndIf(sInt mod)
{
  sInt smode = ModeStack[--ModeStackPos];

  if(mod>=0 && smode==2 && ModArray[mod]==0)
    OutBClose();
}

void BeginIfX(sInt mod)
{
  if(Mode==1)
  {
    NewLine();
    OutF("sLekktor_%s.Set(%d);",LekktorName,mod);
  }

  if(Mode==2)
  {
    if(ModArray[mod]==0)
    {
      NewLine();
      Out("if(0)");
    }
  }
  OutBOpen();
}

void ParseCondBlock(sInt *elsemod,sBool useelse)
{
  sInt mod;
  sBool kill,always;

  kill = always = sFALSE;

  if(!useelse)
  {
    mod = Modify++;
    if(elsemod)
      *elsemod = Modify++;
  }
  else
    mod = *elsemod;

  if(elsemod && Mode==2)
  {
    if(!useelse)
    {
      kill = !ModArray[*elsemod - 1];
      always = !ModArray[*elsemod];
    }
    else
    {
      kill = !ModArray[*elsemod];
      always = !ModArray[*elsemod - 1];
    }
  }

  Pre();
  if(kill || always)
  {
    OutBOpen();
    OutBClose();
    NewLine();
    if(kill)
      OutF("if(0)");
    else
      OutF("if(1)");
  }

  if(Token==TOK_BOPEN)
  {
    MatchBOpen();
    sInt ind = Indent;
    BeginIf(mod);
    sInt ind2 = Indent;

    while(Token!=TOK_BCLOSE)
    {
      ParseStatement();
    }

    sVERIFY(Indent == ind2);
    EndIf(mod);
    sVERIFY(Indent == ind);
    MatchBClose();
  }
  else
  {
    OutBOpen();
    BeginIf(mod);

    ParseStatement();

    EndIf(mod);
    OutBClose();
  }
}

void ParseSwitchBlock()
{
  sInt mod;

  MatchBOpen();
  OutBOpen();
  mod = -1;
  while(Token!=TOK_BCLOSE)
  {
    switch(Token)
    {
    case TOK_CASE:
      OutBClose();

      NewLine();
      Match();
      Out(" ");

      ParseExpr();
      Match(TOK_COLON);

      /*if(Token==TOK_LABEL)
      {
        Match();
      }
      else if(Token)
      {
        ParseExpr();
        Match(TOK_COLON);
      }*/

      mod = Modify++;
      BeginIfX(mod);
      break;

    case TOK_DEFAULT:
      OutBClose();
    
      NewLine();
      Match();
      Match(TOK_COLON);

      mod = Modify++;
      BeginIfX(mod);
      break;                   

    case TOK_BREAK:
      OutBClose();

      NewLine();
      Match();
      Match(TOK_SEMI);

      mod = -1;
      OutBOpen();
      break;

    default:
      ParseStatement();
      break;
    }
  }
  OutBClose();
  MatchBClose();
}

/****************************************************************************/

void Parse()
{
  Scan();

  Indent = 0;
  while(Token!=TOK_EOF)
  {
    ParseStatement();
  }
}

/****************************************************************************/
/****************************************************************************/

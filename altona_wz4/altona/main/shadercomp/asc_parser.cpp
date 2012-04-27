/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/***                                                                      ***/
/***   all language extensions should be marked with // EXTENSION         ***/
/***                                                                      ***/
/****************************************************************************/

#include "shadercomp/asc_doc.hpp"
#include "base/graphics.hpp"

/****************************************************************************/

void ACDoc::_Global()
{
  ACGlobal *gl;
  ACPermute *perm;
  while(Scan.Errors==0 && Scan.Token!=sTOK_END)
  {
    if(Scan.IfToken(AC_TOK_LINE))
    {
      _Line();
    }
    else if(Scan.IfToken(AC_TOK_USE))             // EXTENSION: use cbuffer & permutes
    {
      sPoolString name;
      Scan.ScanName(name);

      perm = sFind(Permutes,&ACPermute::Name,name);    // use permutes
      if(perm)
      {
        if(UsePermute)
          Scan.Error(L"permute can be used only once");
        else
          UsePermute = perm;
        Scan.Match(';');
      }
      else                                          // use cbuffer
      {
        gl = Program.AddMany(1);
        sClear(*gl);
        gl->SourceLine = Scan.TokenLine;
        gl->SourceFile = Scan.TokenFile;
        gl->Kind = ACG_USE;
        gl->Type = FindType(name);
        Scan.Match(';');
        if(!gl->Type)
        {
          Scan.Error(L"type %q not found",name);
        }
        else
        {
          if(gl->Type->Type==ACT_CBUFFER)
          {
            // use cbuffer

            Variables.Add(gl->Type->Members);
  //          gl->Type->Members.Clear();
          }
          else
          {
            Scan.Error(L"cbuffer expected in use clause");
          }
        }
      }
    }
    else if(Scan.Token=='[')
    {
      sTextBuffer tb;
      Scan.ScanRaw(tb,'[',']');

      gl = Program.AddMany(1);
      sClear(*gl);
      gl->SourceLine = Scan.TokenLine;
      gl->SourceFile = Scan.TokenFile;
      gl->Kind = ACG_ATTRIBUTE;
      gl->Attribute = tb.Get();
      if(Scan.IfToken(':'))
      {
        Scan.Match(AC_TOK_PIF);
        Scan.Match('(');
        gl->Permute = _Expression();
        Scan.Match(')');
      }
    }
    else if(Scan.IfToken(AC_TOK_PRAGMA))
    {
      gl = Program.AddMany(1);
      sClear(*gl);
      gl->SourceLine = Scan.TokenLine;
      gl->SourceFile = Scan.TokenFile;
      gl->Kind = ACG_PRAGMA;
      Scan.ScanString(gl->Attribute);
      if(Scan.IfToken(':'))
      {
        Scan.Match(AC_TOK_PIF);
        Scan.Match('(');
        gl->Permute = _Expression();
        Scan.Match(')');
      }
      Scan.Match(';');
    }
    else if(Scan.Token==AC_TOK_PERMUTE)
    {
      _Permute();
    }
    else
    {      
      ACVar *var = _Decl();
      if(var==0)
      {
        ACType *type = UserTypes[UserTypes.GetCount()-1];
        if(type->Type!=ACT_CBUFFER)
        {
          gl = Program.AddMany(1);
          sClear(*gl);
          gl->SourceLine = Scan.TokenLine;
          gl->SourceFile = Scan.TokenFile;
          gl->Kind = ACG_TYPEDEF;
          gl->Type = type;
        }
        Scan.Match(';');
      }
      else if(var->Function && Scan.Token=='{')
      {
        ACFunc *func = (ACFunc *) var;
        sPoolString dummy;

        Function = func;
        Scope = 2;
        func->FuncBody = _Block();
        Function = 0;
        Scope = 0;

        if(sFind(Functions,&ACFunc::Name,var->Name))
          Scan.Error(L"function %q defined twice",var->Name);
        Functions.AddTail(func);

        gl = Program.AddMany(1);
        sClear(*gl);
        gl->SourceLine = Scan.TokenLine;
        gl->SourceFile = Scan.TokenFile;
        gl->Kind = ACG_FUNC;
        gl->Var = var;
      }
      else
      {
        if(var->Function)
          Scan.Error(L"function without body");
//        var->Usages |= ACF_UNIFORM;

        if(sFind(Variables,&ACVar::Name,var->Name))
          Scan.Error(L"global variable %q defined twice",var->Name);
        Variables.AddTail(var);

        gl = Program.AddMany(1);
        sClear(*gl);
        gl->SourceLine = Scan.TokenLine;
        gl->SourceFile = Scan.TokenFile;
        gl->Kind = ACG_VARIABLE;
        gl->Var = var;

        if(Scan.IfToken('='))
          gl->Init = _Expression();
        Scan.Match(';');
      }
    }
  }
}

void ACDoc::_Line()
{
  sPoolString name;
  sInt line;

  line = Scan.ScanInt();
  Scan.ScanString(name);
  if(Scan.Errors==0)
  {
    Scan.SetFilename(name);
    Scan.SetLine(line+1);
    Scan.TokenFile = name;
    Scan.TokenLine = line+1;
  }
}

ACType *ACDoc::_Struct(sInt typekind)      // 'struct' already scanned
{
  ACType *type = new ACType;
  type->Type = typekind;
  if(Scan.Token==sTOK_NAME)
    Scan.ScanName(type->Name);

  sInt constreg = -1;
  sInt constname = 0;

  while(Scan.IfToken(':'))                     // EXTENSION: auto constant registers
  {
    if(Scan.Token==AC_TOK_REGISTER)
    {
      _Register(constname,constreg);
      if(constname!='c')
        Scan.Error(L"constant register expected, found <%c%d>!",constname,constreg);
      type->CRegStart = constreg;
    }
    else if(Scan.Token==sTOK_NAME && Scan.Name==L"slot")
    {
      Scan.MatchName(L"slot");
      sPoolString shadertype;
      Scan.ScanName(shadertype);

      type->CSlot = Scan.ScanInt();
      if(type->CSlot<0 || type->CSlot>=sCBUFFER_MAXSLOT)
        Scan.Error(L"cbuffer slot out of range (max is %d)",(sInt)sCBUFFER_MAXSLOT);
      if(shadertype==L"vs")
        type->CSlot |= sCBUFFER_VS;
      else if(shadertype==L"hs")
        type->CSlot |= sCBUFFER_HS;
      else if(shadertype==L"ds")
        type->CSlot |= sCBUFFER_DS;
      else if(shadertype==L"gs")
        type->CSlot |= sCBUFFER_GS;
      else if(shadertype==L"ps")
        type->CSlot |= sCBUFFER_PS;
      else if(shadertype==L"cs")
        type->CSlot |= sCBUFFER_CS;
      else 
        Scan.Error(L"unknown shader type %q",shadertype);
    }
    else
    {
      Scan.Error(L"don't understand semantik");
    }
  }

  Scan.Match('{');
  while(!Scan.Errors && Scan.Token!='}')
  {
    if(Scan.Token==AC_TOK_EXTERN)
    {
      type->Externs.AddTail(_Extern());
    }
    else
    {
      ACVar *var = _Decl();
      if(sFind(type->Members,&ACVar::Name,var->Name))
        Scan.Error(L"member %q defined twice",var->Name);
      type->Members.AddTail(var);
      while(Scan.IfToken(','))
      {
        ACVar *var2 = _DeclPost(var->Type,var->Usages);
        if(sFind(type->Members,&ACVar::Name,var2->Name))
          Scan.Error(L"member %q defined twice",var2->Name);
        type->Members.AddTail(var2);
      }

      if(typekind==ACT_CBUFFER && (var->Usages & ~(ACF_ROWMAJOR|ACF_COLMAJOR)) )
        Scan.Error(L"usages not allowed in cbuffer");
      if(typekind==ACT_CBUFFER && !var->Semantic.IsEmpty())
        Scan.Error(L"semantics not allowed in cbuffer");
      if(constname && var->RegisterType)
        Scan.Error(L"automatic register binding and explicit register binding at the same time");
      if(constname)
      {
        var->RegisterType = 'c';
        var->RegisterNum = constreg;
        sInt c,r;
        var->Type->SizeOf(c,r);
        if((var->Usages & ACF_ROWMAJOR) && c>1 && r>1)
        {
          if(var->Type->Type==ACT_ARRAY)
          {
            // transpose before calculating array size
            var->Type->BaseType->SizeOf(c,r);
            sSwap(c,r);
            r = 4;
            if(var->Type->ArrayCount==0)
              Scan.Error(L"could not determine array count");
            c *= var->Type->ArrayCount;
          }
          else
            sSwap(c,r);
        }
        constreg += c;
      }
      Scan.Match(';');
    }
  }
  if(constname)
    type->CRegCount = constreg-type->CRegStart;

  Scan.Match('}');

  return type;
}

ACExtern *ACDoc::_Extern()
{
  ACExtern *ext = new ACExtern;

  Scan.Match(AC_TOK_EXTERN);
  Scan.ScanName(ext->Result);             // this parses the type. very clumsy.
  if(ext->Result == L"sOBSOLETE")
  {
    sPoolString name;
    Scan.ScanName(name);
    ext->Result.Add(ext->Result,L" ");
    ext->Result.Add(ext->Result,name);
  }
  if(Scan.IfToken('*'))
  {
    sString<128> b = ext->Result;
    b.Add(L" *");
    ext->Result = b;
  }
  Scan.ScanName(ext->Name);
  ext->File = Scan.TokenFile;
  ext->ParaLine = Scan.TokenLine;
  Scan.ScanRaw(ext->Para,'(',')');
  ext->BodyLine = Scan.TokenLine;
  Scan.ScanRaw(ext->Body,'{','}');
  return ext;
}

void ACDoc::_Permute()
{
  ACPermute *perm;
  ACPermuteMember *mem;
  Scan.Match(AC_TOK_PERMUTE);
  sPoolString groupname;
  sInt shift;
  sInt num;
  sInt power;
  ACPermute *OldUse = UsePermute;

  perm = new ACPermute;
  UsePermute = perm;
  Scan.ScanName(perm->Name);
  Scan.Match('{');

  shift = 0;

  while(!Scan.Errors && Scan.Token!='}')
  {
    if(Scan.IfToken(AC_TOK_ASSERT))
    {
      Scan.Match('(');
      perm->Asserts.AddTail(_Expression());
      Scan.Match(')');
      Scan.Match(';');
    }
    else
    {
      num = 0;
      Scan.ScanName(groupname);
      if(Scan.IfToken('{'))
      {
        while(!Scan.Errors && Scan.Token!='}')
        {
          mem = perm->Members.AddMany(1);
          mem->Group = 0;
          Scan.ScanName(mem->Name);
          mem->Value = num++;
          mem->Shift = shift;
          mem->Mask = 0;
          if(Scan.Token!='}')
            Scan.Match(',');
        }
        Scan.Match('}');
      }
      else
      {
        num = 2;
      }
      Scan.Match(';');
      mem = perm->Members.AddMany(1);
      mem->Group = 1;
      mem->Name = groupname;
      mem->Value = 0;
      mem->Max = num;
      mem->Shift = shift;
      power = sFindHigherPower(num-1)+1;
      mem->Mask = (1<<power)-1;
      shift += power;
    }
  }

  Scan.Match('}');
  Scan.Match(';');

  perm->MaxShift = shift;

  if(sFind(Permutes,&ACPermute::Name,perm->Name))
    Scan.Error(L"permutation %q defined twice",perm->Name);
  Permutes.AddTail(perm);
  UsePermute = OldUse;
}

sBool ACDoc::_IsDecl()
{
  if(Scan.Token==sTOK_NAME)
  {
    if(FindType(Scan.Name)) return 1;
  }
  if(Scan.Token==AC_TOK_IN) return 1;
  if(Scan.Token==AC_TOK_OUT) return 1;
  if(Scan.Token==AC_TOK_INOUT) return 1;
  if(Scan.Token==AC_TOK_UNIFORM) return 1;
  if(Scan.Token==AC_TOK_ROWMAJOR) return 1;
  if(Scan.Token==AC_TOK_COLMAJOR) return 1;
  if(Scan.Token==AC_TOK_CONST) return 1;
  if(Scan.Token==AC_TOK_INLINE) return 1;
  if(Scan.Token==AC_TOK_STATIC) return 1;
  if(Scan.Token==AC_TOK_GROUPSHARED) return 1;

  if(Scan.Token==AC_TOK_STRUCT) return 1;
  return 0;
}

ACVar *ACDoc::_Decl()
{
  sPoolString test;
  ACType *type = 0;
  sInt usages = 0;

// usages and flags

usages_:
  if(Scan.Token==AC_TOK_IN)       { usages |= ACF_IN; Scan.Scan(); goto usages_; }
  if(Scan.Token==AC_TOK_OUT)      { usages |= ACF_OUT; Scan.Scan(); goto usages_; }
  if(Scan.Token==AC_TOK_INOUT)    { usages |= ACF_INOUT; Scan.Scan(); goto usages_; }
  if(Scan.Token==AC_TOK_UNIFORM)  { usages |= ACF_UNIFORM; Scan.Scan(); goto usages_; }
  if(Scan.Token==AC_TOK_ROWMAJOR) { usages |= ACF_ROWMAJOR; Scan.Scan(); goto usages_; }
  if(Scan.Token==AC_TOK_COLMAJOR) { usages |= ACF_COLMAJOR; Scan.Scan(); goto usages_; }
  if(Scan.Token==AC_TOK_CONST)    { usages |= ACF_CONST; Scan.Scan(); goto usages_; }
  if(Scan.Token==AC_TOK_INLINE)   { usages |= ACF_INLINE; Scan.Scan(); goto usages_; }
  if(Scan.Token==AC_TOK_STATIC)   { usages |= ACF_STATIC; Scan.Scan(); goto usages_; }
  if(Scan.Token==AC_TOK_GROUPSHARED) { usages |= ACF_GROUPSHARED; Scan.Scan(); goto usages_; }

  if(Scan.IfName(L"point"))       { usages |= ACF_POINT; goto usages_; }
  if(Scan.IfName(L"line"))        { usages |= ACF_LINE; goto usages_; }
  if(Scan.IfName(L"triangle"))    { usages |= ACF_TRIANGLE; goto usages_; }
  if(Scan.IfName(L"lineadj"))     { usages |= ACF_LINEADJ; goto usages_; }
  if(Scan.IfName(L"triangleadj")) { usages |= ACF_TRIANGLEADJ;  goto usages_; }

// struct

  if(Scan.Token==AC_TOK_STRUCT)
  {
    Scan.Scan();
    type = _Struct(ACT_STRUCT);
    if(sFind(UserTypes,&ACType::Name,type->Name))
      Scan.Error(L"type %q defined twice",type->Name);
    UserTypes.AddTail(type);
    return 0;
  }
  else if(Scan.Token==AC_TOK_CBUFFER)           // EXTENSION
  {
    Scan.Scan();
    ACType *type = _Struct(ACT_CBUFFER);
    if(sFind(UserTypes,&ACType::Name,type->Name))
      Scan.Error(L"type %q defined twice",type->Name);
    UserTypes.AddTail(type);
    return 0;
  }
  else // type
  {
    Scan.ScanName(test);
    type = FindType(test);
    if(!type)
      Scan.Error(L"unknown type %q",test);
    if(Scan.IfToken('<'))
    {
      sPoolString name;
      Scan.ScanName(name);      
      type->Template = FindType(name);
      if(type->Template == 0 && !Scan.Errors)
        Scan.Error(L"unknown type %q in template parameter",name);
      if(Scan.IfToken(','))
        type->TemplateCount = Scan.ScanInt();
      Scan.Match('>');
    }
  }

// name and restdone

  return _DeclPost(type,usages);
}

ACVar *ACDoc::_DeclPost(ACType *type,sInt usages)
{
  ACVar *var;
  sPoolString test;
  sPoolString name;

// name

  Scan.ScanName(name);

// array

  while(Scan.IfToken('['))
  {
    ACType *t2 = new ACType;
    t2->BaseType = type;
    t2->ArrayCountExpr = _Expression();
    if(t2->ArrayCountExpr->Op == ACE_LITERAL && t2->ArrayCountExpr->Literal->Token==sTOK_INT)
      t2->ArrayCount = t2->ArrayCountExpr->Literal->ValueI;
    t2->Type = ACT_ARRAY;
    UserTypes.AddTail(t2);
    type = t2;
    Scan.Match(']');
  }

// func
    
  if(Scan.IfToken('('))
  {
    ACFunc *func = new ACFunc;
    var = func;
    sBool first = 1;
    while(Scan.Errors==0 && Scan.Token!=')')
    {
      if(!first)
        Scan.Match(',');
      first = 0;
      if(Scan.Token==')')                     // EXTENSION (allow trailing , in parameter list)
        break;

      ACVar *member = _Decl();
      if(member)
      {
        if(sFind(func->Locals[SCOPE_PARA],&ACVar::Name,member->Name))
          Scan.Error(L"parameter %q defined twice",member->Name);
        func->Locals[SCOPE_PARA].AddTail(member);
      }
      else
        Scan.Error(L"nested type declarations not supported");
    }
    Scan.Match(')');
  }
  else
  {
    var = new ACVar;
    AllVariables.AddTail(var);
  }

// semantics

  while(Scan.IfToken(':'))
  {
    if(Scan.Token==AC_TOK_REGISTER)
    {
      if(var->RegisterType)
        Scan.Error(L"more than one register binding for %q",name);
      _Register(var->RegisterType,var->RegisterNum);
    }
    else if(Scan.Token==AC_TOK_PIF)           // EXTENSION (permutation dependend declarations)
    {
      Scan.Match(AC_TOK_PIF);
      Scan.Match('(');
      var->Permute = _Expression();
      Scan.Match(')');
    }
    else
    {
      if(!var->Semantic.IsEmpty())
        Scan.Error(L"more than one semantic for %q",name);
      Scan.ScanName(var->Semantic);
    }
  }

  var->Name = name;
  var->Type = type;
  var->Usages = usages;
  return var;
}

void ACDoc::_Register(sInt &regtype,sInt &regnum)
{
  sPoolString test;
    
  Scan.Match(AC_TOK_REGISTER);
  Scan.Match('(');
  Scan.ScanName(test);
  Scan.Match(')');
  const sChar *reg = test;
  if(*reg!='c' && *reg!='s' && *reg!='b' && *reg!='t' && *reg!='u')
  {
    Scan.Error(L"unknown register %q",test);
  }
  else
  {
    regtype = *reg++;
    regnum = 0;
    while(*reg)
    {
      if((*reg)>='0' && (*reg)<='9')
      {
        regnum = regnum*10 + (*reg)-'0';
      }
      else
      {
        Scan.Error(L"unknown register %q",test);
        break;
      }
      reg++;
    }
  }
}

/****************************************************************************/

ACStatement *ACDoc::_Block()
{
  ACStatement *first;
  sInt oldscope = Scope;

  if(Scope<SCOPE_MAX)
    Scope++;
  else
    Scan.Error(L"scopes nested too deep, current max is %d",SCOPE_MAX);

  if(Scan.IfToken('{'))
  {
    if(Scan.Token!='}')
    {
      first = _Statement();
      ACStatement *last = first;
      while(last->Next)
        last = last->Next;
      while(Scan.Errors==0 && Scan.Token!='}')
      {
        last->Next = _Statement();
        while(last->Next)
          last = last->Next;
      }
    }
    else
    {
      first = NewStat();
      first->Op = ACS_EMPTY;
    }
    Scan.Match('}');
  }
  else
  {
    first = _Statement();
  }

  if(Function)
    Function->Locals[Scope].Clear();
  Scope = oldscope;
  return first;
}

ACStatement *ACDoc::NewStat()
{
  ACStatement *stat = Pool->Alloc<ACStatement>();
  sClear(*stat);
  stat->SourceFile = Scan.TokenFile;
  stat->SourceLine = Scan.TokenLine;
  return stat;
}

ACStatement *ACDoc::_Statement()
{

  ACStatement *stat = NewStat();

  if(Scan.IfToken(';'))
  {
    stat->Op = ACS_EMPTY;
  }
  else if(Scan.Token == '{')
  {
    stat->Op = ACS_BLOCK;
    stat->Body = _Block();
  }
  else if(Scan.IfToken(AC_TOK_RETURN))
  {
    stat->Op = ACS_RETURN;
    if(Scan.Token!=';')
      stat->Expr = _Expression();
    Scan.Match(';');
  }
  else if(Scan.IfToken(AC_TOK_BREAK))
  {
    stat->Op = ACS_BREAK;
    Scan.Match(';');
  }
  else if(Scan.IfToken(AC_TOK_CONTINUE))
  {
    stat->Op = ACS_CONTINUE;
    Scan.Match(';');
  }
  else if(Scan.IfToken(AC_TOK_ASM))
  {
    sTextBuffer tb;
    Scan.ScanRaw(tb,'{','}');
    Scan.Match(';');

    stat->Op = ACS_ASM;
    stat->Expr = NewExpr(ACS_ASM,0,0);
    stat->Expr->Literal = Pool->Alloc<ACLiteral>();
    sClear(*stat->Expr->Literal);
    stat->Expr->Literal->Token = AC_TOK_ASM;
    stat->Expr->Literal->Value = tb.Get();
  }
  else if(Scan.Token=='[')
  {
    sTextBuffer tb;
    Scan.ScanRaw(tb,'[',']');

    stat->Op = ACS_ATTRIBUTE;
    stat->Expr = NewExpr(ACS_ATTRIBUTE,0,0);
    stat->Expr->Literal = Pool->Alloc<ACLiteral>();
    sClear(*stat->Expr->Literal);
    stat->Expr->Literal->Token = '[';
    stat->Expr->Literal->Value = tb.Get();
  }
  else if(Scan.IfToken(AC_TOK_IF))
  {
    stat->Op = ACS_IF;
    Scan.Match('(');
    stat->Cond = _Expression();
    Scan.Match(')');
    stat->Body = _Block();
    if(Scan.IfToken(AC_TOK_ELSE))
      stat->Else = _Statement();
  }
  else if(Scan.IfToken(AC_TOK_PIF))
  {
    stat->Op = ACS_PIF;
    Scan.Match('(');
    stat->Cond = _Expression();
    Scan.Match(')');
    stat->Body = _Block();
    if(Scan.IfToken(AC_TOK_PELSE))
      stat->Else = _Statement();
  }
  else if(Scan.IfToken(AC_TOK_WHILE))
  {
    stat->Op = ACS_WHILE;
    Scan.Match('(');
    stat->Cond = _Expression();
    Scan.Match(')');
    stat->Body = _Block();
  }
  else if(Scan.IfToken(AC_TOK_DO))
  {
    stat->Op = ACS_DO;
    stat->Body = _Block();
    Scan.Match(AC_TOK_WHILE);
    Scan.Match('(');
    stat->Cond = _Expression();
    Scan.Match(')');
    Scan.Match(';');
  }
  else if(Scan.IfToken(AC_TOK_FOR))
  {
    stat->Op = ACS_FOR;
    Scan.Match('(');
    stat->Else = _Statement();
    if(!(stat->Else->Op==ACS_VARDECL || stat->Else->Op==ACS_EXPRESSION))
      Scan.Error(L"first for argument must be a variable declaration or an expression");
    stat->Cond = _Expression();
    Scan.Match(';');
    stat->Expr = _Expression();
    Scan.Match(')');
    stat->Body = _Block();
  }
  else if(_IsDecl())
  {
    stat->Op = ACS_VARDECL;
    stat->Var = _Decl();
    if(stat->Var)
    {
      if(Scan.IfToken('='))
      {
        stat->Expr = _Expression();
      }
      if(sFind(Function->Locals[Scope],&ACVar::Name,stat->Var->Name))
        Scan.Error(L"local %q defined twice",stat->Var->Name);
      Function->Locals[Scope].AddTail(stat->Var);
      ACStatement **patch = &stat->Next;
      while(Scan.IfToken(','))
      {
        ACStatement *next = NewStat();

        next->Op = ACS_VARDECL;
        next->Var = _DeclPost(stat->Var->Type,stat->Var->Usages);
        
        if(sFind(Function->Locals[Scope],&ACVar::Name,next->Var->Name))
          Scan.Error(L"local %q defined twice",next->Var->Name);
        Function->Locals[Scope].AddTail(next->Var);

        *patch = next;
        patch = &next->Next;
      }
    }
    else
    {
      Scan.Error(L"nested type declarations not supported");
    }
    Scan.Match(';');
  }
  else
  {
    stat->Op = ACS_EXPRESSION;
    stat->Expr = _Expression();
    Scan.Match(';');
  }

  return stat;
}

/****************************************************************************/

sInt BinaryOp(sInt token,sInt &pri)
{
  switch(token)
  {
  case 0xb0:              pri=1; return ACE_DOT;
  case '*':               pri=1; return ACE_MUL;
  case '/':               pri=1; return ACE_DIV;
  case '%':               pri=1; return ACE_MOD;
  case '+':               pri=2; return ACE_ADD;
  case '-':               pri=2; return ACE_SUB;
  case sTOK_SHIFTL:       pri=3; return ACE_SHIFTL;
  case sTOK_SHIFTR:       pri=3; return ACE_SHIFTR;
  case '>':               pri=4; return ACE_GT;
  case '<':               pri=4; return ACE_LT;
  case sTOK_GE:           pri=4; return ACE_GE;
  case sTOK_LE:           pri=4; return ACE_LE;
  case sTOK_EQ:           pri=5; return ACE_EQ;
  case sTOK_NE:           pri=5; return ACE_NE;
  case '&':               pri=6; return ACE_BAND;
  case '^':               pri=7; return ACE_BXOR;
  case '|':               pri=8; return ACE_BOR;
  case AC_TOK_IMPLIES:    pri=9; return ACE_IMPLIES;
  case AC_TOK_ANDAND:     pri=10; return ACE_AND;
  case AC_TOK_OROR:       pri=11; return ACE_OR;
  case '?':               pri=12; return ACE_COND1;
    /*
  case '=':               pri=9; return ACE_ASSIGN;
  case AC_TOK_ASSIGNADD:  pri=9; return ACE_ASSIGN_ADD;
  case AC_TOK_ASSIGNSUB:  pri=9; return ACE_ASSIGN_SUB;
  case AC_TOK_ASSIGNMUL:  pri=9; return ACE_ASSIGN_MUL;
  case AC_TOK_ASSIGNDIV:  pri=9; return ACE_ASSIGN_DIV;
  */
  }
  return 0;
}

ACExpression *ACDoc::_Expression()
{
  ACExpression *e;
  
  e = _Expression1(12);
  sInt op = 0;
  switch(Scan.Token)
  {
    case '=':               op = ACE_ASSIGN; break;
    case AC_TOK_ASSIGNADD:  op = ACE_ASSIGN_ADD; break;
    case AC_TOK_ASSIGNSUB:  op = ACE_ASSIGN_SUB; break;
    case AC_TOK_ASSIGNMUL:  op = ACE_ASSIGN_MUL; break;
    case AC_TOK_ASSIGNDIV:  op = ACE_ASSIGN_DIV; break;
  }
  if(op)
  {
    Scan.Scan();
    e = NewExpr(op,e,_Expression());
  }
  return e;
}

ACExpression *ACDoc::_Expression1(sInt level)
{
  sInt op,pri;
  ACExpression *a,*b,*c;

  if(level==0)
    return _Value();

  a = _Expression1(level-1);
  for(;;)
  {
    op = BinaryOp(Scan.Token,pri);
    if(!op || pri!=level)
      break;
    Scan.Scan();
    b = _Expression1(level-1);
    if(op==ACE_COND1)
    {
      if(Scan.Token!=':')
        Scan.Error(L"':' missing in conditional operator");
      Scan.Scan();
      c = _Expression1(level-1);
      a = NewExpr(ACE_COND1,a,NewExpr(ACE_COND2,b,c));
    }
    else if(op==ACE_IMPLIES)
    {
      a = NewExpr(ACE_OR,NewExpr(ACE_NOT,a,0),b);
    }
    else
    {
      a = NewExpr(op,a,b);
    }
  }

  return a;
}

ACExpression *ACDoc::_Value()
{
  if(Scan.IfToken('+'))
    return NewExpr(ACE_POSITIVE,_Value(),0);
  if(Scan.IfToken('-'))
    return NewExpr(ACE_NEGATIVE,_Value(),0);
  if(Scan.IfToken('~'))
    return NewExpr(ACE_COMPLEMENT,_Value(),0);
  if(Scan.IfToken('!'))
    return NewExpr(ACE_NOT,_Value(),0);

  ACExpression *e = NewExpr(0,0,0);

  if(Scan.IfToken(AC_TOK_TRUE))
  {
    e->Op = ACE_TRUE;
  }
  else if(Scan.IfToken(AC_TOK_FALSE))
  {
    e->Op = ACE_FALSE;
  }
  else if(Scan.IfName(L"RENDER_DX9"))
  {
    e->Op = ACE_RENDER;
    e->Literal = Pool->Alloc<ACLiteral>();
    sClear(*e->Literal);
    e->Literal->Token = sTOK_INT;
    e->Literal->ValueI = sRENDER_DX9;
    e->Literal->ValueF = e->Literal->ValueI;
  }
  else if(Scan.IfName(L"RENDER_DX11"))
  {
    e->Op = ACE_RENDER;
    e->Literal = Pool->Alloc<ACLiteral>();
    sClear(*e->Literal);
    e->Literal->Token = sTOK_INT;
    e->Literal->ValueI = sRENDER_DX11;
    e->Literal->ValueF = e->Literal->ValueI;
  }
  else if(Scan.IfName(L"RENDER_OGL2"))
  {
    e->Op = ACE_RENDER;
    e->Literal = Pool->Alloc<ACLiteral>();
    sClear(*e->Literal);
    e->Literal->Token = sTOK_INT;
    e->Literal->ValueI = sRENDER_OGL2;
    e->Literal->ValueF = e->Literal->ValueI;
  }
  else if(Scan.Token==sTOK_NAME)
  {
    sPoolString name;
    ACVar *var;
    ACType *type;
    ACPermuteMember *mem;

    Scan.ScanName(name);

    type = FindType(name);
    if(type)
    {
      e->Op = ACE_CONSTRUCT;
      e->CastType = type;
      _ParameterList(&e->Left);
    }
    else
    {
      var = FindFunc(name);
      if(!var)
        var = FindVar(name);
      if(var)
      {
        e->Op = ACE_VAR;
        e->Variable = var;
      }
      else
      {
        mem = 0;
        if(UsePermute)
          mem = sFind(UsePermute->Members,&ACPermuteMember::Name,name);
        if(mem)
        {
          e->Op = ACE_PERMUTE;
          e->Permute = mem;
        }
        else
        {
          Scan.Error(L"unkown symbol %q",name);
        }
      }
    }
  }
  else if(Scan.Token==sTOK_INT)
  {
    e->Op = ACE_LITERAL;
    e->Literal = Pool->Alloc<ACLiteral>();
    sClear(*e->Literal);
    e->Literal->Token = sTOK_INT;
    e->Literal->Value = Scan.ValueString;
    e->Literal->ValueI = Scan.ValI;
    e->Literal->ValueF = e->Literal->ValueI;
    Scan.Scan();
  }
  else if(Scan.Token==sTOK_FLOAT)
  {
    e->Op = ACE_LITERAL;
    e->Literal = Pool->Alloc<ACLiteral>();
    sClear(*e->Literal);
    e->Literal->Token = sTOK_FLOAT;
    e->Literal->Value = Scan.ValueString;
    e->Literal->ValueF = Scan.ValF;
    e->Literal->ValueI = e->Literal->ValueF;
    Scan.Scan();
  }
  else if(Scan.Token=='{')
  {
    e->Op = ACE_LITERAL;
    e->Literal = _Literal();
  }
  else if(Scan.Token=='(')
  {
    Scan.Match('(');
    ACType *ctype = 0;
    if(Scan.Token==sTOK_NAME)
      ctype = FindType(Scan.Name);

    if(ctype && Scan.DirtyPeek()==')')
    {
      Scan.Scan();
      Scan.Match(')');
      e->Op = ACE_CAST;
      e->CastType = ctype;
      e->Left = _Expression();
      return e;                         // this is handled like a prefix operator!
    }
    else
    {
      e = _Expression();
      Scan.Match(')');
    }
  }
  else
  {
    Scan.Error(L"value expected");
  }

postfix:
  if(Scan.IfToken('['))
  {
    e = NewExpr(ACE_INDEX,e,_Expression());
    Scan.Match(']');
    goto postfix;
  }
  if(Scan.IfToken('.'))
  {
    e = NewExpr(ACE_MEMBER,e,0);
    Scan.ScanName(e->Member);
    goto postfix;
  }
  if(Scan.IfToken(AC_TOK_INC))
  {
    e = NewExpr(ACE_POSTINC,e,0);
    goto postfix;
  }
  if(Scan.IfToken(AC_TOK_DEC))
  {
    e = NewExpr(ACE_POSTDEC,e,0);
    goto postfix;
  }
  if(Scan.Token=='(')
  {
    if((e->Op==ACE_VAR && e->Variable->Function))
    {
      e->Op = ACE_CALL;
      _ParameterList(&e->Left);
    }
    else if(e->Op==ACE_MEMBER && e->Member)
    {
      e->Right = NewExpr(ACE_CALL,0,0);
      _ParameterList(&e->Right->Left);
    }
    else
    {
      Scan.Error(L"you can only call() functions and members");
      ACExpression *dummy = NewExpr(ACE_CALL,0,0);
      _ParameterList(&dummy->Left);
    }
    goto postfix;
  }

  return e;
}

ACLiteral *ACDoc::_Literal()
{
  ACLiteral *first = 0;
  ACLiteral **patch = &first;
  ACLiteral *lit = 0;

  Scan.Match('{');
  while(Scan.Errors==0 && Scan.Token!='}')
  {
    if(Scan.Token=='{')
    {
      lit = Pool->Alloc<ACLiteral>();
      sClear(*lit);
      lit->Child = _Literal();
    }
    else
    {
      lit = Pool->Alloc<ACLiteral>();
      sClear(*lit);
      lit->Expr = _Expression();
    }
    /*
    else if(Scan.Token==sTOK_INT)
    {
      lit = Pool->Alloc<ACLiteral>();
      sClear(*lit);
      lit->Token = sTOK_INT;
      lit->Value = Scan.ValueString;
      lit->ValueI = Scan.ValI;
      lit->ValueF = lit->ValueI;
      Scan.Scan();
    }
    else if(Scan.Token==sTOK_FLOAT)
    {
      lit = Pool->Alloc<ACLiteral>();
      sClear(*lit);
      lit->Token = sTOK_FLOAT;
      lit->Value = Scan.ValueString;
      lit->ValueF = Scan.ValF;
      lit->ValueI = lit->ValueF;
      Scan.Scan();
    }
    else
    {
      Scan.Error(L"literal expected");
    }
    */
    *patch = lit;
    patch = &lit->Next;

    if(Scan.Token==',')
      Scan.Scan();
    else
      if(Scan.Token!='}')
        Scan.Error(L"comma expected");
  }
  Scan.Match('}');

  return first;
}


void ACDoc::_ParameterList(ACExpression **patch)
{
  ACExpression *parameter;
  sBool first = 1;

  Scan.Match('(');
  while(Scan.Errors==0 && Scan.Token!=')')
  {
    if(!first)
      Scan.Match(',');
    first = 0;
    parameter = NewExpr(ACE_COMMA,_Expression(),0);
    *patch = parameter;
    patch = &parameter->Right;
  }
  Scan.Match(')');
}

/****************************************************************************/


/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "doc.hpp"

/****************************************************************************/

sBool Document::Parse(const sChar *filename)
{
  Scan.Init();
  Scan.Flags |= sSF_MERGESTRINGS;
  Scan.DefaultTokens();
  Scan.AddToken(L"&&",TOK_ANDAND);
  Scan.AddToken(L"||",TOK_OROR);
  Scan.AddToken(L"if",TOK_IF);
  Scan.AddToken(L"else",TOK_ELSE);
  Scan.AddToken(L"array",TOK_ARRAY);
  Scan.StartFile(filename);

  Priority = 0x10;
  CurrentOp = 0;
  _Global();

  return Scan.Errors==0;
}

/****************************************************************************/

void Document::_Global()
{
  while(!Scan.Errors && Scan.Token!=sTOK_END)
  {
    if(Scan.IfName(L"header"))
    {
      CodeBlock *cb = _CodeBlock();
      if(cb)
        Doc->HCodes.AddTail(cb);
    }
    else if(Scan.IfName(L"header_end"))
    {
      CodeBlock *cb = _CodeBlock();
      if(cb)
        Doc->HEndCodes.AddTail(cb);
    }
    else if(Scan.IfName(L"code"))
    {
      CodeBlock *cb = _CodeBlock();
      if(cb)
        Doc->CCodes.AddTail(cb);
    }
    else if(Scan.IfName(L"type"))
    {
      _Type();
    }
    else if(Scan.IfName(L"operator"))
    {
      _Operator();
    }
    else if(Scan.IfName(L"priority"))
    {
      Scan.Match('=');
      Priority = Scan.ScanInt();
      Scan.Match(';');
    }
    else if(Scan.IfName(L"include_cpp"))
    {
      _Include(Doc->CCodes);
    }
    else if(Scan.IfName(L"include_hpp"))
    {
      _Include(Doc->HCodes);
    }
    else
    {
      Scan.Error(L"unexpected keyword");
    }
  }
}

void Document::_Include(sArray<CodeBlock *> &codes)
{
  sPoolString path;
  sString<sMAXPATH+512> text;
  CodeBlock *code;

  Scan.ScanString(path);
  if(!Scan.Errors)
  {
    text.PrintF(L"#include \"%s\"",path);
    code = new CodeBlock(sPoolString(text),Scan.TokenLine);
    codes.AddTail(code);
  }
  Scan.Match(';');
}

CodeBlock *Document::_CodeBlock()
{
  sPoolString code;
  sInt line = Scan.TokenLine;
  Scan.ScanRaw(code,'{','}');

  if(Scan.Errors)
    return 0;
  else
    return new CodeBlock(code,line);
}

void Document::_External(sArray<External *> &list)
{
  sString<2048> typestr;
  sString<2048> ptypestr;
  sPoolString dummy;
  External *ext = new External;
  list.AddTail(ext);
  ext->Line = Scan.TokenLine;

  // type

  Scan.ScanName(dummy);
  typestr = dummy;
  while(dummy==L"virtual" || dummy==L"static" || dummy==L"const")
  {
    Scan.ScanName(dummy);
    typestr.Add(L" ");
    typestr.Add(dummy);
  }
  ptypestr = dummy;

  if(Scan.IfToken('*'))
  {
    typestr.Add(L" *");
    ptypestr.Add(L" *");
  }
  ext->Type = typestr;
  ext->PureType = ptypestr;

  // name & para

  Scan.ScanName(ext->Name);
  Scan.ScanRaw(ext->Para,'(',')');
  ext->Code = _CodeBlock();  
}

void Document::_Type()
{
  Type *type = new Type;

  if(Scan.IfName(L"virtual")) type->Virtual = 1;
  Scan.ScanName(type->Symbol);
  type->Label = type->Symbol;
  if(!type->Virtual && Scan.IfName(L"virtual")) type->Virtual = 1;

  if(Scan.IfToken(':'))
  {
    if(Scan.Token==sTOK_INT)
    {
      if(Scan.ScanInt()!=0)
        Scan.Error(L"zero expected");
    }
    else
    {
      Scan.ScanName(type->Parent);
    }
  }
  else
  {
    type->Parent = L"AnyType";
  }

  Scan.Match('{');
  while(!Scan.Errors && Scan.Token!='}')
  {
    if(Scan.IfName(L"color"))
    {
      Scan.Match('=');
      type->Color = Scan.ScanInt();
      Scan.Match(';');
    }
    else if(Scan.IfName(L"name"))
    {
      Scan.Match('=');
      Scan.ScanString(type->Label);
      Scan.Match(';');
    }
    else if(Scan.IfName(L"flags"))
    {
      Scan.Match('=');
      type->Flags |= _Choice(L"notab|render3d|uncache");
      Scan.Match(';');
    }
    else if(Scan.IfName(L"gui"))
    {
      Scan.Match('=');
      type->GuiSets |= _Choice(L"base2d|base3d|mode");
      Scan.Match(';');
    }
    else if(Scan.IfName(L"code"))
    {
      type->Code = _CodeBlock();
    }
    else if(Scan.IfName(L"header"))
    {
      type->Header = _CodeBlock();
    }
    else if(Scan.IfName(L"columnheader"))
    {
      Scan.Match('[');
      sInt i = sClamp(Scan.ScanInt(),0,30);
      Scan.Match(']');
      Scan.Match('=');
      Scan.ScanString(type->ColumnHeaders[i]);
      Scan.Match(';');
    }
    else if(Scan.IfName(L"extern"))
    {
      _External(type->Externals);
    }
    else 
    {
      Scan.Error(L"unexpected keyword");
    }
  }
  Scan.Match('}');
  if(!Scan.Errors)
    Doc->Types.AddTail(type);
  else
    delete type;
}

sInt Document::_Choice(const sChar *match)    // get power of two from a|b|c|d -> 1,2,4,8
{
  sPoolString flag;
  sInt value = 0;
  do
  {
    if(Scan.Token==sTOK_INT)
    {
      value |= Scan.ScanInt();
    }
    else
    {
      Scan.ScanName(flag);
      sInt choice = sFindChoice(flag,match);
      if(choice==-1)
        Scan.Error(L"unknown flag <%s>",flag);
      else
        value |= 1<<choice;
    }
  }
  while(!Scan.Errors && Scan.IfToken('|'));

  return value;
}


sInt Document::_Flag(const sChar *match)      // get multiple choice text : a|b:*4c|d -> 0,1,0,16
{
  sPoolString flag;
  sInt value = 0;
  do
  {
    if(Scan.Token==sTOK_INT)
    {
      value |= Scan.ScanInt();
    }
    else
    {
      sInt mask_;
      sInt value_;
      Scan.ScanName(flag);
      sBool found = sFindFlag(flag,match,mask_,value_);
      if(!found)
        Scan.Error(L"unknown flag <%s>",flag);
      else
        value |= value_;
    }
  }
  while(!Scan.Errors && Scan.IfToken('|'));

  return value;
}


void Document::_Operator()
{
  sPoolString name;
  Op *op = new Op;
  CurrentOp = op;
 

  Scan.ScanName(op->OutputType);
  Scan.ScanName(op->Name);
  if(Scan.Token == sTOK_STRING)
    Scan.ScanString(op->Label);
  else
    op->Label = op->Name;
  Scan.Match('(');
  sInt first = 1;
  while(!Scan.Errors && Scan.Token!=')')
  {
    if(!first)
      Scan.Match(',');
    first = 0;

    Input *in = new Input;

    for(;;)
    {
      if(Scan.IfToken('*'))
        op->Flags |= 1;
      else if(Scan.IfToken('?'))
        in->InputFlags = IF_OPTIONAL;
      else if(Scan.IfToken('~'))
        in->InputFlags = IF_WEAK;
      else
        break;
    }

    Scan.ScanName(in->Type);
    op->Inputs.AddTail(in);
    if(Scan.IfToken('='))
    {
      Scan.ScanName(in->DefaultOpType);
      Scan.ScanName(in->DefaultOpName);
    }

    if((op->Flags & 1) && Scan.Token!=')')
      Scan.Error(L"variable numer of inputs must be declared for last input");
  }
  if(op->Inputs.GetCount()>1 || (op->Flags & 1))
    op->Column = 2;
  else if(op->Inputs.GetCount()==1)
    op->Column = 1;
  Input *input;
  sFORALL(op->Inputs,input)
    if(input->Type!= op->OutputType)
      op->Column = 3;
  Scan.Match(')');
  Scan.Match('{');
  while(!Scan.Errors && Scan.Token!='}')
  {
    if(Scan.IfName(L"parameter"))
    {
      sInt offset = 0;
      sInt stringoffset = 0;
      sInt linkoffset = 0;
      _Parameters(op,0,offset,stringoffset,0,linkoffset);
    }
    else if(Scan.IfName(L"code"))
    {
      if(op->Code)
        Scan.Error(L"more than one code blocks!");
      op->Code = _CodeBlock();
    }
    else if(Scan.IfName(L"shortcut"))
    {
      Scan.Match('=');
      op->Shortcut = Scan.ScanInt();
      Scan.Match(';');
    }
    else if(Scan.IfName(L"column"))
    {
      Scan.Match('=');
      op->Column = Scan.ScanInt();
      Scan.Match(';');
    }
    else if(Scan.IfName(L"gridcolumns"))
    {
      Scan.Match('=');
      op->GridColumns = Scan.ScanInt();
      Scan.Match(';');
    }
    else if(Scan.IfName(L"flags"))
    {
      Scan.Match('=');
      op->Flags |= _Choice(L"||load|store|delete_import|delete_array_import|hide|conversion"
        L"|logging|slow|blockhandles|passinput|passoutput|curve|clip|obsolete|verticalresize|comment"
        L"|call|input|loop|endloop|shellswitch|typefrominput|blockchange");
      Scan.Match(';');
    }
    else if(Scan.IfName(L"tab"))
    {
      Scan.Match('=');
      Scan.ScanName(op->TabType);
      Scan.Match(';');
    }
    else if(Scan.IfName(L"extract"))
    {
      Scan.Match('=');
      Scan.ScanString(op->Extract);
      Scan.Match(';');
    }
    else if(Scan.IfName(L"new"))
    {
      Scan.Match('=');
      Scan.ScanName(op->OutputClass);
      Scan.Match(';');
    }
    else if(Scan.IfName(L"handles"))
    {
      op->Handles = _CodeBlock();
    }
    else if(Scan.IfName(L"setdefaultsarray"))
    {
      op->SetDefaultsArray = _CodeBlock();
    }
    else if(Scan.IfName(L"actions"))
    {
      op->Actions = _CodeBlock();
    }
    else if(Scan.IfName(L"actions"))
    {
      op->Actions = _CodeBlock();
    }
    else if(Scan.IfName(L"drag"))
    {
      op->SpecialDrag = _CodeBlock();
    }
    else if(Scan.IfName(L"description"))
    {
      op->Description = _CodeBlock();
    }
    else if(Scan.IfName(L"helper"))
    {
      op->Helper = _Struct();
    }
    else if(Scan.IfName(L"customed"))
    {
      Scan.Match('=');
      Scan.ScanName(op->CustomEd);
      Scan.Match(';');
    }
    else
    {
      Scan.Error(L"unknown keyword");
    }
  }
  Scan.Match('}');

  if(!Scan.Errors)
    Doc->Ops.AddTail(op);
  else
    delete op;

  CurrentOp = 0;
}

void Document::_Parameter(Op *op,ExprNode *cond,sInt &offset,sInt &stringoffset,sInt inarray,sInt &linkoffset)
{
  sPoolString name,label;
  sInt type;

  Parameter *para = new Parameter;

  sInt count = 1;
  sBool nolabel = 0;

  if(Scan.IfName(L"padding"))
  {
    if(Scan.IfToken('('))
    {
      offset += Scan.ScanInt();
      Scan.Match(')');
    }
    else
    {
      offset++;
    }
    Scan.Match(';');
    return;
  }

  for(;;)
  {
    if(Scan.IfName(L"nolabel"))             // don't print label, but still indent
      nolabel = 1;
    else if(Scan.IfName(L"layout"))         // triggers relayout
      para->LayoutFlag = 1;
    else if(Scan.IfName(L"continue"))       // for flags
      para->ContinueFlag = 1;
    else if(Scan.IfName(L"filter"))         // for filtering arrays in auto-mode (display 
      para->FilterMode = 1;
    else if(Scan.IfName(L"lines"))          // for text and custom: how many lines?
      para->GridLines = Scan.ScanInt();
    else if(Scan.IfName(L"overbox"))        // for text and custom: extend into "box" area
      para->Flags |= PF_OverBox;
    else if(Scan.IfName(L"overlabel"))      // for text and custom: extend into "label" area
    {
      para->Flags |= PF_OverLabel;
      nolabel = 1;
    }
    else if(Scan.IfName(L"anim"))           // mark animatable parameters
      para->Flags |= PF_Anim;
    else if(Scan.IfName(L"linenumber"))
      para->Flags |= PF_LineNumber;
    else if(Scan.IfName(L"narrow"))
      para->Flags |= PF_Narrow;
    else if(Scan.IfName(L"static"))
      para->Flags |= PF_Static;
    else
      break;
  }

  type = 0;
  if(Scan.IfName(L"label"))
  {
    type = TYPE_LABEL;
  }
  else if(Scan.IfName(L"group"))
  {
    type = TYPE_GROUP;
  }
  else if(Scan.IfName(L"float"))
  {
    type = TYPE_FLOAT;
    para->CType = L"sF32";
  }
  else if(Scan.IfName(L"float2"))
  {
    type = TYPE_FLOAT;
    para->CType = L"sVector2";
    count = 2;
    para->XYZW = 1;
  }
  else if(Scan.IfName(L"float30"))
  {
    type = TYPE_FLOAT;
    para->CType = L"sVector30";
    count = 3;
    para->XYZW = 1;
  }
  else if(Scan.IfName(L"float31"))
  {
    type = TYPE_FLOAT;
    para->CType = L"sVector31";
    count = 3;
    para->XYZW = 1;
  }
  else if(Scan.IfName(L"float4"))
  {
    type = TYPE_FLOAT;
    para->CType = L"sVector4";
    count = 4;
    para->XYZW = 1;
  }
  else if(Scan.IfName(L"int"))
  {
    type = TYPE_INT;
    para->CType = L"sInt";
  }
  else if(Scan.IfName(L"bitmask"))
  {
    type = TYPE_BITMASK;
    para->CType = L"sInt";
    para->Max = 1;
    para->Min = 0;
  }
  else if(Scan.IfName(L"char"))
  {
    Scan.Match('[');
    para->Max = Scan.ScanInt();
    Scan.Match(']');
    type = TYPE_CHARARRAY;
    sString<32> b;
    b.PrintF(L"sString<%d>",para->Max);
    para->CType = sPoolString(b);
  }
  else if(Scan.IfName(L"color"))
  {
    type = TYPE_COLOR;
    para->CType = L"sU32";
  }
  else if(Scan.IfName(L"flags"))
  {
    type = TYPE_FLAGS;
    para->CType = L"sInt";
  }
  else if(Scan.IfName(L"radio"))
  {
    type = TYPE_RADIO;
    para->CType = L"sInt";
  }
  else if(Scan.IfName(L"strobe"))
  {
    type = TYPE_STROBE;
  }
  else if(Scan.IfName(L"action"))
  {
    type = TYPE_ACTION;
  }
  else if(Scan.IfName(L"string"))
  {
    type = TYPE_STRING;
  }
  else if(Scan.IfName(L"filein"))
  {
    type = TYPE_FILEIN;
  }
  else if(Scan.IfName(L"fileout"))
  {
    type = TYPE_FILEOUT;
  }
  else if(Scan.IfName(L"link"))
  {
    type = TYPE_LINK;
    para->LinkMethod = IE_LINK;
    para->Offset = linkoffset++;
  }
  else if(Scan.IfName(L"custom"))
  {
    type = TYPE_CUSTOM;
  }
  else
    Scan.Error(L"unknown parameter type");

  para->Type = type;

  sBool labelscanned = 0;
  if(Scan.Token==sTOK_STRING)
  {
    labelscanned = 1;
    Scan.ScanString(para->Label);
  }

  if(Scan.Token==sTOK_NAME)
  {
    Scan.ScanName(para->Symbol);
    if(!nolabel && para->Label.IsEmpty())
      para->Label = para->Symbol;
  }

  if(!labelscanned && Scan.Token==sTOK_STRING)
    Scan.ScanString(para->Label);


  if(Scan.IfToken('['))
  {
    if(para->XYZW)
      Scan.Error(L"vector types can't be indexed");
    count = Scan.ScanInt();
    Scan.Match(']');
  }
  para->Count = count;
  if(type!=TYPE_INT && type!=TYPE_FLOAT && count!=1)
    Scan.Error(L"arrays only supported for float and int");

  switch(type)
  {
  case TYPE_STRING:
  case TYPE_FILEIN:
  case TYPE_FILEOUT:
    if(Scan.IfToken(':'))
      stringoffset = Scan.ScanInt();
    para->Offset = stringoffset;
    if(para->Offset>=32)
      Scan.Error(L"more than 32 strings - can't mark file in/out correctly");
    stringoffset += count;
    op->MaxStrings = sMax(op->MaxStrings,stringoffset);
    if(type==TYPE_FILEIN)
      op->FileInMask |= 1<<para->Offset;
    if(type==TYPE_FILEOUT)
      op->FileOutMask |= 1<<para->Offset;
    break;
  case TYPE_LINK:
    if(Scan.IfToken(':'))
    {
      para->Offset = Scan.ScanInt();
      linkoffset = para->Offset+1;
    }
    if(para->Offset>=0 && para->Offset<op->Inputs.GetCount())
      op->Inputs[para->Offset]->GuiSymbol = para->Symbol;
    break;
  default:
    para->Offset = -1;
    if(!para->ContinueFlag && !para->CType.IsEmpty())
    {
      if(Scan.IfToken(':'))
        offset = Scan.ScanInt();
      para->Offset = offset;
      if(para->Type==TYPE_CHARARRAY)
        offset += (para->Max+1)/2;
      else
        offset += count;
      if(inarray)
        op->MaxArrayOffset = sMax(op->MaxArrayOffset,offset);
      else
        op->MaxOffset = sMax(op->MaxOffset,offset);
    }
    break;
  }

  if(para->ContinueFlag)
  {
    if(type!=TYPE_FLAGS)
      Scan.Error(L"continue keyword only valid on flags");
    if(inarray)
    {
      if(!sFind(op->ArrayParam,&Parameter::Symbol,para->Symbol))
        Scan.Error(L"could not find <%s> to continue",para->Symbol);
    }
    else
    {
      if(!sFind(op->Parameters,&Parameter::Symbol,para->Symbol))
        Scan.Error(L"could not find <%s> to continue",para->Symbol);
    }
  }
  if(para->LayoutFlag)
  {
    if(type!=TYPE_FLAGS && type!=TYPE_INT)
      Scan.Error(L"layout keyword only valid on flags and int");
  }

  if(Scan.IfToken('('))
  {
    switch(type)
    {
    case TYPE_FLOAT:
      para->Min = Scan.ScanFloat();
      Scan.Match(sTOK_ELLIPSES);
      para->Max = Scan.ScanFloat();
      if(Scan.IfName(L"step"))
      {
        para->Step = Scan.ScanFloat();
        if(Scan.IfToken(','))
          para->RStep = Scan.ScanFloat();
      }
      else if(Scan.IfName(L"logstep"))
      {
        para->Step = -Scan.ScanFloat();
      }
      break;
    case TYPE_COLOR:
      Scan.ScanString(para->Options);
      break;
    case TYPE_INT:
      para->Min = Scan.ScanInt();
      Scan.Match(sTOK_ELLIPSES);
      para->Max = Scan.ScanInt();
      if(Scan.IfName(L"step"))
      {
        para->Step = Scan.ScanFloat();
        if(Scan.IfToken(','))
          para->RStep = Scan.ScanFloat();
      }
      if(Scan.IfName(L"hex"))
      {
        if(Scan.Token==sTOK_INT)
        {
          sString<16> buffer;
          sInt n;
          n = Scan.ScanInt();
          buffer.PrintF(L"%%0%dx",n);
          para->Format = buffer;
        }
        else
        {
          para->Format = L"%06x";
        }
      }
      break;
    case TYPE_BITMASK:
      if(Scan.Token==sTOK_INT)
        para->Max = sClamp(Scan.ScanInt(),1,4);
      break;
    case TYPE_FLAGS:
    case TYPE_RADIO:
    case TYPE_STROBE:
      Scan.ScanString(para->Options);
      break;
    case TYPE_ACTION:
      para->DefaultS[0] = Scan.ScanInt();
      op->ActionInfos.AddMany(1)->Init(para->Label,para->DefaultS[0]);
      break;
    case TYPE_LINK:
      if(Scan.IfName(L"both"))
        para->LinkMethod = IE_BOTH;
      else if(Scan.IfName(L"choose"))
        para->LinkMethod = IE_CHOOSE;
      else if(Scan.IfName(L"anim"))
        para->LinkMethod = IE_ANIM;
      break;
    case TYPE_CUSTOM:
      Scan.ScanName(para->CustomName);
      break;
    case TYPE_FILEIN:
      Scan.ScanString(op->FileInFilter);
      break;
    default:
      Scan.Error(L"parameter options not defined for this type");
      break;
    }
    Scan.Match(')');
  }

  if(Scan.IfToken('='))
  {
    if(para->ContinueFlag)
      Scan.Error(L"specify defaults only once when using continue keyword");
    sBool multiple = 0;
    if(Scan.IfToken('{'))
      multiple = 1;

    sInt num = 0;
    while(!Scan.Errors && Scan.Token!='}' && Scan.Token!=';')
    {
      if(num!=0)
        Scan.Match(',');

      if(num>=count || num>=16)
      {
        Scan.Error(L"too many default values");
      }

      switch(type)
      {
      case TYPE_FLOAT:
        para->DefaultF[num] = Scan.ScanFloat();
        break;
      case TYPE_INT:
      case TYPE_BITMASK:
        para->DefaultS[num] = Scan.ScanInt();
        break;
      case TYPE_COLOR:
        para->DefaultU[num] = Scan.ScanInt();
        break;
      case TYPE_RADIO:
      case TYPE_FLAGS:
        para->DefaultU[num] = _Flag(para->Options);
        break;
      case TYPE_STRING:
      case TYPE_CHARARRAY:
        if(Scan.IfName(L"random"))
          para->DefaultStringRandom = 1;
        else
          Scan.ScanString(para->DefaultString);
        break;
      case TYPE_FILEIN:
      case TYPE_FILEOUT:
        Scan.ScanString(para->DefaultString);
        break;
      default:
        Scan.Error(L"default not defined for this type");
        break;
      }
      num++;
    }
    if(multiple)
      Scan.Match('}');

    if(!multiple)
      for(sInt i=1;i<para->Count;i++)
        para->DefaultU[i] = para->DefaultU[0];
  }
  Scan.Match(';');

  if(para->Type==TYPE_LINK && para->Offset>=0 && para->Offset<op->Inputs.GetCount())
    op->Inputs[para->Offset]->Method = para->LinkMethod;


  para->Condition = cond;

  if(Scan.Errors)
  {
    delete para;
  }
  else
  {
    if(inarray)
      op->ArrayParam.AddTail(para);
    else
      op->Parameters.AddTail(para);
  }
}



void Document::_Parameters(Op *op,ExprNode *cond,sInt &offset,sInt &stringoffset,sInt inarray,sInt &linkoffset)
{
  Scan.Match('{');
  while(!Scan.Errors && Scan.Token!='}')
  {
    if(Scan.IfToken(TOK_ARRAY))
    {
      if(inarray)
        Scan.Error(L"arrays in array not allowed");

      if(Scan.IfName(L"hide"))
        op->HideArray = Scan.ScanInt();
      if(Scan.IfName(L"group"))
        op->GroupArray = 1;
      if(Scan.IfName(L"numbers"))
        op->ArrayNumbers = 1;
      if(Scan.IfName(L"defaultmode"))
      {
        Scan.Match('=');
        if(Scan.IfName(L"auto"))
          op->DefaultArrayMode = 0;
        else if(Scan.IfName(L"all"))
          op->DefaultArrayMode = 1;
        else if(Scan.IfName(L"hide"))
          op->DefaultArrayMode = 2;
        else if(Scan.IfName(L"group"))
          op->DefaultArrayMode = 3;
        else
          Scan.Error(L"specify defaultmode auto, all, hide or group");
      }
      sInt offset2=0;
      sInt stringoffset2=0;
      _Parameters(op,0,offset2,stringoffset2,1,linkoffset);
    }
    else if(Scan.IfToken(TOK_IF))
    {
      ExprNode *expr;
      Scan.Match('(');
      expr = _Expr();
      if(cond)
        expr = NewExpr(EOP_AND,cond,expr);
      Scan.Match(')');
      if(Scan.Token=='{')
        _Parameters(op,expr,offset,stringoffset,inarray,linkoffset);
      else
        _Parameter(op,expr,offset,stringoffset,inarray,linkoffset);
    }
    else if(Scan.IfName(L"tie"))
    {
      sPoolString name;
      Tie *tie = new Tie;
      while(!Scan.Errors && !Scan.IfToken(','))
      {
        Scan.ScanName(name);
        tie->Paras.AddTail(name);
      }
      Scan.ScanName(name);
      tie->Paras.AddTail(name);
      Scan.Match(';');
      if(inarray)
        op->ArrayTies.AddTail(tie);
      else
        op->Ties.AddTail(tie);
    }
    else
    {
      _Parameter(op,cond,offset,stringoffset,inarray,linkoffset);
    }
  }
  Scan.Match('}');
}

/****************************************************************************/

ExprNode *Document::NewExpr(sInt op,ExprNode *left,ExprNode *right)
{
  ExprNode *node = Pool.Alloc<ExprNode>();
  sClear(*node);
  node->Op = op;
  node->Left = left;
  node->Right = right;
  return node;
}

ExprNode *Document::NewExprInt(sInt value)
{
  ExprNode *node = NewExpr(EOP_INT,0,0);
  node->Value = value;
  return node;
}


sBool Document::FindFlag(sPoolString para_,const sChar *choice,sInt &mask_,sInt &value_)
{
  if(!CurrentOp) return 0;
  Parameter *para;
  
  sFORALL(CurrentOp->Parameters,para)
  {
    if(para->Symbol!=para_)
      continue;
    if(para->Type!=TYPE_FLAGS)
      continue;
  
    if(sFindFlag(choice,para->Options,mask_,value_))
      return 1;
  }
  return 0;
}


ExprNode *Document::_Value()
{
  if(Scan.IfToken('!'))
    return NewExpr(EOP_NOT,_Value(),0);
  if(Scan.IfToken('~'))
    return NewExpr(EOP_BITNOT,_Value(),0);

  ExprNode *node = 0;;

  if(Scan.IfName(L"input"))
  {
    node = NewExpr(EOP_INPUT,0,0);
    Scan.Match('[');
    node->Value = Scan.ScanInt();
    Scan.Match(']');
  }
  else if(Scan.Token==sTOK_NAME)
  {
    sPoolString member;
    Scan.ScanName(member);

    node = NewExpr(EOP_SYMBOL,0,0);
    node->Symbol = member;

    if(Scan.IfToken('.'))
    {
      sPoolString choice;
      Scan.ScanName(choice);
      sInt value,mask;
      if(FindFlag(member,choice,mask,value))
      {
        node = NewExpr(EOP_EQ,NewExpr(EOP_BITAND,node,NewExprInt(mask)),NewExprInt(value));
      }
      else
      {
        Scan.Error(L"could not find choice <%s> in flags <%s>",choice,member);
      }
    }
  }
  else if(Scan.Token==sTOK_INT)
  {
    node = NewExpr(EOP_INT,0,0);
    node->Op = EOP_INT;
    node->Value = Scan.ValI;
    Scan.Scan();
  }
  else if(Scan.IfToken('('))
  {
    node = _Expr();
    Scan.Match(')');
  }
  else
  {
    node = NewExpr(0,0,0);
    Scan.Error(L"value expected");
  }

  return node;
}

static struct ExprOpType
{
  sInt Token;
  sInt Priority;
  sInt OpCode;
} ExprOps[] =
{
  { '&',1,EOP_BITAND },
  { '|',2,EOP_BITOR },

  { '>',3,EOP_GT },
  { '<',3,EOP_LT },
  { sTOK_GE,3,EOP_GE },
  { sTOK_LE,3,EOP_LE },

  { sTOK_EQ,4,EOP_EQ },
  { sTOK_NE,4,EOP_NE },

  { TOK_ANDAND,5,EOP_AND },
  { TOK_OROR,6,EOP_OR },

  { 0,0,0 }
};


ExprNode *Document::_Expr(sInt level)
{
  sInt op,pri;
  ExprNode *expr;

  if(level==0)
    return _Value();

  expr = _Expr(level-1);
  while(!Scan.Errors)
  {
    op = 0;
    pri = 0;
    for(sInt i=0;ExprOps[i].Token;i++)
    {
      if(ExprOps[i].Token==Scan.Token)
      {
        op = ExprOps[i].OpCode;
        pri = ExprOps[i].Priority;
        break;
      }
    }
    if(!op || pri!=level)
      break;
    Scan.Scan();
    expr = NewExpr(op,expr,_Expr(level-1));
  }

  return expr;
}

/****************************************************************************/

Struct *Document::_Struct()
{
  Struct *str = new Struct;
  StructMember *mem;

  Scan.Match('{');
  while(!Scan.Errors && !Scan.IfToken('}'))
  {
    mem = str->Members.AddMany(1);
    mem->ArrayCount = 0;
    mem->PointerCount = 0;
    Scan.ScanName(mem->CType);
    mem->Line = Scan.TokenLine;
    while(Scan.IfToken('*'))
      mem->PointerCount++;
    if(Scan.IfToken('['))
    {
      mem->ArrayCount = Scan.ScanInt();
      Scan.Match(']');
    }
    Scan.ScanName(mem->Name);
    Scan.Match(';');
  }

  return str;
}

/****************************************************************************/

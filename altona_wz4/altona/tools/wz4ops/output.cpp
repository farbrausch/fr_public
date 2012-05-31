/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "doc.hpp"

void MakeWikiText(Op *op,sTextBuffer &tb);

/****************************************************************************/

void Sep(sTextBuffer &tb)
{
  tb.Print(L"\n");
  tb.Print(L"/****************************************************************************/\n");
  tb.Print(L"\n");
}

sBool Document::Output()
{
  // headers

  HPP.Print(L"/****************************************************************************/\n");
  HPP.Print(L"/***                                                                      ***/\n");
  HPP.Print(L"/***   Computer generated code - do not modify                            ***/\n");
  HPP.Print(L"/***                                                                      ***/\n");
  HPP.Print(L"/****************************************************************************/\n");
  HPP.Print(L"\n");
  HPP.PrintF(L"#ifndef HEADER_WZ4OPS_%s\n",ProjectName);
  HPP.PrintF(L"#define HEADER_WZ4OPS_%s\n",ProjectName);
  HPP.Print(L"\n");
  HPP.Print(L"#include \"wz4lib/doc.hpp\"\n");

  CPP.Print(L"/****************************************************************************/\n");
  CPP.Print(L"/***                                                                      ***/\n");
  CPP.Print(L"/***   Computer generated code - do not modify                            ***/\n");
  CPP.Print(L"/***                                                                      ***/\n");
  CPP.Print(L"/****************************************************************************/\n");
  CPP.Print(L"\n");
  CPP.Print(L"#define sPEDANTIC_OBSOLETE 1\n");
  CPP.Print(L"#define sPEDANTIC_WARN 1\n");
  CPP.Print(L"#include \"gui/gui.hpp\"\n");
  CPP.Print(L"#include \"gui/textwindow.hpp\"\n");
  CPP.Print(L"#include \"wz4lib/script.hpp\"\n");
  CPP.PrintF(L"#include \"%s.hpp\"\n",ProjectName);
  CPP.Print(L"\n");
  CPP.Print(L"#pragma warning(disable:4189) // unused variables - happens in generated code\n");
  CPP.Print(L"\n");

  // put it all together


  OutputTypes1();
  OutputCodeblocks();
  OutputTypes2();
  OutputOps();
  OutputAnim();
  OutputMain();

  CodeBlock *cb;
  sFORALL(HEndCodes,cb)
  {
    Sep(HPP);
    cb->Output(HPP,HPPFileName);
  }

  // done

  Sep(CPP);
  Sep(HPP);
  HPP.Print(L"#endif");
  HPP.Print(L"\n");

  return 1;
}

/****************************************************************************/

void Document::HPPLine(sInt line)
{
  const sChar *file = Doc->InputFileName;
  if(line==-1)
  {
    line = sCountChar(HPP.Get(),'\n')+2;
    file = Doc->HPPFileName;
  }
  HPP.PrintF(L"#line %d \"%p\"\n",line,file);
}

void Document::CPPLine(sInt line)
{
  const sChar *file = Doc->InputFileName;
  if(line==-1)
  {
    line = sCountChar(CPP.Get(),'\n')+2;
    file = Doc->CPPFileName;
  }
  CPP.PrintF(L"#line %d \"%p\"\n",line,file);
}

void Document::OutputCodeblocks()
{
  CodeBlock *cb;

  sFORALL(HCodes,cb)
  {
    Sep(HPP);
    cb->Output(HPP,HPPFileName);
  }

  sFORALL(CCodes,cb)
  {
    Sep(CPP);
    cb->Output(CPP,CPPFileName);
  }
}

void Document::OutputExpr(ExprNode *node)
{
  if(node->Right) CPP.Print(L"("); 
  switch(node->Op)
  {
    case EOP_LE:     OutputExpr(node->Left); CPP.Print(L"<="); OutputExpr(node->Right); break;
    case EOP_GE:     OutputExpr(node->Left); CPP.Print(L">="); OutputExpr(node->Right); break;
    case EOP_LT:     OutputExpr(node->Left); CPP.Print(L"<");  OutputExpr(node->Right); break;
    case EOP_GT:     OutputExpr(node->Left); CPP.Print(L">");  OutputExpr(node->Right); break;

    case EOP_EQ:     OutputExpr(node->Left); CPP.Print(L"=="); OutputExpr(node->Right); break;
    case EOP_NE:     OutputExpr(node->Left); CPP.Print(L"!="); OutputExpr(node->Right); break;
    case EOP_AND:    OutputExpr(node->Left); CPP.Print(L"&&"); OutputExpr(node->Right); break;
    case EOP_OR:     OutputExpr(node->Left); CPP.Print(L"||"); OutputExpr(node->Right); break;
    case EOP_BITAND: OutputExpr(node->Left); CPP.Print(L"&");  OutputExpr(node->Right); break;
    case EOP_BITOR:  OutputExpr(node->Left); CPP.Print(L"|");  OutputExpr(node->Right); break;

    case EOP_NOT:    CPP.Print(L"!"); OutputExpr(node->Left); break;
    case EOP_BITNOT: CPP.Print(L"~"); OutputExpr(node->Left); break;

    case EOP_INT:    CPP.PrintF(L"0x%04x",node->Value); break;
    case EOP_SYMBOL: CPP.PrintF(L"para->%s",node->Symbol); break;
    case EOP_INPUT:  CPP.PrintF(L"(op->ConnectionMask & (1<<%d))!=0",node->Value); break;
  }
  if(node->Right) CPP.Print(L")"); 
}

void Document::OutputExt(External *ext,const sChar *classname)
{
  HPPLine(ext->Line);
  HPP.PrintF(L"  %s %s(%s);\n",ext->Type,ext->Name,ext->Para);

  CPPLine(ext->Line);
  CPP.PrintF(L"%s %sType_::%s(%s)\n",ext->PureType,classname,ext->Name,ext->Para);
  CPP.Print(L"{\n");
  ext->Code->Output(CPP,Doc->CPPFileName,1);
  CPP.Print(L"}\n");
}

void Document::OutputTypes1()
{
  Type *type;

  Sep(CPP);
  Sep(HPP);
  sFORALL(Types,type)
  {
    CPP.PrintF(L"class %sType_ *%sType;\n",type->Symbol,type->Symbol);
    HPP.PrintF(L"extern class %sType_ *%sType;\n",type->Symbol,type->Symbol);
  }
}

static const sChar *_xyzw = L"xyzw";

void Document::OutputTypes2()
{
  Type *type;
  External *ext;

  Sep(CPP);
  sFORALL(Types,type)
  {
    if(type->Parent.IsEmpty())
      HPP.PrintF(L"class %sType_ : public wType\n",type->Symbol);
    else
      HPP.PrintF(L"class %sType_ : public %sType_\n",type->Symbol,type->Parent);
    HPP.Print (L"{\n");
    HPP.Print (L"public:\n");
    HPP.PrintF(L"  %sType_() { ",type->Symbol);
    if(!type->Parent.IsEmpty())
      HPP.PrintF(L"Parent = %sType; ",type->Parent);
    if(type->Color!=0)
      HPP.PrintF(L"Color= 0x%08x; ",type->Color);
    HPP.PrintF(L"Flags = 0x%04x; ",type->Flags);
    HPP.PrintF(L"GuiSets = 0x%04x; ",type->GuiSets);
    HPP.PrintF(L"Symbol = L\"%s\"; ",type->Symbol);
    HPP.PrintF(L"Label = L\"%s\"; ",type->Label);
    HPP.Print (L" Init(); }\n");
    HPP.PrintF(L"  ~%sType_() { Exit(); }\n",type->Symbol);
/*
    if(type->Virtual)
      HPP.PrintF(L"  wObject *Create() { return 0; }\n",type->Symbol);
    else
      HPP.PrintF(L"  wObject *Create() { return new %s; }\n",type->Symbol);
*/
    if(type->Header)
      type->Header->Output(HPP,HPPFileName);

    sFORALL(type->Externals,ext)
      OutputExt(ext,type->Symbol);
    HPPLine();

    HPP.Print (L"};\n");
    HPP.Print (L"\n");

    if(type->Code)
      type->Code->Output(CPP,CPPFileName);
  }
}

void Document::OutputParaStruct(const sChar *label,sArray<Parameter *> &pa,Op *op)
{
  Parameter *para;
  sArray<Parameter *> sorted;
  sInt offset;

  sFORALL(pa,para)
    if(!para->CType.IsEmpty() && !para->ContinueFlag)
      sorted.AddTail(para);
  sSortUp(sorted,&Parameter::Offset);
  offset = 0;

  HPP.PrintF(L"struct %s%s%s\n",op->OutputType,label,op->Name);
  HPP.Print (L"{\n");
  sFORALL(sorted,para)
  {
    if(para->Offset<offset)
      Scan.Error(L"parameters overlap in op <%s>, parameter <%s>",op->Name,para->Symbol);
    if(para->Offset>offset)
    {
      sInt delta = para->Offset-offset;
      if(delta==1)
        HPP.PrintF(L"  sInt _pad%d;\n",offset);
      else
        HPP.PrintF(L"  sInt _pad%d[%d];\n",offset,delta);
      offset += delta;
    }
    HPP.PrintF(L"  %s %s",para->CType,para->Symbol);
    if(para->Count>1 && !para->XYZW)
      HPP.PrintF(L"[%d]",para->Count);
    HPP.Print(L";\n");
    offset += para->Count;
  }
  HPP.Print (L"};\n");
  HPP.Print (L"\n");
}

void Document::OutputIf(ExprNode *cond,ExprNode **condref,sInt indent)
{
  if(*condref != cond)
  {
    if(*condref)
      CPP.PrintF(L"%_}\n",indent);
    *condref = 0;
    if(cond)
    {
      CPP.PrintF(L"%_if(",indent);
      *condref = cond;
      OutputExpr(cond);
      CPP.PrintF(L")\n%_{\n",indent);
    }
  }
}

void Document::OutputPara(sArray<Parameter *> &pa,Op *op,sBool inarray)
{
  Parameter *para;
  sInt indent;
  const sChar *name = inarray ? L"elem" : L"para";

  ExprNode *opencond = 0;
  sFORALL(pa,para)
  {
    CPP.Print (L"\n");
    indent = 2;
    if(inarray)
      indent += 2;

    OutputIf(para->Condition,&opencond,indent);
    if(opencond)
      indent += 2;
    if((para->Flags & PF_Narrow) && !inarray)
      CPP.PrintF(L"%_gh.ControlWidth = 1;\n",indent);

    if(!para->Label.IsEmpty() && !inarray && para->Type!=TYPE_ACTION)
    {
      if(para->Type==TYPE_GROUP)
      {
        CPP.PrintF(L"%_gh.Group(L\"%s\");\n",indent,para->Label);
      }
      else
      {
        if(para->Flags & PF_Anim)
          CPP.PrintF(L"%_gh.Label(L\"* %s\");\n",indent,para->Label);
        else
          CPP.PrintF(L"%_gh.Label(L\"%s\");\n",indent,para->Label);
      }
    }
    if(para->Count>1)
    {
      CPP.PrintF(L"%_gh.BeginTied();\n",indent);
      switch(para->Type)
      {
      case TYPE_FLOAT:
        for(sInt i=0;i<para->Count;i++)
        {
          if(para->XYZW)
          {
            CPP.PrintF(L"%_sFloatControl *float_%s_%d = gh.Float(&%s->%s.%c,%9F,%9F,%9F);\n",
              indent,para->Symbol,i,name,para->Symbol,_xyzw[i],para->Min,para->Max,para->Step);
          }
          else
          {
            CPP.PrintF(L"%_sFloatControl *float_%s_%d = gh.Float(&%s->%s[%d],%9F,%9F,%9F);\n",
              indent,para->Symbol,i,name,para->Symbol,i,para->Min,para->Max,para->Step);
          }
          CPP.PrintF(L"%_float_%s_%d->Default = %9F; float_%s_%d->RightStep = %9F;\n",
            indent,para->Symbol,i,para->DefaultF[i],para->Symbol,i,para->RStep);
        }
        break;
      case TYPE_INT:
        for(sInt i=0;i<para->Count;i++)
        {
          if(para->XYZW)
          {
            if(para->Format.IsEmpty())
            {
              CPP.PrintF(L"%_sIntControl *int_%s_%d = gh.Int(&%s->%s.%c,%d,%d,%9F);\n",
              indent,para->Symbol,i,name,para->Symbol,_xyzw[i],(sInt)para->Min,(sInt)para->Max,para->Step);
            }
            else
            {
              CPP.PrintF(L"%_sIntControl *int_%s_%d = gh.Int(&%s->%s.%c,%d,%d,%9F,0,L\"%s\");\n",
              indent,para->Symbol,i,name,para->Symbol,_xyzw[i],(sInt)para->Min,(sInt)para->Max,para->Step,para->Format);
            }
          }
          else
          {
            if(para->Format.IsEmpty())
            {
              CPP.PrintF(L"%_sIntControl *int_%s_%d = gh.Int(&%s->%s[%d],%d,%d,%9F);\n",
              indent,para->Symbol,i,name,para->Symbol,i,(sInt)para->Min,(sInt)para->Max,para->Step);
            }
            else
            {
              CPP.PrintF(L"%_sIntControl *int_%s_%d = gh.Int(&%s->%s[%d],%d,%d,%9F,0,L\"%s\");\n",
              indent,para->Symbol,i,name,para->Symbol,i,(sInt)para->Min,(sInt)para->Max,para->Step,para->Format);
            }
          }
          CPP.PrintF(L"%_int_%s_%d->Default = 0x%08x; int_%s_%d->RightStep = %9F;\n",
            indent,para->Symbol,i,para->DefaultU[i],para->Symbol,i,para->RStep);
        }
        break;
      }
      CPP.PrintF(L"%_gh.EndTied();\n",indent);
    }
    else
    {
      switch(para->Type)
      {
      case TYPE_FLOAT:
        CPP.PrintF(L"%_sFloatControl *float_%s = gh.Float(&%s->%s,%9F,%9F,%9F);\n",
          indent,para->Symbol,name,para->Symbol,para->Min,para->Max,para->Step);
        CPP.PrintF(L"%_float_%s->Default = %9F; float_%s->RightStep = %9F;\n",
          indent,para->Symbol,para->DefaultF[0],para->Symbol,para->RStep);
        if(para->Flags & PF_Static)
          CPP.PrintF(L"%_float_%s->Style = sSCS_STATIC;\n",indent,para->Symbol);
        break;
      case TYPE_INT:
        if(para->Format.IsEmpty())
        {
          CPP.PrintF(L"%_sIntControl *int_%s = gh.Int(&%s->%s,%d,%d,%9F);\n",
          indent,para->Symbol,name,para->Symbol,(sInt)para->Min,(sInt)para->Max,para->Step);
        }
        else
        {
          CPP.PrintF(L"%_sIntControl *int_%s = gh.Int(&%s->%s,%d,%d,%9F,0,L\"%s\");\n",
          indent,para->Symbol,name,para->Symbol,(sInt)para->Min,(sInt)para->Max,para->Step,para->Format);
        }
        CPP.PrintF(L"%_int_%s->Default = 0x%08x; int_%s->RightStep = %9F;\n",
          indent,para->Symbol,para->DefaultU[0],para->Symbol,para->RStep);
        if(para->LayoutFlag)
          CPP.PrintF(L"%_int_%s->DoneMsg = gh.LayoutMsg;\n",indent,para->Symbol);
        if(para->Flags & PF_Static)
          CPP.PrintF(L"%_int_%s->Style = sSCS_STATIC;\n",indent,para->Symbol);
        break;
      case TYPE_BITMASK:
        CPP.PrintF(L"%_gh.Bitmask((sU8 *)&%s->%s,%d);\n",indent,name,para->Symbol,para->Max);
        break;
      case TYPE_CHARARRAY:
        CPP.PrintF(L"%_gh.String(%s->%s);\n",indent,name,para->Symbol);
        break;
      case TYPE_FLAGS:
        if(para->Options[0]=='*' && para->Options[1]=='*')
          CPP.PrintF(L"%_gh.Flags(&%s->%s,%s,gh.%s);\n",indent,name,para->Symbol,para->Options+2,para->LayoutFlag ? L"LayoutMsg" : L"ChangeMsg");
        else
          CPP.PrintF(L"%_gh.Flags(&%s->%s,L\"%s\",gh.%s);\n",indent,name,para->Symbol,para->Options,para->LayoutFlag ? L"LayoutMsg" : L"ChangeMsg");
        break;
      case TYPE_RADIO:
        CPP.PrintF(L"%_gh.Radio(&%s->%s,L\"%s\",1);\n",indent,name,para->Symbol,para->Options);
        break;
      case TYPE_STROBE:
        CPP.PrintF(L"%_gh.Flags(&op->Strobe,L\"%s\",gh.ChangeMsg);\n",indent,para->Options);
        break;
      case TYPE_ACTION:
        CPP.PrintF(L"%_gh.ActionMsg.Code = %d;\n",indent,para->DefaultS[0]);
        CPP.PrintF(L"%_gh.PushButton(L\"%s\",gh.ActionMsg);\n",indent,para->Label);
        break;
      case TYPE_COLOR:
        if(inarray)
          CPP.PrintF(L"%_gh.Color(&%s->%s,L\"%s\");\n",indent,name,para->Symbol,para->Options);
        else
          CPP.PrintF(L"%_gh.ColorPick(&%s->%s,L\"%s\",0);\n",indent,name,para->Symbol,para->Options);
        break;
      case TYPE_STRING:
        if(para->GridLines == 1)
        {
          CPP.PrintF(L"%_gh.String(op->EditString[%d])%s;\n",indent,para->Offset,
            (para->Flags & PF_Static)?L"->Style=sSCS_STATIC":L"");
        }
        else
        {
          if(para->Flags & PF_OverLabel)
          CPP.PrintF(L"%_gh.Left = 0;\n",indent);
          CPP.PrintF(L"%_{\n",indent);
          if(para->Flags & PF_OverBox)
            CPP.PrintF(L"%_  sTextWindow *tw = gh.Text(op->EditString[%d],%d,gh.Grid->Columns-gh.Left);\n",indent,para->Offset,para->GridLines);
          else
            CPP.PrintF(L"%_  sTextWindow *tw = gh.Text(op->EditString[%d],%d,gh.Right-gh.Left);\n",indent,para->Offset,para->GridLines);
          CPP.PrintF(L"%_  tw->SetFont(sGui->FixedFont);\n",indent);
          if(para->Flags & PF_LineNumber)
            CPP.PrintF(L"%_  tw->EditFlags |= sTEF_LINENUMBER;\n",indent);
          if(para->Flags & PF_Static)
            CPP.PrintF(L"%_  tw->EditFlags |= sTEF_STATIC;\n",indent);
          CPP.PrintF(L"%_}\n",indent);
        }
        break;
      case TYPE_FILEIN:
        CPP.PrintF(L"%_{\n",indent);
        CPP.PrintF(L"%_  sControl *con = gh.String(op->EditString[%d])%s;\n",indent,para->Offset,
          (para->Flags & PF_Static)?L"->Style=sSCS_STATIC":L"");
        // file parameters are "expensive". only update on "done", not on "change". 
        CPP.PrintF(L"%_  con->DoneMsg = con->ChangeMsg;\n",indent);
        CPP.PrintF(L"%_  con->ChangeMsg = sMessage();\n",indent);
        CPP.PrintF(L"%_  gh.FileLoadDialogMsg.Code = %d;\n",indent,para->Offset);
        CPP.PrintF(L"%_  gh.Box(L\"...\",gh.FileLoadDialogMsg);\n",indent);
        CPP.PrintF(L"%_  gh.Box(L\"reload\",gh.FileReloadMsg);\n",indent);
        CPP.PrintF(L"%_}\n",indent);
        break;
      case TYPE_FILEOUT:
        CPP.PrintF(L"%_{\n",indent);
        CPP.PrintF(L"%_  sControl *con = gh.String(op->EditString[%d])%s;\n",indent,para->Offset,
          (para->Flags & PF_Static)?L"->Style=sSCS_STATIC":L"");
        // file parameters are "expensive". only update on "done", not on "change". 
        CPP.PrintF(L"%_  con->DoneMsg = con->ChangeMsg;\n",indent);
        CPP.PrintF(L"%_  con->ChangeMsg = sMessage();\n",indent);
        CPP.PrintF(L"%_  gh.FileSaveDialogMsg.Code = %d;\n",indent,para->Offset);
        CPP.PrintF(L"%_  gh.Box(L\"...\",gh.FileSaveDialogMsg);\n",indent);
        CPP.PrintF(L"%_}\n",indent);
        break;
      case TYPE_LINK:
        switch(para->LinkMethod)
        {
        case IE_BOTH:
          CPP.PrintF(L"%_gh.Flags(&op->Links[%d].Select,L\"input|link\",gh.ConnectLayoutMsg);\n",indent,para->Offset);
          CPP.PrintF(L"%_if(op->Links[%d].Select==1)\n",indent,para->Offset);
          break;
        case IE_CHOOSE:
          CPP.PrintF(L"%_gh.Flags(&op->Links[%d].Select,L\"|link|unused",indent,para->Offset);
          for(sInt i=0;i<op->Inputs.GetCount();i++)
            CPP.PrintF(L"| %d",i+1);
          CPP.Print(L"\",gh.ConnectLayoutMsg);\n");
          CPP.PrintF(L"%_if(op->Links[%d].Select==1)\n",indent,para->Offset);
          break;
        }
        CPP.PrintF(L"%_{\n",indent);
        CPP.PrintF(L"%_  sControl *con=gh.String(op->Links[%d].LinkName,gh.LabelWidth+gh.WideWidth-gh.Left);\n",indent,para->Offset);
        CPP.PrintF(L"%_  con->ChangeMsg = gh.ConnectMsg;\n",indent);
        CPP.PrintF(L"%_  con->DoneMsg = gh.ConnectLayoutMsg;\n",indent);
        CPP.PrintF(L"%_  sMessage msg = gh.LinkBrowserMsg; msg.Code = %d;\n",indent,para->Offset);
        CPP.PrintF(L"%_  gh.Box(L\"...\",msg);\n",indent);
        CPP.PrintF(L"%_  msg = gh.LinkPopupMsg; msg.Code = %d;\n",indent,para->Offset);
        CPP.PrintF(L"%_  gh.Box(L\"..\",msg);\n",indent);
        CPP.PrintF(L"%_  if(!op->Links[%d].LinkName.IsEmpty())\n",indent,para->Offset);
        CPP.PrintF(L"%_  {\n",indent);
        if(para->LinkMethod==IE_ANIM)
          CPP.PrintF(L"%_    msg = gh.LinkAnimMsg; msg.Code = %d;\n",indent,para->Offset);
        else
          CPP.PrintF(L"%_    msg = gh.LinkGotoMsg; msg.Code = %d;\n",indent,para->Offset);
        CPP.PrintF(L"%_    gh.Box(L\"->\",msg);\n",indent);
        CPP.PrintF(L"%_  }\n",indent);
        if(para->LinkMethod==IE_ANIM)
        {
          CPP.PrintF(L"%_  else\n",indent);
          CPP.PrintF(L"%_  {\n",indent);
          CPP.PrintF(L"%_    msg = gh.LinkAnimMsg; msg.Code = %d;\n",indent,para->Offset);
          CPP.PrintF(L"%_    gh.Box(L\"anim\",msg);\n",indent);
          CPP.PrintF(L"%_  }\n",indent);
        }
        CPP.PrintF(L"%_}\n",indent);
        break;
      case TYPE_CUSTOM:
        CPP.PrintF(L"%_{\n",indent);
        CPP.PrintF(L"%_  sControl *con = new %s(op);\n",indent,para->CustomName);
        CPP.PrintF(L"%_  con->ChangeMsg = gh.ConnectMsg;\n",indent);
        CPP.PrintF(L"%_  con->DoneMsg = gh.ConnectLayoutMsg;\n",indent);
        CPP.PrintF(L"%_  sInt lines = %d;\n",indent,para->GridLines);
        CPP.PrintF(L"%_  sInt left =  %s;\n",indent,(para->Flags & PF_OverLabel) ?L"0":L"gh.Left");
        CPP.PrintF(L"%_  sInt right = %s;\n",indent,(para->Flags & PF_OverBox) ? L"gh.Grid->Columns" : L"gh.Left+gh.WideWidth");
        CPP.PrintF(L"%_  gh.Grid->AddGrid(con,left,gh.Line,right-left,lines);\n",indent);
        CPP.PrintF(L"%_  gh.Left += gh.WideWidth;\n",indent);
        CPP.PrintF(L"%_  gh.Line += lines-1;\n",indent);
//        CPP.PrintF(L"%_  \n",indent);
        CPP.PrintF(L"%_}\n",indent);
        break;
      }
    }
    if((para->Flags & PF_Narrow) && !inarray)
      CPP.PrintF(L"%_gh.ControlWidth = 2;\n",indent);
  }
  indent = 2;
  if(inarray)
    indent += 2;
  OutputIf(0,&opencond,indent);
}


void Document::OutputTies(sArray<Tie *> &ties)
{
  Tie *tie;
  sPoolString *name;
  sFORALL(ties,tie)
  {
    CPP.PrintF(L"  gh.BeginTied();\n");
    sFORALL(tie->Paras,name)
      CPP.PrintF(L"  gh.Tie(float_%s);\n",*name);
    CPP.PrintF(L"  gh.EndTied();\n");
  }
}

void Document::OutputOps()
{
  Op *op;
  Input *in;
  Parameter *para=0;

  sFORALL(Ops,op)
  {
    Sep(CPP);

    // parameter struct

    OutputParaStruct(L"Para",op->Parameters,op);
    if(op->ArrayParam.GetCount()>0)
      OutputParaStruct(L"Array",op->ArrayParam,op);

    // helper struct

    if(op->Helper)
    {
      sString<256> buffer;
      sSPrintF(buffer,L"%sHelper%s",op->OutputType,op->Name);
      OutputStruct(CPP,op->Helper,buffer);
    }

    // code
  
    if(op->Code)
    {
      CPP.PrintF(L"sBool %sCmd%s(wExecutive *exe,wCommand *cmd)\n",op->OutputType,op->Name);
      CPP.Print (L"{\n");
      CPP.PrintF(L"  %sPara%s sUNUSED *para = (%sPara%s *)(cmd->Data); para;\n",op->OutputType,op->Name,op->OutputType,op->Name);
      sFORALL(op->Inputs,in)
        CPP.PrintF(L"  %s sUNUSED *in%d = cmd->GetInput<%s *>(%d); in%d;\n",in->Type,_i,in->Type,_i,_i);
      sPoolString type = op->OutputClass.IsEmpty() ? op->OutputType : op->OutputClass;
      CPP.PrintF(L"  %s *out = (%s *) cmd->Output;\n",type,type);
      if(op->Flags & 0x0800)  // wCF_PASSINPUT "passinput"
        CPP.PrintF(L"  if(!out) { out=new %s; out->CopyFrom(in0); cmd->Output=out; }\n",type);
      else
        CPP.PrintF(L"  if(!out) { out=new %s; cmd->Output=out; }\n",type);
      CPP.Print (L"\n");
      CPP.Print (L"  {\n");
      op->Code->Output(CPP,CPPFileName,1);
      CPP.Print (L"    return 1;\n");
      CPP.Print (L"  }\n");
      CPP.Print (L"}\n");
      CPP.Print (L"\n");
    }

    // handles

    if(op->Handles)
    {
      CPP.PrintF(L"void %sHnd%s(wPaintInfo &pi,wOp *op)\n",op->OutputType,op->Name);
      CPP.PrintF(L"{\n");
      CPP.PrintF(L"  %sPara%s sUNUSED *para = (%sPara%s *)(op->EditData); para;\n",op->OutputType,op->Name,op->OutputType,op->Name);
      if(op->Helper)
        CPP.PrintF(L"  %sHelper%s *helper = (%sHelper%s *)(op->HelperData);\n",op->OutputType,op->Name,op->OutputType,op->Name);
      op->Handles->Output(CPP,CPPFileName,1);
      CPP.PrintF(L"}\n");
      CPP.Print (L"\n");
    }
  
    // setdefaultsarray

    if(op->SetDefaultsArray || op->ArrayParam.GetCount())
    {
      CPP.PrintF(L"void %sArr%s(wOp *op,sInt pos,void *mem)\n",op->OutputType,op->Name);
      CPP.PrintF(L"{\n");
      CPP.PrintF(L"  %sPara%s sUNUSED *para = (%sPara%s *)(op->EditData); para;\n",op->OutputType,op->Name,op->OutputType,op->Name);
      CPP.PrintF(L"  %sArray%s *e = (%sArray%s *)(mem);\n",op->OutputType,op->Name,op->OutputType,op->Name);
      if(op->SetDefaultsArray)
      {
        op->SetDefaultsArray->Output(CPP,CPPFileName,1);
      }
      else    // generate code automatically
      {
        // first initialize proper
  
        sFORALL(op->ArrayParam,para)
        {
          for(sInt i=0;i<para->Count;i++)
          {
            sInt t = para->Type;
            if(t==TYPE_INT || t==TYPE_BITMASK || t==TYPE_COLOR || t==TYPE_FLAGS || t==TYPE_FLOAT || t==TYPE_CHARARRAY || t==TYPE_RADIO)
            {
              if(para->Count==1)
                CPP.PrintF(L"  e->%s = ",para->Symbol);
              else if(para->XYZW)
                CPP.PrintF(L"  e->%s.%c = ",para->Symbol,_xyzw[i]);
              else
                CPP.PrintF(L"  e->%s[%d] = ",para->Symbol,i);

              if(para->Type==TYPE_FLOAT)
                CPP.PrintF(L"%9F;\n",para->DefaultF[i]);
              else if(para->Type==TYPE_CHARARRAY)
                CPP.PrintF(L"%q;\n",para->DefaultString[i]);
              else
                CPP.PrintF(L"0x%08x;\n",para->DefaultU[i]);
            }
          }
        }

        // possible interpolation

        CPP.PrintF(L"  sInt max = op->ArrayData.GetCount();\n");
        CPP.PrintF(L"  if(max>=1)\n");
        CPP.PrintF(L"  {\n");
        CPP.PrintF(L"    %sArray%s *p0,*p1;\n",op->OutputType,op->Name);
        CPP.PrintF(L"    sF32 f0,f1;\n");
        CPP.PrintF(L"    if(max==1)\n");
        CPP.PrintF(L"    {\n");
        CPP.PrintF(L"      f0 =  1; p0 = (%sArray%s *)op->ArrayData[0];\n",op->OutputType,op->Name);
        CPP.PrintF(L"      f1 =  0; p1 = (%sArray%s *)op->ArrayData[0];\n",op->OutputType,op->Name);
        CPP.PrintF(L"    }\n");
        CPP.PrintF(L"    else if(pos==0)\n");
        CPP.PrintF(L"    {\n");
        CPP.PrintF(L"      f0 =  2; p0 = (%sArray%s *)op->ArrayData[0];\n",op->OutputType,op->Name);
        CPP.PrintF(L"      f1 = -1; p1 = (%sArray%s *)op->ArrayData[1];\n",op->OutputType,op->Name);
        CPP.PrintF(L"    }\n");
        CPP.PrintF(L"    else if(pos==max)\n");
        CPP.PrintF(L"    {\n");
        CPP.PrintF(L"      f0 =  2; p0 = (%sArray%s *)op->ArrayData[max-1];\n",op->OutputType,op->Name);
        CPP.PrintF(L"      f1 = -1; p1 = (%sArray%s *)op->ArrayData[max-2];\n",op->OutputType,op->Name);
        CPP.PrintF(L"    }\n");
        CPP.PrintF(L"    else\n");
        CPP.PrintF(L"    {\n");
        CPP.PrintF(L"      f0 =0.5f; p0 = (%sArray%s *)op->ArrayData[pos-1];\n",op->OutputType,op->Name);
        CPP.PrintF(L"      f1 =0.5f; p1 = (%sArray%s *)op->ArrayData[pos  ];\n",op->OutputType,op->Name);
        CPP.PrintF(L"    }\n");

        sFORALL(op->ArrayParam,para)
        {
          for(sInt i=0;i<para->Count;i++)
          {
            sInt t = para->Type;
            if(t==TYPE_FLOAT)
            {
              if(para->Count==1)
                CPP.PrintF(L"    e->%s = f0*p0->%s + f1*p1->%f;\n",para->Symbol,para->Symbol,para->Symbol);
              else if(para->XYZW)
                CPP.PrintF(L"    e->%s.%c = f0*p0->%s.%c + f1*p1->%s.%c;\n",para->Symbol,_xyzw[i],para->Symbol,_xyzw[i],para->Symbol,_xyzw[i]);
              else
                CPP.PrintF(L"    e->%s[%d] = f0*p0->%s[%d] + f1*p1->%s[%d];\n",para->Symbol,i,para->Symbol,i,para->Symbol,i);
            }
          }
        }
        CPP.PrintF(L"  }\n");
      }
      CPP.PrintF(L"}\n");
      CPP.Print (L"\n");
    }

    // custom editor

    if(!op->CustomEd.IsEmpty())
    {
      CPP.PrintF(L"wCustomEditor *%sCed%s(wOp *op)\n",op->OutputType,op->Name);
      CPP.PrintF(L"{\n");
      CPP.PrintF(L"  return new %s(op);\n",op->CustomEd);
      CPP.PrintF(L"}\n");
      CPP.Print (L"\n");
    }

    // actions

    if(op->Actions)
    {
      CPP.PrintF(L"sInt %sAct%s(wOp *op,sInt code,sInt pos)\n",op->OutputType,op->Name);
      CPP.PrintF(L"{\n");
      CPP.PrintF(L"  %sPara%s sUNUSED *para = (%sPara%s *)(op->EditData); para;\n",op->OutputType,op->Name,op->OutputType,op->Name);
      op->Actions->Output(CPP,CPPFileName,1);
      CPP.PrintF(L"}\n");
      CPP.Print (L"\n");
    }

    // ...

    if(op->SpecialDrag)
    {
      CPP.PrintF(L"void %sDrag%s(const sWindowDrag &dd,sDInt mode,wOp *op,const sViewport &view,wPaintInfo &pi)\n",op->OutputType,op->Name);
      CPP.PrintF(L"{\n");
      CPP.PrintF(L"  %sPara%s sUNUSED *para = (%sPara%s *)(op->EditData); para;\n",op->OutputType,op->Name,op->OutputType,op->Name);
      op->SpecialDrag->Output(CPP,CPPFileName,1);
      CPP.PrintF(L"}\n");
      CPP.Print (L"\n");
    }

    if(op->Description)
    {
      CPP.PrintF(L"const sChar *%sDescription%s(wOp *op)\n",op->OutputType,op->Name);
      CPP.PrintF(L"{\n");
      CPP.PrintF(L"  %sPara%s sUNUSED *para = (%sPara%s *)(op->EditData); para;\n",op->OutputType,op->Name,op->OutputType,op->Name);
      op->Description->Output(CPP,CPPFileName,1);
      CPP.PrintF(L"}\n");
      CPP.Print (L"\n");
    }

    // gui

    CPP.PrintF(L"void %sGui%s(wGridFrameHelper &gh,wOp *op)\n",op->OutputType,op->Name);
    CPP.Print (L"{\n");
    CPP.PrintF(L"  %sPara%s sUNUSED *para = (%sPara%s *)(op->EditData); para;\n",op->OutputType,op->Name,op->OutputType,op->Name);
    if(op->Parameters.GetCount()>0 && op->Parameters[0]->Type!=TYPE_GROUP)
      CPP.Print (L"  gh.Group(op->Class->Label);\n");
    OutputPara(op->Parameters,op,0);
    OutputTies(op->Ties);
    if(op->ArrayParam.GetCount()>0)
    {
      sInt FilterIndex = -1;
      sInt FilterMode = 0;

      sFORALL(op->ArrayParam,para)
      {
        if(para->FilterMode >=0)
        {
          FilterIndex = para->Offset;
          FilterMode = para->FilterMode;
        }
      }

      CPP.Print(L"  void *ap;\n");
      CPP.Print(L"  sInt pos = 0;\n");
      CPP.Print(L"  gh.LabelWidth = 0;\n");
      CPP.Print(L"  gh.Group(L\"Elements\");\n");
      if(op->GroupArray)
        CPP.Print(L"  gh.Flags(&op->ArrayGroupMode,L\"auto|all|hide|group\",gh.LayoutMsg);\n");
      else
        CPP.Print(L"  gh.Flags(&op->ArrayGroupMode,L\"auto|all|hide\",gh.LayoutMsg);\n");
      CPP.Print(L"  gh.PushButton(L\"clear all\",gh.ArrayClearAllMsg);\n");
//      CPP.Print(L"  gh.PushButton(L\"select all\",gh.ArraySelectAllMsg);\n");
      CPP.Print(L"  gh.ControlWidth = 1;\n");
      CPP.Print(L"  gh.NextLine();\n");
    
      sInt width;
      ExprNode *opencond = 0;
      if(op->ArrayNumbers)
        CPP.PrintF(L"  gh.Grid->AddLabel(L\"#\",pos,gh.Line,1,1,sGFLF_GROUP|sGFLF_NARROWGROUP); pos+=1;\n",para->Label);

      sFORALL(op->ArrayParam,para)
      {
        if(!para->Label.IsEmpty())
        {
          width = sMax(1,para->Count);
          if(para->Type==TYPE_FLAGS)
          {
            const sChar *s = para->Options;
            while(*s) if(*s++==':') width++;
          }
          if(para->Type==TYPE_COLOR)
            width = sGetStringLen(para->Options);
          OutputIf(para->Condition,&opencond,2);
          sInt indent = opencond ? 4 : 2;
          CPP.PrintF(L"%_gh.Grid->AddLabel(L\"%s\",pos,gh.Line,%d,1,sGFLF_GROUP|sGFLF_NARROWGROUP); pos+=%d;\n",indent,para->Label,width,width);
        }
      }
      OutputIf(0,&opencond,2);

      CPP.Print (L"  sBool hideall = (op->ArrayGroupMode==2);\n");
      CPP.Print (L"  sBool hidesome = (op->ArrayGroupMode==3);\n");
      if(op->GroupArray && FilterIndex<0)
      {
        CPP.PrintF(L"  if(op->ArrayGroupMode==0 && op->ArrayData.GetCount()>%d)\n",op->HideArray?op->HideArray:20);
        CPP.PrintF(L"  {\n");
        CPP.PrintF(L"    sInt n = 0;\n");
        CPP.PrintF(L"    sFORALL(op->ArrayData,ap) { if(*((sU32 *)ap)!=0) n++; }\n");
        CPP.PrintF(L"    if(n>%d) hideall=1; else hidesome=1;\n",op->HideArray?op->HideArray:20);
        CPP.PrintF(L"  }\n");
      }
      else if(op->GroupArray && FilterIndex>=0)
      {
        CPP.PrintF(L"  if(op->ArrayGroupMode==0 && op->ArrayData.GetCount()>%d) hidesome=1;\n",op->HideArray?op->HideArray:20);
      }
      else
      {
        CPP.PrintF(L"  if(op->ArrayGroupMode==0 && op->ArrayData.GetCount()>%d) hideall=1;\n",op->HideArray?op->HideArray:20);
      }

      CPP.Print (L"  if(!hideall)\n");
      CPP.Print (L"  {\n");

      CPP.Print (L"    gh.EmptyLine = 0;\n");
      CPP.Print (L"    gh.NextLine();\n");
      CPP.Print (L"    sFORALL(op->ArrayData,ap)\n");
      CPP.Print (L"    {\n");
      CPP.PrintF(L"      %sArray%s *elem = (%sArray%s *)ap;\n",op->OutputType,op->Name,op->OutputType,op->Name);

      if(FilterIndex>=0)
      {
        if(FilterMode==0)
          CPP.PrintF(L"      if(hidesome && ((sU32 *)elem)[%d]!=0) continue;\n",FilterIndex);
        else
          CPP.PrintF(L"      if(hidesome && ((sU32 *)elem)[%d]==0) continue;\n",FilterIndex);   
      }
      else
      {
        CPP.Print (L"      if(hidesome && *((sU32 *)elem)==0) continue;\n");
      }
      if(op->ArrayNumbers)
      {
        CPP.PrintF(L"      sString<16> numbuf; numbuf.PrintF(L\"%%d\",_i);\n");
        CPP.PrintF(L"      gh.Grid->AddLabel(sPoolString(numbuf),0,gh.Line,1,1,0);\n");
      }
      if(op->ArrayNumbers)
        CPP.Print (L"    gh.Left = 1;\n");
      OutputPara(op->ArrayParam,op,1);
      OutputTies(op->ArrayTies);
      CPP.Print (L"      if(hidesome)\n");
      CPP.Print (L"      {\n");
      CPP.Print (L"        gh.RemArrayGroupMsg.Code = _i;\n");
      CPP.Print (L"        gh.Box(L\"Rem\",gh.RemArrayGroupMsg);\n");
      CPP.Print (L"      }\n");
      CPP.Print (L"      else\n");
      CPP.Print (L"      {\n");
      CPP.Print (L"        gh.AddArrayMsg.Code = _i;\n");
      CPP.Print (L"        gh.RemArrayMsg.Code = _i;\n");
      CPP.Print (L"        gh.Box(L\"Add\",gh.AddArrayMsg,sGFLF_HALFUP);\n");
      CPP.Print (L"        gh.Box(L\"Rem\",gh.RemArrayMsg)->InitRadio(&op->HighlightArrayLine,_i);\n");
      CPP.Print (L"      }\n");

      CPP.Print (L"      if(gh.Left<gh.Right && !(_i&1)) gh.Grid->AddLabel(0,gh.Left,gh.Line,gh.Right-gh.Left,1,sGFLF_LEAD);\n");
      CPP.Print (L"      gh.NextLine();\n");
      CPP.Print (L"    }\n");
      CPP.Print (L"    gh.AddArrayMsg.Code = op->ArrayData.GetCount();\n");
      CPP.Print (L"    if(!hidesome)\n");
      CPP.Print (L"      gh.Box(L\"Add\",gh.AddArrayMsg,sGFLF_HALFUP);\n");
      CPP.Print (L"  }\n");
    }
    CPP.Print (L"}\n");
    CPP.Print (L"\n");

    // default

    CPP.PrintF(L"void %sDef%s(wOp *op)\n",op->OutputType,op->Name);
    CPP.Print (L"{\n");
    CPP.PrintF(L"  %sPara%s sUNUSED *para = (%sPara%s *)(op->EditData); para;\n",op->OutputType,op->Name,op->OutputType,op->Name);
    CPP.PrintF(L"  op->ArrayGroupMode = %d;\n",op->DefaultArrayMode);
    sFORALL(op->Parameters,para)
    {
      if(para->ContinueFlag)
        continue;
      for(sInt i=0;i<para->Count;i++)
      {
        sBool skip = 1;
        switch(para->Type)
        {
        case TYPE_INT:
        case TYPE_BITMASK:
        case TYPE_COLOR:
        case TYPE_RADIO:
        case TYPE_FLAGS:
          if(para->DefaultU[i])
            skip = 0;
          break;
        case TYPE_FLOAT:
          if(para->DefaultF[i])
            skip = 0;
          break;
        case TYPE_STRING:
        case TYPE_FILEIN:
        case TYPE_FILEOUT:
          if(!para->DefaultString.IsEmpty() || para->DefaultStringRandom)
            skip = 0;
          break;
        case TYPE_CHARARRAY:
          skip = 0;
          break;
        }
        if(!skip)
        {
          if(para->Type==TYPE_STRING || para->Type==TYPE_FILEIN || para->Type==TYPE_FILEOUT)
            CPP.PrintF(L"  *op->EditString[%d] = ",para->Offset);
          else if(para->Count==1)
            CPP.PrintF(L"  para->%s = ",para->Symbol);
          else if(para->XYZW)
            CPP.PrintF(L"  para->%s.%c = ",para->Symbol,_xyzw[i]);
          else
            CPP.PrintF(L"  para->%s[%d] = ",para->Symbol,i);

          switch(para->Type)
          {
          case TYPE_INT:
          case TYPE_FLAGS:
          case TYPE_RADIO:
          case TYPE_COLOR:
          case TYPE_BITMASK:
            CPP.PrintF(L"0x%08x;\n",para->DefaultU[i]);
            break;

          case TYPE_FLOAT:
            CPP.PrintF(L"%9F;\n",para->DefaultF[i]);
            break;

          case TYPE_STRING:
          case TYPE_CHARARRAY:
            if(para->DefaultStringRandom)
              CPP.PrintF(L"Doc->CreateRandomString();\n");
            else
              CPP.PrintF(L"L\"%s\";\n",para->DefaultString);
            break;

          case TYPE_FILEIN:
          case TYPE_FILEOUT:
            CPP.PrintF(L"L\"%s\";\n",para->DefaultString);
            break;
          }
        }
      }
    }
    CPP.Print (L"}\n");
    CPP.Print (L"\n");

    // bindings

    CPP.PrintF(L"void %sBind%s(wCommand *cmd,ScriptContext *ctx)\n",op->OutputType,op->Name);
    CPP.Print (L"{\n");
    sBool first = 1;
    sFORALL(op->Parameters,para)
    {
      if(para->ContinueFlag)
        continue;
      for(sInt i=0;i<para->Count;i++)       // this loop serves no purpose, but makes things slower during execution!
      {
        switch(para->Type)
        {
        case TYPE_INT:
        case TYPE_BITMASK:
          if(first)
          {
            CPP.Print (L"  ScriptValue *val;\n");
            first = 0;
          }
          CPP.PrintF(L"  val = ctx->MakeValue(ScriptTypeInt,%d);\n",para->Count);
          CPP.PrintF(L"  val->IntPtr = ((sInt *)cmd->Data)+%d;\n",para->Offset);
          CPP.PrintF(L"  ctx->BindLocal(ctx->AddSymbol(L%q),val);\n",para->ScriptName());
          break;

        case TYPE_FLOAT:
          if(first)
          {
            CPP.Print (L"  ScriptValue *val;\n");
            first = 0;
          }
          CPP.PrintF(L"  val = ctx->MakeValue(ScriptTypeFloat,%d);\n",para->Count);
          CPP.PrintF(L"  val->FloatPtr = ((sF32 *)cmd->Data)+%d;\n",para->Offset);
          CPP.PrintF(L"  ctx->BindLocal(ctx->AddSymbol(L%q),val);\n",para->ScriptName());
          break;

        case TYPE_COLOR:
          if(first)
          {
            CPP.Print (L"  ScriptValue *val;\n");
            first = 0;
          }
          CPP.PrintF(L"  val = ctx->MakeValue(ScriptTypeColor,%d);\n",para->Count);
          CPP.PrintF(L"  val->ColorPtr = ((sU32 *)cmd->Data)+%d;\n",para->Offset);
          CPP.PrintF(L"  ctx->BindLocal(ctx->AddSymbol(L%q),val);\n",para->ScriptName());
          break;

        case TYPE_STRING:
        case TYPE_FILEOUT:
        case TYPE_FILEIN:
          if(first)
          {
            CPP.Print (L"  ScriptValue *val;\n");
            first = 0;
          }
          CPP.PrintF(L"  val = ctx->MakeValue(ScriptTypeString,%d);\n",para->Count);
          CPP.PrintF(L"  val->StringPtr = ((sPoolString *)cmd->Strings)+%d;\n",para->Offset);
          CPP.PrintF(L"  ctx->BindLocal(ctx->AddSymbol(L%q),val);\n",para->ScriptName());
          break;
        }
      }
    }
    CPP.Print (L"}\n");

    // bindings 2

    CPP.PrintF(L"void %sBind2%s(wCommand *cmd,ScriptContext *ctx)\n",op->OutputType,op->Name);
    CPP.Print (L"{\n");
    if(op->Parameters.GetCount()>0)
    {
      CPP.PrintF(L"  static sInt initdone = 0;\n");
      CPP.PrintF(L"  static sPoolString name[%d];\n",op->Parameters.GetCount());
      CPP.PrintF(L"  if(!initdone)\n  {\n    initdone = 1; \n");
      sFORALL(op->Parameters,para)
        CPP.PrintF(L"    name[%d] = sPoolString(L%q);\n",_i,para->ScriptName());
      CPP.PrintF(L"  }\n");

      sFORALL(op->Parameters,para)
      {
        if(para->ContinueFlag)
          continue;
        switch(para->Type)
        {
        case TYPE_INT:
        case TYPE_BITMASK:
          CPP.PrintF(L"  ctx->AddImport(name[%d],ScriptTypeInt,%d,cmd->Data+%d);\n",_i,para->Count,para->Offset);
          break;

        case TYPE_FLOAT:
          CPP.PrintF(L"  ctx->AddImport(name[%d],ScriptTypeFloat,%d,cmd->Data+%d);\n",_i,para->Count,para->Offset);
          break;

        case TYPE_COLOR:
          CPP.PrintF(L"  ctx->AddImport(name[%d],ScriptTypeColor,%d,cmd->Data+%d);\n",_i,para->Count,para->Offset);
          break;

        case TYPE_STRING:
        case TYPE_FILEOUT:
        case TYPE_FILEIN:
          CPP.PrintF(L"  ctx->AddImport(name[%d],ScriptTypeString,%d,cmd->Strings+%d);\n",_i,para->Count,para->Offset);
          break;
        }
      }
    }
    CPP.Print (L"}\n");

    CPP.PrintF(L"void %sBind3%s(wOp *op,sTextBuffer &tb)\n",op->OutputType,op->Name);
    CPP.Print (L"{\n");
    if(op->Parameters.GetCount()>0)
    {
      sFORALL(op->Parameters,para)
      {
        const sChar *tname = 0;
        switch(para->Type)
        {
          case TYPE_BITMASK:
          case TYPE_INT:      tname = L"int"; break;
          case TYPE_FLOAT:    tname = L"float"; break;
          case TYPE_COLOR:    tname = L"color"; break;
          case TYPE_STRING:
          case TYPE_FILEIN:
          case TYPE_FILEOUT:  tname = L"string"; break;
        }
        if(tname)
        {
          CPP.PrintF(L"  tb.PrintF(L\"  import %s : %s",para->ScriptName(),tname);
          if(para->Count>1)
            CPP.PrintF(L"[%d]",para->Count);
          CPP.Print(L";\\n\");\n");
        }
      }
    }
    CPP.Print (L"}\n");


    // WikiText
    
    sTextBuffer tb;
    MakeWikiText(op,tb);
    CPP.PrintF(L"const sChar *%sWiki%s =\n",op->OutputType,op->Name);
    const sChar *s = tb.Get();
    while(*s)
    {
      sInt i=0;
      while(s[i]!='\n' && s[i]!=0) i++;
      CPP.Print(L"L\"");
      CPP.Print(s,i);
      CPP.Print(L"\\n\"\n");
      s+=i;
      if(*s=='\n')s++;
    }
    CPP.Print (L";\n");
  }
}

void Document::OutputStruct(sTextBuffer &tb,Struct *str,const sChar *name)
{
  StructMember *mem;
  tb.PrintF(L"struct %s\n",name);
  tb.Print(L"{\n");
  sFORALL(str->Members,mem)
  {
    CPPLine(mem->Line);
    tb.PrintF(L"  %s ",mem->CType);
    for(sInt i=0;i<mem->PointerCount;i++)
      tb.Print(L"*");
    tb.Print(mem->Name);
    if(mem->ArrayCount)
      tb.PrintF(L"[%d]",mem->ArrayCount);
    tb.Print(L";\n");
  }
  CPPLine(-1);
  tb.Print(L"};\n");
}

void Document::OutputAnim()
{
  Op *op;
  Parameter *para;

  Sep(HPP);
  Sep(CPP);

  sFORALL(Ops,op)
  {
    sInt needanim = 0;
    sFORALL(op->Parameters,para)
      needanim |= (para->Flags & PF_Anim)?1:0;
  
    HPP.PrintF(L"struct %sAnim%s\n",op->OutputType,op->Name);
    HPP.PrintF(L"{\n");
    HPP.PrintF(L"  void Init(class ScriptContext *sc);\n");
    HPP.PrintF(L"  void Bind(class ScriptContext *sc,%sPara%s *para);\n",op->OutputType,op->Name);
//    HPP.PrintF(L"  void UnBind(class ScriptContext *sc,%sPara%s *para);\n",op->OutputType,op->Name);
    HPP.PrintF(L"\n");
    sFORALL(op->Parameters,para)
    {
      if(para->Flags & PF_Anim)
      {
        HPP.PrintF(L"  struct ScriptSymbol *_%s;\n",para->Symbol);
        /*
        if(para->Type==TYPE_COLOR)
        {
          HPP.PrintF(L"  sF32 *Ptr_%s;\n",para->Symbol);
        }
        */
      }
    }
    HPP.PrintF(L"};\n");
    HPP.PrintF(L"\n");

    CPP.PrintF(L"void %sAnim%s::Init(class ScriptContext *sc)\n",op->OutputType,op->Name);
    CPP.PrintF(L"{\n");
    sFORALL(op->Parameters,para)
      if(para->Flags & PF_Anim)
        CPP.PrintF(L"  _%s = sc->AddSymbol(L%q);\n",para->Symbol,para->ScriptName());
    CPP.PrintF(L"};\n");
    CPP.PrintF(L"\n");

    CPP.PrintF(L"void %sAnim%s::Bind(class ScriptContext *sc,%sPara%s *para)\n",op->OutputType,op->Name,op->OutputType,op->Name);
    CPP.PrintF(L"{\n");
    sFORALL(op->Parameters,para)
    {
      if(para->Flags & PF_Anim)
      {
        if(para->CType==L"sVector30")
        {
          CPP.PrintF(L"  sc->BindLocalFloat(_%s,3,&para->%s.x);\n",para->Symbol,para->Symbol);
        }
        else if(para->CType==L"sVector31")
        {
          CPP.PrintF(L"  sc->BindLocalFloat(_%s,3,&para->%s.x);\n",para->Symbol,para->Symbol);
        }
        else if(para->CType==L"sVector4")
        {
          CPP.PrintF(L"  sc->BindLocalFloat(_%s,4,&para->%s.x);\n",para->Symbol,para->Symbol);
        }
        else if(para->Type==TYPE_INT || para->Type==TYPE_FLAGS  || para->Type==TYPE_RADIO)
        {
          if(para->Count>1)
            CPP.PrintF(L"  sc->BindLocalInt(_%s,%d,para->%s);\n",para->Symbol,para->Count,para->Symbol);
          else
            CPP.PrintF(L"  sc->BindLocalInt(_%s,1,&para->%s);\n",para->Symbol,para->Symbol);
        }
        else if(para->Type==TYPE_FLOAT)
        {
          if(para->Count>1)
            CPP.PrintF(L"  sc->BindLocalFloat(_%s,%d,para->%s);\n",para->Symbol,para->Count,para->Symbol);
          else
            CPP.PrintF(L"  sc->BindLocalFloat(_%s,1,&para->%s);\n",para->Symbol,para->Symbol);
        }
        else if(para->Type==TYPE_COLOR)
        {
          CPP.PrintF(L"  sc->BindLocalColor(_%s,&para->%s);\n",para->Symbol,para->Symbol,para->Symbol);
        }
      }
    }
    CPP.PrintF(L"};\n");
    CPP.PrintF(L"\n");
/*
    CPP.PrintF(L"void %sAnim%s::UnBind(class ScriptContext *sc,%sPara%s *para)\n",op->OutputType,op->Name,op->OutputType,op->Name);
    CPP.PrintF(L"{\n");
    sFORALL(op->Parameters,para)
    {
      if(para->Flags & PF_Anim)
      {
        if(para->Type==TYPE_COLOR)
        {
          CPP.PrintF(L"  para->%s = sMakeColorRGBA(Ptr_%s);\n",para->Symbol,para->Symbol);
        }
      }
    }
    CPP.PrintF(L"};\n");
    CPP.PrintF(L"\n");
    */
  }
}

void Document::OutputMain()
{
  Op *op;
  Type *type;
  Input *in;
  AnimInfo *ai;

  Sep(HPP);
  HPP.PrintF(L"void AddTypes_%s(sBool secondary=0);\n",ProjectName);
  HPP.PrintF(L"void AddOps_%s(sBool secondary=0);\n",ProjectName);

  Sep(CPP);
  CPP.PrintF(L"void AddTypes_%s(sBool secondary)\n",ProjectName);
  CPP.Print(L"{\n");
  CPP.Print(L"  sVERIFY(Doc);\n");
  CPP.Print(L"\n");
  sFORALL(Types,type)
  {
    CPP.PrintF(L"  Doc->Types.AddTail(%sType = new %sType_);\n",type->Symbol,type->Symbol);
//    CPP.PrintF(L"  %sType->Init();\n",type->Symbol);  // now done in constructor 
    CPP.PrintF(L"  %sType->Secondary = secondary;\n",type->Symbol);
    for(sInt i=0;i<sCOUNTOF(type->ColumnHeaders);i++)
      if(!type->ColumnHeaders[i].IsEmpty())
        CPP.PrintF(L"  %sType->ColumnHeaders[%d] = L%q;\n",type->Symbol,i,type->ColumnHeaders[i]);
    CPP.Print(L"\n");
  }

  CPP.Print(L"}\n");

  Sep(CPP);
  CPP.PrintF(L"void AddOps_%s(sBool secondary)\n",ProjectName);
  CPP.Print(L"{\n");
  CPP.Print(L"  sVERIFY(Doc);\n");
  CPP.Print(L"\n");
  CPP.Print(L"  wClass sUNUSED *cl=0; cl;\n");
  CPP.Print(L"  wClassInputInfo sUNUSED *in=0; in;\n");
  CPP.Print(L"\n");

  CPP.Print(L"\n");


  sFORALL(Ops,op)
  {
    CPP.Print(L"\n");
    CPP.Print (L"  cl= new wClass;\n");
    CPP.PrintF(L"  cl->Name = L%q;\n",op->Name);
    CPP.PrintF(L"  cl->Label = L%q;\n",op->Label);
    CPP.PrintF(L"  cl->OutputType = %sType;\n",op->OutputType);
    CPP.PrintF(L"  cl->TabType = %sType;\n",op->TabType.IsEmpty() ? op->OutputType : op->TabType);
    if(op->Code)
      CPP.PrintF(L"  cl->Command = %sCmd%s;\n",op->OutputType,op->Name);
    CPP.PrintF(L"  cl->MakeGui = %sGui%s;\n",op->OutputType,op->Name);
    CPP.PrintF(L"  cl->SetDefaults = %sDef%s;\n",op->OutputType,op->Name);
    CPP.PrintF(L"  cl->BindPara = %sBind%s;\n",op->OutputType,op->Name);
    CPP.PrintF(L"  cl->Bind2Para = %sBind2%s;\n",op->OutputType,op->Name);
    CPP.PrintF(L"  cl->Bind3Para = %sBind3%s;\n",op->OutputType,op->Name);
    CPP.PrintF(L"  cl->WikiText = %sWiki%s;\n",op->OutputType,op->Name);
    if(op->Handles)
      CPP.PrintF(L"  cl->Handles = %sHnd%s;\n",op->OutputType,op->Name);
    if(op->SetDefaultsArray || op->ArrayParam.GetCount())
      CPP.PrintF(L"  cl->SetDefaultsArray = %sArr%s;\n",op->OutputType,op->Name);
    if(op->Actions)
      CPP.PrintF(L"  cl->Actions = %sAct%s;\n",op->OutputType,op->Name);
    if(op->SpecialDrag)
      CPP.PrintF(L"  cl->SpecialDrag = %sDrag%s;\n",op->OutputType,op->Name);
    if(op->Description)
      CPP.PrintF(L"  cl->GetDescription = %sDescription%s;\n",op->OutputType,op->Name);
    if(op->MaxArrayOffset>0)
      CPP.PrintF(L"  cl->ArrayCount = %d;\n",op->MaxArrayOffset);
    if(op->MaxStrings>0)
      CPP.PrintF(L"  cl->ParaStrings = %d;\n",op->MaxStrings);
    if(op->MaxOffset>0)
      CPP.PrintF(L"  cl->ParaWords = %d;\n",op->MaxOffset);
    if(op->FileInMask!=0)
      CPP.PrintF(L"  cl->FileInMask = %d;\n",op->FileInMask);
    if(op->FileOutMask!=0)
      CPP.PrintF(L"  cl->FileOutMask = %d;\n",op->FileOutMask);
    if(!op->FileInFilter.IsEmpty())
      CPP.PrintF(L"  cl->FileInFilter = L%q;\n",op->FileInFilter);
    if(op->Helper)
      CPP.PrintF(L"  cl->HelperWords = (sizeof(%sHelper%s)+3)/4;\n",op->OutputType,op->Name);
    if(op->Shortcut)
      CPP.PrintF(L"  cl->Shortcut = '%c';\n",op->Shortcut);
    if(op->Column)
      CPP.PrintF(L"  cl->Column = %d;\n",op->Column);
    if(op->GridColumns)
      CPP.PrintF(L"  cl->GridColumns = %d;\n",op->GridColumns);
    CPP.PrintF(L"  cl->Flags = 0x%04x;\n",op->Flags);
    if(!op->Extract.IsEmpty())
      CPP.PrintF(L"  cl->Extract = L\"%s\";\n",op->Extract);
    if(!op->CustomEd.IsEmpty())
      CPP.PrintF(L"  cl->CustomEd = %sCed%s;\n",op->OutputType,op->Name);
    if(op->Inputs.GetCount()>0)
      CPP.PrintF(L"  in = cl->Inputs.AddMany(%d);\n",op->Inputs.GetCount());
    sFORALL(op->Inputs,in)
    {
      CPP.PrintF(L"  in[%d].Type = %sType;\n",_i,in->Type);
      if(in->DefaultOpName.IsEmpty())
        CPP.PrintF(L"  in[%d].DefaultClass = 0;\n",_i);
      else
        CPP.PrintF(L"  in[%d].DefaultClass = Doc->FindClass(L\"%s\",L\"%s\");\n",_i,in->DefaultOpName,in->DefaultOpType);
      if(!in->GuiSymbol.IsEmpty())
        CPP.PrintF(L"  in[%d].Name = L\"%s\";\n",_i,in->GuiSymbol);
      CPP.PrintF(L"  in[%d].Flags = 0",_i);
      if(in->InputFlags & IF_OPTIONAL)
        CPP.PrintF(L"|wCIF_OPTIONAL");
      if(in->InputFlags & IF_WEAK)
        CPP.PrintF(L"|wCIF_WEAK");
      switch(in->Method)
      {
        case IE_LINK: CPP.PrintF(L"|wCIF_METHODLINK"); break;
        case IE_BOTH: CPP.PrintF(L"|wCIF_METHODBOTH"); break;
        case IE_CHOOSE: CPP.PrintF(L"|wCIF_METHODCHOOSE"); break;
        case IE_ANIM: CPP.PrintF(L"|wCIF_METHODANIM"); break;
      }
      CPP.Print(L";\n");
    }
    sFORALL(op->AnimInfos,ai)
      CPP.PrintF(L"  cl->Anims.AddMany(1)->Init(%d,L%q);\n",ai->Flags,ai->Name);
    ActionInfo *aci;
    sFORALL(op->ActionInfos,aci)
      CPP.PrintF(L"  cl->ActionIds.AddMany(1)->Init(L%q,%d);\n",aci->Name,aci->Id);

    CPP.Print(L"  Doc->Classes.AddTail(cl);\n");
  }

  CPP.Print(L"\n");
  CPP.Print(L"}\n");

  CPP.Print(L"\n");
  CPP.PrintF(L"// sADDSUBSYSTEM(%s,0x%03x,AddOps_%s,0);\n",ProjectName,Priority+0x100,ProjectName);
  CPP.Print(L"\n");
}

/****************************************************************************/

void CodeBlock::Output(sTextBuffer &tb,const sChar *file,sBool semi)
{
  tb.PrintF(L"#line %d \"%p\"\n",Line,(const sChar *)Doc->InputFileName);
  tb.Print(Code);
  if(semi)
    tb.Print(L";");
  tb.PrintF(L"\n#line %d \"%p\"\n",sCountChar(tb.Get(),'\n')+2,file);
}

/****************************************************************************/

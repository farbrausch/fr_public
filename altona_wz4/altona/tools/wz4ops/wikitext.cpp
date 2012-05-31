/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "doc.hpp"

/****************************************************************************/

void PrintPara(Parameter *para,sTextBuffer &tb,sBool &header);


void MakeWikiText(Op *op,sTextBuffer &tb)
{
  Input *in;
  Parameter *para;

  tb.PrintF(L"= %s : %s\n\n",op->OutputType,op->Name);

  tb.Print (L"!t 2 : 2 3\n");
  tb.Print (L" !T 2\n");

  tb.Print (L"  !i Name\n");
  tb.PrintF(L"  !i %s\n",op->Name);
  tb.Print (L"\n");
  tb.Print (L"  !i Output Type\n");
  tb.PrintF(L"  !i %s\n",op->OutputType);
  tb.Print (L" !i\n");
  tb.Print (L"\n");

  // inputs

  if(op->Inputs.GetCount()>0)
  {
    tb.PrintF(L"== Inputs\n\n");
    tb.Print (L"!T 3 : 1 1 3\n");
    tb.Print (L" !\n");
    tb.Print (L"  ! bc #c0c0c0\n");
    tb.Print (L"  !i Type\n");
    tb.Print (L"  !i Flags\n");
    tb.Print (L"  !i Comment\n");

    sFORALL(op->Inputs,in)
    {
      tb.PrintF(L" !i %s\n",in->Type);
      tb.Print (L" !i");
      if((op->Flags & 0x000001) && _i==op->Inputs.GetCount()-1)
        tb.Print (L" multiple");
      if(in->InputFlags & IF_WEAK)
        tb.Print (L" weak");
      if(in->InputFlags & IF_OPTIONAL)
        tb.Print (L" optional");
      if(in->Method != IE_INPUT)
        tb.PrintF(L" link %s",in->GuiSymbol);
      if(!in->DefaultOpName.IsEmpty())
        tb.PrintF(L" defaults to: %s",in->DefaultOpName);
      tb.Print (L"\n");
      tb.Print (L" !i *\n");
      tb.Print (L"\n");
    }
  }

  // parameters

  if(op->Parameters.GetCount()>0)
  {
    tb.PrintF(L"== Parameters\n\n");
    tb.Print (L"!T 4 : 1 1 1 2\n");
    tb.Print (L" !\n");
    tb.Print (L"  ! bc #c0c0c0\n");
    tb.Print (L"  !i Screen Name\n");
    tb.Print (L"  !i Script Name\n");
    tb.Print (L"  !i Type\n");
    tb.Print (L"  !i Comment\n");
    tb.Print (L"\n");
    sBool header=0;
    sFORALL(op->Parameters,para)
    {
      PrintPara(para,tb,header);
    }
  }

  // array

  if(op->ArrayParam.GetCount()>0)
  {
    tb.PrintF(L"== Array Parameters\n\n");
    tb.Print (L"!T 4 : 1 1 1 2\n");
    tb.Print (L" !\n");
    tb.Print (L"  ! bc #c0c0c0\n");
    tb.Print (L"  !i Screen Name\n");
    tb.Print (L"  !i Script Name\n");
    tb.Print (L"  !i Type\n");
    tb.Print (L"  !i Comment\n");
    tb.Print (L"\n");
    sBool header=0;
    sFORALL(op->Parameters,para)
    {
      PrintPara(para,tb,header);
    }
  }

  tb.PrintF(L"== Comments\n\n*\n\n");

  if(op->SpecialDrag)
    tb.PrintF(L"== Mouse Dragging\n\n\n");

  tb.PrintF(L"== See Also\n\n\n");
}

/****************************************************************************/

void PrintPara(Parameter *para,sTextBuffer &tb,sBool &header)
{
  if(header || para->Type==TYPE_FLAGS  || para->Type==TYPE_RADIO)
  {
    tb.Print (L"!T 4 : 1 1 1 2\n");
    header = 0;
  }
  static const sChar *oldlabel = 0;
  const sChar *label = para->Label.IsEmpty() ? para->Symbol : para->Label;
  const sChar *symbol = para->Symbol;
  switch(para->Type)
  {
  default:
    break;
  case TYPE_LABEL:
    oldlabel = label;
    return;
  case TYPE_INT: 
    tb.PrintF(L" !i %s\n",label);
    tb.PrintF(L" !i %s\n",symbol);
    tb.Print (L" !i int"); 
    if(para->Count>1)
      tb.PrintF(L"[%d]",para->Count);
    tb.Print (L"\n !i *\n\n");
    break;
  case TYPE_FLOAT: 
    tb.PrintF(L" !i %s\n",label);
    tb.PrintF(L" !i %s\n",symbol);
    tb.Print (L" !i float"); 
    if(para->Count>1)
      tb.PrintF(L"[%d]",para->Count);
    tb.Print (L"\n !i *\n\n");
    break;
  case TYPE_COLOR:
    tb.PrintF(L" !i %s\n",label);
    tb.PrintF(L" !i %s\n",symbol);
    tb.Print (L" !i color"); 
    if(para->Count>1)
      tb.PrintF(L"[%d]",para->Count);
    tb.Print (L"\n !i *\n\n");
    break;
  case TYPE_RADIO:
  case TYPE_FLAGS:
    {
      const sChar *label = para->Label.IsEmpty() ? oldlabel : (const sChar *)para->Label;
      const sChar *s = para->Options;
      sBool spacer = 0;
      while(*s)
      {
        if(*s=='*')
        {
          s++;
          while(sIsDigit(*s)) *s++;
        }
        if(spacer)
        {
          tb.Print (L" !i\n");
          tb.Print (L" !i\n");
          tb.Print (L" !i\n");
          tb.Print (L" !i\n");
          tb.Print (L"\n");
        }
        spacer = 1;
        while(*s!=0 && *s!=':')
        {
          while(*s=='|') *s++;
          while(sIsDigit(*s)) *s++;
          while(sIsSpace(*s)) *s++;
          sInt i = 0;
          while(s[i]!='|' && s[i]!=':' && s[i]!=0) i++;
          if(*s!='-')
          {
            tb.PrintF(L" !i %s\n",label); label=L"";
            tb.PrintF(L" !i %s\n",symbol); symbol=L"";
            tb.Print (L" !i ");
            tb.Print (s,i); 
            tb.Print (L"\n !i *\n");
          }
          s+=i;
        }
        if(*s==':')
          s++;
      }
      header = 1;
    }
    break;
  case TYPE_STRING:
    tb.PrintF(L" !i %s\n",label);
    tb.PrintF(L" !i %s\n",symbol);
    tb.Print (L" !i string\n"); 
    tb.Print (L" !i *\n\n");
    break;
  case TYPE_FILEIN:
    tb.PrintF(L" !i %s\n",label);
    tb.PrintF(L" !i %s\n",symbol);
    tb.Print (L" !i load file\n"); 
    tb.Print (L" !i *\n\n");
    break;
  case TYPE_FILEOUT:
    tb.PrintF(L" !i %s\n",label);
    tb.PrintF(L" !i %s\n",symbol);
    tb.Print (L" !i save file\n"); 
    tb.Print (L" !i *\n\n");
    break;
  case TYPE_LINK:
    tb.PrintF(L" !i %s\n",label);
    tb.PrintF(L" !i %s\n",symbol);
    tb.Print (L" !i link"); 
    if(para->Count>1)
      tb.PrintF(L"[%d]",para->Count);
    tb.Print (L"\n !i *\n\n");
    break;
  case TYPE_STROBE:
    tb.PrintF(L" !i %s\n",label);
    tb.PrintF(L" !i %s\n",symbol);
    tb.Print (L" !i strobe\n"); 
    tb.Print (L" !i *\n\n");
    break;
  case TYPE_ACTION:
    tb.PrintF(L" !i %s\n",label);
    tb.PrintF(L" !i %s\n",symbol);
    tb.Print (L" !i action\n"); 
    tb.Print (L" !i *\n\n");
    break;
  case TYPE_CUSTOM:
    tb.PrintF(L" !i %s\n",label);
    tb.PrintF(L" !i %s\n",symbol);
    tb.Print (L" !i custom\n"); 
    tb.Print (L" !i *\n\n");
    break;
  }
  oldlabel = 0;
}

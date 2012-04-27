/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "gui/listwindow.hpp"

/****************************************************************************/

void sListWindow2Field::PaintField(sStaticListWindow *lw,const sRect &client,sObject *obj,sInt line,sInt select)
{
  lw->PaintField(client,select,L"unhandled column");
}

void sListWindow2Field::EditField(sStaticListWindow *lw,const sRect &rect,sObject *obj,sInt line)
{
}

sInt sListWindow2Field::CompareField(sStaticListWindow *lw,sObject *o0,sObject *o1,sInt line0,sInt line1)
{
  return -2;    // -2 is special valie for "not handled"
}

/****************************************************************************/

class sListWindow2Field_Int : public sListWindow2Field
{
public:
  sInt Min;
  sInt Max;
  sF32 Step;

  sListWindow2Field_Int() 
  {
    Min = 0;
    Max = 0;
    Step = 0.25f;
  }

  void PaintField(sStaticListWindow *lw,const sRect &client,sObject *obj,sInt line,sInt select)
  {
    sString<64> buffer;
    lw->AddNotify(Ref.Ref<sInt>(obj));
    sSPrintF(buffer,L"%d",Ref.Ref<sInt>(obj));
    lw->PaintField(client,select,buffer);
  }

  void EditField(sStaticListWindow *lw,const sRect &rect,sObject *obj,sInt line)
  {
    sControl *sc = new sIntControl(&Ref.Ref<sInt>(obj),sInt(Min),sInt(Max),Step);
    sc->ChangeMsg = lw->ChangeMsg;
    sc->DoneMsg = lw->DoneMsg;
    sc->Outer = rect;
    sc->Flags |= sWF_AUTOKILL;
    sGui->AddWindow(sc);
    sGui->SetFocus(sc);
  }

  sInt CompareField(sStaticListWindow *lw,sObject *o0,sObject *o1,sInt line0,sInt line1)
  {
    sInt i0 = Ref.Ref<sInt>(o0);
    sInt i1 = Ref.Ref<sInt>(o1);
    if(i0<i1) return -1;
    if(i0>i1) return 1;
    return 0;
  }
};

class sListWindow2Field_Float : public sListWindow2Field
{
public:
  sF32 Min;
  sF32 Max;
  sF32 Step;

  sListWindow2Field_Float() 
  {
    Min = 0;
    Max = 0;
    Step = 0.25f;
  }

  void PaintField(sStaticListWindow *lw,const sRect &client,sObject *obj,sInt line,sInt select)
  {
    sString<64> buffer;
    lw->AddNotify(Ref.Ref<sF32>(obj));
    sSPrintF(buffer,L"%f",Ref.Ref<sF32>(obj));
    lw->PaintField(client,select,buffer);
  }

  void EditField(sStaticListWindow *lw,const sRect &rect,sObject *obj,sInt line)
  {
    sControl *sc = new sFloatControl(&Ref.Ref<sF32>(obj),Min,Max,Step);
    sc->ChangeMsg = lw->ChangeMsg;
    sc->DoneMsg = lw->DoneMsg;
    sc->Outer = rect;
    sc->Flags |= sWF_AUTOKILL;
    sGui->AddWindow(sc);
    sGui->SetFocus(sc);
  }
};

class sListWindow2Field_String : public sListWindow2Field
{
public:
  void PaintField(sStaticListWindow *lw,const sRect &client,sObject *obj,sInt line,sInt select)
  {
    const sChar *str;
    str = &Ref.Ref<sChar>(obj);
    lw->AddNotify(str,sizeof(sChar)*Size);
    lw->PaintField(client,select,str);
  }

  void EditField(sStaticListWindow *lw,const sRect &rect,sObject *obj,sInt line)
  {
    sControl *sc = new sStringControl(sStringDesc(&Ref.Ref<sChar>(obj),Size));
    sc->ChangeMsg = lw->ChangeMsg;
    sc->DoneMsg = lw->DoneMsg;
    sc->Outer = rect;
    sc->Flags |= sWF_AUTOKILL;
    sGui->AddWindow(sc);
    sGui->SetFocus(sc);
  }

  sInt CompareField(sStaticListWindow *lw,sObject *o0,sObject *o1,sInt line0,sInt line1)
  {
    const sChar *str0 = &Ref.Ref<sChar>(o0);
    const sChar *str1 = &Ref.Ref<sChar>(o1);
    return sCmpStringI(str0,str1);
  }
};

class sListWindow2Field_PoolString : public sListWindow2Field
{
public:
  void PaintField(sStaticListWindow *lw,const sRect &client,sObject *obj,sInt line,sInt select)
  {
    lw->AddNotify(Ref.Ref<sPoolString>(obj));
    lw->PaintField(client,select,Ref.Ref<sPoolString>(obj));
  }

  void EditField(sStaticListWindow *lw,const sRect &rect,sObject *obj,sInt line)
  {
    sControl *sc = new sStringControl(&Ref.Ref<sPoolString>(obj));
    sc->ChangeMsg = lw->ChangeMsg;
    sc->DoneMsg = lw->DoneMsg;
    sc->Outer = rect;
    sc->Flags |= sWF_AUTOKILL;
    sGui->AddWindow(sc);
    sGui->SetFocus(sc);
  }

  sInt CompareField(sStaticListWindow *lw,sObject *o0,sObject *o1,sInt line0,sInt line1)
  {
    sPoolString *str0 = &Ref.Ref<sPoolString>(o0);
    sPoolString *str1 = &Ref.Ref<sPoolString>(o1);
    return sCmpStringI(*str0,*str1);
  }
};

class sListWindow2Field_Choice : public sListWindow2Field
{
public:
  void PaintField(sStaticListWindow *lw,const sRect &client,sObject *obj,sInt line,sInt select)
  {
    sString<64> buffer;
    lw->AddNotify(Ref.Ref<sInt>(obj));
    sMakeChoice(buffer,Choices,Ref.Ref<sInt>(obj));
    lw->PaintField(client,select,buffer);
  }

  void EditField(sStaticListWindow *lw,const sRect &rect,sObject *obj,sInt line)
  {
    sInt n = sCountChoice(Choices);
    if(n==2)
    {
      Ref.Ref<sInt>(obj) = !Ref.Ref<sInt>(obj);
      lw->Update();
    }
    else if(n>2)
    {
      sChoiceControl *cc = new sChoiceControl(Choices,&Ref.Ref<sInt>(obj));
      cc->ChangeMsg = lw->ChangeMsg;
      cc->DoneMsg = lw->DoneMsg;
      cc->FakeDropdown(rect.x0,rect.y1);
      lw->ChoiceDropper = cc;
      // throw away the control, we just need the dropdown list!
    }
  }

  sInt CompareField(sStaticListWindow *lw,sObject *o0,sObject *o1,sInt line0,sInt line1)
  {
    sInt i0 = Ref.Ref<sInt>(o0);
    sInt i1 = Ref.Ref<sInt>(o1);
    if(i0<i1) return -1;
    if(i0>i1) return 1;
    return 0;
  }
};

class sListWindow2Field_Color : public sListWindow2Field
{
public:
  void PaintField(sStaticListWindow *lw,const sRect &client,sObject *obj,sInt line,sInt select)
  {
    sString<64> buffer;
    lw->AddNotify(Ref.Ref<sU32>(obj));
    sSPrintF(buffer,L"%08x",Ref.Ref<sU32>(obj));
    lw->PaintField(client,select,buffer);
  }

  sInt CompareField(sStaticListWindow *lw,sObject *o0,sObject *o1,sInt line0,sInt line1)
  {
  return 0;
  }
};

class sListWindow2Field_Progress : public sListWindow2Field
{
  void PaintField(sStaticListWindow *lw,const sRect &client,sObject *obj,sInt line,sInt select)
  {
    lw->AddNotify(Ref.Ref<sF32>(obj));

    sRect rc = client;
    sRect2D(client,lw->BackColor);

    rc.Extend(-2);
    sRectFrame2D(rc,sGC_TEXT);
    
    rc.Extend(-1);
    rc.x1 = rc.x0 + sInt(Ref.Ref<sF32>(obj) * (rc.x1 - rc.x0));
    sRect2D(rc,sGC_BLUE);
  }
};

/****************************************************************************/

void sStaticListWindow::Construct()
{
  Height = 0; 
  DragHighlight = -1; 
  DragMode = 0;
  DragStart = -1;
  DragEnd = -1;
  Width = 0; 
  IsMulti = 0;
  Flags|=sWF_CLIENTCLIPPING;
  UpdateTreeInfo();
  IndentPixels = sGui->PropFont->GetWidth(L"nn");
  ChoiceDropper = 0;
  ListFlags = 0;
  DragSelectLine = -1;
  DragSelectOther = -1;
  TagArray = 1;
}

sStaticListWindow::~sStaticListWindow()
{
  sDeleteAll(Fields);
}

void sStaticListWindow::Tag()
{
  sWireClientWindow::Tag();
  if(Array && TagArray)
    sNeed(*Array);
  ChoiceDropper->Need();
}

void sStaticListWindow::InitWire(const sChar *name)
{
  sWireClientWindow::InitWire(name);
  sWire->AddDrag(name,L"SelectSingle",sMessage(this,&sStaticListWindow::DragSelectSingle));
  sWire->AddDrag(name,L"SelectMulti",sMessage(this,&sStaticListWindow::DragSelectMulti,0));
  sWire->AddDrag(name,L"SelectAddMulti",sMessage(this,&sStaticListWindow::DragSelectMulti,1));
  sWire->AddDrag(name,L"SelectToggleMulti",sMessage(this,&sStaticListWindow::DragSelectMulti,2));
  sWire->AddDrag(name,L"Edit",sMessage(this,&sStaticListWindow::DragEdit));
  sWire->AddKey(name,L"Delete",sMessage(this,&sStaticListWindow::CmdDelete,0));
  sWire->AddKey(name,L"DeleteReal",sMessage(this,&sStaticListWindow::CmdDelete,1));
  sWire->AddKey(name,L"SelUp",sMessage(this,&sStaticListWindow::CmdSelUp,0));
  sWire->AddKey(name,L"SelDown",sMessage(this,&sStaticListWindow::CmdSelDown,0));
  sWire->AddKey(name,L"SelExtendUp",sMessage(this,&sStaticListWindow::CmdSelUp,1));
  sWire->AddKey(name,L"SelExtendDown",sMessage(this,&sStaticListWindow::CmdSelDown,1));
  sWire->AddKey(name,L"SelPageUp",sMessage(this,&sStaticListWindow::CmdSelUp,2));
  sWire->AddKey(name,L"SelPageDown",sMessage(this,&sStaticListWindow::CmdSelDown,2));
  sWire->AddKey(name,L"SelExtendPageUp",sMessage(this,&sStaticListWindow::CmdSelUp,3));
  sWire->AddKey(name,L"SelExtendPageDown",sMessage(this,&sStaticListWindow::CmdSelDown,3));
  sWire->AddKey(name,L"SelectAll",sMessage(this,&sStaticListWindow::CmdSelectAll,1));
  sWire->AddKey(name,L"MoveUp",sMessage(this,&sStaticListWindow::CmdMoveUp,0));
  sWire->AddKey(name,L"MoveDown",sMessage(this,&sStaticListWindow::CmdMoveDown,0));
  sWire->AddKey(name,L"MovePageUp",sMessage(this,&sStaticListWindow::CmdMoveUp,2));
  sWire->AddKey(name,L"MovePageDown",sMessage(this,&sStaticListWindow::CmdMoveDown,2));
  sWire->AddKey(name,L"MoveLeft",sMessage(this,&sStaticListWindow::CmdMoveLeft));
  sWire->AddKey(name,L"MoveRight",sMessage(this,&sStaticListWindow::CmdMoveRight));
  sWire->AddKey(name,L"TreeOpen",sMessage(this,&sStaticListWindow::CmdOpenClose,0));
  sWire->AddKey(name,L"TreeClose",sMessage(this,&sStaticListWindow::CmdOpenClose,1));
  sWire->AddKey(name,L"TreeToggle",sMessage(this,&sStaticListWindow::CmdOpenClose,2));
}

void sStaticListWindow::AddHeader()
{
  AddBorderHead(new sListWindow2Header(this));
}

/****************************************************************************/

void sStaticListWindow::OnCalcSize()
{
  sInt y,yy;
  sObject *obj;
  sListWindowTreeInfo<sObject *> *ti;
  sListWindow2Column *column;
  sInt max = Columns.GetCount();
  IndentPixels = sGui->PropFont->GetWidth(L"nn");

  Height = sGui->PropFont->GetHeight()+2;
  if(max>0)
    ReqSizeX = Columns[max-1].Pos;
  else
    ReqSizeX = 0;
  ReqSizeY = 0;
  
  if(!Array) return;

  sFORALL(*Array,obj)
  {
    ti = GetTreeInfo(obj);
    if(ti && (ti->Flags & sLWTI_HIDDEN))
      continue;
    if(max==0)
    {
      y = Height;
    }
    else
    {
      y = 0;
      sFORALL(Columns,column)
      {
        yy = OnCalcFieldHeight(column->Field,obj);
        if(yy==0) yy = Height;
        y = sMax(y,yy);
      }
    }
    ReqSizeY += y;
  }
}

void sStaticListWindow::OnPaint2D()
{
  sInt y,yy;
  sObject *obj;
  sListWindow2Column *column;
  sRect r;
  sInt line;
  sInt ys;
  sInt indent;
  sListWindowTreeInfo<sObject *> *ti;

  Height = sGui->PropFont->GetHeight()+2;
  r.y0 = Client.y0;
  ys = 0;
  
  ClearNotify();

  if(Array)
  {
    sFORALL(*Array,obj)
    {
      line = _i;
      sInt select = GetSelectStatus(line);
      indent = 0;
      ti = GetTreeInfo(obj);
      if(ti) 
      {
        if(ti->Flags & sLWTI_HIDDEN)
          continue;
        indent = ti->Level+1;
      }

      // calc size

      y=0;
      sFORALL(Columns,column)
      {
        yy = OnCalcFieldHeight(column->Field,obj);
        if(yy==0) yy = Height;
        y = sMax(y,yy);
      }
      ys += y;
      BackColor = OnBackColor(line,select,obj);

      // draw 

      r.y1 = r.y0 + y;
      r.x0 = Client.x0;
      r.x1 = Client.x1;
      if(r.IsInside(Inner))
      {
        if(indent>0)
        {
          const sChar *text = L"";
          r.x1 = r.x0 + indent*IndentPixels;
          
          if(ti->FirstChild)
            text = (ti->Flags & sLWTI_CLOSED) ? L"+" : L"-";
          sGui->FixedFont->SetColor(sGC_TEXT,BackColor);
          sGui->FixedFont->Print(sF2P_OPAQUE|sF2P_RIGHT,r,text);
          r.x0 = r.x1;
        }
        sFORALL(Columns,column)
        {
          if(_i+1==Columns.GetCount()) 
            r.x1 = Client.x1;
          else
            r.x1 = Client.x0+column->Pos-1;
          if(!OnPaintField(r,column->Field,obj,line,select))
            PaintField(r,column->Field,obj,line,select);
          r.x0 = r.x1+1;
        }
      }
      r.y0 = r.y1;
    }
  }

  if(ys<Client.SizeY())
    sRect2D(Client.x0,Client.y0+ys,Client.x1,Client.y1,sGC_DOC);
  sFORALL(Columns,column)
  {
    if(_i+1!=Columns.GetCount())
    {
      sInt x = Client.x0 + column->Pos;
      sRect2D(x-1,Client.y0,x,Client.y1,(DragHighlight==_i)?sGC_SELECT:sGC_DRAW);
    }
  }

  if(ys!=ReqSizeY)
  {
    ReqSizeY = ys;
    Layout();
  }
}

void sStaticListWindow::OnDrag(const sWindowDrag &dd)
{
  sWireClientWindow::OnDrag(dd);

  // special mode: right clicks work as single selection
  // useful together with context menus

  if(dd.Mode==sDD_START && dd.Buttons==2 && (ListFlags & sLWF_RIGHTPICK))
  {
    // don't destroy multiple selections
    sInt count = 0;
    for(sInt i=0;i<Array->GetCount();i++)
      if(GetSelectStatus(i))
        count++;

    if(count<2)
    {
      sInt line;
      sInt column;
      sRect r;
      if(Hit(dd.MouseX,dd.MouseY,line,column,r))
      {
        ClearSelectStatus();
        SetSelectStatus(line,1);
      }
    }
  }
}

void sStaticListWindow::Sort(sListWindow2Field *f)
{
  sInt max = Array->GetCount();
  sInt dir = f->SortDir ? -1 : 1;


  for(sInt i=0;i<max-1;i++)
  {
    for(sInt j=i+1;j<max;j++)
    {
      sInt r = OnCompareField(f,(*Array)[i],(*Array)[j],i,j);
      if(r==-2)
        r = CompareField(f,(*Array)[i],(*Array)[j],i,j);
      if(r==dir)
        Array->Swap(i,j);
    }
  }
}

void sStaticListWindow::Sort()
{
  sListWindow2Field *f;
  for(sInt i=Fields.GetCount();i>0;i--)
  {
    sFORALL(Fields,f)
      if(f->SortOrder==i)
        Sort(f);
  }
}

void sStaticListWindow::ScrollTo(sInt index)
{
  sInt y,yy;
  sObject *obj;
  sRect r;
  sInt line;
  sInt ys;
  sInt indent;
  sListWindow2Column *column;
  sListWindowTreeInfo<sObject *> *ti;

  Height = sGui->PropFont->GetHeight()+2;
  r.y0 = Client.y0;
  ys = 0;
  
  if(Array)
  {
    sFORALL(*Array,obj)
    {
      line = _i;
//      sInt select = GetSelectStatus(line);
      indent = 0;
      ti = GetTreeInfo(obj);
      if(ti) 
      {
        if(ti->Flags & sLWTI_HIDDEN)
          continue;
        indent = ti->Level+1;
      }

      // calc size

      y=0;
      sFORALL(Columns,column)
      {
        yy = OnCalcFieldHeight(column->Field,obj);
        if(yy==0) yy = Height;
        y = sMax(y,yy);
      }
      ys += y;

      // draw 

      r.y1 = r.y0 + y;
      if(line==index)
      {
        r.x0 = ScrollX;
        r.x1 = ScrollX;
        sWindow::ScrollTo(r,1);
      }
      r.y0 = r.y1;
    }
  }
}

void sStaticListWindow::ScrollToItem(sObject *item)
{
  sInt index = sFindIndex(*Array,item);
  if(index != -1)
    ScrollTo(index);
}

/****************************************************************************/

sInt sStaticListWindow::OnBackColor(sInt line,sInt select,sObject *obj)
{
  return select ? sGC_SELECT : sGC_DOC;
}

sInt sStaticListWindow::OnCalcFieldHeight(sListWindow2Field *field,sObject *obj)
{
  return 0;
}

sBool sStaticListWindow::OnPaintField(const sRect &client,sListWindow2Field *field,sObject *obj,sInt line,sInt select)
{
  return 0;
}

void sStaticListWindow::PaintField(const sRect &client,sListWindow2Field *field,sObject *obj,sInt line,sInt select)
{
  field->PaintField(this,client,obj,line,select);
/*
  const sChar *str;
  sString<64> buffer;
  switch(field->Type)
  {
  case sLWT_STRING:
    str = &field->Ref.Ref<sChar>(obj);
    AddNotify(str,sizeof(sChar)*field->Size);
    PaintField(client,select,str);
    break;
  case sLWT_INT:
//    AddNotify(obj->*(field->IntRef));
//    sSPrintF(buffer,L"%d",obj->*(field->IntRef));
    AddNotify(field->Ref.Ref<sInt>(obj));
    sSPrintF(buffer,L"%d",field->Ref.Ref<sInt>(obj));
    PaintField(client,select,buffer);
    break;
  case sLWT_FLOAT:
    AddNotify(field->Ref.Ref<sF32>(obj));
    sSPrintF(buffer,L"%f",field->Ref.Ref<sF32>(obj));
    PaintField(client,select,buffer);
    break;
  case sLWT_COLOR:
    AddNotify(field->Ref.Ref<sU32>(obj));
    sSPrintF(buffer,L"%08x",field->Ref.Ref<sU32>(obj));
    PaintField(client,select,buffer);
    break;
  case sLWT_CHOICE:
    AddNotify(field->Ref.Ref<sInt>(obj));
    sMakeChoice(buffer,field->Choices,field->Ref.Ref<sInt>(obj));
    PaintField(client,select,buffer);
    break;
  case sLWT_POOLSTRING:
    AddNotify(field->Ref.Ref<sPoolString>(obj));
    PaintField(client,select,field->Ref.Ref<sPoolString>(obj));
    break;
  case sLWT_PROGRESS:
    AddNotify(field->Ref.Ref<sF32>(obj));
    {
      // paint a small progress bar
      sRect rc = client;
      sRect2D(client,BackColor);

      rc.Extend(-2);
      sRectFrame2D(rc,sGC_TEXT);
      
      rc.Extend(-1);
      rc.x1 = rc.x0 + sInt(field->Ref.Ref<sF32>(obj) * (rc.x1 - rc.x0));
      sRect2D(rc,sGC_BLUE);
    }
    break;
  default:
    PaintField(client,select,L"unhandled column");
    break;
  }
*/
}

void sStaticListWindow::PaintField(const sRect &client,sInt select,const sChar *text)
{
  sGui->PropFont->SetColor(sGC_TEXT,BackColor);
  sGui->PropFont->Print(sF2P_OPAQUE|sF2P_SPACE|sF2P_LEFT,client,text);
}

/****************************************************************************/

sBool sStaticListWindow::OnEditField(const sRect &rect,sListWindow2Field *field,sObject *,sInt line)
{
  return 0;
}

void sStaticListWindow::EditField(const sRect &rect,sListWindow2Field *field,sObject *obj,sInt line)
{
  if(field->Flags & sLWF_EDIT)
    field->EditField(this,rect,obj,line);
/*
  return;


  sControl *sc;
  sc = 0;
  switch(field->Type)
  {
  case sLWT_STRING:
    sc = new sStringControl(sStringDesc(&field->Ref.Ref<sChar>(obj),field->Size));
    break;

  case sLWT_POOLSTRING:
    sc = new sStringControl(&field->Ref.Ref<sPoolString>(obj));
    break;

  case sLWT_INT:
    sc = new sIntControl(&field->Ref.Ref<sInt>(obj),sInt(field->Min),sInt(field->Max),field->Step);
    break;

  case sLWT_FLOAT:
    sc = new sFloatControl(&field->Ref.Ref<sF32>(obj),field->Min,field->Max,field->Step);
    break;

  case sLWT_CHOICE:    
    {
      sInt n = sCountChoice(field->Choices);
      if(n==2)
      {
        field->Ref.Ref<sInt>(obj) = !field->Ref.Ref<sInt>(obj);
        Update();
      }
      else if(n>2)
      {
        sChoiceControl *cc = new sChoiceControl(field->Choices,&field->Ref.Ref<sInt>(obj));
        cc->ChangeMsg = ChangeMsg;
        cc->DoneMsg = DoneMsg;
        cc->FakeDropdown(rect.x0,rect.y1);
        ChoiceDropper = cc;
        // throw away the control, we just need the dropdown list!
      }
    }
    break;
  }
  if(sc)
  {
    sc->ChangeMsg = ChangeMsg;
    sc->DoneMsg = DoneMsg;
    sc->Outer = rect;
    sc->Flags |= sWF_AUTOKILL;
    sGui->AddWindow(sc);
    sGui->SetFocus(sc);
  }
  */
}

/****************************************************************************/

sInt sStaticListWindow::OnCompareField(sListWindow2Field *field,sObject *o0,sObject *o1,sInt line0,sInt line1)
{
  return -2;    // -2 is special valie for "not handled"
}

sInt sStaticListWindow::CompareField(sListWindow2Field *field,sObject *o0,sObject *o1,sInt line0,sInt line1)
{
  return field->CompareField(this,o0,o1,line0,line1);
  /*
  const sChar *str0,*str1;
  sPoolString p0,p1;
  sInt i0,i1;
  sU32 u0,u1;
  sF32 f0,f1;
  switch(field->Type)
  {
  case sLWT_STRING:
    str0 = &field->Ref.Ref<sChar>(o0);
    str1 = &field->Ref.Ref<sChar>(o1);
    return sCmpStringI(str0,str1);
  case sLWT_CHOICE:
  case sLWT_INT:
    i0 = field->Ref.Ref<sInt>(o0);
    i1 = field->Ref.Ref<sInt>(o1);
    if(i0<i1) return -1;
    if(i0>i1) return 1;
    return 0;
  case sLWT_FLOAT:
  case sLWT_PROGRESS:
    f0 = field->Ref.Ref<sF32>(o0);
    f1 = field->Ref.Ref<sF32>(o1);
    if(f0<f1) return -1;
    if(f0>f1) return 1;
    return 0;
  case sLWT_COLOR:
    u0 = field->Ref.Ref<sU32>(o0);
    u1 = field->Ref.Ref<sU32>(o1);
    i0 = ((u0>>16)&0xff) + ((u0>>8)&0xff) + ((u0)&0xff);
    i1 = ((u1>>16)&0xff) + ((u1>>8)&0xff) + ((u1)&0xff);
    if(i0<i1) return -1;
    if(i0>i1) return 1;
    return 0;
  case sLWT_POOLSTRING:
    p0 = field->Ref.Ref<sPoolString>(o0);
    p1 = field->Ref.Ref<sPoolString>(o1);
    return sCmpStringI(p0,p1);
  default:
    return 0;
  }
  */
}

/****************************************************************************/

void sStaticListWindow::AddField(sListWindow2Field *field,sInt width)
{
  Fields.AddTail(field);
  if(width)
    AddColumn(field,width);
}

sListWindow2Field *sStaticListWindow::AddField(const sChar *label,sInt flags,sInt width,sInt id,sInt size)
{
  sListWindow2Field *field;

  field = new sListWindow2Field;
  field->Id = id;
  field->Flags = flags;
  field->Size = size;
  field->Label = label;
  Fields.AddTail(field);

  if(width) AddColumn(field,width);

  return field;
}

sListWindow2Field *sStaticListWindow::AddField(const sChar *label,sInt flags,sInt width,sMemberPtr<sInt> ref,sInt min,sInt max,sF32 step)
{
  sListWindow2Field_Int *field;

  field = new sListWindow2Field_Int;
  field->Ref.Offset = ref.Offset;
  field->Flags = flags;
  field->Min = min;
  field->Max = max;
  field->Step = step;
  field->Size = 0;
  field->Label = label;
  Fields.AddTail(field);

  if(width) AddColumn(field,width);

  return field;
}

sListWindow2Field *sStaticListWindow::AddFieldChoice(const sChar *label,sInt flags,sInt width,sMemberPtr<sInt> ref,const sChar *choices)
{
  sListWindow2Field_Choice *field;

  field = new sListWindow2Field_Choice;
  field->Ref.Offset = ref.Offset;
  field->Flags = flags;
  field->Size = 0;
  field->Label = label;
  field->Choices = choices;
  Fields.AddTail(field);

  if(width) AddColumn(field,width);

  return field;
}

sListWindow2Field *sStaticListWindow::AddFieldProgress(const sChar *label,sInt flags,sInt width,sMemberPtr<sF32> ref)
{
  sListWindow2Field_Progress *field;

  field = new sListWindow2Field_Progress;
  field->Ref.Offset = ref.Offset;
  field->Flags = flags;
  field->Size = 0;
  field->Label = label;
  Fields.AddTail(field);

  if(width) AddColumn(field,width);

  return field;
}


sListWindow2Field *sStaticListWindow::AddField(const sChar *label,sInt flags,sInt width,sMemberPtr<sF32> ref,sF32 min,sF32 max,sF32 step)
{
  sListWindow2Field_Float *field;

  field = new sListWindow2Field_Float;
  field->Ref.Offset = ref.Offset;
  field->Flags = flags;
  field->Min = min;
  field->Max = max;
  field->Step = step;
  field->Size = 0;
  field->Label = label;
  Fields.AddTail(field);

  if(width) AddColumn(field,width);

  return field;
}

sListWindow2Field *sStaticListWindow::AddFieldColor(const sChar *label,sInt flags,sInt width,sMemberPtr<sU32> ref)
{
  sListWindow2Field_Color *field;

  field = new sListWindow2Field_Color;
  field->Ref.Offset = ref.Offset;
  field->Flags = flags;
  field->Size = 0;
  field->Label = label;
  Fields.AddTail(field);

  if(width) AddColumn(field,width);

  return field;
}



sListWindow2Field *sStaticListWindow::AddField(const sChar *label,sInt flags,sInt width,sMemberPtr<sChar> ref,sInt size)
{
  sListWindow2Field_String *field;

  field = new sListWindow2Field_String;
  field->Ref.Offset = ref.Offset;
  field->Flags = flags;
  field->Size = size;
  field->Label = label;
  Fields.AddTail(field);

  if(width) AddColumn(field,width);

  return field;
}
/*
sListWindow2Field *sStaticListWindow::AddField(const sChar *label,sInt flags,sInt width,const sChar *sObject::*ref)
{
  sListWindow2Field *field;

  field = new sListWindow2Field;
  field->Type = sLWT_CONSTSTRING;
  field->Ref.Offset = ref.Offset;
  field->Flags = flags;
  field->Size = 0;
  field->Label = label;
  Fields.AddTail(field);

  if(width) AddColumn(field,width);

  return field;
}
*/
sListWindow2Field *sStaticListWindow::AddField(const sChar *label,sInt flags,sInt width,sMemberPtr<sPoolString> ref)
{
  sListWindow2Field_PoolString *field;

  field = new sListWindow2Field_PoolString;
  field->Ref.Offset = ref.Offset;
  field->Flags = flags;
  field->Size = 0;
  field->Label = label;
  Fields.AddTail(field);

  if(width) AddColumn(field,width);

  return field;
}

void sStaticListWindow::AddColumn(sListWindow2Field *field,sInt width)
{
  sListWindow2Column *column;

  column = Columns.AddMany(1);
  column->Field = field;
  Width += width;
  column->Pos = Width;
}

/****************************************************************************/
/****************************************************************************/

sBool sStaticListWindow::Hit(sInt mx,sInt my,sInt &line,sInt &col,sRect &rect)
{
  sInt y,yy,l;
  sRect r;
  sObject *obj;
  sListWindow2Column *column;
  sListWindowTreeInfo<sObject *> *ti;

  if(!Array) return 0;

  Height = sGui->PropFont->GetHeight()+2;
  r.y0 = Client.y0;
  sFORALL(*Array,obj)
  {
    l = _i;

    ti = GetTreeInfo(obj);
    if(ti && (ti->Flags & sLWTI_HIDDEN))
      continue;

    // calc size

    y=0;
    sFORALL(Columns,column)
    {
      yy = OnCalcFieldHeight(column->Field,obj);
      if(yy==0) yy = Height;
      y = sMax(y,yy);
    }

    // check 

    r.y1 = r.y0 + y;
    r.x0 = Client.x0;
    sFORALL(Columns,column)
    {
      if(_i==Columns.GetCount()-1) 
        r.x1 = Client.x1;
      else
        r.x1 = Client.x0+column->Pos;

      if(r.Hit(mx,my))
      {
        line = l;
        col = _i;
        rect = r;

        if(col==0)
        {
          sListWindowTreeInfo<sObject *> *ti = GetTreeInfo((*Array)[line]);
          if(ti && mx<Client.x0+(ti->Level+1)*IndentPixels)
            return 2;
        }

        return 1;
      }
      r.x0 = r.x1;
    }
    r.y0 = r.y1;
  }
  return 0;
}

sBool sStaticListWindow::MakeRect(sInt line,sInt col,sRect &rect)
{
  sInt y,yy,l;
  sRect r;
  sObject *obj;
  sListWindow2Column *column;
  sListWindowTreeInfo<sObject *> *ti;

  if(!Array) return 0;

  Height = sGui->PropFont->GetHeight()+2;
  r.y0 = Client.y0;
  sFORALL(*Array,obj)
  {
    l = _i;

    ti = GetTreeInfo(obj);
    if(ti && (ti->Flags & sLWTI_HIDDEN))
      continue;

    // calc size

    y=0;
    sFORALL(Columns,column)
    {
      yy = OnCalcFieldHeight(column->Field,obj);
      if(yy==0) yy = Height;
      y = sMax(y,yy);
    }

    // check 

    r.y1 = r.y0 + y;
    r.x0 = Client.x0;
    sFORALL(Columns,column)
    {
      if(_i==Columns.GetCount()-1) 
        r.x1 = Client.x1;
      else
        r.x1 = Client.x0+column->Pos;

      if(_i==col && line==l)
      {
        rect = r;
        return 1;
      }
      r.x0 = r.x1;
    }
    r.y0 = r.y1;
  }
  return 0;
}

void sStaticListWindow::HitOpenClose(sInt line)
{
  if(!Array) return;

  sListWindowTreeInfo<sObject *> *ti = GetTreeInfo((*Array)[line]);
  if(ti)
  {
    ti->Flags ^= sLWTI_CLOSED;
    UpdateTreeInfo();
    Update();
  }
}

void sStaticListWindow::DragSelectSingle(const sWindowDrag &dd)
{
  sInt line,column;
  sInt hit;
  sRect r;
  switch(dd.Mode)
  {
  case sDD_START:
    hit= Hit(dd.MouseX,dd.MouseY,line,column,r);
    if(!hit)
      ClearSelectStatus();
    else
    {
      SetSelectStatus(line,1);
      if(hit==2)
      {
        HitOpenClose(line);
      }
      else
      {
        DragMode = 1;
      }
    }
    break;
  case sDD_DRAG:
    if(DragMode && Hit(dd.MouseX,dd.MouseY,line,column,r))
      SetSelectStatus(line,1);
    break;
  case sDD_STOP:
    DragMode = 0;
    break;
  }
}

void sStaticListWindow::DragSelectMulti(const sWindowDrag &dd,sDInt mode)
{
  sInt line,column;
  sRect r;
  if(!Array) return;
  sInt max = Array->GetCount();

  switch(mode)
  {
  case 0:
    switch(dd.Mode)
    {
    case sDD_START:
      DragSelectLine = -2;
      ClearSelectStatus();
    case sDD_DRAG:
      if(!Hit(dd.MouseX,dd.MouseY,line,column,r))
        line = -1;

      if(line>=0 && line!=DragSelectLine)
      {
        DragSelectLine = line;
        ClearSelectStatus();
        SetSelectStatus(DragSelectLine,1);
      }
      break;
    case sDD_STOP:
      DragMode = 0;
      break;
    }
    break;

  case 1:
    switch(dd.Mode)
    {
    case sDD_START:
      if(!(DragSelectLine>=0 && DragSelectLine<max))
        if(Hit(dd.MouseX,dd.MouseY,line,column,r))
          DragSelectLine = line;
      if(DragSelectLine>=0 && DragSelectLine<max)
        DragMode = 1;
      DragSelectOther = -2;
    case sDD_DRAG:
      if(DragMode==1)
      {
        if(!Hit(dd.MouseX,dd.MouseY,line,column,r))
          line = -1;
        if(line!=DragSelectOther && line!=-1)
        {
          DragSelectOther = line;

          sInt t0 = DragSelectLine;
          sInt t1 = line;
          if(t0>t1)
            sSwap(t0,t1);
          for(sInt i=0;i<max;i++)
            SetSelectStatus(i,i>=t0 && i<=t1);
        }
      }
      break;
    case sDD_STOP:
      DragMode = 0;
      break;
    }
    break;


  case 2:
    switch(dd.Mode)
    {
    case sDD_START:
      SelectionCopy.Clear();
      SelectionCopy.AddMany(max);
      for(sInt i=0;i<max;i++)
        SelectionCopy[i] = GetSelectStatus(i)?1:0;

      if(Hit(dd.MouseX,dd.MouseY,line,column,r))
      {
        DragMode = 2;
        DragSelectOther = line;
      }
    case sDD_DRAG:
      if(DragMode == 2 && Hit(dd.MouseX,dd.MouseY,line,column,r))
      {
        sInt t0 = DragSelectOther;
        sInt t1 = line;
        if(t0>t1)
          sSwap(t0,t1);
        for(sInt i=0;i<max;i++)
          SetSelectStatus(i,(i>=t0 && i<=t1)?!SelectionCopy[i]:SelectionCopy[i]);
      }
      break;
    case sDD_STOP:
      DragMode = 0;
    }
    break;
  }
}

sBool sStaticListWindow::ForceEditField(sInt line,sInt column)
{
  sRect r;

  if(MakeRect(line,column,r))
  {
    if(!OnEditField(r,Columns[column].Field,(*Array)[line],line))
      EditField(r,Columns[column].Field,(*Array)[line],line);
    return 1;
  }
  else
  {
    return 0;
  }
}


void sStaticListWindow::DragEdit(const sWindowDrag &dd)
{
  sInt line,column;
  sRect r;
  if(!Array) return;
  switch(dd.Mode)
  {
  case sDD_START:
    switch(Hit(dd.MouseX,dd.MouseY,line,column,r))
    {
    case 1:
      if(!OnEditField(r,Columns[column].Field,(*Array)[line],line))
        EditField(r,Columns[column].Field,(*Array)[line],line);
      break;
    case 2:
      HitOpenClose(line);
      break;
    }
    break;
  }
}

void sStaticListWindow::AddNew(sObject *newitem)
{
  if(!Array) return;
  sInt max = Array->GetCount();
  Update();
  for(sInt i=0;i<max;i++)
  {
    if(GetSelectStatus(i))
    {
      ClearSelectStatus();
      if(i==0)
        Array->AddBefore(newitem,i);
      else
        Array->AddAfter(newitem,i);
      SetSelectStatus(i+1,1);
      return;
    }
  }
  ClearSelectStatus();
  Array->AddTail(newitem);
  SetSelectStatus(max,1);
}

void sStaticListWindow::OpenFolders(sInt i)
{
  sInt level = GetTreeInfo((*Array)[i])->Level;

  i--;
  while(i>=0)
  {
    sInt nl = GetTreeInfo((*Array)[i])->Level;
    if(nl<level)
    {
      GetTreeInfo((*Array)[i])->Flags &= ~sLWTI_CLOSED;
      level = nl;
    }
    i--;
  }
  UpdateTreeInfo();
}

void sStaticListWindow::UpdateTreeInfo()
{
  sObject *obj;
  sObject *parents[sLW_MAXTREENEST];
  sObject *last;
  sInt parentlevel;
  sListWindowTreeInfo<sObject *> *ti,*tip;

  if(!Array) return;
  sClear(parents);

  Update();

  if(Array->GetCount()==0)
    return;
  if(GetTreeInfo((*Array)[0])==0)
    return;

  last = 0;
  parentlevel = 0;
  parents[0] = last;
    
  sFORALL(*Array,obj)
  {
    ti = GetTreeInfo(obj);
    sVERIFY(ti);
    sVERIFY(ti->Level>=0);
    if(ti->Level>=sLW_MAXTREENEST)
      ti->Level = sLW_MAXTREENEST-1;

    if(ti->Level > parentlevel)
    {
      while(parentlevel <= ti->Level)
      {
        parentlevel++;
        parents[parentlevel] = last;
      }
    }
    ti->Parent = parents[ti->Level];
    ti->FirstChild = 0;
    ti->NextSibling = 0;
    ti->TempPtr = &ti->FirstChild;

    parentlevel = ti->Level;
    last = obj;
  }

  sFORALL(*Array,obj)
  {
    ti = GetTreeInfo(obj);
    if(ti->Parent)
    {
      tip = GetTreeInfo(ti->Parent);
      *tip->TempPtr = obj;
      tip->TempPtr = &tip->NextSibling;
      if((tip->Flags & sLWTI_CLOSED) || (tip->Flags & sLWTI_HIDDEN))
        ti->Flags |=  sLWTI_HIDDEN;
      else
        ti->Flags &= ~sLWTI_HIDDEN;
    }
  }
}

void sStaticListWindow::CmdDelete(sDInt mode)
{
  if(!Array) return;

  sArray<sObject *> deletelist;
  sObject *obj;
  sInt firstdelete = -1;

  sFORALL(*Array,obj)
  {
    if(GetSelectStatus(_i))
    {
      if(firstdelete==-1) firstdelete = _i;
      deletelist.AddTail(obj);
    }
  }

  sFORALL(deletelist,obj)
    Array->RemOrder(obj);
  if(mode==1)
  {
    while(!deletelist.IsEmpty())
    {
      obj = deletelist.RemTail();
      delete (void *) obj;
    }
  }


  ClearSelectStatus();
  sInt max = Array->GetCount();
  if(firstdelete>=0 && firstdelete<max)
    SetSelectStatus(firstdelete,1);
  else if(max>0)
    SetSelectStatus(max-1,1);
  OrderMsg.Post();
  Update();
}

/****************************************************************************/

void sStaticListWindow::CmdSelectAll()
{
  if(Array)
  {
    for(sInt i=0;i<Array->GetCount();i++)
      SetSelectStatus(i,1);
  }
}

void sStaticListWindow::CmdSelUp(sDInt extend)
{
  if(!Array) return;

  sInt page = 1;
  if(extend&2)
    page = sMax(1,Inner.SizeY()/Height-1);

  sInt line = GetLastSelect();
  sInt max = Array->GetCount();
  for(sInt i=0;i<page;i++)
  {
    if(line==-1)
      line = 0;
    else if(line>0)
      line--;
    if(!extend)
      ClearSelectStatus();
  }
  if(line>=0 && line<max)
  {
    SetSelectStatus(line,1);
    ScrollTo(line);
  }
  DragSelectLine = -1;
}

void sStaticListWindow::CmdSelDown(sDInt extend)
{
  if(!Array) return;

  sInt page = 1;
  if(extend&2)
    page = sMax(1,Inner.SizeY()/Height-1);

  sInt line = GetLastSelect();
  sInt max = Array->GetCount();
  for(sInt i=0;i<page;i++)
  {
    if(line==-1)
      line = 0;
    else if(line<max-1)
      line++;
    if(!extend)
      ClearSelectStatus();
  }
  if(line>=0 && line<max)
  {
    SetSelectStatus(line,1);
    ScrollTo(line);
  }
  DragSelectLine = -1;
}

void sStaticListWindow::CmdMoveUp(sDInt mode)
{
  if(!Array) return;

  sInt page = 1;
  if(mode&2)
    page = sMax(1,Inner.SizeY()/Height-1);

  sBool post=0;
  for(sInt i=0;i<page;i++)
  {
    if(IsMulti)
    {
      sInt max = Array->GetCount();
      if(!GetSelectStatus(0))
      {
        for(sInt line=1;line<max;line++)
        {
          if(GetSelectStatus(line))
          {
            sSwap((*Array)[line-1],(*Array)[line]);
            post = 1;
          }
        }
      }
    }
    else
    {
      sInt line = GetLastSelect();
      sInt max = Array->GetCount();

      if(line>=1 && line<max)
      {
        sSwap((*Array)[line-1],(*Array)[line]);
        SetSelectStatus(line,0);
        SetSelectStatus(line-1,1);
        post = 1;
      }
    }
  }
  if(post)
  {
    ScrollTo(GetLastSelect());
    Update();
    OrderMsg.Post();
  }
  DragSelectLine = -1;
}

void sStaticListWindow::CmdMoveDown(sDInt mode)
{
  if(!Array) return;

  sBool post=0;
  sInt page = 1;
  if(mode&2)
    page = sMax(1,Inner.SizeY()/Height-1);

  for(sInt i=0;i<page;i++)
  {
    if(IsMulti)
    {
      sInt max = Array->GetCount();
      if(!GetSelectStatus(max-1))
      {
        for(sInt line=max-2;line>=0;line--)
        {
          if(GetSelectStatus(line))
          {
            sSwap((*Array)[line],(*Array)[line+1]);
            post = 1;
          }
        }
      }
    }
    else
    {
      sInt line = GetLastSelect();
      sInt max = Array->GetCount();

      if(line>=0 && line<max-1)
      {
        sSwap((*Array)[line],(*Array)[line+1]);
        SetSelectStatus(line,0);
        SetSelectStatus(line+1,1);
        post = 1;
      }
    }
  }
  if(post)
  {
    ScrollTo(GetLastSelect());
    Update();
    OrderMsg.Post();
  }
  DragSelectLine = -1;
}

void sStaticListWindow::CmdMoveLeft()
{
  if(!Array) return;

  sInt post = 0;
  if(IsMulti)
  {
    sInt max = Array->GetCount();
    for(sInt line=0;line<max;line++)
    {
      if(GetSelectStatus(line))
      {
        sListWindowTreeInfo<sObject *> *ti = GetTreeInfo((*Array)[line]);
        if(ti->Level>0)
          ti->Level--;
        post = 1;
      }
    }
  }
  else
  {
    sInt line = GetLastSelect();
    sInt max = Array->GetCount();

    if(line>=0 && line<max)
    {
      sListWindowTreeInfo<sObject *> *ti = GetTreeInfo((*Array)[line]);
      if(ti->Level>0)
        ti->Level--;
      post = 1;
    }
  }
  if(post)
  {
    UpdateTreeInfo();
    Update();
    OrderMsg.Post();
  }
  DragSelectLine = -1;
}

void sStaticListWindow::CmdMoveRight()
{
  if(!Array) return;

  sInt post = 0;
  if(IsMulti)
  {
    sInt max = Array->GetCount();
    for(sInt line=0;line<max;line++)
    {
      if(GetSelectStatus(line))
      {
        sListWindowTreeInfo<sObject *> *ti = GetTreeInfo((*Array)[line]);
        if(ti->Level<sLW_MAXTREENEST-1)
          ti->Level++;
        post = 1;
      }
    }
  }
  else
  {
    sInt line = GetLastSelect();
    sInt max = Array->GetCount();

    if(line>=0 && line<max)
    {
      sListWindowTreeInfo<sObject *> *ti = GetTreeInfo((*Array)[line]);
      if(ti->Level<sLW_MAXTREENEST-1)
        ti->Level++;
      post = 1;
    }
  }
  if(post)
  {
    UpdateTreeInfo();
    Update();
    OrderMsg.Post();
  }
  DragSelectLine = -1;
}

void sStaticListWindow::CmdOpenClose(sDInt mode)
{
  if(!Array) return;

  sInt line = GetLastSelect();
  sInt max = Array->GetCount();

  if(line>=0 && line<max)
  {
    sListWindowTreeInfo<sObject *> *ti = GetTreeInfo((*Array)[line]);
    if(ti)
    {
      switch(mode)
      {
      case 0:
        ti->Flags &= ~sLWTI_CLOSED;
        break;
      case 1:
        ti->Flags |= sLWTI_CLOSED;
        break;
      case 2:
        ti->Flags ^= sLWTI_CLOSED;
        break;
      }
      UpdateTreeInfo();
      Update();
    }
  }
  DragSelectLine = -1;
}

/****************************************************************************/
/***                                                                      ***/
/***   Header Border, new                                                 ***/
/***                                                                      ***/
/****************************************************************************/

sListWindow2Header::sListWindow2Header(sStaticListWindow *lw)
{
  ListWindow = lw;
  Height = 0;
  DragMode = -1;
  DragStart = 0;
}

sListWindow2Header::~sListWindow2Header()
{
}

void sListWindow2Header::Tag()
{
  sWindow::Tag();
  ListWindow->Need();
}

/****************************************************************************/

void sListWindow2Header::OnCalcSize()
{
  Height = sGui->PropFont->GetHeight()+3;
  ReqSizeY = Height;
}

void sListWindow2Header::OnLayout()
{
  Parent->Inner.y0 += Height;
  Client.y1 = Client.y0 + Height;
}

void sListWindow2Header::OnPaint2D()
{
  sListWindow2Column *lc;
  sRect r;
  sInt x;

  sClipPush();
  sClipRect(Parent->Outer);

  r = Client;
  r.y1--;
  sGui->PropFont->SetColor(sGC_TEXT,sGC_BUTTON);

  x = Client.x0 - Parent->ScrollX;
  sFORALL(ListWindow->Columns,lc)
  {
    r.x0 = x;
    x = r.x1 = Client.x0 + lc->Pos - Parent->ScrollX;

    if(_i+1!=ListWindow->Columns.GetCount())
    {
      r.x1--;
      if(r.x1<Client.x1)
        sRect2D(x-1,Client.y0,x,Client.y1,(DragMode==_i)?sGC_SELECT:sGC_TEXT);
    }
    else
    {
      r.x1 = Client.x1;
    }
    r.x1 = sMin(r.x1,Client.x1);

    // paint the sort-button

    if((lc->Field->Flags & sLWF_SORT) && r.SizeX()>r.SizeY()*2)
    {
      sRect a = r;
      a.x0 = r.x1 = r.x1-r.SizeY();
      lc->SortBox = a;
      sRect2D(a.x0,a.y0,a.x0+1,a.y1,sGC_TEXT);
      a.x0++;
      if(lc->Field->SortOrder>0)
      {
        sChar symbol[2];
        symbol[1] = 0;
        if(lc->Field->SortOrder>1)
          symbol[0] = lc->Field->SortDir ? 0x2191 : 0x2193;
        else
          symbol[0] = lc->Field->SortDir ? 0x25b2 : 0x25bc;

        sGui->PropFont->Print(sF2P_OPAQUE,a,symbol);
      }
      else
      {
        sRect2D(a,sGC_BUTTON);
      }
    }
    else
    {
      lc->SortBox.Init();
    }

    // paint the field

    if(r.SizeX()>0)
      sGui->PropFont->Print(sF2P_OPAQUE|sF2P_SPACE|sF2P_LEFT,r,lc->Field->Label);
  }
  sRect2D(Client.x0,Client.y0+Height-1,Client.x1,Client.y0+Height,sGC_TEXT);

  sClipPop();
}

void sListWindow2Header::OnDrag(const sWindowDrag &dd)
{
  sListWindow2Column *lc;

  sInt x0 = Client.x0 - Parent->ScrollX;

  switch(dd.Mode)
  {
  case sDD_HOVER:
    DragAbsolute = (sGetKeyQualifier()&sKEYQ_SHIFT) ? 1 : 0;
    MousePointer = sMP_ARROW;
    for(sInt i=0;i<ListWindow->Columns.GetCount()-1;i++)
    {
      lc = &ListWindow->Columns[i];
      if(dd.MouseX > lc->Pos+x0-2 && dd.MouseX < lc->Pos+x0+2)
      {
        MousePointer = sMP_SIZEWE;
        break;
      }
    }
    break;

  case sDD_START:
    for(sInt i=0;i<ListWindow->Columns.GetCount();i++)
    {
      lc = &ListWindow->Columns[i];
      if(dd.MouseX > lc->Pos+x0-2 && dd.MouseX < lc->Pos+x0+2 && i<ListWindow->Columns.GetCount()-1)
      {
        DragMode = i;
        DragStart = lc->Pos-dd.MouseX;
        ListWindow->DragHighlight = DragMode;
        sGui->Update(Parent->Outer);
      }
      else if(lc->SortBox.Hit(dd.MouseX,dd.MouseY))
      {
        sListWindow2Field *f;
        if(dd.Flags & sDDF_DOUBLECLICK)
        {
          sFORALL(ListWindow->Fields,f)
          {
            f->SortOrder = 0;
            f->SortDir = 0;
          }
          lc->Field->SortOrder = 1;
        }
        else
        {
          if(lc->Field->SortOrder == 0)
          {
            sFORALL(ListWindow->Fields,f)
              if(f->SortOrder>0)
                f->SortOrder++;
            lc->Field->SortOrder = 1;
            lc->Field->SortDir = 0;
          }
          else
          {
            lc->Field->SortDir = !lc->Field->SortDir;
          }
        }
        ListWindow->Sort();
        sGui->Update(Parent->Outer);
      }
    }
    break;
  case sDD_DRAG:
    if(DragMode>=0 && DragMode<ListWindow->Columns.GetCount()-1)
    {
      lc = &ListWindow->Columns[DragMode];
      sInt oldp = lc->Pos;
      sInt newp = DragStart + dd.MouseX;
      newp = sClamp<sInt>(newp,2,Client.SizeX()-2+Parent->ScrollX);
      if(oldp!=newp)
      {
        if(!DragAbsolute)       // drag keeping width
        {
          if(DragMode>0)
            newp = sMax(newp,ListWindow->Columns[DragMode-1].Pos+2);
          for(sInt i=DragMode;i<ListWindow->Columns.GetCount();i++)
            ListWindow->Columns[i].Pos += (newp-oldp);
        }
        else                    // drag only width
        {
          if(DragMode>0)
            newp = sMax(newp,ListWindow->Columns[DragMode-1].Pos+2);
          if(DragMode<ListWindow->Columns.GetCount()-2)
            newp = sMin(newp,ListWindow->Columns[DragMode+1].Pos-2);
          lc->Pos = newp;
        }
        sGui->Update(Parent->Outer);
      }
    }
    break;
  case sDD_STOP:
    if(DragMode!=-1)
    {
      DragMode = -1;
      ListWindow->DragHighlight = DragMode;
      sGui->Update(Parent->Outer);
    }
    break;
  }
}

/****************************************************************************/



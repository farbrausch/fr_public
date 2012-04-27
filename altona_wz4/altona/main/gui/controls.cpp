/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "gui/controls.hpp"
#include "gui/frames.hpp"
#include "gui/borders.hpp"
#include "gui/dialog.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Control                                                            ***/
/***                                                                      ***/
/****************************************************************************/

sControl::sControl()
{
  Style = 0;
  BackColor = 0;
}

sControl::~sControl()
{
}

void sControl::Tag()
{
  sWindow::Tag();
  DoneMsg.Target->Need();
  ChangeMsg.Target->Need();
}

void sControl::SendChange()
{
  ChangeMsg.Send();
}
void sControl::SendDone()
{
  DoneMsg.Send();
}

void sControl::PostChange()
{
  ChangeMsg.Post();
}

void sControl::PostDone()
{
  DoneMsg.Post();
}

/****************************************************************************/
/***                                                                      ***/
/***   Button                                                             ***/
/***                                                                      ***/
/****************************************************************************/

sButtonControl::sButtonControl(const sChar *label,const sMessage &msg,sInt style)
{
  Style = style;
  ChangeMsg = msg;
  Label = label;
  LabelLength = -1;
  Pressed = 0;
  RadioValue = 0;
  RadioPtr = 0;
  Width = 0;
  Shortcut = 0;
}

sButtonControl::sButtonControl()
{
  Pressed = 0;
  Label = L"";
  LabelLength = -1;
  RadioValue = 0;
  RadioPtr = 0;
  Width = 0;
  Shortcut = 0;
}

void sButtonControl::InitRadio(sInt *ptr,sInt val)
{
  RadioPtr = ptr;
  RadioValue = val;
  Style |= sBCS_RADIO;
  ClearNotify();
  AddNotify(RadioPtr,sizeof(sInt));
}

void sButtonControl::InitToggle(sInt *ptr)
{
  RadioPtr = ptr;
  RadioValue = 1;
  Style |= sBCS_TOGGLE;
  ClearNotify();
  AddNotify(RadioPtr,sizeof(sInt));
}

void sButtonControl::InitReadOnly(sInt *ptr,sInt val)
{
  RadioPtr = ptr;
  RadioValue = val;
  Style |= sBCS_READONLY;
  ClearNotify();
  AddNotify(RadioPtr,sizeof(sInt));
}

void sButtonControl::InitCheckmark(sInt *ptr,sInt val)
{
  RadioPtr = ptr;
  RadioValue = val;
  Style |= sBCS_CHECKMARK;
  ClearNotify();
  AddNotify(RadioPtr,sizeof(sInt));
}


void sButtonControl::MakeShortcut(sString<64> &buffer,sU32 Shortcut)
{
  sChar s[2];

  s[0] = Shortcut & sKEYQ_MASK;
  s[1] = 0;
  buffer[0] = 0;
  if(Shortcut)
  {
    if(Shortcut & sKEYQ_SHIFT)
      buffer.Add(L"shift+");
    if(Shortcut & sKEYQ_CTRL)
      buffer.Add(L"ctrl+");
    if(Shortcut & sKEYQ_ALT)
      buffer.Add(L"alt+");
    if(Shortcut & 0x01000000)
      buffer.Add(L"hit+");
    if(Shortcut & 0x02000000)
      buffer.Add(L"miss+");
    if(Shortcut & 0x04000000)
      buffer.Add(L"double-");

    switch(s[0])
    {
      default:              buffer.Add(s);  break;
      case sKEY_UP:         buffer.Add(L"up"); break;
      case sKEY_DOWN:       buffer.Add(L"down"); break;
      case sKEY_LEFT:       buffer.Add(L"left"); break;
      case sKEY_RIGHT:      buffer.Add(L"right"); break;
      case sKEY_HOME:       buffer.Add(L"home"); break;
      case sKEY_END:        buffer.Add(L"end"); break;
      case sKEY_PAGEUP:     buffer.Add(L"pageup"); break;
      case sKEY_PAGEDOWN:   buffer.Add(L"pagedown"); break;
      case sKEY_INSERT:     buffer.Add(L"insert"); break;
      case sKEY_DELETE:     buffer.Add(L"delete"); break;
      case sKEY_WHEELUP:    buffer.Add(L"wheelup"); break;
      case sKEY_WHEELDOWN:  buffer.Add(L"wheeldown"); break;

      case sKEY_PAUSE:      buffer.Add(L"pause"); break;
      case sKEY_PRINT:      buffer.Add(L"print"); break;
      case sKEY_WINM:       buffer.Add(L"menu"); break;

      case sKEY_BACKSPACE:  buffer.Add(L"backspace"); break;
      case sKEY_TAB:        buffer.Add(L"tab"); break;
      case sKEY_ENTER:      buffer.Add(L"enter"); break;
      case sKEY_ESCAPE:     buffer.Add(L"escape"); break;
      case sKEY_SPACE:      buffer.Add(L"space"); break;
      case sKEY_LMB:        buffer.Add(L"LMB"); break;
      case sKEY_RMB:        buffer.Add(L"RMB"); break;
      case sKEY_MMB:        buffer.Add(L"MMB"); break;
      case sKEY_X1MB:       buffer.Add(L"4MB"); break;
      case sKEY_X2MB:       buffer.Add(L"5MB"); break;
      case sKEY_HOVER:      buffer.Add(L"hover"); break;

      case sKEY_F1:         buffer.Add(L"F1"); break;
      case sKEY_F2:         buffer.Add(L"F2"); break;
      case sKEY_F3:         buffer.Add(L"F3"); break;
      case sKEY_F4:         buffer.Add(L"F4"); break;
      case sKEY_F5:         buffer.Add(L"F5"); break;
      case sKEY_F6:         buffer.Add(L"F6"); break;
      case sKEY_F7:         buffer.Add(L"F7"); break;
      case sKEY_F8:         buffer.Add(L"F8"); break;
      case sKEY_F9:         buffer.Add(L"F9"); break;
      case sKEY_F10:        buffer.Add(L"F10"); break;
      case sKEY_F11:        buffer.Add(L"F11"); break;
      case sKEY_F12:        buffer.Add(L"F12"); break;
    }
  }
}

void sButtonControl::OnCalcSize()
{
  ReqSizeY = sGui->PropFont->GetHeight();
  ReqSizeX = sGui->PropFont->GetWidth(Label,LabelLength) + sGui->PropFont->GetWidth(L"  ");

  if(Style&sBCS_NOBORDER)
  {
    ReqSizeX += 2;
    ReqSizeY += 2;

    if(Shortcut)
    {
      sString<64> buffer;
      MakeShortcut(buffer,Shortcut);
      ReqSizeX += sGui->PropFont->GetWidth(buffer);
      ReqSizeX += sGui->PropFont->GetWidth(L"  ");
    }
    if(Style&sBCS_CHECKMARK)
      ReqSizeX += ReqSizeY;
  }
  else
  {
    ReqSizeX += 4;
    ReqSizeY += 4;
  }

  if(Width!=0)
    ReqSizeX = Width;
}

void sButtonControl::OnPaint2D()
{
  if(Style & sBCS_NOBORDER)
  {
    sInt bp = sGC_BACK;
    if(BackColor)
    {
      sSetColor2D(0,BackColor);
      bp = 0;
    }

    if ((Flags&sWF_HOVER)&&!(Style&sBCS_STATIC))
      bp=sGC_SELECT;

    sGui->PropFont->SetColor(sGC_TEXT,bp);

    sRect r = Client;
    if(Style & sBCS_CHECKMARK)
    {
      sRect r2 = r;
      sRect r3;
      sInt w = 5;
      r.x0 = r2.x1 = r2.x0+r2.SizeY();
      r3.x0 = r2.CenterX()-w;
      r3.x1 = r3.x0 + w*2;
      r3.y0 = r2.CenterY()-w;
      r3.y1 = r3.y0 + w*2;

      sClipPush();

      sBool select = 0;
      if(Style & sBCS_TOGGLEBIT)
      {
        select = *RadioPtr & RadioValue;
      }
      else
      {
        select = *RadioPtr==RadioValue;
      }
      if(select)
      {      
        sRect r4=r3;
        r4.Extend(-2);
        sRect2D(r4,sGC_DRAW);
        sClipExclude(r4);
      }
      sRect2D(r3,bp);
      sClipPop();
      sGui->RectHL(r3,sTRUE);
      sRectHole2D(r2,r3,bp);
    }
    sGui->PropFont->Print(sF2P_OPAQUE|sF2P_LEFT|sF2P_SPACE,r,Label,LabelLength);
    if(Shortcut)
    {
      sString<64> buffer;
      MakeShortcut(buffer,Shortcut);

      sGui->PropFont->Print(sF2P_RIGHT|sF2P_SPACE,r,buffer);
    }
    SetToolTipIfNarrow(Label,LabelLength);
  }
  else
  {
    sInt f = Pressed ? sGPB_DOWN : 0;
    if(RadioPtr)
    {
      if(Style & sBCS_TOGGLEBIT)
        f = (*RadioPtr & RadioValue) ? 1 : 0;
      else
        f = (*RadioPtr == RadioValue);
    }
    if(Style & sBCS_STATIC) f |= sGPB_GRAY;
    if(Style & sBCS_SHOWSELECT) f |= sGPB_DOWN;
    sGui->PaintButton(Client,Label,f,LabelLength,BackColor);

    SetToolTipIfNarrow(Label,LabelLength);

    if(Flags & sWF_CHILDFOCUS)
    {
      sRect r(Client);
      r.Extend(-3);
      sRectFrame2D(r,sGC_DOC);
    }
  }
}

void sButtonControl::OnDrag(const sWindowDrag &dd)
{
  if(!(Style&sBCS_STATIC))
  {
    switch(dd.Mode)
    {
    case sDD_START:
      if(dd.Buttons & 1)
      {
        Pressed = 1;
        Update();
        if(dd.Flags&sDDF_DOUBLECLICK)
          DoubleClickMsg.Send();
        else if(Style & sBCS_IMMEDIATE)
          OnCommand(sCMD_TRIGGER);
      }
      break;
    case sDD_STOP:
      if(Pressed)
      {
        if(!(Style & sBCS_IMMEDIATE) && (Flags&sWF_HOVER))
          OnCommand(sCMD_TRIGGER);
        Pressed = 0;
        Update();
      }
      break;
    }
  }
}


sBool sButtonControl::OnKey(sU32 key)
{
  if(!(Style&sBCS_STATIC))
  {
    key = key & ~sKEYQ_CAPS;
    if(key==' ')
    {
      Pressed = 1;
      OnCommand(sCMD_TRIGGER);
      Update();
      return 1;
    }
    if(key==(' '|sKEYQ_BREAK))
    {
      Pressed = 0;
      return 1;
    }
  }
  return 0;
}

sBool sButtonControl::OnCommand(sInt cmd)
{
  switch(cmd)
  {
  case sCMD_TRIGGER:
    if(!(Style&sBCS_STATIC))
    {
      if(RadioPtr)
      {
        if(Style & sBCS_RADIO)
          *RadioPtr = RadioValue;
        else if(Style & sBCS_TOGGLE)
          *RadioPtr = !*RadioPtr;
        else if(Style & sBCS_TOGGLEBIT)
          *RadioPtr = *RadioPtr ^ RadioValue; 
        sGui->Notify(RadioPtr,sizeof(sInt));
  //      Parent->Update();
      }
      SendChange();
      SendDone();
    }
    break;
  case sCMD_ENTERHOVER:
  case sCMD_LEAVEHOVER:
  case sCMD_ENTERFOCUS:
  case sCMD_LEAVEFOCUS:
    Update();
    break;
  }
  return 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   sFileDialogControl                                                 ***/
/***                                                                      ***/
/****************************************************************************/

sFileDialogControl::sFileDialogControl(const sStringDesc &string,const sChar *text,const sChar *ext,sInt flags) : String(string)
{
  Text = text;
  Extensions = ext;
  Label = L"...";
  if(flags)
    Flags = flags;
  else
    Flags = sSOF_LOAD;
  DoneMsg = sMessage(this,&sFileDialogControl::CmdStart);
}

sFileDialogControl::~sFileDialogControl()
{
}

void sFileDialogControl::CmdStart()
{
  sOpenFileDialog(Text,Extensions,Flags,String,OkMsg,sMessage());
}

/****************************************************************************/
/***                                                                      ***/
/***   Choices                                                            ***/
/***                                                                      ***/
/****************************************************************************/

sChoiceControl::sChoiceControl(const sChar *choices,sInt *val)
{
  InitChoices(choices,val);
  Width = 0;
  Pressed = 0;
}

sChoiceControl::sChoiceControl()
{
  ValueMask = ~0;
  ValueShift = 0;
  Width = 0;
  Style = sCBS_DROPDOWN;
  Pressed = 0;
}

void sChoiceControl::AddMultiChoice(sInt *val)
{
  ChoiceMulti *e = Values.AddMany(1);
  e->Ptr = val;
  e->Default = *val;
  AddNotify(val,sizeof(sInt));
}

void sChoiceControl::InitChoices(const sChar *&s,sInt *val)
{
  sInt id = 0;
  ChoiceInfo *ci;
  sInt min=0;
  sInt max=0;
  sInt len;
  sBool fullmask = 0;
  
  Choices.Clear();
  ValueMask = 0;
  ValueShift = 0;
  if(*s=='#')
  {
    s++;
    fullmask = 1;
  }
  if(*s=='*')
  {
    s++;
    sScanInt(s,ValueShift);
  }
  for(;;)
  {
    while(*s=='|')
    {
      s++;
      id++;
    }
    if(*s==':' || *s==0)
      break;

    if(sIsDigit(*s) || (*s=='-' && sIsDigit(s[1])) || (*s=='+' && sIsDigit(s[1])))
      sScanInt(s,id);
    if(*s==' ')
      s++;

    len = 0;
    while(s[len]!='|' && s[len]!=':' && s[len]!=0) len++;
    ci = Choices.AddMany(1);
    ci->Label = s;
    ci->Length = len;
    ci->Value = id;
    min = sMin(min,id);
    max = sMax(max,id);
    s+=len;
  }

  ValueMask = 1;
  while(ValueMask<max)
    ValueMask = ValueMask*2+1;
  if (min<0)
    ValueMask = ~0;
  ValueMask <<= ValueShift;
  if(fullmask)
    ValueMask = ~0;

  if(val)
  {
    Values.Clear();
    AddMultiChoice(val);
    ClearNotify();
    AddNotify(val,sizeof(sInt));
    Style = Choices.GetCount()==2 ? sCBS_CYCLE : sCBS_DROPDOWN;
  }
  else
  {
    Style |= sCBS_DROPDOWN;
  }
}

/****************************************************************************/

void sChoiceControl::OnCalcSize()
{
  ChoiceInfo *ci;
  ReqSizeY = sGui->PropFont->GetHeight();
  ReqSizeX = 0;
  sFORALL(Choices,ci)
    ReqSizeX = sMax(ReqSizeX,sGui->PropFont->GetWidth(ci->Label,ci->Length));
  ReqSizeX += sGui->PropFont->GetWidth(L"  ");

  ReqSizeX += 4;
  ReqSizeY += 4;

  if(Width!=0)
    ReqSizeX = Width;
}

void sChoiceControl::OnPaint2D()
{
  ChoiceInfo *ci;
  sVERIFY(Values.GetCount()>0);
  ChoiceMulti *cm;

  sInt flags = Pressed ? 1 : 0;
  if(Style & sBCS_STATIC)
    flags |= sGPB_GRAY;

  sInt val = (*(Values[0].Ptr) & ValueMask)>>ValueShift;
  sFORALL(Values,cm)
  {
    if(((*(cm->Ptr) & ValueMask)>>ValueShift)!=val)
    {
      sGui->PaintButton(Client,L"(differ)",flags);
      goto focus;
    }
  }
  

  sFORALL(Choices,ci)
  {
    if(val==ci->Value)
    {
      if(ci->Label[0]=='-' && ci->Length==1 /* && _i==0 */ && Choices.GetCount()==2)
      {
        sString<128> str;
        str.Add(L"(");
        str.Add(Choices[!_i].Label,Choices[!_i].Length);
        str.Add(L")");
        sGui->PaintButton(Client,str,flags|sGPB_GRAY);
      }
      else
      {
        sGui->PaintButton(Client,ci->Label,flags,ci->Length);
        SetToolTipIfNarrow(ci->Label,ci->Length);
      }
      goto focus;
    }
  }
  sGui->PaintButton(Client,L"???",flags);

focus:
  if(Flags & sWF_CHILDFOCUS)
  {
    sRect r(Client);
    r.Extend(-3);
    sRectFrame2D(r,sGC_DOC);
  }
}

void sChoiceControl::OnDrag(const sWindowDrag &dd)
{
  if(!(Style&sBCS_STATIC))
  {
    switch(dd.Mode)
    {
    case sDD_START:
      if(dd.Buttons & 3)          // RMB ist immer toggle
      {
        Pressed = dd.Buttons;
        Update();
      }
      break;
    case sDD_STOP:
      if(Pressed)
      {
        if(Pressed==1)
        {
          if(Style&sCBS_DROPDOWN)
            Dropdown();
          else
            Next();
        }
        else
        {
          Next();
        }
        Pressed = 0;
        Update();
      }
      break;
    }
  }
}

sBool sChoiceControl::OnKey(sU32 key)
{
  if(!(Style&sBCS_STATIC))
  {
    key = key & ~sKEYQ_CAPS;
    if(key==' ')
    {
      Pressed = 1;
      if(Style&sCBS_DROPDOWN)
        Dropdown();
      else
        Next();
      Update();
      return 1;
    }
    if(key==(' '|sKEYQ_BREAK))
    {
      Pressed = 0;
      Update();
      return 1;
    }
  }
  return 0;
}

sBool sChoiceControl::OnCommand(sInt cmd)
{
  switch(cmd)
  {
  case sCMD_ENTERFOCUS:
  case sCMD_LEAVEFOCUS:
    Update();
    return 1;
  default:
    return 0;
  }
}


void sChoiceControl::Next()
{
  ChoiceInfo *ci;
  sInt *value = (Values[0].Ptr);
  sInt val = (*value & ValueMask)>>ValueShift;

  sFORALL(Choices,ci)
  {
    if(val==ci->Value)
    {
      _i++;
      if(_i>=Choices.GetCount())
        _i = 0;
      *value = (*value & ~ValueMask) | (Choices[_i].Value<<ValueShift);
      sGui->Notify(value,sizeof(sInt));
      PostChange();
      PostDone();
      return;
    }
  }
  *value = (*value & ~ValueMask) | (Choices[0].Value<<ValueShift);
  sGui->Notify(value,sizeof(sInt));
  PostChange();
  PostDone();
}

void sChoiceControl::FakeDropdown(sInt x,sInt y)
{
  sMenuFrame *mf;
  ChoiceInfo *ci;

  sWindow *w=this;
  sInt max = 1;
  if(w->Parent==0)
  {
    max = 20;
  }
  else
  {
    while (w->Parent) w=w->Parent;
    max=sMax(1,w->Client.SizeY()/(sGui->PropFont->GetHeight()+2));
  }
  sInt n=0;
  mf = new sMenuFrame(this);
  if(Values.GetCount()>=2)
    mf->AddItem(L"(differ)",sMessage(this,&sChoiceControl::SetDefaultValue,0),0,-1,n++/max);
  sFORALL(Choices,ci)
  {
    sVERIFY(ci->Value<0x40000000);
    mf->AddItem(ci->Label,sMessage(this,&sChoiceControl::SetValue,ci->Value),0,ci->Length,n++/max);
  }
  mf->PopupParent = this;
  sGui->AddWindow(mf,x,y);
}


void sChoiceControl::Dropdown()
{
  FakeDropdown(Client.x0,Client.y1);
}

void sChoiceControl::SetValue(sDInt newval)
{
  ChoiceMulti *cm;
  sBool changed = 0;
  sFORALL(Values,cm)
  {
    sInt val = (*cm->Ptr & ~ValueMask) | ((newval)<<ValueShift);
    if(val!=*cm->Ptr)
    {
      *cm->Ptr = val;
      sGui->Notify(cm->Ptr,sizeof(sInt));
      changed = 1;
    }    
  }
  if(changed)
  {
    PostChange();
    PostDone();
  }
}

void sChoiceControl::SetDefaultValue()
{
  ChoiceMulti *cm;
  sBool changed = 0;
  sFORALL(Values,cm)
  {
    sInt val = (*cm->Ptr & ~ValueMask) | ((cm->Default & ValueMask));
    if(val!=*cm->Ptr)
    {
      *cm->Ptr = val;
      sGui->Notify(cm->Ptr,sizeof(sInt));
      changed = 1;
    }    
  }
  if(changed)
  {
    PostChange();
    PostDone();
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   String                                                             ***/
/***                                                                      ***/
/****************************************************************************/

sStringControl::sStringControl(const sStringDesc &desc)
{
  Style = 0;
  BackColor = 0xffff0000;
  InitString(desc);
}

sStringControl::sStringControl(sInt size,sChar *buffer)
{
  Style = 0;
  BackColor = 0xffff0000;
  InitString(sStringDesc(buffer,size));
}

sStringControl::sStringControl(sPoolString *pool)
{
  Style = 0;
  BackColor = 0xffff0000;
  InitString(pool);
}

sStringControl::sStringControl(sTextBuffer *tb)
{
  Style = 0;
  BackColor = 0xffff0000;
  InitString(tb);
}

sStringControl::sStringControl()
{
  Style = 0;
  BackColor = 0xffff0000;
  InitString(sStringDesc(0,0));
}

sStringControl::~sStringControl()
{
  if(PoolPtr || TextBuffer)
    delete[] Buffer;
}

void sStringControl::Tag()
{
  sControl::Tag();
  DragTogether->Need();
}

void sStringControl::InitString(const sStringDesc &desc)
{
  TextBuffer = 0;
  PoolPtr = 0;
  Buffer = desc.Buffer;
  Size = desc.Size;
  DragTogether = 0;
  Percent = 0;
  sWindow::Flags |= sWF_SCROLLX;
  Changed = 0;

  PrintInfo.Init();
  PrintInfo.SelectBackColor=sGC_SELECT;
  MarkMode = MarkBegin = MarkEnd = 0;
  ClearNotify();
  AddNotify(Buffer,Size*sizeof(sChar));
}

void sStringControl::InitString(sPoolString *pool)
{
  DragTogether = 0;

  TextBuffer = 0;
  PoolPtr = pool;
  Buffer = 0;
  Size = 0;
  sWindow::Flags |= sWF_SCROLLX;
  Changed = 0;

  PrintInfo.Init();
  PrintInfo.SelectBackColor=sGC_SELECT;
  MarkMode = MarkBegin = MarkEnd = 0;
  ClearNotify();
  AddNotify(Buffer,Size*sizeof(sChar));
  AddNotify(pool,sizeof(pool));
}

void sStringControl::InitString(sTextBuffer *tb)
{
  DragTogether = 0;

  TextBuffer = tb;
  PoolPtr = 0;
  Buffer = new sChar[sMAXPATH];
  Size = sMAXPATH;
  sWindow::Flags |= sWF_SCROLLX;
  Changed = 0;
  
  sCopyString(Buffer,tb->Get(),Size);

  PrintInfo.Init();
  PrintInfo.SelectBackColor=sGC_SELECT;
  MarkMode = MarkBegin = MarkEnd = 0;
  ClearNotify();
  AddNotify(Buffer,Size*sizeof(sChar));
}

/****************************************************************************/

const sChar *sStringControl::GetText()
{
  const sChar *text = Buffer;
  if(Buffer==0 && PoolPtr)
    text = *PoolPtr;
  if(text==0)
    text = L"";

  return text;
}

void sStringControl::OnCalcSize()
{
  if(!(Style & sSCS_FIXEDWIDTH))
    ReqSizeX = sGui->PropFont->GetWidth(GetText()) + 3*sGui->PropFont->GetWidth(L" ");
  ReqSizeY = sGui->PropFont->GetHeight()+4;
}

void sStringControl::OnPaint2D()
{
  sRect r;

  r = Inner;
  sGui->RectHL(r,sTRUE);
  r.Extend(-1);
  sClipPush();
  sClipRect(r);

  r = Client;
  r.Extend(-1);
  if(Style&sSCS_BACKCOLOR)
  {
    sSetColor2D(0,BackColor);
    sInt i = ((BackColor>>16)&255) + ((BackColor>>8)&255) + ((BackColor>>0)&255);
    sGui->PropFont->SetColor(i>3*128 ? sGC_BLACK : sGC_WHITE , 0);
  }
  else if(Style & sSCS_STATIC)
  {
    sGui->PropFont->SetColor(sGC_LOW2,sGC_BACK);
  }
  else
  {
    sGui->PropFont->SetColor(sGC_TEXT,sGC_DOC);
  }
  if(MakeBuffer(0))
    ResetCursor();

  const sChar *text = GetText();
  sPrintInfo *pi=0;
  if((Flags & sWF_CHILDFOCUS) && !(Style & sSCS_STATIC))
    pi = &PrintInfo;

  if(Style & sSCS_BACKBAR)
  {
    sRect r0,r1;
    r0 = r1 = r;
    r0.x1 = r1.x0 = r.x0 + sInt(Percent * r.SizeX());
    sClipPush();
    sClipRect(r1);
    sGui->PropFont->Print(sF2P_SPACE|sF2P_LEFT|sF2P_OPAQUE,r,text,-1,0,0,0,pi);
    sClipPop();
    sClipPush();
    sClipRect(r0);
    sGui->PropFont->SetColor(sGC_TEXT,sGC_BUTTON);
    sGui->PropFont->Print(sF2P_SPACE|sF2P_LEFT|sF2P_OPAQUE,r,text,-1,0,0,0,pi);
    sClipPop();
  }
  else
  {
    sGui->PropFont->Print(sF2P_SPACE|sF2P_LEFT|sF2P_OPAQUE,r,text,-1,0,0,0,pi);
  }

  sClipPop();
}

sBool sStringControl::OnKey(sU32 key)
{
  sInt len;
  sBool update;
  sBool change;
  sBool result;
  sBool updatemark;
  sInt oldcursorpos;

  if(!Buffer) return 0;
  if(Style & sSCS_STATIC) return 0;

  if(MakeBuffer(0))
    ResetCursor();

  len = sGetStringLen(Buffer);
  update = 0;
  change = 0;
  result = 1;
  updatemark = 0;
  oldcursorpos = PrintInfo.CursorPos;

  key &= ~sKEYQ_CAPS;
  if(key&sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
  if(key&sKEYQ_CTRL)  key |= sKEYQ_CTRL;
  key &= ~(sKEYQ_REPEAT|sKEYQ_ALTGR);

  switch(key)
  {
    // cursor movement

  case sKEY_LEFT:
    if(PrintInfo.CursorPos>0)
      PrintInfo.CursorPos--;
    update = 1;
    break;
  case sKEY_RIGHT:
    if(PrintInfo.CursorPos<len)
      PrintInfo.CursorPos++;
    update = 1;
    break;
  case sKEY_HOME:
    PrintInfo.CursorPos = 0;
    update = 1;
    break;
  case sKEY_END:
    PrintInfo.CursorPos = len;
    update = 1;
    break;

  case sKEY_INSERT:
    PrintInfo.Overwrite = !PrintInfo.Overwrite;
    update = 1;
    break;

    // block marking

  case sKEY_LEFT|sKEYQ_SHIFT:
    if(PrintInfo.CursorPos>0)
      PrintInfo.CursorPos--;
    updatemark = 1;
    break;
  case sKEY_RIGHT|sKEYQ_SHIFT:
    if(PrintInfo.CursorPos<len)
      PrintInfo.CursorPos++;
    updatemark = 1;
    break;
  case sKEY_HOME|sKEYQ_SHIFT:
    PrintInfo.CursorPos = 0;
    updatemark = 1;
    break;
  case sKEY_END|sKEYQ_SHIFT:
    PrintInfo.CursorPos = len;
    updatemark = 1;
    break;

    // deletion

  case sKEY_BACKSPACE:
    if(!MarkMode)
    {
      if(PrintInfo.CursorPos==0)
        break;
      PrintInfo.CursorPos--;
    }
    /* nobreak */
  case sKEY_DELETE:
    if(MarkMode)
    {
      DeleteMark();
      MarkMode = 0;
    }
    else
    {
      for(sInt i=PrintInfo.CursorPos;i<len;i++)
        Buffer[i] = Buffer[i+1];
    }
    change = 1;
    break;

    // clipboard

  case 'a'|sKEYQ_CTRL:
    MarkMode = 1;
    PrintInfo.SelectStart = MarkBegin = 0;
    PrintInfo.SelectEnd = MarkEnd = len;
    Update();
    break;
  case 'x'|sKEYQ_CTRL:
  case sKEY_DELETE|sKEYQ_SHIFT:
    if(MarkMode)
    {
      sSetClipboard(Buffer+PrintInfo.SelectStart,PrintInfo.SelectEnd-PrintInfo.SelectStart);
      DeleteMark();
      change = 1;
    }
    break;
  case 'c'|sKEYQ_CTRL:
  case sKEY_INSERT|sKEYQ_CTRL:
    if(MarkMode)
    {
      sSetClipboard(Buffer+PrintInfo.SelectStart,PrintInfo.SelectEnd-PrintInfo.SelectStart);
      update = 1;
    }
    break;
  case 'v'|sKEYQ_CTRL:
  case sKEY_INSERT|sKEYQ_SHIFT:
    if(MarkMode)
      DeleteMark();
    {
      const sChar *string = sGetClipboard();
      const sChar *s = string;

      while(*s)
        InsertChar(*s++);

      delete[] string;
      change = 1;
    }
    break;

    // keys

  case sKEY_ENTER:
    sGui->SetFocus(Parent); // the work is done in sCMD_LEAVEFOCUS!
    break;
   
  default:
    key = key&~(sKEYQ_SHIFT);
    if(key>=0x20 && key<0xe000)
    {
      if(MarkMode)
      {
        DeleteMark();
        len = sGetStringLen(Buffer);
      }
      if(PrintInfo.Overwrite && PrintInfo.CursorPos < len)
      {
        Buffer[PrintInfo.CursorPos++] = key;
        change = 1;
      }
      else if(len<Size-1)
      {
        InsertChar(key);
        change = 1;
      }
    }
    else
    {
      result = 0;
    }
    break;
  }

  if(updatemark)
  {
    if(!MarkMode)
      MarkBegin = oldcursorpos;
    MarkMode = 1;
    MarkEnd = PrintInfo.CursorPos;
    PrintInfo.SelectStart = sMin(MarkBegin,MarkEnd);
    PrintInfo.SelectEnd   = sMax(MarkBegin,MarkEnd);
  }
  if(update || change)
  {
    MarkMode = 0;
    PrintInfo.SelectStart = -1;
    PrintInfo.SelectEnd   = -1;
  }
  if(update || change || updatemark)
  {
    const sChar *text = Buffer;
    if(Buffer==0 && PoolPtr)
      text = *PoolPtr;
    if(text==0)
      text = L"";
    sRect r;
    r = Client;
    r.Extend(-1);
    PrintInfo.QueryPos = text + PrintInfo.CursorPos;
    PrintInfo.Mode = sPIM_POS2POINT;
    sGui->PropFont->Print(sF2P_SPACE|sF2P_LEFT|sF2P_OPAQUE,r,text,-1,0,0,0,&PrintInfo);
    r.x0 = r.x1 = PrintInfo.QueryX;
    r.y0 = r.y1 = PrintInfo.QueryY;
    PrintInfo.Mode = 0;
    PrintInfo.QueryPos = 0;
    ScrollTo(r,1);
    Update();
  }
  if(change)
  {
    Changed = 1;
    if(ParseBuffer())
    {
      if(!ChangeMsg.IsEmpty()) // only update instantaneously when change msg registered
      {
        if(PoolPtr)
          *PoolPtr = Buffer;
        if(TextBuffer)
          *TextBuffer = Buffer;
        SendChange();
      }
      sGui->Notify(Buffer,Size*sizeof(sChar));
      if(PoolPtr)
        sGui->Notify(PoolPtr,sizeof(PoolPtr));
    }
  }
  
  return result;
}

void sStringControl::DeleteMark()
{
  if(!Buffer) return;
  sInt len = sGetStringLen(Buffer);
  for(sInt i=PrintInfo.SelectEnd;i<=len;i++)
    Buffer[i+PrintInfo.SelectStart-PrintInfo.SelectEnd] = Buffer[i];
  if(PrintInfo.CursorPos == PrintInfo.SelectEnd)
  {
    PrintInfo.CursorPos -= PrintInfo.SelectEnd - PrintInfo.SelectStart;
    PrintInfo.CursorPos = sClamp(PrintInfo.CursorPos,0,sGetStringLen(Buffer));
  }
  MarkMode = 0;
  Changed = 1;
}

void sStringControl::InsertChar(sInt c)
{
  if(!Buffer) return;
  sInt len = sGetStringLen(Buffer);
  for(sInt i=len;i>=PrintInfo.CursorPos;i--)
    Buffer[i+1] = Buffer[i];
  Buffer[PrintInfo.CursorPos++] = c;
  Changed = 1;
}

void sStringControl::ResetCursor()
{
  MarkMode = 0;
  MarkBegin = 0;
  MarkEnd = 0;
  PrintInfo.CursorPos = 0;
  PrintInfo.SelectStart = -1;
  PrintInfo.SelectEnd = -1;
}

sStringDesc sStringControl::GetBuffer()
{
  return sStringDesc(Buffer,Size);
}

/****************************************************************************/

void sStringControl::SetCharPos(sInt x)
{
  sInt pos = x - sGui->PropFont->GetWidth(L" ");
  sInt charPos = sGui->PropFont->GetCharCountFromWidth(pos,Buffer?Buffer:L"");
    if(PrintInfo.CursorPos != charPos || PrintInfo.SelectStart != -1
      || PrintInfo.SelectEnd != -1)
    Update();

  MarkMode = 0;
  MarkBegin = 0;
  MarkEnd = 0;
  PrintInfo.CursorPos = charPos;
  PrintInfo.SelectStart = -1;
  PrintInfo.SelectEnd = -1;
}

void sStringControl::OnDrag(const sWindowDrag &dd)
{
  sInt pos,charPos,start,end;
  sBool update = sFALSE;

  pos = dd.MouseX - Inner.x0 - sGui->PropFont->GetWidth(L" ");
  charPos = sGui->PropFont->GetCharCountFromWidth(pos,Buffer?Buffer:L"");

  switch(dd.Mode)
  {
  case sDD_START:
    MarkMode = 0;
    MarkBegin = 0;
    MarkEnd = 0;
    if(PrintInfo.CursorPos != charPos || PrintInfo.SelectStart != -1
      || PrintInfo.SelectEnd != -1)
      update = sTRUE;
    
    PrintInfo.CursorPos = charPos;
    PrintInfo.SelectStart = -1;
    PrintInfo.SelectEnd = -1;
    break;

  case sDD_DRAG:
  case sDD_STOP:
    if(!MarkMode)
      MarkBegin = charPos;//PrintInfo.CursorPos;
    MarkMode = 1;
    MarkEnd = charPos;

    start = sMin(MarkBegin,MarkEnd);
    end   = sMax(MarkBegin,MarkEnd);

    if(PrintInfo.CursorPos != charPos || PrintInfo.SelectStart != start
      || PrintInfo.SelectEnd != end)
    {
      PrintInfo.CursorPos = charPos;
      PrintInfo.SelectStart = start;
      PrintInfo.SelectEnd = end;
      update = sTRUE;
    }
    if(dd.Mode==sDD_STOP && MarkBegin==MarkEnd)
    {
      MarkMode = 0;
      update = sTRUE;
    }
    if((sGetKeyQualifier()&sKEYQ_CTRL))
      OnKey('a'|sKEYQ_CTRL);
    break;
  }


  if(update)
    Update();
}

sBool sStringControl::OnCommand(sInt cmd)
{
  sBool changed = 0;
  switch(cmd)
  {
  case sCMD_ENTERFOCUS:
    if(PoolPtr)
    {
      sDeleteArray(Buffer);
      Buffer = new sChar[sMAXPATH];
      Size = sMAXPATH;
      sCopyString(Buffer,(const sChar *)(*PoolPtr),Size);
    }
    return 1;

  case sCMD_LEAVEFOCUS:
    changed = Changed;
    if(PoolPtr)
    {
      changed = (sCmpString(*PoolPtr,Buffer)!=0);
      *PoolPtr = Buffer;
      sDeleteArray(Buffer);
      Size = 0;
    }
    if(TextBuffer)
    {
      changed = (sCmpString(TextBuffer->Get(),Buffer)!=0);
      *TextBuffer = Buffer;
    }
    if(changed)
      PostDone();

//    if(!ParseBuffer())
    {
      ParseBuffer();
      MakeBuffer(1);
      ResetCursor();
      Update();
    }
    return 1;
  }
  return 0;
}

/****************************************************************************/

sBool sStringControl::MakeBuffer(sBool unconditional)
{
  return 0;
}

sBool sStringControl::ParseBuffer()
{
  return 1;
}

/****************************************************************************/
/***                                                                      ***/
/***   Byte                                                               ***/
/***                                                                      ***/
/****************************************************************************/

template <> const sChar *sByteControl::ClassName() { return L"sByteControl"; }

template <> void sValueControl<sU8>::MoreInit() { RightStep = 0.25f; }

template <> sBool sValueControl<sU8>::MakeBuffer(sBool unconditional)
{
  if(unconditional || *Value != OldValue)
  { 
    OldValue = *Value;
    sSPrintF(String,Format,OldValue);
    return 1; 
  }
  else 
  { 
    return 0; 
  } 
}

template <> sBool sValueControl<sU8>::ParseBuffer()
{
  const sChar *s = String;
  sInt val;
  sBool ok = 1;
  sInt code;
  
  // scan print format

  code = Format[sGetStringLen(Format)-1];

  // scan value

  while(sIsSpace(*s)) *s++;
  switch(code)
  {
  case 'x':
    ok = sScanHex(s,val);
    break;
  default:
    ok = sScanInt(s,val);
    break;
  }
  while(sIsSpace(*s)) *s++;
  if(*s)
    ok = 0;

  // check range

  if(val<Min || val>Max)
  {
    val = sClamp(val,sInt(Min),sInt(Max));
//    ok = 0;
  }

  // assign and return

  if(ok)
    *Value = OldValue = sU8(val);

  return ok;
}

template <> void sValueControl<sU8>::OnPaint2D()
{
  if(ColorPtr)
  {
    BackColor = 0xff000000|(ColorPtr[2]<<16)|(ColorPtr[1]<<8)|(ColorPtr[0]<<0);
  }
  Percent = sF32(*Value-Min)/(Max-Min);

  sStringControl::OnPaint2D();
}

/****************************************************************************/
/***                                                                      ***/
/***   Word                                                               ***/
/***                                                                      ***/
/****************************************************************************/

template <> const sChar *sWordControl::ClassName() { return L"sWordControl"; }

template <> void sValueControl<sU16>::MoreInit() { RightStep = 0.25f; }

template <> sBool sValueControl<sU16>::MakeBuffer(sBool unconditional)
{
  if(unconditional || *Value != OldValue)
  { 
    OldValue = *Value;
    sSPrintF(String,Format,OldValue);
    return 1; 
  }
  else 
  { 
    return 0; 
  } 
}

template <> sBool sValueControl<sU16>::ParseBuffer()
{
  const sChar *s = String;
  sInt val;
  sBool ok = 1;
  sInt code;

  // scan print format

  code = Format[sGetStringLen(Format)-1];

  // scan value

  while(sIsSpace(*s)) *s++;
  switch(code)
  {
  case 'x':
    ok = sScanHex(s,val);
    break;
  default:
    ok = sScanInt(s,val);
    break;
  }
  while(sIsSpace(*s)) *s++;
  if(*s)
    ok = 0;

  // check range

  if(val<Min || val>Max)
  {
    val = sClamp(val,sInt(Min),sInt(Max));
    //    ok = 0;
  }

  // assign and return

  if(ok)
    *Value = OldValue = sU16(val);

  return ok;
}

template <> void sValueControl<sU16>::OnPaint2D()
{
  if(ColorPtr)
  {
    BackColor = 0xff000000|(ColorPtr[2]<<16)|(ColorPtr[1]<<8)|(ColorPtr[0]<<0);
  }
  Percent = sF32(*Value-Min)/(Max-Min);

  sStringControl::OnPaint2D();
}

/****************************************************************************/
/***                                                                      ***/
/***   Int                                                                ***/
/***                                                                      ***/
/****************************************************************************/

template <> const sChar *sIntControl::ClassName() { return L"sIntControl"; }

template <> void sValueControl<sInt>::MoreInit() { RightStep = 0.25f; }

template <> sBool sValueControl<sInt>::MakeBuffer(sBool unconditional)
{
  if(unconditional || *Value != OldValue)
  { 
    OldValue = *Value;
    sInt i = (OldValue & DisplayMask) >> DisplayShift;
    sSPrintF(String,Format,i);
    return 1; 
  }
  else 
  { 
    return 0; 
  } 
}

template <> sBool sValueControl<sInt>::ParseBuffer()
{
  const sChar *s = String;
  sInt val;
  sBool ok = 1;
  sInt code;
  
  // scan print format

  code = Format[sGetStringLen(Format)-1];

  // scan value

  while(sIsSpace(*s)) *s++;
  switch(code)
  {
  case 'x':
    ok = sScanHex(s,val);
    break;
  default:
    ok = sScanInt(s,val);
    break;
  }
  while(sIsSpace(*s)) *s++;
  if(*s)
    ok = 0;

  // check range

  if(val<Min || val>Max)
  {
    val = sClamp(val,Min,Max);
//    ok = 0;
  }

  // assign and return

  if(ok)
  {
    OldValue = (OldValue & ~DisplayMask) | ((val<<DisplayShift)&DisplayMask);
    *Value = OldValue;
  }

  return ok;
}


template <> void sValueControl<sInt>::OnPaint2D()
{
  if(ColorPtr)
  {
    BackColor = 0xff000000|(ColorPtr[0]<<16)|(ColorPtr[1]<<8)|(ColorPtr[2]<<0);
  }
  Percent = sF32(*Value-Min)/(Max-Min);

  sStringControl::OnPaint2D();
}

/****************************************************************************/
/***                                                                      ***/
/***   Float                                                              ***/
/***                                                                      ***/
/****************************************************************************/

template <> const sChar *sFloatControl::ClassName() { return L"sFloatControl"; }

template <> void sValueControl<sF32>::MoreInit() { RightStep = 0.125f; }

template <> sBool sValueControl<sF32>::MakeBuffer(sBool unconditional)
{
  if(unconditional || *Value != OldValue)
  { 
    OldValue = *Value;
    sSPrintF(String,Format,OldValue);
    return 1; 
  }
  else 
  { 
    return 0; 
  } 
}

template <> sBool sValueControl<sF32>::ParseBuffer()
{
  const sChar *s = String;
  sF32 val;
  sBool ok = 1;
  
  // scan sign

  while(sIsSpace(*s)) *s++;
  ok = sScanFloat(s,val);
  while(sIsSpace(*s)) *s++;
  if(*s)
    ok = 0;

  // check range

  if(val<Min || val>Max)
  {
    val = sClamp(val,Min,Max);
//    ok = 0;
  }

  // assign and return

  if(ok)
    *Value = OldValue = val;

  return ok;
}


template <> void sValueControl<sF32>::OnPaint2D()
{
  if(ColorPtr)
  {
    sInt r= sClamp(sInt(ColorPtr[0]*255),0,255);
    sInt g= sClamp(sInt(ColorPtr[1]*255),0,255);
    sInt b= sClamp(sInt(ColorPtr[2]*255),0,255);
    BackColor = 0xff000000|(r<<16)|(g<<8)|(b<<0);
  }
  Percent = sF32(*Value-Min)/(Max-Min);

  sStringControl::OnPaint2D();
}

/****************************************************************************/
/***                                                                      ***/
/***   Flags                                                              ***/
/***                                                                      ***/
/****************************************************************************/

sFlagControl::sFlagControl()
{
  Val = 0;
  Mask = ~0;
  Shift = 0;
  Choices = L"off|on";
  Max = CountChoices();

  Toggle = 0;
}

sFlagControl::sFlagControl(sInt *val,const sChar *choices)
{
  Val = val;
  Mask = ~0;
  Shift = 0;
  Choices = choices ? choices : L"off|on";
  Max = CountChoices();

  Toggle = 0;
}

sFlagControl::sFlagControl(sInt *val,const sChar *choices,sInt mask,sInt shift)
{
  Val = val;
  Mask = mask;
  Shift = shift;
  Choices = choices ? choices : L"off|on";
  Max = CountChoices();

  Toggle = 0;
}

/****************************************************************************/

sInt sFlagControl::GetValue()
{
  sInt n = ((*Val&Mask)>>Shift);
  return sClamp(n,0,Max-1);
}

void sFlagControl::SetValue(sInt n)
{
  *Val = (*Val & ~Mask) | (n<<Shift);
}

void sFlagControl::NextChoice(sInt delta)
{
  sInt n = GetValue() + delta;

  while(n<0) n+=Max;
  while(n>Max-1) n-=Max;

  SetValue(n);
}

void sFlagControl::MakeChoice(sInt n,const sStringDesc &desc)
{
  const sChar *s = Choices;
  sChar *d = desc.Buffer;
  sChar *e = desc.Buffer+desc.Size;

  sVERIFY(desc.Size>0);

  while(n>0)
  {
    while(*s!=0 && *s!='|')
      s++;
    if(*s=='|')
      s++;
    n--;
  }
  while(*s!=0 && *s!='|' && d<e-1)
    *d++ = *s++;
  *d++ = 0;
}

sInt sFlagControl::CountChoices()
{
  const sChar *s = Choices;
  sInt max = 1;

  while(*s)
  {
    if(*s++ == '|')
      max++;
  }

  return max;
}

/****************************************************************************/
  
void sFlagControl::OnCalcSize()
{
  ReqSizeY = sGui->PropFont->GetHeight()+4;
}

void sFlagControl::OnPaint2D()
{
  sString<256> buffer;
  MakeChoice(GetValue(),buffer);
  sGui->PaintButton(Client,buffer,Toggle);
}

void sFlagControl::OnDrag(const sWindowDrag &dd)
{
  sBool t;
  switch(dd.Mode)
  {
  case sDD_START:
    Toggle = 1;
    Update();
    break;
  case sDD_DRAG:
    t = Client.Hit(dd.MouseX,dd.MouseY);
    if(t!=Toggle)
    {
      Toggle = t;
      Update();
    }
    break;
  case sDD_STOP:
    if(Client.Hit(dd.MouseX,dd.MouseY))
      NextChoice(1);
    Toggle = 0;
    Update();
    break;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Progress bar                                                       ***/
/***                                                                      ***/
/****************************************************************************/

sProgressBarControl::sProgressBarControl()
{
  Percentage = 0.0f;
}

void sProgressBarControl::SetPercentage(sF32 percentage)
{
  Percentage = sClamp(percentage,0.0f,1.0f);
  Update();
}

void sProgressBarControl::OnCalcSize()
{
  ReqSizeY = sGui->PropFont->GetHeight()+4;
}

void sProgressBarControl::OnPaint2D()
{
  sRect rc0,rc1;

  rc0 = Client;
  rc1 = Client;
  rc0.x1 = rc1.x0 = Client.x0 + Percentage * (Client.x1 - Client.x0);
  sRect2D(rc0,sGC_SELECT);
  sRect2D(rc1,sGC_BACK);
}

/****************************************************************************/
/***                                                                      ***/
/***   Bitmask                                                            ***/
/***                                                                      ***/
/****************************************************************************/

sBitmaskControl::sBitmaskControl()
{
  Val = 0;
}

void sBitmaskControl::Init(sU8 *val)
{
  Val = val;
  ClearNotify();
  AddNotify(Val,sizeof(sU8));
}

void sBitmaskControl::OnCalcSize()
{
  ReqSizeY = sGui->PropFont->GetHeight()+4;
}

void sBitmaskControl::OnLayout()
{
  for(sInt i=0;i<=8;i++)
    X[i] = Client.x0 + (Client.SizeX()-1)*i/8;
}

void sBitmaskControl::OnPaint2D()
{
  sRect2D(Client,sGC_RED);
  sRectFrame2D(Client,sGC_DRAW);
  for(sInt i=1;i<8;i++)
    sRect2D(X[i],Client.y0+1,X[i]+1,Client.y1-1,sGC_DRAW);
  for(sInt i=0;i<8;i++)
  {
    sInt col = sGC_DOC;
    if(Val && (*Val & (1<<i)))
      col = sGC_SELECT;
    sRect2D(X[i]+1,Client.y0+1,X[i+1],Client.y1-1,col);
  }
}

void sBitmaskControl::OnDrag(const sWindowDrag &dd)
{
  if(dd.Mode==sDD_START && Val)
  {
    for(sInt i=0;i<8;i++)
    {
      if(dd.StartX>=X[i] && dd.StartX<X[i+1])
      {
        *Val = *Val ^ (1<<i);
        sGui->Notify(Val);
        Update();
      }
    }
  }
}

/****************************************************************************/
/****************************************************************************/

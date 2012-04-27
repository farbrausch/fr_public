/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "gui/textwindow.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Text Window                                                        ***/
/***                                                                      ***/
/****************************************************************************/

sTextWindow::sTextWindow()
{
  TextFlags = sF2P_TOP|sF2P_LEFT|sF2P_MULTILINE;
  EditFlags = 0;
  MarkMode = 0;
  MarkBegin = 0;
  MarkEnd = 0;
  GoodCursorX = 0;
  EnterCmd = 0;
  Font = sGui->PropFont;
  WindowHeight = Font->GetHeight()*5/2;
  Text = 0;
  TextTag = 0;
  BackColor = sGC_BACK;
  DragCursorStart = 0;
  DisableDrag = 0;
  ShowCursorAlways = 0;
  UndoAlloc = 4096;
  UndoBuffer = new UndoStep[UndoAlloc];
  UndoValid = 0;
  UndoIndex = 0;
  HintLine = 0;
  TabSize = 8;
  HintLineColor = sGC_HIGH;

  PrintInfo.Init();
  PrintInfo.SelectBackColor=sGC_SELECT;

  CursorTimer = 0;
  Timer = 0;
}

sTextWindow::~sTextWindow()
{
  UndoClear();
  delete Timer;
  delete[] UndoBuffer;
}

void sTextWindow::InitCursorFlash()
{
  if(!Timer)
    Timer = new sMessageTimer(sMessage(this,&sTextWindow::CmdCursorToggle),125,1);
}

void sTextWindow::Tag()
{
  sWindow::Tag();
  TextTag->Need();
}

void sTextWindow::SetText(sTextBuffer *text)
{
  Text = text;
  MarkMode = 0;
  MarkBegin = 0;
  MarkEnd = 0;
  PrintInfo.CursorPos = 0;
  PrintInfo.SelectStart = 0;
  PrintInfo.SelectEnd = 0;
  GoodCursorX = 0;
  ClearNotify();
  AddNotify(*Text);
  UndoClear();
}

void sTextWindow::OnPaint2D()
{
  if(Text)
  {
    sPrintInfo pi = PrintInfo;
    pi.HintLine = HintLine ? Client.x0 + HintLine * Font->GetWidth(L" ") : 0;
    pi.HintLineColor = HintLineColor;
    pi.CullRect = Inner;
    Font->SetColor(sGC_TEXT,BackColor);
    if(!(Flags & sWF_CHILDFOCUS) || (EditFlags & sTEF_STATIC))
      if(!ShowCursorAlways)
        pi.CursorPos = -1;
    if(GetCursorFlash()==0)
      pi.CursorPos = -1;
    
    TextRect = Client;
    sRect lnr(TextRect);
    if(EditFlags & sTEF_LINENUMBER)
    {
      TextRect.x0 = lnr.x1 = lnr.x0+40;
    }
    sInt h=Font->Print(TextFlags|sF2P_OPAQUE,TextRect,Text->Get(),-1,0,0,0,&pi);
    if(EditFlags & sTEF_LINENUMBER)
    {
      sString<16> str;
      sInt y=lnr.y0;
      sInt fh = Font->GetHeight();
      sInt n=1;
      Font->SetColor(sGC_TEXT,sGC_SELECT);
      while(y<lnr.y1)
      {
        sRect r(lnr.x0,y,lnr.x1,y+fh);
        str.PrintF(L"%4d",n);
        Font->Print(TextFlags|sF2P_OPAQUE,r,str);
        y+=fh;
        n++;
      }
    }
    h = h-Client.y0;
    if(h!=WindowHeight)
    {
      ReqSizeY = WindowHeight = h;
      Layout();
    }
  }
  else
  {
    sRect2D(Client,BackColor);
  }
}

/****************************************************************************/

sBool sTextWindow::GetCursorFlash()
{
  return (CursorTimer & 4)?0:1;
}

void sTextWindow::ResetCursorFlash()
{
  sBool oldcursor = GetCursorFlash();
  CursorTimer = 0;
  if(oldcursor!=GetCursorFlash())
  {
    Update();
    CursorFlashMsg.Send();
  }
}

void sTextWindow::CmdCursorToggle()
{
  sBool oldcursor = GetCursorFlash();
  CursorTimer++;
  if(oldcursor!=GetCursorFlash())
  {
    Update();
    CursorFlashMsg.Send();
  }
}

/****************************************************************************/

void sTextWindow::GetCursorPos(sInt &x,sInt &y)
{
  sPrintInfo pil;
  pil = PrintInfo;
  pil.QueryPos = Text->Get() + PrintInfo.CursorPos;
  pil.QueryX = 0;
  pil.QueryY = 0;
  pil.Mode = sPIM_POS2POINT;

  sInt h = Font->Print(TextFlags|sF2P_OPAQUE,TextRect,Text->Get(),-1,0,0,0,&pil);

  x = pil.QueryX;
  y = pil.QueryY;

  h = h-Client.y0;
  if(h!=WindowHeight)
  {
    ReqSizeY = WindowHeight = h;
    Layout();
  }
}

sInt sTextWindow::FindCursorPos(sInt x,sInt y)
{
  if(Text==0)
    return 0;
  sPrintInfo pil;
  pil = PrintInfo;
  pil.QueryPos = 0;
  pil.QueryX = x;
  pil.QueryY = y;
  pil.Mode = sPIM_POINT2POS;

  Font->Print(TextFlags|sF2P_OPAQUE,TextRect,Text->Get(),-1,0,0,0,&pil);

  if(pil.QueryPos)
    return sInt(pil.QueryPos - Text->Get());
  else
    return Text->GetCount();
}

void sTextWindow::OnCalcSize()
{
  ReqSizeX = Font->GetWidth(L"0")*5;
  ReqSizeY = WindowHeight;
}

/****************************************************************************/

void sTextWindow::OnDrag(const sWindowDrag &dd)
{
  sInt de;
  sInt ss = PrintInfo.SelectStart;
  sInt se = PrintInfo.SelectEnd;
  sInt dummy;

  if(MMBScroll(dd)) return;

  sInt oldpos = PrintInfo.CursorPos;

  switch(dd.Mode)
  {
  case sDD_START:
    DisableDrag = 0;
    if(sGetKeyQualifier()&sKEYQ_SHIFT)
    {
      if(PrintInfo.SelectStart == -1)
      {
        MarkBegin = DragCursorStart = PrintInfo.CursorPos;
        MarkEnd = PrintInfo.CursorPos = FindCursorPos(dd.MouseX,dd.MouseY);
        PrintInfo.SelectStart = sMin(MarkBegin,MarkEnd);
        PrintInfo.SelectEnd   = sMax(MarkBegin,MarkEnd);
      }
    }
    else
    {
      DragCursorStart = FindCursorPos(dd.MouseX,dd.MouseY);
      PrintInfo.CursorPos = DragCursorStart;
    }
    if(((dd.Flags &sDDF_DOUBLECLICK) || (sGetKeyQualifier()&sKEYQ_CTRL))
      && PrintInfo.CursorPos>=0 && Text)
    {
      sInt i;
      const sChar *s = Text->Get();
      MarkMode = 1;

      i = PrintInfo.CursorPos;
      while(i>0 && !sIsSpace(s[i-1])) i--;
      MarkBegin = i;

      i = PrintInfo.CursorPos;
      while(s[i] && !sIsSpace(s[i])) i++;
      MarkEnd = i;

      PrintInfo.SelectStart = sMin(MarkBegin,MarkEnd);
      PrintInfo.SelectEnd   = sMax(MarkBegin,MarkEnd);
      PrintInfo.CursorPos  = PrintInfo.SelectEnd;
      DisableDrag = 1;
    }
    ResetCursorFlash();
    Update();
    break;
  case sDD_DRAG:
    if(!DisableDrag)
    {
      de = FindCursorPos(dd.MouseX,dd.MouseY);
      PrintInfo.CursorPos  = de;
      if(de!=DragCursorStart)
      {
        MarkMode = 1;
        MarkBegin = DragCursorStart;
        MarkEnd = de;
        PrintInfo.SelectStart = sMin(MarkBegin,MarkEnd);
        PrintInfo.SelectEnd   = sMax(MarkBegin,MarkEnd);
      }
      else
      {
        MarkMode = 0;
        MarkBegin = 0;
        MarkEnd = 0;
        PrintInfo.SelectStart = -1;
        PrintInfo.SelectEnd   = -1;
      }
      if(PrintInfo.SelectStart!=ss || PrintInfo.SelectEnd!=se)
        Update();
      ResetCursorFlash();
    }
    break;
  }
  if(PrintInfo.CursorPos!=oldpos)
  {
    GetCursorPos(GoodCursorX,dummy);
    CursorMsg.Post();
  }
}

/****************************************************************************/

sBool sTextWindow::BeginMoveCursor(sBool selmode)
{
  if(!selmode)
  {
    if(MarkMode)
    {
      MarkMode = 0;
      PrintInfo.SelectStart = PrintInfo.SelectEnd = -1;
      return 0;
    }
    else
    {
      return 1;
    }
  }
  else
  {
    if(!MarkMode)
    {
      MarkBegin = PrintInfo.CursorPos;
      sInt dummy;
      GetCursorPos(GoodCursorX,dummy);      // when starting to mark text, use the real cursor pos!
    }
    MarkMode = 1;
    return 1;
  }
}

void sTextWindow::EndMoveCursor(sBool selmode)
{
  if(selmode)
  {
    MarkEnd = PrintInfo.CursorPos;

    PrintInfo.SelectStart = sMin(MarkBegin,MarkEnd);
    PrintInfo.SelectEnd   = sMax(MarkBegin,MarkEnd);
  }
  Update();
}

// "in-line" space, but not linefeeds
static sINLINE sBool IsLineSpace(sChar ch)
{
  return ch == ' ' || ch == '\t';
}

sBool sTextWindow::OnKey(sU32 key)
{
  if(key&sKEYQ_SHIFT) key|=sKEYQ_SHIFT;
  if(key&sKEYQ_CTRL) key|=sKEYQ_CTRL;

  sInt h = Font->GetHeight();
  sBool selmode = (key & sKEYQ_SHIFT) ? 1 : 0;
  sInt oldpos = PrintInfo.CursorPos;
  sInt dummy;

  sBool scrolltocursor = 1;
  switch(key&(0x8000ffff|sKEYQ_SHIFT|sKEYQ_CTRL))
  {
  case sKEY_WHEELUP:
    ScrollTo(ScrollX,ScrollY-14*3);
    Update();
    scrolltocursor = 0;
    break;
  case sKEY_WHEELDOWN:
    ScrollTo(ScrollX,ScrollY+14*3);
    Update();
    scrolltocursor = 0;
    break;
  case sKEY_LEFT:
  case sKEY_LEFT|sKEYQ_SHIFT:
    if(BeginMoveCursor(selmode))
    {
      if(PrintInfo.CursorPos>0)
        PrintInfo.CursorPos--;
      GetCursorPos(GoodCursorX,dummy);
    }
    EndMoveCursor(selmode);
    break;
  case sKEY_RIGHT:
  case sKEY_RIGHT|sKEYQ_SHIFT:
    if(BeginMoveCursor(selmode))
    {
      if(PrintInfo.CursorPos<Text->GetCount())
        PrintInfo.CursorPos++;
      sInt dummy;
      GetCursorPos(GoodCursorX,dummy);
    }
    EndMoveCursor(selmode);
    break;
  case sKEY_UP:
  case sKEY_UP|sKEYQ_SHIFT:
    if(BeginMoveCursor(selmode))
    {
      sInt x,y;
      GetCursorPos(x,y);
      y-=h;
      PrintInfo.CursorPos = FindCursorPos(GoodCursorX,y);
    }
    EndMoveCursor(selmode);
    break;
  case sKEY_DOWN:
  case sKEY_DOWN|sKEYQ_SHIFT:
    if(BeginMoveCursor(selmode))
    {
      sInt x,y;
      GetCursorPos(x,y);
      y+=h;
      PrintInfo.CursorPos = FindCursorPos(GoodCursorX,y);
    }
    EndMoveCursor(selmode);
    break;

  case sKEY_LEFT|sKEYQ_CTRL:
  case sKEY_LEFT|sKEYQ_CTRL|sKEYQ_SHIFT:
    if(BeginMoveCursor(selmode))
    {
      // a line-breaking space counts as word boundary, but only when it's the first space
      // we encounter.
      if(PrintInfo.CursorPos>0 && sIsSpace(Text->GetChar(PrintInfo.CursorPos-1)))
        PrintInfo.CursorPos--;
      while(PrintInfo.CursorPos>0 && IsLineSpace(Text->GetChar(PrintInfo.CursorPos-1)))
        PrintInfo.CursorPos--;
      while(PrintInfo.CursorPos>0 && !sIsSpace(Text->GetChar(PrintInfo.CursorPos-1)))
        PrintInfo.CursorPos--;
      GetCursorPos(GoodCursorX,dummy);
    }
    EndMoveCursor(selmode);
    break;
  case sKEY_RIGHT|sKEYQ_CTRL:
  case sKEY_RIGHT|sKEYQ_CTRL|sKEYQ_SHIFT:
    if(BeginMoveCursor(selmode))
    {
      if(PrintInfo.CursorPos<Text->GetCount() && sIsSpace(Text->GetChar(PrintInfo.CursorPos)))
        PrintInfo.CursorPos++;
      else while(PrintInfo.CursorPos<Text->GetCount() && !sIsSpace(Text->GetChar(PrintInfo.CursorPos)))
        PrintInfo.CursorPos++;
      while(PrintInfo.CursorPos<Text->GetCount() && IsLineSpace(Text->GetChar(PrintInfo.CursorPos)))
        PrintInfo.CursorPos++;
      GetCursorPos(GoodCursorX,dummy);
    }
    EndMoveCursor(selmode);
    break;

  case sKEY_HOME:
  case sKEY_HOME|sKEYQ_SHIFT:
    BeginMoveCursor(selmode);
    {
      sInt x,y;
      GetCursorPos(x,y);
      PrintInfo.CursorPos = FindCursorPos(Inner.x0,y);
      GetCursorPos(GoodCursorX,dummy);
    }
    EndMoveCursor(selmode);
    break;

  case sKEY_END:
  case sKEY_END|sKEYQ_SHIFT:
    BeginMoveCursor(selmode);
    {
      sInt x,y;
      GetCursorPos(x,y);
      PrintInfo.CursorPos = FindCursorPos(Inner.x1,y);
      GetCursorPos(GoodCursorX,dummy);
    }
    EndMoveCursor(selmode);
    break;

  case sKEY_HOME|sKEYQ_CTRL:
  case sKEY_HOME|sKEYQ_CTRL|sKEYQ_SHIFT:
    BeginMoveCursor(selmode);
    {
      PrintInfo.CursorPos = 0;
      GetCursorPos(GoodCursorX,dummy);
    }
    EndMoveCursor(selmode);
    break;

  case sKEY_END|sKEYQ_CTRL:
  case sKEY_END|sKEYQ_CTRL|sKEYQ_SHIFT:
    BeginMoveCursor(selmode);
    {
      PrintInfo.CursorPos = Text ? Text->GetCount() : 0;
      GetCursorPos(GoodCursorX,dummy);
    }
    EndMoveCursor(selmode);
    break;

  case sKEY_PAGEUP:
  case sKEY_PAGEUP|sKEYQ_SHIFT:
    BeginMoveCursor(selmode);
    {
      sInt x,y,s;
      s = Inner.SizeY()-h;
      if(s>h)
      {
        GetCursorPos(x,y);
        y -= s;
        PrintInfo.CursorPos = FindCursorPos(GoodCursorX,y);
      }
    }
    EndMoveCursor(selmode);
    break;

  case sKEY_PAGEDOWN:
  case sKEY_PAGEDOWN|sKEYQ_SHIFT:
    BeginMoveCursor(selmode);
    {
      sInt x,y,s;
      s = Inner.SizeY()-h;
      if(s>h)
      {
        GetCursorPos(x,y);
        y += s;
        PrintInfo.CursorPos = FindCursorPos(GoodCursorX,y);
      }
    }
    EndMoveCursor(selmode);
    break;

  case sKEY_DELETE:
    if(PrintInfo.SelectStart < PrintInfo.SelectEnd)
    {
      DeleteBlock();
      MarkMode = 0;
    }
    else
    {
      DeleteChar();
    }
    GetCursorPos(GoodCursorX,dummy);
    break;
  case sKEY_BACKSPACE:
  case sKEY_BACKSPACE|sKEYQ_SHIFT:
    if(PrintInfo.SelectStart < PrintInfo.SelectEnd)
    {
      DeleteBlock();
      MarkMode = 0;
    }
    else
    {
      if(PrintInfo.CursorPos>0)
      {
        PrintInfo.CursorPos--;
        DeleteChar();
      }
    }
    GetCursorPos(GoodCursorX,dummy);
    break;

  case 'z'|sKEYQ_CTRL:
    Undo();
    GetCursorPos(GoodCursorX,dummy);
    break;

  case 'Z'|sKEYQ_CTRL|sKEYQ_SHIFT:
    Redo();
    GetCursorPos(GoodCursorX,dummy);
    break;

  case sKEY_INSERT:
    MarkMode = 0;
    PrintInfo.Overwrite = !PrintInfo.Overwrite;
    Update();
    break;

  case sKEY_TAB:
    if(MarkMode)
    {
      IndentBlock(2);
    }
    else
    {
      sInt x = GetCursorColumn();
      do
      {
        InsertChar(' ');
        x++;
      }
      while(x % TabSize != 0);
    }
    break;
  case sKEY_TAB|sKEYQ_SHIFT:
    if(MarkMode)
      IndentBlock(-2);
    break;

  case 'a'|sKEYQ_CTRL:
    Mark(0,Text->GetCount());
    break;
  case 'x'|sKEYQ_CTRL:
  case sKEY_DELETE|sKEYQ_SHIFT:
    if(PrintInfo.HasSelection())
    {
      sSetClipboard(Text->Get()+PrintInfo.SelectStart,PrintInfo.SelectEnd-PrintInfo.SelectStart);
      DeleteBlock();
      MarkMode = 0;
    }
    GetCursorPos(GoodCursorX,dummy);
    break;
  case 'c'|sKEYQ_CTRL:
  case sKEY_INSERT|sKEYQ_CTRL:
    if(PrintInfo.HasSelection())
    {
      sSetClipboard(Text->Get()+PrintInfo.SelectStart,PrintInfo.SelectEnd-PrintInfo.SelectStart);
    }
    GetCursorPos(GoodCursorX,dummy);
    break;
  case sKEY_INSERT|sKEYQ_SHIFT:
  case 'v'|sKEYQ_CTRL:
    {
      if(PrintInfo.HasSelection())
        DeleteBlock();
      sChar *t = sGetClipboard();
      InsertString(t);
      delete[] t;
    }
    GetCursorPos(GoodCursorX,dummy);
    MarkOff();
    break;

  default:
    {
      sInt k = (key & 0x8000ffff);
      if(key==sKEY_ENTER)
      {
        Post(EnterCmd);
        EnterMsg.Post();
        ChangeMsg.Post();
        GetCursorPos(GoodCursorX,dummy);
      }
      if((k>=0x20 && k<0xe000) || k==sKEY_ENTER )
      {
        if(MarkMode)
        {
          MarkMode = 0;
          DeleteBlock();
          InsertChar(k);
        }
        else
        {
          if(PrintInfo.Overwrite && Text->GetChar(PrintInfo.CursorPos)!='\n')
            OverwriteChar(k);
          else
            InsertChar(k);
        }
        GetCursorPos(GoodCursorX,dummy);
        break;
      }
    }
    return 0;
  }
  if(scrolltocursor)
    ScrollToCursor();
  ResetCursorFlash();
  if(PrintInfo.CursorPos!=oldpos)
    CursorMsg.Post();
  return 1;
}

/****************************************************************************/

sTextWindow::UndoStep *sTextWindow::UndoGetStep()
{
  if(UndoValid>UndoIndex)
  {
    for(sInt i=UndoIndex;i<UndoValid;i++)
      delete[] UndoBuffer[i].Text;
    UndoValid = UndoIndex;
  }
  if(UndoIndex==UndoAlloc)
  {
    sInt newsize = UndoAlloc*2;
    UndoStep *newbuffer = new UndoStep[newsize];
    for(sInt i=0;i<UndoIndex;i++)
      newbuffer[i] = UndoBuffer[i];
    delete[] UndoBuffer;
    UndoBuffer = newbuffer;
    UndoAlloc = newsize;
  }
  UndoStep *step = &UndoBuffer[UndoIndex++];
  UndoValid = UndoIndex;
  return step;
}


void sTextWindow::UndoFlushCollector()
{
  if(UndoCollectorIndex>0)
  {
    UndoStep *step = UndoGetStep();
    step->Count = UndoCollectorIndex;
    step->Pos = UndoCollectorStart;
    step->Text = new sChar[UndoCollectorIndex];
    step->Delete = 0;
    for(sInt i=0;i<UndoCollectorIndex;i++)
      step->Text[i] = UndoCollector[i];
    UndoCollectorIndex = 0;
    UndoCollectorValid = 0;
  }
}

void sTextWindow::UndoInsert(sInt pos,const sChar *string,sInt len)
{
  if(len==1 && UndoCollectorIndex>0 && (sIsSpace(UndoCollector[UndoCollectorIndex-1]) != sIsSpace(string[0])))
    UndoFlushCollector();
  if(len==1)
  {
    if(UndoCollectorIndex>0 && UndoCollectorStart+UndoCollectorIndex!=pos || UndoCollectorIndex==255)
      UndoFlushCollector();
    if(UndoCollectorIndex==0)
      UndoCollectorStart = pos;
    UndoCollector[UndoCollectorIndex++] = string[0];
    UndoCollectorValid = UndoCollectorIndex;
  }
  else
  {
    UndoFlushCollector();
    UndoStep *step = UndoGetStep();
    step->Count = len;
    step->Pos = pos;
    step->Text = new sChar[len];
    step->Delete = 0;
    for(sInt i=0;i<len;i++)
      step->Text[i] = string[i];
  }
}

void sTextWindow::UndoDelete(sInt pos,sInt size)
{
  UndoFlushCollector();
  UndoStep *step = UndoGetStep();
  step->Count = size;
  step->Pos = pos;
  step->Text = new sChar[size];
  step->Delete = 1;
  const sChar *s = Text->Get();
  for(sInt i=0;i<size;i++)
    step->Text[i] = s[pos+i];
}

void sTextWindow::Undo()
{
  if(EditFlags & sTEF_STATIC) return;
  if(UndoCollectorIndex>0)
  {
    UndoCollectorIndex--;
    Text->Delete(UndoCollectorStart+UndoCollectorIndex,1);
    SetCursor(UndoCollectorStart+UndoCollectorIndex);
  }
  else if(UndoIndex>0)
  {
    UndoStep *step = &UndoBuffer[--UndoIndex];
    if(step->Delete)
    {
      Text->Insert(step->Pos,step->Text,step->Count);
      SetCursor(step->Pos+step->Count);
    }
    else
    {
      Text->Delete(step->Pos,step->Count);
      SetCursor(step->Pos);
    }
  }
  else
    return;

  sGui->Notify(*Text);
  ChangeMsg.Post();
}

void sTextWindow::Redo()
{
  if(UndoValid>UndoIndex)
  {
    UndoStep *step = &UndoBuffer[UndoIndex++];
    if(step->Delete)
    {
      Text->Delete(step->Pos,step->Count);
      SetCursor(step->Pos);
    }
    else
    {
      Text->Insert(step->Pos,step->Text,step->Count);
      SetCursor(step->Pos+step->Count);
    }
  }
  else if(UndoCollectorValid>UndoCollectorIndex)
  {
    Text->Insert(UndoCollectorStart+UndoCollectorIndex,UndoCollector[UndoCollectorIndex]);
    UndoCollectorIndex++;
    SetCursor(UndoCollectorStart+UndoCollectorIndex);
  }
  else
    return;

  sGui->Notify(*Text);
  ChangeMsg.Post();
}

void sTextWindow::UndoClear()
{
  for(sInt i=0;i<UndoValid;i++)
    delete[] UndoBuffer[i].Text;
  UndoValid = 0;
  UndoIndex = 0;
  UndoCollectorIndex = 0;
  UndoCollectorValid = 0;
}

void sTextWindow::UndoGetStats(sInt &undos,sInt &redos)
{
  undos = UndoIndex;
  redos = UndoValid-UndoIndex;
  if(UndoCollectorIndex>0)
    undos++;
  if(UndoCollectorValid>UndoCollectorIndex)
    redos++;
}

/****************************************************************************/

void sTextWindow::Delete(sInt pos,sInt len)
{
  if(EditFlags & sTEF_STATIC) return;
  UndoDelete(pos,len);
  for(sInt i=0;i<len;i++)
  {
    if(PrintInfo.SelectStart > pos)
      PrintInfo.SelectStart--;
    if(PrintInfo.SelectEnd > pos)
      PrintInfo.SelectEnd--;
    if(PrintInfo.CursorPos > pos)
      PrintInfo.CursorPos--;
  }
  Text->Delete(pos,len);
}


void sTextWindow::Insert(sInt pos,const sChar *s,sInt len)
{
  if(EditFlags & sTEF_STATIC) return;
  if(len==-1)
    len = sGetStringLen(s);
  UndoInsert(pos,s,len);
  Text->Insert(pos,s,len);
  if(PrintInfo.SelectStart > PrintInfo.CursorPos)
    PrintInfo.SelectStart += len;
  if(PrintInfo.SelectEnd >= PrintInfo.CursorPos)
    PrintInfo.SelectEnd += len;
  PrintInfo.CursorPos += len;
  MarkBegin = PrintInfo.SelectStart;
  MarkEnd = PrintInfo.SelectEnd;
}

void sTextWindow::Mark(sInt start,sInt end)
{
  if(BeginMoveCursor(1))
  {
    MarkBegin = start;
    PrintInfo.CursorPos = end;
    EndMoveCursor(1);
  }
}

void sTextWindow::MarkOff()
{
  MarkBegin = 0;
  MarkEnd = 0;
  MarkMode = 0;
  PrintInfo.SelectStart = 0;
  PrintInfo.SelectEnd = 0;
}

/****************************************************************************/

void sTextWindow::DeleteBlock()
{
  if(EditFlags & sTEF_STATIC) return;
  if(PrintInfo.SelectStart>=0 && PrintInfo.SelectEnd>=0 && PrintInfo.SelectStart < PrintInfo.SelectEnd)
  {
    Delete(PrintInfo.SelectStart,PrintInfo.SelectEnd-PrintInfo.SelectStart);
    PrintInfo.SelectStart = PrintInfo.SelectEnd = -1;
    Update();
    sGui->Notify(*Text);
    ChangeMsg.Post();
    CursorMsg.Post();
  }
}

void sTextWindow::DeleteChar()
{
  if(EditFlags & sTEF_STATIC) return;
  if(PrintInfo.CursorPos<Text->GetCount())
  {
    Delete(PrintInfo.CursorPos,1);
    Update();
    sGui->Notify(*Text);
    ChangeMsg.Post();
  }
}



void sTextWindow::InsertChar(sInt c)
{
  if(EditFlags & sTEF_STATIC) return;
  sChar s[2];
  s[0] = c;
  s[1] = 0;
  Insert(PrintInfo.CursorPos,s,1);
  sGui->Notify(*Text);
  ChangeMsg.Post();
  CursorMsg.Post();
  Update();
}

void sTextWindow::InsertString(const sChar *s)
{
  if(EditFlags & sTEF_STATIC) return;
  sInt len = sGetStringLen(s);
  Insert(PrintInfo.CursorPos,s,len);
  sGui->Notify(*Text);
  ChangeMsg.Post();
  CursorMsg.Post();
  Update();
}

void sTextWindow::IndentBlock(sInt indent)
{
  if(EditFlags & sTEF_STATIC) return;
  sInt n = PrintInfo.SelectStart;
  {
    const sChar *s = Text->Get();
    while(n>0 && s[n-1]!='\n') n--;
  }

  while(n<PrintInfo.SelectEnd)
  {
    if(indent>0)
    {
      Insert(n,L"        ",indent);
    }
    else
    {
      sInt m=0;
      const sChar *s = Text->Get();
      while(m<-indent && s[n+m]==' ') m++;
      if(m>0)
        Delete(n,m);
    }
    {
      const sChar *s = Text->Get();
      while(n<PrintInfo.SelectEnd && s[n]!='\n')
        n++;
      while(n<PrintInfo.SelectEnd && s[n]=='\n')
        n++;
    }
  }
  sGui->Notify(*Text);
  ChangeMsg.Post();
  CursorMsg.Post();
  Update();
}

void sTextWindow::OverwriteChar(sInt c)
{
  if(EditFlags & sTEF_STATIC) return;
  if(PrintInfo.CursorPos>=Text->GetCount())
  {
    InsertChar(c);
  }
  else
  {
    Text->Set(PrintInfo.CursorPos,c);
    PrintInfo.CursorPos++;
  }
  sGui->Notify(*Text);
  ChangeMsg.Post();
  Update();
}

void sTextWindow::TextChanged()
{
  MarkMode = 0;
  MarkBegin = 0;
  MarkEnd = 0;
  PrintInfo.CursorPos = Text->GetCount();
  PrintInfo.SelectStart = -1;
  PrintInfo.SelectEnd = -1;
  CursorMsg.Post();
  Update();
}

void sTextWindow::SetCursor(sInt p)
{
  sInt c = sClamp(p,0,Text->GetCount());
  if(c!=DragCursorStart)
  {
    DisableDrag = 0;
    DragCursorStart = c;
    PrintInfo.CursorPos = DragCursorStart;

    ScrollToCursor();
    CursorMsg.Post();
    Update();
  }
}

void sTextWindow::SetCursor(const sChar *t)
{
  if(Text)
  {
    const sChar *text = Text->Get();
    if(t>=text && t<text+Text->GetCount())
      SetCursor(t-text);
  }
}

void sTextWindow::SetFont(sFont2D *newFont)
{
  if (newFont)
    Font = newFont;
}

void sTextWindow::ScrollToCursor()
{
  sInt x,y;
  sRect r;

  GetCursorPos(x,y);
  r.Init(x,y,x+1,y+Font->GetHeight());
  ScrollTo(r,1);
}

sFont2D *sTextWindow::GetFont()
{
  return Font;
}

sInt sTextWindow::GetCursorColumn() const
{
  const sChar *s0 = Text->Get();
  const sChar *sc = s0 + PrintInfo.CursorPos;

  sInt n = 0;
  while(sc>s0 && sc[-1]!='\n')
  {
    sc--;
    n++;
  }
  return n;
}

void sTextWindow::Find(const sChar *find,sBool dir,sBool next)
{
  const sChar *text = Text->Get();
  sInt textlen = Text->GetCount();
  sInt findlen = sGetStringLen(find);
  sInt found = -1;
  sInt cursor = PrintInfo.CursorPos;

  if(findlen>0)
  {
    if(dir==0)
    {
      for(sInt i=cursor+next;i<textlen-findlen && found==-1;i++)
        if(sCmpStringILen(text+i,find,findlen)==0)
          found = i;
      for(sInt i=0;i<=cursor && found==-1;i++)
        if(sCmpStringILen(text+i,find,findlen)==0)
          found = i;
    }
    else
    {
      sInt tp = sMin(cursor-next,textlen-findlen);
      for(sInt i=tp;i>=0 && found==-1;i--)
        if(sCmpStringILen(text+i,find,findlen)==0)
          found = i;
      for(sInt i=textlen-findlen;i>=cursor && found==-1;i--)
        if(sCmpStringILen(text+i,find,findlen)==0)
          found = i;
    }
    if(found>=0)
    {
      Mark(found+findlen,found);
      ScrollToCursor();
      CursorMsg.Post();
      Update();
    }
    else
    {
      MarkOff();
      CursorMsg.Post();
      Update();
    }
  }
}

void sTextWindow::GetMark(sInt &start,sInt &end)
{
  if(MarkMode)
  {
    start = MarkBegin;
    end = MarkEnd;
  }
  else
  {
    start = 0;
    end = 0;
  }
}

/****************************************************************************/

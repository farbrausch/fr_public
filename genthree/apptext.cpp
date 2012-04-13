// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "apptext.hpp"

/****************************************************************************/
/****************************************************************************/

sTextControl::sTextControl()
{
  TextAlloc = 0x1000;
  Text = new sChar[TextAlloc];
  Text[0] = 0;

  Cursor = 0;
  Overwrite = 0;
  SelMode = 0;
  SelPos = 0;
  CursorWish = 0;

  DoneCmd = 0;
  CursorCmd = 0;
  ReallocCmd = 0;
  Changed = 0;
  Static = 0;
  RecalcSize = 0;

  File = 0;
  PathName[0] = 0;
  sCopyString(PathName,"Disks/c:/NxN",sDI_PATHSIZE);
}

sTextControl::~sTextControl()
{
  delete[] Text;
}

/****************************************************************************/
/****************************************************************************/

void sTextControl::OnCalcSize()
{
  sChar *p;
  sInt i,x,h;

  SizeX = 4;
  SizeY = 4;

  h = sPainter->GetHeight(sGui->FixedFont);
  p = Text;
  for(;;)
  {
    i = 0;
    while(p[i]!=0 && p[i]!='\n')
      i++;

    SizeY+=h;
    x = sPainter->GetWidth(sGui->FixedFont,p,i)+4+sPainter->GetWidth(sGui->FixedFont," ");
    if(x>SizeX)
      SizeX=x;
    p+=i;
    if(*p==0)
      break;
    if(*p=='\n')
      p++;
  }
}

/****************************************************************************/

void sTextControl::OnPaint()
{
  sChar *p;
  sInt i,x,y,h,xs;
  sInt pos;
  sRect r;
  sInt font;
  sInt s0,s1;

  if(RecalcSize) // usefull for Logwindow
  {
    RecalcSize = 0;
    OnCalcSize();
  }

  x = Client.x0+2;
  y = Client.y0+2;
  font = sGui->FixedFont;
  h = sPainter->GetHeight(font);
  p = Text;
  pos = 0;

  s0 = s1 = 0;
  if(SelMode)
  {
    s0 = sMin(Cursor,SelPos);
    s1 = sMax(Cursor,SelPos);
  }

  r.y0 = y-2;
  r.y1 = y;
  r.x0 = Client.x0;
  r.x1 = Client.x1;
  sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BACK]);

  for(;;)
  {
    i = 0;
    while(p[i]!=0 && p[i]!='\n')
      i++;

    r.y0 = y;
    r.y1 = y+h;
    r.x0 = Client.x0;
    r.x1 = Client.x1;
    if(sGui->CurrentClip.Hit(r))
    {
      sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BACK]);
      if(Cursor>=pos && Cursor<=pos+i)
      {
        r.x0 = x+sPainter->GetWidth(font,p,Cursor-pos);
        if(Overwrite)
        {
          if(Cursor==pos+i)
            r.x1 = x+sPainter->GetWidth(font," ");
          else
            r.x1 = x+sPainter->GetWidth(font,p,Cursor-pos+1);
          sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_SELBACK]);
        }
        else
        {
          r.x1 = r.x0+1;
          r.x0--;
          sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_TEXT]);
        }
      }
      if(SelMode && s0<=pos+i && s1>=pos)
      {
        if(s0<=pos)
          r.x0 = x;
        else
          r.x0 = x+sPainter->GetWidth(font,p,s0-pos);
        if(s1>pos+i)
          r.x1 = x+sPainter->GetWidth(font,p,i)+sPainter->GetWidth(font," ");
        else
          r.x1 = x+sPainter->GetWidth(font,p,s1-pos);
          sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_SELBACK]);
      }
      sPainter->Print(font,x,y,p,sGui->Palette[sGC_TEXT],i);
    }
    xs = sPainter->GetWidth(font,p,i)+4+sPainter->GetWidth(font," ");
    y+=h;
    p+=i;
    pos+=i;
    if(*p==0)
      break;
    if(*p=='\n')
    {
      p++;
      pos++;
    }
  }
  r.y0 = y;
  r.y1 = Client.y1;
  r.x0 = Client.x0;
  r.x1 = Client.x1;
  if(r.y0<r.y1)
    sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BACK]);
}

/****************************************************************************/

void sTextControl::OnKey(sU32 key)
{
  sInt len;
  sU32 ckey;
  sInt i,j;
  sChar buffer[2];

// prepare...

  if(key&sKEYQ_SHIFT) key|=sKEYQ_SHIFT;
  if(key&sKEYQ_CTRL ) key|=sKEYQ_CTRL ;

  len = sGetStringLen(Text);
  if(Cursor>len)
    Cursor = len;

// normal keys

  switch(key&(0x8001ffff|sKEYQ_SHIFT|sKEYQ_CTRL))
  {
  case sKEY_BACKSPACE:
    if(SelMode)
    {
      DelSel();
    }
    else
    {
      if(Cursor==0)
        break;
      Cursor--;
      Engine(Cursor,1,0);
      Post(DoneCmd);
      SelMode = 0;
      OnCalcSize();
      ScrollToCursor();
    }
    break;
  case sKEY_DELETE:
    if(SelMode)
    {
      DelSel();
    }
    else
    {
      Engine(Cursor,1,0);
      OnCalcSize();
      Post(DoneCmd);
    }
    ScrollToCursor();
    break;
  case sKEY_ENTER:
    if(SelMode)
      DelSel();
    i = Cursor-GetCursorX();
    for(j=0;i<Cursor && Text[i]==' ';i++)
      j++;
    if(Cursor>i && Text[Cursor-1]=='{')
      j+=2;
    Engine(Cursor,1,"\n");
    Cursor++;
    for(i=0;i<j;i++)
    {
      Engine(Cursor,1," ");
      Cursor++;
    }
    OnCalcSize();
    ScrollToCursor();
    break;
  case sKEY_PAGEUP:
    len = RealY/sPainter->GetHeight(sGui->FixedFont)-8;
    if(len<1) len = 1;
    for(i=0;i<len;i++)
      OnKey(sKEY_UP);
    break;
  case sKEY_PAGEDOWN:
    len = RealY/sPainter->GetHeight(sGui->FixedFont)-8;
    if(len<1) len = 1;
    for(i=0;i<len;i++)
      OnKey(sKEY_DOWN);
    break;
  case sKEY_INSERT:
    Overwrite = !Overwrite;
    ScrollToCursor();
    break;
  case 'x'|sKEYQ_CTRL:
    OnCommand(sTCC_CUT);
    OnCalcSize();
    ScrollToCursor();
    break;
  case 'c'|sKEYQ_CTRL:
    OnCommand(sTCC_COPY);
    ScrollToCursor();
    break;
  case 'v'|sKEYQ_CTRL:
    OnCommand(sTCC_PASTE);
    OnCalcSize();
    ScrollToCursor();
    break;
  case 'b'|sKEYQ_CTRL:
    OnCommand(sTCC_BLOCK);
    ScrollToCursor();
    break;
  }

//  sDPrintF("key %08x\n",key);
  ckey = key&~(sKEYQ_SHIFT|sKEYQ_ALTGR|sKEYQ_REPEAT);
  if((ckey>=0x20 && ckey<=0x7e) || (ckey>=0xa0 && ckey<=0xff))
  {
    DelSel();
    buffer[0] = ckey;
    buffer[1] = 0;
    if(Overwrite && Cursor<len)
    {
      Engine(Cursor,1,0);
      Engine(Cursor,1,buffer);
      Cursor++;
      Post(DoneCmd);
    }
    else
    {
      Engine(Cursor,1,buffer);
      Cursor++;
      Post(DoneCmd);
    } 
    OnCalcSize();
    ScrollToCursor();
  }
  else
  {
    Parent->OnKey(key);
  }

// cursor movement and shift-block-marking

  switch(key&0x8001ffff)
  {
  case sKEY_LEFT:
  case sKEY_RIGHT:
  case sKEY_UP:
  case sKEY_DOWN:
  case sKEY_HOME:
  case sKEY_END:
    if(SelMode==0 && (key&sKEYQ_SHIFT))
    {
      SelMode = 1;
      SelPos = Cursor;
    }
    if(SelMode==1 && !(key&sKEYQ_SHIFT))
    {
      SelMode = 0;
    }
    break;
  }

  switch(key&0x8001ffff)
  {
  case sKEY_LEFT:
    if(Cursor>0)
      Cursor--;
    ScrollToCursor();
    break;
  case sKEY_RIGHT:
    if(Cursor<len)
      Cursor++;
    ScrollToCursor();
    break;
  case sKEY_UP:
    j = i = CursorWish;
    if(Text[Cursor]=='\n' && Cursor>0)
      Cursor--;
    while(Text[Cursor]!='\n' && Cursor>0)
      Cursor--;
    while(Text[Cursor-1]!='\n' && Cursor>0)
      Cursor--;
    while(i>0 && Text[Cursor]!='\n' && Text[Cursor]!=0)
    {
      Cursor++;
      i--;
    }
    ScrollToCursor();
    CursorWish = j;
    break;
  case sKEY_DOWN:
    j = i = CursorWish;
    while(Text[Cursor]!='\n' && Text[Cursor]!=0)
      Cursor++;
    if(Text[Cursor]=='\n')
    {
      Cursor++;
      while(i>0 && Text[Cursor]!='\n' && Text[Cursor]!=0)
      {
        Cursor++;
        i--;
      }
    }
    ScrollToCursor();
    CursorWish = j;
    break;
  case sKEY_HOME:
    while(Cursor>0 && Text[Cursor-1]!='\n')
      Cursor--;
    ScrollToCursor();
    break;
  case sKEY_END:
    while(Text[Cursor]!='\n' && Text[Cursor]!=0)
      Cursor++;
    ScrollToCursor();
    break;
  }

}

/****************************************************************************/

void sTextControl::OnDrag(sDragData &dd)
{
  if(MMBScrolling(dd,MMBX,MMBY)) return;
  switch(dd.Mode)
  {
  case sDD_START:
    Cursor = SelPos = ClickToPos(dd.MouseX,dd.MouseY);
    CursorWish = GetCursorX();
    Post(CursorCmd);
    SelMode = 1;
    break;
  case sDD_DRAG:
    Cursor = ClickToPos(dd.MouseX,dd.MouseY);
    CursorWish = GetCursorX();
    Post(CursorCmd);
    break;
  }
}

/****************************************************************************/
/****************************************************************************/

sBool sTextControl::OnCommand(sU32 cmd)
{
  sInt s0,s1,len;
  sDiskItem *di;
  sChar buffer[sDI_PATHSIZE];
  sChar *t;

  s0 = sMin(SelPos,Cursor);
  s1 = sMax(SelPos,Cursor);
  len = s1-s0;

  switch(cmd)
  {
  case sTCC_CUT:
    if(SelMode && s0!=s1)
    {
      sGui->ClipboardClear();
      sGui->ClipboardAddText(Text+s0,len);
      Engine(s0,len,0);
      Post(DoneCmd);
      SelMode = 0;
      Cursor = s0;
    }
    return sTRUE;
  case sTCC_COPY:
    if(SelMode && s0!=s1)
    {
      sGui->ClipboardClear();
      sGui->ClipboardAddText(Text+s0,len);
      SelMode = 0;
    }
    return sTRUE;
  case sTCC_PASTE:
    t = sGui->ClipboardFindText();
    if(t && t[0])
    {
      Engine(Cursor,sGetStringLen(t),t);
      Cursor+=sGetStringLen(t);
      Post(DoneCmd);
    }
    return sTRUE;
  case sTCC_BLOCK:
    if(SelMode==0)
    {
      SelMode = 2;
      SelPos = Cursor;
    }
    else
    {
      SelMode = 0;
    }  
    return sTRUE;

  case 3:     // Cancel File Requester
    if(File)
    {
      File->Parent->RemChild(File);
      File = 0;
    }
    return sTRUE;
  case sTCC_OPEN:
    if(!File)
    {
      File = new sFileWindow;
      File->AddTitle("Open File");
      sGui->AddApp(File);
    }
    sGui->SetFocus(File);
    File->OkCmd = 4;
    File->CancelCmd = 3;
    File->SendTo = this;
    Modal = File;
    File->SetPath(PathName);
    return sTRUE;
  case 4:
    if(File)
    {
      File->GetPath(buffer,sizeof(buffer));
      OnCommand(3);
      if(!LoadFile(buffer))
      {
        SetText("");
        (new sDialogWindow)->InitOk(this,"Open File","Load failed",0);
      }
    }
    return sTRUE;

  case sTCC_CLEAR:
    SetText("");
    return sTRUE;      


  case sTCC_SAVEAS:
    if(!File)
    {
      File = new sFileWindow;
      File->AddTitle("Open File");
      sGui->AddApp(File);
    }
    sGui->SetFocus(File);
    File->OkCmd = 5;
    File->CancelCmd = 3;
    File->SendTo = this;
    Modal = File;
    File->SetPath(PathName);
    return sTRUE;
  case 5:
    if(File)
    {
      File->GetPath(PathName,sizeof(PathName));
      OnCommand(3);
      OnCommand(sTCC_SAVE);
    }
    return sTRUE;
  case sTCC_SAVE:
    di = sDiskRoot->Find(PathName,sDIF_CREATE);
    if(di)
    {
      len = sGetStringLen(Text);
      if(di->SetBinary(sDIA_DATA,(sU8 *)Text,len))
      {
        Post(CursorCmd);
        Changed = 0;
      }
      else
      {
        (new sDialogWindow)->InitOk(this,"Save File","Save failed",0);
      }
    }
    return sTRUE;

  default:
    return sFALSE;
  }
}

/****************************************************************************/
/****************************************************************************/

void sTextControl::SetText(sChar *t)
{
  sChar *d;

  Realloc(sGetStringLen(t)+1);

  d = Text;
  while(*t)
  {
    if(*t!='\r')
      *d++ = *t;
    t++;
  }
  *d++ = 0;
  Cursor = 0;
  SelPos = 0;
  SelMode = 0;
  OnCalcSize();
}

/****************************************************************************/

sChar *sTextControl::GetText()
{
  return Text;
}

/****************************************************************************/

void sTextControl::Engine(sInt pos,sInt count,sChar *insert)
{
  sInt len;
  sInt i;

  if(Static)
    return;
  Post(DoneCmd);
  Changed = 1;

  len = sGetStringLen(Text);
  if(insert)
  {
    Realloc(len+count+1);
    for(i=len;i>=pos;i--)
      Text[i+count] = Text[i];
    sCopyMem(Text+pos,insert,count);
  }
  else
  {
    if(pos+count<=len)
      sCopyMem(Text+pos,Text+pos+count,len-pos-count+1);
  }
}

void sTextControl::DelSel()
{
  sInt s0,s1,len;
  if(SelMode)
  {
    s0 = sMin(SelPos,Cursor);
    s1 = sMax(SelPos,Cursor);
    len = s1-s0;
    if(len>0)
    {
      Engine(s0,len,0);
      Cursor = s0;
      SelMode = 0;
    }
  }
}

sBool sTextControl::LoadFile(sChar *name)
{
  sDiskItem *di;
  sU8 *data;
  sInt len;

  di = sDiskRoot->Find(name);
  if(di)
  {
    len = di->GetBinarySize(sDIA_DATA);
    data = new sU8[len+1];
    if(di->GetBinary(sDIA_DATA,data,len))
    {
      data[len] = 0;
      SetText((sChar *)data);
      sCopyString(PathName,name,sDI_PATHSIZE);
      Post(CursorCmd);
      delete[]data;
      return sTRUE;
    }
    delete[]data;
  }
  return sFALSE;
}

/****************************************************************************/
/****************************************************************************/

sInt sTextControl::GetCursorX()
{
  sInt x;
  sInt i;

  i = Cursor-1;
  x = 0;
  while(i>=0 && Text[i]!='\n')
  {
    i--;
    x++;
  }

  return x;
}

/****************************************************************************/

sInt sTextControl::GetCursorY()
{
  sInt i;
  sInt y;

  y=0;
  for(i=0;i<Cursor;i++)
    if(Text[i]<0x20)
      y++;

  return y;
}

/****************************************************************************/

void sTextControl::SetCursor(sInt x,sInt y)
{
  sInt i;

  i = 0;
  while(y>0)
  {
    if(Text[i]==0)
      return;
    if(Text[i]=='\n')
      y--;
    i++;
  }
  while(x>0)
  {
    if(Text[i]==0)
      return;
    if(Text[i]=='\n')
      y--;
    i++;
  }
  Cursor = i;
  CursorWish = x;
  SelMode = 0;
  ScrollToCursor();
}

/****************************************************************************/

void sTextControl::ScrollToCursor()
{
  sInt i,x,y,h,pos;
  sRect r;

  Post(CursorCmd);
  CursorWish = GetCursorX();

  h = sPainter->GetHeight(sGui->FixedFont);
  y = Client.y0+2;
  x = Client.x0+2;
  pos = 0;
  for(;;)
  {
    i = 0;
    while(Text[pos+i]!=0 && Text[pos+i]!='\n')
      i++;
    if(Cursor>=pos && Cursor<=pos+i)
    {
      r.x0 = x+sPainter->GetWidth(sGui->FixedFont,Text+pos,Cursor-pos);
      r.y0 = y;
      r.x1 = r.x0+1;
      r.y1 = y+h;
      ScrollTo(r,sGWS_SAFE);
      return;
    }
    y+=h;
    pos+=i;
    if(Text[pos]==0)
      break;
    if(Text[pos]=='\n')
      pos++;
  }
}

/****************************************************************************/

sInt sTextControl::ClickToPos(sInt mx,sInt my)
{
  sInt i,j,x,y,h,pos;

  h = sPainter->GetHeight(sGui->FixedFont);
  y = Client.y0+2;
  x = Client.x0+2;
  pos = 0;
  for(;;)
  {
    i = 0;
    while(Text[pos+i]!=0 && Text[pos+i]!='\n')
      i++;
    if(my<y+h)
    {
      for(j=1;j<i;j++)
      {
        if(mx<Client.x0+2+sPainter->GetWidth(sGui->FixedFont,Text+pos,j))
          return pos+j-1;
      }
      return pos+i;
    }
    y+=h;
    pos+=i;
    if(Text[pos]==0)
      break;
    if(Text[pos]=='\n')
      pos++;
  }
  return sGetStringLen(Text);
}

/****************************************************************************/

void sTextControl::SetSelection(sInt cursor,sInt sel)
{
  sInt len;

  len = sGetStringLen(Text);
  if(cursor>=0 && sel>=0 && cursor<=len && sel<=len)
  {
    Cursor = cursor;
    SelPos = sel;
    SelMode = 1;
    ScrollToCursor();
  }
}

/****************************************************************************/

void sTextControl::Realloc(sInt size)
{
  sChar *s;
  if(size>TextAlloc)
  {
    size = sMax(size*3/2,TextAlloc*2);
    s = new sChar[size];
    sCopyString(s,Text,size);
    delete[] Text;
    Text = s;
    TextAlloc = size;
    Post(ReallocCmd);
  }
}

/****************************************************************************/

void sTextControl::Log(sChar *s)
{
  sInt tl,sl,stat;

  stat = Static;
  Static = 0;
  tl = sGetStringLen(Text);
  sl = sGetStringLen(s);
  Engine(tl,sl,s);
  Cursor = tl+sl;
  ScrollToCursor();
  Static = stat;
  RecalcSize = sTRUE;
}

/****************************************************************************/

void sTextControl::ClearText()
{
  Text[0] = 0;
  Cursor = 0;
}

/****************************************************************************/
/****************************************************************************/

sTextApp::sTextApp()
{
  sToolBorder *tb;
  sChar *text;


  tb = new sToolBorder;
  tb->AddMenu("File",1);
  tb->AddMenu("Edit",2);

  Status = new sStatusBorder;
  Status->SetTab(0,StatusLine,75);
  Status->SetTab(1,StatusColumn,75);
  Status->SetTab(2,StatusChanged,75);
  Status->SetTab(3,StatusName,0);

  Edit = new sTextControl;
  Edit->AddBorder(tb);
  Edit->AddBorder(Status);
  Edit->AddScrolling(1,1);
  Edit->CursorCmd = 10;
  Edit->DoneCmd = 11;
  AddChild(Edit);

  text = "";
  Edit->SetText(text);

  UpdateStatus();
}

void sTextApp::OnKey(sU32 key)
{
  if(key==sKEY_APPCLOSE)
    Parent->RemChild(this);
}

sBool sTextApp::OnCommand(sU32 cmd)
{
  sMenuFrame *mf;
  sDiskItem *di;

  switch(cmd)
  {
  case 1:     // popup FILE
    mf = new sMenuFrame;
    mf->SendTo = Edit;
    mf->AddBorder(new sNiceBorder);
    mf->AddMenu("New",sTCC_CLEAR,0);
    mf->AddMenu("Open...",sTCC_OPEN,0);
    mf->AddMenu("Save",sTCC_SAVE,0);
    mf->AddMenu("Save As...",sTCC_SAVEAS,0);
    mf->AddSpacer();
    mf->AddMenu("Browser",12,0);
    mf->AddMenu("Exit",sTCC_EXIT,0);
    sGui->AddPulldown(mf);
    return sTRUE;
  case 2:     // popup EDIT
    mf = new sMenuFrame;
    mf->SendTo = Edit;
    mf->AddBorder(new sNiceBorder);
    mf->AddMenu("Cut",sTCC_CUT,'x'|sKEYQ_CTRL);
    mf->AddMenu("Copy",sTCC_COPY,'c'|sKEYQ_CTRL);
    mf->AddMenu("Paste",sTCC_PASTE,'v'|sKEYQ_CTRL);
    mf->AddMenu("Mark Block",sTCC_BLOCK,'b'|sKEYQ_CTRL);
    sGui->AddPulldown(mf);
    return sTRUE;
  case 10:
    UpdateStatus();
    return sTRUE; 
  case 11:
    UpdateStatus();
    return sTRUE; 
  case 12:
    di = sDiskRoot->Find("Programms/Browser");
    if(di)
      di->Cmd(sDIC_EXECUTE,0,0);
    return sTRUE; 
  case sTCC_EXIT:
    Parent->RemChild(this);
    return sTRUE;

  default:
    return sFALSE;
  }
}

void sTextApp::UpdateStatus()
{
  sSPrintF(StatusLine,32,"Line %d",Edit->GetCursorY()+1);
  sSPrintF(StatusColumn,32,"Col %d",Edit->GetCursorX()+1);
  if(Edit->PathName[0]==0)
    sCopyString(StatusName,"(noname)",sizeof(StatusName));
  else
    sCopyString(StatusName,Edit->PathName,sizeof(StatusName));
  StatusChanged[0] = 0;
  if(Edit->Changed)
    sCopyString(StatusChanged,"Changed",32);

}

/****************************************************************************/
/****************************************************************************/


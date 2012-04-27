/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "gui/frames.hpp"
#include "gui/controls.hpp"
#include "gui/borders.hpp"
#include "gui/color.hpp"
#include "gui/textwindow.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Splitter                                                           ***/
/***                                                                      ***/
/****************************************************************************/

sSplitFrame::sSplitFrame()
{
  Drag = 0;
  Count = 0;
  Initialized = 0;
  OldT = 0;
  RelT = 0;

  Knop = 6;
  Proportional = 0;
}

void sSplitFrame::MakeChildData()
{
  Count = Childs.GetCount();

  if(ChildData.GetCount() != Count+1)
  {
    Initialized = 0;
    ChildData.AddMany(Count+1);
    for(sInt i=0;i<Count+1;i++)
    {
      ChildData[i].Pos = 0;
      ChildData[i].StartPos = -1;
      ChildData[i].Align = 0;
      ChildData[i].RelPos = 0;
    }
  }
}

void sSplitFrame::SplitLayout(sInt t)
{
  MakeChildData();

  if(!Initialized)
  {
    for(sInt i=0;i<=Count;i++)
    {
      if(ChildData[i].StartPos==-1)
      {
        ChildData[i].Pos = t * i / Count  +  Knop*i;
      }
      else
      {
        if(ChildData[i].StartPos>0)
          ChildData[i].Pos = ChildData[i].StartPos;
        else
          ChildData[i].Pos = t+ChildData[i].StartPos;
      }
      ChildData[i].RelPos = ChildData[i].Pos;
    }
    Initialized = 1;
    OldT = t;
    RelT = t;
  }
  if(Proportional)
  {
    if(OldT!=t)
    {
      for(sInt i=1;i<Count;i++)
      {
        ChildData[i].Pos = sMulDiv(ChildData[i].RelPos,t,RelT);
      }
    }
    OldT = t;
  }
  else
  {
    if(OldT != t)
    {
      for(sInt i=0;i<=Count;i++)
      {
        if(ChildData[i].Align)
          ChildData[i].Pos -= OldT-t;
      }
    }
    OldT = t;
  }
 
  ChildData[0].Pos = 0;
  ChildData[Count].Pos = t;

  sBool updateall=0;

  for(sInt i=1;i<Drag;i++)
  {
    if(ChildData[i].Pos>ChildData[Drag].Pos-Knop*(Drag-i))
    {
      ChildData[i].Pos = ChildData[Drag].Pos-Knop*(Drag-i);
      updateall = 1;
    }
  }
  for(sInt i=1;i<=Count;i++)
  {
    if(ChildData[i].Pos<ChildData[i-1].Pos+Knop)
    {
      ChildData[i].Pos = ChildData[i-1].Pos+Knop;
      updateall = 1;
    }
    if(ChildData[i].Pos>t-(Count-i)*Knop)
    {
      ChildData[i].Pos = t-(Count-i)*Knop;
      updateall = 1;
    }
    if(ChildData[i].Pos<(i)*Knop)
    {
      ChildData[i].Pos = (i)*Knop;
      updateall = 1;
    }
  }
  if(updateall)
    Update();
}

sInt sSplitFrame::SplitDrag(const sWindowDrag &dd,sInt mousedelta,sInt mousepos)
{
  sBool updateknop = 0;
  switch(dd.Mode)
  {
  case sDD_START:
    Drag = 0;
    for(sInt i=1;i<Count;i++)
    {
      sInt p = ChildData[i].Pos;
      if(mousepos>=p-Knop && mousepos<p)
      {
        Drag = i;
        DragStart = ChildData[i].Pos;
      }
      updateknop = Drag;
    }
    break;
  case sDD_DRAG:
    if(Drag>=1 && Drag<Count)
    {
      ChildData[Drag].Pos = DragStart+mousedelta;
      sRect r; 
      r.Add(Childs[Drag-1]->Outer,Childs[Drag]->Outer);
      sGui->Layout(r);
      for(sInt i=1;i<Count;i++)
        ChildData[Drag].RelPos = ChildData[Drag].Pos;
      RelT = OldT;
    }
    break;
  case sDD_STOP:
    updateknop = Drag;
    Drag = 0;
    break;
  }
  return updateknop;
}

void sSplitFrame::Preset(sInt splitter,sInt value,sBool align)
{
  MakeChildData();
  sVERIFY(splitter>=1 && splitter<Count);
  ChildData[splitter].StartPos = value;
  ChildData[splitter].Align = align;
}
void sSplitFrame::PresetPos(sInt splitter,sInt value)
{
  MakeChildData();
  sVERIFY(splitter>=1 && splitter<Count);
  ChildData[splitter].StartPos = value;
}
void sSplitFrame::PresetAlign(sInt splitter,sBool align)
{
  MakeChildData();
  sVERIFY(splitter>=1 && splitter<Count);
  ChildData[splitter].Align = align;
}

sInt sSplitFrame::GetPos(sInt splitter)
{
  if(splitter>=Count)
    return -1;
  else
    return ChildData[splitter].Pos;
}

/****************************************************************************/

sHSplitFrame::sHSplitFrame()
{
  MousePointer = sMP_SIZENS;
}
/*
void sHSplitFrame::OnCalcSize()
{
  sInt n=-Knop;
  sWindow *w;
  sFORALL(Childs,w)
    n += w->DecoratedSizeY + Knop;

  ReqSizeX = 0;
  ReqSizeY = n;
}
*/
void sHSplitFrame::OnLayout()
{
  sWindow *w;

  SplitLayout(Client.SizeY()+Knop);

  sFORALL(Childs,w)
  {
    sVERIFY(_i>=0 && _i+1<ChildData.GetCount())
    w->Outer.x0 = Client.x0;
    w->Outer.x1 = Client.x1;
    w->Outer.y0 = Client.y0 + ChildData[_i+0].Pos;
    w->Outer.y1 = Client.y0 + ChildData[_i+1].Pos-Knop;
  }
}

void sHSplitFrame::OnPaint2D()
{
  for(sInt i=1;i<Count;i++)
  {
    sBool press = (i==Drag);
    sInt y = Client.y0 + ChildData[i].Pos - Knop;
    sRect2D(Client.x0,y       ,Client.x1,y+1     ,press?sGC_LOW:sGC_HIGH);
    sRect2D(Client.x0,y+1     ,Client.x1,y+Knop-1,sGC_BUTTON);
    sRect2D(Client.x0,y+Knop-1,Client.x1,y+Knop  ,press?sGC_HIGH:sGC_LOW);
  }
}

void sHSplitFrame::OnDrag(const sWindowDrag &dd)
{
  sInt knop = SplitDrag(dd,dd.MouseY-dd.StartY,dd.MouseY-Client.y0);
  if(knop)
  {
    sRect r;
    r.x0 = Client.x0;
    r.x1 = Client.x1;
    r.y0 = Client.y0 + ChildData[knop].Pos-Knop;
    r.y1 = Client.y0 + ChildData[knop].Pos;
    sGui->Update(r);
  }
}

/****************************************************************************/

sVSplitFrame::sVSplitFrame()
{
  MousePointer = sMP_SIZEWE;
}
/*
void sVSplitFrame::OnCalcSize()
{
  sInt n=-Knop;
  sWindow *w;
  sFORALL(Childs,w)
    n += w->DecoratedSizeX + Knop;

  ReqSizeY = 0;
  ReqSizeX = n;
}
*/
void sVSplitFrame::OnLayout()
{
  sWindow *w;

  SplitLayout(Client.SizeX()+Knop);

  sFORALL(Childs,w)
  {
    sVERIFY(_i>=0 && _i+1<ChildData.GetCount())
    w->Outer.y0 = Client.y0;
    w->Outer.y1 = Client.y1;
    w->Outer.x0 = Client.x0 + ChildData[_i+0].Pos;
    w->Outer.x1 = Client.x0 + ChildData[_i+1].Pos-Knop;
  }
}

void sVSplitFrame::OnPaint2D()
{
  for(sInt i=1;i<Count;i++)
  {
    sBool press = (i==Drag);
    sInt p = Client.x0 + ChildData[i].Pos - Knop;
    sRect2D(p       ,Client.y0,p+1     ,Client.y1,press?sGC_LOW:sGC_HIGH);
    sRect2D(p+1     ,Client.y0,p+Knop-1,Client.y1,sGC_BUTTON);
    sRect2D(p+Knop-1,Client.y0,p+Knop  ,Client.y1,press?sGC_HIGH:sGC_LOW);
  }
}

void sVSplitFrame::OnDrag(const sWindowDrag &dd)
{
  sInt knop = SplitDrag(dd,dd.MouseX-dd.StartX,dd.MouseX-Client.x0);
  if(knop)
  {
    sRect r;
    r.y0 = Client.y0;
    r.y1 = Client.y1;
    r.x0 = Client.x0 + ChildData[knop].Pos-Knop;
    r.x1 = Client.x0 + ChildData[knop].Pos;
    sGui->Update(r);
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Menu Strip Frame                                                   ***/
/***                                                                      ***/
/****************************************************************************/

sMenuFrame::sMenuFrame()
{
  SendTo = 0;
  Flags |= sWF_AUTOKILL;
}

sMenuFrame::sMenuFrame(sWindow *w)
{
  SendTo = w;
  AddBorder(new sThickBorder);
  Flags |= sWF_AUTOKILL;
}

sMenuFrame::~sMenuFrame()
{
}
 
void sMenuFrame::Tag()
{
  Item *item;  

  sWindow::Tag();

  SendTo->Need();

  sFORALL(Items,item)
    item->Message.Target->Need();
}

void sMenuFrame::OnCalcSize()
{
  Item *item;  

  for(sInt i=0;i<MaxColumn;i++)
  {
    ColumnHeight[i] = 0;
    ColumnWidth[i] = 0;
  }
  sFORALL(Items,item)
  {
    sInt c = item->Column;
    sVERIFY(c>=0 && c<MaxColumn);
    ColumnHeight[c] += item->Window->DecoratedSizeY;
    ColumnWidth[c] = sMax(ColumnWidth[c],item->Window->DecoratedSizeX);
  }
  
  ReqSizeX = -1;
  ReqSizeY = 0;
  for(sInt i=0;i<MaxColumn;i++)
  {
    if(ColumnWidth[i]>0)
    {
      ReqSizeX += ColumnWidth[i]+1;
      ReqSizeY = sMax(ReqSizeY,ColumnHeight[i]);
    }
  }
}

void sMenuFrame::OnLayout()
{
  sWindow *w;
  Item *item;
  sInt x,c;

  sInt pos[MaxColumn+1];
  sInt y[MaxColumn];

  pos[0] = 0;
  x = 0;
  for(sInt i=0;i<MaxColumn;i++)
  {
    if(ColumnWidth[i]>0)
      x += ColumnWidth[i]+1;
    pos[i+1] = x;
    y[i] = 0;
  }

  sFORALL(Items,item)
  {
    w = item->Window;
    c = item->Column;

    w->Outer.x0 = Client.x0+pos[c];
    w->Outer.x1 = Client.x0+pos[c+1]-1;
    w->Outer.y0 = Client.y0+y[c]; y[c]+=w->DecoratedSizeY;
    w->Outer.y1 = Client.y0+y[c]; 
  }
}

void sMenuFrame::Kill()
{
  if(Flags & sWF_CHILDFOCUS)
  {
    if(SendTo)
      sGui->SetFocus(SendTo);
    else
      sGui->SetFocus(0);
  }
  Close();
}

sBool sMenuFrame::OnShortcut(sU32 key)
{
  Item *item;
  if(key==sKEY_ESCAPE)
  {
    Kill();
    return 1;
  }
  else
  {
    key &= ~sKEYQ_CAPS;
    key = (key & ~sKEYQ_MASK) | sMakeUnshiftedKey(key & sKEYQ_MASK);
    if(key & sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
    if(key & sKEYQ_CTRL) key |= sKEYQ_CTRL;
    sFORALL(Items,item)
    {
      if(item->Shortcut == key)
      {
        CmdPressed(_i);
        break;
      }
    }
    return 0;
  }
}

/* // this seems not needed anymore. sWF_AUTOKILL does it all!
sBool sMenuFrame::OnCommand(sInt cmd)
{
  if(cmd==sCMD_DUMMY)
  {
    Kill();
    return 1;
  }
  return 0;
}
*/

void sMenuFrame::CmdPressed(sDInt id)
{
  Item *item;

  item = &Items[id];
  item->Message.Post();

  Kill();
}

void sMenuFrame::CmdPressedNoKill(sDInt id)
{
  Item *item;

  item = &Items[id];
  item->Message.Post();
}

void sMenuFrame::OnPaint2D()
{
  sInt x0;
  sInt x1;
  sInt x = Client.x0;
  for(sInt i=0;i<MaxColumn;i++)
  {
    if(ColumnWidth[i]>0)
    {
      x0 = x;
      x += ColumnWidth[i];
      x1 = x;
      if(Client.y0+ColumnHeight[i]<Client.y1)
        sRect2D(x0,Client.y0+ColumnHeight[i],x1,Client.y1,sGC_BACK);
      x++;
      if(x<Client.x1)
        sRect2D(x-1,Client.y0,x,Client.y1,sGC_DRAW);
    }
  }
}

/****************************************************************************/

void sMenuFrame::AddItem(const sChar *name,const sMessage &msg,sU32 Shortcut,sInt len,sInt column,sU32 backcol)
{
  sButtonControl *con;
  Item *item;
  sInt id;
  
  Shortcut = (Shortcut & ~sKEYQ_MASK) | sMakeUnshiftedKey(Shortcut & sKEYQ_MASK);
  id = Items.GetCount();

  con = new sButtonControl(name,sMessage(this,&sMenuFrame::CmdPressed,id),sBCS_NOBORDER);
  con->LabelLength = len;
  con->Shortcut = Shortcut;
  con->BackColor = backcol;
  AddChild(con);

  item = Items.AddMany(1);
  item->Message = msg;
  item->Shortcut = Shortcut;
  item->Column = column;
  item->Window = con;
}

void sMenuFrame::AddCheckmark(const sChar *name,const sMessage &msg,sU32 Shortcut,sInt *checkref,sInt checkvalue,sInt len,sInt column,sU32 backcol,sInt buttonstyle)
{
  sButtonControl *con;
  Item *item;
  sInt id;
  
  Shortcut = (Shortcut & ~sKEYQ_MASK) | sMakeUnshiftedKey(Shortcut & sKEYQ_MASK);
  id = Items.GetCount();

  con = new sButtonControl(name,sMessage(this,&sMenuFrame::CmdPressedNoKill,id),sBCS_NOBORDER);
  con->LabelLength = len;
  con->Shortcut = Shortcut;
  con->BackColor = backcol;
  if(buttonstyle)
  {
    con->InitCheckmark(checkref,checkvalue);
    con->Style |= buttonstyle;
  }
  else
  {
    if(checkvalue==-1)
    {
      con->InitCheckmark(checkref,1);
      con->Style |= sBCS_TOGGLE;
    }
    else
    {
      con->InitCheckmark(checkref,checkvalue);
      con->Style |= sBCS_RADIO;
    }
  }
  AddChild(con);

  item = Items.AddMany(1);
  item->Message = msg;
  item->Shortcut = Shortcut;
  item->Column = column;
  item->Window = con;
}

/****************************************************************************/

class sMenuFrameSpacer : public sWindow
{
public:
  sCLASSNAME(sMenuFrameSpacer);
  void OnCalcSize()
  {
    ReqSizeX = 0;
    ReqSizeY = 12;
  }
  void OnPaint2D()
  {
    sRect2D(Client,sGC_BACK);
    sGui->RectHL(sRect(Client.x0+5,Client.y0+5,Client.x1-5,Client.y0+7),sTRUE);
  }
};

void sMenuFrame::AddSpacer(sInt column)
{
  Item *item;
  item = Items.AddMany(1);
  item->Message = sMessage();
  item->Shortcut = 0;
  item->Column = column;
  item->Window = new sMenuFrameSpacer;
  AddChild(item->Window);
}
/****************************************************************************/

class sMenuFrameHeader : public sWindow
{
public:
  sCLASSNAME_NONEW(sMenuFrameHeader);
  sPoolString Name;

  sMenuFrameHeader(sPoolString name) { Name = name; }
  void OnCalcSize()
  {
    ReqSizeX = sGui->PropFont->GetWidth(Name)+sGui->PropFont->GetWidth(L"  ");
    ReqSizeY = sGui->PropFont->GetHeight()+1;
  }
  void OnPaint2D()
  {
    sRect r;
    r = Client;
    r.y1--;
    sGui->PropFont->SetColor(sGC_TEXT,sGC_BUTTON);
    sGui->PropFont->Print(sF2P_OPAQUE,r,Name);
    sRect2D(Client.x0,Client.y1-1,Client.x1,Client.y1,sGC_DRAW);
  }
};

void sMenuFrame::AddHeader(sPoolString name,sInt column)
{
  Item *item;
  item = Items.AddMany(1);
  item->Message = sMessage();
  item->Shortcut = 0;
  item->Column = column;
  item->Window = new sMenuFrameHeader(name);
  AddChild(item->Window);
}

void sMenuFrame::AddChoices(const sChar *choices,const sMessage &msg_)
{
  sMessage msg(msg_);
  sInt n=0;
  for(;;)
  {
    while(*choices=='|')
    {
      n++;
      choices++;
    }
    if(!*choices)
      break;
    if(sIsDigit(*choices))
      sScanInt(choices,n);
    while(*choices==' ') 
      choices++;
    const sChar *start = choices;
    while(*choices!=0 && *choices!='|')
      choices++;

    msg.Code = n;
    AddItem(start,msg,0,choices-start);
  }
}


/****************************************************************************/

void sPopupChoices(const sChar *choices,const sMessage &msg)
{

  sMenuFrame *mf = new sMenuFrame((sWindow *)msg.Target);
//  mf->AddBorder(new sThickBorder);

  mf->AddChoices(choices,msg);

  sGui->AddPulldownWindow(mf);
}

/****************************************************************************/
/***                                                                      ***/
/***   Layout Frame Window                                                ***/
/***                                                                      ***/
/****************************************************************************/

sLayoutFrameWindow::sLayoutFrameWindow(sLayoutFrameWindowMode mode,sWindow *window,sInt pos) 
{
  Mode = mode;
  Window = window;
  Pos = pos;
  Align = pos<0;
  Temp = 0;
  Switch = 0;
  Proportional = 0;
}

sLayoutFrameWindow::~sLayoutFrameWindow() 
{
  sDeleteAll(Childs); 
}

void sLayoutFrameWindow::Add(sLayoutFrameWindow *w) 
{ 
  Childs.AddTail(w); 
}

/****************************************************************************/
/****************************************************************************/

sLayoutFrame::sLayoutFrame()
{
  CurrentScreen = -1;
}

sLayoutFrame::~sLayoutFrame()
{
  sDeleteAll(Screens);
}

void sLayoutFrame::Tag()
{
  sWindow::Tag();
  sNeed(Windows);
}

void sLayoutFrame::SetSubSwitchR(sLayoutFrameWindow *p,sPoolString name,sInt nr)
{
  sLayoutFrameWindow *c;

  if(p->Name==name)
    p->Switch = nr;
  sFORALL(p->Childs,c)
    SetSubSwitchR(c,name,nr);
}

void sLayoutFrame::GetSubSwitchR(sLayoutFrameWindow *p,sPoolString name,sInt &nr)
{
  sLayoutFrameWindow *c;

  if(p->Name==name)
    nr = p->Switch;
  sFORALL(p->Childs,c)
    GetSubSwitchR(c,name,nr);
}

void sLayoutFrame::SetSubSwitch(sPoolString name,sInt nr)
{
  sLayoutFrameWindow *lfw;
  sFORALL(Screens,lfw)
    SetSubSwitchR(lfw,name,nr);
}

sInt sLayoutFrame::GetSubSwitch(sPoolString name)
{
  sInt nr = -1;
  sLayoutFrameWindow *lfw;
  sFORALL(Screens,lfw)
    GetSubSwitchR(lfw,name,nr);

  return nr;
}

/****************************************************************************/

void sLayoutFrame::Switch(sInt screen)
{
  sWindow *win;

  if(CurrentScreen!=-1)
    Screens[CurrentScreen]->Cleanup(Childs[0],this);

  CurrentScreen = screen;

  sFORALL(Windows,win)
    win->Temp = 0;

  Childs.Clear();
  if(CurrentScreen!=-1)
  {
    Screens[CurrentScreen]->Layout(this,this);
    Screens[CurrentScreen]->OnSwitch.Post();
  }

  sGui->Layout();
}

void sLayoutFrameWindow::Layout(sWindow *parent,sLayoutFrame *root)
{
  sSplitFrame *split;
  sLayoutFrameWindow *lfw;
  sInt i;
  switch(Mode)
  {
  case sLFWM_WINDOW:
    sVERIFY(Window);
    sVERIFY(sFindPtr(root->Windows,Window));
    sVERIFY(Window->Temp==0);
    Window->Temp = 1;

    parent->AddChild(Window);
    sFORALL(Childs,lfw)
      lfw->Layout(Window,root);
    break;

  case sLFWM_BORDER:
    sVERIFY(Window);
    sVERIFY(sFindPtr(root->Windows,Window));
    sVERIFY(Window->Temp==0);
    Window->Temp = 1;

    parent->AddBorder(Window);
    sFORALL(Childs,lfw)
      lfw->Layout(Window,root);
    break;

  case sLFWM_BORDERPRE:
    sVERIFY(Window);
    sVERIFY(sFindPtr(root->Windows,Window));
    sVERIFY(Window->Temp==0);
    Window->Temp = 1;

    parent->AddBorderHead(Window);
    sFORALL(Childs,lfw)
      lfw->Layout(Window,root);
    break;

  case sLFWM_SWITCH:
    sVERIFY(Switch>=0 && Switch<Childs.GetCount());
    Childs[Switch]->Layout(parent,root);
    break;

  case sLFWM_HORIZONTAL:
  case sLFWM_VERTICAL:

    sVERIFY(Window);
    sVERIFY(sFindPtr(root->Windows,Window));
    sVERIFY(Window->Temp==0);
    Window->Temp = 1;

    parent->AddChild(Window);
    sFORALL(Childs,lfw)
      lfw->Layout(Window,root);

    split = (sSplitFrame *) Window;
    split->Proportional = Proportional;
    i = 0;
    sFORALL(Childs,lfw)
    {
      if(lfw->Mode!=sLFWM_BORDER && lfw->Mode!=sLFWM_BORDERPRE)
      {
        if(lfw->Align && i>0)
          split->PresetAlign(i,1);
        if(lfw->Pos>0)
          split->PresetPos(i+1,lfw->Pos);
        if(lfw->Pos<0)
          split->PresetPos(i,lfw->Pos);
        i++;
      }
    }
    break;
  }
}

void sLayoutFrameWindow::Cleanup(sWindow *win,sLayoutFrame *root)
{
  sLayoutFrameWindow *lfw;
  sWindow *c;

  sFORALL(Childs,lfw)
    lfw->Cleanup(lfw->Window,root);
  sFORALL(win->Childs,c)
    c->Temp = sFindPtr(root->Windows,c);
  sRemTrue(win->Childs,&sWindow::Temp);
  sFORALL(win->Borders,c)
    c->Temp = sFindPtr(root->Windows,c);
  sRemTrue(win->Borders,&sWindow::Temp);
}

/****************************************************************************/

sSwitchFrame::sSwitchFrame()
{
  CurrentScreen = 0;
}

sSwitchFrame::~sSwitchFrame()
{
}

void sSwitchFrame::Tag()
{
  sWindow::Tag();
  sNeed(Windows);
}

void sSwitchFrame::Switch(sInt screen)
{
  Childs.Clear();
  if(screen!=-1)
    Childs.AddTail(Windows[screen]);
  CurrentScreen = screen;

  sGui->Layout();
}

/****************************************************************************/
/***                                                                      ***/
/***   Grid Frame Window                                                  ***/
/***                                                                      ***/
/****************************************************************************/

sGridFrame::sGridFrame()
{
  Columns = 12;
  Height = sGui->PropFont->GetHeight()+4;

  Flags |= sWF_OVERLAPPEDCHILDS;
}

sGridFrame::~sGridFrame()
{
}

void sGridFrame::Tag()
{
  sGridFrameLayout *lay;

  sWindow::Tag();

  sFORALL(Layout,lay)
    lay->Window->Need();
}

/****************************************************************************/

void sGridFrame::OnCalcSize()
{
  sGridFrameLayout *lay;

  sInt ymax=0;

  sFORALL(Layout,lay)
    ymax = sMax(ymax,lay->GridRect.y1);

  ReqSizeY = Height * ymax;
}

void sGridFrame::OnLayout()
{
  sGridFrameLayout *lay;

  sInt xs = Client.SizeX();
  sInt ys = Height;
  sInt x = Client.x0;
  sInt y = Client.y0;

  sFORALL(Layout,lay)
  {
    if(lay->Window)
    {
      lay->Window->Outer.x0 = x + xs*lay->GridRect.x0/Columns;
      lay->Window->Outer.x1 = x + xs*lay->GridRect.x1/Columns;
      lay->Window->Outer.y0 = y + ys*lay->GridRect.y0;
      lay->Window->Outer.y1 = y + ys*lay->GridRect.y1;
      if(lay->Flags & sGFLF_HALFUP)
      {
        lay->Window->Outer.y0 -= ys/2;
        lay->Window->Outer.y1 -= ys/2;
      }
    }
  }
}

void sGridFrame::OnPaint2D()
{
  sGridFrameLayout *lay;
  sRect r;

  sInt xs = Client.SizeX();
  sInt ys = Height;
  sInt x = Client.x0;
  sInt y = Client.y0;

  sClipPush();
  sFORALL(Layout,lay)
  {
    if(lay->Label || (lay->Flags & sGFLF_GROUP) || (lay->Flags & sGFLF_LEAD))
    {
      r.x0 = x + xs*lay->GridRect.x0/Columns;
      r.x1 = x + xs*lay->GridRect.x1/Columns;
      r.y0 = y + ys*lay->GridRect.y0;
      r.y1 = y + ys*lay->GridRect.y1;
      if(lay->Flags & sGFLF_HALFUP)
      {
        r.y0 -= ys/2;
        r.y1 -= ys/2;
      }

      sGui->PropFont->SetColor(sGC_TEXT,sGC_BACK);
      if(lay->Flags & sGFLF_GROUP)
      {
        sInt h = r.SizeY()/2;
        if(lay->Label)
        {
          sRect rr;
          sGui->PropFont->Print(sF2P_OPAQUE,r,lay->Label);
          sInt w = sGui->PropFont->GetWidth(lay->Label);

          sInt hh = h;
          if(lay->Flags & sGFLF_NARROWGROUP) { hh = 1; h/=2; }

          rr.Init(r.x0+h,r.CenterY(),r.CenterX()-w/2-hh,r.CenterY()+2);
          if(rr.SizeX()>0)
            sGui->RectHL(rr,sTRUE);
          rr.Init(r.CenterX()+w/2+hh,r.CenterY(),r.x1-h,r.CenterY()+2);
          if(rr.SizeX()>0)
            sGui->RectHL(rr,sTRUE);
        }
        else
        {
          sRect2D(r,sGC_BACK);
          sGui->RectHL(sRect(r.x0+h,r.CenterY(),r.x1-h,r.CenterY()+2),sTRUE);
        }
      }
      else if(lay->Flags & sGFLF_LEAD)
      {
        sInt h = r.SizeY()/2;
        sRect2D(r,sGC_BACK);
        sInt y = r.CenterY();
        sRect2D(r.x0+h,y,r.x1-h,y+1,sGC_BUTTON);
      }
      else
      {
        sInt flags = sF2P_SPACE|sF2P_RIGHT|sF2P_OPAQUE;
        if(flags & sGFLF_CENTER)
          flags = sF2P_OPAQUE;
        sGui->PropFont->Print(flags,r,lay->Label);
      }
      sClipExclude(r);
    }
  }
  sRect2D(Client,sGC_BACK);

  sClipPop();
}

void sGridFrame::OnDrag(const sWindowDrag &dd)
{
  MMBScroll(dd);
}

void sGridFrame::Reset()
{
  Childs.Clear();
  Layout.Clear();
  sWindow::Layout();
}

void sGridFrame::AddGrid(sWindow *win,sInt x,sInt y,sInt xs,sInt ys,sInt flags)
{
  sGridFrameLayout *lay;

  AddChild(win);

  lay = Layout.AddMany(1);
  lay->Window = win;
  lay->Label = 0;
  lay->Flags = flags;
  lay->GridRect.Init(x,y,x+xs,y+ys);
}

void sGridFrame::AddLabel(const sChar *str,sInt x,sInt y,sInt xs,sInt ys,sInt flags)
{
  sGridFrameLayout *lay;

  lay = Layout.AddMany(1);
  lay->Window = 0;
  lay->Label = str;
  lay->Flags = flags;
  lay->GridRect.Init(x,y,x+xs,y+ys);
}

sBool sGridFrame::OnKey(sU32 key)
{
  sInt inc = 0;
  key = key & ~sKEYQ_CAPS;
  if(key & sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
   
  if(key==sKEY_TAB)
    inc = 1;
  else if(key==(sKEY_TAB|sKEYQ_SHIFT))
    inc = -1;

  if(inc)
  {
    sWindow *w;
    sInt n = -1;
    sFORALL(Childs,w)
      if(w->Flags & sWF_CHILDFOCUS)
        n = _i;
    if(n>=0)
    {
      n = (n + inc+Childs.GetCount()) % Childs.GetCount();
      sGui->SetFocus(Childs[n]);
    }

    return 1;
  }
  return 0;
}

/****************************************************************************/
/****************************************************************************/

sGridFrameHelper::sGridFrameHelper(sGridFrame *grid)
{
  Grid = grid;
  LabelWidth = 3;
  ControlWidth = 2;
  WideWidth = Grid->Columns-3-LabelWidth;
  BoxWidth = 1;
  Reset();
  TieMode = 0;
  TiePrev = 0;
  TieFirst = 0;
  Static = 0;

  ScaleRange = 256;
  ScaleStep = 0.01f;
  RotateRange = 4;
  RotateStep = 0.01f;
  TranslateRange = 4096;
  TranslateStep = 0.04f;
}

sGridFrameHelper::~sGridFrameHelper()
{
  sVERIFY(TieMode == 0);
}

void sGridFrameHelper::InitControl(sControl *con)
{
  con->ChangeMsg = ChangeMsg;
  con->DoneMsg = DoneMsg;
}

void sGridFrameHelper::Reset()
{
  Line = 0;
  Left = LabelWidth;
  Right = Grid->Columns;
  EmptyLine = 1;
  Grid->Reset();
}

void sGridFrameHelper::NextLine()
{
  if(!EmptyLine)
  {
    Left = LabelWidth;
    Right = Grid->Columns;
    EmptyLine = 1;
    Line++;
  }
}

void sGridFrameHelper::Label(const sChar *label)
{
  NextLine();
  Grid->AddLabel(label,0,Line,LabelWidth,1,0);
  Left = LabelWidth; // this is not as redundant as it seems!
  EmptyLine = 0;
}

void sGridFrameHelper::LabelC(const sChar *label)
{
  if(Left+LabelWidth+ControlWidth>Right)
  {
    Label(label);
  }
  else
  {
    Grid->AddLabel(label,Left,Line,LabelWidth,1,0);
    Left += LabelWidth;
    EmptyLine = 0;
  }
}

void sGridFrameHelper::Group(const sChar *label)
{
  NextLine();
  Grid->AddLabel(label,0,Line,Grid->Columns,1,1);
  EmptyLine = 0;
  NextLine();
}

void sGridFrameHelper::GroupCont(const sChar *label)
{
  Grid->AddLabel(label,Left,Line,Grid->Columns,1,1);
  EmptyLine = 0;
  NextLine();
}

void sGridFrameHelper::Textline(const sChar *text)
{
  NextLine();
  Grid->AddLabel(text,0,Line,Grid->Columns,1,0);
  EmptyLine = 0;
  NextLine();
}

class sButtonControl *sGridFrameHelper::PushButton(const sChar *label,const sMessage &done,sInt layoutflags)
{
  if(Left+ControlWidth>Right) NextLine();
  sButtonControl *con = new sButtonControl();
  con->DoneMsg = done;
  con->Label = label;
  if(Static) con->Style |= sBCS_STATIC;
  Grid->AddGrid(con,Left,Line,ControlWidth,1,layoutflags);
  Left += ControlWidth;
  EmptyLine = 0;
  return con;
}

class sButtonControl *sGridFrameHelper::Box(const sChar *label,const sMessage &done,sInt layoutflags)
{
  if(Left>Right-BoxWidth) NextLine();
  sButtonControl *con = new sButtonControl();
  con->DoneMsg = done;
  con->Label = label;
  if(Static) con->Style |= sBCS_STATIC;
  Grid->AddGrid(con,Right-BoxWidth,Line,BoxWidth,1,layoutflags);
  Right -= BoxWidth;
  EmptyLine = 0;
  return con;
}

class sButtonControl *sGridFrameHelper::BoxToggle(const sChar *label,sInt *value,const sMessage &done,sInt layoutflags)
{
  if(Left>Right-BoxWidth) NextLine();
  sButtonControl *con = new sButtonControl();
  con->InitToggle(value);
  con->DoneMsg = done;
  con->Label = label;
  if(Static) con->Style |= sBCS_STATIC;
  Grid->AddGrid(con,Right-BoxWidth,Line,BoxWidth,1,layoutflags);
  Right -= BoxWidth;
  EmptyLine = 0;
  return con;
}

void sGridFrameHelper::BoxFileDialog(const sStringDesc &string,const sChar *text,const sChar *ext,sInt flags)
{
  if(Left>Right-BoxWidth) NextLine();
  sButtonControl *con = new sFileDialogControl(string,text,ext,flags);
  if(Static) con->Style |= sBCS_STATIC;
  Grid->AddGrid(con,Right-BoxWidth,Line,BoxWidth,1,0);
  Right -= BoxWidth;
  EmptyLine = 0;
}

void sGridFrameHelper::BeginTied()
{
  sVERIFY(TieMode==0);
  TieMode = 1;
  TieFirst = 0;
  TiePrev = 0;
}

void sGridFrameHelper::EndTied()
{
  sVERIFY(TieMode==2);
  TiePrev->DragTogether = TieFirst;
  TieMode = 0;
}

void sGridFrameHelper::Tie(sStringControl *vc)
{
  switch(TieMode)
  {
  case 1:
    TieFirst = vc;
    TiePrev = vc;
    TieMode = 2;

    break;
  case 2:
    TiePrev->DragTogether = vc;
    TiePrev = vc;
    break;
  }
}

void sGridFrameHelper::SetColumns(sInt left,sInt middle,sInt right)
{
  Left = left;
  Right = left+middle;
  Grid->Columns = left+middle+right;
}

void sGridFrameHelper::MaxColumns(sInt left,sInt middle,sInt right)
{
  if(Left<left)
  {
    Right += left-Left;
    Grid->Columns += left-Left;
    Left = left;
  }
  if(Right-Left < middle)
  {
    Grid->Columns += middle - (Right-Left);
    Right = Left+middle;
  }
  if(Grid->Columns < left+middle+right)
    Grid->Columns = left+middle+right;
}

/****************************************************************************/

void sGridFrameHelper::Radio(sInt *val,const sChar *choices,sInt width)
{
  sInt id=0;
  sInt len;
  if(width==-1)
    width = ControlWidth;
  for(;;)
  {
    while(*choices=='|')
    {
      choices++;
      id++;
    }
    if(*choices==0)
      break;
    len = 0;
    while(choices[len]!=0 && choices[len]!='|')
      len++;
  
    if(Left+width>Right) NextLine();
    sButtonControl *con = new sButtonControl(L"",DoneMsg,0);
    InitControl(con);
    con->Label = choices;
    con->LabelLength = len;
    con->InitRadio(val,id);
    if(Static) con->Style |= sBCS_STATIC;
    Grid->AddGrid(con,Left,Line,width);
    Left += width;
    EmptyLine = 0;

    choices += len;
  }
}


sButtonControl *sGridFrameHelper::Toggle(sInt *val,const sChar *label)
{
  if(Left+ControlWidth>Right) NextLine();
  sButtonControl *con = new sButtonControl(L"",DoneMsg,0);
  InitControl(con);
  con->Label = label;
  con->LabelLength = -1;
  con->InitToggle(val);
  if(Static) con->Style |= sBCS_STATIC;
  Grid->AddGrid(con,Left,Line,ControlWidth);
  Left += ControlWidth;
  EmptyLine = 0;
  return con;
}

sButtonControl *sGridFrameHelper::Button(const sChar *label,const sMessage &msg)
{
  if(Left+ControlWidth>Right) NextLine();
  sButtonControl *con = new sButtonControl(label,msg,0);
  if(Static) con->Style |= sBCS_STATIC;
  Grid->AddGrid(con,Left,Line,ControlWidth);
  Left += ControlWidth;
  EmptyLine = 0;
  return con;
}

class sChoiceControl *sGridFrameHelper::Choice(sInt *val,const sChar *choices)
{
  if(Left+ControlWidth>Right) NextLine();
  sChoiceControl *con = new sChoiceControl(choices,val);
  InitControl(con);
  if(Static) con->Style |= sBCS_STATIC;
  Grid->AddGrid(con,Left,Line,ControlWidth);
  Left += ControlWidth;
  EmptyLine = 0;
  return con;
}

sControl *sGridFrameHelper::Flags(sInt *val,const sChar *choices)
{
  sInt mask=0;
  sInt count=0;
  sChoiceControl *con;
  while(*choices)
  {
    if(Left+ControlWidth>Right) NextLine();
    con = new sChoiceControl();
    InitControl(con);
    con->InitChoices(choices,val);
    sVERIFY((mask & con->ValueMask)==0);
    con->ValueMask |= mask;
    if(Static) con->Style |= sBCS_STATIC;

    Grid->AddGrid(con,Left,Line,ControlWidth);
    Left += ControlWidth;
    EmptyLine = 0;
    if(*choices==':')
      choices++;
    count++;
  }
  if(count==1) 
    return con;
  else
    return 0;
}

void sGridFrameHelper::Flags(sInt *val,const sChar *choices,const sMessage &msg)
{
  sInt mask=0;
  while(*choices)
  {
    if(Left+ControlWidth>Right) NextLine();
    sChoiceControl *con = new sChoiceControl();
    InitControl(con);
    con->InitChoices(choices,val);
    sVERIFY((mask & con->ValueMask)==0);
    con->ValueMask |= mask;
    con->DoneMsg = msg;
    con->ChangeMsg = msg;
    if(Static) con->Style |= sBCS_STATIC;

    Grid->AddGrid(con,Left,Line,ControlWidth);
    Left += ControlWidth;
    EmptyLine = 0;
    if(*choices==':')
      choices++;
  }
}

sStringControl *sGridFrameHelper::String(const sStringDesc &string,sInt width)
{
  if(Left+WideWidth>Right) NextLine();
  sStringControl *con = new sStringControl(string);
  InitControl(con);
  if(Static) con->Style |= sSCS_STATIC;
  if(width==0) width = WideWidth;
  Grid->AddGrid(con,Left,Line,width);
  Left += width;
  EmptyLine = 0;
  return con;
}

sStringControl *sGridFrameHelper::String(sPoolString *pool,sInt width)
{
  if(Left+WideWidth>Right) NextLine();
  sStringControl *con = new sStringControl(pool);
  InitControl(con);
  if(Static) con->Style |= sSCS_STATIC;
  if(width==0) width = WideWidth;
  Grid->AddGrid(con,Left,Line,width);
  Left += width;
  EmptyLine = 0;
  return con;
}

sStringControl *sGridFrameHelper::String(sTextBuffer *tb,sInt width)
{
  if(Left+WideWidth>Right) NextLine();
  sStringControl *con = new sStringControl(tb);
  InitControl(con);
  if(Static) con->Style |= sSCS_STATIC;
  if(width==0) width = WideWidth;
  Grid->AddGrid(con,Left,Line,width);
  Left += width;
  EmptyLine = 0;
  return con;
}

sTextWindow *sGridFrameHelper::Text(sTextBuffer *tb,sInt lines,sInt width)
{
  sTextWindow *tw = new sTextWindow;
  tw->AddScrolling(0,1);
  tw->AddBorder(new sSpaceBorder(sGC_DOC));
  tw->SetText(tb);
  tw->ChangeMsg = ChangeMsg;
  tw->EnterMsg = DoneMsg;
  tw->BackColor = sGC_DOC;
  if(Static) tw->EditFlags |= sTEF_STATIC;

  if(width==0) width = WideWidth;
  Grid->AddGrid(tw,Left,Line,width,lines);
  Left += width;
  Line += lines-1;
  EmptyLine = 0;
  return tw;
}

sByteControl *sGridFrameHelper::Byte(sU8 *val,sInt min,sInt max,sF32 step,sU8 *colptr)
{
  if(Left+ControlWidth>Right) NextLine();
  sByteControl *con = new sByteControl(val,min,max,step,colptr);
  InitControl(con);
  if(Static) con->Style |= sSCS_STATIC;
  Tie(con);
  Grid->AddGrid(con,Left,Line,ControlWidth);
  Left += ControlWidth;
  EmptyLine = 0;
  return con;
}

sWordControl *sGridFrameHelper::Word(sU16 *val,sInt min,sInt max,sF32 step,sU16 *colptr)
{
  if(Left+ControlWidth>Right) NextLine();
  sWordControl *con = new sWordControl(val,min,max,step,colptr);
  InitControl(con);
  if(Static) con->Style |= sSCS_STATIC;
  Tie(con);
  Grid->AddGrid(con,Left,Line,ControlWidth);
  Left += ControlWidth;
  EmptyLine = 0;
  return con;
}

sIntControl *sGridFrameHelper::Int(sInt *val,sInt min,sInt max,sF32 step,sInt *colptr,const sChar *format)
{
  if(Left+ControlWidth>Right) NextLine();
  sIntControl *con = new sIntControl(val,min,max,step,colptr);
  if(format)
  {
    con->Format = format;
    con->MakeBuffer(1);
  }
  InitControl(con);
  if(Static) con->Style |= sSCS_STATIC;
  Tie(con);
  Grid->AddGrid(con,Left,Line,ControlWidth);
  Left += ControlWidth;
  EmptyLine = 0;
  return con;
}

sFloatControl *sGridFrameHelper::Float(sF32 *val,sF32 min,sF32 max,sF32 step,sF32 *colptr)
{
  if(Left+ControlWidth>Right) NextLine();
  sFloatControl *con = new sFloatControl(val,min,max,step,colptr);
  InitControl(con);
  if(Static) con->Style |= sSCS_STATIC;
  Tie(con);
  Grid->AddGrid(con,Left,Line,ControlWidth);
  Left += ControlWidth;
  EmptyLine = 0;
  return con;
}
  
void sGridFrameHelper::Color(sU32 *ptr,const sChar *config)
{
  sU8 *byte = (sU8 *)ptr;
  sByteControl *conr=0,*cong=0,*conb=0,*cona=0;
  if(Left+ControlWidth*sGetStringLen(config)>Right) NextLine();
  while(*config)
  {
    switch(*config++)
    {
    case 'r':
      conr = Byte(byte+2,0,255,0.25f,byte);
      conr->Style = sSCS_BACKCOLOR;
			conr->RightStep = 2;
      if(Static) conr->Style |= sSCS_STATIC;
      InitControl(conr);
      break;
    case 'g':
      cong = Byte(byte+1,0,255,0.25f,byte);
      cong->Style = sSCS_BACKCOLOR;
			cong->RightStep = 2;
      if(Static) cong->Style |= sSCS_STATIC;
      InitControl(cong);
      break;
    case 'b':
      conb = Byte(byte+0,0,255,0.25f,byte);
      conb->Style = sSCS_BACKCOLOR;
			conb->RightStep = 2;
      if(Static) conb->Style |= sSCS_STATIC;
      InitControl(conb);
      break;
    case 'a':
      cona = Byte(byte+3,0,255,0.25f);
			cona->RightStep = 2;
      if(Static) cona->Style |= sSCS_STATIC;
      InitControl(cona);
      break;
    }
  }
  if(conr && cong && conb)
  {
    conr->DragTogether = conb;
    cong->DragTogether = conr;
    conb->DragTogether = cong;
  }
}

void sGridFrameHelper::ColorF(sF32 *ptr,const sChar *config)
{
  sFloatControl *conr=0,*cong=0,*conb=0,*cona=0;
  if(Left+ControlWidth*sGetStringLen(config)>Right) NextLine();
  while(*config)
  {
    switch(*config++)
    {
    case 'r':
      conr = Float(ptr+0,0,1,0.001f,ptr);
      conr->Style = sSCS_BACKCOLOR;
      InitControl(conr);
      if(Static) conr->Style |= sSCS_STATIC;
      break;
    case 'g':
      cong = Float(ptr+1,0,1,0.001f,ptr);
      cong->Style = sSCS_BACKCOLOR;
      InitControl(cong);
      if(Static) cong->Style |= sSCS_STATIC;
      break;
    case 'b':
      conb = Float(ptr+2,0,1,0.001f,ptr);
      conb->Style = sSCS_BACKCOLOR;
      InitControl(conb);
      if(Static) conb->Style |= sSCS_STATIC;
      break;
    case 'a':
      cona = Float(ptr+3,0,1,0.001f);
      InitControl(cona);
      if(Static) cona->Style |= sSCS_STATIC;
      break;
    }
  }
  if(conr && cong && conb)
  {
    conr->DragTogether = conb;
    cong->DragTogether = conr;
    conb->DragTogether = cong;
  }
}

/****************************************************************************/

class sColorPickerInfo : public sObject
{
public:
  sCLASSNAME_NONEW(sColorPickerInfo);
  sColorPickerInfo(sU32 *u,sF32 *f,sObject *ref,sBool alpha,sMessage &msg);
  void Tag();

  sU32 *UPtr;
  sF32 *FPtr;
  sObject *TagRef;
  sInt Alpha;
  sMessage ChangeMsg;

  void CmdOpen();
};

sColorPickerInfo::sColorPickerInfo(sU32 *u,sF32 *f,sObject *ref,sBool alpha,sMessage &msg)
{
  UPtr = u;
  FPtr = f;
  TagRef = ref;
  Alpha = alpha;
  ChangeMsg = msg;
}

void sColorPickerInfo::Tag()
{
  sObject::Tag();
  TagRef->Need();
  ChangeMsg.Target->Need();
}

void sColorPickerInfo::CmdOpen()
{
  sColorPickerWindow *cp = new sColorPickerWindow;
  if(UPtr)
    cp->Init(UPtr,TagRef,Alpha);
  if(FPtr)
    cp->Init(FPtr,TagRef,Alpha);
  cp->AddBorder(new sThickBorder);
  cp->Flags |= sWF_AUTOKILL;
  cp->ChangeMsg = ChangeMsg;
  sGui->AddPulldownWindow(cp);
}


void sGridFrameHelper::ColorPick(sU32 *ptr,const sChar *config,sObject *tagref)
{
  sBool alpha = sFindLastChar(config,'a')!=-1;
  sColorPickerInfo *info = new sColorPickerInfo(ptr,0,tagref,alpha,ChangeMsg);
  Color(ptr,config);
  Box(L"^",sMessage(info,&sColorPickerInfo::CmdOpen));
}

void sGridFrameHelper::ColorPickF(sF32 *ptr,const sChar *config,sObject *tagref)
{
  sBool alpha = sFindLastChar(config,'a')!=-1;
  sColorPickerInfo *info = new sColorPickerInfo(0,ptr,tagref,alpha,ChangeMsg);
  ColorF(ptr,config);
  Box(L"^",sMessage(info,&sColorPickerInfo::CmdOpen));
}

sColorGradientControl *sGridFrameHelper::Gradient(class sColorGradient *g,sBool alpha)
{
  if(Left+WideWidth>Right) NextLine();
  sColorGradientControl *con = new sColorGradientControl(g,alpha);
  InitControl(con);
  Grid->AddGrid(con,Left,Line,WideWidth);
  Left += WideWidth;
  EmptyLine = 0;
  return con;
}


void sGridFrameHelper::Bitmask(sU8 *x,sInt width)
{
  if(Left>Right-BoxWidth) NextLine();
  sBitmaskControl *con = new sBitmaskControl();
  con->Init(x);
  con->DoneMsg = DoneMsg;
  con->ChangeMsg = ChangeMsg;
  Grid->AddGrid(con,Left,Line,width);
  Left += width;
  EmptyLine = 0;
}
/****************************************************************************/

void sGridFrameHelper::Scale(sVector31 &v)
{
  Label(L"Scale");
  BeginTied();
  Float(&v.x,-ScaleRange,ScaleRange,ScaleStep);
  Float(&v.y,-ScaleRange,ScaleRange,ScaleStep);
  Float(&v.z,-ScaleRange,ScaleRange,ScaleStep);
  EndTied();
}

void sGridFrameHelper::Rotate(sVector30 &v)
{
  Label(L"Rotate");
  BeginTied();
  Float(&v.x,-RotateRange,RotateRange,RotateStep);
  Float(&v.y,-RotateRange,RotateRange,RotateStep);
  Float(&v.z,-RotateRange,RotateRange,RotateStep);
  EndTied();
}

void sGridFrameHelper::Translate(sVector31 &v)
{
  Label(L"Translate");
  BeginTied();
  Float(&v.x,-TranslateRange,TranslateRange,TranslateStep);
  Float(&v.y,-TranslateRange,TranslateRange,TranslateStep);
  Float(&v.z,-TranslateRange,TranslateRange,TranslateStep);
  EndTied();
}

void sGridFrameHelper::SRT(sSRT &srt)
{
  Scale(srt.Scale);
  Rotate(srt.Rotate);
  Translate(srt.Translate);
}


void sGridFrameHelper::Control(sControl *con,sInt width)
{
  if(width==-1) width = ControlWidth;
  if(Left+width>Right) NextLine();
  InitControl(con);
  Grid->AddGrid(con,Left,Line,width);
  Left += width;
  EmptyLine = 0;
}

void sGridFrameHelper::Custom(sWindow *con,sInt width,sInt height)
{
  if(width==-1) width = ControlWidth;
  if(Left+width>Right) NextLine();
  Grid->AddGrid(con,Left,Line,width,height);
  Left += width;
  EmptyLine = 0;
  for(sInt i=1;i<height;i++)
  {
    NextLine();
    EmptyLine = 0;
  }
}

/****************************************************************************/
/****************************************************************************/

void sGridFrameTemplate::Init()
{
  sClear(*this);
}

sBool sGridFrameTemplate::Condition(void *obj_)
{
  sU8 *obj = (sU8 *) obj_;

  sInt val = (*(sInt *)(obj+ConditionOffset)) & ConditionMask;
  sBool cond = (val == ConditionValue);
  if(Flags & sGFF_CONDNEGATE)
    cond = !cond;
  return cond;
}

void sGridFrameTemplate::Add(sGridFrameHelper &gh,void *obj_,const sMessage &changemsg,const sMessage &relayoutmsg)
{
  sU8 *obj = (sU8 *) obj_;

  if((Flags & sGFF_CONDHIDE) && !Condition(obj_))
    return;

  if(Label && !(Flags & sGFF_NOLABEL))
    gh.Label(Label);

  gh.DoneMsg = gh.ChangeMsg =  changemsg;
  if(Flags&sGFF_RELAYOUT)
    gh.DoneMsg = relayoutmsg;

  switch(Type)
  {
  case sGFT_LABEL:
    break;
  case sGFT_GROUP:
    gh.Group(Label);
    break;
  case sGFT_PUSHBUTTON:
    gh.PushButton(Label,Message);
    break;
  case sGFT_BOX:
    gh.Box(Label,Message);
    break;

  case sGFT_RADIO:
    gh.Radio((sInt *)(obj+Offset),Choices,(Flags&sGFF_WIDERADIO)?2:-1);
    break;
  case sGFT_CHOICE:
    gh.Choice((sInt *)(obj+Offset),Choices);
    break;
  case sGFT_FLAGS:
    gh.Flags((sInt *)(obj+Offset),Choices);
    break;
  case sGFT_STRING:
    gh.String(sStringDesc((sChar *)(obj+Offset),Count));
    break;
  case sGFT_INT:
    for(sInt i=0;i<Count;i++)
      gh.Int((sInt *)(obj+Offset+i*sizeof(sInt)),sInt(Min),sInt(Max),Step);
    break;
  case sGFT_FLOAT:
    for(sInt i=0;i<Count;i++)
      gh.Float((sF32 *)(obj+Offset+i*sizeof(sF32)),Min,Max,Step);
    break;
  case sGFT_BYTE:
    for(sInt i=0;i<Count;i++)
      gh.Byte((sU8 *)(obj+Offset+i*sizeof(sU8)),sInt(Min),sInt(Max),Step);
    break;
  case sGFT_COLOR:
    gh.Color((sU32 *)(obj+Offset),Choices);
    break;
  case sGFT_COLORF:
    gh.ColorF((sF32 *)(obj+Offset),Choices);
    break;
  }
}

sGridFrameTemplate *sGridFrameTemplate::HideCond(sInt offset,sInt mask,sInt value)
{
  ConditionOffset = offset;
  ConditionMask = mask;
  ConditionValue = value;
  Flags |= sGFF_CONDHIDE;
  return this;
}

sGridFrameTemplate *sGridFrameTemplate::HideCondNot(sInt offset,sInt mask,sInt value)
{
  ConditionOffset = offset;
  ConditionMask = mask;
  ConditionValue = value;
  Flags |= sGFF_CONDHIDE;
  Flags |= sGFF_CONDNEGATE;
  return this;
}

/****************************************************************************/

sGridFrameTemplate * sGridFrameTemplate::InitLabel(const sChar *label)
{
  Init();
  Type = sGFT_LABEL;
  Label = label;
  return this;
}

sGridFrameTemplate * sGridFrameTemplate::InitGroup(const sChar *label)
{
  Init();
  Type = sGFT_GROUP;
  Flags = sGFF_NOLABEL;
  Label = label;
  return this;
}

sGridFrameTemplate * sGridFrameTemplate::InitPushButton(const sChar *label,const sMessage &done)
{
  Init();
  Type = sGFT_PUSHBUTTON;
  Flags = sGFF_NOLABEL;
  Label = label;
  Message = done;
  return this;
}

sGridFrameTemplate * sGridFrameTemplate::InitBox(const sChar *label,const sMessage &done)
{
  Init();
  Type = sGFT_BOX;
  Flags = sGFF_NOLABEL;
  Label = label;
  Message = done;
  return this;
}

sGridFrameTemplate * sGridFrameTemplate::InitRadio(const sChar *label,sInt offset,const sChar *choices)
{
  Init();
  Type = sGFT_RADIO;
  Label = label;
  Offset = offset;
  Choices = choices;
  return this;
}

sGridFrameTemplate * sGridFrameTemplate::InitChoice(const sChar *label,sInt offset,const sChar *choices)
{
  Init();
  Type = sGFT_CHOICE;
  Label = label;
  Offset = offset;
  Choices = choices;
  return this;
}

sGridFrameTemplate * sGridFrameTemplate::InitFlags(const sChar *label,sInt offset,const sChar *choices)
{
  Init();
  Type = sGFT_FLAGS;
  Label = label;
  Offset = offset;
  Choices = choices;
  return this;
}

sGridFrameTemplate * sGridFrameTemplate::InitString(const sChar *label,sInt offset,sInt count)
{
  Init();
  Type = sGFT_STRING;
  Label = label;
  Offset = offset;
  Count = count;
  return this;
}

sGridFrameTemplate * sGridFrameTemplate::InitFloat(const sChar *label,sInt offset,sInt count,sF32 min,sF32 max,sF32 step)
{
  Init();
  Type = sGFT_FLOAT;
  Label = label;
  Offset = offset;
  Count = count;
  Min = min;
  Max = max;
  Step = step;
  return this;
}

sGridFrameTemplate * sGridFrameTemplate::InitInt(const sChar *label,sInt offset,sInt count,sInt min,sInt max,sF32 step)
{
  Init();
  Type = sGFT_INT;
  Label = label;
  Offset = offset;
  Count = count;
  Min = min;
  Max = max;
  Step = step;
  return this;
}

sGridFrameTemplate * sGridFrameTemplate::InitByte(const sChar *label,sInt offset,sInt count,sInt min,sInt max,sF32 step)
{
  Init();
  Type = sGFT_BYTE;
  Label = label;
  Offset = offset;
  Count = count;
  Min = min;
  Max = max;
  Step = step;
  return this;
}

sGridFrameTemplate * sGridFrameTemplate::InitColor(const sChar *label,sInt offset,const sChar *config)
{
  Init();
  Type = sGFT_COLOR;
  Label = label;
  Offset = offset;
  Choices = config;
  return this;
}

sGridFrameTemplate * sGridFrameTemplate::InitColorF(const sChar *label,sInt offset,const sChar *config)
{
  Init();
  Type = sGFT_COLORF;
  Label = label;
  Offset = offset;
  Choices = config;
  return this;
}

/****************************************************************************/
/****************************************************************************/

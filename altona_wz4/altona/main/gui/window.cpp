/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "gui/window.hpp"
#include "base/windows.hpp"
#include "gui/manager.hpp"
#include "gui/borders.hpp"
#include "gui/controls.hpp"


/****************************************************************************/
/***                                                                      ***/
/***   Helper Structures                                                  ***/
/***                                                                      ***/
/****************************************************************************/

void sWindowMessage::Send(sWindow *w)
{
  if(Func)
    (w->*Func)(Code);
  else
    sGui->Send(w,Code);
}

void sWindowMessage::Post(sWindow *w)
{
  if(Func)
    (w->*Func)(Code);
  else
    sGui->Post(w,Code);
}

/****************************************************************************/
/***                                                                      ***/
/***   Main Window Class                                                  ***/
/***                                                                      ***/
/****************************************************************************/

sWindow::sWindow()
{
  ReqSizeX = 0;
  ReqSizeY = 0;
  ScrollX = 0;
  ScrollY = 0;
  Flags = 0;
  WireSets = ~0;

  Parent = 0;
  PopupParent = 0;
  MousePointer = sMP_ARROW;

  MMBScrollStartX = 0;
  MMBScrollStartY = 0;
  MMBScrollMode = 0;

  ToolTip = 0;
  ToolTipLength = -1;
}

sWindow::~sWindow()
{
}

void sWindow::OnBeforePaint()
{
}

void sWindow::OnPaint2D()
{
  if(Childs.GetCount()==0)
  {
    sGui->PropFont->SetColor(sGC_TEXT,sGC_BACK);
    sGui->PropFont->Print(sF2P_OPAQUE,Client,GetClassName());
  }
}

void sWindow::OnPaint3D()
{
}

void sWindow::OnLayout()
{
  sVERIFY(Childs.GetCount()<=1);

  sWindow *c;
  sFORALL(Childs,c)
    c->Outer = Client;
}

sBool sWindow::OnKey(sU32 key)
{
  return 0;
}

sBool sWindow::OnShortcut(sU32 key)
{
  return 0;
}

sBool sWindow::OnCommand(sInt cmd)
{
  return 0;
}

void sWindow::OnDrag(const sWindowDrag &dd)
{
}

void sWindow::OnCalcSize()
{
  if(Childs.GetCount()==1)
  {
    ReqSizeX = Childs[0]->ReqSizeX;
    ReqSizeY = Childs[0]->ReqSizeY;
  }
  else
  {
    ReqSizeX = 0;
    ReqSizeY = 0;
  }
}

void sWindow::OnNotify(const void*, sInt)
{
  Update();
}

void sWindow::Tag()
{
  sNeed(Childs);
  sNeed(Borders);
  Parent->Need();
  PopupParent->Need();
}

/****************************************************************************/

void sWindow::AddChild(sWindow *w)
{
  w->Parent = this;
  Childs.AddTail(w);
  sGui->Layout();
}

void sWindow::AddBorder(sWindow *w)
{
  w->Parent = this;
  Borders.AddTail(w);
  sGui->Layout();
}

void sWindow::AddBorderHead(sWindow *w)
{
  w->Parent = this;
  Borders.AddHead(w);
  sGui->Layout();
}

sButtonControl *sWindow::AddButton(const sChar *label,const sMessage &cmd,sInt style)
{
  sButtonControl *bc;

  bc = new sButtonControl(label,cmd,style);
  AddChild(bc);

  return bc;
}

sButtonControl *sWindow::AddRadioButton(const sChar *label,sInt *ptr,sInt val,const sMessage &cmd,sInt style)
{
  sButtonControl *bc;

  bc = new sButtonControl(label,cmd,style);
  bc->InitRadio(ptr,val);
  AddChild(bc);

  return bc;
}

sButtonControl *sWindow::AddToggleButton(const sChar *label,sInt *ptr,const sMessage &cmd,sInt style)
{
  sButtonControl *bc;

  bc = new sButtonControl(label,cmd,style);
  bc->InitToggle(ptr);
  AddChild(bc);

  return bc;
}

void sWindow::AddNotify(const void *p,sDInt n)
{
  sWindowNotify wn;
  wn.Start = p;
  wn.End = (((sU8 *)p)+n);
  *NotifyList.AddMany(1) = wn;
  UpdateNotify();
}

void sWindow::ClearNotify()
{
  NotifyList.Clear();
}

void sWindow::UpdateNotify()
{
  if(Flags & sWF_NOTIFYVALID)
  {
    Flags &= ~sWF_NOTIFYVALID;
    if(Parent)
      Parent->UpdateNotify();
  }
}

void sWindow::Update()
{
  if(sGui)
    sGui->Update(Inner);
}

void sWindow::Layout()
{
  sGui->Layout(Outer);
}

void sWindow::Close()
{
  Layout();
  if(Parent)
    Parent->Childs.Rem(this);
  if(PopupParent && (Flags & sWF_AUTOKILL)/* && (Flags & sWF_CHILDFOCUS)*/)
    sGui->SetFocus(PopupParent);
  Parent = 0;
  PopupParent = 0;
}

void sWindow::SetToolTipIfNarrow(const sChar *str,sInt len,sInt space)
{
  ToolTip = 0;
  ToolTipLength = -1;
  if(sGui->PropFont->GetWidth(str,len)>Client.SizeX()-space)
  {
    ToolTip = str;
    ToolTipLength = len;
  }
}


void sWindow::Post(sInt cmd) 
{ 
  sGui->Post(this,cmd); 
}

/****************************************************************************/

class sScrollBorder *sWindow::AddScrolling(sBool x,sBool y)
{
  Flags |= sWF_CLIENTCLIPPING;
  if(x)
    Flags |= sWF_SCROLLX;
  if(y)
    Flags |= sWF_SCROLLY;

  sScrollBorder *b = new sScrollBorder;
  AddBorder(b);
  return b;
}

void sWindow::ScrollTo(const sRect &r,sBool save)
{
  sInt c,s;
  sRect vis;
  sInt oldx = ScrollX;
  sInt oldy = ScrollY;
  vis = r;
  vis.x0 -= Client.x0;
  vis.y0 -= Client.y0;
  vis.x1 -= Client.x0;
  vis.y1 -= Client.y0;
  if(save)
  {
    if(Client.SizeX()>75)
    {
      vis.x0-=25;
      vis.x1+=25;
    }
    if(Client.SizeY()>75)
    {
      vis.y0-=25;
      vis.y1+=25;
    }
  }
  if(vis.x0<0)
    vis.x0 = 0;
  if(vis.x1>ReqSizeX)
    vis.x1 = ReqSizeX;
  if(vis.y0<0)
    vis.y0 = 0;
  if(vis.y1>ReqSizeY)
    vis.y1 = ReqSizeY;
  if(vis.x1-vis.x0 > Inner.SizeX())
  {
    s = Client.SizeX();
    c = (vis.x1+vis.x0)/2;
    vis.x0 = c-s/2;
    vis.x1 = vis.x0+s;
  }
  if(vis.y1-vis.y0 > Inner.SizeY())
  {
    s = Client.SizeY();
    c = (vis.y1+vis.y0)/2;
    vis.y0 = c-s/2;
    vis.y1 = vis.y0+s;
  }
  if(vis.x0 < ScrollX)
    ScrollX = vis.x0;
  if(vis.x1 > ScrollX+Inner.SizeX())
    ScrollX = vis.x1-Inner.SizeX();
  if(vis.y0 < ScrollY)
    ScrollY = vis.y0;
  if(vis.y1 > ScrollY+Inner.SizeY())
    ScrollY = vis.y1-Inner.SizeY();

  if(ScrollX!=oldx || ScrollY!=oldy)
    Layout();
}

void sWindow::ScrollTo(sInt newx,sInt newy)
{
  sInt xs = Client.SizeX()-Inner.SizeX();
  sInt ys = Client.SizeY()-Inner.SizeY();
  if(xs<0) xs = 0;
  if(ys<0) ys = 0;

  sInt oldx = ScrollX;
  sInt oldy = ScrollY;
  ScrollX = sClamp(newx,0,xs);
  ScrollY = sClamp(newy,0,ys);
  
  if(ScrollX!=oldx || ScrollY!=oldy)
    Layout();
}


sBool sWindow::MMBScroll(const sWindowDrag &dd)
{
  switch(dd.Mode)
  {
  case sDD_START:
    if(dd.Buttons&4)
    {
      MMBScrollMode = 1;
      MMBScrollStartX = ScrollX;
      MMBScrollStartY = ScrollY;
      return 1;
    }
    break;

  case sDD_DRAG:
    if(MMBScrollMode)
    {
      sInt speed = (sGetKeyQualifier()&sKEYQ_SHIFT)?8:1;
      ScrollX = sClamp(MMBScrollStartX-(dd.MouseX-dd.StartX)*speed , 0 , sMax(0,ReqSizeX-Inner.SizeX()));
      ScrollY = sClamp(MMBScrollStartY-(dd.MouseY-dd.StartY)*speed , 0 , sMax(0,ReqSizeY-Inner.SizeY()));
      sGui->Update(Outer);
      Layout();
      return 1;
    }
    break;

  case sDD_STOP:
    if(MMBScrollMode)
    {
      MMBScrollMode = 0;
      return 1;
    }
    break;

  default:
    return MMBScrollMode;
  }

  return 0;
}

/****************************************************************************/

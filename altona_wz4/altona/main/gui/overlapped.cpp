/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "gui/overlapped.hpp"
#include "gui/manager.hpp"
#include "gui/controls.hpp"

/****************************************************************************/

sOverlappedFrame::sOverlappedFrame()
{
  Flags |= sWF_OVERLAPPEDCHILDS;
}

void sOverlappedFrame::OnLayout()
{
  // no layout, just leave the old Outer rect
  // but this is the chance to care about sorting order!

  sWindow *w;

  BackClear = 1;
  sFORALL(Childs,w)
  {
    w->Temp = 1;
    if(w->Flags & sWF_ZORDER_BACK)
    {
      w->Temp = 0;
      w->Outer = Client;
      BackClear = 0;
    }
    else if(w->Flags & sWF_ZORDER_TOP)
    {
      w->Temp = 3;
    }
    else if(w->Flags & sWF_CHILDFOCUS)
    {
      w->Temp = 2;
    }
  }

  sSortUp(Childs,&sWindow::Temp);
}

void sOverlappedFrame::OnPaint2D()
{
  if(BackClear)
    sRect2D(Client,sGC_BUTTON);
}

/****************************************************************************/

static void MoveWindow(sWindow *parent,const sRect &old,sInt dx,sInt dy,sInt mask)
{
  sRect *outer = &parent->Outer;

  if(parent->Parent)
  {
    if(old.x0+dx < parent->Parent->Client.x0)
      dx = parent->Parent->Client.x0 - old.x0;
    if(old.x1+dx > parent->Parent->Client.x1)
      dx = parent->Parent->Client.x1 - old.x1;
    if(old.y0+dy < parent->Parent->Client.y0)
      dy = parent->Parent->Client.y0 - old.y0;
    if(old.y1+dy > parent->Parent->Client.y1)
      dy = parent->Parent->Client.y1 - old.y1;
  }

  if((mask&5)==5)
  {
    outer->x0 = old.x0 + dx;
    outer->x1 = old.x1 + dx;
  }
  else if(mask & 1) 
  {
    outer->x0 = old.x0 + dx;
    if(outer->x0 > outer->x1-20)
      outer->x0 = outer->x1-20;
  }
  else if(mask & 4) 
  {
    outer->x1 = old.x1 + dx;
    if(outer->x1 < outer->x0+20)
      outer->x1 = outer->x0+20;
  }

  if((mask&10)==10)
  {
    outer->y0 = old.y0 + dy;
    outer->y1 = old.y1 + dy;
  }
  else if(mask & 2) 
  {
    outer->y0 = old.y0 + dy;
    if(outer->y0 > outer->y1-20)
      outer->y0 = outer->y1-20;
  }
  else if(mask & 8) 
  {
    outer->y1 = old.y1 + dy;
    if(outer->y1 < outer->y0+20)
      outer->y1 = outer->y0+20;
  }


  if(parent->Parent)
    parent->Parent->Layout();
  else
    sGui->Layout();
}

/****************************************************************************/

void sSizeBorder::OnCalcSize()
{
  ReqSizeX = 8;
  ReqSizeY = 8;
}

void sSizeBorder::OnLayout()
{
  Parent->Inner.Extend(-4);
}

void sSizeBorder::OnPaint2D()
{
  sRect r(Client);
  sGui->RectHL(r,sGC_HIGH,sGC_LOW); r.Extend(-1);
  sGui->RectHL(r,sGC_BACK,sGC_BACK); r.Extend(-1);
  sGui->RectHL(r,sGC_BACK,sGC_BACK); r.Extend(-1);
  sGui->RectHL(r,sGC_LOW,sGC_HIGH); r.Extend(-1);
}

void sSizeBorder::OnDrag(const sWindowDrag &dd)
{
  sVERIFY(Parent);

  const sInt b=15;
  sInt mask = 0;
  if(dd.MouseX<Client.x0+b) mask |= 1;
  if(dd.MouseY<Client.y0+b) mask |= 2;
  if(dd.MouseX>Client.x1-b) mask |= 4;
  if(dd.MouseY>Client.y1-b) mask |= 8;

  switch(dd.Mode)
  {
  case sDD_START:
    DragMask = mask;
    DragStart = Parent->Outer;
    break;
  case sDD_DRAG:
    MoveWindow(Parent,DragStart,dd.MouseX-dd.StartX,dd.MouseY-dd.StartY,DragMask);
    break;
  case sDD_STOP:
    break;
  case sDD_HOVER:
    MousePointer = sMP_OFF;
    if(mask&(1|4)) MousePointer = sMP_SIZEWE;
    if(mask&(2|8)) MousePointer = sMP_SIZENS;
    if(mask== 3||mask==12) MousePointer = sMP_SIZENWSE;
    if(mask== 6||mask== 9) MousePointer = sMP_SIZENESW;
    break;
  }
}

/****************************************************************************/

sTitleBorder::sTitleBorder(sMessage closemsg)
{
  Title=L"";
  CloseButton=new sButtonControl(L"X",closemsg);
  AddChild(CloseButton);
}

sTitleBorder::sTitleBorder(const sChar *title, sMessage closemsg)
{
  Title=title;
  CloseButton=new sButtonControl(L"X",closemsg);
  AddChild(CloseButton);
}

void sTitleBorder::OnCalcSize()
{
  ReqSizeY = 15;
}

void sTitleBorder::OnLayout()
{
  sInt y0 = Client.y0;
  sInt y1 = Client.y0+15;
  Parent->Inner.y0 = Client.y1 = y1;
  y1--;

  sInt x=Client.x1;

  CloseButton->Outer.y0 = y0;
  CloseButton->Outer.y1 = y1;
  if (CloseButton->ChangeMsg.IsValid())
  {
    CloseButton->Outer.x1 = x;
    x -= CloseButton->ReqSizeX;
    CloseButton->Outer.x0 = x;
  }
  else
  {
    CloseButton->Outer.x0 = CloseButton->Outer.x1 = x;
  }

}

void sTitleBorder::OnPaint2D()
{
  sRect r(Client);
  r.y1 = r.y0+14;
  if (Childs.GetCount())
    r.x1=Childs[Childs.GetCount()-1]->Outer.x0;

  sGui->PropFont->SetColor(sGC_TEXT,sGC_BUTTON);
  sGui->PropFont->Print(sF2P_OPAQUE|sF2P_LEFT|sF2P_SPACE,r,Title);
  r.y0 = r.y1;
  r.y1 = r.y0+1;
  r.x1=Client.x1;
  sRect2D(r,sGC_DRAW);
}


void sTitleBorder::OnDrag(const sWindowDrag &dd)
{
  sVERIFY(Parent);
  switch(dd.Mode)
  {
  case sDD_START:
    DragStart = Parent->Outer;
    break;
  case sDD_DRAG:
    MoveWindow(Parent,DragStart,dd.MouseX-dd.StartX,dd.MouseY-dd.StartY,15);
    break;
  case sDD_STOP:
    break;
  }
}

/****************************************************************************/

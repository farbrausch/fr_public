/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"

Document *Doc;
MainWindow *App;

/****************************************************************************/

RectElement::RectElement()
{
  Add = 1;
  Rect.Init(10,10,22,22);
}

const sChar *RectElement::GetName()
{
  return Add ? L"Add" : L"Sub";
}

/****************************************************************************/

Document::Document()
{
}

Document::~Document()
{
}

void Document::Tag()
{
  sNeed(Rects);
}

/****************************************************************************/

RectWindow::RectWindow()
{
  DragElem = 0;
  DragRect.Init();
}

void RectWindow::Tag()
{
  sWindow::Tag();
  DragElem->Need();
}

void RectWindow::OnPaint2D()
{
  RectElement *r;
  sRect rect,*rp;
  sInt x = Client.x0;
  sInt y = Client.y0;

  sGui->BeginBackBuffer(Client);

  sClipPush();
  sFORALLREVERSE(Doc->Rects,r)
  {
    sSetColor2D(0,r->Add ? 0xff707070 : 0xffb0b0b0);
    sRect rr(r->Rect.x0+x,r->Rect.y0+y,r->Rect.x1+x,r->Rect.y1+y);
    sRect2D(rr,0);
    sClipExclude(rr);
  }
  sRect2D(Client,sGC_BACK);
  sClipPop();

  Region.Clear();
  sFORALL(Doc->Rects,r)
  {
    if(r->Add)
      Region.Add(r->Rect);
    else
      Region.Sub(r->Rect);
  }

  sFORALL(Region.Rects,rp)
  {
    rect = *rp;
    rect.x0 += Client.x0;
    rect.y0 += Client.y0;
    rect.x1 += Client.x0;
    rect.y1 += Client.y0;
    sGui->RectHL(rect,sGC_TEXT,sGC_TEXT);
  }

  sGui->EndBackBuffer();
}

void RectWindow::OnCmdDelete()
{
  RectElement *r = App->ListWin->GetSelected();
  Doc->Rects.RemOrder(r);
  App->UpdateList();
  Update();
}

void RectWindow::OnCmdToggle()
{
  RectElement *r = App->ListWin->GetSelected();
  r->Add = !r->Add;
  App->UpdateList();
  Update();
}

void RectWindow::InitWire(const sChar *name)
{
  sWireClientWindow::InitWire(name);
  sWire->AddDrag(name,L"DrawAdd",sMessage(this,&RectWindow::OnDragDraw,1));
  sWire->AddDrag(name,L"DrawSub",sMessage(this,&RectWindow::OnDragDraw,0));
  sWire->AddDrag(name,L"Move",sMessage(this,&RectWindow::OnDragMove));
  sWire->AddDrag(name,L"Select",sMessage(this,&RectWindow::OnDragSelect));
  sWire->AddKey(name,L"Delete",sMessage(this,&RectWindow::OnCmdDelete));
  sWire->AddKey(name,L"Toggle",sMessage(this,&RectWindow::OnCmdToggle));
}

void RectWindow::OnDragDraw(const sWindowDrag &dd,sDInt mode)
{
  sInt mx = dd.MouseX-Client.x0;
  sInt my = dd.MouseY-Client.y0;
  const sInt m = 12;

  switch(dd.Mode)
  {
  case sDD_START:

    DragElem = new RectElement;
    Doc->Rects.AddTail(DragElem);
    DragElem->Rect.Init(mx,my,mx,my);
    DragElem->Add = mode;

    DragRect.x0 = DragElem->Rect.x0 - dd.MouseX;
    DragRect.y0 = DragElem->Rect.y0 - dd.MouseY;
    DragRect.x1 = DragElem->Rect.x1 - dd.MouseX;
    DragRect.y1 = DragElem->Rect.y1 - dd.MouseY;
    DragMask = 12;
    App->UpdateList();
    App->ListWin->SetSelected(DragElem);
    App->UpdateList();
    // break;

  case sDD_DRAG:
    if(DragElem && DragMask)
    {
      sRect r = DragElem->Rect;
      if(DragMask & 1) r.x0 = sClamp(DragRect.x0 + dd.MouseX,0,Client.SizeX());
      if(DragMask & 2) r.y0 = sClamp(DragRect.y0 + dd.MouseY,0,Client.SizeY());
      if(DragMask & 4) r.x1 = sClamp(DragRect.x1 + dd.MouseX,0,Client.SizeX());
      if(DragMask & 8) r.y1 = sClamp(DragRect.y1 + dd.MouseY,0,Client.SizeY());

      if((DragMask & 1) && r.x0>r.x1-m) r.x0 = r.x1-m;
      if((DragMask & 2) && r.y0>r.y1-m) r.y0 = r.y1-m;
      if((DragMask & 4) && r.x1<r.x0+m) r.x1 = r.x0+m;
      if((DragMask & 8) && r.y1<r.y0+m) r.y1 = r.y0+m;

      DragElem->Rect = r;
      sGui->Notify(DragElem->Rect);
      Update();
    }
    break;

  case sDD_STOP:
    DragMask = 0;
    Update();
    break;
  }
}

void RectWindow::OnDragSelect(const sWindowDrag &dd)
{
  RectElement *hit,*r;
  sInt mx = dd.MouseX-Client.x0;
  sInt my = dd.MouseY-Client.y0;

  switch(dd.Mode)
  {
  case sDD_START:
    hit = 0;
    sFORALL(Doc->Rects,r)
      if(r->Rect.Hit(mx,my))
        hit = r;
    if(hit)
      App->ListWin->SetSelected(hit);
    break;
  }
}

void RectWindow::OnDragMove(const sWindowDrag &dd)
{
  RectElement *hit,*r;
  sInt mx = dd.MouseX-Client.x0;
  sInt my = dd.MouseY-Client.y0;
  const sInt w = 4;
  const sInt m = 12;

  switch(dd.Mode)
  {
  case sDD_START:
    hit = 0;
    sFORALL(Doc->Rects,r)
      if(r->Rect.Hit(mx,my))
        hit = r;

    if(hit)
    {
      DragElem = hit;
      DragRect.x0 = DragElem->Rect.x0 - dd.MouseX;
      DragRect.y0 = DragElem->Rect.y0 - dd.MouseY;
      DragRect.x1 = DragElem->Rect.x1 - dd.MouseX;
      DragRect.y1 = DragElem->Rect.y1 - dd.MouseY;
      DragMask = 0;
      if(mx<DragElem->Rect.x0+w) DragMask |= 1;
      if(my<DragElem->Rect.y0+w) DragMask |= 2;
      if(mx>DragElem->Rect.x1-w) DragMask |= 4;
      if(my>DragElem->Rect.y1-w) DragMask |= 8;
      if(DragMask==0)
        DragMask = 15;
      App->ListWin->SetSelected(hit);
      App->UpdateList();
    }
    // break;

  case sDD_DRAG:
    if(DragElem && DragMask)
    {
      sRect r = DragElem->Rect;
      if(DragMask & 1) r.x0 = sClamp(DragRect.x0 + dd.MouseX,0,Client.SizeX());
      if(DragMask & 2) r.y0 = sClamp(DragRect.y0 + dd.MouseY,0,Client.SizeY());
      if(DragMask & 4) r.x1 = sClamp(DragRect.x1 + dd.MouseX,0,Client.SizeX());
      if(DragMask & 8) r.y1 = sClamp(DragRect.y1 + dd.MouseY,0,Client.SizeY());

      if((DragMask & 1) && r.x0>r.x1-m) r.x0 = r.x1-m;
      if((DragMask & 2) && r.y0>r.y1-m) r.y0 = r.y1-m;
      if((DragMask & 4) && r.x1<r.x0+m) r.x1 = r.x0+m;
      if((DragMask & 8) && r.y1<r.y0+m) r.y1 = r.y0+m;

      DragElem->Rect = r;
      sGui->Notify(DragElem->Rect);
      Update();
    }

    break;

  case sDD_STOP:
    DragMask = 0;
    Update();
    break;
  }
}

/****************************************************************************/


MainWindow::MainWindow()
{
  Doc = new Document;

  sWire = new sWireMasterWindow;
  AddChild(sWire);

  ListWin = new sSingleListWindow<RectElement>(&Doc->Rects);
  ListWin->InitWire(L"List");
  ListWin->AddFieldChoice(L"Mode",sLWF_EDIT,50,sMEMBERPTR(RectElement,Add),L"Sub|Add");
  ListWin->AddField(L"x0",sLWF_EDIT,50,sMemberPtr<sInt>(sOFFSET(RectElement,Rect.x0)),-1024,1024,0.25f);
  ListWin->AddField(L"y0",sLWF_EDIT,50,sMemberPtr<sInt>(sOFFSET(RectElement,Rect.y0)),-1024,1024,0.25f);
  ListWin->AddField(L"x1",sLWF_EDIT,50,sMemberPtr<sInt>(sOFFSET(RectElement,Rect.x1)),-1024,1024,0.25f);
  ListWin->AddField(L"y1",sLWF_EDIT,50,sMemberPtr<sInt>(sOFFSET(RectElement,Rect.y1)),-1024,1024,0.25f);
  ListWin->AddBorder(new sListWindow2Header(ListWin));
  ListWin->ChangeMsg = sMessage(this,&MainWindow::Update);
  RectWin = new RectWindow;
  RectWin->InitWire(L"Rect");

  sWire->ProcessFile(L"recttoy.wire.txt");
  sWire->ProcessEnd();
}

MainWindow::~MainWindow()
{
}

void MainWindow::Tag()
{
  sWindow::Tag();

  Doc->Need();
  ListWin->Need();
  RectWin->Need();
}

void MainWindow::UpdateList()
{
  ListWin->Update();
}


/****************************************************************************/
/****************************************************************************/

void sMain()
{
  sInit(sISF_2D,800,600);
  sInitGui();
  sGui->AddBackWindow(App = new MainWindow);
}

/****************************************************************************/

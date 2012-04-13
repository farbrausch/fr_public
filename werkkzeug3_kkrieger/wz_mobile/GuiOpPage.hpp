// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types2.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Operator Window                                                    ***/
/***                                                                      ***/
/****************************************************************************/


template <class OpType> class sGuiOpPage : public sGuiWindow2
{

  enum sGuiOpPageConst
  {
    PAGESX        = 192,       // width in op-units of a page
    PAGESY        = 96,        // height in op-units of a page
    PAGEW         = 7,         // width in pixel of width-dragging button
    BXSIZE        = 3,         // size for birdseye
    BYSIZE        = 3,         // size for birdseye

    DM_PICK       = 1,         // select one element by picking
    DM_RECT       = 2,         // select by rectangle
    DM_DUPLICATE  = 3,         // duplicate selection
    DM_WIDTH      = 4,         // change width
    DM_SELECT     = 5,         // generic selecting (DM_PICK or DM_RECT)
    DM_MOVE       = 6,         // move around
    DM_SCROLL     = 7,
    DM_BIRDSEYE   = 8,

    SM_SET        = 1,         // clear everything else
    SM_ADD        = 2,         // add to selection (SHIFT)
    SM_TOGGLE     = 3,         // toggle selection (CTRL)
  };

  sInt DragStartX;
  sInt DragStartY;
  sInt DragMouseX;
  sInt DragMouseY;
  sInt DragMoveX;
  sInt DragMoveY;
  sInt DragWidth;
  sInt DragHeight;
  sInt DragMode;
  sRect DragRect;
  sInt SelectMode;

  sRect Bird;
  sRect BirdView;

public:

  OpType *EditOp;
  sObjList<OpType> *Ops;
  sObjList<OpType> DummyOps;
  sObjList<OpType> Clipboard;

  sInt CursorX;
  sInt CursorY;
  sInt CursorWidth;

protected:
  sBool CheckDest(sBool dup);
  void MoveDest(sBool dup);
  sBool CheckDest(sInt dx,sInt dy,sInt dw,sInt dh);
  sBool CheckDest(sObjList<OpType> *source,sInt dx,sInt dy);
  OpType *FindOp(sInt mx,sInt my);

  void OpCut();
  void OpCopy();
  void OpPaste();
  void OpDelete();

  virtual void PaintOp(const OpType *) =0;

  sInt PageX;
  sInt PageY;

  sInt Fullsize;
  sInt BirdsEye;

public:
  enum sGuiOpPageCommands
  {
    CMD_PAGE_MENU     = 0x0100,
    CMD_PAGE_SETOP,
    CMD_PAGE_SHOW,

    CMD_PAGE_BIRDON,
    CMD_PAGE_BIRDOFF,
    CMD_PAGE_BIRDTOGGLE,
    CMD_PAGE_FULLSCREEN,

    CMD_PAGE_ADD,

    CMD_PAGE_PASTE,
    CMD_PAGE_COPY,
    CMD_PAGE_CUT,
    CMD_PAGE_DELETE,

    CMD_PAGE_CHANGED,
  };

  sGuiOpPage();
  ~sGuiOpPage();
  void Tag();

  void OnCalcSize();
  void OnPaint();
  void OnDrag(sDragData &);
  void OnKey(sU32 key);
  sBool OnCommand(sU32 cmd);

  void SetPage(sObjList<OpType> *ops);
  void ScrollToCursor();
  void ScrollToOp(OpType *);
  void MakeRect(const OpType *op,sRect &r);
};

// construction / destruction

template <class OpType> sGuiOpPage<OpType>::sGuiOpPage()
{
  Ops = &DummyOps;
  CursorX = 0;
  CursorY = 0;
  CursorWidth = 3;
  BirdsEye = 0;
  Bird.Init();
  BirdView.Init();
  Fullsize = 0;
  EditOp = 0;
}

template <class OpType> sGuiOpPage<OpType>::~sGuiOpPage()
{
}

template <class OpType> void sGuiOpPage<OpType>::Tag()
{
  sGuiWindow2::Tag();
  Ops->Need();
  DummyOps.Need();
  Clipboard.Need();
}

/****************************************************************************/

template <class OpType> void sGuiOpPage<OpType>::SetPage(sObjList<OpType> *ops)
{ 
  if(ops)
  {
    Ops = ops;
  }
  else
  {
    Ops = &DummyOps;
  }
}

template <class OpType> void sGuiOpPage<OpType>::ScrollToCursor()
{
  sRect r;
  
  r.x0 = Client.x0 + (CursorX)*PageX;
  r.x1 = Client.x0 + (CursorX+CursorWidth)*PageX;
  r.y0 = Client.y0 + (CursorY)*PageY;
  r.y1 = Client.y0 + (CursorY+1)*PageY;
  
  ScrollTo(r,sGWS_SAFE);
}

template <class OpType> void sGuiOpPage<OpType>::ScrollToOp(OpType *op)
{
  OpType *other;
  if(op)
  {
    CursorX = op->PosX;
    CursorY = op->PosY;
    ScrollToCursor();

    sFORALL(*Ops,other)
      other->Selected = 0;
    op->Selected = 1;
  }
}


template <class OpType> void sGuiOpPage<OpType>::MakeRect(const OpType *op,sRect &r)
{
  r.x0 = Client.x0 + op->PosX*PageX;
  r.y0 = Client.y0 + op->PosY*PageY;
  r.x1 = r.x0 + op->Width * PageX;
  r.y1 = r.y0 + op->Height * PageY;    
}

/****************************************************************************/

// checking and moving

template <class OpType> sBool sGuiOpPage<OpType>::CheckDest(sBool dup)
{
  sInt j,k;
  sInt x,y,w,h;
  OpType *po;
  sU8 map[PAGESY][PAGESX];
  
  sSetMem(map,0,sizeof(map));
  sFORALL(*Ops,po)
  {
    x = po->PosX;
    y = po->PosY;
    w = po->Width;
    h = po->Height;

    if(po->Selected)
    {
      if(dup)
      {
        for(k=0;k<h;k++)
        {
          for(j=0;j<w;j++)
          {
            if(map[y+k][x+j])
              return sFALSE;
            map[y+k][x+j] = 1;
          }
        }
      }
      x = sRange(x+DragMoveX,PAGESX-w-1,0);
      y = sRange(y+DragMoveY,PAGESY-h-1,0);
      w = sRange(w+DragWidth,PAGESX-x-1,1);
      h = sRange(h+(po->CanChangeHeight()?DragHeight:0),PAGESY-y-1,1);
    }
    for(k=0;k<h;k++)
    {
      for(j=0;j<w;j++)
      {
        if(map[y+k][x+j])
          return sFALSE;
        map[y+k][x+j] = 1;
      }
    }
  }

  return sTRUE;
}

template <class OpType> void sGuiOpPage<OpType>::MoveDest(sBool dup)
{
  sInt x,y,w,h;
  OpType *po,*opo;

  sFORALL(*Ops,po)
  {
    x = po->PosX;
    y = po->PosY;
    w = po->Width;
    h = po->Height;
    if(po->Selected)
    {
      x = sRange(x+DragMoveX,PAGESX-w-1,0);
      y = sRange(y+DragMoveY,PAGESY-h-1,0);
      w = sRange(w+DragWidth,PAGESX-x-1,1);
      h = sRange(h+(po->CanChangeHeight()?DragHeight:0),PAGESY-y-1,1);

      if(dup)
      {
        opo = po;
        po = new OpType;
        po->Copy(opo);
//          po->ConnectAnim(App->Doc);
//          po->Page = Page;
        Ops->Add(po);
        opo->Selected = sFALSE;
      }
      po->PosX = x;
      po->PosY = y;
      po->Width = w;
      po->Height = h;
//        Page->Change(1);
      sGui->Post(CMD_PAGE_CHANGED,this);
    }
  }
}

template <class OpType> sBool sGuiOpPage<OpType>::CheckDest(sInt dx,sInt dy,sInt dw,sInt dh)
{
  OpType *po;

  sFORALL(*Ops,po)
  {
    if(po->PosY<dy+dh && po->PosY+po->Height>dy)
    {
      if(po->PosX<dx+dw && po->PosX+po->Width>dx)
        return sFALSE;
    }
  }
  return sTRUE;
}

template <class OpType> sBool sGuiOpPage<OpType>::CheckDest(sObjList<OpType> *source,sInt dx,sInt dy)
{
  sInt j,k;
  sInt x,y,w,h;
  OpType *po;
  sU8 map[PAGESY][PAGESX];
 
  sSetMem(map,0,sizeof(map));

  sFORALL(*Ops,po)
  {
    x = po->PosX;
    y = po->PosY;
    w = po->Width;
    h = po->Height;

    for(k=0;k<h;k++)
      for(j=0;j<w;j++)
        map[y+k][x+j] = 1;
  }

  sFORALL(*source,po)
  {
    x = po->PosX+dx;
    y = po->PosY+dy;
    w = po->Width;
    h = po->Height;
    if(x<0 || x+w>PAGESX) return sFALSE;
    if(y<0 || y+h>PAGESX) return sFALSE;

    for(k=0;k<h;k++)
    {
      for(j=0;j<w;j++)
      {
        if(map[y+k][x+j])
          return sFALSE;
        map[y+k][x+j] = 1;
      }
    }
  }

  return sTRUE;
}

template <class OpType> OpType *sGuiOpPage<OpType>::FindOp(sInt mx,sInt my)
{
  OpType *po;
  sRect r;

  sFORALL(*Ops,po)
  {
    MakeRect(po,r);
    if(r.Hit(mx,my))
      return po;
  }

  return 0;
}

/****************************************************************************/

template <class OpType> void sGuiOpPage<OpType>::OpCut()
{
  OpType *op;

  Clipboard.Clear();

  for(sInt i=0;i<Ops->GetCount();)
  {
    op = Ops->Get(i);
    if(op->Selected)
    {
      Ops->Rem(i);
      Clipboard.Add(op);
    }
    else
    {
      i++;
    }
  }
}

template <class OpType> void sGuiOpPage<OpType>::OpCopy()
{
  OpType *op,*op2;

  Clipboard.Clear();

  sFORALL(*Ops,op)
  {
    if(op->Selected)
    {
      op2 = new OpType();
      op2->Copy(op);
      Clipboard.Add(op2);
    }
  }
}

template <class OpType> void sGuiOpPage<OpType>::OpPaste()
{
  OpType *po,*copy;
  sInt xmin,ymin;
  sInt x,y;

  x = CursorX;
  y = CursorY;

  xmin = PAGESX;
  ymin = PAGESY;
  sFORALL(Clipboard,po)
  {
    xmin = sMin(xmin,po->PosX);
    ymin = sMin(ymin,po->PosY);
  }
  if(CheckDest(&Clipboard,x-xmin,y-ymin))
  {
    sFORALL(Clipboard,po)
    {
      copy = new OpType;
      copy->Copy(po);
      copy->PosX += x - xmin;
      copy->PosY += y - ymin;
      Ops->Add(copy);
    }
    sGui->Post(CMD_PAGE_CHANGED,this);
  }
}

template <class OpType> void sGuiOpPage<OpType>::OpDelete()
{
  OpType *op;

  for(sInt i=0;i<Ops->GetCount();)
  {
    op = Ops->Get(i);
    if(op->Selected)
    {
      Ops->Rem(i);
    }
    else
    {
      i++;
    }
  }
  sGui->Post(CMD_PAGE_CHANGED,this);
}

/****************************************************************************/
/***                                                                      ***/
/***   Windowing                                                          ***/
/***                                                                      ***/
/****************************************************************************/

template <class OpType> void sGuiOpPage<OpType>::OnCalcSize()
{
  SizeX = PageX*PAGESX;
  SizeY = PageY*PAGESY;
}

/****************************************************************************/

// paint it!

template <class OpType> void sGuiOpPage<OpType>::OnPaint()
{
  GenOp *op;
  sRect r;
  sU32 col;
  sInt shadow;

  // clear things

  ClearBack();

  sPainter->SetClipping(ClientClip);
  sPainter->EnableClipping(1);

  for(sInt i=1;i<PAGESX;i++)
  {
    r.x0 = Client.x0 + i*PageX;
    r.y0 = Client.y0;
    r.x1 = Client.x0 + i*PageX + 1;
    r.y1 = Client.y1;
    sPainter->Paint(sGui->AddMat,r,0xff101010);
  }

  for(sInt i=1;i<PAGESY;i++)
  {
    r.x0 = Client.x0;
    r.y0 = Client.y0 + i*PageY;
    r.x1 = Client.x1;
    r.y1 = Client.y0 + i*PageY + 1;
    sPainter->Paint(sGui->AddMat,r,0xff101010);
  }
  sPainter->Flush();
  sPainter->SetClipping(ClientClip);
  sPainter->EnableClipping(1);

  // cursor

  r.x0 = Client.x0 + CursorX*PageX;
  r.y0 = Client.y0 + CursorY*PageY;
  r.x1 = Client.x0 + (CursorX+CursorWidth)*PageX;
  r.y1 = Client.y0 + (CursorY+1)*PageY;
  PaintRectHL(r,1,sGC_DRAW,sGC_DRAW);

  // ops

  sFORALL(*Ops,op)
  {
    PaintOp(op);
  }

  // paint shadow for dragging

  shadow = 0;
  if(DragMode==DM_MOVE||DragMode==DM_WIDTH)
    shadow = 1;
  if(DragMode==DM_DUPLICATE)
    shadow = 2;
  sFORALL(*Ops,op)
  {
    if(op->Selected && shadow)
    {
      MakeRect(op,r);
      r.x0 += DragMoveX*PageX;
      r.y0 += DragMoveY*PageY;
      r.x1 += (DragMoveX+DragWidth)*PageX;
      r.y1 += (DragMoveY+(op->CanChangeHeight()?DragHeight:0))*PageY;
      sGui->RectHL(r,shadow,sGui->Palette[sGC_DRAW],sGui->Palette[sGC_DRAW]);
    }
  }

  // drag-rect

  if(DragMode==DM_RECT)
  {
    sGui->RectHL(DragRect,1,sGui->Palette[sGC_DRAW],sGui->Palette[sGC_DRAW]);
  }

  // birdseye

  if(BirdsEye)
  {
    r = Bird;
    r.x0--;
    r.y0--;
    r.x1++;
    r.y1++;
    sGui->RectHL(r,1,sGui->Palette[sGC_HIGH2],sGui->Palette[sGC_LOW2]);
    sPainter->Paint(sGui->AlphaMat,Bird,(sGui->Palette[sGC_BACK]&0x00ffffff)|0xc0000000);

    sPainter->Flush();

    /*
    sFORALL(*Ops,op)
    {
      if(op->Class->Comment)
      {
        r.x0 = op->PosX;
        r.x1 = op->PosX+op->Width;
        r.y0 = op->PosY;
        r.y1 = op->PosY+1;
        r.y0 -= op->DataU[0];
        r.y1 += op->DataU[1];
        r.x0 -= op->DataU[2];
        r.x1 += op->DataU[3];
        r.x0 = Bird.x0+r.x0*BXSIZE;
        r.y0 = Bird.y0+r.y0*BYSIZE;
        r.x1 = Bird.x0+r.x1*BXSIZE;
        r.y1 = Bird.y0+r.y1*BYSIZE;
        col0 = (color[op->DataU[4]] & 0x00ffffff) | 0x3f000000;
        sPainter->Paint(sGui->AlphaMat,r,col0);
      }
    }
    */

    sFORALL(*Ops,op)
    {
      r.x0 = Bird.x0+op->PosX*BXSIZE;
      r.y0 = Bird.y0+op->PosY*BYSIZE;
      r.x1 = r.x0+op->Width*BXSIZE-1;
      r.y1 = r.y0+op->Height*BYSIZE-1;
      col = op->Class->Output->Color;

//      if(op->Error || op->GenOp.CalcError)
//        col = 0xffff0000;

      sPainter->Paint(sGui->FlatMat,r,col);
    }
    r.x0 = Bird.x0 + CursorX*BXSIZE-1;
    r.y0 = Bird.y0 + CursorY*BYSIZE-1;
    r.x1 = r.x0 + BXSIZE*CursorWidth +1;
    r.y1 = r.y0 + BYSIZE +1;
    sGui->RectHL(r,1,sGui->Palette[sGC_TEXT],sGui->Palette[sGC_TEXT]);

    BirdView.x0 = Bird.x0 + ScrollX*BXSIZE/PageX;
    BirdView.y0 = Bird.y0 + ScrollY*BYSIZE/PageY;
    BirdView.x1 = Bird.x0 + (ScrollX+ClientClip.x1-ClientClip.x0)*BXSIZE/PageX;
    BirdView.y1 = Bird.y0 + (ScrollY+ClientClip.y1-ClientClip.y0)*BYSIZE/PageY;
    sGui->RectHL(BirdView,1,sGui->Palette[sGC_TEXT],sGui->Palette[sGC_TEXT]);
  }
}

/****************************************************************************/

// drag it!

template <class OpType> void sGuiOpPage<OpType>::OnDrag(sDragData &dd)
{
  GenOp *po,*op2;
  sRect r;
  sBool pickit;
  sInt mode;
  sU32 keyq;

  if(MMBScrolling(dd,DragStartX,DragStartY)) return;

  switch(dd.Mode)
  {
  case sDD_START:
    DragMouseX = dd.MouseX;
    DragMouseY = dd.MouseY;

    if(BirdsEye)
    {
      DragMode = DM_BIRDSEYE;
      if(!BirdView.Hit(dd.MouseX,dd.MouseY))
      {
        ScrollX = (dd.MouseX-Bird.x0)*PageX/BXSIZE - (ClientClip.x1-ClientClip.x0)/2;
        ScrollY = (dd.MouseY-Bird.y0)*PageY/BYSIZE - (ClientClip.y1-ClientClip.y0)/2;
        ScrollX = sRange<sF32>(ScrollX,PAGESX*PageX - (ClientClip.x1-ClientClip.x0),0);
        ScrollY = sRange<sF32>(ScrollY,PAGESY*PageY - (ClientClip.y1-ClientClip.y0),0);
      }
      DragStartX = ScrollX;
      DragStartY = ScrollY;
      break;
    }

    mode = 0;
    keyq = sSystem->GetKeyboardShiftState();
    if(dd.Buttons& 1) mode = 1;   // lmb
    if(dd.Buttons& 2) mode = 2;   // rmb
    if(dd.Buttons& 4) mode = 3;   // mmb
    if(dd.Buttons& 8) mode = 4;   // xmb
    if(dd.Buttons&16) mode = 5;   // ymb
    if(keyq&sKEYQ_CTRL)
      mode |= 0x10;               // ctrl
    else if(keyq&sKEYQ_SHIFT)
      mode |= 0x20;               // shift
    po = FindOp(dd.MouseX,dd.MouseY);
    if(po)
    {
      MakeRect(po,r);
      if(dd.MouseX>=r.x1-PAGEW)
        mode |= 0x80;             // select at width
      mode |= 0x40;               // select at body
    }


    CursorX = sRange<sInt>((dd.MouseX-Client.x0)/PageX-1,PAGESX-3,0);
    CursorY = sRange<sInt>((dd.MouseY-Client.y0)/PageY  ,PAGESY-1,0);
    CursorWidth = 3;
    pickit = 0;
    DragMode = 0;
    SelectMode = SM_SET;

    switch(mode&0x3f)
    {
    case 0x01:
      if(dd.Flags&sDDF_DOUBLE && po)  { sGui->Post(CMD_PAGE_SHOW,this); break; }
      DragMode = DM_RECT;
      if(mode&0x40)
      {
        DragMode = DM_PICK;
        if(po && po->Selected)
          SelectMode = SM_ADD;
      }
      if(mode&0x80)
      {
        DragMode = DM_WIDTH;
      }
      break;
    case 0x02:
      if(dd.Flags&sDDF_DOUBLE)  { sGui->Post(CMD_PAGE_ADD,this); break; }
      DragMode = DM_WIDTH;
      break;
    case 0x03:
      DragMode = DM_SCROLL;
      break;

    case 0x11:
      DragMode = DM_RECT;
      if(mode&0xc0) DragMode = DM_DUPLICATE;
      break;
    case 0x12:
      DragMode = DM_WIDTH;
      break;
    case 0x13:
      DragMode = DM_SCROLL;
      break;

    case 0x21:
      SelectMode = SM_TOGGLE;
      DragMode = DM_RECT;
      if(mode&0xc0) DragMode = DM_PICK;
      break;
    case 0x22:
      SelectMode = SM_ADD;
      DragMode = DM_RECT;
      if(mode&0xc0) DragMode = DM_PICK;
      break;
    case 0x23:
      DragMode = DM_SCROLL;
      break;

    case 0x04:
      DragMode = DM_DUPLICATE;
      break;

    case 0x05:
      sGui->Post(CMD_PAGE_ADD,this);
      break;
    }

    if(DragMode==DM_DUPLICATE || DragMode==DM_WIDTH || DragMode==DM_MOVE)
    {
      if(po && !po->Selected)
      {
        pickit = 1;
        SelectMode = SM_SET;
      }
    }

    DragStartX = dd.MouseX;
    DragStartY = dd.MouseY;
    DragMoveX = 0;
    DragMoveY = 0;
    DragWidth = 0;
    DragHeight = 0;

    if(DragMode==DM_PICK || pickit)
    {
      if(SelectMode==SM_SET)
        sFORALL(*Ops,op2)
          op2->Selected = sFALSE;
      EditOp = po;
      if(po)
      {
        OnCommand(CMD_PAGE_SETOP);
        if(SelectMode==SM_TOGGLE)
          po->Selected = !po->Selected;
        else
          po->Selected = sTRUE;
      }
    }

    if(DragMode==DM_RECT)
    {
      DragRect.Init(dd.MouseX,dd.MouseY,dd.MouseX+1,dd.MouseY+1);
//        EditOp = 0;
    }
    if(DragMode==DM_SCROLL)
    {
      DragStartX = ScrollX;
      DragStartY = ScrollY;
    }
    break;

  case sDD_DRAG:
    switch(DragMode)
    {
    case DM_PICK:
      if((sAbs(dd.MouseX-DragMouseX)>2 || sAbs(dd.MouseY-DragMouseY)>2) && dd.Buttons==1)
        DragMode = DM_MOVE;
      break;
    case DM_RECT:
      DragRect.Init(dd.MouseX,dd.MouseY,DragStartX,DragStartY);
      DragRect.Sort();
      break;
    case DM_MOVE:
    case DM_DUPLICATE:
      DragMoveX = ((dd.MouseX-DragMouseX)+1024*PageX+PageX/2)/PageX-1024;
      DragMoveY = ((dd.MouseY-DragMouseY)+1024*PageY+PageY/2)/PageY-1024;
      break;
    case DM_WIDTH:
      DragWidth = ((dd.MouseX-DragMouseX)+1024*PageX+PageX/2)/PageX-1024;
      DragHeight = ((dd.MouseY-DragMouseY)+1024*PageY+PageY/2)/PageY-1024;
//      sDPrintF("%d (%d %d)\n",DragWidth,dd.MouseX,DragMouseX);
      break;
    case DM_SCROLL:
      ScrollX = sRange<sInt>(DragStartX-(dd.MouseX-DragMouseX),SizeX-RealX,0);
      ScrollY = sRange<sInt>(DragStartY-(dd.MouseY-DragMouseY),SizeY-RealY,0);
      break;
    case DM_BIRDSEYE:
      ScrollX = DragStartX + (dd.MouseX-DragMouseX)*PageX/BXSIZE;
      ScrollY = DragStartY + (dd.MouseY-DragMouseY)*PageY/BYSIZE;
      break;
    }
    break;

  case sDD_STOP:
    switch(DragMode)
    {
    case DM_RECT:
      sFORALL(*Ops,po)
      {
        if(SelectMode==SM_SET)
          po->Selected = sFALSE;
        MakeRect(po,r);
        if(r.Hit(DragRect))
        {
          if(SelectMode==SM_TOGGLE)
            po->Selected = !po->Selected;
          else
            po->Selected = sTRUE;
        }
      }
      break;
    case DM_MOVE:
    case DM_WIDTH:
      if(CheckDest(sFALSE))
        MoveDest(sFALSE);
      break;
    case DM_DUPLICATE:
      if(CheckDest(sTRUE))
        MoveDest(sTRUE);
      break;
    }
    DragMode = 0;
    DragMoveX = 0;
    DragMoveY = 0;
    DragWidth = 0;
    DragHeight = 0;
    break;
  }
}

/****************************************************************************/

// keyboard shortcuts

template <class OpType> void sGuiOpPage<OpType>::OnKey(sU32 key)
{
  switch(key&(0x8001ffff|sKEYQ_SHIFT|sKEYQ_CTRL))
  {
  case sKEY_UP:
    if(CursorY>0)
      CursorY--;
    ScrollToCursor();
    break;
  case sKEY_DOWN:
    if(CursorY+1<PAGESY)
      CursorY++;
    ScrollToCursor();
    break;
  case sKEY_LEFT:
    if(CursorX>0)
      CursorX--;
    ScrollToCursor();
    break;
  case sKEY_RIGHT:
    if(CursorX+CursorWidth<PAGESX)
      CursorX++;
    ScrollToCursor();
    break;
  default:
    MenuListKey(key);
    break;
  }
}

/****************************************************************************/

// command execution

template <class OpType> sBool sGuiOpPage<OpType>::OnCommand(sU32 cmd)
{
  switch(cmd)
  {
  case CMD_PAGE_BIRDTOGGLE:
    if(!BirdsEye)
      OnCommand(CMD_PAGE_BIRDON);
    else
      OnCommand(CMD_PAGE_BIRDOFF);
    break;

  case CMD_PAGE_BIRDOFF:
    BirdsEye = 0;
    break;

  case CMD_PAGE_BIRDON:
    if(!BirdsEye)
    {
      BirdsEye = 1;
      sGui->GetMouse(Bird.x0,Bird.y0);
      Bird.x0 = Bird.x0 - (ScrollX+ClientClip.XSize()/2)*BXSIZE/PageX;
      Bird.y0 = Bird.y0 - (ScrollY+ClientClip.YSize()/2)*BYSIZE/PageY;
      Bird.x0 = sRange(Bird.x0,ClientClip.x1-BXSIZE*PAGESX-1,ClientClip.x0+1);
      Bird.y0 = sRange(Bird.y0,ClientClip.y1-BYSIZE*PAGESY-1,ClientClip.y0+1);
      Bird.x1 = Bird.x0 + BXSIZE*PAGESX;
      Bird.y1 = Bird.y0 + BYSIZE*PAGESY;
    }
    break;

  case CMD_PAGE_FULLSCREEN:
    Fullsize = 1-Fullsize;
    this->SetFullsize(Fullsize);
    break;

  case CMD_PAGE_CUT:
    OpCut();
    break;

  case CMD_PAGE_COPY:
    OpCopy();
    break;

  case CMD_PAGE_PASTE:
    OpPaste();
    break;

  case CMD_PAGE_DELETE:
    OpDelete();
    break;

  default:
    return sFALSE;
  }
  return sTRUE;
}

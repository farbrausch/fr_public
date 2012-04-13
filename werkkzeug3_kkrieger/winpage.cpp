// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "winpage.hpp"
#include "winview.hpp"
#include "winpara.hpp"
#include "werkkzeug.hpp"

/****************************************************************************/
/****************************************************************************/

#define PAGESX          192       // width in op-units of a page
#define PAGESY          96        // height in op-units of a page
#define PAGEW           7         // width in pixel of width-dragging button
#define BXSIZE 3                  // size for birdseye
#define BYSIZE 3                  // size for birdseye

#define DM_PICK         1         // select one element by picking
#define DM_RECT         2         // select by rectangle
#define DM_DUPLICATE    3         // duplicate selection
#define DM_WIDTH        4         // change width
#define DM_SELECT       5         // generic selecting (DM_PICK or DM_RECT)
#define DM_MOVE         6         // move around
#define DM_SCROLL       7
#define DM_BIRDSEYE     8
#define DM_TIMEDRAG     9

#define SM_SET          1         // clear everything else
#define SM_ADD          2         // add to selection (SHIFT)
#define SM_TOGGLE       3         // toggle selection (CTRL)

/****************************************************************************/
/***                                                                      ***/
/***   Page Window                                                        ***/
/***                                                                      ***/
/****************************************************************************/

static const sChar *patterntex[16*4] =
{
  "1000000000000001",
  "0100000000000010",
  "0010000000000100",
  "0001000000001000",
  "0000100000010000",
  "0000010000100000",
  "0000001001000000",
  "0000000110000000",
  "0000000110000000",
  "0000001001000000",
  "0000010000100000",
  "0000100000010000",
  "0001000000001000",
  "0010000000000100",
  "0100000000000010",
  "1000000000000001",

  "1000000000000000",
  "0100000000000000",
  "0010000000000000",
  "0001000000000000",
  "0000100000000000",
  "0000010000000000",
  "0000001000000000",
  "0000000100000000",
  "0000000010000000",
  "0000000001000000",
  "0000000000100000",
  "0000000000010000",
  "0000000000001000",
  "0000000000000100",
  "0000000000000010",
  "0000000000000001",

  "0000000000000001",
  "0000000000000001",
  "0000000000000010",
  "0000000000011100",
  "0000000000100000",
  "0000000001000000",
  "0000000001000000",
  "0000000001000000",
  "0000000010000000",
  "0000011100000000",
  "0000100000000000",
  "0001000000000000",
  "0001000000000000",
  "0001000000000000",
  "0010000000000000",
  "1100000000000000",

  "0000000000000000",
  "0000000000000000",
  "0000011111100000",
  "0000100000010000",
  "0001000000001000",
  "0010000000000100",
  "0010010000100100",
  "0010000000000100",
  "0010000000000100",
  "0010010000100100",
  "0010001111000100",
  "0001000000001000",
  "0000100000010000",
  "0000011111100000",
  "0000000000000000",
  "0000000000000000",
};


WinPage::WinPage()
{
  sBitmap *bm;
  sInt i,j;
  static const sU32 patterncol[16] = { 0x00000000,0xffffffff };

  App = 0;
  Page = 0;
  EditOp = 0;

  DragKey = DM_SELECT;
  StickyKey = 0;
  DragMode = 0;
  DragStartX = 0;
  DragStartY = 0;
  DragRect.Init();
  DragMoveX = 0;
  DragMoveY = 0;
  DragWidth = 0;
  CursorX = 0;
  CursorY = 0;
  CursorWidth = 3;
  AddScrolling(1,1);
  OpWindowX = -1;
  OpWindowY = -1;
  AddOpClass = CMD_PAGE_POPTEX;
  Flags |= sGWF_FLUSH;

  BackCount = 0;
  BackRev = 0;
  BackMode = 0;
  Birdseye = 0;
  TimeDrag = 0;
  Bird.Init();
  BirdView.Init();

  Fullsize = 0;

  for(j=0;j<4;j++)
  {
    bm = new sBitmap;
    bm->Init(16,16);
    for(i=0;i<16*16;i++)
      bm->Data[i] = patterncol[patterntex[i/16+j*16][i&15]&0x0f];
    Patterns[j] = sPainter->LoadBitmapAlpha(bm);
  }
}

WinPage::~WinPage()
{
}

void WinPage::Tag()
{
  sGuiWindow::Tag();
  sBroker->Need(App);
  sBroker->Need(EditOp);
  sBroker->Need(Page);
}

void WinPage::OnInit()
{
}

void WinPage::OnCalcSize()
{
  if(sGui->GetStyle()&sGS_SMALLFONTS)
  {
    PageX = 18;
    PageY = 15;
  }
  else
  {
    PageX = 28;
    PageY = 19;
  }
  SizeX = PageX*PAGESX;
  SizeY = PageY*PAGESY;
}

void WinPage::SetPage(WerkPage *page)
{ 
  PushBackList();

  if(Page)
  {
    Page->ScrollX = ScrollX;
    Page->ScrollY = ScrollY;
  }
  Page = page;
  if(Page)
  {
    ScrollX = sRange<sInt>(Page->ScrollX,SizeX-RealX,0);
    ScrollY = sRange<sInt>(Page->ScrollY,SizeY-RealY,0);
  }
  App->PagelistWin->SetPage(Page);
}

/****************************************************************************/

void WinPage::PaintButton(sRect r,sChar *name,sU32 color,sBool pressed,sInt style,sU32 col)
{
  sGui->Bevel(r,pressed);
  sPainter->Paint(sGui->FlatMat,r,color);
  if(name)
    sPainter->PrintC(sGui->PropFont,r,0,name,col);
}

void WinPage::ScrollToCursor()
{
  sRect r;
  
  r.x0 = Client.x0 + (CursorX)*PageX;
  r.x1 = Client.x0 + (CursorX+CursorWidth)*PageX;
  r.y0 = Client.y0 + (CursorY)*PageY;
  r.y1 = Client.y0 + (CursorY+1)*PageY;
  
  ScrollTo(r,sGWS_SAFE);
  UpdateCusorPosInStatus();
}

void WinPage::UpdateCusorPosInStatus()
{
  sChar buffer[64];
  sSPrintF(buffer,64,"X:%3d Y:%3d",CursorX,CursorY);
  App->SetStat(STAT_MESSAGE,buffer);
}

void WinPage::MakeRect(WerkOp *op,sRect &r)
{
  r.x1 = Client.x0 + (op->PosX+op->Width)*PageX;
  r.y1 = Client.y0 + (op->PosY+1)*PageY;
  r.x0 = Client.x0 + op->PosX*PageX;
  r.y0 = Client.y0 + op->PosY*PageY;
}

WerkOp *WinPage::FindOp(sInt mx,sInt my)
{
  sInt i,max;
  WerkOp *po;
  sRect r;

  if(Page)
  {
    max = Page->Ops->GetCount();
    for(i=0;i<max;i++)
    {
      po = Page->Ops->Get(i);
      MakeRect(po,r);
      if(r.Hit(mx,my))
        return po;
    }
  }

  return sFALSE;
}

void WinPage::GotoOp(WerkOp *op)
{
  sInt i,j;
  WerkPage *page;
  sRect r;

  op->BlinkTime = sSystem->GetTime()+1500;

  for(i=0;i<App->Doc->Pages->GetCount();i++)
  {
    page = App->Doc->Pages->Get(i);
    for(j=0;j<page->Ops->GetCount();j++)
    {
      if(op==page->Ops->Get(j))
        SetPage(page);
    }
  }

  CursorX = op->PosX;
  CursorY = op->PosY;
  if(Page)
  {
    for(i=0;i<Page->Ops->GetCount();i++)
      Page->Ops->Get(i)->Selected = 0;
  }
  op->Selected = 1;
  MakeRect(op,r);
  ScrollTo(r);
}

sBool WinPage::CheckDest(sBool dup)
{
  sInt i,max,j;
  sInt x,y,w;
  WerkPage *pd;
  WerkOp *po;
  sU8 map[PAGESY][PAGESX];
  
  pd = Page;
  if(!pd) return sFALSE;
  sSetMem(map,0,sizeof(map));
  max = pd->Ops->GetCount();
  for(i=0;i<max;i++)
  {
    po = pd->Ops->Get(i);
    x = po->PosX;
    y = po->PosY;
    w = po->Width;

    if(po->Selected)
    {
      if(dup)
      {
        for(j=0;j<w;j++)
        {
          if(map[y][x+j])
            return sFALSE;
          map[y][x+j] = 1;
        }
      }
      x = sRange(x+DragMoveX,PAGESX-w-1,0);
      y = sRange(y+DragMoveY,PAGESY  -1,0);
      w = sRange(w+DragWidth,PAGESX-x-1,1);
    }
    for(j=0;j<w;j++)
    {
      if(map[y][x+j])
        return sFALSE;
      map[y][x+j] = 1;
    }
  }

  return sTRUE;
}

void WinPage::MoveDest(sBool dup)
{
  sInt i,max;
  sInt x,y,w;
  WerkOp *po,*opo;
  
  if(!Page) return;
  if(!Page->Access()) return;
  max = Page->Ops->GetCount();

  for(i=0;i<max;i++)
  {
    po = Page->Ops->Get(i);
    x = po->PosX;
    y = po->PosY;
    w = po->Width;
    if(po->Selected)
    {
      x = sRange(x+DragMoveX,PAGESX-w-1,0);
      y = sRange(y+DragMoveY,PAGESY  -1,0);
      w = sRange(w+DragWidth,PAGESX-x-1,1);

      if(dup)
      {
        opo = po;
        po = new WerkOp;
        po->Copy(opo);
        po->ConnectAnim(App->Doc);
        po->Page = Page;
        Page->Ops->Add(po);
        opo->Selected = sFALSE;
      }
      po->PosX = x;
      po->PosY = y;
      po->Width = w;
      Page->Change(1);
    }
  }
}

sBool WinPage::CheckDest(WerkPage *source,sInt dx,sInt dy)
{
  sInt i,max,j;
  sInt x,y,w;
  WerkOp *po;
  sU8 map[PAGESY][PAGESX];
 
  sSetMem(map,0,sizeof(map));

  max = Page->Ops->GetCount();
  for(i=0;i<max;i++)
  {
    po = Page->Ops->Get(i);
    x = po->PosX;
    y = po->PosY;
    w = po->Width;

    for(j=0;j<w;j++)
      map[y][x+j] = 1;
  }
  max = source->Ops->GetCount();
  for(i=0;i<max;i++)
  {
    po = source->Ops->Get(i);
    x = po->PosX+dx;
    y = po->PosY+dy;
    w = po->Width;
    if(x<0 || x+w>PAGESX) return sFALSE;
    if(y<0 || y+1>PAGESX) return sFALSE;

    for(j=0;j<w;j++)
    {
      if(map[y][x+j])
        return sFALSE;
      map[y][x+j] = 1;
    }
  }

  return sTRUE;
}

sBool WinPage::CheckDest(sInt dx,sInt dy,sInt dw)
{
  sInt i,max;
  WerkOp *po;

  max = Page->Ops->GetCount();
  for(i=0;i<max;i++)
  {
    po = Page->Ops->Get(i);
    if(po->PosY==dy)
    {
      if(po->PosX<dx+dw && po->PosX+po->Width>dx)
        return sFALSE;
    }
  }
  return sTRUE;
}

void WinPage::Delete()
{
  sInt i;
  sBool updateobjects;
  WerkOp *po;

  if(!Page) return;
  if(!Page->Access()) return;
  updateobjects = 0;
  for(i=0;i<Page->Ops->GetCount();)
  {
    po = Page->Ops->Get(i);
    if(po->Selected)
    {
      if(po == App->ViewWin->Object)
        App->ViewWin->SetOff();
      if(po == App->ParaWin->Op)
        App->ParaWin->SetOp(0);
      Page->Ops->Rem(po);
      po->Page = 0;
    }
    else
    {
      i++;
    }
  }

  Page->Change(1);
}

void WinPage::Copy()
{
  sInt i,max;
  WerkOp *po,*copy;
  WerkPage *clip;

  clip = 0;
  max = Page->Ops->GetCount();
  for(i=0;i<max;i++)
  {
    po = Page->Ops->Get(i);
    if(po->Selected)
    {
      if(clip==0)
        clip = new WerkPage;
      copy = new WerkOp;
      copy->Copy(po);
      copy->Page = 0;
      clip->Ops->Add(copy);
    }
  }
  sGui->ClipboardClear();
  if(clip)
    sGui->ClipboardAdd(clip);
}

void WinPage::Paste(sInt x,sInt y)
{
  sInt i,max;
  WerkPage *clip;
  WerkOp *po,*copy;
  sInt xmin,ymin;

  if(!Page) return;
  if(!Page->Access()) return;
  clip = (WerkPage *)sGui->ClipboardFind(sCID_WERKPAGE);
  if(clip)
  {
    max = clip->Ops->GetCount();
    xmin = PAGESX;
    ymin = PAGESY;
    for(i=0;i<max;i++)
    {
      po = clip->Ops->Get(i);
      xmin = sMin(xmin,po->PosX);
      ymin = sMin(ymin,po->PosY);
    }
    if(CheckDest(clip,x-xmin,y-ymin))
    {
      for(i=0;i<max;i++)
      {
        po = clip->Ops->Get(i);
        copy = new WerkOp;
        copy->Copy(po);
        copy->Page = Page;
        copy->PosX += x - xmin;
        copy->PosY += y - ymin;
        copy->ConnectAnim(App->Doc);
        Page->Ops->Add(copy);
        Page->Change(1);
      }
    }
  } 
}

/****************************************************************************/

void WinPage::OnPaint()
{
  sInt i,max,j,pw,x,y;
  WerkOp *po,*op;
  WerkPage *pd;
  sRect r,rc;
  sFRect rf;
  sInt shadow;
  sU32 col,colt;
  sChar *name;
  sBool small;
  sU32 keyq;
  sInt time;
  static sInt oldtime;
  sInt timedelta;
  sChar *ClipName;
  sInt ClipNameX;
  sInt ClipNameY;
  sInt mx,my;

//  static sChar *dragmodes[] = { "???","pick","rect","duplicate","width","select","move","scroll" };

  time = sSystem->GetTime();
  timedelta = time-oldtime;
  oldtime = time;
  sGui->GetMouse(mx,my);
  ClipName = 0;

  if(sGui->GetFocus()!=this)
    TimeDrag = 0;

  if(Flags&sGWF_CHILDFOCUS)
  {
/*
    static sChar buffer[64];
    sSPrintF(buffer,64,"%d %d\n",BackCount,BackRev);
    App->SetStat(STAT_DRAG,buffer);
    */
    keyq = sSystem->GetKeyboardShiftState();
    if(Birdseye)
    {
      App->SetStat(STAT_DRAG,"l:scroll   m:scroll [menu]   r:scroll");
    }
    else if(TimeDrag)
    {
      App->SetStat(STAT_DRAG,"l:time (slow)   m:scroll [menu]   r:time (fast)");
    }
    else
    {
      if(keyq&sKEYQ_CTRL)
      {
        App->SetStat(STAT_DRAG,"l:duplicate  m:scroll [menu]   r:width");
      }
      else if(keyq&sKEYQ_SHIFT)
      {
        App->SetStat(STAT_DRAG,"l:select toggle  m:scroll [menu]   r:select add");
      }
      else
      {
        App->SetStat(STAT_DRAG,"l:select/width [show]   m:scroll [menu]   r:width [add]");
      }
    }
//    App->SetStat(STAT_DRAG,dragmodes[DragKey]);
    OpWindowX = -1;
    OpWindowY = -1;
  }
  small = (sGui->GetStyle()&sGS_SMALLFONTS) ? 1 : 0;

  ClearBack();
  rf.Init(0,0,Client.XSize()/16.0f,Client.YSize()/16.0f);
  if(Page)
  {
    if(Page->Ops==Page->Orig)
    {
      if(!Page->Access())
      {
        sPainter->Paint(Patterns[0],Client,rf,sGui->Palette[sGC_BUTTON]);
        sPainter->Flush();
      }
      else if(Page->Scratch)
      {
        sPainter->Paint(Patterns[1],Client,rf,sGui->Palette[sGC_BUTTON]);
        sPainter->Flush();
      }
    }
    else if(Page->Ops==Page->Scratch)
    {
      sPainter->Paint(Patterns[2],Client,rf,sGui->Palette[sGC_BUTTON]);
      sPainter->Flush();
    }
  }
  sPainter->SetClipping(ClientClip);
  sPainter->EnableClipping(1);


  r.x0 = Client.x0 + CursorX*PageX;
  r.y0 = Client.y0 + CursorY*PageY;
  r.x1 = Client.x0 + (CursorX+CursorWidth)*PageX;
  r.y1 = Client.y0 + (CursorY+1)*PageY;
  sGui->RectHL(r,1,sGui->Palette[sGC_DRAW],sGui->Palette[sGC_DRAW]);

  if(Page==0)
  {
    sPainter->PrintC(sGui->PropFont,Client,0,"Page");
  }
  else
  {
    pd = Page;
    max = pd->Ops->GetCount();

    // paint body

    for(i=0;i<max;i++)
    {
      po = pd->Ops->Get(i);
      MakeRect(po,r);
      name = po->Class->Name;
      if(po->Name[0])
        name = po->Name;
      if(po->Class->Id == 0x01 &&po->LinkName[0])
        name = po->LinkName[0];


      col = 0xffc0c0c0;
      if(po->Class->Output>0 && po->Class->Output<16)
        col = WerkColors[po->Class->Output];
      if(po->Error || po->Op.CalcError)
        col = 0xffff0000;
      colt = sGui->Palette[sGC_TEXT];
      if(po->Bypass || po->Hide)
        colt = sGui->Palette[sGC_BACK];//(((colt&0xfefefe)>>1)+((col&0xfefefe)>>1))|0xff000000;

      if(po->BlinkTime && po->BlinkTime>time && !(time-po->BlinkTime&256))
        col = 0xff007f00;

      PaintButton(r,0,col,po->Selected,0,colt);
      rc = r;
      rc.x0 += 2;
      rc.x1 -= PAGEW-2;
      if(sPainter->PrintC(sGui->PropFont,rc,sFA_CLIP,name,colt))
      {
        rc.x0 = r.x0;
        rc.x1 = r.x1-PAGEW-1;
        if(rc.Hit(mx,my) && DragMode==0)
        {
          ClipName = name;
          ClipNameX = r.x0;
          ClipNameY = r.y0;
        }
      }
      if(small)
      {
        if(App->ParaWin->Op==po)
          sPainter->Paint(sGui->FlatMat,r.x0+2,r.y0+2,4,3,0xff7f0000);
        if(App->ViewWin->Object==po)
          sPainter->Paint(sGui->FlatMat,r.x0+2,r.y1-5,4,3,0xff007f00);
        if(po->Op.Cache)
          sPainter->Paint(sGui->FlatMat,r.x0+2,(r.y0+r.y1)/2-1,4,2,0xff00007f);
      }
      else
      {
        if(App->ParaWin->Op==po)
          sPainter->Paint(sGui->FlatMat,r.x0+4,r.y0+4,8,4,0xff7f0000);
        if(App->ViewWin->Object==po)
          sPainter->Paint(sGui->FlatMat,r.x0+4,r.y1-8,8,4,0xff007f00);
        if(po->Op.Cache)
          sPainter->Paint(sGui->FlatMat,r.x0+4,r.y0+8,8,4,0xff00007f);
      }

      col = sGui->Palette[sGC_DRAW];
      sPainter->Line(r.x1-PAGEW,r.y0+1,r.x1-PAGEW,r.y1-1,col);
      if(po->Op.Cache && !po->Op.Changed)
      {
        pw = (PageY-2)/3;
        j = (po->WheelPhase&0xffff)*pw/65536+1;
        sPainter->Line(r.x1-PAGEW,r.y0+j,r.x1-1,r.y0+j,col);
        sPainter->Line(r.x1-PAGEW,r.y0+j+pw,r.x1-1,r.y0+j+pw,col);
        sPainter->Line(r.x1-PAGEW,r.y0+j+pw*2,r.x1-1,r.y0+j+pw*2,col);
        if(po->Op.WheelSpeed>0)
        {
          po->WheelPhase += po->Op.WheelSpeed*sMin(timedelta*2,50);
          po->Op.WheelSpeed -= timedelta/5+1;
        }
        else
        {
          po->Op.WheelSpeed = 0;
        }
      }

      switch(po->Class->Id)
      {
      case 0x01:                  // load
        col = sGui->Palette[po->Selected?sGC_LOW2:sGC_HIGH2];
        for(j=4;j>=0;j--)
        {
          sPainter->Line(r.x0+j,r.y0,r.x0,r.y0+j,col);
          sPainter->Line(r.x1-j-1,r.y0,r.x1-1,r.y0+j,col);
          col = sGui->Palette[sGC_BACK];
        }
        sPainter->Line(r.x0,r.y0,r.x0,r.y0+5-2,col);
        sPainter->Line(r.x1-1,r.y0,r.x1-1,r.y0+5-2,col);
        if(!small)
        {
          col = sGui->Palette[po->Selected?sGC_LOW:sGC_HIGH]; j=4;
          sPainter->Line(r.x0+1,r.y0+j,r.x0+j,r.y0+1,col);
          sPainter->Line(r.x1-2,r.y0+j,r.x1-j-1,r.y0+1,col);
        }
        break;

      case 0x02:                  // store
        col = sGui->Palette[po->Selected?sGC_HIGH2:sGC_LOW2];
        for(j=4;j>=0;j--)
        {
          sPainter->Line(r.x0,r.y1-1-j,r.x0+j,r.y1-1,col);
          sPainter->Line(r.x1-1,r.y1-1-j,r.x1-j-1,r.y1-1,col);
          col = sGui->Palette[sGC_BACK];
        }
        sPainter->Line(r.x0,r.y1-1,r.x0,r.y1-5+2,col);
        sPainter->Line(r.x1-1,r.y1-1,r.x1-1,r.y1-5+2,col);
        if(!small)
        {
          col = sGui->Palette[po->Selected?sGC_HIGH:sGC_LOW]; j = 4;
          sPainter->Line(r.x0+1,r.y1-1-j,r.x0+j,r.y1-2,col);
          sPainter->Line(r.x1-2,r.y1-1-j,r.x1-j-1,r.y1-2,col);
        }
        break;

      case 0xcd:                  // portal
      case 0x07:                  // trigger
        j = 4;
        col = sGui->Palette[po->Selected?sGC_LOW2:sGC_HIGH2];
        sPainter->Line(r.x1-j-1,r.y0,r.x1-1,r.y0+j,col);
        col = sGui->Palette[po->Selected?sGC_HIGH2:sGC_LOW2];
        sPainter->Line(r.x1-1,r.y1-1-j,r.x1-j-1,r.y1-1,col);
        col = sGui->Palette[sGC_BACK];
        for(j--;j>=0;j--)
        {
          sPainter->Line(r.x1-j-1,r.y0,r.x1-1,r.y0+j,col);
          sPainter->Line(r.x1-1,r.y1-1-j,r.x1-j-1,r.y1-1,col);
        }
        j = 4;
        sPainter->Line(r.x1-1,r.y0,r.x1-1,r.y0+j-1,col);
        sPainter->Line(r.x1-1,r.y1-1,r.x1-1,r.y1-j+1,col);
        if(!small)
        {
          col = sGui->Palette[po->Selected?sGC_LOW:sGC_HIGH]; 
          sPainter->Line(r.x1-2,r.y0+j,r.x1-j-1,r.y0+1,col);
          col = sGui->Palette[po->Selected?sGC_HIGH:sGC_LOW];
          sPainter->Line(r.x1-2,r.y1-1-j,r.x1-j-1,r.y1-2,col);
        }
        break;

      case 0x05:                  // time
      case 0x06:                  // event
      case 0x19:                  // shaker
      case 0xcb:                  // sector
        j = 4;
        y = (r.y0+r.y1)/2;
        col = sGui->Palette[po->Selected?sGC_LOW2:sGC_HIGH2];
        sPainter->Line(r.x1-j-1,y,r.x1-1,y+j,col);
        col = sGui->Palette[po->Selected?sGC_HIGH2:sGC_LOW2];
        sPainter->Line(r.x1-1,y-j,r.x1-j-1,y,col);
        col = sGui->Palette[sGC_BACK];
        for(j--;j>=0;j--)
        {
          sPainter->Line(r.x1-j-1,y,r.x1-1,y+j,col);
          sPainter->Line(r.x1-1,y-j,r.x1-j-1,y,col);
        }
        j = 4;
        sPainter->Line(r.x1-1,y,r.x1-1,y+j-1,col);
        sPainter->Line(r.x1-1,y,r.x1-1,y-j+1,col);
        if(!small)
        {
          col = sGui->Palette[po->Selected?sGC_LOW:sGC_HIGH];
          sPainter->Line(r.x1-2,y+j,r.x1-j-1,y+1,col);
          col = sGui->Palette[po->Selected?sGC_HIGH:sGC_LOW];
          sPainter->Line(r.x1-2,y-j,r.x1-j-2,y-0,col);
        }
        break;
      }

      if(po->Hide)
      {
        sPainter->Paint(sGui->FlatMat,r.x0,r.y1-3,r.x1-r.x0,3,sGui->Palette[sGC_BACK]);
      }
      if(po->Bypass)
      {
        x = (r.x1+r.x0)/2;

        sPainter->Paint(sGui->FlatMat,x-1,r.y0,2,r.y1-r.y0,sGui->Palette[sGC_TEXT]);
      }
    }

    sPainter->Flush();
    sPainter->SetClipping(ClientClip);
    sPainter->EnableClipping(sTRUE);
    // paint shadow for dragging

    shadow = 0;
    if(DragMode==DM_MOVE||DragMode==DM_WIDTH)
      shadow = 1;
    if(DragMode==DM_DUPLICATE)
      shadow = 2;
    for(i=0;i<max;i++)
    {
      po = pd->Ops->Get(i);
      if(po->Selected && shadow)
      {
        MakeRect(po,r);
        r.x0 += DragMoveX*PageX;
        r.y0 += DragMoveY*PageY;
        r.x1 += (DragMoveX+DragWidth)*PageX;
        r.y1 += DragMoveY*PageY;
        sGui->RectHL(r,shadow,sGui->Palette[sGC_DRAW],sGui->Palette[sGC_DRAW]);
      }
    }
  }

  if(DragMode==DM_RECT)
  {
    sGui->RectHL(DragRect,1,sGui->Palette[sGC_DRAW],sGui->Palette[sGC_DRAW]);
  }

  if(Birdseye)
  {
    r = Bird;
    r.x0--;
    r.y0--;
    r.x1++;
    r.y1++;
    sGui->RectHL(r,1,sGui->Palette[sGC_HIGH2],sGui->Palette[sGC_LOW2]);
    sPainter->Paint(sGui->AlphaMat,Bird,(sGui->Palette[sGC_BACK]&0x00ffffff)|0xc0000000);

    sPainter->Flush();

    max = Page->Ops->GetCount();
    for(i=0;i<max;i++)
    {
      op = Page->Ops->Get(i);
      /*
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
      */
    }

    for(i=0;i<max;i++)
    {
      op = Page->Ops->Get(i);

      r.x0 = Bird.x0+op->PosX*BXSIZE;
      r.y0 = Bird.y0+op->PosY*BYSIZE;
      r.x1 = r.x0+op->Width*BXSIZE-1;
      r.y1 = r.y0+BYSIZE-1;
      col = 0xffc0c0c0;
      if(op->Class->Output>0 && op->Class->Output<16)
        col = WerkColors[op->Class->Output];
      if(op->Error || op->Op.CalcError)
        col = 0xffff0000;
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

  if(!Birdseye && ClipName)
  {
    r.x0 = ClipNameX;
    r.y0 = ClipNameY;
    r.x1 = r.x0+6+sPainter->GetWidth(sGui->PropFont,ClipName);
    r.y1 = r.y0+PageY;

    sPainter->Paint(sGui->FlatMat,r,0xffc0c040);
    sGui->RectHL(r,1,sGui->Palette[sGC_DRAW],sGui->Palette[sGC_DRAW]);
    r.x0+=1;
    sPainter->PrintC(sGui->PropFont,r,sFA_LEFT,ClipName,sGui->Palette[sGC_TEXT]);

  }
}

/****************************************************************************/

void WinPage::OnDrag(sDragData &dd)
{
//  sU32 key;
  WerkOp *po;
  WerkPage *pd;
  sInt i,max;
  sRect r;
//  ParaWin *parawin;
  sBool pickit;
  sInt mode;
  sU32 keyq;

  pd = Page;
  if(pd==0) return;
  max = pd->Ops->GetCount();

  if(MMBScrolling(dd,DragStartX,DragStartY)) return;

  switch(dd.Mode)
  {
  case sDD_START:
    DragMouseX = dd.MouseX;
    DragMouseY = dd.MouseY;

    if(Birdseye)
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
    if(TimeDrag)
    {
      DragMode = DM_TIMEDRAG;
      sInt dummy;
      App->MusicNow(DragStartX,dummy);

      if(App->MusicIsPlaying()) App->MusicStart(2);
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
    UpdateCusorPosInStatus();

    switch(mode&0x3f)
    {
    case 0x01:
      if(dd.Flags&sDDF_DOUBLE)  { sGui->Post(CMD_PAGE_SHOW,this); break; }
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
      if(dd.Flags&sDDF_DOUBLE)  { sGui->Post(CMD_PAGE_POPADD,this); break; }
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
      sGui->Post(CMD_PAGE_POPADD,this);
      break;
    }

/*
    if((dd.Buttons==1) && dd.Flags&sDDF_DOUBLE)
    {
      sGui->Post(CMD_PAGE_SHOW,this);
      break;
    }
    SelectMode = SM_SET;
    key = sSystem->GetKeyboardShiftState();
    if(key&sKEYQ_SHIFT)
      SelectMode = SM_ADD;
    if(key&sKEYQ_CTRL)
      SelectMode = SM_TOGGLE;

    DragMode = DragKey;

    if(dd.Buttons==2)
      DragMode = DM_WIDTH;
    else if(DragMode==DM_SELECT)
    {
      po = FindOp(dd.MouseX,dd.MouseY);
      if(po)
      {
        MakeRect(po,r);
        DragMode = DM_PICK;
        pickit = 1;
        if(po->Selected && SelectMode==SM_SET)
          SelectMode = SM_ADD;
        if(dd.MouseX>=r.x1-PAGEW)
          DragMode = DM_WIDTH;
      }
      else 
      {
        DragMode = DM_RECT;
      }
    }
*/
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

    if(DragMode==DM_PICK || pickit)
    {
      if(SelectMode==SM_SET)
        for(i=0;i<max;i++)
          pd->Ops->Get(i)->Selected = sFALSE;
      EditOp = po;
      if(po)
      {
        App->ParaWin->SetOp(po);
//        if(po->Class->EditHandler)
//          (*po->Class->EditHandler)(App,po);
        if(SelectMode==SM_TOGGLE)
          po->Selected = !po->Selected;
        else
          po->Selected = sTRUE;
      }
    }

    if(DragMode==DM_RECT)
    {
      DragRect.Init(dd.MouseX,dd.MouseY,dd.MouseX+1,dd.MouseY+1);
      EditOp = 0;
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
      break;
    case DM_SCROLL:
      ScrollX = sRange<sInt>(DragStartX-(dd.MouseX-DragMouseX),SizeX-RealX,0);
      ScrollY = sRange<sInt>(DragStartY-(dd.MouseY-DragMouseY),SizeY-RealY,0);
      break;
    case DM_BIRDSEYE:
      ScrollX = DragStartX + (dd.MouseX-DragMouseX)*PageX/BXSIZE;
      ScrollY = DragStartY + (dd.MouseY-DragMouseY)*PageY/BYSIZE;
      break;
    case DM_TIMEDRAG:
      {
        sInt x;
        if(dd.Buttons&2)
          x = DragStartX + dd.DeltaX*0x10000;
        else
          x = DragStartX + dd.DeltaX*0x100;
        if(x<0)
          x = 0;
        App->MusicSeek(x);
      }
      break;
    }
    break;

  case sDD_STOP:
    switch(DragMode)
    {
    case DM_RECT:
      for(i=0;i<max;i++)
      {
        po = pd->Ops->Get(i);
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
    case DM_TIMEDRAG:
      if(App->MusicIsPlaying()) App->MusicStart(1);
      break;
    }
    DragMode = 0;
    DragMoveX = 0;
    DragMoveY = 0;
    DragWidth = 0;
    break;
  }
}

/****************************************************************************/

void WinPage::OnKey(sU32 key)
{
  if(key&sKEYQ_SHIFT) key|=sKEYQ_SHIFT;
  if(key&sKEYQ_CTRL)  key|=sKEYQ_CTRL;

  switch(key&(0x8001ffff|sKEYQ_REPEAT))
  {
  case sKEY_APPFOCUS:
//    App->SetActiveWindow(this);
    break;
  case sKEY_APPPOPUP:
    sGui->Post(CMD_PAGE_POPUP,this);
    break;
  case 'a':
    sGui->Post(CMD_PAGE_POPADD,this);
    break;
  case '1':
    sGui->Post(CMD_PAGE_POPMISC,this);
    break;
  case '2':
    sGui->Post(CMD_PAGE_POPTEX,this);
    break;
  case '3':
    sGui->Post(CMD_PAGE_POPMESH,this);
    break;
  case '4':
    sGui->Post(CMD_PAGE_POPSCENE,this);
    break;
  case '5':
    sGui->Post(CMD_PAGE_POPIPP,this);
    break;
  case '6':
    sGui->Post(CMD_PAGE_POPMINMESH,this);
    break;
/*
  case ' ':
    StickyKey = DM_SELECT;
    DragKey = DM_SELECT;
    break;
//  case 'r':
//    StickyKey = DragKey;
//    DragKey = DM_RECT;
//    break;
  case 'p':
    StickyKey = DragKey;
    DragKey = DM_PICK;
    break;
  case 'd':
    StickyKey = DragKey;
    DragKey = DM_DUPLICATE;
    break;
  case 'w':
    StickyKey = DragKey;
    DragKey = DM_WIDTH;
    break;
  case 'm':
    StickyKey = DragKey;
    DragKey = DM_MOVE;
    break;
  case 'y':
    StickyKey = DragKey;
    DragKey = DM_SCROLL;
    break;

//  case 'r'|sKEYQ_BREAK:
  case 'p'|sKEYQ_BREAK:
  case 'd'|sKEYQ_BREAK:
  case 'w'|sKEYQ_BREAK:
  case 'm'|sKEYQ_BREAK:
  case 'y'|sKEYQ_BREAK:
    DragKey = StickyKey;
    break;
*/

  case 's':
    OnCommand(CMD_PAGE_SHOW);
    break;
  case 'S':
    OnCommand(CMD_PAGE_SHOWROOT);
    break;
  case 'G':
    OnCommand(CMD_PAGE_GOTO);
    break;
  case 'g':
    OnCommand(CMD_PAGE_GOTOLINK);
    break;
  case sKEY_DELETE:
    OnCommand(CMD_PAGE_DELETE);
    break;
  case 'x':
    OnCommand(CMD_PAGE_CUT);
    break;
  case 'c':
    OnCommand(CMD_PAGE_COPY);
    break;
  case 'v':
    OnCommand(CMD_PAGE_PASTE);
    break;
  case 'r':
    OnCommand(CMD_PAGE_RENAME);
    break;
  case 'R':
    OnCommand(CMD_PAGE_FINDREFS);
    break;
  case 'j':
    OnCommand(CMD_PAGE_TOGGLESCRATCH);
    break;
  case 'B':
    OnCommand(CMD_PAGE_FINDBUGS);
    break;
  case 'M':
    OnCommand(CMD_PAGE_FINDMATERIALS);
    break;

  case 'u':
    if(App->ParaWin->Op)
      App->ParaWin->Op->Op.Uncache();
    break;
  }
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
  case sKEY_BACKSPACE:
    OnCommand(CMD_PAGE_BACKSPACE);
    break;
  case sKEY_BACKSPACE|sKEYQ_SHIFT:
    OnCommand(CMD_PAGE_BACKSPACER);
    break;
  case 'y':
    if(!(key&sKEYQ_REPEAT))
      OnCommand(CMD_PAGE_BIRDON);
    break;
  case 'y'|sKEYQ_BREAK:
    OnCommand(CMD_PAGE_BIRDOFF);
    break;
  case 't':
    if(!(key&sKEYQ_REPEAT))
      OnCommand(CMD_PAGE_TIMEDRAGON);
    break;
  case 't'|sKEYQ_BREAK:
    OnCommand(CMD_PAGE_TIMEDRAGOFF);
    break;

  case 'b':
    OnCommand(CMD_PAGE_BYPASS);
    break;
  case 'h':
    OnCommand(CMD_PAGE_HIDE);
    break;
  case 'e':
    OnCommand(CMD_PAGE_EXCHANGEOP);
    break;
  case 'E'|sKEYQ_SHIFT:
    OnCommand(CMD_PAGE_EXCHANGEOP2);
    break;
  case sKEY_TAB:
    Fullsize = 1-Fullsize;
    this->SetFullsize(Fullsize);
    break;
  }
  /*
  if((key&sKEYQ_STICKYBREAK) && StickyKey)
    DragKey = StickyKey;
  if(key&sKEYQ_BREAK && (key&0x1ffff)>='a' && (key&0x1ffff)<='z')
    StickyKey = 0;
    */
}

/****************************************************************************/

sBool WinPage::OnCommand(sU32 cmd)
{
  sMenuFrame *mf;
  sDialogWindow *dw;
  WerkOp *op;
  sChar buffer[256];
  sInt i;
//  ToolObject *to;
//  ViewWin *view;

  switch(cmd)
  {

  case CMD_PAGE_POPUP:
    mf = new sMenuFrame;
    mf->AddMenu("Add",CMD_PAGE_POPADD,'a');
    mf->AddMenu("Add Misc",CMD_PAGE_POPMISC,'1');
    mf->AddMenu("Add Texture",CMD_PAGE_POPTEX,'2');
    mf->AddMenu("Add Mesh",CMD_PAGE_POPMESH,'3');
    mf->AddMenu("Add Scene",CMD_PAGE_POPSCENE,'4');
    mf->AddMenu("Add IPP",CMD_PAGE_POPIPP,'4');
    mf->AddMenu("Add MinMesh",CMD_PAGE_POPMINMESH,'5');
    mf->AddSpacer();
/*
    mf->AddMenu("Normal Mode",' ',' ');
    //mf->AddMenu("Select Rect",'r','r');
    mf->AddMenu("Pick",'p','p');
    mf->AddMenu("Duplicate",'d','d');
    mf->AddMenu("Width",'w','w');
    mf->AddMenu("Move",'m','m');
    mf->AddSpacer();
*/
    mf->AddCheck("Birdseye",CMD_PAGE_BIRDTOGGLE,'y',Birdseye);
    mf->AddMenu("Timedrag",0,'t');
    mf->AddMenu("Go back",CMD_PAGE_BACKSPACE,sKEY_BACKSPACE);
    mf->AddMenu("Undo Go back",CMD_PAGE_BACKSPACER,sKEY_BACKSPACE|sKEYQ_SHIFT);
    mf->AddMenu("Goto..",CMD_PAGE_GOTO,'G'|sKEYQ_SHIFT);
    mf->AddMenu("Goto Link",CMD_PAGE_GOTOLINK,'g');
    mf->AddMenu("Bypass",CMD_PAGE_BYPASS,'b');
    mf->AddMenu("Hide",CMD_PAGE_HIDE,'h');
    mf->AddSpacer();
    mf->AddMenu("Rename",CMD_PAGE_RENAME,'r');
    mf->AddMenu("Show Operator",CMD_PAGE_SHOW,'s');
    mf->AddMenu("Show Root",CMD_PAGE_SHOWROOT,'S'|sKEYQ_SHIFT);
    mf->AddMenu("Exchange Op",CMD_PAGE_EXCHANGEOP,'e');
    mf->AddMenu("Exchange MinMesh/Mesh",CMD_PAGE_EXCHANGEOP2,'E');
    mf->AddSpacer();
    mf->AddMenu("Find references",CMD_PAGE_FINDREFS,'R'|sKEYQ_SHIFT);
    mf->AddMenu("Find bugs",CMD_PAGE_FINDBUGS,'B'|sKEYQ_SHIFT);
    mf->AddMenu("Find materials",CMD_PAGE_FINDMATERIALS,'M'|sKEYQ_SHIFT);
    mf->AddSpacer();
    mf->AddMenu("Delete",CMD_PAGE_DELETE,sKEY_DELETE);
    mf->AddMenu("Cut",CMD_PAGE_CUT,'x');
    mf->AddMenu("Copy",CMD_PAGE_COPY,'c');
    mf->AddMenu("Paste",CMD_PAGE_PASTE,'v');
    mf->AddSpacer();
    mf->AddMenu("make spare",CMD_PAGE_MAKESCRATCH,0);
    mf->AddMenu("keep spare",CMD_PAGE_KEEPSCRATCH,0);
    mf->AddMenu("discard spare",CMD_PAGE_DISCARDSCRATCH,0);
    mf->AddMenu("toggle spare",CMD_PAGE_TOGGLESCRATCH,'j');
    mf->AddBorder(new sNiceBorder);
    mf->SendTo = this;
    sGui->AddPopup(mf);
    return sTRUE;

  case CMD_PAGE_SHOW:
    App->SetStat(STAT_MESSAGE,"");
    if(EditOp)
      App->ViewWin->SetObject(EditOp);
    return sTRUE;

  case CMD_PAGE_SHOWROOT:
    op = App->Doc->FindName("root");
#if sLINK_KKRIEGER
    if(!op) op = App->Doc->FindName("root_2");
    if(!op) op = App->Doc->FindName("root_02");
#endif
    if(!op) op = App->Doc->FindName("root_0");
    if(!op) op = App->Doc->FindName("root_00");
    if(op)
      App->ViewWin->SetObject(op);
    return sTRUE;

  case CMD_PAGE_GOTO:
    App->OpBrowser(this,CMD_PAGE_GOTO2);
    return sTRUE;

  case CMD_PAGE_GOTO2:
    if(App->OpBrowserWin)
    {
      App->OpBrowserWin->GetFileName(buffer,sizeof(buffer));
      op = App->Doc->FindName(buffer);
      if(op)
        GotoOp(op);
      App->OpBrowserOff();
    }
    return sTRUE;

  case CMD_PAGE_GOTOLINK:
    op = App->ParaWin->Op;
    if(op && op->GetLink(0))
      GotoOp(op->GetLink(0));
    return sTRUE;

  case CMD_PAGE_POPADD:
    AddOps(AddOpClass);
    App->SetStat(STAT_MESSAGE,"");
    return sTRUE;

  case CMD_PAGE_POPMISC:
    AddOpClass = KC_ANY;
    AddOps(AddOpClass);
    App->SetStat(STAT_MESSAGE,"");
    return sTRUE;

  case CMD_PAGE_POPTEX:
    AddOpClass = KC_BITMAP;
    AddOps(AddOpClass);
    App->SetStat(STAT_MESSAGE,"");
    return sTRUE;

  case CMD_PAGE_POPMESH:
    AddOpClass = KC_MESH;
    AddOps(AddOpClass);
    App->SetStat(STAT_MESSAGE,"");
    return sTRUE;

  case CMD_PAGE_POPSCENE:
    AddOpClass = KC_SCENE;
    AddOps(AddOpClass);
    App->SetStat(STAT_MESSAGE,"");
    return sTRUE;

  case CMD_PAGE_POPIPP:
    AddOpClass = KC_IPP;
    AddOps(AddOpClass);
    App->SetStat(STAT_MESSAGE,"");
    return sTRUE;

  case CMD_PAGE_POPMINMESH:
    AddOpClass = KC_MINMESH;
    AddOps(AddOpClass);
    App->SetStat(STAT_MESSAGE,"");
    return sTRUE;

/*
  case ' ':
//  case 'r':
  case 'p':
  case 'd':
  case 'w':
  case 'm':
    OnKey(cmd);
    return sTRUE;
*/
  case CMD_PAGE_DELETE:
    Delete();
    return sTRUE;
  case CMD_PAGE_CUT:
    Copy();
    Delete();
    return sTRUE;
  case CMD_PAGE_COPY:
    Copy();
    return sTRUE;
  case CMD_PAGE_PASTE:
    Paste(CursorX,CursorY);
    return sTRUE;

  case CMD_PAGE_BACKSPACE:
    PopBackList(0);
    return sTRUE;

  case CMD_PAGE_BACKSPACER:
    PopBackList(1);
    return sTRUE;

  case CMD_PAGE_BYPASS:
    if(Page)
    {
      for(i=0;i<Page->Ops->GetCount();i++)
      {
        op = Page->Ops->Get(i);
        if(op->Selected)
        {
          op->Bypass = !op->Bypass;
          op->Change(1);
        }
      }
    }
    /*
    if(App->ParaWin->Op)
    {
      App->ParaWin->Op->Bypass = !App->ParaWin->Op->Bypass;
      App->ParaWin->Op->Change(1);
    }
    */
    return sTRUE;

  case CMD_PAGE_HIDE:
    if(Page)
    {
      for(i=0;i<Page->Ops->GetCount();i++)
      {
        op = Page->Ops->Get(i);
        if(op->Selected)
        {
          op->Hide = !op->Hide;
          op->Change(1);
        }
      }
    }
    /*
    if(App->ParaWin->Op)
    {
      App->ParaWin->Op->Hide = !App->ParaWin->Op->Hide;
      App->ParaWin->Op->Change(1);
    }
    */
    return sTRUE;

  case CMD_PAGE_EXCHANGEOP:
    ExchangeOp();
    return sTRUE;

  case CMD_PAGE_EXCHANGEOP2:
    ExchangeOp2();
    return sTRUE;

  case CMD_PAGE_BIRDTOGGLE:
    if(Birdseye)
      OnCommand(CMD_PAGE_BIRDOFF);
    else
      OnCommand(CMD_PAGE_BIRDON);
    return sTRUE;

  case CMD_PAGE_BIRDOFF:
    Birdseye = 0;
    return sTRUE;

  case CMD_PAGE_BIRDON:
    Birdseye = 1;
    TimeDrag = 0;
    sGui->GetMouse(Bird.x0,Bird.y0);
    Bird.x0 = Bird.x0 - (ScrollX+ClientClip.XSize()/2)*BXSIZE/PageX;
    Bird.y0 = Bird.y0 - (ScrollY+ClientClip.YSize()/2)*BYSIZE/PageY;
    Bird.x0 = sRange(Bird.x0,ClientClip.x1-BXSIZE*PAGESX-1,ClientClip.x0+1);
    Bird.y0 = sRange(Bird.y0,ClientClip.y1-BYSIZE*PAGESY-1,ClientClip.y0+1);
    Bird.x1 = Bird.x0 + BXSIZE*PAGESX;
    Bird.y1 = Bird.y0 + BYSIZE*PAGESY;
    return sTRUE;

  case CMD_PAGE_TIMEDRAGON:
    TimeDrag = 1;
    Birdseye = 0;
    return sTRUE;
  case CMD_PAGE_TIMEDRAGOFF:
    TimeDrag = 0;
    return sTRUE;


  case CMD_PAGE_RENAME:
    op = App->ParaWin->Op;
    if(op && op->Name[0])
    {
      sCopyString(RenameOld,op->Name,KK_NAME);
      sCopyString(RenameNew,op->Name,KK_NAME);
      dw = new sDialogWindow;
      dw->InitString(RenameNew,KK_NAME);
      dw->InitOkCancel(this,"Rename","Rename All Occurences",CMD_PAGE_RENAME2,0);
    }
    return sTRUE;

  case CMD_PAGE_RENAME2:
    if(App->Doc->FindName(RenameNew))
    {
      dw = new sDialogWindow;
      dw->InitOk(this,"Rename","Name already used.\nCan't rename",0);
    }
    else
    {
      App->Doc->RenameOps(RenameOld,RenameNew);
      App->SetStat(STAT_MESSAGE,"Rename Ops");
    }
    return sTRUE;

  case CMD_PAGE_MAKESCRATCH:
    if(Page)
      Page->MakeScratch();
    return sTRUE;

  case CMD_PAGE_KEEPSCRATCH:
    if(Page && Page->Access())
      Page->KeepScratch();
    return sTRUE;

  case CMD_PAGE_DISCARDSCRATCH:
    if(Page)
      Page->DiscardScratch();
    return sTRUE;

  case CMD_PAGE_TOGGLESCRATCH:
    if(Page)
      Page->ToggleScratch();
    return sTRUE;

  case CMD_PAGE_FINDREFS:
    op = App->ParaWin->Op;
    if(op)
    {
      App->Doc->DumpReferences(op);
      App->FindResults(sTRUE);
    }
    return sTRUE;

  case CMD_PAGE_FINDBUGS:
    op = App->ParaWin->Op;
    if(op)
    {
      App->Doc->DumpBugsRelative(op);
      App->FindResults(sTRUE);
    }
    return sTRUE;

  case CMD_PAGE_FINDMATERIALS:
    op = App->ParaWin->Op;
    if(op)
    {
      App->Doc->DumpMaterials(op);
      App->FindResults(sTRUE);
    }
    return sTRUE;

  default:
    if(cmd>=1024 && cmd<2048)
    {
      if(WerkClasses[cmd-1024].Name)
        AddOp(&WerkClasses[cmd-1024]);
      return sTRUE;
    }
    return sFALSE;
  }
}

void WinPage::AddOps(sU32 rcid)
{
  sU32 cid;
  sInt i,j;
  sInt added,column;
  sMenuFrame *mf;
//  sInt key,chr;
//  sU8 keys[26];
  sBool ok;

  mf = new sMenuFrame;

  mf->AddMenu("Misc"    ,CMD_PAGE_POPMISC   ,'1')->BackColor = WerkColors[KC_NULL];
  mf->AddMenu("Texture" ,CMD_PAGE_POPTEX    ,'2')->BackColor = WerkColors[KC_BITMAP];
  mf->AddMenu("Mesh"    ,CMD_PAGE_POPMESH   ,'3')->BackColor = WerkColors[KC_MESH];
  mf->AddMenu("Scene"   ,CMD_PAGE_POPSCENE  ,'4')->BackColor = WerkColors[KC_SCENE];
  mf->AddMenu("IPP"     ,CMD_PAGE_POPIPP    ,'5')->BackColor = WerkColors[KC_IPP];
  mf->AddMenu("MinMesh" ,CMD_PAGE_POPMINMESH,'6')->BackColor = WerkColors[KC_MINMESH];

//  mf->AddMenu("MeshMin" ,CMD_PAGE_POPMESHMIN,'2');//->BackColor = App->ClassColor(sCID_GENMESH);
//  mf->AddMenu("Level"   ,CMD_PAGE_POPLEVEL  ,'4');//->BackColor = App->ClassColor(sCID_GENMATERIAL);

//  sSetMem(keys,0,26);
  for(column=0;column<6;column++)
  {
    added = 0;
    for(i=0;WerkClasses[i].Name;i++)
    {
      j = WerkClasses[i].Column;
      ok = (j==column);
      cid = WerkClasses[i].Output;
      if(rcid==KC_ANY)
      {
        if(column!=5 && (cid==KC_MESH || cid==KC_MINMESH || cid==KC_SCENE || cid==KC_BITMAP || cid==KC_IPP))
          ok = sFALSE;
      }
      else
      {
        if(column!=5 && cid!=rcid)
          ok = sFALSE;
      }
      
      cid = rcid;

      if(ok)
      {
        if(added==0)
          mf->AddColumn();

        added++;

        if(column==5)
          mf->AddMenu(WerkClasses[i].Name,1024+i,WerkClasses[i].Shortcut);
        else
          mf->AddMenuSort(WerkClasses[i].Name,1024+i,WerkClasses[i].Shortcut);
      }
    }
  }

  mf->AddBorder(new sNiceBorder);
  mf->SendTo = this;
  if(OpWindowX==-1)
  {
    sGui->GetMouse(OpWindowX,OpWindowY);
    sGui->AddWindow(mf,OpWindowX,OpWindowY,-1,1);
    OpWindowX = mf->Position.x0;
    OpWindowY = mf->Position.y0;
  }
  else
  {
    sGui->AddWindow(mf,OpWindowX,OpWindowY);
  }
}

void WinPage::AddOp(WerkClass *cl)
{
  WerkOp *po;
  sInt i;
  sInt max;

  if(!Page) return;
  if(!Page->Access()) return;

  max = Page->Ops->GetCount();
  for(i=0;i<max;i++)
    Page->Ops->Get(i)->Selected = 0;
  if(CheckDest(CursorX,CursorY,CursorWidth))
  {
    po = new WerkOp;
    po->Init(App,cl->Id,CursorX,CursorY,CursorWidth);
    if(po->Class->MaxInput>1)
      po->Width *= 2;
    po->Page = Page;
    po->Selected = sTRUE;
    Page->Ops->Add(po);
    Page->Change(1);
    if(CursorY<PAGESY)
      CursorY++;
    App->ParaWin->SetOp(po);
    App->PagelistWin->UpdateList();
  }
}

void WinPage::PushBackList()
{
  sInt i;

  if(!BackMode)
  {
    if(BackCount==WINPAGE_MAXBACK)
    {
      for(i=0;i<BackCount;i++)
        BackList[i] = BackList[i+1];
      BackCount--;
    }

    BackList[BackCount].ScrollX = ScrollX;
    BackList[BackCount].ScrollY = ScrollY;
    BackList[BackCount].Page = 0;
    for(i=0;i<App->Doc->Pages->GetCount();i++)
      if(App->Doc->Pages->Get(i)==Page)
        BackList[BackCount].Page = i;
    BackCount++;
    BackRev = BackCount;
  }
}

void WinPage::PopBackList(sInt r)
{
  sInt i,index;

  if(!r)
  {
    if(BackCount>0)
    {
      BackList[BackCount].ScrollX = ScrollX;
      BackList[BackCount].ScrollY = ScrollY;
      BackList[BackCount].Page = 0;
      for(i=0;i<App->Doc->Pages->GetCount();i++)
        if(App->Doc->Pages->Get(i)==Page)
          BackList[BackCount].Page = i;
      BackCount--;
      index = BackCount;
    }
    else
    {
      return;
    }
  }
  else
  {
    if(BackCount < BackRev)
    {
      BackCount++;
      index = BackCount;
    }
    else
    {
      return;
    }
  }

  BackMode = 1;

  i = BackList[index].Page;
  if(i<App->Doc->Pages->GetCount())
    SetPage(App->Doc->Pages->Get(i));
  ScrollX = BackList[index].ScrollX;
  ScrollY = BackList[index].ScrollY;

  BackMode = 0;
}

void WinPage::ExchangeOp()
{
  sInt i,max;
  WerkOp *op,*no;
  sF32 *fp;
  
  if(!Page) return;
  max = Page->Ops->GetCount();
  for(i=0;i<max;i++)
  {
    op = Page->Ops->Get(i);
    if(op->Selected)
    {
      no = 0;
      switch(op->Class->Id)
      {
      case 0x01: // load
        no = new WerkOp;
        no->Init(App,0x02,op->PosX,op->PosY,op->Width);
        no->Page = Page;
        no->Selected = sTRUE;
        sCopyString(no->Name,op->LinkName[0],KK_NAME);
        Page->Ops->AddPos(no,i);
        Page->Ops->Rem(op);
        Page->Change(1);
        no = 0;
        break;

      case 0x02: // store
        no = new WerkOp;
        no->Init(App,0x01,op->PosX,op->PosY,op->Width);
        no->Page = Page;
        no->Selected = sTRUE;
        sCopyString(no->LinkName[0],op->Name,KK_NAME);
        Page->Ops->AddPos(no,i);
        Page->Ops->Rem(op);
        Page->Change(1);
        no = 0;
        break;
      case 0xc1: // Scene_Add
        no = new WerkOp;
        no->Init(App,0x92,op->PosX,op->PosY,op->Width);
        no->Page = Page;
        no->Selected = sTRUE;
        break;
      case 0x92: // Mesh_Add
        no = new WerkOp;
        no->Init(App,0xc1,op->PosX,op->PosY,op->Width);
        no->Page = Page;
        no->Selected = sTRUE;
        break;

      case 0xc3: // Scene_Transform
        no = new WerkOp;
        no->Init(App,0x88,op->PosX,op->PosY,op->Width);
        no->Page = Page;
        no->Selected = sTRUE;
        sCopyMem4(no->Op.GetEditPtrU(1),op->Op.GetEditPtrU(0),9);
        break;
      case 0x88: // Mesh_Transform
        if(*op->Op.GetEditPtrU(0)==0)
        {
          no = new WerkOp;
          no->Init(App,0xc3,op->PosX,op->PosY,op->Width);
          no->Page = Page;
          no->Selected = sTRUE;
          sCopyMem4(no->Op.GetEditPtrU(0),op->Op.GetEditPtrU(1),9);
        }
        break;

      case 0xc2: // Scene_Multiply
        no = new WerkOp;
        no->Init(App,0x95,op->PosX,op->PosY,op->Width);
        no->Page = Page;
        no->Selected = sTRUE;
        sCopyMem4(no->Op.GetEditPtrU(0),op->Op.GetEditPtrU(0),10);
        break;
      case 0x95: // Mesh_Multiply
        fp = op->Op.GetEditPtrF(0);
        if(*op->Op.GetEditPtrU(10)==0 && fp[11]==0 && fp[12]==0 && fp[13]==0 && fp[14]==0 && fp[15]==0 && fp[16]==0)
        {
          no = new WerkOp;
          no->Init(App,0xc2,op->PosX,op->PosY,op->Width);
          no->Page = Page;
          no->Selected = sTRUE;
          sCopyMem4(no->Op.GetEditPtrU(0),op->Op.GetEditPtrU(0),10);
        }
        break;
      }
      if(no)
      {
        sCopyString(no->Name,op->LinkName[0],KK_NAME);
        Page->Ops->AddPos(no,i);
        Page->Ops->Rem(op);
        Page->Change(1);
      }
    }
  }
  App->ParaWin->SetOp(0);
  App->PagelistWin->UpdateList();
}

void WinPage::ExchangeOp2()
{
  sInt i,max;
  WerkOp *op,*no,*so;
  sInt newid;
  sInt thisone;
  
  if(!Page) return;
  max = Page->Ops->GetCount();
  so = 0;
  for(i=0;i<max;i++)
  {
    op = Page->Ops->Get(i);
    if(op->Selected)
    {
      no = 0;
      newid = 0;
      thisone = (op==App->ParaWin->Op);
      switch(op->Class->Id)
      {
        case 0x0101: newid = 0x009d; break; // grid
        case 0x0102: newid = 0x0081; break; // cube
        case 0x0103: newid = 0x0083; break; // torus
        case 0x0104: newid = 0x0084; break; // sphere
        case 0x0105: newid = 0x0082; break; // cylinder
        case 0x0110: newid = 0x0096; break; // MatLink
        case 0x0111: newid = 0x0092; break; // Add
        case 0x0112: newid = 0x0085; break; // SelectALl
        case 0x0114: newid = 0x0093; break; // DeleteFaces
        case 0x0115: newid = 0x00a3; break; // Invert
        case 0x0120: newid = 0x0088; break; // Transform
        case 0x0121: newid = 0x0089; break; // Transform Ex
        case 0x0122: newid = 0x008e; break; // Extrude Normal
        case 0x0123: newid = 0x008f; break; // Displace
        case 0x0124: newid = 0x0091; break; // Perlin
        case 0x0125: newid = 0x00ab; break; // Bend2
   
        case 0x009d: newid = 0x0101; break; // grid
        case 0x0081: newid = 0x0102; break; // cube
        case 0x0083: newid = 0x0103; break; // torus
        case 0x0084: newid = 0x0104; break; // sphere
        case 0x0082: newid = 0x0105; break; // cylinder
        case 0x0096: newid = 0x0110; break; // MatLink
        case 0x0092: newid = 0x0111; break; // Add
        case 0x0085: newid = 0x0112; break; // SelectALl
        case 0x0093: newid = 0x0114; break; // DeleteFaces
        case 0x00a3: newid = 0x0115; break; // Invert
        case 0x0088: newid = 0x0120; break; // Transform
        case 0x0089: newid = 0x0121; break; // Transform Ex
        case 0x008e: newid = 0x0122; break; // Extrude Normal
        case 0x008f: newid = 0x0123; break; // Displace
        case 0x0091: newid = 0x0124; break; // Perlin
        case 0x00ab: newid = 0x0125; break; // Bend2


      case 0x0113: // SelectCube
        no = new WerkOp;
        no->Init(App,0x0086,op->PosX,op->PosY,op->Width);
        no->Page = Page;
        no->Selected = sTRUE;
        {
          sInt flags = *op->Op.GetEditPtrU(0);
          sInt mask = (flags&12)==0?0x010000:0x000100;
          sInt mode = flags & 3;
          if((flags&12)==8)
            mode |= 4;
          *no->Op.GetEditPtrU(0) = mask;
          *no->Op.GetEditPtrU(1) = mode;
        }
        sCopyMem4(no->Op.GetEditPtrU(2),op->Op.GetEditPtrU(1),6);
        break;

      case 0x0086: // SelectCube
        no = new WerkOp;
        no->Init(App,0x0113,op->PosX,op->PosY,op->Width);
        no->Page = Page;
        no->Selected = sTRUE;
        {
          sInt mask = *op->Op.GetEditPtrU(0);
          sInt mode = *op->Op.GetEditPtrU(1);
          sInt flags = mode&3;
          if(mask&0x00ff00)
          {
            if(mode&4)
              flags |= 8;
            else
              flags |= 4;
          }
          *no->Op.GetEditPtrU(0) = flags;
        }
        sCopyMem4(no->Op.GetEditPtrU(1),op->Op.GetEditPtrU(2),6);
        break;
      }
      if(newid)
      {
        no = new WerkOp;
        no->Init(App,newid,op->PosX,op->PosY,op->Width);
        no->Page = Page;
        no->Selected = sTRUE;
        sCopyMem4(no->Op.GetEditPtrU(0),op->Op.GetEditPtrU(0),sMin(no->Op.GetDataWords(),op->Op.GetDataWords()));
      }
      if(no)
      {
        sCopyString(no->Name,op->Name,KK_NAME); 
        sCopyMem(no->LinkName,op->LinkName,sizeof(no->LinkName));
        Page->Ops->AddPos(no,i);
        Page->Ops->Rem(op);
        Page->Change(1);
        if(thisone)
          so = no;
      }
    }
  }
  App->ParaWin->SetOp(0);
  App->PagelistWin->UpdateList();
  if(so)
    App->ParaWin->SetOp(so);
}

/****************************************************************************/
/****************************************************************************/



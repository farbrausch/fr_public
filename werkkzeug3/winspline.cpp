// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "winspline.hpp"
#include "winpage.hpp"
#include "wintimeline.hpp"
#include "winview.hpp"
#include "winnova.hpp"
#include "genoverlay.hpp"

static sU32 SplineColors[4] = { 0xffbf0000,0xff00bf00,0xff0000bf,0xff000000 };

/****************************************************************************/
/****************************************************************************/

#define CMD_SPLINELIST_MENU       0x1000
#define CMD_SPLINELIST_ADDSPLINE  0x1001
#define CMD_SPLINELIST_REMSPLINE  0x1002
#define CMD_SPLINELIST_RENAME     0x1003
#define CMD_SPLINELIST_RENAME2    0x1004
#define CMD_SPLINELIST_TOGGLE1    0x1006
#define CMD_SPLINELIST_TOGGLE2    0x1007
#define CMD_SPLINELIST_TOGGLE3    0x1008
#define CMD_SPLINELIST_TOGGLE4    0x1009

#define CMD_SPLINELIST_SELUP      0x100a
#define CMD_SPLINELIST_SELDOWN    0x100b
#define CMD_SPLINELIST_MOVEUP     0x100c
#define CMD_SPLINELIST_MOVEDOWN   0x100d
#define CMD_SPLINELIST_COPY       0x100e
#define CMD_SPLINELIST_PASTE      0x100f
#define CMD_SPLINELIST_CUT        0x1010

#define SIDEBORDER 16

WinSplineList::WinSplineList()
{
  sToolBorder *tb;

  tb = new sToolBorder();
  tb->AddMenu("Splines",0);
  AddBorder(tb);
  Line = -1;


  AddScrolling(0,1);

  Height = sPainter->GetHeight(sGui->PropFont)+2;
  Clipboard = 0;
}

WinSplineList::~WinSplineList()
{
}

void WinSplineList::Tag()
{
  sGuiWindow::Tag();
  sBroker->Need(Clipboard);
}

/****************************************************************************/

void WinSplineList::ToggleChannel(sInt c)
{
  WerkSpline *spline;
  sList<WerkSpline> *list;

  list = GetSplineList();

  sVERIFY(c>=0 && c<4);

  if(list && Line>=0 && Line<list->GetCount())
  {
    spline = list->Get(Line);
    spline->Channel[c].Selected = !spline->Channel[c].Selected;
    App->SplineParaWin->Update();
  }
}

sList<WerkSpline> *WinSplineList::GetSplineList()
{
  if(App->GetNovaMode())
  {
    if(App->EventPara2Win->Event)
      return App->EventPara2Win->Event->Splines;
    else
      return 0;
  }
  else
  {
    if(App->PageWin->Page)
      return App->PageWin->Page->Splines;
    else
      return 0;
  }
}

/****************************************************************************/

void WinSplineList::OnCalcSize()
{
  sList<WerkSpline> *list;

  list = GetSplineList();
  Height = sPainter->GetHeight(sGui->PropFont)+2;
  SizeX = 75;
  if(list)
    SizeY = Height * list->GetCount();
  else
    SizeY = Height;
}

void WinSplineList::OnPaint()
{
  sInt i,j;
  sInt y,x;
  sRect r,rx;
  WerkSpline *spline;
  sInt sel;
  sList<WerkSpline> *list;

  ClearBack();
  list = GetSplineList();

  y = Client.y0;
  x = Client.x1 - Height*4;
  if(list)
  {
    for(i=0;i<list->GetCount();i++)
    {
      spline = list->Get(i);
      sel = (i==Line);
      /*
      sel = 0;
      for(j=0;j<WS_MAXCHANNEL;j++)
        if(spline->Channel[j].Selected)
          sel = 1;
      */
      r = Client;
      r.y0 = y;
      y+=Height;
      r.y1 = y;
      if(sel)
        sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_SELBACK]);
      r.x1 = x;
      sPainter->PrintC(sGui->PropFont,r,sFA_LEFT|sFA_SPACE,spline->Name,sGui->Palette[sel?sGC_SELECT:sGC_TEXT]);
#if sDEBUG
      sChar buffer[32];
      sSPrintF(buffer,sizeof(buffer),"%d",spline->Index);
      sPainter->PrintC(sGui->PropFont,r,sFA_RIGHT|sFA_SPACE,buffer,sGui->Palette[sel?sGC_SELECT:sGC_TEXT]);
#endif
      for(j=0;j<spline->Spline.Count;j++)
      {
        rx = r;
        rx.x0 = x+j*Height;
        rx.x1 = rx.x0+Height;
        rx.Extend(-1);
        sGui->CheckBox(rx,spline->Channel[j].Selected,SplineColors[j]);
      }
    }
  }
  r = Client;
  r.y0 = y;
}

void WinSplineList::OnKey(sU32 key)
{
  sList<WerkSpline> *list;

  list = GetSplineList();
  if(key&sKEYQ_CTRL) key|=sKEYQ_CTRL;
  sDPrintF("key %08x\n",key);

  switch(key&(0x8001ffff|sKEYQ_CTRL))
  {
  case 'a':
    OnCommand(CMD_SPLINELIST_ADDSPLINE);
    break;

  case 'r':
    OnCommand(CMD_SPLINELIST_RENAME);
    break;

  case sKEY_DELETE:
    OnCommand(CMD_SPLINELIST_REMSPLINE);
    if(list && Line==list->GetCount())
    {
      Line--;
      if(Line>=0)
        App->SplineParaWin->SetSpline(list->Get(Line));
    }
    break; 
  case sKEY_BACKSPACE:
    OnCommand(CMD_SPLINELIST_REMSPLINE);
    if(list && Line>0)
    {
      Line--;
      App->SplineParaWin->SetSpline(list->Get(Line));
    }
    break; 

  case 'x':
    OnCommand(CMD_SPLINELIST_CUT);
    break;
  case 'c':
    OnCommand(CMD_SPLINELIST_COPY);
    break;
  case 'v':
    OnCommand(CMD_SPLINELIST_PASTE);
    break;

  case '1':
    ToggleChannel(0);
    break;
  case '2':
    ToggleChannel(1);
    break;
  case '3':
    ToggleChannel(2);
    break;
  case '4':
    ToggleChannel(3);
    break;

  case sKEY_UP:
    OnCommand(CMD_SPLINELIST_SELUP);
    break;
  case sKEY_DOWN:
    OnCommand(CMD_SPLINELIST_SELDOWN);
    break;
  case sKEY_UP|sKEYQ_CTRL:
    OnCommand(CMD_SPLINELIST_MOVEUP);
    break;
  case sKEY_DOWN|sKEYQ_CTRL:
    OnCommand(CMD_SPLINELIST_MOVEDOWN);
    break;
  } 

}

sBool WinSplineList::OnCommand(sU32 cmd)
{
  WerkSpline *spline;
  sMenuFrame *mf;
  sDialogWindow *diag;
  sList<WerkSpline> *list;

  list = GetSplineList();
  switch(cmd)
  {
  case CMD_SPLINELIST_MENU:
    mf = new sMenuFrame;
    mf->AddMenu("add",CMD_SPLINELIST_ADDSPLINE,'a');
    mf->AddMenu("delete",CMD_SPLINELIST_REMSPLINE,sKEY_BACKSPACE);
    mf->AddMenu("cut",CMD_SPLINELIST_CUT,'x');
    mf->AddMenu("copy",CMD_SPLINELIST_COPY,'c');
    mf->AddMenu("paste",CMD_SPLINELIST_PASTE,'v');
    mf->AddMenu("rename",CMD_SPLINELIST_RENAME,'r');
    mf->AddSpacer();
    mf->AddMenu("toggle x",CMD_SPLINELIST_TOGGLE1,'1');
    mf->AddMenu("toggle y",CMD_SPLINELIST_TOGGLE2,'2');
    mf->AddMenu("toggle z",CMD_SPLINELIST_TOGGLE3,'3');
    mf->AddMenu("toggle w",CMD_SPLINELIST_TOGGLE4,'4');
    mf->AddSpacer();
    mf->AddMenu("scroll up",CMD_SPLINELIST_SELUP,sKEY_UP);
    mf->AddMenu("scroll down",CMD_SPLINELIST_SELDOWN,sKEY_DOWN);
    mf->AddMenu("move up",CMD_SPLINELIST_MOVEUP,sKEY_UP|sKEYQ_CTRL);
    mf->AddMenu("move down",CMD_SPLINELIST_MOVEDOWN,sKEY_DOWN|sKEYQ_CTRL);
    mf->AddBorder(new sNiceBorder);
    mf->SendTo = this;
    sGui->AddPopup(mf);
    return sTRUE;

  case CMD_SPLINELIST_ADDSPLINE:
    if(list)
    {
      spline = new WerkSpline;
      spline->Init(3,"<< new >>",WSK_ANY,1,0);
      list->Add(spline);
      App->Doc->AddSpline(spline);
    }
    return sTRUE;

  case CMD_SPLINELIST_CUT:
    spline = GetSpline();
    if(!spline) return sTRUE;
    Clipboard = new WerkSpline();
    Clipboard->Copy(spline);
    /* no break */
  case CMD_SPLINELIST_REMSPLINE:
    if(list && Line>=0 && Line<list->GetCount())
    {
      spline = list->Get(Line);
      App->SplineParaWin->SetSpline(0);
      App->Doc->RemSpline(spline);
    }
    return sTRUE;

  case CMD_SPLINELIST_COPY:
    spline = GetSpline();
    if(spline)
    {
      Clipboard = new WerkSpline();
      Clipboard->Copy(spline);
      sCopyString(Clipboard->Name,"<< copy >>",KK_NAME);
    }
    return sTRUE;

  case CMD_SPLINELIST_PASTE:
    if(list)
    {
      spline = new WerkSpline();
      spline->Copy(Clipboard);
      list->Add(spline);
      App->Doc->AddSpline(spline);
    }
    return sTRUE;

  case CMD_SPLINELIST_RENAME:
    spline = App->SplineListWin->GetSpline();
    if(spline)
    {
      diag = new sDialogWindow;
      diag->InitString(spline->Name,KK_NAME);
      diag->InitOkCancel(this,"rename","enter new name",CMD_SPLINELIST_RENAME2,0);
    }
    return sTRUE;

  case CMD_SPLINELIST_RENAME2:
    App->Doc->ConnectAnim();
    return sTRUE;

  case CMD_SPLINELIST_TOGGLE1:
    ToggleChannel(0);
    return sTRUE;
  case CMD_SPLINELIST_TOGGLE2:
    ToggleChannel(1);
    return sTRUE;
  case CMD_SPLINELIST_TOGGLE3:
    ToggleChannel(2);
    return sTRUE;
  case CMD_SPLINELIST_TOGGLE4:
    ToggleChannel(3);
    return sTRUE;
        

  case CMD_SPLINELIST_SELUP:
    if(list && list->GetCount()>0)
    {
      Line--;
      if(Line<0)
        Line = 0;
      App->SplineParaWin->SetSpline(list->Get(Line));
    }
    return sTRUE;
    
  case CMD_SPLINELIST_SELDOWN:
    if(list && list->GetCount()>0)
    {
      Line++;
      if(Line>=list->GetCount())
        Line = list->GetCount()-1;
      App->SplineParaWin->SetSpline(list->Get(Line));
    }
    return sTRUE;

  case CMD_SPLINELIST_MOVEUP:
    if(list && Line>=1 && Line<list->GetCount())
    {
      list->Swap(Line-1,Line);
      Line--;
    }
    return sTRUE;

  case CMD_SPLINELIST_MOVEDOWN:
    if(list && Line>=0 && Line+1<list->GetCount())
    {
      list->Swap(Line,Line+1);
      Line++;
    }
    return sTRUE;
  }
  return sFALSE;
}

void WinSplineList::OnDrag(sDragData &dd)
{
  WerkSpline *spline;
  sInt i,j;
  sInt hit;
  sRect rx;
  sU32 qual;
  sBool update;
  sList<WerkSpline> *list;

  list = GetSplineList();

  if(dd.Mode==sDD_START && (dd.Buttons&4) && (dd.Flags&sDDF_DOUBLE))
      OnCommand(CMD_SPLINELIST_MENU);
  if(MMBScrolling(dd,DragX,DragY)) return;
  if(!list) return;
  qual = sSystem->GetKeyboardShiftState();
  update = 0;

  switch(dd.Mode)
  {
  case sDD_START:
    for(i=0;i<list->GetCount();i++)
    {
      spline = list->Get(i);
      hit = -1;

      for(j=0;j<spline->Spline.Count;j++)
      {
        rx.x0 = Client.x1-4*Height+j*Height;
        rx.y0 = i*Height+Client.y0;
        rx.x1 = rx.x0+Height;
        rx.y1 = rx.y0+Height;
        rx.Extend(-1);

        if(dd.Buttons&1)
        {
          spline->Channel[j].Selected = 0;
          update = sTRUE;
        }
        if(rx.Hit(dd.MouseX,dd.MouseY))
        {
          spline->Channel[j].Selected = !spline->Channel[j].Selected;
          if(qual & sKEYQ_CTRL)
            hit = spline->Channel[j].Selected;
          update = sTRUE;
        }
      }

      sVERIFY(spline->Spline.Count>=1)

      rx.x0 = Client.x0;
      rx.y0 = i*Height+Client.y0;
      rx.x1 = Client.x1-4*Height;
      rx.y1 = rx.y0+Height;

      if(rx.Hit(dd.MouseX,dd.MouseY))
      {
        spline->Channel[0].Selected = !spline->Channel[0].Selected;
        hit = spline->Channel[0].Selected;
        update = sTRUE;
      }

      if(hit!=-1)
      {
        for(j=0;j<spline->Spline.Count;j++)
          spline->Channel[j].Selected = hit;
        Line = i;
        App->SplineParaWin->SetSpline(list->Get(Line));
        update = sTRUE;
      }
    }
    break;
  }

  if(update)
    App->SplineParaWin->Update();
}


WerkSpline *WinSplineList::GetSpline()
{
  sList<WerkSpline> *list;

  list = GetSplineList();
  if(list && Line>=0 && Line<list->GetCount())
    return list->Get(Line);
  else
    return 0;
}

void WinSplineList::SetSpline(WerkSpline *spline)
{
  sInt j,l;
  WerkPage *page;
  WerkEvent2 *event;
  WerkSpline *s;

  // deselect all

  sFORLIST(App->Doc->Pages,page)
  {
    sFORLIST(page->Splines,s)
    {
      for(l=0;l<s->Spline.Count;l++)
        s->Channel[l].Selected = 0;
    }
  }
  sFORLIST(App->Doc2->Events,event)
  {
    sFORLIST(event->Splines,s)
    {
      for(l=0;l<s->Spline.Count;l++)
        s->Channel[l].Selected = 0;
    }
  }

  // where does this spline come from? 

  // may be from pages?

  sFORLIST(App->Doc->Pages,page)
  {
    for(j=0;j<page->Splines->GetCount();j++)
    {
      if(page->Splines->Get(j)==spline)
      {
        App->PageWin->SetPage(page);
        Line = j;
        for(l=0;l<s->Spline.Count;l++)
          s->Channel[l].Selected = 1;
        App->SplineParaWin->SetSpline(spline);
        return;
      }
    }
  }

  // may be from nova-events?

  sFORLIST(App->Doc2->Events,event)
  {
    for(j=0;j<event->Splines->GetCount();j++)
    {
      if(event->Splines->Get(j)==spline)
      {
        Line = j;
        for(l=0;l<s->Spline.Count;l++)
          s->Channel[l].Selected = 1;
        App->SplineParaWin->SetSpline(spline);
        return;
      }
    }
  }

  // ok, we failed to find where this spline comes from.

  sVERIFYFALSE;
}

/****************************************************************************/
/****************************************************************************/

#define DM_SPLINE_TIME            1
#define DM_SPLINE_ZOOM            2
#define DM_SPLINE_SCROLL          3
#define DM_SPLINE_MOVE            6
#define DM_SPLINE_RECT            7
#define DM_SPLINE_MOVETIME        8
#define DM_SPLINE_RECTCLEAR       9

#define CMD_SPLINE_SCRATCHTIME    0x1001
#define CMD_SPLINE_REMKEY         0x1003
#define CMD_SPLINE_MENU           0x1004
#define CMD_SPLINE_ALIGNX         0x1005
#define CMD_SPLINE_ALIGNY         0x1006
#define CMD_SPLINE_ALLOFTIME      0x1007
#define CMD_SPLINE_TOGGLE1        0x1008
#define CMD_SPLINE_TOGGLE2        0x1009
#define CMD_SPLINE_TOGGLE3        0x100a
#define CMD_SPLINE_TOGGLE4        0x100b
#define CMD_SPLINE_LINKEDIT       0x100c
#define CMD_SPLINE_KEYS           0x100d
#define CMD_SPLINE_SELECTALL      0x100e
#define CMD_SPLINE_BACKSPACE      0x100f

#define CMD_SPLINE_QUANTSET       0x1100


/****************************************************************************/
/****************************************************************************/

WinSpline::WinSpline()
{
  App = 0;
  DragMode = 0;
  DragKey = 0;

  ValMid = 0;
  ValAmp = 4;
  TimeZoom = 512;

  AddScrolling(1,0);
}

WinSpline::~WinSpline()
{
}

void WinSpline::Tag()
{
  sGuiWindow::Tag();
  sBroker->Need(App);
}

/****************************************************************************/

void WinSpline::Time2Pos(sF32 &x,sF32 &y)
{
  x = x*TimeZoom + (Client.x0+SIDEBORDER);
  y = (ValMid-y) * Client.YSize() / ValAmp + (Client.y0+Client.y1)/2;
}

void WinSpline::Pos2Time(sF32 &x,sF32 &y)
{
  x = (x-Client.x0-SIDEBORDER)/TimeZoom;
  y = (-y+(Client.y0+Client.y1)/2) * ValAmp / Client.YSize() + ValMid;
}

void WinSpline::DPos2Time(sF32 &x,sF32 &y)
{
  x = x/TimeZoom;
  y = - y*ValAmp/Client.YSize();
}

sBool WinSpline::GetTime(sF32 &timef)
{
  sInt dummy,beat;
  sInt ts,te;

  GetEvent(ts,te);
  timef = 0;
  App->MusicNow(beat,dummy);
  if(ts!=te && beat>=ts && beat<=te)
  {
    timef = (1.0f*beat-ts)/(te-ts);
    return 1;
  }
  else
  {
    timef = LastTime;
    return 0;
  }
}

void WinSpline::SetTime(sF32 timef)
{
  sInt beat;
  sInt ts,te;

  GetEvent(ts,te);
  timef = sRange<sF32>(timef,1,0);
  LastTime = timef;
  if(ts!=te)
  {
    beat = sFtol(timef*(te-ts))+ts;
    if(beat>=te && te>ts)
      beat = te-1;
    App->MusicSeek(beat);
  }
}

void WinSpline::MakeDragRect(sRect &r)
{
  sF32 x,y;
  x = DragStartX;
  y = DragStartY;
  Time2Pos(x,y);
  r.x0 = x;
  r.y0 = y;
  x = DragEndX;
  y = DragEndY;
  Time2Pos(x,y);
  r.x1 = x;
  r.y1 = y;
  r.Sort();
}

void WinSpline::MoveSelected(sF32 dx,sF32 dy)
{
  sList<WerkSpline> *list;
  WerkSpline *spline;
  WerkSpline::WerkSplineChannel *chan;
  KSplineKey *key;
  sBool changed;
  sF32 fx=16;

  list = App->SplineListWin->GetSplineList();
  changed = 0;
  if(list)
  {
    for(sInt i=0;i<list->GetCount();i++)
    {
      spline = list->Get(i);
      for(sInt j=0;j<spline->Spline.Count;j++)
      {
        chan = &spline->Channel[j];
        if(chan->Selected)
        {
          changed = 1;
          for(sInt k=0;k<chan->Keys.Count;k++)
          {
            key = &chan->Keys[k];
            if(key->Select)
            {
              key->Value+=dy;
              key->Time = sRange<sF32>(key->Time+dx,1,0);
              changed = 1;
            }
          }
        }
        spline->Sort();
      }
    }
  }

  if(changed)
  {
    if(App->ViewWin->Object && App->ViewWin->Object->Op.GetSplineMax())
      App->ViewWin->Object->Change(sFALSE);
    App->SplineParaWin->Update();
  }
}

void WinSpline::GetEvent(sInt &start,sInt &ende)
{
  WerkEvent2 *ev2;
  WerkEvent *ev1;

  start = 0;
  ende  = 0;
  if(App->GetNovaMode())
  {
    ev2 = App->EventPara2Win->Event;
    if(ev2)
    {
      start = ev2->Event.Start;
      ende  = ev2->Event.End;
    }
  }
  else
  {
    ev1 = App->EventWin->Event;
    if(ev1)
    {
      start = ev1->Event.Start;
      ende  = ev1->Event.End;
    }
  }
}

/****************************************************************************/

void WinSpline::OnPaint()
{
  sU32 col,col0,col1;
  sInt i,j,k;
  sF32 scale;
  sF32 x,x0,x1;
  sF32 y,y0,y1;
  sF32 t0,t1;
  sF32 vals[4];
  sRect r;
  WerkSpline *spline;
  WerkSpline::WerkSplineChannel *chan;
  sList<WerkSpline> *list;
  KSplineKey *key;
  sF32 timef;
  sChar buffer[32];

  static sChar *dragmodes[] = { "???","time","zoom","scroll","add","select","move","rect","width","dup","connect" };
  
  list = App->SplineListWin->GetSplineList();
  if(!list) 
  {
    ClearBack();
    return;
  }
  t0 = ClientClip.x0;
  y0 = 0;
  Pos2Time(t0,y0);
  t0 = sRange<sF32>(t0,1,0);
  t1 = ClientClip.x1;
  y1 = 0;
  Pos2Time(t1,y1);
  t1 = sRange<sF32>(t1,1,0);
  
  if(DragKey=='t')
    App->SetStat(0,"scratch time");
  else
    App->SetStat(0,"edit splines");

  // raster

  ClearBack();
  col = sGui->Palette[sGC_BACK];
  for(i=0;i<=16;i++)
  {
    x = i/16.0f; y=0; Time2Pos(x,y);
    col1 = col-0x181818;
    if((i&15)==0)
      col1 = ((col>>1)&0x7f7f7f7f)|0xff000000;
    sPainter->Paint(sGui->FlatMat,x,Client.y0,1,Client.YSize(),col1);
  }

  scale = 1.0f/10;
  for(j=0;j<4;j++)
  {
    y = 0;     x0 = 0; Time2Pos(x0,y); k  = y;
    y = scale; x1 = 1; Time2Pos(x1,y); k -= y;
    if(k>3)
    {
      for(i=-9;i<=9;i++)
      {
        x = 0; y=i*scale; Time2Pos(x,y);
        col1 = col-0x181818;
        if(i==0)
          col1 = ((col>>1)&0x7f7f7f7f)|0xff000000;
        sPainter->Paint(sGui->FlatMat,x0,y,x1-x0,1,col1);
        if(k>10)
        {
          sSPrintF(buffer,sizeof(buffer),"%5.1f",i*scale);
          sPainter->Print(sGui->PropFont,ClientClip.x0+3,y,buffer);
        }
      }
    }
    scale = scale*10;
  }

  // spline key markers

  for(i=0;i<list->GetCount();i++)
  {
    spline = list->Get(i);
    for(j=0;j<spline->Spline.Count;j++)
    {
      col = SplineColors[j];
      chan = &spline->Channel[j];
      if(chan->Selected)
      {
        for(k=0;k<chan->Keys.Count;k++)
        {
          key = &chan->Keys.Array[k];
          x=key->Time; y=key->Value; Time2Pos(x,y);
          r.Init(x-3,sFtol(y)-3,x+4,sFtol(y)+4);
          if(key->Select)
            sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_SELECT]);
          sGui->RectHL(r,1,col,col);
        }
      }
    }
  }

  // spline lines

  for(i=0;i<list->GetCount();i++)
  {
    spline = list->Get(i);
    for(j=0;j<spline->Spline.Count;j++)
    {
      col = SplineColors[j];
      chan = &spline->Channel[j];
      if(chan->Selected && chan->Keys.Count>=2)
      {
        x0 = t0;
        y0 = spline->Spline.Channel[j].Eval(x0,spline->Spline.Interpolation);
        Time2Pos(x0,y0);
        for(k=1;k<=256;k++)
        {
          x1 = t0 + (t1-t0)*k/256;
          y1 = spline->Spline.Channel[j].Eval(x1,spline->Spline.Interpolation);
          Time2Pos(x1,y1);
          sPainter->Line(x0,y0,x1,y1,col);
          x0 = x1;
          y0 = y1;
        }
      }
    }
  }

  // color bar for color animations

  spline = App->SplineListWin->GetSpline();
  if(spline && spline->Kind==WSK_COLOR && spline->Spline.Count>=3)
  {
    col0 = 0;
    x0 = 0;
    y0 = 0;
    Time2Pos(x0,y0);
    for(k=0;k<=256;k++)
    {
      x1 = t0 + (t1-t0)*k/256.0f;
      y1 = 0;
      for(i=0;i<3;i++)
        vals[i] = spline->Spline.Channel[i].Eval(x1,spline->Spline.Interpolation);
      Time2Pos(x1,y1);
      col1 = (sRange<sInt>(sFtol(vals[0]*255),255,0)<<16)|
             (sRange<sInt>(sFtol(vals[1]*255),255,0)<< 8)|
             (sRange<sInt>(sFtol(vals[2]*255),255,0)    )|0xff000000;
      sPainter->Paint(sGui->FlatMat,sFtol(x0),Client.y1-10,sFtol(x1)-sFtol(x0),10,col1);
      x0 = x1;
      y0 = y1;
    }
  }

  // rubberband rect

  if(DragMode==DM_SPLINE_RECT || DragMode==DM_SPLINE_RECTCLEAR)
  {
    MakeDragRect(r);
    if(r.XSize()>3 || r.XSize()>3)
      sGui->RectHL(r,1,sGui->Palette[sGC_DRAW],sGui->Palette[sGC_DRAW]);
  }

  // current time marker

  col = GetTime(timef)?0xffff0000:0xff000000;
  x = timef;
  y = 0;
  Time2Pos(x,y);
  sPainter->Paint(sGui->FlatMat,sFtol(x),Client.y0,1,Client.YSize(),col);

  // set window size

  SizeX = TimeZoom+SIDEBORDER*2;
}

void WinSpline::OnDrag(sDragData &dd)
{
  sInt i,j,k;
  sF32 x;
  sF32 y;
  sRect r;
  sInt firsthit;
  static sInt lx,ly;
  sF32 dx,fx;
  sF32 dy,fy;
  sF32 dummy,val;
  sBool changed;
  sBool clearsel;
  sInt nosel;
  WerkSpline *spline;
  WerkSpline::WerkSplineChannel *chan;
  KSplineKey *key;
  KSplineKey *SelectKey;
  sInt SplitSplineChannel;
  WerkSpline *SplitSpline;
  sF32 SplitSplineVal;
  sU32 qual;
  sBool update;
  sF32 selecttime;
  sList<WerkSpline> *list;
  sInt ts,te;

  GetEvent(ts,te);
  list = App->SplineListWin->GetSplineList();
  dx = dd.MouseX;
  dy = dd.MouseY;
  Pos2Time(dx,dy);
  fx = dx;
  fy = dy;
  LastTime = sRange<sF32>(dx,1,0);
  qual = sSystem->GetKeyboardShiftState();
  update = 0;
  changed = 0;
  selecttime = -1;

  switch(dd.Mode)
  {
  case sDD_START:
    firsthit = 1;
    DragMode = 0;
    lx = dd.MouseX;
    ly = dd.MouseY;

    switch(dd.Buttons)
    {
    case 4:
      if(dd.Flags&sDDF_DOUBLE)
      {
        OnCommand(CMD_SPLINE_MENU);
      }
      else
      {
        DragMode = DM_SPLINE_SCROLL;
        DragStartX = ScrollX;
        DragStartY = ValMid;
      }
      break;
    case 2:
      DragMode = DM_SPLINE_ZOOM;
      DragStartX = TimeZoom;
      DragStartY = ValAmp;
      DragStartTime = LastTime;
      DragStartPos = dd.MouseX-ClientSave.x0;
      break;
    case 1:
      if(DragKey=='t')
      {
        DragMode =DM_SPLINE_TIME;
      }
      else if(list)
      {
        nosel = 1;
        clearsel = !(qual&sKEYQ_SHIFT);
        SelectKey = 0;
        SplitSpline = 0;
        SplitSplineChannel = 0;
        for(i=0;i<list->GetCount();i++)
        {
          spline = list->Get(i);
          for(j=0;j<spline->Spline.Count;j++)
          {
            chan = &spline->Channel[j];
            if(chan->Selected && chan->Keys.Count>=2)
            {
              for(k=0;k<chan->Keys.Count;k++)
              {
                key = &chan->Keys[k];
                x=key->Time; y=key->Value; Time2Pos(x,y);
                key->OldTime = key->Time;
                r.Init(sFtol(x)-3,sFtol(y)-3,sFtol(x)+4,sFtol(y)+4);
                if(r.Hit(dd.MouseX,dd.MouseY) && nosel)
                {
                  if(key->Select==1)
                    clearsel = 0;
                  key->Select = 1;
                  selecttime = key->Time;
                  update = sTRUE;
                  SelectKey = key;
                  if(!GenOverlayManager->LinkEdit)
                    DragMode = DM_SPLINE_MOVE;
                  nosel = 0;
                }
              }

              dy = val = spline->Spline.Channel[j].Eval(LastTime,spline->Spline.Interpolation);
              dummy = 0;
              Time2Pos(dummy,dy);
              if(sFtol(dy)-3<dd.MouseY && sFtol(dy)+3>dd.MouseY)
              {
                SplitSpline = spline;
                SplitSplineChannel = j;
                SplitSplineVal = val;
              }
            }
          }
        }
        if(nosel && SplitSpline)
        {
          for(k=0;k<SplitSpline->Channel[SplitSplineChannel].Keys.Count;k++)
            SplitSpline->Channel[SplitSplineChannel].Keys[k].Select = 0;
          SelectKey = SplitSpline->AddKey(SplitSplineChannel,LastTime,SplitSplineVal);
          clearsel = 1;
          DragMode = DM_SPLINE_MOVE;
        }

        if(clearsel && !nosel)
        {
          for(i=0;i<list->GetCount();i++)
          {
            spline = list->Get(i);
            for(j=0;j<spline->Spline.Count;j++)
            {
              chan = &spline->Channel[j];
              if(chan->Selected && chan->Keys.Count>=2)
              {
                for(k=0;k<chan->Keys.Count;k++)
                {
                  key = &chan->Keys[k];
                  key->Select = 0;
                  update = sTRUE;
                }
              }
            }
          }
          if(SelectKey)
          {
            SelectKey->Select = 1;
            selecttime = SelectKey->Time;
            update = sTRUE;
          }

          if(SplitSpline)
            SplitSpline->Sort();
        }
        if(nosel && !SplitSpline)
        {
          DragMode = DM_SPLINE_RECT;
          if(clearsel)
            DragMode = DM_SPLINE_RECTCLEAR;
          DragStartX = DragEndX = fx;
          DragStartY = DragEndY = fy;
        }
      }
      if(DragMode==DM_SPLINE_MOVE && (qual&sKEYQ_SHIFT))
        DragMode =DM_SPLINE_MOVETIME;
      break;
    }
    break;
  case sDD_DRAG:
    switch(DragMode)
    {
    case DM_SPLINE_MOVE:
    case DM_SPLINE_MOVETIME:
      dx = dd.MouseX-lx;
      dy = dd.MouseY-ly;
      DPos2Time(dx,dy);
      ly = dd.MouseY;
      fx = 16;
      fy = 0;
      DPos2Time(fx,fy);
      if(DragMode==DM_SPLINE_MOVETIME)
        dy = 0;

      if(list)
      {
        for(i=0;i<list->GetCount();i++)
        {
          spline = list->Get(i);
          for(j=0;j<spline->Spline.Count;j++)
          {
            chan = &spline->Channel[j];
            if(chan->Selected)
            {
              changed = 1;
              for(k=0;k<chan->Keys.Count;k++)
              {
                key = &chan->Keys[k];
                if(key->Select)
                {
                  key->Value+=dy;
                  key->Time = sRange<sF32>(key->OldTime+dx,1,0);
                  if(k>0 && key->Time < chan->Keys[k-1].Time && key->Time > chan->Keys[k-1].Time-fx)
                    key->Time = chan->Keys[k-1].Time;
                  if(k<chan->Keys.Count-1 && key->Time > chan->Keys[k+1].Time && key->Time < chan->Keys[k+1].Time+fx)
                    key->Time = chan->Keys[k+1].Time;

                  changed = 1;
                }
              }
            }
            spline->Sort();
          }
        }
      }
      break;
    case DM_SPLINE_ZOOM:
      TimeZoom = sRange<sInt>(DragStartX * (1.0f+dd.DeltaX*0.01f),65536,16);
      ValAmp = sRange<sF32>(DragStartY * (1.0f+dd.DeltaY*0.01f),2100,1/2.0f);
      ScrollX = sRange<sInt>(sFtol(DragStartTime*TimeZoom+0.5f)-DragStartPos+SIDEBORDER,SizeX-RealX,0);
      break;
    case DM_SPLINE_SCROLL:
      dx = dd.DeltaX;
      dy = dd.DeltaY;
      DPos2Time(dx,dy);
      ValMid = DragStartY-dy;
      ScrollX = sRange<sInt>(DragStartX-dd.DeltaX,SizeX-RealX,0);
      break;
    case DM_SPLINE_RECT:
    case DM_SPLINE_RECTCLEAR:
      DragEndX = fx;
      DragEndY = fy;
      break;
    case DM_SPLINE_TIME:
      if(ts!=te)
      {
        dx = dd.MouseX;
        dy = dd.MouseY;
        Pos2Time(dx,dy);
        SetTime(dx);
      }
      break;
    }
    break;
  case sDD_STOP:
    if(DragMode==DM_SPLINE_RECT || DragMode==DM_SPLINE_RECTCLEAR)
    {
      MakeDragRect(r);
      if(list && (r.XSize()>3 || r.YSize()>3))
      {
        if(DragStartX > DragEndX) sSwap(DragStartX,DragEndX);
        if(DragStartY > DragEndY) sSwap(DragStartY,DragEndY);

        for(i=0;i<list->GetCount();i++)
        {
          spline = list->Get(i);
          for(j=0;j<spline->Spline.Count;j++)
          {
            chan = &spline->Channel[j];
            if(chan->Selected)
            {
              changed = 1;
              for(k=0;k<chan->Keys.Count;k++)
              {
                key = &chan->Keys[k];
                if(DragMode==DM_SPLINE_RECTCLEAR)
                  key->Select = 0;
                if(key->Time>DragStartX && key->Time<DragEndX &&
                   key->Value>DragStartY && key->Value<DragEndY)
                {
                  key->Select = 1;
                  selecttime = key->Time;
                  update = sTRUE;
                }
              }
            }
          }
        }
      }
    }

    DragMode = 0; 
    break;
  }

  if(selecttime>=0)
  {
    SetTime(selecttime);
    if(GenOverlayManager->LinkEdit)
      AllOfTime();
  }

  if(update)
    App->SplineParaWin->Update();

  if(changed)
  {
    if(App->ViewWin->Object && App->ViewWin->Object->Op.GetSplineMax())
      App->ViewWin->Object->Change(sFALSE);
  }

  if(GenOverlayManager->LinkEdit)
  {
    App->ViewWin->OnCommand(CMD_VIEW_LINKEDIT);
    App->ViewWin->OnCommand(CMD_VIEW_LINKEDIT);
  }
}

void WinSpline::OnKey(sU32 key)
{
  switch(key&(0x8001ffff|sKEYQ_REPEAT))
  {

  case sKEY_UP:
  case sKEY_UP|sKEYQ_REPEAT:
    MoveSelected(0,ValAmp/sMax(1,ClientClip.YSize()));
    break;
  case sKEY_DOWN:
  case sKEY_DOWN|sKEYQ_REPEAT:
    MoveSelected(0,-ValAmp/sMax(1,ClientClip.YSize()));
    break;
  case sKEY_LEFT:
  case sKEY_LEFT|sKEYQ_REPEAT:
    MoveSelected(-1.0f/TimeZoom,0);
    break;
  case sKEY_RIGHT:
  case sKEY_RIGHT|sKEYQ_REPEAT:
    MoveSelected(1.0f/TimeZoom,0);
    break;

  case sKEY_BACKSPACE:
    OnCommand(CMD_SPLINE_BACKSPACE);
    break;
  case sKEY_DELETE:
  case 'x':
    OnCommand(CMD_SPLINE_REMKEY);
    break;
  case 'b':
    OnCommand(CMD_SPLINE_ALIGNX);
    break;
  case 'v':
    OnCommand(CMD_SPLINE_ALIGNY);
    break;
  case 'a':
    OnCommand(CMD_SPLINE_ALLOFTIME);
    break;
  case 'A':
    OnCommand(CMD_SPLINE_SELECTALL);
    break;
  case 'l':
    OnCommand(CMD_SPLINE_LINKEDIT);
    break;
  case 'k':
    OnCommand(CMD_SPLINE_KEYS);
    break;

  case 't':
    DragKey = key&0xff;
    break;
  case 't'|sKEYQ_BREAK:
    DragKey = 0;
    break;

  case '1':
    App->SplineListWin->ToggleChannel(0);
    break;
  case '2':
    App->SplineListWin->ToggleChannel(1);
    break;
  case '3':
    App->SplineListWin->ToggleChannel(2);
    break;
  case '4':
    App->SplineListWin->ToggleChannel(3);
    break;
  }
}

sBool WinSpline::OnCommand(sU32 cmd)
{
  sMenuFrame *mf;
  sF32 time;

  switch(cmd)
  {
  case CMD_SPLINE_MENU:
    mf = new sMenuFrame;
    mf->AddMenu("Remove Key",CMD_SPLINE_REMKEY,'x');
    mf->AddMenu("Align Time",CMD_SPLINE_ALIGNX,'b');
    mf->AddMenu("Align Value",CMD_SPLINE_ALIGNY,'v');
    mf->AddMenu("All of same time",CMD_SPLINE_ALLOFTIME,'a');
    mf->AddMenu("select all",CMD_SPLINE_SELECTALL,'A'|sKEYQ_SHIFT);
    mf->AddMenu("Add Keys",CMD_SPLINE_KEYS,'k');
    mf->AddCheck("Link Edit",CMD_SPLINE_LINKEDIT,'l',GenOverlayManager->LinkEdit);
    mf->AddCheck("Scratch Time",CMD_SPLINE_SCRATCHTIME,'t',DragKey=='t');
    mf->AddSpacer();
    mf->AddMenu("toggle x",CMD_SPLINE_TOGGLE1,'1');
    mf->AddMenu("toggle y",CMD_SPLINE_TOGGLE2,'2');
    mf->AddMenu("toggle z",CMD_SPLINE_TOGGLE3,'3');
    mf->AddMenu("toggle w",CMD_SPLINE_TOGGLE4,'4');
    mf->AddBorder(new sNiceBorder);
    mf->SendTo = this;
    sGui->AddPopup(mf);
    return sTRUE;

  case CMD_SPLINE_REMKEY:
    ClipDeleteKey();
    return sTRUE;
  case CMD_SPLINE_TOGGLE1:
    App->SplineListWin->ToggleChannel(0);
    return sTRUE;
  case CMD_SPLINE_TOGGLE2:
    App->SplineListWin->ToggleChannel(1);
    return sTRUE;
  case CMD_SPLINE_TOGGLE3:
    App->SplineListWin->ToggleChannel(2);
    return sTRUE;
  case CMD_SPLINE_TOGGLE4:
    App->SplineListWin->ToggleChannel(3);
    return sTRUE;
  case CMD_SPLINE_ALIGNX:
    Align(1);
    return sTRUE;
  case CMD_SPLINE_ALIGNY:
    Align(2);
    return sTRUE;
  case CMD_SPLINE_SCRATCHTIME:
    if(DragKey)
      DragKey = 0;
    else
      DragKey = 't';
    return sTRUE;
  case CMD_SPLINE_ALLOFTIME:
    AllOfTime();
    return sTRUE;
  case CMD_SPLINE_SELECTALL:
    SelectAll();
    return sTRUE;
  case CMD_SPLINE_LINKEDIT:
    App->ViewWin->OnCommand(CMD_VIEW_LINKEDIT);
    return sTRUE;
  case CMD_SPLINE_KEYS:
    GetTime(time);
    AddKeys(time);
    return sTRUE;
  
  case CMD_SPLINE_BACKSPACE:
    if(App->GetNovaMode())
      App->SetMode(MODE_EVENT2);
    else
      App->SetMode(MODE_PAGE);
    return sTRUE;

  default:
    return sFALSE;
  }
}

/****************************************************************************/

void WinSpline::ClipDeleteKey()
{
  sInt i;
  WerkSpline *spline;
  sList<WerkSpline> *list;

  list = App->SplineListWin->GetSplineList();
  if(!list) return;
  for(i=0;i<list->GetCount();i++)
  {
    spline = list->Get(i);
    spline->RemSelection();
    spline->Sort();
    App->SplineParaWin->Update();
    if(App->ViewWin->Object && App->ViewWin->Object->Op.GetSplineMax())
      App->ViewWin->Object->Change(sFALSE);
  }
}

void WinSpline::Align(sU32 mask)
{
  sInt i,j,k;
  WerkSpline *spline;
  WerkSpline::WerkSplineChannel *chan;

  sF32 value;
  sF32 time;
  sInt count;
  sList<WerkSpline> *list;

  list = App->SplineListWin->GetSplineList();
  value = 0;
  time = 0;
  count = 0;
  if(!list) return;

  for(i=0;i<list->GetCount();i++)
  {
    spline = list->Get(i);
    for(j=0;j<spline->Spline.Count;j++)
    {
      chan = &spline->Channel[j];
      if(chan->Selected)
      {
        for(k=0;k<chan->Keys.Count;k++)
        {
          if(chan->Keys[k].Select)
          {
            time += chan->Keys[k].Time;
            value += chan->Keys[k].Value;
            count++;
          }
        }
      }
    }
  }

  if(count==0) return;
  time = time/count;
  value = value/count;

  for(i=0;i<list->GetCount();i++)
  {
    spline = list->Get(i);
    for(j=0;j<spline->Spline.Count;j++)
    {
      chan = &spline->Channel[j];
      if(chan->Selected)
      {
        for(k=0;k<chan->Keys.Count;k++)
        {
          if(chan->Keys[k].Select)
          {
            if(mask&1)   chan->Keys[k].Time = time;
            if(mask&2)   chan->Keys[k].Value = value;
          }
        }
      }
    }
  }

  if(App->ViewWin->Object && App->ViewWin->Object->Op.GetSplineMax())
    App->ViewWin->Object->Change(sFALSE);
}

void WinSpline::AllOfTime()
{
  sInt i,j,k;
  WerkSpline *spline;
  WerkSpline::WerkSplineChannel *chan;

  sF32 value;
  sF32 time;
  sInt count;
  sList<WerkSpline> *list;

  list = App->SplineListWin->GetSplineList();
  value = 0;
  time = 0;
  count = 0;
  if(!list) return;

  for(i=0;i<list->GetCount();i++)
  {
    spline = list->Get(i);
    for(j=0;j<spline->Spline.Count;j++)
    {
      chan = &spline->Channel[j];
      if(chan->Selected)
      {
        for(k=0;k<chan->Keys.Count;k++)
        {
          if(chan->Keys[k].Select)
          {
            time = chan->Keys[k].Time;
            goto findall;
          }
        }
      }
    }
  }
  return;
findall:
  for(i=0;i<list->GetCount();i++)
  {
    spline = list->Get(i);
    for(j=0;j<spline->Spline.Count;j++)
    {
      chan = &spline->Channel[j];
      if(chan->Selected)
      {
        for(k=0;k<chan->Keys.Count;k++)
        {
          chan->Keys[k].Select = chan->Keys[k].Time==time;
        }
      }
    }
  }
  App->SplineParaWin->Update();
}

void WinSpline::SelectAll()
{
  sInt i,j,k;
  WerkSpline *spline;
  WerkSpline::WerkSplineChannel *chan;
  sList<WerkSpline> *list;

  list = App->SplineListWin->GetSplineList();
  if(!list) return;

  for(i=0;i<list->GetCount();i++)
  {
    spline = list->Get(i);
    for(j=0;j<spline->Spline.Count;j++)
    {
      chan = &spline->Channel[j];
      if(chan->Selected)
      {
        for(k=0;k<chan->Keys.Count;k++)
          chan->Keys[k].Select = 1;
      }
    }
  }
}

void WinSpline::AddKeys(sF32 time)
{
  sInt i,j,k;
  sVector v;
  WerkSpline *spline;
  WerkSpline::WerkSplineChannel *chan;
  sList<WerkSpline> *list;

  list = App->SplineListWin->GetSplineList();
  if(!list) return;

  for(i=0;i<list->GetCount();i++)
  {
    spline = list->Get(i);
    spline->Spline.Eval(time,v);
    for(j=0;j<spline->Spline.Count;j++)
    {
      chan = &spline->Channel[j];
      if(chan->Selected)
      {
        // first unselect everything
        for(k=0;k<chan->Keys.Count;k++)
          chan->Keys[k].Select = 0;

        // then create a new key
        spline->AddKey(j,time,(&v.x)[j]);
      }
    }
    spline->Sort();
  }
}

void WinSpline::LinkEdit(sF32 *srt,sBool spline_to_srt)
{
  sInt i,j,k;
  WerkSpline *spline;
  WerkSpline::WerkSplineChannel *chan;
  sList<WerkSpline> *list;

  list = App->SplineListWin->GetSplineList();
  if(!list) return;

  for(i=0;i<list->GetCount();i++)
  {
    spline = list->Get(i);
    for(j=0;j<spline->Spline.Count && j<3;j++)
    {
      chan = &spline->Channel[j];
      if(chan->Selected)
      {
        for(k=0;k<chan->Keys.Count;k++)
        {
          if(chan->Keys[k].Select)
          {
            if(spline_to_srt)
            {
              if(spline->Kind == WSK_ROTATE)
                srt[3+j] = chan->Keys[k].Value;
              if(spline->Kind == WSK_TRANSLATE)
                srt[6+j] = chan->Keys[k].Value;
            }
            else
            {
              if(spline->Kind == WSK_ROTATE)
                chan->Keys[k].Value = srt[3+j];
              if(spline->Kind == WSK_TRANSLATE)
                chan->Keys[k].Value = srt[6+j];
            }
          }
        }
      }
    }
  }
}


/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   spline parameter window                                            ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#define CMD_SPLINEPARA_UPDATECOUNT    0x0101  // spline channel count has changed
#define CMD_SPLINEPARA_UPDATENAME     0x0102
#define CMD_SPLINEPARA_UPDATETIME     0x0103

WinSplinePara::WinSplinePara()
{
  Grid = new sGridFrame;
  Grid->SetGrid(12,4,0,sPainter->GetHeight(sGui->PropFont)+2);
  Grid->AddScrolling(0,1);
  AddChild(Grid);
  Spline = 0;
  Line = 0;
}

void WinSplinePara::Reset()
{
  Grid->RemChilds();
  Flags |= sGWF_LAYOUT;
  Spline = 0;
}

void WinSplinePara::Tag()
{
  sDummyFrame::Tag();
  sBroker->Need(Grid);
  sBroker->Need(Spline);
}

void WinSplinePara::OnKey(sU32 key)
{
  // pretty useless, all keys go to Grid
}

/****************************************************************************/

void WinSplinePara::SetSpline(WerkSpline *spline)
{
  sControl *con;
  sInt i,j,k;
  KSplineKey *key;
  sChar buffer[64];
  WerkSpline *sp;
  sList<WerkSpline> *list;

  list = App->SplineListWin->GetSplineList();

  Reset();
  Spline = spline;
  if(Spline)
  {
    Grid->SetGrid(12,4,0,sPainter->GetHeight(sGui->PropFont)+2);
    Line = 0;
    
    Label("Name");
    con = new sControl;
    con->EditString(CMD_SPLINEPARA_UPDATENAME,Spline->Name,0,sizeof(Spline->Name));
    con->ChangeCmd = 0;
    Grid->Add(con,3,12,Line); 
    Line++;

    NewCount = Spline->Spline.Count;
    Label("Channels");
    con = new sControl;
    con->EditChoice(CMD_SPLINEPARA_UPDATECOUNT,&NewCount,0,"0|1|2|3|4");
    Grid->Add(con,3,12,Line);
    Line++;

    Label("Kind");
    con = new sControl;
    con->EditChoice(0,&Spline->Kind,0,"Any|Scale|Rotate|Translate|Color");
    Grid->Add(con,3,12,Line);
    Line++;

    Label("Interpolation");
    con = new sControl;
    con->EditChoice(0,&Spline->Spline.Interpolation,0,"Cubic|Linear|Step");
    Grid->Add(con,3,12,Line);
    Line++;
  }

  Line++;

  if(list)
  {
    for(k=0;k<list->GetCount();k++)
    {
      sp = list->Get(k);
      for(i=0;i<sp->Spline.Count;i++)
      {
        if(sp->Channel[i].Selected)
        {
          for(j=0;j<sp->Channel[i].Keys.Count;j++)
          {
            key = &sp->Channel[i].Keys[j];
            if(key->Select)
            {
              sSPrintF(buffer,sizeof(buffer),"%16s %c:%d",sp->Name,("xyzw")[i],j);
              Label(buffer);
              con = new sControl;
              con->EditFloat(CMD_SPLINEPARA_UPDATETIME,&key->Time,0);
              con->InitNum(0,1,0.01f,0);
              Grid->Add(con,3,6,Line);
              con = new sControl;
              con->EditFloat(0,&key->Value,0);
              con->InitNum(-256,256,0.01f,0);
              Grid->Add(con,6,12,Line);
              Line++;
            }
          }
        }
      }
    }
  }
  Grid->Flags |= sGWF_LAYOUT;
  App->SetMode(MODE_SPLINE);
}


sBool WinSplinePara::OnCommand(sU32 cmd)
{
  sInt i;
  switch(cmd)
  {
  case CMD_SPLINEPARA_UPDATECOUNT:
    if(Spline && NewCount>=1 && NewCount<=4 && Spline->Spline.Count!=NewCount)
    {
      for(i=NewCount;i<Spline->Spline.Count;i++)
      {
        Spline->Channel[i].Keys.Count=0;
        Spline->Spline.Channel[i].Keys = 0;
        Spline->Spline.Channel[i].KeyCount = 0;
      }
      for(i=Spline->Spline.Count;i<NewCount;i++)
      {
        Spline->Channel[i].Keys.Count=0;
        Spline->AddKey(i,0,0);
        Spline->AddKey(i,1,0);
        Spline->Channel[i].Selected = 1;
        Spline->Spline.Channel[i].Keys = Spline->Channel[i].Keys.Array;
        Spline->Spline.Channel[i].KeyCount = 2;
      }
      Spline->Spline.Count = NewCount;
    }
    return sTRUE;

  case CMD_SPLINEPARA_UPDATENAME:
    App->Doc->ConnectAnim();
    return sTRUE;
    
  case CMD_SPLINEPARA_UPDATETIME:
    if(Spline->Sort())
      SetSpline(Spline);
    return sTRUE;
  }
  return sFALSE;
}

void WinSplinePara::Label(sChar *label)
{
  sControl *con;
  con = new sControl;
  con->Label(label);
  con->LayoutInfo.Init(0,Line,3,Line+1);
  con->Style |= sCS_LEFT;
  Grid->AddChild(con);
}

void WinSplinePara::AddBox(sU32 cmd,sInt pos,sInt offset,sChar *name)
{
  sControl *con;
  con = new sControl;
  con->Button(name,cmd|offset);
  con->LayoutInfo.Init(11-pos,Line,12-pos,Line+1);
  Grid->AddChild(con);
}

/****************************************************************************/
/****************************************************************************/

// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "wintimeline.hpp"
#include "winview.hpp"
#include "winpage.hpp"
#include "winpara.hpp"

#define BLOCKONE 20

#define DM_TIME_TIME       1
#define DM_TIME_ZOOM       2
#define DM_TIME_SCROLL     3
#define DM_TIME_ADD        4
#define DM_TIME_SELECT     5
#define DM_TIME_MOVE       6
#define DM_TIME_RECT       7
#define DM_TIME_WIDTH      8
#define DM_TIME_DUP        9

#define CMD_TIME_CUT       0x0100
#define CMD_TIME_COPY      0x0101
#define CMD_TIME_PASTE     0x0102
#define CMD_TIME_DELETE    0x0103
#define CMD_TIME_POPUP     0x0104
#define CMD_TIME_ADD       0x0105
#define CMD_TIME_QUANTPOP  0x0106
#define CMD_TIME_QUANT     0x0107
#define CMD_TIME_GOTO      0x0108
#define CMD_TIME_MARK      0x0109
#define CMD_TIME_QUANTSET  0x01c0


#define CMD_EVENT_CHANGE          0x0101  // parameter has changed
#define CMD_EVENT_PAGES           0x0102  // page list
#define CMD_EVENT_NAMES           0x0103  // name list
#define CMD_EVENT_GOTO            0x0104  // goto op
#define CMD_EVENT_CONNECT         0x0105  // name has changed
#define CMD_EVENT_UPDATEPOS       0x0106

#define CMD_EVENT_PAGE            0x5400  // a selected page
#define CMD_EVENT_OP              0x5500  // a selected op

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Timeline                                                           ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

WinTimeline::WinTimeline()
{
  BeatZoom = 0x40000;
  BeatQuant = 0x10000;
  DragStartX = 0;
  DragStartY = 0;
  DragMoveX = 0;
  DragMoveY = 0;
  DragMoveZ = 0;
  DragFrame.Init();
  DragKey = DM_TIME_SELECT;
  Level = 0;
  LastTime = 0;
  LastLine = 0;
  App = 0;
  Current = 0;
  Clipboard = new sList<WerkEvent>;

  AddScrolling();
}

WinTimeline::~WinTimeline()
{
}

void WinTimeline::Tag()
{
  sGuiWindow::Tag();
  sBroker->Need(App);
  sBroker->Need(Current);
  sBroker->Need(Clipboard);
}

sBool WinTimeline::MakeRect(WerkEvent *we,sRect &r)
{
  r.x0 = Client.x0 + (sMulShift(we->Event.Start,BeatZoom)>>16);
  r.x1 = Client.x0 + (sMulShift(we->Event.End  ,BeatZoom)>>16);
  r.y0 = Client.y0 +  we->Line   *BLOCKONE;
  r.y1 = Client.y0 + (we->Line+1)*BLOCKONE;
  return sTRUE;
}

sBool WinTimeline::MakeRectMove(WerkEvent *we,sRect &r)
{
  sInt m;

  m = 1;
  if(DragMode == DM_TIME_WIDTH)
    m = 0;
  r.x0 = Client.x0 + (sMulShift((we->Event.Start+DragMoveX*m),BeatZoom)>>16);
  r.x1 = Client.x0 + (sMulShift((we->Event.End  +DragMoveX  ),BeatZoom)>>16);
  r.y0 = Client.y0 + (we->Line+  DragMoveY*m)*BLOCKONE;
  r.y1 = Client.y0 + (we->Line+1+DragMoveY*m)*BLOCKONE;
  return sTRUE;
}

void WinTimeline::ClipCut()
{
  ClipCopy();
  ClipDelete();
  ClipDeselect();
}

void WinTimeline::ClipDeselect()
{
  sInt i,max;

  WerkEvent *ev;

  max = App->Doc->Events->GetCount();
  for(i=0;i<max;i++)
  {
    ev = App->Doc->Events->Get(i);
    ev->Select = sFALSE;
  }
}

void WinTimeline::ClipCopy()
{
  sInt i,max;

  WerkEvent *ev,*copy;
  Clipboard->Clear();

  max = App->Doc->Events->GetCount();
  for(i=0;i<max;i++)
  {
    ev = App->Doc->Events->Get(i);
    if(ev->Select)
    {
      copy = new WerkEvent;
      copy->Copy(ev);
      Clipboard->Add(copy);
    }
  }
}

void WinTimeline::ClipPaste()
{
  sInt i,max;
  sInt minline,mintime;
  WerkEvent *ev,*copy;;
  sInt line,time;

  line = LastLine;
  time = (LastTime+BeatQuant/2)&~(BeatQuant-1);

  max = Clipboard->GetCount();
  if(max==0) return;

  ClipDeselect();

  ev = Clipboard->Get(0);
  minline = ev->Line;
  mintime = ev->Event.Start;
  for(i=1;i<max;i++)
  {
    ev = Clipboard->Get(i);
    minline = sMin(minline,ev->Line);
    mintime = sMin(mintime,ev->Event.Start);
  }
  mintime &= 0xffff0000;

  for(i=0;i<max;i++)
  {
    ev = Clipboard->Get(i);
    copy = new WerkEvent;
    copy->Copy(ev);
    copy->Line = sMax(0,ev->Line - minline + line);
    copy->Event.Start = sMax(0,ev->Event.Start - mintime + time);
    copy->Event.End = copy->Event.Start + ev->Event.End - ev->Event.Start;
    copy->Select = sTRUE;
    App->Doc->Events->Add(copy);
  }
}

void WinTimeline::ClipDelete()
{
  sInt i;
  WerkEvent *we;

  for(i=0;i<App->Doc->Events->GetCount();)
  {
    we = App->Doc->Events->Get(i);
    if(we->Select)
      App->Doc->Events->Rem(we);
    else
      i++;
  }

  App->Doc->ClearInstanceMem();
}

void WinTimeline::Quant()
{
  sInt i,max;
  WerkEvent *we;
  sList<WerkEvent> *list;

  list = App->Doc->Events;

  max = list->GetCount();
  for(i=0;i<max;i++)
  {
    we = list->Get(i);
    if(we->Select)
    {
      we->Event.Start = (we->Event.Start+BeatQuant/2)&~(BeatQuant-1);
      we->Event.End   = (we->Event.End  +BeatQuant/2)&~(BeatQuant-1);
    }
  }
}

/****************************************************************************/

void WinTimeline::OnKey(sU32 key)
{
  switch(key&(0x8001ffff|sKEYQ_REPEAT))
  {
  case 't':
    DragKey = DM_TIME_TIME;
    break;
  case 'z':
    DragKey = DM_TIME_ZOOM;
    break;
  case 'y':
    DragKey = DM_TIME_SCROLL;
    break;
  case ' ':
    DragKey = DM_TIME_SELECT;
    break;
  case 'n':
    DragKey = DM_TIME_MOVE;
    break;
  case 'r':
    DragKey = DM_TIME_RECT;
    break;
  case 'w':
    DragKey = DM_TIME_WIDTH;
    break;
  case 'd':
    DragKey = DM_TIME_DUP;
    break;

  case 't'|sKEYQ_BREAK:
  case 'z'|sKEYQ_BREAK:
  case 'y'|sKEYQ_BREAK:
  case ' '|sKEYQ_BREAK:
  case 'n'|sKEYQ_BREAK:
  case 'r'|sKEYQ_BREAK:
  case 'w'|sKEYQ_BREAK:
  case 'd'|sKEYQ_BREAK:
    DragKey = DM_TIME_SELECT;
    break;

  case sKEY_BACKSPACE:
  case sKEY_DELETE:
    OnCommand(CMD_TIME_DELETE);
    break;
  case 'x':
    OnCommand(CMD_TIME_CUT);
    break;
  case 'c':
    OnCommand(CMD_TIME_COPY);
    break;
  case 'v':
    OnCommand(CMD_TIME_PASTE);
    break;
  case 'a':
    OnCommand(CMD_TIME_ADD);
    break;
  case 'q':
    OnCommand(CMD_TIME_QUANT);
    break;
  case 'g':
    OnCommand(CMD_TIME_GOTO);
    break;
  case 'm':
    OnCommand(CMD_TIME_MARK);
    break;

  case sKEY_LEFT:
  case sKEY_LEFT|sKEYQ_REPEAT:
    MoveSelected((key&sKEYQ_SHIFT)?-0x100000:-BeatQuant,0,DM_TIME_MOVE);
    break;
  case sKEY_RIGHT:
  case sKEY_RIGHT|sKEYQ_REPEAT:
    MoveSelected((key&sKEYQ_SHIFT)?0x100000:BeatQuant,0,DM_TIME_MOVE);
    break;
  case sKEY_UP:
  case sKEY_UP|sKEYQ_REPEAT:
    MoveSelected(0,-1,DM_TIME_MOVE);
    break;
  case sKEY_DOWN:
  case sKEY_DOWN|sKEYQ_REPEAT:
    MoveSelected(0,1,DM_TIME_MOVE);
    break;
  }
}

sBool WinTimeline::OnCommand(sU32 cmd)
{
  sMenuFrame *mf;
  sInt x;
  WerkEvent *we;

  switch(cmd)
  {
  case CMD_TIME_POPUP:
    mf = new sMenuFrame;
    mf->AddMenu("Add",CMD_TIME_ADD,'a');
    mf->AddMenu("Goto",CMD_TIME_GOTO,'g');
    mf->AddSpacer();
    mf->AddMenu("Normal Mode",' ',' ');
    mf->AddMenu("Scratch Time",'t','t');
    mf->AddMenu("Zoom",'z','z');
    mf->AddMenu("Scroll",'y','y');
    mf->AddMenu("Move",'n','n');
    mf->AddMenu("Rect",'r','r');
    mf->AddMenu("Width",'w','w');
    mf->AddMenu("Duplicate",'d','d');
    mf->AddSpacer();
    mf->AddMenu("Delete",CMD_TIME_DELETE,sKEY_DELETE);
    mf->AddMenu("Cut",CMD_TIME_CUT,'x');
    mf->AddMenu("Copy",CMD_TIME_COPY,'c');
    mf->AddMenu("Paste",CMD_TIME_PASTE,'v');
    mf->AddSpacer();
    mf->AddMenu("Quant",CMD_TIME_QUANT,'q');
    mf->AddMenu("Set Quant Step",CMD_TIME_QUANTPOP,0);
    mf->AddMenu("Mark",CMD_TIME_MARK,'m');
    mf->AddBorder(new sNiceBorder);
    mf->SendTo = this;
    sGui->AddPopup(mf);
    return sTRUE;
  case CMD_TIME_QUANTPOP:
    mf = new sMenuFrame;
    mf->AddMenu("$40000 (full bar)"       ,CMD_TIME_QUANTSET+0,0);
    mf->AddMenu("$10000 (1/4 note, beat)" ,CMD_TIME_QUANTSET+1,0);
    mf->AddMenu("$01000 (1/16 note)"      ,CMD_TIME_QUANTSET+2,0);
    mf->AddMenu("$00001 (off)"            ,CMD_TIME_QUANTSET+3,0);
    mf->AddBorder(new sNiceBorder);
    mf->SendTo = this;
    sGui->AddPopup(mf);
    return sTRUE;
  case CMD_TIME_QUANT:
    Quant();
    return sTRUE;
  case CMD_TIME_QUANTSET+0:
    BeatQuant = 0x40000;
    return sTRUE;
  case CMD_TIME_QUANTSET+1:
    BeatQuant = 0x10000;
    return sTRUE;
  case CMD_TIME_QUANTSET+2:
    BeatQuant = 0x01000;
    return sTRUE;
  case CMD_TIME_QUANTSET+3:
    BeatQuant = 0x00001;
    return sTRUE;

  case CMD_TIME_ADD:
    we = new WerkEvent;
    we->Doc = App->Doc;
    x=(LastTime+BeatQuant/2)&~(BeatQuant-1);
    we->Event.Start = x;
    we->Event.End = x+0x00100000;
    we->Line = LastLine;
    we->Select = 1;
    if(App->ParaWin->Op && App->ParaWin->Op->Name[0])
      sCopyString(we->Name,App->ParaWin->Op->Name,sizeof(we->Name));
    else
      sCopyString(we->Name,"new",sizeof(we->Name));
    App->Doc->Events->Add(we);
    App->EventWin->SetEvent(we);
    App->EventWin->OnCommand(CMD_EVENT_CONNECT);
    return sTRUE;
  case CMD_TIME_DELETE:
    ClipDelete();
    return sTRUE;
  case CMD_TIME_CUT:
    ClipCut();
    return sTRUE;
  case CMD_TIME_COPY:
    ClipCopy();
    return sTRUE;
  case CMD_TIME_PASTE:
    ClipPaste();
    return sTRUE;

  case CMD_TIME_MARK:
    MarkTime();
    return sTRUE;

  case CMD_TIME_GOTO:
    App->EventWin->OnCommand(CMD_EVENT_GOTO);
    return sTRUE;

  default:
    return sFALSE;
  }
}

void WinTimeline::OnPaint()
{
  sInt i,x,max,w,skip,beat;
  sRect r;
  sChar buffer[128];
  WerkEvent *we;
  sU32 col,col1;
  sU32 kinds[2][4] =
  {
    { 0xff000000,0xffffffc0,0xffffc0ff,0xffc0ffff },
    { 0xff000000,0xffc0c000,0xffc000c0,0xff00c0c0 }
  };
  static sChar *dragmodes[] = { "???","time","zoom","scroll","add","select","move","rect","width","dup" };

  if(Flags&sGWF_CHILDFOCUS)
    App->SetStat(STAT_DRAG,dragmodes[DragKey]);

  col = sGui->Palette[sGC_BACK];
  ClearBack();
  skip = 4;
  if(BeatZoom>0x40000)
    skip = 1;
  for(i=0;i<App->Doc->SoundEnd>>16;i+=skip)
  {
    x = sMulShift(i,BeatZoom)+Client.x0;
    col1 = col-0x181818;
    if((i&(skip*4-1))==0)
      col1 = ((col>>1)&0x7f7f7f7f)|0xff000000;
    if((i&15)==0)
    {
      sSPrintF(buffer,sizeof(buffer),"%d",i+1);
      sPainter->Print(sGui->PropFont,x,ClientClip.y0,buffer,sGui->Palette[sGC_TEXT]);
    }
    sPainter->Paint(sGui->FlatMat,x,Client.y0,1,Client.YSize(),col1);
  }
  for(i=0;i<=64;i+=8)
    sPainter->Paint(sGui->FlatMat,Client.x0,Client.y0+i*BLOCKONE,Client.XSize(),1,col1);

  App->MusicNow(beat,i);
  x = (sMulShift(beat,BeatZoom)>>16)+Client.x0;
  sPainter->Paint(sGui->FlatMat,x,Client.y0,2,Client.YSize(),0xffff0000);

  max = App->Doc->Events->GetCount();

  for(i=0;i<max;i++)
  {
    we = App->Doc->Events->Get(i);
    if(MakeRect(we,r))
    {
      col = sGui->Palette[sGC_BUTTON];
//      if(Current && (we==Current))
//        col = sGui->Palette[sGC_SELBACK];
      if(we->Op==0 || (we->SplineName[0] && !we->Spline))
        col = 0xffff8080;
      sGui->Button(r,we->Select,we->Name,0,col);
    }
  }

  w = 0;
  if(DragMode==DM_TIME_MOVE || DragMode==DM_TIME_WIDTH)
    w = 1;
  if(DragMode==DM_TIME_DUP)
    w = 2;
  if(w)
  {
    for(i=0;i<max;i++)
    {
      we = App->Doc->Events->Get(i);
      if(we->Select)
      {
        if(MakeRectMove(we,r))
          sGui->RectHL(r,w,0xff000000,0xff000000);
      }
    }
  }

  if(DragMode == DM_TIME_RECT)
  {
    r = DragFrame;
    r.Sort();
    sGui->RectHL(r,1,~0,~0);
  }
}

void WinTimeline::OnCalcSize()
{
  SizeX = sMulShift(App->Doc->SoundEnd,BeatZoom)>>16;
  if(SizeX<BLOCKONE)
    SizeX = BLOCKONE;
  SizeY = BLOCKONE*64;
}

void WinTimeline::OnDrag(sDragData &dd)
{
  sInt x,y,z;
  sInt i,max;
  sBool ok;
  WerkEvent *we,*st;
  sRect r;
  sU32 keystate;
  sList<WerkEvent> *list;

  if(dd.Mode==sDD_START && (dd.Buttons&4) && (dd.Flags&sDDF_DOUBLE))
    OnCommand(CMD_TIME_POPUP);
  if(MMBScrolling(dd,DragStartX,DragStartY))
    return;

  LastTime = x = sDivShift((dd.MouseX-Client.x0)<<16,BeatZoom);
  LastLine = y = (dd.MouseY-Client.y0)/BLOCKONE;
  z = 0;
  keystate = sSystem->GetKeyboardShiftState();
  list = App->Doc->Events;
  max = list->GetCount();


  switch(dd.Mode)
  {
  case sDD_START:
    if(dd.Buttons&2)
    {
      DragMode = DM_TIME_ZOOM;
    }
    else
    {
      DragMode = DragKey;
      DragMoveX = DragMoveY = DragMoveZ = 0;
    }

    switch(DragMode)
    {
    case DM_TIME_ZOOM:
      DragStartX = BeatZoom;
      DragStartY = LastTime;
      DragStartZ = dd.MouseX-ScrollX-Client.x0;
      break;
    case DM_TIME_SCROLL:
      DragStartX = ScrollX;
      DragStartY = ScrollY;
      break;

    case DM_TIME_SELECT:
      ok = sFALSE;
      st = 0;
      for(i=0;i<max;i++)
      {
        we = list->Get(i);
        if(MakeRect(we,r) && r.Hit(dd.MouseX,dd.MouseY))
        {
          if(keystate & sKEYQ_CTRL)
            we->Select = !we->Select;
          else
            we->Select = 1;
          if(!(keystate & (sKEYQ_SHIFT|sKEYQ_CTRL)))
          {
            if(st)
            {
              if(st->Event.End-st->Event.Start > we->Event.End-we->Event.Start)
              {
                st->Select = 0;
                st = we;
              }
              else
                we->Select = 0;
            }
            else
            {
              st = we;
            }
          }
          ok = sTRUE;
        }
        else
        {
          if(!(keystate & (sKEYQ_SHIFT|sKEYQ_CTRL)))
            we->Select = 0;
        }
      }
      EditEvent(st);

      if(!ok)
      {
        DragMode = DM_TIME_RECT;
        DragFrame.Init(dd.MouseX,dd.MouseY,dd.MouseX,dd.MouseY);
      }
      break;
    case DM_TIME_RECT:
      DragFrame.Init(dd.MouseX,dd.MouseY,dd.MouseX,dd.MouseY);
      break;
    case DM_TIME_TIME:
      if(App->MusicIsPlaying()) App->MusicStart(2);
      break;
    }
    break;
  case sDD_DRAG:
    switch(DragMode)
    {
    case DM_TIME_ZOOM:
      BeatZoom = sRange<sInt>(DragStartX*(1.0f+(dd.DeltaX-dd.DeltaY)*0.002),0x7fff0000/sMax(App->Doc->SoundEnd>>16,0x0100),0x00010000);
      OnCalcSize();
      ScrollX = sRange<sInt>((sMulShift(DragStartY,BeatZoom)>>16)-DragStartZ,SizeX-RealX,0);
      break;
    case DM_TIME_SCROLL:      
      ScrollX = sRange<sInt>(DragStartX-dd.DeltaX,SizeX-RealX,0);
      ScrollY = sRange<sInt>(DragStartY-dd.DeltaY,SizeY-RealY,0);
      break;
    case DM_TIME_RECT:
      DragFrame.x1 = dd.MouseX;
      DragFrame.y1 = dd.MouseY;
      break;
    case DM_TIME_SELECT:
      if(sAbs(dd.DeltaX)+sAbs(dd.DeltaY)>4)
        DragMode = DM_TIME_MOVE;
      break;
    case DM_TIME_MOVE:
    case DM_TIME_DUP:
      DragMoveX = sDivShift(dd.DeltaX<<16,BeatZoom);
      DragMoveY = (dd.DeltaY+BLOCKONE*256+BLOCKONE/2)/BLOCKONE-256;
      DragMoveZ = 0;

      DragMoveX = (DragMoveX+BeatQuant/2)&~(BeatQuant-1);
      break;
    case DM_TIME_WIDTH:
      DragMoveX = sDivShift(dd.DeltaX<<16,BeatZoom);
      DragMoveY = 0;//(dd.DeltaY+BLOCKONE*256+BLOCKONE/2)/BLOCKONE-256;
      DragMoveZ = 0;

      DragMoveX = (DragMoveX+BeatQuant/2)&~(BeatQuant-1);
      break;

    case DM_TIME_TIME:
      if(x<0)
        x = 0;
      App->MusicSeek(x);
      break;
    }
    break;
  case sDD_STOP:
    switch(DragMode)
    {
    case DM_TIME_RECT:
      DragFrame.Sort();
      for(i=0;i<max;i++)
      {
        we = list->Get(i);
        if(MakeRect(we,r) && r.Hit(DragFrame) && !DragFrame.Inside(r))
        {
          if(keystate & sKEYQ_CTRL)
            we->Select = !we->Select;
          else
            we->Select = 1;
        }
        else
        {
          if(!(keystate & (sKEYQ_SHIFT|sKEYQ_CTRL)))
            we->Select = 0;
        }
      }
      break;

    case DM_TIME_MOVE:
    case DM_TIME_DUP:
      MoveSelected(DragMoveX,DragMoveY,DragMode);
      break;

    case DM_TIME_WIDTH:
      for(i=0;i<max;i++)
      {
        we = list->Get(i);
        if(we->Select)
        {
          if(we->Event.End+DragMoveX > we->Event.Start)
            we->Event.End += DragMoveX;
        }
      }
      break;
    case DM_TIME_TIME:
      if(App->MusicIsPlaying()) App->MusicStart(1);
      break;
    }
    DragMode = 0;
    App->EventWin->EventToControl();
    break;
  }
}

void WinTimeline::MoveSelected(sInt dx,sInt dy,sInt dragmode)
{
  sInt i,max;
  sBool ok;
  WerkEvent *we,*st;
  sList<WerkEvent> *list;

  list = App->Doc->Events;
  max = list->GetCount();

  ok = sTRUE;
  for(i=0;i<max;i++)
  {
    we = list->Get(i);
    if(we->Select)
    {
      if(we->Event.Start+dx<0)
        ok = sFALSE;
      if(we->Line+dy<0)
        ok = sFALSE;
    }
  }
  if(ok)
  {
    for(i=0;i<max;i++)
    {
      we = list->Get(i);
      if(we->Select)
      {
        if(dragmode==DM_TIME_MOVE)
        {
          st = we;
        }
        else 
        {
          st = new WerkEvent;
          st->Copy(we);
          list->Add(st);
        }
        st->Event.Start += dx;
        st->Event.End += dx;
        st->Line += dy;
      }
    }
  }
}

void WinTimeline::EditEvent(WerkEvent *we)
{
  App->EventWin->SetEvent(we);
}

void WinTimeline::MarkTime()
{
  WerkEvent *ev;
  sInt min = 0x40000000;
  sInt max = 0;
  sInt count = App->Doc->Events->GetCount();
  for(sInt i=0;i<count;i++)
  {
    ev = App->Doc->Events->Get(i);
    if(ev->Select)
    {
      min = sMin<sInt>(min,ev->Event.Start);
      max = sMax<sInt>(max,ev->Event.End);
    }
  }
  if(min<max)
  {
    App->MusicLoop0 = min;
    App->MusicLoop1 = max;
    App->MusicLoopTaken = 0;
  }
}


/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   event window                                                       ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

WinEvent::WinEvent()
{
  Grid = new sGridFrame;
  Grid->SetGrid(12,4,0,sPainter->GetHeight(sGui->PropFont)+2);
  Grid->AddScrolling(0,1);
  AddChild(Grid);
  Event = 0;
  Line = 0;
}

void WinEvent::Reset()
{
  Grid->RemChilds();
  Flags |= sGWF_LAYOUT;
  Event = 0;
}

void WinEvent::Tag()
{
  sDummyFrame::Tag();
  sBroker->Need(Grid);
  sBroker->Need(Event);
}

/****************************************************************************/

void WinEvent::EventToControl()
{
  if(Event)
  {
    StartCourse  =  (Event->Event.Start>>16); 
    StartFine    =  (Event->Event.Start>>8)&255;
    LengthCourse =  (Event->Event.End-Event->Event.Start)>>16; 
    LengthFine   = ((Event->Event.End-Event->Event.Start)>>8)&255;
  }
}

void WinEvent::ControlToEvent()
{
  if(Event)
  {
    Event->Event.Start = (StartCourse <<16)+(StartFine <<8);
    Event->Event.End   = (LengthCourse<<16)+(LengthFine<<8) + Event->Event.Start;
  }
}

void WinEvent::SetEvent(WerkEvent *we)
{
  sControl *con;

  Reset();
  Event = we;
  if(we)
  {
    EventToControl();

    Grid->SetGrid(12,4,0,sPainter->GetHeight(sGui->PropFont)+2);
    Line = 0;
    
    con = new sControl;
    con->EditString(CMD_EVENT_CONNECT,we->Name,0,sizeof(we->Name));
    con->DoneCmd = CMD_EVENT_CONNECT;
    Label("Name");
    Grid->Add(con,3,9,Line); 
    AddBox(CMD_EVENT_PAGES,2,0,"...");
    AddBox(CMD_EVENT_NAMES,1,0,"..");
    AddBox(CMD_EVENT_GOTO ,0,0,"->");
    Line++;

    con = new sControl;
    con->EditInt(CMD_EVENT_CHANGE,&we->Event.Mark,0);
    con->InitNum(0,16,0.125f,0);
    Label("Mark");
    Grid->Add(con,3,12,Line++);

    Label("Start");
    con = new sControl;
    con->EditInt(CMD_EVENT_UPDATEPOS,&StartCourse,0);
    con->InitNum(0,256,0.125f,0);
    Grid->Add(con,3,8,Line);
    con = new sControl;
    con->EditInt(CMD_EVENT_UPDATEPOS,&StartFine,0);
    con->InitNum(0,255,0.125f,0);
    Grid->Add(con,8,12,Line);
    Line++;

    Label("End");
    con = new sControl;
    con->EditInt(CMD_EVENT_UPDATEPOS,&LengthCourse,0);
    con->InitNum(0,256,0.125f,0);
    Grid->Add(con,3,8,Line);
    con = new sControl;
    con->EditInt(CMD_EVENT_UPDATEPOS,&LengthFine,0);
    con->InitNum(0,255,0.125f,0);
    Grid->Add(con,8,12,Line);
    Line++;

    Label("Time interval");
    con = new sControl;
    con->EditFloat(CMD_EVENT_CHANGE,&we->Event.StartInterval,0);
    con->InitNum(-1024,1024,0.001f,0);
    Grid->Add(con,3,8,Line);
    con = new sControl;
    con->EditFloat(CMD_EVENT_CHANGE,&we->Event.EndInterval,0);
    con->InitNum(-1024,1024,0.001f,0);
    Grid->Add(con,8,12,Line++);

    Space();

    con = new sControl;
    con->EditFloat(CMD_EVENT_CHANGE,&we->Event.Velocity,0);
    con->InitNum(-1,1,0.001f,0);
    Label("Velocity");
    Grid->Add(con,3,12,Line++);

    con = new sControl;
    con->EditFloat(CMD_EVENT_CHANGE,&we->Event.Modulation,0);
    con->InitNum(-1,1,0.001f,0);
    Label("Modulation");
    Grid->Add(con,3,12,Line++);

    con = new sControl;
    con->EditInt(CMD_EVENT_CHANGE,&we->Event.Select,0);
    con->InitNum(0,16,0.125f,0);
    Label("Select");
    Grid->Add(con,3,12,Line++);

    con = new sControl;
    con->EditFloat(CMD_EVENT_CHANGE,&we->Event.Scale.x,0);
    con->InitNum(-256,256,0.01f,0);
    con->Zones = 3;
    con->Style |= sCS_ZONES;
    Label("Scale");
    Grid->Add(con,3,12,Line++);

    con = new sControl;
    con->EditFloat(CMD_EVENT_CHANGE,&we->Event.Rotate.x,0);
    con->InitNum(-64,64,0.002f,0);
    con->Zones = 3;
    con->Style |= sCS_ZONES;
    Label("Rotate");
    Grid->Add(con,3,12,Line++);

    con = new sControl;
    con->EditFloat(CMD_EVENT_CHANGE,&we->Event.Translate.x,0);
    con->InitNum(-256,256,0.001f,0);
    con->Zones = 3;
    con->Style |= sCS_ZONES;
    Label("Translate");
    Grid->Add(con,3,12,Line++);

    con = new sControl;
    con->EditURGBA(CMD_EVENT_CHANGE,(sInt *)&we->Event.Color,0);
    Label("Color");
    Grid->Add(con,3,12,Line++);

    con = new sControl;
    con->EditString(CMD_EVENT_CONNECT,we->SplineName,0,KK_NAME);
    Label("Spline");
    Grid->Add(con,3,12,Line++);
  }
  Grid->Flags |= sGWF_LAYOUT;
  App->SetMode(MODE_EVENT);
}


sBool WinEvent::OnCommand(sU32 cmd)
{
  WerkPage *page;
  WerkOp *op;
  sMenuFrame *mf;
  sInt i,j;
  sInt val;


  page = 0;
  val = cmd&0xff;
  switch(cmd)
  {
  case CMD_EVENT_CHANGE:
    CurrentPage = 0;
    break;

  case CMD_EVENT_CONNECT:
    CurrentPage = 0;
    if(Event)
    {
      Event->Event.Op = 0;
      Event->Event.Spline = 0;
      Event->Op = App->Doc->FindName(Event->Name);
      Event->Spline = App->Doc->FindSpline(Event->SplineName);
      if(Event->Op)
        Event->Event.Op = &Event->Op->Op;
      if(Event->Spline)
        Event->Event.Spline = &Event->Spline->Spline;
    }
    break;
  case CMD_EVENT_PAGES:
    CurrentPage = 0;
    mf = new sMenuFrame;
    mf->SendTo = this;
    mf->AddBorder(new sNiceBorder);
    for(i=0;i<App->Doc->Pages->GetCount() && i<=0xff;i++)
      mf->AddMenu(App->Doc->Pages->Get(i)->Name,CMD_EVENT_PAGE+i,0);
    sGui->AddPulldown(mf);
    break;

  case CMD_EVENT_NAMES:
    CurrentPage = 0;
    page = App->PageWin->Page;
    break;

  case CMD_EVENT_GOTO:
    CurrentPage = 0;
    op = App->Doc->FindName(Event->Name);
    if(op)
    {
      App->PageWin->GotoOp(op);
      App->SetMode(MODE_PAGE);
    }
    break;

  case CMD_EVENT_UPDATEPOS:
    ControlToEvent();
    break;

  default:
    if((cmd&0xff00)==CMD_EVENT_PAGE)
    {
      CurrentPage = 0;
      if(val>=0 && val<App->Doc->Pages->GetCount())
        page = App->Doc->Pages->Get(val);
    }
    else if((cmd&0xff00)==CMD_EVENT_OP)
    {
      page = CurrentPage;
      CurrentPage = 0;
      if(page && val>=0 && val<page->Ops->GetCount())
      {
        j = 1;
        if(val==0)
        {
          Event->Name[0] = 0;
        }
        else
        {
          for(i=0;i<page->Ops->GetCount() && j<=0xff;i++)
          {
            op = page->Ops->Get(i);
            if(op->Name[0])
            {
              if(j==val)
              {
                sCopyString(Event->Name,op->Name,KK_NAME);
                sGui->Post(CMD_EVENT_CONNECT,this);
                break;
              }
              j++;
            }
          }
        }
      }
      page = 0;
      break;
    }
    else
    {
      return sFALSE;
    }
  }

  if(page)
  {
    CurrentPage = page;
    mf = new sMenuFrame;
    mf->SendTo = this;
    mf->AddBorder(new sNiceBorder);
    j = 1;
    mf->AddMenu("<< none >>",CMD_EVENT_OP,0);
    for(i=0;i<page->Ops->GetCount() && j<=0xff;i++)
    {
      op = page->Ops->Get(i);
      if(op->Name[0])
      {
        mf->AddMenu(op->Name,CMD_EVENT_OP+j,0);
        j++;
      }
    }
    sGui->AddPulldown(mf);
  }

  return sTRUE;
}

void WinEvent::Label(sChar *label)
{
  sControl *con;
  con = new sControl;
  con->Label(label);
  con->LayoutInfo.Init(0,Line,3,Line+1);
  con->Style |= sCS_LEFT;
  Grid->AddChild(con);
}

void WinEvent::AddBox(sU32 cmd,sInt pos,sInt offset,sChar *name)
{
  sControl *con;
  con = new sControl;
  con->Button(name,cmd|offset);
  con->LayoutInfo.Init(11-pos,Line,12-pos,Line+1);
  Grid->AddChild(con);
}


// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#if !sPLAYER

#include "winnova.hpp"
#include "winnovaview.hpp"
#include "winpage.hpp"
#include "winpara.hpp"
#include "winspline.hpp"
#include "novaplayer.hpp"

#define BLOCKONE 20
#define MAXLINES 64

#define DM_TIME_TIME       1
#define DM_TIME_ZOOM       2
#define DM_TIME_SCROLL     3
#define DM_TIME_ADD        4
#define DM_TIME_SELECT     5
#define DM_TIME_MOVE       6
#define DM_TIME_RECT       7
#define DM_TIME_WIDTH      8
#define DM_TIME_DUP        9

#define CMD_TIME_CUT              0x0100
#define CMD_TIME_COPY             0x0101
#define CMD_TIME_PASTE            0x0102
#define CMD_TIME_DELETE           0x0103
#define CMD_TIME_POPUP            0x0104
#define CMD_TIME_ADD              0x0105
#define CMD_TIME_QUANTPOP         0x0106
#define CMD_TIME_QUANT            0x0107
#define CMD_TIME_SHOW             0x0108
#define CMD_TIME_LOOP             0x0109

#define CMD_TIME_QUANTSET         0x01c0

#define CMD_TIME_ADDS             0x1100  // add effect + n


/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Timeline                                                           ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

WinTimeline2::WinTimeline2()
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
  Clipboard = new sList<WerkEvent2>;

  AddScrolling();
}

WinTimeline2::~WinTimeline2()
{
}

void WinTimeline2::Tag()
{
  sGuiWindow::Tag();
  sBroker->Need(App);
  sBroker->Need(Clipboard);
}


sBool WinTimeline2::MakeRect(sInt line,sInt start,sInt ende,sRect &r)
{
  r.x0 = Client.x0 + (sMulShift(start,BeatZoom)>>16);
  r.x1 = Client.x0 + (sMulShift(ende ,BeatZoom)>>16);
  r.y0 = Client.y0 +  line   *BLOCKONE;
  r.y1 = Client.y0 + (line+1)*BLOCKONE;
  return sTRUE;
}

sBool WinTimeline2::MakeRect(WerkEvent2 *we,sRect &r)
{
  return MakeRect(we->Line,we->Event.Start,we->Event.End,r);
/*
  r.x0 = Client.x0 + (sMulShift(we->Event.Start,BeatZoom)>>16);
  r.x1 = Client.x0 + (sMulShift(we->Event.End  ,BeatZoom)>>16);
  r.y0 = Client.y0 +  we->Line   *BLOCKONE;
  r.y1 = Client.y0 + (we->Line+1)*BLOCKONE;
  return sTRUE;
*/
}

sBool WinTimeline2::MakeRectMove(WerkEvent2 *we,sRect &r)
{
  sInt m;

  m = 1;
  if(DragMode == DM_TIME_WIDTH)
    m = 0;

  return MakeRect(we->Line+DragMoveY*m,we->Event.Start+DragMoveX*m,we->Event.End+DragMoveX,r);
/*
  r.x0 = Client.x0 + (sMulShift((we->Event.Start+DragMoveX*m),BeatZoom)>>16);
  r.x1 = Client.x0 + (sMulShift((we->Event.End  +DragMoveX  ),BeatZoom)>>16);
  r.y0 = Client.y0 + (we->Line+  DragMoveY*m)*BLOCKONE;
  r.y1 = Client.y0 + (we->Line+1+DragMoveY*m)*BLOCKONE;
  return sTRUE;
  */
}

void WinTimeline2::ClipCut()
{
  ClipCopy();
  ClipDelete();
  ClipDeselect();
}

void WinTimeline2::ClipDeselect()
{
  sInt i,max;

  WerkEvent2 *ev;

  max = App->Doc2->Events->GetCount();
  for(i=0;i<max;i++)
  {
    ev = App->Doc2->Events->Get(i);
    ev->Select = sFALSE;
  }
}

void WinTimeline2::ClipCopy()
{
  sInt i,max;

  WerkEvent2 *ev,*copy;
  Clipboard->Clear();

  max = App->Doc2->Events->GetCount();
  for(i=0;i<max;i++)
  {
    ev = App->Doc2->Events->Get(i);
    if(ev->Select)
    {
      copy = new WerkEvent2;
      copy->Copy(ev);
      Clipboard->Add(copy);
    }
  }
}

void WinTimeline2::ClipPaste()
{
  sInt i,max;
  sInt minline,mintime;
  WerkEvent2 *ev,*copy;;
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
    copy = new WerkEvent2;
    copy->Copy(ev);
    copy->Line = sMax(0,ev->Line - minline + line);
    copy->Event.Start = sMax(0,ev->Event.Start - mintime + time);
    copy->Event.End = copy->Event.Start + ev->Event.End - ev->Event.Start;
    copy->Select = sTRUE;
    App->Doc2->Events->Add(copy);
  }
}

void WinTimeline2::ClipDelete()
{
  sInt i;
  WerkEvent2 *we;

  for(i=0;i<App->Doc2->Events->GetCount();)
  {
    we = App->Doc2->Events->Get(i);
    if(we->Select)
      App->Doc2->Events->Rem(we);
    else
      i++;
  }
}

void WinTimeline2::Quant()
{
  sInt i,max;
  WerkEvent2 *we;
  sList<WerkEvent2> *list;

  list = App->Doc2->Events;

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

void WinTimeline2::OnKey(sU32 key)
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
  case 'm':
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
  case 'm'|sKEYQ_BREAK:
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

  case 's':
    OnCommand(CMD_TIME_SHOW);
    break;
  case 'l':
    OnCommand(CMD_TIME_LOOP);
    break;
  }
}

sBool WinTimeline2::OnCommand(sU32 cmd)
{
  sMenuFrame *mf;
  sInt x;
  WerkEvent2 *we;
  sInt i;

  switch(cmd)
  {
  case CMD_TIME_POPUP:
    mf = new sMenuFrame;
    mf->AddMenu("Add",CMD_TIME_ADD,'a');
    mf->AddSpacer();
    mf->AddMenu("Normal Mode",' ',' ');
    mf->AddMenu("Scratch Time",'t','t');
    mf->AddMenu("Zoom",'z','z');
    mf->AddMenu("Scroll",'y','y');
    mf->AddMenu("Move",'m','m');
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
    mf->AddSpacer();
    mf->AddMenu("Show",CMD_TIME_SHOW,'s');
    mf->AddMenu("Loop event",CMD_TIME_LOOP,'l');
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
    mf = new sMenuFrame;
    for(i=0;EffectList[i].Name;i++)
      mf->AddMenu(EffectList[i].Name,CMD_TIME_ADDS+i,0);
    mf->AddBorder(new sNiceBorder);
    mf->SendTo = this;
    sGui->AddPopup(mf);
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
  case CMD_TIME_SHOW:
    App->Doc2->Build();
    App->ViewNovaWin->SetTimeline();
    return sTRUE;
  case CMD_TIME_LOOP:
    if(App->EventPara2Win->Event)
    {
      App->EventPara2Win->Event->LoopFlag = !App->EventPara2Win->Event->LoopFlag;
      App->Doc2->SortEvents();
    }
    return sTRUE;

  default:
    if((cmd&0xff00)==CMD_TIME_ADDS)
    {
      we = App->Doc2->MakeEvent(&EffectList[cmd&0xff]);
      x=(LastTime+BeatQuant/2)&~(BeatQuant-1);
      we->Event.Start = x;
      we->Event.End = x+0x00100000;
      we->Line = LastLine;
      we->Select = 1;
      we->Name[0] = 0;
      if(LastLine+1<MAXLINES)
        LastLine++;

/*
      if(App->ParaWin->Op && App->ParaWin->Op->Name[0])
        sCopyString(we->Name,App->ParaWin->Op->Name,sizeof(we->Name));
      else
        sCopyString(we->Name,"new",sizeof(we->Name));
*/
      App->Doc2->Recompile = 1;
      App->Doc2->Events->Add(we);
      App->Doc2->SortEvents();
      App->EventPara2Win->SetEvent(we);
  //    App->EventWin->OnCommand(CMD_EVENT_CONNECT);
      return sTRUE;

    }
    return sFALSE;
  }
}

void WinTimeline2::OnPaint()
{
  sInt i,x,max,w,skip,beat;
  sRect r;
  sChar buffer[128];
  WerkEvent2 *we;
  sU32 col,col1;
  const sChar *name;

/*
  sU32 kinds[2][4] =
  {
    { 0xff000000,0xffffffc0,0xffffc0ff,0xffc0ffff },
    { 0xff000000,0xffc0c000,0xffc000c0,0xff00c0c0 }
  };
*/

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

  max = App->Doc2->Events->GetCount();

  for(i=0;i<max;i++)
  {
    we = App->Doc2->Events->Get(i);
    if(MakeRect(we,r))
    {
      sChar buffer[64];

      col = we->Event.Effect->ButtonColor;
      if(col==0)
        col = sGui->Palette[sGC_BUTTON];
      if(we->CheckError())
        col = 0xffff8080;
      name = we->Event.Effect->Name;
      if(we->Name[0]) 
        name = we->Name;
      sSPrintF(buffer,sizeof(buffer),"%s (%d)",name,i);
      sGui->Button(r,we->Select,buffer,0,col);
    }
    if(we->LoopFlag)
    {     
      sInt t0 = we->Event.Start;
      sInt t1 = we->Event.End;
      sInt tx = we->Event.LoopEnd;
      for(sInt loop=1;t0+(t1-t0)*loop<tx;loop++)
      {
        if(MakeRect(we->Line,t0+(t1-t0)*loop,sMin(tx,t1+(t0+t1)*loop),r))
        {
          sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BACK]);
          sGui->RectHL(r,1,sGui->Palette[sGC_DRAW],sGui->Palette[sGC_DRAW]);
        }
      }
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
      we = App->Doc2->Events->Get(i);
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

void WinTimeline2::OnCalcSize()
{
  SizeX = sMulShift(App->Doc->SoundEnd,BeatZoom)>>16;
  if(SizeX<BLOCKONE)
    SizeX = BLOCKONE;
  SizeY = BLOCKONE*MAXLINES;
}

void WinTimeline2::OnDrag(sDragData &dd)
{
  sInt x,y,z;
  sInt i,max;
  sBool ok;
  sInt mode;
  WerkEvent2 *we,*st;
  sRect r;
  sU32 keystate;
  sList<WerkEvent2> *list;

  if(dd.Mode==sDD_START && (dd.Buttons&4) && (dd.Flags&sDDF_DOUBLE))
    OnCommand(CMD_TIME_POPUP);
  if(MMBScrolling(dd,DragStartX,DragStartY))
    return;

  LastTime = x = sDivShift((dd.MouseX-Client.x0)<<16,BeatZoom);
  LastLine = y = (dd.MouseY-Client.y0)/BLOCKONE;
  z = 0;
  keystate = sSystem->GetKeyboardShiftState();
  list = App->Doc2->Events;
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

      mode = 0;
      for(i=0;i<max;i++)
      {
        we = list->Get(i);
        if(MakeRect(we,r) && r.Hit(dd.MouseX,dd.MouseY))
        {
          EditEvent(we);
          if(we->Select)
            mode = 1;
          break;
        }
      }
      if(keystate & sKEYQ_SHIFT)        // add selection
        mode = 1;
      if(keystate & sKEYQ_CTRL)         // toggle selection
        mode = 2;

      for(i=0;i<max;i++)
      {
        we = list->Get(i);
        if(MakeRect(we,r) && r.Hit(dd.MouseX,dd.MouseY))
        {
          if(mode==2)
            we->Select = !we->Select;
          else
            we->Select = 1;
          if(mode==0)
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
          if(mode==0)
            we->Select = 0;
        }
      }

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
      ok = sTRUE;
      for(i=0;i<max;i++)
      {
        we = list->Get(i);
        if(we->Select)
        {
          if(we->Event.Start+DragMoveX<0)
            ok = sFALSE;
          if(we->Event.End+DragMoveY<0)
            ok = sFALSE;
        }
      }
      if(!ok) break;
      for(i=0;i<max;i++)
      {
        we = list->Get(i);
        if(we->Select)
        {
          if(DragMode==DM_TIME_MOVE)
          {
            st = we;
          }
          else 
          {
            st = new WerkEvent2;
            st->Copy(we);
            list->Add(st);
            st->UpdateLinks(App->Doc2,App->Doc);
          }
          st->Event.Start += DragMoveX;
          st->Event.End += DragMoveX;
          st->Line += DragMoveY;
        }
      }
      App->Doc2->SortEvents();
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
    App->EventPara2Win->EventToControl();
    break;
  }
}

void WinTimeline2::EditEvent(WerkEvent2 *we)
{
  App->EventPara2Win->SetEvent(we);
}

/****************************************************************************/
/****************************************************************************/

#define CMD_EVENT2_TIMECHANGE     0x0100
#define CMD_EVENT2_CHANGE         0x0101
#define CMD_EVENT2_RELAYOUT       0x0102
#define CMD_EVENT2_LINK           0x0103
#define CMD_EVENT2_GOTOALIAS      0x0104

#define CMD_EVENT2_GOTOLINK       0x1000  // 0..MAXLINK(8)
#define CMD_EVENT2_ANIMTYPE       0x1100

#define CMD_EVENT2_ADDANIM        0x1200
#define CMD_EVENT2_EDITANIM       0x1300
#define CMD_EVENT2_DELANIM        0x1400

#define CMD_EVENT2_SCENEADDANIM   0x1500
#define CMD_EVENT2_SCENEEDITANIM  0x1600
#define CMD_EVENT2_SCENEDELANIM   0x1700

WinEventPara2::WinEventPara2()
{
  Event = 0;
  App = 0;
  Grid = 0;
  GroupName = 0;

  Grid = new sGridFrame;
  Grid->SetGrid(12,4,0,sPainter->GetHeight(sGui->PropFont)+2);
  Grid->AddScrolling(0,1);
  AddChild(Grid);
  AddAnim_Id = 0;
  AddAnim_Type = 0;
  AddAnim_Ptr = 0;
  AddAnim_SceneName = 0;
}

WinEventPara2::~WinEventPara2()
{
}

void WinEventPara2::Tag()
{
  sDummyFrame::Tag();
  sBroker->Need(Event);
}

void WinEventPara2::Label(const sChar *name)
{
  Grid->AddLabel(name,0,3,Line);
}

void WinEventPara2::Group(const sChar *name)
{
  GroupName = name;
}

void WinEventPara2::HandleGroup()
{
  if(GroupName)
  {
    Grid->Add(new sMenuSpacerControl(GroupName),0,12,Line++);
    GroupName = 0;
  }
}
/*
WerkEvent2 *WinEventPara2::MakeEvent(sInt id)
{
  WerkEvent2 *ev;

  ev = new WerkEvent2();
  ev->Event.Init(&EffectList[id]);
  ev->Doc = App->Doc2;
  sCopyMem(ev->Event.Para,ev->Event.Effect->Info->DefaultPara,ev->Event.Effect->ParaSize);
  sCopyMem(ev->Event.Edit,ev->Event.Effect->Info->DefaultEdit,ev->Event.Effect->EditSize);

  return ev;
}
*/
/****************************************************************************/

void WinEventPara2::EventToControl()
{
  if(Event)
  {
    StartCourse  =  (Event->Event.Start>>16); 
    StartFine    =  (Event->Event.Start>>8)&255;
    LengthCourse =  (Event->Event.End-Event->Event.Start)>>16; 
    LengthFine   = ((Event->Event.End-Event->Event.Start)>>8)&255;
  }
}

void WinEventPara2::ControlToEvent()
{
  if(Event)
  {
    Event->Event.Start = (StartCourse <<16)+(StartFine <<8);
    Event->Event.End   = (LengthCourse<<16)+(LengthFine<<8) + Event->Event.Start;
  }
}

/****************************************************************************/

sBool WinEventPara2::OnCommand(sU32 cmd)
{
  KEffect2ParaInfo2 *para;
  WerkSceneNode2 *node;
  WerkScene2 *scene;
  WerkEvent2Anim *anim;
  sInt id;


  switch(cmd)
  {
  case CMD_EVENT2_TIMECHANGE:
    ControlToEvent();
    return sTRUE;
  case CMD_EVENT2_RELAYOUT:
    SetEvent(Event);
    return sTRUE;
  case CMD_EVENT2_LINK:
    if(Event)
      Event->UpdateLinks(App->Doc2,App->Doc);
    return sTRUE;
  case CMD_EVENT2_GOTOALIAS:
    if(Event->AliasEvent)
      SetEvent(Event->AliasEvent);
    return sTRUE;

  default:
    id = cmd&0x00ff;
    cmd = cmd&0xff00;
    scene = 0;
    node = 0;
    para = 0;
    if(Event && id<Event->Event.Effect->Info->ParaInfo.Count)
    {
      para = &Event->Event.Effect->Info->ParaInfo[id];
    }

    if(Event && Event->LinkScenes[0])
    {
      scene = Event->LinkScenes[0];
      if(id<scene->Nodes->GetCount())
        node = scene->Nodes->Get(id);
    }

    switch(cmd)
    {
    case CMD_EVENT2_GOTOLINK:
      if(Event && id<K2MAX_LINK)
      {
        if(Event->LinkScenes[id])
        {
          App->SceneListWin->SetScene(Event->LinkScenes[id]);
          App->SetMode(MODE_SCENE);
        }
        if(Event->LinkOps[id])
        {
          App->PageWin->GotoOp(Event->LinkOps[id]);
          App->SetMode(MODE_PAGE);
        }
      }
      return sTRUE;

    case CMD_EVENT2_ANIMTYPE:
      if(Event && id<32)
      {
        Event->AddAnim(AddAnim_Id,1<<(id),AddAnim_Ptr,AddAnim_SceneName);
        App->Doc2->UpdateLinks(App->Doc);
        App->Doc2->Build();
        SetEvent(Event);

        AddAnim_SceneName = 0;
        AddAnim_Type = 0;
        AddAnim_Id = 0;
        AddAnim_Ptr = 0;
      }
      return sTRUE;

    case CMD_EVENT2_ADDANIM:
      if(para)
      {
        AddAnimGui(para->AnimType,para->Offset,para->Type,Event->Event.Para+para->Offset,AddAnim_SceneName);
      }
      return sTRUE;

    case CMD_EVENT2_EDITANIM:
      if(para)
      {
        WerkSpline *spline;

        anim = Event->FindAnim(para->Offset);
        if(anim)
        {
          sFORLIST(Event->Splines,spline)
            spline->Select(0);
          for(sInt i=0;i<4;i++)
            if(anim->Splines[i])
              anim->Splines[i]->Select(1);
          App->SetMode(MODE_SPLINE);
          App->SplineParaWin->SetSpline(anim->Splines[0]);
          App->SplineListWin->SetSpline(anim->Splines[0]);
          sGui->SetFocus(App->SplineWin);
        }
      }
      return sTRUE;

    case CMD_EVENT2_DELANIM:
      if(para)
      {
        anim = Event->FindAnim(para->Offset);
        if(anim)
          Event->Anims->Rem(anim);
        Event->Recompile = 1;
        App->Doc2->Build();
        SetEvent(Event);
      }
      return sTRUE;      

    case CMD_EVENT2_SCENEADDANIM:
      if(node)
      {
        AddAnimGui(EAT_EULER|EAT_SRT|EAT_TARGET,id*16,sST_MATRIX,&node->Node->Matrix.i.x,node->Name);
      }
      return sTRUE;      
    case CMD_EVENT2_SCENEEDITANIM:
      if(node)
      {
        WerkSpline *spline;

        anim = Event->FindAnim(node->Name);
        if(anim)
        {
          sFORLIST(Event->Splines,spline)
            spline->Select(0);
          for(sInt i=0;i<4;i++)
            if(anim->Splines[i])
              anim->Splines[i]->Select(1);
          App->SetMode(MODE_SPLINE);
          App->SplineParaWin->SetSpline(anim->Splines[0]);
          App->SplineListWin->SetSpline(anim->Splines[0]);
          sGui->SetFocus(App->SplineWin);
        }
      }
      return sTRUE;      
    case CMD_EVENT2_SCENEDELANIM:
      if(node)
      {
        anim = Event->FindAnim(node->Name);
        if(anim)
          Event->Anims->Rem(anim);
        Event->Recompile = 1;
        App->Doc2->Build();
        SetEvent(Event);
      }
      return sTRUE;      
    }
    break;
  }
  return sFALSE;
}

void WinEventPara2::SetEvent(WerkEvent2 *we)
{
  sControl *con;
  sControlTemplate temp;
  KEffect2ParaInfo2 *info;
  KEffectInfo *ei;
  sInt i,j;

  Event = we;
  Grid->RemChilds();
    
  App->EventScript2Win->SetEvent(we);

  if(Event)
  {
    Line = 0;
    GroupName = 0;
    EventToControl();

    Group(we->Event.Effect?we->Event.Effect->Name:"???");
    HandleGroup();

    Grid->AddLabel("Name",0,3,Line);
    con = Grid->AddCon(3,Line,8,1);
    con->EditString(0,Event->Name,0,KK_NAME);
    Line++;

    Grid->AddLabel("Start",0,3,Line);
    con = Grid->AddCon(3,Line,4,1,1);
    con->EditInt(CMD_EVENT2_TIMECHANGE,&StartCourse,0);
    con->InitNum(0,1024,0.125f,1);
    con = Grid->AddCon(7,Line,4,1,1);
    con->EditInt(CMD_EVENT2_TIMECHANGE,&StartFine,0);
    con->InitNum(0,1024,4,1);
    Line++;

    Grid->AddLabel("Length",0,3,Line);
    con = Grid->AddCon(3,Line,4,1,1);
    con->EditInt(CMD_EVENT2_TIMECHANGE,&LengthCourse,0);
    con->InitNum(0,1024,0.125f,1);
    con = Grid->AddCon(7,Line,4,1,1);
    con->EditInt(CMD_EVENT2_TIMECHANGE,&LengthFine,0);
    con->InitNum(0,1024,4,1);
    Line++;

    Group("parameter");
    for(i=0;i<Event->Event.Effect->Info->ParaInfo.Count;i++)
    {
      ei = Event->Event.Effect->Info;
      info = &ei->ParaInfo[i];
      if(info->Type==EPT_GROUP)
      {
        Group(info->Name);
      }
      else
      {
        HandleGroup();
        if(info->AnimType)
        {
          con = Grid->AddCon(11,Line,1,1,0);
          if(Event->FindAnim(info->Offset))
          {
            con->Button("Edit",CMD_EVENT2_EDITANIM+i);
            con->RightCmd = CMD_EVENT2_DELANIM+i;
          }
          else
            con->Button("Anim",CMD_EVENT2_ADDANIM+i);
        }
        switch(info->Type)
        {
        case EPT_INT:
          con = new sControl;
          con->EditInt(CMD_EVENT2_CHANGE,(sInt *)(&Event->Event.Para[info->Offset]),0);
          con->InitNum(info->Min,info->Max,info->Step,0);
          con->Zones = info->Count;
          for(j=0;j<info->Count;j++)
            ((sS32*)con->Default)[j] = ((sS32*)ei->DefaultPara)[j+info->Offset];
          con->Style|=sCS_ZONES;
          con->LayoutInfo.Init(3,Line,11,Line+1);
          Grid->AddChild(con);
          Label(info->Name);
          Line++;
          break;
        case EPT_FLOAT:
          con = new sControl;
          con->EditFloat(CMD_EVENT2_CHANGE,(sF32 *)(&Event->Event.Para[info->Offset]),0);
          con->InitNum(info->Min,info->Max,info->Step,0);
          con->Zones = info->Count;
          for(j=0;j<info->Count;j++)
            con->Default[j] = ei->DefaultPara[j+info->Offset];
          con->Style|=sCS_ZONES;
          con->LayoutInfo.Init(3,Line,11,Line+1);
          Grid->AddChild(con);
          Label(info->Name);
          Line++;
          break;
        case EPT_CHOICE:
          con = new sControl;
          con->EditChoice(CMD_EVENT2_CHANGE,(sInt *)(&Event->Event.Para[info->Offset]),0,info->Choice);
          con->LayoutInfo.Init(3,Line,11,Line+1);
          con->Default[0] = ei->DefaultPara[info->Offset];
          Grid->AddChild(con);
          Label(info->Name);
          Line++;
          break;
        case EPT_FLAGS:
          temp.Init();
          temp.Type = sCT_CYCLE;
          temp.Cycle = info->Choice;
          temp.YPos = Line;
          temp.XPos = 3-2;
          temp.AddFlags(Grid,CMD_EVENT2_CHANGE,(sInt *)(&Event->Event.Para[info->Offset]),0);
          Label(info->Name);
          Line++;
          break;
        case EPT_MATRIX:
          {
            sMatrixEditor *mae;
            sMatrixEdit *me = (sMatrixEdit *)&Event->Event.Edit[info->EditOffset];
            sMatrix *mat = (sMatrix *)&Event->Event.Para[info->Offset];
            sInt lines;

            mae = new sMatrixEditor;
            lines = mae->Init(me,mat,info->Name,CMD_EVENT2_RELAYOUT);
            Grid->Add(mae,0,Line,11,Line+lines);
            Line+=lines;
          }
          break;
        case EPT_COLOR:
          con = new sControl;
          con->EditFRGBA(CMD_EVENT2_CHANGE,(sF32 *)(&Event->Event.Para[info->Offset]),0);
          con->LayoutInfo.Init(3,Line,11,Line+1);
          for(j=0;j<4;j++)
            con->Default[j] = ei->DefaultPara[j+info->Offset];
          Grid->AddChild(con);
          Label(info->Name);
          Line++;
          break;
        case EPT_LINK:
          sVERIFY(info->Offset>=0 && info->Offset<K2MAX_LINK);  

          con = new sControl;
          con->EditString(CMD_EVENT2_LINK,Event->LinkName[info->Offset],0,KK_NAME);
          con->LayoutInfo.Init(3,Line,11,Line+1);
          con->Style |= sCS_ZONES;
          Grid->AddChild(con);
          con = Grid->AddCon(11,Line,1,1,0);
          con->Button("-->",CMD_EVENT2_GOTOLINK);
          Label(info->Name);
          Line++;
          if(Event->LinkScenes[0])
          {
            WerkSceneNode2 *node;
            Group("scene nodes");
            sInt nodeid = 0;
            sFORLIST(Event->LinkScenes[0]->Nodes,node)
            {
              if(node->Name[0])
              {
                sMatrixEditor *mae;
                sInt lines;

                HandleGroup();

                con = Grid->AddCon(11,Line,1,1,0);
                if(Event->FindAnim(node->Name))
                {
                  con->Button("Edit",CMD_EVENT2_SCENEEDITANIM+nodeid);
                  con->RightCmd = CMD_EVENT2_SCENEDELANIM+nodeid;
                }
                else
                {
                  con->Button("Anim",CMD_EVENT2_SCENEADDANIM+nodeid);
                }
                mae = new sMatrixEditor;
                lines = mae->Init(&node->MatrixEdit,&node->Node->Matrix,node->Name,CMD_EVENT2_RELAYOUT);
                Grid->Add(mae,0,Line,11,Line+lines);
                Line+=lines;                
              }
              nodeid++;
            }
          }
          break;
        case EPT_ALIAS:
          con = new sControl;
          con->EditString(CMD_EVENT2_LINK,Event->AliasName,0,KK_NAME);
          con->LayoutInfo.Init(3,Line,11,Line+1);
          con->Style |= sCS_ZONES;
          Grid->AddChild(con);
          con = Grid->AddCon(11,Line,1,1,0);
          con->Button("-->",CMD_EVENT2_GOTOALIAS);
          Label(info->Name);
          Line++;
          break;
        }
      }
    }
  }

  Grid->Flags |= sGWF_LAYOUT;
}


void WinEventPara2::AddAnimGui(sInt animtype,sInt id,sInt scripttype,const sF32 *data,const sChar *name)
{
  sMenuFrame *mf;
  sInt count = 0;
  sInt type = 0;

  for(sInt i=0;i<32;i++)
  {
    if(animtype & (1<<i))
    {
      count++;
      type = 1<<i;
    }
  }
  if(count==1)
  {
    Event->AddAnim(id,type,data,name);
    App->Doc2->UpdateLinks(App->Doc);
    App->Doc2->Build();
    SetEvent(Event);
  }
  if(count>1)
  {
    static const sChar *names[] = { "scalar","vector","euler","srt","target",0 };
    mf = new sMenuFrame;
    mf->SendTo = this;
    mf->AddBorder(new sNiceBorder);
    AddAnim_Type = type;
    AddAnim_Id = id;
    AddAnim_Ptr = data;
    AddAnim_SceneName = name;
    for(sInt i=0;names[i];i++)
      if(animtype & (1<<i))
        mf->AddMenu(names[i],CMD_EVENT2_ANIMTYPE+i,0);
    sGui->AddPopup(mf);
  }
}

/****************************************************************************/
/****************************************************************************/

#define CMD_SCRIPT2_MENU    0x0100
#define CMD_SCRIPT2_SOURCE  0x0101
#define CMD_SCRIPT2_ERRORS  0x0102
#define CMD_SCRIPT2_OUTPUT  0x0103
#define CMD_SCRIPT2_RUN     0x0104
#define CMD_SCRIPT2_COMPILE 0x0105
#define CMD_SCRIPT2_CHANGE  0x0106
#define CMD_SCRIPT2_DISASS  0x0107
#define CMD_SCRIPT2_SYMBOLS 0x0108

#define ES2_MODE_OFF        0
#define ES2_MODE_SOURCE     1
#define ES2_MODE_ERRORS     2
#define ES2_MODE_OUTPUT     3
#define ES2_MODE_DISASS     4

WinEventScript2::WinEventScript2()
{
  Event = 0;
  Text = 0;
  DummyScript = 0;
  Output = 0;
  Disassembly = 0;
  RecompileDelay = 0;


  Text = new sTextControl;
  Text->AddScrolling(0,1);
  AddChild(Text);
  Text->MenuCmd = CMD_SCRIPT2_MENU;
  Text->ChangeCmd = CMD_SCRIPT2_CHANGE;

  DummyScript = new sText("// no script here");
  Output = new sText("no output available");
  Disassembly = new sText("no disassembly available");
  SetMode(ES2_MODE_OFF);
}

WinEventScript2::~WinEventScript2()
{
}

void WinEventScript2::Tag()
{
  sDummyFrame::Tag();
  sBroker->Need(DummyScript);
  sBroker->Need(Text);
  sBroker->Need(Output);
  sBroker->Need(Disassembly);
  sBroker->Need(Event);
} 

sBool WinEventScript2::OnCommand(sU32 cmd)
{
  sMenuFrame *mf;
  switch(cmd)
  {
  case CMD_SCRIPT2_MENU:
    mf = new sMenuFrame;
    mf->SendTo = this;
    mf->AddBorder(new sNiceBorder);
    if(Event==0)
    {
      mf->AddCheck("Output",CMD_SCRIPT2_OUTPUT,0,Mode==ES2_MODE_OUTPUT);
      mf->AddMenu("Compile All",CMD_SCRIPT2_COMPILE,0);
    }
    else
    {
      mf->AddCheck("Source",CMD_SCRIPT2_SOURCE,0,Mode==ES2_MODE_SOURCE);
      mf->AddCheck("Errors",CMD_SCRIPT2_ERRORS,0,Mode==ES2_MODE_ERRORS);
      mf->AddCheck("Output",CMD_SCRIPT2_OUTPUT,0,Mode==ES2_MODE_OUTPUT);
      mf->AddSpacer();
      mf->AddMenu("Compile All",CMD_SCRIPT2_COMPILE,0);
      mf->AddMenu("Run",CMD_SCRIPT2_RUN,0);
      mf->AddMenu("Disassemble",CMD_SCRIPT2_DISASS,0);
      mf->AddMenu("Symbols",CMD_SCRIPT2_SYMBOLS,0);
    }
    sGui->AddPopup(mf);
    return sTRUE;

  case CMD_SCRIPT2_CHANGE:
    RecompileDelay = 1;
    return sTRUE;

  case CMD_SCRIPT2_SOURCE:
    SetMode(ES2_MODE_SOURCE);
    return sTRUE;

  case CMD_SCRIPT2_ERRORS:
    SetMode(ES2_MODE_ERRORS);
    return sTRUE;

  case CMD_SCRIPT2_OUTPUT:
    SetMode(ES2_MODE_OUTPUT);
    return sTRUE;

  case CMD_SCRIPT2_COMPILE:
    App->Doc2->CompileEvents();
    if(Event)
      SetMode(ES2_MODE_ERRORS);
    return sTRUE;

  case CMD_SCRIPT2_DISASS:
    App->Doc2->Build();
    if(Event && Event->Event.Code.CodeCount)
    {
      App->Doc2->Compiler->Disassemble(&Event->Event.Code,Disassembly,1);
      SetMode(ES2_MODE_DISASS);
    }
    return sTRUE;

  case CMD_SCRIPT2_SYMBOLS:
    App->Doc2->Build();
    if(Event && Event->Event.Code.CodeCount)
    {
      ListSymbols();
      SetMode(ES2_MODE_DISASS);
    }
    return sTRUE;

  case CMD_SCRIPT2_RUN:
    if(Event)
    {
      App->Doc2->Build();
      if(Event->Event.Code.ErrorCount)
      {
        SetMode(ES2_MODE_ERRORS);
      }
      else
      {
        App->Doc2->VM->InitFrame();
        App->Doc2->VM->InitEvent(&Event->Event);
        App->Doc2->VM->Execute(&Event->Event.Code);
        Output->Init(App->Doc2->VM->GetOutput());
        SetMode(ES2_MODE_OUTPUT);
      }
    }
    return sTRUE;
  }
  return sFALSE;
}

void WinEventScript2::OnFrame()
{
  if(RecompileDelay && sGui->GetFocus()!=Text && Event)
  {
    Event->Recompile = 1;
    RecompileDelay = 0;
  }
}

void WinEventScript2::SetMode(sInt mode)
{
  Mode = mode;
  Text->SetText(DummyScript);
  Text->Static = 1;
  switch(Mode)
  {
  case ES2_MODE_SOURCE:
    if(Event)
      Text->SetText(Event->Source);
    break;
  case ES2_MODE_OUTPUT:
    Text->SetText(Output);
    Text->Static = 1;
    break;
  case ES2_MODE_ERRORS:
    if(Event)
      Text->SetText(Event->CompilerErrors);
    break;
  case ES2_MODE_DISASS:
    Text->SetText(Disassembly);
    Text->Static = 1;
    break;
  }
}

void WinEventScript2::SetEvent(WerkEvent2 *event)
{
  if(RecompileDelay && Event)
    Event->Recompile = 1;
  Event = event;
  RecompileDelay = 0;
  SetMode(ES2_MODE_SOURCE);
}

void WinEventScript2::ListSymbols()
{
  static const sChar *types[] = { "null","scalar","vector","matrix","object","string","code","error" };
  KEffect2Symbol *sym;
  sInt used;
  if(Event)
  {
    Disassembly->Init("// Symbol List:\n\n");
    for(sInt i=0;i<Event->Exports.Count;i++)
    {
      sym = &Event->Exports[i];
      used = Disassembly->Used;
      Disassembly->PrintF("%s %s;",types[sym->Type],sym->Name);
      while(Disassembly->Used<used+35)
        Disassembly->Print(" ");
      Disassembly->PrintF("// offset 0x%08x\n",sym->Id);
    }
    Disassembly->Print("\n// end of symbols\n");
  }
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Matrix Editor                                                      ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

sMatrixEditor::sMatrixEditor()
{
  Grid = 0;
  MatEd = 0;
  Mat = 0;
  RelayoutCmd = 0;
  Name = 0;
}

sMatrixEditor::~sMatrixEditor()
{
}

sInt sMatrixEditor::Init(sMatrixEdit *ed,sMatrix *mat,const sChar *name,sInt relayoutcmd)
{
  sInt lines;

  sVERIFY(Grid==0);

  MatEd = ed;
  Mat = mat;
  RelayoutCmd = relayoutcmd;
  Name = name;

  Grid = new sGridFrame;
  Grid->SetGrid(11,8,0,sPainter->GetHeight(sGui->PropFont)+2);
  AddChild(Grid);

  ReadMatrix();
  lines = AddControl();
  Grid->SetGrid(11,lines,0,sPainter->GetHeight(sGui->PropFont)+2);

  return lines;
}

/****************************************************************************/

void sMatrixEditor::ReadMatrix()
{
  sMatrix mat;

  if(MatEd->EditMode==0)
    return;

  mat = *Mat;
  MatEd->Scale[0] = mat.i.UnitAbs3();
  MatEd->Scale[1] = mat.j.UnitAbs3();
  MatEd->Scale[2] = mat.k.UnitAbs3();
  MatEd->Pos[0] = mat.l.x;
  MatEd->Pos[1] = mat.l.y;
  MatEd->Pos[2] = mat.l.z;
  
  switch(MatEd->EditMode)
  {
  case 0:   // pure matrix
    break;
  case 1:   // srt
    mat.FindEuler(MatEd->Rotate[0],MatEd->Rotate[1],MatEd->Rotate[2]);
    MatEd->Rotate[0] /= sPI2F;
    MatEd->Rotate[1] /= sPI2F;
    MatEd->Rotate[2] /= sPI2F;
    break;
  case 2:   // dir vector
    MatEd->Target[0] = mat.l.x+mat.k.x;
    MatEd->Target[1] = mat.l.y+mat.k.y;
    MatEd->Target[2] = mat.l.z+mat.k.z;
    break;
  case 3:   // angle vector
    MatEd->Rotate[0] = MatEd->Rotate[1] = MatEd->Rotate[2] = 0;
    break;
  }
}

void sMatrixEditor::WriteMatrix()
{
  sVector t,d;

  t.Init(MatEd->Pos[0],MatEd->Pos[1],MatEd->Pos[2],1);
  switch(MatEd->EditMode)
  {
  case 0:   // pure matrix
    break;
  case 1:   // srt
    Mat->InitEulerPI2(MatEd->Rotate);
    break;
  case 2:   // dir vector
    d.Init(MatEd->Target[0]-t.x,MatEd->Target[1]-t.y,MatEd->Target[2]-t.z,0);
    d.Unit3();
    Mat->InitDir(d);
    break;
  case 3:   // angle vector
    d.Init(MatEd->Target[0],MatEd->Target[1],MatEd->Target[2],0);
    d.Unit3();
    d.Scale3(MatEd->Tilt);
    Mat->InitRot(d);
    break;
  }
  if(MatEd->EditMode!=0)
  {
    Mat->i.Scale3(MatEd->Scale[0]);
    Mat->j.Scale3(MatEd->Scale[1]);
    Mat->k.Scale3(MatEd->Scale[2]);
  }
  Mat->l = t;
}

sInt sMatrixEditor::AddControl()
{
  sInt line;
  sControl *con;

  sVERIFY(Grid);
  Grid->RemChilds();
  line = 0;

  Grid->AddLabel(Name,0,3,line);
  con = new sControl;
  con->EditChoice(1,&MatEd->EditMode,0,"matrix|srt|target|axis");
  Grid->Add(con,3,line,8,1);
  line++;

  switch(MatEd->EditMode)
  {
  case 0:   // matrix
    Grid->AddLabel("   i",0,3,line);
    con = new sControl;
    con->EditFloat(3,&Mat->i.x,0);
    con->Zones = 3;
    con->InitNum(-64,64,0.001f,0);
    con->Default[0] = 1;
    Grid->Add(con,3,line,8,1);
    line++;

    Grid->AddLabel("   j",0,3,line);
    con = new sControl;
    con->EditFloat(3,&Mat->j.x,0);
    con->Zones = 3;
    con->InitNum(-64,64,0.001f,0);
    con->Default[1] = 1;
    Grid->Add(con,3,line,8,1);
    line++;

    Grid->AddLabel("   k",0,3,line);
    con = new sControl;
    con->EditFloat(3,&Mat->k.x,0);
    con->Zones = 3;
    con->InitNum(-64,64,0.001f,0);
    con->Default[2] = 1;
    Grid->Add(con,3,line,8,1);
    line++;

    break;
  case 1:   // srt
    Grid->AddLabel("   Rotate",0,3,line);
    con = new sControl;
    con->EditFloat(2,&MatEd->Rotate[0],0);
    con->Zones = 3;
    con->InitNum(-64,64,0.001f,0);
    Grid->Add(con,3,line,8,1);
    line++;

    Grid->AddLabel("   Scale",0,3,line);
    con = new sControl;
    con->EditFloat(2,&MatEd->Scale[0],0);
    con->Zones = 3;
    con->InitNum(-64,64,0.001f,1);
    Grid->Add(con,3,line,8,1);
    line++;
    break;
  case 2:
    Grid->AddLabel("   Target",0,3,line);
    con = new sControl;
    con->EditFloat(2,&MatEd->Target[0],0);
    con->Zones = 3;
    con->InitNum(-1024,1024,0.125f,0);
    Grid->Add(con,3,line,8,1);
    line++;
    break;
  case 3:
    Grid->AddLabel("   Axis",0,3,line);
    con = new sControl;
    con->EditFloat(2,&MatEd->Target[0],0);
    con->Zones = 3;
    con->InitNum(-64,64,0.001f,0);
    Grid->Add(con,3,line,8,1);
    line++;
    Grid->AddLabel("   Angle",0,3,line);
    con = new sControl;
    con->EditFloat(2,&MatEd->Tilt,0);
    con->InitNum(-64,64,0.001f,0);
    Grid->Add(con,3,line,11,1);
    line++;
    break;
  }
  Grid->AddLabel("   Translate",0,3,line);
  con = new sControl;
  con->EditFloat(2,MatEd->Pos,0);
  con->Zones = 3;
  con->InitNum(-1024,1024,0.125f,0);
  Grid->Add(con,3,line,8,1);
  line++;

  return line;
}

sBool sMatrixEditor::OnCommand(sU32 cmd)
{
  switch(cmd)
  {
  case 1:                   // change that requires relayout
    Send(RelayoutCmd);
    return sTRUE;
  case 2:                   // change that requires matrix update
    WriteMatrix();
    return sTRUE;
  case 3:                   // change that requires nothing to be done
    return sTRUE;
  }
  return sFALSE;
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   All the scene editing stuff                                        ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#define CMD_SCENELIST_SHOW       0x0101

WinSceneList::WinSceneList()
{
  sToolBorder *tb;

  DocList = new sListControl;
  DocList->LeftCmd = 1;
  DocList->MenuCmd = 2;
  DocList->TreeCmd = 3;
  DocList->DoubleCmd = CMD_SCENELIST_SHOW;
  DocList->Style = sLCS_TREE|sLCS_HANDLING|sLCS_DRAG;

  DocList->AddScrolling(0,1);
  DocList->TabStops[0] = 0;
  DocList->TabStops[1] = 11;
  DocList->TabStops[2] = 22;
  DocList->TabStops[3] = 36+0*80;

  AddChild(DocList);

  tb = new sToolBorder();
  tb->AddMenu("Scenes",0);
  AddBorder(tb);
}

WinSceneList::~WinSceneList()
{
}

void WinSceneList::Tag()
{
  sGuiWindow::Tag();
  sBroker->Need(DocList);
}

void WinSceneList::OnCalcSize()
{
  if(Childs)
  {
    SizeX = Childs->SizeX;
    SizeY = Childs->SizeY;
  }
}

void WinSceneList::OnLayout()
{
  if(Childs)
  {
    Childs->Position = Client;
  }
}

void WinSceneList::OnPaint()
{
}

void WinSceneList::SetScene(WerkScene2 *scene)
{
  WerkScene2 *s;
  sInt i;

  i = 0;
  sFORLIST(App->Doc2->Scenes,s)
  {
    if(s==scene)
    {
      DocList->SetSelect(i,1);
      App->SceneNodeWin->SetScene(scene);
    }
    i++;
  }
}

sBool WinSceneList::OnShortcut(sU32 key)
{
  switch(key&0x8001ffff)
  {
  case 's':
    OnCommand(CMD_SCENELIST_SHOW);
    return sTRUE;
  }
  return sFALSE;
}

/****************************************************************************/

sBool WinSceneList::OnCommand(sU32 cmd)
{
  WerkScene2 *scene;
  sMenuFrame *mf;
  sInt nr;
  sInt i;

  nr = DocList->GetSelect();
  if(nr>=0 && nr<App->Doc2->Scenes->GetCount())
    scene = App->Doc2->Scenes->Get(nr);
  else
    scene = 0;

  switch(cmd)
  {
  case 1:
    if(scene)
    {
      DocList->SetSelect(nr,1);
    }
    App->SceneNodeWin->SetScene(scene);
    return sTRUE;
  case 2:
    mf = new sMenuFrame;
    mf->SendTo = DocList;
    mf->AddBorder(new sNiceBorder);
    mf->AddMenu("add",sCMD_LIST_ADD,'a');
    mf->AddMenu("delete",sCMD_LIST_DELETE,sKEY_BACKSPACE);
    mf->AddMenu("rename",sCMD_LIST_RENAME,'r');
    mf->AddSpacer();
    mf->AddMenu("move up",sCMD_LIST_MOVEUP,sKEY_UP|sKEYQ_CTRL);
    mf->AddMenu("move down",sCMD_LIST_MOVEDOWN,sKEY_DOWN|sKEYQ_CTRL);
    sGui->AddPopup(mf);
    return sTRUE;
  case 3:
    for(i=0;i<DocList->GetCount() && i<App->Doc2->Scenes->GetCount();i++)
    {
      scene = App->Doc2->Scenes->Get(i);
      DocList->GetTree(i,scene->TreeLevel,scene->TreeButton);
    }
    return sTRUE;


  case sLCS_CMD_RENAME:
    scene = App->Doc2->Scenes->Get(DocList->HandleIndex);
    sVERIFY(scene);
    DocList->SetRename(scene->Name,KK_NAME);
    return sTRUE;
  case sLCS_CMD_DEL:
    scene = App->Doc2->Scenes->Get(DocList->HandleIndex);
    sVERIFY(scene);
    App->Doc2->Scenes->RemOrder(scene);
    return sTRUE;
  case sLCS_CMD_ADD:
    scene = new WerkScene2;
    sCopyString(scene->Name,DocList->NameBuffer,KK_NAME);
    App->Doc2->Scenes->AddPos(scene,DocList->HandleIndex);
    return sTRUE;
  case sLCS_CMD_SWAP:
    App->Doc2->Scenes->Swap(DocList->HandleIndex);
    return sTRUE;
  case sLCS_CMD_UPDATE:
    UpdateList();
    return sTRUE;
  case sLCS_CMD_EXCHANGE:
    scene = App->Doc2->Scenes->Get(DocList->HandleIndex);
    App->Doc2->Scenes->RemOrder(scene);
    App->Doc2->Scenes->AddPos(scene,DocList->InsertIndex);
    UpdateList();
    return sTRUE;
  case sLCS_CMD_DUP:
    scene = new WerkScene2;
    scene->Copy(App->Doc2->Scenes->Get(DocList->HandleIndex));
    App->Doc2->Scenes->AddPos(scene,DocList->InsertIndex);
    App->Doc2->UpdateLinks(App->Doc);
    UpdateList();
    return sTRUE;

  case CMD_SCENELIST_SHOW:
    if(scene)
    {
      App->Doc2->CompileEvents();
      App->ViewNovaWin->SetScene(scene,-1);
      UpdateList();
      App->SceneNodeWin->UpdateList();
    }
    return sTRUE;
  }
  return sFALSE;
}


void WinSceneList::UpdateList()
{
  sInt i,max;
  WerkScene2 *scene;
  sChar *listname;
  sInt oldindex;

  Flags |= sGWF_LAYOUT;

  oldindex = DocList->GetSelect();
  DocList->ClearList();
  max = App->Doc2->Scenes->GetCount();
  for(i=0;i<max;i++)
  {
    scene = App->Doc2->Scenes->Get(i);

    sSPrintF(scene->ListName,sizeof(scene->ListName),"*%s",scene->Name);
    listname = scene->ListName;
    if(!(App->ViewNovaWin->Scene==scene && App->ViewNovaWin->SceneIndex==-1))
      listname++;

    DocList->Add(listname);
    DocList->SetTree(DocList->GetCount()-1,scene->TreeLevel,scene->TreeButton);
  }
  DocList->CalcLevel();
  if(oldindex>=0 && oldindex<DocList->GetCount())
    DocList->SetSelect(oldindex,1);
}


/****************************************************************************/
/****************************************************************************/

#define CMD_SCENENODE_SHOW      0x0101


WinSceneNode::WinSceneNode()
{
  sToolBorder *tb;

  DocList = new sListControl;
  DocList->LeftCmd = 1;
  DocList->MenuCmd = 2;
  DocList->TreeCmd = 3;
  DocList->DoubleCmd = CMD_SCENENODE_SHOW;
  DocList->Style = sLCS_TREE|sLCS_HANDLING|sLCS_DRAG|sLCS_MULTISELECT;

  DocList->AddScrolling(0,1);
  DocList->TabStops[0] = 0;
  DocList->TabStops[1] = 11;
  DocList->TabStops[2] = 22;
  DocList->TabStops[3] = 36+0*80;

  AddChild(DocList);

  tb = new sToolBorder();
  tb->AddMenu("Nodes",0);
  AddBorder(tb);

  Scene = 0;
}

WinSceneNode::~WinSceneNode()
{
}

void WinSceneNode::Tag()
{
  sGuiWindow::Tag();
  sBroker->Need(DocList);
}

void WinSceneNode::OnCalcSize()
{
  if(Childs)
  {
    SizeX = Childs->SizeX;
    SizeY = Childs->SizeY;
  }
}

void WinSceneNode::OnLayout()
{
  if(Childs)
  {
    Childs->Position = Client;
  }
}

void WinSceneNode::OnPaint()
{
}

sBool WinSceneNode::OnShortcut(sU32 key)
{
  switch(key&0x8001ffff)
  {
  case 's':
    OnCommand(CMD_SCENENODE_SHOW);
    return sTRUE;
  }
  return sFALSE;
}

/****************************************************************************/

void WinSceneNode::SetScene(WerkScene2 *scene)
{
  Scene = scene;
  UpdateList();
}

sBool WinSceneNode::OnCommand(sU32 cmd)
{
  WerkSceneNode2 *node;
  sMenuFrame *mf;
  sInt nr;
  sInt i;

  nr = DocList->GetSelect();
  if(Scene && nr>=0 && nr<Scene->Nodes->GetCount())
    node = Scene->Nodes->Get(nr);
  else
    node = 0;

  switch(cmd)
  {
  case 1: // click
    if(node)
      DocList->SetSelect(nr,!node->Selected);
    App->SceneParaWin->SetNode(Scene,node);
    if(Scene)
    {
      for(i=0;i<DocList->GetCount() && i<Scene->Nodes->GetCount();i++)
      {
        node = Scene->Nodes->Get(i);
        node->Selected = DocList->GetSelect(i);
      }
    }
    return sTRUE;
  case 2: // menu
    mf = new sMenuFrame;
    mf->SendTo = DocList;
    mf->AddBorder(new sNiceBorder);
    mf->AddMenu("add",sCMD_LIST_ADD,'a');
    mf->AddMenu("delete",sCMD_LIST_DELETE,sKEY_BACKSPACE);
    mf->AddMenu("rename",sCMD_LIST_RENAME,'r');
    mf->AddSpacer();
    mf->AddMenu("move up",sCMD_LIST_MOVEUP,sKEY_UP|sKEYQ_CTRL);
    mf->AddMenu("move down",sCMD_LIST_MOVEDOWN,sKEY_DOWN|sKEYQ_CTRL);
    sGui->AddPopup(mf);
    return sTRUE;
  case 3: // tree change (hirarchy)
    if(Scene)
    {
      for(i=0;i<DocList->GetCount() && i<Scene->Nodes->GetCount();i++)
      {
        node = Scene->Nodes->Get(i);
        DocList->GetTree(i,node->TreeLevel,node->TreeButton);
      }
    }
    Scene->UpdateLinks();
    return sTRUE;

  case sLCS_CMD_RENAME:
    sVERIFY(Scene);
    node = Scene->Nodes->Get(DocList->HandleIndex);
    sVERIFY(node);
    DocList->SetRename(node->Name,KK_NAME);
    Scene->UpdateLinks();
    Scene->Recompile = 1;
    return sTRUE;
  case sLCS_CMD_DEL:
    sVERIFY(Scene);
    node = Scene->Nodes->Get(DocList->HandleIndex);
    sVERIFY(node);
    Scene->Nodes->RemOrder(node);
    Scene->UpdateLinks();
    Scene->Recompile = 1;
    return sTRUE;
  case sLCS_CMD_ADD:
    sVERIFY(Scene);
    node = new WerkSceneNode2;
    if(App->ParaWin->Op && App->ParaWin->Op->Name[0])
      sCopyString(node->MeshName,App->ParaWin->Op->Name,KK_NAME);
    node->UpdateLinks(App->Doc);
    Scene->Nodes->AddPos(node,DocList->HandleIndex);
    App->SceneParaWin->SetNode(Scene,node);
    Scene->UpdateLinks();
    Scene->Recompile = 1;
    return sTRUE;
  case sLCS_CMD_SWAP:
    sVERIFY(Scene);
    Scene->Nodes->Swap(DocList->HandleIndex);
    Scene->UpdateLinks();
    Scene->Recompile = 1;
    return sTRUE;
  case sLCS_CMD_UPDATE:
    UpdateList();
    return sTRUE;
  case sLCS_CMD_EXCHANGE:
    node = Scene->Nodes->Get(DocList->HandleIndex);
    Scene->Nodes->RemOrder(node);
    Scene->Nodes->AddPos(node,DocList->InsertIndex);    
    App->Doc2->UpdateLinks(App->Doc);
    UpdateList();
    return sTRUE;
  case sLCS_CMD_DUP:
    node = new WerkSceneNode2;
    node->Copy(Scene->Nodes->Get(DocList->HandleIndex));
    Scene->Nodes->AddPos(node,DocList->InsertIndex);    
    App->Doc2->UpdateLinks(App->Doc);
    UpdateList();
    return sTRUE;

  case CMD_SCENENODE_SHOW:
    if(Scene)
    {
      App->Doc2->Build();
      App->ViewNovaWin->SetScene(Scene,DocList->GetSelect());
      UpdateList();
      App->SceneListWin->UpdateList();
    }
    return sTRUE;
  }
  return sFALSE;
}


void WinSceneNode::UpdateList()
{
  sInt i,max;
  WerkSceneNode2 *node;
  sU32 col;
  sChar *listname;
//  sInt oldindex;

  Flags |= sGWF_LAYOUT;

//  oldindex = DocList->GetSelect();
  DocList->ClearList();
  if(Scene)
  {
    max = Scene->Nodes->GetCount();
    for(i=0;i<max;i++)
    {
      node = Scene->Nodes->Get(i);

      if(node->Name[0])
        sSPrintF(node->ListName,sizeof(node->ListName),"*%s",node->Name);
      else if(node->MeshName[0])
        sSPrintF(node->ListName,sizeof(node->ListName),"*(%s)",node->MeshName);
      else
        sCopyString(node->ListName,"*[noname]",sizeof(node->ListName));
      col = 0;
      if(node->MeshName[0] && !node->MeshOp)
        sAppendString(node->ListName," [error]",sizeof(node->ListName));

      listname = node->ListName+1;
      if(App->ViewNovaWin->Scene==Scene && App->ViewNovaWin->SceneIndex==i)
        listname--;
      DocList->Add(listname,-1,col);
      DocList->SetTree(DocList->GetCount()-1,node->TreeLevel,node->TreeButton);
      if(node->Selected)
        DocList->SetSelect(i,node->Selected);
    }
  }
  DocList->CalcLevel();
//  if(oldindex>=0 && oldindex<DocList->GetCount())
//    DocList->SetSelect(oldindex,1);
}

/****************************************************************************/
/****************************************************************************/

WinSceneAnim::WinSceneAnim()
{
}

WinSceneAnim::~WinSceneAnim()
{
}

void WinSceneAnim::OnPaint()
{
  ClearBack();
}

/****************************************************************************/
/****************************************************************************/

#define CMD_SCENEPARA_UPDATENAME    0x0101
#define CMD_SCENEPARA_SHOW          0x0102
#define CMD_SCENEPARA_UPDATEMESH    0x0103

WinScenePara::WinScenePara()
{
  Grid = new sGridFrame;
  Grid->SetGrid(12,4,0,sPainter->GetHeight(sGui->PropFont)+2);
  AddChild(Grid);
  Scene = 0;
  Node = 0;
}

WinScenePara::~WinScenePara()
{
}

void WinScenePara::Tag()
{
  sGuiWindow::Tag();
  sBroker->Need(Scene);
}

/****************************************************************************/

sBool WinScenePara::OnCommand(sU32 cmd)
{
  switch(cmd)
  {
  case CMD_SCENEPARA_UPDATEMESH:
    if(Node)
      Node->UpdateLinks(App->Doc);
    App->SceneNodeWin->UpdateList();
    return sTRUE;
  case CMD_SCENEPARA_UPDATENAME:
    App->SceneNodeWin->UpdateList();
    if(Scene)
      Scene->Recompile = 1;
    return sTRUE;
  }
  return sFALSE;
}


void WinScenePara::SetNode(WerkScene2 *scene,WerkSceneNode2 *node)
{
  sMatrixEditor *me;
  sControl *con;
  sInt line,i;
  
  Node = node;
  Scene = Scene;
  Grid->RemChilds();
  if(Node)
  {
    line = 0;
    Grid->AddLabel("Node Name",0,3,line);
    con = new sControl;
    con->EditString(CMD_SCENEPARA_UPDATENAME,Node->Name,0,KK_NAME);
    Grid->Add(con,3,line,8,1);
    line++;

    Grid->AddLabel("Mesh Name",0,3,line);
    con = new sControl;
    con->EditString(CMD_SCENEPARA_UPDATEMESH,Node->MeshName,0,KK_NAME);
    Grid->Add(con,3,line,8,1);
    line++;

    me = new sMatrixEditor();
    i = me->Init(&Node->MatrixEdit,&Node->Node->Matrix,"Matrix",0);
    Grid->Add(me,0,line,11,i);
    line += i;
  }

  Grid->Flags |= sGWF_LAYOUT;
}

/****************************************************************************/
/****************************************************************************/

#endif

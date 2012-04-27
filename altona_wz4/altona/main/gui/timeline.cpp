/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "gui/timeline.hpp"
#include "base/sound.hpp"

#define QUANT 0xffff

/****************************************************************************/
/****************************************************************************/

sTimerTimeline::sTimerTimeline()
{
  BeatTime = 0;
  BeatEnd = 0x80000;
  LoopStart = 0;
  LoopEnd = 0;
  Speed = 0x10000;
  LoopEnable = 0;
  Playing = 0;
  Scratching = 0;

  LastTime = sGetTime();
  Time = 0;
}

sTimerTimeline::~sTimerTimeline()
{
}

void sTimerTimeline::OnFrame()
{
  sInt t = sGetTime();
  if(Playing || Scratching)
  {
    sInt oldtime = Time;
    if(!Scratching)
      Time += t-LastTime;

    if(LoopEnable)
    {
      sVERIFY(LoopStart <= LoopEnd);
      sInt loopStartMs = sMulDiv(LoopStart,1000,Speed);
      sInt loopEndMs = sMulDiv(LoopEnd,1000,Speed);

      sBool inloop0 = oldtime>=loopStartMs && oldtime<loopEndMs;
      sBool inloop1 = Time>=loopStartMs && Time<loopEndMs;

      if(inloop0 && !inloop1)
      {
        sInt loopLen = loopEndMs-loopStartMs;
        if(!loopLen)
          Time = loopStartMs;
        else
        {
          if(Time > loopStartMs)
            Time = (Time - loopStartMs) % loopLen + loopStartMs;
        }
      }
    }

    sInt newbeat = sMin(BeatEnd-1,sMulDiv(Time,Speed,1000));
    if(newbeat!=BeatTime)
    {
      BeatTime = newbeat;
      sGui->Notify(BeatTime);
    }
  }
  LastTime = t;
}

void sTimerTimeline::AddNotify(sWindow *win)
{
  win->AddNotify(BeatTime);
  win->AddNotify(BeatEnd);
  win->AddNotify(LoopStart);
  win->AddNotify(LoopEnd);
  win->AddNotify(LoopEnable);
}

void sTimerTimeline::SetTimeline(sInt end,sInt speed)
{
  sVERIFY(end>0);
  sVERIFY(speed>0);

  BeatEnd = end;
  Speed = speed;
  sGui->Notify(BeatEnd);
  sGui->Notify(Speed);
}

void sTimerTimeline::SetLoop(sInt start,sInt end)
{
  sVERIFY(start<=end);
  LoopStart = start;
  LoopEnd = end;
  sGui->Notify(LoopStart);
  sGui->Notify(LoopEnd);
}

sInt sTimerTimeline::GetEnd()
{
  return BeatEnd;
}

sInt sTimerTimeline::GetBeat()
{
  return BeatTime;
}

sInt sTimerTimeline::GetSpeed()
{
  return Speed;
}

void sTimerTimeline::GetLoop(sInt &start,sInt &end)
{
  start = LoopStart;
  end = LoopEnd;
}

sBool sTimerTimeline::GetPlaying()
{
  return Playing;
}

sBool sTimerTimeline::GetLooping()
{
  return LoopEnable;
}

sBool sTimerTimeline::GetScratching()
{
  return Scratching;
}

void sTimerTimeline::EnableLoop(sBool loop)
{
  OnFrame();
  if(LoopEnable!=loop)
    sGui->Notify(LoopEnable);
  LoopEnable = loop;
  OnFrame();
}

void sTimerTimeline::EnablePlaying(sBool play)
{
  if(Playing!=play)
    sGui->Notify(Playing);
  Playing = play;
}

void sTimerTimeline::EnableScratching(sBool scratch)
{
  if(Scratching!=scratch)
    sGui->Notify(Scratching);
  Scratching = scratch;
}

void sTimerTimeline::SeekBeat(sInt beat)
{
  Time = sClamp(sMulDiv(beat,1000,Speed),0,BeatEnd-1);
  OnFrame();
}

/****************************************************************************/
/****************************************************************************/

static sMusicTimeline *sMusicTimelinePlayer;
void sMusicTimelineSoundHandler(sS16 *samples,sInt count)
{
  if(sMusicTimelinePlayer)
    sMusicTimelinePlayer->SoundHandler(samples,count);
}


sMusicTimeline::sMusicTimeline(sMusicPlayer *music)
{
  BeatTime = 0;
  BeatEnd = 0x80000;
  LoopStart = 0;
  LoopEnd = 0;
  Speed = 0x10000;
  LoopEnable = 0;
  Playing = 0;
  Scratching = 0;
  Music = music;
  sVERIFY(sMusicTimelinePlayer==0);
  sMusicTimelinePlayer = this;
  TotalSamples = 0;
  MusicSamples = 0;
  sSetSoundHandler(44100,sMusicTimelineSoundHandler,44100/5);

  LastTime = sGetCurrentSample();
  Time = 0;
}

sMusicTimeline::~sMusicTimeline()
{
  if(this==sMusicTimelinePlayer)
  {
    sClearSoundHandler();
    sMusicTimelinePlayer = 0;
  }
}

void sMusicTimeline::OnFrame()
{
  if(Playing || Scratching)
  {
//    sInt oldtime = Time;
    if(!Scratching)
      Time = sGetCurrentSample() + MusicSamples - TotalSamples;
/*
    if(LoopEnable)
    {
      sBool inloop0 = oldtime>=LoopStart && oldtime<LoopEnd;
      sBool inloop1 = Time>=LoopStart && Time<LoopEnd;
      if(inloop0 && !inloop1)
      {
        sVERIFY(LoopStart<LoopEnd);
        while(Time>=LoopEnd)
          Time -= LoopEnd-LoopStart;
      }
    }
*/
    sInt newbeat = sMin(BeatEnd-1,sMulDiv(Time,Speed,44100));
    if(newbeat!=BeatTime)
    {
      BeatTime = newbeat;
      sGui->Notify(BeatTime);
    }
  }
}

void sMusicTimeline::AddNotify(sWindow *win)
{
  win->AddNotify(BeatTime);
  win->AddNotify(BeatEnd);
  win->AddNotify(LoopStart);
  win->AddNotify(LoopEnd);
  win->AddNotify(LoopEnable);
}

void sMusicTimeline::SetTimeline(sInt end,sInt speed)
{
  sVERIFY(end>0);
  sVERIFY(speed>0);

  BeatEnd = end;
  Speed = speed;
  sGui->Notify(BeatEnd);
  sGui->Notify(Speed);
}

void sMusicTimeline::SetLoop(sInt start,sInt end)
{
  sVERIFY(start<=end);
  LoopStart = start;
  LoopEnd = end;
  sGui->Notify(LoopStart);
  sGui->Notify(LoopEnd);
}

sInt sMusicTimeline::GetEnd()
{
  return BeatEnd;
}

sInt sMusicTimeline::GetBeat()
{
  sInt beat;
  if(Scratching || !Playing)
    beat = sMulDiv(MusicSamples,Speed,44100);
  else
    beat = sMulDiv(MusicSamples+sGetCurrentSample()-TotalSamples,Speed,44100);
  return sClamp(beat,0,BeatEnd-1);
}

sInt sMusicTimeline::GetSpeed()
{
  return Speed;
}

void sMusicTimeline::GetLoop(sInt &start,sInt &end)
{
  start = LoopStart;
  end = LoopEnd;
}

sBool sMusicTimeline::GetPlaying()
{
  return Playing;
}

sBool sMusicTimeline::GetLooping()
{
  return LoopEnable;
}

sBool sMusicTimeline::GetScratching()
{
  return Scratching;
}

void sMusicTimeline::EnableLoop(sBool loop)
{
  OnFrame();
  if(LoopEnable!=loop)
    sGui->Notify(LoopEnable);
  LoopEnable = loop;
  OnFrame();
}

void sMusicTimeline::EnablePlaying(sBool play)
{
  if(Playing!=play)
    sGui->Notify(Playing);
  Playing = play;
}

void sMusicTimeline::EnableScratching(sBool scratch)
{
  if(Scratching!=scratch)
    sGui->Notify(Scratching);
  Scratching = scratch;
}

void sMusicTimeline::SeekBeat(sInt beat)
{
  MusicSamples = Time = sClamp(sMulDiv(beat,44100,Speed),0,BeatEnd-1);
  OnFrame();
}

void sMusicTimeline::SoundHandler(sS16 *samples,sInt count)
{
  if(Music)
  {
    if(Playing)
    {
      Music->PlayPos = MusicSamples;
      Music->Handler(samples,count,0x100);
      if(!Scratching)
        MusicSamples += count;
    }
    else
    {
      sSetMem(samples,0,count*sizeof(sS16)*2);
    }
  }
  TotalSamples+=count;
}

/****************************************************************************/
/****************************************************************************/

sTimeTableClip::sTimeTableClip()
{
  Name = L"<<< new >>>";
  Start = 0;
  Length = 0x1000;
  Line = 0;
  Selected = 0;
  DragStartX = 0;
  DragStartY = 0;
}

sTimeTableClip::sTimeTableClip(sInt start,sInt len,sInt line,const sChar *name)
{
  Name =name;
  Start = start;
  Length = len;
  Line = line;
  Selected = 0;
  DragStartX = 0;
  DragStartY = 0;
}

void sTimeTableClip::AddNotify(sWindow *win)
{
  win->AddNotify(Start);
  win->AddNotify(Line);
  win->AddNotify(Length);
}

sTimetable::sTimetable()
{
  MaxLines = 16;
  LinesUsed = 1;
}

void sTimetable::Tag()
{
  sNeed(Clips);
}

void sTimetable::Sort()
{
  sSortUp(Clips,&sTimeTableClip::Start);
  sSortUp(Clips,&sTimeTableClip::Line);
  if(Clips.GetCount()>=1)
    LinesUsed = Clips[Clips.GetCount()-1]->Line+1;
  else
    LinesUsed = 1;
}

sTimeTableClip *sTimetable::NewEntry()
{
  return new sTimeTableClip;
}

sTimeTableClip *sTimetable::Duplicate(sTimeTableClip *source)
{
  sTimeTableClip *dest = NewEntry();

  dest->Name = source->Name;
  dest->Start = source->Start;
  dest->Length = source->Length;
  dest->Line = source->Line;
  dest->Selected = 0;  

  return dest;
}

/****************************************************************************/
/****************************************************************************/

static sInt AlignUpToTimebase(sInt value,sBool decimal)
{
  sInt v = 1;

  if(decimal)
  {
    for(;;) // doesn't have to be exact powers of 10, *2 or *5 is nice too
    {
      if(v*1 >= value) return v*1;
      if(v*2 >= value) return v*2;
      if(v*5 >= value) return v*5;

      if(v*10 < v) // overflow?
        return value;

      v *= 10;
    }
  }
  else // binary is easier
  {
    for(;;)
    {
      if(v >= value) return v;
      if(v*2 < v)    return value; // overflow

     v *= 2;
    }
  }
}

sWinTimeline::sWinTimeline(sTimelineInterface *tl)
{
  Height = 20 + sGui->PropFont->GetHeight();

  Timeline = tl;
  Timeline->AddNotify(this);

  DecimalTimebase = sFALSE;
}

void sWinTimeline::InitWire(const sChar *name)
{
  sWireClientWindow::InitWire(name);
  sWire->AddKey(name,L"Start",sMessage(this,&sWinTimeline::CmdStart));
  sWire->AddKey(name,L"Pause",sMessage(this,&sWinTimeline::CmdPause));
  sWire->AddKey(name,L"Loop",sMessage(this,&sWinTimeline::CmdLoop));
  sWire->AddDrag(name,L"Scratch",sMessage(this,&sWinTimeline::DragScratch));
  sWire->AddDrag(name,L"Mark",sMessage(this,&sWinTimeline::DragMark));
}

void sWinTimeline::OnCalcSize()
{
  ReqSizeY = Height;
}
 
void sWinTimeline::OnLayout()
{
  Parent->Inner.y1 -= Height;
}

void sWinTimeline::OnPaint2D()
{
  sRect r;
  sInt width,y,ytext,x;
  sInt l0,l1;

  sInt LoopStart;
  sInt LoopEnd;
  sInt BeatEnd;
  sInt BeatTime;
  sInt LoopEnable;

  Timeline->GetLoop(LoopStart,LoopEnd);
  BeatTime = Timeline->GetBeat();
  BeatEnd = Timeline->GetEnd();
  LoopEnable = Timeline->GetLooping();

  width = Client.SizeX()-1;
  y = Client.y1-Height+1;
  ytext = y+sGui->PropFont->GetHeight();

  r = Client;
  r.y0 = y-1;
  r.y1 = y;
  sRect2D(r,sGC_DRAW);

  r = Client;
  r.y0 = y;
  r.y1 = ytext;

  if(LoopStart < LoopEnd)
  {
    l0 = sMulDiv(LoopStart,width,BeatEnd)+Client.x0;
    l1 = sMulDiv(LoopEnd  ,width,BeatEnd)+Client.x0;
    r.x0 = Client.x0;
    r.x1 = l0;
    sRect2D(r,sGC_BACK);
    r.x0 = l0;
    r.x1 = l1;
    sRect2D(r,LoopEnable ? sGC_SELECT : sGC_BUTTON);
    r.x0 = l1;
    r.x1 = Client.x0;
    sRect2D(r,sGC_BACK);
  }
  else
  {
    sRect2D(r,sGC_BACK);
  }

  // determine spacing to use
  static const sInt MinPixelsBetweenBeats = 8; // to make sure it isn't too cluttered
  
  sInt numBeats = (BeatEnd + 0xffff) >> 16;
  sInt minSkip = sMulDiv(MinPixelsBetweenBeats,numBeats,width);

  sInt skip = AlignUpToTimebase(minSkip,DecimalTimebase);
  sInt timeSkip = skip * 0x10000;

  // spacing for text labels, calculate x range of tick marks
  static const sInt MinPixelsBetweenLabels = 100;

  sInt minLabelSkip = sMulDiv(MinPixelsBetweenLabels,numBeats/sMax(skip,1),width);
  sInt labelSkip = AlignUpToTimebase(minLabelSkip,DecimalTimebase);
  sInt ltSkip = labelSkip * timeSkip; // step in time units between adjacent labels

  sArray<sInt> labelTickPos;
  labelTickPos.AddTail(0); // start label is always there
  for(sInt i=ltSkip;i<BeatEnd;i+=ltSkip)
    labelTickPos.AddTail(sMulDiv(i,width,BeatEnd)+Client.x0);

  if(labelTickPos.GetCount()>1) labelTickPos.RemTail(); // pop last one
  labelTickPos.AddTail(Client.x0+width); // end label is always there

  // draw tick marks
  sInt y0long = y;
  sInt y0short = y+5;

  for(sInt i=0;i<BeatEnd;i+=timeSkip)
  {
    x = sMulDiv(i,width,BeatEnd)+Client.x0;
    r.x0 = x;
    r.x1 = x+1;
    r.y0 = ((i % ltSkip) != 0) ? y0short : y0long;
    sRect2D(r,(i&0x30000)==0?sGC_DRAW:sGC_DRAW);
  }

  r.x0 = Client.x1 - 1;
  r.x1 = Client.x1;
  r.y0 = y0long;
  sRect2D(r,sGC_DRAW); // last tick mark

  // draw text labels
  r.y0 = ytext;
  r.y1 = Client.y1;
  sInt count = labelTickPos.GetCount();
  sGui->PropFont->SetColor(sGC_TEXT,sGC_BACK);

  for(sInt i=0;i<count;i++)
  {
    sString<32> text;
    text.PrintF(L"%d",((i == count-1) ? BeatEnd : i*ltSkip) >> 16);

    sInt align = 0;

    if(i == 0) // first one
    {
      r.x0 = labelTickPos[i];
      r.x1 = (labelTickPos[i] + labelTickPos[i+1] + 1) / 2;
      align = sF2P_LEFT;
    }
    else if(i == count-1) // last one
    {
      r.x0 = (labelTickPos[i-1] + labelTickPos[i] + 1) / 2;
      r.x1 = labelTickPos[i];
      align = sF2P_RIGHT;
    }
    else
    {
      r.x0 = (labelTickPos[i-1] + labelTickPos[i] + 1) / 2;
      r.x1 = (labelTickPos[i+1] + labelTickPos[i] + 1) / 2;
    }

    sGui->PropFont->Print(align|sF2P_OPAQUE|sF2P_LIMITED,r,text);
  }

  // draw current time
  x = sMulDiv(BeatTime,width,BeatEnd)+Client.x0;
  r.x0 = sMax(Client.x0,x-1);
  r.x1 = x+1;
  r.y0 = y0long;
  sRect2D(r,sGC_RED);

  Timeline->OnFrame();
}

void sWinTimeline::CmdStart()
{
  sInt ls,le;
  Timeline->GetLoop(ls,le);
  Timeline->EnablePlaying(0);
  Timeline->EnableScratching(0);
  if(ls<le)
    Timeline->SeekBeat(ls);
  else
    Timeline->SeekBeat(0);
  Timeline->EnablePlaying(1);
  Update();
}

void sWinTimeline::CmdPause()
{
  Timeline->EnablePlaying(!Timeline->GetPlaying());
  Update();
}

void sWinTimeline::CmdLoop()
{
  Timeline->EnableLoop(!Timeline->GetLooping());
  Update();
}

void sWinTimeline::DragScratch(const sWindowDrag &dd)
{
  switch(dd.Mode)
  {
  case sDD_START:
    Timeline->EnableScratching(1);
    // nobreak
  case sDD_DRAG:
    Timeline->SeekBeat(sMulDiv(dd.MouseX-Client.x0,Timeline->GetEnd(),Client.SizeX()));
    Update();
    break;
  case sDD_STOP:
    Timeline->EnableScratching(0);
    break;
  }
}

void sWinTimeline::DragMark(const sWindowDrag &dd)
{
}

void sWinTimeline::SetDecimalTimebase(sBool enable)
{
  DecimalTimebase = enable;
  Update();
}

/****************************************************************************/
/****************************************************************************/

sWinTimetable::sWinTimetable(sTimetable *tt,sTimelineInterface *tl)
{
  Timetable = tt;
  Timeline = tl;
  Zoom = 0x1000;
  DragMode = 0;
  DragLockFlags = DLF_NONE;
  tl->AddNotify(this);
}

sWinTimetable::~sWinTimetable()
{
}

void sWinTimetable::Tag()
{
  Timetable->Need();
}

void sWinTimetable::InitWire(const sChar *name)
{
  sWireClientWindow::InitWire(name);
  sWire->AddKey(name,L"Delete",sMessage(this,&sWinTimetable::CmdDelete));
  sWire->AddKey(name,L"Add",sMessage(this,&sWinTimetable::CmdAdd));

  sWire->AddDrag(name,L"Scratch",sMessage(this,&sWinTimetable::DragScratch));
  sWire->AddDrag(name,L"Move",sMessage(this,&sWinTimetable::DragMove));
  sWire->AddDrag(name,L"Duplicate",sMessage(this,&sWinTimetable::DragDuplicate));
  sWire->AddDrag(name,L"Width",sMessage(this,&sWinTimetable::DragWidth));
  sWire->AddDrag(name,L"Frame",sMessage(this,&sWinTimetable::DragFrame));
  sWire->AddDrag(name,L"Zoom",sMessage(this,&sWinTimetable::DragZoom));
}

void sWinTimetable::SetDragLockFlags(DragLockFlag flags)
{
  DragLockFlags = flags;
}

sBool sWinTimetable::OnCheckHit(const sWindowDrag &dd)
{
  sRect r;
  sTimeTableClip *te;
  sFORALL(Timetable->Clips,te)
  {
    EntryToScreen(te,r);
    if(r.Hit(dd.MouseX,dd.MouseY))
      return 1;
  }
  return 0;
}

void sWinTimetable::OnCalcSize()
{
  Height = sGui->PropFont->GetHeight()+4;
  ReqSizeX = Timeline->GetEnd() / Zoom;
  ReqSizeY = Timetable->MaxLines * Height;
}

void sWinTimetable::EntryToScreen(const sTimeTableClip *ent,sRect &rect)
{
  rect.x0 = Client.x0 + sInt(ent->Start/Zoom);
  rect.x1 = Client.x0 + sInt((ent->Start+ent->Length)/Zoom);
  rect.y0 = Client.y0 + Height*ent->Line;
  rect.y1 = rect.y0 + Height;
}

void sWinTimetable::OnPaint2D()
{
  sTimeTableClip *ent;
  sRect r;

  sClipPush();

  sFORALL(Timetable->Clips,ent)
  {
    EntryToScreen(ent,r);
    sGui->PaintButton(r,ent->Name,ent->Selected ? sGPB_DOWN : 0);
    sClipExclude(r);
  }

  sRect2D(Client,sGC_BACK);

  sInt x = Timeline->GetBeat()/Zoom+Client.x0;
  sRect2D(x,Client.y0,x+1,Client.y1,sGC_SELECT);

  sClipPop();


  if(DragMode==1)
  {
    sRect r = DragRect;
    r.Sort();
    sGui->RectHL(r,sGC_DRAW,sGC_DRAW);
  }
}


void sWinTimetable::CmdDelete()
{
  sRemTrue(Timetable->Clips,&sTimeTableClip::Selected);
  Update();
  SelectMsg.Post();
}

void sWinTimetable::CmdAdd()
{
  sTimeTableClip *te;
  sFORALL(Timetable->Clips,te)
    te->Selected = 0;
  te = Timetable->NewEntry();
  te->Selected = 1;
  Timetable->Clips.AddTail(te);
  Update();
  SelectMsg.Post();
}

void sWinTimetable::DragScratch(const sWindowDrag &dd)
{
  switch(dd.Mode)
  {
  case sDD_START:
    Timeline->EnableScratching(1);
    break;
  case sDD_DRAG:
    Timeline->SeekBeat((dd.MouseX-Client.x0)*Zoom);
    break;
  case sDD_STOP:
    Timeline->EnableScratching(0);
    break;
  }
}

void sWinTimetable::DragMove(const sWindowDrag &dd)
{
  sTimeTableClip *te;
  sInt dx,dy;
  sInt px,py;
  sInt update;
  sRect r;
  update = 0;
  switch(dd.Mode)
  {
  case sDD_START:
    sFORALL(Timetable->Clips,te)
    {
      te->Selected = 0;
      te->DragStartX = te->Start;
      te->DragStartY = te->Line;
    }
    sFORALL(Timetable->Clips,te)
    {
      EntryToScreen(te,r);
      if(r.Hit(dd.MouseX,dd.MouseY))
      {
        te->Selected = 1;
        break;
      }
    }
    update = 1;
    SelectMsg.Post();
    break;
  case sDD_DRAG:
    dx = sInt(dd.DeltaX*Zoom);
    dy = sDivDown(dd.DeltaY,Height);
    sFORALL(Timetable->Clips,te)
    {
      if(te->Selected)
      {
        px = sClamp<sInt>(te->DragStartX+dx,0,Timeline->GetEnd()-1)&~QUANT;
        py = sClamp<sInt>(te->DragStartY+dy,0,Timetable->MaxLines-1);
        if(px!=te->Start || py!=te->Line)
        {
          if ((DragLockFlags&DLF_HORIZONTAL)==0)
          {
            te->Start = px;
            sGui->Notify(te->Start);
          }

          if ((DragLockFlags&DLF_VERTICAL)==0)
          {
            te->Line = py;
            sGui->Notify(te->Line);
          }

          update = 1;
        }
      }
    }
    break;
  }
  if(update)
    Update();
}

void sWinTimetable::DragDuplicate(const sWindowDrag &dd)
{
  sTimeTableClip *te,*hit;
  sRect r;
  sArray<sTimeTableClip *> dupes;

  switch(dd.Mode)
  {
  case sDD_START:
    hit = 0;
    sFORALL(Timetable->Clips,te)
    {
      te->DragStartX = te->Start;
      te->DragStartY = te->Line;
      EntryToScreen(te,r);
      if(r.Hit(dd.MouseX,dd.MouseY))
      {
        hit = te;
        break;
      }
    }
    if(hit)
    {
      sFORALL(Timetable->Clips,te)
        te->Selected = 0;
      hit->Selected = 1;
      SelectMsg.Post();
      Update();
    }
    sFORALL(Timetable->Clips,te)
      if(te->Selected)
        dupes.AddTail(Timetable->Duplicate(te));
    Timetable->Clips.Add(dupes);
    break;
  case sDD_DRAG:
  case sDD_STOP:
    DragMove(dd);
    break;
  }
}

void sWinTimetable::DragWidth(const sWindowDrag &dd)
{
  sTimeTableClip *te,*hit;
  sRect r;
  sInt dx,px;

  switch(dd.Mode)
  {
  case sDD_START:
    hit = 0;
    sFORALL(Timetable->Clips,te)
    {
      te->DragStartX = te->Length;
      EntryToScreen(te,r);
      if(r.Hit(dd.MouseX,dd.MouseY))
      {
        hit = te;
        break;
      }
    }
    if(hit)
    {
      sFORALL(Timetable->Clips,te)
        te->Selected = 0;
      hit->Selected = 1;
      SelectMsg.Post();
      Update();
    }
    break;
  case sDD_DRAG:
    dx = sInt(dd.DeltaX*Zoom);
    sFORALL(Timetable->Clips,te)
    {
      if(te->Selected)
      {
        px = sClamp<sInt>((te->DragStartX+dx)&~QUANT,0x1000,0x1000000);
        if(px!=te->Length)
        {
          te->Length = px;
          sGui->Notify(&te->Length);
          Update();
        }
      }
    }
    break;
  }
}

void sWinTimetable::DragFrame(const sWindowDrag &dd)
{
  sTimeTableClip *te;
  sRect r,w;
  switch(dd.Mode)
  {
  case sDD_START:
    DragMode = 1;
    DragRect.x0 = DragRect.x1 = dd.MouseX;
    DragRect.y0 = DragRect.y1 = dd.MouseY;
    Update();
    break;
  case sDD_DRAG:
    DragRect.x1 = dd.MouseX;
    DragRect.y1 = dd.MouseY;
    Update();
    break;
  case sDD_STOP:
    DragMode = 0;
    r = DragRect;
    r.Sort();
    sFORALL(Timetable->Clips,te)
    {
      EntryToScreen(te,w);
      te->Selected = w.IsInside(r);
    }
    SelectMsg.Post();
    Update();
    break;
  }
}

void sWinTimetable::DragZoom(const sWindowDrag &dd)
{
  switch(dd.Mode)
  {
  case sDD_START:
    DragStartX = -ScrollX*Zoom;
    DragZoomX = Zoom;
    DragCenterX = (dd.StartX-Outer.x0)*Zoom;
    break;

  case sDD_DRAG:
    Zoom = sClamp<sInt>((sInt)sFExp(sFLog(DragZoomX)-dd.DeltaX*0.01f),0x100,0x1000000);
    ScrollX = -(DragStartX/Zoom - DragCenterX/Zoom + dd.StartX-Outer.x0);
    Layout();
    Update();
    break;
  }
}

/****************************************************************************/

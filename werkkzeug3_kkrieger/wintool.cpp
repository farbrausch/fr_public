// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_start.hpp"
#include "_gui.hpp"
#include "_util.hpp"
#include "wintool.hpp"

/****************************************************************************/

WinPerfMon::WinPerfMon()
{
  AddScrolling(0,1);
  Mode = 0;
  LastPaintTime = 0;
}

void WinPerfMon::OnPaint()
{
  sInt i,lasttime,gputime,cputime;
  sInt curtime,tdelta,scalefactor;
  sInt avgmax;
  sPerfMonRecord *rec;
  sPerfMonZone2 *zone;
  sInt x,y,x0,x1,xs;
  sChar buffer[128];
  sInt stack[128];
  sInt sp;
  sRect r;
  sInt t0;

// calculate scalefactor; exp(c*500) = 0.5 => c = -1.386294e-3f
  curtime = sSystem->GetTime();
  tdelta = curtime - LastPaintTime;
  LastPaintTime = curtime;
  scalefactor = (1.0f - sFExp(tdelta * -1.386294e-3f)) * 65536;

  ClearBack();
  x = Client.x0;
  y = Client.y0;

  switch(Mode)
  {
  case 0:
    sPainter->Print(sGui->PropFont,x+2,y+2,"CPU usage",sGui->Palette[sGC_TEXT]);
    y += sPainter->GetHeight(sGui->PropFont)+2;

// prepare

    avgmax = 0;
    lasttime = 0;
    zone = sPerfMon->Tokens;
    for(i=0;i<sPerfMon->TokenCount;i++) 
    {
      zone->EnterTime=0;
      zone->TotalTime=0;
      zone++;
    }
    avgmax = sPerfMon->Tokens[0].AverageTime;
    if(avgmax<500)
      avgmax = 500;

// count

    sp = 0;
    xs = Client.XSize()-20;
    y+=10;
    rec = &sPerfMon->Rec[1-sPerfMon->DBuffer][0];
    t0 = rec->Time;
    while(rec->Type)
    {
      if(rec->Type==1)
      {
        zone = &sPerfMon->Tokens[rec->Token];
        zone->TotalTime -= rec->Time;
        stack[sp++] = rec->Time;
      }
      else
      {
        sp--;
        zone = &sPerfMon->Tokens[rec->Token];
        zone->TotalTime += rec->Time;

        x0 = sMulDiv(stack[sp]-t0,xs,avgmax);
        x1 = sMulDiv(rec->Time-t0,xs,avgmax);
        if(x1>x0)
        {
          r.Init(x+10+x0,y+sp*4,x+10+x1,y+sp*4+4);
          sPainter->Paint(sGui->FlatMat,r,zone->Color);
        }
      }
      if(rec->Time>lasttime)
        lasttime = rec->Time;

      rec++;
    }
    sVERIFY(sp==0);

    for(i=0;sPerfMon->SoundRec[1-sPerfMon->DBuffer][i]!=~0;i+=2)
    {
      x0 = sMulDiv(sPerfMon->SoundRec[1-sPerfMon->DBuffer][i+0]-t0,xs,avgmax);
      x1 = sMulDiv(sPerfMon->SoundRec[1-sPerfMon->DBuffer][i+1]-t0,xs,avgmax);
      if(x1>x0)
      {
        r.Init(x+10+x0,y,x+10+x1,y+4*19);
        sPainter->Paint(sGui->FlatMat,r,sPerfMon->Tokens[0].Color);
      }
    }
    y += 4*20;

// cpu/gpu markers
    x0 = 0;
    x1 = sMulDiv(sPerfMon->Markers[1-sPerfMon->DBuffer][1]-t0,xs,avgmax);
    if(x1>x0)
    {
      r.Init(x+10+x0,y,x+10+x1,y+4);
      sPainter->Paint(sGui->FlatMat,r,0xffff7800);
    }
    cputime = sPerfMon->Markers[1-sPerfMon->DBuffer][1]-t0;
    y += 4;

    x0 = sMulDiv(sPerfMon->Markers[1-sPerfMon->DBuffer][0]-t0,xs,avgmax);
    x1 = sMulDiv(lasttime-t0,xs,avgmax);
    if(x1>x0)
    {
      r.Init(x+10+x0,y,x+10+x1,y+4);
      sPainter->Paint(sGui->FlatMat,r,0xff2020c0);
    }
    gputime = lasttime-sPerfMon->Markers[1-sPerfMon->DBuffer][0];
    y += 4;

// done

    zone = sPerfMon->Tokens;
    for(i=0;i<sPerfMon->TokenCount;i++) 
    {
      zone->AverageTime += sMulShift(zone->TotalTime - zone->AverageTime,scalefactor);
      //zone->AverageTime=(zone->AverageTime*3 + zone->TotalTime*1)/4;
  //   if(zone->AverageTime>0)
      {
        sSPrintF(buffer,sizeof(buffer),"%14s  %6d %6d",zone->Name,zone->AverageTime,zone->TotalTime);
        sPainter->Print(sGui->FixedFont,x,y,buffer,zone->Color);
        y+=sPainter->GetHeight(sGui->FixedFont)+1;
      }
      zone++;
    }
    y+=sPainter->GetHeight(sGui->FixedFont)+1;
    sSPrintF(buffer,sizeof(buffer),"%14s  %6d","CPU",cputime);
    sPainter->Print(sGui->FixedFont,x,y,buffer,0xffff7800);
    y+=sPainter->GetHeight(sGui->FixedFont)+1;
    sSPrintF(buffer,sizeof(buffer),"%14s  %6d","GPU",gputime);
    sPainter->Print(sGui->FixedFont,x,y,buffer,0xff2020c0);
    y+=sPainter->GetHeight(sGui->FixedFont)+1;
    sSPrintF(buffer,sizeof(buffer),"%14s  0x%08x","Index max",sPerfMon->IndexOld);
    sPainter->Print(sGui->FixedFont,x,y,buffer,sGui->Palette[sGC_TEXT]);
    y+=sPainter->GetHeight(sGui->FixedFont)+1;
    break;

  default:
    y = Client.y0;
  }
  SizeY = y-Client.y0;
}

void WinPerfMon::OnKey(sU32 key)
{
  switch(key & 0x8001ffff)
  {
  case 'i':
    sPerfMon->IndexOld = 0;
    break;
  }
}

/****************************************************************************/

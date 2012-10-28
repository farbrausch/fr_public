/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "base/system.hpp"
#include "wz4frlib/wz4_gui.hpp"
#include "wz4frlib/wz4_demo2_ops.hpp"
#include "wz4frlib/wz4_demo2nodes.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   MathPaper - elektionisches zoomendes Karopapier                    ***/
/***                                                                      ***/
/****************************************************************************/

static sInt signmodi(sF32 a,sF32 b)
{
  if(a<0)
    return -sInt(-a/b);
  else
    return sInt(a/b);
}

sInt sMathPaper::XToS(sF32 v)
{
  return Client.x0 + sInt((v-Range.x0) * Client.SizeX() / Range.SizeX());
}

sInt sMathPaper::YToS(sF32 v)
{
  return Client.y0 + sInt((v-Range.y0) * Client.SizeY() / Range.SizeY());
}

sF32 sMathPaper::XToV(sInt s)
{
  return Range.x0 + (s-Client.x0) * Range.SizeX() / Client.SizeX();
}

sF32 sMathPaper::YToV(sInt s)
{
  return Range.y0 + (s-Client.y0) * Range.SizeY() / Client.SizeY();
}

sMathPaper::sMathPaper()
{
  Range.Init(-1,-1,1,1);
}

sMathPaper::~sMathPaper()
{
}

/****************************************************************************/

void sMathPaper::Paint2D()
{
  sString<64> buffer;
  sF32 xstep,ystep,xmin,xmax,ymin,ymax;
  sFont2D *font = sGui->FixedFont;
  sInt h = font->GetHeight();
  font->SetColor(sGC_TEXT,sGC_BACK);

//  sRect2D(Client,sGC_RED);
  sSetColor2D(sGC_MAX+0,0xffc0c0c0);
  sSetColor2D(sGC_MAX+1,0xff808080);
  sSetColor2D(sGC_MAX+2,0xff404040);

  // scaling

  xstep = sAbs(Range.SizeX()/Client.SizeX());
  xstep = sPow(10,sInt(sFFloor(sLog10(xstep*10)))+1);    // exactly 10 pixels is most narrow lines!
  xmin = signmodi(Range.x0,xstep);
  xmax = signmodi(Range.x1,xstep);
  ystep = sAbs(Range.SizeY()/Client.SizeY());
  ystep = sPow(10,sInt(sFFloor(sLog10(ystep*10)))+1);
  ymin = signmodi(Range.y1,ystep);
  ymax = signmodi(Range.y0,ystep);

  // lines

  for(sInt i=xmin-xstep;i<=xmax;i++)
  {
    sInt x = XToS(i*xstep);
    sInt col = ( (i==0)?2:(i%10==0) );
    sRect2D(x,Client.y0,x+1,Client.y1,sGC_MAX+col);
    if(col!=0)
    {
      buffer.PrintF(L"%f",i*xstep);
      font->Print(0,x,Client.y1-h,buffer);
    }
  }

  for(sInt i=ymin-ystep;i<=ymax;i++)
  {
    sInt y = YToS(i*ystep);
    sInt col = ( (i==0)?2:(i%10==0) );
    sRect2D(Client.x0,y,Client.x1,y+1,sGC_MAX+col);
    if(col!=0)
    {
      buffer.PrintF(L"%f",i*ystep);
      font->Print(0,Client.x0,y,buffer);
    }
  }

  // rects

  for(sInt y=ymin-ystep-1;y<=ymax;y++)
  {
    for(sInt x=xmin-xstep-1;x<=xmax;x++)
    {
      sRect r;
      r.x0 = XToS((x+0)*xstep)+1;
      r.y0 = YToS((y+1)*ystep)+1;
      r.x1 = XToS((x+1)*xstep);
      r.y1 = YToS((y+0)*ystep);

      sRect2D(r,sGC_DOC);
    }
  }

  // labels

  for(sInt i=ymin-ystep;i<=ymax;i++)
  {
    sInt y = YToS(i*ystep);
    sInt col = ( (i==0)?2:(i%10==0) );
    if(col!=0)
    {
      buffer.PrintF(L"%f",i*ystep);
      font->Print(0,Client.x0,y,buffer);
    }
  }

  for(sInt i=xmin-xstep;i<=xmax;i++)
  {
    sInt x = XToS(i*xstep);
    sInt col = ( (i==0)?2:(i%10==0) );
    if(col!=0)
    {
      buffer.PrintF(L"%f",i*xstep);
      font->Print(0,x,Client.y1-h,buffer);
    }
  }
}

void sMathPaper::DragScroll(const sWindowDrag &dd)
{
  sF32 v0,v1;
  switch(dd.Mode)
  {
  case sDD_START:
    DragStart = Range;
    break;
  case sDD_DRAG:
    v0 = XToV(dd.StartX);
    v1 = XToV(dd.MouseX);
    Range.x0 = DragStart.x0 - (v1-v0);
    Range.x1 = DragStart.x1 - (v1-v0);
    v0 = YToV(dd.StartY);
    v1 = YToV(dd.MouseY);
    Range.y0 = DragStart.y0 - (v1-v0);
    Range.y1 = DragStart.y1 - (v1-v0);
    ScrollMsg.Post();
    break;
  }
}

void sMathPaper::DragZoom(const sWindowDrag &dd,sDInt mode)
{
  switch(dd.Mode)
  {
  case sDD_START:
    DragStart = Range;
    break;
  case sDD_DRAG:
    Range = DragStart;
    if(mode&1)
    {
      sF32 x = XToV(dd.StartX);
      sF32 f = sClamp<sF32>(sExp(-dd.DeltaX*0.01f),0.01f,100.0f);
      sF32 xa = x-DragStart.x0;
      sF32 xb = x-DragStart.x1;
      Range.x0 = x-xa*f;
      Range.x1 = x-xb*f;
    }
    if(mode&2)
    {
      sF32 y = YToV(dd.StartY);
      sF32 f = sClamp<sF32>(sExp(-dd.DeltaY*0.01f),0.01f,100.0f);
      sF32 ya = y-DragStart.y0;
      sF32 yb = y-DragStart.y1;
      Range.y0 = y-ya*f;
      Range.y1 = y-yb*f;
    }
    ScrollMsg.Post();
    break;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Calucluate Spline helper                                           ***/
/***                                                                      ***/
/****************************************************************************/

struct CalcSpline
{
  void PrimeTime(sF32 t,const sArray<Wz4RenderArraySpline *> &sortedkeys,sInt flags,sF32 maxtime);
  void PrimeIndex(sInt i,const sArray<Wz4RenderArraySpline *> &sortedkeys,sInt flags,sF32 maxtime);
  sF32 CalcTime(sF32 t,sInt mode);    
  sF32 CalcFrac(sF32 f,sInt mode);

  sBool good;
  sF32 Default;
  Wz4RenderArraySpline k0,k1,k2,k3;
};

static const Wz4RenderArraySpline sub(const Wz4RenderArraySpline &a,const Wz4RenderArraySpline &b)
{
  Wz4RenderArraySpline r;

  r.Time = a.Time - b.Time;
  r.Value = a.Value - b.Value;

  return r;
}

void CalcSpline::PrimeTime(sF32 t,const sArray<Wz4RenderArraySpline *> &sortedkeys,sInt flags,sF32 maxtime)
{
  const Wz4RenderArraySpline *key;
  sInt max = sortedkeys.GetCount();

  if(max<2)
  {
    PrimeIndex(0,sortedkeys,flags,maxtime);
  }
  else
  {
    if(t<=sortedkeys[0]->Time)
    {
      PrimeIndex(-1,sortedkeys,flags,maxtime);
    }
    else if(t>=sortedkeys[max-1]->Time)
    {
      PrimeIndex(max+1,sortedkeys,flags,maxtime);
    }
    else
    {
      sFORALL(sortedkeys,key)
      {
        if(_i>0)
        {
          if(t >= sortedkeys[_i-1]->Time && t < sortedkeys[_i]->Time)
          {
            PrimeIndex(_i-1,sortedkeys,flags,maxtime);
            return;
          }
        }
      }
      PrimeIndex(-1,sortedkeys,flags,maxtime); // should never happen...
    }
  }
}

void CalcSpline::PrimeIndex(sInt i,const sArray<Wz4RenderArraySpline *> &sortedkeys,sInt flags,sF32 maxtime)
{
  sInt max = sortedkeys.GetCount();
  good = 0;
  if(max==0)
  {
    Default = 0;
    return;
  }
  else if(max==1)
  {
    Default = sortedkeys[0]->Value;
    return;
  }

  if((flags & SSF_BoundMask)!=SSF_BoundWrap)
  {
    if(i<0)
    {
      Default = sortedkeys[0]->Value;
      return;
    }
    else if(i>max-1)
    {
      Default = sortedkeys[max-1]->Value;
      return;
    }
  }

  good = 1;

  if((flags & SSF_BoundMask)!=SSF_BoundWrap)
  {
    k1 = *sortedkeys[i];
    k2 = *sortedkeys[i+1];
    if(i-1>=0)
    {
      k0 = *sortedkeys[i-1];
    }
    else
    {
      k0 = sub(k1,sub(k2,k1));
    }
    if(i+2<max)
    {
      k3 = *sortedkeys[i+2];
    }
    else
    {
      k3 = sub(k2,sub(k1,k2));
    }
  }
  else
  {
    k0 = *sortedkeys[(i+max-1)%max];
    k1 = *sortedkeys[(i+max  )%max];
    k2 = *sortedkeys[(i+max+1)%max];
    k3 = *sortedkeys[(i+max+2)%max];

    if(i-1<0)      k0.Time -= maxtime;
    if(i  <0)      k1.Time -= maxtime;
    if(i+1>=max)   k2.Time += maxtime;
    if(i+2>=max)   k3.Time += maxtime;
  }
}

sF32 CalcSpline::CalcTime(sF32 t,sInt mode)    
{
  if(good)
    return CalcFrac((t-k1.Time)/(k2.Time-k1.Time),mode);
  else
    return Default;
}

sF32 CalcSpline::CalcFrac(sF32 f,sInt mode)
{
  if(good)
  {
    sF32 c0,c1,c2,c3;
    switch(mode)
    {
    case 0:
      c0=0; c1=1;   c2=0; c3=0;
      break;
    case 1:
      c0=0; c1=1-f; c2=f; c3=0;
      break;
    case 2:
      sHermiteUniform(f , k1.Time-k0.Time,k2.Time-k1.Time,k3.Time-k2.Time , c0,c1,c2,c3);
      break;
    case 3:
      sHermite(f,c0,c1,c2,c3);
      break;
    case 4:
      sUniformBSpline(f,c0,c1,c2,c3);
      break;
    }

    return c0*k0.Value + c1*k1.Value + c2*k2.Value + c3*k3.Value;
  }
  else
  {
    return Default;
  }
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Spline Custom Editor                                               ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

sFRect Wz4SplineCed::SaveRange;
sBool Wz4SplineCed::RangeSaved;

Wz4SplineCed::Wz4SplineCed(wOp *op)
{
  Op = 0;

  DragMode = 0;
  ToolMode = 0;
  LastTimePixel = 0;
  Timeline0 = 0;
  Timeline1 = 1;

  SplitRequestX = 0;
  SplitRequestY = 0;
  SplitRequest = 0;

  Op = op;
  Grid.Range.Init(-0.1f,10,1.1f,-10);
  Grid.ScrollMsg = sMessage(this,&Wz4SplineCed::CmdScroll);

  Color[0] = 0xffff0000;
  Color[1] = 0xff00ff00;
  Color[2] = 0xff0000ff;
  Color[3] = 0xff00ffff;
  Color[4] = 0xffff00ff;
  Color[5] = 0xffc0c000;
  Color[6] = 0xff808080;
  Color[7] = 0xff000000;

  ChannelMask = ~0;
  ChannelMax = 0;

  UpdateInfo();
  CmdFrame();
  if(RangeSaved)
    Grid.Range = SaveRange;
}

Wz4SplineCed::~Wz4SplineCed()
{
}

void Wz4SplineCed::Tag()
{
  wCustomEditor::Tag();
  Op->Need();
}

/****************************************************************************/

void Wz4SplineCed::UpdateInfo()
{
  DragMode = 0;

  void *vp;
  Wz4RenderArraySpline *ar;
  for(sInt i=0;i<CHANNELS;i++)
    Curves[i].Clear();
  sFORALL(Op->ArrayData,vp)
  {
    ar = (Wz4RenderArraySpline *) vp;
    if(ar->Use>=0 && ar->Use<CHANNELS)
    Curves[ar->Use].AddTail(ar);
  }
  ChannelMax = ((Wz4RenderParaSpline *)Op->EditData)->Dimensions+1;
  Sort();
}

void Wz4SplineCed::Sort()
{
  for(sInt i=0;i<CHANNELS;i++)
    sSortUp(Curves[i],&Wz4RenderArraySpline::Time);
}

void Wz4SplineCed::MakeHandle(Wz4RenderArraySpline *key,sRect &r)
{
  sInt x,y;

  x = Grid.XToS(key->Time);
  y = Grid.YToS(key->Value);

  r.Init(x-3,y-3,x+4,y+4);
}

Wz4RenderArraySpline *Wz4SplineCed::Hit(sInt mx,sInt my,sInt &curve)
{
  Wz4RenderArraySpline *key;
  sRect r;
  for(sInt i=0;i<CHANNELS;i++)
  {
    if(ChannelMask & (1<<i))
    {
      sFORALL(Curves[i],key)
      {
        MakeHandle(key,r);
        if(r.Hit(mx,my))
        {
          curve = i;
          return key;
        }
      }
    }
  }
  return 0;
}

void Wz4SplineCed::LineSplit(sInt x0,sInt y0,sInt x1,sInt y1,sInt curve)
{
  if(!SplitRequest) return;
  if(x0==x1)  return;
  sInt x = SplitRequestX;
  sInt y = SplitRequestY;
  if(x<x0 || x>=x1) return;
  sInt r = 2;
  sInt ymin = sMin(y0,y1);
  sInt ymax = sMax(y0,y1);
  if(y<ymin-r || y>ymax+r) return;
  sInt yt = y0 + (x-x0)*(y1-y0)/(x1-x0);
  if(y<yt-r || y>yt+r) return;

  // we have a split!

  SplitRequest = 2;

  SplitResultTime = Grid.XToV(x);
  SplitResultCurve = curve;
}

/****************************************************************************/

void Wz4SplineCed::OnChangeOp()
{
  UpdateInfo();
  Update();
  DragItems.Clear();
}

void Wz4SplineCed::OnTime(sInt time)
{
  if(Timeline1>Timeline0)
  {
    time = Grid.XToS(sF32(time-Timeline0)/(Timeline1-Timeline0));
    if(LastTimePixel!=time)
    {
      sInt x0 = sMin(time,LastTimePixel)-1;
      sInt x1 = sMax(time,LastTimePixel)+1;
      sGui->Update(sRect(x0,Client.y0,x1,Client.y1));
    }
  }
}

void Wz4SplineCed::OnCalcSize(sInt &xs,sInt &ys)
{
}

void Wz4SplineCed::OnLayout(const sRect &client)
{
  Client = client;
}

void Wz4SplineCed::OnPaint2D(const sRect &client)
{
  Wz4RenderArraySpline *key;
  sRect r;
  CalcSpline cs;

  Client = client;
  Wz4RenderParaSpline *para = (Wz4RenderParaSpline *) Op->EditData;
  Timeline0 = App->TimelineWin->LoopStart;
  Timeline1 = App->TimelineWin->LoopEnd;

  sGui->BeginBackBuffer(Client);

  // karopapier

  Grid.Client = client;
  Grid.Paint2D();

  // current time

  if(Timeline1>Timeline0)
  {
    LastTimePixel = Grid.XToS(sF32(App->GetTimeBeats()-Timeline0)/(Timeline1-Timeline0));
    sRect2D(LastTimePixel-1,Client.y0,LastTimePixel+1,Client.y1,sGC_RED);
  }

  // labels

  if(1)
  {
    Wz4RenderParaSpline *para = (Wz4RenderParaSpline *) Op->EditData;
    sInt mode = para->GrabCamMode;
    sInt x = Client.x0+10;
    sInt y = Client.y0+10;
    static const sChar *names[8][8]=
    {
      { L"posx",L"posy",L"posz", },
      { L"rotx",L"roty",L"rotz", },
      { L"posx",L"posy",L"posz",L"rotx",L"roty",L"rotz",L"zoom" },
      { L"posx",L"posy",L"posz",L"targetx",L"targety",L"targetz",L"zoom",L"tilt" },
      { L"[0]",L"[1]",L"[2]",L"[3]",L"[4]",L"[5]",L"[6]",L"[7]", }
    };
    sFont2D *font = sGui->FixedFont;

    for(sInt i=0;i<para->Dimensions+1;i++)
    {
      if(ChannelMask & (1<<i))
      {
        const sChar *str = names[mode][i];
        if(str==0) str = names[4][i];
        sSetColor2D(sGC_MAX,Color[i]);
        font->SetColor(sGC_MAX,sGC_BACK);
        font->Print(0,x,y,str);
        x += font->GetWidth(str)+15;
      }
    }
  }

  // spline

  for(sInt i=0;i<CHANNELS;i++)
  {
    if(ChannelMask & (1<<i))
    {
      sInt xd,yd;
      sInt first=1;

      sSetColor2D(0,Color[i]);
      sInt i0 = 0;
      sInt i1 = Curves[i].GetCount()-1;
      if((para->Flags & SSF_BoundMask)==SSF_BoundWrap)
      {
        i0--;
        i1++;
      }
      for(sInt _i=i0;_i<i1;_i++)
      {
        cs.PrimeIndex(_i,Curves[i],para->Flags,para->MaxTime);
        sF32 t0 = cs.k1.Time;
        sF32 t1 = cs.k2.Time;

        if((para->Flags & SSF_BoundMask)!=SSF_BoundExtrapolate)
        {
          t0 = sMax<sF32>(t0,0);
          t1 = sMin<sF32>(t1,para->MaxTime);
        }

        if(t0<t1)
        {
          sInt x0 = Grid.XToS(t0);
          sInt x1 = Grid.XToS(t1);
          if(!(x1<Client.x0 || x0>=Client.x1))
          {
            sInt steps = sClamp((x1-x0)/4,12,150);

            for(sInt k=0;k<=steps;k++)
            {
              sF32 t = t0 + (t1-t0)*k/steps;
              sInt xc = Grid.XToS(t);
              t = (t-cs.k1.Time)/(cs.k2.Time-cs.k1.Time);
              sInt yc = Grid.YToS(cs.CalcFrac(t,para->Spline));
              if(!first)
              {
                sLine2D(xd,yd,xc,yc,0);
                if(SplitRequest)
                  LineSplit(xd,yd,xc,yc,i);
              }
              first = 0;
              xd = xc;
              yd = yc;
            }
          }
        }
      }
    }
  }

  // handles

  for(sInt i=0;i<CHANNELS;i++)
  {
    if(ChannelMask & (1<<i))
    {
      sFORALL(Curves[i],key)
      {
        MakeHandle(key,r);

        if(key->Select)
        {
          sRectFrame2D(r,sGC_SELECT);
          r.Extend(-1);
          sRect2D(r,sGC_DRAW);
        }
        else
        {
          sRectFrame2D(r,sGC_DRAW);
        }
      }
    }
  }

  // DragRect;

  if(DragMode==DM_FRAME || DragMode==DM_FRAMEADD)
    sRectFrame2D(DragRect,sGC_DRAW);

  // done

  sGui->EndBackBuffer();
}

sBool Wz4SplineCed::OnKey(sU32 key)
{
  key = (key & ~sKEYQ_MASK) | sMakeUnshiftedKey(key & sKEYQ_MASK);
  key &= ~sKEYQ_CAPS;
  if(key&sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
  if(key&sKEYQ_CTRL)  key |= sKEYQ_CTRL;
  if(key & sKEYQ_BREAK)
    ToolMode = 0;
  
  switch(key & (sKEYQ_BREAK|sKEYQ_MASK|sKEYQ_SHIFT|sKEYQ_CTRL))
  {
  case 'r':
    CmdReset();
    break;
  case 'f':
    CmdFrame();
    break;
  case 'x':
  case sKEY_DELETE:
    CmdDelete();
    break;
  case 'z':
    CmdZeroValue();
    break;
  case 'a':
    CmdAlignTime();
    break;
  case 's':
    CmdSelectSameTime();
    break;
  case 'Q'|sKEYQ_SHIFT:
    CmdQuantizeTime();
    break;
  case 'c':
    ToolMode = 1;
    break;
  case 't':
    ToolMode = 2;
    break;
  case 'n':
    CmdTimeFromKey();
    break;
  case 'k':
    ToolMode = 3;
    break;
  case 'j':
    ToolMode = 4;

  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
    CmdToggleChannel(key & 15);
    break;

  case '8':
    CmdAllChannels(1);
    break;
  case '9':
    CmdAllChannels(0);
    break;

  case 'o':
    CmdUpdateCam();
    break;

  case sKEY_BACKSPACE:
    CmdExit();
    break;
  }
  return 0;
}

void Wz4SplineCed::OnDrag(const sWindowDrag &dd,const sRect &client)
{
  Client = client;


  if(dd.Mode==sDD_START)
  {
    sInt channel;
    sBool hit;
    hit = Hit(dd.StartX,dd.StartY,channel)!=0;
    DragMode = 0;
    sU32 qual = sGetKeyQualifier();
    qual &= ~sKEYQ_CAPS;
    if(qual & sKEYQ_SHIFT) qual |= sKEYQ_SHIFT;
    if(qual & sKEYQ_CTRL)  qual |= sKEYQ_CTRL;

    if(ToolMode==0)
    {
      if(dd.Buttons==1)
      {
        if(qual&sKEYQ_SHIFT)
        {
          if(hit)
            DragMode = DM_HANDLEX;
          else
            DragMode = DM_FRAMEADD;
        }
        else if(qual & sKEYQ_CTRL)
        {
          if(hit)
            DragMode = DM_HANDLEY;
          else
            ;// freee
        }
        else
        {
          if(hit)
            DragMode = DM_HANDLE;
          else
            DragMode = DM_FRAME;
        }
      }
      else if(dd.Buttons==4)
      {
        if(qual&sKEYQ_SHIFT)
          DragMode = DM_ZOOM;
        else
          DragMode = DM_SCROLL;
      }
      else if(dd.Buttons==2)
      {
        if(qual&sKEYQ_SHIFT)
          DragMode = DM_INSERT;
        else if(qual&sKEYQ_CTRL)
          DragMode = DM_TIME;
        else
          CmdPopup();
      }
    }
    else if(ToolMode==1)
    {
      if(dd.Buttons==1)
        DragMode = DM_COPYCAM;
    }
    else if(ToolMode==2)
    {
      if(dd.Buttons==1)
        DragMode = DM_SCRATCHSLOW;
      if(dd.Buttons==2)
        DragMode = DM_SCRATCHFAST;
      if(dd.Buttons==4)
        DragMode = DM_TIME;
    }
    else if(ToolMode==3)
    {
      if(dd.Buttons==1)
        DragMode = DM_SCISSORALL;
      if(dd.Buttons==2)
        DragMode = DM_SCISSORVIS;
    }
    else if(ToolMode==4)
    {
      if(dd.Buttons==1)
        DragMode = DM_SCALE;
      if(dd.Buttons==2)
        DragMode = DM_SCALEX;
      if(dd.Buttons==4)
        DragMode = DM_SCALEY;
    }
  }

  switch(DragMode)
  {
    case DM_SCROLL:       DragScroll(dd);     break;
    case DM_ZOOM:         DragZoom(dd);       break;
    case DM_HANDLE:       DragHandle(dd,3);   break;
    case DM_HANDLEX:      DragHandle(dd,1);   break;
    case DM_HANDLEY:      DragHandle(dd,2);   break;
    case DM_SCALE:        DragScale(dd,3);   break;
    case DM_SCALEX:       DragScale(dd,1);   break;
    case DM_SCALEY:       DragScale(dd,2);   break;
    case DM_FRAME:        DragFrame(dd);      break;
    case DM_FRAMEADD:     DragFrame(dd,1);    break;
    case DM_INSERT:       DragInsert(dd);     break;
    case DM_COPYCAM:      DragCopyCam(dd);    break;
    case DM_SCRATCHSLOW:  App->TimelineWin->DragScratchIndirect(dd,0x100); break;
    case DM_SCRATCHFAST:  App->TimelineWin->DragScratchIndirect(dd,0x4000); break;
    case DM_TIME:         DragTime(dd);       break;
    case DM_SCISSORALL:   DragScissor(dd,0);    break;
    case DM_SCISSORVIS:   DragScissor(dd,1);    break;
  }

  if(dd.Mode==sDD_STOP)
    DragMode = 0;
}

/****************************************************************************/

void Wz4SplineCed::CmdPopup()
{
  sMenuFrame *mf = new sMenuFrame(App->CustomWin);
  mf->AddHeader(L"Actions",0);
  mf->AddHeader(L"Mouse",1);
  mf->AddHeader(L"Tools",3);

  mf->AddItem(L"Reset",sMessage(this,&Wz4SplineCed::CmdReset),'r');
  mf->AddItem(L"Frame",sMessage(this,&Wz4SplineCed::CmdFrame),'f');
  mf->AddItem(L"Align Time",sMessage(this,&Wz4SplineCed::CmdAlignTime),'a');
  mf->AddItem(L"Zero Value",sMessage(this,&Wz4SplineCed::CmdZeroValue),'z');
  mf->AddItem(L"Time from Key",sMessage(this,&Wz4SplineCed::CmdTimeFromKey),'n');
  mf->AddItem(L"Select same Time",sMessage(this,&Wz4SplineCed::CmdSelectSameTime),'s');
  mf->AddItem(L"Quantize Time",sMessage(this,&Wz4SplineCed::CmdQuantizeTime),'Q'|sKEYQ_SHIFT);
  mf->AddItem(L"Delete",sMessage(this,&Wz4SplineCed::CmdDelete),sKEY_DELETE);
  mf->AddItem(L"Delete",sMessage(this,&Wz4SplineCed::CmdDelete),'x');
  mf->AddItem(L"Exit",sMessage(this,&Wz4SplineCed::CmdExit),sKEY_BACKSPACE);
  mf->AddItem(L"Copy from Camera",sMessage(this,&Wz4SplineCed::CmdUpdateCam),'o');
  mf->AddItem(L"Reverse Time",sMessage(this,&Wz4SplineCed::CmdReverseTime),0);

  mf->AddItem(L"Toggle Channel 0",sMessage(this,&Wz4SplineCed::CmdToggleChannel,0),'0');
  mf->AddItem(L"Toggle Channel 1",sMessage(this,&Wz4SplineCed::CmdToggleChannel,1),'1');
  mf->AddItem(L"Toggle Channel 2",sMessage(this,&Wz4SplineCed::CmdToggleChannel,2),'2');
  mf->AddItem(L"Toggle Channel 3",sMessage(this,&Wz4SplineCed::CmdToggleChannel,3),'3');
  mf->AddItem(L"Toggle Channel 4",sMessage(this,&Wz4SplineCed::CmdToggleChannel,4),'4');
  mf->AddItem(L"Toggle Channel 5",sMessage(this,&Wz4SplineCed::CmdToggleChannel,5),'5');
  mf->AddItem(L"Toggle Channel 6",sMessage(this,&Wz4SplineCed::CmdToggleChannel,6),'6');
  mf->AddItem(L"Toggle Channel 7",sMessage(this,&Wz4SplineCed::CmdToggleChannel,7),'7');
  mf->AddItem(L"All Channels on" ,sMessage(this,&Wz4SplineCed::CmdAllChannels,1),'8');
  mf->AddItem(L"All Channels off" ,sMessage(this,&Wz4SplineCed::CmdAllChannels,0),'9');

  mf->AddItem(L"CaptureCamera",sMessage(),'c',-1,3,sGetColor2D(sGC_BUTTON));
  mf->AddItem(L"CaptureCamera",sMessage(),sKEY_LMB,-1,3);
  mf->AddItem(L"ScratchTime",sMessage(),'t',-1,3,sGetColor2D(sGC_BUTTON));
  mf->AddItem(L"ScratchSlow",sMessage(),sKEY_LMB,-1,3);
  mf->AddItem(L"ScratchFast",sMessage(),sKEY_RMB,-1,3);
  mf->AddItem(L"Time",sMessage(),sKEY_MMB,-1,3);
  mf->AddItem(L"Scissor",sMessage(),'k',-1,3,sGetColor2D(sGC_BUTTON));
  mf->AddItem(L"Scissor All",sMessage(),sKEY_LMB,-1,3);
  mf->AddItem(L"Scissor Visible",sMessage(),sKEY_RMB,-1,3);
  mf->AddItem(L"Scale",sMessage(),'j',-1,3,sGetColor2D(sGC_BUTTON));
  mf->AddItem(L"Scale",sMessage(),sKEY_LMB,-1,3);
  mf->AddItem(L"Scale Time",sMessage(),sKEY_RMB,-1,3);
  mf->AddItem(L"Scale Value",sMessage(),sKEY_MMB,-1,3);

  const sInt HIT = 0x01000000;
  const sInt MISS = 0x02000000;

  mf->AddItem(L"Move",sMessage(),sKEY_LMB|HIT,-1,1);
  mf->AddItem(L"Move Time",sMessage(),sKEY_LMB|sKEYQ_SHIFT|HIT,-1,1);
  mf->AddItem(L"Move Value",sMessage(),sKEY_LMB|sKEYQ_CTRL|HIT,-1,1);
  mf->AddItem(L"Select Rect",sMessage(),sKEY_LMB|MISS,-1,1);
  mf->AddItem(L"Select Rect Add",sMessage(),sKEY_LMB|MISS|sKEYQ_SHIFT,-1,1);
  mf->AddItem(L"Scroll",sMessage(),sKEY_MMB,-1,1);
  mf->AddItem(L"Context",sMessage(),sKEY_RMB,-1,1);
  mf->AddItem(L"Zoom",sMessage(),sKEY_MMB|sKEYQ_SHIFT,-1,1);
  mf->AddItem(L"Time",sMessage(),sKEY_RMB|sKEYQ_CTRL,-1,1);
  mf->AddItem(L"Insert Key",sMessage(),sKEY_RMB|sKEYQ_SHIFT,-1,1);

  sGui->AddPopupWindow(mf);
}

void Wz4SplineCed::CmdScroll()
{
  Update();
}

void Wz4SplineCed::CmdReset()
{
  Grid.Range.Init(0,10,32,-10);
  Update();
}  

void Wz4SplineCed::CmdFrame()
{
  sFRect r;
  Wz4RenderArraySpline *key;
  sInt ok = 0;
  r.x0 = r.y0 = 1.0e20;
  r.x1 = r.y1 = 1.0e-20;
  for(sInt i=0;i<CHANNELS;i++)
  {
    if(ChannelMask & (1<<i))
    {
      sFORALL(Curves[i],key)
      {
        sF32 x = key->Time;
        sF32 y = key->Value;

        r.x0 = sMin(r.x0,x);
        r.y0 = sMin(r.y0,y);
        r.x1 = sMax(r.x1,x);
        r.y1 = sMax(r.y1,y);

        ok = 1;
      }
    }
  }
  if(ok)
  {
    sF32 x = sMax(r.SizeX()/10,0.1f);
    sF32 y = sMax(r.SizeY()/10,0.1f);
    r.x0 -= x;
    r.y0 -= y;
    r.x1 += x;
    r.y1 += y;
    sSwap(r.y0,r.y1);
    Grid.Range = r;
  }
  else
  {
    Grid.Range.Init(0,10,32,-10);
  }
  Update();
}  

void Wz4SplineCed::CmdDelete()
{
  Wz4RenderArraySpline *key;
  sArray<Wz4RenderArraySpline *> del;
  for(sInt i=0;i<CHANNELS;i++)
  {
    if(ChannelMask & (1<<i))
    {
      sInt rest = 0;
      sFORALL(Curves[i],key)
        if(key->Select==0)
          rest++;
      if(rest>=2)
      sFORALL(Curves[i],key)
        if(key->Select)
          del.AddTail(key);
    }
  }
  sFORALL(del,key)
    Op->ArrayData.RemOrder(key);
  sDeleteAll(del);

  ChangeOp(Op);
  UpdateInfo();
}

void Wz4SplineCed::CmdExit()
{
  SaveRange = Grid.Range;
  RangeSaved = 1;
  App->PageListWin->SelectMsg.Send();
  sGui->SetFocus(App->StackWin);
}

void Wz4SplineCed::CmdToggleChannel(sDInt i)
{
  ChannelMask ^= 1<<i;
  Update();
}

void Wz4SplineCed::CmdAllChannels(sDInt i)
{
  if(i)
    ChannelMask = ~0;
  else
    ChannelMask = 0;
  Update();
}

void Wz4SplineCed::CmdUpdateCam()
{
  sF32 data[8];
  sInt count = GrabCam(data);

  CmdSelectSameTime();
  Wz4RenderArraySpline *key;
  sArray<Wz4RenderArraySpline *> del;
  for(sInt i=0;i<CHANNELS && i<count;i++)
  {
    if(ChannelMask & (1<<i))
    {
      sFORALL(Curves[i],key)
        if(key->Select)
          key->Value = data[i];
    }
  }
  ChangeOp(Op);
  Update();
}

void Wz4SplineCed::CmdTimeFromKey()
{
  sF32 time = 0;
  sBool sel = 0;
  Wz4RenderArraySpline *key;
  for(sInt i=0;i<CHANNELS;i++)
  {
    if(ChannelMask & (1<<i))
    {
      sFORALL(Curves[i],key)
      {
        if(key->Select)
        {
          time = key->Time;
          sel = 1;
        }
      }
    }
  }
  if(sel)
  {
    if(Timeline0 < Timeline1)
    {
      App->TimelineWin->ScratchDragStart();
      App->TimelineWin->ScratchDrag(Timeline0 + (Timeline1-Timeline0)*time);
      App->TimelineWin->ScratchDragStop();
    }
  }
}

void Wz4SplineCed::CmdReverseTime()
{
  Wz4RenderParaSpline *para = (Wz4RenderParaSpline *) Op->EditData;
  sF32 max = para->MaxTime;
  Wz4RenderArraySpline *key;
  for(sInt i=0;i<CHANNELS;i++)
  {
    if(ChannelMask & (1<<i))
    {
      sFORALL(Curves[i],key)
      {
        key->Time = max-key->Time;
      }
    }
  }
  Update();
  ChangeOp(Op);
  Sort();
}

/****************************************************************************/

void Wz4SplineCed::DragScroll(const sWindowDrag &dd)
{
  Grid.DragScroll(dd);
}

void Wz4SplineCed::DragZoom(const sWindowDrag &dd)
{
  Grid.DragZoom(dd);
}

void Wz4SplineCed::DragHandle(const sWindowDrag &dd,sDInt axis)
{
  sF32 v0,v1;
  Wz4RenderArraySpline *key;
  sInt channel;
  DragItem *di;
  Wz4RenderArraySpline *DragKey;
  
  switch(dd.Mode)
  {
  case sDD_START:
    DragKey = Hit(dd.StartX,dd.StartY,channel);
    if(DragKey)
    {
      if(!DragKey->Select)
      {
        for(sInt i=0;i<CHANNELS;i++)
          sFORALL(Curves[i],key)
            key->Select = 0;
        DragKey->Select = 1;
      }
      DragItems.Clear();
      for(sInt i=0;i<CHANNELS;i++)
      {
        if(ChannelMask & (1<<i))
        {
          sFORALL(Curves[i],key)
          {
            if(key->Select)
            {
              di = DragItems.AddMany(1);
              di->Key = key;
              di->Time = key->Time;
              di->Value = key->Value;
            }
          }
        }
      }
      Update();
      ChangeOp(Op);
    }
    break;
  case sDD_DRAG:
    sFORALL(DragItems,di)
    {      
      if(axis&1)
      {
        v0 = Grid.XToV(dd.StartX);
        v1 = Grid.XToV(dd.MouseX);
        di->Key->Time = di->Time - (v0-v1);
      }
      if(axis&2)
      {
        v0 = Grid.YToV(dd.StartY);
        v1 = Grid.YToV(dd.MouseY);
        di->Key->Value = di->Value - (v0-v1);
      }
      sGui->Notify(key);
    }
    Update();
    ChangeOp(Op);
    Sort();
    break;
  }
}


void Wz4SplineCed::DragScale(const sWindowDrag &dd,sDInt axis)
{
  Wz4RenderArraySpline *key;
  DragItem *di;
  sF32 zoomx;
  sF32 zoomy;
  
  switch(dd.Mode)
  {
  case sDD_START:
    DragItems.Clear();
    DragCenterX = 0;
    DragCenterY = 0;
    
    for(sInt i=0;i<CHANNELS;i++)
    {
      if(ChannelMask & (1<<i))
      {
        sFORALL(Curves[i],key)
        {
          if(key->Select)
          {
            di = DragItems.AddMany(1);
            di->Key = key;
            di->Time = key->Time;
            di->Value = key->Value;
            DragCenterX += di->Time;
            DragCenterY += di->Value;
          }
        }
      }
    }
    DragCenterX *= 1.0f/DragItems.GetCount();
    DragCenterY *= 1.0f/DragItems.GetCount();
    Update();
    ChangeOp(Op);
    break;
  case sDD_DRAG:
    zoomx = sPow(2,dd.DeltaX*0.004f);
    zoomy = sPow(2,-dd.DeltaY*0.004f);
    sFORALL(DragItems,di)
    {      
      if(axis&1)
      {
        di->Key->Time = (di->Time - DragCenterX) * zoomx + DragCenterX;
      }
      if(axis&2)
      {
        di->Key->Value = (di->Value - DragCenterY) * zoomy + DragCenterY;
      }
      sGui->Notify(key);
    }
    Update();
    ChangeOp(Op);
    Sort();
    break;
  }
}

void Wz4SplineCed::DragFrame(const sWindowDrag &dd,sDInt mode)
{
  Wz4RenderArraySpline *key;
  sBool updateop = 0;
  switch(dd.Mode)
  {
  case sDD_DRAG:
    DragRect.Init(dd.StartX,dd.StartY,dd.MouseX,dd.MouseY);
    DragRect.Sort();

    for(sInt i=0;i<CHANNELS;i++)
    {
      if(ChannelMask & (1<<i))
      {
        sFORALL(Curves[i],key)
        {
          sInt x = Grid.XToS(key->Time);
          sInt y = Grid.YToS(key->Value);

          sBool select = key->Select;
          if(mode==0)
            select = 0;
          select |= DragRect.Hit(x,y);
          if(select!=key->Select)
          {
            key->Select = select;
            updateop = 1;
          }
        }
      }
    }

    if(updateop)
      ChangeOp(Op);
    Update();
    break;
  case sDD_STOP:
    Update();
    break;
  }
}

void Wz4SplineCed::DragInsert(const sWindowDrag &dd)
{
  switch(dd.Mode)
  {
  case sDD_START:
    SplitRequestX = dd.MouseX;
    SplitRequestY = dd.MouseY;
    SplitRequest = 1;
    Update();
    break;
  case sDD_DRAG:
  case sDD_STOP:
    if(SplitRequest==2)
    {
      Wz4RenderArraySpline *key = new Wz4RenderArraySpline;
      Wz4RenderParaSpline *para = (Wz4RenderParaSpline *) Op->EditData;
      CalcSpline cs;

      cs.PrimeTime(SplitResultTime,Curves[SplitResultCurve],para->Flags,para->MaxTime);

      key->Time = SplitResultTime;
      key->Use = SplitResultCurve;
      key->Value = cs.CalcTime(SplitResultTime,para->Spline);
      key->Select = 0;
      Op->ArrayData.AddTail(key);
      ChangeOp(Op);
      UpdateInfo();
      SplitRequest = 0;
    }
    else if(SplitRequest==1)
    {
      SplitRequestX = dd.MouseX;
      SplitRequestY = dd.MouseY;
      Update();
    }
    break;
  }

  if(dd.Mode==sDD_STOP)
    SplitRequest = 0;
}

sInt Wz4SplineCed::GrabCam(sF32 *data)
{
  Wz4RenderParaSpline *para = (Wz4RenderParaSpline *) Op->EditData;
  sInt count;

  sMatrix34 &cam = Doc->LastView.Camera;

  switch(para->GrabCamMode)
  {
  case 0:
    data[0] = cam.l.x;
    data[1] = cam.l.y;
    data[2] = cam.l.z;
    data[3] = Doc->LastView.ZoomY;
    count = 4;
    break;
  case 1:
    cam.FindEulerXYZ2(data[0],data[1],data[2]);
    data[0] *= 1/sPI2F;
    data[1] *= 1/sPI2F;
    data[2] *= 1/sPI2F;
    data[3] = Doc->LastView.ZoomY;
    count = 4;
    break;
  case 2: // pos - euler - zoom
    data[0] = cam.l.x;
    data[1] = cam.l.y;
    data[2] = cam.l.z;
    cam.FindEulerXYZ2(data[3],data[4],data[5]);
    data[3] *= 1/sPI2F;
    data[4] *= 1/sPI2F;
    data[5] *= 1/sPI2F;
    data[6] = Doc->LastView.ZoomY;
    count = 7;
    break;
  case 3:  // pos - target - zoom - tilt
    data[0] = cam.l.x;
    data[1] = cam.l.y;
    data[2] = cam.l.z;
    data[3] = cam.l.x + cam.k.x;
    data[4] = cam.l.y + cam.k.y;
    data[5] = cam.l.z + cam.k.z;
    data[6] = Doc->LastView.ZoomY;
    {  // tilt is a bit harder
      sMatrix34 mat;
      mat.LookAt(cam.l+cam.k,cam.l);
      sF32 dot = sClamp<sF32>(mat.i^cam.j,-1,1);
      data[7] = sACos(dot)/sPI2F-0.25f;
      if(cam.j.y<0)
      {
        if(data[7]<0)
          data[7] = -0.5f-data[7];
        else
          data[7] = 0.5f-data[7];
      }
    }
    count = 8;
    break;
  }
  return count;
}


void Wz4SplineCed::DragCopyCam(const sWindowDrag &dd)
{
  if(dd.Mode==sDD_START)
  {
    sF32 data[8];
    sF32 time = Grid.XToV(dd.StartX);

    sInt count = GrabCam(data);

    if(count>0)
    {
      for(sInt i=0;i<count;i++)
      {
        if((ChannelMask & (1<<i)) && i<ChannelMax)
        {
          Wz4RenderArraySpline *key = new Wz4RenderArraySpline;
          key->Select = 0;
          key->Time = time;
          key->Value = data[i];
          key->Use = i;
          Op->ArrayData.AddTail(key);
        }
      }
      ChangeOp(Op);
      UpdateInfo();
    }
  }
}

void Wz4SplineCed::DragScissor(const sWindowDrag &dd,sDInt mode)
{
  if(dd.Mode==sDD_START)
  {
    Wz4RenderParaSpline *para = (Wz4RenderParaSpline *) Op->EditData;
    sF32 time = Grid.XToV(dd.StartX);

    CalcSpline cs;

    for(sInt i=0;i<CHANNELS;i++)
    {
      if(Curves[i].GetCount()>0)
      {
        if(mode==0 || (ChannelMask & (1<<i)))
        {
          cs.PrimeTime(time,Curves[i],para->Flags,para->MaxTime);
          Wz4RenderArraySpline *key = new Wz4RenderArraySpline;
          key->Select = 0;
          key->Time = time;
          key->Value = cs.CalcTime(time,para->Spline);
          key->Use = i;
          Op->ArrayData.AddTail(key);
        }
      }
    }
    ChangeOp(Op);
    UpdateInfo();
  }
}

void Wz4SplineCed::DragTime(const sWindowDrag &dd)
{
  switch(dd.Mode)
  {
  case sDD_START:
    App->TimelineWin->ScratchDragStart();
    break;
  case sDD_DRAG:
    if(Timeline1>Timeline0)
    {
      if(App->TimelineWin->ScratchDrag(sF32(Grid.XToV(dd.MouseX))*(Timeline1-Timeline0)+Timeline0))
        Update();
    }
    break;
  case sDD_STOP:
    App->TimelineWin->ScratchDragStop();
    break;
  }
}

/****************************************************************************/


void Wz4SplineCed::CmdSelectSameTime()
{
  Wz4RenderArraySpline *key;
  sF32 time;
  sF32 delta = 0.01f;
  for(sInt i=0;i<CHANNELS;i++)
  {
    if(ChannelMask & (1<<i))
    {
      sFORALL(Curves[i],key)
      {
        if(key->Select)
        {
          time = key->Time;
          goto found;
        }
      }
    }
  }
  return;

found:
  for(sInt i=0;i<CHANNELS;i++)
    if(ChannelMask & (1<<i))
      sFORALL(Curves[i],key)
        key->Select = key->Time > time-delta && key->Time < time+delta;
  Update();
  ChangeOp(Op);
}

void Wz4SplineCed::CmdQuantizeTime()
{
  Wz4RenderArraySpline *key;
  for(sInt i=0;i<CHANNELS;i++)
  {
    if(ChannelMask & (1<<i))
    {
      sFORALL(Curves[i],key)
        key->Time = sInt(key->Time*16+0.5f)/16.0f;
    }
  }
  Update();
  ChangeOp(Op);
}

void Wz4SplineCed::CmdZeroValue()
{
  Wz4RenderArraySpline *key;
  for(sInt i=0;i<CHANNELS;i++)
    if(ChannelMask & (1<<i))
      sFORALL(Curves[i],key)
        if(key->Select)
          key->Value = 0;
  Update();
  ChangeOp(Op);
}

void Wz4SplineCed::CmdAlignTime()
{
  Wz4RenderArraySpline *key;
  sF32 time = 0;
  sInt count = 0;
  for(sInt i=0;i<CHANNELS;i++)
  {
    if(ChannelMask & (1<<i))
    {
      sFORALL(Curves[i],key)
      {
        if(key->Select)
        {
          time += key->Time;
          count++;
        }
      }
    }
  }

  if(count>0)
  {
    time = time / count;

    for(sInt i=0;i<CHANNELS;i++)
      if(ChannelMask & (1<<i))
        sFORALL(Curves[i],key)
          if(key->Select)
            key->Time = time;
  }
  Sort();
  Update();
  ChangeOp(Op);
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   timeline editor                                                    ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

sInt Wz4TimelineCed::Time0 = 0;
sInt Wz4TimelineCed::Time1 = 0;
sInt Wz4TimelineCed::ScrollY = 0;

Wz4TimelineCed::Wz4TimelineCed(wOp *op)
{
  DragMode = 0;
  ToolMode = 0;
  LastTimePixel = -1;
  UpdateInfo();
  Height = sGui->PropFont->GetHeight()+6;

  DragStart0 = 0;
  DragStart1 = 0;
  DragStartY = 0;
  if(op)
  {
    Clip *clip = FindClip(op);
    if(clip)
    {
      ScrollTo(clip);
      SelectClip(clip);
    }
  }
}

Wz4TimelineCed::~Wz4TimelineCed()
{
}

void Wz4TimelineCed::Tag()
{
  Clip *c;
  sFORALL(Clips,c)
    c->Op->Need();
}

void Wz4TimelineCed::UpdateInfo()
{
  wOp *op;
  wClass *cl = Doc->FindClass(L"Clip",L"Wz4Render");
  wClass *cl2 = Doc->FindClass(L"ClipRandomizer",L"Wz4Render");
  wClass *mcl = Doc->FindClass(L"MultiClip",L"Wz4Render");

  Clips.Clear();
  sInt tmin = 0x40000000;
  sInt tmax = 0x00000000;
  if(cl)
  {
    sFORALL(Doc->AllOps,op)
    {
      if(op->Class==cl || op->Class==cl2 || op->Class==mcl)
      {
        sChar *s = op->Name;
        if(*s==';')
        {
          s++;
          while(*s==' ')
            s++;
        }
        if((op->Class == cl && (((Wz4RenderParaClip *)op->EditData)->Flags&0x0c)==0) || op->Class == cl2)
        {
          Clip *c = Clips.AddMany(1);
          Wz4RenderParaClip *para = (Wz4RenderParaClip *) op->EditData;
          c->Op = op;
          c->Index = -1;
          c->Start = para->StartTime*0x10000;
          c->Length = para->Length*0x10000;
          c->Line = para->Line;
          c->Select = 0;
          c->Name = s;
          c->Enable = para->Enable;
          c->Color = para->Color;

          if(c->Start<tmin) tmin = c->Start;
          if(c->Start+c->Length>tmax) tmax = c->Start+c->Length;
        }
        if(op->Class == mcl)
        {
          Wz4RenderParaMultiClip *para = (Wz4RenderParaMultiClip *) op->EditData;
          for(sInt i=0;i<op->ArrayData.GetCount();i++)
          {
            Clip *c = Clips.AddMany(1);
            Wz4RenderArrayMultiClip *data = (Wz4RenderArrayMultiClip *) op->ArrayData[i];
            c->Op = op;
            c->Index = i;
            c->Start = data->Start*0x10000;
            c->Length = data->Length*0x10000;
            c->Line = para->Line;
            c->Select = 0;
            c->Name = s;
            c->Enable = data->Enable && para->MasterEnable;
            c->Color = data->Color;
            if(c->Start<tmin) tmin = c->Start;
            if(c->Start+c->Length>tmax) tmax = c->Start+c->Length;
          }
        }
      }
    }
  }
  if(Time0==0 && Time1==0)
  {
    if(tmin<tmax)
    {
      Time0 = tmin;
      Time1 = tmax;
    }
    else
    {
      Time0 = 0;
      Time1 = 0x200000;
    }
  }

}

void Wz4TimelineCed::MakeRect(Clip *clip,sRect &r)
{
  sInt t0 = clip->Start;
  sInt t1 = (clip->Start+clip->Length);

  r.x0 = sMulDiv(t0 - Time0,Client.SizeX(),Time1-Time0)+Client.x0;
  r.x1 = sMulDiv(t1 - Time0,Client.SizeX(),Time1-Time0)+Client.x0;
  r.y0 = clip->Line*Height+Client.y0-ScrollY;
  r.y1 = r.y0 + Height;
}

Wz4TimelineCed::Clip *Wz4TimelineCed::Hit(sInt x,sInt y)
{
  Clip *c;
  sFORALL(Clips,c)
  {
    if(c->Rect.Hit(x,y))
      return c;
  }
  return 0;
}

sInt Wz4TimelineCed::XToS(sInt x)
{
  return sMulDiv(x-Time0,Client.SizeX(),Time1-Time0)+Client.x0;
}

sInt Wz4TimelineCed::XToV(sInt x)
{
  return sMulDiv(x-Client.x0,Time1-Time0,Client.SizeX())+Time0;
}

Wz4TimelineCed::Clip *Wz4TimelineCed::FindClip(wOp *op)
{
  Clip *clip;

  sFORALL(Clips,clip)
  {
    if(clip->Op==op)
      return clip;
  }
  return 0;
}

void Wz4TimelineCed::ScrollTo(Clip *clip)
{
  sInt zoom = Time1-Time0;
  if(clip->Start < Time0)
  {
    Time0 = clip->Start;
    Time1 = Time0+zoom;
  }
  else if(clip->Start+clip->Length > Time1)
  {
    Time1 = clip->Start+clip->Length;
    Time0 = Time1 - zoom;
  }
  if(clip->Line*Height < ScrollY)
    ScrollY = clip->Line*Height;
}

void Wz4TimelineCed::SelectClip(Clip *clip)
{
  Clip *c;
  sFORALL(Clips,c)
    c->Select = 0;
  clip->Select = 1;
  Update();
}

/****************************************************************************/

void Wz4TimelineCed::OnChangeOp()
{
  UpdateInfo();
  Update();
}


void Wz4TimelineCed::OnPaint2D(const sRect &client)
{
  Client = client;
  Inner = App->StackWin->Inner;

  Clip *clip;
  sRect r;
  sClipPush();

  sSortDown(Clips,&Clip::Line);

  sInt lastline = -1;
  sInt lasti = 0;

  sFORALL(Clips,clip)
  {
    if(lastline!=clip->Line)
    {
      lasti = _i;
      lastline = clip->Line;
    }
    sRect r0;
    MakeRect(clip,r0);
    for(sInt i=lasti;i<_i;i++)
    {
      sRect r1;
      MakeRect(&Clips[i],r1);
      if(r0.x1 > r1.x0 || r1.x1 > r0.x0)
      {
        sRect r;
        r.And(r0,r1);
        sRect2D(r,sGC_RED);
        sClipExclude(r);
      }
    }
  }


  sFORALL(Clips,clip)
  {
    MakeRect(clip,clip->Rect);

    sRect r(clip->Rect);
    sGui->PaintButtonBorder(r,clip->Select);
    r.x0 = sMax(r.x0,Inner.x0);
    r.x1 = sMin(r.x1,Inner.x1);

    if(clip->Enable)
    {
      // white|red|yellow|green|cyan|blue|pink|gray
      sU32 col = 0;
      switch(clip->Color)
      {
        case 0: // white
          col = 0xFFEFEFEF;
          break;
        case 1: // red
          col = 0xFFFFAAAA;
          break;
        case 2: // yellow
          col = 0xFFFFFF80;
          break;
        case 3: // green
          col = 0xFF80FF80;
          break;
        case 4: // cyan
          col = 0xFF80FFFF;
          break;
        case 5: // blue
          col = 0xFF8080FF;
          break;
        case 6: // pink
          col = 0xFFFF80FF;
          break;
        case 7: // gray
        default:
          col = 0xFFAAAAAA;
          break;
      }

      sSetColor2D(sGC_MAX,col);
      sGui->PropFont->SetColor(sGC_TEXT,clip->Select?sGC_PINK:sGC_MAX);
      sGui->PropFont->Print(sF2P_OPAQUE,r,clip->Name);
    }
    else
    {
      sString<128> buffer;
      buffer.PrintF(L"(%s)",clip->Name);
      sGui->PropFont->SetColor(sGC_LOW2,clip->Select?sGC_LOW:sGC_BUTTON);
      sGui->PropFont->Print(sF2P_OPAQUE,r,buffer);
    }
    sClipExclude(clip->Rect);
  }

  sSetColor2D(sGC_MAX+1,0xd0d0d0);
  sSetColor2D(sGC_MAX+0,0xa0a0a0);
  sSetColor2D(sGC_MAX+2,0x707070);
 
  sInt skip = 0x10000;
  sBool wide = 0x10000;
  if(XToS(skip)-XToS(0)<50) skip *= 4;
  if(XToS(skip)-XToS(0)<50) {skip *= 4; wide *= 4;}
  if(XToS(skip)-XToS(0)<50) skip *= 4;
  if(XToS(skip)-XToS(0)<50) skip *= 4;

  sInt t0 = Time0 & ~(wide-1);
  sInt t1 = (Time1 & ~(wide-1)) + wide;


  for(sInt i=t0;i<t1;i+=wide)
  {
    sBool thin = ((i&wide*3) ? 1 : 0);
    sInt x0 = XToS(i);
    sInt x1 = XToS(i+wide);
    sU32 pen = sGC_MAX + thin;
    if(i==0)
      pen = sGC_DRAW;
    sRect2D(x0,Client.y0,x0+1,Client.y1,pen);
    sRect2D(x0+1,Client.y0,x1,Client.y1,sGC_DOC);

    if((i & (skip-wide))==skip-wide)
    {
      sInt n = (i+wide-skip);
      sRect r(XToS(n),Client.y0,XToS(n+skip),Client.y1);
      sString<64> str;
      str.PrintF(L"%d",(n>>16));
      sGui->PropFont->SetColor(n&(1<<20)?sGC_MAX+2:sGC_TEXT,sGC_DOC);
      sGui->PropFont->Print(sF2P_BOTTOM|sF2P_LEFT,r,str);
      sGui->PropFont->Print(sF2P_TOP|sF2P_LEFT,r,str);
    }
  }

  sClipPop();

  // current time

  LastTimePixel = XToS(App->GetTimeBeats());
  sRect2D(LastTimePixel-1,Client.y0,LastTimePixel+1,Client.y1,sGC_RED);

  // drag rect

  if(DragMode==DM_FRAME || DragMode==DM_FRAMEADD)
    sRectFrame2D(DragRect,sGC_DRAW);
}

sBool Wz4TimelineCed::OnKey(sU32 key)
{
  key = (key & ~sKEYQ_MASK) | sMakeUnshiftedKey(key & sKEYQ_MASK);
  key &= ~sKEYQ_CAPS;
  if(key&sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
  if(key&sKEYQ_CTRL)  key |= sKEYQ_CTRL;
  if(key & sKEYQ_BREAK)
    ToolMode = 0;

  switch(key & (sKEYQ_BREAK|sKEYQ_MASK|sKEYQ_SHIFT|sKEYQ_CTRL))
  {
  default:
    return 0;
  case sKEY_BACKSPACE:
    CmdExit();
    break;
  case 'r':
    CmdReset();
    break;
  case 't':
    ToolMode = DT_SCRATCH;
    break;
  case 'g':
    CmdGoto();
    break;
  case 'm':
    CmdMarkTime();
    break;
  case 'e':
    CmdEnable();
    break;
  case sKEY_DELETE:
    CmdDeleteMulticlip();
    break;
  case 's':
    CmdShow();
    break;
  case 's'|sKEYQ_SHIFT:
    App->StackWin->CmdShowRoot(0);
    break;
  }
  return 1;
}

void Wz4TimelineCed::OnDrag(const sWindowDrag &dd,const sRect &client)
{
  Clip *clip;
  sU32 key;

  Client = client;

  if(dd.Mode==sDD_START)
  {
    sBool hit = 0;
    sFORALL(Clips,clip)
      if(clip->Rect.Hit(dd.MouseX,dd.MouseY))
        hit = 1;

    key = sGetKeyQualifier();
    key &= ~sKEYQ_CAPS;
    if(key & sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
    if(key & sKEYQ_CTRL)  key |= sKEYQ_CTRL;

    switch(ToolMode)
    {
    case 0:
      if(dd.Buttons==1)
      {
        if(key & sKEYQ_SHIFT)
        {
          if(!hit)
            DragMode = DM_FRAMEADD;
        }
        else if(key & sKEYQ_CTRL)
        {
        }
        else
        {
          if(!hit)
            DragMode = DM_FRAME;
          else
            DragMode = DM_HANDLE;
        }
      }
      if(dd.Buttons==2)
      {
        if(key & sKEYQ_CTRL)
          DragMode = DM_TIME;
        else if(key & sKEYQ_SHIFT)
          DragMode = DM_HANDLELEN;
        else
          CmdPopup();
      }
      if(dd.Buttons==4)
      {
        if(key & sKEYQ_SHIFT)
          DragMode = DM_ZOOM;
        else
          DragMode = DM_SCROLL;
      }
      break;

    case DT_SCRATCH:
      if(dd.Buttons==1)
        DragMode = DM_SCRATCHSLOW;
      if(dd.Buttons==2)
        DragMode = DM_SCRATCHFAST;
      if(dd.Buttons==4)
        DragMode = DM_TIME;
      break;
    }
  }

  switch(DragMode)
  {
  case DM_SCROLL:       DragScroll(dd); break;
  case DM_ZOOM:         DragZoom(dd); break;
  case DM_FRAME:        DragFrame(dd,0); break;
  case DM_FRAMEADD:     DragFrame(dd,1); break;
  case DM_HANDLE:       DragHandle(dd,3); break;
  case DM_HANDLELEN:    DragHandle(dd,4); break;

  case DM_SCRATCHSLOW:  App->TimelineWin->DragScratchIndirect(dd,0x100); break;
  case DM_SCRATCHFAST:  App->TimelineWin->DragScratchIndirect(dd,0x4000); break;
  case DM_TIME:         DragTime(dd);       break;
  }

  if(dd.Mode==sDD_STOP)
    DragMode = 0;
}

void Wz4TimelineCed::OnTime(sInt time)
{
  time = XToS(time);
  if(LastTimePixel!=time)
  {
    sInt x0 = sMin(time,LastTimePixel);
    sInt x1 = sMax(time,LastTimePixel);
    if(x0>x1) sSwap(x0,x1);
    x0--;
    x1++;
    sGui->Update(sRect(x0,Client.y0,x1,Client.y1));
  }
}

/****************************************************************************/

void Wz4TimelineCed::DragScroll(const sWindowDrag &dd)
{
  sInt v0,v1;
  switch(dd.Mode)
  {
  case sDD_START:
    DragStart0 = Time0;
    DragStart1 = Time1;
    DragStartY = ScrollY;
    break;
  case sDD_DRAG:
    ScrollY = sMax(0,DragStartY - dd.MouseY + dd.StartY);
    v0 = XToV(dd.StartX);
    v1 = XToV(dd.MouseX);
    Time0 = DragStart0 + v0 - v1;
    Time1 = DragStart1 + v0 - v1;
    Update();
    break;
  }
}

void Wz4TimelineCed::DragZoom(const sWindowDrag &dd)
{
  switch(dd.Mode)
  {
  case sDD_START:
    DragStart0 = Time0;
    DragStart1 = Time1;
    break;
  case sDD_DRAG:
    {
      sInt x = sMulDiv(dd.StartX-Client.x0,DragStart1-DragStart0,Client.SizeX())+DragStart0;
      sF32 f = sClamp<sF32>(sExp(-dd.DeltaX*0.01f),0.01f,100.0f);
      sInt xa = x-DragStart0;
      sInt xb = x-DragStart1;
      Time0 = x-sInt(xa*f);
      Time1 = x-sInt(xb*f);
    }
    Update();
    break;
  }
}

void Wz4TimelineCed::DragHandle(const sWindowDrag &dd,sDInt mode)
{
  Clip *clip,c;

  sInt dy = (dd.MouseY-dd.StartY+1000*Height+Height/2)/Height-1000;
  sInt dx = sMulDiv(dd.MouseX-dd.StartX,Time1-Time0,Client.SizeX());

  dx = (dx+0x8000) & 0xffff0000;
  sBool update = 0;

  switch(dd.Mode)
  {
  case sDD_START:
    clip = Hit(dd.StartX,dd.StartY);
    if(clip && !clip->Select)
    {
      Clip *c;
      sFORALL(Clips,c)
        c->Select = 0;
      clip->Select = 1;
      Update();
    }
    if(clip)
      App->EditOp(clip->Op,0);
    sFORALL(Clips,clip)
    {
      clip->DragStart = clip->Start;
      clip->DragLength = clip->Length;
      clip->DragLine = clip->Line;
    }
    break;
  case sDD_DRAG:
    sFORALL(Clips,clip)
    {
      if(clip->Select)
      {
        if(mode&1) clip->Start  = sClamp(clip->DragStart + dx,0,0x40000000);
        if(mode&2) clip->Line   = sClamp(clip->DragLine + dy,0,31);
        if(mode&4) clip->Length = sClamp(clip->DragLength + dx,0,0x40000000);

        if(clip->Index==-1)
        {
          Wz4RenderParaClip *para = (Wz4RenderParaClip *) clip->Op->EditData;
          para->Line = clip->Line;
          para->StartTime = clip->Start/65536.0f;
          para->Length = clip->Length/65536.0f;
          ChangeOp(clip->Op);
        }
        else
        {
          Wz4RenderParaMultiClip *para = (Wz4RenderParaMultiClip *) clip->Op->EditData;
          Wz4RenderArrayMultiClip *arr = (Wz4RenderArrayMultiClip *) clip->Op->ArrayData[clip->Index];

          if(para->Line!=clip->Line)
            update = 1;
          para->Line = clip->Line;
          arr->Start = clip->Start/65536.0f;
          arr->Length = clip->Length/65536.0f;
          ChangeOp(clip->Op);
        }
      }
    }
    if(update)
    {
      sFORALL(Clips,clip)
        if(clip->Index>=0)
          clip->Line = ((Wz4RenderParaMultiClip *) clip->Op->EditData)->Line;
    }
    Update();
    break;
  }
}

void Wz4TimelineCed::DragFrame(const sWindowDrag &dd,sDInt mode)
{
  Clip *clip;
  switch(dd.Mode)
  {
  case sDD_START:
  case sDD_DRAG:
    DragRect.Init(dd.MouseX,dd.MouseY,dd.StartX,dd.StartY);
    DragRect.Sort();
    Update();
    break;
  case sDD_STOP:
    sFORALL(Clips,clip)
    {
      if(mode==0)
        clip->Select = 0;
      clip->Select |= clip->Rect.IsInside(DragRect);
    }
    Update();
    break;
  }
}

void Wz4TimelineCed::DragTime(const sWindowDrag &dd)
{
  switch(dd.Mode)
  {
  case sDD_START:
    App->TimelineWin->ScratchDragStart();
    break;
  case sDD_DRAG:
    if(App->TimelineWin->ScratchDrag(XToV(dd.MouseX)))
      Update();
    break;
  case sDD_STOP:
    App->TimelineWin->ScratchDragStop();
    break;
  }
}

/****************************************************************************/

void Wz4TimelineCed::CmdPopup()
{
  sMenuFrame *mf = new sMenuFrame(App->CustomWin);
  mf->AddHeader(L"Actions",0);
  mf->AddHeader(L"Mouse",1);
  mf->AddHeader(L"Tools",3);

  mf->AddItem(L"Reset",sMessage(this,&Wz4TimelineCed::CmdReset),'r');
  mf->AddItem(L"Exit",sMessage(this,&Wz4TimelineCed::CmdExit),sKEY_BACKSPACE);
  mf->AddItem(L"Goto",sMessage(this,&Wz4TimelineCed::CmdGoto),'g');
  mf->AddItem(L"Mark Time",sMessage(this,&Wz4TimelineCed::CmdMarkTime),'m');
  mf->AddItem(L"Enable",sMessage(this,&Wz4TimelineCed::CmdEnable),'e');
  mf->AddItem(L"Delete Multiclip",sMessage(this,&Wz4TimelineCed::CmdDeleteMulticlip),sKEY_DELETE);
  mf->AddItem(L"Show Op",sMessage(this,&Wz4TimelineCed::CmdShow),'s');
  mf->AddItem(L"Show Root",sMessage(App->StackWin,&WinStack::CmdShowRoot,0),'S'|sKEYQ_SHIFT);

  mf->AddItem(L"ScratchTime",sMessage(),'t',-1,3,sGetColor2D(sGC_BUTTON));
  mf->AddItem(L"ScratchSlow",sMessage(),sKEY_LMB,-1,3);
  mf->AddItem(L"ScratchFast",sMessage(),sKEY_RMB,-1,3);
  mf->AddItem(L"Time",sMessage(),sKEY_MMB,-1,3);

  const sInt HIT = 0x01000000;
  const sInt MISS = 0x02000000;

  mf->AddItem(L"Move",sMessage(),sKEY_LMB|HIT,-1,1);
  mf->AddItem(L"Width",sMessage(),sKEY_RMB|sKEYQ_SHIFT|HIT,-1,1);
  mf->AddItem(L"Select Rect",sMessage(),sKEY_LMB|MISS,-1,1);
  mf->AddItem(L"Select Rect Add",sMessage(),sKEY_LMB|MISS|sKEYQ_SHIFT,-1,1);
  mf->AddItem(L"Scroll",sMessage(),sKEY_MMB,-1,1);
  mf->AddItem(L"Zoom",sMessage(),sKEY_MMB|sKEYQ_SHIFT,-1,1);
  mf->AddItem(L"Time",sMessage(),sKEY_RMB|sKEYQ_CTRL,-1,1);
  mf->AddItem(L"Context",sMessage(),sKEY_RMB,-1,1);

  sGui->AddPopupWindow(mf);
}

void Wz4TimelineCed::CmdReset()
{
  Time0 = 0;
  Time1 = 0;
  UpdateInfo();
  ScrollY = 0;
  Update();
}

void Wz4TimelineCed::CmdExit()
{
  App->PageListWin->SelectMsg.Send();
  sGui->SetFocus(App->StackWin);
}

void Wz4TimelineCed::CmdGoto()
{
  wOp *op = App->GetEditOp();
  if(op)
  {
    App->GotoOp(op);
  }
}

void Wz4TimelineCed::CmdMarkTime()
{
  wOp *op = App->GetEditOp();
  wClass *cl = Doc->FindClass(L"Clip",L"Wz4Render");
  wClass *cl2 = Doc->FindClass(L"ClipRandomizer",L"Wz4Render");
  if(op && (op->Class==cl || op->Class==cl2))
  {
    Wz4RenderParaClip *para = (Wz4RenderParaClip *)op->EditData;
    App->TimelineWin->LoopStart = sInt(para->StartTime*0x10000);
    App->TimelineWin->LoopEnd = sInt((para->StartTime + para->Length)*0x10000);
    App->TimelineWin->Update();
  }
}

void Wz4TimelineCed::CmdEnable()
{
  wClass *cl = Doc->FindClass(L"Clip",L"Wz4Render");
  wClass *cl2 = Doc->FindClass(L"ClipRandomizer",L"Wz4Render");
  wClass *mcl = Doc->FindClass(L"MultiClip",L"Wz4Render");
  Clip *clip;
  sFORALL(Clips,clip)
  {
    if(clip->Select && clip->Op)
    {
      if(clip->Op->Class == cl || clip->Op->Class == cl2)
      {
        Wz4RenderParaClip *para = (Wz4RenderParaClip *)clip->Op->EditData;
        para->Enable = !para->Enable;
        clip->Enable = para->Enable; 
        sGui->Notify(para->Enable);
        Doc->Change(clip->Op);
      }
      if(clip->Op->Class == mcl)
      {
        if(clip->Index>=0 && clip->Index<clip->Op->GetArrayCount())
        {
          Wz4RenderArrayMultiClip *a = clip->Op->GetArray<Wz4RenderArrayMultiClip>(clip->Index);
          a->Enable = !a->Enable;
          clip->Enable = a->Enable;
          sGui->Notify(a->Enable);
          Doc->Change(clip->Op);
        }
      }
    }
  }
  Update();
}

void Wz4TimelineCed::CmdDeleteMulticlip()
{
  wClass *mcl = Doc->FindClass(L"MultiClip",L"Wz4Render");
  Clip *clip;
  sFORALL(Clips,clip)
  {
    if(clip->Select && clip->Op)
    {
      if(clip->Op->Class == mcl)
      {
        if(clip->Index>=0 && clip->Index<clip->Op->GetArrayCount())
        {
          clip->Op->RemArray(clip->Index);
          Doc->Change(clip->Op);
        }
      }
    }
  }
  UpdateInfo();
  Update();
}

void Wz4TimelineCed::CmdShow()
{
  Clip *clip;
  sFORALL(Clips,clip)
  {
    if(clip->Select && clip->Op)
    {
      App->ShowOp(clip->Op,0);
      break;
    }
  }
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Beat                                                               ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

Wz4BeatCed::Beat::Beat(wOp *op)
{
  Op = op;
  Para = (Wz4RenderParaBeat *) op->EditData;
  Steps = op->ArrayData.GetCount()*16;
  Lookup = 0;
  UpdateLoop();
}

Wz4BeatCed::Beat::~Beat()
{
  delete[] Lookup;
}

void Wz4BeatCed::Beat::UpdateLoop()
{
  delete[] Lookup;
  Lookup = new sInt[Steps];

  sInt ls = 0;
  sInt le = 0;
  sInt loop = 0;
  sInt li = 0;

  for(sInt i=0;i<Steps;i++)
  {
    sInt byte = GetStepAbs(i);
    switch(byte & RNB_LoopMask)
    {
    case RNB_LoopStart:
      ls = i;
      loop = 0;
      break;
    case RNB_LoopEnd:
      le = i+1;
      loop = 1;
      li = ls;
      break;
    case RNB_LoopDone:
      loop = 0;
      break;
    }

    if(loop==2)
    {
      Lookup[i] = li++;
      if(li==le)
        li=ls;
    }
    else
    {
      Lookup[i] = i;
    }
    if(loop==1)
      loop=2;
  }
}

sU8 Wz4BeatCed::Beat::GetStepAbs(sInt i)
{
  if(i>=0 && i<Steps)
  {
    sU8 *bytes = (sU8 *)Op->ArrayData[i/16];
    return bytes[i&15];
  }
  else
  {
    return 0;
  }
}

void Wz4BeatCed::Beat::SetStepAbs(sInt i,sU8 byte)
{
  if(i>=0 && i<Steps)
  {
    sU8 *bytes = (sU8 *)Op->ArrayData[i/16];
    bytes[i&15] = byte;
  }
}

sU8 Wz4BeatCed::Beat::GetStep(sInt i)
{
  if(i>=0 && i<Steps)
  {
    return GetStepAbs(Lookup[i]);
  }
  else
  {
    return 0;
  }
}

void Wz4BeatCed::Beat::SetStep(sInt i,sU8 byte)
{
  if(i>=0 && i<Steps)
  {
    SetStepAbs(Lookup[i],byte);
  }
}

/****************************************************************************/


sInt Wz4BeatCed::Time0 = 0;
sInt Wz4BeatCed::Time1 = 0x20*0x10000;
sInt Wz4BeatCed::ScrollY = 0;

Wz4BeatCed::Wz4BeatCed(wOp *op)
{
  Client.Init();
  DragMode = 0;
  ToolMode = 0;
  DragRect.Init();
  DragStart0 = 0;
  DragStart1 = 0;
  LastTimePixel = 0;

  DragBeat = 0;
  DragBeatStart = 0;
  DragBeatPos = 0;
  DragSubMode = 0;

  WaveImg = 0;
  WaveTime0 = 0;
  WaveTime1 = 0;
  WaveHeight = 100;
  WaveBS = 0;
  WaveBPS = 0;
  Cursor = 0;
  CursorLength = 1;

  BeatNode = new RNBeat;
  EditOp = op;
  EditBeat = 0;

  UpdateInfo();
  UpdateSpline();
}

Wz4BeatCed::~Wz4BeatCed()
{
  delete WaveImg;
  delete BeatNode;
  sDeleteAll(Beats);
}

void Wz4BeatCed::Tag()
{
  wCustomEditor::Tag();
  
  Beat *b;
  sFORALL(Beats,b)
    b->Op->Need();

  EditOp->Need();
}


sInt Wz4BeatCed::XToS(sInt x)
{
  return sMulDiv(x-Time0,Client.SizeX(),Time1-Time0)+Client.x0;
}

sInt Wz4BeatCed::XToV(sInt x)
{
  return sMulDiv(x-Client.x0,Time1-Time0,Client.SizeX())+Time0;
}


void Wz4BeatCed::UpdateInfo()
{
  wOp *op;
  wClass *cl = Doc->FindClass(L"Beat",L"Wz4Render");

  sDeleteAll(Beats);
  if(cl)
  {
    sFORALL(Doc->AllOps,op)
    {
      if(op->Class == cl)
      {
        Beat *b = new Beat(op);
        Beats.AddTail(b);
      }
    }
  }
}

void Wz4BeatCed::UpdateSpline()
{
  if(EditOp)
  {
    sInt ac = EditOp->ArrayData.GetCount();
    sU8 *bytes = new sU8[ac*16];
    for(sInt i=0;i<ac;i++)
    {
      sCopyMem(bytes+i*16,EditOp->ArrayData[i],16);
    }
    BeatNode->Init((Wz4RenderParaBeat *)EditOp->EditData,ac,(Wz4RenderArrayBeat *)bytes,0);
    delete[] bytes;
  }
}

/****************************************************************************/

void Wz4BeatCed::OnPaint2D(const sRect &client)
{
  WaveRect = client;
  BeatRect = client;
  SplineRect = client;
  Client = client;

  sInt y = client.y0;
  WaveRect.y0   = y; y+=WaveHeight;
  WaveRect.y1   = y; y++;
  SplineRect.y0 = y; y+=WaveHeight;
  SplineRect.y1 = y; y++;
  BeatRect.y0   = y;

  sRect2D(Client.x0,WaveRect.y1,Client.x1,SplineRect.y0,sGC_BUTTON);
  sRect2D(Client.x0,SplineRect.y1,Client.x1,BeatRect.y0,sGC_BUTTON);

  PaintWave();
  PaintSpline();

  WaveImg->Paint(WaveRect.x0,WaveRect.y0);

  sClipPush();
  sClipRect(BeatRect);

  // paint ops

  y = BeatRect.y0;
  Beat *b;
  sFORALL(Beats,b)
  {
    sRect r;
    r.x0 = XToS(0);
    r.x1 = XToS((b->Steps>>b->Para->Tempo)<<16);
    r.y0 = y+5;
    r.y1 = y+25;
    y+=30;
    b->Client = r;

    sInt backcol = (b->Op==EditOp) ? sGC_SELECT : sGC_BACK;


    sInt step = 0x10000>>b->Para->Tempo;
    sInt mask = ~(step-1);
    sInt approxlen = XToS(step*16)-r.x0;
    sInt maxbeat = b->Steps*step;
    if(approxlen>5*16)
    {
      sInt t0 = sMax(0,Time0) & mask;
      sInt t1 = (sMin(Time1,maxbeat) & mask) + step;
      sRect2D(r.x0,r.y0,r.x1,r.y0+1,sGC_DRAW);
      sRect2D(r.x0,r.y1-1,r.x1,r.y1,sGC_DRAW);
      r.y0++;
      r.y1--;
      sInt n = t0/step;
    
      sGui->PropFont->SetColor(sGC_TEXT,backcol);
      sString<4> buffer;
      for(sInt i=t0;i<t1;i+=step)
      {
        r.x0 = XToS(i);
        r.x1 = XToS(i+step);
        sInt byte = b->GetStep(n);
        sInt val = sMin(b->Para->Levels,byte&RNB_LevelMask);
        sRect2D(r.x0,r.y0,r.x0+1,r.y1,sGC_DRAW);
        r.x0++;
        if(val>0)
        {
          if(b->Para->Levels<3)
          {
            sRect2D(r.x0,r.y0,r.x1,r.y1,val==1 ? sGC_YELLOW : sGC_RED);
          }
          else
          {
            sInt y = r.y1 - r.SizeY()*val/b->Para->Levels;
            sRect2D(r.x0,r.y0,r.x1,y,sGC_YELLOW);
            sRect2D(r.x0,y,r.x1,r.y1,sGC_RED);
            buffer.PrintF(L"%01x",val);
            sGui->PropFont->Print(0,r,buffer);
          }
        }
        else
        {
          sRect2D(r.x0,r.y0,r.x1,r.y1,(n&3)==0 ? sGC_BACK : backcol);
        }
        const sInt d = 3;
        switch(b->GetStepAbs(n) & RNB_LoopMask)
        {
        case RNB_LoopStart:
          sRect2D(r.x0,r.y0,r.x0+d,r.y1,sGC_GREEN);
          sRect2D(r.x0,r.y0,r.x1,r.y0+d,sGC_GREEN);
          sRect2D(r.x0,r.y1-d,r.x1,r.y1,sGC_GREEN);
          break;
        case RNB_LoopEnd:
          sRect2D(r.x1-d,r.y0,r.x1,r.y1,sGC_GREEN);
          sRect2D(r.x0,r.y0,r.x1,r.y0+d,sGC_GREEN);
          sRect2D(r.x0,r.y1-d,r.x1,r.y1,sGC_GREEN);
          break;
        case RNB_LoopDone:
          sRect2D(r.x0,r.y0,r.x0+d,r.y1,sGC_GREEN);
          break;
        }
        if(b->Op==EditOp && n>=Cursor && n<Cursor+CursorLength)
          sRectFrame2D(r.x0+1,r.y0+1,r.x1-1,r.y1-1,sGC_DRAW);
        n++;
      }
    }
    else
    {
      sRect r(b->Client);
      sRectFrame2D(r,sGC_DRAW);
      r.Extend(-1);
      sRect2D(r,backcol);
    }
    sClipExclude(b->Client);
  }

  // paint grid

  sInt t0 = Time0 & 0xffff0000;
  sInt t1 = (Time1 & 0xffff0000) + 0x00010000;

  sSetColor2D(sGC_MAX+1,0xd0d0d0);
  sSetColor2D(sGC_MAX+0,0xa0a0a0);
 
  sInt skip = 0x10000;
  if(XToS(skip)-XToS(0)<50) skip *= 4;
  if(XToS(skip)-XToS(0)<50) skip *= 4;
  if(XToS(skip)-XToS(0)<50) skip *= 4;
  sInt istep = XToS(0x10000)-XToS(0)<4 ? 0x40000:0x10000;

  for(sInt i=t0;i<t1;i+=istep)
  {
    sBool thin = ((i&0x00030000) ? 1 : 0);
    sInt x0 = XToS(i);
    sInt x1 = XToS(i+istep);
    sU32 pen = sGC_MAX + thin;
    if(i==0)
      pen = sGC_DRAW;
    sRect2D(x0,BeatRect.y0,x0+1,BeatRect.y1,pen);
    sRect2D(x0+1,BeatRect.y0,x1,BeatRect.y1,sGC_DOC);

    if((i & (skip-0x10000))==skip-0x10000)
    {
      sInt n = i+0x10000-skip;
      sRect r(XToS(n),BeatRect.y0,XToS(n+skip),BeatRect.y1);
      sString<64> str;
      str.PrintF(L"%d",n>>16);
      sGui->PropFont->Print(sF2P_BOTTOM|sF2P_LEFT,r,str);
    }
  }

  // no for the overlays:

  sClipPop();

  // current time

  LastTimePixel = XToS(App->GetTimeBeats());
  sRect2D(LastTimePixel-1,Client.y0,LastTimePixel+1,Client.y1,sGC_RED);

  // drag rect

  if(DragMode==DM_FRAME || DragMode==DM_FRAMEADD)
    sRectFrame2D(DragRect,sGC_DRAW);
}

void Wz4BeatCed::PaintWave()
{
  sInt xs = WaveRect.SizeX();
  sInt ys = WaveRect.SizeY();
  if(WaveImg==0 || 
     WaveTime0!=Time0 || 
     WaveTime1!=Time1 ||
     WaveBS!=Doc->DocOptions.BeatStart ||
     WaveBPS!=Doc->DocOptions.BeatsPerSecond ||
     WaveImg->GetSizeX()!=xs || 
     WaveImg->GetSizeY()!=ys  )
  {
    // remember this setting

    WaveTime0 = Time0;
    WaveTime1 = Time1;
    WaveBS = Doc->DocOptions.BeatStart;
    WaveBPS = Doc->DocOptions.BeatsPerSecond;

    // alloc temps

    sU32 *map = new sU32[xs*ys];
    sInt *minl = new sInt[xs];
    sInt *maxl = new sInt[xs];
    sInt *minr = new sInt[xs];
    sInt *maxr = new sInt[xs];

    // collect min/max

    if(App->MusicData)
    {
      sInt bps = Doc->DocOptions.BeatsPerSecond;
      sInt len = App->MusicSize;
      sInt x0 = WaveRect.x0;
      sInt bs = Doc->DocOptions.BeatStart;
      for(sInt i=0;i<xs;i++)
      {
        sInt b0 = sMulDiv(XToV(x0+i  )-bs,Doc->DocOptions.SampleRate,bps);
        sInt b1 = sMulDiv(XToV(x0+i+1)-bs,Doc->DocOptions.SampleRate,bps);
        
        if(b0<0 || b1>=len)
        {
          minl[i] = ys/2;
          maxl[i] = ys/2;
          minr[i] = ys/2;
          maxr[i] = ys/2;
        }
        else
        {
          sInt _minl,_maxl,_minr,_maxr;
          _minl = _maxl = App->MusicData[b0*2+0];
          _minr = _maxr = App->MusicData[b0*2+1];
          for(sInt j=b0+1;j<=b1;j++)
          {
            sInt sl = App->MusicData[j*2+0];
            sInt sr = App->MusicData[j*2+1];
            _minl = sMin(_minl,sl);
            _maxl = sMax(_maxl,sl);
            _minr = sMin(_minr,sr);
            _maxr = sMax(_maxr,sr);
          }
          minl[i] = (_minl+0x8000)*ys/0x10000;
          maxl[i] = (_maxl+0x8000)*ys/0x10000;
          minr[i] = (_minr+0x8000)*ys/0x10000;
          maxr[i] = (_maxr+0x8000)*ys/0x10000;
        }
      }
    }
    else
    {
      for(sInt i=0;i<xs;i++)
      {
        minl[i] = ys/2;
        maxl[i] = ys/2;
        minr[i] = ys/2;
        maxr[i] = ys/2;
      }
    }

    // write bitmap. this is a cache-friendly 90° rotation!

    for(sInt y=0;y<ys;y++)
    {
      for(sInt x=0;x<xs;x++)
      {
        sU32 col = 0xff000000;
        if(y>=minl[x] && y<=maxl[x])
          col |= 0x00ff0000;
        if(y>=minr[x] && y<=maxr[x])
          col |= 0x0000ff00;
        map[y*xs+x] = col;
      }
    }

    // update bitmap

    if(WaveImg && WaveImg->GetSizeX()==xs && WaveImg->GetSizeY()==ys)
    {
      WaveImg->Update(map);
    }
    else
    {
      delete WaveImg;
      WaveImg = new sImage2D(xs,ys,map);
    }

    // delete temps

    delete[] minl;
    delete[] maxl;
    delete[] minr;
    delete[] maxr;
    delete[] map;
  }
}

void Wz4BeatCed::PaintSpline()
{
  sRect2D(SplineRect,sGC_BLACK);


  ScriptSpline *sp = BeatNode->Spline;
  if(sp)
  {
    const ScriptSplineKey *keys = sp->Curves[0]->GetData();
    sInt max = sp->Curves[0]->GetCount();
    sF32 ampi = 1.0f/(BeatNode->Amp+BeatNode->Bias);
    if(max>2)
    {
      sInt ys = -(SplineRect.SizeY()-8);
      sInt yo = SplineRect.y1-4;

      for(sInt i=1;i<max;i++)
      {
        sF32 x0 = keys[i-1].Time;
        sF32 x1 = keys[i].Time;
        if(x0<Time1/65536.0f && x1>Time0/65536.0f)
        {
          sF32 y0 = 0;
          sp->Eval(x0,&y0,1);
          if(sp->Mode>1 && x0!=x1)
          {
            sF32 xa = x0;
            const sInt n = 16;
            for(sInt j=0;j<n;j++)
            {
              sF32 xb = x0+(x1-x0)*(j+1)/n;
              sF32 y1;
              sp->Eval(xb,&y1,1);
              sLine2D(XToS(xa*0x10000),y0*ampi*ys+yo,
                      XToS(xb*0x10000),y1*ampi*ys+yo,sGC_WHITE);
              xa = xb;
              y0 = y1;
            }
          }
          else
          {       
            sF32 y1 = keys[i].Value;
            sLine2D(XToS(x0*0x10000),y0*ampi*ys+yo,
                    XToS(x1*0x10000),y1*ampi*ys+yo,sGC_WHITE);
          }
        }
      }
    }
  }
}

void Wz4BeatCed::OnDrag(const sWindowDrag &dd,const sRect &client)
{
  sU32 key;
  Beat *b;


  if(dd.Mode==sDD_START)
  {
    sBool hit = 0;
    sFORALL(Beats,b)
      if(b->Client.Hit(dd.MouseX,dd.MouseY))
        hit = 1;

    key = sGetKeyQualifier();
    key &= ~sKEYQ_CAPS;
    if(key & sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
    if(key & sKEYQ_CTRL)  key |= sKEYQ_CTRL;

    switch(ToolMode)
    {
    case 0:
      if(dd.Buttons==1)
      {
        if(key & sKEYQ_SHIFT)
        {
          if(!hit)
            DragMode = DM_FRAMEADD;
          else
            DragMode = DM_SET;
        }
        else if(key & sKEYQ_CTRL)
        {
          if(hit)
            DragMode = DM_ACCENT;
        }
        else
        {
          if(!hit)
            DragMode = DM_FRAME;
          else
            DragMode = DM_SELECT;
        }
      }
      if(dd.Buttons==2)
      {
        if(key & sKEYQ_CTRL)
          DragMode = DM_TIME;
        else
          CmdPopup();
      }
      if(dd.Buttons==4)
      {
        if(key & sKEYQ_SHIFT)
          DragMode = DM_ZOOM;
        else
          DragMode = DM_SCROLL;
      }
      break;

    case DT_SCRATCH:
      if(dd.Buttons==1)
        DragMode = DM_SCRATCHSLOW;
      if(dd.Buttons==2)
        DragMode = DM_SCRATCHFAST;
      if(dd.Buttons==4)
        DragMode = DM_TIME;
      break;
    }
  }

  switch(DragMode)
  {
  case DM_SCROLL:       DragScroll(dd); break;
  case DM_ZOOM:         DragZoom(dd); break;
//  case DM_FRAME:        DragFrame(dd,0); break;
//  case DM_FRAMEADD:     DragFrame(dd,1); break;
  case DM_SCRATCHSLOW:  App->TimelineWin->DragScratchIndirect(dd,0x100); break;
  case DM_SCRATCHFAST:  App->TimelineWin->DragScratchIndirect(dd,0x4000); break;
  case DM_TIME:         DragTime(dd);       break;

  case DM_SELECT:       DragSelect(dd,0); break;
  case DM_SET:          DragSelect(dd,1); break;
  case DM_ACCENT:       DragSelect(dd,2); break;
  }

  if(dd.Mode==sDD_STOP)
    DragMode = 0;
}

void Wz4BeatCed::OnChangeOp()
{
  UpdateInfo();
  UpdateSpline();
  Update();
}

sBool Wz4BeatCed::OnKey(sU32 key)
{
  key = (key & ~sKEYQ_MASK) | sMakeUnshiftedKey(key & sKEYQ_MASK);
  key &= ~sKEYQ_CAPS;
  if(key&sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
  if(key&sKEYQ_CTRL)  key |= sKEYQ_CTRL;
  if(key & sKEYQ_BREAK)
    ToolMode = 0;

  switch(key & (sKEYQ_BREAK|sKEYQ_MASK))
  {
  default:
    return 0;
  case sKEY_BACKSPACE:
    CmdExit();
    break;
  case 'r':
    CmdReset();
    break;
  case 't':
    ToolMode = DT_SCRATCH;
    break;

  case 's':
    CmdLoop(RNB_LoopStart);
    break;
  case 'e':
    CmdLoop(RNB_LoopEnd);
    break;
  case 'd':
    CmdLoop(RNB_LoopDone);
    break;

  case 'x':
    CmdCut();
    break;
  case 'c':
    CmdCopy();
    break;
  case 'v':
    CmdPaste();
    break;

  case sKEY_LEFT:
    CmdCursor(-1);
    break;
  case sKEY_RIGHT:
    CmdCursor(1);
    break;

  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    CmdSetLevel(key&15);
    break;
  }
  return 1;
}

void Wz4BeatCed::OnTime(sInt time)
{
  time = XToS(time);
  if(LastTimePixel!=time)
  {
    sInt x0 = sMin(time,LastTimePixel);
    sInt x1 = sMax(time,LastTimePixel);
    if(x0>x1) sSwap(x0,x1);
    x0--;
    x1++;
    sRect r(x0,Client.y0,x1,Client.y1);
    if(r.IsInside(Client))
      sGui->Update(r);
    LastTimePixel = time;
  }
}

/****************************************************************************/

void Wz4BeatCed::DragScroll(const sWindowDrag &dd)
{
  sInt v0,v1;
  switch(dd.Mode)
  {
  case sDD_START:
    DragStart0 = Time0;
    DragStart1 = Time1;
    DragStartY = ScrollY;
    break;
  case sDD_DRAG:
    ScrollY = sMax(0,DragStartY - dd.MouseY + dd.StartY);
    v0 = XToV(dd.StartX);
    v1 = XToV(dd.MouseX);
    Time0 = DragStart0 + v0 - v1;
    Time1 = DragStart1 + v0 - v1;
    Update();
    break;
  }
}

void Wz4BeatCed::DragZoom(const sWindowDrag &dd)
{
  switch(dd.Mode)
  {
  case sDD_START:
    DragStart0 = Time0;
    DragStart1 = Time1;
    break;
  case sDD_DRAG:
    {
      sInt x = sMulDiv(dd.StartX-Client.x0,DragStart1-DragStart0,Client.SizeX())+DragStart0;
      sF32 f = sClamp<sF32>(sExp(-dd.DeltaX*0.01f),0.01f,100.0f);
      sInt xa = x-DragStart0;
      sInt xb = x-DragStart1;
      Time0 = x-sInt(xa*f);
      Time1 = x-sInt(xb*f);
    }
    Update();
    break;
  }
}

void Wz4BeatCed::DragFrame(const sWindowDrag &dd,sDInt mode)
{
  switch(dd.Mode)
  {
  case sDD_START:
  case sDD_DRAG:
    DragRect.Init(dd.MouseX,dd.MouseY,dd.StartX,dd.StartY);
    DragRect.Sort();
    Update();
    break;
  case sDD_STOP:
/*
    sFORALL(Clips,clip)
    {
      if(mode==0)
        clip->Select = 0;
      clip->Select |= clip->Rect.IsInside(DragRect);
    }
    */
    Update();
    break;
  }
}

void Wz4BeatCed::DragTime(const sWindowDrag &dd)
{
  switch(dd.Mode)
  {
  case sDD_START:
    App->TimelineWin->ScratchDragStart();
    break;
  case sDD_DRAG:
    if(App->TimelineWin->ScratchDrag(XToV(dd.MouseX)))
      Update();
    break;
  case sDD_STOP:
    App->TimelineWin->ScratchDragStop();
    break;
  }
}

void Wz4BeatCed::DragSelect(const sWindowDrag &dd,sDInt mode)
{
  if(dd.Mode==sDD_START)
  {
    Beat *hit,*b;
    hit = 0;
    DragSubMode = 0;
    DragBeat = 0;
    DragBeatPos = 0;
    sFORALL(Beats,b)
    {
      if(b->Client.Hit(dd.MouseX,dd.MouseY))
      {
        hit = b;
        break;
      }
    }
    if(hit)
    {
      sInt beat = XToV(dd.MouseX);
      sInt pos = beat>>(16-hit->Para->Tempo);
      if(pos>=0 && pos<hit->Steps && mode>0)
      {
        sInt val = 1;
        if(hit->Para->Levels==2 && mode==2)
          val = 2;
        if(hit->Para->Levels>2)
          val = hit->Para->Levels;
        if(hit->Para->Levels<=2 || mode==1)
        {
          sInt byte = hit->GetStep(pos);
          if((byte&RNB_LevelMask)==val)
            byte = byte & ~RNB_LevelMask;
          else
            byte = (byte & ~RNB_LevelMask) | val;
          hit->SetStep(pos,byte);
          Update();
          Doc->Change(hit->Op);
          App->ChangeDoc();
        }
        else
        {
          DragSubMode = 1;
        }
      }
      if(mode==0)
        DragSubMode = 2;

      DragBeat = hit;
      DragBeatPos = pos;
      DragBeatStart = hit->GetStep(pos);
      Cursor = pos;
      CursorLength = 1;
      App->EditOp(hit->Op,0);
      EditOp = hit->Op;
      EditBeat = hit;
      UpdateSpline();
    }
  }
  if(dd.Mode==sDD_DRAG && DragBeat)
  {
    if(DragSubMode == 1)
    {
      sInt oldval = DragBeatStart & RNB_LevelMask;
      sInt val = sClamp(oldval - ((dd.DeltaY+500)/5-100),0,DragBeat->Para->Levels);
      if(val!=(DragBeat->GetStep(DragBeatPos) & RNB_LevelMask))
      {
        DragBeat->SetStep(DragBeatPos,(DragBeatStart & ~RNB_LevelMask) | val);
        UpdateSpline();
        Update();
        Doc->Change(DragBeat->Op);
        App->ChangeDoc();
      }
    }
    else if(DragSubMode==2)
    {
      sInt beat = XToV(dd.MouseX);
      sInt pos = beat>>(16-DragBeat->Para->Tempo);
      sInt oc = Cursor;
      sInt ol = CursorLength;
      if(pos>DragBeatPos)
      {
        Cursor = DragBeatPos;
        CursorLength = pos-DragBeatPos+1;
      }
      else
      {
        Cursor = pos;
        CursorLength = DragBeatPos-pos+1;
      }
      if(oc!=Cursor || ol!=CursorLength)
        Update();
    }
  }
  if(dd.Mode==sDD_STOP)
  {
    DragBeat = 0;
  }
}

/****************************************************************************/

void Wz4BeatCed::CmdPopup()
{
  sMenuFrame *mf = new sMenuFrame(App->CustomWin);
  mf->AddHeader(L"Actions",0);
  mf->AddHeader(L"Mouse",1);
  mf->AddHeader(L"Extra",2);
  mf->AddHeader(L"Tools",3);

  mf->AddItem(L"Reset",sMessage(this,&Wz4BeatCed::CmdReset),'r');
  mf->AddItem(L"Exit",sMessage(this,&Wz4BeatCed::CmdExit),sKEY_BACKSPACE);
  mf->AddItem(L"Loop Start",sMessage(this,&Wz4BeatCed::CmdReset),'s');
  mf->AddItem(L"Loop End",sMessage(this,&Wz4BeatCed::CmdReset),'e');
  mf->AddItem(L"Loop Done",sMessage(this,&Wz4BeatCed::CmdReset),'d');

  mf->AddItem(L"Cut",sMessage(this,&Wz4BeatCed::CmdCut),'x');
  mf->AddItem(L"Copy",sMessage(this,&Wz4BeatCed::CmdCopy),'c');
  mf->AddItem(L"Paste",sMessage(this,&Wz4BeatCed::CmdPaste),'v');
  mf->AddItem(L"CursorLeft",sMessage(this,&Wz4BeatCed::CmdCursor,-1),sKEY_LEFT);
  mf->AddItem(L"CursorRight",sMessage(this,&Wz4BeatCed::CmdCursor,1),sKEY_RIGHT);
  mf->AddItem(L"Set0",sMessage(this,&Wz4BeatCed::CmdSetLevel,0),'0',2);
  mf->AddItem(L"Set1",sMessage(this,&Wz4BeatCed::CmdSetLevel,1),'1',2);
  mf->AddItem(L"Set2",sMessage(this,&Wz4BeatCed::CmdSetLevel,2),'2',2);
  mf->AddItem(L"Set3",sMessage(this,&Wz4BeatCed::CmdSetLevel,3),'3',2);
  mf->AddItem(L"Set4",sMessage(this,&Wz4BeatCed::CmdSetLevel,4),'4',2);
  mf->AddItem(L"Set5",sMessage(this,&Wz4BeatCed::CmdSetLevel,5),'5',2);
  mf->AddItem(L"Set6",sMessage(this,&Wz4BeatCed::CmdSetLevel,6),'6',2);
  mf->AddItem(L"Set7",sMessage(this,&Wz4BeatCed::CmdSetLevel,7),'7',2);
  mf->AddItem(L"Set8",sMessage(this,&Wz4BeatCed::CmdSetLevel,8),'8',2);
  mf->AddItem(L"Set9",sMessage(this,&Wz4BeatCed::CmdSetLevel,9),'9',2);


  mf->AddItem(L"ScratchTime",sMessage(),'t',-1,3,sGetColor2D(sGC_BUTTON));
  mf->AddItem(L"ScratchSlow",sMessage(),sKEY_LMB,-1,3);
  mf->AddItem(L"ScratchFast",sMessage(),sKEY_RMB,-1,3);
  mf->AddItem(L"Time",sMessage(),sKEY_MMB,-1,3);

//  const sInt HIT = 0x01000000;
//  const sInt MISS = 0x02000000;

  mf->AddItem(L"Select",sMessage(),sKEY_LMB,-1,1);
  mf->AddItem(L"Set",sMessage(),sKEY_LMB|sKEYQ_SHIFT,-1,1);
  mf->AddItem(L"Accent",sMessage(),sKEY_LMB|sKEYQ_CTRL,-1,1);
//  mf->AddItem(L"Select Rect",sMessage(),sKEY_LMB|MISS,-1,1);
//  mf->AddItem(L"Select Rect Add",sMessage(),sKEY_LMB|MISS|sKEYQ_SHIFT,-1,1);
  mf->AddItem(L"Scroll",sMessage(),sKEY_MMB,-1,1);
  mf->AddItem(L"Zoom",sMessage(),sKEY_MMB|sKEYQ_SHIFT,-1,1);
  mf->AddItem(L"Time",sMessage(),sKEY_RMB|sKEYQ_CTRL,-1,1);
  mf->AddItem(L"Context",sMessage(),sKEY_RMB,-1,1);

  sGui->AddPopupWindow(mf);
}

void Wz4BeatCed::CmdReset()
{
  Time0 = 0;
  Time1 = 0x20*0x10000;
  ScrollY = 0;
  Update();
}

void Wz4BeatCed::CmdExit()
{
  App->PageListWin->SelectMsg.Send();
  sGui->SetFocus(App->StackWin);
}

void Wz4BeatCed::CmdLoop(sDInt n)
{
  sInt pos = Cursor;
  if(n==RNB_LoopEnd)
    pos = Cursor+CursorLength-1;
  if(EditBeat && pos>=0 && pos<EditBeat->Steps)
  {
    sInt byte = EditBeat->GetStepAbs(pos);
    sInt val = byte & ~RNB_LoopMask;
    if((byte & RNB_LoopMask) == n)
      EditBeat->SetStepAbs(pos,val);
    else
      EditBeat->SetStepAbs(pos,val|n);

    EditBeat->UpdateLoop();
    UpdateSpline();
    Update();
    ChangeOp(EditBeat->Op);
  }
}

void Wz4BeatCed::CmdCut()
{
  if(EditBeat && Cursor>=0 && Cursor+CursorLength<EditBeat->Steps)
  {
    CmdCopy();

    for(sInt i=Cursor;i<Cursor+CursorLength;i++)
      EditBeat->SetStep(i,EditBeat->GetStep(i)&~RNB_LevelMask);
    ChangeOp(EditBeat->Op);
  }
}

void Wz4BeatCed::CmdCopy()
{
  if(EditBeat && Cursor>=0 && Cursor+CursorLength<EditBeat->Steps)
  {
    Clipboard.Resize(CursorLength);
    for(sInt i=0;i<CursorLength;i++)
    {
      Clipboard[i] = EditBeat->GetStep(i+Cursor)&RNB_LevelMask;
    }
  }
}

void Wz4BeatCed::CmdPaste()
{
  if(EditBeat && Cursor>=0)
  {
    for(sInt i=0;i<Clipboard.GetCount() && i+Cursor<EditBeat->Steps;i++)
    {
      EditBeat->SetStep(i+Cursor,(EditBeat->GetStep(i+Cursor)&~RNB_LevelMask)|Clipboard[i]);
    }
    ChangeOp(EditBeat->Op);
  }
}

void Wz4BeatCed::CmdCursor(sDInt dx)
{
  if(EditBeat)
  {
    Cursor = sClamp(Cursor+sInt(dx),0,EditBeat->Steps-1);
    ChangeOp(EditBeat->Op);
  }
}

void Wz4BeatCed::CmdSetLevel(sDInt val)
{
  if(EditBeat && Cursor>=0 && Cursor<EditBeat->Steps && val<=EditBeat->Para->Levels)
  {
    CursorLength = 1;
    EditBeat->SetStep(Cursor,(EditBeat->GetStep(Cursor)&~RNB_LevelMask)|val);
    ChangeOp(EditBeat->Op);
  }
}

/****************************************************************************/

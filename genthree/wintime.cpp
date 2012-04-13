// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "wintime.hpp"

#include "winpage.hpp"
#include "winpara.hpp"
#include "winview.hpp"
#include "genmisc.hpp"

#define BLOCKONE 20

#define DM_TIME       1
#define DM_ZOOM       2
#define DM_SCROLL     3
#define DM_ADD        4
#define DM_SELECT     5
#define DM_MOVE       6
#define DM_RECT       7
#define DM_WIDTH      8
#define DM_DUP        9
//#define DM_CONNECT    10

#define CMD_CUT       0x0100
#define CMD_COPY      0x0101
#define CMD_PASTE     0x0102
#define CMD_DELETE    0x0103
#define CMD_POPUP     0x0104
#define CMD_ADD       0x0105
#define CMD_QUANTPOP  0x0106
#define CMD_QUANT     0x0107
#define CMD_QUANTSET  0x01c0


/****************************************************************************/


struct sString2SetEntry
{
  sChar *String;
  sObject *Object;
};

struct sString2Set
{
private:
  sInt FindNr(sChar *string);
public:
  sArray<sString2SetEntry>  Set;

  void Init();                              // create
  void Exit();                              // destroy
  void Clear();
  sObject *Find(sChar *string);             // find object set for string
  sBool Add(sChar *string,sObject *);       // add object/string pair. return sTRUE if already in list. overwites
};


sInt sString2Set::FindNr(sChar *string)
{
  sInt i;

  for(i=0;i<Set.Count;i++)
    if(sCmpString(Set[i].String,string)==0)
      return i;
  return -1;
}

void sString2Set::Init()
{
  Set.Init(16);
  Set.Count = 0;
}

void sString2Set::Exit()
{
  Set.Exit();
}

void sString2Set::Clear()
{
  Set.Count = 0;
}

sObject *sString2Set::Find(sChar *string)
{
  sInt i;

  i = FindNr(string);
  if(i!=-1)
    return Set[i].Object;
  else
    return 0;
}

sBool sString2Set::Add(sChar *string,sObject *obj)
{
  sInt i;

  i = FindNr(string);
  if(i!=-1)
  {
    Set[i].Object = obj;
    return sTRUE;
  }
  else
  {
    i = Set.Count++;
    Set.AtLeast(i+1);
    Set[i].String = string;
    Set[i].Object = obj;
    return sFALSE;
  }
}

/****************************************************************************/
/****************************************************************************/

TimeOp::TimeOp()
{
  Clear();
}

TimeOp::~TimeOp()
{
}

void TimeOp::Clear()
{
  x0 = 0;
  x1 = 16;
  y0 = 0;
  Type = TOT_EVENT;
  Select = 0;

  Velo = 0;
  Mod = 0;
  sSetMem(SRT,0,9*4);
  sCopyString(Text,"new",sizeof(Text));

  Anim = 0;
  Temp = 0;
}

void TimeOp::Tag()
{
  sBroker->Need(Anim);
}

sBool TimeOp::Inside(TimeOp *op)
{
  if(op->x0 >= x1) return sFALSE;
  if(op->x1 <  x0) return sFALSE;
  if(op->y0 == y0) return sFALSE;
  return sTRUE;
}

void TimeOp::Copy(sObject *o)
{
  TimeOp *ot;

  sVERIFY(o->GetClass()==sCID_TOOL_TIMEOP);
  ot = (TimeOp *) o;
  
  x0 = ot->x0;
  x1 = ot->x1;
  y0 = ot->y0;
  Type = ot->Type;
  Select = ot->Select;

  Velo = ot->Velo;
  Mod = ot->Mod;
  sCopyMem(SRT,ot->SRT,9*4);
  sCopyString(Text,ot->Text,sizeof(Text));

  Temp = ot->Temp;
  Anim = ot->Anim;
}

/****************************************************************************/

/****************************************************************************/
/****************************************************************************/

TimeDoc::TimeDoc()
{
  Ops = new sList<TimeOp>;
  BeatMax = 0x02000000;
}

TimeDoc::~TimeDoc()
{
}

void TimeDoc::Clear()
{
  Ops->Clear();
  BeatMax = 0x02000000;
}

void TimeDoc::Tag()
{
  ToolDoc::Tag();
  sBroker->Need(Ops);
}

sBool TimeDoc::Write(sU32 *&data)
{
  sU32 *hdr;
  sInt i,max,j;
  TimeOp *top;
  sList<AnimDoc> *anims;
  AnimDoc *anim;

  anims = new sList<AnimDoc>;

  max = Ops->GetCount();
  for(i=0;i<max;i++)
  {
    top = Ops->Get(i);
    top->Temp = i;
    if(top->Anim)
      top->Anim->Temp=-1;
  }
  j = 0;
  for(i=0;i<max;i++)
  {
    top = Ops->Get(i);
    if(top->Anim && top->Anim->Temp==-1)
    {
      top->Anim->Temp = j++;
      anims->Add(top->Anim);
    }
  }


  hdr = sWriteBegin(data,sCID_TOOL_TIMEDOC,6);
  *data++ = max;
  *data++ = anims->GetCount();
  *data++ = BeatMax;
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;

  for(i=0;i<anims->GetCount();i++)
  {
    anim = anims->Get(i);
    sVERIFY(anim->Temp == i);
    
    *data++ = anim->Channels.Count;
    *data++ = anim->Keys.Count;
    *data++ = anim->PaintRoot;
    *data++ = 0;
    sWriteString(data,anim->RootName);
    for(j=0;j<anim->Channels.Count;j++)
    {
      *data++ = anim->Channels[j].Offset;
      *data++ = anim->Channels[j].Count;
      *data++ = anim->Channels[j].Mode;
      *data++ = anim->Channels[j].Tension;
      *data++ = anim->Channels[j].Continuity;
      *data++ = anim->Channels[j].Bias;
      *data++ = 0;
      *data++ = 0;
      sWriteString(data,anim->Channels[j].Name);
    }
    for(j=0;j<anim->Keys.Count;j++)
    {
      *data++ = anim->Keys[j].Time;
      *data++ = anim->Keys[j].Channel;
      *data++ = anim->Keys[j].Value[0];
      *data++ = anim->Keys[j].Value[1];
      *data++ = anim->Keys[j].Value[2];
      *data++ = anim->Keys[j].Value[3];
    }
  }

  for(i=0;i<max;i++)
  {
    top = Ops->Get(i);

    *data++ = top->Type;
    *data++ = top->Anim ? top->Anim->Temp : -1;
    *data++ = top->x0;
    *data++ = top->x1;
    *data++ = top->y0;
    *data++ = top->Velo;
    *data++ = top->Mod;
    sCopyMem(data,top->SRT,9*4); data+=9;
    *data++ = 0;
    *data++ = 0;
    *data++ = 0;
    *data++ = 0;
    sWriteString(data,top->Text);
  }

  sWriteEnd(data,hdr);
  return sTRUE;
}

sBool TimeDoc::Read(sU32 *&data)
{
  sInt i,max,j,maxanim,maxc,maxk;
  TimeOp *top;
  sList<AnimDoc> *anims;
  AnimDoc *anim;
  sInt version;

  Clear();

  version = sReadBegin(data,sCID_TOOL_TIMEDOC);
  if(version<5 || version>7) return sFALSE;
  max = *data++;
  maxanim = *data++;
  BeatMax = *data++;
  BeatMax = 0x02000000;
  data+=5;

  anims = new sList<AnimDoc>;

  for(i=0;i<max;i++)
    Ops->Add(new TimeOp);
  for(i=0;i<maxanim;i++)
  {
    anim = new AnimDoc;
    anim->App = App;
    anims->Add(anim);
  }
  for(i=0;i<maxanim;i++)
  {
    anim = anims->Get(i);
    maxc = *data++;
    maxk = *data++;
    anim->PaintRoot = *data++;
    data+=1;
    if(version>=6)
      sReadString(data,anim->RootName,sizeof(anim->RootName));

    anim->Channels.AtLeast(maxc);
    anim->Channels.Count = maxc;
    anim->Keys.AtLeast(maxk);
    anim->Keys.Count = maxk;

    for(j=0;j<anim->Channels.Count;j++)
    {
      anim->Channels[j].Init();
      anim->Channels[j].Target = 0;
      anim->Channels[j].Offset = *data++;
      anim->Channels[j].Count = *data++;
      anim->Channels[j].Mode = *data++;
      anim->Channels[j].Tension = *data++;
      anim->Channels[j].Continuity = *data++;
      anim->Channels[j].Bias = *data++;
      anim->Channels[j].Select[0] = 0;
      anim->Channels[j].Select[1] = 0;
      anim->Channels[j].Select[2] = 0;
      anim->Channels[j].Select[3] = 0;
      data+=2;
      sReadString(data,anim->Channels[j].Name,sizeof(anim->Channels[j].Name));
    }
    for(j=0;j<anim->Keys.Count;j++)
    {
      anim->Keys[j].Init();
      anim->Keys[j].Time = *data++;
      anim->Keys[j].Channel = *data++;
      anim->Keys[j].Value[0] = *data++;
      anim->Keys[j].Value[1] = *data++;
      anim->Keys[j].Value[2] = *data++;
      anim->Keys[j].Value[3] = *data++;
    }

    anim->Update();
  }

  for(i=0;i<max;i++)
  {
    top = Ops->Get(i);
    top->Select = 0;

    top->Type = *data++;
    j = *data++;
    top->Anim = (j==-1) ? 0 : anims->Get(j);
    top->x0 = *data++;
    top->x1 = *data++;
    top->y0 = *data++;
    top->Velo = *data++;
    top->Mod = *data++;
    sCopyMem(top->SRT,data,9*4); data+=9;
    data+=4;
    sReadString(data,top->Text,sizeof(top->Text));
  }

  return sReadEnd(data);
}

/****************************************************************************/

void TimeDoc::SortOps()
{
  sInt i,j,max;

  max = Ops->GetCount();
  for(i=1;i<max;i++)
  {
    for(j=i;j>0;j--)
      if(Ops->Get(j)->y0 < Ops->Get(j-1)->y0)
        Ops->Swap(j,j-1);
      else
        break;
  }
}

/****************************************************************************/
/****************************************************************************/

TimeWin::TimeWin()
{
  Doc = 0;
  BeatZoom = 0x40000;
  AddScrolling();
  DragKey = DM_SELECT;
  StickyKey = 0;
  Level = 0;
  Current = 0;
  BeatQuant = 0x10000;
  LastTime = 0;
}

TimeWin::~TimeWin()
{
}

void TimeWin::Tag()
{
  ToolWindow::Tag();
  sBroker->Need(Doc);
  sBroker->Need(Current);
}

void TimeWin::SetDoc(ToolDoc *doc) 
{
  if(doc)
    sVERIFY(doc->GetClass()==sCID_TOOL_TIMEDOC);
  Doc = (TimeDoc *)doc;
  Current = 0;
}

sBool TimeWin::MakeRect(TimeOp *top,sRect &r)
{
  r.x0 = Client.x0 + (sMulShift(top->x0,BeatZoom)>>16);
  r.x1 = Client.x0 + (sMulShift(top->x1,BeatZoom)>>16);
  r.y0 = Client.y0 + top->y0*BLOCKONE;
  r.y1 = Client.y0 + (top->y0+1)*BLOCKONE;
  return sTRUE;
}

sBool TimeWin::MakeRectMove(TimeOp *top,sRect &r)
{
  sInt m;

  m = 1;
  if(DragMode == DM_WIDTH)
    m = 0;
  r.x0 = Client.x0 + (sMulShift((top->x0+DragMoveX*m),BeatZoom)>>16);
  r.x1 = Client.x0 + (sMulShift((top->x1+DragMoveX),BeatZoom)>>16);
  r.y0 = Client.y0 + (top->y0+DragMoveY*m)*BLOCKONE;
  r.y1 = Client.y0 + (top->y0+1+DragMoveY*m)*BLOCKONE;
  return sTRUE;
}

void TimeWin::ClipCut()
{
  ClipCopy();
  ClipDelete();
}

void TimeWin::ClipCopy()
{
}

void TimeWin::ClipPaste()
{
}

void TimeWin::ClipDelete()
{
  sInt i;
  TimeOp *top;

  if(!Doc) return;

  for(i=0;i<Doc->Ops->GetCount();)
  {
    top = Doc->Ops->Get(i);
    if(top->Select)
      Doc->Ops->Rem(top);
    else
      i++;
  }
}

void TimeWin::Quant()
{
  sInt i,max;
  TimeOp *top;

  if(!Doc) return;

  max = Doc->Ops->GetCount();
  for(i=0;i<max;i++)
  {
    top = Doc->Ops->Get(i);
    if(top->Select)
    {
      top->x0 = (top->x0+BeatQuant/2)&~(BeatQuant-1);
      top->x1 = (top->x1+BeatQuant/2)&~(BeatQuant-1);
    }
  }
}

/****************************************************************************/

void TimeWin::OnKey(sU32 key)
{
  switch(key&(0x8001ffff|sKEYQ_REPEAT))
  {
  case 't':
    StickyKey = DragKey;
    DragKey = DM_TIME;
    break;
  case 'z':
    StickyKey = DragKey;
    DragKey = DM_ZOOM;
    break;
  case 'y':
    StickyKey = DragKey;
    DragKey = DM_SCROLL;
    break;
  case ' ':
    StickyKey = DragKey;
    DragKey = DM_SELECT;
    break;
  case 'm':
    StickyKey = DragKey;
    DragKey = DM_MOVE;
    break;
  case 'r':
    StickyKey = DragKey;
    DragKey = DM_RECT;
    break;
  case 'w':
    StickyKey = DragKey;
    DragKey = DM_WIDTH;
    break;
  case 'd':
    StickyKey = DragKey;
    DragKey = DM_DUP;
    break;
/*
  case 'e':
    StickyKey = DragKey;
    DragKey = DM_CONNECT;
    break;
    */

  case 't'|sKEYQ_BREAK:
  case 'z'|sKEYQ_BREAK:
  case 'y'|sKEYQ_BREAK:
  case ' '|sKEYQ_BREAK:
  case 'm'|sKEYQ_BREAK:
  case 'r'|sKEYQ_BREAK:
  case 'w'|sKEYQ_BREAK:
  case 'd'|sKEYQ_BREAK:
    DragKey = StickyKey;
    StickyKey = DM_SELECT;
    break;

  case sKEY_BACKSPACE:
  case sKEY_DELETE:
    OnCommand(CMD_DELETE);
    break;
  case 'x':
    OnCommand(CMD_CUT);
    break;
  case 'c':
    OnCommand(CMD_COPY);
    break;
  case 'v':
    OnCommand(CMD_PASTE);
    break;
  case 'a':
    OnCommand(CMD_ADD);
    break;
  case 'q':
    OnCommand(CMD_QUANT);
    break;
  }
  /*
  if((key&sKEYQ_STICKYBREAK) && StickyKey)
    DragKey = StickyKey;
  if((key&sKEYQ_BREAK) && ((((key&0x1ffff)>='a' && (key&0x1ffff)<='z')) || (key&0x1ffff)==' '))
    StickyKey = 0;
    */
//  sDPrintF("key %08x\n",key);
}

sBool TimeWin::OnCommand(sU32 cmd)
{
  sMenuFrame *mf;
  switch(cmd)
  {
  case CMD_POPUP:
    mf = new sMenuFrame;
    mf->AddMenu("Add",CMD_ADD,'a');
    mf->AddSpacer();
    mf->AddMenu("Normal Mode",' ',' ');
    mf->AddMenu("Scratch Time",'t','t');
    mf->AddMenu("Zoom",'z','z');
    mf->AddMenu("Scroll",'y','y');
    mf->AddMenu("Move",'m','m');
    mf->AddMenu("Rect",'r','r');
    mf->AddMenu("Width",'w','w');
    mf->AddMenu("Duplicate",'d','d');
    mf->AddMenu("Connect",'e','e');
    mf->AddSpacer();
    mf->AddMenu("Delete",CMD_DELETE,sKEY_DELETE);
    mf->AddMenu("Cut",CMD_CUT,'x');
    mf->AddMenu("Copy",CMD_COPY,'c');
    mf->AddMenu("Paste",CMD_PASTE,'v');
    mf->AddSpacer();
    mf->AddMenu("Quant",CMD_QUANT,'q');
    mf->AddMenu("Set Quant Step",CMD_QUANTPOP,0);
    mf->AddBorder(new sNiceBorder);
    mf->SendTo = this;
    sGui->AddPopup(mf);
    return sTRUE;
  case CMD_QUANTPOP:
    mf = new sMenuFrame;
    mf->AddMenu("Add",CMD_ADD,'a');
    mf->AddSpacer();
    mf->AddMenu("$40000 (full bar)"       ,CMD_QUANTSET+0,0);
    mf->AddMenu("$10000 (1/4 note, beat)" ,CMD_QUANTSET+1,0);
    mf->AddMenu("$01000 (1/16 note)"      ,CMD_QUANTSET+2,0);
    mf->AddMenu("$00001 (off)"            ,CMD_QUANTSET+3,0);
    mf->AddBorder(new sNiceBorder);
    mf->SendTo = this;
    sGui->AddPopup(mf);
    return sTRUE;
  case CMD_QUANT:
    Quant();
    return sTRUE;
  case CMD_QUANTSET+0:
    BeatQuant = 0x40000;
    return sTRUE;
  case CMD_QUANTSET+1:
    BeatQuant = 0x10000;
    return sTRUE;
  case CMD_QUANTSET+2:
    BeatQuant = 0x01000;
    return sTRUE;
  case CMD_QUANTSET+3:
    BeatQuant = 0x00001;
    return sTRUE;

  case 't':
  case 'z':
  case 'y':
  case ' ':
  case 'm':
  case 'r':
  case 'w':
  case 'd':
  case 'e':
    OnKey(cmd);
    return sTRUE;
  case CMD_ADD:
    if(Doc)
    {
      sInt x;
      TimeOp *top;

      top = new TimeOp;
      x=(LastTime+BeatQuant/2)&~(BeatQuant-1);
      top->Anim = new AnimDoc;
      top->Anim->App = App;
      top->x0 = x;
      top->x1 = x+0x100000;
      top->y0 = LastLine;
      top->Type = TOT_MESH;
      top->Select = 1;
      sCopyString(top->Anim->RootName,"test",sizeof(top->Anim->RootName));
      Doc->Ops->Add(top);
    }
    return sTRUE;
  case CMD_DELETE:
    ClipDelete();
    return sTRUE;
  case CMD_CUT:
    ClipCut();
    return sTRUE;
  case CMD_COPY:
    ClipCopy();
    return sTRUE;
  case CMD_PASTE:
    ClipPaste();
    return sTRUE;
  default:
    return sFALSE;
  }
}

void TimeWin::OnPaint()
{
  sInt i,x,max,w,skip;
  sRect r;
  sChar buffer[128];
  TimeOp *top;
  sU32 col,col1;
  sU32 kinds[2][4] =
  {
    { 0xff000000,0xffffffc0,0xffffc0ff,0xffc0ffff },
    { 0xff000000,0xffc0c000,0xffc000c0,0xff00c0c0 }
  };
  static sChar *dragmodes[] = { "???","time","zoom","scroll","add","select","move","rect","width","dup","connect" };

  if(Flags&sGWF_CHILDFOCUS)
    App->SetStat(0,dragmodes[DragKey]);

  if(Doc==0)
  {
    sPainter->Paint(sGui->FlatMat,Client,sGui->Palette[sGC_BACK]);
    return;
  }

  col = sGui->Palette[sGC_BACK];
  sPainter->Paint(sGui->FlatMat,Client,col);
  skip = 4;
  if(BeatZoom>0x40000)
    skip = 1;
  for(i=0;i<Doc->BeatMax>>16;i+=skip)
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

  x = (sMulShift(App->Player->BeatNow,BeatZoom)>>16)+Client.x0;
  sPainter->Paint(sGui->FlatMat,x,Client.y0,2,Client.YSize(),0xffff0000);

  max = Doc->Ops->GetCount();
/*
  if(DragKey == DM_CONNECT)
  {
    for(i=0;i<max;i++)
      Doc->Ops->Get(i)->Temp = 0;
    if(Current)
      for(i=0;i<Current->Connect->GetCount();i++)
        Current->Connect->Get(i)->Temp = 1;
  }
*/
  for(i=0;i<max;i++)
  {
    top = Doc->Ops->Get(i);
    if(MakeRect(top,r))
    {
      col = sGui->Palette[sGC_BUTTON];
      if(Current && (top==Current || (top->Anim && Current->Anim && top->Anim==Current->Anim)))
        col = sGui->Palette[sGC_SELBACK];
/*
      if(DragKey == DM_CONNECT)
      {
        if(top == Current)
          col = kinds[1][top->Type];
        else if(top->Temp==0)
          col = sGui->Palette[sGC_BUTTON];
      }
*/
      sGui->Button(r,top->Select,top->Anim && top->Anim->Name[0] ? top->Anim->Name : top->Anim->RootName,0,col);
    }
  }


  w = 0;
  if(DragMode==DM_MOVE || DragMode==DM_WIDTH)
    w = 1;
  if(DragMode==DM_DUP)
    w = 2;
  if(w)
  {
    for(i=0;i<max;i++)
    {
      top = Doc->Ops->Get(i);
      if(top->Select)
      {
        if(MakeRectMove(top,r))
          sGui->RectHL(r,w,0xff000000,0xff000000);
      }
    }
  }

  if(DragMode == DM_RECT)
  {
    r = DragFrame;
    r.Sort();
    sGui->RectHL(r,1,~0,~0);
  }

  SizeX = sMulShift(Doc->BeatMax,BeatZoom)>>16;
  if(SizeX<BLOCKONE)
    SizeX = BLOCKONE;
  SizeY = BLOCKONE*16;
}

void TimeWin::OnDrag(sDragData &dd)
{
  sInt x,y,z;
  sInt i,max;
  sBool ok;
  TimeOp *top,*st;
  sRect r;
  sU32 keystate;
  ParaWin *parawin;
  AnimWin *animwin;

  if(MMBScrolling(dd,DragStartX,DragStartY))
    return;
  if(!Doc)
    return;

  LastTime = x = sDivShift((dd.MouseX-Client.x0)<<16,BeatZoom);
  LastLine = y = (dd.MouseY-Client.y0)/BLOCKONE;
  z = 0;
  keystate = sSystem->GetKeyboardShiftState();
  max = Doc->Ops->GetCount();


  switch(dd.Mode)
  {
  case sDD_START:

    if(dd.Buttons&2)
    {
      DragMode = 0;
      OnCommand(CMD_POPUP);
      return;
    }
    DragMode = DragKey;
    DragMoveX = DragMoveY = DragMoveZ = 0;
    switch(DragMode)
    {
    case DM_ZOOM:
      DragStartX = BeatZoom;
      break;
    case DM_SCROLL:
      DragStartX = ScrollX;
      DragStartY = ScrollY;
      break;

    case DM_SELECT:
      ok = sFALSE;
      st = 0;
      for(i=0;i<max;i++)
      {
        top = Doc->Ops->Get(i);
        if(MakeRect(top,r) && r.Hit(dd.MouseX,dd.MouseY))
        {
          if(keystate & sKEYQ_CTRL)
            top->Select = !top->Select;
          else
            top->Select = 1;
          if(!(keystate & (sKEYQ_SHIFT|sKEYQ_CTRL)))
          {
            if(st)
            {
              if(st->x1-st->x0 > top->x1-top->x0)
              {
                st->Select = 0;
                st = top;
              }
              else
                top->Select = 0;
            }
            else
            {
              st = top;
            }
          }
          ok = sTRUE;
        }
        else
        {
          if(!(keystate & (sKEYQ_SHIFT|sKEYQ_CTRL)))
            top->Select = 0;
        }
      }
      if(st)
      {
        parawin = (ParaWin *) App->FindActiveWindow(sCID_TOOL_PARAWIN);
        if(parawin)
          parawin->SetTime(st,Doc);
        if(st->Anim)
        {
          animwin = (AnimWin *) App->FindActiveWindow(sCID_TOOL_ANIMWIN);
          if(animwin)
            animwin->SetOp(st);
        }
      }
      Current = st;
      if(!ok)
      {
        DragMode = DM_RECT;
        DragFrame.Init(dd.MouseX,dd.MouseY,dd.MouseX,dd.MouseY);
      }
      break;
    case DM_RECT:
      DragFrame.Init(dd.MouseX,dd.MouseY,dd.MouseX,dd.MouseY);
      break;
/*
    case DM_CONNECT:
      st = 0;
      for(i=0;i<max && !st;i++)
      {
        top = Doc->Ops->Get(i);
        if(MakeRect(top,r) && r.Hit(dd.MouseX,dd.MouseY))
          st = top;
      }
      if(st)
      {
        if(Current)
        {
          if(Current==st)
          {
            Current = 0;
          }
          else
          {
            if(Current->Connect->IsInside(st))
              Current->Connect->Rem(st);
            else
              Current->Connect->Add(st);
          }
        }
        else
        {
          Current = st;
        }
      }
      break;
*/
    }
    break;
  case sDD_DRAG:
    switch(DragMode)
    {
    case DM_ZOOM:
      BeatZoom = sRange<sInt>(DragStartX*(1.0f+(dd.DeltaX-dd.DeltaY)*0.002),0x00400000,0x00010000);
      break;
    case DM_SCROLL:      
      ScrollX = sRange<sInt>(DragStartX-dd.DeltaX,SizeX-RealX,0);
      ScrollY = sRange<sInt>(DragStartY-dd.DeltaY,SizeY-RealY,0);
      break;
    case DM_RECT:
      DragFrame.x1 = dd.MouseX;
      DragFrame.y1 = dd.MouseY;
      break;
    case DM_SELECT:
      if(sAbs(dd.DeltaX)+sAbs(dd.DeltaY)>4)
        DragMode = DM_MOVE;
      break;
    case DM_MOVE:
    case DM_DUP:
      DragMoveX = sDivShift(dd.DeltaX<<16,BeatZoom);
      DragMoveY = (dd.DeltaY+BLOCKONE*256+BLOCKONE/2)/BLOCKONE-256;
      DragMoveZ = 0;

      DragMoveX = (DragMoveX+BeatQuant/2)&~(BeatQuant-1);
      break;
    case DM_WIDTH:
      DragMoveX = sDivShift(dd.DeltaX<<16,BeatZoom);
      DragMoveY = 0;//(dd.DeltaY+BLOCKONE*256+BLOCKONE/2)/BLOCKONE-256;
      DragMoveZ = 0;

      DragMoveX = (DragMoveX+BeatQuant/2)&~(BeatQuant-1);
      break;

    case DM_TIME:
      if(x<0)
        x = 0;
      App->Player->BeatNow = x;
      App->Player->TimeNow = sMulDiv(App->Player->BeatNow,App->Player->TimeSpeed,0x10000);
      App->Player->LastTime = App->Player->TimeNow;
      break;
    }
    break;
  case sDD_STOP:
    switch(DragMode)
    {
    case DM_RECT:
      DragFrame.Sort();
      for(i=0;i<max;i++)
      {
        top = Doc->Ops->Get(i);
        if(MakeRect(top,r) && r.Hit(DragFrame) && !DragFrame.Inside(r))
        {
          if(keystate & sKEYQ_CTRL)
            top->Select = !top->Select;
          else
            top->Select = 1;
        }
        else
        {
          if(!(keystate & (sKEYQ_SHIFT|sKEYQ_CTRL)))
            top->Select = 0;
        }
      }
      break;

    case DM_MOVE:
    case DM_DUP:
      ok = sTRUE;
      for(i=0;i<max;i++)
      {
        top = Doc->Ops->Get(i);
        if(top->Select)
        {
          if(top->x0+DragMoveX<0)
            ok = sFALSE;
          if(top->y0+DragMoveY<0)
            ok = sFALSE;
        }
      }
      if(!ok) break;
      for(i=0;i<max;i++)
      {
        top = Doc->Ops->Get(i);
        if(top->Select)
        {
          if(DragMode==DM_MOVE)
          {
            st = top;
          }
          else 
          {
            st = new TimeOp;
            st->Copy(top);
            Doc->Ops->Add(st);
          }
          st->x0 += DragMoveX;
          st->x1 += DragMoveX;
          st->y0 += DragMoveY;
        }
      }
      break;

    case DM_WIDTH:
      for(i=0;i<max;i++)
      {
        top = Doc->Ops->Get(i);
        if(top->Select)
        {
          if(top->x1+DragMoveX > top->x0)
            top->x1 += DragMoveX;
        }
      }
      break;
    }
    DragMode = 0;
    break;
  }
}

/****************************************************************************/
/****************************************************************************/

void AnimKey::Init()
{
  sSetMem(this,0,sizeof(*this));
}

void AnimChannel::Init()
{
  sSetMem(this,0,sizeof(*this));
  Count = 1;
  Select[0] = 1;
  Select[1] = 1;
  Select[2] = 1;
  Select[3] = 1;
}

/****************************************************************************/

AnimDoc::AnimDoc()
{
  Channels.Init(16);
  Keys.Init(32);

  Channels.Count=1;
  Channels[0].Init();
  Keys.Count = 2;
  Keys[0].Init();
  Keys[1].Init(); Keys[1].Time = 0x10000;

  Temp = 0;
  RootName[0] = 0;
  Root = 0;
  PaintRoot = 1;
}

AnimDoc::~AnimDoc()
{
  Channels.Exit();
  Keys.Exit();
}

void AnimDoc::Tag()
{
  sInt i;

  ToolDoc::Tag();
  sBroker->Need(Root);
  for(i=0;i<Channels.Count;i++)
    sBroker->Need(Channels[i].Target);
}

void AnimDoc::Update()
{
  sInt i,j;
  AnimChannel *c;
  ToolObject *tool;

  for(i=0;i<Channels.Count;i++)
    Channels[i].Target = 0;

  for(i=0;i<Keys.Count-1;i++)
  {
    for(j=i+1;j<Keys.Count;j++)
    {
      if(Keys[i].Time+Keys[i].Channel*0x20000 > Keys[j].Time+Keys[j].Channel*0x20000)
        sSwap(Keys[i],Keys[j]);
    }
  }

  tool = App->FindObject(RootName);
  if(!tool || !tool->Cache) return;
  Root = tool;

  for(i=0;i<Channels.Count;i++)
  {
    c= &Channels[i];
    c->Target = FindLabel(Root->Cache,App->CodeGen.MakeLabel(c->Name)<<16);
    if(c->Target)
      c->Count = OffsetSize(c->Target->GetClass(),c->Offset);
    if(c->Count==0)
      c->Count = 1;
  }
}

void AnimDoc::AddChannel()
{
  AnimChannel *c;
  AnimKey *key;
  sInt nr;

  nr = Channels.Count;
  if(nr==0)
  {
    c = Channels.Add();
    c->Init();
  }
  else
  {
    c = Channels.Add();
    *c = Channels[nr-1];
  }

  key = Keys.Add();
  key->Init();
  key->Channel = nr;
  key = Keys.Add();
  key->Init();
  key->Channel = nr;
  key->Time = 0x10000;

  Update();
}

void AnimDoc::RemChannel(sInt nr)
{
  sInt i;

  if(nr<Channels.Count)
  {
    Channels.Count--;
    for(i=nr;i<Channels.Count;i++)
      Channels[i] = Channels[i+1];
    for(i=0;i<Keys.Count;)
    {
      if(Keys[i].Channel==nr)
        Keys[i] = Keys[--Keys.Count];
      else
        i++;
    } 
    for(i=0;i<Keys.Count;i++)
      if(Keys[i].Channel>nr)
        Keys[i].Channel--;
  }
  Update();
}

void AnimDoc::AddKey(sInt time)
{
  sInt i,j;
  sBool sel;
  AnimChannel *chan;
  AnimKey *key;

  Update();

  for(i=0;i<Channels.Count;i++)
  {
    chan = &Channels[i];
    sel = 0;
    for(j=0;j<chan->Count;j++)
      sel |= chan->Select[j];
    if(sel)
    {
      key = Keys.Add();
      key->Init();
      key->Channel = i;
      key->Time = time;
    }

  }

  Update();
}

/****************************************************************************/

AnimWin::AnimWin()
{
  Doc = 0;
  TOp = 0;
  DragKey = 0;
  StickyKey = 0;
  DragMode = 0;

  ValMid = 0;
  ValAmp = 0x40000;
  TimeZoom = 512;

  AddScrolling(1,0);
  Spline = new GenSpline;
}

AnimWin::~AnimWin()
{
}

void AnimWin::Tag()
{
  ToolWindow::Tag();
  sBroker->Need(Doc);
  sBroker->Need(Spline);
}

void AnimWin::SetDoc(ToolDoc *doc)
{
  if(doc)
    sVERIFY(doc->GetClass()==sCID_TOOL_ANIMDOC);
  Doc = (AnimDoc *)doc;
  TOp = 0;
}

void AnimWin::SetOp(TimeOp *top)
{
  TOp = top;
  Doc = top->Anim;
}

/****************************************************************************/

void AnimWin::Time2Pos(sInt &x,sInt &y)
{
  x = Client.x0+4 + sMulShift(x,TimeZoom);
  y = (Client.y0+Client.y1)/2 - sMulDiv(y-ValMid,Client.YSize(),ValAmp);
}

void AnimWin::Pos2Time(sInt &x,sInt &y)
{
  x = sMulDiv(x-Client.x0-4,0x10000,TimeZoom);
  y = sMulDiv(y+(Client.y0+Client.y1)/2,ValAmp,Client.YSize())+ValMid;
}

void AnimWin::DPos2Time(sInt &x,sInt &y)
{
  x = sMulDiv(x,0x10000,TimeZoom);
  y = sMulDiv(-y,ValAmp,Client.YSize());
}

void AnimWin::OnPaint()
{
  sU32 col,col1;
  sInt i,j,k,x,y,x0,x1,y0,y1;
  sU32 pfx,pfy;
  //sInt last,xl[4],yl[4];
  sF32 val0[4],val1[4];
  sRect r;
  AnimKey *key;
  GenSplineKey *sk;
  AnimChannel *chan;
  static sChar *dragmodes[] = { "???","time","zoom","scroll","add","select","move","rect","width","dup","connect" };
  static sU32 rgba[4] = { 0xffff0000,0xff00ff00,0xff0000ff,0xffffffff };

  if(Flags&sGWF_CHILDFOCUS)
    App->SetStat(0,dragmodes[DragKey]);

  if(Doc==0)
  {
    sPainter->Paint(sGui->FlatMat,Client,sGui->Palette[sGC_BACK]);
    return;
  }

  col = sGui->Palette[sGC_BACK];
  sPainter->Paint(sGui->FlatMat,Client,col);
  for(i=0;i<=16;i++)
  {
    x = i<<12; y=0; Time2Pos(x,y);
    col1 = col-0x181818;
    if((i&15)==0)
      col1 = ((col>>1)&0x7f7f7f7f)|0xff000000;
    sPainter->Paint(sGui->FlatMat,x,Client.y0,1,Client.YSize(),col1);
  }
  for(i=-16;i<=16;i++)
  {
    x0 = 0; Time2Pos(x0,y); 
    x1 = 0x10000; Time2Pos(x1,y);
    x = 0; y=i<<16; Time2Pos(x,y);
    col1 = col-0x181818;
    if(i==0)
      col1 = ((col>>1)&0x7f7f7f7f)|0xff000000;
    sPainter->Paint(sGui->FlatMat,x0,y,x1-x0,1,col1);
  }

  x = App->Player->BeatNow;
  if(TOp && x>=TOp->x0 && x<TOp->x1)
  {
    x = sDivShift(x-TOp->x0,TOp->x1-TOp->x0);
    y = 0;
    Time2Pos(x,y);
    sPainter->Paint(sGui->FlatMat,x,Client.y0,2,Client.YSize(),0xffff0000);
  }

  pfx = pfy = 0;
  for(i=0;i<Doc->Keys.Count;i++)
  {
    key = &Doc->Keys[i];
    for(j=0;j<Doc->Channels[key->Channel].Count;j++)
    {
      if(Doc->Channels[key->Channel].Select[j])
      {
        x=key->Time; y=key->Value[j]; Time2Pos(x,y);
        r.Init(x-3,y-3,x+4,y+4);
        if(key->Select[j])
        {
          pfx = key->Value[j];
          pfy = key->Time;
          sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_SELECT]);
        }
        sGui->RectHL(r,1,sGui->Palette[sGC_DRAW],sGui->Palette[sGC_DRAW]);
      }
    }
  }

  static sChar buffer[128];
  sSPrintF(buffer,sizeof(buffer),"%08x %h %h",App->Player->BeatNow,pfx,pfy);
  App->SetStat(1,buffer);

  for(i=0;i<Doc->Channels.Count;i++)
  {
    chan = &Doc->Channels[i];
    Spline->Clear();
    Spline->Dimensions = chan->Count;
    Spline->Tension = chan->Tension/65536.0f;
    Spline->Continuity = chan->Continuity/65536.0f;
    Spline->Bias = chan->Bias/65536.0f;

    for(j=0;j<Doc->Keys.Count;j++)
    {
      key = &Doc->Keys[j];
      if(key->Channel==i)
      {
        sk = Spline->Keys.Add();
        sk->Time = key->Time/65536.0f;
        sk->Value[0] = key->Value[0]/65536.0f;
        sk->Value[1] = key->Value[1]/65536.0f;
        sk->Value[2] = key->Value[2]/65536.0f;
        sk->Value[3] = key->Value[3]/65536.0f;
      }
    }

    Spline->Sort();
    Spline->Calc(0,val0);

    for(j=1;j<=64;j++)
    {
      Spline->Calc(j/64.0f,val1);
      for(k=0;k<chan->Count;k++)
      {
        if(chan->Select[k])
        {
          x0 = (j-1)*0x10000/64;
          y0 = sFtol(val0[k]*0x10000);
          x1 = j*0x10000/64;
          y1 = sFtol(val1[k]*0x10000);
          Time2Pos(x0,y0);
          Time2Pos(x1,y1);

          sPainter->Line(x0,y0,x1,y1,rgba[k]);    
          val0[k] = val1[k];
        }
      }
    }
  }

  SizeX = TimeZoom+8;
}

void AnimWin::OnKey(sU32 key)
{
  switch(key&(0x8001ffff|sKEYQ_REPEAT))
  {
  case 't':
    StickyKey = DragKey;
    DragKey = DM_TIME;
    break;
  case 'z':
    StickyKey = DragKey;
    DragKey = DM_ZOOM;
    break;
  case 'y':
    StickyKey = DragKey;
    DragKey = DM_SCROLL;
    break;
  case ' ':
    StickyKey = DragKey;
    DragKey = DM_SELECT;
    break;
  case 'm':
    StickyKey = DragKey;
    DragKey = DM_MOVE;
    break;
  case 'r':
    StickyKey = DragKey;
    DragKey = DM_RECT;
    break;

  case 't'|sKEYQ_BREAK:
  case 'z'|sKEYQ_BREAK:
  case 'y'|sKEYQ_BREAK:
  case ' '|sKEYQ_BREAK:
  case 'm'|sKEYQ_BREAK:
  case 'r'|sKEYQ_BREAK:
    DragKey = StickyKey;
    StickyKey = DM_SELECT;
    break;

  case sKEY_BACKSPACE:
  case sKEY_DELETE:
    OnCommand(CMD_DELETE);
    break;
  case 'x':
    OnCommand(CMD_CUT);
    break;
  case 'c':
    OnCommand(CMD_COPY);
    break;
  case 'v':
    OnCommand(CMD_PASTE);
    break;
  case 'a':
    OnCommand(CMD_ADD);
    break;
  case 'q':
    OnCommand(CMD_QUANT);
    break;
  }
  if((key&sKEYQ_STICKYBREAK) && StickyKey)
    DragKey = StickyKey;
  if((key&sKEYQ_BREAK) && ((((key&0x1ffff)>='a' && (key&0x1ffff)<='z')) || (key&0x1ffff)==' '))
    StickyKey = 0;
//  sDPrintF("key %08x\n",key);
}

void AnimWin::OnDrag(sDragData &dd)
{
  sInt i,j;
  AnimKey *key;
  sInt x,y;
  sRect r;
  sInt firsthit;
  static sInt lx,ly;
  sInt dx,dy;
  sBool changed;

  dx = dd.MouseX;
  dy = dd.MouseY;
  Pos2Time(dx,dy);
  LastTime = dx;

  switch(dd.Mode)
  {
  case sDD_START:
    if(dd.Buttons&2)
    {
      OnCommand(CMD_POPUP);
      break;
    }
    firsthit = 1;
    DragMode = DragKey;

    if(dd.Buttons&4)
      DragMode = DM_SCROLL;
    switch(DragMode)
    {
    case DM_SELECT:
      for(i=0;i<Doc->Keys.Count;i++)
      {
        key = &Doc->Keys[i];
        for(j=0;j<Doc->Channels[key->Channel].Count;j++)
        {
          if(Doc->Channels[key->Channel].Select[j])
          {
            x=key->Time; y=key->Value[j]; Time2Pos(x,y);
            r.Init(x-3,y-3,x+4,y+4);
            if(dd.Buttons&1)
              key->Select[j] = 0;
            if(r.Hit(dd.MouseX,dd.MouseY))
            {
              if((dd.Buttons&2) || firsthit)
              {
                key->Select[j] = !key->Select[j];
                firsthit = sFALSE;
              }
            }
          }
        }
        lx = dd.MouseX;
        ly = dd.MouseY;
      }
      break;
    case DM_ZOOM:
      DragStartX = TimeZoom;
      DragStartY = ValAmp;
      break;
    case DM_SCROLL:
      DragStartX = ScrollX;
      DragStartY = ValMid;
      break;
    }
    break;
  case sDD_DRAG:
    switch(DragMode)
    {
    case DM_SELECT:
    case DM_MOVE:
      dx = dd.MouseX-lx;
      dy = dd.MouseY-ly;
      DPos2Time(dx,dy);
      lx = dd.MouseX;
      ly = dd.MouseY;
      changed = 0;
      for(i=0;i<Doc->Keys.Count;i++)
      {
        key = &Doc->Keys[i];
        firsthit = 0;
        for(j=0;j<Doc->Channels[key->Channel].Count;j++)
        {
          if(Doc->Channels[key->Channel].Select[j])
          {
            if(key->Select[j])
            {
              key->Value[j]+=dy;
              firsthit = 1;
              changed = 1;
            }
          }
        }
        if(firsthit)
        {
          key->Time = sRange(key->Time+dx,0x10000,0);
        }
      }
      if(changed)
        App->DemoPrev();
      break;
    case DM_ZOOM:
      TimeZoom = sRange<sInt>(DragStartX * (1.0f+dd.DeltaX*0.01f),65536,16);
      ValAmp = sRange<sInt>(DragStartY * (1.0f+dd.DeltaY*0.01f),0x1000000,0x1000);
      break;
    case DM_SCROLL:
      dx = dd.DeltaX;
      dy = dd.DeltaY;
      DPos2Time(dx,dy);
      ValMid = DragStartY-dy;
      ScrollX = sRange<sInt>(DragStartX-dd.DeltaX,SizeX-RealX,0);
      break;
    case DM_TIME:
      if(TOp)
      {
        dx = dd.MouseX;
        dy = dd.MouseY;
        Pos2Time(dx,dy);
        x = sMulShift(dx,TOp->x1-TOp->x0)+TOp->x0;
        if(x<0)
          x = 0;
        App->Player->BeatNow = x;
        App->Player->TimeNow = sMulDiv(App->Player->BeatNow,App->Player->TimeSpeed,0x10000);
        App->Player->LastTime = App->Player->TimeNow;
      }
      break;
    }
    break;
  case sDD_STOP:
    DragMode = 0;
    App->DemoPrev();
    break;
  }
}

sBool AnimWin::OnCommand(sU32 cmd)
{
  sMenuFrame *mf;

  switch(cmd)
  {
  case CMD_ADD:
    if(Doc)
      Doc->AddKey(LastTime);
    return sTRUE;

  case CMD_CUT:
    ClipCopyKey();
    ClipDeleteKey();
    return sTRUE;
  case CMD_DELETE:
    ClipDeleteKey();
    return sTRUE;
  case CMD_COPY:
    ClipCopyKey();
    return sTRUE;
  case CMD_PASTE:
    ClipPasteKey();
    return sTRUE;

  case CMD_POPUP:
    mf = new sMenuFrame;
    mf->AddMenu("Add",CMD_ADD,'a');
    mf->AddMenu("Delete",CMD_DELETE,sKEY_DELETE);
    mf->AddMenu("Cut",CMD_CUT,'x');
    mf->AddMenu("Copy",CMD_COPY,'c');
    mf->AddMenu("Paste",CMD_PASTE,'v');
    mf->AddSpacer();
    mf->AddMenu("Normal Mode",' ',' ');
    mf->AddMenu("Scratch Time",'t','t');
    mf->AddMenu("Zoom",'z','z');
    mf->AddMenu("Scroll",'y','y');
    mf->AddMenu("Move",'m','m');
    mf->AddMenu("Rect",'r','r');
    mf->AddSpacer();
    mf->AddBorder(new sNiceBorder);
    mf->SendTo = this;
    sGui->AddPopup(mf);
    return sTRUE;
  case 't':
  case 'z':
  case 'y':
  case ' ':
  case 'm':
  case 'r':
    OnKey(cmd);
    return sTRUE;

  default:
    return sFALSE;
  }
}

/****************************************************************************/

void AnimWin::ClipCopyKey()
{
}

void AnimWin::ClipPasteKey()
{
}

void AnimWin::ClipDeleteKey()
{
  sInt i,j;
  AnimChannel *chan;
  sInt ok;

  if(Doc)
  {
    for(i=0;i<Doc->Keys.Count;)
    {
      ok = 0;
      chan = &Doc->Channels[Doc->Keys[i].Channel];
      for(j=0;j<chan->Count;j++)
        ok |= chan->Select[j] && Doc->Keys[i].Select[j];
      if(ok)
      {
        Doc->Keys[i] = Doc->Keys[--Doc->Keys.Count];
      }
      else
      {
        i++;
      }
    }
  }
}

/****************************************************************************/
/****************************************************************************/

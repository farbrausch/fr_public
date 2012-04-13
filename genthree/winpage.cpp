// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "winpage.hpp"
#include "winview.hpp"
#include "winpara.hpp"
#include "cslce.hpp"

/****************************************************************************/
/****************************************************************************/

#define PAGEX           28        // width in pixel of an operator-unit
#define PAGEY           19        // height in pixels of an op-unit
#define PAGESX          192       // width in op-units of a page
#define PAGESY          64        // height in op-units of a page

#define DM_PICK         1         // select one element by picking
#define DM_RECT         2         // select by rectangle
#define DM_DUPLICATE    3         // duplicate selection
#define DM_WIDTH        4         // change width
#define DM_SELECT       5         // generic selecting (DM_PICK or DM_RECT)
#define DM_MOVE         6         // move around
#define DM_SCROLL       7

#define SM_SET          1         // clear everything else
#define SM_ADD          2         // add to selection (SHIFT)
#define SM_TOGGLE       3         // toggle selection (CTRL)

#define CMD_POPUP       256
#define CMD_POPMISC     257
#define CMD_POPTEX      258
#define CMD_POPMESH     259
#define CMD_POPADD      260
#define CMD_SHOW        261
#define CMD_DELETE      263
#define CMD_CUT         264
#define CMD_COPY        265
#define CMD_PASTE       266
#define CMD_POPSCENE    267
#define CMD_POPMAT      268
#define CMD_POPFX				269

/****************************************************************************/
/****************************************************************************/

sBool IsGoodName(sChar *name)
{

  if(!((*name>='a' && *name<='z') || (*name>='A' && *name<='Z')))
    return sFALSE;
  name++;
  while(*name)
  {
    if(!((*name>='a' && *name<='z') || 
         (*name>='A' && *name<='Z') ||
         (*name>='0' && *name<='9') || *name=='_'))
      return sFALSE;
    name++;
  }
  return sTRUE;
}

/****************************************************************************/
/***                                                                      ***/
/***   Classes                                                            ***/
/***                                                                      ***/
/****************************************************************************/

void PageOpClass::Init(sChar *name,sInt funcid)
{
  sSetMem(this,0,sizeof(*this));
  Name = sDupString(name);
  FuncId = funcid;
  InputCount = 0;
  Flags = 0;
  Color = sGui->Palette[sGC_BUTTON];
  InputFlags =0;

  AddPara(name,sCT_LABEL);  
}

sInt PageOpClass::AddPara(sChar *name,sInt type)
{
  Para[ParaCount].Name = sDupString(name);
  Para[ParaCount].XPos = 0;
  Para[ParaCount].XSize = 12;
  Para[ParaCount].YPos = ParaCount;
  Para[ParaCount].YSize = 1;
  Para[ParaCount].Type = type;
  
  return ParaCount++;
}

/****************************************************************************/
/****************************************************************************/

PageOp::PageOp()
{
  Class = 0;
  PosX = 0;
  PosY = 0;
  Width = 3;
  Selected = 0;
  InputCount = 0;
  Error = 1;

  sSetMem(Data,0,sizeof(Data));
  sSetMem(Inputs,0,sizeof(Inputs));
}

PageOp::~PageOp()
{
  sInt i;

  for(i=0;i<MAX_PAGEOPPARA;i++)
  {
    if(Data[i].Anim)
      delete[] Data[i].Anim;
  }
}

void PageOp::Tag()
{
  sInt i;

  ToolObject::Tag();
  for(i=0;i<InputCount;i++)
    sBroker->Need(Inputs[i]);
}

void PageOp::Copy(sObject *o)
{
  PageOp *po;
  sInt i;
  sVERIFY(o->GetClass() == sCID_TOOL_PAGEOP);
  
  po = (PageOp *) o;

  Class = po->Class;
  PosX = po->PosX;
  PosY = po->PosY;
  Width = po->Width;
  Selected = po->Selected;
  Name[0] = 0;
  Doc = po->Doc;
  CID = po->CID;

  for(i=0;i<MAX_PAGEOPPARA;i++)
  {
    Data[i].Data[0] = po->Data[i].Data[0];
    Data[i].Data[1] = po->Data[i].Data[1];
    Data[i].Data[2] = po->Data[i].Data[2];
    Data[i].Data[3] = po->Data[i].Data[3];
    Data[i].Animated = po->Data[i].Animated;
    if(po->Data[i].Anim)
      Data[i].Anim = sDupString(po->Data[i].Anim,MAX_ANIMSTRING);
  }
}

void PageOp::MakeRect(sRect &r,sRect &client)
{
  r.x1 = client.x0 + (PosX+Width)*PAGEX;
  r.y1 = client.y0 + (PosY+1)*PAGEY;
  r.x0 = client.x0 + PosX*PAGEX;
  r.y0 = client.y0 + PosY*PAGEY;
}

void PageOp::Init(PageOpClass *cl,sInt x,sInt y,sInt w,PageDoc *doc)
{
  sInt i;

  PosX = x;
  PosY = y;
  Width = w;
  Class = cl;
  sSetMem(Data,0,sizeof(Data));

  Name[0] = 0;
  CID = cl->OutputCID;
  if(CID==0)
    CID = cl->InputCID[0];
  Doc = doc;

  for(i=0;i<MAX_PAGEOPPARA && cl->Para[i].Name;i++)
  {
    Data[i].Data[0] = cl->Para[i].Default[0];
    Data[i].Data[1] = cl->Para[i].Default[1];
    Data[i].Data[2] = cl->Para[i].Default[2];
    Data[i].Data[3] = cl->Para[i].Default[3];
    Data[i].Animated = 0;
    if(cl->Para[i].Type==sCT_STRING)
      Data[i].Anim = sDupString("",MAX_ANIMSTRING);
  }
}

/****************************************************************************/
/****************************************************************************/

PageDoc::PageDoc()
{
  Ops = new sList<PageOp>;
}

PageDoc::~PageDoc()
{
}

void PageDoc::Clear()
{
  sInt i;
  for(i=0;i<Ops->GetCount();i++)
    Ops->Get(i)->Selected = 1;
  Delete();
}

void PageDoc::Tag()
{
  ToolDoc::Tag();
  sBroker->Need(Ops);
}

/****************************************************************************/
/*
void PageDoc::UpdatePara()
{
  if(NeedUpdatePage)
    UpdatePage();


  NeedUpdatePara = 0;
}
*/
void PageDoc::UpdatePage()
{
  sInt i,max,j,k;
  ToolObject *to;
  PageOp *po,*pp;
  sBool updateobjects;
  sInt inc;
  PageOp *in[MAX_PAGEOPINPUTS];
  ToolWindow *tw;

  updateobjects = 0;

// find input operators

  max = Ops->GetCount();
  for(i=0;i<max;i++)
  {
    po = Ops->Get(i);
    po->Error = 0;
    inc = 0;

    for(j=0;j<max;j++)                      // find all inputs
    {
      pp = Ops->Get(j);
      if(pp->PosY == po->PosY-1 &&
         pp->PosX+pp->Width > po->PosX &&
         pp->PosX < po->PosX+po->Width)
      {
        if(inc<MAX_PAGEOPINPUTS)
        {
          in[inc++] = pp;
        }
        else
        {
          po->Error = 1;
        }
      }
    }

    for(j=0;j<inc-1;j++)                    // sort inputs by x
      for(k=j+1;k<inc;k++)
        if(in[j]->PosX > in[k]->PosX)
          sSwap(in[j],in[k]);

    if(inc!=po->InputCount || sCmpMem(in,po->Inputs,inc*4)!=0)  // changed?
    {
      sSetMem(po->Inputs,0,sizeof(po->Inputs));
      for(j=0;j<inc;j++)
        po->Inputs[j] = in[j];
      po->InputCount = inc;
      po->ChangeCount = App->GetChange();
    }

    if(po->Class->InputFlags&SCFU_MULTI)
    {
      for(j=0;j<po->InputCount;j++)
        if(po->Inputs[j]->Class->OutputCID != po->Class->InputCID[0])
          po->Error = 1;      
    }
    else
    {
      if(po->InputCount!=po->Class->InputCount)
        po->Error = 1;
      for(j=0;j<po->InputCount;j++)
        if(po->Inputs[j]->Class->OutputCID != po->Class->InputCID[j])
          po->Error = 1;
    }
  }

// add new STORE's

  max = Ops->GetCount();
  for(i=0;i<max;i++)
  {
    po = Ops->Get(i);
    if((po->Class->Flags & POCF_SL) && po->Data[1].Anim[0]) 
    {
      if(IsGoodName(po->Data[1].Anim))
      {
        sCopyString(po->Name,po->Data[1].Anim,sizeof(po->Name));
        to = App->FindObject(po->Data[1].Anim);
        if(to)
        {
          if(to!=(ToolObject*)po || po->Inputs[0]==0 || po->CID != po->Inputs[0]->Class->OutputCID)
            po->Error = 1;
        }
        else
        {
          updateobjects = 1;
          App->Objects->Add(po);
        }
      }
      else
        po->Error = 1;
    }
  }
  max = Ops->GetCount();
  for(i=0;i<max;i++)
  {
    if(po->Class->Flags & POCF_LOAD)
    {
      if(IsGoodName(po->Data[1].Anim))
      {
        to = App->FindObject(po->Data[1].Anim);
        if(!to)
          po->Error = 1;
        else
          if(to->CID!=po->Class->OutputCID)
            po->Error = 1;
      }
      else
        po->Error = 1;
    }
  }

// done

  if(updateobjects)
    App->UpdateObjectList();


  // set flags;

  max = Ops->GetCount();
  for(i=0;i<max;i++)
  {
    po = Ops->Get(i);
    po->Showed = 0;
    po->Edited = 0;
  }
  for(i=0;i<App->Windows->GetCount();i++)
  {
    tw = App->Windows->Get(i);
    switch(tw->GetClass())
    {
    case sCID_TOOL_VIEWWIN:
      to = ((ViewWin*)tw)->GetObject();
      if(to && to->Doc==this && to->GetClass()==sCID_TOOL_PAGEOP)
      {
        po = (PageOp *)to;
        po->Showed = sTRUE;
      }
      break;
    case sCID_TOOL_PARAWIN:
      po = ((ParaWin*)tw)->GetOp();
      if(po && po->Doc==this)
        po->Edited = sTRUE;
      break;
    }
  }

// update cache-need

  for(i=0;i<Ops->GetCount();i++)
  {
    po = Ops->Get(i);
    po->NeedsCache = 0;
  }
  for(i=0;i<Ops->GetCount();i++)
  {
    po = Ops->Get(i);
    if(po->Showed)
      po->NeedsCache = 1;
    if(po->Edited || (po->Showed && po->InputCount>1))
    {
      for(j=0;j<po->InputCount;j++)
        if(po->Inputs[j])
          po->Inputs[j]->NeedsCache = 1;
    }
  }
}


sBool PageDoc::Write(sU32 *&data)
{
  sU32 *hdr;
  PageOp *po;
  sInt i,j,k,zones;

  hdr = sWriteBegin(data,sCID_TOOL_PAGEDOC,1);
  *data++ = Ops->GetCount();
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;
  for(i=0;i<Ops->GetCount();i++)
  {
    po = Ops->Get(i);

    *data++ = po->Class->FuncId;
    *data++ = po->PosX;
    *data++ = po->PosY;
    *data++ = po->Width;
    *data++ = po->Selected;
    *data++ = po->Class->ParaCount;
    *data++ = 0;
    *data++ = 0;
    for(j=0;j<po->Class->ParaCount;j++)
    {
      if(po->Class->Para[j].Type!=sCT_LABEL)
      {
        if(po->Data[j].Animated)
        {
          *data++ = 5;
          sWriteString(data,po->Data[j].Anim);
        }
        else
        {
          switch(po->Class->Para[j].Type)
          {
          case sCT_STRING:
            *data++ = 6;
            sWriteString(data,po->Data[j].Anim);
            break;
          case sCT_CYCLE:
          case sCT_CHOICE:
          case sCT_MASK:
            *data++ = 1;
            *data++ = po->Data[j].Data[0];
            break;
          case sCT_LABEL:
            break;
          default:
            zones = po->Class->Para[j].Zones;
            if(zones<1) zones = 1;
            *data++ = zones;
            for(k=0;k<zones;k++)
              *data++ = po->Data[j].Data[k];
            break;
          }
        }
      }
    }
  }

  sWriteEnd(data,hdr);
  return sTRUE;
}

sBool PageDoc::Read(sU32 *&data)
{
  sInt version;
  PageOp *po;
  sInt i,j,k,zones;
  sInt count,pc;
  sChar buffer[MAX_ANIMSTRING];

  Ops->Clear();

  version = sReadBegin(data,sCID_TOOL_PAGEDOC);
  if(version<1) return sFALSE;
  count = *data++;
  data+=3;

  for(i=0;i<count;i++)
  {
    po = new PageOp;
    po->Class = App->FindPageOpClass(*data++);
    if(po->Class==0) return sFALSE;
    po->PosX = *data++;
    po->PosY = *data++;
    po->Width = *data++;
    po->Selected = *data++;
    pc = *data++;
    data+=2;

    po->CID = po->Class->OutputCID; 
    if(po->CID==0 && (po->Class->Flags & POCF_STORE) && (po->Class->InputCount>0))
    {
      po->CID = po->Class->InputCID[0];
    }
    po->Doc = this;

    if(po->Class->ParaCount<pc) return sFALSE;
    for(j=0;j<pc;j++)
    {
      if(po->Class->Para[j].Type==sCT_LABEL)
        continue;
      zones = *data++;
      switch(zones)
      {
      case 5:
        po->Data[j].Animated = 1;
        /* nobreak */
      case 6:
        if(!sReadString(data,buffer,sizeof(buffer))) return sFALSE;
        po->Data[j].Anim = sDupString(buffer,MAX_ANIMSTRING);
        break;
      case 1:
        po->Data[j].Data[0] = *data;
        po->Data[j].Data[1] = *data;
        po->Data[j].Data[2] = *data;
        po->Data[j].Data[3] = *data;
        data++;
        break;
      case 2:
      case 3:
      case 4:
        for(k=0;k<zones;k++)
          po->Data[j].Data[k] = *data++;
        break;
      default:
        return sFALSE;
      }
    }
    Ops->Add(po);
  }
  UpdatePage();

  return sReadEnd(data);
}

sBool PageDoc::CheckDest(PageDoc *source,sInt dx,sInt dy)
{
  sInt i,max,j;
  sInt x,y,w;
  PageOp *po;
  sU8 map[PAGESY][PAGESX];
 
  sSetMem(map,0,sizeof(map));

  max = Ops->GetCount();
  for(i=0;i<max;i++)
  {
    po = Ops->Get(i);
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

sBool PageDoc::CheckDest(sInt dx,sInt dy,sInt dw)
{
  sInt i,max;
  PageOp *po;

  max = Ops->GetCount();
  for(i=0;i<max;i++)
  {
    po = Ops->Get(i);
    if(po->PosY==dy)
    {
      if(po->PosX<dx+dw && po->PosX+po->Width>dx)
        return sFALSE;
    }
  }
  return sTRUE;
}

void PageDoc::Delete()
{
  sInt i;
  sBool updateobjects;
  PageOp *po;

  updateobjects = 0;
  for(i=0;i<Ops->GetCount();)
  {
    po = Ops->Get(i);
    if(po->Selected)
    {
      if(po->Class->Flags & POCF_SL)
      {
        App->Objects->Rem(po);
        updateobjects = 1;
      }
      Ops->Rem(po);
    }
    else
    {
      i++;
    }
  }
  if(updateobjects)
    App->UpdateObjectList();

  UpdatePage();
}

void PageDoc::Copy()
{
  sInt i,max;
  PageOp *po,*copy;
  PageDoc *clip;

  clip = 0;
  max = Ops->GetCount();
  for(i=0;i<max;i++)
  {
    po = Ops->Get(i);
    if(po->Selected)
    {
      if(clip==0)
        clip = new PageDoc;
      copy = new PageOp;
      copy->Copy(po);
      clip->Ops->Add(copy);
    }
  }
  sGui->ClipboardClear();
  if(clip)
    sGui->ClipboardAdd(clip);
}

void PageDoc::Paste(sInt x,sInt y)
{
  sInt i,max;
  PageDoc *clip;
  PageOp *po,*copy;
  sInt xmin,ymin;

  clip = (PageDoc *)sGui->ClipboardFind(sCID_TOOL_PAGEDOC);
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
        copy = new PageOp;
        copy->Copy(po);
        copy->PosX += x - xmin;
        copy->PosY += y - ymin;
        Ops->Add(copy);
      }
    }
  } 
  UpdatePage();
}

/****************************************************************************/
/***                                                                      ***/
/***   Page Window                                                        ***/
/***                                                                      ***/
/****************************************************************************/

PageWin::PageWin()
{
  Doc = 0;
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
  AddOpClass = CMD_POPTEX;
  Flags |= sGWF_FLUSH;
}

PageWin::~PageWin()
{
}

void PageWin::Tag()
{
  ToolWindow::Tag();
  sBroker->Need(Doc);
  sBroker->Need(EditOp);
}

void PageWin::OnInit()
{
}

void PageWin::OnCalcSize()
{
  SizeX = PAGEX*PAGESX;
  SizeY = PAGEY*PAGESY;
}

void PageWin::SetDoc(ToolDoc *doc)
{ 
  if(doc)
    sVERIFY(doc->GetClass()==sCID_TOOL_PAGEDOC); 
  Doc = (PageDoc *)doc; 
}

/****************************************************************************/

void PageWin::PaintButton(sRect r,sChar *name,sU32 color,sBool pressed,sInt style)
{
  sGui->Bevel(r,pressed);
  sPainter->Paint(sGui->FlatMat,r,color);
  sPainter->PrintC(sGui->PropFont,r,0,name,sGui->Palette[sGC_TEXT]);
}

void PageWin::ScrollToCursor()
{
  sRect r;
  
  r.x0 = Client.x0 + (CursorX)*PAGEX;
  r.x1 = Client.x0 + (CursorX+CursorWidth)*PAGEX;
  r.y0 = Client.y0 + (CursorY)*PAGEY;
  r.y1 = Client.y0 + (CursorY+1)*PAGEY;
  
  ScrollTo(r,sGWS_SAFE);
}

PageOp *PageWin::FindOp(sInt mx,sInt my)
{
  sInt i,max;
  PageDoc *pd;
  PageOp *po;
  sRect r;

  pd = Doc;
  if(pd)
  {
    max = pd->Ops->GetCount();
    for(i=0;i<max;i++)
    {
      po = pd->Ops->Get(i);
      po->MakeRect(r,Client);
      if(r.Hit(mx,my))
        return po;
    }
  }

  return sFALSE;
}

sBool PageWin::CheckDest(sBool dup)
{
  sInt i,max,j;
  sInt x,y,w;
  PageDoc *pd;
  PageOp *po;
  sU8 map[PAGESY][PAGESX];
  
  pd = Doc;
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

void PageWin::MoveDest(sBool dup)
{
  sInt i,max;
  sInt x,y,w;
  PageDoc *pd;
  PageOp *po,*opo;
  
  pd = Doc;
  if(!pd) return;
  max = pd->Ops->GetCount();

  for(i=0;i<max;i++)
  {
    po = pd->Ops->Get(i);
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
        po = new PageOp;
        po->Copy(opo);
        pd->Ops->Add(po);
        opo->Selected = sFALSE;
      }
      po->PosX = x;
      po->PosY = y;
      po->Width = w;
    }
  }

  pd->UpdatePage();
}

/****************************************************************************/

void PageWin::OnPaint()
{
  sInt i,max,j;
  PageOp *po;
  PageDoc *pd;
  sRect r;
  sInt shadow;
  sU32 col;
  sChar *name;

 //  ToolWindow *tw;
//  ToolObject *to;
  static sChar *dragmodes[] = { "???","pick","rect","duplicate","width","select","move","scroll" };

  if(Flags&sGWF_CHILDFOCUS)
  {
    App->SetStat(0,dragmodes[DragKey]);
    OpWindowX = -1;
    OpWindowY = -1;
  }

  sPainter->Paint(sGui->FlatMat,Client,sGui->Palette[sGC_BACK]);
  if(Doc==0)
  {
    sPainter->PrintC(sGui->PropFont,Client,0,"Page");
  }
  else
  {
    pd = Doc;
    max = pd->Ops->GetCount();

    // paint body

    for(i=0;i<max;i++)
    {
      po = pd->Ops->Get(i);
      po->MakeRect(r,Client);
      name = po->Class->Name;
      if(po->Class->Flags & (POCF_STORE|POCF_LOAD|POCF_LABEL))
      {
        if(po->Data[1].Anim)
          name = po->Data[1].Anim;
      }
      PaintButton(r,name,po->Error ? 0xffff00000 : po->Class->Color,po->Selected,0);
      if(po->Edited)
        sPainter->Paint(sGui->FlatMat,r.x0+4,r.y0+4,8,4,0xff7f0000);
      if(po->Showed)
        sPainter->Paint(sGui->FlatMat,r.x0+4,r.y1-8,8,4,0xff007f00);
      if(po->Cache)
        sPainter->Paint(sGui->FlatMat,r.x0+4,r.y0+8,8,r.y1-r.y0-16,0xff00007f);
      if(po->Class->Flags & (POCF_LOAD|POCF_LABEL))
      {
        col = sGui->Palette[po->Selected?sGC_LOW2:sGC_HIGH2];
        for(j=4;j>=0;j--)
        {
          sPainter->Line(r.x0+j,r.y0,r.x0,r.y0+j,col);
          sPainter->Line(r.x1-j-1,r.y0,r.x1-1,r.y0+j,col);
          col = sGui->Palette[sGC_BACK];
        }
        sPainter->Line(r.x0,r.y0,r.x0,r.y0+5-2,col);
        sPainter->Line(r.x1-1,r.y0,r.x1-1,r.y0+5-2,col);
        col = sGui->Palette[po->Selected?sGC_LOW:sGC_HIGH]; j=4;
        sPainter->Line(r.x0+1,r.y0+j,r.x0+j,r.y0+1,col);
        sPainter->Line(r.x1-2,r.y0+j,r.x1-j-1,r.y0+1,col);
      }

      if(po->Class->Flags & (POCF_STORE|POCF_LABEL))
      {
        col = sGui->Palette[po->Selected?sGC_HIGH2:sGC_LOW2];
        for(j=4;j>=0;j--)
        {
          sPainter->Line(r.x0,r.y1-1-j,r.x0+j,r.y1-1,col);
          sPainter->Line(r.x1-1,r.y1-1-j,r.x1-j-1,r.y1-1,col);
          col = sGui->Palette[sGC_BACK];
        }
        sPainter->Line(r.x0,r.y1-1,r.x0,r.y1-5+2,col);
        sPainter->Line(r.x1-1,r.y1-1,r.x1-1,r.y1-5+2,col);
        col = sGui->Palette[po->Selected?sGC_HIGH:sGC_LOW]; j = 4;
        sPainter->Line(r.x0+1,r.y1-1-j,r.x0+j,r.y1-2,col);
        sPainter->Line(r.x1-2,r.y1-1-j,r.x1-j-1,r.y1-2,col);
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
        po->MakeRect(r,Client);
        r.x0 += DragMoveX*PAGEX;
        r.y0 += DragMoveY*PAGEY;
        r.x1 += (DragMoveX+DragWidth)*PAGEX;
        r.y1 += DragMoveY*PAGEY;
        sGui->RectHL(r,shadow,sGui->Palette[sGC_DRAW],sGui->Palette[sGC_DRAW]);
      }
    }
  }

  if(DragMode==DM_RECT)
  {
    sGui->RectHL(DragRect,1,sGui->Palette[sGC_DRAW],sGui->Palette[sGC_DRAW]);
  }

  r.x0 = Client.x0 + CursorX*PAGEX;
  r.y0 = Client.y0 + CursorY*PAGEY;
  r.x1 = Client.x0 + (CursorX+CursorWidth)*PAGEX;
  r.y1 = Client.y0 + (CursorY+1)*PAGEY;
  sGui->RectHL(r,1,sGui->Palette[sGC_DRAW],sGui->Palette[sGC_DRAW]);
}

/****************************************************************************/

void PageWin::OnDrag(sDragData &dd)
{
  sU32 key;
  PageOp *po;
  PageDoc *pd;
  sInt i,max;
  sRect r;
  ParaWin *parawin;
  sBool pickit;

  pd = Doc;
  if(pd==0) return;
  max = pd->Ops->GetCount();

  if(MMBScrolling(dd,DragStartX,DragStartY)) return;

  switch(dd.Mode)
  {
  case sDD_START:
    CursorX = sRange<sInt>((dd.MouseX-Client.x0)/PAGEX-1,PAGESX-3,0);
    CursorY = sRange<sInt>((dd.MouseY-Client.y0)/PAGEY  ,PAGESY-1,0);
    CursorWidth = 3;
    pickit = 0;

    if(dd.Buttons&2)
    {
      sGui->Post(CMD_POPUP,this);
      break;
    }
    SelectMode = SM_SET;
    key = sSystem->GetKeyboardShiftState();
    if(key&sKEYQ_SHIFT)
      SelectMode = SM_ADD;
    if(key&sKEYQ_CTRL)
      SelectMode = SM_TOGGLE;

    DragMode = DragKey;

    if(DragMode==DM_SELECT)
    {
      po = FindOp(dd.MouseX,dd.MouseY);
      if(po)
      {
        DragMode = DM_PICK;
        pickit = 1;
        if(po->Selected)
          SelectMode = SM_ADD;
      }
      else 
      {
        DragMode = DM_RECT;
      }
    }

    if(DragMode==DM_DUPLICATE || DragMode==DM_WIDTH || DragMode==DM_MOVE)
    {
      po = FindOp(dd.MouseX,dd.MouseY);
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
      EditOp = po = FindOp(dd.MouseX,dd.MouseY);
      if(po)
      {
        parawin = (ParaWin *) App->FindActiveWindow(sCID_TOOL_PARAWIN);
        if(parawin)
        {
          parawin->SetOp(po,Doc);
          pd->UpdatePage();
        }
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
      if(sAbs(dd.DeltaX)>2 || sAbs(dd.DeltaY)>2)
        DragMode = DM_MOVE;
      break;
    case DM_RECT:
      DragRect.Init(dd.MouseX,dd.MouseY,DragStartX,DragStartY);
      DragRect.Sort();
      break;
    case DM_MOVE:
    case DM_DUPLICATE:
      DragMoveX = (dd.DeltaX+1024*PAGEX+PAGEX/2)/PAGEX-1024;
      DragMoveY = (dd.DeltaY+1024*PAGEY+PAGEY/2)/PAGEY-1024;
      break;
    case DM_WIDTH:
      DragWidth = (dd.DeltaX+1024*PAGEX+PAGEX/2)/PAGEX-1024;
      break;
    case DM_SCROLL:
      ScrollX = sRange<sInt>(DragStartX-dd.DeltaX,SizeX-RealX,0);
      ScrollY = sRange<sInt>(DragStartY-dd.DeltaY,SizeY-RealY,0);
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
        po->MakeRect(r,Client);
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
    break;
  }
}

/****************************************************************************/

void PageWin::OnKey(sU32 key)
{
  switch(key&(0x8001ffff|sKEYQ_REPEAT))
  {
  case sKEY_APPFOCUS:
    App->SetActiveWindow(this);
    break;
  case 'a':
    sGui->Post(CMD_POPADD,this);
    break;
  case '1':
    sGui->Post(CMD_POPMISC,this);
    break;
  case '2':
    sGui->Post(CMD_POPTEX,this);
    break;
  case '3':
    sGui->Post(CMD_POPMESH,this);
    break;
  case '4':
    sGui->Post(CMD_POPSCENE,this);
    break;
  case '5':
    sGui->Post(CMD_POPMAT,this);
    break;
	case '6':
		sGui->Post(CMD_POPFX,this);
		break;

  case ' ':
    StickyKey = DM_SELECT;
    DragKey = DM_SELECT;
    break;
  case 'r':
    StickyKey = DragKey;
    DragKey = DM_RECT;
    break;
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

  case 'r'|sKEYQ_BREAK:
  case 'p'|sKEYQ_BREAK:
  case 'd'|sKEYQ_BREAK:
  case 'w'|sKEYQ_BREAK:
  case 'm'|sKEYQ_BREAK:
  case 'y'|sKEYQ_BREAK:
    DragKey = StickyKey;
    break;


  case 's':
    OnCommand(CMD_SHOW);
    break;
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
  }
  switch(key&(0x8001ffff))
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
  }
  /*
  if((key&sKEYQ_STICKYBREAK) && StickyKey)
    DragKey = StickyKey;
  if(key&sKEYQ_BREAK && (key&0x1ffff)>='a' && (key&0x1ffff)<='z')
    StickyKey = 0;
    */
}

/****************************************************************************/

sBool PageWin::OnCommand(sU32 cmd)
{
  sMenuFrame *mf;
//  ToolObject *to;
  ViewWin *view;

  switch(cmd)
  {

  case CMD_POPUP:
    mf = new sMenuFrame;
    mf->AddMenu("Add",CMD_POPADD,'a');
    mf->AddMenu("Add Misc",CMD_POPMISC,'1');
    mf->AddMenu("Add Texture",CMD_POPTEX,'2');
    mf->AddMenu("Add Mesh",CMD_POPMESH,'3');
    mf->AddMenu("Add Scene",CMD_POPSCENE,'4');
    mf->AddMenu("Add Material",CMD_POPMAT,'5');
		mf->AddMenu("Add FXChain",CMD_POPFX,'6');
    mf->AddSpacer();
    mf->AddMenu("Normal Mode",' ',' ');
    mf->AddMenu("Select Rect",'r','r');
    mf->AddMenu("Pick",'p','p');
    mf->AddMenu("Duplicate",'d','d');
    mf->AddMenu("Width",'w','w');
    mf->AddMenu("Move",'m','m');
    mf->AddSpacer();
    mf->AddMenu("Show",CMD_SHOW,'s');
    mf->AddSpacer();
    mf->AddMenu("Delete",CMD_DELETE,sKEY_DELETE);
    mf->AddMenu("Cut",CMD_CUT,'x');
    mf->AddMenu("Copy",CMD_COPY,'c');
    mf->AddMenu("Paste",CMD_PASTE,'v');
    mf->AddBorder(new sNiceBorder);
    mf->SendTo = this;
    sGui->AddPopup(mf);
    return sTRUE;

  case CMD_SHOW:
    view = (ViewWin *) App->FindActiveWindow(sCID_TOOL_VIEWWIN);
    if(EditOp && view && EditOp->Class->OutputCID)
    {
      view->SetObject(EditOp);
    }
    if(EditOp && view && (EditOp->Class->Flags & POCF_STORE)) // store
    {
      view->SetObject(EditOp);
    }
    if(Doc)
      Doc->UpdatePage();
    return sTRUE;

  case CMD_POPADD:
    AddOps(AddOpClass);
    return sTRUE;

  case CMD_POPMISC:
    AddOpClass = 0;
    AddOps(AddOpClass);
    return sTRUE;

  case CMD_POPTEX:
    AddOpClass = sCID_GENBITMAP;
    AddOps(AddOpClass);
    return sTRUE;

  case CMD_POPMESH:
    AddOpClass = sCID_GENMESH;
    AddOps(AddOpClass);
    return sTRUE;

  case CMD_POPSCENE:
    AddOpClass = sCID_GENSCENE;
    AddOps(AddOpClass);
    return sTRUE;

  case CMD_POPMAT:
    AddOpClass = sCID_GENMATERIAL;
    AddOps(AddOpClass);
    return sTRUE;

	case CMD_POPFX:
		AddOpClass = sCID_GENFXCHAIN;
		AddOps(AddOpClass);
		return sTRUE;

  case ' ':
  case 'r':
  case 'p':
  case 'd':
  case 'w':
  case 'm':
    OnKey(cmd);
    return sTRUE;

  case CMD_DELETE:
    if(Doc)
      Doc->Delete();
    return sTRUE;
  case CMD_CUT:
    if(Doc)
    {
      Doc->Copy();
      Doc->Delete();
    }
    return sTRUE;
  case CMD_COPY:
    if(Doc)
      Doc->Copy();
    return sTRUE;
  case CMD_PASTE:
    if(Doc)
      Doc->Paste(CursorX,CursorY);
    return sTRUE;

  default:
    if(cmd>=1024 && cmd<2048)
    {
      if(App->ClassList[cmd-1024].Name)
        AddOp(&App->ClassList[cmd-1024]);
      return sTRUE;
    }
    return sFALSE;
  }
}

void PageWin::AddOps(sU32 rcid)
{
  sU32 cid;
  sInt i,j;
  sInt added,column;
  sMenuFrame *mf;
//  sInt key,chr;
//  sU8 keys[26];
  sBool ok;

  mf = new sMenuFrame;

  mf->AddMenu("Misc"    ,CMD_POPMISC ,'1')->BackColor = App->ClassColor(0);
  mf->AddMenu("Texture" ,CMD_POPTEX  ,'2')->BackColor = App->ClassColor(sCID_GENBITMAP);
  mf->AddMenu("Mesh"    ,CMD_POPMESH ,'3')->BackColor = App->ClassColor(sCID_GENMESH);
  mf->AddMenu("Scene"   ,CMD_POPSCENE,'4')->BackColor = App->ClassColor(sCID_GENSCENE);
  mf->AddMenu("Material",CMD_POPMAT  ,'5')->BackColor = App->ClassColor(sCID_GENMATERIAL);
	mf->AddMenu("FXChain" ,CMD_POPFX   ,'6')->BackColor = App->ClassColor(sCID_GENFXCHAIN);

//  sSetMem(keys,0,26);
  for(column=0;column<6;column++)
  {
    added = 0;
    for(i=App->ClassCount-1;i>=0;i--)
    {
      j = sMin(App->ClassList[i].InputCount,2);
      if(j>=1) j++;
      if(App->ClassList[i].InputFlags&SCFU_ROWA)  j=0;
      if(App->ClassList[i].InputFlags&SCFU_ROWB)  j=3;
      if(App->ClassList[i].InputFlags&SCFU_ROWC)  j=1;

      ok = sFALSE;
      
      cid = rcid;
      if(cid==0 && column==0)
      {
        j = column;
        cid = sCID_GENSPLINE;        
      }
      if(cid==0 && column==1)
      {
        j = column;
        cid = sCID_GENPARTICLES;        
      }
      if(j==column && (App->ClassList[i].InputFlags&0x0f)!=0)
      {
        if(cid!=0)
          if(App->ClassList[i].OutputCID==cid || (App->ClassList[i].OutputCID==0 && App->ClassList[i].InputCID[0]==cid))
            ok = sTRUE;
        if(cid==sCID_GENMATERIAL)
          if(App->ClassList[i].InputCID[0]==sCID_GENMATPASS || App->ClassList[i].OutputCID==sCID_GENMATPASS)
            ok = sTRUE;
      }

      if(ok)
      {
        if(added==0)
          mf->AddColumn();

        added++;
        /*
        chr = App->ClassList[i].Name[0];
        key = -1;
        if(chr>='a' && chr<'z')
          key = chr-'a';
        if(chr>='A' && chr<'Z')
          key = chr-'A';
        chr = 0;
        if(key>=0 && keys[key]==0)
        {
          chr = key+'a';
          keys[key]++;
        }
        else if(key>=0 && keys[key]==1)
        {
          chr = key+'A'+sKEYQ_SHIFT;
          keys[key]++;
        }
        */
        mf->AddMenu(App->ClassList[i].Name,1024+i,App->ClassList[i].Shortcut);
      }
    }
  }

  mf->AddBorder(new sNiceBorder);
  mf->SendTo = this;
  if(OpWindowX==-1)
  {
    sGui->GetMouse(OpWindowX,OpWindowY);
    OpWindowX -= 24; if(OpWindowX<0) OpWindowX = 0;
    OpWindowY -= 5;  if(OpWindowY<0) OpWindowY = 0;
  }

  sGui->AddWindow(mf,OpWindowX,OpWindowY);
}

void PageWin::AddOp(PageOpClass *cl)
{
  PageOp *po;

  if(Doc)
  {
    if(Doc->CheckDest(CursorX,CursorY,CursorWidth))
    {
      po = new PageOp;
      po->Init(cl,CursorX,CursorY,CursorWidth,Doc);
      po->ChangeCount = App->GetChange();
      Doc->Ops->Add(po);
      Doc->UpdatePage();
      if(CursorY<PAGESY)
        CursorY++;
    }
  }
}


/****************************************************************************/
/****************************************************************************/

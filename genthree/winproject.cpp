// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "winproject.hpp"

#include "winpage.hpp"
#include "winview.hpp"
#include "wintime.hpp"

/****************************************************************************/

#define CMD_ADDPAGEDOC    256
#define CMD_DELETE        257
#define CMD_RENAME        258
#define CMD_MOVEUP        259
#define CMD_MOVEDOWN      260
#define CMD_RENAME2       261
#define CMD_ADDEDITDOC    262
#define CMD_ADDTIMEDOC    263

/****************************************************************************/
/***                                                                      ***/
/***   project                                                            ***/
/***                                                                      ***/
/****************************************************************************/

ProjectWin::ProjectWin()
{
  sToolBorder *tb;

  DocList = new sListControl;
  DocList->LeftCmd = 1;
  DocList->RightCmd = 2;
  DocList->AddScrolling(0,1);
  AddChild(DocList);

  tb = new sToolBorder();
  tb->AddMenu("Pages",0);
  AddBorder(tb);
}

void ProjectWin::OnInit()
{
  UpdateList();
}

ProjectWin::~ProjectWin()
{
}

void ProjectWin::OnCalcSize()
{
  if(Childs)
  {
    SizeX = Childs->SizeX;
    SizeY = Childs->SizeY;
  }
}

void ProjectWin::OnLayout()
{
  if(Childs)
  {
    Childs->Position = Client;
  }
}

/****************************************************************************/

void ProjectWin::UpdateList()
{
  sInt i,max;
  ToolDoc *td;

  DocList->ClearList();
  max = App->Docs->GetCount();
  for(i=0;i<max;i++)
  {
    td = App->Docs->Get(i);
    DocList->Add(td->Name);
  }
}

/****************************************************************************/

sBool ProjectWin::OnCommand(sU32 cmd)
{
  sInt i;
  ToolDoc *td;
  ToolWindow *tw;
  sDialogWindow *diag;
  sMenuFrame *mf;
  PageDoc *pagedoc;
  EditDoc *editdoc;
  TimeDoc *timedoc;

  switch(cmd)
  {
  case 1:
    i = DocList->GetSelect();
    if(i>=0 && i<App->Docs->GetCount())
    {
      td = App->Docs->Get(i);
      switch(td->GetClass())
      {
      case sCID_TOOL_PAGEDOC:
        tw = App->FindActiveWindow(sCID_TOOL_PAGEWIN);
        tw->SetDoc(td);
        App->SetWindowSet(tw);
        break;
      case sCID_TOOL_EDITDOC:
        tw = App->FindActiveWindow(sCID_TOOL_EDITWIN);
        tw->SetDoc(td);
        App->SetWindowSet(tw);
        break;
      case sCID_TOOL_TIMEDOC:
        tw = App->FindActiveWindow(sCID_TOOL_TIMEWIN);
        tw->SetDoc(td);
        App->SetWindowSet(tw);
        break;
      }
    }
    return sTRUE;
  case 2:
    mf = new sMenuFrame;
    mf->SendTo = this;
    mf->AddBorder(new sNiceBorder);
    mf->AddMenu("add operator page",CMD_ADDPAGEDOC,0);
    mf->AddMenu("add source",CMD_ADDEDITDOC,0);
    mf->AddMenu("add timeline",CMD_ADDTIMEDOC,0);
    mf->AddSpacer();
    mf->AddMenu("delete",CMD_DELETE,0);
    mf->AddMenu("rename",CMD_RENAME,0);
    mf->AddMenu("move up",CMD_MOVEUP,0);
    mf->AddMenu("move down",CMD_MOVEDOWN,0);
    sGui->AddPopup(mf);
    return sTRUE;

  case CMD_ADDPAGEDOC:
    pagedoc = new PageDoc;
    pagedoc->App = App;
    sCopyString(pagedoc->Name,"new ops",sizeof(pagedoc->Name));
    App->Docs->Add(pagedoc);
    App->UpdateDocList();
    return sTRUE;

  case CMD_ADDEDITDOC:
    editdoc = new EditDoc;
    editdoc->App = App;
    sCopyString(editdoc->Name,"new source",sizeof(editdoc->Name));
    App->Docs->Add(editdoc);
    App->UpdateDocList();
    return sTRUE;

  case CMD_ADDTIMEDOC:
    timedoc = new TimeDoc;
    timedoc->App = App;
    sCopyString(timedoc->Name,"timeline",sizeof(timedoc->Name));
    App->Docs->Add(timedoc);
    App->UpdateDocList();
    return sTRUE;

  case CMD_DELETE:
    i = DocList->GetSelect();
    if(i>=0 && i<App->Docs->GetCount())
    {
      App->Docs->Get(i)->Clear();
      App->Docs->RemOrder(i);
      App->UpdateAllWindows();
    }
    return sTRUE;
  case CMD_RENAME:
    i = DocList->GetSelect();
    if(i>=0 && i<App->Docs->GetCount())
    {
      td = App->Docs->Get(i);
      diag = new sDialogWindow;
      diag->InitString(td->Name,sizeof(td->Name));
      diag->InitOkCancel(this,"rename","enter new name",CMD_RENAME2,0);
    }
    return sTRUE;
  case CMD_RENAME2:
    App->UpdateDocList();
    return sTRUE;
  case CMD_MOVEUP:
    i = DocList->GetSelect();
    if(i>=1 && i<App->Docs->GetCount())
    {
      App->Docs->Swap(i-1);
      App->UpdateDocList();
    }
    return sTRUE;
  case CMD_MOVEDOWN:
    i = DocList->GetSelect();
    if(i>=0 && i<App->Docs->GetCount()-1)
    {
      App->Docs->Swap(i);
      App->UpdateDocList();
    }
    return sTRUE;

  default:
    return sFALSE;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   object                                                             ***/
/***                                                                      ***/
/****************************************************************************/

ObjectWin::ObjectWin()
{
  sToolBorder *tb;

  ObjectList = new sListControl;
  ObjectList->LeftCmd = 1;
  ObjectList->AddScrolling(0,1);
  AddChild(ObjectList);

  tb = new sToolBorder();
  tb->AddMenu("Objects",0);
  AddBorder(tb);
}

void ObjectWin::OnInit()
{
  UpdateList();
}

ObjectWin::~ObjectWin()
{
}

void ObjectWin::OnCalcSize()
{
  if(Childs)
  {
    SizeX = Childs->SizeX;
    SizeY = Childs->SizeY;
  }
}

void ObjectWin::OnLayout()
{
  if(Childs)
  {
    Childs->Position = Client;
  }
}

/****************************************************************************/

void ObjectWin::UpdateList()
{
  sInt i,max;
  ToolObject *to;

  ObjectList->ClearList();
  max = App->Objects->GetCount();
  for(i=0;i<max;i++)
  {
    to = App->Objects->Get(i);
    ObjectList->Add(to->Name);
  }
}

/****************************************************************************/

sBool ObjectWin::OnCommand(sU32 cmd)
{
  sInt i;
  ToolObject *to;
  ToolWindow *tw;
  switch(cmd)
  {
  case 1:
    i = ObjectList->GetSelect();
    if(i>=0 && i<App->Objects->GetCount())
    {
      to = App->Objects->Get(i);
      tw = App->FindActiveWindow(sCID_TOOL_VIEWWIN);
      if(tw)
        ((ViewWin *)tw)->SetObject(to);
    }
    return sTRUE;
  default:
    return sFALSE;
  }
}


/****************************************************************************/
/***                                                                      ***/
/***   object                                                             ***/
/***                                                                      ***/
/****************************************************************************/

EditWin::EditWin()
{
  TextControl = new sTextControl;
  TextControl->AddScrolling(0,1);
  TextControl->DoneCmd = 1;
  AddChild(TextControl);
  Doc = 0;
  
  /*
  tb = new sToolBorder();
  tb->AddMenu("Objects",0);
  AddBorder(tb);
  */
}

void EditWin::OnInit()
{

}

EditWin::~EditWin()
{
}

void EditWin::Tag()
{
  ToolWindow::Tag();
  sBroker->Need(Doc);
}

void EditWin::OnCalcSize()
{
  if(Childs)
  {
    SizeX = Childs->SizeX;
    SizeY = Childs->SizeY;
  }
}

void EditWin::OnLayout()
{
  if(Childs)
  {
    Childs->Position = Client;
  }
}

void EditWin::SetDoc(ToolDoc *td)
{
  if(td)
    sVERIFY(td->GetClass()==sCID_TOOL_EDITDOC);

  Doc = (EditDoc*) td;
  if(Doc)
    TextControl->SetText(Doc->Text);
}

sBool EditWin::OnCommand(sU32 cmd)
{
  if(cmd==1)
  {
    if(Doc)
    {
      sCopyString(Doc->Text,TextControl->GetText(),Doc->Alloc);
      App->GetChange();
    }
    return sTRUE;
  }
  else
  {
    return sFALSE;
  }
}

/****************************************************************************/
/****************************************************************************/

EditDoc::EditDoc()
{
  Alloc = 65536;
  Text = new sChar[Alloc];
  sCopyString(Text,"// new text\n// please code something cool\n\n",128);
}

EditDoc::~EditDoc()
{
  delete[] Text;
}

void EditDoc::Clear()
{
  Text[0] = 0;
}

void EditDoc::Tag()
{
  ToolDoc::Tag();
}

sBool EditDoc::Write(sU32 *&data)
{
  sU32 *hdr;
  hdr = sWriteBegin(data,sCID_TOOL_EDITDOC,1);
  *data++ = Alloc;
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;
  sWriteString(data,Text);
  sWriteEnd(data,hdr);
  return sTRUE;
}

sBool EditDoc::Read(sU32 *&data)
{
  sInt version;
  delete[] Text;
  version = sReadBegin(data,sCID_TOOL_EDITDOC);
  if(version<1) return sFALSE;  
  Alloc = *data++;
  data+=3;
  Text = new sChar[Alloc];
  sReadString(data,Text,Alloc);
  return sReadEnd(data);
}

void EditDoc::Update(ToolObject *)
{
}

/****************************************************************************/
/***                                                                      ***/
/***   project                                                            ***/
/***                                                                      ***/
/****************************************************************************/

PerfMonWin::PerfMonWin()
{
  Mode = 0;
  AddScrolling(0,1);
}

PerfMonWin::~PerfMonWin()
{
}

void PerfMonWin::OnKey(sU32 key)
{
  switch(key)
  {
  case '1':
  case '2':
  case '3':
  case '4':
    Mode = (key-1)&15;
    break;
  }
}

void PerfMonWin::OnPaint()
{
  sInt i,h;
  sInt avgmax;
  sPerfMonRecord *rec;
  sPerfMonZone2 *zone;
  sInt x,y,x0,x1,xs;
  sChar buffer[128];
  sInt stack[128];
  sInt sp;
  sRect r;
  sInt t0;


  sPainter->Paint(sGui->FlatMat,Client,0xff808080);
  x = Client.x0;
  y = Client.y0;

  switch(Mode)
  {
  case 0:

// prepare

    avgmax = 0;
    zone = sPerfMon->Tokens;
    for(i=0;i<sPerfMon->TokenCount;i++) 
    {
      zone->EnterTime=0;
      zone->TotalTime=0;
      zone++;
    }
    avgmax = sPerfMon->Tokens[0].AverageTime;
    if(avgmax<10000)
      avgmax = 10000;

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

// done

    zone = sPerfMon->Tokens;
    for(i=0;i<sPerfMon->TokenCount;i++) 
    {
      zone->AverageTime=(zone->AverageTime*3 + zone->TotalTime*1)/4;
  //   if(zone->AverageTime>0)
      {
        sSPrintF(buffer,sizeof(buffer),"%14s  %6d %6d",zone->Name,zone->AverageTime,zone->TotalTime);
        sPainter->Print(sGui->FixedFont,x,y,buffer,zone->Color);
        y+=sPainter->GetHeight(sGui->FixedFont)+1;
      }
      zone++;
    }
    break;

  case 1:
    r = Client;
    r.x0 += sPainter->GetWidth(sGui->FixedFont,"123 ");
    h = sPainter->GetHeight(sGui->FixedFont);
    y = sPainter->PrintM(sGui->FixedFont,r,sFA_LEFT|sFA_TOP|sFA_SPACE,App->CodeGen.Source,sGui->Palette[sGC_TEXT]);
    for(i=0;i*h<y-Client.y0;i++)
    {
      if(Client.y0+i*h > ClientClip.y0 && Client.y0+i*h+h < ClientClip.y1)
      {
        sSPrintF(buffer,sizeof(buffer),"%03d",i+1);
        sPainter->Print(sGui->FixedFont,Client.x0,Client.y0+i*h,buffer,sGui->Palette[sGC_TEXT]);
      }
    }
    break;

  default:
    y = Client.y0;
  }
  SizeY = y-Client.y0;
}

/****************************************************************************/

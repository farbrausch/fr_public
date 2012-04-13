// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "winpara.hpp"
#include "winpage.hpp"
#include "wintime.hpp"
#include "winview.hpp"

/****************************************************************************/

#define CMD_EDITPARA        0x100       // used by pagewin
#define CMD_UPDATETIME      0x101   // call SetTime() again
#define CMD_RECOMPILETIME   0x102
#define CMD_ADDANIM         0x103
#define CMD_

#define CMD_DELANIM         0x200     // + the anim-channel to delete

/****************************************************************************/
/***                                                                      ***/
/***   Parameter Window                                                   ***/
/***                                                                      ***/
/****************************************************************************/

ParaWin::ParaWin()
{
  Op = 0;
  Doc = 0;
  OpTime = 0;
  DocTime = 0;
  Grid = new sGridFrame;
  Grid->SetGrid(12,4,0,sPainter->GetHeight(sGui->PropFont)+4);
  Grid->AddScrolling(0,1);
  AddChild(Grid);
  ShowEvent = 0;
}

ParaWin::~ParaWin()
{
}

void ParaWin::Tag()
{
  ToolWindow::Tag();
  sBroker->Need(Op);
  sBroker->Need(Doc);
}

void ParaWin::OnCalcSize()
{
  if(Childs)
  {
    SizeX = Childs->SizeX;
    SizeY = Childs->SizeY;
  }
}

void ParaWin::OnLayout()
{
  if(Childs)
  {
    Childs->Position = Client;
  }
}

void ParaWin::SetOp(PageOp *op,PageDoc *doc)
{
  sInt i,line;
  sControl *con;
  sControlTemplate *info;
  sControlTemplate temp;

  Op = op;
  Doc = doc;
  View = 0;
  OpTime = 0;
  DocTime = 0;
  Grid->RemChilds();
  Grid->SetGrid(12,4,0,sPainter->GetHeight(sGui->PropFont)+4);

  if(!op || !op->Class) 
  {
    Op = 0;
    return;
  }

  line = 0;
  for(i=0;i<MAX_PAGEOPPARA && op->Class->Para[i].Name;i++)
  {
    info = &op->Class->Para[i];

    temp = *info;
    temp.Type = sCT_LABEL;
    temp.YPos = line;
    con = new sControl;
    con->LayoutInfo.Init(0,temp.YPos,2,temp.YSize+temp.YPos);
    con->InitTemplate(&temp,CMD_EDITPARA,&op->Data[i].Data[0]);
    con->Style |= sCS_RIGHTLABEL;
    Grid->AddChild(con);
    
    temp = *info;
    temp.Name = 0;
    temp.YPos = line;
    con = new sControl;
    con->LayoutInfo.Init(2,temp.YPos,12,temp.YSize+temp.YPos);

    if(op->Data[i].Anim)
    {
      con->EditString(CMD_EDITPARA,op->Data[i].Anim,0,MAX_ANIMSTRING);
      Grid->AddChild(con);
      line++;
    }
    else if(temp.Type==sCT_CYCLE || temp.Type==sCT_CHOICE)
    {
      line += temp.AddFlags(Grid,CMD_EDITPARA,&op->Data[i].Data[0]);
    }
    else
    {
      sVERIFY(temp.Type!=sCT_STRING);
      con->InitTemplate(&temp,CMD_EDITPARA,&op->Data[i].Data[0]);
//      con->LabelWidth = 128;
      Grid->AddChild(con);
      line++;
    }
  }
  Grid->Flags |= sGWF_LAYOUT;
}


void ParaWin::SetTime(TimeOp *op,TimeDoc *doc)
{
  sControl *con;
  sInt line;
  sU32 cmd;
//  sInt srt;
//  sChar *typenam[4] = { "","camera","mesh","event" };

  OpTime = op;
  DocTime = doc;
  View = 0;
  Op = 0;
  Doc = 0;
  cmd = CMD_RECOMPILETIME;
  Grid->RemChilds();
  Grid->SetGrid(24,4,0,sPainter->GetHeight(sGui->PropFont)+4);

  if(!op) 
    return;
  if(!op->Anim)
    return;

  line = 0;

  op->Anim->Update();

  con = new sControl;
  con->LayoutInfo.Init(0,line,4,line+1);
  con->Label("Root");
  con->Style |= sCS_RIGHTLABEL;
  Grid->AddChild(con);
  con = new sControl;
  con->LayoutInfo.Init(4,line,14,line+1);
  con->EditString(cmd,op->Anim->RootName,0,sizeof(op->Anim->RootName));
  con->Style |= sCS_RIGHTLABEL;
  Grid->AddChild(con);
  con = new sControl;
  con->LayoutInfo.Init(14,line,24,line+1);
  con->EditCycle(cmd,&op->Anim->PaintRoot,0,"Hide|Paint");
  con->Style |= sCS_RIGHTLABEL;
  Grid->AddChild(con);
  line++;

  con = new sControl;
  con->LayoutInfo.Init(0,line,4,line+1);
  con->Label("Start / End");
  con->Style |= sCS_RIGHTLABEL;
  Grid->AddChild(con);
  con = new sControl;
  con->LayoutInfo.Init(4,line,14,line+1);
  con->EditHex(cmd,(sU32 *)&op->x0,0);
  con->InitNum(0,0x10000000,0x1000,0x00000000);
  con->Style |= sCS_RIGHTLABEL;
  Grid->AddChild(con);
  con = new sControl;
  con->LayoutInfo.Init(14,line,24,line+1);
  con->EditHex(cmd,(sU32 *)&op->x1,0);
  con->InitNum(0,0x10000000,0x1000,0x00100000);
  con->Style |= sCS_RIGHTLABEL;
  Grid->AddChild(con);
  line++;

  con = new sControl;
  con->LayoutInfo.Init(0,line,4,line+1);
  con->Label("Additinal Name");
  con->Style |= sCS_RIGHTLABEL;
  Grid->AddChild(con);
  con = new sControl;
  con->LayoutInfo.Init(4,line,24,line+1);
  con->EditString(cmd,op->Anim->Name,0,sizeof(op->Anim->Name));
  con->Style |= sCS_RIGHTLABEL;
  Grid->AddChild(con);
  line++;
  /*
  con = new sControl;
  con->LayoutInfo.Init(0,line,4,line+1);
  con->Label("Type");
  con->Style |= sCS_RIGHTLABEL;
  Grid->AddChild(con);
  con = new sControl;
  con->LayoutInfo.Init(4,line,24,line+1);
  con->EditChoice(0,&op->Type,0,"off|camera|object");
  con->Style |= sCS_RIGHTLABEL;
  Grid->AddChild(con);
  line++;
*/
  
  con = new sControl;
  con->LayoutInfo.Init(0,line,1,line+1);
  con->EditBool(CMD_UPDATETIME,&ShowEvent,ShowEvent?"<<":">>");
  Grid->AddChild(con);

  if(ShowEvent)
  {
    con = new sControl;
    con->LayoutInfo.Init(1,line,4,line+1);
    con->Label("Event Velocity");
    con->Style |= sCS_RIGHTLABEL;
    Grid->AddChild(con);
    con = new sControl;
    con->LayoutInfo.Init(4,line,24,line+1);
    con->EditFixed(cmd,&op->Velo,0);
    con->InitNum(-0x1000000,0x1000000,0x1000,0x10000);
    con->Style |= sCS_RIGHTLABEL;
    Grid->AddChild(con);
    line++;

    con = new sControl;
    con->LayoutInfo.Init(0,line,4,line+1);
    con->Label("Event Modulation");
    con->Style |= sCS_RIGHTLABEL;
    Grid->AddChild(con);
    con = new sControl;
    con->LayoutInfo.Init(4,line,24,line+1);
    con->EditFixed(cmd,&op->Mod,0);
    con->InitNum(-0x1000000,0x1000000,0x1000,0x00000);
    con->Style |= sCS_RIGHTLABEL;
    Grid->AddChild(con);
    line++;

    con = new sControl;
    con->LayoutInfo.Init(0,line,4,line+1);
    con->Label("Event Scale");
    con->Style |= sCS_RIGHTLABEL;
    Grid->AddChild(con);
    con = new sControl;
    con->LayoutInfo.Init(4,line,24,line+1);
    con->EditFloat(cmd,&op->SRT[0],0);
    con->Zones = 3;
    con->InitNum(-256,256,0.01f,1.0f);
    con->Style |= sCS_RIGHTLABEL;
    Grid->AddChild(con);
    line++;

    con = new sControl;
    con->LayoutInfo.Init(0,line,4,line+1);
    con->Label("Event Rotate");
    con->Style |= sCS_RIGHTLABEL;
    Grid->AddChild(con);
    con = new sControl;
    con->LayoutInfo.Init(4,line,24,line+1);
    con->EditFloat(cmd,&op->SRT[3],0);
    con->Zones = 3;
    con->InitNum(-256,256,0.01f,0.0f);
    con->Style |= sCS_RIGHTLABEL;
    Grid->AddChild(con);
    line++;

    con = new sControl;
    con->LayoutInfo.Init(0,line,4,line+1);
    con->Label("Event Trans");
    con->Style |= sCS_RIGHTLABEL;
    Grid->AddChild(con);
    con = new sControl;
    con->LayoutInfo.Init(4,line,24,line+1);
    con->EditFloat(cmd,&op->SRT[6],0);
    con->Zones = 3;
    con->InitNum(-256,256,0.01f,0.0f);
    con->Style |= sCS_RIGHTLABEL;
    Grid->AddChild(con);
    line++;

    con = new sControl;
    con->LayoutInfo.Init(0,line,4,line+1);
    con->Label("Event Text");
    con->Style |= sCS_RIGHTLABEL;
    Grid->AddChild(con);
    con = new sControl;
    con->LayoutInfo.Init(4,line,24,line+1);
    con->EditString(cmd,op->Text,0,sizeof(op->Text));
    con->Style |= sCS_RIGHTLABEL;
    Grid->AddChild(con);
    line++;
  }

  if(op->Anim)
  {
    AnimDoc *anim;
    AnimChannel *channel;
    sInt i,j;
    static sInt dummy;
    static sChar *number[4] = {"x","y","z","w"};
    static sU32 rgba[4] = { 0xffff0000,0xff00ff00,0xff0000ff,0xffffffff };
    sChar *choices;

    line++;
    con = new sControl;
    con->LayoutInfo.Init(0,line,6,line+1);
    con->Label("Label");
    Grid->AddChild(con);
    con = new sControl;
    con->LayoutInfo.Init(6,line,10,line+1);
    con->Label("Parameter");
    Grid->AddChild(con);
    con = new sControl;
    con->LayoutInfo.Init(10,line,14,line+1);
    con->Label("Mode");
    Grid->AddChild(con);
    con = new sControl;
    con->LayoutInfo.Init(14,line,16,line+1);
    con->Label("Tension");
    Grid->AddChild(con);
    con = new sControl;
    con->LayoutInfo.Init(16,line,18,line+1);
    con->Label("Contiunity");
    Grid->AddChild(con);
    con = new sControl;
    con->LayoutInfo.Init(18,line,22,line+1);
    con->Label("Show Channel");
    Grid->AddChild(con);
    con = new sControl;
    con->LayoutInfo.Init(22,line,24,line+1);
    con->Label("Edit");
    Grid->AddChild(con);

    line++;
    anim = op->Anim;
    anim->Update();
    for(i=0;i<anim->Channels.Count;i++)
    {
      channel = &anim->Channels[i];
      con = new sControl;
      con->LayoutInfo.Init(0,line,6,line+1);
      con->EditString(CMD_UPDATETIME,channel->Name,0,sizeof(channel->Name));
      con->ChangeCmd = 0;
      Grid->AddChild(con);
      con = new sControl;
      con->LayoutInfo.Init(6,line,10,line+1);
      choices = 0;
      if(channel->Target)
        choices = OffsetName(channel->Target->GetClass(),-1);
      if(choices)
        con->EditChoice(CMD_UPDATETIME,&channel->Offset,0,choices);
      else
        con->Label("<< unknown class >>");

      Grid->AddChild(con);
      con = new sControl;
      con->LayoutInfo.Init(10,line,14,line+1);
      con->EditChoice(0,&channel->Mode,0,
        "+spline(time)|*spline(time)|+spline(time)*velo|*spline(time)*velo|"
        "*velo|+velo|+mod|*mod|s|r|t|spline(time)*s*velo|spline(time)*r*velo|spline(time)*t*velo|"
        "+spline(mousex)|+spline(mousey)|text");
      Grid->AddChild(con);

      con = new sControl;
      con->LayoutInfo.Init(14,line,16,line+1);
      con->EditFixed(cmd,&channel->Tension,0);
      con->InitNum(-0x100000,0x100000,0x0200,0x10000);
      Grid->AddChild(con);

      con = new sControl;
      con->LayoutInfo.Init(16,line,18,line+1);
      con->EditFixed(cmd,&channel->Continuity,0);
      con->InitNum(-0x100000,0x100000,0x0200,0x10000);
      Grid->AddChild(con);

      for(j=0;j<channel->Count;j++)
      {
        con = new sControl;
        con->LayoutInfo.Init(18+j,line,19+j,line+1);
        con->EditBool(cmd,&channel->Select[j],number[j]);
        con->TextColor = rgba[j];
        Grid->AddChild(con);
      }

      con = new sControl;
      con->LayoutInfo.Init(22,line,24,line+1);
      con->Button("del",CMD_DELANIM+i);
      Grid->AddChild(con);
      line++;
    }
    con = new sControl;
    con->LayoutInfo.Init(22,line,24,line+1);
    con->Button("add",CMD_ADDANIM);
    Grid->AddChild(con);
  }  

//  Grid->SetGrid(12,line,0,sPainter->GetHeight(sGui->PropFont)+4);

  Grid->Flags |= sGWF_LAYOUT;
}


void ParaWin::SetView(ViewWin *win)
{
  sControl *con;
  sInt line;

  OpTime = 0;
  DocTime = 0;
  View = win;
  Op = 0;
  Doc = 0;
  Grid->RemChilds();
  Grid->SetGrid(24,4,0,sPainter->GetHeight(sGui->PropFont)+4);

  if(!win) 
    return;

  line = 0;

  con = new sControl;
  con->LayoutInfo.Init(0,line,6,line+1);
  con->Label("Position");
  con->Style |= sCS_RIGHTLABEL;
  Grid->AddChild(con);
  con = new sControl;
  con->LayoutInfo.Init(6,line,24,line+1);
  con->EditFloat(0,&win->Cam.l.x,0);
  con->Zones = 3;
  con->InitNum(-256,256,0.025f,0); 
  Grid->AddChild(con);
  line++;
/*
  con = new sControl;
  con->LayoutInfo.Init(0,line,6,line+1);
  con->Label("Rotation");
  con->Style |= sCS_RIGHTLABEL;
  Grid->AddChild(con);
  con = new sControl;
  con->LayoutInfo.Init(6,line,24,line+1);
  con->EditFloat(0,&win->CamAngle.x,0);
  con->Zones = 3;
  con->InitNum(-256,256,0.025f,0); 
  Grid->AddChild(con);
  line++;
*/
  con = new sControl;
  con->LayoutInfo.Init(0,line,6,line+1);
  con->Label("Zoom");
  con->Style |= sCS_RIGHTLABEL;
  Grid->AddChild(con);
  con = new sControl;
  con->LayoutInfo.Init(6,line,24,line+1);
  con->EditFloat(0,&win->CamZoom,0);
  con->InitNum(-256,256,0.125f,0);
  Grid->AddChild(con);
  line++;

  con = new sControl;
  con->LayoutInfo.Init(0,line,6,line+1);
  con->Label("Mode");
  con->Style |= sCS_RIGHTLABEL;
  Grid->AddChild(con);
  con = new sControl;
  con->LayoutInfo.Init(6,line,24,line+1);
  con->EditChoice(0,&win->MeshWire,0,"solid|wire");
  Grid->AddChild(con);
  line++;

  con = new sControl;
  con->LayoutInfo.Init(0,line,6,line+1);
  con->Label("Selection");
  con->Style |= sCS_RIGHTLABEL;
  Grid->AddChild(con);
  con = new sControl;
  con->LayoutInfo.Init(6,line,24,line+1);
  con->EditMask(0,(sInt *)&win->SelectMask,0,24,"efv");
  Grid->AddChild(con);
  line++;

  con = new sControl;
  con->LayoutInfo.Init(0,line,6,line+1);
  con->Label("Clear Color");
  con->Style |= sCS_RIGHTLABEL;
  Grid->AddChild(con);
  con = new sControl;
  con->LayoutInfo.Init(6,line,24,line+1);
  con->Zones = 3;
  con->EditRGB(0,(sInt *)&win->ClearColor,0);
  Grid->AddChild(con);
  line++;

  Grid->Flags |= sGWF_LAYOUT;
}

/****************************************************************************/

sBool ParaWin::OnCommand(sU32 cmd)
{
  switch(cmd)
  {
  case CMD_EDITPARA:
    if(Op)
    {
      Op->ChangeCount = App->GetChange();
      if((Op->Class->Flags & (POCF_STORE|POCF_LOAD|POCF_LABEL)) && Doc) // store
      {
        Doc->UpdatePage();
      }
    }
    return sTRUE;
  case CMD_UPDATETIME:
    SetTime(OpTime,DocTime);
    return sTRUE;

  case CMD_RECOMPILETIME:
    App->DemoPrev();
    return sTRUE;

  case CMD_ADDANIM:
    if(OpTime && OpTime->Anim)
      OpTime->Anim->AddChannel();
    SetTime(OpTime,DocTime);
    return sTRUE;

  default:
    if(cmd>=CMD_DELANIM && cmd<CMD_DELANIM+0x100)
    {
      if(OpTime && OpTime->Anim)
        OpTime->Anim->RemChannel(cmd-CMD_DELANIM);
      SetTime(OpTime,DocTime);
    }
    return sFALSE;
  }
}


/****************************************************************************/
/****************************************************************************/

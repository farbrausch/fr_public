// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "apptool.hpp"
#include "_xsi.hpp"

#include "winpara.hpp"
#include "winpage.hpp"
#include "winproject.hpp"
#include "winview.hpp"
#include "wintime.hpp"

#include "genbitmap.hpp"
#include "genmesh.hpp"
#include "appbrowser.hpp"

/****************************************************************************/

#define CMD_FILEMENU        0x0100

#define CMD_VIEWMENU        0x0120
#define CMD_VIEW_TEX6       0x0121
#define CMD_VIEW_TEX7       0x0122
#define CMD_VIEW_TEX8       0x0123
#define CMD_VIEW_TEX9       0x0124
#define CMD_VIEW_TEX10      0x0125
#define CMD_VIEW_SCROLLBARS 0x0126
#define CMD_VIEW_MAXTITLE   0x0127
#define CMD_VIEW_MINIMIZE   0x0128
#define CMD_VIEW_DOCKER     0x0129
#define CMD_VIEW_RESETWINDOWS  0x012a
#define CMD_VIEW_DOCKER2    0x012b

#define CMD_VIEW_SET0       0x0130
#define CMD_VIEW_SET1       0x0131
#define CMD_VIEW_SET2       0x0132
#define CMD_VIEW_SET3       0x0133

#define CMD_VIEW_RES0       0x0140
#define CMD_VIEW_RES1       0x0141
#define CMD_VIEW_RES2       0x0142
#define CMD_VIEW_RES3       0x0143
#define CMD_VIEW_RES4       0x0144
#define CMD_VIEW_TURBO      0x014f

#define CMD_VIEW_DEMOPLAY   0x0150
#define CMD_VIEW_DEMOSTOP   0x0151
#define CMD_VIEW_DEMOLOOP   0x0152
#define CMD_VIEW_DEMOERROR  0x0153

#define CMD_VIEW_FLUSH      0x015f

#define CMD_FILE_EXPORTASC  0x0160
#define CMD_FILE_EXPORTBIN  0x0161
#define CMD_FILE_EXPORTPAC  0x0162

#define CMD_PANIC           0x0200

/****************************************************************************/
/***                                                                      ***/
/***   Object                                                             ***/
/***                                                                      ***/
/****************************************************************************/

ToolObject::ToolObject()
{
  Name[0] = 0;
  Doc = 0;
  CID = 0;
  NeedsCache = 0;
  ChangeCount = 0;
  CalcCount = 0;
  Cache = 0;
  GlobalLoad = -1;
  GlobalStore = -1;
  Snip = 0;
}

void ToolObject::Tag()
{
  sBroker->Need(Cache);
  sBroker->Need(Doc);
}

void ToolObject::Calc()
{
  ToolCodeGen *gen;
  sVERIFY(Doc);
  sVERIFY(Doc->App);

  if(Cache==0 || CalcCount<Doc->App->CheckChange())
  {
    gen = &Doc->App->CodeGen;
    gen->Begin();
    gen->AddResult(this);
    if(gen->End(Doc->App->PlayerCalc->SC))
    {
      CurrentGenPlayer = Doc->App->PlayerCalc;
      CurrentGenPlayer->TotalFrame++;
      gen->Execute(Doc->App->PlayerCalc->SR);
      CurrentGenPlayer = 0;
    }
    gen->Cleanup();
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Doc                                                                ***/
/***                                                                      ***/
/****************************************************************************/

ToolDoc::ToolDoc()
{
  Name[0] = 0;
  App = 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   windows                                                            ***/
/***                                                                      ***/
/****************************************************************************/

ToolWindow::ToolWindow() 
{
  AddBorder(new sFocusBorder);
  App = 0;
  WinId = 0;
  ToolBorder = 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Docker Window                                                      ***/
/***                                                                      ***/
/****************************************************************************/

DockerWindow::DockerWindow()
{
  App = 0;
  Flags |= sGWF_TOPMOST;
  PressedRect = -1;
  LastRect = 0;

  Root = 0;
  Choices = 0;
  ChoiceCount = 0;
  WinCount = 0;
}

/****************************************************************************/

void DockerWindow::PaintR(sGuiWindow *win,sInt ci)
{
  sInt i;
  sRect r;

  switch(win->GetClass())
  {
  case sCID_VSPLITFRAME:
  case sCID_HSPLITFRAME:
    for(i=0;i<win->GetChildCount();i++)
    {
      PaintR(win->GetChild(i),i);
    }
    break;
  default:
    r.x0 = Client.x0 + (win->Position.x0-Root->ClientClip.x0)*Client.XSize()/Root->ClientClip.XSize();
    r.y0 = Client.y0 + (win->Position.y0-Root->ClientClip.y0)*Client.YSize()/Root->ClientClip.YSize();
    r.x1 = Client.x0 + (win->Position.x1-Root->ClientClip.x0)*Client.XSize()/Root->ClientClip.XSize();
    r.y1 = Client.y0 + (win->Position.y1-Root->ClientClip.y0)*Client.YSize()/Root->ClientClip.YSize();
    Windows[WinCount].Window = win;
    Windows[WinCount].Rect = r;
    Windows[WinCount].ChildIndex = ci;
    WinCount++;
    break;
  }
}

void DockerWindow::OnPaint()
{
  sInt i;

  WinCount = 0;
  PaintR(Root->GetChild(0),0);

  sPainter->Paint(sGui->FlatMat,Client,sGui->Palette[sGC_BACK]);
  for(i=0;i<WinCount;i++)
    sGui->Button(Windows[i].Rect,PressedRect==i,((ToolWindow *)Windows[i].Window)->ShortName);
}

void DockerWindow::OnDrag(sDragData &dd)
{
  sInt i;

  switch(dd.Mode)
  {
  case sDD_START:
    PressedRect = -1;
    for(i=0;i<WinCount;i++)
      if(Windows[i].Rect.Hit(dd.MouseX,dd.MouseY))
        PressedRect = i;
    if(PressedRect!=-1 && (dd.Buttons&2))
    {
      LastRect = PressedRect;
      sGui->Post(1,this);
    }
    break;
  case sDD_STOP:
    PressedRect = -1;
    break;
  }
}

void DockerWindow::OnKey(sU32 key)
{
  switch(key&0x8001ffff)
  {
  case sKEY_ENTER:
  case sKEY_ESCAPE:
  case sKEY_APPCLOSE:
    Parent->RemChild(this);
    break;
  case 'h':
    sGui->Post(2,this);
    break;
  case 'v':
    sGui->Post(3,this);
    break;
  case 'c':
    sGui->Post(4,this);
    break;
  case sKEY_DELETE:
    sGui->Post(5,this);
    break;
  }
}

sBool DockerWindow::OnCommand(sU32 cmd)
{
  sMenuFrame *mf;
  sInt i,j;
  sGuiWindow *win;
  sGuiWindow *parent,*remwin,*anywin;

  anywin = 0;
  for(i=0;i<ChoiceCount && anywin==0;i++)
  {
    if(Choices[i] && Choices[i]->Parent==0)
      anywin = Choices[i];
  }
  switch(cmd)
  {
  case 1:
    mf = new sMenuFrame;
    mf->SendTo = this;
    mf->AddBorder(new sNiceBorder);

    mf->AddMenu("Horizontal Split",2,'h');
    mf->AddMenu("Vertical Split",3,'v');
    mf->AddMenu("Change Window..",4,'c');
    mf->AddMenu("Delete Window",5,sKEY_DELETE);
    sGui->AddPopup(mf);
    return sTRUE;

  case 2:
    if(LastRect>=0 && LastRect<WinCount && anywin)
    {
      win = Windows[LastRect].Window;
      parent = win->Parent;
      if(parent->GetClass()==sCID_HSPLITFRAME)
      {
        ((sHSplitFrame *)parent)->SplitChild(Windows[LastRect].ChildIndex,anywin);
      }
      else
      {
        remwin = new sHSplitFrame;
        parent->SetChild(win,remwin);
        remwin->AddChild(win);
        remwin->AddChild(anywin);
      }
      parent->Flags |= sGWF_LAYOUT;
    }
    return sTRUE;

  case 3:
    if(LastRect>=0 && LastRect<WinCount && anywin)
    {
      win = Windows[LastRect].Window;
      parent = win->Parent;
      if(parent->GetClass()==sCID_VSPLITFRAME)
      {
        ((sVSplitFrame *)parent)->SplitChild(Windows[LastRect].ChildIndex,anywin);
      }
      else
      {
        remwin = new sVSplitFrame;
        parent->SetChild(win,remwin);
        remwin->AddChild(win);
        remwin->AddChild(anywin);
      }
      parent->Flags |= sGWF_LAYOUT;
    }
    return sTRUE;

  case 4:
    mf = new sMenuFrame;
    mf->SendTo = this;
    mf->AddBorder(new sNiceBorder);
    for(i=0;i<ChoiceCount;i++)
      if(Choices[i] && Choices[i]->Parent==0)
        mf->AddMenu(((ToolWindow *)Choices[i])->ShortName,i+16,0);
    sGui->AddPopup(mf);
    return sTRUE;

  case 5:
    if(LastRect>=0 && LastRect<WinCount)
    {
      win = Windows[LastRect].Window;
      parent = win->Parent;
      if(parent->GetClass()==sCID_HSPLITFRAME || parent->GetClass()==sCID_VSPLITFRAME)
      {
        parent->RemChild(win);
        if(parent->GetChildCount()<2)
        {
          sVERIFY(parent->GetChildCount()==1);
          remwin = parent;
          parent = remwin->Parent;
          win = remwin->GetChild(0);
          remwin->RemChild(win);
          parent->SetChild(remwin,win);
        }
        parent->Flags |= sGWF_LAYOUT;
        sGui->SetFocus(parent);
      }
    }

    return sTRUE;

  default:
    if(cmd>=16 && (sInt)cmd<16+ChoiceCount)
    {
      if(LastRect>=0 && LastRect<WinCount)
      {
        win = Windows[LastRect].Window;
        i = Windows[LastRect].ChildIndex;
        j = cmd-16;
        sVERIFY(win->Parent);
        sVERIFY(win->Parent->GetChildCount()>i);
        sVERIFY(win->Parent->GetChild(i)==win);
        sVERIFY(Choices && j>=0 && j<ChoiceCount);
        sVERIFY(Choices[j]);
        win->Parent->Flags |= sGWF_LAYOUT;
        win->Parent->SetChild(i,Choices[j]);
      }
      return sTRUE;
    }
    return sFALSE;
  }
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Application                                                        ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

sToolApp::sToolApp()
{
  sToolBorder *tb;
  sInt i;

// stuff

  LogMode = 0;
  ChangeCount = 2;
  ChangeMode = 0;
  TextureSize = 8;
  ScreenRes = 0;
  TurboMode = 0;

  Windows = new sList<ToolWindow>(64);
  Docs = new sList<ToolDoc>(64);
  Objects = new sList<ToolObject>(64);
  CurrentLoadStore = new sList<ToolObject>(64);

  PlayerCalc = new GenPlayer;
  Player = new GenPlayer;
  CodeGen.Init();
  CodeGen.App = this;

  MakeClassList();
  Docker = new DockerWindow;
  Docker->App = this;
  Docker->AddTitle("Window Set");
  Docker->Position.Init(0,0,640,480);
//  DemoPrev();

// Borders

  Status = new sStatusBorder;
  AddBorder(Status);
  Status->SetTab(0,Stat[0],250);
  Status->SetTab(1,Stat[1],250);
  Status->SetTab(2,Stat[2],0);
  Status->SetTab(3,Stat[3],150);
  Status->SetTab(4,Stat[4],50);
  sSPrintF(Stat[0],sizeof(Stat[0]),"");
  sSPrintF(Stat[1],sizeof(Stat[1]),"");
  sSPrintF(Stat[2],sizeof(Stat[2]),"");
  sSPrintF(Stat[3],sizeof(Stat[3]),"");
  sSPrintF(Stat[4],sizeof(Stat[4]),"");

  TimeBorder = new GenTimeBorder;
  TimeBorder->Player = Player;
  AddBorder(TimeBorder);

  tb = new sToolBorder;
  tb->AddMenu("File",CMD_FILEMENU);
  tb->AddMenu("View",CMD_VIEWMENU);
  AddBorder(tb);

  FileWindow = new sFileWindow;
  FileWindow->AddTitle("File Operations");
  FileWindow->Flags |= sGWF_TOPMOST;
  {
    sChar buffer[4096];

    sCopyString(buffer,"disks/",sizeof(buffer));
    sSystem->GetCurrentDir(buffer+6,sizeof(buffer)-6);
    FileWindow->SetPath(buffer);
    sAppendString(buffer,"/default.scl",sizeof(buffer));
    FileWindow->SetPath(buffer);
  }

// window sets

  sSetMem(WinSetStorage,0,sizeof(WinSetStorage));

  WinSetStorage[ 0] = new ViewWin;      WinSetStorage[ 0]->ShortName = "view 1";
  WinSetStorage[ 1] = new ViewWin;      WinSetStorage[ 1]->ShortName = "view 2";
  WinSetStorage[ 2] = new ViewWin;      WinSetStorage[ 2]->ShortName = "view 3";
  WinSetStorage[ 3] = new ViewWin;      WinSetStorage[ 3]->ShortName = "view 4";
  WinSetStorage[ 4] = new PageWin;      WinSetStorage[ 4]->ShortName = "page 1";
  WinSetStorage[ 5] = new PageWin;      WinSetStorage[ 5]->ShortName = "page 2";
  WinSetStorage[ 6] = new PageWin;      WinSetStorage[ 6]->ShortName = "page 3";
  WinSetStorage[ 7] = new PageWin;      WinSetStorage[ 7]->ShortName = "page 4";
  WinSetStorage[ 8] = new ParaWin;      WinSetStorage[ 8]->ShortName = "para 1";
  WinSetStorage[ 9] = new ParaWin;      WinSetStorage[ 9]->ShortName = "para 2";
  WinSetStorage[10] = new ProjectWin;   WinSetStorage[10]->ShortName = "docs";
  WinSetStorage[11] = new ProjectWin;   WinSetStorage[11]->ShortName = "docs 2";
  WinSetStorage[12] = new ObjectWin;    WinSetStorage[12]->ShortName = "objects";
  WinSetStorage[13] = new ObjectWin;    WinSetStorage[13]->ShortName = "objects 2";
  WinSetStorage[14] = new PerfMonWin;   WinSetStorage[14]->ShortName = "perfmon";
  WinSetStorage[15] = new PerfMonWin;   WinSetStorage[15]->ShortName = "perfmon 2";
  WinSetStorage[16] = new EditWin;      WinSetStorage[16]->ShortName = "edit 1";
  WinSetStorage[17] = new EditWin;      WinSetStorage[17]->ShortName = "edit 2";
  WinSetStorage[18] = new EditWin;      WinSetStorage[18]->ShortName = "edit 3";
  WinSetStorage[19] = new EditWin;      WinSetStorage[19]->ShortName = "edit 4";
  WinSetStorage[20] = new TimeWin;      WinSetStorage[20]->ShortName = "time 1";
  WinSetStorage[21] = new TimeWin;      WinSetStorage[21]->ShortName = "time 2";
  WinSetStorage[22] = new TimeWin;      WinSetStorage[22]->ShortName = "time 3";
  WinSetStorage[23] = new TimeWin;      WinSetStorage[23]->ShortName = "time 4";
  WinSetStorage[24] = new AnimWin;      WinSetStorage[24]->ShortName = "anim 1";
  WinSetStorage[25] = new AnimWin;      WinSetStorage[25]->ShortName = "anim 2";
  WinSetStorage[26] = new AnimWin;      WinSetStorage[26]->ShortName = "anim 3";
  WinSetStorage[27] = new AnimWin;      WinSetStorage[27]->ShortName = "anim 4";
  for(i=0;WinSetStorage[i];i++)
  {
    WinSetStorage[i]->WinId = i;
    WinSetStorage[i]->App = this;
    WinSetStorage[i]->OnInit();
    Windows->Add(WinSetStorage[i]);
  }
  WindowSet = -1;

// childs

  SecondWindow = 0;
  if(sSystem->GetScreenCount()>1)
  {
    SecondWindow = new sDummyFrame;
    SecondWindow->AddTitle("Genthree secondary window");
    SecondWindow->Position.Init(0,0,640,480);
    SecondWindow->FindTitle()->Maximise(1);
    sGui->AddApp(SecondWindow,1);
  }
  OnCommand(CMD_VIEW_RESETWINDOWS);

// inital objects for childs

  sGui->Post(sCMDLS_READ,this);
}

sToolApp::~sToolApp()
{
  DeleteClassList();
  CodeGen.Exit();
}

void sToolApp::Tag()
{
  sInt i;

  sGuiWindow::Tag();
  sBroker->Need(Docs);
  sBroker->Need(Windows);
  sBroker->Need(Objects);
  sBroker->Need(Player);
  sBroker->Need(PlayerCalc);
  sBroker->Need(CurrentLoadStore);
  sBroker->Need(FileWindow);
  sBroker->Need(Docker);
  for(i=0;i<sDW_MAXWINDOW;i++)
    sBroker->Need(WinSetStorage[i]);
}


sBool sToolApp::OnDebugPrint(sChar *text)
{
  if(sCmpString(text,"@[0]")==0) 
  {
    LogMode = 0;
    return sTRUE;
  }
  if(sCmpString(text,"@[1]")==0) 
  {
    LogMode = 1;
//    DebugInit->ClearText();
    return sTRUE;
  }
  if(sCmpString(text,"@[2]")==0) 
  {
    LogMode = 2;
//    DebugTick->ClearText();
    return sTRUE;
  }
  if(LogMode == 1)
  {
//    DebugInit->Log(text);
    return sTRUE;
  }
  if(LogMode == 2)
  {
//    DebugTick->Log(text);
    return sTRUE;
  }
  return sFALSE;
}

void sToolApp::MakeClassList()
{
  sInt i,j;
  SCFunction *func;
  SCSymbol *sym;
  SCFuncArg *arg;
  PageOpClass *cl;
  sU32 cid;

  CodeGen.Begin();
  CodeGen.End(PlayerCalc->SC,0,0);
  CodeGen.Cleanup();

  ClassCount = 0;
  ClassList = new PageOpClass[512];

// other

  sym = PlayerCalc->SC->GetSymbols();
  while(sym)
  {
    if(sym->Kind==SCS_FUNC)
    {
      func = (SCFunction *) sym;
      cl = &ClassList[ClassCount++];
      cl->Init(func->Name,func->Index);
      sCopyMem4(cl->InputCID,func->OStackIn,4);
      for(i=0;i<4;i++)
        if(cl->InputCID[i]!=0)
          cl->InputCount=i+1;
      cl->OutputCID = func->OStackOut[0];
      cl->Shortcut = func->Shortcut;
      if(cl->Shortcut>='A' && cl->Shortcut<='Z')
        cl->Shortcut |= sKEYQ_SHIFT;
      cl->InputFlags = func->Flags;

      arg = func->Args;
      while(arg)
      {
        i = cl->AddPara(arg->Name,0);
        if(arg->Type->Base == SCT_INT || arg->Type->Base == SCT_FLOAT)
        {
          if(arg->Flags & SCFA_FLOAT)
          {   
            cl->Para[i].Type = sCT_FIXED;
            cl->Para[i].Step = 0x00002000;
            if(arg->Flags & SCFA_SPEED)
              cl->Para[i].Step = arg->GuiSpeed;
            for(j=0;j<4;j++)
            {
              cl->Para[i].Min[j] = -0x40000000;
              cl->Para[i].Max[j] = 0x40000000;
              if(arg->Flags & SCFA_DEFAULT)
                cl->Para[i].Default[j] = arg->Default[j];
              if(arg->Flags & SCFA_MIN)
                cl->Para[i].Min[j] = arg->GuiMin[j];
              if(arg->Flags & SCFA_MAX)
                cl->Para[i].Max[j] = arg->GuiMax[j];
            }
          }
          else
          {
            cl->Para[i].Type = sCT_INT;
            cl->Para[i].Step = 0x00002000/65536.0f;
            if(arg->Flags & SCFA_SPEED)
              cl->Para[i].Step = arg->GuiSpeed/65536.0f;
            for(j=0;j<4;j++)
            {
              cl->Para[i].Min[j] = -0x8000;
              cl->Para[i].Max[j] = 0x7fff;
              if(arg->Flags & SCFA_DEFAULT)
                cl->Para[i].Default[j] = arg->Default[j]/65536.0f;
              if(arg->Flags & SCFA_MIN)
                cl->Para[i].Min[j] = arg->GuiMin[j]/65536.0f;
              if(arg->Flags & SCFA_MAX)
                cl->Para[i].Max[j] = arg->GuiMax[j]/65536.0f;
            }
          }
          if(arg->Type->Count>1)
            cl->Para[i].Zones = arg->Type->Count;

          if(arg->Flags & SCFA_SPECIAL)
          {
            if(arg->GuiSpecial[0]=='#')
            {
              cl->Para[i].Type = sCT_CYCLE;
              cl->Para[i].Cycle = sDupString(arg->GuiSpecial+1);
            }
            if(sCmpString(arg->GuiSpecial,"rgba")==0)
            {
              cl->Para[i].Type = sCT_RGBA;
              cl->Para[i].Min[0] = cl->Para[i].Min[1] = cl->Para[i].Min[2] = cl->Para[i].Min[3] = 0;
              cl->Para[i].Max[0] = cl->Para[i].Max[1] = cl->Para[i].Max[2] = cl->Para[i].Max[3] = 0xffff;
            }
            if(sCmpMem(arg->GuiSpecial,"mask8",5)==0)
            {
              cl->Para[i].Type = sCT_MASK;
              cl->Para[i].Zones = 8;
              if(arg->GuiSpecial[5]==':')
                cl->Para[i].Cycle = arg->GuiSpecial+6;
            }
            if(sCmpMem(arg->GuiSpecial,"mask24",6)==0)
            {
              cl->Para[i].Type = sCT_MASK;
              cl->Para[i].Zones = 24;
              if(arg->GuiSpecial[6]==':')
                cl->Para[i].Cycle = arg->GuiSpecial+7;
            }
            if(sCmpMem(arg->GuiSpecial,"cycle:",6)==0)
            {
              cl->Para[i].Type = sCT_CYCLE;//CHOICE;
              cl->Para[i].Cycle = arg->GuiSpecial+6;
            }
            if(sCmpString(arg->GuiSpecial,"string")==0)
            {
              cl->Para[i].Type = sCT_STRING;
              cl->Para[i].Cycle = 0;
            }
            if(sCmpString(arg->GuiSpecial,"label")==0)
            {
              cl->Para[i].Type = sCT_STRING;
              cl->Para[i].Cycle = "1234";
            }
          }
        }
        else
        {
          cl->Para[i].Type = sCT_STRING;
        }
        arg = arg->Next;
        i++;
      }
    }

    sym = sym->Next;
  }

// load & store

  static sU32 classlist[]={sCID_GENMESH,sCID_GENBITMAP,sCID_GENSCENE,sCID_GENFXCHAIN,sCID_GENMATERIAL,sCID_GENSPLINE,0};
  for(i=0;classlist[i];i++)
  {
    cid = classlist[i];

    cl = &ClassList[ClassCount++];  
    cl->Init("load",0x10011+i*16);  
    cl->Flags |= POCF_LOAD;  
    cl->AddPara("var",sCT_STRING);  
    cl->Shortcut = 'l';  
    cl->OutputCID = cid;
    cl->InputFlags = SCFU_NOP;

    cl = &ClassList[ClassCount++];  
    cl->Init("store",0x10012+i*16); 
    cl->Flags |= POCF_STORE; 
    cl->AddPara("var",sCT_STRING);  
    cl->Shortcut = 's';  
    cl->InputCID[0] = cid;  
    cl->InputCount = 1;
    cl->OutputCID = cid;
    cl->InputFlags = SCFU_NOP;
  }

  for(i=0;i<ClassCount;i++)
  {
    cl = &ClassList[i];
    cl->Color = ClassColor(cl->OutputCID ? cl->OutputCID : cl->InputCID[0]);
  }
}

sU32 sToolApp::ClassColor(sU32 cid)
{
  sU32 col;

  switch(cid)
  {
  case sCID_GENMESH:
    col = 0xffffa0c0;
    break;
  case sCID_GENBITMAP:
    col = 0xffa0a0ff;
    break;
  case sCID_GENMATERIAL:
  case sCID_GENMATPASS:
    col = 0xffa0ffa0;
    break;
  case sCID_GENSCENE:
    col = 0xffffffa0;
    break;
	case sCID_GENFXCHAIN:
		col = 0xffffa070;
		break;
  default:
    col = 0xffc0c0c0;
    break;
  }
  return col;
}

void sToolApp::DeleteClassList()
{
  sInt i,j;

  for(i=0;i<ClassCount;i++)
  {
    if(ClassList[i].Name)
      delete[] ClassList[i].Name;
    for(j=0;j<MAX_PAGEOPPARA;j++)
      if(ClassList[i].Para[j].Name)
        delete[] ClassList[i].Para[j].Name;
  }
  delete[] ClassList;
  ClassList = 0;
  ClassCount = 0;
}

PageOpClass *sToolApp::FindPageOpClass(sInt func)
{
  sInt i;

  for(i=0;i<ClassCount;i++)
    if(ClassList[i].FuncId == func)
      return &ClassList[i];
  return &ClassList[0];
}


/****************************************************************************/


sBool sToolApp::OnShortcut(sU32 key)
{
  switch(key&0x8001ffff)
  {
  case sKEY_APPCLOSE:
    OnCommand(sCMDLS_EXIT);
    return sTRUE;
  case sKEY_PAUSE:
    OnCommand(CMD_PANIC);
    return sTRUE;
  case sKEY_F1:
    OnCommand(CMD_VIEW_SET0);
    return sTRUE;
  case sKEY_F2:
    OnCommand(CMD_VIEW_SET1);
    return sTRUE;
  case sKEY_F3:
    OnCommand(CMD_VIEW_SET2);
    return sTRUE;
  case sKEY_F4:
    OnCommand(CMD_VIEW_SET3);
    return sTRUE;
  case sKEY_F9:
    OnCommand(CMD_VIEW_DOCKER);
    return sTRUE;
  case sKEY_F10:
    OnCommand(CMD_VIEW_DOCKER2);
    return sTRUE;
  case sKEY_F11:
    OnCommand(CMD_VIEW_FLUSH);
    SetStat(2,"Flush!");
    return sTRUE;
  case sKEY_F12:
    OnCommand(sCMDLS_SAVE);
    SetStat(2,"Quicksave!");
    return sTRUE;
  case sKEY_F5:
    OnCommand(CMD_VIEW_DEMOPLAY);
    return sTRUE;
  case sKEY_F6:
    OnCommand(CMD_VIEW_DEMOSTOP);
    return sTRUE;
  case sKEY_F7:
    OnCommand(CMD_VIEW_DEMOLOOP);
    return sTRUE;
  case sKEY_F8:
    OnCommand(CMD_VIEW_DEMOERROR);
    return sTRUE;
  default:
    return sFALSE;
  }
}

sBool sToolApp::OnCommand(sU32 cmd)
{
  sMenuFrame *mf;
  sSizeBorder *sb;
  sInt i,j;
  ToolObject *to;
  ToolWindow *tw;
  ToolDoc *td;
  sChar buffer[sDI_PATHSIZE];

  switch(cmd)
  {
  case CMD_FILEMENU:
    mf = new sMenuFrame;
    mf->SendTo = this;
    mf->AddBorder(new sNiceBorder);
    mf->AddMenu("New",sCMDLS_NEW,0);
    mf->AddMenu("Open",sCMDLS_OPEN,'o');
    mf->AddMenu("Save as..",sCMDLS_SAVEAS,'a');
    mf->AddMenu("Save",sCMDLS_SAVE,sKEY_F12);
    mf->AddSpacer();
    mf->AddMenu("Export Ascii",CMD_FILE_EXPORTASC,0);
    mf->AddMenu("Export Binary",CMD_FILE_EXPORTBIN,0);
    mf->AddMenu("Export Packed",CMD_FILE_EXPORTPAC,0);
    mf->AddSpacer();
    mf->AddMenu("Browser",sCMDLS_BROWSER,'b');
    mf->AddMenu("Exit",sCMDLS_EXIT,sKEYQ_SHIFT|sKEY_ESCAPE);
    sGui->AddPulldown(mf);
    return sTRUE;
  case CMD_VIEWMENU:
    mf = new sMenuFrame;
    mf->SendTo = this;
    mf->AddBorder(new sNiceBorder);
    mf->AddCheck("Texture Size 64",CMD_VIEW_TEX6,0,TextureSize==6);
    mf->AddCheck("Texture Size 128",CMD_VIEW_TEX7,0,TextureSize==7);
    mf->AddCheck("Texture Size 256",CMD_VIEW_TEX8,0,TextureSize==8);
    mf->AddCheck("Texture Size 512",CMD_VIEW_TEX9,0,TextureSize==9);
    mf->AddCheck("Texture Size 1024",CMD_VIEW_TEX10,0,TextureSize==10);
    mf->AddSpacer();
    mf->AddCheck("Window Set 1",CMD_VIEW_SET0,sKEY_F1,WindowSet==0);
    mf->AddCheck("Window Set 2",CMD_VIEW_SET1,sKEY_F2,WindowSet==1);
    mf->AddCheck("Window Set 3",CMD_VIEW_SET2,sKEY_F3,WindowSet==2);
    mf->AddCheck("Window Set 4",CMD_VIEW_SET3,sKEY_F4,WindowSet==3);
    if(sSystem->GetScreenCount()==1)
    {
      mf->AddSpacer();
      mf->AddCheck("Windowed"           ,CMD_VIEW_RES0,0,ScreenRes==0);
      mf->AddCheck("1400x1050"          ,CMD_VIEW_RES1,0,ScreenRes==1);
      mf->AddCheck("1600x1200"          ,CMD_VIEW_RES2,0,ScreenRes==2);
      mf->AddCheck("800x600"            ,CMD_VIEW_RES3,0,ScreenRes==3);
      mf->AddCheck("1280x1024"          ,CMD_VIEW_RES4,0,ScreenRes==4);
    }
    mf->AddCheck("Turbo",CMD_VIEW_TURBO,0,TurboMode);
    mf->AddSpacer();
    mf->AddCheck("Scrollbars",CMD_VIEW_SCROLLBARS,0,sGui->GetStyle()&sGS_SCROLLBARS);
    mf->AddCheck("Title Bar when Maximised",CMD_VIEW_MAXTITLE,0,sGui->GetStyle()&sGS_MAXTITLE);
    mf->AddMenu("Minimize/Maximize Window",CMD_VIEW_MINIMIZE,0);
    mf->AddMenu("Change Window Set",CMD_VIEW_DOCKER,sKEY_F9);
    if(sSystem->GetScreenCount()==2)
      mf->AddMenu("Change Window Set (2)",CMD_VIEW_DOCKER,sKEY_F10);
    mf->AddMenu("Reset Windows",CMD_VIEW_RESETWINDOWS,0);
    mf->AddMenu("Play Demo",CMD_VIEW_DEMOPLAY,sKEY_F5);
    mf->AddMenu("Stop Demo",CMD_VIEW_DEMOSTOP,sKEY_F6);
    mf->AddMenu("Loop Demo",CMD_VIEW_DEMOLOOP,sKEY_F7);
    mf->AddMenu("Jump to Error",CMD_VIEW_DEMOLOOP,sKEY_F8);
    mf->AddSpacer();
    mf->AddMenu("Flush",CMD_VIEW_FLUSH,sKEY_F11);
    sGui->AddPulldown(mf);
    return sTRUE;
    
  case sCMDLS_CLEAR:
    Docs->Clear();
    Objects->Clear();
    sGui->GarbageCollection();
    UpdateAllWindows();
    DocChangeCount = CheckChange();
    return sTRUE;

  case sCMDLS_READ:
    if(sImplementFileMenu(cmd,this,this,FileWindow,DocChangeCount!=CheckChange()))
    {
      if(Docs->GetCount()>0 && Docs->Get(0)->GetClass()==sCID_TOOL_PAGEDOC)
        if(FindActiveWindow(sCID_TOOL_PAGEWIN))
          ((PageWin *)FindActiveWindow(sCID_TOOL_PAGEWIN))->Doc = (PageDoc *) Docs->Get(0);
    }
    return sTRUE;

  case CMD_VIEW_TEX6:
  case CMD_VIEW_TEX7:
  case CMD_VIEW_TEX8:
  case CMD_VIEW_TEX9:
  case CMD_VIEW_TEX10:
    TextureSize = cmd-CMD_VIEW_TEX6+6;
    CheckChange();
    for(i=0;i<Objects->GetCount();i++)
    {
      to = Objects->Get(i);
      if(to->CID==sCID_GENBITMAP)
      {
        to->Cache = 0;
        to->ChangeCount = GetChange();
      }
    }
    for(i=0;i<Windows->GetCount();i++)
    {
      tw = Windows->Get(i);
      if(tw->GetClass()==sCID_TOOL_VIEWWIN)
      {
        to = ((ViewWin *)tw)->GetObject();
        if(to)
        {
          to->Cache = 0;
          to->ChangeCount = GetChange();
        }
      }
    }
    sGui->GarbageCollection();
    return sTRUE;

  case CMD_VIEW_SCROLLBARS:
    sGui->SetStyle(sGui->GetStyle()^sGS_SCROLLBARS);
    return sTRUE;

  case CMD_VIEW_MAXTITLE:
    sGui->SetStyle(sGui->GetStyle()^sGS_MAXTITLE);
    return sTRUE;

  case CMD_VIEW_MINIMIZE:
    sb = FindTitle();
    sb->Maximise(!sb->Maximised);    
    return sTRUE;

  case CMD_VIEW_DOCKER:
    Docker->Root = this;
    Docker->Choices = (sGuiWindow **)&WinSetStorage[0];
    Docker->ChoiceCount = sDW_MAXWINDOW;
    if(Docker->Parent)
      sGui->SetFocus(Docker);
    else
      sGui->AddApp(Docker);
    return sTRUE;
  case CMD_VIEW_DOCKER2:
    if(SecondWindow)
    {
      Docker->Root = SecondWindow;
      Docker->Choices = (sGuiWindow **)&WinSetStorage[0];
      Docker->ChoiceCount = sDW_MAXWINDOW;
      if(Docker->Parent)
        sGui->SetFocus(Docker);
      else
        sGui->AddApp(Docker);
    }
    return sTRUE;

  case CMD_VIEW_RES0:
    ScreenRes = 0;
    sSystem->Reset(sSF_DIRECT3D,1400,1050,0,0);
    return sTRUE;
  case CMD_VIEW_RES1:
    ScreenRes = 1;
    sGui->SetStyle(sGui->GetStyle()|sGS_MAXTITLE);
    sSystem->Reset(sSF_DIRECT3D|sSF_FULLSCREEN,1400,1050,0,0);
    return sTRUE;
  case CMD_VIEW_RES2:
    ScreenRes = 2;
    sGui->SetStyle(sGui->GetStyle()|sGS_MAXTITLE);
    sSystem->Reset(sSF_DIRECT3D|sSF_FULLSCREEN,1600,1200,0,0);
    return sTRUE;
  case CMD_VIEW_RES3:
    ScreenRes = 3;
    sGui->SetStyle(sGui->GetStyle()|sGS_MAXTITLE);
    sSystem->Reset(sSF_DIRECT3D|sSF_FULLSCREEN,800,600,0,0);
    return sTRUE;
  case CMD_VIEW_RES4:
    ScreenRes = 4;
    sGui->SetStyle(sGui->GetStyle()|sGS_MAXTITLE);
    sSystem->Reset(sSF_DIRECT3D|sSF_FULLSCREEN,1280,1024,0,0);
//    sSystem->Reset(sSF_DIRECT3D|sSF_MULTISCREEN,800,600,800,600);
    return sTRUE;
  case CMD_VIEW_TURBO:
    TurboMode = !TurboMode;
    sSystem->SetResponse(!TurboMode);
    return sTRUE;

  case CMD_VIEW_SET0:
    SetWindowSet(0);
    return sTRUE;
  case CMD_VIEW_SET1:
    SetWindowSet(1);
    return sTRUE;
  case CMD_VIEW_SET2:
    SetWindowSet(2);
    return sTRUE;
  case CMD_VIEW_SET3:
    SetWindowSet(3);
    return sTRUE;

  case CMD_VIEW_RESETWINDOWS:
    if(WindowSet!=-1)
      SetWindowSet(-1);
    sSetMem(WinSetData,0,sizeof(WinSetData));

    i = 0; j=0;
    WinSetData[j][0][i].Init(-1,0x0000,2); i++;
    WinSetData[j][0][i].Init(-2,0x0000,2); i++;
    WinSetData[j][0][i].Init( 0,0x0000,0); i++;
    WinSetData[j][0][i].Init(-1,0x0000,2); i++;
    WinSetData[j][0][i].Init(-2,0x0000,2); i++;
    WinSetData[j][0][i].Init( 1,0x8000,0); i++;
    WinSetData[j][0][i].Init( 8,0x8000,0); i++;
    WinSetData[j][0][i].Init(14,0xc000,0); i++;
    WinSetData[j][0][i].Init(-2,0x8000,2); i++;
    WinSetData[j][0][i].Init(-1,0x0000,2); i++;
    WinSetData[j][0][i].Init(10,0x0000,0); i++;
    WinSetData[j][0][i].Init(12,0x8000,0); i++;
    WinSetData[j][0][i].Init( 4,0x2000,0); i++;
    WinSetData[j][1][i].Init( 3,0x0000,0); i++;

    i = 0; j=1;
    WinSetData[j][0][i].Init(-2,0x0000,2); i++;
    WinSetData[j][0][i].Init(-1,0x0000,2); i++;
    WinSetData[j][0][i].Init( 0,0x0000,0); i++; // view
    WinSetData[j][0][i].Init( 4,0x0000,0); i++; // page
    WinSetData[j][0][i].Init(-1,0x0000,2); i++;
    WinSetData[j][0][i].Init(16,0x0000,0); i++; // edit
    WinSetData[j][0][i].Init(-2,0xc000,2); i++;
    WinSetData[j][0][i].Init( 8,0x0000,0); i++; // para
    WinSetData[j][0][i].Init(10,0x0000,0); i++; // pagelist
    WinSetData[j][1][i].Init( 3,0x0000,0); i++;
   
    i = 0; j=2;
    WinSetData[j][0][i].Init(-1,0x0000,2); i++;
    WinSetData[j][0][i].Init(-2,0x0000,2); i++;
    WinSetData[j][0][i].Init( 0,0x0000,0); i++; // view
    WinSetData[j][0][i].Init(-1,0x0000,2); i++;
    WinSetData[j][0][i].Init( 8,0x0000,0); i++; // para
    WinSetData[j][0][i].Init(24,0x8000,0); i++; // anim
    WinSetData[j][0][i].Init(-2,0x8000,2); i++;
    WinSetData[j][0][i].Init(-1,0x0000,2); i++;
    WinSetData[j][0][i].Init(10,0x0000,0); i++; // pagelist
    WinSetData[j][0][i].Init(12,0x8000,0); i++; // objectlist
    WinSetData[j][0][i].Init(20,0x2000,0); i++; // time
    WinSetData[j][1][i].Init( 3,0x0000,0); i++; // view

    i = 0; j=3;
    WinSetData[j][0][i].Init( 0,0x0000,0); i++;
    WinSetData[j][1][i].Init( 3,0x0000,0); i++;

    WindowSet = -1;
    SetWindowSet(0);
    return sTRUE;

  case CMD_VIEW_DEMOPLAY:
    j = 1;
    DemoPrev();
    for(i=0;i<Windows->GetCount();i++)
    {
      if(Windows->Get(i)->GetClass()==sCID_TOOL_VIEWWIN)
      {
        ((ViewWin *)Windows->Get(i))->SetPlayerMode(j);
        j = 0;
      }
    }
//    Player->TimeMode=sGPT_STOP;
    return sTRUE;

  case CMD_VIEW_DEMOSTOP:
    if(!Player->TimePlay && Player->Status!=0)
      Player->TimePlay=1;
    else if(Player->TimePlay)
      Player->TimePlay=0;
    return sTRUE;
  case CMD_VIEW_DEMOLOOP:
    Player->TimeLoop = !Player->TimeLoop;
    return sTRUE;
  case CMD_VIEW_DEMOERROR:
    if(Player->SC->ErrorLine!=0)
    {
      tw = FindActiveWindow(sCID_TOOL_EDITWIN);
      if(tw)
      {
        td = FindDoc(Player->SC->ErrorFile);
        if(td && td->GetClass()==sCID_TOOL_EDITDOC)
        {
          SetActiveWindow(tw);
          ((EditWin*)tw)->SetDoc(td);
          ((EditWin*)tw)->SetLine(Player->SC->ErrorLine-1);
          ((EditWin*)tw)->SetSelection(Player->SC->ErrorSymbolPos,Player->SC->ErrorSymbolPos+Player->SC->ErrorSymbolWidth);
        }
      }
    }
    return sTRUE;

  case CMD_FILE_EXPORTASC:
    if(FileWindow->ChangeExtension(buffer,".txt"))
      Export(buffer,0);
    return sTRUE;

  case CMD_FILE_EXPORTBIN:
    if(FileWindow->ChangeExtension(buffer,".raw"))
      Export(buffer,1);
    return sTRUE;
  
  case CMD_FILE_EXPORTPAC:
    if(FileWindow->ChangeExtension(buffer,".pac"))
      Export(buffer,2);
    return sTRUE;
  
  case CMD_PANIC:
    for(i=0;i<Windows->GetCount();i++)
      if(Windows->Get(i)->GetClass()==sCID_TOOL_VIEWWIN)
        ((ViewWin *)Windows->Get(i))->SetObject(0);
    return sTRUE;

  case CMD_VIEW_FLUSH:
    for(i=0;i<Objects->GetCount();i++)
      Objects->Get(i)->ChangeCount = GetChange();
    return sTRUE;

  default:
    return sImplementFileMenu(cmd,this,this,FileWindow,DocChangeCount!=CheckChange());
  }
}

void sToolApp::OnDrag(sDragData &)
{
}

void sToolApp::OnFrame()
{
  sInt time,mem,sec;
  sInt oc,rc;
  sChar buffer[256];

  sec = sSystem->GetTimeOfDay();
  time = sec/60;
  mem = sSystem->MemoryUsed()/1024;
  sBroker->Stats(oc,rc);

  sSPrintF(buffer,sizeof(buffer),"%d Obj   %d.%03d MByte",oc,mem/1024,(mem&0x3ff)*1000/1024);
  SetStat(3,buffer);

  sSPrintF(buffer,sizeof(buffer),"%02d:%02d",time/60,time%60);
#if !sRELEASE
  if(time<7*60 && time>1*60 && (sec&1))
    sSetMem(buffer,' ',5);
#endif
  SetStat(4,buffer);
}

/****************************************************************************/
/****************************************************************************/

ToolWindow *sToolApp::FindActiveWindow(sU32 cid)
{
  sInt i,max;
  ToolWindow *win;
  max = Windows->GetCount();
  for(i=0;i<max;i++)
  {
    win = Windows->Get(i);
    if(win->GetClass()==cid)
      return win;
  }
  return 0;
}

void sToolApp::SetActiveWindow(ToolWindow *win)
{
  Windows->RemOrder(win);
  Windows->AddHead(win);
}

void sToolApp::SetWindowSet(ToolWindow *win)
{
  sInt i,j,id;

  for(i=0;i<sDW_MAXSET;i++)
  {
    for(j=0;j<sDW_MAXPERSET*2;j++)
    {
      id = WinSetData[i][0][j].Id;
      if(id>=0 && WinSetStorage[id]==win)
      {
        SetWindowSet(i);
        return;
      }
    }
  }
}

void sToolApp::SetWindowSet(sInt nr)
{
  WindowSetEntry *e;
  sInt i;

  sVERIFY(nr>=-1 && nr<sDW_MAXSET);

// delete old window set

  if(WindowSet>=0 && WindowSet<sDW_MAXSET)
  {
    e = &WinSetData[WindowSet][0][0];
    RemWindowSetR(e,this,this->GetChild(0));
    this->RemChilds();
    if(SecondWindow)
    {
      e = &WinSetData[WindowSet][1][0];
      RemWindowSetR(e,SecondWindow,SecondWindow->GetChild(0));
      SecondWindow->RemChilds();
    }
    for(i=0;i<sDW_MAXPERSET;i++)
    {
      WinSetData[WindowSet][0][i].Window = 0;
      WinSetData[WindowSet][1][i].Window = 0;
    }
  }

// set new window set

  WindowSet = nr;
  if(WindowSet!=-1)
  {
    e = &WinSetData[nr][0][0];
    SetWindowSetR(e,this);
    if(SecondWindow)
    {
      e = &WinSetData[nr][1][0];
      SetWindowSetR(e,SecondWindow);
    }
    sGui->SetFocus(this);
  }
}

void sToolApp::SetWindowSetR(WindowSetEntry *&e,sGuiWindow *p)
{
  sInt i,max;
  sVSplitFrame *vs;
  sHSplitFrame *hs;

  p->Flags |= sGWF_LAYOUT;

  switch(e->Id)
  {
  case -1:
    hs = new sHSplitFrame;
    e->Window = hs;
    p->AddChild(hs);
    max = e->Count;
    e++;
    for(i=0;i<max;i++)
    {
      hs->Pos[i] = -e->Pos;
      SetWindowSetR(e,hs);
    }
    break;
  case -2:
    vs = new sVSplitFrame;
    e->Window = vs;
    p->AddChild(vs);
    max = e->Count;
    e++;
    for(i=0;i<max;i++)
    {
      vs->Pos[i] = -e->Pos;
      SetWindowSetR(e,vs);
    }
    break;
  default:
    sVERIFY(e->Id>=0 && e->Id<sDW_MAXWINDOW);
    sVERIFY(WinSetStorage[e->Id]);

    if(WinSetStorage[e->Id]->Parent==0)
    {
      e->Window = WinSetStorage[e->Id];
      p->AddChild(WinSetStorage[e->Id]);
    }
    else
    {
      for(i=0;i<sDW_MAXWINDOW;i++)
      {
        if(WinSetStorage[i] && WinSetStorage[i]->Parent==0)
        {
          e->Window = WinSetStorage[i];
          p->AddChild(WinSetStorage[i]);
          break;
        }
      }
      sVERIFY(i!=sDW_MAXWINDOW);
    }
    e++;
    break;
  }
}

void sToolApp::RemWindowSetR(WindowSetEntry *&e,sGuiWindow *p,sGuiWindow *win)
{
  sInt i,j,max;

  switch(win->GetClass())
  {
  case sCID_HSPLITFRAME:
    max = win->GetChildCount();
    e->Count = max;
    e->Id = -1;
    e++;
    for(i=0;i<max;i++)
    {
      e->Pos = (((sHSplitFrame *)win)->Pos[i] * 0x10000 + win->Position.YSize()/2) / win->Position.YSize();
      RemWindowSetR(e,win,win->GetChild(i));
    }
    win->RemChilds();
    break;

  case sCID_VSPLITFRAME:
    max = win->GetChildCount();
    e->Count = max;
    e->Id = -2;
    e++;
    for(i=0;i<max;i++)
    {
      e->Pos = (((sVSplitFrame *)win)->Pos[i] * 0x10000 + win->Position.XSize()/2) / win->Position.XSize();
      RemWindowSetR(e,win,win->GetChild(i));
    }
    win->RemChilds();
    break;

  default:
    j = -1;
    for(i=0;i<sDW_MAXWINDOW && j==-1;i++)
    {
      if(WinSetStorage[i]==win)
        j = i;
    }
    sVERIFY(j!=-1);
    e->Id = j;
    e->Count = 0;
    e++;
    break;
  }
}

sU32 sToolApp::GetChange()
{
  if(ChangeMode==0)
  {
    ChangeMode = 1;
    ChangeCount++;
  }
  return ChangeCount;
}

sU32 sToolApp::CheckChange()
{
  if(ChangeMode==1)
  {
    ChangeMode = 0;
    ChangeCount++;
  }
  return ChangeCount;
}

void sToolApp::SetStat(sInt num,sChar *text)
{
  sVERIFY(num>=0 && num<5);
  sCopyString(Stat[num],text,sizeof(Stat[num]));
}

/****************************************************************************/

sBool sToolApp::DemoPrev(sBool doexport)
{
  sInt i,j;
  ToolObject *to;
  ToolDoc *td;
  TimeDoc *time;
  AnimDoc *anim;

  Player->SR->Load(0);

// generate all objects

  for(i=0;i<Objects->GetCount();i++)
    Objects->Get(i)->Temp = 0;
  for(i=0;i<Docs->GetCount();i++)
  {
    td = Docs->Get(i);
    if(td->GetClass()==sCID_TOOL_TIMEDOC)
    {
      time = (TimeDoc *)td;
      for(j=0;j<time->Ops->GetCount();j++)
      {
        anim = time->Ops->Get(j)->Anim;
        if(anim && anim->RootName[0])
        {
          to = FindObject(anim->RootName);
          if(to)
            to->Temp = 1;
        }
      }
    }
  }

  for(i=0;i<Objects->GetCount();i++)
  {
    to = Objects->Get(i);
    if(to->Name[0] && to->Temp)
    {
      to->Calc();
      if(!to->Cache)
        return 0;
    }
  }

// begin code 

  CodeGen.Begin();

// add cached variables

  for(i=0;i<Objects->GetCount();i++)
  {
    to = Objects->Get(i);
    if(to->Name[0] && to->Temp)
      CodeGen.AddResult(to);
  }

// add timelines

  for(i=0;i<Docs->GetCount();i++)
  {
    td = Docs->Get(i);
    if(td->GetClass()==sCID_TOOL_TIMEDOC)
      CodeGen.AddTimeline((TimeDoc *)td);
  }

// generate code

  if(!CodeGen.End(Player->SC,1,doexport))
  {
    OnCommand(CMD_VIEW_DEMOERROR);
    CodeGen.Cleanup();
    return 0;
  }

// execute code

  if(!doexport)
  {
    if(CodeGen.Execute(Player->SR))
    {
      Player->Status = 1;
      Player->Run();
    }
    else
    {
      OnCommand(CMD_VIEW_DEMOERROR);
      CodeGen.Cleanup();
      return 0;
    }
  }

// done

  CodeGen.Cleanup();
  return sTRUE;
}

sBool sToolApp::Export(sChar *name,sInt binary)
{
  sDiskItem *di;
  sU8 *data;
  sInt size;

  if(DemoPrev(sTRUE))
  {
    switch(binary)
    {
    case 0:
      di = sDiskRoot->Find(name,sDIF_CREATE);
      if(di)
        if(di->Save((sU8 *)CodeGen.Source,CodeGen.SourceUsed))
          return sTRUE;
      break;

    case 1:
      di = sDiskRoot->Find(name,sDIF_CREATE);
      if(di) 
        if(di->Save((sU8 *)CodeGen.Bytecode,Player->SC->GetCodeSize()))
          return sTRUE;
      break;

    case 2:
      di = sDiskRoot->Find(name,sDIF_CREATE);
      if(di)
      {
        Player->SR->Load((sU32 *)CodeGen.Bytecode);
        data = Player->SR->PackedExport(size);
        if(di->Save(data,size))
          return sTRUE;
      }
      break;

    }
  } 
  return sFALSE;
}


/*
void sToolApp::DemoPrev()
{
  sChar *text;
  sU32 *code;
  sInt i,j,max;
  ToolDoc *doc;
  EditDoc *ed;
  TimeDoc *timedoc;
  ToolObject *to;

  sVERIFY(CodeNest==0);

// calculate all

  max = Objects->GetCount();
  for(i=0;i<max;i++)
  {
    to = Objects->Get(i);
    to->Calc();
  }


// create code for global variables

  CodeG0 = 0;
  CodeG1 = CodeAlloc/4;
  Code0 = CodeAlloc/4;
  Code1 = CodeAlloc-1;
  CodeGlobal = FIRSTGLOBAL;                       // #5 = magic number from system.txt (i am so evil)
  sSetMem(Code,' ',CodeAlloc);
  Code[CodeAlloc-1] = 0;

  max = Objects->GetCount();
  for(i=0;i<max;i++)
  {
    to = Objects->Get(i);
    if(to->Name)
    {
      j = CodeVar(to,to->Name);
      Player->SR->SetGlobalObject(j++,to->Cache);
    }
  }
  CodeAddGlobal("  void Pages() { }\n");
  Code[CodeG0] = 0;

// compile and start

  Player->SC->Reset();
  
  text = sSystem->LoadText("system.txt");
  code = 0;

  Player->Status=0;
  if(text && Player->SC->Parse(text,"system.txt"))
  {
    delete[] text;

//    sDPrintF("<<<%s>>>\n",Code);
    if(Player->SC->Parse(Code,"__globals"))
      Player->Status = 1;

    for(i=0;i<Docs->GetCount() && Player->Status;i++)
    {
      doc = Docs->Get(i);
      if(doc->GetClass()==sCID_TOOL_TIMEDOC)
      {
        timedoc = (TimeDoc *)doc;
        if(!Player->SC->Parse(timedoc->MakeSource(),timedoc->Name))
          Player->Status=0;
      }
    }
    for(i=0;i<Docs->GetCount() && Player->Status;i++)
    {
      doc = Docs->Get(i);
      if(doc->GetClass()==sCID_TOOL_EDITDOC)
      {
        ed = (EditDoc *) doc;
        if(!Player->SC->Parse(ed->Text,ed->Name))
          Player->Status=0;
      }
    }
    if(Player->Status)
    {
      Player->SC->Optimise();

      code = Player->SC->Generate();
      if(code)
      {
        Player->SR->Load(code);
        Player->Run();
      }
      else
        Player->Status = 1;
    }
  }

// flush load/store

  max = CurrentLoadStore->GetCount();
  for(i=0;i<max;i++)
  {
    to = CurrentLoadStore->Get(i);
    to->GlobalStore = -1;
    to->GlobalLoad = -1;
  }
  CurrentLoadStore->Clear();

// set text editor

  if(Player->SC->GetErrorPos()[0])
    SetStat(2,Player->SC->GetErrorPos());
  else
    SetStat(2,"compilation ok.");

  if(Player->SC->ErrorLine!=0)
  {
    OnCommand(CMD_VIEW_DEMOERROR);
  }
}
*/
/****************************************************************************/
/*
void sToolApp::Export(sChar *name,sInt binary)
{
  sInt i,max;
  ToolObject *to;
  EditDoc *ed;
  TimeDoc *timedoc;
  sChar *d,*p,*text;
  sInt len;
  sDiskItem *di;

// create code for global variables

  sVERIFY(CodeNest==0);
  CodeG0 = 0;
  CodeG1 = CodeAlloc/4;
  Code0 = CodeAlloc/4;
  Code1 = CodeAlloc-1;
  CodeGlobal = FIRSTGLOBAL;
  sSetMem(Code,' ',CodeAlloc);
  Code[CodeAlloc-1] = 0;
  CodeNest++;

  max = Objects->GetCount();
  for(i=0;i<max;i++)
  {
    to = Objects->Get(i);
    to->ChangeCount = GetChange();
    sVERIFY(to->GlobalLoad == -1);
    sVERIFY(to->GlobalStore == -1);
  }
  CheckChange();

// create code

  for(i=0;i<max;i++)
  {
    to = Objects->Get(i);
    to->Calc();
  }

// concat code

  text = sSystem->LoadText("system.txt");
  sVERIFY(text);

  p = d = new sChar[1024*1024];
  len = sGetStringLen(text);
  sCopyMem(p,text,len); p+=len; 
  delete[] text;
  sCopyString(p,"\n/ **************************** /\n\n",1000); while(*p) p++;
  sCopyMem(p,Code,CodeG0); p+=CodeG0;
  sCopyString(p,"\n/ **************************** /\n\n",1000); while(*p) p++;
  sSPrintF(p,1000,"void Pages()\n{\n"); while(*p) p++;
  sCopyMem(p,Code+CodeG1,Code0-CodeG1); p+=Code0-CodeG1;
  sSPrintF(p,1000,"}\n"); while(*p) p++;
  sCopyString(p,"\n/ **************************** /\n\n",1000); while(*p) p++;
  for(i=0;i<Docs->GetCount() && Player->Status;i++)
  {
    if(Docs->Get(i)->GetClass()==sCID_TOOL_TIMEDOC)
    {
      timedoc = (TimeDoc *) Docs->Get(i);
      text = timedoc->MakeSource();
      len = sGetStringLen(text);
      sCopyMem(p,text,len); p+=len; 
      sCopyString(p,"\n/ **************************** /\n\n",1000); while(*p) p++;
    }
  }
  sCopyString(p,"\n/ **************************** /\n\n",1000); while(*p) p++;
  for(i=0;i<Docs->GetCount() && Player->Status;i++)
  {
    if(Docs->Get(i)->GetClass()==sCID_TOOL_EDITDOC)
    {
      ed = (EditDoc *) Docs->Get(i);
      len = sGetStringLen(ed->Text);
      sCopyMem(p,ed->Text,len); p+=len; 
      sCopyString(p,"\n/ **************************** /\n\n",1000); while(*p) p++;
    }
  }
  sCopyString(p,"\n/ **************************** /\n\n",1000); while(*p) p++;
  sCopyString(p,"\n/ **************************** /\n\n",1000); while(*p) p++;

  p[0] = 0;
  sDPrint(d);

  if(binary)
  {
    sU32 *code;
    sInt size;
    ScriptCompiler *sc;

    sc = new ScriptCompiler;

    sc->Reset();
    sc->Parse(d,"export");
    sc->Optimise();
    code = sc->Generate();
    size = sc->GetCodeSize();

    di = sDiskRoot->Find(name,sDIF_CREATE);
    if(di)
      di->Save((sU8 *)code,size);
    delete[] d;
  }
  else
  {
    di = sDiskRoot->Find(name,sDIF_CREATE);
    if(di)
      di->Save((sU8 *)d,p-d);
    delete[] d;
  }

// cleanup

  max = CurrentLoadStore->GetCount();
  for(i=0;i<max;i++)
  {
    to = CurrentLoadStore->Get(i);
    to->GlobalStore = -1;
    to->GlobalLoad = -1;
  }
  CurrentLoadStore->Clear();

  CodeNest--;
}
*/
/****************************************************************************/

ToolObject *sToolApp::FindObject(sChar *name)
{
  sInt i,max;
  ToolObject *to;

  max = Objects->GetCount();
  for(i=0;i<max;i++)
  {
    to = Objects->Get(i);
    if(sCmpString(to->Name,name)==0)
      return to;
  }
  return 0;
}
/*
ToolObject *sToolApp::FindObjectRef(sObject *ref)
{
  sInt i,max;
  ToolObject *to;

  max = Objects->GetCount();
  for(i=0;i<max;i++)
  {
    to = Objects->Get(i);
    if(to->Reference == ref)
      return to;
  }
  return 0;
}
*/
void sToolApp::UpdateObjectList()
{
  sInt i,max;
  ToolWindow *tw;

  max = Windows->GetCount();
  for(i=0;i<max;i++)
  {
    tw = Windows->Get(i);
    if(tw->GetClass()==sCID_TOOL_OBJECTWIN)
      ((ObjectWin *)tw)->UpdateList();
  }
}

ToolDoc *sToolApp::FindDoc(sChar *name)
{
  sInt i,max;
  ToolDoc *td;

  max = Docs->GetCount();
  for(i=0;i<max;i++)
  {
    td = Docs->Get(i);
    if(sCmpString(td->Name,name)==0)
      return td;
  }
  return 0;
}


void sToolApp::UpdateDocList()
{
  sInt i,max;
  ToolWindow *tw;

  max = Windows->GetCount();
  for(i=0;i<max;i++)
  {
    tw = Windows->Get(i);
    if(tw->GetClass()==sCID_TOOL_PROJECTWIN)
      ((ProjectWin *)tw)->UpdateList();
  }
}

void sToolApp::UpdateAllWindows()
{
  sInt i,max;
  ToolWindow *tw;

  UpdateDocList();
  max = Windows->GetCount();
  for(i=0;i<max;i++)
  {
    tw = Windows->Get(i);
    tw->SetDoc(0);
  }
  sGui->GarbageCollection();
}

/****************************************************************************/
/****************************************************************************/

sBool sToolApp::Write(sU32 *&data)
{
  sU32 *hdr;
  sInt i;
  ToolDoc *td;

  hdr = sWriteBegin(data,sCID_TOOL_DOC,1);

  *data++ = Docs->GetCount();
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;
  for(i=0;i<Docs->GetCount();i++)
  {
    td = Docs->Get(i);
    sWriteString(data,td->Name);
    
    if(!td->Write(data))
      return sFALSE;
  }
  sWriteEnd(data,hdr);
  DocChangeCount = CheckChange();
  return sTRUE;
}

sBool sToolApp::Read(sU32 *&data)
{
  PageDoc *pagedoc;
  EditDoc *editdoc;
  TimeDoc *timedoc;
  ToolDoc *td;
  sInt version;
  sInt count;
  sInt i;
  sChar buffer[64];

  version = sReadBegin(data,sCID_TOOL_DOC);
  if(version<1) return sFALSE;

  Docs->Clear();
  Objects->Clear();

  count = *data++;
  data+=3;

  for(i=0;i<count;i++)
  {
    sReadString(data,buffer,sizeof(buffer));
    td = 0;
    switch(data[1])
    {
    case sCID_TOOL_PAGEDOC:
      pagedoc = new PageDoc;
      td = pagedoc;
      break;
    case sCID_TOOL_EDITDOC:
      editdoc = new EditDoc;
      td = editdoc;
      break;
    case sCID_TOOL_TIMEDOC:
      timedoc = new TimeDoc;
      td = timedoc;
      break;
    case sCID_TOOL_ANIMDOC:
      td = new AnimDoc;
      break;
    default:
      return sFALSE;
    }
    if(td)
    {
      td->App = this;
      if(!td->Read(data))
        return sFALSE;
      sCopyString(td->Name,buffer,sizeof(td->Name));
      Docs->Add(td);
    }
  }

  for(i=0;i<Docs->GetCount();i++)
  {
    td = Docs->Get(i);
    if(td->GetClass()==sCID_TOOL_PAGEDOC)
    {
      ((PageDoc *)td)->UpdatePage();
    }
  }

  UpdateAllWindows();
  DocChangeCount = CheckChange();
  return sReadEnd(data);
}



/****************************************************************************/

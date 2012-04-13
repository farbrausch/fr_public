// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "werkkzeug.hpp"
#include "kdoc.hpp"
#include "appbrowser.hpp"
#include "_viruz2.hpp"
#include "_ogg.hpp"
#include "genoverlay.hpp"
#include "genbitmap.hpp"
#include "engine.hpp"
#include "novaplayer.hpp"

#include "winpage.hpp"
#include "winview.hpp"
#include "wintimeline.hpp"
#include "winspline.hpp"
#include "winpara.hpp"
#include "wintool.hpp"
#include "wincalc.hpp"
#include "winnova.hpp"
#include "winnovaview.hpp"

#include "exepacker.hpp"
#include "mapfile.hpp"


/****************************************************************************/

void WerkkzeugSoundHandler(sS16 *steriobuffer,sInt samples,void *user); 

/****************************************************************************/
/***                                                                      ***/
/***   application                                                        ***/
/***                                                                      ***/
/****************************************************************************/

#define CMD_MENU_FILE         0x1001
#define CMD_MENU_EDIT         0x1002
#define CMD_MENU_PAGE         0x1010
#define CMD_MENU_SPLINE       0x1011
#define CMD_MENU_TIMELINE     0x1012
#define CMD_MENU_SCENE        0x1013
#define CMD_MENU_SHADER       0x1014
#define CMD_MENU_SHADOW       0x1015
#define CMD_MENU_SOUND        0x1016
#define CMD_MENU_NOVA         0x1017

#define CMD_BROWSERPREVIEW    0x1060

#define CMD_FILE_SAVECONFIG   0x1080
#define CMD_FILE_EXPORT       0x1081
#define CMD_FILE_OWNALL       0x1082
#define CMD_FILE_OWNALL2      0x1083
#define CMD_FILE_MAKEDEMO     0x1084
#define CMD_FILE_RENDERVIDEO  0x1085

#define CMD_EDIT_EDITOR       0x1101
#define CMD_EDIT_DEMO         0x1102
#define CMD_EDIT_TEXL         0x1103
#define CMD_EDIT_TEXM         0x1104
#define CMD_EDIT_TEXH         0x1105
#define CMD_EDIT_STATS        0x1106
#define CMD_EDIT_GAMEINFO     0x1107
#define CMD_EDIT_PERFMON      0x1108
#define CMD_EDIT_LOG          0x1109
#define CMD_EDIT_FINDCLASS    0x110b
#define CMD_EDIT_FINDCLASS2   0x110c
#define CMD_EDIT_CLEARRESPAWN 0x110d
#define CMD_EDIT_FINDSWITCH   0x110e
#define CMD_EDIT_FINDBUGS     0x110f

#define CMD_MUSIC_START       0x1201
#define CMD_MUSIC_TOGGLE      0x1202
#define CMD_MUSIC_LOOP        0x1203

/****************************************************************************/

WerkkzeugApp::WerkkzeugApp()
{
  sHSplitFrame *h0,*h1;
  sVSplitFrame *v0,*v1,*v2,*v3;
  sToolBorder *tb;
  sDiskItem *di;
  sChar buffer[4096];
  WerkPage *page;
  sControl *con;
  sInt i;
  KEffect2 *e2;
//  WerkOp *op;

// zero it out

  AutoSaveMax = 1;
  MusicVolume = 256;
  OpBrowserWin = 0;
  Doc = 0;
  sCopyString(OpBrowserPath,"By Dot",sDI_PATHSIZE);
  sCopyString(UserName,"default-user",KK_NAME);
  UserCount = 0;

// create effect info

  for(i=0;EffectList[i].Name;i++)
  {
    e2 = &EffectList[i];
    e2->Info = new KEffectInfo;
    e2->Info->Init();
    e2->Info->Clear();
    sVERIFY(e2->ParaSize<=sizeof(e2->Info->DefaultPara));
    sVERIFY(e2->EditSize<=sizeof(e2->Info->DefaultEdit));
    (*e2->OnPara)(e2->Info);
  }

// window layout

  sSystem->ContinuousUpdate(0);
  sSystem->SetResponse(1);

  ViewWin = new WinView;
  ViewWin->AddBorder(new sFocusBorder);
  ViewWin->App = this;
  PageWin = new WinPage;
  PageWin->AddBorder(new sFocusBorder);
  PageWin->App = this;
  PagelistWin = new WinPagelist;
  PagelistWin->AddBorder(new sFocusBorder);
  PagelistWin->App = this;
  StatusWin = new WinStatus;
  StatusWin->AddBorder(new sFocusBorder);
  StatusWin->App = this;
  TimelineWin = new WinTimeline;
  TimelineWin->AddBorder(new sFocusBorder);
  TimelineWin->App = this;
  Timeline2Win = new WinTimeline2;
  Timeline2Win->AddBorder(new sFocusBorder);
  Timeline2Win->App = this;
  EventPara2Win = new WinEventPara2;
  EventPara2Win->AddBorder(new sFocusBorder);
  EventPara2Win->App = this;
  EventScript2Win = new WinEventScript2;
  EventScript2Win->AddBorder(new sFocusBorder);
  EventScript2Win->App = this;
  ParaWin = new WinPara;
  ParaWin->AddBorder(new sFocusBorder);
  ParaWin->App = this;
  AnimPageWin = new WinAnimPage;
  AnimPageWin->AddBorder(new sFocusBorder);
  AnimPageWin->App = this;
  EventWin = new WinEvent;
  EventWin->AddBorder(new sFocusBorder);
  EventWin->App = this;
  SplineWin = new WinSpline;
  SplineWin->AddBorder(new sFocusBorder);
  SplineWin->App = this;
  SplineListWin = new WinSplineList;
  SplineListWin->AddBorder(new sFocusBorder);
  SplineListWin->App = this;
  SplineParaWin = new WinSplinePara;
  SplineParaWin->AddBorder(new sFocusBorder);
  SplineParaWin->App = this;

  SceneListWin = new WinSceneList;
  SceneListWin->AddBorder(new sFocusBorder);
  SceneListWin->App = this;
  SceneNodeWin = new WinSceneNode;
  SceneNodeWin->AddBorder(new sFocusBorder);
  SceneNodeWin->App = this;
  SceneAnimWin = new WinSceneAnim;
  SceneAnimWin->AddBorder(new sFocusBorder);
  SceneAnimWin->App = this;
  SceneParaWin = new WinScenePara;
  SceneParaWin->AddBorder(new sFocusBorder);
  SceneParaWin->App = this;
  ViewNovaWin = new WinNovaView;
  ViewNovaWin->AddBorder(new sFocusBorder);
  ViewNovaWin->App = this;

  TimeBorderWin = new WinTimeBorder;
  TimeBorderWin->App = this;
  StatisticsWin = new WinStatistics(this);
  StatisticsWin->AddBorder(new sFocusBorder);
  GameInfoWin = new WinGameInfo(this);
  GameInfoWin->AddBorder(new sFocusBorder);
  PerfMonWin = new WinPerfMon;
  PerfMonWin->AddBorder(new sFocusBorder);
  LogWin = new sLogWindow;
  LogWin->AddBorder(new sFocusBorder);
  FindResultsWin = new WinFindResults(this);
  FindResultsWin->AddBorderHead(new sFocusBorder);
  CalcWin = new WinCalc(this);
  CalcWin->AddBorder(new sFocusBorder);
  ShowFindResults = sFALSE;

  h0 = new sHSplitFrame;
  h1 = new sHSplitFrame;
  v0 = new sVSplitFrame;
  v1 = new sVSplitFrame;
  v2 = new sVSplitFrame;
  v3 = new sVSplitFrame;
  Switch0 = new sSwitchFrame;
  Switch1 = new sSwitchFrame;
  Switch2 = new sSwitchFrame;
  SwitchView = new sSwitchFrame;
  ParaSplit = new sHSplitFrame;
  TopSplit = v0;

  AddChild(h0);
  h0->AddChild(v0);
  h0->AddChild(v1);
  v0->AddChild(SwitchView);
  v0->AddChild(Switch0);
  v0->Pos[1] = 800;
  v1->AddChild(Switch2);
  v1->AddChild(Switch1);
  v1->Pos[1] = 150;
  v2->AddChild(SplineListWin);
  v2->AddChild(SplineWin);
  v2->Pos[1] = 150;
  v3->AddChild(SceneNodeWin);
  v3->AddChild(SceneAnimWin);
  v3->Pos[1] = 250;
  ParaSplit->AddChild(ParaWin);
  ParaSplit->AddChild(AnimPageWin);
  h1->AddChild(EventPara2Win);
  h1->AddChild(EventScript2Win);

  Switch0->AddChild(ParaSplit);         // 0
  Switch0->AddChild(SplineParaWin);
  Switch0->AddChild(EventWin);
  Switch0->AddChild(StatisticsWin);
  Switch0->AddChild(GameInfoWin);       // 4
  Switch0->AddChild(PerfMonWin);
  Switch0->AddChild(LogWin);
  Switch0->AddChild(h1);                // 7
  Switch0->AddChild(SceneParaWin);      // 8

  Switch1->AddChild(PageWin);           // 0 page
  Switch1->AddChild(v2);                // 1 spline
  Switch1->AddChild(TimelineWin);       // 2 timeline
  Switch1->AddChild(v3);                // 3 scene
  Switch1->AddChild(Timeline2Win);      // 4 timeline2

  Switch2->AddChild(PagelistWin);
  Switch2->AddChild(SceneListWin);

  SwitchView->AddChild(ViewWin);
  SwitchView->AddChild(ViewNovaWin);

  tb = new sToolBorder;
  tb->AddMenu("File",CMD_MENU_FILE);
  tb->AddMenu("View",CMD_MENU_EDIT);
  tb->AddMenu("   ",0);

  con = new sControl; con->EditBool(CMD_MENU_PAGE,&RadioPage,"Page");
  con->Style |= sCS_SMALLWIDTH;
  tb->AddChild(con);
  con = new sControl; con->EditBool(CMD_MENU_TIMELINE,&RadioTime,"Timeline");
  con->Style |= sCS_SMALLWIDTH;
  tb->AddChild(con);
  con = new sControl; con->EditBool(CMD_MENU_SPLINE,&RadioSpline,"Spline");
  con->Style |= sCS_SMALLWIDTH;
  tb->AddChild(con);
  con = new sControl; con->EditBool(CMD_MENU_SCENE,&RadioScene,"Scene");
  con->Style |= sCS_SMALLWIDTH;
  tb->AddChild(con);
  
  tb->AddMenu("   ",0);
  con = new sControl; con->EditCycle(CMD_MENU_NOVA,&NovaMode,0,"Legacy|Nova");
  con->Style |= sCS_SMALLWIDTH;
  tb->AddChild(con);

  RadioPage = 1;
  RadioTime = 0;
  RadioSpline = 0;
  RadioScene = 0;
  NovaMode = 0;
/*
  tb->AddButton("Page",CMD_MENU_PAGE);
  tb->AddButton("Spline",CMD_MENU_SPLINE);
  tb->AddButton("Time",CMD_MENU_TIMELINE);
  tb->AddButton("Lightmaps",CMD_MENU_LIGHTMAPS);
*/

  tb->AddButton("   ",0)->Style = sCS_NOBORDER;

  con = new sControl;
  con->EditChoice(CMD_MENU_SHADER,&GenOverlayManager->CurrentShader,0,"PS00|PS11|PS14|PS20");
  con->Style |= sCS_SMALLWIDTH | sCS_STATIC;
  tb->AddChild(con);

  con = new sControl;
  con->EditChoice(CMD_MENU_SHADOW,&GenOverlayManager->EnableShadows,0,"Shadows|No Shadows|No Lights");
  con->Style |= sCS_SMALLWIDTH;
  tb->AddChild(con);

  con = new sControl;
  con->EditCycle(CMD_MENU_SOUND,&GenOverlayManager->SoundEnable,0,"|Sound");
  con->Style |= sCS_SMALLWIDTH;
  tb->AddChild(con);

  con = new sControl;
  con->EditChoice(CMD_MENU_SOUND,&ViewWin->CamSpeed,0,"Slow|Normal Speed|Fast|Faster");
//  con->Style |= sCS_SMALLWIDTH;
  tb->AddChild(con);

  AddBorder(tb);

  Status = new sStatusBorder;
  AddBorder(Status);
  AddBorder(TimeBorderWin);
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

// file requester

  FileWindow = new sFileWindow;
  FileWindow->AddTitle("File Operations");
  FileWindow->Flags |= sGWF_TOPMOST;
  sCopyString(buffer,"disks/",sizeof(buffer));
  sSystem->GetCurrentDir(buffer+6,sizeof(buffer)-6);
  FileWindow->SetPath(buffer);
  sAppendString(buffer,"/default.k",sizeof(buffer));
  FileWindow->SetPath(buffer);

  AddTitle("Werkkzeug "VERSION,0,sCMDLS_EXIT);

// document

  Doc = new WerkDoc;
  Doc->App = this;
  Doc2 = new WerkDoc2;
  Doc2->App = this;
  TotalSample = 0;
  MusicReset();

  LoadConfig();       // we have to load config before calling OwnAll() and after setting up filerequester

  page = Doc->AddPage("start");
  Doc->OwnAll();
  PageWin->SetPage(page);

// initial load

  FileWindow->GetPath(buffer,sizeof(buffer));
  di = sDiskRoot->Find(buffer,sDIF_EXISTING);
  if(di)
  {
    sGui->Post(sCMDLS_READ,this);
    AutoSaveTimer = sSystem->GetTime();
    sSystem->SetSoundHandler(WerkkzeugSoundHandler,64,this);
  }

// nova defaults
/*
  {
    WerkEvent2 *we;
    WerkScene2 *ws;
    WerkSceneNode2 *wn;
    WerkSpline *sp;

    we = Doc2->MakeEvent(Doc2->FindEffect("camera"));
    we->Line = 1;
    we->Event.Start = 0;
    we->Event.End = 0x200000;
    we->Source->Init("Matrix.l = [0,0,-5,1];\n//ClearColor = eval(time,testspline);\n");
    Doc2->Events->Add(we);

    sp = new WerkSpline;
    sp->Init(4,"testspline",0,1,0);
    sp->AddKey(0,0.5f,1.0f);
    sp->AddKey(1,0.5f,1.0f);
    sp->AddKey(2,0.5f,1.0f);
    sp->Sort();
    we->Splines->Add(sp);

    we = Doc2->MakeEvent(Doc2->FindEffect("mesh"));
    we->Line = 2;
    we->Event.Start = 0;
    we->Event.End = 0x100000;
    sCopyString(we->LinkName[0],"mesh_test",KK_NAME);
    Doc2->Events->Add(we);

    we = Doc2->MakeEvent(Doc2->FindEffect("scene"));
    we->Line = 2;
    we->Event.Start = 0x100000;
    we->Event.End = 0x200000;
    we->Source->Init("print(time);\n");
    sCopyString(we->LinkName[0],"test2",KK_NAME);
    Doc2->Events->Add(we);

    we = Doc2->MakeEvent(Doc2->FindEffect("glare"));
    we->Line = 3;
    we->Event.Start = 0;
    we->Event.End = 0x200000;
    Doc2->Events->Add(we);

    Doc2->SortEvents();


    ws = new WerkScene2;
    sCopyString(ws->Name,"test1",KK_NAME);
    Doc2->Scenes->Add(ws);

    ws = new WerkScene2;
    sCopyString(ws->Name,"test2",KK_NAME);
    Doc2->Scenes->Add(ws);

    wn = new WerkSceneNode2;
    sCopyString(wn->MeshName,"mesh1",KK_NAME);
    sCopyString(wn->Name,"node1",KK_NAME);
    wn->Node->Matrix.l.Init(0,0,5,1);
    ws->Nodes->Add(wn);

    wn = new WerkSceneNode2;
    sCopyString(wn->MeshName,"mesh2",KK_NAME);
    sCopyString(wn->Name,"node2",KK_NAME);
    ws->Nodes->Add(wn);

    SceneListWin->UpdateList();
  }
*/
  LogWin->PrintLine(APPNAME" "VERSION" started.");
}

WerkkzeugApp::~WerkkzeugApp()
{
  sSystem->SetSoundHandler(0,0,0);

  for(sInt i=0;EffectList[i].Name;i++)
  {
    EffectList[i].Info->Exit();
    sDelete(EffectList[i].Info);
  }
}

void WerkkzeugApp::Tag()
{
  sGuiWindow::Tag();
  sBroker->Need(FileWindow);
  sBroker->Need(Doc);
  sBroker->Need(Doc2);
  sBroker->Need(Music);
  sBroker->Need(OpBrowserWin);
  sBroker->Need(FindResultsWin);
}

/****************************************************************************/

void WerkkzeugApp::OnPaint()
{
//  sPainter->Paint(sGui->FlatMat,Client,0xffff0000);
}

void WerkkzeugApp::OnCalcSize()
{
}

void WerkkzeugApp::OnKey(sU32 key)
{
  switch(key & 0x8001ffff)
  {
  case sKEY_APPCLOSE:
    Parent->RemChild(this);
    break;
  }
}

void WerkkzeugApp::OnDrag(sDragData &)
{
}

void WerkkzeugApp::OnFrame()
{
  sInt time;
  if(AutoSaveMax>0)
  {
    time = sSystem->GetTime();
    if(time > AutoSaveTimer + AutoSaveMax*1000*60)
    {
      AutoSaveTimer = sSystem->GetTime();
      OnCommand(sCMDLS_AUTOSAVE);
    }
  }
}

sBool WerkkzeugApp::OnDebugPrint(sChar *t)
{
  static sInt error;
  if(LogWin)
  {
    LogWin->PrintFLine("%04d: %s",error++,t);
  }
  return sFALSE;
}

sBool WerkkzeugApp::OnShortcut(sU32 key)
{
  if(key&sKEYQ_CTRL)  key|=sKEYQ_CTRL;
  if(key&sKEYQ_SHIFT) key|=sKEYQ_SHIFT;
  switch(key)
  {

// old keys

  case sKEY_F5:
    OnCommand(CMD_MUSIC_START);
    return sTRUE;
  case sKEY_F6:
  case '<':
    OnCommand(CMD_MUSIC_TOGGLE);
    return sTRUE;
  case sKEY_F7:
    OnCommand(CMD_MUSIC_LOOP);
    return sTRUE;
  case sKEY_F8:
    return sTRUE;

  case sKEY_F9:
    NovaMode=!NovaMode;
    OnCommand(CMD_MENU_NOVA);
    return sTRUE;

  case sKEY_TAB|sKEYQ_ALT:      // don't give alt-tab to app.
    return sTRUE;

/*// sample xxx

  case '1'|sKEYQ_CTRL:
    sSystem->SamplePlay(0);
    return sTRUE;
  case '2'|sKEYQ_CTRL:
    sSystem->SamplePlay(1);
    return sTRUE;
  case '3'|sKEYQ_CTRL:
    sSystem->SamplePlay(2);
    return sTRUE;
  case '4'|sKEYQ_CTRL:
    sSystem->SamplePlay(3);
    return sTRUE;*/

// new keys

  case sKEY_F1:
    OnCommand(CMD_MENU_PAGE);
    return sTRUE;
  case sKEY_F2:
    OnCommand(CMD_MENU_TIMELINE);
    return sTRUE;
  case sKEY_F3:
    OnCommand(CMD_MENU_SPLINE);
    return sTRUE;
  case sKEY_F4:
    OnCommand(CMD_MENU_SCENE);
    return sTRUE;
  case 'i'|sKEYQ_CTRL:
    OnCommand(CMD_EDIT_STATS);
    return sTRUE;
  case 'g'|sKEYQ_CTRL:
    OnCommand(CMD_EDIT_GAMEINFO);
    return sTRUE;
  case 'p'|sKEYQ_CTRL:
    OnCommand(CMD_EDIT_PERFMON);
    return sTRUE;
  case 'l'|sKEYQ_CTRL:
    OnCommand(CMD_EDIT_LOG);
    return sTRUE;
  case 'e'|sKEYQ_CTRL:
    GenOverlayManager->EnableShadows = (GenOverlayManager->EnableShadows+1)%3;
    return sTRUE;

  case sKEY_PAUSE:
    ViewWin->SetOff();
    ViewNovaWin->SetOff();
    return sTRUE;
  case 'a'|sKEYQ_CTRL:
    sGui->Post(sCMDLS_SAVEAS,this);
    return sTRUE;
  case 's'|sKEYQ_CTRL:
    sGui->Post(sCMDLS_SAVE,this);
    return sTRUE;
  case 'o'|sKEYQ_CTRL:
    sGui->Post(sCMDLS_OPEN,this);
    return sTRUE;
  case 'q'|sKEYQ_CTRL:
    sGui->Post(sCMDLS_QUICKSAVE,this);
    return sTRUE;
  case 'b'|sKEYQ_CTRL:
    sGui->Post(sCMDLS_BROWSER,this);
    return sTRUE;
  case 'r'|sKEYQ_CTRL:
    sGui->Post(CMD_PAGE_RENAME,PageWin);
    return sTRUE;
  default:
    return sFALSE;
  }
}

sBool WerkkzeugApp::OnCommand(sU32 cmd)
{
  sMenuFrame *mf;
  sDialogWindow *dw;
  static sChar buffer[sDI_PATHSIZE];
  static sChar classname[64];
  sInt i,max;

  switch(cmd)
  {
  case 1:
    return sTRUE;

  case CMD_MENU_FILE:
    mf = new sMenuFrame;
    mf->SendTo = this;
    mf->AddBorder(new sNiceBorder);

    mf->AddMenu("New",sCMDLS_NEW,0);
    mf->AddMenu("Open",sCMDLS_OPEN,'o'|sKEYQ_CTRL);
    mf->AddMenu("Merge",sCMDLS_MERGE,0);
    mf->AddMenu("Save as..",sCMDLS_SAVEAS,'a'|sKEYQ_CTRL);
    mf->AddMenu("Save",sCMDLS_SAVE,'s'|sKEYQ_CTRL);
    mf->AddMenu("Quicksave",sCMDLS_QUICKSAVE,'q'|sKEYQ_CTRL);
    mf->AddSpacer();
    mf->AddMenu("Render Video",CMD_FILE_RENDERVIDEO,0);
    mf->AddMenu("Export",CMD_FILE_EXPORT,0);
    mf->AddMenu("Make Demo",CMD_FILE_MAKEDEMO,0);
    mf->AddMenu("Own All",CMD_FILE_OWNALL,0);
    mf->AddSpacer();
    mf->AddMenu("Browser",sCMDLS_BROWSER,'b'|sKEYQ_CTRL);
    mf->AddMenu("Save Config",CMD_FILE_SAVECONFIG,0);
    mf->AddMenu("Exit",sCMDLS_EXIT,sKEYQ_SHIFT|sKEY_ESCAPE);
    sGui->AddPulldown(mf);
    return sTRUE;

  case CMD_MENU_EDIT:
    mf = new sMenuFrame;
    mf->SendTo = this;
    mf->AddBorder(new sNiceBorder);

    mf->AddMenu("Editor Settings",CMD_EDIT_EDITOR,0);
    mf->AddMenu("Demo Settings",CMD_EDIT_DEMO,0);
    mf->AddMenu("Statistics",CMD_EDIT_STATS,sKEYQ_CTRL|'i');
    mf->AddMenu("Game Info",CMD_EDIT_GAMEINFO,sKEYQ_CTRL|'g');
    mf->AddMenu("Performance Monitor",CMD_EDIT_PERFMON,sKEYQ_CTRL|'p');
    mf->AddMenu("Log",CMD_EDIT_LOG,sKEYQ_CTRL|'l');
    mf->AddSpacer();
    mf->AddCheck("Low Textures",CMD_EDIT_TEXL,0,GenBitmapTextureSizeOffset==-1);
    mf->AddCheck("Normal Textures",CMD_EDIT_TEXM,0,GenBitmapTextureSizeOffset==0);
    mf->AddCheck("High Textures",CMD_EDIT_TEXH,0,GenBitmapTextureSizeOffset==1);
    mf->AddSpacer();
    mf->AddMenu("Find Class",CMD_EDIT_FINDCLASS,0);
    mf->AddMenu("Find Bugs",CMD_EDIT_FINDBUGS,0);
    mf->AddMenu("Clear Respawn Points",CMD_EDIT_CLEARRESPAWN,0);
    mf->AddMenu("Find Switches",CMD_EDIT_FINDSWITCH,0);
    mf->AddSpacer();
    mf->AddMenu("Start Music",CMD_MUSIC_START,sKEY_F5);
    mf->AddMenu("Toggle Music",CMD_MUSIC_TOGGLE,sKEY_F6);
    mf->AddMenu("Toggle Music",CMD_MUSIC_TOGGLE,'>');
    mf->AddMenu("Loop Music",CMD_MUSIC_LOOP,sKEY_F7);
    sGui->AddPulldown(mf);
    return sTRUE;

  case CMD_MENU_PAGE:
    SetMode(MODE_PAGE);
    return sTRUE;
  case CMD_MENU_SPLINE:
    SetMode(MODE_SPLINE);
    return sTRUE;
  case CMD_MENU_TIMELINE:
    if(NovaMode)
      SetMode(MODE_EVENT2);
    else
      SetMode(MODE_EVENT);
    return sTRUE;
  case CMD_MENU_SCENE:
    SetMode(MODE_SCENE);
    return sTRUE;
  case CMD_MENU_NOVA:
    Doc->Connect();       // debug. remove this one when nova-loading is implemented
    Doc2->UpdateLinks(Doc);
    SetMode(-1);
    return sTRUE;

  case CMD_MENU_SHADER:
    Doc->ChangeClass(0xd0);
    return sTRUE;

  case CMD_EDIT_EDITOR:
    {
      WinEditPara *ep;
      ep = new WinEditPara;
      ep->SetApp(this);
      sGui->AddWindow(ep,200,50);
    }
    return sTRUE;

  case CMD_EDIT_DEMO:
    {
      WinDemoPara *ep;
      ep = new WinDemoPara;
      ep->SetApp(this);
      sGui->AddWindow(ep,200,50);
    }
    return sTRUE;

  case CMD_EDIT_STATS:
    SetMode(MODE_STATISTIC);
    return sTRUE;
  case CMD_EDIT_GAMEINFO:
    SetMode(MODE_GAMEINFO);
    return sTRUE;
  case CMD_EDIT_PERFMON:
    SetMode(MODE_PERFMON);
    return sTRUE;
  case CMD_EDIT_LOG:
    SetMode(MODE_LOG);
    return sTRUE;

  case CMD_EDIT_TEXL:
    GenBitmapTextureSizeOffset = -1;
    Doc->Flush(KC_BITMAP);
    return sTRUE;

  case CMD_EDIT_TEXM:
    GenBitmapTextureSizeOffset = 0;
    Doc->Flush(KC_BITMAP);
    return sTRUE;

  case CMD_EDIT_TEXH:
    GenBitmapTextureSizeOffset = 1;
    Doc->Flush(KC_BITMAP);
    return sTRUE;

  case CMD_EDIT_FINDCLASS:
    dw = new sDialogWindow;
    dw->InitString(classname,KK_NAME);
    dw->InitOkCancel(this,"Find Class","Name of Class to look for..",CMD_EDIT_FINDCLASS2,0);
    return sTRUE;

  case CMD_EDIT_FINDCLASS2:
    Doc->DumpOpByClass(classname);
    FindResults(sTRUE);
    return sTRUE;

  case CMD_EDIT_FINDBUGS:
    Doc->DumpBugs();
    FindResults(sTRUE);
    return sTRUE;

  case CMD_EDIT_CLEARRESPAWN:
    for(i=0;i<16;i++)
      Doc->KEnv->Game->Switches[KGS_RESPAWN+i] = 0;
    return sTRUE;

  case CMD_EDIT_FINDSWITCH:
    Doc->DumpUsedSwitches();
    SetMode(MODE_LOG);
    return sTRUE;

  case CMD_MUSIC_START:
    MusicSeek(MusicLoop0);
    MusicStart(1);
    return sTRUE;
  case CMD_MUSIC_TOGGLE:
    MusicStart(!MusicPlay);
    return sTRUE;
  case CMD_MUSIC_LOOP:
    MusicLoop = !MusicLoop;
    return sTRUE;

  case CMD_FILE_SAVECONFIG:
    if(!SaveConfig())
      (new sDialogWindow)->InitOk(this,"Save","Save Config Failed",0);
    return sTRUE;

  case CMD_FILE_EXPORT:
    if(FileWindow->ChangeExtension(buffer,".kx"))
      Export(buffer);
    return sTRUE;

  case CMD_FILE_MAKEDEMO:
    if(FileWindow->ChangeExtension(buffer,".exe"))
      MakeDemo(buffer);
    return sTRUE;

  case CMD_FILE_RENDERVIDEO:
    if(FileWindow->ChangeExtension(buffer,".avi"))
    {
      if(!sCmpMem(buffer,"Disks/",6))
      {
        SetMode(MODE_LOG);
        ViewWin->RecordAVI(buffer+6,30);
      }
    }
    return sTRUE;

  case CMD_FILE_OWNALL:
    sSPrintF(buffer,sizeof(buffer),"Change the ownership of all files to yourself (%s)?\n This will make future merges quite impossible!",UserName);
    (new sDialogWindow)->InitYesNo(this,"Own All",buffer,CMD_FILE_OWNALL2,0);
    return sTRUE;
  case CMD_FILE_OWNALL2:
    Doc->OwnAll();
    return sTRUE;

  case sCMDLS_CLEAR:
    Clear();
    return sTRUE;

  case sCMDLS_READ:
    if(sImplementFileMenu(cmd,this,this,FileWindow,sTRUE/*DocChangeCount!=CheckChange()*/))
    {
//      if(Docs->GetCount()>0 && Docs->Get(0)->GetClass()==sCID_TOOL_PAGEDOC)
//        if(FindActiveWindow(sCID_TOOL_PAGEWIN))
//          ((PageWin *)FindActiveWindow(sCID_TOOL_PAGEWIN))->Doc = (PageDoc *) Docs->Get(0);
      FileWindow->GetFile(buffer,sizeof(buffer));
      sSPrintF(WinTitle,sizeof(WinTitle),"%s %s -- %s",APPNAME,VERSION,buffer);
      sSystem->SetWinTitle(WinTitle);
      if(Doc->OutdatedOps)
      {
        Doc->OutdatedOps = 0;
        (new sDialogWindow)->InitOk(this,"outdated version","you are using an outdated version of the werkkzeug. some operators contain paramters that this version can not handle.\n\ndata will be lost if you continue!\n",0);
      }
    }
    SetStat(STAT_MESSAGE,"Loaded file.");
    sDPrintF("Load complete.\n");
    SaveConfig();
    return sTRUE;

  case sCMDLS_QUICKSAVE:
    sImplementFileMenu(cmd,this,this,FileWindow,sTRUE/*DocChangeCount!=CheckChange()*/);
    SetStat(STAT_MESSAGE,"Quicksave.");
    return sTRUE;

  case sCMDLS_AUTOSAVE:
    sImplementFileMenu(cmd,this,this,FileWindow,sTRUE/*DocChangeCount!=CheckChange()*/);
    SetStat(STAT_MESSAGE,"Autosave.");
    return sTRUE;

  case sCMDLS_WRITE:
    sImplementFileMenu(cmd,this,this,FileWindow,sTRUE/*DocChangeCount!=CheckChange()*/);
    SetStat(STAT_MESSAGE,"Saved file.");
    max = Doc->Pages->GetCount();
    for(i=0;i<max;i++)
      Doc->Pages->Get(i)->Changed=0;
    FileWindow->GetFile(buffer,sizeof(buffer));
    sSPrintF(WinTitle,sizeof(WinTitle),"%s %s -- %s",APPNAME,VERSION,buffer);
    sSystem->SetWinTitle(WinTitle);
    PagelistWin->UpdateList();
    return sTRUE;

  case CMD_BROWSERPREVIEW:
    if(OpBrowserWin)
    {
      WerkOp *op;

      OpBrowserWin->GetFileName(buffer,sizeof(buffer));
      op = Doc->FindName(buffer);
      if(op)
      {
        ViewWin->SetObjectB(op);
        WerkOp *op2 = op;
        while(op2 && op2->Class->Output==KC_ANY)
        {
          if(op2->GetInput(0))
            op2 = op2->GetInput(0);
          else if(op2->GetLink(0))
            op2 = op2->GetLink(0);
          else
            op2 = 0;
        }
        if(op2 && op2->Class && op2->Class->Output==KC_MATERIAL)
          ViewWin->OnCommand(CMD_VIEW_RESET);
      }
    }
    return sTRUE;


  default:
    return sImplementFileMenu(cmd,this,this,FileWindow,sTRUE/*DocChangeCount!=CheckChange()*/);
  }
}


/****************************************************************************/
  
void WerkkzeugApp::Clear()
{
  Doc->Clear();
  PageWin->SetPage(Doc->AddPage("start"));
  ParaWin->Reset();
  ViewWin->SetObject(0);
  PagelistWin->UpdateList();
  sGui->GarbageCollection();
}

sBool WerkkzeugApp::Write(sU32 *&data)
{
  WerkPage *page;
  sInt i,j,max;


// update user counts

  max = Doc->Pages->GetCount();
  j = UserCount+1;
  for(i=0;i<max;i++)
  {
    page = Doc->Pages->Get(i);
    if(sCmpString(page->Owner,UserName)==0)
      j = sMax(j,page->OwnerCount);
  }
  for(i=0;i<max;i++)
  {
    page = Doc->Pages->Get(i);
    if(page->Changed && sCmpString(page->Owner,UserName)==0)
    {
      page->OwnerCount = j;
      UserCount = j;
    }
  }
  SaveConfig();
  PagelistWin->UpdateList();

// flush

  AutoSaveTimer = sSystem->GetTime();
  PageWin->SetPage(PageWin->Page);

// write

  *data++ = sMAKE4('n','o','v','a');
  *data++ = 1;
  if(!Doc->Write(data)) return 0;
  if(!Doc2->Write(data)) return 0;
  return 1;
}

sBool WerkkzeugApp::Read(sU32 *&data)
{
  sBool error;

  Clear();
  AutoSaveTimer = sSystem->GetTime();

  if(*data==sMAKE4('n','o','v','a'))
  {
    sInt version;
    data++;
    error = 0;
    version = *data++;
    if(version>=1 && version<=1) 
    {
      if(Doc->Read(data))
      {
        Doc->Connect();
        if(Doc2->Read(data))
        {
          error = 1;
        }
      }
    }
  }
  else
  {
    error = Doc->Read(data);
  }

  Doc->Connect();
  Doc2->SortEvents();
  Doc2->UpdateLinks(Doc);
  Doc2->Build();

  SceneListWin->UpdateList();
  SceneNodeWin->UpdateList();

  PageWin->SetPage(Doc->Pages->Get(0));
  ParaWin->Reset();
  PagelistWin->UpdateList();
  SetMode(MODE_STATISTIC);
  
  MusicLoad();

  return error;
}

sBool WerkkzeugApp::Merge(sU32 *&data)
{
  WerkDoc *mdoc;
  WerkPage *mpage;
  WerkPage *page;
  sBool ok,found;
  sInt i,j;

// before merging, get rid of all scratchpads


// load the merge-doc

  mdoc = new WerkDoc;
  mdoc->App = this;
  ok = mdoc->Read(data);

// before merging, get rid of all scratchpads

  for(i=0;ok && i<Doc->Pages->GetCount();i++)
    if(Doc->Pages->Get(i)->Scratch)
      ok = sFALSE;

  for(i=0;ok && i<mdoc->Pages->GetCount();i++)
    if(mdoc->Pages->Get(i)->Scratch)
      ok = sFALSE;

// error?

  if(!ok)
  {
    delete mdoc;
    SetMode(MODE_STATISTIC);
    return sFALSE;
  }

// merge pages;

  for(i=0;i<mdoc->Pages->GetCount();i++)
  {
    mpage = mdoc->Pages->Get(i);
    found = 0;
    for(j=0;j<Doc->Pages->GetCount() && !found;j++)
    {
      page = Doc->Pages->Get(j);
      if(sCmpString(page->Name,mpage->Name)==0)
      {
        found = 1;
        page->Scratch = CopyOpList(mpage->Orig,page);
        page->MergeError = ME_KEEP;
        if(sCmpString(page->Owner,mpage->Owner)==0 && mpage->OwnerCount > page->OwnerCount)
        {
          page->MergeError = ME_UPDATE;
          sSwap(page->Scratch,page->Orig);
          page->Ops = page->Orig;
        }
        if(sCmpString(mpage->NextOwner,UserName)==0)
          sCopyString(page->Owner,UserName,KK_NAME);
        else
          sCopyString(page->NextOwner,mpage->NextOwner,KK_NAME);
      }
    }
    if(!found)
    {
      mpage = Doc->AddCopyOfPage(mpage);
      mpage->MergeError = ME_NEW;
      mpage->Doc = Doc;
      if(sCmpString(mpage->NextOwner,UserName)==0)
      {
        sCopyString(mpage->Owner,UserName,KK_NAME);
        mpage->NextOwner[0] = 0;
      }
    }
  }

// done

  mdoc->Clear();
  sGui->GarbageCollection();
  PagelistWin->FullInfo = 1;
  PagelistWin->UpdateList();

  SetMode(MODE_STATISTIC);
  return sTRUE;
}

/****************************************************************************/

sBool WerkkzeugApp::SaveConfig()
{
  sU8 mem[1024+sDI_PATHSIZE];
  sU32 *data;
  sU32 flags;
  sU32 *hdr;
  sChar path[sDI_PATHSIZE];

  flags = sGui->GetStyle();

  data = (sU32 *) mem;
  hdr = sWriteBegin(data,sCID_WERKK3CONFIG,7);

  *data++ = (flags & sGS_MAXTITLE) ? 1 : 0;
  *data++ = (flags & sGS_SMALLFONTS) ? 1 : 0;
  *data++ = (flags & sGS_SCROLLBARS) ? 1 : 0;
  *data++ = sGui->Palette[sGC_BACK];
  *data++ = sGui->Palette[sGC_BUTTON];
  *data++ = sGui->Palette[sGC_SELBACK];
  *data++ = sGui->Palette[sGC_TEXT];

  *data++ = AutoSaveMax;
  *data++ = UserCount;
  *data++ = GenBitmapTextureSizeOffset;
  *data++ = MusicVolume;
  *data++ = GenOverlayManager->CurrentShader;
  *data++ = KMemoryManager->GetBudget(KC_BITMAP);
  *data++ = KMemoryManager->GetBudget(KC_MESH);
  *(sF32 *)data = GenOverlayManager->CullDistance; data++;
  *(sF32 *)data = GenOverlayManager->AutoLightRange; data++;

  *data++ = 1; // ViewWin->ShowCollision;
  *data++ = ViewWin->BitmapZoom;
  *data++ = (flags & sGS_SKIN05) ? 1 : 0;
  *data++ = 0;
  *data++ = ViewWin->WireColor[0];
  *data++ = ViewWin->WireColor[1];
  *data++ = ViewWin->WireColor[2];
  *data++ = ViewWin->WireColor[3];
  *data++ = ViewWin->WireColor[4];
  *data++ = ViewWin->WireColor[5];
  *data++ = ViewWin->WireColor[6];
  *data++ = ViewWin->WireColor[7];
  *data++ = ViewWin->WireColor[8];
  *data++ = ViewWin->WireColor[9];
  *data++ = ViewWin->WireColor[10];
  *data++ = ViewWin->WireColor[11];

  sWriteString(data,UserName);
  FileWindow->GetPath(path,sizeof(path));
  sWriteString(data,path);

  *data++ = ViewWin->SelEdge;           // V6
  *data++ = ViewWin->SelFace;
  *data++ = ViewWin->SelVertex;
  *data++ = 1; // ViewWin->ShowCollision;

  *data++ = ViewWin->WireColor[12];
  *data++ = ViewWin->WireOptions | 0x80000000;
  *data++ = GenOverlayManager->EnableShadows;
  *data++ = GenOverlayManager->SoundEnable;

  *data++ = 0;
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;
  
  sWriteEnd(data,hdr);
  if(sSystem->SaveFile("werkkzeug.config",mem,((sU8*)data)-mem))
  {
    sDPrintF("Config Safed\n");
    return sTRUE;
  }
  else
  {
    sDPrintF("Config Safe Failed!\n");
    return sFALSE;
  }

}

sBool WerkkzeugApp::LoadConfig()
{
  sU8 *mem;
  sU32 *data;
  sInt version;
  sInt result;
  sU32 color[4];
  sU32 flags;
  sInt budget;
  sChar path[sDI_PATHSIZE];

  result = 1;
  mem = sSystem->LoadFile("werkkzeug.config");
  if(!mem) return 0;
  data = (sU32 *)mem;

  version = sReadBegin(data,sCID_WERKK3CONFIG);
  if(version<1 || version>7) result = 0;

  if(result)
  {
    flags = sGui->GetStyle();
    flags &= ~(sGS_MAXTITLE|sGS_SMALLFONTS|sGS_SCROLLBARS);
    if(*data++) flags |= sGS_MAXTITLE;
    if(*data++) flags |= sGS_SMALLFONTS;
    if(*data++) flags |= sGS_SCROLLBARS;
    color[0] = *data++;
    color[1] = *data++;
    color[2] = *data++;
    color[3] = *data++;
    if(version>=2)
    {
      AutoSaveMax = *data++;
      UserCount = *data++;
      GenBitmapTextureSizeOffset = (sS32) (*data++);
      MusicVolume = *data++;
      data++; // skip current shader version
      //GenOverlayManager->CurrentShader = *data++;

      budget = *data++; if(budget==0) budget = 192<<20;
      KMemoryManager->SetBudget(KC_BITMAP,sRange<sInt>(budget,768<<20,48<<20));

      budget = *data++; if(budget==0) budget = 64<<20;
      KMemoryManager->SetBudget(KC_MESH,sRange<sInt>(budget,768<<20,48<<20));

      GenOverlayManager->CullDistance = *(sF32 *)data; data+=1;
    }
    if(version>=7)
    {
      GenOverlayManager->AutoLightRange = *(sF32 *)data; data++;
    }
    if(version>=3)
    {
      data++; // ShowCollision
      ViewWin->BitmapZoom = sRange<sInt>(*data++,7,3);
      if(*data++) flags |= sGS_SKIN05;
      data++;
      ViewWin->WireColor[ 0] = *data++;
      ViewWin->WireColor[ 1] = *data++;
      ViewWin->WireColor[ 2] = *data++;
      ViewWin->WireColor[ 3] = *data++;
      ViewWin->WireColor[ 4] = *data++;
      ViewWin->WireColor[ 5] = *data++;
      ViewWin->WireColor[ 6] = *data++;
      ViewWin->WireColor[ 7] = *data++;
      ViewWin->WireColor[ 8] = *data++;
      ViewWin->WireColor[ 9] = *data++;
      ViewWin->WireColor[10] = *data++;
      ViewWin->WireColor[11] = *data++;

      if(!ViewWin->WireColor[ 9])  ViewWin->WireColor[ 9] = 0xff404040;
      if(!ViewWin->WireColor[10])  ViewWin->WireColor[10] = 0xff000000;
      if(!ViewWin->WireColor[11])  ViewWin->WireColor[11] = 0xff804040;
      ViewWin->WireColor[12] = 0xff004080;
    }
    if(version>=4)
    {
      sReadString(data,UserName,KK_NAME);
    }
    if(version>=5)
    {
      sReadString(data,path,sizeof(path));
      FileWindow->SetPath(path);
    }
    if(version>=6)
    {
      ViewWin->SelEdge = *data++;
      ViewWin->SelFace = *data++;
      ViewWin->SelVertex = *data++;
      *data++; // ShowCollision

      ViewWin->WireColor[12] = *data++;
      ViewWin->WireOptions = *data++;
      GenOverlayManager->EnableShadows = *data++;
      GenOverlayManager->SoundEnable = *data++;
      data += 8;

      if(!ViewWin->WireColor[12])  ViewWin->WireColor[12] = 0xff004080;
      if(ViewWin->WireOptions & 0x80000000)
        ViewWin->WireOptions &= 0x7fffffff;
      else
        ViewWin->WireOptions = EWF_ALL;
    }
  }

  if(!sReadEnd(data)) result = 0;
  delete[] mem;

  if(result)
  {
    sGui->Palette[sGC_BACK]    = color[0];
    sGui->Palette[sGC_TEXT]    = color[3];
    sGui->Palette[sGC_DRAW]    = color[3];
    sGui->Palette[sGC_HIGH]    = ((color[0]&0xfcfcfc)>>2)+0xffc0c0c0;
    sGui->Palette[sGC_LOW]     = ((color[0]&0xfcfcfc)>>2)+0xff404040;
    sGui->Palette[sGC_HIGH2]   = 0xffffffff;
    sGui->Palette[sGC_LOW2]    = 0xff000000;
    sGui->Palette[sGC_BUTTON]  = color[1];
    sGui->Palette[sGC_BARBACK] = ((color[0]&0xfcfcfc)>>2)+0xffc0c0c0;
    sGui->Palette[sGC_SELECT]  = ((color[0]&0xfefefe)>>1)+0xff808080;
    sGui->Palette[sGC_SELBACK] = color[2];
    sGui->SetStyle(flags);
  }

  if(result)
    sDPrintF("Config Loaded\n");
  else
    sDPrintF("Config Load Failed\n");
  return result;
}




/****************************************************************************/

void WerkkzeugApp::Export(sChar *name)
{
  sDiskItem *di;
  sBool ok;
  sU8 *data,*dataStart;
  static sChar msg[128];
  WerkExport export;

  ok = sTRUE;
  data = dataStart = new sU8[64*1024*1024];

  if(!export.Export(Doc,data))
    ok = sFALSE;
  else
  {
    di = sDiskRoot->Find(name,sDIF_CREATE);
    if(!(di && di->Save(dataStart,(sInt) (data-dataStart))))
      ok = sFALSE;
  }

  if(!ok)
    (new sDialogWindow)->InitOk(this,"Export","Export failed",0);
  else
  {
    sSPrintF(msg,128,"Export ok, size %d bytes!",data-dataStart);
    (new sDialogWindow)->InitOk(this,"Export",msg,0);
  }

  delete[] dataStart;
}

/****************************************************************************/

extern "C" sU8 PlayerDemo[];
extern "C" sU8 PlayerIntro[];
extern "C" sU8 PlayerKKrieger[];

void WerkkzeugApp::MakeDemo(sChar *name)
{
  sDiskItem *di;
  sBool ok;
  sU8 *data,*dataStart;
  sU8 *exe,*packedExe;
  sInt outSize;
  static sChar msg[1024];
  WerkExport export;
  //MAPFile map;
  CCAPackerBackEnd pbe;
  PackerFrontEnd *pfe = 0;
  EXEPacker packer;
  KKBlobList blobs;

  ok = sTRUE;
  data = dataStart = new sU8[64*1024*1024];
#if !sNEWCONFIG
  exe = 0;
  exe = sSystem->LoadFile("..\\player release\\werkkzeug3.exe");
  if(!exe)
    exe = PlayerEXE;
#else
  switch(Doc->PlayerType)
  {
  default: 
    exe = PlayerDemo;
    break;
  case 1:
    exe = PlayerIntro;
    break;
  case 2:
    exe = PlayerKKrieger;
    break;
  }
#endif

  /*mapf = sSystem->LoadFile("..\\player release\\werkkzeug3.map");
  map.Init();
  map.LoadDbgHelp();
  map.ReadMapFile((sChar*)mapf);
  delete[] mapf;*/

  if(!export.Export(Doc,data))
  {
    sSPrintF(msg,sizeof(msg),"Export failed");
    ok = sFALSE;
  }
  else
  {
    blobs.AddBlob(dataStart,data-dataStart);

    if(data - dataStart < 512*1024)
      pfe = new BestPackerFrontEnd(&pbe);
    else
      pfe = new GoodPackerFrontEnd(&pbe);

    packedExe = packer.Pack(pfe,exe,outSize,0,0,&blobs);
    delete pfe;
    pfe = 0;

    /*mapf = (sU8*)map.WriteReport();
    sSystem->SaveFile("analyze.txt",mapf,sGetStringLen((sChar*)mapf));
    delete[] mapf;*/

    if(!packedExe)
    {
      sSPrintF(msg,sizeof(msg),"KKrunchy Error: %s",packer.GetErrorMessage());
      ok = sFALSE;
    }
    else
    {
      di = sDiskRoot->Find(name,sDIF_CREATE);
      if(!di || !di->Save(packedExe,outSize))
      {
        sSPrintF(msg,sizeof(msg),"Save failed");
        ok = sFALSE;
      }
      else
        sSPrintF(msg,sizeof(msg),"Make demo ok, size %d bytes.",outSize);
    }

    delete[] packedExe;
  }

  (new sDialogWindow)->InitOk(this,ok ? "Make demo worked" : "Make demo failed",msg,0);

#if !sNEWCONFIG
  if(exe != PlayerEXE)
    delete[] exe;
#endif
  delete[] dataStart;
}

/****************************************************************************/

void WerkkzeugApp::SetStat(sInt nr,sChar *string)
{ 
  sCopyString(Stat[nr],string,256); 
}

void WerkkzeugApp::SetView(sInt i)
{
  SwitchView->Set(i);
}

void WerkkzeugApp::SetMode(sInt m)
{
  if(m==MODE_NORMAL) // switch back from gamemode/perfmon
  {
    if(RadioPage)       m = MODE_PAGE;
    if(RadioSpline)     m = MODE_SPLINE;
    if(RadioTime)       { if(NovaMode) m = MODE_EVENT2; else m = MODE_EVENT; }
    if(RadioScene)      m = MODE_SCENE;
  }

  if(Switch0Mode != m)
  {
    Switch0Mode = m;
    switch(m)
    {
    default:
    case MODE_PAGE:     // page
      Switch0->Set(0);
      Switch1->Set(0);
      Switch2->Set(0);
      RadioPage = 1;
      RadioTime = 0;
      RadioSpline = 0;
      RadioScene = 0;
      break;
    case MODE_SPLINE:     // Spline
      Switch0->Set(1);
      Switch1->Set(1);
      Switch2->Set(0);
      RadioPage = 0;
      RadioTime = 0;
      RadioSpline = 1;
      RadioScene = 0;
      break;
    case MODE_EVENT:     // Timeline
      Switch0->Set(2);
      Switch1->Set(2);
      Switch2->Set(0);
      RadioPage = 0;
      RadioTime = 1;
      RadioSpline = 0;
      RadioScene = 0;
      break;
    case MODE_SCENE:      // scene editor
      Switch0->Set(8);
      Switch1->Set(3);
      Switch2->Set(1);
      RadioPage = 0;
      RadioTime = 0;
      RadioSpline = 0;
      RadioScene = 1;
      break;

    case MODE_EVENT2:    // Timeline2
      Switch0->Set(7);
      Switch1->Set(4);
      Switch2->Set(0);
      RadioPage = 0;
      RadioTime = 1;
      RadioSpline = 0;
      RadioScene = 0;
      break;

    case MODE_STATISTIC:
      Switch0->Set(3);
      break;
    case MODE_GAMEINFO:
      Switch0->Set(4);
      break;
    case MODE_PERFMON:
      Switch0->Set(5);
      break;
    case MODE_LOG:
      Switch0->Set(6);
      break;
    }
  }
}

sInt WerkkzeugApp::GetMode()
{
  return Switch0Mode;
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   windows                                                            ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/***                                                                      ***/
/***   pagelist                                                           ***/
/***                                                                      ***/
/****************************************************************************/


//#define CMD_WINPAGE_ADDPAGEDOC      0x100
//#define CMD_WINPAGE_DELETE          0x101
//#define CMD_WINPAGE_RENAME          0x102
//#define CMD_WINPAGE_MOVEUP          0x103
//#define CMD_WINPAGE_MOVEDOWN        0x104
//#define CMD_WINPAGE_RENAME2         0x105
#define CMD_WINPAGE_ADDEDITDOC      0x106
#define CMD_WINPAGE_ADDTIMEDOC      0x107
#define CMD_WINPAGE_FULLINFO        0x108
#define CMD_WINPAGE_DELIGATE        0x109
#define CMD_WINPAGE_DELIGATE2       0x10a
#define CMD_WINPAGE_FAKE            0x10b
#define CMD_WINPAGE_MAKESCRATCH     0x10c
#define CMD_WINPAGE_KEEPSCRATCH     0x10d
#define CMD_WINPAGE_DISCARDSCRATCH  0x10e
#define CMD_WINPAGE_TOGGLESCRATCH   0x10f
//#define CMD_WINPAGE_DELETE2         0x110
#define CMD_WINPAGE_TREECHANGE      0x111
#define CMD_WINPAGE_DELETETREE      0x112
#define CMD_WINPAGE_DELETETREE2     0x113

WinPagelist::WinPagelist()
{
  sToolBorder *tb;

  DocList = new sListControl;
  DocList->LeftCmd = 1;
  DocList->MenuCmd = 2;
  DocList->Style = sLCS_TREE|sLCS_HANDLING|sLCS_DRAG;
  DocList->TreeCmd = CMD_WINPAGE_TREECHANGE;
  DocList->AddScrolling(0,1);
  DocList->TabStops[0] = 0;
  DocList->TabStops[1] = 11;
  DocList->TabStops[2] = 22;
  DocList->TabStops[3] = 36+0*80;
  DocList->TabStops[4] = 36+1*80;
  DocList->TabStops[5] = 36+2*80;
  DocList->TabStops[6] = 36+3*80;
  DocList->TabStops[7] = 36+4*80;

  AddChild(DocList);

  tb = new sToolBorder();
  tb->AddMenu("Pages",0);
  AddBorder(tb);

  FullInfo = 0;
  DeligateName[0] = 0;
}

void WinPagelist::OnInit()
{
  UpdateList();
}

WinPagelist::~WinPagelist()
{
}

void WinPagelist::OnCalcSize()
{
  if(Childs)
  {
    SizeX = Childs->SizeX;
    SizeY = Childs->SizeY;
  }
}

void WinPagelist::OnLayout()
{
  if(Childs)
  {
    Childs->Position = Client;
  }
}

void WinPagelist::SetPage(WerkPage *page)
{
  sInt i,max;
  max = App->Doc->Pages->GetCount();
  for(i=0;i<max;i++)
  {
    if(App->Doc->Pages->Get(i)==page)
    {
      DocList->SetSelect(i,1);
      DocList->ScrollTo(i);
    }
  }
}

/****************************************************************************/

void WinPagelist::UpdateList()
{
  sInt i,max;
  WerkPage *page;
  static const sChar *mergetext[] = { ".","keep","update","new" };
  sChar c1,c2;

  Flags |= sGWF_LAYOUT;

  DocList->ClearList();
  max = App->Doc->Pages->GetCount();
  for(i=0;i<max;i++)
  {
    page = App->Doc->Pages->Get(i);
    c1 = ' ';
    if(sCmpString(page->Owner,App->UserName)!=0) c1 = 'p';
    if(page->NextOwner[0]) c1 = 'd';
    if(page->FakeOwner) c1 = 'f';
    c2 = ' ';
    if(page->Scratch)
    {
      if(page->Ops==page->Scratch) c2='s';
      if(page->Ops==page->Orig) c2='o';
    }

    if(FullInfo)
      sSPrintF(page->ListName,sizeof(page->ListName),"%c|%c|%c|%s|%s|%08x|%s|%s",page->Changed?'*':' ',c1,c2,page->Name,page->Owner,page->OwnerCount,page->NextOwner[0]?page->NextOwner:"-/-",mergetext[page->MergeError]);
    else
      sSPrintF(page->ListName,sizeof(page->ListName),"%s",page->Name);
//      sSPrintF(page->ListName,sizeof(page->ListName),"%c|%c|%c|%s",page->Changed?'*':' ',c1,c2,page->Name);
    DocList->Add(page->ListName);
    DocList->SetTree(DocList->GetCount()-1,page->TreeLevel,page->TreeButton);
  }
  DocList->CalcLevel();

  SetPage(App->PageWin->Page);
}

/****************************************************************************/

sBool WinPagelist::OnCommand(sU32 cmd)
{
  sInt i,j,k,level,button;
  WerkPage *page;
  sList<WerkPage> *list;
  sDialogWindow *diag;
  sMenuFrame *mf;

  list = App->Doc->Pages;
  i = DocList->GetSelect();
  if(i>=0 && i<list->GetCount())
    page = list->Get(i);
  else
    page = 0;

  switch(cmd)
  {
  case 1:
    if(page)
      App->PageWin->SetPage(page);
    return sTRUE;
  case 2:
    if(page)
      App->PageWin->SetPage(page);
    mf = new sMenuFrame;
    mf->SendTo = this;
    mf->AddBorder(new sNiceBorder);
    mf->AddMenu("add",sCMD_LIST_ADD,'a');
    mf->AddMenu("delete",sCMD_LIST_DELETE,sKEY_BACKSPACE);
    mf->AddMenu("delete tree",CMD_WINPAGE_DELETETREE,0);
    mf->AddMenu("rename",sCMD_LIST_RENAME,'r');
    mf->AddSpacer();
    mf->AddMenu("move up",sCMD_LIST_MOVEUP,sKEY_UP|sKEYQ_CTRL);
    mf->AddMenu("move down",sCMD_LIST_MOVEDOWN,sKEY_DOWN|sKEYQ_CTRL);
    mf->AddSpacer();
    mf->AddMenu("make spare",CMD_WINPAGE_MAKESCRATCH,0);
    mf->AddMenu("keep spare",CMD_WINPAGE_KEEPSCRATCH,0);
    mf->AddMenu("discard spare",CMD_WINPAGE_DISCARDSCRATCH,0);
    mf->AddMenu("toggle spare",CMD_WINPAGE_TOGGLESCRATCH,0);
    mf->AddSpacer();
    mf->AddMenu("deligate",CMD_WINPAGE_DELIGATE,0);
//    mf->AddMenu("fake",CMD_WINPAGE_FAKE,0);
    mf->AddSpacer();
    mf->AddCheck("full info",CMD_WINPAGE_FULLINFO,0,FullInfo);
    sGui->AddPopup(mf);
    return sTRUE;

  case CMD_WINPAGE_DELETETREE:
    i = DocList->GetSelect();
    if(i>=0 && i<list->GetCount())
    {
      diag = new sDialogWindow;
      diag->InitOkCancel(this,"delete","delete page and\nall pages inside?",CMD_WINPAGE_DELETETREE2,0);
    }
    return sTRUE;

  case CMD_WINPAGE_DELETETREE2:
    i = DocList->GetSelect();
    UpdateList();
    if(i>=0 && i<list->GetCount())
    {
      DocList->GetTree(i,j,button);
      k = i;
      for(;;)
      {
        App->Doc->RemPage(list->Get(i));
        k++;
        if(k>=DocList->GetCount())
          break;
        DocList->GetTree(k,level,button);
        if(level<=j)
          break;
      }
      
      UpdateList();
    }
    return sTRUE;
    


  case sLCS_CMD_RENAME:
    page = App->Doc->Pages->Get(DocList->HandleIndex);
    sVERIFY(page);
    DocList->SetRename(page->Name,KK_NAME);
    return sTRUE;
  case sLCS_CMD_DEL:
    page = App->Doc->Pages->Get(DocList->HandleIndex);
    sVERIFY(page);
    App->Doc->RemPage(page);
    return sTRUE;
  case sLCS_CMD_ADD:
    App->Doc->AddPage(DocList->NameBuffer,DocList->HandleIndex);
    return sTRUE;
  case sLCS_CMD_SWAP:
    App->Doc->Pages->Swap(DocList->HandleIndex);
    return sTRUE;
  case sLCS_CMD_UPDATE:
    UpdateList();
    return sTRUE;
  case sLCS_CMD_EXCHANGE:
    page = App->Doc->Pages->Get(DocList->HandleIndex);
    App->Doc->Pages->RemOrder(page);
    App->Doc->Pages->AddPos(page,DocList->InsertIndex);
    UpdateList();
    return sTRUE;
  case sLCS_CMD_DUP:    // can't copy pages..
    return sTRUE;
/*
  case CMD_WINPAGE_FAKE:
    if(page && sCmpString(page->Owner,App->UserName)!=0)
    {
      page->FakeOwner = 1;
      UpdateList();
      if(App->ParaWin->Op) App->ParaWin->SetOp(App->ParaWin->Op);
    } 
    return sTRUE;
*/
  case CMD_WINPAGE_DELIGATE:
    if(page && page->Access())
    {
      diag = new sDialogWindow;
      if(page->NextOwner[0])
        sCopyString(DeligateName,page->NextOwner,KK_NAME);
      diag->InitString(DeligateName,KK_NAME);
      diag->InitOkCancel(this,"deligate to","enter new user",CMD_WINPAGE_DELIGATE2,0);
    }
    return sTRUE;

  case CMD_WINPAGE_DELIGATE2:
    if(page && page->Access())
    {
      sCopyString(page->NextOwner,DeligateName,KK_NAME);
      UpdateList();
      if(App->ParaWin->Op) App->ParaWin->SetOp(App->ParaWin->Op);
    }
    return sTRUE;

  case CMD_WINPAGE_FULLINFO:
    FullInfo = !FullInfo;
    UpdateList();
    return sTRUE;

  case CMD_WINPAGE_MAKESCRATCH:
    if(page)
      page->MakeScratch();
    return sTRUE;

  case CMD_WINPAGE_KEEPSCRATCH:
    if(page && page->Access())
      page->KeepScratch();
    return sTRUE;

  case CMD_WINPAGE_DISCARDSCRATCH:
    if(page)
      page->DiscardScratch();
    return sTRUE;

  case CMD_WINPAGE_TOGGLESCRATCH:
    if(page)
      page->ToggleScratch();
    return sTRUE;

  case CMD_WINPAGE_TREECHANGE:
    for(i=0;i<DocList->GetCount() && i<App->Doc->Pages->GetCount();i++)
    {
      page = App->Doc->Pages->Get(i);
      DocList->GetTree(i,page->TreeLevel,page->TreeButton);
    }
    return sTRUE;

  default:
    return sFALSE;
  }
}

sBool WinPagelist::OnShortcut(sU32 key)
{
/*
  if(key&sKEYQ_CTRL) key|=sKEYQ_CTRL;
  switch(key&(0x8001ffff|sKEYQ_CTRL))
  {
  case 'a':
    OnCommand(CMD_WINPAGE_ADDPAGEDOC);
    return sTRUE;
  case sKEY_BACKSPACE:
  case sKEY_DELETE:
    OnCommand(CMD_WINPAGE_DELETE);
    return sTRUE;
  case 'r':
    OnCommand(CMD_WINPAGE_RENAME);
    return sTRUE;
  case sKEY_UP|sKEYQ_CTRL:
    OnCommand(CMD_WINPAGE_MOVEUP);
    return sTRUE;
  case sKEY_DOWN|sKEYQ_CTRL:
    OnCommand(CMD_WINPAGE_MOVEDOWN);
    return sTRUE;
  }
  */
  return sFALSE;
}

/****************************************************************************/
/***                                                                      ***/
/***   Music!                                                             ***/
/***                                                                      ***/
/****************************************************************************/

void WerkkzeugSoundHandler(sS16 *steriobuffer,sInt samples,void *user)
{
  ((WerkkzeugApp *)user)->SoundHandler(steriobuffer,samples);
}

void WerkkzeugApp::SoundHandler(sS16 *buffer,sInt samples)
{
  sInt time=0;
  sInt i,j;
  sInt a;
  sInt vol;
  static sInt ao;

  if(MusicPlay && Doc)
  {
    vol = GenOverlayManager->SoundEnable ? MusicVolume : 0;
    if(Music)
    {
      Music->PlayPos = TimeNow;
      Music->Handler(buffer,samples,vol);
      TimeNow+=samples;
    }
    else
    {
      for(i=0;i<samples;i+=64)
      {
        time = TimeNow;
        a = 0x7000-(BeatNow&0xffff);
        if(a<0) a = 0;
        for(j=0;j<64;j++)
        {
          buffer[0] = (sFtol(sFSin((time+j)*0.0501f)*((a*j+ao*(63-j))/64))*vol)>>8;
          buffer[1] = (sFtol(sFSin((time+j)*0.0503f)*((a*j+ao*(63-j))/64))*vol)>>8;
          buffer+=2;
        }
        ao = a;
        TimeNow += 64;
      }
    }
    BeatNow = sDivShift(TimeNow,TimeSpeed);
    if(MusicLoop && BeatNow >= MusicLoop1)
    {
      BeatNow = BeatNow - (MusicLoop1-MusicLoop0);
      if(BeatNow<0)
        BeatNow = 0;
      TimeNow = sMulShift(BeatNow,TimeSpeed);
    }
  }
  else
  {
    sSetMem4((sU32 *)buffer,0,samples);
  }
  TotalSample+=samples;
}

void WerkkzeugApp::MusicReset()
{
  sSystem->SetSoundHandler(0,0,0);
  Music = 0;
  MusicLoop0 = 0x000000;
  MusicLoop1 = 0x040000;
  MusicLoopTaken = 0;
  MusicLoop = 0;
  MusicPlay = 0;
  BeatNow = 0;
  TimeNow = 0;
  TimeSpeed = 0; 
  LastTime = 0;
}


struct VFXEntry
{
  sU8 MajorId;
  sU8 MinorId;
  sS8 Gain;
  sU8 reserved;
  sU32 StartPos;
  sU32 EndPos;
  sU32 LoopSize;
};

void WerkkzeugApp::MusicLoad()
{
  const sInt maxsamples = 0x10000;
  VFXEntry *ent;
  sU32 *mem32;
  sU8 *mem;
  sU8 *cache,*outcache,*cacheptr;
  sS16 *mems;
  sF32 *memf;
  sU8 *v2m;
  sInt freq,count,size,cachesize;
  sInt i,j;
  CV2MPlayer *viruz;
  sInt start,len;
  sInt handle;
  sF32 amp,ampx;
  sInt fade;
  sChar cachename[256];

  sSystem->SampleRemAll();
  if(Doc->SampleName[0])
  {
    mems = new sS16[maxsamples*2];
    memf = new sF32[maxsamples*2];
    viruz = new CV2MPlayer;

    sSPrintF(cachename,sizeof(cachename),"%s.sfxcache",Doc->SampleName);

    mem = sSystem->LoadFile(Doc->SampleName);
    mem32 = (sU32 *)mem;

    if(mem)
    {
      sVERIFY(mem32[0]==sMAKE4('V','F','X','0')); 
      size = mem32[1];
      v2m = (sU8*)&mem32[2];
      mem32 = (sU32 *) (((sU8 *)mem32)+size);
      freq = mem32[2];
      count = mem32[3];
      ent = (VFXEntry *) &mem32[4];

      cache = sSystem->LoadFileIfNewerThan(cachename,Doc->SampleName,cachesize);
      if(!cache)
      {
        sDPrintF("not using cached sound effects\n");

        if(viruz->Open(v2m))
        {
          cachesize = 0;
          for(i=0;i<count;i++)
          {
            len = ent[i].LoopSize ? ent[i].LoopSize : ent[i].EndPos - ent[i].StartPos;
            if(len > maxsamples)
              len = maxsamples;

            cachesize += len * 4;
          }

          outcache = new sU8[cachesize];
          cacheptr = outcache;

          for(i=0;i<count;i++)
          {
            sDPrintF("%02x %02x %4.1f db:%08x %08x %08x\n",
              ent->MajorId,
              ent->MinorId,
              ent->Gain/5.0f,
              ent->StartPos,
              ent->EndPos,
              ent->LoopSize);

            start = ent->StartPos;
            if(ent->LoopSize)
              start = ent->EndPos-ent->LoopSize;
            len = sMin<sInt>(ent->EndPos-start,maxsamples);
            ampx = sFPow(10.f,(ent->Gain/5.0f)/20.f);
            
            viruz->Play(ent->StartPos);
            viruz->Render(memf,len);
            fade = 1000;
            if(ent->MajorId&1)
              fade = 44100;

            for(j=0;j<len;j++)
            {
              amp = ampx;
              if(j<fade)
                amp = amp*j/fade;
              if(j>len-fade)
                amp = amp*(len-j)/fade;

              mems[j*2+0] = sRange<sF32>(memf[j*2+0]*amp*0x7fff,0x7fff,-0x7fff);
              mems[j*2+1] = sRange<sF32>(memf[j*2+1]*amp*0x7fff,0x7fff,-0x7fff);
            }
            sCopyMem(cacheptr,mems,4*len);
            cacheptr += 4*len;
            handle = sSystem->SampleAdd((sS16 *)mems,len,4,ent->MinorId,!(ent->MajorId & 1));
            ent++;
          }

          sSystem->SaveFile(cachename,outcache,cachesize);
          delete[] outcache;
        }
      }
      else
      {
        sDPrintF("using cached sound effects\n");

        cacheptr = cache;
        for(i=0;i<count;i++)
        {
          len = ent->LoopSize ? ent->LoopSize : ent->EndPos - ent->StartPos;
          if(len > maxsamples)
            len = maxsamples;

          sSystem->SampleAdd((sS16 *)cacheptr,len,4,ent->MinorId,!(ent->MajorId & 1));
          cacheptr += len*4;
          ent++;
        }
      }

      delete[] mem;
      delete[] cache;
    }
    delete[] mems;
    delete[] memf;
    delete viruz;
  }

  MusicReset();

  sInt snlen = sGetStringLen(Doc->SongName);
  if(snlen < 4 || sCmpMem(Doc->SongName + snlen - 4,".ogg",4))
  {
    sInt inSize;
    sU8 *inBuffer = sSystem->LoadFile(Doc->SongName,inSize);

    if(inBuffer)
    {
      sInt convertedSize;
      sU8 *convertBuffer = sViruz2::ConvertV2M(inBuffer,inSize,convertedSize);
      delete[] inBuffer;

      Music = new sViruz2;
      if(!Music->LoadAndCache(Doc->SongName,convertBuffer,convertedSize,sTRUE))
      {
        delete Music;
        Music = 0;
      }
    }
  }
  else
  {
    Music = new sOGGDecoder;

    if(!Music->LoadAndCache(Doc->SongName))
    {
      delete Music;
      Music = 0;
    }
  }

  TotalSample = 0;
  sSystem->SetSoundHandler(WerkkzeugSoundHandler,64,this);
}

void WerkkzeugApp::MusicStart(sInt mode)
{
  MusicPlay = mode;
  TimeSpeed = sFtol(60*44100/Doc->SoundBPM);
  TimeNow = sMulShift(BeatNow,TimeSpeed);
  LastTime = TimeNow;
}

sBool WerkkzeugApp::MusicIsPlaying()
{
  return MusicPlay!=0;
}

void WerkkzeugApp::MusicSeek(sInt beat)
{
  BeatNow = beat;
  MusicStart(MusicPlay);
}

void WerkkzeugApp::MusicNow(sInt &beat,sInt &ms)
{
  if(MusicPlay==1)
  {
    sInt cs = sSystem->GetCurrentSample();
    LastTime = TimeNow - TotalSample + cs;
    if(LastTime<0)
      LastTime = 0;
  }
  beat = TimeSpeed ? sDivShift(LastTime,TimeSpeed) : 0;
  if(MusicLoop && beat >= MusicLoop0)
    MusicLoopTaken = 1;
  if(MusicLoop && MusicLoopTaken && MusicPlay==1 && beat<MusicLoop0)
    beat += (MusicLoop1 - MusicLoop0);

  ms = sMulDiv(beat,60*1000,Doc->SoundBPM*0x10000);
}

/****************************************************************************/
/***                                                                      ***/
/***   Edit Parameter                                                     ***/
/***                                                                      ***/
/****************************************************************************/

WinEditPara::WinEditPara()
{
  App = 0;
}

void WinEditPara::SetApp(WerkkzeugApp *app)
{
  sControl *con;
  sInt mask;
  sInt line;

  App = app;

  Grid = new sGridFrame;
  AddChild(Grid);
  AddTitle("Editor Parameters",1);
  Flags |= sGWF_TOPMOST;
  Grid->SetGrid(16,99,20,sPainter->GetHeight(sGui->PropFont)+8);
  line = 0;

  con = new sControl;
  con->EditCycle(0x110,&GSFonts,0,"Large Fonts|Small Fonts");
  con->LayoutInfo.Init(8,0,16,1);
  Grid->AddChild(con);

  con = new sControl;
  con->EditCycle(0x111,&GSTitle,0,"No Title Bar|Title Bar");
  con->LayoutInfo.Init(8,1,16,2);
  Grid->AddChild(con);

  con = new sControl;
  con->EditCycle(0x112,&GSScroll,0,"Show Scrollbars|Hide Scrollbars");
  con->LayoutInfo.Init(8,2,16,3);
  Grid->AddChild(con);

  con = new sControl;
  con->EditCycle(0x115,&GSSkin05,0,"|Skin'05");
  con->LayoutInfo.Init(8,3,16,4);
  Grid->AddChild(con);

  con = new sControl;
  con->EditURGB(0x101,(sInt *)&Color[0],0);
  con->LayoutInfo.Init(0,line,8,line+1); line++;
  Grid->AddChild(con);

  con = new sControl;
  con->EditURGB(0x101,(sInt *)&Color[1],0);
  con->LayoutInfo.Init(0,line,8,line+1); line++;
  Grid->AddChild(con);

  con = new sControl;
  con->EditURGB(0x101,(sInt *)&Color[2],0);
  con->LayoutInfo.Init(0,line,8,line+1); line++;
  Grid->AddChild(con);

  con = new sControl;
  con->EditURGB(0x101,(sInt *)&Color[3],0);
  con->LayoutInfo.Init(0,line,8,line+1); line++;
  Grid->AddChild(con);

  line++;

  con = new sControl;
  con->EditInt(0x113,&App->AutoSaveMax,"Autosave");
  con->Style |= sCS_SIDELABEL;
  con->InitNum(0,15,0.25,0);
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  Grid->AddChild(con);

  con = new sControl;
  con->EditString(0x113,App->UserName,"User Name",KK_NAME);
  con->Style |= sCS_SIDELABEL;
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  Grid->AddChild(con);

  con = new sControl;
  con->EditInt(0x113,&App->MusicVolume,"Volume");
  con->Style |= sCS_SIDELABEL;
  con->InitNum(0,256,2,0);
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  Grid->AddChild(con);

  con = new sControl;
  con->EditFloat(0x113,&GenOverlayManager->CullDistance,"Cull Distance");
  con->Style |= sCS_SIDELABEL;
  con->InitNum(0,0x10000,4.0f,0);
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  Grid->AddChild(con);

  con = new sControl;
  con->EditFloat(0x113,&GenOverlayManager->AutoLightRange,"Auto Light Range");
  con->Style |= sCS_SIDELABEL;
  con->InitNum(0,1024,1.0f,0);
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  Grid->AddChild(con);

  con = new sControl;
  TextureMem = KMemoryManager->GetBudget(KC_BITMAP) >> 20;
  con->EditInt(0x114,&TextureMem,"Texture Cache");
  con->Style |= sCS_SIDELABEL;
  con->InitNum(48,768,2,0);
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  Grid->AddChild(con);

  con = new sControl;
  MeshMem = KMemoryManager->GetBudget(KC_MESH) >> 20;
  con->EditInt(0x114,&MeshMem,"Mesh Cache");
  con->Style |= sCS_SIDELABEL;
  con->InitNum(48,768,2,0);
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  Grid->AddChild(con);

  con = new sControl;
  con->EditCycle(0x113,&GenOverlayManager->SoundEnable,"Flags","|sound");
  con->Style |= sCS_SIDELABEL;
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  Grid->AddChild(con);

  line++;

  con = new sControl;
  con->EditFloat(0x113,&App->ViewWin->HintSize,"Hint Size");
  con->Style |= sCS_SIDELABEL;
  con->InitNum(0,1,0.001f,0.125);
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  Grid->AddChild(con);

  /*con = new sControl;
  con->EditCycle(0x113,&App->ViewWin->ShowCollision,"Show Collision","off|on");
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  con->Style |= sCS_SIDELABEL;
  Grid->AddChild(con);*/

  con = new sControl;
  con->EditURGB(0x113,(sInt *)&App->ViewWin->WireColor[0],"Wire Sel");
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  con->Style |= sCS_SIDELABEL;
  Grid->AddChild(con);

  con = new sControl;
  con->EditURGB(0x113,(sInt *)&App->ViewWin->WireColor[1],"Wire");
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  con->Style |= sCS_SIDELABEL;
  Grid->AddChild(con);

  con = new sControl;
  con->EditURGB(0x113,(sInt *)&App->ViewWin->WireColor[2],"Wire Sub");
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  con->Style |= sCS_SIDELABEL;
  Grid->AddChild(con);

  con = new sControl;
  con->EditURGB(0x113,(sInt *)&App->ViewWin->WireColor[3],"Col Add");
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  con->Style |= sCS_SIDELABEL;
  Grid->AddChild(con);

  con = new sControl;
  con->EditURGB(0x113,(sInt *)&App->ViewWin->WireColor[4],"Col Sub");
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  con->Style |= sCS_SIDELABEL;
  Grid->AddChild(con);

  con = new sControl;
  con->EditURGB(0x113,(sInt *)&App->ViewWin->WireColor[11],"Col Zone");
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  con->Style |= sCS_SIDELABEL;
  Grid->AddChild(con);

  con = new sControl;
  con->EditURGB(0x113,(sInt *)&App->ViewWin->WireColor[5],"Vertex");
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  con->Style |= sCS_SIDELABEL;
  Grid->AddChild(con);

  con = new sControl;
  con->EditURGB(0x113,(sInt *)&App->ViewWin->WireColor[6],"Hints");
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  con->Style |= sCS_SIDELABEL;
  Grid->AddChild(con);

  con = new sControl;
  con->EditURGBA(0x113,(sInt *)&App->ViewWin->WireColor[7],"Hidden");
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  con->Style |= sCS_SIDELABEL;
  Grid->AddChild(con);

  con = new sControl;
  con->EditURGBA(0x113,(sInt *)&App->ViewWin->WireColor[8],"Face");
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  con->Style |= sCS_SIDELABEL;
  Grid->AddChild(con);

  con = new sControl;
  con->EditURGBA(0x113,(sInt *)&App->ViewWin->WireColor[9],"Grid");
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  con->Style |= sCS_SIDELABEL;
  Grid->AddChild(con);

  con = new sControl;
  con->EditURGBA(0x113,(sInt *)&App->ViewWin->WireColor[10],"Background");
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  con->Style |= sCS_SIDELABEL;
  Grid->AddChild(con);

  con = new sControl;
  con->EditURGBA(0x113,(sInt *)&App->ViewWin->WireColor[12],"Portal");
  con->LayoutInfo.Init(0,line,16,line+1); line++;
  con->Style |= sCS_SIDELABEL;
  Grid->AddChild(con);

  con = new sControl;
  con->Button("Ok",0x100);
  con->LayoutInfo.Init(12,line,16,line+1); line++;
  Grid->AddChild(con);
  Grid->SetGrid(16,line,20,sPainter->GetHeight(sGui->PropFont)+8);

  mask = sGui->GetStyle();
  GSTitle = (mask & sGS_MAXTITLE) ? 1 : 0;
  GSFonts = (mask & sGS_SMALLFONTS) ? 1 : 0;
  GSScroll = (mask & sGS_SCROLLBARS) ? 1 : 0;
  GSSkin05 = (mask & sGS_SKIN05) ? 1 : 0;

  Color[0] = sGui->Palette[sGC_BACK];
  Color[1] = sGui->Palette[sGC_BUTTON];
  Color[2] = sGui->Palette[sGC_SELBACK];
  Color[3] = sGui->Palette[sGC_TEXT];
  Flags |= sGWF_SETSIZE;

  MaxLines = line;
}

WinEditPara::~WinEditPara()
{
}

void WinEditPara::OnPaint()
{
  ClearBack(sGC_BUTTON);
}

void WinEditPara::OnLayout()
{
  Grid->Position = Client;
  Grid->Position.Extend(-4);
}

void WinEditPara::Tag()
{
  sGuiWindow::Tag();
}

void WinEditPara::OnCalcSize()
{
  Grid->OnCalcSize();
  SizeX = Grid->SizeX+8;
  SizeY = Grid->SizeY+8;
}

sBool WinEditPara::OnCommand(sU32 cmd)
{
  switch(cmd)
  {
  case 0x100:   // ok
    App->SaveConfig();
    App->PagelistWin->UpdateList();
    KillMe();
    return sTRUE;
  case 0x101:   // change color
    sGui->Palette[sGC_BACK]    = Color[0];
    sGui->Palette[sGC_TEXT]    = Color[3];
    sGui->Palette[sGC_DRAW]    = Color[3];
    sGui->Palette[sGC_HIGH]    = ((Color[0]&0xfcfcfc)>>2)+0xffc0c0c0;
    sGui->Palette[sGC_LOW]     = ((Color[0]&0xfcfcfc)>>2)+0xff404040;
    sGui->Palette[sGC_HIGH2]   = 0xffffffff;
    sGui->Palette[sGC_LOW2]    = 0xff000000;
    sGui->Palette[sGC_BUTTON]  = Color[1];
    sGui->Palette[sGC_BARBACK] = ((Color[0]&0xfcfcfc)>>2)+0xffc0c0c0;
    sGui->Palette[sGC_SELECT]  = ((Color[0]&0xfefefe)>>1)+0xff808080;
    sGui->Palette[sGC_SELBACK] = Color[2];
    return sTRUE;
  case 0x110:   // change style
    if(GSFonts)
      sGui->SetStyle(sGui->GetStyle() | sGS_SMALLFONTS);
    else
      sGui->SetStyle(sGui->GetStyle() & ~sGS_SMALLFONTS);
    Grid->SetGrid(16,MaxLines,20,sPainter->GetHeight(sGui->PropFont)+8);
    return sTRUE;
  case 0x111:   // change style
    if(GSTitle)
      sGui->SetStyle(sGui->GetStyle() | sGS_MAXTITLE);
    else
      sGui->SetStyle(sGui->GetStyle() & ~sGS_MAXTITLE);
    return sTRUE;
  case 0x112:   // change style
    if(GSScroll)
      sGui->SetStyle(sGui->GetStyle() | sGS_SCROLLBARS);
    else
      sGui->SetStyle(sGui->GetStyle() & ~sGS_SCROLLBARS);
    return sTRUE;
  case 0x113:   // all other controls
    return sTRUE;
  case 0x114:   // texture memory
    KMemoryManager->SetBudget(KC_BITMAP,TextureMem<<20);
    KMemoryManager->SetBudget(KC_MESH,MeshMem<<20);
    return sTRUE;
  case 0x115:
    if(GSSkin05)
    {
      sGui->SetStyle(sGui->GetStyle() | sGS_SKIN05);
    }
    else
    {
      sGui->SetStyle(sGui->GetStyle() & ~sGS_SKIN05);
      OnCommand(0x101);
    }
    return sTRUE;
  }
  return sFALSE;
}

/****************************************************************************/
/***                                                                      ***/
/***   Demo Parameter                                                     ***/
/***                                                                      ***/
/****************************************************************************/

#define CMD_DEMOPARA_OK             0x1001
#define CMD_DEMOPARA_RELOAD         0x1002
#define CMD_DEMOPARA_RESTART        0x1003
#define CMD_DEMOPARA_SONGLENGTH     0x1004
#define CMD_DEMOPARA_GAMMA          0x1005

WinDemoPara::WinDemoPara()
{
  sControl *con;
  sInt max;

  App = 0;
  Grid = new sGridFrame;
  AddChild(Grid);
  AddTitle("Demo Parameters",1);
  Flags |= sGWF_TOPMOST;
  Flags |= sGWF_SETSIZE;
  max = 12;
  Grid->SetGrid(16,max,20,sPainter->GetHeight(sGui->PropFont)+8);

  con = new sControl;
  con->Button("Ok",CMD_DEMOPARA_OK);
  con->LayoutInfo.Init(12,max-1,16,max);
  Grid->AddChild(con);

  con = new sControl;
  con->Button("Reload Song",CMD_DEMOPARA_RELOAD);
  con->LayoutInfo.Init(0,max-1,4,max);
  Grid->AddChild(con);

  con = new sControl;
  con->Button("Restart Song",CMD_DEMOPARA_RESTART);
  con->LayoutInfo.Init(4,max-1,8,max);
  Grid->AddChild(con);
}

void WinDemoPara::SetApp(WerkkzeugApp *app)
{
  sControl *con;
  WerkDoc *doc;
  sInt Line;

  App = app;
  doc = app->Doc;
  Line = 0;

  SongLengthBeat =  App->Doc->SoundEnd>>16;
  SongLengthFine = (App->Doc->SoundEnd>>8)&0xff;

  Grid->AddLabel("Demo",0,4,Line);
  con = new sControl;
  con->EditString(0,doc->DemoName,0,sizeof(doc->DemoName));
  con->LayoutInfo.Init(4,Line,16,Line+1);  Grid->AddChild(con); 
  Line++;

  Grid->AddLabel("Song",0,4,Line);
  con = new sControl;
  con->EditString(0,doc->SongName,0,sizeof(doc->SongName));
  con->LayoutInfo.Init(4,Line,16,Line+1);  Grid->AddChild(con); 
  Line++;

  Grid->AddLabel("Samples",0,4,Line);
  con = new sControl;
  con->EditString(0,doc->SampleName,0,sizeof(doc->SampleName));
  con->LayoutInfo.Init(4,Line,16,Line+1);  Grid->AddChild(con); 
  Line++;

  Grid->AddLabel("BPM",0,4,Line);
  con = new sControl;
  con->EditFloat(0,&doc->SoundBPM,0);
  con->InitNum(60,280,0.01f,120);
  con->LayoutInfo.Init(4,Line,12,Line+1);  Grid->AddChild(con); 

  con = new sControl;
  con->EditChoice(0,&doc->BuzzTiming,0,"Correct|Buzz");
  con->LayoutInfo.Init(12,Line,16,Line+1);  Grid->AddChild(con); 
  Line++;

  Grid->AddLabel("Sample Offset",0,4,Line);
  con = new sControl;
  con->EditInt(0,&doc->SoundOffset,0);
  con->InitNum(-0x10000,0x10000,0.25,0);
  con->LayoutInfo.Init(4,Line,16,Line+1);  Grid->AddChild(con); 
  Line++;

  Grid->AddLabel("Song Length",0,4,Line);
  con = new sControl;
  con->EditInt(CMD_DEMOPARA_SONGLENGTH,&SongLengthBeat,0);
  con->InitNum(4,4096,0.25,0);
  con->LayoutInfo.Init(4,Line,10,Line+1);  Grid->AddChild(con); 
  con = new sControl;
  con->EditInt(CMD_DEMOPARA_SONGLENGTH,&SongLengthFine,0);
  con->InitNum(0,255,0.25,0);
  con->LayoutInfo.Init(10,Line,16,Line+1);  Grid->AddChild(con); 
  Line++;

  Grid->AddLabel("FR-Code",0,4,Line);
  con = new sControl;
  con->EditInt(0,&doc->FrCode,0);
  con->InitNum(1,99,0.125,0);
  con->LayoutInfo.Init(4,Line,16,Line+1);  Grid->AddChild(con); 
  Line++;

  Grid->AddLabel("Border",0,4,Line);
  con = new sControl;
  con->EditFloat(0,&doc->Aspect,0);
  con->InitNum(0.25,4.00,0.01f,4.0f/3.0f);
  con->LayoutInfo.Init(4,Line,16,Line+1);  Grid->AddChild(con); 
  Line++;

  Grid->AddLabel("Screen",0,4,Line);
  con = new sControl;
  con->EditInt(0,&doc->ResX,0);
  con->InitNum(-0x10000,0x10000,0.25,0);
  con->Zones = 2;
  con->Style |= sCS_ZONES;
  con->LayoutInfo.Init(4,Line,16,Line+1);  Grid->AddChild(con); 
  Line++;

  Grid->AddLabel("Global Gamma",0,4,Line);
  con = new sControl;
  con->EditFloat(CMD_DEMOPARA_GAMMA,&doc->GlobalGamma,0);
  con->InitNum(0.01f,4.00,0.01f,1.0f);
  con->LayoutInfo.Init(4,Line,16,Line+1);  Grid->AddChild(con); 
  Line++;

  Grid->AddLabel("Player Type",0,4,Line);
  con = new sControl;
  con->EditChoice(0,&doc->PlayerType,0,"Demo|Intro|KKrieger");
  con->LayoutInfo.Init(4,Line,16,Line+1);  Grid->AddChild(con); 
  Line++;

/*
  con = new sControl;
  con->EditCycle(0x110,&GSFonts,0,"Large Fonts|Small Fonts");
  con->LayoutInfo.Init(8,0,16,1);
  Grid->AddChild(con);

  con = new sControl;
  con->EditCycle(0x111,&GSTitle,0,"No Title Bar|Title Bar");
  con->LayoutInfo.Init(8,1,16,2);
  Grid->AddChild(con);

  con = new sControl;
  con->EditCycle(0x112,&GSScroll,0,"Show Scrollbars|Hide Scrollbars");
  con->LayoutInfo.Init(8,2,16,3);
  Grid->AddChild(con);
  */
}

WinDemoPara::~WinDemoPara()
{
}

void WinDemoPara::OnPaint()
{
  ClearBack(sGC_BUTTON);
}

void WinDemoPara::OnLayout()
{
  Grid->Position = Client;
  Grid->Position.Extend(-4);
}

void WinDemoPara::Tag()
{
  sGuiWindow::Tag();
}

void WinDemoPara::OnCalcSize()
{
  Grid->OnCalcSize();
  SizeX = Grid->SizeX+8;
  SizeY = Grid->SizeY+8;
}

sBool WinDemoPara::OnCommand(sU32 cmd)
{
  switch(cmd)
  {
  case CMD_DEMOPARA_OK:
    KillMe();
    return sTRUE;

  case CMD_DEMOPARA_RELOAD:
    App->MusicLoad();
    App->MusicSeek(0);
    App->MusicStart(1);
    return sTRUE;

  case CMD_DEMOPARA_RESTART:
    App->MusicSeek(0);
    App->MusicStart(1);
    return sTRUE;

  case CMD_DEMOPARA_SONGLENGTH:
    App->Doc->SoundEnd = (SongLengthBeat<<16)|(SongLengthFine<<8);
    return sTRUE;

  case CMD_DEMOPARA_GAMMA:
    sSystem->SetGamma(App->Doc->GlobalGamma);
//    sDPrintF("%f\n",App->Doc->GlobalGamma);
    return sTRUE;
  }
  return sFALSE;
}

/****************************************************************************/

WinTimeBorder::WinTimeBorder()
{
  Height = 21;
  App = 0;
}

void WinTimeBorder::Tag()
{
  sBroker->Need(App);
  sGuiWindow::Tag();
}

void WinTimeBorder::OnCalcSize()
{
  SizeY = Height;
  SizeX = 0;
}

void WinTimeBorder::OnSubBorder()
{
  Parent->Client.y1 -= Height;
}

void WinTimeBorder::OnPaint()
{
  sRect r;
  sU32 col;
  sInt i;
  sInt x,y,x0,x1;
  sInt l0,l1,se,now;

  col = sGui->Palette[sGC_DRAW];
  r = Client;
  r.y0 = r.y1 - Height;
  sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BUTTON]);
  r.Extend(-2);
  sGui->RectHL(r,1,col,col);

  if(App)
  {
    l0 = App->MusicLoop0;
    l1 = App->MusicLoop1;
    se = App->Doc->SoundEnd;
    App->MusicNow(now,i);
    if(l1>l0)
    {
      x0 = r.x0+sMulDiv(l0,r.XSize(),se);
      x1 = r.x0+sMulDiv(l1,r.XSize(),se);
      sPainter->Paint(sGui->FlatMat,x0,r.y0,x1-x0,r.y1-r.y0,sGui->Palette[App->MusicLoop?sGC_SELBACK:sGC_BACK]);
    }
    for(i=0;i<se;i+=0x40000)
    {
      y = (i&0xf0000)?(r.y0+r.y1*2)/3:r.y0+4;
      x = r.x0+sMulDiv(i,r.XSize(),se);
      sPainter->Paint(sGui->FlatMat,x,y,1,r.y1-y,col);
    }
    x = r.x0+sMulDiv(now,r.XSize(),se);
    sPainter->Paint(sGui->FlatMat,x-1,r.y0,2,r.y1-r.y0,0xffff0000);
  }

  Flags &= ~sGWF_PASSRMB;
}

void WinTimeBorder::OnDrag(sDragData &dd)
{
  sRect r;
  sInt x;
  sInt se;

  sVERIFY(App);
  se = App->Doc->SoundEnd;

  r = Client;
  r.y0 = r.y1 - Height;
  sPainter->Paint(sGui->FlatMat,r,sGui->Palette[sGC_BUTTON]);
  r.Extend(-2);
  x = sMulDiv(dd.MouseX-r.x0,se,r.XSize());

  switch(dd.Mode)
  {
  case sDD_START:
    if(dd.Buttons&1)
    {
      DragStart = x;
      DragMode = 1;
      if(App->MusicIsPlaying()) App->MusicStart(2);
    }
    else if(dd.Buttons&2)
    {
      DragStart = x;
      DragMode = 2;
    }
    break;
  case sDD_DRAG:
    if(DragMode == 1)
    {
      if(x<0) x = 0;
      App->MusicSeek(x);
    }
    if(DragMode == 2)
    {
      App->MusicLoop0 = (DragStart+0x20000)/0x40000*0x40000;
      App->MusicLoop1 = (x        +0x20000)/0x40000*0x40000;
      if(App->MusicLoop0 > App->MusicLoop1)
        sSwap(App->MusicLoop0,App->MusicLoop1);
      App->MusicLoopTaken = 0;
    }
    break;
  case sDD_STOP:
    DragMode = 0;
    if(dd.Buttons&1)
    {
      if(App->MusicIsPlaying()) App->MusicStart(1);
    }
    break;
  }
}

/****************************************************************************/
/****************************************************************************/

WinStatistics::WinStatistics(class WerkkzeugApp *app)
{
  App = app;
}

void WinStatistics::Print()
{
  sInt i,j,k,max;
  WerkDoc *doc;
  WerkPage *page;
  KKriegerGame *game;

  doc = App->Doc;
  game = &App->ViewWin->Game;

  // pages and ops

  PrintGroup("Pages & Ops");
  max = doc->Pages->GetCount();
  j = 0;
  for(i=0;i<max;i++)
    j += doc->Pages->Get(i)->Orig->GetCount();
  PrintLine("pages:        %d (%d ops)",max,j);

  j = 0;
  k = 0;
  for(i=0;i<max;i++)
  {
    page = doc->Pages->Get(i);
    if(page->Scratch)
    {
      j = doc->Pages->Get(i)->Scratch->GetCount();
      k++;
    }
  }
  PrintLine("spares:       %d (%d ops)",k,j);

  page = App->PageWin->Page;
  if(page)
  {
    PrintLine("current page: %d ops",page->Orig->GetCount());
  }

  for(i=0;WerkClasses[i].Name;i++);
  PrintLine("op classes:   %d",i);

  // memory

  PrintGroup("Memory");

  sBroker->Stats(i,j);
  PrintLine("objects:      %d",i);
  i = sSystem->MemoryUsed();
  PrintLine("memory:       %d Bytes (%d MB)",i,i>>20);
  PrintLine("in meshes:    %04dMB / %04dMB",KMemoryManager->GetSize(KC_MESH  )>>20,KMemoryManager->GetBudget(KC_MESH  )>>20);
  PrintLine("in bitmaps:   %04dMB / %04dMB",KMemoryManager->GetSize(KC_BITMAP)>>20,KMemoryManager->GetBudget(KC_BITMAP)>>20);

  // 3d engine

  PrintGroup("3d Engine");

  j=0;
  for(i=0;i<MAX_TEXTURE;i++)
    if(sSystem->Textures[i].Flags)
      j++;
  PrintLine("textures:     %d (%d max)",j,MAX_TEXTURE);

  j=0;
  for(i=0;i<MAX_GEOBUFFER;i++)
    if(sSystem->GeoBuffer[i].Type)
      j++;
  PrintLine("geobuffers:   %d (%d max)",j,MAX_GEOBUFFER);

  j=0;
  for(i=0;i<MAX_GEOHANDLE;i++)
    if(sSystem->GeoHandle[i].Mode)
      j++;
  PrintLine("geohandles:   %d (%d max)",j,MAX_GEOHANDLE);
  //PrintLine("materials:    %d (%d max)",sMaterial11->MtrlCount(),MAX_MATERIALS);

  // game

  PrintGroup("Game & Collision");

  PrintLine("add cells:    %d",game->CellAdd.Count);
  PrintLine("sub cells:    %d",game->CellSub.Count);
  PrintLine("zone cells:   %d",game->CellZone.Count);
  PrintLine("dynamic cells:%d (%d max)",game->DCellUsed,KKRIEGER_MAXDCELL);
  PrintLine("particles:    %d (%d max)",game->PartUsed,KKRIEGER_MAXPARTICLE);
  PrintLine("constraints:  %d (%d max)",game->ConsUsed,KKRIEGER_MAXCONSTRAINT);
  PrintLine("max planes:   %d",game->MaxPlanes);

  PrintGroup("Current Add-Cell");
  PrintLine("Adds Connect: %d",game->PlayerCell?game->PlayerCell->Adds.Count:0);
  PrintLine("Adds Inside:  %d",game->PlayerAdds);
  PrintLine("Subs:         %d",game->PlayerCell?game->PlayerCell->Subs.Count:0);
  PrintLine("Zones:        %d",game->PlayerCell?game->PlayerCell->Zones.Count:0);
  PrintLine("Planes:       %d",game->PlayerCell?game->PlayerCell->PlaneCount:0);
  PrintGroup("Collision Performance");
  PrintLine("Ticks per Frame:      %d",game->TickPerFrame);
  PrintLine("FindFirstIntersect(): %d",game->CallFindFirstIntersect);
  PrintLine("FindSubIntersect():   %d",game->CallFindSubIntersect);
  PrintLine("MoveParticle():       %d",game->CallMoveParticle);
  PrintLine("CallOutsideMask():    %d",game->CallOutsideMask);
  PrintLine("CallIntersectOut():   %d",game->CallIntersectOut);
  PrintLine("CallIntersectIn():    %d",game->CallIntersectIn);
}

/****************************************************************************/

WinGameInfo::WinGameInfo(class WerkkzeugApp *app)
{
  App = app;
}

void WinGameInfo::Print()
{
  sInt i,j;

  sChar buffer[64];
  sChar *s;
  KKriegerGame *game;
  static const sChar *names[16] = 
  {
    "Life",
    "Armor",
    "Alpha",
    "Beta",
    "Ammo 0",
    "Ammo 1",
    "Ammo 2",
    "Ammo 3",
    "Weapon 0",
    "Weapon 1",
    "Weapon 2",
    "Weapon 3",
    "Weapon 4",
    "Weapon 5",
    "Weapon 6",
    "Weapon 7",
  };

  game = &App->ViewWin->Game;

  PrintGroup("Player");
  PrintLine("Position      :%06.2f %06.2f %06.2f",game->PlayerPos.x,game->PlayerPos.y,game->PlayerPos.z);
  PrintLine("Subs in Cell  :%d",game->PlayerCell ? game->PlayerCell->Subs.Count:0);
  PrintLine("Zones in Cell :%d",game->PlayerCell ? game->PlayerCell->Zones.Count:0);
  PrintLine("Weapon        :%d",game->Player.CurrentWeapon);
  PrintGroup("Switches");
  for(i=0;i<16;i++)
  {
    s = buffer;
    for(j=0;j<16;j++)
    {
      *s++ = game->Switches[i*16+j]+'0';
      if(j==3 || j==7 || j==11)
        *s++ = '-';
    }
    *s++ = 0;
    if(i<8)
      PrintLine("%02x (%3d):%s     %-8s : %04d (%04d max)",i*16,i*16,buffer,names[i],(&game->Player.Life)[i],(&game->Player.LifeMax)[i]);
    else
      PrintLine("%02x (%3d):%s     %-8s : %s",i*16,i*16,buffer,names[i],(&game->Player.Life)[i]?"yes":"no");
  }
}

void WinGameInfo::OnDrag(sDragData &dd)
{
  sReportWindow::OnDrag(dd);
  sInt x,y,w;
  KKriegerGame *game;

  game = &App->ViewWin->Game;

  if(dd.Mode==sDD_START)
  {
    w = sPainter->GetWidth(sGui->FixedFont,"0");
    y = (dd.MouseY-Client.y0-5)/sPainter->GetHeight(sGui->FixedFont)-6;
    if(y<0 || y>15) return;

    x = (dd.MouseX-Client.x0-5)/w-9;
    if(x<0) return;
    if(x>=4)
    {
      if(x==4) return;
      x--;
      if(x>=8)
      {
        if(x==8) return;
        x--;
        if(x>=12)
        {
          if(x==12) return;
          x--;
        }
      }
    }
    if(x>15) return;

    
    game->Switches[x+y*16]++;
    if(dd.Buttons!=2 && game->Switches[x+y*16]>=2)
      game->Switches[x+y*16] = 0;
  }
}

/****************************************************************************/

WinFindResults::WinFindResults(WerkkzeugApp *app)
{
  sToolBorder *tb;
  sListHeader *lh;
  sInt numwidth;

  App = app;
  Results.Init();
  sCopyString(TitleBuffer,"Find results",sizeof(TitleBuffer));

  tb = new sToolBorder;
  Title = new sControl;
  Title->Label(TitleBuffer);
  tb->MenuStyle = 1;
  tb->AddChild(Title);
  tb->AddButton("Close",1);
  AddBorder(tb);

  numwidth = sPainter->GetWidth(sGui->PropFont,"1");

  ResultsList = new sListControl;
  ResultsList->LeftCmd = 2;
  ResultsList->TabStops[0] = 0;
  ResultsList->TabStops[1] = 5 * numwidth;
  ResultsList->TabStops[2] = ResultsList->TabStops[1] + 12 * sPainter->GetWidth(sGui->PropFont,"w");
  ResultsList->TabStops[3] = ResultsList->TabStops[2] + 4 * numwidth;
  ResultsList->TabStops[4] = ResultsList->TabStops[3] + 4 * numwidth;
  ResultsList->AddScrolling(0,1);

  lh = new sListHeader(ResultsList);
  lh->AddTab("#");
  lh->AddTab("Page");
  lh->AddTab("x");
  lh->AddTab("y");
  lh->AddTab("Notes");
  ResultsList->AddBorder(lh);

  AddChild(ResultsList);
}

WinFindResults::~WinFindResults()
{
  Results.Exit();
}

void WinFindResults::Tag()
{
  sInt i;

  sGuiWindow::Tag();

  for(i=0;i<Results.Count;i++)
    sBroker->Need(Results[i].Op);
}

void WinFindResults::OnLayout()
{
  ResultsList->Position = Client;
}

void WinFindResults::OnFrame()
{
  sInt i,j;
  WerkOp *op;

  i = 0;
  while(i < Results.Count)
  {
    op = Results[i].Op;

    if(!op)
    {
      sSPrintF(Results[i].TextBuf,sizeof(Results[i].TextBuf),
        "%3d|<global>|0|0|%s",i+1,Results[i].ExtraText);
      ResultsList->SetName(i,Results[i].TextBuf);
      i++;
    }
    else if(op->Page)
    {
      sSPrintF(Results[i].TextBuf,sizeof(Results[i].TextBuf),"%3d|%s|%d|%d|%s",
        i+1,op->Page->Name,op->PosX,op->PosY,Results[i].ExtraText);
      ResultsList->SetName(i,Results[i].TextBuf);
      i++;
    }
    else // op was deleted, delete from internal lists too
    {
      Results.Count--;
      for(j=i;j<Results.Count;j++)
        Results[j] = Results[j+1];

      ResultsList->Rem(i);
    }
  }
}

sBool WinFindResults::OnCommand(sU32 cmd)
{
  sInt select;

  switch(cmd)
  {
  case 1: // close
    App->FindResults(sFALSE);
    return sTRUE;

  case 2: // select
    select = ResultsList->GetSelect();
    if(select>=0 && select<Results.Count)
    {
      WerkOp *op = Results[select].Op;
      if(op)
      {
        App->PageWin->GotoOp(op);
        App->ParaWin->SetOp(op);
      }
    }
    return sTRUE;

  default:
    return sFALSE;
  }
}

void WinFindResults::Clear()
{
  Results.Count = 0;
}

void WinFindResults::Add(WerkOp *op,sChar *text)
{
  FindResult *res = Results.Add();

  res->Op = op;
  if(text)
    sCopyString(res->ExtraText,text,sizeof(res->ExtraText));
  else
    res->ExtraText[0] = 0;
}

void WinFindResults::Update()
{
  sInt i;

  ResultsList->ClearList();
  for(i=0;i<Results.Count;i++)
    ResultsList->Add("",0x100+i);
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   document                                                           ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

WerkSpline::WerkSpline()
{
  sInt i;

  Spline.Count = 0;
  Spline.Interpolation = 0;
  for(i=0;i<WS_MAXCHANNEL;i++)
  {
    Channel[i].Keys.Init(0);
    Channel[i].Selected = 0;
    Spline.Channel[i].Keys = Channel[i].Keys.Array;
    Spline.Channel[i].KeyCount = 0;
  }
  Name[0]=0;
  Kind = 0;
  Index = -1;
}

WerkSpline::~WerkSpline()
{
  Clear();
}

void WerkSpline::Tag()
{
  sObject::Tag();
}

void WerkSpline::Clear()
{
  sInt i;
  for(i=0;i<WS_MAXCHANNEL;i++)
    Channel[i].Keys.Exit();
  Index = -1;
}

void WerkSpline::Copy(sObject *o)
{
  WerkSpline *so;
  sInt i;

  so = (WerkSpline *)o;
  sVERIFY(so->GetClass()==sCID_WERKSPLINE);

  Init(so->Spline.Count,so->Name,so->Kind,0,so->Spline.Interpolation);

  for(i=0;i<Spline.Count;i++)
  {
    Channel[i].Selected = 0;
    Channel[i].Keys.Copy(so->Channel[i].Keys);
    Spline.Channel[i].Keys = Channel[i].Keys.Array;
    Spline.Channel[i].KeyCount = Channel[i].Keys.Count;
  }
  Sort();
}

void WerkSpline::Init(sInt count,sChar *name,sInt kind,sBool addkeys,sInt inter)
{
  sInt i;

  sCopyString(Name,name,KK_NAME);
  Kind = kind;
  Spline.Count = count;
  Spline.Interpolation = inter;
  for(i=0;i<count;i++)
  {
    if(addkeys)
    {
      AddKey(i,0,0)->Select = 0;
      AddKey(i,1,0)->Select = 0;
    }
  }
  Sort();
}

void WerkSpline::InitDef(sInt count,sChar *name,sInt kind,const sF32 *def,sInt inter)
{
  sInt i;

  sCopyString(Name,name,KK_NAME);
  Kind = kind;
  Spline.Count = count;
  Spline.Interpolation = inter;
  for(i=0;i<count;i++)
  {
    AddKey(i,0,*def)->Select = 0;
    AddKey(i,1,*def)->Select = 0;
    def++;
  }
  Sort();
}

void WerkSpline::Select(sInt value)
{
  for(sInt i=0;i<WS_MAXCHANNEL;i++)
    Channel[i].Selected = value;
}


/****************************************************************************/

KSplineKey *WerkSpline::AddKey(sInt channel,sF32 time,sF32 value)
{
  KSplineKey *key;

  key = Channel[channel].Keys.Add();
  key->Time = time;
  key->OldTime = time;
  key->Value = value;
  key->Select = 1;
  Spline.Channel[channel].Keys = Channel[channel].Keys.Array;
  Spline.Channel[channel].KeyCount = Channel[channel].Keys.Count;

  return key;
}

void WerkSpline::RemSelection()
{
  sInt i,c;

  for(c=0;c<Spline.Count;c++)
  {
    if(Channel[c].Selected)
    {
      for(i=0;i<Channel[c].Keys.Count;)
      {
        if(Channel[c].Keys.Array[i].Select && Channel[c].Keys.Count>2)
        {
          Channel[c].Keys.Array[i] = Channel[c].Keys.Array[--Channel[c].Keys.Count];
        }
        else
        {
          i++;
        }
      }
      Spline.Channel[c].Keys = Channel[c].Keys.Array;
      Spline.Channel[c].KeyCount = Channel[c].Keys.Count;
    }
  }
}

void WerkSpline::MoveSelection(sF32 time,sF32 value)
{
  sInt i,max,c;

  for(c=0;c<Spline.Count;c++)
  {
    if(Channel[c].Selected)
    {
      max = Channel[c].Keys.Count;
      for(i=0;i<max;i++)
      {
        if(Channel[c].Keys.Array[i].Select)
        {
          Channel[c].Keys.Array[i].Time += time;
          Channel[c].Keys.Array[i].Value += value;
        }
      }
    }
  }
}

sBool WerkSpline::Sort()
{
  sInt i,j,max,c;
  sBool swapped = 0;

  for(c=0;c<Spline.Count;c++)
  {
    max = Channel[c].Keys.Count;
    for(i=0;i<max-1;i++)
    {
      for(j=i;j<max-1;j++)
      {
        if(Channel[c].Keys.Array[j].Time > Channel[c].Keys.Array[j+1].Time)
        {
          sSwap(Channel[c].Keys.Array[j],Channel[c].Keys.Array[j+1]);
          swapped = 1;
        }
      }
    }
  }

  return swapped;
}

/****************************************************************************/

WerkOpAnim::WerkOpAnim()
{
  PosX = 0;
  PosY = 0;
  Width = 3;
  Bytecode = KA_NOP;
  Parameter = 0;
  Select = 0;
  InputCount = 0;
  OutputCount = 0;
  Error = 0;
  ConstVector.Init(0,0,0,0);
}

WerkOpAnim::~WerkOpAnim()
{
}

void WerkOpAnim::Tag()
{
  sInt i;

  sObject::Tag();
  for(i=0;i<InputCount;i++)
    Inputs[i]->Tag();
}

void WerkOpAnim::Init(sInt code,sInt para)
{
  sInt i;

  Bytecode = code;
  Parameter = para;
  Select = 0;
  Error = 0;
  AutoOp = 0;
  OutputCount = 0;
  InputCount = 0;
  MaxMask = 15;
  for(i=0;i<WOA_MAXINPUT;i++)
    Inputs[i] = 0;

  if(code>0x80)
    code&=0xf0;
  Info = 0;
  for(i=0;AnimOpCmds[i].Name;i++)
    if(AnimOpCmds[i].Code==code)
      Info = &AnimOpCmds[i];
  if(Info->Flags & AOIF_STOREPARA)
    MaxMask = Bytecode&15;
  UnionKind = Info->UnionKind;
}

void WerkOpAnim::Copy(sObject *o)
{
  WerkOpAnim *oa;

  sVERIFY(o->GetClass()==sCID_WERKOPANIM);
  oa = (WerkOpAnim *)o;

  PosX = oa->PosX;
  PosY = oa->PosY;
  Width = oa->Width;
  Bytecode = oa->Bytecode;
  Parameter = oa->Parameter;
  AutoOp = oa->AutoOp;
  Select = oa->Select;
  MaxMask = oa->MaxMask;
  UnionKind = oa->UnionKind;
  sCopyMem(SplineName,oa->SplineName,KK_NAME);
  Info = oa->Info;

  // Error
  // UsedInCode
  // OutputCount
  // InputCount
}


/****************************************************************************/
/*
WerkAnim::WerkAnim()
{
  sSetMem(&Anim,0,sizeof(Anim));
  sSetMem(Name,0,sizeof(Name));
  Target = 0;
}

WerkAnim::~WerkAnim()
{
}

void WerkAnim::Tag()
{
  sObject::Tag();
  sBroker->Need(Target);
}
*/
/****************************************************************************/
/****************************************************************************/

WerkOp::WerkOp()
{
  PosX = 0;
  PosY = 0;
  Width = 3;
  Page = 0;
  sSetMem(Name,0,sizeof(Name));
  sSetMem(LinkName,0,sizeof(LinkName));
  sSetMem(SplineName,0,sizeof(SplineName));
//  sSetMem(Inputs,0,sizeof(Inputs));
//  sSetMem(Links,0,sizeof(Links));
//  Anims = new sList<WerkAnim>;
  AnimOps = new sList<WerkOpAnim>;

  Error = 0;
  Selected = 0;
  Class = 0;
  SetDefaults = 0;
  SaveId = 0;
  Bypass = 0;
  Hide = 0;
  BlinkTime = 0;
  CycleFind = 0;

  sSetMem(&Op,0,sizeof(KOp));
  Op.Init(0);
  Op.Werk = this;
}

WerkOp::WerkOp(WerkkzeugApp *app,sInt id,sInt x,sInt y,sInt w)
{
  Page = 0;

  sSetMem(Name,0,sizeof(Name));
  sSetMem(LinkName,0,sizeof(LinkName));
  sSetMem(SplineName,0,sizeof(SplineName));

  AnimOps = new sList<WerkOpAnim>;

  Error = 0;
  Selected = 0;
  Class = 0;
  SetDefaults = 0;
  SaveId = 0;
  Bypass = 0;
  Hide = 0;
  BlinkTime = 0;
  CycleFind = 0;

  sSetMem(&Op,0,sizeof(KOp));
  Op.Werk = this;
  Init(app,id,x,y,w,sTRUE);
}

WerkOp::~WerkOp()
{
  KEvent *event,*next;

  event = Op.FirstEvent;
  while(event)
  {
    next = event->NextOp;
    event->Op = 0;
    event->NextOp = 0;
    event = next;
  }
  if(Op.SceneMemLink)
  {
    Op.SceneMemLink->DeleteChain();
    Op.SceneMemLink = (KInstanceMem*) 0xffffffff;
  }
  Op.FirstEvent = 0;

  Op.Exit();
  if(Op.Cache)
    Op.Cache->Release();
}

void WerkOp::Tag()
{
  sInt i;
  for(i=0;i<Op.GetInputCount();i++)
    if(Op.GetInput(i))
      sBroker->Need(Op.GetInput(i)->Werk);
  for(i=0;i<KK_MAXLINK;i++)
    if(Op.GetLink(i))
      sBroker->Need(Op.GetLink(i)->Werk);
  //sBroker->Need(Anims);
  sBroker->Need(Page);
  sBroker->Need(AnimOps);
  sObject::Tag();
}

void WerkOp::Copy(sObject *o)
{
  WerkOp *oo;
  WerkOpAnim *oa,*oan;
  sInt i;

  sVERIFY(o->GetClass()==sCID_WERKOP);
  oo = (WerkOp *)o;

  Init(0,oo->Class->Id,oo->PosX,oo->PosY,oo->Width);

  sCopyMem(Op.GetEditPtr(0),oo->Op.GetEditPtr(0),Op.GetDataWords()*4);
  for(i=0;i<Op.GetStringMax();i++)
    Op.SetString(i,oo->Op.GetString(i));
  Op.Cache = 0;                   // be carefull!!!

  Page = oo->Page;
  sCopyMem(Name,oo->Name,sizeof(Name));
  sCopyMem(LinkName,oo->LinkName,sizeof(LinkName));
  sCopyMem(SplineName,oo->SplineName,sizeof(SplineName));
//  Anims->Copy(oo->Anims);
  Class = oo->Class;
  Error = oo->Error;
  Hide = oo->Hide;
  Bypass = oo->Bypass;
  SetDefaults = 0;
  for(i=0;i<oo->AnimOps->GetCount();i++)
  {
    oa = oo->AnimOps->Get(i);
    oan = new WerkOpAnim;
    oan->Copy(oa);
    AnimOps->Add(oan);
  }

  i = oo->Op.GetBlobSize(); 
  if(i)
  {
    sU8 *blob;
    blob = new sU8[i];
    sCopyMem(blob,oo->Op.GetBlobData(),i);
    Op.SetBlob(blob,i);
  }

}

/****************************************************************************/

void WerkOp::Init(WerkkzeugApp *app,sInt id,sInt x,sInt y,sInt w,sBool inConstruct)
{
  sInt i;
  Class = FindWerkClass(id);
  PosX = x;
  PosY = y;
  Width = w;

  if(Class==0)
    Class = FindWerkClass(0x04);    // 0x04 = error

  if(!inConstruct)
    Op.Exit();

  Op.Init(Class->Convention);
  Op.Convention = Class->Convention;
  Op.InitHandler = Class->InitHandler;
  Op.ExecHandler = Class->ExecHandler;
  Op.Command = id;
  Op.Result = Class->Output;
  for(i=0;Class->Packing[i];i++)
    if(Class->Packing[i]>='A' && Class->Packing[i]<='Z')
      Op.ChangeMask|=1<<i;
  if(Class->EditHandler && app)
  {
    SetDefaults = 1;
    (*Class->EditHandler)(app,this);
    SetDefaults = 0;
  }
}


void WerkOp::AddOutput(WerkOp *oo)
{
  Op.AddOutput(&oo->Op);
  /*
  if(Op.OutputCount<KK_MAXOUTPUT)
  {
    Outputs[Op.OutputCount] = oo;
    Op.Output[Op.OutputCount] = &oo->Op;
    Op.OutputCount++;
  }
  else
  {
    Error = 1; 
  }
  */
}

void WerkOp::Change(sBool reconnect,sInt channel)
{
  Op.CopyEditToAnim();
  Op.Change(channel);
  sVERIFY(Page);
  Page->Change(reconnect);
}

static sU8 CodeBuffer[4096];
static sInt CodeCount;

sBool WerkOp::ConnectAnim(WerkDoc *doc)
{
  sInt i,j,k,max;
  WerkOpAnim *oa,*ob;
  sInt error;
  sU8 stored[KK_MAXVAR+64];
  sBool swapped;

// prepare

  sSetMem(stored,0,sizeof(stored));
  max = AnimOps->GetCount();          
  for(i=0;i<max;i++)
  {
    oa = AnimOps->Get(i);
    oa->OutputCount = 0;
    oa->InputCount = 0;
    oa->Error = 0;
  }

// sort 

  swapped = 1;
  for(i=0;i<max-1 && swapped;i++)
  {
    swapped = 0;
    for(j=0;j<max-i-1;j++)
    {
      oa = AnimOps->Get(j); // i > j
      ob = AnimOps->Get(j+1); // j < i
      if(oa->PosY > ob->PosY)
      {
        AnimOps->Swap(j,j+1);
        swapped = 1;
      }
    }
  }

// connect

  for(i=0;i<max;i++)
  {
    oa = AnimOps->Get(i);             // find inputs
    for(j=0;j<max;j++)
    {
      ob = AnimOps->Get(j);
      if(ob->PosY == oa->PosY-1 &&
         ob->PosX < oa->PosX+oa->Width &&
         ob->PosX+ob->Width > oa->PosX)
      {
        if(oa->InputCount<WOA_MAXINPUT)
        {
          oa->Inputs[oa->InputCount++] = ob;
          ob->OutputCount++;
        }
        else
        {
          oa->Error = 1;
        }
      }
    }

    for(j=0;j<oa->InputCount-1;j++)       // sort inputs left -> right
      for(k=j+1;k<oa->InputCount;k++)
        if(oa->Inputs[j]->PosX > oa->Inputs[k]->PosX)
          sSwap(oa->Inputs[j],oa->Inputs[k]);
  }

// check for errors

  error = 0;
  for(i=0;i<max;i++)
  {
    oa = AnimOps->Get(i);
    oa->UsedInCode = sFALSE;
    if(!oa->Info)
    {
      oa->Error = 1;
    }
    else
    {
      if(oa->Info->Inputs!=oa->InputCount)
      {
        if(!(oa->Info->Inputs==0 && oa->InputCount==1))
          oa->Error = 1;
      }
      if(oa->Info->Flags & (AOIF_STOREVAR|AOIF_STOREPARA))
      {
        if(oa->OutputCount!=0)
          oa->Error = 1;
      }
      else
      {
        if(oa->OutputCount==0)
          oa->Error = 1;
      }
      if(oa->Bytecode==KA_SPLINE)
      {
        if(doc->FindSpline(oa->SplineName)==0)
          oa->Error = 1;
      }
    }
    if(oa->Error)
      error = 1;
  }

// generate code

  CodeCount = 0;
  error = 0;
  for(i=0;i<max;i++)
  {
    oa = AnimOps->Get(i);
    if(oa->Info->Flags & (AOIF_STOREVAR|AOIF_STOREPARA) && oa->InputCount>0)
    {
      if(GenCodeAnim(oa,doc,stored))
      {
        oa->Error = 1;
        error = 1;
      }
    }
  }
  if(error)
  {
    CodeCount = 0;
    CodeBuffer[CodeCount++] = KA_END;
    for(j=0;j<max;j++)
      AnimOps->Get(j)->UsedInCode = sFALSE;
  }

  CodeBuffer[CodeCount++] = KA_END;
  sVERIFY(CodeCount<4096);
  if(sCmpMem(Op.GetAnimCode(),CodeBuffer,CodeCount)!=0)
  {
    Op.SetAnimCode(CodeBuffer,CodeCount);
    Change(0);
    return 1;
  }
  else
  {
    return 0;
  }
}

sBool WerkOp::GenCodeAnim(WerkOpAnim *oa,WerkDoc *doc,sU8 *stored)
{
  WerkSpline *spline;
  sInt i;
  sBool error;

  if(oa->Error)
    return sTRUE;

  oa->UsedInCode = sTRUE;
  error = 0;
  for(i=0;i<oa->InputCount;i++)
    error |= GenCodeAnim(oa->Inputs[i],doc,stored);

  if(oa->Info->Flags & AOIF_STOREVAR)
  {
    sVERIFY(oa->Parameter>=0 && oa->Parameter<KK_MAXVAR)
    if(stored[oa->Parameter] & oa->Bytecode & 15)
      error = 1;
    stored[oa->Parameter] |= oa->Bytecode & 15;
  }
  if(oa->Info->Flags & AOIF_STOREPARA)
  {
    sVERIFY(oa->Parameter>=0 && oa->Parameter<64)
    if(stored[KK_MAXVAR+oa->Parameter] & oa->Bytecode & 15)
      error = 1;
    stored[KK_MAXVAR+oa->Parameter] |= oa->Bytecode & 15;
  }
  if(oa->Info->Flags & AOIF_LOADVAR)
  {
    if(stored[oa->Parameter])
      error = 1;
  }

  CodeBuffer[CodeCount++] = oa->Bytecode;
  if(oa->Info->Flags & (AOIF_STOREVAR|AOIF_STOREPARA|AOIF_LOADVAR|AOIF_LOADPARA))
    CodeBuffer[CodeCount++] = oa->Parameter;

  switch(oa->UnionKind)
  {
  case AOU_VECTOR:
    sCopyMem(CodeBuffer+CodeCount,&oa->ConstVector,16);
    CodeCount+=16;
    break;
  case AOU_COLOR:
    sCopyMem(CodeBuffer+CodeCount,&oa->ConstColor,4);
    CodeCount+=4;
    break;
  case AOU_SCALAR:
    sCopyMem(CodeBuffer+CodeCount,&oa->ConstVector,4);
    CodeCount+=4;
    break;
  case AOU_NAME:
    spline = doc->FindSpline(oa->SplineName);
    i = 0;
    if(spline)
      i = spline->Index;

    *((sU16 *)(CodeBuffer+CodeCount)) = i;
    CodeCount+=2;
    break;
  case AOU_EASE:
    sCopyMem(CodeBuffer+CodeCount,&oa->ConstVector,8);
    CodeCount+=8;
    break;
  }

  if(oa->Info->Inputs==0 && oa->InputCount==1)
  {
    static sInt autocode[4] = { KA_MUL,KA_ADD,KA_SUB,KA_DIV };
    CodeBuffer[CodeCount++] = autocode[oa->AutoOp];
  }

  return error;
}

sInt WerkOp::GetResultClass()
{
  sInt result;

  result = KC_ANY;
  if(Op.CycleFlag==0)
  {
    Op.CycleFlag = 1;
    if(Class->Output!=KC_ANY)
      result = Class->Output;
    else if(GetInput(0))
      result = GetInput(0)->GetResultClass();
    else if(GetLink(0))
      result = GetLink(0)->GetResultClass();
    Op.CycleFlag = 0;
  }
  return result;
}


/****************************************************************************/
/****************************************************************************/

WerkPage::WerkPage()
{
  Ops = new sList<WerkOp>;
  Splines = new sList<WerkSpline>;
  Orig = Ops;
  Scratch = 0;
  Changed = sFALSE;
  Reconnect = sTRUE;
  Doc = 0;
  ScrollX = 0;
  ScrollY = 0;
  Name[0] = 0;

  Owner[0] = 0;
  OwnerCount = 0;
  NextOwner[0] = 0;
  FakeOwner = 0;
  MergeError = 0;
  TreeLevel = 0;
  TreeButton = ' ';
}

WerkPage::~WerkPage()
{
}

void WerkPage::Tag()
{
  sBroker->Need(Ops);
  sBroker->Need(Orig);
  sBroker->Need(Scratch);
  sBroker->Need(Splines);
}
 
void WerkPage::Clear()
{
  sInt i;
  for(i=0;i<Ops->GetCount();i++)
    Ops->Get(i)->Clear();
  Ops->Clear();
  if(Orig)
  {
    for(i=0;i<Orig->GetCount();i++)
      Orig->Get(i)->Clear();
    Orig->Clear();
  }
  if(Scratch)
  {
    for(i=0;i<Scratch->GetCount();i++)
      Scratch->Get(i)->Clear();
    Scratch->Clear();
    Scratch = 0;
  }
  if(Splines)
  {
    for(i=0;i<Splines->GetCount();i++)
      Splines->Get(i)->Clear();
    Splines->Clear();
    Splines = 0;
  }
}

/****************************************************************************/

void WerkPage::Change(sBool reconnect)
{
  if(Orig==Ops)
  {
    if(Changed==0)
    {
      Changed = 1;
      Doc->App->PagelistWin->UpdateList();
    }
    Changed = 1;
  }
  if(reconnect)
    Reconnect = 1;
}

void WerkPage::Connect1()       // pass1: connect operators and collect stores
{
  sInt i,max,j,k,changed,swapped;
  WerkOp *po,*pp;
  sInt inc;
  WerkOp *in[KK_MAXINPUT];

  max = Ops->GetCount();

// sort operators. this speeds up the search for TOP operator

  swapped = 1;
  for(i=0;i<max-1 && swapped;i++)
  {
    swapped = 0;
    for(j=0;j<max-i-1;j++)
    {
      po = Ops->Get(j); // i > j
      pp = Ops->Get(j+1); // j < i
      if(po->PosY > pp->PosY)
      {
        Ops->Swap(j,j+1);
        swapped = 1;
      }
    }
  }


// find input operators

  for(i=0;i<max;i++)
  {
    po = Ops->Get(i);
    sVERIFY(po->Page==this);
    if(po->Hide) 
      continue;

//    if(Reconnect)
    {
      po->Error = 0;
      inc = 0;

      for(j=i-1;j>=0;j--)                      // find all inputs
      {
        pp = Ops->Get(j);
        if(pp->PosY<po->PosY-1)             // the advantage of sorted ops!
          break;
        if(pp->Hide)
          continue;

        if(pp->PosY == po->PosY-1 &&
          pp->PosX+pp->Width > po->PosX &&
          pp->PosX < po->PosX+po->Width)
        {
          while(pp && pp->Bypass) // skip bypassed ops
          {
            if(pp->Error==0)
              pp = pp->GetInput(0);     // this will work with zero inputs!
            else
              pp = 0;
          }
          if(pp)
          {
            if(inc<KK_MAXINPUT)
              in[inc++] = pp;
            else
              po->Error = 1;
          }
        }
      }

      for(j=0;j<inc-1;j++)                    // sort inputs by x
        for(k=j+1;k<inc;k++)
          if(in[j]->PosX > in[k]->PosX)
            sSwap(in[j],in[k]);

      changed = (inc!=po->GetInputCount()); // changed?
      for(j=0;j<inc && !changed;j++)
        if(in[j] != po->GetInput(j))
          changed = 1;

      if(changed)                             // if changed reset inputs 
      {
        po->Op.ClearInput();
        for(j=0;j<inc;j++)
          po->Op.AddInput(&in[j]->Op);
        po->Op.Change(-1);
      }

      if(po->Bypass)                       // check bypass 
      {
        if(po->Name[0])
          po->Error = 1;
      }

      if(po->Class->Convention&OPC_VARIABLEINPUT)  // typecheck
      {
        if(inc<po->Class->MinInput)
          po->Error = 1;
        for(j=0;j<inc;j++)
        {
          if(po->GetInput(j)->Error)
            po->Error = 1;
          if(po->GetInput(j)->Class->Output != po->Class->Inputs[0])
          {
            if(po->GetInput(j)->Class->Output==KC_ANY || po->Class->Inputs[0]==KC_ANY)
            {
              /* check here if the actual objects are correct! */
            }
            else
            {
              po->Error = 1;
            }
          }
        }
      }
      else
      {
        if(inc<po->Class->MinInput || inc>po->Class->MaxInput)
          po->Error = 1;
        for(j=0;j<inc;j++)
        {
          if(po->GetInput(j)->Error)
            po->Error = 1;
          if(po->GetInput(j)->Class->Output != po->Class->Inputs[j])
          {
            if(po->GetInput(j)->Class->Output==KC_ANY || po->Class->Inputs[j]==KC_ANY)
            {
              /* check here if the actual objects are correct! */
            }
            else
            {
              po->Error = 1;
            }
          }
        }
      }
    }
        

    if(po->Name[0])                         // add stores
    {
      pp = Doc->FindName(po->Name); 
      if(!pp)
      {
        Doc->AddStore(po);
        //Doc->Stores->Add(po);
      }
      else
      {
        po->Error = 1;
        pp->Error = 1;
      }
    }
  }

  Reconnect = sFALSE;
}

// add new STORE's

/*
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
*/
// done

  // set flags;
/*
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
*/
// update cache-need
/*
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
  */

void WerkPage::Connect2()         // pass2: follow links
{
  sInt i,j,max,changed;
  WerkOp *po,*lo;
  WerkOp *link[KK_MAXLINK];
  WerkSpline *spline[KK_MAXSPLINE];

// follow links

  max = Ops->GetCount();
  for(i=0;i<max;i++)
  {
    po = Ops->Get(i);
    sSetMem(link,0,sizeof(link));
    changed = 0;
    for(j=0;j<KK_MAXLINK;j++)
    {
      link[j]=0;
      if(po->LinkName[j][0])
      {
        /*// technically speaking, the next line is a hack, but it makes
        // reconnecting *much* faster
        if(po->GetLink(j) && Doc->Stores->IsInside(po->GetLink(j)) && !sCmpString(po->GetLink(j)->Name,po->LinkName[j]))
          link[j] = po->GetLink(j);
        else*/
          link[j] = Doc->FindName(po->LinkName[j]);
      }
      if(po->GetLink(j)!=link[j])
        changed = 1;

      lo = link[j];
      while(lo && lo->Class->Output==KC_ANY && lo->GetInputCount()==1 && lo->GetInput(0))
        lo = lo->GetInput(0);   // this should skip "store" ops with undetermined type

      if(lo)
      {
        if(lo->Error)
          po->Error = 1;
        if(lo->Class->Output != po->Class->Links[j])
        {
          if(lo->Class->Output==KC_ANY || po->Class->Links[j]==KC_ANY)
          {
            /* check here if the actual objects are correct! */
          }
          else
          {
            po->Error = 1;
          }
        }
      }
    }
    for(j=0;j<KK_MAXSPLINE;j++)
    {
      spline[j] = 0;
      if(po->SplineName[j][0])
        spline[j] = Doc->FindSpline(po->SplineName[j]);
      if(spline[j]==0 && po->Op.GetSpline(j)!=0)
        changed = 1;
      if(spline[j] &&  &spline[j]->Spline!=po->Op.GetSpline(j))
        changed = 1;
    }
    if(changed)  // changed?
    {
      for(j=0;j<KK_MAXLINK;j++)
        po->Op.SetLink(j,link[j]?&link[j]->Op:0);
      for(j=0;j<po->Op.GetSplineMax();j++)
        po->Op.SetSpline(j,spline[j]?&spline[j]->Spline:0);
      po->Op.Change(-1);
    }
  }
}

sBool WerkPage::Access()
{
  return NextOwner[0]==0 && (FakeOwner || Ops==Scratch || sCmpString(Owner,Doc->App->UserName)==0);
}

/****************************************************************************/

sList<WerkOp> *CopyOpList(sList<WerkOp> *src,WerkPage *page)
{
  sInt i,max;
  WerkOp *op;
  sList<WerkOp> *dest;

  dest = new sList<WerkOp>;
  max = src->GetCount();
  for(i=0;i<max;i++)
  {
    op = new WerkOp;
    op->Copy(src->Get(i));
    op->Page = page;
    dest->Add(op);
  }    

  return dest;
}


void WerkPage::MakeScratch()
{
  if(!Scratch)
  {
    Scratch = CopyOpList(Orig,this);
    Ops = Scratch;
    Doc->App->PagelistWin->UpdateList();
    Doc->Connect();
  }
}

void WerkPage::KeepScratch()
{
  if(Scratch)
  {
    Orig = Scratch;
    Scratch = 0;
    Ops = Orig;
    Change(0);
    Doc->App->PagelistWin->UpdateList();
    sGui->GarbageCollection();
    Doc->Connect();
  }
}

void WerkPage::DiscardScratch()
{
  if(Scratch)
  {
    Scratch = 0;
    Ops = Orig;
    Doc->App->PagelistWin->UpdateList();
    sGui->GarbageCollection();
    Doc->Connect();
  }
}

void WerkPage::ToggleScratch()
{
  if(Scratch)
  {
    if(Ops==Scratch)
      Ops = Orig;
    else
      Ops = Scratch;
    Doc->Connect();
    Doc->App->PagelistWin->UpdateList();
    Doc->App->ParaWin->SetOp(0);
  }
}

/****************************************************************************/
/****************************************************************************/

void WerkDoc::Flush(sInt kind)
{
  WerkPage *page;
  WerkOp *op;
  sInt i,j;

  for(i=0;i<Pages->GetCount();i++)
  {
    page = Pages->Get(i);
    for(j=0;j<page->Ops->GetCount();j++)
    {
      op = page->Ops->Get(j);
      if(kind == KC_ANY || op->Class->Output==kind)
      {
        op->Op.Change();
      }
    }
  }
}

WerkPage *WerkDoc::FindPage(WerkOp *op)
{
  WerkPage *page;
  sInt i,j;

  for(i=0;i<Pages->GetCount();i++)
  {
    page = Pages->Get(i);
    for(j=0;j<page->Ops->GetCount();j++)
    {
      if(op == page->Ops->Get(j))
        return page;
    }
  }
  return 0;
}

void WerkDoc::ChangeClass(sInt classid)
{
  sInt i,j;
  WerkPage *page;
  WerkOp *op;

  for(i=0;i<Pages->GetCount();i++)
  {
    page = Pages->Get(i);
    for(j=0;j<page->Ops->GetCount();j++)
    {
      op = page->Ops->Get(j);
      if(op->Class->Id == classid)
        op->Change(0);
    }
  }
}

WerkSpline *WerkDoc::FindSpline(sChar *name)
{
  sInt i,j;
  WerkPage *page;
  WerkSpline *spline;

  for(i=0;i<Pages->GetCount();i++)
  {
    page = Pages->Get(i);
    for(j=0;j<page->Splines->GetCount();j++)
    {
      spline = page->Splines->Get(j);
      if(sCmpString(spline->Name,name)==0)
      {
        return spline;
      }
    }
  }
  return 0;
}


/****************************************************************************/
/****************************************************************************/

WerkEvent::WerkEvent()
{
  Doc = 0;
  Event.Init();
  sSetMem(Name,0,sizeof(Name));
  SplineName[0] = 0;
  Op = 0;
  Line = 0;
  Select = 0;
  Spline = 0;

//  Targets = new sList<WerkAnim>;

  Event.Velocity = 1.0f;
  Event.Scale.Init(1,1,1,0);
  Event.Color = 0xffffffff;
}

WerkEvent::~WerkEvent()
{
  sVERIFY(Doc);

  Event.Exit();
}

void WerkEvent::Tag()
{
  sBroker->Need(Op);
//  sBroker->Need(Targets);
  sBroker->Need(Doc);
}

void WerkEvent::Copy(sObject *o)
{
  WerkEvent *oe;
  sVERIFY(o->GetClass()==sCID_WERKEVENT);

  oe = (WerkEvent *) o;
  Event = oe->Event;
  Op = oe->Op;
  sCopyString(Name,oe->Name,sizeof(Name));
  sCopyString(SplineName,oe->SplineName,sizeof(SplineName));
  Line = oe->Line;
  Select = oe->Select;
  Doc = oe->Doc;
}

/****************************************************************************/
/****************************************************************************/

WerkDoc::WerkDoc()
{
  Pages = new sList<WerkPage>;
  Events = new sList<WerkEvent>;
  Stores = new sList<WerkOp>;
  sCopyString(DemoName,"demo name",sizeof(DemoName));
  sCopyString(SongName,"test.v2m",sizeof(SongName));
  sCopyString(SampleName,"test.v2m",sizeof(SampleName));
  SoundBPM = 120;
  BuzzTiming = 0;
  SoundOffset = 0;
  SoundEnd = 0x01000000;
  FrCode = 23;
  ResX = 800;
  ResY = 600;
  Aspect = 1.0f*ResX/ResY;
  GlobalGamma = 1.0f;
  PlayerType = 0;
  EnvKSplines.Init(0x4000);
  EnvKSplines.Count = 0x4000;
  sSetMem(EnvKSplines.Array,0,4*0x4000);
  EnvWerkSplines.Init(0x4000);
  EnvWerkSplines.Count = 0x4000;
  sSetMem(EnvWerkSplines.Array,0,4*0x4000);
  KEnv = new KEnvironment;
}

WerkDoc::~WerkDoc()
{
  EnvKSplines.Exit();
  EnvWerkSplines.Exit();
  delete KEnv;
}


void WerkDoc::Clear()
{
  sInt i;
  for(i=0;i<Pages->GetCount();i++)
    Pages->Get(i)->Clear();
  Pages->Clear();
  for(i=0;i<Events->GetCount();i++)
    Events->Get(i)->Clear();
  Events->Clear();
  for(i=0;i<Stores->GetCount();i++)
    Stores->Get(i)->Clear();
  Stores->Clear();
  OutdatedOps = 0;
}

void WerkDoc::Tag()
{
  sInt i;

  for(i=0;i<EnvWerkSplines.Count;i++)
    sBroker->Need(EnvWerkSplines.Get(i));

  sBroker->Need(Pages);
  sBroker->Need(Events);
  sBroker->Need(Stores);
}

void WerkDoc::AddSpline(WerkSpline *spline)
{
  sInt i;

  for(i=0;i<EnvKSplines.Count;i++)
  {
    if(EnvKSplines.Array[i]==0)
    {
      sVERIFY(EnvWerkSplines.Array[i]==0);
      EnvKSplines.Array[i] = &spline->Spline;
      EnvWerkSplines.Array[i] = spline;
      spline->Index = i;
      return;
    }
  }

  sFatal("too many splines (max is %d)",EnvKSplines.Count);
}

void WerkDoc::RemSpline(WerkSpline *spline)
{
  sInt i;
  WerkPage *page;

  i = spline->Index;
  if(i>=0)
  {
    sVERIFY(i<EnvWerkSplines.Count);
    sVERIFY(EnvWerkSplines.Array[i]==spline);
    sVERIFY(EnvKSplines.Array[i]==&spline->Spline);
    EnvWerkSplines.Array[i] = 0;
    EnvKSplines.Array[i] = 0;
    spline->Index = -1;
  }
  for(i=0;i<Pages->GetCount();i++)
  {
    page = Pages->Get(i);
    page->Splines->Rem(spline);
  }
}

/****************************************************************************/

sBool WerkDoc::Write(sU32 *&data)
{
  WerkPage *page;
  WerkEvent *event;
  WerkSpline *spline;
  WerkOp *op;
  WerkOpAnim *oa;
//  WerkAnim *anim;
  sInt i,j,k,l,max,iol;
  sInt ec,pc,oc;
  sU32 *hdr;
  sList<WerkOp> *oplist;

// header

  hdr = sWriteBegin(data,sCID_WERKKZEUG,13);

// enumerate

  ec = Events->GetCount();
  pc = Pages->GetCount();
  oc = 0;
  for(i=0;i<pc;i++)
  {
    page = Pages->Get(i);
    for(j=0;j<page->Orig->GetCount();j++)
    {
      op = page->Orig->Get(j);
      op->SaveId = oc++;
    }
    if(page->Scratch)
    {
      for(j=0;j<page->Scratch->GetCount();j++)
      {
        op = page->Scratch->Get(j);
        op->SaveId = oc++;
      }
    }
  }

// header

  *data++ = ec;
  *data++ = pc;
  *data++ = oc;
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;

// pages

  for(i=0;i<pc;i++)
  {
    page = Pages->Get(i);
    *data++ = page->Ops->GetCount();
    *data++ = page->ScrollX;
    *data++ = page->ScrollY;
    *data++ = page->FakeOwner;
    *data++ = page->OwnerCount;
    *data++ = page->Scratch!=0;
    *data++ = (page->Ops==page->Scratch);
    *data++ = page->TreeLevel | (page->TreeButton<<24);
    sWriteString(data,page->Name);
    sWriteString(data,page->Owner);
    sWriteString(data,page->NextOwner);
  }

// ops

  for(i=0;i<pc;i++)
  {
    page = Pages->Get(i);
    oplist = page->Orig;
    for(iol=0;iol<2;iol++)
    {
      for(j=0;j<oplist->GetCount();j++)
      {
        op = oplist->Get(j);
        
        *data++ = op->Op.Command;
        *data++ = 0;//op->Anims->GetCount();
        *data++ = op->PosX;
        *data++ = op->PosY;
        *data++ = op->Width;
        *data++ = i;
        *data++ = op->Op.Convention;
        *data++ = iol;
        *data++ = op->Bypass;
        *data++ = op->Op.GetStringMax();
        *data++ = op->AnimOps->GetCount();      // V7
        *data++ = op->Hide;
        *data++ = op->Op.GetBlobSize();

        sWriteString(data,op->Name);              // name

        max = OPC_GETLINK(op->Op.Convention);     // links
        for(k=0;k<max;k++)
          sWriteString(data,op->LinkName[k]);

        max = OPC_GETDATA(op->Op.Convention);     // ordinary parameter
        sCopyMem4(data,op->Op.GetEditPtrU(0),max); data+=max;

        max = OPC_GETSTRING(op->Op.Convention);   // strings
        for(k=0;k<max;k++)
          sWriteString(data,op->Op.GetString(k));

        max = OPC_GETSPLINE(op->Op.Convention);   // splines
        for(k=0;k<max;k++)
          sWriteString(data,op->SplineName[k]);

        for(k=0;k<op->AnimOps->GetCount();k++)
        {
          oa = op->AnimOps->Get(k);

          *data++ = oa->PosX;
          *data++ = oa->PosY;
          *data++ = oa->Width;
          *data++ = oa->Bytecode;
          *data++ = oa->Parameter;
          *data++ = oa->UnionKind;
          *data++ = oa->AutoOp;
          *data++ = 0;
          switch(oa->UnionKind)
          {
          case AOU_VECTOR:
            oa->ConstVector.Write(data);
            break;
          case AOU_SCALAR:
            *(sF32 *)data = oa->ConstVector.x; 
            data++;
            break;
          case AOU_COLOR:
            *data++ = oa->ConstColor;
            break;
          case AOU_NAME:
            sWriteString(data,oa->SplineName);
            break;
          case AOU_EASE:
            *(sF32 *)data = oa->ConstVector.x; 
            data++;
            *(sF32 *)data = oa->ConstVector.y; 
            data++;
            break;
          }
        }
      }
      oplist = page->Scratch;
      if(oplist==0) break;
    }
  }

// events

  for(i=0;i<ec;i++)
  {
    event = Events->Get(i);
    *data++ = event->Op ? event->Op->SaveId : -1;
    *data++ = event->Event.Start;
    *data++ = event->Event.End;
    *data++ = event->Event.Select;
    *(sF32 *)data = event->Event.Velocity; data++;
    *(sF32 *)data = event->Event.Modulation; data++;
    *data++ = event->Line;
    *data++ = event->Select;
    *data++ = event->Event.Mark;    // V2
    *data++ = event->Event.Color;
    *data++ = 0;
    *data++ = 0;
    sWriteString(data,event->Name);
    event->Event.Scale.Write3(data);    // V8
    event->Event.Rotate.Write3(data);
    event->Event.Translate.Write3(data);
    sWriteString(data,event->SplineName); // V10
    *(sF32 *)data = event->Event.StartInterval; data++;   // V13
    *(sF32 *)data = event->Event.EndInterval; data++;
  }

// anims (<V7)
/*
  for(i=0;i<pc;i++)
  {
    page = Pages->Get(i);
    oplist = page->Orig;
    for(iol=0;iol<2;iol++)
    {
      for(j=0;j<oplist->GetCount();j++)
      {
        op = oplist->Get(j);
        for(k=0;k<op->Anims->GetCount();k++)
        {
          anim = op->Anims->Get(k);

          *data++ = anim->Target ? anim->Target->SaveId : -1;
          *data++ = anim->Anim.Mark;
          *data++ = anim->Anim.Mode;
          *data++ = anim->Anim.Format;
          *data++ = anim->Anim.Channels;
          *data++ = anim->Anim.Offset;
          *data++ = anim->Anim.Count;
          *(sF32 *)data = anim->Anim.Tension; data++;
          *(sF32 *)data = anim->Anim.Continuity; data++;
          *data++ = 0;
          *data++ = 0;
          *data++ = 0;

          for(l=0;l<anim->Anim.Count;l++)
          {
            *(sF32 *)data++ = anim->Anim.Keys[l].Value;
            *data++ = anim->Anim.Keys[l].Time;
            *data++ = anim->Anim.Keys[l].Channel;
          }
        }
      }
      oplist = page->Scratch;
      if(oplist==0) break;
    }
  }
*/
// splines (V7)

  for(i=0;i<pc;i++)
  {
    page = Pages->Get(i);
    *data++ = page->Splines->GetCount();

    for(j=0;j<page->Splines->GetCount();j++)
    {
      spline = page->Splines->Get(j);
      *data++ = spline->Kind;
      *data++ = spline->Spline.Count;
      *data++ = spline->Spline.Interpolation;
      *data++ = 0;
      sWriteString(data,spline->Name);
      for(k=0;k<spline->Spline.Count;k++)
      {
        *data++ = spline->Channel[k].Keys.Count;
        *data++ = 0;
        for(l=0;l<spline->Channel[k].Keys.Count;l++)
        {
          *(sF32 *)data = spline->Channel[k].Keys.Get(l).Time;  data++;
          *(sF32 *)data = spline->Channel[k].Keys.Get(l).Value; data++;
        }
      }
    }
  }

// blob data (V12)

  *data++ = 0xdeadbeef;
  for(i=0;i<pc;i++)
  {
    sInt blobsize;
    const sU8 *blobdata;

    page = Pages->Get(i);
    oplist = page->Orig;

    for(iol=0;iol<2;iol++)
    {
      for(j=0;j<oplist->GetCount();j++)
      {
        op = oplist->Get(j);
        blobdata = op->Op.GetBlob(blobsize);

        if(blobsize)
        {
          sVERIFY(blobdata);
          sCopyMem(data,blobdata,blobsize);
          data+=(blobsize+3)/4;
        }
      }
      oplist = page->Scratch;
      if(oplist==0) break;
    }
  }
  *data++ = 0xdeadbeef;

// demo parameters (V9)

  *data++ = FrCode;
  *data++ = Aspect;
  *data++ = ResX;
  *data++ = ResY;

  *(sF32 *)data = SoundBPM; data++;
  *data++ = SoundOffset;
  *data++ = SoundEnd;
  *(sF32 *)data = GlobalGamma; data++;

  *data++ = PlayerType;
  *data++ = BuzzTiming;
  *data++ = 0;
  *data++ = 0;

  *data++ = 0;
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;

  sWriteString(data,DemoName);
  sWriteString(data,SongName);
  sWriteString(data,SampleName);      // V11

// done
  
  sWriteEnd(data,hdr);
  return sTRUE;
}

/****************************************************************************/

sBool WerkDoc::Read(sU32 *&data)
{
  sInt version;
  WerkPage *page;
  WerkEvent *event;
  WerkOp *op;
  WerkOpAnim *oa;
//  WerkAnim *anim;
  WerkSpline *spline;
  sInt error;
  sInt iol;
  sInt stringcount;
  sInt oacount;
  sInt autoop;
  static sChar bigstringbuffer[0x4000];

  WerkEvent **el;
  WerkOp **ol;

  sInt i,j,k,l,max,keys;
  sInt oc,pc,ec,ac;
  sU32 conv;
  sU32 test;

  version = sReadBegin(data,sCID_WERKKZEUG);
  if(version<1) return sFALSE;
  if(version>13) return sFALSE;

  Pages->Clear();
  Events->Clear();
  Stores->Clear();

// header

  ec = *data++;
  pc = *data++;
  oc = *data++;
  ac = *data++;
  data+=4;

  el = new WerkEvent*[ec];
  ol = new WerkOp*[oc];

// pages

  for(i=0;i<pc;i++)
  {
    page = new WerkPage;
    page->Doc = this;
    Pages->Add(page);
    if(version>=3)
    {
      data++; // skip operator count, that's not needed
      page->ScrollX = *data++;
      page->ScrollY = *data++;
      page->FakeOwner = *data++;
      page->OwnerCount = *data++;
      if(*data++) page->Scratch=new sList<WerkOp>;
      if(*data++ && page->Scratch) page->Ops = page->Scratch;
      page->TreeLevel = *data++;
      page->TreeButton = (page->TreeLevel>>24)&0xff;
      page->TreeLevel &= 0x00ffffff;
      sReadString(data,page->Name,KK_NAME);
      sReadString(data,page->Owner,KK_NAME);
      sReadString(data,page->NextOwner,KK_NAME);
    }
    else
    {
      data++; // skip operator count, that's not needed
      page->ScrollX = *data++;
      page->ScrollY = *data++;
      data++; // one free space
      sReadString(data,page->Name,KK_NAME);
      sCopyString(page->Owner,App->UserName,KK_NAME);
    }
    if(sCmpString(page->NextOwner,App->UserName)==0)
    {
      sCopyString(page->Owner,App->UserName,KK_NAME);
      page->NextOwner[0] = 0;
    }
  }

// ops
  for(i=0;i<oc;i++)
  {
    op = new WerkOp(App,data[0],data[2],data[3],data[4]);
    ol[i] = op;
    conv = data[6];
    iol = data[7];
    op->Page = Pages->Get(data[5]);
    data+=8;
    if(version>=5)
    {
      op->Bypass = *data++;
      stringcount = *data++;
      oacount = *data++;
      op->Hide = *data++;
    }
    else
    {
      op->Bypass = 0;
      stringcount = 0;
      oacount = 0;
      op->Hide = 0;
    }
    if(version>=12)
    {
      op->Op.SetBlob(0,*data++);
    }

    if(iol)
      op->Page->Scratch->Add(op);
    else
      op->Page->Orig->Add(op);

    sReadString(data,op->Name,KK_NAME);         // name

    // links
    max = sMin(OPC_GETLINK(conv),OPC_GETLINK(op->Op.Convention));
    //max = OPC_GETLINK(conv);                    // links
    for(j=0;j<max;j++)
      sReadString(data,op->LinkName[j],KK_NAME);
    while(j++<OPC_GETLINK(conv))
      sReadString(data,bigstringbuffer,KK_NAME);

    // parameters
    if(OPC_GETDATA(conv)>OPC_GETDATA(op->Op.Convention))
      OutdatedOps = 1;
    max = sMin(OPC_GETDATA(conv),OPC_GETDATA(op->Op.Convention));
    //max = OPC_GETDATA(conv);                    // parameters
    sCopyMem4(op->Op.GetEditPtrU(0),data,max); 
    //data+=max;
    data += OPC_GETDATA(conv);

    if(version>=6)                              // strings
    {
      max = sMin(OPC_GETSTRING(conv),OPC_GETSTRING(op->Op.Convention));
      //max = OPC_GETSTRING(conv);
      sVERIFY(max<=stringcount);
      for(j=0;j<max;j++)
      {
        sReadString(data,bigstringbuffer,sizeof(bigstringbuffer));
        op->Op.SetString(j,bigstringbuffer);
      }
      while(j++<stringcount)
        sReadString(data,bigstringbuffer,sizeof(bigstringbuffer));
    }
    else
    {
      if(conv & 0x000f0000)                     // string parameter (not names/links)
      {
        op->Op.SetString(0,(sChar *)data);
        data+=32;
      }
    }

    // parameter splines
    sInt splineconv;
    splineconv = OPC_GETSPLINE(conv);          // workaround a bug in old save code
    if(version<12)                  
      splineconv = (conv&0x00f00000)>>20;
    max = sMin(splineconv,OPC_GETSPLINE(op->Op.Convention));
    //max = OPC_GETSPLINE(conv);                  // splines as parameter
    for(j=0;j<max;j++)
      sReadString(data,op->SplineName[j],KK_NAME);
    while(j++<splineconv)
      sReadString(data,bigstringbuffer,KK_NAME);

    // skip old animation (may be buggy)

    if(version>=7)
    {
      for(k=0;k<oacount;k++)
      {
        sInt px,py,w,byte,para,uk;

        px = *data++;
        py = *data++;
        w  = *data++;
        byte = *data++;
        para = *data++;
        uk = *data++;
        autoop = *data++;
        data++;

        oa = new WerkOpAnim;
        oa->Init(byte,para);
        oa->PosX = px;
        oa->PosY = py;
        oa->Width = w;
        oa->UnionKind = uk;

        switch(oa->UnionKind)
        {
        case AOU_VECTOR:
          oa->ConstVector.Read(data);
          break;
        case AOU_SCALAR:
          oa->ConstVector.x = *(sF32 *)data;
          data++;
          break;
        case AOU_COLOR:
          oa->ConstColor = *data++;
          break;
        case AOU_NAME:
          sReadString(data,oa->SplineName,KK_NAME);
          break;
        case AOU_EASE:
          oa->ConstVector.x = *(sF32 *)data;
          data++;
          oa->ConstVector.y = *(sF32 *)data;
          data++;
          break;
        }

        oa->UnionKind = oa->Info->UnionKind;
        oa->AutoOp = autoop;

        op->AnimOps->Add(oa);
      }
    }
  }

// events
  for(i=0;i<ec;i++)
  {
    event = new WerkEvent;
    event->Doc = this;
    Events->Add(event);

    event->Op = (*data!=-1) ? ol[*data] : 0; data++;
    event->Event.Op = &event->Op->Op;
    event->Event.Start = *data++;
    event->Event.End = *data++;
    event->Event.Select = *data++;
    event->Event.Velocity = *(sF32 *)data; data++;
    event->Event.Modulation = *(sF32 *)data; data++;
    event->Line = *data++;
    event->Select = *data++;
    if(version>=2)
    {
      event->Event.Mark = *data++;
      event->Event.Color = *data++;
      data+=2;
    }
    sReadString(data,event->Name,KK_NAME);
    if(version>=8)
    {
      event->Event.Scale.Read3(data);
      event->Event.Rotate.Read3(data);
      event->Event.Translate.Read3(data);
    }
    if(version>=10)
    {
      sReadString(data,event->SplineName,KK_NAME);
    }
    if(version>=13)
    {
      event->Event.StartInterval = *(sF32 *)data; data++;
      event->Event.EndInterval = *(sF32 *)data; data++;
    }
  }

// anims
  if(version<7)
  {
    for(i=0;i<ac;i++)
    {
/*
      anim = new WerkAnim;
      
      anim->Target = (*data!=-1) ? ol[*data] : 0; data++;
      if(version>=2)
        anim->Anim.Mark = *data++;
      else
        data++;
      anim->Anim.Mode = *data++;
      anim->Anim.Format = *data++;
      anim->Anim.Channels = *data++;
      anim->Anim.Offset = *data++;
      anim->Anim.Count = *data++;
      anim->Anim.Tension = *(sF32 *)data; data++;
      anim->Anim.Continuity = *(sF32 *)data; data++;
      data+=3;
      for(j=0;j<anim->Anim.Count;j++)
      {
        anim->Anim.Keys[j].Value = *(sF32 *)data; data++;
        anim->Anim.Keys[j].Time = *data++;
        anim->Anim.Keys[j].Channel = *data++;
      }

      sVERIFY(anim->Target);
      if(anim->Target) 
      {
        anim->Anim.Target = &anim->Target->Op;
        anim->Target->Anims->Add(anim);
      }
*/
      data+=6;
      max = *data++;
      data+=5;
      for(j=0;j<max;j++)
        data+=3;
    }
  }

// splines (V7)
  if(version>=7)
  {
    for(i=0;i<pc;i++)
    {
      page = Pages->Get(i);
      max = *data++;

      for(j=0;j<max;j++)
      {
        sInt kind,count,inter;
        sF32 val,time;

        kind = *data++;
        count = *data++;
        inter = *data++;
        data++;

        spline = new WerkSpline;
        spline->Init(count,"",kind,0,inter);
        AddSpline(spline);
        sReadString(data,spline->Name,KK_NAME);

        for(k=0;k<spline->Spline.Count;k++)
        { 
          keys = *data++;
          data++;

          for(l=0;l<keys;l++)
          {
            time = *(sF32 *)data; data++;
            val  = *(sF32 *)data; data++;
            spline->AddKey(k,time,val)->Select = 0;
          }
        }
        spline->Sort();
        page->Splines->Add(spline);
      }
    }
  }

  if(version>=12)
  {
    test = *data++;
    sVERIFY(test==0xdeadbeef);
    
    for(i=0;i<oc;i++)     // version 12: read blobdata
    {
      sInt blobsize;
      sU8 *blobdata;
      op = ol[i];

      blobsize = op->Op.GetBlobSize();
      if(blobsize!=0)
      {
        blobdata = new sU8[blobsize];
        sCopyMem(blobdata,data,blobsize);
        op->Op.SetBlob(blobdata,blobsize);
        data+=(blobsize+3)/4;
      }
    }

    test = *data++;
    sVERIFY(test==0xdeadbeef);
  }

  delete[] ol;
  delete[] el;

// demo-parameters (V9)

  if(version>=9)
  {
    FrCode = *data++;
    Aspect = *data++;
    ResX = *data++;
    ResY = *data++;
    SoundBPM = *(sF32 *)data; data++;
    SoundOffset = *data++;
    SoundEnd = *data++;
    GlobalGamma = *(sF32 *)data++;
    PlayerType = *data++;
    BuzzTiming = *data++;
    data += 6;
    sReadString(data,DemoName,sizeof(DemoName));
    sReadString(data,SongName,sizeof(SongName));
    if(version>=11)
      sReadString(data,SampleName,sizeof(SampleName));

    if(GlobalGamma<=0 || GlobalGamma>256) GlobalGamma = 1.0f;
    sSystem->SetGamma(GlobalGamma);
  }

// create empty document

  sDPrintF("actual load done\n");

  error = sReadEnd(data);
  if(Pages->GetCount()==0)
    AddPage("start");

// fix old versions of blobs (ugly)

  sFORLIST(Pages,page)
  {
    sFORLIST(page->Ops,op)
    {
      if(op->Class->Id==0x0018)
      {
        sInt *di = (sInt *)op->Op.GetBlobData();
        if(di[1]==1)
        {
          sInt bytes;
          BlobSpline *bs = MakeBlobSpline(&bytes,di[0]);
          bs->Select = di[2];
          di+=4;

          for(sInt i=0;i<bs->Count;i++)
          {
            sF32 *df = (sF32*)di;
            bs->Keys[i].Select = di[0];
            bs->Keys[i].Time = df[1];
            bs->Keys[i].px = df[2];
            bs->Keys[i].py = df[3];
            bs->Keys[i].pz = df[4];
            bs->Keys[i].rx = df[5];
            bs->Keys[i].ry = df[6];
            bs->Keys[i].rz = df[7];
            bs->Keys[i].Zoom = 1.0f;
            di += 8;
          }

          sVERIFY(di == (sInt *)(op->Op.GetBlobData()+op->Op.GetBlobSize()));

          op->Op.SetBlob((sU8 *)bs,bytes);
        }
      }
    }
  }

// connect & un-change

  Connect();
  ConnectAnim();
  for(i=0;i<Pages->GetCount();i++)
    Pages->Get(i)->Changed = 0;

// done

  return error;
}

/****************************************************************************/

WerkPage *WerkDoc::AddPage(sChar *name,sInt pos)
{
  WerkPage *page;
  page = new WerkPage;
  page->Doc = this;
  sCopyString(page->Name,name,KK_NAME);
  sCopyString(page->Owner,App->UserName,KK_NAME);
  if(pos==-1)
    Pages->Add(page);
  else
    Pages->AddPos(page,pos);
  App->PagelistWin->UpdateList();
  return page;
}

void WerkDoc::RemPage(WerkPage *page)
{
  sInt i;
  for(i=0;i<page->Splines->GetCount();i++)
    RemSpline(page->Splines->Get(i));
  Pages->RemOrder(page);
}

sBool WerkDoc::FindNameIndex(sChar *name,sInt &index)
{
  sInt l,r,x,cmp;
  WerkOp *op;

  l = 0;
  r = Stores->GetCount();
  while(l < r)
  {
    x = (l + r) / 2;
    op = Stores->Get(x);
    cmp = sCmpString(name,op->Name);

    if(cmp == 0) // found!
    {
      index = x;
      return sTRUE;
    }
    else if(cmp < 0)
      r = x;
    else
      l = x + 1;
  }

  index = l;
  return sFALSE;
}

void WerkDoc::AddStore(WerkOp *store)
{
  sInt index;
  sBool found;

  found = FindNameIndex(store->Name,index);
  sVERIFY(!found);

  Stores->AddPos(store,index);
}

WerkOp *WerkDoc::FindName(sChar *name)         // find store by name
{
  /*WerkOp *op;
  sInt i,max;

  max = Stores->GetCount();
  for(i=0;i<max;i++)
  {
    op = Stores->Get(i);
    if(sCmpString(name,op->Name)==0)
      return op;
  }
  return 0;*/
  sInt i;
  sBool found;

  found = FindNameIndex(name,i);
  return found ? Stores->Get(i) : 0;
}

WerkOp *WerkDoc::FindRoot(sInt index)
{
  WerkOp *root,*root0;
  sChar buffer[16];

  sSPrintF(buffer,sizeof(buffer),"root_%d",index);

  root = FindName(buffer);
  if(index==0)
  {
    root0 = FindName("root");   // "root" is an alias to "root_0"
    if(root0 && root)           // error when both "root_0" and "root" are defined
      return 0;
    if(root==0)
      root = root0;
  }

  return root;
}

void WerkDoc::OwnAll()
{
  sInt i,max,count;
  WerkPage *page;
  max = Pages->GetCount();

  count = App->UserCount+1;
  for(i=0;i<max;i++)
  {
    page = Pages->Get(i);
    if(page->OwnerCount > count)
      count = page->OwnerCount;
  }
  for(i=0;i<max;i++)
  {
    page = Pages->Get(i);
    if(sCmpString(page->Owner,App->UserName)!=0)
    {
      sCopyString(page->Owner,App->UserName,KK_NAME);
      page->OwnerCount = count;
      page->NextOwner[0] = 0;
      App->UserCount = count;
    }
  }
  if(App->ParaWin->Op) App->ParaWin->SetOp(App->ParaWin->Op);
}

WerkPage *WerkDoc::AddCopyOfPage(WerkPage *source)
{
  WerkPage *page;
  WerkOp *op;
  WerkSpline *spline;
  sInt i,max;

  page = AddPage(source->Name);
  sCopyString(page->Owner,source->Owner,KK_NAME);
  sCopyString(page->NextOwner,source->NextOwner,KK_NAME);

  page->ScrollX = source->ScrollX;
  page->ScrollY = source->ScrollY;
  page->TreeLevel = source->TreeLevel;
  page->TreeButton = source->TreeButton;
  page->OwnerCount = source->OwnerCount;
  page->MergeError = 0;

  max = source->Ops->GetCount();
  for(i=0;i<max;i++)
  {
    op = new WerkOp;
    op->Copy(source->Ops->Get(i));
    page->Ops->Add(op);
  }
  max = source->Splines->GetCount();
  for(i=0;i<max;i++)
  {
    spline = new WerkSpline;
    spline->Copy(source->Splines->Get(i));
    page->Splines->Add(spline);
    AddSpline(spline);
  }

  return page;
}

/****************************************************************************/

void WerkDoc::ClearInstanceMem()
{
  sInt i,j,max;
  WerkPage *page;
  WerkOp *op;

  KEnv->InitView();
  for(i=0;i<Pages->GetCount();i++)
  {
    page = Pages->Get(i);
    max = page->Ops->GetCount();
    for(j=0;j<max;j++)
    {
      op = page->Ops->Get(j);
      if(op->Op.SceneMemLink)
      {
        op->Op.SceneMemLink->DeleteChain();
        op->Op.SceneMemLink = 0;
      }
    }
/*    for(j=0;j<page->Splines->GetCount();j++)
    {
      page->Splines->Get(j)->Clear();
    }*/
  }
}

void WerkDoc::Connect()
{
  sInt i,max;
  sInt j,ops;
  sInt k;
  WerkOp *op;
  WerkPage *page;
  WerkEvent *event;
//  WerkAnim *anim;
//  sInt mark;

// do we really have to reconnect?

  j = 0;
  max = Pages->GetCount();
  for(i=0;i<max;i++)
  {
    j |= Pages->Get(i)->Reconnect;
    Pages->Get(i)->Reconnect=0;
  }
  if(!j)
    return;
  ClearInstanceMem();

// do connections and find all stores

  Stores->Clear();
  max = Pages->GetCount();
  for(i=0;i<max;i++)
    Pages->Get(i)->Connect1();
  for(i=0;i<max;i++)
    Pages->Get(i)->Connect2();


// fix events

  for(i=0;i<Events->GetCount();i++)
  {
    event = Events->Get(i);  
    event->Op = 0;
    event->Event.Op = 0;
    event->Event.Spline = 0;
    event->Op = FindName(event->Name);
    event->Spline = FindSpline(event->SplineName);
    if(event->Op)
      event->Event.Op = &event->Op->Op;
    if(event->Spline)
      event->Event.Spline = &event->Spline->Spline;
  }

// get Outputcount

  for(i=0;i<max;i++)
  {
    page = Pages->Get(i);
    ops = page->Ops->GetCount();
    for(j=0;j<ops;j++)
      page->Ops->Get(j)->Op.ClearOutput();
  }
  for(i=0;i<max;i++)
  {
    page = Pages->Get(i);
    ops = page->Ops->GetCount();
    for(j=0;j<ops;j++)
    {
      op = page->Ops->Get(j);
      for(k=0;k<op->GetInputCount();k++)
        op->GetInput(k)->AddOutput(op);
      for(k=0;k<KK_MAXLINK;k++)
        if(op->Op.GetLink(k))
          op->GetLink(k)->AddOutput(op);
    }
  }

  // flush events
  ClearInstanceMem();
}

void WerkDoc::ConnectAnim()
{
  sInt i,j,max;
  WerkPage *page;
  WerkOp *op;

  for(i=0;i<Pages->GetCount();i++)
  {
    page = Pages->Get(i);
    max = page->Ops->GetCount();
    for(j=0;j<max;j++)
    {
      op = page->Ops->Get(j);
      op->ConnectAnim(this);
    }
  }
}

/****************************************************************************/
/****************************************************************************/

void WerkDoc::ClearCycleFlag()
{
  WerkOp *op;
  WerkPage *page;
  sInt i,j;

  for(i=0;i<Pages->GetCount();i++)
  {
    page = Pages->Get(i);
    for(j=0;j<page->Ops->GetCount();j++)
    {
      op = page->Ops->Get(j);
      op->CycleFind = 0;
    }
  }
}


void WerkDoc::DumpOpByClassR(sChar *name,WerkOp *op)
{
  sInt i,max;
  WerkOp *child;

  if(op->CycleFind==0)
  {
    op->CycleFind=1;

    max = op->GetInputCount();
    for(i=0;i<max;i++)
    {
      child = op->GetInput(i);
      DumpOpByClassR(name,child);
    }

    max = op->GetLinkMax();
    for(i=0;i<max;i++) 
    {
      child = op->GetLink(i);
      if(child)
        DumpOpByClassR(name,child);
    }

    if(sCmpStringI(op->Class->Name,name)==0)
      App->FindResultsWin->Add(op);
  }
}

void WerkDoc::DumpOpByClass(sChar *name)
{
  WerkOp *op;
  sInt i;

  ClearCycleFlag();
  App->FindResultsWin->Clear();

  for(i=0;i<MAX_OP_ROOT;i++)
  {
    op = FindRoot(i);
    if(op)
      DumpOpByClassR(name,op);

    App->FindResultsWin->Update();
  }
}

void WerkDoc::DumpReferencesR(WerkOp *ref,WerkOp *op)
{
  sInt i,max;
  WerkOp *child;

  if(!op->CycleFind)
  {
    op->CycleFind = 1;

    max = op->GetInputCount();
    for(i=0;i<max;i++)
    {
      child = op->GetInput(i);
      DumpReferencesR(ref,child);
      
      if(child == ref)
        App->FindResultsWin->Add(op,"as input");
    }

    max = op->GetLinkMax();
    for(i=0;i<max;i++)
    {
      child = op->GetLink(i);
      if(child)
        DumpReferencesR(ref,child);

      if(child == ref)
        App->FindResultsWin->Add(op,"as link");
    }
  }
}

void WerkDoc::DumpReferences(WerkOp *ref)
{
  App->FindResultsWin->Clear();
  for(sInt i=0;i<Pages->GetCount();i++)
  {
    WerkPage *page = Pages->Get(i);

    // go through actual ops
    for(sInt j=0;j<page->Orig->GetCount();j++)
    {
      WerkOp *op = page->Orig->Get(j);

      sInt max = op->GetInputCount();
      for(sInt k=0;k<max;k++)
        if(op->GetInput(k) == ref)
          App->FindResultsWin->Add(op,"as input");

      max = op->GetLinkMax();
      for(sInt k=0;k<max;k++)
        if(op->GetLink(k) == ref)
          App->FindResultsWin->Add(op,"as link");
    }
  }
  App->FindResultsWin->Update();

  /*WerkOp *op;
  sInt i;

  ClearCycleFlag();
  App->FindResultsWin->Clear();

  for(i=0;i<MAX_OP_ROOT;i++)
  {
    op = FindRoot(i);
    if(op)
      DumpReferencesR(ref,op);
  }

  App->FindResultsWin->Update();*/
}

void WerkDoc::DumpBugsR(WerkOp *op)
{
  sInt max;

  if(!op->CycleFind)
  {
    op->CycleFind = 1;

    if(!op->Class || op->Class->Id == 0x04)
      App->FindResultsWin->Add(op,"unknown class");
    else
    {
      sInt id = op->Class->Id;

      switch(id)
      {
      case 0x06: // event
        if(op->GetInputCount() != 1)
          App->FindResultsWin->Add(op,"Event ops need exactly one input");
        break;

      case 0x62: // particles
        App->FindResultsWin->Add(op,"'Particles' op is deprecated");
        break;

      case 0x8d: // cut
        App->FindResultsWin->Add(op,"'Cut' op is deprecated");
        break;

      case 0x9d: // mesh grid
      case 0x101: // minmesh grid
        if((*op->Op.GetEditPtrU(0) & 1) == 0)
          App->FindResultsWin->Add(op,"1-sided 'Grid's cause shadow problems");
        break;

      case 0xa4: // set pivot
        App->FindResultsWin->Add(op,"'Set Pivot' op is deprecated");
        break;

      case 0xb2: // bsp
        App->FindResultsWin->Add(op,"'BSP' op is experimental");
        break;

      case 0xcf: // forcelights
        App->FindResultsWin->Add(op,"'ForceLights' op is deprecated");
        break;

      case 0xe5: // sharpen
        App->FindResultsWin->Add(op,"'Sharpen' op doesn't work in player");
        break;

      case 0xc0: case 0xc2: case 0xc3: // scene/multiply/transform
        {
          sF32 *scale = op->Op.GetEditPtrF(0);
          if(scale[0] != 1.0f || scale[1] != 1.0f || scale[2] != 1.0f)
            App->FindResultsWin->Add(op,"Scale in scene op not (1,1,1)");
        }
        break;

      case 0xc4: // scene light
        if(*op->Op.GetEditPtrF(10) <= 0.0f)
          App->FindResultsWin->Add(op,"Light range is <=0");
        break;
      }

      // check op data
      sChar *packStr = op->Class->Packing;
      sBool err = sFALSE;

      for(sInt i=0;packStr[i];i++)
      {
        sChar pack = packStr[i];
        if(pack >= 'A' && pack <= 'Z')
          pack += 'a' - 'A';

        sF32 valF = *op->Op.GetEditPtrF(i);
        sU32 valU = *op->Op.GetEditPtrU(i);
        sS32 valS = *op->Op.GetEditPtrS(i);

        switch(pack)
        {
        case 'g': if(sFAbs(valF) > 65504.0f)          err = sTRUE; break;
        case 'e': if(valF < -8.0f || valF > 8.0f)     err = sTRUE; break;
        case 's': if(valS < -32768 || valS > 32767)   err = sTRUE; break;
        case 'b': if(valU > 255)                      err = sTRUE; break;
        case 'm': if(valU > 0xffffff)                 err = sTRUE; break;
        }
      }

      if(err)
        App->FindResultsWin->Add(op,"parameter range error (export packing)");
    }

    max = op->GetInputCount();
    for(sInt i=0;i<max;i++)
      DumpBugsR(op->GetInput(i));

    max = op->GetLinkMax();
    for(sInt i=0;i<max;i++)
    {
      WerkOp *child = op->GetLink(i);
      if(child)
        DumpBugsR(child);
    }
  }
}

void WerkDoc::DumpBugs()
{
  ClearCycleFlag();
  App->FindResultsWin->Clear();

  sBool gotRoot = sFALSE, gotRoot0 = sFALSE;

  for(sInt i=0;i<MAX_OP_ROOT;i++)
  {
    WerkOp *op = FindRoot(i);
    if(op)
    {
      DumpBugsR(op);
      gotRoot = sTRUE;
      if(i == 0)
        gotRoot0 = sTRUE;
    }
  }

  if(!gotRoot)
    App->FindResultsWin->Add(0,"no root operator found");
  else if(!gotRoot0)
    App->FindResultsWin->Add(0,"roots found, but no root 0");

  App->FindResultsWin->Update();
}

void WerkDoc::DumpBugsRelative(WerkOp *base)
{
  ClearCycleFlag();
  App->FindResultsWin->Clear();

  DumpBugsR(base);

  App->FindResultsWin->Update();
}

void WerkDoc::DumpMaterialsR(WerkOp *op)
{
  sInt max;

  if(!op->CycleFind)
  {
    op->CycleFind = 1;
    if(op->Class)
    {
      sInt id = op->Class->Id;

      switch(id)
      {
      case 0x96:  // Mesh MatLink
      case 0x110: // MinMesh MatLink
      case 0x116: // MinMesh MatLinkID
      case 0x117: // MinMesh MatInput
        {
          WerkOp *link = op->GetLink(0);
          if(id == 0x117) // MatInput
            link = op->GetInput(1);

          if(link)
          {
            sChar *name = link->Name;
            if(!name || !name[0])
              name = "(unnamed op)";

            sChar buffer[256];
            sSPrintF(buffer,sizeof(buffer),"pass %d: '%s'\n",
              *op->Op.GetEditPtrU(1),name);

            App->FindResultsWin->Add(op,buffer);
          }
        }
        break;
      }
    }

    max = op->GetInputCount();
    for(sInt i=0;i<max;i++)
      DumpMaterialsR(op->GetInput(i));

    max = op->GetLinkMax();
    for(sInt i=0;i<max;i++)
    {
      WerkOp *child = op->GetLink(i);
      if(child)
        DumpMaterialsR(child);
    }
  }
}

void WerkDoc::DumpMaterials(WerkOp *base)
{
  ClearCycleFlag();
  App->FindResultsWin->Clear();

  DumpMaterialsR(base);

  App->FindResultsWin->Update();
}

/****************************************************************************/

static sInt DumpUsedSwitchArray[KKRIEGER_SWITCHES];

void WerkDoc::DumpUsedSwitchesR(WerkOp *op)
{
  sInt i,max;
  WerkOp *child;

  if(op->CycleFind==0)
  {
    op->CycleFind=1;

    max = op->GetInputCount();
    for(i=0;i<max;i++)
    {
      child = op->GetInput(i);
      DumpUsedSwitchesR(child);
    }

    max = op->GetLinkMax();
    for(i=0;i<max;i++) 
    {
      child = op->GetLink(i);
      if(child)
        DumpUsedSwitchesR(child);
    }

    switch(op->Class->Id)
    {
    case 0x08:    // delay
    case 0xcf:    // forcelights
      DumpUsedSwitchArray[255&*op->Op.GetEditPtrU(1)]++;
      break;

    case 0x0c:    // playsample
    case 0x09:    // if
    case 0x0a:    // setif
      DumpUsedSwitchArray[255&*op->Op.GetEditPtrU(0)]++;
      break;

    case 0x0b:    // state
      DumpUsedSwitchArray[255&*op->Op.GetEditPtrU(0)]++;
      DumpUsedSwitchArray[255&*op->Op.GetEditPtrU(3)]++;
      break;

    case 0x11:    // monster
      DumpUsedSwitchArray[255&*op->Op.GetEditPtrU(17)]++;
      break;

    case 0x9c:    // collisioncube
      DumpUsedSwitchArray[255&*op->Op.GetEditPtrU(11)]++;
      DumpUsedSwitchArray[255&*op->Op.GetEditPtrU(13)]++;
      break;
    }
  }  
}

void WerkDoc::DumpUsedSwitches()
{
  WerkOp *op;
  sInt i,j,rn;
  sChar buffer[256];
  sChar *s;

  ClearCycleFlag();

  for(rn=0;rn<MAX_OP_ROOT;rn++)
  {
    op = FindRoot(rn);
    if(op)
    {
      sDPrintF("used switches:\n");
      DumpUsedSwitchesR(op);

      for(i=0;i<16;i++)
      {
        s = buffer;
        for(j=0;j<16;j++)
        {
          if(DumpUsedSwitchArray[i*16+j]>9)
            *s++ = '*';
          else
            *s++ = DumpUsedSwitchArray[i*16+j]+'0';
          if(j==3 || j==7 || j==11)
            *s++ = '-';
        }
        *s++ = 0;
        sDPrintF("%02x (%3d):%s\n",i*16,i*16,buffer);
      }
    }
  }
}

/****************************************************************************/

void WerkDoc::RenameOps(sChar *o,sChar *n)
{
  sInt i,max;

  max = Pages->GetCount();
  for(i=0;i<max;i++)
    Pages->Get(i)->RenameOps(o,n);
  max = Events->GetCount();
  for(i=0;i<max;i++)
    Events->Get(i)->Rename(o,n);
  Connect();
}

void WerkPage::RenameOps(sChar *o,sChar *n)
{
  sInt i,max;

  max = Ops->GetCount();
  for(i=0;i<max;i++)
    Ops->Get(i)->Rename(o,n);
}

void WerkOp::Rename(sChar *o,sChar *n)
{
  sInt i;
  if(sCmpString(Name,o)==0)
    sCopyString(Name,n,KK_NAME);
  for(i=0;i<KK_MAXLINK;i++)
    if(sCmpString(LinkName[i],o)==0)
      sCopyString(LinkName[i],n,KK_NAME);
}

void WerkEvent::Rename(sChar *o,sChar *n)
{
//  sInt i,max;
//  max = Targets->GetCount();
//  for(i=0;i<max;i++)
//    Targets->Get(i)->Rename(o,n);
  if(sCmpString(Name,o)==0)
    sCopyString(Name,n,KK_NAME);
}
/*
void WerkAnim::Rename(sChar *o,sChar *n)
{
  if(sCmpString(Name,o)==0)
    sCopyString(Name,n,KK_NAME);
}
*/
/****************************************************************************/
/****************************************************************************/

WerkOp *WerkExport::SkipNops(WerkOp *op)
{
  if(!op || op->Op.HasAnim())
    return op;
  else if(op->Op.Command == 0x01) // load
    return SkipNops(op->GetLink(0));
  else if(op->Op.Command == 0x02 && op->Op.GetInputCount() == 1) // store
    return SkipNops(op->GetInput(0));
  else if(op->Op.Command == 0x03 && op->Op.GetInputCount() == 1) // nop
    return SkipNops(op->GetInput(0));
  else if(op->Op.Command == 0x2b) // hscb
  {
    if(*op->Op.GetEditPtrF(0) == 0.0f && *op->Op.GetEditPtrF(1) == 1.0f
      && *op->Op.GetEditPtrF(2) == 1.0f && *op->Op.GetEditPtrF(3) == 1.0f)
      return SkipNops(op->GetInput(0));
    else
      return op;
  }
  else if(op->Op.Command == 0x2c) // rotate
  {
    if(*op->Op.GetEditPtrF(0) == 0.0f && *op->Op.GetEditPtrF(1) == 1.0f
      && *op->Op.GetEditPtrF(2) == 1.0f && *op->Op.GetEditPtrF(3) == 0.5f
      && *op->Op.GetEditPtrF(4) == 0.5f && *op->Op.GetEditPtrU(6) == 0
      && *op->Op.GetEditPtrU(7) == 0)
      return SkipNops(op->GetInput(0));
    else
      return op;
  }
  else if(op->Op.Command == 0x88) // mesh transform
  {
    if(*op->Op.GetEditPtrF(1) == 1.0f && *op->Op.GetEditPtrF(2) == 1.0f
      && *op->Op.GetEditPtrF(3) == 1.0f && *op->Op.GetEditPtrF(4) == 0.0f
      && *op->Op.GetEditPtrF(5) == 0.0f && *op->Op.GetEditPtrF(6) == 0.0f
      && *op->Op.GetEditPtrF(7) == 0.0f && *op->Op.GetEditPtrF(8) == 0.0f
      && *op->Op.GetEditPtrF(9) == 0.0f)
      return SkipNops(op->GetInput(0));
    else
      return op;
  }
  else if(op->Op.Command == 0x92 && op->Op.GetInputCount() == 1) // mesh add
    return SkipNops(op->GetInput(0));
  else if(op->Op.Command == 0xc1 && op->Op.GetInputCount() == 1) // scene add
    return SkipNops(op->GetInput(0));
  else if(op->Op.Command == 0xc3) // scene transform
  {
    if(*op->Op.GetEditPtrF(0) == 1.0f && *op->Op.GetEditPtrF(1) == 1.0f
      && *op->Op.GetEditPtrF(2) == 1.0f && *op->Op.GetEditPtrF(3) == 0.0f
      && *op->Op.GetEditPtrF(4) == 0.0f && *op->Op.GetEditPtrF(5) == 0.0f
      && *op->Op.GetEditPtrF(6) == 0.0f && *op->Op.GetEditPtrF(7) == 0.0f
      && *op->Op.GetEditPtrF(8) == 0.0f)
      return SkipNops(op->GetInput(0));
    else
      return op;
  }
  else
    return op;
}

void WerkExport::ExportSpline(KSpline *spline)
{
  sInt i;

  for(i=0;i<SplineSet->Count;i++)
    if(SplineSet->Get(i) == spline)
      break;

  if(i == SplineSet->Count)
  {
    *(SplineSet->Add()) = spline;
    spline->SaveId = i;
  }
}

void WerkExport::BuildExportSetR(WerkOp *op)
{
  sInt i;

  op = SkipNops(op);
  if(!ExportSet->IsInside(op))
  {
    for(i=0;i<op->GetInputCount();i++)
      BuildExportSetR(op->GetInput(i));

    for(i=0;i<op->GetLinkMax();i++)
      if(op->GetLink(i))
        BuildExportSetR(op->GetLink(i));

    for(i=0;i<op->Op.GetSplineMax();i++)
      if(op->Op.GetSpline(i))
        ExportSpline(op->Op.GetSpline(i));

    sVERIFY(op->Op.Command < MAXOPID);
    if(OpTypeRemap[op->Op.Command] == -1)
    {
      OpTypeRemap[op->Op.Command] = OpTypeCount;
      OpTypes[OpTypeCount++] = op->Op.Command;
    }

    op->SaveId = ExportSet->GetCount();
    op->Op.OpId = ExportSet->GetCount();
    ExportSet->Add(op);

    if(op->Op.Command == 0x06 && op->GetInputCount() != 1)
      sDPrintF("eeeh.... '%s'\n",op->Name);
  }
}

sBool WerkExport::Export(WerkDoc *doc,sU8 *&dataPtr)
{
  sInt i,j,k,nInputs,in,typeByte,link,eventCount,sz;
  WerkExportOpSize total;
  sU32 conv;
  sChar *pack;
  sU8 *data,*dstart,*bytecode,*gstart;
  WerkClass *cls;
  WerkEvent *event;
  WerkOp *root[MAX_OP_ROOT],*op;
  KSpline *spline;

  sU8 *SongData;
  sInt SongSize;
  sU8 *SampleData;
  sInt SampleSize;

  Doc = doc;
  Doc->Connect();

  // do a find bugs, so the user can't export without seeing the bug list
  Doc->DumpBugs();
  Doc->App->FindResults(sTRUE);

  for(i=0;i<MAX_OP_ROOT;i++)
  {
    root[i] = Doc->FindRoot(i);
    if(root[i])
      root[i] = SkipNops(root[i]);
  }

  if(!root[0])
    return sFALSE;
  
  // prepare and build export set

  SongSize = 0;
  SampleSize = 0;
  if(doc->SongName)
  {
    SongData = sSystem->LoadFile(doc->SongName,SongSize);
    if(!SongData)
      SongSize = 0;

    sInt snlen = sGetStringLen(Doc->SongName);
    if(snlen < 4 || sCmpMem(Doc->SongName + snlen - 4,".ogg",4))
    {
      // v2mconv
      sInt convertedSize;
      sU8 *convertBuffer = sViruz2::ConvertV2M(SongData,SongSize,convertedSize);

      delete[] SongData;
      SongData = convertBuffer;
      SongSize = convertedSize;
    }
  }
  if(doc->SampleName)
  {
    SampleData = sSystem->LoadFile(doc->SampleName,SampleSize);
    if(!SampleData)
      SampleSize = 0;
  }

  ExportSet = new sList<WerkOp>;
  SplineSet = new sArray<KSpline*>;
  SplineSet->Init();
  OpTypeCount = 0;
  for(i=0;i<MAXOPID;i++)
  {
    OpTypeRemap[i] = -1;
  }
  for(i=0;i<256;i++)
  {
    OpTypes[i] = 0;
  }

  for(i=0;i<MAX_OP_ROOT;i++)
  {
    if(root[i])
      BuildExportSetR(root[i]);
  }
  sVERIFY(OpTypeCount < 128);
  sVERIFY(ExportSet->GetCount() < 32768);

  // count (interesting) events and add event splines
  eventCount = 0;
  for(i=0;i<Doc->Events->GetCount();i++)
  {
    event = Doc->Events->Get(i);
    if(ExportSet->IsInside(SkipNops(event->Op)))
    {
      eventCount++;
      if(event->Event.Spline)
        ExportSpline(event->Event.Spline);
    }
  }

  // go through all the relevant anim bytecode sections to find used
  // splines
  for(i=0;i<ExportSet->GetCount();i++)
  {
    KSpline *spline;

    op = ExportSet->Get(i);
    bytecode = op->Op.GetAnimCode();

    do
    {
      j = *bytecode;
      switch(j)
      {
      case KA_SPLINE:
        spline = Doc->EnvKSplines[*((sU16 *) (bytecode+1))];
        if(!spline || spline == (KSpline *) 0xcdcdcdcd)
          sDPrintF("missing/damaged splines! op x=%d y=%d page='%s'\n",op->PosX,op->PosY,op->Page->Name);

        ExportSpline(spline);
        //ExportSpline(Doc->EnvKSplines[*((sU16 *) (bytecode+1))]);
        bytecode += 3;
        break;

      default:
        bytecode += CalcCmdSize(bytecode);
      }
    }
    while(j != KA_END);
  }

  sVERIFY(SplineSet->Count < 32767);

  // prepare export stats
  for(i=0;i<MAXOPID;i++)
  {
    OpSize[i].Graph = 0;
    OpSize[i].Data = 0;
    OpSize[i].Count = 0;
    OpSize[i].LastOp = 0;
  }

  // we are now ready to write export data
  data = dataPtr;
  dstart = data;

  // some header stuff..
  
  sInt flags = 1;                                   // default flags
  if(SampleSize>0) flags |= 2;                      // kkrieger-style sound effects
  if(doc->PlayerType==1) flags &= ~1;               // intros don't save name
  if(doc->BuzzTiming) flags |= 4;                   // timing fix for buzz
  *(sU32 *)data = flags;
  data+=4;

  if(flags & 1)                                     // write name
  {
    sCopyMem(data,doc->DemoName,32);
    data += 32;
  }

  *(sU32 *)data = SongSize; data+=4;                // song
  sCopyMem(data,SongData,SongSize);
  data += sAlign(SongSize,4);

  if(flags & 2)
  {
    *(sU32 *)data = SampleSize; data+=4;            // soundeffects (kkrieger)
    sCopyMem(data,SampleData,SampleSize);
    data += sAlign(SampleSize,4);
  }

  *(sU32 *)data = Doc->SoundBPM*0x10000; data+=4;   // BPM
  *(sU32 *)data = Doc->SoundEnd; data+=4;           // end of demo


  // the document itself....

  sWriteShort(data,ExportSet->GetCount());          
  sWriteShort(data,SplineSet->Count);

  // write all roots (there might be multiple for multiparters)
  for(i=0;i<MAX_OP_ROOT;i++)
  {
    if(root[i])
      sWriteShort(data,root[i]->SaveId);
    else
      sWriteShort(data,ExportSet->GetCount());
  }

  // write op classes list
  for(i=0;i<OpTypeCount;i++)
  {
    for(cls=WerkClasses;cls->Id != OpTypes[i];cls++);

    conv = cls->Convention;
    if(cls->MinInput != cls->MaxInput)
      conv |= OPC_FLEXINPUT;

    *((sU32 *) data) = conv; data += 4;
    *((sU16 *) data) = cls->Id; data += 2;
    sCopyString((sChar *) data,cls->Packing,256);
    data += sGetStringLen((sChar *) data) + 1;
  }

  *((sU32 *) data) = 0; data += 4;
  //sSystem->SaveFile("exp\\pregraph.dat",dstart,data-dstart);

  // write op graph
  gstart = data;

  for(i=0;i<ExportSet->GetCount();i++)
  {
    dstart = data;

    op = ExportSet->Get(i);
    sVERIFY(op->Op.Command<MAXOPID);
    typeByte = OpTypeRemap[op->Op.Command];

    for(nInputs=0;nInputs<op->GetInputCount() && op->GetInput(nInputs);nInputs++);

    if(nInputs == 1 && SkipNops(op->GetInput(0))->SaveId == i-1) // linked?
    {
      *data++ = typeByte | 0x80;
    }
    else
    {
      *data++ = typeByte;
      if(op->Class->MinInput != op->Class->MaxInput)
        *data++ = nInputs;
      else
        sVERIFY(nInputs == op->Class->MinInput);

      for(in=0;in<nInputs;in++)
      {
        link = SkipNops(op->GetInput(in))->SaveId;
        sVERIFY(link < i);
        sWriteShort(data,i - 1 - link);
      }
    }
    
    // count size
    OpSize[op->Op.Command].Graph += data - dstart;
    OpSize[op->Op.Command].Count++;
    OpSize[op->Op.Command].LastOp = op;
  }

  for(i=0;i<ExportSet->GetCount();i++)
  {
    op = ExportSet->Get(i);
    for(in=0;in<OPC_GETLINK(op->Op.Convention);in++)
    {
      link = op->GetLink(in) ? SkipNops(op->GetLink(in))->SaveId+1 : 0;
      sWriteShort(data,link);
    }
  }

  //sSystem->SaveFile("exp\\graph.dat",gstart,data-gstart);

  // write ops sorted by type
  for(i=0;i<OpTypeCount;i++)
  {
    sBool found=sFALSE;

    for(in=0;in<ExportSet->GetCount();in++)
    {
      op = ExportSet->Get(in);
      if(op->Class->Id == OpTypes[i])
      {
        if(!found)
        {
          OpSize[op->Op.Command].DStart = data;
          found = sTRUE;
        }

        // write data
        dstart = data;
        pack = op->Class->Packing;

        for(j=0;pack[j];j++)
        {
          typeByte = pack[j];
          if(typeByte >= 'A' && typeByte <= 'Z')
            typeByte += 0x20;

          switch(typeByte)
          {
          case 'g': sWriteF16(data,*op->Op.GetEditPtrF(j));                break;
          case 'f': sWriteF24(data,*op->Op.GetEditPtrF(j));                break;
          case 'e': sWriteX16(data,*op->Op.GetEditPtrF(j));                break;
          case 'i': *((sS32 *) data) = *op->Op.GetEditPtrS(j); data += 4;  break;
          case 's': *((sS16 *) data) = *op->Op.GetEditPtrS(j); data += 2;  break;
          case 'b': *data++ = *op->Op.GetEditPtrU(j);                      break;
          case 'c': *((sU32 *) data) = *op->Op.GetEditPtrU(j); data += 4;  break;
          case 'm': *((sU32 *) data) = *op->Op.GetEditPtrU(j); data += 3;  break;
          }
        }

        for(j=0;j<OPC_GETSTRING(op->Op.Convention);j++)
        {
          if(!op->Op.GetString(j))
            *data++ = 0;
          else
          {
            sCopyMem(data,op->Op.GetString(j),sGetStringLen(op->Op.GetString(j))+1);
            data += sGetStringLen(op->Op.GetString(j))+1;
          }
        }

        for(j=0;j<OPC_GETSPLINE(op->Op.Convention);j++)
        {
          *((sU16 *) data) = op->Op.GetSpline(j) ? op->Op.GetSpline(j)->SaveId+1 : 0;
          data += 2;
        }

        if(op->Op.Convention & OPC_BLOB)
        {
          *((sU32 *) data) = op->Op.GetBlobSize();
          data+=4;
        }

        // count size (for statistics)
        OpSize[op->Op.Command].Data += data - dstart;
      }
    }
  }

  // write anim bytecode
  dstart = data;
  for(i=0;i<ExportSet->GetCount();i++)
  {
    op = ExportSet->Get(i);
    bytecode = op->Op.GetAnimCode();

    do
    {
      j = *bytecode;
      switch(j)
      {
      case KA_SPLINE:
        *data++ = *bytecode++;
        *((sU16 *) data) = Doc->EnvKSplines[*((sU16 *) bytecode)]->SaveId;
        data += 2;
        bytecode += 2;
        break;

      default:
        k = CalcCmdSize(bytecode);
        sCopyMem(data,bytecode,k);
        data += k;
        bytecode += k;
      }
    }
    while(j != KA_END);
  }

  // write events
  sVERIFY(eventCount < 32768);
  sWriteShort(data,eventCount);
  for(i=0;i<Doc->Events->GetCount();i++)
  {
    event = Doc->Events->Get(i);
    if(ExportSet->IsInside(SkipNops(event->Op)))
    {
      // op id
      sWriteShort(data,SkipNops(event->Op)->SaveId);

      // just copy the other fields
      *((sU32 *) data) = event->Event.Start; data += 4;
      *((sU32 *) data) = event->Event.End; data += 4;
      sWriteF24(data,event->Event.Velocity);
      sWriteF24(data,event->Event.Modulation);
      *data++ = event->Event.Select;
      sWriteF24(data,event->Event.Scale.x);
      sWriteF24(data,event->Event.Scale.y);
      sWriteF24(data,event->Event.Scale.z);
      sWriteF24(data,event->Event.Rotate.x);
      sWriteF24(data,event->Event.Rotate.y);
      sWriteF24(data,event->Event.Rotate.z);
      sWriteF24(data,event->Event.Translate.x);
      sWriteF24(data,event->Event.Translate.y);
      sWriteF24(data,event->Event.Translate.z);
      *((sU32 *) data) = event->Event.Color; data += 4;
      sWriteShort(data,event->Event.Spline ? event->Event.Spline->SaveId+1 : 0);
      sWriteF16(data,event->Event.StartInterval);
      sWriteF16(data,event->Event.EndInterval);
    }
  }

  // write splines
  for(i=0;i<SplineSet->Count;i++)
  {
    spline = SplineSet->Get(i);
    sVERIFY(spline->Count <= 4);

    *data++ = spline->Count | (spline->Interpolation << 3);
    
    for(j=0;j<spline->Count;j++)
    {
      sWriteShort(data,spline->Channel[j].KeyCount);
      for(k=0;k<spline->Channel[j].KeyCount;k++)
      {
        *data++ = spline->Channel[j].Keys[k].Time * 255;
        /**((sU16 *) data) = spline->Channel[j].Keys[k].Time * 32768;
        data += 2;*/
        sWriteF16(data,spline->Channel[j].Keys[k].Value);
        /**data++ = spline->Channel[j].Keys[k].Time * 255;
        sWriteF16(data,spline->Channel[j].Keys[k].Value);*/
      }
    }
  }

  // write blobs (for .kk export)
  for(i=0;i<ExportSet->GetCount();i++)
  {
    op = ExportSet->Get(i);
    if(op->Op.GetBlobSize())
    {
      sCopyMem(data,op->Op.GetBlobData(),op->Op.GetBlobSize());
      data += op->Op.GetBlobSize();
    }
  }

  dataPtr = data;
  delete ExportSet;
  SplineSet->Exit();
  delete SplineSet;

  // print export information
  total.Graph = 0;
  total.Data = 0;
  total.Count = 0;
  total.LastOp = 0;
  for(i=0;i<MAXOPID;i++)
  {
    total.Graph += OpSize[i].Graph;
    total.Data += OpSize[i].Data;
    total.Count += OpSize[i].Count;
  }

  sDPrintF("export statistics\n\n");
  sDPrintF("%-20s | %-5s | %-5s | %-5s | %7s\n","Op","Count","SizeG","SizeD","AvgSize");
  sDPrintF("---------------------+-------+-------+-------+--------\n");
  
  sU8 *everything,*eptr;
  everything = new sU8[256*1024];
  eptr = everything;

  for(i=0;i<MAXOPID;i++)
  {
    if(OpSize[i].Count)
    {
      sz = 100*(OpSize[i].Graph+OpSize[i].Data)/OpSize[i].Count;

      sDPrintF("%-20s | %5d | %5d | %5d | %3d.%02d\n",
        OpSize[i].LastOp->Class->Name,OpSize[i].Count,OpSize[i].Graph,
        OpSize[i].Data,sz/100,sz%100);

      sChar buffer[256];
      sSPrintF(buffer,sizeof(buffer),"exp\\%02x_%s.dat",i,OpSize[i].LastOp->Class->Name);
      //sSystem->SaveFile(buffer,OpSize[i].DStart,OpSize[i].Data);

      sCopyMem(eptr,OpSize[i].DStart,OpSize[i].Data);
      eptr += OpSize[i].Data;
    }
  }

  sz = 100*(total.Graph+total.Data)/total.Count;
  sDPrintF("%-20s | %5d | %5d | %5d | %3d.%02d\n","Total",total.Count,
    total.Graph,total.Data,sz/100,sz%100);

  //sSystem->SaveFile("exp\\everything.dat",everything,eptr-everything);
  delete[] everything;

  return sTRUE;
}

/****************************************************************************/
/***                                                                      ***/
/***   classes                                                            ***/
/***                                                                      ***/
/****************************************************************************/


/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/

sU32 WerkColors[] = 
{
  0xffc0c0c0,   // 0    KC_NULL
  0xffc0ffff,   // 1    KC_BITMAP
  0xffc000ff,   // 2    KC_MINMESH
  0xffffffa0,   // 3    KC_SCENE
  0xff404040,   // 4    
  0xffc0ffc0,   // 5    KC_MATERIAL
  0xffffc0ff,   // 6    KC_MESH
  0xff404040,   // 7    KC_PAINTER
  0xffffa070,   // 8    KC_IPP
  0xffc0c0c0,   // 9    KC_EFFECT
  0xff404040,   // 10
  0xffc0c0c0,   // 11   KC_DEMO
  0xff404040,   // 12
  0xff404040,   // 13
  0xff404040,   // 14
  0xff404040,   // 15
};


/****************************************************************************/
/****************************************************************************/

WerkClass *FindWerkClass(sInt id)
{
  sInt i;

  for(i=0;WerkClasses[i].Name;i++)
  {
    if(WerkClasses[i].Id==id)
      return &WerkClasses[i];
  }
  return 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   File-Browser Support for Operator-Stores                           ***/
/***                                                                      ***/
/****************************************************************************/

class DIWerkOp : public sDiskItem
{
public:
  WerkOp *Op;

  DIWerkOp();
  void Tag();
  virtual sBool Init(sChar *,sDiskItem *,void *);
  virtual sBool GetAttr(sInt attr,void *d,sInt &s);
  virtual sBool SetAttr(sInt attr,void *d,sInt s);
  virtual sInt Cmd(sInt cmd,sChar *s,sDiskItem *d);
  virtual sBool Support(sInt id);
};

DIWerkOp::DIWerkOp()
{
  Op = 0;
}

void DIWerkOp::Tag()
{
  sDiskItem::Tag();
  sBroker->Need(Op);
}

sBool DIWerkOp::Init(sChar *name,sDiskItem *parent,void *op)
{
  Parent = parent;
  Op = (WerkOp *)op;
  return sTRUE;
}

sBool DIWerkOp::GetAttr(sInt attr,void *data,sInt &size)
{
  sVERIFY(Op);
  switch(attr)
  {
  case sDIA_NAME:
    sCopyString((sChar *)data,Op->Name,size);
    return sTRUE;
  case sDIA_ISDIR:
    *(sInt*)data = 0;
    return sTRUE;
  default:
    return sFALSE;
  }
}

sBool DIWerkOp::SetAttr(sInt attr,void *d,sInt s)
{
  return sFALSE;
}

sInt DIWerkOp::Cmd(sInt cmd,sChar *s,sDiskItem *d)
{
  return 0;
}

sBool DIWerkOp::Support(sInt id)
{
  switch(id)
  {
  case sDIA_NAME:
  case sDIA_ISDIR:
    return sTRUE;
  default:
    return sFALSE;
  }
}

/****************************************************************************/

class DIWerkPage : public sDiskItem
{
public:
  WerkPage *Page;
  sInt Complete;

  DIWerkPage();
  void Tag();
  virtual sBool Init(sChar *,sDiskItem *,void *);
  virtual sBool GetAttr(sInt attr,void *d,sInt &s);
  virtual sBool SetAttr(sInt attr,void *d,sInt s);
  virtual sInt Cmd(sInt cmd,sChar *s,sDiskItem *d);
  virtual sBool Support(sInt id);
};

DIWerkPage::DIWerkPage()
{
  Page = 0;
  Complete = 0;
}

void DIWerkPage::Tag()
{
  sDiskItem::Tag();
  sBroker->Need(Page);
}

sBool DIWerkPage::Init(sChar *name,sDiskItem *parent,void *page)
{
  Parent = parent;
  Page = (WerkPage *)page;
  return sTRUE;
}

sBool DIWerkPage::GetAttr(sInt attr,void *data,sInt &size)
{
  sVERIFY(Page);
  switch(attr)
  {
  case sDIA_NAME:
    sCopyString((sChar *)data,Page->Name,size);
    return sTRUE;
  case sDIA_ISDIR:
    *(sInt*)data = 1;
    return sTRUE;
  case sDIA_INCOMPLETE:
    *(sInt*)data = Complete ? 0 : 1;
    return sTRUE;
  default:
    return sFALSE;
  }
}

sBool DIWerkPage::SetAttr(sInt attr,void *d,sInt s)
{
  return sFALSE;
}

sInt DIWerkPage::Cmd(sInt cmd,sChar *s,sDiskItem *d)
{
  sInt i;
  sDiskItem *opdi;
  WerkOp *op;

  sVERIFY(Page);

  switch(cmd)
  {
  case sDIC_FINDALL:
  case sDIC_FIND:
    if(!Complete)
    {
      for(i=0;i<Page->Ops->GetCount();i++)
      {
        op =  Page->Ops->Get(i);
        if(op->Name[0])
        {
          opdi = new DIWerkOp;
          opdi->Init(op->Name,this,op);
          List->Add(opdi);
        }
      }
      Sort();
      Complete = 1;
    }
    break;
  }
  return 0;
}

sBool DIWerkPage::Support(sInt id)
{
  switch(id)
  {
  case sDIA_NAME:
  case sDIA_ISDIR:
  case sDIA_INCOMPLETE:
  case sDIC_FIND:
  case sDIC_FINDALL:
    return sTRUE;
  default:
    return sFALSE;
  }
}

/****************************************************************************/

//0123456789
static sChar DIWerkAlphaString[] = "abcdefghijklmnopqrstuvwxyz_";

class DIWerkAlpha : public sDiskItem
{
public:
  WerkkzeugApp *App;
  sInt Complete;
  sChar Letter;

  DIWerkAlpha();
  void Tag();
  virtual sBool Init(sChar *,sDiskItem *,void *);
  virtual sBool GetAttr(sInt attr,void *d,sInt &s);
  virtual sBool SetAttr(sInt attr,void *d,sInt s);
  virtual sInt Cmd(sInt cmd,sChar *s,sDiskItem *d);
  virtual sBool Support(sInt id);
};

DIWerkAlpha::DIWerkAlpha()
{
  App = 0;
  Complete = 0;
  Letter = 0;
}

void DIWerkAlpha::Tag()
{
  sDiskItem::Tag();
  sBroker->Need(App);
}

sBool DIWerkAlpha::Init(sChar *name,sDiskItem *parent,void *app)
{
  Parent = parent;
  App = (WerkkzeugApp *)app;
  Letter = *name;
  return sTRUE;
}

sBool DIWerkAlpha::GetAttr(sInt attr,void *data,sInt &size)
{
  sVERIFY(App);
  switch(attr)
  {
  case sDIA_NAME:
    if(Letter)
    {
      sVERIFY(size>=2);
      ((sChar *)data)[0] = Letter;
      ((sChar *)data)[1] = 0;
    }
    else
      sCopyString((sChar *)data,"other",size);
    return sTRUE;
  case sDIA_ISDIR:
    *(sInt*)data = 1;
    return sTRUE;
  case sDIA_INCOMPLETE:
    *(sInt*)data = Complete ? 0 : 1;
    return sTRUE;
  default:
    return sFALSE;
  }
}

sBool DIWerkAlpha::SetAttr(sInt attr,void *d,sInt s)
{
  return sFALSE;
}

sInt DIWerkAlpha::Cmd(sInt cmd,sChar *s,sDiskItem *d)
{
  sInt i,j,k,c;
  sDiskItem *opdi;
  WerkPage *page;
  WerkOp *op;

  sVERIFY(App);

  switch(cmd)
  {
  case sDIC_FINDALL:
  case sDIC_FIND:
    if(!Complete)
    {
      for(i=0;i<App->Doc->Pages->GetCount();i++)
      {
        page = App->Doc->Pages->Get(i);
        for(j=0;j<page->Ops->GetCount();j++)
        {
          op = page->Ops->Get(j);
          if(op->Name[0])
          {
            c = op->Name[0];
            if(c>='A' && c<='Z') c = c - 'A' + 'a';
            if(Letter==0)
            {
              for(k=0;DIWerkAlphaString[k];k++)
              {
                if(c==DIWerkAlphaString[k])
                {
                  c = 0;
                }
              }
              c = !c;
            }
            if(c==Letter)
            {
              opdi = new DIWerkOp;
              opdi->Init(op->Name,this,op);
              List->Add(opdi);
            }
          }
        }
      }
      Sort();
      Complete = 1;
    }
    break;
  }
  return 0;
}

sBool DIWerkAlpha::Support(sInt id)
{
  switch(id)
  {
  case sDIA_NAME:
  case sDIA_ISDIR:
  case sDIA_INCOMPLETE:
  case sDIC_FIND:
  case sDIC_FINDALL:
    return sTRUE;
  default:
    return sFALSE;
  }
}

/****************************************************************************/

class DIWerkDot : public sDiskItem
{
public:
  WerkkzeugApp *App;
  sInt Complete;
  sChar Name[KK_NAME];
  sChar Path[KK_NAME];

  DIWerkDot();
  void Tag();
  virtual sBool Init(sChar *,sDiskItem *,void *);
  virtual sBool GetAttr(sInt attr,void *d,sInt &s);
  virtual sBool SetAttr(sInt attr,void *d,sInt s);
  virtual sInt Cmd(sInt cmd,sChar *s,sDiskItem *d);
  virtual sBool Support(sInt id);
};

DIWerkDot::DIWerkDot()
{
  App = 0;
  Complete = 0;
  Path[0] = 0;
  Name[0] = 0;
}

void DIWerkDot::Tag()
{
  sDiskItem::Tag();
  sBroker->Need(App);
}

sBool DIWerkDot::Init(sChar *name,sDiskItem *parent,void *app)
{
  sChar *s;

  Parent = parent;
  App = (WerkkzeugApp *)app;

  sCopyString(Path,name,KK_NAME);
  s = name;
  while(*name)
  {
    if(*name=='.')
      s = name+1;
    name++;
  }
  sCopyString(Name,s,KK_NAME);

  return sTRUE;
}

sBool DIWerkDot::GetAttr(sInt attr,void *data,sInt &size)
{
  sVERIFY(App);
  switch(attr)
  {
  case sDIA_NAME:
    if(Name[0]==0)
      sCopyString((sChar *)data,"By Dot",size);
    else
      sCopyString((sChar *)data,Name,size);
    return sTRUE;
  case sDIA_ISDIR:
    *(sInt*)data = 1;
    return sTRUE;
  case sDIA_INCOMPLETE:
    *(sInt*)data = Complete ? 0 : 1;
    return sTRUE;
  default:
    return sFALSE;
  }
}

sBool DIWerkDot::SetAttr(sInt attr,void *d,sInt s)
{
  return sFALSE;
}

sInt DIWerkDot::Cmd(sInt cmd,sChar *s,sDiskItem *d)
{
  sInt i,j,k;
  sInt len,path;
  WerkPage *page;
  WerkOp *op;
  DIWerkDot *wd;
  DIWerkOp *wo;
  sChar dotname[KK_NAME];
  sList<DIWerkDot> *Dots;

  sVERIFY(App);

  switch(cmd)
  {
  case sDIC_FINDALL:
  case sDIC_FIND:
    if(!Complete)
    {
      Dots = new sList<DIWerkDot>;
      len = sGetStringLen(Path);
      if(len==0)
        len = -1;
      for(i=0;i<App->Doc->Pages->GetCount();i++)
      {
        page = App->Doc->Pages->Get(i);
        for(j=0;j<page->Ops->GetCount();j++)
        {
          op = page->Ops->Get(j);
          if(op->Name[0]==0)
            continue;
          if(len>=0)
          {
            if(sCmpMem(Path,op->Name,len)!=0)
              continue;
            if(op->Name[len]!='.' && len!=0)
              continue;
          }
          path = -1;

          for(k=0;op->Name[len+1+k] && path==-1;k++)
            if(op->Name[len+1+k]=='.')
              path = k;

          if(path!=-1)
          {
            sCopyString(dotname,op->Name,path+len+1);
            for(k=0;k<Dots->GetCount();k++)
              if(sCmpString(dotname,Dots->Get(k)->Path)==0)
                goto alreadythere;
            wd = new DIWerkDot;
            wd->Init(dotname,this,App);
            List->Add(wd);
            Dots->Add(wd);
          }
          else
          {
            wo = new DIWerkOp;
            wo->Init(op->Name,this,op);
            List->Add(wo);
          }
        alreadythere:;
        }
      }
      Sort();
      Complete = 1;
      delete Dots;
    }
    break;
  }
  return 0;
}

sBool DIWerkDot::Support(sInt id)
{
  switch(id)
  {
  case sDIA_NAME:
  case sDIA_ISDIR:
  case sDIA_INCOMPLETE:
  case sDIC_FIND:
  case sDIC_FINDALL:
    return sTRUE;
  default:
    return sFALSE;
  }
}

/****************************************************************************/

void WerkkzeugApp::OpBrowserOff()
{
  if(OpBrowserWin)
  {
    OpBrowserWin->GetPath(OpBrowserPath,sDI_PATHSIZE);
    OpBrowserWin->KillMe();
    OpBrowserWin = 0;
  }
}

void WerkkzeugApp::OpBrowser(sGuiWindow *sendto,sU32 cmd)
{
  sDIFolder *root,*sub;
  DIWerkPage *dipage;
  DIWerkAlpha *dialpha;
  DIWerkDot *didot;
  WerkPage *page;
  sInt i;

  root = new sDIFolder;
  root->Init("Operators",0,0);

// by page

  sub = new sDIFolder;  sub->Init("By Page",root,0); root->Add(sub);
  for(i=0;i<Doc->Pages->GetCount();i++)
  {
    page = Doc->Pages->Get(i);
    dipage = new DIWerkPage;
    dipage->Init(page->Name,sub,page);
    sub->Add(dipage);
  }

// by dot notation

  didot = new DIWerkDot;  didot->Init("",root,this);  root->Add(didot);

// by alphabet

  sub = new sDIFolder;  sub->Init("By Name",root,0); root->Add(sub);
  for(i=0;i<DIWerkAlphaString[i];i++)
  {
    dialpha = new DIWerkAlpha;
    dialpha->Init(DIWerkAlphaString+i,sub,this);
    sub->Add(dialpha);
  }
  dialpha = new DIWerkAlpha;
  dialpha->Init("",sub,this);
  sub->Add(dialpha);

// display browser

  OpBrowserOff();
  OpBrowserWin = new sBrowserApp();
  OpBrowserWin->AddTitle("Find an Op!",0,sBWCMD_EXIT);
  OpBrowserWin->Position.Init(0,0,500,300);
  OpBrowserWin->SetRoot(root);
  OpBrowserWin->SendTo = sendto;
  OpBrowserWin->DoubleCmd = cmd;
  OpBrowserWin->FileCmd = CMD_BROWSERPREVIEW;
  OpBrowserWin->Flags |= sGWF_AUTOKILL;
  OpBrowserWin->SetPath("By Dot");
  OpBrowserWin->SetPath(OpBrowserPath);
  sGui->AddPopup(OpBrowserWin);
  OpBrowserWin->TreeFocus();
}

/****************************************************************************/

void WerkkzeugApp::FindResults(sBool toggle)
{
  if(toggle == ShowFindResults)
    return;

  if(toggle)
    TopSplit->SplitChild(1,FindResultsWin);
  else
    TopSplit->RemoveSplit(FindResultsWin);

  ShowFindResults = toggle;
  Flags |= sGWF_LAYOUT;
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   New Document Objects                                               ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

void sMatrixEdit::Init()
{
  sSetMem(this,0,sizeof(*this));
  EditMode = 1;
}

/****************************************************************************/

WerkEvent2Anim::WerkEvent2Anim()
{
  for(sInt i=0;i<4;i++)
    Splines[i] = 0;
  Offset = 0;
  AnimType = 0;
}

WerkEvent2Anim::~WerkEvent2Anim()
{
}

void WerkEvent2Anim::Tag()
{
  sObject::Tag();
  for(sInt i=0;i<4;i++)
    sBroker->Need(Splines[i]);
}

/****************************************************************************/

WerkEvent2::WerkEvent2()
{
  Source = new sText;
  CompilerErrors = new sText;
  Splines = new sList<WerkSpline>;
  Exports.Init();
  Anims = new sList<WerkEvent2Anim>;
  Recompile = 1;
  AliasName[0] = 0;
  AliasEvent = 0;
  Clear();
}

WerkEvent2::~WerkEvent2()
{
  Exports.Exit();
  Event.Exit();
}

void WerkEvent2::Tag()
{
  sInt i;

  sObject::Tag();
  sBroker->Need(Doc);
  for(i=0;i<K2MAX_LINK;i++)
  {
    sBroker->Need(LinkOps[i]);
    sBroker->Need(LinkScenes[i]);
  }
  sBroker->Need(Source);
  sBroker->Need(CompilerErrors);
  sBroker->Need(Splines);
  sBroker->Need(Anims);
  sBroker->Need(AliasEvent);
}

void WerkEvent2::Clear()
{
  sInt i;

  Recompile = 1;
  Line = 0;
  LoopFlag = 0;
  Doc = 0;
  Select = 0;
  Name[0] = 0;
  sSetMem(&Event,0,sizeof(Event));
  for(i=0;i<K2MAX_LINK;i++)
  {
    LinkName[i][0] = 0;
    LinkOps[i] = 0;
    LinkScenes[i] = 0;
  }
  Source->Init("// type your script here...");
}

void WerkEvent2::Copy(sObject *o)
{
  WerkEvent2 *we;

  sVERIFY(o->GetClass()==sCID_WERKEVENT2);
  we = (WerkEvent2 *) o;

  Event.Init(we->Event.Effect);
  Event.Start = we->Event.Start;
  Event.End = we->Event.End;
  sCopyMem(Event.Edit,we->Event.Edit,Event.Effect->EditSize);
  sCopyMem(Event.Para,we->Event.Para,Event.Effect->ParaSize);
  Line = we->Line;
  sCopyString(Name,we->Name,KK_NAME);
  sCopyMem(LinkName,we->LinkName,sizeof(LinkName));
  Source->Copy(we->Source);
  Splines->Copy(we->Splines);
  Doc = we->Doc;
  Recompile = 1;
}

void WerkEvent2::UpdateLinks(WerkDoc2 *doc2,WerkDoc *doc1)
{
  sInt i;

  for(i=0;i<K2MAX_LINK;i++)
  {
    LinkOps[i] = 0;
    LinkScenes[i] = 0;
    Event.Links[i] = 0;
    if(LinkName[i][0] && Event.Effect->Types[i]==KC_SCENE2)
    {
      LinkScenes[i] = doc2->FindScene(LinkName[i]);
    }
    else if(LinkName[i][0] && Event.Effect->Types[i])
    {
      LinkOps[i] = doc1->FindName(LinkName[i]);
    }
  }
  AliasEvent = doc2->FindEvent(AliasName);
  if(AliasEvent)
    Event.Alias = &AliasEvent->Event;
}

void WerkEvent2::Calc(KEnvironment *kenv1)
{
  sInt i;
  for(i=0;i<K2MAX_LINK;i++)
  {
    if(LinkOps[i])
    {
      LinkOps[i]->Op.Calc(kenv1,KCF_NEED);
      Event.Links[i] = LinkOps[i]->Op.Cache;
    }
    else if(LinkScenes[i])
    {
      LinkScenes[i]->CalcScene(kenv1);
      Event.Links[i] = LinkScenes[i]->Root;
    }
  }
}

sBool WerkEvent2::CheckError()
{
  sInt i;

  if(Event.Effect==0)
    return 0;
  for(i=0;i<K2MAX_LINK;i++)
  {
    if(Event.Effect->Types[i]==0)
      continue;
    if(LinkName[i][0] && !(LinkOps[i] || LinkScenes[i]))
      return 1;
  }
  if(AliasName[0] && AliasEvent==0)
    return 1;
  return 0;
}

void WerkEvent2::AddSymbol(const sChar *name,sInt id,sInt type)
{
  KEffect2Symbol *ex;

  ex = Exports.Add();
  sCopyString(ex->Name,name,KK_NAME);
  ex->Id = id;
  ex->Type = type;
}

void WerkEvent2::AddAnimCode()
{
  sChar *s;
  WerkEvent2Anim *anim;
  KEffect2Symbol *sym;
//  KEffectInfo *info;
  const sChar *borderstring = "// police line, do not cross!\n";
//  sVERIFY(Source->Text);
  s = (sChar *)sFindString(Source->Text,borderstring);
  if(s) *s = 0;
  Source->FixUsed();
  if(Anims->GetCount()>0)
    Source->Print(borderstring);
  sFORLIST(Anims,anim)
  {
//    info = Event.Effect->Info;
    sym = 0;
    for(sInt i=0;i<Exports.Count && !sym;i++)
    {
      if(Exports[i].Id==anim->Offset)
        sym = &Exports[i];
    }

    if(sym)
    {
      switch(anim->AnimType)
      {
      case EAT_SCALAR:
        Source->PrintF("%s = eval(time,%s).x;\n",sym->Name,anim->Splines[0]->Name);
        break;
      case EAT_VECTOR:
      case EAT_COLOR:
        Source->PrintF("%s = eval(time,%s);\n",sym->Name,anim->Splines[0]->Name);
        break;
      case EAT_EULER:
        Source->PrintF("%s = euler(eval(time,%s));\n",sym->Name,anim->Splines[0]->Name);
        Source->PrintF("%s.l = eval(time,%s);\n",sym->Name,anim->Splines[1]->Name);
        break;
      case EAT_SRT:
        Source->PrintF("%s = euler(eval(time,%s)) * scale(eval(time,%s));\n",
          sym->Name,anim->Splines[0]->Name,anim->Splines[1]->Name);
        Source->PrintF("%s.l = eval(time,%s);\n",
          sym->Name,anim->Splines[2]->Name);
        break;
      case EAT_TARGET:
        Source->PrintF("%s = lookat(eval(time,%s))-eval(time,%s));\n",
          sym->Name,anim->Splines[0]->Name,anim->Splines[1]->Name);
        Source->PrintF("%s.l = eval(time,%s);\n",
          sym->Name,anim->Splines[1]->Name);
        break;
      default:
        sVERIFYFALSE;
      }
    }
    else
    {
      Source->PrintF("// unknown animation offset %d;\n",anim->Offset);
    }
  }
}

void WerkEvent2::AddAnim(sInt offset,sInt type,const sF32 *data,const sChar *scenename)
{
  WerkEvent2Anim *anim;
  WerkSpline *spline;
  sChar name[32];
  sInt len;
  const sF32 f0[4] = {0,0,0,0};
  const sF32 f1[4] = {1,1,1,1};
  sSPrintF(name,32,"Spline%08X",sGetRnd());
  len = sGetStringLen(name);
  name[len+0] = 0;
  name[len+1] = 0;
  name[len+2] = 0;
  if(data==0)
    data = f0;

  anim = new WerkEvent2Anim;
  anim->AnimType = type;
  anim->Offset = offset;
  sCopyString(anim->SceneNode,scenename,KK_NAME);
  
  switch(type)
  {
  case EAT_SCALAR:
    spline = new WerkSpline;
    spline->InitDef(1,name,0,data,0);
    Splines->Add(spline);
    anim->Splines[0] = spline;
    break;

  case EAT_VECTOR:
    spline = new WerkSpline;
    spline->InitDef(4,name,0,data,0);
    Splines->Add(spline);
    anim->Splines[0] = spline;
    break;

  case EAT_EULER:
    spline = new WerkSpline;
    name[len] = '_';
    name[len+1] = 'r';
    spline->InitDef(3,name,WSK_ROTATE,f0,0);
    Splines->Add(spline);
    anim->Splines[0] = spline;

    spline = new WerkSpline;
    name[len] = '_';
    name[len+1] = 't';
    spline->InitDef(3,name,WSK_TRANSLATE,data+12,0);
    Splines->Add(spline);
    anim->Splines[1] = spline;
    break;

  case EAT_SRT:
    spline = new WerkSpline;
    name[len] = '_';
    name[len+1] = 's';
    spline->InitDef(3,name,WSK_SCALE,f1,0);
    Splines->Add(spline);
    anim->Splines[0] = spline;

    spline = new WerkSpline;
    name[len] = '_';
    name[len+1] = 'r';
    spline->InitDef(3,name,WSK_ROTATE,f0,0);
    Splines->Add(spline);
    anim->Splines[1] = spline;

    spline = new WerkSpline;
    name[len] = '_';
    name[len+1] = 't';
    spline->InitDef(3,name,WSK_TRANSLATE,data+12,0);
    Splines->Add(spline);
    anim->Splines[2] = spline;
    break;

  case EAT_TARGET:
    spline = new WerkSpline;
    name[len] = '_';
    name[len+1] = 't';
    spline->InitDef(3,name,WSK_TRANSLATE,f0,0);
    Splines->Add(spline);
    anim->Splines[0] = spline;

    spline = new WerkSpline;
    name[len] = '_';
    name[len+1] = 'p';
    spline->InitDef(3,name,WSK_TRANSLATE,data+12,0);
    Splines->Add(spline);
    anim->Splines[1] = spline;
    break;

  case EAT_COLOR:
    spline = new WerkSpline;
    spline->InitDef(4,name,WSK_COLOR,data,0);
    Splines->Add(spline);
    anim->Splines[0] = spline;
    break;

  default:
    sVERIFYFALSE;
  }

  Anims->Add(anim);
  Recompile = 1;
}

WerkEvent2Anim *WerkEvent2::FindAnim(sInt offset)
{
  WerkEvent2Anim *anim;
  sFORLIST(Anims,anim)
  {
    if(anim->Offset == offset)
      return anim;
  }
  return 0;
}

WerkEvent2Anim *WerkEvent2::FindAnim(const sChar *name)
{
  WerkEvent2Anim *anim;
  sFORLIST(Anims,anim)
  {
    if(sCmpString(anim->SceneNode,name)==0)
      return anim;
  }
  return 0;
}

void WerkEvent2::AddSceneExports(WerkScene2 *scene)
{
  WerkSceneNode2 *node;
  sInt id;

  sFORLIST(scene->Nodes,node)
  {
    id = Event.SceneNodes.Count*16;
    *Event.SceneNodes.Add() = node->Node;
    if(node->Name[0])
      AddSymbol(node->Name,id,sST_MATRIX);
  }
}

/****************************************************************************/

WerkDoc2::WerkDoc2()
{
  Events = new sList<WerkEvent2>;
  Scenes = new sList<WerkScene2>;
  VM = new WerkScriptVM(this);
  Compiler = new WerkScriptCompiler(this);
  Env2.Init();
  App = 0;
  Recompile = 1;
}

WerkDoc2::~WerkDoc2()
{
  delete VM;
  delete Compiler;
  Env2.Exit();
}

void WerkDoc2::Tag()
{
  sObject::Tag();
  sBroker->Need(Events);
  sBroker->Need(Scenes);
}

void WerkDoc2::Clear()
{
  Events->Clear();
  Scenes->Clear();
  Recompile = 1;
}

sBool WerkDoc2::Write(sU32 *&data)
{
  WerkEvent2 *event;
  WerkScene2 *scene;
  WerkSpline *spline;
  WerkSceneNode2 *node;
  const KEffect2 *ef;

  *data++ = 1;
  *data++ = Events->GetCount();
  *data++ = Scenes->GetCount();
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;
  *data++ = 0;
  *data++ = K2MAX_LINK;

  sFORLIST(Events,event)
  {
    ef = event->Event.Effect;
    *data++ = ef->Id;
    *data++ = ef->ParaSize;
    *data++ = ef->EditSize;
    *data++ = event->Event.Start;
    *data++ = event->Event.End;
    *data++ = event->Line;
    *data++ = event->LoopFlag;
    *data++ = event->Splines->GetCount();
    *data++ = 0;
    *data++ = 0;
    *data++ = 0;
    *data++ = 0;

    sWriteString(data,event->Name);
    sWriteString(data,event->AliasName);
    event->Source->Write(data);
    for(sInt i=0;i<K2MAX_LINK;i++)
      sWriteString(data,event->LinkName[i]);

    sCopyMem(data,event->Event.Para,ef->ParaSize); data+=(ef->ParaSize+3)/4;
    sCopyMem(data,event->Event.Edit,ef->EditSize); data+=(ef->EditSize+3)/4;

    sFORLIST(event->Splines,spline)
    {
      *data++ = spline->Kind;
      *data++ = spline->Spline.Count;
      *data++ = spline->Spline.Interpolation;
      *data++ = 0;
      sWriteString(data,spline->Name);
      for(sInt k=0;k<spline->Spline.Count;k++)
      {
        *data++ = spline->Channel[k].Keys.Count;
        *data++ = 0;
        for(sInt l=0;l<spline->Channel[k].Keys.Count;l++)
        {
          *(sF32 *)data = spline->Channel[k].Keys.Get(l).Time;  data++;
          *(sF32 *)data = spline->Channel[k].Keys.Get(l).Value; data++;
        }
      }
    }
  }

  sFORLIST(Scenes,scene)
  {
    *data++ = scene->TreeLevel;
    *data++ = scene->TreeButton;
    *data++ = scene->Nodes->GetCount();
    *data++ = 0;
    sWriteString(data,scene->Name);
    sFORLIST(scene->Nodes,node)
    {
      *data++ = node->TreeLevel;
      *data++ = node->TreeButton;
      *data++ = 0;
      *data++ = 0;
      node->Node->Matrix.Write(data);
      sWriteString(data,node->Name);
      sWriteString(data,node->MeshName);
    }
  }
  
  return sTRUE;
}

sBool WerkDoc2::Read(sU32 *&data)
{
  sInt version;
  sInt ec,sc;
  sInt maxlink;

  version = *data++;
  ec = *data++;
  sc = *data++;
  data++;
  data++;
  data++;
  data++;
  maxlink = *data++;

  if(version<1 || version>1) return sFALSE;
  if(maxlink>K2MAX_LINK) return sFALSE;

  Clear();

  for(sInt i=0;i<ec;i++)
  {
    sInt ps,es,id,spc;
    sInt fake;
    WerkEvent2 *event;
    const KEffect2 *ef;

    id = *data++;
    ps = *data++;
    es = *data++;

    event = MakeEvent(FindEffect(id));
    fake = 0;
    if(event==0)
    {
      event = MakeEvent(FindEffect("alias"));
      fake = 1;
    }
    ef = event->Event.Effect;

    event->Event.Start = *data++;
    event->Event.End = *data++;
    event->Line = *data++;
    event->LoopFlag = *data++;
    spc = *data++;
    data+=4;

    sReadString(data,event->Name,KK_NAME);
    sReadString(data,event->AliasName,KK_NAME);
    event->Source->Read(data);
    for(sInt j=0;j<maxlink;j++)
      sReadString(data,event->LinkName[j],KK_NAME);

    if(!fake) 
      sCopyMem(event->Event.Para,data,sMin(ef->ParaSize,ps)); 
    data+=(ps+3)/4;
    if(!fake) 
      sCopyMem(event->Event.Edit,data,sMin(ef->EditSize,es)); 
    data+=(es+3)/4;

    Events->Add(event);

    for(sInt j=0;j<spc;j++)
    {
      WerkSpline *spline;
      sInt kind,count,inter;
      sF32 val,time;

      spline = new WerkSpline;

      kind = *data++;
      count = *data++;
      inter = *data++;
      data++;

      spline = new WerkSpline;
      spline->Init(count,"",kind,0,inter);
      sReadString(data,spline->Name,KK_NAME);

      for(sInt k=0;k<spline->Spline.Count;k++)
      { 
        sInt keys;

        keys = *data++;
        data++;

        for(sInt l=0;l<keys;l++)
        {
          time = *(sF32 *)data; data++;
          val  = *(sF32 *)data; data++;
          spline->AddKey(k,time,val)->Select = 0;
        }
      }
      spline->Sort();
      
      event->Splines->Add(spline);
    }
  }

  for(sInt i=0;i<sc;i++)
  {
    WerkScene2 *scene;
    sInt nc;

    scene = new WerkScene2;
    scene->TreeLevel = *data++;
    scene->TreeButton = *data++;
    nc = *data++;
    data++;
    sReadString(data,scene->Name,KK_NAME);
    for(sInt j=0;j<nc;j++)
    {
      WerkSceneNode2 *node;

      node = new WerkSceneNode2;
      node->TreeLevel = *data++;
      node->TreeButton = *data++;
      data+=2;
      node->Node->Matrix.Read(data);
      sReadString(data,node->Name,KK_NAME);
      sReadString(data,node->MeshName,KK_NAME);

      scene->Nodes->Add(node);
    }
    Scenes->Add(scene);
  }

  return sTRUE;
}


void WerkDoc2::SortEvents()
{
  sInt i,j;
  sInt max;
  WerkEvent2 *wi,*wj;

  max = Events->GetCount();

  for(i=0;i<max-1;i++)
  {
    for(j=i+1;j<max;j++)
    {
      wi = Events->Get(i);
      wj = Events->Get(j);

      if((wi->Line>wj->Line) ||
         (wi->Line==wj->Line && wi->Event.End>wj->Event.End))
      {
        wi->Recompile = 1;
        wj->Recompile = 1;
        Recompile = 1;
        Events->Swap(i,j);
      }
    }
  }

  for(i=0;i<max;i++)
  {
    wi = Events->Get(i);    
    wi->Event.LoopEnd = wi->Event.End;
  }

  for(i=0;i<max-1;i++)
  {
    wi = Events->Get(i);    
    wj = Events->Get(i+1);
    if(wi->LoopFlag && wi->Line == wj->Line)
      wi->Event.LoopEnd = wj->Event.Start;
  }
}

WerkEvent2 *WerkDoc2::FindEvent(const sChar *name)
{
  WerkEvent2 *event;

  if(name==0 || name[0]==0) return 0;
  sFORLIST(Events,event)
  {
    if(sCmpString(event->Name,name)==0)
      return event;
  }

  return 0;
}

WerkScene2 *WerkDoc2::FindScene(const sChar *name)
{
  WerkScene2 *scene;

  if(name==0 || name[0]==0) return 0;

  sFORLIST(Scenes,scene)
  {
    if(sCmpString(scene->Name,name)==0)
      return scene;
  }
  return 0;
}


void WerkDoc2::UpdateLinks(WerkDoc *doc1)
{
  WerkEvent2 *event;
  WerkScene2 *scene;
  WerkSceneNode2 *node;

  
  sFORLIST(Events,event)
  {
    event->UpdateLinks(this,doc1);
  }

  sFORLIST(Scenes,scene)
  {
    scene->UpdateLinks();
    sFORLIST(scene->Nodes,node)
    {
      node->UpdateLinks(doc1);
    }
  }
}

void WerkDoc2::CalcEvents(sInt time)
{
  WerkEvent2 *event;

  sFORLIST(Events,event)
  {
    if(time>=event->Event.Start && time<event->Event.End)
      event->Calc(App->Doc->KEnv);
  }
}

KEffect2 *WerkDoc2::FindEffect(const sChar *name)
{
  for(sInt i=0;EffectList[i].Name;i++)
    if(sCmpString(EffectList[i].Name,name)==0)
      return &EffectList[i];
  return 0;
}

KEffect2 *WerkDoc2::FindEffect(sInt id)
{
  for(sInt i=0;EffectList[i].Name;i++)
    if(EffectList[i].Id==id)
      return &EffectList[i];
  return 0;
}

WerkEvent2 *WerkDoc2::MakeEvent(KEffect2 *ef)
{
  WerkEvent2 *ev;

  ev = new WerkEvent2();
  ev->Event.Init(ef);
  ev->Doc = App->Doc2;
  sCopyMem(ev->Event.Para,ev->Event.Effect->Info->DefaultPara,ev->Event.Effect->ParaSize);
  sCopyMem(ev->Event.Edit,ev->Event.Effect->Info->DefaultEdit,ev->Event.Effect->EditSize);

  return ev;
}

/****************************************************************************/

WerkScene2::WerkScene2()
{
  Name[0] = 0;
  TreeLevel = 0;
  TreeButton = 0;
  Nodes = new sList<WerkSceneNode2>;
  Recompile = 1;

  Root = new KSceneNode2;
}

WerkScene2::~WerkScene2()
{
  sRelease(Root);
}

void WerkScene2::Tag()
{
  sObject::Tag();
  sBroker->Need(Nodes);
}

void WerkScene2::Copy(sObject *o)
{
  WerkScene2 *ws;
  WerkSceneNode2 *node,*cn;

  sVERIFY(o->GetClass()==sCID_WERKSCENE2);
  ws = (WerkScene2*) o;

  TreeLevel = ws->TreeLevel;
  sCopyString(Name,ws->Name,KK_NAME);
  sFORLIST(ws->Nodes,node)
  {
    cn = new WerkSceneNode2;
    cn->Copy(node);
    Nodes->Add(cn);
  }
  Recompile = 1;
  UpdateLinks();
}

void WerkScene2::UpdateLinks()
{
  sInt l,i;
  KSceneNode2 **nodes[32];
  WerkSceneNode2 *node;

  nodes[0] = &Root->First;
  Root->First = 0;
  Root->Next = 0;
  Root->Mesh = 0;
  sFORLIST(Nodes,node)
  {
    node->Node->BackLink = node;
    node->Node->Next = 0;
    node->Node->First = 0;
    node->HirarchicError = 1;
    l = node->TreeLevel;
    if(l+1>=32) continue;             // error: exceed hardcoded limit
    if(nodes[l]==0) continue;         // error: no direct parent in hirarchie
    nodes[l+1] = &node->Node->First;
    for(i=l+2;i<32;i++)               // clear all levels above
      nodes[i] = 0; 
    *nodes[l] = node->Node;
    nodes[l] = &node->Node->Next;
  }
}

void WerkScene2::CalcScene(KEnvironment *env1,sInt index)
{
  UpdateLinks();
  if(index<0 || index>Nodes->GetCount())
    CalcSceneR(env1,Root);
  else
    CalcSceneR(env1,Nodes->Get(index)->Node);
}

void WerkScene2::CalcSceneR(KEnvironment *env1,KSceneNode2 *node)
{
  GenMesh *mesh;

  if(node->BackLink && node->BackLink->MeshOp)
  {
    node->BackLink->MeshOp->Op.Calc(env1,KCF_NEED);
    mesh = (GenMesh *) node->BackLink->MeshOp->Op.Cache;
    if(mesh && mesh->ClassId==KC_MESH)
    {
      mesh->Prepare();
      node->Mesh = mesh->PreparedMesh;
    }
  }
  if(node->Next)
    CalcSceneR(env1,node->Next);
  if(node->First)
    CalcSceneR(env1,node->First);
}

/****************************************************************************/
/****************************************************************************/

WerkScriptVM::WerkScriptVM(WerkDoc2 *doc)
{
  Doc = doc;
  sSetMem(Data,0,sizeof(Data));
}

void WerkScriptVM::InitFrame()
{
  ((sMatrix *)&Data[0])->Init();
  ((sVector *)&Data[16])->Init(0,0,0,1);
  ((sVector *)&Data[20])->Init(0,0,0,0);
  Data[24] = Doc->Env2.BeatTime/65536.0f;
  Data[25] = 0;
}

void WerkScriptVM::InitEvent(KEvent2 *event)
{
  sF32 s,e,t,v;

  Data[25] = 0;
  if(event)
  {
    t = Data[24];
    s = event->Start/65536;
    e = event->End/65536;
    if(s!=e)
    {
      v = (t-s)/(e-s);
      if(v<0)
        v = 0;
      Data[25] = sFMod(v,1.0f);
    }
  }
}

sBool WerkScriptVM::Extern(sInt id,sInt offset,sInt type,sF32 *data,sBool store)
{
  const static sInt sizetable[] = {1,4,16,1};
  sInt size;
  KEvent2 *ev;
  sF32 *ptr;

  sVERIFY(offset>=0);
  size = sizetable[type];
  ptr = 0;
  if(id==0)
  {
    sVERIFY(offset+type<sizeof(Data)/4);
    ptr = Data+offset;
  }
  else if(id>=0 && id-1<Doc->Env2.Events.Count)
  {
    ev = Doc->Env2.Events[id-1];
    if(type!=3)
    {
      if(ev->Effect->OnSet)
        (*ev->Effect->OnSet)(ev,&Doc->Env2,offset,data,(1<<size)-1);
    }
    else
    {
      if(offset < ev->Splines.Count)
        ptr = (sF32 *)&ev->Splines[offset];
    }
  }

  if(ptr==0)
  {
    return 0;
  }
  else
  {
    if(store)
      sCopyMem(ptr,data,size*4);
    else
      sCopyMem(data,ptr,size*4);
    return 1;
  }
}

sBool WerkScriptVM::UseObject(sInt id,void *object)
{
  sVector v;
  sF32 f;
  KSpline *spline = (KSpline *)object;

  switch(id)
  {
  case 0:                       // evaluate spline
    f = PopS();
    v.Init(0,0,0,1);
    spline->Eval(f,v);
    PushV(v);
    return 1;
  }
  return 0;
}

/****************************************************************************/

WerkScriptCompiler::WerkScriptCompiler(WerkDoc2 *doc)
{
  Doc = doc;
}

sBool WerkScriptCompiler::ExternSymbol(sChar *group,sChar *name,sU32 &groupid,sU32 &offset)
{
  WerkEvent2 *event;
  sInt i,j;

  groupid = 0;
  offset = 0;

  j = 1;
  sFORLIST(Doc->Events,event)
  {
    if(sCmpString(event->Name,group)==0)
    {
      for(i=0;i<event->Exports.Count;i++)
      {
        if(sCmpString(event->Exports[i].Name,name)==0)
        {
          groupid = j;
          offset = event->Exports[i].Id;
          return sTRUE;
        }
      }
    }
    j++;
  }

  return sFALSE;
}

/****************************************************************************/

void WerkDoc2::CompileEvents()
{
  Build();
}

void WerkDoc2::Build()
{
  WerkEvent2 *event;
  WerkScene2 *scene;
  WerkSpline *spline;
  KEffect2Symbol *es;
  sInt i,global;

  // gather recompileflags from scene to event to doc

  sFORLIST(Events,event)
  {
    for(i=0;i<K2MAX_LINK;i++)
    {
      if(event->LinkScenes[i])
        event->Recompile |= event->LinkScenes[i]->Recompile;
    }
    Recompile |= event->Recompile;
  }
  sFORLIST(Scenes,scene)
  {
    scene->Recompile = 0;
  }
  if(!Recompile) return;    // quickout...

  // get effect exports, and create effect list

  Env2.Events.Count = 0;
  sFORLIST(Events,event)
  {
    *Env2.Events.Add() = &event->Event;
    if(event->Recompile)
    {
      event->Exports.Count = 0;
      event->Event.SceneNodes.Count = 0;
      if(event->LinkScenes[0])
      {
        event->AddSceneExports(event->LinkScenes[0]);
      }
      else
      {
        for(i=0;i<event->Event.Effect->Info->Exports.Count;i++)
        {
          es = &event->Event.Effect->Info->Exports[i];
          event->AddSymbol(es->Name,es->Id,es->Type);
        }
      }
      event->Event.Splines.Count = 0;
      sFORLIST(event->Splines,spline)
      {
        event->AddSymbol(spline->Name,event->Event.Splines.Count,sST_OBJECT);
        *event->Event.Splines.Add() = &spline->Spline;
      }
    }
  }
  Recompile = 0;

  // compile

  global = 1;
  sFORLIST(Events,event)
  {
    if(event->Recompile)
    {
      CompileEvent(event,global);
      event->Recompile = 0;
    }
    global++;
  }
}

void WerkDoc2::CompileEvent(WerkEvent2 *event,sInt global)
{
  sInt i;
  KEffect2Symbol *exp;

  event->AddAnimCode();

  Compiler->ClearLocals();
  Compiler->AddLocal("unit"   ,sST_MATRIX,sSS_EXTERN|sSS_CONST,0, 0);
  Compiler->AddLocal("origin" ,sST_VECTOR,sSS_EXTERN|sSS_CONST,0,16);
  Compiler->AddLocal("null"   ,sST_VECTOR,sSS_EXTERN|sSS_CONST,0,20);
  Compiler->AddLocal("beat"   ,sST_SCALAR,sSS_EXTERN|sSS_CONST,0,24);
  Compiler->AddLocal("time"   ,sST_SCALAR,sSS_EXTERN|sSS_CONST,0,25);

  for(i=0;i<event->Exports.Count;i++)
  {
    exp = &event->Exports[i];
    Compiler->AddLocal(exp->Name,exp->Type,sSS_EXTERN,global,exp->Id);
  }

  Compiler->Compiler(&event->Event.Code,event->Source->Text,event->CompilerErrors);
}

/****************************************************************************/

WerkSceneNode2::WerkSceneNode2()
{
  Name[0] = 0;
  TreeLevel = 0;
  TreeButton = 0;

  MatrixEdit.Init();
  MeshName[0] = 0;
  ListName[0] = 0;
  MeshOp = 0;
  HirarchicError = 0;

  Node = new KSceneNode2;
}

WerkSceneNode2::~WerkSceneNode2()
{
  sRelease(Node);  
}

void WerkSceneNode2::Tag()
{
  sObject::Tag();
  sBroker->Need(MeshOp);
}

void WerkSceneNode2::Copy(sObject *o)
{
  WerkSceneNode2 *no;

  sVERIFY(o->GetClass()==sCID_WERKSCENENODE2);
  no = (WerkSceneNode2 *)o;

  TreeLevel = no->TreeLevel;
  sCopyString(Name,no->Name,KK_NAME);
  MatrixEdit = no->MatrixEdit;
  sCopyString(MeshName,no->MeshName,KK_NAME);
  Node->Copy(no->Node);
}

void WerkSceneNode2::UpdateLinks(WerkDoc *doc1)
{
  if(MeshName[0])
    MeshOp = doc1->FindName(MeshName);
  else
    MeshOp = 0;
  Node->BackLink = this;
  Node->Mesh = 0;
}

/****************************************************************************/
/****************************************************************************/


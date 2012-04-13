// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "_types.hpp"
#include "_start.hpp"
#include "_startdx.hpp"
#include "_util.hpp"
#include "_gui.hpp"

#include "main.hpp"

#include "appbrowser.hpp"
#include "appcube.hpp"
#include "appfractal.hpp"
#include "appprefs.hpp"
#include "apptext.hpp"
#include "appbabewatch.hpp"
#include "apptool.hpp"

#include "genfx.hpp"

/****************************************************************************/
/****************************************************************************/

//sDeskManager *sDesktop;
sGuiManager *sGui;
sGuiPainter *sPainter;
sDiskItem *sDiskRoot;

/****************************************************************************/
/****************************************************************************/

void Menu()
{
  sMenuFrame *win;

  win = new sMenuFrame; 
  win->AddMenu("menu A",1,'a');
  win->AddMenu("menu B",2,'b'|sKEYQ_CTRL);
  win->AddMenu("menu C",3,'c'|sKEYQ_SHIFT);

  win->AddBorder(new sNiceBorder);
  sGui->AddPulldown(win);
}

void LaunchTest()
{
  sGuiWindow *w2;
  sToolBorder *tb;

  w2 = new sTestWindow; 
    w2->Position.Init(300,100,600,400); 
    w2->AddBorder(new sSizeBorder);
    tb = new sToolBorder;
    tb->AddMenu("menu",666); 
    tb->AddMenu("menu",667);
    tb->AddButton("button",0); ;
    tb->AddButton("button",0);
    w2->AddBorder(tb);
    w2->AddBorder(new sScrollBorder);
  sGui->AddApp(w2);

}

void LaunchGrid()
{
  sGridFrame *win;
  static sInt Data[64];
  static sChar buffer[64];
  
  sCopyString(buffer,"hello, world!",sizeof(buffer));

  win = new sGridFrame();
  win->SetGrid(12,5,0,sPainter->GetHeight(sGui->PropFont)+8);
  win->Position.Init(0,0,300,200);
  win->AddCon(0,0,4,1)->Button("button",0); 
  win->AddCon(0,1,4,1)->EditBool(0,&Data[0],"bool");
  win->AddCon(0,2,4,1)->EditCycle(0,&Data[1],"cycle","null|eins|zwei|drei|vier");
  win->AddCon(4,0,4,1)->EditRGB(0,&Data[8],"rgb");
  win->AddCon(4,1,4,1)->EditRGBA(0,&Data[12],"rgba");
  win->AddCon(4,2,4,1,2)->EditFixed(0,&Data[16],0);
  win->AddCon(0,3,2,1)->Label("vector"); 
  win->AddCon(2,3,6,1,4)->EditInt(0,&Data[20],"");
  win->AddCon(8,0,4,1)->EditInt(0,&Data[2],"int");
  win->AddCon(8,1,4,1)->EditFloat(0,(sF32 *)&Data[3],"float");
  win->AddCon(8,2,4,1)->EditHex(0,(sU32 *)&Data[4],"hex");
  win->AddCon(8,3,4,1)->EditFixed(0,&Data[5],"fixed");
  win->AddCon(0,4,2,1)->Label("edit"); 
  win->AddCon(2,4,10,1)->EditString(0,buffer,0,64);
  win->AddBorder(new sSizeBorder);
  win->AddBorder(new sScrollBorder);
  sGui->AddApp(win);
}

void LaunchBrowser()
{
  sGuiWindow *win;

  win = new sBrowserApp;

  win->AddTitle("Browser");
  win->Position.Init(0,0,500,300);
  sGui->AddApp(win);
}

void LaunchCube()
{
  sGuiWindow *win;

  win = new sCubeApp;

  win->AddTitle("Cube");
  win->Position.Init(0,0,500,300);
  sGui->AddApp(win);
}

void LaunchFractal()
{
  sGuiWindow *win;

  win = new sFractalApp;

  win->AddTitle("Fractal");
  win->Position.Init(0,0,500,300);
  sGui->AddApp(win);
}

void LaunchPrefs()
{
  sGuiWindow *win;

  win = new sPrefsApp;

  win->AddTitle("Preferences");
  win->Position.Init(0,0,300,200);
  sGui->AddApp(win);
}

void LaunchText()
{
  sGuiWindow *win;

  win = new sTextApp;

  win->AddTitle("Text Editor");
  win->Position.Init(0,0,sPainter->GetWidth(sGui->FixedFont," ")*85,700);
  sGui->AddApp(win);
}

void LaunchBabewatch()
{
  sGuiWindow *win;

  win = new sBabewatchApp;

  win->AddTitle("Babewatch");
  win->Position.Init(0,0,1024,768);
  sGui->AddApp(win);
}

void LaunchTool()
{
  sGuiWindow *win;

  win = new sToolApp;

  win->AddTitle("GenThree");
  win->Position.Init(0,0,1024,768);
  sGui->AddApp(win,0);
}

void InitGui()
{
  sOverlappedFrame *ov;
  sInt i;
  sScreenInfo si;

  for(i=0;i<sGUI_MAXROOTS;i++)
  {
    if(sSystem->GetScreenInfo(i,si))
    {
      ov = new sOverlappedFrame;
      ov->RightClickCmd = 4;
      sGui->SetRoot(ov,i);
    }
  }

  LaunchTool();
  sGui->GetRoot()->GetChild(0)->FindTitle()->Maximise(1);
  //LaunchBabewatch();
}

void AddApp(sChar *name,void(*start)())
{
  sDiskItem *di;
  sDIFolder *df;

  df = (sDIFolder *) sDiskRoot->Find("Programms");
  if(df && df->GetClass()==sCID_DIFOLDER)
  {
    di = new sDIApp;
    if(di->Init(name,df,start))
      df->Add(di);
  }
}

void AddApps()
{
  AddApp("Browser",LaunchBrowser);
  AddApp("Text Editor",LaunchText);
  AddApp("Cube",LaunchCube);
  AddApp("Fractal",LaunchFractal);
  AddApp("Preferences",LaunchPrefs);
  AddApp("Babewatch",LaunchBabewatch);
  AddApp("Tool",LaunchTool);
/*
  AddApp("Frame Test",LaunchGrid);
  AddApp("Basic Test",LaunchTest);
*/
}

void DirWalk(sDiskItem *dir,sInt indent)
{
  sDiskItem *file;
  sInt i,j;
  sChar name[128];

  if(indent<2)
    dir->Cmd(sDIC_FINDALL,0,0);

  i=0;
  for(i=0;;i++)
  {
    file = dir->GetChild(i);
    if(!file)
      break;
    for(j=0;j<indent;j++)
      sDPrintF("  ");
    file->GetString(sDIA_NAME,name,sizeof(name));
    if(file->GetBool(sDIA_ISDIR))
    {
      sDPrintF("[%s]\n",name);
      DirWalk(file,indent+1);
    }
    else
    {
      sDPrintF("%s %d\n",name,file->GetBinarySize(sDIA_DATA));
    }
  }
}

/****************************************************************************/
/****************************************************************************/


sBool sAppHandler(sInt code,sU32 value)
{
  switch(code)
  {
  case sAPPCODE_CONFIG:
    sSetConfig(sSF_DIRECT3D,0,0);
//    sSetConfig(sSF_DIRECT3D|sSF_MULTISCREEN,0,0);
//    sSetConfig(sSF_OPENGL,0,0);
//    sSetConfig(sSF_FULLSCREEN|sSF_DIRECT3D,1600,1200);
//    sSetConfig(sSF_MULTISCREEN|sSF_OPENGL,640,480);
    break;
  case sAPPCODE_INIT:

    sDiskRoot  = new sDIRoot;
    AddApps();
    sBroker->AddRoot(sDiskRoot);
    sPainter = new sGuiPainter;
    sPainter->Init();
    sBroker->AddRoot(sPainter);
    sGui = new sGuiManager;
    sBroker->AddRoot(sGui);
    InitGui();
		InitFXMaster();
    sInitPerlin();
    sBroker->Free();

    sDPrintF("*********** start\n");
//    DirWalk(sDiskRoot,0);
    sDPrintF("*********** end\n");

    break;

  case sAPPCODE_EXIT:
		CloseFXMaster();
    sSystem->SetSoundHandler(0,0,0);
    sBroker->RemRoot(sPainter);
    sBroker->RemRoot(sGui); sGui = 0;
    sBroker->RemRoot(sDiskRoot);
    sBroker->Free();
    break;

  case sAPPCODE_KEY:
    switch(value)
    {
    case sKEY_ESCAPE|sKEYQ_SHIFTL:
    case sKEY_ESCAPE|sKEYQ_SHIFTR:
    case sKEY_CLOSE:
      sSystem->Exit();
      break;
    default:
      sGui->OnKey(value);
      break;
    }
    break;
    
  case sAPPCODE_PAINT:
    {
      sInt i;
      sBool ok;

      sGui->OnFrame();
      sGui->OnPaint();
      ok = sFALSE;
      for(i=0;i<sGUI_MAXROOTS;i++)
        if(sGui->GetRoot(i) && sGui->GetRoot(i)->GetChildCount()!=0)
          ok = sTRUE;
      if(!ok) 
        sSystem->Exit();
      /*
      sViewport view;
      view.Init();
      view.Screen = 1;
      view.ClearColor = 0xffff0000;
      sSystem->BeginViewport(view);
      sSystem->EndViewport();
      */
    }
    break;

  case sAPPCODE_LAYOUT:
    break;

  case sAPPCODE_CMD:
    switch(value)
    {
    case 667:
      Menu();
      break;
    case 1:
      (new sDialogWindow)->InitOk(0,"test","ok?",0);
      break;
    case 2:
      (new sDialogWindow)->InitOkCancel(0,"test","ok or cancel?",0,0);
      break;
    case 3:
      (new sDialogWindow)->InitYesNo(0,"test","yes or no?",0,0);
      break;
    case 4:
      LaunchBrowser();
      break;
    }
    break;

  case sAPPCODE_DEBUGPRINT:
    if(sGui)
      return sGui->OnDebugPrint((sChar *)value);
    return sFALSE;

  default:
    return sFALSE;
  }
  return sTRUE;
}

/****************************************************************************/


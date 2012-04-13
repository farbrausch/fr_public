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
#include "werkkzeug.hpp"
#include "genoverlay.hpp"
#include "geneffect.hpp"
#include "engine.hpp"

/****************************************************************************/
/****************************************************************************/

//sDeskManager *sDesktop;
sGuiManager *sGui;
sGuiPainter *sPainter;
sDiskItem *sDiskRoot;

/****************************************************************************/
/****************************************************************************/

#if !sPLAYER
#include "script.hpp"
/*
const sChar source[] =
"  scalar i,j; vector a,b;\n"
"  extern scalar ext;\n"
"  a=[1,1,1,1]; b=[1,2,3,4];"
"  print([1,2,3+4,0.5]);\n"
"  print(dp3(a,b));\n"
"  print(\" - \");  i=0?3:4; print(i);\n"
"  print(\" - \");  i=1?3:4; print(i);\n"
"  print(\" - \");  print(b.y);\n"
"  print(\" - \");  ext=0.5; print(ext);\n"
;
*/
/*
const sChar source[] =
"  scalar i;\n"
"  i = 0;\n"
"  if(i) { print(\"a\"); } else { print (\"b\"); }\n"
"  print(2==3); print(2!=3);  print(2>3); print(2>=3);  print(2<3); print(2<=3);  print(2>2); print(2>=2);\n"
"  i=0; do { print(i*i); print(\"\\n\"); i=i+1; } while(i<5);\n"
;
*/
const sChar source[] = "print(1+2);";

void ScriptTest()
{
  sScript script;
  sScriptVM *vm;
  sScriptCompiler *com;
  sText *errors;
  sText *dis;

  vm = new sScriptVM;
  com = new sScriptCompiler;
  errors = new sText;
  dis = new sText;

  script.Init();

  if(com->Compiler(&script,source,errors))
  {
    com->Disassemble(&script,dis,1);
    sDPrint(dis->Text);
    vm->Execute(&script,0);
  }


  sDPrint("compiler errors:\n");
  sDPrint(errors->Text);   
  sDPrint("script-output:\n");
  sDPrint(vm->GetOutput());
  sDPrint("\n");
  sFatal("das wars");

  delete vm;
  delete com;
  delete errors;
}

// ----

#include "shadercompile.hpp"

void ShCompileTest()
{
  sShaderCompiler *com;
  static const sChar *compileFiles[] =
  {
    "material11.vsh", "material11.vsfr",
    "material11.psh", "material11.psfr",
    0,
  };
  sText *errors;
  sText *message;
  sBool noErrors = sTRUE;

  com = new sShaderCompiler;
  errors = new sText;
  message = new sText;

  for(sInt i=0;compileFiles[i];i+=2)
  {
    sChar *shsource = sSystem->LoadText(compileFiles[i+0]);
    if(com->Compile(shsource,errors))
    {
      message->PrintF("%s compiled ok\n",compileFiles[i+0]);
      sSystem->SaveFile(compileFiles[i+1],(sU8 *)com->GetShader(),com->GetShaderSize()*4);
    }
    else
    {
      message->PrintF("%s had errors:\n",compileFiles[i+0]);
      message->Print(errors->Text);

      noErrors = sFALSE;
    }

    delete[] shsource;
  }

  delete com;
  delete errors;

  if(noErrors)
  {
    // hack hack hack: make sure data.asm gets rebuild ;)
    const sChar *dataAsm = sSystem->LoadText("../data.asm");
    sSystem->SaveFile("../data.asm",(const sU8 *)dataAsm,sGetStringLen(dataAsm));
    delete[] dataAsm;
  }

  sFatal(message->Text);
}

#endif

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

void LaunchWerkk()
{
  sGuiWindow *win;

  win = new WerkkzeugApp;

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

//  sGui->SetStyle(sGui->GetStyle()|sGS_MAXTITLE);
  sGui->SetStyle(sGui->GetStyle()|sGS_SMALLFONTS);

  LaunchWerkk();
  sGui->GetRoot()->GetChild(0)->FindTitle()->Maximise(1);
  //LaunchCube();
  //sGui->GetRoot()->GetChild(0)->FindTitle()->Maximise(1);
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
  AddApp("Werkkzeug",LaunchWerkk);
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


sBool sAppHandler(sInt code,sDInt value)
{
  switch(code)
  {
  case sAPPCODE_CONFIG:
#if 1
    sSetConfig(sSF_DIRECT3D,0,0);
//    sSetConfig(sSF_FULLSCREEN|sSF_DIRECT3D,1280,1024);
#else
#ifndef _DEBUG
    sSetConfig(sSF_FULLSCREEN|sSF_DIRECT3D,1280,1024);
#else
    sSetConfig(sSF_DIRECT3D,0,0);
#endif
#endif
//    sSetConfig(sSF_DIRECT3D|sSF_MULTISCREEN,0,0);
//    sSetConfig(sSF_FULLSCREEN|sSF_DIRECT3D,1280,1024);

    break;
  case sAPPCODE_INIT:
//    ScriptTest();   break;
//    ShCompileTest();

    sInitPerlin();
    GenOverlayInit();
    MemManagerInit();

    Engine = new Engine_;

    sDiskRoot  = new sDIRoot;
    AddApps();
    sBroker->AddRoot(sDiskRoot);
    sPainter = new sGuiPainter;
    sPainter->Init();
    sBroker->AddRoot(sPainter);
    sGui = new sGuiManager;
    sBroker->AddRoot(sGui);
    InitGui();
    sBroker->Free();

    sDPrintF("*********** start\n");
//    DirWalk(sDiskRoot,0);
    sDPrintF("*********** end\n");

    break;

  case sAPPCODE_EXIT:
    delete Engine;

    GenOverlayExit();
    sSystem->SetSoundHandler(0,0,0);
    sBroker->RemRoot(sPainter);
    sBroker->RemRoot(sGui); sGui = 0;
    sBroker->RemRoot(sDiskRoot);
    sBroker->Free();
    MemManagerExit(); // must be last because garbagecollected objects may rely on it
    FreeParticles();
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

  case sAPPCODE_TICK:
    sGui->OnTick(value);
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



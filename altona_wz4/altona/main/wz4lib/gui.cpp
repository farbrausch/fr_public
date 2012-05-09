/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) by Dierk Ohlerich                                    ***/
/***   all rights reserverd                                               ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "gui.hpp"
#include "gui/textwindow.hpp"
#include "doc.hpp"
#include "build.hpp"
#include "gui/gui.hpp"
#include "util/painter.hpp"
#include "util/shaders.hpp"
#include "base/sound.hpp"
#include "wz4lib/serials.hpp"
#include "wz4lib/version.hpp"

#include "wz4lib/basic_ops.hpp"
#include "wz4lib/view.hpp"
#include "wz4lib/script.hpp"
#include "wz4lib/wiki.hpp"
#include "util/algorithms.hpp"


MainWindow *App;
extern "C" const sChar8 WireTXT[];

/****************************************************************************/

#define STB_VORBIS_NO_CRT
#define STB_VORBIS_NO_PUSHDATA_API
#define STB_VORBIS_NO_FAST_SCALED_FLOAT
#define malloc(x) sAllocMem(x,16,0)
#define free(x) sFreeMem(x)
#define assert(x) sVERIFY(x)
#define alloca(x) sALLOCSTACK(sU8,x)

static void qsort ( void * base, size_t num, size_t size, int ( * comparator ) ( const void *, const void * ) )
{
  // qsort is used by stb_vorbis only at init time, so a really slow bubble sort is enough
  sBool doit;
  do
  {
    doit=sFALSE;
    for (sInt i=0; i<(sInt)(num-1); i++)
    {
      sU8 *a=(sU8*)base+i*size;
      sU8 *b=a+size;
      if (comparator(a,b)>0)
      {
        doit=sTRUE;
        for (sInt j=0; j<(sInt)size; j++) sSwap(a[j],b[j]);
      }
    }
  } while (doit);
}

#include "stb_vorbis.inl"

/****************************************************************************/

static void Wz4SoundHandler(sS16 *samples,sInt count)
{
  App->SoundHandler(samples,count);
}


class WinPopupText : public sWireTextWindow
{
public:
  void OnCalcSize() 
  {
    sWireTextWindow::OnCalcSize();
    ReqSizeX = sMax(ReqSizeX,800); 
    ReqSizeY = sMax(ReqSizeY,600);
  }
};

/****************************************************************************/

MainWindow::MainWindow()
{
  App = this;
  if(!Doc)
    new wDocument();
  Painter = new sPainter();
  sGui->HandleShiftEscape = 0;
  SamplePos = 0;
  SampleOffset = 0;
  MusicData = 0;
  MusicSize = 0;
  AutosaveTimer = 0;
  BackupIndex = 0;
  BlinkTimer = new sMessageTimer(sMessage(this,&MainWindow::CmdBlinkTimer),125,1);
  
  DocOptionsWin = 0;
  EditOptionsWin = 0;
  TestOptionsWin = 0;
  ShellSwitchesWin = 0;

  AddPopupX = 0;
  AddPopupY = 0;
  BPM = 0;

  PageListWin = 0;
  TreeViewWin = 0;
  sClear(ParaWin);
  sClear(ViewWin);
  StackWin = 0;
  StoreBrowserWin = 0;
  OpListWin = 0;
  TimelineWin = 0;
  VolumeWin = 0;
  UnblockChangeWin = 0;
  CamSpeedWin = 0;
  Status = 0;
  CurveOpsWin = 0;
  ClipOpsWin = 0;
  CustomWin = 0;
  Wiki = 0;
  PopupTextWin = 0;

  DocOptionsWin = 0;
  EditOptionsWin = 0;
  TestOptionsWin = 0;
  ShellSwitchesWin = 0;

  AddType = 0;
  AddTypeSec = 0;

  WikiPath = 0;
  WikiCheckout = 0;
  WikiToggleFlag = 0;
  UnitTestPath = L"";
}

void MainWindow::MainInit()
{
  wType *type;
  sFORALL(Doc->Types,type)
  {
    if(type != AnyTypeType)
    {
      AddType = type;
      break;
    }
  }

  Doc->AppChangeFromCustomMsg = sMessage(this,&MainWindow::CmdAppChangeFromCustom);
  Doc->AppScrollToArrayLineMsg = sMessage(this,&MainWindow::CmdScrollToArrayLine);

  AddChild(sWire = new sWireMasterWindow);
  sWire->AddWindow(L"main",this);

  TimelineWin = new WinTimeline;    // must be early, ViewWin and PageWin use the ptr.
  TimelineWin->ScratchMsg = sMessage(this,&MainWindow::ScratchTime);
  TimelineWin->InitWire(L"Timeline");

  PageListWin = new WinPageList; PageListWin->InitWire(L"PageList");
  TreeViewWin = new WinTreeView; TreeViewWin->InitWire(L"TreeView");
  ParaWin[0] = new WinPara; ParaWin[0]->InitWire(L"Para");
  ParaWin[1] = new WinPara; ParaWin[1]->InitWire(L"AnimPara");
  ViewWin[0] = new WinView; ViewWin[0]->InitWire(L"View0");
  ViewWin[1] = new WinView; ViewWin[1]->InitWire(L"View1");
  ViewWin[2] = new WinView; ViewWin[2]->InitWire(L"View2");
  StackWin = new WinStack; StackWin->InitWire(L"Page");
  StoreBrowserWin = new WinStoreBrowser; StoreBrowserWin->InitWire(L"StoreBrowser");
  PopupTextWin = new WinPopupText; PopupTextWin->InitWire(L"PopupText");
  PopupTextWin->EditFlags = sTEF_STATIC|sTEF_LINENUMBER;
  PopupTextWin->SetFont(sGui->FixedFont);
  PopupTextWin->ReqSizeX = 640;
  PopupTextWin->ReqSizeY = 48;
  OpListWin = new WinOpList; OpListWin->InitWire(L"OpList");
//  OpListWin->AddBorder(new sListWindow2Header(OpListWin));
  Status = new sStatusBorder; sWire->AddWindow(L"Status",Status);

  CustomWin = new WinCustom; sWire->AddWindow(L"Custom",CustomWin);

  ClipOpsWin = new WinClipOpsList(&AnimClipOps);  
  ClipOpsWin->SelectMsg = sMessage(this,&MainWindow::CmdSetClip);
  ClipOpsWin->InitWire(L"ClipOps");

  CurveOpsWin = new WinCurveOpsList(&AnimCurveOps);  
  CurveOpsWin->SelectMsg = sMessage(this,&MainWindow::CmdSetCurve);
  CurveOpsWin->InitWire(L"CurveOps");

  Wiki = new WikiHelper;
  Wiki->WikiPath = WikiPath;
  Wiki->WikiCheckout = WikiCheckout;
  Wiki->InitWire(L"main");


  VolumeWin = new sIntControl(&Doc->EditOptions.Volume,0,100,1); 
  VolumeWin->ReqSizeX = 40;
  VolumeWin->Style |= sSCS_BACKBAR;
  sWire->AddWindow(L"Volume",VolumeWin);

  UnblockChangeWin = new sButtonControl(L"Unblock Changes",sMessage(this,&MainWindow::CmdUnblockChange));
  sWire->AddWindow(L"UnblockChange",UnblockChangeWin);

  CamSpeedWin = new sChoiceControl(L"cam slow|cam normal|cam fast|cam ultra",&Doc->EditOptions.MoveSpeed);
  CamSpeedWin->ReqSizeX = 60;
  sWire->AddWindow(L"CamSpeed",CamSpeedWin);

  ParaPresetWin = new sButtonControl(L"Presets",sMessage(ParaWin[0],&WinPara::CmdPreset),sBCS_NOBORDER|sBCS_IMMEDIATE);
  sWire->AddWindow(L"ParaPreset",ParaPresetWin);

  for(sInt t=0;t<3;t++)
  {
    ViewWin[t]->SpeedDamping = 0.5f;
    ViewWin[t]->SideSpeed = 0.01f;
    ViewWin[t]->ForeSpeed = 0.01f;
  }

  Status->AddTab(200); 
  Status->AddTab(0);
  Status->AddTab(-150);
  Status->AddTab(-150);
//  Status->Print(0,L"status left");
//  Status->Print(1,L"status middle");
//  Status->Print(2,L"status right");

  sWire->AddKey(L"View0",L"UnblockChange",sMessage(this,&MainWindow::CmdUnblockChange));

  sWire->AddKey(L"main",L"New",sMessage(this,&MainWindow::CmdNew));
  sWire->AddKey(L"main",L"Open",sMessage(this,&MainWindow::CmdOpen));
  sWire->AddKey(L"main",L"OpenBackup",sMessage(this,&MainWindow::CmdOpenBackup));
  sWire->AddKey(L"main",L"Merge",sMessage(this,&MainWindow::CmdMerge));
  sWire->AddKey(L"main",L"Save",sMessage(this,&MainWindow::CmdSave));
  sWire->AddKey(L"main",L"SaveAs",sMessage(this,&MainWindow::CmdSaveAs));
  sWire->AddKey(L"main",L"Exit",sMessage(this,&MainWindow::CmdExit));
  sWire->AddKey(L"main",L"Panic",sMessage(this,&MainWindow::CmdPanic));
  sWire->AddKey(L"main",L"FlushCache",sMessage(this,&MainWindow::CmdFlushCache));
  sWire->AddKey(L"main",L"ChargeCache",sMessage(this,&MainWindow::CmdChargeCache));
  sWire->AddKey(L"main",L"UnblockChange",sMessage(this,&MainWindow::CmdUnblockChange));
  sWire->AddKey(L"main",L"EditOptions",sMessage(this,&MainWindow::CmdEditOptions));
  sWire->AddKey(L"main",L"DocOptions",sMessage(this,&MainWindow::CmdDocOptions));
  sWire->AddKey(L"main",L"TestOptions",sMessage(this,&MainWindow::CmdTestOptions));
  sWire->AddKey(L"main",L"ShellSwitches",sMessage(this,&MainWindow::CmdShellSwitches));
  sWire->AddKey(L"main",L"LoadConfig",sMessage(this,&MainWindow::CmdLoadConfig));
  sWire->AddKey(L"main",L"SaveConfig",sMessage(this,&MainWindow::CmdSaveConfig));
  sWire->AddKey(L"main",L"BlackAmbient",sMessage(this,&MainWindow::CmdSetAmbient,0));
  sWire->AddKey(L"main",L"WhiteAmbient",sMessage(this,&MainWindow::CmdSetAmbient,1));
  sWire->AddKey(L"main",L"StartTime",sMessage(this,&MainWindow::CmdStartTime));
  sWire->AddKey(L"main",L"RestartTime",sMessage(this,&MainWindow::CmdRestartTime));
  sWire->AddKey(L"main",L"LoopTime",sMessage(this,&MainWindow::CmdLoopTime));
  sWire->AddChoice(L"main",L"TextureQuality",sMessage(),&Doc->DocOptions.TextureQuality,L"textures full|textures DXT|textures low");
  sWire->AddChoice(L"main",L"Lod",sMessage(),&Doc->DocOptions.LevelOfDetail,L"LoD low|LoD med|LoD high|LoD extra|LoD auto");
  sWire->AddKey(L"main",L"Switch0",sMessage(this,&MainWindow::CmdSwitchScreen,0));
  sWire->AddKey(L"main",L"Switch1",sMessage(this,&MainWindow::CmdSwitchScreen,1));
  sWire->AddKey(L"main",L"Switch2",sMessage(this,&MainWindow::CmdSwitchScreen,2));
  sWire->AddKey(L"main",L"Switch3",sMessage(this,&MainWindow::CmdSwitchScreen,3));
  sWire->AddKey(L"main",L"RenameAll",sMessage(this,&MainWindow::CmdRenameAll));
  sWire->AddKey(L"main",L"References",sMessage(this,&MainWindow::CmdReferences));
  sWire->AddKey(L"main",L"FindClass",sMessage(this,&MainWindow::CmdFindClass));
  sWire->AddKey(L"main",L"FindObsolete",sMessage(this,&MainWindow::CmdFindObsolete));
  sWire->AddKey(L"main",L"KillClips",sMessage(this,&MainWindow::CmdKillClips));
  sWire->AddKey(L"main",L"SelUnconnected",sMessage(this,&MainWindow::CmdSelUnconnected));
  sWire->AddKey(L"main",L"DelUnconnected",sMessage(this,&MainWindow::CmdDelUnconnected));
  sWire->AddKey(L"main",L"AutoSave",sMessage(this,&MainWindow::CmdAutoSave));
  sWire->AddKey(L"main",L"FindStore",sMessage(this,&MainWindow::CmdFindStore));
  sWire->AddKey(L"main",L"FindString",sMessage(this,&MainWindow::CmdFindString));
  sWire->AddKey(L"main",L"Wiki",sMessage(ParaWin[0],&WinPara::CmdWiki));

  sWire->AddKey(L"main",L"Goto0",sMessage(this,&MainWindow::CmdGotoShortcut,0));
  sWire->AddKey(L"main",L"Goto1",sMessage(this,&MainWindow::CmdGotoShortcut,1));
  sWire->AddKey(L"main",L"Goto2",sMessage(this,&MainWindow::CmdGotoShortcut,2));
  sWire->AddKey(L"main",L"Goto3",sMessage(this,&MainWindow::CmdGotoShortcut,3));
  sWire->AddKey(L"main",L"Goto4",sMessage(this,&MainWindow::CmdGotoShortcut,4));
  sWire->AddKey(L"main",L"Goto5",sMessage(this,&MainWindow::CmdGotoShortcut,5));
  sWire->AddKey(L"main",L"Goto6",sMessage(this,&MainWindow::CmdGotoShortcut,6));
  sWire->AddKey(L"main",L"Goto7",sMessage(this,&MainWindow::CmdGotoShortcut,7));
  sWire->AddKey(L"main",L"Goto8",sMessage(this,&MainWindow::CmdGotoShortcut,8));
  sWire->AddKey(L"main",L"Goto9",sMessage(this,&MainWindow::CmdGotoShortcut,9));
  sWire->AddKey(L"main",L"GotoRoot",sMessage(this,&MainWindow::CmdGotoRoot));

  sWire->AddKey(L"main",L"Browser"   ,sMessage(this,&MainWindow::CmdBrowser));
  sWire->AddKey(L"main",L"GotoUndo"  ,sMessage(this,&MainWindow::GotoUndo));
  sWire->AddKey(L"main",L"GotoRedo"  ,sMessage(this,&MainWindow::GotoRedo));

  ExtendWireRegister();

  // if -w shell parameter is set, load external wire.txt else load internal default wire.txt
  const sChar *wirefilename = sGetShellParameter(L"w",0);
  if(wirefilename && sCreateFile(wirefilename,sFA_READ) )
    sWire->ProcessFile(wirefilename);
  else
    sWire->ProcessText(WireTXT,L"werkkzeug4.wire.txt");

  ExtendWireProcess();
  sWire->ProcessEnd();

  sWire->ToolChangedMsg = sMessage(this,&MainWindow::UpdateTool);

  sGetCurrentDir(ProgramDir);
  PrepareBackup();

  CmdLoadConfig();

  sBool resetwin = 1;

  const sChar *filename = sGetShellParameter(0,0);
  if(filename)
  {
    if(Doc->Load(filename))
    {
      ResetWindows();
      CheckAfterLoading();
      resetwin = 0;
      OnDocOptionsChanged();
    }
    else
    {
      sErrorDialog(sMessage(),L"initial load failed");
    }
    Doc->Filename = filename;
  }
  else
  {
    if(Doc->Load(Doc->EditOptions.File))
    {
      ResetWindows();
      CheckAfterLoading();
      resetwin = 0;
      OnDocOptionsChanged();
    }
  }
  if(resetwin)
    ResetWindows();
  CalcWasDone();
  CmdReloadMusic();

  sSetSoundHandler(Doc->DocOptions.SampleRate,Wz4SoundHandler,5000);
  sSetTimerEvent(17,1);
}

void MainWindow::Finalize()
{
  sClearSoundHandler();
}

MainWindow::~MainWindow()
{
  delete BlinkTimer;
  delete Painter;
  sDeleteAll(App->StoreTree);
  delete[] MusicData;
}

void MainWindow::Tag()
{
  if(PageListWin==0)
    sFatal(L"please call MainWindow::MainInit() after constructor");
  sWindow::Tag();
  Doc->Need();
  Wiki->Need();

  sNeed(GotoStack);
  sNeed(GotoStackRedo);

  StoreBrowserWin->Need();
  OpListWin->Need();

  PageListWin->Need();
  TreeViewWin->Need();
  for(sInt i=0;i<sCOUNTOF(ParaWin);i++)
    ParaWin[i]->Need();
  for(sInt i=0;i<sCOUNTOF(ViewWin);i++)
    ViewWin[i]->Need();
  StackWin->Need();
}

sBool MainWindow::OnCommand(sInt cmd)
{
  if(cmd==sCMD_TIMER)
  {
    TimelineWin->UpdateSample(sGetCurrentSample()-SampleOffset+SamplePos);
    CustomWin->OnTime(App->GetTimeBeats());
    return 1;
  }
  else
  {
    return 0;
  }
}


void MainWindow::ChangeDoc()
{
  if(!Doc->DocChanged)
  {
    Doc->DocChanged = 1;
    UpdateWindows();
  }
  if(AutosaveTimer==0 && Doc->EditOptions.AutosavePeriod>0)
    AutosaveTimer = sGetTime() + Doc->EditOptions.AutosavePeriod*1000;     // autosave in 60 seconds...
}

void MainWindow::UpdateTool()
{
  Status->Print(STATUS_TOOL,sWire->GetCurrentToolName());
}

void MainWindow::UpdateStatus()
{
  sGraphicsStats stats;
  UpdateTool();
  sGetGraphicsStats(stats);
  sU64 hwtex = stats.StaticTextureMem;
  sU64 swtex = sTotalImageDataMem;
  sU64 vbmem = stats.StaticVertexMem;
  sU64 tmem = sMemoryUsed;
  sInt leaks = sGetMemoryLeakTracker() ? sGetMemoryLeakTracker()->GetLeakCount() : 0;
  Status->PrintF(STATUS_MEMORY,L"%KB hwtex  %KB vertex | %KB swtex | %KB mainmem, %k allocs",hwtex,vbmem,swtex,tmem,leaks);
  Status->Print(STATUS_TOOL,sWire->GetCurrentToolName());
  if(Doc->DocChanged)
    Status->PrintF(STATUS_FILENAME,L"*%s",Doc->Filename);
  else
    Status->PrintF(STATUS_FILENAME,L"%s",Doc->Filename);

  sString<sMAXPATH> name;
  name.PrintF(L"werkkzeug4 V%d.%d",WZ4_VERSION,WZ4_REVISION);
  if(sRENDERER==sRENDER_DX9)
    name.Add(L" (DX9)");
  if(sRENDERER==sRENDER_DX11)
    name.Add(L" (DX11)");
  if(sRENDERER==sRENDER_OGL2)
    name.Add(L" (OGL2)");
  if(sCONFIG_64BIT)
    name.Add(L" (64Bit)");
  if(sCONFIG_BUILD_DEBUG)
    name.Add(L" (debug)");
  else if(sCONFIG_BUILD_DEBUGFAST)
    name.Add(L" (debugfast)");
  else if(!sCONFIG_BUILD_RELEASE)
    name.Add(L" (unknown debug)");
  if(!Doc->Filename.IsEmpty())
    name.PrintAddF(L" - %s",Doc->Filename);
  if(name != sGetWindowName())
    sSetWindowName(name);

  if(0) // subtile
  {
    sInt style = Doc->BlockedChanges ? 0 : sBCS_STATIC;
    if(style!=UnblockChangeWin->Style)
    {
      UnblockChangeWin->Style = style;
      UnblockChangeWin->Update();
    }
  }
  else    // fat
  {
    sU32 color = Doc->BlockedChanges ? 0xffff0000 : 0;
    if(color!=UnblockChangeWin->BackColor)
    {
      UnblockChangeWin->BackColor = color;
      UnblockChangeWin->Update();
    }
  }
}

void MainWindow::UpdateWindows()
{
  PageListWin->UpdateTreeInfo();
  UpdateStatus();
  Update();
}

void MainWindow::UpdateImportantOp()
{
  wOp *op,*in;
  sFORALL(Doc->AllOps,op)
    op->ImportantOp = 0;

  for(sInt i=0;i<sCOUNTOF(ViewWin);i++)
  {
    op = ViewWin[i]->Op;
    if(op)
      op->ImportantOp = 1;
  }
  for(sInt i=0;i<sCOUNTOF(ParaWin);i++)
  {
    op = ParaWin[i]->Op;
    if(op)
    {
      op->ImportantOp = 1;
      sFORALL(op->Inputs,in)
        in->ImportantOp = 1;
    }
  }
}

void MainWindow::ResetWindows()
{
  TreeViewWin->SetPage(0);
  StackWin->SetPage(0);
  PageListWin->SetSelected(-1);
  if(Doc->Pages.GetCount()>0)
  {
    PageListWin->SetSelected(0);
  }
  PageListWin->Update();
  TreeViewWin->UpdateTreeInfo();
  for(sInt i=0;i<sCOUNTOF(ParaWin);i++)
    ParaWin[i]->SetOp(0);
  for(sInt i=0;i<sCOUNTOF(ViewWin);i++)
  {
    ViewWin[i]->SetOp(0);
    ViewWin[i]->CmdReset3D();
  }
  UpdateWindows();
}

void MainWindow::CmdBlinkTimer()
{
  StackWin->CmdBlinkTimer();
  TreeViewWin->CmdBlinkTimer();
  if(AutosaveTimer && sGetTime()>AutosaveTimer)
    CmdAutoSave();
}

void MainWindow::PrepareBackup()
{
  if(!sCOMMANDLINE)
  {
    BackupDir = ProgramDir;
    BackupDir.AddPath(L"backup");
    if(!sCheckDir(BackupDir))
      sMakeDir(BackupDir);

    sArray<sDirEntry> list;
    sDirEntry *de;
    sLoadDir(list,BackupDir);
    BackupIndex = 0;
    const sChar *s;
    sFORALL(list,de)
    {
      s = de->Name;
      sInt i;
      if(sScanInt(s,i) && *s=='#')
        BackupIndex = i;
    }
  }
}

void MainWindow::ShowOp(wOp *op,sInt n)
{
  sVERIFY(n>=0 && n<sCOUNTOF(ViewWin));
  UpdateStatus();
  ViewWin[n]->SetOp(op);
  StackWin->Update();
  TreeViewWin->Update();
}

sBool MainWindow::GotoOpBase(wOp *op,sBool blink)
{
  if(op && op->Page)
  {
    PageListWin->SetSelected(op->Page);

    if(!op->Page->IsTree)
    {
      SwitchPage(0);
      StackWin->SetPage(op->Page,1);
      StackWin->GotoOp(op,blink);

      StackWin->SetLastOp(op);
      return 1;
    }
    else
    {
      SwitchPage(1);
      TreeViewWin->SetPage(op->Page);
      TreeViewWin->GotoOp(op,blink);
      return 1;
    }
  }

  return 0;
}

void MainWindow::GotoOp(wOp *op,sBool blink)
{
  if(GotoOpBase(op,blink))
  {
    GotoPush();
    EditOp(op,0);
  }
}

void MainWindow::GotoPush(wOp *op)
{
  if(!op)
    op = GetEditOp();

  sInt i = GotoStack.GetCount();
  if(op && (i==0 || GotoStack[i-1]!=op))
  {
    GotoStack.AddTail(op);
    GotoStackRedo.Clear();
  }
}

void MainWindow::GotoUndo()
{
  if(!GotoStack.IsEmpty())
  {
    wOp *oldop = GetEditOp();
    if(oldop)
      GotoStackRedo.AddTail(oldop);
    wOp *op = GotoStack.RemTail();
    GotoOpBase(op);
    EditOp(op,0);
  }
}

void MainWindow::GotoRedo()
{
  if(!GotoStackRedo.IsEmpty())
  {
    wOp *oldop = GetEditOp();
    if(oldop)
      GotoStack.AddTail(oldop);
    wOp *op = GotoStackRedo.RemTail();
    GotoOpBase(op);
    EditOp(op,0);
  }
}

void MainWindow::GotoClear()
{
  GotoStack.Clear();
  GotoStackRedo.Clear();
}

void MainWindow::EditOp(wOp *op,sInt n)
{
  if(op && n==0 && (op->Class->Flags & (wCF_CLIP|wCF_CURVE)) && sWire->GetScreen()==3)
    n = 1;

  UpdateStatus();
  ParaWin[n]->SetOp(op);
  StackWin->Update();
  StackWin->Update();
  if(op)
  {
    if(!op->Name.IsEmpty())
      Doc->LastName = op->Name;
    if((op->Class->Flags & wCF_LOAD) && op->Links.GetCount()==1 && !op->Links[0].LinkName.IsEmpty())
      Doc->LastName = op->Links[0].LinkName;

    if(op->Class->Flags & wCF_CLIP)
      ClipOpsWin->SetSelected(op);
    if(op->Class->Flags & wCF_CURVE)
      CurveOpsWin->SetSelected(op);
  }
  if(op->Page && op->Page->IsTree)
    Status->PrintF(STATUS_MESSAGE,L"");
  else
    Status->PrintF(STATUS_MESSAGE,L"cursor x:%d,y:%d",((wStackOp *)op)->PosX,((wStackOp *)op)->PosY);
}

void MainWindow::EditOpReloadAll()
{
  for(sInt i=0;i<sCOUNTOF(ParaWin);i++)
    ParaWin[i]->SetOp(ParaWin[i]->Op);
}

void MainWindow::AnimOpR1(wOp *op)
{
  if(op->NoError())
  {
    if(op->Class->Flags & wCF_CLIP)
      if(!sFindPtr(AnimClipOps,op))
        AnimClipOps.AddTail(op);
    wOp *out;
    sFORALL(op->Outputs,out)
      AnimOpR1(out);
  }
}

void MainWindow::AnimOpR2(wOp *op)
{
  if(op->NoError())
  {
    if(op->Class->Flags & wCF_CURVE)
      if(!sFindPtr(AnimCurveOps,op))
        AnimCurveOps.AddTail(op);
    wOp *in;
    sFORALL(op->Inputs,in)
      AnimOpR2(in);
    wOpInputInfo *info;
    sFORALL(op->Links,info)
      if(info->Link)
        AnimOpR2(info->Link);
  }
}

void MainWindow::AnimOp(wOp *op)
{
  Doc->Connect();
  AnimClipOps.Clear();
  AnimOpR1(op);
  if(AnimClipOps.GetCount()>0)
    ClipOpsWin->SetSelected(0);
  ClipOpsWin->Update();
  CmdSetClip();
}

void MainWindow::CmdSetClip()
{
  AnimCurveOps.Clear();
  wOp *op = ClipOpsWin->GetSelected();
  if(op)
  {
    AnimOpR2(op);
    if(AnimCurveOps.GetCount()>0)
      CurveOpsWin->SetSelected(0);
    CurveOpsWin->Update();
    EditOp(op,1);
    ShowOp(op,3);
  }
}

void MainWindow::CmdSetCurve()
{
  wOp *op = CurveOpsWin->GetSelected();
  if(op)
    EditOp(op,1);
}


wOp *MainWindow::GetEditOp()
{
  return ParaWin[0]->Op;
}

void MainWindow::ClearOp(wOp *op)
{
  if(op==0)
  {
    for(sInt i=0;i<sCOUNTOF(ParaWin);i++)
      ParaWin[i]->SetOp(0);
    for(sInt i=0;i<sCOUNTOF(ViewWin);i++)
      ViewWin[i]->SetOp(0);
  }
  else
  {
    for(sInt i=0;i<sCOUNTOF(ParaWin);i++)
    {
      if(ParaWin[i]->Op==op)
        ParaWin[i]->SetOp(0);
    }
    for(sInt i=0;i<sCOUNTOF(ViewWin);i++)
    {
      if(ViewWin[i]->Op==op)
        ViewWin[i]->SetOp(0);
    }
  }
  StackWin->Update();
  Status->PrintF(STATUS_MESSAGE,L"");
}

sBool MainWindow::IsShown(wOp *op)
{
  for(sInt i=0;i<sCOUNTOF(ViewWin);i++)
    if(ViewWin[i]->Op==op) return 1;
  return 0;
}

sBool MainWindow::IsEdited(wOp *op)
{
  for(sInt i=0;i<sCOUNTOF(ParaWin);i++)
    if(ParaWin[i]->Op==op) return 1;
  return 0;
}


void MainWindow::UpdateViews()
{
  for(sInt i=0;i<sCOUNTOF(ViewWin);i++)
    ViewWin[i]->Update();
}

void MainWindow::DeselectHandles()
{
  for(sInt i=0;i<sCOUNTOF(ViewWin);i++)
  {
    ViewWin[i]->pi.Dragging = 0;
    ViewWin[i]->pi.ClearHandleSelection();
  }
}

void MainWindow::CmdAddType(sDInt nr)
{
  if(nr==-1)
    AddType = 0;
  else if(nr>=0 && nr<Doc->Types.GetCount())
    AddType = Doc->Types[nr];
  AddPopup2();
}

void MainWindow::AddPopup(sMessage msg,sBool sec,sInt typekey)
{
  AddPopupMessage = msg;
  AddTypeSec = sec;
  sGui->GetMousePos(AddPopupX,AddPopupY);

  AddPopup2(typekey);
}

void MainWindow::AddConversionPopup(sMessage msg)
{
  wClass *cl,*cl2;
  sString<256> buffer;

  AddPopupMessage = msg;
  sGui->GetMousePos(AddPopupX,AddPopupY);

  sMenuFrame *mf = new sMenuFrame(this);
  sFORALL(Doc->Conversions,cl)
  {
    buffer.PrintF(L"%s -> %s",cl->Inputs[0].Type->Label,cl->OutputType->Label);
    AddPopupMessage.Code = -1;
    sFORALL(Doc->Classes,cl2)
      if(cl==cl2)
        AddPopupMessage.Code = _i;

    mf->AddItem(sPoolString(buffer),AddPopupMessage,0);
  }
  mf->PopupParent = (sWindow *)AddPopupMessage.Target;
  sGui->AddWindow(mf,AddPopupX,AddPopupY);
}

void MainWindow::AddPopup2(sInt typekey)
{
  wClass *cl;
  wType *type;
  sInt n;
  sMenuFrame *mf = new sMenuFrame(this);

  n = '1';
  mf->AddHeader(L"types",0);
  mf->AddItem(L"misc.",sMessage(this,&MainWindow::CmdAddType,-1),n,-1,0,0);
  if(typekey==n)
    AddType=0;

  sFORALL(Doc->Types,type)
  {
    if(!(type->Flags & wTF_NOTAB) && type != AnyTypeType && type->Secondary==AddTypeSec)
    {
      n++;
      mf->AddItem(type->Label,sMessage(this,&MainWindow::CmdAddType,_i),n<='9'?n:0,-1,0,type->Color);
      if(typekey==n)
        AddType=type;
    }
  }

  sU32 colmask = 0;
  sFORALL(Doc->Classes,cl)
  {
    sBool misc = (App->AddType==0 && (cl->TabType->Flags & wTF_NOTAB));
    if((cl->TabType == App->AddType || cl->TabType == AnyTypeType || misc) && !(cl->Flags & wCF_HIDE))
    {
      sU32 sc = cl->Shortcut;
      if(sc>='A' && sc<='Z') sc |= sKEYQ_SHIFT;
      AddPopupMessage.Code = _i;
      const sChar *name = cl->Label;
      if(misc)
      {
        sString<256> buffer;
        sSPrintF(buffer,L"%s (%s)",cl->Label,cl->TabType->Label);
        name = sPoolString(buffer);
      }
      sInt col = sClamp(cl->Column,0,30);
      if((colmask & (1<<col))==0)
      {
        colmask |= (1<<col);
        mf->AddHeader(misc && col!=30 ? L"" : cl->TabType->ColumnHeaders[col],col+1);
      }
      mf->AddItem(name,AddPopupMessage,sc,-1,col+1,(misc || cl->TabType->Color!=cl->OutputType->Color) ? cl->OutputType->Color : 0);
    }
  }

  mf->PopupParent = (sWindow *)AddPopupMessage.Target;
  sGui->AddWindow(mf,AddPopupX,AddPopupY);
}

void MainWindow::StartCustomEditor(wCustomEditor *ced)
{
  CustomWin->Set(ced);
  SwitchPage(2);
  sGui->SetFocus(CustomWin);
}

void MainWindow::SwitchPage(sInt mode)
{
  sInt n= sWire->GetSubSwitch(L"StackOrTree");
  if(n!=mode)
  {
    if(n==3)
      Wiki->Close();
    sWire->SetSubSwitch(L"StackOrTree",mode);
  }
  WikiToggleFlag = (mode==3);
}

void MainWindow::SwitchPageToggle(sInt mode)
{
  sInt n= sWire->GetSubSwitch(L"StackOrTree");
  if(n==3)
    Wiki->Close();
  if(n!=mode)
  {
    sWire->SetSubSwitch(L"StackOrTree",mode);
    WikiToggleFlag = (mode==3);
  }
  else
  {
    wPage *page = PageListWin->GetSelected();
    if(page && page->IsTree)
      sWire->SetSubSwitch(L"StackOrTree",1);
    else
      sWire->SetSubSwitch(L"StackOrTree",0);
    WikiToggleFlag = 0;
  }
}

/****************************************************************************/

void MainWindow::CalcWasDone()
{
  TimelineWin->SetRange(0,Doc->DocOptions.Beats*0x10000);
  App->OnCommand(sCMD_TIMER);
}

sInt MainWindow::GetTimeBeats()
{
  return TimelineWin->Time;
}

sInt MainWindow::GetTimeMilliseconds()
{
  return Doc->BeatsToMilliseconds(TimelineWin->Time);
}

void MainWindow::ScratchTime()
{
  SamplePos = Doc->BeatsToSample(TimelineWin->Time);
}

void MainWindow::SoundHandler(sS16 *samples,sInt count)
{
  static sInt t0,t1;
  static sInt pauset;
  static const sInt paused = 0x1000;
  static sInt po0,po1;
  sInt vol = Doc->EditOptions.Volume * 0x4000/100;

  if(TimelineWin->Start<TimelineWin->End && !TimelineWin->Pause)
  {
    sInt period = sDivShift(Doc->DocOptions.SampleRate,Doc->DocOptions.BeatsPerSecond);

    sInt start,end;
    if(TimelineWin->LoopEnable && TimelineWin->LoopStart<TimelineWin->LoopEnd)
    {
      start = Doc->BeatsToSample(TimelineWin->LoopStart);
      end   = Doc->BeatsToSample(TimelineWin->LoopEnd  );
    }
    else
    {
      start = Doc->BeatsToSample(TimelineWin->Start);
      end   = Doc->BeatsToSample(TimelineWin->End  );
    }

    if(MusicData)
    {
      if(TimelineWin->ScratchMode)
      {
        sInt p = SamplePos;
        for(sInt i=0;i<count;i++)
        {
          if(pauset==0)
            po0 = p;
          if(pauset==paused/2)
            po1 = p;
          sInt f0 = pauset&(paused-1);
          sInt f1 = (pauset+paused/2)&(paused-1);
          sInt p0 = (po0+f0) % MusicSize;
          sInt p1 = (po1+f1) % MusicSize;
          sInt v0 = sInt(sFSin(sPIF*f0/paused)*0x8000);
          sInt v1 = sInt(sFSin(sPIF*f1/paused)*0x8000);
          samples[i*2+0] = sClamp(((MusicData[p0*2+0]*v0)>>15) + ((MusicData[p1*2+0]*v1)>>15),-0x7fff,0x7fff)*vol/0x4000;
          samples[i*2+1] = sClamp(((MusicData[p0*2+1]*v0)>>15) + ((MusicData[p1*2+0]*v1)>>15),-0x7fff,0x7fff)*vol/0x4000;

          pauset++;
          if(pauset>=paused)
            pauset-=paused;
        }
      }
      else
      {
        sInt p = SamplePos;
        for(sInt i=0;i<count;i++)
        {
          sInt pos = sMax(0,p) % MusicSize;
          samples[i*2+0] = MusicData[pos*2+0]*vol/0x4000;
          samples[i*2+1] = MusicData[pos*2+1]*vol/0x4000;

          p++;
          if(p>=end)
            p = p - end + start;
        }
        SamplePos = p;
      }
    }
    else
    {
      for(sInt i=0;i<count;i++)
      {
        sInt a = sMax(0x7fff - (SamplePos % period)*3,0);
        samples[i*2+0] = sInt(sFSin((t0&0xffffff)*sPI2F/0x1000000)*a)*vol/0x4000;
        samples[i*2+1] = sInt(sFSin((t1&0xffffff)*sPI2F/0x1000000)*a)*vol/0x4000;
        t0 += 0x18000;
        t1 += 0x18200;
        if(!TimelineWin->ScratchMode)
        {
          SamplePos++;
          if(SamplePos>=end)
            SamplePos = SamplePos - end + start;
        }
      }
    }
  }
  else
  {
    for(sInt i=0;i<count*2;i++)
      samples[i] = 0;
  }
  SampleOffset += count;
}

void MainWindow::CmdStartTime()
{
  TimelineWin->Pause = !TimelineWin->Pause;
}

void MainWindow::CmdLoopTime()
{
  if(TimelineWin->LoopStart<TimelineWin->LoopEnd)
  {
    TimelineWin->LoopEnable = !TimelineWin->LoopEnable;
  }
  else
  {
    TimelineWin->LoopEnable = 0;
  }
  TimelineWin->Update();
}

void MainWindow::CmdRestartTime()
{
  if(TimelineWin->LoopStart<TimelineWin->LoopEnd)
  {
    TimelineWin->TimeMin = TimelineWin->LoopStart;
    SamplePos = Doc->BeatsToSample(TimelineWin->LoopStart);
  }
  else
  {
    TimelineWin->TimeMin = TimelineWin->Start;
    SamplePos = Doc->BeatsToSample(TimelineWin->Start);
  }
  TimelineWin->Pause = 0;
}

/****************************************************************************/

static sArray<StoreBrowserItem *> *UpdateStoreTreeGlobal;
static wType *UpdateStoreTreeGlobalFilter;

static void UpdateStoreTreeCallback(const sChar *name,wType *type)
{
  if(UpdateStoreTreeGlobalFilter==0 || type->IsTypeOrConversion(UpdateStoreTreeGlobalFilter))
  {
    StoreBrowserItem *dest;
    dest = new StoreBrowserItem;
    sClear(dest->TreeInfo);
    dest->Op = 0;
    dest->Name = name;
    UpdateStoreTreeGlobal->AddTail(dest);
  }
}

struct BrowserItemSort
{
  sBool operator()(const StoreBrowserItem *a_, const StoreBrowserItem *b_)const
  {
    const sChar *a = a_->Name;
    const sChar *b = b_->Name;
    sInt aa,bb;
    do
    {
      aa = *a++;
      if(aa>='a' && aa<='z')
        aa=aa-'a'+'A';
      if(aa=='_')
        aa = 1;
      bb = *b++;
      if(bb>='a' && bb<='z')
        bb=bb-'z'+'Z';
      if(bb=='_')
        bb = 1;
    }
    while(aa!=0 && aa==bb);
    return sSign(aa-bb)<0;
  }
};


void MainWindow::UpdateStoreTree(wType *filter)
{
  wOp *op;
  wObject *obj;
  StoreBrowserItem *dest,*src;
  sDeleteAll(StoreTree);
  const sChar *lastname = L"";
  const sChar *a,*b;
  sChar *d;
  sString<256> buffer;

  sArray<StoreBrowserItem *> temp;
  UpdateStoreTreeGlobal = &temp;
  UpdateStoreTreeGlobalFilter = filter;

  sFORALL(Doc->Stores,op)
  {
    wOp *follow = op;
    while(follow->Class->OutputType==AnyTypeType && follow->Inputs.GetCount()>0 && follow->Inputs[0])
      follow = follow->Inputs[0];
    if(filter==0 || follow->Class->OutputType->IsTypeOrConversion(filter))
    {
      dest = new StoreBrowserItem;
      sClear(dest->TreeInfo);
      dest->Op = op;
      dest->Name = op->Name;
      temp.AddTail(dest);
    }

    if(!op->Name.IsEmpty())
    {
      obj = Doc->Builder->FindCache(op);
      if(obj)
      {
        obj->Type->ListExtractions(obj,UpdateStoreTreeCallback,op->Name);
        obj->Release();
      }
    }
  }
  UpdateStoreTreeGlobal = 0;

  sIntroSort(sAll(temp),BrowserItemSort());

  sFORALL(temp,src)
  {
    if(!src->Name.IsEmpty())
    {
      // add empty directories as required

      a = src->Name;
      b = lastname;
      d = buffer;
      sInt level = 0;
      while(*a==*b && *a)
      {
        if(*a=='.' || *a==':' || *a=='-')
          level++;
        *d++ = *a++;
        b++;
      }
      while(*a)
      {
        if(*a=='.' || *a==':' || *a=='-')
        {
          *d = 0;
          if(sCmpString(lastname,buffer)!=0)   // sometimes, the dir name is already an object name, then skip it.
          {
            dest = new StoreBrowserItem;
            sClear(dest->TreeInfo);
            dest->Op = 0;
            dest->Name = buffer;
            dest->TreeInfo.Level = level;
            dest->TreeInfo.Flags = sLWTI_CLOSED;
            StoreTree.AddTail(dest);
          }
          level++;
        }
        *d++ = *a++;
      }

      // add the item itself

      src->TreeInfo.Level = level;
      StoreTree.AddTail(src);
      lastname = src->Name;
    }
  }
  StoreBrowserWin->UpdateTreeInfo();
}

void MainWindow::StartStoreBrowser(const sMessage &cmd,const sChar *oldname,wType *filter)
{
  UpdateStoreTree(filter);
  StoreBrowserWin->OpMsg = cmd;
  sWire->Popup(L"Picker");
  if(oldname)
    StoreBrowserWin->Goto(oldname);
}

void MainWindow::PopupText(const sChar *text)
{
  PopupTextBuffer.Clear();
  PopupTextBuffer.Print(text);
  PopupTextWin->SetText(&PopupTextBuffer);
  sWire->Popup(L"PopupTextPopup");
}

/****************************************************************************/

void MainWindow::CmdNew()
{
  if(Doc->DocChanged)
    sContinueReallyDialog(sMessage(this,&MainWindow::CmdNew2),sMessage(this,&MainWindow::CmdSaveNew));
  else
    CmdNew2();
}

void MainWindow::CmdNew2()
{
  Doc->New();
  Doc->DefaultDoc();
  Status->PrintF(STATUS_MESSAGE,L"new document");
  ResetWindows();
}

void MainWindow::CmdOpen()
{
  if(Doc->DocChanged)
    sContinueReallyDialog(sMessage(this,&MainWindow::CmdOpen2),sMessage(this,&MainWindow::CmdSaveOpen));
  else
    CmdOpen2();
}

void MainWindow::CmdOpen2()
{
  CmdPanic();
  sOpenFileDialog(L"open",L"wz4",sSOF_LOAD,Doc->Filename,sMessage(this,&MainWindow::CmdOpen3),sMessage());
}

void MainWindow::CmdOpen3()
{
  CmdPanic();

  if(!Doc->Load(Doc->Filename))
  {
    sErrorDialog(sMessage(),L"could not load file");
  }
  else
  {
    CheckAfterLoading();
    OnDocOptionsChanged();
    CalcWasDone();
    CmdReloadMusic();
  }
  ResetWindows();
}

void MainWindow::CmdMerge()
{
  CmdPanic();
  sOpenFileDialog(L"merge",L"wz4",sSOF_LOAD,MergeFilename,sMessage(this,&MainWindow::CmdMerge2),sMessage());
}

void MainWindow::CmdMerge2()
{
  CmdPanic();

  wDocument *MasterDoc = Doc;
  wDocument *MergeDoc = new wDocument;

  if(!MergeDoc->Load(MergeFilename))
  {
    sErrorDialog(sMessage(),L"could not load file");
  }
  else
  {
    wOp *op;
    MergeDoc->Connect();
    Doc = MasterDoc;

    sFORALL(MergeDoc->AllOps,op)
      op->Class = Doc->FindClass(op->Class->Name,op->Class->OutputType->Symbol);

    MasterDoc->Pages.Add(MergeDoc->Pages);
    CheckAfterLoading();
    OnDocOptionsChanged();
    Doc->Connect();
  }
  ResetWindows();
}

void MainWindow::CmdOpenBackup()
{
  CmdPanic();
  BackupFilename = BackupDir;
  BackupFilename.AddPath(L"!1_autosave.wz4");
  sOpenFileDialog(L"open",L"wz4",sSOF_LOAD,BackupFilename,sMessage(this,&MainWindow::CmdOpenBackup2),sMessage());
}

void MainWindow::CmdOpenBackup2()
{
  CmdPanic();
  sString<sMAXPATH> filename = Doc->Filename;

  if(!Doc->Load(BackupFilename))
  {
    sErrorDialog(sMessage(),L"could not load file");
  }
  else
  {
    CheckAfterLoading();
    OnDocOptionsChanged();
  }
  Doc->Filename = filename;
  ResetWindows();
}


void MainWindow::CheckAfterLoading()
{
  if(!Doc->DocOptions.PageName.IsEmpty())
  {
    wPage *page;
    page = sFind(Doc->Pages,&wPage::Name,Doc->DocOptions.PageName);
    if(page)
    {
      PageListWin->SetSelected(page);
      StackWin->SetPage(page);
    }
  }
  if(Doc->UnknownOps>0)
  {
    sErrorDialog(sMessage(),L"Unknown Ops.\n\n"
      L"The file contains operator classes that are not recognised by the werkkzeug any more\n"
      L"When you save this file, work may be lost."
      L"Please check the file for <UnknownOp> operators.\n");
    Status->PrintF(STATUS_MESSAGE,L"loaded <%s>: unknown ops",Doc->Filename,-1,0xffff0000);
  }
  else
  {
    Status->PrintF(STATUS_MESSAGE,L"loaded <%s>",Doc->Filename);
    Status->SetColor(STATUS_MESSAGE,0xff00ff00);
  }
}
void MainWindow::CmdSaveAs()
{
  sOpenFileDialog(L"save as",L"wz4",sSOF_SAVE,Doc->Filename,sMessage(this,&MainWindow::CmdSave),sMessage());
}

void MainWindow::CmdSave()
{
  if(sCheckFile(Doc->Filename))
  {
    if(!sCOMMANDLINE)
    {
      sString<sMAXPATH> backup;
      BackupIndex++;
      backup.PrintF(L"%s/%05d#%s",BackupDir,BackupIndex,sFindFileWithoutPath(Doc->Filename));
      if(sCopyFile(Doc->Filename,backup,1))
        CmdSave2();
      else
        sYesNoDialog(sMessage(this,&MainWindow::CmdSave2),sMessage(),L"backup failed. save anyway?");
    }
  }
  else
  {
    CmdSave2();
  }
}

void MainWindow::CmdAutoSave()
{
  if(!sCOMMANDLINE)
  {
    sString<sMAXPATH> from,to;
    from.PrintF(L"%s/!9_autosave.wz4",BackupDir);
    sDeleteFile(from);

    for(sInt i=8;i>=1;i--)
    {
      from.PrintF(L"%s/!%d_autosave.wz4",BackupDir,i);
      to.PrintF(L"%s/!%d_autosave.wz4",BackupDir,i+1);
      sRenameFile(from,to);
    }
    to.PrintF(L"%s/!1_autosave.wz4",BackupDir);
    sBool sc = Doc->DocChanged;
    if(sSaveObject(to,Doc))
    {
      Status->PrintF(STATUS_MESSAGE,L"autosave ok");
    }
    else
    {
      Status->PrintF(STATUS_MESSAGE,L"autosave failed");
      Status->SetColor(STATUS_MESSAGE,0xff00ff00);
    }
    Doc->DocChanged = sc;
    AutosaveTimer = 0;
  }
}

void MainWindow::CmdSave2()
{
  if(!Doc->Save(Doc->Filename))
    sErrorDialog(sMessage(),L"could not save file");
  else
  {
    wDocInclude *inc;
    sFORALL(Doc->Includes,inc)
      if(!inc->Protected)
        inc->Save();
    Status->PrintF(STATUS_MESSAGE,L"saved <%s>",Doc->Filename);
    Status->SetColor(STATUS_MESSAGE,0xff00ff00);
  }
  UpdateWindows();
}

void MainWindow::CmdExit()
{
  if(Doc->DocChanged)
    sQuitReallyDialog(sMessage(this,&MainWindow::CmdExit2),sMessage(this,&MainWindow::CmdSaveQuit));
//    sYesNoDialog(sMessage(this,&MainWindow::CmdExit2),sMessage(),L"The document has changed.\nPress YES to quit without saving.");
  else
    CmdExit2();
}

void MainWindow::CmdExit2()
{
  CmdSaveConfig();
  sExit();
}

void MainWindow::CmdSaveQuit()
{
  CmdSaveConfig();
  if(!Doc->Save(Doc->Filename))
    sErrorDialog(sMessage(),L"could not save file");
  else
    sExit();
}

void MainWindow::CmdSaveNew()
{
  if(!Doc->Save(Doc->Filename))
    sErrorDialog(sMessage(),L"could not save file");
  else
    CmdNew2();
}

void MainWindow::CmdSaveOpen()
{
  if(!Doc->Save(Doc->Filename))
    sErrorDialog(sMessage(),L"could not save file");
  else
    CmdOpen2();
}

void MainWindow::CmdPanic()
{
  for(sInt i=0;i<sCOUNTOF(ViewWin);i++)
    ViewWin[i]->SetOp(0);
}

void MainWindow::CmdFlushCache()
{
  Doc->FlushCaches();
  CmdPanic();
  Update();
}

void MainWindow::CmdFlushCache2()
{
  Doc->FlushCaches();
  Update();
}

void MainWindow::CmdChargeCache()
{
  Doc->ChargeCaches();
}

void MainWindow::CmdUnblockChange()
{
  Doc->UnblockChange();
}

void MainWindow::CmdEditOptions()
{
  if(!EditOptionsWin)
  {
    EditOptionsWin = new sGridFrame;
    sScreenInfo si;
    sGetScreenInfo(si);
    EditOptionsBuffer = Doc->EditOptions;
    sGridFrameHelper gh(EditOptionsWin);

    gh.Group(L"On Startup");
    gh.Label(L"Load File");
    gh.String(&Doc->EditOptions.File);
    gh.Label(L"Goto Screen");
    gh.Choice(&Doc->EditOptions.Screen,L"page|dual|tree");
    gh.Label(L"Flags");
    gh.Flags(&Doc->EditOptions.Flags,L"-|ignore slow:*1-|gray unconnected");
    gh.Group(L"Colors");
    gh.Label(L"Background");
    gh.ColorPick(&Doc->EditOptions.BackColor,L"rgba",0);
    gh.Label(L"Grid");
    gh.ColorPick(&Doc->EditOptions.GridColor,L"rgb",0);
    gh.Label(L"Ambient");
    gh.ColorPick(&Doc->EditOptions.AmbientColor,L"rgb",0);
    gh.Label(L"Spline Display");
    gh.Flags(&Doc->EditOptions.SplineMode,L"*0speed|exact:*1no target|target dir|target curve:*3-|tilt");
    gh.Group(L"Camera");
    gh.Label(L"Clip Near");
    gh.Float(&Doc->EditOptions.ClipNear,0.01f,16.0f,0.01f);
    gh.Label(L"Clip Far");
    gh.Float(&Doc->EditOptions.ClipFar,1,1024*1024,256);
    gh.Label(L"Move Speed (old)");
    gh.Flags(&Doc->EditOptions.MoveSpeed,L"slow|normal|fast|ultra");
    gh.Label(L"Default Camera Speed");
    gh.Int(&Doc->EditOptions.DefaultCamSpeed,-40,40,0.25f);
    gh.PushButton(L"Use Current",sMessage(this,&MainWindow::CmdSetDefaultCamSpeed));
    gh.Label(L"Multisample Level");
    gh.Int(&Doc->EditOptions.MultiSample,0,si.MultisampleLevels)->ChangeMsg = sMessage(this,&MainWindow::CmdUpdateMultisample,0);
    gh.Label(L"FPS Font Size");
    gh.Int(&Doc->EditOptions.ZoomFont,1,4);
    gh.Group(L"Misc");
    gh.Label(L"Autosave Period (seconds)");
    gh.Int(&Doc->EditOptions.AutosavePeriod,0,60*60);
    gh.Label(L"Memory Limit (MB) (0=off)");
    gh.Int(&Doc->EditOptions.MemLimit,0,16*1024,256);
    gh.Label(L"Expensive IPP Quality");
    gh.Choice(&Doc->EditOptions.ExpensiveIPPQuality,L"low|medium|high");
    gh.Label(L"GUI theme");
    gh.Choice(&Doc->EditOptions.Theme,L"Altona default|darker|custom")->ChangeMsg = sMessage(this,&MainWindow::CmdUpdateTheme);
    gh.PushButton(L"edit",sMessage(this,&MainWindow::CmdEditTheme),0);

  //  gh.Label(L"Minimal Timeline");
  //  gh.Int(&Doc->EditOptions.MinimalTimeline,0,1024)->ChangeMsg = sMessage(this,&MainWindow::CmdChangeTimeline);

    gh.Group();
    gh.Box(L"OK",sMessage(this,&MainWindow::CmdEditOptionsOk,1));
    gh.Box(L"Cancel",sMessage(this,&MainWindow::CmdEditOptionsOk,0));

    gh.Grid->ReqSizeX = 600;
  //  gh.Grid->Flags |= sWF_AUTOKILL;
    sGui->AddFloatingWindow(gh.Grid,L"Editor Options");
  }
}

void MainWindow::CmdEditOptionsOk(sDInt ok)
{
  if(!ok)
  {
    Doc->EditOptions = EditOptionsBuffer;
    Doc->EditOptions.ApplyTheme();
  }
  EditOptionsWin->Close();
  EditOptionsWin = 0;
  sGui->SetFocus(this);
}

void MainWindow::CmdSetDefaultCamSpeed()
{
  Doc->EditOptions.DefaultCamSpeed = ViewWin[0]->GearShift;
  sGui->Notify(Doc->EditOptions.DefaultCamSpeed);
}

void MainWindow::CmdDocOptions()
{
  if(DocOptionsWin)
  {
    DocOptionsWin->Close();
    DocOptionsWin = 0;
  }
  if(!DocOptionsWin)
  {
    DocOptionsWin = new sGridFrame;
    DocOptionsBuffer = Doc->DocOptions;
    sGridFrameHelper gh(DocOptionsWin);
    sIntControl *coni;
    sFloatControl *conf;

    gh.Label(L"Goto Page");
    gh.String(&Doc->DocOptions.PageName);
    gh.Label(L"Project Path");
    gh.String(&Doc->DocOptions.ProjectPath);
    gh.Label(L"Packfiles, seperated by '|'");
    gh.String(&Doc->DocOptions.Packfiles);
    gh.Label(L"Project Name");
    gh.String(&Doc->DocOptions.ProjectName);
    gh.Label(L"Project ID");
    gh.Int(&Doc->DocOptions.ProjectId,-100000,100000,1);
    gh.Label(L"Project Site ID");
    gh.Int(&Doc->DocOptions.SiteId,0,100000,1);
    gh.Label(L"Texture Quality");
    gh.Choice(&Doc->DocOptions.TextureQuality,L"full|DXT|low");
    gh.Label(L"Level of Detail");
    gh.Choice(&Doc->DocOptions.LevelOfDetail,L"low|medium|high|extra");
    gh.Label(L"Beats Per Minute");
    BPM = Doc->DocOptions.BeatsPerSecond*60.0f/0x10000;
    conf = gh.Float(&BPM,1,1000,0.0001);
    conf->ChangeMsg = sMessage(this,&MainWindow::CmdBpmChange);
    conf->RightStep = 0.25f;
    gh.BoxToggle(L"Variable",&Doc->DocOptions.VariableBpm,sMessage(this,&MainWindow::CmdDocOptions));
    gh.Label(L"Beat Start");
    coni = gh.Int(&Doc->DocOptions.BeatStart,-0x100000,0x100000,1,0,L"%06x");
    coni->ChangeMsg = sMessage(this,&MainWindow::Update);
    coni->RightStep = 0x100;
    gh.Label(L"Beats (Max Timeline)");
    coni = gh.Int(&Doc->DocOptions.Beats,1,4096,16);
    coni->ChangeMsg = sMessage(this,&MainWindow::CmdChangeTimeline);
    coni->Default = 32;
    gh.Box(L"auto",sMessage(this,&MainWindow::CmdMusicLength));
    gh.Label(L"Music (ogg)");
    gh.String(&Doc->DocOptions.MusicFile);
    gh.Box(L"...",sMessage(this,&MainWindow::CmdMusicDialog));
    gh.Box(L"reload",sMessage(this,&MainWindow::CmdReloadMusic));
    gh.Label(L"Resolution");
    gh.Int(&Doc->DocOptions.ScreenX,16,1920)->Default = 800;
    gh.Int(&Doc->DocOptions.ScreenY,16,1200)->Default = 600;

    gh.Label(L"Player Dialog");
    gh.Flags(&Doc->DocOptions.DialogFlags,L"-|Benchmark:*1-|Multithreading:*2-|override resolution:*3-|low quality:*4loop button|-:*5-|hidden parts");
    gh.Label(L"Quality Flag");
    gh.Flags(&Doc->LowQuality,L"high|low");

    wDocInclude *inc;
    gh.Group(L"Includes");
    sFORALL(Doc->Includes,inc)
    {
      gh.Label(L"Filename");
      gh.String(inc->Filename);
      gh.Box(L"rem",sMessage(this,&MainWindow::CmdRemInclude,_i));
      gh.Box(L"...",sMessage(this,&MainWindow::CmdLoadInclude,_i));
      if(!inc->Active && !sCheckFile(inc->Filename))
        gh.Box(L"new",sMessage(this,&MainWindow::CmdCreateInclude,_i));
      if(!inc->Active && sCheckFile(inc->Filename))
        gh.Box(L"load",sMessage(this,&MainWindow::CmdLoadInclude2,_i));
    }
    gh.NextLine();
    gh.Box(L"add",sMessage(this,&MainWindow::CmdAddInclude));
    gh.Box(L"update",sMessage(this,&MainWindow::CmdDocOptions));
    if(Doc->DocOptions.VariableBpm)
    {
      wDocOptions::BpmSegment *s;
      gh.Group(L"Variable BPM");
      gh.Label(L"");
      gh.NextLine();
      sFORALL(Doc->DocOptions.BpmSegments,s)
      {
        gh.Label(L"Beats / BPM");
        gh.Int(&s->Beats,1,4096)->ChangeMsg = sMessage(this,&MainWindow::CmdUpdateVarBpm);
        gh.Float(&s->Bpm,20,300,0.01f)->ChangeMsg = sMessage(this,&MainWindow::CmdUpdateVarBpm);
        gh.Box(L"Add",sMessage(this,&MainWindow::CmdAddBpm,_i),sGFLF_HALFUP);
        gh.Box(L"Rem",sMessage(this,&MainWindow::CmdRemBpm,_i));
      }
      gh.NextLine();
      gh.Box(L"Add",sMessage(this,&MainWindow::CmdAddBpm,Doc->DocOptions.BpmSegments.GetCount()),sGFLF_HALFUP);
    }
    gh.Group(L"Hidden Parts");
    wDocOptions::HiddenPart *hp;
    sFORALL(Doc->DocOptions.HiddenParts,hp)
    {
      gh.Label(L"Code / Beat Length");
      gh.String(&hp->Code);
      gh.Int(&hp->LastBeat,1,65536);
      gh.Box(L"Rem",sMessage(this,&MainWindow::CmdRemHiddenPart,_i));
      gh.Label(L"Song / BPM");
      gh.String(&hp->Song);
      gh.Float(&hp->Bpm,1,240)->Default=120;
      gh.Label(L"Store / Flags");
      gh.String(&hp->Store,gh.WideWidth-2);
      gh.ControlWidth = 1;
      gh.Flags(&hp->Flags,L"-|infinite:*1hidden|dialog:*2-|freeflight");
      gh.ControlWidth = 2;
    }
    gh.Label(L"");
    gh.Box(L"Add",sMessage(this,&MainWindow::CmdAddHiddenPart,Doc->DocOptions.HiddenParts.GetCount()));

    gh.Group();
    gh.Box(L"OK",sMessage(this,&MainWindow::CmdDocOptionsOk,1));
    gh.Box(L"Cancel",sMessage(this,&MainWindow::CmdDocOptionsOk,0));

    gh.Grid->ReqSizeX = 600;
//  gh.Grid->Flags |= sWF_AUTOKILL;
    sGui->AddFloatingWindow(gh.Grid,L"Document Options");
  }
}

void MainWindow::CmdBpmChange()
{
  sF32 bps = sInt(BPM * 0x10000 / 60);
  if(bps!=Doc->DocOptions.BeatsPerSecond)
  {
    Doc->DocOptions.BeatsPerSecond = bps;
    CalcWasDone();
    TimelineWin->Update();
    Update();
  }
}

void MainWindow::CmdUpdateVarBpm()
{
  Doc->DocOptions.UpdateTiming();
}

void MainWindow::CmdAddBpm(sDInt i)
{
  wDocOptions::BpmSegment e;
  e.Beats = 1;
  e.Bpm = 120;
  Doc->DocOptions.BpmSegments.AddBefore(e,i);
  Doc->DocOptions.UpdateTiming();
  CmdDocOptions();
}

void MainWindow::CmdRemBpm(sDInt i)
{
  Doc->DocOptions.BpmSegments.RemAtOrder(i);
  Doc->DocOptions.UpdateTiming();
  CmdDocOptions();
}

void MainWindow::CmdAddHiddenPart(sDInt i)
{
  wDocOptions::HiddenPart e;
  e.Code = L"secret";
  e.Store = L"hidden";
  e.Song = L"";
  e.LastBeat = 1024;
  e.Bpm = 120;
  e.Flags = 0;
  
  Doc->DocOptions.HiddenParts.AddBefore(e,i);
  CmdDocOptions();
}

void MainWindow::CmdRemHiddenPart(sDInt i)
{
  Doc->DocOptions.HiddenParts.RemAtOrder(i);
  CmdDocOptions();
}

void MainWindow::CmdChangeTimeline()
{
  CalcWasDone();
  TimelineWin->Update();
}

void MainWindow::CmdMusicLength()
{
  // time(s) = samples(n) / freq(n/s)
  // beats(n) = bps(b/s) * time(s)

  Doc->DocOptions.Beats = 1+sInt(sF64(MusicSize)/sF64(Doc->DocOptions.SampleRate)*sF64(Doc->DocOptions.BeatsPerSecond/0x10000));
  sGui->Notify(Doc->DocOptions.Beats);
  CmdChangeTimeline();
}

void MainWindow::CmdSetAmbient(sDInt n)
{
  Doc->EditOptions.AmbientColor = n ? 0xffffffff : 0xff000000;
  Update();
}

void MainWindow::CmdUpdateTheme()
{
  Doc->EditOptions.ApplyTheme();
}

void MainWindow::CmdEditTheme()
{
  sOpenGuiThemeDialog(&Doc->EditOptions.CustomTheme);
}

void MainWindow::CmdMusicDialog()
{
  sOpenFileDialog(L"Open Music File",L"ogg",sSOF_LOAD,Doc->DocOptions.MusicFile,sMessage(this,&MainWindow::CmdReloadMusic),sMessage());
}

void MainWindow::CmdReloadMusic()
{
  sDeleteArray(MusicData);
  MusicSize = 0;

  sDirEntry mfentry, cfentry;
  if (sGetFileInfo(Doc->DocOptions.MusicFile,&mfentry))
  {
    sString<sMAXPATH> cachefile;
    cachefile.PrintF(L"%s.raw",Doc->DocOptions.MusicFile);

    // generate raw cache if not found or out of date
    if (!sGetFileInfo(cachefile,&cfentry) || mfentry.LastWriteTime!=cfentry.LastWriteTime)
    {
      sDInt oggsize;
      sU8 *oggdata=sLoadFile(Doc->DocOptions.MusicFile,oggsize);
      sFile *cf=sCreateFile(cachefile,sFA_WRITE);
      if (oggdata&&cf)
      {
        sInt error=0;
        stb_vorbis *dec=stb_vorbis_open_memory(oggdata,oggsize,&error,0);
        if (!error)
        {
          const sInt BSIZE=65536;
          sInt read;
          sS16 buffer[BSIZE];
          do
          {
            read=stb_vorbis_get_samples_short_interleaved(dec,2,buffer,BSIZE);
            cf->Write(buffer,4*read);
          } while (read);
          sDelete(cf);
          sSetFileTime(cachefile,mfentry.LastWriteTime);
        }
        stb_vorbis_close(dec);
      }
      sDelete(cf);
      delete[] oggdata;
    }

    // load cache file
    sDInt cfsize=0;
    MusicData = (sS16*)sLoadFile(cachefile,cfsize);
    MusicSize = cfsize/4;
  }
}

void MainWindow::CmdDocOptionsOk(sDInt ok)
{
  if(!ok)
  {
    Doc->DocOptions = DocOptionsBuffer;     // undo changes
  }
  else
  {
    if(sCmpMem(&Doc->DocOptions,&DocOptionsBuffer,sizeof(DocOptionsBuffer))!=0)
      ChangeDoc();
  }
  DocOptionsWin->Close();
  DocOptionsWin = 0;
  sGui->SetFocus(this);
}

void MainWindow::CmdTestOptions()
{
  if(TestOptionsWin)
  {
    TestOptionsWin->Close();
    TestOptionsWin = 0;
  }
  if(!TestOptionsWin)
  {
    TestOptionsWin = new sGridFrame;
    TestOptionsBuffer = Doc->TestOptions;
    sGridFrameHelper gh(TestOptionsWin);

    gh.Label(L"Mode");
    gh.Choice(&Doc->TestOptions.Mode,L"Compare|Write|Compare to Platform");
    gh.Label(L"Compare To Platform");
    gh.Choice(&Doc->TestOptions.Compare,L"Unknown|DirectX9|DirectX11");
    gh.Label(L"Fail in Red");
    gh.Flags(&Doc->TestOptions.FailRed,L"-|fatal");
    gh.Label(L"Recalc");
    gh.Button(L"Recalc Now",sMessage(this,&MainWindow::CmdFlushCache2));

    gh.Group();
    gh.Box(L"OK",sMessage(this,&MainWindow::CmdTestOptionsOk,1));
    gh.Box(L"Cancel",sMessage(this,&MainWindow::CmdTestOptionsOk,0));

    gh.Grid->ReqSizeX = 600;
//  gh.Grid->Flags |= sWF_AUTOKILL;
    sGui->AddFloatingWindow(gh.Grid,L"Unit Test Options");
  }
}

void MainWindow::CmdTestOptionsOk(sDInt ok)
{
  if(!ok)
  {
    TestOptionsBuffer = Doc->TestOptions;     // undo changes
  }  
  TestOptionsWin->Close();
  TestOptionsWin = 0;
  sGui->SetFocus(this);
}


void MainWindow::CmdShellSwitches()
{
  if(ShellSwitchesWin)
  {
    ShellSwitchesWin->Close();
    ShellSwitchesWin = 0;
  }
  if(!ShellSwitchesWin)
  {
    ShellSwitchesWin = new sGridFrame;
    SetShellSwitches();
//  ShellSwitchesWin->Flags |= sWF_AUTOKILL;
    sGui->AddFloatingWindow(ShellSwitchesWin,L"Override Shell Switches");
  }
}
void MainWindow::SetShellSwitches()
{
  sGridFrameHelper gh(ShellSwitchesWin);
  gh.ChangeMsg = sMessage(this,&MainWindow::CmdShellSwitchChange);

  gh.Group(L"operator switches");
  sString<1024> b;
  gh.Left = 0;
  for(sInt i=0;i<wSWITCHES;i++)
  {
    b.PrintF(L"*%d-| %s",i,Doc->ShellSwitchOptions[i]);
    if(i%5==0)
    {
      gh.Label(L" ");
      gh.Left = 0;
    }
    gh.Flags(&Doc->ShellSwitches,sPoolString(b));
  }

  gh.Group(L"script defines");

  wScriptDefine *sd;
  gh.WideWidth = 4;
  sFORALL(Doc->ScriptDefines,sd)
  {
    gh.NextLine();
    gh.Left = 0;
    gh.Flags(&sd->Mode,L"|string|int|float")->ChangeMsg = sMessage(this,&MainWindow::SetShellSwitches);
    gh.String(&sd->Name);
    switch(sd->Mode)
    {
    case 1: gh.String(&sd->StringValue); break;
    case 2: gh.Int(&sd->IntValue,-0x8000000LL,0x7ffffff); break; 
    case 3: gh.Float(&sd->FloatValue,-65536.0f*65536,65536.0f*65536); break; 
    }
    gh.Box(L"rem",sMessage(this,&MainWindow::RemScriptDefine,_i));
  }
  gh.NextLine();
  gh.Box(L"add",sMessage(this,&MainWindow::AddScriptDefine));

  gh.Group(L"");
  gh.Box(L"OK",sMessage(this,&MainWindow::CmdShellSwitchesOk));

  gh.Grid->ReqSizeX = 600;
}

void MainWindow::RemScriptDefine(sDInt n)
{
  if(Doc->ScriptDefines.IsIndexValid(n))
  {
    Doc->ScriptDefines.RemAtOrder(n);
    SetShellSwitches();
  }
}

void MainWindow::AddScriptDefine()
{
  wScriptDefine *sd = Doc->ScriptDefines.AddMany(1);
  sd->Mode = 1;
  sd->Name = L"new";
  sd->StringValue = L"";
  sd->IntValue = 0;
  sd->FloatValue = 0.0f;
  SetShellSwitches();
}

void MainWindow::CmdShellSwitchesOk()
{
  ShellSwitchesWin->Close();
  ShellSwitchesWin = 0;
  sGui->SetFocus(this);
}

void MainWindow::CmdShellSwitchChange()
{
  Doc->Connect();
  sGui->Update();
}

/****************************************************************************/

void MainWindow::CmdAddInclude()
{
  Doc->Includes.AddTail(new wDocInclude);
  CmdDocOptionsOk(1);
  CmdDocOptions();
}

void MainWindow::CmdRemInclude(sDInt i)
{
  if(i>=0 && i<Doc->Includes.GetCount())
  {
    wDocInclude *inc = Doc->Includes[i];
    Doc->Includes.RemOrder(inc);
    inc->Clear();
    CmdDocOptionsOk(1);
    CmdDocOptions();
  }
}

void MainWindow::CmdLoadInclude(sDInt i)
{
  if(i>=0 && i<Doc->Includes.GetCount())
  {
    wDocInclude *inc = Doc->Includes[i];
    if(sSystemOpenFileDialog(L"Select File",L"wz4i",sSOF_SAVE,inc->Filename))
    {
      sInt len = sGetStringLen(Doc->DocOptions.ProjectPath);
      if(sCmpStringPLen(Doc->DocOptions.ProjectPath,inc->Filename,len)==0)
      {
        sString<256> str = inc->Filename;
        sChar *x = str+len;
        if(*x=='/' || *x=='\\') x++;
        inc->Filename = x;
      }

      inc->Load();
    }
    CmdDocOptionsOk(1);
    CmdDocOptions();
  }
}
void MainWindow::CmdLoadInclude2(sDInt i)
{
  if(i>=0 && i<Doc->Includes.GetCount())
  {
    wDocInclude *inc = Doc->Includes[i];
    inc->Load();
    CmdDocOptionsOk(1);
    CmdDocOptions();
  }
}
void MainWindow::CmdCreateInclude(sDInt i)
{
  if(i>=0 && i<Doc->Includes.GetCount())
  {
    wDocInclude *inc = Doc->Includes[i];
    if(inc->Active==0 && !sCheckFile(inc->Filename))
    {
      inc->New();
      CmdDocOptionsOk(1);
      CmdDocOptions();
    }
  }
}

/****************************************************************************/

void MainWindow::CmdLoadConfig()
{
  if(!sCOMMANDLINE)
  {
    sLoadObject<wEditOptions>(L"config_wz4",&Doc->EditOptions);
    if(Doc->EditOptions.Screen<3)
      sWire->SwitchScreen(Doc->EditOptions.Screen);
    else
      sWire->SwitchScreen(0);
    Doc->EditOptions.ApplyTheme();
    CmdUpdateMultisample();
  }
}

void MainWindow::CmdSaveConfig()
{
  if(!sCOMMANDLINE)
  {
    sString<sMAXPATH> path;
    Doc->EditOptions.File = Doc->Filename;
    Doc->EditOptions.Screen = sWire->GetScreen();
    if(Doc->EditOptions.Screen == 2)
      Doc->EditOptions.Screen = 0;

    path = ProgramDir;
    path.AddPath(L"config_wz4");
    sSaveObject<wEditOptions>(path,&Doc->EditOptions);
  }
}

void MainWindow::CmdUpdateMultisample()
{
  sScreenMode sm;
  sScreenInfo si;
  sGetScreenInfo(si);
  if(Doc->EditOptions.MultiSample > si.MultisampleLevels)
    Doc->EditOptions.MultiSample = si.MultisampleLevels;
  if(Doc->EditOptions.MultiSample<0)
    Doc->EditOptions.MoveSpeed = 0;

  sGetScreenMode(sm);
  if(sm.MultiLevel != Doc->EditOptions.MultiSample-1)
  {
    sm.MultiLevel = Doc->EditOptions.MultiSample-1;
    sSetScreenMode(sm);
  }
  Update();
}

void MainWindow::CmdSwitchScreen(sDInt i)
{
  SwitchPage(0);
  sWire->SwitchScreen(i);
}

void MainWindow::CmdRenameAll()
{
  wOp *op = App->GetEditOp();
  if(op && !op->Name.IsEmpty())
  {
    RenameFrom = op->Name;
    RenameTo = op->Name;
    RenameTitle.PrintF(L"Rename all ops from %q to...",op->Name);
    sStringDialog(RenameTo,sMessage(this,&MainWindow::CmdRenameAll2),sMessage(),RenameTitle);
  }
}

void MainWindow::CmdRenameAll2()
{
  if(!Doc->RenameAllOps(RenameFrom,RenameTo))
    sErrorDialog(sMessage(),L"rename failed");
}

void MainWindow::CmdReferences()
{
  wOp *op = GetEditOp();
  if(op && !op->Name.IsEmpty())
  {
    OpListWin->References(op->Name);
    sWire->Popup(L"References");
  }
}


static void IncFindClass(sArray<sPoolString> &list,const sChar *str,void *ref)
{
  wClass *cl;
  sInt len = sGetStringLen(str);
  sFORALL(Doc->Classes,cl)
  {
    if(sCmpStringILen(str,cl->Label,len)==0)
    {
      sPoolString name = sPoolString(cl->Label);
      if(!sFind(list,name))
        list.AddTail(name);
    }
  }
}

void MainWindow::CmdFindClass()
{
  wOp *op = GetEditOp();
  if(op)
    FindBuffer = op->Class->Label;

  sFindDialog(L"Find Class",FindBuffer,sMessage(this,&MainWindow::CmdFindClass2),0,IncFindClass);
}

void MainWindow::CmdFindClass2()
{
  OpListWin->FindClass(FindBuffer);
  sWire->Popup(L"Find Class");
}

static void IncFindStore(sArray<sPoolString> &list,const sChar *str,void *ref)
{
  wOp *op;
  sInt len = sGetStringLen(str);
  sFORALL(Doc->Stores,op)
  {
    if(sCmpStringILen(str,op->Name,len)==0)
      list.AddTail(sPoolString(op->Name));
  }
}

void MainWindow::CmdFindStore()
{
  FindBuffer = L"";
  sFindDialog(L"Find Store",FindBuffer,sMessage(this,&MainWindow::CmdFindStore2),0,IncFindStore);
}

void MainWindow::CmdFindStore2()
{
  wOp *op;
  sFORALL(Doc->Stores,op)
  {
    if(sCmpString(op->Name,FindBuffer)==0)
    {
      GotoOp(op);
      break;
    }
  }
}

static void IncFindString(sArray<sPoolString> &list,const sChar *str,void *ref)
{
  wOp *op;
  sInt len = sGetStringLen(str);
  sFORALL(Doc->AllOps,op)
  {
    for(sInt i=0;i<op->EditStringCount;i++)
    {
      if(sCmpStringILen(str,op->EditString[i]->Get(),len)==0)
      {
        sPoolString str = op->EditString[i]->Get();
        if(!sFind(list,str))
          list.AddTail(str);
      }
    }
  }
}

void MainWindow::CmdFindString()
{
  FindBuffer = L"";
  sFindDialog(L"Find String",FindBuffer,sMessage(this,&MainWindow::CmdFindString2),0,IncFindString);
}

void MainWindow::CmdFindString2()
{
  OpListWin->FindString(FindBuffer);
  sWire->Popup(L"Find Class");
}

void MainWindow::CmdFindObsolete()
{
  OpListWin->FindObsolete();
  sWire->Popup(L"Find Class");
}

void MainWindow::CmdKillClips()
{
  ChangeDoc();
  Doc->Connect();
  wOp *op;

  sFORALL(Doc->AllOps,op)
  {
    if(sCmpStringI(op->Class->Name,L"clip")==0)
    {
      if(!op->Page->IsTree)
      {
        wStackOp *sop = (wStackOp *) op;
        if(((sU32 *)sop->EditData)[7]==0)
          sop->Hide = 1;
      }
    }
  }
}

void MainWindow::CmdSelUnconnected()
{
  wOp *op;

  Doc->Connect();
  sFORALL(Doc->AllOps,op)
    op->Select = !op->ConnectedToRoot;
  Update();
}

void MainWindow::CmdDelUnconnected()
{
  sOkCancelDialog(sMessage(this,&MainWindow::CmdDelUnconnected2),sMessage(),
    L"Delete all ops that are not connected to the \"root\" store op?\n\n"
    L"This is damn dangerous, please think carefully!\n");
}

void MainWindow::CmdDelUnconnected2()
{
  wPage *page;
  wOp *op;
  wStackOp *sop;
  wClass *nopclass = Doc->FindClass(L"Nop",L"AnyType");
  sVERIFY(nopclass);

  // first, replace all bypassed ops with nops

  sFORALL(Doc->Pages,page)
  {
    if(!page->IsTree)
    {
      sFORALL(page->Ops,op)
      {
        if(op->Bypass)
        {
          if(op->Inputs.GetCount()>0)
          {
            op->Init(nopclass);
            op->Bypass = 0;
            for(sInt i=1;i<op->Inputs.GetCount();i++)
            {
              sop = (wStackOp *) op->Inputs[i];
              sVERIFY(!sop->Page->IsTree);
              sop->Hide = 1;
            }
          }
        }
      }
    }
  }

  // reconnect and delete

  Doc->Connect();
  sFORALL(Doc->Pages,page)
    if(!page->IsTree)
      sRemFalse(page->Ops,&wOp::ConnectedToRoot);

  // finish

  Doc->Connect();
  ChangeDoc();
  Update();
}

void MainWindow::CmdGotoShortcut(sDInt num)
{
  sString<10> str;
  str.PrintF(L"_%d",num);
  wOp *op = Doc->FindStore(str);
  if(op)
    App->GotoOp(op);
}

void MainWindow::CmdGotoRoot()
{
  wOp *op = Doc->FindStore(L"root");
  if(op)
    App->GotoOp(op);
}

void MainWindow::CmdBrowser()
{
  StartStoreBrowser(sMessage(this,&MainWindow::CmdBrowser2),0,0);
}

void MainWindow::CmdBrowser2(sDInt opo)
{
  wOp *op = (wOp *)opo;
  GotoOp(op);
  EditOp(op,0);
}

void MainWindow::CmdAppChangeFromCustom()
{
  App->ChangeDoc();
  App->ParaWin[0]->SetOp(App->ParaWin[0]->Op);
  App->ParaWin[0]->UpdateUndoRedoButton();
}

void MainWindow::CmdScrollToArrayLine(sDInt n)
{
  // this is very hackish, but it works
  sWindow *w;
  sButtonControl *bc;
  sFORALL(ParaWin[0]->Childs,w)
  {
    if(sCmpString(w->GetClassName(),L"sButtonControl")==0)
    {
      bc = (sButtonControl *)w;
      if(sCmpString(bc->Label,L"Rem")==0 && bc->GetRadioValue()==n)
      {
        if(bc->Client.SizeX()>0)
          ParaWin[0]->ScrollTo(bc->Client,1);
      }
    }
  }

  UpdateWindows();
}

/****************************************************************************/

void MainWindow::MakePresetPath(const sStringDesc &path,wOp *op,const sChar *name)
{
  sString<sMAXPATH> buffer;

  sVERIFY(op);
  buffer.PrintF(L"%s/presets/%s_%s_%s.preset",WikiPath,op->Class->OutputType->Symbol,op->Class->Name,name);
  sCopyString(path,buffer);
}

void MainWindow::ExtractPresetName(const sStringDesc &name,const sChar *path)
{
  const sChar *s0,*s1;
  s0 = path;
  while(*s0 && *s0!='_') s0++; if(*s0) s0++;
  while(*s0 && *s0!='_') s0++; if(*s0) s0++;
  s1 = s0;
  while(*s1 && *s1!='.') s1++;
  sCopyString(name,s0);
  sInt len = s1-s0;
  if(len<name.Size)
    name.Buffer[len] = 0;
}

void MainWindow::LoadOpPreset(wOp *op,const sChar *name)
{
  sString<sMAXPATH> path;
  sVERIFY(op);
  MakePresetPath(path,op,name);
  sLoadObject(path,op);
}

void MainWindow::SaveOpPreset(wOp *op,const sChar *name)
{
  sString<sMAXPATH> path;
  sVERIFY(op);
  MakePresetPath(path,op,name);
  sSaveObject(path,op);
}

/****************************************************************************/
/****************************************************************************/

WinPageList::WinPageList() : sSingleTreeWindow<wPage>(&Doc->Pages,&wPage::TreeInfo)
{
//  AddBorder(new sFocusBorder);
//  AddBorder(new sListWindow2Header(this));
//  AddBorder(new sScrollBorder);
//  Flags |= sWF_SCROLLY;
  SelectMsg = sMessage(this,&WinPageList::CmdSetPage);
  OrderMsg = sMessage(App,&MainWindow::ChangeDoc);
  
  AddField(L"Name",sLWF_EDIT,100,sMEMBERPTR(wPage,Name));
  AddFieldChoice(L"Mode",0,35,sMEMBERPTR(wPage,IsTree),L"Stack|Tree");
  AddField(L"Include",sLWF_EDIT,100,1,0);
//  AddFieldChoice(L"Access",sLWF_EDIT,50,sMEMBERPTR(wPage,ManualWriteProtect),L"Read Write|Read Only");
  AddField(L"Access",sLWF_EDIT,50,2,0);
  AddHeader();
  SetIncludePage = 0;
}

WinPageList::~WinPageList()
{
}

void WinPageList::Tag()
{
  sSingleTreeWindow<wPage>::Tag();
  SetIncludePage->Need();
}

sBool WinPageList::OnPaintField(const sRect &client,sListWindow2Field *field,sObject *obj,sInt line,sInt select)
{
  const sChar *str;
  wPage *page = (wPage *) obj;
  switch(field->Id)
  {
  case 1:
    PaintField(client,select,page->Include ? page->Include->Filename : L"");
    return 1;

  case 2:
    str = L"Read / Write";
    if(page->ManualWriteProtect)
      str = L"Manual Write Protect";
    if(page->Include && page->Include->Protected)
      str = L"File Write Protect";
    PaintField(client,select,str);
    return 1;

  default:
    return 0;
  }
}

sBool WinPageList::OnEditField(const sRect &client,sListWindow2Field *field,sObject *obj,sInt line)
{
  wPage *page = (wPage *) obj;
  switch(field->Id)
  {
  case 1:
    SetIncludePage = page;
    wDocInclude *inc;

    if(Doc->Includes.GetCount()>0 && SetIncludePage)
    {
      sMenuFrame *mf = new sMenuFrame(this);
      sFORALL(Doc->Includes,inc)
        mf->AddItem(inc->Filename,sMessage(this,&WinPageList::CmdSetInclude,_i),0);
      sGui->AddWindow(mf,client.x0,client.y1);
    }
    return 1;

  case 2:
    if(!(page->Include && page->Include->Protected))
    {
      page->ManualWriteProtect = !page->ManualWriteProtect;
      Update();
    }
    return 1;

  default:
    return 0;
  }
}

void WinPageList::CmdSetInclude(sDInt i)
{
  if(i>=0 && i<Doc->Includes.GetCount() && SetIncludePage)
  {
    SetIncludePage->Include = Doc->Includes[i];
    Update();
    SetIncludePage = 0;
  }
}

void WinPageList::InitWire(const sChar *name)
{
  sSingleListWindow<wPage>::InitWire(name);

  sWire->AddKey(name,L"AddStack",sMessage(this,&WinPageList::CmdAdd,0));
  sWire->AddKey(name,L"AddTree",sMessage(this,&WinPageList::CmdAdd,1));
  sWire->AddKey(name,L"Rename",sMessage(this,&WinPageList::CmdRename));
  sWire->AddKey(name,L"DeleteSave",sMessage(this,&WinPageList::CmdDeleteSave));
  sWire->AddKey(name,L"Cut",sMessage(this,&WinPageList::CmdCut));
  sWire->AddKey(name,L"Copy",sMessage(this,&WinPageList::CmdCopy));
  sWire->AddKey(name,L"Paste",sMessage(this,&WinPageList::CmdPaste));
}

sBool WinPageList::OnCommand(sInt cmd)
{
  if(cmd==sCMD_ENTERFOCUS)
  {
    CmdSetPage();
    return 1;
  }
  return 0;
}

void WinPageList::CmdAdd(sDInt t)
{
  wPage *page = new wPage;
  page->IsTree = t;
  page->DefaultName();
  AddNew(page);
  App->ChangeDoc();
}

void WinPageList::CmdRename()
{
  ForceEditField(GetLastSelect(),0);
}

void WinPageList::CmdSetPage()
{
  wPage *page = GetSelected();
  if(page)
  {
    Doc->DocOptions.PageName = page->Name;
    if(page->IsTree)
    {
      App->SwitchPage(1);
      App->TreeViewWin->SetPage(page);
    }
    else
    {
      App->SwitchPage(0);
      App->StackWin->SetPage(page);
    }
  }
}

void WinPageList::CmdDeleteSave()
{
  sYesNoDialog(sMessage(this,&WinPageList::CmdDelete2,1),sMessage(this,&WinPageList::CmdDelete2,0),L"really delete page?");
}

void WinPageList::CmdDelete2(sDInt i)
{
  sGui->SetFocus(this);
  if(i)
    CmdDelete();
}

void WinPageList::CmdCut()
{
  wPage *page = GetSelected();
  if(page)
  {
    sSetClipboardObject(page,sSerId::wPage);
    Doc->Pages.RemOrder(page);
  }
}

void WinPageList::CmdCopy()
{
  wPage *page = GetSelected();
  if(page)
  {
    sSetClipboardObject(page,sSerId::wPage);
  }
}

void WinPageList::CmdPaste()
{
  wPage *page = 0;
  if(sGetClipboardObject(page,sSerId::wPage))
  {
    Doc->Pages.AddTail(page);
    Update();
    Doc->Connect();
    App->ChangeDoc();
  }
}

/****************************************************************************/

WinClipOpsList::WinClipOpsList(sArray<wOp *> *ar) : sSingleListWindow<wOp>(ar)
{
  this->AddField(L"Name",0,200,1,0);
}

void WinClipOpsList::InitWire(const sChar *name)
{
  sSingleListWindow<wOp>::InitWire(name);
}

sBool WinClipOpsList::OnPaintField(const sRect &client,sListWindow2Field *field,sObject *obj,sInt line,sInt select)
{
  if(field->Id==1)
  {
    wOp *op = (wOp *) obj;
    PaintField(client,select,op->Name.IsEmpty()?L"???":op->Name);
    return 1;
  }
  return 0;
}

/****************************************************************************/

WinCurveOpsList::WinCurveOpsList(sArray<wOp *> *ar) : sSingleListWindow<wOp>(ar)
{
  this->AddField(L"Name",0,200,1,0);
}

void WinCurveOpsList::InitWire(const sChar *name)
{
  sSingleListWindow<wOp>::InitWire(name);
}

sBool WinCurveOpsList::OnPaintField(const sRect &client,sListWindow2Field *field,sObject *obj,sInt line,sInt select)
{
  if(field->Id==1)
  {
    wOp *op = (wOp *) obj;
    PaintField(client,select,op->Name.IsEmpty()?op->Class->Label:sPoolString(op->Name));
    return 1;
  }
  return 0;
}


/****************************************************************************/
/***                                                                      ***/
/***   custum editor                                                      ***/
/***                                                                      ***/
/****************************************************************************/

wCustomEditor::wCustomEditor()
{
}

wCustomEditor::~wCustomEditor()
{
}

void wCustomEditor::OnCalcSize(sInt &xs,sInt &ys)
{
}

void wCustomEditor::OnLayout(const sRect &Client)
{
}

void wCustomEditor::OnPaint2D(const sRect &Client)
{
}

sBool wCustomEditor::OnKey(sU32 key)
{
  return 0;
}

void wCustomEditor::OnDrag(const sWindowDrag &dd,const sRect &Client)
{
}

void wCustomEditor::OnChangeOp()
{
}

void wCustomEditor::OnTime(sInt time)
{
}

void wCustomEditor::ChangeOp(wOp *op)
{
  Doc->ChangeDefaults(op);
  Doc->Change(op);
  Doc->AppChangeFromCustomMsg.Post();
} 

void wCustomEditor::Update()
{
  App->CustomWin->Update();
}

void wCustomEditor::ScrollTo(const sRect &r,sBool safe)
{
  App->CustomWin->ScrollTo(r,safe);
}

/****************************************************************************/
/****************************************************************************/

WinTreeView::WinTreeView() : sMultiTreeWindow<wTreeOp>(0,sMEMBERPTR(wTreeOp,Select),sMEMBERPTR(wTreeOp,TreeInfo))
{
  SelectMsg = sMessage(this,&WinTreeView::CmdEdit,0);
  ChangeMsg = sMessage(this,&WinTreeView::CmdChange);
  OrderMsg = sMessage(this,&WinTreeView::CmdChange);
  
  AddField(L"Link",0,150,2,0);
  AddField(L"Status",0,20,3,0);
  AddField(L"Class",0,100,1,0);
  AddField(L"Name",sLWF_EDIT,200,sMEMBERPTR(wTreeOp,Name));
  AddField(L"Description",0,250,4,0);
  AddHeader();

  HasBlink = sFALSE;
  BlinkToggle = sFALSE;

  Tree = 0;
  Page = 0;
}

WinTreeView::~WinTreeView()
{
}

void WinTreeView::SetPage(wPage *page)
{
  Page = page;
  if(page && page->IsTree)
    Tree = &page->Tree;
  else
    Tree = 0;
  SetArray(Tree);
  UpdateTreeInfo();
  Layout();
}

void WinTreeView::GotoOp(wOp *op,sBool blink)
{
  // make sure all parent nodes are expanded so this node is visible.
  for(wTreeOp *cur = ((wTreeOp*) op)->TreeInfo.Parent; cur; cur = cur->TreeInfo.Parent)
    cur->TreeInfo.Flags &= ~sLWTI_CLOSED;

  sInt index = sFindIndex(*Array,op);
  if(index>=0)
  {
    ClearSelectStatus();
    SetSelectStatus(index,1);
  }

  UpdateTreeInfo();
  Layout();
  OnCalcSize();
  ScrollToItem(op);
  if(blink)  
    op->BlinkTimer = sGetTime()+2000;
  Update();
}

void WinTreeView::Tag()
{
  Page->Need();
  sMultiTreeWindow<wTreeOp>::Tag();
  if(Tree)
    sNeed(*Tree);
}

void WinTreeView::InitWire(const sChar *name)
{
  sMultiTreeWindow<wTreeOp>::InitWire(name);

  sWire->AddKey(name,L"Add",sMessage(this,&WinTreeView::CmdAdd,0));
  sWire->AddKey(name,L"AddMore",sMessage(this,&WinTreeView::CmdAdd,1));
  sWire->AddKey(name,L"Conversions",sMessage(this,&WinTreeView::CmdAddConversion));
  sWire->AddKey(name,L"Show",sMessage(this,&WinTreeView::CmdShow,0));
  sWire->AddKey(name,L"Show2",sMessage(this,&WinTreeView::CmdShow,1));
  sWire->AddKey(name,L"ShowRoot",sMessage(this,&WinTreeView::CmdShowRoot,0));

  sWire->AddKey(name,L"Cut",sMessage(this,&WinTreeView::CmdCut));
  sWire->AddKey(name,L"Copy",sMessage(this,&WinTreeView::CmdCopy));
  sWire->AddKey(name,L"Paste",sMessage(this,&WinTreeView::CmdPaste));

  sWire->AddKey(name,L"Add1"      ,sMessage(this,&WinTreeView::CmdAddType,'1'));
  sWire->AddKey(name,L"Add2"      ,sMessage(this,&WinTreeView::CmdAddType,'2'));
  sWire->AddKey(name,L"Add3"      ,sMessage(this,&WinTreeView::CmdAddType,'3'));
  sWire->AddKey(name,L"Add4"      ,sMessage(this,&WinTreeView::CmdAddType,'4'));
  sWire->AddKey(name,L"Add5"      ,sMessage(this,&WinTreeView::CmdAddType,'5'));
  sWire->AddKey(name,L"Add6"      ,sMessage(this,&WinTreeView::CmdAddType,'6'));
  sWire->AddKey(name,L"Add7"      ,sMessage(this,&WinTreeView::CmdAddType,'7'));
  sWire->AddKey(name,L"Add8"      ,sMessage(this,&WinTreeView::CmdAddType,'8'));
  sWire->AddKey(name,L"Add9"      ,sMessage(this,&WinTreeView::CmdAddType,'9'));
}

void WinTreeView::CmdAdd(sDInt sec)
{
  if(Page->IsProtected()) return;
  App->AddPopup(sMessage(this,&WinTreeView::CmdAdd2),sec);
}

void WinTreeView::CmdAddType(sDInt type)
{
  if(Page->IsProtected()) return;
  App->AddPopup(sMessage(this,&WinTreeView::CmdAdd2),0,type);
}

void WinTreeView::CmdAddConversion()
{
  if(Page->IsProtected()) return;
  App->AddConversionPopup(sMessage(this,&WinTreeView::CmdAdd2));
}

void WinTreeView::CmdAdd2(sDInt n)
{
  if(Page->IsProtected()) return;
  if(n>=0 && n<Doc->Classes.GetCount())
  {
    wTreeOp *op = new wTreeOp;
    op->Init(Doc->Classes[n]);
    App->LoadOpPreset(op,L"default");
    AddNew(op);
    Doc->Connect();
    App->ChangeDoc();
  }
}

void WinTreeView::CmdChange()
{
  Doc->Connect();
  App->ChangeDoc();
}

void WinTreeView::CmdShow(sDInt win)
{
  sInt n = GetLastSelect();
  wTreeOp *op;
  if(n>=0 && Tree)
  {
    op = (*Tree)[n];
    App->ShowOp(op,win);
  }
}

void WinTreeView::CmdShowRoot(sDInt win)
{
  wOp *op = Doc->FindStore(L"root");
  if(op)
    App->ShowOp(op,win);
}

void WinTreeView::CmdEdit(sDInt win)
{
  if(Page->IsProtected()) return;
  sInt n = GetLastSelect();
  wTreeOp *op;
  if(n>=0 && Tree)
  {
    op = (*Tree)[n];
    App->EditOp(op,win);
  }
}

void WinTreeView::CmdCut()
{
  CmdCopy();
  if(Page->IsProtected()) return;
  CmdDelete();
}

void WinTreeView::CmdCopy()
{
  sArray<wTreeOp *> ops;
  wTreeOp *op;

  if(Tree)
  {
    sFORALL(*Tree,op)
      if(op->Select)
        ops.AddTail(op);

    if(ops.GetCount()>0)
      sSetClipboardArray(ops,sSerId::wTreeOp);
  }
}

void WinTreeView::CmdPaste()
{
  if(Page->IsProtected()) return;
  if(Tree)
  {
    sArray<wTreeOp *> ops;
    sArray<wStackOp *> sops;

    if(sGetClipboardArray(ops,sSerId::wTreeOp))
    {
      wTreeOp *op;
      sInt line = this->GetLastSelect();

      ClearSelectStatus();

      sFORALL(ops,op)
      {
        Tree->AddAfter(op,line);
        line++;
        SetSelectStatus(line,1);
      }

      Update();
      Doc->Connect();
      App->ChangeDoc();
    }
    if(sGetClipboardArray(sops,sSerId::wStackOp))
    {
      wTreeOp *op;
      wStackOp *sop;
      sInt line = this->GetLastSelect();

      sFORALL(sops,sop)
      {
        op = new wTreeOp;
        op->CopyFrom(sop);
        Tree->AddAfter(op,line++);
        op->Select = 1;
        SetSelectStatus(line,1);
      }

      Update();
      Doc->Connect();
      App->ChangeDoc();
    }
  }
}

sInt WinTreeView::OnBackColor(sInt line,sInt select,sObject *obj)
{
  wOp *op;
  op = (wOp *)obj;

  sInt backcol = sGC_BACK;

  if(!op->NoError())
    backcol = sGC_RED;
  if(select)
    backcol = sGC_SELECT;

  if(sGetTime() < op->BlinkTimer)
  {
    HasBlink = sTRUE;
    if(BlinkToggle)
      backcol = sGC_RED;
  }

  return backcol;
}

void WinTreeView::OnPaint2D()
{
  HasBlink = sFALSE;
  sMultiTreeWindow<wTreeOp>::OnPaint2D();
  Doc->CurrentPage = Page;
}

sBool WinTreeView::OnPaintField(const sRect &client,sListWindow2Field *field,sObject *obj,sInt line,sInt select)
{
  sString<256> str;
  wOp *op;
  op = (wOp *)obj;

  switch(field->Id)
  {
  case 1:
    sSetColor2D(0,op->Class->OutputType->Color);
    sGui->PropFont->SetColor(sGC_BLACK,(BackColor == sGC_RED) ? sGC_RED : 0);
    sGui->PropFont->Print(sF2P_OPAQUE|sF2P_SPACE|sF2P_LEFT,client,op->Class->Label);
    return 1;
  case 2:
    if(op->Links.GetCount()>=1 && !op->Links[0].LinkName.IsEmpty() && (op->Class->Flags & wCF_LOAD))
    {
      str.PrintF(L"load : %s",op->Links[0].LinkName);
      sGui->PropFont->SetColor(sGC_TEXT,BackColor);
      sGui->PropFont->Print(sF2P_OPAQUE|sF2P_SPACE|sF2P_LEFT,client,str);
    }
    else if(!op->Name.IsEmpty())
    {
      if(op->Name[0]!=';')
      {
        str.PrintF(L"store : %s",op->Name);
        sGui->PropFont->SetColor(sGC_TEXT,BackColor);
        sGui->PropFont->Print(sF2P_OPAQUE|sF2P_SPACE|sF2P_LEFT,client,str);
      }
      else
      {
        const sChar *s = op->Name+1;
        while(sIsSpace(*s)) s++;
        str.PrintF(L"%s ; %s",op->Class->Label,s);
        sGui->PropFont->SetColor(sGC_LOW2,BackColor);
        sGui->PropFont->Print(sF2P_OPAQUE|sF2P_SPACE|sF2P_LEFT,client,str);
      }
    }
    else
    {
      sGui->PropFont->SetColor(sGC_LOW2,BackColor);
      sGui->PropFont->Print(sF2P_OPAQUE|sF2P_SPACE|sF2P_LEFT,client,op->Class->Label);
    }
    return 1;
  case 3:
    {
      sRect b0,b1,b2;
      sInt x,y;

      x = client.CenterX();
      y = client.CenterY();

      b0.y0 = b1.y0 = b2.y0 = y-3;
      b0.y1 = b1.y1 = b2.y1 = y+3;
      b0.x0 = x -6;
      b0.x1 = x -3;
      b1.x0 = x -1;
      b1.x1 = x +2;
      b2.x0 = x +4;
      b2.x1 = x +7;

      sRect2D(client,BackColor);
      if(App->IsShown(op))
        sRect2D(b0,sGC_GREEN);
      if(App->IsEdited(op))
        sRect2D(b1,sGC_RED);
      if(op->Cache)
        sRect2D(b2,sGC_BLUE);
    }
    return 1;
  case 4:
    {
      const sChar *desc = op->Class->GetDescription ? op->Class->GetDescription(op) : 0;
      sGui->PropFont->SetColor(sGC_TEXT,BackColor);
      sGui->PropFont->Print(sF2P_OPAQUE|sF2P_SPACE|sF2P_LEFT,client,desc ? desc : L"");
    }
    return 1;

  default:
    return 0;
  }
}

void WinTreeView::CmdBlinkTimer()
{
  BlinkToggle = !BlinkToggle;
  if(HasBlink)
    Update();
}

/****************************************************************************/
/****************************************************************************/

WinPara::WinPara()
{
  Op = 0;
  CurrentLinkId = 0;
  CurrentDisplayedError = 0;
  Flags |= sWF_BEFOREPAINT;
  UndoRedoButton = 0;
  ScriptButton = 0;
  ScriptShowControl = 0;
  PresetKeepArrayFlag = 0;

  Status = new sStatusBorder();
  Status->AddTab(1);
  AddBorder(Status);
}

WinPara::~WinPara()
{
}

void WinPara::Tag()
{
  sWindow::Tag();
  Op->Need();
}

void WinPara::OnBeforePaint()
{
  if(Op)
  {
    const sChar *str = Op->CalcErrorString;
    if(Op->ConnectErrorString)
      str = Op->ConnectErrorString;
    if(str!=CurrentDisplayedError)
    {
      CurrentDisplayedError = str;
      if(CurrentDisplayedError)
        Status->PrintF(0,L"Error: %s",CurrentDisplayedError);
      else
        Status->PrintF(0,L"Ok");
    }
  }
}

void WinPara::SetOp(wOp *op)
{
  wOpInputInfo *info;
  sControl *con;

  if(Op!=op)
  {
    if(op)
      Undo.CopyFrom(op);
    Redo.Clear();
  }

  Op = op;
  ScriptShowControl = 0;
  
  Columns = 14;
  if(Op && Op->Class->GridColumns)
    Columns = Op->Class->GridColumns;
  wGridFrameHelper gh(this);
  gh.ConnectLayoutMsg = sMessage(this,&WinPara::CmdConnectLayout);
  gh.ConnectMsg = sMessage(this,&WinPara::CmdConnect);
  gh.LayoutMsg = sMessage(this,&WinPara::CmdLayout);
  gh.ChangeMsg = sMessage(this,&WinPara::CmdChanged);
  gh.DoneMsg = sMessage();
  gh.LinkBrowserMsg = sMessage(this,&WinPara::CmdLinkBrowser);
  gh.LinkPopupMsg = sMessage(this,&WinPara::CmdLinkPopup);
  gh.LinkGotoMsg = sMessage(this,&WinPara::CmdLinkGoto);
  gh.LinkAnimMsg = sMessage(this,&WinPara::CmdLinkAnim);
  gh.AddArrayMsg = sMessage(this,&WinPara::AddArray);
  gh.RemArrayMsg = sMessage(this,&WinPara::RemArray);
  gh.RemArrayGroupMsg = sMessage(this,&WinPara::RemArrayGroup);
  gh.FileLoadDialogMsg = sMessage(this,&WinPara::FileLoadDialog);
  gh.FileReloadMsg = sMessage(this,&WinPara::FileReload);
  gh.FileSaveDialogMsg = sMessage(this,&WinPara::FileSaveDialog);
  gh.ActionMsg = sMessage(this,&WinPara::CmdAction);
  gh.ArrayClearAllMsg = sMessage(this,&WinPara::CmdArrayClearAll);

  ClearNotify();
  UndoRedoButton = 0;
  ScriptButton = 0;
  ScriptShowControl = 0;
  if(Op && !Op->Page->IsProtected())
  {
    AddNotify(Op);
    AddNotify(Op->EditData,sizeof(Op->Class->ParaWords)*sizeof(sU32));
    gh.Label(L"Name");
    con = gh.String(Op->Name);
    con->DoneMsg = gh.ConnectMsg;
    con->ChangeMsg = sMessage();

//    gh.BoxToggle(L"Wiki",&App->WikiToggleFlag,sMessage(this,&WinPara::CmdWiki));
    UndoRedoButton = gh.Box(L"Undo",sMessage(this,&WinPara::CmdUndoRedo));
    UndoRedoButton->Style = sBCS_STATIC;
    ScriptButton = gh.BoxToggle(L"Script",&op->ScriptShow,sMessage(this,&WinPara::CmdLayout));
    ScriptButton->BackColor = (Op->ScriptSourceValid) ? 0xff40c040 : 0;

    if(Op->Class->CustomEd)
      gh.Box(L"Edit",sMessage(this,&WinPara::CmdCustomEd));

    sFORALL(Op->Links,info)
    {
      if(info->DefaultUsed && info->Default)
      {
        (*info->Default->Class->MakeGui)(gh,info->Default);
      }
    }
    if(Op->Class->MakeGui)
    {
      (*Op->Class->MakeGui)(gh,Op);
    }
    else
    {
      gh.Group(Op->Class->Label);
    }
    if(op->ScriptParas.GetCount()>0)
    {
      const sChar *group = L"Scripting Parameters";
      wScriptParaInfo *sp;
      sFORALL(op->ScriptParas,sp)
      {
        if(group)
        {
          gh.Group(group);
          group = 0;
        }
        switch(sp->Type)
        {
        case 0:
          group = sp->Name;
          break;
        case ScriptTypeInt:
          if(!(sp->GuiFlags & 1))
            gh.Label(sp->Name);
          if(sp->GuiFlags & 2)
          {
            gh.Flags(sp->IntVal,sp->GuiChoices);
          }
          else
          {
            for(sInt i=0;i<sp->Count;i++)
            {
              const sChar *fmt = 0;
              if(sp->GuiExtras>0)
              {
                const static sChar *formats[9] = { 0,L"%01x",L"%02x",L"%03x",L"%04x",L"%05x",L"%06x",L"%07x",L"%08x" };
                fmt = formats[sMin(sp->GuiExtras,8)];
              }
              sIntControl *con = gh.Int(sp->IntVal+i,sp->Min,sp->Max,sp->Step,0,fmt);
              con->Default = sp->Default;
            }
          }
          break;

        case ScriptTypeFloat:
          if(!(sp->GuiFlags & 1))
            gh.Label(sp->Name);
          for(sInt i=0;i<sp->Count;i++)
          {
            sFloatControl *con = gh.Float(sp->FloatVal+i,sp->Min,sp->Max,sp->Step);
            con->Default = sp->Default;
          }
          break;

        case ScriptTypeString:
          if(!(sp->GuiFlags & 1))
            gh.Label(sp->Name);
          gh.String(&sp->StringVal);
          if(sp->GuiExtras == 1)
            gh.Box(L"...",sMessage(this,&WinPara::FileOutDialogScript,_i));
          if(sp->GuiExtras == 2)
            gh.Box(L"...",sMessage(this,&WinPara::FileInDialogScript,_i));
          break;

        case ScriptTypeColor:
          if(!(sp->GuiFlags & 1))
            gh.Label(sp->Name);
          gh.ColorPick(&sp->ColorVal,sp->GuiExtras ? L"rgb" : L"rgba",0);
          break;
        }
      }
    }
    if(op->ScriptShow)
    {
      gh.Group(L"Scripting");
      gh.Left = 0;
      ScriptShowControl = gh.Text(&op->ScriptSource,12,gh.Grid->Columns);
      ScriptShowControl->ChangeMsg = sMessage(this,&WinPara::CmdChangeScript);  
      ScriptShowControl->SetFont(sGui->FixedFont);
      ScriptShowControl->EditFlags = sTEF_LINENUMBER;
    }
    gh.Label(L" ");   // one more line of space. this helps automatic scrolling when adding array elements.
  }
  App->UpdateImportantOp();
}

void WinPara::CmdConnectLayout()
{
  Doc->Connect();
  Doc->ChangeDefaults(Op);
  Doc->Change(Op);
  SetOp(Op);
  App->ChangeDoc();
  UpdateUndoRedoButton();
}

void WinPara::CmdConnect()
{
  Doc->Connect();
  Doc->ChangeDefaults(Op);
  Doc->Change(Op);
  App->ChangeDoc();
  UpdateUndoRedoButton();
}

void WinPara::CmdLayout()
{
  CmdChanged();
  SetOp(Op);
  UpdateUndoRedoButton();
}

void WinPara::CmdChanged()
{
//  App->UpdateViews();

  Doc->ChangeDefaults(Op);
  Doc->Change(Op);
  App->ChangeDoc();
  App->CustomWin->ChangeOp();
  UpdateUndoRedoButton();
  if(Op->Class->Flags & wCF_COMMENT)
    App->StackWin->Update();
}

static sU32 getguihash(wOp *op)
{
  sU32 hash = 0;
  wScriptParaInfo *sp;
  sFORALL(op->ScriptParas,sp)
  {
    hash++;
    hash ^= sChecksumCRC32((sU8 *)sp,sizeof(*sp));
    hash ^= sChecksumCRC32((sU8 *)(const sChar *)sp->Name,sGetStringLen(sp->Name)*sizeof(sChar));
  }
  return hash;
}

void WinPara::CmdChangeScript()
{
  sInt cursor = 0;
  sU32 hash1,hash2;
  if(ScriptShowControl)
    cursor = ScriptShowControl->GetCursorPos();
  hash1 = getguihash(Op);
  Op->UpdateScript();
  hash2 = getguihash(Op);
  if(hash1!=hash2)
  {
    CmdLayout();
    if(ScriptShowControl)
    {
      sGui->SetFocus(ScriptShowControl);
      ScriptShowControl->SetCursor(cursor);
    }
  }
  CmdChanged();
  if(ScriptButton)
    ScriptButton->BackColor = (Op->ScriptSourceValid) ? 0xff40c040 : 0;
  App->StackWin->Update();
}


void WinPara::UpdateUndoRedoButton()
{
  if(UndoRedoButton)
  {
    if(Undo.IsSame(Op))
    {
      if(Redo.IsValid(Op))
      {
        UndoRedoButton->Label = L"Redo";
        UndoRedoButton->Style = 0;
        UndoIsRedo = 1;
      }
      else
      {
        UndoRedoButton->Label = L"Undo";
        UndoRedoButton->Style = sBCS_STATIC;
        UndoIsRedo = 0;
      }
    }
    else
    {
      UndoRedoButton->Label = L"Undo";
      UndoRedoButton->Style = 0;
      UndoIsRedo = 0;
    }

    UndoRedoButton->Update();
  }
}

void WinPara::CmdDocChange()
{
  App->ChangeDoc();
  App->CustomWin->ChangeOp();
  UpdateUndoRedoButton();
}

void WinPara::CmdLinkBrowser(sDInt n)
{
  CurrentLinkId = n;
  const sChar *oldstore = 0;

  if(Op && CurrentLinkId>=0 && CurrentLinkId<Op->Links.GetCount())
    oldstore = Op->Links[CurrentLinkId].LinkName;

  wType *type = 0;
  if(n>=0 && n<Op->Class->Inputs.GetCount())
    type = Op->Class->Inputs[n].Type;

  App->StartStoreBrowser(sMessage(this,&WinPara::CmdLinkBrowser2),oldstore,type);
}

void WinPara::CmdLinkBrowser2()
{
  if(CurrentLinkId>=0 && CurrentLinkId<Op->Links.GetCount())
  {
    if(Op->Links[CurrentLinkId].LinkName != App->StoreBrowserWin->StoreName)
    {
      Op->Links[CurrentLinkId].LinkName = App->StoreBrowserWin->StoreName;
      CmdConnectLayout();
      Update();
    }
  }
}

void WinPara::CmdLinkPopup(sDInt offset)
{
  CurrentLinkId = offset;
  wOp *op;

  if(App->StackWin->GetPage())
  {
    sMenuFrame *mf = new sMenuFrame(this);
    wType *type = 0;
    if(offset>=0 && offset<Op->Class->Inputs.GetCount())
      type = Op->Class->Inputs[offset].Type;
    mf->AddItem(L"< none >",sMessage(this,&WinPara::CmdLinkPopup2,0),0);
    sFORALL(App->StackWin->GetPage()->Ops,op)
    {
      wOp *follow = op;
      while(follow->Class->OutputType==AnyTypeType && follow->Inputs.GetCount()>0 && follow->Inputs[0])
        follow = follow->Inputs[0];
      if(!op->Name.IsEmpty() && (type==0 || follow->Class->OutputType->IsTypeOrConversion(type)))
        mf->AddItem(op->Name,sMessage(this,&WinPara::CmdLinkPopup2,sDInt(op)),0);
    }
    sGui->AddPulldownWindow(mf);
  }
}

void WinPara::CmdLinkPopup2(sDInt intop)
{
  wOp *op = (wOp *)intop;

  if(CurrentLinkId>=0 && CurrentLinkId<Op->Links.GetCount() && (op==0 || (sFindPtr(App->StackWin->GetPage()->Ops,op) && !op->Name.IsEmpty())))
  {
    if(op==0)
    {
      if(!Op->Links[CurrentLinkId].LinkName.IsEmpty())
      {
        Op->Links[CurrentLinkId].LinkName = L"";
        CmdConnectLayout();
        Update();
      }
    }
    else
    {
      if(Op->Links[CurrentLinkId].LinkName != op->Name)
      {
        Op->Links[CurrentLinkId].LinkName = op->Name;
        CmdConnectLayout();
        Update();
      }
    }
  }
}

void WinPara::CmdLinkGoto(sDInt offset)
{
  if(offset>=0 && offset<Op->Links.GetCount())
    if(wOp *target = Op->Links[offset].Link)
      App->GotoOp(target);
}

void WinPara::CmdLinkAnim(sDInt offset)
{
  if(offset>=0 && offset<Op->Links.GetCount())
  {
    wOp *op = Op->Links[offset].Link;
    if(op)
    {
      App->AnimOp(op);
      wOp *func;
      sFORALL(op->Outputs,func)
      {
        if(func->Class->Flags & wCF_CURVE)
        {
          App->ShowOp(func,3);
          App->EditOp(func,1);
        }
      }
    }
    else
    {
      wClass *cl = Doc->FindClass(L"Variable",L"Wz4Variable");
      if(cl && !Op->Name.IsEmpty())
      {
        wDocName name;

        name.PrintF(L"%s_%s",Op->Name,Op->Class->Inputs[offset].Name);
        if(Doc->FindStore(name)==0)
        {
          Op->Links[offset].LinkName = name;
          wPage *page = sFind(Doc->Pages,&wPage::Name,sPoolString(L"AnimationPage"));
          if(page && !page->IsTree)
          {
            page->Name = L"???";
            page = 0;
          }
          if(page==0)
          {
            page = new wPage;
            page->Name = L"AnimationPage";
            page->IsTree = 1;
            Doc->Pages.AddTail(page);
          }
          sArray<wTreeOp *> &tree = page->Tree;
          sInt groupindex = sFindIndex(tree,&wTreeOp::Name,L"AnimationVariable");
          wTreeOp *group = groupindex>=0 ? tree[groupindex] : 0;
          if(group==0)
          {
            wClass *clg = Doc->FindClass(L"Group",L"AnyType");
            sVERIFY(clg);
            group = new wTreeOp;
            group->Init(clg);
            group->Name = L"AnimationVariable";
            tree.AddTail(group);
            groupindex = sFindIndex(tree,&wTreeOp::Name,L"AnimationVariable");
          }
          wTreeOp *nop = new wTreeOp;
          nop->Init(cl);
          nop->Name = name;
          nop->TreeInfo.Level = group->TreeInfo.Level+1;
          tree.AddAfter(nop,groupindex);
          Doc->Connect();
          App->ChangeDoc();
          App->Update();
          SetOp(Op);
        }
      }
    }
  }
  sWire->SwitchScreen(3);
}

void WinPara::AddArray(sDInt pos)
{
  if(Op)
  {
    App->DeselectHandles();
    Op->AddArray(pos);
    CmdLayout();
    App->CustomWin->ChangeOp();
    ScrollTo(0,100000);
  }
}

void WinPara::RemArray(sDInt pos)
{
  if(Op)
  {
    App->DeselectHandles();
    Op->RemArray(pos);
    CmdLayout();
    App->CustomWin->ChangeOp();
  }
}

void WinPara::CmdArrayClearAll()
{
  if(Op)
  {
    App->DeselectHandles();
    sDeleteAll(Op->ArrayData);
    CmdLayout();
    App->CustomWin->ChangeOp();
  }
}

void WinPara::RemArrayGroup(sDInt pos)
{
  if(Op)
  {
    Op->RemArray(pos);
    while(Op->ArrayData.GetCount()>pos && *Op->GetArray<sU32>(pos)==0)
      Op->RemArray(pos);

    CmdLayout();
    App->CustomWin->ChangeOp();
  }
}

void WinPara::FileLoadDialog(sDInt offset)
{
  if(Op && Op->Class->ParaStrings>offset)
  {
    FileDialogBuffer = Op->EditString[offset]->Get();
    if(sSystemOpenFileDialog(L"Select File",Op->Class->FileInFilter.IsEmpty()?L"":Op->Class->FileInFilter,sSOF_LOAD,FileDialogBuffer))
    {
      Op->EditString[offset]->Clear();

      sInt len = sGetStringLen(Doc->DocOptions.ProjectPath);
      if(sCmpStringPLen(Doc->DocOptions.ProjectPath,FileDialogBuffer,len)==0)
      {
        if(FileDialogBuffer[len]=='/' || FileDialogBuffer[len]=='\\') len++;
        Op->EditString[offset]->Print(FileDialogBuffer+len);
      }
      else
      {
        Op->EditString[offset]->Print(FileDialogBuffer);
      }
      CmdLayout();
    }
  }
}

void WinPara::FileReload()
{
  Doc->Change(Op);
}

void WinPara::FileSaveDialog(sDInt offset)
{
  if(Op && Op->Class->ParaStrings>offset)
  {
    FileDialogBuffer = Op->EditString[offset]->Get();
    if(sSystemOpenFileDialog(L"Select File",L"*",sSOF_SAVE,FileDialogBuffer))
    {
      Op->EditString[offset]->Clear();

      sInt len = sGetStringLen(Doc->DocOptions.ProjectPath);
      if(sCmpStringPLen(Doc->DocOptions.ProjectPath,FileDialogBuffer,len)==0)
      {
        if(FileDialogBuffer[len]=='/' || FileDialogBuffer[len]=='\\') len++;
        Op->EditString[offset]->Print(FileDialogBuffer+len);
      }
      else
      {
        Op->EditString[offset]->Print(FileDialogBuffer);
      }
      CmdLayout();
    }
  }
}
void WinPara::FileDialogScript(sDInt offset,sInt mode)
{
  if(Op && Op->ScriptParas.GetCount()>offset)
  {
    FileDialogBuffer = Op->ScriptParas[offset].StringVal;
    if(sSystemOpenFileDialog(L"Select File",L"*",mode,FileDialogBuffer))
    {
      sInt len = sGetStringLen(Doc->DocOptions.ProjectPath);
      if(sCmpStringPLen(Doc->DocOptions.ProjectPath,FileDialogBuffer,len)==0)
      {
        if(FileDialogBuffer[len]=='/' || FileDialogBuffer[len]=='\\') len++;
        Op->ScriptParas[offset].StringVal = FileDialogBuffer+len;
      }
      else
      {
        Op->ScriptParas[offset].StringVal = FileDialogBuffer;
      }

      CmdLayout();
    }
  }
}

void WinPara::FileInDialogScript(sDInt offset)
{
  FileDialogScript(offset,sSOF_LOAD);
}

void WinPara::FileOutDialogScript(sDInt offset)
{
  FileDialogScript(offset,sSOF_SAVE);
}

void WinPara::CmdAction(sDInt code)
{
  if(Op && Op->Class->Actions)
  {
    if((*Op->Class->Actions)(Op,code,-1))
    {
      CmdLayout();
    }
  }
}

void WinPara::CmdCustomEd()
{
  if(Op && Op->Class->CustomEd)
  {
    App->StartCustomEditor((*Op->Class->CustomEd)(Op));
  }
}

void WinPara::CmdUndoRedo()
{
  if(UndoIsRedo)
  {
    Redo.CopyTo(Op);
    CmdLayout();
  }
  else
  {
    Redo.CopyFrom(Op);
    Undo.CopyTo(Op);
    CmdLayout();
  }
  Update();
}

void WinPara::CmdWiki()
{
  App->SwitchPageToggle(3);
  if(App->WikiToggleFlag)
    App->Wiki->SetOp(Op);
}

/****************************************************************************/

void FilterPresetName(const sStringDesc &name,const sChar *s)
{
  const sChar *s0,*s1;
  s0 = s;
  while(*s0 && *s0!='_') s0++; if(*s0) s0++;
  while(*s0 && *s0!='_') s0++; if(*s0) s0++;
  s1 = s0;
  while(*s1 && *s1!='.') s1++;
  sCopyString(name,s0);
  sInt len = s1-s0;
  if(len<name.Size)
    name.Buffer[len] = 0;
}

void WinPara::CmdPreset()
{
  App->Wiki->InitSvn();
  if(Op)
  {
    sDirEntry *ent;
    sString<sMAXPATH> path;
    sString<256> pattern;
    wDocName name;

    path.PrintF(L"%s/presets",App->WikiPath);
    pattern.PrintF(L"%s_%s_*.preset",Op->Class->OutputType->Symbol,Op->Class->Name);
    sLoadDir(PresetDir,path,pattern);

    sMenuFrame *mf = new sMenuFrame(this);
    mf->AddItem(L"default",sMessage(this,&WinPara::CmdPresetDefault),0);
    mf->AddItem(L"new preset",sMessage(this,&WinPara::CmdPresetAdd),0);
    mf->AddItem(L"overwrite preset",sMessage(this,&WinPara::CmdPresetOver),0);
    mf->AddItem(L"delete preset",sMessage(this,&WinPara::CmdPresetDelete),0);
    mf->AddCheckmark(L"keep elements",sMessage(),0,&PresetKeepArrayFlag,-1);
    mf->AddSpacer();
    sFORALL(PresetDir,ent)
    {
      FilterPresetName(name,ent->Name);
      if(name!=L"default")
        mf->AddItem(sPoolString(name),sMessage(this,&WinPara::CmdPresetSet,_i),0);
    }

    sGui->AddPulldownWindow(mf);
  }
}

void WinPara::CmdPresetAdd()
{
  sStringDialog(PresetName,sMessage(this,&WinPara::CmdPresetAdd2),sMessage(),L"Name for Preset");
}

void WinPara::CmdPresetAdd2()
{
  if(Op)
  {
    sString<sMAXPATH> filename;
    filename.PrintF(L"%s/presets/%s_%s_%s.preset",App->WikiPath,Op->Class->OutputType->Symbol,Op->Class->Name,PresetName);
    sSaveObject(filename,Op);
    App->Wiki->SetCommit();
  }
}

void WinPara::CmdPresetDelete()
{
  sDirEntry *ent;
  wDocName name;

  sMenuFrame *mf = new sMenuFrame(this);
  mf->AddHeader(L"delete");
  sFORALL(PresetDir,ent)
  {
    FilterPresetName(name,ent->Name);
    mf->AddItem(sPoolString(name),sMessage(this,&WinPara::CmdPresetDelete2,_i),0);
  }
  sGui->AddPopupWindow(mf);
}

void WinPara::CmdPresetDelete2(sDInt i)
{
  sDirEntry *ent;
  sString<sMAXPATH> filename;
  sString<sMAXPATH> svn;
  if(PresetDir.IsIndexValid(i))
  {
    ent = &PresetDir[i];
    filename.PrintF(L"%s/presets/%s",App->WikiPath,ent->Name);
    svn.PrintF(L"svn delete %s --keep-local",filename);
    App->Wiki->Execute(svn);
    sDeleteFile(filename);
    App->Wiki->SetCommit();
  }
}

void WinPara::CmdPresetOver()
{
  sDirEntry *ent;
  wDocName name;

  sMenuFrame *mf = new sMenuFrame(this);
  mf->AddHeader(L"overwrite");
  sFORALL(PresetDir,ent)
  {
    FilterPresetName(name,ent->Name);
    mf->AddItem(sPoolString(name),sMessage(this,&WinPara::CmdPresetOver2,_i),0);
  }
  sGui->AddPopupWindow(mf);
}

void WinPara::CmdPresetOver2(sDInt i)
{
  sDirEntry *ent;
  sString<sMAXPATH> filename;
  if(PresetDir.IsIndexValid(i) && Op)
  {
    ent = &PresetDir[i];
    filename.PrintF(L"%s/presets/%s",App->WikiPath,ent->Name);
    sSaveObject(filename,Op);
    App->Wiki->SetCommit();
  }
}

void WinPara::CmdPresetDefault()
{
  sString<sMAXPATH> filename;
  if(Op)
  {
    filename.PrintF(L"%s/presets/%s_%s_%s.preset",App->WikiPath,Op->Class->OutputType->Symbol,Op->Class->Name,L"default");
    LoadPreset(filename);
  }
}

void WinPara::CmdPresetSet(sDInt i)
{
  sDirEntry *ent;
  sString<sMAXPATH> filename;
  if(Op && PresetDir.IsIndexValid(i))
  {
    ent = &PresetDir[i];
    filename.PrintF(L"%s/presets/%s",App->WikiPath,ent->Name);
    LoadPreset(filename);
  }
}

void WinPara::LoadPreset(const sChar *filename)
{
  sArray<void *> safe;
  if(PresetKeepArrayFlag)
  {
    safe = Op->ArrayData;
    Op->ArrayData.Clear();
  }
  if(!sLoadObject(filename,Op))
    (*Op->Class->SetDefaults)(Op);
  if(PresetKeepArrayFlag)
  {
    Op->ArrayData = safe;
  }
  CmdConnect();
  SetOp(Op);
}

/****************************************************************************/
/****************************************************************************/

WinStoreBrowser::WinStoreBrowser() : sSingleTreeWindow<StoreBrowserItem>(&App->StoreTree,&StoreBrowserItem::TreeInfo)
{
  AddField(L"Name",0,200,sMEMBERPTR(StoreBrowserItem,Name));
  AddHeader();
  SelectMsg = sMessage(this,&WinStoreBrowser::CmdSelectOp);
  DoneMsg = sMessage(this,&WinStoreBrowser::CmdSelectDone);
}

void WinStoreBrowser::Goto(const sChar *name)
{
  StoreBrowserItem *item;
  sFORALL(App->StoreTree,item)
  {
    if(item->Name==name)
    {
      SetSelected(_i);
      OpenFolders(_i);
      this->ScrollTo(_i);
      break;
    }
  }
  StoreName = name;
}

void WinStoreBrowser::InitWire(const sChar *name)
{
  sSingleTreeWindow<StoreBrowserItem>::InitWire(name);
  sWire->AddKey(name,L"Close",sMessage(sWire,&sWireMasterWindow::CmdClosePopup));
  sWire->AddKey(name,L"SelectClose",sMessage(this,&WinStoreBrowser::CmdSelectClose));
}

WinStoreBrowser::~WinStoreBrowser()
{
}

void WinStoreBrowser::Tag()
{
  sSingleTreeWindow<StoreBrowserItem>::Tag();
}

void WinStoreBrowser::CmdSelectOp()
{
  StoreBrowserItem *item = GetSelected();
  wOp *op = 0;  

  if(item) 
  {
    StoreName = item->Name;
    op = Doc->FindStore(item->Name);
  }
  if(op)
  {
    App->ShowOp(op,2);
    OpMsg.Code = sDInt(op);
  }
  else
  {
    App->ShowOp(0,2);
    OpMsg.Code = 0;
  }
}

void WinStoreBrowser::CmdSelectDone()
{
  CmdSelectOp();
  OpMsg.Send();
  sWire->CmdClosePopup();
}

void WinStoreBrowser::CmdSelectClose()
{
  CmdSelectOp();
  OpMsg.Send();
  sWire->CmdClosePopup();
}

/****************************************************************************/

WinOpList::WinOpList() : sSingleListWindow<wOp>(0)
{
  SetArray(&Ops);

  // NEXT FREE ID: 7 (increment as you add new fields)
  AddField(L"Page",0,100,1,0);
  AddField(L"Class",0,50,2,0);
  AddField(L"Type",0,100,5,0);
  AddField(L"PosX",0,50,3,0);
  AddField(L"PosY",0,50,4,0);
  AddField(L"Name",0,150,6,0);
  
  Header = new sListWindow2Header(this);
  AddBorder(Header);
  SelectMsg = sMessage(this,&WinOpList::CmdSelect);
}

void WinOpList::References(const sChar *name)
{
  wOp *op;
  wOpInputInfo *info;
  Doc->Connect();
  Ops.Clear();
  sFORALL(Doc->AllOps,op)
  {
    sFORALL(op->Links,info)
    {
      if(info->LinkName==name)
      {
        Ops.AddTail(op);
        break;
      }
    }
  }
}

void WinOpList::FindClass(const sChar *name)
{
  wOp *op;
  Doc->Connect();
  Ops.Clear();
  sFORALL(Doc->AllOps,op)
  {
    if(op->Class->Label==name)
      Ops.AddTail(op);
  }
}

void WinOpList::FindString(const sChar *name)
{
  wOp *op;
  Ops.Clear();
  sFORALL(Doc->AllOps,op)
  {
    for(sInt i=0;i<op->EditStringCount;i++)
    {
      if(sCmpStringI(name,op->EditString[i]->Get())==0)
      {
        Ops.AddTail(op);
        break;
      }
    }
  }
}

void WinOpList::FindObsolete()
{
  wOp *op;
  Ops.Clear();
  sFORALL(Doc->AllOps,op)
  {
    if(op->Class->Flags & sCF_OBSOLETE)
      Ops.AddTail(op);
  }
}

WinOpList::~WinOpList()
{
}

void WinOpList::Tag()
{
  sNeed(Ops);
  Header->Need();
}

void WinOpList::InitWire(const sChar *name)
{
  sSingleListWindow<wOp>::InitWire(name);
  sWire->AddKey(name,L"Close",sMessage(sWire,&sWireMasterWindow::CmdClosePopup));
  sWire->AddKey(name,L"SelectClose",sMessage(this,&WinOpList::CmdSelectClose));
}

void WinOpList::OnCalcSize()
{
  sStaticListWindow::OnCalcSize();
/*
  ReqSizeX = 400;
  if(ReqSizeY<20)
    ReqSizeY = 300;
    */
}

sBool WinOpList::OnPaintField(const sRect &client,sListWindow2Field *field,sObject *obj,sInt line,sInt select)
{
  wStackOp *op = (wStackOp *) obj;
  wPage *page = op->Page;
  sBool ok = page && !page->IsTree;
  
  sString<256> buffer;
  switch(field->Id)
  {
  case 1:
    if(page)
      buffer = page->Name;
    PaintField(client,select,buffer);
    break;
  case 2:
    PaintField(client,select,op->Class->Label);
    break;
  case 3:
    if(ok)
      buffer.PrintF(L"%d",op->PosX);
    PaintField(client,select,buffer);
    break;
  case 4:
    if(ok)
      buffer.PrintF(L"%d",op->PosY);
    PaintField(client,select,buffer);
    break;
  case 5:
    PaintField(client,select,op->Class->OutputType->Label);
    break;
  case 6:
    PaintField(client,select,op->Name);
    break;
  default:

    return 0;
  }
  return 1;
}


void WinOpList::CmdSelectClose()
{
  wOp *op = GetSelected();
  App->GotoOp(op);
  sWire->CmdClosePopup();
}

void WinOpList::CmdSelect()
{
  wOp *op = GetSelected();
  if(op && op->Page && !op->Page->IsTree)
    App->GotoOp(op);
}

/****************************************************************************/
/****************************************************************************/

void WinStack::MakeRect(sRect &r,wStackOp *op,sInt dx,sInt dy,sInt dw,sInt dh)
{
  if(op)
  {
    dx += op->PosX;
    dy += op->PosY;
    dw += op->SizeX;
    dh += op->SizeY;
  }
  r.x0 = dx * OpXS + Client.x0;
  r.y0 = dy * OpYS + Client.y0;
  r.x1 = r.x0 + dw*OpXS;
  r.y1 = r.y0 + dh*OpYS;
}

const sChar *WinStack::MakeOpName(wStackOp *op)
{
  if((op->Class->Flags & (wCF_LOAD)) && !op->Links[0].LinkName.IsEmpty())   // load links are more important than load storenames
    return op->Links[0].LinkName;
  if(op->Name[0])
    return op->Name;
  if((op->Class->Flags & (wCF_CALL)) && !op->Links[0].LinkName.IsEmpty())   // call links are less important than call storenames
    return op->Links[0].LinkName;
  return op->Class->Label;
}

wStackOp *WinStack::GetHit(sInt mx,sInt my)
{
  wStackOp *op,*cop;
  sRect r;
  cop = 0;
  if(Page)
  {
    sFORALL(Page->Ops,op)
    {
      MakeRect(r,op);
      if(r.Hit(mx,my))
      {
        if(!(op->Class->Flags & wCF_COMMENT))
        {
          return op;
        }
        else
        {
          cop = op;
        }
      }
    }
  }
  return cop;
}

void WinStack::CursorStatus()
{
  App->Status->PrintF(STATUS_MESSAGE,L"cursor x:%d,y:%d",CursorX,CursorY);
}

/****************************************************************************/

WinStack::WinStack()
{
  OpXS = 24;
  OpYS = 16;
  CursorX = 0;
  CursorY = 0;
  Page = 0;
  LastOp = 0;
  HoverOp = 0;
  BirdseyeEnable = 0;
	MaxBlinkTimer = 2000;
  ShowArrows = 0;

  DragMoveMode = 0;
  DragRectMode = 0;

//  AddBorder(new sFocusBorder);
//  AddBorder(new sScrollBorder);
  Flags |= /*sWF_SCROLLY|sWF_SCROLLX|*/sWF_CLIENTCLIPPING;

  AddNotify(*Doc);
}

WinStack::~WinStack()
{
}

void WinStack::Tag()
{
  sWireClientWindow::Tag();
  Page->Need();
  LastOp->Need();
  HoverOp->Need();
}

void WinStack::InitWire(const sChar *name)
{
  sWireClientWindow::InitWire(name);

  sWire->AddTool(name,L"Birdseye"     ,sMessage(this,&WinStack::ToolBirdseye));
  sWire->AddTool(name,L"ShowArrows"   ,sMessage(this,&WinStack::ToolArrows));

  sWire->AddDrag(name,L"Select"       ,sMessage(this,&WinStack::DragSelect,0));
  sWire->AddDrag(name,L"SelectAdd"    ,sMessage(this,&WinStack::DragSelect,1));
  sWire->AddDrag(name,L"SelectToggle" ,sMessage(this,&WinStack::DragSelect,2));
  sWire->AddDrag(name,L"Frame"        ,sMessage(this,&WinStack::DragFrame,0));
  sWire->AddDrag(name,L"FrameAdd"     ,sMessage(this,&WinStack::DragFrame,1));
  sWire->AddDrag(name,L"FrameToggle"  ,sMessage(this,&WinStack::DragFrame,2));
  sWire->AddDrag(name,L"Width"        ,sMessage(this,&WinStack::DragMove,1));
  sWire->AddDrag(name,L"Duplicate"    ,sMessage(this,&WinStack::DragMove,2));
  sWire->AddDrag(name,L"BirdScroll"   ,sMessage(this,&WinStack::DragBirdScroll));
  sWire->AddDrag(name,L"ScratchSlow",sMessage(App->TimelineWin,&WinTimeline::DragScratchIndirect,0x100));
  sWire->AddDrag(name,L"ScratchFast",sMessage(App->TimelineWin,&WinTimeline::DragScratchIndirect,0x4000));

  sWire->AddKey(name,L"Add"       ,sMessage(this,&WinStack::CmdAddPopup,0));
  sWire->AddKey(name,L"AddMore"   ,sMessage(this,&WinStack::CmdAddPopup,1));
  sWire->AddKey(name,L"Conversions",sMessage(this,&WinStack::CmdAddConversion));
  sWire->AddKey(name,L"Del"       ,sMessage(this,&WinStack::CmdDel));
  sWire->AddKey(name,L"Cut"       ,sMessage(this,&WinStack::CmdCut));
  sWire->AddKey(name,L"Copy"      ,sMessage(this,&WinStack::CmdCopy));
  sWire->AddKey(name,L"Paste"     ,sMessage(this,&WinStack::CmdPaste));
  sWire->AddKey(name,L"SelectAll" ,sMessage(this,&WinStack::CmdSelectAll));
  sWire->AddKey(name,L"SelectTree",sMessage(this,&WinStack::CmdSelectTree));
  sWire->AddKey(name,L"Show"      ,sMessage(this,&WinStack::CmdShowOp,0));
  sWire->AddKey(name,L"Show2"     ,sMessage(this,&WinStack::CmdShowOp,1));
  sWire->AddKey(name,L"ShowRoot"  ,sMessage(this,&WinStack::CmdShowRoot,0));
  sWire->AddKey(name,L"Goto"      ,sMessage(this,&WinStack::CmdGoto));
  sWire->AddKey(name,L"Exchange"  ,sMessage(this,&WinStack::CmdExchange));
  sWire->AddKey(name,L"Bypass"    ,sMessage(this,&WinStack::CmdBypass));
  sWire->AddKey(name,L"Hide"      ,sMessage(this,&WinStack::CmdHide));
  sWire->AddKey(name,L"UnCache"   ,sMessage(this,&WinStack::CmdUnCache));
  sWire->AddKey(name,L"Up"        ,sMessage(this,&WinStack::CmdCursor,0));
  sWire->AddKey(name,L"Down"      ,sMessage(this,&WinStack::CmdCursor,1));
  sWire->AddKey(name,L"Left"      ,sMessage(this,&WinStack::CmdCursor,2));
  sWire->AddKey(name,L"Right"     ,sMessage(this,&WinStack::CmdCursor,3));
  sWire->AddKey(name,L"Add1"      ,sMessage(this,&WinStack::CmdAddType,'1'));
  sWire->AddKey(name,L"Add2"      ,sMessage(this,&WinStack::CmdAddType,'2'));
  sWire->AddKey(name,L"Add3"      ,sMessage(this,&WinStack::CmdAddType,'3'));
  sWire->AddKey(name,L"Add4"      ,sMessage(this,&WinStack::CmdAddType,'4'));
  sWire->AddKey(name,L"Add5"      ,sMessage(this,&WinStack::CmdAddType,'5'));
  sWire->AddKey(name,L"Add6"      ,sMessage(this,&WinStack::CmdAddType,'6'));
  sWire->AddKey(name,L"Add7"      ,sMessage(this,&WinStack::CmdAddType,'7'));
  sWire->AddKey(name,L"Add8"      ,sMessage(this,&WinStack::CmdAddType,'8'));
  sWire->AddKey(name,L"Add9"      ,sMessage(this,&WinStack::CmdAddType,'9'));
}

void WinStack::SetPage(wPage *page,sBool nogotopush)
{
  if(page && !page->IsTree)
  {
    if(Page!=page)
    {
      Page = page;
      ScrollX = page->ScrollX;
      ScrollY = page->ScrollY;
      if(!nogotopush)
        App->GotoPush();
    }
  }
  else
    Page = 0;
  Layout();
}

void WinStack::SetLastOp(wOp *op)
{
  if(op->Page && !op->Page->IsTree && op->Page==Page)
    LastOp = (wStackOp *)op;
}


sBool WinStack::OnCheckHit(const sWindowDrag &dd)
{
  if(!Page)
    return 0;
  else
    return GetHit(dd.MouseX,dd.MouseY)!=0;
}

void WinStack::OnCalcSize()
{
  ReqSizeX = wPAGEXS*OpXS;
  ReqSizeY = wPAGEYS*OpYS;
}

void WinStack::CmdBlinkTimer()
{
  BlinkToggle = !BlinkToggle;
  if(HasBlink)
    Update();
}

void WinStack::OnPaint2D()
{
  sRect r;
  wStackOp *op;
  const sChar *name;


  Doc->CurrentPage = Page;

  sBool hideunconnected = (Doc->EditOptions.Flags & wEOF_GRAYUNCONNECTED) ? 1 : 0;

  sClipPush();
  HasBlink = 0;
  if(Page)
  {
    Page->ScrollX = ScrollX;
    Page->ScrollY = ScrollY;
  }

  // birdseye

  if(BirdseyeEnable)
  {
    sRect r,ro;

    sInt xs = wPAGEXS*3;
    sInt ys = wPAGEYS*3;

    r.x0 = Inner.CenterX()-xs/2;
    r.y0 = Inner.CenterY()-ys/2;
    r.x1 = r.x0 + xs;
    r.y1 = r.y0 + ys;

    BirdsOuter = r;

    sClipPush();
    if(Page)
    {
      sFORALL(Page->Ops,op)
      {
        if(!(op->Class->Flags & wCF_COMMENT))
        {
          ro.x0 = r.x0 + op->PosX*3;
          ro.y0 = r.y0 + op->PosY*3;
          ro.x1 = ro.x0 + op->SizeX*3-1;
          ro.y1 = ro.y0 + op->SizeY*3-1;

          sSetColor2D(0,op->Class->OutputType->Color);
          sRect2D(ro,0);
          sClipExclude(ro);
        }
      }
    }

    //sSetColor2D(0,0xff404040);
    sRect2D(r,sGC_LOW2);
    sClipPop();

    ro.x0 = r.x0 + sMulDiv(-Client.x0+Inner.x0,r.SizeX(),Client.SizeX());
    ro.y0 = r.y0 + sMulDiv(-Client.y0+Inner.y0,r.SizeY(),Client.SizeY());
    ro.x1 = r.x0 + sMulDiv(-Client.x0+Inner.x1,r.SizeX(),Client.SizeX());
    ro.y1 = r.y0 + sMulDiv(-Client.y0+Inner.y1,r.SizeY(),Client.SizeY());
    sRectFrame2D(ro,sGC_DRAW);
    BirdsInner = ro;

    r.Extend(1);
    sRectFrame2D(r,sGC_DRAW);

    sClipExclude(r);
  }

  // warning

  if(Page)
  {
    sChar *warning = 0;
    if(Page->ManualWriteProtect)
      warning = L"manual write protection";
    if(Page->Include && Page->Include->Protected)
      warning = L"file is write protected";
    if(warning)
    {
      sRect r;
      r.x0 = Inner.CenterX()-125;
      r.y0 = Inner.CenterY()-25;
      r.x1 = r.x0+250;
      r.y1 = r.y0+50;
      sRectFrame2D(r,sGC_DRAW);
      r.Extend(-1);
      sGui->PropFont->SetColor(sGC_TEXT,sGC_DOC);
      sGui->PropFont->Print(sF2P_OPAQUE,r,warning);
      r.Extend(1);
      sClipExclude(r);
    }
  }

  // paint ops

  if(Page)
  {

    // tooltips

    if(HoverOp)
    {
      MakeRect(r,HoverOp);
      name = MakeOpName(HoverOp);
      const sChar *desc = HoverOp->Class->GetDescription ? HoverOp->Class->GetDescription(HoverOp) : 0;
      if(desc)
      {
        sInt namew = sGui->PropFont->GetWidth(name);
        sInt descw = sGui->PropFont->GetWidth(desc);
        
        sString<512> text;
        sInt xs;

        if(namew+4>r.SizeX())
        {
          text.PrintF(L"%s\n%s",name,desc);
          xs = sMax(namew,descw);
          r.y1 += sGui->PropFont->GetHeight();
        }
        else
        {
          text = desc;
          xs = descw;
          r.y0 = r.y1;
          r.y1 = r.y0 + sGui->PropFont->GetHeight() + 4;
        }
        
        r.x1 = r.x0+xs+8;
        r.Extend(-2);
        sGui->PropFont->SetColor(sGC_TEXT,sGC_SELECT);
        sGui->PropFont->Print(sF2P_OPAQUE|sF2P_LEFT|sF2P_SPACE|sF2P_MULTILINE,r,text);
        r.Extend(1);
        sRectFrame2D(r,sGC_DRAW);
        sClipExclude(r);
      }
      else
      {
        sInt xs = sGui->PropFont->GetWidth(name);
        if(xs+4>r.SizeX())
        {
          r.x1 = r.x0+xs+8;
          r.Extend(-2);
          sGui->PropFont->SetColor(sGC_TEXT,sGC_SELECT);
          sGui->PropFont->Print(sF2P_OPAQUE|sF2P_LEFT|sF2P_SPACE,r,name);
          r.Extend(1);
          sRectFrame2D(r,sGC_DRAW);
          sClipExclude(r);
        }
      }
    }

    // die ops selber

    sFORALL(Page->Ops,op)
    {
      if(!(op->Class->Flags & wCF_COMMENT))
      {
        MakeRect(r,op);
        sRect rr(r);

        sInt l0,h0;
        sInt l1,h1;
        sU32 color = op->Class->OutputType->Color;
        sInt textcolor = sGC_BLACK;

        if(0)     // load & stores einfärben: will ich nicht weil verwirrend
        {
          if(op->Class->OutputType == AnyTypeType)
          {
            if(op->Cache)
            {
              color = op->Cache->Type->Color;
            }
            else
            {
              wOp *iop = op->FirstInputOrLink();
              sInt max = 10;
              while(iop && max>0)
              {
                if(iop->Class->OutputType!=AnyTypeType)
                {
                  color = iop->Class->OutputType->Color;
                  break;
                }
                iop = iop->FirstInputOrLink();
                max--;
              }
            }
          }
        }

        if(op->SlowSkipFlag)
        {
          l0 = sGC_BLACK;
          h0 = sGC_BLACK;
          color = sGetColor2D(sGC_BUTTON);
        }
        else
        {
          if(op->Select)
          {
            l0 = sGC_LOW2;
            h0 = sGC_HIGH2;
          }
          else
          {
            h0 = sGC_LOW2;
            l0 = sGC_HIGH2;
          }
          /*
          if(op->Hide)
          {
            h0 = h1 = l0 = l1 = sGC_BACK;
          }
          */
          if(!op->NoError() && !op->Hide)
            color = sGetColor2D(sGC_RED);
        }
        if(hideunconnected && !op->ConnectedToRoot)
        {
          color = sGetColor2D(sGC_BUTTON);
          textcolor = sGC_LOW2;
        }
        if(!op->CheckShellSwitch())
        {
          color = sGetColor2D(sGC_BUTTON);
          textcolor = sGC_LOW2;
        }

        if(sGetTime()<op->BlinkTimer)
        {
          HasBlink = 1;

          if(0)                   // flash 3 times (soft)
          {
            sF32 phase = (3.0f - 0.5f) * (1.0f - 1.0f * (op->BlinkTimer - sGetTime()) / MaxBlinkTimer);
            sF32 fade = 0.5f * (1.0f + sFCos(phase * sPI2F));
            color = sColorFade(color,0xffffd0d0,fade);
          }
          else                    // hard aggessive blinking
          {
            if(BlinkToggle)
              color = 0xffff0000;
          }
        }

        if (op->Select)
          color=sMulColor(color,0xf8f8f8);

        l1=sColorFade(color,sGetColor2D(l0),0.5f);
        h1=sColorFade(color,sGetColor2D(h0),0.5f);

        sGui->RectHL(r,l0,h0); 
        r.Extend(-1);

        sSetColor2D(0,l1);
        sRect2D(r.x0,r.y0,r.x1,r.y0+1,0);
        sRect2D(r.x0,r.y0+1,r.x0+1,r.y1-1,0);
        sSetColor2D(0,h1);
        sRect2D(r.x0,r.y1-1,r.x1,r.y1,0);
        sRect2D(r.x1-1,r.y0+1,r.x1,r.y1-1,0);
        r.Extend(-1);

        sSetColor2D(0,color);
        sGui->PropFont->SetColor(textcolor,0);


        name = MakeOpName(op);
        sInt xs = sGui->PropFont->GetWidth(name);
        sInt namepf = (xs+4>r.SizeX()) ? sF2P_LEFT|sF2P_SPACE : 0;

        if(op->Hide)
        {
          sRect2D(r,0);
          sLine2D(r.x0,r.y0+0,r.x1-1,r.y1-2,sGC_RED);
          sLine2D(r.x0,r.y0+1,r.x1-1,r.y1-1,sGC_RED);
          sLine2D(r.x0,r.y1-2,r.x1-1,r.y0+0,sGC_RED);
          sLine2D(r.x0,r.y1-1,r.x1-1,r.y0+1,sGC_RED);
          sGui->PropFont->Print(namepf,r,name);
        }
        else
        {
          sGui->PropFont->Print(namepf|sF2P_OPAQUE,r,name);
        }
        if(op->Class->Flags & sCF_OBSOLETE)
        {
          sGui->PropFont->SetColor(sGC_RED,sGC_BUTTON);
          sGui->PropFont->Print(0,r,L"obsolete");
          sGui->PropFont->SetColor(sGC_TEXT,sGC_BUTTON);
        }


        if(App->IsShown(op))
        {
          sInt y = r.y0+2;
          sRect2D(r.x0+4,y,r.x0+8,y+2,sGC_GREEN);
        }
        if(App->IsEdited(op))
        {
          sInt y = (r.y0+r.y1)/2-1;
          sRect2D(r.x0+4,y,r.x0+8,y+2,sGC_RED);
        }
        if(op->Cache)
        {
          sInt y = r.y1-2-2;
          sRect2D(r.x0+4,y,r.x0+8,y+2,sGC_BLUE);
        }


        if((op->Class->Flags & wCF_LOAD) && !op->Hide)
        {
          sRect2D(rr.x0,rr.y0  ,rr.x0+4,rr.y0+2,sGC_BUTTON);
          sRect2D(rr.x0,rr.y0+2,rr.x0+2,rr.y0+4,sGC_BUTTON);
          sSetColor2D(0,l1);
          sLine2D(rr.x0+1,rr.y0+4,rr.x0+5,rr.y0  ,0);
          sLine2D(rr.x0,rr.y0+4,rr.x0+4,rr.y0  ,l0);

          sRect2D(rr.x1-4,rr.y0  ,rr.x1,rr.y0+2,sGC_BUTTON);
          sRect2D(rr.x1-2,rr.y0+2,rr.x1,rr.y0+4,sGC_BUTTON);
          sSetColor2D(0,h1);
          sLine2D(rr.x1-5,rr.y0  ,rr.x1-1,rr.y0+4,0);
          sLine2D(rr.x1-4,rr.y0  ,rr.x1,rr.y0+4,h0);
        }

        if((op->Class->Flags & wCF_STORE) && !op->Hide)
        {
          sRect2D(rr.x0  ,rr.y1-2,rr.x0+4,rr.y1  ,sGC_BUTTON);
          sRect2D(rr.x0  ,rr.y1-4,rr.x0+2,rr.y1-2,sGC_BUTTON);
          sSetColor2D(0,l1);
          sLine2D(rr.x0+1,rr.y1-5,rr.x0+4,rr.y1-2,0);
          sLine2D(rr.x0  ,rr.y1-5,rr.x0+4,rr.y1-1,l0);

          sRect2D(rr.x1-4,rr.y1-2,rr.x1  ,rr.y1  ,sGC_BUTTON);
          sRect2D(rr.x1-2,rr.y1-4,rr.x1  ,rr.y1-2,sGC_BUTTON);
          sSetColor2D(0,h1);
          sLine2D(rr.x1-5,rr.y1-2,rr.x1-1,rr.y1-6,0);
          sLine2D(rr.x1-5,rr.y1-1,rr.x1-1,rr.y1-5,h0);
        }

        if((op->Class->Flags & wCF_CALL) && !op->Hide)
        {
          sInt y= rr.CenterY();
          sRect2D(rr.x0  ,y-3,rr.x0+3,y+4  ,sGC_BUTTON);
          sSetColor2D(0,h1);
          sLine2D(rr.x0+1,y-4,rr.x0+5,y+0,0);
          sLine2D(rr.x0  ,y-4,rr.x0+5,y+1,h0);
          sSetColor2D(0,l1);
          sLine2D(rr.x0+2,y+3,rr.x0+5,y-0,0);
          sLine2D(rr.x0+1,y+3,rr.x0+5,y-1,l0);
          
        }

        if(op->Bypass)
        {
          sInt x = rr.CenterX();
          sRect2D(x-1,rr.y0,x+1,rr.y1,sGC_RED);
        }

        sClipExclude(rr);
      }
    }

    // comment ops darunter

    sFORALL(Page->Ops,op)
    {
      if((op->Class->Flags & wCF_COMMENT))
      {
        static sU32 colors[8] = { 0xffffff,0xff0000,0xffff00,0x00ff00,0x00ffff,0x0000ff,0xff00ff,0x000000 };
        MakeRect(r,op);
        r.Extend(4);
        sRect rr(r);

        sU32 l0,l1,h0,h1;
        sU32 color = colors[op->EditU()[0]&7]|0xffc0c0c0;
        if(op->Select)
        {
          l0 = sGC_LOW2;
          h0 = sGC_HIGH2;
        }
        else
        {
          h0 = sGC_LOW2;
          l0 = sGC_HIGH2;
        }

        if (op->Select)
          color=sMulColor(color,0xf8f8f8);

        l1=sColorFade(color,sGetColor2D(l0),0.5f);
        h1=sColorFade(color,sGetColor2D(h0),0.5f);

        sGui->RectHL(r,l0,h0); 
        r.Extend(-1);

        sSetColor2D(0,l1);
        sRect2D(r.x0,r.y0,r.x1,r.y0+1,0);
        sRect2D(r.x0,r.y0+1,r.x0+1,r.y1-1,0);
        sSetColor2D(0,h1);
        sRect2D(r.x0,r.y1-1,r.x1,r.y1,0);
        sRect2D(r.x1-1,r.y0+1,r.x1,r.y1-1,0);
        r.Extend(-1);

        sSetColor2D(0,color);
        sGui->PropFont->SetColor(sGC_BLACK,0);

        const sChar *text = L"";
        if(op->EditStringCount>0 && op->EditString[0])
          text = op->EditString[0]->Get();
        sGui->PropFont->Print(sF2P_OPAQUE|sF2P_MULTILINE,r,text,-1,3);

        sClipExclude(rr);
      }
    }
  }

  // marks
 
  sRect2D(Client,sGC_BUTTON);

  if(Page)
  {
    MakeRect(r,0,CursorX,CursorY,3,1);
    sRectFrame2D(r,sGC_DRAW);
  }

  sClipPop();

  // lines

  if(ShowArrows)
  {
    wStackOp *op;
    sFORALL(Page->Ops,op)
    {
      if((op->Class->Flags & wCF_LOAD) && op->Links.GetCount()==1 && op->Links[0].Link)
      {
        wStackOp *target = (wStackOp *)op->Links[0].Link;
        if(target->Page==Page)
        {
          sRect r0,r1;
          MakeRect(r0,op);
          MakeRect(r1,target);

          sInt x0 = r0.CenterX();
          sInt y0 = r0.CenterY();
          sInt x1 = r1.CenterX();
          sInt y1 = r1.CenterY();
          sF32 l = 0.1f*sSqrt(sSquare(x0-x1)+sSquare(y0-y1));
          sInt dx = (x1-x0)/l;
          sInt dy = (y1-y0)/l;
          
          for(sInt i=0;i<5;i++)
          {
            static const sInt jx[5] = { 0,-1,0,1,0 };
            static const sInt jy[5] = { 0,0,-1,0,1 };
            sInt x = jx[i];
            sInt y = jy[i];

            sLine2D(x+x0,y+y0,x+x1      ,y+y1      ,sGC_YELLOW);
            sLine2D(x+x0,y+y0,x+x0+dy+dx,y+y0-dx+dy,sGC_YELLOW);
            sLine2D(x+x0,y+y0,x+x0-dy+dx,y+y0+dx+dy,sGC_YELLOW);
          }
        }
      }
    }
  }

  // dragging

  if(DragRectMode)
  {
    sRectFrame2D(DragRect,sGC_DRAW);
  }

  if(DragMoveMode && Page)
  {
    sFORALL(Page->Ops,op)
    {
      if(op->Select)
      {
        MakeRect(r,op,DragMoveX,DragMoveY,DragMoveW,DragMoveH);
        for(sInt i=0;i<DragMoveMode;i++)
        {
          sRectFrame2D(r,sGC_DRAW);
          r.Extend(-1);
        }
      }
    }
  }
}

/****************************************************************************/

void WinStack::GotoOp(wOp *op,sBool blink)
{
  wStackOp *pop;
  sFORALL(Page->Ops,pop)
    pop->Select = 0;

  op->Select = 1;
  sRect r;
  MakeRect(r,(wStackOp *)op);
  ScrollTo(r,1);
  if(blink) 
    op->BlinkTimer = sGetTime()+MaxBlinkTimer;
  Update();
}

void WinStack::CmdGoto()
{
  wOp *op = App->GetEditOp();
  if(op)
  {
    if(op->Class->CustomEd)
    {
      App->StartCustomEditor((*op->Class->CustomEd)(op));
    }
    if(op->Links.GetCount()>0 && op->Links[0].Link)
    {
      App->GotoOp(op->Links[0].Link);
    }
  }
}

/****************************************************************************/

void WinStack::ToolBirdseye(sDInt enable)
{
  BirdseyeEnable = enable;
  Update();
}

void WinStack::ToolArrows(sDInt enable)
{
  ShowArrows = enable;
  Update();
}

void WinStack::DragBirdScroll(const sWindowDrag &dd)
{
  sInt x,y;
  switch(dd.Mode)
  {
  case sDD_START:
    if(BirdsInner.Hit(dd.MouseX,dd.MouseY))
    {
      DragBirdX = dd.MouseX - BirdsInner.x0;
      DragBirdY = dd.MouseY - BirdsInner.y0;
    }
    else
    {
      DragBirdX = BirdsInner.SizeX()/2;
      DragBirdY = BirdsInner.SizeY()/2;
    }
    break;

  case sDD_DRAG:
    x = sMulDiv(dd.MouseX-BirdsOuter.x0-DragBirdX,Client.SizeX(),BirdsOuter.SizeX());
    y = sMulDiv(dd.MouseY-BirdsOuter.y0-DragBirdY,Client.SizeY(),BirdsOuter.SizeY());
    ScrollTo(x,y);
    break;
  }
}

/****************************************************************************/

void WinStack::OnDrag(const sWindowDrag &dd)
{
  wStackOp *op;
  switch(dd.Mode)
  {
  case sDD_START:
    CursorX = (dd.StartX - Client.x0) / OpXS - 1;
    CursorY = (dd.StartY - Client.y0) / OpYS;
    CursorStatus();
    break;
  case sDD_HOVER:
    op = GetHit(dd.MouseX,dd.MouseY);
    if(op!=HoverOp)
    {
      HoverOp = op;
      Update();
    }
    break;
  }

  sWireClientWindow::OnDrag(dd); 
}


void WinStack::DragSelect(const sWindowDrag &dd,sDInt mode)
{
  wStackOp *op,*opall;
  if(Page && dd.Mode==sDD_START)
  {
    op = GetHit(dd.StartX,dd.StartY);
    if(mode==0 && (!op || !op->Select))
      sFORALL(Page->Ops,opall)
        opall->Select = 0;
    if(op)
    {
      if(mode==2)
        op->Select = !op->Select;
      else
        op->Select = 1;
    }
    App->EditOp(op,0);
    LastOp = op;
  }
  if(mode==0)
    DragMove(dd,0);
}

void WinStack::DragFrame(const sWindowDrag &dd,sDInt mode)
{
  wStackOp *op;
  sRect r;
  switch(dd.Mode)
  {
  case sDD_START:
    DragRectMode = 1;
    DragRect.Init(dd.StartX,dd.StartY,dd.MouseX,dd.MouseY);
    DragRect.Sort();
    LastOp = 0;
    Update();
    break;
  case sDD_DRAG:
    DragRect.Init(dd.StartX,dd.StartY,dd.MouseX,dd.MouseY);
    DragRect.Sort();
    Update();
    break;
  case sDD_STOP:
    DragRectMode = 0;
    if(Page)
    {
      if(mode==0)
        sFORALL(Page->Ops,op)
          op->Select = 0;
      sFORALL(Page->Ops,op)
      {
        if(!(op->Class->Flags & wCF_COMMENT))
        {
          MakeRect(r,op);
          if(r.IsInside(DragRect))
          {
  //          GotoClear();
            if(mode==2)
              op->Select = !op->Select;
            else
              op->Select = 1;
          }
        }
        else
        {
          op->Select = 0;
        }
      }
    }
    Update();
    break;
  }
}

void WinStack::DragMove(const sWindowDrag &dd,sDInt mode)  
{
  wStackOp *op,*opall,*opnew;
  if(Page->IsProtected()) return;
  switch(dd.Mode)
  {
  case sDD_START:
    op = GetHit(dd.StartX,dd.StartY);
    if(op && op->Select==0)
    {
      sVERIFY(Page);
      sFORALL(Page->Ops,opall)
        opall->Select = 0;
      op->Select = 1;
//      GotoClear();
    }
    DragMoveMode = (mode==2) ? 2 : 1;
    DragMoveX = 0;
    DragMoveY = 0;
    DragMoveW = 0;
    DragMoveH = 0;
    Update();
    break;
  case sDD_DRAG:
    if(mode!=1)
    {
      DragMoveX = (dd.DeltaX+1024*OpXS+OpXS/2)/OpXS-1024;
      DragMoveY = (dd.DeltaY+1024*OpYS+OpYS/2)/OpYS-1024;
    }
    else
    {
      op = GetHit(dd.StartX,dd.StartY);
      DragMoveW = (dd.DeltaX+1024*OpXS+OpXS/2)/OpXS-1024;
      if(op->Class->Flags & wCF_VERTICALRESIZE)
        DragMoveH = (dd.DeltaY+1024*OpYS+OpYS/2)/OpYS-1024;
    }
    Update();
    break;
  case sDD_STOP:
    DragMoveMode = 0;
    if(DragMoveX || DragMoveY || DragMoveW || DragMoveH)
    {
      if(Page && Page->CheckMove(DragMoveX,DragMoveY,DragMoveW,DragMoveH,mode!=2))
      {
        sFORALL(Page->Ops,op)
        {
          if(op->Select)
          {
            opnew = op;
            if(mode==2)
            {
              opnew = new wStackOp();
              opnew->CopyFrom(op);
              Page->Ops.AddTail(opnew);
            }
            opnew->PosX = op->PosX+DragMoveX;
            opnew->PosY = op->PosY+DragMoveY;
            opnew->SizeX = sClamp(op->SizeX+DragMoveW,1,wPAGEXS-op->PosX);
            opnew->SizeY = sClamp(op->SizeY+DragMoveH,1,wPAGEYS-op->PosY);
            App->ChangeDoc();
          }
        }
      }
    }

    Doc->Connect();
    Update();
    break;
  }
}

/****************************************************************************/

void WinStack::CmdShowOp(sDInt n)
{
  if(LastOp)
  {
    if(LastOp->Hide)
      App->ShowOp(0,n);
    else
      App->ShowOp(LastOp,n);
    if(n==3)
      sWire->SwitchScreen(3);
  }
}

void WinStack::CmdShowRoot(sDInt win)
{
  wOp *op = Doc->FindStore(L"root");
  if(op)
    App->ShowOp(op,win);
}

void WinStack::CmdCursor(sDInt udlr)
{
  switch(udlr)
  {
  case 0:
    if(CursorY>0)
      CursorY--;
    break;
  case 1:
    if(CursorY<wPAGEYS)
      CursorY++;
    break;
  case 2:
    if(CursorX>0)
      CursorX--;
    break;
  case 3:
    if(CursorX<wPAGEXS)
      CursorX++;
    break;
  }

  wStackOp *op;
  sFORALL(Page->Ops,op)
  {
    op->Select = 0;
    if(CursorX>=op->PosX && CursorX<op->PosX+op->SizeX &&
       CursorY>=op->PosY && CursorY<op->PosY+op->SizeY)
    {
      op->Select = 1;
      App->EditOp(op,0);
    }
  }
  CursorStatus();
  Update();
}

void WinStack::CmdDel()
{
  if(Page->IsProtected()) return;
  if(Page)
  {
    wOp *op;
    sFORALL(Page->Ops,op)
      if(op->Select)
        App->ClearOp(op);

    sRemTrue(Page->Ops,&wStackOp::Select);
    Doc->Connect();
    App->ChangeDoc();
    App->GotoClear();
    Update();
  }
}

void WinStack::CmdAddPopup(sDInt sec)
{
  if(Page->IsProtected()) return;
  App->AddPopup(sMessage(this,&WinStack::CmdAdd),sec);
}

void WinStack::CmdAddType(sDInt type)
{
  if(Page->IsProtected()) return;
  App->AddPopup(sMessage(this,&WinStack::CmdAdd),0,type);
}

void WinStack::CmdAddConversion()
{
  if(Page->IsProtected()) return;
  App->AddConversionPopup(sMessage(this,&WinStack::CmdAdd));
}

void WinStack::CmdAdd(sDInt nr)
{
  if(Page->IsProtected()) return;
  if(Page && nr>=0 && nr<Doc->Classes.GetCount())
  {
    sInt width = 3;
//    sInt width = Doc->Classes[nr]->Inputs.GetCount()>1 ? 6 : 3;

    if(width==6 && !Page->CheckDest(0,CursorX,CursorY,width,1,0))
      width = 3;

    if(Page->CheckDest(0,CursorX,CursorY,width,1,0))
    {
      wStackOp *op = new wStackOp();
      op->Init(Doc->Classes[nr]);
      App->LoadOpPreset(op,L"default");
      op->PosX = CursorX;
      op->PosY = CursorY;
      op->SizeX = width;

      Page->Ops.AddTail(op);
      Doc->Connect();
      App->ChangeDoc();
      App->EditOp(op,0);

      if(CursorY < wPAGEYS-1)
        CursorY++;

      CursorStatus();
      Update();
    }
  }
}

void WinStack::CmdCut()
{
  CmdCopy();
  if(Page->IsProtected()) return;
  CmdDel();
  App->ChangeDoc();
}

void WinStack::CmdCopy()
{
  wStackOp *op;
  sArray<wStackOp*> ops;

  if(Page)
  {
    sFORALL(Page->Ops,op)
      if(op->Select)
        ops.AddTail(op);

    if(ops.GetCount()>0)
      sSetClipboardArray(ops,sSerId::wStackOp);
  }
}

void WinStack::CmdPaste()
{
  if(Page->IsProtected()) return;
  if(Page)
  {
    sArray<wStackOp*> ops;
    if(sGetClipboardArray(ops,sSerId::wStackOp))
    {
      wStackOp *op;
      sInt xmin,ymin;
      xmin = wPAGEXS;
      ymin = wPAGEYS;
      sFORALL(ops,op)
      {
        xmin = sMin(xmin,op->PosX);
        ymin = sMin(ymin,op->PosY);
      }
      sFORALL(ops,op)
      {
        if(!Page->CheckDest(op,op->PosX-xmin+CursorX,op->PosY-ymin+CursorY,op->SizeX,op->SizeY,0))
          return;
      }

      sFORALL(Page->Ops,op)
        op->Select = 0;

      sFORALL(ops,op)
      {
        op->PosX = op->PosX - xmin + CursorX;
        op->PosY = op->PosY - ymin + CursorY;      
        op->Select = 1;
        Page->Ops.AddTail(op);
      }

      App->ChangeDoc();
      Doc->Connect();
      Update();

      if(ops.GetCount() == 1) // 1 op pasted; auto-select it.
        App->EditOp(ops[0],0);
    }
    sArray<wTreeOp*> tops;
    if(sGetClipboardArray(tops,sSerId::wTreeOp))
    {
      wStackOp *op;
      wTreeOp *top;
      sInt max = tops.GetCount();

      sFORALL(tops,top)
      {
        if(!Page->CheckDest(0,CursorX,CursorY+_i,3,1,0))
          return;
      }

      sFORALL(tops,top)
      {
        op = new wStackOp;
        ((wOp *)op)->CopyFrom(top);
        op->PosX = CursorX;
        op->PosY = CursorY + (max-1) - _i;
        op->SizeX = 3;
        op->SizeY = 1;
        op->Select = 1;
        Page->Ops.AddTail(op);
      }
      App->ChangeDoc();
      Doc->Connect();
      Update();
    }
  }
}

void WinStack::CmdSelectAll()
{
  wStackOp *op;

  if(Page)
    sFORALL(Page->Ops,op)
      op->Select = 1;

  Update();
}

void WinStack::CmdSelectTree()
{
  if(Page)
  {
    wOp *op,*r,*rr;
    sArray<wOp *> active;
    sArray<wOp *> current;

    Doc->Connect();

    sFORALL(Page->Ops,op)
      if(op->Select)
        active.AddTail(op);

    while(active.GetCount()>0)
    {
      current = active;
      active.Clear();
      sFORALL(current,op)
      {
        sFORALL(op->Inputs,r)
        {
          if(r->Select==0)
          {
            r->Select = 1;
            active.AddTail(r);
          }
        }
        sFORALL(op->Outputs,r)
        {
          if(r->Select==0)
          {
            sFORALL(r->Inputs,rr)
            {
              if(rr==op)
              {
                r->Select = 1;
                active.AddTail(r);
              }
            }
          }
        }
      }
    }
  }

  Update();
}

static sBool ReInit(wOp *op,wClass *cl)
{
  sInt countu = sMin(op->Class->ParaWords,cl->ParaWords);
  sInt counts = sMin(op->Class->ParaStrings,cl->ParaStrings);

  sU32 *datau = new sU32[countu];
  sPoolString *datas = new sPoolString[counts];

  for(sInt i=0;i<countu;i++)
    datau[i] = op->EditU()[i];
  for(sInt i=0;i<counts;i++)
    datas[i] = op->EditString[i]->Get();

  op->Init(cl);

  for(sInt i=0;i<countu;i++)
    op->EditU()[i] = datau[i];
  for(sInt i=0;i<counts;i++)
  {
    op->EditString[i]->Clear();
    op->EditString[i]->Print((const sChar *) datas[i]);
  }
  Doc->Change(op);

  delete[] datau;
  delete[] datas;

  return 1;
}

void WinStack::CmdExchange()
{
  if(Page->IsProtected()) return;
  wStackOp *op;
  wDocName name;
  wClass *loadclass = Doc->FindClass(L"Load",L"AnyType");
  wClass *storeclass = Doc->FindClass(L"Store",L"AnyType");

  wClass *ex[3][2];
  sInt max = 0;

  sClear(ex);
 
  ex[max][0] = Doc->FindClass(L"Transform",L"Wz4Render");
  ex[max][1] = Doc->FindClass(L"Transform",L"Wz4Mesh");
  max++;
  ex[max][0] = Doc->FindClass(L"Render",L"Wz4Render");
  ex[max][1] = Doc->FindClass(L"Add",L"Wz4Mesh");
  max++;
  ex[max][0] = Doc->FindClass(L"Multiply",L"Wz4Render");
  ex[max][1] = Doc->FindClass(L"MultiplyNew",L"Wz4Mesh");
  max++;

  sBool changed = 0;
  if(Page && loadclass && storeclass)
  {
    sFORALL(Page->Ops,op)
    {
      if(op->Select)
      {
        if(op->Class == loadclass)
        {
          name = op->Links[0].LinkName;
          op->Init(storeclass);
          op->Name = name;
          Doc->Change(op);
          changed = 1;
        }
        else if(op->Class == storeclass)
        {
          name = op->Name;
          op->Init(loadclass);
          op->Links[0].LinkName = name;
          op->Name = L"";
          Doc->Change(op);
          changed = 1;
        }
        for(sInt i=0;i<max;i++)
        {
          if(ex[i][0] && ex[i][1])
          {
            if(op->Class==ex[i][0])
            {
              changed |= ReInit(op,ex[i][1]);
              break;
            }
            else if(op->Class==ex[i][1])
            {
              changed |= ReInit(op,ex[i][0]);
              break;
            }
          }
        }
      }
    }
  }

  if(changed)
  {
    App->EditOpReloadAll();
    App->ChangeDoc();
    Doc->Connect();
  }
}

void WinStack::CmdBypass()
{
  if(Page->IsProtected()) return;
  wStackOp *op;
  wDocName name;

  sBool changed = 0;
  if(Page)
  {
    sFORALL(Page->Ops,op)
    {
      if(op->Select)
      {
        op->Bypass = !op->Bypass;
        changed = 1;
      }
    }
  }
  if(changed)
  {
    App->ChangeDoc();
    Doc->Connect();
  }
}

void WinStack::CmdHide()
{
  if(Page->IsProtected()) return;
  wStackOp *op;

  sBool changed = 0;
  if(Page)
  {
    sFORALL(Page->Ops,op)
    {
      if(op->Select)
      {
        op->Hide = !op->Hide;
        changed = 1;
      }
    }
  }
  if(changed)
  {
    App->ChangeDoc();
    Doc->Connect();
  }
}

void WinStack::CmdUnCache()
{
  wStackOp *op;

  if(Page)
  {
    sFORALL(Page->Ops,op)
    {
      if(op->Select)
        Doc->Change(op);
    }
  }
  Update();
}

/****************************************************************************/

WinTimeline::WinTimeline()
{
  Height = sGui->PropFont->GetHeight()+3;
  Start = 0;
  End = 0;
  LoopStart = 0;
  LoopEnd = 0;
  LoopEnable = 0;
  Time = 0;
  ScratchMode = 0;
  Pause = 1;
  TimeMin = 0;
}

WinTimeline::~WinTimeline()
{
}

void WinTimeline::InitWire(const sChar *name)
{
  sWireClientWindow::InitWire(name);
  sWire->AddDrag(name,L"Scratch",sMessage(this,&WinTimeline::DragScratch));
  sWire->AddDrag(name,L"Loop",sMessage(this,&WinTimeline::DragLoop));
  sWire->AddKey(name,L"ToggleLoop",sMessage(this,&WinTimeline::CmdToogleLoop));
  sWire->AddDrag(name,L"ScratchSlow",sMessage(this,&WinTimeline::DragScratch,0x100));
  sWire->AddDrag(name,L"ScratchFast",sMessage(this,&WinTimeline::DragScratch,0x4000));
}

void WinTimeline::OnPaint2D()
{
  sRect r = Client;
  sString<64> s;
  sInt x;

  sSetColor2D(sGC_MAX,0xffff8080);

  sGui->RectHL(r,sGC_HIGH,sGC_LOW);
  r.Extend(-1);

  sGui->PropFont->SetColor(sGC_TEXT,sGC_BACK);

  if(End>Start)
  {
    sBool fine = (BeatToPos(0x10000)-BeatToPos(0x00000) < 4);
    if(LoopStart<LoopEnd && LoopEnd>Start && LoopStart<End)
    {
      sInt x0 = r.x0;
      sInt x1 = BeatToPos(LoopStart);
      sInt x2 = BeatToPos(LoopEnd);
      sInt x3 = r.x1;
      x1 = sClamp(x1,x0,x3);
      x2 = sClamp(x2,x0,x3);
      sRect2D(x0,r.y0,x1,r.y1,sGC_BACK);
      sRect2D(x1,r.y0,x2,r.y1,LoopEnable ? sGC_MAX : sGC_SELECT);
      sRect2D(x2,r.y0,x3,r.y1,sGC_BACK);
    }
    else
    {
      sRect2D(r,sGC_BACK);
    }
    sInt step = 3;
    if(BeatToPos(0x40000)-BeatToPos(0x00000)<30)
      step = 15;
    for(sInt i=(Start+0xffff)/0x10000;i<End/0x10000;i++)
    {
      x = BeatToPos(i*0x10000);
      if((i&15)==0)
        sRect2D(x,r.y0,x+1,r.y1,sGC_DRAW);
      else if((i&3)==0)
        sRect2D(x,r.y0+5,x+1,r.y1,sGC_DRAW);
      else if(!fine)
        sRect2D(x,r.y1-5,x+1,r.y1,sGC_DRAW);
    }
    for(sInt i=(Start+0xffff)/0x10000;i<End/0x10000;i++)
    {
      if((i&step)==0)
      {
        x = BeatToPos(i*0x10000);
        s.PrintF(L"%d",i/*+1*/);
        sGui->PropFont->Print(0,x+2,r.y0,s);
      }
    }
    x = BeatToPos(Time);
    sRect2D(x,r.y0,x+2,r.y1,sGC_RED);
  }
  else
  {
    sGui->PropFont->Print(sF2P_OPAQUE,r,L"(no timeline)");
  }
}

void WinTimeline::OnCalcSize()
{
  ReqSizeX = 0;
  ReqSizeY = Height;
}

void WinTimeline::OnLayout()
{
  Client = Parent->Inner;
  Parent->Inner.y1 = Client.y0 = Client.y1 - Height;
}

sInt WinTimeline::PosToBeat(sInt x)
{
  return sClamp(sMulDiv(x-(Client.x0+1),End-Start,Client.SizeX()-2)+Start,Start,End);
}

sInt WinTimeline::BeatToPos(sInt beat)
{
  return sClamp(Client.x0+1 + sMulDiv(beat-Start,Client.SizeX()-2,End-Start),Client.x0+1,Client.x1-1);
}

void WinTimeline::UpdateSample(sInt sample)
{
  if(!ScratchMode && !Pause)
  {
    sInt xa = BeatToPos(Time);
    Time = Doc->SampleToBeats(sample);
    if(TimeMin>=0)     // prevent time from going backwards due to latency
    {
      if(Time<TimeMin)
        Time = TimeMin;
      else
        TimeMin = -1;
    }
    if(LoopEnable && LoopStart<LoopEnd)
    {
     if(Time<LoopStart) 
        Time += LoopEnd-LoopStart;
    }
    else
    {
      if(Time<Start)
        Time += End-Start;
    }
    sInt xb = BeatToPos(Time);
    if(xa!=xb)
    {
      sGui->Update(Client);
    }
  }
}

void WinTimeline::SetRange(sInt start,sInt end)
{
  if(start!=Start || end!=End)
  {
    Start = start;
    End = end;
    Time = sClamp(Time,Start,End);
    if(LoopStart<Start || LoopStart>=End || LoopEnd<Start || LoopEnd>=End)
    {
      LoopStart = 0;
      LoopEnd = 0;
      LoopEnable = 0;
    }
    Update();
  }
}

void WinTimeline::DragScratch(const sWindowDrag &dd)
{
  sRect r = Client;
  r.Extend(-1);
  sInt t;

  switch(dd.Mode)
  {
  case sDD_START:
    ScratchDragStart();
    break;
  case sDD_DRAG:
    t = sClamp(sMulDiv(dd.MouseX-r.x0,End-Start,r.SizeX())+Start,Start,End);
    ScratchDrag(t);
    break;
  case sDD_STOP:
    ScratchDragStop();
    break;
  }
}

void WinTimeline::ScratchDragStart()
{
  ScratchMode = 1;
  ScratchMsg.Post();
  ChangeMsg.Post();
}

sBool WinTimeline::ScratchDrag(sF32 t)
{
  if(t!=Time)
  {
    sRect r(Client);
    r.x0 = BeatToPos(t);
    r.x1 = BeatToPos(Time);
    r.Sort();
    r.x0-=2;
    r.x1+=2;

    TimeMin = Time = t;      
    ScratchMsg.Post();
    ChangeMsg.Post();
    sInt secs = Doc->BeatsToMilliseconds(Time)/1000;
    App->Status->PrintF(STATUS_MESSAGE,L"beat %3.6f, time %d:%02d",(t/0x10000),secs/60,secs%60);

    sGui->Update(r);
    return 1;
  }
  return 0;
}

void WinTimeline::ScratchDragStop()
{
  ScratchMode = 0;
  ScratchMsg.Post();
  ChangeMsg.Post();
}

void WinTimeline::DragScratchIndirect(const sWindowDrag &dd,sDInt speed)
{
  sRect r = Client;
  r.Extend(-1);
  sInt t;

  switch(dd.Mode)
  {
  case sDD_START:
    ScratchStart = Time;
    ScratchDragStart();
    break;
  case sDD_DRAG:
    t = sMax(0,ScratchStart + dd.HardDeltaX*sInt(speed));
    ScratchDrag(t);
    break;
  case sDD_STOP:
    ScratchDragStop();
    break;
  }
}

void WinTimeline::DragLoop(const sWindowDrag &dd)
{
  sRect r = Client;
  r.Extend(-1);
  sInt t = sClamp(sMulDiv(dd.MouseX-r.x0,End-Start,r.SizeX())+Start,Start,End);

  sInt ts,te;
  switch(dd.Mode)
  {
  case sDD_START:
    DragLoop0 = (t+0x8000)/0x10000*0x10000;
  case sDD_DRAG:
    DragLoop1 = (t+0x8000)/0x10000*0x10000;
  case sDD_STOP:
    if(DragLoop0!=DragLoop1)
    {
      ts = sMin(DragLoop0,DragLoop1);
      te = sMax(DragLoop0,DragLoop1);
    }
    else
    {
      ts = 0;
      te = 0;
    }
    if(ts!=LoopStart || te!=LoopEnd)
    {
      LoopStart = ts;
      LoopEnd = te;
      Update();
      ChangeMsg.Post();
    }
    break;
  }
}

void WinTimeline::CmdToogleLoop()
{
  LoopEnable = !LoopEnable;
}

/****************************************************************************/
/****************************************************************************/

WinCustom::WinCustom()
{
  Child = 0;
  AddBorder(new sFocusBorder);
  Flags |= sWF_CLIENTCLIPPING;
}

WinCustom::~WinCustom()
{
}

void WinCustom::Tag()
{
  sWindow::Tag();
  Child->Need();
}

void WinCustom::Set(wCustomEditor *w)
{
  Child = w;
  Update();
}

/****************************************************************************/

void WinCustom::OnCalcSize()
{
  if(Child)
  {
    Child->OnCalcSize(ReqSizeX,ReqSizeY);
  }
  else
  {
    ReqSizeX = 0;
    ReqSizeY = 0;
  }
}

void WinCustom::OnLayout()
{
  if(Child)
    Child->OnLayout(Client);
}

void WinCustom::OnPaint2D()
{
  if(Child)
    Child->OnPaint2D(Client);
  else
    sGui->PropFont->Print(sF2P_OPAQUE,Client,L"custom window (currently unused)");
}

sBool WinCustom::OnKey(sU32 key)
{
  if(Child)
    return Child->OnKey(key);
  else
    return 0;
}

void WinCustom::OnDrag(const sWindowDrag &dd)
{
  if(Child)
    Child->OnDrag(dd,Client);
}

void WinCustom::ChangeOp()
{
  if(Child) 
    Child->OnChangeOp();
}

void WinCustom::OnTime(sInt time)
{
  if(Child) 
    Child->OnTime(time);
}

/****************************************************************************/
/****************************************************************************/

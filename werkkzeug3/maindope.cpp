// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#include "_start.hpp"
#include "_gui.hpp"
#include "_viruz2.hpp"
#include "kdoc.hpp"
#include "kkriegergame.hpp"
#include "genoverlay.hpp"
#include "genscene.hpp"
#include "geneffect.hpp"
#include "engine.hpp"
#include "wintool.hpp"
#include "engine.hpp"

/****************************************************************************/

sMAKEZONE(ExecTree    ,"ExecTree "  ,0xff40b040);

sGuiManager *sGui = 0;
sGuiPainter *sPainter = 0;
KDoc *Document = 0;
KEnvironment *Environment = 0;
KKriegerGame *Game = 0;
sU8 *DocumentData = 0, *DocumentDataPtr;
sF32 GlobalFps;
CV2MPlayer *Sound = 0;

static sLogWindow *LogWin = 0;
static class DopeApp *App = 0;
static sBool GameRun = sFALSE;
static sInt WaitRender = 0;
static sPerfInfo Perf;
static sU32 UsageMask;

/****************************************************************************/

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

static void RenderSoundEffects(KDoc *doc,sU8 *data)
{
  const sInt maxsamples = 0x10000;
  static sF32 smpf[maxsamples*2];
  static sS16 *smps = (sS16 *) smpf;
  sU32 *data32,size;
  sU8 *v2m;
  sInt i,j,count,start,len,fade;
  sF32 amp,ampx;
  VFXEntry *ent;

  sVERIFY(Sound);

  data32 = (sU32 *) data;
  sVERIFY(data32[0] == sMAKE4('V','F','X','0'));
  size = data32[1];
  v2m = (sU8 *) &data32[2];
  data32 = (sU32 *) (((sU8 *)data32)+size+12);
  count = *data32++;
  ent = (VFXEntry *) data32;

  if(Sound->Open(v2m))
  {
    for(i=0;i<count;i++)
    {
      start = ent->StartPos;
      if(ent->LoopSize)
        start = ent->EndPos - ent->LoopSize;
      len = sMin<sInt>(ent->EndPos - start,maxsamples);
      ampx = sFPow(10.0f,(ent->Gain / 5.0f) / 20.0f);

      Sound->Play(ent->StartPos);
      Sound->Render(smpf,len);
      fade = 1000;
      if(ent->MajorId & 1)
        fade = 44100;

      for(j=0;j<len;j++)
      {
        amp = ampx;
        if(j<fade)
          amp = amp*j/fade;
        if(j>len-fade)
          amp = amp*(len-j)/fade;

        smps[j*2+0] = sRange<sInt>(smpf[j*2+0]*amp*0x7fff,0x7fff,-0x7fff);
        smps[j*2+1] = sRange<sInt>(smpf[j*2+1]*amp*0x7fff,0x7fff,-0x7fff);
      }

      sSystem->SampleAdd(smps,len,4,ent->MinorId,!(ent->MajorId & 1));
      sSystem->Progress(doc->Ops.Count+i,doc->Ops.Count+count);
      ent++;
    }

    Sound->Close();
  }
}

/****************************************************************************/

static void SoundHandler(sS16 *stream,sInt left,void *user)
{
  static sF32 buffer[4096*2];
  static sInt timer;
  sF32 *fp;
  sInt count;
  sInt i;

  timer += left;
  if(Sound)
  {
    while(left>0)
    {
      count = sMin<sInt>(4096,left);
      Sound->Render(buffer,count);
      fp = buffer;
      for(i=0;i<count*2;i++)
        *stream++ = sRange<sInt>(*fp++*0x7fff,0x7fff,-0x7fff);
      left-=count;
    }
  }
  if(timer>=(2*60+25)*44100)
  {
    Sound->Open(Document->SongData);
    Sound->Play(0);
    timer = 0;
  }
}

/****************************************************************************/

class DopeLoader : public sGuiWindow
{
  sRect ProgressRect;
  sInt ProgressFactor;

public:
  DopeLoader();
  void Tag();
  void OnLayout();
  void OnPaint();
  void SetProgress(sInt current,sInt max);
};

DopeLoader::DopeLoader()
{
  Flags |= sGWF_LAYOUT;
  AddChild(LogWin);
}

void DopeLoader::Tag()
{
  sGuiWindow::Tag();
}

void DopeLoader::OnLayout()
{
  sInt height = sPainter->GetHeight(sGui->PropFont);
 
  ProgressRect.Init(Client.x0 + 2, Client.y0 + 6 + height, Client.x1 - 2, Client.y0 + 6 + height + sSCREENY / 40);
  LogWin->Position.Init(ProgressRect.x0,ProgressRect.y1+4,ProgressRect.x1,Client.y1-2);
}

void DopeLoader::OnPaint()
{
  sChar buffer[512];

  sPainter->Paint(sGui->FlatMat,Client,sGui->Palette[sGC_BUTTON]);

  sSPrintF(buffer,sizeof(buffer),"%s loading",sAPPVER);
  sPainter->Print(sGui->PropFont,Client.x0+2,Client.y0+2,buffer,sGui->Palette[sGC_TEXT]);
  sRect pr = ProgressRect;
  sGui->Bevel(pr, sTRUE);
  pr.x1 = pr.x0 + sMulShift(pr.x1 - pr.x0, ProgressFactor);
  sPainter->Paint(sGui->FlatMat,pr,sGui->Palette[sGC_SELBACK]);
}

void DopeLoader::SetProgress(sInt current,sInt max)
{
  ProgressFactor = max ? 65536 * current / max : 65536;
}

/****************************************************************************/

class DopeMainWin : public sGuiWindow
{
  sGridFrame *Grid;
  sControl *Mode;
  sControl *FPS;
  sControl *RenderStats;

  sChar Status1[128];
  sChar Status2[128];

public:
  DopeMainWin();

  void Tag();
  void OnLayout();
  void OnPaint();
};

DopeMainWin::DopeMainWin()
{
  sControl *con;
  sGuiWindow *win;
  sInt gridystep;

  Flags |= sGWF_LAYOUT;
  Grid = new sGridFrame;
  AddChild(Grid);
  gridystep = sPainter->GetHeight(sGui->PropFont) + 8;
  sInt maxy = (sSCREENY / 3) / gridystep;

  if((maxy + 1) * gridystep - 8 > sSCREENY / 3)
    maxy--;

  Grid->SetGrid(51,99,sSCREENX*20/1024,gridystep);

  Status1[0] = Status2[0] = 0;

  // status
  Mode = new sControl;
  Mode->Label("");
  Mode->Style = sCS_LEFT|sCS_NOBORDER;
  Grid->Add(Mode,0,13,0);

  FPS = new sControl;
  FPS->Label(Status1);
  FPS->Style = sCS_LEFT|sCS_NOBORDER;
  Grid->Add(FPS,13,25,0);

  RenderStats = new sControl;
  RenderStats->Label(Status2);
  RenderStats->Style = sCS_LEFT|sCS_NOBORDER;
  Grid->Add(RenderStats,13,25,1);

  // ----
  Grid->AddLabel("Render mode",0,4,1);

  con = new sControl;
  con->EditChoice(0,&GenOverlayManager->EnableShadows,0,"Shadows|No Shadows|No Lights");
  Grid->Add(con,4,8,1);

  con = new sControl;
  con->EditCycle(0,&GenOverlayManager->EnableIPP,0,"|Postprocess");
  Grid->Add(con,8,12,1);

  // ----
  Grid->AddLabel("Passes",0,4,2);

  con = new sControl;
  con->EditCycle(0,(sInt *) &UsageMask,0,"|Light");
  con->CycleBits = 1;
  con->CycleShift = ENGU_LIGHT;
  Grid->Add(con,4,8,2);

  con = new sControl;
  con->EditCycle(0,(sInt *) &UsageMask,0,"|Texture");
  con->CycleBits = 1;
  con->CycleShift = ENGU_MATERIAL;
  Grid->Add(con,8,12,2);

  con = new sControl;
  con->EditCycle(0,(sInt *) &UsageMask,0,"|Envi");
  con->CycleBits = 1;
  con->CycleShift = ENGU_ENVI;
  Grid->Add(con,12,16,2);

  con = new sControl;
  con->EditCycle(0,(sInt *) &UsageMask,0,"|Other");
  con->CycleBits = 1;
  con->CycleShift = ENGU_OTHER;
  Grid->Add(con,16,20,2);

  /*// ----
  Grid->AddLabel("Optimizations",0,4,3);

  con = new sControl;
  con->EditCycle(0,(sInt *) &GenScenePasses,0,"|Sort materials");
  con->CycleBits = 1;
  con->CycleShift = 16;
  Grid->Add(con,4,8,3);

  if(sSystem->GpuMask & sGPU_TWOSIDESTENCIL)
  {
    con = new sControl;
    con->EditCycle(0,(sInt *) &GenScenePasses,0,"|2-sided stencil");
    con->CycleBits = 1;
    con->CycleShift = 17;
    Grid->Add(con,8,12,3);
  }*/

  // ----
  Grid->AddLabel("Tweaks",0,4,4);

  con = new sControl;
  con->EditChoice(0,&GenOverlayManager->ForceResolution,0,"Large res|Medium res|Small res|Default res");
  Grid->Add(con,4,8,4);

  con = new sControl;
  con->EditCycle(0,&WaitRender,0,"|Wait for render");
  Grid->Add(con,8,12,4);

  // ---- log
  LogWin->LayoutInfo.Init(0,5,24,maxy);
  Grid->AddChild(LogWin);

  // ---- right side
  win = new WinPerfMon;
  win->AddBorderHead(new sThinBorder);
  win->LayoutInfo.Init(25,0,51,maxy);
  Grid->AddChild(win);
}

void DopeMainWin::Tag()
{
  sGuiWindow::Tag();
}

void DopeMainWin::OnLayout()
{
  Grid->Position = Position;
  Grid->Position.y0 += 4;
}

void DopeMainWin::OnPaint()
{
  sPainter->Paint(sGui->FlatMat,Client,sGui->Palette[sGC_BUTTON]);

  Mode->Label(GameRun ? "In game mode. Press Scroll Lock to enter GUI mode."
    : "In GUI mode. Press Scroll Lock to enter game mode.");
  Mode->Style |= sCS_LEFT;

  sSPrintF(Status1,sizeof(Status1),"%07.3f fps / %06.2f ms",GlobalFps,1000.0f / GlobalFps);
  FPS->Label(Status1);
  FPS->Style |= sCS_LEFT;

  sSPrintF(Status2,sizeof(Status2),"%6d tri %6d vert %3d batches",Perf.Triangle,Perf.Vertex,Perf.Batches);
  RenderStats->Label(Status2);
  RenderStats->Style |= sCS_LEFT;
}

/****************************************************************************/

class DopeApp : public sDummyFrame
{
  DopeMainWin *MainWin;

public:
  DopeApp();
  ~DopeApp();
  void Tag();
  void OnPaint();
  
  void LoadingComplete();
  sSwitchFrame *Switch;
};

static DopeLoader *LoaderWin = 0;

/****************************************************************************/

DopeApp::DopeApp()
{
  LogWin = new sLogWindow;
  LogWin->AddBorderHead(new sThinBorder);

  LoaderWin = new DopeLoader;

  Switch = new sSwitchFrame;
  Switch->AddChild(LoaderWin);

  AddChild(Switch);
}

DopeApp::~DopeApp()
{
  LogWin = 0;
}

void DopeApp::Tag()
{
  sGuiWindow::Tag();
}

void DopeApp::OnPaint()
{
}

void DopeApp::LoadingComplete()
{
  LoaderWin->RemChild(LogWin);
  MainWin = new DopeMainWin();

  Switch->AddChild(MainWin);
  Switch->Set(1);
}

/****************************************************************************/

static void InitGUI()
{
  // create manager + painter
  sPainter = new sGuiPainter;
  sPainter->Init();
  sBroker->AddRoot(sPainter);
  sGui = new sGuiManager;
  sBroker->AddRoot(sGui);

  // create root window
  sScreenInfo si;
  sVERIFY(sSystem->GetScreenInfo(0,si));
  sGui->SetRoot(new sOverlappedFrame,0);
  sGui->SetStyle(sGui->GetStyle() | sGS_SMALLFONTS);

  // launch application
  App = new DopeApp;
  sInt starty = sSCREENY*2/3;

  App->Position.Init(0,0,sSCREENX,sSCREENY-starty);
  sGui->AddApp(App);
  App->Position.Init(0,starty,sSCREENX,sSCREENY);
}

/****************************************************************************/

void DopeProgress(sInt done,sInt max)
{
  LoaderWin->SetProgress(done,max);
  sGui->OnFrame();
  sGui->OnPaint();
}

/****************************************************************************/

extern sInt VertexMem;

sBool sAppHandler(sInt code,sU32 value)
{
  sViewport vp;
  static sInt FirstTime,LastTime;
  static sF32 oldfps;
  sInt ThisTime,beat,max;
  KOp *root;
  sInt mode;

  switch(code)
  {
  case sAPPCODE_CONFIG:
    sSetConfig(sSF_DIRECT3D,sSCREENX,sSCREENY);
    break;

  case sAPPCODE_INIT:
    // global init
    sInitPerlin();
    GenOverlayInit();

    Engine = new Engine_;

    InitGUI();
    sSystem->ContinuousUpdate(sFALSE);

    // load game file
    DocumentData = sSystem->LoadFile(sSystem->GetCmdLine());
    if(!DocumentData)
      (new sDialogWindow)->InitOk(0, sAPPVER,
      "Please specify a valid file name on the command line.", 1);
    else
    {
      // kkrieger runtime environment
      Document = new KDoc;
      DocumentDataPtr = DocumentData;
      Document->Init(DocumentDataPtr);
      Environment = new KEnvironment;
      Game = new KKriegerGame;
      Game->Init();
      
      Engine = new Engine_;

      //GenBitmapTextureSizeOffset = -1;
      sREGZONE(ExecTree);

      // precalculation
      sInt start = sSystem->GetTime();

      Environment->Splines = &Document->Splines.Array;
      Environment->SplineCount = Document->Splines.Count;
      Environment->Game = Game;
      Environment->InitView();
      Environment->InitFrame(0,0);
      sDPrintF("calculating %d ops...\n", Document->Ops.Count);
      Document->Precalc(Environment);
      Environment->ExitFrame();

      sInt pretime = sSystem->GetTime() - start;
      sDPrintF("took %d.%03d seconds\n",pretime/1000,pretime%1000);
      
      // sound effects
      sDPrintF("sound effects...\n");
#if 0
      if(Document->SongData)
      {
        Sound = new CV2MPlayer;

        if(Document->SampleData)
          RenderSoundEffects(Document,Document->SampleData);

        /*Sound->Open(Document->SongData);
        Sound->Play(0);
        sSystem->SetSoundHandler(SoundHandler,64);*/
      }
#endif
      sSystem->Progress(1,1);

      // collision
      sDPrintF("collision...\n");
      sSystem->Progress(1,1);

      Game->ResetRoot(Environment,Document->RootOps[Document->CurrentRoot],1);
      sDPrintF("done.\n");

      FirstTime = sSystem->GetTime();
      LastTime = 0;
      oldfps = 0.0f;

      WaitRender = 0;

      UsageMask = ~0U;

      App->LoadingComplete();
      sGui->OnFrame(); // to layout the windows

      sDPrintF("everything done, memory usage %d bytes (%d MB)\n",
        sSystem->MemoryUsed(),sSystem->MemoryUsed()>>20);
      sDPrintF("%d distinct material setups\n",sSystem->MtrlCount());
      sDPrintF("%d bytes (%d MB) in geo buffers\n",
        sSystem->BufferMemAlloc,sSystem->BufferMemAlloc>>20);
      sDPrintF("%d bytes (%d MB) in textures\n",
        sSystem->TexMemAlloc,sSystem->TexMemAlloc>>20);

      //Game->Switches[KGS_GAME] = KGS_GAME_RUN;

      GameRun = sTRUE;
    }
    break;

  case sAPPCODE_EXIT:
    if(Sound)
    {
      CV2MPlayer *old = Sound;
      Sound = 0;
      delete old;
    }

    delete[] DocumentData;

    if(Environment)
      delete Environment;

    FreeParticles();

    if(Game)
    {
      Game->Exit();
      delete Game;
    }

    if(Document)
    {
      Document->Exit();
      delete Document;
    }

    if(Engine)
      delete Engine;

    GenOverlayExit();
    sBroker->RemRoot(sPainter);
    sBroker->RemRoot(sGui);
    sGui = 0;
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

    case sKEY_SCROLL:
      if(Game)
        GameRun = !GameRun;
      break;

    default:
      if(!GameRun || !Game->OnKey(value))
        sGui->OnKey(value);
      break;
    }
    break;

  case sAPPCODE_PAINT:
    // paint gui
    sSystem->SetResponse(WaitRender);
    sGui->OnFrame();
    sGui->OnPaint();

    if(Document)
    {
      ThisTime = sSystem->GetTime() - FirstTime;
      beat = sMulDiv(ThisTime,118*0x10000,60000);

      oldfps += (ThisTime - LastTime - oldfps) * 0.1f;
      GlobalFps = 1000.0f / oldfps;

      // game tick
      if(GameRun)
      {
        max = ThisTime/10 - LastTime/10;
        if(max>10) max=10;
        sFloatFix();
        Game->OnTick(Environment,max);

        sSystem->SetWinMouse(sSCREENX/2,sSCREENY/2);
      }

      sSystem->Sample3DCommit();
      LastTime = ThisTime;

      sFloatFix();

      // root-switching logic and outermost stuff
      Engine->SetUsageMask(UsageMask);

      vp.Init();
      vp.Window.Init(0,0,sSystem->ConfigX,sSystem->ConfigY*2/3);
      GenOverlayManager->SetMasterViewport(vp);

      mode = Game->GetNewRoot();
      if(mode!=Document->CurrentRoot)
      {
        App->Switch->Set(0);
        Document->CurrentRoot = mode;
        Environment->InitView();
        Environment->InitFrame(0,0);
        Document->Precalc(Environment);
        Environment->ExitFrame();
        Game->ResetRoot(Environment,Document->RootOps[Document->CurrentRoot],0);
        App->Switch->Set(1);

        sDPrintF("%d distinct material setups\n",sSystem->MtrlCount());
        sDPrintF("%d bytes (%d MB) in geo buffers\n",
          sSystem->BufferMemAlloc,sSystem->BufferMemAlloc>>20);
        sDPrintF("%d bytes (%d MB) in textures\n",
          sSystem->TexMemAlloc,sSystem->TexMemAlloc>>20);
      }

      // event processing

      Environment->InitFrame(beat,ThisTime);
      Document->AddEvents(Environment);
      Game->AddEvents(Environment);
      Game->FlushPhysic();

      // game painting

      Environment->GameCam.Init();
      Game->GetCamera(Environment->GameCam);
      Environment->GameCam.ZoomY = 1.0f*1024/512;

      sSystem->GetPerf(Perf,sPIM_BEGIN);

      root = Document->RootOps[Document->CurrentRoot];
      if(root->Cache->ClassId==KC_DEMO)
      {
        GenOverlayManager->Reset();
        GenOverlayManager->RealPaint = sTRUE;
        GenOverlayManager->Game = Game;

        sFloatFix();
        root->Exec(Environment);

        GenOverlayManager->RealPaint = sFALSE;
        GenOverlayManager->Reset();
      }

      sSystem->GetPerf(Perf,sPIM_END);

      // other per-frame processing
      sFloatFix();
      Environment->ExitFrame();
      Environment->Mem.Flush();
    }
    break;

  case sAPPCODE_TICK:
    sGui->OnTick(value);
    break;

  case sAPPCODE_DEBUGPRINT:
    if(LogWin)
      LogWin->PrintLine((sChar *) value);
    return sFALSE;

  case sAPPCODE_CMD:
    if(value == 1)
      sSystem->Exit();
    break;

  default:
    return sFALSE;
  }

  return sTRUE;
}

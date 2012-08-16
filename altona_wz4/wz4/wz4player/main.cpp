/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "base/types.hpp"
#include "wz4lib/doc.hpp"
#include "wz4frlib/packfile.hpp"
#include "wz4frlib/packfilegen.hpp"
#include "wz4lib/version.hpp"
#include "util/painter.hpp"
#include "util/taskscheduler.hpp"
#include "extra/blobheap.hpp"
#include "extra/freecam.hpp"

#include "selector_win.hpp"
#include "vorbisplayer.hpp"

/****************************************************************************/

static sDemoPackFile *PackFile=0;

#define PrepareFactor 16     // every 16 beats count as (Preparefactor) ops for the progressbar

/****************************************************************************/
/****************************************************************************/

void RegisterWZ4Classes()
{
  for(sInt i=0;i<2;i++)
  {
    sREGOPS(basic,0);
    sREGOPS(poc,1);
//    sREGOPS(minmesh,1);           // obsolete wz3
//    sREGOPS(genspline,1);         // obsolete wz3

    sREGOPS(chaos_font,0);        // should go away soon (detuned)
    sREGOPS(wz3_bitmap,0);
    sREGOPS(wz4_anim,0);
//    sREGOPS(wz4_bitmap,1);        // bitmap system based on new image libarary, never finished
//    sREGOPS(wz4_cubemap,0);       // bitmap system based on new image libarary, never finished
    sREGOPS(wz4_mtrl,1);
    sREGOPS(chaosmesh,1);         // should go away soon (detuned)
//    sREGOPS(wz4_demo,1);          // old demo system, never worked
    sREGOPS(wz4_demo2,0);
    sREGOPS(wz4_mesh,0);
    sREGOPS(chaosfx,0);
    sREGOPS(easter,0);
    sREGOPS(wz4_bsp,0);
    sREGOPS(wz4_mtrl2,0);
//    sREGOPS(wz4_rovemtrl,0);      // was never finished
    sREGOPS(tron,1);
    sREGOPS(wz4_ipp,0);
    sREGOPS(wz4_audio,0);         // audio to image -> not usefull (yet)
    //sREGOPS(wz4_ssao,0);
    sREGOPS(fxparticle,0);
    sREGOPS(wz4_modmtrl,0);
    sREGOPS(wz4_modmtrlmod,0);

//    sREGOPS(fr033,0);   // bp invitation
    sREGOPS(fr062,0);   // the cube
    sREGOPS(fr063_chaos,0);   // chaos+tron
    sREGOPS(fr063_tron,0);   // chaos+tron
    sREGOPS(fr063_mandelbulb,0);   // chaos+tron
    sREGOPS(fr063_sph,0);   // chaos+tron
//    sREGOPS(fr070,0);

    sREGOPS(adf,0);     
    sREGOPS(pdf,0);

  }

  Doc->FindType(L"Scene")->Secondary = 1;
  Doc->FindType(L"GenBitmap")->Order = 2;
  Doc->FindType(L"Wz4Mesh")->Order = 3;
  Doc->FindType(L"Wz4Render")->Order = 4;
  Doc->FindType(L"Wz4Mtrl")->Order = 5;

  wClass *cl = Doc->FindClass(L"MakeTexture2",L"Texture2D");
  sVERIFY(cl);
  Doc->Classes.RemOrder(cl);
  Doc->Classes.AddHead(cl);   // order preserving!

}

/****************************************************************************/
/****************************************************************************/

static sString<64> BenchmarkString;

static void BenchmarkInit() {}

static void BenchmarkExit()
{
  if (!BenchmarkString.IsEmpty())
    sSystemMessageDialog(BenchmarkString,sSMF_OK);
}

sADDSUBSYSTEM(benchmark,0x7f,BenchmarkInit,BenchmarkExit);

/****************************************************************************/
/****************************************************************************/

//sINITMEM(sIMF_NOLEAKTRACK,0);
//sINITMEM(sIMF_NORTL,1200*1024*1024);
sINITMEM(0,0);

static sInt ProgressProxyExtra = 0;
static sInt ProgressProxyDone = 0;
static sInt SkipSampleCount = 0;

void ProgressProxy(sInt done,sInt max);


class MyApp : public sApp
{
public:

  sString<1024> WZ4Name;

  bSelectorResult Selection;

  sInt PreFrames;
  sInt Loaded;
  sString<256> ErrString;

  wOp *RootOp;
  wObject *RootObj;

  wOp *LoaderOp;
  wObject *LoaderObj;

  sBool HasMusic;
  bMusicPlayer MusicPlayer;

  sInt StartFlag;
  sInt StartTime;
  sInt Time;
  sPainter *Painter;
  sBool Fps;
  sBool MTMon;
  sBool Pause;
  sInt PrintHelp;
  sInt FramesRendered;
  sBool DemoDone;
  sBool OverrideCamera;

  sRect ScreenRect;
  wPaintInfo PaintInfo;
  sFreeflightCamera FreeCam;

  MyApp(bSelectorResult &sel)
  {
    Loaded=sFALSE;
    StartTime=0;
    Time=0;
    PreFrames=2;
    StartFlag=0;
    RootOp=0;
    RootObj=0;
    LoaderOp=0;
    LoaderObj=0;
    Fps = 0;
    MTMon = 0;
    Pause = 0;
    PrintHelp = 0;
    Painter = new sPainter();
    FramesRendered = 0;
    DemoDone = sFALSE;
    Selection = sel;
    OverrideCamera = 0;
    FreeCam.SpaceshipMode(1);

    new wDocument;
    sAddRoot(Doc);
    Doc->LowQuality = Selection.LowQuality;
    Doc->IsPlayer=sTRUE;
    sDPrintF(L"quality: %d\n",Doc->LowQuality);
    Doc->EditOptions.BackColor=0xff000000;
  }

  ~MyApp()
  {
    sRelease(RootObj);
    sRelease(LoaderObj);
    sRemRoot(Doc);
    delete Painter;

    if (PackFile)
    {
      sRemFileHandler(PackFile);
      sDelete(PackFile);
    }

    if (DemoDone && Selection.Benchmark)
    {
      BenchmarkString.PrintF(L"Your benchmark result:\n%d points at %dx%d ",FramesRendered,ScreenRect.SizeX(),ScreenRect.SizeY());
      if (Selection.Mode.MultiLevel>=0)
        BenchmarkString.PrintAddF(L"(FSAA level %d)",Selection.Mode.MultiLevel);
      else
        BenchmarkString.PrintAddF(L"(no FSAA)");
    }

  }

  void Load()
  {
    sInt t1=sGetTime();
    const sChar *rootname = L"root";
    ProgressProxyExtra = Doc->DocOptions.Beats/16;
    if(Selection.HiddenPart>=0)
    {
      wDocOptions::HiddenPart *hp = &Doc->DocOptions.HiddenParts[Selection.HiddenPart];
      rootname = hp->Store;
      Doc->DocOptions.MusicFile = hp->Song;
      Doc->DocOptions.BeatsPerSecond = sInt(hp->Bpm*0x10000/60);
      Doc->DocOptions.BeatStart = 0;
      Doc->DocOptions.Beats = hp->LastBeat;
      Doc->DocOptions.Infinite = (hp->Flags & wDODH_Infinite)?1:0;
      ProgressProxyExtra = 0;
      if(hp->Flags & wDODH_FreeFlight)
      {
        OverrideCamera = 1;
        FreeCam.SetPos(sVector31(0,0,-96));
        PrintHelp = 1;
      }
    }

    // load music
    if (!Doc->DocOptions.MusicFile.IsEmpty())
    {
      MusicPlayer.Init(Doc->DocOptions.MusicFile);
      MusicPlayer.SetLoop(Doc->DocOptions.Infinite);
    }

    sInt t2=sGetTime();
    RootOp = Doc->FindStore(rootname);
    if (!RootOp)
    {
      sExit();
      return;
    }

    RootObj=Doc->CalcOp(RootOp);
    sInt t3=sGetTime();

    sLogF(L"player",L"load timing: %dms load, %dms calc -> %ds\n",t2-t1,t3-t2,(t3-t1+500)/1000);

    Loaded=sTRUE;
  };


  void SetPaintInfo(wPaintInfo &pi)
  {
    pi.Window = 0;
    pi.Op = RootOp;

    pi.Lod = Doc->DocOptions.LevelOfDetail;


    pi.Env = 0;
    pi.Grid = 0;
    pi.Wireframe = 0;
    pi.CamOverride = 0;
    pi.Zoom3D = 1.0f;
  }


  void DoPaint(wObject *obj, wPaintInfo &pi)
  {
    sSetTarget(sTargetPara(sST_CLEARALL|sST_NOMSAA,0));

    sInt sx,sy;
    sGetRendertargetSize(sx,sy);
    sF32 scrnaspect=sGetRendertargetAspect();
    sF32 demoaspect=sF32(Doc->DocOptions.ScreenX)/sF32(Doc->DocOptions.ScreenY);
    sF32 arr=scrnaspect/demoaspect;
    sF32 w=0.5f, h=0.5f;
    if (arr>1) w/=arr; else h*=arr;
    ScreenRect.Init(sx*(0.5f-w)+0.5f,sy*(0.5f-h)+0.5f,sx*(0.5f+w)+0.5f,sy*(0.5f+h)+0.5f);

    pi.Client=ScreenRect;
    pi.Rect.Init();

    if(obj)
    {
      sTargetSpec spec(ScreenRect);
      sSetTarget(sTargetPara(sST_CLEARNONE,0,spec));

      sViewport view;
      view.SetTargetCurrent();
      view.Prepare();

      pi.CamOverride = OverrideCamera;
      if(OverrideCamera)
      {
        FreeCam.MakeViewport(view);
        view.ClipNear = 1.0f/128;
        view.ClipFar = 1024;
      }

      pi.View = &view;
      pi.Spec = spec;

      obj->Type->BeforeShow(obj,pi);
      Doc->LastView = *pi.View;
      pi.SetCam = 0;
      Doc->Show(obj,pi);
    }
  }

  void PaintLoaderOp(sInt done, sInt max)
  {
    SetPaintInfo(PaintInfo);

    PaintInfo.TimeMS = 0; // unused so far...
    PaintInfo.TimeBeat = sMulDiv(16*65536-1,done,max); // -1: loader bar ends at beat 16!

    DoPaint(LoaderObj,PaintInfo);
  }

  void OnPaint3D()
  {
    sSetTarget(sTargetPara(sST_CLEARALL,0));

    if (!Loaded)
    {
      // load and charge
      if (!PreFrames--)
      {
        PreFrames=0;

        if (!Doc->Load(WZ4Name))
        {
          sFatal(L"could not load wz4 file %s",WZ4Name);
          return;
        }

        ProgressPaintFunc=0;
        LoaderOp = Doc->FindStore(L"loading");
        if (LoaderOp)
          LoaderObj = Doc->CalcOp(LoaderOp);

        sRender3DEnd();

        ProgressPaintFunc=ProgressProxy;
        if (!LoaderObj) sProgressBegin();

        Load();

        if(RootObj)
        {
          if(Doc->CacheWarmupBeat.GetCount()==0)
          {
            for(sInt i=0;i<Doc->DocOptions.Beats/16;i++)
              Doc->CacheWarmupBeat.AddTail(i*16*0x10000);
          }
          sSortDown(Doc->CacheWarmupBeat);


          sProgressDoCallBeginEnd = 0;

          sViewport view;
          SetPaintInfo(PaintInfo);
          sInt *warmbeat;
          PaintInfo.CacheWarmupAgain = 0;
          if(Selection.HiddenPart>=0)
            Doc->CacheWarmupBeat.Clear();
          Doc->IsCacheWarmup = 1;
          sFORALL(Doc->CacheWarmupBeat,warmbeat)
          {
            do
            {
              sRender3DBegin();

              PaintInfo.TimeBeat = (*warmbeat);
              PaintInfo.TimeMS = Doc->BeatsToMilliseconds(PaintInfo.TimeBeat);
              sDPrintF(L"warmup %08x\n",PaintInfo.TimeBeat);

              PaintInfo.CacheWarmup = 1;
              PaintInfo.CacheWarmupAgain = sMax(0,PaintInfo.CacheWarmupAgain-1);
              DoPaint(RootObj,PaintInfo);

              sInt extraprogress = sMulDiv(_i,ProgressProxyExtra*PrepareFactor,Doc->CacheWarmupBeat.GetCount());
              if (LoaderObj)
                PaintLoaderOp(ProgressProxyDone+extraprogress,ProgressProxyDone+ProgressProxyExtra*PrepareFactor);
              else
                sProgress(ProgressProxyDone+extraprogress,ProgressProxyDone+ProgressProxyExtra*PrepareFactor);
              sRender3DEnd();
            }
            while(PaintInfo.CacheWarmupAgain>0);
          }
          Doc->IsCacheWarmup = 0;
          PaintInfo.TimeBeat = 0;
          PaintInfo.TimeMS = 0;
          sProgressDoCallBeginEnd = 1;
          sSchedMon->FlipFrame();
        }

        if (!LoaderObj) sProgressEnd();

        sRender3DBegin();

        PaintInfo.CacheWarmup = 0;
        StartFlag=1;
      }
    }

    if (Loaded)
    {
      if (StartFlag)
      {
        if (MusicPlayer.IsLoaded())
          MusicPlayer.Play();
        StartFlag=0;
      }
      if (MusicPlayer.IsLoaded())
      {
        Time = sMax(0,sInt(1000*MusicPlayer.GetPosition()));
      }
      else
      {
        if (!StartTime) StartTime=sGetTime();
        Time = sGetTime()-StartTime;
      }

      SetPaintInfo(PaintInfo);

      PaintInfo.TimeMS = Time;
      PaintInfo.TimeBeat = Doc->MilliSecondsToBeats(Time);

      sTextBuffer Log;
      if(PrintHelp)
      {
        Log.PrintF(L"Werkkzeug 4 Player Help:\n");
        Log.PrintF(L"F1       - Help\n");
        Log.PrintF(L"f        - fps and debug log\n");
        Log.PrintF(L"m        - multithreading debug\n");
        Log.PrintF(L"j        - jump forward by a second\n");
        if(Selection.HiddenPart>=0)
          Log.PrintF(L"o        - override camera\n");
        Log.PrintF(L"SPACE    - pause demo\n");
        Log.PrintF(L"\n");
        if(OverrideCamera)
        {
          Log.PrintF(L"Free Camera:\n");
          Log.PrintF(L"a d w s  - move\n");
          Log.PrintF(L"LMB RMB  - rotate\n");
          Log.PrintF(L"r        - reset\n");
          Log.PrintF(L"wheel    - camera speed (%d)\n",FreeCam.GetSpeed());
          Log.PrintF(L"\n");
        }
      }
      static sTiming time;

      time.OnFrame(sGetTime());
      if(Fps)
      {
        PaintInfo.ViewLog = &Log;

        sGraphicsStats stat;
        sGetGraphicsStats(stat);


        sString<128> b;

        sF32 ms = time.GetAverageDelta();
        Log.PrintF(L"'%f' ms %f fps\n",ms,1000/ms);
        Log.PrintF(L"'%d' batches '%k' verts %k ind %k prim %k splitters\n",
          stat.Batches,stat.Vertices,stat.Indices,stat.Primitives,stat.Splitter);
        sInt sec = PaintInfo.TimeMS/1000;
        Log.PrintF(L"Beat %d  Time %dm%2ds  Frames %d\n",PaintInfo.TimeBeat/0x10000,sec/60,sec%60,FramesRendered);
      }
      else
      {
        PaintInfo.ViewLog = 0;
      }
      PaintInfo.CamOverride = 0;
      FreeCam.OnFrame(time.GetSlices(),time.GetJitter());


      // end of demo?
      if ((PaintInfo.TimeBeat>>16)>=Doc->DocOptions.Beats)
      {
        if (Selection.Loop)
        {
          StartFlag=1;
          if (MusicPlayer.IsLoaded())
          {
            MusicPlayer.Seek(0);
            MusicPlayer.Play();
          }
        }
        else
        {
          DemoDone=sTRUE;
          sExit();
        }
      }

      DoPaint(RootObj,PaintInfo);
      FramesRendered++;



      sEnableGraphicsStats(0);
      sSetTarget(sTargetPara(0,0,&PaintInfo.Client));
      App->Painter->SetTarget(PaintInfo.Client);
      App->Painter->Begin();
      App->Painter->SetPrint(0,0xff000000,1,0xff000000);
      App->Painter->Print(10,11,Log.Get());
      App->Painter->Print(11,10,Log.Get());
      App->Painter->Print(12,11,Log.Get());
      App->Painter->Print(11,12,Log.Get());
      App->Painter->SetPrint(0,0xffc0c0c0,1,0xffffffff);
      App->Painter->Print(11,11,Log.Get());
      App->Painter->End();
      sEnableGraphicsStats(1);
      sSchedMon->FlipFrame();
      if(MTMon)
      {
        sTargetSpec ts;
        sSchedMon->Paint(ts);
      }
    }
  }

  void OnInput(const sInput2Event &ie)
  {
    FreeCam.OnInput(ie);
    sU32 key = ie.Key;
    key &= ~sKEYQ_CAPS;
    if(key & sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
    if(key & sKEYQ_CTRL ) key |= sKEYQ_CTRL;
    switch(key)
    {
    case sKEY_ESCAPE:
    case sKEY_ESCAPE|sKEYQ_SHIFT:
    case sKEY_F4|sKEYQ_ALT:
      sExit();
      break;

    case 'f':
    case 'F'|sKEYQ_SHIFT:
      Fps = !Fps;
      break;

    case 'm':
    case 'M'|sKEYQ_SHIFT:
      if(sSchedMon==0)
      {
        sSchedMon = new sStsPerfMon();
      }
      MTMon = !MTMon;
      break;

    case sKEY_F1:
      PrintHelp = !PrintHelp;
      break;

    case 'j': case 'j'|sKEYQ_REPEAT:
      MusicPlayer.Skip(Doc->DocOptions.SampleRate);
      break;

    case 'o':
      if(Selection.HiddenPart>=0)
      {
        OverrideCamera = !OverrideCamera;
        if(OverrideCamera)
        {
          FreeCam.Set(PaintInfo.SetCamMatrix,PaintInfo.SetCamZoom);
        }
      }
      break;

    case ' ':
      Pause = !Pause;
      if(Pause)
        MusicPlayer.Pause();
      else
        MusicPlayer.Play();
      break;
    }
  }
} *App=0;


// for fr-062: check for vertex textures
static void CheckCapsCallback(void *user)
{
  sGraphicsCaps caps;
  sGetGraphicsCaps(caps);
  if(!(caps.VertexTex2D & (1<<sTEX_R32F)))
    sSystemMessageDialog(L"your graphic card suxx.\nwell, we try to start the demo anyway.\nwill not look as intended.\nslightly.",sSMF_OK);
}

extern void sCollector(sBool exit=sFALSE);

static void MakePackfile(const sChar *wz4name, const sChar *packname)
{
  sArray<sPackFileCreateEntry> files;
  files.HintSize(10000);

  new wDocument;
  sAddRoot(Doc);
  Doc->IsPlayer=sTRUE;


  sAddFileLogger(files);
  sBool ok=sFALSE;

  ProgressPaintFunc=sProgress;
  sProgressBegin();

  // load and calc wz4 file to log all file reads
  if (Doc->Load(wz4name))
  {
    sArray<const sChar *> ops2store;
    ops2store.AddTail(L"root");
    ops2store.AddTail(L"loading");
    wDocOptions::HiddenPart *hp;
    sFORALL(Doc->DocOptions.HiddenParts,hp)
      ops2store.AddTail(hp->Store);

    const sChar *opname;
    sFORALL(ops2store,opname)
    {
      wOp *rootop=Doc->FindStore(opname);
      if (rootop)
      {
        wObject *rootobj=Doc->CalcOp(rootop);
        sRelease(rootobj);
      }
    }

    // add music file if applicable
    if (!Doc->DocOptions.MusicFile.IsEmpty() && sCheckFile(Doc->DocOptions.MusicFile))
      files.AddTail(sPackFileCreateEntry(Doc->DocOptions.MusicFile,sFALSE));
    sFORALL(Doc->DocOptions.HiddenParts,hp)
      if(!hp->Song.IsEmpty() && sCheckFile(hp->Song))
        files.AddTail(sPackFileCreateEntry(hp->Song,sFALSE));

    // add text file with name of wz4 file in it
    sString<sMAXPATH> txtname;
    sGetTempDir(txtname);
    txtname.PrintAddF(L"wz4file.txt");
    sSaveTextAnsi(txtname,wz4name);

    sPoolString n2=txtname;
    files.AddTail(sPackFileCreateEntry(n2,0));

    ok=sTRUE;
  }

  Doc->DocOptions.TextureQuality = 1;

  sRemFileLogger();
  sRemRoot(Doc);
  sCollect();
  sCollector(sTRUE);

  sProgressEnd();

  if (!ok) 
    return;

  // now make the packfile
  sCreateDemoPackFile(packname, files, sTRUE);

  return;
}

static sBool LoadOptions(const sChar *filename, wDocOptions &options)
{
  sFile *f=sCreateFile(filename);
  if (!f) return sFALSE;

  sReader r;
  r.Begin(f);

  wDocument::SerializeOptions(r,options);
  sBool ret=r.End();

  delete f;
  return ret;
}


/****************************************************************************/
/****************************************************************************/

void ProgressProxy(sInt done,sInt max)
{

  ProgressProxyDone = max;
  if (App->LoaderObj)
  {
    static const sInt MAX_LOADER_FPS=20;
    static sInt lasttime=0;
    
    sInt time=sGetTime();
    if (lasttime==0 || ((time-lasttime)>=(1000/MAX_LOADER_FPS)))
    {
      lasttime=time;
      sRender3DBegin();
      sSetTarget(sTargetPara(sST_CLEARALL,0));
      App->PaintLoaderOp(done,max+ProgressProxyExtra*PrepareFactor);
      sRender3DEnd();
    }
  }
  else
    sProgress(done,max+ProgressProxyExtra*PrepareFactor);
}


static void sExitSts()
{
  sDelete(sSched);
}


void sMain()
{
  bSelectorResult Selection;
  sClear(Selection);
  sAddSubsystem(L"StealingTaskScheduler (wz4player style)",0x80,0,sExitSts);

  sArray<sDirEntry> dirlist;
  sString<1024> wintitle;
  wintitle.PrintF(L"werkkzeug4 player V%d.%d",WZ4_VERSION,WZ4_REVISION);

  sSetWindowName(wintitle);
  const sChar *makepack=sGetShellString(L"p",L"-pack");
  sAddGlobalBlobHeap();

  // find packfile and open it
  if (!makepack)
  {
    sString<1024> pakname;
    sString<1024> programname;
    const sChar *cmd=sGetCommandLine();
    while (sIsSpace(*cmd) || *cmd=='"') cmd++;
    const sChar *pstart=cmd;
    while (*cmd && !(sIsSpace(*cmd) || *cmd=='"')) cmd++;
    sCopyString(programname,pstart,cmd-pstart+1);
    
    sCopyString(sFindFileExtension(programname),L"pak",4);
    if (sCheckFile(programname))
      pakname=programname;
    else
    {
      // use first packfile from current directory
      sLoadDir(dirlist,L".",L"*.pak");
      if (!dirlist.IsEmpty())
        pakname=dirlist[0].Name;       
      dirlist.Clear();
    }

    if (!pakname.IsEmpty())
    {
      PackFile = new sDemoPackFile(pakname);
      sAddFileHandler(PackFile);
    }
  }

  // find fitting wz4 file
  sString<1024> wz4name;

  const sChar *fromcmd=sGetShellParameter(0,0); // command line?
  if (fromcmd && sCheckFile(fromcmd))
    wz4name=fromcmd;
  else
  {
    const sChar *fromtxt=sLoadText(L"wz4file.txt"); // text file with name in it? (needed for packfile)
    if (fromtxt) 
    {
      wz4name=fromtxt;
      sDeleteArray(fromtxt);
    }
    else
    {
      sLoadDir(dirlist,L".",L"*.wz4"); // wz4 file in current directory?
      if (!dirlist.IsEmpty())
        wz4name=dirlist[0].Name;       
      dirlist.Clear();
    }
  }
  if (wz4name.IsEmpty())
  {
    sFatal(L"no .suitable .wz4 file found");
  }
  
  // make a pack file?
  if (makepack)
  {
    sInit(sISF_3D|sISF_2D,640,480);
    sVERIFY(sSched==0);
    sSched = new sStsManager(128*1024,512,0);
    sSchedMon = new sStsPerfMon();

    MakePackfile(wz4name,makepack);
    return;
  }

  // try to load doc options from wz4 file
  wDocOptions opt;
  LoadOptions(wz4name,opt);

  // get demo title from doc options
  sString<256> title=sGetWindowName();
  if (!opt.ProjectName.IsEmpty())
  {
    if (opt.ProjectId>0)
      title.PrintF(L"fr-0%d: %s",opt.ProjectId,opt.ProjectName);
    else if (opt.ProjectId<0)
      title.PrintF(L"fr-minus-0%d: %s",-opt.ProjectId,opt.ProjectName);
    else
      title.PrintF(L"%s",opt.ProjectName);
    sSetWindowName(title);
  }

  // open selector and set screen mode
  sTextBuffer tb;
  sInt flags=sISF_2D|sISF_3D|sISF_CONTINUOUS; // need 2D for text rendering
  if(sGetShellSwitch(L"nodialog"))
  {
    Selection.Mode.ScreenX = 640;
    Selection.Mode.ScreenY = 480;
    Selection.Mode.Aspect = 1.333333;
  }
  else
  {
    bSelectorSetup selsetup;
    selsetup.Title=title;
    selsetup.IconInt=100;
    selsetup.IconURL=L"http://www.farbrausch.com";
    selsetup.Caption=L"farbrausch";
    selsetup.SubCaption.PrintF(L"werkkzeug4 player V%d.%d",WZ4_VERSION,WZ4_REVISION);
    selsetup.DialogFlags = opt.DialogFlags;
    selsetup.DialogScreenX = opt.ScreenX;
    selsetup.DialogScreenY = opt.ScreenY;
    tb.Print(L"original demo");
    wDocOptions::HiddenPart *hp;
    sFORALL(opt.HiddenParts,hp)
      if(hp->Flags & wDODH_Dialog)
        tb.PrintF(L"|%s",hp->Code);
    selsetup.HiddenPartChoices = tb.Get();

    if (opt.SiteId)
    {

      selsetup.Sites[0].IconInt=100;
      selsetup.Sites[0].URL.PrintF(L"http://www.farbrausch.com/prod.py?which=%d",opt.SiteId);

      static const sChar *siteids[]={L"twitter",L"facebook",L"myspace",L"digg",L"stumbleupon"};
      for (sInt i=0; i<sCOUNTOF(siteids); i++)
      {
        selsetup.Sites[i+1].IconInt=200+i;
        selsetup.Sites[i+1].URL.PrintF(L"http://www.farbrausch.com/share.py?which=%d&site=%s",opt.SiteId,siteids[i]);
      }
    }

    if (!bOpenSelector(selsetup,Selection)) return;
  }
  if (Selection.Mode.Flags & sSM_FULLSCREEN) flags|=sISF_FULLSCREEN;

  title.PrintAddF(L"  (player V%d.%d)",WZ4_VERSION,WZ4_REVISION);
  if(sCONFIG_64BIT)
    title.PrintAddF(L" (64Bit)");
  sSetWindowName(title);

//  sCheckCapsHook->Add(CheckCapsCallback);   // for fr-062 vertex texture check

  Selection.Mode.RTZBufferX = 2048;
  Selection.Mode.RTZBufferY = 2048;
  sSetScreenMode(Selection.Mode);
  sInit(flags,Selection.Mode.ScreenX,Selection.Mode.ScreenY);

  sSched = new sStsManager(128*1024,512,Selection.OneCoreForOS ? -1 : 0);

  wDocOptions::HiddenPart *hp;
  sFORALL(opt.HiddenParts,hp)
    if(sGetShellSwitch(hp->Code))
      Selection.HiddenPart = _i;

  // .. and go!
  App=new MyApp(Selection);
  App->WZ4Name = wz4name;
  sSetApp(App);
}

/****************************************************************************/

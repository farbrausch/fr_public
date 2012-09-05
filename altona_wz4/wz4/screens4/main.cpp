/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "base/types.hpp"
#include "util/painter.hpp"
#include "util/taskscheduler.hpp"
#include "extra/blobheap.hpp"
#include "wz4lib/doc.hpp"
#include "wz4lib/basic.hpp"
#include "wz4lib/version.hpp"
#include "wz4frlib/packfile.hpp"
#include "wz4frlib/wz4_demo2.hpp"
#include "wz4frlib/wz4_demo2nodes.hpp"
#include "wz4frlib/screens4fx.hpp"

#include "config.hpp"
#include "vorbisplayer.hpp"
#include "network.hpp"
#include "playlists.hpp"

#include "network/netdebug.hpp"

/****************************************************************************/

static Config *MyConfig;
static sDemoPackFile *PackFile=0;
static sScreenMode ScreenMode;

#define PrepareFactor 16     // every 16 beats count as (Preparefactor) ops for the progressbar

/****************************************************************************/
/****************************************************************************/

void RegisterWZ4Classes()
{
  for(sInt i=0;i<2;i++)
  {
    sREGOPS(basic,0);
    sREGOPS(poc,1);
    sREGOPS(chaos_font,0);        // should go away soon (detuned)
    sREGOPS(wz3_bitmap,0);
    sREGOPS(wz4_anim,0);
    sREGOPS(wz4_mtrl,1);
    sREGOPS(chaosmesh,1);         // should go away soon (detuned)
    sREGOPS(wz4_demo2,0);
    sREGOPS(wz4_mesh,0);
    sREGOPS(chaosfx,0);
    sREGOPS(easter,0);
    sREGOPS(wz4_bsp,0);
    sREGOPS(wz4_mtrl2,0);
    sREGOPS(tron,1);
    sREGOPS(wz4_ipp,0);
    sREGOPS(wz4_audio,0);         // audio to image -> not usefull (yet)
    sREGOPS(fxparticle,0);
    sREGOPS(wz4_modmtrl,0);
    sREGOPS(wz4_modmtrlmod,0);

    sREGOPS(fr062,0);   // the cube
    sREGOPS(fr063_chaos,0);   // chaos+tron
    sREGOPS(fr063_tron,0);   // chaos+tron
    sREGOPS(fr063_mandelbulb,0);   // chaos+tron
    sREGOPS(fr063_sph,0);   // chaos+tron

    sREGOPS(adf,0);     
    sREGOPS(pdf,0);

    sREGOPS(screens4,0);
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

static void SetupScreenMode(Config::Resolution resolution, sBool fullscreen)
{
  sClear(ScreenMode);
  ScreenMode.Aspect = (float)resolution.Width/(float)resolution.Height;
  ScreenMode.Display = -1;
  if (fullscreen)
  {
    ScreenMode.Flags |= sSM_FULLSCREEN;
  }
  ScreenMode.Frequency = resolution.RefreshRate;
  ScreenMode.MultiLevel = -1;
  ScreenMode.OverMultiLevel = -1;
  ScreenMode.OverX = ScreenMode.ScreenX = MyConfig->DefaultResolution.Width;
  ScreenMode.OverY = ScreenMode.ScreenY = MyConfig->DefaultResolution.Height;

  // find max resolution for z buffer
  //ScreenMode.RTZBufferX = 2048;
  //ScreenMode.RTZBufferY = 2048;
  /*
  for (sInt i=0; i<MyConfig->Keys.GetCount(); i++) if (MyConfig->Keys[i].Type == Config::SETRESOLUTION)
  {
    sm.RTZBufferX = sMax(sm.RTZBufferX, MyConfig->Keys[i].ParaRes.Width);
    sm.RTZBufferY = sMax(sm.RTZBufferY, MyConfig->Keys[i].ParaRes.Height);
  }
  */
}

/****************************************************************************/
/****************************************************************************/

template <typename T> class sAutoPtr
{
public:
  sAutoPtr() : ptr(0) {}
  sAutoPtr(T* obj) { ptr = obj; }
  sAutoPtr(const sAutoPtr<T> &p) {sAddRef(ptr=p.ptr); }
  sAutoPtr& operator= (const sAutoPtr &p) { sRelease(ptr); sAddRef(ptr=p.ptr); return *this; }
  ~sAutoPtr() { sRelease(ptr); }
  T* operator -> () const { return ptr; }
  operator T*() const { return ptr; }

private:
  T* ptr;
};

class sRandomMarkov
{
public:
  sRandomMarkov() { Total=0; }

  explicit sRandomMarkov(sInt max)
  {
    Init(max);
  }

  void Init(sInt n)
  {
    Weights.Reset();
    Weights.HintSize(n);
    for (sInt i=0; i<n; i++)
      Weights.AddTail(1);
    Total = n;
    Rnd.Seed(sGetRandomSeed());
  }

  sInt Get()
  {
    // pick random number
    sInt i;
    sInt x = Rnd.Int32()%Total;
    sInt n = Weights.GetCount();
    for (i=0; x>=Weights[i] && i<n; i++) x-=Weights[i];

    // update model
    for (int j=0; j<n; j++) Weights[j]++;
    Total+=n-Weights[i];
    Weights[i]=0;

    return i;  
  }

private:

  sRandomMT Rnd;
  sStaticArray<sInt> Weights;
  sInt Total;
};

/****************************************************************************/
/****************************************************************************/

sINITMEM(0,0);
sINITDEBUGMEM(0);

class MyApp : public sApp
{
public:

  sString<1024> WZ4Name;

  sInt PreFrames;
  sInt Loaded;
  sString<256> ErrString;

  wOp *RootOp;
  sAutoPtr<Wz4Render> RootObj;

  sBool HasMusic;

  sInt StartTime;
  sInt Time;
  sPainter *Painter;
  sBool Fps;
  sBool MTMon;
  sInt FramesRendered;

  sRect ScreenRect;
  wPaintInfo PaintInfo;

  RPCServer *Server;
  WebServer *Web;

  PlaylistMgr PlMgr;

  sRandomMT Rnd;

  struct NamedObject
  {
    sString<64> Name;
    sAutoPtr<Wz4Render> Obj;
  };

  sArray<NamedObject> Transitions;
  sRandomMarkov RndTrans;

  wClass *CallClass;

  struct SlideEntry
  {
    wOp *TexOp;
    sAutoPtr<Texture2D> Tex;

    sAutoPtr<Wz4Render> Pic, Siegmeister;
  } *Entry[2];

  sAutoPtr<Rendertarget2D> CurRT;
  sAutoPtr<Rendertarget2D> NextRT;

  sAutoPtr<Wz4Render> CurShow;
  
  sAutoPtr<Wz4Render> NextRender;
  sAutoPtr<Wz4Render> CurRender;
  sAutoPtr<Wz4Render> Main;

  sAutoPtr<Wz4Render> CurSlide;
  sAutoPtr<Wz4Render> NextSlide;

  sF32 TransTime, TransDurMS, CurRenderTime, NextRenderTime, SlideStartTime;
  sBool Started;

  bMusicPlayer SoundPlayer;

  MyApp()
  {
    Loaded=sFALSE;
    StartTime=0;
    Time=0;
    PreFrames=2;
    RootOp=0;
    RootObj=0;
    Fps = 0;
    MTMon = 0;
    Painter = new sPainter();
    FramesRendered = 0;
    CallClass = 0;
    CurSlide = 0;
    NextSlide = 0;
    TransTime = -1.0f;
    CurRenderTime = NextRenderTime = 0.0f;
    Started = sFALSE;
    SlideStartTime = 0;

    Entry[0] = new SlideEntry;
    Entry[1] = new SlideEntry;

    new wDocument;
    sAddRoot(Doc);
    Doc->EditOptions.BackColor=0xff000000;

    Server = 0;
    Web = 0;
    Rnd.Seed(sGetRandomSeed());
  }

  ~MyApp()
  {

    sDelete(Server);
    sDelete(Web);

    sDelete(Entry[0]);
    sDelete(Entry[1]);

    sRemRoot(Doc);
    delete Painter;

    if (PackFile)
    {
      sRemFileHandler(PackFile);
      sDelete(PackFile);
    }
  
    sDelete(MyConfig);
  }


  wOp *MakeCall(const sChar *name, wOp *input)
  {
    if (!CallClass)
      CallClass = Doc->FindClass(L"Call",L"AnyType");

    wOp *callop = new wOp;
    callop->Class = CallClass;
    sU32 *flags = new sU32[1];
    flags[0] = 1;
    callop->EditData = flags;
    callop->Inputs.HintSize(2);
    callop->Inputs.AddTail(Doc->FindStore(name));
    callop->Inputs.AddTail(input);

    return callop;
  }

  void MakeNextSlide(NewSlideData *ns)
  {
    SlideEntry *e = Entry[0];

    if (ns->ImgData)
    {
      sTexture2D *tex = e->Tex->Texture->CastTex2D();
      tex->ReInit(ns->ImgData->SizeX,ns->ImgData->SizeY,ns->ImgData->Format);
      tex->LoadAllMipmaps(ns->ImgData->Data);
    }

    switch (ns->Type)
    {
    case IMAGE:
      NextSlide = e->Pic;
      break;
    case SIEGMEISTER_BARS:
    case SIEGMEISTER_WINNERS:
      NextSlide = e->Siegmeister;
      RNSiegmeister *node = GetNode<RNSiegmeister>(L"Siegmeister",e->Siegmeister);
      node->DoBlink = (ns->Type == SIEGMEISTER_WINNERS);
      node->Fade = node->DoBlink?1:0;
      node->Alpha=ns->SiegData->BarAlpha;
      node->Color=ns->SiegData->BarColor;
      node->BlinkColor1=ns->SiegData->BarBlinkColor1;
      node->BlinkColor2=ns->SiegData->BarBlinkColor2;
      node->Spread = MyConfig->BarAnimSpread;
      node->Bars.Clear();
      node->Bars.Copy(ns->SiegData->BarPositions);
      break;
    }

    sSwap(Entry[0], Entry[1]);
    NextRenderTime = 0;
  }


  void SetChild(const sAutoPtr<Wz4Render> &node, const sAutoPtr<Wz4Render> &child, sF32 *time)
  {
    sReleaseAll(node->RootNode->Childs);
    if (child)
    {
      child->RootNode->AddRef();
      node->RootNode->Childs.AddTail(child->RootNode);
      ((RNAdd*)node->RootNode)->TimeOverride = time;
    }
    else
    {
      ((RNAdd*)node->RootNode)->TimeOverride = 0;
    }
  }

  void SetRTRecursive(Wz4RenderNode *node, Rendertarget2D *rt, sInt pass)
  {
    if (!sCmpString(node->Op->Class->Name,L"Camera"))
    {
      RNCamera *cam = (RNCamera*)node;
      sRelease(cam->Target);
      cam->Target=rt;
      sAddRef(cam->Target);
      cam->Para.Renderpass = pass;
      return;
    }

    Wz4RenderNode *child;
    sFORALL(node->Childs,child)
      SetRTRecursive(child,rt, pass);
  }

  void SetRT(const sAutoPtr<Wz4Render> node, const sAutoPtr<Rendertarget2D> rt, int pass)
  {
    SetRTRecursive(node->RootNode, rt, pass);
  }

  template<class T> T* GetNodeRecursive(const sChar *classname, Wz4RenderNode *node)
  {
    if (!sCmpString(node->Op->Class->Name,classname))
      return (T*)node;

    Wz4RenderNode *child;
    T *ret;
    sFORALL(node->Childs,child)
    {
      ret = GetNodeRecursive<T>(classname, child);
      if (ret) return ret;
    }

    return 0;
  }

  template<class T> T* GetNode(const sChar *classname, const sAutoPtr<Wz4Render> node)
  {
    if (!node) return 0;
    return GetNodeRecursive<T>(classname, node->RootNode);
  }

  void SetTransition(int i, sF32 duration=1.0f)
  {
    const NamedObject &no = Transitions[i];
    SetChild(Main,no.Obj,&TransTime);
    SetRT(NextSlide, NextRT, 200);
    SetChild(NextRender,NextSlide,&NextRenderTime);

    TransTime = 0.0f;
    TransDurMS = 1000*duration;
    Started = sFALSE;
  }

  void EndTransition()
  {
    CurSlide = NextSlide;
    CurRenderTime = NextRenderTime;
    NextSlide = 0;
    TransTime = -1.0f;
    Started = sFALSE;

    SetRT(CurSlide, CurRT, 100);
    SetChild(CurRender,CurSlide,&CurRenderTime);
    SetChild(NextRender,0,0);
    SetChild(Main,CurShow,0);
  }

  void Load()
  {
    sInt t1=sGetTime();
    const sChar *rootname = L"root";
  
    RootOp = Doc->FindStore(rootname);
    if (!RootOp)
    {
      sExit();
      return;
    }
    RootObj=(Wz4Render*)Doc->CalcOp(RootOp);

    // the big test
    Entry[0]->TexOp = Doc->FindStore(L"Tex1");
    Entry[1]->TexOp = Doc->FindStore(L"Tex2");
    
    CurRT = (Rendertarget2D *)Doc->CalcOp(Doc->FindStore(L"CurRT"));
    NextRT = (Rendertarget2D *)Doc->CalcOp(Doc->FindStore(L"NextRT"));

    CurShow = (Wz4Render*)Doc->CalcOp(Doc->FindStore(L"CurShow"));
    CurRender = (Wz4Render*)Doc->CalcOp(Doc->FindStore(L"CurRender"));
    NextRender = (Wz4Render*)Doc->CalcOp(Doc->FindStore(L"NextRender"));
    Main = (Wz4Render*)Doc->CalcOp(Doc->FindStore(L"Main"));

    // load transitions
    wOp *op;
    sFORALL(Doc->Stores,op)
    {
      if (!sCmpStringILen(op->Name,L"trans_",6))
      {
        NamedObject no;
        no.Name = op->Name+6;
        no.Obj = (Wz4Render*)Doc->CalcOp(op);
        Transitions.AddTail(no);
      }
    }
    RndTrans.Init(Transitions.GetCount());

    // load slide renderers
    for (sInt i=0; i<2; i++)
    {
      SlideEntry *e = Entry[i];
      e->Tex = (Texture2D*)Doc->CalcOp(e->TexOp);
      e->Pic = (Wz4Render*)Doc->CalcOp(MakeCall(L"slide_pic",e->TexOp));
      e->Siegmeister = (Wz4Render*)Doc->CalcOp(MakeCall(L"slide_siegmeister",e->TexOp));
    }

    Server = new RPCServer(PlMgr, MyConfig->Port);
    //Web = new WebServer(PlMgr, MyConfig->HttpPort);

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

      pi.CamOverride = sFALSE;

      pi.View = &view;
      pi.Spec = spec;

      obj->Type->BeforeShow(obj,pi);
      Doc->LastView = *pi.View;
      pi.SetCam = 0;
      Doc->Show(obj,pi);
    }
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

        sRender3DEnd();

        if (!Doc->Load(WZ4Name))
        {
          sFatal(L"could not load wz4 file %s",WZ4Name);
          return;
        }

        ProgressPaintFunc=0;

        Load();

        sBool ret = sRender3DBegin();
        PaintInfo.CacheWarmup = 0;
      }
    }

    if (Loaded)
    {      
      sInt tdelta;
      if (!StartTime) 
      {
          StartTime=sGetTime();
          Time = 0;
          tdelta = 0;
      }
      else
      {
        sInt newtime = sGetTime()-StartTime;
        tdelta = newtime-Time;
        Time = newtime;
      }

      NewSlideData *newslide = PlMgr.OnFrame(tdelta/1000.0f);

      if (newslide)
      {
        if (!newslide->Error)
        {
          if (TransTime>=0)
            EndTransition();

          MakeNextSlide(newslide);

          if (newslide->TransitionTime>0)
          {
            if (newslide->TransitionId >= Transitions.GetCount())
              SetTransition(RndTrans.Get(),newslide->TransitionTime);
            else
              SetTransition(newslide->TransitionId,newslide->TransitionTime);
          }
          else
          {
            
            EndTransition();
          }
        }
        else sDPrintF(L"skipping faulty slide.\n");
        delete newslide;
      }

      // handle Siegmeister bar animation
      RNSiegmeister *siegnode = GetNode<RNSiegmeister>(L"Siegmeister",CurSlide);
      if (siegnode && !siegnode->DoBlink && Started)
      {
        siegnode->Fade = sMin(1.0f,(CurRenderTime-SlideStartTime)/MyConfig->BarAnimTime);
        if (siegnode->Fade >= 1)
        {
          PlMgr.Next(sTRUE,sTRUE);
          Started=sFALSE; // let's hope nobody presses space.
        }
      }

      SetPaintInfo(PaintInfo);

      PaintInfo.TimeMS = Time;
      PaintInfo.TimeBeat = Doc->MilliSecondsToBeats(Time);

      sTextBuffer Log;
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

      DoPaint(RootObj,PaintInfo);
      FramesRendered++;

      if (TransTime>=0)
      {
        TransTime += tdelta/TransDurMS;
        if (TransTime>=1.0f)
          EndTransition();
      }
      CurRenderTime += tdelta/1000.0f;
      NextRenderTime += tdelta/1000.0f;

      sEnableGraphicsStats(0);
      sSetTarget(sTargetPara(0,0,&PaintInfo.Client));
      App2->Painter->SetTarget(PaintInfo.Client);
      App2->Painter->Begin();
      App2->Painter->SetPrint(0,0xff000000,1,0xff000000);
      App2->Painter->Print(10,11,Log.Get());
      App2->Painter->Print(11,10,Log.Get());
      App2->Painter->Print(12,11,Log.Get());
      App2->Painter->Print(11,12,Log.Get());
      App2->Painter->SetPrint(0,0xffc0c0c0,1,0xffffffff);
      App2->Painter->Print(11,11,Log.Get());
      App2->Painter->End();
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
    if (PlMgr.OnInput(ie))
      return;

    sU32 key = ie.Key;
    key &= ~sKEYQ_CAPS;
    if(key & sKEYQ_SHIFT) key |= sKEYQ_SHIFT;
    if(key & sKEYQ_CTRL ) key |= sKEYQ_CTRL;
    if(key & sKEYQ_ALT ) key |= sKEYQ_ALT;

    Config::KeyEvent *kev;
    sFORALL(MyConfig->Keys,kev) if (key == kev->Key) switch (kev->Type)
    {
    case Config::PLAYSOUND:
      SoundPlayer.Init(kev->ParaStr);
      SoundPlayer.Play();
      return;
    case Config::STOPSOUND:
      SoundPlayer.Exit();
      return;
    case Config::FULLSCREEN:
      ScreenMode.Flags ^= sSM_FULLSCREEN;
      sSetScreenMode(ScreenMode);
      return;
    case Config::SETRESOLUTION:
      SetupScreenMode(kev->ParaRes,ScreenMode.Flags&sSM_FULLSCREEN);
      sSetScreenMode(ScreenMode);
      return;
    }

    switch(key)
    {

    case ' ':
      if (TransTime<0 && !Started)
      {
        Started=1;
        SlideStartTime = CurRenderTime;
      }
      break;

    case sKEY_ESCAPE|sKEYQ_SHIFT:
    case sKEY_F4|sKEYQ_ALT:
      sExit();
      break;

    case 'F'|sKEYQ_SHIFT:
      Fps = !Fps;
      break;

    case 'M'|sKEYQ_SHIFT:
      if(sSchedMon==0)
      {
        sSchedMon = new sStsPerfMon();
      }
      MTMon = !MTMon;
      break;

    }
  }
} *App2=0;


extern void sCollector(sBool exit=sFALSE);


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

//{"rpc":{"@attributes":{"type":"request","name":"get_playlists"}}


static void sExitSts()
{
  sDelete(sSched);
}


void sMain()
{
  sAddSubsystem(L"StealingTaskScheduler (wz4player style)",0x80,0,sExitSts);

  sGetMemHandler(sAMF_HEAP)->MakeThreadSafe();

  sArray<sDirEntry> dirlist;
  sString<1024> wintitle;
  wintitle.PrintF(L"screens4");

  sSetWindowName(wintitle);
  sAddGlobalBlobHeap();

  MyConfig = new Config;
  if (!MyConfig->Read(L"config.txt"))
  {
    sFatal(MyConfig->GetError());
  }

  sAddDebugServer(MyConfig->HttpPort);

  // find packfile and open it
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
  
  // open selector and set screen mode
  sInt flags=sISF_2D|sISF_3D|sISF_CONTINUOUS; // need 2D for text rendering

  sString<256> title=sGetWindowName();
  title.PrintAddF(L"  (player V%d.%d)",WZ4_VERSION,WZ4_REVISION);
  if(sCONFIG_64BIT)
    title.PrintAddF(L" (64Bit)");
  sSetWindowName(title);


  sInt refresh = MyConfig->DefaultResolution.RefreshRate;
  if (MyConfig->DefaultResolution.Width==0 || MyConfig->DefaultResolution.Height==0)
  {
    sScreenInfo si;
    sGetScreenInfo(si);
    MyConfig->DefaultResolution.Width = si.CurrentXSize;
    MyConfig->DefaultResolution.Height = si.CurrentYSize;
  }

  SetupScreenMode(MyConfig->DefaultResolution,MyConfig->DefaultFullscreen);
  sSetScreenMode(ScreenMode);
  if (ScreenMode.Flags & sSM_FULLSCREEN) flags|=sISF_FULLSCREEN;
  sInit(flags,ScreenMode.ScreenX,ScreenMode.ScreenY);
 
  sSched = new sStsManager(128*1024,512,0);

  // .. and go!
  App2=new MyApp;
  App2->WZ4Name = wz4name;
  sSetApp(App2);
}

/****************************************************************************/

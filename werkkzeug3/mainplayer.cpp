// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#include "_start.hpp"
#include "_viruz2.hpp"
#include "_ogg.hpp"
#include "kdoc.hpp"
#include "kkriegergame.hpp"
#include "engine.hpp"
#include "genoverlay.hpp"
#include "rtmanager.hpp"

#define WAITFORKEY  0
#define LINKEDIN    0
#define LOADERTUNE  0//!sINTRO

#if sNOCRT
extern sInt MallocSize;
#endif

/****************************************************************************/

static CV2MPlayer *Sound;
static sMusicPlayer *Player = 0;
static sInt SoundTimer;
KDoc *Document;
KEnvironment *Environment;
KKriegerGame *Game;
sF32 GlobalFps;

extern "C" sU8 DebugData[],LoaderTune[];
extern "C" sU32 LoaderTuneSize;

static sU8* PtrTable[] =
{
  (sU8 *) 0x54525450,     // entry 0: export data ('PTRT')
  (sU8 *) 0x454C4241,     // entry 1: viruzII tune ('ABLE')
};

extern sInt IntroTargetAspect;
extern sInt IntroLoop;

#define SOUNDALIGN 4096

/****************************************************************************/

/****************************************************************************/

#if sLINK_KKRIEGER
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
#endif

/****************************************************************************/

void IntroSoundHandler(sS16 *stream,sInt left,void *user)
{
  static sF32 buffer[4096*2];
  sF32 *fp;
  sInt count;
  sInt i;

  SoundTimer += left;
  if(Sound)
  {
    while(left>0)
    {
      count = sMin<sInt>(4096,left);
      Sound->Render(buffer,count);
      fp = buffer;
      for(i=0;i<count*2;i++)
        //*stream++ = 0;
        *stream++ = sRange<sInt>(*fp++*0x7fff,0x7fff,-0x7fff);
      left-=count;
    }
#if sLINK_KKRIEGER
#pragma lekktor(off)
    if(SoundTimer>=(2*60+25)*44100)
    {
      Sound->Open(Document->SongData);
      Sound->Play(0);
      SoundTimer = 0;
    }
#pragma lekktor(on)
#endif
  }
  else
  {
    for(i=0;i<left*2;i++)
      *stream++ = 0;
    SoundTimer = 0;
  }
}

static void MusicPlayerHandler(sS16 *buffer,sInt samples,void *user)
{
  sMusicPlayer *player = (sMusicPlayer *) user;
  player->Render(buffer,samples);
}

extern sBool ConfigDialog(sInt nr,const sChar *title);

static void PaintHole(const sRect &outer,const sRect &inner,sU32 color)
{
  sU32 colors[4];
  sRect rects[4];
  sInt nRects;

  for(sInt i=0;i<4;i++)
    colors[i] = color;

  nRects = 0;
  
  // top
  if(inner.y0 > outer.y0) 
    rects[nRects++].Init(outer.x0,outer.y0,outer.x1,inner.y0);

  // left
  if(inner.x0 > outer.x0)
    rects[nRects++].Init(outer.x0,inner.y0,inner.x0,inner.y1);

  // right
  if(inner.x1 < outer.x1)
    rects[nRects++].Init(inner.x1,inner.y0,outer.x1,inner.y1);

  // bottom
  if(inner.y1 < outer.y1)
    rects[nRects++].Init(outer.x0,inner.y1,outer.x1,outer.y1);

  sSystem->ColorFill(nRects,rects,colors);
}

extern sInt AspectRatioList[][2];
extern sInt IntroTexQuality;
extern sBool IntroShadows;

// handles aspect ratio stuff
static void DetermineTargetViewport(sViewport &vp)
{
#if !sLINK_KKRIEGER
  sRect r = vp.Window;

  sInt targetWidth = r.XSize();
  //sInt targetHeight = 3 * AspectRatioList[IntroTargetAspect][0] * r.YSize() / (4 * AspectRatioList[IntroTargetAspect][1]);
  sInt targetHeight = AspectRatioList[IntroTargetAspect][0] * r.YSize() / (2 * AspectRatioList[IntroTargetAspect][1]);

  if(targetHeight > r.YSize())
  {
    targetWidth = sMulDiv(targetWidth,r.YSize(),targetHeight);
    targetHeight = r.YSize();
  }

  vp.Window.x0 = (r.XSize() - targetWidth) / 2;
  vp.Window.x1 = (r.XSize() + targetWidth) / 2;
  vp.Window.y0 = (r.YSize() - targetHeight) / 2;
  vp.Window.y1 = (r.YSize() + targetHeight) / 2;
#else
  vp.Window.Init(0,sSystem->ConfigY*1/6,sSystem->ConfigX,sSystem->ConfigY*5/6);
#endif
}

sBool sAppHandler(sInt code,sDInt value)
{
  sInt beat;
  static const sU8 *data;
  sViewport vp,clearvp;
  sInt i,max;
  sF32 curfps;
  static sF32 oldfps;
  static sInt FirstTime,ThisTime,LastTime;
  static sInt framectr=0;
  sRect r;

  KOp *root;

  switch(code)
  {
  case sAPPCODE_CONFIG:
#if !LINKEDIN
    data = PtrTable[0];

#if !sINTRO
    if(((sInt)data)==0x54525450)
    {
      data = sSystem->LoadFile(sSystem->GetCmdLine());
      if(data==0)
      {
        // get module name and load a .kx file with the same name
        sChar kxData[256];
        sSystem->GetModuleBaseName(kxData, sizeof(kxData));
        sAppendString(kxData, ".kx", sizeof(kxData));
        data = sSystem->LoadFile(kxData);
        if(data==0)
          sSystem->Abort("need data file");
      }
    }
#endif
#else
    data = DebugData;
#endif

#if sCONFIGDIALOG
    {
      sChar *name = 0;
      if(data[0]&1) name = (sChar *)(data+4);
      return ConfigDialog(101,name) == 1;
    }
#endif
    break;

  case sAPPCODE_INIT:
#if !sCONFIGDIALOG
#if !LINKEDIN
    data = PtrTable[0];
    if(((sInt)data)==0x54525450)
    {
      data = sSystem->LoadFile(sSystem->GetCmdLine());
      if(!data)
        sFatal("need data file");
    }
#else
    data = DebugData;
#endif
#endif
    Document = new KDoc;

    Document->Init(data);
    Environment = new KEnvironment;
    Sound = 0;

    // low texture size: only for texture quality setting "normal"
    GenBitmapTextureSizeOffset = (IntroTexQuality >= 1) ? 0 : -1;
    GenBitmapDefaultFormat = (IntroTexQuality >= 2) ? sTF_A8R8G8B8 : sTF_DXT1;

    sInitPerlin();
    
    // determine render viewport
    vp.Init();
    DetermineTargetViewport(vp);

    RenderTargetManager = new RenderTargetManager_;
    RenderTargetManager->SetMasterViewport(vp);

    // preallocate some rendertargets
    Engine = new Engine_;
    RenderTargetManager->Add(0x00010000,0,0);
    RenderTargetManager->Add(0x00020010,512,256); // glare temp 1
    RenderTargetManager->Add(0x00020011,256,128); // glare temp 2

    GenOverlayInit();
    GenOverlayManager->SetMasterViewport(vp);

    if(!IntroShadows)
      Engine->SetUsageMask(~(1<<ENGU_SHADOW));

#if sLINK_KKRIEGER
    Game = new KKriegerGame;
    Game->Init();
#else
    Game = 0;
#endif

#if LOADERTUNE
    // start loader tune
    Player = new sViruz2;
    Player->Load(LoaderTune,LoaderTuneSize);
    Player->Start(0);
    sSystem->SetSoundHandler(MusicPlayerHandler,SOUNDALIGN,Player);
#endif

    sFloatFix();
    i = sSystem->GetTime();
    Environment->Splines = &Document->Splines.Array;
    Environment->SplineCount = Document->Splines.Count;
    Environment->Game = Game;
    Environment->InitView();
    Environment->InitFrame(0,0);
    Document->Precalc(Environment);
    Environment->ExitFrame();

    // stop loader tune
#if LOADERTUNE
    sSystem->SetSoundHandler(0,0,0);
    delete Player;
    Player = 0;
#endif

#if WAITFORKEY
    sSystem->WaitForKey();
#endif

    if(Document->SongSize)
    {
#if sINTRO
      Sound = new CV2MPlayer;

#if sLINK_KKRIEGER
      if(Document->SampleSize)
        RenderSoundEffects(Document,Document->SampleData);
#endif

      Sound->Init();
      Sound->Open(Document->SongData);
      Sound->Play(0);
      SoundTimer = 0;
      sSystem->SetSoundHandler(IntroSoundHandler,SOUNDALIGN);
#else // !sINTRO
#if sLINK_OGG
      Player = new sOGGDecoder;
#else
      Player = new sViruz2;
#endif
      Player->Load(Document->SongData,Document->SongSize);
      Player->Start(0);
      sSystem->SetSoundHandler(MusicPlayerHandler,SOUNDALIGN,Player);
#endif
    }

#if sLINK_KKRIEGER
    Game->ResetRoot(Environment,Document->RootOps[Document->CurrentRoot],1);
#endif

    FirstTime = sSystem->GetTime();
    LastTime = 0;
    oldfps = 0.0f;

#if sNOCRT
    sDPrintF("memory allocated: %d bytes\n",MallocSize);
#endif

    break;

#if !sINTRO
  case sAPPCODE_EXIT:
    sSystem->SetSoundHandler(0,0,0);
    if(Sound)
    {
      CV2MPlayer *old;
      old = Sound;
      Sound = 0;
      delete old;
    }
    delete Player;
    delete Environment;
#if sLINK_KKRIEGER
    Game->Exit();
    delete Game;
#endif
    Document->Exit();
    delete Document;
    delete RenderTargetManager;
    delete Engine;
    GenOverlayExit();
    break;
#endif

  case sAPPCODE_PAINT:
    // tick processing (moved up to reduce input lag by 1 frame)

    ThisTime = sSystem->GetTime() - FirstTime;

#if sLINK_KKRIEGER
    beat = sMulDiv(ThisTime,Document->SongBPM,60000);
#else
    if(Document->BuzzTiming)
    {
      sInt sample;
      sample = Document->SongSize ? sSystem->GetCurrentSample() : sMulDiv(ThisTime,44100,1000);
      beat = sDivShift(sample,60*44100/((Document->SongBPM>>16)*8)) / 8;
    }
    else
      beat = sMulDiv(sSystem->GetCurrentSample(),Document->SongBPM,44100*60);
//    beat *=4;
//    beat += 0x3000000;

#endif

#if !sLINK_KKRIEGER
    if(beat>Document->SongLength)
    {
      if(IntroLoop)
      {
#if sINTRO
        sSystem->SetSoundHandler(0,SOUNDALIGN);
        Sound->Open(Document->SongData);
        Sound->Play(0);
        sSystem->SetSoundHandler(IntroSoundHandler,SOUNDALIGN);
#else
        sSystem->SetSoundHandler(0,SOUNDALIGN);
        delete Player;

#if sLINK_OGG
        Player = new sOGGDecoder;
#else
        Player = new sViruz2;
#endif
        Player->Load(Document->SongData,Document->SongSize);
        Player->Start(0);
        sSystem->SetSoundHandler(MusicPlayerHandler,SOUNDALIGN,Player);
#endif
      }
      else
        sSystem->Exit();
    }
#endif

    curfps = ThisTime - LastTime;
    oldfps += (curfps - oldfps) * 0.1f;
    GlobalFps = 1000.0f / oldfps;

    max = ThisTime/10-LastTime/10;
    if(max>10) max = 10;

    sFloatFix();
#if sLINK_KKRIEGER
    Game->OnTick(Environment,max);
    sSystem->Sample3DCommit();
#endif
    LastTime = ThisTime;

    sFloatFix();

    // root-switching logic and outermost stuff

    vp.Init();
    r = vp.Window;
    DetermineTargetViewport(vp);

    GenOverlayManager->SetMasterViewport(vp);
    RenderTargetManager->SetMasterViewport(vp);

#if sLINK_KKRIEGER
    sInt mode;
    mode = Game->GetNewRoot();

    if(mode!=Document->CurrentRoot)
    {
      sSystem->SetSoundHandler(0,0);
      Document->CurrentRoot = mode;
      Environment->InitView();
      Environment->InitFrame(0,0);
      Document->Precalc(Environment);
      Environment->ExitFrame();
      Game->ResetRoot(Environment,Document->RootOps[Document->CurrentRoot],0);

      Sound->Open(Document->SongData);
      Sound->Play(0);
      sSystem->SetSoundHandler(IntroSoundHandler,SOUNDALIGN);
    }
#endif

    // timing
    Environment->InitFrame(beat,ThisTime);
    Document->AddEvents(Environment);
#if sLINK_KKRIEGER
    Game->AddEvents(Environment);
    Game->FlushPhysic();
#endif

    // game painting

    clearvp.Init();

    sSystem->SetViewport(clearvp);
    sSystem->Clear(sVCF_ALL,0);

    Environment->GameCam.Init();
#if sLINK_KKRIEGER
    Game->GetCamera(Environment->GameCam);
#endif
    

    //Environment->Aspect =  1.0f*vp.Window.XSize()/vp.Window.YSize();
    Environment->Aspect = 2.0f;
    //Environment->Aspect = 4.0f / 3.0f;

    root = Document->RootOps[Document->CurrentRoot];

    if(beat>=0 && root->Cache->ClassId==KC_DEMO)
    {
      GenOverlayManager->Reset(Environment);
      GenOverlayManager->RealPaint = sTRUE;
      GenOverlayManager->Game = Game;

      sFloatFix();
      root->Exec(Environment);

      GenOverlayManager->RealPaint = sFALSE;
      GenOverlayManager->Reset(Environment);
    }

    sFloatFix();
    Environment->ExitFrame();
    Environment->Mem.Flush();

    // finally, the black bars
    PaintHole(r,vp.Window,0xff000000);

//    sSystem->SetWinMouse(vp.Window.x1/2,vp.Window.y1/2);
    break;

  case sAPPCODE_KEY:
#if sLINK_KKRIEGER
    Game->OnKey(value);
    if(value == (sKEY_ESCAPE|sKEYQ_SHIFT) || value==sKEY_CLOSE)
    {
      sSystem->SetSoundHandler(0,0,0);
      sSystem->Exit();
    }
#else
    if(value == sKEY_ESCAPE || value == sKEY_CLOSE)
    {
      sSystem->SetSoundHandler(0,0,0);
      sSystem->Exit();
    }
#endif
    break;
#if !sINTRO
  default:
    return sFALSE;
#endif
  }

  return sTRUE;
}

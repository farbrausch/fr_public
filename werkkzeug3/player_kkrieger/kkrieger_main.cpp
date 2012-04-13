// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#include "_start.hpp"
#include "_viruz2.hpp"
#include "_ogg.hpp"
#include "kdoc.hpp"
#include "kkriegergame.hpp"
#include "engine.hpp"
#include "genoverlay.hpp"

/****************************************************************************/

static CV2MPlayer *Sound;
static sMusicPlayer *Player = 0;
static sInt SoundTimer;
KDoc *Document;
KEnvironment *Environment;
KKriegerGame *Game;
sF32 GlobalFps;

extern "C" sU8 DebugData[];
static sU8* PtrTable[] =
{
  (sU8 *) 0x54525450,     // entry 0: export data ('PTRT')
  (sU8 *) 0x454C4241,     // entry 1: viruzII tune ('ABLE')
};

extern sInt IntroLoop;

/****************************************************************************/

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
#pragma lekktor(off)
    if(SoundTimer>=(2*60+25)*44100)
    {
      Sound->Open(Document->SongData);
      Sound->Play(0);
      SoundTimer = 0;
    }
#pragma lekktor(on)
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

extern sBool ConfigDialog(sInt nr);

sBool sAppHandler(sInt code,sDInt value)
{
  sInt beat;
  sU8 *data;
  sViewport vp,clearvp;
  sInt i,max;
  sF32 curfps;
  static sF32 oldfps;
  static sInt FirstTime,ThisTime,LastTime,sample;
  static sInt framectr=0;
  
  KOp *root;

  //GenBitmapTextureSizeOffset = -1;
  switch(code)
  {
#if !sINTRO
  case sAPPCODE_CONFIG:
    sSetConfig(sSF_DIRECT3D|sSF_FULLSCREEN,800,600);
    break;
#endif

  case sAPPCODE_INIT:
    data = PtrTable[0];
    if(((sInt)data)==0x54525450)
    {

      data = sSystem->LoadFile(sSystem->GetCmdLine());
      if(data==0)
        sSystem->Abort("need data file");
    }
    Document = new KDoc;
    Document->Init(data);
    Environment = new KEnvironment;
    Sound = 0;

    sInitPerlin();   
    GenOverlayInit();

    Engine = new Engine_;

    Game = new KKriegerGame;
    Game->Init();

    sFloatFix();
    i = sSystem->GetTime();
    Environment->Splines = &Document->Splines.Array;
    Environment->SplineCount = Document->Splines.Count;
    Environment->Game = Game;
    Environment->InitView();
    Environment->InitFrame(0,0);
    Document->Precalc(Environment);
    Environment->ExitFrame();

    if(Document->SongSize)
    {
      Sound = new CV2MPlayer;

      if(Document->SampleSize)
        RenderSoundEffects(Document,Document->SampleData);

      Sound->Open(Document->SongData);
      Sound->Play(0);
      SoundTimer = 0;
      sSystem->SetSoundHandler(IntroSoundHandler,64);
    }

    Game->ResetRoot(Environment,Document->RootOps[Document->CurrentRoot],1);

    FirstTime = sSystem->GetTime();
    LastTime = 0;
    oldfps = 0.0f;
    break;
#if !sINTRO || sPROJECT == sPROJ_SNOWBLIND
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
    Game->Exit();
    delete Game;
    Document->Exit();
    delete Document;
    delete Engine;
    GenOverlayExit();
    break;
#endif
  case sAPPCODE_PAINT:
    // tick processing (moved up to reduce input lag by 1 frame)

    ThisTime = sSystem->GetTime() - FirstTime;

    beat = sMulDiv(ThisTime,Document->SongBPM,60000);

    curfps = ThisTime - LastTime;
    oldfps += (curfps - oldfps) * 0.1f;
    GlobalFps = 1000.0f / oldfps;

    max = ThisTime/10-LastTime/10;
    if(max>10) max = 10;

    sFloatFix();
    Game->OnTick(Environment,max);
    sSystem->Sample3DCommit();
    LastTime = ThisTime;

    sFloatFix();

    // root-switching logic and outermost stuff

    vp.Init();
    vp.Window.Init(0,sSystem->ConfigY*1/6,sSystem->ConfigX,sSystem->ConfigY*5/6);
    GenOverlayManager->SetMasterViewport(vp);

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
      sSystem->SetSoundHandler(IntroSoundHandler,64);
    }

    // timing

    Environment->InitFrame(beat,ThisTime);
    Document->AddEvents(Environment);
    Game->AddEvents(Environment);
    Game->FlushPhysic();

    // game painting

    clearvp.Init();
    clearvp.ClearColor = 0;
    sSystem->BeginViewport(clearvp);
    sSystem->EndViewport();

    Environment->GameCam.Init();
    Game->GetCamera(Environment->GameCam);
    Environment->GameCam.ZoomY = 1.0f*vp.Window.XSize()/vp.Window.YSize();

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

    sFloatFix();
    Environment->ExitFrame();
    Environment->Mem.Flush();
//    sSystem->SetWinMouse(vp.Window.x1/2,vp.Window.y1/2);
    break;

  case sAPPCODE_KEY:
#if sPROJECT==sPROJ_KKRIEGER
    Game->OnKey(value);
    if(value == (sKEY_ESCAPE|sKEYQ_SHIFT) || value==sKEY_CLOSE)
      sSystem->Exit();
#else
    if(value == sKEY_ESCAPE)
      sSystem->Exit();
#endif
    break;
#if !sINTRO
  default:
    return sFALSE;
#endif
  }

  return sTRUE;
}

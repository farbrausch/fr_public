// This file is distributed under a BSD license. See LICENSE.txt for details.
#include "_types.hpp"
#include "_start.hpp"
#include "_startdx.hpp"
#include "_diskitem.hpp"
#include "_viruz2.hpp"
#include "genplayer.hpp"
#include "genfx.hpp"

#include "main.hpp"
void ProgressBar(sInt done);

/****************************************************************************/

GenPlayer *Player; 
ScriptRuntime *SR__;
sInt TotalFrame;
static sInt TimeSpeed;
static sInt BeatMax;
static sInt LastTime;
sInt TimeNow;
static sInt TotalSample;
static sViruz2 Sound;

/****************************************************************************/
/***                                                                      ***/
/***   Intro Version (does not need GenPlayer                             ***/
/***                                                                      ***/
/****************************************************************************/

extern "C" sU8 Script[];
extern "C" sU8 ScriptPack[];
extern "C" sU8 V2MTune[];
extern "C" sU8 V2MEnd[];

#if sINTRO

void IntroRenderHandler(sS16 *stream,sInt samples)
{
  Sound.Render(stream,samples);
}

sBool sAppHandler(sInt code,sU32 value)
{
  sViewport Viewport;
  sInt beatnow;
  const ScriptFunction *f;

  switch(code)
  {
  case sAPPCODE_INIT:
    SR__ = new ScriptRuntime;
//    sBroker->AddRoot(SR);

    for(f=&ScriptFunctions[0];f->id;f++)
	  {
      sVERIFY(f->params!=0xf0000);
		  SR__->AddCode(f->id,f->params,f->func);
//      sDPrintF("%02x\n",f->id);
	  }

    Sound.Stream = V2MTune;
    Sound.Init(0);
    sInitPerlin();
		InitFXMaster();
		Viewport.Init();

		FXMaster->SetMasterViewport(Viewport);
//    SR__->Load((sU32 *)Script);
    SR__->PackedImport((sU8 *)ScriptPack);

    SR__->SetGlobal(sGPG_BPM,140*0x10/*,512*0x10000*/);
    SR__->SetGlobal(sGPG_PLAYER,0000);
//    SR__->SetGlobal(sGPG_BEATMAX/*(sInt)this*/);
    FXMaster->ResetAlloc();
    TotalFrame++;
    SR__->ShowProgress = 1;
		SR__->Execute(4);
    SR__->Execute(1);

    TimeSpeed = sMulDiv(60*44100,0x10000,SR__->GetGlobal(sGPG_BPM));
    BeatMax = SR__->GetGlobal(sGPG_BEATMAX)/0x10000;

    for(beatnow=0x00000000;beatnow<0x01a00000;beatnow+=0x00040000)
    {
      SR__->SetGlobal(sGPG_BEAT,beatnow);
      SR__->Execute(2);
    }

    ProgressBar(1);
    SR__->ShowProgress = 0;

    break;

  case sAPPCODE_EXIT:
		CloseFXMaster();
    SR__ = 0;
//    sBroker->RootCount = 0;
    sBroker->Free();
    break;
    
  case sAPPCODE_PAINT:
    Viewport.Init();
/*
    static sInt tdelta,tt;
    tdelta = sSystem->GetTime()-tt;
    tt+=tdelta;
    sDPrintF("%08x %08x\n",sSystem->GetCurrentSample()-LastTime,tdelta);
    */
    LastTime = sSystem->GetCurrentSample();
    if(LastTime<0)
      LastTime = 0;

    beatnow = sMulDiv(LastTime,0x10000,TimeSpeed);
    TimeNow = LastTime; 
#if !sRELEASE
    beatnow *= 3;
#endif

    SR__->SetGlobal(sGPG_BEAT,beatnow);
//    SR->SetGlobal(sGPG_TIME,LastTime);
//    SR->SetGlobal(sGPG_TICKS,1);
//    SR->SetGlobal(sGPG_PLAYER,(sInt)0);
//    SR->SetGlobal(sGPG_MOUSEX,mx*0x10000/screen.XSize);
//    SR->SetGlobal(sGPG_MOUSEY,my*0x10000/screen.YSize);

    // clear the screen
    FXMaster->ResetAlloc();
    CurrentGenPlayer = 0;
    TotalFrame++;
    SR__->Execute(2);

//    sBroker->Free();
    break;
  default:
    return sFALSE;
  }
  return sTRUE;
}

#endif


/****************************************************************************/
/***                                                                      ***/
/***   Full Version                                                       ***/
/***                                                                      ***/
/****************************************************************************/

#if !sINTRO

sBool sAppHandler(sInt code,sU32 value)
{
  sU32 *bytecode;
	sViewport view;

//	sBool ok;

  switch(code)
  {
  case sAPPCODE_CONFIG:
#if sDEBUG || 1
    sSetConfig(sSF_DIRECT3D,800,600);
#else
		sSetConfig(sSF_DIRECT3D|sSF_FULLSCREEN,800,600);
#endif
    break;

  case sAPPCODE_INIT:
    Player=new GenPlayer;

    text = sSystem->LoadText("default.raw");
    sVERIFY(text);
		bytecode = (sU32 *) text;
    sVERIFY(bytecode);

    sBroker->AddRoot(Player);
    sInitPerlin();
		InitFXMaster();
		view.Init();
		FXMaster->SetMasterViewport(view);
    Player->SR->Load(bytecode);
    Player->Status = 1;

    Player->Run();

    break;

  case sAPPCODE_EXIT:
		CloseFXMaster();
    sSystem->SetSoundHandler(0,0,0);
    sBroker->RemRoot(Player);
    sBroker->Free();
    break;

  case sAPPCODE_KEY:
    switch(value)
    {
    case sKEY_ESCAPE|sKEYQ_SHIFTL:
    case sKEY_ESCAPE|sKEYQ_SHIFTR:
    case sKEY_ESCAPE:
    case sKEY_CLOSE:
      sSystem->Exit();
      break;
    }
    break;
    
  case sAPPCODE_PAINT:
    Player->TimePlay = 1;
    Player->OnPaint(0);
    break;

  case sAPPCODE_LAYOUT:
    break;

  case sAPPCODE_CMD:
    break;

  case sAPPCODE_DEBUGPRINT:
    return sFALSE;

  default:
    return sFALSE;
  }
  return sTRUE;
}

#endif



/****************************************************************************/


// This file is distributed under a BSD license. See LICENSE.txt for details.
#ifndef __GENPLAYER_HPP__
#define __GENPLAYER_HPP__

#include "cslce.hpp"
#include "cslrt.hpp"
#include "_types.hpp"
#include "_start.hpp"

/****************************************************************************/
/****************************************************************************/
 
class GenPlayer : public sObject
{
public:
  GenPlayer();
  ~GenPlayer();
  sU32 GetClass() { return sCID_GENPLAYER; }
  void Tag();

  void OnPaint(sRect *r);
  void OnShow(sRect *r,sInt funcid);
  void Compile(sChar *text);
  void Run();
  void Stop();

  void SoundHandler(sS16 *buffer,sInt samples);

  class GenBitmapGlobals *BitmapQueue;
#if !sPLAYER
  ScriptCompiler *SC;
#endif
  ScriptRuntime *SR;
  sInt Status;                    // 0=uncompiled, 1=compiled, 2=running
  sMusicPlayer *MP;               // v2m / OGG / Mp3
  sChar MPName[256];              // name of current song (for caching)

// variables that can be modified by commands

  sViewport Viewport;
  sMatrix Matrix;
  sCamera Camera;
  sObject *Show;
  sInt ShowTexture;

  sInt BeatMax;                   // total beats (no fractional part)
  sInt TimeSpeed;                 // speed in samples per peat
  sInt TimeNow;                   // current time in samples
  sInt TimePlay;                  // 0=stop, 1=play
  sInt TimeLoop;                  // 0=off, 1=on
  sInt BeatNow;                   // current beat * 0x10000
  sInt BeatLoopStart;             // loop start beat * 0x10000
  sInt BeatLoopEnd;               // loop end beat * 0x10000
  sInt TotalSample;               // for timing..
  sInt LastTime;                  // if we are in pause mode...
  sInt TotalFrame;                // this just counts up. allways up. several days..
};

extern GenPlayer *CurrentGenPlayer;

struct ScriptFunction
{
	sInt id;
	sInt params;
	void *func;
};
extern const struct ScriptFunction ScriptFunctions[];

/****************************************************************************/
/****************************************************************************/

#define sGPG_TIME     0
#define sGPG_BEAT     1
#define sGPG_TICKS    2
#define sGPG_BPM      3
#define sGPG_BEATMAX  4
#define sGPG_PLAYER   5
#define sGPG_MOUSEX   6
#define sGPG_MOUSEY   7

/****************************************************************************/
/****************************************************************************/

#if sLINK_GUI

#include "_util.hpp"
#include "_gui.hpp"

class GenTimeBorder : public sGuiWindow
{
  sInt Height;
  sInt DragMode;
  sInt DragStart;
public:
  GenTimeBorder();
  void Tag();
  void OnCalcSize();
  void OnSubBorder();
  void OnPaint();
  void OnDrag(sDragData &dd);

  GenPlayer *Player;
};

#endif

#endif  

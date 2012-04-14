
#ifndef _SOUNDSYS_H_
#define _SOUNDSYS_H_

// define this to have the enhanced syncing features,
// undefine this if you "only" want to play back music.
//#define RULESYNC

// the tick rate you want the timer functions to operate at
#define TIMER_TICKS_PER_SEC 1000


extern "C"
{

#ifdef _DEBUG
  extern int ssCPUUsage;
#endif

	extern void __stdcall ssRender(sF32 *outbuf, sU32 len);

	// init sound system
	// data: pointer to V2M file
	// hWnd: your window handle
  extern void __stdcall ssInit(sU8 *data, void *hWnd);

	// close sound system
  extern void __stdcall ssClose();

	// start playing (from song start)
	extern void __stdcall ssPlay();

	// stop playing
	extern void __stdcall ssStop();

	// get the timer value (will be reset when starting)
	// Note: The timer value is synchronized with the sound
	//			 output and will thus start at a negative value
	//       while the sound system fills up the first buffer!
	extern sS32 __stdcall ssGetTime();


	// Does a fade-out of the music within "time" milliseconds.
	// NOTE: Due to sound card latency, the fade will start
	//       some msecs later than you called this function, so
	//       make sure you wait a certain amount of time to
	//       not interrupt a running fade... or so...
	extern void __stdcall ssFadeOut(sU32 time);

  // make the sound system render the next block
  // put this in your frame loop
  extern void __stdcall ssDoTick();

#ifdef RULESYNC
	// Get current song position (being played actually)
	// return code format:
	// negative: not (yet) playing or any other error
  // otherwise: 0xBBBBTTNN, where
	// BBBB is the bar number (starting at 0)
	// TT   is the number of the 32th tick within the current bar
	// NN   is the total number of 32th ticks a the current bar has (32, normally, may change with
	//      different time signatures than 4/4)
	extern sS32 __stdcall ssGetPos();


	// Retrieve an array of timer<->song position 
	// returns: number of found song positions
	// dest: pointer to a variable which will receive the address of an array of long
	//       values structured as following:
	//       first  long: timer value
	//       second long: song position (see above for a description)
	//       ... and so on for every found position
	// otherv2m: NULL to get positions of song used with ssInit, ptr to v2m data otherwise
	// NOTE: it is your responsibility to free the array again.
	extern sU32 __stdcall ssCalcPositions(sS32 **dest, sU8 *otherv2m=0);

	// Starts playing at a certain timer position
	// time: the timer value you want to play from
	// NOTE: the actual playing position may differ from the
	//       value you specified, but will be as close as possible.
	//       Use ssGetPos()/ssGetSongTime() to determine where
	//       you really are.
	extern void __stdcall ssPlayFrom(sS32 time);

	// get the song timer value
	// Just as ssGetTime(), but it will reflect the song position even
	// after ssPlayFrom() calls.
	extern sS32 __stdcall ssGetSongTime();

#endif
}


#endif

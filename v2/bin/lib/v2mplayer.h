/*************************************************************************************/
/*************************************************************************************/
/**                                                                                 **/
/**  V2 module player (.v2m)                                                        **/
/**  (c) Tammo 'kb' Hinrichs 2000-2008                                              **/
/**  This file is under the Artistic License 2.0, see LICENSE.txt for details       **/
/**                                                                                 **/
/*************************************************************************************/
/*************************************************************************************/

#ifndef V2MPLAYER_H_
#define V2MPLAYER_H_

/*************************************************************************************/
/**                                                                                 **/
/**  Type definitions                                                               **/
/**                                                                                 **/
/*************************************************************************************/

#ifndef V2TYPES
#define V2TYPES

typedef int               sInt;
typedef unsigned int      sUInt;
typedef sInt              sBool;
typedef char              sChar;

typedef signed   char     sS8;
typedef signed   short    sS16;
typedef signed   long     sS32;
typedef signed   __int64  sS64;

typedef unsigned char     sU8;
typedef unsigned short    sU16;
typedef unsigned long     sU32;
typedef unsigned __int64  sU64;

typedef float             sF32;
typedef double            sF64;

#define sTRUE             1
#define sFALSE            0

template<class T> inline T sMin(const T a, const T b) { return (a<b)?a:b;  }
template<class T> inline T sMax(const T a, const T b) { return (a>b)?a:b;  }
template<class T> inline T sClamp(const T x, const T min, const T max) { return sMax(min,sMin(max,x)); }

#endif


/*************************************************************************************/
/**                                                                                 **/
/**  V2M player class                                                               **/
/**                                                                                 **/
/*************************************************************************************/

class V2MPlayer
{
public:

  // init
  // call this instead of a constructor
  void Init(sU32 a_tickspersec=1000) { m_tpc=a_tickspersec; m_base.valid=0; }



	// opens a v2m file for playing
	//
	// a_v2mptr			: ptr to v2m data
	//								NOTE: the memory block has to remain valid
	//								as long as the player is opened!
	// a_samplerate : samplerate at which the synth is operating
	//                if this is zero, output will be disabled and
	//                only the timing query functions will work

	// returns  : flag if succeeded
	//
	sBool Open (const void *a_v2mptr, sU32 a_samplerate=44100);


	// closes player
	//
	void  Close ();


	// starts playing
	//
	// a_time   : time offset from song start in msecs
	//
	void  Play (sU32 a_time=0);


	// stops playing
	//
	// a_fadetime : optional fade out time in msecs
	//
	void  Stop (sU32 a_fadetime=0);



	// render call (to be used from sound thread et al)
	// renders samples (or silence if not playing) into buffer
	//
	// a_buffer : ptr to stereo float sample buffer (0dB=1.0f)
	// a_len    : number of samples to render
	// 
	// returns  : flag if playing
	//
  void Render (sF32 *a_buffer, sU32 a_len, sBool a_add=0);



	// render proxy for C-style callbacks
	// 
	// a_this   : void ptr to instance
	// rest as in Render()
	//
	static void __stdcall RenderProxy(void *a_this, sF32 *a_buffer, sU32 a_len) 
	{ 
		reinterpret_cast<V2MPlayer*>(a_this)->Render(a_buffer,a_len);
	}

  // returns if song is currently playing
  sBool IsPlaying();


	#ifdef V2MPLAYER_SYNC_FUNCTIONS

	// Retrieves an array of timer<->song position 
	//
	// a_dest: pointer to a variable which will receive the address of an array of long
	//         values structured as following:
	//         first  long: time in ms
	//         second long: song position (see above for a description)
	//											format: 0xBBBBTTNN, where
	//															BBBB is the bar number (starting at 0)
	//															TT   is the number of the 32th tick within the current bar
	//															NN   is the total number of 32th ticks a the current bar has 
	//													         (32, normally, may change with different time signatures than 4/4)
	//         ... and so on for every found position
	//
	// NOTE: it is your responsibility to free the array again.
	//
	// returns: number of found song positions
	//
	sU32 CalcPositions(sS32 **a_dest);


	#endif


	// ------------------------------------------------------------------------------------------------------
	//  no need to look beyond this point.
	// ------------------------------------------------------------------------------------------------------

private: 

	// struct defs

	// General info from V2M file
	struct V2MBase
	{
		sBool valid;
		const sU8   *patchmap;
		const sU8   *globals;
		sU32	timediv;
		sU32	timediv2;
		sU32	maxtime;
		const sU8   *gptr;
		sU32  gdnum;
		struct Channel {
			sU32	notenum;
			const sU8		*noteptr;
			sU32	pcnum;
			const sU8		*pcptr;
			sU32	pbnum;
			const sU8		*pbptr;
			struct CC {
				sU32	ccnum;
				const sU8		*ccptr;
			} ctl[7];
		} chan[16];
		const char  *speechdata;
		const char  *speechptrs[256];
	};


	// player state
	struct PlayerState
	{
		enum { OFF, STOPPED, PLAYING, } state;
		sU32	time;
		sU32	nexttime;
		const sU8		*gptr;
		sU32	gnt;
		sU32	gnr;
		sU32  usecs;
		sU32  num;
		sU32  den;
		sU32	tpq;
		sU32  bar;
		sU32  beat;
		sU32  tick;
		struct Channel {
			const sU8		*noteptr;
			sU32  notenr;
			sU32	notent;
			sU8		lastnte;
			sU8   lastvel;
			const sU8   *pcptr;
			sU32  pcnr;
			sU32  pcnt;
			sU8		lastpc;
			const sU8		*pbptr;
			sU32  pbnr;
			sU32  pbnt;
			sU8		lastpb0;
			sU8		lastpb1;
			struct CC {
				const sU8		*ccptr;
				sU32  ccnt;
				sU32  ccnr;
				sU8   lastcc;
			} ctl[7];
		} chan[16];
		sU32 cursmpl;
		sU32 smpldelta;
		sU32 smplrem;
		sU32 tdif;
	};

  // \o/
  sU8         m_synth[3*1024*1024];   // TODO: keep me uptodate or use "new"

	// member variables
	sU32        m_tpc;
	V2MBase			m_base;
	PlayerState m_state;
	sU32        m_samplerate;
	sS32				m_timeoffset;
	sU8         m_midibuf[4096];
	sF32        m_fadeval;
	sF32        m_fadedelta;

	// internal methods

	sBool InitBase(const void *a_v2m);  // inits base struct from v2m
	void  Reset();                      // resets player, inits synth
	void  Tick();                       // one midi player tick


};

#endif
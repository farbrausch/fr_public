#ifndef _SOUNDDEF_H_
#define _SOUNDDEF_H_

class file;

// no problem if this header is included multiple times
// in case you get any linker collisions, prepend 
// __declspec(selectany) to the problematic declaration

enum V2CTLTYPES { VCTL_SKIP, VCTL_SLIDER, VCTL_MB, };

typedef struct {
	int		no;
	char  *name;
	char	*name2;
} V2TOPIC;

typedef struct {

	int   version;
	char  *name;
	V2CTLTYPES ctltype;
	int	  offset, min, max;
	int   isdest;
	char  *ctlstr;
} V2PARAM;

////////////////////////////////////////////
//
// V2 Patch Topics
//
////////////////////////////////////////////

const V2TOPIC v2topics[] = {
	{  2, "Voice","Vo" },
	{  6, "Osc 1","O1" },
	{  6, "Osc 2","O2" },
	{  6, "Osc 3","O3" },
	{  3, "VCF 1","F1" },
	{  3, "VCF 2","F2" },
	{  2, "Filters","Fi" },
	{  4, "Voice Dist","VD" },
	{  6, "Amp EG","E1" },
	{  6, "EG 2","E2" },
	{  7, "LFO 1","L1" },
	{  7, "LFO 2","L2" },
	{ 10, "Global","Gl" },
	{  4, "Channel Dist","CD" },
	{  7, "Chorus/Flanger","CF" },
	{  9, "Compressor","CC" },
	{  1, "Polyphony","Po" },
};
const int v2ntopics = sizeof(v2topics)/sizeof(V2TOPIC);

////////////////////////////////////////////
//
// V2 Modulation Sources
//
////////////////////////////////////////////

__declspec(selectany) const char *v2sources[] = {
	"Velocity",
	"Modulation",
	"Breath",
	"Ctl #3",
	"Ctl #4",
	"Ctl #5",
	"Ctl #6",
	"Volume",
	"Amp EG",
	"EG 2",
	"LFO 1",
	"LFO 2",
	"Note",
};
const int v2nsources = sizeof(v2sources)/sizeof(char *);

////////////////////////////////////////////
//
// V2 Patch Parameters
//
////////////////////////////////////////////

const V2PARAM v2parms[] = {
	// Voice (2)
	{ 0, "Panning", VCTL_SLIDER, 64,   0, 127, 1, 0		    											},
	{ 2, "Txpose",  VCTL_SLIDER, 64,   0, 127, 1, 0		    											},
  // Osc 1 (6)
	{ 0, "Mode"  ,  VCTL_MB    ,  0,   0,   7, 0, "Off|Saw/Tri|Pulse|Sin|Noise|XX|AuxA|AuxB"	},
	{ 2, "Ringmod", VCTL_SKIP  ,  0,   0,   1, 0, ""			      									},
	{ 0, "Txpose",  VCTL_SLIDER, 64,   0, 127, 1, 0		    											},
	{ 0, "Detune",  VCTL_SLIDER, 64,   0, 127, 1, 0															},
	{ 0, "Color",   VCTL_SLIDER, 64,   0, 127, 1, 0															},
	{ 0, "Volume",  VCTL_SLIDER,  0,   0, 127, 1, 0	                            },
  // Osc 2 (6)
	{ 0, "Mode"  ,  VCTL_MB    ,  0,   0,   7, 0, "!Off|Tri|Pul|Sin|Noi|FM|AuxA|AuxB"			},
	{ 2, "RingMod", VCTL_MB    ,  0,   0,   1, 0, "Off|On"					      				},
	{ 0, "Txpose" , VCTL_SLIDER, 64,   0, 127, 1, 0															},
	{ 0, "Detune",  VCTL_SLIDER, 64,   0, 127, 1, 0															},
	{ 0, "Color",   VCTL_SLIDER, 64,   0, 127, 1, 0															},
	{ 0, "Volume",  VCTL_SLIDER,  0,   0, 127, 1, 0                              },
  // Osc 3 (6)
	{ 0, "Mode"  ,  VCTL_MB    ,  0,   0,   7, 0, "!Off|Tri|Pul|Sin|Noi|FM|AuxA|AuxB"			},
	{ 2, "RingMod", VCTL_MB    ,  0,   0,   1, 0, "Off|On"												},
	{ 0, "Txpose",  VCTL_SLIDER, 64,   0, 127, 1, 0															},
	{ 0, "Detune",  VCTL_SLIDER, 64,   0, 127, 1, 0															},
	{ 0, "Color",   VCTL_SLIDER, 64,   0, 127, 1, 0															},
	{ 0, "Volume",  VCTL_SLIDER,  0,   0, 127, 1, 0                              },
  // VCF 1 (3)
	{ 0, "Mode",    VCTL_MB    ,  0,   0,   7, 0, "Off|Low|Band|High|Notch|All|MoogL|MoogH"  },
	{ 0, "Cutoff",  VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 0, "Reso",	  VCTL_SLIDER,  0,   0, 127, 1, 0															},
  // VCF 2 (3)
	{ 0, "Mode",    VCTL_MB    ,  0,   0,   7, 0, "Off|Low|Band|High|Notch|All|MoogL|MoogH"  },
	{ 0, "Cutoff",  VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 0, "Reso",		VCTL_SLIDER,  0,   0, 127, 1, 0															},
	// Routing (2)
	{ 0, "Routing", VCTL_MB    ,  0,   0,   2, 0, "!single|serial|parallel"			  },
	{ 3, "Balance", VCTL_SLIDER, 64,   0, 127, 1, 0														  },
	// Distortion (4)
	{ 0, "Mode",    VCTL_MB    ,  0,   0,  10, 0, "Off|OD|Clip|Crush|Dec|LPF|BPF|HPF|NoF|APF|MoL"   },
	{ 0, "InGain",  VCTL_SLIDER, 32,   0, 127, 1, 0															},
	{ 0, "Param 1", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 0, "Param 2", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	// Amp Envelope (6)
	{ 0, "Attack",  VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 0, "Decay",   VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 0, "Sustain", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 0, "SusTime", VCTL_SLIDER, 64,   0, 127, 1, 0															},
	{ 0, "Release", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 0, "Amplify", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	// Envelope 2 (6)
	{ 0, "Attack",  VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 0, "Decay",   VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 0, "Sustain", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 0, "SusTime", VCTL_SLIDER, 64,   0, 127, 1, 0															},
	{ 0, "Release", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 0, "Amplify", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	// LFO 1 (7)
	{ 0, "Mode"  ,  VCTL_MB    ,  0,   0,   4, 0, "Saw|Tri|Pulse|Sin|S+H"				},
	{ 0, "KeySync", VCTL_MB    ,  0,   0,   2, 0, "!Off|On"											},
	{ 0, "EnvMode", VCTL_MB    ,  0,   0,   2, 0, "!Off|On"											},
	{ 0, "Rate",		VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 0, "Phase",   VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 5, "Polarity",VCTL_MB,      0,   0,   2, 0, "!+|-|±"                       },
	{ 0, "Amplify", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	// LFO 2 (7)
	{ 0, "Mode"  ,  VCTL_MB    ,  0,   0,   4, 0, "Saw|Tri|Pulse|Sin|S+H"				},
	{ 0, "KeySync", VCTL_MB    ,  0,   0,   2, 0, "!Off|On"											},
	{ 0, "EnvMode", VCTL_MB    ,  0,   0,   2, 0, "!Off|On"											},
	{ 0, "Rate",		VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 0, "Phase",   VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 5, "Polarity",VCTL_MB,      0,   0,   2, 0, "!+|-|±"                       },
	{ 0, "Amplify", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	// Globals (10)
	{ 0, "KeySync", VCTL_MB    ,  0,   0,   2, 0, "!None|Osc|Full"							},
	{ 0, "ChanVol", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 6, "AuxA Recv",VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 6, "AuxB Recv",VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 6, "AuxA Send",VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 6, "AuxB Send",VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 0, "Reverb",	VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 0, "Delay",   VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 0, "FXRoute", VCTL_MB    ,  0,   0,   1, 0, "!Dis -> Cho|Cho -> Dis" },
	{ 1, "Boost",	  VCTL_SLIDER,  0,   0, 127, 1, 0															},
	// Channel Dist (4)
	{ 0, "Mode",    VCTL_MB    ,  0,   0,  10, 0, "Off|OD|Clip|Crush|Dec|LPF|BPF|HPF|NoF|APF|MoL"   },
	{ 0, "InGain",  VCTL_SLIDER, 32,   0, 127, 1, 0															},
	{ 0, "Param 1", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 0, "Param 2", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	// Chorus/Flanger (7)
	{ 0, "Amount",  VCTL_SLIDER, 64,   0, 127, 1, 0															},
	{ 0, "FeedBk",	VCTL_SLIDER, 64,   0, 127, 1, 0															},
	{ 0, "Delay L", VCTL_SLIDER,  0,   1, 127, 1, 0															},
	{ 0, "Delay R", VCTL_SLIDER,  0,   1, 127, 1, 0															},
	{ 0, "M. Rate", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 0, "M.Depth", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 0, "M.Phase", VCTL_SLIDER, 64,   0, 127, 1, 0															},
	// Compressor (9)
	{ 1, "Mode",    VCTL_MB    ,  0,   0,   2, 0, "!Off|Peak|RMS"								},
	{ 1, "Couple",  VCTL_MB    ,  0,   0,   1, 0, "!Mono|Stereo"					  			},
	{ 1, "AutoGain",VCTL_MB    ,  0,   0,   1, 0, "!Off|On"								 			},
	{ 1, "LkAhead", VCTL_SLIDER,  0,   0,  10, 1, 0															},
	{ 1, "Threshd", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 1, "Ratio",	  VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 1, "Attack",	VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 1, "Release", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 1, "OutGain", VCTL_SLIDER, 64,   0, 127, 1, 0															},
	//  Polyphony (1) 
	{ 0, "MaxPoly", VCTL_SLIDER,  0,   1,  16, 0, 0															},
};

const int v2nparms = sizeof(v2parms)/sizeof(V2PARAM);
// patch size
const int v2soundsize=v2nparms+1+255*3;

////////////////////////////////////////////
//
// V2 Initial Patch Parameter Setup
//
////////////////////////////////////////////

const unsigned char v2initsnd[v2soundsize] =
{
	  64,                          // Panning
	  64,                          // Transpose
    1,  0, 64,  64,   0, 127,       // osc1    : simple sawtooth
		0,  0, 64,  64,  32, 127,       // osc2    : off
		0,  0, 64,  64,  32, 127,       // osc3    : off
		1, 127,   0,                 // vcf1    : low pass, open
		0,	64,   0,                 // vcf2    : off
    0,                           // routing : single
    64,                          // filter balance: mid
		0,  32,   0,   64,            // VoiceDis: off
		0,  64, 127,  64,  80,   0,  // env1    : pling
		0,  64, 127,  64,  80,  64,  // env2    : pling
		1,   1,   0,  64,   2, 0,   0,  // lfo1    : defaultbla
		1,   1,   0,  64,   2, 0, 127,  // lfo2    : defaultbla
		0,                           // oscsync
		0,			                     // chanvol
    0, 0,                        // aux a/b recv
    0, 0,                        // aux a/b send
		0, 0,										     // aux 1/2 sends
		0,                           // FXRoute
		0,                           // Boost
		0,  32,  100,   64,            // ChanDist: off  
		64, 64, 32, 32, 0, 0, 64,    // Chorus/Flanger: off
		0, 0, 1, 2, 90, 32, 20, 64, 64,		// Compressor: off
		1,                           // maxpoly : 1
		4,                           // mods    : 4
		0, 127,  37,                 // velocity -> aenv ampl
		1, 127,  50,                 // modulation -> lfo1 ampl
	 10,	65,   1,                 // lfo1 -> txpose
	  7, 127,  59,                 // ctl7 (volume) -> chanvol
	  0,                           // rest of mods
};

////////////////////////////////////////////
//
// V2 Global Topics
//
////////////////////////////////////////////

const V2TOPIC v2gtopics[] = {
	{  4, "Reverb","Rv" },
	{  7, "Stereo Delay","SD" },
//FICKEN	{  9, "Parametric EQ","EQ" },
	{  2, "Post Filters","Fi" },
	{  9, "Sum Compressor","SC" },
	{  1, "Gui Features","GF" },
};
const int v2ngtopics = sizeof(v2gtopics)/sizeof(V2TOPIC);

////////////////////////////////////////////
//
// V2 Global Parameters
//
////////////////////////////////////////////

const V2PARAM v2gparms[] = {
  // 00: Reverb
	{ 0, "Time" ,   VCTL_SLIDER,  0,   0, 127, 1, 0		    	  									},
	{ 0, "HighCut", VCTL_SLIDER,  0,   0, 127, 1, 0						  								},
	{ 4, "LowCut",  VCTL_SLIDER,  0,   0, 127, 1, 0							  							},
	{ 0, "Volume",  VCTL_SLIDER,  0,   0, 127, 1, 0							  							},
	// 03: Delay
	{ 0, "Volume",  VCTL_SLIDER, 64,   0, 127, 1, 0															},
	{ 0, "FeedBk",	VCTL_SLIDER, 64,   0, 127, 1, 0															},
	{ 0, "Delay L", VCTL_SLIDER,  0,   1, 127, 1, 0															},
	{ 0, "Delay R", VCTL_SLIDER,  0,   1, 127, 1, 0															},
	{ 0, "M. Rate", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 0, "M.Depth", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 0, "M.Phase", VCTL_SLIDER, 64,   0, 127, 1, 0															},
	// EQ
  //FICKEN{ 6, "Gain 1",  VCTL_SLIDER, 64,   0, 127, 1, 0														  },
	//FICKEN{ 6, "Freq 1",  VCTL_SLIDER,  0,   0, 127, 1, 0														  },
	//FICKEN{ 6, "Q 1",     VCTL_SLIDER,  0,   1, 127, 129, 0														  },
	//FICKEN{ 6, "Gain 2",  VCTL_SLIDER, 64,   0, 127, 1, 0														  },
	//FICKEN{ 6, "Freq 2",  VCTL_SLIDER,  0,   0, 127, 1, 0														  },
	//FICKEN{ 6, "Q 2",     VCTL_SLIDER,  0,   1, 127, 129, 0														  },
	//FICKEN{ 6, "Gain 3",  VCTL_SLIDER, 64,   0, 127, 1, 0														  },
	//FICKEN{ 6, "Freq 3",  VCTL_SLIDER,  0,   0, 127, 1, 0														  },
	//FICKEN{ 6, "Q 3",     VCTL_SLIDER,  0,   1, 127, 129, 0														  },
	// 10: PostProcessing
	{ 0, "Low Cut", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 0, "HighCut", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	// 12: Compressor
	{ 1, "Mode",    VCTL_MB    ,  0,   0,   2, 0, "!Off|Peak|RMS"								},
	{ 1, "Couple",  VCTL_MB    ,  0,   0,   1, 0, "!Mono|Stereo"					  			},
	{ 1, "AutoGain",VCTL_MB    ,  0,   0,   1, 0, "!Off|On"								 			},
	{ 1, "LkAhead", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 1, "Threshd", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 1, "Ratio",	  VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 1, "Attack",	VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 1, "Release", VCTL_SLIDER,  0,   0, 127, 1, 0															},
	{ 1, "OutGain", VCTL_SLIDER, 64,   0, 127, 1, 0															},
  // gui features (deprecated)
	{ 6, "",VCTL_SKIP,  0,   0,   3, 0, ""        }, // mystery parameter: we dont go to ravenholm
};

const int v2ngparms = sizeof(v2gparms)/sizeof(V2PARAM);

////////////////////////////////////////////
//
// V2 Initial Global Parameter Setup
//
////////////////////////////////////////////

const unsigned char v2initglobs[v2ngparms] =
{
		64,  64, 32, 127,						  		// Reverb
		100, 80, 64, 64, 0, 0, 64,				// Delay
//FICKEN		64, 64,64,64,64,64,64,64,64,      // EQ
		0, 127,														// lc/hc
		0, 0, 1, 2, 90, 32, 20, 64, 64,		// Compressor
    0                                 // gui color
};

// total sound memory size
const int smsize=128*sizeof(void*) + 128*v2soundsize;

/*
extern const V2TOPIC v2topics[];
extern const int v2ntopics;
extern const char *v2sources[];
extern const int v2nsources;
extern const V2PARAM v2parms[];
extern const int v2nparms;
extern const unsigned char v2initsnd[];
extern const int v2soundsize;

extern const V2TOPIC v2gtopics[];
extern const int v2ngtopics;
extern const V2PARAM v2gparms[];
extern const int v2ngparms;
extern const unsigned char v2initglobs[];

extern const int     smsize;
*/

extern int	 *v2topics2;

extern int	 *v2gtopics2;

extern unsigned char *soundmem;
extern long          *patchoffsets;
extern unsigned char *editmem;
extern char					 patchnames [128][32];
extern char					 globals[];

extern int           v2version;
extern int           *v2vsizes;
extern int           *v2gsizes;
extern int					 v2curpatch;

#ifdef RONAN
extern char          speech[64][256];
extern char          *speechptrs[64];
#endif

extern void sdInit();
extern void sdClose();
extern sBool sdSaveBank(file &out);
extern sBool sdSavePatch(file &out);
extern sBool sdLoad(file &in);
extern sBool sdImportV2MPatches(file &in, const char *prefix);
extern void sdCopyPatch();
extern void sdPastePatch();
extern void sdInitPatch();

#endif

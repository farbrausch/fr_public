//#include "stdafx.h"

#include "types.h"
#include "sounddef.h"
#include <string.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

const V2TOPIC v2topics[] = {
	{  2, "Voice" },
	{  6, "Osc 1" },
	{  6, "Osc 2" },
	{  6, "Osc 3" },
	{  3, "VCF 1" },
	{  3, "VCF 2" },
	{  2, "Filters" },
	{  4, "Voice Dist" },
	{  6, "Amp EG" },
	{  6, "EG 2" },
	{  7, "LFO 1" },
	{  7, "LFO 2" },
	{  6, "Global" },
	{  4, "Channel Dist" },
	{  7, "Chorus/Flanger" },
	{  9, "Compressor" },
	{  1, "Polyphony" },
};
const int v2ntopics = sizeof(v2topics)/sizeof(V2TOPIC);



const char *v2sources[] = {
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
};
const int v2nsources = sizeof(v2sources)/sizeof(char *);


const V2PARAM v2parms[] = {
	// Voice
	{ 0, "Panning", VCTL_SLIDER, 64, 127, 1, 0		    											},
	{ 2, "Txpose",  VCTL_SLIDER, 64, 127, 1, 0		    											},
  // Osc 1
	{ 0, "Mode"  ,  VCTL_MB    ,  0,   4, 0, "Off|Saw/Tri|Pulse|Sin|Noise"	},
	{ 2, "Ringmod", VCTL_SKIP  ,  0,   1, 0, ""			      									},
	{ 0, "Txpose",  VCTL_SLIDER, 64, 127, 1, 0		    											},
	{ 0, "Detune",  VCTL_SLIDER, 64, 127, 1, 0															},
	{ 0, "Color",   VCTL_SLIDER, 64, 127, 1, 0															},
	{ 0, "Volume",  VCTL_SLIDER,  0, 127, 1, 0	                            },
  // Osc 2
	{ 0, "Mode"  ,  VCTL_MB    ,  0,   5, 0, "!Off|Tri|Pul|Sin|Noi|FM"			},
	{ 2, "RingMod", VCTL_MB    ,  0,   1, 0, "Off|On"					      				},
	{ 0, "Txpose" , VCTL_SLIDER, 64, 127, 1, 0															},
	{ 0, "Detune",  VCTL_SLIDER, 64, 127, 1, 0															},
	{ 0, "Color",   VCTL_SLIDER, 64, 127, 1, 0															},
	{ 0, "Volume",  VCTL_SLIDER,  0, 127, 1, 0                              },
  // Osc 3
	{ 0, "Mode"  ,  VCTL_MB    ,  0,   5, 0, "!Off|Tri|Pul|Sin|Noi|FM"			},
	{ 2, "RingMod", VCTL_MB    ,  0,   1, 0, "Off|On"												},
	{ 0, "Txpose",  VCTL_SLIDER, 64, 127, 1, 0															},
	{ 0, "Detune",  VCTL_SLIDER, 64, 127, 1, 0															},
	{ 0, "Color",   VCTL_SLIDER, 64, 127, 1, 0															},
	{ 0, "Volume",  VCTL_SLIDER,  0, 127, 1, 0                              },
  // VCF 1
	{ 0, "Mode",    VCTL_MB    ,  0,   5, 0, "Off|Low|Band|High|Notch|All"  },
	{ 0, "Cutoff",  VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "Reso",	  VCTL_SLIDER,  0, 127, 1, 0															},
  // VCF 2
	{ 0, "Mode",    VCTL_MB    ,  0,   5, 0, "Off|Low|Band|High|Notch|All"  },
	{ 0, "Cutoff",  VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "Reso",		VCTL_SLIDER,  0, 127, 1, 0															},
	// Routing
	{ 0, "Routing", VCTL_MB    ,  0,   2, 0, "!single|serial|parallel"			  },
	{ 3, "Balance", VCTL_SLIDER, 64, 127, 1, 0														  },
	// Distortion
	{ 0, "Mode",    VCTL_MB    ,  0,   8, 0, "Off|OD|Clip|Crush|Dec|LPF|BPF|HPF|NoF|APF"   },
	{ 0, "InGain",  VCTL_SLIDER, 32, 127, 1, 0															},
	{ 0, "Param 1", VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "Param 2", VCTL_SLIDER,  0, 127, 1, 0															},
	// Amp Envelope
	{ 0, "Attack",  VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "Decay",   VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "Sustain", VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "SusTime", VCTL_SLIDER, 64, 127, 1, 0															},
	{ 0, "Release", VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "Amplify", VCTL_SLIDER,  0, 127, 1, 0															},
	// Envelope 2
	{ 0, "Attack",  VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "Decay",   VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "Sustain", VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "SusTime", VCTL_SLIDER, 64, 127, 1, 0															},
	{ 0, "Release", VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "Amplify", VCTL_SLIDER,  0, 127, 1, 0															},
	// LFO 1
	{ 0, "Mode"  ,  VCTL_MB    ,  0,   4, 0, "Saw|Tri|Pulse|Sin|S+H"				},
	{ 0, "KeySync", VCTL_MB    ,  0,   2, 0, "!Off|On"											},
	{ 0, "EnvMode", VCTL_MB    ,  0,   2, 0, "!Off|On"											},
	{ 0, "Rate",		VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "Phase",   VCTL_SLIDER,  0, 127, 1, 0															},
	{ 5, "Polarity",VCTL_MB,      0,   2, 0, "!+|-|±"                       },
	{ 0, "Amplify", VCTL_SLIDER,  0, 127, 1, 0															},
	// LFO 2
	{ 0, "Mode"  ,  VCTL_MB    ,  0,   4, 0, "Saw|Tri|Pulse|Sin|S+H"				},
	{ 0, "KeySync", VCTL_MB    ,  0,   2, 0, "!Off|On"											},
	{ 0, "EnvMode", VCTL_MB    ,  0,   2, 0, "!Off|On"											},
	{ 0, "Rate",		VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "Phase",   VCTL_SLIDER,  0, 127, 1, 0															},
	{ 5, "Polarity",VCTL_MB,      0,   2, 0, "!+|-|±"                       },
	{ 0, "Amplify", VCTL_SLIDER,  0, 127, 1, 0															},
	// Globals 
	{ 0, "OscSync", VCTL_MB    ,  0,   1, 0, "!Off|On"											},
	{ 0, "ChanVol", VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "Reverb",	VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "Delay",   VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "FXRoute", VCTL_MB    ,  0,   1, 0, "!D -> C|C -> D"								},
	{ 1, "Boost",	  VCTL_SLIDER,  0, 127, 1, 0															},
	// Channel Dist
	{ 0, "Mode",    VCTL_MB    ,  0,   8, 0, "Off|OD|Clip|Crush|Dec|LPF|BPF|HPF|NoF|APF"   },
	{ 0, "InGain",  VCTL_SLIDER, 32, 127, 1, 0															},
	{ 0, "Param 1", VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "Param 2", VCTL_SLIDER,  0, 127, 1, 0															},
	// Chorus/Flanger
	{ 0, "Amount",  VCTL_SLIDER, 64, 127, 1, 0															},
	{ 0, "FeedBk",	VCTL_SLIDER, 64, 127, 1, 0															},
	{ 0, "Delay L", VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "Delay R", VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "M. Rate", VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "M.Depth", VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "M.Phase", VCTL_SLIDER, 64, 127, 1, 0															},
	// Compressor
	{ 1, "Mode",    VCTL_MB    ,  0,   2, 0, "!Off|Peak|RMS"								},
	{ 1, "Couple",  VCTL_MB    ,  0,   1, 0, "!Mono|Stereo"					  			},
	{ 1, "AutoGain",VCTL_MB    ,  0,   1, 0, "!Off|On"								 			},
	{ 1, "LkAhead", VCTL_SLIDER,  0,  10, 1, 0															},
	{ 1, "Threshd", VCTL_SLIDER,  0, 127, 1, 0															},
	{ 1, "Ratio",	  VCTL_SLIDER,  0, 127, 1, 0															},
	{ 1, "Attack",	VCTL_SLIDER,  0, 127, 1, 0															},
	{ 1, "Release", VCTL_SLIDER,  0, 127, 1, 0															},
	{ 1, "OutGain", VCTL_SLIDER, 64, 127, 1, 0															},
	//  Polyphony
	{ 0, "MaxPoly", VCTL_SLIDER,  0,  16, 0, 0															},
};

const int v2nparms = sizeof(v2parms)/sizeof(V2PARAM);
const int v2soundsize=v2nparms+1+255*3;

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
		0,  32,   0,   0,            // VoiceDis: off
		0,  64,  64,  64,  64,   0,  // env1    : pling
		0,  64,  64,  64,  64,  64,  // env2    : pling
		1,   1,   0,  64,   2, 0,   0,  // lfo1    : defaultbla
		1,   1,   0,  64,   2, 0, 127,  // lfo2    : defaultbla
		0,                           // oscsync
		0,			                     // chanvol
		2, 0,										     // aux sends
		0,                           // FXRoute
		0,                           // Boost
		0,  32,   0,   0,            // ChanDist: off
		64, 64, 32, 32, 0, 0, 64,    // Chorus/Flanger: off
		0, 0, 1, 2, 90, 32, 20, 64, 64,		// Compressor: off
		4,                           // maxpoly : 4
		4,                           // mods    : 4
		0, 127,  37,                 // velocity -> aenv ampl
		1, 127,  50,                 // modulation -> lfo1 ampl
	 10,	65,   1,                 // lfo1 -> txpose
	  7, 127,  59,                 // ctl7 (volume) -> chanvol
	  0,                           // rest of mods
};



const V2TOPIC v2gtopics[] = {
	{  4, "Reverb" },
	{  7, "Stereo Delay" },
	{  2, "Post Filters" },
	{  9, "Sum Compressor" },
};
const int v2ngtopics = sizeof(v2gtopics)/sizeof(V2TOPIC);

const V2PARAM v2gparms[] = {
  // 00: Reverb
	{ 0, "Time" ,   VCTL_SLIDER,  0, 127, 1, 0		    	  									},
	{ 0, "HighCut", VCTL_SLIDER,  0, 127, 1, 0						  								},
	{ 4, "LowCut",  VCTL_SLIDER,  0, 127, 1, 0							  							},
	{ 0, "Volume",  VCTL_SLIDER,  0, 127, 1, 0							  							},
	// 03: Delay
	{ 0, "Volume",  VCTL_SLIDER, 64, 127, 1, 0															},
	{ 0, "FeedBk",	 VCTL_SLIDER, 64, 127, 1, 0															},
	{ 0, "Delay L", VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "Delay R", VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "M. Rate", VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "M.Depth", VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "M.Phase", VCTL_SLIDER, 64, 127, 1, 0															},
	// 10: PostProcessing
	{ 0, "Low Cut", VCTL_SLIDER,  0, 127, 1, 0															},
	{ 0, "HighCut", VCTL_SLIDER,  0, 127, 1, 0															},
	// 12: Compressor
	{ 1, "Mode",    VCTL_MB    ,  0,   2, 0, "!Off|Peak|RMS"								},
	{ 1, "Couple",  VCTL_MB    ,  0,   1, 0, "!Mono|Stereo"					  			},
	{ 1, "AutoGain",VCTL_MB    ,  0,   1, 0, "!Off|On"								 			},
	{ 1, "LkAhead", VCTL_SLIDER,  0, 127, 1, 0															},
	{ 1, "Threshd", VCTL_SLIDER,  0, 127, 1, 0															},
	{ 1, "Ratio",	  VCTL_SLIDER,  0, 127, 1, 0															},
	{ 1, "Attack",	VCTL_SLIDER,  0, 127, 1, 0															},
	{ 1, "Release", VCTL_SLIDER,  0, 127, 1, 0															},
	{ 1, "OutGain", VCTL_SLIDER, 64, 127, 1, 0															},
};

const int v2ngparms = sizeof(v2gparms)/sizeof(V2PARAM);


const unsigned char v2initglobs[v2ngparms] =
{
		64,  64, 32, 127,						  	// Reverb
		100, 80, 64, 64, 0, 0, 64,				// Delay
		0, 127,														// lc/hc
		0, 0, 1, 2, 90, 32, 20, 64, 64,		// Compressor
};


unsigned char *soundmem;
long          *patchoffsets;
unsigned char *editmem;
const int     smsize=128*v2soundsize + 128*4;
char					patchnames [128][32];
char          globals[v2ngparms];
int           v2version;
int           *v2vsizes;
int           *v2gsizes;
int           *v2topics2;
int           *v2gtopics2;

void sdInit()
{
	char s[256];
  soundmem = new unsigned char [smsize+v2soundsize];
	patchoffsets = (long *)soundmem;
	unsigned char *sptr=soundmem+128*4;

	sprintf(s,"sound size: %d\n",v2nparms);
	OutputDebugString(s);

	for (int i=0; i<129; i++)
	{
		if (i<128)
		{
			patchoffsets[i]=(long)(sptr-soundmem);
			sprintf(s,"Init Patch #%03d",i);
			strcpy(patchnames[i],s);
		}
		else 
			editmem=sptr;
		memcpy(sptr,v2initsnd,v2soundsize);
		sptr+=v2soundsize;
	}
	for (int i=0; i<v2ngparms; i++)
		globals[i]=v2initglobs[i];

	// init version control
	v2version=0;
	for (int i=0; i<v2nparms; i++)
		if (v2parms[i].version>v2version) v2version=v2parms[i].version;
	for (int i=0; i<v2ngparms; i++)
		if (v2gparms[i].version>v2version) v2version=v2gparms[i].version;

	v2vsizes = new int[v2version+1];
	v2gsizes = new int[v2version+1];
	memset(v2vsizes,0,(v2version+1)*sizeof(int));
	memset(v2gsizes,0,(v2version+1)*sizeof(int));
	for (int i=0; i<v2nparms; i++)
	{
		const V2PARAM &p=v2parms[i];
//		ATLASSERT(p.version<=v2version);
		for (int j=v2version; j>=p.version; j--)
			v2vsizes[j]++;
	}
	for (int i=0; i<v2ngparms; i++)
	{
		const V2PARAM &p=v2gparms[i];
		//ATLASSERT(p.version<=v2version);
		for (int j=v2version; j>=p.version; j--)
			v2gsizes[j]++;
	}
//ATLASSERT(v2vsizes[v2version]==v2nparms);

	for (int i=0; i<=v2version; i++)
	{
		sprintf(s,"size of version %d sound bank: %d params, %d globals\n",i,v2vsizes[i],v2gsizes[i]);
		v2vsizes[i]+=1+255*3;
		OutputDebugString(s);
	}

	v2topics2=new int[v2ntopics];
	int p=0;
	for (int i=0; i<v2ntopics; i++)
	{
		v2topics2[i]=p;
		p+=v2topics[i].no;
	}


	v2gtopics2=new int[v2ngtopics];
	p=0;
	for (int i=0; i<v2ngtopics; i++)
	{
		v2gtopics2[i]=p;
		p+=v2gtopics[i].no;
	}

}

void sdClose()
{
	delete soundmem;
	delete v2vsizes;
	delete v2topics2;
	delete v2gtopics2;
}
#include "../types.h"
#include "../synth.h"
#include "../dsio.h"
#include "MidiInterface.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define MI_SAMPLERATE 44100

namespace
{
  static sU8  g_synth[3*1024*1024];

	static sU8  g_bankdata[256*1024];
	static sU8  g_globals[1024];

	static sU8  g_midibuf[1024];
	static sU32 g_mbsize;

	static MITIMERCALLBACK g_cbfunc;
	static void * g_cbparm;
	static sU32 g_cbrate;

	static bool LoadV2B(const void *v2bdata)
	////////////////////////////////////////
	{
		const sU8 *p=(const sU8*)v2bdata;
		
		// read bank tag
		sU32 tag=*(sU32*)p; p+=4;
		if (tag!='0p2v') 		
		{
			OutputDebugString("V2 ERROR: bank not recognized!\n");
			return false;
		}
		
		// skip patch names
		p+=128*32;

		// bank size
		sU32 banksize=*(sU32*)p; p+=4;
    sU32 patchsize=banksize/128;

		// fill in patch offsets
		sU32 *offptr=(sU32*)g_bankdata;
		sU8  *bankptr=(sU8*)(offptr+128);
		for (sU32 i=0; i<128; i++) *(offptr++)=sU32(bankptr-g_bankdata)+patchsize*i;

		// copy bank data (no, we don't have memcpy)
		for (sU32 i=0; i<banksize; i++) *bankptr++=*p++;

		// globals size
		sU32 globsize=*(sU32*)p; p+=4;

		// copy globals
		sU8 *gp=g_globals;
		for (sU32 i=0; i<globsize; i++) *gp++=*p++;

		return true;
	}

	static sU32 cbrleft;
	static void __stdcall RenderFunc(void *, sF32 *buf, sU32 len)
	/////////////////////////////////////////////////////////////
	{
		// fragmentation loop
		while (len)
		{
			// check if we have to call the callback function
			if (cbrleft==0)
			{
				if (g_cbfunc) g_cbfunc(g_cbparm);
				cbrleft=g_cbrate?g_cbrate:0xffffffff;
			}

			// check if there is pending midi data
			if (g_mbsize) synthProcessMIDI(g_synth,g_midibuf);
			g_mbsize=0; g_midibuf[0]=0xfd;

			// render all remaining samples
			sU32 torender=sMin(len,cbrleft);

			if (torender)
				synthRender(g_synth,buf,torender);

			buf+=2*torender;
			len-=torender;
			cbrleft-=torender;
		}
	}
	
}


bool __stdcall miInit(const void *v2bdata, MITIMERCALLBACK callback, void *cbparm, void *hwnd)
//////////////////////////////////////////////////////////////////////////////
{
	// load/convert bank data
	if (!LoadV2B(v2bdata)) return false;

	// init synthesizer
	synthInit(g_synth,g_bankdata,MI_SAMPLERATE);
	synthSetGlobals(g_synth,g_globals);
	g_mbsize=0; g_midibuf[0]=0xfd;

	// init callback
	g_cbfunc=callback;
	g_cbparm=cbparm;
	miSetBPM(125,16); // because of Protracker!
	cbrleft=0;

	// init DSIO
	if (!dsInit(RenderFunc,0,hwnd))
	{
		OutputDebugString("V2 ERROR: could not init DirectSound output!\n");
		return false;
	}
	dsSetVolume(1.0f);

	return true;
}



void __stdcall miClose()
//////////////
{
	// simply stopping dsio is enough
	dsClose();
}



void __stdcall miSetBPM(int bpm, int fraction)
////////////////////////////////////
{
	// calc samples/tick
	sF32 ticksperminute=sF32(bpm)*sF32(fraction)/4.0f;
	g_cbrate=sU32(MI_SAMPLERATE*60.0f/ticksperminute + 0.5f);
	cbrleft=sMin(g_cbrate,cbrleft);
}



void __stdcall miSendMIDIData(const unsigned char *data, unsigned long len)
///////////////////////////////////////////////////////////
{
	sU32 reallen=sMin(1023-g_mbsize,len);
	for (sU32 i=0; i<reallen; i++) g_midibuf[g_mbsize++]=*data++;
	g_midibuf[g_mbsize]=0xfd;
}



void __stdcall miTick()
/////////////
{
  dsTick();
}

#pragma warning(disable:4244) // conversion from bigger to smaller

#include "stdafx.h"
#include "../types.h"
#include "../tool/file.h"
#include "midi.h"
#include "../dsio.h"
#include "../synth.h"
#include "../samples.h"
#include <math.h>
#include "v2mrecorder.h"

char msdevnames[256][256];
int  msnumdevs;

static int msusedev=-1;
static HMIDIIN usedev=0;

static CRITICAL_SECTION cs;

static LARGE_INTEGER pfreq;

static sU8 midibuf[65536];
static int mbuflen;
static int lastoffs;
static sU32 *frameptr;

static float pleft, pright;
static int clipl, clipr;
static int mscpu;
static int xlatepp;

static CV2MRecorder *recorder=0;
static int rectime;

static CV2MRecorder* lastRecorder = 0;

sU8 *theSynth=0;

void msInit()
{
	msusedev=-1;
	mscpu=0;
	InitializeCriticalSection(&cs);
	pleft=pright=0;
	clipl=clipr=0;
	xlatepp=1;

	lastoffs=0;
	*(sU32*)midibuf=lastoffs;
	frameptr=(sU32*)(midibuf+4);
	mbuflen=8;
	
	midibuf[mbuflen]=0xFD;

	QueryPerformanceFrequency(&pfreq);

	recorder=0;

  theSynth = new sU8[3*1024*1024];

}

void msClose()
{
  //V2SamplerExit();
  delete theSynth;
  theSynth=0;
	delete recorder;
	DeleteCriticalSection(&cs);
}

void msProcessEvent(DWORD offs, DWORD dwParam1)
{
	EnterCriticalSection(&cs);
	if (mbuflen<65500)
	{
		if ((int)offs<lastoffs)
		{
			LeaveCriticalSection(&cs);
			return;
		}

		if (offs!=lastoffs)
		{
			midibuf[mbuflen++]=0xfd;
			*frameptr=(midibuf+mbuflen-((sU8*)frameptr)-4);
			*(sU32*)(midibuf+mbuflen)=lastoffs=offs;
			frameptr=(sU32*)(midibuf+mbuflen+4);
			mbuflen+=8;
		}

		sU8 b0=(sU8)(dwParam1&0xff);
		sU8 b1=(sU8)((dwParam1>>8)&0xff);
		sU8 b2=(sU8)((dwParam1>>16)&0xff);

#ifdef SINGLECHN
		// lock to channel 1
		if (b0<0xf0) b0&=0xf0;
#endif

		// translate poly pressure to control change if necessary
		if ((b0>>4)==0xa && xlatepp)
			b0=(b0&0x0f)|0xb0;

		if (recorder) recorder->AddEvent(rectime+offs,b0,b1,b2);
	
		switch (b0>>4)
		{
		case 0x8: //noteoff
		case 0x9: //noteon
		case 0xa: //poly pressure
		case 0xb: //ctlchange
		case 0xe: //pitch
			midibuf[mbuflen++]=b0;
			midibuf[mbuflen++]=b1;
			midibuf[mbuflen++]=b2;
			break;
		case 0xc: //pgmchange
		case 0xd: //chn pressure
			midibuf[mbuflen++]=b0;
			midibuf[mbuflen++]=b1;
			break;
		case 0xf: //realtime;
			printf("%02x",b0);
			break;
		default:
			printf("- got msg %02x %02x %02x\n",b0,b1,b2);
		}


	}
	LeaveCriticalSection(&cs);
}

void msSetProgram1(int p)
{
#ifdef SINGLECHN
	EnterCriticalSection(&cs);
	unsigned char mb2[4];
	mb2[0]=0xc0;
	mb2[1]=p&0x7f;
	mb2[2]=0xfd;
	synthProcessMIDI(theSynth,mb2);
	LeaveCriticalSection(&cs);
#endif
}

extern "C" extern unsigned long* __stdcall  ficken();

void __stdcall msAudioCallback(float *left, float *right, unsigned int len, int mixwithsource)
{
	LARGE_INTEGER ta,tb;
	QueryPerformanceCounter(&ta);

	EnterCriticalSection(&cs);

	sUInt offs=0;
	sInt mboffs=0;

	midibuf[mbuflen++]=0xfd;
	*frameptr=(midibuf+mbuflen-((sU8*)frameptr)-4);

	while (mboffs<mbuflen || offs<len)
	{
		// get next pos of midi data from buffer
		sU32 newoffs;
		sU32 tlen;
		if (mboffs<mbuflen)
		{
			newoffs=*(sU32*)(midibuf+mboffs);
			mboffs+=4;
			tlen=*(sU32*)(midibuf+mboffs);
			mboffs+=4;
		}
		else
		{
			newoffs=len;
			tlen=0;
		}

		// render...
		if (newoffs>offs)
    {
      /*
      static int bla=0;
      if ((bla++)<3)
      {
        printf("render\n");
        unsigned long *p=ficken();
        printf("%08x %08x %08x %08x %08x %08x",p[0],p[1],p[2],p[3],p[4],p[5]);
      }

      static sF32 test[65536];
      */

			synthRender(theSynth,left+offs,newoffs-offs,right+offs,mixwithsource);
//			synthRender(theSynth,test,newoffs-offs);
    }
		offs=newoffs;

		// process MIDI
		if (tlen)
    {
			synthProcessMIDI(theSynth,midibuf+mboffs);
    }
		mboffs+=tlen;
	}

	lastoffs=0;
	*(sU32*)midibuf=lastoffs;
	frameptr=(sU32*)(midibuf+4);
	mbuflen=8;

	LeaveCriticalSection(&cs);

	QueryPerformanceCounter(&tb);
	tb.QuadPart=44100*(tb.QuadPart-ta.QuadPart)/pfreq.QuadPart;
	mscpu=(int)(100*tb.QuadPart/len);

	rectime+=len;
	
	// peak level meter
	for (unsigned int i=0; i<len; i++)
	{
		float l=fabsf(left[i]);
		pleft*=0.99985f;
		if (l>pleft) pleft=l;
		if (l>1.0f) clipl=1;
		float r=fabsf(right[i]);
		pright*=0.99985f;
		if (r>pright) pright=r;
		if (r>1.0f) clipr=1;
	}

}

void msGetLD(float *l, float *r, int *cl, int *cr)
{
	if (l) *l=pleft;
	if (r) *r=pright;
	if (cl) *cl=clipl;
	if (cr) *cr=clipr;
	clipl=clipr=0;
}


int msGetCPUUsage()
{
	return mscpu;
}

static int msSampleRate = 44100;

#ifdef RONAN
void msSetSampleRate(int newrate,void *patchmap, void *globals, const char **spp) {
#else
void msSetSampleRate(int newrate,void *patchmap, void *globals) {
#endif
	if(msSampleRate != newrate) {
		msStopAudio();
		msSampleRate = newrate;
#ifdef RONAN
		msStartAudio(0,patchmap,globals, spp);
#else
		msStartAudio(0,patchmap,globals);
#endif
	}
}


#ifdef RONAN
void msStartAudio(HWND hwnd, void *patchmap, void *globals, const char **speech)
#else
void msStartAudio(HWND hwnd, void *patchmap, void *globals)
#endif
{
	EnterCriticalSection(&cs);
  printf("synthinit %08x %08x %d\n",theSynth,patchmap,msSampleRate);
	synthInit(theSynth,patchmap,msSampleRate);
	synthSetGlobals(theSynth,globals);
  //V2SamplerInit(theSynth);

#ifdef RONAN
	synthSetLyrics(theSynth,speech);
#endif
  LeaveCriticalSection(&cs);
}

void msStopAudio()
{
}


void msStartRecord()
{
	if (lastRecorder)
	{
		delete lastRecorder;
		lastRecorder = 0;
	}
	if (recorder)
	{
		delete recorder;
	}
	recorder=new CV2MRecorder(synthGetFrameSize(theSynth),msSampleRate);

	// send program change events for currently used programs
	int pg[16];
	synthGetPgm(theSynth,pg);
	for (sU32 i=0; i<16; i++) recorder->AddEvent(0,0xc0|i,pg[i],0);

	rectime=0;
	
}


int msIsRecording() { return recorder!=0; }

int msStopRecord(file &f)
{
	if (recorder)
	{
		int res=recorder->Export(f);
		delete recorder;
		recorder=0;
		return res;
	}
	else 
	{
		return 0;
	}
}

// i am jacks fixed source code

void msStopRecord()
{
	if (recorder)
	{
		lastRecorder = recorder;
		recorder=0;
	}
}

int msWriteLastRecord(file &f)
{
	if (lastRecorder)
	{
		int res = lastRecorder->Export(f);
		delete lastRecorder;
		lastRecorder = 0;
		return res;
	}
	else
	{
		return 0;
	}
}

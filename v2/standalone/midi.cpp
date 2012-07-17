
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include <windows.h>
#include <mmsystem.h>

#include "atlstr.h"

#include "../types.h"
#include "../libv2.h"
#include "midi.h"
#include "cstring.h"

char msdevnames[256][256];
int  msnumdevs;

static int msusedev=-1;
static HMIDIIN usedev=0;

static CRITICAL_SECTION cs;

static LARGE_INTEGER pfreq;

static sU8 midibuf[65536];
static int mbuflen;

static float pleft, pright;
static int clipl, clipr;
static int mscpu;

sU8 v2instance[3*1024*1024];

void msInit()
{
  int i;
	CString s;
	MIDIINCAPS mc;
	msusedev=-1;
	mscpu=0;

	OutputDebugString("MIDI init...\n");
	msnumdevs=midiInGetNumDevs();
	s.Format("- found %d devices\n",msnumdevs);
	OutputDebugString(s);
	for (int i=0; i<msnumdevs; i++)
	{
		midiInGetDevCaps(i,&mc,sizeof(mc));
		s.Format("device %d is '%s'\n",i,mc.szPname);
		OutputDebugString(s);
		strcpy_s(msdevnames[i],256,mc.szPname);
	}
	InitializeCriticalSection(&cs);
	mbuflen=0;

	for (i=0; i<msnumdevs; i++)
		if (!strncmp(msdevnames[i],"LB",2) ||
			  !strncmp(msdevnames[i],"MIDI Yoke",9) ||
        !strncmp(msdevnames[i],"LoopBe Internal MIDI", 20))
		{
			msSetDevice(i);
			break;
		}

	pleft=pright=0;
	clipl=clipr=0;

	QueryPerformanceFrequency(&pfreq);

}

void msClose()
{
	msSetDevice(-1);
	DeleteCriticalSection(&cs);
}


static void CALLBACK msCallBack(HMIDIIN hMidiIn, UINT wMsg, DWORD dwInstance, DWORD dwParam1,  DWORD dwParam2)
{
	sInt oldlen=mbuflen;
	switch (wMsg)
	{
		case MIM_CLOSE:
			OutputDebugString("- MIM_CLOSE rec'd\n");
			break;
		case MIM_DATA:
			EnterCriticalSection(&cs);
			if (mbuflen<65530)
			{
				sU8 b0=(sU8)(dwParam1&0xff);
				sU8 b1=(sU8)((dwParam1>>8)&0xff);
				sU8 b2=(sU8)((dwParam1>>16)&0xff);


				switch (b0>>4)
				{
				case 0x8: //noteoff
				case 0x9: //noteon
				case 0xa: //polypressure
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
					break;
				default:
					CString s;
					s.Format("- got msg %02x %02x %02x\n",b0,b1,b2);
					OutputDebugString(s);
				}
			}
			LeaveCriticalSection(&cs);
			break;
		case MIM_ERROR:
			OutputDebugString("- MIM_ERROR rec'd\n");
			break;
		case MIM_LONGDATA:
			OutputDebugString("- MIM_LONGDATA rec'd\n");
			break;
		case MIM_LONGERROR:
			OutputDebugString("- MIM_LONGERROR rec'd\n");
			break;
		case MIM_MOREDATA:
			OutputDebugString("- MIM_MOREDATA rec'd\n");
			break;
		case MIM_OPEN:
			OutputDebugString("- MIM_OPEN rec'd\n");
			break;
		default:
			OutputDebugString("- unknown message rec'd\n");
			break;
	}
	/*
	if (mbuflen>oldlen)
	{
		char buf[256];
		for (int i=0; i<(mbuflen-oldlen); i++)
		{
			sprintf(buf, "%02x ",midibuf[oldlen+i]);
			OutputDebugString(buf);
		}
		OutputDebugString("\n");
	}
	*/
}



void msSetDevice(int n)
{
	if (usedev)
	{
		midiInStop(usedev);
	  midiInClose(usedev);
		msusedev=-1;
	}
	usedev=0;
	if (n>=0 && n<msnumdevs)
	{
		CString s;
		s.Format("- using device %d\n",n);
		OutputDebugString(s);
		midiInOpen(&usedev,n,(unsigned long)msCallBack,0,CALLBACK_FUNCTION);
		midiInStart(usedev);
		msusedev=n;
	}
	else
	{
		OutputDebugString("- disabling midi in support\n");
	}
}

int msGetDevice()
{
	return msusedev;
}

extern "C" extern int FICKEN;

void __stdcall msAudioCallback(void *, float *outbuf, sU32 len)
{
	EnterCriticalSection(&cs);
	midibuf[mbuflen]=0xFD;
	synthProcessMIDI(v2instance,midibuf);
	mbuflen=0;
	LeaveCriticalSection(&cs);

	LARGE_INTEGER ta,tb;
	QueryPerformanceCounter(&ta);
	synthRender(v2instance,outbuf,len);
	QueryPerformanceCounter(&tb);
	tb.QuadPart=44100*(tb.QuadPart-ta.QuadPart)/pfreq.QuadPart;
	mscpu=(int)(100*tb.QuadPart/len);

	// peak level meter
	for (unsigned int i=0; i<len; i++)
	{
		float *b=outbuf+2*i;
		float l=(float)fabs(b[0]);
		pleft*=0.99985f;
		if (l>pleft) pleft=l;
		if (l>1.0f) clipl=1;
		float r=(float)fabs(b[1]);
		pright*=0.99985f;
		if (r>pright) pright=r;
		if (r>1.0f) clipr=1;
	}
}

void msGetLD(float *l, float *r, int *cl, int *cr)
{
	if (l) *l=pleft;
	if (r) *r=pright;

//  synthSetVUMode(1);
//	synthGetChannelVU(0,l,r);

	if (cl) *cl=clipl;
	if (cr) *cr=clipr;
	clipl=clipr=0;
}


int msGetCPUUsage()
{
	return mscpu;
}

#ifdef RONAN
void msStartAudio(HWND hwnd, void *patchmap, void *globals, const char **speech)
#else
void msStartAudio(HWND hwnd, void *patchmap, void *globals)
#endif
{
	synthInit(v2instance, patchmap, 44100);
	synthSetGlobals(v2instance, globals);
#ifdef RONAN
	synthSetLyrics(v2instance, speech);
#endif

	dsInit(msAudioCallback,0,hwnd);
	OutputDebugString("durch\n");
}

void msStopAudio()
{
	dsClose();
}

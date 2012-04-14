/*
** Example Winamp .RAW input plug-in
** Copyright (c) 1998, Justin Frankel/Nullsoft Inc.
*/

#define _CRT_SECURE_NO_DEPRECATE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>

#include "in2.h"
#include "../types.h"
#include "../dsio.h"
#include "../soundsys.h"
#include "../sounddef.h"
#include "../v2mconv.h"

#ifdef _DEBUG
#include <stdarg.h>
#include <stdio.h>
void __cdecl printf2(const char *format, ...)
{
  va_list    arg;
  char buf[4096];
  va_start(arg, format);
  vsprintf(buf, format, arg);
  va_end(arg);
  OutputDebugString(buf);
}
#endif

// avoid CRT. Evil. Big. Bloated.
BOOL WINAPI _DllMainCRTStartup(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}

// post this to the main window at end of file (after playback as stopped)
#define WM_WA_MPEG_EOF WM_USER+2

// raw configuration
#define NCH 2
#define SAMPLERATE 44100
#define BPS 16

#define WORDSIZE NCH*(BPS/8)
#define BUFSIZE 576*WORDSIZE

extern In_Module mod; // the output module (declared near the bottom of this file)

char lastfn[MAX_PATH]; // currently playing file (used for getting info on the current file)
int file_length; // file length, in bytes
int decode_pos_ms; // current decoding position, in milliseconds
int paused; // are we paused?
int seek_needed; // if != -1, it is the point that the decode thread should seek to, in ms.

float synth_buffer[576*2*2]; // synth buffer
char sample_buffer[BUFSIZE*2]; // sample buffer

HANDLE thread_handle=INVALID_HANDLE_VALUE;	// the handle to the decode thread

int killDecodeThread=0;					// the kill switch for the decode thread

DWORD WINAPI __stdcall DecodeThread(void *b); // the decode thread procedure

unsigned char *myv2m;
signed long *timeline;
int tllen;
int song_len_ms;
int myversion;

static DSIOCALLBACK *sscb;


void config(HWND hwndParent)
{
	MessageBox(hwndParent, "No configuration.", "V2M Player Configuration",MB_OK);
}



void about(HWND hwndParent)
{
	MessageBox(hwndParent,"Farbrausch V2M Player, (C) 2002 Tammo 'kb' Hinrichs\nBuild: " __DATE__ ", " __TIME__,"Farbrausch Consumer Consulting V2M Player",MB_OK);
}



void init() { 
	/* any one-time initialization goes here (configuration reading, etc) */ 
	myv2m=0;
	timeline=0;
	sdInit();
	lastfn[0]=0;
}



void quit() { 
	/* one-time deinit, such as memory freeing */ 
	delete myv2m;
	myv2m=0;
	delete timeline;
	timeline=0;
	sdClose();

}



// used for detecting URL streams.. unused here. strncmp(fn,"http://",7) to detect HTTP streams, etc
int isourfile(char *fn) 
{ 
		return 0; 
} 


static sU8 *loadv2m(char *fn, int *outlen=0)
{
	HANDLE input_file=INVALID_HANDLE_VALUE;

	input_file = CreateFile(fn,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
												   OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (input_file == INVALID_HANDLE_VALUE) // error opening file
		return 0;
	int len=GetFileSize(input_file,NULL);

	sU8 *filebuf=new unsigned char [len];
	if (!filebuf)
		return 0;

  DWORD readlen;
	ReadFile(input_file,filebuf,len,&readlen,0);
	CloseHandle(input_file);
	if (!readlen) 
	{
		delete filebuf;
		return 0;
	}

	if (outlen) *outlen=readlen;
	return filebuf;
}


int play(char *fn) 
{ 
	int maxlatency;
	unsigned long thread_id;

	lastfn[0]=0;

	delete myv2m;
	myv2m=loadv2m(fn,&file_length);
		
	if (!myv2m)
	{
		MessageBox(0,"Error opening file!\n","Farbrausch V2M Player",MB_OK|MB_ICONERROR);
		return 1;
	}

	// convert v2m to latest version
	myversion=CheckV2MVersion(myv2m,file_length);
	if (myversion<0)
	{
		MessageBox(0,v2mconv_errors[-myversion],"Farbrausch V2M Player",MB_OK|MB_ICONERROR);
		delete myv2m;
		myv2m=0;
		return 1;
	}

	sU8 *newv2m;
	sInt newlen;
	ConvertV2M(myv2m,file_length,&newv2m,&newlen);
	delete myv2m;
	myv2m=newv2m;
	file_length=newlen;
	myversion=v2version-myversion;

	paused=0;
	decode_pos_ms=0;
	seek_needed=-1;

	maxlatency = mod.outMod->Open(SAMPLERATE,NCH,BPS, -1,-1);
	if (maxlatency < 0) // error opening device
	{
		delete myv2m;
		myv2m=0;
		return 1;
	}

	strcpy_s(lastfn,260,fn);

	// dividing by 1000 for the first parameter of setinfo makes it
	// display 'H'... for hundred.. i.e. 14H Kbps.
	mod.SetInfo((SAMPLERATE*BPS*NCH)/1000,SAMPLERATE/1000,NCH,1);


	// initialize vis stuff
	mod.SAVSAInit(maxlatency,SAMPLERATE);
	mod.VSASetInfo(SAMPLERATE,NCH);

	mod.outMod->SetVolume(-666); // set the output plug-ins default volume


	// TODO: init and start synth here
	ssInit(myv2m,0);

	delete timeline;
	timeline=0;
	tllen=ssCalcPositions(&timeline);
	song_len_ms=timeline[2*(tllen-1)]+2000; // 2secs after last event

	ssPlay();

	killDecodeThread=0;
	thread_handle = (HANDLE) CreateThread(NULL,0,(LPTHREAD_START_ROUTINE) DecodeThread,(void *) &killDecodeThread,0,&thread_id);
	SetThreadPriority(thread_handle,THREAD_PRIORITY_ABOVE_NORMAL);
	
	return 0; 
}


void pause() { paused=1; mod.outMod->Pause(1); }
void unpause() { paused=0; mod.outMod->Pause(0); }
int ispaused() { return paused; }


void stop() { 
	if (thread_handle != INVALID_HANDLE_VALUE)
	{
		killDecodeThread=1;
		if (WaitForSingleObject(thread_handle,1000) == WAIT_TIMEOUT)
		{
			MessageBox(mod.hMainWindow,"error asking thread to die!\n","error killing decode thread",0);
			TerminateThread(thread_handle,0);
		}
		CloseHandle(thread_handle);
		thread_handle = INVALID_HANDLE_VALUE;
	}

  ssStop();

	mod.outMod->Close();
	mod.SAVSADeInit();

	delete myv2m;
	myv2m=0;
	delete timeline;
	timeline=0;

}


int getlength() { 
  // TODO: länge rausfinden!
	return song_len_ms;
}


int getoutputtime() { 
	// TODO: get output time!
	return decode_pos_ms+(mod.outMod->GetOutputTime()-mod.outMod->GetWrittenTime()); 
}


void setoutputtime(int time_in_ms) { 
	seek_needed=time_in_ms; 
}

void setvolume(int volume) { mod.outMod->SetVolume(volume); }
void setpan(int pan) { mod.outMod->SetPan(pan); }

int infoDlg(char *fn, HWND hwnd)
{
	// TODO: implement info dialog. 
	MessageBox(hwnd,"No info available!","Farbrausch V2M Player",MB_OK);
	return 0;
}


void getfileinfo(char *filename, char *title, int *length_in_ms)
{
	if (!filename || !*filename)  // currently playing file
	{
		if (length_in_ms) *length_in_ms=getlength();
		if (title && lastfn[0]) 
		{
			char *p=lastfn+strlen(lastfn);
			while (*p != '\\' && p >= lastfn) p--;
			strcpy(title,++p);
		}
	}
	else 
	{
		printf2("getinfo for '%s'\n",filename);
		if (title)
		{
			char *p=filename+strlen(filename);
			while (*p != '\\' && p >= filename) p--;
			strcpy(title,++p);
		}
		if (length_in_ms) // other file
		{
			int v2mlen;
			sU8 *v2m=loadv2m(filename,&v2mlen);
			if (v2m && v2mlen)
			{
				sInt tll;
				sS32 *tl=0;
				tll=ssCalcPositions(&tl,v2m);
				*length_in_ms=tl[2*(tll-1)]+2000; // 2secs after last event			
			}
			delete v2m;
		}
	}
}

void eq_set(int on, char data[10], int preamp) 
{ 
	// most plug-ins can't even do an EQ anyhow.. I'm working on writing
	// a generic PCM EQ, but it looks like it'll be a little too CPU 
	// consuming to be useful :)
}


// render 576 samples into buf. 
// note that if you adjust the size of sample_buffer, for say, 1024
// sample blocks, it will still work, but some of the visualization 
// might not look as good as it could. Stick with 576 sample blocks
// if you can, and have an additional auxiliary (overflow) buffer if 
// necessary.. 
static unsigned long get_576_samples(char *buf)
{
	unsigned long l;
	l=BUFSIZE;

	sscb(0,synth_buffer,576);

	signed short *outs=(signed short *)buf;
	for (int i=0; i<2*576; i++)
	{
		float v=32768*synth_buffer[i];
		if (v<-32760) v=-32760;
		if (v>32760) v=32760;
		outs[i]=(signed short)v;
	}

	return l;
}

DWORD WINAPI __stdcall DecodeThread(void *b)
{
	int done=0;
	while (! *((int *)b) ) 
	{
		if (seek_needed != -1)
		{
			decode_pos_ms = seek_needed-(seek_needed%1000);
			seek_needed=-1;
			done=0;
			mod.outMod->Flush(decode_pos_ms);
			ssPlayFrom(decode_pos_ms);
		}
		if (done)
		{
			mod.outMod->CanWrite();
			if (!mod.outMod->IsPlaying())
			{
				PostMessage(mod.hMainWindow,WM_WA_MPEG_EOF,0,0);
				return 0;
			}
			Sleep(10);
		}
		else if (mod.outMod->CanWrite() >= (BUFSIZE)<<(mod.dsp_isactive()?1:0))
		{	
			int l=BUFSIZE;
			l=get_576_samples(sample_buffer);
			if (!l) 
				done=1;
			else
			{
				mod.SAAddPCMData((char *)sample_buffer,NCH,BPS,decode_pos_ms);
				mod.VSAAddPCMData((char *)sample_buffer,NCH,BPS,decode_pos_ms);
				decode_pos_ms+=(576*1000)/SAMPLERATE;
				if (mod.dsp_isactive()) l=mod.dsp_dosamples((short *)sample_buffer,l/NCH/(BPS/8),BPS,NCH,SAMPLERATE)*(NCH*(BPS/8));
				mod.outMod->Write(sample_buffer,l);
			}
			if (decode_pos_ms>song_len_ms) done=1;
		}
		else Sleep(20);
	}
	return 0;
}



In_Module mod = 
{
	IN_VER,
	"Farbrausch V2 Module Player v0.1",
	0,	// hMainWindow
	0,  // hDllInstance
	"v2m\0V2 Module File (*.V2M)\0",
	1,	// is_seekable
	1, // uses output
	config,
	about,
	init,
	quit,
	getfileinfo,
	infoDlg,
	isourfile,
	play,
	pause,
	unpause,
	ispaused,
	stop,
	
	getlength,
	getoutputtime,
	setoutputtime,

	setvolume,
	setpan,

	0,0,0,0,0,0,0,0,0, // vis stuff


	0,0, // dsp

	eq_set,

	NULL,		// setinfo

	0 // out_mod

};

extern "C" __declspec( dllexport ) In_Module * winampGetInModule2()
{
	return &mod;
}


//---------------------------------------------------------------------------------------------

// FAKE DSIO IMPLEMENTATION

sU32 __stdcall dsInit(DSIOCALLBACK *callback, void *parm, void *hWnd)
{
	sscb=callback;
	return 1;
}

void __stdcall dsClose()
{
}

sS32 __stdcall dsGetCurSmp()
{
	return decode_pos_ms;
}
	
void __stdcall dsSetVolume(float vol)
{
}

void __stdcall dsFadeOut(int time)
{
}

void __stdcall dsTick()
{
}

//----------------------------------------------------------------------------------------------

// V2M CONVERTER !
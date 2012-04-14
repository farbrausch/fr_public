// tinyplayer.cpp : Defines the entry point for the console application.
//

#pragma once
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include "../types.h"
#include "../dsio.h"
#include "../v2mplayer.h"

HANDLE stdout;

void print(const char *bla)
{
	unsigned long bw;
	int len=-1;
	while (bla[++len]);
	WriteFile(stdout,bla,len,&bw,0);
}

/*
struct V2PerfModule
{
  sChar Name[16];
  sU64  Time;
};

void printat(int x,int y,const char *bla)
{
  COORD coord;

  coord.X = x;
  coord.Y = y;
  SetConsoleCursorPosition(stdout,coord);
  print(bla);
}

//extern "C" V2PerfModule V2Perf[];

void printstats(int x,int y,const char *bla)
{
  char buffer[128];
  int i;
  wsprintf(buffer,"%3ds",(GetTickCount() - startTicks) / 1000);
  printat(1,12,buffer);

  sU64 totalTime = V2Perf[0].Time;

  for(i=0;V2Perf[i].Name[0];i++)
  {
    V2Perf[i].Name[15] = 0;
    sInt timeInt = V2Perf[i].Time;
    sInt cpu = MulDiv(timeInt,100,totalTime);

    wsprintf(buffer,"%s %3d%% %12ld",V2Perf[i].Name,cpu,V2Perf[i].Time);
    printat(1,14+i,buffer);
  }
}
*/

////////////////////////////////////////////////////////////////////////////////////////////////////



/*

extern "C" const unsigned char theSoundbank[];

static const int notes[3][16]=
{
	{40, 0, 0, 0, 0, 0, 0,40, 0, 0,40, 0, 0, 0, 0, 0, },
	{ 0, 0, 0, 0,60, 0, 0, 0, 0, 0, 0, 0,60, 0, 0, 0, },
	{72, 0,72,72, 0,72, 0, 0,72, 0,72,72, 0,72, 0, 0, },
};

static const int patch[3]={3,10,84};


class CMySequencer
{
public:

	void Init()
	{
		first=1;
		position=0;
		lastnote[0]=lastnote[1]=lastnote[2]=0;

		miInit(theSoundbank, MidiProxy, this, GetForegroundWindow());
		miSetBPM(125,16);
	}

	void Close()
	{
		miClose();
	}

  //------------------------------------------------------------------------------------

private:

	int first;
	int position;
	int lastnote[3];

	// the typical callback proxy because we're a class and we need a "this" pointer...
	// which was therefore stored in the custom callback parameter. 
	static void __stdcall MidiProxy(void *p) { ((CMySequencer*)p)->MidiCallback(); }

	unsigned char midibuf[1024];
	int mblen;

	// our MIDI callback routine
	void MidiCallback()
	{
		if (first) // first send program change commands
		{
			unsigned char pgmch[2];
			for (int i=0; i<3; i++)
			{
				pgmch[0]=0xc0 | i;  // "program change"
				pgmch[1]=patch[i];
				miSendMIDIData(pgmch,2);
			}
			first=0;
		}

		// now send notes
		for (int channel=0; channel<3; channel++)
		{
			unsigned char command[3];

			// send note-off if there's a last note playing
			if (lastnote[channel])
			{
				command[0]=0x80 | channel;  // "note off"
				command[1]=lastnote[channel];
				command[2]=0;
				miSendMIDIData(command,3);
			}

			// ... and send the new note
			if ((lastnote[channel]=notes[channel][position])!=0)
			{
				command[0]=0x90 | channel; // "note on"
				command[1]=lastnote[channel];
				command[2]=100;
				miSendMIDIData(command,3);
			}
		}

		// increase position
		position++;
		if (position==16) position=0;


	}

};
*/

extern "C" sU8 theTune[];
extern "C" sU8 theTune2[];
static CV2MPlayer p1, p2;

static sU32 __stdcall render(void*, sF32 *buffer, sU32 samples)
{
  p1.Render(buffer,samples);
  p2.Render(buffer,samples,sTRUE);
  return 0;
}

#ifdef NDEBUG
extern "C" void mainCRTStartup()
#else
void main()
#endif
{
	stdout=GetStdHandle(STD_OUTPUT_HANDLE);

	print("\n");
	print("  __\n");
	print(" | ||  __\n");
	print(" | || | ||  -<-´\n");
	print(" | || | || \n");
	print(" | || | || \n");
	print(" | || | || \n");
	print(" | || | || \n");

	/*CMySequencer seq;
	seq.Init();*/

  sInt startTicks = GetTickCount();
  p1.Init();
  p2.Init();

  p1.Open(theTune);
  p2.Open(theTune2);

  dsInit(render,0,GetForegroundWindow());

  dsLock();
  p1.Play();
  dsUnlock();

	print("\n\npress ESC to quit\n");
	while (GetAsyncKeyState(VK_ESCAPE)>=0)
  {
    //printstats();    
    Sleep(10);
    if (GetAsyncKeyState(VK_SPACE)<0)
    {
      dsLock();
      p2.Play();
      dsUnlock();
    }
  }

	//seq.Close();
  dsClose();

	ExitProcess(0);
}


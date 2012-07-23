/*************************************************************************************/
/*************************************************************************************/
/**                                                                                 **/
/**  Tinyplayer - TibV2 example                                                     **/
/**  written by Tammo 'kb' Hinrichs 2000-2008                                       **/
/**  This file is in the public domain                                              **/
/**  "Patient Zero" is (C) Melwyn+LB 2005, do not redistribute                      **/
/**                                                                                 **/
/*************************************************************************************/
/*************************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../v2mplayer.h"
#include "../libv2.h"

static HANDLE stdout;
static V2MPlayer player;

void print(const char *bla)
{
	unsigned long bw;
	int len=-1;
	while (bla[++len]);
	WriteFile(stdout,bla,len,&bw,0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" const sU8 theTune[];

extern void synthPrintCoverage();

#ifdef NDEBUG
extern "C" void mainCRTStartup()
#else
void main()
#endif
{
	stdout=GetStdHandle(STD_OUTPUT_HANDLE);

	print("\nFarbrausch Tiny Music Player v0.dontcare TWO\n");
	print("Code and Synthesizer (W) 2000-2008 kb/Farbrausch\n");
	print("\n\nNow Playing: 'Patient Zero' by Melwyn & Little Bitchard\n");

  player.Init();
  player.Open(theTune);

  dsInit(player.RenderProxy,&player,GetForegroundWindow());

  player.Play();

  sInt startTicks = GetTickCount();

	print("\n\npress ESC to quit\n");
	while (GetAsyncKeyState(VK_ESCAPE)>=0)
  {
    Sleep(10);
  }

  dsClose();
  player.Close();

  //synthPrintCoverage();

	ExitProcess(0);
}

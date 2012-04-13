// This code is in the public domain. See LICENSE for details.

#ifndef _DSIO_H_
#define _DSIO_H_

typedef void (__stdcall DSIOCALLBACK)(float *buf, unsigned long len);

extern "C" {
	extern int __stdcall dsInit(DSIOCALLBACK *callback, void *hWnd);
	extern void __stdcall dsClose();
	extern int __stdcall dsGetCurSmp();
	extern void __stdcall dsSetVolume(float vol);
	extern void __stdcall dsFadeOut(int time);
  extern void __stdcall dsTick();
}

#endif

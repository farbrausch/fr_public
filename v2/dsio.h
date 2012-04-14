
#ifndef _DSIO_H_
#define _DSIO_H_


typedef void (__stdcall DSIOCALLBACK)(void *parm, sF32 *buf, sU32 len);

extern "C" {
	extern sU32 __stdcall dsInit(DSIOCALLBACK *callback, void *parm, void *hWnd);
	extern void __stdcall dsClose();
	extern sS32 __stdcall dsGetCurSmp();
	extern void __stdcall dsSetVolume(float vol);
  extern void __stdcall dsTick();
  extern void __stdcall dsLock();
  extern void __stdcall dsUnlock();
}

#endif
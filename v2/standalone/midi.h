#ifndef _MIDI_H_
#define _MIDI_H_

extern char msdevnames[256][256];
extern int  msnumdevs;
extern void msInit();
extern void msClose();
extern void msSetDevice(int n);
extern int  msGetDevice();

#ifdef RONAN
extern void msStartAudio(HWND hwnd, void *patchmap, void *globals, const char **);
#else
extern void msStartAudio(HWND hwnd, void *patchmap, void *globals);
#endif

extern void msStopAudio();
extern void msGetLD(float *l, float *r, int *cl, int *cr);
extern int  msGetCPUUsage();

#endif

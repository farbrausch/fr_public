#ifndef _MIDI_H_
#define _MIDI_H_

extern void msInit();
extern void msClose();

#ifdef RONAN
extern void msStartAudio(HWND hwnd, void *patchmap, void *globals, const char **);
extern void msSetSampleRate(int newrate,void *patchmap, void *globals, const char **);
#else
extern void msStartAudio(HWND hwnd, void *patchmap, void *globals);
extern void msSetSampleRate(int newrate,void *patchmap, void *globals);
#endif


extern void msStopAudio();
extern void msGetLD(float *l, float *r, int *cl, int *cr);
extern void msProcessEvent(DWORD lastoffs, DWORD dwParam1);
extern int  msGetCPUUsage();

void msStartRecord();
int msIsRecording(); 
int msStopRecord(file &f);

// lr: added
void msStopRecord();
int msWriteLastRecord(file &f);

void msSetProgram1(int p);

extern sU8 *theSynth;

#endif

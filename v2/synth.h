
#ifndef _SYNTH_H_
#define _SYNTH_H_

extern "C"
{
  extern unsigned int __stdcall synthGetSize();
  
	extern void __stdcall synthInit(void *pthis, const void *patchmap, int samplerate=44100);
	extern void __stdcall synthRender(void *pthis, void *buf, int smp, void *buf2=0, int add=0);
	extern void __stdcall synthProcessMIDI(void *pthis, const void *ptr);
	extern void __stdcall synthSetGlobals(void *pthis, const void *ptr);
//  extern void __stdcall synthSetSampler(void *pthis, const void *bankinfo, const void *samples);
	extern void __stdcall synthGetPoly(void *pthis, void *dest);
	extern void __stdcall synthGetPgm(void *pthis, void *dest);
//	extern void __stdcall synthGetLD(void *pthis, float *l, float *r);

	// only if VUMETER define is set in synth source

	// vu output values are floats, 1.0 == 0dB
	// you might want to clip or logarithmize the values for yourself
	extern void __stdcall synthSetVUMode(void *pthis, int mode); // 0: peak, 1: rms
	extern void __stdcall synthGetChannelVU(void *pthis, int ch, float *l, float *r); // ch: 0..15
	extern void __stdcall synthGetMainVU(void *pthis, float *l, float *r);

	extern long __stdcall synthGetFrameSize(void *pthis);

#ifdef RONAN
	extern void __stdcall synthSetLyrics(void *pthis, const char **ptr);
#endif

}

#endif

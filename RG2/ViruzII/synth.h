
#ifndef _SYNTH_H_
#define _SYNTH_H_

extern "C"
{
	extern void __stdcall synthInit(void *patchmap, int samplerate=44100);
	extern void __stdcall synthRender(void *buf, int smp, void *buf2=0, int add=0);
	extern void __stdcall synthProcessMIDI(void *ptr);
	extern void __stdcall synthSetGlobals(void *ptr);
	extern void __stdcall synthGetPoly(void *dest);
	extern void __stdcall synthGetPgm(void *dest);
	extern void __stdcall synthGetLD(float *l, float *r);

	// only if VUMETER define is set in synth source

	// vu output values are floats, 1.0 == 0dB
	// you might want to clip or logarithmize the values for yourself
	extern void __stdcall synthSetVUMode(int mode); // 0: peak, 1: rms
	extern void __stdcall synthGetChannelVU(int ch, float *l, float *r); // ch: 0..15
	extern void __stdcall synthGetMainVU(float *l, float *r);
}

#endif

#ifndef V2_MIDI_H_
#define V2_MIDI_H_

// V2 Direct MIDI interface for Melwyn, (C) 2004 T. 'kb' H.

// definition of your own timer callback routine
typedef void (__stdcall *MITIMERCALLBACK)(void *parm);

// initalizes the midi out. the first parameter is a pointer to a valid
// .v2b file which you can save with the V2 GUI, the second parameter
// is your own timer callback routine ala "void __stdcall blah(void *p)",
// the third param is a user supplied void* value (which you then get
// as eg. "p") and the fourth value is the handle of your current window
bool __stdcall miInit(const void *v2bdata, MITIMERCALLBACK callback, void *cbparm, void *HWND);

// shuts down the whole mess again
void __stdcall miClose();

// sets the rate at which your timer callback will be called. bpm is the
// beats per minute rate and fraction is how many calls per bar you
// want. Eg. miSetBPM(125,16) calls you every 16th of a 125bpm pattern.
void __stdcall miSetBPM(int bpm, int fraction);

// sends a block of MIDI data to the synth. You MUST (!!!) use this routine
// only from your own timer callback routine! You can put as many commands
// as you like into one call of course.
void __stdcall miSendMIDIData(const unsigned char *data, unsigned long len);

// make the sound system render the next block
// put this in your frame loop to avoid stuttering. You can also omit it, 
// but your video may run smoother if you use this call.
void __stdcall miTick();
#endif
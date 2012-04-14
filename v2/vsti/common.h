// lr: stuff that buzz & vsti use 

#pragma once



#include <vector>
#include <string>

#define lrBox(LRMSG) MessageBox(0,LRMSG,"Farbrausch V2 VSTi",MB_OK)

// aus v2edit gestohlen
enum V2CTLTYPES { VCTL_SKIP, VCTL_SLIDER, VCTL_MB, };

typedef struct {
	int		no;
	char  *name;
	char  *name2;
} V2TOPIC;

typedef struct { 

	int   version;
	char  *name;
	V2CTLTYPES ctltype;
	int		offset, min, max;
	int   isdest;
	char  *ctlstr;	
} V2PARAM;

typedef std::vector<std::string> V2STRINGLIST;

struct ParamInfo : public V2PARAM {
	char paramname[32];
	V2STRINGLIST controlstrings;
};

#ifndef __AudioEffect__

class AudioEffectX;
class AEffEditor;
class VstEvents;

#define V2SOUNDSIZE (m_v2nparms+1+255*3)

#else

#define V2SOUNDSIZE (v2nparms+1+255*3)

#endif

/*

these are the old style entry functions. new functions should be added to IV2Host or IV2Client

*/

typedef AEffEditor* (__cdecl *V2VSTI_INIT_FUNC)(AudioEffectX* effect);
typedef void (__cdecl *V2VSTI_SHUTDOWN_FUNC)(AEffEditor* editor);
typedef void (__cdecl *V2VSTI_PROCESS_FUNC)(float** inputs, float** outputs, long sampleframes);
typedef void (__cdecl *V2VSTI_PROCESS_REPLACING_FUNC)(float** inputs, float** outputs, long sampleframes);
typedef long (__cdecl *V2VSTI_PROCESS_EVENTS_FUNC)(VstEvents* events);
typedef long (__cdecl *V2VSTI_GET_CHUNK_FUNC)(void** data, bool isPreset);
typedef long (__cdecl *V2VSTI_SET_CHUNK_FUNC)(void* data, long byteSize, bool isPreset);
typedef char* (__cdecl *V2VSTI_GET_PATCH_NAMES_FUNC)();
typedef void (__cdecl *V2VSTI_SET_PROGRAM_FUNC)(AEffEditor* editor, UINT nr);
typedef void (__cdecl *V2VSTI_GET_PARAM_DEFS_FUNC)
	(V2TOPIC** topics, int* ntopics, V2PARAM** parms, int* nparms, 
	 V2TOPIC** gtopics, int* ngtopics, V2PARAM** gparms, int* ngparms);
typedef void (__cdecl *V2VSTI_GET_PARAMS_FUNC)(unsigned char** psoundmem, char** pglobals);
typedef void (__cdecl *V2VSTI_UPDATE_UI_FUNC)(int program, int index);
typedef void (__cdecl *V2VSTI_SET_SAMPLERATE_FUNC)(int newrate);

typedef void (__cdecl *V2BUZZ_INIT_FUNC)();
typedef void (__cdecl *V2BUZZ_SHUTDOWN_FUNC)();
typedef void (__cdecl *V2BUZZ_PROCESS_EVENT)(DWORD dwOffset, DWORD dwMidiData);
typedef void (__cdecl *V2BUZZ_WORK)(float* pSamples, int nSampleCount);
typedef void (__cdecl *V2BUZZ_DESTROY_EDITOR)();
typedef void (__cdecl *V2BUZZ_SHOW_EDITOR)(HWND hWndParent);
typedef void (__cdecl *V2BUZZ_SET_CHANNEL)(int channel);

// tired of entry points? me too

#include "v2interfaces.h"

static const float _0_5=0.5;

static int _declspec(naked) fastceil2(float x)
{
	static int t;
	__asm
	{
		fld [esp+4]
		fadd _0_5
		//frndint
		fistp t
		mov eax, t
		ret
	}
}

static inline float parm2float(int x, int min, int max) { return ((float)(x-min))/((float)(max-min)+0.5f); }
static inline int float2parm(float x, int min, int max) { return fastceil2(x*(max-min)-0.5f)+min; }

struct MIDIEvent
{
    operator DWORD()
    {
        return data;
    }

	// midi status codes
	enum MIDICommand
    {
		// substatus
		MIDINoteOff             = 0x8, // chan, note, velocity
		MIDINote                = 0x9, // chan, note, velocity
		MIDIAftertouch          = 0xa, // chan, note, new velocity
		MIDIControl             = 0xb, // chan, control id, value
		MIDIProgramChange       = 0xc, // chan, program
		MIDIChannelAftertouch   = 0xd, // ?, chan
		MIDIPitchBend           = 0xe, // ?, lsb, msb

		// status
		MIDISysEx               = 0xf0, 
		MIDISongPosition        = 0xf2,
		MIDITrackSelect         = 0xf3,
		MIDITuneRequest         = 0xf6,
		MIDISysExEnd            = 0xf7,
		MIDITimingClock         = 0xf8,
		MIDIStart               = 0xfa,
		MIDIContinue            = 0xfb,
		MIDIStop                = 0xfc,
		MIDIActiveSensing       = 0xfe,
		MIDIReset               = 0xff,

		// control messages
		MIDIAllNotesOff     	= 0x7b,
	};

	union
    {
		__int32 data;
		struct
        {
			union
            {
				struct
                {
					unsigned char channel:4;
					unsigned char substatus:4;
				};
				unsigned char status;
			};
			unsigned char param1;
			unsigned char param2;
			unsigned char unused;
		};
	};

    MIDIEvent()
    {
        data = 0;
    }
};
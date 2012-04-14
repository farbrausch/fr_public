// Copyright (C) 1997-2000 Oskari Tammelin (ot@iki.fi)
// This header file may be used to write _freeware_ DLL "machines" for Buzz
// Using it for anything else is not allowed without a permission from the author
   
#ifndef __MACHINE_INTERFACE_H
#define __MACHINE_INTERFACE_H

#include <stdio.h>
#include <assert.h>
#include <string.h>

#define MI_VERSION				15
  
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;

double const PI = 3.14159265358979323846;

#define MAX_BUFFER_LENGTH		256			// in number of samples

// machine types
#define MT_MASTER				0 
#define MT_GENERATOR			1
#define MT_EFFECT				2

// special parameter values
#define NOTE_NO					0
#define NOTE_OFF				255
#define NOTE_MIN				1					// C-0
#define NOTE_MAX				((16 * 9) + 12)		// B-9
#define SWITCH_OFF				0
#define SWITCH_ON				1
#define SWITCH_NO				255
#define WAVE_MIN				1
#define WAVE_MAX				200
#define WAVE_NO					0

// CMachineParameter flags
#define MPF_WAVE				1
#define MPF_STATE				2	
#define MPF_TICK_ON_EDIT		4				

// CMachineInfo flags
#define MIF_MONO_TO_STEREO		(1<<0)
#define MIF_PLAYS_WAVES			(1<<1)
#define MIF_USES_LIB_INTERFACE	(1<<2)
#define MIF_USES_INSTRUMENTS	(1<<3)
#define MIF_DOES_INPUT_MIXING	(1<<4)
#define MIF_NO_OUTPUT			(1<<5)		// used for effect machines that don't actually use any outputs (WaveOutput, AuxSend etc.)
#define MIF_CONTROL_MACHINE		(1<<6)		// used to control other (non MIF_CONTROL_MACHINE) machines
#define MIF_INTERNAL_AUX		(1<<7)		// uses internal aux bus (jeskola mixer and jeskola mixer aux)

// work modes
#define WM_NOIO					0
#define WM_READ					1
#define WM_WRITE				2
#define WM_READWRITE			3

// state flags
#define SF_PLAYING				1
#define SF_RECORDING			2


enum BEventType
{
	DoubleClickMachine,					// return true to ignore default handler (open parameter dialog), no parameters
	gDeleteMachine						// data = CMachine *, param = ThisMac
		
};

class CMachineInterface;
typedef bool (CMachineInterface::*EVENT_HANDLER_PTR)(void *);


enum CMPType { pt_note, pt_switch, pt_byte, pt_word };

class CMachineParameter
{
public:

	CMPType Type;			// pt_byte
	char const *Name;		// Short name: "Cutoff"
	char const *Description;// Longer description: "Cutoff Frequency (0-7f)"
	int MinValue;			// 0
	int MaxValue;			// 127
	int NoValue;			// 255
	int Flags;
	int DefValue;			// default value for params that have MPF_STATE flag set
};

class CMachineAttribute
{
public:
	char const *Name;
	int MinValue;
	int MaxValue;
	int DefValue;
};

class CMasterInfo
{
public:
	int BeatsPerMin;		// [16..500] 	
	int TicksPerBeat;		// [1..32]
	int SamplesPerSec;		// usually 44100, but machines should support any rate from 11050 to 96000
	int SamplesPerTick;		// (int)((60 * SPS) / (BPM * TPB))  
	int PosInTick;			// [0..SamplesPerTick-1]
	float TicksPerSec;		// (float)SPS / (float)SPT  

};

// CWaveInfo flags
#define WF_LOOP			1
#define WF_STEREO		8
#define WF_BIDIR_LOOP	16

class CWaveInfo
{
public:
	int Flags;
	float Volume;

};

class CWaveLevel
{
public:
	int numSamples;
	short *pSamples;
	int RootNote;
	int SamplesPerSec;
	int LoopStart;
	int LoopEnd;
};

// oscillator waveforms (used with GetOscillatorTable function)
#define OWF_SINE			0
#define OWF_SAWTOOTH		1
#define OWF_PULSE			2		// square 
#define OWF_TRIANGLE		3
#define OWF_NOISE			4	
#define OWF_303_SAWTOOTH	5

// each oscillator table contains one full cycle of a bandlimited waveform at 11 levels
// level 0 = 2048 samples  
// level 1 = 1024 samples
// level 2 = 512 samples
// ... 
// level 9 = 8 samples 
// level 10 = 4 samples
// level 11 = 2 samples
//
// the waves are normalized to 16bit signed integers   
//
// GetOscillatorTable retusns pointer to a table 
// GetOscTblOffset returns offset in the table for a specified level 
 
inline int GetOscTblOffset(int const level)
{
	assert(level >= 0 && level <= 10);
	return (2048+1024+512+256+128+64+32+16+8+4) & ~((2048+1024+512+256+128+64+32+16+8+4) >> level);
}

class CPattern;
class CSequence;
class CMachineInterfaceEx;
class CMachine;

class CMachineDataOutput;
class CMachineInfo;

class CMICallbacks
{
public:
	virtual CWaveInfo const * GetWave(int const i);
	virtual CWaveLevel const * GetWaveLevel(int const i, int const level);
	virtual void MessageBox(char const *txt);
	virtual void Lock();
	virtual void Unlock();
	virtual int GetWritePos();			
	virtual int GetPlayPos();	
	virtual float * GetAuxBuffer();
	virtual void ClearAuxBuffer();
	virtual int GetFreeWave();
	virtual bool AllocateWave(int const i, int const size, char const *name);
	virtual void ScheduleEvent(int const time, dword const data);
	virtual void MidiOut(int const dev, dword const data);
	virtual short const *GetOscillatorTable(int const waveform);

	// envelopes
	virtual int GetEnvSize(int const wave, int const env);
	virtual bool GetEnvPoint(int const wave, int const env, int const i, word &x, word &y, int &flags);

	virtual CWaveLevel const *GetNearestWaveLevel(int const i, int const note);
	
	// pattern editing
	virtual void SetNumberOfTracks(int const n);
	virtual CPattern *CreatePattern(char const *name, int const length);
	virtual CPattern *GetPattern(int const index);
	virtual char const *GetPatternName(CPattern *ppat);
	virtual void RenamePattern(char const *oldname, char const *newname);
	virtual void DeletePattern(CPattern *ppat);
	virtual int GetPatternData(CPattern *ppat, int const row, int const group, int const track, int const field);
	virtual void SetPatternData(CPattern *ppat, int const row, int const group, int const track, int const field, int const value);
 		
	// sequence editing
	virtual CSequence *CreateSequence();
	virtual void DeleteSequence(CSequence *pseq);
	

	// special ppat values for GetSequenceData and SetSequenceData 
	// empty = NULL
	// <break> = (CPattern *)1
	// <mute> = (CPattern *)2
	// <thru> = (CPattern *)3
	virtual CPattern *GetSequenceData(int const row);
	virtual void SetSequenceData(int const row, CPattern *ppat);
		

	// buzz v1.2 (MI_VERSION 15) additions start here
	
	virtual void SetMachineInterfaceEx(CMachineInterfaceEx *pex);
	// group 1=global, 2=track
	virtual void ControlChange__obsolete__(int group, int track, int param, int value);						// set value of parameter
	
	// direct calls to audiodriver, used by WaveInput and WaveOutput
	// shouldn't be used for anything else
	virtual int ADGetnumChannels(bool input);
	virtual void ADWrite(int channel, float *psamples, int numsamples);
	virtual void ADRead(int channel, float *psamples, int numsamples);

	virtual CMachine *GetThisMachine();	// only call this in Init()!
	virtual void ControlChange(CMachine *pmac, int group, int track, int param, int value);		// set value of parameter (group & 16 == don't record)

	// returns pointer to the sequence if there is a pattern playing
	virtual CSequence *GetPlayingSequence(CMachine *pmac);

	// gets ptr to raw pattern data for row of a track of a currently playing pattern (or something like that)
	virtual void *GetPlayingRow(CSequence *pseq, int group, int track);

	virtual int GetStateFlags();

	virtual void SetnumOutputChannels(CMachine *pmac, int n);	// if n=1 Work(), n=2 WorkMonoToStereo()

	virtual void SetEventHandler(CMachine *pmac, BEventType et, EVENT_HANDLER_PTR p, void *param);

	virtual char const *GetWaveName(int const i);

	virtual void SetInternalWaveName(CMachine *pmac, int const i, char const *name);	// i >= 1, NULL name to clear

	virtual void GetMachineNames(CMachineDataOutput *pout);		// *pout will get one name per Write()
	virtual CMachine *GetMachine(char const *name);
	virtual CMachineInfo const *GetMachineInfo(CMachine *pmac);
	virtual char const *GetMachineName(CMachine *pmac);

	virtual bool GetInput(int index, float *psamples, int numsamples, bool stereo, float *extrabuffer);
};


class CLibInterface
{
public:
	virtual void GetInstrumentList(CMachineDataOutput *pout) {}			
	
	// make some space to vtable so this interface can be extended later 
	virtual void Dummy1() {}
	virtual void Dummy2() {}
	virtual void Dummy3() {}
	virtual void Dummy4() {}
	virtual void Dummy5() {}
	virtual void Dummy6() {}
	virtual void Dummy7() {}
	virtual void Dummy8() {}
	virtual void Dummy9() {}
	virtual void Dummy10() {}
	virtual void Dummy11() {}
	virtual void Dummy12() {}
	virtual void Dummy13() {}
	virtual void Dummy14() {}
	virtual void Dummy15() {}
	virtual void Dummy16() {}
	virtual void Dummy17() {}
	virtual void Dummy18() {}
	virtual void Dummy19() {}
	virtual void Dummy20() {}
	virtual void Dummy21() {}
	virtual void Dummy22() {}
	virtual void Dummy23() {}
	virtual void Dummy24() {}
	virtual void Dummy25() {}
	virtual void Dummy26() {}
	virtual void Dummy27() {}
	virtual void Dummy28() {}
	virtual void Dummy29() {}
	virtual void Dummy30() {}
	virtual void Dummy31() {}
	virtual void Dummy32() {}


};


class CMachineInfo
{
public:
	int Type;								// MT_GENERATOR or MT_EFFECT, 
	int Version;							// MI_VERSION
											// v1.2: high word = internal machine version
											// higher version wins if two copies of machine found
	int Flags;				
	int minTracks;							// [0..256] must be >= 1 if numTrackParameters > 0 
	int maxTracks;							// [minTracks..256] 
	int numGlobalParameters;				
	int numTrackParameters;					
	CMachineParameter const **Parameters;
	int numAttributes;
	CMachineAttribute const **Attributes;
	char const *Name;						// "Jeskola Reverb"
	char const *ShortName;					// "Reverb"
	char const *Author;						// "Oskari Tammelin"
	char const *Commands;					// "Command1\nCommand2\nCommand3..."
	CLibInterface *pLI;						// ignored if MIF_USES_LIB_INTERFACE is not set
};

class CMachineDataInput
{
public:
	virtual void Read(void *pbuf, int const numbytes);

	void Read(int &d) { Read(&d, sizeof(int)); }
	void Read(dword &d) { Read(&d, sizeof(dword)); }
	void Read(short &d) { Read(&d, sizeof(short)); }
	void Read(word &d) { Read(&d, sizeof(word)); }
	void Read(char &d) { Read(&d, sizeof(char)); }
	void Read(byte &d) { Read(&d, sizeof(byte)); }
	void Read(float &d) { Read(&d, sizeof(float)); }
	void Read(double &d) { Read(&d, sizeof(double)); }
	void Read(bool &d) { Read(&d, sizeof(bool)); }

};

class CMachineDataOutput
{
public:
	virtual void Write(void *pbuf, int const numbytes);

	void Write(int d) { Write(&d, sizeof(int)); }
	void Write(dword d) { Write(&d, sizeof(dword)); }
	void Write(short d) { Write(&d, sizeof(short)); }
	void Write(word d) { Write(&d, sizeof(word)); }
	void Write(char d) { Write(&d, sizeof(char)); }
	void Write(byte d) { Write(&d, sizeof(byte)); }
	void Write(float d) { Write(&d, sizeof(float)); }
	void Write(double d) { Write(&d, sizeof(double)); }
	void Write(bool d) { Write(&d, sizeof(bool)); }
	void Write(char const *str) { Write((void *)str, (int const)(strlen(str)+1)); }

};

// envelope info flags
#define EIF_SUSTAIN			1
#define EIF_LOOP			2

class CEnvelopeInfo
{
public:
	char const *Name;
	int Flags;
};

class CMachineInterface
{
public:
	virtual ~CMachineInterface() {}
	virtual void Init(CMachineDataInput * const pi) {}
	virtual void Tick() {}
	virtual bool Work(float *psamples, int numsamples, int const mode) { return false; }
	virtual bool WorkMonoToStereo(float *pin, float *pout, int numsamples, int const mode) { return false; }
	virtual void Stop() {}
	virtual void Save(CMachineDataOutput * const po) {}
	virtual void AttributesChanged() {}
	virtual void Command(int const i) {}

	virtual void SetNumTracks(int const n) {}
	virtual void MuteTrack(int const i) {}
	virtual bool IsTrackMuted(int const i) const { return false; }

	virtual void MidiNote(int const channel, int const value, int const velocity) {}
	virtual void Event(dword const data) {}

	virtual char const *DescribeValue(int const param, int const value) { return NULL; }

	virtual CEnvelopeInfo const **GetEnvelopeInfos() { return NULL; }

	virtual bool PlayWave(int const wave, int const note, float const volume) { return false; }
	virtual void StopWave() {}
	virtual int GetWaveEnvPlayPos(int const env) { return -1; }


public:
	// initialize these members in the constructor 
	void *GlobalVals;
	void *TrackVals;
	int *AttrVals;
		
	// these members are initialized by the 
	// engine right after it calls CreateMachine()
	// don't touch them in the constructor
	CMasterInfo *pMasterInfo;
	CMICallbacks *pCB;					

};

// buzz v1.2 extended machine interface
class CMachineInterfaceEx
{
public:
	virtual char const *DescribeParam(int const param) { return NULL; }		// use this to dynamically change name of parameter
	virtual bool SetInstrument(char const *name) { return false; }

	virtual void GetSubMenu(int const i, CMachineDataOutput *pout) {}

	virtual void AddInput(char const *macname, bool stereo) {}	// called when input is added to a machine
	virtual void DeleteInput(char const *macename) {}			
	virtual void RenameInput(char const *macoldname, char const *macnewname) {}

	virtual void Input(float *psamples, int numsamples, float amp) {} // if MIX_DOES_INPUT_MIXING

	virtual void MidiControlChange(int const ctrl, int const channel, int const value) {}

	virtual void SetInputChannels(char const *macname, bool stereo) {}

	virtual bool HandleInput(int index, int amp, int pan) { return false; }

	// make some space to vtable so this interface can be extended later 
	virtual void Dummy1() {}
	virtual void Dummy2() {}
	virtual void Dummy3() {}
	virtual void Dummy4() {}
	virtual void Dummy5() {}
	virtual void Dummy6() {}
	virtual void Dummy7() {}
	virtual void Dummy8() {}
	virtual void Dummy9() {}
	virtual void Dummy10() {}
	virtual void Dummy11() {}
	virtual void Dummy12() {}
	virtual void Dummy13() {}
	virtual void Dummy14() {}
	virtual void Dummy15() {}
	virtual void Dummy16() {}
	virtual void Dummy17() {}
	virtual void Dummy18() {}
	virtual void Dummy19() {}
	virtual void Dummy20() {}
	virtual void Dummy21() {}
	virtual void Dummy22() {}
	virtual void Dummy23() {}
	virtual void Dummy24() {}
	virtual void Dummy25() {}
	virtual void Dummy26() {}
	virtual void Dummy27() {}
	virtual void Dummy28() {}
	virtual void Dummy29() {}
	virtual void Dummy30() {}
	virtual void Dummy31() {}
	virtual void Dummy32() {}

};
 
class CMILock
{
public:
	CMILock(CMICallbacks *p) { pCB = p; pCB->Lock(); }
	~CMILock() { pCB->Unlock(); }
private:
	CMICallbacks *pCB;
};

#define MACHINE_LOCK CMILock __machinelock(pCB);

#ifdef STATIC_BUILD

	typedef CMachineInfo const *(__cdecl *GET_INFO)();												
	typedef CMachineInterface *(__cdecl *CREATE_MACHINE)();										

	extern void RegisterMachine(CMachineInfo const *pmi, GET_INFO gi, CREATE_MACHINE cm);	

#define DLL_EXPORTS(INIT_FUNC)															\
	static CMachineInfo const * __cdecl GetInfo() { return &MacInfo; }					\
	static CMachineInterface * __cdecl CreateMachine() { return new mi; }				\
	void INIT_FUNC() { RegisterMachine(&MacInfo, GetInfo, CreateMachine); }				\


#define DLL_EXPORTS_NS(NS, INIT_FUNC) /* namespaced version */ 								\
	static CMachineInfo const * __cdecl GetInfo() { return &NS::MacInfo; }					\
	static CMachineInterface * __cdecl CreateMachine() { return new NS::mi; }				\
	void INIT_FUNC() { RegisterMachine(&NS::MacInfo, GetInfo, CreateMachine); }				\



#else

	#define DLL_EXPORTS extern "C" { \
	__declspec(dllexport) CMachineInfo const * __cdecl GetInfo() \
	{ \
		return &MacInfo; \
	} \
	__declspec(dllexport) CMachineInterface * __cdecl CreateMachine() \
	{ \
		return new mi; \
	} \
	} 

#endif

#endif 
// lr: messy vsti plugin header

#pragma once
#include "vst2/audioeffectx.h"

#include "common.h"

enum
{
	kNumPrograms = 128,
	kNumOutputs = 2,

	kBogus = 0,

	kNumParams
};

/*
class ViruzVstiProgram
{
public:
	ViruzVstiProgram();
	~ViruzVstiProgram() {}

	float fBogus;

	char name[24];
};*/

class ViruzVsti : public AudioEffectX, public V2::IClient
{
public:
	ViruzVsti(audioMasterCallback audioMaster);
	~ViruzVsti();
	
	virtual void process(float **inputs, float **outputs, long sampleframes);
	virtual void processReplacing(float **inputs, float **outputs, long sampleframes);
	virtual long processEvents(VstEvents* events);

	virtual void setProgram(long program);
	virtual void setProgramName(char *name);
	virtual void getProgramName(char *name);
	virtual void setParameter(long index, float value);
	virtual float getParameter(long index);
	virtual void getParameterLabel(long index, char *label);
	virtual void getParameterDisplay(long index, char *text);
	virtual void getParameterName(long index, char *text);
	virtual void setSampleRate(float sampleRate);
	virtual void setBlockSize(long blockSize);
	virtual void resume();

	virtual bool getOutputProperties (long index, VstPinProperties* properties);
	virtual bool getProgramNameIndexed (long category, long index, char* text);
	virtual bool copyProgram (long destination);
	virtual bool getEffectName (char* name);
	virtual bool getVendorString (char* text);
	virtual bool getProductString (char* text);
	virtual long getVendorVersion () {return 1;}
	virtual long canDo (char* text);

	long getChunk(void** data, bool isPreset = false) {
		return v2vstiGetChunk(data,isPreset);
	}	// returns byteSize

	long setChunk(void* data, long byteSize, bool isPreset = false) {
		return v2vstiSetChunk(data,byteSize,isPreset);
	}

	virtual void setParameterAutomated(long index, float value);

	// V2::IClient implementation

	virtual void CurrentParameterChanged(int index)
	{
		setParameterAutomated(index,getParameter(index));
	}
private:
	void SetupParamInfo(ParamInfo* pi);
	void SetupParamInfos();
	char ModuleName[1024];
	ParamInfo* ParamInfos;
	char* PatchNames;
	HMODULE module;
	V2VSTI_INIT_FUNC v2vstiInit;
	V2VSTI_SHUTDOWN_FUNC v2vstiShutdown;
	V2VSTI_PROCESS_FUNC v2vstiProcess;
	V2VSTI_PROCESS_REPLACING_FUNC v2vstiProcessReplacing;
	V2VSTI_PROCESS_EVENTS_FUNC v2vstiProcessEvents;
	V2VSTI_GET_CHUNK_FUNC v2vstiGetChunk;
	V2VSTI_SET_CHUNK_FUNC v2vstiSetChunk;
	V2VSTI_GET_PATCH_NAMES_FUNC v2vstiGetPatchNames;	
	V2VSTI_SET_PROGRAM_FUNC v2vstiSetProgram;
	V2VSTI_GET_PARAM_DEFS_FUNC v2vstiGetParameterDefs;
	V2VSTI_GET_PARAMS_FUNC v2vstiGetParameters;
	V2VSTI_UPDATE_UI_FUNC v2vstiUpdateUI;
	V2VSTI_SET_SAMPLERATE_FUNC v2vstiSetSampleRate;
	V2::GetHostFunc v2GetHost;
	V2::IHost* host;
	V2TOPIC* v2topics;
	int v2ntopics;
	V2PARAM* v2parms;
	int v2nparms;
	V2TOPIC* v2gtopics;
	int v2ngtopics;
	V2PARAM* v2gparms;
	int v2ngparms;
	unsigned char* soundmem;
	char* globals;
};

#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <windows.h>
#include <math.h>
#include "vsti.h"


/*
float _declspec(naked) fastfloor(float x)
{
	__asm
	{
		fld [esp+4]
		fsub _0_5
		frndint
		ret
	}
}

int _declspec(naked) fastfloor2(float x)
{
	static int t;
	__asm
	{
		fld [esp+4]
		fsub _0_5
		//frndint
		fistp t
		mov eax, t
		ret
	}
}
*/


static AudioEffect* effect = 0;

static UINT UniqueId = 0x1337;
static char TempPathName[1024];
static char ModuleFileName[1024]; // path to v2edit library

bool oome = false;

/*
ViruzVstiProgram::ViruzVstiProgram() {
	strcpy (name, "Basic");
}*/ 

ViruzVsti::ViruzVsti(audioMasterCallback audioMaster)
	: AudioEffectX(audioMaster,kNumPrograms,kNumParams)
{	
	ParamInfos = NULL;

#ifdef SINGLECHN
	GetTempFileName(TempPathName,"v2se",UniqueId++,ModuleName);
#else
	GetTempFileName(TempPathName,"v2e",UniqueId++,ModuleName);
#endif
  GetLastError();
	CopyFile(ModuleFileName,ModuleName,FALSE);
  char buffer[128]; sprintf(buffer,"error %d\n",GetLastError()); OutputDebugString(buffer);
  OutputDebugString(ModuleFileName);
  OutputDebugString("\n");
  OutputDebugString(ModuleName);
  OutputDebugString("\n");
	module = LoadLibrary(ModuleName); // load v2edit
//	module = LoadLibrary(ModuleFileName); // load v2edit
  if(module) {
		// map functions
		v2vstiInit = (V2VSTI_INIT_FUNC)GetProcAddress(module,"v2vstiInit");
		v2vstiShutdown = (V2VSTI_SHUTDOWN_FUNC)GetProcAddress(module,"v2vstiShutdown");
		v2vstiProcess = (V2VSTI_PROCESS_FUNC)GetProcAddress(module,"v2vstiProcess");
		v2vstiProcessReplacing = (V2VSTI_PROCESS_REPLACING_FUNC)GetProcAddress(module,"v2vstiProcessReplacing");
		v2vstiProcessEvents = (V2VSTI_PROCESS_EVENTS_FUNC)GetProcAddress(module,"v2vstiProcessEvents");
		v2vstiGetChunk = (V2VSTI_GET_CHUNK_FUNC)GetProcAddress(module,"v2vstiGetChunk");
		v2vstiSetChunk = (V2VSTI_SET_CHUNK_FUNC)GetProcAddress(module,"v2vstiSetChunk");
		v2vstiGetPatchNames = (V2VSTI_GET_PATCH_NAMES_FUNC)GetProcAddress(module,"v2vstiGetPatchNames");
		v2vstiSetProgram = (V2VSTI_SET_PROGRAM_FUNC)GetProcAddress(module,"v2vstiSetProgram");
		v2vstiGetParameterDefs = (V2VSTI_GET_PARAM_DEFS_FUNC)GetProcAddress(module,"v2vstiGetParameterDefs");
		v2vstiGetParameters = (V2VSTI_GET_PARAMS_FUNC)GetProcAddress(module,"v2vstiGetParameters");
		v2vstiUpdateUI = (V2VSTI_UPDATE_UI_FUNC)GetProcAddress(module,"v2vstiUpdateUI");
		v2vstiSetSampleRate = (V2VSTI_SET_SAMPLERATE_FUNC)GetProcAddress(module,"v2vstiSetSampleRate");
		v2GetHost = (V2::GetHostFunc)GetProcAddress(module,"v2GetHost");

		if(!(v2vstiInit)) {
			lrBox("Invalid v2edit.dle\n");
			v2vstiShutdown = NULL;
		} else {
			host = &v2GetHost();
			host->SetClient(*this);
			editor = v2vstiInit(this);
			PatchNames = v2vstiGetPatchNames();
			v2vstiGetParameterDefs(&v2topics,&v2ntopics,&v2parms,&v2nparms,&v2gtopics,&v2ngtopics,&v2gparms,&v2ngparms);
			v2vstiGetParameters(&soundmem,&globals);

			numParams = v2nparms+v2ngparms;
			cEffect.numParams = numParams;
			SetupParamInfos();

			setProgram (0);
			if (audioMaster)
			{
				setNumInputs (0);
				setNumOutputs (kNumOutputs);
				canProcessReplacing ();
				hasVu (false);
				hasClip (false);
				isSynth ();
				programsAreChunks();
#ifdef SINGLECHN
				setUniqueID ('fV2s');
#else
				setUniqueID ('frV2');
#endif
			}
		}
	} else
		lrBox("Couldn't load v2edit.dle\n");
	suspend ();	
}

ViruzVsti::~ViruzVsti() {
	if(ParamInfos)
		delete[] ParamInfos;
	if(v2vstiShutdown)
		v2vstiShutdown(editor);
	editor = NULL; // crashes without this
	if(module)
		FreeLibrary(module);	
	DeleteFile(ModuleName);
}

//------------------------------------------------------------------------
void ViruzVsti::setParameterAutomated(long index, float value)
{	
	//setParameter(index, value);
	if(audioMaster)
		audioMaster(&cEffect, audioMasterAutomate, index, 0, 0, value);		// value is in opt
}
//------------------------------------------------------------------------

void ViruzVsti::SetupParamInfo(ParamInfo* pi)
{
		if(pi->ctltype != VCTL_MB)
			return;
		int width=50;
		int n=1;
		int max=7;
		char *s,*sp=pi->ctlstr;

		if (sp[0]=='!')
		{
			sp++;
			max=3;
		}

		do
		{
			s=sp+1;
			sp=strchr(s,'|');
			if (sp) n++;
		} while (sp);

	  if (n>max) width=max*width/n;
		
		s=pi->ctlstr[0]=='!'?pi->ctlstr+1:pi->ctlstr;
		char s2[256];
		for (int i=0; i<128; i++)
		{
			if (!*s)
				break;
			char *sx=strchr(s,'|');
			if (sx)
			{
				strncpy(s2,s,(sx-s));
				s2[sx-s]=0;
			}
			else
				strcpy(s2,s);
			s+=strlen(s2);
			while (*s=='|') s++;
			pi->controlstrings.push_back(s2);
		}
}
void ViruzVsti::SetupParamInfos()
{
	ParamInfo* pi = ParamInfos = new ParamInfo[numParams];
	V2PARAM* psi = v2parms;
	int pnum = 0;
	for(int t=0;t<v2ntopics;t++) {
		for(int p=0;p<v2topics[t].no;p++) {
			memcpy(pi,psi,sizeof(V2PARAM));
			memcpy(pi->paramname,v2topics[t].name2,2);
			pi->paramname[2]=' ';
			strcpy(pi->paramname+3,pi->name);
			SetupParamInfo(pi);
			pi++; psi++;
		}		
	}
	psi = v2gparms;
	for(int t=0;t<v2ngtopics;t++) {
		for(int p=0;p<v2gtopics[t].no;p++) {
			memcpy(pi,psi,sizeof(V2PARAM));
			memcpy(pi->paramname,v2gtopics[t].name2,2);
			pi->paramname[2]=' ';
			strcpy(pi->paramname+3,pi->name);
			SetupParamInfo(pi);
			pi++; psi++;
		}		
	}
}
//------------------------------------------------------------------------
void ViruzVsti::process(float **inputs, float **outputs, long sampleframes) 
{
	v2vstiProcess(inputs,outputs,sampleframes);
}
//------------------------------------------------------------------------
void ViruzVsti::processReplacing(float **inputs, float **outputs, long sampleframes)
{
	v2vstiProcessReplacing(inputs,outputs,sampleframes);
}
//------------------------------------------------------------------------
long ViruzVsti::processEvents(VstEvents* events) 
{
	return v2vstiProcessEvents(events);	
}
//------------------------------------------------------------------------
void ViruzVsti::setProgram (long program)
{
	/*
	ViruzVstiProgram *ap = &programs[program];*/
	v2vstiSetProgram(editor,(UINT)program);
	curProgram = program;
}

//-----------------------------------------------------------------------------------------
void ViruzVsti::setProgramName (char *name)
{
	char nb[128];
	strcpy(nb,name);
	_strupr(nb);
	if (strlen(nb)>3 &&
		  ((nb[0]>='0' && nb[0]<='9') || (nb[0]>='A' && nb[0]<='F')) &&
			((nb[1]>='0' && nb[1]<='9') || (nb[1]>='A' && nb[1]<='F')) &&
			nb[2]==' ')
		name+=3;
	strcpy (&PatchNames[curProgram*32],name);	
	v2vstiSetProgram(editor,(UINT)curProgram);
}

//-----------------------------------------------------------------------------------------
void ViruzVsti::getProgramName (char *name)
{
	// für paniq: hex davor
	char buf[128];
	sprintf(buf,"%02x %s",curProgram,&PatchNames[curProgram*32]);
	memcpy(name,buf,23);
	name[24] = 0;
}

//-----------------------------------------------------------------------------------------

void ViruzVsti::getParameterLabel (long index, char *label)
{
	label[0] = 0;
}

//-----------------------------------------------------------------------------------------
void ViruzVsti::getParameterDisplay (long index, char *text)
{
	text[0] = 0;
	if(index < v2nparms) {
		int ofs = 128*4+V2SOUNDSIZE*curProgram;
		if(ParamInfos[index].ctltype == VCTL_MB)
			sprintf(text,"%s",ParamInfos[index].controlstrings[(int)soundmem[ofs+index]-(int)v2parms[index].offset].c_str());
		else
			sprintf(text,"%i",(int)soundmem[ofs+index]-(int)v2parms[index].offset);		
	} else {
		int i = index - v2nparms;
		if(ParamInfos[index].ctltype == VCTL_MB)
			sprintf(text,"%s",ParamInfos[index].controlstrings[(int)globals[i]-(int)v2gparms[i].offset].c_str());
		else
			sprintf(text,"%i",(int)globals[i]-(int)v2gparms[i].offset);		
	}
}

//-----------------------------------------------------------------------------------------
void ViruzVsti::getParameterName (long index, char *label)
{
	strcpy(label,ParamInfos[index].paramname); 
}

//-----------------------------------------------------------------------------------------
void ViruzVsti::setParameter (long index, float value)
{	
	if(index < v2nparms) {
		int ofs = 128*4+V2SOUNDSIZE*curProgram;				
//		soundmem[ofs+index] = (unsigned char)fastceil2((value * (float)v2parms[index].max)-0.5f);
		soundmem[ofs+index] = (unsigned char)float2parm(value,v2parms[index].min,v2parms[index].max);
	} else {
		int i = index-v2nparms;
//		globals[i] = (char)fastceil2((value * (float)v2gparms[i].max)-0.5f);
		globals[i] = (unsigned char)float2parm(value,v2gparms[i].min,v2gparms[i].max);
	}	
	v2vstiUpdateUI(curProgram,index);
}

//-----------------------------------------------------------------------------------------


float ViruzVsti::getParameter (long index)
{
	float value = 0.0f;
	if(index < v2nparms) {
		int ofs = 128*4+V2SOUNDSIZE*curProgram;
//		value = ((float)soundmem[ofs+index])/((float)v2parms[index].max+0.5f);
		value = parm2float(soundmem[ofs+index],v2parms[index].min,v2parms[index].max);
	} else {
		index -= v2nparms;
//		value = ((float)globals[index])/((float)v2gparms[index].max+0.5f);
		value = parm2float(globals[index],v2gparms[index].min,v2gparms[index].max);		
	}
	return value;
}
//------------------------------------------------------------------------
void ViruzVsti::setSampleRate (float sampleRate)
{
	AudioEffectX::setSampleRate (sampleRate);
	v2vstiSetSampleRate((int)sampleRate);
}

//-----------------------------------------------------------------------------------------
void ViruzVsti::setBlockSize (long blockSize)
{
	AudioEffectX::setBlockSize (blockSize);
	// you may need to have to do something here...
}

//-----------------------------------------------------------------------------------------
void ViruzVsti::resume ()
{
	wantEvents ();
}
//------------------------------------------------------------------------
bool ViruzVsti::getOutputProperties (long index, VstPinProperties* properties)
{
	if (index < kNumOutputs)
	{
		sprintf (properties->label, "V2 Out %1d", index + 1);
		properties->flags = kVstPinIsActive;
		if (index < 2)
			properties->flags |= kVstPinIsStereo;	// test, make channel 1+2 stereo
		return true;
	}
	return false;
}
//-----------------------------------------------------------------------------------------
bool ViruzVsti::getProgramNameIndexed (long category, long index, char* text)
{
	if (index < kNumPrograms)
	{
		char buf[128];
		sprintf(buf,"%02x %s",index,&PatchNames[index*32]);
		memcpy(text,buf,23);
		text[24] = 0;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------
bool ViruzVsti::copyProgram (long destination)
{
	if (destination < kNumPrograms)
	{
		int ofss = 128*4+V2SOUNDSIZE*curProgram;
		int ofsd = 128*4+V2SOUNDSIZE*destination;
		memcpy(&soundmem[ofsd],&soundmem[ofss],V2SOUNDSIZE);
		return true;
	}
	return false;
}
//-----------------------------------------------------------------------------------------
bool ViruzVsti::getEffectName (char* name)
{
#ifdef SINGLECHN
	strcpy (name, "V2 VSTi (single)");
#else
	strcpy (name, "V2 VSTi (multi)");
#endif
	return true;
}

//-----------------------------------------------------------------------------------------
bool ViruzVsti::getVendorString (char* text)
{
	strcpy (text, "farbrausch");
	return true;
}

//-----------------------------------------------------------------------------------------
bool ViruzVsti::getProductString (char* text)
{ 
#ifdef SINGLECHN
	strcpy (text, "farbrausch V2 VSTi (single)");
#else
	strcpy (text, "farbrausch V2 VSTi (multi)");
#endif
	return true; 
}

//-----------------------------------------------------------------------------------------
long ViruzVsti::canDo (char* text)
{
	if (!strcmp (text, "receiveVstEvents"))
		return 1;
	if (!strcmp (text, "receiveVstMidiEvent"))
		return 1;
	return -1;	// explicitly can't do; 0 => don't know
}
//-----------------------------------------------------------------------------------------
 
// prototype of the export function main

#pragma warning (disable: 4326)

int main (audioMasterCallback audioMaster) 
{	
	// get vst version
	if(!audioMaster (0, audioMasterVersion, 0, 0, 0, 0))
		return 0;  // old version

	effect = new ViruzVsti(audioMaster);
	if (!effect) {
		return 0;
	}
	if (oome)
	{ 
		delete effect;
		return 0;
	}
	return int(effect->getAeffect ());
}


#include <windows.h>
void* hInstance;
BOOL WINAPI DllMain (HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
{
	hInstance = hInst;
	GetModuleFileName(hInst,ModuleFileName,1024);
	int c = (int)strlen(ModuleFileName);
	char* p = ModuleFileName+c-1;	
	while((*p != '\\')&&(--c))
		--p;
	*p = 0;
	strcpy(TempPathName,ModuleFileName);
#ifdef SINGLECHN
	strcat(ModuleFileName,"\\v2edit_s.dle");
#else
	strcat(ModuleFileName,"\\v2edit.dle");
#endif

	switch(dwReason) {
		case DLL_PROCESS_ATTACH:
			{
			} break;
		case DLL_PROCESS_DETACH:
			{
			} break;
		default:
			break;
	}

	return 1;
}

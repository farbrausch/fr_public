// lr: wir sind so geil, wir machen alles in headerfiles.

#pragma once
#include "MachineInterfaceTL.h"
#include "common.h"
#include "console.h"
#include <math.h>
#include <list>


HINSTANCE g_hInstance = 0;
static char g_tempPathName[MAX_PATH];
static char g_moduleFileName[MAX_PATH]; // path to v2edit library

#ifdef _DEBUG
//Console con;
#define QC_VERBOSE_CALL() printf("%s\n",__FUNCTION__)
#else
#define QC_VERBOSE_CALL()
#define printf() 
#pragma warning(disable : 4002) // too many arguments in macro
#endif

namespace Buzz
{

class V2Interface : public V2::IClient
{
protected:
	HMODULE m_hModule;
	//static void paramChanged(AudioEffect* tag, int index);
	//void SetupParamInfo(ParamInfo* pi);
	//void SetupParamInfos();

public:
	class V2Machine
		: public Machine<V2Machine>
	{
	public:
		CMachine* m_pMachine;

		enum Parameters
		{
			ParametersChannel = 0,            
			ParametersProgram,
		};

		enum TrackParameters
		{
			TrackParametersNote = 0,
			TrackParametersVolume,
			TrackParametersCCNumber,
			TrackParametersCCValue,
		};

		DECLARE_MACHINE()
		{
			IsGenerator();
			MonoToStereo();

			Name("farbrausch V2");
			ShortName("V2");
			Author("Tammo Hinrichs and Leonard Ritter");

			Commands("&Instrument Editor...");

			// from here, the order of declaration is important
			// global
			GlobalParameter(parameterTypeByte,"Channel","MIDI Channel",0,15,255,0,parameterFlagIsState|parameterFlagTickOnEdit);
			GlobalParameter(parameterTypeByte,"Program","MIDI Program",0,127,255,0,parameterFlagIsState|parameterFlagTickOnEdit);
			// track
			TrackNoteParameter("Note","Sends a Note");
			TrackParameter(parameterTypeByte,"Volume","Current Volume",0,127,255,127,parameterFlagIsState);
			TrackParameter(parameterTypeByte,"CC#","Controller Number",0,127,255,255,parameterFlagIsState);
			TrackParameter(parameterTypeByte,"CC Value","Controller Value",0,127,255,255,parameterFlagIsState);
		}

		class Ex
			: public CMachineInterfaceEx
		{
		public:
			virtual char const *DescribeParam(int const param)
			{
				QC_VERBOSE_CALL();
				return NULL; 
			}		// use this to dynamically change name of parameter
			virtual bool SetInstrument(char const *name)
			{
				QC_VERBOSE_CALL();
				return false; 
			}

			virtual void GetSubMenu(int const i, CMachineDataOutput *pout)
			{
				QC_VERBOSE_CALL();
			}
			virtual void AddInput(char const *macname, bool stereo) 
			{
				QC_VERBOSE_CALL();
			}	// called when input is added to a machine
			virtual void DeleteInput(char const *macename)
			{
				QC_VERBOSE_CALL();
			}			

			virtual void RenameInput(char const *macoldname, char const *macnewname)
			{
				QC_VERBOSE_CALL();
			}

			virtual void Input(float *psamples, int numsamples, float amp) 
			{
				QC_VERBOSE_CALL();
			} // if MIX_DOES_INPUT_MIXING

			virtual void MidiControlChange(int const ctrl, int const channel, int const value) 
			{
				QC_VERBOSE_CALL();
			}

			virtual void SetInputChannels(char const *macname, bool stereo) 
			{
				QC_VERBOSE_CALL();
			}

			virtual bool HandleInput(int index, int amp, int pan) 
			{ 
				QC_VERBOSE_CALL();
				return false; 
			}
		};

		Ex m_ex;

		bool IsFirstInstance()
		{
			if ((*m_machines.begin()) == this)
				return true;
			else
				return false;
		}

		void Init(CMachineDataInput *const pi)
		{
			m_pMachine = pCB->GetThisMachine();
			pCB->SetMachineInterfaceEx(&m_ex);
			m_v2.SetSampleRate(pMasterInfo->SamplesPerSec);
			//= m_v2.GetChunk(chunkPtr,false);
	        
			if (pi)
			{
				int version = 0;
				pi->Read(version);
				if(version >= 0)
				{
					//printf("reading local data...\n");
					pi->Read(m_currentChannel);
					for(int i=0; i < 256; i++)
					{
						pi->Read(m_volumes[i]);
						pi->Read(m_programs[i]);
						pi->Read(m_ccNumber[i]);
						pi->Read(m_ccValue[i]);
					}
				}
				if (version >= 1)
				{
					//printf("reading main data...\n");
					for(int i=0; i < 16; i++)
					{
						pi->Read(m_v2.m_currentPrograms[i]);
					}
					int chunksize = 0;
					char* chunkPtr = 0;
					pi->Read(chunksize);
					//printf("reading %i bytes...\n",chunksize);
					chunkPtr = new char[chunksize];
					pi->Read(chunkPtr,chunksize);
					m_v2.SetChunk(chunkPtr,chunksize,false);
					delete[] chunkPtr;
				}
			}
			else
			{
				// find free channel and assign
				for(int i=0; i < 16; i++)
				{
					bool found = false;
					V2MachineList::iterator machine;
					for(machine = m_machines.begin(); machine != m_machines.end(); machine++)
					{
						if((*machine != this) && ((*machine)->m_currentChannel == i))
						{
							found = true;
							break;
						}
					}
					if (!found)
					{
						m_currentChannel = i;
						break;
					}
				}

				if (m_pMachine)
				{
					pCB->ControlChange(m_pMachine,1,0,ParametersChannel,(int)m_currentChannel);
				}
			}

			bool (V2Machine::*evHandler)(void *) = &V2Machine::OnDoubleClickMachine;
			pCB->SetEventHandler(m_pMachine, DoubleClickMachine,(EVENT_HANDLER_PTR)evHandler, this);
		}

		void ChangeLocalParameterProgram(int index)
		{
			if (m_pMachine)
			{
				pCB->ControlChange(m_pMachine,1,0,ParametersProgram,(int)index);
			}
		}

		const char* GetMachineName()
		{
			if (m_pMachine)
			{
				return pCB->GetMachineName(m_pMachine);
			}
			return NULL;
		}

		bool OnDoubleClickMachine(void* self)
		{
			OpenEditor();
			return true;
		}

		enum
		{
			LastWorkStateUnknown,
			LastWorkStateTick,
			LastWorkStateWork,
		};

		int		m_lastWorkState;
		bool	m_muted;

		void SetLastWorkState(int lastWorkState)
		{
			if ((m_lastWorkState == LastWorkStateTick)&&(lastWorkState == LastWorkStateTick))
			{
				m_muted = true;
			}
			else
			{
				m_muted = false;
			}
			m_lastWorkState = lastWorkState;
		}

		V2Machine()
		{
			m_lastWorkState = LastWorkStateUnknown;
			m_muted = false;
			if(!m_machines.size())
			{
				m_v2.InitInterface();
			}
			m_machines.push_back(this);
			m_currentChannel = 0;
			for(int i=0; i < 256; i++)
			{
				m_lastNote[i] = parameterValueNoteOff;
				m_volumes[i] = 127;
				m_programs[i] = 0;
				m_ccNumber[i] = 0;
				m_ccValue[i] = 0;
			}
		}
		
		~V2Machine()
		{
			m_machines.remove(this);
			if(!m_machines.size())
			{
				// der letzte macht das licht aus
				m_v2.DestroyInterface();
			}
		}

		int m_lastNote[256];
		Byte m_volumes[256];
		Byte m_programs[256];
		Byte m_ccNumber[256];
		Byte m_ccValue[256];
		Byte m_currentChannel;

		void SendProgramChange(int program)
		{            
			//printf("PRGM %i ON %i\n",program,m_currentChannel);
			MIDIEvent me;
			me.channel = m_currentChannel; 
			me.substatus = me.MIDIProgramChange;
			me.param1 = program;
			me.param2 = 0;  
			m_v2.m_currentPrograms[m_currentChannel] = program;
			m_v2.ProcessEvent(0,me);
			if (m_v2.m_host->GetSynth()->GetCurrentChannelIndex() == m_currentChannel)
			{
				// force update of GUI
				m_v2.m_host->SetCurrentPatchIndex(program);
			}
		}

		void SendControllerChange(int controller, int value)
		{
			//printf("CC#%i=%i ON %i\n",controller,value,m_currentChannel);
			MIDIEvent me;
			me.channel = m_currentChannel;
			me.substatus = me.MIDIControl;
			me.param1 = controller;
			me.param2 = value;
			m_ccNumber[m_currentChannel] = controller;
			m_ccValue[m_currentChannel] = value;
			m_v2.ProcessEvent(0,me);
		}

		void SendNoteOn(int note, int velocity)
		{
			if (IsMuted())
				return; 
			MIDIEvent me;
			note -= 1;
			int realnote = note%16;
			int realoctave = note/16;
			me.channel = m_currentChannel; 
			me.substatus = me.MIDINote;
			me.param1 = realnote+(realoctave*12);
			me.param2 = velocity;                           
			m_v2.ProcessEvent(0,me);            
		}

		void SendNoteOff(int note)
		{
			MIDIEvent me;
			note -= 1;
			int realnote = note%16;
			int realoctave = note/16;
			me.channel = m_currentChannel; // TODO
			me.substatus = me.MIDINoteOff;
			me.param1 = realnote+(realoctave*12);
			me.param2 = 0; // TODO
			m_v2.ProcessEvent(0,me);
		}

		void Tick()
		{
			SetLastWorkState(LastWorkStateTick);
    		Byte note;
			Byte program;
			Map(ParametersChannel,m_currentChannel);
			if (Map(ParametersProgram,program))
			{
				SendProgramChange(program);
			}
			for(int index = 0; index < GetTrackCount(); index++)
			{
				Map(index,TrackParametersVolume,m_volumes[index]);                
				Map(index,TrackParametersCCNumber,m_ccNumber[index]);
				if (Map(index,TrackParametersCCValue,m_ccValue[index]))
				{
					SendControllerChange(m_ccNumber[index],m_ccValue[index]);
				}
				if (Map(index,TrackParametersNote,note))
				{
					if(m_lastNote[index] != parameterValueNoteOff)
						SendNoteOff(m_lastNote[index]); // shut up
					if (note != parameterValueNoteOff)
					{
						SendNoteOn(note,m_volumes[index]);
					}
					m_lastNote[index] = note;
				}
			}            
		}

		void OnParameterChange(int index)
		{
		}

		// override
		void OnTrackParameterChange(int track, int index)
		{
		}

		bool Work(float* psamples, int numsamples, const int mode)
		{
			QC_VERBOSE_CALL();
			return false;
		}

		bool IsMuted()
		{
			return m_muted;
		}

		V2Machine* GetFirstNotMutedMachine()
		{
			V2MachineList::iterator machine;
			for(machine = m_machines.begin(); machine != m_machines.end(); machine++)
			{
				if(!(*machine)->IsMuted())
				{
					return *machine;
				}
			}
			return NULL;
		}

		bool WorkMonoToStereo(float *pin, float *pout, int numsamples, int const mode)
		{ 
			SetLastWorkState(LastWorkStateWork);
			//QC_VERBOSE_CALL(); 
			if (GetFirstNotMutedMachine() == this)
			{
				m_v2.Work(pout,numsamples);
				return true;
			}
			else
			{
				return false;
			}
		}

		void OpenEditor()
		{
			m_v2.ShowEditor(GetForegroundWindow());
			m_v2.SetCurrentChannel(m_currentChannel);
		}

		void Command(int const i)
		{
			//QC_VERBOSE_CALL();
			if (i==0)
			{
				OpenEditor();
			}
		}

		void Stop()
		{
			//QC_VERBOSE_CALL(); 
			for(int i=0; i < GetTrackCount(); i++)
			{
				if (m_lastNote[i] != parameterValueNoteOff)
				{
					SendNoteOff(m_lastNote[i]);
					m_lastNote[i] = parameterValueNoteOff;
				}
			}
		}

		void SaveLocalData(CMachineDataOutput * const po)
		{
			//printf("writing local data...\n");
			po->Write(m_currentChannel);
			for(int i=0; i < 256; i++)
			{
				po->Write(m_volumes[i]);
				po->Write(m_programs[i]);
				po->Write(m_ccNumber[i]);
				po->Write(m_ccValue[i]);
			}
		}

		void Save(CMachineDataOutput * const po)
		{
			QC_VERBOSE_CALL(); 
			if (IsFirstInstance())
			{
				//printf("writing main data...\n");
				po->Write((int)1); 
				SaveLocalData(po);
				void* chunkPtr = 0;
				for(int i=0; i < 16; i++)
				{
					po->Write(m_v2.m_currentPrograms[i]);
				}
				int chunksize = m_v2.GetChunk(&chunkPtr,false);
				//printf("writing %i bytes...\n",chunksize);
				po->Write(chunksize); // size
				po->Write(chunkPtr,chunksize);
			}
			else
			{
				po->Write((int)0); // nodata
				SaveLocalData(po);
			}
		}
		
		void AttributesChanged() {QC_VERBOSE_CALL(); }

		//void SetNumTracks(int const n) {QC_VERBOSE_CALL(); }
		void MuteTrack(int const i) {QC_VERBOSE_CALL(); }
		bool IsTrackMuted(int const i) { QC_VERBOSE_CALL(); return false; }

		void MidiNote(int const channel, int const value, int const velocity) {QC_VERBOSE_CALL(); }
		void Event(dword const data)
		{
			QC_VERBOSE_CALL();
		}

		char const *DescribeValue(int const param, int const value)
		{
			//printf("DescribeValue: %i %i\n",param,value);
			static char out[1024];
			if (param >= m_info.numGlobalParameters)
			{
				int tparam = param;
				tparam -= m_info.numGlobalParameters;
				switch(tparam)
				{
					case TrackParametersVolume:
						{
							sprintf(out,"%i",value);
							return out;
						} break;
					case TrackParametersCCNumber:
						{
							switch(value)
							{
								case 1:
									return "Modulation";
								case 2:
									return "Breath";
								case 3:
								case 4:
								case 5:
								case 6:
									sprintf(out,"#%i",value); break;
								case 7:
									return "Channel Volume";
								case 255:
									return "None";
								default:
									sprintf(out,"%i",value); break;
							}
							return out;
						} break;
					case TrackParametersCCValue:
						{
							switch(value)
							{
								case 255:
									return "None";
								default:
									sprintf(out,"%i",value); break;
							}
							return out;
						} break;
					default:
						break;
				}
			}
			else
			{
				switch(param)
				{
					case ParametersProgram:
						{
							sprintf(out,"%s",&m_v2.m_patchNames[value*32]);
							return out;
						} break;
					default:
						break;
				}
			}
			return NULL;
		}

		CEnvelopeInfo const **GetEnvelopeInfos() { QC_VERBOSE_CALL(); return NULL; }

		bool PlayWave(int const wave, int const note, float const volume) { QC_VERBOSE_CALL(); return false; }
		void StopWave() {QC_VERBOSE_CALL(); }
		int GetWaveEnvPlayPos(int const env) { QC_VERBOSE_CALL(); return -1; }

	};

	typedef std::list<V2Machine*> V2MachineList;
	static V2MachineList	m_machines;
	static V2Interface		m_v2;

	V2VSTI_PROCESS_FUNC             Process;
	V2VSTI_PROCESS_REPLACING_FUNC   ProcessReplacing;
	V2VSTI_PROCESS_EVENTS_FUNC      ProcessEvents;
	V2VSTI_GET_CHUNK_FUNC           GetChunk;
	V2VSTI_SET_CHUNK_FUNC           SetChunk;
	V2VSTI_GET_PATCH_NAMES_FUNC     GetPatchNames;	
	V2VSTI_SET_PROGRAM_FUNC         SetProgram;
	V2VSTI_GET_PARAM_DEFS_FUNC      GetParameterDefs;
	V2VSTI_GET_PARAMS_FUNC          GetParameters;
	V2VSTI_UPDATE_UI_FUNC           UpdateUI;
	V2VSTI_SET_SAMPLERATE_FUNC      SetSampleRate;
	V2BUZZ_INIT_FUNC                Init;
	V2BUZZ_SHUTDOWN_FUNC            Shutdown;
	V2BUZZ_PROCESS_EVENT            ProcessEvent;
	V2BUZZ_WORK                     Work;
	V2BUZZ_SHOW_EDITOR              ShowEditor;
	V2BUZZ_DESTROY_EDITOR           DestroyEditor;
	V2BUZZ_SET_CHANNEL				SetChannel;
	V2::GetHostFunc					GetHost;

	ParamInfo*      m_paramInfos;
	char*           m_patchNames;
	V2TOPIC*        m_v2topics;
	int             m_v2ntopics;
	V2PARAM*        m_v2parms;
	int             m_v2nparms;
	V2TOPIC*        m_v2gtopics;
	int             m_v2ngtopics;
	V2PARAM*        m_v2gparms;
	int             m_v2ngparms;
	unsigned char*  m_soundmem;
	char*           m_globals;
	//void*           m_editor;
	int             m_numParams;
	int             m_currentProgram;
	int             m_currentPrograms[16];
	V2::IHost*		m_host;

	static void __cdecl ParamChanged(void* tag, int index)
	{
	}

	void SetupParamInfo(ParamInfo* pi)
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
	void SetupParamInfos()
	{
		ParamInfo* pi = m_paramInfos = new ParamInfo[m_numParams];
		V2PARAM* psi = m_v2parms;
		int pnum = 0;
		for(int t=0;t<m_v2ntopics;t++) {
			for(int p=0;p<m_v2topics[t].no;p++) {
				memcpy(pi,psi,sizeof(V2PARAM));
				memcpy(pi->paramname,m_v2topics[t].name2,2);
				pi->paramname[2]=' ';
				strcpy(pi->paramname+3,pi->name);
				SetupParamInfo(pi);
				pi++; psi++;
			}		
		}
		psi = m_v2gparms;
		for(int t=0;t<m_v2ngtopics;t++) {
			for(int p=0;p<m_v2gtopics[t].no;p++) {
				memcpy(pi,psi,sizeof(V2PARAM));
				memcpy(pi->paramname,m_v2gtopics[t].name2,2);
				pi->paramname[2]=' ';
				strcpy(pi->paramname+3,pi->name);
				SetupParamInfo(pi);
				pi++; psi++;
			}		
		}
	}

	void* GetProcAddress(LPCSTR lpstrProcAddress)
	{
		void* ptr = 0;
		ptr = (void*)::GetProcAddress(m_hModule,lpstrProcAddress);
		if (!ptr)
		{
			lrBox(lpstrProcAddress);
		}
		return ptr;
	}

	void InitInterface()
	{
		m_host = NULL;
		m_paramInfos = 0;
		m_currentProgram = 0;
		memset(m_currentPrograms,0,sizeof(int)*16);

		m_hModule = LoadLibrary(g_moduleFileName); // load v2edit
		if(!m_hModule)
		{
			lrBox(g_moduleFileName);
		}
		// map functions
		Init = (V2BUZZ_INIT_FUNC)GetProcAddress("v2buzzInit");
		Shutdown = (V2BUZZ_SHUTDOWN_FUNC)GetProcAddress("v2buzzShutdown");
		Process = (V2VSTI_PROCESS_FUNC)GetProcAddress("v2vstiProcess");
		ProcessReplacing = (V2VSTI_PROCESS_REPLACING_FUNC)GetProcAddress("v2vstiProcessReplacing");
		ProcessEvents = (V2VSTI_PROCESS_EVENTS_FUNC)GetProcAddress("v2vstiProcessEvents");
		GetChunk = (V2VSTI_GET_CHUNK_FUNC)GetProcAddress("v2vstiGetChunk");
		SetChunk = (V2VSTI_SET_CHUNK_FUNC)GetProcAddress("v2vstiSetChunk");
		GetPatchNames = (V2VSTI_GET_PATCH_NAMES_FUNC)GetProcAddress("v2vstiGetPatchNames");
		SetProgram = (V2VSTI_SET_PROGRAM_FUNC)GetProcAddress("v2vstiSetProgram");
		GetParameterDefs = (V2VSTI_GET_PARAM_DEFS_FUNC)GetProcAddress("v2vstiGetParameterDefs");
		GetParameters = (V2VSTI_GET_PARAMS_FUNC)GetProcAddress("v2vstiGetParameters");
		UpdateUI = (V2VSTI_UPDATE_UI_FUNC)GetProcAddress("v2vstiUpdateUI");
		SetSampleRate = (V2VSTI_SET_SAMPLERATE_FUNC)GetProcAddress("v2vstiSetSampleRate");        
		ProcessEvent = (V2BUZZ_PROCESS_EVENT)GetProcAddress("v2buzzProcessEvent");
		Work = (V2BUZZ_WORK)GetProcAddress("v2buzzWork");
		ShowEditor = (V2BUZZ_SHOW_EDITOR)GetProcAddress("v2buzzShowEditor");
		DestroyEditor = (V2BUZZ_DESTROY_EDITOR)GetProcAddress("v2buzzDestroyEditor");
		SetChannel = (V2BUZZ_SET_CHANNEL)GetProcAddress("v2buzzSetChannel");
		GetHost = (V2::GetHostFunc)GetProcAddress("v2GetHost");

		if (Init)
		{
			m_host = &GetHost();
			m_host->SetClient(*this);
			Init();
			m_patchNames = GetPatchNames();
			GetParameterDefs(&m_v2topics,&m_v2ntopics,&m_v2parms,&m_v2nparms,&m_v2gtopics,&m_v2ngtopics,&m_v2gparms,&m_v2ngparms);
			GetParameters(&m_soundmem,&m_globals);

			m_numParams = m_v2nparms+m_v2ngparms;

			SetupParamInfos();

		}
	}

	void SetCurrentChannel(int currentChannel)
	{
		SetChannel(currentChannel);
	}

	void SetCurrentProgram(int currentProgram)
	{
		m_currentProgram = currentProgram;
		SetProgram(NULL,m_currentProgram);
	}

	int GetCurrentProgram()
	{
		return m_currentProgram;
	}

	void SetParameter(int index, float value)
	{
		if(index < m_v2nparms)
		{
			int ofs = 128*4+V2SOUNDSIZE*m_currentProgram;
			m_soundmem[ofs+index] = (unsigned char)float2parm(value,m_v2parms[index].min,m_v2parms[index].max);
		}
		else
		{
			int i = index-m_v2nparms;
			m_globals[i] = (unsigned char)float2parm(value,m_v2gparms[i].min,m_v2gparms[i].max);
		}	        
		UpdateUI(m_currentProgram,index);
	}


	float GetParameter(int index)
	{
		float value = 0.0f;
		if(index < m_v2nparms)
		{
			int ofs = 128*4+V2SOUNDSIZE*m_currentProgram;
			value = parm2float(m_soundmem[ofs+index],m_v2parms[index].min,m_v2parms[index].max);
		} else
		{
			index -= m_v2nparms;
			value = parm2float(m_globals[index],m_v2gparms[index].min,m_v2gparms[index].max);
		}
		return value;
	}

	V2Interface()
	{
	}

	void Reset()
	{
		DestroyInterface();
		InitInterface();
	}

	V2Machine* FindMachineByChannel(int index)
	{
		V2MachineList::iterator machine;
		for(machine = m_machines.begin(); machine != m_machines.end(); machine++)
		{
			if((*machine)->m_currentChannel == index)
			{
				return *machine;
			}
		}
		return NULL;
	}

	void DestroyInterface()
	{
		if (m_hModule)
		{
			if(DestroyEditor)
			{
				DestroyEditor();
			}
			if(m_paramInfos)
			{
				delete[] m_paramInfos;
				m_paramInfos = NULL;
			}
			if(Shutdown)
			{
				Shutdown();
				Shutdown = 0;
			}
			if (m_host)
			{
				m_host->Release();
				m_host = NULL;
			}
			//m_editor = NULL; // crashes without this
			FreeLibrary(m_hModule);
			m_hModule = NULL;
		}
	}

	~V2Interface()
	{
	}

	/// IClient implementation

	virtual void CurrentParameterChanged(int index)
	{
	}

	// the gui has changed the current channel
	virtual void CurrentChannelChanged()
	{
	}

	// the gui has changed the current patch
	virtual void CurrentPatchChanged()
	{
		int channel = m_host->GetSynth()->GetCurrentChannelIndex();
		int program = m_host->GetSynth()->GetCurrentPatchIndex();
		
		V2Machine* machine = FindMachineByChannel(channel);
		if (machine)
		{
			machine->ChangeLocalParameterProgram(program);
		}
	}

	// return a local channel name if available
	virtual const char* GetChannelName(int channelIndex)
	{
		V2Machine* machine = FindMachineByChannel(channelIndex);
		if (machine)
		{
			return machine->GetMachineName();
		}
		return NULL;
	}

};

__declspec(selectany) V2Interface V2Interface::m_v2;
__declspec(selectany) V2Interface::V2MachineList V2Interface::m_machines;

} // namespace Buzz

EXPORT_MACHINE(Buzz::V2Interface::V2Machine)

BOOL WINAPI DllMain (HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
{
 	g_hInstance = hInst;
	GetModuleFileName(hInst,g_moduleFileName,1024);
	int c = (int)strlen(g_moduleFileName);
	char* p = g_moduleFileName+c-1;	
	while((*p != '\\')&&(--c))
		--p;
	*p = 0;
	strcpy(g_tempPathName,g_moduleFileName);
#ifdef SINGLECHN
	strcat(g_moduleFileName,"\\v2edit_s.dle");
#else
	strcat(g_moduleFileName,"\\v2edit.dle");
#endif

	switch(dwReason) {
		case DLL_PROCESS_ATTACH:
			{
				Buzz::V2Interface::V2Machine::Prepare();
			} break;
		case DLL_PROCESS_DETACH:
			{
				Buzz::V2Interface::V2Machine::Cleanup();
 			} break;
		default:
			break;
	}

	return 1;
}
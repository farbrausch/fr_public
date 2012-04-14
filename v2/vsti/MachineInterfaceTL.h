#pragma once

// MachineInterfaceTL.h 1.1
// templates for buzz machines
// by leonard ritter (paniq@gmx.net)
// http://www.quence.com

// whats missing:
// - support for attributes

#include "MachineInterface.h"
#include <vector>
#include <stdio.h>

namespace Buzz
{
	typedef unsigned char Byte;
	typedef unsigned short Word;
	typedef unsigned long DWord;

	float const Pi = 3.14159265358979323846f;

	enum
	{
		version						= 15,

		maxBufferLength				= 256,// in number of samples

		// machine types
		machineTypeMaster			= 0,
		machineTypeGenerator		= 1,
		machineTypeEffect			= 2,

		// special parameter values
		parameterValueNoteNone		= 0,
		parameterValueNoteOff		= 255,
		parameterValueNoteMin		= 1, // C-0
		parameterValueNoteMax		= ((16 * 9) + 12), // B-9

		parameterValueSwitchNone	= 255,
		parameterValueSwitchOff		= 0,
		parameterValueSwitchOn		= 1,

		parameterValueWaveNone		= 0,
		parameterValueWaveMin		= 1,
		parameterValueWaveMax		= 200,

		// CMachineParameter flags
		parameterFlagIsWave			= 1 << 0,
		parameterFlagIsState		= 1 << 1,
		parameterFlagTickOnEdit		= 1 << 2,

		// CMachineInfo flags
		infoFlagMonoToStereo		= 1 << 0,
		infoFlagPlaysWaves			= 1 << 1,
		infoFlagUsesLibInterface	= 1 << 2,
		infoFlagUsesInstruments		= 1 << 3,
		infoFlagDoesInputMixing		= 1 << 4,
		infoFlagNoOutput			= 1 << 5, // used for effect machines that don't actually use any outputs (WaveOutput, AuxSend etc.)
		infoFlagControlMachine		= 1 << 6, // used to control other (non MIF_CONTROL_MACHINE) machines
		infoFlagInternalAux			= 1 << 7, // uses internal aux bus (jeskola mixer and jeskola mixer aux)

		// work modes
		workModeNoIO				= 0,
		workModeRead				= 1,
		workModeWrite				= 2,
		workModeReadWrite			= 3,

		// state flags
		stateFlagPlaying			= 1,
		stateFlagRecording			= 2,

		// event types
		eventTypeDoubleClick		= 0,
		eventTypeDelete				= 1,

		// parameter types
		parameterTypeNote			= 0,
		parameterTypeSwitch			= 1,
		parameterTypeByte			= 2,
		parameterTypeWord			= 3,
	};

	class Parameter
		: public CMachineParameter
	{
	public:
		int m_offset;
		bool m_isFloat;

		Parameter(int type, const char* name, const char* description, int minValue, int maxValue, int noValue, int defaultValue, int flags=0)
		{
			m_isFloat = false;
			Type = (CMPType)type;
			Name = name;
			Description = description;
			MinValue = minValue;
			MaxValue = maxValue;
			NoValue = noValue;
			DefValue = defaultValue;
			Flags = flags;
		}
	};

	typedef std::vector<Parameter*> Parameters;

	class Info
		: public CMachineInfo
	{
	public:
		Info()
		{
		}
	};

	template<typename TOwner>
	class Machine
		: public CMachineInterface
	{
	public:
		typedef TOwner ThisMachineClass;

		static Parameters m_parameters;
		static Info m_info;
		static int m_globalValuesOffset;
		static int m_trackValuesOffset;
		static int m_attribValuesOffset;
        static int m_trackParameterSize;
        int m_numTracks;
		char* m_globalValues;
		char* m_trackValues;
		char* m_attribValues;        

		Machine()
		{
			if(m_globalValuesOffset != -1)
			{				
				GlobalVals = (void*)(((char*)this)+m_globalValuesOffset);
				m_globalValues = 0;
			}
			else
			{
				if(m_info.numGlobalParameters)
					m_globalValues = new char[GetGlobalParameterBlockSize()];
				else
					m_globalValues = 0;
				GlobalVals = m_globalValues;

			}
			if(m_trackValuesOffset != -1)
			{
				TrackVals = (void*)(((char*)this)+m_trackValuesOffset);
				m_trackValues = 0;
			}
			else
			{
                if(m_info.numTrackParameters)
                    m_trackValues = new char[GetTrackParameterBlockSize()*m_info.maxTracks];
                else
                    m_trackValues = 0;
                TrackVals = m_trackValues;
			}
			if(m_attribValuesOffset != -1)
			{
				AttrVals = (int*)(((char*)this)+m_trackValuesOffset);
			}
			else
			{
				AttrVals = 0;
				m_attribValues = 0;
			}

            m_numTracks = m_info.minTracks;
		}

		virtual ~Machine()
		{
			if(m_trackValues)
				delete m_trackValues;
			if(m_globalValues)
				delete m_globalValues;
			if(m_attribValues)
				delete m_attribValues;
		}

		// to be called from inside DeclareInfo
		static void IsGenerator()
		{
			m_info.Type = machineTypeGenerator;
		}

		static void IsEffect()
		{
			m_info.Type = machineTypeEffect;
		}

		static void SetInfoFlag(int infoFlag)
		{
			m_info.Flags |= infoFlag;
		}

		static void MonoToStereo() { SetInfoFlag(infoFlagMonoToStereo); }
		static void PlaysWaves() { SetInfoFlag(infoFlagPlaysWaves); }
		static void UsesLibInterface() { SetInfoFlag(infoFlagUsesLibInterface); }
		static void UsesInstruments() { SetInfoFlag(infoFlagUsesInstruments); }
		static void DoesInputMixing() { SetInfoFlag(infoFlagDoesInputMixing); }
		static void NoOutput() { SetInfoFlag(infoFlagNoOutput); }
		static void ControlMachine() { SetInfoFlag(infoFlagControlMachine); }
		static void InternalAux() { SetInfoFlag(infoFlagInternalAux); }

		static void Tracks(int min=1, int max=256)
		{
			m_info.minTracks = min;
			m_info.maxTracks = max;
		}

		static void Name(const char* name)
		{
			m_info.Name = name;
			if(!m_info.ShortName)
				m_info.ShortName = name;
		}

		static void ShortName(const char* shortName)
		{
			m_info.ShortName = shortName;
			if(!m_info.Name)
				m_info.Name = shortName;
		}

		static void Author(const char* author)
		{
			m_info.Author = author;
		}

		static void Commands(const char* commands)
		{
			m_info.Commands = commands;
		}

		inline void* GetParameterValuePtr(int index)
		{
			return (void*)(m_globalValues + m_parameters[index]->m_offset);
		}

		inline void* GetParameterValuePtr(int track, int index)
		{
            index += m_info.numGlobalParameters;
            return (void*)(m_trackValues + (m_trackParameterSize*track) + m_parameters[index]->m_offset);
		}

		template<typename T>
		inline T GetValue(int index)
		{
			return *((T*)GetParameterValuePtr(index));
		}

		template<typename T>
		inline T GetValue(int track, int index)
		{
			return *((T*)GetParameterValuePtr(track,index));
		}

		inline int GetWord(int index)
		{
			return (int)GetValue<Word>(index);
		}

		inline int GetWord(int track, int index)
		{
			return (int)GetValue<Word>(track,index);
		}

		inline int GetByte(int index)
		{
			return (int)GetValue<Byte>(index);
		}

		inline int GetByte(int track, int index)
		{
			return (int)GetValue<Byte>(track,index);
		}

		inline bool GetSwitch(int index)
		{
			return GetValue<Byte>(index)?true:false;
		}

		inline bool GetSwitch(int track, int index)
		{
			return GetValue<Byte>(track,index)?true:false;
		}

		inline int GetNote(int index)
		{
			return (int)GetValue<Byte>(index);
		}

		inline int GetNote(int track, int index)
		{
			return (int)GetValue<Byte>(track, index);
		}

		float ScaleNormalToFloat(float f, float min, float max)
		{
			return ((max-min)*f)+min;
		}

		float ScaleFloatToNormal(float f, float min, float max)
		{
			return (f-min)/(max-min);
		}

		inline float GetFloat(int index)
		{
			return ((float)GetValue<Word>(index))/32768.0f;
		}

		inline float GetFloat(int track, int index)
		{
			return ((float)GetValue<Word>(track,index))/32768.0f;
		}

		inline float GetFloat(int index, float min, float max)
		{
			return ScaleNormalToFloat(((float)GetValue<Word>(index))/32768.0f,min,max);
		}

		inline float GetFloat(int track, int index, float min, float max)
		{
			return ScaleNormalToFloat(((float)GetValue<Word>(track,index))/32768.0f,min,max);
		}

		inline float WordToFloat(int w, float min=0.0f, float max=1.0f)
		{
			return ScaleNormalToFloat((float)w / 32768.0f,min,max);
		}

		template<typename T>
		inline bool IsValid(int index)
		{
			return (GetValue<T>(index) != m_parameters[index]->NoValue);
		}

		template<typename T>
		inline bool IsValid(int track, int index)
		{            
			return (GetValue<T>(track,index) != m_parameters[index+m_info.numGlobalParameters]->NoValue);
		}

		template<typename T>
		inline bool Map(int index, T& target)
		{
			bool r = IsValid<T>(index);
			if(r)
				target = GetValue<T>(index);
			return r;
		}

		template<typename T>
		inline bool Map(int track, int index, T& target)
		{
			bool r = IsValid<T>(track,index);
			if(r)
				target = GetValue<T>(track,index);
			return r;
		}

		inline bool Map(int index, float& target)
		{
			bool r = IsValid<Word>(index);
			if(r)
				target = GetFloat(index);
			return r;
		}

		inline bool Map(int track, int index, float& target)
		{
			bool r = IsValid<Word>(track,index);
			if(r)
				target = GetFloat(track,index);
			return r;
		}

		inline bool Map(int index, float& target, float min, float max)
		{
			bool r = IsValid<Word>(index);
			if(r)
				target = GetFloat(index,min,max);
			return r;
		}

		inline bool Map(int track, int index, float& target, float min, float max)
		{
			bool r = IsValid<Word>(track,index);
			if(r)
				target = GetFloat(track,index,min,max);
			return r;
		}

		inline bool Map(int index, bool& target)
		{
			bool r = IsValid<Byte>(index);
			if(r)
				target = GetSwitch(index);
			return r;
		}

		inline bool Map(int track, int index, bool& target)
		{
			bool r = IsValid<Byte>(track,index);
			if(r)
				target = GetSwitch(track,index);
			return r;
		}

		const char *DescribeValue(const int param, const int value)
		{
			static char txt[32];

			switch((int)(m_parameters[param]->Type))
			{
				case parameterTypeSwitch: 
					{
						return (value == parameterValueSwitchOff)?"Off":"On";
					} break;
				case parameterTypeWord: 
					{
						if(m_parameters[param]->m_isFloat)
						{
							sprintf(txt,"%.4f",WordToFloat(value));
							return txt;
						}
						else
							return NULL;
					} break;
				default:
					{
						return NULL;
					} break;
			}					
		}

		static int AssignGlobalParameterOffsets()
		{
			int size = 0;
			if(m_info.numGlobalParameters)
			{
				for(int i=0; i!=m_info.numGlobalParameters; i++)
				{
					m_parameters[i]->m_offset = size;
					switch((int)(m_parameters[i]->Type))
					{
						case parameterTypeByte: size += sizeof(Byte); break;
						case parameterTypeNote: size += sizeof(Byte); break;
						case parameterTypeSwitch: size += sizeof(Byte); break;
						case parameterTypeWord: size += sizeof(Word); break;
					}					
				}
			}
			return size;
		}

        int GetSizeForType(int type)
        {
            switch(type)
            {
				case parameterTypeByte: return sizeof(Byte);
				case parameterTypeNote: return sizeof(Byte);
				case parameterTypeSwitch: return sizeof(Byte);
				case parameterTypeWord: return sizeof(Word);
                default:
                    return 1;
            }
        }

		static int AssignTrackParameterOffsets()
		{
			int size = 0;
            if(m_info.numTrackParameters)
			{
                for(int i=0; i!=m_info.numTrackParameters; i++)
				{
					m_parameters[i+m_info.numGlobalParameters]->m_offset = size;
					switch((int)(m_parameters[i+m_info.numGlobalParameters]->Type))
					{
						case parameterTypeByte: size += sizeof(Byte); break;
						case parameterTypeNote: size += sizeof(Byte); break;
						case parameterTypeSwitch: size += sizeof(Byte); break;
						case parameterTypeWord: size += sizeof(Word); break;
					}
				}
			}
			return size;
		}

        static int AssignParameterOffsets()
        {
            int globalSize = AssignGlobalParameterOffsets();
            int trackSize = AssignTrackParameterOffsets();
            m_trackParameterSize = trackSize;
            return globalSize+trackSize;
        }

		static int GetGlobalParameterBlockSize(int untilParam=65536)
		{
			int size = 0;
			if(m_info.numGlobalParameters)
			{
				for(int i=0; i!=m_info.numGlobalParameters; i++)
				{
					if(i == untilParam)
						break;
					switch((int)(m_parameters[i]->Type))
					{
						case parameterTypeByte: size += sizeof(Byte); break;
						case parameterTypeNote: size += sizeof(Byte); break;
						case parameterTypeSwitch: size += sizeof(Byte); break;
						case parameterTypeWord: size += sizeof(Word); break;
					}					
				}
			}
			return size;
		}

		static int GetTrackParameterBlockSize(int untilParam=65536)
		{
			int size = 0;
			if(m_info.numTrackParameters)
			{
				for(int i=0; i!=m_info.numTrackParameters; i++)
				{
					if(i == untilParam)
						break;
					switch((int)(m_parameters[i+m_info.numGlobalParameters]->Type))
					{
						case parameterTypeByte: size += sizeof(Byte); break;
						case parameterTypeNote: size += sizeof(Byte); break;
						case parameterTypeSwitch: size += sizeof(Byte); break;
						case parameterTypeWord: size += sizeof(Word); break;
					}					
				}
			}
			return size;
		}

        // override
        void OnParameterChange(int index)
        {
        }

        // override
        void OnTrackParameterChange(int track, int index)
        {
        }

		void Tick()
		{
            for(int index=0; index < m_info.numGlobalParameters; index++)
            {
                switch(m_parameters[index]->Type)
                {
					case parameterTypeByte:
                        {
                            if (IsValid<Byte>(index))
                                ((TOwner*)this)->OnParameterChange(index);
                        } break;
					case parameterTypeNote:
                        {
                            if (IsValid<Byte>(index))
                                ((TOwner*)this)->OnParameterChange(index);
                        } break;
					case parameterTypeSwitch: 
                        {
                            if (IsValid<Byte>(index))
                                ((TOwner*)this)->OnParameterChange(index);
                        } break;
					case parameterTypeWord: 
                        {
                            if (IsValid<Word>(index))
                                ((TOwner*)this)->OnParameterChange(index);
                        } break;
                }
            }

            for(int track=0; track < m_numTracks; track++)
            {
                for(int index=0; index < m_info.numTrackParameters; index++)
                {
                    switch(m_parameters[m_info.numGlobalParameters+index]->Type)
                {
					case parameterTypeByte:
                        {
                            if (IsValid<Byte>(track,index))
                                ((TOwner*)this)->OnTrackParameterChange(track,index);
                        } break;
					case parameterTypeNote:
                        {
                            if (IsValid<Byte>(track,index))
                                ((TOwner*)this)->OnTrackParameterChange(track,index);
                        } break;
					case parameterTypeSwitch: 
                        {
                            if (IsValid<Byte>(track,index))
                                ((TOwner*)this)->OnTrackParameterChange(track,index);
                        } break;
					case parameterTypeWord: 
                        {
                            if (IsValid<Word>(track,index))
                                ((TOwner*)this)->OnTrackParameterChange(track,index);
                        } break;
                }
                }
            }
        }

		static Parameter* GlobalParameter(int parameterType, const char* name, const char* description, int parameterValueMin, int parameterValueMax, int parameterValueNone, int parameterValueDefault, int parameterFlags=parameterFlagIsState)
		{
			Parameter* p = new Parameter(parameterType,name,description,parameterValueMin,parameterValueMax,parameterValueNone,parameterValueDefault,parameterFlags);
			m_parameters.push_back(p);
			m_info.numGlobalParameters++;
			return p;
		}

		static void GlobalNoteParameter(const char* name, const char* description)
		{
            GlobalParameter(parameterTypeNote,name,description,parameterValueNoteMin,parameterValueNoteMax,parameterValueNoteNone,parameterValueNoteNone,0);
		}

		static void GlobalParameter(const char* name, const char* description, float parameterValueDefault, int parameterFlags=parameterFlagIsState)
		{
			Parameter* p = GlobalParameter(parameterTypeWord,name,description,0,32768,65535,(int)(parameterValueDefault*32768.0f),parameterFlags);
			p->m_isFloat = true;
		}

		static void GlobalParameter(const char* name, const char* description, bool parameterValueDefault, int parameterFlags=parameterFlagIsState)
		{
			GlobalParameter(parameterTypeSwitch,name,description,parameterValueSwitchOff,parameterValueSwitchOn,parameterValueSwitchNone,parameterValueDefault?parameterValueSwitchOn:parameterValueSwitchOff,parameterFlags);
		}

		static Parameter* TrackParameter(int parameterType, const char* name, const char* description, int parameterValueMin, int parameterValueMax, int parameterValueNone, int parameterValueDefault, int parameterFlags=parameterFlagIsState)
		{
			Parameter* p = new Parameter(parameterType,name,description,parameterValueMin,parameterValueMax,parameterValueNone,parameterValueDefault,parameterFlags);
			m_parameters.push_back(p);
			m_info.numTrackParameters++;
			return p;

		}

		static void TrackNoteParameter(const char* name, const char* description)
		{
			TrackParameter(parameterTypeNote,name,description,parameterValueNoteMin,parameterValueNoteMax,parameterValueNoteNone,parameterValueNoteNone,parameterFlagTickOnEdit);
		}

		static void TrackParameter(const char* name, const char* description, float parameterValueDefault, int parameterFlags=parameterFlagIsState)
		{
			Parameter* p = TrackParameter(parameterTypeWord,name,description,0,32768,65535,(int)(parameterValueDefault*32768.0f),parameterFlags);
			p->m_isFloat = true;
		}

		static void GlobalValues(void* v)
		{
			m_globalValuesOffset = (int)v;
		}

		static void TrackValues(void* v)
		{
			m_trackValuesOffset = (int)v;
		}

		static void AttribValues(void* v)
		{
			m_attribValuesOffset = (int)v;
		}

		// override
		static void DeclareInfo()
		{
		}

		static void Prepare()
		{
			PrepareInfo();			
		}

		static void PrepareInfo()
		{
			memset(&m_info,0,sizeof(Info));
			TOwner::DeclareInfo();
			m_info.Version = version;
			//_ASSERT(!m_parameters.empty());
			m_info.Parameters = (const CMachineParameter**)(&m_parameters[0]);
			if(m_info.numTrackParameters)
            {
                if (!m_info.minTracks)
                {
				    Tracks();
                }
            }
			// TODO: prepare attributes?
			
			if(!m_info.Name)
			{
				Name("A Stupid Machine");
				ShortName("Stoopid");
			}
			if(!m_info.Author)
				Author("Anonymous");

			AssignParameterOffsets();
		}

		static void Cleanup()
		{
			if(!m_parameters.empty())
			{
				Parameters::iterator i;
				for(i = m_parameters.begin(); i != m_parameters.end(); ++i)
				{
					delete (*i);
				}
				m_parameters.clear();
			}
		}

        int GetTrackCount()
        {
            return m_numTracks;
        }

	    void SetNumTracks(int const n)
        {
            m_numTracks = n;
        }

	};

	template<typename TOwner>
	__declspec(selectany) Parameters Machine<TOwner>::m_parameters;
	template<typename TOwner>
	__declspec(selectany) Info Machine<TOwner>::m_info;
	template<typename TOwner>
	__declspec(selectany) int Machine<TOwner>::m_globalValuesOffset = -1;
	template<typename TOwner>
	__declspec(selectany) int Machine<TOwner>::m_trackValuesOffset = -1;
	template<typename TOwner>
	__declspec(selectany) int Machine<TOwner>::m_attribValuesOffset = -1;
	template<typename TOwner>
	__declspec(selectany) int Machine<TOwner>::m_trackParameterSize = 0;

#define STARTING_AT(identifier) \
	((void*)(&(((ThisMachineClass*)0)->identifier)))

#define DECLARE_MACHINE() \
		static void DeclareInfo()		

#define EXPORT_MACHINE(className) \
	extern "C" { \
		__declspec(dllexport) CMachineInfo const * __cdecl GetInfo() \
		{ \
			return &className::m_info; \
		} \
		__declspec(dllexport) CMachineInterface * __cdecl CreateMachine() \
		{ \
			CMachineInterface* mi = new className; \
			return mi; \
		} \
	}

};

#pragma once

#include "../types.h"
#include "midi.h"
#include "v2interfaces.h"
#include <cmath>
#include <cassert>

// V2 API functions
// uni- and simplifies access to patch memory, modulations and other settings

// Synth inherits from V2::Host and implements more functions of V2::IHost
// in case you want to expose any more functions of Synth to wrappers,
// just extend IHost for the functions that are already declared here.

#include "v2parameters.h"


namespace V2
{
	enum
	{
		TopicCount					=  v2ntopics,
		ParameterCount				=  v2nparms,

		GlobalTopicCount			=  v2ngtopics,
		GlobalParameterCount		=  v2ngparms,

		PatchCount					= 128, 
		MaxModulationCount			= 255,

		ClipFrameCount				= 5,
	};


	struct Patch
	{
		Parameter		parameters[ParameterCount];
		unsigned char	modulationCount;
		Modulation		modulations[MaxModulationCount];
	};

	struct Globals
	{
		Parameter		parameters[GlobalParameterCount];
	};

	struct Bank
	{
		PatchOffset		patchOffsets[PatchCount];
		Patch			patches[PatchCount];
	};

	template<int>
	struct GenerateCompilerErrorIf { enum { Result = _generic_compiler_error_ }; };

	template<>
	struct GenerateCompilerErrorIf<false> { enum { Result = false }; };

	// just make sure nothing else gets inbetween
	// our local memory mappings
	enum
	{
		GlobalsSizeCheck = GenerateCompilerErrorIf<sizeof(Globals) != sizeof(v2initglobs)>::Result,
		PatchSizeCheck = GenerateCompilerErrorIf<sizeof(Patch) != v2soundsize>::Result,
		InitPatchSizeCheck = GenerateCompilerErrorIf<sizeof(Patch) != sizeof(v2initsnd)>::Result,
		BankSizeCheck = GenerateCompilerErrorIf<sizeof(Bank) != smsize>::Result,
	};

	// wrap in a class for future modular extensions
	class Synth : public InteractsWithIClientImpl< ISynth >
	{
	protected:
		CString					m_modulationTargetNames[ParameterCount];
		bool					m_initialized;
		int						m_clipLeft;
		int						m_clipRight;
		int						m_channelIndex;
		IGUI*					m_gui;

		void PrepareModulationTargetNames()
		{
			int offset = 0;
			for(int topicIndex = 0; topicIndex < GetTopicCount(); topicIndex++)
			{
				for (int index = 0; index < GetTopicInfo(topicIndex)->GetParameterCount(); index++)
				{
					int parameterIndex = offset + index;
					m_modulationTargetNames[parameterIndex] = GetTopicInfo(topicIndex)->GetName() + CString(_T(" ")) + GetParameterInfo(parameterIndex)->GetName();
				}
				offset += GetTopicInfo(topicIndex)->GetParameterCount();
			}
		}

		void SetModulationCount(int patchIndex, int modulationCount)
		{
			GetPatch(patchIndex)->modulationCount = modulationCount;
		}

		void SetModulationCount(int modulationCount)
		{
			SetModulationCount(GetCurrentPatchIndex(),modulationCount);
		}

	public:
		Synth()
		{
			m_initialized = false;
			m_clipLeft = 0;
			m_clipRight = 0;
			m_channelIndex = 0;
			m_gui = 0;
		}

		// returns the GUI interface
		virtual IGUI* GetGUI()
		{
			assert(m_gui);
			return m_gui;
		}

		int GetCurrentChannelIndex()
		{
			return m_channelIndex;
		}

		void SetCurrentChannelIndex(int channelIndex, bool silent=false)
		{
			if (m_channelIndex == channelIndex)
				return;
			m_channelIndex = channelIndex;
			SetCurrentPatchIndex(GetPGMOfChannel(channelIndex), true); // retain the client update
			if (!silent)
			{
				// all values are updated, now send all messages
				GetClient()->CurrentChannelChanged();
				GetClient()->CurrentPatchChanged();
			}
		}

		void Initialize()
		{
			if (m_initialized)
				return;
			PrepareModulationTargetNames();
		}

		CString &GetModulationTargetName(int parameterIndex)
		{
			return m_modulationTargetNames[parameterIndex];
		}

		int GetParameterCount()
		{
			return ParameterCount;
		}

		int GetGlobalParameterCount()
		{
			return GlobalParameterCount;
		}

		int GetPatchCount()
		{
			return PatchCount;
		}

		int GetTopicCount()
		{
			return TopicCount;
		}

		int GetGlobalTopicCount()
		{
			return GlobalTopicCount;
		}

		Bank* GetBank()
		{
			return (Bank*)soundmem;
		}

		Patch* GetPatch(int index)
		{
			return (Patch*)(soundmem+((DWORD*)soundmem)[index]);
			//return (Patch*)&GetBank()->patches[index];
		}

		Globals* GetGlobals()
		{
			return (Globals*)globals;
		}

		int GetCurrentPatchIndex()
		{
			return v2curpatch;
		}

		void SetCurrentPatchIndex(int currentPatch, bool silent = false)
		{
			if (currentPatch == v2curpatch)
				return;
			v2curpatch = currentPatch;
#ifdef SINGLECHN
			msSetProgram1(v2curpatch);
#endif
			if (!silent)
				GetClient()->CurrentPatchChanged();
		}

		TopicInfo* GetTopicInfo(int index)
		{
			return (TopicInfo*)(&v2topics[index]);
		}

		TopicInfo* GetGlobalTopicInfo(int index)
		{
			return (TopicInfo*)(&v2gtopics[index]);
		}

		ParameterInfo* GetParameterInfo(int parameterIndex)
		{
			return (ParameterInfo*)(&v2parms[parameterIndex]);
		}

		ParameterInfo* GetGlobalParameterInfo(int parameterIndex)
		{
			return (ParameterInfo*)(&v2gparms[parameterIndex]);
		}

		int FindTopicByParameter(int parameterIndex)
		{
			int offset = 0;
			for(int index = 0; index < GetTopicCount(); index++)
			{
				offset += GetTopicInfo(index)->GetParameterCount();
				if (parameterIndex < offset)
				{
					return index;
				}
			}
			return -1;
		}

		int GetParameterOffset(int topicIndex)
		{
			int offset = 0;
			for(int index = 0; index < topicIndex; index++)
			{
				offset += GetTopicInfo(index)->GetParameterCount();
			}
			return offset;
		}

		CString GetPatchName(int patchIndex)
		{
			return patchnames[patchIndex];
		}

		void SetPatchName(int patchIndex, LPCTSTR patchName)
		{
			CString name = patchName;
			if (name.GetLength() >= 32)
			{
				name = name.Left(31);
			}
			_tcscpy(patchnames[patchIndex],name);
		}

		int ClampParameterValue(int parameterIndex, int value)
		{
			return ClampValue(value,GetParameterInfo(parameterIndex));
		}

		int GetPatchParameter(int patchIndex, int parameterIndex)
		{
			int ret = (int)GetPatch(patchIndex)->parameters[parameterIndex] - GetParameterInfo(parameterIndex)->GetAlignOffset();
			return ret;
		}

		void SetPatchParameter(int patchIndex, int parameterIndex, int value)
		{
			GetPatch(patchIndex)->parameters[parameterIndex] = (char)(ClampParameterValue(parameterIndex,value) + GetParameterInfo(parameterIndex)->GetAlignOffset());
		}

		int GetParameter(int parameterIndex)
		{
			return GetPatchParameter(GetCurrentPatchIndex(),parameterIndex);
		}

		void SetParameter(int parameterIndex, int value)
		{
			SetPatchParameter(GetCurrentPatchIndex(),parameterIndex,value);
		}

		int ClampGlobalParameterValue(int parameterIndex, int value)
		{
			return ClampValue(value,GetGlobalParameterInfo(parameterIndex));
		}

		int GetLogoColor()
		{
			// CHAOS: colorized logo background
			// lr: i really think this should be wrapped differently, but well.
			return GetGlobalParameterCount()-1;
		}

		int GetGlobalParameter(int parameterIndex)
		{
			int ret = (int)GetGlobals()->parameters[parameterIndex] - GetGlobalParameterInfo(parameterIndex)->GetAlignOffset();
			return ret;
		}

		void SetGlobalParameter(int parameterIndex, int value)
		{
			GetGlobals()->parameters[parameterIndex] = (char)(ClampGlobalParameterValue(parameterIndex,value) + GetGlobalParameterInfo(parameterIndex)->GetAlignOffset());
			synthSetGlobals(theSynth,GetGlobals());
		}

		int GetModulationCount(int patchIndex)
		{
			return GetPatch(patchIndex)->modulationCount;
		}

		int GetModulationCount()
		{
			return GetModulationCount(GetCurrentPatchIndex());
		}

		Modulation* GetPatchModulation(int patchIndex, int modulationIndex)
		{
			return &GetPatch(patchIndex)->modulations[modulationIndex];
		}

        Modulation* GetModulation(int modulationIndex)
		{
			return GetPatchModulation(GetCurrentPatchIndex(),modulationIndex);
		}

		int AddModulation(int patchIndex)
		{
			if (GetModulationCount(patchIndex) < MaxModulationCount)
			{
				SetModulationCount(patchIndex,GetModulationCount(patchIndex)+1);			
			}			
			int index = GetModulationCount(patchIndex)-1;
			GetPatchModulation(patchIndex,index)->Initialize();
			return index;
		}

		int AddModulation()
		{
			return AddModulation(GetCurrentPatchIndex());
		}

		void DeleteModulation(int patchIndex, int modulationIndex)
		{
			int modulationCount = GetModulationCount(patchIndex);
			SetModulationCount(patchIndex,0); // avoid player mismatch
			for (int index = modulationIndex; index < (modulationCount-1); ++index)
			{
				*GetPatchModulation(patchIndex,index) = *GetPatchModulation(patchIndex,index+1);
			}
			SetModulationCount(patchIndex,modulationCount-1);
		}

		void DeleteModulation(int modulationIndex)
		{
			DeleteModulation(GetCurrentPatchIndex(),modulationIndex);
		}

		int GetModulationSourceCount()
		{
			return v2nsources;
		}

		CString GetModulationSourceName(int sourceIndex)
		{
			return v2sources[sourceIndex];
		}

		void Reset()
		{
		#ifdef RONAN
			msStartAudio(0,soundmem,globals,(const char **)speechptrs);
		#else
			msStartAudio(0,soundmem,globals);
		#endif
		}

		bool IsRecording()
		{
			return msIsRecording()?true:false;
		}

		void ToggleRecording()
		{
			SetRecording(!IsRecording());
		}

		void StartRecording()
		{
			SetRecording(true);
		}

		void StopRecording()
		{
			SetRecording(false);
		}

		void SetRecording(bool recording)
		{
			if (IsRecording() == recording)
				return;
			if (IsRecording())
			{
				msStopRecord();				
			}
			else
			{
				msStartRecord();				
			}
		}

		int WriteRecording(file& f)
		{
			return msWriteLastRecord(f);
		}

		int WriteRecording(LPCTSTR filePath)
		{
			int res = sFALSE;
			fileS moduleFile;
			if (moduleFile.open(filePath,fileS::cr|fileS::wr))
			{
				res = WriteRecording(moduleFile);
				moduleFile.close();
			}
			return res;
		}		

		void GetVUData(float& dbLeft, bool& clipLeft, float& dbRight, bool& clipRight)
		{
			int vuClipLeft, vuClipRight;

			msGetLD(&dbLeft,&dbRight,&vuClipLeft,&vuClipRight);
			if (dbLeft)
				dbLeft = (float)(18.0f*(log(dbLeft)/log(2.0f)));
			else
				dbLeft = -100;
			if (dbRight)
				dbRight = (float)(18.0f*(log(dbRight)/log(2.0f)));
			else
				dbRight = -100;

			if (vuClipLeft) 
			{
				m_clipLeft = ClipFrameCount;
			}
			if (vuClipRight)
			{
				m_clipRight = ClipFrameCount;
			}

			if (m_clipLeft)
			{
				m_clipLeft--;
			}

			if (m_clipRight)
			{
				m_clipRight--;
			}

			clipLeft = m_clipLeft?true:false;
			clipRight = m_clipRight?true:false;
		}

		bool GetPolyphonyStats(int* statBuffer, int elementCount)
		{
			synthGetPoly(theSynth,statBuffer);
			return true;
		}

		bool GetPGMStats(int* statBuffer, int elementCount)
		{
			synthGetPgm(theSynth,statBuffer);
			return true;
		}

		int GetPGMOfChannel(int channelIndex)
		{
			int statBuffer[16];
			if (!GetPGMStats(statBuffer,16))
				return 0;
			return statBuffer[channelIndex];
		}

		int GetCPULoad()
		{
			return msGetCPUUsage();
		}

#ifdef RONAN
		CString EnglishToPhonemes(LPCTSTR english)
		{
			TCHAR phonemes[4096];			
			e2p_initio((TCHAR*)english,phonemes);
			e2p_main();
			return phonemes;
		}

		CString GetSpeechText(int speechTextIndex)
		{
			return speech[speechTextIndex];
		}

		void SetSpeechText(int speechTextIndex, LPCTSTR speechText)
		{
			_tcscpy(speech[speechTextIndex],speechText);
		}
#endif


	};

	__declspec(selectany) Synth g_theSynth;

	Synth* GetTheSynth()
	{
		g_theSynth.Initialize();
		return &g_theSynth;
	}

}; // V2
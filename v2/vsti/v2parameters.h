#pragma once

#include "../sounddef.h"

#ifdef RONAN
	extern "C" extern void e2p_main();
	extern "C" extern void e2p_initio(char *a_in, char *a_out);
#endif

// V2 parameter wrappers

namespace V2
{
	class TopicInfo
		: public V2TOPIC
	{
	public:
		int GetParameterCount()
		{
			return no;
		}

		CString GetName()
		{
			return name;
		}

		CString GetShortName()
		{
			return name2;
		}
	};

	enum ControlType
	{
		ControlTypeSlider	= VCTL_SLIDER,
		ControlTypeRadio	= VCTL_MB,
		ControlTypeHidden	= VCTL_SKIP,
	};

	class ParameterInfo
		: public V2PARAM
	{
	public:
		ControlType GetControlType()
		{
			return (ControlType)ctltype;
		}

		int GetVersion()
		{
			return version;
		}

		CString GetName()
		{
			return name;
		}

		int GetOptions(CString* strings, int stringCount)
		{
			if (GetControlType() != ControlTypeRadio)
				return 0;
			// making sushi                    
			CString controlText = ctlstr;
			CString token = _T("");
			int index = 0;
			int currentPosition = 0;
			if (controlText[0] == '!')
				currentPosition = 1; // format control characters? HAH! we dont need no silly format control characters!
	        
			token = controlText.Tokenize(_T("|"),currentPosition);
			while ((token != _T("")) && (index < stringCount))
			{
				strings[index] = token;
				token = controlText.Tokenize(_T("|"),currentPosition);
				index++;
			}	
			return index;
		}

		bool IsModulatorDestination()
		{
			return (isdest&1)?true:false;
		}

		bool HasSeparator()
		{
			return (isdest&128)?true:false;
		}

		int GetMinimum()
		{
			return min - offset;
		}

		int GetMaximum()
		{
			return max - offset;
		}

		int GetOffset()
		{
			return GetMinimum();
		}

		int GetRange()
		{
			return max - min + 1;
		}

		int GetAlignOffset()
		{
			return offset;
		}
	};

	typedef unsigned char	Parameter;
	typedef unsigned char	ParameterIndex;
	typedef unsigned char	ControllerIndex;
	typedef void*			PatchOffset;

	struct Modulation
	{
		ControllerIndex	source;
		Parameter		factor;
		ParameterIndex	target;

		void Initialize()
		{
			SetSourceIndex(0);
			SetTargetIndex(0);
			SetFactor(0);
		}

		int GetTargetIndex()
		{
			return target;
		}

		void SetTargetIndex(int targetIndex)
		{
			target = (ParameterIndex)targetIndex;
		}

		int GetSourceIndex()
		{
			return source;
		}

		void SetSourceIndex(int sourceIndex)
		{
			source = (ControllerIndex)sourceIndex;
		}

		void SetFactor(int factor)
		{
			factor += 64;
			if (factor > 127)
			{
				this->factor = 127;
			}
			else if (factor < 0)
			{
				this->factor = 0;
			}
			else
			{
				this->factor = factor;
			}
		}

		int GetFactor()
		{
			return factor - 64;
		}
	};

	static int ClampValue(int value, ParameterInfo* info)		
	{
		int min = info->GetMinimum();
		int max = info->GetMaximum();
		if (value < min)
			value = min;
		if (value > max)
			value = max;
		return value;
	}	

}; // V2
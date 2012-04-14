#pragma once

#include <gdiplus.h>
#pragma comment(lib,"gdiplus.lib")

#include "v2interfaces.h"
#include "v2parameters.h"

#include <map>
#include <xutility>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace V2
{
	namespace GUI
	{
		using namespace Gdiplus;

		// modify GetColor to alter color translation
		enum ColorPreset
		{
			ColorPresetCustom = 0, // local values are preferred

			ColorPresetBackground1,
			ColorPresetBackground2,

			ColorPresetSliderOutline,
			ColorPresetSliderFill1,
			ColorPresetSliderFill2,
			ColorPresetSliderKnobFill1,
			ColorPresetSliderKnobFill2,
			ColorPresetSliderKnobFillFocused1,
			ColorPresetSliderKnobFillFocused2,
			ColorPresetSliderKnobOutline,
			ColorPresetSliderText,

			ColorPresetGroupFill1,
			ColorPresetGroupFill2,
			ColorPresetGroupOutline,
			ColorPresetGroupCaptionFill1,
			ColorPresetGroupCaptionFill2,
			ColorPresetGroupCaptionText,
			ColorPresetGroupText,

			ColorPresetLabelFill1,
			ColorPresetLabelFill2,
			ColorPresetLabelOutline,
			ColorPresetLabelText,

			ColorPresetButtonFill1,
			ColorPresetButtonFill2,
			ColorPresetButtonFillFocused1,
			ColorPresetButtonFillFocused2,
			ColorPresetButtonFillSelected1,
			ColorPresetButtonFillSelected2,
			ColorPresetButtonOutline,
			ColorPresetButtonText,

			ColorPresetVUOutline,
			ColorPresetVUFill,
			ColorPresetVUVolumeLow,
			ColorPresetVUVolumeMedium,
			ColorPresetVUVolumeHigh,

			ColorPresetLogoBackgroundFill1,
			ColorPresetLogoBackgroundFill2,
		};

		static int MakeByte(float normal)
		{
			int value = (int)(normal*255.0f);
			if(value > 255)
				return 255;
			if (value < 0)
				return 0;
			return value;
		}

		static void MakeColorRGB(const Color& color, float& hue, float& saturation, float& brightness)
		{
			float minValue,maxValue,delta;
			float r,g,b;

			r = (float)color.GetRed()/255.0f;
			g = (float)color.GetGreen()/255.0f;
			b = (float)color.GetBlue()/255.0f;

			minValue = std::min(std::min(r,g),b);
			maxValue = std::max(std::max(r,g),b);
			brightness = maxValue;				// v
			  
			delta = maxValue - minValue;
			  
			if( maxValue != 0 )
				saturation = delta / maxValue;		// s
			else {
				// r = g = b = 0		// s = 0, v is undefined
				saturation = 0;
				hue = 0;
				return;
			}

			if (delta == 0)
				hue = 0;
			else if( r == maxValue )
				hue = ( g - b ) / delta;		// between yellow & magenta
			else if( g == maxValue )
				hue = 2 + ( b - r ) / delta;	// between cyan & yellow
			else
				hue = 4 + ( r - g ) / delta;	// between magenta & cyan
			  
			hue /= 6;				// linear
		}

		static Color MakeColorHSB(float hue, float saturation, float brightness)
		{
			if (saturation == 0.0f)
			{                    
				return Color(MakeByte(brightness),MakeByte(brightness),MakeByte(brightness));
			}
			float fraction,p,q,t,h;
			int i;
			h = hue*6.0f;
			i = (int)h;
			fraction = h - (float)i;
			p = brightness * (1.0f - saturation);
			q = brightness * (1.0f - saturation * fraction);
			t = brightness * (1.0f - saturation * (1.0f - fraction));
			switch(i)
			{
				case 0: return Color(MakeByte(brightness),MakeByte(t),MakeByte(p));
				case 1: return Color(MakeByte(q),MakeByte(brightness),MakeByte(p));
				case 2: return Color(MakeByte(p),MakeByte(brightness),MakeByte(t));
				case 3: return Color(MakeByte(p),MakeByte(q),MakeByte(brightness));
				case 4: return Color(MakeByte(t),MakeByte(p),MakeByte(brightness));
				case 5: return Color(MakeByte(brightness),MakeByte(p),MakeByte(q));
			}
			return Color(MakeByte(brightness),MakeByte(p),MakeByte(q));
		}

		struct HSB
		{
			float hue;
			float saturation;
			float brightness;

			float& operator [](int index)
			{
				assert((index>=0)&&(index<3));
				switch(index)
				{
					case 0: return hue;
					case 1: return saturation;
					case 2: return brightness;
				}
				return *reinterpret_cast<float*>((void*)0);
			}

			HSB()
			{
				hue = saturation = brightness = 0;
			}

			HSB& operator =(const HSB& hsb)
			{
				hue = hsb.hue;
				saturation = hsb.saturation;
				brightness = hsb.brightness;
				return *this;
			}

			HSB(const HSB& hsb)
			{
				hue = hsb.hue;
				saturation = hsb.saturation;
				brightness = hsb.brightness;
			}

			HSB(const Color& color)
			{
				MakeColorRGB(color, hue, saturation, brightness);
			}

			operator Color()
			{
				return MakeColorHSB(hue,saturation,brightness);
			}
		};

		static HSB GetDefaultColor(ColorPreset colorPreset)
		{
			switch(colorPreset)
			{
				case ColorPresetBackground1: return Color(192,192,192);
				case ColorPresetBackground2: return Color(176,176,176);
				
				case ColorPresetSliderOutline: return Color(128,128,128);
				case ColorPresetSliderFill1: return Color(192,192,192);
				case ColorPresetSliderFill2: return Color(176,176,176);
				case ColorPresetSliderKnobFill1: return Color(255,255,255);
				case ColorPresetSliderKnobFill2: return Color(192,192,192);
				case ColorPresetSliderKnobFillFocused1: return Color(255,255,255);
				case ColorPresetSliderKnobFillFocused2: return Color(220,220,220);
				case ColorPresetSliderKnobOutline: return Color(0,0,0);
				case ColorPresetSliderText: return Color(0,0,0);
				
				case ColorPresetGroupFill1: return Color(210,210,210);
				case ColorPresetGroupFill2: return Color(194,194,194);
				case ColorPresetGroupOutline: return Color(0,0,0);
				case ColorPresetGroupCaptionFill1: return Color(0,0,0); 
				case ColorPresetGroupCaptionFill2: return Color(0,0,0);
				case ColorPresetGroupCaptionText: return Color(255,255,255);
				case ColorPresetGroupText: return Color(0,0,0);
				
				case ColorPresetLabelFill1: return Color(192,192,192);
				case ColorPresetLabelFill2: return Color(176,176,176);
				case ColorPresetLabelOutline: return Color(128,128,128);
				case ColorPresetLabelText: return Color(0,0,0);
				
				case ColorPresetButtonFill1: return Color(255,255,255);
				case ColorPresetButtonFill2: return Color(192,192,192);
				case ColorPresetButtonFillFocused1: return Color(255,255,255);
				case ColorPresetButtonFillFocused2: return Color(220,220,220);
				case ColorPresetButtonFillSelected1: return MakeColorHSB(0.12f,0.1f,1.0f);
				case ColorPresetButtonFillSelected2: return MakeColorHSB(0.12f,0.1f,0.9f);
				case ColorPresetButtonOutline: return Color(128,128,128);
				case ColorPresetButtonText: return Color(0,0,0);
				
				case ColorPresetVUOutline: return Color(0,0,0);
				case ColorPresetVUFill: return MakeColorHSB(0.12f,0.1f,0.3f);
				case ColorPresetVUVolumeLow: return MakeColorHSB(0.12f,0.1f,1.0f);
				case ColorPresetVUVolumeMedium: return Color(255,255,0);
				case ColorPresetVUVolumeHigh: return Color(255,0,0);

				case ColorPresetLogoBackgroundFill1: return Color(210,210,210);
				case ColorPresetLogoBackgroundFill2: return Color(194,194,194);

				default: return Color(0,0,0);
			}
		}

		typedef std::map<ColorPreset,HSB> PresetToHSBMap;
		PresetToHSBMap g_colorMap;

		void ResetPresetToHSBMap()
		{
			g_colorMap.clear();
		}

		HSB& GetHSBFromMap(ColorPreset colorPreset)
		{
			PresetToHSBMap::iterator iterator = g_colorMap.find(colorPreset);
			if (iterator != g_colorMap.end())
				return iterator->second;
			g_colorMap.insert(PresetToHSBMap::value_type(colorPreset,GetDefaultColor(colorPreset)));
			iterator = g_colorMap.find(colorPreset);
			assert(iterator != g_colorMap.end());
			return iterator->second;
		}

		static Color GetColor(ColorPreset colorPreset)
		{
			return GetHSBFromMap(colorPreset);
		}

		// stuff for gui appearance
		// although this is not really synthish
		// we are going to use topic and param too
		namespace Appearance
		{

			enum
			{
				// for regular stuff
				F_COLOR_H,
				F_COLOR_S,
				F_COLOR_B,
				// for gradients
				F_COLOR_HG,
				F_COLOR_SG,
				F_COLOR_BG,
			};

			struct ParameterInfo
			{
				char  *id;
				char  *name;
				V2CTLTYPES ctltype;
				int	  offset, min, max;
				int   isdest;
				char  *ctlstr;
				int   subtype;
				int   flags;

				CString GetID() const
				{
					return id;
				}

				int GetFlags() const
				{
					return flags;
				}

				int GetSubType() const
				{
					return subtype;
				}

				int GetGradientDifference() const
				{
					return isdest;
				}

				ColorPreset GetColorPreset() const
				{
					return (ColorPreset)flags;
				}

				ControlType GetControlType() const
				{
					return (ControlType)ctltype;
				}

				CString GetName() const
				{
					return name;
				}

				int GetOptions(CString* strings, int stringCount) const
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

				bool HasSeparator() const
				{
					return (isdest&128)?true:false;
				}

				int GetMinimum() const
				{
					return min - offset;
				}

				int GetMaximum() const
				{
					return max - offset;
				}

				int GetOffset() const
				{
					return GetMinimum();
				}

				int GetRange() const
				{
					return max - min + 1;
				}

				int GetAlignOffset() const
				{
					return offset;
				}
			} ;

			// V2 GUI Appearance Topics

			const V2TOPIC topics[] = {
				{  3, "Background Color", "" },

				{  3, "Slider Background Outline Color", "" },
				{  3, "Slider Background Color", "" },
				{  3, "Slider Color", "" },
				{  3, "Slider Color (Focused)", "" },
				{  3, "Slider Outline Color", "" },
				{  3, "Slider Text Color", "" },

				{  3, "Group Background Color", "" },
				{  3, "Group Background Outline Color", "" },
				{  3, "Group Caption Color", "" },
				{  3, "Group Caption Text Color", "" },
				{  3, "Group Text Color", "" },

				{  3, "Label Fill Color", "" },
				{  3, "Label Outline Color", "" },
				{  3, "Label Text Color", "" },

				{  3, "Button Color", "" },
				{  3, "Button Color (Focused)", "" },
				{  3, "Button Color (Selected)", "" },
				{  3, "Button Outline Color", "" },
				{  3, "Button Text Color", "" },

				{  3, "VU-Meter Outline Color", "" },
				{  3, "VU-Meter Background Color", "" },
				{  3, "VU-Meter Color", "" },
				//{  3, "ColorPresetVUVolumeMedium", "" },
				{  3, "VU-Meter Color (Critical)", "" },

				{  3, "Logo Background Color", ""},

			};

			// about loading/saving:
			// versioning is supported but basically not needed as 
			// long as parameter id's are not changed. unknown param ids 
			// will be skipped on either side, and only diversions
			// from the original parameter setup will be saved.
			// * note that id must be unique

			// V2 GUI Appearance Parameters

#define V2_ExposeColorPresetInfo(parameterID, colorPreset) \
			{parameterID"H","Hue",			VCTL_SLIDER,	0,		0,		255,	0,						0,		F_COLOR_H,		colorPreset  }, \
			{parameterID"S","Saturation",	VCTL_SLIDER,	0,		0,		255,	0,						0,		F_COLOR_S,		colorPreset  }, \
			{parameterID"B","Brightness",	VCTL_SLIDER,	0,		0,		255,	0,						0,		F_COLOR_B,		colorPreset  },

#define V2_ExposeGradientColorPresetInfo(parameterID, colorPreset, gradientDifference) \
			{parameterID"H","Hue",			VCTL_SLIDER,	0,		0,		255,	0,						0,		F_COLOR_HG,		colorPreset  }, \
			{parameterID"S","Saturation",	VCTL_SLIDER,	0,		0,		255,	0,						0,		F_COLOR_SG,		colorPreset  }, \
			{parameterID"B","Brightness",	VCTL_SLIDER,	0,		0,		255,	gradientDifference,		0,		F_COLOR_BG,		colorPreset  },

			const ParameterInfo parameterInfos[] = {
				// id		| name			| ctltype		|offset	|min	|max	|isdest	|ctlstr	|subtype		|flags
				V2_ExposeGradientColorPresetInfo("BGGC",	ColorPresetBackground1, -6)

				V2_ExposeColorPresetInfo(		 "SOC",		ColorPresetSliderOutline)
				V2_ExposeGradientColorPresetInfo("SFC",		ColorPresetSliderFill1, -6)
				V2_ExposeGradientColorPresetInfo("SKFC",	ColorPresetSliderKnobFill1, -25)
				V2_ExposeGradientColorPresetInfo("SKFFC",	ColorPresetSliderKnobFillFocused1, -14)
				V2_ExposeColorPresetInfo(		 "SKOC",	ColorPresetSliderKnobOutline)
				V2_ExposeColorPresetInfo(		 "STC",		ColorPresetSliderText)

				V2_ExposeGradientColorPresetInfo("GFC",		ColorPresetGroupFill1, -6)
				V2_ExposeColorPresetInfo(		 "GOC",		ColorPresetGroupOutline)
				V2_ExposeGradientColorPresetInfo("GCFC",	ColorPresetGroupCaptionFill1, -6)
				V2_ExposeColorPresetInfo(		 "GCTC",	ColorPresetGroupCaptionText)
				V2_ExposeColorPresetInfo(		 "GTC",		ColorPresetGroupText)

				V2_ExposeGradientColorPresetInfo("LFC",		ColorPresetLabelFill1, -6)
				V2_ExposeColorPresetInfo(		 "LOC",		ColorPresetLabelOutline)
				V2_ExposeColorPresetInfo(		 "LTC",		ColorPresetLabelText)

				V2_ExposeGradientColorPresetInfo("BFC",		ColorPresetButtonFill1, -25)
				V2_ExposeGradientColorPresetInfo("BFFC",	ColorPresetButtonFillFocused1, -14)
				V2_ExposeGradientColorPresetInfo("BFSC",	ColorPresetButtonFillSelected1, -10)
				V2_ExposeColorPresetInfo(		 "BOC",		ColorPresetButtonOutline)
				V2_ExposeColorPresetInfo(		 "BTC",		ColorPresetButtonText)

				V2_ExposeColorPresetInfo(		 "VOC",		ColorPresetVUOutline)
				V2_ExposeColorPresetInfo(		 "VFC",		ColorPresetVUFill)
				V2_ExposeColorPresetInfo(		 "VVLC",	ColorPresetVUVolumeLow)
				//V2_ExposeColorPresetInfo(		 "VVMC",	ColorPresetVUVolumeMedium)
				V2_ExposeColorPresetInfo(		 "VVHC",	ColorPresetVUVolumeHigh)

				V2_ExposeColorPresetInfo(		 "LBFC",	ColorPresetLogoBackgroundFill1)

			};

			enum
			{
				TopicCount = sizeof(topics)/sizeof(V2TOPIC),
				ParameterCount = sizeof(parameterInfos)/sizeof(ParameterInfo)
			};

			struct IParameter
			{
				ParameterInfo*	m_info;
				bool			m_touched;


				IParameter()
				{
					m_touched = false;
				}

				virtual void SetInfo(ParameterInfo* info)
				{
					this->m_info = info;
				}

				bool Touched()
				{ 
					return m_touched; 
				}

				void Untouch()
				{
					m_touched = false;
				}

				void Touch()
				{
					m_touched = true;
				}

				virtual int GetValue() = 0;
				virtual void SetValue(int value) = 0;
			};

			struct SimpleParameter : IParameter
			{
				int				m_value;

				virtual void SetInfo(ParameterInfo* info)
				{
					m_value = info->GetMinimum();
					__super::SetInfo(info);
				}

				int GetValue()
				{
					return m_value;
				}

				void SetValue(int value)
				{
					m_value = value;
					Touch();
				}

				SimpleParameter()
				{
					m_value = 0;
				}

			};

			template<int componentIndex>
			struct ColorParameter : IParameter
			{
				virtual int GetValue()
				{
					return (int)(GetHSBFromMap(m_info->GetColorPreset())[componentIndex]*255.0f);
				}

				virtual void SetValue(int value)
				{
					GetHSBFromMap(m_info->GetColorPreset())[componentIndex] = (float)value/255.0f;
					Touch();
				}
			};

			template<int componentIndex>
			struct ColorGradientParameter : IParameter
			{
				virtual int GetValue()
				{
					return (int)(GetHSBFromMap(m_info->GetColorPreset())[componentIndex]*255.0f);
				}

				virtual void SetValue(int value)
				{
					float value1 = (float)value/255.0f;
					float value2 = (float)value/255.0f + (float)m_info->GetGradientDifference()/100.0f;
					if (value2 < 0)
						value2 = 0;
					if (value2 > 1)
						value2 = 1;
					GetHSBFromMap(m_info->GetColorPreset())[componentIndex] = value1;
					GetHSBFromMap((ColorPreset)(m_info->GetColorPreset()+1))[componentIndex] = value2;
					Touch();
				}
			};

			struct Patch : InteractsWithIClientImpl< IGUI >
			{
				IParameter*	parameters[ParameterCount];
				unsigned char* m_savedata;

				Patch()
				{
					m_savedata = 0;
					for (int index = 0; index < ParameterCount; index++)
					{
						switch (parameterInfos[index].GetSubType())
						{
							case F_COLOR_H: parameters[index] = new ColorParameter<0>(); break;
							case F_COLOR_S: parameters[index] = new ColorParameter<1>(); break;
							case F_COLOR_B: parameters[index] = new ColorParameter<2>(); break;
							case F_COLOR_HG: parameters[index] = new ColorGradientParameter<0>(); break;
							case F_COLOR_SG: parameters[index] = new ColorGradientParameter<1>(); break;
							case F_COLOR_BG: parameters[index] = new ColorGradientParameter<2>(); break;
							default: parameters[index] = new SimpleParameter(); break;
						}
						parameters[index]->SetInfo((ParameterInfo*)&parameterInfos[index]);
					}
				}

				~Patch()
				{
					for (int index = 0; index < ParameterCount; index++)
					{
						delete parameters[index];
					}
					
					GetSaveData(0);
				}

				unsigned char* GetSaveData(size_t size)
				{
					if (m_savedata)
					{
						delete[] m_savedata;
						m_savedata = 0;
					}
					if (size)
					{
						m_savedata = new unsigned char[size];
					}
					return m_savedata;
				}

				int GetTopicCount()
				{
					return TopicCount;
				}

				TopicInfo* GetTopicInfo()
				{
					return (TopicInfo*)(&topics[0]);
				}

				ParameterInfo* GetParameterInfo(int index=0)
				{
					return (ParameterInfo*)(&parameterInfos[index]);
				}

				int GetParameterCount()
				{
					return ParameterCount;
				}

				IParameter& GetParameter(int parameterIndex)
				{
					return *parameters[parameterIndex];
				}

				void UntouchParameters()
				{
					for (int index = 0; index < ParameterCount; index++)
					{
						parameters[index]->Untouch();
					}
				}

				void Reset()
				{
					ResetPresetToHSBMap();
					UntouchParameters();
				}


				template<typename FileType>
				bool LoadAppearance(FileType& file)
				{
					Reset();
					char id[5];
					id[4] = '\0';
					if (file.read(id,4)<4) return false;
					if (strcmp(id,"v2a0")) return false;					
					int size,version;
					file.read(version);
					file.read(size);
					if (size)
					{
						unsigned char* data = new unsigned char[size];
						bool result = false;
						if (file.read(data, size) == size)
						{							
							LoadSettings(data, size, version);
							result = true;
						}
						delete[] data;
						if (!result)
							return false;
					}
					return true;
				}
				
				template<typename FileType>
				bool SaveAppearance(FileType& file)
				{
					int size,version;
					void* buffer = SaveSettings(size,version);
					if (!file.puts("v2a0")) return false;
					if (file.write(version) != sizeof(int)) return false;
					if (file.write(size) != sizeof(int)) return false;
					if (buffer)
					{
						if (file.write(buffer,size) != size) return false;
					}
					return true;
				}

				struct ParamData
				{
					char id[32];
					int value;
				};

				struct ParamDataHeader
				{
					int count;
					ParamData data[1];
				};

				// return a pointer to gui settings (not deleted by client)
				virtual void* SaveSettings(int& size, int& version)
				{					
					int touchedCount = 0;
					for (int index = 0; index < ParameterCount; index++)
					{
						if (GetParameter(index).Touched())
							touchedCount++;
					}
					size = sizeof(int) + touchedCount*sizeof(ParamData);
					version = 1;
					ParamDataHeader* header = (ParamDataHeader*)GetSaveData(size);
					header->count = touchedCount;
					int headerIndex = 0;
					for (int index = 0; index < ParameterCount; index++)
					{
						if (GetParameter(index).Touched())
						{
							assert(GetParameterInfo(index)->GetID().GetLength() < 32);
							strcpy(header->data[headerIndex].id,GetParameterInfo(index)->GetID());
							header->data[headerIndex].value = GetParameter(index).GetValue();
							headerIndex++;
						}						
					}
					return header;
				}

				// load gui settings from pointer
				virtual void LoadSettings(void* data, int size, int version) 
				{
					ParamDataHeader* header = (ParamDataHeader*)data;
					for (int headerIndex = 0; headerIndex < header->count; headerIndex++)
					{
						ParamData* data = &header->data[headerIndex];
						for (int index = 0; index < ParameterCount; index++)
						{
							if (GetParameterInfo(index)->GetID() == data->id)
							{
								GetParameter(index).SetValue(data->value);
							}
						}
					}
				}
			};

			__declspec(selectany) Appearance::Patch g_patch;
		} // namespace Appearance

		static Appearance::Patch* GetTheAppearance()
		{
			return &Appearance::g_patch;
		}
	} // namespace GUI
} // namespace V2
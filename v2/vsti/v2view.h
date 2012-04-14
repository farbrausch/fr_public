#pragma once


#include <gdiplus.h>
#include <list>
#include <vector>
#pragma comment(lib,"gdiplus.lib")

#include "multibutton.h"
#include "../sounddef.h"
#include "../types.h"
#include "../synth.h"
#include "../tool/file.h"
#include "midi.h"
#include <math.h>

#include "OffscreenPaint.h"

#include "chaos.h"

#include "v2api.h"
#include "v2appearance.h"

// lr:
// please note that none of the view objects should ever directly
//
//	* set or get global variables
//	* communicate with any top level functions
//
// this will make migration from global to local objects much easier.
//
// there is currently only one single call to GetTheSynth() which
// returns the global object which will be replaced with local
// instances soon. please do not make any more calls to this function,
// instead pass the local m_synth pointer on to child objects,
// and declare m_synth members for new classes, accompanied by
// the usual GetSets.

namespace V2
{
	namespace GUI
	{
		// current patch has changed
		UINT UWM_V2PATCH_CHANGED = ::RegisterWindowMessage(_T("UWM_V2PATCH_CHANGED"));		
		// modulation setup has changed (only used on delete)
		UINT UWM_V2MODULATION_CHANGED = ::RegisterWindowMessage(_T("UWM_V2MODULATION_CHANGED"));

		using namespace Gdiplus;

		__declspec(selectany) ULONG_PTR g_gdiplusToken = NULL;
		__declspec(selectany) ULONG_PTR g_gdiplusHookToken = NULL;
		__declspec(selectany) GdiplusStartupInput g_gdiplusStartupInput;		
		__declspec(selectany) GdiplusStartupOutput g_gdiplusStartupOutput;

		// defaults
		namespace 
		{
			inline float GetSpacing() { return 5.0f; }
			inline PointF GetSpacingPoint() { return PointF(GetSpacing(),GetSpacing()); }
			inline float GetGroupCaptionHeight() { return 16.0f; }
			inline float GetBorderSize() { return 1.0f; }
			inline SizeF GetTabSize() { return SizeF(500.0f,100.0f); }
			inline float GetMaximumTabHeight() { return 730; } // please leave this as it is (anything else breaks support for 1024x768)
			inline SizeF GetModulationTabSize() { return SizeF(300.0f,GetMaximumTabHeight()); }
			inline float GetStatusGroupHeight() { return 120.0f; }
			inline float GetScrollBoxWidth() { return 12.0f; }
			inline SizeF GetSliderSize() { return SizeF(150.0f,16.0f); }
			inline SizeF GetButtonGroupSize() { return GetSliderSize(); }
			inline SizeF GetSliderCaptionSize() { return SizeF(50,9); }
			inline SizeF GetGroupSize() { return SizeF(2*(GetSliderSize().Width+GetSliderCaptionSize().Width)+3*GetSpacing(),120); }
			inline SizeF GetSliderKnobSize() { return SizeF(8,16); }
			inline SizeF GetModulationSourceButtonSize() { return SizeF(80.0f,16.0f); }
			inline SizeF GetModulationTargetButtonSize() { return SizeF(120.0f,16.0f); }
			inline SizeF GetModulationFactorSliderSize() { return SizeF(150.0f,16.0f); }
			inline SizeF GetModulationFactorSliderKnobSize() { return GetSliderKnobSize(); }
			inline SizeF GetModulationDeleteButtonSize() { return SizeF(50.0f,16.0f); }
			inline SizeF GetModulationAddButtonSize() { return SizeF(80,16.0f); }
			inline SizeF GetScrollBoxKnobSize() { return SizeF(GetScrollBoxWidth(),GetScrollBoxWidth()); }
			inline SizeF GetPatchComboBoxSize() { return SizeF(200.0f,16.0f); }
			inline SizeF GetFileComboBoxSize() { return SizeF(40.0f,16.0f); }
			inline SizeF GetEditComboBoxSize() { return SizeF(40.0f,16.0f); }
			inline SizeF GetRecordButtonSize() { return SizeF(40.0f,16.0f); }
			inline SizeF GetChannelComboBoxSize() { return SizeF(70.0f,16.0f); }
			inline SizeF GetVUSize() { return SizeF(8.0f,40.0f); }
			inline SizeF GetCPULabelSize() { return SizeF(20.0f,16.0f); }
			inline SizeF GetVoiceLabelSize() { return SizeF(20.0f,16.0f); }
			inline SizeF GetPGMLabelSize() { return GetVoiceLabelSize(); }
			inline SizeF GetSpeechEnglishEditSize() { return SizeF(120.0f,16.0f); }
			inline SizeF GetSpeechPhoneticEditSize() { return SizeF(180.0f,16.0f); }
			inline SizeF GetSpeechEditSize() { return SizeF(GetSpacing()+GetSpeechEnglishEditSize().Width + GetSpeechPhoneticEditSize().Width + GetSliderCaptionSize().Width,16.0f); }
			inline SizeF GetLogoSpaceSize() { return SizeF(162.0f,GetVUSize().Height); }
			inline CString GetFrameWindowTitle() { return _T("farbrausch V2"); };
			inline CString GetStatusWindowText() { return _T("Here is space for an official version number."); }
			inline int GetMaxMenuEntriesPerColumn() { return 32; }
			
			inline float GetDefaultHue() { return 0.6f; }
			inline float GetCaptionSaturation() { return 0.5f; }
			inline float GetCaptionBrightnessLeft() { return 0.3f; }
			inline float GetCaptionBrightnessRight() { return 0.6f; }
		};

		enum ButtonMode
		{
			ButtonModeDefault,
			ButtonModeSwitch,
			ButtonModeRadio,
			ButtonModeTab,
		};

		typedef std::list<HWND> HWNDList;

		static void Initialize()
		{
			if (!g_gdiplusToken)
			{
				g_gdiplusStartupInput.SuppressBackgroundThread = TRUE;
				GdiplusStartup(&g_gdiplusToken, &g_gdiplusStartupInput, &g_gdiplusStartupOutput);
				g_gdiplusStartupOutput.NotificationHook(&g_gdiplusHookToken);				
			}
		}

		static void Uninitialize()
		{
			if (g_gdiplusToken)
			{
				g_gdiplusStartupOutput.NotificationUnhook(g_gdiplusHookToken);
				GdiplusShutdown(g_gdiplusToken);
			}
		}

		namespace // Tools
		{   
			// some additional math stuff

			static float sgn(float x)
			{
				if (!x)
					return 0;
				else
					return (x > 0)?1.0f:-1.0f;
			}

			static float round(float x)
			{
				return (float)((int)(x+sgn(x)*0.5f));
			}

			// rule of three/dreisatz
			static float ro3(float value, float newRange, float oldRange)
			{
				if (oldRange != 0.0f)
					return (value*newRange)/oldRange;
				else
					return 0.0f;
			}

			static PointF RO3(PointF& value, PointF& newRange, PointF& oldRange)
			{
				return PointF(
					ro3(value.X,newRange.X,oldRange.X),
					ro3(value.Y,newRange.Y,oldRange.Y)
					);
			}

			static SizeF RO3(SizeF& value, SizeF& newRange, SizeF& oldRange)
			{
				return SizeF(
					ro3(value.Width,newRange.Width,oldRange.Width),
					ro3(value.Height,newRange.Height,oldRange.Height)
					);
			}

			static SizeF GetRectSize(RectF& rect)
			{
				SizeF size;
				rect.GetSize(&size);
				return size;
			}

			static RectF Round(RectF& rectF)
			{
				return RectF(round(rectF.X),round(rectF.Y),round(rectF.Width),round(rectF.Height));
			}

			static PointF Round(PointF& pointF)
			{
				return PointF(round(pointF.X),round(pointF.Y));
			}

			static SizeF Round(SizeF& sizeF)
			{
				return SizeF(round(sizeF.Width),round(sizeF.Height));
			}

			static RectF MakeRectF(LPCRECT lpRect)
			{
				return RectF((REAL)lpRect->left,(REAL)lpRect->top,(REAL)(lpRect->right-lpRect->left),(REAL)(lpRect->bottom-lpRect->top));
			}

			static RectF MakeBevelRectF(LPCRECT lpRect)
			{
				return RectF((REAL)lpRect->left,(REAL)lpRect->top,(REAL)(lpRect->right-lpRect->left-1),(REAL)(lpRect->bottom-lpRect->top-1));
			}

			static RECT MakeRECT(RectF& rectF)
			{
				RECT rect;
				rect.left = (LONG)rectF.GetLeft();
				rect.top = (LONG)rectF.GetTop();
				rect.right = (LONG)rectF.GetRight();
				rect.bottom = (LONG)rectF.GetBottom();
				return rect;
			}

			static PointF MakePointF(const LPPOINT lpPoint)
			{
				return PointF((REAL)lpPoint->x,(REAL)lpPoint->y);
			}

			static SizeF MakeSizeF(PointF& point)
			{
				return SizeF(point.X,point.Y);
			}

			static SizeF Scale(SizeF& size, float factor)
			{
				return SizeF(size.Width*factor,size.Height*factor);
			}
		

			static RectF &DrawGradient(Graphics& graphics, RectF& rect, Color& firstColor, Color& secondColor, REAL angle=90.0f)
			{
				//SolidBrush brushBackground(GetColor(firstPreset));
				LinearGradientBrush brushBackground(
					rect,
					firstColor,
					secondColor,
					angle);
				graphics.FillRectangle(&brushBackground,rect);
				return rect;
			}

			static RectF &DrawGradient(Graphics& graphics, RectF& rect, ColorPreset firstPreset, REAL angle=90.0f)
			{				
				return DrawGradient(graphics,rect,GetColor(firstPreset),GetColor((ColorPreset)(firstPreset+1)),angle);
			}

			template<typename ParameterInfoType>
			static PointF MakePointFromParameterInfoF(ParameterInfoType* info)
			{
				return PointF((REAL)(info->GetOffset()),0);
			}

			template<typename ParameterInfoType>
			static SizeF MakeSizeFromParameterInfoF(ParameterInfoType* info)
			{
				return SizeF((REAL)(info->GetRange()),1);
			}

			static WCHAR* MakeWideCharString(const char* text)
			{
				int textLength = strlen(text);
				WCHAR* wideCharText = new WCHAR[textLength+1];
				MultiByteToWideChar(CP_ACP,0,text,-1,wideCharText,textLength+1);
				return wideCharText;
			}

			bool QuestionBox(HWND hWnd, LPCTSTR text, UINT type=MB_YESNO)
			{
				return (::MessageBox(hWnd, text,GetFrameWindowTitle(),type) == IDYES)?true:false;
			}

			int MessageBox(HWND hWnd, LPCTSTR text, UINT type=MB_OK)
			{
				return ::MessageBox(hWnd, text,GetFrameWindowTitle(),type);
			}

			int ErrorBox(HWND hWnd, LPCTSTR text)
			{
				return MessageBox(hWnd,text,MB_ICONERROR);
			}

			int InformationBox(HWND hWnd, LPCTSTR text)
			{
				return MessageBox(hWnd,text,MB_ICONEXCLAMATION);
			}

			template<typename T>
			class ScopeArrayPointer
			{
			protected:
				T* m_pointer;
			public:
				ScopeArrayPointer(T* pointer)
				{
					m_pointer = pointer;
				}
				~ScopeArrayPointer()
				{
					delete[] m_pointer;
				}

				operator T*()
				{
					return m_pointer;
				}
			};

			class SWCSPtr
				: public ScopeArrayPointer<WCHAR>
			{
			public:
				SWCSPtr(const char* text)
					: ScopeArrayPointer<WCHAR>(MakeWideCharString(text))
				{
				}
			};            

			RectF GetStringRect(LPCTSTR text, Font& font, HWND hWnd=0)
			{
				if (!hWnd)
				{
					hWnd = GetDesktopWindow();
				}
				Graphics graphics(hWnd);
				SWCSPtr ptr(text);
				RectF boundingBox;
				graphics.MeasureString(ptr,-1,&font,PointF(0,0),&boundingBox);
				return boundingBox;
			}

			class BitmapBitsLocker
			{
			protected:
				BitmapData	m_bitmapData;
				Bitmap*		m_bitmap;

			public:
				DWORD* GetBits()
				{
					return (DWORD*)m_bitmapData.Scan0;
				}

				int GetBitmapSize()
				{
					return m_bitmap->GetWidth() * m_bitmap->GetHeight();
				}

				BitmapBitsLocker(Bitmap* bmp)
				{
					m_bitmap = bmp;
          Gdiplus::Rect rect(0,0,m_bitmap->GetWidth(),m_bitmap->GetHeight());
					m_bitmap->LockBits(&rect,ImageLockModeWrite,PixelFormat32bppARGB,&m_bitmapData);
				}

				~BitmapBitsLocker()
				{
					m_bitmap->UnlockBits(&m_bitmapData);
				}
			};

			Bitmap* LoadBitmapARGB(UINT id)
			{
				Bitmap bitmap(_Module.m_hInst,(WCHAR*)MAKEINTRESOURCE(id));
				return bitmap.Clone(0,0,bitmap.GetWidth(),bitmap.GetHeight(),PixelFormat32bppARGB);
			}

			void AddAlphaChannelToBitmap(Bitmap* image, COLORREF transparentColor)
			{
				BitmapBitsLocker bbl(image);
				DWORD* bbits = bbl.GetBits();
				int bmpsize = bbl.GetBitmapSize();
				for (int i=0;i<bmpsize;i++)
				{
					if ((*bbits & 0xffffff) == (transparentColor & 0xffffff))
					{
						*bbits = 0x00000000;
					} 
					else 
					{
						*bbits = 0xff000000 | (*bbits & 0xffffff);
					}
					bbits++;
				}
			}
		}

		class MessageSystemFont
			: public CFont
		{
		public:
			MessageSystemFont()
			{
				NONCLIENTMETRICS ncm;
				ncm.cbSize = sizeof(ncm);
				SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(ncm),&ncm,0);
				CreateFontIndirect(&ncm.lfMessageFont);
			}
		};

		class MenuSystemFont
			: public CFont
		{
		public:
			MenuSystemFont()
			{
				NONCLIENTMETRICS ncm;
				ncm.cbSize = sizeof(ncm);
				SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(ncm),&ncm,0);
				CreateFontIndirect(&ncm.lfMenuFont);
			}
		};

		__declspec(selectany) HWNDList g_allWindows;
		__declspec(selectany) UINT UWM_FULLINVALIDATE = ::RegisterWindowMessage(_T("UWM_FULLINVALIDATE"));

		static void InvalidateAllWindows()
		{
			HWNDList::iterator window;
			for (window = g_allWindows.begin(); window != g_allWindows.end(); ++window)
			{
				PostMessage(*window,UWM_FULLINVALIDATE,0,0);
			}
		}

		template <typename T, typename TBase=CWindow>
		class WindowBaseTemplate
			: public CWindowImpl<T,TBase>
		{
		protected:
			RectF m_clientRect;
			RectF m_rect;

		public:
			BEGIN_MSG_MAP_EX(WindowBaseTemplate<T> )            
				MSG_WM_SIZE(OnSize)
				MSG_WM_MOVE(OnMove)
				MSG_WM_CREATE(OnCreate)
				MSG_WM_DESTROY(OnDestroy)
			END_MSG_MAP()

			BOOL OnCreate(LPCREATESTRUCT lpCreateStruct)
			{
				SetMsgHandled(FALSE);
				g_allWindows.push_back(*this);
				return TRUE;
			}

			void OnDestroy()
			{
				SetMsgHandled(FALSE);
				g_allWindows.remove(*this);
			}

			void OnSize(UINT uType, CSize size)
			{
				SetMsgHandled(FALSE);
				m_rect = GetRect();
				m_clientRect = GetClientRect();
				((T*)this)->Invalidate();
			}

			void OnMove(CPoint position)
			{
				SetMsgHandled(FALSE);
				m_rect = GetRect();
				m_clientRect = GetClientRect();                
			}

			HWND Create(HWND hWndParent, ATL::_U_RECT rect, LPCTSTR szWindowName,
					DWORD dwStyle, DWORD dwExStyle,
					ATL::_U_MENUorID MenuOrID = 0U, LPVOID lpCreateParam = NULL)
			{
				return __super::Create(hWndParent,rect,szWindowName,dwStyle,dwExStyle,MenuOrID,lpCreateParam);
			}

			HWND Create(HWND hWndParent=NULL)
			{
				UINT windowStyles = WS_VISIBLE;
				UINT windowExStyles = 0;
				if(hWndParent)
				{
					windowStyles |= WS_CHILD;
					windowStyles |= WS_CLIPCHILDREN|WS_CLIPSIBLINGS;
					//windowExStyles |= WS_EX_TRANSPARENT;
				}
				else
				{
					windowStyles |= WS_BORDER|WS_CAPTION|WS_SYSMENU;
				}
				return __super::Create(hWndParent,rcDefault,NULL,windowStyles,windowExStyles);
			}

			PointF GetPosition()
			{
				RECT rect;
				__super::GetWindowRect(&rect);
				POINT point;
				point.x = rect.left;
				point.y = rect.top;
				if(GetParent())
				{
					::ScreenToClient(GetParent(),&point);
				}
				return PointF((REAL)point.x,(REAL)point.y);
			}

			PointF GetAbsolutePosition()
			{
				RECT rect;
				__super::GetWindowRect(&rect);
				return PointF((REAL)rect.left,(REAL)rect.top);
			}

			SizeF GetSize()
			{
				RECT rect;
				__super::GetWindowRect(&rect);
				return SizeF((REAL)(rect.right-rect.left),(REAL)(rect.bottom-rect.top));
			}

			void Move(RectF& rect)
			{
				MoveWindow(&MakeRECT(rect));
				m_clientRect = GetClientRect();
			}

			RectF GetRect()
			{
				return RectF(GetPosition(),GetSize());
			}

			RectF GetAbsoluteRect()
			{
				return RectF(GetAbsolutePosition(),GetSize());
			}

			RectF GetClientRect()
			{
				RECT rect;
				__super::GetClientRect(&rect);
				return MakeRectF(&rect);
			}

			void GetClientRect(LPRECT rect)
			{
				__super::GetClientRect(rect);
			}

			RectF GetBevelClientRect()
			{
				RECT rect;
				__super::GetClientRect(&rect);
				return MakeBevelRectF(&rect);
			}

			void SetClientSize(SizeF& size)
			{
				RectF rect(GetRect());
				RectF clientRect(GetClientRect());
				SizeF rectSize;
				SizeF clientRectSize;
				rect.GetSize(&rectSize);
				clientRect.GetSize(&clientRectSize);
				SizeF border(rectSize - clientRectSize);				
				rect.Width = size.Width + border.Width;
				rect.Height = size.Height + border.Height;
				Move(rect);
			}

			CString GetWindowText()
			{
				TCHAR windowText[1025];
				__super::GetWindowText(windowText,1024);
				return windowText;
			}

			void SetWindowText(LPCTSTR text)
			{
				__super::SetWindowText(text);
				((T*)this)->Invalidate();
			}

			struct EnumChildInfo
			{
				HWND parentWindow;
				HWNDList* windowList;
				
				EnumChildInfo(HWND parentWindow, HWNDList& windowList)
				{
					this->parentWindow = parentWindow;
					this->windowList = &windowList;
				}
			};
			
			static BOOL CALLBACK EnumChildProc(HWND hWnd, LPARAM lParam)
			{
				EnumChildInfo* enumChildInfo = (EnumChildInfo*)lParam;
				if (::GetParent(hWnd) == enumChildInfo->parentWindow)
				{
					enumChildInfo->windowList->push_back(hWnd);
				}
				return TRUE;
			}
				
			void GetChildWindows(HWNDList& windows)
			{
				EnumChildInfo enumChildInfo(*this,windows);
				EnumChildWindows(*this,EnumChildProc,(LPARAM)&enumChildInfo);
			}

			RectF GetChildWindowsRect()
			{
				HWNDList windows;
				GetChildWindows(windows);
				RectF rectEnclosing(0,0,0,0);
				for(HWNDList::iterator i = windows.begin(); i != windows.end(); ++i)
				{
					Window window;
					window.Attach(*i);					
					rectEnclosing.Union(rectEnclosing,rectEnclosing,window.GetRect());
					window.Detach();
				}
				return rectEnclosing;
			}

			int MessageBox(LPCTSTR text, UINT type=MB_OK)
			{
				return V2::GUI::MessageBox(*this, text, type);
			}

			int ErrorBox(LPCTSTR text)
			{
				return V2::GUI::ErrorBox(*this,text);
			}

			int InformationBox(LPCTSTR text)
			{
				return V2::GUI::InformationBox(*this,text);
			}

		};

		template <typename T>
		class WindowTemplate
			: public WindowBaseTemplate<T>,
			public lr::GUI::OffscreenPaint<T>
		{
		protected:
			bool  m_focused;
		

		public:
			DECLARE_WND_CLASS_EX("V2Window",/*CS_OWNDC|CS_DBLCLKS|*/CS_HREDRAW|CS_VREDRAW,-1)

			WindowTemplate()
			{
				m_focused = false;
			}

			~WindowTemplate()
			{
			}

			void Invalidate()
			{
				UpdateOffscreenBitmap();
				__super::Invalidate();
			}

			BEGIN_MSG_MAP_EX(WindowTemplate<T>)            
				MSG_WM_LBUTTONDOWN(OnLButtonDown)
				MSG_WM_ERASEBKGND(OnEraseBkGnd)
				MSG_WM_PAINT(OnPaint)
				MSG_WM_SETFOCUS(OnSetFocus)
				MSG_WM_KILLFOCUS(OnKillFocus)
				MESSAGE_HANDLER_EX(UWM_FULLINVALIDATE,OnFullInvalidate)
				CHAIN_MSG_MAP(__super)
			END_MSG_MAP()
			
			LRESULT OnFullInvalidate(UINT msg, WPARAM wParam, LPARAM lParam)
			{
				Invalidate();
				return 0;
			}

			void OnSetFocus(HWND hOldWnd)
			{
				m_focused = true;
				Invalidate();
				SetMsgHandled(FALSE);
			}

			void OnKillFocus(HWND hOldWnd)
			{
				m_focused = false;
				Invalidate();
				SetMsgHandled(FALSE);
			}

			bool Focused()
			{
				return m_focused;
			}

			void OnLButtonDown(UINT uMode, CPoint& position)
			{
				SetFocus();
				SetMsgHandled(FALSE);
			}

			BOOL OnEraseBkGnd(HDC hDC)
			{
				// erase the background in the main routine
				// to reduce flickering
				return TRUE;
			}

			void OnPaint(HDC hDC)
			{
				PaintOffscreen(*this);
			}
		};

		class Window
			: public WindowTemplate<Window>
		{
		public:
		};

		template<typename T>
		class ControlTemplate
			: public WindowTemplate<T>
		{
		public:
			ControlTemplate()
			{
				//NoOffscreenPainting();
			}
		};

		class Menu
			: public CMenu
		{
		protected:
			int m_breakCount;

		public:
			void CreatePopupMenu()
			{
				m_breakCount = 0;
				__super::CreatePopupMenu();
			}

			void ModifyMenu(int id, LPCTSTR text)
			{
				__super::ModifyMenu(id+1, MF_BYCOMMAND|MF_STRING, id+1, text);
			}

			void AppendMenu(LPCTSTR text, int id)
			{
				UINT flags = MF_STRING;
				if (m_breakCount == GetMaxMenuEntriesPerColumn())
				{
					flags |= MF_MENUBARBREAK;
					m_breakCount = 0;
				}
				__super::AppendMenu(flags,(UINT)(id+1),text);
				m_breakCount++;
			}

			void AppendSeparator()
			{
				__super::AppendMenu(MF_SEPARATOR);
			}

			int Popup(HWND hWnd, PointF& position)
			{
				
				return (int)TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD,(int)position.X,(int)position.Y,hWnd) - 1;
			}
		};

		// ==========================================================
		//
		//	MeasureControl
		//
		// ==========================================================

		template<typename T>
		class MeasureControlTemplate
			: public ControlTemplate<T>
		{
		protected:
			SizeF    m_knobSize;
			SizeF    m_valueRange;
			PointF   m_valueOffset;
			PointF   m_value;

		public:
			enum AxisMode
			{
				AxisModeX = 0,
				AxisModeY = 1,
				AxisModeXY = 2,
			};

			MeasureControlTemplate()
			{                
				m_knobSize = SizeF(16,16);
			}

			PointF GetValue()
			{
				return m_value;
			}

			void SetValue(PointF& value)
			{
				if ((m_value.X == value.X) && (m_value.Y == value.Y))
					return;
				m_value = Round(value);
				if (m_value.X > (m_valueRange.Width+m_valueOffset.X))
					m_value.X = m_valueRange.Width+m_valueOffset.X;
				else if (m_value.X < m_valueOffset.X)
					m_value.X = m_valueOffset.X;
				if (m_value.Y > (m_valueRange.Height+m_valueOffset.Y))
					m_value.Y = m_valueRange.Height+m_valueOffset.Y;
				else if (m_value.Y < m_valueOffset.Y)
					m_value.Y = m_valueOffset.Y;
				if (*this)
					Invalidate();
			}

			AxisMode GetAxisMode()
			{
				if (m_valueRange.Width)
				{
					if (m_valueRange.Height)
					{
						return AxisModeXY;
					}
					else
					{
						return AxisModeX;
					}
				}
				return AxisModeY;
			}

			void SetMeasures(PointF& valueOffset, SizeF& valueRange)
			{
				m_valueOffset = valueOffset;
				m_valueRange = valueRange - SizeF(1.0f,1.0f);
				SetValue(GetValue()); // revalidate
				if (*this)
					Invalidate();
			}

			bool PointInKnob(PointF& point)
			{
				return (GetKnobRect().Contains(point))?true:false;
			}

			bool PointInKnob(CPoint& point)
			{
				return (GetKnobRect().Contains(PointF((REAL)point.x,(REAL)point.y)))?true:false;
			}

			SizeF ValueToClientSize(SizeF& value)
			{                
				return RO3(value,GetKnobScale(),m_valueRange);
			}

			SizeF ClientToValueSize(SizeF& client)
			{
				return RO3(client,m_valueRange,GetKnobScale());
			}

			PointF ValueToClientPoint(PointF& value)
			{
				return PointF(ValueToClientSize(MakeSizeF(value - m_valueOffset))) + GetKnobOffset();
			}

			PointF ClientToValuePoint(PointF& client)
			{
				return PointF(ClientToValueSize(MakeSizeF(client - GetKnobOffset()))) + m_valueOffset;
			}

			void SetOptimalKnobSize()
			{
				SizeF optimalKnobSize = RO3(SizeF(1.0f,1.0f),GetRectSize(m_clientRect),m_valueRange);
				if(m_knobSize.Width <= 0.0f)
					m_knobSize.Width = optimalKnobSize.Width;
				if(m_knobSize.Height <= 0.0f)
					m_knobSize.Height = optimalKnobSize.Height;
			}

			SizeF GetKnobScale()
			{
				return GetRectSize(m_clientRect) - m_knobSize;
			}

			PointF GetKnobOffset()
			{
				PointF knobOffset(0,0);
				if (!m_valueRange.Width)
					knobOffset.X = (m_clientRect.Width/2.0f) - (m_knobSize.Width/2.0f);
				if (!m_valueRange.Height)
					knobOffset.Y = (m_clientRect.Height/2.0f) - (m_knobSize.Height/2.0f);
				return knobOffset;
			}

			void SetKnobSize(SizeF& knobSize)
			{
				m_knobSize = knobSize;
				SetOptimalKnobSize();
			}

			SizeF GetKnobSize()
			{
				return m_knobSize;
			}

			RectF GetKnobRect()
			{
				return Round(RectF(ValueToClientPoint(m_value),m_knobSize));
			}

		};

		// ==========================================================
		//
		//	Slider
		//
		// ==========================================================

		// a slider that can do X, Y and XY sliding
		// inherit class and override GetValueString()
		// to implement your own interpretation of values.
		template<typename T>
		class SliderTemplate
			: public MeasureControlTemplate<T>
		{
		protected:
			bool     m_dragging;
			PointF   m_cursorOffset;
			bool	 m_drawText;
			bool	 m_anyDrag;
			bool	 m_quickPick;
			bool	 m_showKnob;

		public:
			BEGIN_MSG_MAP_EX(Slider)
				MSG_WM_LBUTTONDOWN(OnLButtonDown)
				MSG_WM_LBUTTONUP(OnLButtonUp)
				MSG_WM_MOUSEWHEEL(OnMouseWheel)
				MSG_WM_KEYDOWN(OnKeyDown)
				MSG_WM_MOUSEMOVE(OnMouseMove)
				CHAIN_MSG_MAP(__super)
			END_MSG_MAP()

			void OnKeyDown(TCHAR vkey, UINT repeatCount, UINT flags)
			{                
				if (m_showKnob)
				{
					switch(vkey)
					{
						case VK_RIGHT:
						case VK_UP:
							{
								SetValue(GetValue() + PointF(1.0f,1.0f));
								((T*)this)->OnValueChanged();
							} break;
						case VK_LEFT:
						case VK_DOWN:
							{
								SetValue(GetValue() + PointF(-1.0f,-1.0f));
								((T*)this)->OnValueChanged();
							} break;
						default: break;
					}
				}
			}

			LRESULT OnMouseWheel(UINT vkeys, short delta, CPoint position)
			{
				if (m_showKnob)
				{
					float movement = ((float)delta)/((float)WHEEL_DELTA);
					PointF value = GetValue();
					value.X += movement;
					value.Y += movement;
					SetValue(value);
					((T*)this)->OnValueChanged();
				}
				return 0;
			}

			// override
			void OnValueChanged()
			{
			}

			// override
			void GetValueString(CString& x, CString& y)
			{
				x.Format("%i",(int)round(m_value.X));
				y.Format("%i",(int)round(m_value.Y));
			}

			CString GetString()
			{
				CString x;
				CString y;
				GetValueString(x,y);
				CString out;
				switch(GetAxisMode())
				{
					case AxisModeX: out.Format("%s",(LPCTSTR)x); break;
					case AxisModeY: out.Format("%s",(LPCTSTR)y); break;
					case AxisModeXY: out.Format("%s,%s",(LPCTSTR)x,(LPCTSTR)y); break;
				}
				return out;
			}

			void SetCursorOffset(PointF& offset)
			{
				m_cursorOffset = offset;
			}

			PointF GetCursorOffset()
			{
				return m_cursorOffset;
			}

			void OnLButtonDown(UINT uMode, CPoint& position)
			{       
				if (m_showKnob)
				{
					if (m_anyDrag || PointInKnob(position))
					{
						BeginDrag(MakePointF(&position));
						SetMsgHandled(FALSE);
					}
					else if (m_quickPick)
					{
						SetValue(ClientToValuePoint(MakePointF(&position)));
						((T*)this)->OnValueChanged();
						BeginDrag(MakePointF(&position));
						SetMsgHandled(FALSE);
					}
					else
					{
						SetMsgHandled(TRUE);
					}
				}
			}

			void OnLButtonUp(UINT uMode, CPoint& position)
			{
				if (Dragging())
					EndDrag();
			}

			void OnMouseMove(UINT uMode, CPoint& position)
			{
				if (Dragging())
					DoDrag(MakePointF(&position));
			}

			void BeginDrag(PointF& position)
			{
				m_dragging = TRUE;
				SetCapture();
				PointF knobLocation;
				GetKnobRect().GetLocation(&knobLocation);
				SetCursorOffset(knobLocation - position);
				Invalidate();
			}

			void DoDrag(PointF& position)
			{
				PointF newKnobLocation = position + m_cursorOffset;
				SetValue(ClientToValuePoint(newKnobLocation));
				((T*)this)->OnValueChanged();
				Invalidate();
			}

			void EndDrag()
			{
				ReleaseCapture();
				m_dragging = FALSE;
				Invalidate();
			}

			bool Dragging()
			{
				return m_dragging;
			}


			void PaintBackground(Graphics& graphics)
			{
				// background
				RectF rectClient = GetClientRect();
				DrawGradient(graphics,rectClient,ColorPresetSliderFill1,-90.0f);
	            
				// outline
				Pen pen(GetColor(ColorPresetSliderOutline));
				rectClient.Width-=1.0f;
				rectClient.Height-=1.0f;
				graphics.DrawRectangle(&pen,rectClient);
			}

			void PaintKnob(Graphics& graphics)
			{
				// knob fill
				RectF rectKnob(GetKnobRect());
				DrawGradient(
					graphics,
					rectKnob,Focused()?ColorPresetSliderKnobFillFocused1:ColorPresetSliderKnobFill1,
					Dragging()?-90.0f:90.0f);

				// knob outline
				Pen penKnob(GetColor(ColorPresetSliderKnobOutline));
				rectKnob.Width-=0.5f;
				rectKnob.Height-=0.5f;
				graphics.DrawRectangle(&penKnob,rectKnob);
			}

			void PaintKnobText(Graphics& graphics)
			{
				// measure and find value text position
				SWCSPtr ptr(GetString());
				Font font(L"Tahoma",8.0f);
				RectF boundingBox;
				graphics.MeasureString(ptr,-1,&font,PointF(0,0),&boundingBox);
				PointF textLoc;
				RectF rectKnob(GetKnobRect());
				rectKnob.GetLocation(&textLoc);
				switch(GetAxisMode())
				{
					case AxisModeXY:
					case AxisModeX:
						{
							if (rectKnob.X > (m_clientRect.Width/2.0f))
								textLoc.X = rectKnob.X - boundingBox.Width; // put left
							else
								textLoc.X = rectKnob.GetRight()+1; // put right
						} break;
					case AxisModeY:
						{
							if (rectKnob.Y > (m_clientRect.Height/2.0f))
								textLoc.Y = rectKnob.Y - boundingBox.Height; // put top
							else
								textLoc.Y = rectKnob.GetBottom()+1; // put bottom
						} break;
					default:
						break;
				}

				// draw value text
				StringFormat stringFormat;
				SolidBrush textBrush(GetColor(ColorPresetSliderText));
				graphics.DrawString(ptr,-1,&font,textLoc,&stringFormat,&textBrush);
			}

			// overridden
			void Paint(Graphics& graphics)
			{
				graphics.SetSmoothingMode(SmoothingModeAntiAlias);

				PaintBackground(graphics);

				if (m_showKnob)
				{
					PaintKnob(graphics);
				}

				if (m_drawText)
				{
					PaintKnobText(graphics);
				}
			}

			void SetAnyDrag(bool anyDrag)
			{
				m_anyDrag = anyDrag;
			}

			void SetQuickPick(bool quickPick)
			{
				m_quickPick = quickPick;
			}

			void ShowKnob(bool showKnob)
			{
				m_showKnob = showKnob;
				if (*this)
					Invalidate();
			}

			SliderTemplate()
			{                
				m_dragging = false;
				m_drawText = true;
				m_anyDrag = true;
				m_quickPick = false;
				m_showKnob = true;
			}

			~SliderTemplate()
			{
			}
	        
		}; 


		/*
		// basic implementation
		class Slider
			: public SliderTemplate<Slider>
		{
		public:
		};
		*/

		template <typename T>
		class TargetSlider
			: public SliderTemplate< TargetSlider<T> >
		{
		protected:
			typename typedef void (T::*OnSliderFunc)(TargetSlider<T>& slider);
			T*				m_target;
			OnSliderFunc	m_eventFunction;

		public:
			// override
			void OnValueChanged()
			{
				if (m_target && m_eventFunction)
				{
					(m_target->*m_eventFunction)(*this);
				}
			}

			void SetEventTarget(T* target, OnSliderFunc eventFunction)
			{
				m_target = target;
				m_eventFunction = eventFunction;
			}

			TargetSlider()
			{
				m_target = NULL;
				m_eventFunction = NULL;
			}
		};

		template<typename T>
		class GroupTemplate
			: public ControlTemplate<T>
		{
		protected:
			SizeF m_largestCaption;
			float m_captionHeight;
			float m_itemCaptionHeight;
			float m_borderSize;

			ColorPreset m_captionColor;
			
			Color m_customCaptionColor1;
			Color m_customCaptionColor2;

			ColorPreset m_backgroundColor;

			ColorPreset m_decalBackgroundColor;

			Bitmap* m_decalBitmap;
			PointF	m_decalBitmapPosition;

			struct WindowInfo
			{
				HWND hWnd;
				bool showCaption;
				bool forceBreak;
				CString caption;

				void SetCaption(LPCTSTR caption)
				{
					this->caption = caption;
				}

				WindowInfo(HWND hWnd, bool showCaption, bool forceBreak)
				{
					this->hWnd = hWnd;
					this->showCaption = showCaption;
					this->forceBreak = forceBreak;
					this->caption = _T("");
				}
			};

			typedef std::list<WindowInfo> WindowInfoList;

			WindowInfoList m_windowInfoList;

		public:
			BEGIN_MSG_MAP_EX(GroupTemplate<T>)
				//MSG_WM_SIZE(OnSize)
				//MSG_WM_MOVE(OnMove)
				MSG_WM_DESTROY(OnDestroy)
				CHAIN_MSG_MAP(__super)
			END_MSG_MAP()

			void SetDecalBitmap(UINT id)
			{
				if (m_decalBitmap)
				{
					delete m_decalBitmap;
					m_decalBitmap = 0;
				}
				if (id != 0)
				{
					m_decalBitmap = LoadBitmapARGB(id);
					AddAlphaChannelToBitmap(m_decalBitmap,RGB(255,0,255));
				}
			}

			RectF GetDecalBitmapRect()
			{
				return RectF(m_decalBitmapPosition, GetDecalBitmapSize());
			}

			SizeF GetDecalBitmapSize()
			{
				if (m_decalBitmap)
					return SizeF((REAL)m_decalBitmap->GetWidth()/2, (REAL)m_decalBitmap->GetHeight()/2);
				else
					return SizeF(0,0);
			}

			void OnDestroy()
			{
				m_windowInfoList.clear();
			}

			void OnSize(UINT uType, CSize size)
			{
			}

			void OnMove(CPoint position)
			{
			}

			void SetDecalBitmapPosition(PointF& position)
			{
				m_decalBitmapPosition = position;
			}

			RectF GetContentRect()
			{
				RectF rectContent = GetClientRect();
				rectContent.Y += m_captionHeight;
				rectContent.Height -= m_captionHeight;
				rectContent.Inflate(-m_borderSize,-m_borderSize);
				return rectContent;
			}

			SizeF FindLargestCaption()
			{
				Graphics graphics(*this);
				Font font(L"Tahoma",m_itemCaptionHeight,FontStyleRegular,UnitPixel);

				CString captionText;
				SizeF largestCaption(0,0);
				for(WindowInfoList::iterator windowInfo = m_windowInfoList.begin(); windowInfo != m_windowInfoList.end(); ++windowInfo)
				{
					HWND hWnd = windowInfo->hWnd;
					Window window;
					window.Attach(hWnd);
					{
						// measure and find value text position
						if (windowInfo->caption.GetLength())
							captionText = windowInfo->caption;
						else
							captionText = window.GetWindowText();
						SWCSPtr ptr(captionText);
						RectF boundingBox;
						graphics.MeasureString(ptr,-1,&font,PointF(0,0),&boundingBox);
						if (boundingBox.Width > largestCaption.Width)
							largestCaption.Width = boundingBox.Width;
						if (boundingBox.Height > largestCaption.Height)
							largestCaption.Height = boundingBox.Height;
					}
					window.Detach();
				}
				return largestCaption;
			}

			void Fit(bool addBorder=true)
			{
				RectF rect(RectF(0,0,0,0));
				for(WindowInfoList::iterator windowInfo = m_windowInfoList.begin(); windowInfo != m_windowInfoList.end(); ++windowInfo)
				{
					HWND hWnd = windowInfo->hWnd;
					Window window;
					window.Attach(hWnd);
					RectF winRect(window.GetRect());
					rect.Union(rect,rect,winRect);
					window.Detach();
				}
				if (addBorder)
				{
					rect.Width += m_borderSize;
					rect.Height += m_borderSize;
				}
				Move(RectF(GetPosition(),SizeF(rect.Width,rect.Height)));
			}

			void Layout(bool calculateLargestCaption=true, bool adjustSizeAfterwards=true, bool breakOnWidth=true)
			{
				RectF rectContent = GetContentRect();
				PointF location;
				rectContent.GetLocation(&location);
				if (calculateLargestCaption)
				{
					m_largestCaption = FindLargestCaption();
				}
				SizeF captionSize = m_largestCaption;
				float maxHeight = captionSize.Height; // maximum height per line
				SizeF maxGroupExtends(0,0);
				for(WindowInfoList::iterator windowInfo = m_windowInfoList.begin(); windowInfo != m_windowInfoList.end(); ++windowInfo)
				{
					HWND hWnd = windowInfo->hWnd;
					Window window;
					window.Attach(hWnd);
					SizeF windowSize = window.GetSize();
					float indent = 0; // for caption
					if(windowInfo->showCaption)
					{
						indent = captionSize.Width;
					}
					float estimatedWidth = location.X + windowSize.Width + indent;
					if (breakOnWidth && ((estimatedWidth > rectContent.GetRight())||windowInfo->forceBreak))
					{
						location.X = rectContent.GetLeft();
						location.Y += maxHeight + m_borderSize;
						maxHeight = 0;
					}
					if (windowSize.Height > maxHeight)
						maxHeight = windowSize.Height;
					location.X += indent;
					window.Move(RectF(location,windowSize));
					window.Detach();
					location.X += windowSize.Width + m_borderSize;
					if (location.X > maxGroupExtends.Width)
						maxGroupExtends.Width = location.X;
				}

				if (adjustSizeAfterwards)
				{
					maxGroupExtends.Height = location.Y + maxHeight + m_borderSize;
					// finally fit size of enclosing window
					RectF rectGroup = GetRect();
					rectGroup.Width = maxGroupExtends.Width;
					rectGroup.Height = maxGroupExtends.Height;
					Move(rectGroup);
				}
			}

			void SetLargestCaptionSize(SizeF& largestCaption)
			{
				m_largestCaption = largestCaption;
			}

			void Clear()
			{
				m_windowInfoList.clear();
			}

			void RemoveWindow(HWND hWnd)
			{
				for(WindowInfoList::iterator windowInfo = m_windowInfoList.begin(); windowInfo != m_windowInfoList.end(); ++windowInfo)
				{
					if (windowInfo->hWnd == hWnd)
					{
						m_windowInfoList.erase(windowInfo);
						return;
					}
				}
				ATLASSERT(0);
			}

			void AddWindow(HWND hWnd, bool showCaption=false, bool forceBreak=false)
			{
				::SetParent(hWnd,*this);
				m_windowInfoList.push_back(WindowInfo(hWnd,showCaption,forceBreak));
			}

			void AddWindows(HWNDList& windowList, bool showCaption=false, bool forceBreak=false)
			{
				for(HWNDList::iterator window = windowList.begin(); window != windowList.end(); ++window)
				{
					((T*)this)->AddWindow(*window,showCaption,forceBreak);
				}
			}

			void AddChildWindows(bool showCaption=false, bool forceBreak=false)
			{
				HWNDList windowList;
				GetChildWindows(windowList);
				((T*)this)->AddWindows(windowList,showCaption,forceBreak);
			}

			void SetWindowCaption(HWND hWnd, LPCTSTR captionText)
			{
				WindowInfoList::reverse_iterator windowInfo = FindWindowInfo(hWnd);
				if (windowInfo != m_windowInfoList.rend())
				{
					windowInfo->caption = captionText;
					Invalidate();
				}
			}

			typename WindowInfoList::reverse_iterator FindWindowInfo(HWND hWnd)
			{
				for(WindowInfoList::reverse_iterator windowInfo = m_windowInfoList.rbegin(); windowInfo != m_windowInfoList.rend(); ++windowInfo)
				{
					if (windowInfo->hWnd == hWnd)
						return windowInfo;
				}
				return m_windowInfoList.rend();
			}

			void PaintCaptions(Graphics& graphics)
			{
				SizeF captionSize = m_largestCaption;
				if (!captionSize.Width)
					return;
				// prepare drawing
				Font font(L"Tahoma",7,FontStyleRegular);
				StringFormat stringFormat;
				SolidBrush textBrush(GetColor(ColorPresetGroupText));
				CString captionText;

				for(WindowInfoList::iterator windowInfo = m_windowInfoList.begin(); windowInfo != m_windowInfoList.end(); ++windowInfo)
				{
					if (windowInfo->showCaption)
					{
						HWND hWnd = windowInfo->hWnd;
						Window window;
						window.Attach(hWnd);
						{
							RectF windowRect = window.GetRect();

							if (windowInfo->caption.GetLength())
								captionText = windowInfo->caption;
							else
								captionText = window.GetWindowText();

							SWCSPtr ptr(captionText);
							PointF location(
								windowRect.X - captionSize.Width,
								windowRect.Y + windowRect.Height/2.0f - captionSize.Height/2.0f);
							graphics.DrawString(ptr,-1,&font,location,&stringFormat,&textBrush);
						}
						window.Detach();
					}
				}
			}

			void PaintCaption(Graphics& graphics)
			{
				// caption
				RectF rectCaption = GetClientRect();
				rectCaption.Height = m_captionHeight;
				if (m_captionColor == ColorPresetCustom)
					DrawGradient(graphics,rectCaption,m_customCaptionColor1,m_customCaptionColor2,0.0f);
				else
					DrawGradient(graphics,rectCaption,m_captionColor,0.0f);

				// caption outline
				Pen penOutline(GetColor(ColorPresetGroupOutline));
				graphics.DrawRectangle(&penOutline,RectF(rectCaption.X,rectCaption.Y,rectCaption.Width-1,rectCaption.Height-1));

				// caption text
				rectCaption.Inflate(-3.0f,-3.0f);
				SWCSPtr ptr(GetWindowText());
				Font font(L"Tahoma",rectCaption.Height,FontStyleBold,UnitPixel);
				StringFormat stringFormat;
				SolidBrush textBrush(GetColor(ColorPresetGroupCaptionText));
				PointF location;
				rectCaption.GetLocation(&location);
				graphics.DrawString(ptr,-1,&font,location,&stringFormat,&textBrush);
			}

			void PaintDecal(Graphics& graphics)
			{
				if (m_decalBitmap)    
				{
					Color bgColor = GetColor(m_backgroundColor);
					DrawGradient(graphics,GetDecalBitmapRect(),GetColor(m_decalBackgroundColor),Color(0,bgColor.GetR(),bgColor.GetG(),bgColor.GetB()));
					graphics.SetInterpolationMode(InterpolationModeHighQuality);
					graphics.DrawImage(m_decalBitmap,GetDecalBitmapRect());
					graphics.SetInterpolationMode(InterpolationModeDefault);
				}
			}

			void PaintBackground(Graphics& graphics, bool drawOutline=true)
			{
				// background
				RectF rectClient = GetClientRect();

				DrawGradient(graphics,rectClient,m_backgroundColor,80.0f);

				PaintDecal(graphics);

				if (drawOutline)
				{
					// outline
					Pen penOutline(GetColor(ColorPresetGroupOutline));
					rectClient.Width-=1.0f;
					rectClient.Height-=1.0f;
					graphics.DrawRectangle(&penOutline,rectClient);                
				}
			}

			void SetBackgroundColor(ColorPreset color)
			{
				m_backgroundColor = color;

				if (*this)
					Invalidate();
			}

			void SetDecalBackgroundColor(ColorPreset color)
			{
				m_decalBackgroundColor = color;

				if (*this)
					Invalidate();
			}

			void SetCaptionColor(ColorPreset color)
			{
				m_captionColor = color;
				if (*this)
					Invalidate();
			}

			void SetCaptionColor(Color& firstColor, Color& secondColor)
			{
				m_captionColor = ColorPresetCustom;
				m_customCaptionColor1 = firstColor;
				m_customCaptionColor2 = secondColor;
				if (*this)
					Invalidate();
			}

			GroupTemplate()
			{
				m_itemCaptionHeight = 8.0f;
				m_captionHeight = 0.0f;
				m_borderSize = GetSpacing();
				m_largestCaption = SizeF(0,0);
				m_captionColor = ColorPresetGroupCaptionFill1;
				m_backgroundColor = ColorPresetGroupFill1;
				m_decalBackgroundColor = ColorPresetLogoBackgroundFill1;
				m_decalBitmap = NULL;
				m_decalBitmapPosition = PointF(0,0);
				//NoOffscreenPainting();
			}

			~GroupTemplate()
			{
				if (m_decalBitmap)
				{
					delete m_decalBitmap;
				}
			}
		};

		// ==========================================================
		//
		//	Bitmap
		//
		// ==========================================================

		// ==========================================================
		//
		//	Label
		//
		// ==========================================================

		class Label
			: public ControlTemplate<Label>
		{
		protected:
			CString m_labelText;

		public:
			CString GetLabelText()
			{
				return m_labelText;
			}

			void SetLabelText(LPCTSTR text)
			{
				m_labelText = text;
				Invalidate();
			}

			// overridden
			void Paint(Graphics& graphics)
			{
				// background
				RectF rectClient = GetClientRect();
				DrawGradient(graphics,rectClient,ColorPresetLabelFill1,80.0f);

				// text
				RectF rectCaption(rectClient);
				rectCaption.Inflate(-4.0f,-4.0f);
				SWCSPtr ptr(m_labelText);
				Font font(L"Tahoma",9.0f,FontStyleRegular,UnitPixel);
				StringFormat stringFormat;
				SolidBrush textBrush(GetColor(ColorPresetLabelText));
				PointF location;
				RectF rectString = GetStringRect(m_labelText,font,*this);
				location.X = rectCaption.X + rectCaption.Width/2.0f - rectString.Width/2.0f;
				location.Y = rectCaption.Y + rectCaption.Height/2.0f - rectString.Height/2.0f;
				graphics.DrawString(ptr,-1,&font,location,&stringFormat,&textBrush);

				// outline
				Pen penOutline(GetColor(ColorPresetLabelOutline));
				rectClient.Width-=1.0f;
				rectClient.Height-=1.0f;
				graphics.DrawRectangle(&penOutline,rectClient);                
			}
		};

		// ==========================================================
		//
		//	Group
		//
		// ==========================================================

		// basic implementation
		class Group
			: public GroupTemplate<Group>
		{
		public:
			// overridden
			void Paint(Graphics& graphics)
			{
				//graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);
				//graphics.SetSmoothingMode(SmoothingModeAntiAlias);
				PaintBackground(graphics);

				PaintCaption(graphics);

				// captions
				PaintCaptions(graphics);	            
			}

			Group()
			{
				m_captionHeight = GetGroupCaptionHeight();
			}
		};

		// ==========================================================
		//
		//	Button
		//
		// ==========================================================

		template <typename T>
		class ButtonTemplate
			: public ControlTemplate<T>
		{
		protected:
			bool m_selected;
			bool m_pressed;
			ButtonMode m_buttonMode;
			bool m_allowSelect;

		public:
			BEGIN_MSG_MAP_EX(ButtonTemplate<T>)
				MSG_WM_LBUTTONDOWN(OnLButtonDown)
				MSG_WM_LBUTTONUP(OnLButtonUp)
				MSG_WM_KEYDOWN(OnKeyDown)
				//MSG_WM_MOUSEMOVE(OnMouseMove)
				CHAIN_MSG_MAP(__super)
			END_MSG_MAP()

			void OnKeyDown(TCHAR vkey, UINT repeatCount, UINT flags)
			{
				SetMsgHandled(FALSE);
			}

			void OnLButtonDown(UINT uMode, CPoint& position)
			{
				SetMsgHandled(FALSE);
				SetPressed(true);
				((T*)this)->ValueChanged(true);
				SetCapture();
			}

			void OnLButtonUp(UINT uMode, CPoint& position)
			{
				if (Pressed())
				{
					ReleaseCapture();
					SetPressed(false);
					SetMsgHandled(FALSE);
					if(GetClientRect().Contains(MakePointF(&position)))
						((T*)this)->ValueChanged(false);
				}
			}

			void OnMouseMove(UINT uMode, CPoint& position)
			{
				SetMsgHandled(FALSE);
			}

			bool IsRadioButton()
			{
				return (m_buttonMode == ButtonModeRadio);
			}

			bool Pressed()
			{
				return m_pressed;
			}

			void SetSelected(bool selected)
			{
				m_selected = selected;
				if(*this)
					Invalidate();
			}

			void SetPressed(bool pressed)
			{
				m_pressed = pressed;
				if(*this)
					Invalidate();
			}


			void SetAllowSelect(bool allowSelect)
			{
				m_allowSelect = allowSelect;
			}


			// override
			void ValueChanged(bool isDown)
			{
			}

			ButtonTemplate()
			{
				m_pressed = false; 
				m_selected = false;
				m_allowSelect = true;
				m_buttonMode = ButtonModeDefault;
			}

			void SetButtonMode(ButtonMode buttonMode)
			{
				m_buttonMode = buttonMode;
				if(*this)
					Invalidate();
			}

			bool Selected()
			{
				return m_selected;
			}

			// overridden
			void Paint(Graphics& graphics)
			{
				//graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);
				graphics.SetSmoothingMode(SmoothingModeAntiAlias);
				// background
				RectF rectClient = GetClientRect();
	
				ColorPreset preset;
			  
				if (!m_allowSelect)
				{
					preset = ColorPresetLabelFill1;
				}
				else if (Selected())
				{
					preset = ColorPresetButtonFillSelected1;
				}
				else if (Focused())
				{
					preset = ColorPresetButtonFillFocused1;
				}
				else
				{
					preset = ColorPresetButtonFill1;
				}

				DrawGradient(graphics,rectClient,preset,m_pressed?-90.0f:90.0f);

				// button caption text
				RectF rectCaption(rectClient);
				rectCaption.Inflate(-4.0f,-4.0f);
				SWCSPtr ptr(GetWindowText());
				Font font(L"Tahoma",9.0f,FontStyleRegular,UnitPixel);
				StringFormat stringFormat;
				SolidBrush textBrush(GetColor(ColorPresetButtonText));
				PointF location;
				RectF rectString = GetStringRect(GetWindowText(),font,*this);
				location.X = rectCaption.X + rectCaption.Width/2.0f - rectString.Width/2.0f;
				location.Y = rectCaption.Y + rectCaption.Height/2.0f - rectString.Height/2.0f;
				graphics.DrawString(ptr,-1,&font,location,&stringFormat,&textBrush);

				// outline
				rectClient.Width-=1.0f;
				rectClient.Height-=1.0f;
				Pen penOutline(GetColor(ColorPresetButtonOutline));
				graphics.DrawRectangle(&penOutline,rectClient);                
		        
			}


		};

		/*
		//useless
		class Button
			: public ButtonTemplate<Button>
		{
		protected:
		public:
		};
		*/

		template <typename T>
		class TargetButton
			: public ButtonTemplate< TargetButton<T> >
		{
		protected:
			typename typedef void (T::*OnButtonFunc)(TargetButton<T>& button, bool isDown);
			T*				m_target;
			OnButtonFunc	m_eventFunction;

		public:
			// override
			void ValueChanged(bool isDown)
			{
				if (m_target && m_eventFunction)
				{
					(m_target->*m_eventFunction)(*this,isDown);
				}
			}

			void SetEventTarget(T* target, OnButtonFunc eventFunction)
			{
				m_target = target;
				m_eventFunction = eventFunction;
			}

			TargetButton()
			{
				m_target = NULL;
				m_eventFunction = NULL;
			}
		};

		// ==========================================================
		//
		//	Edit
		//
		// ==========================================================

		template<typename T>
		class EditTemplate
			: public WindowBaseTemplate<T,CEdit>
		{
		protected:
			bool m_isVisibleOnFocusOnly;
			bool m_ctrlPressed;
			bool m_modified;

		public:
			typedef WindowBaseTemplate<T,CEdit> BaseTemplate;

			BEGIN_MSG_MAP_EX(Edit)
				MSG_WM_SETFOCUS(OnSetFocus)
				MSG_WM_KILLFOCUS(OnKillFocus)				
				MSG_WM_KEYDOWN(OnKeyDown)
				MSG_WM_KEYUP(OnKeyUp)
				CHAIN_MSG_MAP(BaseTemplate)
			END_MSG_MAP()

			void OnKeyUp(TCHAR vkey, UINT repeatCount, UINT flags)
			{
				SetMsgHandled(FALSE);
				((T*)this)->OnKeyPressed(vkey);
			}

			// override
			void OnValueChanged(bool apply)
			{
			}

			// override
			void OnKeyPressed(TCHAR vkey)
			{
			}

			CString GetText()
			{
				TCHAR text[65536];
				__super::GetLine(0,text,65535);
				return text;
			}

			void SetText(LPCTSTR text)
			{
				__super::SetWindowText(text);				
			}

			HWND Create(HWND hWndParent=NULL, bool readOnly=false)
			{
				UINT windowStyles = WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|SS_CENTERIMAGE|WS_TABSTOP;
				if (readOnly)
				{
					windowStyles |= ES_READONLY;
				}
				HWND hWnd = __super::Create(hWndParent,rcDefault,NULL,windowStyles,0);
				SetFont((HFONT)GetStockObject(DEFAULT_GUI_FONT));
				SetLimitText(255);
				return hWnd;
			}

			void OnKeyDown(TCHAR vkey, UINT repeatCount, UINT flags)
			{
				SetMsgHandled(FALSE);
				switch(vkey)
				{
					case VK_ESCAPE:
						{
							m_modified = false;
							((T*)this)->OnValueChanged(false);
						} break;
					case VK_RETURN:
						{
							((T*)this)->OnValueChanged(true);
						} break;
					default:
						{
						} break;
				}

			}

			void OnSetFocus(HWND hOldWnd)
			{
				SetMsgHandled(FALSE);
				m_modified = true;
				/*
				if (IsVisibleOnFocusOnly())
				{
					ShowWindow(SW_SHOW);
				}
				*/
			}

			void OnKillFocus(HWND hOldWnd)
			{
				SetMsgHandled(FALSE);
				if (IsVisibleOnFocusOnly())
				{
					ShowWindow(SW_HIDE);
				}
			}

			bool IsVisibleOnFocusOnly()
			{
				return m_isVisibleOnFocusOnly;
			}

			void IsVisibleOnFocusOnly(bool isVisibleOnFocusOnly)
			{
				m_isVisibleOnFocusOnly = isVisibleOnFocusOnly;
			}

			EditTemplate()
			{
				m_isVisibleOnFocusOnly = false;
				m_ctrlPressed = false;
				m_modified = false;
			}
		};

		enum EditEvent
		{
			EditEventTextChanged,
			EditEventKeyPressed,
		};

		template <typename T>
		class TargetEdit
			: public EditTemplate< TargetEdit<T> >
		{
		protected:
			typename typedef void (T::*OnEditFunc)(TargetEdit<T>& edit, EditEvent editEvent, int parameter);
			T*				m_target;
			OnEditFunc		m_eventFunction;

		public:
			// overridden
			void OnValueChanged(bool apply)
			{
				if (m_target && m_eventFunction)
				{
					(m_target->*m_eventFunction)(*this,EditEventTextChanged,apply?1:0);
				}
			}

			// overridden
			void OnKeyPressed(TCHAR vkey)
			{
				if (m_target && m_eventFunction)
				{
					(m_target->*m_eventFunction)(*this,EditEventKeyPressed,(int)vkey);
				}
			}

			void SetEventTarget(T* target, OnEditFunc eventFunction)
			{
				m_target = target;
				m_eventFunction = eventFunction;
			}

			TargetEdit()
			{
				m_target = NULL;
				m_eventFunction = NULL;
			}
		};

		// ==========================================================
		//
		//	VU
		//
		// ==========================================================

		class VU
			: public MeasureControlTemplate<VU>
		{
		protected:
			bool m_clip;
		public:
			// overridden
			void Paint(Graphics& graphics)
			{
				RectF rect(GetClientRect());
				SolidBrush brushBackground(GetColor(ColorPresetVUFill));
				Pen penOutline(GetColor(ColorPresetVUOutline));
				graphics.FillRectangle(&brushBackground,rect);

				PointF knobPoint;
				GetKnobRect().GetLocation(&knobPoint);
				knobPoint.Y = rect.Height - knobPoint.Y;
				RectF vuRect(knobPoint,SizeF(rect.Width-knobPoint.X,rect.Height-knobPoint.Y));
				SolidBrush brushLowVolume(GetColor(m_clip?ColorPresetVUVolumeHigh:ColorPresetVUVolumeLow));
				graphics.FillRectangle(&brushLowVolume,vuRect);

				// outline
				rect.Width-=1.0f;
				rect.Height-=1.0f;
				graphics.DrawRectangle(&penOutline,rect);
			}

			VU()
			{
				m_clip = false;
				SetMeasures(PointF(0,0),SizeF(0,90));
				SetKnobSize(SizeF(16.0f,1.0f));
			}

			void SetValue(float dbValue, bool clip)
			{
				__super::SetValue(PointF(0.0f,90+dbValue));
				m_clip = clip;
			}
		};

		// ==========================================================
		//
		//	ComboBox
		//
		// ==========================================================

		enum ComboBoxEvent
		{
			ComboBoxEventSelectionChanged,
			ComboBoxEventTextChanged,
			ComboBoxEventBeforePopup,
		};

		template<typename T>
		class ComboBoxTemplate
			: public ButtonTemplate<T>
		{
			typedef TargetEdit< ComboBoxTemplate<T> > Edit;
			struct Entry
			{
				int			id;
				CString		text;

				CString GetText(bool addNumber=false)
				{
					if (addNumber)
					{
						CString text;
						text.Format("[%03i|%02X] %s",id,id,(LPCTSTR)this->text);
						return text;
					}
					else
					{
						return this->text;
					}
				}

				Entry(LPCTSTR text, int id)
				{
					this->id = id;
					this->text = text;
				}
			};

			typedef std::vector<Entry> EntryList;

		protected:
			bool				m_isReadOnly;
			bool				m_isMenuBox;
			EntryList			m_entryList;
			int					m_currentEntry;
			bool				m_showIDs;

		public:
			Menu		m_menu;
			Edit		m_edit;

			BEGIN_MSG_MAP_EX(ComboBoxTemplate<T>)
				MSG_WM_CREATE(OnCreate)
				MSG_WM_DESTROY(OnDestroy)
				MSG_WM_SIZE(OnSize)
				MSG_WM_MOVE(OnMove)
				MSG_WM_RBUTTONUP(OnRButtonUp)
				CHAIN_MSG_MAP(__super)
			END_MSG_MAP()

			// override
			void ValueChanged(bool isDown)
			{
				if (!isDown && m_allowSelect)
				{
					((T*)this)->OnComboBoxEvent(ComboBoxEventBeforePopup,0);
					if (!m_isMenuBox)
					{
						SetCurrentEntryId(Popup());
						((T*)this)->OnComboBoxEvent(ComboBoxEventSelectionChanged,m_entryList[m_currentEntry].id);
					}
					else
					{
						((T*)this)->OnComboBoxEvent(ComboBoxEventSelectionChanged,Popup());
					}
				}
			}

			// override
			void OnComboBoxEvent(ComboBoxEvent event, int parameter)
			{

			}

			void OnDestroy()
			{
				SetMsgHandled(FALSE);
				m_menu.DestroyMenu();
				m_edit.DestroyWindow();
			}

			CString GetCurrentText()
			{
				return m_entryList[m_currentEntry].text;
			}

			void OnEdit(Edit& edit, EditEvent editEvent, int parameter)
			{
				if (editEvent == EditEventTextChanged)
				{
					//if (parameter)
					{
						m_entryList[m_currentEntry].text = edit.GetText();
						SetWindowText(m_entryList[m_currentEntry].GetText(m_showIDs));
						m_menu.ModifyMenu(m_entryList[m_currentEntry].id,m_entryList[m_currentEntry].GetText(m_showIDs));
						Invalidate();
						((T*)this)->OnComboBoxEvent(ComboBoxEventTextChanged,m_entryList[m_currentEntry].id);
					}
					SetFocus();
				}
			}

			BOOL OnCreate(LPCREATESTRUCT lpCreateStruct)
			{
				SetMsgHandled(FALSE);

				m_menu.CreatePopupMenu();
				m_edit.Create(*this);
				m_edit.IsVisibleOnFocusOnly(true);
				m_edit.SetEventTarget(this,&V2::GUI::ComboBoxTemplate<T>::OnEdit);
				m_edit.ShowWindow(SW_HIDE);
				//return m_modulationTargetMenu.Popup(*this,position);
				return TRUE;
			}

			void OnSize(UINT uType, CSize size)
			{
				RectF rect(GetClientRect());
				rect.Inflate(-1.0f,-1.0f);
				m_edit.Move(rect);
				SetMsgHandled(FALSE);
			}

			void OnMove(CPoint position)
			{
				SetMsgHandled(FALSE);
			}

			void OnRButtonUp(UINT uMode, CPoint& position)
			{
				SetMsgHandled(FALSE);
				if (!m_isReadOnly && !m_isMenuBox)
				{
					if(GetClientRect().Contains(MakePointF(&position)))
					{
						RectF rect(GetClientRect());
						rect.Inflate(-1.0f,-1.0f);
						m_edit.Move(rect);
						m_edit.ShowWindow(SW_SHOW);
						m_edit.SetWindowText(m_entryList[m_currentEntry].text);
						m_edit.SetSel(0,-1);
						m_edit.SetFocus();
					}
				}
			}

			int IdToIndex(int id)
			{				
				for(int index = 0; index < (int)m_entryList.size(); index++)
				{
					if (m_entryList[index].id == id)
						return index;
				}
				return -1;
			}

			void SetCurrentEntryId(int currentEntryId)
			{
				SetCurrentEntryIndex(IdToIndex(currentEntryId));
			}

			void SetCurrentEntryIndex(int currentEntry)
			{
				if (m_currentEntry == currentEntry)
					return;
				if (currentEntry < 0)
					return;
				if (currentEntry >= (int)m_entryList.size())
					return;
				m_currentEntry = currentEntry;
				if (!m_isMenuBox)
				{				
					SetWindowText(m_entryList[m_currentEntry].GetText(m_showIDs));
					Invalidate();
				}				
			}

			int GetCurrentEntry()
			{
				return m_currentEntry;
			}

			void Clear()
			{
				m_entryList.clear();
				m_menu.DestroyMenu();
				m_menu.CreatePopupMenu();
				m_currentEntry = -1;
				SetWindowText("");
				Invalidate();
			}

			void AppendMenu(LPCTSTR text, int id)
			{
				Entry entry(text,id);
                m_entryList.push_back(entry);
				m_menu.AppendMenu(entry.GetText(m_showIDs),id);
			}

			void AppendSeparator()
			{
				m_menu.AppendSeparator();
			}

			void SetReadOnly(bool isReadOnly)
			{
				m_isReadOnly = isReadOnly;
			}

			void SetMenuBox(bool isMenuBox)
			{
				m_isMenuBox = isMenuBox;
			}

			void ShowIDs(bool showIDs)
			{
				m_showIDs = showIDs;
			}

			int Popup()
			{
				RectF rect(GetAbsoluteRect());				
				return m_menu.Popup(*this,PointF(rect.X,rect.GetBottom()));
			}

			ComboBoxTemplate()
			{
				m_isReadOnly = false;
				m_isMenuBox = false;
				m_currentEntry = -1;
				m_showIDs = false;
				m_allowSelect = true;
			}

			~ComboBoxTemplate()
			{
			}			
		};

		template<typename T>
		class TargetComboBox
			: public ComboBoxTemplate< TargetComboBox<T> >
		{
		protected:
			typename typedef void (T::*OnComboBoxFunc)(TargetComboBox<T>& comboBox, ComboBoxEvent event, int parameter);
			T*				m_target;
			OnComboBoxFunc	m_eventFunction;

		public:
			// overridden
			void OnComboBoxEvent(ComboBoxEvent event, int parameter)
			{
				if (m_target && m_eventFunction)
				{
					(m_target->*m_eventFunction)(*this,event,parameter);
				}
			}

			void SetEventTarget(T* target, OnComboBoxFunc eventFunction)
			{
				m_target = target;
				m_eventFunction = eventFunction;
			}

			TargetComboBox()
			{
				m_target = NULL;
				m_eventFunction = NULL;
			}
		};

		// ==========================================================
		//
		//	ButtonGroup
		//
		// ==========================================================

		// ehem. multibutton
		template<typename T>
		class ButtonGroupTemplate
			: public Group
		{
		protected:
			class RadioButton
				: public ButtonTemplate<RadioButton>
			{
			protected:
				T*				m_buttonGroup;
				int				m_index;

			public:
				// override
				void ValueChanged(bool isDown)
				{
					m_buttonGroup->ValueChanged(this,isDown);
				}

				int GetIndex()
				{
					return m_index;
				}

				RadioButton(T* buttonGroup, int index)
				{
					m_buttonGroup = buttonGroup;
					m_index = index;
					SetButtonMode(ButtonModeRadio);
				}
			};

			typedef std::list<RadioButton*> ButtonList;

			ButtonList m_buttonList;

			void ClearButtonList()
			{
				for (ButtonList::iterator button = m_buttonList.begin(); button != m_buttonList.end(); ++button)
				{
					if (*(*button))
						(*button)->DestroyWindow();
					delete (*button);
				}
				m_buttonList.clear();
			}

			RadioButton* NewButton()
			{
				RadioButton* button = new RadioButton(((T*)this),(int)m_buttonList.size());
				m_buttonList.push_back(button);
				return button;
			}

		public:
			BEGIN_MSG_MAP_EX(ButtonGroupTemplate<T>)
				//MSG_WM_PAINT(OnPaint)
				MSG_WM_DESTROY(OnDestroy)
				CHAIN_MSG_MAP(__super)
			END_MSG_MAP()

			void SetIndex(int index)
			{
				for (ButtonList::iterator button = m_buttonList.begin(); button != m_buttonList.end(); ++button)
				{
					RadioButton* radioButton = (*button);
					if (radioButton->GetIndex() == index)
					{
						radioButton->SetSelected(true);
					}
					else
					{
						radioButton->SetSelected(false);
					}
				}
			}

			// override
			void IndexChanged(int index)
			{
			}

			// override
			void ValueChanged(RadioButton* target, bool isDown)
			{
				if (isDown)
				{
					SetIndex(target->GetIndex());
					((T*)this)->IndexChanged(target->GetIndex());
				}
			}

			void OnDestroy()
			{
				SetMsgHandled(FALSE);
				ClearButtonList();
			}

			void AddButton(LPCTSTR text)
			{
				RadioButton* button = NewButton();
				AddWindow(button->Create(*this));
				button->SetWindowText(text);                
			}

			// overridden
			void Paint(Graphics& graphics)
			{
				//graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);
				//graphics.SetSmoothingMode(SmoothingModeAntiAlias);
				
				// background
				RectF rectClient = GetClientRect();
				DrawGradient(graphics,rectClient,ColorPresetGroupFill1,-90.0f);
				
				/*
				// outline
				rectClient.Width-=1.0f;
				rectClient.Height-=1.0f;
				Pen penOutline(GetColor(ColorPresetButtonOutline));
				graphics.DrawRectangle(&penOutline,rectClient);                
				*/            
			}

			/*
			// wir haben uns dann doch dagegen entschieden

			void OnPaint(HDC hDC)
			{
				// Don't paint
				// I know just what you're saying 
				// So please stop explaining 
				// Don't paint it 'cause it hurts 
				// Don't paint 
				// I know what you're thinking 
				// I don't need your reasons 
				// Don't paint it 'cause it hurts                 
				PAINTSTRUCT ps;
				BeginPaint(&ps);
				EndPaint(&ps);
				SetMsgHandled(TRUE);
			}
			*/

			SizeF GetOptimalButtonSize()
			{
				SizeF sizeText = GetSize();
				Font font(L"Tahoma",9.0f,FontStyleRegular,UnitPixel);
				SizeF sizeButtonText(0,0);
				for(WindowInfoList::iterator windowInfo = m_windowInfoList.begin(); windowInfo != m_windowInfoList.end(); ++windowInfo)
				{
					Window window;
					window.Attach(windowInfo->hWnd);
					RectF rectString = GetStringRect(window.GetWindowText(),font,*this);
					if (rectString.Width > sizeButtonText.Width)
						sizeButtonText.Width = rectString.Width;
					if (rectString.Height > sizeButtonText.Height)
						sizeButtonText.Height = rectString.Height;
					window.Detach();
				}
				return sizeButtonText + SizeF(GetSpacing(),0.0f); // make some space left/right
			}

			void Layout()
			{
				if (m_windowInfoList.size() <= 1)
				{
					ShowWindow(SW_HIDE);
					return; // go away
				}
				float controlCount = (float)m_windowInfoList.size();
				RectF rectClient = GetRect();
				//rectClient.Inflate(-2.0f*m_borderSize,-2.0f*m_borderSize);
				SizeF sizeOptimal = GetOptimalButtonSize();
				sizeOptimal.Width *= controlCount;
				if (sizeOptimal.Width > rectClient.Width)
					rectClient.Width = sizeOptimal.Width;
				if (sizeOptimal.Height > rectClient.Height)
					rectClient.Height = sizeOptimal.Height;
				float buttonWidth = round(rectClient.Width / controlCount);
				rectClient.Width = buttonWidth * controlCount;
				//rectClient.Inflate(2.0f*m_borderSize,2.0f*m_borderSize);
				Move(rectClient); // fit
				// resize controls to fit size
				for(WindowInfoList::iterator windowInfo = m_windowInfoList.begin(); windowInfo != m_windowInfoList.end(); ++windowInfo)
				{
					Window window;
					window.Attach(windowInfo->hWnd);
					window.Move(RectF(PointF(0,0),SizeF(buttonWidth,rectClient.Height)));
					window.Detach();
				}
				__super::Layout(false,false,false); // ultracheap
			}

			ButtonGroupTemplate()
			{
				m_captionHeight = 0.0f; // no caption
				m_borderSize = 0.0f; // no border
			}

			~ButtonGroupTemplate()
			{                
			}
		};

		class ButtonGroup
			: public ButtonGroupTemplate<ButtonGroup>
		{
		public:
		};

		// ==========================================================
		//
		//	ScrollBox
		//
		// ==========================================================

		class ScrollBox
			: public SliderTemplate<ScrollBox>
		{
		protected:
			HWND m_hWndTarget;

		public:
			void SetTarget(HWND hWnd)
			{
				m_hWndTarget = hWnd;
			}

			// override
			void OnValueChanged()
			{
				PointF value = GetValue();
				if (m_hWndTarget)
				{
					Window window;
					window.Attach(m_hWndTarget);
					RectF rect = window.GetRect();
					rect.Y = -value.Y;
					window.Move(rect);
					window.Detach();
				}				
			}

			ScrollBox()
			{
				m_hWndTarget = NULL;
				m_drawText = false;
				m_quickPick = true;
				m_anyDrag = false;
			}
		};
		// ==========================================================
		//
		//	Tab
		//
		// ==========================================================

		class Tab
			: public GroupTemplate<Tab>
		{
		protected:
			class TabContainer
				: public GroupTemplate<TabContainer>
			{
			public:
				// overridden
				void Paint(Graphics& graphics)
				{
					//graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);
					//graphics.SetSmoothingMode(SmoothingModeAntiAlias);
					PaintBackground(graphics,false);

					//PaintCaption(graphics);

					// captions
					//PaintCaptions(graphics);	            
				}

				TabContainer()
				{
					//m_captionHeight = GetGroupCaptionHeight();
				}
			};

			TabContainer	m_container;
			ScrollBox		m_scrollBox;

		public:
			BEGIN_MSG_MAP_EX(TabGroup)
				MSG_WM_CREATE(OnCreate)
				MSG_WM_DESTROY(OnDestroy)
				CHAIN_MSG_MAP(__super)
			END_MSG_MAP()

			void OnDestroy()
			{
				m_container.DestroyWindow();
				m_scrollBox.DestroyWindow();
				SetMsgHandled(FALSE);
			}

			BOOL OnCreate(LPCREATESTRUCT lpCreateStruct)
			{
				m_container.Create(*this);
				m_scrollBox.Create(*this);
				m_scrollBox.SetKnobSize(GetScrollBoxKnobSize());
				m_scrollBox.SetTarget(m_container);
				SetMsgHandled(FALSE);
				return TRUE;
			}

			// overriden
			void AddWindow(HWND hWnd, bool showCaption=false)
			{
				m_container.AddWindow(hWnd,showCaption);
			}

			// overriden
			void RemoveWindow(HWND hWnd)
			{
				m_container.RemoveWindow(hWnd);
			}

			void Layout(bool calculateLargestCaption=true,bool adjustSizeAfterwards = true, bool breakOnWidth = true)
			{
				m_container.Move(RectF(PointF(0,0),GetSize()));
				m_container.Layout(calculateLargestCaption,adjustSizeAfterwards,breakOnWidth);
				RectF clientRect = m_container.GetRect();
				//m_container.Move(clientRect);
				float fullHeight = clientRect.Height;
				if (clientRect.Height > GetMaximumTabHeight())
				{
					clientRect.Height = GetMaximumTabHeight();
				}
				else if (clientRect.Height < GetMaximumTabHeight())
				{
					clientRect.Height = GetMaximumTabHeight();
				}
				RectF scrollBoxRect(clientRect);
				scrollBoxRect.X = clientRect.GetRight()+1.0f;
				scrollBoxRect.Width = GetScrollBoxWidth();
				m_scrollBox.Move(scrollBoxRect);
				m_scrollBox.SetMeasures(PointF(0.0f,0.0f),SizeF(1.0f,fullHeight-clientRect.Height));
				m_scrollBox.SetValue(PointF(0.0f,0.0f));
				if (fullHeight <= clientRect.Height)
				{
					m_scrollBox.ShowKnob(false);
				}
				else
				{
					m_scrollBox.ShowKnob(true);
					m_scrollBox.SetKnobSize(SizeF(GetScrollBoxKnobSize().Width,ro3(clientRect.Height,scrollBoxRect.Height,fullHeight)));
				}
				SetClientSize(SizeF(scrollBoxRect.GetRight(),clientRect.GetBottom()));
			}

			void SetLargestCaptionSize(SizeF& largestCaption)
			{
				__super::SetLargestCaptionSize(largestCaption);
				m_container.SetLargestCaptionSize(largestCaption);
			}

			// overridden
			void Paint(Graphics& graphics)
			{
				//graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);
				//graphics.SetSmoothingMode(SmoothingModeAntiAlias);
				PaintBackground(graphics,false);
			}

			Tab()
			{
			}
		};

		// ==========================================================
		//
		//	TabGroup
		//
		// ==========================================================

		class TabGroup
			: public GroupTemplate<TabGroup>
		{
		protected:
			class TabButtonGroup
				: public ButtonGroupTemplate<TabButtonGroup>
			{
			protected:
				TabGroup* m_tabGroup;

			public:
				// override
				void IndexChanged(int index)
				{
					m_tabGroup->ActivateTab(index);
				}
				
                TabButtonGroup(TabGroup* tabGroup)
				{
					m_tabGroup = tabGroup;
				}
			};

			TabButtonGroup	m_buttonGroup;
			Window			m_container;

		public:
			BEGIN_MSG_MAP_EX(TabGroup)
				MSG_WM_CREATE(OnCreate)
				MSG_WM_DESTROY(OnDestroy)
				CHAIN_MSG_MAP(__super)
			END_MSG_MAP()

			void ActivateTab(int index)
			{
				for(WindowInfoList::iterator windowInfo = m_windowInfoList.begin(); windowInfo != m_windowInfoList.end(); ++windowInfo)
				{
					Window window;
					window.Attach(windowInfo->hWnd);
					if (!index)
					{
						window.ShowWindow(SW_SHOW);
					}
					else
					{
						window.ShowWindow(SW_HIDE);
					}
					window.Detach();
					--index;
				}
			}

			// overriden
			void AddWindow(HWND hWnd, bool showCaption=false)
			{
				__super::AddWindow(hWnd,showCaption);
				::SetParent(hWnd,m_container);
			}

			// overridden
			void Paint(Graphics& graphics)
			{
				//graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);
				//graphics.SetSmoothingMode(SmoothingModeAntiAlias);
				PaintBackground(graphics);
				PaintCaption(graphics);
			}

			void Layout()
			{			
				// ensure that all controls are in the upper left
				for(WindowInfoList::iterator windowInfo = m_windowInfoList.begin(); windowInfo != m_windowInfoList.end(); ++windowInfo)
				{
					Window window;
					window.Attach(windowInfo->hWnd);
					SizeF windowSize = window.GetSize();
					m_buttonGroup.AddButton(window.GetWindowText());
					window.Move(RectF(PointF(0.0f,0.0f),windowSize));
					window.Detach();					
					
				}
				RectF winRect = m_container.GetChildWindowsRect();
				RectF clientRect = winRect;	
				clientRect.X += 1.0f;
				clientRect.Y += m_captionHeight;
				clientRect.Width = winRect.Width;
				clientRect.Height = winRect.Height;
				/*
				if (clientRect.Height > GetMaximumTabHeight())
					clientRect.Height = GetMaximumTabHeight();
				*/
				//clientRect.Height += m_captionHeight;
				m_container.Move(clientRect);
				RectF rect = GetRect();
				rect.Width = clientRect.GetRight() + 1.0f;
				rect.Height = clientRect.GetBottom() + 1.0f;
				Move(rect);
				m_buttonGroup.Move(RectF(1,1,150.0f,m_captionHeight-2));
				m_buttonGroup.Layout();
				ActivateTab(0);
				m_buttonGroup.SetIndex(0);
			}

			void OnDestroy()
			{
				m_buttonGroup.DestroyWindow();
				m_container.DestroyWindow();
				SetMsgHandled(FALSE);
			}

			BOOL OnCreate(LPCREATESTRUCT lpCreateStruct)
			{
				m_container.Create(*this);
				m_buttonGroup.Create(*this);
				SetMsgHandled(FALSE);
				return TRUE;
			}

			TabGroup()
				: m_buttonGroup(this)
			{
				m_borderSize = 0.0f;
				m_captionHeight = GetGroupCaptionHeight();
			}
		};

		// ==========================================================
		//
		//	ParameterController
		//
		// ==========================================================
		
		class ParameterController
		{
		protected:
			int m_parameterIndex;

		public:
			virtual ~ParameterController()
			{
			}

			virtual void Update()
			{
			}

			void SetParameterIndex(int parameterIndex)
			{
				m_parameterIndex = parameterIndex;
			}

			int GetParameterIndex()
			{
				return m_parameterIndex;
			}

			virtual void SetParameter(int value) = 0;
			virtual int GetParameter() = 0;

			virtual void SetSynth(Synth* synth)
			{
			}
		};

		template<typename ParameterInfoType=ParameterInfo>
		class ParameterControllerWithInfo : public ParameterController
		{
		protected:
			ParameterInfoType*          m_info;

		public:
			virtual void SetFromParameterInfo()
			{
			}

			void SetParameterInfo(ParameterInfoType* info)
			{
				m_info = info;
				SetFromParameterInfo();
			}
		};

		class SynthParameterController : public ParameterControllerWithInfo<>
		{
		protected:
			Synth*					m_synth;

		public:
			virtual void SetParameter(int value)
			{
				m_synth->SetParameter(m_parameterIndex,value);
			}

			virtual int GetParameter()
			{
				return m_synth->GetParameter(m_parameterIndex);
			}

			void SetSynth(Synth* synth)
			{
				m_synth = synth;
			}

			Synth* GetSynth()
			{
				return m_synth;
			}

		};

		class SynthGlobalParameterController
			: public SynthParameterController
		{
		public:
			virtual void SetParameter(int value)
			{
				m_synth->SetGlobalParameter(m_parameterIndex,value);
			}

			virtual int GetParameter()
			{
				return m_synth->GetGlobalParameter(m_parameterIndex);
			}

		};

		class AppearanceParameterController
			: public ParameterControllerWithInfo<Appearance::ParameterInfo>
		{
		protected:
		public:
			virtual void SetParameter(int value)
			{
				GetTheAppearance()->GetParameter(m_parameterIndex).SetValue(value);				
				InvalidateAllWindows();
			}

			virtual int GetParameter()
			{
				return GetTheAppearance()->GetParameter(m_parameterIndex).GetValue();
			}

		};

		// ==========================================================
		//
		//	View
		//
		// ==========================================================

		class View
			: public GroupTemplate<View>
		{
		protected:
			typedef TargetButton<View> Button;
			typedef TargetComboBox<View> ComboBox;
			typedef TargetEdit<View> Edit;

			template<typename ParameterControllerType>
			class LinkedButtonGroup
				: public ButtonGroupTemplate<LinkedButtonGroup<ParameterControllerType>>,
					public ParameterControllerType
			{
			public:
				// override
				void IndexChanged(int index)
				{
					SetParameter(index);
				}

				virtual void Update()
				{
					SetIndex(GetParameter());
				}

				void AddButtons()
				{
					CString strings[16];
					int optionCount = m_info->GetOptions(strings,16);
					for (int index = 0; index < optionCount; index++)
					{
						AddButton(strings[index]);
					}
				}

				virtual void SetFromParameterInfo()
				{
					SetWindowText(m_info->GetName());
					AddButtons();
					Layout();
				}

				~LinkedButtonGroup()
				{
					if (*this)
						DestroyWindow();
				}
			};

			template<typename ParameterControllerType>
			class LinkedSlider
				: public SliderTemplate<LinkedSlider<ParameterControllerType>>,
				public ParameterControllerType
			{
			public:
				virtual void Update()
				{
					PointF value((REAL)GetParameter(),0.0f);
					SetValue(value);
				}

				// override
				void OnValueChanged()
				{
					PointF value = GetValue();
					SetParameter((int)round(value.X));
				}

				virtual void SetFromParameterInfo()
				{
					SetWindowText(m_info->GetName());
					__super::SetMeasures(
						MakePointFromParameterInfoF(m_info),
						MakeSizeFromParameterInfoF(m_info));
				}

				// override
				void GetValueString(CString& x, CString& y)
				{

				}

				~LinkedSlider()
				{
					if (*this)
						DestroyWindow();
				}
			};

			struct WindowInfo
			{
				HWND    hWnd;

				WindowInfo(HWND hWnd)
				{
					this->hWnd = hWnd;
				}
			};

			Menu   m_modulationSourceMenu;
			Menu   m_modulationTargetMenu;

			int PopupModulationSourceMenu(PointF& position)
			{
				return m_modulationSourceMenu.Popup(*this,position);
			}

			int PopupModulationTargetMenu(PointF& position)
			{
				return m_modulationTargetMenu.Popup(*this,position);
			}

			void PrepareModulationSourceMenu()
			{
				m_modulationSourceMenu.CreatePopupMenu();
				for(int index = 0; index < m_synth->GetModulationSourceCount(); index++)
				{
					if (index == 1 || index == 8)
						m_modulationSourceMenu.AppendSeparator();
					m_modulationSourceMenu.AppendMenu(m_synth->GetModulationSourceName(index),index);
				}
			}

			void PrepareModulationTargetMenu()
			{
				m_modulationTargetMenu.CreatePopupMenu();
				int offset = 0;
				for(int topicIndex = 0; topicIndex < m_synth->GetTopicCount(); topicIndex++)
				{
					if (topicIndex)
						m_modulationTargetMenu.AppendSeparator();
					for (int index = 0; index < m_synth->GetTopicInfo(topicIndex)->GetParameterCount(); index++)
					{				
						int parameterIndex = offset + index;
						if (m_synth->GetParameterInfo(parameterIndex)->IsModulatorDestination())
						{
							m_modulationTargetMenu.AppendMenu(m_synth->GetModulationTargetName(parameterIndex),parameterIndex);
						}
					}
					offset += m_synth->GetTopicInfo(topicIndex)->GetParameterCount();
				}
			}

			void PrepareModulationMenus()
			{
				PrepareModulationSourceMenu();
				PrepareModulationTargetMenu();
			}

			class MidiStatsGroup
				: public Group
			{
			public:
				Synth* m_synth;	

				enum
				{
#ifdef SINGLECHN
					MIDIChannelCount = 1,
#else
					MIDIChannelCount = 16,
#endif
				};

				Window	m_placeHolder[MIDIChannelCount];
				Window  m_channelCaption;
				Window  m_voiceCaption;
				Window  m_pgmCaption;
				Label	m_pgmLabel[MIDIChannelCount];
				Label	m_voiceLabel[MIDIChannelCount+1];
				Label	m_cpuLoadLabel;

				int m_polyphonyStats[MIDIChannelCount+1];
				int m_pgmStats[MIDIChannelCount];
				int m_cpuLoad;

				BEGIN_MSG_MAP_EX(MidiStatsGroup)
					MSG_WM_CREATE(OnCreate)
					MSG_WM_DESTROY(OnDestroy)
					CHAIN_MSG_MAP(__super)
				END_MSG_MAP()

				void SetSynth(Synth* synth)
				{
					m_synth = synth;
				}

				BOOL OnCreate(LPCREATESTRUCT lpCreateStruct)				
				{
					memset(m_polyphonyStats,0,sizeof(int)*(MIDIChannelCount+1));
					memset(m_pgmStats,0,sizeof(int)*MIDIChannelCount);
					m_cpuLoad = 0;

					PointF position = GetSpacingPoint();

					AddWindow(m_channelCaption.Create(*this),true);
					m_channelCaption.Move(RectF(position,SizeF(16,16)));
					m_channelCaption.SetWindowText(_T("Channel #"));
					m_channelCaption.ShowWindow(SW_HIDE);

					position.X += 50;

					for (int index = 0; index < MIDIChannelCount; index++)
					{
						CString text;
						text.Format(_T("%02i"),index+1);
						AddWindow(m_placeHolder[index].Create(*this),true);
						m_placeHolder[index].Move(RectF(position,GetPGMLabelSize()));
						m_placeHolder[index].SetWindowText(text);
						m_placeHolder[index].ShowWindow(SW_HIDE);
						position.X += m_placeHolder[index].GetSize().Width+GetSpacing();
					}

					position.X = GetSpacing();
					position.Y += 16.0f;

					AddWindow(m_pgmCaption.Create(*this),true);
					m_pgmCaption.Move(RectF(position,SizeF(16,16)));
					m_pgmCaption.SetWindowText(_T("Patch #"));
					m_pgmCaption.ShowWindow(SW_HIDE);

					position.X += 50;

					for (int index = 0; index < MIDIChannelCount; index++)
					{
						AddWindow(m_pgmLabel[index].Create(*this),(index?false:true));
						m_pgmLabel[index].Move(RectF(position,GetPGMLabelSize()));
						m_pgmLabel[index].SetLabelText(_T("000"));
						position.X += m_pgmLabel[index].GetSize().Width+GetSpacing();
					}

					AddWindow(m_cpuLoadLabel.Create(*this));
					m_cpuLoadLabel.Move(RectF(position,GetPGMLabelSize()));
					m_cpuLoadLabel.SetLabelText(_T("0%"));

					position.X = GetSpacing();
					position.Y += GetVoiceLabelSize().Height + GetSpacing();

					AddWindow(m_voiceCaption.Create(*this),true);
					m_voiceCaption.Move(RectF(position,SizeF(16,16)));
					m_voiceCaption.SetWindowText(_T("Polyphony"));
					m_voiceCaption.ShowWindow(SW_HIDE);

					position.X += 50;

					for (int index = 0; index < (MIDIChannelCount+1); index++)
					{
						AddWindow(m_voiceLabel[index].Create(*this),(index?false:true));
						m_voiceLabel[index].Move(RectF(position,GetVoiceLabelSize()));
						m_voiceLabel[index].SetLabelText(_T("0"));
						position.X += m_voiceLabel[index].GetSize().Width+GetSpacing();
					}

					SetLargestCaptionSize(SizeF(1.0f,16.0f));

					Fit();
					SetMsgHandled(FALSE);
					return TRUE;
				}

				void OnDestroy()
				{
					for (int index = 0; index < MIDIChannelCount; index++)
					{
						m_voiceLabel[index].DestroyWindow();
						m_pgmLabel[index].DestroyWindow();
						m_placeHolder[index].DestroyWindow();
					}
					m_voiceLabel[MIDIChannelCount].DestroyWindow();
					m_channelCaption.DestroyWindow();
					m_voiceCaption.DestroyWindow();
					m_pgmCaption.DestroyWindow();
					m_cpuLoadLabel.DestroyWindow();
					SetMsgHandled(FALSE);
				}

				void Update()
				{
					int polyphonyStats[17];
					int pgmStats[16];
					int cpuLoad;

					m_synth->GetPolyphonyStats(polyphonyStats,17);
					m_synth->GetPGMStats(pgmStats,16);
					cpuLoad = m_synth->GetCPULoad();

					CString text;

					for (int index = 0; index < (MIDIChannelCount+1); index++)
					{
						if (m_polyphonyStats[index] != polyphonyStats[index])
						{
							m_polyphonyStats[index] = polyphonyStats[index];
							text.Format(_T("%d"),m_polyphonyStats[index]);
							m_voiceLabel[index].SetLabelText(text);
						}
					}

					for (int index = 0; index < MIDIChannelCount; index++)
					{
						if (m_pgmStats[index] != pgmStats[index])
						{
							m_pgmStats[index] = pgmStats[index];
							text.Format(_T("%03d"),m_pgmStats[index]);
							m_pgmLabel[index].SetLabelText(text);
						}
					}

					if (m_cpuLoad != cpuLoad)
					{
						m_cpuLoad = cpuLoad;
						text.Format("%3d%%",m_cpuLoad);
						m_cpuLoadLabel.SetLabelText(text);
					}
				}

				MidiStatsGroup()
				{
					m_captionHeight = 0.0f; // no caption
				}				
			};

			class ModulationGroup
				: public Group
			{
			public:
				typedef TargetButton<ModulationGroup> Button;
				typedef TargetSlider<ModulationGroup> Slider;

				Button m_sourceButton;
				Button m_targetButton;
				Slider m_factorSlider;
				Button m_deleteButton;
				Synth* m_synth;								
				int	   m_index;
				View*  m_view;

				BEGIN_MSG_MAP_EX(ModulationGroup)
					MSG_WM_CREATE(OnCreate)
					MSG_WM_DESTROY(OnDestroy)
					CHAIN_MSG_MAP(__super)
				END_MSG_MAP()

				void SetView(View* view)
				{
					m_view = view;
				}

				void SetTarget(Synth* synth, int modulationIndex)
				{
					m_synth = synth;
					m_index = modulationIndex;
				}

				void OnDestroy()
				{
					m_sourceButton.DestroyWindow();
					m_targetButton.DestroyWindow();
					m_factorSlider.DestroyWindow();
					m_deleteButton.DestroyWindow();
					SetMsgHandled(FALSE);
				}

				void Update()
				{
					Modulation* modulation = m_synth->GetModulation(m_index);
					m_sourceButton.SetWindowText(m_synth->GetModulationSourceName(modulation->GetSourceIndex()));
					m_sourceButton.Invalidate();
					m_targetButton.SetWindowText(m_synth->GetModulationTargetName(modulation->GetTargetIndex()));
					m_targetButton.Invalidate();
					m_factorSlider.SetValue(PointF((float)modulation->GetFactor(),0));
				}

				void OnButton(Button& button, bool isDown)
				{
					ATLASSERT(m_view);
					if (!isDown)
					{
						
						PointF position(button.GetAbsolutePosition());
						position.Y += button.GetSize().Height;
						if (&button == &m_sourceButton)
						{
							int sourceIndex = m_view->PopupModulationSourceMenu(position);
							if (sourceIndex != -1)
							{
								m_synth->GetModulation(m_index)->SetSourceIndex(sourceIndex);
								Update();
							}
						}
						else if (&button == &m_targetButton)
						{
							int targetIndex = m_view->PopupModulationTargetMenu(position);
							if (targetIndex != -1)
							{
								m_synth->GetModulation(m_index)->SetTargetIndex(targetIndex);
								Update();
							}
						}
						else if (&button == &m_deleteButton)
						{
							m_synth->DeleteModulation(m_index);
							m_view->PostModulationsChanged();
						}
					}
				}

				void OnSlider(Slider& slider)
				{
					if (&slider == &m_factorSlider)
					{
						m_synth->GetModulation(m_index)->SetFactor((int)slider.GetValue().X);
					}
				}

				BOOL OnCreate(LPCREATESTRUCT lpCreateStruct)
				{
					m_sourceButton.Create(*this);
					m_sourceButton.SetWindowText(_T("Source"));
					m_sourceButton.Move(RectF(PointF(0,0),GetModulationSourceButtonSize()));
					m_sourceButton.SetEventTarget(this,&V2::GUI::View::ModulationGroup::OnButton);
					
					m_targetButton.Create(*this);
					m_targetButton.SetWindowText(_T("Target"));
					m_targetButton.Move(RectF(PointF(0,0),GetModulationTargetButtonSize()));
					m_targetButton.SetEventTarget(this,&V2::GUI::View::ModulationGroup::OnButton);

					m_factorSlider.Create(*this);
					m_factorSlider.Move(RectF(PointF(0,0),GetModulationFactorSliderSize()));
					m_factorSlider.SetKnobSize(GetModulationFactorSliderKnobSize());
					m_factorSlider.SetMeasures(PointF(-64.0,0),SizeF(128,1));
					m_factorSlider.SetEventTarget(this,&V2::GUI::View::ModulationGroup::OnSlider);
					
					m_deleteButton.Create(*this);
					m_deleteButton.SetWindowText(_T("Remove"));
					m_deleteButton.Move(RectF(PointF(0,0),GetModulationDeleteButtonSize()));
					m_deleteButton.SetEventTarget(this,&V2::GUI::View::ModulationGroup::OnButton);

					AddChildWindows(false);

					Window window;
					window.Attach(GetParent());
					Move(RectF(PointF(0,0),window.GetSize() - SizeF(0,2*GetSpacing())));
					window.Detach();
					Layout(false);
					SetMsgHandled(FALSE);
					return TRUE;
				}

				ModulationGroup()
				{
					m_synth = NULL;
					m_index = -1;
					m_captionHeight = 0.0f; // no caption
				}				
			};

			enum
			{
				VUCount = 2,
				SpeechEditCount = 64,
				StatusTimerID = 303,
			};

			typedef std::list<Group*> GroupList;
			typedef std::list<ParameterController*> ParameterControllerList;

			Synth* m_synth;

			ParameterControllerList m_parameterControllerList;
			GroupList m_groupList;

			MidiStatsGroup	m_midiStatsGroup;
			
			Window		m_decalSpacer;

			ComboBox	m_patchComboBox;
			ComboBox	m_channelComboBox;
			ComboBox	m_fileComboBox;
			ComboBox	m_editComboBox;
			Button		m_recordButton;

			VU			m_vu[VUCount];

			Group		m_speechDesignGroup;
			Group		m_speechTextGroup;
			Edit		m_speechEnglishEdit;
			Edit		m_speechPhoneticEdit;
			Edit		m_speechEdit[SpeechEditCount];

			bool		m_vstMode;

			// current file name
			CString m_fileName;

			enum TabIndex
			{
				TabIndexPatch = 0,
				TabIndexGlobal,
				TabIndexSpeech,
				TabIndexAppearance,
				// add new groups here

				// total
				TabIndexCount,
			};

			enum MenuCommand
			{
				MenuCommandFileLoadBankOrPatch		= 0x100,
				MenuCommandFileImportV2M			= 0x101,
				MenuCommandFileSaveBank				= 0x102,
				MenuCommandFileSavePatch			= 0x103,
				MenuCommandFileSaveAppearance		= 0x104,
				MenuCommandFileLoadAppearance		= 0x105,
				MenuCommandFileResetAppearance		= 0x106,
				
				MenuCommandEditCopyPatch			= 0x200,
				MenuCommandEditPastePatch			= 0x201,
				MenuCommandEditResetPatch			= 0x202,
			};


			TabGroup m_tabGroup;
			Tab m_tabs[TabIndexCount];

			TabGroup m_modulationTabGroup;
			Button m_addModulationButton;
			Tab m_modulationTab;

			Group m_statusGroup;

			Tab& CreateTab(TabIndex tabIndex, LPCTSTR text)
			{
				Tab& tab = m_tabs[tabIndex];
				m_tabGroup.AddWindow(tab.Create(m_tabGroup),false);
				tab.SetWindowText(text);
				// give some space
				tab.Move(RectF(GetSpacingPoint(),GetTabSize()));
				return tab;
			}

			void FinalizeTab(Tab& tab)
			{
				tab.Layout(false,true);
			}

			typedef void (View::*ParameterControllerIteratorFunc)(ParameterController* controller);

			void WithParameterControllersDo(ParameterControllerIteratorFunc parameterControllerIteratorFunc)
			{
				for (ParameterControllerList::iterator controller = m_parameterControllerList.begin(); controller != m_parameterControllerList.end(); ++controller)
				{
					(this->*parameterControllerIteratorFunc)(*controller);
				}
			}
	        
			void UpdateParameterController(ParameterController* controller)
			{
				controller->Update();
			}

			void UpdateParameterControllers()
			{
				WithParameterControllersDo(&V2::GUI::View::UpdateParameterController);
			}

			void ClearParameterControllers()
			{
				for (ParameterControllerList::iterator controller = m_parameterControllerList.begin(); controller != m_parameterControllerList.end(); ++controller)
				{
					delete (*controller);
				}
				m_parameterControllerList.clear();
			}

			/*
			void ClearSliders()
			{
				for (LinkedSliderList::iterator slider = m_linkedSliderList.begin(); slider != m_linkedSliderList.end(); ++slider)
				{
					if (*(*slider))
						(*slider)->DestroyWindow();
					delete (*slider);
				}
				m_linkedSliderList.clear();
			}
			*/

			void ClearGroups()
			{
				for (GroupList::iterator group = m_groupList.begin(); group != m_groupList.end(); ++group)
				{
					if (*(*group))
						(*group)->DestroyWindow();
					delete (*group);
				}
				m_groupList.clear();
			}

			/*
			void ClearButtonGroups()
			{
				for (LinkedButtonGroupList::iterator buttonGroup = m_linkedButtonGroupList.begin(); buttonGroup != m_linkedButtonGroupList.end(); ++buttonGroup)
				{
					if (*(*buttonGroup))
						(*buttonGroup)->DestroyWindow();
					delete (*buttonGroup);
				}
				m_linkedButtonGroupList.clear();
			}
			*/

			void AddParameterController(ParameterController* controller)
			{
				m_parameterControllerList.push_back(controller);
			}

			Group* NewGroup()
			{
				Group* group = new Group();
				m_groupList.push_back(group);
				return group;
			}

			Group* NewGroup(Color& color1, Color& color2)
			{
				Group* group = NewGroup();
				group->SetCaptionColor(color1,color2);
				return group;
			}

			Group* NewGroup(float captionHue)
			{
				return NewGroup(
					MakeColorHSB(
						captionHue,
						GetCaptionSaturation(),
						GetCaptionBrightnessLeft()
						),
					MakeColorHSB(
						captionHue,
						GetCaptionSaturation(),
						GetCaptionBrightnessRight()
						)
					);
			}

			template<typename ParameterControllerType>
			LinkedButtonGroup<ParameterControllerType>* NewButtonGroup()
			{
				LinkedButtonGroup<ParameterControllerType>* buttonGroup = new LinkedButtonGroup<ParameterControllerType>();
				buttonGroup->SetSynth(m_synth);
				AddParameterController(buttonGroup);
				return buttonGroup;
			}

			template<typename ParameterControllerType>
			LinkedSlider<ParameterControllerType>* NewSlider()
			{
				LinkedSlider<ParameterControllerType>* slider = new LinkedSlider<ParameterControllerType>();
				slider->SetSynth(m_synth);
				AddParameterController(slider);
				return slider;
			}

		public:

			void VSTMode()
			{
				m_vstMode = true;
			}


			void PostModulationsChanged()
			{
				PostMessage(UWM_V2MODULATION_CHANGED,0,0);
			}

			void PostPatchChanged()
			{
				PostMessage(UWM_V2PATCH_CHANGED,0,0);
			}

			BEGIN_MSG_MAP_EX(View)
				MSG_WM_CREATE(OnCreate)
				MSG_WM_DESTROY(OnDestroy)
				MSG_WM_TIMER(OnTimer)
				if (uMsg == UWM_V2MODULATION_CHANGED)
				{
					SetMsgHandled(TRUE);
					UpdateModulations();
					lResult = 0;
					if(IsMsgHandled())
						return TRUE;
				}				
				if (uMsg == UWM_V2PATCH_CHANGED)
				{
					SetMsgHandled(TRUE);
					Update();
					lResult = 0;
					if(IsMsgHandled())
						return TRUE;
				}				
				//MSG_WM_SIZE(OnSize)
				CHAIN_MSG_MAP(__super)
			END_MSG_MAP()

			void Update()
			{
				UpdateParameterControllers();
				UpdateModulations();
#ifdef RONAN
				UpdateSpeechTexts();
#endif
				if (m_patchComboBox)
					m_patchComboBox.SetCurrentEntryId(m_synth->GetCurrentPatchIndex());
				if (m_channelComboBox)
					m_channelComboBox.SetCurrentEntryId(m_synth->GetCurrentChannelIndex());
			}

			void OnSize(UINT uType, CSize size)
			{
				//RectF rectClient = GetClientRect();
				//m_patchGroup.Move(rectClient);
				//m_patchGroup.Layout(false,false);
			}
	        
			// overridden
			void Paint(Graphics& graphics)
			{
				DrawGradient(graphics,GetClientRect(),ColorPresetBackground1);
			}

			template<typename ParameterControllerType, typename ParameterInfoType>
			void AddElements(Tab& page, const TopicInfo* topics, int topicCount, const ParameterInfoType* parms, bool applyColorScheme)
			{
				RectF rectSlider(PointF(0,0),GetSliderSize()); // default slider size
				RectF rectButtonGroup(PointF(0,0),GetButtonGroupSize()); // default button group size
				SizeF knobSize = GetSliderKnobSize(); // default slider knob size
				SizeF captionSize(GetSliderCaptionSize()); // default caption size for sliders
				RectF rectGroup(PointF(0,0),GetGroupSize()); // default group size

				TopicInfo* topic = (TopicInfo*)topics;
				ParameterInfoType* param = (ParameterInfoType*)parms;
				int paramIndexAbsolute = 0;

				float hueStep = 1.0f/(float)topicCount;
				float hue = 1.0f - hueStep;
				
				// walk through topics
				for(int topicIndex=0; topicIndex != topicCount; topicIndex++,topic++)
				{
					// we dont really use topicIndex
					// and advance topic++ instead. if topics was
					// terminated this would have been much more elegant.
					
					
					// check if group is visually empty
					bool createGroup = false;
					for(int paramIndex=0; paramIndex != topic->GetParameterCount(); paramIndex++)
					{
						if (param[paramIndex].GetControlType() != ControlTypeHidden)
						{
							createGroup = true;
							break;
						}
					}				

					if (createGroup)
					{

						Group* group = applyColorScheme?NewGroup(hue):NewGroup();
						hue -= hueStep;

						page.AddWindow(group->Create(page));
						group->SetWindowText(topic->GetName());
						group->Move(rectGroup);
		                
						for(int paramIndex=0; paramIndex != topic->GetParameterCount(); paramIndex++,param++)
						{
							// same here. param is increased all the time. we just count 
							// to see what belongs to the current topic and when to stop.

							switch (param->GetControlType())
							{
								case ControlTypeRadio:
									{
										LinkedButtonGroup<ParameterControllerType>* buttonGroup = NewButtonGroup<ParameterControllerType>();
										buttonGroup->SetParameterIndex(paramIndexAbsolute);
										group->AddWindow(buttonGroup->Create(*group),true);
										buttonGroup->Move(rectButtonGroup);
										buttonGroup->SetParameterInfo(param);
									} break;
								case ControlTypeSlider:
									{
										LinkedSlider<ParameterControllerType>* slider = NewSlider<ParameterControllerType>();
										slider->SetParameterIndex(paramIndexAbsolute);
										group->AddWindow(slider->Create(*group),true);
										slider->Move(rectSlider);
										slider->SetParameterInfo(param);
										slider->SetKnobSize(knobSize);
									} break;
								case ControlTypeHidden:
									{
										// well, skip.
									} break;
								default:
									break;
							}

							paramIndexAbsolute++;
						}

						// finally layout all the stuff neatly
						group->SetLargestCaptionSize(captionSize);
						group->Layout(false);
					}
				}
			}

			void CreatePatchTab()
			{
				Tab& tab = CreateTab(TabIndexPatch,_T("Patch"));
				AddElements<SynthParameterController,ParameterInfo>(
					tab,
					m_synth->GetTopicInfo(0),
					m_synth->GetTopicCount(),
					m_synth->GetParameterInfo(0),
					true);
				FinalizeTab(tab);
			}

			void CreateGlobalTab()
			{
				Tab& tab = CreateTab(TabIndexGlobal,_T("Global"));
				AddElements<SynthGlobalParameterController,ParameterInfo>(
					tab,
					m_synth->GetGlobalTopicInfo(0),
					m_synth->GetGlobalTopicCount(),
					m_synth->GetGlobalParameterInfo(0),
					false);
				FinalizeTab(tab);
			}

			void CreateAppearanceTab()
			{
				Tab& tab = CreateTab(TabIndexAppearance,_T("Appearance"));
				AddElements<AppearanceParameterController,Appearance::ParameterInfo>(
					tab,
					GetTheAppearance()->GetTopicInfo(),
					GetTheAppearance()->GetTopicCount(),
					GetTheAppearance()->GetParameterInfo(),
					false);
				FinalizeTab(tab);
			}

#ifdef RONAN
			void UpdateSpeechTexts()
			{
				for (int index = 0; index < SpeechEditCount; index++)
				{
					m_speechEdit[index].SetText(m_synth->GetSpeechText(index));
				}				
			}

			void OnSpeechEnglishEdit(Edit& edit, EditEvent editEvent, int parameter)
			{
				m_speechPhoneticEdit.SetText(m_synth->EnglishToPhonemes(edit.GetText()));
			}

			void OnSpeechEdit(Edit& edit, EditEvent editEvent, int parameter)
			{
				for (int index = 0; index < SpeechEditCount; index++)
				{
					if (&m_speechEdit[index] == &edit)
					{
						m_synth->SetSpeechText(index,m_speechEdit[index].GetText());
					}
				}
			}

			void CreateSpeechTab()
			{
				Tab& tab = CreateTab(TabIndexSpeech,_T("Speech"));

				tab.AddWindow(m_speechDesignGroup.Create(tab));
				m_speechDesignGroup.SetWindowText(_T("Design"));
				m_speechDesignGroup.Move(RectF(PointF(0,0),GetGroupSize()));

				m_speechDesignGroup.AddWindow(m_speechEnglishEdit.Create(m_speechDesignGroup),true);
				m_speechDesignGroup.SetWindowCaption(m_speechEnglishEdit,_T("English"));
				m_speechEnglishEdit.Move(RectF(PointF(0,0),GetSpeechEnglishEditSize()));
				m_speechEnglishEdit.SetEventTarget(this,&V2::GUI::View::OnSpeechEnglishEdit);

				m_speechDesignGroup.AddWindow(m_speechPhoneticEdit.Create(m_speechDesignGroup,true),true);
				m_speechDesignGroup.SetWindowCaption(m_speechPhoneticEdit,_T("Phonemes"));
				m_speechPhoneticEdit.Move(RectF(PointF(0,0),GetSpeechPhoneticEditSize()));

				m_speechDesignGroup.SetLargestCaptionSize(GetSliderCaptionSize());
				m_speechDesignGroup.Layout(false);
				
				tab.AddWindow(m_speechTextGroup.Create(tab));
				m_speechTextGroup.SetWindowText(_T("Text"));
				m_speechTextGroup.Move(RectF(PointF(0,0),GetGroupSize()));

				CString captionText;
				for (int index = 0; index < SpeechEditCount; index++)
				{
					captionText.Format(_T("Text #%02i"),index);
					m_speechTextGroup.AddWindow(m_speechEdit[index].Create(m_speechTextGroup),true);
					m_speechTextGroup.SetWindowCaption(m_speechEdit[index],captionText);
					m_speechEdit[index].Move(RectF(PointF(0,0),GetSpeechEditSize()));
					m_speechEdit[index].SetEventTarget(this,&V2::GUI::View::OnSpeechEdit);
				}				

				m_speechTextGroup.SetLargestCaptionSize(GetSliderCaptionSize());
				m_speechTextGroup.Layout(false);

				FinalizeTab(tab);
			}
#endif
			ModulationGroup m_modulationGroups[MaxModulationCount];

			void CreateModulations()
			{
				for (int index = 0; index < m_synth->GetModulationCount(); index++)
				{
					m_modulationTab.AddWindow(m_modulationGroups[index].Create(m_modulationTab));
					m_modulationGroups[index].SetView(this);
					m_modulationGroups[index].SetTarget(m_synth,index);
				}
				if (m_synth->GetModulationCount())
					m_modulationTab.Layout(false);
			}

			void DestroyModulations()
			{
				for (int index = 0; index < MaxModulationCount; index++)
				{
					if (m_modulationGroups[index])
					{
						m_modulationTab.RemoveWindow(m_modulationGroups[index]);
						m_modulationGroups[index].DestroyWindow();
					}
				}
			}

			void UpdateModulations()
			{
				DestroyModulations();
				CreateModulations();
				UpdateModulationValues();
			}

			void UpdateModulationValues()
			{
				for (int index = 0; index < m_synth->GetModulationCount(); index++)
				{
					m_modulationGroups[index].Update();
				}
			}

			void OnAddModulation(Button& button, bool isDown)
			{
				if (!isDown)
				{
					m_synth->AddModulation();
					UpdateModulations();
				}
			}

			void CreateModulationTab()
			{
				m_modulationTabGroup.Create(*this);
				m_modulationTabGroup.Move(RectF(PointF(0,0),GetTabSize()));

				m_modulationTabGroup.AddWindow(m_modulationTab.Create(m_modulationTabGroup),false);
				m_modulationTabGroup.SetWindowText(_T("Modulations"));
				// give some space
				m_modulationTab.Move(RectF(GetSpacingPoint(),GetModulationTabSize()));

				m_modulationTab.AddWindow(m_addModulationButton.Create(m_modulationTab));
				m_addModulationButton.SetWindowText(_T("Add Modulation"));
				m_addModulationButton.Move(RectF(PointF(0,0),GetModulationAddButtonSize()));			
				m_addModulationButton.SetEventTarget(this,&V2::GUI::View::OnAddModulation);

				CreateModulations();

				m_modulationTabGroup.Layout();
			}

			void SetFileName(LPCTSTR fileName)
			{
				m_fileName = fileName;
			}

			CString GetFileName()
			{
				return m_fileName;
			}

			void OnFileLoaded()
			{
				m_synth->Reset();
				UpdatePatchNames();
				PostPatchChanged();
			}

			void OnComboBox(ComboBox& comboBox, ComboBoxEvent event, int parameter)
			{
				if (&comboBox == &m_patchComboBox)
				{
					if (event == ComboBoxEventSelectionChanged)
					{
						m_synth->SetCurrentPatchIndex(parameter);
						Update();
					}
					else if (event == ComboBoxEventTextChanged)
					{
						m_synth->SetPatchName(parameter,comboBox.GetCurrentText());
					}
					if (m_vstMode && v2vstiAEffect) v2vstiAEffect->updateDisplay();
				}
				else if (&comboBox == &m_channelComboBox)
				{
					if (event == ComboBoxEventSelectionChanged)
					{
						m_synth->SetCurrentChannelIndex(parameter);
						Update();
					}
					if (event == ComboBoxEventBeforePopup)
					{
						UpdateChannelNames();
					}
				}
				else if (&comboBox == &m_fileComboBox)
				{
					// welcome to ekelsource-country

					switch(parameter)
					{
						case MenuCommandFileLoadBankOrPatch:
							{
								TCHAR filter[] = _T("V2 patches/banks\0*.v2b;*.v2p\0V2 sound banks (*.v2b)\0*.v2b\0V2 patches (*.v2p)\0*.v2p\0All Files\0*.*\0\0");
 								CFileDialog fileDialog(TRUE,_T("*.v2b;*.v2p"),NULL,OFN_HIDEREADONLY,filter,*this);
								if (fileDialog.DoModal() == IDOK)
								{
									TCHAR* fileName = fileDialog.m_szFileName;
									CString s = fileName;
									CString fn = fileName;
									CString path = fileName;
									if (path.GetLength())
									{
										fileS bankFile;
										sBool result = sFALSE;
										if (bankFile.open(path))
										{
											if (sdLoad(bankFile))
											{
												SetFileName(path);
												result = sTRUE;
											}
											bankFile.close();
										}
										OnFileLoaded();
										if (!result)
											ErrorBox(_T("The bank or patch couldn't be read."));
									}
								}
							} break;
						case MenuCommandFileImportV2M:
							{
								TCHAR filter[] = "V2 modules (*.v2m)\0*.v2m;*.v2m\0All Files\0*.*\0\0";
 								CFileDialog fileDialog(TRUE,"*.v2m",NULL,OFN_HIDEREADONLY,filter,*this);
								if (fileDialog.DoModal() == IDOK)
								{
									TCHAR* ptr = fileDialog.m_szFileName;
									TCHAR fn[256]; strcpy(fn,ptr);
									TCHAR* r = strrchr(fn,'/'); if (r) strcpy(fn,r+1);
									r = strrchr(fn,'\\'); if (r) strcpy(fn,r+1);
									r = strrchr(fn,':'); if (r) strcpy(fn,r+1);
									r = strrchr(fn,'.'); if (r) *r=0;

									CString s = ptr;
									CString path = ptr;
									if (path.GetLength())
									{
										fileS v2mFile;
										sBool result = sFALSE;
										if (v2mFile.open(path))
										{
											if (sdImportV2MPatches(v2mFile,fn))
											{
												result = sTRUE;
											}
											v2mFile.close();
										}
										OnFileLoaded();
										if (!result)
											ErrorBox(_T("The module couldn't be read."));
									}
								}
							} break;
						case MenuCommandFileSaveBank:
							{
								TCHAR filter[] = _T("V2 sound banks (*.v2b)\0*.v2b\0All Files (*.*)\0*.*\0\0");
								CFileDialog fileDialog(FALSE,"*.v2b",GetFileName(),OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,filter,*this);
								if (fileDialog.DoModal() == IDOK)
								{
									CString path = fileDialog.m_szFileName;
									
									if (path.GetLength())
									{
										fileS bankFile;
										sBool result = sFALSE;
										if (bankFile.open(path,fileS::cr|fileS::wr))
										{
											if (sdSaveBank(bankFile))
											{
												SetFileName(path);
												result = sTRUE;
											}
											bankFile.close();
										}
										if (!result)
											ErrorBox(_T("The bank couldn't be saved."));
									}
								}
							} break;
						case MenuCommandFileSavePatch:
							{
								TCHAR fn[256];
								_tcscpy(fn,m_synth->GetPatchName(m_synth->GetCurrentPatchIndex()));
								_tcscat(fn,_T(".v2p"));
								TCHAR filter[] = _T("V2 patches (*.v2p)\0*.v2p\0All Files (*.*)\0*.*\0\0");
								CFileDialog fileDialog(FALSE,"*.v2p",fn,OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,filter,*this);
								if (fileDialog.DoModal() == IDOK)
								{
									CString path = fileDialog.m_szFileName;
									
									if (path.GetLength())
									{
										fileS patchFile;
										sBool result = sFALSE;
										if (patchFile.open(path,fileS::cr|fileS::wr))
										{
											if (sdSavePatch(patchFile))
											{
												result = sTRUE;
											}
											patchFile.close();
										}										
										if (!result)
											ErrorBox(_T("The patch couldn't be saved."));
									}
								}
							} break;
						case MenuCommandFileLoadAppearance:
							{
								TCHAR filter[] = _T("V2 appearances (*.v2a)\0*.v2a\0All Files (*.*)\0*.*\0\0");
 								CFileDialog fileDialog(TRUE,_T("*.v2a"),NULL,0,filter,*this);
								if (fileDialog.DoModal() == IDOK)
								{
									TCHAR* fileName = fileDialog.m_szFileName;
									CString path = fileName;
									if (path.GetLength())
									{
										fileS appearanceFile;
										sBool result = sFALSE;
										if (appearanceFile.open(path))
										{
											if (GetTheAppearance()->LoadAppearance(appearanceFile))
											{												
												result = sTRUE;
											}
											appearanceFile.close();
										}
										if (!result)
											ErrorBox(_T("The bank or patch couldn't be read."));
										else
										{
											Update();
											InvalidateAllWindows();
										}
									}
								}
							} break;
						case MenuCommandFileSaveAppearance:
							{
								TCHAR filter[] = _T("V2 appearances (*.v2a)\0*.v2a\0All Files (*.*)\0*.*\0\0");
								CFileDialog fileDialog(FALSE,_T("*.v2a"),_T("hund.v2a"),OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,filter,*this);
								if (fileDialog.DoModal() == IDOK)
								{
									CString path = fileDialog.m_szFileName;
									
									if (path.GetLength())
									{
										fileS appearanceFile;
										sBool result = sFALSE;
										if (appearanceFile.open(path,fileS::cr|fileS::wr))
										{
											if (GetTheAppearance()->SaveAppearance(appearanceFile))
											{
												result = sTRUE;
											}
											appearanceFile.close();
										}										
										if (!result)
											ErrorBox(_T("The appearance couldn't be saved."));
									}
								}
							} break;
						case MenuCommandFileResetAppearance:
							{
								if (QuestionBox(*this, _T("Really reset appearance?")))
								{
									GetTheAppearance()->Reset();
									InvalidateAllWindows();
									Update();
								}								
							} break;
						default:
							break;
					}
				}
				else if (&comboBox == &m_editComboBox)
				{
					switch(parameter)
					{
						case MenuCommandEditCopyPatch:
							{
								sdCopyPatch();
							} break;
						case MenuCommandEditPastePatch:
							{
								sdPastePatch();
								UpdatePatchNames();
								PostPatchChanged();
							} break;
						case MenuCommandEditResetPatch:
							{
								sdInitPatch();
								UpdatePatchNames();
								PostPatchChanged();
							} break;
						default:
							break;
					}
				}
			}			

			void UpdatePatchNames()
			{
				if (!m_patchComboBox)
					return;
				m_patchComboBox.Clear();
				for (int index = 0; index < m_synth->GetPatchCount(); index++)
				{
					m_patchComboBox.AppendMenu(m_synth->GetPatchName(index),index);
				}
				m_patchComboBox.SetCurrentEntryId(m_synth->GetCurrentPatchIndex());
			}

			void UpdateChannelNames()
			{
				if (!m_channelComboBox)
					return;
				CString text;
				m_channelComboBox.Clear();
				for (int index=0; index < 16; index++)
				{	
					const char* name = GetTheAppearance()->GetClient()->GetChannelName(index);
					if (name)
					{
						text.Format(_T("#%02i: %s"),index+1,name);
					}
					else
					{
						text.Format(_T("Channel #%02i"),index+1);
					}
					m_channelComboBox.AppendMenu(text,index);
				}
				m_channelComboBox.SetCurrentEntryId(m_synth->GetCurrentChannelIndex());
			}

#ifndef SINGLECHN
			void UpdateRecordButton()
			{
				if (m_synth->IsRecording())
				{
					m_recordButton.SetSelected(true);
					m_recordButton.SetWindowText(_T("Stop"));
				}
				else
				{
					m_recordButton.SetSelected(false);
					m_recordButton.SetWindowText(_T("Record"));
				}
			}

			void OnRecordButton(Button& button, bool isDown)
			{
				if (!isDown)
				{
					if (m_synth->IsRecording())
					{
						m_synth->StopRecording();

						TCHAR filter[] = "V2 modules (*.v2m)\0*.v2m\0All Files (*.*)\0*.*\0\0";
						CFileDialog fileDialog(FALSE,"*.v2m",GetFileName(),OFN_HIDEREADONLY,filter,*this);
						if (fileDialog.DoModal() == IDOK)
						{
							CString path = fileDialog.m_szFileName;								
							if (path.GetLength())
							{
								int result = m_synth->WriteRecording(path);
								if (!result)
									ErrorBox(_T("The module couldn't be saved. Possibly nothing has been recorded."));
							}
						}						
					}
					else
					{
						m_synth->StartRecording();
					}
					UpdateRecordButton();
				}
			}
#endif

			void OnTimer(UINT id, TIMERPROC timerProc)
			{
				if (id == StatusTimerID)
				{
					UpdateVU();
					m_midiStatsGroup.Update();
				}
			}

			void UpdateVU()
			{
				float dbLeft, dbRight;
				bool clipLeft, clipRight;

				m_synth->GetVUData(dbLeft,clipLeft,dbRight,clipRight);

				m_vu[0].SetValue(dbLeft,clipLeft);
				m_vu[1].SetValue(dbRight,clipRight);
			}

			void StartStatusTimer()
			{
				SetTimer(StatusTimerID,20);
			}

			void StopStatusTimer()
			{
				KillTimer(StatusTimerID);
			}

			void CreateStatusGroup()
			{
				m_statusGroup.Create(*this);
				m_statusGroup.SetWindowText(GetStatusWindowText());

				for(int index = 0; index < VUCount; index++)
				{
					m_statusGroup.AddWindow(m_vu[index].Create(m_statusGroup));
					m_vu[index].SetValue(0.0f,false);
				}				

				m_statusGroup.AddWindow(m_decalSpacer.Create(m_statusGroup));
				m_decalSpacer.Move(RectF(PointF(0,0),GetLogoSpaceSize()));
				m_decalSpacer.ShowWindow(SW_HIDE);

				m_statusGroup.AddWindow(m_midiStatsGroup.Create(m_statusGroup));

				for(int index = 0; index < VUCount; index++)
				{
					m_vu[index].Move(RectF(PointF(0,0),SizeF(GetVUSize().Width,m_midiStatsGroup.GetSize().Height)));
				}				

				m_statusGroup.AddWindow(m_fileComboBox.Create(m_statusGroup),false,true);
				m_fileComboBox.SetMenuBox(true);
				m_fileComboBox.SetEventTarget(this,&V2::GUI::View::OnComboBox);
				m_fileComboBox.Move(RectF(PointF(0,0),GetFileComboBoxSize()));
				m_fileComboBox.SetWindowText(_T("File"));
				m_fileComboBox.AppendMenu(_T("Load Patch/Bank"),MenuCommandFileLoadBankOrPatch);
				m_fileComboBox.AppendMenu(_T("Import V2M Patches"),MenuCommandFileImportV2M);
				m_fileComboBox.AppendSeparator();
				m_fileComboBox.AppendMenu(_T("Save Bank"),MenuCommandFileSaveBank);
				m_fileComboBox.AppendMenu(_T("Save Patch"),MenuCommandFileSavePatch);
				m_fileComboBox.AppendSeparator();
				m_fileComboBox.AppendMenu(_T("Load Appearance"),MenuCommandFileLoadAppearance);
				m_fileComboBox.AppendMenu(_T("Save Appearance"),MenuCommandFileSaveAppearance);
				m_fileComboBox.AppendMenu(_T("Reset Appearance"),MenuCommandFileResetAppearance);

				m_statusGroup.AddWindow(m_editComboBox.Create(m_statusGroup));
				m_editComboBox.SetMenuBox(true);
				m_editComboBox.SetEventTarget(this,&V2::GUI::View::OnComboBox);
				m_editComboBox.Move(RectF(PointF(0,0),GetEditComboBoxSize()));
				m_editComboBox.SetWindowText(_T("Edit"));
				m_editComboBox.AppendMenu(_T("Copy Patch"),MenuCommandEditCopyPatch);
				m_editComboBox.AppendMenu(_T("Paste Patch"),MenuCommandEditPastePatch);
				m_editComboBox.AppendSeparator();
				m_editComboBox.AppendMenu(_T("Reset Patch"),MenuCommandEditResetPatch);

				m_statusGroup.AddWindow(m_channelComboBox.Create(m_statusGroup));

				if (!m_vstMode)
				{
					m_channelComboBox.Move(RectF(PointF(0,0),GetChannelComboBoxSize()));
					m_channelComboBox.SetReadOnly(true);
					m_channelComboBox.SetEventTarget(this,&V2::GUI::View::OnComboBox);
					UpdateChannelNames();
				}

				m_statusGroup.AddWindow(m_patchComboBox.Create(m_statusGroup));
				m_patchComboBox.ShowIDs(true);
				if (m_vstMode)
				{
					m_patchComboBox.SetAllowSelect(false);
					m_patchComboBox.SetReadOnly(true);
				}
				m_patchComboBox.Move(RectF(PointF(0,0),GetPatchComboBoxSize()));
				m_patchComboBox.SetEventTarget(this,&V2::GUI::View::OnComboBox);
				UpdatePatchNames();


#ifndef SINGLECHN
				m_statusGroup.AddWindow(m_recordButton.Create(m_statusGroup));
				m_recordButton.Move(RectF(PointF(0,0),GetRecordButtonSize()));
				m_recordButton.SetEventTarget(this,&V2::GUI::View::OnRecordButton);
				UpdateRecordButton();
#endif

			}

			void AdjustStatusGroup()
			{
				float width = m_modulationTabGroup.GetSize().Width + m_tabGroup.GetSize().Width + GetSpacing();
				m_statusGroup.Move(RectF(PointF(0,0),SizeF(width,m_statusGroup.GetSize().Height)));
				m_statusGroup.Layout(false,false,true);

				m_statusGroup.SetDecalBitmap(IDB_V2LOGO);
				m_statusGroup.SetDecalBitmapPosition(m_decalSpacer.GetPosition());

				m_statusGroup.Fit();
				m_statusGroup.Move(RectF(PointF(0,0),SizeF(width,m_statusGroup.GetSize().Height)));
			}

			void CreateTabGroup() 
			{
				m_tabGroup.Create(*this);
				m_tabGroup.Move(RectF(PointF(0,0),GetTabSize()));

				CreatePatchTab();
				CreateGlobalTab();
				#ifdef RONAN
				CreateSpeechTab();
				#endif
				CreateAppearanceTab();
				
				m_tabGroup.Layout();
			}

			BOOL OnCreate(LPCREATESTRUCT lpCreateStruct)
			{
				SetMsgHandled(FALSE);

				PrepareModulationMenus();

				CreateStatusGroup();

				CreateTabGroup();
				
				CreateModulationTab();

				AdjustStatusGroup();
				
				// layout the view
//				Move(RectF(GetPosition(),SizeF(900.0f,600.0f)));
				Move(RectF(GetPosition(),SizeF(900.0f,600.0f)));
				AddChildWindows();
				Layout();

				// update all controls
				Update();	

				StartStatusTimer();

				return TRUE;
			}

			void Layout()
			{
				__super::Layout(false,true);
			}

			void OnDestroy()                
			{
				StopStatusTimer();

				ClearParameterControllers();

				if (m_decalSpacer)
					m_decalSpacer.DestroyWindow();

				if (m_speechDesignGroup)
					m_speechDesignGroup.DestroyWindow();
				if (m_speechTextGroup)
					m_speechTextGroup.DestroyWindow();
				if (m_speechEnglishEdit)
					m_speechEnglishEdit.DestroyWindow();
				if (m_speechPhoneticEdit)
					m_speechPhoneticEdit.DestroyWindow();

				for(int index = 0; index < SpeechEditCount; index++)
				{
					if (m_speechEdit[index])
						m_speechEdit[index].DestroyWindow();
				}			

				for(int index = 0; index < TabIndexCount; index++)
				{
					if (m_tabs[(TabIndex)index])
					{
						m_tabs[(TabIndex)index].DestroyWindow();
					}
				}

				m_midiStatsGroup.DestroyWindow();

				DestroyModulations();
				m_modulationSourceMenu.DestroyMenu();
				m_modulationTargetMenu.DestroyMenu();
				m_addModulationButton.DestroyWindow();

				for(int index = 0; index < VUCount; index++)
				{
					m_vu[index].DestroyWindow();
				}				

				if (m_patchComboBox)
					m_patchComboBox.DestroyWindow();
				if (m_channelComboBox)
					m_channelComboBox.DestroyWindow();
				m_fileComboBox.DestroyWindow();
				m_editComboBox.DestroyWindow();

				m_tabGroup.DestroyWindow();
				m_statusGroup.DestroyWindow();
				m_modulationTab.DestroyWindow();
				m_modulationTabGroup.DestroyWindow();

				SetMsgHandled(FALSE);
			}

			void SetSynth(Synth* synth)
			{
				m_synth = synth;
			}

			Synth* GetSynth()
			{
				return m_synth;
			}

			View()
			{
				m_vstMode = false;
				//NoOffscreenPainting();
				m_synth = GetTheSynth();
			}

			~View()
			{
			}
		};

		// ==========================================================
		//
		//	Frame
		//
		// ==========================================================

		class Frame
			: public WindowTemplate<Frame>
		{
		public:
			V2::GUI::View m_view;
			int m_currentProgram;

			Frame()
			{
				v2curpatch = 0;
				//m_view.EnablePatchControls();
			}

			void SetChannel(int channel, bool silent=false)
			{
				if (m_view)
				{
					m_view.GetSynth()->SetCurrentChannelIndex(channel,silent);
					m_view.PostPatchChanged();
				}
			}

			void SetProgram(int program, bool silent=false)
			{
				if (m_view)
				{
					m_view.GetSynth()->SetCurrentPatchIndex(program,silent);
					m_view.PostPatchChanged();
				}
			}

			BEGIN_MSG_MAP_EX(Frame)
				MSG_WM_CREATE(OnCreate)
				MSG_WM_DESTROY(OnDestroy)
				MSG_WM_CLOSE(OnClose)
				MSG_WM_MOVE(OnMove)
				MSG_WM_SIZE(OnSize)
				MSG_WM_LBUTTONDOWN(OnLButtonDown)
				MSG_WM_NCLBUTTONDOWN(OnLButtonDown)
				CHAIN_MSG_MAP(__super)
			END_MSG_MAP()

			void OnPrev()
			{
				if(m_currentProgram > 0)
					SetProgram(m_currentProgram-1);
				else
					SetProgram(127);        
			}

			void OnNext()
			{
				if(m_currentProgram < 127)
					SetProgram(m_currentProgram+1);
				else
					SetProgram(0);        
			}

			void Rename()
			{

			}

			HWND Create(HWND hWndParent=NULL)
			{				
				UINT windowStyles = //WS_BORDER|WS_CAPTION|WS_SYSMENU|WS_VISIBLE;
					WS_CAPTION|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_SYSMENU|WS_DLGFRAME|WS_OVERLAPPED|WS_MINIMIZEBOX|WS_VISIBLE;
				UINT windowExStyles = WS_EX_TOOLWINDOW;
				HWND hWnd = CWindowImpl<Frame>::Create(hWndParent,rcDefault,GetFrameWindowTitle(),windowStyles,windowExStyles);
				//SetWindowPos(HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
				return hWnd;
			}

			void OnSize(UINT uType, CSize size)
			{
				if (*this != GetFocus())
					SetFocus();
				SetMsgHandled(FALSE);
			}

			void OnMove(CPoint position)
			{
				/*
				RECT rect;
				GetClientRect(&rect);
				if(m_view)
					m_view.SetWindowPos(0,&rect,SWP_NOZORDER|SWP_SHOWWINDOW);
				*/
				SetMsgHandled(FALSE);
			}

			void OnLButtonDown(UINT uMode, CPoint& position)
			{
				if (*this != GetFocus())
					SetFocus();
				SetMsgHandled(FALSE);
			}


			BOOL OnCreate(LPCREATESTRUCT lpCreateStruct)
			{           
				if(!m_view)
				{
					HWND m_hWndClient = m_view.Create(*this);
					//::SendMessage(m_hWndClient, VPE_GOTOPATCH, (WPARAM)m_currentProgram,0);

					SizeF size = m_view.GetSize();
					SetClientSize(size);

				}
				SetMsgHandled(FALSE);
				return TRUE;
			}

			void OnClose()
			{
				SetMsgHandled(TRUE);
				ShowWindow(SW_HIDE);
			}

			void OnDestroy()
			{
				SetMsgHandled(FALSE);
				if (m_view)
					m_view.DestroyWindow();
			}
		};

	}; // GUI
}; // V2

// you have reached the end of the source.
// please close your IDE.
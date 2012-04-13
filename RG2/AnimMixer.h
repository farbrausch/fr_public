// This code is in the public domain. See LICENSE for details.

// AnimMixer.h: interface for the CAnimMixer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ANIMMIXER_H__AD553F71_7610_44D5_986F_2842F341BEE6__INCLUDED_)
#define AFX_ANIMMIXER_H__AD553F71_7610_44D5_986F_2842F341BEE6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class frAnimationClip;

class CAnimMixer : public CWindowImpl<CAnimMixer>  
{
public:
  DECLARE_WND_CLASS(0);

	enum eFlags
	{
		amSnapLeft		= 0x01,
		amSnapMid			= 0x02,
		amSnapRight		=	0x04,
		amFollowMode	= 0x08
	};

  HWND      m_oldFocusWin;
  
  sF32      m_frameStep;
	sInt			m_startPixel;

  sInt      m_moveElement;
  sInt      m_baseRelStart;
  sInt      m_baseTrack, m_maxTrack;
  CPoint    m_dragPoint, m_scrlPoint;
  sInt      m_curElem;

  CRect     m_rcDraw;

  sBool     m_lbDown, m_mbDown;

  sBool     m_useRuleMouse;
  CPoint    m_ruleMouseCoord;

  sInt      m_timeBarPos;
  sInt      m_curFrame;

	sInt			m_flags;

  frButton*	m_cmdButtons;
  sInt      m_cmdButtonCount;

	frAnimationClip*	m_thisClip;

	CAnimMixer();
	virtual ~CAnimMixer();

  BEGIN_MSG_MAP(CAnimMixer)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButton)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnLButton)
    MESSAGE_HANDLER(WM_MBUTTONDOWN, OnMButton)
    MESSAGE_HANDLER(WM_MBUTTONUP, OnMButton)
    MESSAGE_HANDLER(WM_COMMAND, OnCommand)
    MESSAGE_HANDLER(WM_FRAME_CHANGED, OnFrameChanged)
    MESSAGE_HANDLER(WM_RULEMOUSEINPUT, OnRuleMouseInput)
    MESSAGE_HANDLER(WM_SHOWWINDOW, OnShowWindow)
  END_MSG_MAP()

  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnLButton(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMButton(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnFrameChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnRuleMouseInput(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnShowWindow(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  
  sInt mapTime(sF32 frame) const;
  void clipChanged();
  void drawBar(sInt mode = 0);

  void processMove(sInt cursorX, sInt cursorY);
  void processScroll(sInt cursorX, sBool scrollDirect = sFALSE);
	void updateThisClip();
	void getElemRect(sInt elem, RECT& rc) const;
	sInt getMaxTrackNumber() const;
	sInt performSnap(sInt elem, sInt framePos) const;
};

#endif // !defined(AFX_ANIMMIXER_H__AD553F71_7610_44D5_986F_2842F341BEE6__INCLUDED_)

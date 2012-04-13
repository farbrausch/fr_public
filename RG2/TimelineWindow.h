// This code is in the public domain. See LICENSE for details.

// TimelineWindow.h: interface for the CTimelineWindow class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TIMELINEWINDOW_H__3FADEE79_2147_4F77_85D4_724EE79DEC08__INCLUDED_)
#define AFX_TIMELINEWINDOW_H__3FADEE79_2147_4F77_85D4_724EE79DEC08__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class frAnimationClip;

class CTimelineButton: public frButtonImpl
{
private:
  CBitmapHandle   m_bmp;
  sInt            m_buttonInd;

public:
  void      SetBitmap(HBITMAP bmp);
  void      SetIndex(sInt ind);

  void      DoPaint(CDCHandle dc);
};

class CTimelineWindow : public CWindowImpl<CTimelineWindow>, public CIdleHandler
{
public:
  DECLARE_WND_CLASS(0);

  CBitmap						m_buttonsBmp;
  CTimelineButton		m_buttons[6];
  sInt							m_playState; // 0=do nothing 1=play

  frButton					m_clipNew, m_clipRename, m_clipDelete, m_renderVideo;
  frButton					m_sound;  
  frNumEditControl	m_startFrame, m_bpmRate;
  
  sU32							m_curClip;
  sInt							m_curFrame;

	frAnimationClip*	m_clip;
	sInt							m_clipLength;
	sInt							m_fullClipLength;
	sBool							m_isSimpleClip;

  CComboBox					m_clipName;

  sInt							m_oldClipEnd;
	sInt							m_timeBarPos;
  sS64							m_startTime;

  sBool							m_lButtonDown;
  sS32							m_lastPlayFrame;

	CTimelineWindow();
	virtual ~CTimelineWindow();

  BEGIN_MSG_MAP(CTimelineWindow)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_SET_CURRENT_CLIP, OnSetCurrentClip)
    MESSAGE_HANDLER(WM_CLIP_CHANGED, OnClipChanged)
    MESSAGE_HANDLER(WM_GET_CLIP_ID, OnGetClipID)
    MESSAGE_HANDLER(WM_CTLCOLOREDIT, OnControlColorEdit)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
    MESSAGE_HANDLER(WM_FILE_LOADED, OnFileLoaded)
    MESSAGE_HANDLER(WM_SET_CURRENT_FRAME, OnSetCurrentFrame)
    COMMAND_ID_HANDLER(101, OnPlay)
    COMMAND_ID_HANDLER(102, OnFrame)
    COMMAND_ID_HANDLER(104, OnFrame)
    COMMAND_ID_HANDLER(105, OnPlay)
    COMMAND_ID_HANDLER(150, OnSound)
    COMMAND_HANDLER(200, CBN_SELCHANGE, OnClipSelChange)
    COMMAND_ID_HANDLER(300, OnClipRename)
    COMMAND_ID_HANDLER(301, OnClipDelete)
    COMMAND_ID_HANDLER(302, OnClipNew)
    COMMAND_ID_HANDLER(303, OnRenderVideo)
		COMMAND_ID_HANDLER(400, OnStartFrameChange)
    COMMAND_ID_HANDLER(500, OnBPMChange)
  END_MSG_MAP()

  BOOL    OnIdle();

  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSetCurrentClip(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnClipChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnGetClipID(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnControlColorEdit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnFileLoaded(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSetCurrentFrame(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnPlay(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnSound(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnFrame(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnClipSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnClipRename(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnClipDelete(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnClipNew(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnStartFrameChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnBPMChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnRenderVideo(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  
private:
  void    SetPlayState(sInt newState);
  void    SetCurrentClip(sU32 clipID);
  
  void    fillClipComboBox();
  sS32    getTime();

  void    processScratch(CPoint pnt);
  void    setCurrentFrame(sInt frameNum, sBool reseekSound=sFALSE, sBool noUpdate=sFALSE);

  void    drawTimeBar(sInt mode = 0);
	sBool		updateClipPtr();
};

#endif // !defined(AFX_TIMELINEWINDOW_H__3FADEE79_2147_4F77_85D4_724EE79DEC08__INCLUDED_)

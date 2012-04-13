// This code is in the public domain. See LICENSE for details.

// 3dPreviewWindow.h: interface for the CPreviewWindow class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_3DPREVIEWWINDOW_H__D42F90BA_E760_49D5_A90A_7841E5C4D47C__INCLUDED_)
#define AFX_3DPREVIEWWINDOW_H__D42F90BA_E760_49D5_A90A_7841E5C4D47C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class frModel;

class C3DPreviewClientWindow : public CWindowImpl<C3DPreviewClientWindow>, public CIdleHandler
{
public:
  DECLARE_WND_CLASS(0);

  sU32          m_curOp;
  HWND          m_oldFocusWin;
  sBool         m_wireframe, m_showCull;
	sInt					m_camMode;

  fr::vector    m_camPos;
  sF32          m_camRotH, m_camRotP;

  sInt          m_mButton, m_mdx, m_mdy;
  sBool         m_mTimerSet;

  sBool         m_lButtonDown, m_rButtonDown;
  CPoint        m_dragOrigin, m_moveOrigin;

  sBool         m_useRuleMouse;
  sBool         m_engineInitialized;

  BEGIN_MSG_MAP(C3DPreviewClientWindow)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
    MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButtonDown)
    MESSAGE_HANDLER(WM_RBUTTONUP, OnRButtonUp)
    MESSAGE_HANDLER(WM_MBUTTONDOWN, OnMButtonDown)
    MESSAGE_HANDLER(WM_RULEMOUSEINPUT, OnRuleMouseInput)
    MESSAGE_HANDLER(WM_GET_THIS, OnGetThis)
  ALT_MSG_MAP(1)
    MESSAGE_HANDLER(WM_SET_DISPLAY_MESH, OnSetDisplayMesh)
		MESSAGE_HANDLER(WM_STORE_DATA, OnStoreData)
  END_MSG_MAP()

  BOOL OnIdle();
  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnRButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSetDisplayMesh(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnRuleMouseInput(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnGetThis(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnStoreData(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  
  void ProcessMouse(sInt button, sInt dx, sInt dy);
  void ProcessInput(sBool paintIfNecessary=sTRUE);
  void PaintModel(sBool noScreen=sFALSE, sInt w=0, sInt h=0, sF32 aspect=1.333f);
  frModel *lockModel() const;
  void unlockModel() const;

  void ResetCamera();
  void CalcCamMatrix(fr::matrix &cam);
  void SetWireframe(sBool wireframe);
  void SetShowCull(sBool showCull);
	void SetCamMode(sInt camMode);
};

class C3DPreviewWindow: public CWindowImpl<C3DPreviewWindow>
{
public:
  DECLARE_WND_CLASS(0);
  
  C3DPreviewClientWindow  m_client;
  frButton                m_wireframe, m_showCull, m_resetCam;
	frButtonChooser					m_camMode;
  
  BEGIN_MSG_MAP(C3DPreviewWindow)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
    MESSAGE_HANDLER(WM_COMMAND, OnCommand)
    MESSAGE_HANDLER(WM_GET_THIS, OnGetThis)
    CHAIN_MSG_MAP_ALT_MEMBER(m_client, 1)
  END_MSG_MAP()

  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnGetThis(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};

#endif // !defined(AFX_3DPREVIEWWINDOW_H__D42F90BA_E760_49D5_A90A_7841E5C4D47C__INCLUDED_)

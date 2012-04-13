// This code is in the public domain. See LICENSE for details.

// PreviewWindow.h: interface for the CPreviewWindow class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PREVIEWWINDOW_H__D42F90BA_E760_49D5_A90A_7841E5C4D47C__INCLUDED_)
#define AFX_PREVIEWWINDOW_H__D42F90BA_E760_49D5_A90A_7841E5C4D47C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "viewcomm.h"
#include "frcontrol.h"

struct frTexture;

class CPreviewClientWindow : public CWindowImpl<CPreviewClientWindow>
{
public:
  DECLARE_WND_CLASS(0);
  
  sU32          m_curOp;
  HWND          m_hWndSelect, m_oldFocusWin;
  sBool         m_tiled, m_lButtonDown;
	sInt					m_alphaMode;
  
  sU8           *m_imageBuf;
  sBool         m_update;
  sInt          m_scale1, m_scale2;
  
  sInt          m_scrlX, m_scrlY;
  sInt          m_scrlMaxX, m_scrlMaxY;
  
  sBool         m_clip;
  CPoint        m_dragOrigin;
  
  HCURSOR       m_crossCursor, m_arrowCursor;
  
  CPreviewClientWindow();
  virtual ~CPreviewClientWindow();
  
  BEGIN_MSG_MAP(CPreviewClientWindow)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
    MESSAGE_HANDLER(WM_MBUTTONDOWN, OnMButtonDown)
    MESSAGE_HANDLER(WM_MBUTTONUP, OnMButtonUp)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
  ALT_MSG_MAP(1)
    MESSAGE_HANDLER(WM_SET_DISPLAY_TEXTURE, OnSetDisplayTexture)
    MESSAGE_HANDLER(WM_POINTSELECT, OnPointSelect)
  END_MSG_MAP()
    
  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSetDisplayTexture(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnPointSelect(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnControlColor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  
  void UpdateScrollSize(sBool redraw);
  
  BOOL CursorHitTest(POINT pt) const;
  
  sInt ImageWidth() const;
  sInt ImageHeight() const;
  sBool UpdateImage();
  void CalcXYPos(const POINT pt, sInt &x, sInt &y);
  void CalcXYPos(LPARAM lParam, sInt &x, sInt &y);

  void SetTiled(sBool tiled);
  void SetScale(sInt scale1, sInt scale2);
	void SetAlphaMode(sInt mode);
  
  void DoPaint(CDCHandle dc);
  
private:
  frTexture *getCurrentTex() const;
};

class CPreviewWindow: public CWindowImpl<CPreviewWindow>, public CMessageFilter
{
public:
  DECLARE_WND_CLASS(0);
  
  CPreviewClientWindow  m_client;
  frButtonChooser       m_zoom, m_alpha;
  frButton              m_tile;
  CBrush                m_bgBrush;
  
  BEGIN_MSG_MAP(CPreviewWindow)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_COMMAND, OnCommand)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
    MESSAGE_HANDLER(WM_CTLCOLORBTN, OnControlColor)
    CHAIN_MSG_MAP_ALT_MEMBER(m_client, 1)
  END_MSG_MAP()

  BOOL PreTranslateMessage(MSG *pMsg);
  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnControlColor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};

#endif // !defined(AFX_PREVIEWWINDOW_H__D42F90BA_E760_49D5_A90A_7841E5C4D47C__INCLUDED_)

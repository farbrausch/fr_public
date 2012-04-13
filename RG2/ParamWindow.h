// This code is in the public domain. See LICENSE for details.

// ParamWindow.h: interface for the CParamWindow class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PARAMWINDOW_H__6C7BBC02_95D7_4788_8CF1_C00A8515ACDA__INCLUDED_)
#define AFX_PARAMWINDOW_H__6C7BBC02_95D7_4788_8CF1_C00A8515ACDA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "viewcomm.h"

class frPlugin;
struct frParam;

class CParamWindow : public CWindowImpl<CParamWindow>, public CMessageFilter, public CIdleHandler
{
public:
  DECLARE_WND_CLASS(0);

	CParamWindow();
	virtual ~CParamWindow();

  struct paramStruct;

  struct focusItem
  {
    HWND  hWnd;
    sBool group;
  };

  typedef std::vector<focusItem>  focusList;
  
  paramStruct     *m_params;
  sU32            m_nParams, m_curParam, m_pStart;

  frPlugin        *m_curPlug;
  sU32            m_curPlugID;
  
  sInt            m_curEditParam;
  sBool           m_updateParams;
  focusList       m_focusList;
  CPoint          m_dragOrigin;

  CImageList      m_animMarks;
  sInt            m_leftMargin;

  sInt            m_scrlX, m_scrlY;
  sInt            m_scrlSX, m_scrlSY;

  frParam*        m_size;
  frParam*        m_nameParam;

  BEGIN_MSG_MAP(CParamWindow)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_COMMAND, OnCommand)
    MESSAGE_HANDLER(WM_CTLCOLOREDIT, OnControlColor)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_SET_EDIT_PLUGIN, OnSetEditPlugin)
    MESSAGE_HANDLER(WM_MBUTTONDOWN, OnMButtonDown)
    MESSAGE_HANDLER(WM_MBUTTONUP, OnMButtonUp)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButtonDown)
  END_MSG_MAP()

  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnControlColor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSetEditPlugin(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  
  BOOL PreTranslateMessage(MSG *pMsg);
  BOOL OnIdle();

  void DoPaint(CDCHandle dc);

  void SetEditedPlugin(sU32 plID, sBool force=sFALSE);
  void GetParamValue(sInt ind);
	void SetParamValue(sInt ind);
	void UpdateLinkParam(sInt ind, sU32 newLink);
	void UpdateParamValue(sInt ind);
  sInt GetAnimState(sInt param);
  
  void ClearFocusList();
  void AddToFocusList(HWND hWnd, sBool newGroup=sFALSE);
  sInt GetFocusGroupStart(sInt pos) const;
  sInt GetFocusGroupLength(sInt pos) const;
  sInt GetFocusGroupNum(sInt pos) const;

  void Scroll(sInt ox, sInt oy);
};

#endif // !defined(AFX_PARAMWINDOW_H__6C7BBC02_95D7_4788_8CF1_C00A8515ACDA__INCLUDED_)

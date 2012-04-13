// This code is in the public domain. See LICENSE for details.

// GraphWindow.h: interface for the CGraphWindow class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GRAPHWINDOW_H__24A9687F_05CB_44BD_9275_31E1B447A624__INCLUDED_)
#define AFX_GRAPHWINDOW_H__24A9687F_05CB_44BD_9275_31E1B447A624__INCLUDED_

#include "types.h"	// Added by ClassView
#include "resource.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

struct frPluginDef;
struct savedSelection;

class CGraphWindow : public CWindowImpl<CGraphWindow>
{
public:
  DECLARE_WND_CLASS(0);

  CPoint                  m_dragOrigin, m_oldDragPoint, m_sizeOrigin, m_scrlOrigin;
  unsigned                m_fDrag:1;
  unsigned                m_fStartDrag:1;
  unsigned                m_fSized:1;
  unsigned                m_fDragMenuActive:1;
  unsigned                m_fMoved:1;
  unsigned                m_fJustCreated:1;
  sU32                    m_onButton;
  CRect                   m_baseRect;
  CRect                   m_oldRect;
  CSize                   m_origSize;
  UINT                    m_clipboardFormat;
  CBitmap                 m_buttonsBitmap;
  CRect                   m_windowRect;
  CRect                   m_selectRect;

	savedSelection*					m_originalSelection;

  sInt                    m_scrlX, m_scrlY;

	CGraphWindow();
	virtual ~CGraphWindow();
  
  class opData;
	class opButton;
  friend class opButton;

  opData*                 m_operators;

  class dragMenu;
  dragMenu*               m_dragMenu;

  BEGIN_MSG_MAP(CGraphWindow)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
    MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
    MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButtonDown)
    MESSAGE_HANDLER(WM_RBUTTONUP, OnRButtonUp)
    MESSAGE_HANDLER(WM_MBUTTONDOWN, OnMButtonDown)
    MESSAGE_HANDLER(WM_MBUTTONUP, OnMButtonUp)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_PARM_CHANGED, OnParmChanged)
    MESSAGE_HANDLER(WM_SETPAGE, OnSetPage)
    MESSAGE_HANDLER(WM_DELETECURPAGE, OnDeleteCurPage)
    MESSAGE_HANDLER(WM_SYNC_CALCULATIONS, OnSyncCalculations)
    MESSAGE_HANDLER(WM_CLEAR_GRAPH, OnClearGraph)
    MESSAGE_HANDLER(WM_FILE_LOADED, OnFileLoaded)
    MESSAGE_HANDLER(WM_UPDATE_OP_BUTTON, OnUpdateOpButton)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_SET_DISPLAY_MESH, OnSetDisplayMesh)
  ALT_MSG_MAP(1)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
    COMMAND_ID_HANDLER(ID_EDIT_COPY, OnEditCopy)
    COMMAND_ID_HANDLER(ID_EDIT_CUT, OnEditCut)
    COMMAND_ID_HANDLER(ID_EDIT_PASTE, OnEditPaste)
    COMMAND_ID_HANDLER(ID_GRAPH_SEARCHSTORE, OnSearchStore)
  END_MSG_MAP()

  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnRButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnParmChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSetPage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnDeleteCurPage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSyncCalculations(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnClearGraph(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnFileLoaded(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnUpdateOpButton(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSetDisplayMesh(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  
  LRESULT OnEditCopy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnEditCut(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnEditPaste(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnSearchStore(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  
  void GetScrollOffset(CPoint &pt) const;
  void SetScrollOffset(sInt scrlX, sInt scrlY);

  void PaintSelectRect(sBool remove = sFALSE);

  void ShowDragMenu();
  void KillDragMenu();
  
  void DoSearchStore();
  void CenterOnOperator(sU32 id);
};

#endif // !defined(AFX_GRAPHWINDOW_H__24A9687F_05CB_44BD_9275_31E1B447A624__INCLUDED_)

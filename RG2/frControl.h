// This code is in the public domain. See LICENSE for details.

// frControl.h: farbrausch controls
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FRCONTROL_H__6F409D6C_6C26_45C3_B273_C3235926D005__INCLUDED_)
#define AFX_FRCONTROL_H__6F409D6C_6C26_45C3_B273_C3235926D005__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdio.h>
#include "expression.h"
#include "ruleMouseInput.h"

#define FRBS_BORDERSTYLE      0x00000003
#define   FRBS_AUTO3D_DOUBLE    0x00000000
#define   FRBS_AUTO3D_SINGLE    0x00000001
#define   FRBS_FLAT             0x00000002
#define   FRBS_NOBORDER         0x00000003
#define FRBS_HOTTRACK         0x00000004
#define FRBS_BITMAP           0x00000008
#define FRBS_AUTOSIZE         0x00000010
#define FRBS_SOLID            0x00000080
#define FRBS_NOTIFY           0x00000100
#define FRBS_CHECK            0x00000200
#define FRBS_AUTO             0x00000400

#define FREBS_SHAREIMAGELISTS   0x00000001
#define FREBS_PARTIALCUSTOMDRAW 0x00000002

bool frRectsIntersect(const RECT& rc1, const RECT& rc2);

template <class T, class TBase=CWindow, class TWinTraits=CControlWinTraits>
class ATL_NO_VTABLE frModelessDialogImpl: public CWindowImpl<T, TBase, TWinTraits>,
  public CMessageFilter
{
public:
  DECLARE_WND_CLASS(NULL);

  BOOL PreTranslateMessage(MSG *pMsg)
  {
    return IsDialogMessage(pMsg);
  }

  BEGIN_MSG_MAP(frModelessDialogImpl)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
  END_MSG_MAP()

  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    CMessageLoop *pLoop=_Module.GetMessageLoop();
    pLoop->AddMessageFilter(this);

    bHandled=FALSE;
    return 0;
  }

  LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    CMessageLoop *pLoop=_Module.GetMessageLoop();
    pLoop->AddMessageFilter(this);

    bHandled=FALSE;
    return 0;
  }
};

class frSaveFocus
{
public:
  HWND  m_focusWindow;

  frSaveFocus()
  {
    m_focusWindow=::GetFocus();
  }

  ~frSaveFocus()
  {
    ::SetFocus(m_focusWindow);
  }
};

class frButtonImpl: public CWindowImpl<frButtonImpl, CWindow, CControlWinTraits>
{
public:
  DECLARE_WND_CLASS_EX(NULL, CS_DBLCLKS, 0);

  enum
  {
    _nImageNormal=0,
    _nImagePushed,
    _nImageFocusOrHover,
    _nImageDisabled,
    _nImageCount=4
  };

  CImageList    m_ImageList;
  int           m_nImage[_nImageCount];

  CFontHandle   m_font;

  DWORD         m_dwExtendedStyle;

  CToolTipCtrl  m_tip;
  LPTSTR        m_lpstrToolTipText;

  unsigned      m_fMouseOver:1;
  unsigned      m_fFocus:1;
  unsigned      m_fPressed:1;
  unsigned      m_fLeavePressed:1;
  unsigned      m_fChecked:1;

  COLORREF      m_colText, m_colHot, m_colFace, m_colShadow, m_colLight, m_colHilight, m_colDkShadow;
  bool          m_systemColors;
  int           m_smXEdge, m_smYEdge;
  bool          m_trackMouseLeave;

  frButtonImpl(HIMAGELIST hImageList=0);
  virtual ~frButtonImpl();

  HWND Create(HWND hWndParent, RECT& rcPos, LPCTSTR szWindowName = NULL,  DWORD dwStyle = 0, DWORD dwExStyle = 0,
    UINT nID = 0, LPVOID lpCreateParam = NULL);
  
  HIMAGELIST GetImageList() const
  {
    return m_ImageList;
  }

  HIMAGELIST SetImageList(HIMAGELIST hImageList);

  DWORD GetButtonExtendedStyle() const
  {
    return m_dwExtendedStyle;
  }

  DWORD SetButtonExtendedStyle(DWORD dwExtendedStyle)
  {
    DWORD sty=m_dwExtendedStyle;
    m_dwExtendedStyle=dwExtendedStyle;
    return sty;
  }

  int GetToolTipTextLength() const;
  bool GetToolTipText(LPTSTR lpstrText, int nLength) const;
  bool SetToolTipText(LPCTSTR lpstrText);
  
  void SetImages(int nNormal, int nPushed = -1, int nFocusOrHover = -1, int nDisabled = -1);
  BOOL SizeToImage();

  void SetColorScheme(COLORREF txt, COLORREF hot, COLORREF dkShadow, COLORREF shadow, COLORREF face, COLORREF light, COLORREF hilight);

  void PostNotify(DWORD code);
  
  virtual void DoPaint(CDCHandle dc);
  
  BEGIN_MSG_MAP(frButtonImpl)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_PRINTCLIENT, OnPaint)
    MESSAGE_HANDLER(WM_SETFOCUS, OnFocus)
    MESSAGE_HANDLER(WM_KILLFOCUS, OnFocus)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDblClk)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
    MESSAGE_HANDLER(WM_CAPTURECHANGED, OnCaptureChanged)
    MESSAGE_HANDLER(WM_ENABLE, OnEnable)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
    MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)
    MESSAGE_HANDLER(WM_SETFONT, OnSetFont)
    MESSAGE_HANDLER(WM_SYSCOLORCHANGE, OnSysColorChange)
    MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
    MESSAGE_HANDLER(BM_CLICK, OnClick)
    MESSAGE_HANDLER(BM_SETCHECK, OnSetCheck)
    MESSAGE_HANDLER(BM_GETCHECK, OnGetCheck)
  END_MSG_MAP()
    
  LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
  LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMouseMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnPaint(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnFocus(UINT uMsg, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
  LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
  LRESULT OnLButtonDblClk(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
  LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
  LRESULT OnCaptureChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
  LRESULT OnEnable(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
  LRESULT OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMouseLeave(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
  LRESULT OnKeyUp(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
  LRESULT OnSetFont(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSysColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSetCheck(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnGetCheck(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  void SetCheck(sInt state);

  int GetCheck()
  {
    return (IsCheck()?(m_fChecked?BST_CHECKED:BST_UNCHECKED):0);
  }

  // overrideables

  virtual void AdjustTextRect(LPRECT rect);
  virtual void DoPartialCustomDraw(CDCHandle dc, LPRECT textRect);
  virtual void DoClick(WPARAM wParam, LPARAM lParam);
  virtual void DoDoubleClick(WPARAM wParam, LPARAM lParam);

  // internal workings
  
  void Init();
  void Close();
  BOOL StartTrackMouseLeave();
  BOOL HitTest(LPARAM lParam, BOOL Client=TRUE);
  void UpdateColors();
  void UpdateSettings();
  
  bool IsHotTracking() const
  {
    return ((GetStyle() & FRBS_HOTTRACK)!=0);
  }

  bool IsBitmapButton() const
  {
    return ((GetStyle() & FRBS_BITMAP) != 0);
  }

  bool IsFlat() const
  {
    return ((GetStyle() & FRBS_BORDERSTYLE) == FRBS_FLAT);
  }

  bool IsSingle3D() const
  {
    return ((GetStyle() & FRBS_BORDERSTYLE) == FRBS_AUTO3D_SINGLE);
  }
  
  bool IsDouble3D() const
  {
    return ((GetStyle() & FRBS_BORDERSTYLE) == FRBS_AUTO3D_DOUBLE);
  }
  
  bool IsSolid() const
  {
    return ((GetStyle() & FRBS_SOLID) != 0);
  }

  bool IsPartialCustomDraw() const
  {
    return (m_dwExtendedStyle & FREBS_PARTIALCUSTOMDRAW)!=0;
  }

  bool IsCheck() const
  {
    return ((GetStyle() & FRBS_CHECK)!=0);
  }
};

class frButton: public frButtonImpl
{
public:
  DECLARE_WND_CLASS(_T("frButton"));

  frButton(HIMAGELIST hImageList = NULL) : 
		frButtonImpl(hImageList)
  {
  }
};

class frButtonMenu: public frButtonImpl
{
public:
  DECLARE_WND_CLASS_EX(_T("frButtonMenu"), CS_DBLCLKS, 0);

  frButtonMenu();
  ~frButtonMenu();

  BOOL AddMenuItem(UINT nMenuId, LPCTSTR strMenu, UINT nMenuFlags=MF_STRING);
  void AttachMenu(HMENU hMenu);
  BOOL DeleteMenuItem(UINT id, UINT nFlags=MF_BYCOMMAND);
  BOOL DeleteMenuItems();

  void AdjustTextRect(LPRECT rect);
  void DoPartialCustomDraw(CDCHandle dc, LPRECT textRect);
  void DoClick(WPARAM wParam, LPARAM lParam);

  BOOL DrawDownArrow(CDCHandle& dc, const RECT& rc, BOOL bDisabled, BOOL bHotTrack);

private:
  UINT _CalculateTrackMenuPosition(CPoint &pt);
  
  CMenu m_menu;
};

class frButtonChooser: public frButtonImpl, public CMessageFilter
{
public:
  DECLARE_WND_CLASS_EX(_T("frButtonChooser"), CS_DBLCLKS, 0);

  BEGIN_MSG_MAP(frButtonChooser)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(CB_GETCURSEL, OnGetSelection)
    MESSAGE_HANDLER(CB_SETCURSEL, OnSetSelection)
    CHAIN_MSG_MAP(frButtonImpl)
  END_MSG_MAP()

  frButtonChooser();
  ~frButtonChooser();

  BOOL PreTranslateMessage(MSG *pMsg);
  
  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnGetSelection(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSetSelection(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  
  BOOL AddMenuItem(UINT nMenuId, LPCTSTR strMenu, UINT nMenuFlags=MF_STRING);
  BOOL DeleteMenuItem(UINT id, UINT nFlags=MF_BYCOMMAND);
  BOOL DeleteMenuItems();

  UINT GetSelection() const;
  void SetSelection(UINT nMenuId, BOOL bNotify=FALSE);

  void AdjustTextRect(LPRECT rect);
  void DoPartialCustomDraw(CDCHandle dc, LPRECT textRect);
  void DoClick(WPARAM wParam, LPARAM lParam);

  BOOL DrawDownArrow(CDCHandle& dc, const RECT& rc, BOOL bDisabled, BOOL bHotTrack);

private:
  UINT _CalculateTrackMenuPosition(CPoint &pt);
  
  CMenu m_menu;
  UINT  m_selection;
};

#define FRNES_ALIGN           0x00000003
#define   FRNES_ALIGN_RIGHT   0x00000000
#define   FRNES_ALIGN_LEFT    0x00000001
#define   FRNES_ALIGN_CENTER  0x00000002
#define FRNES_NOKBDEDIT       0x00000004
#define FRNES_IMMEDIATEUPDATE 0x00000008
#define FRNES_ARROWKBDEDIT    0x00000010
#define FRNES_NOCLICKEDIT     0x00000020

#define FRNEN_BEGINDRAG       0x00000100
#define FRNEN_ENDDRAG         0x00000200

class frNumEditControl: public CWindowImpl<frNumEditControl, CWindow, CControlWinTraits>, public CMessageFilter
{
public:
  DECLARE_WND_CLASS_EX(_T("frNumEditControl"), CS_DBLCLKS, 0);

  typedef std::list<frNumEditControl *> updateList;
  typedef void (*arrowKeyHandler)(WPARAM key, LPVOID user);
  typedef sF64 (*translateValueHandler)(sF64 in, sBool inv, LPVOID user);
  
  CFontHandle m_font;

  sF64        m_value, m_min, m_max, m_stepSize;
  sInt        m_precision, m_accumDelta;
  sF64        m_valOrig, m_dragStartValue;
  sBool       m_clickTimedOut;

  CEdit       m_edit;

  CPoint      m_dragOrigin;

  unsigned    m_fPressed:1;
  unsigned    m_fHot:1;
  unsigned    m_fFocus:1;
  unsigned    m_fDrag:1;
  unsigned    m_fUseRuleMouse:1;
  updateList  m_updateGroup;

  arrowKeyHandler       m_arrowKeyHandler;
  LPVOID                m_arrowKeyHandlerParam;

  translateValueHandler m_translateValueHandler;
  LPVOID                m_translateValueHandlerParam;

  fr::expressionParser  *m_evaluator;

  BOOL PreTranslateMessage(MSG *pMsg);
  
  BEGIN_MSG_MAP(frNumEditControl)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_SETFONT, OnSetFont)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDown)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
    MESSAGE_HANDLER(WM_MBUTTONDOWN, OnMButtonDown)
    MESSAGE_HANDLER(WM_MBUTTONDBLCLK, OnMButtonDown)
    MESSAGE_HANDLER(WM_MBUTTONUP, OnMButtonUp)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
    MESSAGE_HANDLER(WM_COMMAND, OnCommand)
    MESSAGE_HANDLER(WM_CAPTURECHANGED, OnCaptureChanged)
    MESSAGE_HANDLER(WM_SETFOCUS, OnFocus)
    MESSAGE_HANDLER(WM_KILLFOCUS, OnFocus)
    MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
    MESSAGE_HANDLER(WM_CHAR, OnChar)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
    if (GetStyle() & FRNES_ARROWKBDEDIT)
    {
      MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
      MESSAGE_HANDLER(WM_SYSKEYDOWN, OnKeyDown)
    }
    MESSAGE_HANDLER(WM_RULEMOUSEINPUT, OnRuleMouseInput)
  END_MSG_MAP()

  frNumEditControl();

  void SetEvaluator(fr::expressionParser *eval)
  {
    m_evaluator=eval;
  }

  fr::expressionParser *GetEvaluator() const
  {
    return m_evaluator;
  }

  void SetValue(sF64 value);

  sF64 GetValue() const
  {
    return m_value;
  }

  void SetMin(sF64 min)
  {
    m_min=min;
  }

  void SetMax(sF64 max)
  {
    m_max=max;
  }

  void SetStep(sF64 step)
  {
    m_stepSize=step;
  }

  void SetPrecision(sInt precision);

  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSetFont(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnCaptureChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnGetDlgCode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnRuleMouseInput(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  void OnEditKeypress(WPARAM wParam);

  void Init();
  void UpdateValue(sF32 newValue, sF32 dragStart, sInt accumDelta, sBool updateAll);
  void BeginEdit();
  void SetEditPos(const CPoint &clickPoint, sBool clientCoords=sFALSE);
  void DoPaint(CDCHandle dc);

  void AddToUpdateGroup(frNumEditControl *ctrl);
  void RemoveFromUpdateGroup(frNumEditControl *ctrl);

  void SetArrowKeyHandler(arrowKeyHandler handler, LPVOID user);
  void SetTranslateValueHandler(translateValueHandler handler, LPVOID user);

private:
  const sChar *formatVal();
  void mouseDelta(WPARAM wParam, sInt deltaX);
};

#endif // !defined(AFX_FRCONTROL_H__6F409D6C_6C26_45C3_B273_C3235926D005__INCLUDED_)

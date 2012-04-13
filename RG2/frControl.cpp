// This code is in the public domain. See LICENSE for details.

#include "stdafx.h"
#include <stdio.h>
#include "expression.h"
#include "ruleMouseInput.h"

#ifndef OBM_DNARROWI
#define OBM_DNARROWI 32736
#endif

#ifndef OBM_DNARROW
#define OBM_DNARROW 32752
#endif

bool frRectsIntersect(const RECT &rc1, const RECT &rc2)
{
  if (rc1.left<rc2.left)
  {
    if (rc1.right<=rc2.left)
      return false;
  }
  else
  {
    if (rc2.right<=rc1.left)
      return false;
  }

  if (rc1.top<rc2.top)
  {
    if (rc1.bottom<=rc2.top)
      return false;
  }
  else
  {
    if (rc2.bottom<=rc1.top)
      return false;
  }

  return true;
}

// ---- frButtonImpl

frButtonImpl::frButtonImpl(HIMAGELIST hImageList)
  : m_ImageList(hImageList), m_lpstrToolTipText(0), m_dwExtendedStyle(0), m_fMouseOver(0), m_fFocus(0), m_fPressed(0), m_fLeavePressed(0), m_fChecked(0)
{
  m_nImage[_nImageNormal]=-1;
  m_nImage[_nImagePushed]=-1;
  m_nImage[_nImageFocusOrHover]=-1;
  m_nImage[_nImageDisabled]=-1;

  m_systemColors=true;
  m_trackMouseLeave=true;
}

frButtonImpl::~frButtonImpl()
{
  if (!(m_dwExtendedStyle & FREBS_SHAREIMAGELISTS))
    m_ImageList.Destroy();

  if (m_lpstrToolTipText)
  {
    delete[] m_lpstrToolTipText;
    m_lpstrToolTipText=0;
  }
}

HWND frButtonImpl::Create(HWND hWndParent, RECT& rcPos, LPCTSTR szWindowName, DWORD dwStyle, DWORD dwExStyle,
  UINT nID, LPVOID lpCreateParam)
{
  if (GetWndClassInfo().m_lpszOrigName == NULL)
    GetWndClassInfo().m_lpszOrigName = GetWndClassName();

  ATOM atom = GetWndClassInfo().Register(&m_pfnSuperWindowProc);
  
  dwStyle = GetWndStyle(dwStyle);
  dwExStyle = GetWndExStyle(dwExStyle);
  
  ATLASSERT(m_hWnd == NULL);
  
  if(atom == 0)
    return NULL;
  
  _Module.AddCreateWndData(&m_thunk.cd, this);
  
  HWND hWnd = ::CreateWindowEx(dwExStyle, (LPCTSTR)MAKELONG(atom, 0), szWindowName,
    dwStyle, rcPos.left, rcPos.top, rcPos.right - rcPos.left,
    rcPos.bottom - rcPos.top, hWndParent, (HMENU)nID,
    _Module.GetModuleInstance(), lpCreateParam);
  
  ATLASSERT(m_hWnd == hWnd);
  
  return hWnd;
}

HIMAGELIST frButtonImpl::SetImageList(HIMAGELIST hImageList)
{
  HIMAGELIST hImageListPrev=m_ImageList;
  m_ImageList=hImageList;
  if ((GetStyle() & FRBS_AUTOSIZE) && ::IsWindow(m_hWnd))
    SizeToImage();
  return hImageListPrev;
}

int frButtonImpl::GetToolTipTextLength() const
{
  return (!m_lpstrToolTipText) ? -1 : lstrlen(m_lpstrToolTipText);
}

bool frButtonImpl::GetToolTipText(LPTSTR lpstrText, int nLength) const
{
  ATLASSERT(lpstrText != NULL);
  if(m_lpstrToolTipText == NULL)
    return false;
  return (lstrcpyn(lpstrText, m_lpstrToolTipText, min(nLength, lstrlen(m_lpstrToolTipText) + 1)) != NULL);
}

bool frButtonImpl::SetToolTipText(LPCTSTR lpstrText)
{
  if(m_lpstrToolTipText != NULL)
  {
    delete [] m_lpstrToolTipText;
    m_lpstrToolTipText = NULL;
  }
  if(lpstrText == NULL)
  {
    if(m_tip.IsWindow())
      m_tip.Activate(FALSE);
    return true;
  }
  ATLTRY(m_lpstrToolTipText = new TCHAR[lstrlen(lpstrText) + 1]);
  if(m_lpstrToolTipText == NULL)
    return false;
  bool bRet = (lstrcpy(m_lpstrToolTipText, lpstrText) != NULL);
  if(bRet && m_tip.IsWindow())
  {
    m_tip.Activate(TRUE);
    m_tip.AddTool(m_hWnd, m_lpstrToolTipText);
  }
  return bRet;
}

void frButtonImpl::SetImages(int nNormal, int nPushed, int nFocusOrHover, int nDisabled)
{
  if(nNormal != -1)
    m_nImage[_nImageNormal] = nNormal;
  if(nPushed != -1)
    m_nImage[_nImagePushed] = nPushed;
  if(nFocusOrHover != -1)
    m_nImage[_nImageFocusOrHover] = nFocusOrHover;
  if(nDisabled != -1)
    m_nImage[_nImageDisabled] = nDisabled;
}

BOOL frButtonImpl::SizeToImage()
{
  ATLASSERT(::IsWindow(m_hWnd) && m_ImageList.m_hImageList != NULL);
  int cx = 0;
  int cy = 0;
  if(!m_ImageList.GetIconSize(cx, cy))
    return FALSE;

  return ResizeClient(cx, cy);
}

void frButtonImpl::SetColorScheme(COLORREF txt, COLORREF hot, COLORREF dkShadow, COLORREF shadow, COLORREF face, COLORREF light, COLORREF hilight)
{
  m_colText=txt;
  m_colHot=hot;
  m_colDkShadow=dkShadow;
  m_colShadow=shadow;
  m_colFace=face;
  m_colLight=light;
  m_colHilight=hilight;
  m_systemColors=false;
}

void frButtonImpl::PostNotify(DWORD code)
{
  DWORD id=GetDlgCtrlID();

  ::PostMessage(GetParent(), WM_COMMAND, (WPARAM) MAKELONG(id, code), (LPARAM) m_hWnd);
}

void frButtonImpl::DoPaint(CDCHandle dc)
{
  if (IsBitmapButton())
  {
    ATLASSERT(m_ImageList.m_hImageList != NULL);
    ATLASSERT(m_nImage[0] != -1);
  }
  
  int nImage = -1;
  bool bHover = IsHotTracking();
  if(m_fPressed == 1)
    nImage = m_nImage[_nImagePushed];
  else if((!bHover && m_fFocus == 1) || (bHover && (m_fMouseOver==1 || m_fFocus==1)))
    nImage = m_nImage[_nImageFocusOrHover];
  else if(!IsWindowEnabled())
    nImage = m_nImage[_nImageDisabled];
  if(nImage == -1)
    nImage = m_nImage[_nImageNormal];

  CRect clientRect, textRect;

  GetClientRect(&clientRect);
  textRect=clientRect;

  if (IsSingle3D() || IsDouble3D())
  {
    dc.FillSolidRect(&textRect, m_colFace);
    textRect.DeflateRect(1+!!IsDouble3D(), 1+!!IsDouble3D());
  }
  else if (IsFlat())
    textRect.DeflateRect(2, 2);

  if (m_dwExtendedStyle & FREBS_PARTIALCUSTOMDRAW)
    AdjustTextRect(&textRect);

  int xyPos = 0;

  if ((m_fPressed == 1) && (IsSingle3D() || IsDouble3D()))
  {
    if (!IsBitmapButton())
      xyPos=1;
    else if (m_nImage[_nImagePushed]==-1)
      xyPos=1;
  }
  else if ((m_fPressed==1) && IsFlat())
    xyPos=1;

  if (IsBitmapButton())
    m_ImageList.Draw(dc, nImage, xyPos+textRect.left, xyPos+textRect.top, ILD_NORMAL);
  else
  {
    int len=GetWindowTextLength();
    TCHAR *txt=new TCHAR[len+1];
    GetWindowText(txt, len+1);

    CSize sz;

    if ((!bHover || m_fMouseOver==0) && (m_fFocus==0))
      dc.SetTextColor(m_colText);
    else
      dc.SetTextColor(m_colHot);

    CFontHandle oldFont=dc.SelectFont(m_font);
    dc.GetTextExtent(txt, len, &sz);
    dc.SetBkMode(TRANSPARENT);
    dc.FillSolidRect(&textRect, m_colFace);

    dc.GetTextExtent(txt, len, &sz);
    dc.DrawState(CPoint((textRect.left+textRect.right-sz.cx)/2+xyPos, (textRect.top+textRect.bottom-sz.cy)/2+xyPos),
      sz, txt, IsWindowEnabled()?DSS_NORMAL:DSS_DISABLED, TRUE, len);

    dc.SelectFont(oldFont);

    delete[] txt;
  }
  
  if ((GetStyle() & FRBS_BORDERSTYLE) != FRBS_NOBORDER)
  {
    COLORREF col1, col2, col3, col4;

    if (IsFlat())
    {
      col1=m_colDkShadow;
      col2=m_colDkShadow;
      col3=m_colHilight;
      col4=m_colHilight;

      if (bHover && m_fMouseOver==1)
      {
        col3=m_colHilight;
        col4=m_colShadow;
      }
    }
    else if (IsSingle3D() || IsDouble3D())
    {
      col1=m_colFace;
      col2=m_colFace;
      col3=m_colFace;
      col4=m_colFace;

      if (!bHover || m_fMouseOver==1 || IsSolid() || IsDouble3D())
      {
        col1=m_colHilight;
        col2=m_colDkShadow;

        if (IsDouble3D())
        {
          col3=m_colLight;
          col4=m_colShadow;
        }
      }

      if (bHover && m_fMouseOver==0 && IsDouble3D())
      {
        col3=m_colFace;
        col4=m_colFace;
      }
    }

    sBool down=IsCheck()?m_fChecked:m_fPressed;

    if (down)
    {
      COLORREF t;
      t=col1; col1=col2; col2=t;
      t=col3; col3=col4; col4=t;
    }

    dc.Draw3dRect(clientRect.left, clientRect.top, clientRect.right-clientRect.left, clientRect.bottom-clientRect.top, col1, col2);
    dc.Draw3dRect(clientRect.left+1, clientRect.top+1, clientRect.right-clientRect.left-2, clientRect.bottom-clientRect.top-2, col3, col4);
    
    if (!bHover && m_fFocus == 1)
    {
      CRect rect=textRect;

      rect.DeflateRect(m_smXEdge, m_smYEdge);
      dc.DrawFocusRect(&rect);
    }
  }

  if (m_dwExtendedStyle & FREBS_PARTIALCUSTOMDRAW)
    DoPartialCustomDraw(dc, &textRect);
}

LRESULT frButtonImpl::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
  Init();

  bHandled = FALSE;
  return 0;
}

LRESULT frButtonImpl::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  Close();

  bHandled=FALSE;
  return 0;
}

LRESULT frButtonImpl::OnMouseMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  MSG msg = { m_hWnd, uMsg, wParam, lParam };
  if(m_tip.IsWindow())
    m_tip.RelayEvent(&msg);
  bHandled = FALSE;
  return 1;
}

LRESULT frButtonImpl::OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  return 1;
}

LRESULT frButtonImpl::OnPaint(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  if(wParam != NULL)
    DoPaint((HDC)wParam);
  else
  {
    CPaintDC dc(m_hWnd);
    DoPaint(dc.m_hDC);
  }
  return 0;
}

LRESULT frButtonImpl::OnFocus(UINT uMsg, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
  m_fFocus = (uMsg == WM_SETFOCUS) ? 1 : 0;

  if (GetStyle() & FRBS_NOTIFY)
    PostNotify(m_fFocus?BN_SETFOCUS:BN_KILLFOCUS);

  Invalidate();
  UpdateWindow();
  bHandled = FALSE;
  return 1;
}

LRESULT frButtonImpl::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
  if (!HitTest(lParam))
    return 0;

  if (::GetCapture() != m_hWnd)
    SetCapture();

  ::SetFocus(m_hWnd);

  if ((GetStyle() & (FRBS_CHECK|FRBS_AUTO)) == (FRBS_CHECK|FRBS_AUTO))
    m_fChecked^=1;
  
  if (m_fPressed==0)
  {
    m_fPressed = 1;
    Invalidate();
    UpdateWindow();
  }

  DoClick(wParam, lParam);

  return 0;
}

LRESULT frButtonImpl::OnLButtonDblClk(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
  if (!HitTest(lParam))
    return 0;

  if(::GetCapture() != m_hWnd)
    SetCapture();

  ::SetFocus(m_hWnd);
  
  if ((GetStyle() & (FRBS_CHECK|FRBS_AUTO)) == (FRBS_CHECK|FRBS_AUTO))
    m_fChecked^=1;

  if(m_fPressed == 0)
  {
    m_fPressed = 1;
    Invalidate();
    UpdateWindow();
  }

  DoDoubleClick(wParam, lParam);
  
  return 0;
}

LRESULT frButtonImpl::OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
  if (m_fPressed!=0)
  {
    m_fPressed=0;
    m_fLeavePressed=0;
    Invalidate();
    UpdateWindow();
  }

  if(::GetCapture() == m_hWnd)
    ::ReleaseCapture();
  
  return 0;
}

LRESULT frButtonImpl::OnCaptureChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
  if (m_fPressed==1)
  {
    m_fPressed=0;
    m_fLeavePressed=0;
    Invalidate();
    UpdateWindow();
  }

  bHandled=FALSE;
  return 1;
}

LRESULT frButtonImpl::OnEnable(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
  Invalidate();
  UpdateWindow();
  bHandled=FALSE;
  return 1;
}

LRESULT frButtonImpl::OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
  if (::GetCapture()==m_hWnd)
  {
    if (m_trackMouseLeave)
    {
      unsigned int uPressed=!!HitTest(lParam);

      if (!uPressed)
      {
        m_fLeavePressed=m_fPressed;
        ::ReleaseCapture();
      }

      if (m_fPressed!=uPressed)
      {
        m_fPressed=uPressed;
        Invalidate();
        UpdateWindow();
      }
    }
  }
  else if (IsHotTracking() && m_fMouseOver==0)
  {
    m_fMouseOver=1;
    m_fPressed=m_fLeavePressed;
    Invalidate();
    UpdateWindow();
    StartTrackMouseLeave();
  }

  bHandled=FALSE;
  return 1;
}

LRESULT frButtonImpl::OnMouseLeave(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  if (m_fMouseOver==1)
  {
    m_fMouseOver=0;
    Invalidate();
    UpdateWindow();
  }

  return 0;
}

LRESULT frButtonImpl::OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
  if (wParam==VK_SPACE && IsHotTracking())
    return 0;
  
  if (wParam==VK_SPACE && m_fPressed==0)
  {
    m_fPressed=1;
    Invalidate();
    UpdateWindow();
  }

  bHandled=FALSE;
  return 1;
}

LRESULT frButtonImpl::OnKeyUp(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
  if (wParam==VK_SPACE && IsHotTracking())
    return 0;

  if (wParam==VK_SPACE && m_fPressed==1)
  {
    m_fPressed=0;
    Invalidate();
    UpdateWindow();
  }

  bHandled=FALSE;
  return 1;
}

LRESULT frButtonImpl::OnSetFont(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_font=(HFONT) wParam;

  if (LOWORD(lParam)==TRUE)
  {
    Invalidate();
    UpdateWindow();
  }

  return 0;
}

LRESULT frButtonImpl::OnSysColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (m_systemColors)
    UpdateColors();

  bHandled=FALSE;
  return 0;
}

LRESULT frButtonImpl::OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  UpdateSettings();
  
  bHandled=FALSE;
  return 0;
}

LRESULT frButtonImpl::OnClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  POINT pt;

  GetCursorPos(&pt);
  ScreenToClient(&pt);
  DoClick(0, MAKELONG(pt.x, pt.y));

  return 0;
}

LRESULT frButtonImpl::OnSetCheck(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  SetCheck(wParam);

  return 0;
}

LRESULT frButtonImpl::OnGetCheck(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  return GetCheck();
}

void frButtonImpl::SetCheck(sInt state)
{
  sBool checkState=(state!=BST_UNCHECKED);
  
  if (m_fChecked!=checkState)
  {
    m_fChecked=checkState;
    Invalidate();
    UpdateWindow();
  }
}

void frButtonImpl::AdjustTextRect(LPRECT rect)
{
  ATLASSERT(FALSE);
}

void frButtonImpl::DoPartialCustomDraw(CDCHandle dc, LPRECT textRect)
{
  ATLASSERT(FALSE);
}

void frButtonImpl::DoClick(WPARAM wParam, LPARAM lParam)
{
  PostNotify(BN_CLICKED);
}

void frButtonImpl::DoDoubleClick(WPARAM wParam, LPARAM lParam)
{
  PostNotify(BN_DOUBLECLICKED);
}

void frButtonImpl::Init()
{
  m_tip.Create(m_hWnd);
  ATLASSERT(m_tip.IsWindow());
  if(m_tip.IsWindow() && m_lpstrToolTipText != NULL)
  {
    m_tip.Activate(TRUE);
    m_tip.AddTool(m_hWnd, m_lpstrToolTipText);
  }
  
  if(m_ImageList.m_hImageList != NULL && (GetStyle() & FRBS_AUTOSIZE) != 0)
    SizeToImage();

  UpdateColors();
  UpdateSettings();
}

void frButtonImpl::Close()
{
  if (::IsWindow(m_tip))
    m_tip.DestroyWindow();
}

BOOL frButtonImpl::StartTrackMouseLeave()
{
  TRACKMOUSEEVENT tme;
  tme.cbSize = sizeof(tme);
  tme.dwFlags = TME_LEAVE;
  tme.hwndTrack = m_hWnd;
  return _TrackMouseEvent(&tme);
}

BOOL frButtonImpl::HitTest(LPARAM lParam, BOOL Client)
{
  POINT pt={GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
  RECT rect;
  
  if (Client)
    GetClientRect(&rect);
  else
    GetWindowRect(&rect);
  
  return ::PtInRect(&rect, pt);
}

void frButtonImpl::UpdateColors()
{
  m_colText=::GetSysColor(COLOR_BTNTEXT);
  m_colHot=::GetSysColor(COLOR_HOTLIGHT);
  m_colFace=::GetSysColor(COLOR_3DFACE);
  m_colShadow=::GetSysColor(COLOR_3DSHADOW);
  m_colLight=::GetSysColor(COLOR_3DLIGHT);
  m_colHilight=::GetSysColor(COLOR_3DHILIGHT);
  m_colDkShadow=::GetSysColor(COLOR_3DDKSHADOW);
}

void frButtonImpl::UpdateSettings()
{
  m_smXEdge=::GetSystemMetrics(SM_CXEDGE);
  m_smYEdge=::GetSystemMetrics(SM_CYEDGE);
}

// ---- frButtonMenu

frButtonMenu::frButtonMenu()
{
  SetButtonExtendedStyle(FREBS_PARTIALCUSTOMDRAW);
  m_menu.CreatePopupMenu();
}

frButtonMenu::~frButtonMenu()
{
  m_menu.DestroyMenu();
}

BOOL frButtonMenu::AddMenuItem(UINT nMenuId, LPCTSTR strMenu, UINT nMenuFlags)
{
  return m_menu.AppendMenu(nMenuFlags | MF_STRING, nMenuId, strMenu);
}

void frButtonMenu::AttachMenu(HMENU hMenu)
{
  m_menu.DestroyMenu();
  m_menu.Attach(hMenu);
}

BOOL frButtonMenu::DeleteMenuItem(UINT id, UINT nFlags)
{
  return m_menu.DeleteMenu(id, nFlags);
}

BOOL frButtonMenu::DeleteMenuItems()
{
  m_menu.DestroyMenu();
  return m_menu.CreatePopupMenu();
}

void frButtonMenu::AdjustTextRect(LPRECT rect)
{
  ATLASSERT(::IsWindow(m_hWnd));

  rect->right-=::GetSystemMetrics(SM_CXVSCROLL)-2*m_smXEdge;
}

void frButtonMenu::DoPartialCustomDraw(CDCHandle dc, LPRECT textRect)
{
  textRect->left=textRect->right;
  textRect->right=textRect->left+::GetSystemMetrics(SM_CXVSCROLL)-2*m_smYEdge;

  dc.FillSolidRect(textRect, m_colFace);
  DrawDownArrow(dc, *textRect, !IsWindowEnabled(), IsHotTracking() && m_fMouseOver);
}

void frButtonMenu::DoClick(WPARAM wParam, LPARAM lParam)
{
  CPoint pt;
  UINT Flags=0;
  Flags=_CalculateTrackMenuPosition(pt);
  m_menu.TrackPopupMenu(Flags|TPM_LEFTBUTTON, pt.x, pt.y, GetParent());
}

BOOL frButtonMenu::DrawDownArrow(CDCHandle& dc, const RECT& rc, BOOL bDisabled, BOOL bHotTrack)
{
  // center arrow
  SIZE size = { 8, 4 };
  RECT rcDest = rc;
  POINT p = { 0, 0 };
  SIZE szDelta = { (rc.right - rc.left - size.cx) / 2, (rc.bottom - rc.top - size.cy) / 2 };
  if(rc.right - rc.left > size.cx || rc.bottom-rc.top>size.cy)
  {
    rcDest.left = rc.left + szDelta.cx;
    rcDest.top = rc.top + szDelta.cy;
    rcDest.right = rcDest.left + size.cx;
    rcDest.bottom = rcDest.top + size.cy;
  }
  
  dc.FillSolidRect(&rcDest, m_colFace);
  
  CPen pen;
  pen.CreatePen(PS_SOLID, 1, bDisabled?m_colShadow:bHotTrack?m_colHot:m_colText);
  
  CPenHandle oldPen=dc.SelectPen(pen);
  
  dc.MoveTo(rcDest.left+3, rcDest.top+3);
  dc.LineTo(rcDest.left+2, rcDest.top+2);
  dc.LineTo(rcDest.left+4, rcDest.top+2);
  dc.LineTo(rcDest.left+5, rcDest.top+1);
  dc.LineTo(rcDest.left+1, rcDest.top+1);
  dc.LineTo(rcDest.left+0, rcDest.top+0);
  dc.LineTo(rcDest.left+7, rcDest.top+0);
  
  if (bDisabled)
  {
    dc.SelectPen(oldPen);
    pen.DeleteObject();
    pen.CreatePen(PS_SOLID, 1, m_colHilight);
    oldPen=dc.SelectPen(pen);
    dc.LineTo(rcDest.left+3, rcDest.top+4);
  }
  
  dc.SelectPen(oldPen);
  
  return TRUE;
}

UINT frButtonMenu::_CalculateTrackMenuPosition(CPoint &pt)
{
  CRect rect;
  GetWindowRect(&rect);
  pt.x=rect.left<0?0:rect.left;
  pt.y=rect.bottom;
  return TPM_LEFTALIGN;
}

// ---- frButtonChooser

frButtonChooser::frButtonChooser()
{
  SetButtonExtendedStyle(FREBS_PARTIALCUSTOMDRAW);
  m_menu.CreatePopupMenu();
}

frButtonChooser::~frButtonChooser()
{
  m_menu.DestroyMenu();
}

BOOL frButtonChooser::PreTranslateMessage(MSG *pMsg)
{
  if (pMsg->message==WM_KEYDOWN && pMsg->wParam==VK_RETURN && !(pMsg->lParam & (1<<30)) && pMsg->hwnd==m_hWnd)
  {
    DoClick(0, 0);
    return TRUE;
  }

  return FALSE;
}

LRESULT frButtonChooser::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CMessageLoop *pLoop=_Module.GetMessageLoop();
  pLoop->AddMessageFilter(this);

  bHandled=FALSE;
  return 0;
}

LRESULT frButtonChooser::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CMessageLoop *pLoop=_Module.GetMessageLoop();
  pLoop->RemoveMessageFilter(this);

  bHandled=FALSE;
  return 0;
}

LRESULT frButtonChooser::OnGetSelection(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  return GetSelection();
}

LRESULT frButtonChooser::OnSetSelection(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  SetSelection(wParam);

  return 0;
}

BOOL frButtonChooser::AddMenuItem(UINT nMenuId, LPCTSTR strMenu, UINT nMenuFlags)
{
  BOOL res=m_menu.AppendMenu(nMenuFlags | MF_STRING, nMenuId, strMenu);
  SetSelection(nMenuId);
  return res;
}

BOOL frButtonChooser::DeleteMenuItem(UINT id, UINT nFlags)
{
  return m_menu.DeleteMenu(id, nFlags);
}

BOOL frButtonChooser::DeleteMenuItems()
{
  m_menu.DestroyMenu();
  return m_menu.CreatePopupMenu();
}

UINT frButtonChooser::GetSelection() const
{
  return m_selection;
}

void frButtonChooser::SetSelection(UINT nMenuId, BOOL bNotify)
{
  MENUITEMINFO  inf;
  
  inf.cbSize=sizeof(inf);
  inf.fMask=MIIM_STRING;
  inf.dwTypeData=0;
  m_menu.GetMenuItemInfo(nMenuId, FALSE, &inf);
  
  inf.cch++;
  inf.dwTypeData=new TCHAR[inf.cch];
  m_menu.GetMenuItemInfo(nMenuId, FALSE, &inf);
  
  m_selection=nMenuId;

  SetWindowText(inf.dwTypeData);
  delete[] inf.dwTypeData;
  
  Invalidate();
  UpdateWindow();

  if (bNotify)
    SendMessage(GetParent(), WM_COMMAND, MAKELONG(GetDlgCtrlID(), CBN_SELCHANGE), (LPARAM) m_hWnd);
}

void frButtonChooser::AdjustTextRect(LPRECT rect)
{
  ATLASSERT(::IsWindow(m_hWnd));

  rect->right-=::GetSystemMetrics(SM_CXVSCROLL)-2*m_smXEdge;
}

void frButtonChooser::DoPartialCustomDraw(CDCHandle dc, LPRECT textRect)
{
  textRect->left=textRect->right;
  textRect->right=textRect->left+::GetSystemMetrics(SM_CXVSCROLL)-2*m_smXEdge;

  dc.FillSolidRect(textRect, m_colFace);
  DrawDownArrow(dc, *textRect, !IsWindowEnabled(), IsHotTracking() && m_fMouseOver);
}

void frButtonChooser::DoClick(WPARAM wParam, LPARAM lParam)
{
  CPoint pt;
  UINT Flags=0;
  Flags=_CalculateTrackMenuPosition(pt);
  UINT sel=m_menu.TrackPopupMenu(Flags|TPM_LEFTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD, pt.x, pt.y, m_hWnd);

  if (sel)
    SetSelection(sel, TRUE);
}

BOOL frButtonChooser::DrawDownArrow(CDCHandle& dc, const RECT& rc, BOOL bDisabled, BOOL bHotTrack)
{
  // center arrow
  SIZE size = { 8, 4 };
  RECT rcDest = rc;
  POINT p = { 0, 0 };
  SIZE szDelta = { (rc.right - rc.left - size.cx) / 2, (rc.bottom - rc.top - size.cy) / 2 };
  if(rc.right - rc.left > size.cx || rc.bottom-rc.top>size.cy)
  {
    rcDest.left = rc.left + szDelta.cx;
    rcDest.top = rc.top + szDelta.cy;
    rcDest.right = rcDest.left + size.cx;
    rcDest.bottom = rcDest.top + size.cy;
  }

  dc.FillSolidRect(&rcDest, m_colFace);

  CPen pen;
  pen.CreatePen(PS_SOLID, 1, bDisabled?m_colShadow:bHotTrack?m_colHot:m_colText);
  
  CPenHandle oldPen=dc.SelectPen(pen);

  dc.MoveTo(rcDest.left+3, rcDest.top+3);
  dc.LineTo(rcDest.left+2, rcDest.top+2);
  dc.LineTo(rcDest.left+4, rcDest.top+2);
  dc.LineTo(rcDest.left+5, rcDest.top+1);
  dc.LineTo(rcDest.left+1, rcDest.top+1);
  dc.LineTo(rcDest.left+0, rcDest.top+0);
  dc.LineTo(rcDest.left+7, rcDest.top+0);

  if (bDisabled)
  {
    dc.SelectPen(oldPen);
    pen.DeleteObject();
    pen.CreatePen(PS_SOLID, 1, m_colHilight);
    oldPen=dc.SelectPen(pen);
    dc.LineTo(rcDest.left+3, rcDest.top+4);
  }

  dc.SelectPen(oldPen);
  
  return TRUE;
}

UINT frButtonChooser::_CalculateTrackMenuPosition(CPoint &pt)
{
  CRect rect;
  GetWindowRect(&rect);
  pt.x=rect.right;
  pt.y=rect.bottom;
  return TPM_RIGHTALIGN;
}

// ---- frNumEditControl

BOOL frNumEditControl::PreTranslateMessage(MSG *pMsg)
{
  if (pMsg->message==WM_KEYDOWN && pMsg->hwnd==m_edit)
  {
    if (pMsg->wParam==VK_RETURN || pMsg->wParam==VK_ESCAPE)
    {
      OnEditKeypress(pMsg->wParam);
      return TRUE;
    }
    else
      return FALSE;
  }
  else
    return FALSE;
}

frNumEditControl::frNumEditControl() : m_fPressed(0), m_fHot(0), m_fFocus(0), m_fDrag(0), m_evaluator(0), m_fUseRuleMouse(0)
{
}

void frNumEditControl::SetValue(sF64 value)
{
  m_value=value;
  m_dragStartValue=m_value;
  m_accumDelta=0;

  if (::IsWindow(m_hWnd))
  {
    Invalidate();
    UpdateWindow();
  }
}

void frNumEditControl::SetPrecision(sInt precision)
{
  m_precision=precision;
  
  if (::IsWindow(m_hWnd))
  {
    Invalidate();
    UpdateWindow();
  }
}

LRESULT frNumEditControl::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  Init();

  CMessageLoop *pLoop=_Module.GetMessageLoop();
  pLoop->AddMessageFilter(this);

  m_updateGroup.clear();
  m_arrowKeyHandler=0;
  m_arrowKeyHandlerParam=0;
  m_translateValueHandler=0;
  m_translateValueHandlerParam=0;

  m_dragStartValue=0;
  m_accumDelta=0;

  bHandled=FALSE;
  return 0;
}

LRESULT frNumEditControl::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CMessageLoop *pLoop=_Module.GetMessageLoop();
  pLoop->RemoveMessageFilter(this);

  if (::IsWindow(m_edit.m_hWnd))
    m_edit.DestroyWindow();
  
  bHandled=FALSE;
  return 0;
}

LRESULT frNumEditControl::OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  return 1;
}

LRESULT frNumEditControl::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (wParam)
    DoPaint((HDC) wParam);
  else
  {
    CPaintDC dc(m_hWnd);
    DoPaint(dc.m_hDC);
  }

  return 0;
}

LRESULT frNumEditControl::OnSetFont(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_font=wParam?(HFONT) wParam:AtlGetStockFont(DEFAULT_GUI_FONT);

  if (HIWORD(lParam)==TRUE)
  {
    Invalidate();
    UpdateWindow();
  }

  return 0;
}

LRESULT frNumEditControl::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (::GetCapture()!=m_hWnd)
  {
    SetCapture();
    m_fUseRuleMouse=ruleMouseSetTarget(m_hWnd);
  }

  SetFocus();

  if (m_fPressed==0)
  {
    m_dragOrigin.x=GET_X_LPARAM(lParam);
    m_dragOrigin.y=GET_X_LPARAM(lParam);
    m_fPressed=1;
    m_fDrag=0;
    m_valOrig=m_value;
  }

  m_clickTimedOut=sFALSE;
  SetTimer(1, 200);

  return 0;
}

LRESULT frNumEditControl::OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (::GetCapture()==m_hWnd)
  {
    ReleaseCapture();
    if (m_fUseRuleMouse && (HWND) ruleMouseGetTarget==m_hWnd)
      ruleMouseSetTarget(0);
  }

  if (m_fDrag)
    ::PostMessage(GetParent(), WM_COMMAND, MAKELONG(GetDlgCtrlID(), FRNEN_ENDDRAG), 0);
    
  if (m_value!=m_valOrig)
    ::PostMessage(GetParent(), WM_COMMAND, MAKELONG(GetDlgCtrlID(), EN_CHANGE), 0);
  else if (!m_clickTimedOut && !(GetStyle() & FRNES_NOCLICKEDIT))
  {
    BeginEdit();
    SetEditPos(CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), sTRUE);
  }

  m_fPressed=0;

  return 0;
}

LRESULT frNumEditControl::OnMButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (::GetCapture()!=m_hWnd)
  {
    SetCapture();
    m_fUseRuleMouse=ruleMouseSetTarget(m_hWnd);
  }
  
  SetFocus();
  
  if (m_fPressed==0)
  {
    m_dragOrigin.x=GET_X_LPARAM(lParam);
    m_dragOrigin.y=GET_X_LPARAM(lParam);
    m_fPressed=1;
    m_fDrag=0;
    m_valOrig=m_value;
  }
  
  return 0;
}

LRESULT frNumEditControl::OnMButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (::GetCapture()==m_hWnd)
  {
    ReleaseCapture();
    if (m_fUseRuleMouse && (HWND) ruleMouseGetTarget()==m_hWnd)
      ruleMouseSetTarget(0);
  }
  
  if (m_fDrag)
    ::PostMessage(GetParent(), WM_COMMAND, MAKELONG(GetDlgCtrlID(), FRNEN_ENDDRAG), 0);
  
  if (m_value!=m_valOrig)
    ::PostMessage(GetParent(), WM_COMMAND, MAKELONG(GetDlgCtrlID(), EN_CHANGE), 0);

  return 0;
}

LRESULT frNumEditControl::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CPoint ptCursor(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
  
  if (m_fPressed)
  {
    if (!m_fDrag)
    {
      m_fDrag=1;
      ::PostMessage(GetParent(), WM_COMMAND, MAKELONG(GetDlgCtrlID(), FRNEN_BEGINDRAG), 0);
    }

    if (!m_fUseRuleMouse)
      mouseDelta(wParam, ptCursor.x-m_dragOrigin.x);

    m_dragOrigin=ptCursor;
  }
  else if (::GetCapture()==m_hWnd)
  {
    RECT rc;
    GetClientRect(&rc);

    if (!PtInRect(&rc, ptCursor))
    {
      m_fHot=0;
      ReleaseCapture();
      if (m_fUseRuleMouse && (HWND) ruleMouseGetTarget()==m_hWnd)
        ruleMouseSetTarget(0);

      Invalidate();
      UpdateWindow();
    }
  }
  else
  {
    SetCapture();
    m_fUseRuleMouse=ruleMouseSetTarget(m_hWnd);
    m_fHot=1;

    Invalidate();
    UpdateWindow();
  }
  
  return 0;
}

LRESULT frNumEditControl::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CPoint pnt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
  RECT rc;

  GetWindowRect(&rc);

  if (pnt.x>=rc.left && pnt.y>=rc.top && pnt.x<rc.right && pnt.y<rc.bottom)
  {
    BeginEdit();
    SetEditPos(pnt);
  }

  return 0;
}

LRESULT frNumEditControl::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (HIWORD(wParam)==EN_KILLFOCUS && LOWORD(wParam)==100)
    OnEditKeypress(VK_ESCAPE);

  return 0;
}

LRESULT frNumEditControl::OnCaptureChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (m_fPressed==1 || m_fHot==1)
  {
    m_fPressed=0;
    m_fHot=0;
    Invalidate();
    UpdateWindow();
  }
  
  bHandled=FALSE;
  return 1;
}

LRESULT frNumEditControl::OnFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  UINT uFocus=uMsg==WM_SETFOCUS;

  if (m_fFocus!=uFocus)
  {
    m_fFocus=uFocus;

    Invalidate();
    UpdateWindow();
  }

  bHandled=FALSE;
  return 0;
}

LRESULT frNumEditControl::OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (wParam!=13 && wParam!=27) // not return or escape
  {
    BeginEdit();

    m_edit.SetWindowText(_T(""));
    ::PostMessage(m_edit, uMsg, wParam, lParam);
  }
  else if (wParam!=27) // not escape
  {
    BeginEdit();

    m_edit.SetSelAll();
  }
    
  bHandled=FALSE;
  return 0;
}

LRESULT frNumEditControl::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_clickTimedOut=sTRUE;
  KillTimer(1);

  return 0;
}

LRESULT frNumEditControl::OnGetDlgCode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  return DLGC_WANTALLKEYS|((GetStyle() & FRNES_ARROWKBDEDIT) ? DLGC_WANTARROWS : 0);
}

LRESULT frNumEditControl::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  GetAsyncKeyState(VK_MENU); // vk_menu wird mit einer abfrage delayed geupdated

  if (uMsg==WM_SYSKEYDOWN && !GetAsyncKeyState(VK_MENU))
  {
    bHandled=FALSE;
    return 1;
  }

  if (wParam==VK_LEFT || wParam==VK_RIGHT || wParam==VK_UP || wParam==VK_DOWN)
  {
    if (!m_arrowKeyHandler)
    {
      sF32 mod=1.0f;

      if (GetAsyncKeyState(VK_SHIFT))
        mod=10.0f;

      if (wParam==VK_LEFT || wParam==VK_DOWN)
        mod=-mod;

      UpdateValue(m_value+m_stepSize*mod, m_value+m_stepSize*mod, 0, GetAsyncKeyState(VK_MENU));
    }
    else
      m_arrowKeyHandler(wParam, m_arrowKeyHandlerParam);

    return 0;
  }

  bHandled=FALSE;
  return 1;
}

LRESULT frNumEditControl::OnRuleMouseInput(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  sInt buttons, dx, dy;
  WPARAM wp=0;

  ruleMousePoll(buttons, dx, dy);

  if (buttons & 1)  wp|=MK_LBUTTON;
  if (buttons & 2)  wp|=MK_RBUTTON;
  if (buttons & 4)  wp|=MK_MBUTTON;

  if (m_fPressed)
    mouseDelta(wp, dx);

  return 0;
}

void frNumEditControl::OnEditKeypress(WPARAM wParam)
{
  if (wParam==VK_RETURN)
  {
    static sChar buf[128];

    m_edit.GetWindowText(buf, 128);
    sF64 val;

    if (!m_evaluator)
      val=atof(buf);
    else
      m_evaluator->evaluate(buf, val);

    if (m_translateValueHandler)
      val=m_translateValueHandler(val, sTRUE, m_translateValueHandlerParam);

    if (val<m_min || val>m_max)
    {
      static sChar  msg[256];
      sF64          t_min, t_max;

      t_min=m_min;
      t_max=m_max;

      if (m_translateValueHandler)
      {
        t_min=m_translateValueHandler(t_min, sFALSE, m_translateValueHandlerParam);
        t_max=m_translateValueHandler(t_max, sFALSE, m_translateValueHandlerParam);
      }

      sprintf(msg, "Value must be between %g and %g", t_min, t_max);
      MessageBox(msg, "Warning", MB_ICONINFORMATION|MB_OK);
      BeginEdit();

      return;
    }

    if (val!=m_value)
    {
      m_value=val;

      m_dragStartValue=val;
      m_accumDelta=0;

      SetWindowText(formatVal());
      SendMessage(GetParent(), WM_COMMAND, MAKELONG(GetDlgCtrlID(), EN_CHANGE), 0);

      Invalidate();
      UpdateWindow();
    }
  }

  if (m_edit.IsWindow())
    m_edit.DestroyWindow();
}

void frNumEditControl::Init()
{
  m_value=0.0;
  m_min=0.0;
  m_max=100.0;
  m_stepSize=0.01;
  m_precision=3;
  m_fPressed=0;
}

void frNumEditControl::UpdateValue(sF32 newValue, sF32 dragStart, sInt accumDelta, sBool updateAll)
{
  if (newValue<m_min)
  {
    newValue=m_min;
    m_dragStartValue=newValue;
    m_accumDelta=0;
  }
  
  if (newValue>m_max)
  {
    newValue=m_max;
    m_dragStartValue=newValue;
    m_accumDelta=0;
  }

  if (newValue == m_value)
    return;

  m_value=newValue;
  m_dragStartValue=dragStart;
  m_accumDelta=accumDelta;
  
  SetWindowText(formatVal());
  
  if (GetStyle() & FRNES_IMMEDIATEUPDATE)
    SendMessage(GetParent(), WM_COMMAND, MAKELONG(GetDlgCtrlID(), EN_CHANGE), 0);

  if (updateAll && m_updateGroup.size())
  {
    for (updateList::iterator it=m_updateGroup.begin(); it!=m_updateGroup.end(); ++it)
      (*it)->UpdateValue(m_value, dragStart, accumDelta, sFALSE);
  }
  
  Invalidate();
  UpdateWindow();
}

void frNumEditControl::BeginEdit()
{
  RECT rc;

  if (!::IsWindow(m_edit) && !(GetStyle() & FRNES_NOKBDEDIT))
  {
    sU32 align;

    switch (GetStyle() & FRNES_ALIGN)
    {
    case FRNES_ALIGN_RIGHT:
    default:
      align=ES_RIGHT;
      break;
      
    case FRNES_ALIGN_LEFT:
      align=ES_LEFT;
      break;

    case FRNES_ALIGN_CENTER:
      align=ES_CENTER;
      break;
    }

    GetClientRect(&rc);
    m_edit.Create(m_hWnd, &rc, formatVal(), WS_CHILD|WS_VISIBLE|align|ES_AUTOHSCROLL, WS_EX_TOPMOST, 100);
    m_edit.SetFont(m_font);
    ::SetFocus(m_edit);
  }
}

void frNumEditControl::SetEditPos(const CPoint &clickPoint, sBool clientCoords)
{
  CPoint pnt=clickPoint;

  if (!clientCoords)
    ScreenToClient(&pnt);
  
  int ch=m_edit.CharFromPos(pnt);
  m_edit.SetSel(ch, ch);
}

void frNumEditControl::DoPaint(CDCHandle dc)
{
  RECT rect;

  GetClientRect(&rect);

  dc.SetBkColor(::GetSysColor(COLOR_3DFACE));
  ::SendMessage(GetParent(), WM_CTLCOLOREDIT, (WPARAM) ((HDC) dc), (LPARAM) m_hWnd);

  dc.SetTextColor(::GetSysColor((m_fHot|m_fFocus)?COLOR_HOTLIGHT:COLOR_BTNTEXT));
  CFontHandle oldFont=dc.SelectFont(m_font);

  switch (GetStyle() & FRNES_ALIGN)
  {
  case FRNES_ALIGN_RIGHT:
  default:
    dc.SetTextAlign(TA_RIGHT|TA_TOP);
    dc.ExtTextOut(rect.right-1, rect.top, ETO_OPAQUE, &rect, formatVal());
    break;
    
  case FRNES_ALIGN_LEFT:
    dc.SetTextAlign(TA_LEFT|TA_TOP);
    dc.ExtTextOut(0, rect.top, ETO_OPAQUE, &rect, formatVal());
    break;

  case FRNES_ALIGN_CENTER:
    dc.SetTextAlign(TA_CENTER|TA_TOP);
    dc.ExtTextOut((rect.left+rect.right)/2, rect.top, ETO_OPAQUE, &rect, formatVal());
    break;
  }

  dc.SelectFont(oldFont);
}

void frNumEditControl::AddToUpdateGroup(frNumEditControl *ctrl)
{
  m_updateGroup.push_back(ctrl);
}

void frNumEditControl::RemoveFromUpdateGroup(frNumEditControl *ctrl)
{
  m_updateGroup.remove(ctrl);
}

void frNumEditControl::SetArrowKeyHandler(arrowKeyHandler handler, LPVOID user)
{
  m_arrowKeyHandler=handler;
  m_arrowKeyHandlerParam=user;
}

void frNumEditControl::SetTranslateValueHandler(translateValueHandler handler, LPVOID user)
{
  m_translateValueHandler=handler;
  m_translateValueHandlerParam=user;
}

const sChar *frNumEditControl::formatVal()
{
  static sChar fmt[16];
  static sChar val[256];
  static sF64 value;
  
  if (m_translateValueHandler)
    value=m_translateValueHandler(m_value, sFALSE, m_translateValueHandlerParam);
  else
    value=m_value;
  
  if (m_precision)
  {
    sprintf(fmt, "%%.%df", m_precision);
    sprintf(val, fmt, value);
  }
  else
    sprintf(val, "%d", (sInt) value);
  
  return val;
}

void frNumEditControl::mouseDelta(WPARAM wParam, sInt deltaX)
{
  sInt fact=1;

  if (wParam & MK_MBUTTON)
    fact*=5;

  if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
    fact*=10;

  m_accumDelta+=deltaX*fact;
  UpdateValue(m_dragStartValue+m_accumDelta*m_stepSize, m_dragStartValue, m_accumDelta, !!(GetAsyncKeyState(VK_MENU) & 0x8000));
}

// This code is in the public domain. See LICENSE for details.

#ifndef __opbutton_h_
#define __opbutton_h_

#define FBN_MOVEORSIZE  0
#define FBN_CLICK       1
#define FBN_MIDDLECLICK 2
#define FBN_CTRLCLICK   3
#define FBN_SHIFTCLICK  4
#define FBN_MOVING      5

#include "frcontrol.h"

class frOpButton: public frButtonImpl<frOpButton, CWindow, CControlWinTraits>
{
public:
  DECLARE_WND_CLASS(_T("frOpButton"));

  unsigned      m_fEdited:1;
  unsigned      m_fDisplayed:1;
  unsigned      m_fLBDown:1;
  unsigned      m_fSized:1;
  sBool         m_justCreated;
  
  CBitmapHandle m_buttonsBitmap;
  sInt          m_buttonType;

  CPoint        m_moveOrigin, m_sizeOrigin;
  CSize         m_origSize;

  frOpButton() : m_fEdited(0), m_fDisplayed(0), m_fLBDown(0), m_buttonType(0)
  {
    m_trackMouseLeave=false;
  }

  typedef   frButtonImpl<frOpButton, CWindow, CControlWinTraits>  baseClass;

  BEGIN_MSG_MAP(frOpButton)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
    MESSAGE_HANDLER(WM_MBUTTONDOWN, OnMButtonDown)
    MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButtonDown)
    MESSAGE_HANDLER(WM_RBUTTONUP, OnRButtonUp)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_SIZE, OnPosSizeChange)
    CHAIN_MSG_MAP(baseClass)
  END_MSG_MAP()

  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    m_justCreated=sTRUE;

    bHandled=FALSE;
    return 0;
  }

  LRESULT OnLButtonUp(UINT Msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
  {
    LRESULT lRet=baseClass::OnLButtonUp(Msg, wParam, lParam, bHandled);
    CPoint  ptCursor(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    
    if (ptCursor.x!=m_moveOrigin.x || ptCursor.y!=m_moveOrigin.y)
      OnPosSizeChange(Msg, wParam, lParam, bHandled);

    m_fLBDown=0;

    Invalidate();
    UpdateWindow();
    
    return lRet;
  }
  
  LRESULT OnMButtonDown(UINT Msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
  {
    ::SendMessage(GetParent(), WM_COMMAND, MAKELONG(GetDlgCtrlID(), FBN_MIDDLECLICK), 0);
    
    SetWindowPos(HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    ::SetFocus(GetParent());

    bHandled=FALSE;
    return 0;
  }
  
  LRESULT OnRButtonDown(UINT Msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
  {
    m_fSized=0;

    if (wParam & MK_LBUTTON)
    {
      m_sizeOrigin=CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

      CRect rc;
      GetWindowRect(&rc);

      m_origSize=rc.Size();
    }
    else
      bHandled=FALSE;

    return 0;
  }
  
  LRESULT OnRButtonUp(UINT Msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
  {
    if (m_fSized)
      m_moveOrigin+=CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))-m_sizeOrigin;
    else
      bHandled=FALSE;
    
    return 0;
  }
  
  LRESULT OnMouseMove(UINT Msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
  {
    LRESULT lRet=baseClass::OnMouseMove(Msg, wParam, lParam, bHandled);
    
    if (m_fLBDown)
    {
      CPoint  ptCursor(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
      CRect   rect;
      CWindow parent=GetParent();
      
      GetWindowRect(&rect);

      if (!(wParam & MK_RBUTTON))
      {
        rect.OffsetRect(ptCursor.x-m_moveOrigin.x, ptCursor.y-m_moveOrigin.y);

        if (m_gridSnap)
          m_gridSnap->SnapRect(&rect);

        parent.ScreenToClient(&rect);
        SetWindowPos(HWND_TOP, &rect, 0);

        PostNotify(FBN_MOVING);

        parent.Invalidate();
        parent.UpdateWindow();
      }
      else
      {
        m_fSized=1;

        CSize sz=m_origSize;

        sz.cx+=ptCursor.x-m_sizeOrigin.x+13;
        sz.cx-=sz.cx%25;
        if (sz.cx<75)
          sz.cx=75;

        rect.right=rect.left+sz.cx;
        parent.ScreenToClient(&rect);
        SetWindowPos(HWND_TOP, &rect, 0);

        PostNotify(FBN_MOVING);

        parent.Invalidate();
        parent.UpdateWindow();
      }
    }

    return lRet;
  }
  
  LRESULT OnPosSizeChange(UINT Msg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
  {
    if (!m_justCreated)
      PostNotify(FBN_MOVEORSIZE);

    m_justCreated=sFALSE;
    
    return 0;
  }
  
  void DoClick(WPARAM wParam, LPARAM lParam)
  {
    sInt code=FBN_CLICK;

    if (wParam & MK_SHIFT)    code=FBN_SHIFTCLICK;
    if (wParam & MK_CONTROL)  code=FBN_CTRLCLICK;

    PostNotify(code);

    m_fLBDown=1;
    m_moveOrigin=CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

    SetWindowPos(HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    ::SetFocus(GetParent());
  }

  void DoDoubleClick(WPARAM wParam, LPARAM lParam)
  {
    m_fLBDown=1;
    
    SetWindowPos(HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    ::SetFocus(GetParent());
    
    Invalidate();
    UpdateWindow();
  }
  
  void DoPaint(CDCHandle dc)
  {
    RECT rc;
    
    GetClientRect(&rc);
    CDC tempDC;
    tempDC.CreateCompatibleDC(dc);
    
    CBitmapHandle oldBitmap=tempDC.SelectBitmap(m_buttonsBitmap);
    
    sInt srcx=GetCheck()?100:0;
    
    if (m_fMouseOver)
      srcx+=200;
    
    if (m_fDisplayed)
      srcx+=400;

    const sInt srcy=m_buttonType*25;
    
    dc.SetStretchBltMode(COLORONCOLOR);
    dc.BitBlt(0, 0, 21, 25, tempDC, srcx, srcy, SRCCOPY);
    dc.StretchBlt(21, 0, rc.right-42, 25, tempDC, srcx+21, srcy, 100-21*2, 25, SRCCOPY);
    dc.BitBlt(rc.right-21, 0, 21, 25, tempDC, srcx+99-21, srcy, SRCCOPY);
    
    AdjustClipRectangle(&rc);

    CFontHandle oldFont=dc.SelectFont(m_font);
    
    int len=GetWindowTextLength();
    TCHAR *txt=new TCHAR[len+1];
    GetWindowText(txt, len+1);
    
    dc.SetTextColor(0);
    dc.SetBkMode(TRANSPARENT);
    dc.DrawText(txt, len, &rc, DT_CENTER|DT_END_ELLIPSIS|DT_SINGLELINE|DT_VCENTER);

    delete[] txt;

    dc.SelectFont(oldFont);

    tempDC.SelectBitmap(oldBitmap);
  }

  void AdjustClipRectangle(LPRECT rc)
  {
    rc->top+=4;
    rc->bottom-=4;

    switch (m_buttonType)
    {
    case 0: // standard/generator
      rc->left+=4;
      rc->right-=4;
      break;

    case 1: // filter
      rc->left+=15;
      rc->right-=4;
      break;

    case 2: // combiner
      rc->left+=4;
      rc->right-=15;
      break;

    case 3: // store
    case 4: // load
      rc->left+=4;
      rc->right-=22;
      break;
    }
  }

  void UpdateButtonColors()
  {
    if (!IsWindow()) // kann passieren dass der button weg ist, weil die page geändert wurde
      return;

    if (!m_fDisplayed)
      if (!m_fEdited)
      {
        m_systemColors=true;
        UpdateColors();
      }
      else
        SetColorScheme(0xffffff, 0xffff00, 0x000000, 0xaa0000, 0xff0000, 0xff0000, 0xff8080);
    else
      if (!m_fEdited)
        SetColorScheme(0xffffff, 0x00ffff, 0x000000, 0x0000aa, 0x0000ff, 0x0000ff, 0x8080ff);
      else
        SetColorScheme(0x00ffff, 0xffffff, 0x404040, 0xaa00aa, 0xff00ff, 0xff00ff, 0xff80ff);

    Invalidate();
    UpdateWindow();
  }

  void SetDisplayed(sBool displayed)
  {
    m_fDisplayed=!!displayed;
    
    UpdateButtonColors();
  }

  void SetEdited(sBool edited)
  {
    m_fEdited=!!edited;
    UpdateButtonColors();
  }

  void SetButtonsBitmap(HBITMAP bitmap)
  {
    m_buttonsBitmap=bitmap;
  }

  void SetButtonType(sInt type)
  {
    m_buttonType=type;

    Invalidate();
    UpdateWindow();
  }
};

#endif

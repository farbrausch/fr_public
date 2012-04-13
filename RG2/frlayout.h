// This code is in the public domain. See LICENSE for details.

#pragma once
#pragma warning (push)
#pragma warning (disable: 4706) // assignment within conditional expression

#include "types.h"

#define BEGIN_DYNAMIC_LAYOUT_MAP(thisClass) \
  static const _frlLayoutMap *GetLayoutMap() \
  { \
    static const _frlLayoutMap theMap[]= \
    { \

#define DYNAMIC_LAYOUT_ELEMENT(id, flags) \
      { id, flags },

#define END_DYNAMIC_LAYOUT_MAP() \
      { (sU16) -1, 0 } \
    }; \
    return theMap; \
  }

template<class T, class TBase=CWindow, class TWinTraits=CControlWinTraits>
class frDynamicLayoutImpl: public CWindowImpl<T, TBase, TWinTraits>
{
public:
  // constants
  enum
  {
    m_cxySplitterSize=3,
    m_cxyTextOffset=4
  };
  
  enum
  {
    // flags
    FRDL_NONE         = 0x0000,
    FRDL_TITLEPANE    = 0x0001,
    FRDL_NOCLOSE      = 0x0002,
    FRDL_EDGE         = 0x0004,
  };
  
  // map data
  struct _frlLayoutMap
  {
    sU16  m_ID;
    sU16  m_Flags;
  };

  // layout info
  struct frlLayoutInfo
  {
    HWND            m_hWnd;
    frlLayoutInfo   *m_child[2];
    sU16            m_ID, m_flags;
    sInt            m_splitterPos, m_splitterMax;
    sBool           m_horiz, m_open;
    RECT            m_rects[2]; // splitter: left/right, leaf: title/client

    frlLayoutInfo()
    {
      m_child[0]=m_child[1]=0;
      m_hWnd=0;
      m_ID=0;
      m_flags=0;
      m_splitterPos=-1;
      m_splitterMax=1;
      m_horiz=sFALSE;
      m_open=sFALSE;
      ::SetRectEmpty(&m_rects[0]);
      ::SetRectEmpty(&m_rects[1]);
    }

    ~frlLayoutInfo()
    {
      if (m_child[0]) delete m_child[0];
      if (m_child[1]) delete m_child[1];
    }
  };

  frlLayoutInfo       *m_layout, *m_dragLayout, *m_overClose;
  const _frlLayoutMap *m_layoutMap;
  sInt                m_nLayoutElems, m_cxyDragOffset;
  RECT                m_rcLayout;
  sInt                m_cxyHeader;
  CMenuHandle         m_layoutMenu;

  static HCURSOR      m_hCursor, m_vCursor;

  frDynamicLayoutImpl() : m_layout(0), m_dragLayout(0), m_layoutMap(0), m_nLayoutElems(0), m_overClose(0)
  {
    ::SetRectEmpty(&m_rcLayout);
    
    T *pT=static_cast<T*>(this);
    m_layoutMap=pT->GetLayoutMap();

    for (m_nLayoutElems=0; m_layoutMap[m_nLayoutElems].m_ID!=(sU16) -1; m_nLayoutElems++);

    if (!m_hCursor)
    {
      ::EnterCriticalSection(&_Module.m_csStaticDataInit);
      if (!m_hCursor)
        m_hCursor=AtlLoadSysCursor(IDC_SIZEWE);
      ::LeaveCriticalSection(&_Module.m_csStaticDataInit);
    }

    if (!m_vCursor)
    {
      ::EnterCriticalSection(&_Module.m_csStaticDataInit);
      if (!m_vCursor)
        m_vCursor=AtlLoadSysCursor(IDC_SIZENS);
      ::LeaveCriticalSection(&_Module.m_csStaticDataInit);
    }

    pT->CalcHeaderSize();
  }
  
  ~frDynamicLayoutImpl()
  {
    if (m_layout)
    {
      delete m_layout;
      m_layout=0;
    }
  }
  
  frlLayoutInfo *AddLayoutItem(frlLayoutInfo *parent, sBool right, sU16 id, HWND hWnd)
  {
    T *pT=static_cast<T*>(this);
    
    ATLASSERT(!parent == !m_layout);

    if (parent)
      ATLASSERT(!parent->m_child[!!right]);

    frlLayoutInfo *inf=new frlLayoutInfo;
    inf->m_hWnd=hWnd;
    inf->m_ID=id;
    inf->m_open=sTRUE;

    for (sInt i=0; i<m_nLayoutElems; i++)
      if (m_layoutMap[i].m_ID==id)
        inf->m_flags=m_layoutMap[i].m_Flags;

    if (parent)
      parent->m_child[!!right]=inf;
    else
      m_layout=inf;

    pT->UpdateDynamicLayout();

    return inf;
  }

  frlLayoutInfo *AddLayoutSplitter(frlLayoutInfo *parent, sBool right, sBool horiz, sInt splitterPos=-1, sBool proportional=sTRUE)
  {
    T *pT=static_cast<T*>(this);

    ATLASSERT(!parent == !m_layout);

    if (parent)
      ATLASSERT(!parent->m_child[!!right]);

    frlLayoutInfo *inf=new frlLayoutInfo;
    inf->m_horiz=horiz;
    inf->m_splitterPos=splitterPos;
    inf->m_flags=!!proportional;

    if (parent)
      parent->m_child[!!right]=inf;
    else
      m_layout=inf;

    pT->UpdateDynamicLayout();

    return inf;
  }

  void SetLayoutRect(LPRECT lpRect=0, sBool bUpdate=sTRUE)
  {
    T *pT=static_cast<T*>(this);

    if (!lpRect)
      pT->GetClientRect(&m_rcLayout);
    else
      m_rcLayout=*lpRect;

    if (bUpdate)
      pT->UpdateDynamicLayout();
  }

  void GetLayoutRect(LPRECT lpRect) const
  {
    ATLASSERT(lpRect != 0);
    
    *lpRect=m_rcLayout;
  }

  void SetLayoutMenu(HMENU hMenu)
  {
    m_layoutMenu=hMenu;

    UpdateLayoutMenu();
  }

  HMENU GetLayoutMenu() const
  {
    return m_layoutMenu;
  }

  sBool SetSplitterPos(frlLayoutInfo *inf, sInt splitterPos=-1, sBool update=sTRUE)
  {
    T *pT=static_cast<T*>(this);

    RECT rc;
    ATLASSERT(inf != 0);

    GetSplitterRect(inf, &rc);

    if (splitterPos==-1)
    {
      if (!inf->m_horiz)
        splitterPos=(rc.right-rc.left)/2;
      else
        splitterPos=(rc.bottom-rc.top)/2;
    }

    sInt cxyMax;
    if (!inf->m_horiz)
      cxyMax=rc.right-rc.left;
    else
      cxyMax=rc.bottom-rc.top;

    if (splitterPos<m_cxySplitterSize)
      splitterPos=m_cxySplitterSize;
    else if (splitterPos>cxyMax-m_cxySplitterSize)
      splitterPos=cxyMax-m_cxySplitterSize;

    if (cxyMax<1)
      cxyMax=1;

    sBool ret=inf->m_splitterPos!=splitterPos;
    inf->m_splitterPos=splitterPos;
    inf->m_splitterMax=cxyMax;

    if (update && ret)
      pT->UpdateDynamicLayout();

    return ret;
  }

  sInt GetSplitterPos(const frlLayoutInfo *inf) const
  {
    return inf->m_splitterPos;
  }

  void DrawDynamicLayout(CDCHandle dc)
  {
    ATLASSERT(dc.m_hDC != 0);

    if (m_layout && m_layout->m_open)
      DrawLayoutRecurse(dc, m_layout);
    else
      DrawEmptyLayout(dc);
  }

  void DrawLayoutRecurse(CDCHandle dc, frlLayoutInfo *inf)
  {
    T *pT=static_cast<T*>(this);

    if (!inf->m_hWnd) // splitter
    {
      if (inf->m_child[0] && inf->m_child[1] && inf->m_child[0]->m_open && inf->m_child[1]->m_open)
        pT->DrawSplitterBar(dc, inf);

      if (inf->m_child[0] && inf->m_child[0]->m_open)
        DrawLayoutRecurse(dc, inf->m_child[0]);

      if (inf->m_child[1] && inf->m_child[1]->m_open)
        DrawLayoutRecurse(dc, inf->m_child[1]);
    }
    else // leaf
    {
      if (inf->m_flags & FRDL_TITLEPANE)
        pT->DrawTitlePane(dc, inf);

      if (inf->m_flags & FRDL_EDGE)
      {
        CRect rc=inf->m_rects[1];

        rc.InflateRect(1, 1);
        pT->DrawLeafEdge(dc, &rc);
      }
    }
  }

  // overrideables

  void DrawSplitterBar(CDCHandle dc, frlLayoutInfo *inf)
  {
    ATLASSERT(inf != 0);

    RECT rc;

    GetSplitterBarRect(inf, &rc);
    dc.FillRect(&rc, (HBRUSH) LongToPtr(COLOR_3DFACE+1));
  }

  void DrawTitlePane(CDCHandle dc, frlLayoutInfo *inf)
  {
    ATLASSERT(inf != 0);
    ATLASSERT(::IsWindow(inf->m_hWnd));

    int len=::GetWindowTextLength(inf->m_hWnd);

    RECT rc=inf->m_rects[0];
    dc.DrawEdge(&rc, EDGE_ETCHED, BF_LEFT|BF_TOP|BF_RIGHT|BF_ADJUST);
    dc.FillRect(&rc, (HBRUSH) LongToPtr(COLOR_3DFACE+1));
    
    TCHAR *text=new TCHAR[len+1];
    ::GetWindowText(inf->m_hWnd, text, len+1);
    text[len]=0;

    CFontHandle fnt=AtlGetDefaultGuiFont();
    CFontHandle oldFnt=dc.SelectFont(fnt);

    RECT rcButton=rc;

    if (!(inf->m_flags & FRDL_NOCLOSE))
    {
      T *pT=static_cast<T*>(this);

      pT->CalcCloseButtonRect(inf, &rcButton);
      pT->DrawCloseButton(dc, &rc, &rcButton);
    }
    else
      rcButton.left=rcButton.right;

    rc.left+=m_cxyTextOffset;
    rc.right=rcButton.left-m_cxyTextOffset;

    dc.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
    dc.SetBkMode(TRANSPARENT);
    dc.DrawText(text, len, &rc, DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
    dc.SelectFont(oldFnt);
    
    delete[] text;
  }
  
  void DrawEmptyLayout(CDCHandle dc)
  {
    RECT rc;
    
    GetClientRect(&rc);
    dc.DrawEdge(&rc, BDR_SUNKENOUTER, BF_RECT|BF_ADJUST);
    dc.FillRect(&rc, (HBRUSH) LongToPtr(COLOR_APPWORKSPACE+1));
  }

  void DrawLeafEdge(CDCHandle dc, LPCRECT lpr)
  {
    dc.Draw3dRect(lpr->left, lpr->top, lpr->right-lpr->left, lpr->bottom-lpr->top, ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DHILIGHT));
  }

  void DrawCloseButton(CDCHandle dc, LPRECT lpClip, LPRECT lpRect, sBool mouseOver=sFALSE)
  {
    CPen pen;
    pen.CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_WINDOWTEXT));

    CPenHandle oldPen=dc.SelectPen(pen);

    CRgn clipRgn;
    clipRgn.CreateRectRgnIndirect(lpClip);
    dc.SelectClipRgn(clipRgn);

    dc.FillRect(lpRect, (HBRUSH) LongToPtr(COLOR_3DFACE+1));
    if (mouseOver)
    {
      lpRect->left++;
      dc.Draw3dRect(lpRect, ::GetSysColor(COLOR_3DHILIGHT), ::GetSysColor(COLOR_3DSHADOW));
      lpRect->left--;
    }

    lpRect->left+=4; lpRect->top+=4;
    lpRect->right-=4; lpRect->bottom-=4;

    dc.MoveTo(lpRect->left, lpRect->top);
    dc.LineTo(lpRect->right, lpRect->bottom);
    dc.MoveTo(lpRect->left + 1, lpRect->top);
    dc.LineTo(lpRect->right + 1, lpRect->bottom);
    
    dc.MoveTo(lpRect->left, lpRect->bottom - 1);
    dc.LineTo(lpRect->right, lpRect->top - 1);
    dc.MoveTo(lpRect->left + 1, lpRect->bottom - 1);
    dc.LineTo(lpRect->right + 1, lpRect->top - 1);

    dc.SelectPen(oldPen);
    dc.SelectClipRgn(0);
  }
  
  void GetSplitterRect(frlLayoutInfo *inf, LPRECT lpRect) const
  {
    ATLASSERT(inf != 0);
    ATLASSERT(lpRect != 0);

    sBool pane1=inf->m_child[0] && inf->m_child[0]->m_open;
    sBool pane2=inf->m_child[1] && inf->m_child[1]->m_open;
    
    if (pane1 && pane2) // beide
    {
      *lpRect=inf->m_rects[0];
      if (!inf->m_horiz)
        lpRect->right=inf->m_rects[1].right;
      else
        lpRect->bottom=inf->m_rects[1].bottom;
    }
    else if (pane1 && !pane2) // nur links
      *lpRect=inf->m_rects[0];
    else if (!pane1 && pane2) // nur rechts
      *lpRect=inf->m_rects[1];
    else
      ::SetRectEmpty(lpRect);
  }
  
  void GetSplitterBarRect(frlLayoutInfo *inf, LPRECT lpRect) const
  {
    ATLASSERT(inf != 0);
    ATLASSERT(lpRect != 0);

    if (!inf->m_horiz)
    {
      lpRect->left=inf->m_rects[0].right;
      lpRect->right=inf->m_rects[1].left;
      lpRect->top=inf->m_rects[0].top;
      lpRect->bottom=inf->m_rects[0].bottom;
    }
    else
    {
      lpRect->left=inf->m_rects[0].left;
      lpRect->right=inf->m_rects[0].right;
      lpRect->top=inf->m_rects[0].bottom;
      lpRect->bottom=inf->m_rects[1].top;
    }
  }

  typedef frDynamicLayoutImpl<T, TBase, TWinTraits> thisClass;
  BEGIN_MSG_MAP(thisClass)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
  ALT_MSG_MAP(1) // Parent window messages
    MESSAGE_HANDLER(WM_COMMAND, OnParentCommand)
  END_MSG_MAP()

  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    bHandled=FALSE;
    return 0;
  }

  LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    return 1;
  }

  LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    T *pT=static_cast<T*>(this);
    
    CPaintDC dc(pT->m_hWnd);
    pT->DrawDynamicLayout(dc.m_hDC);

    return 0;
  }

  LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    T *pT=static_cast<T*>(this);

    pT->SetLayoutRect();
    pT->UpdateDynamicLayout();

    bHandled=FALSE;
    return 0;
  }

  LRESULT OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    T *pT=static_cast<T*>(this);

    if ((HWND) wParam==pT->m_hWnd && lParam==HTCLIENT)
    {
      DWORD dwPos=::GetMessagePos();
      POINT ptPos={GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos)};
      pT->ScreenToClient(&ptPos);
      
      if (pT->SplitterBarHitTest(m_layout, ptPos.x, ptPos.y))
        return 1;
    }

    bHandled=FALSE;
    return 0;
  }

  LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    T *pT=static_cast<T*>(this);

    sInt x=GET_X_LPARAM(lParam);
    sInt y=GET_Y_LPARAM(lParam);

    frlLayoutInfo *hit=0;

    if ((wParam & MK_LBUTTON) && ::GetCapture()==m_hWnd)
    {
      sInt xyNewSplitPos=0;

      RECT rc;
      pT->GetSplitterRect(m_dragLayout, &rc);
      
      if (!m_dragLayout->m_horiz)
        xyNewSplitPos=x-rc.left-m_cxyDragOffset;
      else
        xyNewSplitPos=y-rc.top-m_cxyDragOffset;

      if (xyNewSplitPos==-1)
        xyNewSplitPos=-2;

      if (m_dragLayout->m_splitterPos!=xyNewSplitPos)
      {
        if (pT->SetSplitterPos(m_dragLayout, xyNewSplitPos, sTRUE))
          pT->UpdateWindow();
      }
    }
    else
    {
      if ((hit=pT->SplitterBarHitTest(m_layout, x, y)))
      {
        if (!hit->m_horiz)
          ::SetCursor(m_hCursor);
        else
          ::SetCursor(m_vCursor);

        bHandled=FALSE;
      }
      
      if ((hit=pT->CloseButtonHitTest(m_layout, x, y)))
      {
        if (hit!=m_overClose)
        {
          RECT rc;
          CDCHandle dc=GetDC();
          
          if (m_overClose)
          {
            pT->CalcCloseButtonRect(m_overClose, &rc);
            pT->DrawCloseButton(dc, &m_overClose->m_rects[0], &rc);
          }
          
          pT->CalcCloseButtonRect(hit, &rc);
          pT->DrawCloseButton(dc, &hit->m_rects[0], &rc, sTRUE);
          
          ReleaseDC(dc);
          
          m_overClose=hit;
        }
      }
    }
    
    if (m_overClose && hit!=m_overClose)
    {
      RECT rc;
      CDCHandle dc=GetDC();
      
      pT->CalcCloseButtonRect(m_overClose, &rc);
      pT->DrawCloseButton(dc, &m_overClose->m_rects[0], &rc);
      
      ReleaseDC(dc);

      m_overClose=0;
    }
    
    return 0;
  }

  LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    T *pT=static_cast<T*>(this);
    
    frlLayoutInfo *hit;

    sInt x=GET_X_LPARAM(lParam);
    sInt y=GET_Y_LPARAM(lParam);

    if ((hit=pT->SplitterBarHitTest(m_layout, x, y)))
    {
      RECT rc;

      pT->GetSplitterRect(hit, &rc);
      pT->SetCapture();

      if (!hit->m_horiz)
      {
        ::SetCursor(m_hCursor);

        m_cxyDragOffset=x-rc.left-hit->m_splitterPos;
      }
      else
      {
        ::SetCursor(m_vCursor);
        m_cxyDragOffset=y-rc.top-hit->m_splitterPos;
      }
      
      m_dragLayout=hit;
    }
    else if ((hit=pT->CloseButtonHitTest(m_layout, x, y)))
    {
      ::ShowWindow(hit->m_hWnd, SW_HIDE);
      pT->UpdateDynamicLayout();
      m_overClose=0;

      UpdateWindow();

      if ((hit=pT->CloseButtonHitTest(m_layout, x, y)))
      {
        RECT rc;
        CDCHandle dc=GetDC();
        
        pT->CalcCloseButtonRect(hit, &rc);
        pT->DrawCloseButton(dc, &hit->m_rects[0], &rc, sTRUE);
        
        ReleaseDC(dc);
          
        m_overClose=hit;
      }
    }

    bHandled=FALSE;
    return 1;
  }

  LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    if (::GetCapture()==m_hWnd)
      ::ReleaseCapture();

    bHandled=FALSE;
    return 1;
  }

  LRESULT OnParentCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    frlLayoutInfo *inf=GetItemByID(LOWORD(wParam));

    if (inf && ::IsWindow(inf->m_hWnd))
    {
      ::ShowWindow(inf->m_hWnd, inf->m_open ? SW_HIDE : SW_SHOW);
      UpdateDynamicLayout();
    }
    else
      bHandled=FALSE;

    return 0;
  }

  void UpdateDynamicLayout()
  {
    T *pT=static_cast<T*>(this);

    if (m_layout)
    {
      RECT rc=m_rcLayout;
      pT->UpdateLayoutRecurse(m_layout, 0);
      pT->UpdateLayoutRecurse(m_layout, &rc);
      UpdateLayoutMenu();
    }

    if (m_layout && !m_layout->m_open)
      Invalidate();
  }

  void UpdateLayoutRecurse(frlLayoutInfo *inf, LPCRECT lpRect)
  {
    T *pT=static_cast<T*>(this);

    if (!lpRect)
    {
      if (!inf->m_hWnd) // splitter
      {
        inf->m_open=sFALSE;

        if (inf->m_child[0])
        {
          pT->UpdateLayoutRecurse(inf->m_child[0], 0);
          inf->m_open|=inf->m_child[0]->m_open;
        }

        if (inf->m_child[1])
        {
          pT->UpdateLayoutRecurse(inf->m_child[1], 0);
          inf->m_open|=inf->m_child[1]->m_open;
        }
      }
      else // leaf
        inf->m_open=!!::IsWindowVisible(inf->m_hWnd);
    }
    else if (inf->m_open)
    {
      if (!inf->m_hWnd) // splitter
      {
        sBool pane1=inf->m_child[0] && inf->m_child[0]->m_open;
        sBool pane2=inf->m_child[1] && inf->m_child[1]->m_open;

        if (pane1 && pane2) // beide panes (also splitter)
        {
          inf->m_rects[0]=*lpRect;
          inf->m_rects[1]=*lpRect;

          RECT splitter;
          splitter=*lpRect;

          if (!inf->m_horiz)
          {
            if (inf->m_splitterPos==-1)
            {
              inf->m_splitterPos=(lpRect->right-lpRect->left)/2;
              inf->m_splitterMax=lpRect->right-lpRect->left;
            }

            if (inf->m_flags & 1) // proportional?
            {
              inf->m_splitterPos=MulDiv(inf->m_splitterPos, lpRect->right-lpRect->left, inf->m_splitterMax);
              inf->m_splitterMax=lpRect->right-lpRect->left;
            }

            inf->m_rects[0].right=inf->m_rects[0].left+inf->m_splitterPos;
            inf->m_rects[1].left+=inf->m_splitterPos+m_cxySplitterSize;

            splitter.left+=inf->m_splitterPos;
            splitter.right=splitter.left+m_cxySplitterSize;
          }
          else
          {
            if (inf->m_splitterPos==-1)
            {
              inf->m_splitterPos=(lpRect->bottom-lpRect->top)/2;
              inf->m_splitterMax=lpRect->bottom-lpRect->top;
            }

            if (inf->m_flags & 1) // proportional?
            {
              inf->m_splitterPos=MulDiv(inf->m_splitterPos, lpRect->bottom-lpRect->top, inf->m_splitterMax);
              inf->m_splitterMax=lpRect->bottom-lpRect->top;
            }
            
            inf->m_rects[0].bottom=inf->m_rects[0].top+inf->m_splitterPos;
            inf->m_rects[1].top+=inf->m_splitterPos+m_cxySplitterSize;

            splitter.top+=inf->m_splitterPos;
            splitter.bottom=splitter.top+m_cxySplitterSize;
          }

          pT->InvalidateRect(&splitter, FALSE);

          pT->UpdateLayoutRecurse(inf->m_child[0], &inf->m_rects[0]);
          pT->UpdateLayoutRecurse(inf->m_child[1], &inf->m_rects[1]);
        }
        else if (pane1 && !pane2) // nur die erste pane
        {
          inf->m_rects[0]=*lpRect;
          pT->UpdateLayoutRecurse(inf->m_child[0], &inf->m_rects[0]);
        }
        else if (!pane1 && pane2) // nur die zweite pane
        {
          inf->m_rects[1]=*lpRect;
          pT->UpdateLayoutRecurse(inf->m_child[1], &inf->m_rects[1]);
        }
      }
      else // leaf
      {
        inf->m_rects[1]=*lpRect; // client area

        if (inf->m_flags & FRDL_TITLEPANE)
        {
          inf->m_rects[0]=*lpRect;
          inf->m_rects[0].bottom=inf->m_rects[0].top+m_cxyHeader;
          inf->m_rects[1].top=inf->m_rects[0].bottom;

          InvalidateRect(&inf->m_rects[0], FALSE);
        }

        if (inf->m_flags & FRDL_EDGE)
        {
          InvalidateRect(inf->m_rects+1, FALSE);
          ::InflateRect(inf->m_rects+1, -1, -1);
        }

        CWindow wnd=inf->m_hWnd;
        wnd.SetWindowPos(0, inf->m_rects+1, SWP_NOZORDER|SWP_NOACTIVATE);
      }
    }
  }

  frlLayoutInfo *SplitterBarHitTest(frlLayoutInfo *inf, sInt x, sInt y) const
  {
    const T *pT=static_cast<const T *>(this);

    if (!inf)
      return 0;

    if (inf->m_hWnd)
      return 0;

    frlLayoutInfo *res;

    if (inf->m_child[0])
    {
      res=pT->SplitterBarHitTest(inf->m_child[0], x, y);
      if (res)
        return res;
    }

    if (inf->m_child[1])
    {
      res=pT->SplitterBarHitTest(inf->m_child[1], x, y);
      if (res)
        return res;
    }

    sBool pane1=inf->m_child[0] && inf->m_child[0]->m_open;
    sBool pane2=inf->m_child[1] && inf->m_child[1]->m_open;

    if (!(pane1 && pane2))
      return 0;

    RECT rc;
    pT->GetSplitterBarRect(inf, &rc);
    if (x>=rc.left && x<rc.right && y>=rc.top && y<rc.bottom)
      return inf;
    else
      return 0;
  }
  
  void CalcHeaderSize()
  {
    CFontHandle fnt=AtlGetDefaultGuiFont();
    LOGFONT lf;
    
    fnt.GetLogFont(lf);
    
    m_cxyHeader=abs(lf.lfHeight)+::GetSystemMetrics(SM_CYEDGE)+m_cxyTextOffset*2;
  }
  
  void CalcCloseButtonRect(frlLayoutInfo *inf, LPRECT lpRect) const
  {
    ATLASSERT(inf != 0);
    ATLASSERT(lpRect != 0);
    ATLASSERT(inf->m_hWnd != 0);
    ATLASSERT(inf->m_open == sTRUE);
    ATLASSERT((inf->m_flags & FRDL_TITLEPANE) && !(inf->m_flags & FRDL_NOCLOSE));
    
    CFontHandle fnt=AtlGetDefaultGuiFont();
    LOGFONT lf;
    fnt.GetLogFont(lf);
    
    sInt sz=abs(lf.lfHeight)+m_cxyTextOffset;
    
    *lpRect=inf->m_rects[0];

    lpRect->right-=::GetSystemMetrics(SM_CXEDGE)+2;
    lpRect->left=lpRect->right-sz;
    lpRect->top=lpRect->top+(m_cxyHeader+::GetSystemMetrics(SM_CYEDGE)-sz)/2;
    lpRect->bottom=lpRect->top+sz;
  }

  frlLayoutInfo *CloseButtonHitTest(frlLayoutInfo *inf, sInt x, sInt y) const
  {
    frlLayoutInfo *res;

    if (!inf)
      return 0;

    if (!inf->m_hWnd)
    {
      if (inf->m_child[0])
        if ((res=CloseButtonHitTest(inf->m_child[0], x, y)))
          return res;

      if (inf->m_child[1])
        if ((res=CloseButtonHitTest(inf->m_child[1], x, y)))
          return res;
    }
    else
    {
      if (!inf->m_open)
        return 0;

      if (!(inf->m_flags & FRDL_TITLEPANE) || (inf->m_flags & FRDL_NOCLOSE))
        return 0;

      RECT rc;
      CalcCloseButtonRect(inf, &rc);

      if (x>=rc.left && x<rc.right && y>=rc.top && y<rc.bottom)
        return inf;
    }

    return sFALSE;
  }

  frlLayoutInfo *GetItemByIDRecurse(frlLayoutInfo *inf, sU16 id) const
  {
    if (!inf->m_hWnd) // splitter
    {
      frlLayoutInfo *res;

      if (inf->m_child[0])
        if ((res=GetItemByIDRecurse(inf->m_child[0], id)))
          return res;

      if (inf->m_child[1])
        if ((res=GetItemByIDRecurse(inf->m_child[1], id)))
          return res;

      return 0;
    }
    else
    {
      if (inf->m_ID==id) // we found it
        return inf;
      else
        return 0; // this is not the right one
    }
  }

  frlLayoutInfo *GetItemByID(sU16 id) const
  {
    return GetItemByIDRecurse(m_layout, id);
  }

  void UpdateLayoutMenu()
  {
    sInt i, count;

    if (!::IsMenu(m_layoutMenu.m_hMenu))
      return;

    // first, clear the menu
    count=m_layoutMenu.GetMenuItemCount();
    for (i=0; i<count; i++)
      m_layoutMenu.DeleteMenu(0, MF_BYPOSITION);

    // second, repopulate it
    for (i=0; i<m_nLayoutElems; i++)
    {
      if (m_layoutMap[i].m_Flags & FRDL_NOCLOSE)
        continue; // don't bother with noncloseable windows

      frlLayoutInfo *inf=GetItemByID(m_layoutMap[i].m_ID);

      if (inf && ::IsWindow(inf->m_hWnd))
      {
        sInt len=::GetWindowTextLength(inf->m_hWnd);
        TCHAR *buf=new TCHAR[len+1];
        ::GetWindowText(inf->m_hWnd, buf, len+1);
        buf[len]=0;

        m_layoutMenu.AppendMenu(MF_STRING | (inf->m_open ? MF_CHECKED : MF_UNCHECKED), inf->m_ID, buf);
        delete[] buf;
      }
    }
  }
};

template<class T, class TBase, class TWinTraits> HCURSOR frDynamicLayoutImpl<T, TBase, TWinTraits>::m_hCursor=0;
template<class T, class TBase, class TWinTraits> HCURSOR frDynamicLayoutImpl<T, TBase, TWinTraits>::m_vCursor=0;

#pragma warning (pop)

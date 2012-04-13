// This code is in the public domain. See LICENSE for details.

// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__503EA551_E313_42E5_A630_6367B5452B26__INCLUDED_)
#define AFX_MAINFRM_H__503EA551_E313_42E5_A630_6367B5452B26__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "frOpGraph.h"
#include "stream.h"
#include "tstream.h"
#include "TG2Config.h"
#include "preferences.h"
#include "texbase.h"

class CMainFrame : public CFrameWindowImpl<CMainFrame>, public CUpdateUI<CMainFrame>, public CMessageFilter, public CIdleHandler
{
public:
  DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)
    
  CTG2View m_view;
  CMultiPaneStatusBarCtrl m_sbar;
  CRecentDocumentList m_mru;
  sInt m_showMode;
  CBitmap m_titleBar, m_menuBackground;
  CBitmap m_borders;
  CCommandBarCtrl m_CmdBar;
  sBool m_isActive;
  WINDOWPLACEMENT m_placement;

  sBool m_ncClick;
  CRect m_ncRect;
  sInt m_ncBut;
  
  TCHAR m_strFilePath[_MAX_PATH];
  TCHAR m_strFileName[_MAX_PATH];

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		if(CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
			return TRUE;

		return m_view.PreTranslateMessage(pMsg);
	}

	virtual BOOL OnIdle()
	{
		UIEnable(ID_EDIT_UNDO, g_graph ? g_graph->m_undo.canUndo() : sFALSE);
		UIEnable(ID_EDIT_REDO, g_graph ? g_graph->m_undo.canRedo() : sFALSE);

		return FALSE;
	}

  CMainFrame()
  {
    frInitTextureBase();

    g_config = new CTG2Config;
    g_graph = new frOpGraph;
    m_strFilePath[0] = 0;
    m_strFileName[0] = 0;

    m_titleBar.LoadBitmap(IDB_TITLEBAR_NEW);
    m_borders.LoadBitmap(IDB_BORDERS);
    m_ncClick = sFALSE;
    m_isActive = sTRUE;
  }

  ~CMainFrame()
  {
    if (g_graph)
    {
      delete g_graph;
      g_graph = 0;
    }

    if (g_config)
    {
      delete g_config;
      g_config = 0;
    }

    frCloseTextureBase();
  }
  
  BEGIN_MSG_MAP(CMainFrame)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_CLOSE, OnClose)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    
    MESSAGE_HANDLER(WM_NCACTIVATE, OnNCActivate)
    MESSAGE_HANDLER(WM_NCCALCSIZE, OnNCCalcSize)
    MESSAGE_HANDLER(WM_NCPAINT, OnNCPaint)
    MESSAGE_HANDLER(WM_NCHITTEST, OnNCHitTest)
    MESSAGE_HANDLER(WM_NCLBUTTONDOWN, OnNCLButtonDown)
    MESSAGE_HANDLER(WM_NCLBUTTONUP, OnNCLButtonUp)
    MESSAGE_HANDLER(WM_NCMOUSEMOVE, OnNCMouseMove)
    
    COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
    COMMAND_ID_HANDLER(ID_FILE_NEW, OnFileNew)
    COMMAND_ID_HANDLER(ID_FILE_OPEN, OnFileOpen)
    COMMAND_RANGE_HANDLER(ID_FILE_MRU_FIRST, ID_FILE_MRU_LAST, OnFileRecent)
    COMMAND_ID_HANDLER(ID_FILE_SAVE, OnFileSave)
    COMMAND_ID_HANDLER(ID_FILE_SAVE_AS, OnFileSaveAs)
    COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER(ID_EDIT_UNDO, OnEditUndo)
		COMMAND_ID_HANDLER(ID_EDIT_REDO, OnEditRedo)
    COMMAND_ID_HANDLER(ID_EDIT_PREFERENCES, OnEditPreferences)

    MESSAGE_HANDLER(WM_SET_CALC_TIME, OnSetCalcTime)
    MESSAGE_HANDLER(WM_SET_ACTION_MESSAGE, OnSetActionMessage)
    MESSAGE_HANDLER(WM_SET_POSITION_MESSAGE, OnSetPositionMessage)
    MESSAGE_HANDLER(WM_SET_STATUS_MESSAGE, OnSetStatusMessage)
    
    NOTIFY_HANDLER(ATL_IDW_TOOLBAR, NM_CUSTOMDRAW, OnCommandBarCustomDraw)

    CHAIN_MSG_MAP_ALT_MEMBER(m_view, 1)
    CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
    CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
  END_MSG_MAP()
    
  BEGIN_UPDATE_UI_MAP(CMainFrame)
    UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_EDIT_UNDO, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_EDIT_REDO, UPDUI_MENUPOPUP)
  END_UPDATE_UI_MAP()
    
  LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
  {    
    g_winMap.Add(IDR_MAINFRAME, m_hWnd);

    CMenuHandle hMenu = GetMenu();
    
    COLORSCHEME cs;
    cs.dwSize = sizeof(cs);
    cs.clrBtnHighlight = RGB(200, 198, 196);
    cs.clrBtnShadow = RGB(103, 99, 96);
    
    CSize szb;
    m_menuBackground.LoadBitmap(IDB_MENUBACKGROUND);
    m_menuBackground.GetSize(szb);
    
    HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE|TBSTYLE_TRANSPARENT);

    CSize szp, szm;
    m_CmdBar.GetPadding(&szp);
    m_CmdBar.GetMaxSize(&szm);
    
    szm.cy += 4;
    
    if (szm.cy < szb.cy) // adjust padding if toolbar smaller than background image
      szp.cy += szb.cy - szm.cy;
    
    m_CmdBar.SetPadding(szp.cx, szp.cy);
    m_CmdBar.AttachMenu(hMenu);
    m_CmdBar.LoadImages(IDR_MAINFRAME);
    m_CmdBar.SetColorScheme(&cs);
    SetMenu(0);
    
    CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
    AddSimpleReBarBand(hWndCmdBar);
    
    REBARBANDINFO rbi;
    rbi.cbSize = sizeof(rbi);
    rbi.clrBack = RGB(193, 191, 189);
    rbi.clrFore = RGB(193, 191, 189);
    rbi.hbmBack = m_menuBackground;
    rbi.fMask = RBBIM_COLORS | RBBIM_BACKGROUND;
    ::SendMessage(m_hWndToolBar, RB_SETBANDINFO, 0, (LPARAM) &rbi);

    CreateSimpleStatusBar("");
    m_sbar.SubclassWindow(m_hWndStatusBar);
    m_sbar.ModifyStyle(0, SBARS_SIZEGRIP, SWP_FRAMECHANGED);
    m_sbar.SetBkColor(RGB(174, 169, 167));

    int arrParts[] = {ID_POS_PANE, ID_ACTION_PANE, ID_TIME_PANE, ID_DEFAULT_PANE};
    m_sbar.SetPanes(arrParts, sizeof(arrParts) / sizeof(int), false);
    
		m_hWndClient = m_view.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0);

		UISetCheck(ID_VIEW_STATUS_BAR, 1);
		UIEnable(ID_EDIT_UNDO, FALSE);
		UIEnable(ID_EDIT_REDO, FALSE);

    CMenuHandle hFileMenu = hMenu.GetSubMenu(0);
    CMenuHandle hMRUMenu = hFileMenu.GetSubMenu(5);
    m_mru.SetMenuHandle(hMRUMenu);
    m_mru.ReadFromRegistry(lpcstrTG2RegKey);

		CMessageLoop* pLoop = _Module.GetMessageLoop();
		pLoop->AddMessageFilter(this);
		pLoop->AddIdleHandler(this);

    m_showMode = SW_SHOWDEFAULT;
    ::PostMessage(m_hWnd, WM_COMMAND, ID_FILE_NEW, 0);
    
    return 0;
  }

	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		pLoop->RemoveMessageFilter(this);
		pLoop->RemoveIdleHandler(this);

		bHandled = FALSE;
		return 0;
	}
  
  LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    if (QueryClose())
    {
      bHandled=FALSE;

      CRegKey key;
      sChar buf[128];

      WINDOWPLACEMENT &plm=m_placement;
      
      wsprintf(buf, "%d,%d,%d,%d", plm.rcNormalPosition.left, plm.rcNormalPosition.top,
        plm.rcNormalPosition.right, plm.rcNormalPosition.bottom);

      key.Create(HKEY_CURRENT_USER, lpcstrTG2RegKey);
      key.SetDWORDValue(_T("WindowMode"), m_showMode);
      key.SetStringValue(_T("WindowPos"), buf);
      key.Close();
    }
    
    return 0;
  }
  
  LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    switch (wParam)
    {
    case SIZE_MAXIMIZED:  m_showMode=SW_MAXIMIZE;     break;
    case SIZE_MINIMIZED:  m_showMode=SW_MINIMIZE;     break;
    case SIZE_RESTORED:   m_showMode=SW_SHOWDEFAULT;  break;
    }

    GetWindowPlacement(&m_placement);
    
    CSize sz;
    m_titleBar.GetSize(sz);
    sz.cy/=3;

    CRect rc;
    GetWindowRect(&rc);
    rc.bottom=rc.top+sz.cy;
    rc.OffsetRect(-rc.left, -rc.top);
    
    HDC dc=GetWindowDC();

    DrawTitleBar(dc, &rc);
    GetWindowRect(&rc);
    rc.OffsetRect(-rc.left, -rc.top);
    rc.top+=sz.cy;
    DrawBorders(dc, &rc);
    ReleaseDC(dc);
    
    bHandled=FALSE;
    return 0;
  }
  
  LRESULT OnNCActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    HDC dc=GetWindowDC();
    
    m_isActive=!!wParam;

    CRect rc;
    SIZE sz;
    GetWindowRect(&rc);
    m_titleBar.GetSize(sz);
    sz.cy/=3;
    rc.OffsetRect(-rc.left, -rc.top);
    rc.bottom=rc.top+sz.cy;

    DrawTitleBar(dc, &rc);

    GetWindowRect(&rc);
    rc.OffsetRect(-rc.left, -rc.top);
    rc.top+=sz.cy;
    DrawBorders(dc, &rc);

    ReleaseDC(dc);
    
    return TRUE;
  }
  
  LRESULT OnNCCalcSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    CSize sz;
    m_titleBar.GetSize(sz);
    sz.cy/=3;

    if (!wParam)
    {
      LPRECT rc=(LPRECT) lParam;

      // title bar
      rc->top+=sz.cy;
      // borders
      rc->left+=5;
      rc->right-=5;
      rc->bottom-=5;
    }
    else
    {
      LPNCCALCSIZE_PARAMS ncs=(LPNCCALCSIZE_PARAMS) lParam;

      // title bar
      ncs->rgrc[0].top+=sz.cy;
      // borders
      ncs->rgrc[0].left+=5;
      ncs->rgrc[0].right-=5;
      ncs->rgrc[0].bottom-=5;
    }

    return wParam?WVR_ALIGNBOTTOM|WVR_ALIGNLEFT|WVR_REDRAW|WVR_VALIDRECTS:0;
  }
  
  LRESULT OnNCPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    HRGN updateRgn=(HRGN) wParam;
    
    CRect frameRect, rc;
    CSize sz;

    m_titleBar.GetSize(sz);
    sz.cy/=3;

    GetWindowRect(&frameRect);
    rc=frameRect;
    rc.bottom=rc.top+sz.cy;
    
    rc.OffsetRect(-frameRect.left, -frameRect.top);

    HDC dc=GetWindowDC();
    DrawTitleBar(dc, &rc);

    rc=frameRect;
    rc.top+=sz.cy;
    rc.OffsetRect(-frameRect.left, -frameRect.top);

    DrawBorders(dc, &rc);
    ReleaseDC(dc);

    return 0;
  }
  
  LRESULT OnNCHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    RECT rc;
    CSize sz;

    m_titleBar.GetSize(sz);
    sz.cy/=3;

    sInt xframe=5, yframe=5;

    GetWindowRect(&rc);
    if (!::PtInRect(&rc, pt))
      return HTNOWHERE;

    if (pt.y>=rc.bottom-yframe) // bottom
    {
      if (pt.x<rc.left+xframe) // left
        return HTBOTTOMLEFT;
      else if (pt.x>=rc.right-xframe) // right
        return HTBOTTOMRIGHT;
      else
        return HTBOTTOM;
    }

    if (pt.x<rc.left+xframe) // left
      return HTLEFT;

    if (pt.x>=rc.right-xframe)
      return HTRIGHT;

    if (pt.y<rc.top+sz.cy)
    {
      if (pt.x>=rc.left+4 && pt.x<=rc.left+16 && pt.y>=rc.top+3 && pt.y<=rc.top+16)
        return HTSYSMENU;

      sInt ypos=rc.top+3;

      if (pt.y>=ypos && pt.y<ypos+14)
      {
        sInt left=rc.right-56;
      
        if (pt.x>=left && pt.x<left+14)
          return HTMINBUTTON;
        else if (pt.x>=left+20 && pt.x<left+35)
          return HTMAXBUTTON;
        else if (pt.x>=left+36 && pt.x<left+51)
          return HTCLOSE;
      }

      return HTCAPTION;
    }
    
    return HTNOWHERE;
  }
  
  LRESULT OnNCLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    m_ncClick=sTRUE;

    CRect wRect;
    CSize sz;
    GetWindowRect(&wRect);
    m_titleBar.GetSize(sz);
    sz.cy/=3;

    sInt top=wRect.top+3;
    sInt left=wRect.right-56;

    m_ncBut=0;

    switch (wParam)
    {
    case HTMINBUTTON:
      m_ncBut=wParam;
      m_ncRect=CRect(left, top, left+15, top+15);
      DrawCaptionButton(wParam, sTRUE);
      break;

    case HTMAXBUTTON:
      m_ncBut=wParam;
      m_ncRect=CRect(left+20, top, left+35, top+15);
      DrawCaptionButton(wParam, sTRUE);
      break;

    case HTCLOSE:
      m_ncBut=wParam;
      m_ncRect=CRect(left+36, top, left+51, top+15);
      DrawCaptionButton(wParam, sTRUE);
      break;

    default:
      return DefWindowProc(uMsg, wParam, lParam);
    }

    return 0;
  }
  
  LRESULT OnNCLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    LRESULT res=0;

    CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    
    if (m_ncClick && ::PtInRect(&m_ncRect, pt))
    {
      DrawCaptionButton(m_ncBut, sFALSE);
      
      switch (m_ncBut)
      {
      case HTMINBUTTON:
        ShowWindow(SW_MINIMIZE);
        break;
      
      case HTMAXBUTTON:
        ShowWindow((m_showMode==SW_MAXIMIZE || m_showMode==SW_SHOWMAXIMIZED)?SW_RESTORE:SW_MAXIMIZE);
        break;
      
      case HTCLOSE:
        PostMessage(WM_CLOSE, 0, 0);
        break;

      default:
        res=DefWindowProc(uMsg, wParam, lParam);
      }
    }
    else
      res=DefWindowProc(uMsg, wParam, lParam);

    m_ncClick=sFALSE;
    
    return res;
  }

  LRESULT OnNCMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    static sBool hasHot=sFALSE;

    if (m_ncClick && m_ncBut)
      DrawCaptionButton(m_ncBut, ::PtInRect(&m_ncRect, CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))));
    else if (wParam==HTMINBUTTON || wParam==HTMAXBUTTON || wParam==HTCLOSE)
    {
      hasHot=sTRUE;
      if (wParam!=HTMINBUTTON)
        DrawCaptionButton(HTMINBUTTON, sFALSE, sFALSE);

      if (wParam!=HTMAXBUTTON)
        DrawCaptionButton(HTMAXBUTTON, sFALSE, sFALSE);

      if (wParam!=HTCLOSE)
        DrawCaptionButton(HTCLOSE, sFALSE, sFALSE);
      
      DrawCaptionButton(wParam, sFALSE, sTRUE);
    }
    else
    {
      if (hasHot)
      {
        DrawCaptionButton(HTMINBUTTON, sFALSE, sFALSE);
        DrawCaptionButton(HTMAXBUTTON, sFALSE, sFALSE);
        DrawCaptionButton(HTCLOSE, sFALSE, sFALSE);
      }

      m_ncClick=sFALSE;
    }

    return 0;
  }
  
  LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
  {
    PostMessage(WM_CLOSE);
    return 0;
  }

  LRESULT OnFileNew(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
  {
    if (QueryClose())
    {
      ::SendMessage(g_winMap.Get(ID_GRAPH_WINDOW), WM_CLEAR_GRAPH, 0, 0);
      ::SendMessage(g_winMap.Get(ID_PAGE_WINDOW), WM_REFRESHPAGEVIEW, 0, 0);

      ::SendMessage(g_winMap.Get(ID_SPLINE_WINDOW), WM_SET_EDIT_SPLINE, 0, 0);
      ::SendMessage(g_winMap.Get(ID_TIMELINE_WINDOW), WM_SET_CURRENT_CLIP, 0, 0);
      
      m_view.m_page.AddPage("tex page 1", 0);
      m_view.m_page.AddPage("geo page 2", 1, sFALSE);
      m_view.m_page.AddPage("comp page 3", 2, sFALSE);
      g_graph->resetModified();

      m_strFilePath[0]=0;
      m_strFileName[0]=0;
    }

    return 0;
  }
    
  LRESULT OnFileOpen(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
  {
    CFileDialog dlg(TRUE, _T("rg21"), _T(""), OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT, _T("RauschGenerator 2.1 Files (*.rg21)\0*.rg21\0RG2 Files (*.tg2)\0*.tg2\0"));
    sInt        nRet=dlg.DoModal();
    
    if (nRet==IDOK)
    {
      BOOL bRet=QueryClose();
      
      if (bRet && DoFileOpen(dlg.m_ofn.lpstrFile, dlg.m_ofn.lpstrFileTitle))
      {
        m_mru.AddToList(dlg.m_ofn.lpstrFile);
        m_mru.WriteToRegistry(lpcstrTG2RegKey);
      }
    }
    
    return 0;
  }

  LRESULT OnFileRecent(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
  {
    if (!QueryClose())
      return 0;

    TCHAR szFile[MAX_PATH];
    if (m_mru.GetFromList(wID, szFile))
    {
      LPTSTR lpstrFileName=szFile;
      
      for (sInt i=lstrlen(szFile)-1; i>=0; i--)
      {
        if (szFile[i]=='\\' || szFile[i]=='/')
        {
          lpstrFileName=&szFile[i+1];
          break;
        }
      }
        
      if (DoFileOpen(szFile, lpstrFileName))
        m_mru.MoveToTop(wID);
      else
        m_mru.RemoveFromList(wID);
      
      m_mru.WriteToRegistry(lpcstrTG2RegKey);        
    }

    return 0;
  }
    
  LRESULT OnFileSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
  {
    sBool saveFailed=sFALSE;
    
    if (m_strFilePath[0])
    {
      if (!SaveFile(m_strFilePath))
        saveFailed=sTRUE;
    }
    else
    {
      if (!DoFileSaveAs())
        saveFailed=sTRUE;
    }
      
    if (saveFailed)
    {
      CString str;
      
      str.LoadString(IDR_MAINFRAME);
      MessageBox(_T("Save failed"), str, MB_ICONERROR|MB_OK);
    }
        
    return 0;
  }
    
  LRESULT OnFileSaveAs(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
  {
    if (!DoFileSaveAs())
    {
      CString str;

      str.LoadString(IDR_MAINFRAME);
      MessageBox(_T("Save failed"), str, MB_ICONERROR|MB_OK);
    }

    return 0;
  }

  LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		BOOL bNew = !::IsWindowVisible(m_hWndStatusBar);
		::ShowWindow(m_hWndStatusBar, bNew ? SW_SHOWNOACTIVATE : SW_HIDE);
		UISetCheck(ID_VIEW_STATUS_BAR, bNew);
		UpdateLayout();
		return 0;
	}

	LRESULT OnEditUndo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if (g_graph)
			g_graph->m_undo.undo();

		return 0;
	}

	LRESULT OnEditRedo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if (g_graph)
			g_graph->m_undo.redo();

		return 0;
	}

	LRESULT OnEditPreferences(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
  {
    CPreferencesDialog dlg;
    dlg.DoModal();

    return 0;
  }

  LRESULT OnSetCalcTime(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    sChar buf[24];
    wsprintf(buf, "calc in %d ms", lParam);
    m_sbar.SetPaneText(ID_TIME_PANE, buf);

    return 0;
  }

  LRESULT OnSetActionMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    if (lParam)
      m_sbar.SetPaneText(ID_ACTION_PANE, (LPCTSTR) lParam);
    else
      m_sbar.SetPaneText(ID_ACTION_PANE, "");
    
    return 0;
  }

  LRESULT OnSetPositionMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    if (wParam)
    {
      sChar buf[64];
      wsprintf(buf, "(%d,%d)", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
      m_sbar.SetPaneText(ID_POS_PANE, buf);
    }
    else
      m_sbar.SetPaneText(ID_POS_PANE, "");
    
    return 0;
  }

  LRESULT OnSetStatusMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    if (lParam)
      m_sbar.SetPaneText(ID_DEFAULT_PANE, (LPCTSTR) lParam);
    else
      m_sbar.SetPaneText(ID_DEFAULT_PANE, "");

    m_sbar.UpdateWindow();
    
    return 0;
  }
  
  LRESULT OnCommandBarCustomDraw(WPARAM wParam, LPNMHDR nmh, BOOL &bHandled)
  {
    LPNMTBCUSTOMDRAW tbc=(LPNMTBCUSTOMDRAW) nmh;

    if (tbc->nmcd.dwDrawStage==CDDS_POSTERASE)
    {
      CDCHandle dc=tbc->nmcd.hdc;

      dc.FillSolidRect(&tbc->nmcd.rc, RGB(174, 169, 167));
    }

    return 0;
  }

  BOOL QueryClose()
  {
    if (!g_graph)
      return TRUE;

    if (!g_graph->m_modified)
      return TRUE;
    else
    {
      CString str;

      str.LoadString(IDR_MAINFRAME);
      int res=MessageBox(_T("Save changes?"), str, MB_ICONQUESTION|MB_YESNOCANCEL|MB_DEFBUTTON1);

      if (res==IDYES)
        ::SendMessage(m_hWnd, WM_COMMAND, ID_FILE_SAVE, 0);

      return res!=IDCANCEL;
    }
  }

  BOOL DoFileOpen(LPCTSTR lpstrFileName, LPCTSTR lpstrFileTitle)
  {
    if (LoadFile(lpstrFileName))
    {
      sInt strl = strlen(lpstrFileName);

      if (strl < 4 || stricmp(lpstrFileName + strl - 4, ".tg2")) // don't allow saving .tg2 files (it's obvious why)
      {
        lstrcpy(m_strFilePath, lpstrFileName);
        lstrcpy(m_strFileName, lpstrFileTitle);
      }
      else
      {
        m_strFilePath[0] = 0;
        m_strFileName[0] = 0;
      }

      return TRUE;
    }
    else
    {
      CString str;
      
      str.LoadString(IDR_MAINFRAME);
      MessageBox(_T("Open failed"), str, MB_ICONERROR|MB_OK);
      
      return FALSE;
    }
  }
  
  BOOL DoFileSaveAs()
  {
    BOOL        bRet=TRUE;
    CFileDialog dlg(FALSE, _T("rg21"), NULL, OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT, _T("RauschGenerator 2.1 Files (*.rg21)\0*.rg21\0"));
    int         nRet=dlg.DoModal();
    
    if (nRet==IDOK)
    {
      bRet=SaveFile(dlg.m_ofn.lpstrFile);
      lstrcpy(m_strFilePath, dlg.m_ofn.lpstrFile);
      lstrcpy(m_strFileName, dlg.m_ofn.lpstrFileTitle);
    }
    
    return bRet;
  }
  
  BOOL LoadFile(LPCTSTR lpstrFileName)
  {
    fr::streamF f;
    
    ::SendMessage(g_winMap.Get(ID_GRAPH_WINDOW), WM_SYNC_CALCULATIONS, 0, 0);
    
    if (f.open(lpstrFileName))
    {
      ::SendMessage(g_winMap.Get(ID_GRAPH_WINDOW), WM_CLEAR_GRAPH, 0, 0);

      sInt strl = strlen(lpstrFileName);

      fr::streamWrapper strm(f, sFALSE);

      if (strl < 4 || stricmp(lpstrFileName + strl - 4, ".tg2"))
        g_graph->serialize(strm);
      else
        g_graph->import(strm);

      f.close();
      fr::debugOut("file %s loaded (%d ops)\n", lpstrFileName, g_graph->m_ops.size());
      
      ::SendMessage(g_winMap.Get(ID_GRAPH_WINDOW), WM_FILE_LOADED, 0, 0);
      ::SendMessage(g_winMap.Get(ID_PAGE_WINDOW), WM_REFRESHPAGEVIEW, 0, 0);
      ::SendMessage(g_winMap.Get(ID_TIMELINE_WINDOW), WM_FILE_LOADED, 0, 0);
      
      return TRUE; 
    }
    else
      return FALSE;
  }
  
  BOOL SaveFile(LPCTSTR lpstrFileName)
  {
    fr::streamF f;
    
    if (f.open(lpstrFileName, fr::streamF::cr|fr::streamF::wr))
    {
      fr::streamWrapper strm(f, sTRUE);
      g_graph->serialize(strm);
      f.close();
      
      m_mru.AddToList(lpstrFileName);
      m_mru.WriteToRegistry(lpcstrTG2RegKey);
      
      return TRUE;
    }
    else
      return FALSE;
  }
  
  void DrawTitleBar(CDCHandle dc, LPCRECT rc)
  {
    CSize sz, sz2;
    m_titleBar.GetSize(sz);
    sz.cy/=3;
    
    sInt len=GetWindowTextLength();
    TCHAR *text=new TCHAR[len+1];
    GetWindowText(text, len+1);
    text[len]=0;

    CFontHandle deffnt=AtlGetStockFont(DEFAULT_GUI_FONT);
    LOGFONT lf;
    deffnt.GetLogFont(lf);

    CFont fnt;
    lf.lfWeight=FW_BOLD;
    fnt.CreateFontIndirect(&lf);
    
    CDC tempDC, tempDC2;
    tempDC.CreateCompatibleDC(dc);
    tempDC2.CreateCompatibleDC(dc);

    CFontHandle oldFont=tempDC2.SelectFont(fnt);
    tempDC2.GetTextExtent(text, len, &sz2);
    
    RECT textRC=*rc;
    sInt buttonSize=rc->bottom-rc->top-2*::GetSystemMetrics(SM_CYFRAME)-2;
    
    textRC.left+=3+26;
    textRC.top+=3;
    textRC.right-=3*buttonSize+::GetSystemMetrics(SM_CXFRAME)+3;

    sz2.cx+=textRC.left;

    CBitmap tempBmp;
    tempBmp.CreateCompatibleBitmap(dc, sz2.cx, 21);
    CBitmapHandle oldBitmap=tempDC.SelectBitmap(m_titleBar);
    CBitmapHandle oldBitmap2=tempDC2.SelectBitmap(tempBmp);

    sInt yst=m_isActive?21:0;

    tempDC2.BitBlt(0, 0, 26, 21, tempDC, 0, yst, SRCCOPY);
    tempDC2.StretchBlt(26, 0, sz2.cx-26, 21, tempDC, 26, yst, 1, 21, SRCCOPY);
    
    tempDC2.SetBkMode(TRANSPARENT);
    tempDC2.SetTextColor(m_isActive?RGB(235, 235, 235):RGB(110, 110, 110));
    tempDC2.DrawText(text, -1, &textRC, DT_SINGLELINE|DT_END_ELLIPSIS|DT_VCENTER);
    tempDC2.SelectFont(oldFont);  
    dc.BitBlt(rc->left, rc->top, sz2.cx, sz.cy, tempDC2, 0, 0, SRCCOPY);
    dc.StretchBlt(rc->left+sz2.cx, rc->top, rc->right-rc->left-65-sz2.cx, 21, tempDC, 26, yst, 1, 21, SRCCOPY);
    dc.BitBlt(rc->right-65, rc->top, 10, 21, tempDC, 27, yst, SRCCOPY);
    dc.BitBlt(rc->right-55, rc->top, 55, 21, tempDC, 37, 0, SRCCOPY);
    tempDC.SelectBitmap(oldBitmap);
    tempDC2.SelectBitmap(oldBitmap2);

    delete[] text;
  }

  void DrawBorders(CDCHandle dc, LPCRECT rc)
  {
    CDC tempDC;
    tempDC.CreateCompatibleDC(dc);
    CBitmapHandle oldBitmap=tempDC.SelectBitmap(m_borders);

    // flächen
    dc.StretchBlt(rc->left, rc->top, 5, rc->bottom-rc->top-5, tempDC, 0, 0, 5, 1, SRCCOPY);
    dc.StretchBlt(rc->right-5, rc->top, 5, rc->bottom-rc->top-5, tempDC, 6, 0, 5, 1, SRCCOPY);
    dc.StretchBlt(rc->left+5, rc->bottom-5, rc->right-rc->left-10, 5, tempDC, 5, 1, 1, 5, SRCCOPY);

    // ecken
    dc.BitBlt(rc->left, rc->bottom-5, 5, 5, tempDC, 0, 1, SRCCOPY);
    dc.BitBlt(rc->right-5, rc->bottom-5, 5, 5, tempDC, 6, 1, SRCCOPY);
    
    tempDC.SelectBitmap(oldBitmap);
  }
  
  void DrawCaptionButton(sInt hitCode, sBool pushed, sBool hot=sFALSE)
  {
    CRect wRect;
    CSize sz;
    
    m_titleBar.GetSize(sz);
    sz.cy/=3;
    GetWindowRect(&wRect);
    
    sInt left=wRect.right-wRect.left-56;

    CDC dc=GetWindowDC();
    CDC tempDC;

    tempDC.CreateCompatibleDC(dc);
    CBitmapHandle oldBitmap=tempDC.SelectBitmap(m_titleBar);
    
    sInt xs=0;
    sInt ys=0;

    if (hot)
      ys=21;

    if (pushed)
      ys=42;

    switch (hitCode)
    {
    case HTMINBUTTON: xs=0;   break;
    case HTMAXBUTTON: xs=20;  break;
    case HTCLOSE:     xs=36;  break;
    }

    dc.BitBlt(left+xs, 0, 14, 21, tempDC, xs+36, ys, SRCCOPY);
    tempDC.SelectBitmap(oldBitmap);
  }
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__503EA551_E313_42E5_A630_6367B5452B26__INCLUDED_)

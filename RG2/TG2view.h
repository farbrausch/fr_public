// This code is in the public domain. See LICENSE for details.

// TG2View.h : interface of the CTG2View class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_TG2VIEW_H__2579F428_F0EF_4358_94B0_41BE6BDBC008__INCLUDED_)
#define AFX_TG2VIEW_H__2579F428_F0EF_4358_94B0_41BE6BDBC008__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "PreviewWindow.h"
#include "3dPreviewWindow.h"
#include "ParamWindow.h"
#include "PageWindow.h"
#include "GraphWindow.h"
#include "SplineEdit.h"
#include "TimelineWindow.h"
#include "AnimMixer.h"
#include "WindowMapper.h"

#include "TG2Config.h"
#include "frLayout.h"
#include "exporter.h"
#include "frOpGraph.h"

#include "plugbase.h"
#include "cmpbase.h"

class CViruzIIConfig : public CDialogImpl<CViruzIIConfig>
{
public:
  enum { IDD=IDD_VIRUZCONFIG };

  BEGIN_MSG_MAP(CViruzIIConfig)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDOK, OnEnd)
    COMMAND_ID_HANDLER(IDCANCEL, OnEnd)
    COMMAND_ID_HANDLER(IDC_OPENTUNE, OnOpenTune)
    COMMAND_ID_HANDLER(IDC_OPENTUNE2, OnOpenTune)
  END_MSG_MAP()
  
  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    SetDlgItemText(IDC_TUNEEDIT, g_graph->getTuneName());
    SetDlgItemText(IDC_TUNEEDIT2, g_graph->m_loaderTuneName);

    return 0;
  }

  LRESULT OnEnd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
  {
    if (wID==IDOK)
    {
      TCHAR text[256];
      GetDlgItemText(IDC_TUNEEDIT, text, 256);
      g_graph->setTuneName(text);

      GetDlgItemText(IDC_TUNEEDIT2, text, 256);
      g_graph->m_loaderTuneName=text;
    }

    EndDialog(wID==IDOK);
    return 0;
  }

  LRESULT OnOpenTune(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
  {
    CFileDialog fileDlg(TRUE, "*.v2m", 0, OFN_OVERWRITEPROMPT, "ViruzII modules (*.v2m)\0*.v2m\0", m_hWnd);

    if (fileDlg.DoModal()==IDOK)
      SetDlgItemText((wID == IDC_OPENTUNE) ? IDC_TUNEEDIT : IDC_TUNEEDIT2, fileDlg.m_ofn.lpstrFile);

    return 0;
  }
};

class CTG2View : public frDynamicLayoutImpl<CTG2View>
{
public:
	DECLARE_WND_CLASS(NULL);

  CPreviewWindow    m_preview;
  C3DPreviewWindow  m_3dpreview;
  CParamWindow      m_param;
  CPageWindow       m_page;
  CGraphWindow      m_graph;
  CSplineEdit       m_spline;
  CTimelineWindow   m_timeline;
  CAnimMixer        m_animMixer;
  sU32              m_tabHitTime;
  sBool             m_editVisible, m_pollBetween;
  sInt              m_fsWindow;

  frlLayoutInfo     *m_lTLWork;
  
  frButton          *m_buttons;
  frButtonChooser   m_resChooser;
  sInt              m_nButtons;

  sInt              m_curModule; // 0=generate 1=animate
  sInt              m_subMode;
  
  CBitmap           m_paneBitmap;

  typedef frDynamicLayoutImpl<CTG2View> baseClass;

	BOOL PreTranslateMessage(MSG* pMsg)
	{
    BOOL ret=FALSE;
    sBool pollState=m_pollBetween;

    if (pMsg->message==WM_KEYDOWN && !(pMsg->lParam & (1<<30)))
    {
      if (pMsg->wParam==VK_TAB)
      {
        m_tabHitTime=GetTickCount();
        ToggleFullscreenPreview(m_fsWindow);
        
        pollState=sFALSE;
        ret=TRUE;
      }
    }
    else if (pMsg->message==WM_KEYUP && (pMsg->lParam & (1<<30)))
    {
      if (pMsg->wParam==VK_TAB)
      {
        m_tabHitTime=GetTickCount()-m_tabHitTime;
        if (m_tabHitTime>=g_config->getSlopeKeyDelay() && m_pollBetween)
          ToggleFullscreenPreview(m_fsWindow);

        ret=TRUE;
      }
    }
    else if ((pMsg->message>=WM_KEYFIRST && pMsg->message<=WM_KEYLAST))
      pollState=sTRUE;

    m_pollBetween=pollState;
		return ret;
	}

  CTG2View()
  {
    m_lTLWork=0;

    m_paneBitmap.LoadBitmap(IDB_PANEBACKGROUND);
    CalcHeaderSize();
    UpdateDynamicLayout();

    m_subMode=0;
  }

	BEGIN_MSG_MAP(CTG2View)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_COMMAND, OnCommand)
    MESSAGE_HANDLER(WM_SET_FS_WINDOW, OnSetFullscreenWindow)
    MESSAGE_HANDLER(WM_SET_SUBMODE, OnSetSubmode)
    CHAIN_MSG_MAP(baseClass)
  ALT_MSG_MAP(1)
    COMMAND_RANGE_HANDLER(ID_MODULE_0, ID_MODULE_1, OnModuleChange)
    CHAIN_MSG_MAP_ALT_MEMBER(m_graph, 1)
	END_MSG_MAP()

  BEGIN_DYNAMIC_LAYOUT_MAP(CTG2View)
    DYNAMIC_LAYOUT_ELEMENT(ID_PREVIEW_WINDOW, FRDL_NOCLOSE)
    DYNAMIC_LAYOUT_ELEMENT(ID_3DPREVIEW_WINDOW, FRDL_NOCLOSE)
    DYNAMIC_LAYOUT_ELEMENT(ID_PARAM_WINDOW, FRDL_TITLEPANE|FRDL_NOCLOSE)
    DYNAMIC_LAYOUT_ELEMENT(ID_PAGE_WINDOW, FRDL_NOCLOSE|FRDL_EDGE)
    DYNAMIC_LAYOUT_ELEMENT(ID_GRAPH_WINDOW, FRDL_TITLEPANE|FRDL_NOCLOSE)
    DYNAMIC_LAYOUT_ELEMENT(ID_SPLINE_WINDOW, FRDL_TITLEPANE|FRDL_NOCLOSE)
    DYNAMIC_LAYOUT_ELEMENT(ID_TIMELINE_WINDOW, FRDL_NOCLOSE)
    DYNAMIC_LAYOUT_ELEMENT(ID_ANIMMIXER_WINDOW, FRDL_TITLEPANE|FRDL_NOCLOSE)
  END_DYNAMIC_LAYOUT_MAP()

  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    HWND hPreview=m_preview.Create(m_hWnd, rcDefault, _T("Preview Window"), WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN, 0);
    HWND h3DPreview=m_3dpreview.Create(m_hWnd, rcDefault, _T("3D Preview Window"), WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN, 0);
    HWND hParam=m_param.Create(m_hWnd, rcDefault, _T("Parameters: (no edit item)"), WS_CHILD|WS_CLIPCHILDREN, 0);
    HWND hPage=m_page.Create(m_hWnd, rcDefault, _T("Pages"), WS_CHILD|WS_CLIPCHILDREN, 0);
    HWND hGraph=m_graph.Create(m_hWnd, rcDefault, _T("Operator Graph"), WS_CHILD|WS_CLIPCHILDREN, 0);
    HWND hSpline=m_spline.Create(m_hWnd, rcDefault, _T("Spline Editor"), WS_CHILD|WS_CLIPCHILDREN, 0);
    HWND hTimeline=m_timeline.Create(m_hWnd, rcDefault, _T("Timeline"), WS_CHILD|WS_CLIPCHILDREN, 0);
    HWND hAnimMixer=m_animMixer.Create(m_hWnd, rcDefault, _T("Animation Mixer"), WS_CHILD|WS_CLIPCHILDREN, 0);

    CRegKey key;
    DWORD editWindowsVisible=1, currentFSWindow=0, currentModule=0;
    sChar splitterPositions[128];
    sU32 splitterPosSize=128;
    sInt splitterPos1=280+22+10, splitterPos2=300+5+300, splitterPos3=200, splitterPos4=300;

    if (key.Open(HKEY_CURRENT_USER, lpcstrTG2RegKey, KEY_READ) == ERROR_SUCCESS)
    {
      key.QueryDWORDValue(_T("EditWindowsVisible"), editWindowsVisible);
      key.QueryStringValue(_T("SplitterPositions"), splitterPositions, &splitterPosSize);
      key.QueryDWORDValue(_T("CurrentFSWindow"), currentFSWindow);
      key.QueryDWORDValue(_T("CurrentModule"), currentModule);
      key.Close();
    }

    sscanf(splitterPositions, "%d,%d,%d,%d", &splitterPos1, &splitterPos4, &splitterPos3, &splitterPos2);

    if (splitterPos2<splitterPos4)
      splitterPos2=splitterPos4+5+300;

    m_curModule=currentModule;
    
    ToggleFullscreenPreview(currentFSWindow, editWindowsVisible);

    m_editVisible=!!editWindowsVisible;
    m_fsWindow=currentFSWindow;
    
    frlLayoutInfo *tlwork=AddLayoutSplitter(0, sFALSE, sTRUE, 0, sFALSE);
    frlLayoutInfo *root=AddLayoutSplitter(tlwork, sFALSE, sTRUE, splitterPos1, sFALSE);
    frlLayoutInfo *top=AddLayoutSplitter(root, sFALSE, sFALSE, splitterPos2, sFALSE);
    
    frlLayoutInfo *preview=AddLayoutSplitter(top, sFALSE, sFALSE, splitterPos4, sFALSE);
    frlLayoutInfo *bottom=AddLayoutSplitter(root, sTRUE, sFALSE, splitterPos3, sFALSE);
    frlLayoutInfo *work=AddLayoutSplitter(bottom, sTRUE, sTRUE, 0, sFALSE);
    frlLayoutInfo *work2=AddLayoutSplitter(work, sTRUE, sTRUE, 0, sFALSE);

    AddLayoutItem(preview, sFALSE, ID_PREVIEW_WINDOW, hPreview);
    AddLayoutItem(preview, sTRUE, ID_3DPREVIEW_WINDOW, h3DPreview);

    AddLayoutItem(top, sTRUE, ID_PARAM_WINDOW, hParam);
    AddLayoutItem(bottom, sFALSE, ID_PAGE_WINDOW, hPage);

    AddLayoutItem(work, sFALSE, ID_GRAPH_WINDOW, hGraph);
    AddLayoutItem(work2, sFALSE, ID_SPLINE_WINDOW, hSpline);
    AddLayoutItem(work2, sTRUE, ID_ANIMMIXER_WINDOW, hAnimMixer);

    AddLayoutItem(tlwork, sTRUE, ID_TIMELINE_WINDOW, hTimeline);

    m_lTLWork=tlwork;

    key.Close();

    g_winMap.Add(ID_MAIN_VIEW, m_hWnd);
    
    g_winMap.Add(ID_PREVIEW_WINDOW, hPreview);
    g_winMap.Add(ID_3DPREVIEW_WINDOW, h3DPreview);
    g_winMap.Add(ID_PARAM_WINDOW, hParam);
    g_winMap.Add(ID_PAGE_WINDOW, hPage);
    g_winMap.Add(ID_GRAPH_WINDOW, hGraph);
    g_winMap.Add(ID_SPLINE_WINDOW, hSpline);
    g_winMap.Add(ID_TIMELINE_WINDOW, hTimeline);
    g_winMap.Add(ID_ANIMMIXER_WINDOW, hAnimMixer);

    static const sChar *buttons[]={"calc cams", "V2 setup", "V2 reset", "Make Demo", 0};
    for (m_nButtons=0; buttons[m_nButtons]; m_nButtons++);

    m_buttons=new frButton[m_nButtons];
    for (sInt i=0; i<m_nButtons; i++)
    {
      m_buttons[i].Create(m_hWnd, rcDefault, buttons[i], FRBS_FLAT|FRBS_HOTTRACK|WS_CHILD|WS_VISIBLE, 0, 1000+i);
      m_buttons[i].SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
        RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));
      m_buttons[i].SetFont(AtlGetDefaultGuiFont());
    }

    m_resChooser.Create(m_hWnd, rcDefault, 0, FRBS_FLAT|FRBS_HOTTRACK|WS_CHILD|WS_VISIBLE, 0, 2000);
    m_resChooser.SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
        RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));
    m_resChooser.SetFont(AtlGetDefaultGuiFont());

    m_resChooser.AddMenuItem(1, "512x384");
    m_resChooser.AddMenuItem(2, "640x480");
    m_resChooser.AddMenuItem(3, "800x600");
    m_resChooser.AddMenuItem(4, "1024x768");
    m_resChooser.SetSelection(2);

    bHandled=FALSE;
    return 0;
  }

  LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    CRegKey key;
    sChar   splitterPositions[128];

    frlLayoutInfo *baseLayout=m_layout->m_child[0];
    
    sprintf(splitterPositions, "%d,%d,%d,%d", baseLayout->m_splitterPos,
      baseLayout->m_child[0]->m_child[0]->m_splitterPos, baseLayout->m_child[1]->m_splitterPos,
      baseLayout->m_child[0]->m_splitterPos);
    
    key.Create(HKEY_CURRENT_USER, lpcstrTG2RegKey);
    key.SetDWORDValue(_T("EditWindowsVisible"), m_editVisible);
    key.SetDWORDValue(_T("CurrentFSWindow"), m_fsWindow);
    key.SetStringValue(_T("SplitterPositions"), splitterPositions);
    key.SetDWORDValue(_T("CurrentModule"), m_curModule);
    key.Close();

    for (sInt i=0; i<m_nButtons; i++)
      m_buttons[i].DestroyWindow();

    delete[] m_buttons;
    
    bHandled=FALSE;
    return 0;
  }

  LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    RECT rc;
    GetClientRect(&rc);

    sInt xright=rc.right;

    CDCHandle dc=GetDC();
    CFontHandle oldFont=dc.SelectFont(AtlGetDefaultGuiFont());

    for (sInt i=m_nButtons-1; i>=0; i--)
    {
      TCHAR text[256];
      m_buttons[i].GetWindowText(text, 256);

      SIZE sz;
      dc.GetTextExtent(text, -1, &sz);

      const sInt width=sz.cx+8;

      xright-=width;
      m_buttons[i].SetWindowPos(0, xright, 0, width, 19, SWP_NOZORDER);
      xright--;
    }

    m_resChooser.SetWindowPos(0, xright-64, 0, 64, 19, SWP_NOZORDER);

    dc.SelectFont(oldFont);
    ReleaseDC(dc);

    bHandled=FALSE;
    return 0;
  }

  LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    switch (LOWORD(wParam))
    {
    case 1000: // calc cams
      {
        // go through all compositing ops and evaluate them
        sInt counter = 0, count = 0;
        sBool wasComp = sTRUE;

        for (frOpGraph::opMapIt it = g_graph->m_ops.begin(); it != g_graph->m_ops.end(); ++it)
        {
          if (it->second.plg->def->type == 2) // compositing op? inc count.
            count++;
        }

        sU32 oldView3DOp = g_graph->m_view3DOp;

        for (frOpGraph::opMapIt it = g_graph->m_ops.begin(); it != g_graph->m_ops.end(); ++it)
        {
          if (wasComp)
          {
            sChar buf[32];
            sprintf(buf, "calc cams %d%%", MulDiv(counter, 100, count));
            ::SendMessage(g_winMap.Get(IDR_MAINFRAME), WM_SET_STATUS_MESSAGE, 0, (LPARAM) buf);

            wasComp = sFALSE;
          }

					frComposePlugin* plg = (frComposePlugin*) it->second.plg;

					if (plg && plg->def->type == 2) // compositing op?
          {
            wasComp = sTRUE;
            counter++;

						plg->process(); // process it.

            // then set it as 3d view item
            ::SendMessage(g_winMap.Get(ID_3DPREVIEW_WINDOW), WM_SET_DISPLAY_MESH, 0, it->first);
          }
        }

        ::SendMessage(g_winMap.Get(IDR_MAINFRAME), WM_SET_STATUS_MESSAGE, 0, (LPARAM) "calc cams done");
        ::SendMessage(g_winMap.Get(ID_3DPREVIEW_WINDOW), WM_SET_DISPLAY_MESH, 0, oldView3DOp);
      }
      return 0;

    case 1001: // ViruzII settings
      {
        CViruzIIConfig conf;
        conf.DoModal();
      }
      return 0;

    case 1002: // ViruzII hard reset
      g_graph->setTuneName(g_graph->getTuneName());
      return 0;

    case 1003: // Make Demo
      {
        frGraphExporter exporter;

        static const sInt xRes[]={0, 512, 640, 800, 1024};
        static const sInt yRes[]={0, 384, 480, 600, 768};
        const sInt res=m_resChooser.GetSelection();

        exporter.makeDemo(xRes[res], yRes[res]);
      }
      return 0;
    }

    bHandled=FALSE;
    return 0;
  }

  LRESULT OnSetFullscreenWindow(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    m_fsWindow=wParam;
    return 0;
  }

  LRESULT OnSetSubmode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    m_subMode=wParam;
    ToggleFullscreenPreview(m_fsWindow, m_editVisible);

    return 0;
  }

  LRESULT OnModuleChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
  {
    m_curModule=wID-ID_MODULE_0;
    ToggleFullscreenPreview(m_fsWindow, m_editVisible);

    return 0;
  }

  void ToggleFullscreenPreview(sInt whichOne, sInt forcedVisible=-1)
  {
    sInt vis;

    if (forcedVisible==-1)
    {
      m_editVisible=!m_editVisible;
      vis=m_editVisible;
    }
    else
      vis=forcedVisible;
    
    m_param.ShowWindow(vis ? SW_SHOW : SW_HIDE);
    m_page.ShowWindow((vis && m_curModule == 0) ? SW_SHOW : SW_HIDE);
    m_graph.ShowWindow(((whichOne == 2 || vis) && m_curModule == 0) ? SW_SHOW : SW_HIDE);
    m_preview.ShowWindow((whichOne == 0 || vis) ? SW_SHOW : SW_HIDE);
    m_3dpreview.ShowWindow((whichOne == 1 || vis) ? SW_SHOW : SW_HIDE);
    m_spline.ShowWindow(((whichOne == 2 || vis) && m_curModule == 1 && m_subMode == 0) ? SW_SHOW : SW_HIDE);
    m_animMixer.ShowWindow(((whichOne == 2 || vis) && m_curModule == 1 && m_subMode == 1)? SW_SHOW : SW_HIDE);
    m_timeline.ShowWindow(m_curModule == 1 ? SW_SHOW : SW_HIDE);
    UpdateDynamicLayout();
  }
  
  void DrawSplitterBar(CDCHandle dc, frlLayoutInfo *inf)
  {
    RECT rc;
    
    GetSplitterBarRect(inf, &rc);
    if (rc.right-rc.left>rc.bottom-rc.top) // horizontal splitter
    {
      dc.FillSolidRect(rc.left, rc.top+0, rc.right-rc.left, 1, RGB(181, 177, 176));
      dc.FillSolidRect(rc.left, rc.top+1, rc.right-rc.left, 1, RGB(150, 148, 145));
      dc.FillSolidRect(rc.left, rc.top+2, rc.right-rc.left, 1, RGB(122, 118, 115));
    }
    else // vertical
    {
      dc.FillSolidRect(rc.left+0, rc.top, 1, rc.bottom-rc.top, RGB(181, 177, 176));
      dc.FillSolidRect(rc.left+1, rc.top, 1, rc.bottom-rc.top, RGB(150, 148, 145));
      dc.FillSolidRect(rc.left+2, rc.top, 1, rc.bottom-rc.top, RGB(122, 118, 115));
    }
  }

  void DrawLeafEdge(CDCHandle dc, LPCRECT rc)
  {
    dc.Draw3dRect(rc->left, rc->top, rc->right-rc->left, rc->bottom-rc->top, RGB(103, 99, 96), RGB(200, 198, 196));
  }
  
  void DrawTitlePane(CDCHandle dc, frlLayoutInfo *inf)
  {
    CDC tempDC;
    tempDC.CreateCompatibleDC(dc);

    CRect rc=inf->m_rects[0];

    CBitmapHandle oldBitmap=tempDC.SelectBitmap(m_paneBitmap);
    dc.StretchBlt(rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, tempDC, 0, 0, 1, m_cxyHeader, SRCCOPY);
    tempDC.SelectBitmap(oldBitmap);

    rc.DeflateRect(2, 2);
    rc.OffsetRect(0, -2);

    sInt len=::GetWindowTextLength(inf->m_hWnd);
    TCHAR *text=new TCHAR[len+1];

    ::GetWindowText(inf->m_hWnd, text, len+1);
    text[len]=0;

    CFontHandle oldFont=dc.SelectFont(AtlGetDefaultGuiFont());
    rc.left+=m_cxyTextOffset;
    rc.right-=m_cxyTextOffset;
    
    dc.SetTextColor(RGB(0, 0, 0));
    dc.SetBkMode(TRANSPARENT);
    dc.DrawText(text, len, &rc, DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
    
    dc.SelectFont(oldFont);

    delete[] text;
  }
  
  void CalcHeaderSize()
  {
    SIZE sz;

    m_paneBitmap.GetSize(sz);
    m_cxyHeader=sz.cy;
  }

  void UpdateDynamicLayout()
  {
    if (m_lTLWork)
    {
      RECT rc;
      GetClientRect(&rc);

      SetSplitterPos(m_lTLWork, rc.bottom-3-60, sFALSE);
    }

    baseClass::UpdateDynamicLayout();
  }
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TG2VIEW_H__2579F428_F0EF_4358_94B0_41BE6BDBC008__INCLUDED_)

// This code is in the public domain. See LICENSE for details.

// PageWindow.cpp: implementation of the CPageWindow class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "PageWindow.h"
#include "WindowMapper.h"
#include "frOpGraph.h"
#include "resource.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPageWindow::CPageWindow()
{
  m_activeLVItem=-1;
}

CPageWindow::~CPageWindow()
{
}

// ---- message handling

LRESULT CPageWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_buttonImages.CreateFromImage(IDB_PAGEVIEW, 29, 0, CLR_NONE, IMAGE_BITMAP, LR_DEFAULTSIZE|LR_CREATEDIBSECTION);
  m_lvImages.CreateFromImage(IDB_PGLVSYMBOLS, 8, 0, CLR_DEFAULT, IMAGE_BITMAP);
  
  m_pageBox.Create(m_hWnd, rcDefault, 0, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|LVS_SMALLICON|LVS_AUTOARRANGE|LVS_EDITLABELS|LVS_SINGLESEL, 0);
  m_pageBox.SetFont(AtlGetStockFont(DEFAULT_GUI_FONT));
  m_pageBox.SetExtendedListViewStyle(LVS_EX_FLATSB|LVS_EX_LABELTIP);
  m_pageBox.SetImageList(m_lvImages, LVSIL_SMALL);
  m_pageBox.SetBkColor(RGB(174, 169, 167));
  m_pageBox.SetTextBkColor(RGB(174, 169, 167));
  m_pageBox.SetTextColor(RGB(0, 0, 0));

  m_addTexButton.Create(m_hWnd, CRect(2, 2, 31, 19), 0, WS_CHILD|WS_VISIBLE|FRBS_NOBORDER|FRBS_HOTTRACK|FRBS_BITMAP, 0, 100);
  m_addTexButton.SetImageList(m_buttonImages);
  m_addTexButton.SetImages(0, 0, 4);
  m_addTexButton.SetToolTipText("Add texture page");
  
  m_addGeoButton.Create(m_hWnd, CRect(31, 2, 60, 19), 0, WS_CHILD|WS_VISIBLE|FRBS_NOBORDER|FRBS_HOTTRACK|FRBS_BITMAP, 0, 101);
  m_addGeoButton.SetImageList(m_buttonImages);
  m_addGeoButton.SetImages(1, 1, 5);
  m_addGeoButton.SetToolTipText("Add geometry page");

  m_addCmpButton.Create(m_hWnd, CRect(60, 2, 89, 19), 0, WS_CHILD|WS_VISIBLE|FRBS_NOBORDER|FRBS_HOTTRACK|FRBS_BITMAP, 0, 102);
  m_addCmpButton.SetImageList(m_buttonImages);
  m_addCmpButton.SetImages(2, 2, 6);
  m_addCmpButton.SetToolTipText("Add compositing page");
  
  m_delButton.Create(m_hWnd, CRect(89, 2, 118, 19), 0, WS_CHILD|WS_VISIBLE|FRBS_NOBORDER|FRBS_HOTTRACK|FRBS_BITMAP, 0, 200);
  m_delButton.SetImageList(m_buttonImages);
  m_delButton.SetImages(3, 3, 7);
  m_delButton.SetToolTipText("Remove page");

  bHandled=FALSE;
  return 0;
}

LRESULT CPageWindow::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (HIWORD(wParam)==0)
  {
    switch (LOWORD(wParam))
    {
    case 100:
    case 101:
    case 102:
      {
        static sChar buf[24];
        const sChar *types[3]={"tex", "geo", "comp"};

        sprintf(buf, "%s page %d", types[LOWORD(wParam)-100], m_pageBox.GetItemCount()+1);
        AddPage(buf, LOWORD(wParam)-100);
      }
      break;

    case 200:
      DeletePage();
      break;

    default:
      bHandled=FALSE;
    }
  }

  return 0;
}

LRESULT CPageWindow::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CRect rc;
  
  GetClientRect(&rc);
  rc.top=22+3;
  m_pageBox.SetWindowPos(0, &rc, SWP_NOZORDER);
  
  bHandled=FALSE;
  return 0;
}

LRESULT CPageWindow::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  LPNMHDR nmh=(LPNMHDR) lParam;
  
  switch (nmh->code)
  {
  case NM_CLICK:
    {
      LPNMITEMACTIVATE nia=(LPNMITEMACTIVATE) lParam;
      LVHITTESTINFO inf;
      
      inf.pt=nia->ptAction;
      int item=m_pageBox.HitTest(&inf);
      
      if (item!=-1)
        SelectPage(item);
    }
    return 0;
    
  case NM_RCLICK:
    {
      LVHITTESTINFO hti;

      GetCursorPos(&hti.pt);
      m_pageBox.ScreenToClient(&hti.pt);
      
      sInt item=m_pageBox.HitTest(&hti);

      if (item!=-1)
        m_pageBox.EditLabel(item);
    };
    return 0;
    
  case LVN_ENDLABELEDIT:
    {
      NMLVDISPINFO *di=(NMLVDISPINFO *) lParam;
      
      if (di->item.pszText)
      {
        LVITEM lvi;
        
        lvi.mask=LVIF_PARAM;
        lvi.iItem=di->item.iItem;
        m_pageBox.GetItem(&lvi);

        frOpGraph::pgMapIt it=g_graph->m_pages.find(lvi.lParam);
        if (it!=g_graph->m_pages.end())
          it->second.name=di->item.pszText;
      }
    }
    return TRUE;
    
  case LVN_KEYDOWN:
    {
      LPNMLVKEYDOWN lkd=(LPNMLVKEYDOWN) lParam;
      
      if (lkd->wVKey==VK_DELETE || lkd->wVKey==VK_BACK)
        DeletePage();
    }
    return 0;
    break;
    
  default:
    bHandled=FALSE;
  }
  
  return 0;
}

LRESULT CPageWindow::OnRefreshPageView(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_pageBox.DeleteAllItems();
 
  sInt i=0, pg=0;
  
  for (frOpGraph::pgMapIt it=g_graph->m_pages.begin(); it!=g_graph->m_pages.end(); ++it)
  {
    m_pageBox.InsertItem(LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM, m_pageBox.GetItemCount(),
      it->second.name, 0, 0, 0, it->second.pageID);
    
    if (it->second.pageID==g_graph->m_curPage)
      pg=i;
    
    i++;
  }
  
  SelectPage(pg, sTRUE);
  
  return 0;
}

LRESULT CPageWindow::OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CDCHandle dc=(HDC) wParam;
  CRect rc;

  GetClientRect(&rc);
  rc.bottom=22;
  dc.FillSolidRect(&rc, RGB(103, 99, 96));

  rc.top=22; rc.bottom=23;
  dc.FillSolidRect(&rc, RGB(122, 118, 115)); rc.OffsetRect(0, 1);
  dc.FillSolidRect(&rc, RGB(150, 148, 145)); rc.OffsetRect(0, 1);
  dc.FillSolidRect(&rc, RGB(181, 177, 176)); 
  
  return 1;
}

// ---- tool functions

void CPageWindow::AddPage(const sChar *name, sInt type, sBool select)
{
  frOpGraph::frOpPage &pg=g_graph->addPage(type);

  pg.name=name;
  
  int nItem=m_pageBox.InsertItem(LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM, m_pageBox.GetItemCount(), pg.name,
    0, 0, 0, pg.pageID);

  g_graph->modified();
  
  if (select)
    SelectPage(nItem, sTRUE);
}

void CPageWindow::SelectPage(sInt nIndex, sBool force)
{
  LVITEM lvi;
  
  if (m_activeLVItem==nIndex && !force)
    return;
  
  lvi.mask=LVIF_IMAGE|LVIF_PARAM;
  if (m_activeLVItem!=-1)
  {
    lvi.iItem=m_activeLVItem;
    m_pageBox.GetItem(&lvi);
    lvi.iImage=0;
    m_pageBox.SetItem(&lvi);
  }
  
  lvi.iItem=nIndex;
  m_pageBox.GetItem(&lvi);
  lvi.iImage=1;
  m_pageBox.SetItem(&lvi);
  
  m_pageBox.SelectItem(nIndex);
  m_pageBox.UpdateWindow();
  
  ::SendMessage(g_winMap.Get(ID_GRAPH_WINDOW), WM_SETPAGE, lvi.lParam, 0);
  m_activeLVItem=nIndex;
}

void CPageWindow::DeletePage()
{
  CString str, pg;
  CString tstr;
  
  tstr.LoadString(IDR_MAINFRAME);
  
  if (m_pageBox.GetItemCount()>1 && m_activeLVItem!=-1)
  {
    m_pageBox.GetItemText(m_activeLVItem, 0, pg);
    str.Format("Really remove '%s' and all its contents?", pg);
    
    if (MessageBox(str, tstr, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2)==IDYES)
    {
      LVITEM lvi;
      lvi.mask=LVIF_PARAM;
      lvi.iItem=m_activeLVItem;
      m_pageBox.GetItem(&lvi);
      
      SendMessage(g_winMap.Get(ID_GRAPH_WINDOW), WM_DELETECURPAGE, 0, 0);

      g_graph->m_pages.erase(g_graph->m_pages.find(lvi.lParam));
      g_graph->modified();
      
      sInt cnt=m_pageBox.GetItemCount();
      m_pageBox.DeleteItem(m_activeLVItem);
      
      if (m_activeLVItem==cnt-1)
        SelectPage(m_activeLVItem-1, TRUE);
      else
        SelectPage(m_activeLVItem, TRUE);
    }
  }
  else if (m_activeLVItem==-1)
    MessageBox(_T("No page currently selected"), tstr, MB_ICONERROR|MB_OK);
  else
    MessageBox(_T("Can't remove last page"), tstr, MB_ICONERROR|MB_OK);
}

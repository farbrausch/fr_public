// This code is in the public domain. See LICENSE for details.

// PageWindow.h: interface for the CPageWindow class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PAGEWINDOW_H__AE432DC5_BD76_4209_A966_A6D5AA47034D__INCLUDED_)
#define AFX_PAGEWINDOW_H__AE432DC5_BD76_4209_A966_A6D5AA47034D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CPageWindow : public CWindowImpl<CPageWindow>  
{
public:
  DECLARE_WND_CLASS(0);
  
  CImageList    m_lvImages, m_buttonImages;
  CListViewCtrl m_pageBox;
  sInt          m_activeLVItem;
  frButton      m_addTexButton, m_addGeoButton, m_addCmpButton, m_delButton;

  CPageWindow();
	virtual ~CPageWindow();

  BEGIN_MSG_MAP(CPageWindow)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_COMMAND, OnCommand)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
    MESSAGE_HANDLER(WM_REFRESHPAGEVIEW, OnRefreshPageView)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
  END_MSG_MAP()

  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnRefreshPageView(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  
  void AddPage(const sChar *name, sInt type=0, sBool select=sTRUE);
  void SelectPage(sInt index, sBool force=sFALSE);
  void DeletePage();
};

#endif // !defined(AFX_PAGEWINDOW_H__AE432DC5_BD76_4209_A966_A6D5AA47034D__INCLUDED_)

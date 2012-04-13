// This code is in the public domain. See LICENSE for details.

// WindowMapper.h: interface for the CWindowMapper class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WINDOWMAPPER_H__AAFE0936_FB20_4229_BDBD_556ED8E2614B__INCLUDED_)
#define AFX_WINDOWMAPPER_H__AAFE0936_FB20_4229_BDBD_556ED8E2614B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CWindowMapper  
{
  struct window
  {
    sU32  id;
    HWND  hWnd;
  };

  window* m_windows;
  sInt    m_nWindows;

public:
	CWindowMapper();
	virtual ~CWindowMapper();

  void  Add(sU32 id, HWND hWnd);
  HWND  Get(sU32 id);
};

extern CWindowMapper  g_winMap;

#endif // !defined(AFX_WINDOWMAPPER_H__AAFE0936_FB20_4229_BDBD_556ED8E2614B__INCLUDED_)

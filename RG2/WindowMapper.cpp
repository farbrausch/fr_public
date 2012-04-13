// This code is in the public domain. See LICENSE for details.

// WindowMapper.cpp: implementation of the CWindowMapper class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "WindowMapper.h"

CWindowMapper g_winMap;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWindowMapper::CWindowMapper()
  : m_windows(0), m_nWindows(0)
{
}

CWindowMapper::~CWindowMapper()
{
  if (m_windows)
    free(m_windows);
}

void CWindowMapper::Add(sU32 id, HWND hWnd)
{
  m_windows = (window*) realloc(m_windows, ++m_nWindows * sizeof(window));
  m_windows[m_nWindows - 1].id = id;
  m_windows[m_nWindows - 1].hWnd = hWnd;
}

HWND CWindowMapper::Get(sU32 id)
{
  for (sInt i = 0; i < m_nWindows; i++)
  {
    if (m_windows[i].id == id)
      return m_windows[i].hWnd;
  }

  return 0;
}

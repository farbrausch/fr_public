// This code is in the public domain. See LICENSE for details.

// PreviewWindow.cpp: implementation of the CPreviewWindow class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "PreviewWindow.h"
#include "WindowMapper.h"
#include "resource.h"
#include "viewcomm.h"
#include "frOpGraph.h"
#include "plugbase.h"
#include "texbase.h"

extern LPCTSTR lpcstrTG2RegKey;

// ---- tools

static inline sInt clamp(sInt a, sInt mn, sInt mx)
{
  return (a < mn) ? mn : (a > mx) ? mx : a;
}

// ---- das client window

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPreviewClientWindow::CPreviewClientWindow()
{
  m_hWndSelect = 0;
  m_tiled = sFALSE;
  m_update = 0;
  m_scale1 = m_scale2 = 1;
	m_alphaMode = 0;
}

CPreviewClientWindow::~CPreviewClientWindow()
{
}

// ---- message processing

LRESULT CPreviewClientWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_curOp = 0;
  m_imageBuf = 0;
	m_update = sFALSE;

  m_crossCursor = AtlLoadSysCursor(IDC_CROSS);
  m_arrowCursor = AtlLoadSysCursor(IDC_ARROW);

  m_scrlX = m_scrlY = 0;
  m_scrlMaxX = m_scrlMaxY = 0;

  bHandled = FALSE;
  return 0;
}

LRESULT CPreviewClientWindow::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	free(m_imageBuf);

  bHandled = FALSE;
  return 0;
}

LRESULT CPreviewClientWindow::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	UpdateScrollSize(TRUE);

  bHandled = FALSE;
  return 0;
}

LRESULT CPreviewClientWindow::OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CDCHandle dc((HDC) wParam);
  CRect rc;
  COLORREF bgCol = RGB(127, 127, 127);

  if (!m_curOp)
  {
    GetClientRect(&rc);
    dc.FillSolidRect(&rc, bgCol);
  }
  else
  {
    GetClientRect(&rc);
    if (rc.right > ImageWidth())
    {
      rc.left = ImageWidth();
      dc.FillSolidRect(&rc, bgCol);
    }
    
    GetClientRect(&rc);
    if (rc.bottom > ImageHeight())
    {
      rc.top = ImageHeight();
      rc.right = ImageWidth();
      dc.FillSolidRect(&rc, bgCol);
    }
  }
  
  return 1;
}

LRESULT CPreviewClientWindow::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CPaintDC dc(m_hWnd);

  dc.SetViewportOrg(-m_scrlX, -m_scrlY);
  DoPaint(dc.m_hDC);

  return 0;
}

LRESULT CPreviewClientWindow::OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (LOWORD(lParam) == HTCLIENT)
  {
    if (m_hWndSelect && CursorHitTest(GetCurrentMessage()->pt))
      SetCursor(m_crossCursor);
    else
      SetCursor(m_arrowCursor);
  }
  else
    bHandled=FALSE;
  
  return 0;
}

LRESULT CPreviewClientWindow::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  sInt x, y;
  
  CalcXYPos(lParam, x, y);

  frTexture* curTex = getCurrentTex();

  if (m_hWndSelect && curTex && x < curTex->w && y < curTex->h)
    SendMessage(m_hWndSelect, WM_POINTSELECT, 0, MAKELONG(x, y));
  else
    bHandled = FALSE;

  m_lButtonDown = sTRUE;
  
  return 0;
}

LRESULT CPreviewClientWindow::OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  ::SendMessage(g_winMap.Get(IDR_MAINFRAME), WM_SET_POSITION_MESSAGE, 0, 0);
  
  m_lButtonDown = sFALSE;
  m_hWndSelect = 0;
  
  return 0;
}

LRESULT CPreviewClientWindow::OnMButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_dragOrigin = CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

  if (::GetCapture() != m_hWnd)
    SetCapture();
  
  return 0;
}

LRESULT CPreviewClientWindow::OnMButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (::GetCapture() == m_hWnd)
    ::ReleaseCapture();

  return 0;
}

LRESULT CPreviewClientWindow::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  sInt x, y;
  
  ::SendMessage(g_winMap.Get(ID_MAIN_VIEW), WM_SET_FS_WINDOW, 0, 0);
  
  CalcXYPos(lParam, x, y);
  frTexture* curTex = getCurrentTex();
  
  if (m_hWndSelect && curTex)
  {
    if (x < curTex->w && y < curTex->h)
      ::SendMessage(g_winMap.Get(IDR_MAINFRAME), WM_SET_POSITION_MESSAGE, 1, MAKELPARAM(x, y));
    else
      ::SendMessage(g_winMap.Get(IDR_MAINFRAME), WM_SET_POSITION_MESSAGE, 0, 0);
    
    if (wParam & MK_LBUTTON)
      ::SendMessage(m_hWndSelect, WM_POINTSELECT, 0, MAKELONG(x, y));
  }
  else if (wParam & MK_MBUTTON)
  {
    CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

    m_scrlX = clamp(m_scrlX - pt.x + m_dragOrigin.x, 0, m_scrlMaxX);
    m_scrlY = clamp(m_scrlY - pt.y + m_dragOrigin.y, 0, m_scrlMaxY);

    Invalidate();
    UpdateWindow();

    m_dragOrigin = pt;
  }
  else
    bHandled = FALSE;
  
  return 0;
}

LRESULT CPreviewClientWindow::OnControlColor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  return 0;
}

LRESULT CPreviewClientWindow::OnSetDisplayTexture(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if ((sU32) lParam != m_curOp)
  {
    m_curOp = (sU32) lParam;
    UpdateScrollSize(sFALSE);
  }
  
  m_update = sTRUE;

	Invalidate();
  ::InvalidateRect(GetParent(), 0, TRUE);
  ::UpdateWindow(GetParent());

  return 0;
}

LRESULT CPreviewClientWindow::OnPointSelect(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (wParam == 1 || wParam == 2)
  {
    m_hWndSelect = (HWND) lParam;
    m_clip = (wParam == 1);
  }
  
  return 0;
}

// ---- scrollbar handling

void CPreviewClientWindow::UpdateScrollSize(sBool redraw)
{
  if (!IsWindow())
    return;
  
  frTexture* curTex = getCurrentTex();
  
  if (curTex)
  {
    RECT rc;

    const sInt szx = ImageWidth();
    const sInt szy = ImageHeight();

    GetClientRect(&rc);

    m_scrlMaxX = szx - rc.right;	if (m_scrlMaxX < 0) m_scrlMaxX = 0;
    m_scrlMaxY = szy - rc.bottom; if (m_scrlMaxY < 0) m_scrlMaxY = 0;

    m_scrlX = clamp(m_scrlX, 0, m_scrlMaxX);
    m_scrlY = clamp(m_scrlY, 0, m_scrlMaxY);
  }
  else
  {
    m_scrlX = m_scrlY = 0;
    m_scrlMaxX = m_scrlMaxY = 0;
  }

  if (redraw)
  {
    Invalidate();
    UpdateWindow();
  }
}

// ---- gui helpers

BOOL CPreviewClientWindow::CursorHitTest(POINT pt) const
{
	const sInt x = pt.x + m_scrlX;
	const sInt y = pt.y + m_scrlY;
  
  return (x >= 0) && (y >= 0) && (x < ImageWidth()) && (y < ImageWidth());
}

// ---- image handling

sInt CPreviewClientWindow::ImageWidth() const
{
  frTexture* curTex = getCurrentTex();

	return MulDiv((curTex ? curTex->w : 0) * (m_tiled ? 3 : 1), m_scale1, m_scale2);
}

sInt CPreviewClientWindow::ImageHeight() const
{
  frTexture* curTex = getCurrentTex();

	return MulDiv((curTex ? curTex->h : 0) * (m_tiled ? 3 : 1), m_scale1, m_scale2);
}

sBool CPreviewClientWindow::UpdateImage()
{
  sBool ret = sFALSE;
    
  if (m_update)
  {
		frTexture* curTex = getCurrentTex();

    curTex->lock();
    ((frTexturePlugin *) g_graph->m_ops[m_curOp].plg)->process();
    
		const sU32 width = curTex->w;
		const sU32 height = curTex->h;
    const sU32 sz = width * height;

		m_imageBuf = (sU8 *) realloc(m_imageBuf, sz * 4);
    
		const sU16* src = curTex->data;
    void* dst = m_imageBuf;
		static const sU64 add1 = 0x3fff3fff3fff3fff;
		static const sU64 xor1 = 0x7fff7fff7fff7fff;

		switch (m_alphaMode)
		{
		case 0: // ignore alpha
			__asm
			{
				emms;
				mov       esi, [src];
				mov       edi, [dst];
				mov       ecx, [sz];
				lea				esi, [esi + ecx * 8];
				lea				edi, [edi + ecx * 4];
				neg				ecx;

simplelp:
				movq      mm0, [esi + ecx * 8];
				movq      mm1, [esi + ecx * 8 + 8];
				movq      mm2, [esi + ecx * 8 + 16];
				movq      mm3, [esi + ecx * 8 + 24];
				psrlw     mm0, 7;
				psrlw     mm1, 7;
				psrlw     mm2, 7;
				psrlw     mm3, 7;
				packuswb  mm0, mm1;
				packuswb  mm2, mm3;
				movq      [edi + ecx * 4], mm0;
				movq      [edi + ecx * 4 + 8], mm2;
				add				ecx, 4;
				jnz				simplelp;
				emms;
			}
			break;

		case 1: // photoshop-style checker
			__asm
			{
				emms;
				mov       esi, [src];
				mov       edi, [dst];

				mov				edx, [height];

ps_ylp:
				mov       ecx, [width];
				lea				esi, [esi + ecx * 8];
				lea				edi, [edi + ecx * 4];
				neg				ecx;

				pxor			mm2, mm2;

				mov				ebx, edx;
				and				ebx, 8;
				movd			mm5, ebx;

ps_xlp:
				movq      mm0, [esi + ecx * 8];
				movq			mm1, mm0;

				punpckhwd	mm1, mm1;
				punpckhwd	mm1, mm1;

				mov				ebx, ecx;
				and				ebx, 8;
				movd			mm4, ebx;
				pxor			mm4, mm5;
				punpcklwd	mm4, mm4;
				movq			mm3, mm1;
				punpcklwd	mm4, mm4;
				psllw			mm4, 10;
				pxor			mm3, [xor1];
				paddw			mm4, [add1];

				pmulhw		mm0, mm1;
				pmulhw		mm4, mm3;
				paddw			mm0, mm4;
				psrlw			mm0, 6;

				packuswb  mm0, mm2;
				movd      [edi + ecx * 4], mm0;
				inc				ecx;
				jnz				ps_xlp;

				dec				edx;
				jnz				ps_ylp;

				emms;
			}
			break;
		}

    curTex->unlock();
		ret = TRUE;
  }
  
  m_update = sFALSE;
  return ret;
}

void CPreviewClientWindow::CalcXYPos(const POINT pt, sInt &x, sInt &y)
{
  x = MulDiv(pt.x + m_scrlX, m_scale2, m_scale1);
  y = MulDiv(pt.y + m_scrlY, m_scale2, m_scale1);
  
  frTexture* curTex = getCurrentTex();
  
  if ((m_clip || m_tiled) && curTex)
  {
    sInt f = m_tiled ? 3 : 1;

    if (x < 0)							x = 0;
    if (x >= curTex->w * f)	x = (curTex->w * f) - 1;
    if (y < 0)              y = 0;
    if (y >= curTex->h * f)	y = (curTex->h * f) - 1;
  }
  
  if (m_tiled && curTex && x >= 0 && y >= 0 && x < curTex->w * 3 && y < curTex->h * 3)
  {
		x &= curTex->w - 1;
		y &= curTex->h - 1;
  }
}
  
void CPreviewClientWindow::CalcXYPos(LPARAM lParam, sInt &x, sInt &y)
{
  CalcXYPos(CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), x, y);
}
  
// ---- interfacing

void CPreviewClientWindow::SetTiled(sBool tiled)
{
  m_tiled = tiled;

	UpdateScrollSize(sTRUE);

  Invalidate();
  UpdateWindow();
}

void CPreviewClientWindow::SetScale(sInt scale1, sInt scale2)
{
  m_scale1 = scale1;
  m_scale2 = scale2;
    
	UpdateScrollSize(sTRUE);

  Invalidate();
  UpdateWindow();
}

void CPreviewClientWindow::SetAlphaMode(sInt mode)
{
	m_alphaMode = mode;
	m_update = sTRUE;

	Invalidate();
	UpdateWindow();
}

// ---- painting

void CPreviewClientWindow::DoPaint(CDCHandle dc)
{
  BITMAPINFOHEADER bmih;
  
  frTexture* curTex = getCurrentTex();
  
  if (!curTex)
    return;
  
	memset(&bmih, 0, sizeof(bmih));
  bmih.biSize = sizeof(BITMAPINFOHEADER);
  bmih.biWidth = curTex->w;
  bmih.biHeight = -curTex->h;
  bmih.biPlanes = 1;
  bmih.biBitCount = 32;
  bmih.biCompression = BI_RGB;
  
  if (UpdateImage())
    UpdateScrollSize(sTRUE);

	dc.SetStretchBltMode(COLORONCOLOR);

	// paint the sectors.
	for (sInt ys = 0; ys < (m_tiled ? 3 : 1); ys++)
	{
		for (sInt xs = 0; xs < (m_tiled ? 3 : 1); xs++)
		{
			const sInt xStart = MulDiv(xs * curTex->w, m_scale1, m_scale2);
			const sInt yStart = MulDiv(ys * curTex->h, m_scale1, m_scale2);

			if (m_scale1 == m_scale2)
				dc.SetDIBitsToDevice(xStart, yStart, curTex->w, curTex->h, 0, 0, 0, curTex->h, m_imageBuf, (BITMAPINFO*) &bmih, DIB_RGB_COLORS);
			else
				dc.StretchDIBits(xStart, yStart, MulDiv(curTex->w, m_scale1, m_scale2), MulDiv(curTex->h, m_scale1, m_scale2), 0, 0,
				curTex->w, curTex->h, m_imageBuf, (BITMAPINFO*) &bmih, DIB_RGB_COLORS, SRCCOPY);
		}
	}
}

// ---- utility

frTexture *CPreviewClientWindow::getCurrentTex() const
{
  if (g_graph->m_ops.find(m_curOp) != g_graph->m_ops.end())
    return ((frTexturePlugin *) g_graph->m_ops[m_curOp].plg)->getData();
  else
    return 0;
}

// ---- und das main window

BOOL CPreviewWindow::PreTranslateMessage(MSG* pMsg)
{
  if (pMsg->message == WM_SYSKEYDOWN && pMsg->wParam >= 0x31 && pMsg->wParam <= 0x35)
  {
    sInt key = pMsg->wParam;

    if (key >= 0x31 && key <= 0x34)
    {
      m_client.SetScale(pMsg->wParam - 0x30, 1);
      m_zoom.SetSelection(3 + pMsg->wParam - 0x30);
    }
    else
    {
      sBool tiled = !m_tile.GetCheck();

      m_tile.SetCheck(tiled);
      m_client.SetTiled(tiled);
    }

    return TRUE;
  }

  return FALSE;
}

LRESULT CPreviewWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CRegKey key;

  DWORD tiled = 0, zoom = 4, alpha = 1;
  if (key.Open(HKEY_CURRENT_USER, lpcstrTG2RegKey, KEY_READ) == ERROR_SUCCESS)
  {
    key.QueryDWORDValue(_T("TextureTiled"), tiled);
    key.QueryDWORDValue(_T("TextureZoom"), zoom);
	  key.QueryDWORDValue(_T("TextureAlpha"), alpha);
    key.Close();
  }
  
  m_client.Create(m_hWnd, rcDefault, 0, WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN, 0);

  CFontHandle fnt = AtlGetDefaultGuiFont();

  m_tile.Create(m_hWnd, CRect(2, 0, 36, 20), _T("tile"), WS_CHILD|WS_VISIBLE|FRBS_AUTO3D_SINGLE|FRBS_CHECK|FRBS_HOTTRACK|FRBS_SOLID|FRBS_AUTO, 0, 100);
  m_tile.SetFont(fnt);
  m_tile.SetCheck(tiled);
  m_tile.SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
    RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));
  m_client.SetTiled(tiled);

	if (zoom == 2)
		zoom++; // 1:3 is out

  m_zoom.Create(m_hWnd, CRect(40, 0, 100, 20), 0, WS_CHILD|WS_VISIBLE|FRBS_FLAT|FRBS_HOTTRACK, 0, 101);
  m_zoom.AddMenuItem(1, "1:4");
  m_zoom.AddMenuItem(3, "1:2");
  m_zoom.AddMenuItem(4, "1:1");
  m_zoom.AddMenuItem(5, "2:1");
  m_zoom.AddMenuItem(6, "3:1");
  m_zoom.AddMenuItem(7, "4:1");
  m_zoom.SetSelection(zoom);
  m_zoom.SetFont(fnt);
  m_zoom.SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
    RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));

	m_alpha.Create(m_hWnd, CRect(104, 0, 174, 20), 0, WS_CHILD|WS_VISIBLE|FRBS_FLAT|FRBS_HOTTRACK, 0, 102);
	m_alpha.AddMenuItem(1, "no alpha");
	m_alpha.AddMenuItem(2, "photoshop");
	m_alpha.SetSelection(alpha);
	m_alpha.SetFont(fnt);
	m_alpha.SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
		RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));
	m_client.SetAlphaMode(alpha - 1);
  
  static const sInt s1tab[] = { 1, 1, 1, 1, 2, 3, 4 };
  static const sInt s2tab[] = { 4, 3, 2, 1, 1, 1, 1 };
  m_client.SetScale(s1tab[zoom - 1], s2tab[zoom - 1]);
  
  CMessageLoop* pLoop = _Module.GetMessageLoop();
  pLoop->AddMessageFilter(this);

  m_bgBrush.CreateSolidBrush(RGB(174, 169, 167));
  
  bHandled = FALSE;
  return 0;
}

LRESULT CPreviewWindow::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CRegKey key;
  
  key.Create(HKEY_CURRENT_USER, lpcstrTG2RegKey);
  key.SetDWORDValue(_T("TextureTiled"), m_tile.GetCheck());
  key.SetDWORDValue(_T("TextureZoom"), m_zoom.GetSelection());
	key.SetDWORDValue(_T("TextureAlpha"), m_alpha.GetSelection());
  key.Close();
  
  CMessageLoop* pLoop = _Module.GetMessageLoop();
  pLoop->RemoveMessageFilter(this);

  m_bgBrush.DeleteObject();
  
  bHandled = FALSE;
  return 0;
}

LRESULT CPreviewWindow::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CRect rc;

  GetClientRect(&rc);
  rc.top=21;
  rc.DeflateRect(1, 1);
  m_client.SetWindowPos(0, &rc, SWP_NOZORDER);

  bHandled=FALSE;
  return 0;
}

LRESULT CPreviewWindow::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (LOWORD(wParam) == 100) // tile
  {
    sBool tiled = !!m_tile.GetCheck();
    m_client.SetTiled(tiled);
  }
  else if (HIWORD(wParam) == CBN_SELCHANGE)
  {
    if (LOWORD(wParam) == 101) // zoom
    {
      sInt s1tab[] = { 1, 1, 1, 1, 2, 3, 4 };
      sInt s2tab[] = { 4, 3, 2, 1, 1, 1, 1 };
      sInt opt = m_zoom.GetSelection() - 1;

      m_client.SetScale(s1tab[opt], s2tab[opt]);
    }
		else if (LOWORD(wParam) == 102) // alpha
			m_client.SetAlphaMode(m_alpha.GetSelection() - 1);
  }

  bHandled = FALSE;
  return 0;
}

LRESULT CPreviewWindow::OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CDCHandle dc = (HDC) wParam;
  RECT rc;

  GetClientRect(&rc);
  rc.bottom = rc.top + 21;
  dc.FillSolidRect(&rc, RGB(174, 169, 167));

  GetClientRect(&rc);
  rc.top += 21;
  dc.Draw3dRect(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, RGB(103, 99, 96), RGB(200, 199, 196));
  
  CFontHandle oldFont = dc.SelectFont(AtlGetDefaultGuiFont());
  dc.SetTextColor(RGB(0, 0, 0));
  dc.SetBkMode(TRANSPARENT);

  const sChar *desc = "(no view item)";
  frOpGraph::opMapIt it = g_graph->m_ops.find(m_client.m_curOp);
  if (it != g_graph->m_ops.end())
    desc = it->second.plg->getDisplayName();

  dc.TextOut(176, 3, desc);
  dc.SelectFont(oldFont);
  
  return 1;
}

LRESULT CPreviewWindow::OnControlColor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CDCHandle dc = (HDC) wParam;

  dc.SetBkColor(RGB(174, 169, 167));
  dc.SetTextColor(RGB(0, 0, 0));

  return (LRESULT) ((HBRUSH) m_bgBrush);
}

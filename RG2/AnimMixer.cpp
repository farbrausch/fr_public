// This code is in the public domain. See LICENSE for details.

// AnimMixer.cpp: implementation of the CAnimMixer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AnimMixer.h"
#include "viewcomm.h"
#include "WindowMapper.h"
#include "resource.h"
#include "animbase.h"
#include "frOpGraph.h"
#include <vector>
#include <algorithm>
#include "debug.h"
#include "ViruzII.h"
#include "ruleMouseInput.h"
#include "guitool.h"

// ---- CAnimMixer

CAnimMixer::CAnimMixer()
	: m_lbDown(sFALSE), m_mbDown(sFALSE), m_moveElement(-1), m_curElem(-1), m_curFrame(0), m_timeBarPos(-0x7fffffff), m_flags(0),
	m_startPixel(0), m_thisClip(0)
{
}

CAnimMixer::~CAnimMixer()
{
}

// ---- message handling

LRESULT CAnimMixer::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_frameStep = 1.0f / 8.0f;

  const sChar* toggles[]={"Snap Left", "Snap Mid", "Snap Right", "Follow mode", 0}; // match this with bit order in flags
  for (m_cmdButtonCount = 0; toggles[m_cmdButtonCount]; m_cmdButtonCount++);

  CDCHandle dc = GetDC();
  CFontHandle oldFont = dc.SelectFont(AtlGetDefaultGuiFont());

  m_cmdButtons = new frButton[m_cmdButtonCount];

  sInt xpos = 4;
  for (sInt i = 0; i < m_cmdButtonCount; i++)
  {
    SIZE sz;
    dc.GetTextExtent(toggles[i], -1, &sz);

    const sInt width = sz.cx + 8;

    m_cmdButtons[i].Create(m_hWnd, CRect(xpos, 2, xpos+width, 22), toggles[i],
			WS_CHILD | WS_VISIBLE | FRBS_AUTO3D_SINGLE | FRBS_CHECK | FRBS_HOTTRACK | FRBS_SOLID | FRBS_AUTO, 0, i + 100);
    m_cmdButtons[i].SetFont(AtlGetDefaultGuiFont());
    m_cmdButtons[i].SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
      RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));
		m_cmdButtons[i].SetCheck(m_flags & (1 << i));

    xpos += width + 1;
  }

  dc.SelectFont(oldFont);
  ReleaseDC(dc);

  bHandled = FALSE;
  return 0;
}

LRESULT CAnimMixer::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  for (sInt i = 0; i < m_cmdButtonCount; i++)
    m_cmdButtons[i].DestroyWindow();

  FRSAFEDELETEA(m_cmdButtons);

  bHandled=FALSE;
  return 0;
}

LRESULT CAnimMixer::OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	drawBar(1);

  CDCHandle dc = (HDC) wParam;

	// prepare...
	CRect rc;
  GetClientRect(&rc);

	CFontHandle oldFont = dc.SelectFont(AtlGetDefaultGuiFont());
	dc.SetTextColor(RGB(0, 0, 0));
	dc.SetBkMode(TRANSPARENT);

	CPen blackPen;
	blackPen.CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	CPenHandle oldPen = dc.SelectPen(blackPen);

	// fill background
	dc.FillSolidRect(&rc, RGB(174, 169, 167));

	// draw tl bar at y 45
	dc.MoveTo(mapTime(0), 45);
	dc.LineTo(rc.right, 45);

	// track grid
	CPen pen_grid;
	pen_grid.CreatePen(PS_DOT, 1, RGB(174*3/4, 169*3/4, 167*3/4)^RGB(174, 169, 167));

	dc.SelectPen(pen_grid);
	dc.SetROP2(R2_XORPEN);
	dc.SetBkColor(RGB(0, 0, 0));
	dc.SetBkMode(TRANSPARENT);

	for (sInt trackY=46+22; trackY<rc.bottom; trackY+=22)
	{
		dc.MoveTo(mapTime(0), trackY);
		dc.LineTo(rc.right, trackY);
	}

	// bar marks
	dc.SetROP2(R2_COPYPEN);
	dc.SelectPen(blackPen);

	const sF32 xstep = 60000.0f / g_graph->m_bpmRate;
	const sF32 ixstep = g_graph->m_bpmRate / (60000.0f * m_frameStep);
	const sInt xms = (m_startPixel) * ixstep;
	const sInt xme = (m_startPixel + rc.right) * ixstep;

	LOGFONT lf;
	((CFontHandle) AtlGetDefaultGuiFont()).GetLogFont(lf);

	if (lf.lfHeight<0)
		lf.lfHeight=MulDiv(-lf.lfHeight, dc.GetDeviceCaps(LOGPIXELSY), 72);

	for (sInt i=fr::maximum(xms-1,0); i<=xme+1; i++)
	{
		const sInt xpos=mapTime(i*xstep);

		const sInt beat=i&3;

		dc.MoveTo(xpos, 45);
		dc.LineTo(xpos, 37-(beat==0)*2);

		TCHAR buf[32];
		sprintf(buf, "%d.%d", i/4, beat);

		dc.SetTextAlign(TA_BOTTOM|(i?TA_CENTER:TA_LEFT));
		dc.TextOut(xpos, 35, buf);

		if (!beat)
		{
			dc.MoveTo(xpos, rc.bottom);
			dc.LineTo(xpos, 45);
		}
	}

	dc.SelectPen(oldPen);
	dc.SelectFont(oldFont);

  return 1;
}

LRESULT CAnimMixer::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CPoint ptCursor(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

  if (::GetCapture() != m_hWnd)
  {
    m_oldFocusWin = ::GetFocus();
    
    SetCapture();
    SetFocus();

    m_useRuleMouse = ruleMouseSetTarget(m_hWnd);
    sInt buttons, dx, dy;
    ruleMousePoll(buttons, dx, dy); // to clear accumulated deltas

    m_ruleMouseCoord = ptCursor;
  }
  else
  {
    CRect rc;
    GetClientRect(&rc);
    rc.top += 40;
    
    if (!::PtInRect(&rc, ptCursor) && !m_lbDown && !m_mbDown)
    {
      ::ReleaseCapture();
      ::SetFocus(m_oldFocusWin);
      if (ruleMouseGetTarget() == (void *) m_hWnd)
        ruleMouseSetTarget(0);
    }
  }
  
  if (m_lbDown) // Move
    processMove(ptCursor.x, ptCursor.y);

  if (m_mbDown && !m_useRuleMouse) // Scroll
    processScroll(ptCursor.x);

  return 0;
}

LRESULT CAnimMixer::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	RECT rcElem;

	updateThisClip();

  switch (wParam)
  {
  case VK_SPACE:
    {
      CMenu dragMenu;
      dragMenu.CreatePopupMenu();

      typedef std::vector<fr::string> stringVec;
      typedef stringVec::iterator stringVecIt;
      typedef stringVec::const_iterator stringVecCIt;
      
      stringVec clipNames;
      for (frClipContainer::nameMapCIt nit = g_graph->m_clips->m_names.begin(); nit != g_graph->m_clips->m_names.end(); ++nit)
        clipNames.push_back(nit->first);

      std::sort(clipNames.begin(), clipNames.end());

      for (sInt i = 0; i < clipNames.size(); i++)
      {
        if (clipNames[i] != m_thisClip->getName() && clipNames[i] != fr::string("Timeline"))
          dragMenu.AppendMenu(MF_STRING, i + 1, clipNames[i]);
      }

      CPoint ptCursor;
      ::GetCursorPos(&ptCursor);
 
      sInt res = dragMenu.TrackPopupMenu(TPM_CENTERALIGN | TPM_RETURNCMD | TPM_VCENTERALIGN | TPM_NONOTIFY,
        ptCursor.x, ptCursor.y, m_hWnd);

      if (res) // clip selected
      {
        const fr::string selClipName = clipNames[res - 1];
				ScreenToClient(&ptCursor);

				const sInt startFrame = (m_startPixel + ptCursor.x) / m_frameStep; 
				const sInt trackNum = (ptCursor.y - 48) / 22;

        m_thisClip->addElement(startFrame,
					fr::minimum(trackNum, getMaxTrackNumber() + 1),	selClipName);

				const sInt lastElem = m_thisClip->m_elements.size() - 1;
				m_thisClip->m_elements.back().relStart = performSnap(lastElem, startFrame);
				getElemRect(lastElem, rcElem);

				clipChanged();

        InvalidateRect(&rcElem, FALSE);
        UpdateWindow();
      }
    }
    break;

  case VK_BACK:
  case VK_DELETE:
		getElemRect(m_curElem, rcElem);
    m_thisClip->deleteElement(m_curElem);

		if (m_moveElement == m_curElem)
			m_moveElement = -1;

    m_curElem = -1;
    clipChanged();

    InvalidateRect(&rcElem, TRUE);
    UpdateWindow();
    break;

  default:
    bHandled = FALSE;
  }

  return 0;
}

LRESULT CAnimMixer::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	drawBar(1);

	CPaintDC dc(m_hWnd);

	// layout elements
	updateThisClip();

	CFontHandle oldFont = dc.SelectFont(AtlGetDefaultGuiFont());
	dc.SetTextColor(RGB(255, 255, 255));
	dc.SetTextAlign(TA_TOP|TA_LEFT);
	dc.SetBkMode(TRANSPARENT);

	if (m_thisClip)
	{
		for (sInt i = 0; i < m_thisClip->m_elements.size(); ++i)
		{
			frAnimationClip::element& elem = m_thisClip->m_elements[i];

			CRect rc;
			getElemRect(i, rc);

			dc.Draw3dRect(&rc, RGB(0, 0, 0), RGB(0, 0, 0));
			rc.DeflateRect(1, 1);
			dc.Draw3dRect(&rc, (m_curElem == i) ? RGB(4,78,51) : RGB(75,249,186), (m_curElem == i) ? RGB(75,249,186) : RGB(4, 78, 51));
			rc.DeflateRect(1, 1);
			dc.FillSolidRect(&rc, RGB(6, 158, 106));

			fr::string clipName;

			if (elem.type == 0) // curve
				clipName = "(curve)";
			else
			{
				const frAnimationClip* clp = g_graph->m_clips->getClip(elem.id);

				if (clp)
					clipName = clp->getName();
				else
					clipName = "(invalid)";
			}

			dc.DrawText(clipName, -1, &rc, DT_VCENTER|DT_CENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
		}
	}
	
	dc.SelectFont(oldFont);
	drawBar(2);

  return 0;
}

LRESULT CAnimMixer::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  GetClientRect(&m_rcDraw);
  
  m_rcDraw.top = 40;
  m_rcDraw.left += 4;
  m_rcDraw.right -= 4;
  
  bHandled=FALSE;
  return 0;
}

LRESULT CAnimMixer::OnLButton(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_lbDown = (uMsg == WM_LBUTTONDOWN);  
  CPoint ptCursor(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

	updateThisClip();

  if (m_lbDown)
  {
    const sInt refFrame = (m_startPixel + ptCursor.x - m_rcDraw.left) / m_frameStep;
    const sInt track = (ptCursor.y - 48) / 22;

    m_moveElement = -1;

    if (track >= 0)
    {
      for (sInt i = m_thisClip->m_elements.size() - 1; i >= 0; --i)
      {
        frAnimationClip::element& elem = m_thisClip->m_elements[i];

        if (track == elem.trackID && refFrame >= elem.relStart && refFrame < (elem.relStart + elem.len))
        {
					RECT oldElemRect, newElemRect;
					getElemRect(m_curElem, oldElemRect);
					getElemRect(i, newElemRect);

          m_moveElement = i;
          m_curElem = i;

          m_baseRelStart = elem.relStart;
          m_baseTrack = elem.trackID;
          m_dragPoint = ptCursor;
          m_maxTrack = getMaxTrackNumber() + 1;

          InvalidateRect(&oldElemRect, FALSE);
					InvalidateRect(&newElemRect, FALSE);
          UpdateWindow();

          break;
        }
      }
    }
  }

  bHandled = FALSE;
  return 0;
}

LRESULT CAnimMixer::OnMButton(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_mbDown = (uMsg == WM_MBUTTONDOWN);
  CPoint ptCursor(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
  
  if (m_mbDown)
    m_scrlPoint = m_useRuleMouse ? m_ruleMouseCoord : ptCursor;
  
  bHandled = FALSE;
  return 0;
}

LRESULT CAnimMixer::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	const sInt index = LOWORD(wParam) - 100;

	if (index >= 0 && index < 31)
	{
		if (index < 3)
		{
			for (sInt j = 0; j < 3; j++)
			{
				if (j != index)
				{
					m_cmdButtons[j].SetCheck(0);
					m_flags &= ~(1 << j);
				}
			}
		}

		m_flags ^= 1 << index;
	}

  bHandled = FALSE;
  return 0;
}

LRESULT CAnimMixer::OnFrameChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (m_flags & 8) // follow mode
	{
		RECT rc;
		sInt mapped = mapTime(m_curFrame);

		GetClientRect(&rc);

		if (mapped >= rc.right * 7 / 8) // need to scroll right
			processScroll(mapped - rc.right / 8, sTRUE);
		else if (mapped < 0) // need to scroll left
			processScroll(mapped - rc.right * 7 / 8, sTRUE);
	}

	m_curFrame = lParam;
	drawBar();
  
  return 0;
}

LRESULT CAnimMixer::OnRuleMouseInput(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  sInt buttons, dx, dy;

  ruleMousePoll(buttons, dx, dy);

  m_ruleMouseCoord.x += dx;
  m_ruleMouseCoord.y += dy;

  if (m_mbDown)
    processScroll(m_ruleMouseCoord.x);

  return 0;
}

LRESULT CAnimMixer::OnShowWindow(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (!wParam && ::GetCapture() == m_hWnd)
    ReleaseCapture();

  bHandled = FALSE;
  return 0;
}

// ---- tools

sInt CAnimMixer::mapTime(sF32 frame) const
{
  return frame * m_frameStep + m_rcDraw.left - m_startPixel;
}

void CAnimMixer::clipChanged()
{
	updateThisClip();

  frAnimationClip::element moveElem, currentElem;

  if (m_moveElement != -1)
	{
		FRASSERT(m_moveElement < m_thisClip->m_elements.size());
    moveElem = m_thisClip->m_elements[m_moveElement];
	}

  if (m_curElem != -1)
	{
		FRASSERT(m_curElem < m_thisClip->m_elements.size());
    currentElem = m_thisClip->m_elements[m_curElem];
	}

  if (m_thisClip)
    std::stable_sort(m_thisClip->m_elements.begin(), m_thisClip->m_elements.end());

  if (m_moveElement != -1)
  {
    sInt i;
    for (i = 0; i < m_thisClip->m_elements.size() && m_thisClip->m_elements[i] != moveElem; i++);
    m_moveElement = i;
  }

  if (m_curElem != -1)
  {
    sInt i;
    for (i = 0; i < m_thisClip->m_elements.size() && m_thisClip->m_elements[i] != currentElem; i++);
    m_curElem = i;
  }
  
  ::PostMessage(g_winMap.Get(ID_TIMELINE_WINDOW), WM_CLIP_CHANGED, 0, 0);
  g_graph->modified();
}

void CAnimMixer::drawBar(sInt mode)
{
	CDCHandle dc = GetDC();
  CPen timePen;

  timePen.CreatePen(PS_SOLID, 3, RGB(255, 0, 0)^RGB(128, 128, 128));
  CPenHandle oldPen = dc.SelectPen(timePen);
  int op = dc.SetROP2(R2_XORPEN);

  CRect rc;
  GetClientRect(&rc);
  rc.left += 4;
  rc.top += 45;
  rc.right -= 4;

	if (mode != 2)
	{
		if (m_timeBarPos != -0x7fffffff)
		{
			dc.MoveTo(m_timeBarPos, rc.bottom - 1);
			dc.LineTo(m_timeBarPos, rc.top + 1);
			m_timeBarPos = -0x7fffffff;
		}
	}
  
	if (mode != 1)
	{
		m_timeBarPos = mapTime(m_curFrame);
		
		dc.MoveTo(m_timeBarPos, rc.bottom - 1);
		dc.LineTo(m_timeBarPos, rc.top + 1);
	}
  
  dc.SelectPen(oldPen);
  dc.SetROP2(op);

	ReleaseDC(dc);
}

void CAnimMixer::processMove(sInt cursorX, sInt cursorY)
{
	updateThisClip();

  if (m_thisClip && m_moveElement != -1)
  {
    sInt nsFrame = fr::maximum(0.0f, m_baseRelStart + (cursorX - m_dragPoint.x) / m_frameStep);
    sInt nTrack = fr::clamp(m_baseTrack + sInt(cursorY - m_dragPoint.y) / 22, 0, m_maxTrack);

    frAnimationClip::element& elem = m_thisClip->m_elements[m_moveElement];
		nsFrame = performSnap(m_moveElement, nsFrame);

		if (nsFrame != elem.relStart || nTrack != elem.trackID)
		{
			elem.updateLen();

			CRect rc1, rc2;
			getElemRect(m_moveElement, rc1);

			elem.relStart = nsFrame;
			elem.trackID = nTrack;

			getElemRect(m_moveElement, rc2);

			CRgn rgn1, rgn2;
			rgn1.CreateRectRgnIndirect(&rc1);
			rgn2.CreateRectRgnIndirect(&rc2);

			rgn1.CombineRgn(rgn1, rgn2, RGN_DIFF);
			InvalidateRgn(rgn1, TRUE); 
			UpdateWindow(); // erasebackground is a per-region flag, so we need to paint twice
			InvalidateRect(&rc2, FALSE);
			UpdateWindow();

			clipChanged();
		}
  }
}

void CAnimMixer::processScroll(sInt cursorX, sBool scrollDirect)
{
	sInt oldStartPixel = m_startPixel;

	if (scrollDirect)
		m_startPixel = fr::maximum(0, m_startPixel + cursorX);
	else
	{
		sInt scrollDelta = m_scrlPoint.x - cursorX;
		if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
			scrollDelta *= 8;

		m_startPixel = fr::maximum(0, m_startPixel + scrollDelta);
		m_scrlPoint.x = cursorX;
	}

  RECT rc;
  GetClientRect(&rc);
	rc.top = 24; // where the buttons end

  ScrollWindow(oldStartPixel - m_startPixel, 0, &rc, &rc);
	m_timeBarPos += oldStartPixel - m_startPixel;

  UpdateWindow();
}

void CAnimMixer::updateThisClip()
{
	const sU32 thisClipID = ::SendMessage(g_winMap.Get(ID_TIMELINE_WINDOW), WM_GET_CLIP_ID, 0, 0);
	frAnimationClip* newClip = g_graph->m_clips->getClip(thisClipID);

	if (newClip != m_thisClip)
	{
		m_thisClip = newClip;
		m_moveElement = -1;
		m_curElem = -1;
	}
}

void CAnimMixer::getElemRect(sInt elemIndex, RECT& rc) const
{
	frAnimationClip::element& elem = m_thisClip->m_elements[elemIndex];

	elem.updateLen();
	rc.left = mapTime(elem.relStart);
	rc.right = mapTime(elem.relStart + elem.len);
	rc.top = elem.trackID * 22 + 46;
	rc.bottom = (elem.trackID + 1) * 22 + 46;
}

sInt CAnimMixer::getMaxTrackNumber() const
{
	sInt maxTrack = -1;
	for (sInt i = 0; i < m_thisClip->m_elements.size(); ++i)
		maxTrack = fr::maximum(maxTrack, m_thisClip->m_elements[i].trackID);

	return maxTrack;
}

sInt CAnimMixer::performSnap(sInt elemIndex, sInt framePos) const
{
	if (!(m_flags & 7)) // no snap activated?
		return framePos; // no processing, then

	frAnimationClip::element& elem = m_thisClip->m_elements[elemIndex];
	m_thisClip->updateLength();

	sInt snapPoint = framePos;
	sInt snapDelta = 0;

	if (m_flags & 2) // snap mid?
		snapDelta = elem.len / 2;
	else if (m_flags & 4) // snap right?
		snapDelta = elem.len;

	snapPoint += snapDelta;

	// snap to nearest beat
	const sF32 beatf = 60000.0f / g_graph->m_bpmRate;

	sInt bp = snapPoint * g_graph->m_bpmRate / 60000.0f;
	sInt bs1 = bp * beatf;
	sInt bs2 = (bp + 1) * beatf;

	if (abs(bs1 - snapPoint) < abs(bs2 - snapPoint))
		snapPoint = bs1;
	else
		snapPoint = bs2;
	
	return fr::maximum(0, snapPoint - snapDelta);
}

// This code is in the public domain. See LICENSE for details.

// SplineEdit.cpp: implementation of the CSplineEdit class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SplineEdit.h"
#include "bezier.h"
#include "curve.h"
#include "animbase.h"
#include "frOpGraph.h"
#include "WindowMapper.h"
#include "viewcomm.h"
#include "resource.h"
#include "viruzII.h"
#include "guitool.h"
#include <math.h>

static COLORREF curveColor[8] = { 0x0000ff, 0x00ff00, 0xff0000, 0x00ffff, 0xff00ff, 0xffff00, 0x000000, 0xffffff };

CSplineEdit::CSplineEdit()
{
  m_lbDown=sFALSE;
  m_rbDown=sFALSE;
  m_mbDown=sFALSE;
  m_updateDraw=sFALSE;
  m_moveKey=-1;
  m_curKey=-1;
  m_curCurve=-1;
  
  m_mappedX=125.0f*40.0f;
  m_mappedY=20.0f;
  m_startX=0.0f;
  m_startY=0.5f*m_mappedY;

  m_curvOpID=0;
  m_curvCurveID=0;
  m_curves=0;

  m_barPos=-1;
  m_curFrame=-1;
}

CSplineEdit::~CSplineEdit()
{
}

// ---- message handling

LRESULT CSplineEdit::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  sInt i;
  for (i = 0; i < 3; i++)
  {
    m_curveShow[i].Create(m_hWnd, rcDefault, _T(""), WS_CHILD|FRBS_AUTO|FRBS_AUTO3D_SINGLE|FRBS_SOLID|FRBS_CHECK|FRBS_HOTTRACK, 0, 400+i);
    m_curveShow[i].SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
      curveColor[i], RGB(174,169,167), RGB(200, 198, 196));
    m_curveShow[i].SetCheck(sTRUE);
  }

  TCHAR *types[3]={ _T("Constant"), _T("Linear"), _T("Catmull-Rom") };

  for (i=0; i<3; i++)
  {
    m_types[i].Create(m_hWnd, CRect((i+1)*80, 2, (i+2)*80, 22), types[i],
      WS_CHILD|WS_VISIBLE|FRBS_AUTO|FRBS_AUTO3D_SINGLE|FRBS_HOTTRACK|FRBS_SOLID|FRBS_CHECK, 0, 100+i);
    m_types[i].SetFont(AtlGetDefaultGuiFont());
    m_types[i].SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
      RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));
  }

  m_snapFLast.Create(m_hWnd, CRect(i*80+90, 2, i*80+190, 22), _T("Snap First->Last"),
    WS_CHILD|WS_VISIBLE|FRBS_AUTO|FRBS_AUTO3D_SINGLE|FRBS_HOTTRACK|FRBS_SOLID, 0, 200);
  m_snapFLast.SetFont(AtlGetDefaultGuiFont());
  m_snapFLast.SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
    RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));

  m_snapLFirst.Create(m_hWnd, CRect(i*80+190, 2, i*80+290, 22), _T("Snap Last->First"),
    WS_CHILD|WS_VISIBLE|FRBS_AUTO|FRBS_AUTO3D_SINGLE|FRBS_HOTTRACK|FRBS_SOLID, 0, 201);
  m_snapLFirst.SetFont(AtlGetDefaultGuiFont());
  m_snapLFirst.SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
    RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));

  m_snapValue.Create(m_hWnd, CRect(i*80+290, 2, i*80+370, 22), _T("Snap Value"),
    WS_CHILD|WS_VISIBLE|FRBS_AUTO|FRBS_AUTO3D_SINGLE|FRBS_HOTTRACK|FRBS_SOLID|FRBS_CHECK, 0, 300);
  m_snapValue.SetFont(AtlGetDefaultGuiFont());
  m_snapValue.SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
    RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));
  m_snapValue.SetCheck(0);

  m_snapTime.Create(m_hWnd, CRect(i*80+370, 2, i*80+450, 22), _T("Snap Time"),
    WS_CHILD|WS_VISIBLE|FRBS_AUTO|FRBS_AUTO3D_SINGLE|FRBS_HOTTRACK|FRBS_SOLID|FRBS_CHECK, 0, 301);
  m_snapTime.SetFont(AtlGetDefaultGuiFont());
  m_snapTime.SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
    RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));
  m_snapTime.SetCheck(0);
  
  UpdateCurveType();

  bHandled=FALSE;
  return 0;
}

LRESULT CSplineEdit::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  bHandled=FALSE;
  return 0;
}

LRESULT CSplineEdit::OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  return 1;
}

LRESULT CSplineEdit::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CPaintDC dc(m_hWnd);
  CDC bufDC;

  bufDC.CreateCompatibleDC(dc);
  CBitmapHandle oldBitmap = bufDC.SelectBitmap(m_bufBitmap);

  CRect rc;
  GetClientRect(&rc);
  SetDrawRect(rc);

  if (m_bufferOld || !m_updateDraw)
  {
    bufDC.FillSolidRect(&rc, RGB(174, 169, 167));
    DrawGrid(bufDC.m_hDC);

    m_bufferOld=sFALSE;
  }

  updateCurvesPtr();

  if (m_curves)
  {
    CDC tempDC;
    tempDC.CreateCompatibleDC(dc);
    CBitmapHandle oldBitmap = tempDC.SelectBitmap(m_bufBitmap2);

    tempDC.BitBlt(0, 0, rc.right, rc.bottom, bufDC, 0, 0, SRCCOPY);

    DrawCurves(tempDC.m_hDC, *m_curves);
    dc.BitBlt(0, 0, rc.right, rc.bottom, tempDC, 0, 0, SRCCOPY);

    tempDC.SelectBitmap(oldBitmap);
  }
  else
    dc.BitBlt(0, 0, rc.right, rc.bottom, bufDC, 0, 0, SRCCOPY);

  bufDC.SelectBitmap(oldBitmap);

  m_barPos=-1;
  drawBar(dc.m_hDC);

  return 0;
}

LRESULT CSplineEdit::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CPoint  ptCursor(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
  sBool   redraw=sFALSE;

	SetFocus();
  updateCurvesPtr();

  if (m_lbDown && m_moveKey!=-1 && m_curCurve!=-1 && m_curves && m_curveShow[m_curCurve].GetCheck())
  {
    fr::fCurve      *curve=m_curves->getCurve(m_curCurve);
    fr::fCurve::key *curKey=curve->getKey(m_moveKey);
    const CPoint    delta=ptCursor-m_dragPoint;

    sInt newFrame=m_origFrame+delta.x*m_mappedX/(m_rcDraw.right-m_rcDraw.left);
    if (newFrame<m_minFrame)
      newFrame=m_minFrame;

    if (m_maxFrame>=m_minFrame && newFrame>m_maxFrame)
      newFrame=m_maxFrame;

    sF32 newValue=m_origValue-delta.y*m_mappedY/(m_rcDraw.bottom-m_rcDraw.top);

    if (m_snapTime.GetCheck())
    {
      const sF32 beatStep=(60000.0f/8.0f)/g_graph->m_bpmRate;
      const sInt bp=newFrame/beatStep;
      const sInt bf1=bp*beatStep, bf2=(bp+1)*beatStep;

      if (abs(bf1-newFrame)<abs(bf2-newFrame))
        newFrame=bf1;
      else
        newFrame=bf2;
    }

    if (m_snapValue.GetCheck())
    {
      const sF32 ystep=gridMapStep(m_mappedY, (m_rcDraw.bottom-m_rcDraw.top)/30, 10)*0.1f;
      const sInt vp=newValue/ystep;
      sF32 vn, bd;

      vn=(vp-1)*ystep;
      bd=abs(vn-newValue);
      for (sInt i=0; i<=1; i++)
      {
        const sF32 tn=(vp+i)*ystep;
        const sF32 d=abs(tn-newValue);
        if (d<bd)
        {
          bd=d;
          vn=tn;
        }
      }

      newValue=vn;
    }

    curKey->frame=newFrame;
    curKey->value=newValue;

    curve->keyModified(m_moveKey);
    ::PostMessage(g_winMap.Get(ID_TIMELINE_WINDOW), WM_CLIP_CHANGED, 0, 0);

    g_graph->modified();

    redraw=sTRUE;
  }

  if (m_rbDown)
  {
    const CPoint delta=ptCursor-m_sizePoint;

    m_mappedX=m_origMappedX*exp(-delta.x*0.01f);
    m_mappedY=m_origMappedY*exp(delta.y*0.01f);
    m_startY=m_midY+m_mappedY*0.5f;

    m_bufferOld=sTRUE;
    redraw=sTRUE;
  }

  if (m_mbDown)
  {
    const CPoint delta=ptCursor-m_scrlPoint;

    m_startX-=delta.x*m_mappedX/(m_rcDraw.right-m_rcDraw.left);
    m_startY+=delta.y*m_mappedY/(m_rcDraw.bottom-m_rcDraw.top);

    m_scrlPoint=ptCursor;

    if (m_startX<0)
      m_startX=0;

    m_bufferOld=sTRUE;
    redraw=sTRUE;
  }

  if (redraw)
  {
    m_updateDraw=sTRUE;
    Invalidate();
    UpdateWindow();
  }

  return 0;
}

LRESULT CSplineEdit::OnLButton(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CPoint  ptCursor(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
  sBool   redraw=sFALSE;

  m_lbDown=(uMsg==WM_LBUTTONDOWN);

  if (m_lbDown)
  {
    if (::GetCapture()!=m_hWnd)
      SetCapture();
  }
  else
  {
    if (::GetCapture()==m_hWnd)
      ::ReleaseCapture();
  }

  updateCurvesPtr();
  
  if (m_lbDown && ::PtInRect(&m_rcDrawExt, ptCursor))
  {
    // calculate some useful constants & translate click point
    const sF32  pixToValX=m_mappedX/(m_rcDraw.right-m_rcDraw.left);
    const sF32  pixToValY=m_mappedY/(m_rcDraw.bottom-m_rcDraw.top);
    const sF32  valToPixX=(m_rcDraw.right-m_rcDraw.left)/m_mappedX;
    const sF32  valToPixY=(m_rcDraw.bottom-m_rcDraw.top)/m_mappedY;
    const sF32  clickX=m_startX+(ptCursor.x-m_rcDraw.left)*pixToValX;
    const sF32  clickY=m_startY-(ptCursor.y-m_rcDraw.top)*pixToValY;

    sInt curvHit, hit;
    ControlPointHitTest(ptCursor, curvHit, hit);
    
    if (hit==-1 && m_curves) // no control point clicked, check if it's ok to insert one
    {
      static fr::cubicBezierCurve2D bezier(1);

      for (sInt chan=m_curves->getNChannels()-1; chan>=0; chan--) // for each channel:
      {
        fr::fCurve *curve=m_curves->getCurve(chan);

        // convert to equivalent bezier curve(s)
        curve->convertToBezier(bezier);
        
        // find nearest point on curve
        const fr::vector2 clickPoint(clickX, clickY);
        const fr::vector2 nearestPoint=bezier.nearestPointOnCurve(clickPoint);
        fr::vector2 scaledDist=nearestPoint-clickPoint;
        scaledDist.x*=valToPixX;
        scaledDist.y*=valToPixY;

        const sInt nearestFrame=nearestPoint.x+0.5f;

        // not to far from spline and no control point there yet?
        if (scaledDist.lenSq()<9.0f && !curve->findKey(nearestFrame))
        {
          curve->subdivide(nearestFrame);
          
          const fr::fCurve::key *k1, *k2; // hack to get control point index
          k1=curve->getKey(0);
          k2=curve->findKey(nearestFrame);
        
          hit=k2-k1;
          curvHit=chan;

          redraw=sTRUE;
          break;
        }
      }
    }

    if (hit!=-1) // control point clicked?
    {
      m_dragPoint=ptCursor;
      m_moveKey=hit;
      m_curKey=hit;
      m_curCurve=curvHit;
      redraw=sTRUE;

      UpdateCurveType();

      const fr::fCurve *curve=m_curves->getCurve(m_curCurve);

      m_origFrame=curve->getKey(hit)->frame;
      m_origValue=curve->getKey(hit)->value;

      m_minFrame=(hit==0)?0:curve->getKey(hit-1)->frame+1;
      m_maxFrame=(hit==curve->getNKeys()-1)?m_minFrame-1:curve->getKey(hit+1)->frame-1;

      if (hit==0)
        m_maxFrame=m_minFrame;
    }
  }
  else
    m_moveKey=-1;

  if (redraw)
  {
    m_updateDraw=sTRUE;
    Invalidate();
    UpdateWindow();
  }

  return 0;
}

LRESULT CSplineEdit::OnRButton(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_rbDown=(uMsg==WM_RBUTTONDOWN);

  if (m_rbDown)
  {
    if (::GetCapture()!=m_hWnd)
      SetCapture();
  }
  else
  {
    if (::GetCapture()==m_hWnd)
      ::ReleaseCapture();
  }

  CPoint ptCursor(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

  updateCurvesPtr();

  sInt curvHit, hit;
  ControlPointHitTest(ptCursor, curvHit, hit);

  if (m_rbDown && hit!=-1)
  {
    fr::fCurve *curv=m_curves->getCurve(curvHit);

    if (curv->getNKeys()>2 && hit>0)
    {
      curv->removeKey(curv->getKey(hit)->frame);
      g_graph->modified();

      if (m_moveKey==hit)
        m_moveKey=-1;

      Invalidate();
      UpdateWindow();

      ::PostMessage(g_winMap.Get(ID_TIMELINE_WINDOW), WM_CLIP_CHANGED, 0, 0);
    }
    else
      Beep(440, 100);

    m_rbDown=sFALSE;
  }
  else
  {
    m_origMappedX=m_mappedX;
    m_origMappedY=m_mappedY;
    m_midY=m_startY-m_mappedY*0.5f;
  
    m_sizePoint=ptCursor;
  }

  return 0;
}

LRESULT CSplineEdit::OnMButton(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_mbDown=(uMsg==WM_MBUTTONDOWN);
  
  if (m_mbDown)
  {
    if (::GetCapture()!=m_hWnd)
      SetCapture();
  }
  else
  {
    if (::GetCapture()==m_hWnd)
      ::ReleaseCapture();
  }
  
  m_scrlPoint=CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
  
  return 0;
}

LRESULT CSplineEdit::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CSize sz(lParam);

  if (!m_bufBitmap.IsNull())
    m_bufBitmap.DeleteObject();

  if (!m_bufBitmap2.IsNull())
    m_bufBitmap2.DeleteObject();

  HDC hDC=GetDC();
  m_bufBitmap.CreateCompatibleBitmap(hDC, sz.cx, sz.cy);
  m_bufBitmap2.CreateCompatibleBitmap(hDC, sz.cx, sz.cy);
  ReleaseDC(hDC);
  
  m_bufferOld=sTRUE;

  placeCurveShowButtons();

  bHandled=FALSE;
  return 0;
}

LRESULT CSplineEdit::OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (wParam == 'P' && m_curves->getNChannels() == 3)
	{
		// copy from (camera) position
		sF32 values[3];

		::SendMessage(g_winMap.Get(ID_3DPREVIEW_WINDOW), WM_STORE_DATA, 0, (LPARAM) values);
		setValues(values);
	}
	else if (wParam == 'R' && m_curves->getNChannels() == 3)
	{
		// copy from (camera) rotation
		sF32 values[3];

		::SendMessage(g_winMap.Get(ID_3DPREVIEW_WINDOW), WM_STORE_DATA, 1, (LPARAM) values);
		setValues(values);
	}

	bHandled = FALSE;
	return 0;
}

LRESULT CSplineEdit::OnSetSplineType(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  updateCurvesPtr();

  if (m_curCurve!=-1 && m_curves)
    m_curves->getCurve(m_curCurve)->setType((fr::fCurve::curveType) (wID-100));

  UpdateCurveType();

  m_updateDraw=sTRUE;
  Invalidate();
  UpdateWindow();

  return 0;
}

LRESULT CSplineEdit::OnSetEditSpline(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  sU32 *ids=(sU32 *) lParam;

  if (ids)
    SetSpline(ids[0], ids[1]);
  else
    SetSpline(0, 0);

  return 0;
}

LRESULT CSplineEdit::OnFrameChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  const sInt newFrame=lParam;

  if (newFrame!=m_curFrame)
  {
    m_curFrame=newFrame;
    HDC dc=GetDC();
    drawBar(dc);
    ReleaseDC(dc);
  }
  
  return 0;
}

LRESULT CSplineEdit::OnSnapValue(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (m_curCurve!=-1 && m_curves)
  {
    fr::fCurve *crv=m_curves->getCurve(m_curCurve);

    if (crv->getNKeys())
    {
      const sInt lastKey=crv->getNKeys()-1;

      switch (wID)
      {
      case 200: // snap first->last
        crv->getKey(0)->value=crv->getKey(lastKey)->value;
        crv->keyModified(0);
        break;

      case 201: // snap last->first
        crv->getKey(lastKey)->value=crv->getKey(0)->value;
        crv->keyModified(lastKey);
        break;
      }

      ::PostMessage(g_winMap.Get(ID_TIMELINE_WINDOW), WM_CLIP_CHANGED, 0, 0);
      
      m_updateDraw=sTRUE;
      Invalidate();
      UpdateWindow();
    }
  }
  
  return 0;
}

LRESULT CSplineEdit::OnShowCurve(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  m_updateDraw=sTRUE;
  Invalidate();
  UpdateWindow();

  return 0;
}

// ---- paint tool funcs

void CSplineEdit::SetDrawRect(const CRect &rc)
{
  m_rcDraw=rc;
  m_rcDraw.DeflateRect(80, 30);
  m_rcDraw.right+=80-16;
  m_rcDrawExt=m_rcDraw;
  m_rcDrawExt.InflateRect(3, 3); // compensate for possible control point overhang
}

// ---- gui helpers

void CSplineEdit::ControlPointHitTest(CPoint ptCursor, sInt &curv, sInt &hit)
{
  curv=-1;
  hit=-1;

  if (!m_curves)
    return;

  if (::PtInRect(&m_rcDrawExt, ptCursor))
  {
    // calculate some useful constants
    const sF32  pixToValX=m_mappedX/(m_rcDraw.right-m_rcDraw.left);
    const sF32  pixToValY=m_mappedY/(m_rcDraw.bottom-m_rcDraw.top);
    const sF32  valToPixX=(m_rcDraw.right-m_rcDraw.left)/m_mappedX;
    const sF32  valToPixY=(m_rcDraw.bottom-m_rcDraw.top)/m_mappedY;
  
    // translate click point to value space
    const sF32  clickX=m_startX+(ptCursor.x-m_rcDraw.left)*pixToValX;
    const sF32  clickY=m_startY-(ptCursor.y-m_rcDraw.top)*pixToValY;
  
    // binary search if and which curve and control point was clicked
    for (sInt chan=m_curves->getNChannels()-1; chan>=0; chan--)
    {
      if (!m_curveShow[chan].GetCheck())
        continue;

      const fr::fCurve  *curve=m_curves->getCurve(chan);
      sInt              l=0, r=curve->getNKeys()-1;
    
      while (r>=l)
      {
        sInt x=(l+r)/2;
        const fr::fCurve::key *curKey=curve->getKey(x);
      
        // calc distance
        const sF32  distX=clickX-curKey->frame;
        const sF32  distY=clickY-curKey->value;
      
        if ((distX*distX*valToPixX*valToPixX)+(distY*distY*valToPixY*valToPixY)<25.0f) // control point clicked?
        {
          hit=x;
          curv=chan;
          break;
        }
      
        if (distX<0)
          r=x-1;
        else
          l=x+1;
      }
    
      if (hit!=-1)
        break;
    }
  }
}

void CSplineEdit::UpdateCurveType()
{
  sInt curType=-1;

  updateCurvesPtr();

  if (m_curCurve!=-1 && m_curves)
    curType=m_curves->getCurve(m_curCurve)->getType();

  for (sInt i=0; i<3; i++)
  {
    m_types[i].SetCheck(i==curType);
    m_types[i].EnableWindow(curType!=-1);
  }
}

// ---- painting

void CSplineEdit::DrawGrid(CDCHandle dc)
{
  CPen pen_grid, pen_guide, pen_bar;
  pen_grid.CreatePen(PS_DOT, 1, RGB(174*3/4, 169*3/4, 167*3/4)^RGB(174, 169, 167));
  pen_guide.CreatePen(PS_SOLID, 1, RGB(174/4, 169/4, 167/4));
  pen_bar.CreatePen(PS_SOLID, 1, RGB(174/2, 169/2, 167/2)^RGB(174, 169, 167));
  
  CFont descFont;
  descFont.CreateFont(13, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
    DEFAULT_PITCH, "Arial");
  
  const sF32 xstep=fr::maximum(gridMapStep(m_mappedX, (m_rcDraw.right-m_rcDraw.left)/150, 10), 1.0f);
  const sInt xms=m_startX/xstep;
  const sInt xme=(m_startX+m_mappedX)/xstep;

  const sF32 ystep=fr::maximum(gridMapStep(m_mappedY, (m_rcDraw.bottom-m_rcDraw.top)/30, 10), 0.01f);
  const sInt yms=(m_startY-m_mappedY)/ystep;
  const sInt yme=m_startY/ystep;
    
  CFontHandle oldFont=dc.SelectFont(descFont);
  CPenHandle oldPen=dc.SelectPen(pen_guide);
  dc.SetBkMode(TRANSPARENT);
  dc.SetTextColor(RGB(174/4, 169/4, 167/4));
  dc.SetTextAlign(TA_RIGHT|TA_TOP);
  
  sInt i;
  for (i=yms; i<=yme; i++)
  {
    const sInt yc=mapY(i*ystep);

    if (yc>=m_rcDraw.top && yc<=m_rcDraw.bottom)
    {
      dc.MoveTo(m_rcDraw.left, yc);
      dc.LineTo(m_rcDraw.left-4, yc);
    
      TCHAR buf[16];
      sprintf(buf, "%.2f", i*ystep);
    
      dc.TextOut(m_rcDraw.left-6, yc-6, buf);
    }
  }

  dc.SetTextAlign(TA_CENTER|TA_TOP);
  for (i=xms; i<=xme; i++)
  {
    const sInt xc=mapX(i*xstep);

    if (xc>=m_rcDraw.left && xc<=m_rcDraw.right)
    {
      dc.MoveTo(xc, m_rcDraw.bottom+1);
      dc.LineTo(xc, m_rcDraw.bottom+4);
      
      TCHAR buf[16];
      sprintf(buf, "%.3f", i*xstep*0.001f);
      
      dc.TextOut(xc, m_rcDraw.bottom+5, buf);
    }
  }
  
  CRgn clipRgn;
  clipRgn.CreateRectRgn(m_rcDraw.left, m_rcDraw.top, m_rcDraw.right+1, m_rcDraw.bottom+1);
  dc.SelectClipRgn(clipRgn);
  
  // draw grid (y)
  dc.SelectPen(pen_grid);
  dc.SetROP2(R2_XORPEN);
  dc.SetBkColor(RGB(0, 0, 0));
  
  for (i=yms; i<=yme; i++)
  {
    if (i==0)
      continue; // null line is already there
    
    const sInt yc=mapY(i*ystep);
    dc.MoveTo(m_rcDraw.left+1, yc);
    dc.LineTo(m_rcDraw.right, yc);
  }
  
  // draw grid (x)
  for (i=xms; i<=xme; i++)
  {
    const sInt xc=mapX(i*xstep);
    dc.MoveTo(xc, m_rcDraw.top);
    dc.LineTo(xc, m_rcDraw.bottom);
  }
  
  // draw bars
  dc.SelectPen(pen_bar);
  const fr::viruzIIPositions *pos=g_viruz->getPositions();
  sInt lastPos=-1;

  for (sU32 posIndex=0; posIndex<pos->getNumPositions(); posIndex++)
  {
    sInt posi=pos->getPosCode(posIndex) & 0xfff800;
    if (posi==lastPos)
      continue;

    lastPos=posi;

    if ((posi & 0xff00) == 0x0000)
      dc.SetBkColor(RGB(0, 0, 0));
    else
      dc.SetBkColor(RGB(174, 169, 167));
    
    const sInt xpos=mapX(pos->getTime(posIndex));
    
    dc.MoveTo(xpos, m_rcDraw.top);
    dc.LineTo(xpos, m_rcDraw.bottom);
  }
  
  // draw lines
  dc.SelectPen(pen_guide);
  dc.SetROP2(R2_COPYPEN);
  dc.MoveTo(m_rcDraw.left, mapY(0.0f)); // null
  dc.LineTo(m_rcDraw.right, mapY(0.0f));
  dc.MoveTo(m_rcDraw.left, m_rcDraw.top); // left edge
  dc.LineTo(m_rcDraw.left, m_rcDraw.bottom);
  dc.MoveTo(m_rcDraw.left, m_rcDraw.bottom); // bottom edge
  dc.LineTo(m_rcDraw.right+1, m_rcDraw.bottom);
  
  dc.SelectPen(oldPen);
  dc.SelectFont(oldFont);
}

void CSplineEdit::DrawCurves(CDCHandle dc, const fr::fCurveGroup &x)
{
  for (sInt chan=0; chan<x.getNChannels(); chan++)
  {
    if (!m_curveShow[chan].GetCheck())
      continue;

    DrawCurve(dc, *x.getCurve(chan), chan, curveColor[chan & 7]);
  }
}

void CSplineEdit::DrawCurve(CDCHandle dc, const fr::fCurve &x, sInt curvNdx, COLORREF color)
{
  static fr::cubicBezierCurve2D bezier(1);
  sInt i;

  x.convertToBezier(bezier);

  CRgn clipRgn;
  clipRgn.CreateRectRgn(m_rcDraw.left, m_rcDraw.top, m_rcDraw.right+1, m_rcDraw.bottom+1);
  dc.SelectClipRgn(clipRgn);
  
  CPen penControl, penActControl, penCurve, penTangent;
  penControl.CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
  penActControl.CreatePen(PS_SOLID, 1, RGB(128, 0, 0));
  penCurve.CreatePen(PS_SOLID, 1, color);
  penTangent.CreatePen(PS_DOT, 1, RGB(174/2, 169/2, 167/2));

  CBrush brushControl, brushActControl;
  brushControl.CreateSolidBrush(RGB(0, 0, 0));
  brushActControl.CreateSolidBrush(RGB(255, 0, 0));

  // draw/erase the bezier curve
  CPenHandle oldPen=dc.SelectPen(penCurve);
  
  for (i=0; i<bezier.nSegments; i++)
  {
    if (bezier.segment[i].D.x-bezier.segment[i].A.x>1) // not a jump?
    {
      // draw the curve
      POINT pts[3];
      pts[0].x=mapX(bezier.segment[i].B.x);
      pts[0].y=mapY(bezier.segment[i].B.y);
      pts[1].x=mapX(bezier.segment[i].C.x);
      pts[1].y=mapY(bezier.segment[i].C.y);
      pts[2].x=mapX(bezier.segment[i].D.x);
      pts[2].y=mapY(bezier.segment[i].D.y);

      dc.MoveTo(mapX(bezier.segment[i].A.x), mapY(bezier.segment[i].A.y));
      dc.PolyBezierTo(pts, 3);
    }
  }
  
  CRgn oldClip;
  ::GetClipRgn(dc, oldClip);
  
  dc.SelectClipRgn(0);
  dc.SelectPen(penControl);
  CBrushHandle oldBrush=dc.SelectBrush(brushControl);

  // draw the control points
  for (i=0; i<x.getNKeys(); i++)
  {
    const fr::fCurve::key *skey=x.getKey(i);
  
    if (skey->frame>=m_startX && skey->frame<m_startX+m_mappedX && skey->value<=m_startY && skey->value>m_startY-m_mappedY)
    {
      const sInt xc=mapX(skey->frame);
      const sInt yc=mapY(skey->value);
    
      if (i==m_curKey && curvNdx==m_curCurve)
      {
        dc.SelectPen(penActControl);
        dc.SelectBrush(brushActControl);
      }

      dc.Ellipse(xc-3, yc-3, xc+4, yc+4);
    
      if (i==m_curKey && curvNdx==m_curCurve)
      {
        dc.SelectPen(penControl);
        dc.SelectBrush(brushControl);
      }
    }
  }

  dc.SelectPen(oldPen);
  dc.SelectBrush(oldBrush);
  dc.SelectClipRgn(oldClip);
}

void CSplineEdit::SetSpline(sU32 opID, sU32 curveID)
{
  if (opID!=m_curvOpID || curveID!=m_curvCurveID)
  {
    m_moveKey=-1;
    m_curKey=-1;
    m_curCurve=-1;
    m_lbDown=sFALSE;
    m_rbDown=sFALSE;
    m_mbDown=sFALSE;
    m_updateDraw=sTRUE;

    m_curvOpID=opID;
    m_curvCurveID=curveID;

    UpdateCurveType();
    Invalidate();
    UpdateWindow();
  }
}

void CSplineEdit::updateCurvesPtr()
{
  if (g_graph->m_ops.find(m_curvOpID)!=g_graph->m_ops.end())
  {
    frCurveContainer::opCurves curves=g_graph->m_curves->getOperatorCurves(m_curvOpID);
    
    m_curves=curves.getCurve(m_curvCurveID);
    placeCurveShowButtons();
  }
  else
    m_curves=0;
}

void CSplineEdit::drawBar(CDCHandle dc)
{
  CPen timePen;
  
  timePen.CreatePen(PS_SOLID, 1, RGB(255, 255, 255)^RGB(174, 169, 167));
  CPenHandle oldPen=dc.SelectPen(timePen);
  int op=dc.SetROP2(R2_XORPEN);
  
  CRect rc=m_rcDraw;
  
  if (m_barPos!=-1)
  {
    dc.MoveTo(m_barPos, rc.bottom-1);
    dc.LineTo(m_barPos, rc.top-1);
    
    m_barPos=-1;
  }

  if (m_curFrame!=-1)
    m_barPos=mapX(m_curFrame);
  
  dc.MoveTo(m_barPos, rc.bottom-1);
  dc.LineTo(m_barPos, rc.top-1);
  
  dc.SelectPen(oldPen);
  dc.SetROP2(op);
}

void CSplineEdit::placeCurveShowButtons()
{
  CRect rc;
  GetClientRect(&rc);

  if (m_curves)
  {
    sInt numCurves=m_curves->getNChannels();

    sInt startX=rc.right-5-numCurves*20;
    for (sInt i=0; i<numCurves; i++)
    {
      m_curveShow[i].SetWindowPos(0, startX+i*20, 2, 20, 20, SWP_NOZORDER);
      m_curveShow[i].ShowWindow(SW_SHOW);
    }

    for (sInt i=numCurves; i<sizeof(m_curveShow)/sizeof(m_curveShow[0]); i++)
      m_curveShow[i].ShowWindow(SW_HIDE);
  }
}

void CSplineEdit::setValues(const sF32* vals)
{
	updateCurvesPtr();
	if (!m_curves)
		return;

	for (sInt i = 0; i < m_curves->getNChannels(); i++)
	{
		fr::fCurve* curve = m_curves->getCurve(i);
		fr::fCurve::key* kk = curve->findKey(m_curFrame);

		if (!kk)
			kk = &curve->addKey(m_curFrame);

		kk->value = vals[i];
	}

  ::PostMessage(g_winMap.Get(ID_TIMELINE_WINDOW), WM_CLIP_CHANGED, 0, 0);
  g_graph->modified();

	m_updateDraw=sTRUE;
  Invalidate();
  UpdateWindow();
}

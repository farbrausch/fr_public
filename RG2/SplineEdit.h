// This code is in the public domain. See LICENSE for details.

// SplineEdit.h: interface for the CSplineEdit class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SPLINEEDIT_H__CB19BDBF_1EFA_4838_88AB_3E0E053AEB80__INCLUDED_)
#define AFX_SPLINEEDIT_H__CB19BDBF_1EFA_4838_88AB_3E0E053AEB80__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

namespace fr
{
  class fCurve;
  class fCurveGroup;
}

class CSplineEdit : public CWindowImpl<CSplineEdit>  
{
public:
  DECLARE_WND_CLASS(0);

	CSplineEdit();
	~CSplineEdit();

  BEGIN_MSG_MAP(CSplineEdit)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButton)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnLButton)
    MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButton)
    MESSAGE_HANDLER(WM_RBUTTONUP, OnRButton)
    MESSAGE_HANDLER(WM_MBUTTONDOWN, OnMButton)
    MESSAGE_HANDLER(WM_MBUTTONUP, OnMButton)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_CHAR, OnChar)
    COMMAND_RANGE_HANDLER(100, 103, OnSetSplineType)
    MESSAGE_HANDLER(WM_SET_EDIT_SPLINE, OnSetEditSpline)
    MESSAGE_HANDLER(WM_FRAME_CHANGED, OnFrameChanged)
    COMMAND_RANGE_HANDLER(200, 201, OnSnapValue)
    COMMAND_RANGE_HANDLER(400, 408, OnShowCurve)
  END_MSG_MAP()

  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnLButton(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnRButton(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMButton(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSetSplineType(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnSetEditSpline(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnFrameChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSnapValue(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnShowCurve(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  
private:
  CBitmap         m_bufBitmap, m_bufBitmap2;
  sBool           m_lbDown, m_rbDown, m_mbDown;
  sBool           m_bufferOld;
  CPoint          m_dragPoint, m_sizePoint, m_scrlPoint;
  sF32            m_origMappedX, m_origMappedY, m_midY;
  sF32            m_mappedX, m_mappedY;
  sF32            m_startX, m_startY;
  CRect           m_rcDraw, m_rcDrawExt;
  
  sU32            m_curvOpID, m_curvCurveID;

  frButton        m_types[4], m_snapFLast, m_snapLFirst;
  frButton        m_snapValue, m_snapTime;
  frButton        m_curveShow[3];

  sBool           m_updateDraw;
  sInt            m_moveKey, m_origFrame, m_minFrame, m_maxFrame;
  sF32            m_origValue;

  sInt            m_curKey, m_curCurve;
  
  sInt            m_barPos;
  sInt            m_curFrame;
  
  void        SetDrawRect(const CRect &rc);
  inline sInt mapX(sF32 xc)     { return (xc-m_startX)*(m_rcDraw.right-m_rcDraw.left)/m_mappedX+m_rcDraw.left; }
  inline sInt mapY(sF32 yc)     { return (m_startY-yc)*(m_rcDraw.bottom-m_rcDraw.top)/m_mappedY+m_rcDraw.top; }

  void        ControlPointHitTest(CPoint pt, sInt &curve, sInt &hit);
  void        UpdateCurveType();

  void        DrawGrid(CDCHandle dc);
  void        DrawCurves(CDCHandle dc, const fr::fCurveGroup &x);
  void        DrawCurve(CDCHandle dc, const fr::fCurve &x, sInt curvNdx, COLORREF color);
  void        SetSpline(sU32 opID, sU32 curveID);

private:
  fr::fCurveGroup *m_curves;

  void        updateCurvesPtr();
  void        drawBar(CDCHandle dc);
  void        placeCurveShowButtons();
	void				setValues(const sF32* vals);
};

#endif // !defined(AFX_SPLINEEDIT_H__CB19BDBF_1EFA_4838_88AB_3E0E053AEB80__INCLUDED_)

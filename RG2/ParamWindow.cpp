// This code is in the public domain. See LICENSE for details.

// ParamWindow.cpp: implementation of the CParamWindow class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ParamWindow.h"
#include "WindowMapper.h"
#include "viewcomm.h"
#include "resource.h"
#include "frOpGraph.h"
#include "expression.h"
#include "frcontrol.h"
#include "tool.h"
#include "plugbase.h"
#include "texbase.h"
#include "animbase.h"
#include "undoBuffer.h"
#include <math.h>

static CBrush g_bgBrush;
static fr::expressionParser g_evaluator;

// ---- tool parameter edit classes

class CColorControl: public CWindowImpl<CColorControl, CWindow, CControlWinTraits>
{
public:
  DECLARE_WND_CLASS(_T("frColorControl"));

  CButton           m_colorButton;
  frNumEditControl  m_rEdit;
  frNumEditControl  m_gEdit;
  frNumEditControl  m_bEdit;

  sBool             m_isFloat;
  sF32              m_red, m_green, m_blue;

  BEGIN_MSG_MAP(CColorControl)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_COMMAND, OnCommand)
    MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
    MESSAGE_HANDLER(WM_CTLCOLOREDIT, OnControlColor)
  END_MSG_MAP()

  CColorControl(sBool isFloat)
    : m_isFloat(isFloat)
  {
  }

  static sInt clamp(sInt a, sInt mn, sInt mx)
  {
    return a < mn ? mn : a > mx ? mx : a;
  }

  void UpdateColor()
  {
    m_rEdit.SetValue(m_red);
    m_gEdit.SetValue(m_green);
    m_bEdit.SetValue(m_blue);

    Invalidate();
    UpdateWindow();
  }

  void SetColorInt(sU8 r, sU8 g, sU8 b)
  {
    m_red = r;
    m_green = g;
    m_blue = b;

    UpdateColor();
  }

  void SetColorFloat(sF32 r, sF32 g, sF32 b)
  {
    m_red = r;
    m_green = g;
    m_blue = b;

    UpdateColor();
  }

  void GetColorInt(sU8& r, sU8& g, sU8& b)
  {
    r = clamp(m_red, 0, 255);
    g = clamp(m_green, 0, 255);
    b = clamp(m_blue, 0, 255);
  }

  void GetColorFloat(sF32& r, sF32& g, sF32& b)
  {
    r = m_red;
    g = m_green;
    b = m_blue;
  }

  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    CFontHandle fnt = AtlGetDefaultGuiFont();
    
    m_colorButton.Create(m_hWnd, rcDefault, 0, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_PUSHBUTTON | BS_OWNERDRAW, WS_EX_STATICEDGE, 100);

    m_rEdit.Create(m_hWnd, rcDefault, 0, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | FRNES_ALIGN_CENTER | FRNES_IMMEDIATEUPDATE | FRNES_NOCLICKEDIT, WS_EX_STATICEDGE, 101);
    m_rEdit.SetFont(fnt);
    m_rEdit.SetPrecision(m_isFloat ? 3 : 0);
    m_rEdit.SetStep(m_isFloat ? 0.01f : 1.0f);
    m_rEdit.SetMin(m_isFloat ? -16.0f : 0.0f);
    m_rEdit.SetMax(m_isFloat ? 16.0f : 255.0f);
    m_rEdit.SetEvaluator(&g_evaluator);

    m_gEdit.Create(m_hWnd, rcDefault, 0, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | FRNES_ALIGN_CENTER | FRNES_IMMEDIATEUPDATE | FRNES_NOCLICKEDIT, WS_EX_STATICEDGE, 102);
    m_gEdit.SetFont(fnt);
    m_gEdit.SetPrecision(m_isFloat ? 3 : 0);
    m_gEdit.SetStep(m_isFloat ? 0.01f : 1.0f);
    m_gEdit.SetMin(m_isFloat ? -16.0f : 0.0f);
    m_gEdit.SetMax(m_isFloat ? 16.0f : 255.0f);
    m_gEdit.SetEvaluator(&g_evaluator);

    m_bEdit.Create(m_hWnd, rcDefault, 0, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | FRNES_ALIGN_CENTER | FRNES_IMMEDIATEUPDATE | FRNES_NOCLICKEDIT, WS_EX_STATICEDGE, 103);
    m_bEdit.SetFont(fnt);
    m_bEdit.SetPrecision(m_isFloat ? 3 : 0);
    m_bEdit.SetStep(m_isFloat ? 0.01f : 1.0f);
    m_bEdit.SetMin(m_isFloat ? -16.0f : 0.0f);
    m_bEdit.SetMax(m_isFloat ? 16.0f : 255.0f);
    m_bEdit.SetEvaluator(&g_evaluator);

    m_rEdit.AddToUpdateGroup(&m_gEdit);
    m_rEdit.AddToUpdateGroup(&m_bEdit);

    m_gEdit.AddToUpdateGroup(&m_rEdit);
    m_gEdit.AddToUpdateGroup(&m_bEdit);

    m_bEdit.AddToUpdateGroup(&m_rEdit);
    m_bEdit.AddToUpdateGroup(&m_gEdit);

    bHandled=FALSE;
    return 0;
  }

  LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    HDWP  dwp;
    sInt  w=GET_X_LPARAM(lParam), h=GET_Y_LPARAM(lParam);
    
    dwp=::BeginDeferWindowPos(4);
    m_rEdit.DeferWindowPos(dwp, 0, (w*0)/4, 0, (w*1)/4-(w*0)/4-2, h, SWP_NOZORDER);
    m_gEdit.DeferWindowPos(dwp, 0, (w*1)/4, 0, (w*2)/4-(w*1)/4-2, h, SWP_NOZORDER);
    m_bEdit.DeferWindowPos(dwp, 0, (w*2)/4, 0, (w*3)/4-(w*2)/4-2, h, SWP_NOZORDER);
    m_colorButton.DeferWindowPos(dwp, 0, (w*3)/4, 0, (w*4)/4-(w*3)/4, h, SWP_NOZORDER);
    ::EndDeferWindowPos(dwp);
    
    UpdateWindow();
    
    bHandled=FALSE;
    return 0;
  }
  
  LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    static sChar  buf[128];
    
    switch (LOWORD(wParam))
    {
    case 100:
      if (HIWORD(wParam)==BN_CLICKED)
      {
        static COLORREF custColors[16];
        CHOOSECOLOR     cc;
        frSaveFocus     fc;
        const sF32      fact = m_isFloat ? 255.0f : 1.0f;
        
        memset(&cc, 0, sizeof(cc));
        cc.lStructSize = sizeof(cc);
        cc.hwndOwner = m_hWnd;
        cc.lpCustColors = (LPDWORD) custColors;
        cc.rgbResult = RGB(clamp(m_red * fact, 0, 255), clamp(m_green * fact, 0, 255), clamp(m_blue * fact, 0, 255));
        cc.Flags = CC_FULLOPEN | CC_RGBINIT;
        
        if (ChooseColor(&cc))
        {
          if (m_isFloat)
            SetColorFloat((cc.rgbResult & 0xff) / 255.0f, ((cc.rgbResult >> 8) & 0xff) / 255.0f, ((cc.rgbResult >> 16) & 0xff) / 255.0f);
          else
            SetColorInt(cc.rgbResult & 0xff, (cc.rgbResult >> 8) & 0xff, (cc.rgbResult >> 16) & 0xff);

          ValueChanged(sTRUE);
        }
      }
      break;
      
    case 101:
      if (HIWORD(wParam) == EN_CHANGE)
      {
        m_red = m_rEdit.GetValue();
        ValueChanged();
      }
      break;
      
    case 102:
      if (HIWORD(wParam) == EN_CHANGE)
      {
        m_green = m_gEdit.GetValue();
        ValueChanged();
      }
      break;

    case 103:
      if (HIWORD(wParam) == EN_CHANGE)
      {
        m_blue = m_bEdit.GetValue();
        ValueChanged();
      }
      break;
    }
    
    return 0;
  }
  
  LRESULT OnDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
  {
    DRAWITEMSTRUCT  *dis=(DRAWITEMSTRUCT *) lParam;
    CDCHandle       dc(dis->hDC);
    const sF32      fact = m_isFloat ? 255.0f : 1.0f;

    dc.FillSolidRect(dis->rcItem.left, dis->rcItem.top, dis->rcItem.right-dis->rcItem.left,
      dis->rcItem.bottom-dis->rcItem.top, RGB(clamp(m_red * fact, 0, 255), clamp(m_green * fact, 0, 255), clamp(m_blue * fact, 0, 255)));
    
    return TRUE;
  }

  LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    CDCHandle dc=(HDC) wParam;
    RECT rc;
    
    GetClientRect(&rc);
    dc.FillRect(&rc, g_bgBrush);
    
    return 1;
  }
  
  LRESULT OnControlColor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    CDCHandle dc=(HDC) wParam;
    
    dc.SetBkMode(TRANSPARENT);
    dc.SetBkColor(RGB(174, 169, 167));
    
    return (LRESULT) ((HBRUSH) g_bgBrush);
  }

  void ValueChanged(sBool set=sFALSE)
  {
    if (set)
    {
      m_rEdit.SetValue(m_red);
      m_gEdit.SetValue(m_green);
      m_bEdit.SetValue(m_blue);
    }

    Invalidate();
    UpdateWindow();
    
    SendMessage(GetParent(), WM_COMMAND, MAKELONG(GetDlgCtrlID(), EN_CHANGE), (LPARAM) m_hWnd);
  }
};

class CPositionControl : public CWindowImpl<CPositionControl, CWindow, CControlWinTraits>
{
public:
  DECLARE_WND_CLASS(_T("frPositionControl"));

  frButton          m_pointButton;
  frNumEditControl  m_xEdit;
  frNumEditControl  m_yEdit;

  sF32              m_posX;
  sF32              m_posY;
  sInt              m_xSize, m_ySize;
  sBool             m_clip;

  BEGIN_MSG_MAP(CPositionControl)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_COMMAND, OnCommand)
    MESSAGE_HANDLER(WM_POINTSELECT, OnPointSelect)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
    MESSAGE_HANDLER(WM_CTLCOLOREDIT, OnControlColor)
  END_MSG_MAP()

  void SetPos(sF32 posX, sF32 posY)
  {
    m_posX = posX;
    m_posY = posY;

    m_xEdit.SetValue(m_posX);
    m_yEdit.SetValue(m_posY);
  }

  void GetPos(sF32& x, sF32& y)
  {
    x = m_posX;
    y = m_posY;
  }

  void SetSize(sInt xSize, sInt ySize)
  {
    m_xSize = xSize;
    m_ySize = ySize;
  }

  void SetClip(sBool clip)
  {
    m_clip = clip;
    m_xEdit.SetMin(clip ? 0.0f : -32.0f);
    m_xEdit.SetMax(clip ? 1.0f : 33.0f);
    m_yEdit.SetMin(clip ? 0.0f : -32.0f);
    m_yEdit.SetMax(clip ? 1.0f : 33.0f);
  }

  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    CFontHandle fnt = AtlGetDefaultGuiFont();

    m_pointButton.Create(m_hWnd, rcDefault, _T("Point"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | FRBS_FLAT | FRBS_HOTTRACK, 0, 100);
    m_pointButton.SetFont(fnt);
    m_pointButton.SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
      RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));
    
    m_xEdit.Create(m_hWnd, rcDefault, 0, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | FRNES_IMMEDIATEUPDATE | FRNES_NOCLICKEDIT, WS_EX_STATICEDGE, 101);
    m_xEdit.SetFont(fnt);
    m_xEdit.SetPrecision(3);
    m_xEdit.SetStep(0.005f);
    m_xEdit.SetEvaluator(&g_evaluator);
    
    m_yEdit.Create(m_hWnd, rcDefault, 0, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | FRNES_IMMEDIATEUPDATE | FRNES_NOCLICKEDIT, WS_EX_STATICEDGE, 102);
    m_yEdit.SetFont(fnt);
    m_yEdit.SetPrecision(3);
    m_yEdit.SetStep(0.005f);
    m_yEdit.SetEvaluator(&g_evaluator);
    
    bHandled = FALSE;
    return 0;
  }

  LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    HDWP  dwp;
    sInt  w=GET_X_LPARAM(lParam), h=GET_Y_LPARAM(lParam);

    dwp=::BeginDeferWindowPos(3);
    m_xEdit.DeferWindowPos(dwp, 0, (w*0)/3, 0, (w*1)/3-(w*0)/3-2, h, SWP_NOZORDER);
    m_yEdit.DeferWindowPos(dwp, 0, (w*1)/3, 0, (w*2)/3-(w*1)/3-2, h, SWP_NOZORDER);
    m_pointButton.DeferWindowPos(dwp, 0, (w*2)/3, 0, (w*3)/3-(w*2)/3, h, SWP_NOZORDER);
    ::EndDeferWindowPos(dwp);

    UpdateWindow();

    bHandled=FALSE;
    return 0;
  }

  LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    switch (LOWORD(wParam))
    {
    case 100:
      if (HIWORD(wParam) == BN_CLICKED)
        SendMessage(g_winMap.Get(ID_PREVIEW_WINDOW), WM_POINTSELECT, 1 + (m_clip != 1), (LPARAM) m_hWnd);
      
      break;

    case 101:
      if (HIWORD(wParam) == EN_CHANGE)
      {
        m_posX = m_xEdit.GetValue();
        ValueChanged();
      }
      break;

    case 102:
      if (HIWORD(wParam) == EN_CHANGE)
      {
        m_posY = m_yEdit.GetValue();
        ValueChanged();
      }
      break;
    }

    return 0;
  }

  LRESULT OnPointSelect(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    m_posX = GET_X_LPARAM(lParam) * 1.0f / m_xSize;
    m_posY = GET_Y_LPARAM(lParam) * 1.0f / m_ySize;
    
    ValueChanged(sTRUE);
    
    return 0;
  }
  
  LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    CDCHandle dc=(HDC) wParam;
    RECT rc;
    
    GetClientRect(&rc);
    dc.FillRect(&rc, g_bgBrush);
    
    return 1;
  }
  
  LRESULT OnControlColor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    CDCHandle dc=(HDC) wParam;
    
    dc.SetBkMode(TRANSPARENT);
    dc.SetBkColor(RGB(174, 169, 167));
    
    return (LRESULT) ((HBRUSH) g_bgBrush);
  }
  
  void ValueChanged(sBool set=sFALSE)
  {
    if (set)
    {
      m_xEdit.SetValue(m_posX);
      m_yEdit.SetValue(m_posY);
    }

    SendMessage(GetParent(), WM_COMMAND, MAKELONG(GetDlgCtrlID(), EN_CHANGE), (LPARAM) m_hWnd);
  }
};

class CSizeSelector: public CWindowImpl<CSizeSelector>
{
public:
  DECLARE_WND_CLASS(_T("frSizeSelector"));

  frNumEditControl  m_editX, m_editY;
  sInt              m_oldValueX, m_oldValueY;

  BEGIN_MSG_MAP(CSizeSelector)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_COMMAND, OnCommand)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
    MESSAGE_HANDLER(WM_CTLCOLOREDIT, OnControlColor)
  END_MSG_MAP()

  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    m_editX.Create(m_hWnd, rcDefault, 0, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|FRNES_ALIGN_CENTER|FRNES_ARROWKBDEDIT|FRNES_NOCLICKEDIT, WS_EX_STATICEDGE, 100);
    m_editX.SetMin(3);
    m_editX.SetMax(10);
    m_editX.SetPrecision(0);
    m_editX.SetStep(0.01);
    m_editX.SetTranslateValueHandler(translateValueHandler, 0);
    m_editX.SetFont(AtlGetDefaultGuiFont());
    m_editX.SetEvaluator(&g_evaluator);
    
    m_editY.Create(m_hWnd, rcDefault, 0, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|FRNES_ALIGN_CENTER|FRNES_ARROWKBDEDIT|FRNES_NOCLICKEDIT, WS_EX_STATICEDGE, 101);
    m_editY.SetMin(3);
    m_editY.SetMax(10);
    m_editY.SetPrecision(0);
    m_editY.SetStep(0.01);
    m_editY.SetTranslateValueHandler(translateValueHandler, 0);
    m_editY.SetFont(AtlGetDefaultGuiFont());
    m_editY.SetEvaluator(&g_evaluator);
    
    m_editX.AddToUpdateGroup(&m_editY);
    m_editY.AddToUpdateGroup(&m_editX);

    bHandled=FALSE;
    return 0;
  }

  LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    HDWP  dwp;
    sInt  w=GET_X_LPARAM(lParam), h=GET_Y_LPARAM(lParam);

    dwp=::BeginDeferWindowPos(3);
    m_editX.DeferWindowPos(dwp, 0, (w*0)/2, 0, (w*1)/2-(w*0)/2-2, h, SWP_NOZORDER);
    m_editY.DeferWindowPos(dwp, 0, (w*1)/2, 0, (w*2)/2-(w*1)/2-2, h, SWP_NOZORDER);
    ::EndDeferWindowPos(dwp);

    UpdateWindow();
    
    bHandled=FALSE;
    return 0;
  }
  
  LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    if (HIWORD(wParam)==EN_CHANGE && LOWORD(wParam)==100)
    {
      sInt m_value=m_editX.GetValue()+0.5f;
      
      if (m_value!=m_oldValueX)
        ::PostMessage(GetParent(), uMsg, MAKELONG(GetDlgCtrlID(), EN_CHANGE), lParam);
    }
    else if (HIWORD(wParam)==EN_CHANGE && LOWORD(wParam)==101)
    {
      sInt m_value=m_editY.GetValue()+0.5f;
      
      if (m_value!=m_oldValueY)
        ::PostMessage(GetParent(), uMsg, MAKELONG(GetDlgCtrlID(), EN_CHANGE), lParam);
    }
    else
      bHandled=FALSE;
    
    return 0;
  }
  
  LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    CDCHandle dc=(HDC) wParam;
    RECT rc;
    
    GetClientRect(&rc);
    dc.FillRect(&rc, g_bgBrush);
    
    return 1;
  }
  
  LRESULT OnControlColor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    CDCHandle dc=(HDC) wParam;
    
    dc.SetBkMode(TRANSPARENT);
    dc.SetBkColor(RGB(174, 169, 167));
    
    return (LRESULT) ((HBRUSH) g_bgBrush);
  }
  
  void SetSize(sInt sx, sInt sy)
  {
    m_oldValueX=sx; m_editX.SetValue(sx);
    m_oldValueY=sy; m_editY.SetValue(sy);
  }

  void GetSize(sInt &sx, sInt &sy) const
  {
    sx=m_editX.GetValue()+0.5f;
    sy=m_editY.GetValue()+0.5f;
  }

  static sF64 translateValueHandler(sF64 value, sBool inv, LPVOID user)
  {
    return inv?(sInt) (log(value)/log(2.0)+0.5f):pow(2.0, (sInt) (value+0.5f));
  }
};

class CMultiFloatControl : public CWindowImpl<CMultiFloatControl, CWindow, CControlWinTraits>
{
public:
  DECLARE_WND_CLASS(_T("frMFloatControl"));

  frNumEditControl  m_edit[3];
  sF32              m_v[3];
  sInt              m_itemCount;
  
  BEGIN_MSG_MAP(CMultiFloatControl)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_COMMAND, OnCommand)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
    MESSAGE_HANDLER(WM_CTLCOLOREDIT, OnControlColor)
  END_MSG_MAP()

  CMultiFloatControl(sInt nItems)
    : m_itemCount(nItems)
  {
    FRASSERT(nItems <= 3);
  }

  void SetValue(const sF32* v)
  {
    for (sInt i = 0; i < m_itemCount; i++)
    {
      m_v[i] = v[i];
      m_edit[i].SetValue(v[i]);
    }
  }

  void GetValue(sF32* v)
  {
    for (sInt i = 0; i < m_itemCount; i++)
      v[i] = m_v[i];
  }

  void SetMin(sF32 min)
  {
    for (sInt i = 0; i < m_itemCount; i++)
      m_edit[i].SetMin(min);
  }

  void SetMax(sF32 max)
  {
    for (sInt i = 0; i < m_itemCount; i++)
      m_edit[i].SetMax(max);
  }
  
  void SetStep(sF32 step)
  {
    for (sInt i = 0; i < m_itemCount; i++)
      m_edit[i].SetStep(step);
  }

  void SetPrecision(sInt prec)
  {
    for (sInt i = 0; i < m_itemCount; i++)
      m_edit[i].SetPrecision(prec);
  }

  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    CFontHandle fnt = AtlGetDefaultGuiFont();

    for (sInt i = 0; i < m_itemCount; i++)
    {
      m_edit[i].Create(m_hWnd, rcDefault, 0, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | FRNES_IMMEDIATEUPDATE | FRNES_NOCLICKEDIT, WS_EX_STATICEDGE, 100 + i);
      m_edit[i].SetFont(fnt);
      m_edit[i].SetEvaluator(&g_evaluator);
    }

    for (sInt i = 0; i < m_itemCount; i++)
    {
      for (sInt j = 0; j < m_itemCount; j++)
      {
        if (i != j)
          m_edit[i].AddToUpdateGroup(m_edit + j);
      }
    }

    bHandled = FALSE;
    return 0;
  }

  LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    HDWP  dwp;
    sInt  w = GET_X_LPARAM(lParam), h = GET_Y_LPARAM(lParam);

    dwp = ::BeginDeferWindowPos(m_itemCount);

    for (sInt i = 0; i < m_itemCount; i++)
      m_edit[i].DeferWindowPos(dwp, 0, (w * i) / m_itemCount, 0, (w * i + w) / m_itemCount - (w * i) / m_itemCount - 2, h, SWP_NOZORDER);

    ::EndDeferWindowPos(dwp);

    UpdateWindow();

    bHandled=FALSE;
    return 0;
  }

  LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    const sInt i = LOWORD(wParam) - 100;

    if (i >= 0 && i < m_itemCount && HIWORD(wParam) == EN_CHANGE)
    {
      m_v[i] = m_edit[i].GetValue();

      SendMessage(GetParent(), WM_COMMAND, MAKELONG(GetDlgCtrlID(), EN_CHANGE), (LPARAM) m_hWnd);
    }
    
    return 0;
  }
  
  LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    CDCHandle dc=(HDC) wParam;
    RECT rc;
    
    GetClientRect(&rc);
    dc.FillRect(&rc, g_bgBrush);
    
    return 1;
  }
  
  LRESULT OnControlColor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    CDCHandle dc=(HDC) wParam;
    
    dc.SetBkMode(TRANSPARENT);
    dc.SetBkColor(RGB(174, 169, 167));
    
    return (LRESULT) ((HBRUSH) g_bgBrush);
  }
};

class CLinkControl: public CWindowImpl<CLinkControl, CWindow, CControlWinTraits>
{
public:
  DECLARE_WND_CLASS(_T("frLinkControl"));

  CStatic           m_refName;
  frButton          m_farRefButton;
  frButton          m_nearRefButton;

  sU32              m_linkOp;
  sU32              m_curOpPage;
  sU32              m_refOpID;
  sInt              m_linkType;

  BEGIN_MSG_MAP(CLinkControl)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_COMMAND, OnCommand)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
    MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnControlColor)
  END_MSG_MAP()

  CLinkControl()
    : m_curOpPage(0), m_refOpID(0), m_linkOp(0), m_linkType(-1)
  {
  }

  void SetLinkType(sInt linkType)
  {
    m_linkType = linkType;

    frOpGraph::pgMapIt it = g_graph->m_pages.find(m_curOpPage);
    if (it != g_graph->m_pages.end())
      m_nearRefButton.EnableWindow(it->second.type == linkType);
  }

  void SetCurOpPage(sU32 curOpPage)
  {
    m_curOpPage = curOpPage;
    SetRefOp(m_refOpID);
  }

  void SetLinkOp(sU32 id)
  {
    m_linkOp = id;
  }

  void SetRefOp(sU32 opID)
  {
    m_refOpID = opID;

    sChar refString[520];
    refString[0] = 0;

    frOpGraph::opMapIt it = g_graph->m_ops.find(opID);
    if (it != g_graph->m_ops.end())
    {
      if (it->second.pageID != m_curOpPage)
      {
        strcpy(refString, g_graph->m_pages[it->second.pageID].name);
        strcat(refString, ".");
      }

      strcat(refString, it->second.getName());
    }
    else
      strcpy(refString, "(none)");

    m_refName.SetWindowText(refString);
  }

  sU32 GetRefOp() const
  {
    return m_refOpID;
  }

  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    HFONT fnt=AtlGetStockFont(DEFAULT_GUI_FONT);

    m_refName.Create(m_hWnd, rcDefault, _T(""), WS_CHILD|WS_VISIBLE, WS_EX_STATICEDGE, 10);
    m_refName.SetFont(AtlGetDefaultGuiFont());

    m_farRefButton.Create(m_hWnd, rcDefault, _T("..."), WS_CHILD|WS_VISIBLE|FRBS_FLAT|FRBS_HOTTRACK|FRBS_SOLID, 0, 100);
    m_farRefButton.SetFont(AtlGetDefaultGuiFont());
    m_farRefButton.SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
      RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));

    m_nearRefButton.Create(m_hWnd, rcDefault, _T(".."), WS_CHILD|WS_VISIBLE|FRBS_FLAT|FRBS_HOTTRACK|FRBS_SOLID, 0, 101);
    m_nearRefButton.SetFont(AtlGetDefaultGuiFont());
    m_nearRefButton.SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
      RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));

    bHandled=FALSE;
    return 0;
  }

  LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    HDWP  dwp;
    sInt  w=GET_X_LPARAM(lParam), h=GET_Y_LPARAM(lParam);

    dwp=::BeginDeferWindowPos(3);
    m_refName.DeferWindowPos(dwp, 0, (w*0)/12, 0, (w*10)/12-(w*0)/12-2, h, SWP_NOZORDER);
    m_farRefButton.DeferWindowPos(dwp, 0, (w*10)/12, 0, (w*11)/12-(w*10)/12, h, SWP_NOZORDER);
    m_nearRefButton.DeferWindowPos(dwp, 0, (w*11)/12+2, 0, (w*12)/12-(w*11)/12-2, h, SWP_NOZORDER);
    ::EndDeferWindowPos(dwp);

    UpdateWindow();

    bHandled=FALSE;
    return 0;
  }

  LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    static sChar  buf[128];

    switch (LOWORD(wParam))
    {
    case 100:
      if (HIWORD(wParam)==BN_CLICKED)
      {
        SelectOp(SelectPage());
        SendMessage(GetParent(), WM_COMMAND, MAKELONG(GetDlgCtrlID(), EN_CHANGE), (LPARAM) m_hWnd);
      }
      break;

    case 101:
      if (HIWORD(wParam)==BN_CLICKED)
      {
        SelectOp(m_curOpPage);
        SendMessage(GetParent(), WM_COMMAND, MAKELONG(GetDlgCtrlID(), EN_CHANGE), (LPARAM) m_hWnd);
      }
      break;
    }

    return 0;
  }

  LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    CDCHandle dc=(HDC) wParam;
    RECT rc;

    GetClientRect(&rc);
    dc.FillRect(&rc, g_bgBrush);

    return 1;
  }

  LRESULT OnControlColor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    CDCHandle dc=(HDC) wParam;

    dc.SetBkMode(TRANSPARENT);
    dc.SetBkColor(RGB(174, 169, 167));

    return (LRESULT) ((HBRUSH) g_bgBrush);
  }

  sU32 SelectPage()
  {
    CMenu tempMenu;

    tempMenu.CreatePopupMenu();
    for (frOpGraph::pgMapIt it = g_graph->m_pages.begin(); it != g_graph->m_pages.end(); ++it)
    {
      if (it->second.type == m_linkType)
        tempMenu.AppendMenu(MF_STRING, it->first, it->second.name);
    }

    RECT rc;
    GetClientRect(&rc);
    ClientToScreen(&rc);

    return tempMenu.TrackPopupMenu(TPM_TOPALIGN | TPM_RIGHTALIGN | TPM_NONOTIFY | TPM_RETURNCMD, rc.right, rc.bottom, m_hWnd);
  }

  void SelectOp(sU32 refPage)
  {
    if (!refPage)
      return;

    CMenu tempMenu;
    
    tempMenu.CreatePopupMenu();
    tempMenu.AppendMenu(MF_STRING, 1, "(none)");

		if (g_graph->m_pages[refPage].type == m_linkType)
		{
			for (frOpIterator it(g_graph->m_pages[refPage]); it; ++it)
			{
				if (!g_graph->dependsOn(it.getID(), m_linkOp))
					tempMenu.AppendMenu(MF_STRING, it.getID() + 1, it.getOperator().getName());
			}
		}

    RECT rc;
    GetClientRect(&rc);
    ClientToScreen(&rc);

    sInt res = tempMenu.TrackPopupMenu(TPM_TOPALIGN | TPM_RIGHTALIGN | TPM_NONOTIFY | TPM_RETURNCMD, rc.right, rc.bottom, m_hWnd);
		if (res)
			SetRefOp(res - 1);
  }
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CParamWindow::CParamWindow()
{
  m_nParams=0;
  m_params=0;
  m_curPlug=0;
  m_scrlX=m_scrlY=0;
  m_scrlSX=m_scrlSY=0;
  m_updateParams=sFALSE;
  g_bgBrush.CreateSolidBrush(RGB(174, 169, 167));
  m_animMarks.CreateFromImage(MAKEINTRESOURCE(IDB_ANIMMARKS), 14, 0, CLR_DEFAULT, IMAGE_BITMAP);

  m_size=new frParam;
  m_size->type=frtpSize;
  m_size->desc="Size";
  m_size->animIndex=0;

  m_nameParam = new frParam;
  m_nameParam->type = frtpString;
  m_nameParam->desc = "Name";
  m_nameParam->animIndex = 0;
  m_nameParam->maxLen = 256;
}

CParamWindow::~CParamWindow()
{
  FRSAFEDELETE(m_size);
  FRSAFEDELETE(m_nameParam);
}

// ---- parameter struct

struct CParamWindow::paramStruct
{
  frParam*            param;
  frParamType         type;
  union
  {
    frNumEditControl*   fre;
    frButtonChooser*    sel;
    frButton*           but;
    CPositionControl*   pos;
    CColorControl*      col;
    CEdit*              edt;
    CSizeSelector*      siz;
    CMultiFloatControl* mfl;
    CLinkControl*       lnk;
  };
  sInt                pIndex;
  HWND                hWnd;
  
  paramStruct() : param(0), type(frtpInt), fre(0), pIndex(-1)
  {
  }
  
  ~paramStruct()
  {
    if ((type == frtpColor || type == frtpFloatColor) && col)
    {
      col->DestroyWindow();
      delete col;
      col=0;
    }
    
    if ((type == frtpPoint) && pos)
    {
      pos->DestroyWindow();
      delete pos;
      pos=0;
    }
    
    if ((type == frtpSelect) && sel)
    {
      sel->DestroyWindow();
      delete sel;
      sel=0;
    }
    
    if ((type == frtpButton) && but)
    {
      but->DestroyWindow();
      delete but;
      but=0;
    }
    
    if ((type == frtpInt || type == frtpFloat) && fre)
    {
      fre->DestroyWindow();
      delete fre;
      fre=0;
    }
    
    if ((type == frtpString) && edt)
    {
      edt->DestroyWindow();
      delete edt;
      edt=0;
    }
    
    if ((type == frtpSize) && siz)
    {
      siz->DestroyWindow();
      delete siz;
      siz=0;
    }

    if ((type == frtpTwoFloat || type == frtpThreeFloat) && mfl)
    {
      mfl->DestroyWindow();
      delete mfl;
      mfl=0;
    }
    
    if ((type == frtpLink) && lnk)
    {
      lnk->DestroyWindow();
      delete lnk;
      lnk = 0;
    }
  }
};

// ---- parameter setting code (with undo stuff)

class setParameterCommand: public frUndoCommand
{
	CParamWindow*	m_paramWnd;
	sInt					m_parameter;
	frParam				m_oldValue;
	frParam				m_newValue;

	void setValue(const frParam& value)
	{
		CParamWindow::paramStruct* params = m_paramWnd->m_params;

		if (!params || !params[m_parameter].fre)
			return;

		FRDASSERT(params[m_parameter].param->type == value.type);

		if (params[m_parameter].param->type != frtpLink)
			*params[m_parameter].param = value;
		else
			m_paramWnd->UpdateLinkParam(m_parameter, value.linkv->opID);

		m_paramWnd->UpdateParamValue(m_parameter);
		m_paramWnd->SetParamValue(m_parameter);
	}

public:
	setParameterCommand(CParamWindow* wnd, sInt parm, const frParam& oldVal, const frParam& newVal)
		: frUndoCommand(9), m_paramWnd(wnd), m_parameter(parm), m_oldValue(oldVal),
		m_newValue(newVal)
	{
	}

	void execute()
	{
		setValue(m_newValue);
	}

	void unExecute()
	{
		setValue(m_oldValue);
	}

	sBool mergeWith(frUndoCommand* b)
	{
		setParameterCommand* cmd = (setParameterCommand*) b;

		if (cmd->m_parameter != m_parameter)
			return sFALSE;

		m_newValue = cmd->m_newValue;

		return sTRUE;
	}
};

// ---- message handling

LRESULT CParamWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CMessageLoop* pLoop = _Module.GetMessageLoop();
  pLoop->AddMessageFilter(this);
  pLoop->AddIdleHandler(this);

  m_curParam = 0;
  
  bHandled = FALSE;
  return 0;
}

LRESULT CParamWindow::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  const sInt ctl = LOWORD(wParam) - 100;

  bHandled = FALSE;
  
  if (!m_params)
    return 1;
  
  if (ctl < 0 || ctl >= m_nParams)
    return 1;
  
  if (!m_params[ctl].fre)
    return 1;
  
  bHandled = TRUE;

  if (HIWORD(wParam) == EN_CHANGE || m_params[ctl].param->type == frtpSelect)
    GetParamValue(ctl);
  
  return 0;
}

LRESULT CParamWindow::OnControlColor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CDCHandle dc = (HDC) wParam;

  dc.SetBkMode(TRANSPARENT);
  dc.SetBkColor(RGB(174, 169, 167));
  
  return (LRESULT) ((HBRUSH) g_bgBrush);
}

LRESULT CParamWindow::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  SetEditedPlugin(0);
  
  CMessageLoop* pLoop = _Module.GetMessageLoop();
  pLoop->RemoveMessageFilter(this);
  pLoop->RemoveIdleHandler(this);

  bHandled = FALSE;
  return 0;
}

LRESULT CParamWindow::OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CDCHandle dc = (HDC) wParam;
  RECT rc;

  GetClientRect(&rc);
  dc.FillSolidRect(&rc, RGB(174, 169, 167));

  return 1;
}

LRESULT CParamWindow::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CPaintDC dc(m_hWnd);

  dc.SetViewportOrg(-m_scrlX, -m_scrlY);
  DoPaint(dc.m_hDC);

  return 0;
}
  
LRESULT CParamWindow::OnSetEditPlugin(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  SetEditedPlugin(lParam, wParam);

  return 0;
}

LRESULT CParamWindow::OnMButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (::GetCapture() != m_hWnd)
    SetCapture();

  m_dragOrigin.SetPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
  
  return 0;
}

LRESULT CParamWindow::OnMButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (::GetCapture() == m_hWnd)
    ReleaseCapture();

  return 0;
}

LRESULT CParamWindow::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CPoint ptCursor(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

  if (wParam & MK_MBUTTON)
  {
    Scroll(m_dragOrigin.x - ptCursor.x, m_dragOrigin.y - ptCursor.y);
    m_dragOrigin = ptCursor;
  }

  return 0;
}

LRESULT CParamWindow::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  bHandled = FALSE;

  return 0;
}

LRESULT CParamWindow::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CPoint ptCursor(GET_X_LPARAM(lParam) + m_scrlX, GET_Y_LPARAM(lParam) + m_scrlY);

  if (ptCursor.x >= m_leftMargin - 14 && ptCursor.x < m_leftMargin)
  {
    const sInt xd = ptCursor.x - (m_leftMargin - 7);
    sInt bd = 50, bm;

    for (sInt i = 0; i < m_nParams; i++)
    {
      if (!m_params[i].param->animIndex)
        continue;

      const sInt yd = ptCursor.y - (i * 24 + 5 + 3 + 7);
      const sInt dist=xd * xd + yd * yd;

      if (i == 0 || dist < bd)
      {
        bm = i;
        bd = dist;
      }
    }

    if (bd <= 7*7) // anim mark clicked?
    {
    }
    else
      bHandled=FALSE;
  }
  else
    bHandled=FALSE;

  return 0;
}

LRESULT CParamWindow::OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CPoint ptCursor(GET_X_LPARAM(lParam)+m_scrlX, GET_Y_LPARAM(lParam)+m_scrlY);
  
  if (ptCursor.x>=m_leftMargin-14 && ptCursor.x<m_leftMargin)
  {
    const sInt xd=ptCursor.x-(m_leftMargin-7);
    sInt bd=7*7+1, bm=-1;
    
    for (sInt i=0; i<m_nParams; i++)
    {
      if (!m_params[i].param->animIndex || m_params[i].pIndex == -1)
        continue;
      
      const sInt yd=ptCursor.y-(i*24+5+3+7);
      const sInt dist=xd*xd+yd*yd;
      
      if (dist<bd)
      {
        bm=i;
        bd=dist;
      }
    }
    
    if (bd<=7*7) // anim mark clicked?
    {
      // add animation clip for this operator
      const sU32 clip=g_graph->m_clips->addSimpleClip(m_curPlugID, m_params[bm].pIndex); // add the clip
      ::PostMessage(g_winMap.Get(ID_TIMELINE_WINDOW), WM_SET_CURRENT_CLIP, 0, clip);
    }
    else
      bHandled=FALSE;
  }
  else
    bHandled=FALSE;
  
  return 0;
}

// ---- advanced message handling

BOOL CParamWindow::PreTranslateMessage(MSG *pMsg)
{
  GetAsyncKeyState(VK_MENU);

  if (pMsg->message==WM_KEYDOWN && (pMsg->wParam==VK_UP || pMsg->wParam==VK_DOWN || pMsg->wParam==VK_LEFT || pMsg->wParam==VK_RIGHT) && !GetAsyncKeyState(VK_CONTROL) && !GetAsyncKeyState(VK_MENU))
  {
    if (m_focusList.size())
    {
      sU32    oldParam=m_curParam;
      WPARAM  key=pMsg->wParam;
      HWND    focusWin=::GetFocus();

      for (sInt i=0; i<m_focusList.size(); ++i) // update focus if necessary
      {
        if (m_focusList[i].hWnd==focusWin)
        {
          oldParam=i;
          break;
        }
      }

      if (key==VK_LEFT || key==VK_RIGHT) // simple: just next or previous control
      {
        TCHAR nbuf[128];
        GetClassName(focusWin, nbuf, 128);

        if (!lstrcmp(nbuf, _T("Edit")))
          return FALSE; // give arrow keys to edit controls

        m_curParam=(oldParam+m_focusList.size()+(key==VK_LEFT ? -1 : 1)) % m_focusList.size();
      }
      else if (key==VK_UP || key==VK_DOWN) // next/previous group while preserving pos
      {
        sInt groupStart=GetFocusGroupStart(oldParam);
        sInt groupPos=oldParam-groupStart;

        sInt newGroupStart;
        
        if (key==VK_UP)
          newGroupStart=GetFocusGroupStart(groupStart-1);
        else
          newGroupStart=(groupStart+GetFocusGroupLength(groupStart)) % m_focusList.size();

        sInt newGroupLen=GetFocusGroupLength(newGroupStart);

        if (groupPos>=newGroupLen)
          groupPos=newGroupLen-1;

        m_curParam=(newGroupStart+groupPos) % m_focusList.size();
      }

      ::SetFocus(m_focusList[m_curParam].hWnd);

      return TRUE;
    }
  }

  return FALSE;
}

BOOL CParamWindow::OnIdle()
{
  if (m_updateParams)
  {
    SetEditedPlugin(m_curPlugID, sTRUE);
    m_updateParams=sFALSE;
  }

  return FALSE;
}

// ---- painting

void CParamWindow::DoPaint(CDCHandle dc)
{
  if (m_params && m_nParams)
  {
    CFontHandle oldFont=dc.SelectFont(AtlGetDefaultGuiFont());

    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(::GetSysColor(COLOR_BTNTEXT));
    
    for (sU32 i=0; i<m_nParams; i++)
    {
      if (m_params && m_params[i].type!=frtpButton)
      {
        dc.TextOut(4, i*24+5+3, m_params[i].param->desc);

        if (m_params[i].param->animIndex)
          m_animMarks.Draw(dc, GetAnimState(i), m_leftMargin-14, i*24+5+3, ILD_NORMAL);
      }
    }

    dc.SelectFont(oldFont);
  }
}

// ---- parameter handling

void CParamWindow::SetEditedPlugin(sU32 plID, sBool force)
{
  sU32  i;
  sInt  descSz;

  if (plID == m_curPlugID && !force)
    return;

  if (m_params)
  {
    delete[] m_params;
    m_params=0;
    m_nParams=0;

    ClearFocusList();
  }
  
  frOpGraph::opMapIt it = g_graph->m_ops.find(plID);
  
  if (it == g_graph->m_ops.end())
  {
    m_curPlug = 0;
    m_curPlugID = 0;

    SetWindowText("Parameters: (no edit item)");

    Invalidate();
    UpdateWindow();
		::InvalidateRect(GetParent(), 0, FALSE);
		::UpdateWindow(GetParent());

    return;
  }

  frPlugin* pl = it->second.plg;

  sChar title[256];
  strcpy(title, "Parameters: ");
  strcat(title, pl->getDisplayName());
  SetWindowText(title);

	::InvalidateRect(GetParent(), 0, FALSE);
	::UpdateWindow(GetParent());

  if (pl->def->nInputs==0 && !pl->def->isSystem && pl->def->type==0)
    m_pStart=2;
  else
    m_pStart=1;
  
  sInt nParams = pl->getNParams();
  m_params = new paramStruct[nParams + m_pStart];

  ClearFocusList();

  CFont fnt=AtlGetDefaultGuiFont();
  CDC   tempdc;
  
  tempdc.CreateCompatibleDC(0);
  CFontHandle oldFont=tempdc.SelectFont(fnt);
  descSz=0;

  m_nParams = m_pStart;

  for (i = 0; i < nParams; i++)
  {
    if (pl->displayParameter(i))
    {
      m_params[m_nParams].pIndex = i;
      m_params[m_nParams++].param = pl->getParam(i);
    }
  }

  m_params[0].param = m_nameParam;
  m_nameParam->stringv = g_graph->m_ops[plID].getName();

  if (m_pStart==2) // texture generator
  {
    m_params[1].param=m_size;
    frTexturePlugin *tpg=(frTexturePlugin *) pl;

    sInt xs=0, ys=0;

    while ((1<<xs)<tpg->targetWidth)
      xs++;

    while ((1<<ys)<tpg->targetHeight)
      ys++;

    m_size->tfloatv.a=xs;
    m_size->tfloatv.b=ys;
  }

  for (i=0; i<m_nParams; i++)
  {
    SIZE  sz;

    tempdc.GetTextExtent(m_params[i].param->desc, strlen(m_params[i].param->desc), &sz);
    if (sz.cx>descSz)
      descSz=sz.cx;
  }

  descSz+=14;
  m_leftMargin=descSz+4;

  CSize sizeScroll(0, 0);

  for (i=0; i<m_nParams; i++)
  {
    RECT  rc;
    sU32  stex;

    rc.left=descSz+8;
    rc.top=i*24+5;
    rc.bottom=rc.top+24;
    sizeScroll.cy=rc.bottom;

    stex=(m_nParams>1 && i==0)?WS_GROUP|WS_TABSTOP:WS_TABSTOP;
    
    m_params[i].fre = 0;
    m_params[i].hWnd = 0;
    m_params[i].type = m_params[i].param->type;

    switch (m_params[i].param->type)
    {
    case frtpInt:
    case frtpFloat:
      {
        frNumEditControl *edt=new frNumEditControl;

        rc.right=rc.left+192;
        rc.bottom=rc.top+20;

        edt->Create(m_hWnd, rc, 0, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|stex|FRNES_IMMEDIATEUPDATE|FRNES_NOCLICKEDIT|FRNES_ARROWKBDEDIT, WS_EX_STATICEDGE, i+100);
        edt->SetFont(fnt);
        m_params[i].fre=edt;
        m_params[i].hWnd=edt->m_hWnd;

        AddToFocusList(edt->m_hWnd, sTRUE);

        edt->SetStep(m_params[i].param->step);
        edt->SetMin(m_params[i].param->min);
        edt->SetMax(m_params[i].param->max);
        edt->SetEvaluator(&g_evaluator);
        
        if (m_params[i].param->type==frtpInt)
        {
          edt->SetValue(m_params[i].param->intv);
          edt->SetPrecision(0);
        }
        else if (m_params[i].param->type==frtpFloat)
        {
          edt->SetValue(m_params[i].param->floatv);
          edt->SetPrecision(m_params[i].param->prec);
        }
      }
      break;

    case frtpColor:
    case frtpFloatColor:
      {
        const sBool isFloat = m_params[i].param->type == frtpFloatColor;
        CColorControl* col = new CColorControl(isFloat);
        
        rc.right = rc.left + 192;
        rc.bottom = rc.top + 20;
        
        col->Create(m_hWnd, rc, 0, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | stex, 0, i + 100);

        if (!isFloat)
          col->SetColorInt(m_params[i].param->colorv.r, m_params[i].param->colorv.g, m_params[i].param->colorv.b);
        else
          col->SetColorFloat(m_params[i].param->fcolorv.r, m_params[i].param->fcolorv.g, m_params[i].param->fcolorv.b);
        
        m_params[i].col = col;
        m_params[i].hWnd = col->m_hWnd;

        AddToFocusList(col->m_rEdit.m_hWnd, sTRUE);
        AddToFocusList(col->m_gEdit.m_hWnd);
        AddToFocusList(col->m_bEdit.m_hWnd);
      }
      break;

    case frtpPoint:
      {
        CPositionControl* pos = new CPositionControl;

        rc.right = rc.left + 192;
        rc.bottom = rc.top + 20;

        frTexture* dat = static_cast<frTexturePlugin*>(pl)->getData();

        pos->Create(m_hWnd, rc, 0, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | stex, 0, i + 100);
        pos->SetSize(dat->w, dat->h);
        pos->SetPos(m_params[i].param->pointv.x, m_params[i].param->pointv.y);
        pos->SetClip(m_params[i].param->clip);

        m_params[i].pos = pos;
        m_params[i].hWnd = pos->m_hWnd;

        AddToFocusList(pos->m_xEdit.m_hWnd, sTRUE);
        AddToFocusList(pos->m_yEdit.m_hWnd);
      }
      break;

    case frtpButton:
      {
        frButton *bt=new frButton;

        rc.right=rc.left+192;
        rc.bottom=rc.top+20;

        bt->Create(m_hWnd, rc, m_params[i].param->desc, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|FRBS_HOTTRACK|FRBS_FLAT|stex, 0, i+100);
        bt->SetFont(fnt);
        bt->SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
          RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));

        m_params[i].but=bt;
        m_params[i].hWnd=bt->m_hWnd;

        AddToFocusList(bt->m_hWnd, sTRUE);
      }
      break;

    case frtpSelect:
      {
        frButtonChooser *chs=new frButtonChooser;

        rc.right=rc.left+192;
        rc.bottom=rc.top+20;

        chs->Create(m_hWnd, rc, 0, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|FRBS_HOTTRACK|FRBS_FLAT|stex, 0, i+100);
        chs->SetFont(fnt);
        chs->SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
          RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));
        
        const sChar *msg=m_params[i].param->selectv.opts;
        sInt cnt=1;
        while (msg && *msg)
        {
          static sChar buf[256];
          sChar *p;
          strcpy(buf, msg);
          if (p=strchr(buf, '|'))
            *p=0;

          chs->AddMenuItem(cnt, buf);
          cnt++;

          msg=strchr(msg, '|');
          if (msg)
            msg++;
        }

        chs->SetSelection(m_params[i].param->selectv.sel+1);
        m_params[i].sel=chs;
        m_params[i].hWnd=chs->m_hWnd;

        AddToFocusList(chs->m_hWnd, sTRUE);
      }
      break;

    case frtpString:
      {
        CEdit *edit=new CEdit;

        rc.right=rc.left+192;
        rc.bottom=rc.top+20;

        edit->Create(m_hWnd, rc, m_params[i].param->stringv, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|ES_AUTOHSCROLL|stex, WS_EX_STATICEDGE, i+100);
        edit->SetLimitText(m_params[i].param->maxLen);
        edit->SetFont(fnt);
        m_params[i].edt=edit;
        m_params[i].hWnd=edit->m_hWnd;

        AddToFocusList(edit->m_hWnd, sTRUE);
      }
      break;

    case frtpSize:
      {
        CSizeSelector *size=new CSizeSelector;

        rc.right=rc.left+192;
        rc.bottom=rc.top+20;

        size->Create(m_hWnd, rc, 0, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS, 0, i+100);
        size->SetSize(m_params[i].param->tfloatv.a, m_params[i].param->tfloatv.b);
        m_params[i].siz=size;
        m_params[i].hWnd=size->m_hWnd;

        AddToFocusList(size->m_editX.m_hWnd, sTRUE);
        AddToFocusList(size->m_editY.m_hWnd);
      }
      break;

    case frtpTwoFloat:
    case frtpThreeFloat:
      {
        const sInt count = (m_params[i].param->type == frtpTwoFloat) ? 2 : 3;
        CMultiFloatControl* mfl = new CMultiFloatControl(count);
        
        rc.right = rc.left + 192;
        rc.bottom = rc.top + 20;
        
        mfl->Create(m_hWnd, rc, 0, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|stex, 0, i+100);
        mfl->SetValue(&m_params[i].param->trfloatv.a);
        mfl->SetMin(m_params[i].param->min);
        mfl->SetMax(m_params[i].param->max);
        mfl->SetStep(m_params[i].param->step);
        mfl->SetPrecision(m_params[i].param->prec);
        
        m_params[i].mfl = mfl;
        m_params[i].hWnd = mfl->m_hWnd;

        for (sInt i = 0; i < count; i++)
          AddToFocusList(mfl->m_edit[i].m_hWnd, i == 0);
      }
      break;

    case frtpLink:
      {
        CLinkControl *lnk = new CLinkControl;

        rc.right = rc.left + 192;
        rc.bottom = rc.top + 20;

        lnk->Create(m_hWnd, rc, 0, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|stex, 0, i+100);
        lnk->SetLinkOp(plID);
        lnk->SetLinkType(m_params[i].param->maxLen);
        lnk->SetCurOpPage(g_graph->m_ops[plID].pageID);
        lnk->SetRefOp(m_params[i].param->linkv->opID);

        m_params[i].lnk=lnk;
        m_params[i].hWnd=lnk->m_hWnd;
      }
      break;
    }
    
    if (rc.right>sizeScroll.cx)
      sizeScroll.cx=rc.right;
  }

  tempdc.SelectFont(oldFont);
  tempdc.DeleteDC();

  m_scrlX=0;
  m_scrlY=0;
  m_scrlSX=sizeScroll.cx;
  m_scrlSY=sizeScroll.cy;
  
  if (m_focusList.size())
  {
    m_curParam%=m_focusList.size();
    ::SetFocus(m_focusList[m_curParam].hWnd);
  }
  else
    m_curParam=0;
    
  m_curPlug = pl;
  m_curPlugID = plID;

  Invalidate();
  UpdateWindow();
}

void CParamWindow::GetParamValue(sInt i)
{
	frParam oldValue;

  if (!m_params || !m_params[i].fre)
    return;

	oldValue = *m_params[i].param;
  
  switch (m_params[i].param->type)
  {
  case frtpInt:
  case frtpFloat:
    if (m_params[i].param->type == frtpInt)
      m_params[i].param->intv = m_params[i].fre->GetValue();
    else if (m_params[i].param->type == frtpFloat)
      m_params[i].param->floatv = m_params[i].fre->GetValue();
    
    break;
    
  case frtpPoint:
    m_params[i].pos->GetPos(m_params[i].param->pointv.x, m_params[i].param->pointv.y);
    break;
    
  case frtpColor:
    m_params[i].col->GetColorInt(m_params[i].param->colorv.r, m_params[i].param->colorv.g, m_params[i].param->colorv.b);
    break;
    
  case frtpSelect:
    m_params[i].param->selectv.sel = SendMessage(m_params[i].sel->m_hWnd, CB_GETCURSEL, 0, 0)-1;
    break;
    
  case frtpString:
    m_params[i].edt->GetWindowText(m_params[i].param->stringv.getBuffer(m_params[i].param->maxLen), m_params[i].param->maxLen+1);
    m_params[i].param->stringv.releaseBuffer();
    break;

  case frtpSize:
    {
      sInt sx, sy;
      m_params[i].siz->GetSize(sx, sy);
      m_params[i].param->tfloatv.a=sx;
      m_params[i].param->tfloatv.b=sy;
    }
    break;

  case frtpTwoFloat:
  case frtpThreeFloat:
    m_params[i].mfl->GetValue(&m_params[i].param->trfloatv.a);
    break;

  case frtpFloatColor:
    m_params[i].col->GetColorFloat(m_params[i].param->fcolorv.r, m_params[i].param->fcolorv.g, m_params[i].param->fcolorv.b);
    break;

  case frtpLink:
		UpdateLinkParam(i, m_params[i].lnk->GetRefOp());
    break;
  }
  
	UpdateParamValue(i);
	g_graph->m_undo.add(new setParameterCommand(this, i, oldValue, *m_params[i].param));
}

void CParamWindow::SetParamValue(sInt i)
{
	if (!m_params || !m_params[i].fre)
		return;

	frParam* parm = m_params[i].param;

	switch (parm->type)
	{
	case frtpInt:
		m_params[i].fre->SetValue(parm->intv);
		break;

	case frtpFloat:
		m_params[i].fre->SetValue(parm->floatv);
		break;

	case frtpPoint:
		m_params[i].pos->SetPos(parm->pointv.x, parm->pointv.y);
		break;

	case frtpColor:
		m_params[i].col->GetColorInt(parm->colorv.r, parm->colorv.g, parm->colorv.b);
		break;

	case frtpSelect:
		m_params[i].sel->SetSelection(parm->selectv.sel + 1);
		break;

	case frtpString:
		m_params[i].edt->SetWindowText(parm->stringv);
		break;

	case frtpSize:
		m_params[i].siz->SetSize(parm->tfloatv.a, parm->tfloatv.b);
		break;

	case frtpTwoFloat:
	case frtpThreeFloat:
		m_params[i].mfl->SetValue(&parm->trfloatv.a);
		break;

	case frtpFloatColor:
		m_params[i].col->SetColorFloat(parm->fcolorv.r, parm->fcolorv.g, parm->fcolorv.b);
		break;

	case frtpLink:
		m_params[i].lnk->SetRefOp(parm->linkv->opID);
		break;
	}
}

void CParamWindow::UpdateLinkParam(sInt i, sU32 newLink)
{
	FRDASSERT(m_params && m_params[i].fre);
	FRDASSERT(m_params[i].param->type == frtpLink);

	frOpGraph::opMapIt it;

	const sU32 oldID = m_params[i].param->linkv->opID;
	it = g_graph->m_ops.find(oldID);
	if (it != g_graph->m_ops.end())
		it->second.delOutput(m_curPlugID | 0x80000000); // delete the output from the previous referee

	const sU32 newID = newLink;
	it = g_graph->m_ops.find(newID);
	if (it != g_graph->m_ops.end())
		it->second.addOutput(m_curPlugID | 0x80000000); // add the output to the new referee

	it = g_graph->m_ops.find(m_curPlugID);
	it->second.delInput(oldID | 0x80000000); // delete the old input reference
	it->second.addInput(m_params[i].pIndex, newID | 0x80000000); // add the new input reference

	m_params[i].param->linkv->opID = newID;
	m_params[i].param->linkv->plg = 0;
}

void CParamWindow::UpdateParamValue(sInt i)
{
	FRDASSERT(m_params && m_params[i].fre);

	if (m_params[i].pIndex != -1 && m_curPlug->setParam(m_params[i].pIndex, m_params[i].param))
		m_updateParams = sTRUE;
	else if (i == 0) // name changed
		g_graph->m_ops[m_curPlugID].setName(m_nameParam->stringv);
	else if (m_pStart == 2 && i == 1) // texgenerator size changed
	{
		frTexturePlugin *tplg=(frTexturePlugin *) m_curPlug;
		tplg->targetWidth = 1 << sInt(m_size->tfloatv.a);
		tplg->targetHeight = 1 << sInt(m_size->tfloatv.b);
		m_updateParams = sTRUE;
	}

	if (m_curPlugID)
		::SendMessage(g_winMap.Get(ID_GRAPH_WINDOW), WM_UPDATE_OP_BUTTON, 0, m_curPlugID);

	if (GetAnimState(i) != 2) // parameter not animated?
		g_graph->m_curves->updateBasePose(m_curPlugID, i);

	::SendMessage(g_winMap.Get(ID_GRAPH_WINDOW), WM_PARM_CHANGED, 0, 0);
}

sInt CParamWindow::GetAnimState(sInt param)
{
  sInt state=2;

  if (!m_curPlugID)
    return state;

  const sInt animIdx = m_params[param].param->animIndex;

  frCurveContainer::opCurves &crv = g_graph->m_curves->getOperatorCurves(m_curPlugID);
  fr::fCurveGroup *basePose = crv.getCurve(animIdx);
  if (basePose)
    state=1;

  const sU32 animClip = ::SendMessage(g_winMap.Get(ID_TIMELINE_WINDOW), WM_GET_CLIP_ID, 0, 0);
  if (!animClip)
    return state;

  const frAnimationClip *clip = g_graph->m_clips->getClip(animClip);
  if (!clip || !clip->isSimple())
    return state;
  
  const sU32 animPlug = clip->m_elements.front().id;
  if (animPlug != m_curPlugID)
    return state;

  const sU32 curveID = clip->m_elements.front().curveID;

  if (crv.getAnimIndex(curveID) != animIdx)
    return state;

  fr::fCurveGroup *curve = crv.getCurve(curveID);
  const sInt curFrame = 0; // FIXME
  
  for (sInt i=0; i<curve->getNChannels(); i++)
  {
    if (curve->getCurve(i)->findKey(curFrame))
      return 0;
  }

  return state;
}

// ---- focus list management

void CParamWindow::ClearFocusList()
{
  m_focusList.clear();
}

void CParamWindow::AddToFocusList(HWND hWnd, sBool newGroup)
{
  focusItem item;

  item.hWnd=hWnd;
  item.group=newGroup;

  m_focusList.push_back(item);
}

sInt CParamWindow::GetFocusGroupStart(sInt pos) const
{
  if (!m_focusList.size())
    return 0;

  pos=(pos+m_focusList.size())%m_focusList.size();
  while (pos && !m_focusList[pos].group)
    pos--;

  return pos;
}

sInt CParamWindow::GetFocusGroupLength(sInt start) const
{
  if (!m_focusList.size())
    return 0;

  start=GetFocusGroupStart(start);

  sInt len = 1;
  while ((start+len<m_focusList.size()) && !m_focusList[start+len].group)
    len++;

  return len;
}

sInt CParamWindow::GetFocusGroupNum(sInt pos) const
{
  sInt ndx=0;

  if (!m_focusList.size())
    return 0;

  while (pos--)
    if (m_focusList[pos].group)
      ndx++;

  return ndx;
}

// ---- scrolling

static sInt clamp(sInt a, sInt mn, sInt mx)
{
  return a<mn?mn:a>mx?mx:a;
}

void CParamWindow::Scroll(sInt ox, sInt oy)
{
  RECT rc;
  sInt scrlSX, scrlSY, mnx, mny;

  GetClientRect(&rc);
  scrlSX=max(0, m_scrlSX-rc.right);
  scrlSY=max(0, m_scrlSY-rc.bottom);

  mnx=clamp(m_scrlX+ox, 0, scrlSX);
  mny=clamp(m_scrlY+oy, 0, scrlSY);

  if (mnx!=m_scrlX || mny!=m_scrlY)
  {
    ScrollWindowEx(m_scrlX-mnx, m_scrlY-mny, 0, 0, 0, 0, SW_ERASE|SW_INVALIDATE|SW_SCROLLCHILDREN);
    m_scrlX=mnx;
    m_scrlY=mny;
  }

  Invalidate();
  UpdateWindow();
}

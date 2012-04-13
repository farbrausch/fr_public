// This code is in the public domain. See LICENSE for details.

// PreviewWindow.cpp: implementation of the CPreviewWindow class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "3dPreviewWindow.h"
#include "WindowMapper.h"
#include "resource.h"
#include "viewcomm.h"
#include "frOpGraph.h"
#include "TG2Config.h"
#include "plugbase.h"
#include "geobase.h"
#include "cmpbase.h"

#include "directx.h"
#include "tool.h"
#include "engine.h"
#include "tstream.h"

extern LPCTSTR lpcstrTG2RegKey;

using namespace fr;
using namespace fvs;

// ---- message processing

BOOL C3DPreviewClientWindow::OnIdle()
{
  ProcessInput();

  static sU32 lastSaveTime = 0;
  const sU32 curTime = GetTickCount();

  if (curTime - lastSaveTime >= 300000)
  {
    lastSaveTime = curTime;

    if (g_graph->m_modified)
    {
      fr::streamF autosave("autosave.rg21", fr::streamF::cr | fr::streamF::wr);
      fr::streamWrapper sw(autosave, sTRUE);
      g_graph->serialize(sw);
      autosave.close();
			g_graph->modified();
    }
  }

  return TRUE;
}

LRESULT C3DPreviewClientWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_curOp=0;
  m_wireframe=sFALSE;
  m_lButtonDown=sFALSE;
  m_rButtonDown=sFALSE;
  m_mTimerSet=sFALSE;
  m_showCull=sFALSE;
  m_mdx=0;
  m_mdy=0;
  m_engineInitialized = sFALSE;

  CMessageLoop *pLoop=_Module.GetMessageLoop();
  pLoop->AddIdleHandler(this);

  ResetCamera();

  bHandled=FALSE;
  return 0;
}

LRESULT C3DPreviewClientWindow::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (m_engineInitialized)
  {
    doneEngine();
    closeDirectX();

    m_engineInitialized = sFALSE;
  }

  CMessageLoop *pLoop=_Module.GetMessageLoop();
  pLoop->RemoveIdleHandler(this);

  bHandled=FALSE;
  return 0;
}

LRESULT C3DPreviewClientWindow::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (!m_engineInitialized)
  {
    initDirectX();

    conf.xRes=100;
    conf.yRes=100;
    conf.windowed=sTRUE;
    conf.triplebuffer=sFALSE;
    conf.notrilin=sFALSE;
    conf.lowtexq=sFALSE;
    conf.multithread=sTRUE;

    startupDirectX(m_hWnd);
    initEngine();
    
    m_engineInitialized = sTRUE;
  }
  else
    SetTimer(1, 20);

  bHandled=FALSE;
  return 0;
}

LRESULT C3DPreviewClientWindow::OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  return 1;
}

LRESULT C3DPreviewClientWindow::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  PaintModel();
  ValidateRect(0);

  return 0;
}

LRESULT C3DPreviewClientWindow::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (wParam==1) // resize timer
  {
    KillTimer(1);

    RECT rc;
    GetClientRect(&rc);

    if (rc.right<1)
      rc.right=1;

    if (rc.bottom<1)
      rc.bottom=1;

    resizeBackbuffer(rc.right, rc.bottom);
		if (g_engine)
		{
			g_engine->setMaterial(0);
			g_engine->resetStates();
		}

    Invalidate();
    UpdateWindow();
  }
  else if (wParam==2) // mouse timer
  {
    KillTimer(2);

    ProcessMouse(m_mButton, m_mdx, m_mdy);
    m_mdx=0;
    m_mdy=0;

    m_mTimerSet=sFALSE;
  }
  else
    bHandled=FALSE;

  return 0;
}

LRESULT C3DPreviewClientWindow::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (::GetCapture()!=m_hWnd)
  {
    SetCapture();
    m_oldFocusWin=SetFocus();
    m_useRuleMouse=ruleMouseSetTarget(m_hWnd);
    
    ::SendMessage(g_winMap.Get(ID_MAIN_VIEW), WM_SET_FS_WINDOW, 1, 0);
  }
  else
  {
    CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    RECT rc;

    GetClientRect(&rc);

    if (!::PtInRect(&rc, pt) && !m_lButtonDown && !m_rButtonDown && !(wParam & MK_MBUTTON))
    {
      if (::GetCapture()==m_hWnd)
      {
        ::ReleaseCapture();
        if (::GetFocus()==m_hWnd)
          ::SetFocus(m_oldFocusWin);

        if (m_useRuleMouse && (HWND) ruleMouseGetTarget() == m_hWnd)
          ruleMouseSetTarget(0);
      }

      return 0;
    }
  }

  CPoint ptCursor(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

  if ((m_lButtonDown || m_rButtonDown || wParam & MK_MBUTTON) && !m_useRuleMouse)
  {
    m_mdx+=ptCursor.x-m_dragOrigin.x;
    m_mdy+=ptCursor.y-m_dragOrigin.y;
    m_dragOrigin=ptCursor;

    if (m_lButtonDown)
      m_mButton=0;
    else if (m_rButtonDown)
      m_mButton=2;
    else if (wParam & MK_MBUTTON)
      m_mButton=1;

    if (!m_mTimerSet)
    {
      SetTimer(2, 3);
      m_mTimerSet=sTRUE;
    }
  }
  
  return 0;
}

LRESULT C3DPreviewClientWindow::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_lButtonDown=sTRUE;
  m_dragOrigin.SetPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

  return 0;
}

LRESULT C3DPreviewClientWindow::OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_lButtonDown=sFALSE;
  
  return 0;
}

LRESULT C3DPreviewClientWindow::OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_rButtonDown=sTRUE;
  m_dragOrigin.SetPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
  
  return 0;
}

LRESULT C3DPreviewClientWindow::OnRButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_rButtonDown=sFALSE;
  
  return 0;
}

LRESULT C3DPreviewClientWindow::OnMButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_moveOrigin=CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
  return 0;
}

LRESULT C3DPreviewClientWindow::OnSetDisplayMesh(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if ((sU32) lParam!=m_curOp)
    m_curOp=(sU32) lParam;
    
  if (!wParam)
  {
		::InvalidateRect(GetParent(), 0, TRUE);
    Invalidate();
    ::UpdateWindow(GetParent());
		UpdateWindow();
  }
  
  return 0;
}

LRESULT C3DPreviewClientWindow::OnRuleMouseInput(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  sInt dx, dy, buttons;
  ruleMousePoll(buttons, dx, dy);

  sInt btn=-1;
  if (buttons & 1)
    btn=0;
  else if (buttons & 2)
    btn=2;
  else if (buttons & 4)
    btn=1;

  if (btn != -1)
    ProcessMouse(btn, dx, dy);

  return 0;
}

// ---- scene painting / camera control

void C3DPreviewClientWindow::ProcessMouse(sInt btn, sInt dx, sInt dy)
{
  switch (btn)
  {
  case 0: // left
    {
      ProcessInput(sFALSE);

      sF32 deltaH=dx/240.0f, deltaP=dy/360.0f;
      
      m_camRotH=fmod(m_camRotH+deltaH, 2.0f);
      m_camRotP-=deltaP;
      if (m_camRotP<-0.5f)
        m_camRotP=-0.5f;
      else if (m_camRotP>0.5f)
        m_camRotP=0.5f;
      
      PaintModel();
    }
    break;
    
  case 1: // middle
    {
      matrix cam;
      CalcCamMatrix(cam);
      
      m_camPos-=0.04f*(1.0f*dx*cam.getXVector()-1.0f*dy*cam.getYVector());
      PaintModel();
    }
    break;
    
  case 2: // right
    {
      ProcessInput(sFALSE);
      
      sF32 deltaH=dx/90.0f, deltaP=dy/180.0f;
      sF32 curLen=m_camPos.len();
      
      if (curLen)
      {
        vector ncp=m_camPos;
        
        sF32 curH=atan2(ncp.x, ncp.z);
        sF32 curP=0.5f*3.1415926535f-atan2(ncp.y, sqrt(ncp.x*ncp.x+ncp.z*ncp.z));
        
        curH=fmod(curH+deltaH, 2.0f*3.1415926535f);
        curP-=deltaP;
        if (curP<0.0001f)
          curP=0.0001f;
        else if (curP>3.1414f)
          curP=3.1414f;
        
        ncp.set(sin(curP)*sin(curH), cos(curP), sin(curP)*cos(curH));
        ncp.scale(curLen);
        m_camPos=ncp;
        
        m_camRotH=curH/3.1415926535f;
        m_camRotP=curP/3.1415926535f-0.5f;
      }

      PaintModel();
    }
    break;
  }
}

void C3DPreviewClientWindow::ProcessInput(sBool paintIfNecessary)
{
  static sU32 oldt=0;
  LARGE_INTEGER now, freq;
  
  QueryPerformanceCounter(&now);
  QueryPerformanceFrequency(&freq);
  
  sF32 tdif=1.0f*(now.LowPart-oldt)/freq.LowPart;
  oldt=now.LowPart;
  
  if (::GetFocus()==m_hWnd)
  {
    sBool fwd=!!(GetAsyncKeyState(g_config->getForwardKey()) & 32768);
    sBool back=!!(GetAsyncKeyState(g_config->getBackwardKey()) & 32768);
    sBool left=!!(GetAsyncKeyState(g_config->getLeftKey()) & 32768);
    sBool rgt=!!(GetAsyncKeyState(g_config->getRightKey()) & 32768);
    sBool up=!!(GetAsyncKeyState(g_config->getUpKey()) & 32768);
    sBool down=!!(GetAsyncKeyState(g_config->getDownKey()) & 32768);
    sBool changed=sFALSE;
    
    if (fwd || back || left || rgt || up || down)
    {
      matrix cam;
      CalcCamMatrix(cam);
      
      const sF32 xsgn=rgt-left, ysgn=up-down, zsgn=fwd-back;
      m_camPos+=tdif*10.0f*(xsgn*cam.getXVector()+ysgn*cam.getYVector()+zsgn*cam.getZVector());
      changed=sTRUE;
    }
    
    if (changed && paintIfNecessary)
      PaintModel();
  }
}

void C3DPreviewClientWindow::PaintModel(sBool noScreen, sInt w, sInt h, sF32 aspect)
{
  frComposePlugin *camMode=0;

  if (g_graph->m_ops.find(m_curOp)!=g_graph->m_ops.end())
  {
    frOpGraph::frOperator& op=g_graph->m_ops[m_curOp];
    if (op.def->type==2) // process compositing objects every frame
    {
      op.plg->process();
      camMode=(frComposePlugin *) op.plg;
    }
  }

  sF32 sw, sh;
  
 	if (noScreen)
 	{
    // TODO: is this really right?
 		sh = aspect * w;
 		sw = h;

 	  if (sw > sh)
 		{
 			sw /= sh;
 			sh = 1.0f;
 		}
 		else
 		{
 			sh /= sw;
 			sw = 1.0f;
 		}		
 	}
 	else
 	{
  	CRect rc;
  	GetClientRect(&rc);
  
  	MONITORINFO mInf;
  	mInf.cbSize=sizeof(mInf);
  	GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY), &mInf);
  
		w=rc.Width();
 		h=rc.Height();
  	sw=mInf.rcMonitor.right-mInf.rcMonitor.left;
  	sh=mInf.rcMonitor.bottom-mInf.rcMonitor.top;
  	aspect=1.0f*sw/sh;
  
    if (w*sh>h*sw)
    {
      const sF32 sc = 1.0f * (h*sw) / (sh*w);

      sw = sc;
      sh = sc * w / h;
    }
    else
    {
      const sF32 sc = aspect * (w * sh) / (sw * h);

      sw = sc * h / w;
      sh = sc;
    }
  }

  g_engine->clearStats();

  if (!camMode)
  {
    // setup paint
    matrix cam;
    CalcCamMatrix(cam);

    clear(D3DCLEAR_ZBUFFER|D3DCLEAR_TARGET, m_wireframe?0xaea9a7:0x000000, 1.0f);

    // calc paint flags
    sU32 flags = frRG2Engine::pfShowVirtObjs;

    if (m_wireframe)
      flags |= frRG2Engine::pfWireframe;

    if (m_showCull)
      flags |= frRG2Engine::pfShowCull;

    // actual paint
    frModel *model=lockModel();
    g_engine->paintModel(model, cam, sw, sh, flags);
    unlockModel();
  }
  else
  {
		matrix cam;
		CalcCamMatrix(cam);

    setViewport(0, 0, fvs::conf.xRes, fvs::conf.yRes);
    clear();

    composeViewport vp;

    // in noScreen (i.e. render) mode, we trust all metrics we have to make sense.
    // in display mode, however, we want to make sure the window we are rendering in
    // has the desired 4:3 aspect ratio.

    if (noScreen)
    {
      vp.xstart = 0;
      vp.ystart = 0;
      vp.w = fvs::conf.xRes;
      vp.h = fvs::conf.yRes;
    }
    else
    {
      const sF32 targetAspect = 1.333f;
      const sF32 ws = fvs::conf.xRes / targetAspect;
      const sF32 hs = fvs::conf.yRes;

      // choose the smaller dimension to limit the viewport size
      const sF32 targetSize = (ws < hs) ? ws : hs;

      vp.w = targetSize * targetAspect;
      vp.h = targetSize;
      vp.xstart = (fvs::conf.xRes - vp.w) / 2;
      vp.ystart = (fvs::conf.yRes - vp.h) / 2;

      sw = 1.0f;
      sh = targetAspect;
    }

		if (m_camMode == 1)
			vp.camOverride = &cam;
		else
			vp.camOverride = 0;

    camMode->paint(vp, sw, sh);
  }

  if (!noScreen)
  {
    sU32 vertCount, triCount, lineCount, partCount, objCount, staticObjCount,
			mtrlChCount;

    g_engine->getStats(vertCount, lineCount, triCount, partCount, objCount,
			staticObjCount, mtrlChCount);

    static TCHAR txt[256];
    wsprintf(txt, "%d verts, %d tris, %d lines, %d particles, %d objects "
			"(%d%% static), %d material changes", vertCount, triCount, lineCount,
			partCount, objCount, objCount ? MulDiv(staticObjCount, 100, objCount) : 0,
			mtrlChCount);

    ::PostMessage(g_winMap.Get(IDR_MAINFRAME), WM_SET_STATUS_MESSAGE, 0, (LPARAM) txt);
  }

  updateScreen();

  if (!noScreen)
    ValidateRect(0);
}

// ---- tools

frModel *C3DPreviewClientWindow::lockModel() const
{
  if (g_graph->m_ops.find(m_curOp)==g_graph->m_ops.end())
    return 0;

  frOpGraph::frOperator& op=g_graph->m_ops[m_curOp];
  frGeometryPlugin *plg=0;

  switch (op.def->type)
  {
  case 1:
    plg=(frGeometryPlugin *) op.plg;
    break;

  case 2:
    return 0; // not supported anymore!
  }

  if (!plg)
    return 0;

  plg->lock();

  return plg->getData();
}

void C3DPreviewClientWindow::unlockModel() const
{
  if (g_graph->m_ops.find(m_curOp)==g_graph->m_ops.end())
    return;

  frOpGraph::frOperator& op=g_graph->m_ops[m_curOp];
  frGeometryPlugin *plg=0;
  
  switch (op.def->type)
  {
  case 1:
    plg=(frGeometryPlugin *) op.plg;
    break;
    
  case 2:
    return; // not supported anymore!
  }
  
  if (!plg)
    return;
  
  plg->unlock();
}

// ---- statusvariablen

void C3DPreviewClientWindow::ResetCamera()
{
  m_camPos=vector(0.0f, 0.0f, -5.0f);
  
  m_camRotH=1.0f;
  m_camRotP=0.0f;

  if (m_engineInitialized)
    PaintModel();
}

void C3DPreviewClientWindow::CalcCamMatrix(matrix &cam)
{
  matrix mat1, mat2;
  
  mat1.rotateHPB((m_camRotH-1.0f)*3.1415926535f, m_camRotP*3.1415926535f, 0.0f);
  mat2.translate(-m_camPos.x, -m_camPos.y, -m_camPos.z);
  cam.mul(mat2, mat1);
}

LRESULT C3DPreviewClientWindow::OnGetThis(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  FRASSERT(lParam != 0);

  C3DPreviewClientWindow **ptr=(C3DPreviewClientWindow**) lParam;
  *ptr = this;
  return 0;
}

LRESULT C3DPreviewClientWindow::OnStoreData(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	sF32* data = (sF32*) lParam;

	switch (wParam)
	{
	case 0: // get camera position
		data[0] = m_camPos.x;
		data[1] = m_camPos.y;
		data[2] = m_camPos.z;
		break;

	case 1: // get camera rotation
		data[0] = m_camRotH - 1.0f;;
		data[1] = m_camRotP;
		data[2] = 0.0f;
		break;
	}

	return 0;
}

void C3DPreviewClientWindow::SetWireframe(sBool wireframe)
{
  if (m_wireframe!=wireframe)
  {
    m_wireframe=wireframe;
    PaintModel();
  }
}

void C3DPreviewClientWindow::SetShowCull(sBool showCull)
{
  if (m_showCull != showCull)
  {
    m_showCull=showCull;
    PaintModel();
  }
}

void C3DPreviewClientWindow::SetCamMode(sInt camMode)
{
	m_camMode = camMode;
}

// ---- und das main window

LRESULT C3DPreviewWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_client.Create(m_hWnd, rcDefault, 0, WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN, 0);

  CFontHandle fnt=AtlGetDefaultGuiFont();

  m_wireframe.Create(m_hWnd, CRect(2, 0, 66, 20), _T("wireframe"), WS_CHILD|WS_VISIBLE|FRBS_AUTO3D_SINGLE|FRBS_CHECK|FRBS_HOTTRACK|FRBS_SOLID|FRBS_AUTO, 0, 100);
  m_wireframe.SetFont(fnt);
  m_wireframe.SetCheck(sFALSE);
  m_wireframe.SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
    RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));

  m_showCull.Create(m_hWnd, CRect(66, 0, 130, 20), _T("show cull"), WS_CHILD|WS_VISIBLE|FRBS_AUTO3D_SINGLE|FRBS_CHECK|FRBS_HOTTRACK|FRBS_SOLID|FRBS_AUTO, 0, 102);
  m_showCull.SetFont(fnt);
  m_showCull.SetCheck(sFALSE);
  m_showCull.SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
    RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));

  m_resetCam.Create(m_hWnd, CRect(132, 0, 196, 20), _T("reset cam"), WS_CHILD|WS_VISIBLE|FRBS_FLAT|FRBS_HOTTRACK, 0, 101);
  m_resetCam.SetFont(fnt);
  m_resetCam.SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
    RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));

	m_camMode.Create(m_hWnd, CRect(198, 0, 326, 20), 0, WS_CHILD|WS_VISIBLE|FRBS_FLAT|FRBS_HOTTRACK, 0, 200);
	m_camMode.SetFont(fnt);
  m_camMode.SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
    RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));
	m_camMode.AddMenuItem(1, "stay on playcam");
	m_camMode.AddMenuItem(2, "free fly");
	m_camMode.SetSelection(1);
  
  bHandled=FALSE;
  return 0;
}

LRESULT C3DPreviewWindow::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  bHandled=FALSE;
  return 0;
}

LRESULT C3DPreviewWindow::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CRect rc;
  
  GetClientRect(&rc);
  rc.top=21;
  rc.DeflateRect(1, 1);
  m_client.SetWindowPos(0, &rc, SWP_NOZORDER);
  
  bHandled=FALSE;
  return 0;
}

LRESULT C3DPreviewWindow::OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CDCHandle dc=(HDC) wParam;
  RECT rc;
  
  GetClientRect(&rc);
  rc.bottom=rc.top+21;
  dc.FillSolidRect(&rc, RGB(174, 169, 167));
  
  CFontHandle oldFont=dc.SelectFont(AtlGetDefaultGuiFont());
  dc.SetTextColor(RGB(0, 0, 0));
  dc.SetBkMode(TRANSPARENT);
  
  const sChar *txt="(no view item)";
  
  if (g_graph->m_ops.find(m_client.m_curOp)!=g_graph->m_ops.end())
    txt=g_graph->m_ops[m_client.m_curOp].plg->getDisplayName();
  
  dc.TextOut(198, 3, txt);
  dc.SelectFont(oldFont);

  GetClientRect(&rc);
  rc.top+=21;
  dc.Draw3dRect(rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, RGB(103, 99, 96), RGB(200, 199, 196));
  
  return 1;
}

LRESULT C3DPreviewWindow::OnGetThis(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  return ::SendMessage(m_client.m_hWnd, WM_GET_THIS, wParam, lParam);
}

LRESULT C3DPreviewWindow::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (LOWORD(wParam) == 100) // wireframe
    m_client.SetWireframe(!!m_wireframe.GetCheck());
  else if (LOWORD(wParam) == 101) // reset cam
    m_client.ResetCamera();
  else if (LOWORD(wParam) == 102) // show cull
    m_client.SetShowCull(!!m_showCull.GetCheck());
	else if (LOWORD(wParam) == 200) // cam mode
		m_client.SetCamMode(m_camMode.GetSelection() - 1);
  else
    bHandled=FALSE;
  
  return 0;
}

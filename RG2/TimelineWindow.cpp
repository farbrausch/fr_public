// This code is in the public domain. See LICENSE for details.

// TimelineWindow.cpp: implementation of the CTimelineWindow class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TimelineWindow.h"
#include "animbase.h"
#include "frOpGraph.h"
#include "resource.h"
#include "viewcomm.h"
#include "WindowMapper.h"
#include "ViruzII.h"
#include "VideoDlg.h"
#include "directx.h"
#include "3DPreviewWindow.h"
#include "plugbase.h"
#include "guitool.h"
#include <math.h>

#include <mmsystem.h>
#include <vfw.h>

#pragma comment(lib, "vfw32.lib")

// ---- CRenameClipDialog

class CRenameClipDialog: public CDialogImpl<CRenameClipDialog>
{
public:
  enum { IDD=IDD_RENAMECLIP };

  BEGIN_MSG_MAP(CRenameClipDialog)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDOK, OnEnd)
    COMMAND_ID_HANDLER(IDCANCEL, OnEnd)
  END_MSG_MAP()

  fr::string m_name, m_origName;

  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    SetDlgItemText(IDC_NAME, m_name);
    SendDlgItemMessage(IDC_NAME, EM_SETSEL, 0, -1);
    
    ::SetFocus(GetDlgItem(IDC_NAME));
    CenterWindow(GetParent());

    return 0;
  }

  LRESULT OnEnd(WORD wNotifyID, WORD wID, HWND hWndCtl, BOOL& bHandled)
  {
    if (wID == IDOK)
    {
      GetDlgItemText(IDC_NAME, m_name.getBuffer(256), 255);
      m_name.releaseBuffer();

      if (m_name != m_origName)
      {
        if (m_name.isEmpty() || m_name.compare("(no clip)", sTRUE) == 0 || g_graph->m_clips->findClipByName(m_name))
        {
          MessageBox(_T("Invalid name"), _T("RG2"), MB_ICONERROR|MB_OK);
          return 0;
        }
      }
    }

    EndDialog(wID == IDOK);

    return 0;
  }

  void SetName(const sChar* name)
  {
    m_name = name;
    m_origName = m_name;
  }

  const fr::string& GetName() const
  {
    return m_name;
  }
};

// ---- CTimelineButton

void CTimelineButton::SetBitmap(HBITMAP bmp)
{
  m_bmp = bmp;
}

void CTimelineButton::SetIndex(sInt ind)
{
  m_buttonInd = ind;
}

void CTimelineButton::DoPaint(CDCHandle dc)
{
  CDC tempDC;
  tempDC.CreateCompatibleDC(dc);

  unsigned int down = IsCheck() ? m_fChecked : m_fPressed;

  CBitmapHandle oldBitmap = tempDC.SelectBitmap(m_bmp);
  dc.BitBlt(0, 0, 25, 25, tempDC, down * 25 + m_fMouseOver * 50, m_buttonInd * 25, SRCCOPY);
  tempDC.SelectBitmap(oldBitmap);
}

// ---- CTimelineWindow

CTimelineWindow::CTimelineWindow()
	: m_playState(0), m_curClip(0), m_clip(0), m_timeBarPos(-1), m_lButtonDown(sFALSE), m_lastPlayFrame(0), m_curFrame(0),
	m_isSimpleClip(sFALSE), m_clipLength(0), m_fullClipLength(0)
{
}

CTimelineWindow::~CTimelineWindow()
{
}

sInt g_lastCompositingOp;

BOOL CTimelineWindow::OnIdle()
{
  sS32 currentFrame = getTime();

  if (updateClipPtr() && m_playState != 0 && !m_lButtonDown)
  {
		const sInt clipLen = m_clipLength ? m_clipLength : 2000;
    const sInt startFrame = m_startFrame.GetValue() * 1000;
		sInt realFrame = currentFrame - (m_isSimpleClip ? startFrame : 0);

    if (realFrame >= clipLen)
    {
			m_curFrame = startFrame;
      SetPlayState(m_buttons[3].GetCheck() ? 1 : 0); // restart playing when looping
    }
		else
			setCurrentFrame(realFrame + (m_isSimpleClip ? startFrame : 0));
  }

	return FALSE;
}

LRESULT CTimelineWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_buttonsBmp = AtlLoadBitmap(MAKEINTRESOURCE(IDB_TLBUTTONS));

  static const sBool  isCheck[6] = { sFALSE, sFALSE, sFALSE, sTRUE, sFALSE, sFALSE };
  static sInt         bitmaps[6] = { 0, 1, 5, 3, 2, 4 };

  for (sInt i = 0; i < 6; i++)
  {
    sU32 checked = isCheck[i] ? FRBS_AUTO | FRBS_CHECK : 0;

    m_buttons[i].Create(m_hWnd, rcDefault, 0, FRBS_HOTTRACK | WS_CHILD | WS_VISIBLE | checked, 0, i + 100);
    m_buttons[i].SetBitmap(m_buttonsBmp);
    m_buttons[i].SetIndex(bitmaps[i]);
    m_buttons[i].SetCheck(isCheck[i]);
  }

  m_clipName.Create(m_hWnd, rcDefault, 0, CBS_DROPDOWNLIST | CBS_AUTOHSCROLL | WS_CHILD | WS_VISIBLE | CBS_HASSTRINGS | CBS_SORT | WS_VSCROLL, 0, 200);
  m_clipName.SetFont(AtlGetDefaultGuiFont());

  m_clipNew.Create(m_hWnd, rcDefault, _T("New"), FRBS_HOTTRACK | FRBS_FLAT | WS_CHILD | WS_VISIBLE, 0, 302);
  m_clipNew.SetFont(AtlGetDefaultGuiFont());
  m_clipNew.SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
    RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));
  
  m_clipRename.Create(m_hWnd, rcDefault, _T("Rename"), FRBS_HOTTRACK | FRBS_FLAT | WS_CHILD | WS_VISIBLE, 0, 300);
  m_clipRename.SetFont(AtlGetDefaultGuiFont());
  m_clipRename.SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
    RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));

  m_clipDelete.Create(m_hWnd, rcDefault, _T("Delete"), FRBS_HOTTRACK | FRBS_FLAT | WS_CHILD | WS_VISIBLE, 0, 301);
  m_clipDelete.SetFont(AtlGetDefaultGuiFont());
  m_clipDelete.SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
    RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));

  m_renderVideo.Create(m_hWnd, rcDefault, _T("Render Video"), FRBS_HOTTRACK | FRBS_FLAT | WS_CHILD | WS_VISIBLE, 0, 303);
  m_renderVideo.SetFont(AtlGetDefaultGuiFont());
  m_renderVideo.SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
    RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));

  m_sound.Create(m_hWnd, rcDefault, _T("Sound"), FRBS_HOTTRACK | FRBS_AUTO3D_SINGLE | FRBS_SOLID | FRBS_CHECK | WS_CHILD | WS_VISIBLE, 0, 150);
  m_sound.SetFont(AtlGetDefaultGuiFont());
  m_sound.SetColorScheme(RGB(0, 0, 0), RGB(0, 0, 255), RGB(66, 65, 64), RGB(103, 99, 96),
    RGB(174, 169, 167), RGB(174, 169, 167), RGB(200, 198, 196));
  m_sound.SetCheck(0);

  m_startFrame.Create(m_hWnd, rcDefault, 0, WS_CHILD | WS_VISIBLE | FRNES_ALIGN_CENTER | FRNES_IMMEDIATEUPDATE | WS_CLIPSIBLINGS, WS_EX_STATICEDGE, 400);
  m_startFrame.SetFont(AtlGetDefaultGuiFont());
  m_startFrame.SetMin(0);
  m_startFrame.SetMax(1e+15);
  m_startFrame.SetStep(0.001);
  m_startFrame.SetPrecision(3);
  
  m_bpmRate.Create(m_hWnd, rcDefault, 0, WS_CHILD | WS_VISIBLE | FRNES_ALIGN_CENTER | FRNES_IMMEDIATEUPDATE | WS_CLIPSIBLINGS, WS_EX_STATICEDGE, 500);
  m_bpmRate.SetFont(AtlGetDefaultGuiFont());
  m_bpmRate.SetMin(0);
  m_bpmRate.SetMax(320);
  m_bpmRate.SetStep(0.1);
  m_bpmRate.SetPrecision(0);
  m_bpmRate.SetValue(g_graph->m_bpmRate);
  
  CMessageLoop* pMessageLoop = _Module.GetMessageLoop();
  pMessageLoop->AddIdleHandler(this);

  bHandled = FALSE;
  return 0;
}

LRESULT CTimelineWindow::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CMessageLoop* pMessageLoop = _Module.GetMessageLoop();
  pMessageLoop->RemoveIdleHandler(this);
  
  bHandled = FALSE;
  return 0;
}

LRESULT CTimelineWindow::OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CDCHandle dc = (HDC) wParam;
  
  RECT rc;
  GetClientRect(&rc);
  dc.FillSolidRect(&rc, RGB(174, 169, 167));

  return 1;
}

LRESULT CTimelineWindow::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	drawTimeBar(1);

  CPaintDC dc(m_hWnd);
  
  CRect rc;
  GetClientRect(&rc);

  CFontHandle oldFont = dc.SelectFont(AtlGetDefaultGuiFont());

  dc.SetTextAlign(TA_LEFT);
  dc.SetTextColor(0x000000);
  dc.SetBkMode(TRANSPARENT);
  dc.TextOut(10 + rc.right / 15, 35, "BPM Rate:", 9);
  dc.SelectFont(oldFont);
    
  rc.left += 4;
  rc.top += 4;
  rc.right = (rc.right * 7) / 10 - 6;
  rc.bottom = 31;

  dc.Draw3dRect(&rc, RGB(103, 99, 96), RGB(200, 198, 196));
  rc.DeflateRect(1, 1);
  dc.FillSolidRect(&rc, RGB(128, 128, 128));

	if (updateClipPtr())
	{
		CRect mapRect = rc;
		mapRect.left += 6;
		mapRect.right -= 6;

		const sInt clipLength = fr::maximum(1, m_fullClipLength);
		const sInt lenDigits = ceil(log10(clipLength*1.0));
		const sInt mapWidth = mapRect.right - mapRect.left;
		const sInt step = ceil(gridMapStep(clipLength, mapWidth / (10 + lenDigits * 27), 10.0f));
		const sInt count = (clipLength / step) + ((clipLength % step) != 0);

		dc.SetTextColor(RGB(0, 0, 0));
		dc.SetBkMode(TRANSPARENT);

		CFontHandle newFont = AtlGetDefaultGuiFont();
		LOGFONT lf;
		newFont.GetLogFont(lf);

		CFontHandle oldFont = dc.SelectFont(newFont);

		for (sInt i = 0; i <= count; i++)
		{
			const sInt value = (i != count) ? i * step : clipLength;

			if ((i == count - 1) && ((clipLength - i * step) * mapWidth / clipLength) < 30)
				continue;

			TCHAR buf[80];
			sprintf(buf, "%.3f", value * 0.001f);

			const sInt xpos = (value * mapWidth + (clipLength >> 1)) / clipLength + mapRect.left;

			sU32 align = TA_BOTTOM;
			sInt adj = 0;

			if (i == 0)
			{
				align |= TA_LEFT;
				adj = -1;
			}
			else if (i == count)
			{
				align |= TA_RIGHT;
				adj = 1;
			}
			else
				align |= TA_CENTER;

			dc.SetTextAlign(align);
			dc.TextOut(xpos + adj, mapRect.bottom, buf);
		}

		dc.SelectFont(oldFont);

		const sInt lineStep = ceil(gridMapStep(clipLength, mapWidth / 7, 10.0f));
		const sInt lineCount = (clipLength / lineStep) + ((clipLength % lineStep) != 0);

		CPen blackPen;
		blackPen.CreatePen(PS_SOLID, 1, RGB(0, 0, 0));

		CPenHandle oldPen = dc.SelectPen(blackPen);

		const sInt baseY = mapRect.bottom - MulDiv(-lf.lfHeight, dc.GetDeviceCaps(LOGPIXELSY), 72);
		const sInt baseLen = baseY - mapRect.top;

		for (sInt i = 0; i <= lineCount; i++)
		{
			const sInt value = (i != lineCount) ? i * lineStep : clipLength;
			const sInt xpos = (value * mapWidth + (clipLength >> 1)) / clipLength + mapRect.left;

			sInt len = baseLen*2;

			if ((value % 1000) == 0)
				len += baseLen;

			if ((value % step) == 0 || (i == lineCount))
				len += baseLen;

			len >>= 2;

			dc.MoveTo(xpos, baseY);
			dc.LineTo(xpos, baseY - len);
		}

		dc.SelectPen(oldPen);
	}

  drawTimeBar(2);

  return 0;
}

LRESULT CTimelineWindow::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  const sInt baseXPos = (GET_X_LPARAM(lParam) * 7 / 10 - 6 - 5 * 25 - 60) / 2;

  for (sInt i = 1; i < 6; i++)
    m_buttons[i].SetWindowPos(0, baseXPos + (i - 1) * 25, 33, 25, 25, SWP_NOZORDER);

  m_sound.SetWindowPos(0, baseXPos + 5 * 25, 33, 60, 25, SWP_NOZORDER);

  RECT rc;
  GetClientRect(&rc);

  const sInt xpos = (rc.right * 7) / 10 - 4;
  const sInt wtotal = rc.right * 3 / 10;
  const sInt wsingle = wtotal / 4;
  m_clipName.SetWindowPos(0, xpos, 4, wtotal, 520, SWP_NOZORDER);
  m_clipNew.SetWindowPos(0, xpos, 28, wsingle, 20, SWP_NOZORDER);
  m_clipRename.SetWindowPos(0, xpos + wsingle, 28, wsingle, 20, SWP_NOZORDER);
  m_clipDelete.SetWindowPos(0, xpos + wsingle * 2, 28, wsingle, 20, SWP_NOZORDER);
  m_renderVideo.SetWindowPos(0,  xpos + wsingle * 3, 28, wsingle, 20, SWP_NOZORDER);

  m_startFrame.SetWindowPos(0, 4, 34, rc.right / 15, 16, SWP_NOZORDER);
  m_bpmRate.SetWindowPos(0, 4 + rc.right / 15 + 60, 34, rc.right / 15, 16, SWP_NOZORDER);
  
  bHandled = FALSE;
  return 0;
}

LRESULT CTimelineWindow::OnSetCurrentClip(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (!lParam && g_graph->m_clips->m_names.size())
    lParam = g_graph->m_clips->m_names.begin()->second;

  SetCurrentClip(lParam);
  
  return 0;
}


LRESULT CTimelineWindow::OnSetCurrentFrame(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  setCurrentFrame(lParam);  
  return 0;
}


LRESULT CTimelineWindow::OnClipChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (updateClipPtr())
	{
		m_clip->updateLength();
		m_clipLength = m_clip->getLength(); // only thing that can change that is interesting to us.
		m_fullClipLength = m_clipLength + (m_isSimpleClip ? m_startFrame.GetValue() * 1000.0f : 0);
	}

	Invalidate();
	UpdateWindow();

  return 0;
}

LRESULT CTimelineWindow::OnGetClipID(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  return m_curClip;
}

LRESULT CTimelineWindow::OnControlColorEdit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	SetBkColor((HDC) wParam, RGB(174, 169, 167));
  return 0;
}

LRESULT CTimelineWindow::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  SetCapture();
  processScratch(CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
  m_lButtonDown = sTRUE;

  return 0;
}

LRESULT CTimelineWindow::OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (::GetCapture() == m_hWnd)
  {
    ::ReleaseCapture();
    
    if (m_lButtonDown)
    {
      processScratch(CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
      m_lButtonDown = sFALSE;
    }
  }

  return 0;
}

LRESULT CTimelineWindow::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CPoint ptCursor(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

  if (wParam & MK_LBUTTON)
  {
    processScratch(ptCursor);
    m_lButtonDown = sTRUE;
  }
  else
    m_lButtonDown = sFALSE;

  return 0;
}

LRESULT CTimelineWindow::OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_lButtonDown = sFALSE;
  return 0;
}

LRESULT CTimelineWindow::OnFileLoaded(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_bpmRate.SetValue(g_graph->m_bpmRate);
  return 0;
}

LRESULT CTimelineWindow::OnPlay(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wID != 101 || !updateClipPtr()) // backward disabled for now
    return 0;

	sInt newPlayState = (wID == 101) ? 1 : 2;

	SetPlayState(newPlayState != m_playState ? newPlayState : 0); // forward/backward
  
	if (wID != 101)
		m_curFrame = m_clipLength;

	m_clip->applyBasePoses();

  return 0;
}

LRESULT CTimelineWindow::OnSound(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (m_playState != 0)
  {
    const sInt curTime = getTime();

    if (m_sound.GetCheck()) // sound currently on
    {
      g_viruz->stop();
      
      LARGE_INTEGER time, freq;
      QueryPerformanceFrequency(&freq);
      QueryPerformanceCounter(&time);

      m_startTime = time.QuadPart - sS64(curTime) * freq.LowPart / 1000;
    }
    else
      g_viruz->play(curTime);
  }

  m_sound.SetCheck(!m_sound.GetCheck());

  return 0;
}

LRESULT CTimelineWindow::OnFrame(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  SetPlayState(0); // stop
  
	if (updateClipPtr())
	{
		sInt newFrame;
		const sInt startFrame = m_startFrame.GetValue() * 1000;
		const sInt endFrame = m_fullClipLength - 1;

		if (wID != 102)
			newFrame = endFrame;
		else
			newFrame = startFrame;

		setCurrentFrame(newFrame);
	}
  
  return 0;
}

LRESULT CTimelineWindow::OnClipSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  const sInt curSel = m_clipName.GetCurSel();
  const sInt textLen = m_clipName.GetLBTextLen(curSel);
  
  fr::string clipName;
  m_clipName.GetLBText(curSel, clipName.getBuffer(textLen));
  clipName.releaseBuffer();

  const frAnimationClip* curClip = g_graph->m_clips->findClipByName(clipName);
  const sU32 clipID = curClip ? curClip->getID():0;
  
  if (clipID != m_curClip)
    SetCurrentClip(clipID);
  
  return 0;
}

LRESULT CTimelineWindow::OnClipRename(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	if (updateClipPtr())
	{
		CRenameClipDialog dlg;

		dlg.SetName(m_clip->getName());
		if (dlg.DoModal())
		{
			m_clip->setName(dlg.GetName());
			SetCurrentClip(m_curClip); // req'd??
			g_graph->modified();
		}
	}

  return 0;
}

LRESULT CTimelineWindow::OnClipDelete(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (m_curClip && MessageBox(_T("Really delete clip?"), _T("RG2"), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES)
  {
    const sU32 cclip = m_curClip;
    
    SetCurrentClip(0);
    g_graph->m_clips->deleteClip(cclip);
    g_graph->modified();

    fillClipComboBox();
    ::SendMessage(g_winMap.Get(ID_SPLINE_WINDOW), WM_SET_EDIT_SPLINE, 0, 0);
    m_clipName.SelectString(-1, "(no clip)");
  }

  return 0;
}

LRESULT CTimelineWindow::OnClipNew(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  SetCurrentClip(g_graph->m_clips->addClip());
  g_graph->modified();

  return 0;
}

static void fastMemCpy(void* dst, void* src, int nbytes) // only use for large chunks of mem
{
  static sU8 tbuf[2048];

  __asm 
  {
    mov esi, src;
    mov edi, dst;
    mov ecx, nbytes;
    shr ecx, 6; // 64 bytes per iteration 
    jz nomid;

loopnb: 
    movq mm1,  0[ESI]; // Read in source data 
    movq mm2,  8[ESI]; 
    movq mm3, 16[ESI]; 
    movq mm4, 24[ESI]; 
    movq mm5, 32[ESI]; 
    movq mm6, 40[ESI]; 
    movq mm7, 48[ESI]; 
    movq mm0, 56[ESI]; 

    movntq  0[EDI], mm1; // Non-temporal stores 
    movntq  8[EDI], mm2; 
    movntq 16[EDI], mm3; 
    movntq 24[EDI], mm4; 
    movntq 32[EDI], mm5; 
    movntq 40[EDI], mm6; 
    movntq 48[EDI], mm7; 
    movntq 56[EDI], mm0; 

    add esi, 64;
    add edi, 64;
    dec ecx;
    jnz loopnb;

nomid:
    mov ecx, nbytes;
    and ecx, 63;
    shr ecx, 2; // 4bytes/it
    jz nosmall;

    rep movsd;

nosmall:
    mov ecx, nbytes;
    and ecx, 3; // tail loop
    jz notiny;

    rep movsb;

notiny:
    emms
  } 
}

LRESULT CTimelineWindow::OnRenderVideo(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	if (!m_curClip)
		return 0;

  CVideoDialog dlg;
	if (dlg.DoModal()==IDOK)
	{
		PAVIFILE avifile=0;
		PAVISTREAM vstr=0,vstrc=0,astr=0,astrc=0;
		AVISTREAMINFO vstrh,astrh;
		AVICOMPRESSOPTIONS vopts,aopts;

		BITMAPINFOHEADER bmi;
		ZeroMemory(&bmi,sizeof(bmi));
		bmi.biSize=sizeof(bmi);
		bmi.biWidth=dlg.config.xres;
		bmi.biHeight=dlg.config.yres;
		bmi.biPlanes=1;
		bmi.biBitCount=24;
		bmi.biCompression=BI_RGB;
		bmi.biSizeImage=((bmi.biWidth*bmi.biBitCount+31)/32 * 4)*bmi.biHeight;

		sInt framecount=0;
		
		if (dlg.config.isavi)
		{
			AVIFileInit();

			if (AVIFileOpen(&avifile,dlg.config.filename,OF_WRITE|OF_CREATE,NULL) != AVIERR_OK)
			{
				MessageBox("Error opening the AVI file!","rg2 video export thingie",MB_OK|MB_ICONERROR);
				goto ente1;
			}

			// Fill in the header for the video stream....
			ZeroMemory(&vstrh, sizeof(vstrh));
			vstrh.fccType                = streamtypeVIDEO;	// video stream type
			vstrh.dwScale                = 10000;
			vstrh.dwRate                 = (sU32)((10000*dlg.config.fps*(dlg.config.fieldsenable + 1))+0.5);
			vstrh.dwSuggestedBufferSize  = dlg.config.xres*dlg.config.yres*3;
			SetRect(&vstrh.rcFrame, 0, 0, dlg.config.xres, dlg.config.yres);
			strcpy(vstrh.szName,"Video");


			// Step 3 : Create the stream;
	    if (AVIFileCreateStream(avifile,&vstr, &vstrh) != AVIERR_OK)
			{
				MessageBox("Error creating the video stream!","rg2 video export thingie",MB_OK|MB_ICONERROR);
				goto ente2;
			}

		//	AVIStreamSetFormat(vstr,0,&bmi,sizeof(bmi));

			if (dlg.config.audio)
			{
					// Fill in the header for the audio stream....
					ZeroMemory(&astrh, sizeof(astrh));
					astrh.fccType                = streamtypeAUDIO;
					astrh.dwScale                = 4;
					astrh.dwRate                 = 44100*4;
					astrh.dwSuggestedBufferSize  = 0;
					astrh.dwSampleSize           = 4;
					strcpy(astrh.szName,"Audio");
					
					// Step 3 : Create the stream;
					if (AVIFileCreateStream(avifile,&astr, &astrh) != AVIERR_OK)
					{
						MessageBox("Error creating the audio stream!","rg2 video export thingie",MB_OK|MB_ICONERROR);
						goto ente2;
					}
					
					WAVEFORMATEX wfx;
					wfx.wFormatTag=WAVE_FORMAT_PCM;
					wfx.nChannels=2;
					wfx.nSamplesPerSec=44100;
					wfx.nAvgBytesPerSec=176400;
					wfx.nBlockAlign=4;
					wfx.wBitsPerSample=16;
					wfx.cbSize=0;
					AVIStreamSetFormat(astr,0,&wfx,sizeof(wfx));

			}

			// Step 4: Get codec and infos about codec
			ZeroMemory(&vopts, sizeof(vopts));
			ZeroMemory(&aopts, sizeof(aopts));

			PAVISTREAM strs[2] = {vstr,astr};
			AVICOMPRESSOPTIONS FAR * opts[2] = {&vopts,&aopts};
			const sInt nstreams=dlg.config.audio?2:1;

			if (!AVISaveOptions(GetForegroundWindow(), 0, nstreams, strs, (LPAVICOMPRESSOPTIONS FAR *) &opts))
			{
				AVISaveOptionsFree(nstreams,(LPAVICOMPRESSOPTIONS FAR *) &opts);
				goto ente2;
			}

			// Create a compressed stream using codec options.
	    if (AVIMakeCompressedStream(&vstrc, vstr, &vopts, NULL) != AVIERR_OK)
			{
				AVISaveOptionsFree(1,(LPAVICOMPRESSOPTIONS FAR *) &opts);
				MessageBox("Error creating the compressed video stream!","rg2 video export thingie",MB_OK|MB_ICONERROR);
				goto ente2;
			}

			AVIStreamSetFormat(vstrc,0,&bmi,sizeof(bmi));

			if (dlg.config.audio)
			{
		    if (AVIMakeCompressedStream(&astrc, astr, &aopts, NULL) != AVIERR_OK)
				{
					AVISaveOptionsFree(1,(LPAVICOMPRESSOPTIONS FAR *) &opts);
					MessageBox("Error creating the compressed audio stream!","rg2 video export thingie",MB_OK|MB_ICONERROR);
					goto ente2;
				}
			}

			AVISaveOptionsFree(nstreams,(LPAVICOMPRESSOPTIONS FAR *) &opts);

		}

			

		// aufi gehts!
		sU32 rt=fvs::newRenderTarget(dlg.config.xres,dlg.config.yres,sTRUE,sTRUE);
		sInt rw,rh;
		fvs::getRenderTargetDimensions(rt,0,0,&rw,&rh);
		fr::debugOut("my rt is %d (%dx%d)\n",rt,rw,rh);

    sF32 startFrame = m_startFrame.GetValue();
		sF32 endFrame = updateClipPtr() ? m_clip->getLength() * 0.001f : 0.0f;

		C3DPreviewClientWindow *prv3d;
		::SendMessage(g_winMap.Get(ID_3DPREVIEW_WINDOW), WM_GET_THIS, 0, (LPARAM)&prv3d);
		fr::debugOut("rendering window %08x, start %3.3f, end %3.3f\n",prv3d,startFrame,endFrame);

		sInt nf=(sInt)((endFrame-startFrame)*dlg.config.fps + 0.999);

		sU32 *imgbuf[2];
		sU8 *bmpbuf = new sU8[dlg.config.xres*dlg.config.yres*3];
		imgbuf[0]=new sU32[dlg.config.xres*dlg.config.yres];
		if (dlg.config.fieldsenable) imgbuf[1]=new sU32[dlg.config.xres*dlg.config.yres];

		sInt oldxres=fvs::conf.xRes;
		sInt oldyres=fvs::conf.yRes;
		fvs::conf.xRes=dlg.config.xres;
		fvs::conf.yRes=dlg.config.yres;
	
		for (int frame=0; frame<nf; frame++) for (int field=0; field<dlg.config.fieldsenable+1; field++)
		{
			// set engine to right frame
			sF32 timecode=1000.0*(startFrame+(frame+0.5f*field)*(endFrame-startFrame)/(sF32)nf);
			fr::debugOut("frame %d/%d , field %d at %3.3f\n",frame,nf,field,timecode);
			setCurrentFrame((sInt)timecode,sFALSE,sTRUE);

			// render!
			fvs::setRenderTarget(rt);
			fvs::setViewport(0,0,dlg.config.xres,dlg.config.yres);
			prv3d->PaintModel(sTRUE,dlg.config.xres,dlg.config.yres,dlg.config.aspect);

			sU32 *rtpix=fvs::lockRenderTarget(rt);

			sU32 *dst=imgbuf[field];
			for (int yy=0; yy<dlg.config.yres; yy++)
			{
        fastMemCpy(dst, rtpix, sizeof(sU32) * dlg.config.xres);

				dst+=dlg.config.xres;
				rtpix+=rw;
			}
			fvs::unlockRenderTarget(rt);

			sU8 *destptr=bmpbuf+(dlg.config.yres-1)*dlg.config.xres*3;
			for (int yy=0; yy<dlg.config.yres; yy++)
			{
				sU32 *srcptr=imgbuf[dlg.config.fieldsenable?(yy^dlg.config.fieldsmode)&1:0]+dlg.config.xres*yy;
				for (int xx=0; xx<dlg.config.xres; xx++)
				{
					*destptr++=((sU8*)(srcptr+xx))[0];
					*destptr++=((sU8*)(srcptr+xx))[1];
					*destptr++=((sU8*)(srcptr+xx))[2];
				}
				destptr-=6*dlg.config.xres;
			}

			if (dlg.config.isavi)
			{
				AVIStreamWrite(vstrc,framecount++,1,bmpbuf,3*dlg.config.xres*dlg.config.yres,0,0,0);
			}
		}

		fvs::releaseRenderTarget(rt);
		fvs::conf.xRes=oldxres;
		fvs::conf.yRes=oldyres;
		fvs::resizeBackbuffer(fvs::conf.xRes,fvs::conf.yRes);
		fvs::setViewport(0,0,fvs::conf.xRes,fvs::conf.yRes);

		delete[] imgbuf[0];
		if (dlg.config.fieldsenable) delete[] imgbuf[1];
    delete[] bmpbuf;

ente2:
		if (dlg.config.isavi)
		{
			if (vstr) AVIStreamRelease(vstr);
			if (astr) AVIStreamRelease(astr);
			if (vstrc) AVIStreamRelease(vstrc);
			if (astrc) AVIStreamRelease(astrc);
			AVIFileRelease(avifile);
			AVIFileExit();
		}

		
ente1: ;


	}
	return 0;
}

LRESULT CTimelineWindow::OnStartFrameChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_fullClipLength = m_clipLength + (m_isSimpleClip ? m_startFrame.GetValue() * 1000.0f : 0);

	Invalidate(TRUE);
	UpdateWindow();

	return 0;
}

LRESULT CTimelineWindow::OnBPMChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  g_graph->m_bpmRate = m_bpmRate.GetValue();
  g_graph->modified();

  HWND hWnd = g_winMap.Get(ID_SPLINE_WINDOW);
  ::InvalidateRect(hWnd, 0, TRUE);
  ::UpdateWindow(hWnd);

  hWnd = g_winMap.Get(ID_ANIMMIXER_WINDOW);
  ::InvalidateRect(hWnd, 0, TRUE);
  ::UpdateWindow(hWnd);

  return 0;
}

void CTimelineWindow::SetPlayState(sInt newState)
{
  m_playState = newState;

  g_viruz->stop();
  
  m_buttons[1].SetCheck(m_playState == 1);
  m_buttons[5].SetCheck(m_playState == 2);
  
  if (m_playState == 1)
  {
    if (m_sound.GetCheck())
			g_viruz->play(m_curFrame);
    
    LARGE_INTEGER time, freq;
    QueryPerformanceCounter(&time);
    QueryPerformanceFrequency(&freq);
    
    m_startTime = time.QuadPart;
    m_startTime -= m_curFrame * 1.0 / 1000.0 * freq.LowPart;
  }
  
	if (updateClipPtr())
		m_clip->applyBasePoses();

  m_lastPlayFrame = -1;
}

void CTimelineWindow::SetCurrentClip(sU32 newClipID)
{
  frAnimationClip* newClip = g_graph->m_clips->getClip(newClipID);
  
  if (newClip || !newClipID)
  {
		if (updateClipPtr())
		{
			m_clip->applyBasePoses();
			m_clip->setStart(m_startFrame.GetValue());

			frOpGraph::opMapIt it = g_graph->m_ops.find(g_graph->m_view3DOp);
			if (it != g_graph->m_ops.end())
				it->second.plg->process();

			::PostMessage(g_winMap.Get(ID_3DPREVIEW_WINDOW), WM_SET_DISPLAY_MESH, 0, g_graph->m_view3DOp);
		}

    fillClipComboBox();

    sF32 clipStart = 0;
    if (newClipID && newClip)
    {
      clipStart = newClip->getStart();

			if (newClip->isSimple())
      {
        sU32 ids[2] = { newClip->m_elements.front().id, newClip->m_elements.front().curveID };
    
        ::SendMessage(g_winMap.Get(ID_SPLINE_WINDOW), WM_SET_EDIT_SPLINE, 0, (LPARAM) ids);
        ::SendMessage(g_winMap.Get(ID_MAIN_VIEW), WM_SET_SUBMODE, 0, 0);
      }
      else // compound clip => anim mixer
        ::SendMessage(g_winMap.Get(ID_MAIN_VIEW), WM_SET_SUBMODE, 1, 0);

      m_clipName.SelectString(-1, newClip->getName());
    }
    else
    {
      ::SendMessage(g_winMap.Get(ID_SPLINE_WINDOW), WM_SET_EDIT_SPLINE, 0, 0);
      m_clipName.SelectString(-1, "(no clip)");
    }

		if (newClip)
			newClip->updateLength();

    m_startFrame.SetValue(clipStart);
    m_curClip = newClipID;
		m_clip = newClip;
		m_isSimpleClip = newClip ? newClip->isSimple() : sFALSE;
		m_clipLength = newClip ? newClip->getLength() : 0;
		m_fullClipLength = m_clipLength + (m_isSimpleClip ? m_startFrame.GetValue() * 1000.0f : 0);

    Invalidate();
    UpdateWindow();

    HWND hParamWindow = g_winMap.Get(ID_PARAM_WINDOW);
    ::InvalidateRect(hParamWindow, 0, TRUE);
    ::UpdateWindow(hParamWindow);

    HWND hAnimMixerWindow = g_winMap.Get(ID_ANIMMIXER_WINDOW);
    ::InvalidateRect(hAnimMixerWindow, 0, TRUE);
    ::UpdateWindow(hAnimMixerWindow);
  }
}

void CTimelineWindow::fillClipComboBox()
{
  m_clipName.ResetContent();
  m_clipName.AddString("(no clip)");

  for (frClipContainer::nameMapCIt it = g_graph->m_clips->m_names.begin(); it != g_graph->m_clips->m_names.end(); ++it)
    m_clipName.AddString(it->first);
}

sS32 CTimelineWindow::getTime()
{
  if (!m_sound.GetCheck())
  {
    LARGE_INTEGER perf, freq;
    QueryPerformanceCounter(&perf);
    QueryPerformanceFrequency(&freq);

    sS32 tSinceStart = sS64((perf.QuadPart - m_startTime) * 1000) / freq.LowPart;
    return tSinceStart;    
  }
  else
    return fr::maximum((sS32) 0, g_viruz->getSongTime());
}

void CTimelineWindow::processScratch(CPoint pnt)
{
  CRect rc;
  GetClientRect(&rc);
  rc.left += 11;
  rc.right = (rc.right * 7) / 10 - 13;

	if (updateClipPtr())
	{
		const sInt frameEnd = fr::maximum(1, m_fullClipLength);

		setCurrentFrame(fr::clamp(MulDiv(pnt.x - rc.left, frameEnd, rc.right - rc.left), 0, frameEnd), sTRUE);
	}
}

void CTimelineWindow::setCurrentFrame(sInt frameNum, sBool reseekSound, sBool noUpdate)
{
	if (!updateClipPtr())
		return;

  sInt lastRealFrame = m_lastPlayFrame; // may cause slight problems with switches, gotta test that.
  m_curFrame = frameNum;
  g_lastCompositingOp = 0;
  
  sInt realFrame = m_curFrame;
  if (m_isSimpleClip)
  {
    const sInt start = m_startFrame.GetValue() * 1000;
    realFrame -= start;
    lastRealFrame -= start;
  }

  if (realFrame >= lastRealFrame) // forward eval?
    m_clip->evaluateRange(lastRealFrame, realFrame);
  else if (realFrame < lastRealFrame) // back seek eval
  {
    m_clip->applyBasePoses();
    m_clip->evaluateRange(0, realFrame);
  }

  frOpGraph::opMapIt it;

	it = g_graph->m_ops.find(g_graph->m_viewOp);
	if (it != g_graph->m_ops.end())
		it->second.plg->process();

	::SendMessage(g_winMap.Get(ID_PREVIEW_WINDOW), WM_SET_DISPLAY_TEXTURE, noUpdate, g_graph->m_viewOp);

	it = g_graph->m_ops.find(g_graph->m_view3DOp);
  if (it != g_graph->m_ops.end())
    it->second.plg->process();

  ::SendMessage(g_winMap.Get(ID_3DPREVIEW_WINDOW), WM_SET_DISPLAY_MESH, noUpdate, g_graph->m_view3DOp);
	drawTimeBar();

  if (g_lastCompositingOp)
    ::SendMessage(g_winMap.Get(ID_GRAPH_WINDOW), WM_SET_DISPLAY_MESH, 0, g_lastCompositingOp);

	::SendMessage(g_winMap.Get(ID_SPLINE_WINDOW), WM_FRAME_CHANGED, 0, realFrame);
	::SendMessage(g_winMap.Get(ID_ANIMMIXER_WINDOW), WM_FRAME_CHANGED, 0, realFrame);

	if (!noUpdate)
	{
		m_lastPlayFrame = m_curFrame;

		if (reseekSound && m_playState != 0)
		{
			if (m_sound.GetCheck())
			{
				g_viruz->stop();
				g_viruz->play(m_curFrame);
			}
			else
			{
				LARGE_INTEGER freq;
				QueryPerformanceFrequency(&freq);

				m_startTime += sS64(getTime() - m_curFrame) * freq.LowPart / 1000;
			}
		}
	}
}

void CTimelineWindow::drawTimeBar(sInt mode)
{
	CDCHandle dc = GetDC();
  CPen timePen;

  timePen.CreatePen(PS_SOLID, 3, RGB(255, 0, 0) ^ RGB(174, 169, 167));
  CPenHandle oldPen = dc.SelectPen(timePen);
  int op = dc.SetROP2(R2_XORPEN);

  CRect rc;
  GetClientRect(&rc);
  rc.left += 11;
  rc.top += 5;
  rc.right = (rc.right * 7) / 10 - 13;
  rc.bottom = 29;
 
	if (mode != 2 && m_timeBarPos != -1)
	{
		dc.MoveTo(m_timeBarPos, rc.bottom - 1);
		dc.LineTo(m_timeBarPos, rc.top + 1);

		m_timeBarPos = -1;
	}

	if (mode != 1 && updateClipPtr())
	{
		m_timeBarPos = rc.left + MulDiv(rc.right - rc.left, fr::clamp(m_curFrame, 0, m_fullClipLength), m_fullClipLength);

		dc.MoveTo(m_timeBarPos, rc.bottom - 1);
		dc.LineTo(m_timeBarPos, rc.top + 1);
	}
	    
  dc.SelectPen(oldPen);
  dc.SetROP2(op);
	ReleaseDC(dc);
}

sBool CTimelineWindow::updateClipPtr()
{
	if (m_curClip)
		m_clip = g_graph->m_clips->getClip(m_curClip);
	else
		m_clip = 0;

	if (!m_clip)
		m_curClip = 0;

	return m_clip != 0;
}

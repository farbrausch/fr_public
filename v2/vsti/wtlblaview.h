// VIRUZPANEL.h : interface of the CVIRUZPANEL class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_VIRUZPANEL_H__213CE356_CC77_11D4_9322_00A0CC20D58C__INCLUDED_)
#define AFX_VIRUZPANEL_H__213CE356_CC77_11D4_9322_00A0CC20D58C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "multibutton.h"
#include "../sounddef.h"
#include "../types.h"
#include "../synth.h"
#include "../tool/file.h"
#include "midi.h"
#include <math.h>

#define  VPE_SETPOINTER  0xF0
#define  VPE_EDIT        0xF1
#define  VPE_GOTOPATCH   0xF2
#define	 VPE_PARAMCHG		 0xF3

#ifdef RONAN
	extern "C" extern void e2p_main();
	extern "C" extern void e2p_initio(char *a_in, char *a_out);
#endif

/*******************************************************************************\
*
*	Linkes Patchedit-Panel: Parameteransicht
*
\*******************************************************************************/

class CViruZPanel : public CScrollWindowImpl<CViruZPanel>
{
public:
	DECLARE_WND_CLASS_EX("fr_ViruZPanel", CS_HREDRAW|CS_VREDRAW, COLOR_BTNFACE)

	CStatic *topics;
	CStatic *labels;
	CWindow **parm;
	
	BOOL PreTranslateMessage(MSG* pMsg)
	{
		pMsg;
		return FALSE;
	}

	BEGIN_MSG_MAP(CViruZPanel)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_COMMAND, OnCommand)
		MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
		MESSAGE_HANDLER(TBM_SETPOS, OnSetPos)
		CHAIN_MSG_MAP(CScrollWindowImpl<CViruZPanel>)
	END_MSG_MAP()

	CViruZPanel() : CScrollWindowImpl<CViruZPanel>()
	{
		parm=new CWindow *[v2nparms];
		for (int i=0; i<v2nparms; i++)
			parm[i]=0;
		labels=new CStatic [v2nparms];
		topics=new CStatic [v2ntopics];
	}

	~CViruZPanel()
	{
		delete labels;
		delete topics;
		delete parm;
	}

	void DoPaint(CDCHandle /*dc*/)
	{
	}


	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		for (int i=0; i<v2nparms; i++)
			if (parm[i] && *parm[i]) parm[i]->DestroyWindow();
		for (int i=0; i<v2nparms; i++)
			if (labels[i]) labels[i].DestroyWindow();
		for (int i=0; i<v2ntopics; i++)
			if (topics[i]) topics[i].DestroyWindow();

		bHandled=0;
		return 0;
	}

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		HFONT fnt=(HFONT)GetStockObject(DEFAULT_GUI_FONT);

		int ypos=2;
		int xpos=0;
		int tn=0;
		for (int i=0; i<v2nparms; i++)
		{

			if (tn<v2ntopics && v2topics2[tn]==i)
			{
				if (i) ypos+=20;
				if (xpos) ypos+=30;
				topics[tn].Create(m_hWnd,CRect(7,ypos+5,420,ypos+25), v2topics[tn].name, 
																			 WS_CHILD|WS_VISIBLE, WS_EX_STATICEDGE);
				ypos+=31;
				xpos=0;
				tn++;
			}

			V2PARAM p=v2parms[i];
			if (p.ctltype!=VCTL_SKIP)
			{
				labels[i].Create(m_hWnd,CRect(xpos+7,ypos+5,xpos+55,ypos+20), p.name, WS_CHILD|WS_VISIBLE);
				labels[i].SetFont(fnt);
			}

			CMultiButton *mb;
			CTrackBarCtrl *tb;
			switch (p.ctltype)
			{
			case VCTL_MB:
				mb=new CMultiButton();
				mb->Create(m_hWnd, CRect(xpos+60,ypos+2,xpos+285,ypos+22), p.ctlstr);
				parm[i]=mb;
				if (!xpos && p.ctlstr[0]=='!') {xpos=210; }
				else { xpos=0; ypos+=30; }

				break;
			case VCTL_SLIDER:
				tb=new CTrackBarCtrl();
				tb->Create(m_hWnd, CRect(xpos+50,ypos,xpos+200,ypos+25), "",WS_CHILD|WS_VISIBLE|TBS_HORZ|TBS_BOTTOM|TBS_AUTOTICKS|TBS_TOOLTIPS);
				tb->SetTicFreq(((p.max-p.min)+1)>>3);
				tb->SetRange(p.min-p.offset,p.max-p.offset,TRUE);
				parm[i]=tb;
				if (!xpos) {xpos=210; }
				else { xpos=0; ypos+=30; }
				break;
			case VCTL_SKIP:
			  parm[i]=new CWindow();
				parm[i]->m_hWnd=0;
				break;
			}
		}

		SetScrollSize(430,ypos+40,TRUE);
		return 0;
	}

	LRESULT OnCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
	  bHandled=0;
		for (int i=0; i<v2nparms; i++) if ((LPARAM)parm[i]->m_hWnd==lParam)
		{
			V2::g_theSynth.GetClient()->CurrentParameterChanged(i);
			int pos=wParam+v2parms[i].offset;
			::SendMessage(GetParent(),WM_COMMAND,(i<<8)|pos,(LPARAM)m_hWnd);
			bHandled=1;
		}
		return 0;
	}

	LRESULT OnHScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		bHandled=0;
		for (int i=0; i<v2nparms; i++) if ((LPARAM)parm[i]->m_hWnd==lParam)
		{
			V2::g_theSynth.GetClient()->CurrentParameterChanged(i);
			int pos=((CTrackBarCtrl *)(parm[i]))->GetPos()+v2parms[i].offset;
			::SendMessage(GetParent(),WM_COMMAND,(i<<8)|pos,(LPARAM)m_hWnd);
			bHandled=1;
		}
		return 0;
	}

	LRESULT OnSetPos(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		int n=(int)wParam;
		int v=(int)lParam;
		if (n<0 || n>=v2nparms || v<0 || v>127)
			return 0;
		::SendMessage(parm[n]->m_hWnd, TBM_SETPOS, 1, v-v2parms[n].offset);
		return 0;
	}

	LRESULT OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		HDC dc((HDC)wParam);
		for (int i=0; i<v2ntopics; i++) if (lParam==(LPARAM)(topics[i].m_hWnd))
		{
			SetTextColor(dc,0xfff0f0f0);
			SetBkMode(dc,TRANSPARENT);
			return (LRESULT)(GetSysColorBrush(COLOR_3DSHADOW));
		}
		SetTextColor(dc,0xff000000);
		SetBkMode(dc,TRANSPARENT);
		return (LRESULT)(GetSysColorBrush(COLOR_BTNFACE));
	}

};

/*******************************************************************************\
*
*	Rechtes Patchedit-Panel: Modulationsansicht
*
\*******************************************************************************/

class CViruZModPanel : public CScrollWindowImpl<CViruZModPanel>
{
public:
	DECLARE_WND_CLASS_EX("fr_ViruZModPanel", CS_HREDRAW|CS_VREDRAW, COLOR_BTNFACE)

	char *ptr;
	HFONT fnt;
	CButton addbutton;
	CStatic statline;

	CButton				sources[255];
	CTrackBarCtrl values[255];
	CButton       dests[255];
	CButton       erasers[255];
	CStatic       arrows[255];

	CMenu         srcmenu, destmenu;
	int           mselno;
		

	char v2dests[256][256];

	int  v2ndests;

	BEGIN_MSG_MAP(CViruZModPanel)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_COMMAND, OnCommand)
		MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
		MESSAGE_HANDLER(VPE_SETPOINTER, OnSetPointer)
		CHAIN_MSG_MAP(CScrollWindowImpl<CViruZModPanel>)
	END_MSG_MAP()

	CViruZModPanel() : CScrollWindowImpl<CViruZModPanel>()
	{
		CString s;
		destmenu.CreatePopupMenu();
		v2ndests=0;
		int lasttn=0;
		int tn=0;
		for (int i=0; i<v2nparms; i++)
		{
			if (tn<(v2ntopics-1) && v2topics2[tn+1]==i)
				tn++;
			s=v2topics[tn].name;
			s+=" ";
			s+=v2parms[i].name;
			v2dests[i][255]=0;
			strcpy(v2dests[i],s);
			if (v2parms[i].isdest&1)
			{
				if (lasttn!=tn)
					destmenu.AppendMenu(MF_SEPARATOR);
				destmenu.AppendMenu(MF_STRING,i+256,s);
				lasttn=tn;
			}
		}

		srcmenu.CreatePopupMenu();
		for (int i=0; i<v2nsources; i++)
		{
			if (i==1 || i==8)
				srcmenu.AppendMenu(MF_SEPARATOR);
			srcmenu.AppendMenu(MF_STRING,i+512,v2sources[i]);
		}

	}

	~CViruZModPanel()
	{
	}

	void DoPaint(CDCHandle /*dc*/)
	{
	}


	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		for (int i=0; i<255; i++)
		{
			sources[i].DestroyWindow();
			arrows[i].DestroyWindow();
			dests[i].DestroyWindow();
	    values[i].DestroyWindow();
			erasers[i].DestroyWindow();
		}
		addbutton.DestroyWindow();
		statline.DestroyWindow();
		bHandled=0;
		return 0;
	}

	
	void UpdateDisplay(int nr=-1)
	{
		int n=(int)ptr[0];

		int i;
		for (i=((nr==-1)?0:nr); i<((nr==-1)?n:nr+1); i++)
		{
			char *p2=ptr+1+3*i;
			sources[i].ShowWindow(1);
			arrows[i].ShowWindow(1);
			dests[i].ShowWindow(1);
			values[i].ShowWindow(1);
			erasers[i].ShowWindow(1);

			sources[i].SetWindowText(v2sources[p2[0]]);
			dests[i].SetWindowText(v2dests[p2[2]]);
			values[i].SetPos(p2[1]-64);
		}
		for (;i<((nr==-1)?255:nr+1); i++)
		{
			sources[i].ShowWindow(0);
			arrows[i].ShowWindow(0);
			dests[i].ShowWindow(0);
			values[i].ShowWindow(0);
			erasers[i].ShowWindow(0);
		}

		CString sl;
		sl.Format("%d of 255 modulations used",n);
		statline.SetWindowText(sl);

		SetScrollSize(230, 70*n+90, TRUE);
	}


	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		fnt=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
	
		addbutton.Create(m_hWnd,CRect(5,5,65,30),"Add",WS_VISIBLE|WS_CHILD);
		addbutton.SetFont(fnt);

		statline.Create(m_hWnd,CRect(80,10,220,30),"0 of 255 modulations used",WS_VISIBLE|WS_CHILD|SS_RIGHT);
		statline.SetFont(fnt);

		int ypos=50;
		for (int i=0; i<255; i++)
		{
			sources[i].Create(m_hWnd,CRect(5,ypos,80,ypos+22),v2sources[0],WS_CHILD,WS_EX_STATICEDGE);
			sources[i].SetFont(fnt);
			arrows[i].Create(m_hWnd,CRect(90,ypos+3,105,ypos+24),"-->",WS_CHILD);
			arrows[i].SetFont(fnt);
			dests[i].Create(m_hWnd,CRect(110,ypos,220,ypos+22),v2dests[0],WS_CHILD,WS_EX_STATICEDGE);
			dests[i].SetFont(fnt);
      values[i].Create(m_hWnd, CRect(5,ypos+25,155,ypos+50), "",WS_CHILD|TBS_HORZ|TBS_BOTTOM|TBS_AUTOTICKS|TBS_TOOLTIPS);
			values[i].SetTicFreq(8);
			values[i].SetRange(-64,63);
			erasers[i].Create(m_hWnd,CRect(160,ypos+27,220,ypos+45),"Delete",WS_CHILD);
			erasers[i].SetFont(fnt);
			ypos+=70;
		}

		SetScrollSize(230,ypos+40,TRUE);
		SetScrollPos(0,0);
		return 0;
	}


	LRESULT OnCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		bHandled=0;
		HWND src=(HWND)lParam;
		int i;

		// Menucommand
		if (!lParam)
		{
			if (wParam>=0x100 && wParam<0x200) // dest select
			{
				ptr[3*mselno+1+2]=wParam-0x100;
				::SendMessage(GetParent(),VPE_EDIT,0,0);
				UpdateDisplay(mselno);
			}
			if (wParam>=0x200 && wParam<0x300) // dest select
			{
				ptr[3*mselno+1]=wParam-0x200;
				::SendMessage(GetParent(),VPE_EDIT,0,0);
				UpdateDisplay(mselno);
			}
			bHandled=1;
			return 0;
		}

		// Add-Button
		if (wParam==BN_CLICKED && src==addbutton)
		{
			int n=ptr[0];
			if (n<255)
			{
				ptr[3*n+1]=0;
				ptr[3*n+2]=64;
				ptr[3*n+3]=0;
				ptr[0]++;
				::SendMessage(GetParent(),VPE_EDIT,0,0);
				UpdateDisplay(n);
			}
			bHandled=1;
			return 0;
		}
		
		// Voice-Buttons
		CPoint p;
		GetCursorPos(&p);
		for (i=0; i<255; i++)
		{
			if (wParam==BN_CLICKED && src==sources[i])
			{
				srcmenu.TrackPopupMenu(TPM_CENTERALIGN|TPM_LEFTBUTTON,p.x,p.y-100,m_hWnd);
				mselno=i;
				bHandled=1;
				return 0;
			}
			if (wParam==BN_CLICKED && src==dests[i])
			{
				destmenu.TrackPopupMenu(TPM_CENTERALIGN|TPM_LEFTBUTTON,p.x,p.y,m_hWnd);
				mselno=i;
				bHandled=1;
				return 0;
			}
			if (wParam==BN_CLICKED && src==erasers[i])
			{
				if (MessageBox("Really delete this modulation?","Farbrausch ViruZ II",MB_ICONQUESTION|MB_YESNO)==IDYES)
				{
					for (int n=i; n<(ptr[0]-1); n++)
					{
						register char *p2=ptr+3*n+1;
						p2[0]=p2[3];
						p2[1]=p2[4];
						p2[2]=p2[5];
					}
					ptr[0]--;
					UpdateDisplay();
					::SendMessage(GetParent(),VPE_EDIT,0,0);
				}
			}

		}
		return 0;
	}

	LRESULT OnHScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		bHandled=0;
		for (int i=0; i<255; i++) if ((HWND)lParam==values[i])
		{
			ptr[1+3*i+1]=values[i].GetPos()+64;
			::SendMessage(GetParent(),VPE_EDIT,0,0);
			bHandled=1;
		}
		return 0;
	}

	LRESULT OnSetPointer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{
		ptr=(char *)lParam;
		UpdateDisplay();
		return 0;
	}

};


/*******************************************************************************\
*
*	Patchedit-Panel
*
\*******************************************************************************/

class CViruZPatchEdit : public CSplitterWindowImpl<CViruZPatchEdit>
{
public:
	DECLARE_WND_CLASS_EX(_T("fr_ViruZPatchEdit"), CS_DBLCLKS, COLOR_WINDOW)

	CViruZPanel leftpane;
	CViruZModPanel rightpane;

	char *ptr;

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		pMsg;
		return FALSE;
	}

	BEGIN_MSG_MAP(CViruZPatchEdit)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(VPE_SETPOINTER, OnSetPointer)
		MESSAGE_HANDLER(VPE_PARAMCHG, OnParamChg)
		MESSAGE_HANDLER(WM_COMMAND, OnCommand)
		MESSAGE_HANDLER(VPE_EDIT, OnEdit)
		CHAIN_MSG_MAP(CSplitterWindowImpl<CViruZPatchEdit>)
	END_MSG_MAP()



	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		SetSplitterPanes(0,0);
		leftpane.DestroyWindow();
		rightpane.DestroyWindow();
		bHandled=0;
		return 0;
	}

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{
		CSplitterWindowImpl<CViruZPatchEdit>::OnCreate (uMsg, wParam, lParam, bHandled);
		
		ptr=0;

		leftpane.Create(m_hWnd, CRect(0,0,430,400), "", WS_VISIBLE|WS_CHILD);
		rightpane.Create(m_hWnd, CRect(0,0,430,400), "", WS_VISIBLE|WS_CHILD);

		SetSplitterPanes(leftpane,rightpane);
		SetSplitterExtendedStyle(SPLIT_NONINTERACTIVE);
		SetSplitterRect(CRect(0,0,900,0));
		SetSplitterPos(-1);

		return 0;
	}

	LRESULT OnSetPointer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{
		ptr=(char *)lParam;
		if (ptr)
			for (int i=0; i<v2nparms; i++)
				::SendMessage(leftpane.m_hWnd, TBM_SETPOS, i, ptr[i]);
		::SendMessage(rightpane.m_hWnd, VPE_SETPOINTER, wParam, (LPARAM)(ptr+v2nparms));
			
		return 0;
	}

	LRESULT OnParamChg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{		
		::SendMessage(leftpane.m_hWnd, TBM_SETPOS, lParam, ptr[lParam]);
		return 0;
	}

	LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{
		bHandled=0;
		if (lParam==(LPARAM)(leftpane.m_hWnd))
		{
			if (ptr)
				ptr[(wParam>>8)&0xff]=wParam&0xff;
			::SendMessage(GetParent(),VPE_EDIT,0,0);
			bHandled=1;
		}
		else if (lParam==(LPARAM)(rightpane.m_hWnd))
		{
		}
		return 0;
	}

	LRESULT OnEdit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{
		::SendMessage(GetParent(), VPE_EDIT, wParam, lParam);
		return 0;
	}
};


/*******************************************************************************\
*
*	Globals-Panel
*
\*******************************************************************************/


class CViruZGlobalsPanel : public CScrollWindowImpl<CViruZGlobalsPanel>
{
public:
	DECLARE_WND_CLASS_EX("fr_ViruZGlobalsPanel", CS_HREDRAW|CS_VREDRAW, COLOR_BTNFACE)

	CStatic *topics;
	CStatic *labels;
	CWindow **parm;

	char *ptr;
	
	BOOL PreTranslateMessage(MSG* pMsg)
	{
		pMsg;
		return FALSE;
	}

	BEGIN_MSG_MAP(CViruZGlobalsPanel)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_COMMAND, OnCommand)
		MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
		MESSAGE_HANDLER(VPE_PARAMCHG, OnParamChg)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
		MESSAGE_HANDLER(VPE_SETPOINTER, OnSetPointer)
		CHAIN_MSG_MAP(CScrollWindowImpl<CViruZGlobalsPanel>)
	END_MSG_MAP()

	CViruZGlobalsPanel() : CScrollWindowImpl<CViruZGlobalsPanel>()
	{
		parm=new CWindow *[v2ngparms];
		for (int i=0; i<v2ngparms; i++)
			parm[i]=0;
		labels=new CStatic [v2ngparms];
		topics=new CStatic [v2ngtopics];
	}

	~CViruZGlobalsPanel()
	{
		delete labels;
		delete topics;
		delete parm;
	}

	void DoPaint(CDCHandle /*dc*/)
	{
	}


	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		for (int i=0; i<v2ngparms; i++)
			if (parm[i]) parm[i]->DestroyWindow();
		for (int i=0; i<v2ngparms; i++)
			labels[i].DestroyWindow();
		for (int i=0; i<v2ngtopics; i++)
			topics[i].DestroyWindow();

		bHandled=0;
		return 0;
	}

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		HFONT fnt=(HFONT)GetStockObject(DEFAULT_GUI_FONT);

		int ypos=2;
		int xpos=0;
		int tn=0;
		for (int i=0; i<v2ngparms; i++)
		{

			if (tn<v2ngtopics && v2gtopics2[tn]==i)
			{
				if (i) ypos+=20;
				if (xpos) ypos+=30;
				topics[tn].Create(m_hWnd,CRect(7,ypos+5,420,ypos+25), v2gtopics[tn].name, 
																			 WS_CHILD|WS_VISIBLE, WS_EX_STATICEDGE);
				ypos+=31;
				xpos=0;
				tn++;
			}

			V2PARAM p=v2gparms[i];
			labels[i].Create(m_hWnd,CRect(xpos+7,ypos+5,xpos+55,ypos+20), p.name, WS_CHILD|WS_VISIBLE);
			labels[i].SetFont(fnt);

			CMultiButton *mb;
			CTrackBarCtrl *tb;
			switch (p.ctltype)
			{
			case VCTL_MB:
				mb=new CMultiButton();
				mb->Create(m_hWnd, CRect(xpos+60,ypos+2,xpos+285,ypos+22), p.ctlstr);
				parm[i]=mb;
				if (!xpos && p.max<3) {xpos=210; }
				else { xpos=0; ypos+=30; }

				break;
			case VCTL_SLIDER:
				tb=new CTrackBarCtrl();
				tb->Create(m_hWnd, CRect(xpos+50,ypos,xpos+200,ypos+25), "",WS_CHILD|WS_VISIBLE|TBS_HORZ|TBS_BOTTOM|TBS_AUTOTICKS|TBS_TOOLTIPS);
				tb->SetTicFreq((p.max+1)>>3);
				tb->SetRange(-p.offset,p.max-p.offset,TRUE);
				parm[i]=tb;
				if (!xpos) {xpos=210; }
				else { xpos=0; ypos+=30; }
				break;
			}
		}

		SetScrollSize(430,ypos+40,TRUE);
		return 0;
	}

	LRESULT OnCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
	  bHandled=0;
		for (int i=0; i<v2ngparms; i++) if ((LPARAM)parm[i]->m_hWnd==lParam)
		{
			V2::g_theSynth.GetClient()->CurrentParameterChanged(i+v2nparms);
			int pos=wParam+v2gparms[i].offset;
			ptr[i]=pos;
			synthSetGlobals(ptr);
			bHandled=1;
		}
		return 0;
	}

	LRESULT OnHScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		bHandled=0;
		for (int i=0; i<v2ngparms; i++) if ((LPARAM)parm[i]->m_hWnd==lParam)
		{
			V2::g_theSynth.GetClient()->CurrentParameterChanged(i+v2nparms);
			int pos=((CTrackBarCtrl *)(parm[i]))->GetPos()+v2gparms[i].offset;
			ptr[i]=pos;
			synthSetGlobals(ptr);
			bHandled=1;
		}
		return 0;
	}

	LRESULT OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		HDC dc((HDC)wParam);
		for (int i=0; i<v2ngtopics; i++) if (lParam==(LPARAM)(topics[i].m_hWnd))
		{
			SetTextColor(dc,0xfff0f0f0);
			SetBkMode(dc,TRANSPARENT);
			return (LRESULT)(GetSysColorBrush(COLOR_3DSHADOW));
		}
		SetTextColor(dc,0xff000000);
		SetBkMode(dc,TRANSPARENT);
		return (LRESULT)(GetSysColorBrush(COLOR_BTNFACE));
	}

	LRESULT OnParamChg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{		
		parm[lParam]->SendMessage(TBM_SETPOS, lParam, ptr[lParam]-v2gparms[lParam].offset);
		synthSetGlobals(ptr);
		return 0;
	}

	LRESULT OnSetPointer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{
		ptr=(char *)lParam;
		synthSetGlobals(ptr);
		if (ptr)
			for (int i=0; i<v2ngparms; i++) {
				//v2vstiParamChanged(v2vstiAEffect,i+v2nparms); // VSTi automation
				parm[i]->SendMessage(TBM_SETPOS, i, ptr[i]-v2gparms[i].offset);
			}
		return 0;
	}

};

/*******************************************************************************\
*
*	Sprachsynth-Panel
*
\*******************************************************************************/

#ifdef RONAN

class CViruZLisaPanel : public CScrollWindowImpl<CViruZLisaPanel>
{
public:
	DECLARE_WND_CLASS_EX("fr_ViruZLisaPanel", CS_HREDRAW|CS_VREDRAW, COLOR_BTNFACE)

	CStatic xl1, xl2;
	CEdit engl_in;
	CEdit phon_out;

	CStatic labels[64];
	CEdit   texts[64];

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		pMsg;
		return FALSE;
	}

	BEGIN_MSG_MAP(CViruZGlobalsPanel)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_COMMAND, OnCommand)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
		MESSAGE_HANDLER(VPE_SETPOINTER, OnSetPointer)
		CHAIN_MSG_MAP(CScrollWindowImpl<CViruZLisaPanel>)
	END_MSG_MAP()

	CViruZLisaPanel() : CScrollWindowImpl<CViruZLisaPanel>()
	{
	}

	~CViruZLisaPanel()
	{
	}

	void DoPaint(CDCHandle /*dc*/)
	{
	}


	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		xl1.DestroyWindow();
		engl_in.DestroyWindow();
		xl2.DestroyWindow();
		phon_out.DestroyWindow();
		for (sInt i=0; i<64; i++)
		{
			labels[i].DestroyWindow();
			texts[i].DestroyWindow();
		}
		bHandled=0;
		return 0;
	}

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		HFONT fnt=(HFONT)GetStockObject(DEFAULT_GUI_FONT);

		sInt ypos=0;
		sInt xpos=0;

		xl1.Create(m_hWnd,CRect(xpos+9,ypos+8,xpos+55,ypos+27),"Translate:",WS_CHILD|WS_VISIBLE);
		xl1.SetFont(fnt);

	  engl_in.Create(m_hWnd,CRect(xpos+60,ypos+5,xpos+265,ypos+27),"english",WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|SS_CENTERIMAGE|WS_TABSTOP,WS_EX_CLIENTEDGE|WS_EX_CONTROLPARENT);
		engl_in.SetFont(fnt);
		engl_in.SetLimitText(255);

		xl2.Create(m_hWnd,CRect(xpos+270,ypos+8,xpos+288,ypos+27),"=>",WS_CHILD|WS_VISIBLE);
		xl2.SetFont(fnt);

	  phon_out.Create(m_hWnd,CRect(xpos+289,ypos+5,xpos+655,ypos+27),"phonemes",WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_READONLY|SS_CENTERIMAGE,WS_EX_STATICEDGE);
		phon_out.SetFont(fnt);
		phon_out.SetLimitText(255);

		ypos+=40;

		for (sInt i=0; i<64; i++)
		{
			CString s;
			s.Format("Text #%02d",i);
			labels[i].Create(m_hWnd,CRect(xpos+7,ypos+8,xpos+107,ypos+25),s,WS_CHILD|WS_VISIBLE);
			labels[i].SetFont(fnt);

			texts[i].Create(m_hWnd,CRect(xpos+60,ypos+5,xpos+655,ypos+25),"",WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|WS_TABSTOP,WS_EX_CLIENTEDGE|WS_EX_CONTROLPARENT);
			texts[i].SetFont(fnt);
			texts[i].LimitText(255);

			ypos+=25;
		}

		SetScrollSize(650,ypos+40,TRUE);

		SendMessage(VPE_SETPOINTER,0,0);

		return 0;
	}

	LRESULT OnCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
	  bHandled=0;
		switch (HIWORD(wParam))
		{
		case EN_CHANGE:
			if (lParam==(LPARAM)(engl_in.m_hWnd))
			{
				char in[256];
				char out[256];
				engl_in.GetWindowText(in,255);
				e2p_initio(in,out);
				e2p_main();
				phon_out.SetWindowText(out);
			}
			for (sInt i=0; i<64; i++) if (lParam==(LPARAM)(texts[i].m_hWnd))
			{
				char txt[256];
				ZeroMemory(txt,256);
				texts[i].GetWindowText(txt,255);
				memcpy(speech[i],txt,256);
			}
			break;
		}
		return 0;
	}

	LRESULT OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		HDC dc((HDC)wParam);
		SetTextColor(dc,0xff000000);
		SetBkMode(dc,TRANSPARENT);
		return (LRESULT)(GetSysColorBrush(COLOR_BTNFACE));
	}

	LRESULT OnSetPointer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{
		for (sInt i=0; i<64; i++)
			texts[i].SetWindowText(speech[i]);
		/*
		ptr=(char *)lParam;
		synthSetGlobals(ptr);
		if (ptr)
			for (int i=0; i<v2ngparms; i++)
				parm[i]->SendMessage(TBM_SETPOS, i, ptr[i]-v2gparms[i].offset);
				*/
		return 0;
	}

};

#endif


/*******************************************************************************\
*
*	Status
*
\*******************************************************************************/

class CViruZStatusPanel : public CWindowImpl<CViruZStatusPanel>
{
public:
	DECLARE_WND_CLASS_EX("fr_ViruZStatusPanel", CS_HREDRAW|CS_VREDRAW, COLOR_BTNFACE)

	CStatic frlogo, thlogo;
	CButton v2logo;
	HFONT sysfnt, v2fnt;
	HBITMAP v2bmp;

	CString filename;

	//CComboBox			patchsel, devsel;
	//CButton       editbutton, comparebutton, storebutton/*, panicbutton*/;
//	CButton				importbutton, exportbutton;

    CComboBox     patchsel;
	CButton       filemenubutton;
	CButton       editmenubutton;
	CButton       recordbutton;
	CStatic       patchno, chtitle, chs[16], pntitle, pnos[16], potitle, poly[16], potitle2, pototal, cpumeter;

	CProgressBarCtrl  vleft, vright;
	CMenu filemenu,editmenu;

	int clipl, clipr;
	int gpo[17], gpg[16];
	int editmode;
	int comparemode;
	int oldcpu;
    bool m_enablePatchControls;

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		pMsg;
		return FALSE;
	}

    void EnablePatchControls()
    {
        m_enablePatchControls = true;
    }

	BEGIN_MSG_MAP(CViruZPanel)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_COMMAND, OnCommand)
		MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(VPE_GOTOPATCH, OnGotoPatch)
		MESSAGE_HANDLER(VPE_EDIT, OnEdit)
	END_MSG_MAP()

    bool UsePatchControls()
    {
        return m_enablePatchControls;
    }

	CViruZStatusPanel() : CWindowImpl<CViruZStatusPanel>()
	{
        m_enablePatchControls = false;
		filemenu.CreatePopupMenu();
	  filemenu.AppendMenu(MF_STRING,0x100,"Load Patch/Bank");
	  filemenu.AppendMenu(MF_STRING,0x101,"Import V2M Patches");
		filemenu.AppendMenu(MF_SEPARATOR);
	  filemenu.AppendMenu(MF_STRING,0x102,"Save Bank");
	  filemenu.AppendMenu(MF_STRING,0x103,"Save Patch");

		editmenu.CreatePopupMenu();
		editmenu.AppendMenu(MF_STRING,0x200,"Copy Patch");
		editmenu.AppendMenu(MF_STRING,0x201,"Paste Patch");
		editmenu.AppendMenu(MF_SEPARATOR);
		editmenu.AppendMenu(MF_STRING,0x202,"Initialize Patch");
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		v2logo.DestroyWindow();
		cpumeter.DestroyWindow();
		filemenubutton.DestroyWindow();
		editmenubutton.DestroyWindow();
		recordbutton.DestroyWindow();
		patchno.DestroyWindow();
		vleft.DestroyWindow();
		vright.DestroyWindow();
		chtitle.DestroyWindow();
		pntitle.DestroyWindow();
		potitle.DestroyWindow();
		for (int i=0; i<16; i++)
		{
			chs[i].DestroyWindow();
			pnos[i].DestroyWindow();
			poly[i].DestroyWindow();
		}
		potitle2.DestroyWindow();
		pototal.DestroyWindow();
        if (UsePatchControls())
        {
            patchsel.DestroyWindow();
        }
		DeleteObject(v2fnt);
		DeleteObject(v2bmp);
		return 0;
	}

    void UpdatePatchNames()
    {
        if (UsePatchControls())
        {
		    patchsel.ResetContent();
		    patchsel.LimitText(32);
            CString s;
		    for (int i=0; i<128; i++)
            {
        	    s.Format("%s",patchnames[i]);
			    patchsel.AddString(s);
            }			
            patchsel.SetCurSel(v2curpatch);
        }
    }

	void UpdateDisplay(int updcb=1)
	{
		if (!editmode) comparemode=0;
		CString s;
        if (UsePatchControls())
        {
		    if (updcb)
		    {
                patchsel.SetCurSel(v2curpatch);
		    }

		    s.Format(" %02X",v2curpatch);
		    patchno.SetWindowText(s);
        }
        else
        {
		    s.Format(" %03d: %s",v2curpatch,patchnames[v2curpatch]);
		    patchno.SetWindowText(s);
        }
	}

	void SetEditBuffer(int next)
	{
        v2curpatch = next;
        ::SendMessage(GetParent(),VPE_SETPOINTER,(WPARAM)globals,(LPARAM)&soundmem[128*4+v2soundsize*v2curpatch]);
		UpdateDisplay(0);
	}


	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		sysfnt=(HFONT)GetStockObject(DEFAULT_GUI_FONT);

		v2fnt=CreateFont(48,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,OUT_TT_PRECIS,
			CLIP_DEFAULT_PRECIS,PROOF_QUALITY,VARIABLE_PITCH|FF_DONTCARE,"Impact");

		v2bmp=(HBITMAP)LoadImage(hInstance,MAKEINTRESOURCE(IDB_BITMAP1),IMAGE_BITMAP,0,0,LR_LOADMAP3DCOLORS);

		v2logo.Create(m_hWnd, CRect(45,5,170,55), NULL, WS_VISIBLE|WS_CHILD|BS_BITMAP|BS_FLAT);
		v2logo.SetBitmap(v2bmp);
		//v2logo.SetFont(v2fnt);
		
		// old cpumeter was @ CRect(45,82,170,96)
		cpumeter.Create(m_hWnd, CRect(635,25,695,40), "???%", WS_VISIBLE|WS_CHILD);
		cpumeter.SetFont(sysfnt);
		oldcpu=-1;


		sInt x=5;

		filemenubutton.Create(m_hWnd, CRect(x,75,x+60,98), "File...", WS_VISIBLE|WS_CHILD, WS_EX_STATICEDGE);
		filemenubutton.SetFont(sysfnt);

		x+=60+20;

		editmenubutton.Create(m_hWnd, CRect(x,75,x+60,98), "Edit...", WS_VISIBLE|WS_CHILD, WS_EX_STATICEDGE);
		editmenubutton.SetFont(sysfnt);

		x+=60+20;

		recordbutton.Create(m_hWnd, CRect(x,75,x+60,98), "Record!", WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX|BS_PUSHLIKE|BS_FLAT);
		recordbutton.SetFont(sysfnt);
		recordbutton.SetCheck(msIsRecording());


		x+=60+30;

        if (UsePatchControls())
        {
		    patchno.Create(m_hWnd,CRect(x,75,x+20,96), " xx ", WS_VISIBLE|WS_CHILD|SS_CENTERIMAGE, WS_EX_STATICEDGE);
		    patchno.SetFont(sysfnt);
    		x+=20+5;
		    patchsel.Create(m_hWnd, CRect(x,75,x+150,96), "????????????????", WS_VISIBLE|WS_CHILD|CBS_DROPDOWN|WS_VSCROLL);
		    patchsel.SetFont(sysfnt);

            UpdatePatchNames();
        }
        else
        {
		    patchno.Create(m_hWnd,CRect(x,75,x+150,96), " xxx: ????????????????", WS_VISIBLE|WS_CHILD|SS_CENTERIMAGE, WS_EX_STATICEDGE);
		    patchno.SetFont(sysfnt);
        }
  		x+=150+20;

		// old vumeters were @ CRect(5, 5, 18, 95),CRect(20, 5, 33, 95)
		vleft.Create(m_hWnd, CRect(5, 5, 18, 60), "left", WS_VISIBLE|WS_CHILD|PBS_SMOOTH|PBS_VERTICAL, WS_EX_STATICEDGE);
		vleft.SetRange(0,90);
		vleft.SetPos(0);

		vright.Create(m_hWnd, CRect(20, 5, 33, 60), "right", WS_VISIBLE|WS_CHILD|PBS_SMOOTH|PBS_VERTICAL, WS_EX_STATICEDGE);
		vright.SetRange(0,90);
		vright.SetPos(0);

		chtitle.Create(m_hWnd,CRect(180,5,230,20), "Channel #", WS_VISIBLE|WS_CHILD|SS_CENTERIMAGE);
		chtitle.SetFont(sysfnt);
		pntitle.Create(m_hWnd,CRect(180,25,230,40), "Patch #", WS_VISIBLE|WS_CHILD|SS_CENTERIMAGE);
		pntitle.SetFont(sysfnt);
		potitle.Create(m_hWnd,CRect(180,45,230,60), "Polyphony", WS_VISIBLE|WS_CHILD|SS_CENTERIMAGE);
		potitle.SetFont(sysfnt);
		for (int i=0; i<16; i++)
		{
			CString s;
			s.Format("%02d",i+1);
			chs[i].Create(m_hWnd,CRect(235+25*i,5,257+25*i,20), s, WS_VISIBLE|WS_CHILD|SS_CENTERIMAGE|SS_CENTER);
			chs[i].SetFont(sysfnt);
			pnos[i].Create(m_hWnd,CRect(235+25*i,25,257+25*i,40), "---", WS_VISIBLE|WS_CHILD|SS_CENTERIMAGE|SS_CENTER, WS_EX_STATICEDGE);
			pnos[i].SetFont(sysfnt);
			poly[i].Create(m_hWnd,CRect(235+25*i,45,257+25*i,60), "?", WS_VISIBLE|WS_CHILD|SS_CENTERIMAGE|SS_CENTER, WS_EX_STATICEDGE);
			poly[i].SetFont(sysfnt);
		}
		potitle2.Create(m_hWnd,CRect(635,45,645,60), "->", WS_VISIBLE|WS_CHILD|SS_CENTERIMAGE);
		potitle2.SetFont(sysfnt);
		pototal.Create(m_hWnd,CRect(650,45,672,60), "0", WS_VISIBLE|WS_CHILD|SS_CENTERIMAGE|SS_CENTER, WS_EX_STATICEDGE);
		pototal.SetFont(sysfnt);

		clipl=clipr=0;
		for (int i=0; i<16; i++)
			gpo[i]=gpg[i]=-1;
		gpo[16]=-1;

		::SetTimer(m_hWnd,303,20,0);

		editmode=comparemode=0;
		UpdateDisplay();
		UpdateWindow();
		return 0;
	}

	void StorePatch()
	{
		editmode=0;
	}

	LRESULT OnCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		CString s;
		s.Format("got cmd %d from %d\n",wParam,lParam);
		//tputDebugString(s);

		CPoint p;
		GetCursorPos(&p);

		HWND src=(HWND)lParam;

        if (UsePatchControls() && (src==patchsel))
		{
			switch (wParam>>16)
			{

			    case CBN_SELENDOK:  // Patchwexel
                    {
				        int next=patchsel.GetCurSel();
                        ::SendMessage(GetParent(),VPE_GOTOPATCH,next,0);
                        return 0;
                    }
			    case CBN_EDITCHANGE: // Patchumbenennung
                    {
                        char buf[256];
				        buf[255]=0;
				        ::SendMessage(patchsel,WM_GETTEXT,255,(LPARAM)buf);
                        strcpy(patchnames[v2curpatch],buf);
				        patchsel.InsertString(v2curpatch,buf);
				        patchsel.DeleteString(v2curpatch+1);                        
				        return 0;
                    }
                default:
                    break;
			}
		}
		else if (wParam==BN_CLICKED && src==filemenubutton)
		{
			filemenu.TrackPopupMenu(TPM_CENTERALIGN|TPM_LEFTBUTTON,p.x,p.y-10,m_hWnd);
			bHandled=1;
			return 0;
		}
		else if (wParam==BN_CLICKED && src==editmenubutton)
		{
			editmenu.TrackPopupMenu(TPM_CENTERALIGN|TPM_LEFTBUTTON,p.x,p.y-10,m_hWnd);
			bHandled=1;
			return 0;
		}
		else if (wParam==BN_CLICKED && src==recordbutton)
		{
			int check=recordbutton.GetCheck();
			if (check)
				msStartRecord();
			else
			{
				bHandled=1;
				fileMTmp tmpfile;
				tmpfile.open();
				if (msStopRecord(tmpfile))
				{

					printf("ficken2: %d, %d\n",tmpfile.tell(),tmpfile.size());

					static char szFilter[] = "V2 modules (*.v2m)\0*.v2m\0All Files (*.*)\0*.*\0\0";
					CFileDialog cf(FALSE,"*.v2m",filename,OFN_HIDEREADONLY,szFilter,m_hWnd);
					if (cf.DoModal()==IDCANCEL)
						return 0;
					CString path=cf.m_szFileName;
					
					if (!path.GetLength()) return 0;

					fileS out;
					sBool ok=sFALSE;
					if (out.open(path,fileS::cr|fileS::wr))
					{
						tmpfile.seek(0);
						printf("tmpsize is %d\n",tmpfile.size());
						char buf[65536];
						sU32 bread;
						do
						{
							bread=tmpfile.read(buf,65536);
							printf("bread: %d\n",bread);
							out.write(buf,bread);
						} while (bread==65536);
						ok=sTRUE;
					}

					out.close();
					if (!ok)
						MessageBox("Save failed!","Farbrausch V2",MB_ICONERROR);
					return 0;

				}
				else
					MessageBox("Nothing recorded!","Farbrausch V2",MB_ICONINFORMATION|MB_OK);
			}
			return 0;
		}
		else if (!lParam) // menu command?
		{
			switch (wParam)
			{
			case 0x100: // load bank/patch
				{
					static char szFilter[] = "V2 patches/banks\0*.v2b;*.v2p\0V2 sound banks (*.v2b)\0*.v2b\0V2 patches (*.v2p)\0*.v2p\0All Files\0*.*\0\0";
 					CFileDialog cf(TRUE,"*.v2b;*.v2p",NULL,OFN_HIDEREADONLY,szFilter,m_hWnd);
					if (cf.DoModal()==IDCANCEL)
						return 0;
					char *ptr=cf.m_szFileName;
					CString s,fn,path=ptr;
					if (!path.GetLength())
						return 0;

					fileS in;
					sBool ok=sFALSE;
					if (in.open(path))
						if (sdLoad(in))
						{
							filename=path;
							ok=sTRUE;
						}

					in.close();
					v2vstiReset();
                    UpdatePatchNames();
					::SendMessage(GetParent(),VPE_GOTOPATCH,v2curpatch,0);
					if (!ok)
						MessageBox("Load failed!","Farbrausch V2",MB_ICONERROR);
					return 0;
				}
				break;
			case 0x101: // import v2m
				{
					static char szFilter[] = "V2 modules (*.v2m)\0*.v2m;*.v2m\0All Files\0*.*\0\0";
 					CFileDialog cf(TRUE,"*.v2m",NULL,OFN_HIDEREADONLY,szFilter,m_hWnd);
					if (cf.DoModal()==IDCANCEL)
						return 0;
					char *ptr=cf.m_szFileName;
					char fn[256]; strcpy(fn,ptr);
					char *r=strrchr(fn,'/'); if (r) strcpy(fn,r+1);
					r=strrchr(fn,'\\'); if (r) strcpy(fn,r+1);
					r=strrchr(fn,':'); if (r) strcpy(fn,r+1);
					r=strrchr(fn,'.'); if (r) *r=0;

					CString s,path=ptr;
					if (!path.GetLength())
						return 0;

					fileS in;
					sBool ok=sFALSE;
					if (in.open(path))
						if (sdImportV2MPatches(in,fn))
						{
							ok=sTRUE;
						}

					in.close();
					v2vstiReset();
                    UpdatePatchNames();
					::SendMessage(GetParent(),VPE_GOTOPATCH,v2curpatch,0);
					if (!ok)
						MessageBox("Load failed!","Farbrausch V2",MB_ICONERROR);
					return 0;
				}
				break;
			case 0x102: // save bank
				{
					static char szFilter[] = "V2 sound banks (*.v2b)\0*.v2b\0All Files (*.*)\0*.*\0\0";
					CFileDialog cf(FALSE,"*.v2b",filename,OFN_HIDEREADONLY,szFilter,m_hWnd);
					if (cf.DoModal()==IDCANCEL)
						return 0;
					CString path=cf.m_szFileName;
					
					if (!path.GetLength()) return 0;

					fileS out;
					sBool ok=sFALSE;
					if (out.open(path,fileS::cr|fileS::wr))
						if (sdSaveBank(out))
						{
							filename=path;
							ok=sTRUE;
						}

					out.close();
					if (!ok)
						MessageBox("Save failed!","Farbrausch V2",MB_ICONERROR);
					return 0;
				}
				break;
			case 0x103: // save patch
				{
					char fn[256];
					strcpy(fn,patchnames[v2curpatch]);
					strcat(fn,".v2p");
					static char szFilter[] = "V2 patches (*.v2p)\0*.v2p\0All Files (*.*)\0*.*\0\0";
					CFileDialog cf(FALSE,"*.v2p",fn,OFN_HIDEREADONLY,szFilter,m_hWnd);
					if (cf.DoModal()==IDCANCEL)
						return 0;
					CString path=cf.m_szFileName;
					
					if (!path.GetLength()) return 0;

					fileS out;
					sBool ok=sFALSE;
					if (out.open(path,fileS::cr|fileS::wr))
						if (sdSavePatch(out))
						{
							ok=sTRUE;
						}

					out.close();
					if (!ok)
						MessageBox("Save failed!","Farbrausch V2",MB_ICONERROR);
					return 0;
				}
				break;
			case 0x200: // copy patch
				{
					sdCopyPatch();
				}
				break;
			case 0x201: // paste patch
				{
					sdPastePatch();
                    UpdatePatchNames();
					::SendMessage(GetParent(),VPE_GOTOPATCH,v2curpatch,0);
				}
				break;
			case 0x202: // init patch
				{
					sdInitPatch();
                    UpdatePatchNames();
					::SendMessage(GetParent(),VPE_GOTOPATCH,v2curpatch,0);
				}
				break;
			}
		}
		/*
		else if (src==exportbutton && wParam==BN_CLICKED)
		{
			// TODO: add code to initialize document
			static char szFilter[] = "ViruZ II sound banks (*.v2b)\0*.v2b\0All Files (*.*)\0*.*\0\0";
			CFileDialog cf(FALSE,"*.sbi",filename,OFN_HIDEREADONLY,szFilter,m_hWnd);
			cf.DoModal();
			CString path=cf.m_szFileName;
			
			if (!path.GetLength())
				return 0;

			OutputDebugString("Saving: ");
			OutputDebugString(path);
			OutputDebugString("\n");

			fileS out;
			if (!out.open(path,fileS::cr|fileS::wr)) goto saveerr;
			if (!out.puts("v2p0")) goto saveerr;
			// 1: patchnamen
			if (out.write(patchnames,128*32)<128*32) goto saveerr;

			// 2: patchdaten
			if (!out.putsU32(smsize-128*4)) goto saveerr;
			if (out.write(soundmem+128*4,smsize-128*4)<smsize-128*4) goto saveerr;

			// 3: globals
			if (!out.putsU32(v2ngparms)) goto saveerr;
			if (out.write(globals,v2ngparms)<v2ngparms) goto saveerr;

			out.close();

			filename=path;
			return 0;

			saveerr:
			out.close();
			MessageBox("Save failed!","Farbrausch Viruz II",MB_ICONERROR);
			return 0;
		}
		/*
		else if (src==devsel)
		{
			switch (wParam>>16)
			{
			case CBN_SELENDOK:  // Patchwexel
				msSetDevice(devsel.GetCurSel()-1);
				return 0;
			}
		}
		else if (src==panicbutton && wParam==BN_CLICKED)
		{
			msStopAudio();
			HWND ich=m_hWnd, er;
			while (er=::GetParent(ich)) ich=er;
			msStartAudio(ich,soundmem,globals);
		}*/



		return 0;
	}

	LRESULT OnEdit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{
		if (!editmode)
		{
			editmode=1;
			UpdateDisplay(0);
		}

		// lr: instant store
		StorePatch();
		UpdateDisplay(0);

		return 0;
	}

	LRESULT OnHScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		bHandled=0;
		return 0;
	}

	LRESULT OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		int po[17], pg[16];
		CString s;
		synthGetPoly(po);
		synthGetPgm(pg);
		for (int i=0; i<16; i++)
		{
			if (po[i]!=gpo[i])
			{
				s.Format("%d",po[i]);
				poly[i].SetWindowText(s);
				gpo[i]=po[i];
			}
			if (pg[i]!=gpg[i])
			{
				s.Format("%03d",pg[i]);
				pnos[i].SetWindowText(s);
				gpg[i]=pg[i];
			}
		}
		if (po[16]!=gpo[16])
		{
			s.Format("%d",po[16]);
			pototal.SetWindowText(s);
			gpo[16]=po[16];
		}

		int cpu=msGetCPUUsage();
    if (cpu!=oldcpu)
		{
			s.Format("%3d%%",cpu);
			cpumeter.SetWindowText(s);
			oldcpu=cpu;
		}

		float l,r;
		int cl, cr;
		msGetLD(&l,&r,&cl,&cr);
		if (l) l=(float)(18.0f*(log(l)/log(2.0f))); else l=-100;
		if (r) r=(float)(18.0f*(log(r)/log(2.0f))); else l=-100;
		if (cl) 
		{
			clipl=5;
			vleft.SetBarColor(0xff4080ff);
		}
		if (cr) {
			clipr=5;
			vright.SetBarColor(0xff4080ff);
		}

		vleft.SetPos(90+(int)(l));
		vright.SetPos(90+(int)(r));

		if (clipl)
		{
			clipl--;
			if (!clipl) vleft.SetBarColor(CLR_DEFAULT);
		}
		if (clipr) 
		{
			clipr--;
			if (!clipr) vright.SetBarColor(CLR_DEFAULT);
		}

		return 0;
	}

	LRESULT OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		bHandled=0;
		return 0;
	}

	LRESULT OnGotoPatch(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{
		SetEditBuffer((int)wParam);
		UpdateDisplay(1);
		return 0;
	}

};



/*******************************************************************************\
*
*	Unterpanel
*
\*******************************************************************************/


class CViruZWorkspace;
typedef CWindowImpl<CViruZWorkspace> vwhorzsplitter;

class CViruZWorkspace : public vwhorzsplitter
{
public:
	DECLARE_WND_CLASS_EX(_T("fr_ViruZWorkspace"), CS_DBLCLKS, COLOR_BTNFACE)

	CTabCtrl tubby;
	HWND		 views[10];
	int      nv;

	CViruZPatchEdit patched;
	CViruZGlobalsPanel globed;
#ifdef RONAN
	CViruZLisaPanel speeched;
#endif

	char *ptr;

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		pMsg;
		return FALSE;
	}

	BEGIN_MSG_MAP(CViruZWorkspace)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(VPE_SETPOINTER, OnSetPointer)
		MESSAGE_HANDLER(VPE_EDIT, OnEdit)
		MESSAGE_HANDLER(VPE_PARAMCHG, OnParamChg)
		MESSAGE_HANDLER(WM_COMMAND, OnCommand)
		MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
		MESSAGE_HANDLER(WM_SIZE, OnCWP)
	END_MSG_MAP()

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		tubby.DestroyWindow();
		patched.DestroyWindow();
		globed.DestroyWindow();
#ifdef RONAN
		speeched.DestroyWindow();
#endif
		bHandled=0;
		return 0;
	}

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{
		
		HFONT sysfnt=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
		
		ptr=0;

		CRect wr;
		GetWindowRect(wr);

		int h=wr.Height();
		int w=wr.Width();
		nv=0;

		tubby.Create(m_hWnd, CRect(0,0,wr.right-5,wr.bottom-5), "", WS_VISIBLE|WS_CHILD|TCS_FLATBUTTONS|TCS_BUTTONS|TCS_HOTTRACK);
		tubby.SetFont(sysfnt);

		patched.Create(m_hWnd, CRect(100,100,300,300), "", WS_CHILD, WS_EX_STATICEDGE);
		views[0]=patched;			TCITEM bla0={ TCIF_TEXT, 0, 0, "Patch Editor", 10, -1, 0};
		tubby.InsertItem(0,&bla0); nv++;

		globed.Create(m_hWnd, CRect(200,200,400,400), "", WS_CHILD, WS_EX_STATICEDGE);
		views[1]=globed;			TCITEM bla1={ TCIF_TEXT, 0, 0, "Globals", 10, -1, 0};
		tubby.InsertItem(1,&bla1); nv++;

#ifdef RONAN
		speeched.Create(m_hWnd, CRect(300,300,500,500), "", WS_CHILD, WS_EX_STATICEDGE);
		views[2]=speeched;		TCITEM bla2={ TCIF_TEXT, 0, 0, "Speech", 10, -1, 0};
		tubby.InsertItem(2,&bla2); nv++;
#endif

		patched.ShowWindow(SW_SHOW);

		return 0;
	}

	LRESULT OnParamChg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{		
		if(lParam < v2nparms) {
			::SendMessage(patched.m_hWnd, VPE_PARAMCHG, wParam, lParam);
		} else {
			::SendMessage(globed.m_hWnd, VPE_PARAMCHG, wParam, lParam-v2nparms);
		}
		return 0;
	}

	LRESULT OnSetPointer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{
		::SendMessage(patched.m_hWnd, VPE_SETPOINTER, wParam, lParam);
		::SendMessage(globed.m_hWnd, VPE_SETPOINTER, lParam, wParam);
		return 0;
	}

	LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{
		return 0;
	}

	LRESULT OnCWP(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{
		CRect wr;
		GetWindowRect(wr);
		tubby.MoveWindow(0,0,wr.Width(),wr.Height());
		CRect cr(0,0,wr.Width(),wr.Height());
		tubby.AdjustRect(0,cr);
		for (int i=0; i<nv; i++)
			::MoveWindow(views[i],cr.left-4,cr.top-2,cr.right+4,cr.bottom-16,1);
		return 0;
	}


	LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{
		NMHDR *nm=(LPNMHDR)lParam;
		if (nm->hwndFrom==tubby && nm->code==TCN_SELCHANGE)
		{
			int n=tubby.GetCurSel();
			ATLTRACE("sel window %d!\n",n);

			for (int i=0; i<nv; i++)
				if (i!=n) ::ShowWindow(views[i], SW_HIDE);
			::ShowWindow(views[n], SW_SHOW);
		}
		return 0;
	}

	LRESULT OnEdit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{
		::SendMessage(GetParent(), VPE_EDIT, wParam, lParam);
		return 0;
	}
};


/*******************************************************************************\
*
*	Mainpanel
*
\*******************************************************************************/


class CViruZClientWindow;
typedef CSplitterWindowImpl<CViruZClientWindow, false> vcwhorzsplitter;

class CViruZClientWindow : public vcwhorzsplitter
{
public:
	DECLARE_WND_CLASS_EX(_T("fr_ViruZClientWin"), CS_DBLCLKS, COLOR_WINDOW)

	CViruZStatusPanel oberpane;
	CViruZWorkspace unterpane;

	char *ptr;

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		pMsg;
		return FALSE;
	}

	virtual BOOL OnIdle()
	{
		return FALSE;
	}

    void EnablePatchControls()
    {
        // gets called before creation.

        // TODO: on call enable navigation buttons,
        // prepare displaying of combobox instead
        // of static control.
        oberpane.EnablePatchControls();
    }

	BEGIN_MSG_MAP(CViruZClientWindow)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(VPE_SETPOINTER, OnSetPointer)
		MESSAGE_HANDLER(VPE_EDIT, OnEdit)
		MESSAGE_HANDLER(VPE_GOTOPATCH, OnGotoPatch)
		MESSAGE_HANDLER(VPE_PARAMCHG, OnParamChg)
		MESSAGE_HANDLER(WM_COMMAND, OnCommand)
		CHAIN_MSG_MAP(vcwhorzsplitter)
	END_MSG_MAP()

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		SetSplitterPanes(0,0);
		oberpane.DestroyWindow();
		unterpane.DestroyWindow();
		bHandled=0;
		return 0;
	}

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{
		vcwhorzsplitter::OnCreate (uMsg, wParam, lParam, bHandled);
		
		ptr=0;

		unterpane.Create(m_hWnd, CRect(0,0,430,400), "", WS_VISIBLE|WS_CHILD);
		oberpane.Create(m_hWnd, CRect(0,0,430,400), "", WS_VISIBLE|WS_CHILD);

		SetSplitterPanes(oberpane,unterpane);
		SetSplitterExtendedStyle(SPLIT_NONINTERACTIVE);
		SetSplitterRect(CRect(0,0,0,200));
		SetSplitterPos(-1);

		return 0;
	}

	LRESULT OnParamChg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{
		::SendMessage(unterpane.m_hWnd, VPE_PARAMCHG, wParam, lParam);
		return 0;
	}

	LRESULT OnSetPointer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{
		::SendMessage(unterpane.m_hWnd, VPE_SETPOINTER, wParam, lParam);
		return 0;
	}

	LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{
		return 0;
	}

	LRESULT OnEdit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{
		::SendMessage(oberpane.m_hWnd, VPE_EDIT, wParam, lParam);
		return 0;
	}

	LRESULT OnGotoPatch(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
	{
		::SendMessage(oberpane.m_hWnd, VPE_GOTOPATCH, wParam, lParam);
		return 0;
	}

};



/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIRUZPANEL_H__213CE356_CC77_11D4_9322_00A0CC20D58C__INCLUDED_)

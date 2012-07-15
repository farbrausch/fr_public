// MULTIBUTTON.h : interface of the CMULTIBUTTON class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MULTIBUTTON_H__213CE356_CC77_11D4_9322_00A0CC20D58C__INCLUDED_)
#define AFX_MULTIBUTTON_H__213CE356_CC77_11D4_9322_00A0CC20D58C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CMultiButton : public CWindowImpl<CMultiButton>
{
public:
	DECLARE_WND_CLASS("fr_MultiButton")

	HFONT myfont;
	CButton buttons[128];

	unsigned long nbuttons;
	char strings[4096];

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		pMsg;
		return FALSE;
	}

	BEGIN_MSG_MAP(CMultiButton)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_COMMAND, OnCommand)
		MESSAGE_HANDLER(TBM_SETPOS, OnSetVal)
	END_MSG_MAP()


	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		DeleteObject(myfont);
		return 0;
	}

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{

		GetWindowText(strings,4095);

		myfont=(HFONT)::GetStockObject(DEFAULT_GUI_FONT);

		int width=50;
		int n=1;
		int max=7;
		char *s,*sp=strings;
    int i;

		if (sp[0]=='!')
		{
			sp++;
			max=3;
		}

		do
		{
			s=sp+1;
			sp=strchr(s,'|');
			if (sp) n++;
		} while (sp);

	  if (n>max) width=max*width/n;
		
		s=strings[0]=='!'?strings+1:strings;
		char s2[256];
		for (i=0; i<128; i++)
		{
			if (!*s)
				break;
			char *sx=strchr(s,'|');
			if (sx)
			{
				strncpy(s2,s,(sx-s));
				s2[sx-s]=0;
			}
			else
				strcpy(s2,s);
			s+=strlen(s2);
			while (*s=='|') s++;
			buttons[i].Create(m_hWnd,CRect(width*i,0,width*(i+1)-1,20),s2,WS_VISIBLE|WS_CHILD|BS_AUTORADIOBUTTON|BS_PUSHLIKE|BS_FLAT|(i?0:WS_GROUP));
			buttons[i].SetFont(myfont);
		}
		SetWindowPos(0,0,0,width*i-1,20,SWP_NOMOVE|SWP_NOZORDER);
		nbuttons=i;
		return 0;
	}

	LRESULT OnCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		for (unsigned long i=0; i<nbuttons; i++) if (lParam==(LPARAM)buttons[i].m_hWnd && wParam==BN_CLICKED)
		{
			::SendMessage(GetParent(), WM_COMMAND, (WPARAM)i, (LPARAM)m_hWnd);
		}
		return 0;
	}

	LRESULT OnSetVal(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
	{
		unsigned long bla=(unsigned long)lParam;
		if (bla<nbuttons)
		{
			for (unsigned long i=0; i<nbuttons; i++)
				buttons[i].SetCheck(i==bla);
		}
		return 0;
	}

	LRESULT SetVal(UINT n)
	{
		return SendMessage(TBM_SETPOS,0,n);
	}

};




/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MULTIBUTTON_H__213CE356_CC77_11D4_9322_00A0CC20D58C__INCLUDED_)

// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__213CE353_CC77_11D4_9322_00A0CC20D58C__INCLUDED_)
#define AFX_MAINFRM_H__213CE353_CC77_11D4_9322_00A0CC20D58C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include "../sounddef.h"
#include "../tool/file.h"

class CMainFrame : public CFrameWindowImpl<CMainFrame>, public CUpdateUI<CMainFrame>,
		public CMessageFilter, public CIdleHandler
{
public:
	DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)

	CViruZClientWindow m_view;

	CString filename;

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		if(CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
			return TRUE;

		///return m_view.PreTranslateMessage(pMsg);
		return FALSE;
	}

	virtual BOOL OnIdle()
	{
		return FALSE;
	}

	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(ID_FILE_OPEN, OnFileOpen)
		COMMAND_ID_HANDLER(ID_FILE_SAVE, OnFileSave)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		MESSAGE_HANDLER(WM_COMMAND, OnCommand)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
	END_MSG_MAP()

	BEGIN_UPDATE_UI_MAP(CMainFrame)
	END_UPDATE_UI_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{

		m_hWndClient = m_view.Create(m_hWnd, rcDefault, "v2 frame", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
		::SendMessage(m_hWndClient, VPE_GOTOPATCH, 0,0);

		CMessageLoop* pLoop = _Module.GetMessageLoop();		pLoop->AddMessageFilter(this);
		pLoop->AddIdleHandler(this);

		return 0;
	}

	LRESULT OnCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		CString s;
		s.Format("got cmd: %x / %x\n",wParam,lParam);
		OutputDebugString(s);
		return 0;
	}

	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		PostMessage(WM_CLOSE);
		return 0;
	}


	LRESULT OnFileOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		static char szFilter[] = "V.II sound banks (*.v2b)\0*.v2b\0All Files (*.*)\0*.*\0\0";
 		CFileDialog cf(TRUE,"*.v2b",NULL,OFN_HIDEREADONLY,szFilter,m_hWnd);
		cf.DoModal();
		char *ptr=cf.m_szFileName;
		CString s,fn,path=ptr;
	  if (!path.GetLength())
			return 0;

    fileS in;
		int   i,fver=-1;

		char id[5];
		int  sml, pw;
		id[4]=0;
		if (!in.open(path)) goto loaderr;
		if (in.read(id,4)<4) goto loaderr;
		if (strcmp(id,"v2p0")) goto loaderr;
		if (in.read(patchnames,128*32)<128*32) goto loaderr;
		sml=in.getsU32();
		pw=sml/128;

		sU32 globsize;
		in.seekcur(sml);
		in.read(globsize);
		in.seekcur(-(sml+4));

	
		s.Format("Sizes: Patch %d, Globals %d\n",pw,globsize);
		OutputDebugString(s);

		for (i=0; i<=v2version; i++)
			if (pw==v2vsizes[i] && globsize==v2gsizes[i]) fver=i;

		if (fver==-1)
		{
			MessageBox("Sound defs in file have unknown size... aborting!",
				   "Farbrausch V.II",MB_ICONERROR);
			in.close();
			::SendMessage(m_hWndClient,VPE_GOTOPATCH,0,0);
			return 0;
		}
		else if (fver!=v2version)
		{
			s.Format("Found version %d\n",fver);
			OutputDebugString(s);
			MessageBox("Old file format, trying to convert",
				   "Farbrausch V.II",MB_ICONINFORMATION);

			int rss=pw-255*3-1;
			for (i=0; i<128; i++)
			{
				sU8 *patch=soundmem+128*4+v2soundsize*i;
				// fill patch with default values
				memcpy(patch,v2initsnd,v2soundsize);
				// load patch
				for (int j=0; j<v2nparms; j++)
					if (v2parms[j].version<=fver)
						patch[j]=in.getsU8();

				patch+=v2nparms;
				in.read(patch,1);
			  // remap mods
				for (int j=0; j<*patch; j++)
				{
					sU8 *mod=patch+3*j+1;
					if (in.read(mod,3)<3) goto loaderr;
					for (int k=0; k<=mod[2]; k++)
						if (v2parms[k].version>fver) mod[2]++;
				}
				if (in.read(patch+v2nparms,3*(255-*patch))<3*(255-*patch)) goto loaderr;
			}

		}
		else
			if (in.read(soundmem+128*4,sml)<sml) goto loaderr;

		if (!in.eof())
		{
			memcpy(globals,v2initglobs,v2ngparms);
			sml=in.getsU32();
			if (sml<v2ngparms)
			{
				for (int j=0; j<v2ngparms; j++)
					if (v2gparms[j].version<=fver)
						globals[j]=in.getsU8();
			}
			else
			{
				if (in.read(globals,v2ngparms)<v2ngparms) goto loaderr;
				in.seekcur(sml-v2ngparms);
			}
		}

#ifdef RONAN
		ZeroMemory(speech,64*256);
#endif

		if (!in.eof())
		{
			sml=in.getsU32();
#ifdef RONAN
			if (sml<=64*256)
				in.read(speech,sml);
			else
			{
				in.read(speech,64*256);
				in.seekcur(sml-64*256);
			}
#else
			if (sml) MessageBox("Warning: Not loading speech synth texts\nIf you overwrite the file later, the texts will be lost!","Farbrausch V.II",MB_ICONEXCLAMATION);
			in.seekcur(sml);
#endif
		}

		in.close();
		filename=path;
		::SendMessage(m_hWndClient,VPE_GOTOPATCH,0,0);
		return 0;

		loaderr:
		in.close();
		::SendMessage(m_hWndClient,VPE_GOTOPATCH,0,0);
		MessageBox("Load failed!","Farbrausch V.II",MB_ICONERROR);
		return 0;

	}

	LRESULT OnClose(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if (MessageBox("Really quit?", "Farbrausch V.II", MB_ICONWARNING|MB_OKCANCEL) == IDOK)
			bHandled=0;
		return 0;
	}

	LRESULT OnFileSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		// TODO: add code to initialize document
		static char szFilter[] = "V.II sound banks (*.v2b)\0*.v2b\0All Files (*.*)\0*.*\0\0";
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

		// 4: speech synth
#ifdef RONAN
		if (!out.putsU32(64*256)) goto saveerr;
		if (out.write(speech,64*256)<64*256) goto saveerr;
#else
		if (!out.putsU32(0)) goto saveerr;
#endif

		out.close();

		filename=path;
		return 0;

		saveerr:
		out.close();
		MessageBox("Save failed!","Farbrausch V.II",MB_ICONERROR);
		return 0;
	}

	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CAboutDlg dlg;
		dlg.DoModal();
		return 0;
	}
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__213CE353_CC77_11D4_9322_00A0CC20D58C__INCLUDED_)

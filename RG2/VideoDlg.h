// This code is in the public domain. See LICENSE for details.

#ifndef __VideoDlg_h__
#define __VideoDlg_h__

class CVideoDialog: public CDialogImpl<CVideoDialog>
{
public:
  enum { IDD=IDD_VIDEO };

	struct {
		sInt xres, yres;
		sF32 aspect, fps;
		sInt fieldsenable;
		sInt fieldsmode;
		sBool isavi;
    sBool audio;
		char filename[1024];
	} config;

  BEGIN_MSG_MAP(CVideoDialog)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
    COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
    COMMAND_HANDLER(IDC_FILEBUTTON, BN_CLICKED, OnSelFile)
    COMMAND_HANDLER(IDC_BMPSEQ, BN_CLICKED, OnClickedBmpSeq)
    COMMAND_HANDLER(IDC_AVI, BN_CLICKED, OnClickedAvi)
//    COMMAND_HANDLER(IDC_SELECTNONE, BN_CLICKED, OnSelect)
  END_MSG_MAP()



  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
		// fill default values
    SetDlgItemText(IDC_XPELS,"640");
    SetDlgItemText(IDC_YPELS,"480");
    SetDlgItemText(IDC_ASPECT,"1.0");
    SetDlgItemText(IDC_FPS,"29.97");
    SetDlgItemText(IDC_FILENAME,"");
		SendDlgItemMessage(IDC_FIELDS0, BM_SETCHECK, 1, 0);
		SendDlgItemMessage(IDC_BMPSEQ, BM_SETCHECK, 1, 0);
		::EnableWindow(GetDlgItem(IDC_AUDIO),FALSE);
		::EnableWindow(GetDlgItem(IDC_VIDEOCONF),FALSE);
    CenterWindow(GetParent());
    return TRUE;
  }

  
  LRESULT OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
  {
    if (wID==IDOK)
    {
			char buf[4096];
			GetDlgItemText(IDC_XPELS,buf,4095);
			config.xres=atoi(buf);
			GetDlgItemText(IDC_YPELS,buf,4095);
			config.yres=atoi(buf);
			GetDlgItemText(IDC_ASPECT,buf,4095);
			config.aspect=atof(buf);
			GetDlgItemText(IDC_FPS,buf,4095);
			config.fps=atof(buf);
			config.fieldsenable=(SendDlgItemMessage(IDC_FIELDS0,BM_GETCHECK,0,0))?0:1;
			config.fieldsmode=(SendDlgItemMessage(IDC_FIELDS2,BM_GETCHECK,0,0))?1:0;
			config.isavi=(SendDlgItemMessage(IDC_AVI,BM_GETCHECK,0,0))?1:0;
			config.audio=(SendDlgItemMessage(IDC_AUDIO,BM_GETCHECK,0,0))?1:0;
			GetDlgItemText(IDC_FILENAME,config.filename,1023);
		}

    EndDialog(wID);
    return 0;
  }

  LRESULT OnSelFile(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)    
	{
		LPCTSTR exts, defext;
		if (SendDlgItemMessage(IDC_BMPSEQ,BM_GETCHECK,0,0))
		{
			defext=_T("bmp");
			exts=_T("Windows Bitmap (*.bmp)\0*.bmp\0");
		}
		else
		{
			defext=_T("avi");
			exts=_T("AVI movie file (*.avi)\0*.avi\0");
		}

		CFileDialog file(FALSE, defext, 0, OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT, exts);

		if (file.DoModal()==IDOK)
		{
			SetDlgItemText(IDC_FILENAME,file.m_ofn.lpstrFile);			
		}

		return 0;
	}

  LRESULT OnClickedBmpSeq(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)    
	{
		::EnableWindow(GetDlgItem(IDC_AUDIO),FALSE);
    return 0;
	}

	LRESULT OnClickedAvi(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)    
	{
		::EnableWindow(GetDlgItem(IDC_AUDIO),TRUE);
    return 0;
	}
};

#endif

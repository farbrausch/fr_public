// This code is in the public domain. See LICENSE for details.

#ifndef __preferences_h__
#define __preferences_h__

class CPreferencesDialog: public CDialogImpl<CPreferencesDialog>
{
public:
  enum { IDD=IDD_PREFERENCES };

  CTrackBarCtrl m_slopeKeySlider;

  BEGIN_MSG_MAP(CPreferencesDialog)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
    COMMAND_HANDLER(IDC_CONTENT_DIR_BUTTON, BN_CLICKED, OnDirButtonClicked)
    COMMAND_HANDLER(IDC_BACKUP_DIR_BUTTON, BN_CLICKED, OnDirButtonClicked)
    COMMAND_HANDLER(IDC_SLOPEKEYDELAY_EDIT, EN_CHANGE, OnSlopeKeyEditChange)
    COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
    COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    m_slopeKeySlider=GetDlgItem(IDC_SLOPEKEYDELAY_SLIDER);

    m_slopeKeySlider.SetRange(100, 500);
    m_slopeKeySlider.SetTicFreq(25);
    m_slopeKeySlider.SetLineSize(25);
    m_slopeKeySlider.SetPageSize(100);
    m_slopeKeySlider.SetPos(g_config->getSlopeKeyDelay());

    SetDlgItemInt(IDC_SLOPEKEYDELAY_EDIT, g_config->getSlopeKeyDelay());
    
    SendDlgItemMessage(IDC_CONTENT_DIR_EDIT, EM_LIMITTEXT, _MAX_PATH-1);
    SetDlgItemText(IDC_CONTENT_DIR_EDIT, g_config->getContentPath());

    SendDlgItemMessage(IDC_BACKUP_DIR_EDIT, EM_LIMITTEXT, _MAX_PATH-1);
    SetDlgItemText(IDC_BACKUP_DIR_EDIT, g_config->getBackupPath());
    
    CenterWindow(GetParent());

    return TRUE;
  }

  LRESULT OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    SetDlgItemInt(IDC_SLOPEKEYDELAY_EDIT, m_slopeKeySlider.GetPos());

    return 0;
  }

  LRESULT OnSlopeKeyEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
  {
    sInt val=GetDlgItemInt(IDC_SLOPEKEYDELAY_EDIT);
    m_slopeKeySlider.SetPos(val);

    return 0;
  }

  LRESULT OnDirButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtrl, BOOL& bHandled)
  {
    BROWSEINFO inf;
    TCHAR path[MAX_PATH];
    
    inf.hwndOwner=m_hWnd;
    inf.pidlRoot=0;
    inf.lpszTitle=(wID==IDC_CONTENT_DIR_BUTTON)?_T("Select Content Directory"):_T("Select Backup Directory");
    inf.ulFlags=BIF_USENEWUI|BIF_VALIDATE;
    inf.lpfn=0;
    inf.lParam=0;
    inf.pszDisplayName=path;

    LPITEMIDLIST result=SHBrowseForFolder(&inf);
    
    if (result)
    {
      SHGetPathFromIDList(result, path);
      SetDlgItemText((wID==IDC_CONTENT_DIR_BUTTON)?IDC_CONTENT_DIR_EDIT:IDC_BACKUP_DIR_EDIT, path);
    }

    return 0;
  }

  LRESULT OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
  {
    if (wID==IDOK)
    {
      TCHAR cpath[MAX_PATH], bpath[MAX_PATH];

      GetDlgItemText(IDC_CONTENT_DIR_EDIT, cpath, MAX_PATH);
      GetDlgItemText(IDC_BACKUP_DIR_EDIT, bpath, MAX_PATH);
      
      g_config->setSlopeKeyDelay(m_slopeKeySlider.GetPos());
      g_config->setContentPath(cpath);
      g_config->setBackupPath(bpath);
    }

    EndDialog(wID);
    return 0;
  }
};

#endif

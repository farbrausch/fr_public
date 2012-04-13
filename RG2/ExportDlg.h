// This code is in the public domain. See LICENSE for details.

#ifndef __ExportDlg_h__
#define __ExportDlg_h__

#include "exporter.h"

class CExportDialog: public CDialogImpl<CExportDialog>
{
public:
  enum { IDD=IDD_EXPORT };

  CListBox m_listBox;
  std::vector<sU32> m_selList;
  std::vector<fr::string> m_nameList;

  BEGIN_MSG_MAP(CExportDialog)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_HANDLER(IDC_SELECTALL, BN_CLICKED, OnSelect)
    COMMAND_HANDLER(IDC_SELECTNONE, BN_CLICKED, OnSelect)
    COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
    COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    m_listBox=GetDlgItem(IDC_TEXTURELIST);

    for (frOpGraph::opMapIt it=g_graph->m_ops.begin(); it!=g_graph->m_ops.end(); ++it)
    {
      if (it->second.plg->isFinal()) // final store?
      {
        frParam *name=it->second.plg->getParam(0);

        sInt n=m_listBox.AddString(name->stringv);
        m_selList.push_back(it->second.opID);
        m_nameList.push_back(name->stringv);
      }
    }

    m_listBox.SelItemRange(TRUE, 0, m_listBox.GetCount());
      
    CenterWindow(GetParent());

    return TRUE;
  }

  LRESULT OnSelect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)    
  {
    m_listBox.SelItemRange(wID==IDC_SELECTALL, 0, m_listBox.GetCount());

    return 0;
  }
  
  LRESULT OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
  {
    if (wID==IDOK && m_listBox.GetSelCount())
    {
      CFileDialog file(FALSE, _T("tex"), 0, OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT, _T("Texture Generator 2 Export (*.tex)\0*.tex\0"));

      if (file.DoModal()==IDOK)
      {
        fr::streamF f;

        if (f.open(file.m_ofn.lpstrFile, fr::streamF::wr|fr::streamF::cr))
        {
          frGraphExporter export;
          
          export.prepareExport();

          for (sInt i=0; i<m_listBox.GetCount(); i++)
          {
            if (m_listBox.GetSel(i)>0)
              export.addToExport(m_selList[i]);
          }

          sInt ncnt=export.finishExport(f);
          sInt sz=f.size();
          f.close();

          fr::streamF m;
          m.open("mapping.txt", fr::streamF::wr|fr::streamF::cr);
          for (i=0; i<m_listBox.GetCount(); i++)
          {
            if (m_listBox.GetSel(i)>0)
            {
              const sChar *desc=m_nameList[i];
              sU32 mapped=export.mapTexture(desc);

              sChar buf[768];
              sprintf(buf, "%s = %d\n", desc, mapped);
              m.write(buf, strlen(buf));
            }
          }
          m.close();

          TCHAR buf[1024];
          wsprintf(buf, "Successfully Exported. %d bytes, %d buffers required.", sz, ncnt);

          MessageBox(buf, _T("Export successful"), MB_ICONINFORMATION|MB_OK);
        }
        else
        {
          CString str;
          str.LoadString(IDR_MAINFRAME);
          MessageBox(_T("Export failed"), str, MB_ICONERROR|MB_OK);
        }
      }
    }

    EndDialog(wID);
    return 0;
  }
};

#endif

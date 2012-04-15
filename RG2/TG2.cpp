// This code is in the public domain. See LICENSE for details.

// TG2.cpp : main source file for TG2.exe
//

#include "stdafx.h"

#include <atlframe.h>
#include "resource.h"

LPCTSTR lpcstrTG2RegKey=_T("Software\\Farbrausch\\Texture Generator 2");

#include "TG2View.h"
#include "MainFrm.h"
#include "ruleMouseInput.h"
#include "frOpGraph.h"
#include "stream.h"
#include "tstream.h"

CAppModule _Module;
static HWND mbHWnd = 0;

class CMyMessageLoop: public CMessageLoop
{
public:
  virtual BOOL OnIdle(int nIdleCount)
  {
    BOOL state = FALSE;
    
    for (int i = 0; i < m_aIdleHandler.GetSize(); i++)
    {
      CIdleHandler* pIdleHandler = m_aIdleHandler[i];
      if (pIdleHandler)
        state |= pIdleHandler->OnIdle();
    }

		Sleep(1);
    
    return state;
  }
};

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMyMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainFrame wndMain;
  RECT rc=CWindow::rcDefault;
  CRegKey key;
  
  DWORD nShow=nCmdShow;
  sChar winpos[128];
  sU32 winsz=128;

  winpos[0] = 0;

  if (key.Open(HKEY_CURRENT_USER, lpcstrTG2RegKey, KEY_READ) == ERROR_SUCCESS)
  {
    key.QueryDWORDValue(_T("WindowMode"), nShow);
    key.QueryStringValue(_T("WindowPos"), winpos, &winsz);
    key.Close();
  }

  sscanf(winpos, "%d,%d,%d,%d", &rc.left, &rc.top, &rc.right, &rc.bottom);

  if (rc.left>=GetSystemMetrics(SM_CXSCREEN) || rc.top>=GetSystemMetrics(SM_CYSCREEN) ||
    rc.right<=rc.left || rc.bottom<=rc.top) // sanity checks
    rc=CWindow::rcDefault;


	if(wndMain.CreateEx(0, rc) == NULL)
	{
		ATLTRACE(_T("Main window creation failed!\n"));
		return 0;
	}

  mbHWnd = wndMain.m_hWnd;
  ruleMouseInit(mbHWnd);

	wndMain.ShowWindow(nShow);
	int nRet = theLoop.Run();
	_Module.RemoveMessageLoop();

  ruleMouseClose();
  mbHWnd = 0;

	return nRet;
}

void __stdcall myReportErrorProc(const sChar* errorMsg)
{
	static sChar buffer[4096 + 512];
	strcpy(buffer, errorMsg);

	if (g_graph)
	{
		try
		{
			fr::streamF outf;

			if (outf.open("crash_recover.rg21", fr::streamF::cr | fr::streamF::wr))
			{
				fr::streamWrapper wrap(outf, sTRUE);
				g_graph->serialize(wrap);
				outf.flush();
				outf.close();

				strcat(buffer, "\n\nThe attempt to save the current working data as crash_recover.rg21 "
					"ended successfully. This file may aid you in data recovery.");
			}
			else
				strcat(buffer, "\n\nCould not open crash_recover.rg21 for writing, unable to save the "
					"current working dataset.");
		}
		catch (...)
		{
			strcat(buffer, "\n\nAn exception happened while trying to save the current working dataset as "
				"crash_recover.rg21. Your working data is lost.");
		}
	}

  ::MessageBox(mbHWnd, buffer, "farbrausch RauschGenerator 2.1", MB_ICONERROR | MB_OK);
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

#if (_WIN32_IE >= 0x0300)
	INITCOMMONCONTROLSEX iccx;
	iccx.dwSize = sizeof(iccx);
	iccx.dwICC = ICC_BAR_CLASSES;	// change to support other controls
	BOOL bRet = ::InitCommonControlsEx(&iccx);
	bRet;
	ATLASSERT(bRet);
#else
	::InitCommonControls();
#endif

  fr::setReportError(myReportErrorProc);

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));


	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}

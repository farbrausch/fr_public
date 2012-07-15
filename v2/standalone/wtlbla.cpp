// wtlbla.cpp : main source file for wtlbla.exe
//


// Change these values to use different versions
#define WINVER		0x0500
#define _WIN32_IE	0x0500
#define _RICHEDIT_VER	0x0200

#include <windows.h>
#include <mmsystem.h>

#include <atlbase.h>
#include <atlapp.h>

#include <atlwin.h>
#include <atlmisc.h>
#include <atlddx.h>
#include <atlgdi.h>
#include <atlctrls.h>
#include <atlctrlw.h>
#include <atlctrlx.h>
#include <atlscrl.h>
#include <atldlgs.h>
#include <atlframe.h>
#include <atlsplit.h>

#include "midi.h"
#include "../types.h"
#include "../soundsys.h"
#include "../sounddef.h"
#include "resource.h"

CAppModule _Module;
extern sU8 v2instance[3*1024*1024];

#include "cstring.h"

#include "wtlblaView.h"
#include "aboutdlg.h"
#include "MainFrm.h"

extern sU8 v2instance[3*1024*1024];


int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	sdInit();
	msInit();

	CMainFrame wndMain;
	if(wndMain.CreateEx(0,CRect(100,100,100+420+10+280,100+570+110)) == NULL)
		return 0;

#ifdef RONAN
	msStartAudio(wndMain,soundmem,globals,(const char **)speechptrs);
#else
	msStartAudio(wndMain,soundmem,globals);
#endif

	wndMain.ShowWindow(nCmdShow);
	wndMain.UpdateWindow();
	int nRet = theLoop.Run();

	msStopAudio();
	msClose();
	sdClose();

	_Module.RemoveMessageLoop();
	return nRet;
}



int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

#if 0
	INITCOMMONCONTROLSEX iccx;
	iccx.dwSize = sizeof(iccx);
	iccx.dwICC = ICC_BAR_CLASSES|ICC_PROGRESS_CLASS ;	// change to support other controls
	BOOL bRet = ::InitCommonControlsEx(&iccx);
	bRet;
	ATLASSERT(bRet);
#else
	::InitCommonControls();
#endif

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));


	int nRet = Run(lpstrCmdLine, nCmdShow);


	_Module.Term();
	::CoUninitialize();

	return nRet;
}

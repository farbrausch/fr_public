// This code is in the public domain. See LICENSE for details.

// the playa.

//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "types.h"
#include "directx.h"
#include "rtlib.h"
#include "soundsys.h"
#include "engine.h"
#include "debug.h"
#include "opsys.h"
#include "cmpop.h"

#undef LOOP
#define THREEINTROS

HWND hViewerWnd;

extern "C" sU8 introData[];

#ifdef _DEBUG
sU32 totalAlloc = 0, peakAlloc = 0, curAlloc = 0;
#endif

LRESULT __stdcall WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
  case WM_PAINT:
    ShowCursor(0);
    ValidateRect(hWnd, 0);
    return 0;

  case WM_KEYDOWN:
    if (wParam != VK_ESCAPE)
      return 1;
    
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }
  
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

#ifdef THREEINTROS
extern "C" sU8 mainDialog[];
static sInt curIntro = 0, curResolution = 0;

INT_PTR __stdcall DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  sInt i;
  RECT rc1,rc2;

  switch(uMsg)
  {
  case WM_INITDIALOG:
    CheckRadioButton(hWnd, 1048, 1048 + 2, 1048 + curIntro);
    CheckRadioButton(hWnd, 1051, 1051 + 2, 1051 + curResolution);
    GetClientRect(GetDesktopWindow(), &rc1);
    GetWindowRect(hWnd, &rc2);
    rc2.right -= rc2.left;
    rc2.bottom -= rc2.top;
    MoveWindow(hWnd, (rc1.right - rc2.right) / 2, (rc1.bottom - rc2.bottom) / 2, rc2.right, rc2.bottom, FALSE);
    break;

  case WM_COMMAND:
    if(LOWORD(wParam) < 1048)
      EndDialog(hWnd, LOWORD(wParam));
    else
    {
      i = LOWORD(wParam);
      if(i < 1051)
        curIntro = i - 1048;
      else
        curResolution = i - 1051;

      CheckRadioButton(hWnd, 1048, 1048 + 2, 1048 + curIntro);
      CheckRadioButton(hWnd, 1051, 1051 + 2, 1051 + curResolution);
    }
    break;
  }

  return 0;
}
#endif

// new is *required* to return zeroed memory here!

void * __cdecl operator new(size_t sz)
{
#ifdef _DEBUG
  totalAlloc += sz;
  curAlloc += sz;

  if (curAlloc > peakAlloc)
    peakAlloc = curAlloc;
#endif

  return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sz);
}

void __cdecl operator delete(void *ptr)
{
  if (!ptr)
    return;

#ifdef _DEBUG
  curAlloc -= HeapSize(GetProcessHeap(), 0, ptr);
#endif

  HeapFree(GetProcessHeap(), 0, ptr);
}

static void __declspec(noreturn) exitProgram()
{
	ssStop();
	opsCleanup();
	ssClose();
	engineClose();
	fvs::closeDirectX();

#ifdef _DEBUG
	TCHAR buf[128];
	wsprintf(buf, "alloc total %d current %d peak %d\n", totalAlloc, curAlloc, peakAlloc);
	OutputDebugString(buf);
#endif

	ExitProcess(0);
}

static void msgPoll()
{
  MSG msg;

  while (PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE))
  {
    if (GetMessage(&msg, 0, 0, 0))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    else
			exitProgram();
  }
}

static void __stdcall tgPoll(sInt cnt, sInt max)
{
  static sU32 oldt = 0;
  const sU32 curt = GetTickCount();

	if ((sS32) (curt - oldt) >= 200)
  {
    oldt = curt;

    fvs::clear();
    
    fvs::vbSlice vbs(4, sTRUE);
    fvs::stdVertex* v = (fvs::stdVertex*) vbs.lock();
    
    for (sInt i = 0; i < 4; i++)
    {
			v[i].x = (0.0625000f + (i & 1) * cnt * 0.875f / max) * fvs::conf.xRes;
			v[i].y = (0.4583333f + (i & 2) * 0.041667f) * fvs::conf.yRes;
      v[i].z = v[i].rhw = 0.5f;
      v[i].color = 0xffffff;
    }
    
    vbs.unlock();
    fvs::drawPrimitive(D3DPT_TRIANGLESTRIP, vbs.use(), 2);
    fvs::updateScreen();
		msgPoll();
  }
}

static void __stdcall errorReport(const sChar *msg)
{
  ssClose();
  fvs::closeDirectX();

  MessageBox(hViewerWnd, msg, "fvs²", MB_ICONERROR|MB_OK);
  ExitProcess(0);
}

#if !defined(_DEBUG)
void WinMainCRTStartup()
#else
int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#endif
{
#ifdef THREEINTROS
  DialogBoxIndirect(GetModuleHandle(0), (LPDLGTEMPLATE) mainDialog, 0, DialogProc);
#endif

	// some setup first
	__asm
	{
		push	01e3fh;
		fldcw	[esp];
		pop		eax;
		cld;
	}

  const sU8* dataPtr = introData;
  const sInt xRes = ((const sInt*) dataPtr)[0];
  const sInt yRes = ((const sInt*) dataPtr)[1];
  const sU8* loaderTune = ((const sU8**) dataPtr)[2];
  dataPtr += sizeof(sInt) * 3;

	fvs::conf.xRes = xRes;
	fvs::conf.yRes = yRes;
  fvs::conf.triplebuffer = sTRUE;
#ifdef _DEBUG
  fvs::conf.windowed = sTRUE;
#endif

#ifdef _DEBUG
  RECT rc = {0, 0, fvs::conf.xRes, fvs::conf.yRes};
  AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

  hViewerWnd = CreateWindowEx(0, "STATIC", "fvs²", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0,
    rc.right - rc.left, rc.bottom - rc.top, 0, 0, GetModuleHandle(0), 0);
#else
	hViewerWnd = CreateWindowEx(0, "STATIC", "fvs²", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0,
		100, 100, 0, 0, GetModuleHandle(0), 0);
#endif

  SetWindowLong(hViewerWnd, GWL_WNDPROC, (LONG) WndProc);
  ShowWindow(hViewerWnd, SW_SHOW);
  
  fr::setReportError(errorReport);
  fvs::startupDirectX(hViewerWnd);

  engineInit();

  if (loaderTune)
	{
		ssInit((sU8 *) loaderTune, hViewerWnd);
		ssPlay();
	}

  // perform precalc
  const sU8* viruzPtr = opsInitialize(dataPtr);
  opsCalculate(tgPoll);

  if (loaderTune)
    ssClose();

  ssInit((sU8*) viruzPtr, hViewerWnd);
  ssPlay();

  Sleep(10);
  while (ssGetTime() < 0);

  sInt oldTime = 0;
#ifdef _DEBUG
  sBool faster = sFALSE;
#endif

  while (1)
  {
    sInt time = ssGetTime() / 8; // exported timing is in 1/125s, not ms.
#ifdef _DEBUG
    if (GetAsyncKeyState(VK_NEXT) & 1)
      faster = sTRUE;

    if (faster)
      time *= 5;
#endif

    if (!opsAnimate(-1, oldTime, time))
		{
#ifdef LOOP
			// loop mode
			ssClose();
			opsCleanup();

			if (loaderTune)
			{
				ssInit((sU8*) loaderTune, hViewerWnd);
				ssPlay();
			}
			
			opsInitialize(dataPtr);
			opsCalculate(tgPoll);
			
			if (loaderTune)
				ssClose();

			ssInit((sU8*) viruzPtr, hViewerWnd);
			ssPlay();
			Sleep(10);
			while (ssGetTime() < 0);
			oldTime = 0;
			continue;
#else
			break;
#endif
		}

    cmpPaintFrame();
    ssDoTick();
    fvs::updateScreen();
		msgPoll();

    oldTime = time;
  }

	exitProgram();
}

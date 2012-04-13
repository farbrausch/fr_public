// This file is distributed under a BSD license. See LICENSE.txt for details.


#include "start_mobile.hpp"

#define UNICODE 1
#include <windows.h>
#include <stdarg.h>

#if sPLATFORM==sPLAT_PDA
#include <gx.h>
#pragma comment(lib,"gx.lib")
#else
#include <crtdbg.h>
#include <malloc.h>
#pragma comment(lib,"winmm.lib")
#endif



/****************************************************************************/
/****************************************************************************/

static sU16 *Buffer;
static sInt ScreenX,ScreenY,ScreenBPR;
static sInt ProgressMax,ProgressCount,ProgressLastPixel,ProgressLastTime;
static HWND Window;
static sInt Initialized = 0;
static sInt PrintLine;
static sDInt MemoryUsedCount;
static sInt WKeepRunning = 0;
static sChar CmdLine[2048];
#define MAX_CMDPARA 16
static const sChar *CmdPara[MAX_CMDPARA];
HDC hdc;

/****************************************************************************/

#if sPLATFORM!=sPLAT_PDA
#undef new

void * __cdecl operator new(unsigned int size)
{
  void *p;

#if !sRELEASE
  p = _malloc_dbg(size,_NORMAL_BLOCK,"unknown",1);
#else
  p = malloc(size);
#endif

  if(p==0)
  {
    sTerminate(L"ran out of virtual memory...\n");
  }
  else
  {
    MemoryUsedCount+=_msize(p); 
//    sDPrintF(L"%08x %2d.%03dMB ( +%08x = %08x )\n",MemoryUsedCount,MemoryUsedCount/1024/1024,MemoryUsedCount/1024%1024,_msize(p),size);
  }
  return p;
}

void * __cdecl operator new(unsigned int size,const char *file,int line)
{
  void *p;

#if !sRELEASE
  p = _malloc_dbg(size,_NORMAL_BLOCK,file,line);
#else
  p = malloc(size);
#endif

  if(p==0)
  {
    sTerminate(L"ran out of virtual memory...\n");
  }
  else
  {
    MemoryUsedCount+=_msize(p); 
//    sDPrintF(L"%08x %2d.%03dMB ( +%08x = %08x )\n",MemoryUsedCount,MemoryUsedCount/1024/1024,MemoryUsedCount/1024%1024,_msize(p),size);
  }

  return p;
}


void __cdecl operator delete(void *p)
{
	if(p)
	{
		MemoryUsedCount-=_msize(p);
//    sDPrintF(L"%08x %2d.%03dMB ( -%08x )\n",MemoryUsedCount,MemoryUsedCount/1024/1024,MemoryUsedCount/1024%1024,_msize(p));
    free(p);
	}
}

#define new new(__FILE__,__LINE__)
#endif


#if sPLATFORM==sPLAT_PDA

static GXDisplayProperties Display;

#else

#define SCREENX 480
#define SCREENY 640

static sU16 ScreenBuffer[SCREENX*SCREENY];
static sU32 ScreenBuffer32[SCREENX*SCREENY];

void *GXBeginDraw()
{
  ScreenX = SCREENX;
  ScreenY = SCREENY;
  ScreenBPR = SCREENX;
  return ScreenBuffer;
}

void GXEndDraw()
{
  HDC dc = GetDC(Window);
  sU32 col;
  BITMAPINFO bmi;

  for(sInt i=0;i<SCREENX*SCREENY;i++)
  {
    col = ScreenBuffer[i];
    col = ((col&0xf800)<<8)|((col&0x07e0)<<5)|((col&0x001f)<<3)|0xff000000;
    ScreenBuffer32[i] = col;
  }

  memset(&bmi,0,sizeof(bmi));
  bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
  bmi.bmiHeader.biWidth = SCREENX;
  bmi.bmiHeader.biHeight = -SCREENY;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biSizeImage = SCREENX*SCREENY*4;

  SetDIBitsToDevice(dc,0,0,SCREENX,SCREENY,0,0,0,SCREENY,ScreenBuffer32,&bmi,0);

  ReleaseDC(Window,dc);
}

#endif
 
/****************************************************************************/

void Paint()
{
  Buffer=(sU16 *)GXBeginDraw();
  if(Buffer)
  {
    sAppOnPaint(ScreenX,ScreenY,Buffer);
    GXEndDraw();
  }

}

void sDrawRect(sInt x0,sInt y0,sInt x1,sInt y1,sU16 col)
{
  sU16 *d = Buffer+x0+y0*ScreenBPR;
  for(sInt y=0;y<y1-y0;y++)
  {
    for(sInt x=0;x<x1-x0;x++)
      d[x] = col;
    d+=ScreenBPR;
  }
}

void sText(const sChar *format,...)
{
  sChar buffer[1024];
  RECT r;

  sFormatString(buffer,sCOUNTOF(buffer),format,&format);
 
  r.left = 4;
  r.right = ScreenX;
  r.top = PrintLine;
  r.bottom = PrintLine+20;
  DrawText(hdc,buffer,sGetStringLen(buffer),&r,DT_SINGLELINE|DT_VCENTER|DT_LEFT);

  PrintLine+=10;
}

/****************************************************************************/
/*
void sDPrintF(sChar *format,...)
{
  static TCHAR buffer[1024];
  va_list va;

  va_start(va,format);
  wvsprintf(buffer,format,va);
  va_end(va);
  OutputDebugString(buffer);
}
*/
/****************************************************************************/

void sProgressStart(sInt max)
{
  ProgressMax = max;
  ProgressCount = 0;
  sU16 col0 = 0;
  sU16 col1 = ~0;
  ProgressLastPixel = 12+ProgressCount*(ScreenX-24)/ProgressMax;
  ProgressLastTime = sGetTime();

  Buffer=(sU16 *)GXBeginDraw();
  if(Buffer)
  {
    sDrawRect(0,0,ScreenX,ScreenY,col0);
    sDrawRect(8,ScreenY/2-12,ScreenX-8,ScreenY/2+12,col1);
    sDrawRect(10,ScreenY/2-10,ScreenX-10,ScreenY/2+10,col0);
    GXEndDraw();
  }
}

void sProgress(sInt count)
{
  sU16 col0 = 0;
  sU16 col1 = ~0;
  sInt x0,x1;
  sInt w = ScreenX-24;

  // calc bar extends

  x0 = ProgressLastPixel;
  ProgressCount += count;
  if(ProgressCount>ProgressMax)
    ProgressCount = 0;

  x1 = 12+ProgressCount*w/ProgressMax;

  // draw bar if it has changed

  if(x0!=x1)
  {
#if sPLATFORM!=sPLAT_PDA
    sInt time = sGetTime();
    if(time < ProgressLastTime+500)
      return;
    ProgressLastTime = time;
#endif
    Buffer=(sU16 *)GXBeginDraw();
    if(Buffer)
    {
      sDrawRect(x0,ScreenY/2-8,x1,ScreenY/2+8,col1);
      GXEndDraw();
    }
  }
}

void sProgressEnd()
{
  sDPrintF(L"progres: %d\n",ProgressCount);
//  Sleep(500);
}

sInt sProgressGet()
{
  return ProgressCount;
}

/****************************************************************************/

void sExit()
{
  PostMessage(Window,WM_CLOSE,0,0);
}

void sTerminate(sChar *message)
{
#if sPLATFORM==sPLAT_PDA
  GXCloseDisplay();
  GXCloseInput();
#else
  _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)&~(_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF));
#endif
  MessageBox(Window,message,L"werkkzeug mobile",MB_OK);
  TerminateProcess(0,0);
}

void sOutputDebugString(sChar *buffer)
{
  OutputDebugString(buffer);
}

sInt sGetTime()
{
#if sPLATFORM==sPLAT_PDA
  return GetTickCount();
#else
  return timeGetTime();
#endif
}

/*
void sFatal(sChar *message)
{
#if sPLATFORM==sPLAT_PDA
  GXCloseDisplay();
  GXCloseInput();
#endif
  MessageBox(Window,message,L"werkkzeug mobile",MB_OK);
  TerminateProcess(0,0);
}
*/

const sChar *sGetShellParameter(sInt i)
{
  sVERIFY(i>=0 && i<MAX_CMDPARA)
  return CmdPara[i];
}

sU8 *sLoadFile(const sChar *name)
{
  sInt result;
  HANDLE handle;
  DWORD test;
  sU8 *mem;
  sInt size;
   
  mem = 0;
  result = sFALSE;
  handle = CreateFile(name,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
  if(handle != INVALID_HANDLE_VALUE)
  {
    size = GetFileSize(handle,&test);
    if(test==0)
    {
      mem = new sU8[size];
      if(ReadFile(handle,mem,size,&test,0))
        result = sTRUE;
      if(size!=(sInt)test)
        result = sFALSE;
    }
    CloseHandle(handle);
  }


  if(!result)
  {
    if(mem)
      delete[] mem;
    mem = 0;
  }

  return mem;
}

sInt sLoadDirCE(const sChar *pattern,sDirEntryCE *buffer,sInt max)
{
  WIN32_FIND_DATA fd;
  HANDLE dir;
  sInt ok;
  sInt count;


  memset(&fd,0,sizeof(fd));
  
  dir = FindFirstFile(pattern,&fd);
  if(!dir)
    return 0;

  ok = 1;
  count = 0;
  while(ok && count<max)
  {
    if(fd.dwFileAttributes!=FILE_ATTRIBUTE_DIRECTORY)
    {
      buffer[count].Size = fd.nFileSizeLow;
      sCopyString(buffer[count].Name,fd.cFileName,sCOUNTOF(buffer[count].Name));
      count++;
    }
    ok = FindNextFile(dir,&fd);
  }

  return count;
}

/****************************************************************************/
/****************************************************************************/

LRESULT CALLBACK WinProc(HWND win,UINT msg,WPARAM wp,LPARAM lp)
{
  static sInt key;
  static sInt mx;
  static sInt my;
  static sInt mb; 

//  TCHAR buffer[256];
//  wsprintf(buffer,L"%08x %08x %08x\n",msg,wp,lp);
//  OutputDebugString(buffer);

  switch(msg)
  {
  case WM_KILLFOCUS:
    PostMessage(win,WM_CLOSE,0,0);
    break;

  case WM_LBUTTONDOWN:
    mb = 1;
    mx = LOWORD(lp);
    my = HIWORD(lp);
    if(Initialized)
    {
      sAppOnMouse(sMDD_START,mx,my);
      InvalidateRect(win,0,0);
    }
    break;

  case WM_LBUTTONUP:
    mb = 0;
    mx = LOWORD(lp);
    my = HIWORD(lp);
    if(Initialized)
    {
      sAppOnMouse(sMDD_STOP,mx,my);
      InvalidateRect(win,0,0);
    }
	  break;

  case WM_MOUSEMOVE:
    mx = LOWORD(lp);
    my = HIWORD(lp);
    if(Initialized && mb)
    {
      sAppOnMouse(sMDD_DRAG,mx,my);
      InvalidateRect(win,0,0);
    }
    break;

  case WM_KEYDOWN:
//    sDPrintF(L"key %04x\n",wp);

         if(wp=='1') wp=sMKEY_APP0;
    else if(wp=='2') wp=sMKEY_APP1;
    else if(wp=='3') wp=sMKEY_APP2;
    else if(wp=='4') wp=sMKEY_APP3;
    else if(wp==VK_ESCAPE) PostMessage(win,WM_CLOSE,0,0);

    if(wp!=0x84 && Initialized)
    {
      sAppOnKey((sInt)wp);
      InvalidateRect(win,0,0);
    }      
    break;

  case WM_KEYUP:
         if(wp=='1') wp=sMKEY_APP0;
    else if(wp=='2') wp=sMKEY_APP1;
    else if(wp=='3') wp=sMKEY_APP2;
    else if(wp=='4') wp=sMKEY_APP3;

    if(wp!=0x84 && Initialized)
    {
      sAppOnKey(((sInt)wp)|sMKEYQ_BREAK);
      InvalidateRect(win,0,0);
    }
    break;

  case WM_CHAR:
    break;

  case WM_PAINT:
    if(Initialized)
    {
      PAINTSTRUCT ps;
      Paint();

      if(!WKeepRunning)
      {
        hdc = BeginPaint(win,&ps);
        SetBkMode(hdc,TRANSPARENT);
        SetTextColor(hdc,RGB(0,255,0));
        PrintLine = 0;
        sAppOnPrint();
        EndPaint(win,&ps);
      }
      WKeepRunning = 0;
    }
    break;
    break;

  case WM_CREATE:
    break;

  case WM_DESTROY:
    if(Initialized)
      sAppExit();
#if sPLATFORM==sPLAT_PDA
    GXCloseDisplay();
    GXCloseInput();
#endif
    PostQuitMessage(0);
    break;
  default:
    return DefWindowProc(win,msg,wp,lp);
    break;
  }
  return 0;
}

#if sPLATFORM==sPLAT_PDA
int WINAPI WinMain(HINSTANCE inst,HINSTANCE prev,LPTSTR cmd,int show)
#else
int WINAPI WinMain(HINSTANCE inst,HINSTANCE prev,char *cmd,int show)
#endif
{
  WNDCLASS wc;
  MSG msg;
  sInt i;
  sChar *s,*ende;
  
  // init mem debugging

#if sPLATFORM!=sPLAT_PDA
  _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)|_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF);
  sChar *test = new sChar[16];
  sCopyMem(test,"TestLeak",9);
  timeBeginPeriod(1);
#endif

  // init types

  sInitTypes();

  
  // parse CMDLINE
  // the trick is that this code works with 7 and 16 bit chars as input.
  // 7 bit - thats no umlauts vor WinXP!

  for(i=0;i<MAX_CMDPARA;i++)
    CmdPara[i] = 0;

  s = CmdLine;
  ende = CmdLine + sCOUNTOF(CmdLine) - 2;
  i = 0;
  while(*cmd && i<MAX_CMDPARA && s<ende)
  {
    while(*cmd==' ' || *cmd=='\n' || *cmd=='\t' || *cmd=='\r')
      cmd++;
    if(*cmd==0) 
      break;

    CmdPara[i++] = s;
    if(*cmd=='"')
    {
      cmd++;
      while(*cmd!=0 && *cmd!='"' && *cmd!=' ' && *cmd!='\n' && *cmd!='\t' && *cmd!='\r' && s<ende)
      {
        *s++ = *cmd++;
      }
      if(*cmd=='"') 
        cmd++;
    }
    else
    {
      while(*cmd!=0 && *cmd!=' ' && *cmd!='\n' && *cmd!='\t' && *cmd!='\r' && s<ende)
      {
        *s++ = *cmd++;
      }
    }
    *s++ = 0;
  }

  // make windiw

  memset(&wc,0,sizeof(wc));
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.hInstance = inst;
  wc.hbrBackground = (HBRUSH) GetStockObject(NULL_BRUSH);
  wc.lpszClassName = L".theprodukkt";
  wc.lpfnWndProc = WinProc;

  RegisterClass(&wc);

#if sPLATFORM!=sPLAT_PDA

  const sInt xs = SCREENX;
  const sInt ys = SCREENY;
  Window = CreateWindow(L".theprodukkt",L".theprodukkt",WS_VISIBLE,
    0,0,xs,ys,
    0,0,inst,0);

  RECT r;
  GetWindowRect(Window,&r);
  SetWindowPos(Window,0,0,0,2*xs-(r.right-r.left),2*ys-(r.bottom-r.top),0);
  GetWindowRect(Window,&r);

  ShowWindow(Window,show);
  UpdateWindow(Window);

#else
  Window = CreateWindow(L".theprodukkt",L".theprodukkt",WS_VISIBLE,
    0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),
    0,0,inst,0);
  
  ShowWindow(Window,show);
  UpdateWindow(Window);

  if(!GXOpenDisplay(Window,GX_FULLSCREEN))
    sFatal(L"gx required");

  Display = GXGetDisplayProperties();
  if(Display.cBPP!=16) 
    sFatal(L"16 bits per pixel");
  ScreenX = Display.cxWidth;
  ScreenY = Display.cyHeight;
  ScreenBPR = Display.cxWidth;
  
  GXOpenInput();
#endif

  sAppInit();
  Initialized = 1;
  
  while(GetMessage(&msg,0,0,0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

#if sPLATFORM!=sPLAT_PDA
  timeEndPeriod(1);
#endif

  return (sInt)msg.wParam;
}

void sKeepRunning()
{
  WKeepRunning = 1;
}

/****************************************************************************/
/***                                                                      ***/
/***   Font System                                                        ***/
/***                                                                      ***/
/****************************************************************************/

BITMAPINFO FontBMI;
HBITMAP FontHBM;
HFONT FontHandle;
sU32 *FontMem;
HDC FontDC;

sInt sFontBegin(sInt pagex,sInt pagey,const sChar *name,sInt xs,sInt ys,sInt style)
{
  LOGFONT lf;
  TEXTMETRIC met;

  FontDC = CreateCompatibleDC(0);

  FontBMI.bmiHeader.biSize = sizeof(FontBMI.bmiHeader);
  FontBMI.bmiHeader.biWidth = pagex;
  FontBMI.bmiHeader.biHeight = -pagey;
  FontBMI.bmiHeader.biPlanes = 1;
  FontBMI.bmiHeader.biBitCount = 32;
  FontBMI.bmiHeader.biCompression = BI_RGB;
  FontHBM = CreateDIBSection(FontDC,&FontBMI,DIB_RGB_COLORS,
    (void **) &FontMem,0,0);
  SelectObject(FontDC,FontHBM);

  if(name[0]==0) name = L"arial";

  lf.lfHeight         = -ys; 
  lf.lfWidth          = xs; 
  lf.lfEscapement     = 0; 
  lf.lfOrientation    = 0; 
  lf.lfWeight         = (style&2) ? FW_BOLD : FW_NORMAL; 
  lf.lfItalic         = (style&1) ? 1 : 0; 
  lf.lfUnderline      = 0; 
  lf.lfStrikeOut      = 0; 
  lf.lfCharSet        = DEFAULT_CHARSET; 
  lf.lfOutPrecision   = OUT_DEFAULT_PRECIS; 
  lf.lfClipPrecision  = CLIP_DEFAULT_PRECIS; 
  lf.lfQuality        = (style&4) ? ANTIALIASED_QUALITY : NONANTIALIASED_QUALITY;
  lf.lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;
  sCopyMem(lf.lfFaceName,name,sizeof(lf.lfFaceName));

  FontHandle = CreateFontIndirect(&lf);
/*
  FontHandle = CreateFont(-ys,xs,0,0,(style&2) ? FW_BOLD : FW_NORMAL,
    (style&1) ? 1 : 0,0,0,DEFAULT_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,
    PROOF_QUALITY,
    //(style&4) ? ANTIALIASED_QUALITY : NONANTIALIASED_QUALITY,
    DEFAULT_PITCH|FF_DONTCARE,name);
*/
  SetBkMode(FontDC,TRANSPARENT);
//  SelectObject(GDIScreenDC,FontHandle);    // win98: first select font into screen dc
  SelectObject(FontDC,FontHandle);
  SelectObject(FontDC,FontHBM);
  SetTextColor(FontDC,0xffffff);

  GetTextMetrics(FontDC,&met);

  return met.tmHeight;
//  Advance = met.tmHeight+met.tmExternalLeading;
//  Baseline = met.tmHeight-met.tmDescent;
}

void sFontPrint(sInt x,sInt y,const sChar *string,sInt len)
{
  if(len==-1)
  {
    len++;
    while(string[len]) len++;
  }
  ExtTextOut(FontDC,x,y,0,0,string,len,0);
}

sU32 *sFontBitmap() 
{ 
  return FontMem; 
}

void sFontEnd()
{
  DeleteObject(FontHBM);
  DeleteObject(FontDC);
}

/****************************************************************************/

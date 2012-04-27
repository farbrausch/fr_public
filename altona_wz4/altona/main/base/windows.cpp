/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/
#include "base/types.hpp"
#include "base/system.hpp"
#include "base/windows.hpp"

#if sCONFIG_SYSTEM_WINDOWS

#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#pragma comment(lib,"shell32.lib")

extern HDC sGDIDC;
extern HDC sGDIDCOffscreen;
extern HWND sHWND;

namespace sWin32
{
  sBool ModalDialogActive = sFALSE;
}

#if sRENDERER==sRENDER_DX11
HDC DXGIGetDC();
void DXGIReleaseDC();
#endif

struct sImage2DPrivate
{
  HBITMAP DDB;
  HBITMAP OriginalBitmap;
  HDC DC;
  sInt SizeX;
  sInt SizeY;
};

/****************************************************************************/

void sGetAppDataDir(const sStringDesc &str)
{
  sChar path[sMAXPATH];
  HRESULT result = SHGetFolderPath(NULL,CSIDL_APPDATA|CSIDL_FLAG_CREATE,NULL,0,path);
  sVERIFY(result==S_OK);
  sSPrintF(str,L"%p/",path);
}

/****************************************************************************/
/***                                                                      ***/
/***   GDI                                                                ***/
/***                                                                      ***/
/****************************************************************************/


/****************************************************************************/

#define MAX_BRUSHES 32
#define MAX_CLIPS   64

static HCURSOR Cursors[sMP_MAX];
static sU32 BrushColor[MAX_BRUSHES];
static HBRUSH BrushHandle[MAX_BRUSHES];
static HPEN BrushPen[MAX_BRUSHES];
static HRGN ClipStack[MAX_CLIPS];
static sInt ClipStackResult[MAX_CLIPS];
static sInt ClipIndex;
static sString<256> WindowName;
static sString<2048> DragDropFilename;
static sU32 ClipboardFormat;

inline sU32 GDICOL(sU32 col) { return ((col&0xff0000)>>16) | (col&0x00ff00) | ((col&0x0000ff)<<16); }

void InitGDI()
{
  ClipboardFormat = RegisterClipboardFormatW(L"Altona Clipboard Object");
  if(ClipboardFormat==0)
    sFatal(L"failed to register clipboard format");

//  if(sGetSystemFlags() & sISF_2D)
  {
    sU32 color = GDICOL(0);
    for(sInt i=0;i<MAX_BRUSHES;i++)
    {
      BrushColor[i] = color;
      BrushHandle[i] = CreateSolidBrush(color);
      BrushPen[i] = CreatePen(PS_SOLID,0,color);
    }
    for(sInt i=0;i<MAX_CLIPS;i++)
    {
      ClipStack[i] = CreateRectRgn(0,0,0,0);
    }

    static const LPCWSTR cursormap[] =
    {
      0,
      IDC_ARROW,
      IDC_WAIT,
      IDC_CROSS,
      IDC_HAND,
      IDC_IBEAM,
      IDC_NO,
      IDC_SIZEALL,
      IDC_SIZENS,
      IDC_SIZEWE,
      IDC_SIZENWSE,
      IDC_SIZENESW,
    };
    Cursors[0] = 0;
    for(sInt i=1;i<sMP_MAX;i++)
      Cursors[i] = LoadCursorW(0,cursormap[i]);
  }
}

void ExitGDI()
{
  if(sGetSystemFlags() & sISF_2D)
  {
    for(sInt i=0;i<MAX_BRUSHES;i++)
    {
      DeleteObject(BrushHandle[i]);
      DeleteObject(BrushPen[i]);
    }
    for(sInt i=0;i<MAX_CLIPS;i++)
    {
      DeleteObject(ClipStack[i]);
    }
  }
}

sADDSUBSYSTEM(GDI,0x88,InitGDI,ExitGDI);

/****************************************************************************/
/***                                                                      ***/
/***   Window handling                                                    ***/
/***                                                                      ***/
/****************************************************************************/


void sUpdateWindow()
{
  InvalidateRect(sHWND,0,0);
}
void sUpdateWindow(const sRect &r)
{
  InvalidateRect(sHWND,(CONST RECT *)&r,0);
}

void sSetMousePointer(sInt code)
{
  static sInt oldcursor = 0xaaaaaaaa;
  sVERIFY(code>=0 && code<sMP_MAX);
  if(Cursors[code])                         // when GDI is not initialized, we can not set the cursor, but we still can enable/disable it!
    SetCursor(Cursors[code]);
  if(code==0 && oldcursor!=1)
    ShowCursor(0);
  if(code!=0 && oldcursor==0)
    ShowCursor(1);
  oldcursor = code;
}

const sChar *sGetWindowName()
{
  if(WindowName.IsEmpty())
    return L"altona application";
  else
    return WindowName;
}

void sSetWindowName(const sChar *name)
{
  if(WindowName!=name)
  {
    WindowName = name;
    if (sHWND) SetWindowTextW(sHWND,name);
  }
}

void sSetWindowMode(sInt mode)
{
  switch(mode)
  {
  case sWM_NORMAL:
    ShowWindow(sHWND,SW_RESTORE);
    break;
  case sWM_MAXIMIZED:
    ShowWindow(sHWND,SW_MAXIMIZE);
    break;
  case sWM_MINIMIZED:
    ShowWindow(sHWND,SW_MINIMIZE);
    break;
  }
}

void sSetWindowSize(sInt xs,sInt ys)
{
  RECT r2;
  r2.left = r2.top = 0;
  r2.right = xs; 
  r2.bottom = ys;
  AdjustWindowRect(&r2,WS_OVERLAPPEDWINDOW,FALSE);
  SetWindowPos(sHWND,HWND_NOTOPMOST,0,0,r2.right-r2.left,r2.bottom-r2.top,SWP_NOMOVE|SWP_NOZORDER);
}

sInt sGetWindowMode()
{
  WINDOWPLACEMENT wp;
  if(GetWindowPlacement(sHWND,&wp))
  {
    switch(wp.showCmd)
    {
    case SW_SHOWMAXIMIZED:
      return sWM_MAXIMIZED;

    case SW_SHOWMINIMIZED:
      return sWM_MINIMIZED;
    }
  }
  return sWM_NORMAL;
}

void sGetWindowPos(sInt &x,sInt &y)
{
  RECT r;
  GetWindowRect(sHWND, &r);
  
  x = r.left;
  y = r.top;
}

void sGetWindowSize(sInt &sx,sInt &sy)
{
  RECT r;
  GetWindowRect(sHWND, &r);
  
  sx = r.right-r.left;
  sy = r.bottom-r.top;
}


void sSetWindowPos(sInt x,sInt y)
{
  RECT r;
  GetWindowRect(sHWND, &r);
  SetWindowPos(sHWND,HWND_NOTOPMOST,x,y,r.right-r.left,r.bottom-r.top,SWP_NOZORDER);
}

sBool sHasWindowFocus()
{
#if sCONFIG_OPTION_NPP
  return sTRUE;
#else
  return GetForegroundWindow()==sHWND;
#endif
}

void sSetClipboard(const sChar *text,sInt len)
{
  OpenClipboard(sHWND);
  EmptyClipboard();

  if(len==-1)
    len = sGetStringLen(text);
  sInt size = len+1;

  for(sInt i=0;text[i];i++)
    if(text[i]=='\n')
      size++;

  HANDLE hmem = GlobalAlloc(GMEM_MOVEABLE,size*2);

  sChar *d = (sChar *) GlobalLock(hmem);
  for(sInt i=0;i<len;i++)
  {
    if(text[i]=='\n')
      *d++ = '\r';
    *d++ = text[i];
  }
  *d++ = 0;
  GlobalUnlock(hmem);

  SetClipboardData(CF_UNICODETEXT,hmem);
  CloseClipboard();
}

sChar *sGetClipboard()
{
  sChar *result=0;

  OpenClipboard(sHWND);

  HANDLE hmem = GetClipboardData(CF_UNICODETEXT);
  if(hmem)
  {
    sChar *s = (sChar *)GlobalLock(hmem);
    sInt size = sGetStringLen(s)+1;
    result = new sChar[size];
    sInt i = 0;

    while(*s)
    {
      if(*s!='\r')
        result[i++] = *s;
      s++;
    }
    result[i++] = 0;

    GlobalUnlock(hmem);
  }
  else
  {
    result = new sChar[1];
    result[0] = 0;
  }

  CloseClipboard();

  return result;
}

void sSetClipboard(const sU8 *data,sDInt size,sU32 serid,sInt sermode)
{
  OpenClipboard(sHWND);
  EmptyClipboard();

  HANDLE hmem = GlobalAlloc(GMEM_MOVEABLE,size+sizeof(sU64)*3);

  sU8 *d = (sU8 *) GlobalLock(hmem);
  ((sU64 *)d)[0] = size;
  ((sU64 *)d)[1] = serid;
  ((sS64 *)d)[2] = sermode;
  sCopyMem(d+sizeof(sU64)*3,data,size);

  GlobalUnlock(hmem);
  SetClipboardData(ClipboardFormat,hmem);
  CloseClipboard();
}

sU8 *sGetClipboard(sDInt &size,sU32 serid,sInt sermode)
{
  sU8 *data = 0;
  size = 0;

  OpenClipboard(sHWND);

  HANDLE hmem = GetClipboardData(ClipboardFormat);
  if(hmem)
  {
    sU8 *d = (sU8 *) GlobalLock(hmem);
    size = ((sU64 *)d)[0];
    if(((sU64 *)d)[1] == serid && ((sS64 *)d)[2] == sermode)
    {
      data = new sU8[size];
      sCopyMem(data,d+sizeof(sU64)*3,size);
    }

    GlobalUnlock(hmem);
  }

  CloseClipboard();

  return data;
}


void sEnableFileDrop(sBool enable)
{
  DragAcceptFiles(sHWND,enable);
}

// internal usage only
void sSetDragDropFile(const sChar *name)
{
  DragDropFilename = name;
}

const sChar *sGetDragDropFile()
{
  return DragDropFilename;
}

/****************************************************************************/

sBool sSystemOpenFileDialog(const sChar *label,const sChar *extensions,sInt flags,const sStringDesc &buffer)
{
  sChar oldpath[2048];
  OPENFILENAMEW ofn;
  sInt result=0;

  // determine default extension (=first in list)
  sString<256> defaultExt;
  defaultExt = extensions;
  sInt pipePos;
  if((pipePos = sFindFirstChar(defaultExt,'|')) != -1)
    defaultExt[pipePos] = 0;

  sChar ext[2048];
  sClear(ext);
  sChar *extp = ext;
  sInt filterindex = 0;

  if((flags & 3) == sSOF_LOAD && sFindChar(extensions,'|')>=0) // opening, more than one extension specified?
  {
    filterindex = 1;
    static const sChar allSupported[] = L"All supported extensions";
    sCopyMem(extp,allSupported,sizeof(allSupported));
    extp += sCOUNTOF(allSupported);

    // add all supported extensions
    const sChar *curExt = extensions;
    sBool first = sTRUE;

    for(;;)
    {
      while(*curExt=='|') curExt++;
      if(!*curExt) break;

      if(!first)
        *extp++ = ';';

      *extp++ = '*';
      *extp++ = '.';
      while(*curExt!='|' && *curExt)
        *extp++ = *curExt++;

      first = sFALSE;
    }

    *extp++ = 0;
  }

  for(;;)
  {
    while(*extensions=='|') extensions++;
    if(!*extensions) break;
    const sChar *ext1 = extensions;
    while(*ext1!='|' && *ext1)
      *extp++ = *ext1++;
    *extp++ = 0;
    *extp++ = '*';
    *extp++ = '.';
    while(*extensions!='|' && *extensions)
      *extp++ = *extensions++;
    *extp++ = 0;
  }

  static const sChar allFiles[] = L"All files (*.*)\0*.*\0";
  sCopyMem(extp,allFiles,sizeof(allFiles));
  extp += sCOUNTOF(allFiles);

  sSetMem(&ofn,0,sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = sHWND;
  ofn.lpstrFile = buffer.Buffer;
  ofn.nMaxFile = buffer.Size;
  ofn.lpstrFilter = ext;
  ofn.nFilterIndex = filterindex;
  ofn.lpstrTitle = label;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  if(0)     // this stuff does not work in windows any more. thanks to microsoft being so very intuitive
  {
    sString<sMAXPATH> initialdir;
    sGetCurrentDir(initialdir);
    initialdir.AddPath(buffer.Buffer);
    sChar *lastslash = 0;
    for(sInt i=0;initialdir[i];i++)
      if(initialdir[i]=='/' || initialdir[i]=='\\')
        lastslash = &initialdir[i];
    if(lastslash)
      *lastslash = 0;
    if(buffer.Buffer[0]!=0)
      ofn.lpstrInitialDir = initialdir;
  }

  if(!defaultExt.IsEmpty())
    ofn.lpstrDefExt = defaultExt;

  for(sInt i=0;buffer.Buffer[i];i++) if(buffer.Buffer[i]=='/') buffer.Buffer[i]='\\';
  sInt len = sGetStringLen(buffer.Buffer);
  if(len>0 && buffer.Buffer[len-1]=='\\') buffer.Buffer[len-1]=0;

  GetCurrentDirectoryW(sCOUNTOF(oldpath),oldpath);
  sWin32::ModalDialogActive = sTRUE;

  switch(flags & 3)
  {
  case sSOF_LOAD:
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    if (flags&sSOF_MULTISELECT) ofn.Flags|=OFN_ALLOWMULTISELECT;
    result = GetOpenFileNameW(&ofn);
    break;
  case sSOF_SAVE:
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_EXPLORER;
    result = GetSaveFileNameW(&ofn);
    break;
  case sSOF_DIR:
    { CoInitializeEx(0L,COINIT_APARTMENTTHREADED);
      BROWSEINFO bi;
      memset(&bi, 0, sizeof(bi));
      bi.ulFlags   = BIF_USENEWUI;
      bi.hwndOwner = GetDesktopWindow();
      bi.lpszTitle = label;

      SetActiveWindow(GetDesktopWindow());
      UpdateWindow(GetDesktopWindow());

      LPITEMIDLIST pIDL;
      pIDL = SHBrowseForFolderW(&bi);
      if(pIDL!=0L)
      { // Create a buffer to store the path, then 
        // get the path.
        sChar buffer2[_MAX_PATH] = L"\0";
        if(SHGetPathFromIDList(pIDL, buffer2) != 0)
        {
          // Set the string value.
          sCopyString(buffer,buffer2);
          result = true;
        }
        // free the item id list
        CoTaskMemFree(pIDL);
      }
    }
    break;
  default:
    result = 0;
    break;
  }

  sWin32::ModalDialogActive = sFALSE;
  SetCurrentDirectoryW(oldpath);

  for(sInt i=0;buffer.Buffer[i];i++)
    if(buffer.Buffer[i]=='\\') buffer.Buffer[i]='/';

  return result;
}

/****************************************************************************/

sBool sSystemMessageDialog(const sChar *label,sInt flags)
{
  const sChar *title = sGetWindowName();
  if((flags & 7)==sSMF_ERROR) title = L"error";

  return sSystemMessageDialog(label,flags,title);
}

sBool sSystemMessageDialog(const sChar *label,sInt flags,const sChar *title)
{
  sBool result = 0;
  switch(flags&7)
  {
  case sSMF_ERROR:
    result = MessageBoxW(sHWND,label,title,MB_OK|MB_ICONEXCLAMATION);
    break;
  case sSMF_OK:
    result = MessageBoxW(sHWND,label,title,MB_OK);
    break;
  case sSMF_OKCANCEL:
    result = (MessageBoxW(sHWND,label,title,MB_OKCANCEL)==IDOK);
    break;
  case sSMF_YESNO:
    result = (MessageBoxW(sHWND,label,title,MB_YESNO)==IDYES);
    break;
  }

  return result;
}

/****************************************************************************/
/***                                                                      ***/
/***   Clipping                                                           ***/
/***                                                                      ***/
/****************************************************************************/

void sClipFlush()
{
  sVERIFY(ClipIndex==0);
}

void sClipPush()
{
  sVERIFY(ClipIndex<MAX_CLIPS);
  ClipStackResult[ClipIndex] = GetClipRgn(sGDIDC,ClipStack[ClipIndex]);
  ClipIndex++;
}

void sClipPop()
{
  sVERIFY(ClipIndex>0);
  ClipIndex--;
  SelectClipRgn(sGDIDC,ClipStackResult[ClipIndex] ? ClipStack[ClipIndex] : 0);
}

void sClipExclude(const sRect &r)
{
  ExcludeClipRect(sGDIDC,r.x0,r.y0,r.x1,r.y1);
}

void sClipRect(const sRect &r)
{
  IntersectClipRect(sGDIDC,r.x0,r.y0,r.x1,r.y1);
}

/****************************************************************************/
/***                                                                      ***/
/***   Painting                                                           ***/
/***                                                                      ***/
/****************************************************************************/

void sSetColor2D(sInt colid,sU32 color)
{
  sVERIFY(colid>=0 && colid<MAX_BRUSHES);
  color = GDICOL(color);
  if(BrushColor[colid]!=color)
  {
    DeleteObject(BrushHandle[colid]);
    DeleteObject(BrushPen[colid]);
    BrushColor[colid] = color;
    BrushHandle[colid] = CreateSolidBrush(color);
    BrushPen[colid] = CreatePen(PS_SOLID,0,color);
  }
}

sU32 sGetColor2D(sInt colid)
{
  sVERIFY(colid>=0 && colid<MAX_BRUSHES);
  return GDICOL(BrushColor[colid]);
}

void sRect2D(sInt x0,sInt y0,sInt x1,sInt y1,sInt colid)
{
  sVERIFY(colid>=0 && colid<MAX_BRUSHES);
  if(x0!=x1 && y0!=y1)
  {
    sRect r(x0,y0,x1,y1);
    FillRect(sGDIDC,(const RECT *) &r,BrushHandle[colid]);
  }
}

void sRect2D(const sRect &r,sInt colid)
{
  sVERIFY(colid>=0 && colid<MAX_BRUSHES);
  if(r.SizeX()>0 && r.SizeY()>0)
    FillRect(sGDIDC,(const RECT *) &r,BrushHandle[colid]);
}

void sRectHole2D(const sRect &out,const sRect &hole,sInt colid)
{
  sRect2D(out .x0,out .y0,out .x1,hole.y0,colid);
  sRect2D(out .x0,hole.y0,hole.x0,hole.y1,colid);
  sRect2D(hole.x1,hole.y0,out .x1,hole.y1,colid);
  sRect2D(out .x0,hole.y1,out .x1,out .y1,colid);
}

void sRectInvert2D(sInt x0,sInt y0,sInt x1,sInt y1)
{
  BitBlt(sGDIDC,x0,y0,x1-x0,y1-y0,0,0,0,DSTINVERT);
}

void sRectInvert2D(const sRect &r)
{
  BitBlt(sGDIDC,r.x0,r.y0,r.SizeX(),r.SizeY(),0,0,0,DSTINVERT);
}

void sRectFrame2D(const sRect &r,sInt colid)
{
  sRectFrame2D(r.x0,r.y0,r.x1,r.y1,colid);
}

void sRectFrame2D(sInt x0,sInt y0,sInt x1,sInt y1,sInt colid)
{
  sRect2D(x0  ,y0  ,x1  ,y0+1,colid);
  sRect2D(x0  ,y1-1,x1  ,y1  ,colid);
  sRect2D(x0  ,y0+1,x0+1,y1-1,colid);
  sRect2D(x1-1,y0+1,x1  ,y1-1,colid);
}

void sLine2D(sInt x0,sInt y0,sInt x1,sInt y1,sInt colid)
{
  sVERIFY(colid>=0 && colid<MAX_BRUSHES);
  SelectObject(sGDIDC,BrushPen[colid]);
  MoveToEx(sGDIDC,x0,y0,0);
  LineTo(sGDIDC,x1,y1);
}

void sLine2D(sInt *list,sInt count,sInt colid)
{
  sVERIFY(colid>=0 && colid<MAX_BRUSHES);
  if(count<2) return;
  SelectObject(sGDIDC,BrushPen[colid]);
  MoveToEx(sGDIDC,list[0],list[1],0);
  for(sInt i=1;i<count;i++)
    LineTo(sGDIDC,list[i*2+0],list[i*2+1]);
}

void sLineList2D(sInt *list,sInt count,sInt colid)
{
  sVERIFY(colid>=0 && colid<MAX_BRUSHES);
  SelectObject(sGDIDC,BrushPen[colid]);
  for(sInt i=0;i<count;i++)
  {
    MoveToEx(sGDIDC,list[i*4+0],list[i*4+1],0);
    LineTo(sGDIDC,list[i*4+2],list[i*4+3]);
  }
}

void sBlit2D(const sU32 *data,sInt width,const sRect &dest)
{
  BITMAPINFO bmi;
  sClear(bmi);

  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = width;
  bmi.bmiHeader.biHeight = -dest.SizeY();
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  SetDIBitsToDevice(sGDIDC,dest.x0,dest.y0,dest.SizeX(),dest.SizeY(),0,0,0,dest.SizeY(),data,&bmi,DIB_RGB_COLORS);
}

void sStretch2D(const sU32 *data,sInt width,const sRect &source,const sRect &dest)
{
  BITMAPINFO bmi;
  sClear(bmi);

  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = width;
  bmi.bmiHeader.biHeight = -source.y1;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  SetStretchBltMode(sGDIDC,STRETCH_DELETESCANS);
  StretchDIBits(
    sGDIDC,
    dest.x0,dest.y0,dest.SizeX(),dest.SizeY(),
    source.x0,source.y0,source.SizeX(),source.SizeY(),
    data,&bmi,DIB_RGB_COLORS,SRCCOPY);
}

/****************************************************************************/
/***                                                                      ***/
/***   Offscreen Rendering                                                ***/
/***                                                                      ***/
/****************************************************************************/

static sBool Render2DMode;
static sInt Render2DSizeX;
static sInt Render2DSizeY;
static HDC WMPaintDC;
HBITMAP Render2DBM;

void sRender2DBegin(sInt xs,sInt ys)
{
  HDC screendc;
  sVERIFY(Render2DMode==0);
  sVERIFY(WMPaintDC==0)
  WMPaintDC = sGDIDC;

  Render2DMode = 1;
  Render2DSizeX = xs;
  Render2DSizeY = ys;

  screendc = GetDC(0);
  sGDIDC = CreateCompatibleDC(screendc);
  if(sGDIDC==0) sFatal(L"could not create GDI-DC for bitmap rendering");
  Render2DBM = CreateCompatibleBitmap(screendc,xs,ys);
  if(Render2DBM==0) sFatal(L"could not create GDI-BM for bitmap rendering");
  ReleaseDC(0,screendc);
  SelectObject(sGDIDC,Render2DBM);
}

void sRender2DBegin(sImage2D *img)
{
  sVERIFY(Render2DMode==0);
  sVERIFY(WMPaintDC==0)
  WMPaintDC = sGDIDC;

  Render2DMode = 3;
  Render2DSizeX = img->GetSizeX();
  Render2DSizeY = img->GetSizeY();
  Render2DBM = img->prv->DDB;
  sGDIDC = img->prv->DC;
  SelectObject(sGDIDC,img->prv->DDB);
}

void sRender2DBegin()
{
  sVERIFY(Render2DMode==0);
  sVERIFY(WMPaintDC==0)
  WMPaintDC = sGDIDC;

  Render2DMode = 2;
  Render2DSizeX = 0;
  Render2DSizeY = 0;

#if sRENDERER==sRENDER_DX11
  sGDIDC = DXGIGetDC();
#else
  sGDIDC = GetDC(sHWND);
#endif
}

void sRender2DEnd()
{
  sVERIFY(Render2DMode!=0);
  sVERIFY(sGDIDC);

  if(Render2DMode==1)
  {
    DeleteObject(Render2DBM);
    DeleteDC(sGDIDC);
  }
  if(Render2DMode==2)
  {
#if sRENDERER==sRENDER_DX11
    DXGIReleaseDC();
#else
    ReleaseDC(sHWND,sGDIDC);
#endif
  }
  if(Render2DMode==3)
  {
  }

  Render2DMode = 0;
  sGDIDC = WMPaintDC;
  WMPaintDC = 0;
}

void sRender2DSet(sU32 *data)
{
  BITMAPINFO bmi;

  sVERIFY(Render2DMode==1 || Render2DMode==3);
  sVERIFY(sGDIDC);

  bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
  bmi.bmiHeader.biWidth = Render2DSizeX;
  bmi.bmiHeader.biHeight = -Render2DSizeY;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  SetDIBits(sGDIDC,Render2DBM,0,Render2DSizeY,data,&bmi,DIB_RGB_COLORS);
}

void sRender2DGet(sU32 *data)
{
  BITMAPINFO bmi;

  sVERIFY(Render2DMode==1 || Render2DMode==3);
  sVERIFY(sGDIDC);

  bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
  bmi.bmiHeader.biWidth = Render2DSizeX;
  bmi.bmiHeader.biHeight = -Render2DSizeY;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  GetDIBits(sGDIDC,Render2DBM,0,Render2DSizeY,data,&bmi,DIB_RGB_COLORS);
}


/****************************************************************************/
/***                                                                      ***/
/***   2D Blitting                                                        ***/
/***                                                                      ***/
/****************************************************************************/

sImage2D::sImage2D(sInt xs,sInt ys,sU32 *data)
{
  prv = new sImage2DPrivate;
  prv->SizeX = xs;
  prv->SizeY = ys;

  HDC dc = GetDC(sHWND);
  prv->DC = CreateCompatibleDC(dc);
  prv->DDB = CreateCompatibleBitmap(dc,xs,ys);
  prv->OriginalBitmap = (HBITMAP) SelectObject(prv->DC,prv->DDB);
  if(data)
    Update(data);
  ReleaseDC(sHWND,dc);
}

sImage2D::~sImage2D()
{
  SelectObject(prv->DC,prv->OriginalBitmap);
  DeleteObject(prv->DDB);
  DeleteDC(prv->DC);
  delete prv;
}

sInt sImage2D::GetSizeX()
{
  return prv->SizeX;
}

sInt sImage2D::GetSizeY()
{
  return prv->SizeY;
}

/****************************************************************************/

void sImage2D::Update(sU32 *data)
{
  BITMAPINFO bmi;
  sClear(bmi);
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = prv->SizeX;
  bmi.bmiHeader.biHeight = -prv->SizeY;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  bmi.bmiHeader.biSizeImage = prv->SizeX*prv->SizeY*4;

  SelectObject(prv->DC,prv->OriginalBitmap);
  SetDIBits(prv->DC,prv->DDB,0,prv->SizeY,data,&bmi,DIB_RGB_COLORS);
  SelectObject(prv->DC,prv->DDB);
}

void sImage2D::Paint(sInt x,sInt y)
{
  BitBlt(sGDIDC,x,y,prv->SizeX,prv->SizeY,prv->DC,0,0,SRCCOPY);
}

void sImage2D::Paint(const sRect &source,sInt x,sInt y)
{
  BitBlt(sGDIDC,x,y,source.SizeX(),source.SizeY(),prv->DC,source.x0,source.y0,SRCCOPY);
}

void sImage2D::Stretch(const sRect &source,const sRect &dest)
{
  SetStretchBltMode(sGDIDC,HALFTONE);
  SetBrushOrgEx(sGDIDC,0,0,0);
  StretchBlt(sGDIDC,dest.x0,dest.y0,dest.SizeX(),dest.SizeY(),prv->DC,source.x0,source.y0,source.SizeX(),source.SizeY(),SRCCOPY);
}

/****************************************************************************/
/***                                                                      ***/
/***   Fonts                                                              ***/
/***                                                                      ***/
/****************************************************************************/

static const sInt NUMABC = 256; // cache ABCWidths for first N unicode characters

struct sFont2DPrivate
{
  sInt Height;
  sInt CharHeight;
  sInt Baseline;
  HFONT Font;
  sU32 BackPen;
  sU32 BackColor;
  sU32 TextColor;
  ABC Widths[NUMABC];
};

sFont2D::sFont2D(const sChar *name,sInt size,sInt flags,sInt width)
{
  prv=0;
  Init(name,size,flags,width);
}

sFont2D::~sFont2D()
{
  Exit();
}

void sFont2D::AddResource(const sChar *filename)
{
  AddFontResourceExW(filename, FR_PRIVATE, NULL);
}

void sFont2D::Init(const sChar *name,sInt size,sInt flags,sInt width)
{
  Exit();

  TEXTMETRIC tm;

  prv = new sFont2DPrivate;
  prv->Font=0;
  prv->BackPen = 0;

  if (name && name[0])
  {
    LOGFONTW log;
    sClear(log);
    log.lfHeight = size;
    log.lfWidth = width;
    log.lfCharSet = DEFAULT_CHARSET;
    if(flags & sF2C_BOLD)
      log.lfWeight = FW_BOLD;
    if(flags & sF2C_ITALICS)
      log.lfItalic = 1;
    if(flags & sF2C_UNDERLINE)
      log.lfUnderline = 1;
    if(flags & sF2C_STRIKEOUT)
      log.lfStrikeOut = 1;
    if(flags & sF2C_SYMBOLS)
      log.lfCharSet = SYMBOL_CHARSET;
    if(flags & sF2C_NOCLEARTYPE)
      log.lfQuality = ANTIALIASED_QUALITY;
    sCopyString(log.lfFaceName,name,LF_FACESIZE);
    prv->Font = CreateFontIndirectW(&log);
  }

  if (!prv->Font) // no font found -> get default Windows font
  {
    NONCLIENTMETRICS ncm;
    sClear(ncm);
    ncm.cbSize=sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(ncm),&ncm,0);
    prv->Font = CreateFontIndirectW(&ncm.lfMessageFont);
  }

  prv->BackColor = GDICOL(0xffffff);
  prv->TextColor = GDICOL(0x000000);

  // Get font metrics. must create a DC for this!
  HDC dc = GetDC(sHWND);
  HGDIOBJ oldfont = SelectObject(dc,prv->Font);
  GetTextMetricsW(dc,&tm);
  GetCharABCWidths(dc,0,NUMABC-1,prv->Widths);
  SelectObject(dc,oldfont);
  ReleaseDC(sHWND,dc);

  prv->Baseline = tm.tmAscent;
  prv->Height = tm.tmHeight;
  prv->CharHeight = tm.tmHeight - tm.tmInternalLeading;
}

void sFont2D::Exit()
{
  if (!prv) return;
  DeleteObject(prv->Font);
  sDelete(prv);
}

sInt sFont2D::GetHeight()
{
  return prv->Height;
}

sInt sFont2D::GetBaseline()
{
  return prv->Baseline;
}

sInt sFont2D::GetWidth(const sChar *text,sInt len)
{
  SIZE out;

  if(len==-1) len=sGetStringLen(text);
  if(len==0) return 0;

  SelectObject(sGDIDCOffscreen,prv->Font);
  GetTextExtentPoint32W(sGDIDCOffscreen,text,len,&out);
  sInt width = out.cx;

  if(1)  // exact calculation, usefull for italics
  {
    sInt abcC;
    sInt ch = text[len-1];
    if(ch < NUMABC)
      abcC = prv->Widths[ch].abcC;
    else
    {
      ABC abc;
      GetCharABCWidths(sGDIDCOffscreen,ch,ch,&abc);
      abcC = abc.abcC;
    }

    if(abcC<0)
      width -= abcC;
  }

  return width;
}

sInt sFont2D::GetAdvance(const sChar *text,sInt len)
{
  SIZE out;

  if(len==-1) len=sGetStringLen(text);

  SelectObject(sGDIDCOffscreen,prv->Font);
  GetTextExtentPoint32W(sGDIDCOffscreen,text,len,&out);

  return out.cx;
}

sInt sFont2D::GetCharHeight()
{
  return prv->CharHeight;
}

sInt sFont2D::GetCharCountFromWidth(sInt width,const sChar *text,sInt len)
{
  SIZE out;
  INT count;

  if(width<0)
    return 0;
  if(len==-1) len=sGetStringLen(text);

  SelectObject(sGDIDCOffscreen,prv->Font);
  GetTextExtentExPointW(sGDIDCOffscreen,text,len,width,&count,0,&out);

  return count;
}

sFont2D::sLetterDimensions sFont2D::sGetLetterDimensions(const sChar letter)
{
  sLetterDimensions result;

  SelectObject(sGDIDCOffscreen,prv->Font);

  ABC abc;
  
  if(GetCharABCWidths(sGDIDCOffscreen,letter,letter,&abc))
  {
    result.Pre      = abc.abcA;
    result.Cell     = abc.abcB;
    result.Post     = abc.abcC;
    result.Advance  = abc.abcA + abc.abcB + abc.abcC;

    MAT2 mat;
    sClear(mat);
    mat.eM11.value = 1;
    mat.eM22.value = 1;
    GLYPHMETRICS metrics;

    if (GDI_ERROR != GetGlyphOutline(sGDIDCOffscreen, letter, GGO_METRICS, &metrics, 0, 0, &mat))
    {
      result.Height = metrics.gmBlackBoxY;
      result.OriginY = metrics.gmptGlyphOrigin.y;
    }
    else
    {
      SIZE size;
      GetTextExtentPoint32(sGDIDCOffscreen, &letter, 1, &size);
      result.Height = size.cy;
      result.OriginY = size.cy;
    }
  }
  else
  {
    SIZE size;
    GetTextExtentPoint32(sGDIDCOffscreen,&letter,1,&size);
    
    result.Pre      = 0;
    result.Cell     = size.cx;
    result.Post     = 0;
    result.Advance  = size.cx;
    result.Height   = size.cy;
    result.OriginY  = size.cy;
  }
  

  return result;
}

sBool sFont2D::LetterExists(sChar letter)
{
  SelectObject(sGDIDCOffscreen,prv->Font);
  
  sChar in[2];
  WORD out[2];
  
  in[0] = letter;
  in[1] = 0;

  
  if(GetGlyphIndices(sGDIDCOffscreen,&letter,1,out,GGI_MARK_NONEXISTING_GLYPHS)!=1)
    return 0;
  return out[0]!=0xffff;
}

void sFont2D::SetColor(sInt text,sInt back)
{
  sVERIFY(text>=0 && text<MAX_BRUSHES);
  sVERIFY(back>=0 && back<MAX_BRUSHES);
  prv->BackPen = back;
  prv->BackColor = BrushColor[back];
  prv->TextColor = BrushColor[text];
}

void sFont2D::PrintMarked(sInt flags,const sRect *rc,sInt x,sInt y,const sChar *text,sInt len,sPrintInfo *pi)
{
  if(len==-1) len=sGetStringLen(text);
  sVERIFY(rc);
  sVERIFY(pi);
  sRect r(*rc);
  const sChar *ot = text;
  sInt ol = len;
  sInt ox = x;
  sBool nocull = 0;

  switch(pi->Mode)
  {
  case sPIM_PRINT:
    nocull = pi->CullRect.IsEmpty() || rc->IsInside(pi->CullRect);
    if(pi->SelectStart<len && pi->SelectEnd>0 && pi->SelectEnd>pi->SelectStart)
    {
      sInt i = pi->SelectStart;
      if(i>0)
      {
        r.x1 = x + GetWidth(text,i);
        PrintBasic(flags,&r,x,y,text,i);
        r.x0 = x = r.x1;

        len -= i;
        pi->SelectEnd -= i;
        pi->SelectStart -= i;
        text += i;
      }

      sU32 oldbackcolor=prv->BackColor;
      if (pi->SelectBackColor!=~0)
        prv->BackColor=BrushColor[pi->SelectBackColor];
      else
        sSwap(prv->TextColor,prv->BackColor);

      i = sMin(pi->SelectEnd,len);
      r.x1 = x + GetWidth(text,i);
      if(nocull)
        PrintBasic(flags,&r,x,y,text,i);
      r.x0 = x = r.x1;

      len -= i;
      pi->SelectEnd -= i;
      pi->SelectStart -= i;
      text += i;

      if (pi->SelectBackColor!=~0)
        prv->BackColor=oldbackcolor;
      else
        sSwap(prv->TextColor,prv->BackColor);
    }
    if(nocull)
    {
      r.x1 = x + GetWidth(text,len);
      PrintBasic(flags,&r,x,y,text,len);
      if(flags & sF2P_OPAQUE)
      {
        r.x0 = x = r.x1;
        r.x1 = rc->x1;
        if(pi->HintLine && pi->HintLine>=r.x0 && pi->HintLine+2<=r.x1)
        {
          r.x1 = pi->HintLine;
          sRect2D(r,prv->BackPen);
          r.x0 = r.x1;
          r.x1 = r.x0+2;
          sRect2D(r,pi->HintLineColor);
          r.x0 = r.x1;
          r.x1 = rc->x1;
        }
        sRect2D(r,prv->BackPen);
      }
    }

    if(pi->CursorPos>=0 && pi->CursorPos<=ol && pi->Mode==sPIM_PRINT)
    {
      sInt t = GetWidth(ot,pi->CursorPos);
      sInt w = 2;
      if(pi->Overwrite)
      {
        if(pi->CursorPos<ol)
          w = GetWidth(ot+pi->CursorPos,1);
        else
          w = GetWidth(L" ",1);
      }
      sRectInvert2D(ox+t,rc->y0,ox+t+w,rc->y1);
    }
    break;

  case sPIM_POINT2POS:
    if(pi->QueryY>=rc->y0 && pi->QueryY<rc->y1)
    {
      x = ox;
	    sInt i;
      for(i=0;i<len;i++)
      {
        x += GetWidth(ot+i,1);
        if(pi->QueryX < x)
          break;
      }
      pi->QueryPos = ot+i;
      pi->Mode = sPIM_QUERYDONE;
    }
    break;

  case sPIM_POS2POINT:
    if(pi->QueryPos>=ot && pi->QueryPos<=ot+len)
    {
      pi->QueryY = rc->y0;
      pi->QueryX = ox+GetWidth(ot,sInt(pi->QueryPos-ot));
    }
    break;
  }
}



void sFont2D::PrintBasic(sInt flags,const sRect *r,sInt x,sInt y,const sChar *text,sInt len)
{
  UINT opt;

  if(len==-1) len=sGetStringLen(text);

  opt = 0;
  if(r)
  {
    opt |= ETO_CLIPPED;
  }
  if((flags & sF2P_OPAQUE) && r)
  {
    SetBkColor(sGDIDC,prv->BackColor);
    opt |= ETO_OPAQUE;
    SetBkMode(sGDIDC,OPAQUE);
  }
  else
  {
    SetBkMode(sGDIDC,TRANSPARENT);
  }
  SetTextColor(sGDIDC,prv->TextColor);
  SelectObject(sGDIDC,prv->Font);
  ExtTextOutW(sGDIDC,x,y,opt,(CONST RECT *)r,text,len,0);
}

sInt sFont2D::Print(sInt flags,const sRect &r,const sChar *text,sInt len,sInt margin,sInt xo,sInt yo,sPrintInfo *pi)
{
  if(len==-1) len=sGetStringLen(text);
  sInt x,y;
  sPrintInfo pil;
  const sChar *textstart=text;
  sInt result;

  sRect r2(r);

  pil.Init();
  if(pi)
  {
    pil = *pi;
  }
  pil.CursorPos = -1;
  pil.Overwrite = 0;
  pil.SelectStart = -1;
  pil.SelectEnd = -1;

  r2.Extend(-margin);
  x = r2.x0 + xo;
  y = r2.y0 + yo;
  sInt xs = r2.SizeX();
  sInt h = GetHeight();
  sRect rs(r);
  result = rs.y0;

  if(pil.Mode==sPIM_POINT2POS && pil.QueryY<y)
  {
    pi->Mode = sPIM_QUERYDONE;
    pi->QueryPos = text;
    return result;
  }

  if(flags & sF2P_MULTILINE)
  {
    const sChar *textend = text+len;
    for(;;)
    {
      sInt i=0;
      sInt best = 0;
      if(pi)
      {
        pil.CursorPos   = pi->CursorPos   - sInt(text-textstart);
        pil.Overwrite   = pi->Overwrite;
        pil.SelectStart = pi->SelectStart - sInt(text-textstart);
        pil.SelectEnd   = pi->SelectEnd   - sInt(text-textstart);
      }
      for(;;)
      {
        while(text+i<textend && text[i]!=' ' && text[i]!='\n')
          i++;
        if(GetWidth(text,i)>xs)
          break;
        best = i;
        if(text[i]=='\n')
          break;
        if(text+i>=textend)
          break;
        i++;
      }
      if(best==0)
      {
        i = 0;
        while(text+i<textend && text[i]!=' ' && text[i]!='\n' && GetWidth(text,i)<=xs)
          i++;
        best = i-1;
        if(best<1 && text<textend && text[0]!=' ' && text[0]!='\n')
          best = 1;
        if(best<0)
          best = 0;
        else if(pil.CursorPos>=best)
          pil.CursorPos = -1;
      }

      rs.y1 = y + h;
      sRect rclip = rs;
      rclip.And(r);

      PrintMarked(flags,&rclip,x,y,text,best,&pil);

      rs.y0 = rs.y1;
      text+=best;
      if(text>=textend) break;
      y+=h;
      if(text[0]==' ' || text[0]=='\n')
        text++;
    }

    if(pil.Mode == sPIM_POS2POINT && pil.QueryPos == text) // query was to end of document?
    {
      pil.QueryX = x;
      pil.QueryY = y;
    }

    result = y+h+margin;
    rs.y1 = r.y1;
    if(rs.y1>rs.y0 && pil.Mode==sPIM_PRINT && (flags & sF2P_OPAQUE))
    {
      sRect r(rs);
      if(pil.HintLine && pil.HintLine>=r.x0 && pil.HintLine+2<=r.x1)
      {
        r.x1 = pil.HintLine;
        sRect2D(r,prv->BackPen);
        r.x0 = r.x1;
        r.x1 = r.x0+2;
        sRect2D(r,pil.HintLineColor);
        r.x0 = r.x1;
        r.x1 = rs.x1;
      }
      sRect2D(r,prv->BackPen);
    }

    if(pi)
    {
      pi->QueryPos = pil.QueryPos;
      pi->QueryX = pil.QueryX;
      pi->QueryY = pil.QueryY;
      pi->Mode = pil.Mode;
    }
  }
  else
  {
    sInt space=0;
    if(flags & sF2P_SPACE)
      space = GetWidth(L" ");
    if(flags & sF2P_LEFT)
    {
      x = r2.x0 + space;
    }
    else if(flags & sF2P_RIGHT)
    {
      x = r2.x1 - GetWidth(text,len) - space;
    }
    else
    {
      x = (r2.x0 + r2.x1 - GetWidth(text,len))/2;
    }
    if(flags & sF2P_TOP)
    {
      y = r2.y0;
    }
    else if(flags & sF2P_BOTTOM)
    {
      y = r2.y1 - h;
    }
    else
    {
      y = (r2.y0 + r2.y1 - h)/2;
    }

    if(pi)
    {
      sPrintInfo pi2 = *pi;
      PrintMarked(flags,&r,x+xo,y+yo,text,len,&pi2);
      if(pi->Mode)
      {
        pi->QueryPos = pi2.QueryPos;
        pi->QueryX = pi2.QueryX;
        pi->QueryY = pi2.QueryY;
      }
    }
    else
    {
      PrintBasic(flags,&r,x,y,text,len);
    }

    result = y+h;
  }

  return result;
}

#endif

// system independend functions

void sPrintInfo::Init()
{
  Mode = sPIM_PRINT;

  CursorPos = 0;
  Overwrite = 0;
  SelectStart = -1;
  SelectEnd = -1;
  SelectBackColor = ~0;

  QueryX = 0;
  QueryY = 0;
  QueryPos = 0;

  HintLine = 0;
  HintLineColor = 0;
}

void sFont2D::Print(sInt flags,sInt x,sInt y,const sChar *text,sInt len)
{
  if(len==-1) len=sGetStringLen(text);

  PrintBasic(flags,0,x,y,text,len);
}


/****************************************************************************/

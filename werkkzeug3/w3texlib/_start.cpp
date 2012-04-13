// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#include "_start.hpp"

#define WINVER 0x500
#define _WIN32_WINNT 0x0500
#define DIRECTINPUT_VERSION 0x0800

#include <windows.h>
#include <olectl.h>
#define WINZERO(x) {sSetMem(&x,0,sizeof(x));}
#define WINSET(x) {sSetMem(&x,0,sizeof(x));x.dwSize = sizeof(x);}
#define RELEASE(x) {if(x)x->Release();x=0;}
#define DXERROR(hr) {if(FAILED(hr))sFatal("%s(%d) : directx error %08x (%d)",__FILE__,__LINE__,hr,hr&0x3fff);}

#undef DeleteFile
#undef GetCurrentDirectory
#undef LoadBitmap
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"oleaut32.lib")

/****************************************************************************/

#if sMEMDEBUG

#include <crtdbg.h>
#undef new

void * __cdecl operator new(unsigned int size)
{
  void *p;

  p = _malloc_dbg(size,_NORMAL_BLOCK,"unknown",1);
  if(p==0)
    exit(1);

  return p;
}

void * __cdecl operator new(unsigned int size,const char *file,int line)
{
  void *p;

  p = _malloc_dbg(size,_NORMAL_BLOCK,file,line);
  if(p==0)
    exit(1);

  return p;
}

void __cdecl operator delete(void *p)
{
  if(p)
		_free_dbg(p,_NORMAL_BLOCK);
}

#define new new(__FILE__,__LINE__)
#endif

/****************************************************************************/

namespace Werkk3TexLib
{
#pragma warning (disable: 4312)
#pragma warning (disable: 4996)

#define STBI_NO_HDR
#define STBI_NO_STDIO
#include "stb_image.c"

static sSystem_ system;
sSystem_ *sSystem = &system;

/****************************************************************************/

sU32 *sSystem_::FontMem = 0;

static HDC GDIScreenDC;
static HDC GDIDC;
static HBITMAP GDIHBM;
static BITMAPINFO FontBMI;
static HBITMAP FontHBM;
static HFONT FontHandle;

#define HIMETRIC_PER_INCH         2540
#define MAP_PIX_TO_LOGHIM(x,ppli) ((HIMETRIC_PER_INCH*(x)+((ppli)>>1))/(ppli))
#define MAP_LOGHIM_TO_PIX(x,ppli) (((ppli)*(x)+HIMETRIC_PER_INCH/2)/HIMETRIC_PER_INCH)

sBool sSystem_::LoadBitmapCore(const sU8 *data,sInt size,sInt &xout,sInt &yout,sU8 *&dataout)
{
  HDC screendc;
  HDC hdc;
  HDC hdc2,hdcold;
  HBITMAP hbm,hbm2;
  BITMAPINFO bmi;
  sInt i;
  HRESULT hr;
  IPicture *pic;
  IStream *str;
  DWORD dummy;
  POINT wpl,wpp;
  sInt xs,ys;
//  sBitmap *bm;

  union _ULARGE_INTEGER usize;
  union _LARGE_INTEGER useek;
  union _ULARGE_INTEGER udummy;

  xout = 0;
  yout = 0;
  dataout = 0;

#if sLINK_PNG
  if(LoadPNG(data,size,xout,yout,dataout))
    return sTRUE;
#else
  dataout = stbi_png_load_from_memory(data,size,&xout,&yout,0,4);
  if(dataout)
  {
    int count = xout*yout;
    // byteswap rgba->bgra
    sU8 *buffer = dataout;
    for(int i=0;i<count;i++)
      sSwap(buffer[i*4+0],buffer[i*4+2]);

    return sTRUE;
  }
#endif

  screendc = GetDC(0);
  hr = CreateStreamOnHGlobal(0,TRUE,&str);
  if(!FAILED(hr))
  {
    usize.QuadPart = size;
    str->SetSize(usize);
    str->Write(data,size,&dummy);
    useek.QuadPart = 0;
    str->Seek(useek,STREAM_SEEK_SET,&udummy);
    hr = OleLoadPicture(str,size,TRUE,IID_IPicture,(void**)&pic);

    if(!FAILED(hr))
    {
      pic->get_Width(&wpl.x); 
      pic->get_Height(&wpl.y); 
      wpp = wpl;
  	  sInt px=GetDeviceCaps(screendc,LOGPIXELSX);
  	  sInt py=GetDeviceCaps(screendc,LOGPIXELSY);
  	  wpp.x=MAP_LOGHIM_TO_PIX(wpl.x,px);
  	  wpp.y=MAP_LOGHIM_TO_PIX(-wpl.y,py);

      hdc = CreateCompatibleDC(screendc);
      hdc2 = CreateCompatibleDC(screendc);

      xout = xs = sMakePower2(wpp.x);
      yout = ys = sMakePower2(-wpp.y);

      dataout = new sU8[xs*ys*4];
      hbm = CreateCompatibleBitmap(screendc,xs,ys);
      SelectObject(hdc,hbm);

      sSetMem4((sU32 *)dataout,0xffff0000,xs*ys);
      bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
      bmi.bmiHeader.biWidth = xs;
      bmi.bmiHeader.biHeight = -ys;
      bmi.bmiHeader.biPlanes = 1;
      bmi.bmiHeader.biBitCount = 32;
      bmi.bmiHeader.biCompression = BI_RGB;

      hr = pic->SelectPicture(hdc2,&hdcold,(unsigned int *)&hbm2);
      if(!FAILED(hr))
      {
        SetMapMode(hdc2,MM_TEXT);
        StretchBlt(hdc,0,0,xs,ys,hdc2,0,0,wpp.x,-wpp.y,SRCCOPY);
        GetDIBits(hdc,hbm,0,ys,dataout,&bmi,DIB_RGB_COLORS);
        pic->SelectPicture(hdcold,0,(unsigned int *)&hbm2);
      }
      pic->Release();

      DeleteDC(hdc2);
      DeleteDC(hdc);
      DeleteObject(hbm);
      for(i=0;i<xs*ys;i++)
        ((sU32 *)dataout)[i] |= 0xff000000;
    }
    str->Release();
  }
  ReleaseDC(0,screendc);

  return dataout!=0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Host Font Interface                                                ***/
/***                                                                      ***/
/****************************************************************************/

sInt sSystem_::FontBegin(sInt pagex,sInt pagey,const sChar *name,sInt xs,sInt ys,sInt style)
{
  TEXTMETRIC met;
  HDC GDIScreenDC = GetDC(0);

  if(!GDIDC)
  {
    GDIDC = CreateCompatibleDC(GDIScreenDC);
    GDIHBM = CreateCompatibleBitmap(GDIScreenDC,16,16);
  }

  SelectObject(GDIDC,GDIHBM);

  FontBMI.bmiHeader.biSize = sizeof(FontBMI.bmiHeader);
  FontBMI.bmiHeader.biWidth = pagex;
  FontBMI.bmiHeader.biHeight = -pagey;
  FontBMI.bmiHeader.biPlanes = 1;
  FontBMI.bmiHeader.biBitCount = 32;
  FontBMI.bmiHeader.biCompression = BI_RGB;
  FontHBM = CreateDIBSection(GDIScreenDC,&FontBMI,DIB_RGB_COLORS,
    (void **) &FontMem,0,0);
  SelectObject(GDIDC,FontHBM);

  if(name[0]==0) name = "arial";

  FontHandle = CreateFont(-ys,xs,0,0,(style&2) ? FW_BOLD : FW_NORMAL,
    (style&1) ? 1 : 0,0,0,DEFAULT_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,
    PROOF_QUALITY,
    DEFAULT_PITCH|FF_DONTCARE,name);

  SetBkMode(GDIDC,TRANSPARENT);
  SelectObject(GDIScreenDC,FontHandle);    // win98: first select font into screen dc
  SelectObject(GDIDC,FontHandle);
  SelectObject(GDIDC,FontHBM);
  SetTextColor(GDIDC,0xffffff);

  GetTextMetrics(GDIDC,&met);
  ReleaseDC(0,GDIScreenDC);

  return met.tmHeight;
}

sInt sSystem_::FontWidth(const sChar *string,sInt len)
{
  SIZE p;

  if(len==-1)
  {
    len++;
    while(string[len]) len++;
  }
  GetTextExtentPoint32(GDIDC,string,len,&p);
  return p.cx;
}

void sSystem_::FontCharWidth(sInt ch,sInt *widths)
{
  ABC abc;

  GetCharABCWidths(GDIDC,ch,ch,&abc);

  widths[0] = abc.abcA;
  widths[1] = abc.abcB;
  widths[2] = abc.abcC;
}

void sSystem_::FontPrint(sInt x,sInt y,const sChar *string,sInt len)
{
  if(len==-1)
  {
    len++;
    while(string[len]) len++;
  }
  ExtTextOut(GDIDC,x,y,0,0,string,len,0);
}

void sSystem_::FontEnd()
{
  SelectObject(GDIDC,GDIHBM);
  DeleteObject(FontHBM);
  DeleteDC(GDIDC);
  DeleteObject(GDIHBM);
}

}

/****************************************************************************/
/****************************************************************************/

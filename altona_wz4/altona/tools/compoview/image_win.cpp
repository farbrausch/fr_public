/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) 2005-2006 by Dierk Ohlerich                          ***/
/***   all rights reserved                                                ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "image_win.hpp"

#if sPLATFORM==sPLAT_WINDOWS

#undef new
#include <olectl.h>
#include <Gdiplus.h>
#pragma comment (lib, "gdiplus.lib")
#define new sDEFINE_NEW

#include "base/system.hpp"
#include "util/image.hpp"

/****************************************************************************/
/***                                                                      ***/
/***  Wrapper class from sFile to Win32 IStream                           ***/
/***                                                                      ***/
/****************************************************************************/

class sIStreamWrapper : public IStream
{
protected:
  sFile   *File;
  sU32    RefCount;

public:
  sIStreamWrapper(sFile *f) : File(f), RefCount(1) {}
  virtual ~sIStreamWrapper() {}

  // IUnknown

  virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject)
  {
    if (!ppvObject) return E_INVALIDARG;
    if (iid==IID_IUnknown || iid==IID_IStream || iid==IID_ISequentialStream)
    {
      RefCount++;
      *ppvObject=this;
      return S_OK;
    }
    return E_NOINTERFACE;
  }

  virtual ULONG STDMETHODCALLTYPE AddRef()
  {
    return ++RefCount;
  }

  virtual ULONG STDMETHODCALLTYPE Release()
  {
    sVERIFY(RefCount>1); // must never reach zero
    return --RefCount;
  }

  // ISequentialStream

  virtual HRESULT STDMETHODCALLTYPE Read(void *buf, ULONG count, ULONG *cntout)
  {
    if (!buf) return STG_E_INVALIDPOINTER;
    count = sMin<ULONG>(count,File->GetSize()-File->GetOffset());
    if (!count || File->Read(buf,count))
    {
      if (cntout) *cntout=count;
      return S_OK;
    }
    if (cntout) *cntout=0;
    return STG_E_READFAULT;
  }

  virtual HRESULT STDMETHODCALLTYPE Write(const void *buf, ULONG count, ULONG *cntout)
  {
    sVERIFYFALSE;
    return STG_E_INVALIDFUNCTION;
  }
   
  // IStream

  virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
  {
    sS64 offs;
    switch (dwOrigin)
    {
    case STREAM_SEEK_SET: offs=0; break;
    case STREAM_SEEK_CUR: offs=File->GetOffset(); break;
    case STREAM_SEEK_END: offs=File->GetSize(); break;
    default: return STG_E_INVALIDFUNCTION;
    }
    File->SetOffset(offs+dlibMove.QuadPart);

    if (plibNewPosition) plibNewPosition->QuadPart=File->GetOffset();
    return S_OK;
  } 

  virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize)
  {
    sVERIFYFALSE;
    return STG_E_INVALIDFUNCTION;
  }

  virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
  {
    sVERIFYFALSE;
    return STG_E_INVALIDFUNCTION;
  }

  virtual HRESULT STDMETHODCALLTYPE Commit(DWORD)
  {
    return S_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE Revert()
  {
    sVERIFYFALSE;
    return STG_E_INVALIDFUNCTION;
  }

  virtual HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD dwLockType)
  {
    sVERIFYFALSE;
    return STG_E_INVALIDFUNCTION;
  }

  virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD dwLockType)
  {
    sVERIFYFALSE;
    return STG_E_INVALIDFUNCTION;
  }

  virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG *pstatstg, DWORD grfStatFlag)
  {
    if (!(grfStatFlag&1)) 
    {
      sVERIFYFALSE;
      return STG_E_ACCESSDENIED;
    }
    sSetMem(pstatstg,0,sizeof(*pstatstg));
    pstatstg->type = STGTY_STREAM;
    pstatstg->cbSize.QuadPart = File->GetSize();
    return S_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE Clone(IStream **)
  {
    sVERIFYFALSE;
    return STG_E_INSUFFICIENTMEMORY;
  }
};

/****************************************************************************/
/****************************************************************************/

sBool sLoadImageWin32(sFile *file, sImage &img)
{
  Gdiplus::Status st;
  Gdiplus::Color background(0,0,0,0);
  Gdiplus::Bitmap *gdibitmap = Gdiplus::Bitmap::FromStream(&sIStreamWrapper(file),TRUE);

  HBITMAP hbmp=0;
  st = gdibitmap->GetHBITMAP(background,&hbmp);

  sBool ok = sFALSE;

  if (!st) 
  {
    BITMAP    bmp;
    GetObject(hbmp,sizeof(bmp),&bmp);
    sVERIFY(bmp.bmType==0 && bmp.bmPlanes==1);

    if (bmp.bmType==0 && bmp.bmPlanes==1)
    {
    
      sBool topdown=sFALSE;
      if (bmp.bmHeight<0)
      {
        bmp.bmHeight*=-1;
        topdown=sTRUE;
      }

      img.Init(bmp.bmWidth,bmp.bmHeight);

      sU8 *s = (sU8*)bmp.bmBits;
      switch(bmp.bmBitsPixel)
      {
      case 24:
        for(sInt y=0;y<img.SizeY;y++)
        {
          sU32 *d;
          if (topdown)
            d = img.Data + y*img.SizeX;
          else
            d = img.Data + (img.SizeY-1-y)*img.SizeX;

          for(sInt x=0;x<img.SizeX;x++,s+=3)
            *d++ = 0xff000000|(s[0])|(s[1]<<8)|(s[2]<<16);
        }
        ok = sTRUE;
        break;

      case 32:
        for(sInt y=0;y<img.SizeY;y++)
        {
          sU32 *d;
          if (topdown)
            d = img.Data + y*img.SizeX;
          else
            d = img.Data + (img.SizeY-1-y)*img.SizeX;

          for(sInt x=0;x<img.SizeX;x++,s+=4)
            *d++ = *(sU32*)s;
        }
        ok = sTRUE;
        break;

      default:
        sLogF(L"ERROR",L"can't load bmp with %d bits per pixel\n",bmp.bmBitsPixel);
        return false;
      }
    }

    DeleteObject(hbmp);
  }

  delete gdibitmap;
  return ok;
}

sImage *sLoadImageWin32(sFile *file)
{
  sImage *img = new sImage;
  if (!sLoadImageWin32(file, *img))
    sDelete(img);
  return img;
}

sBool sLoadImageWin32(const sChar *name, sImage &img)
{
  sFile *file = sCreateFile(name,sFA_READRANDOM);
  sBool ok = sFALSE;
  if (file)
    ok = sLoadImageWin32(file, img);
  delete file;
  return ok;
}

sImage *sLoadImageWin32(const sChar *name)
{
  sImage *img=0;
  sFile *file = sCreateFile(name,sFA_READRANDOM);
  if (file)
    img = sLoadImageWin32(file);
  delete file;
  return img;
}

/****************************************************************************/

static ULONG_PTR sGdiToken=0;

static void sInitGdiPlus()
{
  Gdiplus::GdiplusStartupInput gsin;
  sClear(gsin);
  gsin.GdiplusVersion=1;
  GdiplusStartup(&sGdiToken,&gsin,NULL);
}

static void sExitGdiPlus()
{
  Gdiplus::GdiplusShutdown(sGdiToken);
}

sADDSUBSYSTEM(GdiPlus, 0x78, sInitGdiPlus, sExitGdiPlus);

/****************************************************************************/

#else

sImage *sLoadImageWin32(sFile *file)
{
  return null;
}

sImage *sLoadImageWin32(const sChar *name)
{
  return null;
}


#endif
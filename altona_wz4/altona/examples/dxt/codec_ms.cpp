
/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>

#include "codec_ms.hpp"
#include "base/graphics.hpp"

extern IDirect3DDevice9 *DXDev;
void DXError(sU32 err,const sChar *file,sInt line,const sChar *system);
#define DXErr(err) { if(FAILED(err)) DXError(err,sTXT(__FILE__),__LINE__,L"d3d"); }

/****************************************************************************/
/****************************************************************************/

CodecMS::CodecMS() 
{
}

CodecMS::~CodecMS() 
{
}

/****************************************************************************/

const sChar *CodecMS::GetName()
{
  return L"microsoft";
}

void CodecMS::Pack(sImage *bmp,sImageData *dxt,sInt level)
{
  IDirect3DSurface9 *surf;
  sInt xs,ys;
  D3DFORMAT d3dformat,srcfmt;
  sImage *bmpdel=0;
  D3DLOCKED_RECT lr;
  RECT wr;
  sU8 *s,*d;
  sInt blocksize;

  xs = bmp->SizeX;
  ys = bmp->SizeY;
  switch(level)
  {
  case sTEX_DXT1:
    d3dformat = D3DFMT_DXT1;
    srcfmt = D3DFMT_X8R8G8B8;
    blocksize = 8;
    break;
  case sTEX_DXT1A:
    d3dformat = D3DFMT_DXT1;
    srcfmt = D3DFMT_A8R8G8B8;
    blocksize = 8;
    bmpdel = bmp->Copy();
    bmp = bmpdel;
    for(sInt i=0;i<bmp->SizeX*bmp->SizeY;i++)
    {
      if(bmp->Data[i]>=0x80000000) 
        bmp->Data[i]|=0xff000000; 
      else 
        bmp->Data[i]&=0x00ffffff;
    }
    break;
  case sTEX_DXT3:
    d3dformat = D3DFMT_DXT3;
    srcfmt = D3DFMT_A8R8G8B8;
    blocksize = 16;
    break;
  case sTEX_DXT5:
    d3dformat = D3DFMT_DXT5;
    srcfmt = D3DFMT_A8R8G8B8;
    blocksize = 16;
    break;
  default:
    d3dformat = D3DFMT_UNKNOWN;
    srcfmt = D3DFMT_UNKNOWN;
    blocksize = 0;
    sVERIFYFALSE;
  }

  wr.top = 0;
  wr.left = 0;
  wr.right = xs;
  wr.bottom = ys;

  surf = 0;
  sVERIFY(DXDev);
  DXErr(DXDev->CreateOffscreenPlainSurface(xs,ys,d3dformat,D3DPOOL_SCRATCH,&surf,0));
  DXErr(D3DXLoadSurfaceFromMemory(surf,0,0,bmp->Data,srcfmt,xs*4,0,&wr,D3DX_FILTER_POINT|D3DX_FILTER_DITHER,0));

  DXErr(surf->LockRect(&lr,0,D3DLOCK_READONLY));

  s = (sU8 *)lr.pBits;
  d = dxt->Data;
  for(sInt y=0;y<ys;y+=4)
  {
    sCopyMem(d,s,xs/4*blocksize);
    s += lr.Pitch;
    d += xs/4*blocksize;
  }

  DXErr(surf->UnlockRect());
  surf->Release();

  delete bmpdel;
}

/****************************************************************************/

void CodecMS::Unpack(sImage *bmp,sImageData *dxt,sInt level)
{
  IDirect3DSurface9 *surf;
  sInt xs,ys;
  D3DFORMAT d3dformat;
  D3DLOCKED_RECT lr;
  RECT wr;
  sU8 *s,*d;
  sInt blocksize;

  xs = bmp->SizeX;
  ys = bmp->SizeY;
  switch(level)
  {
  case sTEX_DXT1:
  case sTEX_DXT1A:
    d3dformat = D3DFMT_DXT1;
    blocksize = 8;
    break;
  case sTEX_DXT3:
    d3dformat = D3DFMT_DXT3;
    blocksize = 16;
    break;
  case sTEX_DXT5:
    d3dformat = D3DFMT_DXT5;
    blocksize = 16;
    break;
  default:
    d3dformat = D3DFMT_UNKNOWN;
    blocksize = 0;
    sVERIFYFALSE;
  }

  wr.top = 0;
  wr.left = 0;
  wr.right = xs;
  wr.bottom = ys;

  surf = 0;
  sVERIFY(DXDev);
  DXErr(DXDev->CreateOffscreenPlainSurface(xs,ys,D3DFMT_A8R8G8B8,D3DPOOL_SCRATCH,&surf,0));
  DXErr(D3DXLoadSurfaceFromMemory(surf,0,0,dxt->Data,d3dformat,xs/4*blocksize,0,&wr,D3DX_FILTER_POINT,0));

  DXErr(surf->LockRect(&lr,0,D3DLOCK_READONLY));

  s = (sU8 *)lr.pBits;
  d = (sU8 *)bmp->Data;
  for(sInt y=0;y<ys;y++)
  {
    sCopyMem(d,s,xs*4);
    s += lr.Pitch;
    d += xs*4;
  }

  DXErr(surf->UnlockRect());
  surf->Release();
}

/****************************************************************************/
/****************************************************************************/



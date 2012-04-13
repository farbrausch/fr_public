// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#include "_rygdxt.hpp"
#include "w3texlib.hpp"
#include "w3texmain.hpp"

using namespace Werkk3TexLib;

/****************************************************************************/

// W3Image implementation
W3Image::W3Image(int x,int y,W3ImageFormat fmt)
{
  XSize = x;
  YSize = y;
  Format = fmt;

  MipLevels = 0;
  for(int i=0;i<16;i++)
  {
    MipSize[i] = 0;
    MipData[i] = 0;
  }
}

W3Image::W3Image(const W3Image &x)
{
  XSize = x.XSize;
  YSize = x.YSize;
  Format = x.Format;

  MipLevels = x.MipLevels;
  for(int i=0;i<MipLevels;i++)
  {
    MipSize[i] = x.MipSize[i];
    MipData[i] = new unsigned char[MipSize[i]];
    sCopyMem(MipData[i],x.MipData[i],MipSize[i]);
  }
}

W3Image::~W3Image()
{
  for(int i=0;i<MipLevels;i++)
    delete[] MipData[i];
}

W3Image &W3Image::operator =(const W3Image &x)
{
  W3Image tmp(x);
  Swap(tmp);

  return *this;
}

void W3Image::Swap(W3Image &x)
{
  sSwap(XSize,x.XSize);
  sSwap(YSize,x.YSize);
  sSwap(Format,x.Format);

  sSwap(MipLevels,x.MipLevels);
  for(int i=0;i<16;i++)
  {
    sSwap(MipSize[i],x.MipSize[i]);
    sSwap(MipData[i],x.MipData[i]);
  }
}

/****************************************************************************/

// W3TextureContext implementation

struct W3TextureContext::Private
{
  struct Image
  {
    sChar *Name;
    W3Image *Img;
  };

  KDoc Document;
  bool DocInitialized;
  sArray<Image> Images;
};

W3TextureContext::W3TextureContext()
{
  // Allocate private data
  d = new Private;
  d->DocInitialized = false;
  d->Images.Init();

  // initialize lib
  sInitPerlin();
  sInitDXT();
}

W3TextureContext::~W3TextureContext()
{
  FreeDocument();
  FreeImages();

  d->Images.Exit();
  delete d;
}

bool W3TextureContext::Load(const unsigned char *data)
{
  bool result;

  FreeDocument();
  result = d->Document.Init(data) != sFALSE;
  d->DocInitialized = result;

  if(result)
  {
    d->Images.Resize(d->Document.Roots.Count);
    for(sInt i=0;i<d->Document.Roots.Count;i++)
    {
      d->Images[i].Name = sDupString(d->Document.RootNames[i]);
      d->Images[i].Img = 0;
    }
  }
  else
    d->Images.Resize(0);

  return result;
}

bool W3TextureContext::Calculate(W3ProgressCallback progress)
{
  for(sInt i=0;i<d->Images.Count;i++)
  {
    delete d->Images[i].Img;
    d->Images[i].Img = 0;
  }

  bool result = d->Document.Calculate(progress) != sFALSE;

  if(result)
  {
    sInt i;
    for(i=0;i<sMin(d->Document.RootImages.Count,d->Images.Count);i++)
      d->Images[i].Img = d->Document.RootImages[i];

    while(i<d->Images.Count)
      d->Images[i++].Img = 0;
  }

  return result;
}

void W3TextureContext::FreeDocument()
{
  if(d->DocInitialized)
  {
    d->Document.Exit();
    d->DocInitialized = false;
  }
}

void W3TextureContext::FreeImages()
{
  for(sInt i=0;i<d->Images.Count;i++)
  {
    delete[] d->Images[i].Name;
    delete d->Images[i].Img;
  }

  d->Images.Resize(0);
}

int W3TextureContext::GetImageCount() const
{
  return d->Images.Count;
}

const W3Image *W3TextureContext::GetImageByName(const char *name) const
{
  // try to find a match
  for(sInt i=0;i<d->Images.Count;i++)
  {
    if(!sCmpString(name,d->Images[i].Name)) // found it!
      return d->Images[i].Img;
  }

  // not found, return 0
  return 0;
}

const W3Image *W3TextureContext::GetImageByNumber(int index) const
{
  // return image if valid index specified, 0 otherwise.
  if(index >= 0 && index < d->Images.Count)
    return d->Images[index].Img;
  else
    return 0;
}

const char *W3TextureContext::GetImageName(int index) const
{
  // return name if valid index specified, 0 otherwise.
  if(index >= 0 && index < d->Images.Count)
    return d->Images[index].Name;
  else
    return 0;
}

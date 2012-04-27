/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "doc.hpp"
#include "base/system.hpp"
#include "base/graphics.hpp"

#include "codec_old.hpp"
#include "codec_altona.hpp"
#include "codec_ms.hpp"
#include "codec_test.hpp"
#include "codec_nvidia.hpp"
#include "codec_ati.hpp"
#include "codec_ryg.hpp"

Document *Doc;

/****************************************************************************/
/****************************************************************************/

DocImage::DocImage()
{
  Image = 0;
  sClear(Dxt);
}

DocImage::~DocImage()
{
  delete Image;
  for(sInt i=0;i<MAX_CODECS;i++)
    delete Dxt[i];
}

void DocImage::Calc(sInt codec, sInt format, sBool loop)
{
  sInt temp_format;
  switch(format)
  {
  case sTEX_DXT1:
    temp_format = 0;
    break;
  case sTEX_DXT1A:
    temp_format = 1;
    break;
  case sTEX_DXT3:
    temp_format = 2;
    break;
  case sTEX_DXT5:
    temp_format = 3;
    break;
  default:
    sVERIFYFALSE;
  }
  if((Dxt[codec]==0 || (Dxt[codec]->Format&sTEX_FORMAT)!=format || loop) && Doc->Codecs[codec] != 0)
  {
    if(Dxt[codec]==0)
      Dxt[codec] = new sImageData;
    Dxt[codec]->Init2(format|sTEX_2D,1,Image->SizeX,Image->SizeY,1);
    sInt starttime = sGetTime();
    Doc->Codecs[codec]->Pack(Image,Dxt[codec],format);
    CompressionTime[codec][temp_format] =  sGetTime() - starttime;
  }
}

/****************************************************************************/
/****************************************************************************/

Document::Document()
{
  sClear(Codecs);
  sInt i=0;
  Codecs[i++] = new CodecAltona;
  Codecs[i++] = new CodecOld;
#if sPLATFORM==sPLAT_WINDOWS
  Codecs[i++] = new CodecMS;
#endif
  Codecs[i++] = new CodecTest;
  Codecs[i++] = new CodecRyg;
#if LINK_ATI
  Codecs[i++] = new CodecATI;
#endif
#if LINK_NVIDIA
  Codecs[i++] = new CodecNVIDIA;
#endif
}

Document::~Document()
{
  for(sInt i=0;i<MAX_CODECS;i++)
    delete Codecs[i];
}

/****************************************************************************/

void Document::Scan(const sChar *name)
{
  sArray<sDirEntry> list;
  sDirEntry *de;
  sString<sMAXPATH> str;
  const sChar *ext;

  if(sLoadDir(list,name))
  {
    sFORALL(list,de)
    {
      str = name;
      str.AddPath(de->Name);
      ext = sFindFileExtension(str);
      if(sCmpStringI(ext,L"pic")==0 || sCmpStringI(ext,L"bmp")==0 || sCmpStringI(ext,L"tga")==0)
        LoadImage(str,de->Name);
    }
  }
}

void Document::LoadImage(const sChar *path,const sChar *name)
{
  DocImage *img;

  img = new DocImage;
  img->Image = new sImage;
  img->Image->Load(path);
  img->Name = name;
  Images.AddTail(img);

  
  // this will help testing DXT1 with alpha
  if(0)
  {
    for(sInt i=0;i<img->Image->SizeX*img->Image->SizeY;i+=4)
      img->Image->Data[i]&=0xffffff;
  }
}

/****************************************************************************/
/****************************************************************************/
 
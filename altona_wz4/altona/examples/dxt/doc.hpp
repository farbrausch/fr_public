/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#pragma once
#include "base/types2.hpp"
#include "util/image.hpp"

/****************************************************************************/

#define MAX_CODECS  8

class DocCodec
{
public:
  DocCodec() {}
  virtual ~DocCodec() {}

  virtual const sChar *GetName() = 0;
  virtual void Pack(sImage *bmp,sImageData *dxt,sInt level=1) = 0;
  virtual void Unpack(sImage *bmp,sImageData *dxt,sInt level=1) = 0;
};

/****************************************************************************/

class DocImage : public sObject
{
public:
  sInt CompressionTime[MAX_CODECS][4];

  sImage *Image;
  sImageData *Dxt[MAX_CODECS];

  sString<256> Name;
  const sChar *GetName() { return Name; }

  DocImage();
  ~DocImage();

  void Calc(sInt codec, sInt format, sBool loop = false);
};

/****************************************************************************/

class Document : public sObject
{
public:
  Document();
  ~Document();
  sArray<DocImage *> Images;
  DocCodec *Codecs[MAX_CODECS];

  void Scan(const sChar *name);
  void LoadImage(const sChar *path,const sChar *name);
};

extern Document *Doc;

/****************************************************************************/

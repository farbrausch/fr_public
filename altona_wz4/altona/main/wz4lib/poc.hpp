/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) by Dierk Ohlerich                                    ***/
/***   all rights reserverd                                               ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WERKKZEUG4_POC_HPP
#define FILE_WERKKZEUG4_POC_HPP

#ifndef __GNUC__
#pragma once
#endif

#include "base/types.hpp"
#include "base/graphics.hpp"
#include "util/image.hpp"
#include "doc.hpp"
#include "basic.hpp"

/****************************************************************************/

class PocBitmap : public BitmapBase
{
public:
  PocBitmap();
  ~PocBitmap();

  void CopyTo(sImage *dest);
  void CopyTo(sImageI16 *dest);
  void CopyTo(sImageData *dest,sInt format);

  void CopyFrom(const PocBitmap *src);
  wObject *Copy();

  sImage *Image;
  sTexture2D *Texture;
  sString<256> Name;
};

struct PocBitmapAtlas
{
  sImage *Image;
  sInt PosX;
  sInt PosY;
  sInt SizeX;
  sInt SizeY;
  sInt Order;
};

enum
{
  PBMA_AUTO,
  PBMA_HORIZONTAL,
  PBMA_VERTICAL,
};

void PocBitmapMakeAtlas(sStaticArray<PocBitmapAtlas> &in,sImage *out,sU32 emptycol, sInt mode=PBMA_AUTO);

/****************************************************************************/

class PocMaterial : public wObject
{
public:
  sSimpleMaterial *Material;
  wObject *Tex[8];
  sVertexFormatHandle *Format;
  sString<64> Name;

  PocMaterial();
  ~PocMaterial();
  wObject *Copy();
};

/****************************************************************************/

struct PocMeshVertex
{
  sVector31 Pos;
  sVector30 Normal;
  sF32 u,v;
};

struct PocMeshCluster 
{
  sInt StartIndex;
  sInt EndIndex;
  PocMaterial *Mtrl;
};

class PocMesh : public wObject
{
public:
  PocMesh();
  ~PocMesh();
  void Init(sInt vc,sInt ic,sInt cl=1);
  void Copy(PocMesh *src);
  void CacheSolid();
  void Hit(const sRay &ray,wHitInfo &info);
  void Wireframe(wObject *obj,wPaintInfo &pi,sGeometry *geo,sMatrix34 &mat);
  wObject *Copy();

  sArray<PocMeshVertex> Vertices;
  sArray<sU32> Indices;
  sArray<PocMeshCluster> Clusters;
  
  sGeometry *SolidGeo;
};

/****************************************************************************/

#endif // FILE_WERKKZEUG4_POC_HPP


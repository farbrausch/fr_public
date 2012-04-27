/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) by Dierk Ohlerich                                    ***/
/***   all rights reserved                                                ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_FR033_HPP
#define FILE_WZ4FRLIB_FR033_HPP

#include "base/types.hpp"
#include "util/image.hpp"
#include "wz4lib/doc.hpp"
#include "wz4lib/basic.hpp"
#include "wz4frlib/wz4_demo2.hpp"
#include "wz4frlib/fr033_ops.hpp"

/****************************************************************************/

struct Wz3Texture
{
  sString<64> Name;
  Texture2D *Tex;
};

class Wz3TexPackage : public wObject
{
public:
  sArray<Wz3Texture> Textures;

  Wz3TexPackage();
  ~Wz3TexPackage();

  sBool LoadKTX(const sChar *filename,sInt sizeOffset=0);
  Texture2D *Lookup(const sChar *name);
};

/****************************************************************************/

class RNLimitTransform : public Wz4RenderNode
{
public:
  Wz4RenderParaLimitTransform Para,ParaBase;
  Wz4RenderAnimLimitTransform Anim;

  RNLimitTransform();

  void Simulate(Wz4RenderContext *ctx);
  void Transform(Wz4RenderContext *ctx,const sMatrix34 &);
};

/****************************************************************************/

class RNTrembleMesh : public Wz4RenderNode
{
  sAABBoxC Bounds;
public:

  Wz4Mesh *Mesh;
  Wz4RenderParaTrembleMesh Para,ParaBase;
  Wz4RenderAnimTrembleMesh Anim;

  RNTrembleMesh();
  ~RNTrembleMesh();
  void Init();

  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);
};

/****************************************************************************/

#endif // FILE_WZ4FRLIB_FR033_HPP


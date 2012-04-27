/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_EASTER_HPP
#define FILE_WZ4FRLIB_EASTER_HPP

#include "base/types.hpp"
#include "wz4frlib/wz4_demo2.hpp"
#include "wz4frlib/wz4_demo2_ops.hpp"
#include "wz4frlib/wz4_mesh.hpp"
#include "wz4frlib/wz4_mtrl2.hpp"

#include "wz4frlib/easter_ops.hpp"

/****************************************************************************/

class RNCubeTrees : public Wz4RenderNode
{
  sGeometry *TreeGeo, *CubeGeo;
  sBool TreeOk, CubeOk;
  sF32 Time;

  sStaticArray<sMatrix34> CubeMatrices;
  sMatrix34 CubeRotMat;

  void MakeBranch(sInt depth, sInt seed, sF32 time, sF32 width, sVector31 pos, sVector30 dir, sVector30Arg ref, sVertexStandard *&vp, sU16 *&ip, sInt &ic, sInt &vc);

public:

  static const sU32 CubeVertexDecl[];
  static sVertexFormatHandle *CubeVertexFormat;

  struct CubeVertex
  {
    sF32 px,py,pz;                  // 3 position
    sF32 nx,ny,nz;                  // 3 normal
    sF32 tx,ty,tz,tsign;            // 4 tangent
    sF32 u0,v0;                     // 4 uv's (we WILL want a lightmap!)

    volatile inline void Init(const sVector31 &p, const sVector30 &n, const sVector4 &t, sF32 u, sF32 v)
    {
      px=p.x; py=p.y; pz=p.z; nx=n.x; ny=n.y; nz=n.z; tx=t.x; ty=t.y; tz=t.z; tsign=t.w; u0=u; v0=v;
    }

    static sVertexFormatHandle* VertexFormat() {  return CubeVertexFormat;}

  };

  RNCubeTrees();
  ~RNCubeTrees();
  
  Wz4RenderParaCubeTrees ParaBase,Para;
  Wz4RenderAnimCubeTrees Anim;

  sStaticArray<Wz4RenderArrayCubeTrees> Array;

  Wz4Mtrl *TreeMtrl, *CubeMtrl;

  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);
};


/****************************************************************************/

class RNTVNoise : public Wz4RenderNode
{
  sGeometry *Geo;
  sMaterial *Mtrl;
  sF32 Time;

public:
  RNTVNoise();
  ~RNTVNoise();
  
  Wz4RenderParaTVNoise ParaBase,Para;
  Wz4RenderAnimTVNoise Anim;

  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);
};

/****************************************************************************/

class RNJulia4D : public Wz4RenderNode
{
  sGeometry *Geo;
  sMaterial *Pass1Mtrl;
  sMaterial *Pass2Mtrl;
  sMaterial *Pass3Mtrl;
  sF32 Time;

  sTexture2D *DistRT;

public:
  RNJulia4D();
  ~RNJulia4D();
  
  Wz4RenderParaJulia4D ParaBase,Para;
  Wz4RenderAnimJulia4D Anim;

  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);
};

/****************************************************************************/
#endif // FILE_WZ4FRLIB_EASTER_HPP


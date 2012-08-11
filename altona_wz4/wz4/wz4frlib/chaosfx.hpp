/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_CHAOSFX_HPP
#define FILE_WZ4FRLIB_CHAOSFX_HPP

#include "base/types.hpp"
#include "wz4frlib/wz4_demo2.hpp"
#include "wz4frlib/wz4_demo2_ops.hpp"
#include "wz4frlib/wz4_mesh.hpp"
#include "wz4frlib/wz4_mtrl2.hpp"
#include "wz4frlib/chaosfx_ops.hpp"
#include "util/shaders.hpp"

/****************************************************************************/
/****************************************************************************/

class RNCubeExample : public Wz4RenderNode
{
  sGeometry *Geo;                 // geometry holding the cube
  sMatrix34 Matrix;               // calculated in Prepare, used in Render
  void MakeCube();
public:
  RNCubeExample();
  ~RNCubeExample();

  void Simulate(Wz4RenderContext *ctx);   // execute the script. 
  void Prepare(Wz4RenderContext *ctx);    // do simulation
  void Render(Wz4RenderContext *ctx);     // render a pass

  Wz4RenderParaCubeExample Para,ParaBase; // animated parameters from op
  Wz4RenderAnimCubeExample Anim;          // information for the script engine

  Wz4Mtrl *Mtrl;                  // material from inputs
};

/****************************************************************************/

class RNPrint : public Wz4RenderNode
{
public:
  RNPrint();
  ~RNPrint();

  void Simulate(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);

  Wz4RenderParaPrint Para,ParaBase;
  Wz4RenderAnimPrint Anim;
  class ChaosFont *Font;
  sPoolString Text;
};

/****************************************************************************/

class RNRibbons : public Wz4RenderNode
{
  sGeometry *Geo;
  sGeometry *GeoWire;
  sSimpleMaterial *Mtrl;
  
public:
  RNRibbons();
  ~RNRibbons();
  
  Wz4RenderParaRibbons ParaBase,Para;
  Wz4RenderAnimRibbons Anim;

  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);

  Wz4Mtrl *MtrlEx;
};

/****************************************************************************/

class RNRibbons2 : public Wz4RenderNode
{
  sGeometry *Geo;
  sGeometry *GeoWire;
  sSimpleMaterial *Mtrl;

  sRandomMT Random;
  sVector31 Sources[4];
  sF32 Power[4];
  
public:
  RNRibbons2();
  ~RNRibbons2();
  
  Wz4RenderParaRibbons2 ParaBase,Para;
  Wz4RenderAnimRibbons2 Anim;

  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);

  void Eval(const sVector31 &pos,sVector30 &norm);

  Wz4Mtrl *MtrlEx;
};

/****************************************************************************/

#define BNNB 9
#define BNN (1<<BNNB)
#define BNNM (BNN-1)

class RNBlowNoise : public Wz4RenderNode
{
  sVertexFormatHandle *VertFormat;
  sGeometry *Geo;
  sGeometry *GeoWire;
  //sSimpleMaterial *Mtrl;
  
  struct NoiseData
  {
    sF32 Value;
    sF32 DX;
    sF32 DY;
  };

  struct Vertex
  {
    sVector31 Pos;
    sVector31 PosOld;
    sVector30 Normal;
    sVector30 Tangent;
    sF32 U,V;
  };

  Vertex *Verts;
  sInt SizeX;
  sInt SizeY;
  NoiseData Noise[BNN][BNN];
  void InitNoise();
  void SampleNoise(sF32 x,sF32 y,NoiseData &nd);

  ScriptSymbol *_Scale;
  ScriptSymbol *_Scroll;

public:
  RNBlowNoise();
  ~RNBlowNoise();
  void Init();

  sArray<struct Wz4RenderArrayBlowNoise *> Layers;
  Wz4RenderParaBlowNoise Para,ParaBase;
  Wz4RenderAnimBlowNoise Anim;
  Wz4Mtrl *Mtrl;

  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);
};

/****************************************************************************/

class RNDebrisClassic : public Wz4RenderNode
{
public:
  RNDebrisClassic();
  ~RNDebrisClassic();
  void Init();

  Wz4RenderParaDebrisClassic Para,ParaBase;
  Wz4RenderAnimDebrisClassic Anim;
  Wz4Mesh *Mesh;
  sImageI16 Bitmap[3];

  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx) { if(Mesh) Mesh->BeforeFrame(Para.LightEnv); }
  void Render(Wz4RenderContext *ctx);
};

/****************************************************************************/

class RNBoneVibrate : public Wz4RenderNode
{
  sInt Count;
  sMatrix34 *mate;
  sMatrix34 *mats;
  sMatrix34CM *mats0;
public:
  RNBoneVibrate();
  ~RNBoneVibrate();
  void Init();

  void Simulate(Wz4RenderContext *ctx);   // execute the script. 
  void Prepare(Wz4RenderContext *ctx);    // do simulation
  void Render(Wz4RenderContext *ctx);     // render a pass

  Wz4RenderParaBoneVibrate Para,ParaBase; // animated parameters from op
  Wz4RenderAnimBoneVibrate Anim;          // information for the script engine

  Wz4Mesh *Mesh;                  // material from inputs
};

/****************************************************************************/
/****************************************************************************/


#endif // FILE_WZ4FRLIB_CHAOSFX_HPP


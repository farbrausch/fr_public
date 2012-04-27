/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_WZ4_DEMO2NODES_HPP
#define FILE_WZ4FRLIB_WZ4_DEMO2NODES_HPP

#include "base/types.hpp"
#include "wz4frlib/wz4_demo2.hpp"
#include "wz4frlib/wz4_demo2_ops.hpp"

/****************************************************************************/

class RNCamera : public Wz4RenderNode
{
public:
  RNCamera();
  ~RNCamera();

  ScriptSymbol *Symbol;
  Wz4RenderParaCamera Para,ParaBase;
  Wz4RenderAnimCamera Anim;
  Rendertarget2D *Target;

  void Init(const sChar *splinename);

  void Simulate(Wz4RenderContext *ctx);
};

/****************************************************************************/

class RNBillboardCamera : public Wz4RenderNode
{
  sBool AllowRender;
public:
  RNBillboardCamera();
  ~RNBillboardCamera();

  Wz4RenderParaBillboardCamera Para,ParaBase;
  Wz4RenderAnimBillboardCamera Anim;
  Rendertarget2D *Target;

  void Init(const sChar *splinename);

  void Simulate(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);
};

/****************************************************************************/

class RNRenderMesh : public Wz4RenderNode
{
public:
  RNRenderMesh();
  ~RNRenderMesh();

  Wz4RenderParaRenderMesh Para,ParaBase;
  Wz4RenderAnimRenderMesh Anim;
  Wz4Mesh *Mesh;

  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);
};

/****************************************************************************/

class RNMultiplyMesh : public Wz4RenderNode
{
  ScriptSymbol *_S[4];
  ScriptSymbol *_R[4];
  ScriptSymbol *_T[4];
  ScriptSymbol *_Count[4];

  sMatrix34 Mat[4];

  sArray<sMatrix34CM> Mats;
public:
  RNMultiplyMesh();
  ~RNMultiplyMesh();

  Wz4RenderParaMultiplyMesh Para,ParaBase;
  Wz4RenderAnimMultiplyMesh Anim;
  Wz4Mesh *Mesh;

  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);
};

/****************************************************************************/

class RNAdd : public Wz4RenderNode
{
public:
  RNAdd();

  void Simulate(Wz4RenderContext *ctx);
};

/****************************************************************************/

class RNTransform : public Wz4RenderNode
{
public:
  Wz4RenderParaTransform Para,ParaBase;
  Wz4RenderAnimTransform Anim;

  RNTransform();

  void Simulate(Wz4RenderContext *ctx);
  void Transform(Wz4RenderContext *ctx,const sMatrix34 &);
};

/****************************************************************************/

class RNTransformPivot : public Wz4RenderNode
{
public:
  Wz4RenderParaTransformPivot Para,ParaBase;
  Wz4RenderAnimTransformPivot Anim;

  RNTransformPivot();

  void Simulate(Wz4RenderContext *ctx);
  void Transform(Wz4RenderContext *ctx,const sMatrix34 &);
};

/****************************************************************************/

class RNSplinedObject : public Wz4RenderNode
{
  ScriptSymbol *SplineSymbol;
  ScriptSpline *Spline;
public:
  Wz4RenderParaSplinedObject Para,ParaBase;
  Wz4RenderAnimSplinedObject Anim;

  RNSplinedObject();

  void Init(const sChar *splinename);
  void Simulate(Wz4RenderContext *ctx);
  void Transform(Wz4RenderContext *ctx,const sMatrix34 &);
};

/****************************************************************************/

class RNLookAt : public Wz4RenderNode
{
public:
  Wz4RenderParaLookAt Para,ParaBase;
  Wz4RenderAnimLookAt Anim;

  RNLookAt();

  void Simulate(Wz4RenderContext *ctx);
  void Transform(Wz4RenderContext *ctx,const sMatrix34 &);
};

/****************************************************************************/

class RNSkyCube : public Wz4RenderNode
{
public:
  Wz4RenderParaSkyCube Para,ParaBase;
  Wz4RenderAnimSkyCube Anim;

  RNSkyCube();

  void Simulate(Wz4RenderContext *ctx);
  void Transform(Wz4RenderContext *ctx,const sMatrix34 &);
};

/****************************************************************************/

class RNShaker : public Wz4RenderNode
{
public:
  Wz4RenderParaShaker Para,ParaBase;
  Wz4RenderAnimShaker Anim;

  RNShaker();

  void Simulate(Wz4RenderContext *ctx);
};

/****************************************************************************/

class RNMultiply : public Wz4RenderNode
{
public:
  Wz4RenderParaMultiply Para,ParaBase;
  Wz4RenderAnimMultiply Anim;

  RNMultiply();

  void Simulate(Wz4RenderContext *ctx);
  void Transform(Wz4RenderContext *ctx,const sMatrix34 &);
};

/****************************************************************************/

class RNClip : public Wz4RenderNode
{
public:
  Wz4RenderParaClip Para,ParaBase;
  Wz4RenderAnimClip Anim;
  sBool Active;

  RNClip();
  void Init();

  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);
  void Transform(Wz4RenderContext *ctx,const sMatrix34 &);
};

/****************************************************************************/

class RNClipRand : public Wz4RenderNode
{
public:
  Wz4RenderParaClipRandomizer Para,ParaBase;
  Wz4RenderAnimClipRandomizer Anim;
  sBool Active;
  sInt ActiveClip;
  struct clip
  {
    sF32 Start;
    sF32 Length;
    Wz4RenderNode *Child;
    sF32 Sort;
  };
  sArray<clip> Clips;

  RNClipRand();
  void Init();

  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);
  void Transform(Wz4RenderContext *ctx,const sMatrix34 &);
};

/****************************************************************************/

class RNMultiClip : public Wz4RenderNode
{
public:
  Wz4RenderParaMultiClip Para,ParaBase;
  sArray<Wz4RenderArrayMultiClip> Clips;
  sBool Active;

  RNMultiClip();
  void Init(const Wz4RenderParaMultiClip &,sInt count,const Wz4RenderArrayMultiClip *arr);

  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);
};

/****************************************************************************/

class RNFader : public Wz4RenderNode
{
  ScriptSymbol *_Fader;
  ScriptSymbol *_Rotor;
public:
  Wz4RenderParaFader Para,ParaBase;

  RNFader();

  void Simulate(Wz4RenderContext *ctx);
};

/****************************************************************************/

class RNVariable: public Wz4RenderNode
{
  ScriptSymbol *_Symbol;
  sInt Count;
  sVector4 Value;
public:
  RNVariable();
  void Init(Wz4RenderParaVariable *para,const sChar *name);

  void Simulate(Wz4RenderContext *ctx);
};

/****************************************************************************/

class RNSpline : public Wz4RenderNode
{
  sPoolString Name;
public:
  RNSpline();
  ~RNSpline();
  void Init(Wz4RenderParaSpline *,sInt ac,Wz4RenderArraySpline *,const sChar *name);

  void Simulate(Wz4RenderContext *ctx);
  ScriptSpline *Spline;
};

/****************************************************************************/

class RNLayer2D : public Wz4RenderNode
{
  class Layer2dMtrl *Mtrl;
  sGeometry *Geo;
public:
  RNLayer2D();
  ~RNLayer2D();
  void Init();

  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);

  Wz4RenderParaLayer2D Para,ParaBase;
  Wz4RenderAnimLayer2D Anim;
  sF32 DocScreenY;

  Texture2D *Texture[2];
};

/****************************************************************************/

enum RNBeatBits
{
  RNB_LevelMask   = 0x0f,         // up to 16 different values

  RNB_LoopMask    = 0xc0,         // loops:
  RNB_LoopStart   = 0x40,         // first bit of the loop
  RNB_LoopEnd     = 0x80,         // first bit after the loop, where looping starts
  RNB_LoopDone    = 0xc0,         // from here on looping is disabled again
};

class RNBeat : public Wz4RenderNode
{
  sPoolString Name;
/*
  sInt Length;
  sInt Speed;
  sInt Levels;                    // number of values
  sU8 *Beat;                      // value + loopbits
  sU8 *LoopedBeat;                // value only
  sF32 Level[16];                // levels for the 16 values
  */
public:
  RNBeat();
  ~RNBeat();
  void Init(Wz4RenderParaBeat *,sInt ac,Wz4RenderArrayBeat *,const sChar *name);

  void Simulate(Wz4RenderContext *ctx);

  ScriptSpline *Spline;
  sF32 Amp;
  sF32 Bias;
};

/****************************************************************************/

class RNBoneTrain : public Wz4RenderNode
{
  sInt Count;
//  sMatrix34 *mate;
  sMatrix34 *mats;
  sMatrix34CM *mats0;
  ScriptSymbol *Symbol;
public:
  RNBoneTrain();
  ~RNBoneTrain();
  void Init(const sChar *name);

  void Simulate(Wz4RenderContext *ctx);   // execute the script. 
  void Prepare(Wz4RenderContext *ctx);    // do simulation
  void Render(Wz4RenderContext *ctx);     // render a pass

  Wz4RenderParaBoneTrain Para,ParaBase; // animated parameters from op
  Wz4RenderAnimBoneTrain Anim;          // information for the script engine

  Wz4Mesh *Mesh;                  // material from inputs
};

/****************************************************************************/

#endif // FILE_WZ4FRLIB_WZ4_DEMO2NODES_HPP


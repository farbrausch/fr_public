/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_FR063_CHAOS_HPP
#define FILE_WZ4FRLIB_FR063_CHAOS_HPP

#include "base/types.hpp"
#include "wz4frlib/wz4_demo2.hpp"
#include "wz4frlib/wz4_demo2_ops.hpp"
#include "wz4frlib/wz4_mesh.hpp"
#include "wz4frlib/wz4_mtrl2.hpp"
#include "wz4frlib/fr063_chaos_ops.hpp"

#include "extra/mcubes.hpp"

#include "util/shaders.hpp"

/****************************************************************************/


struct WaterParticle
{
  sVector31 NewPos;
  sVector31 OldPos;
  sU32 Color;
  sU32 Hash;
};

class WaterFX
{
  sRandom Rnd;

  struct HashIndex
  {
    sInt Start;
    sInt Count;
  };
  enum Constants
  {
    HASHSIZE = 0x400,
    HASHMASK = HASHSIZE-1,

    STEPX = 1,
    STEPY = 17,
    STEPZ = 23,

    BATCH = 16,
  };

  struct counterforcevector
  {
    sVector30 f;
    sU32 pad[5];
  };

  HashIndex HashTable[HASHSIZE];
  sArray<WaterParticle> *Parts[2];
  sVector4 Planes[4];
  sArray<counterforcevector> CounterForce;
  sVector30 LastCounterForce;

public:
  WaterFX();
  ~WaterFX();
  const sArray<WaterParticle> &GetArray() const { return *Parts[0]; }
  sArray<WaterParticle> &GetArray() { return *Parts[0]; }
  sInt GetCount() const { return Parts[0]->GetCount(); }
  void Reset();
  void AddRain(sInt count,sU32 color);
  void AddDrop(sInt count,sU32 color,sF32 radius);
  void Nudge(const sVector30 &speed);
  void Step(class sStsManager *sched,class sStsWorkload *wl);

  void Func(sInt n,sInt threadid);

  sF32 GravityY;
  sF32 CentralGravity;
  sF32 OuterForce;
  sF32 InnerForce;
  sF32 InteractRadius;
  sF32 Friction;
};

class RNFR063_Water : public Wz4RenderNode
{
  MarchingCubes *MC;
  sArray<sVector31> MCParts;
  class WaterFX *Water;
  class WaterFX *Drop;
  sInt Step;
public:
  RNFR063_Water();
  ~RNFR063_Water();
  void Init();

  void Simulate(Wz4RenderContext *ctx);   // execute the script. 
  void Prepare(Wz4RenderContext *ctx);    // do simulation
  void Render(Wz4RenderContext *ctx);     // render a pass

  Wz4RenderParaFR063_Water Para,ParaBase; // animated parameters from op
  Wz4RenderAnimFR063_Water Anim;          // information for the script engine

  Wz4Mtrl *Mtrl;                          // material from inputs
};


/****************************************************************************/

class RNFR063_MultiProgress : public Wz4RenderNode
{
  sGeometry *Geo;
  sSimpleMaterial *Mtrl;
public:
  RNFR063_MultiProgress();
  ~RNFR063_MultiProgress();
  void Init();

  void Simulate(Wz4RenderContext *ctx);   // execute the script. 
  void Prepare(Wz4RenderContext *ctx);    // do simulation
  void Render(Wz4RenderContext *ctx);     // render a pass

  Wz4RenderParaFR063_MultiProgress Para,ParaBase; // animated parameters from op
  Wz4RenderAnimFR063_MultiProgress Anim;          // information for the script engine

//  Wz4Mtrl *Mtrl;                          // material from inputs
};


/****************************************************************************/

#endif // FILE_WZ4FRLIB_FR063_CHAOS_HPP


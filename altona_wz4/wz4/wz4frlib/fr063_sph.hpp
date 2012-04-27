/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_FR063_SPH_HPP
#define FILE_WZ4FRLIB_FR063_SPH_HPP

#include "wz4frlib/fr063_sph_ops.hpp"
#include "wz4frlib/wz4_demo2.hpp"
#include "wz4frlib/wz4_demo2_ops.hpp"

enum Constants
{
  HASHSIZE = 0x400,
  HASHMASK = HASHSIZE-1,

  STEPX = 1,
  STEPY = 17,
  STEPZ = 23,

  BATCH = 0x1000000,
  MAXNEAR = 512,
};
/****************************************************************************/

class RPSPH;
class SphGenerator : public wObject
{
public: 
  SphGenerator();
  ~SphGenerator();

  virtual void Reset()=0;
  virtual void SimPart(RPSPH *)=0;
};

class SphGenObject : public SphGenerator
{
  class Wz4BSP *Bsp;
public:
  SphGeneratorParaSphObject Para;

  SphGenObject();
  ~SphGenObject();
  void Init(class Wz4Mesh *in0);
  void Reset();
  void SimPart(RPSPH *);
};

class SphGenSource : public SphGenerator
{
  sRandomKISS Rnd;
  sMatrix34 Matrix;
public:
  SphGeneratorParaSphSource Para;

  SphGenSource();
  void Init();
  void Reset();
  void SimPart(RPSPH *);
};

/****************************************************************************/

class SphCollision : public wObject
{
public:
  SphCollision();
  ~SphCollision();

  virtual void Init()=0;
  virtual void CollPart(RPSPH *)=0;
};

class SphCollSimple : public SphCollision
{
public:
  SphCollisionParaSphSimpleColl Para;

  void Init();
  void CollPart(RPSPH *);
};

class SphCollPlane : public SphCollision
{
public:
  SphCollisionParaSphPlaneColl Para;

  void Init();
  void CollPart(RPSPH *);
};

class SphCollShock : public SphCollision
{
public:
  SphCollisionParaSphShock Para;

  void Init();
  void CollPart(RPSPH *);
};

/****************************************************************************/

class RPSPH : public Wz4ParticleNode
{
  friend static void InterT(sStsManager *,sStsThread *thread,sInt start,sInt count,void *data);
  void Hash();
  void Inter(sInt n0,sInt n1);
  void Physics();

public:
  RPSPH();
  ~RPSPH();
  void Init();
  void Reset();
  void SimPart();

  sArray<SphGenerator *> Gens;
  sArray<SphCollision *> Colls;

  Wz4ParticlesParaSphSimulator Para,ParaBase;
  Wz4ParticlesAnimSphSimulator Anim;

  void Simulate(Wz4RenderContext *ctx);
  sInt GetPartCount();
  sInt GetPartFlags();
  void Func(Wz4PartInfo &pinfo,sF32 time,sF32 dt);

  struct Particle
  {
    sVector31 NewPos;
    sVector31 OldPos;
    sU32 Color;
    sU32 Hash;
  };

  struct Spring
  {
    sInt Part0;
    sInt Part1;
    sF32 RestLength;
  };

  struct HashIndex
  {
    sInt Start;
    sInt Count;
  };

  sInt SimStep;
  sF32 LastTimeF;
  sF32 CurrentTime;
  sF32 SimTimeStep;
//  sArray<Particle> Parts;

  HashIndex HashTable[HASHSIZE];
  sArray<Particle> *Parts[2];
  sArray<Spring> Springs;
};

/****************************************************************************/



/****************************************************************************/

#endif // FILE_WZ4FRLIB_FR063_SPH_HPP


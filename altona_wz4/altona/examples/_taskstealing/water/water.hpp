/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WATER_WATER_HPP
#define FILE_WATER_WATER_HPP

#include "base/types2.hpp"
#include "base/math.hpp"

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

    BATCH = 0x1000000,
    MAXNEAR = 512,
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

/****************************************************************************/

#endif // FILE_WATER_WATER_HPP



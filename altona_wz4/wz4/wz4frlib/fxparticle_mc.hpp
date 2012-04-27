/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_FXPARTICLE_MC_HPP
#define FILE_WZ4FRLIB_FXPARTICLE_MC_HPP

#include "base/types.hpp"
#include "wz4frlib/wz4_demo2.hpp"
#include "wz4frlib/wz4_demo2_ops.hpp"
#include "wz4frlib/wz4_mesh.hpp"
#include "wz4frlib/wz4_mtrl2.hpp"
#include "wz4frlib/fxparticle_ops.hpp"
#include "util/shaders.hpp"
#include "extra/mcubes.hpp"

/****************************************************************************/

// template<class PartType,class SimdType,class FieldType>

struct RNMarchingCubesTemplate
{
  static const sInt Color = 0;
  struct SimdType
  {
    __m128 x,y,z;
    static __m128 cr,cg,cb;
  };

  struct PartType
  {
    sF32 x,y,z;
    static sU32 c;
  };

  typedef MCPotField FieldType;
  typedef Wz4RenderParaMarchingCubes OpParaType;
  typedef Wz4RenderAnimMarchingCubes OpAnimType;
};

/****************************************************************************/

struct RNMarchingCubesColorTemplate
{
  static const sInt Color = 1;
  struct SimdType
  {
    __m128 x,y,z;
    __m128 cr,cg,cb;
  };

  struct PartType
  {
    sF32 x,y,z;
    sU32 c;
  };

  typedef MCPotFieldColor FieldType;
  typedef Wz4RenderParaMarchingCubesColor OpParaType;
  typedef Wz4RenderAnimMarchingCubesColor OpAnimType;
};

/****************************************************************************/

template<class T>
class RNMarchingCubesBase : public Wz4RenderNode
{
public:
  enum RNMarchingCubesConsts
  {
    ContainerSize = 64,
  //  CellSize = 8,
    HashSize = (1<<10),
    HashMask = (HashSize-1),
  };


  struct funcinfo
  {
    __m128 tresh4;
    __m128 treshf4;
    __m128 one;
    __m128 epsilon;
    typename T::SimdType *parts4;
    sInt pn4;

    sF32 iso;
    sF32 tresh;
    sF32 treshf;
    typename T::PartType *parts;
    sInt pn;
  };
private:

  // particle system interface

  Wz4PartInfo PInfo;
  sF32 Time;
  MarchingCubesHelper MC;

  sInt MaxThread;

  typename T::FieldType **PotData;
  sInt PotSize;

  typename T::SimdType **SimdParts;
  sInt SimdCount;

  sStsWorkload *Workload;

  // hashing

  struct PartContainer
  {
    PartContainer *Next;
    sInt Count;
    typename T::PartType Parts[ContainerSize];
  };
  struct HashContainer
  {
    HashContainer *Next;
    PartContainer *FirstPart;
    sInt IX,IY,IZ;
  };

  sArray<HashContainer *> FreeHashConts;  // currently free containers
  sArray<HashContainer *> NodeHashConts;  // for each gridcube, the first container in list
  sArray<HashContainer *> ThreadHashConts;// for each gridcube, the first container in list

  sArray<PartContainer *> AllPartContBlocks;  // i allocate the containers in blocks of 1024, because i need so many of them

  sArray<PartContainer *> FreePartConts;  // currently free containers
  sArray<PartContainer *> NodePartConts;  // particle containers in NodeConts list
  sArray<PartContainer *> ThreadPartConts;//

  HashContainer *HashTable[HashSize];

  // GeoBuffers

  GeoBufferHelper Geos;
  GeoBufferHelper::GeoBuffer *CurrentGeo;

  // functions

  HashContainer *GetHashContainer();
  PartContainer *GetPartContainer();

  static void func(const sVector31 &v,typename T::FieldType &pot,const funcinfo &fi);
  void Spatial();
public:
  template <int base,int subdiv> void RenderT(sInt start,sInt count,sInt thread);
private:
  void Render();
  

public:
  RNMarchingCubesBase();
  ~RNMarchingCubesBase();
  void Init();
  
  typename T::OpParaType Para,ParaBase;
  typename T::OpAnimType Anim;

  Wz4ParticleNode *Source;
  Wz4Mtrl *Mtrl;

  void Simulate(Wz4RenderContext *ctx);
  void Prepare(Wz4RenderContext *ctx);
  void Render(Wz4RenderContext *ctx);
};

/****************************************************************************/

typedef RNMarchingCubesBase<RNMarchingCubesTemplate> RNMarchingCubes;
typedef RNMarchingCubesBase<RNMarchingCubesColorTemplate> RNMarchingCubesColor;

/****************************************************************************/

#endif // FILE_WZ4FRLIB_FXPARTICLE_MC_HPP


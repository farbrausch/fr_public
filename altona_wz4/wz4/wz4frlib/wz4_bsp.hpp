/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_WZ4_BSP_HPP
#define FILE_WZ4FRLIB_WZ4_BSP_HPP

#include "base/types.hpp"
#include "base/types2.hpp"
#include "base/math.hpp"
#include "wz4frlib/wz4_mesh.hpp"

/****************************************************************************/

enum Wz4BSPPolyClass
{
  WZ4BSP_BACK = 0,
  WZ4BSP_ON = 1,
  WZ4BSP_FRONT = 2,
  WZ4BSP_STRADDLING = 3,
};

enum Wz4BSPError
{
  WZ4BSP_OK = 0,
  WZ4BSP_NONPLANAR_INPUT = 1,
  WZ4BSP_NONPLANAR_POLY = 2,
  WZ4BSP_DEGENERATE_POLY = 3,
  WZ4BSP_CANT_HIT_CLIP_PLANE = 4,
  WZ4BSP_CANT_HIT_BOTH_PLANES = 5,
  WZ4BSP_ROUNDOFF_PROBLEM = 6,
  WZ4BSP_WHAT_THE_FUCK = 7,
};

const sChar *Wz4BSPGetErrorString(Wz4BSPError err);

struct Wz4BSPFace
{
  sInt Count;           // # of points
  sInt RefCount;
  sVector4 Plane;
  sVector31 Points[1];  // must be last! (variable size)

  static Wz4BSPFace *Alloc(sInt count);
  static void Free(Wz4BSPFace *face);

  static Wz4BSPFace *MakeTri(const sVector31 &a,const sVector31 &b,const sVector31 &c);
  static Wz4BSPFace *MakeQuad(const sVector31 &a,const sVector31 &b,const sVector31 &c,const sVector31 &d);
  static Wz4BSPFace *CleanUp(Wz4BSPFace *in);
  Wz4BSPFace *MakeCopy() const;

  void AddRef() { RefCount++; }
  void Release() { if(--RefCount==0) Free(this); }

  sF32 CalcArea() const;
  void CalcPlane();
  Wz4BSPError Split(const sVector4 &plane,Wz4BSPFace *&front,Wz4BSPFace *&back,sF32 eps) const;

  // for polyhedral mass properties (Wz4BSPPolyhedron::CalcMassProperties)
  void CalcFaceIntegrals(sInt gamma,sVector30 &f,sVector30 &f2,sVector30 &f3,sVector30 &fmix) const;

  Wz4BSPPolyClass Classify(const sVector4 &plane,sF32 eps) const;
  sVector4 GetPlane() const;
  sBool IsPlanar(sF32 eps) const;
  
  void AddToBBox(sAABBox &box) const;
};

struct Wz4BSPPolyhedron
{
  sArray<Wz4BSPFace*> Faces;

  Wz4BSPPolyhedron();
  Wz4BSPPolyhedron(const Wz4BSPPolyhedron &x);
  ~Wz4BSPPolyhedron();

  Wz4BSPPolyhedron& operator =(const Wz4BSPPolyhedron &x);

  void Clear();
  void InitBBox(const sAABBox &box);
  void InitTetrahedron(const sVector31 &o,const sVector31 &x,const sVector31 &y,const sVector31 &z);
  void Swap(Wz4BSPPolyhedron &x);

  void CalcBBox(sAABBox &box) const;
  void CalcCentroid(sVector31 &centroid) const;
  sF32 CalcArea() const;
  sF32 CalcVolume() const;
  void FindCloserVertex(const sVector31 &target,sVector31 &closest,sF32 closestDistSq) const;
  Wz4BSPError Split(const sVector4 &plane,Wz4BSPPolyhedron &front,Wz4BSPPolyhedron &back,sF32 eps) const;
  void GenMesh(Wz4Mesh *mesh,sF32 explode,const sVector30 &shift) const;
  void Dump() const; // debug

  void CalcMassProperties(sF32 &vol,sVector31 &cog,sVector30 &inertD,sVector30 &inertOD) const;

  Wz4BSPPolyClass Classify(const sVector4 &plane,sF32 eps) const;
  sBool IsEmpty() const { return Faces.GetCount() == 0; }
};

struct Wz4BSPNode
{
  static const sInt Inner = 0;
  static const sInt Empty = 1;
  static const sInt Solid = 2;

  Wz4BSPNode(sInt type=Inner);
  ~Wz4BSPNode();

  sVector4 Plane;
  sInt Type; // Inner/Empty/Solid
  sAABBox Bounds;
  Wz4BSPNode *Child[2]; // back, front
};

class Wz4BSP : public wObject
{
  Wz4BSPNode *Root;
  sAABBox Bounds; // in internal coordinate system

  sVector30 CenterPos; // in world coordinate system
  
  sF32 PlaneThickness;
  Wz4BSPError ErrorCode;

  sF32 RandomPlaneProb;
  sF32 MaxVolume,MinVolume;

  sVector4 PickSplittingPlane(const sArray<Wz4BSPFace*> &faces,sRandomMT &rand);
  Wz4BSPNode *BuildTreeR(sArray<Wz4BSPFace*> &splitFaces,sRandomMT &rand);
  Wz4BSPNode *CopyTreeR(Wz4BSPNode *node);
  Wz4BSPNode *SplitTreeR(Wz4BSPNode *node,const sAABBox &box,const sVector4 &plane,sInt side,sInt &splitSolids);
  Wz4BSPNode *SplitTree(Wz4BSPNode *root,const sVector4 &plane,sInt &splitSolids);

  void GeneratePolyhedronsR(Wz4BSPNode *node,const Wz4BSPPolyhedron &base,Wz4Mesh *out,sF32 explode);
  void CalcBoundsR(Wz4BSPNode *node,const sAABBox &box);
  Wz4BSPNode *MakeRandomSplitsR(Wz4BSPNode *node,sRandomMT &rand,const Wz4BSPPolyhedron &poly,sInt &maxSplits);

public:
  Wz4BSP();
  ~Wz4BSP();
  void CopyFrom(Wz4BSP *bsp);

  void MakePolyhedron(sInt nFaces,sInt nIter,sF32 power,sBool dualize,sInt seed);
  Wz4BSPError FromMesh(const Wz4Mesh *mesh,sF32 planeThickness);
  void MakeRandomSplits(sF32 randomPlaneProb,sF32 minEdge,sF32 maxEdge,sInt seed,sInt maxSplits);
  Wz4BSPError GeneratePolyhedrons(Wz4Mesh *mesh,sF32 explode);

  sBool TraceRay(const sRay &ray,sF32 tMin,sF32 tMax,sF32 &tHit,sVector30 &hitNormal);
  sBool IsInside(const sVector31 &pos);
};


/****************************************************************************/

#endif // FILE_WZ4FRLIB_WZ4_BSP_HPP


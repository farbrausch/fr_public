// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once

#include "_types.hpp"
#include "doc.hpp"

/****************************************************************************/
/****************************************************************************/

struct GenMinVector
{
  sF32 x,y,z;
  void Init()                                             { x=y=z=0; }
  void Init(sF32 X,sF32 Y,sF32 Z)                         { x=X;       y=Y;       z=Z; }
  void Init(const sVector &a)                             { x=a.x;     y=a.y;     z=a.z; }
  void Out(sVector &a,sF32 w)                             { a.x=x;     a.y=y;     a.z=z; a.w=w; }
  void Add(const GenMinVector &a)                         { x+=a.x;    y+=a.y;    z+=a.z; }
  void Sub(const GenMinVector &a)                         { x-=a.x;    y-=a.y;    z-=a.z; }
  void Add(const GenMinVector &a,const GenMinVector &b)   { x=a.x+b.x; y=a.y+b.y; z=a.z+b.z; }
  void Sub(const GenMinVector &a,const GenMinVector &b)   { x=a.x-b.x; y=a.y-b.y; z=a.z-b.z; }
  void AddScale(const GenMinVector &a,sF32 s)             { x+=a.x*s;  y+=a.y*s;  z+=a.z*s; }
  sF32 Dot(const GenMinVector &a) const                   { return x*a.x + y*a.y + z*a.z; }
  void UnitSafe();
  void Cross(const GenMinVector &a,const GenMinVector &b);
};

/****************************************************************************/

#define KMM_MAXBONE   4           // boneweights per vertex
#define KMM_MAXUV     2           // uv sets per vertext
#define KMM_MAXVERT   8           // vertices per face

struct GenMinVert                 // a vertex
{
  sU8 Select;                     // selection flag
  sU8 BoneCount;                  // bones used
  sU8 pad0;
  sU8 pad1;
  sU32 Color;                     // vertex color

  GenMinVector Pos;               // position
  GenMinVector Normal;            // normal
  GenMinVector Tangent;           // tangent for t-space
  sF32 UV[KMM_MAXUV][2];          // uv-sets
  sF32 Weights[KMM_MAXBONE];      // bone weights
  sU16 Matrix[KMM_MAXBONE];       // bone matrix index

  sBool IsSame(const GenMinVert &v,sBool uv);
};

struct GenMinFace                 // a face
{
  sU8 Select;                     // selection flag
  sU8 Count;                      // vertex per face count
  sU8 Cluster;                    // material index of this face. 0 is deleted
  sU8 pad1;
  GenMinVector Normal;            // face normal, for CalcNormals();
  sInt Vertices[KMM_MAXVERT]; 
  sInt Adjacent[KMM_MAXVERT];     // face*8+vertIndex
};

struct GenMinCluster              // a cluster of faces with same material
{
  class GenMaterial *Mtrl;        // link to a KObject 
  sInt RenderPass;
  void Init(GenMaterial *mtrl,sInt pass=0) { Mtrl=mtrl; RenderPass=pass; }
};

/****************************************************************************/

enum GenMinMeshSelectMode
{
  MMS_ADD       = 0x0000,
  MMS_SUB       = 0x0001,
  MMS_SET       = 0x0002,
  MMS_SETNOT    = 0x0003,

  MMS_VERTEX    = 0x0000,
  MMS_FULLFACE  = 0x0004,
  MMS_PARTFACE  = 0x0008,
};

enum GenMinMeshSelectUse
{
  MMU_ALL         = 0,
  MMU_SELECTED    = 1,
  MMU_UNSELECTED  = 2,
};

class GenMinMesh : public GenObject
{
  sInt ChangeFlag;
  sInt NormalsOK;

  // hashing
  static sU32 HashVector(const GenMinVector &v);

public:
  class EngMesh *PreparedMesh;

#if !sPLAYER
  class EngMesh *WireMesh;
  sInt WireFlags;
  sU32 WireSelMask;
#endif

  GenMinMesh();
  ~GenMinMesh();
  sInt RefCount;
  void AddRef();
  void Release();
  void Copy(GenMinMesh *);
  GenMinMesh *Copy();
  void Clear();

  void ChangeGeo();
  void ChangeTopo();

  // structure

  sArray<GenMinVert> Vertices;
  sArray<GenMinFace> Faces;
  sArray<GenMinCluster> Clusters;

  // utilities

  GenMinVert *MakeGrid(sInt x,sInt y,sInt flags);      //flags: 1 add top point, 2 add bottom point, 4=stitch tx, 8=stitch ty
  sInt AddCluster(class GenMaterial *,sInt pass);

  void Transform(sInt sel,const sMatrix &mat,sInt src=0,sInt dest=0);  // transform position, uv0, uv1
  void Merge();                                 // merge vertices at same position.
  void CalcNormals();                           // calculate normal and tangent
  void Add(GenMinMesh *mesh);
  void Invert();
  void CalcBBox(sAABox &box) const;

  void SelectAll(sInt in,sInt out);

  sInt *CalcMergeVerts() const;

  void CalcAdjacency();
  void VerifyAdjacency();

//  void Prepare() {}
//  void UnPrepare() {}

#if !sPLAYER
  void PrepareWire(sInt flags,sU32 selMask) ;
  void UnPrepareWire();
#endif
};

/****************************************************************************/

// generators

GenMinMesh * __stdcall MinMesh_SingleVert();
GenMinMesh * __stdcall MinMesh_Grid(sInt mode,sInt tesu,sInt tesv);
GenMinMesh * __stdcall MinMesh_Cube(sInt tx,sInt ty,sInt tz,sInt flags,sFSRT srt);
GenMinMesh * __stdcall MinMesh_Torus(sInt x,sInt y,sF32 ro,sF32 ri,sF32 phase,sF32 arclen,sInt flags);
GenMinMesh * __stdcall MinMesh_Sphere(sInt x,sInt y);
GenMinMesh * __stdcall MinMesh_Cylinder(sInt tx,sInt ty,sInt,sInt tz);
GenMinMesh * __stdcall MinMesh_XSI(sChar *filename);

// special

GenMinMesh * __stdcall MinMesh_MatLink(GenMinMesh *mesh,GenMaterial *mtrl,sInt mask,sInt pass);
GenMinMesh * __stdcall MinMesh_Add(sInt count,GenMinMesh *msh,...);
GenMinMesh * __stdcall MinMesh_SelectAll(GenMinMesh *msh,sU32 mode);
GenMinMesh * __stdcall MinMesh_SelectCube(GenMinMesh *msh,sInt dmode,sF323 center,sF323 size);
GenMinMesh * __stdcall MinMesh_DeleteFaces(GenMinMesh *msh);
GenMinMesh * __stdcall MinMesh_Invert(GenMinMesh *msh);

// geometry modifiers

GenMinMesh * __stdcall MinMesh_TransformEx(GenMinMesh *msh,sInt mask,sFSRT srt);
GenMinMesh * __stdcall MinMesh_Displace(GenMinMesh *msh,class GenBitmap *bmp,sInt mask,sF323 ampli);
GenMinMesh * __stdcall MinMesh_Perlin(GenMinMesh *msh,sInt mask,sFSRT srt,sF323 ampl);
GenMinMesh * __stdcall MinMesh_ExtrudeNormal(GenMinMesh *msh,sInt mask,sF32 distance);
GenMinMesh * __stdcall MinMesh_Bend2(GenMinMesh *msh,sF323 center,sF323 rotate,sF32 len,sF32 axis);

// hack
GenMinMesh * __stdcall MinMesh_MTetra(sF32 isoValue);

// topology modifiers

GenMinMesh * __stdcall MinMesh_Triangulate(GenMinMesh *msh,sInt mask,sInt threshold,sInt type);
GenMinMesh * __stdcall MinMesh_Subdivide(GenMinMesh *msh,sInt mask,sF32 alpha,sInt count);
GenMinMesh * __stdcall MinMesh_Extrude(GenMinMesh *mesh,sInt smask,sInt dmask,sInt mode,sInt count,sF323 dist,sF323 s,sF323 r);

/****************************************************************************/
/****************************************************************************/

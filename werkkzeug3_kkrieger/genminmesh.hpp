// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once
#include "kdoc.hpp"
#include "engine.hpp"

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
  sF32 Len() const                                        { return sFSqrt(x*x + y*y + z*z); }
  void UnitSafe();
  void Cross(const GenMinVector &a,const GenMinVector &b);
  sInt Classify();
};

/****************************************************************************/

#define KMM_MAXBONE   4           // boneweights per vertex
#define KMM_MAXUV     2           // uv sets per vertext
#define KMM_MAXVERT   8           // vertices per face

struct GenMinVert                 // a vertex
{
  sU8 Select;                     // selection flag
  sU8 BoneCount;                  // bones used
  sU8 TempByte;
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
  sU8 Temp;                       // small temp space
  GenMinVector Normal;            // face normal, for CalcNormals();
  sInt Vertices[KMM_MAXVERT]; 
  sInt Adjacent[KMM_MAXVERT];     // face*8+vertIndex
};

struct GenMinCluster              // a cluster of faces with same material
{
  class GenMaterial *Mtrl;        // link to a KObject 
  sInt RenderPass;
  sInt Id;
  sInt AnimType;                  // 0=static 1=rigid 2=skinned
  sInt AnimMatrix;                // rigid only
  sInt Temp;                      // used by MergeClusters;
  
  void Init(GenMaterial *mtrl,sInt pass=0,sInt id=0,sInt anim=0,sInt mtx=-1)
  {
    Mtrl=mtrl;
    RenderPass=pass;
    Id=id;
    AnimType=anim;
    AnimMatrix=mtx;
  }
  sBool Equals(const GenMinCluster &cl)
  {
    if(Mtrl!=cl.Mtrl) return 0;
    if(RenderPass!=cl.RenderPass) return 0;
    if(Id!=cl.Id) return 0;
    if(AnimType!=cl.AnimType) return 0;
    if(AnimMatrix!=cl.AnimMatrix) return 0;
    return 1;
  }

};

struct GenMinMatrix               // matrices make up the skeleton
{
  void Init();
  void Exit();

  sMatrix BasePose;               // basepose matrix. urg :-)
  sInt Parent;                    // parent matrix, -1 for root
  
  // multiple ways of animation

  // XSI

  sInt KeyCount;                  // number of keys (or 0)
  sF32 SRT[9];                    // unanimated SRT
  sF32 *SPtr;                     // scale overwrite, or 0
  sF32 *RPtr;                     // rotate overwrite, or 0
  sF32 *TPtr;                     // translate overwrite, or 0

  // compressed splines

  /*.ryg.*/

  // werkkzeug splines

  class GenSpline *Spline;

  // no animations

  sMatrix NoAnimation;

  // internal temps
  
  sF32 Offset;                    // offset to time, for SplineTrain effect. so we can use the same time for each matrix.
  sF32 Factor;                    // factor to time, for SplineTrain effect. so we can use the same time for each matrix.
  sInt Used;                      // is this matrix used as a weight?
//  sInt Index;                     // index in weight-matrix-array, or -1
  sMatrix Temp;                   // FK matrix without basepose
};

class GenMinMeshAnim : public EngMeshAnim
{
public:
  sArray<GenMinMatrix> Matrices;

  GenMinMeshAnim(sInt matrixCount);
  ~GenMinMeshAnim();
  GenMinMeshAnim *Copy();

  virtual sInt GetMatrixCount();
  virtual void EvalAnimation(sF32 time,sF32 metamorph,sMatrix *matrices);
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

class GenMinMesh : public KObject
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
  void Copy(KObject *);
  void Clear();

  void ChangeGeo();
  void ChangeTopo();

  // structure

  sArray<GenMinVert> Vertices;
  sArray<GenMinFace> Faces;
  sArray<GenMinCluster> Clusters;
  GenMinMeshAnim *Animation;

  // topological queries (only correct after CalcAdjacency!)

  sInt OppositeEdge(sInt edgeTag) const;
  sInt NextFaceEdge(sInt edgeTag) const;
  sInt PrevFaceEdge(sInt edgeTag) const;
  sInt NextVertEdge(sInt edgeTag) const;
  sInt PrevVertEdge(sInt edgeTag) const;

  // utilities

  GenMinVert *MakeGrid(sInt x,sInt y,sInt flags);      //flags: 1 add top point, 2 add bottom point, 4=stitch tx, 8=stitch ty
  sInt AddCluster(GenMaterial *,sInt pass,sInt id=0,sInt anim=0,sInt mtx=-1);
  void CreateAnimation(sInt matrixCount);

  void Transform(sInt sel,const sMatrix &mat,sInt src=0,sInt dest=0);  // transform position, uv0, uv1
  void Merge();                                 // merge vertices at same position.
  void CalcNormals();                           // calculate normal and tangent
  void Add(GenMinMesh *mesh);
  void MergeClusters();                         // merge equal clusters and remove unused clusters
  void Invert();
  void CalcBBox(sAABox &box) const;

  void SelectAll(sInt in,sInt out);

  sInt *CalcMergeVerts() const;

  sBool CalcAdjacencyCore(const sInt *remap); // returns sTRUE iff mesh is closed
  sBool CalcAdjacency(); // return sTRUE iff mesh is closed
  void VerifyAdjacency();
  void BakeAnimation(sF32 fade,sF32 metamorph);

  void AutoStitch();

  void Prepare();
  void UnPrepare();

#if !sPLAYER
  void PrepareWire(sInt flags,sU32 selMask);
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
GenMinMesh * __stdcall MinMesh_Cylinder(sInt tx,sInt ty,sInt,sInt tz,sInt arc);
GenMinMesh * __stdcall MinMesh_XSI(sChar *filename);
GenMinMesh * __stdcall MinMesh_Kopuli();

// special

GenMinMesh * __stdcall MinMesh_MatLink(GenMinMesh *mesh,GenMaterial *mtrl,sInt mask,sInt pass);
GenMinMesh * __stdcall MinMesh_Add(sInt count,GenMinMesh *msh,...);
GenMinMesh * __stdcall MinMesh_SelectAll(GenMinMesh *msh,sU32 mode);
GenMinMesh * __stdcall MinMesh_SelectCube(GenMinMesh *msh,sInt dmode,sF323 center,sF323 size);
GenMinMesh * __stdcall MinMesh_DeleteFaces(GenMinMesh *msh);
GenMinMesh * __stdcall MinMesh_Invert(GenMinMesh *msh);
GenMinMesh * __stdcall MinMesh_MatLinkId(GenMinMesh *mesh,GenMaterial *mtrl,sInt id,sInt pass);
GenMinMesh * __stdcall MinMesh_Pipe(GenSpline *spline_,GenMinMesh *mesh0_,GenMinMesh *mesh1_,GenMinMesh *mesh2_,sInt flags,sF32 texzoom,sF32 ringdist);
GenMinMesh * __stdcall MinMesh_Multiply(KOp *,GenMinMesh *msh,sFSRT srt,sInt count,sInt mode,sF32 tu,sF32 tv,sF323 lrot,sF32 extrude);
GenMinMesh * __stdcall MinMesh_Multiply2(sInt seed,sInt3 count1,sF323 translate1,sInt3 count2,sF323 translate2,sInt random,sInt3 count3,sF323 translate3,sInt inCount,GenMinMesh *inMesh,...);
GenMinMesh * __stdcall MinMesh_SelectLogic(GenMinMesh *msh,sInt mode);

// geometry modifiers

GenMinMesh * __stdcall MinMesh_TransformEx(GenMinMesh *msh,sInt mask,sFSRT srt);
GenMinMesh * __stdcall MinMesh_ScaleAnim(GenMinMesh *mesh,sF32 scale);
GenMinMesh * __stdcall MinMesh_Displace(GenMinMesh *msh,class GenBitmap *bmp,sInt mask,sF323 ampli);
GenMinMesh * __stdcall MinMesh_Perlin(GenMinMesh *msh,sInt mask,sFSRT srt,sF323 ampl);
GenMinMesh * __stdcall MinMesh_ExtrudeNormal(GenMinMesh *msh,sInt mask,sF32 distance);
GenMinMesh * __stdcall MinMesh_Bend2(GenMinMesh *msh,sF323 center,sF323 rotate,sF32 len,sF32 axis);
GenMinMesh * __stdcall MinMesh_BakeAnimation(GenMinMesh *msh,sF32 fade,sF32 metamorph);
GenMinMesh * __stdcall MinMesh_BoneChain(GenMinMesh *msh,sF323 p0,sF323 p1,sInt count,sInt flags);
GenMinMesh * __stdcall MinMesh_BoneTrain(GenMinMesh *mesh,class GenSpline *spline,sF32 delta,sInt mode);
GenMinMesh * __stdcall MinMesh_Center(GenMinMesh *msh,sInt which);
GenMinMesh * __stdcall MinMesh_RenderAutoMap(GenMinMesh *msh,sInt flags);

// hack
GenMinMesh * __stdcall MinMesh_MTetra(sF32 isoValue);

// topology modifiers

GenMinMesh * __stdcall MinMesh_Triangulate(GenMinMesh *msh);
GenMinMesh * __stdcall MinMesh_Subdivide(GenMinMesh *msh,sInt mask,sF32 alpha,sInt count);
GenMinMesh * __stdcall MinMesh_Extrude(GenMinMesh *mesh,sInt mode,sInt count,sF323 distance);

// compressed meshes

GenMinMesh * __stdcall MinMesh_Compress(GenMinMesh *msh);

/****************************************************************************/
/****************************************************************************/

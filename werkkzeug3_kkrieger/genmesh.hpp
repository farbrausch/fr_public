// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __GENMESH2_HPP__
#define __GENMESH2_HPP__

#include "_types.hpp"
#include "kdoc.hpp"
#if sLINK_XSI
#include "_xsi.hpp"
#endif

/****************************************************************************/

struct GenMeshVert;
struct GenMeshEdge;
struct GenMeshFace;
struct GenMeshCell;

class GenMaterial;
class GenBitmap;

class GenMinMesh;

sBool CheckMesh(class GenMesh *&mesh,sU32 mask=0x80000000);

// selection mask:  00vvffee --- vert:corn:face:edge

#define MSM_ADD             0     // add to selection
#define MSM_SUB             1     // sub from selection
#define MSM_SET             2     // set selection
#define MSM_SETNOT          3     // set not

#define MAS_EDGE            1     // mesh all2sel edge
#define MAS_FACE            2     // mesh all2sel face
#define MAS_VERT            4     // mesh all2sel vert

/****************************************************************************/
 
struct GenMeshUpdate              // update parameters, USE AddRef()
{
  sInt MatrixIndex;               // identify matrix in MeshRecorder
  sInt Operation;                 // identify operation in MeshRecorder

  union                           // link to operator data
  {
    sU8 *Data;
    sU32 *DataU;
    sS32 *DataS;
    sF32 *DataF;
  };
};

/****************************************************************************/

struct GenMeshElem                          // same for all elements of a mesh
{
  sU8 Mask;                                 // most selections are done with bitmask
  sU8 Id;                                   // you can assign an id and generate masks from it
  sU8 Select;                               // selection is set from the mask and used internally
  sU8 Used;                                 // unused elements are (supposed to be) removed during cleanup.

  void InitElem();
  void SelElem(sU32 mask,sBool state,sInt mode);
  void SelLogic(sU32 smask1,sU32 smask2,sU32 dmask,sInt mode);
};

struct GenMeshVert : public GenMeshElem     // vertex data. maybe multiple per corner
{
  sInt Next;                                // next vertex
  sInt First;                               // the vertex containing the "real" position.
  sInt Temp;
  sInt Temp2;
  sInt ReIndex;
  sU8 Weight[4];                            // palette skinning
  sU8 Matrix[4];

  void Init();
  void Sel(sU32 mask,sBool state,sInt mode) {SelElem(mask>>16,state,mode);}
}; // =32 bytes (player), 36 bytes (editor)

struct GenMeshEdge : public GenMeshElem     // edge of a mesh
{
  sInt Next[2];                             // next edge in face. low bit set when edge[i]->Face[1]==face[i]
  sInt Prev[2];                             // prev edge in face. low bit set when edge[i]->Face[1]==face[i]

  sInt Face[2];                             // faces each side of edge
  sInt Vert[2];                             // vertices connected by edge

  sInt Temp[2];
  sInt Crease;

  void Init();
  void Sel(sU32 mask,sBool state,sInt mode) {SelElem(mask>>0,state,mode);}
}; // =48 bytes

struct GenMeshFace : public GenMeshElem     // face of a mesh
{
  sInt Material;                            // material used, 0 = face deleted
  sInt Edge;                                // one of the edges. low bit set when Edge->Face[1]==this
  sInt Temp;
  sInt Temp2;
  sInt Temp3;

  void Init();
  void Sel(sU32 mask,sBool state,sInt mode) {SelElem(mask>>8,state,mode);}
}; // =24 bytes

struct GenMeshMtrl                          // material of a face. use Mtrl[Face->Id]
{
  GenMaterial *Material;                    // the material to use
  sInt JobIds[16];                          // just assume there are never more then 16 info in a job
  sInt Pass;                                // sort passes
  sInt Remap;                               // used only during GenMesh::Add()
};

struct GenMeshMatrix
{
  sF32 BaseSRT[9];
  sF32 TransSRT[9];
  sMatrix Matrix;
  sInt Parent;
  sInt Used;
};

struct GenMeshCurve 
{
  sInt Offset;
  sInt Matrix;
  sInt Curve;
};

struct GenMeshColl
{
  sInt Vert[8];                             // points at extremes. bits 012=xyz, 0=min, 1=max.
  sInt Mode;                                // KCM_ADD, KCM_SUB
  KLogic Logic;                             // what to do...
};

/****************************************************************************/

enum ClipCode
{
  CC_ALL_ON_PLANE,
  CC_NOT_CROSSED,
  CC_CROSSED
};

struct GenSimpleFace
{
  sInt VertexCount;
  sVector *Vertices;

  void Init(sInt Verts);
  void Exit();

  void AddVertex(const sVector &v); // assumes Vertices[] has enough space!
  
  ClipCode Clip(const sVector &plane,GenSimpleFace *sides) const;
  sBool Inside(const sVector *planes,sInt nPlanes) const;
};

struct GenSimpleFaceList : public GenSimpleFace
{
  GenSimpleFaceList *Next;
};

class GenSimpleMesh : public KObject
{
public:
  sArray<GenSimpleFace> Faces;

  GenSimpleMesh();
  ~GenSimpleMesh();
  void Copy(KObject *);

  void Clear();
  void Add(const GenSimpleMesh *other);

  void AddQuad(const sVector &v1,const sVector &v2,const sVector &v3,const sVector &v4);
  void AddFace(const GenSimpleFace &face);
  void Cube(const sAABox &box);

  sBool CSGSplitR(const GenSimpleFace &face,const sVector *planes,sInt plane,sInt nPlanes,sBool keepOut);
  void CSGAgainst(const sVector *planes,sInt nPlanes,sBool keepOut);
};

class GenSimpleBrush
{
public:
  GenSimpleMesh *Mesh;
  sInt PlaneCount;
  sVector *Planes;
  sAABox BBox;

  GenSimpleBrush();
  ~GenSimpleBrush();

  void Cube(const sAABox &box);
  void CSGAgainst(const GenSimpleBrush &other,sBool keepOut);
};

/****************************************************************************/

#define sGMI_POS          (0)
#define sGMI_NORMAL       (1)
#define sGMI_TANGENT      (2)
#define sGMI_COLOR0       (3)
#define sGMI_COLOR1       (4)
#define sGMI_UV0          (5)
#define sGMI_UV1          (6)
#define sGMI_UV2          (7)
#define sGMI_UV3          (8)
#define sGMI_LAST         sGMI_UV3

#define sGMF_POS          (1<<0)
#define sGMF_NORMAL       (1<<1)
#define sGMF_TANGENT      (1<<2)
#define sGMF_COLOR0       (1<<3)
#define sGMF_COLOR1       (1<<4)
#define sGMF_UV0          (1<<5)
#define sGMF_UV1          (1<<6)
#define sGMF_UV2          (1<<7)
#define sGMF_UV3          (1<<8)

#define sGMF_NORMALS      (sGMF_NORMAL|sGMF_TANGENT)
#define sGMF_DEFAULT      (sGMF_POS|sGMF_NORMAL|sGMF_COLOR0|sGMF_TANGENT|sGMF_UV0/*|sGMF_UV1*/)
#define sGMF_ALL          (sGMF_POS|sGMF_NORMAL|sGMF_TANGENT|sGMF_COLOR0|sGMF_COLOR1|sGMF_UV0|sGMF_UV1|sGMF_UV2|sGMF_UV3)

/****************************************************************************/

#define GMA_NOP           0
#define GMA_TRANSFORM     1
#define GMA_PERLIN        2
#define GMA_BONE          3
#define GMA_EXNORMAL      4
#define GMA_EXTRUDE       5
#define GMA_SUBDIVIDE     6
#define GMA_MULTIPLY      7
#define GMA_BEVEL         8
#define GMA_BEND          9
#define GMA_TRANSFORMEX   10

/****************************************************************************/

class GenMesh;
typedef sU32 *(*RecorderProc)(GenMesh *mesh,sU32 *code,sMatrix *matrices,KObject **objects);

class GenMesh : public KObject
{
public:
  GenMesh();
  ~GenMesh();
  void Copy(KObject *);

  static sU32 Features2Mask(sInt colorSets,sInt uvSets);

  void Init(sU32 vertmask,sInt vertcount);    // count is only an estimate (state too large!)
  void Realloc(sInt vertcount);

  sArray<GenMeshVert> Vert;
  sArray<GenMeshEdge> Edge;
  sArray<GenMeshFace> Face;
  sArray<GenMeshMtrl> Mtrl;       // material 0 is unused!
  sArray<GenMeshColl> Coll;       // collision
  sArray<sInt>        Lgts;       // lightslots
  sArray<sInt>        Parts;      // start offsets of convex parts

  sVector *VertBuf;               // structure of arrays for Vertexbuffers, 0 = unused
  sInt VertCount;                 // vertex count
  sInt VertAlloc;                 // vertices allocated

#if !sINTRO
  sInt _VertMask;                 // gmf flags
  sInt _VertSize;                 // vectors per vertex
  sInt _VertMap[16];              // map to index;
#endif

  __forceinline sInt VertMask();
  __forceinline sInt VertSize();
  __forceinline sInt VertMap(sInt i);

#if !sINTRO
  sArray<GenMeshMatrix> BoneMatrix;
  sArray<GenMeshCurve> BoneCurve;
  sInt KeyCount;
  sInt CurveCount;
  sF32 *KeyBuf;
  sInt Anim0,Anim1;               // bone animation range (in frames)
#endif

  class EngMesh *PreparedMesh;

  sInt Pivot;
  sBool GotNormals;
  sAABox BBox;

#if !sPLAYER
  class EngMesh *WireMesh;
  sInt WireFlags;
  sU32 WireSelMask;
#endif

  // i = edge<<1 | direction
  inline GenMeshEdge *GetEdge(sU32 i) { return &Edge.Array[i>>1]; }
  GenMeshFace *GetFace(sU32 i);
  GenMeshFace *GetFaceI(sU32 i);
  GenMeshVert *GetVert(sU32 i);
  sInt GetFaceId(sU32 i);
  sInt GetVertId(sU32 i);
  sInt NextFaceEdge(sU32 i);
  sInt PrevFaceEdge(sU32 i);
  sInt NextVertEdge(sU32 i);
  sInt PrevVertEdge(sU32 i);

  sInt SkipFaceEdge(sU32 i,sInt num);
  sInt SkipVertEdge(sU32 i,sInt num);

  sInt AddPivot();

  /*inline GenMeshFace *GetFace(sU32 i) { return &Face.Array[GetEdge(i)->Face[i&1]]; }
  inline GenMeshFace *GetFaceI(sU32 i) { return &Face.Array[GetEdge(i)->Face[(i&1)^1]]; }
  inline GenMeshVert *GetVert(sU32 i) { return &Vert.Array[GetEdge(i)->Vert[i&1]]; }
  inline sInt GetFaceId(sU32 i) { return GetEdge(i)->Face[i&1]; }
  inline sInt GetVertId(sU32 i) { return GetEdge(i)->Vert[i&1]; }
  inline sInt NextFaceEdge(sU32 i) { return GetEdge(i)->Next[i&1]; }
  inline sInt PrevFaceEdge(sU32 i) { return GetEdge(i)->Prev[i&1]; }
	inline sInt NextVertEdge(sU32 i) { return GetEdge(i)->Prev[i&1]^1; }
  inline sInt PrevVertEdge(sU32 i) { return GetEdge(i)->Next[(i&1)^1]; }*/

  inline sVector &VertPos(sInt v)   { return VertBuf[v*VertSize()]; }
  inline sVector &VertNorm(sInt v)  { return VertBuf[v*VertSize()+1]; }

#if !sPLAYER
	sInt GetValence(sU32 e);
	sInt GetDegree(sU32 e);
#endif

  void Compact();

  sInt AddVert();
  sInt AddNewVert();
  sInt AddCopiedVert(sInt src);
  void SplitFace(sU32 e0,sU32 e1,sU32 dmask=0,sInt dmode=0);                    // create edge from start of e0 to start of e1 and split face
  void SplitBridge(sU32 i,sU32 e,sInt &va,sInt &vb,sU32 dmask=0,sInt dmode=0,sInt *dv1=0,sInt *dv2=0);  // creates a "bridge" edge between i and e
  sInt AddEdge(sInt v0,sInt v1,sInt face);
	void MakeFace(sInt face,sInt nedges,...);
#if !sINTRO
  void Verify();
#endif
  void ReIndex();
  void CleanupMesh();
  sBool IsBorderEdge(sU32 i,sInt mode);

  void Mask2Sel(sU32 mask);
  void All2Sel(sBool set=1,sInt mask=MAS_EDGE|MAS_FACE|MAS_VERT);
  void Id2Mask(sU32 mask,sInt id);
  void Sel2Mask(sU32 dmask,sInt dmode=0);
  void All2Mask(sU32 dmask,sInt dmode=0);
  void Face2Vert();
	void Edge2Vert(sInt uvs);
  void Vert2FaceEdge();
  void CalcNormal(sVector &n,sInt o0,sInt o1,sInt o2);
  void CalcFaceNormal(sVector &n,sInt edge);
  void CalcFaceNormalAccurate(sVector &n,sInt f);
  void CalcFacePlane(sVector &p,sInt f);
  void CalcFaceCenter(sVector &m,sInt f);
  void CalcBBox(sAABox &box);
  void CopyVert(sInt dest,sInt src);

  void TransVert(sMatrix &mat,sInt sj=0,sInt dj=0);
  void SelectCube(const sVector &vc,const sVector &vs,sU32 dmask,sInt dmode);
  void SelectSphere(const sVector &vc,const sVector &vs,sU32 dmask,sInt dmode);
  void Ring(sInt count,sU32 dmask,sF32 radius,sF32 phase,sInt arc=0);
  void Extrude(sU32 dmask,sInt dmode=0,sInt mode=0);
  void Subdivide(sF32 alpha=1.0f);
  sInt Bones(sF32 phase);
  void BonesModify(sInt matrix,sF32 phase);
  void FixVertCycle(sInt edge);
  void Crease(sInt selType=0,sU32 dmask=0,sInt dmode=0,sInt what=0xfe);
  void UnCrease(sBool edge=0,sInt what=0xfe);
	void CalcNormals(sInt type=0,sInt calcWhat=3);
  void NeedAllNormals();
	void Triangulate(sInt threshold=5,sU32 dmask=0,sInt dmode=0,sInt type=0); // Triangulates selected polygons
	void Cut(const sVector &plane,sInt mode=1); // Cut or clip by a plane
	void ExtrudeNormal(sF32 distance);
	void Displace(GenBitmap *bitmap,sF32 amplx,sF32 amply,sF32 amplz);
	void Bevel(sF32 elevate,sF32 pullin,sInt mode,sU32 dmask=0,sInt dmode=0);
	void Perlin(const sMatrix &mat,const sVector &amp);
	sBool Add(GenMesh *other,sInt keepmaterial=0);
	void DeleteFaces();
  void SelectGrow();

  void CubicProjection(const sMatrix &mat,sInt mask=0,sInt dest=sGMI_UV0,sBool real=sFALSE);

#if sLINK_XSI
  void ImportXSI(sXSILoader *xsi);
  void ImportXSIR(sXSIModel *xm,sInt parent);
  void ImportXSI(sXSICluster *xc,sInt firstvertex);
#endif
  void Prepare();
  void UnPrepare();
#if !sPLAYER
  void PrepareWire(sInt flags,sU32 selMask);
  void UnPrepareWire();
#endif
};

/****************************************************************************/

#if !sINTRO

sInt GenMesh::VertMask()
{
  return _VertMask;
}

sInt GenMesh::VertSize()
{
  return _VertSize;
}

sInt GenMesh::VertMap(sInt i)
{
  return _VertMap[i];
}

#else

__forceinline sInt GenMesh::VertMask()
{
  return sGMF_DEFAULT;
}

__forceinline sInt GenMesh::VertSize()
{
  return 5;
}

__forceinline sInt GenMesh::VertMap(sInt i)
{
  static const sInt _VertMap[] =
  {
    0,1,2,3, -1,4,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  };

  return _VertMap[i];
}

#endif

/****************************************************************************/

GenMesh * __stdcall Mesh_Cube(sInt tx,sInt ty,sInt tz,sInt flags,sFSRT srt);
GenMesh * __stdcall Mesh_SelectAll(GenMesh *msh,sU32 mask,sInt mode);
GenMesh * __stdcall Mesh_SelectCube(GenMesh *msh,sU32 dmask,sInt dmode,sF323 center,sF323 size);
GenMesh * __stdcall Mesh_Subdivide(KOp *,GenMesh *msh,sInt mask,sF32 alpha,sInt count);
GenMesh * __stdcall Mesh_Extrude(KOp *,GenMesh *mesh,sInt smask,sInt dmask,sInt mode,sInt count,sF323 dist,sF323 s,sF323 r);
GenMesh * __stdcall Mesh_Transform(KOp *,GenMesh *msh,sInt mask,sFSRT srt);
GenMesh * __stdcall Mesh_Cylinder(sInt tx,sInt ty,sInt mode,sInt tz,sInt arc);
GenMesh * __stdcall Mesh_TransformEx(KOp *,GenMesh *msh,sInt mask,sFSRT srt,sInt sj,sInt dj);
GenMesh * __stdcall Mesh_Crease(GenMesh *msh,sInt mask,sInt what,sInt selType);
GenMesh * __stdcall Mesh_UnCrease(GenMesh *msh,sInt mask,sInt what,sInt selType);
GenMesh * __stdcall Mesh_CalcNormals(GenMesh *msh,sInt mode,sInt mask,sInt what);
GenMesh * __stdcall Mesh_Torus(sInt x,sInt y,sF32 ro,sF32 ri,sF32 phase,sF32 arclen,sInt flags);
GenMesh * __stdcall Mesh_Sphere(sInt x,sInt y);
GenMesh * __stdcall Mesh_Triangulate(GenMesh *msh,sInt mask,sInt threshold,sInt type);
GenMesh * __stdcall Mesh_Cut(GenMesh *msh,sF322 dir,sF32 offset,sInt mode);
GenMesh * __stdcall Mesh_ExtrudeNormal(KOp *,GenMesh *msh,sInt mask,sF32 distance);
GenMesh * __stdcall Mesh_Displace(KOp *,GenMesh *msh,GenBitmap *bmp,sInt mask,sF323 ampli);
GenMesh * __stdcall Mesh_Bevel(KOp *,GenMesh *msh,sInt mask,sF32 elev,sF32 pull,sInt mode);
GenMesh * __stdcall Mesh_Perlin(KOp *,GenMesh *msh,sInt mask,sFSRT srt,sF323 ampl);
GenMesh * __stdcall Mesh_Add(sInt count,GenMesh *msh,...);
GenMesh * __stdcall Mesh_DeleteFaces(GenMesh *msh,sInt mask);
GenMesh * __stdcall Mesh_BeginRecord(GenMesh *msh);
GenMesh * __stdcall Mesh_SelectRandom(GenMesh *msh,sU32 dmask,sInt dmode,sU32 ratio,sInt seed);
GenMesh * __stdcall Mesh_Multiply(KOp *,GenMesh *msh,sFSRT srt,sInt count,sInt mode,sF32 tu,sF32 tv,sF323 lrot,sF32 extrude);
GenMesh * __stdcall Mesh_Bend(KOp *,GenMesh *msh,sInt mask,sFSRT srt1,sFSRT srt2,sF322 dir,sF322 yrange,sInt mode);
GenMesh * __stdcall Mesh_CollisionCube(GenMesh *input,KOp *event,KOp *event2,sF32 x0,sF32 x1,sF32 y0,sF32 y1,sF32 z0,sF32 z1,sInt mode,sInt tx,sInt ty,sInt tz,sInt eventa=0,sInt eventb=0,sInt eventc=0,sInt eventd=0);
GenMesh * __stdcall Mesh_Grid(sInt mode,sInt tesu,sInt tesv);
GenMesh * __stdcall Mesh_SelectLogic(GenMesh *msh,sU32 smask1,sU32 smask2,sU32 dmask,sInt mode);
GenMesh * __stdcall Mesh_SelectGrow(GenMesh *msh,sU32 smask,sU32 dmask,sInt dmode,sInt amount);
GenMesh * __stdcall Mesh_Invert(GenMesh *msh);
GenMesh * __stdcall Mesh_SetPivot(GenMesh *msh,sInt attr,sF323 pivot);
GenMesh * __stdcall Mesh_UVProjection(GenMesh *msh,sInt mask,sFSRT srt,sInt type);
GenMesh * __stdcall Mesh_Center(GenMesh *msh,sInt mask,sInt which);
GenMesh * __stdcall Mesh_AutoCollision(GenMesh *msh,sF32 enlarge,sInt mode,sInt tx,sInt ty,sInt tz,sInt outmask);
GenMesh * __stdcall Mesh_SelectSphere(GenMesh *msh,sU32 dmask,sInt dmode,sF323 center,sF323 size);
GenMesh * __stdcall Mesh_SelectFace(GenMesh *msh,sU32 dmask,sInt dmode,sInt face);
GenMesh * __stdcall Mesh_SelectAngle(GenMesh *msh,sU32 dmask,sInt dmode,sF322 dir,sF32 thresh);
GenMesh * __stdcall Mesh_Bend2(GenMesh *msh,sF323 center,sF323 rotate,sF32 len,sF32 axis);
GenMesh * __stdcall Mesh_SmoothAngle(GenMesh *msh,sF32 angle);
GenMesh * __stdcall Mesh_Color(GenMesh *msh,sF323 pos,sF322 dir,sU32 color,sF32 amplify,sF32 range,sInt op);
GenMesh * __stdcall Mesh_BendS(GenMesh *msh,sF323 anchor,sF323 rotate,sF32 len,KSpline *spline);
GenMesh * __stdcall Mesh_LightSlot(sF323 pos,sU32 color,sF32 amplify,sF32 range);
GenMesh * __stdcall Mesh_ShadowEnable(GenMesh *mesh,sBool enable);
GenMesh * __stdcall Mesh_Multiply2(sInt seed,sInt3 count1,sF323 translate1,sInt3 count2,sF323 translate2,sInt random,sInt3 count3,sF323 translate3,sInt inCount,GenMesh *inMesh,...);

GenSimpleMesh * __stdcall Mesh_BSP(GenMesh *mesh);

GenMesh * __stdcall Mesh_XSI(sChar *filename);
GenMesh * __stdcall Mesh_SingleVert();

GenMinMesh * __stdcall Mesh_ToMin(GenMesh *mesh);

/****************************************************************************/

#endif

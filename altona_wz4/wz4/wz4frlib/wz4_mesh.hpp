/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_WZ4_MESH_HPP
#define FILE_WZ4FRLIB_WZ4_MESH_HPP

#define WZ4MESH_LOWMEM 1

#include "base/types.hpp"
#include "base/graphics.hpp"
#include "wz4frlib/wz4_mtrl2.hpp"
#include "wz4frlib/wz4_anim.hpp"

/****************************************************************************/

class Wz4MeshCluster
{
public:
  sGeometry *Geo[2];
  sGeometry *InstanceGeo[4];
  sArray<sInt> Matrices;          // bone matrix splitting for chunks
  Wz4Mtrl *Mtrl;
  sInt Temp;
  sInt ChunkStart;                // first chunk id in this cluster
  sInt ChunkEnd;                  // last chunk id +1 in this cluster
  sInt IndexSize;                 // sGF_INDEX16 or sGF_INDEX32
  sAABBoxC Bounds;                // bounding box of the whole cluster
  sInt LocalRenderPass;           // used as sort key

  Wz4MeshCluster();
  ~Wz4MeshCluster();
};

struct Wz4MeshVertex
{
  sVector31 Pos;
  sVector30 Normal;
  sVector30 Tangent;
  sF32 BiSign;
  sF32 U0,V0;
  sF32 U1,V1;
#if !WZ4MESH_LOWMEM
  sU32 Color0;
  sU32 Color1;
#endif
  sS16 Index[4];
  sF32 Weight[4];
  union
  {
    sF32 Select;
    sInt SelectTemp; // 2nd temp field - wipes selection!
  };
  sInt Temp;

  void Init();
  void Init(const sVector31 &pos,sF32 u,sF32 v);
  void Transform(const sMatrix34 &mat,const sMatrix34 &matInvT);
  void AddWeight(sInt index,sF32 weight);
  void NormWeight();
  void Skin(sMatrix34 *basemat,sInt max,sVector31 &pos);
  sBool operator==(const Wz4MeshVertex &v) const;
  sU32 Hash() const;

  void Zero();
  void AddScale(Wz4MeshVertex *v,sF32 scale);
  void CopyFrom(Wz4MeshVertex *v);
};

struct Wz4MeshFace
{
  sInt Cluster;
  sU8 Count;          // must be 3 or 4.
  sU8 Select;
  sU8 Selected;       // saved selections: bitfield - 1 bit per slot.
  sU8 _pad;           // explicit padding. (this byte is unused)
  sInt Vertex[4];     // vertices (CCW)
  sInt Temp;

  void Init(sInt count);
  void Invert();
};

struct Wz4MeshSel
{
  sU32 Id;            // element ID
  sF32 Selected;      // selection value
};

enum Wz4MeshSelMode
{
  wMSM_LOAD   = 0x00,
  wMSM_STORE  = 0x01,
};

enum Wz4MeshSelType
{
  wMST_VERTEX   = 0x00,
  wMST_FACE     = 0x01,
};

struct Wz4MeshFaceConnect
{
  sInt Adjacent[4];   // opposite halfedge for each halfedge in the mesh (face*4+vertInd)
                      // edge (face*4+vertInd) is outgoing edge from (face,vertInd)
};

struct Wz4ChunkPhysics
{
  sF32 Volume;        // uniform density of 1, so this corresponds to mass
  sVector31 COM;      // center of mass
  sVector30 InertD;   // diagonal elements of mass distribution matrix
  sVector30 InertOD;  // off-diagonal elements of mass distribution matrix (xy,yz,zx)

  sInt FirstVert;     // first vertex in mesh corresponding to this chunk
  sInt FirstFace;     // first face in mesh corresponding to this chunk
  sInt FirstIndex;    // first index in mesh corresponding to this chunk
  sInt Temp;

  sVector30 Normal;   // Normal for Debris Op
  sVector30 Random;   // Random Rotation Axis for Debris Op

  void Transform(const sMatrix34 &mat);
  void GetInertiaTensor(sMatrix34 &tensor) const; 
};

class Wz4Mesh : public wObject 
{
  sGeometry *WireGeoLines;
  sGeometry *WireGeoVertex;
  sGeometry *WireGeoFaces;
  sGeometry *InstanceGeo;
  sGeometry *InstancePlusGeo;
  sGeometry *WireGeoInst;
public:
  sArray<Wz4MeshVertex> Vertices;
  sArray<Wz4MeshFace> Faces;
  sArray<Wz4MeshSel> SelVertices[8];  // stored vertices selection in slots
  sArray<Wz4MeshCluster *> Clusters;
  Wz4Skeleton *Skeleton;
  sArray<Wz4ChunkPhysics> Chunks; // alternative to skeleton: just unconnected chunks (debris-style)#
  sInt BBoxValid;                 // this is set after ChargeBbox is called.
  sInt SaveFlags;                 // how to compress the mesh?

  sString<64> Name;

  Wz4Mesh();
  ~Wz4Mesh();

  template <class streamer> void Serialize_(streamer &stream);
  void Serialize(sWriter &stream);
  void Serialize(sReader &stream);

  void CopyFrom(Wz4Mesh *);
  void CopyClustersFrom(Wz4Mesh *src);
  sBool IsEmpty();
  void Flush();
  void Clear();
  void ClearClusters();
  void AddDefaultCluster();
  void Add(Wz4Mesh *mesh);
  void MergeClusters();
  void MergeVertices();           // merge identical vertices and kill unused vertices
  sBool IsDegenerateFace(sInt face) const; // degenerate face: one in which a vertex position occurs twice
  void RemoveDegenerateFaces();
  void ConvertFrom(class ChaosMesh *);

  void InsertClusterAfter(Wz4MeshCluster *cl,sInt pos);
  void SplitClustersAnim(sInt maxMats); // split clusters for bone vertex shader
  void SplitClustersChunked(sInt maxMats);
  void SortClustersByLocalRenderPass();

  // connectivity

  sInt GetTriCount();
  sInt *BasePos(sInt toitself=0);
  sInt *BaseNormal();
  Wz4MeshFaceConnect *Adjacency();
  void CalcNormals(sInt *basemap, sBool onlyselected=sFALSE);
  void CalcTangents(sInt *basemap, sBool onlyselected=sFALSE);
  void CalcNormals();
  void CalcTangents();
  void CalcNormalAndTangents(sBool forceSmooth=sFALSE, sBool onlyselected=sFALSE);

  // connectivity queries and traversal: functions that don't require adjacency information

  sInt GetFaceInd(sInt edgeTag) const;          // extract face index from edge tag
  sInt GetVertInd(sInt edgeTag) const;          // extract vertex index (inside face) from edge tag

  const Wz4MeshFace &FaceFromEdge(sInt edgeTag) const;
  Wz4MeshFace &FaceFromEdge(sInt edgeTag);

  const Wz4MeshVertex &StartVertFromEdge(sInt edgeTag) const;
  Wz4MeshVertex &StartVertFromEdge(sInt edgeTag);

  sInt OutgoingEdge(sInt face,sInt vert) const; // gets outgoing half-edge tag for vertex #vert in face
  sInt IncomingEdge(sInt face,sInt vert) const; // gets incoming half-edge tag for vertex #vert in face
  sInt NextFaceEdge(sInt edgeTag) const;        // next edge in this face (CCW)
  sInt PrevFaceEdge(sInt edgeTag) const;        // prev edge in this face (CCW)

  sInt StartVertex(sInt edgeTag) const;         // start vertex of half-edge with this tag
  sInt EndVertex(sInt edgeTag) const;           // end vertex of half-edge with this tag

  // connectivity queries and traversal: functions that require adjacency information

  sInt OppositeEdge(const Wz4MeshFaceConnect *conn,sInt edgeTag) const; // corresponding half-edge in opposite direction
  sInt NextVertEdge(const Wz4MeshFaceConnect *conn,sInt edgeTag) const; // next outgoing edge with same start vertex (CCW)
  sInt PrevVertEdge(const Wz4MeshFaceConnect *conn,sInt edgeTag) const; // prev outoging edge with same start vertex (CCW)

  // topological operations
  void SetOpposite(Wz4MeshFaceConnect *conn,sInt edge,sInt opposite);
  void EdgeFlip(Wz4MeshFaceConnect *conn,sInt edgeTag); // flip internal edge (only works for internal edges between two triangles!)

  // splitting

  sInt SplitEdgeAlongPlane(sInt va,sInt vb,const sVector4 &plane);
  void SplitGenFace(sInt base,const sInt *verts,sInt count,sBool reuseBase);
  void SplitAlongPlane(const sVector4 &plane);

  // helpers for ops

  sBool DivideInChunksR(Wz4MeshFace *mf,sInt mfi,Wz4MeshFaceConnect *conn);
  sVector30 GetFaceNormal(sInt face) const;
  void CalcBBox(sAABBox &box) const;

  // selection
  void SelStoreLoad(sInt mode, sInt type, sInt slot);
  void SelectGrow(Wz4MeshFaceConnect *adj, sInt amount, sInt power, sF32 range);
  void SelFacesToVertices(sBool outputType, sBool addToInput, sF32 vertexValue, sBool clearFaces);
  void SelVerticesToFaces(sBool outputType, sBool addToInput, sF32 vertexValue);

  /*** ops ***/

  // transforms

  void Transform(const sMatrix34 &mat);
  void TransformUV(const sMatrix34 &mat);
  void BakeAnim(sF32 time);
  void Displace(const sMatrix34 &mat,sImageI16 *img,sF32 amount,sF32 bias,sInt flags,sInt sel);
  sBool Deform(sInt count,const sVector31 &p0,const sVector31 &p1,sInt keycount,const struct Wz4MeshArrayDeform *keys,sInt selection,sInt upflags,const sVector30 &up);
  void ExtrudeNormal(sInt logic,sF32 amount);
  void Heal(Wz4Mesh *reference, sInt flags, sF32 posThres, sF32 normThres);
  void TransformRange(sInt rangeMode,sInt mode,sInt selection,sVector2 direction, sVector2 axialRange,
                      sVector31 scaleStart, sVector30 rotateStart, sVector31 translateStart,
                      sVector31 scaleEnd, sVector30 rotateEnd, sVector31 translateEnd);

  // topology

  void Facette(sF32 smooth=0);
  void Crease();                  // create a crease between selected and unselected faces
  void Uncrease(sInt select);     // remove all creases at the selected vertices
  void Subdivide(sF32 smooth); // subdivide all selected faces
  void Extrude(sInt steps,sF32 amount,sInt flags,const sVector31 &center,sF32 localScale,sInt SelectUpdateFlag,const sVector2 &uvOffset);
  void Splitter(Wz4Mesh *in,sF32 depth,sF32 scale);
  void Dual(Wz4Mesh *in,sF32 random);
  sBool DivideInChunks(sInt flags,const sVector30 &normal,const sVector30 &rot);
  void Bevel(sF32 amount);
  void Weld(sF32 weldEpsilon);
  void Mirror(sBool mx, sBool my, sBool mz, sInt selection, sInt mode); 

  // generators

  void MakeGrid(sInt tx,sInt tz);
  void MakeDisc(sInt tx,sF32 ri,sF32 phase,sBool doublesided);
  void MakeCube(sInt tx,sInt ty,sInt tz);
  void MakeTorus(sInt segment,sInt slices,sF32 ri,sF32 ro,sF32 phase=0,sF32 arc=1.0f);
  void MakeSphere(sInt segments,sInt slices);
  void MakeCylinder(sInt segments,sInt slices,sInt top,sInt flags);
  void MakeText(const sChar *text,const sChar *font,sF32 height,sF32 extrude,sF32 maxErr,sInt flags);
  void MakePath(const sChar *path,sF32 extrude,sF32 maxErr,sF32 weldThreshold,sInt flags);

private:
  void Finish2DExtrusionOp(sF32 extrude,sInt flags);

public:

  // painting

  void ChargeWire(sVertexFormatHandle *fmt);
  void ChargeSolid(sInt flags);
  void ChargeBBox();
  sInt ChargeCount;
  sInt DontClearVertices;
  void Charge(); // EXPERIMENTAL: charges and then deletes original data when IsPlayer flag is set

  void BeforeFrame(sInt index,sInt mc=0,const sMatrix34CM *mats=0);
  void Render(sInt flags,sInt index,const sMatrix34CM *mat,sF32 time,const sFrustum &frustum);
  void RenderInst(sInt flags,sInt index,sInt ic,const sMatrix34CM *imats,sU32 *colors=0);
  void RenderBone(sInt flags,sInt index,sInt mc,const sMatrix34CM *mats,sInt chunks=-1,const sMatrix34CM *mat=0);
  void RenderBoneInst(sInt flags,sInt index,sInt mc,const sMatrix34CM *mats,sInt ic,const sMatrix34CM *imats);
  
  // Mesh format support
  
  sBool LoadXSI(const sChar *file,sBool forceanim = 0,sBool forcergb = 0);
  sBool LoadLWO(const sChar *file);
  sBool LoadOBJ(const sChar *file);
  void  LoadWz3MinMesh(const sU8 *&blob);
  sBool LoadWz3MinMesh(const sChar *file);
  sBool SaveOBJ(const sChar *file);
};

/****************************************************************************/

// useful helper for Deform();

struct BendKey
{
  sF32 Time;
  sVector31 Pos;
  sVector30 Dir;
  sF32 Twist;
  sF32 Scale;
};

void BendMatrices(sInt count,const BendKey *bk,sMatrix34 *mats,const sVector30 &up,sInt um);

/****************************************************************************/

#endif // FILE_WZ4FRLIB_WZ4_MESH_HPP


/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WERKKZEUG4_CHAOSMESH_CODE_HPP
#define FILE_WERKKZEUG4_CHAOSMESH_CODE_HPP

#include "base/types2.hpp"
#include "base/math.hpp"
#include "base/graphics.hpp"
#include "base/serialize.hpp"
#include "wz4lib/poc_ops.hpp"
#include "wz4frlib/wz4_mtrl_ops.hpp"
#include "wz4_anim.hpp"

/****************************************************************************/

#define CHAOSMESHWEIGHTS 4

struct ChaosMeshVertexPosition
{
  sVector31 Position;
  sInt MatrixIndex[CHAOSMESHWEIGHTS];
  sF32 MatrixWeight[CHAOSMESHWEIGHTS];
  sInt Select;

  void Init();
  void AddWeight(sInt index,sF32 value);
  void ClearWeights();
};

sBool Equals(const ChaosMeshVertexPosition &a,const ChaosMeshVertexPosition &b);

struct ChaosMeshVertexNormal
{
  sVector30 Normal;

  void Init();
};


struct ChaosMeshVertexTangent
{
  sVector30 Tangent;
  sF32 BiSign;

  void Init();
};

struct ChaosMeshVertexProperty
{
  sU32 C[2];
  sF32 U[4],V[4];

  void Init();
};

struct ChaosMeshFace
{
  sInt Cluster;                   // index to material or cluster, -1 for deleted face
  sInt Count;                     // 3 or 4
  sInt Select;
  sInt Positions[4];
  sInt Normals[4];
  sInt Property[4];   // should be properties
  sInt Tangents[4];

  void Init();
};

#define sMVF_MATRICES 4
#define sMVF_COLORS 2
#define sMVF_UVS 4

struct ChaosMeshVertexFat
{
  sVector31 Position;
  sInt MatrixIndex[sMVF_MATRICES];            // -1 for unused
  sF32 MatrixWeight[sMVF_MATRICES];           // 0 for unused

  sVector30 Normal;
  sVector30 Tangent;
  sF32 BiSign;

  sU32 C[sMVF_COLORS];
  sF32 U[sMVF_UVS],V[sMVF_UVS];

  sInt Index;                                 // index must be last, it is ignored on comparision


  bool operator< (const ChaosMeshVertexFat &b) { return sCmpMem(this,&b,sizeof(ChaosMeshVertexFat)-sizeof(sInt))< 0; }
  bool operator==(const ChaosMeshVertexFat &b) { return sCmpMem(this,&b,sizeof(ChaosMeshVertexFat)-sizeof(sInt))==0; }
};



/****************************************************************************/

class ChaosMeshCluster                      // clusters hold a processed copy of the geometry
{
public:
  ChaosMeshCluster();
  ~ChaosMeshCluster();

  Wz4Material *Material;                    // material for this cluster, as indexed by the face

  sArray<ChaosMeshVertexFat> Vertices;      // during the building, "fat" vertices are build
  sArray<sInt> Indices;                     // during the building, faces are transformed to index buffers
  sArray<sInt> Matrices;                    // during the building, SkeletonMatrixIndex = Matrices[ClusterMatrixIndex]
  sGeometry *Geo;                           // finally, fat vertices are thinned out and a real geometry is build
  sGeometry *WireGeo;                       // special geometry for wireframe more, build on demand
  sAABBox Bounds;                           // BoundingBox
//  sInt JointId;                             // the whole object is bound to a joint. This is only used when no weights are given!
  
  void CopyFrom(const ChaosMeshCluster *cl);
  void Charge();                            // perform the final step of uploading the geometry
  void ChargeWire();                        // .. for wireframe. does not eliminate double edges
};

class ChaosMeshModel              // models are used to specify subsets of dotxsi files. usually no models are created
{
public:
  wDocName Name;
  sInt StartCluster,EndCluster;
  sInt StartPos,EndPos;
  sInt StartNorm,EndNorm;
  sInt StartTang,EndTang;
  sInt StartProp,EndProp;
  sInt JointId;                             // the whole object is bound to a joint.
  class ChaosMesh *BackLink;      // used only during import
  void CopyFrom(const ChaosMeshModel *s);
};

enum ChaosMeshCopyFlags
{
  SEPARATE_VERTEX = 1,            // must be set
  SEPARATE_FACE = 2,              // must be set
  SEPARATE_MATERIAL = 4,          // create copies of materail
  SEPARATE_SKELETON = 8,          // create copies of skeleton
};


struct ChaosMeshAnimClip
{
  sF32 Start;                       // 0..1 time value for clip start
  sF32 End;                         // 0..1 time value for clip end
  sF32 Speed;                       // multiply time in seconds by this to get correct speed
  sInt Id;                          // identify clip;
};

struct ChaosMtrlVariant
{
  sU32 AndFlags;                    // Flags = (OldFlags & AndFlags) | OrFlags;
  sU32 OrFlags;
  sU32 BlendColor;                  // ~0 -> no change, otherwise replace
  sU32 BlendAlpha;                  // ~0 -> no change, otherwise replace

  ChaosMtrlVariant();
};

class ChaosMesh : public wObject
{
public:
  ChaosMesh();
  ~ChaosMesh();
  sString<64> Name;
  template <class streamer> void Serialize_(streamer &s,sTexture2D *shadow=0);
  void Serialize(sWriter &,sTexture2D *shadow=0);
  void Serialize(sReader &,sTexture2D *shadow=0);
  
  // animation

  Wz4Skeleton *Skeleton;
  sArray<ChaosMeshAnimClip> AnimClips;

  // geometry

  sArray<ChaosMeshVertexPosition> Positions;
  sArray<ChaosMeshVertexNormal> Normals;
  sArray<ChaosMeshVertexTangent> Tangents;
  sArray<ChaosMeshVertexProperty> Properties;
  sArray<ChaosMeshFace> Faces;
  sArray<ChaosMeshCluster *> Clusters;
  sArray<ChaosMeshModel *> Models;

  // tools for building geometry

  void Clear();
  sInt AddCluster(Wz4Material *mtrl);
  void AddGrid(const sMatrix34 &mat,sInt tx,sInt ty,sBool doublesided);
  void AddCube(const sMatrix34 &mat0,sInt tx,sInt ty,sInt tz,sBool wrapuv=0,sF32 sx=1,sF32 sy=1,sF32 sz=1);
  void AddTorus(const sMatrix34 &mat,sInt tx,sInt ty,sF32 ri,sF32 ro,sF32 phase=0,sF32 arclen=1);
  void AddSphere(const sMatrix34 &mat,sInt tx,sInt ty);
  void AddCylinder(const sMatrix34 &mat,sInt tx,sInt ty,sInt toprings);
  void CopyFrom(const ChaosMesh *,sInt seg = SEPARATE_VERTEX|SEPARATE_FACE);
  void CopyFrom(const ChaosMeshModel *);
  void Add(const ChaosMesh *);
  void Triangulate();
  void CalcNormals();
  void CalcTangents();
  void MergePositions();
  void MergeNormals();
  void Cleanup();                 // remove unused positions,normals,properties
  void SplitForMatrices();
  void InsertClusterAfter(ChaosMeshCluster *cl,sInt pos);
  void ListTextures(sArray<Texture2D *> &ar);
  void ListTextures(sArray<TextureCube *> &ar);

  // special ops

  void ExportXSI(const sChar *);
  sBool MergeShape(ChaosMesh *ref,ChaosMesh *shape,sF32 phase,ChaosMesh *weight);

  // tools for building clusters

  void UpdateBuffers(sBool preTransformOpt=sFALSE); // build stage 2 geometry, if not already there
  void FlushBuffers();            // free stage 2 geometry and hardware geometry, so it has to be rebuild
  void Charge();                  // transfer to hardware (textures, materials, geometries)};
  void SetShadowMap(sTexture2D *shadowmap);
  void SetMaterialVariants(sInt count,ChaosMtrlVariant *);
  void Paint(const sViewport *view,const SceneInstance *inst,sInt variant=0);
  void Paint(const Wz4ShaderEnv *env,sF32 time,sInt variant=0);    // no bound check
  void PaintMtrl(const Wz4ShaderEnv *env,Wz4Material *mtrl);
  void PaintWire(sGeometry *geo,sF32 time);
  void PaintSelection(sGeometry *geo,sGeometry *quads,sF32 zoom,sF32 time,const sViewport *View);
};

/****************************************************************************/

#endif // FILE_WERKKZEUG4_CHAOSMESH_CODE_HPP


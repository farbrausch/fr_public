// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __ENGINE_HPP__
#define __ENGINE_HPP__

#include "_types.hpp"
#include "genmesh.hpp"

/****************************************************************************/

class GenMinMesh;

struct GenMaterialPass;
class GenMaterial;
class GenScene;
struct KKriegerCell;

struct sVertexTSpace3;
struct sViewport;
struct sMaterialEnv;

/****************************************************************************/

#define ENG_MAXPASS         256   // # of render passes (must be power of 2)

// usage (=forced pass subdivision for realtime lighting)
// DON'T CHANGE ANY OF THIS UNLESS YOU KNOW WHAT YOU'RE DOING!
#define ENGU_BASE           0     // z-fill + ambient
#define ENGU_PRELIGHT       1     // before lighting passes
#define ENGU_AMBIENT        2     // ambient lighting pass
#define ENGU_SHADOW         3     // shadow volumes
#define ENGU_LIGHT          4     // lights
#define ENGU_POSTLIGHT      5     // decal & detail tex
#define ENGU_POSTLIGHT2     6     // envmaps (add)
#define ENGU_OTHER          7     // other (single pass) materials
#define ENGU_MAX            8

// the other finalizers will probably be re-added later.
#define MPP_OFF             0     // unused
#define MPP_STATIC          1     // static vertex and index buffer
#define MPP_SHADOW          2     // shadow volume extrusion
#define MPP_SPRITES         3     // jolly old sprites
#define MPP_THICKLINES      4     // thick lines
#define MPP_WIRELINES       5     // wireframe lines
#define MPP_WIREVERTEX      6     // wireframe vertices
#define MPP_MAX             7

// animation types
#define ENGANIM_STATIC      0     // no animation at all
#define ENGANIM_RIGID       1     // one dynamic matrix transform.
#define ENGANIM_SKINNED     2     // dynamic skinning

// Wireframe flags
#define EWF_VERTS           0x0001
#define EWF_EDGES           0x0002
#define EWF_FACES           0x0004
#define EWF_COLLISION       0x0008
#define EWF_SOLIDPORTALS    0x0010
#define EWF_LINEPORTALS     0x0020
#define EWF_HIDDEN          0x0040
#define EWF_ALL             0x007f

/****************************************************************************/

struct EngPaintInfo
{
  sVector LightPos;
  sF32 LightRange;
  sInt LightId;

  sU32 StencilFlags;
  void *BoneData;
};

/****************************************************************************/

#define EWC_WIREHIGH        0
#define EWC_WIRELOW         1
#define EWC_WIRESUB         2
#define EWC_COLADD          3
#define EWC_COLSUB          4
#define EWC_VERTEX          5
#define EWC_HINT            6
#define EWC_HIDDEN          7
#define EWC_FACE            8
#define EWC_GRID            9
#define EWC_BACKGROUND      10
#define EWC_COLZONE         11
#define EWC_PORTAL          12
#define EWC_MAX             13

/****************************************************************************/

class EngMeshAnim : public KObject
{
public:
  virtual sInt GetMatrixCount() = 0;
  virtual void EvalAnimation(sF32 time,sF32 metamorph,sMatrix *matrices) = 0;
};

/****************************************************************************/

class EngMesh : public KObject
{
private:
  struct SilEdge;
  struct SVCache;
  struct Batch;
  struct Job;
  class BatchBuilder;

  struct BoneInfo // 4+8+16 = 28 bytes
  {
    sU8 BoneCount;
    sU8 _pad[3];
    sF32 Weight[4];
    sU16 Matrix[4];
  };

public:
  EngMesh();
  ~EngMesh();
  void Copy(KObject *);

  sInt VertCount;                           // number of vertices
  sVertexTSpace3Big *Vert;                  // main vertex buffer
  BoneInfo *BoneInfos;                      // bone information
  sInt PartCount;                           // # of parts
  sF32 *PartBBs;                            // part bounding boxes
  sArray<GenMeshMtrl> Mtrl;                 // materials
  sArray<Job> Jobs;                         // jobs

  EngMeshAnim *Animation;                   // animation object

  void FromGenMesh(GenMesh *mesh);
  void FromGenMeshWire(GenMesh *mesh,sInt wireFlags,sU32 wireMask);
  void FromGenMinMesh(GenMinMesh *mesh);
  void FromGenMinMeshWire(GenMinMesh *mesh,sInt wireFlags,sU32 wireMask);
  static void SetWireColors(sU32 *colors);

  void *EvalBones(sMatrix *matrices,sGrowableMemStack &alloc);

  void PaintJob(sInt jobId,GenMaterialPass *pass,const EngPaintInfo &paintInfo);
  void Preload();

private:
  sBool Preloaded;

  // general helpers
  sInt AddMaterial(GenMaterial *mtrl,sInt pass);
  sInt AddJob(sInt mtrlId,sInt program);

  // FromGenMesh helpers
  void FillVertexBuffer(GenMesh *mesh);
  void CalcPartBoundingBoxes(GenMesh *mesh);
  void CalcBoundingBox(GenMesh *mesh,sInt mtrlId,sAABox &box);
  void PrepareJobs(GenMesh *mesh);
  void PrepareJob(Job *job,GenMesh *mesh);
  void PrepareSpriteJob(Job *job,GenMesh *mesh);
  void PrepareThickLinesJob(Job *job,GenMesh *mesh);

  // FromGenMeshWire helpers
  void AddWireEdgeJob(GenMesh *mesh,sInt mtrl,sU32 mask);
  void AddWireFaceJob(GenMesh *mesh,sInt mtrl,sU32 mask,sU32 compare);
  void AddWireVertJob(GenMesh *mesh,sInt mtrl,sU32 mask);
  void AddWireCollisionJob(GenMesh *mesh,sInt mtrl);

  // FromGenMinMesh helpers
  void FillVertexBuffer(GenMinMesh *mesh);
  void CalcBoundingBox(GenMinMesh *mesh,sInt clusterId,sAABox &box);
  void PrepareJobs(GenMinMesh *mesh);
  void PrepareJob(Job *job,GenMinMesh *mesh);
  void PrepareSpriteJob(Job *job,GenMinMesh *mesh);
  void PrepareThickLinesJob(Job *job,GenMinMesh *mesh);

  // FromGenMinMeshWire helpers
  void AddWireEdgeJob(GenMinMesh *mesh,sInt mtrl,sU32 mask);
  void AddWireFaceJob(GenMinMesh *mesh,sInt mtrl,sU32 mask,sU32 compare);
  void AddWireVertJob(GenMinMesh *mesh,sInt mtrl,sU32 mask);

  // Shadow volume stuff
  static sU8 SFaceIn[];
  static sU8 SPartSkip[];

  void UpdateShadowCacheJob(Job *job,sVector *planes,SVCache *svc,const EngPaintInfo &info);

  // Wireframe stuff
  static sInt WireInit;
  static GenMaterial *MtrlWire[];

  static void PrepareWireMaterials();
  static void ReleaseWireMaterials();
};

/****************************************************************************/

struct EngLight
{
  sVector Position;

  sU32 Flags;
  sU32 Color;
  sF32 Amplify;
  sF32 Range;
  KEvent *Event;
  sInt Id;                        // set to 0 for temporary/editor lights

  sF32 Importance;                // set by AddLightJob, used internally
  sFRect LightRect;               // used internally
  sFrustum SVFrustum;             // used internally
  sFrustum ZFailCull;             // used internally
};

#define EL_WEAPON     0x0001      // weapon light (prioritize in selection)
#define EL_SHADOW     0x0002      // does light cast shadows?

/****************************************************************************/

class EngMaterialInsert
{
public:
  virtual sInt GetPriority() = 0;
  virtual void BeforeUsage(sInt pass,sInt usage,const EngLight *light) = 0;
  virtual void AfterUsage(sInt pass,sInt usage,const EngLight *light) = 0;
};

/****************************************************************************/

class Engine_
{
public:
  Engine_();
  ~Engine_();

  sU32 AmbientLight;

  void SetViewProject(const sMaterialEnv &env);
  void ApplyViewProject();
  void StartFrame();

  void SetUsageMask(sU32 mask);

  void AddPaintJob(EngMesh *mesh,const sMatrix &matrix,sF32 time,sInt passAdjust=0);
  void AddPaintJob(KOp *op,const sMatrix &matrix,KEnvironment *kenv,sInt passAdjust=0); 
  void AddLightJob(const EngLight &light);

  void AddAmbientLight(sU32 color);

  void ClearLightJobs()                               { LightJobCount = 0; }
  sInt GetNumLightJobs() const                        { return LightJobCount; }
  
  void AddPortalJob(GenScene *sectors[],const sAABox &portalBox,const sMatrix &matrix,sInt cost);
  void AddSectorJob(GenScene *sector,KOp *sectorData,const sMatrix &matrix);

  void ProcessPortals(KEnvironment *kenv,KKriegerCell *observerCell);

  void Paint(KEnvironment *kenv,sBool specular=sTRUE);
  void PaintSimple(KEnvironment *kenv);

#if !sINTRO
  void DebugPaintBoundingBoxes(sU32 color);

  void DebugPaintStart(sBool world = sTRUE);
  void DebugPaintAABox(const sAABox &box,sU32 color);
  void DebugPaintRect(const sFRect &rect,sU32 color);
  void DebugPaintLine(sF32 x0,sF32 y0,sF32 z0,sF32 x1,sF32 y1,sF32 z1,sU32 color);
  void DebugPaintTri(const sVector &v0,const sVector &v1,const sVector &v2,sU32 color);
#endif

private:
  struct MeshJob;
  struct EffectJob;
  struct PaintJob;
  struct PortalJob;

  sMaterialEnv Env;
  sMatrix View;
  sMatrix Project;
  sMatrix ViewProject;

  sFrustum Frustum;
  sBool CurrentSectorPaint;

  EngLight *LightJobs;
  sInt LightJobCount;

  sMemStack Mem;
  sGrowableMemStack BigMem;

  sArray<sMatrix> Matrices;
  sArray<PaintJob *> PaintJobs;
  sArray<PaintJob *> PaintJobs2;

  EngMaterialInsert *Inserts[ENG_MAXPASS];
  sBool NeedCurrentRender[ENG_MAXPASS];
  sU32 UsageMask;

  MeshJob *MeshJobs;
  EffectJob *EffectJobs;
  PortalJob *PortalJobs;
  GenScene *SectorJobs;

  EngLight WeaponLight;
  sBool WeaponLightSet;

  void InsertLightJob(const EngLight &light,sF32 importance,sF32 fade);
  sInt AddMatrix(const sMatrix &matrix);

  void PortalVisR(GenScene *start,sInt thresh,const sFRect &box);
  
  void BuildPaintJobs();
  void SortPaintJobs();
  void RenderPaintJobs(KEnvironment *kenv);

#if !sINTRO
  sInt GeoLine;
  sInt GeoTri;
  sMaterial *MtrlLines;
#endif
};

extern Engine_ *Engine;

/****************************************************************************/

#endif

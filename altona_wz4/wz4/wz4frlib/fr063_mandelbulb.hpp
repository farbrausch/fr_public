/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_FR063_MANDELBULB_HPP
#define FILE_WZ4FRLIB_FR063_MANDELBULB_HPP


#include "base/types.hpp"
#include "wz4frlib/wz4_demo2.hpp"
#include "wz4frlib/wz4_demo2_ops.hpp"
#include "wz4frlib/fr063_mandelbulb_ops.hpp"

#include "base/types2.hpp"
#include "base/system.hpp"
#include "base/graphics.hpp"
#include "util/shaders.hpp"
#include "extra/blobheap.hpp"
#include "extra/mcubes.hpp"

/****************************************************************************/


class RNFR063_Mandelbulb : public Wz4RenderNode
{
  friend void RNFR063_Mandelbulb_LostDevice(void *ptr);
  class sStsWorkload *Workload;
  sInt ToggleNew;
  sInt FogFactor;
  class OctNode *Root;
  class OctManager *OctMan;

public:
  RNFR063_Mandelbulb();
  ~RNFR063_Mandelbulb();
  void Init();

  void Simulate(Wz4RenderContext *ctx);   // execute the script. 
  void Prepare(Wz4RenderContext *ctx);    // do simulation
  void Render(Wz4RenderContext *ctx);     // render a pass

  Wz4RenderParaFR063_Mandelbulb Para,ParaBase; // animated parameters from op
  Wz4RenderAnimFR063_Mandelbulb Anim;          // information for the script engine

  Wz4Mtrl *Mtrl;
//  sString<sMAXPATH> DumpFile;
};

/****************************************************************************/

class MandelbulbIsoData : public wObject
{
  friend static void CalcNodeT(sStsManager *,sStsThread *thread,sInt start,sInt count,void *data);
  enum IsoEnum
  {
    CellSize = 8,
  };
public:
  struct IsoNode
  {
    sInt Level;
    sVector31 Min,Max;
    sF32 PMin,PMax;
    IsoNode *Childs[8];
    sInt LeafFlag;
    sALIGNED(MCPotField,Pot[CellSize+2][CellSize+1][CellSize+1],16);
  };
private:
  struct IsoNodeChunk
  {
    IsoNode N[256];
  };
  sInt MaxThread;
  sThreadLock ChunkLock;
  sArray<IsoNodeChunk *> FreeChunks;
  sArray<IsoNodeChunk *> AllChunks;
  IsoNodeChunk **ThreadChunk;
  sInt *ThreadChunkCount;
  IsoNode *GetNode(sInt thread);


  IsoNode *Root;
  sArray<IsoNode *> AllNodes;
  void CalcNode(sInt level,sVector31 min,sVector31 max,IsoNode *&store,sStsWorkload *wl,sInt thread);
  void ScanNodes(IsoNode *);


  struct CalcNodeInfo
  {
    MandelbulbIsoData *_this;
    sStsWorkload *Workload;
    sInt Count;
    sInt Level[8];
    sVector31 Min[8],Max[8];
    IsoNode **Store[8];
  };
  void CalcNode(CalcNodeInfo *,sInt thread,sInt n0,sInt n1);

public: 
  MandelbulbIsoData();
  ~MandelbulbIsoData();

  MandelbulbIsoDataParaFR063_MandelbulbIsoData Para;
  sArray<IsoNode *> LeafNodes;

  void Init();
};

class RNFR063_MandelbulbIso : public Wz4RenderNode
{
  friend static void MarchIsoT(sStsManager *,sStsThread *thread,sInt start,sInt count,void *data);

  enum IsoEnum
  {
    CellSize = 8,
  };

  sInt MaxThread;
  MarchingCubesHelper MC;
public:
  RNFR063_MandelbulbIso();
  ~RNFR063_MandelbulbIso();
  void Init();
  void March();


  void Simulate(Wz4RenderContext *ctx);   // execute the script. 
  void Prepare(Wz4RenderContext *ctx);    // do simulation
  void Render(Wz4RenderContext *ctx);     // render a pass

  Wz4RenderParaFR063_MandelbulbIso Para,ParaBase; // animated parameters from op
  Wz4RenderAnimFR063_MandelbulbIso Anim;          // information for the script engine

  Wz4Mtrl *Mtrl;
  MandelbulbIsoData *IsoData;
};


/****************************************************************************/



#define BLOBHEAP 1

/*
#define MANDEL_SPLIT 0.1f     // larger tiles will be split
#define MANDEL_DRAW  0.05f    // smaller tiles will be drawn immediatly, larger tiles check thier childs
#define MANDEL_DROP  0.035f   // smaller tiles will be dropped
*/
/*
#define MANDEL_SPLIT (0.15f*0.5f)     // larger tiles will be split
#define MANDEL_DRAW  (0.14f*0.5f)     // smaller tiles will be drawn immediatly, larger tiles check thier childs
#define MANDEL_DROP  (0.12f*0.5f)     // smaller tiles will be dropped
*/
/****************************************************************************/
/****************************************************************************/

class OctNode;
struct OctGeoHandle;
struct OctMemBundle;

class OctManager
{
  struct OctRenderJob
  {
    sF32 *Dest;
    sVector4 Info;
    sInt YPos;
  };
  struct Target
  {
    Target();
    ~Target();
    sTexture2D *tex;
    sGpuToCpu *trans;
    sArray<OctRenderJob> Jobs;
    sInt RenderYPos;
  };
  struct CSVertex
  {
    sF32 px,py;
    sF32 yoff;
    sF32 f0,f1,f2,f3;
    sF32 u,v;

    void Init(sF32 PX,sF32 PY,sF32 YOFF,const sVector4 &F,sF32 U,sF32 V)
    { px=PX; py=PY; yoff=YOFF; f0=F.x; f1=F.y; f2=F.z; f3=F.w; u=U; v=V; }
  };
  struct GeoBuffer
  {
    GeoBuffer(OctManager *oct);
    ~GeoBuffer();
    sGeometry *Geo;
    sBool Drawn;
    sInt IndexAlloc;
    sInt IndexUsed;
    sInt VertexAlloc;
    sInt VertexUsed;
    sArray<OctGeoHandle *> Handles;   // sorted! should be DLIST
  };


  sArray<sGeometry *> DummyGeos;
  sArray<Target *> FreeTargets;
  sArray<Target *> ReadyTargets;
  sArray<Target *> BusyTargets;
  sArray<Target *> DelayTargets;
  sArray<Target *> DoneTargets;
  Target *CurrentTarget;

  // GPGPU resources

  sVertexFormatHandle *CSFormat;
  sGeometry *CSGeo;
  sMaterial *CSMtrl;
  sViewport CSView;
  sTexture2D *CSTex[3];


  // gemometry resources

  sArray<GeoBuffer *> Geos;
  sArray<OctGeoHandle *> GeoHandles;    // removing from here is acutally slow! user list in GeoBuffer, and nothing else!
  sArray<OctGeoHandle *> DirtyHandles;
  sArray<sDrawRange> DrawRange;
  void DrawSolid(GeoBuffer *);
  void DrawAlpha(GeoBuffer *);

  // other

  sArray<OctMemBundle *> FreeMemBundles;
  sArray<OctNode *> FreeNodes;
public:
  OctManager(Wz4RenderParaFR063_Mandelbulb *);
  ~OctManager();
  void NewFrame();
  void LostDevice();

  void ReadbackRender();
  void AddRender(sInt X,sInt Y,sInt Z,sInt Q,sF32 *Dest);
  void StartRender();

  // stats

  sInt VerticesDrawn;
  sInt VerticesLife;
  sInt DeepestDrawn;
  sInt NodesDrawn;
  sInt GeoRoundRobin;

  sInt NodesTotal;
  sInt MemBundlesTotal;
  sInt GetUsedNodes() { return NodesTotal-FreeNodes.GetCount(); }
  sInt GetUsedMemBundles() { return MemBundlesTotal-FreeMemBundles.GetCount(); }

  // para

  sF32 Scale;
  sF32 Iterations;
  sF32 Isovalue;
  sInt CacheWarmup;

  // pipeline

  sArray<OctNode *> Pipeline0;
  sArray<OctNode *> Pipeline0b;
  sArray<OctNode *> Pipeline1;
  sArray<OctNode *> Pipeline2;

  // Geometry

  void AllocHandle(OctGeoHandle &);
  void FreeHandle(OctGeoHandle &);
  void MakeInvisible();
  void Draw();
  
  sVertexFormatHandle *MeshFormat;
  struct MeshVertex
  {
    sF32 px,py,pz;
    sU8 n[4];
    void Init(sF32 PX,sF32 PY,sF32 PZ,sF32 NX,sF32 NY,sF32 NZ)
    { px=PX; py=PY; pz=PZ; n[0]=sU8((NX+1)*127); n[1]=sU8((NY+1)*127); n[2]=sU8((NZ+1)*127); n[3]=0; }
  };

  // per thread data

  sInt ThreadMax;
  sInt ThreadVertexAlloc;
  sInt ThreadIndexAlloc;
  sInt **ThreadICache;
  MeshVertex **ThreadVB;
  sU16 **ThreadIB;  

  // Other

  OctMemBundle *AllocMemBundle();
  void FreeMemBundle(OctMemBundle *);

  OctNode *AllocNode();
  void FreeNode(OctNode *);

  sBool EndGame;
};

//extern OctManager *OctMan;

struct OctMemBundle     // this was once more complex, most of that ended to be per thread memory
{
  OctMemBundle();
  ~OctMemBundle();
  sF32 *pot;
};

struct OctGeoHandle
{
  OctGeoHandle();
  ~OctGeoHandle();
  void Clear();
  void Alloc(sInt vc,sInt ic);

  // info from user
  sInt Visible;
  sInt VertexCount;
  sInt IndexCount;
  sInt Alpha;                     // 0..0xff

#if BLOBHEAP
  struct sBlobHeapHandle *hnd;
#else
  sU8 *Data;
  sU16 *IB;                       // memory owned by manager
  OctManager::MeshVertex *VB;
#endif

  // info from manager

  sBool Valid;
  sBool Enqueued;
  sInt Geometry;
  sInt VertexStart;
  sInt IndexStart;
};


class OctNode
{
  sInt VertexUsed;
  sInt IndexUsed;
  sInt VertexAlloc;
  sInt IndexAlloc;
  OctManager::MeshVertex *VB;
  sU16 *IB;
  sF32 *pot;
  sInt *icache;
  OctManager *OctMan;

  OctMemBundle *Bundle;
public:
  OctNode(OctManager *oct);
  ~OctNode();
  void Clear();
  void Free();

  sInt x,y,z,q;

  sF32 Area,BestArea,WorstArea;
  sBool Evictable;
  sBool Splittable;
  sBool Splitting;
  sBool Initializing;
  sBool NeverDelete;
  OctGeoHandle GeoHandle;
  
  sVector31 Center;
  sAABBox Box;
  sBool HasChilds;
  sBool PotentialChild[8];
  OctNode *Childs[8];
  OctNode *NewChilds[8];
  OctNode *Parent;
  sInt VertCount;

  void Init0(sInt x,sInt y,sInt z,sInt q);
  void Init1(sInt threadindex);
  void Init2();
  void MakeChilds0();
  void MakeChilds1(class sStsWorkload *wl);
  void MakeChilds2();
  void Draw(const sFrustum &fr,sF32 thresh,sF32 fade);
  void DrawShadow(const sFrustum &fr,sInt shadowlevel);
  sBool PrepareDraw(const sViewport &view,sInt shadowlevel);
  OctNode *FindBest();
  OctNode *FindWorst();
  void UpdateArea();
  void DeleteChilds();
};

/****************************************************************************/
/****************************************************************************/


#endif // FILE_WZ4FRLIB_FR063_MANDELBULB_HPP


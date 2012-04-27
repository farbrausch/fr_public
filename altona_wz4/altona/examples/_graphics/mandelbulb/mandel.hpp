/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_MANDEL_MANDEL_HPP
#define FILE_MANDEL_MANDEL_HPP

#include "base/types2.hpp"
#include "base/system.hpp"
#include "base/graphics.hpp"
#include "util/shaders.hpp"
#include "extra/blobheap.hpp"

#define BLOBHEAP 1

/*
#define MANDEL_SPLIT 0.1f     // larger tiles will be split
#define MANDEL_DRAW  0.05f    // smaller tiles will be drawn immediatly, larger tiles check thier childs
#define MANDEL_DROP  0.035f   // smaller tiles will be dropped
*/

#define MANDEL_SPLIT (0.15f*0.5f)     // larger tiles will be split
#define MANDEL_DRAW  (0.14f*0.5f)     // smaller tiles will be drawn immediatly, larger tiles check thier childs
#define MANDEL_DROP  (0.12f*0.5f)     // smaller tiles will be dropped

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
    GeoBuffer();
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
  void Draw(GeoBuffer *);

  // other

  sArray<OctMemBundle *> FreeMemBundles;
  sArray<OctNode *> FreeNodes;

public:
  OctManager();
  ~OctManager();
  void NewFrame();

  void ReadbackRender();
  void AddRender(const sVector4 &Info,sF32 *Dest);
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

  // pipeline

  sArray<OctNode *> Pipeline0;
  sArray<OctNode *> Pipeline0b;
  sArray<OctNode *> Pipeline1;
  sArray<OctNode *> Pipeline2;

  // Geometry

  void AllocHandle(OctGeoHandle &);
  void FreeHandle(OctGeoHandle &);
  void Draw();
  
  sVertexFormatHandle *MeshFormat;
  struct MeshVertex
  {
    sF32 px,py,pz;
    sU8 n[4];
    void Init(sF32 PX,sF32 PY,sF32 PZ,sF32 NX,sF32 NY,sF32 NZ)
    { px=PX; py=PY; pz=PZ; n[0]=sInt((NX+1)*127); n[1]=sInt((NY+1)*127); n[2]=sInt((NZ+1)*127); n[3]=0; }
  };

  // Other

  OctMemBundle *AllocMemBundle();
  void FreeMemBundle(OctMemBundle *);

  OctNode *AllocNode();
  void FreeNode(OctNode *);

  sBool EndGame;

#if BLOBHEAP
  class sBlobHeap *BlobHeap;
#endif
};

extern OctManager *OctMan;

struct OctMemBundle
{
  OctMemBundle();
  ~OctMemBundle();
  sF32 *pot;
  sInt *icache;
  OctManager::MeshVertex *VB;
  sU16 *IB;
  
  sInt VertexAlloc;
  sInt IndexAlloc;
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

  OctMemBundle *Bundle;
public:
  OctNode();
  ~OctNode();
  void Clear();
  void Free();

  sInt x,y,z,q;

  sF32 Area,BestArea,WorstArea;
  sBool Evictable;
  sBool Splittable;
  sBool Splitting;
  sBool Initializing;
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
  void Init1();
  void Init2();
  void MakeChilds0();
  void MakeChilds1(class sStsWorkload *wl);
  void MakeChilds2();
  void Draw(sMaterial *mtrl,const sViewport &view);
  sBool PrepareDraw(const sViewport &view);
  OctNode *FindBest();
  OctNode *FindWorst();
  void UpdateArea();
  void DeleteChilds();
};


/****************************************************************************/

#endif // FILE_MANDEL_MANDEL_HPP


// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "engine.hpp"
#include "genmesh.hpp"
#include "genminmesh.hpp"
#include "genmaterial.hpp"
#include "genscene.hpp"
#include "geneffect.hpp"
#include "genoverlay.hpp"
#include "kkriegergame.hpp"
#include "_start.hpp"
#include "rtmanager.hpp"
#include <xmmintrin.h>
#include <malloc.h>

#include "_gui.hpp"

#if sINTRO
#define _aligned_malloc(n,a) (::operator new(n))
#define _aligned_free(n)     (::operator delete(n))
#endif

#if sPROFILE
#include "_util.hpp"
sMAKEZONE(PortalVis   ,"PortalVis"    ,0xff00ff00);
sMAKEZONE(PortalJob   ,"PortalJob"    ,0xff70b040);
sMAKEZONE(ShadowJob   ,"ShadowJob"    ,0xff800000);
sMAKEZONE(UpdatePlanes,"UpdatePlanes" ,0xff902020);
sMAKEZONE(BuildJobs   ,"BuildJobs"    ,0xff5f2297);
sMAKEZONE(SortJobs    ,"SortJobs"     ,0xffe4ab3b);
sMAKEZONE(RenderJobs  ,"RenderJobs"   ,0xff7ade53);
sMAKEZONE(InstancePrg ,"InstancePrg"  ,0xff3e9024);
sMAKEZONE(EvalBones   ,"EvalBones"    ,0xffef23ff);
sMAKEZONE(VertCopy    ,"VertCopy"     ,0xff2090ee);
sMAKEZONE(VCopyStencil,"VCopyStencil" ,0xff18406e);
#endif

/****************************************************************************/

// Fixed limits, increase as required
#ifdef KKRIEGER
#define MAXLIGHT    4             // number of *active* lights in scene
#else
#define MAXLIGHT    16            // this affects render pass sorting!!
#endif

#define MAXBATCH    16            // number of batches per job
#define MAXSVCACHE  8             // sv caches/job.
#define MAXENGMEM   0x60000       // memory used internally by engine

// Feature toggles for intro
#define BONES       !sINTRO
#define THICKLINES  1
#define SHADOWS     !sINTRO

/****************************************************************************/

// helper functions
static sInt ConvertRGB(sF32 value)
{
  return sRange<sInt>(sFtol(value * 255.0f),255,0);
}

static sInt ConvertXYZ(sF32 value)
{
  return sRange<sInt>(sFtol((value + 1.0f) * 127.5f),255,0);
}

static sU32 PackColor(const sVector *color)
{
  return (ConvertRGB(color->x) << 16) | (ConvertRGB(color->y) <<  8)
       | (ConvertRGB(color->z) <<  0) | (ConvertRGB(color->w) << 24);
}

static sU32 PackVector(const sVector *vector)
{
  return (ConvertXYZ(vector->x) << 16) | (ConvertXYZ(vector->y) << 8)
       | (ConvertXYZ(vector->z) <<  0) | (ConvertXYZ(vector->w) << 24);
}

static void UnpackVector(sU32 packed,sVector &unpacked)
{
  unpacked.x = ((packed >> 16) & 0xff) / 127.5f - 1.0f;
  unpacked.y = ((packed >>  8) & 0xff) / 127.5f - 1.0f;
  unpacked.z = ((packed >>  0) & 0xff) / 127.5f - 1.0f;
  unpacked.w = ((packed >> 24) & 0xff) / 127.5f - 1.0f;
}

/****************************************************************************/

struct EngMesh::SilEdge                     // silhouette extraction edge
{
  sInt Vert[2];                             // vertex index
  sInt Face[2];                             // face (plane) indices
};

/****************************************************************************/

struct EngMesh::SVCache
{
  sVector LightPos;
  sF32 LightRange;
  sInt LightId;
  
  sInt *IndexBuffer;                        // index buffer for sil edges
  sInt SilIndices;                          // # of silhouette indices
  sInt CapIndices;                          // # of cap indices

  void Init();
  void Exit();
};

void EngMesh::SVCache::Init()
{
  LightId = 0;

  IndexBuffer = 0;
  SilIndices = 0;
  CapIndices = 0;
}

void EngMesh::SVCache::Exit()
{
  delete[] IndexBuffer;
}

/****************************************************************************/

struct EngMesh::Job                         // painting is done in jobs
{
  sInt MtrlId;                              // material id for this job
  sInt Program;                             // (finalizer) program

  sInt Geometry;                            // handle of geometry object
  sInt VertexCount;                         // # of vertices
  sInt IndexCount;                          // # of indices in buffer
  sInt EdgeCount;                           // # of (silhouette) edges
  sInt PlaneCount;                          // # of planes
  sInt PartCount;                           // # of convex parts

  sInt *VertexBuffer;                       // vertices (refs into main VB)
  sInt *IndexBuffer;                        // indices.
  sInt *FaceMap;                            // for SV caps
  SilEdge *Edges;                           // for silhouette extraction
  sVector *Planes;                          // for silhouette extraction

  sInt *FacePartEnd;                        // to cull groups of faces
  sInt *EdgePartEnd;                        // to cull groups of edges

  sInt AnimType;                            // ENGANIM_*
  sInt AnimMatrix;                          // for "rigid" type
  sAABox BBox;                              // bounding box

  EngMesh::SVCache SVCache[MAXSVCACHE];     // shadow volume caches.

  void Init();
  void Alloc(sInt vc,sInt ic,sInt ec,sInt fmc,sInt pc=0);
  void Exit();

  void OptimizeIndices();                   // optimize for cache coherence
  void ReIndexVertices();                   // orders vertices sequentially

  void UpdatePlanes(sVector *target,const sVertexTSpace3Big *verts);

  EngMesh::SVCache *GetSVCache(sInt lightId);
};

void EngMesh::Job::Init()
{
  MtrlId = 0;
  Program = MPP_OFF;

  Geometry = sINVALID;
  VertexCount = 0;
  IndexCount = 0;
  EdgeCount = 0;
  PlaneCount = 0;
  PartCount = 0;

  VertexBuffer = 0;
  IndexBuffer = 0;
  FaceMap = 0;
  Edges = 0;
  Planes = 0;

  FacePartEnd = 0;
  EdgePartEnd = 0;

  AnimType = 0;
  AnimMatrix = -1;

  for(sInt i=0;i<MAXSVCACHE;i++)
    SVCache[i].Init();
}

void EngMesh::Job::Alloc(sInt vc,sInt ic,sInt ec,sInt fmc,sInt pc)
{
  sVERIFY(Geometry == sINVALID);

  VertexCount = vc;
  IndexCount = ic;
  EdgeCount = ec;
  PartCount = pc;

  VertexBuffer = new sInt[vc];
  IndexBuffer = new sInt[ic];
  FaceMap = new sInt[fmc];
  Edges = new EngMesh::SilEdge[ec];
  Planes = 0;

  FacePartEnd = new sInt[pc];
  EdgePartEnd = new sInt[pc];
}

void EngMesh::Job::Exit()
{
  if(Geometry != sINVALID)
    sSystem->GeoRem(Geometry);

  delete[] VertexBuffer;
  delete[] IndexBuffer;
  delete[] FaceMap;
  delete[] Edges;
  delete[] Planes;

  delete[] FacePartEnd;
  delete[] EdgePartEnd;

  for(sInt i=0;i<MAXSVCACHE;i++)
    SVCache[i].Exit();
}

/****************************************************************************/

struct VCacheVert
{
  sInt CachePos;      // its position in the cache (-1 if not in)
  sInt Score;         // its score (higher=better)
  sInt TrisLeft;      // # of not-yet-used tris
  sInt *TriList;      // list of triangle indices
  sInt OpenPos;       // position in "open vertex" list
};

struct VCacheTri
{
  sInt Score;         // current score (-1 if already done)
  sInt Inds[3];       // vertex indices
};

static void DumpCacheEfficiency(const sInt *inds,sInt indCount)
{
  static const sInt cacheSize = 24;
  static const sBool isFIFO = sTRUE;

  sInt misses = 0;
  sInt cache[cacheSize+1];
  sInt wrPos = 0;

  if(!indCount)
    return;

  // initialize cache (we simulate a FIFO here)
  for(sInt i=0;i<cacheSize;i++)
    cache[i] = -1;

  // simulate
  for(sInt i=0;i<indCount;i++)
  {
    sInt ind = inds[i];
    cache[cacheSize] = ind;

    // find in cache
    sInt cachePos;
    for(cachePos=0;cache[cachePos] != inds[i];cachePos++);
    misses += cachePos == cacheSize;

    if(isFIFO)
    {
      cache[wrPos] = ind;
      if(++wrPos == cacheSize)
        wrPos = 0;
    }
    else
    {
      // move to front
      for(sInt j=cachePos;j>0;j--)
        cache[j] = cache[j-1];

      cache[0] = ind;
    }
  }
    
  // print results
  sF32 ACMR = misses * 3.0f / indCount;
  sDPrintF("ACMR: %d.%03d (fifo: %s)\n",sInt(ACMR),sInt(ACMR*1000+0.5f)%1000,isFIFO ? "yes" : "no");
}

void EngMesh::Job::OptimizeIndices()
{
  if(!IndexCount)
    return;

  // alloc+initialize vertices
  VCacheVert *verts = new VCacheVert[VertexCount];
  for(sInt i=0;i<VertexCount;i++)
  {
    verts[i].CachePos = -1;
    verts[i].Score = 0;
    verts[i].TrisLeft = 0;
    verts[i].TriList = 0;
    verts[i].OpenPos = -1;
  }

  // prepare triangles
  sInt nTris = IndexCount/3;
  VCacheTri *tris = new VCacheTri[nTris];
  sInt *indPtr = IndexBuffer;

  for(sInt i=0;i<nTris;i++)
  {
    tris[i].Score = 0;

    for(sInt j=0;j<3;j++)
    {
      sInt ind = *indPtr++;
      tris[i].Inds[j] = ind;
      verts[ind].TrisLeft++;
    }
  }

  // alloc space for vert->tri indices
  sInt *vertTriInd = new sInt[nTris*3];
  sInt *vertTriPtr = vertTriInd;

  for(sInt i=0;i<VertexCount;i++)
  {
    verts[i].TriList = vertTriPtr;
    vertTriPtr += verts[i].TrisLeft;
    verts[i].TrisLeft = 0;
  }

  // make vert->tri tables
  for(sInt i=0;i<nTris;i++)
  {
    for(sInt j=0;j<3;j++)
    {
      sInt ind = tris[i].Inds[j];
      verts[ind].TriList[verts[ind].TrisLeft] = i;
      verts[ind].TrisLeft++;
    }
  }

  // open vertices
  sInt *openVerts = new sInt[VertexCount];
  sInt openCount = 0;

  // the cache
  static const sInt cacheSize = 32;
  static const sInt maxValence = 15;
  sInt cache[cacheSize+3];
  sInt pos2Score[cacheSize];
  sInt val2Score[maxValence+1];

  for(sInt i=0;i<cacheSize+3;i++)
    cache[i] = -1;

  for(sInt i=0;i<cacheSize;i++)
  {
    sF32 score = (i<3) ? 0.75f : sFPow(1.0f - (i-3)/(cacheSize-3),1.5f);
    pos2Score[i] = score * 65536.0f + 0.5f;
  }

  val2Score[0] = 0;
  for(sInt i=1;i<16;i++)
  {
    sF32 score = 2.0f * sFInvSqrt(i);
    val2Score[i] = score * 65536.0f + 0.5f;
  }

  // outer loop: find triangle to start with
  indPtr = IndexBuffer;
  sInt seedPos = 0;

  while(1)
  {
    sInt seedScore = -1;
    sInt seedTri = -1;

    // if there are open vertices, search them for the seed triangle
    // which maximum score.
    for(sInt i=0;i<openCount;i++)
    {
      VCacheVert *vert = &verts[openVerts[i]];

      for(sInt j=0;j<vert->TrisLeft;j++)
      {
        sInt triInd = vert->TriList[j];
        VCacheTri *tri = &tris[triInd];

        if(tri->Score > seedScore)
        {
          seedScore = tri->Score;
          seedTri = triInd;
        }
      }
    }

    // if we haven't found a seed triangle yet, there are no open
    // vertices and we can pick any triangle
    if(seedTri == -1)
    {
      while(seedPos < nTris && tris[seedPos].Score<0)
        seedPos++;

      if(seedPos == nTris) // no triangle left, we're done!
        break;

      seedTri = seedPos;
    }

    // the main loop.
    sInt bestTriInd = seedTri;
    while(bestTriInd != -1)
    {
      VCacheTri *bestTri = &tris[bestTriInd];

      // mark this triangle as used, remove it from the "remaining tris"
      // list of the vertices it uses, and add it to the index buffer.
      bestTri->Score = -1;

      for(sInt j=0;j<3;j++)
      {
        sInt vertInd = bestTri->Inds[j];
        *indPtr++ = vertInd;

        VCacheVert *vert = &verts[vertInd];
        
        // find this triangles' entry
        sInt k = 0;
        while(vert->TriList[k] != bestTriInd)
        {
          sVERIFY(k < vert->TrisLeft);
          k++;
        }

        // swap it to the end and decrement # of tris left
        if(--vert->TrisLeft)
          sSwap(vert->TriList[k],vert->TriList[vert->TrisLeft]);
        else if(vert->OpenPos >= 0)
          sSwap(openVerts[vert->OpenPos],openVerts[--openCount]);
      }

      // update cache status
      cache[cacheSize] = cache[cacheSize+1] = cache[cacheSize+2] = -1;

      for(sInt j=0;j<3;j++)
      {
        sInt ind = bestTri->Inds[j];
        cache[cacheSize+2] = ind;

        // find vertex index
        sInt pos;
        for(pos=0;cache[pos]!=ind;pos++);

        // move to front
        for(sInt k=pos;k>0;k--)
          cache[k] = cache[k-1];

        cache[0] = ind;

        // remove sentinel if it wasn't used
        if(pos!=cacheSize+2)
          cache[cacheSize+2] = -1;
      }

      // update vertex scores
      for(sInt i=0;i<cacheSize+3;i++)
      {
        sInt vertInd = cache[i];
        if(vertInd == -1)
          continue;

        VCacheVert *vert = &verts[vertInd];

        vert->Score = val2Score[sMin(vert->TrisLeft,maxValence)];
        if(i < cacheSize)
        {
          vert->CachePos = i;
          vert->Score += pos2Score[i];
        }
        else
          vert->CachePos = -1;

        // also add to open vertices list if the vertex is indeed open
        if(vert->OpenPos<0 && vert->TrisLeft)
        {
          vert->OpenPos = openCount;
          openVerts[openCount++] = vertInd;
        }
      }

      // update triangle scores, find new best triangle
      sInt bestTriScore = -1;
      bestTriInd = -1;

      for(sInt i=0;i<cacheSize;i++)
      {
        if(cache[i] == -1)
          continue;

        const VCacheVert *vert = &verts[cache[i]];

        for(sInt j=0;j<vert->TrisLeft;j++)
        {
          sInt triInd = vert->TriList[j];
          VCacheTri *tri = &tris[triInd];

          sVERIFY(tri->Score != -1);

          sInt score = 0;
          for(sInt k=0;k<3;k++)
            score += verts[tri->Inds[k]].Score;

          tri->Score = score;
          if(score > bestTriScore)
          {
            bestTriScore = score;
            bestTriInd = triInd;
          }
        }
      }
    }
  }

  // cleanup
  delete[] verts;
  delete[] tris;
  delete[] vertTriInd;
  delete[] openVerts;
}

void EngMesh::Job::ReIndexVertices()
{
  sInt *vertOrder = new sInt[VertexCount];
  sInt *vertPos = new sInt[VertexCount];

  // prepare tables
  sCopyMem(vertOrder,VertexBuffer,sizeof(sInt) * VertexCount);
  for(sInt i=0;i<VertexCount;i++)
    vertPos[i] = -1;

  // just go through indices in order, adding vertices as we go
  sInt outCount = 0;

  for(sInt i=0;i<IndexCount;i++)
  {
    sInt v = IndexBuffer[i];
    sInt remap = vertPos[v];
    
    if(remap == -1) // not yet seen
    {
      vertPos[v] = remap = outCount;
      VertexBuffer[outCount++] = vertOrder[v];
    }

    IndexBuffer[i] = remap;
  }

  // add not yet used vertices at the back
  for(sInt i=0;i<VertexCount;i++)
  {
    if(vertPos[i] == -1)
      VertexBuffer[outCount++] = vertOrder[i];
  }

  delete[] vertOrder;
  delete[] vertPos;
}

/****************************************************************************/

void EngMesh::Job::UpdatePlanes(sVector *target,const sVertexTSpace3Big *verts)
{
  sZONE(UpdatePlanes);

  sInt lastPlane = 0;

  // this is somewhat inner-loopish, so you may consider optimizing
  // it... the two levels of indirection may suck.
  sVector *plane = target;
  plane->Init(0,0,0,0);
  plane++;

  for(sInt f=0;f<IndexCount/3;f++)
  {
    if(FaceMap[f] != lastPlane)
    {
      sInt i = f*3;
      const sVertexTSpace3Big *v0 = &verts[VertexBuffer[IndexBuffer[i+0] >> 1]];
      const sVertexTSpace3Big *v1 = &verts[VertexBuffer[IndexBuffer[i+1] >> 1]];
      const sVertexTSpace3Big *v2 = &verts[VertexBuffer[IndexBuffer[i+2] >> 1]];

      // plane vectors
      sF32 d1x = v1->x - v0->x;
      sF32 d1y = v1->y - v0->y;
      sF32 d1z = v1->z - v0->z;
      sF32 d2x = v2->x - v0->x;
      sF32 d2y = v2->y - v0->y;
      sF32 d2z = v2->z - v0->z;

      // normal
      sF32 nx = d1y * d2z - d1z * d2y;
      sF32 ny = d1z * d2x - d1x * d2z;
      sF32 nz = d1x * d2y - d1y * d2x;

      // don't need to normalize this, it's only used for sign tests
      /*// normalize normal
      sF32 len = nx*nx + ny*ny + nz*nz;
      if(len)
      {
        len = sFInvSqrt(len);
        nx *= len;
        ny *= len;
        nz *= len;
      }*/

      // store plane
      plane->x = nx;
      plane->y = ny;
      plane->z = nz;
      plane->w = -(v0->x * nx + v0->y * ny + v0->z * nz);

      plane++;
      lastPlane = FaceMap[f];
    }
  }
}

EngMesh::SVCache *EngMesh::Job::GetSVCache(sInt lightId)
{
  // TODO: maybe implement somewhat better logic here
  for(sInt i=0;i<MAXSVCACHE-1;i++)
    if(!SVCache[i].LightId || SVCache[i].LightId == lightId)
      return &SVCache[i];

  return &SVCache[MAXSVCACHE-1];
}

/****************************************************************************/

sU8 EngMesh::SFaceIn[65536];
sU8 EngMesh::SPartSkip[0x4000];

EngMesh::EngMesh()
{
  Vert = 0;
  BoneInfos = 0;
  CompletelyRigid = sFALSE;
  PartCount = 0;
  PartBBs = 0;
  Preloaded = sFALSE;
  Mtrl.Init();
  Jobs.Init();

  Instances = 0;
  InstanceCount = InstanceAlloc = 0;

  Animation = 0;

#if !sPLAYER
  PrepareWireMaterials();
#endif

#if sINTRO
  sSetMem(SPartSkip,1,sizeof(SPartSkip));
#endif
}

EngMesh::~EngMesh()
{
  for(sInt i=1;i<Mtrl.Count;i++)
    Mtrl[i].Material->Release();

  for(sInt i=0;i<Jobs.Count;i++)
    Jobs[i].Exit();

  _aligned_free(Vert);
  if(!CompletelyRigid)
    delete[] BoneInfos;
  else
    delete[] MatrixInds;

  delete[] PartBBs;
  Mtrl.Exit();
  Jobs.Exit();

  if(Instances)
    _aligned_free(Instances);

  sRelease(Animation);

#if !sPLAYER
  ReleaseWireMaterials();
#endif
}

void EngMesh::Copy(KObject *o)
{
  // there's absolutely no valid reason for EngMeshes to be copied, so don't!
  sVERIFYFALSE;
}

/****************************************************************************/

sInt EngMesh::GetSize()
{
  sInt size = 0;

  if(Vert)        size += sizeof(sVertexTSpace3Big) * VertCount;
  if(!CompletelyRigid)
  {
    if(BoneInfos)   size += sizeof(BoneInfo) * VertCount;
  }
  else
  {
    if(MatrixInds)  size += sizeof(sU16) * VertCount;
  }

  if(PartBBs)     size += sizeof(sF32) * 6 * PartCount;

  for(sInt i=0;i<Jobs.Count;i++)
  {
    Job *job = &Jobs[i];

    if(job->VertexBuffer)   size += sizeof(sInt) * job->VertexCount;
    if(job->IndexBuffer)    size += sizeof(sInt) * job->IndexCount;
    if(job->FaceMap)        size += sizeof(sInt) * (job->IndexCount / 3);
    if(job->Edges)          size += sizeof(SilEdge) * job->EdgeCount;
    if(job->Planes)         size += sizeof(sVector) * job->PlaneCount;
  }

  return size;
}

/****************************************************************************/

#if sLINK_FATMESH

// Converts a GenMesh to a list of jobs and a vertex buffer.
void EngMesh::FromGenMesh(GenMesh *mesh)
{
  mesh->NeedAllNormals();
  CalcPartBoundingBoxes(mesh);
  FillVertexBuffer(mesh);

  Mtrl.Copy(mesh->Mtrl);
  for(sInt i=1;i<Mtrl.Count;i++)
    Mtrl[i].Material->AddRef();

  PrepareJobs(mesh);
}

/****************************************************************************/

// Same as above, but for wireframe mode
void EngMesh::FromGenMeshWire(GenMesh *mesh,sInt wireFlags,sU32 wireMask)
{
  sAABox bbox;

  delete[] PartBBs;

  mesh->NeedAllNormals();
  mesh->CalcBBox(bbox); // calculate mesh bounding box
  PartCount = 1;
  PartBBs = new sF32[6];
  PartBBs[0] = (bbox.Max.x + bbox.Min.x) * 0.5f;
  PartBBs[1] = (bbox.Max.x - bbox.Min.x) * 0.5f;
  PartBBs[2] = (bbox.Max.y + bbox.Min.y) * 0.5f;
  PartBBs[3] = (bbox.Max.y - bbox.Min.y) * 0.5f;
  PartBBs[4] = (bbox.Max.z + bbox.Min.z) * 0.5f;
  PartBBs[5] = (bbox.Max.z - bbox.Min.z) * 0.5f;
  FillVertexBuffer(mesh);

  Mtrl.Init();

  GenMeshMtrl *nullMtrl = Mtrl.Add();
  nullMtrl->Material = 0;
  nullMtrl->Pass = 0;

  if(wireFlags & EWF_EDGES)
    AddWireEdgeJob(mesh,AddMaterial(MtrlWire[0],0),(wireMask >> 0) & 0xff);
  
  if(wireFlags & EWF_FACES)
    AddWireFaceJob(mesh,AddMaterial(MtrlWire[1],0),(wireMask >> 8) & 0xff,0);

  if(wireFlags & EWF_VERTS)
    AddWireVertJob(mesh,AddMaterial(MtrlWire[4],0),(wireMask >> 16) & 0xff);

  if(wireFlags & EWF_HIDDEN)
    AddWireFaceJob(mesh,AddMaterial(MtrlWire[2],2),0,1);

  if(wireFlags & EWF_COLLISION)
    AddWireCollisionJob(mesh,AddMaterial(MtrlWire[3],4));
}

#endif

/****************************************************************************/

// Converts a GenMinMesh to a list of jobs and a vertex buffer.
#if sLINK_MINMESH

void EngMesh::FromGenMinMesh(GenMinMesh *mesh)
{
  mesh->CalcNormals();
  mesh->CalcAdjacency();

  CompletelyRigid = mesh->CompletelyRigid;

  FillVertexBuffer(mesh);
  PrepareJobs(mesh);

  sRelease(Animation);
  Animation = mesh->Animation;
  if(Animation)
    Animation->AddRef();
}

#endif

/****************************************************************************/

// Same as above, but for wireframe mode
#if sLINK_MINMESH
#if !sINTRO
void EngMesh::FromGenMinMeshWire(GenMinMesh *mesh,sInt wireFlags,sU32 wireMask)
{
  mesh->CalcNormals();
  mesh->CalcAdjacency();
  FillVertexBuffer(mesh);

  Mtrl.Init();

  GenMeshMtrl *nullMtrl = Mtrl.Add();
  nullMtrl->Material = 0;
  nullMtrl->Pass = 0;

  if(wireFlags & EWF_EDGES)
    AddWireEdgeJob(mesh,AddMaterial(MtrlWire[0],0),(wireMask >> 0) & 0xff);
  
  if(wireFlags & EWF_FACES)
    AddWireFaceJob(mesh,AddMaterial(MtrlWire[1],0),(wireMask >> 8) & 0xff,0);

  if(wireFlags & EWF_VERTS)
    AddWireVertJob(mesh,AddMaterial(MtrlWire[4],0),(wireMask >> 16) & 0xff);

  if(wireFlags & EWF_HIDDEN)
    AddWireFaceJob(mesh,AddMaterial(MtrlWire[2],2),0,1);

  sRelease(Animation);
  Animation = mesh->Animation;
  if(Animation)
    Animation->AddRef();
}
#endif
#endif

/****************************************************************************/

// Full bone innerloop (FPU variant)
static void FullBoneInner(sVertexTSpace3Big *out,const sVertexTSpace3Big *in,EngMesh::BoneInfo *inInfo,sMatrix *matrices,sInt vc)
{
  for(int i=0;i<vc;i++)
  {
    sMatrix m;
    m.i.x = 0.0f; m.i.y = 0.0f; m.i.z = 0.0f;
    m.j.x = 0.0f; m.j.y = 0.0f; m.j.z = 0.0f;
    m.k.x = 0.0f; m.k.y = 0.0f; m.k.z = 0.0f;
    m.l.x = 0.0f; m.l.y = 0.0f; m.l.z = 0.0f;

    // sum weighted matrices
    for(int j=0;j<4;j++)
    {
      sF32 w = inInfo->Weight[j];
      if(!w)
        break;

      const sMatrix *mat = matrices + inInfo->Matrix[j];

      m.i.x += w*mat->i.x;
      m.i.y += w*mat->i.y;
      m.i.z += w*mat->i.z;
      m.j.x += w*mat->j.x;
      m.j.y += w*mat->j.y;
      m.j.z += w*mat->j.z;
      m.k.x += w*mat->k.x;
      m.k.y += w*mat->k.y;
      m.k.z += w*mat->k.z;
      m.l.x += w*mat->l.x;
      m.l.y += w*mat->l.y;
      m.l.z += w*mat->l.z;
    }

    // transform
    out->x = m.i.x * in->x + m.j.x * in->y + m.k.x * in->z + m.l.x;
    out->y = m.i.y * in->x + m.j.y * in->y + m.k.y * in->z + m.l.y;
    out->z = m.i.z * in->x + m.j.z * in->y + m.k.z * in->z + m.l.z;

    out->nx = m.i.x * in->nx + m.j.x * in->ny + m.k.x * in->nz;
    out->ny = m.i.y * in->nx + m.j.y * in->ny + m.k.y * in->nz;
    out->nz = m.i.z * in->nx + m.j.z * in->ny + m.k.z * in->nz;

    out->sx = m.i.x * in->sx + m.j.x * in->sy + m.k.x * in->sz;
    out->sy = m.i.y * in->sx + m.j.y * in->sy + m.k.y * in->sz;
    out->sz = m.i.z * in->sx + m.j.z * in->sy + m.k.z * in->sz;

    // the vertex shader does renormalization and orthogonalization now

    // copy rest
    out->c = in->c;
    out->u = in->u;
    out->v = in->v;

    out++;
    in++;
    inInfo++;
  }
}

static void RigidInnerSSE_asm(sVertexTSpace3Big *outp,const sVertexTSpace3Big *inp,sU16 *matrixInds,sMatrix *matrices,sInt vc)
{
  static const sU32 __declspec(align(16)) mask[4] = { ~0,~0,~0,0 };

  if(!vc)
    return;

  __asm
  {
    mov       edi, [outp];
    mov       esi, [inp];
    mov       ebx, [matrixInds];
    mov       edx, [matrices];
    mov       ecx, [vc];

    align     16;

vertloop:
    movzx     eax, word ptr [ebx];
    shl       eax, 6;
    add       eax, edx;

    prefetcht0  [esi+144];
    prefetcht0  [eax+64];

    movaps    xmm4, [esi+ 0];
    movaps    xmm0, [eax+ 0];
    movaps    xmm1, [eax+16];
    movaps    xmm2, [eax+32];
    movaps    xmm3, [eax+48];

    // pos
    movaps    xmm5, xmm4;
    movaps    xmm6, xmm4;
    movaps    xmm7, xmm4;
    andps     xmm3, [mask];
    shufps    xmm4, xmm4, 000h;
    shufps    xmm5, xmm5, 055h;
    shufps    xmm6, xmm6, 0aah;
    shufps    xmm7, xmm7, 0ffh;
    mulps     xmm4, xmm0;
    mulps     xmm5, xmm1;
    mulps     xmm6, xmm2;
    addps     xmm3, xmm4;     // xmm3=pos_half

    // normal
    movaps    xmm4, [esi+16];
    addps     xmm5, xmm6;     // xmm5=pos_other_half
    movaps    xmm6, xmm4;
    addps     xmm3, xmm5;     // xmm3=pos
    movaps    xmm5, xmm4;
    shufps    xmm4, xmm4, 000h;
    mulps     xmm7, xmm0;
    shufps    xmm6, xmm6, 055h;
    mulps     xmm4, xmm1;
    mulps     xmm6, xmm2;
    addps     xmm4, xmm7;
    movaps    xmm7, xmm5;
    shufps    xmm5, xmm5, 0aah;
    add       edi, 48;
    shufps    xmm7, xmm7, 0ffh;
    addps     xmm4, xmm6;     // xmm4=normal

    // tangent
    movaps    xmm6, [esi+32];
    mulps     xmm0, xmm5;
    mulps     xmm1, xmm7;
    add       esi, 48;
    movaps    xmm5, xmm6;
    shufps    xmm6, xmm6, 000h;
    addps     xmm0, xmm1;
    add       ebx, 2;
    mulps     xmm2, xmm6;     // xmm0+xmm2=tangent
    movaps    xmm1, xmm4;
    shufps    xmm4, xmm4, 03fh;
    addps     xmm0, xmm2;     // xmm0=tangent
    shufps    xmm1, xmm1, 0f9h;
    addps     xmm3, xmm4;     // xmm3=first 4 final
    movlhps   xmm1, xmm0;     // xmm1=mid 4 final
    movhlps   xmm0, xmm0;
    dec       ecx;

    movntps   [edi-48], xmm3;
    movntps   [edi-32], xmm1;
    movss     xmm5, xmm0;     // xmm5=last 4 final
    movntps   [edi-16], xmm5;

    jnz       vertloop;
  }
}

#pragma warning (push)
#pragma warning (disable: 4731) // frame pointer modified by asm code

static void FullBoneInnerSSE_asm(sVertexTSpace3Big *outp,const sVertexTSpace3Big *inp,EngMesh::BoneInfo *inInfo,sMatrix *matrices,sInt vc)
{
  if(!vc)
    return;

  __asm
  {
    mov       edi, [outp];
    mov       esi, [inp];
    mov       ebx, [inInfo];
    mov       edx, [matrices];
    push      ebp;
    mov       ebp, [vc];
    add       ebx, 4;

    align     16;

vertloop:
    movzx     eax, word ptr [ebx+12];
    movss     xmm4, [ebx-4];
    shl       eax, 6;
    shufps    xmm4, xmm4, 0c0h; // (w,w,w,0)
    add       eax, edx;
    mov       ecx, 6;
    push      ebx;

    prefetcht0  [ebx+32];
    prefetcht0  [esi+96];

    movaps    xmm0, [eax+ 0];
    movaps    xmm1, [eax+16];
    movaps    xmm2, [eax+32];
    movaps    xmm3, [eax+48];
    mulps     xmm0, xmm4;
    mulps     xmm1, xmm4;
    mulps     xmm2, xmm4;
    mulps     xmm3, xmm4;

weightmats:
    mov       eax, [ebx];
    test      eax, eax;
    jz        matsdone;

    movzx     eax, word ptr [ebx+ecx+8];
    movss     xmm4, [ebx];
    shl       eax, 6;
    shufps    xmm4, xmm4, 0c0h; // (w,w,w,0)
    add       eax, edx;

    movaps    xmm5, xmm4;
    movaps    xmm6, xmm4;
    movaps    xmm7, xmm4;
    add       ebx, 4;
    mulps     xmm4, [eax+ 0];
    mulps     xmm5, [eax+16];
    mulps     xmm6, [eax+32];
    mulps     xmm7, [eax+48];
    sub       ecx, 2;
    addps     xmm0, xmm4;
    addps     xmm1, xmm5;
    addps     xmm2, xmm6;
    addps     xmm3, xmm7;

    jnz       weightmats;

matsdone:
    pop       ebx;

    // pos
    movaps    xmm4, [esi+ 0];
    movaps    xmm5, xmm4;
    movaps    xmm6, xmm4;
    movaps    xmm7, xmm4;
    shufps    xmm4, xmm4, 000h; // in.px
    shufps    xmm5, xmm5, 055h; // in.py
    shufps    xmm6, xmm6, 0aah; // in.pz
    shufps    xmm7, xmm7, 0ffh; // in.nx
    mulps     xmm4, xmm0;       // pi
    mulps     xmm5, xmm1;       // pj
    mulps     xmm6, xmm2;       // pk
    addps     xmm3, xmm4;       // xmm3=pl+pi

    // normal
    movaps    xmm4, [esi+16];
    addps     xmm5, xmm6;       // xmm5=pj+pk
    movaps    xmm6, xmm4;
    addps     xmm3, xmm5;       // xmm3=pos
    movaps    xmm5, xmm4;
    shufps    xmm4, xmm4, 000h; // in.ny
    mulps     xmm7, xmm0;       // ni
    shufps    xmm6, xmm6, 055h; // in.nz
    mulps     xmm4, xmm1;       // nj
    mulps     xmm6, xmm2;       // nk
    addps     xmm4, xmm7;       // xmm4=nj+ni
    movaps    xmm7, xmm5;
    shufps    xmm5, xmm5, 0aah; // in.sx
    add       edi, 48;
    addps     xmm4, xmm6;       // xmm4=normal
    movaps    xmm6, [esi+32];
    shufps    xmm7, xmm7, 0ffh; // in.sy

    // tangent
    mulps     xmm0, xmm5;       // ti
    shufps    xmm4, xmm4, 039h; // (normal.y,normal.z,0,normal.x)
    mulps     xmm1, xmm7;       // tj
    movaps    xmm5, xmm6;
    shufps    xmm6, xmm6, 000h; // in.sz
    addps     xmm0, xmm1;       // ti+tj
    shufps    xmm1, xmm4, 0efh; // (0,0,0,normal.x)
    mulps     xmm2, xmm6;       // tk
    addps     xmm3, xmm1;       // xmm3=first 4 final
    addps     xmm0, xmm2;       // xmm0=tangent
    movntps   [edi-48], xmm3;
    movlhps   xmm4, xmm0;       // xmm4=mid 4 final
    add       ebx, 24;
    movntps   [edi-32], xmm4;
    movhlps   xmm0, xmm0;
    add       esi, 48;
    movss     xmm5, xmm0;       // xmm5=last 4 final
    dec       ebp;
    movntps   [edi-16], xmm5;

    jnz       vertloop;

    pop       ebp;
  }
}

#pragma warning (pop)

// Do the bone transforms
void *EngMesh::EvalBones(sMatrix *matrices,sGrowableMemStack &alloc)
{
#if BONES
  sZONE(EvalBones);

  if(!BoneInfos) // && !MatrixInds
    return Vert;

  sVertexTSpace3Big *vert = alloc.Alloc<sVertexTSpace3Big>(VertCount);
  sVertexTSpace3Big *out = vert;
  const sVertexTSpace3Big *in = Vert;
  BoneInfo *inInfo = BoneInfos;

  if(CompletelyRigid)
  {
    /*static sU64 totalClocks = 0;
    static sU32 totalVerts = 0;

    totalVerts += VertCount;
    __asm
    {
      rdtsc;
      sub     dword ptr [totalClocks+0], eax;
      sbb     dword ptr [totalClocks+4], edx;
    }

    for(sInt i=0;i<VertCount;i++)
    {
      const sMatrix *m = matrices + inInfo->Matrix[0];

      out->x = m->i.x * in->x + m->j.x * in->y + m->k.x * in->z + m->l.x;
      out->y = m->i.y * in->x + m->j.y * in->y + m->k.y * in->z + m->l.y;
      out->z = m->i.z * in->x + m->j.z * in->y + m->k.z * in->z + m->l.z;

      out->nx = m->i.x * in->nx + m->j.x * in->ny + m->k.x * in->nz;
      out->ny = m->i.y * in->nx + m->j.y * in->ny + m->k.y * in->nz;
      out->nz = m->i.z * in->nx + m->j.z * in->ny + m->k.z * in->nz;

      out->sx = m->i.x * in->sx + m->j.x * in->sy + m->k.x * in->sz;
      out->sy = m->i.y * in->sx + m->j.y * in->sy + m->k.y * in->sz;
      out->sz = m->i.z * in->sx + m->j.z * in->sy + m->k.z * in->sz;

      // copy rest
      out->c = in->c;
      out->u = in->u;
      out->v = in->v;

      out++;
      in++;
      inInfo++;
    }

    __asm
    {
      rdtsc;
      add     dword ptr [totalClocks+0], eax;
      adc     dword ptr [totalClocks+4], edx;
    }

    if(totalVerts > 100000)
    {
      sF64 avgTime = 1.0f * totalClocks / totalVerts;
      sDPrintF("rigid avg. clocks/vertex: %.2f\n",avgTime);

      totalVerts = 0;
      totalClocks = 0;
    }*/
    RigidInnerSSE_asm(out,in,MatrixInds,matrices,VertCount);
  }
  else
  {
    //FullBoneInner(out,in,inInfo,matrices,VertCount);
    FullBoneInnerSSE_asm(out,in,inInfo,matrices,VertCount);
  }

  return vert;
#else
  return Vert;
#endif
}

/****************************************************************************/

static sVector SPlaneBuffer[262144];

// Instancing inner loop
static void InstancingInner(sVertexTSpace3Big *vp,const sVertexTSpace3Big *vert,const sInt *vind,sInt vc,const sMatrix *mat,sInt inst)
{
  while(inst--)
  {
    for(sInt i=0;i<vc;i++)
    {
      const sVertexTSpace3Big *s = &vert[vind[i]];
      vp->x = s->x*mat->i.x + s->y*mat->j.x + s->z*mat->k.x + mat->l.x;
      vp->y = s->x*mat->i.y + s->y*mat->j.y + s->z*mat->k.y + mat->l.y;
      vp->z = s->x*mat->i.z + s->y*mat->j.z + s->z*mat->k.z + mat->l.z;
      vp->nx = s->nx*mat->i.x + s->ny*mat->j.x + s->nz*mat->k.x;
      vp->ny = s->nx*mat->i.y + s->ny*mat->j.y + s->nz*mat->k.y;
      vp->nz = s->nx*mat->i.z + s->ny*mat->j.z + s->nz*mat->k.z;
      vp->sx = s->sx*mat->i.x + s->sy*mat->j.x + s->sz*mat->k.x;
      vp->sy = s->sx*mat->i.y + s->sy*mat->j.y + s->sz*mat->k.y;
      vp->sz = s->sx*mat->i.z + s->sy*mat->j.z + s->sz*mat->k.z;
      vp->c = s->c;
      vp->u = s->u;
      vp->v = s->v;
      vp++;
    }

    mat++;
  }
}

static void VertCopyInner(sVertexTSpace3Big *vp,const sVertexTSpace3Big *vert,sInt *vind,sInt vc)
{
  sZONE(VertCopy);

#if !sINTRO
  __asm
  {
    mov     edi, [vp];
    mov     esi, [vind];
    mov     ebx, [vert];
    mov     ecx, [vc];
    and     ecx, not 1;
    jz      vert_copy_tail;
    shl     ecx, 2;
    add     esi, ecx;
    neg     ecx;

vert_copy_main:
    mov     eax, [esi+ecx];
    mov     edx, [esi+ecx+4];
    shl     eax, 4;
    shl     edx, 4;
    lea     eax, [eax*2+eax];
    lea     edx, [edx*2+edx];
    add     eax, ebx;
    add     edx, ebx;

    prefetcht0  [eax+48*4];
    prefetcht0  [edx+48*4];

    movq    mm0, [eax+ 0];
    movq    mm1, [eax+ 8];
    movq    mm2, [eax+16];
    movq    mm3, [eax+24];
    movntq  [edi+ 0], mm0;
    movntq  [edi+ 8], mm1;
    movntq  [edi+16], mm2;
    movntq  [edi+24], mm3;
    movq    mm0, [eax+32];
    movq    mm1, [eax+40];
    movq    mm2, [edx+ 0];
    movq    mm3, [edx+ 8];
    movntq  [edi+32], mm0;
    movntq  [edi+40], mm1;
    movntq  [edi+48], mm2;
    movntq  [edi+56], mm3;
    movq    mm0, [edx+16];
    movq    mm1, [edx+24];
    movq    mm2, [edx+32];
    movq    mm3, [edx+40];
    movntq  [edi+64], mm0;
    movntq  [edi+72], mm1;
    movntq  [edi+80], mm2;
    movntq  [edi+88], mm3;

    add     edi, 96;
    add     ecx, 8;
    jnz     vert_copy_main;

vert_copy_tail:
    mov     ecx, [vc];
    and     ecx, 1;
    jz      vert_copy_end;

vert_copy_tail_lp:
    mov     eax, [esi];
    shl     eax, 4;
    lea     eax, [eax*2+eax];
    add     eax, ebx;

    movq    mm0, [eax+ 0];
    movq    mm1, [eax+ 8];
    movq    mm2, [eax+16];
    movq    mm3, [eax+24];
    movq    mm4, [eax+32];
    movq    mm5, [eax+40];
    movntq  [edi+ 0], mm0;
    movntq  [edi+ 8], mm1;
    movntq  [edi+16], mm2;
    movntq  [edi+24], mm3;
    movntq  [edi+32], mm4;
    movntq  [edi+40], mm5;

    add     esi, 4;
    add     edi, 48;
    dec     ecx;
    jnz     vert_copy_tail_lp;

vert_copy_end:
    emms;
  }
#else
  for(sInt i=0;i<vc;i++)
    vp[i] = vert[vind[i]];
#endif
}

static void VertCopyStencilInner(sVertexXYZW *vp,const sVertexTSpace3Big *vert,sInt *vind,sInt vc)
{
  static const sU64 andmask = 0x00000000ffffffff;
  static const sU64 ormask  = 0x3f80000000000000;

  sZONE(VCopyStencil);

#if !sINTRO
  __asm
  {
    movq    mm6, [ormask];
    movq    mm7, [andmask];
    mov     edi, [vp];
    mov     esi, [vind];
    mov     ebx, [vert];
    mov     ecx, [vc];
    and     ecx, not 1;
    jz      vert_copy_stencil_tail;
    shl     ecx, 2;
    add     esi, ecx;
    neg     ecx;

vert_copy_stencil_main:
    mov     eax, [esi+ecx];
    mov     edx, [esi+ecx+4];
    shl     eax, 4;
    shl     edx, 4;
    lea     eax, [eax*2+eax];
    lea     edx, [edx*2+edx];
    add     eax, ebx;
    add     edx, ebx;

    movq    mm0, [eax+ 0];
    movq    mm1, [eax+ 8];
    movq    mm2, [edx+ 0];
    movq    mm3, [edx+ 8];
    movq    mm4, mm6;
    movq    mm5, mm6;
    pand    mm1, mm7;
    pand    mm3, mm7;
    por     mm4, mm1;
    por     mm5, mm3;
    movntq  [edi+ 0], mm0;
    movntq  [edi+ 8], mm4;
    movntq  [edi+16], mm0;
    movntq  [edi+24], mm1;
    movntq  [edi+32], mm2;
    movntq  [edi+40], mm5;
    movntq  [edi+48], mm2;
    movntq  [edi+56], mm3;

    add     edi, 64;
    add     ecx, 8;
    jnz     vert_copy_stencil_main;

vert_copy_stencil_tail:
    mov     ecx, [vc];
    and     ecx, 1;
    jz      vert_copy_stencil_end;

vert_copy_stencil_tail_lp:
    mov     eax, [esi];
    shl     eax, 4;
    lea     eax, [eax*2+eax];
    add     eax, ebx;

    movq    mm0, [eax+ 0];
    movq    mm1, [eax+ 8];
    movq    mm2, mm6;
    pand    mm1, mm7;
    por     mm2, mm1;

    movntq  [edi+ 0], mm0;
    movntq  [edi+ 8], mm2;
    movntq  [edi+16], mm0;
    movntq  [edi+24], mm1;

    add     esi, 4;
    add     edi, 32;
    dec     ecx;
    jnz     vert_copy_stencil_tail_lp;

vert_copy_stencil_end:
    emms;
  }
#else
  for(sInt i=0;i<vc;i++)
  {
    const sVertexTSpace3Big *src = &vert[vind[i]];

    vp[0].x = src->x;
    vp[0].y = src->y;
    vp[0].z = src->z;
    vp[0].w = 1.0f;
    vp[1].x = src->x;
    vp[1].y = src->y;
    vp[1].z = src->z;
    vp[1].w = 0.0f;
    vp += 2;
  }
#endif
}

// Paints the given job
void EngMesh::PaintJob(sInt jobId,GenMaterialPass *pass,const EngPaintInfo &paintInfo)
{
  sVERIFY(jobId < Jobs.Count);
  Job *job = &Jobs[jobId];

  // skip empty jobs
  if(!job->VertexCount)
    return;

  //sZONE(PaintJob);

  // preparation
  sInt program = job->Program; 

#if SHADOWS
  SVCache *svCache;
  sBool svUpdate;
  if(program == MPP_SHADOW)
  {
    svCache = job->GetSVCache(paintInfo.LightId);
    if(BoneInfos ||
      !paintInfo.LightId || paintInfo.LightId != svCache->LightId
      || paintInfo.LightPos != svCache->LightPos
      || paintInfo.LightRange != svCache->LightRange)
      svUpdate = sTRUE;
    else
      svUpdate = sFALSE;

    svUpdate = sTRUE;

    if(svUpdate)
    {
      sVector *planes;

      if(paintInfo.BoneData)
      {
        job->UpdatePlanes(SPlaneBuffer,(sVertexTSpace3Big *) paintInfo.BoneData);
        planes = SPlaneBuffer;
      }
      else
        planes = job->Planes;

      UpdateShadowCacheJob(job,planes,svCache,paintInfo);
      svCache->LightPos = paintInfo.LightPos;
      svCache->LightRange = paintInfo.LightRange;
      svCache->LightId = paintInfo.LightId;
    }
  }
#endif

  // choose right source vertices
  sVertexTSpace3Big *vert = Vert;
  if(paintInfo.BoneData)
    vert = (sVertexTSpace3Big *) paintInfo.BoneData;

  sInt handle = job->Geometry;
  sBool bigInd = job->VertexCount >= (program == MPP_SHADOW ? 32768 : 65536);
  if(program==MPP_INSTANCES) bigInd = 1;
  sInt bigFlag = bigInd ? sGEO_IND32B : 0;

  // create geobuffers if necessary
  if(handle == sINVALID)
  {
    switch(program)
    {
    case MPP_STATIC:
      if(!BoneInfos)
        handle = sSystem->GeoAdd(sFVF_TSPACE3BIG,sGEO_TRI|sGEO_STATIC|bigFlag);
      else
        handle = sSystem->GeoAdd(sFVF_TSPACE3BIG,sGEO_TRI|sGEO_DYNVB|sGEO_STATIB|bigFlag);
      break;

#if SHADOWS
    case MPP_SHADOW:
      if(!BoneInfos)
        handle = sSystem->GeoAdd(sFVF_XYZW,sGEO_TRI|sGEO_STATVB|sGEO_DYNIB|bigFlag);
      else
        handle = sSystem->GeoAdd(sFVF_XYZW,sGEO_TRI|sGEO_DYNVB|sGEO_DYNIB|bigFlag);
      break;
#endif

    case MPP_SPRITES:
      handle = sSystem->GeoAdd(sFVF_STANDARD,sGEO_QUAD|sGEO_DYNAMIC);
      break;

#if THICKLINES
    case MPP_THICKLINES:
      handle = sSystem->GeoAdd(sFVF_STANDARD,sGEO_QUAD|sGEO_DYNAMIC);
      break;
#endif

    case MPP_INSTANCES:
      handle = sSystem->GeoAdd(sFVF_TSPACE3BIG,sGEO_TRI|sGEO_DYNAMIC|bigFlag);
      break;

    case MPP_INSTANCES_SH:
      handle = sSystem->GeoAdd(sFVF_TSPACE3BIG,sGEO_TRI|sGEO_STATVB|sGEO_DYNIB|bigFlag);
      break;

#if !sPLAYER
    case MPP_WIRELINES:
      if(!BoneInfos)
        handle = sSystem->GeoAdd(sFVF_COMPACT,sGEO_LINE|sGEO_STATIC);
      else
        handle = sSystem->GeoAdd(sFVF_COMPACT,sGEO_LINE|sGEO_DYNAMIC);
      break;

    case MPP_WIREVERTEX:
      handle = sSystem->GeoAdd(sFVF_COMPACT,sGEO_QUAD|sGEO_DYNAMIC);
      break;
#endif

    default:
      sVERIFYFALSE;
    }

    job->Geometry = handle;
  }

  // try to draw
  sInt update = sSystem->GeoDraw(handle);

  // do we need to update first?
  if(update) // yes we do
  {
    switch(program)
    {
    case MPP_STATIC:
      {
        sVertexTSpace3Big *vp;
        sU16 *ip;
        sInt *vind = job->VertexBuffer;
        sInt vc = job->VertexCount;
        sInt ic = job->IndexCount;

        // update geometry
        sSystem->GeoBegin(handle,vc,ic,(sF32 **)&vp,(void **)&ip,update);

        // copy vertices
        if(update & sGEO_VERTEX)
          VertCopyInner(vp,vert,vind,vc);

        // copy indices
        if(update & sGEO_INDEX)
        {
          if(bigInd)
            sCopyMem(ip,job->IndexBuffer,ic*4);
          else
            for(sInt i=0;i<ic;i++)
              ip[i] = job->IndexBuffer[i];
        }

        // draw
        sSystem->GeoEnd(handle);
        sSystem->GeoDraw(handle);

        // ich hoffe das geht so nicht schief - aber sonst uploadet er
        // die vertices mehrfach, wenn sie in mehreren passes gebraucht
        // werden => sehr blöde idee (und lahm)   -ryg
        /*if(BoneInfos)
          sSystem->GeoFlush(handle,sGEO_VERTEX);*/
      }
      break;

#if SHADOWS
    case MPP_SHADOW:
      {
        // draw from cache: get counts
        sInt vc = job->VertexCount;
        sInt ic = svCache->SilIndices;
        if(paintInfo.StencilFlags & sMBF_STENCILZFAIL)
          ic += svCache->CapIndices;

        // vertices
        if(update & sGEO_VERTEX)
        {
          sVertexXYZW *vp;
          sU16 *ip;

          sSystem->GeoBegin(handle,vc*2,ic ? 0 : 3,(sF32 **)&vp,(void **)&ip,ic ? (update & sGEO_VERTEX) : update);
          sInt *vind = job->VertexBuffer;

          VertCopyStencilInner(vp,vert,vind,vc);

          sSystem->GeoEnd(handle);
          sSystem->GeoFlush(handle,sGEO_INDEX);
        }

        // update and draw
        if(ic)
        {
          sU16 *ip;

          // indices
          sSystem->GeoBegin(handle,0,ic,0,(void **)&ip,update & sGEO_INDEX);
          
          if(bigInd)
            sCopyMem(ip,svCache->IndexBuffer,ic*4);
          else
          {
            for(sInt i=0;i<ic;i++)
              ip[i] = svCache->IndexBuffer[i];
          }

          sSystem->GeoEnd(handle);

          // draw, if possible with two-sided stencil
          if(sSystem->GpuMask & sGPU_TWOSIDESTENCIL)
          {
            sMaterial11::SetShadowStates(sMBF_STENCILINDE|sMBF_DOUBLESIDED|paintInfo.StencilFlags);
            sSystem->GeoDraw(handle);
          }
          else
          {
            sMaterial11::SetShadowStates(sMBF_STENCILINC|paintInfo.StencilFlags);
            sSystem->GeoDraw(handle);
            sMaterial11::SetShadowStates(sMBF_STENCILDEC|sMBF_INVERTCULL|paintInfo.StencilFlags);
            sSystem->GeoDraw(handle);
          }

          // flush indices
          sSystem->GeoFlush(handle,/*BoneInfos ? sGEO_VERTEX|sGEO_INDEX : */sGEO_INDEX);

#if 0 && !sINTRO // DEBUG PAINT CODE
          Engine->DebugPaintStart(sFALSE);

          // show non-culled faces
          sInt *vind = job->VertexBuffer;
          sInt firstInd = svCache->SilIndices;
          sInt count = job->IndexCount/3;
          for(sInt i=0;i<count;i++)
          {
            if(1 || SFaceIn[job->FaceMap[i]])
            {
              sVector v0,v1,v2;
              sU32 color = SFaceIn[job->FaceMap[i]] ? 0xff800000 : 0xff808000;

              sInt i0 = job->IndexBuffer[i*3+0];
              sInt i1 = job->IndexBuffer[i*3+1];
              sInt i2 = job->IndexBuffer[i*3+2];
              sVertexTSpace3Big *p0 = &vert[vind[i0]];
              sVertexTSpace3Big *p1 = &vert[vind[i1]];
              sVertexTSpace3Big *p2 = &vert[vind[i2]];
              v0.Init(p0->x,p0->y,p0->z);
              v1.Init(p1->x,p1->y,p1->z);
              v2.Init(p2->x,p2->y,p2->z);

              //Engine->DebugPaintTri(v0,v1,v2,0xff803c00);
              Engine->DebugPaintTri(v0,v1,v2,color);
            }
          }

          // show silhouette edges
          for(sInt i=0;i<svCache->SilIndices;i+=6)
          {
            sInt i0 = svCache->IndexBuffer[i+0];
            sInt i1 = svCache->IndexBuffer[i+1];
            sVertexTSpace3Big *v0 = &vert[vind[i0]];
            sVertexTSpace3Big *v1 = &vert[vind[i1]];

            Engine->DebugPaintLine(v0->x,v0->y,v0->z,v1->x,v1->y,v1->z,0xffff0000);
          }
#endif
        }
      }
      break;
#endif

    case MPP_SPRITES:
      {
        sVertexStandard *vp;
        sInt *vind = job->VertexBuffer;
        sInt count = job->VertexCount;
        sF32 scale = pass->Size;
        sF32 aspect = pass->Aspect;

        // get transform
        sMatrix mat;
        sSystem->GetTransform(sGT_MODELVIEW,mat);
        mat.Trans3();

        // calc local axes
        sVector vx,vy;
        vx.Scale3(mat.i,scale);
        vy.Scale3(mat.j,scale);

        // update geometry
        while(count)
        {
          sInt vc = sMin(count,0x1000);
          sSystem->GeoBegin(handle,vc*4,0,(sF32 **)&vp,0);

          // fill vertex buffer
          for(sInt i=0;i<vc;i++)
          {
            sVertexTSpace3Big *vsrc = &vert[vind[i]];

            // gen verts
            vp[0].x = vsrc->x - vx.x + vy.x;
            vp[0].y = vsrc->y - vx.y + vy.y;
            vp[0].z = vsrc->z - vx.z + vy.z;
            vp[0].nx = 0.0f;
            vp[0].ny = 0.0f;
            vp[0].nz = 1.0f;
            vp[0].u = 0.0f;
            vp[0].v = 0.0f;

            vp[1].x = vsrc->x + vx.x + vy.x;
            vp[1].y = vsrc->y + vx.y + vy.y;
            vp[1].z = vsrc->z + vx.z + vy.z;
            vp[1].nx = 0.0f;
            vp[1].ny = 0.0f;
            vp[1].nz = 1.0f;
            vp[1].u = 1.0f;
            vp[1].v = 0.0f;

            vp[2].x = vsrc->x + vx.x - vy.x;
            vp[2].y = vsrc->y + vx.y - vy.y;
            vp[2].z = vsrc->z + vx.z - vy.z;
            vp[2].nx = 0.0f;
            vp[2].ny = 0.0f;
            vp[2].nz = 1.0f;
            vp[2].u = 1.0f;
            vp[2].v = 1.0f;

            vp[3].x = vsrc->x - vx.x - vy.x;
            vp[3].y = vsrc->y - vx.y - vy.y;
            vp[3].z = vsrc->z - vx.z - vy.z;
            vp[3].nx = 0.0f;
            vp[3].ny = 0.0f;
            vp[3].nz = 1.0f;
            vp[3].u = 0.0f;
            vp[3].v = 1.0f;
            
            vp += 4;
          }

          // draw
          sSystem->GeoEnd(handle);
          sSystem->GeoDraw(handle);
          sSystem->GeoFlush(handle);

          // update counters
          count -= vc;
          vind += vc;
        }
      }
      break;

#if THICKLINES
    case MPP_THICKLINES:
      {
        sVertexStandard *vp;
        sInt *vind = job->VertexBuffer;
        sInt count = job->VertexCount;
        sF32 s0 = pass->Size * pass->Aspect;
        sF32 s1 = pass->Size / pass->Aspect;

        // get transform
        sMatrix mat;
        sSystem->GetTransform(sGT_MODELVIEW,mat);

        // update geometry
        while(count)
        {
          sInt vc = sMin(count,0x2000);
          sSystem->GeoBegin(handle,vc*2,0,(sF32 **)&vp,0);

          // fill vertex buffer
          for(sInt i=0;i<vc;i+=2)
          {
            sVertexTSpace3Big *v0 = &vert[vind[i+0]];
            sVertexTSpace3Big *v1 = &vert[vind[i+1]];

            // calc transformed positions and scales
            sVector p0,p1;
            p0.Rotate34(mat,(sVector &)v0->x);
            p1.Rotate34(mat,(sVector &)v1->x);

            sF32 ip0z,ip1z;
            ip0z = 1.0f / sMax(p0.z,0.01f);
            ip1z = 1.0f / sMax(p1.z,0.01f);

            // calc projected direction and normalize it
            sF32 pdx,pdy,ilen;
            pdx = p0.x * ip0z - p1.x * ip1z;
            pdy = p0.y * ip0z - p1.y * ip1z;

            ilen = sFInvSqrt(pdx*pdx + pdy*pdy);
            pdx *= ilen;
            pdy *= ilen;

            // calc local axes
            sVector vx,vy;
            vx.x =  mat.i.y * pdx * s0 - mat.i.x * pdy * s0;
            vx.y =  mat.j.y * pdx * s0 - mat.j.x * pdy * s0;
            vx.z =  mat.k.y * pdx * s0 - mat.k.x * pdy * s0;
            vy.x = -mat.i.x * pdx * s1 - mat.i.y * pdy * s1;
            vy.y = -mat.j.x * pdx * s1 - mat.j.y * pdy * s1;
            vy.z = -mat.k.x * pdx * s1 - mat.k.y * pdy * s1;

            // gen verts
            vp[0].x = v1->x - vx.x + vy.x;
            vp[0].y = v1->y - vx.y + vy.y;
            vp[0].z = v1->z - vx.z + vy.z;
            vp[0].nx = 0.0f;
            vp[0].ny = 0.0f;
            vp[0].nz = 1.0f;
            vp[0].u = 0.0f;
            vp[0].v = 0.0f;

            vp[1].x = v1->x + vx.x + vy.x;
            vp[1].y = v1->y + vx.y + vy.y;
            vp[1].z = v1->z + vx.z + vy.z;
            vp[1].nx = 0.0f;
            vp[1].ny = 0.0f;
            vp[1].nz = 1.0f;
            vp[1].u = 1.0f;
            vp[1].v = 0.0f;

            vp[2].x = v0->x + vx.x - vy.x;
            vp[2].y = v0->y + vx.y - vy.y;
            vp[2].z = v0->z + vx.z - vy.z;
            vp[2].nx = 0.0f;
            vp[2].ny = 0.0f;
            vp[2].nz = 1.0f;
            vp[2].u = 1.0f;
            vp[2].v = 1.0f;

            vp[3].x = v0->x - vx.x - vy.x;
            vp[3].y = v0->y - vx.y - vy.y;
            vp[3].z = v0->z - vx.z - vy.z;
            vp[3].nx = 0.0f;
            vp[3].ny = 0.0f;
            vp[3].nz = 1.0f;
            vp[3].u = 0.0f;
            vp[3].v = 1.0f;
            vp += 4;
          }

          // draw
          sSystem->GeoEnd(handle);
          sSystem->GeoDraw(handle);
          sSystem->GeoFlush(handle);

          // update counters
          count -= vc;
          vind += vc;
        }
      }
      break;
#endif

    case MPP_INSTANCES:     // this is inefficient, but it won't tear the engine apart.
      if(InstanceCount > 0)
      {
        sZONE(InstancePrg);

        sVertexTSpace3Big *vp;
        sU16 *ip;
        sInt *vind = job->VertexBuffer;
        sInt vc = job->VertexCount;
        sInt ic = job->IndexCount;
        sInt inst = InstanceCount;

        const sInt maxinst = sMin<sInt>(MAX_DYNVBSIZE/sizeof(*vp)/vc,MAX_DYNIBSIZE/2/ic);

        if(inst>maxinst) inst = maxinst;

        // update geometry
        sSystem->GeoBegin(handle,vc*inst,ic*inst,(sF32 **)&vp,(void **)&ip,update);

        // copy vertices
        if(update & sGEO_VERTEX)
          InstancingInner(vp,vert,vind,vc,Instances,inst);

        // copy indices
        if(update & sGEO_INDEX)
        {
          sInt io = 0;
          sU32 *ip32 = (sU32 *) ip;
          for(sInt j=0;j<inst;j++)
          {
            if(bigInd)
            {
              for(sInt i=0;i<ic;i++)
                ip32[i] = job->IndexBuffer[i]+io;
              ip32 += ic;
            }
            else
            {
              for(sInt i=0;i<ic;i++)
                ip[i] = job->IndexBuffer[i]+io;
              ip += ic;
            }

            io += vc;
          }
        }

        // draw
        sSystem->GeoEnd(handle);
        sSystem->GeoDraw(handle);
      }
      break;

    case MPP_INSTANCES_SH: // maybe this is a better idea!
      if(InstanceCount > 0)
      {
        static const sInt numCopies = 76;

        sVertexTSpace3Big *vp;
        sU16 *ip;
        sInt *vind = job->VertexBuffer;
        sInt vc = job->VertexCount;
        sInt ic = job->IndexCount;

        // update geometry
        sSystem->GeoBegin(handle,vc*numCopies,ic*numCopies,(sF32 **)&vp,(void **)&ip,update);

        // copy vertices
        if(update & sGEO_VERTEX)
        {
          for(sInt copy=0;copy<numCopies;copy++)
          {
            for(sInt i=0;i<vc;i++)
            {
              *vp = vert[vind[i]];
              vp->c = (vp->c & 0xffffff) | ((copy*3)<<24);
              vp++;
            }
          }
        }

        // copy indices
        if(update & sGEO_INDEX)
        {
          sInt offset = 0;
          for(sInt copy=0;copy<numCopies;copy++)
          {
            if(bigInd)
            {
              for(sInt i=0;i<ic;i++)
                ((sU32 *) ip)[i] = job->IndexBuffer[i] + offset;

              ip += 2*ic; // 2* because of sU16 ip
            }
            else
            {
              for(sInt i=0;i<ic;i++)
                ip[i] = job->IndexBuffer[i] + offset;

              ip += ic;
            }

            offset += vc;
          }
        }

        // draw
        sSystem->GeoEnd(handle);

        // now, PAINT! (paint paint paint paint...)
        const sMatrix *instanceMat = Instances;

        for(sInt inst=0;inst<InstanceCount;inst+=numCopies)
        {
          sVector para[numCopies*3];
          sInt count = sMin(InstanceCount-inst,numCopies);

          // extract the constants
          sVector *paraOut = para;

          for(sInt i=0;i<count;i++)
          {
            paraOut->x = instanceMat->i.x;
            paraOut->y = instanceMat->j.x;
            paraOut->z = instanceMat->k.x;
            paraOut->w = instanceMat->l.x;
            paraOut++;

            paraOut->x = instanceMat->i.y;
            paraOut->y = instanceMat->j.y;
            paraOut->z = instanceMat->k.y;
            paraOut->w = instanceMat->l.y;
            paraOut++;

            paraOut->x = instanceMat->i.z;
            paraOut->y = instanceMat->j.z;
            paraOut->z = instanceMat->k.z;
            paraOut->w = instanceMat->l.z;
            paraOut++;
            instanceMat++;
          }

          // set the constants and go!
          sSystem->MtrlSetVSConstants(27,para,count*3);
          sSystem->GeoDraw(handle,ic*count);
        }

        sSystem->GeoFlush(handle,sGEO_INDEX); // this is a hack.
      }
      break;


#if !sPLAYER
    case MPP_WIRELINES:
      {
        sVertexCompact *vp;
        sInt *vind = job->VertexBuffer;
        sInt vc = job->VertexCount;
        sF32 extrude = pass->Size;

        // update geometry
        sSystem->GeoBegin(handle,vc,0,(sF32 **)&vp,0);

        // fill vertex buffer
        for(sInt i=0;i<vc;i++)
        {
          sVertexTSpace3Big *vsrc = &vert[vind[i]];

          vp[i].x = vsrc->x + vsrc->nx * extrude;
          vp[i].y = vsrc->y + vsrc->ny * extrude;
          vp[i].z = vsrc->z + vsrc->nz * extrude;
          vp[i].c0 = 0;
        }

        // draw
        sSystem->GeoEnd(handle);
        sSystem->GeoDraw(handle);
      }
      break;

    case MPP_WIREVERTEX:
      {
        sVertexCompact *vp;
        sInt *vind = job->VertexBuffer;
        sInt count = job->VertexCount;
        sF32 scale = pass->Size;
        sF32 extrude = pass->Aspect;

        // get transform
        sMatrix mat;
        sSystem->GetTransform(sGT_MODELVIEW,mat);
        mat.Trans4();

        while(count)
        {
          sInt vc = sMin(count,0x3f00);

          // update geometry
          sSystem->GeoBegin(handle,vc*4,0,(sF32 **)&vp,0);

          // fill vertex buffer
          for(sInt i=0;i<vc;i++)
          {
            sVertexTSpace3Big *vsrc = &vert[vind[i]];
            sF32 projZ = vsrc->x*mat.k.x + vsrc->y*mat.k.y + vsrc->z*mat.k.z + mat.k.w;
            sF32 size = (sFAbs(projZ) + 0.001f) * scale;
            sVector vx,vy,n;

            // extrude
            n.x = vsrc->nx * extrude;
            n.y = vsrc->ny * extrude;
            n.z = vsrc->nz * extrude;

            // calc local axes
            vx.Scale3(mat.i,size);
            vy.Scale3(mat.j,size);

            // gen verts
            vp[0].x = vsrc->x - vx.x + vy.x + n.x;
            vp[0].y = vsrc->y - vx.y + vy.y + n.y;
            vp[0].z = vsrc->z - vx.z + vy.z + n.z;
            vp[0].c0 = vsrc->c;

            vp[1].x = vsrc->x + vx.x + vy.x + n.x;
            vp[1].y = vsrc->y + vx.y + vy.y + n.y;
            vp[1].z = vsrc->z + vx.z + vy.z + n.z;
            vp[1].c0 = vsrc->c;

            vp[2].x = vsrc->x + vx.x - vy.x + n.x;
            vp[2].y = vsrc->y + vx.y - vy.y + n.y;
            vp[2].z = vsrc->z + vx.z - vy.z + n.z;
            vp[2].c0 = vsrc->c;

            vp[3].x = vsrc->x - vx.x - vy.x + n.x;
            vp[3].y = vsrc->y - vx.y - vy.y + n.y;
            vp[3].z = vsrc->z - vx.z - vy.z + n.z;
            vp[3].c0 = vsrc->c;
            
            vp += 4;
          }

          // draw
          sSystem->GeoEnd(handle);
          sSystem->GeoDraw(handle);
          sSystem->GeoFlush(handle);

          // update counters
          count -= vc;
          vind += vc;
        }
      }
      break;
#endif

    default:
      sVERIFYFALSE;
    }
  }
}

/****************************************************************************/

// Create vertex/index buffers for the mesh and free Vertex array if it's
// no longer required.
void EngMesh::Preload()
{
  if(Preloaded)
    return;

  sMaterialEnv env;
  env.Init();

  sViewport vp;
  vp.Init();
  vp.Window.Init(0,0,1,1);
  sSystem->SetViewport(vp);

  sBool mayDeleteVert = BoneInfos ? sFALSE : sTRUE;
  EngPaintInfo paintInfo;

  paintInfo.LightId = 0;
  paintInfo.LightPos.Init(0,0,0,0);
  paintInfo.LightRange = 1;
  paintInfo.StencilFlags = 0;
  paintInfo.BoneData = 0;

  // paint everything to preload
  for(sInt i=1;i<Mtrl.Count;i++)
  {
    GenMaterial *mtrl = Mtrl[i].Material;

    for(sInt j=0;j<mtrl->Passes.Count;j++)
    {
      GenMaterialPass *pass = &mtrl->Passes[j];

      if(pass->Program != MPP_STATIC && pass->Program != MPP_SHADOW)
        mayDeleteVert = sFALSE;
      else
      {
        pass->Mtrl->Set(env);
        PaintJob(Mtrl[i].JobIds[j],pass,paintInfo);
      }
    }
  }

  // now, we can delete data that has become unnecessary if the mesh
  // is static
  if(!BoneInfos)
  {
    for(sInt i=0;i<Jobs.Count;i++)
    {
      switch(Jobs[i].Program)
      {
      case MPP_STATIC:
        sDeleteArray(Jobs[i].VertexBuffer);
        sDeleteArray(Jobs[i].IndexBuffer);
        break;

      case MPP_SHADOW:
        sDeleteArray(Jobs[i].VertexBuffer);
        break;
      }
    }
  }

  // we can also delete vertex data if it's not needed anymore
  if(mayDeleteVert)
  {
    _aligned_free(Vert);
    Vert = 0;
  }

  Preloaded = sTRUE;
}

/****************************************************************************/

sMatrix *EngMesh::GetInstanceMatrices(sInt count)
{
  InstanceCount = count;
  if(InstanceAlloc < count)
  {
    InstanceAlloc = count;

    _aligned_free(Instances);
    Instances = (sMatrix *) _aligned_malloc(count * sizeof(sMatrix),16);
  }

  return Instances;
}

void EngMesh::UpdateInstanceCount(sInt count)
{
  sVERIFY(count <= InstanceCount);
  InstanceCount = count;
}

/****************************************************************************/

void EngMesh::SetWireColors(sU32 *colors)
{
  if(WireInit)
  {
    ((sSimpleMaterial *) MtrlWire[0]->Passes[0].Mtrl)->Color = colors[EWC_WIRELOW];
    ((sSimpleMaterial *) MtrlWire[0]->Passes[1].Mtrl)->Color = colors[EWC_WIREHIGH];
    ((sSimpleMaterial *) MtrlWire[1]->Passes[0].Mtrl)->Color = colors[EWC_FACE];
    ((sSimpleMaterial *) MtrlWire[2]->Passes[0].Mtrl)->Color = colors[EWC_HIDDEN];
    ((sSimpleMaterial *) MtrlWire[3]->Passes[0].Mtrl)->Color = colors[EWC_COLADD];
    ((sSimpleMaterial *) MtrlWire[3]->Passes[1].Mtrl)->Color = colors[EWC_COLSUB];
    ((sSimpleMaterial *) MtrlWire[3]->Passes[2].Mtrl)->Color = colors[EWC_COLZONE];
    ((sSimpleMaterial *) MtrlWire[4]->Passes[0].Mtrl)->Color = colors[EWC_VERTEX];
  }
}

/****************************************************************************/

sInt EngMesh::AddMaterial(GenMaterial *genMtrl,sInt pass)
{
  // save id, create new material
  sInt id = Mtrl.Count;
  GenMeshMtrl *mtrl = Mtrl.Add();

  // fill values and return
  mtrl->Material = genMtrl;
  mtrl->Material->AddRef();
  mtrl->Pass = pass;

  return id;
}

sInt EngMesh::AddJob(sInt mtrlId,sInt program)
{
  // save id, create new job
  sInt id = Jobs.Count;
  Job *job = Jobs.Add();

  // fill out job and return
  job->Init();
  job->MtrlId = mtrlId;
  job->Program = program;

  return id;
}

/****************************************************************************/

#if sLINK_FATMESH

// Convert a meshes' vertices into a compact vertex list.
void EngMesh::FillVertexBuffer(GenMesh *mesh)
{
  _aligned_free(Vert);
  //delete[] Vert;
  VertCount = mesh->Vert.Count;
  Vert = (sVertexTSpace3Big *) _aligned_malloc(mesh->Vert.Count * sizeof(sVertexTSpace3Big),16);
  //Vert = new sVertexTSpace3Big[mesh->Vert.Count];
  
  sVertexTSpace3Big *outVert = Vert;
  sVector *srcVert = mesh->VertBuf;
  sInt vuv = mesh->VertMap(sGMI_UV0);
  sInt vnr = mesh->VertMap(sGMI_NORMAL);
  sInt vtn = mesh->VertMap(sGMI_TANGENT);
  sInt vco = mesh->VertMap(sGMI_COLOR0);
  
  for(sInt i=0;i<mesh->Vert.Count;i++)
  {
    outVert->x = srcVert[0].x;
    outVert->y = srcVert[0].y;
    outVert->z = srcVert[0].z;
    outVert->nx = srcVert[vnr].x;
    outVert->ny = srcVert[vnr].y;
    outVert->nz = srcVert[vnr].z;
    outVert->sx = srcVert[vtn].x;
    outVert->sy = srcVert[vtn].y;
    outVert->sz = srcVert[vtn].z;
    outVert->c = PackColor (srcVert + vco);
    outVert->u = srcVert[vuv].x;
    outVert->v = srcVert[vuv].y;

    srcVert += mesh->VertSize();
    outVert++;
  }
}

// Compute bounding boxes for all parts in the mesh
void EngMesh::CalcPartBoundingBoxes(GenMesh *mesh)
{
  delete[] PartBBs;

  PartCount = mesh->Parts.Count;
  PartBBs = new sF32[PartCount * 6];

  sF32 *outPtr = PartBBs;
  for(sInt i=0;i<PartCount;i++)
  {
    sInt end = (i == PartCount-1) ? mesh->Face.Count : mesh->Parts[i+1];
    sVector min,max;

    min.Init( 1e+20f, 1e+20f, 1e+20f,1.0f);
    max.Init(-1e+20f,-1e+20f,-1e+20f,1.0f);

    // find min/max values for the current part
    for(sInt j=mesh->Parts[i];j<end;j++)
    {
      sInt e,ee;

      e = ee = mesh->Face[j].Edge;
      do
      {
        const sVector &v = mesh->VertPos(mesh->GetVertId(e));
        min.x = sMin(min.x,v.x);
        min.y = sMin(min.y,v.y);
        min.z = sMin(min.z,v.z);
        max.x = sMax(max.x,v.x);
        max.y = sMax(max.y,v.y);
        max.z = sMax(max.z,v.z);

        e = mesh->NextFaceEdge(e);
      }
      while(e != ee);
    }

    // output box
    for(sInt j=0;j<3;j++)
    {
      *outPtr++ = (max[j] + min[j]) * 0.5f;
      *outPtr++ = (max[j] - min[j]) * 0.5f;
    }
  }
}

// Calculates a bounding box for the given material id
void EngMesh::CalcBoundingBox(GenMesh *mesh,sInt mtrlId,sAABox &box)
{
  sVector min,max;

  min.Init( 1e+20f, 1e+20f, 1e+20f,1.0f);
  max.Init(-1e+20f,-1e+20f,-1e+20f,1.0f);

  // find extents for this material
  for(sInt i=0;i<mesh->Face.Count;i++)
  {
    sInt faceMtrl = mesh->Face[i].Material;
    if(!faceMtrl || mtrlId != -1 && faceMtrl != mtrlId)
      continue;

    sInt e,ee;
    e = ee = mesh->Face[i].Edge;
    do
    {
      const sVector &v = mesh->VertPos(mesh->GetVertId(e));
      min.x = sMin(min.x,v.x);
      min.y = sMin(min.y,v.y);
      min.z = sMin(min.z,v.z);
      max.x = sMax(max.x,v.x);
      max.y = sMax(max.y,v.y);
      max.z = sMax(max.z,v.z);

      e = mesh->NextFaceEdge(e);
    }
    while(e != ee);
  }

  box.Min = min;
  box.Max = max;
}

// Prepares all materials for the given mesh
void EngMesh::PrepareJobs(GenMesh *mesh)
{
  sBool firstShadowJob = sTRUE;
  sBool *hasShadow = new sBool[Mtrl.Count];

  hasShadow[0] = sFALSE;
  for(sInt i=1;i<Mtrl.Count;i++)
  {
    GenMaterial *mtrl = Mtrl[i].Material;
    hasShadow[i] = sFALSE;

    for(sInt j=0;j<mtrl->Passes.Count;j++)
      if(mtrl->Passes[j].Program == MPP_SHADOW)
        hasShadow[i] = sTRUE;
  }

  for(sInt i=1;i<Mtrl.Count;i++)
  {
    GenMaterial *mtrl = Mtrl[i].Material;

    // Reuse prepared jobs if possible
    sInt jobIds[MPP_MAX];
    for(sInt j=0;j<MPP_MAX;j++)
      jobIds[j] = -1;

    for(sInt j=0;j<mtrl->Passes.Count;j++)
    {
      GenMaterialPass *pass = &mtrl->Passes[j];
      sInt program = pass->Program;
      sInt jobId = jobIds[program];

      if(jobId == -1) // we haven't seen this program yet
      {
        jobId = AddJob(i,program);
        jobIds[program] = jobId;

        PrepareJob(&Jobs[jobId],mesh,firstShadowJob,hasShadow);
      }

      Mtrl[i].JobIds[j] = jobId;
    }
  }

  delete[] hasShadow;
}

// Prepares a single mesh job
void EngMesh::PrepareJob(Job *job,GenMesh *mesh,sBool &firstShadowJob,const sBool *hasShadow)
{
  // check whether we understand the program used
  sInt program = job->Program;
  sBool isShadow = program == MPP_SHADOW;

  if(isShadow && !firstShadowJob)
    return;

  // compute bounding box
  CalcBoundingBox(mesh,isShadow ? -1 : job->MtrlId,job->BBox);

  switch(program)
  {
  case MPP_SPRITES:
    PrepareSpriteJob(job,mesh);
    return;

#if THICKLINES
  case MPP_THICKLINES:
    PrepareThickLinesJob(job,mesh);
    return;
#endif
  }

  // count faces, edges and vertices
  sInt faceCount,triCount,edgeCount,vertCount;
  mesh->All2Sel(0,MAS_EDGE|MAS_FACE|MAS_VERT);

  faceCount = 0;
  triCount = 0;
  for(sInt i=0;i<mesh->Face.Count;i++)
  {
    mesh->Face[i].Temp = faceCount;
    sInt mtrl = mesh->Face[i].Material;

    if((isShadow && mesh->Face[i].Used && hasShadow[mtrl]) || (!isShadow && mtrl == job->MtrlId))
    //if(mesh->Face[i].Material == job->MtrlId && (!isShadow || mesh->Face[i].Used))
    {
      mesh->Face[i].Select = 1;
      faceCount++;

      sInt e,ee,ec;
      e = ee = mesh->Face[i].Edge;
      ec = 0;
      
      do
      {
        ec++;
        e = mesh->NextFaceEdge(e);
      }
      while(e != ee);

      triCount += ec - 2;
    }
  }

  edgeCount = 0; 
  for(sInt i=0;i<mesh->Edge.Count;i++)
  {
    if(mesh->GetFace(i*2+0)->Select || mesh->GetFace(i*2+1)->Select)
    {
      mesh->Edge[i].Temp[0] = edgeCount;
      edgeCount++;

      mesh->GetVert(i*2+0)->Select = 1;
      mesh->GetVert(i*2+1)->Select = 1;
    }
  }

  vertCount = 0;
  for(sInt i=0;i<mesh->Vert.Count;i++)
  {
    mesh->Vert[i].Temp = vertCount;
    vertCount += mesh->Vert[i].Select;
  }

  // build them buffers!
  job->Alloc(vertCount,triCount * 3,isShadow ? edgeCount : 0,
    isShadow ? triCount : 0,mesh->Parts.Count);

#if SHADOWS
  // prepare face planes if necessary
  if(isShadow)
  {
    job->PlaneCount = faceCount+1;
    job->Planes = new sVector[faceCount+1];
    job->Planes[0].Init(0,0,0,0);

    for(sInt i=0;i<mesh->Face.Count;i++)
      if(mesh->Face[i].Select)
        mesh->CalcFacePlane(job->Planes[mesh->Face[i].Temp+1],i);
  }
#endif

  // generate vertex buffer
  for(sInt i=0;i<mesh->Vert.Count;i++)
  {
    if(mesh->Vert[i].Select)
      job->VertexBuffer[mesh->Vert[i].Temp] = i;
  }

  // generate index+edge buffers
  sInt curPart = 0;

  faceCount = 0;
  triCount = 0;
  edgeCount = 0;
  vertCount = 0;

  for(sInt faceInd=0;faceInd<mesh->Face.Count;faceInd++)
  {
    // update current part
    while(curPart < mesh->Parts.Count-1 && faceInd >= mesh->Parts[curPart+1])
    {
      job->FacePartEnd[curPart] = faceCount;
      job->EdgePartEnd[curPart] = edgeCount;
      curPart++;
    }

    GenMeshFace *curFace = &mesh->Face[faceInd];
    if(!curFace->Select)
      continue;

    // go round face, write edges and triangulate
    sInt v0 = -1,v1 = -1,v2 = -1;
    sInt count = 0,e,ee;

    e = ee = curFace->Edge;

    do
    {
      // vertex
      count++;
      v1 = v2;
      v2 = mesh->GetVert(e)->Temp;
      if(v0 == -1)
        v0 = v2;

      if(count >= 3) // write a face
      {
        job->IndexBuffer[triCount*3+0] = v0;
        job->IndexBuffer[triCount*3+1] = v1;
        job->IndexBuffer[triCount*3+2] = v2;
        if(isShadow)
        {
          job->IndexBuffer[triCount*3+0] *= 2;
          job->IndexBuffer[triCount*3+1] *= 2;
          job->IndexBuffer[triCount*3+2] *= 2;
          job->FaceMap[triCount] = faceCount +1;
        }

        triCount++;

        v1 = v2;
      }

#if SHADOWS
      // edge
      if(isShadow && !mesh->GetEdge(e)->Select)
      {
        mesh->GetEdge(e)->Select = 1;

        SilEdge *edge = &job->Edges[edgeCount++];
        edge->Vert[0] = mesh->GetVert(e)->Temp * 2;
        edge->Vert[1] = mesh->GetVert(mesh->NextFaceEdge(e))->Temp * 2;
        edge->Face[0] = mesh->GetFace(e)->Temp + 1;
        edge->Face[1] = mesh->GetFace(e^1)->Temp + 1;
      }
#endif

      e = mesh->NextFaceEdge(e);
    }
    while(e != ee);

    faceCount++;
  }

  sVERIFY(triCount*3 == job->IndexCount);
  if(isShadow)
    firstShadowJob = sFALSE;

  if(program == MPP_STATIC || program == MPP_INSTANCES || program == MPP_INSTANCES_SH)
  {
#if !sINTRO
    /*job->OptimizeIndices();
    job->ReIndexVertices();*/
#endif
  }
}

// Prepares a sprite job.
void EngMesh::PrepareSpriteJob(Job *job,GenMesh *mesh)
{
  mesh->All2Sel(1,MAS_FACE);
  mesh->Face2Vert();

  // count vertices
  sInt vertCount = 0;
  for(sInt i=0;i<mesh->Vert.Count;i++)
    if(mesh->Vert[i].First == i && mesh->Vert[i].Select)
      vertCount++;

  // create batches
  job->Alloc(vertCount,0,0,0,0);

  vertCount = 0;
  for(sInt i=0;i<mesh->Vert.Count;i++)
    if(mesh->Vert[i].First == i && mesh->Vert[i].Select)
      job->VertexBuffer[vertCount++] = i;
}

// Prepares a thick lines job.
void EngMesh::PrepareThickLinesJob(Job *job,GenMesh *mesh)
{
  sInt edgeCount = 0;

  // select interesting edges and count them
  for(sInt i=0;i<mesh->Edge.Count;i++)
  {
    mesh->Edge[i].Select = mesh->GetFace(i*2)->Material == job->MtrlId
    || mesh->GetFace(i*2+1)->Material == job->MtrlId;

    edgeCount += mesh->Edge[i].Select;
  }

  // build them vertex buffers
  job->Alloc(edgeCount*2,0,0,0,0);

  edgeCount = 0;
  for(sInt i=0;i<mesh->Edge.Count;i++)
  {
    GenMeshEdge *curEdge = &mesh->Edge[i];

    if(curEdge->Select)
    {
      job->VertexBuffer[edgeCount*2+0] = curEdge->Vert[0];
      job->VertexBuffer[edgeCount*2+1] = curEdge->Vert[1];
      edgeCount++;
    }
  }
}

/****************************************************************************/

void EngMesh::AddWireEdgeJob(GenMesh *mesh,sInt mtrl,sU32 mask)
{
  sInt count[2];
  
  // count selected edges
  count[0] = 0;
  count[1] = 0;
  for(sInt i=0;i<mesh->Edge.Count;i++)
  {
    if(mesh->GetFace(i*2)->Material || mesh->GetFace(i*2+1)->Material)
    {
      count[0]++;
      if(mesh->Edge[i].Mask & mask)
        count[1]++;
    }
  }

  count[0] -= count[1];

  // for the selected and unselected case...
  for(sInt sel=0;sel<2;sel++)
  {
    // add the actual job
    sInt jobId = AddJob(mtrl,MPP_WIRELINES);
    Job *job = &Jobs[jobId];
    Mtrl[mtrl].JobIds[sel] = jobId;

    CalcBoundingBox(mesh,-1,job->BBox);

    // fill out the batches
    job->Alloc(count[sel]*2,0,0,0,0);

    sInt counter = 0;
    for(sInt edgeInd=0;edgeInd<mesh->Edge.Count;edgeInd++)
    {
      if(mesh->GetFace(edgeInd*2)->Material || mesh->GetFace(edgeInd*2+1)->Material)
      {
        GenMeshEdge *edge = &mesh->Edge[edgeInd];

        if(((edge->Mask & mask) != 0) == sel)
        {
          job->VertexBuffer[counter*2+0] = edge->Vert[0];
          job->VertexBuffer[counter*2+1] = edge->Vert[1];
          counter++;
        }
      }
    }

    sVERIFY(counter == count[sel]);
  }
}

void EngMesh::AddWireFaceJob(GenMesh *mesh,sInt mtrl,sU32 mask,sU32 compare)
{
  // add job
  sInt jobId = AddJob(mtrl,MPP_STATIC);
  Job *job = &Jobs[jobId];
  Mtrl[mtrl].JobIds[0] = jobId;

  CalcBoundingBox(mesh,-1,job->BBox);

  // count vertices+triangles
  sInt triCount = 0,vertCount = 0;

  mesh->All2Sel(0,MAS_VERT|MAS_FACE);
  for(sInt faceInd = 0;faceInd < mesh->Face.Count;faceInd++)
  {
    GenMeshFace *curFace = &mesh->Face[faceInd];

    if(curFace->Material && ((curFace->Mask & mask) != compare))
    {
      sInt e,ee,count;

      curFace->Select = 1;
      count = 0;
      e = ee = curFace->Edge;

      do
      {
        count++;
        if(!mesh->GetVert(e)->Select)
        {
          mesh->GetVert(e)->Select = 1;
          vertCount++;
        }

        e = mesh->NextFaceEdge(e);
      }
      while(e != ee);

      triCount += count - 2;
    }
  }

  // alloc job, create vertex buffer
  job->Alloc(vertCount,triCount*3,0,0,0);

  vertCount = 0;
  for(sInt i=0;i<mesh->Vert.Count;i++)
  {
    if(mesh->Vert[i].Select)
    {
      mesh->Vert[i].Temp = vertCount;
      job->VertexBuffer[vertCount++] = i;
    }
  }

  sVERIFY(vertCount == job->VertexCount);

  // create index buffer
  triCount = 0;

  for(sInt faceInd=0;faceInd<mesh->Face.Count;faceInd++)
  {
    GenMeshFace *curFace = &mesh->Face[faceInd];
    if(!curFace->Select)
      continue;

    sInt v0,v1,v2 = -1;
    sInt e,ee,count;
    
    e = ee = curFace->Edge;
    count = 0;

    do
    {
      count++;
      v1 = v2;
      v2 = mesh->GetVert(e)->Temp;
      if(count == 1)
        v0 = v2;
      else if(count >= 3)
      {
        job->IndexBuffer[triCount*3+0] = v0;
        job->IndexBuffer[triCount*3+1] = v1;
        job->IndexBuffer[triCount*3+2] = v2;
        triCount++;
      }

      e = mesh->NextFaceEdge(e);
    }
    while(e != ee);
  }

  sVERIFY(triCount * 3 == job->IndexCount);
}

void EngMesh::AddWireVertJob(GenMesh *mesh,sInt mtrl,sU32 mask)
{
  // add job
  sInt jobId = AddJob(mtrl,MPP_WIREVERTEX);
  Job *job = &Jobs[jobId];
  Mtrl[mtrl].JobIds[0] = jobId;

  CalcBoundingBox(mesh,-1,job->BBox);

  // count vertices
  sInt vertCount = 0;

  for(sInt i=0;i<mesh->Vert.Count;i++)
    if(mesh->Vert[i].Mask & mask)
      vertCount++;

  // create job
  job->Alloc(vertCount,0,0,0,0);

  vertCount = 0;
  for(sInt i=0;i<mesh->Vert.Count;i++)
    if(mesh->Vert[i].Mask & mask)
      job->VertexBuffer[vertCount++] = i;
}

void EngMesh::AddWireCollisionJob(GenMesh *mesh,sInt mtrl)
{
  static const sU8 cubeTable[6*4] =
  {
    0,2,3,1, 2,6,7,3, 3,7,5,1, 4,0,1,5, 2,0,4,6, 7,6,4,5
  };

  for(sInt type=0;type<3;type++) // for each collision type
  {
    // add job
    sInt jobId = AddJob(mtrl,MPP_STATIC);
    Job *job = &Jobs[jobId];
    Mtrl[mtrl].JobIds[type] = jobId;
    CalcBoundingBox(mesh,-1,job->BBox);

    // count collision cubes
    sInt collCubes = 0;
    for(sInt i=0;i<mesh->Coll.Count;i++)
      if((mesh->Coll[i].Mode & 3) == type)
        collCubes++;

    // create job
    job->Alloc(collCubes * 8,collCubes * 36,0,0,0);

    // create vertex and index buffer
    for(sInt collInd=0;collInd<mesh->Coll.Count;collInd++)
    {
      GenMeshColl *curColl = &mesh->Coll[collInd];

      if((curColl->Mode & 3) != type)
        continue;

      sInt basev = collInd*8;

      // vertices
      for(sInt j=0;j<8;j++)
        job->VertexBuffer[basev+j] = curColl->Vert[j];

      // indices
      for(sInt j=0;j<6;j++)
      {
        sInt basei = collInd*36 + j*6;
        job->IndexBuffer[basei+0] = basev + cubeTable[j*4+0];
        job->IndexBuffer[basei+1] = basev + cubeTable[j*4+1];
        job->IndexBuffer[basei+2] = basev + cubeTable[j*4+2];
        job->IndexBuffer[basei+3] = basev + cubeTable[j*4+0];
        job->IndexBuffer[basei+4] = basev + cubeTable[j*4+2];
        job->IndexBuffer[basei+5] = basev + cubeTable[j*4+3];
      }
    }
  }
}

#endif

/****************************************************************************/

#if sLINK_MINMESH

void EngMesh::FillVertexBuffer(GenMinMesh *mesh)
{
  _aligned_free(Vert);
  //delete[] Vert;
  VertCount = mesh->Vertices.Count;
  //Vert = new sVertexTSpace3Big[mesh->Vertices.Count];
  Vert = (sVertexTSpace3Big *) _aligned_malloc(mesh->Vertices.Count*sizeof(sVertexTSpace3Big),16);

  sVertexTSpace3Big *outVert = Vert;
  GenMinVert *inVert = &mesh->Vertices[0];

  for(sInt i=0;i<mesh->Vertices.Count;i++)
  {
    outVert->x = inVert->Pos.x;
    outVert->y = inVert->Pos.y;
    outVert->z = inVert->Pos.z;
    outVert->nx = inVert->Normal.x;
    outVert->ny = inVert->Normal.y;
    outVert->nz = inVert->Normal.z;
    outVert->sx = inVert->Tangent.x;
    outVert->sy = inVert->Tangent.y;
    outVert->sz = inVert->Tangent.z;
    outVert->c = inVert->Color;
    outVert->u = inVert->UV[0][0];
    outVert->v = inVert->UV[0][1];

    outVert++;
    inVert++;
  }

  // find out whether we need bones
  sBool needBones = sFALSE;
  for(sInt i=1;i<mesh->Clusters.Count;i++)
    if(mesh->Clusters[i].AnimType == 2)
      needBones = sTRUE;

  // create bone structures
  if(needBones)
  {
    if(!CompletelyRigid)
    {
      BoneInfos = new BoneInfo[mesh->Vertices.Count];
      BoneInfo *outInfo = BoneInfos;
      inVert = &mesh->Vertices[0];

      for(sInt i=0;i<mesh->Vertices.Count;i++)
      {
        sInt boneCount = inVert->BoneCount;

        for(sInt j=0;j<4;j++)
        {
          outInfo->Weight[j] = inVert->Weights[j];
          outInfo->Matrix[j] = inVert->Matrix[j];
        }

        // (insertion) sort bones by weight
        for(sInt j=1;j<boneCount;j++)
        {
          sF32 w = outInfo->Weight[j];
          sU16 m = outInfo->Matrix[j];
          sInt k = j;

          while(k && sFAbs(outInfo->Weight[k-1]) < sFAbs(w))
          {
            outInfo->Weight[k] = outInfo->Weight[k-1];
            outInfo->Matrix[k] = outInfo->Matrix[k-1];
            k--;
          }

          outInfo->Weight[k] = w;
          outInfo->Matrix[k] = m;
        }

        // remove weights that don't have a significant effect
        sF32 wSum = 1.0f;
        static const sF32 thresh = 1.0f/64.0f;
        while(boneCount && sFAbs(outInfo->Weight[boneCount-1]) < thresh)
          wSum -= outInfo->Weight[--boneCount];

        for(sInt i=0;i<boneCount;i++)
          outInfo->Weight[i] /= wSum;

        for(sInt i=boneCount;i<4;i++)
        {
          outInfo->Weight[i] = 0;
          outInfo->Matrix[i] = 0;
        }

        outInfo++;
        inVert++;
      }
    }
    else // CompletelyRigid is set
    {
      MatrixInds = new sU16[mesh->Vertices.Count];
      for(sInt i=0;i<mesh->Vertices.Count;i++)
        MatrixInds[i] = mesh->Vertices[i].Matrix[0];
    }
  }
}

void EngMesh::CalcBoundingBox(GenMinMesh *mesh,sInt clusterId,sAABox &box)
{
  sVector min,max;

  min.Init( 1e+20f, 1e+20f, 1e+20f,1.0f);
  max.Init(-1e+20f,-1e+20f,-1e+20f,1.0f);

  // find extents of this cluster
  for(sInt i=0;i<mesh->Faces.Count;i++)
  {
    GenMinFace *face = &mesh->Faces[i];
    sInt faceCluster = face->Cluster;
    if(!faceCluster || clusterId != -1 && faceCluster != clusterId)
      continue;

    for(sInt j=0;j<face->Count;j++)
    {
      const GenMinVert *v = &mesh->Vertices[face->Vertices[j]];

      min.x = sMin(min.x,v->Pos.x);
      min.y = sMin(min.y,v->Pos.y);
      min.z = sMin(min.z,v->Pos.z);
      max.x = sMax(max.x,v->Pos.x);
      max.y = sMax(max.y,v->Pos.y);
      max.z = sMax(max.z,v->Pos.z);
    }
  }

  box.Min = min;
  box.Max = max;
}

void EngMesh::PrepareJobs(GenMinMesh *mesh)
{
  sBool firstShadowJob = sTRUE;
  sBool *hasShadow = new sBool[mesh->Clusters.Count];

  Mtrl.Resize(mesh->Clusters.Count);
  hasShadow[0] = sFALSE;

  for(sInt i=1;i<mesh->Clusters.Count;i++)
  {
    GenMaterial *mtrl = mesh->Clusters[i].Mtrl;
    hasShadow[i] = sFALSE;

    for(sInt j=0;j<mtrl->Passes.Count;j++)
      if(mtrl->Passes[j].Program == MPP_SHADOW)
        hasShadow[i] = sTRUE;
  }

  for(sInt i=0;i<mesh->Clusters.Count;i++)
  {
    GenMinCluster *cluster = &mesh->Clusters[i];
    GenMeshMtrl *outMtrl = &Mtrl[i];
    GenMaterial *mtrl = cluster->Mtrl;

    outMtrl->Pass = cluster->RenderPass;
    outMtrl->Material = mtrl;
    if(!i) // first material is null material
      continue;

    mtrl->AddRef();

    // Reuse jobs if possible
    sInt jobIds[MPP_MAX];
    for(sInt j=0;j<MPP_MAX;j++)
      jobIds[j] = -1;

    for(sInt j=0;j<mtrl->Passes.Count;j++)
    {
      GenMaterialPass *pass = &mtrl->Passes[j];
      sInt program = pass->Program;
      sInt jobId = jobIds[program];

      if(jobId == -1) //  we haven't seen this program yet
      {
        jobId = AddJob(i,program);
        jobIds[program] = jobId;

        Jobs[jobId].AnimType = cluster->AnimType;
        Jobs[jobId].AnimMatrix = cluster->AnimMatrix;

        PrepareJob(&Jobs[jobId],mesh,firstShadowJob,hasShadow);
      }

      outMtrl->JobIds[j] = jobId;
    }
  }

  delete[] hasShadow;
}

void EngMesh::PrepareJob(Job *job,GenMinMesh *mesh,sBool &firstShadowJob,const sBool *hasShadow)
{
  // prepare program, calc bounding box
  sInt program = job->Program;
  sBool isShadow = program == MPP_SHADOW;
  if(isShadow && !firstShadowJob)
    return;

  CalcBoundingBox(mesh,isShadow ? -1 : job->MtrlId,job->BBox);

  switch(program)
  {
  case MPP_STATIC:
  case MPP_SHADOW:
  case MPP_INSTANCES:
  case MPP_INSTANCES_SH:
    break;

  case MPP_SPRITES:
    PrepareSpriteJob(job,mesh);
    return;

#if THICKLINES
  case MPP_THICKLINES:
    PrepareThickLinesJob(job,mesh);
#endif
    return;

  default:
    return;
  }

  // count faces, edges and vertices
  sInt faceCount,triCount,edgeCount,vertCount;
  
  sInt *vertMap = new sInt[mesh->Vertices.Count];
  sInt *facePlaneMap = new sInt[mesh->Faces.Count];

  memset(vertMap,0xff,mesh->Vertices.Count * sizeof(sInt));
  memset(facePlaneMap,0,mesh->Faces.Count * sizeof(sInt));

  faceCount = 0;
  triCount = 0;
  edgeCount = 0;
  vertCount = 0;

  for(sInt i=0;i<mesh->Faces.Count;i++)
  {
    GenMinFace *curFace = &mesh->Faces[i];
    curFace->Temp = 0;

    if(curFace->Count < 3)
      continue;

    if((isShadow && !(curFace->Flags & 1) && hasShadow[curFace->Cluster]) || (!isShadow && curFace->Cluster == job->MtrlId))
    {
      curFace->Temp = 1;
      facePlaneMap[i] = ++faceCount;
      triCount += curFace->Count - 2;

      for(sInt j=0;j<curFace->Count;j++)
      {
        sInt v = curFace->Vertices[j];
        sInt adj = curFace->Adjacent[j];

        if(vertMap[v] == -1)
          vertMap[v] = vertCount++;

        if(i < (adj >> 3))
        //if((adj >> 3) < i)
          edgeCount++;
      }
    }
  }

  // build the buffers
  job->Alloc(vertCount,triCount * 3,isShadow ? edgeCount : 0,
    isShadow ? triCount : 0,1);

  // prepare face planes if necessary
#if SHADOWS
  if(isShadow)
  {
    job->PlaneCount = faceCount+1;
    job->Planes = new sVector[faceCount+1];
    job->Planes[0].Init(0,0,0,0);

    for(sInt i=0;i<mesh->Faces.Count;i++)
    {
      sInt planeInd = facePlaneMap[i];

      if(planeInd)
      {
        GenMinFace *face = &mesh->Faces[i];
        GenMinVector *pt = &mesh->Vertices[face->Vertices[0]].Pos;
        sVector *plane = &job->Planes[planeInd];

        plane->x = face->Normal.x;
        plane->y = face->Normal.y;
        plane->z = face->Normal.z;
        plane->w = -face->Normal.Dot(*pt);
      }
    }
  }
#endif

  // generate vertex buffer
  for(sInt i=0;i<mesh->Vertices.Count;i++)
  {
    sInt map = vertMap[i];
    if(map != -1)
      job->VertexBuffer[map] = i;
  }

  // generate index+edge buffers
  faceCount = 0;
  triCount = 0;
  edgeCount = 0;
  vertCount = 0;

  for(sInt faceInd=0;faceInd<mesh->Faces.Count;faceInd++)
  {
    GenMinFace *curFace = &mesh->Faces[faceInd];
    if(!curFace->Temp)
      continue;

    sInt v0,v1,v2 = -1;

    for(sInt i=0;i<curFace->Count;i++)
    {
      // vertex+face
      v1 = v2;
      v2 = vertMap[curFace->Vertices[i]];

      if(i == 0)
        v0 = v2;
      else if(i >= 2)
      {
        job->IndexBuffer[triCount*3+0] = v0;
        job->IndexBuffer[triCount*3+1] = v1;
        job->IndexBuffer[triCount*3+2] = v2;
        if(isShadow)
        {
          job->IndexBuffer[triCount*3+0] *= 2;
          job->IndexBuffer[triCount*3+1] *= 2;
          job->IndexBuffer[triCount*3+2] *= 2;
          job->FaceMap[triCount] = faceCount + 1;
        }

        triCount++;
      }

#if SHADOWS
      // edge
      sInt adjFace = curFace->Adjacent[i] >> 3;

      if(isShadow && faceInd < adjFace)
      {
        sInt ni = i == curFace->Count-1 ? 0 : i+1;

        SilEdge *edge = &job->Edges[edgeCount++];
        edge->Vert[0] = v2 * 2;
        edge->Vert[1] = vertMap[curFace->Vertices[ni]] * 2;
        edge->Face[0] = facePlaneMap[faceInd];
        edge->Face[1] = adjFace != -1 ? facePlaneMap[adjFace] : 0;
      }
#endif
    }

    faceCount++;
  }

  delete[] facePlaneMap;
  delete[] vertMap;

  if(isShadow)
    firstShadowJob = sFALSE;

#if !sINTRO
  if(program == MPP_STATIC || program == MPP_INSTANCES || program == MPP_INSTANCES_SH)
  {
    job->OptimizeIndices();
    job->ReIndexVertices();
  }
#endif
}

void EngMesh::PrepareSpriteJob(Job *job,GenMinMesh *mesh)
{
  sInt *merge = mesh->CalcMergeVerts();
  sInt vertCount = 0;

  for(sInt i=0;i<mesh->Vertices.Count;i++)
    if(merge[i] == i)
      vertCount++;

  job->Alloc(vertCount,0,0,0,0);
  
  vertCount = 0;
  for(sInt i=0;i<mesh->Vertices.Count;i++)
    if(merge[i] == i)
      job->VertexBuffer[vertCount++] = i;
}

void EngMesh::PrepareThickLinesJob(Job *job,GenMinMesh *mesh)
{
  sInt edgeCount = 0;

  // count interesting edges
  for(sInt faceInd=0;faceInd<mesh->Faces.Count;faceInd++)
  {
    GenMinFace *curFace = &mesh->Faces[faceInd];

    for(sInt j=0;j<curFace->Count;j++)
    {
      sInt adjFace = curFace->Adjacent[j] >> 3;

      if(adjFace < faceInd)
      {
        if(curFace->Cluster == job->MtrlId ||
          adjFace != -1 && mesh->Faces[adjFace].Cluster == job->MtrlId)
          edgeCount++;
      }
    }
  }

  // build vertex buffer
  job->Alloc(edgeCount*2,0,0,0,0);

  edgeCount = 0;
  for(sInt faceInd=0;faceInd<mesh->Faces.Count;faceInd++)
  {
    GenMinFace *curFace = &mesh->Faces[faceInd];
    sInt count = curFace->Count;

    for(sInt j=0;j<count;j++)
    {
      sInt adjFace = curFace->Adjacent[j] >> 3;

      if(adjFace < faceInd)
      {
        if(curFace->Cluster == job->MtrlId ||
          adjFace != -1 && mesh->Faces[adjFace].Cluster == job->MtrlId)
        {
          job->VertexBuffer[edgeCount*2+0] = curFace->Vertices[j];
          job->VertexBuffer[edgeCount*2+1] = curFace->Vertices[(j+1) % count];
          edgeCount++;
        }
      }
    }
  }
}

#endif

/****************************************************************************/

#if sLINK_MINMESH

void EngMesh::AddWireEdgeJob(GenMinMesh *mesh,sInt mtrl,sU32 mask)
{
  // add the job
  sInt jobId = AddJob(mtrl,MPP_WIRELINES);
  Job *job = &Jobs[jobId];
  Mtrl[mtrl].JobIds[0] = jobId;
  CalcBoundingBox(mesh,-1,job->BBox);

  // count edges
  sInt edgeCount = 0;
  for(sInt faceInd=0;faceInd < mesh->Faces.Count;faceInd++)
  {
    GenMinFace *curFace = &mesh->Faces[faceInd];
    if(!curFace->Cluster || curFace->Count <= 2)
      continue;

    for(sInt i=0;i<curFace->Count;i++)
      if((curFace->Adjacent[i] >> 3) < faceInd)
        edgeCount++;
  }

  // fill the vertex buffer
  job->Alloc(edgeCount*2,0,0,0,0);

  edgeCount = 0;
  for(sInt faceInd=0;faceInd < mesh->Faces.Count;faceInd++)
  {
    GenMinFace *curFace = &mesh->Faces[faceInd];
    if(!curFace->Cluster || curFace->Count <= 2)
      continue;

    sInt v0, v1 = curFace->Vertices[curFace->Count-1];
    sInt f0, f1 = curFace->Adjacent[curFace->Count-1] >> 3;
    for(sInt i=0;i<curFace->Count;i++)
    {
      v0 = v1;
      v1 = curFace->Vertices[i];
      f0 = f1;
      f1 = curFace->Adjacent[i] >> 3;

      // only add each edge in one direction
      if(f0 < faceInd)
      {
        job->VertexBuffer[edgeCount*2+0] = v0;
        job->VertexBuffer[edgeCount*2+1] = v1;
        edgeCount++;
      }
    }
  }

  sVERIFY(edgeCount * 2 == job->VertexCount);

  // the "selected" job is unused for this mesh type
  Mtrl[mtrl].JobIds[1] = AddJob(mtrl,MPP_WIRELINES);
}

void EngMesh::AddWireFaceJob(GenMinMesh *mesh,sInt mtrl,sU32 mask,sU32 compare)
{
  // add job
  sInt jobId = AddJob(mtrl,MPP_STATIC);
  Job *job = &Jobs[jobId];
  Mtrl[mtrl].JobIds[0] = jobId;
  CalcBoundingBox(mesh,-1,job->BBox);

  // count vertices and triangles
  sInt vertCount=0,triCount=0;
  sInt *vertRemap = new sInt[mesh->Vertices.Count];
  memset(vertRemap,0xff,mesh->Vertices.Count*sizeof(sInt));

  for(sInt faceInd=0;faceInd<mesh->Faces.Count;faceInd++)
  {
    GenMinFace *curFace = &mesh->Faces[faceInd];

    if(curFace->Cluster && curFace->Count >= 2 && ((curFace->Select & mask) != compare))
    {
      triCount += curFace->Count - 2;

      for(sInt i=0;i<curFace->Count;i++)
      {
        sInt v = curFace->Vertices[i];
        if(vertRemap[v] == -1)
          vertRemap[v] = vertCount++;
      }
    }
  }

  // create job and vertexbuffer
  job->Alloc(vertCount,triCount * 3,0,0,0);

  for(sInt i=0;i<mesh->Vertices.Count;i++)
    if(vertRemap[i] != -1)
      job->VertexBuffer[vertRemap[i]] = i;

  // create index buffer
  triCount = 0;

  for(sInt faceInd=0;faceInd<mesh->Faces.Count;faceInd++)
  {
    GenMinFace *curFace = &mesh->Faces[faceInd];

    if(curFace->Cluster && curFace->Count >= 2 && ((curFace->Select & mask) != compare))
    {
      sInt v0 = vertRemap[curFace->Vertices[0]];
      sInt v2 = vertRemap[curFace->Vertices[1]];
      sInt v1;

      for(sInt j=2;j<curFace->Count;j++)
      {
        v1 = v2;
        v2 = vertRemap[curFace->Vertices[j]];

        job->IndexBuffer[triCount*3+0] = v0;
        job->IndexBuffer[triCount*3+1] = v1;
        job->IndexBuffer[triCount*3+2] = v2;
        triCount++;
      }
    }
  }

  sVERIFY(triCount * 3 == job->IndexCount);

  delete[] vertRemap;
}

void EngMesh::AddWireVertJob(GenMinMesh *mesh,sInt mtrl,sU32 mask)
{
  // add job
  sInt jobId = AddJob(mtrl,MPP_WIREVERTEX);
  Job *job = &Jobs[jobId];
  Mtrl[mtrl].JobIds[0] = jobId;
  CalcBoundingBox(mesh,-1,job->BBox);

  // count selected vertices
  sInt vertCount = 0;

  for(sInt i=0;i<mesh->Vertices.Count;i++)
    if(mesh->Vertices[i].Select)
      vertCount++;

  // create job
  job->Alloc(vertCount,0,0,0,0);

  vertCount = 0;
  for(sInt i=0;i<mesh->Vertices.Count;i++)
    if(mesh->Vertices[i].Select)
      job->VertexBuffer[vertCount++] = i;
}

#endif

/****************************************************************************/

void EngMesh::UpdateShadowCacheJob(Job *job,sVector *planes,SVCache *svCache,const EngPaintInfo &info)
{
  sZONE(ShadowJob);

  sVERIFY(job->PlaneCount <= sizeof(SFaceIn) / sizeof(SFaceIn[0]));
  sVERIFY(PartCount < sizeof(SPartSkip) / sizeof(SPartSkip[0]));

  // light front/back side determination
  for(sInt i=0;i<job->PlaneCount;i++)
  {
    sVector *vec = &planes[i];
    sF32 d = vec->x * info.LightPos.x + vec->y * info.LightPos.y + vec->z * info.LightPos.z + vec->w;
    SFaceIn[i] = d >= 0;
  }

#if 0 && !sINTRO
  // part culling
  sF32 rr = info.LightRange * info.LightRange;
  sF32 *pbb = PartBBs;

  for(sInt i=0;i<PartCount;i++,pbb+=6)
  {
    sF32 d = 0.0f, dt;

    // sphere/box intersection calculation
    dt = sFAbs(info.LightPos.x - pbb[0]) - pbb[1];
    if(dt > 0.0f)
      d += dt * dt;

    dt = sFAbs(info.LightPos.y - pbb[2]) - pbb[3];
    if(dt > 0.0f)
      d += dt * dt;

    dt = sFAbs(info.LightPos.z - pbb[4]) - pbb[5];
    if(dt > 0.0f)
      d += dt * dt;

    SPartSkip[i] = d > rr;
  }
#endif

  sInt ec = job->EdgeCount;
  SilEdge *edges = job->Edges;

  // get index pointer
  if(!svCache->IndexBuffer)
    svCache->IndexBuffer = new sInt[ec*6 + job->IndexCount];

  sInt *ip = svCache->IndexBuffer;

  // find silhouette edges
  sInt silInds = 0;
  for(sInt i=0;i<ec;i++)
  {
    SilEdge *edge = &edges[i];

    if(SFaceIn[edge->Face[0]] != SFaceIn[edge->Face[1]]) // found one
    {
      sInt k = SFaceIn[edge->Face[0]];
      sInt i0 = edge->Vert[k^1];
      sInt i1 = edge->Vert[k];

      ip[0] = i0 + 0;
      ip[1] = i1 + 0;
      ip[2] = i1 + 1;
      ip[3] = i0 + 0;
      ip[4] = i1 + 1;
      ip[5] = i0 + 1;
      ip += 6;
      silInds += 6;
    }
  }

  // process caps
  sInt capInds = job->IndexCount;
  sInt *inds = job->IndexBuffer;
  sInt *indEnd = inds + capInds;
  sInt *face = job->FaceMap;

  while(inds < indEnd)
  {
    sInt j = SFaceIn[*face++];
    ip[0] = inds[0] + j;
    ip[1] = inds[1] + j;
    ip[2] = inds[2] + j;
    ip += 3;
    inds += 3;
  }
 
  // write back
  svCache->SilIndices = silInds;
  svCache->CapIndices = capInds;
}

/****************************************************************************/

sInt EngMesh::WireInit = 0;
GenMaterial *EngMesh::MtrlWire[5];

void EngMesh::PrepareWireMaterials()
{
  if(WireInit++ == 0)
  {
    sMaterial *mi;

    // Wireframe edges
    MtrlWire[0] = new GenMaterial;

    // unselected edge
    mi = new sSimpleMaterial(sINVALID,sMBF_ZON,1,0x404040);
    MtrlWire[0]->AddPass(mi,ENGU_OTHER,MPP_WIRELINES,0,1.0f / 512.0f);

    // selected edge
    mi = new sSimpleMaterial(sINVALID,sMBF_ZON,1,0xffffff);
    MtrlWire[0]->AddPass(mi,ENGU_OTHER,MPP_WIRELINES,0,1.0f / 512.0f);

    // Selected faces
    MtrlWire[1] = new GenMaterial;
    mi = new sSimpleMaterial(sINVALID,sMBF_ZON|sMBF_DOUBLESIDED,1,0x706050);
    MtrlWire[1]->AddPass(mi,ENGU_OTHER,MPP_STATIC);

    // Hidden-line faces
    MtrlWire[2] = new GenMaterial;
    mi = new sSimpleMaterial(sINVALID,sMBF_ZREAD|sMBF_DOUBLESIDED|sMBF_BLENDALPHA|sMBF_ZBIASBACK,1,0x60101010);
    MtrlWire[2]->AddPass(mi,ENGU_OTHER,MPP_STATIC);

    // Collision
    MtrlWire[3] = new GenMaterial;

    // add volume
    mi = new sSimpleMaterial(sINVALID,sMBF_ZOFF|sMBF_DOUBLESIDED|sMBF_BLENDADD,1,0x00001800);
    MtrlWire[3]->AddPass(mi,ENGU_OTHER,MPP_STATIC);

    // sub volume
    mi = new sSimpleMaterial(sINVALID,sMBF_ZOFF|sMBF_DOUBLESIDED|sMBF_BLENDADD,1,0x00180000);
    MtrlWire[3]->AddPass(mi,ENGU_OTHER,MPP_STATIC);

    // zone volume
    mi = new sSimpleMaterial(sINVALID,sMBF_ZOFF|sMBF_DOUBLESIDED|sMBF_BLENDADD,1,0x00000018);
    MtrlWire[3]->AddPass(mi,ENGU_OTHER,MPP_STATIC);

    // Vertices
    MtrlWire[4] = new GenMaterial;

    mi = new sSimpleMaterial(sINVALID,sMBF_ZON|sMBF_ZBIASFORE,1,0x00707070);
    MtrlWire[4]->AddPass(mi,ENGU_OTHER,MPP_WIREVERTEX,0,0.01f,1.0f / 512.0f);
  }
}

void EngMesh::ReleaseWireMaterials()
{
  if(--WireInit == 0)
  {
    for(sInt i=0;i<5;i++)
      MtrlWire[i]->Release();
  }
}

/****************************************************************************/

struct Engine_::MeshJob
{
  MeshJob *Next;                  // linked list
  EngMesh *Mesh;                  // mesh to use
  sF32 Time;                      // for bone animation
  sInt PassAdjust;
  sMatrix *Matrix;                // world matrix
};

struct Engine_::EffectJob
{
  EffectJob *Next;                // linked list
  KOp *Op;                        // effect op
  sInt PassAdjust;
  sMatrix Matrix;

  sInt VarStart;                  // Index of first animation parameter
  sInt VarCount;                  // # of animation parameters
  sVector *Animation;             // Animation parameters
};

struct Engine_::PaintJob // =32 bytes (this should be small!)
{
  sU32 SortKey;                   // sorting key

  sInt JobId;                     // mesh job id (-1 for effects)
  EngLight *Light;                // light to use (0 for effects)

  union
  {
    struct
    {
      EngMesh *Mesh;              // mesh to paint
      sMatrix *Matrix;            // matrix to use
      GenMaterialPass *Pass;      // material pass to paint
      sInt Flags;                 // paint flags (varying uses)
      void *BoneData;             // transformed vertices w/ bones
    };
    struct
    {
      EffectJob *EffJob;          // effect job to use (everything in there)
    };
  };
};

struct Engine_::PortalJob
{
  PortalJob *Next;                // linked list
  GenScene *Sectors[3];           // "left", "right", inbetween
  sInt Cost;                      // 0<=Cost<=256
  sVector PCube[8];               // "portalcube"
};

/****************************************************************************/

Engine_::Engine_()
{
#if sPROFILE
  sREGZONE(PortalVis);
  sREGZONE(PortalJob);
  sREGZONE(ShadowJob);
  sREGZONE(UpdatePlanes);
  sREGZONE(BuildJobs);
  sREGZONE(SortJobs);
  sREGZONE(RenderJobs);
  sREGZONE(InstancePrg);
  sREGZONE(EvalBones);
  sREGZONE(VertCopy);
  sREGZONE(VCopyStencil);
#endif

  LightJobs = new EngLight[MAXLIGHT];
  Mem.Init(MAXENGMEM);
  BigMem.Init(MAXENGMEM);

  // Allocate "current render" target
#if !sINTRO
  RenderTargetManager->Add(0x00000000,0,0);
#endif

  UsageMask = ~0U;

  sMaterialEnv env;
  env.Init();
  SetViewProject(env);

  PaintJobs.Init();
  PaintJobs2.Init();

  StartFrame();

#if !sINTRO
  GeoLine = sSystem->GeoAdd(sFVF_COMPACT,sGEO_LINE);
  GeoTri = sSystem->GeoAdd(sFVF_COMPACT,sGEO_TRI);
  MtrlLines = new sSimpleMaterial(sINVALID,sMBF_ZON|sMBF_DOUBLESIDED/*|sMBF_BLENDADD*/,0,0);
#endif
}

Engine_::~Engine_()
{
  PaintJobs.Exit();
  PaintJobs2.Exit();

#if !sINTRO
  sRelease(MtrlLines);
  sSystem->GeoRem(GeoLine);
  sSystem->GeoRem(GeoTri);
#endif

  delete[] LightJobs;
  Mem.Exit();
  BigMem.Exit();
}

void Engine_::SetViewProject(const sMaterialEnv &env)
{
  sFRect rect;

  Env = env;

  View = env.CameraSpace;
  View.TransR();
  env.MakeProjectionMatrix(Project);
  ViewProject.Mul4(View,Project);

  rect.Init(-1,-1,1,1);
  Frustum.FromViewProject(ViewProject,rect);
  Frustum.Normalize();
}

void Engine_::ApplyViewProject()
{
  sSystem->SetViewProject(&Env);
}

void Engine_::StartFrame()
{
  LightJobCount = 0;
  sSetMem(Inserts,0,sizeof(Inserts));
  sSetMem(NeedCurrentRender,0,sizeof(NeedCurrentRender));

  PaintJobs.Count = 0;
  PaintJobs2.Count = 0;

  MeshJobs = 0;
  EffectJobs = 0;
  PortalJobs = 0;
  SectorJobs = 0;
  AmbientLight = 0;

  WeaponLightSet = sFALSE;
  CurrentSectorPaint = sTRUE;
  Mem.Flush();
  BigMem.Flush();
}

/****************************************************************************/

void Engine_::SetUsageMask(sU32 mask)
{
  UsageMask = mask;
}

/****************************************************************************/

void Engine_::AddPaintJob(EngMesh *mesh,const sMatrix &matrix,sF32 time,sInt passAdjust)
{
  MeshJob *job = Mem.Alloc<MeshJob>();
  job->Next = MeshJobs;
  job->Mesh = mesh;
  job->Time = time;
  job->PassAdjust = passAdjust;
  job->Matrix = AddMatrix(matrix);

  MeshJobs = job;
}

void Engine_::AddPaintJob(KOp *op,const sMatrix &matrix,KEnvironment *kenv,sInt passAdjust)
{
  GenEffect *effect = (GenEffect *) op->Cache;
  sVERIFY(op && effect && effect->ClassId == KC_EFFECT);

  EffectJob *job = Mem.Alloc<EffectJob>();
  job->Next = EffectJobs;
  job->Op = op;
  job->PassAdjust = passAdjust;
  job->Matrix = matrix;
  job->VarStart = op->VarStart;
  job->VarCount = op->VarCount;
  job->Animation = Mem.Alloc<sVector>(job->VarCount);
  sCopyMem(job->Animation,&kenv->Var[job->VarStart],job->VarCount*sizeof(sVector));

  EffectJobs = job;
}

void Engine_::AddLightJob(const EngLight &light)
{
  // Start with a quick sphere-frustum rejection test
  for(sInt i=0;i<Frustum.NPlanes;i++)
  {
    if(Frustum.Planes[i].Distance(light.Position) < -light.Range) // completely out
      return;
  }

  // Lousy light importance heuristic.
  sF32 d = light.Position.Distance(Env.CameraSpace.l);
  sF32 importance = light.Range / sMax(d,0.1f);

  // Light distance fadeout, if wanted
#ifdef KKRIEGER
  sF32 fade = sRange((45.0f - d) / 10.0f,1.0f,0.0f);
#else
  sF32 fade = 1.0f;
#endif

  if(fade > 0.0 && light.Amplify >= 1.0f/255.0f) // light visible
  {
#if !sINTRO
    if(light.Flags & EL_WEAPON)
    {
      sInt oldTime = (WeaponLightSet && WeaponLight.Event) ? WeaponLight.Event->Start : 0;
      sInt newTime = light.Event ? light.Event->Start : 0;

      if(!WeaponLightSet || newTime > oldTime)
      {
        WeaponLight = light;
        WeaponLightSet = sTRUE;
      }
    }
    else
      InsertLightJob(light,importance,fade);
#else
    InsertLightJob(light,importance,fade);
#endif
  }
}

void Engine_::AddAmbientLight(sU32 color)
{
  sInt newR = sRange<sInt>(((AmbientLight >> 16) & 0xff) + ((color >> 16) & 0xff),255,0);
  sInt newG = sRange<sInt>(((AmbientLight >>  8) & 0xff) + ((color >>  8) & 0xff),255,0);
  sInt newB = sRange<sInt>(((AmbientLight >>  0) & 0xff) + ((color >>  0) & 0xff),255,0);

  AmbientLight = (newR << 16) | (newG << 8) | newB;
}

void Engine_::AddPortalJob(GenScene *sectors[],const sAABox &portalBox,const sMatrix &matrix,sInt cost)
{
  // Allocate and link in portaljob
  PortalJob *job = Mem.Alloc<PortalJob>();
  job->Next = PortalJobs;
  PortalJobs = job;

  // Fill in fields
  for(sInt i=0;i<3;i++)
    job->Sectors[i] = sectors[i];

  job->Cost = cost;

  // Calculate portalbox vertices
  for(sInt i=0;i<8;i++)
  {
    sVector v;

    v.x = (i & 1) ? portalBox.Max.x : portalBox.Min.x;
    v.y = (i & 2) ? portalBox.Max.y : portalBox.Min.y;
    v.z = (i & 4) ? portalBox.Max.z : portalBox.Min.z;
    v.w = 1.0f;
    job->PCube[i].Rotate34(matrix,v);
  }
}

void Engine_::AddSectorJob(GenScene *sector,KOp *sectorData,const sMatrix &matrix)
{
  // Insert job
  sector->Next = SectorJobs;
  sector->Sector = sectorData;
  sector->SectorMatrix = matrix;
  sector->SectorPaint = sFALSE;
  sector->SectorVisited = sFALSE;
  sector->PortalBox.Init(1e+15f,1e+15f,-1e+15f,-1e+15f);
  SectorJobs = sector;
}

/****************************************************************************/

void Engine_::ProcessPortals(KEnvironment *kenv,KKriegerCell *observerCell)
{
#if !sINTRO // no portal culling in intros
  sFRect unitBox;
  unitBox.Init(-1,-1,1,1);

  // process sector visibility
  if(!observerCell) // just paint everything
  {
    for(GenScene *job=SectorJobs;job;job=job->Next)
    {
      job->SectorPaint = sTRUE;
      job->PortalBox = unitBox;
    }
  }
  else
  {
    sZONE(PortalVis);

    // process visiblity recursively
    PortalVisR(observerCell->Scene,0,unitBox);

    // also tag all inbetween sectors for visible portals
    for(PortalJob *job=PortalJobs;job;job=job->Next)
    {
      if(job->Sectors[2] && (job->Sectors[0]->SectorPaint || job->Sectors[1]->SectorPaint))
      {
        job->Sectors[2]->SectorPaint = sTRUE;
        job->Sectors[2]->PortalBox = unitBox;
      }
    }
  }

  // exec phase
  sZONE(PortalJob);

  for(GenScene *job=SectorJobs;job;job=job->Next)
  {
    if(job->SectorPaint)
    {
      const sFRect &box = job->PortalBox;

      job->SectorPaint = sFALSE; // paint sectors exactly once
      CurrentSectorPaint = box.x1 > box.x0 && box.y1 > box.y0;

      Frustum.FromViewProject(ViewProject,box);
      Frustum.Normalize();

      kenv->ExecStack.Push(job->SectorMatrix);
      job->Sector->ExecWithNewMem(kenv,&job->Sector->SceneMemLink);
      kenv->ExecStack.Pop();
    }
  }

  // restore frustum and sector paint flags
  CurrentSectorPaint = sTRUE;
  Frustum.FromViewProject(ViewProject,unitBox);
  Frustum.Normalize();
#endif
}

/****************************************************************************/

void Engine_::Paint(KEnvironment *kenv,sBool specular)
{
  // Insert weapon light into list of light jobs, if necessary.
  if(WeaponLightSet)
    InsertLightJob(WeaponLight,LightJobs[0].Importance + 1.0f,1.0f);

#if sPROFILE
  // Official beginning of gpu frame ;)
  sPerfMon->Marker(0);
#endif

  sU32 oldUsageMask = UsageMask;
  
  // remove unwanted passes via usage mask
  if(GenOverlayManager->EnableShadows >= 2)
    UsageMask &= ~(1 << ENGU_LIGHT);
  
  if(GenOverlayManager->EnableShadows >= 1)
    UsageMask &= ~(1 << ENGU_SHADOW);

  // FIRE! (damn, this routine is getting shorter and shorter)
  BuildPaintJobs();
  SortPaintJobs();
  ApplyViewProject();
  RenderPaintJobs(kenv);

  UsageMask = oldUsageMask;
}

void Engine_::PaintSimple(KEnvironment *kenv)
{
  // we only want "other" (i.e. single pass) materials
  sU32 oldUsageMask = UsageMask;
  UsageMask = 1 << ENGU_OTHER;

  BuildPaintJobs();
  SortPaintJobs();
  ApplyViewProject();
  RenderPaintJobs(kenv);

  UsageMask = oldUsageMask;
}

/****************************************************************************/

#if !sINTRO

// Paint bounding boxes of all objects (for debugging)
void Engine_::DebugPaintBoundingBoxes(sU32 color)
{
  DebugPaintStart();

  /*// iterate through list of paintjobs drawing their bboxes
  for(PaintJob *job=PaintJobs;job;job=job->Next)
    DebugPaintAABox(job->BBox,color);*/
}

// Prepare debug painting (set up material)
void Engine_::DebugPaintStart(sBool world)
{
  if(world)
    Env.ModelSpace.Init();

  MtrlLines->Set(Env);
}

// Paint one AABB
void Engine_::DebugPaintAABox(const sAABox &box,sU32 color)
{
  static sU8 cubeEdge[] =
  {
    0,1, 1,5, 5,4, 4,0, 2,3, 3,7, 7,6, 6,2, 0,2, 1,3, 5,7, 4,6
  };
  sVertexCompact *vp;

  sSystem->GeoBegin(GeoLine,24,0,(sF32 **)&vp,0);

  for(sInt i=0;i<24;i++)
  {
    sInt v = cubeEdge[i];
    vp->x = (v & 1) ? box.Max.x : box.Min.x;
    vp->y = (v & 2) ? box.Max.y : box.Min.y;
    vp->z = (v & 4) ? box.Max.z : box.Min.z;
    vp->c0 = color;
    vp++;
  }

  sSystem->GeoEnd(GeoLine);
  sSystem->GeoDraw(GeoLine);
}

// Paint a rect
void Engine_::DebugPaintRect(const sFRect &rect,sU32 color)
{
  static sU8 edgeTable[] = { 0,1, 1,3, 3,2, 2,0 };
  sVertexCompact *vp;
  sMatrix mp;

  mp = Env.CameraSpace;
  //mp.Trans3();
  sF32 near = Env.NearClip * 1.1f;
  sF32 sx = 1.0f / Env.ZoomX;
  sF32 sy = 1.0f / Env.ZoomY;

  sSystem->GeoBegin(GeoLine,8,0,(sF32 **)&vp,0);
  for(sInt i=0;i<8;i++)
  {
    sVector v;

    sInt n = edgeTable[i];
    v = mp.l;
    v.AddScale3(mp.i,near * sx * ((n & 1) ? rect.x1 : rect.x0));
    v.AddScale3(mp.j,near * sy * ((n & 2) ? rect.y1 : rect.y0));
    v.AddScale3(mp.k,near);

    vp->x = v.x;
    vp->y = v.y;
    vp->z = v.z;
    vp->c0 = color;
    vp++;
  }

  sSystem->GeoEnd(GeoLine);
  sSystem->GeoDraw(GeoLine);
}

// Paint one line
void Engine_::DebugPaintLine(sF32 x0,sF32 y0,sF32 z0,sF32 x1,sF32 y1,sF32 z1,sU32 color)
{
  sVertexCompact *vp;

  sSystem->GeoBegin(GeoLine,2,0,(sF32 **)&vp,0);
  vp->x = x0;
  vp->y = y0;
  vp->z = z0;
  vp->c0 = color;
  vp++;

  vp->x = x1;
  vp->y = y1;
  vp->z = z1;
  vp->c0 = color;
  vp++;

  sSystem->GeoEnd(GeoLine);
  sSystem->GeoDraw(GeoLine);
}

// Paint one triangle
void Engine_::DebugPaintTri(const sVector &v0,const sVector &v1,const sVector &v2,sU32 color)
{
  sVertexCompact *vp;

  sSystem->GeoBegin(GeoTri,3,0,(sF32 **)&vp,0);
  vp->x = v0.x;
  vp->y = v0.y;
  vp->z = v0.z;
  vp->c0 = color;
  vp++;

  vp->x = v1.x;
  vp->y = v1.y;
  vp->z = v1.z;
  vp->c0 = color;
  vp++;

  vp->x = v2.x;
  vp->y = v2.y;
  vp->z = v2.z;
  vp->c0 = color;
  vp++;

  sSystem->GeoEnd(GeoTri);
  sSystem->GeoDraw(GeoTri);
}

#endif

/****************************************************************************/

// Derived by hand with some trig. I much prefer this variant over
// Lengyel's method, which is a lot more involved.
static void CalcSphereBounds(const sVector &center,sF32 r,sFRect &rect,const sMatrix &view,const sF32 *zoom)
{
  sVector c;
  sF32 a,ds,ci;
  sF32 sds,cds;

  c.Rotate34(view,center);
  rect.Init(-1,-1,1,1);

  // x/y axis loop
  for(sInt i=0;i<2;i++)
  {
    ci = c[i];
    ds = ci * ci + c.z * c.z;
    a = ds - r * r;

    if(a > 0.0f)
    {
      a = sFSqrt(a);

      // delta
      sds = ci * a - c.z * r; // ds*sin(delta)
      cds = ci * r + c.z * a; // ds*cos(delta)
      if(c.z * ds > -r * sds) // left/top intersection has positive z
        (&rect.x0)[i] = sMax(-1.0f,sds / cds * zoom[i]);

      // gamma
      sds = c.z * r + ci * a; // ds*sin(gamma)
      cds = c.z * a - ci * r; // ds*cos(gamma)
      if(c.z * ds > r * sds) // right/bottom intersection has positive z
        (&rect.x1)[i] = sMin(1.0f,sds / cds * zoom[i]);
    }
  }
}

void Engine_::InsertLightJob(const EngLight &light,sF32 importance,sF32 fade)
{
  // calc bounds
  sFRect rect;
  CalcSphereBounds(light.Position,light.Range,rect,View,&Env.ZoomX);
  if(rect.XSize() <= 0.0f || rect.YSize() <= 0.0f) // if empty, return
    return; // note: previous check should take care of this, but anyway...

  // find insertion point
  sInt i;
  for(i=0;i<LightJobCount;i++)
    if(importance > LightJobs[i].Importance)
      break;

  if(i < MAXLIGHT) // insert
  {
    // move up
    for(sInt j=sMin(LightJobCount,MAXLIGHT-1);j>i;j--)
      LightJobs[j] = LightJobs[j-1];

    // actually store light
    LightJobs[i] = light;
    LightJobs[i].Amplify *= fade;
    LightJobs[i].Importance = importance;

    // calculate frustra for shadow volume culling
    LightJobs[i].LightRect = rect;
    LightJobs[i].SVFrustum.FromViewProject(ViewProject,rect);
    LightJobs[i].SVFrustum.EnlargeToInclude(light.Position);
    LightJobs[i].ZFailCull.ZFailVolume(light.Position,Env.CameraSpace,
      Env.NearClip,&Env.ZoomX,rect);

    // done
    LightJobCount++;
  }
}

sMatrix *Engine_::AddMatrix(const sMatrix &matrix)
{
  sMatrix *mtx = BigMem.Alloc<sMatrix>();
  *mtx = matrix;
  
  return mtx;
}

/****************************************************************************/

static void ProjectAndInclude(const sVector &v,sFRect &box)
{
  sVector proj;

  // project
  proj.Scale4(v,1.0f / v.w);

  // adjust box
  box.x0 = sMin(box.x0,proj.x);
  box.y0 = sMin(box.y0,proj.y);
  box.x1 = sMax(box.x1,proj.x);
  box.y1 = sMax(box.y1,proj.y);
}

void Engine_::PortalVisR(GenScene *start,sInt thresh,const sFRect &box)
{
  static sU8 cubeEdge[] =
  {
    0,1, 1,5, 5,4, 4,0, 2,3, 3,7, 7,6, 6,2, 0,2, 1,3, 5,7, 4,6
  };

  // Tag this sector for painting
  start->SectorVisited++;
  start->SectorPaint = sTRUE;
  start->PortalBox.Or(box);

  // Go through portals to find adjacent ones
  for(PortalJob *job=PortalJobs;job;job=job->Next)
  {
    if(thresh + job->Cost < 256)
    {
      for(sInt i=0;i<2;i++)
      {
        // Found an adjacent portal that hasn't been visited yet?
        if(job->Sectors[i] == start && !job->Sectors[i^1]->SectorVisited)
        {
          sBool needClip = sFALSE;
          sVector tv[8];
          sFRect nbox;

          //nbox.Init( 1e+15f, 1e+15f,-1e+15f,-1e+15f);
          nbox.Init(1,1,-1,-1);

          for(sInt j=0;j<8;j++)
          {
            // transform
            tv[j].Rotate4(ViewProject,job->PCube[j]);

            if(tv[j].z >= 0.0f) // in, adjust box accordingly
              ProjectAndInclude(tv[j],nbox);
            else // at least one out, we need to clip
              needClip = sTRUE;
          }

          if(needClip)
          {
            sBool foundCross = sFALSE;

            for(sInt j=0;j<12;j++)
            {
              sVector *v1 = tv + cubeEdge[j*2 + 0];
              sVector *v2 = tv + cubeEdge[j*2 + 1];

              if(v1->z * v2->z < 0.0f) // crossing
              {
                // calc clipped vert, include box
                sVector v;
                v.Lin4(*v1,*v2,v1->z / (v1->z - v2->z));

                ProjectAndInclude(v,nbox);
                foundCross = sTRUE;
              }
            }

            if(!foundCross) // box completely behind viewer
              nbox.Init(1,1,-1,-1);
          }

          nbox.And(box);
          if(nbox.XSize() <= 0 || nbox.YSize() <= 0)
            nbox.Init(1,1,-1,-1);

          // recurse
          PortalVisR(job->Sectors[i^1],thresh + job->Cost,nbox);
        }
      }
    }
  }

  start->SectorVisited--;
}

/****************************************************************************/

void Engine_::BuildPaintJobs()
{
  sZONE(BuildJobs);

  // process effects (easy)
  for(EffectJob *job=EffectJobs;job;job=job->Next)
  {
    GenEffect *effect = (GenEffect *) job->Op->Cache;
    sInt passIndex = sRange(effect->Pass + job->PassAdjust,ENG_MAXPASS-1,0);

    if(effect->NeedCurrentRender)
      NeedCurrentRender[passIndex] = sTRUE;
    
    PaintJob *pjob = Mem.Alloc<PaintJob>();
    pjob->SortKey = (passIndex << 24) | (effect->Usage << 16);
    pjob->JobId = -1;
    pjob->Light = 0;
    pjob->EffJob = job;

    *PaintJobs.Add() = pjob;
  }

  // process meshes (not so easy)
  for(MeshJob *job=MeshJobs;job;job=job->Next)
  {
    EngMesh *mesh = job->Mesh;
    sMatrix *animMatrix = 0;
    void *boneData = 0;

    // evaluate mesh animation first
    if(mesh->Animation)
    {
      sInt count = mesh->Animation->GetMatrixCount();
      animMatrix = BigMem.Alloc<sMatrix>(count);

      // evaluate animation+bones and apply world transform
      mesh->Animation->EvalAnimation(job->Time,0,animMatrix);
      boneData = mesh->EvalBones(animMatrix,BigMem);
      for(sInt i=0;i<count;i++)
        animMatrix[i].MulA(*job->Matrix);
    }

    // now, go through seperate materials and material passes in turn,
    // adding paint jobs as they come. for everything that's per-light,
    // this is also where jobs are duplicated for each light.
    for(sInt i=1;i<mesh->Mtrl.Count;i++)
    {
      GenMeshMtrl *meshMat = &mesh->Mtrl[i];
      EngMaterialInsert *insert = meshMat->Material->Insert;

      for(sInt j=0;j<meshMat->Material->Passes.Count;j++)
      {
        GenMaterialPass *pass = &meshMat->Material->Passes[j];
        sInt usage = pass->Usage;
        if(!(UsageMask & (1 << usage)))
          continue;

        sInt jobId = meshMat->JobIds[j];
        sInt materialId = sInt(pass) >> 3; // FIXME
        sInt matrixId = mesh->Jobs[jobId].AnimMatrix;
        sMatrix *matrix;

        if(matrixId == -1 || !animMatrix)
          matrix = job->Matrix;
        else
          matrix = animMatrix + matrixId;

        // calculate jobs' bbox in world coordinates (note that even
        // when a mesh is not visible, it may cast shadows that are
        // visible)
        sAABox bbox;
        sInt programId = mesh->Jobs[jobId].Program;

        if(programId==MPP_INSTANCES || programId==MPP_INSTANCES_SH || mesh->Animation)
        {
          bbox.Min.Init(-0x40000000,-0x40000000,-0x40000000);
          bbox.Max.Init( 0x40000000, 0x40000000, 0x40000000);
        }
        else
        {
          bbox.Rotate34(mesh->Jobs[jobId].BBox,*matrix);
        }
        sBool cullVisible = sCullBBox(bbox,Frustum.Planes,5);

        if(cullVisible && usage != ENGU_SHADOW)
          continue;

        sMaterial *mtrl = pass->Mtrl;
        sInt passIndex = (pass->Pass + meshMat->Pass) & (ENG_MAXPASS - 1);
        passIndex = sRange(passIndex + job->PassAdjust,ENG_MAXPASS - 1,0);

        // process material inserts
        if(insert && (!Inserts[passIndex] || insert->GetPriority() > Inserts[passIndex]->GetPriority()))
          Inserts[passIndex] = insert;

        // if the usage is per-light, loop through lights
        if(usage >= ENGU_SHADOW && usage <= ENGU_LIGHT)
        {
          for(sInt lightId=0;lightId<LightJobCount;lightId++)
          {
            EngLight *light = &LightJobs[lightId];

            // some additional per-light culling
            if(!bbox.IntersectsSphere(light->Position,light->Range))
              continue;

            if(usage == ENGU_SHADOW && (!(light->Flags & EL_SHADOW) ||
              sCullBBox(bbox,light->SVFrustum.Planes,5)))
              continue;

            // and now the paintjob creation
            PaintJob *pjob = Mem.Alloc<PaintJob>();

            pjob->SortKey = (passIndex << 24) | (lightId << 20)
              | (usage << 16) | (materialId & 0xffff);
            pjob->JobId = jobId;
            pjob->Mesh = mesh;
            pjob->Matrix = matrix;
            pjob->Pass = pass;
            pjob->Light = light;
            pjob->Flags = sCullBBox(bbox,light->ZFailCull.Planes,light->ZFailCull.NPlanes) ? 0 : sMBF_STENCILZFAIL;
            pjob->BoneData = boneData;

            *PaintJobs.Add() = pjob;
          }
        }
        else // just once
        {
          sInt lightId = (usage < ENGU_SHADOW) ? 0 : MAXLIGHT-1;

          // Create the paintjob
          PaintJob *pjob = Mem.Alloc<PaintJob>();

          pjob->SortKey = (passIndex << 24) | (lightId << 20)
            | (usage << 16) | (materialId & 0xffff);
          pjob->JobId = jobId;
          pjob->Mesh = mesh;
          pjob->Matrix = matrix;
          pjob->Pass = pass;
          pjob->Light = 0;
          pjob->Flags = 0;
          pjob->BoneData = boneData;

          *PaintJobs.Add() = pjob;
        }
      }
    }
  }
}

// Straight radix sort for paint jobs.
void Engine_::SortPaintJobs()
{
  static sInt histogram[4][256]; // FIXME: this REALLY shouldn't be static
  sInt jobCount = PaintJobs.Count;

  sZONE(SortJobs);

  // counting pass
  sSetMem(histogram,0,sizeof(histogram));

  for(sInt i=0;i<jobCount;i++)
  {
    sU32 key = PaintJobs[i]->SortKey;

    histogram[0][(key >>  0) & 0xff]++;
    histogram[1][(key >>  8) & 0xff]++;
    histogram[2][(key >> 16) & 0xff]++;
    histogram[3][(key >> 24) & 0xff]++;
  }

  // summation passes
  for(sInt i=0;i<4;i++)
  {
    sInt current = 0;

    for(sInt j=0;j<256;j++)
    {
      sInt newCurrent = current + histogram[i][j];
      histogram[i][j] = current;
      current = newCurrent;
    }

    sVERIFY(current == jobCount);
  }

  // reordering (sorting) passes
  PaintJobs2.AtLeast(PaintJobs.Alloc);
  PaintJobs2.Count = jobCount;

  for(sInt pass=0;pass<4;pass++)
  {
    sInt shift = pass * 8;
    sInt *histo = histogram[pass];
    PaintJob **pj1 = PaintJobs.Array;
    PaintJob **pj2 = PaintJobs2.Array;

    for(sInt i=0;i<jobCount;i++)
      pj2[histo[(pj1[i]->SortKey >> shift) & 0xff]++] = pj1[i];

    PaintJobs.Swap(PaintJobs2);
  }
}

void Engine_::RenderPaintJobs(KEnvironment *kenv)
{
  sInt curPass = -1,curUsage = -1;
  EngLight *curLight = 0;
  EngPaintInfo paintInfo;

  sZONE(RenderJobs);

  // render all paint jobs
  for(sInt i=0;i<PaintJobs.Count;i++)
  {
    PaintJob *job = PaintJobs[i];
    sInt pass = job->SortKey >> 24;
    sInt usage = (job->SortKey >> 16) & 0xf;
    EngLight *light = job->Light;

    // handle inserts and light changes
    if(pass != curPass || usage != curUsage || light != curLight)
    {
      sSystem->SetScissor(0);
      if(curPass >= 0 && Inserts[curPass])
        Inserts[curPass]->AfterUsage(curPass,curUsage,curLight);

      // grab current render, if required
      if(pass != curPass && NeedCurrentRender[pass])
        RenderTargetManager->GrabToTarget(0x00000000);

      curPass = pass;
      curUsage = usage;
      curLight = light;
      if(Inserts[curPass])
        Inserts[curPass]->BeforeUsage(curPass,curUsage,curLight);
      
      if(light)
      {
        Env.LightType = light->Type;
        Env.LightPos = light->Position;
        Env.LightDir = light->Direction;
        Env.LightColor.InitColor(light->Color);
        Env.LightRange = light->Range;
        Env.LightAmplify = light->Amplify;
        sSystem->SetScissor(&light->LightRect);
      }
    }

    // paint that job
    if(job->JobId != -1) // it's a mesh
    {
      Env.ModelSpace = *job->Matrix;

      // setup paint info (FIXME, i'd like to get rid of this here)
      if(job->Pass->Usage == ENGU_SHADOW)
      {
        sMatrix mat = Env.ModelSpace;
        mat.TransR();

        paintInfo.LightPos.Rotate34(mat,curLight->Position);
        paintInfo.LightRange = curLight->Range;
        paintInfo.LightId = curLight->Id;
        paintInfo.StencilFlags = job->Flags;
      }

      paintInfo.BoneData = job->BoneData;
      Env.Flags = paintInfo.BoneData ? 1 : 0;
      if(job->Pass->Program == MPP_INSTANCES_SH)
        Env.Flags = 2;

      job->Pass->Mtrl->Set(Env);
      job->Mesh->PaintJob(job->JobId,job->Pass,paintInfo);
    }
    else // it's an effect
    {
      EffectJob *ejob = job->EffJob;
      sCopyMem(&kenv->Var[ejob->VarStart],ejob->Animation,ejob->VarCount*sizeof(sVector));

      kenv->ExecStack.Push(ejob->Matrix);
      ejob->Op->ExecWithNewMem(kenv,&ejob->Op->SceneMemLink);
      kenv->ExecStack.Pop();

      ApplyViewProject();
    }
  }

  // finish inserts
  sSystem->SetScissor(0);
  if(curPass >= 0 && Inserts[curPass])
    Inserts[curPass]->AfterUsage(curPass,curUsage,curLight);
}

/****************************************************************************/

Engine_ *Engine = 0;

/****************************************************************************/

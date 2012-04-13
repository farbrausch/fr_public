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

#include "_gui.hpp"

#if sPROFILE
#include "_util.hpp"
sMAKEZONE(PortalVis   ,"PortalVis"  ,0xff00ff00);
sMAKEZONE(PortalJob   ,"PortalJob"  ,0xff70b040);
sMAKEZONE(ShadowJob   ,"ShadowJob"  ,0xff800000);
sMAKEZONE(ShadowBatch ,"ShadowBatch",0xffc00000);
sMAKEZONE(BuildJobs   ,"BuildJobs"  ,0xff5f2297);
sMAKEZONE(SortJobs    ,"SortJobs"   ,0xffe4ab3b);
sMAKEZONE(RenderJobs  ,"RenderJobs" ,0xff7ade53);
sMAKEZONE(EvalBones   ,"EvalBones"  ,0xffef23ff);
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
  sU16 Vert[2];                             // vertex index
  sInt Face[2];                             // face (plane) indices
#if !sINTRO
  sU16 Prev[2];                             // previous link
  sInt Tick;                                // for tagging.
#endif
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

  sInt *FacePartEnd;                        // to cull groups of facaes
  sInt *EdgePartEnd;                        // to cull groups of edges

  sInt AnimType;                            // ENGANIM_*
  sInt AnimMatrix;                          // for "rigid" type
  sAABox BBox;                              // bounding box

  EngMesh::SVCache SVCache[MAXSVCACHE];     // shadow volume caches.

  void Init();
  void Alloc(sInt vc,sInt ic,sInt ec,sInt fmc,sInt pc=0);
  void Exit();

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

void EngMesh::Job::UpdatePlanes(sVector *target,const sVertexTSpace3Big *verts)
{
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

      // normalize normal
      sF32 len = nx*nx + ny*ny + nz*nz;
      if(len)
      {
        len = sFInvSqrt(len);
        nx *= len;
        ny *= len;
        nz *= len;
      }

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
  PartCount = 0;
  PartBBs = 0;
  Preloaded = sFALSE;
  Mtrl.Init();
  Jobs.Init();
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

  delete[] Vert;
  delete[] BoneInfos;
  delete[] PartBBs;
  Mtrl.Exit();
  Jobs.Exit();

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

/****************************************************************************/

// Converts a GenMinMesh to a list of jobs and a vertex buffer.
#if sLINK_MINMESH

void EngMesh::FromGenMinMesh(GenMinMesh *mesh)
{
  mesh->CalcNormals();
  mesh->CalcAdjacency();
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
}
#endif
#endif

/****************************************************************************/

// Do the bone transforms
void *EngMesh::EvalBones(sMatrix *matrices,sGrowableMemStack &alloc)
{
  sZONE(EvalBones);

  if(!BoneInfos)
    return Vert;

  sVertexTSpace3Big *vert = alloc.Alloc<sVertexTSpace3Big>(VertCount);
  sVertexTSpace3Big *out = vert;
  const sVertexTSpace3Big *in = Vert;
  BoneInfo *inInfo = BoneInfos;

  for(int i=0;i<VertCount;i++)
  {
    sF32 len;

    out->x = 0;
    out->y = 0;
    out->z = 0;
    out->nx = 0;
    out->ny = 0;
    out->nz = 0;
    out->sx = 0;
    out->sy = 0;
    out->sz = 0;

    for(int j=0;j<inInfo->BoneCount;j++)
    {
      sF32 w = inInfo->Weight[j];
      const sMatrix *m = matrices + inInfo->Matrix[j];

      out->x += w * (m->i.x * in->x + m->j.x * in->y + m->k.x * in->z + m->l.x);
      out->y += w * (m->i.y * in->x + m->j.y * in->y + m->k.y * in->z + m->l.y);
      out->z += w * (m->i.z * in->x + m->j.z * in->y + m->k.z * in->z + m->l.z);

      out->nx += w * (m->i.x * in->nx + m->j.x * in->ny + m->k.x * in->nz);
      out->ny += w * (m->i.y * in->nx + m->j.y * in->ny + m->k.y * in->nz);
      out->nz += w * (m->i.z * in->nx + m->j.z * in->ny + m->k.z * in->nz);

      out->sx += w * (m->i.x * in->sx + m->j.x * in->sy + m->k.x * in->sz);
      out->sy += w * (m->i.y * in->sx + m->j.y * in->sy + m->k.y * in->sz);
      out->sz += w * (m->i.z * in->sx + m->j.z * in->sy + m->k.z * in->sz);
    }

    // renormalize n
    len = out->nx*out->nx + out->ny*out->ny + out->nz*out->nz;
    if(len)
    {
      len = 1.0f / sFSqrt(len);
      out->nx *= len;
      out->ny *= len;
      out->nz *= len;
    }

    // orthogonalize s against n
    len = out->sx*out->nx + out->sy*out->ny + out->sz*out->nz;
    out->sx -= len * out->nx;
    out->sy -= len * out->ny;
    out->sz -= len * out->nz;

    // normalize s
    len = out->sx*out->sx + out->sy*out->sy + out->sz*out->sz;
    if(len)
    {
      len = 1.0f / sFSqrt(len);
      out->sx *= len;
      out->sy *= len;
      out->sz *= len;
    }

    // copy rest
    out->c = in->c;
    out->u = in->u;
    out->v = in->v;

    out++;
    in++;
    inInfo++;
  }

  return vert;
}

/****************************************************************************/

static sVector SPlaneBuffer[262144];

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

  // choose right source vertices
  sVertexTSpace3Big *vert = Vert;
  if(paintInfo.BoneData)
    vert = (sVertexTSpace3Big *) paintInfo.BoneData;

  sInt handle = job->Geometry;
  sBool bigInd = job->VertexCount >= (program == MPP_SHADOW ? 32768 : 65536);
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

    case MPP_SHADOW:
      if(!BoneInfos)
        handle = sSystem->GeoAdd(sFVF_XYZW,sGEO_TRI|sGEO_STATVB|sGEO_DYNIB|bigFlag);
      else
        handle = sSystem->GeoAdd(sFVF_XYZW,sGEO_TRI|sGEO_DYNVB|sGEO_DYNIB|bigFlag);
      break;

    case MPP_SPRITES:
      handle = sSystem->GeoAdd(sFVF_STANDARD,sGEO_QUAD|sGEO_DYNAMIC);
      break;

#if !sINTRO
    case MPP_THICKLINES:
      handle = sSystem->GeoAdd(sFVF_STANDARD,sGEO_QUAD|sGEO_DYNAMIC);
      break;
#endif

#if !sPLAYER
    case MPP_WIRELINES:
      handle = sSystem->GeoAdd(sFVF_COMPACT,sGEO_LINE|sGEO_STATIC);
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
        sSystem->GeoBegin(handle,vc,ic,(sF32 **)&vp,(void **)&ip);

        // copy vertices
        for(sInt i=0;i<vc;i++)
          vp[i] = vert[vind[i]];

        // copy indices
        if(bigInd)
          sCopyMem(ip,job->IndexBuffer,ic*4);
        else
          for(sInt i=0;i<ic;i++)
            ip[i] = job->IndexBuffer[i];

        // draw
        sSystem->GeoEnd(handle);
        sSystem->GeoDraw(handle);

        if(BoneInfos)
          sSystem->GeoFlush(handle,sGEO_VERTEX);
      }
      break;

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

          for(sInt i=0;i<vc;i++)
          {
            sVertexTSpace3Big *src = &vert[vind[i]];

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
            for(sInt i=0;i<ic;i++)
              ip[i] = svCache->IndexBuffer[i];

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
          sSystem->GeoFlush(handle,BoneInfos ? sGEO_VERTEX|sGEO_INDEX : sGEO_INDEX);

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

#if !sINTRO
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

  if(mayDeleteVert)
  {
    delete[] Vert;
    Vert = 0;
  }

  Preloaded = sTRUE;
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

// Convert a meshes' vertices into a compact vertex list.
void EngMesh::FillVertexBuffer(GenMesh *mesh)
{
  delete[] Vert;
  VertCount = mesh->Vert.Count;
  Vert = new sVertexTSpace3Big[mesh->Vert.Count];
  
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

        PrepareJob(&Jobs[jobId],mesh);
      }

      Mtrl[i].JobIds[j] = jobId;
    }
  }
}

// Prepares a single mesh job
void EngMesh::PrepareJob(Job *job,GenMesh *mesh)
{
  // first compute bounding box
  CalcBoundingBox(mesh,job->MtrlId,job->BBox);

  // check whether we understand the program used
  sInt program = job->Program;
  sBool isShadow = program == MPP_SHADOW;

  switch(program)
  {
  case MPP_SPRITES:
    PrepareSpriteJob(job,mesh);
    return;

  case MPP_THICKLINES:
    PrepareThickLinesJob(job,mesh);
    return;
  }

  // count faces, edges and vertices
  sInt faceCount,triCount,edgeCount,vertCount;
  mesh->All2Sel(0,MAS_EDGE|MAS_FACE|MAS_VERT);

  faceCount = 0;
  triCount = 0;
  for(sInt i=0;i<mesh->Face.Count;i++)
  {
    mesh->Face[i].Temp = faceCount;

    if(mesh->Face[i].Material == job->MtrlId && (!isShadow || mesh->Face[i].Used))
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

      e = mesh->NextFaceEdge(e);
    }
    while(e != ee);

    faceCount++;
  }

  sVERIFY(triCount*3 == job->IndexCount);
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

/****************************************************************************/

#if sLINK_MINMESH

void EngMesh::FillVertexBuffer(GenMinMesh *mesh)
{
  delete[] Vert;
  VertCount = mesh->Vertices.Count;
  Vert = new sVertexTSpace3Big[mesh->Vertices.Count];

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
    BoneInfos = new BoneInfo[mesh->Vertices.Count];
    BoneInfo *outInfo = BoneInfos;
    inVert = &mesh->Vertices[0];

    for(int i=0;i<mesh->Vertices.Count;i++)
    {
      outInfo->BoneCount = inVert->BoneCount;
      for(sInt j=0;j<4;j++)
      {
        outInfo->Weight[j] = inVert->Weights[j];
        outInfo->Matrix[j] = inVert->Matrix[j];
      }

      outInfo++;
      inVert++;
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
  Mtrl.Resize(mesh->Clusters.Count);

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

        PrepareJob(&Jobs[jobId],mesh);
      }

      outMtrl->JobIds[j] = jobId;
    }
  }
}

void EngMesh::PrepareJob(Job *job,GenMinMesh *mesh)
{
  // prepare program, calc bounding box
  CalcBoundingBox(mesh,job->MtrlId,job->BBox);

  sInt program = job->Program;
  sBool isShadow = program == MPP_SHADOW;

  switch(program)
  {
  case MPP_STATIC:
  case MPP_SHADOW:
    break;

  case MPP_SPRITES:
    PrepareSpriteJob(job,mesh);
    return;

  case MPP_THICKLINES:
    PrepareThickLinesJob(job,mesh);
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

    if(curFace->Cluster == job->MtrlId)
    {
      facePlaneMap[i] = ++faceCount;
      triCount += curFace->Count - 2;

      for(sInt j=0;j<curFace->Count;j++)
      {
        sInt v = curFace->Vertices[j];
        sInt adj = curFace->Adjacent[j];

        if(vertMap[v] == -1)
          vertMap[v] = vertCount++;

        if((adj >> 3) < i)
          edgeCount++;
      }
    }
  }

  // build the buffers
  job->Alloc(vertCount,triCount * 3,isShadow ? edgeCount : 0,
    isShadow ? triCount : 0,1);

  // prepare face planes if necessary
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
    if(curFace->Cluster != job->MtrlId)
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

      // edge
      sInt adjFace = curFace->Adjacent[i] >> 3;

      if(isShadow && adjFace < faceInd)
      {
        sInt ni = i == curFace->Count-1 ? 0 : i+1;

        SilEdge *edge = &job->Edges[edgeCount++];
        edge->Vert[0] = v2 * 2;
        edge->Vert[1] = vertMap[curFace->Vertices[ni]] * 2;
        edge->Face[0] = facePlaneMap[faceInd];
        edge->Face[1] = adjFace != -1 ? facePlaneMap[adjFace] : 0;
      }
    }

    faceCount++;
  }

  delete[] facePlaneMap;
  delete[] vertMap;
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
  sInt MatrixId;                  // world matrix id
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
      sInt MatrixId;              // matrix to use
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
  sREGZONE(ShadowBatch);
  sREGZONE(BuildJobs);
  sREGZONE(SortJobs);
  sREGZONE(RenderJobs);
  sREGZONE(EvalBones);
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

  Matrices.Init();
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
  Matrices.Exit();
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

  Matrices.Count = 0;
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
  job->MatrixId = AddMatrix(matrix);

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

sInt Engine_::AddMatrix(const sMatrix &matrix)
{
  sInt index = Matrices.Count;
  *Matrices.Add() = matrix;
  
  return index;
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
    sInt animBase = -1;
    void *boneData = 0;

    // evaluate mesh animation first
    if(mesh->Animation)
    {
      sInt count = mesh->Animation->GetMatrixCount();
      
      Matrices.AtLeast(Matrices.Count + count);
      animBase = Matrices.Count;
      Matrices.Count += count;

      // evaluate animation+bones and apply world transform
      mesh->Animation->EvalAnimation(job->Time,0,&Matrices[animBase]);
      boneData = mesh->EvalBones(&Matrices[animBase],BigMem);
      for(sInt i=0;i<count;i++)
        Matrices[animBase+i].MulA(Matrices[job->MatrixId]);
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

        if(matrixId == -1 || animBase == -1)
          matrixId = job->MatrixId;
        else
          matrixId += animBase;

        // calculate jobs' bbox in world coordinates (note that even
        // when a mesh is not visible, it may cast shadows that are
        // visible)
        sAABox bbox;
        bbox.Rotate34(mesh->Jobs[jobId].BBox,Matrices[matrixId]);
        sBool cullVisible = !mesh->Animation && sCullBBox(bbox,Frustum.Planes,5);

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
            pjob->MatrixId = matrixId;
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
          pjob->MatrixId = matrixId;
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
        Env.LightPos = light->Position;
        Env.LightColor.InitColor(light->Color);
        Env.LightRange = light->Range;
        Env.LightAmplify = light->Amplify;
        sSystem->SetScissor(&light->LightRect);
      }
    }

    // paint that job
    if(job->JobId != -1) // it's a mesh
    {
      Env.ModelSpace = Matrices[job->MatrixId];

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

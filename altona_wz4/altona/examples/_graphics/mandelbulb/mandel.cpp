/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "mandel.hpp"
#include "mandelshader.hpp"
#include "util/taskscheduler.hpp"
#include "util/image.hpp"
#include "extra/blobheap.hpp"

OctManager *OctMan;

const sInt n = 32;                // grid size in units
const sInt n1 = n+1;              // vertices, including left and right edge
const sInt no = 1;                // offset into the cube for normals
const sInt m = n+3;               // need some more to calculate normals
const sInt mm = m*m;
const sInt mmm = m*m*m;

#define TEXTUREHEIGHT 1024        // render multiple cubes in one texture
#define SLICEHEIGHT 42            // m*m*m / (texture width *4) ; rounded up ; *4 for argb

/****************************************************************************/
/****************************************************************************/

OctManager::OctManager()
{
  OctMan = this;
  VerticesDrawn = 0;
  NodesDrawn = 0;
  VerticesLife = 0;
  DeepestDrawn = 0;
  EndGame = 0;

  static const sU32 desc[] = 
  {
    sVF_POSITION|sVF_F3,
    sVF_UV0|sVF_F4,
    sVF_UV1|sVF_F2,
    0,
  };
  static const sU32 desc2[] =
  {
    sVF_POSITION|sVF_F3,
    sVF_NORMAL|sVF_I4,
    0,
  };

  sImage img;
  img.Init(256,64);
  img.Fill(0x00000000);
  for(sInt i=0;i<mmm;i++)
    ((sU8 *)(img.Data))[i] = i%m;
  CSTex[0] = sLoadTexture2D(&img,sTEX_ARGB8888|sTEX_2D);
  for(sInt i=0;i<mmm;i++)
    ((sU8 *)(img.Data))[i] = i/m%m;
  CSTex[1] = sLoadTexture2D(&img,sTEX_ARGB8888|sTEX_2D);
  for(sInt i=0;i<mmm;i++)
    ((sU8 *)(img.Data))[i] = i/(m*m)%m;
  CSTex[2] = sLoadTexture2D(&img,sTEX_ARGB8888|sTEX_2D);

  CSFormat = sCreateVertexFormat(desc);
  CSGeo = new sGeometry(sGF_QUADLIST,CSFormat);
  CSMtrl = new MandelMath;
  CSMtrl->Flags = sMTRL_ZOFF|sMTRL_CULLOFF;
  for(sInt i=0;i<3;i++)
  {
    CSMtrl->Texture[i] = CSTex[i];
    CSMtrl->TFlags[i] = sMTF_LEVEL0|sMTF_CLAMP;
  }
  CSMtrl->Prepare(CSFormat);
  CSView.TargetSizeX = 256;
  CSView.TargetSizeY = TEXTUREHEIGHT;
  CSView.TargetAspect = sF32(CSView.TargetSizeX)/CSView.TargetSizeY;
  CSView.Target.Init(0,0,CSView.TargetSizeX,CSView.TargetSizeY);
  CSView.Orthogonal = sVO_PIXELS;
  CSView.ZoomX = 1;
  CSView.ZoomY = 1;
  CSView.Prepare();

  MeshFormat = sCreateVertexFormat(desc2);

  CurrentTarget = 0;

  for(sInt i=0;i<24;i++)
    Geos.AddTail(new GeoBuffer);
  GeoRoundRobin = 0;

  MemBundlesTotal = 384;
  NodesTotal = 8192;
  for(sInt i=0;i<32;i++)
    FreeTargets.AddTail(new Target);
  for(sInt i=0;i<MemBundlesTotal;i++)
    FreeMemBundles.AddTail(new OctMemBundle);
  for(sInt i=0;i<NodesTotal;i++)
    FreeNodes.AddTail(new OctNode);

#if BLOBHEAP
  const sPtr MB = 1024*1024;
#if sCONFIG_64BIT
  BlobHeap = new sBlobHeap(4096UL*MB,128*MB,4096);    // 64 bit
#else
  BlobHeap = new sBlobHeap(1500*MB,128*MB,4096);    // 32 bit
#endif
#endif
}

OctManager::~OctManager()
{
  sDeleteAll(DummyGeos);
  sDeleteAll(FreeTargets);
  sDeleteAll(ReadyTargets);
  sDeleteAll(BusyTargets);
  sDeleteAll(DelayTargets);
  sDeleteAll(DoneTargets);

  sDeleteAll(Geos);

  sDeleteAll(FreeMemBundles);
  sDeleteAll(FreeNodes);

  delete CSGeo;
  delete CSMtrl;
  delete CSTex[0];
  delete CSTex[1];
  delete CSTex[2];

#if BLOBHEAP
  delete BlobHeap;
#endif
}

void OctManager::NewFrame()
{
  VerticesDrawn = 0;
  DeepestDrawn = 0;
  NodesDrawn = 0;
  sVERIFY(CurrentTarget==0);
}

/****************************************************************************/

OctManager::Target::Target()
{
  tex = new sTexture2D(256,TEXTUREHEIGHT,sTEX_2D|sTEX_ARGB16F|sTEX_RENDERTARGET,1);
  trans = new sGpuToCpu(sTEX_2D|sTEX_ARGB16F,256,TEXTUREHEIGHT);
  RenderYPos = 0;
}
OctManager::Target::~Target()
{
  delete tex;
  delete trans;
}

OctMemBundle::OctMemBundle()
{
  pot = new sF32[mmm];
  icache = new sInt[n1*n1*n1*3];
  VertexAlloc = 0x6000;
  IndexAlloc = VertexAlloc*6;
  VB = new OctManager::MeshVertex[VertexAlloc];
  IB = new sU16[IndexAlloc];
}

OctMemBundle::~OctMemBundle()
{
  delete[] pot;
  delete[] icache;
  delete[] VB;
  delete[] IB;
}

OctMemBundle *OctManager::AllocMemBundle()
{
  if(FreeMemBundles.IsEmpty())
  {
    MemBundlesTotal++;
    return new OctMemBundle;
  }
  else
  {
    return FreeMemBundles.RemTail();
  }
}

void OctManager::FreeMemBundle(OctMemBundle *m)
{
  FreeMemBundles.AddTail(m);
}


OctNode *OctManager::AllocNode()
{
  if(FreeNodes.IsEmpty())
  {
    NodesTotal++;
    return new OctNode;
  }
  else
  {
    return FreeNodes.RemTail();
  }
}

void OctManager::FreeNode(OctNode *m)
{
  if(m)
  {
    m->Free();
    FreeNodes.AddTail(m);
  }
}

/****************************************************************************/

void OctManager::ReadbackRender()
{
  OctRenderJob *job;
  Target *t;

  sFORALL(DoneTargets,t)
  {
    sDInt pitch;
    const sHalfFloat *f16 = (const sHalfFloat *) t->trans->BeginRead(pitch);

    sVERIFY(pitch==256*4*sizeof(sHalfFloat));

    sFORALL(t->Jobs,job)
    {
      sCopyMem(job->Dest,f16+1024*job->YPos,sizeof(sHalfFloat)*mmm); // just copy data, do conversion multithreaded!
    }

    t->trans->EndRead();
  }
  FreeTargets.Add(DoneTargets);
  DoneTargets.Clear();
}

void OctManager::AddRender(const sVector4 &Info,sF32 *Dest)
{
  if(CurrentTarget && CurrentTarget->RenderYPos+SLICEHEIGHT>TEXTUREHEIGHT)
  {
    ReadyTargets.AddTail(CurrentTarget);
    CurrentTarget = 0;
  }
  if(CurrentTarget==0)
  {
    if(FreeTargets.IsEmpty())
      CurrentTarget = new Target;
    else
      CurrentTarget = FreeTargets.RemTail();
    CurrentTarget->RenderYPos = 0;
    CurrentTarget->Jobs.Clear();
  }
  OctRenderJob *job = CurrentTarget->Jobs.AddMany(1);
  job->Dest = Dest;
  job->Info = Info;
  job->YPos = CurrentTarget->RenderYPos;
  CurrentTarget->RenderYPos += SLICEHEIGHT;
}

void OctManager::StartRender()
{
  OctRenderJob *job;
  Target *t;
  CSVertex *vp;
  sGraphicsCaps caps;
  sGetGraphicsCaps(caps);
  sF32 fx = caps.XYOffset;
  sF32 u = 1;
  sF32 v = SLICEHEIGHT/64.0f;

  sCBuffer<MandelMathPara> cb;
  cb.Data->mvp = OctMan->CSView.ModelScreen;
  cb.Modify();

  if(CurrentTarget)
  {
    ReadyTargets.AddTail(CurrentTarget);
    CurrentTarget = 0;
  }

  sFORALL(ReadyTargets,t)
  {
    sSetTarget(sTargetPara(0,0,0,t->tex,0));
    OctMan->CSMtrl->Set(&cb);

    OctMan->CSGeo->BeginLoadVB(4*t->Jobs.GetCount(),sGD_STREAM,&vp);
    sFORALL(t->Jobs,job)
    {
      vp[0].Init(  0+fx,          0+fx+job->YPos,job->YPos,job->Info,0,0);
      vp[1].Init(256+fx,          0+fx+job->YPos,job->YPos,job->Info,u,0);
      vp[2].Init(256+fx,SLICEHEIGHT+fx+job->YPos,job->YPos,job->Info,u,v);
      vp[3].Init(  0+fx,SLICEHEIGHT+fx+job->YPos,job->YPos,job->Info,0,v);
      vp+=4;
    }
    OctMan->CSGeo->EndLoadVB();

    OctMan->CSGeo->Draw();

    t->trans->CopyFrom(t->tex);
  }

  DoneTargets = DelayTargets;
  DelayTargets = BusyTargets;
  BusyTargets = ReadyTargets;
  ReadyTargets.Clear();
}


/****************************************************************************/

OctGeoHandle::OctGeoHandle()
{
  sClear(*this);
  Geometry = -1;
}

OctGeoHandle::~OctGeoHandle()
{
  Clear();
}

void OctGeoHandle::Clear()
{
#if BLOBHEAP
  OctMan->BlobHeap->Free(hnd);
  hnd = 0;
#else
  delete[] Data;
  Data = 0;
#endif
}

void OctGeoHandle::Alloc(sInt vc,sInt ic)
{
  VertexCount = vc;
  IndexCount = ic;
  sInt vbytes = sizeof(OctManager::MeshVertex)*vc;
  sInt ibytes = sizeof(sU16)*ic;
#if BLOBHEAP
  hnd = OctMan->BlobHeap->Alloc(vbytes+ibytes);
#else
  Data = (sU8 *)sAllocMem(vbytes+ibytes,16,0);
  VB = (OctManager::MeshVertex *) (Data);
  IB = (sU16 *)(Data+vbytes);
#endif
}

OctManager::GeoBuffer::GeoBuffer()
{
  Geo = 0;
  Drawn = 0;
  VertexAlloc = 1024*1024;
  IndexAlloc = VertexAlloc*7;
  IndexUsed = 0;
  VertexUsed = 0;

  Geo = new sGeometry(sGF_TRILIST|sGF_INDEX32,OctMan->MeshFormat);
  Geo->InitDyn(IndexAlloc,VertexAlloc);

  /*
  for(sInt i=0;i<3;i++)
  {
    Geo->BeginDynIB(1);
    Geo->BeginDynVB(1);
    Geo->EndDynIB();
    Geo->EndDynVB();
  }
  */


  Handles.HintSize(1024);
}

OctManager::GeoBuffer::~GeoBuffer()
{
  delete Geo;
}

void OctManager::AllocHandle(OctGeoHandle &hnd)
{
  GeoHandles.AddTail(&hnd);
}

void OctManager::FreeHandle(OctGeoHandle &hnd)
{
  GeoHandles.Rem(&hnd);
  if(hnd.Geometry>=0)
    Geos[hnd.Geometry]->Handles.RemOrder(&hnd);
  hnd.Clear();
  hnd.Geometry = -1;
  hnd.Valid = 0;
}

void OctManager::Draw()
{
  OctGeoHandle *gh;
  GeoBuffer *gb;

  sFORALL(Geos,gb)
    gb->Drawn = 0;
  sFORALL(GeoHandles,gh)
    gh->Enqueued = 0;

  // find handles that need update

  DirtyHandles.Clear();
  sFORALL(GeoHandles,gh)
  {
    if(gh->Visible && !gh->Valid)
    {
      DirtyHandles.AddTail(gh);
      gh->Enqueued = 1;
    }
  }

  // update geometries and draw

  gb = Geos[GeoRoundRobin];
  sBool locked = 0;
  MeshVertex *vb = 0;
  sU32 *ib = 0;
  static sInt chargegeo = Geos.GetCount()*2;
  while(!DirtyHandles.IsEmpty())
  {
    gh = DirtyHandles.RemTail();
    sVERIFY(gh->Valid==0);

    // check for space

    if(gb->VertexUsed+gh->VertexCount > gb->VertexAlloc || 
       gb->IndexUsed+gh->IndexCount > gb->IndexAlloc || chargegeo>0)
    {
      if(chargegeo>0) chargegeo--;
      // finish old buffer
      if(locked)
      {
        gb->Geo->EndDynVB();
        gb->Geo->EndDynIB();
        locked = 0;
      }
      Draw(gb);
      gb->Drawn = 1;

      // switch to next buffer

      GeoRoundRobin = GeoRoundRobin + 1;
      if(GeoRoundRobin==Geos.GetCount())
        GeoRoundRobin = 0;
      gb = Geos[GeoRoundRobin];

      vb = (MeshVertex *) gb->Geo->BeginDynVB(1,0);
      ib = (sU32*) gb->Geo->BeginDynIB(1);
      locked = 1;

      // anything important in there?

      OctGeoHandle *oh;
      sFORALL(gb->Handles,oh)
      {
        oh->Valid = 0;
        oh->Geometry = -1;
        if(oh->Visible && !gb->Drawn && ! oh->Enqueued)
          DirtyHandles.AddTail(oh);
      }
      gb->Handles.Clear();
      gb->VertexUsed = 0;
      gb->IndexUsed = 0;
      gb->Drawn = 0;

    }

    // lock buffer

    if(!locked)
    {
      vb = (MeshVertex *) gb->Geo->BeginDynVB();
      ib = (sU32 *) gb->Geo->BeginDynIB();
      locked = 1;
    }

    // copy data & account for it

    gh->Valid = 1;
    gh->Geometry = GeoRoundRobin;
    gh->VertexStart = gb->VertexUsed;
    gh->IndexStart = gb->IndexUsed;
    gb->VertexUsed += gh->VertexCount;
    gb->IndexUsed += gh->IndexCount;
    gb->Handles.AddTail(gh);

#if BLOBHEAP
    sInt vsize = gh->VertexCount*sizeof(MeshVertex);
    sInt isize = gh->IndexCount*sizeof(sU16);
    BlobHeap->CopyFrom(gh->hnd,vb+gh->VertexStart,0,vsize);
    BlobHeap->CopyFrom_UnpackIndex(gh->hnd,ib+gh->IndexStart,vsize,isize,gh->VertexStart);
#else
    sCopyMem(vb+gh->VertexStart,gh->VB,gh->VertexCount*sizeof(MeshVertex));
    for(sInt i=0;i<gh->IndexCount;i++)
      ib[gh->IndexStart+i] = gh->IB[i]+gh->VertexStart;
#endif
  }
  if(locked)
  {
    gb->Geo->EndDynVB();
    gb->Geo->EndDynIB();
  }

  // draw all buffer that are not yet drawn:

  sFORALL(Geos,gb)
    if(gb->Drawn==0)
      Draw(gb);
}

void OctManager::Draw(GeoBuffer *gb)
{
  OctGeoHandle *dh;

  DrawRange.Clear();
  sDrawRange r;
  r.Start = 0;
  r.End = -1;

  sFORALL(gb->Handles,dh)
  {
    if(dh->Visible)
    {
      if(dh->IndexStart==r.End)
      {
        r.End = dh->IndexStart+dh->IndexCount;
      }
      else
      {
        if(r.End>=0)
          DrawRange.AddTail(r);
        r.Start = dh->IndexStart;
        r.End = dh->IndexStart+dh->IndexCount;
      }
    }
  }
  if(r.End>=0)
    DrawRange.AddTail(r);

  if(DrawRange.GetCount()>0)
    gb->Geo->Draw(DrawRange.GetData(),DrawRange.GetCount());
}


/****************************************************************************/
/****************************************************************************/

/*
static const int edgeTable[256]={
0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0   };
*/
static const int TriTable[256][16] =
{
  {  0 , -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  3 ,  0, 8, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  3 ,  0, 1, 9,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  6 ,  1, 8, 3, 9, 8, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  3 ,  3,11, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  6 ,  0,11, 2, 8,11, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  6 ,  1, 9, 0, 2, 3,11,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  1,11, 2, 1, 9,11, 9, 8,11,-1,-1,-1,-1,-1,-1, },
  {  3 ,  1, 2,10,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  6 ,  0, 8, 3, 1, 2,10,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  6 ,  9, 2,10, 0, 2, 9,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  2, 8, 3, 2,10, 8,10, 9, 8,-1,-1,-1,-1,-1,-1, },
  {  6 ,  3,10, 1,11,10, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  0,10, 1, 0, 8,10, 8,11,10,-1,-1,-1,-1,-1,-1, },
  {  9 ,  3, 9, 0, 3,11, 9,11,10, 9,-1,-1,-1,-1,-1,-1, },
  {  6 ,  9, 8,10,10, 8,11,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  3 ,  4, 7, 8,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  6 ,  4, 3, 0, 7, 3, 4,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  6 ,  0, 1, 9, 8, 4, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  4, 1, 9, 4, 7, 1, 7, 3, 1,-1,-1,-1,-1,-1,-1, },
  {  6 ,  8, 4, 7, 3,11, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 , 11, 4, 7,11, 2, 4, 2, 0, 4,-1,-1,-1,-1,-1,-1, },
  {  9 ,  9, 0, 1, 8, 4, 7, 2, 3,11,-1,-1,-1,-1,-1,-1, },
  { 12 ,  4, 7,11, 9, 4,11, 9,11, 2, 9, 2, 1,-1,-1,-1, },
  {  6 ,  1, 2,10, 8, 4, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  3, 4, 7, 3, 0, 4, 1, 2,10,-1,-1,-1,-1,-1,-1, },
  {  9 ,  9, 2,10, 9, 0, 2, 8, 4, 7,-1,-1,-1,-1,-1,-1, },
  { 12 ,  2,10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4,-1,-1,-1, },
  {  9 ,  3,10, 1, 3,11,10, 7, 8, 4,-1,-1,-1,-1,-1,-1, },
  { 12 ,  1,11,10, 1, 4,11, 1, 0, 4, 7,11, 4,-1,-1,-1, },
  { 12 ,  4, 7, 8, 9, 0,11, 9,11,10,11, 0, 3,-1,-1,-1, },
  {  9 ,  4, 7,11, 4,11, 9, 9,11,10,-1,-1,-1,-1,-1,-1, },
  {  3 ,  9, 5, 4,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  6 ,  9, 5, 4, 0, 8, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  6 ,  0, 5, 4, 1, 5, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  8, 5, 4, 8, 3, 5, 3, 1, 5,-1,-1,-1,-1,-1,-1, },
  {  6 ,  9, 5, 4, 2, 3,11,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  0,11, 2, 0, 8,11, 4, 9, 5,-1,-1,-1,-1,-1,-1, },
  {  9 ,  0, 5, 4, 0, 1, 5, 2, 3,11,-1,-1,-1,-1,-1,-1, },
  { 12 ,  2, 1, 5, 2, 5, 8, 2, 8,11, 4, 8, 5,-1,-1,-1, },
  {  6 ,  1, 2,10, 9, 5, 4,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  3, 0, 8, 1, 2,10, 4, 9, 5,-1,-1,-1,-1,-1,-1, },
  {  9 ,  5, 2,10, 5, 4, 2, 4, 0, 2,-1,-1,-1,-1,-1,-1, },
  { 12 ,  2,10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8,-1,-1,-1, },
  {  9 , 10, 3,11,10, 1, 3, 9, 5, 4,-1,-1,-1,-1,-1,-1, },
  { 12 ,  4, 9, 5, 0, 8, 1, 8,10, 1, 8,11,10,-1,-1,-1, },
  { 12 ,  5, 4, 0, 5, 0,11, 5,11,10,11, 0, 3,-1,-1,-1, },
  {  9 ,  5, 4, 8, 5, 8,10,10, 8,11,-1,-1,-1,-1,-1,-1, },
  {  6 ,  9, 7, 8, 5, 7, 9,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  9, 3, 0, 9, 5, 3, 5, 7, 3,-1,-1,-1,-1,-1,-1, },
  {  9 ,  0, 7, 8, 0, 1, 7, 1, 5, 7,-1,-1,-1,-1,-1,-1, },
  {  6 ,  1, 5, 3, 3, 5, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  7, 9, 5, 7, 8, 9, 3,11, 2,-1,-1,-1,-1,-1,-1, },
  { 12 ,  9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7,11,-1,-1,-1, },
  { 12 ,  2, 3,11, 0, 1, 8, 1, 7, 8, 1, 5, 7,-1,-1,-1, },
  {  9 , 11, 2, 1,11, 1, 7, 7, 1, 5,-1,-1,-1,-1,-1,-1, },
  {  9 ,  9, 7, 8, 9, 5, 7,10, 1, 2,-1,-1,-1,-1,-1,-1, },
  { 12 , 10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3,-1,-1,-1, },
  { 12 ,  8, 0, 2, 8, 2, 5, 8, 5, 7,10, 5, 2,-1,-1,-1, },
  {  9 ,  2,10, 5, 2, 5, 3, 3, 5, 7,-1,-1,-1,-1,-1,-1, },
  { 12 ,  9, 5, 8, 8, 5, 7,10, 1, 3,10, 3,11,-1,-1,-1, },
  { 15 ,  5, 7, 0, 5, 0, 9, 7,11, 0, 1, 0,10,11,10, 0, },
  { 15 , 11,10, 0,11, 0, 3,10, 5, 0, 8, 0, 7, 5, 7, 0, },
  {  6 , 11,10, 5, 7,11, 5,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  3 ,  7, 6,11,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  6 ,  3, 0, 8,11, 7, 6,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  6 ,  0, 1, 9,11, 7, 6,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  8, 1, 9, 8, 3, 1,11, 7, 6,-1,-1,-1,-1,-1,-1, },
  {  6 ,  7, 2, 3, 6, 2, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  7, 0, 8, 7, 6, 0, 6, 2, 0,-1,-1,-1,-1,-1,-1, },
  {  9 ,  2, 7, 6, 2, 3, 7, 0, 1, 9,-1,-1,-1,-1,-1,-1, },
  { 12 ,  1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6,-1,-1,-1, },
  {  6 , 10, 1, 2, 6,11, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  1, 2,10, 3, 0, 8, 6,11, 7,-1,-1,-1,-1,-1,-1, },
  {  9 ,  2, 9, 0, 2,10, 9, 6,11, 7,-1,-1,-1,-1,-1,-1, },
  { 12 ,  6,11, 7, 2,10, 3,10, 8, 3,10, 9, 8,-1,-1,-1, },
  {  9 , 10, 7, 6,10, 1, 7, 1, 3, 7,-1,-1,-1,-1,-1,-1, },
  { 12 , 10, 7, 6, 1, 7,10, 1, 8, 7, 1, 0, 8,-1,-1,-1, },
  { 12 ,  0, 3, 7, 0, 7,10, 0,10, 9, 6,10, 7,-1,-1,-1, },
  {  9 ,  7, 6,10, 7,10, 8, 8,10, 9,-1,-1,-1,-1,-1,-1, },
  {  6 ,  6, 8, 4,11, 8, 6,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  3, 6,11, 3, 0, 6, 0, 4, 6,-1,-1,-1,-1,-1,-1, },
  {  9 ,  8, 6,11, 8, 4, 6, 9, 0, 1,-1,-1,-1,-1,-1,-1, },
  { 12 ,  9, 4, 6, 9, 6, 3, 9, 3, 1,11, 3, 6,-1,-1,-1, },
  {  9 ,  8, 2, 3, 8, 4, 2, 4, 6, 2,-1,-1,-1,-1,-1,-1, },
  {  6 ,  0, 4, 2, 4, 6, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  { 12 ,  1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8,-1,-1,-1, },
  {  9 ,  1, 9, 4, 1, 4, 2, 2, 4, 6,-1,-1,-1,-1,-1,-1, },
  {  9 ,  6, 8, 4, 6,11, 8, 2,10, 1,-1,-1,-1,-1,-1,-1, },
  { 12 ,  1, 2,10, 3, 0,11, 0, 6,11, 0, 4, 6,-1,-1,-1, },
  { 12 ,  4,11, 8, 4, 6,11, 0, 2, 9, 2,10, 9,-1,-1,-1, },
  { 15 , 10, 9, 3,10, 3, 2, 9, 4, 3,11, 3, 6, 4, 6, 3, },
  { 12 ,  8, 1, 3, 8, 6, 1, 8, 4, 6, 6,10, 1,-1,-1,-1, },
  {  9 , 10, 1, 0,10, 0, 6, 6, 0, 4,-1,-1,-1,-1,-1,-1, },
  { 15 ,  4, 6, 3, 4, 3, 8, 6,10, 3, 0, 3, 9,10, 9, 3, },
  {  6 , 10, 9, 4, 6,10, 4,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  6 ,  4, 9, 5, 7, 6,11,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  0, 8, 3, 4, 9, 5,11, 7, 6,-1,-1,-1,-1,-1,-1, },
  {  9 ,  5, 0, 1, 5, 4, 0, 7, 6,11,-1,-1,-1,-1,-1,-1, },
  { 12 , 11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5,-1,-1,-1, },
  {  9 ,  7, 2, 3, 7, 6, 2, 5, 4, 9,-1,-1,-1,-1,-1,-1, },
  { 12 ,  9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7,-1,-1,-1, },
  { 12 ,  3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0,-1,-1,-1, },
  { 15 ,  6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, },
  {  9 ,  9, 5, 4,10, 1, 2, 7, 6,11,-1,-1,-1,-1,-1,-1, },
  { 12 ,  6,11, 7, 1, 2,10, 0, 8, 3, 4, 9, 5,-1,-1,-1, },
  { 12 ,  7, 6,11, 5, 4,10, 4, 2,10, 4, 0, 2,-1,-1,-1, },
  { 15 ,  3, 4, 8, 3, 5, 4, 3, 2, 5,10, 5, 2,11, 7, 6, },
  { 12 ,  9, 5, 4,10, 1, 6, 1, 7, 6, 1, 3, 7,-1,-1,-1, },
  { 15 ,  1, 6,10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, },
  { 15 ,  4, 0,10, 4,10, 5, 0, 3,10, 6,10, 7, 3, 7,10, },
  { 12 ,  7, 6,10, 7,10, 8, 5, 4,10, 4, 8,10,-1,-1,-1, },
  {  9 ,  6, 9, 5, 6,11, 9,11, 8, 9,-1,-1,-1,-1,-1,-1, },
  { 12 ,  3, 6,11, 0, 6, 3, 0, 5, 6, 0, 9, 5,-1,-1,-1, },
  { 12 ,  0,11, 8, 0, 5,11, 0, 1, 5, 5, 6,11,-1,-1,-1, },
  {  9 ,  6,11, 3, 6, 3, 5, 5, 3, 1,-1,-1,-1,-1,-1,-1, },
  { 12 ,  5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2,-1,-1,-1, },
  {  9 ,  9, 5, 6, 9, 6, 0, 0, 6, 2,-1,-1,-1,-1,-1,-1, },
  { 15 ,  1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, },
  {  6 ,  1, 5, 6, 2, 1, 6,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  { 12 ,  1, 2,10, 9, 5,11, 9,11, 8,11, 5, 6,-1,-1,-1, },
  { 15 ,  0,11, 3, 0, 6,11, 0, 9, 6, 5, 6, 9, 1, 2,10, },
  { 15 , 11, 8, 5,11, 5, 6, 8, 0, 5,10, 5, 2, 0, 2, 5, },
  { 12 ,  6,11, 3, 6, 3, 5, 2,10, 3,10, 5, 3,-1,-1,-1, },
  { 15 ,  1, 3, 6, 1, 6,10, 3, 8, 6, 5, 6, 9, 8, 9, 6, },
  { 12 , 10, 1, 0,10, 0, 6, 9, 5, 0, 5, 6, 0,-1,-1,-1, },
  {  6 ,  0, 3, 8, 5, 6,10,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  3 , 10, 5, 6,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  3 , 10, 6, 5,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  6 ,  0, 8, 3, 5,10, 6,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  6 ,  9, 0, 1, 5,10, 6,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  1, 8, 3, 1, 9, 8, 5,10, 6,-1,-1,-1,-1,-1,-1, },
  {  6 ,  2, 3,11,10, 6, 5,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 , 11, 0, 8,11, 2, 0,10, 6, 5,-1,-1,-1,-1,-1,-1, },
  {  9 ,  0, 1, 9, 2, 3,11, 5,10, 6,-1,-1,-1,-1,-1,-1, },
  { 12 ,  5,10, 6, 1, 9, 2, 9,11, 2, 9, 8,11,-1,-1,-1, },
  {  6 ,  1, 6, 5, 2, 6, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  1, 6, 5, 1, 2, 6, 3, 0, 8,-1,-1,-1,-1,-1,-1, },
  {  9 ,  9, 6, 5, 9, 0, 6, 0, 2, 6,-1,-1,-1,-1,-1,-1, },
  { 12 ,  5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8,-1,-1,-1, },
  {  9 ,  6, 3,11, 6, 5, 3, 5, 1, 3,-1,-1,-1,-1,-1,-1, },
  { 12 ,  0, 8,11, 0,11, 5, 0, 5, 1, 5,11, 6,-1,-1,-1, },
  { 12 ,  3,11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9,-1,-1,-1, },
  {  9 ,  6, 5, 9, 6, 9,11,11, 9, 8,-1,-1,-1,-1,-1,-1, },
  {  6 ,  5,10, 6, 4, 7, 8,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  4, 3, 0, 4, 7, 3, 6, 5,10,-1,-1,-1,-1,-1,-1, },
  {  9 ,  1, 9, 0, 5,10, 6, 8, 4, 7,-1,-1,-1,-1,-1,-1, },
  { 12 , 10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4,-1,-1,-1, },
  {  9 ,  3,11, 2, 7, 8, 4,10, 6, 5,-1,-1,-1,-1,-1,-1, },
  { 12 ,  5,10, 6, 4, 7, 2, 4, 2, 0, 2, 7,11,-1,-1,-1, },
  { 12 ,  0, 1, 9, 4, 7, 8, 2, 3,11, 5,10, 6,-1,-1,-1, },
  { 15 ,  9, 2, 1, 9,11, 2, 9, 4,11, 7,11, 4, 5,10, 6, },
  {  9 ,  6, 1, 2, 6, 5, 1, 4, 7, 8,-1,-1,-1,-1,-1,-1, },
  { 12 ,  1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7,-1,-1,-1, },
  { 12 ,  8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6,-1,-1,-1, },
  { 15 ,  7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, },
  { 12 ,  8, 4, 7, 3,11, 5, 3, 5, 1, 5,11, 6,-1,-1,-1, },
  { 15 ,  5, 1,11, 5,11, 6, 1, 0,11, 7,11, 4, 0, 4,11, },
  { 15 ,  0, 5, 9, 0, 6, 5, 0, 3, 6,11, 6, 3, 8, 4, 7, },
  { 12 ,  6, 5, 9, 6, 9,11, 4, 7, 9, 7,11, 9,-1,-1,-1, },
  {  6 , 10, 4, 9, 6, 4,10,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  4,10, 6, 4, 9,10, 0, 8, 3,-1,-1,-1,-1,-1,-1, },
  {  9 , 10, 0, 1,10, 6, 0, 6, 4, 0,-1,-1,-1,-1,-1,-1, },
  { 12 ,  8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1,10,-1,-1,-1, },
  {  9 , 10, 4, 9,10, 6, 4,11, 2, 3,-1,-1,-1,-1,-1,-1, },
  { 12 ,  0, 8, 2, 2, 8,11, 4, 9,10, 4,10, 6,-1,-1,-1, },
  { 12 ,  3,11, 2, 0, 1, 6, 0, 6, 4, 6, 1,10,-1,-1,-1, },
  { 15 ,  6, 4, 1, 6, 1,10, 4, 8, 1, 2, 1,11, 8,11, 1, },
  {  9 ,  1, 4, 9, 1, 2, 4, 2, 6, 4,-1,-1,-1,-1,-1,-1, },
  { 12 ,  3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4,-1,-1,-1, },
  {  6 ,  0, 2, 4, 4, 2, 6,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  8, 3, 2, 8, 2, 4, 4, 2, 6,-1,-1,-1,-1,-1,-1, },
  { 12 ,  9, 6, 4, 9, 3, 6, 9, 1, 3,11, 6, 3,-1,-1,-1, },
  { 15 ,  8,11, 1, 8, 1, 0,11, 6, 1, 9, 1, 4, 6, 4, 1, },
  {  9 ,  3,11, 6, 3, 6, 0, 0, 6, 4,-1,-1,-1,-1,-1,-1, },
  {  6 ,  6, 4, 8,11, 6, 8,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  7,10, 6, 7, 8,10, 8, 9,10,-1,-1,-1,-1,-1,-1, },
  { 12 ,  0, 7, 3, 0,10, 7, 0, 9,10, 6, 7,10,-1,-1,-1, },
  { 12 , 10, 6, 7, 1,10, 7, 1, 7, 8, 1, 8, 0,-1,-1,-1, },
  {  9 , 10, 6, 7,10, 7, 1, 1, 7, 3,-1,-1,-1,-1,-1,-1, },
  { 12 ,  2, 3,11,10, 6, 8,10, 8, 9, 8, 6, 7,-1,-1,-1, },
  { 15 ,  2, 0, 7, 2, 7,11, 0, 9, 7, 6, 7,10, 9,10, 7, },
  { 15 ,  1, 8, 0, 1, 7, 8, 1,10, 7, 6, 7,10, 2, 3,11, },
  { 12 , 11, 2, 1,11, 1, 7,10, 6, 1, 6, 7, 1,-1,-1,-1, },
  { 12 ,  1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7,-1,-1,-1, },
  { 15 ,  2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, },
  {  9 ,  7, 8, 0, 7, 0, 6, 6, 0, 2,-1,-1,-1,-1,-1,-1, },
  {  6 ,  7, 3, 2, 6, 7, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  { 15 ,  8, 9, 6, 8, 6, 7, 9, 1, 6,11, 6, 3, 1, 3, 6, },
  {  6 ,  0, 9, 1,11, 6, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  { 12 ,  7, 8, 0, 7, 0, 6, 3,11, 0,11, 6, 0,-1,-1,-1, },
  {  3 ,  7,11, 6,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  6 , 11, 5,10, 7, 5,11,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 , 11, 5,10,11, 7, 5, 8, 3, 0,-1,-1,-1,-1,-1,-1, },
  {  9 ,  5,11, 7, 5,10,11, 1, 9, 0,-1,-1,-1,-1,-1,-1, },
  { 12 , 10, 7, 5,10,11, 7, 9, 8, 1, 8, 3, 1,-1,-1,-1, },
  {  9 ,  2, 5,10, 2, 3, 5, 3, 7, 5,-1,-1,-1,-1,-1,-1, },
  { 12 ,  8, 2, 0, 8, 5, 2, 8, 7, 5,10, 2, 5,-1,-1,-1, },
  { 12 ,  9, 0, 1, 5,10, 3, 5, 3, 7, 3,10, 2,-1,-1,-1, },
  { 15 ,  9, 8, 2, 9, 2, 1, 8, 7, 2,10, 2, 5, 7, 5, 2, },
  {  9 , 11, 1, 2,11, 7, 1, 7, 5, 1,-1,-1,-1,-1,-1,-1, },
  { 12 ,  0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2,11,-1,-1,-1, },
  { 12 ,  9, 7, 5, 9, 2, 7, 9, 0, 2, 2,11, 7,-1,-1,-1, },
  { 15 ,  7, 5, 2, 7, 2,11, 5, 9, 2, 3, 2, 8, 9, 8, 2, },
  {  6 ,  1, 3, 5, 3, 7, 5,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  0, 8, 7, 0, 7, 1, 1, 7, 5,-1,-1,-1,-1,-1,-1, },
  {  9 ,  9, 0, 3, 9, 3, 5, 5, 3, 7,-1,-1,-1,-1,-1,-1, },
  {  6 ,  9, 8, 7, 5, 9, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  5, 8, 4, 5,10, 8,10,11, 8,-1,-1,-1,-1,-1,-1, },
  { 12 ,  5, 0, 4, 5,11, 0, 5,10,11,11, 3, 0,-1,-1,-1, },
  { 12 ,  0, 1, 9, 8, 4,10, 8,10,11,10, 4, 5,-1,-1,-1, },
  { 15 , 10,11, 4,10, 4, 5,11, 3, 4, 9, 4, 1, 3, 1, 4, },
  { 12 ,  2, 5,10, 3, 5, 2, 3, 4, 5, 3, 8, 4,-1,-1,-1, },
  {  9 ,  5,10, 2, 5, 2, 4, 4, 2, 0,-1,-1,-1,-1,-1,-1, },
  { 15 ,  3,10, 2, 3, 5,10, 3, 8, 5, 4, 5, 8, 0, 1, 9, },
  { 12 ,  5,10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2,-1,-1,-1, },
  { 12 ,  2, 5, 1, 2, 8, 5, 2,11, 8, 4, 5, 8,-1,-1,-1, },
  { 15 ,  0, 4,11, 0,11, 3, 4, 5,11, 2,11, 1, 5, 1,11, },
  { 15 ,  0, 2, 5, 0, 5, 9, 2,11, 5, 4, 5, 8,11, 8, 5, },
  {  6 ,  9, 4, 5, 2,11, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  8, 4, 5, 8, 5, 3, 3, 5, 1,-1,-1,-1,-1,-1,-1, },
  {  6 ,  0, 4, 5, 1, 0, 5,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  { 12 ,  8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5,-1,-1,-1, },
  {  3 ,  9, 4, 5,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  4,11, 7, 4, 9,11, 9,10,11,-1,-1,-1,-1,-1,-1, },
  { 12 ,  0, 8, 3, 4, 9, 7, 9,11, 7, 9,10,11,-1,-1,-1, },
  { 12 ,  1,10,11, 1,11, 4, 1, 4, 0, 7, 4,11,-1,-1,-1, },
  { 15 ,  3, 1, 4, 3, 4, 8, 1,10, 4, 7, 4,11,10,11, 4, },
  { 12 ,  2, 9,10, 2, 7, 9, 2, 3, 7, 7, 4, 9,-1,-1,-1, },
  { 15 ,  9,10, 7, 9, 7, 4,10, 2, 7, 8, 7, 0, 2, 0, 7, },
  { 15 ,  3, 7,10, 3,10, 2, 7, 4,10, 1,10, 0, 4, 0,10, },
  {  6 ,  1,10, 2, 8, 7, 4,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  { 12 ,  4,11, 7, 9,11, 4, 9, 2,11, 9, 1, 2,-1,-1,-1, },
  { 15 ,  9, 7, 4, 9,11, 7, 9, 1,11, 2,11, 1, 0, 8, 3, },
  {  9 , 11, 7, 4,11, 4, 2, 2, 4, 0,-1,-1,-1,-1,-1,-1, },
  { 12 , 11, 7, 4,11, 4, 2, 8, 3, 4, 3, 2, 4,-1,-1,-1, },
  {  9 ,  4, 9, 1, 4, 1, 7, 7, 1, 3,-1,-1,-1,-1,-1,-1, },
  { 12 ,  4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1,-1,-1,-1, },
  {  6 ,  4, 0, 3, 7, 4, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  3 ,  4, 8, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  6 ,  9,10, 8,10,11, 8,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  3, 0, 9, 3, 9,11,11, 9,10,-1,-1,-1,-1,-1,-1, },
  {  9 ,  0, 1,10, 0,10, 8, 8,10,11,-1,-1,-1,-1,-1,-1, },
  {  6 ,  3, 1,10,11, 3,10,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  2, 3, 8, 2, 8,10,10, 8, 9,-1,-1,-1,-1,-1,-1, },
  {  6 ,  9,10, 2, 0, 9, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  { 12 ,  2, 3, 8, 2, 8,10, 0, 1, 8, 1,10, 8,-1,-1,-1, },
  {  3 ,  1,10, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  9 ,  1, 2,11, 1,11, 9, 9,11, 8,-1,-1,-1,-1,-1,-1, },
  { 12 ,  3, 0, 9, 3, 9,11, 1, 2, 9, 2,11, 9,-1,-1,-1, },
  {  6 ,  0, 2,11, 8, 0,11,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  3 ,  3, 2,11,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  6 ,  1, 3, 8, 9, 1, 8,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  3 ,  0, 9, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  3 ,  0, 3, 8,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
  {  0 , -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, },
};

/****************************************************************************/

sF32 func2d(sF32 cx,sF32 cy)
{
  
  sF32 zx = 0;
  sF32 zy = 0;
  sInt i = 0;
  sF32 abs = 0;
  do
  {
    sF32 xt = zx*zx-zy*zy+cx;
    sF32 yt = 2*zx*zy+cy;
    i++;
    abs = xt*xt+yt*yt;
    if(abs>4.0f)
      break;
    zx = xt;
    zy = yt;
  }
  while(i<64);
  return i+(4/abs)-8.5f;
  

  //return sSqrt(cx*cx+cy*cy)*8;
}


//sF32 func3d(sF32 cx,sF32 cz,sF32 cy)
sF32 func3d(sF32 cx,sF32 cy,sF32 cz)
{
  sF32 x = cx;
  sF32 y = cy;
  sF32 z = cz;
  sInt i = 0;
  const sF32 n = 8;
  sF32 abs;
  do
  {
    if(0)
    {
      sF32 r = sSqrt(x*x + y*y + z*z );
      sF32 theta = sATan2(sSqrt(x*x + y*y) , z);
      sF32 phi = sATan2(y,x);

      sF32 pw = r*r;  pw = pw*pw;  pw = pw*pw;  // pw = sPow(r,n) | n==8

      x = cx + pw * sSin(theta*n) * sCos(phi*n);
      y = cy + pw * sSin(theta*n) * sSin(phi*n);
      z = cz + pw * sCos(theta*n);
    }
    else
    {
      sF32 r2 = x*x+y*y;
      sF32 r4 = r2*r2;
      sF32 r6 = r2*r4;
      sF32 r8 = r4*r4;

      sF32 x2 = x*x;
      sF32 x4 = x2*x2;
      sF32 x6 = x2*x4;
      sF32 x8 = x4*x4;

      sF32 y2 = y*y;
      sF32 y4 = y2*y2;
      sF32 y6 = y2*y4;
      sF32 y8 = y4*y4;

      sF32 z2 = z*z;
      sF32 z4 = z2*z2;
      sF32 z6 = z2*z4;
      sF32 z8 = z4*z4;

      sF32 a = 1 + (z8 - 28*z6*r2 + 70*z4*r4 - 28*z2*r6) / (r8);

      sF32 xx = a * (x8 - 28*x6*y2 + 70*x4*y4 - 28*x2*y6 + y8);
      sF32 yy = (8*a*x*y) * (x6 - 7*x4*y2 + 7*x2*y4 - y6);
      sF32 zz = (8*z*sFSqrt(r2)) * (z2-r2) * (z4 - 6*z2*r2 + r4);

      x = cx+xx;
      y = cy+yy;
      z = cz+zz;
    }
    abs = x*x + y*y + z*z;
    i++;
  }
  while(i<5 && abs<2);
  return i+(2/abs)-4.5f;
}


/****************************************************************************/


OctNode::OctNode()
{
  Clear();
}

OctNode::~OctNode()
{
  Free();
}

void OctNode::Clear()
{
  VertCount = 0;
  x = y = z = q = 0;
  for(sInt i=0;i<8;i++)
  {
    Childs[i] = 0;
    NewChilds[i] = 0;
    PotentialChild[i] = 1;
  }
  HasChilds = 0;
  Splittable = 0;
  Splitting = 0;
  Initializing = 1;
  Evictable = 0;
  Area = BestArea = WorstArea = 0;
  Parent = 0;

  VertexUsed = 0;
  VertexAlloc = 0;
  VB = 0;
  IndexUsed = 0;
  IndexAlloc = 0;
  IB = 0;

  icache = 0;
  pot = 0;
  Bundle = 0;
}

void OctNode::Free()
{
  sVERIFY(!Initializing || OctMan->EndGame);
  sVERIFY(!Splitting || OctMan->EndGame);
  for(sInt i=0;i<8;i++)
  {
    OctMan->FreeNode(Childs[i]);
    OctMan->FreeNode(NewChilds[i]);
  }
  OctMan->VerticesLife -= VertCount;
  OctMan->FreeHandle(GeoHandle);
  if(Bundle)
    OctMan->FreeMemBundle(Bundle);
  Clear();
}


OctNode *OctNode::FindBest()
{
  if(Splittable)
    return this;
  if(HasChilds)
  {
    sF32 a = 0;
    OctNode *r = 0;
    for(sInt i=0;i<8;i++)
    {
      if(Childs[i] && Childs[i]->BestArea > a)
      {
        r = Childs[i];
        a = Childs[i]->BestArea;
      }
    }
    if(r)
      return r->FindBest();
  }
  return 0;
}

OctNode *OctNode::FindWorst()
{
  if(Evictable)
    return this;
  if(HasChilds)
  {
    sF32 a = 999;
    OctNode *r = 0;
    for(sInt i=0;i<8;i++)
    {
      if(Childs[i] && Childs[i]->WorstArea < a)
      {
        r = Childs[i];
        a = Childs[i]->WorstArea;
      }
    }
    if(r)
      return r->FindWorst();
  }
  return 0;
}

void OctNode::DeleteChilds()
{
  if(Evictable)
  {
    sVERIFY(!Splitting);
    for(sInt i=0;i<8;i++)
    {
      OctMan->FreeNode(Childs[i]);
      Childs[i] = 0;
    }
    HasChilds = 0;
    Evictable = 0;
    WorstArea = 999;
    UpdateArea();
  }
}

void OctNode::UpdateArea()
{
  if(HasChilds)
  {
    BestArea = 0;
    WorstArea = 999;
    for(sInt i=0;i<8;i++)
    {
      if(Childs[i])
      {
        BestArea = sMax(BestArea,Childs[i]->BestArea);
        WorstArea = sMin(WorstArea,Childs[i]->WorstArea);
      }
    }
  }
  if(Parent)
  {
    Parent->UpdateArea();
  }
}

/****************************************************************************/

sBool OctNode::PrepareDraw(const sViewport &view)
{
  sVector30 d = view.Camera.l - Center;
  Area = (1/sSqrt(d^d)/q);
  Evictable = 0;
  Splittable = 0;
  BestArea = 0;
  WorstArea = 999;
  GeoHandle.Visible = 0;
  if(HasChilds)
  {
    sInt n = 0;
    sBool childsplitting = 0;
    for(sInt i=0;i<8;i++)
    {
      if(Childs[i])
      {
        childsplitting |= Childs[i]->PrepareDraw(view);
        if(Childs[i]->HasChilds) n++;
        BestArea = sMax(BestArea,Childs[i]->BestArea);
        WorstArea = sMin(WorstArea,Childs[i]->WorstArea);
      }
      if(NewChilds[i])
        childsplitting = 1;
    }
    if(n==0 && !childsplitting)    // candates for eviction (childs, but no subchilds)
    {
      Evictable = 1;
      WorstArea = Area;
    }

    return childsplitting;
  }
  else          // candiate for splitting (no childs)
  {
    if(!Splitting)
    {
      sF32 dist;
      if(view.VisibleDist(Box,dist))
      {
        Splittable = 1;
        BestArea = Area;
      }
    }
    return Splitting;
  }
}

void OctNode::Draw(sMaterial *mtrl,const sViewport &view)
{
  sF32 dist;
  if(view.VisibleDist(Box,dist))
  {
    if(HasChilds && Area>MANDEL_DRAW)
    {
      for(sInt i=0;i<8;i++)
        if(Childs[i])
          Childs[i]->Draw(mtrl,view);
    }
    else
    {
      if(VertCount)
      {
        GeoHandle.Visible = 1;

        OctMan->VerticesDrawn += VertCount;
        OctMan->NodesDrawn ++;
        OctMan->DeepestDrawn = sMax(OctMan->DeepestDrawn,q);
      }
    }
  }
}

/****************************************************************************/
/****************************************************************************/

void OctNode::MakeChilds0()
{
  if(Splittable && !HasChilds)
  {
    Splittable = 0;
    Splitting = 1;
    BestArea = 0;
    for(sInt i=0;i<8;i++)
    {
      if(PotentialChild[i])
      {
        NewChilds[i] = OctMan->AllocNode();
        NewChilds[i]->Parent = this;
        sInt x0 = (i&1)?1:0;
        sInt y0 = (i&2)?1:0;
        sInt z0 = (i&4)?1:0;
        NewChilds[i]->Init0(x*2+x0,y*2+y0,z*2+z0,q*2);
      }
    }
    OctMan->Pipeline0.AddTail(this);
    UpdateArea();
  }
}


void OctNode::Init0(sInt x_,sInt y_,sInt z_,sInt q_)
{
  sVERIFY(q==0);
  x = x_;
  y = y_;
  z = z_;
  q = q_;

  Initializing = 1;

  Bundle = OctMan->AllocMemBundle();
  VertexUsed = 0;
  VertexAlloc = Bundle->VertexAlloc;
  IndexUsed = 0;
  IndexAlloc = Bundle->IndexAlloc;
  VB = Bundle->VB;
  IB = Bundle->IB;
  pot = Bundle->pot;
  icache = Bundle->icache;

  // render 

  if(1)
  {
    sF32 divi = 1.0f/(n*q);
    OctMan->AddRender(sVector4(x*n*divi,y*n*divi,z*n*divi,divi),pot);
  }
}

static void MandelTaskFunc(sStsManager *,sStsThread *,sInt start,sInt count,void *data)
{
  OctNode *n = (OctNode *) data;
  n->Init1();
}

void OctNode::MakeChilds1(sStsWorkload *wl)
{
  sVERIFY(Splitting);
  for(sInt i=0;i<8;i++)
  {
    if(NewChilds[i])
    {
      if(wl)
      {
        wl->AddTask(wl->NewTask(MandelTaskFunc,NewChilds[i],1,0));
      }
      else
      {
        NewChilds[i]->Init1();
      }
    }
  }
}

void OctNode::MakeChilds2()
{
  sVERIFY(Splitting);
  for(sInt i=0;i<8;i++)
  {
    if(NewChilds[i])
    {
      NewChilds[i]->Init2();
      Childs[i] = NewChilds[i];
      NewChilds[i] = 0;
    }
  }
  Splitting = 0;
  HasChilds = 1;
  UpdateArea();
}

void OctNode::Init1()
{
  sVERIFY(Initializing);

  // set up constants

  for(sInt i=0;i<n1*n1*n1*3;i++)
    icache[i] = -1;

  sHalfFloat *f16 = (sHalfFloat *) pot;
  for(sInt i=mmm-1;i>=0;i--)
    pot[i] = f16[i].Get();

  // continue

//  for(sInt i=0;i<mmm;i++)
//    pot[i] = -1;

  sF32 divi = 1.0f/(n*q);
//  const sF32 s = 2.5f;

  static const sInt edges[12][7] = 
  {// z y x  z y x  dir    left vertex is always lower.
    { 0,0,0, 0,0,1,  0 },
    { 0,0,1, 0,1,1,  1 },
    { 0,1,0, 0,1,1,  0 },
    { 0,0,0, 0,1,0,  1 },

    { 1,0,0, 1,0,1,  0 },
    { 1,0,1, 1,1,1,  1 },
    { 1,1,0, 1,1,1,  0 },
    { 1,0,0, 1,1,0,  1 },

    { 0,0,0, 1,0,0,  2 },
    { 0,0,1, 1,0,1,  2 },
    { 0,1,1, 1,1,1,  2 },
    { 0,1,0, 1,1,0,  2 },
  };

  sInt countref[3][m];
  for(sInt i=0;i<m;i++)
  {
    if(i>=no+0 && i<no+n/2)
    {
      countref[0][i] = 0;
      countref[1][i] = 0*4;
      countref[2][i] = 0*4*4;
    }
    else if(i==no+n/2)
    {
      countref[0][i] = 1;
      countref[1][i] = 1*4;
      countref[2][i] = 1*4*4;
    }
    else if(i>no+n/2 && i<=no+n)
    {
      countref[0][i] = 2;
      countref[1][i] = 2*4;
      countref[2][i] = 2*4*4;
    }
    else
    {
      countref[0][i] = 3;
      countref[1][i] = 3*4;
      countref[2][i] = 3*4*4;
    }
  }

  Box.Min.x = sF32(x*n)*divi;
  Box.Min.y = sF32(y*n)*divi;
  Box.Min.z = sF32(z*n)*divi;
  Box.Max.x = sF32(x*n+n)*divi;
  Box.Max.y = sF32(y*n+n)*divi;
  Box.Max.z = sF32(z*n+n)*divi;
  Center.x = sF32(x*n+n/2)*divi;
  Center.y = sF32(y*n+n/2)*divi;
  Center.z = sF32(z*n+n/2)*divi;

  sInt posq[4*4*4];
  sClear(posq);

  for(sInt tz = 0;tz<m;tz++)
  {
    for(sInt ty = 0;ty<m;ty++)
    {
      for(sInt tx = 0;tx<m;tx++)
      {
        if(0)
        {
          const sF32 s = 1.5f;
          sF32 fx = sF32(x*n+tx)*divi;
          sF32 fy = sF32(y*n+ty)*divi;
          sF32 fz = sF32(z*n+tz)*divi;

          sF32 f = func3d(fx*s,fy*s,fz*s);
          pot[tz*mm+ty*m+tx] = f;
        }
        sF32 f = pot[tz*mm+ty*m+tx];
        if(f>0)
        {
          sInt i = countref[0][tx]+countref[1][ty]+countref[2][tz];
          posq[i]++;
        }
      }
    }
  }

//  if(q>=4)      // here come the holes from!  need to check distance to iso-surface!
  {
    for(sInt i=0;i<8;i++)
    {
      sInt cp = 0;
      sInt px = (i&1) ? 1 : 0;
      sInt py = (i&2) ? 4 : 0;
      sInt pz = (i&4) ? 16 : 0;
      cp += posq[px+py+pz       ];
      cp += posq[px+py+pz  +4   ];
      cp += posq[px+py+pz    +16];
      cp += posq[px+py+pz  +4+16];
      cp += posq[px+py+pz+1     ];
      cp += posq[px+py+pz+1+4   ];
      cp += posq[px+py+pz+1  +16];
      cp += posq[px+py+pz+1+4+16];

      PotentialChild[i] = !(cp==0 || cp==(n/2+1)*(n/2+1)*(n/2+1));
    }

    sInt potentials = 0;
    for(sInt i=0;i<8;i++)
      potentials = potentials | PotentialChild[i];
    if(!potentials)
    {
      return;
    }
  }
  

  OctManager::MeshVertex *vp = VB;
  sU16 *ip = IB;

  for(sInt tz = 0;tz<n;tz++)
  {
    for(sInt ty = 0;ty<n;ty++)
    {
      for(sInt tx = 0;tx<n;tx++)
      {
        sF32 fx = sF32(x*n+tx)*divi;
        sF32 fy = sF32(y*n+ty)*divi;
        sF32 fz = sF32(z*n+tz)*divi;

        sInt mask = 0;
        if (pot[(tz+0+no)*mm+(ty+0+no)*m+(tx+0+no)]>0) mask |= 0x01;
        if (pot[(tz+0+no)*mm+(ty+0+no)*m+(tx+1+no)]>0) mask |= 0x02;
        if (pot[(tz+0+no)*mm+(ty+1+no)*m+(tx+0+no)]>0) mask |= 0x04;
        if (pot[(tz+0+no)*mm+(ty+1+no)*m+(tx+1+no)]>0) mask |= 0x08;

        if (pot[(tz+1+no)*mm+(ty+0+no)*m+(tx+0+no)]>0) mask |= 0x10;
        if (pot[(tz+1+no)*mm+(ty+0+no)*m+(tx+1+no)]>0) mask |= 0x20;
        if (pot[(tz+1+no)*mm+(ty+1+no)*m+(tx+0+no)]>0) mask |= 0x40;
        if (pot[(tz+1+no)*mm+(ty+1+no)*m+(tx+1+no)]>0) mask |= 0x80;

        sInt vc = TriTable[mask][0];
        if(vc>0)
        {
          if(!(VertexUsed+vc<=VertexAlloc && IndexUsed+vc<=IndexAlloc))
            sDPrintF(L"%d %d %d | %d %d %d\n",VertexUsed,vc,VertexAlloc,IndexUsed,vc,IndexAlloc);
          sVERIFY(VertexUsed+vc<=VertexAlloc && IndexUsed+vc<=IndexAlloc);

          for(sInt i=0;i<vc;i+=3)
          {
            sVector31 pos[3];
            sVector30 norm[3];
            sInt verts[3];
            for(sInt j=0;j<3;j++)
            {
              const sInt edge = TriTable[mask][1+i+j];
              sInt e0x = edges[edge][2];
              sInt e0y = edges[edge][1];
              sInt e0z = edges[edge][0];

              sInt cacheindex = (n1*n1*n1)*edges[edge][6]
                              + (n1*n1   )*(tz+e0z)
                              + (n1      )*(ty+e0y)
                              + (1       )*(tx+e0x);
              sVERIFY(cacheindex>=0 && cacheindex<n1*n1*n1*3);
              verts[j] = icache[cacheindex];
              if(verts[j]==-1)
              {
                sInt e1x = edges[edge][5];
                sInt e1y = edges[edge][4];
                sInt e1z = edges[edge][3];

                sVector31 p0,p1;
                p0.x = fx + e0x*divi;
                p0.y = fy + e0y*divi;
                p0.z = fz + e0z*divi;
                p1.x = fx + e1x*divi;
                p1.y = fy + e1y*divi;
                p1.z = fz + e1z*divi;

                sF32 f0   = pot[(tz+e0z  +no)*mm + (ty+e0y  +no)*m + (tx+e0x  +no)];
                sF32 f1   = pot[(tz+e1z  +no)*mm + (ty+e1y  +no)*m + (tx+e1x  +no)];

                sF32 n0mx = pot[(tz+e0z  +no)*mm + (ty+e0y  +no)*m + (tx+e0x-1+no)];
                sF32 n0my = pot[(tz+e0z  +no)*mm + (ty+e0y-1+no)*m + (tx+e0x  +no)];
                sF32 n0mz = pot[(tz+e0z-1+no)*mm + (ty+e0y  +no)*m + (tx+e0x  +no)];
                sF32 n0px = pot[(tz+e0z  +no)*mm + (ty+e0y  +no)*m + (tx+e0x+1+no)];
                sF32 n0py = pot[(tz+e0z  +no)*mm + (ty+e0y+1+no)*m + (tx+e0x  +no)];
                sF32 n0pz = pot[(tz+e0z+1+no)*mm + (ty+e0y  +no)*m + (tx+e0x  +no)];

                sF32 n1mx = pot[(tz+e1z  +no)*mm + (ty+e1y  +no)*m + (tx+e1x-1+no)];
                sF32 n1my = pot[(tz+e1z  +no)*mm + (ty+e1y-1+no)*m + (tx+e1x  +no)];
                sF32 n1mz = pot[(tz+e1z-1+no)*mm + (ty+e1y  +no)*m + (tx+e1x  +no)];
                sF32 n1px = pot[(tz+e1z  +no)*mm + (ty+e1y  +no)*m + (tx+e1x+1+no)];
                sF32 n1py = pot[(tz+e1z  +no)*mm + (ty+e1y+1+no)*m + (tx+e1x  +no)];
                sF32 n1pz = pot[(tz+e1z+1+no)*mm + (ty+e1y  +no)*m + (tx+e1x  +no)];

                sVector30 n0(n0px-n0mx,n0py-n0my,n0pz-n0mz); n0.UnitFast();
                sVector30 n1(n1px-n1mx,n1py-n1my,n1pz-n1mz); n1.UnitFast();

                sF32 u = f0/(f0-f1);

                sVector31 pos_;
                sVector30 norm_;
                pos_.Fade(u,p0,p1);
                norm_.Fade(u,n0,n1);

                vp->Init(pos_.x,pos_.y,pos_.z,norm_.x,norm_.y,norm_.z);
                vp++;
                icache[cacheindex] = verts[j] = VertexUsed++;
              }
            }

            ip[0] = verts[0];
            ip[1] = verts[1];
            ip[2] = verts[2];
            ip+=3;
            IndexUsed+=3;
          }
        }
      }
    }
  }

  VertCount = VertexUsed;
}

void OctNode::Init2()
{
  sVERIFY(Initializing);
  OctMan->AllocHandle(GeoHandle);
  GeoHandle.Alloc(VertexUsed,IndexUsed);
#if BLOBHEAP
  sInt vsize = VertexUsed*sizeof(OctManager::MeshVertex);
  sInt isize = IndexUsed*sizeof(sU16);
  OctMan->BlobHeap->CopyInto(GeoHandle.hnd,0,VB,vsize);
  OctMan->BlobHeap->CopyInto(GeoHandle.hnd,vsize,IB,isize);
#else
  sCopyMem(GeoHandle.VB,VB,VertexUsed*sizeof(OctManager::MeshVertex));
  sCopyMem(GeoHandle.IB,IB,IndexUsed*sizeof(sU16));
#endif

  OctMan->FreeMemBundle(Bundle);
  Bundle = 0;
  VB = 0;
  IB = 0;
  pot = 0;
  icache = 0;
  IndexAlloc = IndexUsed = 0;
  VertexAlloc = VertexUsed = 0;

  OctMan->VerticesLife += VertCount;
  Initializing = 0;
}

/****************************************************************************/

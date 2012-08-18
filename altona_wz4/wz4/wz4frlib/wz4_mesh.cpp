/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4frlib/wz4_mesh.hpp"
#include "wz4frlib/wz4_mesh_ops.hpp"
#include "util/algorithms.hpp"
#include "wz4frlib/wz4_mtrl2.hpp"
//#include "wz4frlib/chaosmesh_code.hpp"

struct SolidVertex
{
  sVector31 Pos;
  sVector30 Normal;
  sVector30 Tangent;
  sF32 BiSign;
  sF32 u0,v0,u1,v1;
};

struct SolidVertexAnim
{
  sVector31 Pos;
  sVector30 Normal;
  sVector30 Tangent;
  sF32 BiSign;
  sU8 Index0;
  sU8 Index1;
  sU8 Index2;
  sU8 Index3;
  sU8 Weight2;
  sU8 Weight1;
  sU8 Weight0;
  sU8 Weight3;
  sF32 u0,v0,u1,v1;
};

static sBool logic(sInt logic,sF32 select)
{
  switch(logic)
  {
  default:
    return 1;
  case 1:
    return 0;
  case 2:
    return select>=0.5f;
  case 3:
    return select<0.5f;
  }
}

void OptimizeIndexOrder(sInt *il,sInt ic,sInt vc);
void DumpCacheEfficiency(const sInt *il,sInt ic);

/****************************************************************************/
/***                                                                      ***/
/***   Components                                                         ***/
/***                                                                      ***/
/****************************************************************************/

Wz4MeshCluster::Wz4MeshCluster()
{
  Geo[0] = 0;
  Geo[1] = 0;
  InstanceGeo[0] = 0;
  InstanceGeo[1] = 0;
  InstanceGeo[2] = 0;
  InstanceGeo[3] = 0;
  Mtrl = 0;
  Temp = 0;
  IndexSize = 0;
  LocalRenderPass = 0;
}

Wz4MeshCluster::~Wz4MeshCluster()
{
  delete Geo[0];
  delete Geo[1];
  delete InstanceGeo[0];
  delete InstanceGeo[1];
  delete InstanceGeo[2];
  delete InstanceGeo[3];
  Mtrl->Release();
}

/****************************************************************************/

void Wz4MeshVertex::Init()
{
  Pos.Init(0,0,0);
  Normal.Init(0,0,1);
  Tangent.Init(0,1,0);
  BiSign = 1;
  U0 = V0 = 0;
  U1 = V1 = 0;
#if !WZ4MESH_LOWMEM
  Color0 = 0xffffffff;
  Color1 = 0xffffffff;
#endif
  Index[0] = -1;
  Index[1] = -1;
  Index[2] = -1;
  Index[3] = -1;
  Weight[0] = 0;
  Weight[1] = 0;
  Weight[2] = 0;
  Weight[3] = 0;
  Select = 0.0f;
  Temp = 0;
}

void Wz4MeshVertex::Init(const sVector31 &pos,sF32 u,sF32 v)
{
  Init();
  Pos = pos;
  U0 = u;
  V0 = v;
}

void Wz4MeshVertex::Transform(const sMatrix34 &mat,const sMatrix34 &matInvT)
{
  Pos = Pos * mat;
  Normal = Normal * matInvT;  Normal.Unit();
  Tangent = Tangent * mat;  Tangent.Unit();
}

void Wz4MeshVertex::AddWeight(sInt index,sF32 weight)
{
  if (weight<=0.0f || index<0) return;
  sInt smallestslot;
  sF32 smallestweight;
  for (sInt i=0; i<4; i++)
  {
    if (Index[i]==index)
    {
      Weight[i]+=weight;
      return;
    }
    if (Index[i]<0)
    {
      Index[i]=index;
      Weight[i]=weight;
      return;
    }
    if (!i || Weight[i]<smallestweight)
    {
      smallestweight=Weight[i];
      smallestslot=i;
    }
  }

  if (weight>smallestweight)
  {
    Weight[smallestslot]=weight;
    Index[smallestslot]=index;
  }
}

void Wz4MeshVertex::NormWeight()
{
  sF32 sum=0;
  for (sInt i=0; i<4; i++)
    if (Index[i]>=0)
      sum+=Weight[i];

  if (sum>0)
  {
    sum=1.0f/sum;
    for (sInt i=0; i<4; i++)
      if (Index[i]>=0)
        Weight[i]*=sum;
  }
}

void Wz4MeshVertex::Skin(sMatrix34 *basemat,sInt max,sVector31 &pos)
{
  if(Index[0]<0 || Index[0]>=max)
  {
    pos = Pos;
  }
  else
  {
    sVector30 accu;

    for(sInt i=0;i<4;i++)
    {
      if(Index[i]>=0 && Index[i]<max)
        accu += sVector30(Pos*basemat[Index[i]])*Weight[i];
      else
        break;
    }
    pos = sVector31(accu);
  }
}

sBool Wz4MeshVertex::operator==(const Wz4MeshVertex &v) const
{
  if(Pos.x!=v.Pos.x) return 0;
  if(Pos.y!=v.Pos.y) return 0;
  if(Pos.z!=v.Pos.z) return 0;
  if(U0!=v.U0) return 0;
  if(V0!=v.V0) return 0;
#if !WZ4MESH_LOWMEM
  if(Color0!=v.Color0) return 0;
#endif
  if(Normal.x!=v.Normal.x) return 0;
  if(Normal.y!=v.Normal.y) return 0;
  if(Normal.z!=v.Normal.z) return 0;
  
  if(Tangent.x!=v.Tangent.x) return 0;
  if(Tangent.y!=v.Tangent.y) return 0;
  if(Tangent.z!=v.Tangent.z) return 0;
  if(BiSign!=v.BiSign) return 0;
  
  if(U1!=v.U1) return 0;
  if(V1!=v.V1) return 0;
#if !WZ4MESH_LOWMEM
  if(Color1!=v.Color1) return 0;
#endif
  for(sInt i=0;i<4;i++)
  {
    if(Index[i]!=v.Index[i]) return 0;
    if(Index[i]>=0 && Weight[i]!=v.Weight[i]) return 0;
  }

  return 1;
}

sU32 Wz4MeshVertex::Hash() const
{
  sU32 *ptr = (sU32 *) this;
  return sChecksumMurMur(ptr,5) ^ ptr[10]^ptr[11];

  ////     pos                    normal          uv0
  //return ptr[0]^ptr[1]^ptr[2] ^ ptr[3]^ptr[4] ^ ptr[10]^ptr[11];
}

void Wz4MeshVertex::Zero()
{
  Pos.Init(0,0,0);
  Normal.Init(0,0,0);
  Tangent.Init(0,0,0);
  BiSign = 0;
  U0 = 0;
  V0 = 0;
  U1 = 0;
  V1 = 0;
#if !WZ4MESH_LOWMEM
  Color0 = 0;
  Color1 = 0;
#endif
  for(sInt i=0;i<4;i++)
  {
    Index[i] = -1;
    Weight[i] = 0;
  }
  Select = 0.0f;
  Temp = 0;
}

void Wz4MeshVertex::AddScale(Wz4MeshVertex *v,sF32 scale)
{
  Pos.x += v->Pos.x * scale;
  Pos.y += v->Pos.y * scale;
  Pos.z += v->Pos.z * scale;
  Normal.x += v->Normal.x * scale;
  Normal.y += v->Normal.y * scale;
  Normal.z += v->Normal.z * scale;
  BiSign += v->BiSign * scale;
  U0 += v->U0 * scale;
  V0 += v->V0 * scale;
  U1 += v->U1 * scale;
  V1 += v->V1 * scale;
#if !WZ4MESH_LOWMEM
  Color0 = sAddColor(Color0,sFadeColor(scale*0x10000,0,v->Color0));
  Color1 = sAddColor(Color1,sFadeColor(scale*0x10000,0,v->Color1));
#endif
  for(sInt i=0;i<4;i++)
    if(v->Index[i]>=0)
      AddWeight(v->Index[i],v->Weight[i]*scale);
}

void Wz4MeshVertex::CopyFrom(Wz4MeshVertex *v)
{
  Pos = v->Pos;
  Normal = v->Normal;
  BiSign = v->BiSign;
  U0 = v->U0;
  V0 = v->V0;
  U1 = v->U1;
  V1 = v->V1;
#if !WZ4MESH_LOWMEM
  Color0 = v->Color0;
  Color1 = v->Color1;
#endif
  Select = v->Select;
  Temp = v->Temp;

  for(sInt i=0;i<4;i++)
  {
    Index[i] = v->Index[i];
    Weight[i] = v->Weight[i];
  }
}

/****************************************************************************/

void Wz4MeshFace::Init(sInt count)
{
  Cluster = 0;
  Count = count;
  Select = 0.0f;
  Vertex[0] = 0;
  Vertex[1] = 0;
  Vertex[2] = 0;
  Vertex[3] = 0;
  Selected = 0;
}

void Wz4MeshFace::Invert()
{
  if(Count==3)
  {
    sSwap(Vertex[0],Vertex[2]);
  }
  else
  {
    sSwap(Vertex[0],Vertex[3]);
    sSwap(Vertex[1],Vertex[2]);
  }
}

/****************************************************************************/

void Wz4ChunkPhysics::Transform(const sMatrix34 &mat)
{
  COM = COM * mat;

  sF32 det = mat.Determinant3x3();
  Volume *= det;

  sMatrix34 inert,matT;
  matT = mat;
  matT.Trans3();

  // this could be done way more efficiently.
  inert.i.x = InertD.x;
  inert.j.y = InertD.y;
  inert.k.z = InertD.z;
  
  inert.i.y = inert.j.x = InertOD.x;
  inert.i.z = inert.k.x = InertOD.z;
  inert.j.z = inert.k.y = InertOD.y;

  inert = matT * inert * mat;

  InertD.x = det * inert.i.x;
  InertD.y = det * inert.j.y;
  InertD.z = det * inert.k.z;

  InertOD.x = det * inert.i.y;
  InertOD.y = det * inert.j.z;
  InertOD.z = det * inert.i.z;
}

void Wz4ChunkPhysics::GetInertiaTensor(sMatrix34 &tensor) const
{
  tensor.i.x = InertD.y + InertD.z;
  tensor.j.y = InertD.z + InertD.x;
  tensor.k.z = InertD.x + InertD.y;

  tensor.i.y = tensor.j.x = -InertOD.x;
  tensor.i.z = tensor.k.x = -InertOD.z;
  tensor.j.z = tensor.k.y = -InertOD.y;

  tensor.l.Init(0.0f,0.0f,0.0f);
}

/****************************************************************************/
/***                                                                      ***/
/***   Constructor / Destructor                                           ***/
/***                                                                      ***/
/****************************************************************************/

Wz4Mesh::Wz4Mesh()
{
  Type = Wz4MeshType;
  WireGeoLines = 0;
  WireGeoVertex = 0;
  WireGeoFaces = 0;
  InstanceGeo = 0;
  InstancePlusGeo = 0;
  WireGeoInst = 0;
  Skeleton = 0;
  BBoxValid = 0;
  SaveFlags = 0;
  ChargeCount = 0;
  DontClearVertices = 0;
}

/****************************************************************************/

Wz4Mesh::~Wz4Mesh()
{
  sDeleteAll(Clusters);
  delete WireGeoLines;
  delete WireGeoVertex;
  delete WireGeoFaces;
  delete WireGeoInst;
  delete InstanceGeo;
  delete InstancePlusGeo;
  Skeleton->Release();
}

/****************************************************************************/

template <class streamer> void Wz4Mesh::Serialize_(streamer &s)
{
  sInt version=s.Header(sSerId::Wz4Mesh, 2); version;

  s | Name;
  if(version>=2)
    s | SaveFlags;

  s.Array(Vertices);
  for (sInt i=0; i<Vertices.GetCount(); i++)
  {
    Wz4MeshVertex &v=Vertices[i];
    s | v.Pos | v.Normal; 
    if(!(SaveFlags & 0x0100)) s | v.Tangent | v.BiSign;
    if(!(SaveFlags & 0x0200)) s | v.U0 | v.V0;
    if(!(SaveFlags & 0x0400)) s | v.U1 | v.V1;
#if !WZ4MESH_LOWMEM
    if(!(SaveFlags & 0x0800)) s | v.Color0 | v.Color1;
#else
    sU32 color = 0;
    if(!(SaveFlags & 0x0800)) s | color | color;
#endif
    if(!(SaveFlags & 0x1000))
    {
      for (sInt i=0; i<4; i++) s.S16(v.Index[i]);
      for (sInt i=0; i<4; i++) s | v.Weight[i];
    }
    if (s.IsReading()) v.Select=0.0f;
    s.Check();
  }

  // clear vertices selection in slots
  if (s.IsReading())
  {
    for(sInt slot=0; slot<8; slot++)
      SelVertices[slot].Clear();
  }

  s.Array(Faces);
  for (sInt i=0; i<Faces.GetCount(); i++)
  {
    Wz4MeshFace &f=Faces[i];
    sU32 count = f.Count;
    s | count;
    f.Count = count;
    if (s.IsReading()) f.Init(f.Count);
    for (sInt i=0; i<f.Count; i++) s | f.Vertex[i];
    s | f.Cluster;
    s.Check();
  }
  
  s.ArrayNew(Clusters);
  for (sInt i=0; i<Clusters.GetCount(); i++)
  {
    Wz4MeshCluster &c=*Clusters[i];

    s.ArrayAll(c.Matrices);

    // somewhat complicated version of s.OnceRef() because we need to find out the type
    if(!(SaveFlags & 1))
    {
      if (s.IsReading())
      {
        sRelease(c.Mtrl);
        if (s.If(0))
        {
          switch (s.PeekHeader())
          {
          case sSerId::Wz4SimpleMtrl: c.Mtrl=new SimpleMtrl; break;
          default: sVERIFYFALSE; c.Mtrl=0; break;
          }
          s.RegisterPtr(c.Mtrl);
          s | c.Mtrl;
          s.Ptr(c.Mtrl);
        }
        else
        {
          s.Ptr(c.Mtrl);
          c.Mtrl->AddRef();
        }
        c.Mtrl->Prepare();
      }
      else
      {
        if(s.If(c.Mtrl && !s.IsRegistered(c.Mtrl)))
        {
          s.RegisterPtr(c.Mtrl);
          s | c.Mtrl;
        }
        s.Ptr(c.Mtrl);
      }
    }

    s | c.ChunkStart | c.ChunkEnd;
  }


  s.Array(Chunks);
  for (sInt i=0; i<Chunks.GetCount(); i++)
  {
    Wz4ChunkPhysics &c=Chunks[i];
    s | c.Volume;
    s | c.COM | c.InertD | c.InertOD;
    s | c.FirstVert | c.FirstFace | c.FirstIndex;
    s | c.Normal | c.Random;
  }

  s.OnceRef(Skeleton);

  s.Footer();
}

void Wz4Mesh::Serialize(sWriter &stream) { Serialize_(stream); }
void Wz4Mesh::Serialize(sReader &stream) { Serialize_(stream); }

/****************************************************************************/

void Wz4Mesh::CopyFrom(Wz4Mesh *src)
{
  sVERIFY(IsEmpty());

  Vertices = src->Vertices;
  Faces = src->Faces;

  // copy vertices selection in slots
  for(sInt i=0; i<8; i++)
    SelVertices[i] = src->SelVertices[i];

  CopyClustersFrom(src);

  if(src->Skeleton)
  {
    Skeleton = src->Skeleton;
    Skeleton->AddRef();
  }

  Chunks.Copy(src->Chunks);
}

void Wz4Mesh::CopyClustersFrom(Wz4Mesh *src)
{
  Wz4MeshCluster *cl,*scl;
  sVERIFY(Clusters.GetCount()==0);
  Clusters.HintSize(src->Clusters.GetCount());
  sFORALL(src->Clusters,scl)
  {
    cl = new Wz4MeshCluster;
    cl->Mtrl = scl->Mtrl;
    cl->Mtrl->AddRef();
    Clusters.AddTail(cl);
  }
}

/****************************************************************************/

sBool Wz4Mesh::IsEmpty()
{
  return Clusters.GetCount()==0 && Vertices.GetCount()==0 && Faces.GetCount()==0 && Skeleton==0;
}

/****************************************************************************/

void Wz4Mesh::Flush()
{
  Wz4MeshCluster *cl;
  sDelete(WireGeoLines);
  sDelete(WireGeoVertex);
  sDelete(WireGeoFaces);
  sFORALL(Clusters,cl)
  {
    sDelete(cl->Geo[0]);
    sDelete(cl->Geo[1]);
  }
}

/****************************************************************************/

void Wz4Mesh::Clear()
{
  sDelete(WireGeoLines);
  sDelete(WireGeoVertex);
  sDelete(WireGeoFaces);
  sDeleteAll(Clusters);
  Vertices.Clear();
  Faces.Clear();
}

void Wz4Mesh::ClearClusters()
{
  sDeleteAll(Clusters);
}

/****************************************************************************/

void Wz4Mesh::AddDefaultCluster()
{
  Wz4MeshCluster *cl = new Wz4MeshCluster;
  Clusters.AddTail(cl);
}

/****************************************************************************/

void Wz4Mesh::Add(Wz4Mesh *add)
{
  sInt v0 = Vertices.GetCount();
  sInt f0 = Faces.GetCount();
  sInt c0 = Clusters.GetCount();


  Vertices.HintSize(Vertices.GetCount()+add->Vertices.GetCount());
  Vertices.Add(add->Vertices);
  Faces.HintSize(Faces.GetCount()+add->Faces.GetCount());
  Faces.Add(add->Faces);

  Wz4MeshCluster *cl;
  Clusters.HintSize(add->Clusters.GetCount());
  sFORALL(add->Clusters,cl)
  {
    Wz4MeshCluster *ncl = new Wz4MeshCluster;
    ncl->Mtrl = cl->Mtrl; cl->Mtrl->AddRef();
    Clusters.AddTail(ncl);
  }

  for(sInt i=f0;i<Faces.GetCount();i++)
  {
    Wz4MeshFace *mf = &Faces[i];
    mf->Cluster += c0;
    for(sInt i=0;i<mf->Count;i++)
      mf->Vertex[i] += v0;
  }

  MergeClusters();
}

/****************************************************************************/

void Wz4Mesh::MergeClusters()
{
  Wz4MeshCluster *cl;
  Wz4MeshFace *mf;
  sInt *clusterMap;
  sArray<Wz4MeshCluster *> newcl;

  // go through all faces, searching for referenced clusters and doing the remapping
  // all in one pass.
  sInt nClusters = Clusters.GetCount();
  clusterMap = new sInt[nClusters];
  for(sInt i=0;i<nClusters;i++)
  {
    clusterMap[i] = -1;
    Clusters[i]->Temp = 1; // mark for deletion
  }

  sFORALL(Faces,mf)
  {
    sInt cli = mf->Cluster;
    sInt mapped = clusterMap[cli];

    if(mapped == -1) // haven't seen this cluster yet
    {
      // do we have a cluster for this material?
      Wz4MeshCluster *inCluster = Clusters[cli];
      sFORALL(newcl,cl)
      {
        if(cl->Mtrl == inCluster->Mtrl)
        {
          mapped = clusterMap[cli] = _i;
          break;
        }
      }

      if(mapped == -1) // actually a new cluster
      {
        mapped = clusterMap[cli] = newcl.GetCount();
        newcl.AddTail(inCluster);
        inCluster->Temp = 0; // clear deletion flag
      }
    }

    mf->Cluster = mapped;
  }

  // delete unused clustes
  sDeleteTrue(Clusters,&Wz4MeshCluster::Temp);
//  if(Clusters.GetCount() != nClusters)
//    sDPrintF(L"MergeClusters: %d clusters removed.\n",nClusters - Clusters.GetCount());

  // done

  delete[] clusterMap;
  Clusters = newcl;
  Flush();
}

/****************************************************************************/

void Wz4Mesh::MergeVertices()
{
  sInt max = Vertices.GetCount();
  sHashTable<Wz4MeshVertex,Wz4MeshVertex> hash(1<<sFindLowerPower(max+0x1000),0x1000);
  sInt *map = new sInt[max];    // new = map[old]
  sInt *remap = new sInt[max];  // old = map[new]
 
  // mark used vertices

  Wz4MeshFace *mf;
  Wz4MeshVertex *mv;
  sFORALL(Vertices,mv)
    mv->Temp = 0;
  sFORALL(Faces,mf)
    for(sInt i=0;i<mf->Count;i++)
      Vertices[mf->Vertex[i]].Temp = 1;

  // calc map

  sInt vc = 0;
  for(sInt i=0;i<max;i++)
  {
    if(Vertices[i].Temp)
    {
      Wz4MeshVertex *hit = hash.Find(&Vertices[i]);
      if(hit)
      {
        map[i] = hit->Temp;
      }
      else
      {
        hit = &Vertices[i];
        hit->Temp = vc;
        map[i] = vc;
        remap[vc++] = i;
        hash.Add(hit,hit);
      }
    }
    else
    {
      map[i] = -1;      // should never be used when assigning new faces
    }
  }

  if(0) sDPrintF(L"optimize mesh: %k -> %k vertices\n",max,vc);
  if(vc==max)
  {
    delete[] map;
    delete[] remap;
    return;      // nothing to do
  }

  // remap vertices

  for(sInt i=0;i<vc;i++)
    Vertices[i] = Vertices[remap[i]];
  Vertices.Resize(vc);

  // remap faces

  Wz4MeshFace *face;
  sFORALL(Faces,face)
  {
    for(sInt i=0;i<face->Count;i++)
      face->Vertex[i] = map[face->Vertex[i]];
  }

  // done

  delete[] map;
  delete[] remap;
}

/****************************************************************************/

sBool Wz4Mesh::IsDegenerateFace(sInt face) const
{
  const Wz4MeshFace &fc = Faces[face];

  // simple N^2 loop is fine for our triangles or quads.
  for(sInt j=1;j<fc.Count;j++)
  {
    const Wz4MeshVertex &vj = Vertices[fc.Vertex[j]];

    for(sInt k=0;k<j;k++)
    {
      const Wz4MeshVertex &vk = Vertices[fc.Vertex[k]];
      if(vj.Pos == vk.Pos)
        return sTRUE;
    }
  }

  return sFALSE;
}

void Wz4Mesh::RemoveDegenerateFaces()
{
  sInt nInFaces = Faces.GetCount();
  sInt nOutFaces = 0;

  for(sInt i=0;i<nInFaces;i++)
  {
    if(!IsDegenerateFace(i))
      Faces[nOutFaces++] = Faces[i]; // keep it
  }

  if(nOutFaces != nInFaces)
  {
    sDPrintF(L"RemoveDegenerateFaces: %d/%d faces removed.\n",nInFaces-nOutFaces,nInFaces);
    Faces.Resize(nOutFaces);
  }
}

/****************************************************************************/

void Wz4Mesh::ConvertFrom(class ChaosMesh *src)
{
  // convert faces

  sInt vc = 0;
  sInt max = src->Faces.GetCount();
  Faces.Resize(max);
  for(sInt i=0;i<max;i++)
  {
    Wz4MeshFace *df = &Faces[i];
    ChaosMeshFace *sf = &src->Faces[i];
 
    df->Init(sf->Count);
    df->Cluster = sf->Cluster;
    df->Select = sf->Select;
    for(sInt j=0;j<df->Count;j++)
      df->Vertex[j] = vc++;
  }

  // make fat vertices

  Vertices.Resize(vc);
  sInt vi = 0;
  for(sInt i=0;i<max;i++)
  {
    ChaosMeshFace *sf = &src->Faces[i];
    for(sInt j=0;j<sf->Count;j++)
    {
      Wz4MeshVertex *v = &Vertices[vi];

      v->Init();
      v->Pos = src->Positions[sf->Positions[j]].Position;
      v->Normal = src->Normals[sf->Normals[j]].Normal;
//      v->Tangent = src->Tangents[sf->Tangents[j]].Tangent;   // tangent space from chaosmesh sucks
//      v->BiSign = src->Tangents[sf->Tangents[j]].BiSign;
      v->U0 = src->Properties[sf->Property[j]].U[0];
      v->V0 = src->Properties[sf->Property[j]].V[0];
      v->U1 = src->Properties[sf->Property[j]].U[1];
      v->V1 = src->Properties[sf->Property[j]].V[1];
#if !WZ4MESH_LOWMEM
      v->Color0 = src->Properties[sf->Property[j]].C[0];
      v->Color1 = src->Properties[sf->Property[j]].C[1];
#endif
      v->Select = src->Positions[sf->Positions[j]].Select?1.0f:0.0f;
      for(sInt k=0;k<4;k++)
      {
        v->Index[k] = src->Positions[sf->Positions[j]].MatrixIndex[k];
        v->Weight[k] = src->Positions[sf->Positions[j]].MatrixWeight[k];
      }

      vi++;
    }
  }
  sVERIFY(vi==vc);

  // animation

  if(src->Skeleton && src->Skeleton->Joints.GetCount()>0)
  {
    Skeleton = src->Skeleton;
    Skeleton->AddRef();
  }

  // copy the clusters

  sInt cl = src->Clusters.GetCount();
  sVERIFY(Clusters.GetCount()==0);
  Clusters.Resize(cl);
  for(sInt i=0;i<cl;i++)
  {
    Clusters[i] = new Wz4MeshCluster;
    SimpleMtrl *mtrl = new SimpleMtrl;
    mtrl->SetMtrl(src->Clusters[i]->Material->Material->Flags);
    mtrl->SetTex(0,(Texture2D *) src->Clusters[i]->Material->Tex[1]);
    mtrl->Prepare();
    Clusters[i]->Mtrl = mtrl;
  }

  // optimize

  MergeVertices();
}

/****************************************************************************/

void Wz4Mesh::InsertClusterAfter(Wz4MeshCluster *cl,sInt pos)
{
  Wz4MeshFace *face;

  Clusters.AddAfter(cl,pos);
  sFORALL(Faces,face)
    if(face->Cluster > pos)
      face->Cluster++;
}

void Wz4Mesh::SplitClustersAnim(sInt maxMats)
{
  sInt mc,maxcluster,used,total;
  Wz4AnimJoint *joint;
  Wz4MeshCluster *cl,*clnew;
  Wz4MeshFace *face;
  Wz4MeshVertex *vp;
  sBool found;
  sAABBox box;
  sVector4 plane,pos;
  sVector30 quer;

  if(!Skeleton)
    return;

  Wz4Skeleton *old = Skeleton;
  Skeleton = new Wz4Skeleton;
  Skeleton->CopyFrom(old);
  old->Release();

  // find unused matrices and delete them
  sFORALL(Skeleton->Joints,joint)
    joint->Temp = 0;

  sFORALL(Faces,face)
  {
    for(sInt i=0;i<face->Count;i++)
    {
      for(sInt j=0;j<4;j++)
      {
        sInt mi = Vertices[face->Vertex[i]].Index[j];
        if(mi>=0)
        {
          Skeleton->Joints[mi].Temp = 1;
          for(;;)
          {
            mi = Skeleton->Joints[mi].Parent;
            if(mi==-1)
              break;
            if(Skeleton->Joints[mi].Temp)
              break;
            Skeleton->Joints[mi].Temp = 1;
          }
        }
      }
    }
  }

  mc = 0;
  sFORALL(Skeleton->Joints,joint)
  {
    if(joint->Temp)
      joint->Temp = mc++;
    else
      joint->Temp = -1;
  }

  if (!mc)
  {
    sDPrintF(L"removing whole skeleton\n");
    sRelease(Skeleton);
    return;
  }

  sDPrintF(L"removing %d joints\n",Skeleton->Joints.GetCount()-mc);

  sFORALL(Vertices,vp)
  {
    for(sInt j=0;j<4;j++)
    {
      sInt mi = vp->Index[j];
      if(mi>=0)
      {
        sVERIFY(Skeleton->Joints[mi].Temp>=0);
        vp->Index[j] = Skeleton->Joints[mi].Temp;
      }
    }
  }

  sFORALL(Skeleton->Joints,joint)
  {
    sInt mi = joint->Parent;
    if(mi>=0 && joint->Temp>=0)
    {
      sVERIFY(Skeleton->Joints[mi].Temp>=0);
      joint->Parent = Skeleton->Joints[mi].Temp;
    }
  }

  sArray<Wz4AnimJoint> nj;
  sFORALL(Skeleton->Joints,joint)
    if(joint->Temp>=0)
      nj.AddTail(*joint);
    else
      delete joint->Channel;
  Skeleton->Joints = nj;

  // split too-big-clusters
  do
  {
restartloop:
    maxcluster = Clusters.GetCount();
    found = 0;
    for(sInt ci=0;ci<maxcluster;ci++)
    {
      cl = Clusters[ci];

      sFORALL(Skeleton->Joints,joint)
        joint->Temp = -1;

      mc = 0;
      sFORALL(Faces,face)
      {
        if(face->Cluster==ci)
        {
          for(sInt i=0;i<face->Count;i++)
          {
            for(sInt j=0;j<4;j++)
            {
              sInt mi = Vertices[face->Vertex[i]].Index[j];
              if(mi!=-1)
              {
                if(Skeleton->Joints[mi].Temp==-1)
                  Skeleton->Joints[mi].Temp = mc++;
              }
            }
          }
        }
      }

      if(mc>=maxMats)
      {
        found = 1;

        // find bounding box

        box.Clear();
        sFORALL(Faces,face)
          if(face->Cluster==ci)
            for(sInt i=0;i<face->Count;i++)
              box.Add(Vertices[face->Vertex[i]].Pos);

        // figure split plane

        quer = box.Max-box.Min;
        if(quer.x>quer.y && quer.x>quer.z)
          plane.Init(1,0,0,-(box.Min.x + box.Max.x)*0.5f);
        else if(quer.y > quer.z)
          plane.Init(0,1,0,-(box.Min.y + box.Max.y)*0.5f);
        else
          plane.Init(0,0,1,-(box.Min.z + box.Max.z)*0.5f);

        // create new cluster

        clnew = new Wz4MeshCluster;
        clnew->Mtrl = cl->Mtrl;
        clnew->Mtrl->AddRef();
        InsertClusterAfter(clnew,ci);

        // split by plane

        used = 0;
        total = 0;
        sFORALL(Faces,face)
        {
          if(face->Cluster==ci)
          {
            pos.Init(0,0,0,0);
            for(sInt i=0;i<face->Count;i++)
              pos = pos + Vertices[face->Vertex[i]].Pos;
            if((plane ^ pos) > 0)
            {
              face->Cluster++;
              used++;
            }
            total++;
          }
        }
        sDPrintF(L"split %d/%d -> %f\n",used,total,plane);

        if(used==0)
        {
          sDPrintF(L"could not split cluster!\n");
          return;
        }

        goto restartloop;
      }
    }
  }
  while(found);
}

void Wz4Mesh::SplitClustersChunked(sInt maxMats)
{
  sInt ccount = Clusters.GetCount();
  sInt fcount = Faces.GetCount();

  // build linked list of faces in each cluster
  sArray<sInt> firstFaceInCluster;
  firstFaceInCluster.AddMany(ccount);
  sInt *nextFaceInCluster = new sInt[fcount];

  for(sInt i=0;i<ccount;i++)
    firstFaceInCluster[i] = -1;

  for(sInt i=fcount-1;i>=0;i--)
  {
    sInt cl = Faces[i].Cluster;
    sVERIFY(cl >= 0 && cl < ccount);
    nextFaceInCluster[i] = firstFaceInCluster[cl];
    firstFaceInCluster[cl] = i;
  }

  // process all clusters
  for(sInt ci=0;ci<Clusters.GetCount();ci++)
  {
    sInt firstMatIndex = -1;
    sInt prevMatIndex = 0;
    sInt fc;

    // go through faces, stop when reaching matrix limit
    sInt oldface = -1;
    for(fc=firstFaceInCluster[ci];fc>=0;fc=nextFaceInCluster[fc])
    {
      Faces[fc].Cluster = ci;

      sInt mi = Vertices[Faces[fc].Vertex[0]].Index[0];
      sVERIFY(mi >= prevMatIndex);
      prevMatIndex = mi;
      if(firstMatIndex == -1)
        firstMatIndex = mi;

      if(mi - firstMatIndex >= maxMats)
        break;
      oldface = fc;
    }

    // split if that wasn't the whole cluster
    if(fc >= 0)
    {
      sVERIFY(oldface>=0);      // should be at least one face processed!
      // add a new cluster
      Wz4MeshCluster *clnew = new Wz4MeshCluster;
      clnew->Mtrl = Clusters[ci]->Mtrl;
      clnew->Mtrl->AddRef();
      Clusters.AddAfter(clnew,ci);

      // terminate linked list for start, add remaining
      // faces to new cluster

      nextFaceInCluster[oldface] = -1;
      firstFaceInCluster.AddAfter(fc,ci);
    }
  }

  delete[] nextFaceInCluster;

//  sDPrintF(L"\n%d clusters after split\n",Clusters.GetCount());
}

void Wz4Mesh::SortClustersByLocalRenderPass()
{
  for(sInt i=0;i<Clusters.GetCount();i++)
    Clusters[i]->Temp = i;

  // sort clusters
  sIntroSort(sAll(Clusters),sMemPtrLess(&Wz4MeshCluster::LocalRenderPass));

  // build remap table
  sFixedArray<sInt> remap(Clusters.GetCount());
  for(sInt i=0;i<Clusters.GetCount();i++)
    remap[Clusters[i]->Temp] = i;

  // remap faces
  for(sInt i=0;i<Faces.GetCount();i++)
    Faces[i].Cluster = remap[Faces[i].Cluster];
}

/****************************************************************************/
/***                                                                      ***/
/***   Connectivity                                                       ***/
/***                                                                      ***/
/****************************************************************************/

sInt Wz4Mesh::GetTriCount()
{
  sInt ic = 0;
  Wz4MeshFace *mf;
  sFORALL(Faces,mf)
    ic += mf->Count-2;
  return ic;
}

sInt *Wz4Mesh::BasePos(sInt toitself)
{
  sInt *map = new sInt[Vertices.GetCount()];

  const sInt HashSize = sMax(4096,1<<sFindLowerPower(Vertices.GetCount()/2));
  sInt *HashMap = new sInt[HashSize];
  for(sInt i=0;i<HashSize;i++)
    HashMap[i] = -1;

  Wz4MeshVertex *vp;
  sFORALL(Vertices,vp)
  {
    sU32 hash = sChecksumMurMur((sU32 *)&vp->Pos.x,3) & (HashSize-1);
    sInt index = HashMap[hash];
    while(index>=0)
    {
      if(Vertices[index].Pos==vp->Pos)
      {
        map[_i] = index;
        goto done;
      }
      index = Vertices[index].Temp;
    }
    vp->Temp = HashMap[hash];
    HashMap[hash] = _i;
    map[_i] = toitself ? _i : -1;
done:;
  }

  delete[] HashMap;
  return map;
}

/****************************************************************************/

struct Wz4MeshTempEdge
{
  sInt Tag;
  sInt v0,v1;     // v0 < v1!

  bool operator < (const Wz4MeshTempEdge &b) const
  {
    return v0 < b.v0 || v0 == b.v0 && v1 < b.v1;
  }
};

static void ConnectFaces(Wz4MeshFaceConnect *conn,const sInt *buf,sInt count)
{
  sVERIFY(count <= 2);

  if(count == 1) // boundary
    conn[*buf >> 2].Adjacent[*buf & 3] = -1;
  else if(count == 2) // inner (shared) edge
  {
    sInt f0 = buf[0], f1 = buf[1];
    conn[f0 >> 2].Adjacent[f0 & 3] = f1;
    conn[f1 >> 2].Adjacent[f1 & 3] = f0;
  }
}

Wz4MeshFaceConnect *Wz4Mesh::Adjacency()
{
  sInt *remap = BasePos();
  Wz4MeshFaceConnect *conn = new Wz4MeshFaceConnect[Faces.GetCount()];
  Wz4MeshFace *face;

  for(sInt i=0;i<Vertices.GetCount();i++)
    if(remap[i]==-1)
      remap[i]=i;

  // calculate number of edges we need
  sInt numEdges = 0;
  sFORALL(Faces,face)
    numEdges += face->Count;

  // make edge list
  sFixedArray<Wz4MeshTempEdge> edges(numEdges);
  sInt edgeCtr = 0;

  sFORALL(Faces,face)
  {
    for(sInt j=0;j<face->Count;j++)
    {
      edges[edgeCtr].Tag = OutgoingEdge(_i,j);
      edges[edgeCtr].v0 = remap[face->Vertex[j]];
      edges[edgeCtr].v1 = remap[face->Vertex[(j + 1 == face->Count) ? 0 : j+1]];
      if(edges[edgeCtr].v0 > edges[edgeCtr].v1)
        sSwap(edges[edgeCtr].v0,edges[edgeCtr].v1);

      edgeCtr++;
    }
  }

  sIntroSort(sAll(edges));

  // generate adjacency
  sInt last0 = -1, last1 = -1, count = 0, temp[2];
  Wz4MeshTempEdge *edge;
  sFORALL(edges,edge)
  {
    if(last0 == edge->v0 && last1 == edge->v1)
    {
      temp[count++] = edge->Tag;
      if(count == 2) // got a complete edge
      {
        ConnectFaces(conn,temp,count);
        count = 0;
      }
    }
    else
    {
      ConnectFaces(conn,temp,count);

      temp[0] = edge->Tag;
      count = 1;
    }

    last0 = edge->v0;
    last1 = edge->v1;
  }

  ConnectFaces(conn,temp,count);

  // cleanup
  delete[] remap;
  return conn;
}

/****************************************************************************/

sInt *Wz4Mesh::BaseNormal()
{
  sInt *map = new sInt[Vertices.GetCount()];

  const sInt HashSize = sMax(4096,1<<sFindLowerPower(Vertices.GetCount()/2));
  sInt *HashMap = new sInt[HashSize];
  for(sInt i=0;i<HashSize;i++)
    HashMap[i] = -1;

  Wz4MeshVertex *vp;
  sVERIFYSTATIC(sOFFSET(Wz4MeshVertex,Pos)==0);
  sVERIFYSTATIC(sOFFSET(Wz4MeshVertex,Normal)==12);
  sVERIFY(sizeof(vp->Pos)+sizeof(vp->Normal)==24);
  sFORALL(Vertices,vp)
  {
    sU32 hash = sChecksumMurMur((sU32 *)&vp->Pos.x,6) & (HashSize-1);
    sInt index = HashMap[hash];
    while(index>=0)
    {
      if(Vertices[index].Normal==vp->Normal && 
         Vertices[index].Pos==vp->Pos)
      {
        map[_i] = index;
        goto done;
      }
      index = Vertices[index].Temp;
    }
    vp->Temp = HashMap[hash];
    HashMap[hash] = _i;
    map[_i] = -1;
done:;
  }
  delete[] HashMap;

  return map;
}

/****************************************************************************/

void Wz4Mesh::CalcTangents()
{
  sInt *map = BaseNormal();
  CalcTangents(map);
  delete[] map;
}

void Wz4Mesh::CalcNormals()
{
  sInt *map = BaseNormal();
  CalcNormals(map);
  delete[] map;
}

void Wz4Mesh::CalcNormalAndTangents(sBool forceSmooth, sBool onlyselected)
{
  sInt *map = forceSmooth ? BasePos() : BaseNormal();
  CalcNormals(map, onlyselected);
  CalcTangents(map, onlyselected);
  delete[] map;
}

/****************************************************************************/

void Wz4Mesh::CalcNormals(sInt *map, sBool onlyselected)
{
  sVector30 *facenormal = new sVector30[Faces.GetCount()];
  Wz4MeshFace *fp;
  Wz4MeshVertex *vp;

  sVector30 *oldnormals=0;
  if (onlyselected) oldnormals = new sVector30[Vertices.GetCount()];

  sFORALL(Vertices,vp)
  {
    if (onlyselected) oldnormals[_i]=vp->Normal;
    vp->Normal.Init(0,0,0);
  }

  sFORALL(Faces,fp)
  {
    sVector30 n(0.0f);
    for(sInt i=0;i<fp->Count;i++)
    {
      sVector31 v0 = Vertices[fp->Vertex[(i+0)%fp->Count]].Pos;
      sVector31 v1 = Vertices[fp->Vertex[(i+1)%fp->Count]].Pos;
      sVector31 v2 = Vertices[fp->Vertex[(i+2)%fp->Count]].Pos;
      sVector30 d0 = v0-v1;
      sVector30 d1 = v1-v2;
      sVector30 nn; nn.Cross(d0,d1);
      //sF32 l = nn.LengthSq();
      //if(l)
      //  nn *= sFRSqrt(l);
      n+=nn;
    }

    sF32 l = n.LengthSq();
    if(l)
      n *= sFRSqrt(l);

    //n.Unit();
    facenormal[_i] = n;

    for(sInt i=0;i<fp->Count;i++)
    {
      sInt index = fp->Vertex[i];
      if(map[index]!=-1)
        index = map[index];
      Vertices[index].Normal += n;
    }
  }
  
  sFORALL(Vertices,vp)
  {
    if(map[_i]==-1)
      vp->Normal.Unit();
    else
      vp->Normal = Vertices[map[_i]].Normal;

    if (onlyselected && vp->Select<1.0f)
    {
      if (vp->Select>0.0f)
      {
        vp->Normal=sFade(vp->Select,oldnormals[_i],vp->Normal);
        vp->Normal.Unit();
      }
      else
        vp->Normal=oldnormals[_i];
    }

  }

  delete[] oldnormals;
  delete[] facenormal;
}

/****************************************************************************/



void Wz4Mesh::CalcTangents(sInt *map, sBool onlyselected)
{
  Wz4MeshFace *mf;
  Wz4MeshVertex *mv;

  // calc tangent space
  sVector30 *oldtangents=0;
  if (onlyselected) oldtangents = new sVector30[Vertices.GetCount()];

  sFORALL(Vertices,mv)
  {
    if (onlyselected)
      oldtangents[_i]=mv->Tangent;
    else
      mv->BiSign = 1;

    mv->Tangent.Init(0,0,0);
  }

  sFORALL(Faces,mf)
  {
    sVector30 t,dp;
    sF32 du;

    for(sInt i=0;i<mf->Count;i++)
    {
      sInt i0 = i;
      sInt i1 = (i0+1)%mf->Count;
      if(map[i0]!=-1) i0=map[i0];
      if(map[i1]!=-1) i0=map[i1];

      dp = Vertices[mf->Vertex[i0]].Pos - Vertices[mf->Vertex[i1]].Pos;
      du = Vertices[mf->Vertex[i0]].U0  - Vertices[mf->Vertex[i1]].U0;

      t = dp * (du/(dp^dp));
      Vertices[mf->Vertex[i0]].Tangent += t;
      Vertices[mf->Vertex[i1]].Tangent += t;
    }
  }

  sFORALL(Vertices,mv)
  {
    if(onlyselected && mv->Select<0.5f)
      mv->Tangent=oldtangents[_i];
    else if(map[_i]==-1) 
      mv->Tangent.Unit();
    else
      mv->Tangent = Vertices[map[_i]].Tangent;
  }

  delete[] oldtangents;
}

/****************************************************************************/

sInt Wz4Mesh::GetFaceInd(sInt edgeTag) const
{
  return edgeTag >> 2;
}

sInt Wz4Mesh::GetVertInd(sInt edgeTag) const
{
  return edgeTag & 3;
}

const Wz4MeshFace &Wz4Mesh::FaceFromEdge(sInt edgeTag) const
{
  return Faces[edgeTag >> 2];
}

Wz4MeshFace &Wz4Mesh::FaceFromEdge(sInt edgeTag)
{
  return Faces[edgeTag >> 2];
}

const Wz4MeshVertex &Wz4Mesh::StartVertFromEdge(sInt edgeTag) const
{
  return Vertices[StartVertex(edgeTag)];
}

Wz4MeshVertex &Wz4Mesh::StartVertFromEdge(sInt edgeTag)
{
  return Vertices[StartVertex(edgeTag)];
}

sInt Wz4Mesh::OutgoingEdge(sInt face,sInt vert) const
{
  return face*4 + vert;
}

sInt Wz4Mesh::IncomingEdge(sInt face,sInt vert) const
{
  const Wz4MeshFace &f = Faces[face];
  return face*4 + (vert + f.Count-1) % f.Count;
}

sInt Wz4Mesh::NextFaceEdge(sInt edgeTag) const
{
  const Wz4MeshFace &f = FaceFromEdge(edgeTag);
  return (edgeTag & -4) + ((edgeTag&3) + 1) % f.Count;
}

sInt Wz4Mesh::PrevFaceEdge(sInt edgeTag) const
{
  const Wz4MeshFace &f = FaceFromEdge(edgeTag);
  return (edgeTag & -4) + ((edgeTag&3) + f.Count-1) % f.Count;
}

sInt Wz4Mesh::StartVertex(sInt edgeTag) const
{
  const Wz4MeshFace &f = FaceFromEdge(edgeTag);
  return f.Vertex[edgeTag & 3];
}

sInt Wz4Mesh::EndVertex(sInt edgeTag) const
{
  return StartVertex(NextFaceEdge(edgeTag));
}

sInt Wz4Mesh::OppositeEdge(const Wz4MeshFaceConnect *conn,sInt edgeTag) const
{
  return conn[edgeTag>>2].Adjacent[edgeTag&3];
}

sInt Wz4Mesh::NextVertEdge(const Wz4MeshFaceConnect *conn,sInt edgeTag) const
{
  return OppositeEdge(conn,PrevFaceEdge(edgeTag));
}

sInt Wz4Mesh::PrevVertEdge(const Wz4MeshFaceConnect *conn,sInt edgeTag) const
{
  sInt o = OppositeEdge(conn,edgeTag);
  return (o >= 0) ? NextFaceEdge(o) : o;
}

void Wz4Mesh::SetOpposite(Wz4MeshFaceConnect *conn,sInt edge,sInt opposite)
{
  if(edge >= 0)
    conn[edge >> 2].Adjacent[edge & 3] = opposite;
}

void Wz4Mesh::EdgeFlip(Wz4MeshFaceConnect *conn,sInt e0)
{
  sInt e1 = OppositeEdge(conn,e0);
  sVERIFY(e0 >= 0 && e1 >= 0); // may only flip internal edges

  const sInt f0i = GetFaceInd(e0);
  const sInt f1i = GetFaceInd(e1);
  const sInt v0i = GetVertInd(e0);
  const sInt v1i = GetVertInd(e1);
  const sInt v0n = (v0i + 1) % 3; // next after v0
  const sInt v1n = (v1i + 1) % 3; // next after v1
  const sInt v0p = (v0i + 2) % 3; // prev before v0
  const sInt v1p = (v1i + 2) % 3; // prev before v1

  Wz4MeshFace &f0 = Faces[f0i];
  Wz4MeshFace &f1 = Faces[f1i];
  Wz4MeshFaceConnect &fc0 = conn[f0i];
  Wz4MeshFaceConnect &fc1 = conn[f1i];
  sVERIFY(f0.Count == 3 && f1.Count == 3); // may only flip triangles

  // get vertices
  sInt va = f0.Vertex[v0i];
  sInt vb = f1.Vertex[v1p];
  sInt vc = f1.Vertex[v1i];
  sInt vd = f0.Vertex[v0p];

  // outer edges
  sInt oab = fc1.Adjacent[v1n];
  sInt obc = fc1.Adjacent[v1p];
  sInt ocd = fc0.Adjacent[v0n];
  sInt oda = fc0.Adjacent[v0p];

  // generate flipped triangles
  f0.Vertex[0] = vb;
  f0.Vertex[1] = vd;
  f0.Vertex[2] = va;
  f1.Vertex[0] = vd;
  f1.Vertex[1] = vb;
  f1.Vertex[2] = vc;

  // update inner adjacency
  fc0.Adjacent[0] = OutgoingEdge(f1i,0);
  fc0.Adjacent[1] = oda;
  fc0.Adjacent[2] = oab;
  fc1.Adjacent[0] = OutgoingEdge(f0i,0);
  fc1.Adjacent[1] = obc;
  fc1.Adjacent[2] = ocd;

  // update outer adjacency
  SetOpposite(conn,oab,OutgoingEdge(f0i,2));
  SetOpposite(conn,obc,OutgoingEdge(f1i,1));
  SetOpposite(conn,ocd,OutgoingEdge(f1i,2));
  SetOpposite(conn,oda,OutgoingEdge(f0i,1));
}

/****************************************************************************/
/***                                                                      ***/
/***   transforms                                                         ***/
/***                                                                      ***/
/****************************************************************************/

void Wz4Mesh::Transform(const sMatrix34 &mat)
{
  sMatrix34 matInvT;

  matInvT = mat;
  matInvT.Invert3();
  matInvT.Trans3();

  Wz4MeshVertex *mv;
  sFORALL(Vertices,mv)
    mv->Transform(mat,matInvT);

  if(mat.Determinant3x3() < 0.0f) // flips orientation
  {
    Wz4MeshFace *f;
    sFORALL(Faces,f)
      f->Invert();

    Wz4MeshVertex *v;
    sFORALL(Vertices,v)
      v->Normal.Neg();
  }
}

void Wz4Mesh::TransformUV(const sMatrix34 &mat)
{
  sVector31 uv;
  Wz4MeshVertex *mv;
  sFORALL(Vertices,mv)
  {
    uv.Init(mv->U0,mv->V0,0);
    uv = uv * mat;
    mv->U0 = uv.x;
    mv->V0 = uv.y;
  }
}

/****************************************************************************/

void Wz4Mesh::BakeAnim(sF32 time)
{
  sMatrix34 *bonemat,*basemat;
  Wz4MeshVertex *v;

  sInt max = Skeleton->Joints.GetCount();
  bonemat = new sMatrix34[max];
  basemat = new sMatrix34[max];
  Skeleton->Evaluate(time,bonemat,basemat);
  
  sFORALL(Vertices,v)
  {
    v->Skin(basemat,max,v->Pos);
    for(sInt i=0;i<4;i++)
    {
      v->Index[i] = -1;
      v->Weight[i] = 0;
    }
  }

  Skeleton->Release();
  Skeleton = 0;

  delete[] bonemat;
  delete[] basemat;
}

/****************************************************************************/

void Wz4Mesh::Displace(const sMatrix34 &mati,sImageI16 *img,sF32 amount,sF32 bias,sInt flags,sInt sel)
{
  sMatrix34 mat;
  sVector31 p;

  mat = mati;
  if(flags &1)
  {
    mat.l -= mat.i*0.5f;
    mat.l -= mat.j*0.5f;
  }

  mat.InvertOrthogonal();
  Wz4MeshVertex *v;
  sInt sx = img->SizeX*256;
  sInt sy = img->SizeY*256;
  sInt x,y;
  sVector30 proj = mat.k;
  proj.Unit();
  sFORALL(Vertices,v)
  {
    if(logic(sel,v->Select))
    {
      if(flags & 2)
      {
        p.Init(v->U0,v->V0,0);
        p = p*mat;

        x = sInt(p.x*sx)&(sx-1);
        y = sInt(p.y*sy)&(sy-1);
      }
      else
      {
        p = v->Pos;
        p = p*mat;

        x = sClamp(sInt(p.x*sx),0,sx-1);
        y = sClamp(sInt(p.y*sy),0,sy-1);
      }

      sF32 a = amount * (img->Filter(x,y)/65535.0f - bias);

      if(flags & 4)
        v->Pos += v->Normal * a;
      else
        v->Pos += proj * a;
    }
  }

  CalcNormalAndTangents();
}

/****************************************************************************/

void BendMatrices(sInt count,const BendKey *bk,sMatrix34 *mats,const sVector30 &up,sInt um)
{
  sVector30 oldup = up;

  for(sInt i=0;i<count;i++)
  {
    mats[i].j = bk[i].Dir;
    mats[i].j.Unit();
    mats[i].i = oldup;
    mats[i].k.Cross(mats[i].i,mats[i].j);
    mats[i].k.Unit();
    if(um!=2)
    {
      mats[i].i.Cross(mats[i].j,mats[i].k);
      if(um==0)
        oldup = mats[i].i;
    }
  }

  // twist, scale and position

  for(sInt i=0;i<count;i++)
  {
    if(bk[i].Twist!=0)
    {
      sMatrix34 matr,mat0;
      mat0 = mats[i];
      matr.RotateAxis(mats[i].j,bk[i].Twist*sPI2F);
      mats[i] = mat0*matr;
    }

    mats[i].i *= bk[i].Scale;
    mats[i].j *= bk[i].Scale;
    mats[i].k *= bk[i].Scale;

    mats[i].l = bk[i].Pos;
  }
}


sBool Wz4Mesh::Deform(sInt count,const sVector31 &p0,const sVector31 &p1,sInt keycount,const Wz4MeshArrayDeform *keys,sInt selflags,sInt upflags,const sVector30 &up)
{
  if(Chunks.GetCount()) return 0;
  if(count<2) return 0;
  Wz4Skeleton *old = 0;

  if(selflags == 0)      // all
  {
    Skeleton = new Wz4Skeleton;
  }
  else
  {
    if(!Skeleton)
    {
      Skeleton = new Wz4Skeleton;

      Wz4AnimJoint *joints = Skeleton->Joints.AddMany(1);
      Wz4ChannelConstant *c = new Wz4ChannelConstant;
      joints->Init();
      joints->Channel = c;
      Wz4MeshVertex *v;

      sFORALL(Vertices,v)
      {
        v->Index[0] = 0;
        v->Weight[0] = 1;
      }
    }
    else
    {
      old = Skeleton;
      Skeleton = new Wz4Skeleton;
      Skeleton->CopyFrom(old);
    }
  }

  sInt jstart = Skeleton->Joints.GetCount();
  Wz4AnimJoint *joints = Skeleton->Joints.AddMany(count);
  BendKey *bk = new BendKey[count];
  sMatrix34 *mats = new sMatrix34[count];

  for(sInt i=0;i<count;i++)
  {
    bk[i].Time = 0;
    if(i<keycount)
    {
      bk[i].Pos = keys[i].Pos;
      bk[i].Scale = keys[i].Scale;
      bk[i].Twist = keys[i].Twist;
    }
    else
    {
      bk[i].Pos.Fade(sF32(i)/(count-1),p0,p1);
      bk[i].Scale = 1;
      bk[i].Twist = 0;
    }
  }

  if(upflags & 16)
    bk[0].Dir = p1-p0;
  else
    bk[0].Dir = bk[1].Pos-bk[0].Pos;
  bk[count-1].Dir = bk[count-1].Pos-bk[count-2].Pos;
  for(sInt i=1;i<count-1;i++)
    bk[i].Dir = bk[i+1].Pos-bk[i-1].Pos;

  sMatrix34 mat;

  if(!(upflags & 256))
  {
    BendMatrices(count,bk,mats,up,upflags&15);
  }

  mat.j = p1-p0;
  mat.j.Unit();
  mat.k.Init(0,0,1);
  mat.i.Cross(mat.j,mat.k);
  mat.i.Unit();
  mat.k.Cross(mat.i,mat.j);

  if((upflags & 256))
  {
    for(sInt i=0;i<count;i++)
    {
      if(i<keycount)
        mats[i].l = keys[i].Pos;
      else
        mats[i].l.Fade(sF32(i)/(count-1),p0,p1);
    }

    mats[0].j = mats[1].l-mats[0].l;
    mats[0].j.Unit();
    mats[0].k = mat.k;
    mats[0].i.Cross(mats[0].j,mats[0].k);
    mats[0].i.Unit();
    mats[0].k.Cross(mats[0].i,mats[0].j);

    for(sInt i=1;i<count-1;i++)
    {
      mats[i].j = mats[i+1].l - mats[i-1].l;
      mats[i].j.Unit();
      mats[i].k = mats[i-1].k;
      mats[i].i.Cross(mats[i].j,mats[i].k);
      mats[i].i.Unit();
      mats[i].k.Cross(mats[i].i,mats[i].j);
    }

    mats[count-1].j = mats[count-1].l-mats[count-2].l;
    mats[count-1].j.Unit();
    mats[count-1].k = mats[count-2].k;
    mats[count-1].i.Cross(mats[count-1].j,mats[count-1].k);
    mats[count-1].i.Unit();
    mats[count-1].k.Cross(mats[count-1].i,mats[count-1].j);
  }

  for(sInt i=0;i<count;i++)
  {
    Wz4ChannelConstant *c = new Wz4ChannelConstant;
    joints[i].Init();
    joints[i].Channel = c;
    joints[i].BasePose = mat;
    joints[i].BasePose.l.Fade(sF32(i)/(count-1),p0,p1);
    joints[i].BasePose.InvertOrthogonal();
    c->Start.Trans = mats[i].l;
    c->Start.Rot.Init(mats[i]);
  }

  delete[] mats;
  delete[] bk;

  Wz4MeshVertex *v;
  sVector4 plane;
  sVector30 n = p1-p0;
  sF32 dist = n.Length();
  n.Unit();
  plane.InitPlane(p0,n);
  
  sFORALL(Vertices,v)
  {
    if(logic(selflags,v->Select))
    {
      sF32 d = (plane^v->Pos)/dist;
      if(d<=0.0f)
      {
        v->Index[0] = jstart + 0;
        v->Weight[0] = 1;
      }
      else if(d>=1.0f)
      {
        v->Index[0] = jstart + count-1;
        v->Weight[0] = 1;
      }
      else
      {
        d *= (count-1);
        sInt n = sInt(d);
        sF32 f = sClamp(d-n,0.0f,1.0f);

        if(n>=0 && n<count-1)
        {
          v->Index[0] = jstart + sClamp(n-1,0,count-1);
          v->Index[1] = jstart + n;
          v->Index[2] = jstart + n+1;
          v->Index[3] = jstart + sClamp(n+2,0,count-1);
          sF32 sa =  2*f*f*f - 3*f*f     + 1;   // p0
          sF32 sb =    f*f*f - 2*f*f + f;       // m0
          sF32 sc =    f*f*f -   f*f;           // m1
          sF32 sd = -2*f*f*f + 3*f*f;           // p1
          v->Weight[0] =   -0.5f*sb;
          v->Weight[1] = sa-0.5f*sc;
          v->Weight[2] = sd+0.5f*sb;
          v->Weight[3] =    0.5f*sc;
        }
      }
    }
  }

  old->Release();
  return 1;
}

void Wz4Mesh::ExtrudeNormal(sInt logic_,sF32 amount)
{
  Wz4MeshVertex *mv;

  sInt max= Vertices.GetCount();
  sInt *map = BasePos(1);
  sVector30 *accu = new sVector30[max];

  for(sInt i=0;i<max;i++)
    accu[i].Init(0,0,0);

  sFORALL(Vertices,mv)
    if(logic(logic_,mv->Select))
      accu[map[_i]] += mv->Normal;

  for(sInt i=0;i<max;i++)
    accu[i].Unit();

  sFORALL(Vertices,mv)
    mv->Pos += accu[map[_i]]*amount;

  delete[] accu;
  delete[] map;

  CalcNormalAndTangents();
}


void Wz4Mesh::Heal(Wz4Mesh *reference, sInt flags, sF32 posThres, sF32 normThres)
{
  posThres*=posThres;
  Wz4MeshVertex *v, *vref;

  sFORALL(Vertices,v)
  {
    // yep, n². Sorry.
    sFORALL(reference->Vertices, vref)
    {
      if ((v->Pos-vref->Pos).LengthSq()>posThres) continue;
      if ((v->Normal^vref->Normal)<normThres) continue;

      if (flags&1) v->Normal=vref->Normal;
      if (flags&2) v->Tangent=vref->Tangent;
      if (flags&4) { v->U0=vref->U0; v->V0=vref->V0; v->U1=vref->U1; v->V1=vref->V1; }
      break;
    }
  }

}

/****************************************************************************/
/***                                                                      ***/
/***   topology                                                           ***/
/***                                                                      ***/
/****************************************************************************/

void Wz4Mesh::Facette(sF32 smooth)
{
  Wz4MeshFace *face;
  Wz4MeshVertex *vp;

  if(smooth>=0.99999f)            // we want a perfectly smooth mesh.
  {
    sFORALL(Vertices,vp)          // make all normals the same
    {
      vp->Normal.Init(0,0,0);
      vp->Tangent.Init(0,0,0);
    }
    MergeVertices();              // remove double vertices, this merges the normals
    CalcNormalAndTangents();      // now calculate the normals and tangents
  }
  else                            // we want a facetted mesh
  {
    sInt vc = 0;                  // first we have to split all faces
    sFORALL(Faces,face)
      vc += face->Count;

    Wz4MeshVertex *nv = new Wz4MeshVertex[vc];
    sInt nc = 0;
    sFORALL(Faces,face)
    {
      for(sInt i=0;i<face->Count;i++)
      {
        nv[nc] = Vertices[face->Vertex[i]];
        face->Vertex[i] = nc;
        nc++;
      }
    }
    sVERIFY(nc==vc);

    Vertices.Resize(nc);
    sCopyMem(Vertices.GetData(),nv,sizeof(Wz4MeshVertex)*nc);
    delete[] nv;

    sInt *map = BasePos();        // now we calculate the smooth normals, and store them in the tangents
    CalcNormals(map);
    sFORALL(Vertices,vp)
    {
      vp->Tangent = vp->Normal;
      vp->Normal.x = _i;
    }
    delete[] map;

    map = BaseNormal();           // now we calculate facetted vertices
    CalcNormals(map);

    sFORALL(Vertices,vp)          // fade from facetted to smooth
      vp->Normal.Fade(smooth,vp->Normal,vp->Tangent);

    CalcTangents(map);            // fix the tangents we destroyed
    delete[] map;
  }
}

/****************************************************************************/

void Wz4Mesh::Crease()
{
  Wz4MeshFaceConnect *adj = Adjacency();

  Wz4MeshVertex *v;
  Wz4MeshFace *f0,*f1;

  sFORALL(Vertices,v)
    v->Temp = -1;

  sFORALL(Faces,f0)
  {
    if(f0->Select>=0.5f)
    {
      for(sInt j=0;j<f0->Count;j++)
      {
        sInt f1i = adj[_i].Adjacent[j];
        if(f1i>=0)
        {
          f1 = &Faces[f1i/4];
          if(f1->Select<0.5f)
          {
            sInt vi = f0->Vertex[j];
            sInt n = Vertices[vi].Temp;
            if(n==-1)
            {
              n = Vertices.GetCount();
              Vertices[vi].Temp = n;
              Wz4MeshVertex *v = Vertices.AddMany(1);
              v->CopyFrom(&Vertices[vi]);
              v->Normal.Init(0,0,0);
            }
            f0->Vertex[j] = n;

            vi = f0->Vertex[(j+1)%f0->Count];
            n = Vertices[vi].Temp;
            if(n==-1)
            {
              n = Vertices.GetCount();
              Vertices[vi].Temp = n;
              Wz4MeshVertex *v = Vertices.AddMany(1);
              v->CopyFrom(&Vertices[vi]);
              v->Normal.Init(0,0,0);
            }
            f0->Vertex[(j+1)%f0->Count] = n;
          }
        }
      }
    }
  }

  CalcNormalAndTangents();
  delete[]adj;
}

void Wz4Mesh::Uncrease(sInt select)
{
  Wz4MeshFace *f;

  sFORALL(Faces,f)
  {
    if(logic(select,f->Select))
    {
      for(sInt j=0;j<f->Count;j++)
        Vertices[f->Vertex[j]].Normal.Init(0,0,0);
    }
  }

  CalcNormalAndTangents();
}

/****************************************************************************/

struct EdgeInfo
{
  sInt FaceA;                   // Face.Vertex[Face0i+0] .. Face.Vertex[Face0i+1]
  sInt FaceAI;
  sInt FaceB;                   // Face.Vertex[Face0i+1] .. Face.Vertex[Face0i+0]
  sInt FaceBI;
  sInt VertA0;
  sInt VertA1;
  sInt VertB0;
  sInt VertB1;
  sInt Vert0;                   // lower vert
  sInt Vert1;                   // higher vert
  sInt SplitVertA;              // -1 or new vertex for split
  sInt SplitVertB;
  sInt Subdivide;               // all faces of this edge need subdivision.
  sInt Hash;                    // hash value
  sInt HashNext;                // next in hashtable, -1 = end
};
struct VertInfo
{
  sVector30 Face;
  sVector30 Edge;
  sInt FaceCount;
  sInt EdgeCount;
  sInt Subdivide;
};

void Wz4Mesh::Subdivide(sF32 smooth)
{
  Facette(1);



  Wz4MeshFace *f;
  Wz4MeshVertex *v;
  EdgeInfo *e;
  sInt max = 0;

  sInt maxface = Faces.GetCount();
  sInt maxvert = Vertices.GetCount();
  sArray<EdgeInfo> edges;
  edges.HintSize(maxface*4);
  EdgeInfo **edgelink = new EdgeInfo*[maxface*4];
  sInt *centervert = new sInt[maxface];
  VertInfo *vertinfo = new VertInfo[maxvert];
  sVector30 *centerpos = new sVector30[maxface];

  // get same position map

  sInt *map = BasePos();
  for(sInt i=0;i<maxvert;i++)
    if(map[i]==-1)
      map[i] = i;

  // build edge information.

  sInt hashmax = 1<<(sFindLowerPower(maxface)/2);
    
  sInt *hashtable = new sInt[hashmax];
  for(sInt i=0;i<hashmax;i++)
    hashtable[i] = -1;

  sFORALL(Faces,f)
  {
    sInt i = _i;
    for(sInt j=0;j<f->Count;j++)
    {
      sInt o0 = f->Vertex[j];
      sInt o1 = f->Vertex[(j+1)%f->Count];
      sInt v0 = map[o0];
      sInt v1 = map[o1];

      sInt hash = (v0^v1)&(hashmax-1);
      sInt ei = hashtable[hash];

      if(v0<v1)
      {
        while(ei>=0)
        {
          e = &edges[ei];
          if(e->Vert0==v0 && e->Vert1==v1 && e->FaceA==-1)
            goto found1a;
          ei = e->HashNext;
        }
        e = edges.AddMany(1);
        e->Vert0 = v0;
        e->Vert1 = v1;
        e->FaceB = -1;
        e->FaceBI = -1;
        e->VertB0 = -1;
        e->VertB1 = -1;
        e->SplitVertA = -1;
        e->SplitVertB = -1;
        e->Subdivide = 1;
        e->Hash = hash;
        e->HashNext = hashtable[hash];
        hashtable[hash] = e-edges.GetData();
found1a:
        e->FaceA = i;
        e->FaceAI = j;
        e->VertA0 = o0;
        e->VertA1 = o1;
        edgelink[i*4+j] = e;
      }
      else
      {
        while(ei>=0)
        {
          e = &edges[ei];
          if(e->Vert0==v1 && e->Vert1==v0 && e->FaceB==-1)
            goto found1b;
          ei = e->HashNext;
        }
        e = edges.AddMany(1);
        e->Vert0 = v1;
        e->Vert1 = v0;
        e->FaceA = -1;
        e->FaceAI = -1;
        e->VertA0 = -1;
        e->VertA1 = -1;
        e->Hash = hash;
        e->HashNext = hashtable[hash];
        hashtable[hash] = e-edges.GetData();
found1b:
        e->FaceB = i;
        e->FaceBI = j;
        e->VertB0 = o1;
        e->VertB1 = o0;
        e->SplitVertA = -1;
        e->SplitVertB = -1;
        e->Subdivide = 1;
        edgelink[i*4+j] = e;
      }
    }
  }

  delete[] hashtable;

  // count splits and mark vertices that need moving

  sInt splitedge = 0;
  sInt splitface = 0;

  sFORALL(Vertices,v)
    v->Temp = 1;

  sFORALL(Faces,f)
  {
    if(f->Select>=0.5f)
    {
      splitface++;
      for(sInt j=0;j<f->Count;j++)
      {
        e = edgelink[_i*4+j];
        if(e->SplitVertA==-1)
        {
          e->SplitVertA = 0;
          e->SplitVertB = 0;
          splitedge++;
        }
      }
    }
    else 
    {
      for(sInt j=0;j<f->Count;j++)
      {
        Vertices[map[f->Vertex[j]]].Temp = 0;
        e = edgelink[_i*4+j];
        e->Subdivide = 0;
      }
    }
  }

  if(splitface==0)
    goto ende;
 
  // create face centerpoints

  sFORALL(Faces,f)
  {
    centervert[_i] = -1;
    sF32 rc = 1.0f/f->Count;
    centerpos[_i].Init(0,0,0);
    for(sInt j=0;j<f->Count;j++)
      centerpos[_i] += sVector30(Vertices[f->Vertex[j]].Pos)*rc;
    if(f->Select>=0.5f)
    {
      centervert[_i] = Vertices.GetCount();
      Wz4MeshVertex *v = Vertices.AddMany(1);

      v->Zero();
      for(sInt j=0;j<f->Count;j++)
        v->AddScale(&Vertices[f->Vertex[j]],rc);
    }
  }

  // create edge split points

  Vertices.HintSize(Vertices.GetCount()+splitedge+splitface);
  sFORALL(edges,e)
  {
    if(e->FaceA==-1 || e->FaceB==-1)
      e->Subdivide = 0;
    if(e->SplitVertA==0)
    {
      sVector31 fp;
      sVector31 fpp;
      Wz4MeshVertex *v0,*v1;
      v0=v1=0;
      if(e->FaceA>=0 && e->FaceB>=0)
        fp = sVector31((centerpos[e->FaceA]+centerpos[e->FaceB])*0.5f);
      else if(e->FaceA>=0)
        fp = sVector31(centerpos[e->FaceA]);
      else if(e->FaceB>=0)
        fp = sVector31(centerpos[e->FaceB]);

      e->SplitVertA = Vertices.GetCount();
      e->SplitVertB = e->SplitVertA;
      if(e->VertA0==e->VertB0 && e->VertA1==e->VertB1)
      {
        Wz4MeshVertex *v = Vertices.AddMany(1);

        v0 = &Vertices[e->VertA0];
        v1 = &Vertices[e->VertA1];

        v->Zero();
        v->AddScale(v0,0.5f);
        v->AddScale(v1,0.5f);
        if(e->Subdivide)
        {
          v->Pos.Average(v0->Pos,v1->Pos);
          fpp.Average(v[0].Pos,fp);
          v[0].Pos.Fade(smooth,v[0].Pos,fpp);
        }
      }
      else if(e->FaceA>=0 || e->FaceB>=0)
      {
        sVector31 pos;
        v0 = &Vertices[e->Vert0];
        v1 = &Vertices[e->Vert1];
        pos.Average(v0->Pos,v1->Pos);
        if(e->Subdivide)
        {
          fpp.Average(pos,fp);
          pos.Fade(smooth,pos,fpp);
        }
        if(e->FaceA>=0)
        {
          Wz4MeshVertex *v = Vertices.AddMany(1);

          v0 = &Vertices[e->VertA0];
          v1 = &Vertices[e->VertA1];
          v->Zero();
          v->AddScale(v0,0.5f);
          v->AddScale(v1,0.5f);
          v->Pos = pos;
        }

        if(e->FaceB>=0)
        {
          e->SplitVertB = Vertices.GetCount();
          Wz4MeshVertex *v = Vertices.AddMany(1);

          v0 = &Vertices[e->VertB0];
          v1 = &Vertices[e->VertB1];
          v->Zero();
          v->AddScale(v0,0.5f);
          v->AddScale(v1,0.5f);
          v->Pos = pos;
        }
      }
    }
  }

  // move original points

  for(sInt i=0;i<maxvert;i++)
  {
    sClear(vertinfo[i]);
    vertinfo[i].Subdivide = 1;
  }
  sFORALL(Faces,f)
  {
    sInt i=_i;
    for(sInt j=0;j<f->Count;j++)
    {
      sInt n = map[f->Vertex[j]];
      vertinfo[n].Face += centerpos[i];
      vertinfo[n].FaceCount++;
    }
  }
  sFORALL(edges,e)
  {
    if(e->SplitVertA>=0)
    {
      vertinfo[e->Vert0].Edge += sVector30(Vertices[e->SplitVertA].Pos);
      vertinfo[e->Vert0].EdgeCount++;
      vertinfo[e->Vert0].Subdivide &= e->Subdivide;
      vertinfo[e->Vert1].Edge += sVector30(Vertices[e->SplitVertA].Pos);
      vertinfo[e->Vert1].EdgeCount++;
      vertinfo[e->Vert1].Subdivide &= e->Subdivide;
    }
    else if(e->SplitVertB>=0)
    {
      vertinfo[e->Vert0].Edge += sVector30(Vertices[e->SplitVertB].Pos);
      vertinfo[e->Vert0].EdgeCount++;
      vertinfo[e->Vert0].Subdivide &= e->Subdivide;
      vertinfo[e->Vert1].Edge += sVector30(Vertices[e->SplitVertB].Pos);
      vertinfo[e->Vert1].EdgeCount++;
      vertinfo[e->Vert1].Subdivide &= e->Subdivide;
    }
  }
  for(sInt i=0;i<maxvert;i++)
  {
    v = &Vertices[i];
    if(v->Temp && vertinfo[i].FaceCount>0)
    {
      sF32 f = vertinfo[i].FaceCount;
      sF32 e = vertinfo[i].EdgeCount;
      sVector30 ff = vertinfo[i].Face/f;
      sVector30 ee = vertinfo[i].Edge/e;
      sVector30 pp = sVector30(v->Pos);
      if(vertinfo[i].Subdivide)
      {
        sVERIFY(vertinfo[i].FaceCount==vertinfo[i].EdgeCount);
        v->Pos.Fade(smooth,v->Pos,sVector31((pp*(f-3)+ee*2+ff)/f));
      }
      else
      {
        v->Pos.Fade(smooth,v->Pos,sVector31(ee));  // case with borders
      }
    }
  }

  // update edges that border a hole. this does not make round holes.
    
  sFORALL(edges,e)
  {
    if(!e->Subdivide)
    {
      Wz4MeshVertex *v0 = &Vertices[e->Vert0];
      Wz4MeshVertex *v1 = &Vertices[e->Vert1];

      if(e->SplitVertA>=0)
        Vertices[e->SplitVertA].Pos.Average(v0->Pos,v1->Pos);
      if(e->SplitVertB>=0)
        Vertices[e->SplitVertB].Pos.Average(v0->Pos,v1->Pos);

    }
  }

  // update faces

  max = Faces.GetCount();
  for(sInt i=0;i<max;i++)
  {
    f = &Faces[i];
    sInt count = f->Count;
    if(f->Select>=0.5f)
    {
      Wz4MeshFace *nf = Faces.AddMany(count);
      f = &Faces[i];
      for(sInt j=0;j<count;j++)
      {
        nf->Cluster = f->Cluster;
        nf->Count = 4;
        nf->Select = f->Select;
        nf->Selected = f->Selected;
        nf->Vertex[0] = centervert[i];
        nf->Vertex[2] = f->Vertex[(j+1)%count];

        e = edgelink[i*4+j];
        if(e->FaceA==i)
          nf->Vertex[1] = e->SplitVertA;
        else
          nf->Vertex[1] = e->SplitVertB;
        
        e = edgelink[i*4+(j+1)%count];
        if(e->FaceA==i)
          nf->Vertex[3] = e->SplitVertA;
        else
          nf->Vertex[3] = e->SplitVertB;
        nf++;
      }

      f->Count = 0;      
    }
    else
    {
      sInt split = 0;
      for(sInt j=0;j<count && !split;j++)
        if(edgelink[i*4+j]->SplitVertA>=0)
          split = 1;

      if(split)
      {
        sInt sv[8];
        sInt sc = 0;
        sInt sg = 0;
        for(sInt j=0;j<count;j++)
        {
          sv[sc++] = f->Vertex[j];
          e = edgelink[i*4+j];
          
          if(e->SplitVertA>=0)
          {
            sg = sc;
            if(e->FaceA==i) 
              sv[sc++] = e->SplitVertA;
            else
              sv[sc++] = e->SplitVertB;
          }
        }
        Wz4MeshFace *nf = Faces.AddMany(sc-2);
        f = &Faces[i];
        for(sInt j=0;j<sc-2;j++)
        {
          nf->Count = 3;
          nf->Cluster = f->Cluster;
          nf->Select = f->Select;
          nf->Selected = f->Selected;
          nf->Vertex[0] = sv[(sg    )%sc];
          nf->Vertex[1] = sv[(sg+j+1)%sc];
          nf->Vertex[2] = sv[(sg+j+2)%sc];
          nf++;
        }
        f->Count = 0;
      }
    }
  }

  // weed out unused faces

  sRemFalse(Faces,&Wz4MeshFace::Count);

  // copy position to split vertices

  for(sInt i=0;i<maxvert;i++)
    Vertices[i].Pos = Vertices[map[i]].Pos;

  for(sInt i=0;i<Vertices.GetCount();i++)
    Vertices[i].NormWeight();

  // done
ende:
  delete[] vertinfo;
  delete[] centervert;
  delete[] edgelink;
  delete[] map;
  delete[] centerpos;
}

/****************************************************************************/

void Wz4Mesh::SelStoreLoad(sInt mode, sInt type, sInt slot)
{
  Wz4MeshFace *f;
  Wz4MeshVertex *v;
  Wz4MeshSel s;

  switch(mode)
  {
  case wMSM_LOAD:
    switch(type)
    {
    case wMST_VERTEX:
      // clear vertices selection
      sFORALL(Vertices,v)
        v->Select = 0.0f;

      // read all vertices stored and set selection
      for(int i=0; i<SelVertices[slot].GetCount(); i++)
        Vertices[SelVertices[slot][i].Id].Select = SelVertices[slot][i].Selected;

      // clear faces selection
      sFORALL(Faces,f)
        f->Select = 0;
      break;

    case wMST_FACE:
      sFORALL(Faces,f)
      {
        if((f->Selected & (1 << slot)) > 0)
          f->Select = 1;
        else
          f->Select = 0;
      }

      // clear vertices selection
      sFORALL(Vertices,v)
        v->Select = 0.0f;
      break;
    }
    break;

  case wMSM_STORE:
    switch(type)
    {
    case wMST_VERTEX:
      // clear previous selection in this slot
      SelVertices[slot].Clear();

      // if vertex is selected add it's value and id to array
      sFORALL(Vertices,v)
      {
        if(v->Select > 0.0f)
        {
          s.Id = _i;
          s.Selected = v->Select;
          SelVertices[slot].AddTail(s);
        }
      }

      // clear faces selection
      sFORALL(Faces,f)
        f->Select = 0;
      break;

    case wMST_FACE:
      sFORALL(Faces,f)
      {
        if(f->Select > 0)
          f->Selected |= (1 << slot);
        else
          f->Selected &= ~(1 << slot);
      }

      // clear vertices selection
      sFORALL(Vertices,v)
        v->Select = 0.0f;
      break;
    }
    break;
  }
}

/****************************************************************************/

void Wz4Mesh::SelVerticesToFaces(sBool outputType, sBool addToInput, sF32 vertexValue)
{
  // outputType = 0 => touched Faces
  // outputType = 1 => enclosed Faces
  // addToInput = 0 => previous face selection is cleared
  // addToInput = 1 => add new selected faces to previous face selection
  // vertexValue => new selected vertex value

  Wz4MeshVertex *v;
  Wz4MeshFace *f;

  sInt pc = Vertices.GetCount();
  sF32 *fsel = new sF32[pc];

  sFORALL(Vertices,v)
  {
    fsel[_i] = v->Select;

    // replace select value
    if(v->Select >= 0.5f)
      v->Select = vertexValue;
  }

  sFORALL(Faces,f)
  {
    sInt n=0;
    sBool action=0;

    for(sInt i=0; i<f->Count; i++)
      n += (fsel[f->Vertex[i]]>=0.5f)?1:0;

    // select touched or enclosed faces
    if(outputType)
    {
      // enclosed faces
      action = (n==f->Count);
    }
    else
    {
       // touched faces
      action = (n>0);
    }

    // selection output
    if(addToInput)
    {
      // add new selected faces to previous face selection
      if(action)
        f->Select = 1;
    }
    else
    {
      // don't keep previous face selection
      f->Select = action?1:0;
    }
  }

  delete[] fsel;
}

/****************************************************************************/

void Wz4Mesh::SelFacesToVertices(sBool outputType, sInt addToInput, sF32 value, sBool clearFaces)
{
  // outputType = 0 => inner vertex
  // outputType = 1 => full vertex
  // addToInput = 0 => previous vertex selection is cleared
  // addToInput = 1 => add new selected vertex to previous vertice selection
  // vertexValue => new selected vertex value
  // clearFaces = 0 => clear previous face selection
  // clearFaces = 1 => keep previous face selection

  Wz4MeshVertex *v;
  Wz4MeshFace *f;

  sInt pc = Vertices.GetCount();
  sF32 *fsel = new sF32[pc];

  for(sInt i=0; i<pc; i++)
    fsel[i] = 0.0f;

  sFORALL(Faces,f)
  {
    if(f->Select==1)
    {
      for(sInt i=0; i<f->Count; i++)
        fsel[f->Vertex[i]] = value;
    }

    if(clearFaces)
      f->Select = 0;
  }

  // select full vertex
  if(outputType)
  {
    sInt *base = BasePos(1);

    for (sInt i=0;i<pc;i++)
      fsel[base[i]] = sMax(fsel[base[i]],fsel[i]);

    for (sInt i=0;i<pc;i++)
      fsel[i] = fsel[base[i]];

    delete[] base;
  }

  sFORALL(Vertices,v)
  {
    if(addToInput)
      v->Select = sMin(v->Select+fsel[_i],1.0f);
    else
      v->Select = fsel[_i];
  }

  delete[] fsel;
}

/****************************************************************************/

void Wz4Mesh::SelectGrow(Wz4MeshFaceConnect *adj, sInt amount, sInt power, sF32 range)
{
  Wz4MeshFace *f;
  sF32 selectValue = sClamp( powf(amount*range, power) ,0.0f, 1.0f);

  sFORALL(Faces,f)
    f->Temp = 0;

  // calculation is based on faces selection
  sFORALL(Faces,f)
  {
    if(f->Select==1 && f->Temp==0)
    {
      for(sInt j=0; j<f->Count; j++)
      {
        sInt m = adj[_i].Adjacent[j];
        if(m>=0)
        {
          Wz4MeshFace *f0 = &Faces[m/4];
          if(f0->Select==0)
          {
            f0->Temp = 1;

            // select faces and vertices
            f0->Select = 1;
            for(sInt k=0; k<f0->Count; k++)
            {
              if(Vertices.IsIndexValid(f0->Vertex[k]))
                Vertices[f0->Vertex[k]].Select = selectValue;
            }
          }
        }
      }
    }
  }
}

/****************************************************************************/

struct IslandEdge 
{ 
  sInt p[2]; 
  sInt px[2];
  sInt Cluster;
};
struct Island
{
  sInt FirstFace;             // first face in island (head of linked list)
  sInt LastFace;              // last face in island (tail of linked list)
  sInt FirstEdge;             // first edge in island (start position in global list)
  sInt NumEdges;              // number of edges in island
  sInt FirstCenter;           // vertices in island but off border (head of linked list)
  sVector31 Center;
  sVector30 Normal;
};

// Fade such that fade(a, b, 0) == a and fade(a, b, 1) == b for all finite floats.
sF32 ExactFade(sF32 a,sF32 b,sF32 t)
{
  return a*(1.0f-t) + b*t;
}

static void ExactFade(sVector31 &out,const sVector31 &a,const sVector31 &b,sF32 t)
{
  sF32 u = 1.0f - t;
  out.x = u*a.x + t*b.x;
  out.y = u*a.y + t*b.y;
  out.z = u*a.z + t*b.z;
}

void Wz4Mesh::Extrude(sInt steps,sF32 amount,sInt flags,const sVector31 &center,sF32 localScale,sInt SelectUpdateFlag,const sVector2 &uvOffset)
{
  sVERIFY(steps>0);

  // preliminiaries

  Wz4MeshFace *f;
  Wz4MeshVertex *v;
  IslandEdge *e;
  Island *isl;

  Wz4MeshFaceConnect *adj = Adjacency();
  sArray<Island> Islands;
  sArray<IslandEdge> Edges;
  sInt originalfacecount = Faces.GetCount();
  sInt *map = BasePos();
  for(sInt i=0;i<Vertices.GetCount();i++)
    if(map[i]==-1)
      map[i] = i;

  // store original faces (to keep trace of selected faces)
  sArray<Wz4MeshFace> originalFaces;
  originalFaces.Add(Faces);

  // find connected islands of selected faces
  sInt nFaces = Faces.GetCount();
  sInt *faceIsland = new sInt[nFaces];
  sInt *nextFaceInIsland = new sInt[nFaces];

  sInt nVerts = Vertices.GetCount();
  sArray<sInt> vertTag1,vertTag2,nextCenter;
  vertTag1.AddMany(nVerts);
  vertTag2.AddMany(nVerts);
  nextCenter.AddMany(nVerts);

  for(sInt i=0;i<nFaces;i++)
    faceIsland[i] = -1;

  sFORALL(Faces,f)
  {
    sInt i = _i;
    if(f->Select>=0.5f && faceIsland[i]==-1)                 // first face of island
    {
      f->Select = 0.0f;              // put into new island

      isl = Islands.AddMany(1);
      isl->FirstFace = isl->LastFace = i;
      isl->FirstEdge = -1;
      isl->NumEdges = 0;
      isl->FirstCenter = -1;
      nextFaceInIsland[i] = -1;
      
      sInt isli = Islands.GetCount()-1;
      faceIsland[i] = isli;

      if(!(flags & 1))
      {
        for(sInt fi = isl->FirstFace; fi != -1; fi = nextFaceInIsland[fi]) // for all faces in island, new or old
        {
          // check all connected faces
          sInt max = Faces[fi].Count;
          for(sInt k=0;k<max;k++)
          {
            sInt m = adj[fi].Adjacent[k]/4;
            if(m>=0 && Faces[m].Select>=0.5f)     // and put them into island to be checked themself
            {
              Faces[m].Select = 0.0f;

              // add to island
              faceIsland[m] = isli;

              // add to face list
              nextFaceInIsland[isl->LastFace] = m;
              nextFaceInIsland[m] = -1;
              isl->LastFace = m;
            }
          }
        }
      }
    }
  }

  // tag fields

  for(sInt i=0;i<nVerts;i++)
    vertTag1[i] = -1;

  // analyse topology

  sInt nTotalExtrudeEdges = 0;
  sInt nEdgeVerts = 0;

  sFORALL(Islands,isl)
  {
    sInt isli = _i;

    // select faces and vertices in this island
    // also calc normal

    sVector30 Normal(0.0f);
    sVector4 Center;
    sInt head = -1;

    for(sInt fi=isl->FirstFace; fi != -1; fi = nextFaceInIsland[fi])
    {
      f = &Faces[fi];
      for(sInt j=0;j<f->Count;j++)
      {
        sInt vi = f->Vertex[j];
        if(vertTag1[vi] != isli)
        {
          // tag active
          vertTag1[vi] = isli;
          Center += Vertices[vi].Pos;
          Normal += Vertices[vi].Normal;

          // add to linked vertex list
          vertTag2[vi] = head; // link to current head
          head = vi;
        }
      }
    }

    // finish normal

    Normal.Unit();
    isl->Normal = Normal;

    Center *= 1.0f / Center.w;
    isl->Center = sVector31(Center);

    // find edges for islands

    sInt firstEdge;
    isl->FirstEdge = firstEdge = Edges.GetCount();

    for(sInt fi=isl->FirstFace; fi != -1; fi = nextFaceInIsland[fi])
    {
      f = &Faces[fi];
      for(sInt j=0;j<f->Count;j++)
      {
        sInt n = adj[fi].Adjacent[j]/4;
        if(n==-1 || faceIsland[n] != isli)
        {
          e = Edges.AddMany(1);
          e->p[0] = f->Vertex[j];
          e->p[1] = f->Vertex[(j+1)%f->Count];
          e->px[0] = -1;
          e->px[1] = -1;
          e->Cluster = f->Cluster;

          isl->NumEdges++;
        }
      }
    }

    // sort edges to loop
  
    sInt max = isl->NumEdges;
    for(sInt i=0;i<max-1;i++)
    {
      for(sInt j=i+2;j<max;j++)
      {
        if(map[Edges[firstEdge+i].p[1]]==map[Edges[firstEdge+j].p[0]])
        {
          Edges.Swap(firstEdge+i+1,firstEdge+j);
          break;
        }
      }
    }

    // find vertices inside island, but off the border

    for(sInt i=0;i<max;i++)
    {
      e = &Edges[firstEdge+i];
      vertTag1[e->p[0]] = -1;
      vertTag1[e->p[1]] = -1;
    }

    nTotalExtrudeEdges += isl->NumEdges;

    sInt first = -1;

    for(sInt vi=head; vi!=-1; vi=vertTag2[vi])
    {
      if(vertTag1[vi] == isli)
      {
        // add to center list
        nextCenter[vi] = first;
        first = vi;
      }
      else
        nEdgeVerts++;
    }

    isl->FirstCenter = first;
  }

  // extrude islands. we do this after completion of analysis of topology.
  for(sInt i=0;i<nVerts;i++)
  {
    vertTag1[i] = -1;
    vertTag2[i] = -1;
  }

  // reserve space for vertices we're about to allocate
  sInt newVertCount = Vertices.GetCount() + nEdgeVerts + nTotalExtrudeEdges*(steps+1)*2;
  Vertices.HintSize(newVertCount);

  sFORALL(Islands,isl)
  {
    sInt isli = _i;

    // duplicate border vertices.

    for(sInt ei=0;ei<isl->NumEdges;ei++)
    {
      e = &Edges[isl->FirstEdge + ei];

      for(sInt j=0;j<2;j++)
      {
        v = &Vertices[e->p[j]];
        if(vertTag1[e->p[j]] != isli)
        {
          vertTag1[e->p[j]] = isli;
          e->px[j] = v->Temp = Vertices.GetCount();
          Wz4MeshVertex *vn = Vertices.AddMany(1);
          v = &Vertices[e->p[j]];
          vn->CopyFrom(v);
          vn->Normal = Vertices[map[e->p[j]]].Normal;
          vn->Temp = -1;

          // add to center list
          nextCenter.AddTail(isl->FirstCenter);
          vertTag1.AddTail(-1);
          vertTag2.AddTail(-1);
          nVerts++;
          isl->FirstCenter = v->Temp;
        }
        else
        {
          e->px[j] = v->Temp;
        }
      }
    }

    // transform away island
    switch((flags >> 1) & 3)
    {
    case 0:
      for(sInt vi=isl->FirstCenter; vi != -1; vi=nextCenter[vi])
      {
        v = &Vertices[vi];
        v->Pos = isl->Center + (v->Pos - isl->Center) * localScale;
        v->Pos += isl->Normal*amount;
        v->U0 += uvOffset.x;
        v->V0 += uvOffset.y;
      }
      break;
    case 1:
      for(sInt vi=isl->FirstCenter; vi != -1; vi=nextCenter[vi])
      {
        v = &Vertices[vi];
        v->Pos = isl->Center + (v->Pos - isl->Center) * localScale;
        v->Pos += v->Normal*amount;
        v->U0 += uvOffset.x;
        v->V0 += uvOffset.y;
      }
      break;
    case 2:
      for(sInt vi=isl->FirstCenter; vi != -1; vi=nextCenter[vi])
      {
        v = &Vertices[vi];
        sVector30 n = v->Pos-center;
        n.Unit();
        v->Pos = isl->Center + (v->Pos - isl->Center) * localScale;
        v->Pos += n*amount;
        v->U0 += uvOffset.x;
        v->V0 += uvOffset.y;
      }
      break;
    }

    // add extrude triangles

    for(sInt ei = 0; ei < isl->NumEdges; ei++)
    {
      e = &Edges[isl->FirstEdge + ei];

      Vertices[e->p[0]].Temp = e->px[0];
      Vertices[e->p[1]].Temp = e->px[1];
      vertTag2[e->p[0]] = isli;
      vertTag2[e->p[1]] = isli;

      f = Faces.AddMany(steps);
      sInt vn = Vertices.GetCount();
      sInt nNewVerts = (steps+1)*2;
      v = Vertices.AddMany(nNewVerts);

      // add to other structures too
      vertTag1.AddMany(nNewVerts);
      vertTag2.AddMany(nNewVerts);
      nextCenter.AddMany(nNewVerts);
      nVerts += nNewVerts;
      for(sInt j=0;j<nNewVerts;j++)
      {
        vertTag1[vn + j] = -1;
        vertTag2[vn + j] = -1;
      }

      for(sInt j=0;j<steps+1;j++)
      {
        sF32 fade = j/sF32(steps);

        for(sInt k=0;k<2;k++)
        {
          const Wz4MeshVertex &va = Vertices[e->p[k]];
          const Wz4MeshVertex &vb = Vertices[e->px[k]];

          v[j*2+k].Init();
          ExactFade(v[j*2+k].Pos,va.Pos,vb.Pos,fade);
          v[j*2+k].Normal.x = isli; // normal set up to prevent verts from being merged erroneously
          v[j*2+k].Normal.y = ei;
          v[j*2+k].Normal.z = j*2+k;
          v[j*2+k].U0 = ExactFade(va.U0,vb.U0,fade);
          v[j*2+k].V0 = ExactFade(va.V0,vb.V0,fade);
          v[j*2+k].U1 = ExactFade(va.U1,vb.U1,fade);
          v[j*2+k].V1 = ExactFade(va.V1,vb.V1,fade);

          for(sInt i=0;i<sCOUNTOF(va.Index);i++)
          {
            if(va.Index[i] >= 0) v[j*2+k].AddWeight(va.Index[i],(1.0f-fade)*va.Weight[i]);
            if(vb.Index[i] >= 0) v[j*2+k].AddWeight(vb.Index[i],fade*vb.Weight[i]);
          }

          v[j*2+k].NormWeight();
        }
      }

      for(sInt i=0;i<steps;i++)
      {
        f->Init(4);
        f->Cluster = e->Cluster;
        f->Vertex[0] = vn+2+i*2;
        f->Vertex[1] = vn+0+i*2;
        f->Vertex[2] = vn+1+i*2;
        f->Vertex[3] = vn+3+i*2;
        f++;
      }
    }

    // update face.

    for(sInt fi = isl->FirstFace; fi != -1; fi = nextFaceInIsland[fi])
    {
      f = &Faces[fi];
      for(sInt j=0;j<f->Count;j++)
      {
        sInt vi = f->Vertex[j];
        if(vertTag2[vi] == isli)
          f->Vertex[j] = Vertices[vi].Temp;
      }
    }
  }

  // update selection

  // clear selected vertices
  sFORALL(Vertices,v)
    v->Select = 0.0f;

  // which faces to select ?
  switch (SelectUpdateFlag)
  {
  case 0: // newest faces
    sFORALL(Faces,f)
      f->Select = (_i>=originalfacecount)?1.0f:0.0f;
    break;

  case 1: // original selected faces
    sFORALL(originalFaces,f)
      Faces[_i].Select = (f->Select > 0.0f)?1.0f:0.0f;
    break;
  }


  // done and free all

  delete[] faceIsland;
  delete[] nextFaceInIsland;
  delete[] adj;
  delete[] map;

  CalcNormalAndTangents();
}

/****************************************************************************/

void Wz4Mesh::Splitter(Wz4Mesh *in,sF32 depth,sF32 scale)
{
  Wz4MeshFace *fi,*fo;
  Wz4MeshVertex *v;
  Wz4ChunkPhysics *ch;

  sRandom rnd;

  CopyClustersFrom(in);

  Chunks.AddMany(in->Faces.GetCount());
  sFORALL(in->Faces,fi)
  {
    sVector31 vert[2][4];
    sVector30 n;
    sVector31 c;

    sInt max = fi->Count;

    { // calculate vertex positions

      sVector30 na,ca;

      // copy original positions


      for(sInt i=0;i<max;i++)
        vert[0][i] = in->Vertices[fi->Vertex[i]].Pos;

      // calculate face normal and center

      na.Init(0,0,0);
      ca.Init(0,0,0);
      for(sInt i=0;i<max;i++)
      {
        n.Cross(vert[0][(i+0)%max]-vert[0][(i+1)%max],vert[0][(i+1)%max]-vert[0][(i+2)%max]);
        n.Unit();
        na += n;
        ca += sVector30(vert[0][i]);
      }
      n = na*(1.0f/sF32(max));
      c = sVector31(ca*(1.0f/sF32(max)));
      c += n*depth;

      // calculat lower positions

      for(sInt i=0;i<max;i++)
        vert[1][i] = (((vert[0][i] + n*depth)-c)*scale)+c;
    }

    // chunk physics

    ch = &Chunks[_i];
    ch->FirstVert = Vertices.GetCount();
    ch->FirstFace = Faces.GetCount();
    ch->FirstIndex = 0;
    ch->Volume = 1;
    ch->COM = c;
    ch->InertD = n;
    ch->InertOD.Init(0,0,0);
    ch->Normal = n;
    ch->Random.InitRandom(rnd);
    ch->Random.Unit();

    // allocate new mesh

    fo = Faces.AddMany(2+max);
    sInt vn = Vertices.GetCount();
    sInt v0 = vn;
    v = Vertices.AddMany(max*6);    // oben(1), unten(1), round (4)

    // top face

    fo->Init(max);
    fo->Cluster = fi->Cluster;
    for(sInt i=0;i<max;i++)
    {
      fo->Vertex[i] = vn++;
      v->CopyFrom(&in->Vertices[fi->Vertex[i]]);
      v++;
    }
    fo++;

    // bottom face

    fo->Init(max);
    fo->Cluster = fi->Cluster;
    for(sInt i=0;i<max;i++)
    {
      fo->Vertex[i] = vn++;
      v->CopyFrom(&in->Vertices[fi->Vertex[i]]);
      v->Pos = vert[1][max-i-1];
      v->Normal.Init(10,i,_i);
      v++;
    }
    fo++;

    // around faces

    for(sInt i=0;i<max;i++)
    {
      fo->Init(4);
      fo->Cluster = fi->Cluster;
      for(sInt j=0;j<4;j++)
        fo->Vertex[j] = vn++;
      fo++;

      v->CopyFrom(&in->Vertices[fi->Vertex[(i+0)%max]]);
      v->Pos = vert[1][(i+0)%max];
      v->Normal.Init(11,i,_i);
      v++;
      v->CopyFrom(&in->Vertices[fi->Vertex[(i+1)%max]]);
      v->Pos = vert[1][(i+1)%max];
      v->Normal.Init(11,i,_i);
      v++;
      v->CopyFrom(&in->Vertices[fi->Vertex[(i+1)%max]]);
      v->Pos = vert[0][(i+1)%max];
      v->Normal.Init(11,i,_i);
      v++;
      v->CopyFrom(&in->Vertices[fi->Vertex[(i+0)%max]]);
      v->Pos = vert[0][(i+0)%max];
      v->Normal.Init(11,i,_i);
      v++;
    }

    // skin to chunk

    for(sInt i=v0;i<vn;i++)
    {
      Vertices[i].Index[0] = _i;
      Vertices[i].Weight[0] = 1;
    }
  }

  SplitClustersChunked(74);
  CalcNormalAndTangents();
}

/****************************************************************************/

struct Wz4MeshDualEdge
{
  sInt p0;
  sInt p1;
  sInt f;
};

void Wz4Mesh::Dual(Wz4Mesh *in,sF32 random)
{
  Wz4MeshFace *fi,*fo;
  Wz4MeshVertex *v;
  sInt max = in->Faces.GetCount();
  sRandom rnd;

  AddDefaultCluster(); 

  // place vertices at the center of the faces

  sF32 r0 = sClamp(random,0.0f,1.0f);     // random 0..1
  sF32 r1 = sClamp(2-random,0.0f,1.0f);   // 1..2 -> 1..0 : multply n-2 vertices with this to push to border of face

  v = Vertices.AddMany(max);
  sFORALL(in->Faces,fi)
  {
    sInt max = fi->Count;
    sF32 rf[4];

    for(sInt j=0;j<max;j++)
      rf[j] = 1-rnd.Float(r0);
    sInt jo = rnd.Int(max);
    for(sInt j=0;j<max-2;j++)
      rf[(j+jo)%max] *= r1;

    sVector4 c;
    c.Init(0,0,0,0);
    for(sInt j=0;j<max;j++)
    {
      c += in->Vertices[fi->Vertex[j]].Pos * rf[j];
    }
    v->Init();
    v->Pos = sVector31(sVector30(c)/c.w);
    v->Normal.Init(10,0,_i);
    v++;
  }

  // create a list of possible edges

  sInt *map = in->BasePos();
  for(sInt i=0;i<in->Vertices.GetCount();i++)
    if(map[i]==-1)
      map[i] = i;
  Wz4MeshFaceConnect *adj = in->Adjacency();
  sArray<Wz4MeshDualEdge> Edges;
  Edges.HintSize(max*8);

  for(sInt i=0;i<max;i++)
  {
    sInt count = in->Faces[i].Count;
    for(sInt j=0;j<count;j++)
    {
      sInt opp = adj[i].Adjacent[j];
      if(opp>=0)
      {
        Wz4MeshDualEdge *e = Edges.AddMany(1);
        e[0].p0 = opp/4;  // original face -> new points
        e[0].p1 = i;
        e[0].f = map[in->Faces[i].Vertex[j]];
      }
    }
  }

  sHeapSortUp(Edges,&Wz4MeshDualEdge::f);

  // create faces. 

  sInt i0=0;
  sInt i1=0;
  sInt emax = Edges.GetCount();
  sArray<sInt> order;
  while(i0<emax)
  {

    // find edge loop

    for(i1=i0;i1<emax && Edges[i1].f==Edges[i0].f;i1++)
      ;
    sInt count= i1-i0;
    if(count>=3)
    {

      // sort edge loop

      order.Resize(count);

      for(sInt j=0;j<count;j++)
        order[j] = i0+j;

      for(sInt j=0;j<count-2;j++)
      {
        for(sInt k=j+2;k<count;k++)
        {
          if(Edges[order[j]].p1==Edges[order[k]].p0)
          {
            sSwap(order[j+1],order[k]);
            break;
          }
        }
      }

      // check if edge loop is correct

      sBool ok =1;
      for(sInt j=0;j<count;j++)
        if(Edges[order[j]].p1 != Edges[order[(j+1)%count]].p0)
          ok = 0;

      // create face

      if(ok)
      {
        if(count<=4)      // tris & quads
        {
          fo = Faces.AddMany(1);
          fo->Init(count);
          for(sInt j=0;j<count;j++)
            fo->Vertex[j] = Edges[order[j]].p0;
        }
        else              // tesselate >4 vertices
        {
          sInt nv = Vertices.GetCount();
          v = Vertices.AddMany(1);
          v->Init();
          v->Pos = in->Vertices[Edges[order[0]].f].Pos;
          v->Normal.Init(10,0,0);

          fo = Faces.AddMany(count);
          for(sInt j=0;j<count;j++)
          {
            fo->Init(3);
            fo->Vertex[0] = nv;
            fo->Vertex[1] = Edges[order[j]].p0;
            fo->Vertex[2] = Edges[order[(j+1)%count]].p0;
            fo++;
          }
        }
      }
    }

    // next loop

    i0 = i1;
  }


  // done

  delete[] adj;
  delete[] map;
  CalcNormalAndTangents();
}

/****************************************************************************/

sBool Wz4Mesh::DivideInChunksR(Wz4MeshFace *mf,sInt mfi,Wz4MeshFaceConnect *conn)
{
  sVERIFY(mf->Temp>=0);
  for(sInt i=0;i<mf->Count;i++)
  {
    if(Vertices[mf->Vertex[i]].SelectTemp<0)
      Vertices[mf->Vertex[i]].SelectTemp = mf->Temp;
    else
      if(Vertices[mf->Vertex[i]].SelectTemp != mf->Temp)
        return 0;
  }
  for(sInt i=0;i<mf->Count;i++)
  {
    if(conn[mfi].Adjacent[i]>=0)
    {
      sInt fi = conn[mfi].Adjacent[i]/4;
      Wz4MeshFace *f = &Faces[fi];
      if(f->Temp<0)
      {
        f->Temp = mf->Temp;
        if(!DivideInChunksR(f,fi,conn))
          return 0;
      }

      if(f->Temp != mf->Temp)
        return 0;
    }
  }
  return 1;
}

sBool Wz4Mesh::DivideInChunks(sInt flags,const sVector30 &normal,const sVector30 &rot)
{
  Wz4MeshFace *mf;
  Wz4MeshVertex *mv;
  Wz4MeshCluster *cl;
  Wz4ChunkPhysics *ch;

  MergeVertices();

  // find connected clusters

  Wz4MeshFaceConnect *conn = Adjacency();

  sFORALL(Faces,mf)
  {
    mf->Temp = -1;
    sVERIFY(mf->Cluster>=0 && mf->Cluster<Clusters.GetCount());
  }
  sFORALL(Vertices,mv)
    mv->SelectTemp = -1;

  sInt chunks = 0;
  sFORALL(Clusters,cl)
  {
    sInt cli = _i;
    sFORALL(Faces,mf)
    {
      if(mf->Cluster==cli && mf->Temp<0)
      {
        mf->Temp = chunks++;
        if(!DivideInChunksR(mf,_i,conn))
        {
          delete[] conn;
          return 0;
        }
      }
    }
  }

  delete[] conn;

  sFORALL(Vertices,mv)
    sVERIFY(mv->SelectTemp>=0);

  // sort faces by cluster

  sIntroSort(sAll(Faces),sMemberLess(&Wz4MeshFace::Temp));

  // reorganize vertices

  sFORALL(Vertices,mv)
    mv->Temp = _i;
  sIntroSort(sAll(Vertices),sMemberLess(&Wz4MeshVertex::SelectTemp));
  sInt *remap = new sInt[Vertices.GetCount()];
  sFORALL(Vertices,mv)
    remap[mv->Temp] = _i;
  sFORALL(Faces,mf)
    for(sInt i=0;i<mf->Count;i++)
      mf->Vertex[i] = remap[mf->Vertex[i]];
  delete[] remap;


  // debug

//  sFORALL(Vertices,mv)
//    mv->Pos.y += mv->SelectTemp*0.1f;

  // make chunks

  Chunks.Resize(chunks);
  
  sRandom rnd;
  sFORALL(Chunks,ch)
  {
    ch = &Chunks[_i];
    ch->FirstVert = 0;
    ch->FirstFace = 0;
    ch->FirstIndex = -1;
    ch->Volume = 1;
    ch->COM.Init(0,0,0);
    ch->InertD = normal;
    ch->InertOD.Init(0,0,0);
    ch->Normal = normal;
    if(flags&1)
    {
      ch->Random.InitRandom(rnd);
      ch->Random.Unit();  
    }
    else
    {
      ch->Random = rot;
    }
    ch->Temp = 0;
  }

  // calculate center of mass

  sFORALL(Vertices,mv)
  {
    if(mv->SelectTemp>=0 && mv->SelectTemp<chunks)
    {
      Chunks[mv->SelectTemp].COM += sVector30(mv->Pos);
      Chunks[mv->SelectTemp].Temp ++;
    }
  }

  // find first face

  sFORALL(Faces,mf)
  {
    if(Chunks[mf->Temp].FirstIndex==-1)
    {
      Chunks[mf->Temp].FirstIndex=-2;
      Chunks[mf->Temp].FirstFace=_i;
    }
  }

  // find first vertex

  sFORALL(Vertices,mv)
  {
    if(Chunks[mv->SelectTemp].FirstIndex<0)
    {
      Chunks[mv->SelectTemp].FirstIndex=0;
      Chunks[mv->SelectTemp].FirstVert=_i;
    }
    mv->Index[0] = mv->SelectTemp;
    mv->Index[1] = -1;
    mv->Index[2] = -1;
    mv->Index[3] = -1;
    mv->Weight[0] = 1;
    mv->Weight[1] = 0;
    mv->Weight[2] = 0;
    mv->Weight[3] = 0;
  }

  // finish up

  sFORALL(Chunks,ch)
  {
    if(ch->Temp>0)
    {
      ch->COM = sVector31(sVector30(ch->COM)*(1.0f/ch->Temp));
      ch->FirstIndex = 0;
    }
  }

  // writing SelectTemp destroyed vertex selection, so just clear
  // it to make sure it's initialized.

  sFORALL(Vertices,mv)
    mv->Select = 0.0f;
  SplitClustersChunked(74);
  return 1;
}

/****************************************************************************/

sVector30 Wz4Mesh::GetFaceNormal(sInt face) const
{
  sVector30 normal(0.0f);

  const Wz4MeshFace &fc = Faces[face];

  // newell normal for polygon
  for(sInt i = fc.Count-1, j = 0; j < fc.Count; i = j, j++)
  {
    const sVector31 &pi = Vertices[fc.Vertex[i]].Pos;
    const sVector31 &pj = Vertices[fc.Vertex[j]].Pos;

    normal.x += (pi.y - pj.y) * (pi.z + pj.z);
    normal.y += (pi.z - pj.z) * (pi.x + pj.x);
    normal.z += (pi.x - pj.x) * (pi.y + pj.y);
  }

  normal.Unit();
  return normal;
}

void Wz4Mesh::CalcBBox(sAABBox &outBox) const
{
  sAABBox box;
  box.Clear();

  for(sInt i=0;i<Vertices.GetCount();i++)
    box.Add(Vertices[i].Pos);

  outBox = box;
}

/****************************************************************************/

static sVector30 Normalize(const sVector30 &a)
{
  sF32 l;
  if(!(l = a.LengthSq()))
    return a;
  else
    return sFRSqrt(l) * a;
}

// scalar triple product
static sF32 TripleProduct(const sVector30 &a,const sVector30 &b,const sVector30 &c)
{
  return (a % b) ^ c;
}

void Wz4Mesh::Bevel(sF32 amount)
{
  Wz4MeshFace *mf,*omf,*pmf;
  Wz4MeshVertex *mv;
  sInt *aflags = new sInt[Faces.GetCount()];
  sInt *bflags = new sInt[Faces.GetCount()];
  sInt *newVert = new sInt[Faces.GetCount()*4];
  sVector30 *faceNorm = new sVector30[Faces.GetCount()];
  for(sInt i=0;i<Faces.GetCount();i++)
    aflags[i] = bflags[i] = 0;

  // we need a clean mesh to start with!

  CalcNormalAndTangents();
  MergeVertices();

  Wz4MeshFaceConnect *conn = Adjacency();

  sAABBox bbox;
  bbox.Clear();

  // init vertex->edge links to "no associated edge"
  // also find bounding box
  sFORALL(Vertices,mv)
  {
    bbox.Add(mv->Pos);
    mv->Temp = -1;
  }

  // calculate diameter to determine weld tolerance
  sF32 diam = sMax(bbox.Max.x-bbox.Min.x,sMax(bbox.Max.y-bbox.Min.y,bbox.Max.z-bbox.Min.z));
  sF32 weldTolerance = 1e-6f * diam;

  // find creases.
  // aflags: bitmask per face
  //   "vertex is part of a crease on the outgoing edge"
  // bflags: also bitmask per facce
  //   "vertex is part of a crease on the incoming edge"

  sFORALL(Faces,mf)
  {
    sInt fi = _i;
    mf->Select = 0;

    faceNorm[fi] = GetFaceNormal(fi);

    for(sInt vi=0;vi<mf->Count;vi++)
    {
      newVert[fi*4+vi] = mf->Vertex[vi];

      sInt oa = conn[fi].Adjacent[vi];
      sInt pa = conn[fi].Adjacent[(vi+mf->Count-1)%mf->Count];
      if(oa>=0 && pa>=0)
      {
        omf = &Faces[oa/4];
        sInt ovi = ((oa&3)+1)%omf->Count;

        pmf = &Faces[pa/4];
        sInt pvi = pa&3;

        if((Vertices[omf->Vertex[ovi]].Normal!=Vertices[mf->Vertex[vi]].Normal))
        {
          aflags[fi] |= 1<<vi;
          Vertices[mf->Vertex[vi]].Temp = OutgoingEdge(fi,vi); // this is a start edge for the duplication pass
        }

        if((Vertices[pmf->Vertex[pvi]].Normal!=Vertices[mf->Vertex[vi]].Normal))
          bflags[fi] |= 1<<vi;
      }
    }
  }

  // debug

  if(amount==0)
  {
    sFORALL(Vertices,mv)
      mv->Select = 0;
    sFORALL(Faces,mf)
      for(sInt j=0;j<mf->Count;j++)
        if((bflags[_i]|bflags[_i]) & (1<<j))
          Vertices[mf->Vertex[j]].Select = 1;
    delete[] aflags;
    delete[] bflags;
    delete[] conn;
    delete[] newVert;
    delete[] faceNorm;
    return;
  }

  // duplicate vertices
  sFORALL(Vertices,mv)
  {
    if(mv->Temp == -1) // not start of a crease, just ignore it
      continue;

    sInt startEdge = mv->Temp;
    sInt wedgeStart = -1;
    sVector31 vertPos = StartVertFromEdge(startEdge).Pos;
    sVector30 edgeNormalA,edgeNormalB,faceNormal;
    sVector30 dists(0.0f);

    sInt count = 0;
    sInt e = startEdge;
    do
    {
      count++;
      e = NextVertEdge(conn,e);
    }
    while(e != startEdge && e >= 0);

    e = startEdge;

    for(sInt i=0;i<count+1;i++)
    {
      sInt fi = GetFaceInd(e);
      sInt vi = GetVertInd(e);

      if(aflags[fi] & (1<<vi)) // wedge boundary
      {
        if(wedgeStart != -1)
        {
          sVector30 dir = vertPos - StartVertFromEdge(NextFaceEdge(e)).Pos;
          edgeNormalB = Normalize(dir % faceNormal);
          dists.y = (vertPos ^ edgeNormalB) - amount;

          // if the two edge vectors are (near) collinear, the system is singular.
          // it's trivial to solve though - just require that the vertex position projected
          // onto the edge direction stays the same.
          if(sFAbs(edgeNormalA ^ edgeNormalB) >= 0.996f)
          {
            edgeNormalB = dir;
            dists.y = vertPos ^ dir;
          }

          // make a new vertex
          sInt v = Vertices.GetCount();
          Wz4MeshVertex *nv = Vertices.AddMany(1);
          nv->CopyFrom(&StartVertFromEdge(e));
          nv->Temp = -1;

          // solve for new position - we have the necessary equations
          sF32 det = TripleProduct(edgeNormalA,edgeNormalB,faceNormal);
          if(sFAbs(det) > 1e-20f) // big enough to avoid overflow
          {
            // we actually need (edgeNormalA,edgeNormalB,faceNormal)
            // in transposed form.
            sMatrix34 tmp(edgeNormalA,edgeNormalB,faceNormal,sVector31(0,0,0));
            tmp.Trans3();

            // solve system using cramer's rule
            sF32 invDet = 1.0f / det;
            nv->Pos.x = invDet * TripleProduct(dists,tmp.j,tmp.k);
            nv->Pos.y = invDet * TripleProduct(tmp.i,dists,tmp.k);
            nv->Pos.z = invDet * TripleProduct(tmp.i,tmp.j,dists);
          }

          // use the new vertex in all affected faces
          for(sInt i = wedgeStart; i != e; i = NextVertEdge(conn,i))
            newVert[GetFaceInd(i)*4+GetVertInd(i)] = v;
        }

        // start of the next wedge
        wedgeStart = e;

        sVector30 dir = StartVertFromEdge(NextFaceEdge(e)).Pos - vertPos;
        faceNormal = faceNorm[GetFaceInd(e)];
        edgeNormalA = Normalize(dir % faceNormal);
        dists.x = (vertPos ^ edgeNormalA) - amount;
        dists.z = vertPos ^ faceNormal;
      }

      e = NextVertEdge(conn,e);
      if(e < 0)
        break;
    }
  }

  sFORALL(Faces,mf)
  {
    for(sInt j=0;j<mf->Count;j++)
      mf->Vertex[j] = newVert[_i*4+j];
  }

  // insert edge bevels

  sInt oldmaxface = Faces.GetCount();
  for(sInt i=0;i<oldmaxface;i++)
  {
    for(sInt j=0;j<Faces[i].Count;j++)
    {
      sInt e = OutgoingEdge(i,j);
      sInt c = Faces[i].Count;
      sInt mask = (1<<j) | (1<< ((j+1)%c) );
      sInt eo = conn[i].Adjacent[j]; // opposite half-edge to e
      if(eo > e && ((aflags[i]|bflags[i]) & mask)==mask )
      {
        sInt mvi = Vertices.GetCount();
        mv = Vertices.AddMany(4);
        mf = Faces.AddMany(1);
        mf->Select = 1;
        mf->Count = 4;
        mf->Cluster = Faces[i].Cluster;

        sInt vaa = StartVertex(e);
        sInt vab = EndVertex(e);
        sInt vba = EndVertex(eo);
        sInt vbb = StartVertex(eo);

        mv[3].CopyFrom(&Vertices[vaa]);
        mv[2].CopyFrom(&Vertices[vab]);
        mv[1].CopyFrom(&Vertices[vbb]);
        mv[0].CopyFrom(&Vertices[vba]);
        for(sInt k=0;k<4;k++)
          mf->Vertex[k] = mvi+k;

        mv[0].Normal.Init(2,sMin(vaa,vba),sMax(vaa,vba));
        mv[3].Normal.Init(2,sMin(vaa,vba),sMax(vaa,vba));
        mv[1].Normal.Init(2,sMin(vab,vbb),sMax(vab,vbb));
        mv[2].Normal.Init(2,sMin(vab,vbb),sMax(vab,vbb));
      }
    }
  }

  // insert corner bevels.
  // find loops of at least 3 faces, and we only need each loop once!

  sArray<sInt> loop;
  for(sInt i=0;i<oldmaxface;i++)
  {
    for(sInt j=0;j<Faces[i].Count;j++)
    {
      sInt n = OutgoingEdge(i,j);
      sInt start = n;
      loop.Clear();
      sInt error = 0;

      while(!error)
      {
        sInt vert = StartVertex(n);
        if(!sFind(loop,vert))
          loop.AddTail(vert);
        sInt m = NextVertEdge(conn,n);
        if(m==start)    // loop closed!
          break;
        if(m<0)         // dead end
          error = 1;
        if(m/4<i)       // not the first time we encounter this loop!
          error = 2;
        sVERIFY(loop.GetCount()<100); // really lost
        n = m;
      }
      if(!error && loop.GetCount()>=3)
      {
        sInt max = loop.GetCount();
        sInt mvi = Vertices.GetCount();
        mv = Vertices.AddMany(max);
        sInt cl = Faces[i].Cluster;

        for(sInt i=0;i<max;i++)
        {
          mv[i].CopyFrom(&Vertices[loop[i]]);
          mv[i].Normal.Init(3,i,j);
        }

        if(max<=4)
        {
          mf = Faces.AddMany(1);
          mf->Select = 1;
          mf->Count = max;
          mf->Cluster = cl;
          for(sInt i=0;i<max;i++)
            mf->Vertex[i] = mvi+i;
        }
        else
        {
          for(sInt i=2;i<max;i++)
          {
            mf = Faces.AddMany(1);
            mf->Select = 1;
            mf->Count = 3;
            mf->Cluster = cl;
            mf->Vertex[0] = mvi;
            mf->Vertex[1] = mvi+i-1;
            mf->Vertex[2] = mvi+i;
          }
        }
      }
    }
  }

  // done

  delete[] conn;
  delete[] aflags;
  delete[] bflags;
  delete[] newVert;
  delete[] faceNorm;

  Weld(weldTolerance);
  RemoveDegenerateFaces();
  CalcNormalAndTangents();
}

/****************************************************************************/

void Wz4Mesh::Mirror(sBool mx, sBool my, sBool mz, sInt selection, sInt mode)
{
    Wz4MeshVertex *mv;    
    sVector30 m;    
    m.x=mx ? -1.0:1.0f;
    m.y=my ? -1.0:1.0f;
    m.z=mz ? -1.0:1.0f;
    sBool flip=(m.x*m.y*m.z)<0.0f;

    sFORALL(Vertices,mv)
    {
      if(logic(selection,mv->Select))      
      {   
        sVector30 t(mv->Pos);
        t=t*m;
        mv->Pos=(sVector31)t;
       if (flip) mv->Normal=mv->Normal*m;
      }
    }

    if (flip)
    {      
      Wz4MeshFace *fp;    
      sFORALL(Faces,fp)
      {
        if(logic(selection,fp->Select))
          fp->Invert();
      }
    }

  if (mode&1) CalcNormals();
  if (mode&4) CalcTangents();
  Flush();
}

/****************************************************************************/


void Wz4Mesh::TransformRange(sInt rangeMode, sInt mode, sInt selection, sVector2 direction, sVector2 axialRange, sVector31 scaleStart, sVector30 rotateStart, sVector31 translateStart,sVector31 scaleEnd, sVector30 rotateEnd, sVector31 translateEnd)
{
  Wz4MeshVertex *mv;
  sMatrix34 mat0,matStart,matEnd,matt,matInvT;
  sSRT srt;

  // start matrix
  srt.Scale = scaleStart;
  srt.Rotate = rotateStart;
  srt.Translate = translateStart;
  srt.MakeMatrix(matStart);

  // end matrix
  srt.Scale = scaleEnd;
  srt.Rotate = rotateEnd;
  srt.Translate = translateEnd;
  srt.MakeMatrix(matEnd);

  // transformation matrix
  matt.Init();
  srt.Rotate = sVector30(direction.x, 0, direction.y);
  srt.MakeMatrix(matt);

  // clean matrix
  mat0.Init();

  // find min/max range
  sF32 ymin,ymax;
  if(rangeMode == 0)
  {
    // all mesh range
    ymin =  1e+30f;
    ymax = -1e+30f;

    sFORALL(Vertices,mv)
    {
      if(logic(selection,mv->Select))
      {
        sF32 t = mv->Pos.x*matt.i.y + mv->Pos.y*matt.j.y + mv->Pos.z*matt.k.y;
        ymin = sMin(ymin,t);
        ymax = sMax(ymax,t);
      }
    }
  }
  else
  {
    // range defined by axialRange
    ymin = axialRange.y;
    ymax = axialRange.x;
  }

  sF32 yscale = 1.0f / (ymax - ymin);

  sFORALL(Vertices,mv)
  {
    if(logic(selection,mv->Select))
    {
      sF32 t = (mv->Pos.x*matt.i.y + mv->Pos.y*matt.j.y + mv->Pos.z*matt.k.y - ymin) * yscale;

      switch(mode)
      {
      case 1: // smooth
        t = t * t * (3.0f - 2.0f * t);
        break;

      case 2: // tent
        t = 1.0f - fabs(t - 0.5f) * 2.0f;
        break;

      case 3: // tent smooth
        t = t * t * (16.0f + t * (16.0f * t - 32.0f));
        break;
      }

      t = sFade(t,1.0f,0.0f);

      mat0.Fade(t, matStart, matEnd);

      matInvT = mat0;
      matInvT.Invert3();
      matInvT.Trans3();

      mv->Transform(mat0, matInvT);
    }
  }

  Flush();
}

/****************************************************************************/

static sU32 GetWeldBucket(sInt x,sInt y,sInt z)
{
  sU32 magic1 = 0x8da6b343; // one prime
  sU32 magic2 = 0xd8163841; // another prime
  sU32 magic3 = 0x5bd1e995; // and another.

  return magic1*sU32(x) + magic2*sU32(y) + magic3*sU32(z);
}

void Wz4Mesh::Weld(sF32 weldEpsilon)
{
  sF32 cellSize = weldEpsilon * 4.0f;
  sF32 weldEpsilonSq = weldEpsilon * weldEpsilon;

  sInt nVerts = Vertices.GetCount();
  sInt nOutVerts = 0;

  sInt hashSize = sMax(256,(1<<(sFindLowerPower(nVerts))) >> 4);
  sInt hashMask = hashSize - 1;

  sInt *first = new sInt[hashSize];
  sInt *next = new sInt[nVerts];

  for(sInt i=0;i<hashSize;i++)
    first[i] = -1;

  // weld all vertices
  Wz4MeshVertex *mv;
  sFORALL(Vertices,mv)
  {
    // compute cell coordinates of bounding box of welding neighborhood
    sInt minX = sInt((mv->Pos.x - weldEpsilon) / cellSize);
    sInt maxX = sInt((mv->Pos.x + weldEpsilon) / cellSize);
    sInt minY = sInt((mv->Pos.y - weldEpsilon) / cellSize);
    sInt maxY = sInt((mv->Pos.y + weldEpsilon) / cellSize);
    sInt minZ = sInt((mv->Pos.z - weldEpsilon) / cellSize);
    sInt maxZ = sInt((mv->Pos.z + weldEpsilon) / cellSize);

    // to make sure we don't visit buckets twice
    sU32 prevBucket[8];
    sInt nPrevBuckets = 0;
    sInt sourcePos = -1;

    for(sInt x=minX;x<=maxX;x++)
    {
      for(sInt y=minY;y<=maxY;y++)
      {
        for(sInt z=minZ;z<=maxZ;z++)
        {
          sU32 bucket = GetWeldBucket(x,y,z) & hashMask;
          for(sInt i=0;i<nPrevBuckets;i++)
            if(bucket == prevBucket[i])
              goto nextcell;

          prevBucket[nPrevBuckets++] = bucket;

          // is mv close to one of the vertices in this bucket?
          for(sInt v=first[bucket];v!=-1;v=next[v])
          {
            if((Vertices[v].Pos - mv->Pos).LengthSq() < weldEpsilonSq) // matches
            {
              sourcePos = v;
              goto gotone;
            }
          }

nextcell:
          ;
        }
      }
    }

gotone:
    if(sourcePos != -1) // actually weld (position only!)
      mv->Pos = Vertices[sourcePos].Pos;
    else
    {
      // not found, add to bucket
      sInt x = sInt(mv->Pos.x / cellSize);
      sInt y = sInt(mv->Pos.y / cellSize);
      sInt z = sInt(mv->Pos.z / cellSize);
      sU32 bucket = GetWeldBucket(x,y,z) & hashMask;

      next[_i] = first[bucket];
      first[bucket] = _i;
      nOutVerts++;
    }
  }

  delete[] first;
  delete[] next;

  sDPrintF(L"Weld: %d/%d vertices are unique.\n",nOutVerts,nVerts);

  // we may have created some double vertices, kill them.
  MergeVertices();
}

/****************************************************************************/
/***                                                                      ***/
/***   Painting                                                           ***/
/***                                                                      ***/
/****************************************************************************/

struct WireFormat
{
  sVector31 Pos;
  sVector30 Normal;
  sU32 Color;

  void Init(const sVector31 &pos,sU32 col) { Pos = pos; Normal.Init(0,0,0); Color = col; }
  void Init(const sVector31 &pos,const sVector30 &norm,sU32 col) { Pos = pos; Normal = norm; Color = col; }
};

void Wz4Mesh::ChargeWire(sVertexFormatHandle *fmt)
{
  sInt selface=0;
  sInt selvert=0;
  WireFormat *vp;
  Wz4MeshVertex *mv;
  sU32 *ip;
  Wz4MeshFace *fp;

  // lines

  if(Vertices.GetCount()==0 || Faces.GetCount()==0)
    return;

  if(1)
  {
    WireGeoLines = new sGeometry;
    WireGeoLines->Init(sGF_LINELIST|sGF_INDEX32,fmt);
    WireGeoLines->BeginLoadVB(Vertices.GetCount(),sGD_STATIC,&vp);
    sVector30 push;
    push.Init(0,0,1.0f);
    sFORALL(Vertices,mv)
    {
      if(mv->Select>0.0f) selvert++;
      vp->Init(mv->Pos,push,0xffffffff);
      vp++;
    }
    WireGeoLines->EndLoadVB();

    sInt ic = 0;
    sFORALL(Faces,fp)
      ic += fp->Count*2;
    WireGeoLines->BeginLoadIB(ic,sGD_STATIC,&ip);
    sFORALL(Faces,fp)
    {
      if(fp->Select>0.0f)
        selface+=(fp->Count-2);
      for(sInt i=0;i<fp->Count;i++)
      {
        ip[0] = fp->Vertex[i];
        ip[1] = fp->Vertex[(i+1)%fp->Count];
        ip+=2;
      }
    }
    WireGeoLines->EndLoadIB();
  }

  // vertices

  if(selvert>0)
  {
    WireGeoVertex = new sGeometry;
    WireGeoVertex->Init(sGF_QUADLIST,fmt);

    sVector30 n[4];
    n[0].Init(-1,-1,0);
    n[1].Init(-1, 1,0);
    n[2].Init( 1, 1,0);
    n[3].Init( 1,-1,0);

    WireGeoVertex->BeginLoadVB(selvert*4,sGD_STATIC,&vp);
    sFORALL(Vertices,mv)
    {
      if(mv->Select>0.0f)
      {
        sU32 col=sColorFade(0x00ff0000,0xffffff00,mv->Select);
        vp[0].Init(mv->Pos,n[0],col);
        vp[1].Init(mv->Pos,n[1],col);
        vp[2].Init(mv->Pos,n[2],col);
        vp[3].Init(mv->Pos,n[3],col);
        vp+=4;
      }
    }
    WireGeoVertex->EndLoadVB();
  }

  // faces

  if(selface>0)
  {
    sInt vc = 0;
    sFORALL(Vertices,mv)
      mv->Temp = -1;

    struct sfvtx
    {
      sInt Vtx;
      sF32 Select;
    } *map = new sfvtx[Vertices.GetCount()];

    WireGeoFaces = new sGeometry;
    WireGeoFaces->Init(sGF_TRILIST|sGF_INDEX32,fmt);

    WireGeoFaces->BeginLoadIB(selface*3,sGD_STATIC,&ip);
    sFORALL(Faces,fp)
    {
      if(fp->Select>0.0f)
      {
        for(sInt i=0;i<fp->Count;i++)
        {
          sInt index = fp->Vertex[i];
          if(Vertices[index].Temp==-1)
          {
            Vertices[index].Temp = vc;
            map[vc].Vtx = index;
            map[vc].Select = fp->Select;
            vc++;
          }
        }
        for(sInt i=2;i<fp->Count;i++)
        {
          ip[0] = Vertices[fp->Vertex[0  ]].Temp;
          ip[1] = Vertices[fp->Vertex[i-1]].Temp;
          ip[2] = Vertices[fp->Vertex[i  ]].Temp;
          ip+=3;
        }
      }
    }
    WireGeoFaces->EndLoadIB();

    WireGeoFaces->BeginLoadVB(vc,sGD_STATIC,&vp);
    for(sInt i=0;i<vc;i++)
    {
      sU32 col=sColorFade(0x00404000,0xffc08000,map[i].Select);
      vp[i].Init(Vertices[map[i].Vtx].Pos,col);
    }
    WireGeoFaces->EndLoadVB();
 
    delete[] map;
  }
}

/****************************************************************************/

void Wz4Mesh::ChargeSolid(sInt flags)
{
  Wz4MeshCluster *cl;
  Wz4MeshVertex *mv;
  Wz4MeshFace *mf;
  Wz4ChunkPhysics *chunk;
  sU32 *ip32;
  sU16 *ip16;
  sInt *jointtemp=0;
  sInt *firstindex=0;

#if WZ4ONLYONEGEO
  sInt geoindex = 0;
#else
  sInt geoindex = ((flags & sRF_TARGET_MASK) == sRF_TARGET_MAIN) ? 0 : 1;
#endif
  if(Clusters.GetCount()==0) return;
  if(Clusters[0]->Geo[geoindex]) return;
  ChargeBBox();

  sInt ccount = Clusters.GetCount();
  sInt fcount = Faces.GetCount();
  sInt vcount = Vertices.GetCount();

  sInt njoints = Skeleton ? Skeleton->Joints.GetCount() : Chunks.GetCount();
  jointtemp = new sInt[njoints];

  if(Chunks.GetCount()>0)
  {
    firstindex = new sInt[fcount];
    for(sInt i=0;i<fcount;i++)
      firstindex[i] = 0;
  }

  // build linked list of all faces in cluster
  sInt *firstFaceInCluster = new sInt[ccount];
  sInt *nextFaceInCluster = new sInt[fcount];
  for(sInt i=0;i<ccount;i++)
    firstFaceInCluster[i] = -1;

  for(sInt i=fcount-1;i>=0;i--)
  {
    sInt cl = Faces[i].Cluster;
    sVERIFY(cl >= 0 && cl < ccount);
    nextFaceInCluster[i] = firstFaceInCluster[cl];
    firstFaceInCluster[cl] = i;
  }

  // set up per-vertex info
  sInt *vertseen = new sInt[vcount];
  sInt *map = new sInt[vcount];
  sInt *revmap = new sInt[vcount];
  sSetMem(vertseen,0xff,vcount*sizeof(sInt)); // clear "last seen in cluster" index for all vertices

  sFORALL(Clusters,cl)
  {
    sInt cli = _i;
    
    sVERIFY(!cl->Geo[geoindex]);

    cl->ChunkStart = 0;
    cl->ChunkEnd = 0;
    cl->Matrices.Clear();
    for(sInt i=0;i<njoints;i++)
      jointtemp[i] = -1;

    // count indices

    sInt ic=0;

    for(sInt _i=firstFaceInCluster[cli];_i>=0;_i=nextFaceInCluster[_i])
    {
      mf = &Faces[_i];
      if(firstindex)
        firstindex[_i] = ic;
      ic += (mf->Count-2)*3;
    }

    if(ic>0)
    {
      // build index list

      sInt *il = new sInt[ic];
      sInt ili = 0;
      sFORALL(Faces,mf)
      {
        if(mf->Cluster==cli)
        {
          for(sInt i=2;i<mf->Count;i++)
          {
            il[ili++] = mf->Vertex[0]; 
            il[ili++] = mf->Vertex[i-1]; 
            il[ili++] = mf->Vertex[i]; 
          }
        }
      }
      sVERIFY(ili==ic);

      // optimize index order here. do not optimize in the presence of chunks
      // seems not to be important for performance. strangely. although it works.
      // switch it on in player, but don't make the wz4 slower!

      if(!firstindex && Doc->IsPlayer)
      {
//        DumpCacheEfficiency(il,ic);
        OptimizeIndexOrder(il,ic,Vertices.GetCount());
//        DumpCacheEfficiency(il,ic);
      }

      // build vertex list

      sInt vc=0;
      for(sInt i=0;i<ic;i++)
      {
        sInt vi = il[i];
        if(vertseen[vi] != cli) // vertex not seen in this cluster yet?
        {
          vertseen[vi] = cli;
          map[vi] = vc; // assign next free index to this vertex
          revmap[vc++] = vi;
        }
      }
      
      // build vertex buffers

      sGeometry *geo = new sGeometry;
      cl->Geo[geoindex] = geo;
      sInt renderflags = (njoints ? sRF_MATRIX_BONE : sRF_MATRIX_ONE) | (geoindex ? sRF_TARGET_ZNORMAL : sRF_TARGET_MAIN);
      sVertexFormatHandle *hnd = (cl->Mtrl ? cl->Mtrl : Wz4MeshType->DefaultMtrl)->GetFormatHandle(renderflags);
      const sU32 *descdata = hnd->GetDesc();

      sU32 *vp;
      sF32 *fp;

      if(ic>65534)
        cl->IndexSize = sGF_INDEX32;
      else
        cl->IndexSize = sGF_INDEX16;

      geo->Init(sGF_TRILIST|cl->IndexSize,hnd);

      geo->BeginLoadVB(vc,sGD_STATIC,&vp);
      for(sInt i=0;i<vc;i++)
      {
        const sU32 *desc = descdata;
        mv = &Vertices[revmap[i]];

        while(*desc)
        {
          switch(*desc)
          {
          case sVF_POSITION|sVF_F3:
            fp = (sF32 *) vp;
            fp[0] = mv->Pos.x;
            fp[1] = mv->Pos.y;
            fp[2] = mv->Pos.z;
            vp+=3;
            break;
          case sVF_NORMAL|sVF_F3:
            fp = (sF32 *) vp;
            fp[0] = mv->Normal.x;
            fp[1] = mv->Normal.y;
            fp[2] = mv->Normal.z;
            vp+=3;
            break;
          case sVF_NORMAL|sVF_I4:
            {
              sU8 nx = sClamp(sInt(mv->Normal.x*127+128),0,254);
              sU8 ny = sClamp(sInt(mv->Normal.y*127+128),0,254);
              sU8 nz = sClamp(sInt(mv->Normal.z*127+128),0,254);
            
              *vp++ = (nx)|(ny<<8)|(nz<<16);
            }
            break;
          case sVF_TANGENT|sVF_F3:
            fp = (sF32 *) vp;
            fp[0] = mv->Tangent.x;
            fp[1] = mv->Tangent.y;
            fp[2] = mv->Tangent.z;
            vp+=3;
            break;
          case sVF_TANGENT|sVF_F4:
            fp = (sF32 *) vp;
            fp[0] = mv->Tangent.x;
            fp[1] = mv->Tangent.y;
            fp[2] = mv->Tangent.z;
            fp[3] = mv->BiSign;
            vp+=4;
            break;
          case sVF_BONEINDEX|sVF_I4:
            {
              sInt ind[4];
              for(sInt i=0;i<4;i++)
              {
                sInt mi = mv->Index[i];
                if(njoints && mi >= 0)
                {
                  if(jointtemp[mi]==-1)
                  {
                    jointtemp[mi] = cl->Matrices.GetCount();
                    *cl->Matrices.AddMany(1) = mi;
                    if(jointtemp[mi] == 74) // == so we only get one warning per cluster
                      sDPrintF(L"ChaosMesh warning: too many matrices referenced in cluster %d\n",cli);
                  }
                  mi = jointtemp[mi];
                }
                ind[i] = mi;
              }

              sU32 i0 = sMax<sInt>(ind[0],0)*3;
              sU32 i1 = sMax<sInt>(ind[1],0)*3;
              sU32 i2 = sMax<sInt>(ind[2],0)*3;
              sU32 i3 = sMax<sInt>(ind[3],0)*3;
              *vp++ = i0 | (i1<<8) | (i2<<16) | (i3<<24);
            }
            break;
          case sVF_BONEWEIGHT|sVF_I4:
            {
              sU32 w1 = (mv->Weight[1]*0.5f+0.5f)*254;
              sU32 w2 = (mv->Weight[2]*0.5f+0.5f)*254;
              sU32 w3 = (mv->Weight[3]*0.5f+0.5f)*254;
              sU32 w0 = 254-(w1-127)-(w2-127)-(w3-127);
              *vp++ = w0 | (w1<<8) | (w2<<16) | (w3<<24);
            }
            break;
          case sVF_COLOR0|sVF_C4:
#if !WZ4MESH_LOWMEM
            *vp++ = mv->Color0;
#else
            *vp++ = 0xffffffff;
#endif
            break;
          case sVF_COLOR1|sVF_C4:
#if !WZ4MESH_LOWMEM
            *vp++ = mv->Color1;
#else
            *vp++ = 0x00000000;
#endif
            break;
          case sVF_UV0|sVF_F2:
            fp = (sF32 *) vp;
            fp[0] = mv->U0;
            fp[1] = mv->V0;
            vp+=2;
            break;
          case sVF_UV1|sVF_F2:
            fp = (sF32 *) vp;
            fp[0] = mv->U1;
            fp[1] = mv->V1;
            vp+=2;
            break;
          default:
            sFatal(L"unknown vertex format");
          }
          desc++;
        }
      }
      geo->EndLoadVB();

      // build index buffers

      if(cl->IndexSize==sGF_INDEX32)
      {
        geo->BeginLoadIB(ic,sGD_STATIC,&ip32);
        for(sInt i=0;i<ic;i++)
          ip32[i] = map[il[i]];
        geo->EndLoadIB();
      }
      else
      {
        geo->BeginLoadIB(ic,sGD_STATIC,&ip16);
        for(sInt i=0;i<ic;i++)
          ip16[i] = map[il[i]];
        geo->EndLoadIB();
      }

      // clean up

      delete[] il;
    }
    if(cl->Matrices.GetCount() > 74)
    {
      sDPrintF(L"final matrix count for cluster %d: %d\n",cli,cl->Matrices.GetCount());
      cl->Matrices.Resize(74);
    }
  }
  sFORALL(Chunks,chunk)
  {
    if (chunk->FirstFace<Faces.GetCount())
    {
      chunk->FirstIndex = firstindex[chunk->FirstFace];
      sInt n = Faces[chunk->FirstFace].Cluster; 
      if(Clusters[n]->ChunkEnd==0)
        Clusters[n]->ChunkStart = _i;
      Clusters[n]->ChunkEnd = _i+1;
    }
  }

  delete[] firstFaceInCluster;
  delete[] nextFaceInCluster;
  delete[] firstindex;
  delete[] vertseen;
  delete[] map;
  delete[] revmap;
  delete[] jointtemp;

//  sDPrintF(L"charge: %5f - %9d / %9d\n",100.0f*Vertices.GetCount()/Vertices.GetSize(),Vertices.GetCount(),Vertices.GetSize());
}


void Wz4Mesh::ChargeBBox()
{
  if(!BBoxValid)
  {
    sInt ccount = Clusters.GetCount();
    sInt fcount = Faces.GetCount();

    sInt *firstFaceInCluster = new sInt[ccount];
    sInt *nextFaceInCluster = new sInt[fcount];

    // build linked list: which faces are in which cluster
    for(sInt i=0;i<ccount;i++)
      firstFaceInCluster[i] = -1;

    for(sInt i=fcount-1;i>=0;i--)
    {
      sInt cl = Faces[i].Cluster;
      sVERIFY(cl >= 0 && cl < ccount);
      nextFaceInCluster[i] = firstFaceInCluster[cl];
      firstFaceInCluster[cl] = i;
    }

    // build bbox per cluster
    for(sInt cli=0;cli<ccount;cli++)
    {
      sAABBox bound;

      for(sInt i=firstFaceInCluster[cli];i>=0;i=nextFaceInCluster[i])
      {
        Wz4MeshFace *mf = &Faces[i];
        for(sInt j=0;j<mf->Count;j++)
          bound.Add(Vertices[mf->Vertex[j]].Pos);
      }

      Clusters[cli]->Bounds.Init(bound);
    }

    delete[] firstFaceInCluster;
    delete[] nextFaceInCluster;
    BBoxValid = 1;
  }
}

// calling this tells the system that you are only interested in the vertex/index buffers,
// and do not need the fat vertices any more.
// please call when holding only ONE reference!

void Wz4Mesh::Charge()
{
  if(Doc->IsPlayer)
  {
    ChargeCount++;
    ChargeBBox();
    ChargeSolid(sRF_TARGET_ZONLY);
    ChargeSolid(sRF_TARGET_MAIN);
    if(RefCount-ChargeCount==1 && DontClearVertices==0)
    {
      Vertices.Reset();
      Faces.Reset();
      sDPrintF(L"%08x deleting\n",this);
    }
    else
      sDPrintF(L"%08x refcount %d chargecount %d\n",this,RefCount,ChargeCount);
  }
}

/****************************************************************************/

// render single mesh or single boned mesh

void Wz4Mesh::BeforeFrame(sInt lightenv,sInt matcount,const sMatrix34CM *mat)
{
  Wz4MeshCluster *cl;

  if(!BBoxValid)
    ChargeBBox();
  sFORALL(Clusters,cl)
  {
    if(cl->Mtrl)
    {
      cl->Mtrl->BeforeFrame(lightenv,1,&cl->Bounds,matcount,mat);
    }
  }
}


void Wz4Mesh::Render(sInt flags,sInt index,const sMatrix34CM *mat,sF32 time,const sFrustum &fr)
{
  Wz4MeshCluster *cl;
  sVERIFY((flags & sRF_MATRIX_MASK)==0); // do not set matrix mask
  sInt geoindex = 0;


  switch(flags & sRF_TARGET_MASK)
  {
  case sRF_TARGET_WIRE:
    if(!WireGeoLines)
      ChargeWire(Wz4MtrlType->GetDefaultFormat(flags | sRF_MATRIX_ONE));
    
    Wz4MtrlType->SetDefaultShader(flags | sRF_MATRIX_ONE,mat,0,0,0,0);
    if(WireGeoFaces)
      WireGeoFaces->Draw();
    if(WireGeoLines)
      WireGeoLines->Draw();
    if(WireGeoVertex)
      WireGeoVertex->Draw();
    break;

  case sRF_TARGET_ZONLY:
  case sRF_TARGET_ZNORMAL:
  case sRF_TARGET_DIST:
#if !WZ4ONLYONEGEO
    geoindex = 1;
#endif
  case sRF_TARGET_MAIN:
    sMatrix34 *bonemat = 0;
    sMatrix34CM *basemat = 0;
    sBool nobbox = 0;
    if(Skeleton)
    {
      sInt bc = Skeleton->Joints.GetCount();
      bonemat = sALLOCSTACK(sMatrix34,bc);
      basemat = sALLOCSTACK(sMatrix34CM,bc);
      
      Skeleton->EvaluateCM(time,bonemat,basemat);

      flags |= sRF_MATRIX_BONE;
      nobbox = 1;
    }
    else
    {
      flags |= sRF_MATRIX_ONE;
    }

    ChargeSolid(flags);
    sFrustum fri;
    sMatrix34CM dummy;
    fri.Transform(fr,*mat);
    sFORALL(Clusters,cl)
    {
      if(cl->Geo[geoindex] && (Doc->IsCacheWarmup || nobbox || fri.IsInside(cl->Bounds)))
      {
        Wz4Mtrl *mtrl = cl->Mtrl ? cl->Mtrl : Wz4MeshType->DefaultMtrl;
        if(mtrl->SkipPhase(flags,index)) continue;

        if(Skeleton)
          mtrl->Set(flags,index,mat,cl->Matrices.GetCount(),basemat,cl->Matrices.GetData());
        else
          mtrl->Set(flags,index,mat,0,0,0);

        cl->Geo[geoindex]->Draw();
      }
    }
    break;
  }
}

/****************************************************************************/

void Wz4Mesh::RenderInst(sInt flags,sInt index,sInt mc,const sMatrix34CM *mats,sU32 *colors)
{
  Wz4MeshCluster *cl;
  sVERIFY((flags & sRF_MATRIX_MASK)==0); // do not set matrix mask
  sInt geoindex = 0;

  if(Skeleton)      // can't do this. please use bakeanim!
    return;

  sGeometry *ig = 0;
  if(colors)
  {
    flags |= sRF_MATRIX_INSTPLUS;
    if(InstancePlusGeo==0)
    {
      InstancePlusGeo = new sGeometry();
      InstancePlusGeo->Init(sGF_TRILIST|sGF_INSTANCES,sVertexFormatInstancePlus);
    }
    ig = InstancePlusGeo;

    sVertexInstancePlus *vp;
    ig->BeginLoadVB(mc,sGD_STREAM,&vp,1);
    for(sInt i=0;i<mc;i++)
    {
      vp[i].Matrix = mats[i];
      vp[i].Plus.InitColor(colors[i]);
    }
    ig->EndLoadVB(-1,1);
    geoindex |= 2;
  }
  else
  {
    flags |= sRF_MATRIX_INSTANCE;
    if(InstanceGeo==0)
    {
      InstanceGeo = new sGeometry();
      InstanceGeo->Init(sGF_TRILIST|sGF_INSTANCES,sVertexFormatInstance);
    }
    ig = InstanceGeo;

    sMatrix34CM *vp;
    ig->BeginLoadVB(mc,sGD_STREAM,&vp,1);
    sCopyMem(vp,mats,sizeof(sMatrix34CM)*mc);
    ig->EndLoadVB(-1,1);
  }


  switch(flags & sRF_TARGET_MASK)
  {
  case sRF_TARGET_ZNORMAL:
  case sRF_TARGET_ZONLY:
  case sRF_TARGET_DIST:
    geoindex |= 1;
  case sRF_TARGET_MAIN:
    ChargeSolid(flags);
    sFORALL(Clusters,cl)
    {
      if(cl->Geo[geoindex&1])
      {
        Wz4Mtrl *mtrl = cl->Mtrl ? cl->Mtrl : Wz4MeshType->DefaultMtrl;
        if(mtrl->SkipPhase(flags,index)) continue;

        if(cl->InstanceGeo[geoindex]==0)
        {
          cl->InstanceGeo[geoindex] = new sGeometry;
          sVertexFormatHandle *fmt = mtrl->GetFormatHandle(flags);
          if(fmt==0)
            fmt = mtrl->GetFormatHandle(flags);
          cl->InstanceGeo[geoindex]->Init(sGF_TRILIST|cl->IndexSize|sGF_INSTANCES,fmt);
        }
        cl->InstanceGeo[geoindex]->Merge(cl->Geo[geoindex&1],ig);

        mtrl->Set(flags,index,0,0,0,0);

        cl->InstanceGeo[geoindex]->Draw(0,0,mc,0);
      }
    }
    break;
  case sRF_TARGET_WIRE:
    if(!WireGeoLines)
      ChargeWire(Wz4MtrlType->GetDefaultFormat(flags));
    
//    if(WireGeoFaces)
//      WireGeoFaces->Draw();
    if(WireGeoLines)
    {
      if(!WireGeoInst)
      {
        WireGeoInst = new sGeometry;
        WireGeoInst->Init(sGF_LINELIST|sGF_INDEX32|sGF_INSTANCES,Wz4MtrlType->GetDefaultFormat(flags));
      }
      WireGeoInst->Merge(WireGeoLines,InstanceGeo);
      Wz4MtrlType->SetDefaultShader(flags,0,0,0,0,0);
      WireGeoInst->Draw(0,0,mc,0);
    }
//    if(WireGeoVertex)
//      WireGeoVertex->Draw();
    break;
  }
}

/****************************************************************************/

void Wz4Mesh::RenderBone(sInt flags,sInt index,sInt mc,const sMatrix34CM *mats,sInt chunks,const sMatrix34CM *mat)
{
  Wz4MeshCluster *cl;
  sVERIFY((flags & sRF_MATRIX_MASK)==0);
  sMatrix34CM dummy;
  dummy.Init();
  sInt geoindex = 0;

  if(chunks==0) return;

  switch(flags & sRF_TARGET_MASK)
  {
  case sRF_TARGET_WIRE:
    if(!WireGeoLines)
      ChargeWire(Wz4MtrlType->GetDefaultFormat(flags | sRF_MATRIX_ONE));

    Wz4MtrlType->SetDefaultShader(flags | sRF_MATRIX_ONE,mats,0,0,0,0);
    if(WireGeoFaces)
      WireGeoFaces->Draw();
    WireGeoLines->Draw();
    if(WireGeoVertex)
      WireGeoVertex->Draw();
    break;

  case sRF_TARGET_ZNORMAL:
  case sRF_TARGET_ZONLY:
  case sRF_TARGET_DIST:
    geoindex = 1;
  case sRF_TARGET_MAIN:
    flags |= sRF_MATRIX_BONE;

    ChargeSolid(flags);
    Wz4Mtrl *lastmtrl=0;
    sFORALL(Clusters,cl)
    {
      if(cl->Geo[geoindex])
      {
        Wz4Mtrl *mtrl = cl->Mtrl ? cl->Mtrl : Wz4MeshType->DefaultMtrl;
        if(mtrl->SkipPhase(flags,index)) continue;
//        if (mtrl!=lastmtrl)
        {
          mtrl->Set(flags,index,mat,cl->Matrices.GetCount(),mats,cl->Matrices.GetData());
          lastmtrl=mtrl;
        }
        
        if(chunks>0 && chunks<Chunks.GetCount())
        {
          if(chunks>=cl->ChunkEnd)
          {
            cl->Geo[geoindex]->Draw();
          }
          else if(chunks>cl->ChunkStart && chunks<cl->ChunkEnd)
          {
            sDrawRange ir;
            ir.Start = 0;
            ir.End = Chunks[chunks].FirstIndex;
            if (ir.End>ir.Start)
              cl->Geo[geoindex]->Draw(&ir,1);
          }
        }
        else
        {
          cl->Geo[geoindex]->Draw();
        }
      }
    }
    break;
  }
}

/****************************************************************************/

void Wz4Mesh::RenderBoneInst(sInt flags,sInt index,sInt mc,const sMatrix34CM *mats,
                             sInt ic,const sMatrix34CM *imats)
{
  Wz4MeshCluster *cl;
  sVERIFY((flags & sRF_MATRIX_MASK)==0);
  sMatrix34CM dummy;
  dummy.Init();
  sInt geoindex = 0;

  if(InstanceGeo==0)
  {
    InstanceGeo = new sGeometry();
    InstanceGeo->Init(sGF_TRILIST|sGF_INSTANCES,sVertexFormatInstance);
  }

  sMatrix34CM *vp;

  InstanceGeo->BeginLoadVB(ic,sGD_STREAM,&vp,1);
  sCopyMem(vp,imats,sizeof(sMatrix34CM)*ic);
  InstanceGeo->EndLoadVB(-1,1);

  switch(flags & sRF_TARGET_MASK)
  {
  case sRF_TARGET_WIRE:
    if(!WireGeoLines)
      ChargeWire(Wz4MtrlType->GetDefaultFormat(flags | sRF_MATRIX_ONE));

    Wz4MtrlType->SetDefaultShader(flags | sRF_MATRIX_ONE,mats,0,0,0,0);
    if(WireGeoFaces)
      WireGeoFaces->Draw();
    WireGeoLines->Draw();
    if(WireGeoVertex)
      WireGeoVertex->Draw();
    break;

  case sRF_TARGET_ZNORMAL:
  case sRF_TARGET_ZONLY:
  case sRF_TARGET_DIST:
    geoindex = 1;
  case sRF_TARGET_MAIN:
    flags |= sRF_MATRIX_BONEINST;

    ChargeSolid(flags);
    Wz4Mtrl *lastmtrl=0;
    sFORALL(Clusters,cl)
    {
      if(cl->Geo[geoindex])
      {
        Wz4Mtrl *mtrl = cl->Mtrl ? cl->Mtrl : Wz4MeshType->DefaultMtrl;
        if(mtrl->SkipPhase(flags,index)) continue;

        if(cl->InstanceGeo[geoindex]==0)
        {
          cl->InstanceGeo[geoindex] = new sGeometry;
          cl->InstanceGeo[geoindex]->Init(sGF_TRILIST|cl->IndexSize|sGF_INSTANCES,mtrl->GetFormatHandle(flags));
        }
        cl->InstanceGeo[geoindex]->Merge(cl->Geo[geoindex],InstanceGeo);
     //   if (mtrl!=lastmtrl)
        {
          mtrl->Set(flags,index,0,cl->Matrices.GetCount(),mats,cl->Matrices.GetData());
          lastmtrl=mtrl;
        }
        
        cl->InstanceGeo[geoindex]->Draw(0,0,ic,0);
      }
    }
    break;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Generators                                                         ***/
/***                                                                      ***/
/****************************************************************************/

void Wz4Mesh::MakeGrid(sInt tx,sInt ty)
{
  sVERIFY(IsEmpty());

  AddDefaultCluster();
  Wz4MeshVertex *mv = Vertices.AddMany((tx+1)*(ty+1));
  Wz4MeshFace *mf = Faces.AddMany(tx*ty);

  for(sInt y=0;y<=ty;y++)
  {
    for(sInt x=0;x<=tx;x++)
    {
      mv->Init();
      mv->Pos.x = x;
      mv->Pos.z = y;
      mv->Normal.x = 0;
      mv->Normal.y = 1;
      mv->Normal.z = 0;
      mv->Tangent.x = 1;
      mv->Tangent.y = 0;
      mv->Tangent.z = 0;
      mv->U0 = sF32(x)/tx;
      mv->V0 = sF32(y)/ty;
      mv++;
    }
  }
  for(sInt y=0;y<ty;y++)
  {
    for(sInt x=0;x<tx;x++)
    {
      mf->Init(4);
      mf->Vertex[0] = (x+0)+(y+0)*(tx+1);
      mf->Vertex[1] = (x+0)+(y+1)*(tx+1);
      mf->Vertex[2] = (x+1)+(y+1)*(tx+1);
      mf->Vertex[3] = (x+1)+(y+0)*(tx+1);
      mf++;
    }
  }
}

/****************************************************************************/

void Wz4Mesh::MakeCube(sInt tx,sInt ty,sInt tz)
{
  Wz4Mesh *mesh[6];
  sMatrix34 mat[6];
  sMatrix34 muv[6];

  for(sInt i=0;i<6;i++)
    mesh[i] = new Wz4Mesh;


  mesh[0]->MakeGrid(tx,tz);
  mat[0].i.Init(-1, 0, 0);
  mat[0].j.Init( 0,-1, 0);
  mat[0].k.Init( 0, 0, 1);
  mat[0].l.Init(tx, 0, 0);
  muv[0].i.Init(-1, 0, 0);
  muv[0].j.Init( 0, 1, 0);
  muv[0].l.Init( 1, 0, 0);

  mesh[1]->MakeGrid(tx,tz);
  mat[1].i.Init(-1, 0, 0);
  mat[1].j.Init( 0, 1, 0);
  mat[1].k.Init( 0, 0,-1);
  mat[1].l.Init(tx,ty,tz);
  muv[1].i.Init(-1, 0, 0);
  muv[1].j.Init( 0, 1, 0);
  muv[1].l.Init( 1, 0, 0);

  mesh[2]->MakeGrid(tx,ty);
  mat[2].i.Init( 1, 0, 0);
  mat[2].j.Init( 0, 0,-1);
  mat[2].k.Init( 0, 1, 0);
  mat[2].l.Init( 0, 0, 0);
  muv[2].i.Init( 1, 0, 0);
  muv[2].j.Init( 0,-1, 0);
  muv[2].l.Init( 0, 1, 0);

  mesh[3]->MakeGrid(tx,ty);
  mat[3].i.Init(-1, 0, 0);
  mat[3].j.Init( 0, 0, 1);
  mat[3].k.Init( 0, 1, 0);
  mat[3].l.Init(tx, 0,tz);
  muv[3].i.Init( 1, 0, 0);
  muv[3].j.Init( 0,-1, 0);
  muv[3].l.Init( 2, 1, 0);

  mesh[4]->MakeGrid(ty,tz);
  mat[4].i.Init( 0, 1, 0);
  mat[4].j.Init(-1, 0, 0);
  mat[4].k.Init( 0, 0, 1);
  mat[4].l.Init( 0, 0, 0);
  muv[4].i.Init( 0,-1, 0);
  muv[4].j.Init(-1, 0, 0);
  muv[4].l.Init( 4, 1, 0);

  mesh[5]->MakeGrid(ty,tz);
  mat[5].i.Init( 0,-1, 0);
  mat[5].j.Init( 1, 0, 0);
  mat[5].k.Init( 0, 0, 1);
  mat[5].l.Init(tx,ty, 0);
  muv[5].i.Init( 0, 1, 0);
  muv[5].j.Init( 1, 0, 0);
  muv[5].l.Init( 1, 0, 0);

  sVERIFY(IsEmpty());

  AddDefaultCluster();
  for(sInt i=0;i<6;i++)
  {
    mesh[i]->Transform(mat[i]);
    mesh[i]->TransformUV(muv[i]);
    Add(mesh[i]);
    mesh[i]->Release();
  }
}

/****************************************************************************/

void Wz4Mesh::MakeTorus(sInt tx,sInt ty,sF32 ri,sF32 ro,sF32 phase,sF32 arc)
{
  sVERIFY(IsEmpty());
  sBool hasarc = 0;
  sF32 fx,fy,px,py;
  sVector31 v;

  if(arc<=0.0f)
  {
    arc = 0.0f;
    hasarc = 1;
  }
  else if(arc>=1.0f)
  {
    arc = 1.0f;
    hasarc = 0;
  }
  else
  {
    hasarc = 1;
  }

  AddDefaultCluster();

  if(arc>0.0f)
  {
    Wz4MeshVertex *mv = Vertices.AddMany((tx+1)*(ty+1));
    Wz4MeshFace *mf = Faces.AddMany(tx*ty);


    for(sInt iy=0;iy<=ty;iy++)
    {
      fy = sF32(iy)/ty*sPI2F;
      py = iy!=ty ? fy : 0;
      py += phase*sPI2F;
      for(sInt ix=0;ix<=tx;ix++)
      {
        fx = sF32(ix)/tx*sPI2F*arc;
        px = ix!=tx || hasarc ? fx : 0;
        v.x = -sFCos(px)*(ro+sFCos(py)*ri);
        v.y = -              sFSin(py)*ri;
        v.z =  sFSin(px)*(ro+sFCos(py)*ri);

        mv->Init(v,sF32(ix)/tx,sF32(iy)/ty);
        mv->Normal.x = px;
        mv->Normal.y = py;
        mv->Normal.z = 0;
        mv++;
      }
    }

    for(sInt iy=0;iy<ty;iy++)
    {
      for(sInt ix=0;ix<tx;ix++)
      {
        mf->Init(4);
        mf->Vertex[0] = (ix+0) + (iy+0)*(tx+1);
        mf->Vertex[1] = (ix+0) + (iy+1)*(tx+1);
        mf->Vertex[2] = (ix+1) + (iy+1)*(tx+1);
        mf->Vertex[3] = (ix+1) + (iy+0)*(tx+1);
        mf++;
      }
    }

    if(hasarc)
    {
      sInt v0 = Vertices.GetCount();
      Wz4MeshVertex *mv = Vertices.AddMany((ty+2)*2);
      Wz4MeshFace *mf = Faces.AddMany(ty*2);

      for(sInt i=0;i<2;i++)
      {
        fx = i*sPI2F*arc;
        px = fx;

        v.x = -sFCos(px)*ro;
        v.y = 0;
        v.z =  sFSin(px)*ro;

        mv->Init(v,0.5f,0.5f);
        mv->Normal.x = i+10;
        mv->Normal.y = 0;
        mv->Normal.z = 0;
        mv++;

        for(sInt iy=0;iy<=ty;iy++)
        {
          fy = sF32(iy)/ty*sPI2F;
          py = iy!=ty ? fy : 0;
          py += phase*sPI2F;

          v = Vertices[iy*(tx+1)+i*tx].Pos;

          mv->Init(v,sFCos(py)*0.5f+0.5f,sFSin(py)*0.5f+0.5f);
          mv->Normal.x = i+10;
          mv->Normal.y = 0;
          mv->Normal.z = 0;
          mv++;
        }
      }

      for(sInt j=0;j<ty;j++)
      {
        mf->Init(3);
        mf->Vertex[0] = v0;
        mf->Vertex[1] = v0+j+2;
        mf->Vertex[2] = v0+j+1;
        mf->Select = 1.0f;
        mf++;
      }
      v0 += ty+2;
      for(sInt j=0;j<ty;j++)
      {
        mf->Init(3);
        mf->Vertex[0] = v0;
        mf->Vertex[1] = v0+j+1;
        mf->Vertex[2] = v0+j+2;
        mf->Select = 1.0f;
        mf++;
      }
    }
  }
}

/****************************************************************************/

void Wz4Mesh::MakeDisc(sInt ty,sF32 ri,sF32 phase,sBool doublesided)
{
  sVERIFY(IsEmpty());
  sVector31 v;

  AddDefaultCluster();

  sInt sides = (doublesided&1) ? 2 : 1;

  Wz4MeshVertex *mv = Vertices.AddMany((ty+2)*sides);
  Wz4MeshFace *mf = Faces.AddMany(ty*sides);

  for(sInt i=0;i<sides;i++)
  {
    v.x = 0;
    v.y = 0;
    v.z = 0;

    mv->Init(v,0.5f,0.5f);
    mv->Normal.x = i+10;
    mv->Normal.y = 0;
    mv->Normal.z = 0;
    mv++;

    for(sInt iy=0;iy<=ty;iy++)
    {
      sF32 fy = sF32(iy)/ty*sPI2F;
      sF32 py = iy!=ty ? fy : 0;
      py += phase*sPI2F;

      v.x = sFSin(py)*ri;
      v.y = 0;
      v.z = sFCos(py)*ri;

      mv->Init(v,sFSin(py)*0.5f+0.5f,-sFCos(py)*0.5f+0.5f);
      mv->Normal.x = i+10;
      mv->Normal.y = 0;
      mv->Normal.z = 0;
      mv++;
    }
  }

  sInt v0 = 0;
  for(sInt j=0;j<ty;j++)
  {
    mf->Init(3);
    mf->Vertex[0] = v0;
    mf->Vertex[1] = v0+j+1;
    mf->Vertex[2] = v0+j+2;
    mf->Select = 1.0f;
    mf++;
  }
  if(sides>1)
  {
    v0 += ty+2;
    for(sInt j=0;j<ty;j++)
    {
      mf->Init(3);
      mf->Vertex[0] = v0;
      mf->Vertex[1] = v0+j+2;
      mf->Vertex[2] = v0+j+1;
      mf->Select = 1.0f;
      mf++;
    }
  }
}

/****************************************************************************/

// tx = arc segments
// ty = horizontal slices (polyons)

void Wz4Mesh::MakeSphere(sInt tx,sInt ty)
{
  sVERIFY(IsEmpty());
  sVERIFY(tx>=3 && ty>=2);

  AddDefaultCluster();
  Wz4MeshVertex *mv = Vertices.AddMany((ty-1)*(tx+1)+2);
  Wz4MeshFace *mf = Faces.AddMany(tx*ty);

  // top/bottom

  mv->Init();
  mv->Pos.Init(0,0.5f,0);
  mv->U0 = 0.5f;
  mv->V0 = 0.0f;
  mv++;
  mv->Init();
  mv->Pos.Init(0,-0.5f,0);
  mv->U0 = 0.5f;
  mv->V0 = 1.0f;
  mv++;

  // vertices

  for(sInt y=0;y<ty-1;y++)
  {
    for(sInt x=0;x<=tx;x++)
    {
      sF32 fy = (y+0.5f)*sPIF/(ty-1);
      sF32 fx = (x!=tx) ? x*sPI2F/tx : 0;
      sVector31 v;
      v.x = -sFSin(fy)*sFSin(fx)*0.5f;
      v.y = sFCos(fy)*0.5f;
      v.z = -sFSin(fy)*sFCos(fx)*0.5f;

      mv->Init();
      mv->Pos = v;
      mv->U0 = 1-sF32(x)/tx;
      mv->V0 = sF32(y+0.5f)/(ty-1);
      mv++;
    }
  }

  // faces around

  for(sInt y0=0;y0<ty-2;y0++)
  {
    for(sInt x0=0;x0<tx;x0++)
    {
      sInt x1 = (x0+1);
      sInt y1 = (y0+1);
      mf->Init(4);
      mf->Vertex[0] = 2+y0*(tx+1)+x0;
      mf->Vertex[1] = 2+y1*(tx+1)+x0;
      mf->Vertex[2] = 2+y1*(tx+1)+x1;
      mf->Vertex[3] = 2+y0*(tx+1)+x1;
      mf++;
    }
  }

  // top & button

  for(sInt x0=0;x0<tx;x0++)
  {
    sInt x1 = (x0+1);

    mf->Init(3);
    mf->Select = 1.0f;
    mf->Vertex[0] = 0;
    mf->Vertex[1] = 2+x0;
    mf->Vertex[2] = 2+x1;
    mf++;

    mf->Init(3);
    mf->Select = 1.0f;
    mf->Vertex[0] = 1;
    mf->Vertex[2] = 2+(ty-2)*(tx+1)+x0;
    mf->Vertex[1] = 2+(ty-2)*(tx+1)+x1;
    mf++;
  }
}

/****************************************************************************/

void Wz4Mesh::MakeCylinder(sInt segments,sInt slices,sInt top,sInt flags)
{
  sVERIFY(IsEmpty());
  slices += 2;
  top += 1;
  sVERIFY(segments>=3 && slices>=3);

  sInt tx = segments;
  sInt ty = slices+top*2;
  AddDefaultCluster();
  Wz4MeshVertex *mv = Vertices.AddMany((ty-1)*(tx+1)+2);
  Wz4MeshFace *mf = Faces.AddMany(tx*(ty-2));

  // top/bottom

  mv->Init();
  mv->Pos.Init(0,1,0);
  mv->U0 = 0.5f;
  mv->V0 = 0.5f;
  mv++;
  mv->Init();
  mv->Pos.Init(0,0,0);
  mv->U0 = 0.5f;
  mv->V0 = 0.5f;
  mv++;

  // vertices

  for(sInt y=0;y<top;y++)
  {
    for(sInt x=0;x<=tx;x++)
    {
      sF32 fy = (y+1.0f)/(top);
      sF32 fx = (x!=tx) ? x*sPI2F/tx : 0;
      sVector31 v;
      v.x = fy*sFSin(fx);
      v.y = 1.0f;
      v.z = fy*sFCos(fx);

      mv->Init();
      mv->Pos = v;
      mv->Normal.x = fx;
      mv->Normal.y = fy;
      mv->Normal.z = 0.1f;
      mv->U0 = v.x*0.5f+0.5f;
      mv->V0 = -v.z*0.5f+0.5f;
      mv++;
    }
  }

  for(sInt y=0;y<slices-1;y++)
  {
    for(sInt x=0;x<=tx;x++)
    {
      sF32 fy = sF32(y)/(slices-2);
      sF32 fx = (x!=tx) ? x*sPI2F/tx : 0;
      sVector31 v;
      v.x = sFSin(fx);
      v.y = 1-fy;
      v.z = sFCos(fx);

      mv->Init();
      mv->Pos = v;
      mv->Normal.x = fx;
      mv->Normal.y = fy;
      mv->Normal.z = 0.2f;
      mv->U0 = 1-sF32(x)/tx;
      mv->V0 = fy;
      mv++;
    }
  }

  for(sInt y=0;y<top;y++)
  {
    for(sInt x=0;x<=tx;x++)
    {
      sF32 fy = sF32(top-y)/(top);
      sF32 fx = (x!=tx) ? x*sPI2F/tx : 0;
      sVector31 v;
      v.x = fy*sFSin(fx);
      v.y = 0.0f;
      v.z = fy*sFCos(fx);

      mv->Init();
      mv->Pos = v;
      mv->Normal.x = fx;
      mv->Normal.y = fy;
      mv->Normal.z = 0.3f;
      mv->U0 = v.x*0.5f+0.5f;
      mv->V0 = v.z*0.5f+0.5f;
      mv++;
    }
  }

  // faces around

  for(sInt y0=0;y0<ty-2;y0++)
  {
    if(y0+1!=top && y0!=ty-2-top)
    {
      sBool istop = (y0<top);
      sBool isbot = (y0>=ty-2-top);
      if((!istop || !(flags & 2)) && (!isbot || !(flags & 4))) 
      {
        for(sInt x0=0;x0<tx;x0++)
        {
          sInt x1 = (x0+1);
          sInt y1 = (y0+1);
          mf->Init(4);
          mf->Select = (istop || isbot)?1.0f:0.0f;
          mf->Vertex[0] = 2+y0*(tx+1)+x0;
          mf->Vertex[1] = 2+y1*(tx+1)+x0;
          mf->Vertex[2] = 2+y1*(tx+1)+x1;
          mf->Vertex[3] = 2+y0*(tx+1)+x1;
          mf++;
        }
      }
    }
  }

  // top & button

  if(!(flags & 2))
  {
    for(sInt x0=0;x0<tx;x0++)
    {
      sInt x1 = (x0+1);

      mf->Init(3);
      mf->Select = 1.0f;
      mf->Vertex[0] = 0;
      mf->Vertex[1] = 2+x0;
      mf->Vertex[2] = 2+x1;
      mf++;
    }
  }
  if(!(flags & 4))
  {
    for(sInt x0=0;x0<tx;x0++)
    {
      sInt x1 = (x0+1);

      mf->Init(3);
      mf->Select = 1.0f;
      mf->Vertex[0] = 1;
      mf->Vertex[2] = 2+(ty-2)*(tx+1)+x0;
      mf->Vertex[1] = 2+(ty-2)*(tx+1)+x1;
      mf++;
    }
  }

  if(flags & 6)
  {
    Faces.Resize(mf-Faces.GetData());
    MergeVertices();
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Font3D op (copy&pasted from Wz3)                                   ***/
/***                                                                      ***/
/****************************************************************************/

#if sPLATFORM==sPLAT_WINDOWS

#pragma comment(lib,"glu32.lib")

struct GLUtesselator;

extern "C"
{
  // Win32
  typedef void *HDC;
  typedef void *HFONT;

  struct FIXED
  {
    union
    {
      struct
      {
        sU16 fract;
        sS16 value;
      };
      sS32 full;
    };
  };

  struct POINTFX
  {
    FIXED x;
    FIXED y;

    sVector31 toVector(sInt xShift,sInt yShift) const
    {
      return sVector31((xShift + x.full / 65536.0f) / 128.0f,(yShift + y.full / 65536.0f) / 128.0f,0.0f);
    }
  };

  struct MAT2
  {
    FIXED eM11,eM12;
    FIXED eM21,eM22;
  };

  struct GLYPHMETRICS
  {
    sU32 gmBlackBoxX,gmBlackBoxY;
    sInt origin[2];
    sS16 gmCellIncX,gmCellIncY;
  };

  struct TTPOLYGONHEADER
  {
    sU32 cb;
    sU32 dwType;
    POINTFX pfxStart;
  };

  struct TTPOLYCURVE
  {
    sU16 wType;
    sU16 cpfx;
    POINTFX apfx[1];
  };

  HDC __stdcall CreateCompatibleDC(HDC hDC);
  HFONT __stdcall CreateFontW(sInt height,sInt width,sInt escape,sInt orient,
    sInt weight,sInt italic,sInt underline,sInt strikeOut,sU32 charSet,
    sU32 outPrecision,sU32 clipPrecision,sU32 quality,sU32 pitchAndFamily,
    const sChar *face);
  HFONT __stdcall SelectObject(HDC hDC,HFONT hFont);
  sU32 __stdcall GetGlyphOutlineW(HDC hDC,sU32 nChar,sU32 fuFormat,
    GLYPHMETRICS *lpgm,sU32 cjBuffer,void *buffer,const MAT2 *lpmat2);
  void __stdcall DeleteObject(HFONT hFont);
  void __stdcall DeleteDC(HDC hDC);

  // GLU
  GLUtesselator * __stdcall gluNewTess();
  void __stdcall gluDeleteTess(GLUtesselator *tess);

  void __stdcall gluTessBeginPolygon(GLUtesselator *tess,void *polygon_data);
  void __stdcall gluTessBeginContour(GLUtesselator *tess);
  void __stdcall gluTessVertex(GLUtesselator *tess,sF64 coords[3],void *data);
  void __stdcall gluTessEndContour(GLUtesselator *tess);
  void __stdcall gluTessEndPolygon(GLUtesselator *tess);
  void __stdcall gluTessNormal(GLUtesselator *tess,sF64 x,sF64 y,sF64 z);
  void __stdcall gluTessCallback(GLUtesselator *tess,sInt which,void (__stdcall *fn)());
  void __stdcall gluTessProperty(GLUtesselator *tess,sInt which,sF64 value);

  typedef void (__stdcall *gluTessCB)(void);

  #define GLU_TESS_BEGIN          100100
  #define GLU_TESS_VERTEX         100101
  #define GLU_TESS_END            100102
  #define GLU_TESS_ERROR          100103
  #define GLU_TESS_EDGE_FLAG      100104
  #define GLU_TESS_COMBINE        100105
  #define GLU_TESS_BEGIN_DATA     100106
  #define GLU_TESS_VERTEX_DATA    100107
  #define GLU_TESS_END_DATA       100108
  #define GLU_TESS_ERROR_DATA     100109
  #define GLU_TESS_EDGE_FLAG_DATA 100110
  #define GLU_TESS_COMBINE_DATA   100111
  
  #define GLU_TESS_WINDING_RULE   100140
  #define GLU_TESS_BOUNDARY_ONLY  100141
  #define GLU_TESS_TOLERANCE      100142
}

static void tess3DAddPoint(GLUtesselator *tess,Wz4Mesh *mesh,const sVector31 &pt)
{
  Wz4MeshVertex *vert = mesh->Vertices.AddMany(1);
  sClear(*vert);
  vert->Pos = pt;
  vert->Normal.z = -1.0f;

  double coords[3];
  coords[0] = pt.x;
  coords[1] = pt.y;
  coords[2] = pt.z;
  gluTessVertex(tess,coords,(void *) sDInt(mesh->Vertices.GetCount() - 1));
}

static void tess3DAddQuadratic(GLUtesselator *tess,Wz4Mesh *mesh,const sVector31 &p1,const sVector31 &p2,sF32 toleranceSq,sInt depth=0)
{
  const sVector31 &p0 = mesh->Vertices.GetTail().Pos;
  sVector30 d = sAverage(p0,p2) - p1;

  if(depth >= 8 || d.LengthSq() <= toleranceSq)
    tess3DAddPoint(tess,mesh,p2);
  else
  {
    sVector31 l = sAverage(p0,p1);
    sVector31 r = sAverage(p1,p2);
    sVector31 m = sAverage(l,r);

    tess3DAddQuadratic(tess,mesh,l,m,toleranceSq,depth+1);
    tess3DAddQuadratic(tess,mesh,r,p2,toleranceSq,depth+1);
  }
}

static void tess3DAddCubic(GLUtesselator *tess,Wz4Mesh *mesh,const sVector31 &p1,const sVector31 &p2,const sVector31 &p3,sF32 toleranceSq,sInt depth=0)
{
  const sVector31 &p0 = mesh->Vertices.GetTail().Pos;
  sVector30 d0 = sAverage(p0,p2) - p1;
  sVector30 d1 = sAverage(p1,p3) - p2;
  if(depth >= 8 || d0.LengthSq() + d1.LengthSq() <= toleranceSq)
    tess3DAddPoint(tess,mesh,p3);
  else
  {
    sVector31 p01 = sAverage(p0,p1);
    sVector31 p12 = sAverage(p1,p2);
    sVector31 p23 = sAverage(p2,p3);
    
    sVector31 p012 = sAverage(p01,p12);
    sVector31 p123 = sAverage(p12,p23);

    sVector31 p0123 = sAverage(p012,p123);

    tess3DAddCubic(tess,mesh,p01,p012,p0123,toleranceSq,depth+1);
    tess3DAddCubic(tess,mesh,p123,p23,p3,toleranceSq,depth+1);
  }
}

static void __stdcall tess3DBeginCB(sInt type,Wz4Mesh *mesh)
{
  if(mesh->Faces.IsEmpty() || mesh->Faces.GetTail().Count)
  {
    Wz4MeshFace *face = mesh->Faces.AddMany(1);
    face->Init(0);
  }
}

static void __stdcall tess3DVertexCB(void *index,Wz4Mesh *mesh)
{
  sInt ind = (sDInt) index;
  Wz4MeshFace &face = mesh->Faces.GetTail();
  face.Vertex[face.Count++] = ind;
  if(face.Count == 3)
    tess3DBeginCB(0,mesh);
}

static void __stdcall tess3DCombineCB(sF64 coords[3],void *d[4],sF32 w[4],sInt *out,Wz4Mesh *mesh)
{
  Wz4MeshVertex *vert = mesh->Vertices.AddMany(1);
  sClear(*vert);
  vert->Pos.Init(coords[0],coords[1],coords[2]);
  vert->Normal.z = -1.0f;
  *out = mesh->Vertices.GetCount() - 1;
}

static void __stdcall tess3DEdgeFlagCB(sBool flag,Wz4Mesh *mesh)
{
}

void Wz4Mesh::Finish2DExtrusionOp(sF32 extrude,sInt flags)
{
  // calc adjacency and try to clean up triangulation by flipping degenerate tris
  Wz4MeshFaceConnect *adjacent = Adjacency();
  Wz4MeshFace *face;

  sFORALL(Faces,face)
  {
    const sVector31 &v0 = Vertices[face->Vertex[0]].Pos;
    const sVector31 &v1 = Vertices[face->Vertex[1]].Pos;
    const sVector31 &v2 = Vertices[face->Vertex[2]].Pos;
    sVector30 n = (v2-v0) % (v1-v0);

    if(n.z < 1e-6f)
    {
      // try to flip it
      for(sInt j=0;j<3;j++)
      {
        if(adjacent[_i].Adjacent[j] != -1)
        {
          EdgeFlip(adjacent,OutgoingEdge(_i,j));
          break;
        }
      }
    }
  }

  // extrude if necessary
  if(extrude && Vertices.GetCount() && Faces.GetCount())
  {
    // create copy of vertices moved back a bit.
    sInt oldVC = Vertices.GetCount();
    Vertices.Resize(oldVC*4);
    sCopyMem(&Vertices[oldVC],&Vertices[0],oldVC * sizeof(Wz4MeshVertex));
    for(sInt i=oldVC;i<oldVC*2;i++)
    {
      Vertices[i].Pos.z = extrude;
      Vertices[i].Normal.z = 1.0f;
    }
    
    // copies of vertices for sharp edges
    sCopyMem(&Vertices[oldVC*2],&Vertices[0],oldVC * 2 * sizeof(Wz4MeshVertex));
    for(sInt i=oldVC*2;i<oldVC*4;i++)
      sSwap(Vertices[i].Normal.x,Vertices[i].Normal.z);

    // create copy of faces and invert them
    sInt oldFC = Faces.GetCount();
    Faces.Resize(oldFC*2);
    sCopyMem(&Faces[oldFC],&Faces[0],oldFC * sizeof(Wz4MeshFace));
    for(sInt i=oldFC;i<oldFC*2;i++)
    {
      Wz4MeshFace *face = &Faces[i];
      sSwap(face->Vertex[0],face->Vertex[2]);
      for(sInt j=0;j<3;j++)
        face->Vertex[j] += oldVC;
    }

    // create extrusion faces
    for(sInt i=0;i<oldFC;i++)
    {
      for(sInt j=0;j<3;j++)
      {
        if(adjacent[i].Adjacent[j] == -1)
        {
          sInt e0 = Faces[i].Vertex[j];
          sInt e1 = Faces[i].Vertex[(j+1)%3];

          Wz4MeshFace *ef = Faces.AddMany(1);
          ef->Init(4);
          ef->Vertex[0] = e1 + oldVC*2;
          ef->Vertex[1] = e0 + oldVC*2;
          ef->Vertex[2] = e0 + oldVC*3;
          ef->Vertex[3] = e1 + oldVC*3;
        }
      }
    }

    if(flags&1)
    {
      // recalc adjacency and go through edges again, doubling vertices
      // on sharp edges
      delete[] adjacent;
      adjacent = Adjacency();

      // calc face normals
      sVector30 *faceNormals = new sVector30[Faces.GetCount()];
      sFORALL(Faces,face)
      {
        const sVector31 &v0 = Vertices[face->Vertex[0]].Pos;
        const sVector31 &v1 = Vertices[face->Vertex[1]].Pos;
        const sVector31 &v2 = Vertices[face->Vertex[2]].Pos;
        faceNormals[_i] = (v1-v0) % (v2-v0);
        faceNormals[_i].Unit();
      }

      //sF32 angleThresh = sFCos(sPIF / 3.0f); // 60° angle
      sF32 angleThresh = sFCos(sPIF / 4.0f); // 45° angle

      // sharp edges
      sFORALL(Faces,face)
      {
        for(sInt j=0;j<face->Count;j++)
        {
          // only go through edges once
          sInt adj = adjacent[_i].Adjacent[j];
          if((_i<<2) >= adj)
            continue;

          if((faceNormals[_i] ^ faceNormals[adj>>2]) <= angleThresh)
          {
            sInt vi[2];
            sInt e0v[2],e1v[2];

            // next face edge for adj
            const Wz4MeshFace *adjf = &Faces[adj>>2];
            sInt adjn = 0;
            if((adj & 3) == adjf->Count-1)
              adjn = adj & ~3;
            else
              adjn = adj + 1;

            vi[0] = j;
            vi[1] = (j+1 == face->Count) ? 0 : j+1;
            e0v[0] = face->Vertex[vi[0]];
            e1v[0] = adjf->Vertex[adjn&3];
            e0v[1] = face->Vertex[vi[1]];
            e1v[1] = adjf->Vertex[adj&3];

            for(sInt k=0;k<2;k++)
            {
              // only need to do something if the vertex is shared
              if(e0v[k] != e1v[k])
                continue;

              // clone it!
              sInt vInd = Vertices.GetCount();
              Vertices.AddMany(1);
              Vertices[vInd] = Vertices[e0v[k]];
              Vertices[vInd].Normal.Neg(); // to make sure it doesn't get merged
              face->Vertex[vi[k]] = vInd;
            }
          }
        }
      }

      delete[] faceNormals;
    }
  }

  delete[] adjacent;

  // uv mapping
  Wz4MeshVertex *mv;
  sFORALL(Vertices,mv)
  {
    mv->U0 = mv->Pos.x + mv->Pos.z;
    mv->V0 = mv->Pos.y;
  }

  CalcNormalAndTangents();
  MergeVertices();
}

void Wz4Mesh::MakeText(const sChar *text,const sChar *font,sF32 height,sF32 extrude,sF32 maxErr,sInt flags)
{
  sVERIFY(IsEmpty());

  AddDefaultCluster();
  height = sMax(height,8/128.0f);
  maxErr = sMax(maxErr,height/300.0f); // avoid excessively high tesselation

  HDC hDC = CreateCompatibleDC(0);
  HFONT hFont = CreateFontW(height*128,0,0,0,(flags&4)?700:400,(flags&8)?1:0,0,0,0,0,0,0,0,font);
  HFONT hOldFont = SelectObject(hDC,hFont);

  MAT2 mat;
  sSetMem(&mat,0,sizeof(mat));
  mat.eM11.value = 1;
  mat.eM22.value = 1;

  static const sInt bufSize = 256*1024;
  sU8 *buffer = new sU8[bufSize];
  sInt xPos = 0, yPos = 0;

  GLUtesselator *tess = gluNewTess();

  gluTessNormal(tess,0.0,0.0,-1.0);
  gluTessCallback(tess,GLU_TESS_BEGIN_DATA,(gluTessCB) tess3DBeginCB);
  gluTessCallback(tess,GLU_TESS_VERTEX_DATA,(gluTessCB) tess3DVertexCB);
  gluTessCallback(tess,GLU_TESS_COMBINE_DATA,(gluTessCB) tess3DCombineCB);
  gluTessCallback(tess,GLU_TESS_EDGE_FLAG_DATA,(gluTessCB) tess3DEdgeFlagCB);

  sF32 tolerance = height * 0.1f * maxErr;
  tolerance *= tolerance;

  for(sInt chr=0;text[chr];chr++)
  {
    if(text[chr] == '\n')
    {
      xPos = 0;
      yPos -= height*128;
      continue;
    }

    Wz4Mesh *tempmesh = new Wz4Mesh;

    GLYPHMETRICS gm;
    sU32 size = GetGlyphOutlineW(hDC,text[chr],0x102,&gm,bufSize,buffer,&mat);
    if(!size && !(flags&2))
    {
      xPos += gm.gmCellIncX;
      continue;
    }

    gluTessBeginPolygon(tess,tempmesh);
    sU8 *ptr = buffer, *end = buffer + size;
    while(ptr < end)
    {
      TTPOLYGONHEADER *hdr = (TTPOLYGONHEADER *) ptr;
      sU8 *polyEnd = ptr + hdr->cb;

      gluTessBeginContour(tess);
      tess3DAddPoint(tess,tempmesh,hdr->pfxStart.toVector(xPos,yPos));
      ptr += sizeof(TTPOLYGONHEADER);

      while(ptr < polyEnd)
      {
        TTPOLYCURVE *tpc = (TTPOLYCURVE *) ptr;

        switch(tpc->wType)
        {
        case 1: // line
          for(sInt i=0;i<tpc->cpfx;i++)
            tess3DAddPoint(tess,tempmesh,tpc->apfx[i].toVector(xPos,yPos));
          break;

        case 2: // qspline
          for(sInt i=0;i<tpc->cpfx-1;i++)
          {
            sVector31 b = tpc->apfx[i].toVector(xPos,yPos);
            sVector31 c = tpc->apfx[i+1].toVector(xPos,yPos);

            if(i < tpc->cpfx - 2)
              c = sAverage(b,c);

            tess3DAddQuadratic(tess,tempmesh,b,c,tolerance);
          }
        }

        ptr += sizeof(TTPOLYCURVE) + (tpc->cpfx - 1) * sizeof(POINTFX);
      }

      gluTessEndContour(tess);
    }

    gluTessEndPolygon(tess);

    if(tempmesh->Faces.GetCount())
    {
      tempmesh->Faces.RemTail();
      tempmesh->Finish2DExtrusionOp(extrude,flags);
    }

    sInt vindex = Vertices.GetCount();
    sInt findex = Faces.GetCount();

    // copy vertices/faces and map bone/vtx refs
    Wz4MeshVertex *vertex;
    sFORALL(tempmesh->Vertices,vertex)
    {
      if (flags&2)
      {
        vertex->Weight[0]=1;
        vertex->Index[0]=chr;
      }
      if (flags&16)
      {
        sSwap(vertex->Pos.x,vertex->Pos.z); vertex->Pos.x*=-1;
        sSwap(vertex->Normal.x,vertex->Normal.z); vertex->Normal.x*=-1;
        sSwap(vertex->Tangent.x,vertex->Tangent.z); vertex->Normal.x*=-1;
      }
      Vertices.AddTail(*vertex);
    }

    Wz4MeshFace *face;
    sFORALL(tempmesh->Faces,face)
    {
      for (sInt i=0; i<face->Count; i++)
        face->Vertex[i]+=vindex;
      Faces.AddTail(*face);
    }

    delete tempmesh;

    // make chunk?
    if (flags&2)
    {
      Wz4ChunkPhysics chunk;

      chunk.Volume = 1.0f;
      if (flags&4)
      {
        chunk.COM.Init(0,yPos/128.0f,(xPos+gm.gmBlackBoxX/2)/128.0f);
        chunk.InertD.Init(-1,0,0);
      }
      else
      {
        chunk.COM.Init((xPos+gm.gmBlackBoxX/2)/128.0f,yPos/128.0f,0);
        chunk.InertD.Init(0,0,1);
      }
      chunk.InertOD.Init(0,0,0);
      chunk.FirstVert = vindex;
      chunk.FirstFace = findex;
      chunk.FirstIndex = 0;
      chunk.Temp = 0;
      chunk.Normal=chunk.InertD;
      chunk.Random.Init(0,1,0);
      Chunks.AddTail(chunk);
    }

    xPos += gm.gmCellIncX;
  }


  // stop tesselation stuff
  gluDeleteTess(tess);
  delete[] buffer;

  SelectObject(hDC,hOldFont);
  DeleteObject(hFont);
  DeleteDC(hDC);
}

static const sChar *skipWhitespace(const sChar *s)
{
  while(*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
    s++;

  return s;
}

static sBool parseSVGVec(const sChar *&cp,sVector30 &out)
{
  if(!sScanFloat(cp,out.x))
    return sFALSE;

  sBool gotWhite = sFALSE;
  while(sIsSpace(*cp))
    gotWhite = sTRUE, cp++;

  if(*cp == ',')
    cp++;
  else if(!gotWhite)
    return sFALSE;

  while(sIsSpace(*cp))
    cp++;

  if(!sScanFloat(cp,out.y))
    return sFALSE;
  out.y = -out.y; // SVG has y pointing downwards
  out.z = 0.0f;
  cp = skipWhitespace(cp);
  return sTRUE;
}

enum PathCommand
{
  PC_NONE,
  PC_LINE,
  PC_QUADRATIC,
  PC_CUBIC,

  // finish commands start here
  PC_FINISH,
  PC_FINISH_POLY,
};

void Wz4Mesh::MakePath(const sChar *path,sF32 extrude,sF32 maxErr,sF32 weldThreshold,sInt flags)
{
  maxErr = sMax(maxErr,1e-4f);

  AddDefaultCluster();

  GLUtesselator *tess = gluNewTess();

  gluTessNormal(tess,0.0,0.0,-1.0);
  gluTessCallback(tess,GLU_TESS_BEGIN_DATA,(gluTessCB) tess3DBeginCB);
  gluTessCallback(tess,GLU_TESS_VERTEX_DATA,(gluTessCB) tess3DVertexCB);
  gluTessCallback(tess,GLU_TESS_COMBINE_DATA,(gluTessCB) tess3DCombineCB);
  gluTessCallback(tess,GLU_TESS_EDGE_FLAG_DATA,(gluTessCB) tess3DEdgeFlagCB);

  sAABBox box;
  box.Clear();

  sF32 toleranceSq = 0.0f;
  sF32 diameter = 0.0f;

  // go over input data twice. first pass: determine bounding box, second pass: process geometry.
  for(sInt pass=0;pass<2;pass++)
  {
    const sChar *cp = skipWhitespace(path);
    PathCommand cmd = PC_NONE;
    sBool inPolygon = sFALSE;
    sBool inContour = sFALSE;
    sBool relative;
    sVector31 lastPos(0.0f);

    while(*cp)
    {
      // new command?
      sChar cur = *cp;
      if((cur >= 'A' && cur <= 'Z') || (cur >= 'a' && cur <= 'z'))
      {
        cp = skipWhitespace(cp+1); // command found, skip to next char
        relative = cur >= 'a';

        switch(sUpperChar(cur))
        {
        case 'M': // moveto
        case 'L': // lineto (we don't distinguish between the two, since we only expect moveto at the start)
          cmd = PC_LINE;
          break;

        case 'Q': // quadratic
          cmd = PC_QUADRATIC;
          break;

        case 'C': // cubic
          cmd = PC_CUBIC;
          break;

        case 'Z': // finish
          cmd = PC_FINISH;
          break;

        case 'N': // next polygon
          cmd = PC_FINISH_POLY;
          break;

        default:
          sDPrintF(L"MakePath: Unknown command '%c'!\n",cur);
          return;
        }
      }
      else if(cur!='-' && !sIsDigit(cur))
      {
        sDPrintF(L"MakePath: Command expected!\n");
        return;
      }

      // prepare for command
      if(cmd < PC_FINISH)
      {
        // start a new polygon if necessary
        if(!inPolygon)
        {
          if(pass == 1)
            gluTessBeginPolygon(tess,this);

          inPolygon = sTRUE;
          lastPos.Init(0.0f,0.0f,0.0f);
        }

        // start a new contour if necessary
        if(!inContour)
        {
          if(pass == 1)
            gluTessBeginContour(tess);

          inContour = sTRUE;
        }
      }

      // consume data
      sVector31 ref(0.0f);
      sVector30 a,b,c;

      if(relative)
        ref = lastPos;

      switch(cmd)
      {
      case PC_LINE:
        if(!parseSVGVec(cp,a))
          return;
        if(pass == 1)
          tess3DAddPoint(tess,this,ref+a);
        else
          box.Add(ref+a);
        lastPos = ref+a;
        break;

      case PC_QUADRATIC:
        if(!parseSVGVec(cp,a) || !parseSVGVec(cp,b))
          return;
        if(pass == 1)
          tess3DAddQuadratic(tess,this,ref+a,ref+b,toleranceSq);
        else
        {
          box.Add(ref+a);
          box.Add(ref+b);
        }
        lastPos = ref+b;
        break;

      case PC_CUBIC:
        if(!parseSVGVec(cp,a) || !parseSVGVec(cp,b) || !parseSVGVec(cp,c))
          return;
        if(pass == 1)
          tess3DAddCubic(tess,this,ref+a,ref+b,ref+c,toleranceSq);
        else
        {
          box.Add(ref+a);
          box.Add(ref+b);
          box.Add(ref+c);
        }
        lastPos = ref+c;
        break;

      case PC_FINISH:
      case PC_FINISH_POLY:
        if(inContour)
        {
          if(pass == 1)
            gluTessEndContour(tess);

          inContour = sFALSE;
        }

        if(cmd == PC_FINISH_POLY && inPolygon)
        {
          if(pass == 1)
            gluTessEndPolygon(tess);

          inPolygon = sFALSE;
        }
        break;

      default:
        sDPrintF(L"MakePath: No command before the first set of coordinates!\n");
        return;
      }
    }

    if(inContour)
    {
      if(pass == 1)
        gluTessEndContour(tess);

      inContour = sFALSE;
    }

    if(inPolygon)
    {
      if(pass == 1)
        gluTessEndPolygon(tess);

      inPolygon = sFALSE;
    }

    if(pass == 0) // finished first pass, set up tolerances
    {
      sVector30 boxSize = box.Max - box.Min;
      diameter = sMax(boxSize.x,sMax(boxSize.y,boxSize.z));

      toleranceSq = sSquare(diameter * maxErr);
    }
  }

  // last face generated is empty.
  if(Faces.GetCount())
    Faces.RemTail();

  gluDeleteTess(tess);

  // the meshes produced from SVG paths tend to be extra-shitty with lots
  // of almost doubled vertices and the like. play it safe and weld with an
  // appropriate tolerance before processing it further.
  Weld(diameter * weldThreshold);
  RemoveDegenerateFaces();
  Finish2DExtrusionOp(extrude,flags);
}

#else

void Wz4Mesh::MakeText(const sChar *text,const sChar *font,sF32 height,sF32 extrude,sF32 maxErr,sInt flags)
{
  sFatal(L"Wz4Mesh::MakeText() only for windows...");
}

void Wz4Mesh::MakePath(const sChar *path,sF32 extrude,sF32 maxErr,sInt flags)
{
  sFatal(L"Wz4Mesh::MakePath() only for windows...");
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Splitting                                                          ***/
/***                                                                      ***/
/****************************************************************************/

static const sF32 ClassifyEpsilon = 1e-6f;

static sInt ClassifyVertex(const Wz4MeshVertex &v,const sVector4 &plane)
{
  sF32 dp = v.Pos ^ plane;
  return (dp > ClassifyEpsilon) ? 1 : (dp < -ClassifyEpsilon) ? -1 : 0;
}

sInt Wz4Mesh::SplitEdgeAlongPlane(sInt va,sInt vb,const sVector4 &plane)
{
  // TODO: hash table so vertices aren't generated multiple times
  const Wz4MeshVertex a = Vertices[va];
  const Wz4MeshVertex b = Vertices[vb];

  sF32 da = a.Pos ^ plane;
  sF32 db = b.Pos ^ plane;
  sVERIFY(sSign(da) != sSign(db) && sFAbs(da-db) >= 2.0f*ClassifyEpsilon);

  sF32 t = da / (da - db);
  sVERIFY(t >= 0.0f && t <= 1.0f);

  Wz4MeshVertex *v = Vertices.AddMany(1);
  v->Pos.Fade(t,a.Pos,b.Pos);
  v->Normal.Fade(t,a.Normal,b.Normal);    v->Normal.Unit();
  v->Tangent.Fade(t,a.Tangent,b.Tangent); v->Tangent.Unit();
  v->BiSign = a.BiSign;
  v->U0 = sFade(t,a.U0,b.U0);
  v->V0 = sFade(t,a.V0,b.V0);
  v->U1 = sFade(t,a.U1,b.U1);
  v->V1 = sFade(t,a.V1,b.V1);
#if !WZ4MESH_LOWMEM
  v->Color0 = sFadeColor(t*65536,a.Color0,b.Color0);
  v->Color1 = sFadeColor(t*65536,a.Color1,b.Color1);
#endif
  for(sInt i=0;i<4;i++)
  {
    v->Index[i] = a.Index[i];
    v->Weight[i] = a.Weight[i];
  }
  v->Select = 0.0f;
  v->Temp = 0;

  return Vertices.GetCount()-1;
}

void Wz4Mesh::SplitGenFace(sInt base,const sInt *verts,sInt count,sBool reuseBase)
{
  Wz4MeshFace *out = reuseBase ? &Faces[base] : Faces.AddMany(1);
  sVERIFY(count >= 3);

  if(count <= 4)
  {
    out->Cluster = Faces[base].Cluster;
    out->Count  = count;
    out->Select = Faces[base].Select;
    out->Selected = Faces[base].Selected;
    for(sInt i=0;i<count;i++)
      out->Vertex[i] = verts[i];
  }
  else if(count == 5)
  {
    out->Cluster = Faces[base].Cluster;
    out->Count = 4;
    out->Select = Faces[base].Select;
    out->Selected = Faces[base].Selected;
    for(sInt i=0;i<4;i++)
      out->Vertex[i] = verts[i];

    out = Faces.AddMany(1);
    out->Cluster = Faces[base].Cluster;
    out->Count = 3;
    out->Select = Faces[base].Select;
    out->Selected = Faces[base].Selected;
    out->Vertex[0] = verts[0];
    out->Vertex[1] = verts[count-2];
    out->Vertex[2] = verts[count-1];
  }
  else
    sVERIFYFALSE;
}

void Wz4Mesh::SplitAlongPlane(const sVector4 &plane)
{
  // since we add faces...
  sInt faceCount = Faces.GetCount();

  for(sInt i=0;i<faceCount;i++)
  {
    sInt backVert[5],frontVert[5];
    sInt backCount=0,frontCount=0;

    sInt vi,vl,vn;
    sInt cls,lastCls;
    vl = Faces[i].Vertex[Faces[i].Count-1];
    lastCls = ClassifyVertex(Vertices[vl],plane);

    for(sInt j=Faces[i].Count-1,k=0;k<Faces[i].Count;j=k,lastCls=cls,vl=vi,k++)
    {
      vi = Faces[i].Vertex[k];
      cls = ClassifyVertex(Vertices[vi],plane);

      switch(lastCls*3 + cls)
      {
      case -1*3 - 1: // back->back
        backVert[backCount++] = vi;
        break;

      case -1*3 + 0: // back->on
        backVert[backCount++]  = vi;
        frontVert[frontCount++] = vi;
        break;

      case -1*3 + 1: // back->front
        vn = SplitEdgeAlongPlane(vi, vl, plane);
        backVert[backCount++] = vn;
        frontVert[frontCount++] = vn;
        frontVert[frontCount++] = vi;
        break;

      case 0*3 - 1: // on->back
        backVert[backCount++] = vl;
        backVert[backCount++] = vi;
        break;

      case 0*3 + 0: // on->on
      case 0*3 + 1: // on->front
        frontVert[frontCount++] = vi;
        break;

      case 1*3 - 1: // front->back
        vn = SplitEdgeAlongPlane(vl, vi, plane);
        frontVert[frontCount++] = vn;
        backVert[backCount++] = vn;
        backVert[backCount++] = vi;
        break;

      case 1*3 + 0: // front->on
      case 1*3 + 1: // front->front
        frontVert[frontCount++] = vi;
        break;
      }
    }

    // generate output polygons
    if(frontCount>=3) SplitGenFace(i,frontVert,frontCount,sTRUE);
    if(backCount>=3)  SplitGenFace(i,backVert,backCount,frontCount<3);
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Wz3 Mesh Loading                                                   ***/
/***                                                                      ***/
/****************************************************************************/

static inline sU16 GetLE16(const sU8 *&data)
{
  sU16 ret = data[0] + (data[1] << 8);
  data += 2;
  return ret;
}

static inline sF32 GetPF16(const sU8 *&data)
{
  union
  {
    sU32 u;
    sF32 f;
  } vd;

  sInt v = *data++;
  if(v == 0x00) // zero
    return 0.0f;
  else if(v == 0x80) // one
    return 1.0f;
  else if(v == 0x01) // 0.5
    return 0.5f;
  else if(v == 0x81) // 0.25
    return 0.25f;
  else
  {
    v = (v << 8) | *data++;
    vd.u = (v & 32768) << 16 // sign
      | ((((v >> 10) & 31) + 128 - 16) << 23)
      | ((v & 1023) << 13);

    return vd.f;
  }
}

static inline sU32 GetLE32(const sU8 *&data)
{
  sU32 ret = data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24);
  data += 4;
  return ret;
}

static inline sF32 GetLEF32(const sU8 *&data)
{
  union
  {
    sF32 f;
    sU32 u;
  } u;

  u.u = GetLE32(data);
  return u.f;
}

static sQuaternion QuatFromEuler(sF32 rx,sF32 ry,sF32 rz)
{
  sMatrix34 mat;
  mat.EulerXYZ(rx*sPI2F,ry*sPI2F,rz*sPI2F);

  sQuaternion quat;
  quat.Init(mat);
  return quat;
}

void Wz4Mesh::LoadWz3MinMesh(const sU8 *&blob)
{
  const sU8 *data = blob;
  Clear();

  // vertices
  Vertices.Resize(GetLE32(data));
  for(sInt i=0;i<Vertices.GetCount();i++)
  {
    Wz4MeshVertex &vert = Vertices[i];

    sInt boneCount = *data++;
    vert.Init();
#if !WZ4MESH_LOWMEM
    vert.Color0 = GetLE32(data);
#else
    GetLE32(data);
#endif
    vert.Pos.x = GetLEF32(data);
    vert.Pos.y = GetLEF32(data);
    vert.Pos.z = GetLEF32(data);
    vert.Normal.Init(i,0,0); // all unique
    vert.U0 = GetLEF32(data);
    vert.V0 = GetLEF32(data);

    for(sInt i=0;i<boneCount;i++)
    {
      sF32 weight = GetPF16(data);
      sInt index = GetLE16(data);
      vert.AddWeight(index,weight);
    }

    vert.NormWeight();
  }

  // faces
  sBool vert16Bit = Vertices.GetCount() < 65536;
  sInt nFaces = GetLE32(data); // includes stitches, so it may be too high
  Faces.HintSize(nFaces);
  for(sInt i=0;i<nFaces;i++)
  {
    sInt nVerts = *data++;
    sVERIFY(nVerts >= 2 && nVerts <= 8);

    sInt cluster = *data++;
    sInt Verts[8];

    if(vert16Bit)
    {
      for(sInt j=0;j<nVerts;j++)
        Verts[j] = GetLE16(data);
    }
    else
    {
      for(sInt j=0;j<nVerts;j++)
        Verts[j] = GetLE32(data);
    }

    if(nVerts > 4)
    {
      // need to tesselate
      for(sInt j=2;j<nVerts;j++)
      {
        Wz4MeshFace tri;
        tri.Init(3);
        tri.Cluster = cluster;
        tri.Vertex[0] = Verts[0];
        tri.Vertex[1] = Verts[j-1];
        tri.Vertex[2] = Verts[j];
        Faces.AddTail(tri);
      }
    }
    else if(nVerts >= 3)
    {
      Wz4MeshFace fc;
      fc.Init(nVerts);
      fc.Cluster = cluster;
      for(sInt j=0;j<nVerts;j++)
        fc.Vertex[j] = Verts[j];
      Faces.AddTail(fc);
    }
    else if(nVerts == 2) // stitch
    {
      sInt v0 = Verts[0];
      sInt v1 = Verts[1];
      if(v0 > v1)
        sSwap(v0,v1);

      Vertices[v1].Normal.x = v0; // stitch them together!
    }
  }

  // clusters
  sInt nClusters = GetLE32(data);
  Clusters.HintSize(nClusters);
  for(sInt i=0;i<nClusters;i++)
  {
    sU32 renderPassAnimType = GetLE32(data);
    sU32 clusterID = GetLE32(data);
    sU32 animMatrix = GetLE32(data);
    
    // ignored! (for now)
    (void) clusterID;
    (void) animMatrix;

    AddDefaultCluster();
    Clusters[i]->LocalRenderPass = renderPassAnimType & 0xff;
  }

  // animation
  sInt nJoints = GetLE32(data);
  if(nJoints)
  {
    Skeleton = new Wz4Skeleton;
    Skeleton->Joints.HintSize(nJoints);

    for(sInt i=0;i<nJoints;i++)
    {
      Wz4AnimJoint *joint = Skeleton->Joints.AddMany(1);

      joint->Init();

      sMatrix44 basePose;
      for(sInt j=0;j<16;j++)
        (&basePose.i.x)[j] = GetLEF32(data);

      joint->BasePose.i = sVector30(basePose.i); sVERIFY(basePose.i.w == 0.0f);
      joint->BasePose.j = sVector30(basePose.j); sVERIFY(basePose.j.w == 0.0f);
      joint->BasePose.k = sVector30(basePose.k); sVERIFY(basePose.k.w == 0.0f);
      joint->BasePose.l = sVector31(basePose.l); sVERIFY(basePose.l.w == 1.0f);
      joint->Parent = GetLE32(data);
      
      sU32 keycountFlags = GetLE32(data);

      // read channel data 
      Wz4ChannelPerFrame *chan = new Wz4ChannelPerFrame;
      joint->Channel = chan;

      sF32 srt[9];
      for(sInt j=0;j<9;j++)
        srt[j] = GetLEF32(data);

      chan->Start.Init();
      chan->Start.Scale.Init(srt[0],srt[1],srt[2]);
      chan->Start.Rot = QuatFromEuler(srt[3],srt[4],srt[5]);
      chan->Start.Trans.Init(srt[6],srt[7],srt[8]);
      chan->Start.Weight = 1.0f;
      chan->Keys = keycountFlags >> 3;
      chan->MaxTime = chan->Keys / 25.0f;

      if(keycountFlags & 1) // scale animated
      {
        chan->Scale = new sVector31[chan->Keys];
        for(sInt j=0;j<chan->Keys;j++)
        {
          chan->Scale[j].x = GetLEF32(data);
          chan->Scale[j].y = GetLEF32(data);
          chan->Scale[j].z = GetLEF32(data);
        }
      }

      if(keycountFlags & 2) // rotation animated
      {
        chan->Rot = new sQuaternion[chan->Keys];
        for(sInt j=0;j<chan->Keys;j++)
        {
          sF32 rx = GetLEF32(data);
          sF32 ry = GetLEF32(data);
          sF32 rz = GetLEF32(data);
          chan->Rot[j] = QuatFromEuler(rx,ry,rz);
        }
      }

      if(keycountFlags & 4) // translation animated
      {
        chan->Trans = new sVector31[chan->Keys];
        for(sInt j=0;j<chan->Keys;j++)
        {
          chan->Trans[j].x = GetLEF32(data);
          chan->Trans[j].y = GetLEF32(data);
          chan->Trans[j].z = GetLEF32(data);
        }
      }
    }

    Skeleton->FixTime(1.0f / 25.0f);
  }

  blob = data;
}

sBool Wz4Mesh::LoadWz3MinMesh(const sChar *file)
{
  sDInt size;
  sU8 *buffer = sLoadFile(file,size);
  if(!buffer)
    return sFALSE;

  const sU8 *data = buffer;
  LoadWz3MinMesh(data);

  sVERIFY(data <= buffer+size);

  // update everything
  CalcNormalAndTangents();
  MergeVertices();
  MergeClusters();
  SortClustersByLocalRenderPass();
  SplitClustersAnim(74);

  // cleanup
  delete[] buffer;
  return sTRUE;
}

/****************************************************************************/
/***                                                                      ***/
/***   TomF's vertex cache optimizer                                      ***/
/***                                                                      ***/
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

void OptimizeIndexOrder(sInt *IndexBuffer,sInt IndexCount,sInt VertexCount)
{
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
    sF32 score = (i<3) ? 0.75f : sFPow(1.0f - (i-3)/sF32(cacheSize-3),1.5f);
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

/****************************************************************************/

void DumpCacheEfficiency(const sInt *inds,sInt indCount)
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
  sDPrintF(L"ACMR: %d.%03d (fifo: %s)\n",sInt(ACMR),sInt(ACMR*1000+0.5f)%1000,isFIFO ? L"yes" : L"no");
}

/****************************************************************************/
/***                                                                      ***/
/***   ...                                                                ***/
/***                                                                      ***/
/****************************************************************************/
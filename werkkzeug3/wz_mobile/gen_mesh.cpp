// This file is distributed under a BSD license. See LICENSE.txt for details.


#include "gen_mesh.hpp"

/****************************************************************************/
/****************************************************************************/

void GenMinVector::UnitSafe()
{
  sF64 e;
  e = x*x+y*y+z*z;
  if(e<0.000000000001f)
  {
    Init(1,0,0);
  }
  else
  {
    e = sFInvSqrt(e);
    x *= e;
    y *= e;
    z *= e;
  }
}

void GenMinVector::Cross(const GenMinVector &a,const GenMinVector &b)
{
  x=a.y*b.z-a.z*b.y; 
  y=a.z*b.x-a.x*b.z; 
  z=a.x*b.y-a.y*b.x;
}

/****************************************************************************/

sBool GenMinVert::IsSame(const GenMinVert &v,sBool uv)
{
  const sF32 e=0.00001f;
  if(sFAbs(Pos.x-v.Pos.x)>e) return 0;
  if(sFAbs(Pos.y-v.Pos.y)>e) return 0;
  if(sFAbs(Pos.z-v.Pos.z)>e) return 0;
  if(BoneCount!=v.BoneCount) return 0;
  if(BoneCount)
  {
    if(sCmpMem(Weights,v.Weights,BoneCount*4)!=0) return 0;
    if(sCmpMem(Matrix ,v.Matrix ,BoneCount*2)!=0) return 0;
  }
  if(uv)
  {
    if(sCmpMem(UV,v.UV,sizeof(UV))!=0) return 0;
  }
  return 1;
}

/****************************************************************************/

sU32 GenMinMesh::HashVector(const GenMinVector &v)
{
  const sU8 *src = (const sU8 *) &v.x;
  sU32 hash = 2166136261;
  for(sInt i=0;i<sizeof(GenMinVector);i++)
    hash = (hash ^ *src++) * 16777619;

  return hash;
}

/****************************************************************************/

GenMinMesh::GenMinMesh()
{
  Vertices.Init();
  Faces.Init();
  Clusters.Init();

  RefCount = 1;
  Clusters.Add()->Init(0);   // deleted cluster
//  AddCluster(GenOverlayManager->DefaultMat,0); // default cluster
  NormalsOK = 0;
  ChangeFlag = 3;
  PreparedMesh = 0;
#if !sPLAYER
  WireMesh = 0;
#endif
}

GenMinMesh::~GenMinMesh()
{
//  for(sInt i=0;i<Clusters.Count;i++)
//    if(Clusters[i].Mtrl)
//      Clusters[i].Mtrl->Release();

  Vertices.Exit();
  Faces.Exit();
  Clusters.Exit();

//  UnPrepare();
#if !sPLAYER
//  UnPrepareWire();
#endif
}

void GenMinMesh::AddRef()
{
  RefCount++;
}

void GenMinMesh::Release()
{
  RefCount--;
  if(RefCount==0)
    delete this;
}

void GenMinMesh::Copy(GenMinMesh *mm)
{
//  for(sInt i=0;i<Clusters.Count;i++)
//    if(Clusters[i].Mtrl)
//      Clusters[i].Mtrl->Release();

  Vertices.Copy(mm->Vertices);
  Faces.Copy(mm->Faces);
  Clusters.Copy(mm->Clusters);
//  for(sInt i=0;i<Clusters.Count;i++)
//    if(Clusters[i].Mtrl)
//      Clusters[i].Mtrl->AddRef();

  ChangeTopo();
}

void GenMinMesh::Clear()
{
  Vertices.Count = 0;
  Faces.Count = 0;
  ChangeTopo();
}

/****************************************************************************/

void GenMinMesh::ChangeGeo()
{
  NormalsOK = 0;
}

void GenMinMesh::ChangeTopo()
{
  NormalsOK = 0;
}

/****************************************************************************/

sInt GenMinMesh::AddCluster(GenMaterial *mtrl,sInt pass)
{
  for(sInt i=1;i<Clusters.Count;i++)
    if(Clusters[i].Mtrl == mtrl)
      return i;

  sInt id = Clusters.Count;
  Clusters.Add()->Init(mtrl,pass);
//  if(mtrl)
//    mtrl->AddRef();
  return id;
}


GenMinVert *GenMinMesh::MakeGrid(sInt tx,sInt ty,sInt flags)
{
  sInt x,y;
  sInt vi,fi,vc,fc,va,fa,bits;
  GenMinVert *vp;
  GenMinFace *fp;

  // additions (top/bottom)


  bits = 0;
  if(flags & 1) { bits++; }
  if(flags & 2) { bits++; }
  va = bits;
  fa = tx*bits;
  if(flags & 4) { fa+=tx+1; }
  if(flags & 8) { fa+=ty+1; }


  // vertices

  vc = (tx+1)*(ty+1);
  vi = Vertices.Count;
  Vertices.SetMax(vi+vc+va);
  vp = &Vertices[vi];
  Vertices.Count = vi+vc+va;
  sSetMem(vp,0,sizeof(*vp)*(vc+va));

  for(y=0;y<=ty;y++)
  {
    for(x=0;x<=tx;x++)
    {
      vp->Pos.x = 1.0f*x/tx-0.5f;
      vp->Pos.y = 1.0f*y/ty-0.5f;
      vp->UV[0][0] = 1.0f*x/tx;
      vp->UV[0][1] = 1.0f-1.0f*(y)/(ty);
      vp->Select = 1;
      vp->Color = ~0;
      vp++;
    }
  }

  // faces

  fc = tx*ty;
  fi = Faces.Count;
  Faces.SetMax(fi+fc+fa);
  fp = &Faces[fi];
  Faces.Count = fi+fc+fa;
  sSetMem(fp,0,sizeof(*fp)*(fc+fa));

  for(y=0;y<ty;y++)
  {
    for(x=0;x<tx;x++)
    {
      fp->Select = 1;
      fp->Count = 4;
      fp->Cluster = 1;
      fp->Vertices[0] = vi+(y+0)*(tx+1)+(x+0);
      fp->Vertices[1] = vi+(y+1)*(tx+1)+(x+0);
      fp->Vertices[2] = vi+(y+1)*(tx+1)+(x+1);
      fp->Vertices[3] = vi+(y+0)*(tx+1)+(x+1);
      fp++;
    }
  }

  // additions (top / bottom)

  sInt center = vi+vc;
  sInt border = vi; 
  for(sInt i=0;i<2;i++)     // note that i is 0 or 1
  {
    if(flags & (i+1))
    {
      vp->UV[0][0] = 1.0f;
      vp->UV[0][1] = 1-i;
      vp->Select = 1;
      vp->Color = ~0;
      vp++;
      for(x=0;x<tx;x++)
      {
        fp->Select = 1;
        fp->Count = 3;
        fp->Cluster = 1;
        fp->Vertices[0] = center;
        fp->Vertices[1] = border+i;
        border++;
        fp->Vertices[2] = border-i;
        fp++;
      }
    }
    border = vi+(ty)*(tx+1);
    if(flags&1) center++;
  }

  // additions: stitches

  if(flags&4)
  {
    for(sInt i=0;i<=tx;i++)
    {
      fp->Count = 2;
      fp->Vertices[0] = vi+i;
      fp->Vertices[1] = vi+i+ty*(tx+1);
      fp++;
    }
  }

  if(flags&8)
  {
    for(sInt i=0;i<=ty;i++)
    {
      fp->Count = 2;
      fp->Vertices[0] = vi+i*(tx+1);
      fp->Vertices[1] = vi+i*(tx+1)+tx;
      fp++;
    }
  }

  return &Vertices[vi];
}

void GenMinMesh::Transform(sInt sel,const sMatrix &mat,sInt src,sInt dest)
{
  sVector v;
  GenMinVert *vp;
  sBool ok;

  vp = Vertices.Array;
  src--;
  dest--;
  sVERIFY(src<KMM_MAXUV);
  sVERIFY(dest<KMM_MAXUV);
  for(sInt i=0;i<Vertices.Count;i++)
  {
    ok = 1;
    if(sel==MMU_SELECTED)
      ok = vp->Select;
    else if(sel==MMU_UNSELECTED)
      ok = !vp->Select;

    if(ok)
    {
      if(src<0)
        vp->Pos.Out(v,1);
      else
        v.Init(vp->UV[src][0],vp->UV[src][1],0,1);
     
      v.Rotate34(mat);

      if(dest<0)
      {
        vp->Pos.Init(v);
      }
      else
      {
        vp->UV[dest][0] = v.x;
        vp->UV[dest][1] = v.y;
      }
    }
    vp++;
  }
}

void GenMinMesh::CalcNormals()
{
  GenMinFace *mf;
  GenMinVert *mv;
  sInt i,j;
  GenMinVector n,t;
  GenMinVector d0,d1;
  sInt p0,p1,p2;

  if(NormalsOK)
    return;
  NormalsOK = sTRUE;

  mf = &Faces[0];
  mv = &Vertices[0];

  for(i=0;i<Vertices.Count;i++)
  {
    mv[i].Normal.Init();
    mv[i].Tangent.Init();
  }
  for(i=0;i<Faces.Count;i++)
  {
    n.Init();
    if(mf[i].Count==2)
    {
      p0 = mf[i].Vertices[0];
      p1 = mf[i].Vertices[1];
      n = mv[p0].Normal;
      mv[p0].Normal.Add(mv[p1].Normal);
      mv[p1].Normal.Add(n);
      n = mv[p0].Tangent;
      mv[p0].Tangent.Add(mv[p1].Tangent);
      mv[p1].Tangent.Add(n);
    }
    else if(mf[i].Count>=3)
    {
      // normal
      p0 = mf[i].Vertices[0];
      p1 = mf[i].Vertices[1];
      p2 = mf[i].Vertices[2];
      d0.Sub(mv[p1].Pos,mv[p0].Pos);
      d1.Sub(mv[p2].Pos,mv[p0].Pos);
      n.Cross(d0,d1);
      n.UnitSafe();
      mf[i].Normal = n;

      // tangent
      sF32 c1 = mv[p0].UV[0][1] - mv[p1].UV[0][1];
      sF32 c2 = mv[p2].UV[0][1] - mv[p0].UV[0][1];
      sF32 ix = (mv[p1].UV[0][0] - mv[p0].UV[0][0]) * c2
              + (mv[p2].UV[0][0] - mv[p0].UV[0][0]) * c1;

      t.Init(0,0,0);
      if(sFAbs(ix) > 1e-20f)
      {
        t.AddScale(d0,c2*ix);
        t.AddScale(d1,c1*ix);
      }

      // orthogonalize
      t.AddScale(n,-t.Dot(n));
      t.UnitSafe();

      // accumulate
      for(j=0;j<mf[i].Count;j++)
      {
        sInt v = mf[i].Vertices[j];
        mv[v].Normal.Add(n);
        mv[v].Tangent.Add(t);
      }
    }
  }
  for(i=0;i<Vertices.Count;i++)
  {
    mv[i].Normal.UnitSafe();
    mv[i].Tangent.UnitSafe();
  }
}

/*
void GenMinMesh::Cleanup()
{
  sInt *temp = new sInt[Vertices.Count];

  for(sInt i=0;i<Vertices.Count;i++)
    temp[i] = 0;

  // delete faces and mark used vertices

  sInt fc = 0;
  for(sInt i=0;i<Faces.Count;i++)
  {
    GenMinFace *face = &Faces[i];

    if(face->Cluster && face->Count==3)     // skip double vertices (like a quad that is actually a tri)
    {
      sInt last = face->Vertices[face->Count-1];  
      sInt pc = 0;                          // quads that are actually a line will have two vertices after this.
      for(sInt j=0;j<face->Count;j++)
      {
        if(face->Vertices[j]!=last)
          last = face->Vertices[pc++] = face->Vertices[j];
      }
      face->Count = (pc>=3)?pc:0;
    }
      

    if(face->Count>0)                    // skip degenerated faces (like generated by the previous pass)
    {
      for(sInt j=0;j<face->Count;j++)  // mark used vertices
        temp[face->Vertices[j]] = 1;

      Faces[fc++] = Faces[i];            // copy face.
    }
  }
  Faces.Count = fc;

  // remap vertices

  sInt vc = 0;                          
  for(sInt i=0;i<Vertices.Count;i++)
  {
    if(temp[i])
    {
      Vertices[vc] = Vertices[i];         // copy the vertex
      temp[i] = vc++;                     // update remapping
    }
    else
    {
      temp[i] = -1;
    }
  }
  Vertices.Count = vc;

  // remap faces

  for(sInt i=0;i<Faces.Count;i++)
  {
    GenMinFace *face = &Faces[i];
    for(sInt j=0;j<face->Count;j++)
    {
      face->Vertices[j] = temp[face->Vertices[j]];
      sVERIFY(face->Vertices[j]>=0);
    }
  }

  // done

  delete[] temp;
}
*/

void GenMinMesh::Add(GenMinMesh *other)
{
  sInt p,a;
  

  // copy faces

  p = Faces.Count;
  a = other->Faces.Count;
  Faces.Resize(p+a);
  sCopyMem(&Faces[p],&other->Faces[0],a*sizeof(GenMinFace));

  // update vertex and cluster in faces

  for(sInt i=p;i<Faces.Count;i++)
  {
    GenMinFace *f = &Faces[i];
    for(sInt j=0;j<f->Count;j++)
      f->Vertices[j] += Vertices.Count;
    if(f->Cluster>0)
      f->Cluster += Clusters.Count-1;
  }

  // copy vertices

  p = Vertices.Count;
  a = other->Vertices.Count;
  Vertices.Resize(p+a);
  sCopyMem(&Vertices[p],&other->Vertices[0],a*sizeof(GenMinVert));

  // copy and refcount clusters. clusters should get merged...

  p = Clusters.Count;
  a = other->Clusters.Count-1;
  Clusters.Resize(p+a);
  sCopyMem(&Clusters[p],&other->Clusters[1],a*sizeof(GenMinCluster));
//  for(sInt i=p;i<Clusters.Count;i++)
//    if(Clusters[i].Mtrl)
//      Clusters[i].Mtrl->AddRef();

  ChangeTopo();
}

void GenMinMesh::Invert()
{
  for(sInt i=0;i<Faces.Count;i++)
  {
    sInt buffer[KMM_MAXVERT];
    GenMinFace *face = &Faces[i];
    sCopyMem(buffer,face->Vertices,face->Count*4);
    for(sInt j=0;j<face->Count;j++)
      face->Vertices[j] = buffer[face->Count-1-j];
  }

  ChangeTopo();
}

void GenMinMesh::CalcBBox(sAABox &box) const
{
  box.Min.Init( 1e+15f,  1e+15f,  1e+15f, 1.0f);
  box.Max.Init(-1e+15f, -1e+15f, -1e+15f, 1.0f);

  for(sInt i=0;i<Faces.Count;i++)
  {
    const GenMinFace *face = &Faces[i];

    for(sInt j=0;j<face->Count;j++)
    {
      const GenMinVert *vert = &Vertices[face->Vertices[j]];
      box.Min.x = sMin(box.Min.x,vert->Pos.x);
      box.Min.y = sMin(box.Min.y,vert->Pos.y);
      box.Min.z = sMin(box.Min.z,vert->Pos.z);
      box.Max.x = sMax(box.Max.x,vert->Pos.x);
      box.Max.y = sMax(box.Max.y,vert->Pos.y);
      box.Max.z = sMax(box.Max.z,vert->Pos.z);
    }
  }
}

void GenMinMesh::SelectAll(sInt in,sInt out)
{
  GenMinVert *vp;
  sInt ok;

  vp = Vertices.Array;
  for(sInt i=0;i<Vertices.Count;i++)
  {
    ok = (in==MMU_ALL || (in==MMU_SELECTED&&vp->Select) || (in==MMU_UNSELECTED&&!vp->Select));

    switch(out)
    {
    case MMS_ADD:
      if(ok) vp->Select = 1;
      break;
    case MMS_SUB:
      if(ok) vp->Select = 0;
      break;
    case MMS_SET:
      vp->Select = ok;
      break;
    case MMS_SETNOT:
      vp->Select = !ok;
      break;
    }
    vp++;
  }
}

/****************************************************************************/

sInt *GenMinMesh::CalcMergeVerts() const
{
  sInt *next = new sInt[Vertices.Count];
  sInt *remap = new sInt[Vertices.Count];
  sInt hashSize = 1024;

  while(Vertices.Count > hashSize * 8)
    hashSize *= 2;

  sInt unique = 0;

  // alloc+clear hash table
  sInt *hash = new sInt[hashSize];
  sSetMem(hash,0xff,hashSize*sizeof(sInt));

  // find vertices with identical position
  for(sInt i=0;i<Vertices.Count;i++)
  {
    // calc bucket
    const GenMinVert *vert = &Vertices[i];
    sU32 bucket = HashVector(vert->Pos) & (hashSize - 1);

    // search for first matching vertex
    sInt first;
    for(first = hash[bucket]; first != -1; first = next[first])
    {
      if(!sCmpMem(&Vertices[first].Pos,&vert->Pos,sizeof(GenMinVector)))
      {
        remap[i] = first;
        break;
      }
    }

    // append to hash list if it's the first at that position
    if(first == -1)
    {
      remap[i] = i;
      next[i] = hash[bucket];
      hash[bucket] = i;

      unique++;
    }
  }

  sDPrintF("%d unique verts\n",unique);

  delete[] hash;
  delete[] next;
  return remap;
}

/****************************************************************************/

struct GenMinTempEdge
{
  sInt FaceVert;
  sInt v0,v1;     // v0 < v1!

  bool operator < (const GenMinTempEdge &b) const
  {
    return v0 < b.v0 || v0 == b.v0 && v1 < b.v1;
  }
};

static void SiftDownEdge(GenMinTempEdge *list,sInt n,sInt k)
{
  GenMinTempEdge v = list[k];

  while(k < (n >> 1))
  {
    sInt j = k*2+1;
    if(j+1 < n && list[j] < list[j+1])
      j++;

    if(!(v < list[j]))
      break;

    list[k] = list[j];
    k = j;
  }

  list[k] = v;
}

static void HeapSortEdges(GenMinTempEdge *list,sInt n)
{
  for(sInt k=(n >> 1)-1;k>=0;k--)
    SiftDownEdge(list,n,k);

  while(--n > 0)
  {
    sSwap(list[0],list[n]);
    SiftDownEdge(list,n,0);
  }
}

static void ConnectFaces(GenMinFace *faces,const sInt *buf,sInt count)
{
  sVERIFY(count <= 2);

  if(count == 1) // boundary
    faces[*buf >> 3].Adjacent[*buf & 7] = -1;
  else if(count == 2) // inner (shared) edge
  {
    sInt f0 = buf[0], f1 = buf[1];
    faces[f0 >> 3].Adjacent[f0 & 7] = f1;
    faces[f1 >> 3].Adjacent[f1 & 7] = f0;
  }
}

void GenMinMesh::CalcAdjacency()
{
  // calculate number of edges we need
  sInt numEdges = 0;
  for(sInt i=0;i<Faces.Count;i++)
    numEdges += Faces[i].Count >= 3 ? Faces[i].Count : 0;

  // calc merge list
  sInt *remap = CalcMergeVerts();

  // make edge list
  GenMinTempEdge *edges = new GenMinTempEdge[numEdges], *edgePtr = edges;
  for(sInt i=0;i<Faces.Count;i++)
  {
    GenMinFace *face = &Faces[i];
    if(face->Count < 3)
      continue;

    for(sInt j=0;j<face->Count;j++)
    {
      edgePtr->FaceVert = i * 8 + j;
      edgePtr->v0 = remap[face->Vertices[j]];
      edgePtr->v1 = remap[face->Vertices[(j + 1 == face->Count) ? 0 : j+1]];
      if(edgePtr->v0 > edgePtr->v1)
        sSwap(edgePtr->v0,edgePtr->v1);

      edgePtr++;
    }
  }

  // sort edge list (using heapsort)
  HeapSortEdges(edges,numEdges);

  // generate adjacency
  sInt last0 = -1, last1 = -1, count = 0, temp[2];
  for(sInt i=0;i<numEdges;i++)
  {
    edgePtr = &edges[i];

    if(last0 == edgePtr->v0 && last1 == edgePtr->v1)
    {
      temp[count++] = edgePtr->FaceVert;
      if(count == 2) // got a complete edge
      {
        ConnectFaces(Faces.Array,temp,count);
        count = 0;
      }
    }
    else
    {
      ConnectFaces(Faces.Array,temp,count);

      temp[0] = edgePtr->FaceVert;
      count = 1;
    }

    last0 = edgePtr->v0;
    last1 = edgePtr->v1;
  }

  ConnectFaces(Faces.Array,temp,count);

  // cleanup
  delete[] remap;
  delete[] edges;
}

void GenMinMesh::VerifyAdjacency()
{
  for(sInt i=0;i<Faces.Count;i++)
  {
    GenMinFace *face = &Faces[i];
    if(face->Count < 3)
      continue;
    
    for(sInt j=0;j<face->Count;j++)
    {
      sInt opposite = face->Adjacent[j];
      if(opposite != -1)
        sVERIFY(Faces[opposite >> 3].Adjacent[opposite & 7] == i*8+j);
    }
  }
}

/****************************************************************************/
/*

#if !sPLAYER

void GenMinMesh::PrepareWire(sInt flags,sU32 selMask)
{
  if(!WireMesh || WireFlags != flags || WireSelMask != selMask)
  {
    UnPrepareWire();

    WireMesh = new EngMesh;
    WireMesh->FromGenMinMeshWire(this,flags,selMask);
    WireFlags = flags;
    WireSelMask = selMask;
  }
}

void GenMinMesh::UnPrepareWire()
{
  if(WireMesh)
  {
    WireMesh->Release();
    WireMesh = 0;
  }
}

#endif
*/
/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Ops                                                                ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

sBool CheckMinMesh(GenMinMesh *&mesh)
{
  GenMinMesh *oldmesh;
  if(mesh==0)
    return 1;
//  if(mesh->ClassId!=KC_MINMESH)
//    return 1;
  if(mesh->RefCount>1) 
  {
    oldmesh = mesh;
    mesh = new GenMinMesh;
    mesh->Copy(oldmesh);
    oldmesh->Release();
  }
  return 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Generator Ops                                                      ***/
/***                                                                      ***/
/****************************************************************************/

GenMinMesh * __stdcall MinMesh_SingleVert()
{
  GenMinMesh *mesh;
  GenMinVert *vp;
  GenMinFace *fp;

  mesh = new GenMinMesh;

  vp = mesh->Vertices.Add();
  sSetMem(vp,0,sizeof(*vp));
  vp->Color = ~0;

  fp = mesh->Faces.Add();
  sSetMem(fp,0,sizeof(*fp));
  fp->Count = 1;
  fp->Cluster = 1;

  return mesh;
}

GenMinMesh * __stdcall MinMesh_Grid(sInt mode,sInt tx,sInt ty)
{
  GenMinMesh *mesh;
  sMatrix mat;

  mesh = new GenMinMesh;

  mesh->MakeGrid(tx,ty,0);
  mat.Init();
  mat.i.Init(-1,0,0,0);
  mat.j.Init(0,0,1,0);
  mesh->Transform(MMU_ALL,mat);
  if(mode&1)
  {
    mesh->SelectAll(MMU_ALL,MMS_SETNOT);
    mesh->MakeGrid(tx,ty,0);
    mat.Init();
    mat.i.Init(1,0,0,0);
    mat.j.Init(0,0,1,0);
    mesh->Transform(MMU_SELECTED,mat);
  }
  mesh->SelectAll(MMU_ALL,MMS_SET);

  return mesh;
}

GenMinMesh * __stdcall MinMesh_Cube(sInt tx,sInt ty,sInt tz,sInt flags,sFSRT srt)
{
  GenMinMesh *mesh;
  sMatrix mat;
  sInt *tess=&tx;

  const static sS8 cube[6][8] =
  {
    { 0,1,  1, 1,  0, 0,-1 ,1 },  
    { 2,1, -1, 1, -1, 0, 0 ,0 },  
    { 0,1, -1, 1,  0, 0, 1 ,3 },  
    { 2,1,  1, 1,  1, 0, 0 ,2 },  
    { 0,2,  1, 1,  0, 1, 0 ,0 },  
    { 0,2,  1,-1,  0,-1, 0 ,0 },  
  };
  const static sS8 sign[2] = { -1,1 };

  mat.Init();
  mesh = new GenMinMesh;
  for(sInt i=0;i<6;i++)
  {
    sSetMem(&mat,0,sizeof(mat));
    mesh->MakeGrid(tess[cube[i][0]],tess[cube[i][1]],0);

    (&mat.i.x)[cube[i][0]] = cube[i][2];
    (&mat.j.x)[cube[i][1]] = cube[i][3];
    mat.l.Init(cube[i][4]*0.5f,cube[i][5]*0.5f,cube[i][6]*0.5f,1);
    mesh->Transform(MMU_SELECTED,mat);

    if(flags&2)            // wraparound
    {
      mat.Init();
      mat.l.x = cube[i][7];
      mesh->Transform(MMU_SELECTED,mat,1,1);
    }

    mesh->SelectAll(MMU_ALL,MMS_SETNOT);
  }
  mesh->SelectAll(MMU_ALL,MMS_SET);

  // post transform
  mat.InitSRT(srt.v);
  mesh->Transform(MMU_ALL,mat);

  // scale uv)

  if(flags&8)
  {
    mat.Init();
    mat.i.x = srt.s.x;
    mat.j.y = srt.s.y;
    mat.k.z = srt.s.z;
    mesh->Transform(MMU_ALL,mat,1,1);
  }


  // center / bottom
  if(flags&4)
  {
    mat.Init();
    mat.l.y = srt.s.y;
    mesh->Transform(MMU_ALL,mat);
  }


  return mesh;
}

GenMinMesh * __stdcall MinMesh_Torus(sInt tx,sInt ty,sF32 ro,sF32 ri,sF32 phase,sF32 arclen,sInt flags)
{
  GenMinMesh *mesh;
  GenMinVert *vp;
  sF32 fx,fy;

  mesh = new GenMinMesh;
  mesh->MakeGrid(ty,tx,arclen == 1.0f ? 12 : 8);

  if(flags&1) // absolute radii
  {
    ri = (ro - ri) * 0.5f;
    ro -= ri;
  }

  vp = mesh->Vertices.Array;
  for(sInt i=0;i<mesh->Vertices.Count;i++)
  {
    fx = (1-vp->UV[0][0]-(phase/ty))*sPI2F;
    fy = ((vp->UV[0][1])*arclen)*sPI2F;
    vp->Pos.x = -sFCos(fy)*(ro+sFSin(fx)*ri);
    vp->Pos.y = -sFCos(fx)*ri;
    vp->Pos.z = sFSin(fy)*(ro+sFSin(fx)*ri);
    vp++;
  }

  return mesh;
}

GenMinMesh * __stdcall MinMesh_Sphere(sInt tx,sInt ty)
{
  GenMinMesh *mesh;
  GenMinVert *vp;
  sF32 fx,fy;

  mesh = new GenMinMesh;
  mesh->MakeGrid(tx,ty,3+8);

  vp = mesh->Vertices.Array;
  for(sInt i=0;i<mesh->Vertices.Count-2;i++)
  {
    fx = (1-vp->UV[0][0])*sPI2F;
    fy = ((0.5f/(ty+1.0f))+(vp->UV[0][1]*((ty)/(ty+1.0f))))*sPIF;
    vp->Pos.x = -sFSin(fy)*sFSin(fx)*0.5f;
    vp->Pos.y = sFCos(fy)*0.5f;
    vp->Pos.z = -sFSin(fy)*sFCos(fx)*0.5f;
    vp++;
  }
  vp->Pos.y = mesh->Vertices[0].Pos.y;
  vp++;
  vp->Pos.y = vp[-2].Pos.y;
  vp++;

  return mesh;
}

GenMinMesh * __stdcall MinMesh_Cylinder(sInt tx,sInt ty,sInt,sInt tz)
{
  GenMinMesh *mesh;
  GenMinVert *vp;
  sF32 fx,fy;
  sInt x,y;

  if(tx<3) tx=3;
  if(ty<1) ty=1;
  if(tz<1) tz=1;

  mesh = new GenMinMesh;

  // middle

  vp = mesh->MakeGrid(tx,ty,8);
  for(y=0;y<=ty;y++)
  {
    for(x=0;x<=tx;x++)
    {
      fx = x*sPI2F/tx;
      fy = y*1.0f/ty;
      vp->Pos.x = sFSin(fx)*0.5f;
      vp->Pos.y = fy-0.5f;
      vp->Pos.z = -sFCos(fx)*0.5f;
      vp->UV[0][0] = x*1.0f/tx;
      vp->UV[0][1] = 1-fy;
      vp++;
    }
  }

  // bottom

  vp = mesh->MakeGrid(tx,tz-1,1);
  for(y=0;y<tz;y++)
  {
    for(x=0;x<=tx;x++)
    {
      fx = x*sPI2F/tx;
      fy = (y+1)*0.5f/(tz);
      vp->Pos.x = sFSin(fx)*fy;
      vp->Pos.y = -0.5f;
      vp->Pos.z = -sFCos(fx)*fy;
      vp->UV[0][0] = vp->Pos.x+0.5f;
      vp->UV[0][1] = vp->Pos.z+0.5f;
      vp++;
    }
  }
  vp->Pos.y = -0.5f;
  vp->UV[0][0] = 0.5f;
  vp->UV[0][1] = 0.5f;
  vp++;

  // top

  vp = mesh->MakeGrid(tx,tz-1,2);
  for(y=tz-1;y>=0;y--)
  {
    for(x=0;x<=tx;x++)
    {
      fx = x*sPI2F/tx;
      fy = (y+1)*0.5f/(tz);
      vp->Pos.x = sFSin(fx)*fy;
      vp->Pos.y = 0.5f;
      vp->Pos.z = -sFCos(fx)*fy;
      vp->UV[0][0] = vp->Pos.x+0.5f;
      vp->UV[0][1] = -vp->Pos.z+0.5f;
      vp++;
    }
  }
  vp->Pos.y = 0.5f;
  vp->UV[0][0] = 0.5f;
  vp->UV[0][1] = 0.5f;
  vp++;

  return mesh;
}

GenMinMesh * __stdcall MinMesh_XSI(sChar *filename)
{
  GenMinMesh *mesh;

  mesh = new GenMinMesh;
  mesh->MakeGrid(1,1,0);

  return mesh;
}


/****************************************************************************/
/***                                                                      ***/
/***   Special Ops                                                        ***/
/***                                                                      ***/
/****************************************************************************/
/*
GenMinMesh * __stdcall MinMesh_MatLink(GenMinMesh *mesh,GenMaterial *mtrl,sInt select,sInt pass)
{
  GenMinFace *face;

  if(!mtrl || mtrl->ClassId != KC_MATERIAL || CheckMinMesh(mesh))
    return 0;

  sInt id = mesh->AddCluster(mtrl,pass);
  for(sInt i=0;i<mesh->Faces.Count;i++)
  {
    face = &mesh->Faces[i];
    if(select==0 || (select==1&&face->Select) || (select==2&&!face->Select))
      face->Cluster = id;
  }
  mesh->ChangeTopo();
  mtrl->Release();

  return mesh;
}
*/
/****************************************************************************/

GenMinMesh * __stdcall MinMesh_Add(sInt count,GenMinMesh *mesh,...)
{
  if(CheckMinMesh(mesh)) return 0;
  for(sInt i=1;i<count;i++)
  {
    GenMinMesh *b = (&mesh)[i];
    mesh->Add(b);
    b->Release();
  }
  mesh->ChangeTopo();
  return mesh;
}

/****************************************************************************/

GenMinMesh * __stdcall MinMesh_SelectAll(GenMinMesh *mesh,sU32 mode)
{
  if(CheckMinMesh(mesh)) return 0;

  if(mode&2)
    for(sInt i=0;i<mesh->Vertices.Count;i++)
      mesh->Vertices[i].Select = (mode&1);
  if(mode&4)
    for(sInt i=0;i<mesh->Faces.Count;i++)
      mesh->Faces[i].Select = (mode&1);

  mesh->ChangeGeo();
  return mesh;
}

/****************************************************************************/

sBool SelectLogic(sInt old,sInt sel,sInt mode)
{
  switch(mode&3)
  {
  case MMS_ADD:
    if(sel) return 1;
    break;
  case MMS_SUB:
    if(sel) return 0;
    break;
  case MMS_SET:
    return sel;
    break;
  case MMS_SETNOT:
    return !sel;
    break;
  }
  return old;
}

GenMinMesh * __stdcall MinMesh_SelectCube(GenMinMesh *mesh,sInt mode,sF323 center,sF323 size)
{
  if(CheckMinMesh(mesh)) return 0;

  for(sInt i=0;i<mesh->Vertices.Count;i++)
    mesh->Vertices[i].pad0 = (
      sFAbs(mesh->Vertices[i].Pos.x-center.x)<=size.x &&
      sFAbs(mesh->Vertices[i].Pos.y-center.y)<=size.y && 
      sFAbs(mesh->Vertices[i].Pos.z-center.z)<=size.z);

  switch(mode&12)
  {
  case MMS_VERTEX:
    for(sInt i=0;i<mesh->Vertices.Count;i++)
      mesh->Vertices[i].Select = SelectLogic(mesh->Vertices[i].Select,mesh->Vertices[i].pad0,mode);
    break;
  case MMS_FULLFACE:
    for(sInt i=0;i<mesh->Faces.Count;i++)
    {
      GenMinFace *face = &mesh->Faces[i];
      sBool ok = 1;
      for(sInt j=0;j<face->Count;j++)
        if(!mesh->Vertices[face->Vertices[j]].pad0)
          ok = 0;
      face->Select = SelectLogic(face->Select,ok,mode);
    }
    break;
  case MMS_PARTFACE:
    for(sInt i=0;i<mesh->Faces.Count;i++)
    {
      GenMinFace *face = &mesh->Faces[i];
      sBool ok = 0;
      for(sInt j=0;j<face->Count;j++)
        if(mesh->Vertices[face->Vertices[j]].pad0)
          ok = 1;
      face->Select = SelectLogic(face->Select,ok,mode);
    }
    break;
  }

  mesh->ChangeGeo();
  return mesh;
}


/****************************************************************************/

GenMinMesh * __stdcall MinMesh_DeleteFaces(GenMinMesh *mesh)
{
  if(CheckMinMesh(mesh)) return 0;

  for(sInt i=0;i<mesh->Faces.Count;i++)
    if(mesh->Faces[i].Select)
      mesh->Faces[i].Cluster = 0;

  mesh->ChangeTopo();
  return mesh;
}

/****************************************************************************/

GenMinMesh * __stdcall MinMesh_Invert(GenMinMesh *mesh)
{
  if(CheckMinMesh(mesh)) return 0;

  mesh->Invert();

  return mesh;
}

/****************************************************************************/
/***                                                                      ***/
/***   Geometry Modifiers                                                 ***/
/***                                                                      ***/
/****************************************************************************/

GenMinMesh * __stdcall MinMesh_TransformEx(GenMinMesh *mesh,sInt mask,sFSRT srt)
{
  sMatrix mat;

  if(CheckMinMesh(mesh)) return 0;

  mat.InitSRT(srt.v);
  mesh->Transform(mask&3,mat,(mask>>2)&7,(mask>>5)&7);  
  mesh->ChangeGeo();
  return mesh;
}
/*
GenMinMesh * __stdcall MinMesh_Displace(GenMinMesh *mesh,class GenBitmap *bmp,sInt mask,sF323 ampli)
{
  BilinearContext ctx;
  sU16 height[4];

  if(CheckMinMesh(mesh)) return 0;
  mesh->CalcNormals();

  // prepare bilinear samples
  BilinearSetup(&ctx,bmp->Data,bmp->XSize,bmp->YSize,0);
  sF32 xScale = bmp->XSize * 65536.0f;
  sF32 yScale = bmp->YSize * 65536.0f;

  for(sInt i=0;i<mesh->Vertices.Count;i++)
  {
    GenMinVert *vert = &mesh->Vertices[i];
		if(mask==0 || (mask==1&&vert->Select) || (mask==2&&!vert->Select))
    {
      BilinearFilter(&ctx,(sU64 *)height,vert->UV[0][0]*xScale,vert->UV[0][1]*yScale);
      sF32 h = (height[0]-16384.0f) * (1.0f/32767.0f);
      vert->Pos.x += vert->Normal.x*h*ampli.x;
      vert->Pos.y += vert->Normal.y*h*ampli.y;
      vert->Pos.z += vert->Normal.z*h*ampli.z;
    }
  }
  mesh->ChangeGeo();
  bmp->Release();

  return mesh;
}
*/
GenMinMesh * __stdcall MinMesh_Perlin(GenMinMesh *mesh,sInt mask,sFSRT srt,sF323 ampl)
{
  if(CheckMinMesh(mesh)) return 0;

  sMatrix mat; mat.InitSRT(srt.v);
  sVector amp; amp.Init(ampl.x,ampl.y,ampl.z,0);

  sInt oldcw;

  // setup fpu: single precision, round towards neg. infinity

  __asm
  {
    fstcw   [oldcw];
    push    0143fh;
    fldcw   [esp];
    pop     eax;
  }

	for(sInt i=0;i<mesh->Vertices.Count;i++)
	{
    GenMinVert *vert = &mesh->Vertices[i];
		if(mask==0 || (mask==1&&vert->Select) || (mask==2&&!vert->Select))
    {
      sVector t0,t1,t2;
      sVector pos;
      sF32 fs[3];
      sInt is[3];

      // rotate input vector, calc sampling points
      vert->Pos.Out(pos,1);
      t0.Rotate34(mat,pos);
      for(sInt j=0;j<3;j++)
      {
        is[j] = sFtol(t0[j]);   // integer coordinate
        fs[j] = t0[j] - is[j];  // fractional part
        is[j] &= 255;           // integer grid wraps round 256
        fs[j] = fs[j]*fs[j]*fs[j]*(10.0f+fs[j]*(6.0f*fs[j]-15.0f));
      }

#define ix is[0]
#define iy is[1]
#define iz is[2]
#define P sPerlinPermute
#define G sPerlinGradient3D

		  // trilinear interpolation of grid points
		  t0.Lin3(G[P[P[P[ix]+iy]+iz]&15],G[P[P[P[ix+1]+iy]+iz]&15],fs[0]);
		  t1.Lin3(G[P[P[P[ix]+iy+1]+iz]&15],G[P[P[P[ix+1]+iy+1]+iz]&15],fs[0]);
		  t0.Lin3(t0,t1,fs[1]);
		  t1.Lin3(G[P[P[P[ix]+iy]+iz+1]&15],G[P[P[P[ix+1]+iy]+iz+1]&15],fs[0]);
		  t2.Lin3(G[P[P[P[ix]+iy+1]+iz+1]&15],G[P[P[P[ix+1]+iy+1]+iz+1]&15],fs[0]);
		  t1.Lin3(t1,t2,fs[1]);
		  t0.Lin3(t0,t1,fs[2]);

#undef ix
#undef iy
#undef iz
#undef P
#undef G

      vert->Pos.x += t0.x*amp.x;
      vert->Pos.y += t0.y*amp.y;
      vert->Pos.z += t0.z*amp.z;
    }
	}

  // restore fpu state
  __asm
  {
    fldcw   [oldcw];
  }

  mesh->ChangeGeo();
  return mesh;
}

GenMinMesh * __stdcall MinMesh_ExtrudeNormal(GenMinMesh *mesh,sInt mask,sF32 distance)
{
  if(CheckMinMesh(mesh)) return 0;

  mesh->CalcNormals();
	for(sInt i=0;i<mesh->Vertices.Count;i++)
	{
    GenMinVert *vert = &mesh->Vertices[i];
		if(mask==0 || (mask==1&&vert->Select) || (mask==2&&!vert->Select))
      vert->Pos.AddScale(vert->Normal,distance);
  }

  mesh->ChangeGeo();
  return mesh;
}

GenMinMesh * __stdcall MinMesh_Bend2(GenMinMesh *mesh,sF323 center,sF323 rotate,sF32 len,sF32 angle)
{
  sMatrix mt,mb;
  sVector vt;
  sF32 vx,vy,t,sa,ca;
  sInt i;

  if(CheckMinMesh(mesh)) return 0;

  mt.InitEulerPI2(&rotate.x);
  mt.l.x = -center.x;
  mt.l.y = -center.y;
  mt.l.z = -center.z;
  mb = mt;
  mb.TransR();
  angle *= sPI2F;

  for(i=0;i<mesh->Vertices.Count;i++)
  {
    GenMinVert *vert = &mesh->Vertices[i];

    sVector pos;vert->Pos.Out(pos,1);
    vt.Rotate34(mt,pos);
    t = vt.y;
    if(t>=0.0f)
      vt.y -= sMin(t,len);

    t = sRange(t/len,1.0f,0.0f) * angle;

    sa = sFSin(t);
    ca = sFCos(t);
    vx = vt.x;
    vy = vt.y;
    vt.x = ca * vx - sa * vy;
    vt.y = sa * vx + ca * vy;

    vt.Rotate34(mb);
    vert->Pos.Init(vt);
  }

  mesh->ChangeGeo();
  return mesh;
}

/****************************************************************************/
/***                                                                      ***/
/***   Marching Tetrahedra                                                ***/
/***                                                                      ***/
/****************************************************************************/

enum
{
  MTShift = 4,
  MTGrid = 1<<MTShift,
  MTMask = MTGrid-1,
  MTYStep = MTGrid,
  MTZStep = MTYStep*MTGrid,
  MTMaxVert = 65535,
  // this is the biggest you can make the grid without changing data types!
};

static sInt MTHash[1024];
static sInt MTData[MTMaxVert*2];
static sF32 MTDensity[MTGrid*MTGrid*MTGrid];

// marching cubes tables coming up!
static const sU16 MTEdgeMaskTable[256]=
{
  0x000,0x109,0x203,0x30a,0x406,0x50f,0x605,0x70c,0x80c,0x905,0xa0f,0xb06,0xc0a,0xd03,0xe09,0xf00,
  0x190,0x099,0x393,0x29a,0x596,0x49f,0x795,0x69c,0x99c,0x895,0xb9f,0xa96,0xd9a,0xc93,0xf99,0xe90,
  0x230,0x339,0x033,0x13a,0x636,0x73f,0x435,0x53c,0xa3c,0xb35,0x83f,0x936,0xe3a,0xf33,0xc39,0xd30,
  0x3a0,0x2a9,0x1a3,0x0aa,0x7a6,0x6af,0x5a5,0x4ac,0xbac,0xaa5,0x9af,0x8a6,0xfaa,0xea3,0xda9,0xca0,
  0x460,0x569,0x663,0x76a,0x066,0x16f,0x265,0x36c,0xc6c,0xd65,0xe6f,0xf66,0x86a,0x963,0xa69,0xb60,
  0x5f0,0x4f9,0x7f3,0x6fa,0x1f6,0x0ff,0x3f5,0x2fc,0xdfc,0xcf5,0xfff,0xef6,0x9fa,0x8f3,0xbf9,0xaf0,
  0x650,0x759,0x453,0x55a,0x256,0x35f,0x055,0x15c,0xe5c,0xf55,0xc5f,0xd56,0xa5a,0xb53,0x859,0x950,
  0x7c0,0x6c9,0x5c3,0x4ca,0x3c6,0x2cf,0x1c5,0x0cc,0xfcc,0xec5,0xdcf,0xcc6,0xbca,0xac3,0x9c9,0x8c0,
  0x8c0,0x9c9,0xac3,0xbca,0xcc6,0xdcf,0xec5,0xfcc,0x0cc,0x1c5,0x2cf,0x3c6,0x4ca,0x5c3,0x6c9,0x7c0,
  0x950,0x859,0xb53,0xa5a,0xd56,0xc5f,0xf55,0xe5c,0x15c,0x055,0x35f,0x256,0x55a,0x453,0x759,0x650,
  0xaf0,0xbf9,0x8f3,0x9fa,0xef6,0xfff,0xcf5,0xdfc,0x2fc,0x3f5,0x0ff,0x1f6,0x6fa,0x7f3,0x4f9,0x5f0,
  0xb60,0xa69,0x963,0x86a,0xf66,0xe6f,0xd65,0xc6c,0x36c,0x265,0x16f,0x066,0x76a,0x663,0x569,0x460,
  0xca0,0xda9,0xea3,0xfaa,0x8a6,0x9af,0xaa5,0xbac,0x4ac,0x5a5,0x6af,0x7a6,0x0aa,0x1a3,0x2a9,0x3a0,
  0xd30,0xc39,0xf33,0xe3a,0x936,0x83f,0xb35,0xa3c,0x53c,0x435,0x73f,0x636,0x13a,0x033,0x339,0x230,
  0xe90,0xf99,0xc93,0xd9a,0xa96,0xb9f,0x895,0x99c,0x69c,0x795,0x49f,0x596,0x29a,0x393,0x099,0x190,
  0xf00,0xe09,0xd03,0xc0a,0xb06,0xa0f,0x905,0x80c,0x70c,0x605,0x50f,0x406,0x30a,0x203,0x109,0x000
};

static const sU8 MTTriTable[256][16] =  
{
  {12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12},
  { 0, 8, 3,12,12,12,12,12,12,12,12,12,12,12,12,12},
  { 0, 1, 9,12,12,12,12,12,12,12,12,12,12,12,12,12},
  { 1, 8, 3, 9, 8, 1,12,12,12,12,12,12,12,12,12,12},
  { 1, 2,10,12,12,12,12,12,12,12,12,12,12,12,12,12},
  { 0, 8, 3, 1, 2,10,12,12,12,12,12,12,12,12,12,12},
  { 9, 2,10, 0, 2, 9,12,12,12,12,12,12,12,12,12,12},
  { 2, 8, 3, 2,10, 8,10, 9, 8,12,12,12,12,12,12,12},
  { 3,11, 2,12,12,12,12,12,12,12,12,12,12,12,12,12},
  { 0,11, 2, 8,11, 0,12,12,12,12,12,12,12,12,12,12},
  { 1, 9, 0, 2, 3,11,12,12,12,12,12,12,12,12,12,12},
  { 1,11, 2, 1, 9,11, 9, 8,11,12,12,12,12,12,12,12},
  { 3,10, 1,11,10, 3,12,12,12,12,12,12,12,12,12,12},
  { 0,10, 1, 0, 8,10, 8,11,10,12,12,12,12,12,12,12},
  { 3, 9, 0, 3,11, 9,11,10, 9,12,12,12,12,12,12,12},
  { 9, 8,10,10, 8,11,12,12,12,12,12,12,12,12,12,12},
  { 4, 7, 8,12,12,12,12,12,12,12,12,12,12,12,12,12},
  { 4, 3, 0, 7, 3, 4,12,12,12,12,12,12,12,12,12,12},
  { 0, 1, 9, 8, 4, 7,12,12,12,12,12,12,12,12,12,12},
  { 4, 1, 9, 4, 7, 1, 7, 3, 1,12,12,12,12,12,12,12},
  { 1, 2,10, 8, 4, 7,12,12,12,12,12,12,12,12,12,12},
  { 3, 4, 7, 3, 0, 4, 1, 2,10,12,12,12,12,12,12,12},
  { 9, 2,10, 9, 0, 2, 8, 4, 7,12,12,12,12,12,12,12},
  { 2,10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4,12,12,12,12},
  { 8, 4, 7, 3,11, 2,12,12,12,12,12,12,12,12,12,12},
  {11, 4, 7,11, 2, 4, 2, 0, 4,12,12,12,12,12,12,12},
  { 9, 0, 1, 8, 4, 7, 2, 3,11,12,12,12,12,12,12,12},
  { 4, 7,11, 9, 4,11, 9,11, 2, 9, 2, 1,12,12,12,12},
  { 3,10, 1, 3,11,10, 7, 8, 4,12,12,12,12,12,12,12},
  { 1,11,10, 1, 4,11, 1, 0, 4, 7,11, 4,12,12,12,12},
  { 4, 7, 8, 9, 0,11, 9,11,10,11, 0, 3,12,12,12,12},
  { 4, 7,11, 4,11, 9, 9,11,10,12,12,12,12,12,12,12},
  { 9, 5, 4,12,12,12,12,12,12,12,12,12,12,12,12,12},
  { 9, 5, 4, 0, 8, 3,12,12,12,12,12,12,12,12,12,12},
  { 0, 5, 4, 1, 5, 0,12,12,12,12,12,12,12,12,12,12},
  { 8, 5, 4, 8, 3, 5, 3, 1, 5,12,12,12,12,12,12,12},
  { 1, 2,10, 9, 5, 4,12,12,12,12,12,12,12,12,12,12},
  { 3, 0, 8, 1, 2,10, 4, 9, 5,12,12,12,12,12,12,12},
  { 5, 2,10, 5, 4, 2, 4, 0, 2,12,12,12,12,12,12,12},
  { 2,10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8,12,12,12,12},
  { 9, 5, 4, 2, 3,11,12,12,12,12,12,12,12,12,12,12},
  { 0,11, 2, 0, 8,11, 4, 9, 5,12,12,12,12,12,12,12},
  { 0, 5, 4, 0, 1, 5, 2, 3,11,12,12,12,12,12,12,12},
  { 2, 1, 5, 2, 5, 8, 2, 8,11, 4, 8, 5,12,12,12,12},
  {10, 3,11,10, 1, 3, 9, 5, 4,12,12,12,12,12,12,12},
  { 4, 9, 5, 0, 8, 1, 8,10, 1, 8,11,10,12,12,12,12},
  { 5, 4, 0, 5, 0,11, 5,11,10,11, 0, 3,12,12,12,12},
  { 5, 4, 8, 5, 8,10,10, 8,11,12,12,12,12,12,12,12},
  { 9, 7, 8, 5, 7, 9,12,12,12,12,12,12,12,12,12,12},
  { 9, 3, 0, 9, 5, 3, 5, 7, 3,12,12,12,12,12,12,12},
  { 0, 7, 8, 0, 1, 7, 1, 5, 7,12,12,12,12,12,12,12},
  { 1, 5, 3, 3, 5, 7,12,12,12,12,12,12,12,12,12,12},
  { 9, 7, 8, 9, 5, 7,10, 1, 2,12,12,12,12,12,12,12},
  {10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3,12,12,12,12},
  { 8, 0, 2, 8, 2, 5, 8, 5, 7,10, 5, 2,12,12,12,12},
  { 2,10, 5, 2, 5, 3, 3, 5, 7,12,12,12,12,12,12,12},
  { 7, 9, 5, 7, 8, 9, 3,11, 2,12,12,12,12,12,12,12},
  { 9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7,11,12,12,12,12},
  { 2, 3,11, 0, 1, 8, 1, 7, 8, 1, 5, 7,12,12,12,12},
  {11, 2, 1,11, 1, 7, 7, 1, 5,12,12,12,12,12,12,12},
  { 9, 5, 8, 8, 5, 7,10, 1, 3,10, 3,11,12,12,12,12},
  { 5, 7, 0, 5, 0, 9, 7,11, 0, 1, 0,10,11,10, 0,12},
  {11,10, 0,11, 0, 3,10, 5, 0, 8, 0, 7, 5, 7, 0,12},
  {11,10, 5, 7,11, 5,12,12,12,12,12,12,12,12,12,12},
  {10, 6, 5,12,12,12,12,12,12,12,12,12,12,12,12,12},
  { 0, 8, 3, 5,10, 6,12,12,12,12,12,12,12,12,12,12},
  { 9, 0, 1, 5,10, 6,12,12,12,12,12,12,12,12,12,12},
  { 1, 8, 3, 1, 9, 8, 5,10, 6,12,12,12,12,12,12,12},
  { 1, 6, 5, 2, 6, 1,12,12,12,12,12,12,12,12,12,12},
  { 1, 6, 5, 1, 2, 6, 3, 0, 8,12,12,12,12,12,12,12},
  { 9, 6, 5, 9, 0, 6, 0, 2, 6,12,12,12,12,12,12,12},
  { 5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8,12,12,12,12},
  { 2, 3,11,10, 6, 5,12,12,12,12,12,12,12,12,12,12},
  {11, 0, 8,11, 2, 0,10, 6, 5,12,12,12,12,12,12,12},
  { 0, 1, 9, 2, 3,11, 5,10, 6,12,12,12,12,12,12,12},
  { 5,10, 6, 1, 9, 2, 9,11, 2, 9, 8,11,12,12,12,12},
  { 6, 3,11, 6, 5, 3, 5, 1, 3,12,12,12,12,12,12,12},
  { 0, 8,11, 0,11, 5, 0, 5, 1, 5,11, 6,12,12,12,12},
  { 3,11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9,12,12,12,12},
  { 6, 5, 9, 6, 9,11,11, 9, 8,12,12,12,12,12,12,12},
  { 5,10, 6, 4, 7, 8,12,12,12,12,12,12,12,12,12,12},
  { 4, 3, 0, 4, 7, 3, 6, 5,10,12,12,12,12,12,12,12},
  { 1, 9, 0, 5,10, 6, 8, 4, 7,12,12,12,12,12,12,12},
  {10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4,12,12,12,12},
  { 6, 1, 2, 6, 5, 1, 4, 7, 8,12,12,12,12,12,12,12},
  { 1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7,12,12,12,12},
  { 8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6,12,12,12,12},
  { 7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9,12},
  { 3,11, 2, 7, 8, 4,10, 6, 5,12,12,12,12,12,12,12},
  { 5,10, 6, 4, 7, 2, 4, 2, 0, 2, 7,11,12,12,12,12},
  { 0, 1, 9, 4, 7, 8, 2, 3,11, 5,10, 6,12,12,12,12},
  { 9, 2, 1, 9,11, 2, 9, 4,11, 7,11, 4, 5,10, 6,12},
  { 8, 4, 7, 3,11, 5, 3, 5, 1, 5,11, 6,12,12,12,12},
  { 5, 1,11, 5,11, 6, 1, 0,11, 7,11, 4, 0, 4,11,12},
  { 0, 5, 9, 0, 6, 5, 0, 3, 6,11, 6, 3, 8, 4, 7,12},
  { 6, 5, 9, 6, 9,11, 4, 7, 9, 7,11, 9,12,12,12,12},
  {10, 4, 9, 6, 4,10,12,12,12,12,12,12,12,12,12,12},
  { 4,10, 6, 4, 9,10, 0, 8, 3,12,12,12,12,12,12,12},
  {10, 0, 1,10, 6, 0, 6, 4, 0,12,12,12,12,12,12,12},
  { 8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1,10,12,12,12,12},
  { 1, 4, 9, 1, 2, 4, 2, 6, 4,12,12,12,12,12,12,12},
  { 3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4,12,12,12,12},
  { 0, 2, 4, 4, 2, 6,12,12,12,12,12,12,12,12,12,12},
  { 8, 3, 2, 8, 2, 4, 4, 2, 6,12,12,12,12,12,12,12},
  {10, 4, 9,10, 6, 4,11, 2, 3,12,12,12,12,12,12,12},
  { 0, 8, 2, 2, 8,11, 4, 9,10, 4,10, 6,12,12,12,12},
  { 3,11, 2, 0, 1, 6, 0, 6, 4, 6, 1,10,12,12,12,12},
  { 6, 4, 1, 6, 1,10, 4, 8, 1, 2, 1,11, 8,11, 1,12},
  { 9, 6, 4, 9, 3, 6, 9, 1, 3,11, 6, 3,12,12,12,12},
  { 8,11, 1, 8, 1, 0,11, 6, 1, 9, 1, 4, 6, 4, 1,12},
  { 3,11, 6, 3, 6, 0, 0, 6, 4,12,12,12,12,12,12,12},
  { 6, 4, 8,11, 6, 8,12,12,12,12,12,12,12,12,12,12},
  { 7,10, 6, 7, 8,10, 8, 9,10,12,12,12,12,12,12,12},
  { 0, 7, 3, 0,10, 7, 0, 9,10, 6, 7,10,12,12,12,12},
  {10, 6, 7, 1,10, 7, 1, 7, 8, 1, 8, 0,12,12,12,12},
  {10, 6, 7,10, 7, 1, 1, 7, 3,12,12,12,12,12,12,12},
  { 1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7,12,12,12,12},
  { 2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9,12},
  { 7, 8, 0, 7, 0, 6, 6, 0, 2,12,12,12,12,12,12,12},
  { 7, 3, 2, 6, 7, 2,12,12,12,12,12,12,12,12,12,12},
  { 2, 3,11,10, 6, 8,10, 8, 9, 8, 6, 7,12,12,12,12},
  { 2, 0, 7, 2, 7,11, 0, 9, 7, 6, 7,10, 9,10, 7,12},
  { 1, 8, 0, 1, 7, 8, 1,10, 7, 6, 7,10, 2, 3,11,12},
  {11, 2, 1,11, 1, 7,10, 6, 1, 6, 7, 1,12,12,12,12},
  { 8, 9, 6, 8, 6, 7, 9, 1, 6,11, 6, 3, 1, 3, 6,12},
  { 0, 9, 1,11, 6, 7,12,12,12,12,12,12,12,12,12,12},
  { 7, 8, 0, 7, 0, 6, 3,11, 0,11, 6, 0,12,12,12,12},
  { 7,11, 6,12,12,12,12,12,12,12,12,12,12,12,12,12},
  { 7, 6,11,12,12,12,12,12,12,12,12,12,12,12,12,12},
  { 3, 0, 8,11, 7, 6,12,12,12,12,12,12,12,12,12,12},
  { 0, 1, 9,11, 7, 6,12,12,12,12,12,12,12,12,12,12},
  { 8, 1, 9, 8, 3, 1,11, 7, 6,12,12,12,12,12,12,12},
  {10, 1, 2, 6,11, 7,12,12,12,12,12,12,12,12,12,12},
  { 1, 2,10, 3, 0, 8, 6,11, 7,12,12,12,12,12,12,12},
  { 2, 9, 0, 2,10, 9, 6,11, 7,12,12,12,12,12,12,12},
  { 6,11, 7, 2,10, 3,10, 8, 3,10, 9, 8,12,12,12,12},
  { 7, 2, 3, 6, 2, 7,12,12,12,12,12,12,12,12,12,12},
  { 7, 0, 8, 7, 6, 0, 6, 2, 0,12,12,12,12,12,12,12},
  { 2, 7, 6, 2, 3, 7, 0, 1, 9,12,12,12,12,12,12,12},
  { 1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6,12,12,12,12},
  {10, 7, 6,10, 1, 7, 1, 3, 7,12,12,12,12,12,12,12},
  {10, 7, 6, 1, 7,10, 1, 8, 7, 1, 0, 8,12,12,12,12},
  { 0, 3, 7, 0, 7,10, 0,10, 9, 6,10, 7,12,12,12,12},
  { 7, 6,10, 7,10, 8, 8,10, 9,12,12,12,12,12,12,12},
  { 6, 8, 4,11, 8, 6,12,12,12,12,12,12,12,12,12,12},
  { 3, 6,11, 3, 0, 6, 0, 4, 6,12,12,12,12,12,12,12},
  { 8, 6,11, 8, 4, 6, 9, 0, 1,12,12,12,12,12,12,12},
  { 9, 4, 6, 9, 6, 3, 9, 3, 1,11, 3, 6,12,12,12,12},
  { 6, 8, 4, 6,11, 8, 2,10, 1,12,12,12,12,12,12,12},
  { 1, 2,10, 3, 0,11, 0, 6,11, 0, 4, 6,12,12,12,12},
  { 4,11, 8, 4, 6,11, 0, 2, 9, 2,10, 9,12,12,12,12},
  {10, 9, 3,10, 3, 2, 9, 4, 3,11, 3, 6, 4, 6, 3,12},
  { 8, 2, 3, 8, 4, 2, 4, 6, 2,12,12,12,12,12,12,12},
  { 0, 4, 2, 4, 6, 2,12,12,12,12,12,12,12,12,12,12},
  { 1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8,12,12,12,12},
  { 1, 9, 4, 1, 4, 2, 2, 4, 6,12,12,12,12,12,12,12},
  { 8, 1, 3, 8, 6, 1, 8, 4, 6, 6,10, 1,12,12,12,12},
  {10, 1, 0,10, 0, 6, 6, 0, 4,12,12,12,12,12,12,12},
  { 4, 6, 3, 4, 3, 8, 6,10, 3, 0, 3, 9,10, 9, 3,12},
  {10, 9, 4, 6,10, 4,12,12,12,12,12,12,12,12,12,12},
  { 4, 9, 5, 7, 6,11,12,12,12,12,12,12,12,12,12,12},
  { 0, 8, 3, 4, 9, 5,11, 7, 6,12,12,12,12,12,12,12},
  { 5, 0, 1, 5, 4, 0, 7, 6,11,12,12,12,12,12,12,12},
  {11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5,12,12,12,12},
  { 9, 5, 4,10, 1, 2, 7, 6,11,12,12,12,12,12,12,12},
  { 6,11, 7, 1, 2,10, 0, 8, 3, 4, 9, 5,12,12,12,12},
  { 7, 6,11, 5, 4,10, 4, 2,10, 4, 0, 2,12,12,12,12},
  { 3, 4, 8, 3, 5, 4, 3, 2, 5,10, 5, 2,11, 7, 6,12},
  { 7, 2, 3, 7, 6, 2, 5, 4, 9,12,12,12,12,12,12,12},
  { 9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7,12,12,12,12},
  { 3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0,12,12,12,12},
  { 6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8,12},
  { 9, 5, 4,10, 1, 6, 1, 7, 6, 1, 3, 7,12,12,12,12},
  { 1, 6,10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4,12},
  { 4, 0,10, 4,10, 5, 0, 3,10, 6,10, 7, 3, 7,10,12},
  { 7, 6,10, 7,10, 8, 5, 4,10, 4, 8,10,12,12,12,12},
  { 6, 9, 5, 6,11, 9,11, 8, 9,12,12,12,12,12,12,12},
  { 3, 6,11, 0, 6, 3, 0, 5, 6, 0, 9, 5,12,12,12,12},
  { 0,11, 8, 0, 5,11, 0, 1, 5, 5, 6,11,12,12,12,12},
  { 6,11, 3, 6, 3, 5, 5, 3, 1,12,12,12,12,12,12,12},
  { 1, 2,10, 9, 5,11, 9,11, 8,11, 5, 6,12,12,12,12},
  { 0,11, 3, 0, 6,11, 0, 9, 6, 5, 6, 9, 1, 2,10,12},
  {11, 8, 5,11, 5, 6, 8, 0, 5,10, 5, 2, 0, 2, 5,12},
  { 6,11, 3, 6, 3, 5, 2,10, 3,10, 5, 3,12,12,12,12},
  { 5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2,12,12,12,12},
  { 9, 5, 6, 9, 6, 0, 0, 6, 2,12,12,12,12,12,12,12},
  { 1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8,12},
  { 1, 5, 6, 2, 1, 6,12,12,12,12,12,12,12,12,12,12},
  { 1, 3, 6, 1, 6,10, 3, 8, 6, 5, 6, 9, 8, 9, 6,12},
  {10, 1, 0,10, 0, 6, 9, 5, 0, 5, 6, 0,12,12,12,12},
  { 0, 3, 8, 5, 6,10,12,12,12,12,12,12,12,12,12,12},
  {10, 5, 6,12,12,12,12,12,12,12,12,12,12,12,12,12},
  {11, 5,10, 7, 5,11,12,12,12,12,12,12,12,12,12,12},
  {11, 5,10,11, 7, 5, 8, 3, 0,12,12,12,12,12,12,12},
  { 5,11, 7, 5,10,11, 1, 9, 0,12,12,12,12,12,12,12},
  {10, 7, 5,10,11, 7, 9, 8, 1, 8, 3, 1,12,12,12,12},
  {11, 1, 2,11, 7, 1, 7, 5, 1,12,12,12,12,12,12,12},
  { 0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2,11,12,12,12,12},
  { 9, 7, 5, 9, 2, 7, 9, 0, 2, 2,11, 7,12,12,12,12},
  { 7, 5, 2, 7, 2,11, 5, 9, 2, 3, 2, 8, 9, 8, 2,12},
  { 2, 5,10, 2, 3, 5, 3, 7, 5,12,12,12,12,12,12,12},
  { 8, 2, 0, 8, 5, 2, 8, 7, 5,10, 2, 5,12,12,12,12},
  { 9, 0, 1, 5,10, 3, 5, 3, 7, 3,10, 2,12,12,12,12},
  { 9, 8, 2, 9, 2, 1, 8, 7, 2,10, 2, 5, 7, 5, 2,12},
  { 1, 3, 5, 3, 7, 5,12,12,12,12,12,12,12,12,12,12},
  { 0, 8, 7, 0, 7, 1, 1, 7, 5,12,12,12,12,12,12,12},
  { 9, 0, 3, 9, 3, 5, 5, 3, 7,12,12,12,12,12,12,12},
  { 9, 8, 7, 5, 9, 7,12,12,12,12,12,12,12,12,12,12},
  { 5, 8, 4, 5,10, 8,10,11, 8,12,12,12,12,12,12,12},
  { 5, 0, 4, 5,11, 0, 5,10,11,11, 3, 0,12,12,12,12},
  { 0, 1, 9, 8, 4,10, 8,10,11,10, 4, 5,12,12,12,12},
  {10,11, 4,10, 4, 5,11, 3, 4, 9, 4, 1, 3, 1, 4,12},
  { 2, 5, 1, 2, 8, 5, 2,11, 8, 4, 5, 8,12,12,12,12},
  { 0, 4,11, 0,11, 3, 4, 5,11, 2,11, 1, 5, 1,11,12},
  { 0, 2, 5, 0, 5, 9, 2,11, 5, 4, 5, 8,11, 8, 5,12},
  { 9, 4, 5, 2,11, 3,12,12,12,12,12,12,12,12,12,12},
  { 2, 5,10, 3, 5, 2, 3, 4, 5, 3, 8, 4,12,12,12,12},
  { 5,10, 2, 5, 2, 4, 4, 2, 0,12,12,12,12,12,12,12},
  { 3,10, 2, 3, 5,10, 3, 8, 5, 4, 5, 8, 0, 1, 9,12},
  { 5,10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2,12,12,12,12},
  { 8, 4, 5, 8, 5, 3, 3, 5, 1,12,12,12,12,12,12,12},
  { 0, 4, 5, 1, 0, 5,12,12,12,12,12,12,12,12,12,12},
  { 8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5,12,12,12,12},
  { 9, 4, 5,12,12,12,12,12,12,12,12,12,12,12,12,12},
  { 4,11, 7, 4, 9,11, 9,10,11,12,12,12,12,12,12,12},
  { 0, 8, 3, 4, 9, 7, 9,11, 7, 9,10,11,12,12,12,12},
  { 1,10,11, 1,11, 4, 1, 4, 0, 7, 4,11,12,12,12,12},
  { 3, 1, 4, 3, 4, 8, 1,10, 4, 7, 4,11,10,11, 4,12},
  { 4,11, 7, 9,11, 4, 9, 2,11, 9, 1, 2,12,12,12,12},
  { 9, 7, 4, 9,11, 7, 9, 1,11, 2,11, 1, 0, 8, 3,12},
  {11, 7, 4,11, 4, 2, 2, 4, 0,12,12,12,12,12,12,12},
  {11, 7, 4,11, 4, 2, 8, 3, 4, 3, 2, 4,12,12,12,12},
  { 2, 9,10, 2, 7, 9, 2, 3, 7, 7, 4, 9,12,12,12,12},
  { 9,10, 7, 9, 7, 4,10, 2, 7, 8, 7, 0, 2, 0, 7,12},
  { 3, 7,10, 3,10, 2, 7, 4,10, 1,10, 0, 4, 0,10,12},
  { 1,10, 2, 8, 7, 4,12,12,12,12,12,12,12,12,12,12},
  { 4, 9, 1, 4, 1, 7, 7, 1, 3,12,12,12,12,12,12,12},
  { 4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1,12,12,12,12},
  { 4, 0, 3, 7, 4, 3,12,12,12,12,12,12,12,12,12,12},
  { 4, 8, 7,12,12,12,12,12,12,12,12,12,12,12,12,12},
  { 9,10, 8,10,11, 8,12,12,12,12,12,12,12,12,12,12},
  { 3, 0, 9, 3, 9,11,11, 9,10,12,12,12,12,12,12,12},
  { 0, 1,10, 0,10, 8, 8,10,11,12,12,12,12,12,12,12},
  { 3, 1,10,11, 3,10,12,12,12,12,12,12,12,12,12,12},
  { 1, 2,11, 1,11, 9, 9,11, 8,12,12,12,12,12,12,12},
  { 3, 0, 9, 3, 9,11, 1, 2, 9, 2,11, 9,12,12,12,12},
  { 0, 2,11, 8, 0,11,12,12,12,12,12,12,12,12,12,12},
  { 3, 2,11,12,12,12,12,12,12,12,12,12,12,12,12,12},
  { 2, 3, 8, 2, 8,10,10, 8, 9,12,12,12,12,12,12,12},
  { 9,10, 2, 0, 9, 2,12,12,12,12,12,12,12,12,12,12},
  { 2, 3, 8, 2, 8,10, 0, 1, 8, 1,10, 8,12,12,12,12},
  { 1,10, 2,12,12,12,12,12,12,12,12,12,12,12,12,12},
  { 1, 3, 8, 9, 1, 8,12,12,12,12,12,12,12,12,12,12},
  { 0, 9, 1,12,12,12,12,12,12,12,12,12,12,12,12,12},
  { 0, 3, 8,12,12,12,12,12,12,12,12,12,12,12,12,12},
  {12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12}
};

static void MTProcessCell(GenMinMesh *mesh,sInt pos,sF32 isoValue)
{
  enum { X = 1, Y = MTYStep, Z = MTZStep };

  static const sInt Step[8] = {
    0+0+0,0+0+X,0+Y+X,0+Y+0,Z+0+0,Z+0+X,Z+Y+X,Z+Y+0
  };

  static const sInt EdgeOffs[2][12] = {
    { 0+0+0,0+0+X,0+Y+X,0+Y+0,Z+0+0,Z+0+X,Z+Y+X,Z+Y+0,0+0+0,0+0+X,0+Y+X,0+Y+0 },
    { 0+0+X,0+Y+X,0+Y+0,0+0+0,Z+0+X,Z+Y+X,Z+Y+0,Z+0+0,Z+0+0,Z+0+X,Z+Y+X,Z+Y+0 }
  };

  static const sF32 EdgeStartC[3][12] = {
    { 0,1,1,0,0,1,1,0,0,1,1,0 },
    { 0,0,1,1,0,0,1,1,0,0,1,1 },
    { 0,0,0,0,1,1,1,1,0,0,0,0 }
  };

  static const sF32 EdgeDirC[3][12] = {
    {  1, 0,-1, 0, 1, 0,-1, 0, 0, 0, 0, 0 },
    {  0, 1, 0,-1, 0, 1, 0,-1, 0, 0, 0, 0 },
    {  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 }
  };

  static const sInt EdgeCode[12] = {
    (0+0+0)*4+0,(0+0+X)*4+1,(0+Y+0)*4+0,(0+0+0)*4+1,
    (Z+0+0)*4+0,(Z+0+X)*4+1,(Z+Y+0)*4+0,(Z+0+0)*4+1,
    (0+0+0)*4+2,(0+0+X)*4+2,(0+Y+X)*4+2,(0+Y+0)*4+2
  };

  // calc vertex flags
  sInt mask = 0;
  for(sInt i=0,tmask=1;i<8;i++,tmask+=tmask)
    mask |= MTDensity[pos + Step[i]] < isoValue ? tmask : 0;

  // get edge flags
  sInt edgeMask = MTEdgeMaskTable[mask];
  if(!edgeMask)
    return;

  // calc actual position and lookup base
  sInt lookupBase = pos << 2;
  sInt posx = pos & MTMask;
  sInt posy = (pos >> MTShift) & MTMask;
  sInt posz = (pos >> (2*MTShift)) & MTMask;

  // calc edge vertices
  sInt edgeVert[12];
  for(sInt i=0,tmask=edgeMask;i<12;i++,tmask>>=1)
  {
    if((tmask & 1) == 0)
      continue;

    sInt lookupCode = lookupBase + EdgeCode[i];
    sInt bucket = lookupCode & 1023;
    sInt v = -1;

    // lookup vertex in hash table
    for(sInt h=MTHash[bucket];h!=-1;h=MTData[h*2+1])
    {
      if(MTData[h*2] == lookupCode)
      {
        v = h;
        break;
      }
    }

    // not found
    if(v == -1)
    {
      // alloc new vertex
      v = mesh->Vertices.Count++;
      MTData[v*2+0] = lookupCode;
      MTData[v*2+1] = MTHash[bucket];
      MTHash[bucket] = v;

      // calc intersection t
      sF32 d0 = MTDensity[pos + EdgeOffs[0][i]];
      sF32 d1 = MTDensity[pos + EdgeOffs[1][i]];
      sF32 t = (isoValue - d0) / (d1 - d0);
      sVERIFY(t >= 0.0f && t <= 1.0f);

      // calc vertex position
      GenMinVert *vp = &mesh->Vertices[v];
      vp->Pos.x = posx + EdgeStartC[0][i] + t * EdgeDirC[0][i];
      vp->Pos.y = posy + EdgeStartC[1][i] + t * EdgeDirC[1][i];
      vp->Pos.z = posz + EdgeStartC[2][i] + t * EdgeDirC[2][i];
    }

    // store vertex index
    edgeVert[i] = v;
  }

  // create tris
  GenMinFace *face = &mesh->Faces[mesh->Faces.Count];
  sInt ctr = 0;

  for(const sU8 *tris=MTTriTable[mask];*tris != 12;tris++)
  {
    face->Vertices[ctr] = edgeVert[*tris];
    if(++ctr == 3)
    {
      mesh->Faces.Count++;
      face++;
      ctr = 0;
    }
  }
}

GenMinMesh * __stdcall MinMesh_MTetra(sF32 isoValue)
{
  // create mesh, set up vertices and faces as necessary
  GenMinMesh *mesh = new GenMinMesh;
  mesh->Vertices.AtLeast(MTMaxVert);
  mesh->Faces.AtLeast(MTMaxVert*2);

  sSetMem(&mesh->Vertices[0],0,sizeof(GenMinVert)*mesh->Vertices.Alloc);
  for(sInt i=0;i<mesh->Faces.Alloc;i++)
  {
    GenMinFace *face = &mesh->Faces[i];

    face->Select = 0;
    face->Count = 3;
    face->Cluster = 1;
  }

  // clear hash
  sSetMem(MTHash,0xff,sizeof(MTHash));

  // calc density field
  for(sInt z=0;z<MTGrid;z++)
  {
    for(sInt y=0;y<MTGrid;y++)
    {
      for(sInt x=0;x<MTGrid;x++)
      {
        sF32 dx,dy,dz,rsq,dens;

        dens = 0.0f;

        dx = x - 8.0f;
        dy = y - 8.0f;
        dz = z - 8.0f;
        rsq = dx*dx + dy*dy + dz*dz;
        dens += 1.0f / rsq;

        MTDensity[x + y*MTYStep + z*MTZStep] = dens;
      }
    }
  }

  // polygonise it!
  for(sInt z=0;z<MTGrid-1;z++)
  {
    for(sInt y=0;y<MTGrid-1;y++)
    {
      for(sInt x=0;x<MTGrid-1;x++)
        MTProcessCell(mesh,x + y*MTYStep + z*MTZStep,isoValue);
    }
  }

  return mesh;
}

/****************************************************************************/
/***                                                                      ***/
/***   Topology Modifiers                                                 ***/
/***                                                                      ***/
/****************************************************************************/

GenMinMesh * __stdcall MinMesh_Triangulate(GenMinMesh *mesh,sInt mask,sInt threshold,sInt type)
{
  if(CheckMinMesh(mesh)) return 0;

  return mesh;
}

/****************************************************************************/
/****************************************************************************/

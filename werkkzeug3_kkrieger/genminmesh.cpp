// This file is distributed under a BSD license. See LICENSE.txt for details.


#include "genminmesh.hpp"
#include "genmaterial.hpp"
#include "genbitmap.hpp"
#include "genoverlay.hpp"
#include "engine.hpp"

#if sLINK_LOADER
#include "_loader.hpp"
#endif

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

sInt GenMinVector::Classify()
{
  sVector v;
  v.Init(x,y,z);
  return v.Classify();
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

void GenMinMatrix::Init()
{
  BasePose.Init();
  Parent = -1;

  KeyCount = 0;
  SPtr = 0;
  RPtr = 0;
  TPtr = 0;

  Offset = 0;
  Factor = 1;
  Spline = 0;

  NoAnimation.Init();

  Used = 0;
//  Index = -1;
}

void GenMinMatrix::Exit()
{
  sDeleteArray(SPtr);
  sDeleteArray(RPtr);
  sDeleteArray(TPtr);

  sRelease(Spline);
}

/****************************************************************************/

GenMinMeshAnim::GenMinMeshAnim(sInt matrixCount)
{
  Matrices.Init(matrixCount);
  Matrices.Resize(matrixCount);

  for(sInt i=0;i<matrixCount;i++)
    Matrices[i].Init();
}

GenMinMeshAnim::~GenMinMeshAnim()
{
  for(sInt i=0;i<Matrices.Count;i++)
    Matrices[i].Exit();

  Matrices.Exit();
}

sInt GenMinMeshAnim::GetMatrixCount()
{
  return Matrices.Count;
}

GenMinMeshAnim *GenMinMeshAnim::Copy()
{
  GenMinMeshAnim *dest;
  GenMinMatrix *d,*s;

  sInt count = Matrices.Count;
  dest = new GenMinMeshAnim(count);
  for(sInt i=0;i<count;i++)
  {
    s = &Matrices[i];
    d = &dest->Matrices[i];

    d->Init();
    d->BasePose = s->BasePose;
    d->Parent = s->Parent;
    d->Factor = s->Factor;
    d->Offset = s->Offset;
    d->KeyCount = s->KeyCount;
    for(sInt j=0;j<9;j++)
      d->SRT[j] = s->SRT[j];
    if(s->SPtr)
    {
      d->SPtr = new sF32[s->KeyCount*3];
      sCopyMem(d->SPtr,s->SPtr,s->KeyCount*3*4);
    }
    if(s->RPtr)
    {
      d->RPtr = new sF32[s->KeyCount*3];
      sCopyMem(d->RPtr,s->RPtr,s->KeyCount*3*4);
    }
    if(s->TPtr)
    {
      d->TPtr = new sF32[s->KeyCount*3];
      sCopyMem(d->TPtr,s->TPtr,s->KeyCount*3*4);
    }

    if(s->Spline)
    {
      d->Spline = s->Spline;
      d->Spline->AddRef();
    }

    d->NoAnimation = s->NoAnimation;
  }
  return dest;
}


void GenMinMeshAnim::EvalAnimation(sF32 time,sF32 metamorph,sMatrix *matrices)
{
  GenMinMatrix *mp;
  sMatrix mat;
  sF32 srt[9];

  if(time<0)
    time = 0;

  for(sInt i=0;i<Matrices.Count;i++)
  {
    mp = &Matrices.Array[i];
    // get animated matrix
    if(mp->KeyCount)
    {
      sInt k0,k1;
      sF32 f;
      sF32 v0,v1;

      k0 = (sInt)(mp->KeyCount*time*1024);
      f = (k0 & 1023)/1024.0f;
      k0 = (k0/1024) % mp->KeyCount;
      k1 = (k0+1) % mp->KeyCount;

      for(sInt i=0;i<9;i++)
        srt[i] = mp->SRT[i];

      for(sInt i=0;i<3;i++)
      {
        if((&mp->SPtr)[i])
        {
          for(sInt j=0;j<3;j++)
          {
            v0 = (&mp->SPtr)[i][k0*3+j];
            v1 = (&mp->SPtr)[i][k1*3+j];
            srt[i*3+j] = v0 + (v1-v0)*f;
          }
        }
      }

      //srt[0] = srt[1] = srt[2] = 1.0f; // HACK HACK HACK

      mat.InitSRT(srt);
    }
    else if(mp->Spline)
    {
      sF32 dummy;
      mp->Spline->Eval(time*mp->Factor+mp->Offset,time,mat,dummy);
    }
    else
    {
      mat = mp->NoAnimation;
    }

    // concatenate matrices
    if(mp->Parent>=0)
      mp->Temp.MulA(mat,Matrices[mp->Parent].Temp);
    else
      mp->Temp = mat;

    matrices[i].MulA(mp->BasePose,mp->Temp);
  }
}

/****************************************************************************/
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
  ClassId = KC_MINMESH;
  Vertices.Init();
  Faces.Init();
  Clusters.Init();

  Clusters.Add()->Init(0);    // deleted cluster
  AddCluster(0,0);            // default cluster
  Animation = 0;
  NormalsOK = 0;
  ChangeFlag = 3;
  PreparedMesh = 0;
#if !sPLAYER
  WireMesh = 0;
#endif
}

GenMinMesh::~GenMinMesh()
{
  for(sInt i=0;i<Clusters.Count;i++)
    if(Clusters[i].Mtrl)
      Clusters[i].Mtrl->Release();
  sRelease(Animation);

  Vertices.Exit();
  Faces.Exit();
  Clusters.Exit();

  UnPrepare();
#if !sPLAYER
  UnPrepareWire();
#endif
}

void GenMinMesh::Copy(KObject *o)
{
  GenMinMesh *mm;
  sVERIFY(o->ClassId==KC_MINMESH);

  mm = (GenMinMesh *) o;

  for(sInt i=0;i<Clusters.Count;i++)
    if(Clusters[i].Mtrl)
      Clusters[i].Mtrl->Release();
  Clusters.Count = 0;

  if(Animation)
    Animation->Release();

  Vertices.Copy(mm->Vertices);
  Faces.Copy(mm->Faces);
  Clusters.Copy(mm->Clusters);
  if(mm->Animation)
    Animation = mm->Animation->Copy();

  for(sInt i=0;i<Clusters.Count;i++)
    if(Clusters[i].Mtrl)
      Clusters[i].Mtrl->AddRef();

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

sInt GenMinMesh::AddCluster(GenMaterial *mtrl,sInt pass,sInt id,sInt anim,sInt mtx)
{
  if(mtrl==0)
    mtrl = GenOverlayManager->DefaultMat;

  for(sInt i=1;i<Clusters.Count;i++)
    if(Clusters[i].Mtrl == mtrl && Clusters[i].Id == id
      && Clusters[i].AnimType == anim && Clusters[i].AnimMatrix == mtx)
      return i;

  sInt result = Clusters.Count;
  Clusters.Add()->Init(mtrl,pass,id,anim,mtx);
  mtrl->AddRef();
  return result;
}

void GenMinMesh::CreateAnimation(sInt matrixCount)
{
  sRelease(Animation);
  Animation = new GenMinMeshAnim(matrixCount);
}

sInt GenMinMesh::OppositeEdge(sInt edgeTag) const
{
  return Faces[edgeTag >> 3].Adjacent[edgeTag & 7];
}

sInt GenMinMesh::NextFaceEdge(sInt edgeTag) const
{
  const GenMinFace *face = &Faces[edgeTag >> 3];
  if((edgeTag & 7) == face->Count - 1)
    return edgeTag & ~7;
  else
    return edgeTag + 1;
}

sInt GenMinMesh::PrevFaceEdge(sInt edgeTag) const
{
  const GenMinFace *face = &Faces[edgeTag >> 3];
  if((edgeTag & 7) == 0)
    return edgeTag + face->Count - 1;
  else
    return edgeTag - 1;
}

sInt GenMinMesh::NextVertEdge(sInt edgeTag) const
{
  return OppositeEdge(PrevFaceEdge(edgeTag));
}

sInt GenMinMesh::PrevVertEdge(sInt edgeTag) const
{
  return NextFaceEdge(OppositeEdge(edgeTag));
}

// flags:
//
// 0x01  close x
// 0x02  close y
// 0x04  stitch x
// 0x08  stitch y
// 0x10  invert grid
// 0x20  use cluster 0, making the grid invisible/deleted

GenMinVert *GenMinMesh::MakeGrid(sInt tx,sInt ty,sInt flags)
{
  sInt x,y;
  sInt vi,fi,vc,fc,va,fa,bits;
  GenMinVert *vp;
  GenMinFace *fp;
  sInt cluster;

  // additions (top/bottom)


  bits = 0;
  if(flags & 1) { bits++; }
  if(flags & 2) { bits++; }
  fa = tx*bits;
  va = bits;
  if(flags & 4) { fa+=tx+1; }
  if(flags & 8) { fa+=ty+1; }
  cluster = (flags&32)?0:1;


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
      vp->Pos.x = (1.0f*x/tx)-0.5f;
      vp->Pos.y = (1.0f*y/ty)-0.5f;
      vp->UV[0][0] = 1.0f*x/tx;
      vp->UV[0][1] = 1.0f-1.0f*(y)/(ty);
      vp->UV[1][0] = 1.0f*((x==tx)?0:x)/tx;
      vp->UV[1][1] = 1.0f-1.0f*((y==ty)?0:y)/(ty);
      vp->Select = 1;
      vp->Color = 0;//~0;
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
      fp->Cluster = cluster;
      fp->Vertices[0] = vi+(y+0)*(tx+1)+(x+0);
      fp->Vertices[1] = vi+(y+1)*(tx+1)+(x+0);
      fp->Vertices[2] = vi+(y+1)*(tx+1)+(x+1);
      fp->Vertices[3] = vi+(y+0)*(tx+1)+(x+1);
      if(flags & 16)
      {
        sSwap(fp->Vertices[0],fp->Vertices[3]);
        sSwap(fp->Vertices[1],fp->Vertices[2]);
      }
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
      vp->Select = 0;
      vp->Color = 0;//~0;
      vp++;
      for(x=0;x<tx;x++)
      {
        fp->Select = 1;
        fp->Count = 3;
        fp->Cluster = cluster;
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
      mv[p0].Normal.Add(mv[p1].Normal);
      mv[p1].Normal = mv[p0].Normal;
      mv[p0].Tangent.Add(mv[p1].Tangent);
      mv[p1].Tangent = mv[p0].Tangent;
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
  for(sInt i=p;i<Clusters.Count;i++)
    if(Clusters[i].Mtrl)
      Clusters[i].Mtrl->AddRef();

  ChangeTopo();
}

void GenMinMesh::MergeClusters()
{
  sInt *rev;
  sInt revcount;

  for(sInt i=0;i<Clusters.Count;i++)
    Clusters[i].Temp = -1;

  Clusters[0].Temp = 0;
  for(sInt i=0;i<Faces.Count;i++)
    Clusters[Faces[i].Cluster].Temp = 0;

  rev = new sInt[Clusters.Count];
  revcount = 0;

  for(sInt i=0;i<Clusters.Count;i++)
  {
    if(Clusters[i].Temp>=0)
    {
      for(sInt j=0;j<revcount;j++)
      {
        if(Clusters[i].Equals(Clusters[rev[j]]))
        {
          Clusters[i].Temp = Clusters[rev[j]].Temp;
          goto next;
        }
      }
      rev[revcount] = i;
      Clusters[i].Temp = revcount;
      revcount++;
next:;
    }
  }

  for(sInt i=0;i<Faces.Count;i++)
  {
    sVERIFY(Clusters[Faces[i].Cluster].Temp>=0);
    Faces[i].Cluster = Clusters[Faces[i].Cluster].Temp;
  }

  for(sInt i=0;i<revcount;i++)
    if(Clusters[rev[i]].Mtrl)
      Clusters[rev[i]].Mtrl->AddRef();
  for(sInt i=0;i<Clusters.Count;i++)
    if(Clusters[i].Mtrl)
      Clusters[i].Mtrl->Release();
  sVERIFY(revcount>0);
  sVERIFY(rev[0]==0);
  for(sInt i=0;i<revcount;i++)
  {
    sVERIFY(i<=rev[i]);
    Clusters[i] = Clusters[rev[i]];
  }
  Clusters.Count = revcount;

  delete[] rev;
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

/*
// this code does nothing. use it when you think this function does not work
  sInt *remap = new sInt[Vertices.Count];
  for(sInt i=0;i<Vertices.Count;i++)
    remap[i] = i;
  return remap;
*/
  sInt *next = new sInt[Vertices.Count];
  sInt *remap = new sInt[Vertices.Count];
  sInt hashSize = 1024;

  while(Vertices.Count > hashSize * 8)
    hashSize *= 2;

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
      const GenMinVert *cmp = &Vertices[first];

      if(!sCmpMem(&cmp->Pos,&vert->Pos,sizeof(vert->Pos))
        && !sCmpMem(&cmp->Matrix,&vert->Matrix,sizeof(vert->Matrix))
        && !sCmpMem(&cmp->Weights,&vert->Weights,sizeof(vert->Weights)))
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
    }
  }

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

sBool GenMinMesh::CalcAdjacencyCore(const sInt *remap)
{
  // calculate number of edges we need
  sInt numEdges = 0;
  for(sInt i=0;i<Faces.Count;i++)
    numEdges += Faces[i].Count >= 3 ? Faces[i].Count : 0;

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
  sBool closed = sTRUE;

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
      if(count != 0)
        closed = sFALSE;

      ConnectFaces(Faces.Array,temp,count);

      temp[0] = edgePtr->FaceVert;
      count = 1;
    }

    last0 = edgePtr->v0;
    last1 = edgePtr->v1;
  }

  if(count != 0)
    closed = sFALSE;

  ConnectFaces(Faces.Array,temp,count);

  // cleanup
  delete[] edges;

  return closed;
}

sBool GenMinMesh::CalcAdjacency()
{
  sInt *remap = CalcMergeVerts();
  sBool result = CalcAdjacencyCore(remap);
  delete[] remap;

  return result;
}

void GenMinMesh::VerifyAdjacency()
{
  sBool closed = sTRUE;

  for(sInt i=0;i<Faces.Count;i++)
  {
    GenMinFace *face = &Faces[i];
    if(face->Count < 2)
      continue;
    else if(face->Count == 2)
    {
      sInt v0 = face->Vertices[0];
      sInt v1 = face->Vertices[1];

      sVERIFY(!sCmpMem(&Vertices[v0].Pos,&Vertices[v1].Pos,sizeof(GenMinVector)));
    }
    else
    {
      for(sInt j=0;j<face->Count;j++)
      {
        sInt opposite = face->Adjacent[j];
        if(opposite != -1)
        {
          sVERIFY(Faces[opposite >> 3].Adjacent[opposite & 7] == i*8+j);
        }
        else
          closed = sFALSE;
      }
    }
  }

  if(!closed)
    sDPrintF("mesh is not closed\n");
}

/****************************************************************************/

#if sLINK_MINMESH
void GenMinMesh::Prepare()
{
  if(!PreparedMesh)
  {
    PreparedMesh = new EngMesh;
    PreparedMesh->FromGenMinMesh(this);
  }
}
#endif

void GenMinMesh::UnPrepare()
{
  if(PreparedMesh)
  {
    PreparedMesh->Release();
    PreparedMesh = 0;
  }
}

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

void GenMinMesh::BakeAnimation(sF32 fade,sF32 metamorph)
{
  if(!Animation)
    return;

//  sDPrintF("%f\n",fade);

  sMatrix *matrices = new sMatrix[Animation->Matrices.Count];
  Animation->EvalAnimation(fade,metamorph,matrices);

  GenMinVert *vp = Vertices.Array;
  for(sInt i=0;i<Vertices.Count;i++,vp++)
  {
    sVector v0,v1,v;

    if(vp->BoneCount>0)
    {
      v1.Init(0,0,0,0);
      v0.Init(vp->Pos.x,vp->Pos.y,vp->Pos.z,1);

      for(sInt j=0;j<vp->BoneCount;j++)
      {
        v.Rotate34(matrices[vp->Matrix[j]],v0);
        v1.AddScale3(v,vp->Weights[j]);
      }

      vp->Pos.Init(v1);
      vp->BoneCount = 0;
    }
  }

  delete[] matrices;

  sRelease(Animation);
  NormalsOK = 0;
}

void GenMinMesh::AutoStitch()
{
  sInt *merge = CalcMergeVerts();
  sInt *cycleNext = new sInt[Vertices.Count];
  sInt *cycleLast = new sInt[Vertices.Count];

  // build cycle links
  for(sInt i=0;i<Vertices.Count;i++)
  {
    sInt first = merge[i];

    if(first == i)
    {
      cycleNext[i] = i;
      cycleLast[i] = i;
    }
    else
    {
      cycleNext[i] = first;
      cycleNext[cycleLast[first]] = i;
      cycleLast[first] = i;
    }
  }

  // add stitches
  for(sInt i=0;i<Vertices.Count;i++)
  {
    sInt next = cycleNext[i];

    if(next != i)
    {
      GenMinFace *stitch = Faces.Add();

      stitch->Count = 2;
      stitch->Vertices[0] = i;
      stitch->Vertices[1] = next;
    }
  }

  delete[] merge;
  delete[] cycleNext;
  delete[] cycleLast;
}

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
  if(mesh->ClassId!=KC_MINMESH)
    return 1;
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
  vp->Color = 0;//~0;

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
  if(mode&1)
    mesh->MakeGrid(tx,ty,16);
  mat.Init();
  mat.i.Init(-1,0,0,0);
  mat.j.Init(0,0,1,0);
  mesh->Transform(MMU_ALL,mat);
  mesh->SelectAll(MMU_ALL,MMS_SET);
  
  return mesh;
}

GenMinMesh * __stdcall MinMesh_Cube(sInt tx,sInt ty,sInt tz,sInt flags,sFSRT srt)
{
  GenMinMesh *mesh;
  sMatrix mat;
  sInt *tess=&tx;

  const static sS8 cube[6][9] =
  {
    { 0,1,  1, 1,  0, 0,-1 ,1 ,  0 },  
    { 2,1,  1, 1, -1, 0, 0 ,0 , 16 },  
    { 0,1,  1, 1,  0, 0, 1 ,3 , 16 },  
    { 2,1,  1, 1,  1, 0, 0 ,2 ,  0 },  
    { 0,2,  1, 1,  0, 1, 0 ,0 ,  0 },  
    { 0,2,  1, 1,  0,-1, 0 ,0 , 16 },  
  };
  const static sS8 sign[2] = { -1,1 };

  mat.Init();
  mesh = new GenMinMesh;
  for(sInt i=0;i<6;i++)
  {
    sSetMem(&mat,0,sizeof(mat));
    mesh->MakeGrid(tess[cube[i][0]],tess[cube[i][1]],cube[i][8]);

    (&mat.i.x)[cube[i][0]] = cube[i][2];
    (&mat.j.x)[cube[i][1]] = cube[i][3];
    mat.l.Init(cube[i][4]*0.5f,cube[i][5]*0.5f,cube[i][6]*0.5f,1);
    mesh->Transform(MMU_SELECTED,mat);

    mat.Init();
    if(cube[i][8])
    {
      mat.l.x = 1;
      mat.i.x = -1;
    }
    if(flags&2)            // wraparound
    {
      mat.l.x += cube[i][7];
    }
    mesh->Transform(MMU_SELECTED,mat,1,1);

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
    mat.l.y = srt.s.y/2;
    mesh->Transform(MMU_ALL,mat);
  }

  return mesh;
}

GenMinMesh * __stdcall MinMesh_Torus(sInt tx,sInt ty,sF32 ro,sF32 ri,sF32 phase,sF32 arclen,sInt flags)
{
  GenMinMesh *mesh;
  GenMinVert *vp;
  sF32 fx,fy;
  sInt closed;

  closed = (arclen == 1.0f);
  mesh = new GenMinMesh;
  mesh->MakeGrid(ty,tx,closed?12:8+3);

  if(flags&1) // absolute radii
  {
    ri = (ro - ri) * 0.5f;
    ro -= ri;
  }

  vp = mesh->Vertices.Array;
  for(sInt i=0;i<mesh->Vertices.Count;i++)
  {
    fx = (1-vp->UV[1][0]-(phase/ty))*sPI2F;
    fy = ((vp->UV[closed][1])*arclen)*sPI2F;
    vp->Pos.x = -sFCos(fy)*(ro+sFSin(fx)*ri);
    vp->Pos.y = -sFCos(fx)*ri;
    vp->Pos.z = sFSin(fy)*(ro+sFSin(fx)*ri);
    vp++;
  }
  if(!closed)
  {
    vp[-2].Pos.x = -sFCos(arclen*sPI2F)*ro;
    vp[-2].Pos.y = 0;
    vp[-2].Pos.z = sFSin(arclen*sPI2F)* ro;
    vp[-1].Pos.x = -sFCos(0)*ro;
    vp[-1].Pos.y = 0;
    vp[-1].Pos.z = sFSin(0)*ro;
  }

  // center / bottom
  if(flags&2)
  {
    sMatrix mat;
    mat.Init();
    mat.l.y = ri;
    mesh->Transform(MMU_ALL,mat);
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
    fx = (1-vp->UV[1][0])*sPI2F;
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

GenMinMesh * __stdcall MinMesh_Cylinder(sInt tx,sInt ty,sInt flags,sInt tz,sInt arc)
{
  GenMinMesh *mesh;
  GenMinVert *vp;
  sF32 fx,fy;
  sInt x,y;
  sInt closed;
  sInt count;

  if(tx<3) tx=3;
  if(ty<1) ty=1;
  if(tz<1) tz=1;

  count = tx;
  if(arc>tx-1)
    arc = tx-1;
  if(arc>0)
    tx = count - arc + 2;

  closed = (flags & 1) ? 32 : 0;
  mesh = new GenMinMesh;

  // middle

  vp = mesh->MakeGrid(tx,ty,8);
  for(y=0;y<=ty;y++)
  {
    for(x=0;x<=tx;x++)
    {
      fx = ((x==tx)?0:x)*sPI2F/count;
      fy = y*1.0f/ty;
      if(x!=tx-1 || arc==0)
      {
        vp->Pos.x = sFSin(fx)*0.5f;
        vp->Pos.z = -sFCos(fx)*0.5f;
      }
      else
      {
        vp->Pos.x = 0;
        vp->Pos.z = 0;
      }
      vp->Pos.y = fy-0.5f;
      vp->UV[0][0] = x*1.0f/tx;
      vp->UV[0][1] = 1-fy;
      vp++;
    }
  }
  MinMesh_SelectAll(mesh,4);
//  mesh->SelectAll(MMU_ALL,MMS_);

  // bottom

  vp = mesh->MakeGrid(tx,tz-1,closed|1);
  for(y=0;y<tz;y++)
  {
    for(x=0;x<=tx;x++)
    {
      fx = ((x==tx)?0:x)*sPI2F/count;
      fy = (y+1)*0.5f/(tz);
      if(x!=tx-1 || arc==0)
      {
        vp->Pos.x = sFSin(fx)*0.5f;
        vp->Pos.z = -sFCos(fx)*0.5f;
      }
      else
      {
        vp->Pos.x = 0;
        vp->Pos.z = 0;
      }
      vp->Pos.y = -0.5f;
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

  vp = mesh->MakeGrid(tx,tz-1,closed|2);
  for(y=tz-1;y>=0;y--)
  {
    for(x=0;x<=tx;x++)
    {
      fx = ((x==tx)?0:x)*sPI2F/count;
      fy = (y+1)*0.5f/(tz);
      if(x!=tx-1 || arc==0)
      {
        vp->Pos.x = sFSin(fx)*0.5f;
        vp->Pos.z = -sFCos(fx)*0.5f;
      }
      else
      {
        vp->Pos.x = 0;
        vp->Pos.z = 0;
      }
      vp->Pos.y = 0.5f;
      vp->UV[0][0] = vp->Pos.x+0.5f;
      vp->UV[0][1] = -vp->Pos.z+0.5f;
      vp++;
    }
  }
  vp->Pos.y = 0.5f;
  vp->UV[0][0] = 0.5f;
  vp->UV[0][1] = 0.5f;
  vp++;

  // center / bottom
  if(flags&2)
  {
    sMatrix mat; mat.Init();
    mat.l.y = 0.5f;
    mesh->Transform(MMU_ALL,mat);
  }

  return mesh;
}

GenMinMesh * __stdcall MinMesh_XSI(sChar *filename)
{
  GenMinMesh *mesh;
  mesh = new GenMinMesh;
#if sLINK_LOADER
  sLoader::Scene *scene = new sLoader::Scene;
  if(scene->LoadXSI(filename))
  {
    scene->CreateMesh(mesh);
    mesh->AutoStitch();
  }
  else
    mesh->MakeGrid(1,1,0);
  delete scene;
#else
  mesh->MakeGrid(1,1,0);
#endif

  return mesh;
}



static sS16 KopuliData[][12] =
{
  { 3066,6360,-2241,4152,9760,-2241,895,11405,-2241,895,11405,-2241,  },
  { 3066,6360,-2241,895,11405,-2241,-388,8663,-2241,-388,8663,-2241,  },
  { 3066,6360,-2241,-388,8663,-2241,1586,5044,-2241,1586,5044,-2241,  },
  { 3066,6360,-2241,8988,-1,-2241,8001,3399,-2241,8001,3399,-2241,  },
  { 3066,6360,-2241,1586,5044,-2241,8988,-1,-2241,8988,-1,-2241,  },
  { 4152,9760,-2241,6422,10637,-2241,3461,13269,-2241,3461,13269,-2241,  },
  { 4152,9760,-2241,6817,7566,-2241,6422,10637,-2241,6422,10637,-2241,  },
  { 4152,9760,-2241,1191,14037,-2241,895,11405,-2241,895,11405,-2241,  },
  { 4152,9760,-2241,3461,13269,-2241,1191,14037,-2241,1191,14037,-2241,  },
  { 6817,7566,-2241,8594,11953,-2241,6422,10637,-2241,6422,10637,-2241,  },
  { 8594,11953,-2241,9877,11076,-2241,8001,14037,-2241,8001,14037,-2241,  },
  { 8594,11953,-2241,8001,14037,-2241,6422,10637,-2241,6422,10637,-2241,  },
  { 9877,11076,-2241,10666,12501,-2241,8001,14037,-2241,8001,14037,-2241,  },
  { 3461,13269,-2241,5337,15024,-2241,303,15901,-2241,303,15901,-2241,  },
  { 3461,13269,-2241,303,15901,-2241,1191,14037,-2241,1191,14037,-2241,  },
  { 5337,15024,-2241,5731,18423,-2241,303,15901,-2241,303,15901,-2241,  },
  { 5731,18423,-2241,3363,20507,-2241,303,15901,-2241,303,15901,-2241,  },
  { 3363,20507,-2241,402,18752,-2241,303,15901,-2241,303,15901,-2241,  },
  { 1191,14037,-2241,-881,14585,-2241,895,11405,-2241,895,11405,-2241,  },
  { -881,14585,-2241,-5224,16120,-2241,895,11405,-2241,895,11405,-2241,  },
  { -881,14585,-2241,-5224,18423,-2241,-5224,16120,-2241,-5224,16120,-2241,  },
  { -5224,18423,-2241,-8481,16340,-2241,-5224,16120,-2241,-5224,16120,-2241,  },
  { -8481,16340,-2241,-7198,14256,-2241,-5224,16120,-2241,-5224,16120,-2241,  },
  { -388,8663,-2241,204,5483,-2241,1586,5044,-2241,1586,5044,-2241,  },
  { -388,8663,-2241,-1967,6908,-2241,204,5483,-2241,204,5483,-2241,  },
  { -388,8663,-2241,-2461,10747,-2241,-1967,6908,-2241,-1967,6908,-2241,  },
  { -2461,10747,-2241,-4435,7018,-2241,-1967,6908,-2241,-1967,6908,-2241,  },
  { -4435,7018,-2241,-6606,6250,-2241,-4237,3838,-2241,-4237,3838,-2241,  },
  { -4435,7018,-2241,-4237,3838,-2241,-1967,6908,-2241,-1967,6908,-2241,  },
  { -4435,7018,-2241,-6409,9869,-2241,-6606,6250,-2241,-6606,6250,-2241,  },
  { -6409,9869,-2241,-9271,6360,-2241,-6606,6250,-2241,-6606,6250,-2241,  },
  { -9271,6360,-2241,-12331,7084,-2241,-9172,2631,-2241,-9172,2631,-2241,  },
  { -9271,6360,-2241,-9172,2631,-2241,-6606,6250,-2241,-6606,6250,-2241,  },
  { -9271,6360,-2241,-10850,9387,-2241,-12331,7084,-2241,-12331,7084,-2241,  },
  { 204,5483,-2241,-388,2302,-2241,1586,5044,-2241,1586,5044,-2241,  },
  { -388,2302,-2241,895,1206,-2241,1586,5044,-2241,1586,5044,-2241,  },
  { 8988,-1,-2241,12245,3728,-2241,8001,3399,-2241,8001,3399,-2241,  },
  { 12245,3728,-2241,10370,6579,-2241,8001,3399,-2241,8001,3399,-2241,  },
  { 4152,9760,2200,4152,9760,-2241,3066,6360,-2241,3066,6360,-2241,  },
  { 3066,6360,-2241,3066,6360,2200,4152,9760,2200,4152,9760,2200,  },
  { 3066,6360,-2241,8001,3399,-2241,8001,3399,2200,8001,3399,2200,  },
  { 8001,3399,2200,3066,6360,2200,3066,6360,-2241,3066,6360,-2241,  },
  { 6817,7566,2200,6817,7566,-2241,4152,9760,-2241,4152,9760,-2241,  },
  { 4152,9760,-2241,4152,9760,2200,6817,7566,2200,6817,7566,2200,  },
  { 8594,11953,2200,8594,11953,-2241,6817,7566,-2241,6817,7566,-2241,  },
  { 6817,7566,-2241,6817,7566,2200,8594,11953,2200,8594,11953,2200,  },
  { 9877,11076,2200,9877,11076,-2241,8594,11953,-2241,8594,11953,-2241,  },
  { 8594,11953,-2241,8594,11953,2200,9877,11076,2200,9877,11076,2200,  },
  { 10666,12501,2200,10666,12501,-2241,9877,11076,-2241,9877,11076,-2241,  },
  { 9877,11076,-2241,9877,11076,2200,10666,12501,2200,10666,12501,2200,  },
  { 8001,14037,2200,8001,14037,-2241,10666,12501,-2241,10666,12501,-2241,  },
  { 10666,12501,-2241,10666,12501,2200,8001,14037,2200,8001,14037,2200,  },
  { 6422,10637,2200,6422,10637,-2241,8001,14037,-2241,8001,14037,-2241,  },
  { 8001,14037,-2241,8001,14037,2200,6422,10637,2200,6422,10637,2200,  },
  { 3461,13269,2200,3461,13269,-2241,6422,10637,-2241,6422,10637,-2241,  },
  { 6422,10637,-2241,6422,10637,2200,3461,13269,2200,3461,13269,2200,  },
  { 5337,15024,2200,5337,15024,-2241,3461,13269,-2241,3461,13269,-2241,  },
  { 3461,13269,-2241,3461,13269,2200,5337,15024,2200,5337,15024,2200,  },
  { 5731,18423,2200,5731,18423,-2241,5337,15024,-2241,5337,15024,-2241,  },
  { 5337,15024,-2241,5337,15024,2200,5731,18423,2200,5731,18423,2200,  },
  { 3363,20507,2200,3363,20507,-2241,5731,18423,-2241,5731,18423,-2241,  },
  { 5731,18423,-2241,5731,18423,2200,3363,20507,2200,3363,20507,2200,  },
  { 402,18752,2200,402,18752,-2241,3363,20507,-2241,3363,20507,-2241,  },
  { 3363,20507,-2241,3363,20507,2200,402,18752,2200,402,18752,2200,  },
  { 303,15901,2200,303,15901,-2241,402,18752,-2241,402,18752,-2241,  },
  { 402,18752,-2241,402,18752,2200,303,15901,2200,303,15901,2200,  },
  { 1191,14037,2200,1191,14037,-2241,303,15901,-2241,303,15901,-2241,  },
  { 303,15901,-2241,303,15901,2200,1191,14037,2200,1191,14037,2200,  },
  { -881,14585,2200,-881,14585,-2241,1191,14037,-2241,1191,14037,-2241,  },
  { 1191,14037,-2241,1191,14037,2200,-881,14585,2200,-881,14585,2200,  },
  { -5224,18423,2200,-5224,18423,-2241,-881,14585,-2241,-881,14585,-2241,  },
  { -881,14585,-2241,-881,14585,2200,-5224,18423,2200,-5224,18423,2200,  },
  { -8481,16340,2200,-8481,16340,-2241,-5224,18423,-2241,-5224,18423,-2241,  },
  { -5224,18423,-2241,-5224,18423,2200,-8481,16340,2200,-8481,16340,2200,  },
  { -7198,14256,2200,-7198,14256,-2241,-8481,16340,-2241,-8481,16340,-2241,  },
  { -8481,16340,-2241,-8481,16340,2200,-7198,14256,2200,-7198,14256,2200,  },
  { -5224,16120,2200,-5224,16120,-2241,-7198,14256,-2241,-7198,14256,-2241,  },
  { -7198,14256,-2241,-7198,14256,2200,-5224,16120,2200,-5224,16120,2200,  },
  { 895,11405,2200,895,11405,-2241,-5224,16120,-2241,-5224,16120,-2241,  },
  { -5224,16120,-2241,-5224,16120,2200,895,11405,2200,895,11405,2200,  },
  { -388,8663,2200,-388,8663,-2241,895,11405,-2241,895,11405,-2241,  },
  { 895,11405,-2241,895,11405,2200,-388,8663,2200,-388,8663,2200,  },
  { -2461,10747,2200,-2461,10747,-2241,-388,8663,-2241,-388,8663,-2241,  },
  { -388,8663,-2241,-388,8663,2200,-2461,10747,2200,-2461,10747,2200,  },
  { -4435,7018,2200,-4435,7018,-2241,-2461,10747,-2241,-2461,10747,-2241,  },
  { -2461,10747,-2241,-2461,10747,2200,-4435,7018,2200,-4435,7018,2200,  },
  { -6409,9869,2200,-6409,9869,-2241,-4435,7018,-2241,-4435,7018,-2241,  },
  { -4435,7018,-2241,-4435,7018,2200,-6409,9869,2200,-6409,9869,2200,  },
  { -9271,6360,2200,-9271,6360,-2241,-6409,9869,-2241,-6409,9869,-2241,  },
  { -6409,9869,-2241,-6409,9869,2200,-9271,6360,2200,-9271,6360,2200,  },
  { -10850,9387,2200,-10850,9387,-2241,-9271,6360,-2241,-9271,6360,-2241,  },
  { -9271,6360,-2241,-9271,6360,2200,-10850,9387,2200,-10850,9387,2200,  },
  { -12331,7084,2200,-12331,7084,-2241,-10850,9387,-2241,-10850,9387,-2241,  },
  { -10850,9387,-2241,-10850,9387,2200,-12331,7084,2200,-12331,7084,2200,  },
  { -9172,2631,2200,-9172,2631,-2241,-12331,7084,-2241,-12331,7084,-2241,  },
  { -12331,7084,-2241,-12331,7084,2200,-9172,2631,2200,-9172,2631,2200,  },
  { -6606,6250,2200,-6606,6250,-2241,-9172,2631,-2241,-9172,2631,-2241,  },
  { -9172,2631,-2241,-9172,2631,2200,-6606,6250,2200,-6606,6250,2200,  },
  { -4237,3838,2200,-4237,3838,-2241,-6606,6250,-2241,-6606,6250,-2241,  },
  { -6606,6250,-2241,-6606,6250,2200,-4237,3838,2200,-4237,3838,2200,  },
  { -1967,6908,2200,-1967,6908,-2241,-4237,3838,-2241,-4237,3838,-2241,  },
  { -4237,3838,-2241,-4237,3838,2200,-1967,6908,2200,-1967,6908,2200,  },
  { 204,5483,2200,204,5483,-2241,-1967,6908,-2241,-1967,6908,-2241,  },
  { -1967,6908,-2241,-1967,6908,2200,204,5483,2200,204,5483,2200,  },
  { -388,2302,2200,-388,2302,-2241,204,5483,-2241,204,5483,-2241,  },
  { 204,5483,-2241,204,5483,2200,-388,2302,2200,-388,2302,2200,  },
  { 895,1206,2200,895,1206,-2241,-388,2302,-2241,-388,2302,-2241,  },
  { -388,2302,-2241,-388,2302,2200,895,1206,2200,895,1206,2200,  },
  { 1586,5044,2200,1586,5044,-2241,895,1206,-2241,895,1206,-2241,  },
  { 895,1206,-2241,895,1206,2200,1586,5044,2200,1586,5044,2200,  },
  { 8988,0,2200,8988,-1,-2241,1586,5044,-2241,1586,5044,-2241,  },
  { 1586,5044,-2241,1586,5044,2200,8988,0,2200,8988,0,2200,  },
  { 12245,3728,2200,12245,3728,-2241,8988,-1,-2241,8988,-1,-2241,  },
  { 8988,-1,-2241,8988,0,2200,12245,3728,2200,12245,3728,2200,  },
  { 10370,6579,2200,10370,6579,-2241,12245,3728,-2241,12245,3728,-2241,  },
  { 12245,3728,-2241,12245,3728,2200,10370,6579,2200,10370,6579,2200,  },
  { 8001,3399,2200,8001,3399,-2241,10370,6579,-2241,10370,6579,-2241,  },
  { 10370,6579,-2241,10370,6579,2200,8001,3399,2200,8001,3399,2200,  },
  { 895,11405,2200,4152,9760,2200,3066,6360,2200,3066,6360,2200,  },
  { -388,8663,2200,895,11405,2200,3066,6360,2200,3066,6360,2200,  },
  { 1586,5044,2200,-388,8663,2200,3066,6360,2200,3066,6360,2200,  },
  { 8001,3399,2200,8988,0,2200,3066,6360,2200,3066,6360,2200,  },
  { 8988,0,2200,1586,5044,2200,3066,6360,2200,3066,6360,2200,  },
  { 3461,13269,2200,6422,10637,2200,4152,9760,2200,4152,9760,2200,  },
  { 6422,10637,2200,6817,7566,2200,4152,9760,2200,4152,9760,2200,  },
  { 895,11405,2200,1191,14037,2200,4152,9760,2200,4152,9760,2200,  },
  { 1191,14037,2200,3461,13269,2200,4152,9760,2200,4152,9760,2200,  },
  { 6422,10637,2200,8594,11953,2200,6817,7566,2200,6817,7566,2200,  },
  { 8001,14037,2200,9877,11076,2200,8594,11953,2200,8594,11953,2200,  },
  { 6422,10637,2200,8001,14037,2200,8594,11953,2200,8594,11953,2200,  },
  { 8001,14037,2200,10666,12501,2200,9877,11076,2200,9877,11076,2200,  },
  { 303,15901,2200,5337,15024,2200,3461,13269,2200,3461,13269,2200,  },
  { 1191,14037,2200,303,15901,2200,3461,13269,2200,3461,13269,2200,  },
  { 303,15901,2200,5731,18423,2200,5337,15024,2200,5337,15024,2200,  },
  { 303,15901,2200,3363,20507,2200,5731,18423,2200,5731,18423,2200,  },
  { 303,15901,2200,402,18752,2200,3363,20507,2200,3363,20507,2200,  },
  { 895,11405,2200,-881,14585,2200,1191,14037,2200,1191,14037,2200,  },
  { 895,11405,2200,-5224,16120,2200,-881,14585,2200,-881,14585,2200,  },
  { -5224,16120,2200,-5224,18423,2200,-881,14585,2200,-881,14585,2200,  },
  { -5224,16120,2200,-8481,16340,2200,-5224,18423,2200,-5224,18423,2200,  },
  { -5224,16120,2200,-7198,14256,2200,-8481,16340,2200,-8481,16340,2200,  },
  { 1586,5044,2200,204,5483,2200,-388,8663,2200,-388,8663,2200,  },
  { 204,5483,2200,-1967,6908,2200,-388,8663,2200,-388,8663,2200,  },
  { -1967,6908,2200,-2461,10747,2200,-388,8663,2200,-388,8663,2200,  },
  { -1967,6908,2200,-4435,7018,2200,-2461,10747,2200,-2461,10747,2200,  },
  { -4237,3838,2200,-6606,6250,2200,-4435,7018,2200,-4435,7018,2200,  },
  { -1967,6908,2200,-4237,3838,2200,-4435,7018,2200,-4435,7018,2200,  },
  { -6606,6250,2200,-6409,9869,2200,-4435,7018,2200,-4435,7018,2200,  },
  { -6606,6250,2200,-9271,6360,2200,-6409,9869,2200,-6409,9869,2200,  },
  { -9172,2631,2200,-12331,7084,2200,-9271,6360,2200,-9271,6360,2200,  },
  { -6606,6250,2200,-9172,2631,2200,-9271,6360,2200,-9271,6360,2200,  },
  { -12331,7084,2200,-10850,9387,2200,-9271,6360,2200,-9271,6360,2200,  },
  { 1586,5044,2200,-388,2302,2200,204,5483,2200,204,5483,2200,  },
  { 1586,5044,2200,895,1206,2200,-388,2302,2200,-388,2302,2200,  },
  { 8001,3399,2200,12245,3728,2200,8988,0,2200,8988,0,2200,  },
  { 8001,3399,2200,10370,6579,2200,12245,3728,2200,12245,3728,2200,  },
};

GenMinMesh * __stdcall MinMesh_Kopuli()
{
  GenMinMesh *mesh;
  sInt i,fc;
  GenMinVert *v;
  GenMinFace *f;

  fc = sizeof(KopuliData)/24;

  mesh = new GenMinMesh;

  mesh->Vertices.SetMax(fc*4);
  mesh->Vertices.Count = fc*4;
  v = mesh->Vertices.Array;
  sSetMem(v,0,sizeof(*v)*(fc*4));

  mesh->Faces.SetMax(fc);
  mesh->Faces.Count = fc;
  f = mesh->Faces.Array;
  sSetMem(f,0,sizeof(*f)*fc);

  for(i=0;i<fc;i++)
  {
    v->Pos.x = KopuliData[i][ 0]/16384.0f;
    v->Pos.y = KopuliData[i][ 1]/16384.0f;
    v->Pos.z = KopuliData[i][ 2]/16384.0f;
    v->Select = 1;
    v->Color = 0;//~0;
    v++;
    v->Pos.x = KopuliData[i][ 3]/16384.0f;
    v->Pos.y = KopuliData[i][ 4]/16384.0f;
    v->Pos.z = KopuliData[i][ 5]/16384.0f;
    v->Select = 1;
    v->Color = 0;//~0;
    v++;
    v->Pos.x = KopuliData[i][ 6]/16384.0f;
    v->Pos.y = KopuliData[i][ 7]/16384.0f;
    v->Pos.z = KopuliData[i][ 8]/16384.0f;
    v->Select = 1;
    v->Color = 0;//~0;
    v++;
    v->Pos.x = KopuliData[i][ 9]/16384.0f;
    v->Pos.y = KopuliData[i][10]/16384.0f;
    v->Pos.z = KopuliData[i][11]/16384.0f;
    v->Select = 1;
    v->Color = 0;//~0;
    v++;

    f->Select = 1;
    f->Cluster = 1;
    if(KopuliData[i][6]==KopuliData[i][9] && KopuliData[i][7]==KopuliData[i][10] && KopuliData[i][8]==KopuliData[i][11])
    {
      f->Count = 3;
      f->Vertices[0] = i*4+2;
      f->Vertices[1] = i*4+1;
      f->Vertices[2] = i*4+0;
    }
    else
    {
      f->Count = 4;
      f->Vertices[0] = i*4+3;
      f->Vertices[1] = i*4+2;
      f->Vertices[2] = i*4+1;
      f->Vertices[3] = i*4+0;
    }
    f++;
  }

  return mesh;
} 

/****************************************************************************/
/***                                                                      ***/
/***   Special Ops                                                        ***/
/***                                                                      ***/
/****************************************************************************/

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
      if(face->Cluster!=0)
        face->Cluster = id;
  }
  mesh->ChangeTopo();
  mtrl->Release();

  return mesh;
}

/****************************************************************************/

GenMinMesh * __stdcall MinMesh_MatLinkId(GenMinMesh *mesh,GenMaterial *mtrl,sInt id,sInt pass)
{
  if(!mtrl || mtrl->ClassId != KC_MATERIAL || CheckMinMesh(mesh))
    return 0;

  for(sInt i=0;i<mesh->Clusters.Count;i++)
  {
    if(mesh->Clusters[i].Id==id)
    {
      sRelease(mesh->Clusters[i].Mtrl);
      mesh->Clusters[i].Mtrl = mtrl; mtrl->AddRef();
      mesh->Clusters[i].RenderPass = pass;
    }
  }

  mesh->ChangeTopo();
  mtrl->Release();

  return mesh;
}

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
  mesh->MergeClusters();
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
  {
    sBool a = sFAbs(mesh->Vertices[i].Pos.x-center.x)<=size.x/2;
    sBool b = sFAbs(mesh->Vertices[i].Pos.y-center.y)<=size.y/2; 
    sBool c = sFAbs(mesh->Vertices[i].Pos.z-center.z)<=size.z/2;
    mesh->Vertices[i].TempByte = (a && b && c);
  }
  switch(mode&12)
  {
  case MMS_VERTEX:
    for(sInt i=0;i<mesh->Vertices.Count;i++)
      mesh->Vertices[i].Select = SelectLogic(mesh->Vertices[i].Select,mesh->Vertices[i].TempByte,mode);
    break;
  case MMS_FULLFACE:
    for(sInt i=0;i<mesh->Faces.Count;i++)
    {
      GenMinFace *face = &mesh->Faces[i];
      sBool ok = 1;
      for(sInt j=0;j<face->Count;j++)
        if(!mesh->Vertices[face->Vertices[j]].TempByte)
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
        if(mesh->Vertices[face->Vertices[j]].TempByte)
          ok = 1;
      face->Select = SelectLogic(face->Select,ok,mode);
    }
    break;
  }

  mesh->ChangeGeo();
  return mesh;
}


GenMinMesh * __stdcall MinMesh_SelectLogic(GenMinMesh *mesh,sInt mode)
{
  if(CheckMinMesh(mesh)) return 0;

  switch(mode)
  {
  case 0:
    for(sInt i=0;i<mesh->Faces.Count;i++)
      mesh->Faces[i].Select = !mesh->Faces[i].Select;
    break;

  case 1:
    for(sInt i=0;i<mesh->Vertices.Count;i++)
      mesh->Vertices[i].Select = !mesh->Vertices[i].Select;
    break;

  case 2:
    for(sInt i=0;i<mesh->Vertices.Count;i++)
      mesh->Vertices[i].Select = 0;
    for(sInt i=0;i<mesh->Faces.Count;i++)
    {
      GenMinFace *face = &mesh->Faces[i];
      if(face->Select)
        for(sInt j=0;j<face->Count;j++)
          mesh->Vertices[face->Vertices[j]].Select = 1;
    }
    break;
  }
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

GenMinMesh * __stdcall MinMesh_BakeAnimation(GenMinMesh *mesh,sF32 fade,sF32 metamorph)
{
  if(CheckMinMesh(mesh)) return 0;

  mesh->BakeAnimation(fade,metamorph);
  return mesh;
}

GenMinMesh * __stdcall MinMesh_BoneChain(GenMinMesh *mesh,sF323 p0f,sF323 p1f,sInt count,sInt flags)
{
  GenMinVert *vp;
  GenMinCluster *cl;
  sVector p0,p1,dir,diff;
  sAABox box;

  if(CheckMinMesh(mesh)) return 0;

  for(sInt i=1;i<mesh->Clusters.Count;i++)
  {
    cl = &mesh->Clusters[i];
    cl->AnimType = 2;
  }

  if(flags & 1)
  {
    mesh->CalcBBox(box);
    p0.Init(0,0,box.Min.z);
    p1.Init(0,0,box.Max.z);
  }
  else
  {
    p0.Init(p0f.x,p0f.y,p0f.z,1);
    p1.Init(p1f.x,p1f.y,p1f.z,1);
  }

  dir.Sub4(p1,p0);
  dir.Scale3(1.0f/dir.Dot3(dir));

  mesh->CreateAnimation(count);
  for(sInt i=0;i<count;i++)
  {
    mesh->Animation->Matrices[i].Temp.Init();
    mesh->Animation->Matrices[i].Temp.l.Lin4(p0,p1,(1.0f*i)/(count-1));
    mesh->Animation->Matrices[i].BasePose.Invert(mesh->Animation->Matrices[i].Temp);
    mesh->Animation->Matrices[i].NoAnimation = mesh->Animation->Matrices[i].Temp;
  }

  for(sInt i=0;i<mesh->Vertices.Count;i++)
  {
    vp = &mesh->Vertices[i];
    diff.Init(vp->Pos.x-p0.x,vp->Pos.x-p0.y,vp->Pos.z-p0.z,0);
    sF32 fade = dir.Dot3(diff)*(count-1);

    sInt fi = sRange(sInt(fade*1024),count*1024-1,0);
    sInt index = fi/1024;
    fade = (fi&1023)/1024.0f;
    if(index<0)
    {
      vp->BoneCount = 1;
      vp->Matrix[0] = 0;
      vp->Weights[0] = 1.0f;
    }
    else if(index+1<count)
    {
      vp->BoneCount = 4;
      vp->Matrix[0] = sMax(0,index-1);
      vp->Matrix[1] = index;
      vp->Matrix[2] = sMin(count-1,index+1);
      vp->Matrix[3] = sMin(count-1,index+2);

      sF32 f1 = fade;
      sF32 f2 = fade*fade;
      sF32 f3 = fade*fade*fade;
      vp->Weights[0] = -0.5f*f3 +1.0f*f2 -0.5f*f1    ;
      vp->Weights[1] =  1.5f*f3 -2.5f*f2           +1;
      vp->Weights[2] = -1.5f*f3 +2.0f*f2 +0.5f*f1    ;
      vp->Weights[3] =  0.5f*f3 -0.5f*f2             ;
    }
    else
    {
      vp->BoneCount = 1;
      vp->Matrix[0] = count-1;
      vp->Weights[0] = 1.0f;
    }
  }

  return mesh;
}

GenMinMesh * __stdcall MinMesh_BoneTrain(GenMinMesh *mesh,GenSpline *spline,sF32 delta,sInt mode)
{
  if(CheckMinMesh(mesh)) { sRelease(spline); return 0; }
  if(!spline) return mesh;
  if(mesh->Animation)
  {
    for(sInt i=0;i<mesh->Animation->Matrices.Count;i++)
    {
      GenMinMatrix *mat = &mesh->Animation->Matrices[i];

      sRelease(mat->Spline);
      mat->Spline = spline;
      mat->Spline->AddRef();
      if(mode&1)
        mat->Factor = i*delta/(mesh->Animation->Matrices.Count-1);
      else
        mat->Offset = i*delta/(mesh->Animation->Matrices.Count-1);
    }
  }
  sRelease(spline);
  return mesh;
}

GenMinMesh * __stdcall MinMesh_ScaleAnim(GenMinMesh *mesh,sF32 scale)
{
  if(CheckMinMesh(mesh)) return 0;

  if(mesh->Animation)
  {
    for(sInt i=0;i<mesh->Animation->Matrices.Count;i++)
    {
      GenMinMatrix *mm = &mesh->Animation->Matrices[i];
      mm->BasePose.l.Scale3(scale);      
      mm->NoAnimation.l.Scale3(scale);

      if(mm->Spline)
      {
        BlobSpline *bs = mm->Spline->GetBlobSpline();
        for(sInt i=0;i<bs->Count;i++)
        {
          bs->Keys[i].px *= scale;
          bs->Keys[i].py *= scale;
          bs->Keys[i].pz *= scale;
        }
      }
      mm->SRT[6] *= scale;
      mm->SRT[7] *= scale;
      mm->SRT[8] *= scale;
      if(mm->TPtr)
      {
        for(sInt i=0;i<mm->KeyCount*3;i++)
          mm->TPtr[i] *= scale;
      }
    } 
  }

  sMatrix m;
  m.Init();
  m.i.x = scale;
  m.j.y = scale;
  m.k.z = scale;
  mesh->Transform(MMU_ALL,m);

  return mesh;
}

/****************************************************************************/
/***                                                                      ***/
/***   Marching Tetrahedra                                                ***/
/***                                                                      ***/
/****************************************************************************/
 
#if !sINTRO
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
#endif

/****************************************************************************/
/***                                                                      ***/
/***   Topology Modifiers                                                 ***/
/***                                                                      ***/
/****************************************************************************/

GenMinMesh * __stdcall MinMesh_Triangulate(GenMinMesh *mesh)
{
  if(CheckMinMesh(mesh)) return 0;

  // calculate new number of faces
  sInt outFaceCount = 0;
  for(sInt i=0;i<mesh->Faces.Count;i++)
  {
    if(mesh->Faces[i].Count <= 3)
      outFaceCount++;
    else
      outFaceCount += mesh->Faces[i].Count - 2;
  }

  // make new face list
  GenMinFace *outFaces = new GenMinFace[outFaceCount];
  GenMinFace *outFace = outFaces;

  for(sInt i=0;i<mesh->Faces.Count;i++)
  {
    GenMinFace *curFace = &mesh->Faces[i];

    if(mesh->Faces[i].Count <= 3)
      *outFace++ = *curFace;
    else
    {
      for(sInt j=2;j<curFace->Count;j++)
      {
        *outFace = *curFace;
        outFace->Count = 3;
        outFace->Vertices[1] = curFace->Vertices[j-1];
        outFace->Vertices[2] = curFace->Vertices[j];
        outFace++;
      }
    }
  }

  // exchange face lists
  delete[] mesh->Faces.Array;
  mesh->Faces.Array = outFaces;
  mesh->Faces.Alloc = mesh->Faces.Count = outFaceCount;

  mesh->ChangeTopo();

  return mesh;
}

static void __stdcall ExtrudeOnce(GenMinMesh *mesh,sInt *groups)
{
  // initialize
  sInt faceCount = mesh->Faces.Count;
  sInt *vertRemap = new sInt[mesh->Vertices.Count];
  for(sInt i=0;i<mesh->Vertices.Count;i++)
    vertRemap[i] = i;

  for(sInt faceInd=0;faceInd<faceCount;faceInd++)
  {
    GenMinFace *curFace = &mesh->Faces[faceInd];
    if(!curFace->Select || curFace->Count <= 2)
      continue;

    // go through halfedges for this face
    sInt count = curFace->Count;
    sInt group = groups[faceInd];

    for(sInt i=0;i<count;i++)
    {
      sInt adjacent = curFace->Adjacent[i] >> 3;
      if(adjacent != -1 && groups[adjacent] == group) // not a boundary
        continue;

      // this edge is a boundary, so extrude a quad
      sInt oldVert[2],newVert[2];

      for(sInt j=0;j<2;j++)
      {
        sInt old = curFace->Vertices[(i + j) % count];

        if(vertRemap[old] == old) // not repositioned yet
        {
          vertRemap[old] = mesh->Vertices.Count;
          GenMinVert *nv = mesh->Vertices.Add();
          *nv = mesh->Vertices[old];
        }

        oldVert[j] = old;
        newVert[j] = vertRemap[old];
      }

      // add the new face
      GenMinFace *newFace = mesh->Faces.Add();
      curFace = &mesh->Faces[faceInd];
      newFace->Select = 0;
      newFace->Count = 4;
      newFace->Cluster = curFace->Cluster;
      newFace->Vertices[0] = oldVert[0];
      newFace->Vertices[1] = oldVert[1];
      newFace->Vertices[2] = newVert[1];
      newFace->Vertices[3] = newVert[0];
      sSetMem(newFace->Adjacent,0xff,sizeof(newFace->Adjacent));
    }
  }

  for(sInt faceInd=0;faceInd<faceCount;faceInd++)
  {
    GenMinFace *curFace = &mesh->Faces[faceInd];
    if(!curFace->Select || curFace->Count <= 2)
      continue;

    for(sInt j=0;j<curFace->Count;j++)
      curFace->Vertices[j] = vertRemap[curFace->Vertices[j]];
  }

  delete[] vertRemap;
}

GenMinMesh * __stdcall MinMesh_Extrude(GenMinMesh *mesh,sInt mode,sInt count,sF323 distance)
{
  if(CheckMinMesh(mesh)) return 0;
  mesh->CalcAdjacency();
  mesh->CalcNormals();

  // prepare
  sF32 angThresh = 0.4995f; // cos(threshold angle)
  sInt origFaceCount = mesh->Faces.Count;
  sInt *faceGroup = new sInt[origFaceCount];
  sInt *groupNext = new sInt[origFaceCount];
  sInt displaceMode = mode & 3;

  // build groups (depth search)
  sSetMem(faceGroup,0xff,origFaceCount * sizeof(sInt));
  sInt groupCount = 0;

  for(sInt i=0;i<origFaceCount;i++)
  {
    GenMinFace *curFace = &mesh->Faces[i];
    if(!curFace->Select || curFace->Count <= 2 || faceGroup[i] != -1)
      continue;

    sInt *faceStackPtr = groupNext;
    *faceStackPtr++ = i;

    while(faceStackPtr != groupNext)
    {
      sInt top = *--faceStackPtr;
      faceGroup[top] = groupCount;

      GenMinFace *face = &mesh->Faces[top];
      for(sInt j=0;j<face->Count;j++)
      {
        sInt adjacent = face->Adjacent[j] >> 3;
        GenMinFace *aface = &mesh->Faces[adjacent];

        if(adjacent != -1 && faceGroup[adjacent] == -1 && aface->Select
          && face->Normal.Dot(aface->Normal) >= angThresh)
        {
          faceGroup[adjacent] = -2;
          *faceStackPtr++ = adjacent;
        }
      }
    }

    groupCount++;
  }

  // build linked lists for groups
  sInt *groupFirst = new sInt[groupCount];
  sSetMem(groupFirst,0xff,sizeof(sInt) * groupCount);
  for(sInt i=origFaceCount-1;i>=0;i--)
  {
    sInt grp = faceGroup[i];
    if(grp >= 0)
    {
      groupNext[i] = groupFirst[grp];
      groupFirst[grp] = i;
    }
  }

  // iterate over extrusion steps
  while(count--)
  {
    // perform extrusion
    ExtrudeOnce(mesh,faceGroup);

    for(sInt i=0;i<mesh->Vertices.Count;i++)
      mesh->Vertices[i].Select = 0;

    for(sInt grp=0;grp<groupCount;grp++)
    {
      // skip empty groups
      sInt first = groupFirst[grp];

      // vertex displacement: first compute group normal (if necessary)
      GenMinVector avgDir;
      if(displaceMode == 0) // average (face!) normal
      {
        avgDir.Init(0,0,0);

        for(sInt face=first;face != -1;face = groupNext[face])
          avgDir.Add(mesh->Faces[face].Normal);

        avgDir.UnitSafe();
      }
      else
        avgDir.Init(1,1,1);

      // reposition vertices
      for(sInt face=first;face != -1;face = groupNext[face])
      {
        GenMinFace *curFace = &mesh->Faces[face];
        sVERIFY(curFace->Select);

        for(sInt j=0;j<curFace->Count;j++)
        {
          GenMinVert *vert = &mesh->Vertices[curFace->Vertices[j]];
          if(vert->Select)
            continue;

          const GenMinVector *disp = &avgDir;
          if(displaceMode == 1) // individual normal
            disp = &vert->Normal;

          vert->Pos.x += disp->x * distance.x;
          vert->Pos.y += disp->y * distance.y;
          vert->Pos.z += disp->z * distance.z;
          vert->Select = 1;
        }
      }
    }
  }

  // swap stitches to end
  sInt pos = mesh->Faces.Count;
  for(sInt i=0;i<pos;i++)
    if(mesh->Faces[i].Count == 2)
      sSwap(mesh->Faces[i],mesh->Faces[--pos]);

  // cleanup
  delete[] faceGroup;
  delete[] groupFirst;
  delete[] groupNext;
  mesh->ChangeTopo();

  return mesh;
}

/****************************************************************************/

static sInt GetValence(GenMinMesh *mesh,sInt edgeTag)
{
  sInt e = edgeTag;
  sInt count = 0;

  do
  {
    count++;
    e = mesh->NextVertEdge(e);
  }
  while(e != edgeTag);

  return count;
}

struct MeshElement
{
  sInt Degree;
  sInt Edge;
  MeshElement **Adjacent;

  sInt Find(MeshElement *ref,sInt adjust) const;
};

sInt MeshElement::Find(MeshElement *ref,sInt adjust) const
{
  for(sInt i=0;i<Degree;i++)
    if(Adjacent[i] == ref)
      return (i + adjust + Degree) % Degree;

  sVERIFYFALSE;
  return -1;
}

class MeshCoder
{
  GenMinMesh *Mesh;

  MeshElement *Elems[2];  // 0=faces 1=vertices
  sInt ElemCount[2];      // 0=faces 1=vertices
  MeshElement **Links,**LinkPtr,**LinkEnd;
  
  MeshElement **FaceMap;
  MeshElement **VertMap;
  sU8 *Data;
  sInt FaceCount;
  sInt VertCount;
  sInt *VertMerge;
  sInt *VertOrder;

  sArray<sInt> Active;

  // encode and decode
  MeshElement *AddElement(sInt type,sInt deg);
  void AddFaceToVertex(MeshElement *f,sInt i,MeshElement *v,sInt j);
  sInt ForceFV(MeshElement *f,sInt j,sInt dir);

  // encode only
  void ActivateVE(MeshElement *f,sInt i);

  // decode only
  MeshElement *DecodeFace();
  void ActivateVD(MeshElement *f,sInt i);

  // debug only
  void AssertSingleFV(MeshElement *f,MeshElement *v);
  void AssertFV(MeshElement *f);
  void AssertLinkedEdge(MeshElement *v);

public:
  // regularizing a mesh
  GenMinMesh *CloseBoundaries(GenMinMesh *mesh);

  // main coding api
  sInt *Encode(GenMinMesh *mesh,sU8 *&p,sInt &outVertCount);
  GenMinMesh *Decode(sU8 *&p);
};

static sInt FindEdgeByVert(GenMinTempEdge *list,sInt n,sInt startVert)
{
  // binary search
  sInt l,r,x;

  l = 0;
  r = n;
  
  while(l < r)
  {
    x = (l + r) / 2;

    if(list[x].v0 == startVert)
      return x;
    else if(list[x].v0 < startVert) // continue in right half
      l = x + 1;
    else // continue in left half
      r = x;
  }

  return -1;
}

GenMinMesh *MeshCoder::CloseBoundaries(GenMinMesh *inMesh)
{
  // first, just make a copy of the mesh
  GenMinMesh *mesh = new GenMinMesh;
  mesh->Copy(inMesh);

  // one-to-one remap
  sInt *remap = new sInt[mesh->Vertices.Count];
  for(sInt i=0;i<mesh->Vertices.Count;i++)
    remap[i] = i;

  // build adjacency, step 1
  mesh->CalcAdjacencyCore(remap);
  //mesh->CalcAdjacency();
  delete[] remap;

  // make list of boundary edges
  sArray<GenMinTempEdge> boundaryEdges;
  boundaryEdges.Init();

  for(sInt i=0;i<mesh->Faces.Count;i++)
  {
    GenMinFace *curFace = &mesh->Faces[i];

    for(sInt j=0;j<curFace->Count;j++)
    {
      if(curFace->Adjacent[j] == -1) // boundary
      {
        GenMinTempEdge *te = boundaryEdges.Add();
        te->v0 = curFace->Vertices[(j+1) % curFace->Count];
        te->v1 = curFace->Vertices[j];
      }
    }
  }

  // sort boundary edges by first vertex
  if(boundaryEdges.Count)
    HeapSortEdges(&boundaryEdges[0],boundaryEdges.Count);

  // fill boundaries
  sInt boundaryFaces = 0;

  while(boundaryEdges.Count)
  {
    // insert a boundary helper vertex
    sInt boundaryVert = mesh->Vertices.Count;
    GenMinVert *tempVert = mesh->Vertices.Add();
    tempVert->Pos.Init(0,0,0);

    // pick the first boundary edge available and follow the loop
    sInt vertex = boundaryEdges[0].v1;
    sInt edge;

    do
    {
      // find next edge in this loop
      edge = FindEdgeByVert(&boundaryEdges[0],boundaryEdges.Count,vertex);
      sVERIFY(edge != -1); // we should ALWAYS be able to find one.
      sInt nextVertex = boundaryEdges[edge].v1;

      // deleted this edge from the list of candidates
      boundaryEdges.Count--;
      for(sInt i=edge;i<boundaryEdges.Count;i++)
        boundaryEdges[i] = boundaryEdges[i+1];

      // insert a "deleted" face to close the mesh
      GenMinFace *face = mesh->Faces.Add();
      face->Count = 3;
      face->Vertices[0] = boundaryVert;
      face->Vertices[1] = vertex;
      face->Vertices[2] = nextVertex;
      boundaryFaces++;

      // continue following
      vertex = nextVertex;
    }
    while(edge != 0);
  }

  boundaryEdges.Exit();

  sDPrintF("%d faces inserted to close boundary.\n",boundaryFaces);

  // everything should be closed now. re-build adjacency and check.
  remap = new sInt[mesh->Vertices.Count];
  for(sInt i=0;i<mesh->Vertices.Count;i++)
    remap[i] = i;

  sBool closed = mesh->CalcAdjacencyCore(remap);
  //sBool closed = mesh->CalcAdjacency();
  delete[] remap;

  // assert everything was ok and return the new mesh
  sVERIFY(closed);

  return mesh;
}

MeshElement *MeshCoder::AddElement(sInt type,sInt deg)
{
  MeshElement *elem = &Elems[type][ElemCount[type]++];
  elem->Degree = deg;
  elem->Adjacent = LinkPtr;
  memset(LinkPtr,0,deg * sizeof(MeshElement *));

  LinkPtr += deg;
  sVERIFY(LinkPtr <= LinkEnd);

  sVERIFY(ElemCount[0] <= FaceCount);
  sVERIFY(ElemCount[1] <= VertCount);

  return elem;
}

void MeshCoder::AddFaceToVertex(MeshElement *f,sInt i,MeshElement *v,sInt j)
{
  v->Adjacent[j] = f;

  for(sInt dir=-1;dir<=1;dir+=2)
  {
    MeshElement *fp = v->Adjacent[(j+dir+v->Degree)%v->Degree];
    sInt in = (i-dir+f->Degree)%f->Degree;
		if(fp && !f->Adjacent[in])
			f->Adjacent[in] = fp->Adjacent[fp->Find(v,dir)];
  }

  AssertLinkedEdge(v);
}

sInt MeshCoder::ForceFV(MeshElement *f,sInt j,sInt dir)
{
  MeshElement *v = f->Adjacent[0];
  sInt i = 0;
  
  while(1)
  {
    i = (i + dir + f->Degree) % f->Degree;
    MeshElement *vt = v->Adjacent[(j - dir + v->Degree) % v->Degree];
    v = f->Adjacent[i];

    if(!i || !vt || !v)
      break;

    j = v->Find(vt,-dir);
    AddFaceToVertex(f,i,v,j);
  }

  return i;
}

void MeshCoder::ActivateVE(MeshElement *f,sInt i)
{
  GenMinFace *face = &Mesh->Faces[f->Edge >> 3];
  sInt vertSlot = ((f->Edge & 7) + i) % face->Count;
  sInt edgeTag = (f->Edge & ~7) + vertSlot;
  sInt vertex = face->Vertices[vertSlot];
  MeshElement *v;

  // (try to) find vertex in active vertex list
  sInt j;
  for(j=0;j<Active.Count;j++)
    if(Active[j] == vertex)
      break;

  if(j == Active.Count) // new vertex
  {
    sVERIFY(VertMap[vertex] == 0);

    sInt val = GetValence(Mesh,edgeTag);
    v = AddElement(1,val);

    *Data++ = val;
    VertOrder[ElemCount[1]-1] = vertex;

    f->Adjacent[i] = v;
    v->Adjacent[0] = f;

    v->Edge = edgeTag;
    *Active.Add() = vertex;
    VertMap[vertex] = v;
  }
  else // split
  {
    v = VertMap[vertex];
    sVERIFY(j < 65536);

    *Data++ = 0;
    *(sU16 *) Data = j; Data += 2;

    j=0;
    sInt e,ee;
    
    e = ee = v->Edge;
    while((e & ~7) != (f->Edge & ~7))
    {
      j++;
      e = Mesh->NextVertEdge(e);
    }

    *Data++ = j;
    f->Adjacent[i] = v;
    AddFaceToVertex(f,i,v,j);
  }

  AssertSingleFV(f,v);
}

MeshElement *MeshCoder::DecodeFace()
{
  sInt deg = *Data++;
  MeshElement *f = AddElement(0,deg);

  return f;
}

void MeshCoder::ActivateVD(MeshElement *f,sInt i)
{
  MeshElement *v;
  sInt val = *Data++;
  sInt j;

  if(val) // new vertex
  {
    v = AddElement(1,val);
    *Active.Add() = v - Elems[1];

    j = 0;

    f->Adjacent[i] = v;
    v->Adjacent[0] = f;
  }
  else // split
  {
    v = &Elems[1][Active[*(sU16 *) Data]]; Data += 2;
    j = *Data++;

    f->Adjacent[i] = v;
    AddFaceToVertex(f,i,v,j);
  }

  AssertSingleFV(f,v);
}

void MeshCoder::AssertSingleFV(MeshElement *f,MeshElement *v)
{
  sInt i,j;

  for(i=0;i<f->Degree;i++)
  {
    if(v == f->Adjacent[i])
    {
      for(j=0;j<v->Degree;j++)
        if(v->Adjacent[j] == f)
          break;

      sVERIFY(j != v->Degree);
      break;
    }
  }

  sVERIFY(i != f->Degree);
}

void MeshCoder::AssertFV(MeshElement *f)
{
  sInt i,j;

  // assert FV consistency
  for(i=0;i<f->Degree;i++)
  {
    MeshElement *v = f->Adjacent[i];
    if(!v)
      continue;

    for(j=0;j<v->Degree;j++)
      if(v->Adjacent[j] == f)
        break;

    sVERIFY(j != v->Degree);
  }

  // then assert that we actually represent the right topology
  sInt e = f->Edge;

  for(i=0;i<f->Degree;i++)
  {
    MeshElement *realVert = VertMap[Mesh->Faces[e >> 3].Vertices[e & 7]];
    sVERIFY(realVert == f->Adjacent[i]);

    e = Mesh->NextFaceEdge(e);
  }
}

void MeshCoder::AssertLinkedEdge(MeshElement *v)
{
  sInt j,lj;

  // assert linked edge consistency
  lj = v->Degree - 1;
  for(j=0;j<v->Degree;j++)
  {
    MeshElement *f1 = v->Adjacent[j];
    MeshElement *f2 = v->Adjacent[lj];

    if(f1 != 0 && f2 != 0)
    {
      sBool consistent = sFALSE;

      for(sInt i1=0;i1<f1->Degree;i1++)
        for(sInt i2=0;i2<f2->Degree;i2++)
          if(f1->Adjacent[i1] == f2->Adjacent[i2])
            consistent = sTRUE;

      sVERIFY(consistent);
    }

    lj = j;
  }

  // then assert that we actually represent the right topology
  sInt e = v->Edge;

  for(sInt i=0;i<v->Degree;i++)
  {
    MeshElement *realFace = FaceMap[e >> 3];
    sVERIFY(!v->Adjacent[i] || realFace == v->Adjacent[i]);

    e = Mesh->NextVertEdge(e);
  }
}

sInt *MeshCoder::Encode(GenMinMesh *mesh,sU8 *&p,sInt &outVertCount)
{
  // currently assumes a closed mesh
  // additional simplifications apply:
  // - no deleted faces
  Mesh = mesh;

  // count real number of faces and vertices
  FaceCount = 0;
  for(sInt i=0;i<mesh->Faces.Count;i++)
    FaceCount += mesh->Faces[i].Count >= 3;

  VertCount = 0;
  VertMerge = mesh->CalcMergeVerts();
  for(sInt i=0;i<mesh->Vertices.Count;i++)
    VertCount += VertMerge[i] == i;

  // remember everything i've said about counting the real number of
  // vertices and stuff? just ignore it.
  VertCount = mesh->Vertices.Count;

  // create data structures
  FaceMap = new MeshElement *[mesh->Faces.Count];
  VertMap = new MeshElement *[mesh->Vertices.Count];
  memset(FaceMap,0,mesh->Faces.Count * sizeof(sInt));
  memset(VertMap,0,mesh->Vertices.Count * sizeof(sInt));

  Elems[0] = new MeshElement[FaceCount];
  Elems[1] = new MeshElement[VertCount];
  ElemCount[0] = 0;
  ElemCount[1] = 0;

  VertOrder = new sInt[VertCount];

  // count number of edges, reserve space for links
  sInt edgeCount = 0;
  for(sInt i=0;i<mesh->Faces.Count;i++)
    if(mesh->Faces[i].Count >= 3)
      edgeCount += mesh->Faces[i].Count;

  Links = new MeshElement *[edgeCount * 2];
  LinkPtr = Links;
  LinkEnd = Links + (edgeCount * 2);
  Active.Init();

  Data = p;

  *(sU32 *) Data = VertCount; Data += 4;
  *(sU32 *) Data = FaceCount; Data += 4;
  *(sU32 *) Data = edgeCount * 2; Data += 4;

  // encode connected components in turn
  for(sInt seedFace=0;seedFace<mesh->Faces.Count;seedFace++)
  {
    // face already encoded? skip.
    if(FaceMap[seedFace] || mesh->Faces[seedFace].Count == 2)
      continue;

    // encode seed face
    sInt k = mesh->Faces[seedFace].Count;
    *Data++ = k;

    MeshElement *f = AddElement(0,k);
    f->Edge = seedFace << 3;
    FaceMap[seedFace] = f;

    for(sInt i=0;i<k;i++)
      ActivateVE(f,i);

    while(Active.Count)
    {
      // pick vertex to complete
      sInt bestDegree = 256;
      sInt bestI = -1;
      MeshElement *v = 0;
      
      for(sInt i=0;i<Active.Count;i++)
      {
        MeshElement *testV = VertMap[Active[i]];
        sVERIFY(testV != 0);

        sInt k = 0;
        for(sInt j=0;j<testV->Degree;j++)
          k += !testV->Adjacent[j];

        if(k < bestDegree)
        {
          bestI = i;
          v = testV;
          bestDegree = k;
        }
      }

      // try to complete this vertex
      sInt edge = v->Edge;

      for(sInt j=0;j<v->Degree;j++)
      {
        if(!v->Adjacent[j])
        {
          // activate this face
          sInt face = edge >> 3;
          sVERIFY(FaceMap[face] == 0);
          sInt d = Mesh->Faces[face].Count;
          *Data++ = d;

          MeshElement *f = AddElement(0,d);
          FaceMap[face] = f;
          f->Edge = edge;

          f->Adjacent[0] = v;
          AddFaceToVertex(f,0,v,j);

          // complete this face
          sInt i = ForceFV(f,j,1);
          sInt iend = ForceFV(f,j,-1);

          if(i)
            while(i <= iend)
              ActivateVE(f,i++);

          // verify that this face is fully and correctly connected now
          for(i=0;i<d;i++)
            sVERIFY(f->Adjacent[i] != 0);

          AssertFV(f);
        }
        
        edge = mesh->NextVertEdge(edge);
      }

      sVERIFY(edge == v->Edge);

      // remove this vertex from the queue
      Active[bestI] = Active[--Active.Count];
    }
  }

  sVERIFY(ElemCount[0] == FaceCount);
  sVERIFY(ElemCount[1] == VertCount);
  
  outVertCount = VertCount;
  p = Data;

  Active.Exit();
  delete[] Links;
  delete[] Elems[0];
  delete[] Elems[1];
  delete[] FaceMap;
  delete[] VertMap;
  delete[] VertMerge;

  return VertOrder;
}

GenMinMesh *MeshCoder::Decode(sU8 *&p)
{
  Mesh = new GenMinMesh;
  Data = p;

  sU32 vertCount,faceCount,linkCount;
  vertCount = *(sU32 *) Data; Data += 4;
  faceCount = *(sU32 *) Data; Data += 4;
  linkCount = *(sU32 *) Data; Data += 4;

  Elems[0] = new MeshElement[faceCount];
  Elems[1] = new MeshElement[vertCount];
  ElemCount[0] = 0;
  ElemCount[1] = 0;

  Links = new MeshElement *[linkCount];
  LinkPtr = Links;
  LinkEnd = Links + linkCount;
  Active.Init();

  // while not all faces are coded
  while(ElemCount[0] < sInt(faceCount))
  {
    // get a seed face and activate its vertices
    MeshElement *f = DecodeFace();

    for(sInt i=0;i<f->Degree;i++)
      ActivateVD(f,i);

    // complete this connected component
    do
    {
      // pick vertex to complete
      sInt bestDegree = 256;
      sInt bestI;

      for(sInt i=0;i<Active.Count;i++)
      {
        MeshElement *v = &Elems[1][Active[i]];
        sInt deg = 0;

        for(sInt k=0;k<v->Degree;k++)
          deg += !v->Adjacent[k];

        if(deg < bestDegree)
        {
          bestDegree = deg;
          bestI = i;
        }
      }

      // complete this vertex
      MeshElement *v = &Elems[1][Active[bestI]];

      for(sInt j=0;j<v->Degree;j++)
      {
        if(!v->Adjacent[j])
        {
          f = DecodeFace();
          f->Adjacent[0] = v;
          AddFaceToVertex(f,0,v,j);

          sInt i = ForceFV(f,j,1);
          sInt iend = ForceFV(f,j,-1);

          if(i)
            while(i <= iend)
              ActivateVD(f,i++);
        }
      }

      Active[bestI] = Active[--Active.Count];
    }
    while(Active.Count);
  }

  // now convert to mesh
  Mesh->Vertices.Resize(vertCount);
  Mesh->Faces.Resize(faceCount);

  for(sInt i=0;i<ElemCount[1];i++)
    Elems[1][i].Edge = i;

  for(sInt i=0;i<ElemCount[0];i++)
  {
    MeshElement *f = &Elems[0][i];
    GenMinFace *face = &Mesh->Faces[i];

    face->Count = f->Degree;
    for(sInt j=0;j<f->Degree;j++)
      face->Vertices[j] = f->Adjacent[j]->Edge;

    face->Cluster = 1;
  }

  Mesh->ChangeTopo();

  // cleanup.
  Active.Exit();
  delete[] Links;
  delete[] Elems[0];
  delete[] Elems[1];

  p = Data;
  return Mesh;
}

GenMinMesh * __stdcall MinMesh_Compress(GenMinMesh *inMesh)
{
  if(CheckMinMesh(inMesh))
    return 0;

  MeshCoder coder;
  static sU8 buffer[256*1024];
  sU8 *bufPtr = buffer;

  // ---- regularize
  sDPrintF("-- regularize\n");
  GenMinMesh *mesh = coder.CloseBoundaries(inMesh);
  inMesh->Release();

  // ---- encode
  sDPrintF("-- encode\n");

  // encode topology
  sInt vertCount;
  sInt *vertOrder = coder.Encode(mesh,bufPtr,vertCount);
  sSystem->SaveFile("topo.dat",buffer,bufPtr - buffer);
  sDPrintF("topology coded size: %d bytes\n",bufPtr - buffer);

  // encode vertex positions
  for(sInt i=0;i<vertCount;i++)
  {
    GenMinVert *vert = &mesh->Vertices[vertOrder[i]];

    *((sF32 *) bufPtr) = vert->Pos.x; bufPtr += 4;
    *((sF32 *) bufPtr) = vert->Pos.y; bufPtr += 4;
    *((sF32 *) bufPtr) = vert->Pos.z; bufPtr += 4;
  }

  delete[] vertOrder;

  // coding stats
  mesh->Release();
  sDPrintF("coded size: %d bytes\n",bufPtr - buffer);

  // ---- decode

  sDPrintF("-- decode\n");

  // decode topology
  bufPtr = buffer;
  mesh = coder.Decode(bufPtr);

  // decode vertex positions
  for(sInt i=0;i<mesh->Vertices.Count;i++)
  {
    GenMinVert *vert = &mesh->Vertices[i];

    vert->Pos.x = *((sF32 *) bufPtr); bufPtr += 4;
    vert->Pos.y = *((sF32 *) bufPtr); bufPtr += 4;
    vert->Pos.z = *((sF32 *) bufPtr); bufPtr += 4;
  }

  // stats
  sDPrintF("decoded, read %d bytes\n",bufPtr - buffer);

  return mesh;
}

/****************************************************************************/
/****************************************************************************/

GenMinMesh * __stdcall MinMesh_Pipe(GenSpline *spline_,GenMinMesh *mesh0,GenMinMesh *mesh1,GenMinMesh *mesh2,sInt flags,sF32 texzoom,sF32 ringdist)
{
  if(spline_->GetBlobSpline()==0) return 0;
  if(spline_->GetBlobSpline()->Mode!=4) return 0;
  if(spline_->GetBlobPipe()==0) return 0;
  if(CheckMinMesh(mesh0)) return 0;

  GenMinMesh *mesh;
  BlobSpline *spline;
  BlobPipe *pipe;
  sMatrix m0,m1,mat;
  sAABox box;
  sF32 zmin[2],zmax[2];
  sInt vmin,vmax;
  sVector d;

  // prepare

  mesh = new GenMinMesh;
  mesh0->CalcBBox(box);
  zmin[0] = box.Min.z;
  zmax[0] = box.Max.z;
  if(mesh1)
    mesh1->CalcBBox(box);
  zmin[1] = box.Min.z;
  zmax[1] = box.Max.z;
  spline = spline_->GetBlobSpline();
  pipe = spline_->GetBlobPipe();
  m1.Init();

  sVERIFY(pipe->Count*2==spline->Count);
  for(sInt seg=0;seg<pipe->Count;seg++)
  {
    // add curved

    if(seg>0)
    {
      sVector center;
      sVector axis;
      sVector d0,d1;
      BlobPipeKey *kp = &pipe->Keys[seg-1];
      BlobSplineKey *k0 = &spline->Keys[seg*2-1];
      BlobSplineKey *k1 = &spline->Keys[seg*2+0];

      d0.x = (k0->px - kp->PosX);
      d0.y = (k0->py - kp->PosY);
      d0.z = (k0->pz - kp->PosZ);
      d1.x = (k1->px - kp->PosX);
      d1.y = (k1->py - kp->PosY);
      d1.z = (k1->pz - kp->PosZ);
      center.x = kp->PosX;
      center.y = kp->PosY;
      center.z = kp->PosZ;
      d0.Unit3();
      d1.Unit3();
      axis.Cross3(d0,d1);
      sF32 gamma = sFACos(d0.Dot3(d1));
      d.Add3(d0,d1);
      d.Unit3();
      center.AddScale3(d,kp->Radius/sFCos(gamma/2));

      vmin = mesh->Vertices.Count;
      mesh->Add(mesh1 ? mesh1 : mesh0);
      vmax = mesh->Vertices.Count;
      d.Sub3(m1.l,m0.l);

      for(sInt i=vmin;i<vmax;i++)
      {
        sVector v;
        v.x = mesh->Vertices[i].Pos.x;
        v.y = mesh->Vertices[i].Pos.y;
        v.z = mesh->Vertices[i].Pos.z;
        sF32 f = (v.z-zmin[1])/(zmax[1]-zmin[1]);

        mat.InitRot(axis,-f*(sPI-gamma));
        v.z = 0;
        v.Rotate34(m1);
        v.Sub3(center);
        v.Rotate34(mat);
        v.Add3(center);
        mesh->Vertices[i].Pos.x = v.x;
        mesh->Vertices[i].Pos.y = v.y;
        mesh->Vertices[i].Pos.z = v.z;
      }
    }

    // scan points

    {
      BlobSplineKey *k0 = &spline->Keys[seg*2+0];
      BlobSplineKey *k1 = &spline->Keys[seg*2+1];
      sQuaternion quat;

      quat.Init(k0->Zoom,k0->rx,k0->ry,k0->rz);
      quat.ToMatrix(m0);
      m0.l.Init4(k0->px,k0->py,k0->pz,1);

      quat.Init(k1->Zoom,k1->rx,k1->ry,k1->rz);
      quat.ToMatrix(m1);
      m1.l.Init4(k1->px,k1->py,k1->pz,1);

      // add straight

      vmin = mesh->Vertices.Count;
      mesh->Add(mesh0);
      vmax = mesh->Vertices.Count;
      d.Sub3(m1.l,m0.l);
      for(sInt i=vmin;i<vmax;i++)
      {
        sVector v;
        v.x = mesh->Vertices[i].Pos.x;
        v.y = mesh->Vertices[i].Pos.y;
        v.z = mesh->Vertices[i].Pos.z;
        sF32 tex = v.z-zmin[0];
        sF32 f = (v.z-zmin[0])/(zmax[0]-zmin[0]);
        v.z = 0;
        v.Rotate34(m0);
        v.AddScale3(d,f);
        mesh->Vertices[i].Pos.x = v.x;
        mesh->Vertices[i].Pos.y = v.y;
        mesh->Vertices[i].Pos.z = v.z;
        if(flags & 1)
          mesh->Vertices[i].UV[0][1] = tex*texzoom;
      }

      if(mesh2)
      {
        vmin = mesh->Vertices.Count;
        mesh->Add(mesh2);
        vmax = mesh->Vertices.Count;

        for(sInt i=vmin;i<vmax;i++)
        {
          sVector v;
          v.x = mesh->Vertices[i].Pos.x;
          v.y = mesh->Vertices[i].Pos.y;
          v.z = mesh->Vertices[i].Pos.z;
          v.Rotate34(m0);
          mesh->Vertices[i].Pos.x = v.x;
          mesh->Vertices[i].Pos.y = v.y;
          mesh->Vertices[i].Pos.z = v.z;
        }

        vmin = mesh->Vertices.Count;
        mesh->Add(mesh2);
        vmax = mesh->Vertices.Count;

        for(sInt i=vmin;i<vmax;i++)
        {
          sVector v;
          v.x = mesh->Vertices[i].Pos.x;
          v.y = mesh->Vertices[i].Pos.y;
          v.z = mesh->Vertices[i].Pos.z;
          v.Rotate34(m1);
          mesh->Vertices[i].Pos.x = v.x;
          mesh->Vertices[i].Pos.y = v.y;
          mesh->Vertices[i].Pos.z = v.z;
        }

        if((flags&2) && ringdist>0.1f)
        {
          sF32 dist = d.Abs3();
          sInt middle = sInt((dist/ringdist)-0.5f);
          for(sInt i=0;i<middle;i++)
          {
            vmin = mesh->Vertices.Count;
            mesh->Add(mesh2);
            vmax = mesh->Vertices.Count;

            mat = m0;
            mat.l.AddScale3(d,(i+1.0f)/(middle+1));

            for(sInt i=vmin;i<vmax;i++)
            {
              sVector v;
              v.x = mesh->Vertices[i].Pos.x;
              v.y = mesh->Vertices[i].Pos.y;
              v.z = mesh->Vertices[i].Pos.z;
              v.Rotate34(mat);
              mesh->Vertices[i].Pos.x = v.x;
              mesh->Vertices[i].Pos.y = v.y;
              mesh->Vertices[i].Pos.z = v.z;
            }
          }
        }
      }
    }
  }

  // done;

  sRelease(spline_);
  sRelease(mesh0);
  sRelease(mesh1);
  sRelease(mesh2);

  mesh->MergeClusters();

  return mesh;
}

/****************************************************************************/

GenMinMesh * __stdcall MinMesh_Multiply(KOp *,GenMinMesh *mesh,sFSRT srt,sInt count,sInt mode,sF32 tu,sF32 tv,sF323 lrot,sF32 extrude)
{
	GenMinMesh *out;
	sMatrix xform,step,lxform,lstep,tmp;
  sVector p;

  if(CheckMinMesh(mesh)) return 0;

	step.InitSRT(srt.v);
  lstep.InitEulerPI2(&lrot.x);
	out = new GenMinMesh;
	xform.Init();
  lxform.Init();
//  if(extrude)
//    mesh->NeedAllNormals();

  sSetRndSeed(count);
	for(sInt j=0;j<count;j++)
	{
    sInt startvert = out->Vertices.Count;
		out->Add(mesh); //,!!j);   // 0:not keepmaterial: 1..n keepmaterial

    tmp.MulA(lxform,xform);

    for(sInt i=startvert;i<out->Vertices.Count;i++)
    {
      GenMinVert *v = &out->Vertices[i];
      p.x = v->Pos.x;
      p.y = v->Pos.y;
      p.z = v->Pos.z;
      p.Rotate34(tmp);
      v->Pos.x = p.x;
      v->Pos.y = p.y;
      v->Pos.z = p.z;

      if(mode&1)
      {
        v->UV[0][0] += j*tu;
        v->UV[0][1] += j*tv;
      }
    }

//    if(extrude)
//    {
//      tmp.Init();
//      tmp.i.x = j*extrude;
//      tmp.j.y = j*extrude;
//      tmp.k.z = j*extrude;
//      out->TransVert(tmp,sGMI_NORMAL,sGMI_POS | 0x10);
//    }

    if(mode&2)
      xform.InitRandomSRT(srt.v);
    else
		  xform.MulA(step);

    lxform.MulA(lstep);
	}

  mesh->Release();
  out->MergeClusters();
	return out;
}

GenMinMesh * __stdcall MinMesh_Multiply2(sInt seed,sInt3 count1,sF323 translate1,sInt3 count2,sF323 translate2,sInt random,sInt3 count3,sF323 translate3,sInt inCount,GenMinMesh *inMesh,...)
{
  if(!inCount)
    return 0;

  GenMinMesh *mesh = new GenMinMesh;
  sSetRndSeed(seed);

  sInt start = 0;
  sMatrix xform;
  xform.Init();

  if(count3.x*count3.y*count3.z * count2.x*count2.y*count2.z * count1.x*count1.y*count1.z > 1024)
  {
    count3.Init(1,1,1);
    count2.Init(1,1,1);
    count1.Init(1,1,1);
  }

  for(sInt z3=0;z3<count3.z;z3++)
  {
    for(sInt y3=0;y3<count3.y;y3++)
    {
      for(sInt x3=0;x3<count3.x;x3++)
      {

        for(sInt z2=0;z2<count2.z;z2++)
        {
          for(sInt y2=0;y2<count2.y;y2++)
          {
            for(sInt x2=0;x2<count2.x;x2++)
            {

              for(sInt z1=0;z1<count1.z;z1++)
              {
                for(sInt y1=0;y1<count1.y;y1++)
                {
                  for(sInt x1=0;x1<count1.x;x1++)
                  {
                    start = mesh->Vertices.Count;

                    GenMinMesh *in = (&inMesh)[sMin<sInt>(sGetRnd(inCount+random),inCount-1)];
                    mesh->Add(in);

                    xform.l.x = x1 * translate1.x + x2 * translate2.x + x3 * translate3.x;
                    xform.l.y = y1 * translate1.y + y2 * translate2.y + y3 * translate3.y;
                    xform.l.z = z1 * translate1.z + z2 * translate2.z + z3 * translate3.z;

                    for(sInt i=start;i<mesh->Vertices.Count;i++)
                    {
                      mesh->Vertices[i].Pos.x += xform.l.x;
                      mesh->Vertices[i].Pos.y += xform.l.y;
                      mesh->Vertices[i].Pos.z += xform.l.z;
                    }
                  }
                }
              }

            }
          }
        }

      }
    }
  }

  for(sInt i=0;i<inCount;i++)
    (&inMesh)[i]->Release();

  return mesh;
}

/****************************************************************************/

GenMinMesh * __stdcall MinMesh_Center(GenMinMesh *mesh,sInt which)
{
  sAABox box;
  sF32 tx,ty,tz;

  if(CheckMinMesh(mesh)) return 0;

  mesh->CalcBBox(box);
  tx = (which&1)?(box.Max.x+box.Min.x)/2:0;
  ty = (which&2)?(box.Max.y+box.Min.y)/2:0;
  tz = (which&4)?(box.Max.z+box.Min.z)/2:0;
  for(sInt i=0;i<mesh->Vertices.Count;i++)
  {
    mesh->Vertices[i].Pos.x -= tx;
    mesh->Vertices[i].Pos.y -= ty;
    mesh->Vertices[i].Pos.z -= tz;
  }
  return mesh;
}

/****************************************************************************/

GenMinMesh * __stdcall MinMesh_RenderAutoMap(GenMinMesh *mesh,sInt flags)
{
  sAABox box;
  GenMinFace *face;
  GenMinVert *vert;
  if(CheckMinMesh(mesh)) return 0;
  sF32 map[6][6];
  sF32 uv[6][2];
  sMatrix mat;
  sVector min,max;
  sInt fields[6];
  sInt fieldmax;

  mesh->CalcNormals();
  mesh->CalcBBox(box);

  fieldmax = 0;
  for(sInt i=0;i<6;i++)
  {
    fields[i] = -1;
    if(flags & (1<<i))
      fields[i] = fieldmax++;
    else
      fields[i] = -1;
  }

  if(fields[0]==-1 && fields[1]>=0) fields[0] = fields[1];    // provide good defaults for unspecified directions
  if(fields[0]==-1 && fields[4]>=0) fields[0] = fields[4];
  if(fields[0]==-1 && fields[5]>=0) fields[0] = fields[5];
  if(fields[0]==-1 && fields[2]>=0) fields[0] = fields[2];
  if(fields[0]==-1 && fields[3]>=0) fields[0] = fields[3];
  if(fields[1]==-1)                 fields[1] = fields[0];
  if(fields[4]==-1 && fields[5]>=0) fields[4] = fields[5];
  if(fields[4]==-1)                 fields[4] = fields[0];
  if(fields[5]==-1)                 fields[5] = fields[4];
  if(fields[2]==-1 && fields[3]>=0) fields[2] = fields[3];
  if(fields[2]==-1)                 fields[2] = fields[0];
  if(fields[3]==-1)                 fields[3] = fields[2];

  for(sInt i=0;i<mesh->Vertices.Count;i++)
    mesh->Vertices[i].TempByte=-1;

  for(sInt i=0;i<mesh->Faces.Count;i++)
  {
    face = &mesh->Faces[i];
    face->Temp = face->Normal.Classify()^1;  
    for(sInt j=0;j<face->Count;j++)
      mesh->Vertices[face->Vertices[j]].TempByte = face->Temp;
  }

  for(sInt i=0;i<6;i++)
  {
    mat.InitClassify(i);
    min.Rotate34(mat,box.Min);
    max.Rotate34(mat,box.Max);
    
    map[i][0] = mat.i.x / sFAbs(max.x-min.x) * (1.0f/fieldmax);
    map[i][1] = mat.i.y / sFAbs(max.y-min.y) * (1.0f/fieldmax);
    map[i][2] = mat.i.z / sFAbs(max.z-min.z) * (1.0f/fieldmax);
    map[i][3] = mat.j.x / sFAbs(max.x-min.x);
    map[i][4] = mat.j.y / sFAbs(max.y-min.y);
    map[i][5] = mat.j.z / sFAbs(max.z-min.z);

//    if(min.x>max.x) sSwap(min.x,max.x);
//    if(min.y>max.y) sSwap(min.y,max.y);
//    if(min.z>max.z) sSwap(min.z,max.z);

    uv[i][0] = -(map[i][0]*min.x + map[i][1]*min.y + map[i][2]*min.z) + (1.0f*fields[i]/fieldmax);
    uv[i][1] = -(map[i][3]*min.x + map[i][4]*min.y + map[i][5]*min.z);
  }

  for(sInt i=0;i<mesh->Vertices.Count;i++)
  {
    vert = &mesh->Vertices[i];
    sVERIFY(vert->TempByte != -1);
    vert->UV[0][0] = map[vert->TempByte][0]*vert->Pos.x 
                   + map[vert->TempByte][1]*vert->Pos.y
                   + map[vert->TempByte][2]*vert->Pos.z
                   + uv[vert->TempByte][0];
    vert->UV[0][1] = map[vert->TempByte][3]*vert->Pos.x 
                   + map[vert->TempByte][4]*vert->Pos.y
                   + map[vert->TempByte][5]*vert->Pos.z
                   + uv[vert->TempByte][1];
    vert->UV[0][1] = 1-vert->UV[0][1];
  }

  return mesh;
}

/****************************************************************************/
/****************************************************************************/

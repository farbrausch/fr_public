// This file is distributed under a BSD license. See LICENSE.txt for details.

#include <_loader.hpp>
#if sLINK_LOADER

#if sLINK_MINMESH
#include <genminmesh.hpp>
#include <genoverlay.hpp>
#endif

using namespace sLoader;

#define DEBUGOUT 0

/****************************************************************************/
/****************************************************************************/

sBool Vertex::Equals(const Vertex &v) const
{
  if(Pos.x!=v.Pos.x) return 0;
  if(Pos.y!=v.Pos.y) return 0;
  if(Pos.z!=v.Pos.z) return 0;
  if(Color!=v.Color) return 0;
  if(UV[0][0]!=v.UV[0][0]) return 0;
  if(UV[0][1]!=v.UV[0][1]) return 0;
  if(BoneCount!=v.BoneCount) return 0;
  for(sInt i=0;i<BoneCount;i++)
  {
    if(BoneModel[i]!=v.BoneModel[i]) return 0;
    if(BoneWeight[i]!=v.BoneWeight[i]) return 0;
  }
  return 1;
}

sBool Vertex::LessThan(const Vertex &v) const
{
  return sCmpMem(this,&v,sizeof(Vertex)) < 0;
}

void Vertex::AddBone(Model *bone,sF32 val)
{
  if(BoneCount>=MAX_WEIGHTS)
  {
    sInt best = 0;
    for(sInt i=1;i<BoneCount;i++)
    {
      if(BoneWeight[i]<BoneWeight[best])
        best = i;
    }

    if(val<BoneWeight[best])
      return;

    BoneCount--;
    BoneWeight[best] = BoneWeight[BoneCount];
    BoneModel[best] = BoneModel[BoneCount];
  }

  BoneWeight[BoneCount] = val;
  BoneModel[BoneCount] = bone;
  BoneCount++;
}

/****************************************************************************/

void Face::Init(sInt count)
{
  Count = count;
  Index = new sInt[count];
}

void Face::Exit()
{
  delete[] Index;
}


/****************************************************************************/
/****************************************************************************/

Cluster::Cluster()
{
}

Cluster::~Cluster()
{
  Face *face;

  sFORALL(Faces,face)
    face->Exit();
}

/****************************************************************************/

static sF32 EvalCubicBezier(sF32 x0,sF32 d0,sF32 d3,sF32 x3,sF32 t)
{
  sF32 d03 = x3 - x0;
  sF32 d12 = d03 + d3 - d0;

  return (((d03 - 3*d12)*t + 3 * (d12 - d0))*t + 3*d0) * t + x0;
}

static sF32 FindTime(sF32 d0,sF32 d3,sF32 time)
{
  sF32 x0=0.0f,x3=1.0f;
  sF32 t0=0.0f,t1=1.0f;

  // first, do 8 steps of bisection to find the approximate region
  // we're in, then just approximate with a linear function
  for(sInt step=0;step<8;step++)
  {
    // calculate new (middle) control point of subdivided curve
    sF32 midp = 0.5f * (x0 + x3) + 0.375f * (d0 + d3);
    sF32 midt = 0.5f * (t0 + t1);

    if(midp < time) // continue in left subinterval
    {
      x3 = midp;
      d3 = 0.25f * (x0 - x3) + 0.125f * (d0 - d3);
      d0 *= 0.5f;
      t1 = midt;
    }
    else // continue in right subinterval
    {
      x0 = midp;
      d0 = 0.25f * (x3 - x0) + 0.125f * (d3 - d0);
      d3 *= 0.5f;
      t0 = midt;
    }
  }

  return t0 + (t1 - t0) * (time - x0) / (x3 - x0);
}

sF32 CubicInterpolate(const CubicKey &a,const CubicKey &b,sF32 t)
{
  sF32 time = FindTime(a.RTanX,b.LTanX,t);
  return EvalCubicBezier(a.Value,a.RTanY,b.LTanY,b.Value,time);
}

/****************************************************************************/

Model::Model()
{
  Transform[0] = 1;
  Transform[1] = 1;
  Transform[2] = 1;
  Transform[3] = 0;
  Transform[4] = 0;
  Transform[5] = 0;
  Transform[6] = 0;
  Transform[7] = 0;
  Transform[8] = 0;

  BasePose[0] = 1;
  BasePose[1] = 1;
  BasePose[2] = 1;
  BasePose[3] = 0;
  BasePose[4] = 0;
  BasePose[5] = 0;
  BasePose[6] = 0;
  BasePose[7] = 0;
  BasePose[8] = 0;

  AnimKeys = 0;
  for(sInt i=0;i<9;i++)
    AnimCurves[i] = 0;

  AnimIndex = -1;
  Animated = sFALSE;
  Parent = 0;

  OriginalVertexCount = 0;
  Visibility = 0;
}

Model::~Model()
{
  for(sInt i=0;i<9;i++)
    sDeleteArray(AnimCurves[i]);
}

/****************************************************************************/

Scene::Scene()
{
  Root = 0;
  Optimized = 0;
}

Scene::~Scene()
{
  delete Root;
}

sBool Scene::LoadCube()
{
  Cluster *clus;
  Vertex *vert;
  Face *face;
  static sInt cube[6][4] =
  {
    { 3,2,1,0, },
    { 4,5,6,7, },
    { 0,1,5,4, },
    { 1,2,6,5, },
    { 2,3,7,6, },
    { 3,0,4,7, },
  };

  clus = new Cluster;
  clus->Vertices.Resize(8);
  clus->Faces.Resize(6);
  sFORALL(clus->Vertices,vert)
  {
    vert->Pos.x = ((_i+0)&2) ? -1 : 1;
    vert->Pos.y = ((_i+1)&2) ? -1 : 1;
    vert->Pos.z = ((_i+0)&4) ? -1 : 1;
    vert->Color = ~0;
  }
  sFORALL(clus->Faces,face)
  {
    face->Init(4);
    for(sInt j=0;j<4;j++)
      face->Index[j] = cube[_i][j];
  }

  Root = new Model;
  Root->Clusters.Add(clus);

  return sTRUE;
}

Material *Scene::FindMtrl(const sChar *name)
{
  Material *mtrl;

  sFORALL(Mtrls,mtrl)
    if(mtrl->Name==name)
      return mtrl;

  return 0;
}

Model *Scene::FindModelR(const sChar *name,Model *parent)
{
  Model *model;

  if(parent->Name == name)
    return parent;

  sFORALL(parent->Childs,model)
  {
    model = FindModelR(name,model);
    if(model)
      return model;
  }

  return 0;
}

Model *Scene::FindModel(const sChar *name)
{
  return FindModelR(name,Root);
}

/****************************************************************************/
/****************************************************************************/

namespace XSILoader{
sBool Main(Scene *scene,sChar *text);
};

sBool Scene::LoadXSI(sChar *name)
{
  sChar *text = sSystem->LoadText(name);
  if(!text) return 0;

  sBool result = XSILoader::Main(this,text);

  delete[] text;
  return result;
}

/****************************************************************************/
/****************************************************************************/

void Scene::OptimizeR(Model *m,const sMatrix &mat)
{
  Model *child;
  Cluster *cluster;
  Vertex *vert;

  Models.Add(m);        // add invisible nodes, they might be required for animation

  if(m->Visibility)
  {
    sFORALL(m->Clusters,cluster)
    {
      cluster->AnimType = m->Animated ? 1 : 0;

      sFORALL(cluster->Vertices,vert)
      {
        if(vert->BoneCount==0)
        {
          vert->BoneCount=1;
          vert->BoneModel[0] = m;
          vert->BoneWeight[0] = 1.0f;
        }

        if(vert->BoneCount != 1 || vert->BoneModel[0] != m)
        {
          cluster->AnimType = 2; // skinned.

          sF32 total=0;
          for(sInt i=0;i<vert->BoneCount;i++)
            total += vert->BoneWeight[i];
          total = 1.0f/total;
          for(sInt i=0;i<vert->BoneCount;i++)
            vert->BoneWeight[i] *= total;
        }
      }

      Clusters.Add(cluster);
    }
  }

  sFORALL(m->Childs,child)
  {
    child->Animated |= m->Animated;
    OptimizeR(child,mat);
    child->Parent = m;
  }
}

// Randomized Quicksort with Bentley-McIllroy-3-way-partitioning
// (after Sedgewick)
void Scene::VertexListSort(const Vertex *verts,sInt *inds,sInt l,sInt r)
{
  while(l < r)
  {
    // pick random pivot and exchange to right side
    sInt randomPivot = sGetRnd(r-l+1) + l;
    sSwap(inds[randomPivot],inds[r]);

    // partition
    sInt i = l-1, j = r;
    sInt p = l-1, q = r;
    Vertex v = verts[inds[r]];

    for(;;)
    {
      while(verts[inds[++i]].LessThan(v));
      while(v.LessThan(verts[inds[--j]]) && j != l);

      if(i >= j)
        break;

      sSwap(inds[i],inds[j]);
      if(verts[inds[i]].Equals(v)) sSwap(inds[++p],inds[i]);
      if(verts[inds[j]].Equals(v)) sSwap(inds[--q],inds[j]);
    }

    sSwap(inds[i],inds[r]);
    j = i-1;
    i = i+1;

    for(sInt k=l;k<p;k++,j--)
      sSwap(inds[k],inds[j]);

    for(sInt k=r-1;k>q;k--,i++)
      sSwap(inds[i],inds[k]);

    // recurse
    if(j-l < r-i)
    {
      VertexListSort(verts,inds,l,j);
      l = i;
    }
    else
    {
      VertexListSort(verts,inds,i,r);
      r = j;
    }
  }
}

sBool Scene::Optimize()
{
  sMatrix mat;
  Face *face,*newface;
  Model *m;
  Cluster *cl;

  if(Optimized) return 1;

  mat.Init();

  // find jobs

  OptimizeR(Root,mat);

  // reindex vertices

  sFORALL(Clusters,cl)
  {
    sInt max = cl->Vertices.Count;
    sInt newmax/*,ok*/;
    sInt *index = new sInt[max];
    sInt *map = new sInt[max];
    sInt *order = new sInt[max];

    // sort vertices, eliminate equal ones
    for(sInt i=0;i<max;i++)
      order[i] = i;

    VertexListSort(&cl->Vertices[0],order,0,max-1);
    sInt lastUnique = -1;

    for(sInt i=0;i<max;i++)
    {
      if(i && cl->Vertices[order[i]].Equals(cl->Vertices[lastUnique]))
        map[order[i]] = lastUnique;
      else
        map[order[i]] = lastUnique = order[i];
    }

    // reindex
    newmax = 0;
    for(sInt i=0;i<max;i++)
    {
      if(map[i] == i) // first of its kind
      {
        index[newmax] = i;
        map[i] = newmax++;
      }
      else
        map[i] = -(map[i] + 1);
    }

    for(sInt i=0;i<max;i++)
      if(map[i] < 0)
        map[i] = map[-map[i] - 1];

    /*newmax = 0;
    for(sInt i=0;i<max;i++)
    {
      ok = 0;
      for(sInt j=0;j<i && !ok;j++)
      {
        if(cl->Vertices[i].Equals(cl->Vertices[j]))
        {
          map[i] = map[j];
          ok = 1;
        }
      }
      if(!ok)
      {
        index[newmax] = i;
        map[i] = newmax;
        newmax++;
      }
    }*/

    for(sInt i=0;i<newmax;i++)
      cl->Vertices[i] = cl->Vertices[index[i]];
    cl->Vertices.Count = newmax;

    sFORALL(cl->Faces,face)
    {
      for(sInt i=0;i<face->Count;i++)
        face->Index[i] = map[face->Index[i]];
    }

    delete[] index;
    delete[] map;
    delete[] order;
  }

  // triangulate faces with more than 8 vertices

  sFORALL(Clusters,cl)
  {
    sFORALL(cl->Faces,face)
    {
      if(face->Count>KMM_MAXVERT)
      {
        for(sInt i=3;i<face->Count;i++)
        {
          newface = cl->Faces.Add();  // add new face, reallocate
          face = &cl->Faces[_i];   // face-ptr may not be valid anymore!
          newface->Init(3);
          newface->Index[0] = face->Index[0];
          newface->Index[1] = face->Index[i-1];
          newface->Index[2] = face->Index[i-0];
        }
        face->Count = 3;
      }
    }
  }

  // index model-transforms

  sFORALL(Models,m)
    m->AnimIndex = _i;

  // done

  Optimized = 1;
  return 1;
}

/****************************************************************************/
/****************************************************************************/

#if sLINK_MINMESH

sBool Scene::CreateMesh(class GenMinMesh *mesh)
{
  Cluster *cl;
  Model *m;
  Vertex *vert;
  Face *face;
  GenMinVert *vp;
  GenMinFace *fp;
  GenMinMatrix *mp;
  sInt cli;

  sInt vc;
  sInt fc;
  sInt mc;

  Optimize();

  vc = fc = 0;
  sFORALL(Clusters,cl)
  {
    vc += cl->Vertices.GetCount();
    fc += cl->Faces.GetCount();
  }
  mc = Mtrls.GetCount();
  
  //mesh->Clear();
  mesh->Faces.Resize(fc);
  mesh->Vertices.Resize(vc);
  mesh->CreateAnimation(Models.GetCount());

  sFORALL(Models,m)
    m->AnimIndex = _i;

  mp = mesh->Animation->Matrices.Array;
  sFORALL(Models,m)
  {
    mp->Init();
    mp->BasePose.InitSRTInv(m->BasePose);
    mp->NoAnimation.InitSRT(m->Transform);

    if(m->AnimKeys)
    {
      mp->KeyCount = m->AnimKeys;
      for(sInt i=0;i<9;i++)
        mp->SRT[i] = m->Transform[i];

      sVERIFY((&mp->SPtr)+1 == &mp->RPtr);
      sVERIFY((&mp->SPtr)+2 == &mp->TPtr);
      for(sInt i=0;i<3;i++)
      {
        if(m->AnimCurves[0+i*3] || m->AnimCurves[1+i*3] || m->AnimCurves[2+i*3])
        {
          (&mp->SPtr)[i] = new sF32[mp->KeyCount*3];
          for(sInt j=0;j<mp->KeyCount;j++)
          {
            for(sInt k=0;k<3;k++)
            {
              if(m->AnimCurves[k+i*3])
                (&mp->SPtr)[i][j*3+k] = m->AnimCurves[k+i*3][j];
              else
                (&mp->SPtr)[i][j*3+k] = m->Transform[k+i*3];
            }
          }
        }
      }
    }


    if(m->Parent)
    {
      mp->Parent = m->Parent->AnimIndex;
      sVERIFY(m->Parent->AnimIndex<m->AnimIndex);
    }
    else
    {
      mp->Parent = -1;
    }

    mp++;
  }

  // evaluate animation at frame 0, so we may pretransform "static" models
  sMatrix *matrices = new sMatrix[mesh->Animation->Matrices.Count];
  sMatrix *matrices2 = new sMatrix[mesh->Animation->Matrices.Count];
  mesh->Animation->EvalAnimation(0,0,matrices);

  // build vertices
  sInt clusterTypes[3] = { 0 };
  
  vp = &mesh->Vertices[0];
  fp = &mesh->Faces[0];
  vc = 0;
  sFORALL(Clusters,cl)
  {
    cli = 1;
    if(cl->Mtrl)
    {
      sInt matrix = -1;
      if(cl->AnimType == 1 && cl->Vertices.GetCount() != 0)
      {
        sVERIFY(cl->Vertices[0].BoneCount >= 1);
        matrix = cl->Vertices[0].BoneModel[0]->AnimIndex;
      }

      cli = mesh->AddCluster(0,0,cl->Mtrl->Id,cl->AnimType,matrix);
    }

    clusterTypes[cl->AnimType]++;

    sFORALL(cl->Vertices,vert)
    {
      sSetMem(vp,0,sizeof(*vp));
      vp->Select = 1;
      vp->Color = vert->Color;

      sVector pos = vert->Pos;
      sVector normal = vert->Normal;
      if(cl->AnimType == 0) // static, pretransform!
      {
        pos.Rotate34(matrices[vert->BoneModel[0]->AnimIndex]);
        normal.Rotate3(matrices[vert->BoneModel[0]->AnimIndex]);
      }

      vp->Pos.Init(pos);
      vp->Normal.Init(normal);
      vp->Tangent.Init(0,0,0);
      for(sInt i=0;i<KMM_MAXUV && i<4;i++)
      {
        vp->UV[i][0] = vert->UV[i][0];
        vp->UV[i][1] = vert->UV[i][1];
      }
      vp->BoneCount = cl->AnimType ? sMin(4,vert->BoneCount) : 0;
      for(sInt i=0;i<vp->BoneCount;i++)
      {
        vp->Weights[i] = vert->BoneWeight[i];
        vp->Matrix[i] = vert->BoneModel[i]->AnimIndex;
      }
      vp++;
    }
    sFORALL(cl->Faces,face)
    {
      sSetMem(fp,0,sizeof(*fp));
      fp->Select = 1;
      fp->Count = face->Count;
      fp->Cluster = cli;
      sVERIFY(fp->Count<=KMM_MAXVERT);
      for(sInt i=0;i<face->Count;i++)
        fp->Vertices[i] = vc + face->Index[i];
      fp++;
    }
    vc += cl->Vertices.GetCount();
  }

  delete[] matrices;
  delete[] matrices2;

  sDPrintF("CreateMesh done. Cluster animation types:\n");
  sDPrintF("  static  %5d clusters\n",clusterTypes[0]);
  sDPrintF("  rigid   %5d clusters\n",clusterTypes[1]);
  sDPrintF("  skinned %5d clusters\n",clusterTypes[2]);
  sDPrintF("  total   %5d clusters\n",Clusters.GetCount());
  sDPrintF("  mesh    %5d clusters\n",mesh->Clusters.Count);

  return sTRUE;
}

#endif  // sLINK_MINMESH

/****************************************************************************/
/****************************************************************************/

namespace XSILoader
{
;

sChar *Scan;
sInt ErrorFlag;
sInt ErrorLine;
sInt Version;
sInt Line;
sInt Indent;
sString<256> Name;
sString<256> String;
sString<256> Symbol;
Scene *GlobalScene;

sInt Error()
{
  if(!ErrorFlag)
  {
    ErrorLine = Line;
    ErrorFlag = 1;
  }
  return 0;
}


void SkipSpace()
{
  while(*Scan==' ' || *Scan=='\r' || *Scan=='\t' || *Scan=='\n')
  {
    if(*Scan=='\n')
      Line++;
    Scan++;
  }
}

sBool Match(sChar c)
{
  SkipSpace();
  if(*Scan==c)
  {
    Scan++;
    return 1;
  }
  else
  {
    return Error();
  }
}

sBool MatchOpen()  { return Match('{'); }
sBool MatchClose() { return Match('}'); }
sBool MatchComma() { return Match(','); }
sBool MatchSemi()  { return Match(';'); }

sInt MatchInt()
{
  sInt i;
  SkipSpace();
  i = sScanInt(Scan);
  MatchComma();
  return i;
}

sF32 MatchFloat()
{
  sF32 f;
  SkipSpace();
  f = sScanFloat(Scan);
  MatchComma();
  return f;
}

sBool MatchString()
{
  sInt i;
  SkipSpace();
  if(*Scan++!='"') return 0;

  i = 0;
  while(*Scan!='"')
  {
    if(i>=String.Size()-1 || *Scan==0)
      return Error();
    if(*Scan=='\n')
      Line++;

    String[i++] = *Scan++;
  }
  String[i++] = 0;
  Match('"');
  return MatchComma();
}

sBool MatchName()
{
  sInt i;
  SkipSpace();

  i = 0;
  while(*Scan>=0x21)
  {
    if(i>=Name.Size()-1)
      return Error();
    Name[i++] = *Scan++;
  }
  Name[i] = 0;

  return 1;
}

sInt MatchSymbol()
{
  sInt i;
  SkipSpace();

  i = 0;
  while(*Scan>=0x21)
  {
    if(i>=Symbol.Size()-1)
      return Error();
    Symbol[i++] = *Scan++;
  }
  Symbol[i] = 0;

  return 1;
}

void ScanSection()
{
  MatchName();
  SkipSpace();
#if DEBUGOUT
  sDPrintF("\n");
  for(sInt i=0;i<Indent;i++)
    sDPrintF("  ");
  sDPrintF("%s",(sChar *)Name);
#endif

  if(*Scan=='{')
  {
    Symbol[0] = 0;
  }
  else
  {
    MatchSymbol();
#if DEBUGOUT
    sDPrintF(" (%s)",(sChar *)Symbol);
#endif
  }
  MatchOpen();
}

void SkipSection()
{
  sInt level = 1;

  while(*Scan && level>0)
  {
    if(*Scan=='{') level++;
    if(*Scan=='}') level--;
    if(*Scan=='\n') Line++;
    *Scan++;
  }
}

/****************************************************************************/

void XSI_Material()
{
  ScanSection();
  Indent++;
  Material *mtrl = new Material;

  mtrl->Name = Symbol;
  mtrl->Id = 0;
  sChar *s = Symbol;
  const sChar *p = sFindString(s,"_");
  if(p)
  {
    p++;
    mtrl->Id = sScanInt(p);
  }

  GlobalScene->Mtrls.Add(mtrl);
#if DEBUGOUT
  sDPrintF(" (mtrl %d <%s>)",mtrl->Id,(sChar *)mtrl->Name);
#endif

  SkipSection();
  Indent--;
}

void SI_MaterialLibrary()
{
  sInt count = MatchInt();
  for(sInt i=0;i<count;i++)
  {
    XSI_Material();
  }
  MatchClose();
}

class XSIClusterTSpace
{
public:
  sString<64> Name;
  sInt Index;
  sInt Count;
  sF32 *UV;

  XSIClusterTSpace()
  {
    Index = 0;
    Count = 0;
    UV = 0;
  }

  ~XSIClusterTSpace()
  {
    delete[] UV;
  }
};

class XSICluster
{
public:
  sInt PosCount;
  sVector *Positions;

  sInt NormalCount;
  sVector *Normals;

  sInt ColorCount;
  sU32 *Colors;

  sAutoArray<XSIClusterTSpace> TSpaces;

  XSICluster()
  {
    PosCount = 0;
    Positions = 0;

    NormalCount = 0;
    Normals = 0;

    ColorCount = 0;
    Colors = 0;
  }

  ~XSICluster()
  {
    delete[] Positions;
    delete[] Normals;
    delete[] Colors;
  }
};

XSICluster *SI_Shape()
{
  XSICluster *cl = new XSICluster;
  sInt sect;
  sInt count;

  sect = MatchInt();
  MatchString(); if(String!="ORDERED") Error();

  if(ErrorFlag) return 0;

  for(sInt i=0;i<sect;i++)
  {
    count = MatchInt();
    MatchString();
    if(String=="POSITION")
    {
      sVERIFY(cl->Positions==0);
      cl->PosCount = count;
      cl->Positions = new sVector[count];

      for(sInt j=0;j<count && !ErrorFlag;j++)
      {
        cl->Positions[j].x = MatchFloat();
        cl->Positions[j].y = MatchFloat();
        cl->Positions[j].z = MatchFloat();
        cl->Positions[j].w = 1;
      }
    }
    else if(String=="NORMAL")
    {
      sVERIFY(cl->Normals==0);
      cl->NormalCount = count;
      cl->Normals = new sVector[count];

      for(sInt j=0;j<count && !ErrorFlag;j++)
      {
        cl->Normals[j].x = MatchFloat();
        cl->Normals[j].y = MatchFloat();
        cl->Normals[j].z = MatchFloat();
        cl->Normals[j].w = 1;
      }
    }
    else if(String=="COLOR")
    {
      sVERIFY(cl->Colors==0);
      cl->ColorCount = count;
      cl->Colors = new sU32[count];

      for(sInt j=0;j<count && !ErrorFlag;j++)
      {
        sVector v;

        v.x = MatchFloat();
        v.y = MatchFloat();
        v.z = MatchFloat();
        v.w = MatchFloat();

        cl->Colors[j] = v.GetColor();
      }
    }
    else if(sCmpMem((sChar *)String,"TEX_COORD_UV",12)==0)
    {
      XSIClusterTSpace *ts = new XSIClusterTSpace;
      sChar *s = 12+(sChar *)String;
      
      ts->Count = count;
      ts->UV = new sF32[count*2];
      ts->Index = sScanInt(s);
      MatchString(); ts->Name = (sChar *)String;

      for(sInt j=0;j<count && !ErrorFlag;j++)
      {
        ts->UV[j*2+0] = MatchFloat();
        ts->UV[j*2+1] = 1-MatchFloat();
      }

      cl->TSpaces.Add(ts);
    }
    else
    {
      Error();
    }

    if(ErrorFlag) break;
  }
  MatchClose();

  return cl;
}

Cluster *SI_PolygonList(XSICluster *xcl)
{
  Cluster *cl;
  XSIClusterTSpace *ts;
  Material *mtrl;
  Face *face;
  Vertex *vert;
  sString<256> what_;
  sChar *what;
  sInt fc;
  sInt ic;
  sInt c,tc,tsi;


  fc = MatchInt();
  MatchString(); 
  what_ = String;
  what = (sChar *)what_;
  MatchString(); 
  mtrl = GlobalScene->FindMtrl(String);
  ic = MatchInt();

  if(ErrorFlag) return 0;

// count

  cl = new Cluster;
  cl->Mtrl = mtrl;
  cl->Faces.Resize(fc);
  cl->Vertices.Resize(ic);
  sSetMem(&cl->Vertices[0],0,ic*sizeof(*vert));
  tc = 0;
  sFORALL(cl->Faces,face)
  {
    c = MatchInt();
    if(c<3) Error();
    if(ErrorFlag) break;
    face->Init(c);
//    for(sInt i=c-1;i>=0;i--)
    for(sInt i=0;i<c;i++)
      face->Index[i] = tc++;
  }
  if(tc!=ic) Error();

// pos

  if(ErrorFlag) return 0;
  sFORALL(cl->Vertices,vert)
  {
    c = MatchInt();
    if(c<0 || c>=xcl->PosCount) Error();
    if(ErrorFlag) break;
    vert->Pos = xcl->Positions[c];
    vert->Index = c;
  }

  while(*what)
  {
    if(ErrorFlag) return 0;
    if(*what=='|') what++;

    if(sCmpMem(what,"NORMAL",6)==0)
    {
      what+=6;
      sFORALL(cl->Vertices,vert)
      {
        c = MatchInt();
        if(c<0 || c>=xcl->NormalCount) Error();
        if(ErrorFlag) break;
        vert->Normal = xcl->Normals[c];
      }
    }
    else if(sCmpMem(what,"COLOR",5)==0)
    {
      what+=5;
      sFORALL(cl->Vertices,vert)
      {
        c = MatchInt();
        if(c<0 || c>=xcl->ColorCount) Error();
        if(ErrorFlag) break;
        vert->Color = xcl->Colors[c];
      }
    }
    else if(sCmpMem(what,"TEX_COORD_UV",12)==0)
    {
      what+=12;
      tsi = 0;
      while(*what>='0' && *what<='9')
        tsi = tsi*10 + (*what++) - '0';
      ts = 0;
      for(sInt i=0;i<xcl->TSpaces.GetCount() && ts==0;i++)
        if(xcl->TSpaces.Get(i)->Index==tsi)
          ts = xcl->TSpaces.Get(i);
      if(!ts)
      {
        Error();
        return 0;
      }
      
      sFORALL(cl->Vertices,vert)
      {
        c = MatchInt();
        if(c<0 || c>=ts->Count) Error();
        if(ErrorFlag) break;
        vert->UV[0][0] = ts->UV[c*2+0];
        vert->UV[0][1] = ts->UV[c*2+1];
      }
    }

  }
  if(ErrorFlag) return 0;

  SkipSection();

  return cl;
}

void SI_Mesh(Model *mod)
{
  Cluster *cl=0;
  XSICluster *xcl=0;

  sVERIFY(mod->OriginalVertexCount == 0);

  for(;;)
  {
    SkipSpace();
    if(*Scan==0 || *Scan=='}' || ErrorFlag) break;
    ScanSection();
    Indent++;

    if(Name=="SI_Shape")
    {
      if(xcl!=0) { Indent--; break; }
      xcl = SI_Shape();
      mod->OriginalVertexCount = xcl->PosCount;
    }
    else if(Name=="SI_PolygonList")
    {
      if(xcl==0) { Indent--; break; }
      cl = SI_PolygonList(xcl);
      if(cl)
        mod->Clusters.Add(cl);
    }
    else
    {
#if DEBUGOUT
      sDPrintF(" (skipped)");
#endif
      SkipSection();
    }

    Indent--;
  }
  delete xcl;
  MatchClose();
}

void SI_Transform(Model *mod)
{
  sF32 srt[9];

  for(sInt i=0;i<9;i++)
    srt[i] = MatchFloat();

  srt[3] *= 1.0f/360.0f;
  srt[4] *= 1.0f/360.0f;
  srt[5] *= 1.0f/360.0f;

  if(sCmpMem((sChar *)Symbol,"BASEPOSE-",9)==0)
  {
    for(sInt i=0;i<9;i++)
      mod->BasePose[i] = srt[i];
  }
  else
  {
    for(sInt i=0;i<9;i++)
      mod->Transform[i] = srt[i];
  }

  MatchClose();
}

void SI_FCurve(Model *mod)
{
  sInt index;
  sInt count;
  sF32 scale;

  index = -1;
  MatchString();
  MatchString();
  if(String=="SCALING-X")     index = 0;
  if(String=="SCALING-Y")     index = 1;
  if(String=="SCALING-Z")     index = 2;
  if(String=="ROTATION-X")    index = 3;
  if(String=="ROTATION-Y")    index = 4;
  if(String=="ROTATION-Z")    index = 5;
  if(String=="TRANSLATION-X") index = 6;
  if(String=="TRANSLATION-Y") index = 7;
  if(String=="TRANSLATION-Z") index = 8;
  if(index==-1) Error();
  MatchString();
  if(String!="CUBIC") Error();
  if(MatchInt()!=1) Error();
  if(MatchInt()!=5) Error();
  count = MatchInt();
  if(ErrorFlag) return;

  if(mod->AnimCurves[index]) Error();
  if(mod->AnimKeys>0 && mod->AnimKeys!=count) Error();
  if(ErrorFlag) return;

  mod->Animated = sTRUE;
  mod->AnimKeys = count;
  mod->AnimCurves[index] = new sF32[count];
  scale = (index>=3 && index<6) ? 1.0f/360.0f : 1.0f;

  for(sInt i=0;i<count;i++)
  {
    MatchFloat();
    mod->AnimCurves[index][i] = MatchFloat()*scale;
    MatchFloat();
    MatchFloat();
    MatchFloat();
    MatchFloat();
  }

  MatchClose();
}

Model *SI_Model()
{
  Model *mod;

  mod = new Model;
  mod->Name = Symbol;
  mod->Visibility = 1;

  for(;;)
  {
    SkipSpace();
    if(*Scan==0 || *Scan=='}' || ErrorFlag) break;
    ScanSection();
    Indent++;

    if(Name=="SI_Model")
    {
      mod->Childs.Add(SI_Model());
    }
    else if(Name=="SI_Mesh")
    {
      SI_Mesh(mod);
    }
    else if(Name=="SI_Transform")
    {
      SI_Transform(mod);
    }
    else if(Name=="SI_Visibility")
    {
      mod->Visibility = MatchInt();
      MatchClose();
    }
    else if(Name=="SI_FCurve")
    {
      SI_FCurve(mod);
    }
    else
    {
#if DEBUGOUT
      sDPrintF(" (skipped)");
#endif
      SkipSection();
    }

    Indent--;
  }
  MatchClose();

  return mod;
}

/****************************************************************************/
/****************************************************************************/

void SI_EnvelopeList()
{
  sInt count = MatchInt();
  Indent++;
  for(sInt i=0;i<count && !ErrorFlag;i++)
  {
    ScanSection();

    if(Name=="SI_Envelope")
    {
      MatchString();
      Model *model = GlobalScene->FindModel(String);
      if(model==0)
      {
        sDPrintF("\nunknown model <%s>",(sChar *)String);
        Error();
      }

      MatchString();
      Model *bone = GlobalScene->FindModel(String);
      if(bone==0)
      {
        sDPrintF("\nunknown model <%s>",(sChar *)String);
        Error();
      }

      sInt n = MatchInt();

      sF32 *map = new sF32[model->OriginalVertexCount];
      sSetMem(map,0,model->OriginalVertexCount*4);

      for(sInt j=0;j<n;j++)
      {
        sInt vert = MatchInt();
        sF32 val = MatchFloat();

        sVERIFY(vert>=0 && vert<model->OriginalVertexCount);
        map[vert] = val/100.0f;
      }

      if(!ErrorFlag)
      {
        Cluster *cl;
        sFORALL(model->Clusters,cl)
        {
          Vertex *vert;
          sFORALL(cl->Vertices,vert)
          {
            sVERIFY(vert->Index>=0 && vert->Index<model->OriginalVertexCount);
            if(map[vert->Index]>0.0f)
              vert->AddBone(bone,map[vert->Index]);
          }
        }
      }

      delete[] map;
    }

    MatchClose();
  }

  Indent--;
  MatchClose();
}

/****************************************************************************/
/****************************************************************************/

sBool Main(Scene *scene,sChar *text)
{
  Scan = text+16;
  ErrorFlag = 0;
  ErrorLine = 0;
  Line = 1;
  Indent = 0;

  for(sInt i=0;i<17;i++)
    if(text[i]==0)
      return 0;

  Version = (text[4]&15)*1000 + (text[5]&15)*100 + (text[6]&15)*10 + (text[7]&15);
  text[4]=text[5]=text[6]=text[7]='0';

  if(sCmpMem(text,"xsi 0000txt 0032",16)!=0)
    return 0;

  GlobalScene = scene;
  scene->Root = new Model;

  for(;;)
  {
    SkipSpace();
    if(*Scan==0 || ErrorFlag) break;
    ScanSection();
    Indent++;

    if(Name=="SI_Model")
    {
      scene->Root->Childs.Add(SI_Model());
    }
    else if(Name=="SI_MaterialLibrary")
    {
      SI_MaterialLibrary();
    }
    else if(Name=="SI_EnvelopeList")
    {
      SI_EnvelopeList();
    }
    else
    {
#if DEBUGOUT
      sDPrintF(" (skipped)");
#endif
      SkipSection();
    }
    Indent--;
  }
  sDPrintF("\n");

  if(Indent!=0)
  {
    sDPrintF("indent should be 0 (is %d)\n",Indent);
  }
  if(ErrorFlag)
  {
    sDPrintF("error in line %d\n",ErrorLine);
  }
  GlobalScene = 0;
  return ErrorFlag==0;
}

/****************************************************************************/

}; // namespace XSILoader

/****************************************************************************/
/****************************************************************************/

#endif  // sLINK_LOADER

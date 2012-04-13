// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "gen_mobmesh.hpp"
#include "player_mobile/engine_soft.hpp"
#include "player_mobile/pa.hpp"

#if DEMO_VERSION && (VARIANT==VARIANT_TEXTURE)
#define NOTINDEMO Column = GCC_INVISIBLE;
#else
#define NOTINDEMO
#endif

/****************************************************************************/
/***                                                                      ***/
/***   The MobMesh Class                                                  ***/
/***                                                                      ***/
/****************************************************************************/

void GenMobCluster::Init()
{
  sSetMem(this,0,sizeof(*this));
}

/****************************************************************************/

GenMobMesh::GenMobMesh()
{
  RefCount = 1;

  GenMobCluster *cl;
  Variant = GVI_MESH_MOBMESH;
  cl = Clusters.Add();
  cl->Init();
  cl = Clusters.Add();
  cl->Init();

  Soft = 0;
}

GenMobMesh::~GenMobMesh()
{
  delete Soft;
  /*
  GenMobCluster *cl;
  sFORALL(Clusters,cl)
  {
    delete cl->Image[0];
    delete cl->Image[1];
  }
  */
}

void GenMobMesh::Copy(GenMobMesh *o)
{
  PA(L"copymesh");
  Vertices.Copy(o->Vertices);
  Faces.Copy(o->Faces);
  Clusters.Copy(o->Clusters);
  CamSpline.Copy(o->CamSpline);
}

GenMobMesh *GenMobMesh::Copy()
{
  GenMobMesh *o = new GenMobMesh;
  sVERIFY(o!=this);
  o->Copy(this);
  sVERIFY(o!=this);
  return o;
}

/****************************************************************************/

GenMobVertex *GenMobMesh::AddVertices(sInt count)
{
  sInt i = Vertices.Count;
  Vertices.SetMax(i+count);
  Vertices.Count = i+count;

  sInt bytes = sizeof(GenMobVertex)*count;
  sSetMem(Vertices.Array+i,0,bytes);

  return Vertices.Array+i;
}

GenMobFace *GenMobMesh::AddFaces(sInt count)
{
  sInt i = Faces.Count;
  Faces.SetMax(i+count);
  Faces.Count = i+count;
  sSetMem(Faces.Array+i,0,sizeof(GenMobFace)*count);
  return Faces.Array+i;
}

GenMobCluster *GenMobMesh::AddClusters(sInt count)
{
  sInt i = Clusters.Count;
  Clusters.SetMax(i+count);
  Clusters.Count = i+count;
  for(sInt j=i;j<Clusters.Count;j++)
    Clusters.Array[j].Init();
  return Clusters.Array+i;
}

/****************************************************************************/

GenMobVertex *GenMobMesh::AddGrid(sInt tx,sInt ty,sInt flags)
{
  sInt x,y;
  sInt vi,fi,vc,fc,va,fa,bits,tyy;
  GenMobVertex *vp;
  GenMobFace *fp;
  sInt cluster;

  // additions (top/bottom)


  bits = 0;
  if(flags & 1) { bits++; }
  if(flags & 2) { bits++; }
  va = bits;
  fa = tx*bits;
  /*
  if(flags & 4) { fa+=tx+1; }
  if(flags & 8) { fa+=ty+1; }
  */
  cluster = (flags&32)?0:1;

  tyy = sMax(1,ty);

  // vertices

  vc = (tx+1)*(ty+1);
  vi = Vertices.Count;
  vp = AddVertices(vc+va);

  for(y=0;y<=ty;y++)
  {
    for(x=0;x<=tx;x++)
    {
      vp->Pos.x = (sFIXONE*x/tx)-(sFIXONE/2);
      vp->Pos.y = (sFIXONE*y/tyy)-(sFIXONE/2);
      vp->Select = 1;
      vp++;
    }
  }

  // faces

  fc = tx*ty;
  fi = Faces.Count;
  fp = AddFaces(fc+fa);

  for(y=0;y<ty;y++)
  {
    for(x=0;x<tx;x++)
    {
      fp->Select = 1;
      fp->Count = 4;
      fp->Cluster = cluster;
      fp->Vertex[0].Index = vi+(y+0)*(tx+1)+(x+0);
      fp->Vertex[1].Index = vi+(y+1)*(tx+1)+(x+0);
      fp->Vertex[2].Index = vi+(y+1)*(tx+1)+(x+1);
      fp->Vertex[3].Index = vi+(y+0)*(tx+1)+(x+1);
      if(flags & 16)
      {
        sSwap(fp->Vertex[0].Index,fp->Vertex[3].Index);
        sSwap(fp->Vertex[1].Index,fp->Vertex[2].Index);
      }
      for(sInt i=0;i<fp->Count;i++)
      {
        fp->Vertex[i].TexU0 = Vertices[fp->Vertex[i].Index].Pos.x + (sFIXONE/2);
        fp->Vertex[i].TexV0 = sFIXONE-Vertices[fp->Vertex[i].Index].Pos.y + (sFIXONE/2);
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
      vp->Select = 0;
      vp->Pos.y = i*sFIXONE-sFIXHALF;
      vp++;
      for(x=0;x<tx;x++)
      {
        fp->Select = 1;
        fp->Count = 3;
        fp->Cluster = cluster;
        fp->Vertex[0].Index = center;
        fp->Vertex[1].Index = border+i;
        border++;
        fp->Vertex[2].Index = border-i;
        for(sInt j=0;j<fp->Count;j++)
        {
          fp->Vertex[j].TexU0 = Vertices[fp->Vertex[j].Index].Pos.x + (sFIXONE/2);
          fp->Vertex[j].TexV0 = sFIXONE-Vertices[fp->Vertex[j].Index].Pos.y + (sFIXONE/2);
        }
        fp++;
      }
    }
    border = vi+(ty)*(tx+1);
    if(flags&1) center++;
  }

  // additions: stitches
/*
  if(flags&4)
  {
    for(sInt i=0;i<=tx;i++)
    {
      fp->Count = 2;
      fp->Vertex[0].Index = vi+i;
      fp->Vertex[1].Index = vi+i+ty*(tx+1);
      fp++;
    }
  }

  if(flags&8)
  {
    for(sInt i=0;i<=ty;i++)
    {
      fp->Count = 2;
      fp->Vertex[0].Index = vi+i*(tx+1);
//      fp->v[1] = vi+i*(tx+1)+tx;
      fp++;
    }
  }
*/
  return &Vertices[vi];
}

/****************************************************************************/

void GenMobMesh::Add(const GenMobMesh *add,sBool newclusters)
{
  GenMobVertex *vp;
  GenMobFace *fp;
  GenMobCluster *cp;
  sInt vi,ci;
  sInt i,j;

  vi = Vertices.Count;
  ci = Clusters.Count;

  vp = AddVertices(add->Vertices.Count);
  sCopyMem(vp,add->Vertices.Array,sizeof(GenMobVertex)*add->Vertices.Count);

  fp = AddFaces(add->Faces.Count);
  sCopyMem(fp,add->Faces.Array,sizeof(GenMobFace)*add->Faces.Count);

  if(newclusters && add->Clusters.Count>2)
  {
    cp = AddClusters(add->Clusters.Count-2);
    sCopyMem(cp,add->Clusters.Array+2,sizeof(GenMobCluster)*(add->Clusters.Count-2));
  }
  for(i=vi;i<Vertices.Count;i++)
    Vertices.Array[i].Select = 1;

  for(i=0;i<add->Faces.Count;i++)
  {
    for(j=0;j<fp->Count;j++)
      fp->Vertex[j].Index += vi;
    if(newclusters && fp->Cluster>=2)
      fp->Cluster = fp->Cluster-2 + ci;
    fp->Select = 1;
    fp++;
  }
}

/****************************************************************************/
/****************************************************************************/

void GenMobMesh::Transform(sInt sel,const sIMatrix34 &mat,sInt src,sInt dest)
{
  sIVector3 v;
  GenMobVertex *vp;
  GenMobFace *fp;
  sBool ok;

  vp = Vertices.Array;

  if(src==0 && dest==0)
  {
    sFORALL(Vertices,vp)
    {
      ok = 1;
      if(sel==MMU_SELECTED)
        ok = vp->Select;
      else if(sel==MMU_UNSELECTED)
        ok = !vp->Select;

      if(ok)
        vp->Pos.Rotate34(mat);
    }
  }
  else 
  {
    sFORALL(Faces,fp)
    {
      for(sInt i=0;i<fp->Count;i++)
      {
        vp = &Vertices[fp->Vertex[i].Index];

        ok = 1;
        if(sel==MMU_SELECTED)
          ok = vp->Select;
        else if(sel==MMU_UNSELECTED)
          ok = !vp->Select;

        if(ok)
        {
          switch(src)
          {
          default:
            v = vp->Pos;
            break;
          case 1:
            v.Init(fp->Vertex[i].TexU0,fp->Vertex[i].TexV0,0);
            break;
          case 2:
            v.Init(fp->Vertex[i].TexU1,fp->Vertex[i].TexV1,0);
            break;
          }
         
          v.Rotate34(mat);

          switch(dest)
          {
          default:
            vp->Pos = v;
            break;
          case 1:
            fp->Vertex[i].TexU0 = v.x;
            fp->Vertex[i].TexV0 = v.y;
            break;
          case 2:
            fp->Vertex[i].TexU1 = v.x;
            fp->Vertex[i].TexV1 = v.y;
            break;
          }
        }
      }
    }
  }
}

/****************************************************************************/

void GenMobMesh::SetAllFaces(sInt sel)
{
  GenMobFace *fp;

  sFORALL(Faces,fp)
    fp->Select = sel;  
}

/****************************************************************************/

void GenMobMesh::SetAllVerts(sInt sel)
{
  GenMobVertex *vp;

  sFORALL(Vertices,vp)
    vp->Select = sel;
}

/****************************************************************************/

void GenMobMesh::SelectAll(sInt in,sInt out)
{
  GenMobVertex *vp;
  sInt ok;

  sFORALL(Vertices,vp)
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
  }
}

/****************************************************************************/


void GenMobMesh::CalcNormals()
{
  GenMobVertex *mv,*v0,*v1,*v2;
  GenMobFace *mf;
  sIVector3 n,d1,d2,n0;
  sInt i;

  {sFORALL(Vertices,mv)
  {
    mv->Normal.Init(0,0,0);
  }}

  {sFORALL(Faces,mf)
  {
    n.Init(0,0,0);
    for(i=2;i<mf->Count;i++)
    {
      v0 = &Vertices[mf->Vertex[0].Index];
      v1 = &Vertices[mf->Vertex[i-1].Index];
      v2 = &Vertices[mf->Vertex[i].Index];

      d1.Sub(v0->Pos,v1->Pos);
      d2.Sub(v0->Pos,v2->Pos);
      n0.Cross(d1,d2);

      n.Add(n0);
    }
    n.Unit();
    mf->Normal.Init(n);

    for(i=0;i<mf->Count;i++)
    {
      Vertices[mf->Vertex[i].Index].Normal.Add(n);
    }
  }}

  {sFORALL(Vertices,mv)
  {
    mv->Normal.Unit();
  }}
}

/****************************************************************************/

void GenMobMesh::MergeVerts()
{
  GenMobVertex *vs,*vc;
  GenMobFace *mf;
  sInt i,nc,j;
  sInt *Map;

  Map = new sInt[Vertices.Count];

  nc = 0;
  for(i=0;i<Vertices.Count;i++)
  {
    vs = &Vertices[i];
    for(j=0;j<i-1;j++)
    {
      vc = &Vertices[j];
      if(vs->Pos.x==vc->Pos.x && vs->Pos.y==vc->Pos.y && vs->Pos.z==vc->Pos.z)
      {
        Map[i] = j;
        goto found;
      }
    }
    Map[i] = nc;
    Vertices[nc++] = *vs;
found: ;
  }
  Vertices.Count = nc;

  sFORALL(Faces,mf)
  {
    for(i=0;i<mf->Count;i++)
      mf->Vertex[i].Index = Map[mf->Vertex[i].Index];
  }

  delete[] Map;
}

/****************************************************************************/

void GenMobMesh::SortSpline()
{
  sInt max = CamSpline.Count;
  for(sInt i=0;i<max-1;i++)
    for(sInt j=i+1;j<max;j++)
      if(CamSpline[i].Time > CamSpline[j].Time)
        CamSpline.Swap(i,j);
}

void GenMobMesh::CalcSpline(sInt time,sIVector3 &campos,sIVector3 &target)
{
  sInt keycount = CamSpline.Count;
  if(keycount<2) 
  {
    if(keycount==1)
    {
      campos = CamSpline[0].Cam;
      target = CamSpline[0].Target;
    }
    else
    {
      campos.Init(0,0,5);
      target.Init(0,0,0);
    }
  }
  else
  {
    sInt k;

    for(k=0;k<keycount-1;k++)
    {
      if(time>=CamSpline[k].Time && time<CamSpline[k+1].Time)
        goto found;
    }
    campos = CamSpline[0].Cam;
    target = CamSpline[0].Target;
    return;

found:

    sInt dest[6];    

    sInt dif,tdif,t,d0,d1;

    for(sInt i=0;i<6;i++)
    {
      dif = CamSpline[k+1].Value[i] - CamSpline[k].Value[i];
      tdif = CamSpline[k+1].Time - CamSpline[k].Time;
      t = (time - CamSpline[k].Time)*sFIXONE / tdif;

      if(k == 0) // first key
        d0 = dif;
      else
        d0 = sMulDiv(tdif , (CamSpline[k+1].Value[i] - CamSpline[k-1].Value[i]) , (CamSpline[k+1].Time - CamSpline[k-1].Time));

      if(k == keycount-2) // last key
        d1 = dif;
      else
        d1 = sMulDiv(tdif , (CamSpline[k+2].Value[i] - CamSpline[k+0].Value[i]) , (CamSpline[k+2].Time - CamSpline[k+0].Time));

      sInt r0 = sIMul(d0 + d1 - 2*dif , t);
      sInt r1 = sIMul(r0 + 3*dif - 2*d0 - d1 , t);
      dest[i] = sIMul(r1 + d0,t) + CamSpline[k].Value[i];
    }

    campos.Init(dest[0],dest[1],dest[2]);
    target.Init(dest[3],dest[4],dest[5]);
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Controlling the Calculation                                        ***/
/***                                                                      ***/
/****************************************************************************/

#if sPLAYER

GenMobMesh * CalcMobMesh(GenNode *node)
{
  GenMobMesh *obj;
  GenMobMesh * (*handler)(GenNode *) = (GenMobMesh * (*)(GenNode *)) node->Handler;
  obj = (*handler)(node);
#if sMOBILE
  sProgress(1);
#endif

  return obj;
}

GenMobMesh * CalcMobMeshCopy(GenNode *node)
{
  GenMobMesh *obj;
  GenMobMesh * (*handler)(GenNode *) = (GenMobMesh * (*)(GenNode *)) node->Handler;
  obj = (*handler)(node);
#if sMOBILE
  sProgress(1);
#endif

  return obj;
}

#else
/*
GenMobMesh *FindMobMeshInCache(sAutoArray<GenObject> *cache)
{
  GenObject *obj;

  sFORLIST(cache,obj)
    if(obj->Variant==GVI_MESH_MOBMESH)
      return (GenMobMesh *)obj;

  return 0;
}
*/
GenMobMesh *CalcMobMesh(GenNode *node)
{
  GenMobMesh *obj;
  GenMobMesh * (*handler)(GenNode *) = (GenMobMesh * (*)(GenNode *)) node->Handler;
  sVERIFY(node && node->Op);
  obj = node->Op->CacheMesh;
  if(!obj)
  {
    obj = (*handler)(node);
    if(obj && node->Cache)
    {
      node->Op->CacheMesh = obj;
    }
  }

  obj = node->Op->CacheMesh;
  sVERIFY(obj);
  obj->AddRef();

  return obj;
}

GenMobMesh *CalcMobMeshCopy(GenNode *node)
{
  GenMobMesh *obj = CalcMobMesh(node);
  GenMobMesh *copy = obj->Copy();
  obj->Release();
  return copy;
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Operators Themself!                                                ***/
/***                                                                      ***/
/****************************************************************************/

GenMobMesh * MobMesh_NodeLoad(GenNode *node)
{
  GenMobMesh *mesh;

  mesh = CalcMobMeshCopy(node->Inputs[0]);

  return mesh;
}

GenMobMesh * MobMesh_NodeStore(GenNode *node)
{
  GenMobMesh *mesh;

  if(node->StoreCache)
  {
    mesh = (GenMobMesh *)node->StoreCache;
  }
  else
  {
    mesh = CalcMobMeshCopy(node->Inputs[0]);
    mesh->RefCount += node->Para[0]+9999;
    node->StoreCache = mesh;
  }

  return mesh;
}

/****************************************************************************/
/****************************************************************************/

GenMobMesh * MobMesh_Nop(GenNode *node)
{
  GenMobMesh *mesh;

  mesh = CalcMobMeshCopy(node->Inputs[0]);

  return mesh;
}


GenMobMesh * MobMesh_Load(GenNode *node)
{
  GenMobMesh *mesh;

  mesh = CalcMobMeshCopy(node->Links[0]);

  return mesh;
}

/****************************************************************************/


GenMobMesh * MobMesh_Cube(GenNode *node)
{
  GenMobMesh *mesh;
  sIMatrix34 mat;
  sInt *tess=node->Para;
  sInt flags = node->Para[3];
  sISRT srt = *((sISRT*)&node->Para[4]);

  PA(L"mob_cube");

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
  srt.Fix14();

  mesh = new GenMobMesh;

  for(sInt i=0;i<6;i++)
  {
    sSetMem(&mat,0,sizeof(mat));
    mesh->AddGrid(tess[cube[i][0]],tess[cube[i][1]],cube[i][8]);

    (&mat.i.x)[cube[i][0]] = cube[i][2]*sFIXONE;
    (&mat.j.x)[cube[i][1]] = cube[i][3]*sFIXONE;
    mat.l.Init(cube[i][4]*(sFIXONE/2),cube[i][5]*(sFIXONE/2),cube[i][6]*(sFIXONE/2));
    mesh->Transform(MMU_SELECTED,mat);

    mat.Init();
    if(cube[i][8])
    {
      mat.l.x = sFIXONE;
      mat.i.x = -sFIXONE;
    }
    if(flags&2)            // wraparound
    {
      mat.l.x += cube[i][7]*sFIXONE;
    }
    mesh->Transform(MMU_SELECTED,mat,1,1);

    mesh->SetAllFaces(0);
    mesh->SetAllVerts(0);
  }
  mesh->SetAllFaces(1);
  mesh->SetAllVerts(1);

  // post transform
  mat.InitSRT(srt);
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

  mesh->MergeVerts();
  return mesh;
}

/****************************************************************************/

GenMobMesh * MobMesh_Grid(GenNode *node)
{
  GenMobMesh *mesh;
  sIMatrix34 mat;

  PA(L"mob_grid");

  mesh = new GenMobMesh;
  mesh->AddGrid(node->Para[1],node->Para[2],0);
  if(node->Para[0]&1)
    mesh->AddGrid(node->Para[1],node->Para[2],16);
  mat.Init();
  mat.i.Init(sFIXONE,0,0);
  mat.j.Init(0,0,sFIXONE);
  mesh->Transform(MMU_ALL,mat);
  mesh->SetAllFaces(1);
  mesh->SetAllVerts(1);
  
  return mesh;
}

/****************************************************************************/

GenMobMesh * MobMesh_Torus(GenNode *node)
{
  GenMobMesh *mesh;
  GenMobVertex *vp;
  sInt fx,fy;
  sInt closed;

  PA(L"mob_torus");

  sInt tx = node->Para[0];
  sInt ty = node->Para[1];
  sInt ro = node->Para[2]>>1;
  sInt ri = node->Para[3]>>1;
  sInt phase = node->Para[4]>>1;
  sInt arclen = node->Para[5]>>1;
  sInt flags = node->Para[6];

  closed = (arclen == sFIXONE);
  mesh = new GenMobMesh;
  mesh->AddGrid(ty,tx,closed?12:8+3);

  if(flags&1) // absolute radii
  {
    ri = (ro - ri) / 2;
    ro -= ri;
  }

  sFORALL(mesh->Vertices,vp)
  {
    fx = (sFIXHALF-vp->Pos.x-(phase/ty));
    fy = (((sFIXHALF-vp->Pos.y)*arclen)>>14);
    vp->Pos.x = ((-sICos(fy)*(ro+((sISin(fx)*ri)>>sFIXSHIFT)))>>sFIXSHIFT);
    vp->Pos.y = ((-sICos(fx)*ri)>>sFIXSHIFT);
    vp->Pos.z = ((sISin(fy)*(ro+((sISin(fx)*ri)>>sFIXSHIFT)))>>sFIXSHIFT);
  }
  if(!closed)
  {
    vp = &mesh->Vertices.Array[mesh->Vertices.Count];
    vp[-2].Pos.x = ((-sICos(arclen)*ro)>>sFIXSHIFT);
    vp[-2].Pos.y = 0;
    vp[-2].Pos.z = ((sISin(arclen)*ro)>>sFIXSHIFT);
    vp[-1].Pos.x = ((-sICos(0)*ro)>>sFIXSHIFT);
    vp[-1].Pos.y = 0;
    vp[-1].Pos.z = ((sISin(0)*ro)>>sFIXSHIFT);
  }

  // center / bottom
  if(flags&2)
  {
    sIMatrix34 mat;
    mat.Init();
    mat.l.y = ri;
    mesh->Transform(MMU_ALL,mat);
  }

  mesh->MergeVerts();
  return mesh;
}

/****************************************************************************/

GenMobMesh * MobMesh_Sphere(GenNode *node)
{
  GenMobMesh *mesh;
  GenMobVertex *vp;
  sInt fx,fy;

  PA(L"mob_sphere");

  sInt tx = node->Para[0];
  sInt ty = node->Para[1];

  mesh = new GenMobMesh;
  mesh->AddGrid(tx,ty,3+8);

  sFORALL(mesh->Vertices,vp)
  {
    fx = (sFIXHALF-vp->Pos.x);
    fy = ( (sFIXHALF/(ty+1)) + (((-vp->Pos.y+sFIXHALF)*((ty*sFIXONE)/(ty+1)))>>sFIXSHIFT) )/2;
    vp->Pos.x = (-sISin(fy)*sISin(fx))>>(sFIXSHIFT+1);
    vp->Pos.y = sICos(fy)/2;
    vp->Pos.z = (-sISin(fy)*sICos(fx))>>(sFIXSHIFT+1);
  }
  vp = &mesh->Vertices.Array[mesh->Vertices.Count];
  vp[-2].Pos.x = 0;
  vp[-2].Pos.y = mesh->Vertices[0].Pos.y;
  vp[-2].Pos.z = 0;
  vp[-1].Pos.x = 0;
  vp[-1].Pos.y = vp[-3].Pos.y;
  vp[-1].Pos.z = 0;

  mesh->MergeVerts();
  return mesh;
}

/****************************************************************************/

GenMobMesh * MobMesh_Cylinder(GenNode *node)
{
  GenMobMesh *mesh;
  GenMobVertex *vp;
  GenMobFace *fp;
  sInt fi;
  sInt fx,fy;
  sInt x,y;
  sInt closed;

  PA(L"mob_cylinder");

  sInt tx = node->Para[0];
  sInt ty = node->Para[1];
  sInt tz = node->Para[2];
  sInt flags = node->Para[3];

  if(tx<3) tx=3;
  if(ty<1) ty=1;
  if(tz<1) tz=1;

  closed = (flags & 1) ? 32 : 0;
  mesh = new GenMobMesh;

  // middle

  vp = mesh->AddGrid(tx,ty,8);
  for(y=0;y<=ty;y++)
  {
    for(x=0;x<=tx;x++)
    {
      fx = ((x==tx)?0:x*sFIXONE)/tx;
      fy = y*sFIXONE/ty;
      vp->Pos.x = (sISin(fx)*sFIXHALF)>>sFIXSHIFT;
      vp->Pos.y = fy-sFIXHALF;
      vp->Pos.z = -((sICos(fx)*sFIXHALF)>>sFIXSHIFT);
      vp++;
    }
  }
//  MinMesh_SelectAll(mesh,4);
//  mesh->SelectAll(MMU_ALL,MMS_);

  // bottom

  fi = mesh->Faces.Count;
  vp = mesh->AddGrid(tx,tz-1,closed|1);
  for(y=0;y<tz;y++)
  {
    for(x=0;x<=tx;x++)
    {
      fx = ((x==tx)?0:x*sFIXONE)/tx;
      fy = (y*sFIXONE+sFIXONE)/2/tz;
      vp->Pos.x = (sISin(fx)*fy)>>sFIXSHIFT;
      vp->Pos.y = -sFIXHALF;
      vp->Pos.z = -((sICos(fx)*fy)>>sFIXSHIFT); 
      vp++;
    }
  }
  vp->Pos.y = -sFIXHALF;
  vp++;

  // top

  vp = mesh->AddGrid(tx,tz-1,closed|2);
  for(y=tz-1;y>=0;y--)
  {
    for(x=0;x<=tx;x++)
    {
      fx = ((x==tx)?0:x*sFIXONE)/tx;
      fy = (y+1)*sFIXHALF/(tz);
      vp->Pos.x = (sISin(fx)*fy)>>sFIXSHIFT;
      vp->Pos.y = sFIXHALF;
      vp->Pos.z = -((sICos(fx)*fy)>>sFIXSHIFT); 
      vp++;
    }
  }
  vp->Pos.y = sFIXHALF;
  vp++;

  // map top and bottom

  for(;fi<mesh->Faces.Count;fi++)
  {
    fp = &mesh->Faces[fi];
    for(sInt i=0;i<fp->Count;i++)
    {
      vp = &mesh->Vertices[fp->Vertex[i].Index];
      if(vp->Pos.y<0)
      {
        fp->Vertex[i].TexU0 = vp->Pos.x+sFIXHALF;
        fp->Vertex[i].TexV0 = vp->Pos.z+sFIXHALF;
      }
      else
      {
        fp->Vertex[i].TexU0 = vp->Pos.x+sFIXHALF;
        fp->Vertex[i].TexV0 = -vp->Pos.z+sFIXHALF;
      }
    }
  }

  // center / bottom
  if(flags&2)
  {
    sIMatrix34 mat; mat.Init();
    mat.l.y = sFIXHALF;
    mesh->Transform(MMU_ALL,mat);
  }

  mesh->MergeVerts();
  return mesh;
}

/****************************************************************************/

GenMobMesh *MobMesh_CamKey(GenNode *node)
{
  GenMobMesh *mesh;
  GenMobCamKey *key;

  mesh = CalcMobMeshCopy(node->Inputs[0]);

  key = mesh->CamSpline.Add();
  key->Cam.x    = node->Para[0]>>1;
  key->Cam.y    = node->Para[1]>>1;
  key->Cam.z    = node->Para[2]>>1;
  key->Target.x = node->Para[3]>>1;
  key->Target.y = node->Para[4]>>1;
  key->Target.z = node->Para[5]>>1;
  key->Time     = node->Para[6]>>1;

  mesh->SortSpline();

  return mesh;
}


/****************************************************************************/
/****************************************************************************/


GenMobMesh *MobMesh_Material(GenNode *node)
{
  GenMobMesh *mesh;
  GenMobCluster *cl;
  GenMobFace *mf;
  sInt ci,sel;

  mesh = CalcMobMeshCopy(node->Inputs[0]);

  PA(L"mob_material");

  ci = mesh->Clusters.Count;
  cl = mesh->AddClusters(1);
  cl->Init();
  cl->Flags = node->Para[1]>>8;
  cl->TFlags[0] = node->Para[2];
  cl->TFlags[1] = node->Para[3];
#if !sPLAYER
  cl->TextureOp[0] = node->Inputs[1] ? node->Inputs[1]->Op : 0;
  cl->TextureOp[1] = node->Inputs[2] ? node->Inputs[2]->Op : 0;
#else
  cl->Image[0] = node->Inputs[1] ? node->Inputs[1]->Image : 0;
  cl->Image[1] = node->Inputs[2] ? node->Inputs[2]->Image : 0;
#endif
  
  sel = node->Para[0]&3;
  sFORALL(mesh->Faces,mf)
  {
    if(mf->Cluster!=0 && (sel==MMU_ALL || ((sel==MMU_SELECTED) && mf->Select) || ((sel==MMU_UNSELECTED) && !mf->Select)))
    {
      if(!(node->Para[0]&4) || mf->Cluster==1)
        mf->Cluster = ci;
    }
  }

  return mesh;
}

GenMobMesh *MobMesh_Add(GenNode *node)
{
  GenMobMesh *mesh,*m0;

  mesh = CalcMobMeshCopy(node->Inputs[0]);
  for(sInt i=1;i<8 && node->Inputs[i];i++)
  {
    m0 = CalcMobMesh(node->Inputs[i]);
    {
      PA(L"mob_add");

      mesh->Add(m0,1);
      m0->Release();
    }
  }

  return mesh;
}

GenMobMesh *MobMesh_SelectAll(GenNode *node)
{
  GenMobMesh *mesh;
  sInt flags = node->Para[0];

  mesh = CalcMobMeshCopy(node->Inputs[0]);

  PA(L"mob_selectall");

  if(flags&2)
    mesh->SetAllVerts(flags&1);
  if(flags&4)
    mesh->SetAllFaces(flags&1);

  return mesh;
}


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

GenMobMesh *MobMesh_SelectCube(GenNode *node)
{
  GenMobMesh *mesh;
  GenMobFace *mf;
  sIVector3 size;
  sIVector3 center;
  sInt i;

  mesh = CalcMobMeshCopy(node->Inputs[0]);

  PA(L"mob_selectcube");

  sInt mode = node->Para[0];
  center.x = node->Para[1];
  center.y = node->Para[2];
  center.z = node->Para[3];
  size.x = node->Para[4];
  size.y = node->Para[5];
  size.z = node->Para[6];

  for(i=0;i<mesh->Vertices.Count;i++)
  {
    sBool a = sAbs(mesh->Vertices[i].Pos.x-center.x)<=size.x/2;
    sBool b = sAbs(mesh->Vertices[i].Pos.y-center.y)<=size.y/2; 
    sBool c = sAbs(mesh->Vertices[i].Pos.z-center.z)<=size.z/2;
    mesh->Vertices[i].TempByte = (a && b && c);
  }
  switch(mode&0x0c)
  {
  case MMS_VERTEX:
    for(i=0;i<mesh->Vertices.Count;i++)
      mesh->Vertices[i].Select = SelectLogic(mesh->Vertices[i].Select,mesh->Vertices[i].TempByte,mode);
    break;
  case MMS_FULLFACE:

    for(i=0;i<mesh->Faces.Count;i++)
    {
      mf = &mesh->Faces[i];
      sBool ok = 1;
      for(sInt j=0;j<mf->Count;j++)
        if(!mesh->Vertices[mf->Vertex[j].Index].TempByte)
          ok = 0;
      mf->Select = SelectLogic(mf->Select,ok,mode);
    }
    break;
  case MMS_PARTFACE:
    for(i=0;i<mesh->Faces.Count;i++)
    {
      mf = &mesh->Faces[i];
      sBool ok = 0;
      for(sInt j=0;j<mf->Count;j++)
        if(mesh->Vertices[mf->Vertex[j].Index].TempByte)
          ok = 1;
      mf->Select = SelectLogic(mf->Select,ok,mode);
    }
    break;
  }
  return mesh;
}

GenMobMesh *MobMesh_DeleteFaces(GenNode *node)
{
  GenMobMesh *mesh;
  GenMobFace *mf;

  mesh = CalcMobMeshCopy(node->Inputs[0]);

  PA(L"mob_deletefaces");

  sFORALL(mesh->Faces,mf)
  {
    if(mf->Select)
      mf->Cluster = 0;
  }

  return mesh;
}

GenMobMesh *MobMesh_Invert(GenNode *node)
{
  GenMobMesh *mesh;
  GenMobFace *mf;

  sInt flags = node->Para[0];
  sInt select = flags & 3;

  mesh = CalcMobMeshCopy(node->Inputs[0]);

  sFORALL(mesh->Faces,mf)
  {
    if(select==0 || (select==1 && mf->Select) || (select==2 && !mf->Select))
    {
      sSwap(mf->Vertex[1],mf->Vertex[2]);
      if(mf->Count==4)
        sSwap(mf->Vertex[0],mf->Vertex[3]);
    }
  }

  return mesh;
}

/****************************************************************************/

GenMobMesh *MobMesh_TransformEx(GenNode *node)
{
  GenMobMesh *mesh;
  sIMatrix34 mat;

  sInt flags = node->Para[0];
  sISRT srt = *((sISRT*)&node->Para[1]);
  srt.Fix14();
  mat.InitSRT(srt);

  mesh = CalcMobMeshCopy(node->Inputs[0]);

  PA(L"mob_transformex");

  sInt sel = flags & 3;
  sInt src = (flags>>2) & 7;
  sInt dest = (flags>>5) & 7;

  mesh->Transform(sel,mat,src,dest);

  return mesh;
}

GenMobMesh *MobMesh_Displace(GenNode *node)
{
  GenMobMesh *mesh;
//  Image *img;

  mesh = CalcMobMeshCopy(node->Inputs[0]);
/*
#if !sPLAYER
  GenOp *op = node->Inputs[1] ? node->Inputs[1]->Op : 0;
  image = 0;
  if(op)
  {
    GenBitmap *bm = CalcBitmap(op,256,256,GenBitmapTile::Pixel8C::Variant);
  }

#else
  image = node->Inputs[1] ? node->Inputs[1]->Image : 0;
#endif
*/
  return mesh;
}

GenMobMesh *MobMesh_Perlin(GenNode *node)
{
  GenMobMesh *mesh;

  mesh = CalcMobMeshCopy(node->Inputs[0]);

  return mesh;
}

GenMobMesh *MobMesh_ExtrudeNormal(GenNode *node)
{
  GenMobMesh *mesh;
  GenMobVertex *mv;

  sInt dist = node->Para[0]>>1;

  mesh = CalcMobMeshCopy(node->Inputs[0]);

  PA(L"mob_extrudenormals");

  mesh->CalcNormals();
  sFORALL(mesh->Vertices,mv)
  {
    mv->Pos.AddScale(mv->Normal,dist);
  }

  return mesh;
}

GenMobMesh *MobMesh_Bend2(GenNode *node)
{
  GenMobMesh *mesh;
  GenMobVertex *vert;
  sIMatrix34 mt,mb;
  sIVector3 vt;
  sInt vx,vy,t,sa,ca;
  sIVector3 center,rotate;

  mesh = CalcMobMeshCopy(node->Inputs[0]);

  PA(L"mob_bend2");

  center.x = node->Para[0]>>1;
  center.y = node->Para[1]>>1;
  center.z = node->Para[2]>>1;
  rotate.x = node->Para[3]>>1;
  rotate.y = node->Para[4]>>1;
  rotate.z = node->Para[5]>>1;
  sInt len = node->Para[6]>>1;
  sInt angle = node->Para[7]>>1;

  mt.InitEuler(rotate.x,rotate.y,rotate.z);
  mt.l.x = -center.x;
  mt.l.y = -center.y;
  mt.l.z = -center.z;
  mb = mt;
  mb.TransR();

  sFORALL(mesh->Vertices,vert)
  {
    vt = vert->Pos;
    vt.Rotate34(mt);
    t = vt.y;

    if(t<0)
    {
      t = 0;
    }
    else if(t>=len)
    {
      vt.y -= len;
      t = angle;
    }
    else
    {
      vt.y = 0;
      t = sMulDiv(t,angle,len);
    }

    sa = sISin(t);
    ca = sICos(t);
    vx = vt.x;
    vy = vt.y;
    vt.x = sIMul(ca,vx) - sIMul(sa,vy);
    vt.y = sIMul(sa,vx) + sIMul(ca,vy);

    vt.Rotate34(mb);
    vert->Pos = vt;
  }

  return mesh;
}

GenMobMesh *MobMesh_Bend(GenNode *node)
{
  GenMobMesh *mesh;
  GenMobVertex *vert;
  sIMatrix34 mt,mb;
  sIVector3 vt;
  sInt vx,vy,t,sa,ca;
  sIVector3 center,rotate;

  mesh = CalcMobMeshCopy(node->Inputs[0]);

  PA(L"mob_bend");

  center.x = node->Para[0]>>1;
  center.y = node->Para[1]>>1;
  center.z = node->Para[2]>>1;
  rotate.x = node->Para[3]>>1;
  rotate.y = node->Para[4]>>1;
  rotate.z = node->Para[5]>>1;
  sInt len = node->Para[6]>>1;
  sInt angle = node->Para[7]>>1;

  mt.InitEuler(rotate.x,rotate.y,rotate.z);
  center.Rotate34(mt);
  mt.l.x = -center.x;
  mt.l.y = -center.y;
  mt.l.z = -center.z;
  mb = mt;
  mb.TransR();

  sFORALL(mesh->Vertices,vert)
  {
    vt = vert->Pos;
    vt.Rotate34(mt);
    t = vt.y;

    if(t<0)
    {
      t = 0;
    }
    else if(t>=len)
    {
      vt.y -= len;
      t = angle;
    }
    else
    {
      vt.y = 0;
      t = sMulDiv(t,angle,len);
    }

    sa = sISin(t);
    ca = sICos(t);
    vx = vt.x;
    vy = vt.y;
    vt.x = sIMul(ca,vx) - sIMul(sa,vy);
    vt.y = sIMul(sa,vx) + sIMul(ca,vy);

    vt.Rotate34(mb);
    vert->Pos = vt;
  }

  return mesh;
}

GenMobMesh *MobMesh_Multiply(GenNode *node)
{
  GenMobMesh *org,*mesh;
  sIMatrix34 mat,matr,mat0;

  org = CalcMobMesh(node->Inputs[0]);

  PA(L"mob_multiply");

  sInt count = node->Para[0];
  sISRT srt = *((sISRT*)&node->Para[1]);
  srt.Fix14();
  mat.Init();
  matr.InitSRT(srt);

  mesh = new GenMobMesh();
  for(sInt i=0;i<count;i++)
  {
    mesh->Add(org,i!=0);
    mesh->Transform(MMU_SELECTED,mat,0,0);
    mesh->SelectAll(MMU_ALL,MMS_SETNOT);

    mat0.Mul(mat,matr);
    mat = mat0;
  }

  org->Release();

  return mesh;
}

GenMobMesh *MobMesh_Light(GenNode *node)
{
  GenMobMesh *mesh;
  GenMobVertex *mv;
  GenMobFace *mf;
  GenMobSample *ms;
  sIVector3 pos,dir,norm;
  sIVector3 color;
  sInt range,flags;

  pos.x = node->Para[0]>>1;
  pos.y = node->Para[1]>>1;
  pos.z = node->Para[2]>>1;
  range = node->Para[3]>>1;// range = 0x10000000/sRange<sInt>(range,0x01000000,0x100);
  color.x = (((node->Para[5]>> 0)&0xff)*node->Para[4])>>15;
  color.y = (((node->Para[5]>> 8)&0xff)*node->Para[4])>>15;
  color.z = (((node->Para[5]>>16)&0xff)*node->Para[4])>>15;
  flags = node->Para[6];


  mesh = CalcMobMeshCopy(node->Inputs[0]);

  PA(L"mob_light");

  mesh->CalcNormals();
  sFORALL(mesh->Faces,mf)
  {
    for(sInt i=0;i<mf->Count;i++)
    {
      ms = &mf->Vertex[i];
      mv = &mesh->Vertices[ms->Index];

      if((flags & 1)==0)
        norm.Init(mf->Normal.x,mf->Normal.y,mf->Normal.z);
      else
        norm = mv->Normal;
      dir.Sub(pos,mv->Pos);
      sInt l = sIMul(dir.x,norm.x) + sIMul(dir.y,norm.y) + sIMul(dir.z,norm.z);
      if(l>16)
      {
        sInt d = sIMul(dir.x,dir.x) + sIMul(dir.y,dir.y) + sIMul(dir.z,dir.z);
        if(flags & 2)
        {
          d = sIntSqrt15D(d)<<8;
          if(d<range)
          {
            l = sMulDiv(range-d,l,range);
          }
          else
          {
            l = 0;
          }
        }
        else
        {
          l = sDivShift(l,d);
        }
        
        ms->Color.x = sMin<sInt>(0x7fff,ms->Color.x+((color.x*l)>>sFIXSHIFT));
        ms->Color.y = sMin<sInt>(0x7fff,ms->Color.y+((color.y*l)>>sFIXSHIFT));
        ms->Color.z = sMin<sInt>(0x7fff,ms->Color.z+((color.z*l)>>sFIXSHIFT));
      }
    }
  }

  return mesh;
}


GenMobMesh *MobMesh_LightMix(GenNode *node)
{
  GenMobMesh *mesh;
  GenMobFace *mf;
  GenMobSample *ms;
  sIVector3 col,amb;

  col.x = (((node->Para[1]>> 0)&0xff)*node->Para[2])>>9;
  col.y = (((node->Para[1]>> 8)&0xff)*node->Para[2])>>9;
  col.z = (((node->Para[1]>>16)&0xff)*node->Para[2])>>9;
  amb.x = ((node->Para[0]>> 0)&0xff);
  amb.y = ((node->Para[0]>> 8)&0xff);
  amb.z = ((node->Para[0]>>16)&0xff);

  mesh = CalcMobMeshCopy(node->Inputs[0]);

  PA(L"mob_lightmix");

  sFORALL(mesh->Faces,mf)
  {
    for(sInt i=0;i<mf->Count;i++)
    {
      ms = &mf->Vertex[i];

      ms->Color.x = sIntSqrt15D( (((ms->Color.x*col.x)>>14)+amb.x)<<7 );
      ms->Color.y = sIntSqrt15D( (((ms->Color.y*col.y)>>14)+amb.y)<<7 );
      ms->Color.z = sIntSqrt15D( (((ms->Color.z*col.z)>>14)+amb.z)<<7 );
    }
  }

  return mesh;
}

/****************************************************************************/
/***                                                                      ***/
/***   User Interface                                                     ***/
/***                                                                      ***/
/****************************************************************************/

#if !sMOBILE

class Class_MobMesh_Load : public GenClass
{
public:
  Class_MobMesh_Load()
  {
    Name = "load";
    ClassId = GCI_MESH_LOAD;
    Flags = GCF_LOAD;
    Column = GCC_LINK;
    Shortcut = 'l';
    Output = Doc->FindType(GTI_MESH);

    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_Load);
    Para.Add(GenPara::MakeLink(1,"load",Output,GPF_DEFAULT|GPF_LINK_REQUIRED));
  }
};

class Class_MobMesh_Store : public GenClass
{
public:
  Class_MobMesh_Store()
  {
    Name = "store";
    ClassId = GCI_MESH_STORE;
    Flags = GCF_STORE;
    Column = GCC_LINK;
    Shortcut = 's';
    Output = Doc->FindType(GTI_MESH);

    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_Nop);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
  }
};

class Class_MobMesh_Nop : public GenClass
{
public:
  Class_MobMesh_Nop()
  {
    Name = "nop";
    ClassId = GCI_MESH_NOP;
    Flags = GCF_NOP;
    Column = GCC_LINK;
    Output = Doc->FindType(GTI_MESH);

    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_Nop);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
  }
};


/****************************************************************************/

class Class_MobMesh_Grid : public GenClass
{
public:
  Class_MobMesh_Grid()
  {
    Name = "grid";
    ClassId = GCI_MESH_GRID;
    Column = GCC_GENERATOR;
    Shortcut = 'g';
    Output = Doc->FindType(GTI_MESH);

    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_Grid);
    Para.Add(GenPara::MakeFlags(1,"Modes","1sided|2sided",0));
    Para.Add(GenPara::MakeIntV(2,"Tesselate",2,64,1,1,0.25f));
  }
};

class Class_MobMesh_Cube : public GenClass
{
public:
  Class_MobMesh_Cube()
  {
    Name = "cube";
    ClassId = GCI_MESH_CUBE;
    Column = GCC_GENERATOR;
    Shortcut = 'q';
    Output = Doc->FindType(GTI_MESH);

    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_Cube);
    Para.Add(GenPara::MakeIntV(1,"Tesselate",3,64,1,1,0.25f));
    Para.Add(GenPara::MakeFlags(2,"Flags","*1|wrap uv:*2center|bottom:*3|scale uv",0));
    Para.Add(GenPara::MakeFloatV(3,"Scale",3,128,-128,1,0.01f));
    Para.Add(GenPara::MakeFloatV(4,"Rotate",3,16,-16,0,0.001f));
    Para.Add(GenPara::MakeFloatV(5,"Translate",3,128,-128,0,0.01f));
  }
};

class Class_MobMesh_Cylinder : public GenClass
{
public:
  Class_MobMesh_Cylinder()
  {
    Name = "cylinder";
    ClassId = GCI_MESH_CYLINDER;
    Column = GCC_GENERATOR;
    Shortcut = 'z';
    Output = Doc->FindType(GTI_MESH);

    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_Cylinder);

    Para.Add(GenPara::MakeIntV(1,"Facets",1,64,3,8,0.25f));
    Para.Add(GenPara::MakeIntV(2,"Slices",1,64,1,1,0.25f));
    Para.Add(GenPara::MakeIntV(3,"Rings",1,64,1,1,0.25f));
    Para.Add(GenPara::MakeFlags(4,"Mode","closed|open:*1center|bottom",0));
  }
};


class Class_MobMesh_Torus : public GenClass
{
public:
  Class_MobMesh_Torus()
  {
    Name = "torus";
    ClassId = GCI_MESH_TORUS;
    Column = GCC_GENERATOR;
    Shortcut = 'o';
    Output = Doc->FindType(GTI_MESH);

    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_Torus);
    Para.Add(GenPara::MakeIntV(1,"Facets",1,64,3,8,0.25f));
    Para.Add(GenPara::MakeIntV(2,"Slices",1,64,3,4,0.25f));
    Para.Add(GenPara::MakeFloatV(3,"Outer",1,128,-128,0.5f,0.002f));
    Para.Add(GenPara::MakeFloatV(4,"Inner",1,128,-128,0.125f,0.002f));
    Para.Add(GenPara::MakeFloatV(5,"Phase",1,128,-128,0.5f,0.05f));
    Para.Add(GenPara::MakeFloatV(6,"Arclen",1,128,-128,1,0.002f));
    Para.Add(GenPara::MakeFlags(7,"Mode","relative|absolute:*1center|bottom",0));
  }
};


class Class_MobMesh_Sphere : public GenClass
{
public:
  Class_MobMesh_Sphere()
  {
    Name = "sphere";
    ClassId = GCI_MESH_SPHERE;
    Column = GCC_GENERATOR;
    Shortcut = 'h';
    Output = Doc->FindType(GTI_MESH);

    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_Sphere);

    Para.Add(GenPara::MakeIntV(1,"Facets",1,64,3,8,0.25f));
    Para.Add(GenPara::MakeIntV(2,"Slices",1,64,1,4,0.25f));
  }
};

class Class_MobMesh_CamKey : public GenClass
{
public:
  Class_MobMesh_CamKey()
  {
    Name = "CamKey";
    ClassId = GCI_MESH_CAMKEY;
    Column = GCC_GENERATOR;
    Shortcut = 'k';
    Output = Doc->FindType(GTI_MESH);
    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_CamKey );

    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Para.Add(GenPara::MakeFloatV(2,"Position",3,128,-128,0,0.01f));
    Para.Add(GenPara::MakeFloatV(3,"Target",3,128,-128,0,0.01f));
    Para.Add(GenPara::MakeFloatV(4,"Time",1,256,0,0,0.25f));

    NOTINDEMO;
  }
};

/****************************************************************************/

class Class_MobMesh_Material : public GenClass
{
public:
  Class_MobMesh_Material()
  {
    Name = "material";
    ClassId = GCI_MESH_MATERIAL;
    Column = GCC_SPECIAL;
    Shortcut = 'm';
    Output = Doc->FindType(GTI_MESH);

    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Inputs.Add(GenInput::Make(2,Doc->FindType(GTI_BITMAP),0,GIF_DONTFOLLOW));
    Inputs.Add(GenInput::Make(3,Doc->FindType(GTI_BITMAP),0,GIF_DONTFOLLOW));
    
    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_Material);
    Para.Add(GenPara::MakeFlags(1,"Select","all|selected|unselected:*2|only new",0));
    Para.Add(GenPara::MakeFlags(2,"Flags","*8Gouraud|Textured|BlendAdd|Tex*Color|Tex+Tex|Tex*Tex:*12|Doublesided",0x100));
    Para.Add(GenPara::MakeFlags(3,"","",0));
    Para.Add(GenPara::MakeFlags(5,"","",0));

    NOTINDEMO;
  }
};

class Class_MobMesh_Add : public GenClass
{
public:
  Class_MobMesh_Add()
  {
    Name = "Add";
    ClassId = GCI_MESH_ADD;
    Column = GCC_ADD;
    Shortcut = 'a';
    Output = Doc->FindType(GTI_MESH);
    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_Add);

    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Inputs.Add(GenInput::Make(2,Output,0,0));
    Inputs.Add(GenInput::Make(3,Output,0,0));
    Inputs.Add(GenInput::Make(4,Output,0,0));
    Inputs.Add(GenInput::Make(5,Output,0,0));
    Inputs.Add(GenInput::Make(6,Output,0,0));
    Inputs.Add(GenInput::Make(7,Output,0,0));
    Inputs.Add(GenInput::Make(8,Output,0,0));
  }
};

class Class_MobMesh_SelectAll : public GenClass
{
public:
  Class_MobMesh_SelectAll()
  {
    Name = "selectall";
    ClassId = GTI_MESH_SELECTALL;
    Column = GCC_FILTER;
    Shortcut = 'A'|sKEYQ_SHIFT;
    Output = Doc->FindType(GTI_MESH);
    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_SelectAll);

    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Para.Add(GenPara::MakeFlags(2,"Flags","clear|set:*1|vertices:*2|faces",0));

    NOTINDEMO;
  }
};

class Class_MobMesh_SelectCube : public GenClass
{
public:
  Class_MobMesh_SelectCube()
  {
    Name = "selectcube";
    ClassId = GCI_MESH_SELECTCUBE;
    Column = GCC_FILTER;
    Shortcut = 'Q'|sKEYQ_SHIFT;
    Output = Doc->FindType(GTI_MESH);
    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_SelectCube);

    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Para.Add(GenPara::MakeFlags(2,"Flags","add|sub|set|set not:*2|face->vertex",0));
    Para.Add(GenPara::MakeFloatV(3,"Center",3,128,-128,0,0.01f));
    Para.Add(GenPara::MakeFloatV(4,"Size",3,128,-128,1.25f,0.01f));

    NOTINDEMO;
  }

  void DrawHints(GenOp *op,sInt handle)
  {
    sVertexCompact *vp;
    sU16 *ip;
    sU32 col=0xffffff00;
    sVector c,s;
    sIVector3 ci,si;

    sSystem->GeoBegin(handle,8,24,(sF32 **)&vp,(void **)&ip);
    op->FindValue(3)->StorePara(op,&ci.x);
    op->FindValue(4)->StorePara(op,&si.x);

    c.Init(ci.x*1.0f/sFIXONE,ci.y*1.0f/sFIXONE,ci.z*1.0f/sFIXONE);
    s.Init(si.x*0.5f/sFIXONE,si.y*0.5f/sFIXONE,si.z*0.5f/sFIXONE);

    vp[0].Init(c.x+s.x , c.y+s.y , c.z+s.z , col);
    vp[1].Init(c.x-s.x , c.y+s.y , c.z+s.z , col);
    vp[2].Init(c.x-s.x , c.y-s.y , c.z+s.z , col);
    vp[3].Init(c.x+s.x , c.y-s.y , c.z+s.z , col);
    vp[4].Init(c.x+s.x , c.y+s.y , c.z-s.z , col);
    vp[5].Init(c.x-s.x , c.y+s.y , c.z-s.z , col);
    vp[6].Init(c.x-s.x , c.y-s.y , c.z-s.z , col);
    vp[7].Init(c.x+s.x , c.y-s.y , c.z-s.z , col);

    *ip++ = 0; *ip++ = 1;
    *ip++ = 1; *ip++ = 2;
    *ip++ = 2; *ip++ = 3;
    *ip++ = 3; *ip++ = 0;
    *ip++ = 4; *ip++ = 5;
    *ip++ = 5; *ip++ = 6;
    *ip++ = 6; *ip++ = 7;
    *ip++ = 7; *ip++ = 4;
    *ip++ = 0; *ip++ = 4;
    *ip++ = 1; *ip++ = 5;
    *ip++ = 2; *ip++ = 6;
    *ip++ = 3; *ip++ = 7;

    sSystem->GeoEnd(handle);
    sSystem->GeoDraw(handle);
  }
};

class Class_MobMesh_DeleteFaces : public GenClass
{
public:
  Class_MobMesh_DeleteFaces()
  {
    Name = "deletefaces";
    ClassId = GCI_MESH_DELETEFACES;
    Column = GCC_FILTER;
    Shortcut = 'd';
    Output = Doc->FindType(GTI_MESH);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));

    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_DeleteFaces);

    NOTINDEMO;
  }
};

class Class_MobMesh_Invert : public GenClass
{
public:
  Class_MobMesh_Invert()
  {
    Name = "invert";
    ClassId = GCI_MESH_INVERT;
    Column = GCC_FILTER;
    Shortcut = 'i';
    Output = Doc->FindType(GTI_MESH);

    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_Invert);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Para.Add(GenPara::MakeFlags(2,"Select","all|selected|unselected",0));

    NOTINDEMO;
  }
};

/****************************************************************************/

class Class_MobMesh_TransformEx : public GenClass
{
public:
  Class_MobMesh_TransformEx()
  {
    Name = "Transform";
    ClassId = GCI_MESH_TRANSFORMEX;
    Column = GCC_FILTER;
    Shortcut = 't';
    Output = Doc->FindType(GTI_MESH);
    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_TransformEx);

    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Para.Add(GenPara::MakeFlags(2,"Flags","all|selected|unselected:*2pos->|uv0->|uv1->:*5->pos|->uv0|->uv1",0));
    Para.Add(GenPara::MakeFloatV(3,"Scale",3,128,-128,1,0.01f));
    Para.Add(GenPara::MakeFloatV(4,"Rotate",3,16,-16,0,0.001f));
    Para.Add(GenPara::MakeFloatV(5,"Translate",3,128,-128,0,0.01f));

    NOTINDEMO;
  }
};

class Class_MobMesh_Displace : public GenClass
{
public:
  Class_MobMesh_Displace()
  {
    Name = "Displace";
    ClassId = GCI_MESH_DISPLACE;
    Column = GCC_SPECIAL;
    Output = Doc->FindType(GTI_MESH);
    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_Displace);

    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Inputs.Add(GenInput::Make(1,Doc->FindType(GTI_BITMAP),0,GIF_DONTFOLLOW));
    Para.Add(GenPara::MakeFloatV(2,"Distance",1,128,-128,0,0.001f));

    NOTINDEMO;
  }
};

class Class_MobMesh_Perlin : public GenClass
{
public:
  Class_MobMesh_Perlin()
  {
    Name = "Perlin";
    ClassId = GCI_MESH_PERLIN;
    Column = GCC_FILTER;
    Output = Doc->FindType(GTI_MESH);
    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));

    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_Perlin);

    NOTINDEMO;
  }
};

class Class_MobMesh_ExtrudeNormal : public GenClass
{
public:
  Class_MobMesh_ExtrudeNormal()
  {
    Name = "ExtrudeNormal";
    ClassId = GCI_MESH_EXTRUDENORMAL;
    Column = GCC_FILTER;
    Output = Doc->FindType(GTI_MESH);
    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_ExtrudeNormal);

    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Para.Add(GenPara::MakeFloatV(2,"Distance",1,128,-128,0,0.001f));

    NOTINDEMO;
  }
};

class Class_MobMesh_Bend2 : public GenClass
{
public:
  Class_MobMesh_Bend2()
  {
    Name = "Bend2 (old)";
    ClassId = GCI_MESH_BEND2;
    Column = GCC_INVISIBLE;
    Shortcut = 0;
    Output = Doc->FindType(GTI_MESH);
    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_Bend2);

    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Para.Add(GenPara::MakeFloatV(2,"Center",3,128,-128,0,0.01f));
    Para.Add(GenPara::MakeFloatV(3,"Rotate",3,16,-16,0,0.001f));
    Para.Add(GenPara::MakeFloatV(4,"Length",1,128,-128,1,0.01f));
    Para.Add(GenPara::MakeFloatV(5,"Angle",1,16,-16,0.25,0.001f));

    NOTINDEMO;
  }

  void DrawHints(GenOp *op,sInt handle)
  {
    sVertexCompact *vp;
    sU16 *ip;
    sU32 col=0xffffff00;
    sVector c,r,h,s,o;
    sIVector3 ci,ri;
    sMatrix mat;
    sInt len;

    sSystem->GeoBegin(handle,10,10,(sF32 **)&vp,(void **)&ip);
    op->FindValue(2)->StorePara(op,&ci.x);
    op->FindValue(3)->StorePara(op,&ri.x);
    op->FindValue(4)->StorePara(op,&len);

    c.Init(ci.x* 1.0f/0x8000,ci.y* 1.0f/0x8000,ci.z* 1.0f/0x8000);
    r.Init(ri.x*sPI2F/0x8000,ri.y*sPI2F/0x8000,ri.z*sPI2F/0x8000);
    mat.InitEuler(r.x,r.y,r.z);

    h.x = c.x + mat.i.y * len / 0x8000;
    h.y = c.y + mat.j.y * len / 0x8000;
    h.z = c.z + mat.k.y * len / 0x8000;

    o.x = c.x + mat.i.z * 1024;
    o.y = c.y + mat.j.z * 1024;
    o.z = c.z + mat.k.z * 1024;

    s.Init(0.125f,0.125f,0.125f);

    vp[0].Init(c.x+s.x , c.y     , c.z     , col);
    vp[1].Init(c.x-s.x , c.y     , c.z     , col);
    vp[2].Init(c.x     , c.y+s.y , c.z     , col);
    vp[3].Init(c.x     , c.y-s.y , c.z     , col);
    vp[4].Init(c.x     , c.y     , c.z+s.z , col);
    vp[5].Init(c.x     , c.y     , c.z-s.z , col);

    vp[6].Init(c.x     , c.y     , c.z     , col);
    vp[7].Init(h.x     , h.y     , h.z     , col);

    vp[8].Init(c.x+o.x , c.y+o.y , c.z+o.z , col);
    vp[9].Init(c.x-o.x , c.y-o.y , c.z-o.z , col);

    *ip++ = 0; *ip++ = 1;
    *ip++ = 2; *ip++ = 3;
    *ip++ = 4; *ip++ = 5;
    *ip++ = 6; *ip++ = 7;
    *ip++ = 8; *ip++ = 9;

    sSystem->GeoEnd(handle);
    sSystem->GeoDraw(handle);
  }
};

class Class_MobMesh_Bend : public GenClass
{
public:
  Class_MobMesh_Bend()
  {
    Name = "Bend";
    ClassId = GCI_MESH_BEND;
    Column = GCC_FILTER;
    Shortcut = 'b';
    Output = Doc->FindType(GTI_MESH);
    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_Bend);

    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Para.Add(GenPara::MakeFloatV(2,"Center",3,128,-128,0,0.01f));
    Para.Add(GenPara::MakeFloatV(3,"Rotate",3,16,-16,0,0.001f));
    Para.Add(GenPara::MakeFloatV(4,"Length",1,128,-128,1,0.01f));
    Para.Add(GenPara::MakeFloatV(5,"Angle",1,16,-16,0.25,0.001f));

    NOTINDEMO;
  }

  void DrawHints(GenOp *op,sInt handle)
  {
    sVertexCompact *vp;
    sU16 *ip;
    sU32 col=0xffffff00;
    sVector c,r,h,s,o;
    sIVector3 ci,ri;
    sMatrix mat;
    sInt len;

    sSystem->GeoBegin(handle,10,10,(sF32 **)&vp,(void **)&ip);
    op->FindValue(2)->StorePara(op,&ci.x);
    op->FindValue(3)->StorePara(op,&ri.x);
    op->FindValue(4)->StorePara(op,&len);

    c.Init(ci.x* 1.0f/0x8000,ci.y* 1.0f/0x8000,ci.z* 1.0f/0x8000);
    r.Init(ri.x*sPI2F/0x8000,ri.y*sPI2F/0x8000,ri.z*sPI2F/0x8000);
    mat.InitEuler(r.x,r.y,r.z);

    h.x = c.x + mat.i.y * len / 0x8000;
    h.y = c.y + mat.j.y * len / 0x8000;
    h.z = c.z + mat.k.y * len / 0x8000;

    o.x = c.x + mat.i.z * 1024;
    o.y = c.y + mat.j.z * 1024;
    o.z = c.z + mat.k.z * 1024;

    s.Init(0.125f,0.125f,0.125f);

    vp[0].Init(c.x+s.x , c.y     , c.z     , col);
    vp[1].Init(c.x-s.x , c.y     , c.z     , col);
    vp[2].Init(c.x     , c.y+s.y , c.z     , col);
    vp[3].Init(c.x     , c.y-s.y , c.z     , col);
    vp[4].Init(c.x     , c.y     , c.z+s.z , col);
    vp[5].Init(c.x     , c.y     , c.z-s.z , col);

    vp[6].Init(c.x     , c.y     , c.z     , col);
    vp[7].Init(h.x     , h.y     , h.z     , col);

    vp[8].Init(c.x+o.x , c.y+o.y , c.z+o.z , col);
    vp[9].Init(c.x-o.x , c.y-o.y , c.z-o.z , col);

    *ip++ = 0; *ip++ = 1;
    *ip++ = 2; *ip++ = 3;
    *ip++ = 4; *ip++ = 5;
    *ip++ = 6; *ip++ = 7;
    *ip++ = 8; *ip++ = 9;

    sSystem->GeoEnd(handle);
    sSystem->GeoDraw(handle);
  }
};


class Class_MobMesh_Multiply : public GenClass
{
public:
  Class_MobMesh_Multiply()
  {
    Name = "Multiply";
    ClassId = GCI_MESH_MULTIPLY;
    Column = GCC_FILTER;
    Shortcut = 'M'|sKEYQ_SHIFT;
    Output = Doc->FindType(GTI_MESH);
    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_Multiply);

    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Para.Add(GenPara::MakeInt(2,"Count",256,1,2,0.25f));
    Para.Add(GenPara::MakeFloatV(3,"Scale",3,128,-128,1,0.01f));
    Para.Add(GenPara::MakeFloatV(4,"Rotate",3,16,-16,0,0.001f));
    Para.Add(GenPara::MakeFloatV(5,"Translate",3,128,-128,0,0.01f));

    NOTINDEMO;
  }
};

class Class_MobMesh_Light : public GenClass
{
public:
  Class_MobMesh_Light()
  {
    Name = "Light";
    ClassId = GCI_MESH_LIGHT;
    Column = GCC_FILTER;
    Shortcut = 'L'|sKEYQ_SHIFT;
    Output = Doc->FindType(GTI_MESH);
    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_Light);

    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Para.Add(GenPara::MakeFloatV(2,"Position",3,128,-128,0,0.01f));
    Para.Add(GenPara::MakeFloatV(3,"Range",1,128,0,1,0.01f));
    Para.Add(GenPara::MakeFloatV(4,"Brightness",1,128,0,1,0.001f));
    Para.Add(GenPara::MakeColor(5,"Color 0",0xffffffff));
    Para.Add(GenPara::MakeFlags(6,"Flags","vertex|face:*1|Range",0));

    NOTINDEMO;
  }

  void DrawHints(GenOp *op,sInt handle)
  {
    sVertexCompact *vp;
    sU16 *ip;
    sU32 col=0xffffff00;
    sVector c,s;
    sIVector3 ci;

    sSystem->GeoBegin(handle,6,6,(sF32 **)&vp,(void **)&ip);
    op->FindValue(2)->StorePara(op,&ci.x);

    c.Init(ci.x* 1.0f/0x8000,ci.y* 1.0f/0x8000,ci.z* 1.0f/0x8000);

    s.Init(0.125f,0.125f,0.125f);
    vp[0].Init(c.x+s.x , c.y     , c.z     , col);
    vp[1].Init(c.x-s.x , c.y     , c.z     , col);
    vp[2].Init(c.x     , c.y+s.y , c.z     , col);
    vp[3].Init(c.x     , c.y-s.y , c.z     , col);
    vp[4].Init(c.x     , c.y     , c.z+s.z , col);
    vp[5].Init(c.x     , c.y     , c.z-s.z , col);

    *ip++ = 0; *ip++ = 1;
    *ip++ = 2; *ip++ = 3;
    *ip++ = 4; *ip++ = 5;

    sSystem->GeoEnd(handle);
    sSystem->GeoDraw(handle);
  }
};

class Class_MobMesh_LightMix : public GenClass
{
public:
  Class_MobMesh_LightMix()
  {
    Name = "LightMix";
    ClassId = GCI_MESH_LIGHTMIX;
    Column = GCC_FILTER;
    Shortcut = 0;
    Output = Doc->FindType(GTI_MESH);
    Handlers.Set(GVI_MESH_MOBMESH,MobMesh_LightMix);

    Inputs.Add(GenInput::Make(1,Output,0,GIF_REQUIRED));
    Para.Add(GenPara::MakeColor(2,"Ambient",0xff404040));
    Para.Add(GenPara::MakeColor(3,"Light",0xffc0c0c0));
    Para.Add(GenPara::MakeFloatV(4,"Brightness",1,16,0,1,0.001f));

    NOTINDEMO;
  }
};

#endif

/****************************************************************************/
/****************************************************************************/

#if !sMOBILE

void AddMobMeshClasses(GenDoc *doc)
{
  doc->Classes.Add(new Class_MobMesh_Load);
  doc->Classes.Add(new Class_MobMesh_Store);
  doc->Classes.Add(new Class_MobMesh_Nop);

  doc->Classes.Add(new Class_MobMesh_Cube);
  doc->Classes.Add(new Class_MobMesh_Grid);
  doc->Classes.Add(new Class_MobMesh_Cylinder);
  doc->Classes.Add(new Class_MobMesh_Torus);
  doc->Classes.Add(new Class_MobMesh_Sphere);

  doc->Classes.Add(new Class_MobMesh_CamKey);

  doc->Classes.Add(new Class_MobMesh_Material);
  doc->Classes.Add(new Class_MobMesh_Add);
  doc->Classes.Add(new Class_MobMesh_SelectAll);
  doc->Classes.Add(new Class_MobMesh_SelectCube);
  doc->Classes.Add(new Class_MobMesh_DeleteFaces);
  doc->Classes.Add(new Class_MobMesh_Invert);

  doc->Classes.Add(new Class_MobMesh_TransformEx);
//  doc->Classes.Add(new Class_MobMesh_Displace);
//  doc->Classes.Add(new Class_MobMesh_Perlin);
  doc->Classes.Add(new Class_MobMesh_ExtrudeNormal);
  doc->Classes.Add(new Class_MobMesh_Bend2);
  doc->Classes.Add(new Class_MobMesh_Bend);
  doc->Classes.Add(new Class_MobMesh_Multiply);
  doc->Classes.Add(new Class_MobMesh_Light);
  doc->Classes.Add(new Class_MobMesh_LightMix);
}
#endif

GenHandlerArray GenMobMeshHandlers[] =
{
  { MobMesh_Cube          ,0x2010 },
  { MobMesh_Grid          ,0x2020 },
  { MobMesh_Cylinder      ,0x2030 },
  { MobMesh_Torus         ,0x2040 },
  { MobMesh_Sphere        ,0x2050 },

  { MobMesh_CamKey        ,0x20f0 },

  { MobMesh_Material      ,0x2110 },
  { MobMesh_Add           ,0x2120 },
  { MobMesh_SelectAll     ,0x2130 },
  { MobMesh_SelectCube    ,0x2140 },
  { MobMesh_DeleteFaces   ,0x2150 },
  { MobMesh_Invert        ,0x2160 },

  { MobMesh_TransformEx   ,0x2210 },
  { MobMesh_Displace      ,0x2220 },
  { MobMesh_Perlin        ,0x2230 },
  { MobMesh_ExtrudeNormal ,0x2240 },
  { MobMesh_Bend2         ,0x2250 },
  { MobMesh_Multiply      ,0x2280 },
  { MobMesh_Light         ,0x2290 },
  { MobMesh_LightMix      ,0x22a0 },
  { MobMesh_Bend          ,0x22b0 },

  { 0,0 }
};


/****************************************************************************/



/****************************************************************************/

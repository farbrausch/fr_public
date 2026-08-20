/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4frlib/wz4_extra_mesh.hpp"

namespace Wz4Extra
{

/****************************************************************************/
/***                                                                      ***/
/***   building blocks                                                    ***/
/***                                                                      ***/
/****************************************************************************/

// Every generator here lays its vertices out as a (nu x nv) grid and hands it
// to this. Seams are expected to be duplicated -- a closed ring emits its
// first column again as its last -- because that is what gives the seam its
// own texture coordinates.

static void AddGridFaces(Wz4Mesh *out,sInt base,sInt nu,sInt nv)
{
  Wz4MeshFace *mf = out->Faces.AddMany((nu-1)*(nv-1));
  for(sInt v=0;v<nv-1;v++)
  {
    for(sInt u=0;u<nu-1;u++)
    {
      mf->Init(4);
      mf->Vertex[0] = base + (u+0) + (v+0)*nu;
      mf->Vertex[1] = base + (u+0) + (v+1)*nu;
      mf->Vertex[2] = base + (u+1) + (v+1)*nu;
      mf->Vertex[3] = base + (u+1) + (v+0)*nu;
      mf++;
    }
  }
}

// A tube around a curve. The frame is built from a reference direction given
// per sample rather than carried along the curve: carrying it accumulates a
// twist that does not come back to zero when the curve closes, and the
// reference here (pointing away from the axis the curve winds around) is
// periodic by construction.

static void BuildTube(Wz4Mesh *out,const sVector31 *pos,const sVector30 *ref,
                      const sF32 *radius,sInt count,sInt slices,sBool closed,sBool caps)
{
  sInt nu = slices+1;                   // +1 for the duplicated seam
  sInt nv = count;
  sInt base = out->Vertices.GetCount();

  Wz4MeshVertex *mv = out->Vertices.AddMany(nu*nv);

  for(sInt i=0;i<nv;i++)
  {
    // tangent by central difference, wrapping when the curve is closed
    sInt ia = i>0       ? i-1 : (closed ? nv-2 : 0);
    sInt ib = i<nv-1    ? i+1 : (closed ? 1    : nv-1);
    sVector30 t = pos[ib]-pos[ia];
    if(t.LengthSq()<0.0000001f)
      t.Init(0,1,0);
    t.Unit();

    sVector30 n = ref[i] - t*(ref[i]^t);
    if(n.LengthSq()<0.0000001f)
    {
      // reference parallel to the curve: any perpendicular will do
      n = t.Cross(sVector30(0,1,0));
      if(n.LengthSq()<0.0000001f)
        n = t.Cross(sVector30(1,0,0));
    }
    n.Unit();
    sVector30 b = t.Cross(n);

    for(sInt j=0;j<nu;j++)
    {
      // j%slices, not j: the duplicated seam vertex has to land on exactly the
      // same position as the one it duplicates, or the mesh counts as open
      // there and everything that walks edges -- smoothing, shelling -- sees a
      // hole that is not really there
      sF32 a = sF32(j%slices)/slices*sPI2F;
      sVector30 o = n*sFCos(a) + b*sFSin(a);
      mv->Init(pos[i]+o*radius[i],sF32(j)/slices,sF32(i)/(nv-1));
      mv++;
    }
  }

  AddGridFaces(out,base,nu,nv);

  if(caps && !closed)
  {
    // one fan per end, around a vertex sitting on the curve itself
    for(sInt e=0;e<2;e++)
    {
      sInt row = e ? nv-1 : 0;
      sInt center = out->Vertices.GetCount();
      Wz4MeshVertex *cv = out->Vertices.AddMany(1);
      cv->Init(pos[row],0.5f,0.5f);

      Wz4MeshFace *mf = out->Faces.AddMany(slices);
      for(sInt j=0;j<slices;j++)
      {
        mf->Init(3);
        mf->Vertex[0] = center;
        // the two ends face opposite ways
        mf->Vertex[e?1:2] = base + row*nu + j;
        mf->Vertex[e?2:1] = base + row*nu + j+1;
        mf++;
      }
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   primitives                                                         ***/
/***                                                                      ***/
/****************************************************************************/

namespace
{
  // vertex cache for icosphere subdivision: an edge must produce the same new
  // vertex from both of the faces that share it, or the sphere comes apart
  struct MidpointCache
  {
    sU64 *Key;
    sInt *Value;
    sInt Size;

    void Init(sInt maxentries)
    {
      Size = 16;
      while(Size < maxentries*2) Size *= 2;
      Key = new sU64[Size];
      Value = new sInt[Size];
      for(sInt i=0;i<Size;i++) { Key[i] = ~0ULL; Value[i] = -1; }
    }
    void Exit() { delete[] Key; delete[] Value; }

    sInt *Find(sInt a,sInt b)
    {
      if(a>b) sSwap(a,b);
      sU64 key = (sU64(a)<<32) | sU32(b);
      sU32 h = sU32((key*2654435761ULL)>>16) & (Size-1);
      while(Key[h]!=~0ULL && Key[h]!=key)
        h = (h+1) & (Size-1);
      Key[h] = key;
      return &Value[h];
    }
  };

  // a position on the unit sphere, with the texture coordinates that go with it
  static void SphereVertex(Wz4MeshVertex *mv,sVector30 n,sF32 radius)
  {
    n.Unit();
    sF32 u = sFATan2(n.z,n.x)/sPI2F + 0.5f;
    sF32 v = sFACos(sClamp<sF32>(n.y,-1,1))/sPIF;
    mv->Init(sVector31(n*radius),u,v);
    mv->Normal = n;
  }
}

void MakeIcosphere(Wz4Mesh *out,sInt subdiv,sF32 radius)
{
  subdiv = sClamp<sInt>(subdiv,0,6);

  static const sF32 t = 1.61803398875f;    // golden ratio
  static const sF32 IcoVert[12][3] =
  {
    {-1, t, 0},{ 1, t, 0},{-1,-t, 0},{ 1,-t, 0},
    { 0,-1, t},{ 0, 1, t},{ 0,-1,-t},{ 0, 1,-t},
    { t, 0,-1},{ t, 0, 1},{-t, 0,-1},{-t, 0, 1},
  };
  static const sInt IcoFace[20][3] =
  {
    {0,11,5},{0,5,1},{0,1,7},{0,7,10},{0,10,11},
    {1,5,9},{5,11,4},{11,10,2},{10,7,6},{7,1,8},
    {3,9,4},{3,4,2},{3,2,6},{3,6,8},{3,8,9},
    {4,9,5},{2,4,11},{6,2,10},{8,6,7},{9,8,1},
  };

  // work on plain positions first and only turn them into mesh vertices at the
  // end, so that the midpoint cache has something cheap to index
  sArray<sVector30> pos;
  sArray<sInt> idx;
  for(sInt i=0;i<12;i++)
  {
    sVector30 v(IcoVert[i][0],IcoVert[i][1],IcoVert[i][2]);
    v.Unit();
    pos.AddTail(v);
  }
  for(sInt i=0;i<20;i++)
    for(sInt j=0;j<3;j++)
      idx.AddTail(IcoFace[i][j]);

  for(sInt s=0;s<subdiv;s++)
  {
    MidpointCache cache;
    cache.Init(idx.GetCount());

    sArray<sInt> next;
    for(sInt f=0;f<idx.GetCount();f+=3)
    {
      sInt v[3] = { idx[f],idx[f+1],idx[f+2] };
      sInt m[3];
      for(sInt e=0;e<3;e++)
      {
        sInt a = v[e], b = v[(e+1)%3];
        sInt *slot = cache.Find(a,b);
        if(*slot<0)
        {
          sVector30 mid = (pos[a]+pos[b])*0.5f;
          mid.Unit();
          *slot = pos.GetCount();
          pos.AddTail(mid);
        }
        m[e] = *slot;
      }

      sInt tri[4][3] = { {v[0],m[0],m[2]},{v[1],m[1],m[0]},{v[2],m[2],m[1]},{m[0],m[1],m[2]} };
      for(sInt i=0;i<4;i++)
        for(sInt j=0;j<3;j++)
          next.AddTail(tri[i][j]);
    }

    cache.Exit();
    idx.Copy(next);
  }

  out->AddDefaultCluster();

  // Every triangle gets its own three vertices. Sharing them would be smaller,
  // but the seam where u wraps from 1 back to 0 would then smear a whole
  // texture across one column of triangles.
  sInt tris = idx.GetCount()/3;
  Wz4MeshVertex *mv = out->Vertices.AddMany(tris*3);
  Wz4MeshFace *mf = out->Faces.AddMany(tris);

  for(sInt f=0;f<tris;f++)
  {
    sF32 uu[3];
    for(sInt j=0;j<3;j++)
    {
      SphereVertex(mv+f*3+j,pos[idx[f*3+j]],radius);
      uu[j] = mv[f*3+j].U0;
    }

    // pull the wrapped corner of a triangle that straddles the seam over to
    // the other side, so it interpolates the short way round
    sF32 umin = sMin(uu[0],sMin(uu[1],uu[2]));
    sF32 umax = sMax(uu[0],sMax(uu[1],uu[2]));
    if(umax-umin > 0.5f)
      for(sInt j=0;j<3;j++)
        if(mv[f*3+j].U0 < 0.5f)
          mv[f*3+j].U0 += 1.0f;

    mf[f].Init(3);
    mf[f].Vertex[0] = f*3+0;
    mf[f].Vertex[1] = f*3+2;
    mf[f].Vertex[2] = f*3+1;
  }
}

/****************************************************************************/

void MakePlatonic(Wz4Mesh *out,sInt type,sF32 radius)
{
  static const sF32 p = 1.61803398875f;      // golden ratio
  static const sF32 q = 0.61803398875f;      // 1/p

  // vertices, then faces as counts followed by indices
  static const sF32 TetraV[4][3]  = { {1,1,1},{1,-1,-1},{-1,1,-1},{-1,-1,1} };
  static const sInt TetraF[]      = { 3,0,2,1, 3,0,1,3, 3,0,3,2, 3,1,2,3, -1 };

  static const sF32 CubeV[8][3]   = { {-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
                                      {-1,-1, 1},{1,-1, 1},{1,1, 1},{-1,1, 1} };
  static const sInt CubeF[]       = { 4,0,3,2,1, 4,4,5,6,7, 4,0,1,5,4,
                                      4,1,2,6,5, 4,2,3,7,6, 4,3,0,4,7, -1 };

  static const sF32 OctaV[6][3]   = { {1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1} };
  static const sInt OctaF[]       = { 3,0,2,4, 3,2,1,4, 3,1,3,4, 3,3,0,4,
                                      3,2,0,5, 3,1,2,5, 3,3,1,5, 3,0,3,5, -1 };

  static const sF32 DodecaV[20][3]= { {1,1,1},{1,1,-1},{1,-1,1},{1,-1,-1},
                                      {-1,1,1},{-1,1,-1},{-1,-1,1},{-1,-1,-1},
                                      {0,q,p},{0,q,-p},{0,-q,p},{0,-q,-p},
                                      {q,p,0},{q,-p,0},{-q,p,0},{-q,-p,0},
                                      {p,0,q},{p,0,-q},{-p,0,q},{-p,0,-q} };
  static const sInt DodecaF[]     = { 5,0,8,10,2,16,  5,0,16,17,1,12, 5,0,12,14,4,8,
                                      5,8,4,18,6,10,  5,16,2,13,3,17, 5,12,1,9,5,14,
                                      5,10,6,15,13,2, 5,17,3,11,9,1,  5,14,5,19,18,4,
                                      5,6,18,19,7,15, 5,13,15,7,11,3, 5,9,11,7,19,5, -1 };

  static const sF32 IcosaV[12][3] = { {-1,p,0},{1,p,0},{-1,-p,0},{1,-p,0},
                                      {0,-1,p},{0,1,p},{0,-1,-p},{0,1,-p},
                                      {p,0,-1},{p,0,1},{-p,0,-1},{-p,0,1} };
  static const sInt IcosaF[]      = { 3,0,11,5, 3,0,5,1,  3,0,1,7,  3,0,7,10, 3,0,10,11,
                                      3,1,5,9,  3,5,11,4, 3,11,10,2,3,10,7,6, 3,7,1,8,
                                      3,3,9,4,  3,3,4,2,  3,3,2,6,  3,3,6,8,  3,3,8,9,
                                      3,4,9,5,  3,2,4,11, 3,6,2,10, 3,8,6,7,  3,9,8,1, -1 };

  const sF32 (*verts)[3];
  const sInt *faces;
  sInt vcount;

  switch(type)
  {
  default:
  case PLATONIC_TETRA:  verts = TetraV;  vcount =  4; faces = TetraF;  break;
  case PLATONIC_CUBE:   verts = CubeV;   vcount =  8; faces = CubeF;   break;
  case PLATONIC_OCTA:   verts = OctaV;   vcount =  6; faces = OctaF;   break;
  case PLATONIC_DODECA: verts = DodecaV; vcount = 20; faces = DodecaF; break;
  case PLATONIC_ICOSA:  verts = IcosaV;  vcount = 12; faces = IcosaF;  break;
  }

  out->AddDefaultCluster();

  // flat faces want their own vertices anyway, and a pentagon has to be
  // triangulated because a Wz4MeshFace holds at most four corners
  const sInt *f = faces;
  while(*f>=0)
  {
    sInt n = *f++;
    sInt base = out->Vertices.GetCount();
    Wz4MeshVertex *mv = out->Vertices.AddMany(n);

    sVector30 center(0,0,0);
    for(sInt i=0;i<n;i++)
    {
      sVector30 v(verts[f[i]][0],verts[f[i]][1],verts[f[i]][2]);
      v.Unit();
      center = center + v;
    }
    center = center*(1.0f/n);

    for(sInt i=0;i<n;i++)
    {
      sVector30 v(verts[f[i]][0],verts[f[i]][1],verts[f[i]][2]);
      v.Unit();
      sF32 a = sF32(i)/n*sPI2F;
      mv[i].Init(sVector31(v*radius),0.5f+0.5f*sFCos(a),0.5f+0.5f*sFSin(a));
      mv[i].Normal = center.GetUnit();
    }

    if(n<=4)
    {
      Wz4MeshFace *mf = out->Faces.AddMany(1);
      mf->Init(n);
      for(sInt i=0;i<n;i++)
        mf->Vertex[i] = base+i;
    }
    else
    {
      Wz4MeshFace *mf = out->Faces.AddMany(n-2);
      for(sInt i=0;i<n-2;i++)
      {
        mf[i].Init(3);
        mf[i].Vertex[0] = base;
        mf[i].Vertex[1] = base+i+1;
        mf[i].Vertex[2] = base+i+2;
      }
    }

    f += n;
  }
}

/****************************************************************************/

void MakeTorusKnot(Wz4Mesh *out,sInt p,sInt q,sInt segments,sInt slices,
                   sF32 radius,sF32 ringradius,sF32 tuberadius,sBool caps)
{
  segments = sClamp<sInt>(segments,3,4096);
  slices = sClamp<sInt>(slices,3,256);

  sInt count = segments+1;              // +1 closes the tube onto itself
  sVector31 *pos = new sVector31[count];
  sVector30 *ref = new sVector30[count];
  sF32 *rad = new sF32[count];

  for(sInt i=0;i<count;i++)
  {
    sF32 t = sF32(i%segments)/segments*sPI2F;
    sF32 r = radius + ringradius*sFCos(q*t);

    pos[i].Init(r*sFCos(p*t),ringradius*sFSin(q*t),r*sFSin(p*t));

    // away from the ring the knot is wound on: periodic, so the frame lines
    // up again after a full turn
    sVector31 center(radius*sFCos(p*t),0,radius*sFSin(p*t));
    ref[i] = pos[i]-center;
    rad[i] = tuberadius;
  }

  out->AddDefaultCluster();
  BuildTube(out,pos,ref,rad,count,slices,sTRUE,caps);

  delete[] pos;
  delete[] ref;
  delete[] rad;
}

/****************************************************************************/

// r(angle) of the superformula. para is m,n1,n2,n3,a,b.
static sF32 SuperFormula(const sF32 *para,sF32 angle)
{
  sF32 m = para[0], n1 = para[1], n2 = para[2], n3 = para[3];
  sF32 a = para[4], b = para[5];
  if(a==0) a = 0.0001f;
  if(b==0) b = 0.0001f;
  if(n1==0) n1 = 0.0001f;

  sF32 t1 = sFPow(sAbs(sFCos(m*angle*0.25f)/a),n2);
  sF32 t2 = sFPow(sAbs(sFSin(m*angle*0.25f)/b),n3);
  sF32 s = t1+t2;
  if(s<0.000001f) return 0;
  return sFPow(s,-1.0f/n1);
}

void MakeSupershape(Wz4Mesh *out,sInt segments,sInt slices,sF32 radius,
                    const sF32 *para0,const sF32 *para1)
{
  segments = sClamp<sInt>(segments,3,1024);   // around, longitude
  slices = sClamp<sInt>(slices,3,1024);       // pole to pole, latitude

  out->AddDefaultCluster();

  sInt nu = segments+1;
  sInt nv = slices+1;
  sInt base = out->Vertices.GetCount();
  Wz4MeshVertex *mv = out->Vertices.AddMany(nu*nv);

  for(sInt v=0;v<nv;v++)
  {
    sF32 lat = (sF32(v)/slices - 0.5f)*sPIF;
    sF32 r2 = SuperFormula(para1,lat);
    for(sInt u=0;u<nu;u++)
    {
      sF32 lon = (sF32(u%segments)/segments - 0.5f)*sPI2F;
      sF32 r1 = SuperFormula(para0,lon);

      // the shapes run away to infinity for some parameters -- clamp rather
      // than hand the renderer a coordinate it cannot deal with
      r1 = sClamp<sF32>(r1,0,1000.0f);
      r2 = sClamp<sF32>(r2,0,1000.0f);

      sVector31 pos;
      pos.x = radius * r1*sFCos(lon) * r2*sFCos(lat);
      pos.y = radius * r2*sFSin(lat);
      pos.z = radius * r1*sFSin(lon) * r2*sFCos(lat);
      mv[u+v*nu].Init(pos,sF32(u)/segments,sF32(v)/slices);
    }
  }

  AddGridFaces(out,base,nu,nv);
}

/****************************************************************************/

void MakeSpring(Wz4Mesh *out,sF32 turns,sF32 radius,sF32 height,sInt segments,
                sInt slices,sF32 tuberadius,sF32 taper,sBool caps)
{
  segments = sClamp<sInt>(segments,3,256);        // per turn
  slices = sClamp<sInt>(slices,3,256);
  turns = sClamp<sF32>(turns,0.01f,1024.0f);

  sInt count = sInt(segments*turns)+1;
  count = sClamp<sInt>(count,2,65536);

  sVector31 *pos = new sVector31[count];
  sVector30 *ref = new sVector30[count];
  sF32 *rad = new sF32[count];

  for(sInt i=0;i<count;i++)
  {
    sF32 f = sF32(i)/(count-1);
    sF32 a = f*turns*sPI2F;
    pos[i].Init(radius*sFCos(a),height*(f-0.5f),radius*sFSin(a));
    ref[i].Init(sFCos(a),0,sFSin(a));          // straight out from the axis
    rad[i] = tuberadius*(1.0f + taper*(f-0.5f));
  }

  out->AddDefaultCluster();
  BuildTube(out,pos,ref,rad,count,slices,sFALSE,caps);

  delete[] pos;
  delete[] ref;
  delete[] rad;
}

/****************************************************************************/

void MakeLathe(Wz4Mesh *out,const sF32 *profile,sInt count,sInt segments,
               sF32 arc,sBool caps)
{
  if(count<2) return;
  segments = sClamp<sInt>(segments,3,4096);
  arc = sClamp<sF32>(arc,0.001f,1.0f);
  sBool full = arc>=0.999f;

  out->AddDefaultCluster();

  sInt nu = segments+1;
  sInt nv = count;
  sInt base = out->Vertices.GetCount();
  Wz4MeshVertex *mv = out->Vertices.AddMany(nu*nv);

  for(sInt v=0;v<nv;v++)
  {
    sF32 pr = profile[v*2+0];
    sF32 ph = profile[v*2+1];
    for(sInt u=0;u<nu;u++)
    {
      // on a full revolution the last column sits exactly on the first, which
      // is what makes the seam's texture coordinates come out right
      sF32 a = sF32(full ? (u%segments) : u)/segments*sPI2F*arc;
      mv[u+v*nu].Init(sVector31(pr*sFCos(a),ph,pr*sFSin(a)),sF32(u)/segments,sF32(v)/(nv-1));
    }
  }

  AddGridFaces(out,base,nu,nv);

  if(caps)
  {
    for(sInt e=0;e<2;e++)
    {
      sInt row = e ? nv-1 : 0;
      if(sAbs(profile[row*2+0]) < 0.00001f)     // already closed on the axis
        continue;

      sInt center = out->Vertices.GetCount();
      Wz4MeshVertex *cv = out->Vertices.AddMany(1);
      cv->Init(sVector31(0,profile[row*2+1],0),0.5f,0.5f);

      Wz4MeshFace *mf = out->Faces.AddMany(segments);
      for(sInt u=0;u<segments;u++)
      {
        mf->Init(3);
        mf->Vertex[0] = center;
        mf->Vertex[e?1:2] = base + row*nu + u;
        mf->Vertex[e?2:1] = base + row*nu + u+1;
        mf++;
      }
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   modifiers                                                          ***/
/***                                                                      ***/
/****************************************************************************/

void Shell(Wz4Mesh *mesh,sF32 thickness,sInt mode)
{
  sInt vcount = mesh->Vertices.GetCount();
  sInt fcount = mesh->Faces.GetCount();
  if(vcount==0 || fcount==0) return;

  sF32 outer = 0,inner = -thickness;
  switch(mode)
  {
  case SHELL_OUTSIDE:  outer = thickness;      inner = 0;              break;
  case SHELL_INSIDE:   outer = 0;              inner = -thickness;     break;
  case SHELL_CENTERED: outer = thickness*0.5f; inner = -thickness*0.5f;break;
  }

  Wz4MeshFaceConnect *conn = mesh->Adjacency();

  // the inner surface: same vertices, pushed the other way, faces reversed
  Wz4MeshVertex *nv = mesh->Vertices.AddMany(vcount);
  for(sInt i=0;i<vcount;i++)
  {
    nv[i] = mesh->Vertices[i];
    nv[i].Pos = nv[i].Pos + nv[i].Normal*inner;
    nv[i].Normal = -nv[i].Normal;
  }
  if(outer!=0)
  {
    for(sInt i=0;i<vcount;i++)
      mesh->Vertices[i].Pos = mesh->Vertices[i].Pos + mesh->Vertices[i].Normal*outer;
  }

  Wz4MeshFace *nf = mesh->Faces.AddMany(fcount);
  for(sInt i=0;i<fcount;i++)
  {
    Wz4MeshFace *of = &mesh->Faces[i];
    nf[i] = *of;
    for(sInt j=0;j<of->Count;j++)
      nf[i].Vertex[j] = of->Vertex[of->Count-1-j] + vcount;
  }

  // and a rim wherever the original surface just stopped
  sInt border = 0;
  for(sInt i=0;i<fcount;i++)
    for(sInt j=0;j<mesh->Faces[i].Count;j++)
      if(conn[i].Adjacent[j] < 0)
        border++;

  if(border>0)
  {
    Wz4MeshFace *bf = mesh->Faces.AddMany(border);
    for(sInt i=0;i<fcount;i++)
    {
      Wz4MeshFace *of = &mesh->Faces[i];
      for(sInt j=0;j<of->Count;j++)
      {
        if(conn[i].Adjacent[j] >= 0) continue;
        sInt v0 = of->Vertex[j];
        sInt v1 = of->Vertex[(j+1)%of->Count];
        bf->Init(4);
        bf->Cluster = of->Cluster;
        bf->Vertex[0] = v0;
        bf->Vertex[1] = v0+vcount;
        bf->Vertex[2] = v1+vcount;
        bf->Vertex[3] = v1;
        bf++;
      }
    }
  }

  delete[] conn;
  mesh->Flush();
}

/****************************************************************************/

void Smooth(Wz4Mesh *mesh,sInt iterations,sF32 strength,sBool taubin,sInt selection)
{
  sInt vcount = mesh->Vertices.GetCount();
  if(vcount==0) return;
  iterations = sClamp<sInt>(iterations,1,64);

  // A cube has 24 vertices, three per corner, because they differ in normal
  // and texture coordinates. Smoothing them apart would tear the mesh, so
  // everything here happens per position, not per vertex.
  sInt *base = mesh->BasePos(1);

  sArray<sVector31> pos;
  pos.AddMany(vcount);
  for(sInt i=0;i<vcount;i++)
    pos[i] = mesh->Vertices[i].Pos;

  sArray<sVector30> sum;
  sArray<sInt> cnt;
  sum.AddMany(vcount);
  cnt.AddMany(vcount);

  for(sInt it=0;it<iterations*(taubin?2:1);it++)
  {
    // Taubin alternates a positive pass with a slightly larger negative one.
    // The pair is chosen so that the two passes cancel for low frequencies and
    // only the noise is left behind -- without it, repeated smoothing walks
    // every mesh towards its own centre.
    sF32 amount = strength;
    if(taubin && (it&1))
      amount = 1.0f/(0.1f - 1.0f/strength);

    for(sInt i=0;i<vcount;i++) { sum[i].Init(0,0,0); cnt[i] = 0; }

    Wz4MeshFace *mf;
    sFORALL(mesh->Faces,mf)
    {
      for(sInt j=0;j<mf->Count;j++)
      {
        sInt a = base[mf->Vertex[j]];
        sInt b = base[mf->Vertex[(j+1)%mf->Count]];
        sum[a] = sum[a] + (pos[b]-sVector31(0,0,0));
        cnt[a]++;
        sum[b] = sum[b] + (pos[a]-sVector31(0,0,0));
        cnt[b]++;
      }
    }

    for(sInt i=0;i<vcount;i++)
    {
      sInt b = base[i];
      if(cnt[b]==0) continue;

      sF32 sel = 1.0f;
      if(selection==1) sel = mesh->Vertices[i].Select;          // selected only
      if(selection==2) sel = 1.0f-mesh->Vertices[i].Select;     // unselected only
      if(sel<=0) continue;

      sVector31 avg = sVector31(sum[b]*(1.0f/cnt[b]));
      pos[i] = pos[i] + (avg-pos[i])*(amount*sel);
    }

    // members of one position class must stay together
    for(sInt i=0;i<vcount;i++)
      if(base[i]!=i)
        pos[i] = pos[base[i]];
  }

  for(sInt i=0;i<vcount;i++)
    mesh->Vertices[i].Pos = pos[i];

  delete[] base;
  mesh->Flush();
}

/****************************************************************************/

void UVProject(Wz4Mesh *mesh,sInt mode,sInt uvset,const sMatrix34 &mat,
               sF32 tilex,sF32 tiley,sF32 offsetx,sF32 offsety)
{
  // mat places the projector in the world; what we need is the other way round
  sMatrix34 inv = mat;
  inv.Invert34();

  Wz4MeshVertex *mv;
  sFORALL(mesh->Vertices,mv)
  {
    sVector31 p = mv->Pos * inv;
    sVector30 n = mv->Normal * inv;
    sF32 u = 0,v = 0;

    switch(mode)
    {
    default:
    case UVPROJECT_PLANAR:
      u = p.x;
      v = -p.y;
      break;

    case UVPROJECT_CYLINDER:
      u = sFATan2(p.z,p.x)/sPI2F + 0.5f;
      v = -p.y;
      break;

    case UVPROJECT_SPHERE:
      {
        sVector30 d(p.x,p.y,p.z);
        d.Unit();
        u = sFATan2(d.z,d.x)/sPI2F + 0.5f;
        v = sFACos(sClamp<sF32>(d.y,-1,1))/sPIF;
      }
      break;

    case UVPROJECT_BOX:
      // the face's own direction decides which of the six planes it gets
      switch(n.MaxAxisAbs())
      {
      case 0: u = (n.x>0) ? -p.z : p.z; v = -p.y; break;
      case 1: u = p.x; v = (n.y>0) ? -p.z : p.z;  break;
      default:u = (n.z>0) ? p.x : -p.x; v = -p.y; break;
      }
      break;
    }

    u = u*tilex + offsetx;
    v = v*tiley + offsety;

    if(uvset==0) { mv->U0 = u; mv->V0 = v; }
    else         { mv->U1 = u; mv->V1 = v; }
  }
  mesh->Flush();
}

/****************************************************************************/

void Weld(Wz4Mesh *mesh,sF32 threshold,sBool recalcnormals)
{
  sInt vcount = mesh->Vertices.GetCount();
  if(vcount==0 || threshold<=0) return;

  // a grid with cells the size of the threshold: two vertices that are close
  // enough to merge are then either in the same cell or in a neighbouring one
  sF32 cell = threshold;
  sInt hashsize = 16;
  while(hashsize < vcount*2) hashsize *= 2;

  sInt *first = new sInt[hashsize];
  sInt *next = new sInt[vcount];
  sInt *remap = new sInt[vcount];
  for(sInt i=0;i<hashsize;i++) first[i] = -1;

  sF32 thres2 = threshold*threshold;

  for(sInt i=0;i<vcount;i++)
  {
    sVector31 p = mesh->Vertices[i].Pos;
    sInt cx = sInt(sRoundDown(p.x/cell));
    sInt cy = sInt(sRoundDown(p.y/cell));
    sInt cz = sInt(sRoundDown(p.z/cell));

    sInt found = -1;
    for(sInt dz=-1;dz<=1 && found<0;dz++)
    {
      for(sInt dy=-1;dy<=1 && found<0;dy++)
      {
        for(sInt dx=-1;dx<=1 && found<0;dx++)
        {
          sU32 h = sU32((cx+dx)*73856093) ^ sU32((cy+dy)*19349663) ^ sU32((cz+dz)*83492791);
          for(sInt j=first[h&(hashsize-1)];j>=0;j=next[j])
          {
            if((mesh->Vertices[j].Pos-p).LengthSq() <= thres2)
            {
              found = j;
              break;
            }
          }
        }
      }
    }

    if(found>=0)
    {
      remap[i] = found;
    }
    else
    {
      sU32 h = sU32(cx*73856093) ^ sU32(cy*19349663) ^ sU32(cz*83492791);
      h &= (hashsize-1);
      next[i] = first[h];
      first[h] = i;
      remap[i] = i;
    }
  }

  Wz4MeshFace *mf;
  sFORALL(mesh->Faces,mf)
    for(sInt j=0;j<mf->Count;j++)
      mf->Vertex[j] = remap[mf->Vertex[j]];

  delete[] first;
  delete[] next;
  delete[] remap;

  mesh->RemoveDegenerateFaces();
  mesh->MergeVertices();                 // also drops what is now unreferenced
  if(recalcnormals)
    mesh->CalcNormalAndTangents();
  mesh->Flush();
}

/****************************************************************************/

void ScatterOnMesh(Wz4Mesh *out,Wz4Mesh *target,Wz4Mesh *item,sInt count,
                   sInt seed,sF32 scalemin,sF32 scalemax,sF32 align,
                   sBool spin,sBool keeptarget)
{
  count = sClamp<sInt>(count,0,16384);
  if(item->Faces.GetCount()==0 || target->Faces.GetCount()==0)
    return;

  if(keeptarget)
    out->Add(target);
  if(out->Clusters.GetCount()==0)
    out->AddDefaultCluster();

  // one entry per triangle of the target, with the running total of the area
  // in front of it, so that a uniform random number picks a triangle in
  // proportion to how much surface it contributes
  struct Tri { sInt a,b,c; sF32 Sum; };
  sArray<Tri> tris;
  sF32 total = 0;

  Wz4MeshFace *mf;
  sFORALL(target->Faces,mf)
  {
    for(sInt j=2;j<mf->Count;j++)
    {
      Tri t;
      t.a = mf->Vertex[0];
      t.b = mf->Vertex[j-1];
      t.c = mf->Vertex[j];
      sVector30 e0 = target->Vertices[t.b].Pos - target->Vertices[t.a].Pos;
      sVector30 e1 = target->Vertices[t.c].Pos - target->Vertices[t.a].Pos;
      total += e0.Cross(e1).Length()*0.5f;
      t.Sum = total;
      tris.AddTail(t);
    }
  }
  if(total<=0) return;

  sInt itemcluster = out->Clusters.GetCount();
  Wz4MeshCluster *cl;
  sFORALL(item->Clusters,cl)
  {
    Wz4MeshCluster *ncl = new Wz4MeshCluster;
    ncl->Mtrl = cl->Mtrl;
    if(ncl->Mtrl) ncl->Mtrl->AddRef();
    out->Clusters.AddTail(ncl);
  }
  if(item->Clusters.GetCount()==0)
    itemcluster = 0;

  sRandomMT rnd;
  rnd.Seed(seed);

  sInt icount = item->Vertices.GetCount();
  sInt ifcount = item->Faces.GetCount();

  for(sInt i=0;i<count;i++)
  {
    // pick a triangle, then a point in it
    sF32 pick = rnd.Float(total);
    sInt lo = 0,hi = tris.GetCount()-1;
    while(lo<hi)
    {
      sInt mid = (lo+hi)/2;
      if(tris[mid].Sum < pick) lo = mid+1; else hi = mid;
    }
    const Tri &t = tris[lo];

    sF32 u = rnd.Float(1.0f);
    sF32 v = rnd.Float(1.0f);
    if(u+v>1) { u = 1-u; v = 1-v; }

    const Wz4MeshVertex &va = target->Vertices[t.a];
    const Wz4MeshVertex &vb = target->Vertices[t.b];
    const Wz4MeshVertex &vc = target->Vertices[t.c];

    sVector31 p = va.Pos + (vb.Pos-va.Pos)*u + (vc.Pos-va.Pos)*v;
    sVector30 n = va.Normal + (vb.Normal-va.Normal)*u + (vc.Normal-va.Normal)*v;
    if(n.LengthSq()<0.000001f)
      n.Init(0,1,0);
    n.Unit();

    // the item's own up (+y) turned towards the surface normal, by as much as
    // align asks for
    sVector30 up(0,1,0);
    sVector30 axis = up.Cross(n);
    sF32 dot = sClamp<sF32>(up^n,-1,1);
    sMatrix34 rot;
    rot.Init();
    if(axis.LengthSq()>0.000001f)
    {
      axis.Unit();
      sQuaternion quat;
      quat.Init(axis,sFACos(dot)*sClamp<sF32>(align,0,1));
      rot.Init(quat);
    }
    else if(dot<0 && align>0)
    {
      rot.i.Init(1,0,0); rot.j.Init(0,-1,0); rot.k.Init(0,0,-1);
    }

    if(spin)
    {
      sMatrix34 s;
      s.EulerXYZ(0,rnd.Float(sPI2F),0);
      rot = s*rot;
    }

    sF32 scale = scalemin + rnd.Float(1.0f)*(scalemax-scalemin);

    sMatrix34 mat = rot;
    mat.i = mat.i*scale;
    mat.j = mat.j*scale;
    mat.k = mat.k*scale;
    mat.l = sVector31(p.x,p.y,p.z);

    sMatrix34 matit = mat;             // normals do not want the translation
    matit.l.Init(0,0,0);

    sInt vbase = out->Vertices.GetCount();
    Wz4MeshVertex *mv = out->Vertices.AddMany(icount);
    for(sInt j=0;j<icount;j++)
    {
      mv[j] = item->Vertices[j];
      mv[j].Pos = mv[j].Pos*mat;
      mv[j].Normal = (mv[j].Normal*matit).GetUnit();
      mv[j].Tangent = (mv[j].Tangent*matit).GetUnit();
    }

    Wz4MeshFace *nf = out->Faces.AddMany(ifcount);
    for(sInt j=0;j<ifcount;j++)
    {
      nf[j] = item->Faces[j];
      nf[j].Cluster += itemcluster;
      for(sInt k=0;k<nf[j].Count;k++)
        nf[j].Vertex[k] += vbase;
    }
  }

  out->Flush();
}

/****************************************************************************/

}

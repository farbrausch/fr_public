// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "genmesh.hpp"
#include "genmaterial.hpp"
#include "genbitmap.hpp"
#include "genoverlay.hpp"
#include "genscene.hpp"
#include "engine.hpp"
#include "_util.hpp"
#include "genminmesh.hpp"

#define SCRIPTVERIFY(x) {if(!(x)) return 0;}

// rules:
// - first all SCRIPTVERIFY
// - then CheckMesh()
// - after CheckMesh() you may not return 0
//
// if return 0, then caller calls Release() for every argument.
// if return !=0, then routine calls Release() for every argument
// (except what it may return)

/****************************************************************************/

/****************************************************************************/
/****************************************************************************/

void GenMeshElem::InitElem()
{
  Mask = 0;
  Id = 0;
  Select = 0;
  Used = 1;
}

void GenMeshElem::SelElem(sU32 mask,sBool state,sInt mode)
{
  switch(mode)
  {
  case MSM_ADD:     if(state) Mask |= mask;                     break;
  case MSM_SUB:     if(state) Mask &= ~mask;                    break;
  case MSM_SET:     if(state) Mask |= mask; else Mask &= ~mask; break;
  case MSM_SETNOT:  if(state) Mask &= ~mask; else Mask |= mask; break;
  }
}

void GenMeshElem::SelLogic(sU32 smask1,sU32 smask2,sU32 dmask,sInt mode)
{
  sBool s1,s2;

  s1 = (Mask & smask1) ? 1 : 0;
  s2 = (Mask & smask2) ? 1 : 0;
  if(mode&4)  s1 ^= 1;
  if(mode&8)  s2 ^= 1;

  switch(mode&3)
  {
  case 0: s1 |= s2; break;
  case 1: s1 &= s2; break;
  case 2: s1 ^= s2; break;
  }

  if(mode&16) s1 ^= 1;

  if(s1) Mask |= dmask; else Mask &= ~dmask;
}

void GenMeshVert::Init()
{
  sInt i;

  InitElem();
  Next = -1;
  First = -1;
  Temp = -1;
  Temp2 = -1;
  ReIndex = -1;
  for(i=0;i<4;i++)
  {
    Matrix[i] = 0xff;
    Weight[i] = 0;
  }
}

void GenMeshEdge::Init()
{
  InitElem();
  Next[0] = -1;
  Next[1] = -1;
  Prev[0] = -1;
  Prev[1] = -1;
  Face[0] = -1;
  Face[1] = -1;
  Vert[0] = -1;
  Vert[1] = -1;
  Temp[0] = -1;
  Temp[1] = -1;
  Crease = 0;
}

void GenMeshFace::Init()
{
  InitElem();
  Material = 1;
  Edge = -1;
  Temp = -1;
  Temp2 = 0;
  Temp3 = 0;
}

/****************************************************************************/

void GenSimpleFace::Init(sInt Verts)
{
  VertexCount = Verts;
  Vertices = new sVector[VertexCount];

  sVERIFY(VertexCount < 64);
}

void GenSimpleFace::Exit()
{
  delete[] Vertices;
  Vertices = 0;
}

void GenSimpleFace::AddVertex(const sVector &v)
{
  Vertices[VertexCount++] = v;
}

static sInt ClassifyVert(const sVector &v,const sVector &plane)
{
  static const sF32 epsilon = 1e-3f; // 1 mm
  sF32 dot = v.Dot3(plane) + plane.w;

  if(dot > epsilon) // front / in
    return 0;
  else if(dot < -epsilon) // back / out
    return 1;
  else // on
    return 2;
}

ClipCode GenSimpleFace::Clip(const sVector &plane,GenSimpleFace *faces) const
{
  static const sF32 div_epsilon = 1e-20f;
  sF32 d1,d2,t;
  sInt lastside,side;
  sInt lasti,i;
  sInt cross;
  sVector v;
  sBool allOn = sTRUE;

//  faces[0].Init(VertexCount + 2);
//  faces[1].Init(VertexCount + 2);
  faces[0].VertexCount = 0;
  faces[1].VertexCount = 0;
  cross = 0;

  lasti = VertexCount - 1;
  lastside = ClassifyVert(Vertices[lasti],plane);

  for(i=0;i<VertexCount;i++)
  {
    side = ClassifyVert(Vertices[i],plane);

    if(side == 2) // on, add on both sides
    {
      faces[0].AddVertex(Vertices[i]);
      faces[1].AddVertex(Vertices[i]);
    }
    else if(side == lastside || lastside == 2) // not crossed
      faces[side].AddVertex(Vertices[i]);
    else // crossed (generates new "on" point)
    {
      // calc clip t
      d1 = Vertices[lasti].Dot3(plane) + plane.w;
      d2 = Vertices[i].Dot3(plane) + plane.w;
      t = -d1 / (d2 - d1);

      // clip and assert new point is on the plane (else we're in trouble)
      v.Lin4(Vertices[lasti],Vertices[i],t);
      sVERIFY(ClassifyVert(v,plane) == 2);

      // add to both sides
      faces[0].AddVertex(v);
      faces[1].AddVertex(v);

      // add out point to out side
      faces[side].AddVertex(Vertices[i]);

      cross = sTRUE;
    }

    if(side != 2)
      allOn = sFALSE;

    lastside = side;
    lasti = i;
  }

  return allOn ? CC_ALL_ON_PLANE : cross ? CC_CROSSED : CC_NOT_CROSSED;
}

sBool GenSimpleFace::Inside(const sVector *planes,sInt nPlanes) const
{
  sInt i,j;

  for(i=0;i<nPlanes;i++)
    for(j=0;j<VertexCount;j++)
      if(ClassifyVert(Vertices[j],planes[i]) == 0) // out?
        return sFALSE;

  return sTRUE;
}

/****************************************************************************/

GenSimpleMesh::GenSimpleMesh()
{
  ClassId = KC_SMESH;

  Faces.Init();
}

GenSimpleMesh::~GenSimpleMesh()
{
  Clear();
  Faces.Exit();
}

void GenSimpleMesh::Copy(KObject *o)
{
  GenSimpleMesh *os;

  sVERIFY(o->ClassId == KC_SMESH);
  os = (GenSimpleMesh *) o;

  Faces.Copy(os->Faces);
}

void GenSimpleMesh::Clear()
{
  sInt i;

  for(i=0;i<Faces.Count;i++)
    Faces[i].Exit();

  Faces.Count = 0;
}

void GenSimpleMesh::Add(const GenSimpleMesh *other)
{
  sInt i;

  for(i=0;i<other->Faces.Count;i++)
    AddFace(other->Faces[i]);
}

void GenSimpleMesh::AddQuad(const sVector &v1,const sVector &v2,const sVector &v3,const sVector &v4)
{
  GenSimpleFace *fc = Faces.Add();

  fc->Init(4);
  fc->VertexCount = 0;
  fc->AddVertex(v1);
  fc->AddVertex(v2);
  fc->AddVertex(v3);
  fc->AddVertex(v4);
}

void GenSimpleMesh::AddFace(const GenSimpleFace &face)
{
  sInt i;

  GenSimpleFace *fc = Faces.Add();

  fc->Init(face.VertexCount);
  for(i=0;i<face.VertexCount;i++)
    fc->Vertices[i] = face.Vertices[i];
}

void GenSimpleMesh::Cube(const sAABox &box)
{
  sVector Verts[8];
  sInt i;

  Clear();

  // prepare vertices
  for(i=0;i<8;i++)
  {
    Verts[i].x = (i&1) ? box.Max.x : box.Min.x;
    Verts[i].y = (i&2) ? box.Max.y : box.Min.y;
    Verts[i].z = (i&4) ? box.Max.z : box.Min.z;
    Verts[i].w = 1.0f;
  }

  // add faces
  AddQuad(Verts[0],Verts[2],Verts[3],Verts[1]); // front
  AddQuad(Verts[4],Verts[6],Verts[2],Verts[0]); // left
  AddQuad(Verts[5],Verts[7],Verts[6],Verts[4]); // back
  AddQuad(Verts[1],Verts[3],Verts[7],Verts[5]); // right
  AddQuad(Verts[2],Verts[6],Verts[7],Verts[3]); // top
  AddQuad(Verts[5],Verts[4],Verts[0],Verts[1]); // bottom
}

sBool GenSimpleMesh::CSGSplitR(const GenSimpleFace &face,const sVector *planes,sInt plane,sInt nPlanes,sBool keepOut)
{
  GenSimpleFace sides[2];
  sVector sidev[2][64];
  sBool split[2];
  sInt i;
  sBool ret;

  sFatal("dierk hat hier das allokationschema geändert und nicht getestet...");

  sides[0].Vertices = sidev[0];
  sides[1].Vertices = sidev[1];
  face.Clip(planes[plane],sides);
  sVERIFY(sides[0].VertexCount <= 64);
  sVERIFY(sides[1].VertexCount <= 64);


  for(i=0;i<2;i++)
  {
    if(sides[i].VertexCount < 3) // degenerate faces can't be split
      split[i] = sFALSE;
    else
    {
      if(plane == nPlanes - 1) // leaf, check whether face inside volume
        split[i] = sides[i].Inside(planes,nPlanes) == keepOut;
      else
        split[i] = CSGSplitR(sides[i],planes,plane+1,nPlanes,keepOut);
    }
  }

  if(split[0] || split[1]) // we have splits, add all remaining polys.
  {
    for(i=0;i<2;i++)
      if(sides[i].VertexCount >= 3 && !split[i])
        AddFace(sides[i]);

    ret = sTRUE;
  }
  else
    ret = sFALSE;

//  sides[0].Exit();
//  sides[1].Exit();
  return ret;
}

void GenSimpleMesh::CSGAgainst(const sVector *planes,sInt nPlanes,sBool keepOut)
{
  sInt i,inFaceCount;
  GenSimpleFace *faces;
  sBool split;

  inFaceCount = Faces.Count;
  faces = Faces.Array;

  // somewhat hackish, but this has to make do for now
  Faces.Array = new GenSimpleFace[16];
  Faces.Alloc = 16;
  Faces.Count = 0;

  for(i=0;i<inFaceCount;i++)
  {
    split = CSGSplitR(faces[i],planes,0,nPlanes,keepOut);
    if(!split) // insert whole face
      AddFace(faces[i]);
  }

  // free old face list
  for(i=0;i<inFaceCount;i++)
    faces[i].Exit();

  delete[] faces;
}

/****************************************************************************/

GenSimpleBrush::GenSimpleBrush()
{
  Mesh = new GenSimpleMesh;
  PlaneCount = 0;
  Planes = 0;
}

GenSimpleBrush::~GenSimpleBrush()
{
  Mesh->Release();
  delete[] Planes;
}

void GenSimpleBrush::Cube(const sAABox &box)
{
  Mesh->Cube(box);

  PlaneCount = 6;
  Planes = new sVector[PlaneCount];

  Planes[0].Init(-1.0f, 0.0f, 0.0f, box.Min.x);
  Planes[1].Init( 1.0f, 0.0f, 0.0f,-box.Max.x);
  Planes[2].Init( 0.0f,-1.0f, 0.0f, box.Min.y);
  Planes[3].Init( 0.0f, 1.0f, 0.0f,-box.Max.y);
  Planes[4].Init( 0.0f, 0.0f,-1.0f, box.Min.z);
  Planes[5].Init( 0.0f, 0.0f, 1.0f,-box.Max.z);

  BBox = box;
}

void GenSimpleBrush::CSGAgainst(const GenSimpleBrush &other,sBool keepOut)
{
  if(BBox.Intersects(other.BBox))
    Mesh->CSGAgainst(other.Planes,other.PlaneCount,sTRUE);
}

/****************************************************************************/
/****************************************************************************/

GenMesh::GenMesh()
{
  ClassId = KC_MESH;

  Vert.Init(16);
  Edge.Init(16);
  Face.Init(16);
  Mtrl.Init(16);
  Coll.Init(1);
  Lgts.Init(1);
  Parts.Init(1);

#if !sINTRO
  _VertMask = 0;
  _VertSize = 0;
#endif

  VertAlloc = 0;
  VertCount = 0;
  VertBuf = 0;

#if !sINTRO
  BoneMatrix.Init();
  BoneCurve.Init();
  KeyCount = 0;
  CurveCount = 0;
  KeyBuf = 0;
  Anim0 = 0;
  Anim1 = 16;
#endif

  PreparedMesh = 0;

  Pivot = -1;
  GotNormals = sFALSE;

  Mtrl.Count = 2;                 // null-material
  Mtrl[0].Material = 0;
  Mtrl[0].Pass = 0;
  Mtrl[1].Material = GenOverlayManager->DefaultMat;
  Mtrl[1].Material->AddRef();
  Mtrl[1].Pass = 0;

#if !sPLAYER
  WireMesh = 0;
  WireFlags = 0;
  WireSelMask = 0;
#endif
}

GenMesh::~GenMesh()
{
  sInt i;

#if !sPLAYER
  UnPrepareWire();
#endif

  for(i=1;i<Mtrl.Count;i++)
    Mtrl[i].Material->Release();
#if !sINTRO
  if(KeyBuf)
    delete[] KeyBuf;
#endif
  if(VertBuf)
    delete[] VertBuf;

  Vert.Exit();
  Edge.Exit();
  Face.Exit();
  Mtrl.Exit();
  Coll.Exit();
  Lgts.Exit();
  Parts.Exit();

  UnPrepare();

#if !sINTRO
  BoneMatrix.Exit();
  BoneCurve.Exit();
#endif
}

void GenMesh::Copy(KObject *o)
{
  GenMesh *om;
  sInt i;//,j;

  sVERIFY(o->ClassId==KC_MESH);
  om = (GenMesh *)o;

  for(i=1;i<Mtrl.Count;i++)
    Mtrl[i].Material->Release();

  Vert.Copy(om->Vert);
  Edge.Copy(om->Edge);
  Face.Copy(om->Face);
  Mtrl.Copy(om->Mtrl);
  Coll.Copy(om->Coll);
  Lgts.Copy(om->Lgts);
  Parts.Copy(om->Parts);

  for(i=1;i<Mtrl.Count;i++)
    Mtrl[i].Material->AddRef();

  PreparedMesh = 0;
#if !sPLAYER
  WireMesh = 0;
  WireFlags = 0;
  WireSelMask = 0;
#endif

  Init(om->VertMask(),om->VertAlloc);
  sVERIFY(VertSize()==om->VertSize());
  sCopyMem4((sU32 *)VertBuf,(sU32 *)om->VertBuf,om->VertSize()*om->VertCount*4);
  VertCount = om->VertCount;

#if !sINTRO
  BoneMatrix.Copy(om->BoneMatrix);
  BoneCurve.Copy(om->BoneCurve);
  KeyCount = om->KeyCount;
  CurveCount = om->CurveCount;
  if(om->KeyBuf)
  {
    KeyBuf = new sF32[KeyCount*CurveCount];
    sCopyMem(KeyBuf,om->KeyBuf,KeyCount*CurveCount*4);
  }
  Anim0 = om->Anim0;
  Anim1 = om->Anim1;
#endif
  
  Pivot = om->Pivot;
  GotNormals = om->GotNormals;
}

sU32 GenMesh::Features2Mask(sInt colorSets,sInt uvSets)
{
  sVERIFY(colorSets>=0 && colorSets<=1);
  sVERIFY(uvSets>=1 && uvSets<=4);

  return sGMF_POS|sGMF_NORMAL|sGMF_TANGENT|(((1<<colorSets)-1)<<sGMI_COLOR0)
    |(((2<<uvSets)-1)<<sGMI_UV0);
}

void GenMesh::Init(sU32 vertmask,sInt vertcount)
{
#if !sINTRO
  _VertMask = vertmask;
  _VertSize = 0;

  for(sInt i=0;i<16;i++)
  {
    if(_VertMask & (1<<i))
      _VertMap[i] = _VertSize++;
    else
      _VertMap[i] = -1;
  }

  sVERIFY(_VertSize);
  sVERIFY(_VertMask&1);
#else
  sVERIFY(vertmask == sGMF_DEFAULT);
#endif

  VertAlloc = vertcount;
  VertCount = 0;
  VertBuf = new sVector[VertSize()*VertAlloc];
  sSetMem4((sU32*)VertBuf,0,VertSize()*VertAlloc*4);

  Vert.AtLeast(vertcount);
  Edge.AtLeast(vertcount*2);
  Face.AtLeast(vertcount/2);
}

void GenMesh::Realloc(sInt vertcount)
{
  sInt ns;
  sVector *nd;

  if(vertcount>=VertAlloc)
  {
    ns = sMax(vertcount,VertAlloc*2-VertAlloc/2);
    nd = new sVector[ns*VertSize()];
    sCopyMem4((sU32 *)nd,(sU32 *)VertBuf,VertSize()*4*VertCount);
    sSetMem4((sU32 *)(nd+VertSize()*VertCount),0,VertSize()*4*(ns-VertCount));
    delete[] VertBuf;

    VertBuf = nd;
    VertAlloc = ns;
  }
  VertCount = vertcount;
}

/****************************************************************************/
/****************************************************************************/

GenMeshFace *GenMesh::GetFace(sU32 i)
{
  return &Face.Array[GetEdge(i)->Face[i&1]];
}

GenMeshFace *GenMesh::GetFaceI(sU32 i)
{
  return &Face.Array[GetEdge(i)->Face[~i&1]];
}

GenMeshVert *GenMesh::GetVert(sU32 i)
{
  return &Vert.Array[GetEdge(i)->Vert[i&1]];
}

sInt GenMesh::GetFaceId(sU32 i)
{
  return GetEdge(i)->Face[i&1];
}

sInt GenMesh::GetVertId(sU32 i)
{
  return GetEdge(i)->Vert[i&1];
}

sInt GenMesh::NextFaceEdge(sU32 i)
{
  return GetEdge(i)->Next[i&1];
}

sInt GenMesh::PrevFaceEdge(sU32 i)
{
  return GetEdge(i)->Prev[i&1];
}

sInt GenMesh::NextVertEdge(sU32 i)
{
  return GetEdge(i)->Prev[i&1]^1;
}

sInt GenMesh::PrevVertEdge(sU32 i)
{
  return GetEdge(i)->Next[~i&1];
}

sInt GenMesh::SkipFaceEdge(sU32 i,sInt num)
{
  if(num<0)
  {
    while(num++)
      i = PrevFaceEdge(i);
  }
  else
  {
    while(num--)
      i = NextFaceEdge(i);
  }

  return i;
}

sInt GenMesh::SkipVertEdge(sU32 i,sInt num)
{
  if(num<0)
  {
    while(num++)
      i = PrevVertEdge(i);
  }
  else
  {
    while(num--)
      i = NextVertEdge(i);
  }

  return num;
}

sInt GenMesh::AddPivot()
{
  sInt i,vs;

  if(Pivot==-1)
  {
    Vert.Resize(Vert.Count+1);
    Realloc(Vert.Count);
    Pivot = Vert.Count-1;
    vs = VertSize();
    for(i=0;i<vs;i++)
      VertBuf[Pivot*vs+i].Init(0,0,0,1);
  }

  return Pivot;
}

#if !sPLAYER

sInt GenMesh::GetValence(sU32 e)
{
	sInt v,k;
	sU32 ee;

	v = GetVertId(e); k = 0; ee = e;
	do
	{
		if(GetVertId(e)==v)
			k++;

		e = NextVertEdge(e);
	}
	while(e!=ee);

	return k;
}

sInt GenMesh::GetDegree(sU32 f)
{
	sInt e,ee,i;

	e = ee = Face[f].Edge;
	i = 0;
	do
	{
		i++;
		e = NextFaceEdge(e);
	}
	while(e!=ee);

	return i;
}

#endif

void GenMesh::Compact()
{
  sVector *nd;

  Vert.Compact();
  Edge.Compact();
  Face.Compact();
  Mtrl.Compact();
  Coll.Compact();
  Lgts.Compact();

  if(VertAlloc > VertCount)
  {
    nd = new sVector[VertCount*VertSize()];
    sCopyMem4((sU32 *)nd,(sU32 *)VertBuf,VertSize()*4*VertCount);
    delete[] VertBuf;
    VertBuf = nd;
    VertAlloc = VertCount;
  }
}

sInt GenMesh::AddVert()
{
  sInt c;

  c = Vert.Count; 
  Realloc(c+1);
  Vert.AtLeast(c+1); 
  Vert.Count++;

  return c;
}

sInt GenMesh::AddNewVert()
{
  sInt v;

  v = AddVert();
  Vert[v].Init();
  Vert[v].First = Vert[v].Next = Vert[v].ReIndex = v;

  return v;
}

sInt GenMesh::AddCopiedVert(sInt src)
{
  sInt v;

  v = AddVert();
  Vert[v] = Vert[src];
  Vert[v].ReIndex = v;
  
  return v;
}

void GenMesh::SplitFace(sU32 i0,sU32 i1,sU32 dmask,sInt dmode)
{
  sInt ee,f0,f1,p0,p1;
  sU32 i;

  sVERIFY(GetFaceId(i0)==GetFaceId(i1));
	sVERIFY(i0!=i1);

  p0 = PrevFaceEdge(i0);
  p1 = PrevFaceEdge(i1);
  f0 = GetFaceId(i0);
  /*ee = Edge.Count; Edge.AtLeast(ee+1); Edge.Count++;
  f1 = Face.Count; Face.AtLeast(f1+1); Face.Count++;*/
  ee = Edge.Count; Edge.Add();
  f1 = Face.Count; Face.Add();

  register GenMeshEdge *Edge = this->Edge.Array;
  register GenMeshFace *Face = this->Face.Array;
  Edge[ee] = Edge[i0/2];
	Edge[ee].Crease = 0; // kill crease flags
  Edge[ee].Sel(dmask,1,dmode);
  Edge[ee].Vert[0] = GetVertId(i1);
  Edge[ee].Vert[1] = GetVertId(i0);
  Edge[ee].Face[0] = f0;
  Edge[ee].Face[1] = f0;
  Edge[ee].Next[0] = i0;
  Edge[ee].Next[1] = i1;
  Edge[ee].Prev[0] = p1;
  Edge[ee].Prev[1] = p0;
  Edge[ee].Temp[0] = -1;
  Edge[ee].Temp[1] = -1;

  Edge[i0>>1].Prev[i0&1] = ee*2;
  Edge[p0>>1].Next[p0&1] = ee*2+1;
  Edge[i1>>1].Prev[i1&1] = ee*2+1;
  Edge[p1>>1].Next[p1&1] = ee*2;

  Face[f1] = Face[f0];
  Face[f1].Sel(dmask,1,dmode);
  Face[f0].Edge = i0;
  Face[f1].Edge = i1;

  i = i1;
  do
  {
    sVERIFY(GetFaceId(i)==f0);
		sVERIFY(NextFaceEdge(PrevFaceEdge(i)) == i);
		sVERIFY(PrevFaceEdge(NextFaceEdge(i)) == i);
    Edge[i/2].Face[i&1] = f1;
    i = NextFaceEdge(i);
  }
  while(i!=i1);

#if !sPLAYER
 //just checking!
  i = i0;
  do
  {
    sVERIFY(GetFaceId(i)==f0);
		sVERIFY(NextFaceEdge(PrevFaceEdge(i)) == i);
		sVERIFY(PrevFaceEdge(NextFaceEdge(i)) == i);
    i = NextFaceEdge(i);
  }
  while(i!=i0);
#endif
}

void GenMesh::SplitBridge(sU32 a,sU32 d,sInt &va,sInt &vb,sU32 dmask,sInt dmode,sInt *dv1,sInt *dv2)
{
  sU32 e,p0,p1,n0,n1;
  sInt v0,v1,cf,i,lc;

  //e = Edge.Count; Edge.AtLeast(e+1); Edge.Count = e+1;
  e = Edge.Count; Edge.Add();
  v0 = va;

  Vert[v0].Sel(dmask,1,dmode);
  Vert[v0].First = v0;
  Vert[v0].Next = v0;

  a = a^1;
  p0 = PrevFaceEdge(d);
  n1 = NextFaceEdge(a);
  v1 = GetVertId(n1);
  sVERIFY(Vert[v1].First == GetVert(d)->First);

  cf=0; // cumulative crease flag
  if(n1==d)
  {
    p0 = e*2+1;
    n1 = e*2;
  }
  else
  {
    for(i=n1;i!=d;i=PrevVertEdge(i))
    {
      if(Edge[i/2].Crease)
      {
        cf|=Edge[i/2].Crease;
        lc=i;
      }
    }
  }

  register GenMeshEdge *Edge = this->Edge.Array;
  Edge[e] = Edge[a/2];
  Edge[e].Crease = cf;
  Edge[e].Sel(dmask,1,dmode);
  Edge[e].Vert[0] = v0;
  Edge[e].Vert[1] = v1;
  Edge[e].Face[0] = GetFaceId(d);
  Edge[e].Face[1] = GetFaceId(a);
  Edge[e].Next[0] = n0 = d;
  Edge[e].Next[1] = n1;
  Edge[e].Prev[0] = p0;
  Edge[e].Prev[1] = p1 = a;
  Edge[e].Temp[0] = -1;
  Edge[e].Temp[1] = -1;

  Edge[n0/2].Prev[n0&1] = Edge[p0/2].Next[p0&1] = e*2;
  Edge[n1/2].Prev[n1&1] = Edge[p1/2].Next[p1&1] = e*2+1;

  if(cf)
  {
    v0 = GetVertId(PrevVertEdge(lc)^1);
    v1 = AddCopiedVert(v0);
    Vert[v1].Sel(dmask,1,dmode);
    Edge[lc/2].Vert[lc&1] = v1;

    if(dv1)
    {
      *dv1 = v0;
      *dv2 = v1;
    }
  }
  else if(dv1)
    *dv1 = -1;

  FixVertCycle(e*2);
  FixVertCycle(e*2+1);

  va = GetVertId(e*2);
  vb = GetVertId(n1);
}

sInt GenMesh::AddEdge(sInt v0,sInt v1,sInt face)
{
  sInt v0f,v1f;
  sInt e,ee;

  v0f = Vert[v0].First;
  v1f = Vert[v1].First;

  if(Edge.Count)
  {
    e = ee = Vert[v1f].Temp2;
    if(e != -1)
    {
      do
      {
        sVERIFY(Vert[Edge[e].Vert[0]].First == v1f);

        if(Edge[e].Face[1] == -1 && Edge[e].Vert[1] == v0f)
        {
          Edge[e].Vert[1] = v0;
          Edge[e].Face[1] = face;
          Face[face].Edge = e*2+1;
          return e*2+1;
        }

        e = Edge[e].Temp[0];
      }
      while(e != ee);
    }
  }

  e = Edge.Count;
  Edge.AtLeast(e+1);
  Edge.Count = e+1;
  Edge[e].Init();
  Edge[e].Vert[0] = v0;
  Edge[e].Vert[1] = v1f;
  Edge[e].Face[0] = face;
  Edge[e].Face[1] = -1;
  
  if(Vert[v0f].Temp2 != -1)
  {
    ee = Vert[v0f].Temp2;
    Edge[e].Temp[0] = Edge[ee].Temp[0];
    Edge[ee].Temp[0] = e;
  }
  else
  {
    Edge[e].Temp[0] = e;
    Vert[v0f].Temp2 = e;
  }

  Face[face].Edge = e*2;
  return e*2;
}

void GenMesh::MakeFace(sInt face,sInt nedges,...)
{
	sInt i,t,e;
	sInt *edges;

	edges = &nedges + 1;
	Face[face].Init();
	Face[face].Edge = edges[0];
	t = edges[nedges-1];
	for(i=0;i<nedges;i++)
	{
		e=edges[i];
		sVERIFY(Edge[e/2].Face[e&1]==-1 || Face[GetFaceId(e)].Material==0);
		Edge[t/2].Next[t&1]=e;
		Edge[e/2].Prev[e&1]=t;
		Edge[e/2].Face[e&1]=face;
		t=e;
	}
}

#if !sINTRO
void GenMesh::Verify()
{
  sInt i,e,ee;
  sInt ec,fc,vc;

  ec = Edge.Count*2;
  vc = Vert.Count;
  fc = Face.Count;

// check face circles

  for(i=0;i<fc;i++)
  {
		if(Face[i].Material==0)
			continue;
    sVERIFY(Face[i].Edge>=0 && Face[i].Edge<ec);
    ee = e = Face[i].Edge;
    do
    {
      sVERIFY(GetFaceId(e)==i);
      e = NextFaceEdge(e);
    }
    while(ee!=e);
    ee = e = Face[i].Edge;
    do
    {
      sVERIFY(GetFaceId(e)==i);
      e = PrevFaceEdge(e);
    }
    while(ee!=e);
  }

// check vertices

  for(i=0;i<vc;i++)
  {
    sVERIFY(Vert[i].Next>=0 && Vert[i].Next<vc);
    sVERIFY(Vert[Vert[i].First].First == Vert[i].First);

    //sVERIFY(VertBuf[i*VertSize].w == 1.0f);
  }

// check edges

  for(i=0;i<Edge.Count;i++)
  {
    sVERIFY(Edge[i].Next[0]>=0 && Edge[i].Next[0]<ec);
    sVERIFY(Edge[i].Next[1]>=0 && Edge[i].Next[1]<ec);
    sVERIFY(Edge[i].Prev[0]>=0 && Edge[i].Prev[0]<ec);
    sVERIFY(Edge[i].Prev[1]>=0 && Edge[i].Prev[1]<ec);
    sVERIFY(Edge[i].Face[0]>=0 && Edge[i].Face[0]<fc);
    sVERIFY(Edge[i].Face[1]>=0 && Edge[i].Face[1]<fc);
    sVERIFY(Edge[i].Vert[0]>=0 && Edge[i].Vert[0]<vc);
    sVERIFY(Edge[i].Vert[1]>=0 && Edge[i].Vert[1]<vc);
    sVERIFY(GetVert(i*2+1)->First == GetVert(NextFaceEdge(i*2  )  )->First);
    sVERIFY(GetVert(i*2  )->First == GetVert(NextFaceEdge(i*2+1)  )->First);
    sVERIFY(GetVert(i*2  )->First == GetVert(PrevFaceEdge(i*2  )^1)->First);
    sVERIFY(GetVert(i*2+1)->First == GetVert(PrevFaceEdge(i*2+1)^1)->First);

    // the following test makes sense, but is currently out because ReadCompact
    // produces slightly broken meshes (no crease, yet double vertices nonetheless)
    /*if(!Edge[i].Crease)
    {
      sVERIFY(GetVertId(i*2  ) == GetVertId(PrevVertEdge(i*2  )));
      sVERIFY(GetVertId(i*2+1) == GetVertId(PrevVertEdge(i*2+1)));
    }*/
  }
}
#endif

void GenMesh::ReIndex()
{
  sInt i,j,vc,ec,cc;

  vc = Vert.Count;
  for(i=0;i<vc;i++)
  {
    if(Vert[i].ReIndex==i)
    {
      Vert[i].First = Vert[Vert[i].First].ReIndex;
      Vert[i].Next = Vert[Vert[i].Next].ReIndex;
    }
    else
    {
      Vert[i].First = Vert[i].Next = i;
      Vert[i].Mask = Vert[i].Select = 0;
    }
  }

  ec = Edge.Count;
  for(i=0;i<ec;i++)
  {
    Edge[i].Vert[0] = Vert[Edge[i].Vert[0]].ReIndex;
    Edge[i].Vert[1] = Vert[Edge[i].Vert[1]].ReIndex;
  }

  cc = Coll.Count;
  for(i=0;i<cc;i++)
    for(j=0;j<8;j++)
      Coll[i].Vert[j] = Vert[Coll[i].Vert[j]].ReIndex;

  for(i=0;i<vc;i++)
    Vert[i].ReIndex = i;
}

sBool GenMesh::IsBorderEdge(sU32 i,sInt mode)
{
  sBool s0,s1;

  s0 = GetFace(i)->Select;
  s1 = GetFaceI(i)->Select;
  if((Edge[i/2].Crease & sGMF_NORMALS) || mode)
    s1 = 0;

  return s0 && !s1;
}

/****************************************************************************/
/****************************************************************************/

void GenMesh::Mask2Sel(sU32 mask)
{
  sInt i;

  if(mask&0x00ff0000)
    for(i=0;i<Vert.Count;i++)
      Vert[i].Select = ((Vert[i].Mask & (mask>>16))!=0);
  if(mask&0x0000ff00)
    for(i=0;i<Face.Count;i++)
      Face[i].Select = ((Face[i].Mask & (mask>> 8))!=0);
  if(mask&0x000000ff)
    for(i=0;i<Edge.Count;i++)
      Edge[i].Select = ((Edge[i].Mask & (mask    ))!=0);
}

void GenMesh::All2Sel(sBool sel,sInt mask)
{
  sInt i;

  if(mask&MAS_VERT)
    for(i=0;i<Vert.Count;i++)
      Vert[i].Select = sel;
  if(mask&MAS_FACE)
    for(i=0;i<Face.Count;i++)
      Face[i].Select = sel && Face[i].Material;
  if(mask&MAS_EDGE)
    for(i=0;i<Edge.Count;i++)
      Edge[i].Select = sel;
}

void GenMesh::Id2Mask(sU32 mask,sInt id)
{
  sFatal("not implememted");
}

void GenMesh::Sel2Mask(sU32 dmask,sInt dmode)
{
  sInt i;

  if(dmask&0x00ff0000)
    for(i=0;i<Vert.Count;i++)
      Vert[i].Sel(dmask,Vert[i].Select,dmode);
  if(dmask&0x0000ff00)
    for(i=0;i<Face.Count;i++)
      Face[i].Sel(dmask,Face[i].Select,dmode);
  if(dmask&0x000000ff)
    for(i=0;i<Edge.Count;i++)
      Edge[i].Sel(dmask,Edge[i].Select,dmode);
}

void GenMesh::All2Mask(sU32 dmask,sInt dmode)
{
  sInt i;

  for(i=0;i<Vert.Count;i++)
  {
    if(dmask&0x00ff0000)
      Vert[i].Sel(dmask,1,dmode);
  }
  for(i=0;i<Face.Count;i++)
  {
    if(Face[i].Material && (dmask&0x0000ff00))
      Face[i].Sel(dmask,1,dmode);
  }
  for(i=0;i<Edge.Count;i++)
  {
    if(dmask&0x000000ff)
      Edge[i].Sel(dmask,1,dmode);
  }
}

void GenMesh::Face2Vert()
{
  sInt i,e,ee;

  All2Sel(0,MAS_VERT);
  for(i=0;i<Face.Count;i++)
  {
    if(Face[i].Select)
    {
      e = ee = Face[i].Edge;
      do
      {
        GetVert(e)->Select = 1;
        e = NextFaceEdge(e);
      }
      while(e!=ee);
    }
  }
}

void GenMesh::Edge2Vert(sInt uvs)
{
	sInt i,j;

  j = VertMap(sGMI_UV0);
	for(i=0;i<Edge.Count*2;i++)
	{
		if(Edge[i/2].Select)
		{
      if((&VertBuf[GetVertId(PrevFaceEdge(i))*VertSize()+j].x)[uvs]>0.5f)
      {
        GetVert(i)->Select=1;
        GetVert(NextFaceEdge(i))->Select=1;
      }
		}
	}
}

void GenMesh::Vert2FaceEdge()
{
  sInt i,e,ee;
  sBool all;

  for(i=0;i<Edge.Count;i++)
    Edge[i].Select = GetVert(i*2+0)->Select && GetVert(i*2+1)->Select;
  
  for(i=0;i<Face.Count;i++)
  {
		if(!Face[i].Material)
			continue;
    e = ee = Face[i].Edge;
    all = sTRUE;
    do
    {
      if(!GetVert(e)->Select)
        all = 0;
      e = NextFaceEdge(e);
    }
    while(ee!=e && all);

    Face[i].Select = all;
  }
}

void GenMesh::CalcNormal(sVector &n,sInt v0,sInt v1,sInt v2)
{
  sVector t1,t2;

  t1.Sub3(VertBuf[v1 * VertSize()],VertBuf[v0 * VertSize()]);
  t2.Sub3(VertBuf[v2 * VertSize()],VertBuf[v0 * VertSize()]);
  n.Cross3(t1,t2);
}

void GenMesh::CalcFaceNormal(sVector &n,sInt e)
{
  CalcNormal(n,GetVertId(PrevFaceEdge(e)),GetVertId(e),
    GetVertId(NextFaceEdge(e)));
}

void GenMesh::CalcFaceNormalAccurate(sVector &n,sInt f)
{
  sInt v0,e,ee,ne;
  sF32 l;

  e = ee = Face[f].Edge;
  v0 = GetVertId(PrevFaceEdge(e))*VertSize();

  do
  {
    ne = NextFaceEdge(e);
    CalcNormal(n,GetVertId(PrevFaceEdge(e)),GetVertId(e),GetVertId(ne));
    e = ne;
  }
  while((l = n.Dot3(n)) > 1e-40f && e != ee);

  if(l >= 1e-40f)
    n.Scale3(sFInvSqrt(l));
  else
    n.Init3(1,0,0);
}

void GenMesh::CalcFacePlane(sVector &p,sInt f)
{
  CalcFaceNormalAccurate(p,f);
  p.w = -p.Dot3(VertPos(GetVertId(Face[f].Edge)));
}

void GenMesh::CalcFaceCenter(sVector &m,sInt f)
{
  sInt e,ee,cnt;
  
  e = ee = Face[f].Edge;
  m.Init(0,0,0,1);
  cnt = 0;

  do
  {
    m.Add3(VertPos(GetVertId(e)));
    cnt++;
    e = NextFaceEdge(e);
  }
  while(e!=ee);
  m.Scale3(1.0f / cnt);
}

void GenMesh::CalcBBox(sAABox &box)
{
  sInt i;

  All2Sel(1,MAS_FACE);
  Face2Vert();

  box.Min.Init( 1e+15f,  1e+15f,  1e+15f, 1.0f);
  box.Max.Init(-1e+15f, -1e+15f, -1e+15f, 1.0f);
  for(i=0;i<Vert.Count;i++)
  {
    if(Vert[i].Select)
    {
      const sVector &v = VertBuf[i * VertSize()];
      box.Min.x = sMin(box.Min.x,v.x);
      box.Min.y = sMin(box.Min.y,v.y);
      box.Min.z = sMin(box.Min.z,v.z);
      box.Max.x = sMax(box.Max.x,v.x);
      box.Max.y = sMax(box.Max.y,v.y);
      box.Max.z = sMax(box.Max.z,v.z);
    }
  }
}

void GenMesh::CopyVert(sInt dest,sInt src)
{
  sVector *vdst = VertBuf + dest * VertSize();
  sVector *vsrc = VertBuf + src * VertSize();

  sCopyMem4((sU32 *)vdst,(sU32 *)vsrc,VertSize() * 4);
}

/****************************************************************************/
/****************************************************************************/

void GenMesh::TransVert(sMatrix &mat,sInt sj,sInt djj)
{
  sInt dj;

  sj = VertMap(sj); if(sj==-1) return;
  dj = VertMap(djj&0x0f); if(dj==-1) return;
  if(djj&0x10)
    dj |= 0x10;

  sInt col = VertMap(sGMI_COLOR0);

  for(sInt i=0;i<Vert.Count;i++)
  {
    if(Vert[i].Select)
    {
      sVector *v = VertBuf + i * VertSize();
      sF32 sx,sy,sz,sw;
      sF32 dx,dy,dz;

      sx = v[sj].x; sy = v[sj].y; sz = v[sj].z;
      sw = (sj == col) ? 1.0f : v[sj].w;

      dx = sx*mat.i.x + sy*mat.j.x + sz*mat.k.x + sw*mat.l.x;
      dy = sx*mat.i.y + sy*mat.j.y + sz*mat.k.y + sw*mat.l.y;
      dz = sx*mat.i.z + sy*mat.j.z + sz*mat.k.z + sw*mat.l.z;

      if(dj & 0x10)
      {
        sInt ind = dj & 0x0f;

        v[ind].x += dx;
        v[ind].y += dy;
        v[ind].z += dz;
      }
      else
      {
        v[dj].x = dx;
        v[dj].y = dy;
        v[dj].z = dz;
        v[dj].w = v[sj].w;
      }
    }
  }
}

/****************************************************************************/

void GenMesh::SelectCube(const sVector &vc,const sVector &vs,sU32 dmask,sInt dmode)
{
  sInt i;

  for(i=0;i<Vert.Count;i++)
    Vert[i].Select = sFAbs(VertPos(i).x-vc.x)<=vs.x &&
    sFAbs(VertPos(i).y-vc.y)<=vs.y && sFAbs(VertPos(i).z-vc.z)<=vs.z;

  Vert2FaceEdge();
  Sel2Mask(dmask,dmode);
}

/****************************************************************************/

void GenMesh::SelectSphere(const sVector &vc,const sVector &vs,sU32 dmask,sInt dmode)
{
  sInt i,j;
  sF32 r,t;
  sVector *vt;

  for(i=0;i<Vert.Count;i++)
  {
    vt = VertBuf + i * VertSize();
    r = 0.0f;
    for(j=0;j<3;j++)
    {
      t = ((&vt->x)[j] - (&vc.x)[j]) / (&vs.x)[j];
      r += t * t;
    }

    Vert[i].Select = (r <= 1.0f);
  }

  Vert2FaceEdge();
  Sel2Mask(dmask,dmode);
}

/****************************************************************************/

void GenMesh::Ring(sInt segments,sU32 dmask,sF32 radius,sF32 phase,sInt arc)
{
  sInt i,in,ip;
  sInt sj;
  sVector *vp;
  GenMeshEdge *ed;
  sInt count;

  sVERIFY(arc<segments);

  if(arc>0)
    count = segments - arc + 2;
  else
    count = segments;

  Vert.AtLeast(count);
  Edge.AtLeast(count);  Edge.Count=count;
  Face.AtLeast(2);      Face.Count=2;

  Face[0].Init();
  Face[0].Edge = 0;
  Face[1].Init();
  Face[1].Edge = 1;

  sj = VertMap(sGMI_UV0);

  for(i=0;i<count;i++)
  {
    AddNewVert();
    vp = &VertBuf[i*VertSize()];
    if(i==count-1 && arc>0)
    {
      vp->x = 0;
      vp->y = 0;
    }
    else
    {
      vp->x =  sFSin(phase+i*sPI2F/(segments))*radius;
      vp->y = -sFCos(phase+i*sPI2F/(segments))*radius;
    }
    vp->w = 1.0f;
    if(sj!=-1)
    {
      vp[sj].x = i*1.0f/count;
      vp[sj].w = 1.0f;
    }

    in=(i+1)%count;
    ip=(i+count-1)%count;

    ed = &Edge[i];
    ed->Init();
    ed->Next[0] = 2*in;
    ed->Next[1] = 2*ip+1;
    ed->Prev[0] = 2*ip;
    ed->Prev[1] = 2*in+1;
    ed->Face[0] = 0;
    ed->Face[1] = 1;
    ed->Vert[0] = i;
    ed->Vert[1] = in;
  }

  All2Mask(dmask);
}

/****************************************************************************/

void GenMesh::Extrude(sU32 dmask,sInt dmode,sInt mode)
{
  sInt i,e,ec,va,vb,ov,o1,s1;
  
  ec = Edge.Count;
  for(i=0;i<ec*2;i++)
  {
    Edge[i/2].Temp[i&1] = -1;
    if(IsBorderEdge(i,mode))
    {
      Edge[i/2].Temp[i&1] = 0;
      e = i;
      do
      {
        e = NextVertEdge(e);
      }
      while(!IsBorderEdge(e^1,mode));

      ov = va = AddCopiedVert(GetVertId(i));
      SplitBridge(e,i,va,vb,dmask,dmode,&s1,&o1);
      CopyVert(ov,GetVertId(i));

      if(s1!=-1)
        CopyVert(o1,s1);
    }
  }

  for(i=0;i<ec*2;i++)
  {
		if(!Edge[i/2].Temp[i&1])
    {
      SplitFace(NextFaceEdge(NextFaceEdge(i)),PrevFaceEdge(i),0,0);
      Face[Face.Count-1].Mask = 0;
      Face[Face.Count-1].Select = 0;
      Face[Face.Count-1].Sel(dmask,1,dmode);
    }
  }
}

/****************************************************************************/

static sInt MakeSubdMask(sInt mask,sInt vmask)
{
  sInt omask,m;
  omask = 0;
  m = 1;

  while(vmask)
  {
    if(vmask&1)
    {
      if(mask&1)
        omask |= m;

      m <<= 1;
    }

    mask >>= 1;
    vmask >>= 1;
  }

  return omask;
}

void GenMesh::Subdivide(sF32 alpha)
{
  sInt i,j,k,count,e,ee,c,v,vv;
  sInt va0,va1,vb0,vb1,va,vb;
  sInt ec,s0,s1;
  sU32 Temp[128];

  sInt fc = Face.Count;
  sInt ovc = Vert.Count;
  sInt vs = VertSize();

// face.temp    = first new edge
// face.temp2   = center
// edge.temp[0]
// edge.temp[1]
// vert.temp    = duplicated vertex for storing old value
// vert.temp2   = vertex->edge ptr
// calculate center positions

  for(i=0;i<ovc;i++)
  {
    Vert[i].Temp = -1;
    Vert[i].Temp2 = i;
  }

  for(i=0;i<fc;i++)
  {
    Face[i].Temp = -1;
    Face[i].Temp2 = -1;
    
    if(Face[i].Select)
    {
      ee = e = Face[i].Edge;
      sInt c = Face[i].Temp2 = AddNewVert();
      Vert[c].Select = Face[i].Select;
      Vert[c].Mask = Face[i].Mask;

      // clear accu
      sSetMem(&VertBuf[c*vs],0,sizeof(sVector)*vs);
      sInt count = 0;

			do
			{
        // sum up
				v = GetVertId(e);
        sVector *vsrc = VertBuf + v*vs;
        for(sInt j=0;j<vs;j++)
          VertBuf[c*vs + j].Add4(vsrc[j]);

        // process vertices
				if(Vert[v].ReIndex == v)
				{
          vv = AddCopiedVert(v);
					Vert[v].Temp = e;
          Vert[v].Temp2 = v;
					Vert[v].ReIndex = vv;
          Vert[vv].Temp2 = v;
				}

				e = NextFaceEdge(e);
        count++;
			}
      while(e!=ee);

      // scale by 1/count to get middle
      sF32 scale = 1.0f / count;
      for(sInt j=0;j<vs;j++)
        VertBuf[c*vs + j].Scale4(scale);
    }
  }

// even vertices
	for(i=0;i<ovc;i++)
	{
		if(Vert[i].ReIndex!=i && Vert[i].Temp!=-1)
		{
			k = 0;
			e = ee = Vert[i].Temp;
			ec = 0; // edge crease flags
			c = -1;

      // go around 1-ring, collecting vertices and crease flags
			do
			{
				Temp[k++] = GetVertId(e^1);

        if(GetFace(e)->Temp2!=-1)
					Temp[k++] = GetFace(e)->Temp2;
        else
					c = e;

				if(GetVertId(e) == i)
					ec |= Edge[e/2].Crease;

				e = NextVertEdge(e);
			}
			while(e!=ee);

      sInt vs = VertSize();
      sVector *vd = VertBuf + Vert[i].ReIndex * vs;
      sVector *vo = VertBuf + i * vs; // orig

			if(c==-1) // no boundary
			{
        // calc weights
        sF32 w1 = 4.0f * alpha / (k * k);
        sF32 w2 = 1.0f - alpha * 4.0f / k;
        sU32 mask = MakeSubdMask(ec,VertMask());

        // calc vert (attribute by attribute)
        for(j=0;j<vs;j++,mask>>=1)
        {
          if(mask & 1) // crease, just copy
            vd[j] = vo[j];
          else // weight verts accordingly
          {
            vd[j].Scale4(vo[j],w2);
            for(sInt n=0;n<k;n++)
              vd[j].AddScale4(VertBuf[Temp[n] * vs + j],w1);
          }
        }
			}
			else // boundary
			{
        // find verts
        va = GetVertId(c^1);
        do
        {
          c = NextVertEdge(c);
        }
        while(GetFaceI(c)->Select);
        vb = GetVertId(c^1);

        // calc weights
        sF32 w1 = alpha * 0.125f;
        sF32 w2 = 1.0f - 2.0f * w1;

        sVector *vertA = VertBuf + va * vs;
        sVector *vertB = VertBuf + vb * vs;

        // calc vert (attribute by attribute)
        for(j=0;j<vs;j++)
        {
          vd[j].Add4(vertA[j],vertB[j]);
          vd[j].Scale4(w1);
          vd[j].AddScale4(vo[j],w2);
        }
			}
		}
	}

// split edges (calc weights here because they're the same everywhere)
  sF32 w1 = alpha * 0.25f;
  sF32 w2 = 0.5f - w1;

  ec = Edge.Count;
  for(i=0;i<ec*2;i+=2)
  {
    // split if select
    s0 = GetFace(i)->Select;
    s1 = GetFaceI(i)->Select;
    if(s0 || s1)
    {
      // split edge
      va = AddNewVert();
      Vert[va].Mask = GetVert(i)->Mask | GetVert(SkipFaceEdge(i,2))->Mask;
      SplitBridge(NextVertEdge(i^1),PrevVertEdge(i^1),va,vb,0x100000,0);

      vb0 = GetVert(PrevFaceEdge(i^1))->Temp2;
      vb1 = GetVert(NextFaceEdge(i^1))->Temp2;
      va0 = GetVert(i)->Temp2;
      va1 = GetVert(NextFaceEdge(NextFaceEdge(i)))->Temp2;
      j = Edge[i/2].Crease;
      sVERIFY(va0!=-1 && va1!=-1 && vb0!=-1 && vb1!=-1);

      // calc boundary mask and vertices to weight in
      sInt mask = (s0 && s1) ? MakeSubdMask(~j,VertMask()) : 0;
      sInt vc0 = GetFace(i)->Temp2;
      sInt vc1 = GetFaceI(i)->Temp2;

      sInt vs = VertSize();
      sVector *vd0 = VertBuf + va * vs;
      sVector *vd1 = VertBuf + vb * vs;

      // perform actual weighting
      for(j=0;j<vs;j++,mask>>=1)
      {
        sVector t0,t1;

        t0.Add4(VertBuf[va0*vs + j],VertBuf[va1*vs + j]);
        if(vd0 != vd1)
          t1.Add4(VertBuf[vb0*vs + j],VertBuf[vb1*vs + j]);

        if(mask & 1)
        {
          vd0[j].Scale4(t0,w2);
          if(vd0 != vd1)
            vd1[j].Scale4(t1,w2);

          t0.Add4(VertBuf[vc0*vs + j],VertBuf[vc1*vs + j]);
          vd0[j].AddScale4(t0,w1);
          if(vd0 != vd1)
            vd1[j].AddScale4(t0,w1);
        }
        else
        {
          vd0[j].Scale4(t0,0.5f);
          if(vd0 != vd1)
            vd1[j].Scale4(t1,0.5f);
        }
      }

      // store link to correct edge for face generation
      if(s0)
        GetFace(i)->Temp = i;

      if(s1)
        GetFaceI(i)->Temp = PrevFaceEdge(i^1);
    }
  }

// create faces
  fc = Face.Count;
  for(i=0;i<fc;i++)
  {
		if(Face[i].Select)
    {
      sVERIFY(Face[i].Temp!=-1);
      sVERIFY(Face[i].Temp2!=-1);
      e = Face[i].Temp;

      count=0;
      ee = e;
      do
      {
        ee = NextFaceEdge(ee);
        sVERIFY(count<64);
        Temp[count++] = ee;
        ee = NextFaceEdge(ee);
      }
      while(e!=ee);

      va = Face[i].Temp2;
      SplitBridge(NextVertEdge(Temp[0]),Temp[0],va,vb,0,0);
      for(j=1;j<count;j++)
        SplitFace(PrevFaceEdge(PrevFaceEdge(PrevFaceEdge(Temp[j]))),Temp[j]);
    }
  }

  ReIndex();
}

/****************************************************************************/

#if 0

static sU32 *BoneInner(GenMesh *mesh,sU32 *data,sMatrix *matrices,KObject **objects)
{
  sU32 count,i;
  sVector *vec;
  sF32 dx,dy,dz;

  count = *data++;
	while(count--)
	{
    vec = mesh->VertBuf + *data++;

    dx = dy = dz = 0.0f;
		for (i = 0; i < 4; i++)
		{
			sF32 weight = (sF32&) *data++;
			sMatrix* mtx = matrices + *data++;

			dx+=weight*(vec->x*mtx->i.x+vec->y*mtx->j.x+vec->z*mtx->k.x+mtx->l.x);
			dy+=weight*(vec->x*mtx->i.y+vec->y*mtx->j.y+vec->z*mtx->k.y+mtx->l.y);
			dz+=weight*(vec->x*mtx->i.z+vec->y*mtx->j.z+vec->z*mtx->k.z+mtx->l.z);
		}

    vec->x = dx;
    vec->y = dy;
    vec->z = dz;
	}

	return data;
}

sInt GenMesh::Bones(sF32 phase)
{
#if !sINTRO
	sU32 *data,*cptr;
  sF32 wgt;
  sInt i,j,mi,lm=0;

	data = RecBegin(BoneInner);

  sVERIFY(RBIndex+BoneMatrix.Count<1024);
  BonesModify(-1,phase);
  mi = RBIndex;
  RBIndex += BoneMatrix.Count;

  cptr = data; *data++ = 0;

  for(i=0;i<Vert.Count;i++)
  {
    if(Vert[i].Matrix[0] != 0xff)
    {
      *cptr += 1;
      *data++ = i * VertSize();
      for(j=0;j<4;j++)
      {
        if(Vert[i].Matrix[j]!=0xff)
        {
          wgt = Vert[i].Weight[j]/255.0f;
          lm = Vert[i].Matrix[j]+mi;
        }
        else
          wgt = 0.0f;

        *data++ = *((sU32 *) &wgt);
        *data++ = lm;
      }
    }
  }

  RecEnd(data);
  return mi;
#else
  return 0;
#endif
}

void GenMesh::BonesModify(sInt matrix,sF32 phase)
{
#if !sINTRO
  sMatrix *mp;
  sInt i;
  sInt ki0,ki1;
  sF32 f1,val;
  sMatrix mat;
  GenMeshCurve *cur;

  if(matrix<0)
  {
    mp = &RBMat[RBIndex];
    sVERIFY(CurveCount==BoneCurve.Count);
  }
  else
  {
    mp = &RecMat.Array[matrix];
    sVERIFY(matrix+BoneMatrix.Count <= RecMat.Count);
  }

  if(KeyCount>0 && BoneCurve.Count>0)
  {
    i = (Anim0 + phase*(Anim1-Anim0))*1024;
    ki0 = (i/1024)%KeyCount;
    ki1 = (i/1024+1)%KeyCount;
    i &= 1023;
    f1 = i/1024.0f;

    cur = BoneCurve.Array;
    for(i=0;i<BoneCurve.Count;i++)
    {
      val = KeyBuf[i*KeyCount+ki0] + (KeyBuf[i*KeyCount+ki1]-KeyBuf[i*KeyCount+ki0]) * f1;
      BoneMatrix[cur->Matrix].TransSRT[cur->Curve] = val;

      cur++;
    }
  }

  BoneMatrix[0].Matrix.InitSRT(BoneMatrix[0].TransSRT);
  mat.InitSRTInv(BoneMatrix[0].BaseSRT);
  mp[0].MulA(mat,BoneMatrix[0].Matrix);
  
  for(i=1;i<BoneMatrix.Count;i++)
  {
    mat.InitSRT(BoneMatrix[i].TransSRT);
    BoneMatrix[i].Matrix.MulA(mat,BoneMatrix[BoneMatrix[i].Parent].Matrix);
    mat.InitSRTInv(BoneMatrix[i].BaseSRT);
    mp[i].MulA(mat,BoneMatrix[i].Matrix);
  }

  for(i=0;i<BoneMatrix.Count;i++)
  {
    mp[i].i.z = -mp[i].i.z;
    mp[i].j.z = -mp[i].j.z;
    mp[i].k.z = -mp[i].k.z;
    mp[i].l.z = -mp[i].l.z;
  }
#endif
}

#endif

/****************************************************************************/

void GenMesh::FixVertCycle(sInt i)
{
  sInt v,v0,vn,e;

  v0 = v = GetVert(i)->ReIndex;
  Vert[v].First = v0;
  Vert[v].Next = v0;
  e = i;

  do
  {
    Edge[e/2].Vert[e&1] = v;
    e = NextVertEdge(e);
    vn = GetVert(e)->ReIndex;

    if(Edge[e/2].Crease)
    {
      if(vn!=v && vn!=v0)
      {
        Vert[v].Next = vn;
        Vert[vn].First = v0;
        Vert[vn].Next = v0;
      }

      v = vn;
    }
  }
  while(e!=i);
}

void GenMesh::Crease(sInt selType,sU32 dmask,sInt dmode,sInt what)
{
  sInt i,j,k,e,got,v0,v,vf;
  sBool sel;

  sVERIFY(!(what & 1));

  for(i=0;i<Edge.Count*2;i++)
  {
    if(selType==0)
      sel = GetFace(i)->Select && !GetFaceI(i)->Select;
    else if(selType==1)
      sel = Edge[i/2].Select;
    else
      sel = GetFace(i)->Select;

    if(sel)
    {
	    if(!Edge[i/2].Crease)
	    {
		    for(j=0;j<2;j++)
		    {
			    k = i^j;
			    v0 = GetVertId(k);
			    vf = Vert[v0].First;
          
          got = 0;
          e = k;
          do
          {
            got |= Edge[e/2].Crease;
            e = NextVertEdge(e);
          }
          while(e!=k);

			    if(got) // already atleast one crease on this vertex
			    {
            v = AddCopiedVert(v0);
            Vert[v].Sel(dmask,1,dmode);
				    Vert[v].First = vf;
				    Vert[v0].Next = v;
            CopyVert(v,v0);

				    do
				    {
					    sVERIFY(GetVertId(k)==v0);
					    Edge[k/2].Vert[k&1] = v;
					    k = NextVertEdge(k);
				    }
				    while(!Edge[k/2].Crease);
			    }
		    }
	    }

	    Edge[i/2].Crease|=what; // update crease flags
    }
  }
}

void GenMesh::UnCrease(sBool edge,sInt what)
{
	sInt i;
	sBool sel;

	sVERIFY(!(what & 1));

	for(i=0;i<Edge.Count;i++)
	{
		if(edge)
			sel = Edge[i].Select;
		else
      sel = IsBorderEdge(i*2,0);

		if(sel)
			Edge[i].Crease &= ~what;
	}

	// todo: fix vertices too
}

/****************************************************************************/

void GenMesh::CalcNormals(sInt type,sInt calcWhat)
{
	sInt i,e,ee,component;
	sBool ok;

	sInt msk = type ? 0 : sGMF_NORMAL;

  sInt vs = VertSize();
  sInt sn = VertMap(sGMI_NORMAL);
  sInt st = VertMap(sGMI_TANGENT);
  sInt su = VertMap(sGMI_UV0);

  // first build vertex->edge links in Temp
	for(i=0;i<Vert.Count;i++)
		Vert[i].Temp = -1;

	for(i=0;i<Edge.Count*2;i++)
		Vert[GetVertId(i)].Temp = i;

	// write data. format: count, face offsets, dest vertex offset.
  for(component=0;component<2;component++)
  {
    msk = type ? 0 : (component ? sGMF_UV0 : sGMF_NORMAL);

    if(calcWhat & (1 << component))
    {
	    for(i=0;i<Vert.Count;i++)
	    {
        if(Vert[i].Temp==-1)
          continue;

		    // find "left border" (finds a crease or just a full loop)
        e = ee = Vert[i].Temp;
        do
        {
          if(Edge[e/2].Crease & msk)
            ee = e;
          else
            e = PrevVertEdge(e);
        }
        while(e!=ee);
        
        ee = e;
        ok = (calcWhat & 4 || GetFace(e)->Select) ? sTRUE : sFALSE;

        // accumulate on this side of the crease
        // (the loop construction is absolutely awful, fix that somehow)
        sVector accu,t,t1,t2,*v0,*v1,*v2;
        sF32 t1l,t2l;
        sBool wasExit,exit=sFALSE;

        v0 = VertBuf + GetVertId(e) * vs;
        v2 = VertBuf + GetVertId(NextFaceEdge(e)) * vs;
        
        accu.Init4(0,0,0,0);
        t2.Sub3(*v2,*v0);
        t2l = t2.Dot3(t2);

        do
        {
          wasExit = exit;

          // go to next vert, cycle variables
          if(GetFace(e)->Select)
            ok = sTRUE;

          t1 = t2;
          t1l = t2l;
          v1 = v2;
          if(!wasExit)
            v2 = VertBuf + GetVertId(NextFaceEdge(e)) * vs;
          else
            v2 = VertBuf + GetVertId(PrevFaceEdge(PrevVertEdge(e))) * vs;

          t2.Sub3(*v2,*v0);
          t2l = t2.Dot3(t2);

          if(component == 0) // calc normal
          {
            if(t1l * t2l > 1e-20f)
            {
              t.Cross3(t1,t2);
              accu.AddScale3(t,1.0f/(t1l * t2l));
            }
          }
          else // calc tangent
          {
            sF32 c1 = v0[su].y - v1[su].y;
            sF32 c2 = v2[su].y - v0[su].y;
            sF32 ix = (v1[su].x - v0[su].x) * c2 + (v2[su].x - v0[su].x) * c1;

            if(sFAbs(ix) > 1e-20f)
            {
              ix = 1.0f / ix;
              t.Scale3(t1,c2*ix);
              t.AddScale3(t2,c1*ix);
              if(t.Dot3(t) > 1e-20f)
              {
                t.Unit3();
                accu.Add3(t);
              }
            }
          }

          e = NextVertEdge(e);
          exit = e == ee || (Edge[e/2].Crease & msk);
        }
        while(!wasExit);

		    if(ok) // write back
		    {
          sVector *vd = VertBuf + i * vs;

          if(component == 0) // normal
          {
            accu.UnitSafe3();
            vd[sn] = accu;
          }
          else // tangent
          {
            // remove component parallel to normal first
            accu.AddScale3(vd[sn],-accu.Dot3(vd[sn]));
            accu.UnitSafe3();
            vd[st] = accu;
          }
		    }
	    }
    }
  }
}

void GenMesh::NeedAllNormals()
{
  if(!GotNormals)
  {
    CalcNormals(0,7);
    GotNormals = sTRUE;
  }
}

/****************************************************************************/

void GenMesh::Triangulate(sInt threshold,sU32 dmask,sInt dmode,sInt type)
{
	sInt i,j,c,v,e,ee;
	sInt Edgei[256];
	sU32 Temp[256];

	for(i=0;i<Face.Count;i++)
	{
		if(Face[i].Select)
		{
			e = ee = Face[i].Edge;
			c = 0;

			do
			{
				sVERIFY(c < 256);
				Edgei[c] = e;
				Temp[c++] = GetVertId(e);
				e = NextFaceEdge(e);
			}
			while(e != ee);

			if(c>=threshold)
			{
        switch(type)
        {
        case 0:
          {
				    v = AddNewVert();
            Vert[v].Sel(dmask,1,dmode);

            sVector *vd = VertBuf + v * VertSize();
            sSetMem(vd,0,sizeof(sVector) * VertSize());
            
            sF32 ic = 1.0f / c;

				    for(j=0;j<c;j++)
				    {
              sVector *vs = VertBuf + Temp[j] * VertSize();

              // add weighted
              for(sInt k=0;k<VertSize();k++)
                vd[k].AddScale4(vs[k],ic);

              // change connectivity
					    if(j==0)
                SplitBridge(NextVertEdge(Edgei[0]),Edgei[0],v,e,dmask,dmode);
					    else
						    SplitFace(PrevFaceEdge(PrevFaceEdge(Edgei[j])),Edgei[j],dmask,dmode);
				    }
          }
          break;

        case 1:
          for(j=0;j<c-3;j++)
            SplitFace(PrevFaceEdge(PrevFaceEdge(Edgei[j])),Edgei[j],dmask,dmode);
          break;

        case 2:       // special hack for tesselating cylinders with arc
          {
            sInt start = 0;
            for(j=0;j<c;j++)      // find the vertex that is "nearest". that's our start point
            {
              if((VertBuf + Temp[j] * VertSize())->z < (VertBuf + Temp[start] * VertSize())->z)
                start = j;
            }
            if(start!=0)          // lower deckel
            {
              for(j=start+3;j<start+c;j++)
                SplitFace(PrevFaceEdge(PrevFaceEdge(Edgei[j%c])),Edgei[j%c],dmask,dmode);
            }
            else                  // upper deckel
            {
              for(j=start+1;j<start+c-2;j++)
                SplitFace(PrevFaceEdge(PrevFaceEdge(Edgei[j%c])),Edgei[j%c],dmask,dmode);
            }
          }
          break;
        }
			}
		}
	}
}

void GenMesh::Cut(const sVector &plane,sInt mode)
{
	sInt i,j,va0,va1,vb0,vb1,ec,va,vb,e,ee,pe,noe,noet;
	sF32 t0,t1;
	sBool force;

	// classify vertices
	for(i=0;i<Vert.Count;i++)
	{
		t0 = VertPos(i).Dot4(plane);
		Vert[i].Temp = *((sU32 *) &t0);
	} 

	// clip edges
	ec = Edge.Count;
	for(i=0;i<ec*2;i+=2)
	{
		va0 = GetVertId(i);
		va1 = GetVertId(NextFaceEdge(i));
		vb1 = GetVertId(i^1);
		vb0 = GetVertId(NextFaceEdge(i^1));

		if ((Vert[va0].Temp ^ Vert[vb1].Temp) < 0) // edge crosses plane?
		{
			t0 = *((sF32 *) &Vert[va0].Temp);
			t1 = *((sF32 *) &Vert[vb1].Temp) - t0;

			if (fabs(t1) > 1e-5f) // avoid precision problems
			{
        va = AddNewVert();

        SplitBridge(NextVertEdge(i^1),PrevVertEdge(i^1),va,vb);
				t0 = -t0 / t1;
        
        for(j=0;j<VertSize();j++)
          VertBuf[va*VertSize()+j].Lin3(VertBuf[va0*VertSize()+j],VertBuf[va1*VertSize()+j],t0);
				Vert[va].Temp = 0;

				if(va!=vb)
				{
          for(j=0;j<VertSize();j++)
            VertBuf[vb*VertSize()+j].Lin3(VertBuf[vb0*VertSize()+j],VertBuf[vb1*VertSize()+j],t0);
					Vert[vb].Temp = 0;
				}
			}
		}
	}

	for(i=0;i<Edge.Count*2;i+=2)
	{
		GetEdge(i)->Temp[0] = GetVert(i)->Temp >= 0 && GetVert(i^1)->Temp >= 0;
		GetEdge(i)->Temp[1] = 0;
	}

	// fix faces
	noe = 0;
	for(i=0;i<Face.Count;i++)
	{
		ec = 0;
		e = Face[i].Edge;
		ee = PrevFaceEdge(e);
		pe = -1;

		do
		{
			force = sFALSE;

			if(GetEdge(e)->Temp[0]) // edge is on right side of plane?
			{
				if(pe==-1)
				{
					ee = e;
					force = sTRUE;
				}
				else if(pe!=PrevFaceEdge(e)) // need to add new edge
				{
					Edge.AtLeast(Edge.Count+1);
					Edge[Edge.Count].Init();
					Edge[Edge.Count].Vert[0] = GetVertId(NextFaceEdge(pe));
					Edge[Edge.Count].Vert[1] = GetVertId(e);
					Edge[Edge.Count].Next[0] = e;
					Edge[Edge.Count].Prev[0] = pe;
					Edge[Edge.Count].Face[0] = i;
					Edge[Edge.Count].Temp[0] = 0;
					Edge[Edge.Count].Temp[1] = 1;
					Edge[e/2].Prev[e&1] = Edge.Count*2;
					Edge[pe/2].Next[pe&1] = Edge.Count*2;
					e = Edge.Count*2;
					Edge.Count++;
					noe++; // number of open edges
				}

				Face[i].Edge = e;
				pe = e;
				ec++;
			}

			e = NextFaceEdge(e);
		}
		while(PrevFaceEdge(e) != ee || force);

		if(ec<3)
		{
			Face[i].Material=0;
			Face[i].Select=0;
			Face[i].Mask=0;
		}
	}

	// close mesh
	noet = noe;
	while(noe)
	{
		// find first open edge
		i = Edge.Count - noet;
		while(!Edge[i].Temp[1])
			i++;
		sVERIFY(i<Edge.Count);

		// prepare new face
		Face.AtLeast(Face.Count+1);
		Face[Face.Count].Init();
		Face[Face.Count].Edge = i*2+1;
		Face[Face.Count].Material = mode ? Face[Edge[i].Face[0]].Material : 0;
		Face.Count++;

		e = ee = i*2+1;

		// add open edges till face is closed
		do
		{
			GetEdge(e)->Temp[1] = 0;
			sVERIFY(noe>0);
			noe--;

			for(i=(Edge.Count-noet)*2;i<Edge.Count*2;i++)
			{
				if((GetEdge(i)->Temp[1] || i==ee) && Vert[GetVertId(i)].First==Vert[GetVertId(e^1)].First)
					break;
			}
			sVERIFY(i!=Edge.Count*2);

			Edge[e/2].Next[e&1]=i;
			Edge[i/2].Prev[i&1]=e;
			Edge[i/2].Face[i&1]=Face.Count-1;
			e=i;
		}
		while(e!=ee);
	}

	CleanupMesh();
}

void GenMesh::CleanupMesh()
{
	sInt i,j,fc,ec,e,ee;
	sInt *remape,*remapf;

	// cleanup faces and mark+renumber edges
	fc = 0;
	ec = 0;
	remape = new sInt[Edge.Count];
	remapf = new sInt[Face.Count];

	// prepare edges
	for(i=0;i<Edge.Count;i++)
		Edge[i].Used = 0;

	// mark+renumber faces
	for(i=0;i<Face.Count;i++)
	{
		if (Face[i].Material != -1)
		{
			e = ee = Face[i].Edge;

			do
			{
				Edge[e/2].Used = 1;
				e = NextFaceEdge(e);
			}
			while(e != ee);

			remapf[i] = fc;
			Face[fc] = Face[i];
			fc++;
		}
	}

	// mark+renumber edges
	for(i=0;i<Edge.Count;i++)
	{
		if(Edge[i].Used)
		{
			remape[i] = ec*2;
			Edge[ec] = Edge[i];
			ec++;
		}
	}
	
	Face.Count = fc;
	Edge.Count = ec;

	// fix links
	for(i=0;i<ec;i++)
	{
		for(j=0;j<4;j++)
		{
			e = Edge[i].Next[j];
			Edge[i].Next[j] = remape[e/2] | (e&1);
		}

		for(j=0;j<2;j++)
			Edge[i].Face[j] = remapf[Edge[i].Face[j]];
	}

	for(i=0;i<fc;i++)
	{
		e = Face[i].Edge;
		Face[i].Edge = remape[e/2] | (e&1);
	}

	delete[] remape;
	delete[] remapf;
}

/****************************************************************************/

void GenMesh::ExtrudeNormal(sF32 distance)
{
  sMatrix mat;
  
  mat.Init();
  mat.i.x = distance;
  mat.j.y = distance;
  mat.k.z = distance;
  TransVert(mat,sGMI_NORMAL,sGMI_POS | 0x10);
}

/****************************************************************************/

void GenMesh::Displace(GenBitmap *bitmap,sF32 amplx,sF32 amply,sF32 amplz)
{
  BilinearContext ctx;

  // prepare bilinear samples
  BilinearSetup(&ctx,bitmap->Data,bitmap->XSize,bitmap->YSize,0);
  sF32 xScale = bitmap->XSize * 65536.0f;
  sF32 yScale = bitmap->YSize * 65536.0f;

  sInt su = VertMap(sGMI_UV0);
  sInt sn = VertMap(sGMI_NORMAL);

  for(sInt i=0;i<Vert.Count;i++)
  {
    if(Vert[i].Select)
    {
      // loop around vertex in a first pass, accumulating direction
      sInt v = i;
      sInt count = 0;
      sVector disp;

      disp.Init(0,0,0,0);

      do
      {
        sVector *vert = VertBuf + v * VertSize();
        sU16 height[4];

        // sample texture
        BilinearFilter(&ctx,(sU64 *)height,vert[su].x * xScale,vert[su].y * yScale);
        sF32 h = (height[0] - 16384.0f) * (1.0f / 32767.0f);

        // accumulate displacement vector
        disp.x += vert[sn].x * h * amplx;
        disp.y += vert[sn].y * h * amply;
        disp.z += vert[sn].z * h * amplz;

        // continue to next vertex
        Vert[v].Select = 0;
        v = Vert[v].Next;
        count++;
      }
      while(v != i);

      // scale displacement vector accordingly, then displace the verts
      disp.Scale3(1.0f / count);

      do
      {
        VertBuf[v * VertSize()].Add3(disp);
        v = Vert[v].Next;
      }
      while(v != i);
    }
  }
}

/****************************************************************************/

void GenMesh::Bevel(sF32 elevate,sF32 pullin,sInt mode,sU32 dmask,sInt dmode)
{
	sInt i,v,vf,vn,e;
  sInt sp[5];
	static sInt items[5] = { sGMI_POS,sGMI_UV0,sGMI_UV1,sGMI_UV2,sGMI_UV3 };

	Extrude(dmask,dmode,mode);

  sInt vs = VertSize();
  sInt sn = VertMap(sGMI_NORMAL);
  for(i=0;i<5;i++)
    sp[i] = VertMap(items[i]);

	for(i=0;i<Edge.Count*2;i++)
    Edge[i/2].Temp[i&1] = IsBorderEdge(i,mode) ? 0 : -1;

	for(i=0;i<Edge.Count*2;i++)
	{
		if(Edge[i/2].Temp[i&1]!=-1)
		{
			e=i;
			do
			{
				e=PrevVertEdge(e);
				sVERIFY(e!=i);
			}
			while(Edge[e/2].Temp[~e&1]==-1);

			Edge[i/2].Temp[i&1]=e^1;
		}
	}

	for(i=0;i<Edge.Count*2;i++)
	{
		e=Edge[i/2].Temp[i&1];
		if(e!=-1)
		{
			sVERIFY(Edge[e/2].Temp[e&1]!=-1);
			v = vf = GetVertId(e);

			do
			{
        sInt pv = GetVertId(i);
        sInt nv = GetVertId(Edge[e/2].Temp[e&1]);

        Vert[v].ReIndex = vn = AddCopiedVert(v);
        CopyVert(vn,v);

        sVector *vert = VertBuf + v*vs;
        sVector *dvert = VertBuf + vn*vs;

        for(sInt j=0;j<5;j++)
        {
          sInt sj = sp[j];

          if(sj != -1)
          {
            sVector d0,d1,t;

				    // calculate pull-in vector
				    d0.Sub3(VertBuf[pv*vs + sj],vert[sj]); d0.UnitSafe3();
				    d1.Sub3(VertBuf[nv*vs + sj],vert[sj]); d1.UnitSafe3();
				    t.Add3(d0,d1);

				    if(t.Abs3()<1e-5f)
					    t.Cross3(d1,vert[sn]); // fixme

				    // do it.
				    dvert[sj].x += pullin*t.x;
				    dvert[sj].y += pullin*t.y;
				    dvert[sj].z += pullin*t.z;
          }
        }

				v = Vert[v].Next;
			}
			while(mode==1 && v!=vf);
		}
	}

	ReIndex();

	Face2Vert();
	ExtrudeNormal(elevate);
}

/****************************************************************************/

void GenMesh::Perlin(const sMatrix &mat,const sVector &amp)
{
  sInt oldcw;

  // setup fpu: single precision, round towards neg. infinity
  __asm
  {
    fstcw   [oldcw];
    push    0143fh;
    fldcw   [esp];
    pop     eax;
  }

	for(sInt i=0;i<Vert.Count;i++)
	{
		if(Vert[i].Select)
    {
      sVector *vert = VertBuf + i * VertSize();
      sVector t0,t1,t2;
      sF32 fs[3];
      sInt is[3];

      // rotate input vector, calc sampling points
      t0.Rotate34(mat,*vert);
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

      vert->AddMul3(t0,amp);
    }
	}

  // restore fpu state
  __asm
  {
    fldcw   [oldcw];
  }
}

/****************************************************************************/

sBool GenMesh::Add(GenMesh *other,sInt not_useD_anymore)
{
	sInt i,j,v,e,f,m,c,l,p;
	GenMeshVert *vp;
	GenMeshEdge *ep;
	GenMeshFace *fp;
	GenMeshMtrl *mm;
  GenMeshColl *cp;

	if(VertMask()!=other->VertMask())
		return sFALSE;

	// make space for data

	v=Vert.Count;	Vert.AtLeast(v+other->Vert.Count); Vert.Count+=other->Vert.Count;
	e=Edge.Count;	Edge.AtLeast(e+other->Edge.Count); Edge.Count+=other->Edge.Count;
	f=Face.Count; Face.AtLeast(f+other->Face.Count); Face.Count+=other->Face.Count;
	m=Mtrl.Count; Mtrl.AtLeast(m+other->Mtrl.Count-1); //Mtrl.Count+=other->Mtrl.Count;
  c=Coll.Count; Coll.AtLeast(c+other->Coll.Count); Coll.Count+=other->Coll.Count;
  l=Lgts.Count; Lgts.AtLeast(l+other->Lgts.Count); Lgts.Count+=other->Lgts.Count;
  p=Parts.Count; Parts.AtLeast(p+other->Parts.Count); Parts.Count+=other->Parts.Count;
	Realloc(v+other->Vert.Count);

  // copy vertex info, reindex vertices
	for(i=0;i<other->Vert.Count;i++)
	{
    vp = &Vert[v+i];
		*vp=other->Vert[i];
		vp->Next+=v;
		vp->First+=v;
    vp->ReIndex+=v;
	}

  // copy edge indo, reindex edges

	for(i=0;i<other->Edge.Count;i++)
	{
    ep = &Edge[e+i];
		*ep = other->Edge[i];
		for(j=0;j<2;j++)
		{
			ep->Next[j]+=e*2;
			ep->Prev[j]+=e*2;
			ep->Vert[j]+=v;
			ep->Face[j]+=f;
		}
	}

  // copy materials which are not unique

  other->Mtrl[0].Remap = 0;
  for(i=1;i<other->Mtrl.Count;i++)
  {
    for(j=1;j<Mtrl.Count;j++)
    {
      if(Mtrl[j].Material == other->Mtrl[i].Material &&
         Mtrl[j].Pass == other->Mtrl[i].Pass)
      {
        other->Mtrl[i].Remap = j;
        goto nextmtrl;
      }
    }
    other->Mtrl[i].Remap = Mtrl.Count;
    mm = Mtrl.Add();
    mm->Material = other->Mtrl[i].Material;
    if(mm->Material)
      mm->Material->AddRef();
    mm->Pass = other->Mtrl[i].Pass;

    nextmtrl:;
  }

  // copy faces, reindex faces, assign new materials
   
	for(i=0;i<other->Face.Count;i++)
	{
    fp = &Face[f+i];
		*fp=other->Face[i];
		fp->Edge+=e*2;
		fp->Material = other->Mtrl[fp->Material].Remap;
	}

  // copy collision

  for(i=0;i<other->Coll.Count;i++)
  {
    cp = &Coll[c+i];
    for(j=0;j<8;j++)
      cp->Vert[j] = other->Coll[i].Vert[j]+v;
    cp->Mode = other->Coll[i].Mode;
    cp->Logic = other->Coll[i].Logic;
  }

  // copy lightslots
  for(i=0;i<other->Lgts.Count;i++)
    Lgts[l+i] = other->Lgts[i]+v;

  // copy parts
  for(i=0;i<other->Parts.Count;i++)
    Parts[p+i] = other->Parts[i]+f;

	// add vert buffers
  sCopyMem(VertBuf + v * VertSize(),other->VertBuf,other->Vert.Count * VertSize() * sizeof(sVector));

  // a freshly added mesh doesn't have a pivot!
  Pivot = -1;
  UnPrepare();
 
	return sTRUE;
}

/****************************************************************************/

void GenMesh::DeleteFaces()
{
	sInt i;

	for(i=0;i<Face.Count;i++)
	{
		if(Face[i].Select)
		{
			Face[i].Material=0;
			Face[i].Select=0;
			Face[i].Mask=0;
		}
	}
}

/****************************************************************************/

void GenMesh::SelectGrow()
{
  sInt i;
  GenMeshFace *f,*fi;

  for(i=0;i<Face.Count;i++)
    Face[i].Temp = 0;

  for(i=0;i<Edge.Count*2;i++)
  {
    f = GetFace(i);
    fi = GetFaceI(i);

    if(f->Select && !f->Temp && !fi->Select)
    {
      fi->Select = 1;
      fi->Temp = 1;
    }
  }
}

/****************************************************************************/

void GenMesh::CubicProjection(const sMatrix &mat,sInt mask,sInt dest,sBool real)
{
  sInt i,j,bm,axis;
  sF32 max;
  sVector n;
  sMatrix matuv,matt;

  for(j=0;j<6;j++)
  {
    bm = real ? ((j & 1) ? 1 : -1) : 1;
    All2Sel(0,MAS_FACE|MAS_VERT);

    for(i=0;i<Face.Count;i++)
    {
      if(!mask || (Face[i].Mask & mask))
      {
        // determine projection axis
        CalcFaceNormal(n,Face[i].Edge);
        max = n.x, axis = 0;
        if(sFAbs(n.y) > sFAbs(max)) max = n.y, axis = 2;
        if(sFAbs(n.z) > sFAbs(max)) max = n.z, axis = 4;
        if(max>0.0f) axis++;

        // select appropriately
        Face[i].Select = axis == j;
      }
      else
        Face[i].Select = 0;
    }

    Crease(0,0,0,1<<dest);
    Face2Vert();

    sSetMem(&matt,0,sizeof(matt));
    if(j/2==0)
    {
      matt.j.y = -1.0f;
      matt.k.x = 1.0f * bm;
    }
    else if(j/2==1)
    {
      matt.i.x = 1.0f;
      matt.k.y = -1.0f * bm;
    }
    else
    {
      matt.i.x = -1.0f * bm;
      matt.j.y = -1.0f;
    }

    matuv.MulA(mat,matt);
    TransVert(matuv,sGMI_POS,dest);
  }
}

/****************************************************************************/
/****************************************************************************/

void GenMesh::Prepare()
{
  if(!PreparedMesh)
  {
    PreparedMesh = new EngMesh;
    PreparedMesh->FromGenMesh(this);
  }
}

/****************************************************************************/

void GenMesh::UnPrepare()
{
  if(PreparedMesh)
  {
    PreparedMesh->Release();
    PreparedMesh = 0;
  }

#if !sPLAYER
  UnPrepareWire();
#endif
}

/****************************************************************************/

#if !sPLAYER

void GenMesh::PrepareWire(sInt flags,sU32 selMask)
{
  if(!WireMesh || WireFlags != flags || WireSelMask != selMask)
  {
    UnPrepareWire();

    WireMesh = new EngMesh;
    WireMesh->FromGenMeshWire(this,flags,selMask);
    WireFlags = flags;
    WireSelMask = selMask;
  }
}

void GenMesh::UnPrepareWire()
{
  if(WireMesh)
  {
    WireMesh->Release();
    WireMesh = 0;
  }
}

#endif

/****************************************************************************/
/***                                                                      ***/
/***   Commands                                                           ***/
/***                                                                      ***/
/****************************************************************************/

sBool CheckMesh(GenMesh *&mesh,sU32 mask)
{
  GenMesh *oldmesh;
  if(mesh==0)
    return 1;
  if(mesh->ClassId!=KC_MESH)
    return 1;
  if(mesh->RefCount>1) 
  {
    oldmesh = mesh;
    mesh = new GenMesh;
    mesh->Copy(oldmesh);
    oldmesh->Release();
  }
  if(!(mask&0x80000000))
  {
	  if(mask)
		  mesh->Mask2Sel(mask);
    else
      mesh->All2Sel();
  }
  return 0;
}

/****************************************************************************/
/****************************************************************************/

GenMesh * __stdcall Mesh_Cube(sInt tx,sInt ty,sInt tz,sInt flags,sFSRT srt)
{
	sInt i,j;
  sInt bm;
  sMatrix mat;
  sVector vc,vs,su;
  GenMesh *mesh;

  mesh = new GenMesh;
  mesh->Init(sGMF_DEFAULT,1024);
  mesh->Ring(4,0,1.4142135623730950488016887242097f/2,sPI2F/8);

  mesh->Face[0].Select = sTRUE;
  mesh->Extrude(0x00010000);
  mat.Init();
  mat.l.z = 1;
  mesh->Mask2Sel(0x00010000);
  mesh->TransVert(mat);

  mat.l.Init(0.5f,0.5f,0.0,1.0f);
  mesh->All2Sel();
  mesh->TransVert(mat);
  mat.Init();
  mesh->TransVert(mat,sGMI_POS,sGMI_UV0);

  if(flags&8) // setup scale uv
    su.Init3(srt.s.x,srt.s.y,srt.s.z);
  else
    su.Init3(1,1,1);

// extrude tesselation

  vc.Init(1,1,1,1);
  for(j=0;j<3;j++)
  {
    vs.Init(1025.0f,1025.0f,1025.0f,1);
    (&vs.x)[j] = 0.1f;
    mesh->SelectCube(vc,vs,0,0);
    mat.Init();
    (&mat.l.x)[j] = 1.0f;

    for(i=1;i<(&tx)[j];i++)
    {
      mesh->Extrude(0x00010000,0x00000100);
      mesh->Face2Vert();
      mesh->TransVert(mat);
      mesh->TransVert(mat,sGMI_UV0,sGMI_UV0);
    }
  }

// rescale t0 0.5 .. -0.5

  mat.Init();
  mat.i.x = 1.0f/tx;
  mat.j.y = 1.0f/ty;
  mat.k.z = 1.0f/tz;
  mesh->All2Sel();
  mesh->TransVert(mat,sGMI_UV0,sGMI_UV0);
  mat.l.Init(-0.5f,-0.5f,-0.5f,1);
  mesh->TransVert(mat);

// mapping
  for(j=0;j<6;j++)
  {
    bm = ((j&1)?1:-1);
    vc.Init(0,0,0,1);
    vs.Init(1025,1025,1025,1);
    (&vc.x)[j/2] = bm*0.5f;
    (&vs.x)[j/2] = 0.001f;
    mesh->SelectCube(vc,vs,0,0);
    mesh->Crease(0,0,0,(flags&1) ? (sGMF_ALL & ~sGMF_POS) : sGMF_UV0);
    mesh->Face2Vert();
    mesh->Sel2Mask((0x010100)<<j,MSM_SET);

    sSetMem4((sU32 *)&mat,0,16);
    if(j/2==0)
    {
      mat.j.y = -1.0f * su.y;
      mat.k.x = 1.0f * bm * su.z;
    }
    else if(j/2==1)
    {
      mat.i.x = 1.0f * su.x;
      mat.k.y = -1.0f * bm * su.z;
    }
    else
    {
      mat.i.x = -1.0f * bm * su.x;
      mat.j.y = -1.0f * su.y;
    }

    mat.l.x = 0.5f*(sFAbs(mat.i.x)+sFAbs(mat.k.x));
    mat.l.y = 0.5f*(sFAbs(mat.j.y)+sFAbs(mat.k.y));
    mesh->TransVert(mat,sGMI_POS,sGMI_UV0);
  }

  if(flags&2) // wraparound uv
  {
    // face 0 (left), then 4 (front), then 1, then 5.
    mat.Init();
    mat.l.x = su.z;
    mesh->Mask2Sel(0x100000);
    mesh->TransVert(mat,sGMI_UV0,sGMI_UV0);
    mat.l.x += su.x;
    mesh->Mask2Sel(0x020000);
    mesh->TransVert(mat,sGMI_UV0,sGMI_UV0);
    mat.l.x += su.z;
    mesh->Mask2Sel(0x200000);
    mesh->TransVert(mat,sGMI_UV0,sGMI_UV0);
  }

  // add collision

#if sLINK_KKRIEGER
  if(flags&0x0030)
  {
    i = ((flags&0x0030)>>4)-1;
    i |= ((flags&0x00c0)>>4);
    mesh = Mesh_CollisionCube(mesh,0,0,-0.5f,0.5f,-0.5f,0.5f,-0.5f,0.5f,i,1,1,1);
  }
#endif

  // transform the cube

  mesh->All2Sel(1);
  if(flags&4) // origin on bottom?
  {
    mat.Init();
    mat.l.y = 0.5f;
    mesh->TransVert(mat);
  }
  mat.InitSRT(srt.v);
  mesh->TransVert(mat);

  // just one convex part, starting at face 0
  *mesh->Parts.Add() = 0;

	return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_SelectAll(GenMesh *mesh,sU32 mask,sInt mode)
{
  if(CheckMesh(mesh)) return 0;

  mesh->All2Mask(mask,mode);
	return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_SelectCube(GenMesh *mesh,sU32 dmask,sInt dmode,sF323 center,sF323 size)
{
  if(CheckMesh(mesh)) return 0;
  size.x *= 0.5f;
  size.y *= 0.5f;
  size.z *= 0.5f;
  mesh->SelectCube((sVector&)center.x,(sVector&)size.x,0,0);
  if(dmode&4)
    mesh->Face2Vert();
  mesh->Sel2Mask(dmask,dmode&3);

	return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_Subdivide(KOp *upd,GenMesh *mesh,sInt mask,sF32 alpha,sInt count)
{
  if(CheckMesh(mesh,mask<<8)) return 0;

	while(count--)
		mesh->Subdivide(alpha);

  mesh->GotNormals = sFALSE;

	return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_Extrude(KOp *upd,GenMesh *mesh,sInt smask,sInt dmask,sInt mode,sInt count,sF323 dist,sF323 s,sF323 r)
{
  sInt i,j,grp,e,ee;
  static sInt stk[4096];
  sMatrix mat;

  if(CheckMesh(mesh,smask<<8)) return 0;

  mesh->NeedAllNormals();
  grp = ((mode & 8) || ((mode & 6) == 2)) ? 0 : 1;

  // first, classify faces
  for(i=0;i<mesh->Face.Count;i++)
    mesh->Face[i].Temp = grp - 1;

  while(1)
  {
    for(i=0;i<mesh->Face.Count;i++)
    {
      if(mesh->Face[i].Temp==-1 && mesh->Face[i].Select)
        break;
    }

    if(i==mesh->Face.Count)
      break;

    stk[0] = i;
    j = 1;
    while(j)
    {
      i = stk[--j];
      mesh->Face[i].Temp = grp;

      e = ee = mesh->Face[i].Edge;
      do
      {
        i = mesh->GetFaceId(e^1);
        if(mesh->Face[i].Temp==-1 && !mesh->IsBorderEdge(e,mode&1))
        {
          mesh->Face[i].Temp = -2;
          stk[j++] = i;
        }

        e = mesh->NextFaceEdge(e);
      }
      while(e!=ee);
    }

    grp++;
  }

  // scale transform parameters
  sF32 ic = 1.0f / count;

  for(i=0;i<3;i++)
  {
    s[i] = sFPow(sFAbs(s[i]),ic) * (s[i] < 0 ? -1.0f : 1.0f);
    r[i] = r[i] * ic;
    dist[i] = dist[i] * ic;
  }

  mat.InitEulerPI2(&r.x);
  sInt vs = mesh->VertSize();
  sInt sn = mesh->VertMap(sGMI_NORMAL);

  if(((mode>>4)&3)>=MSM_SET) // selection mode is "set"?
    mesh->All2Mask((dmask & 0xff)<<8,MSM_SUB); // clear target selection

  sVector n,p;
  if((mode & 6) == 4) // direction
    n.Init(dist.x,dist.y,dist.z,0);

  // main loop
  while(count--)
  {
    mesh->Extrude((dmask & 0xff)<<8,(mode>>4)&3,mode&1);

    for(i=0;i<mesh->Vert.Count;i++)
      mesh->Vert[i].Temp = 0;

    sInt vcount;

    for(i=0;i<grp;i++)
    {
      sInt ffirst = -1;

      for(sInt j=mesh->Face.Count-1;j>=0;j--)
      {
        GenMeshFace *face = &mesh->Face[j];
        if(face->Select && face->Temp == i)
        {
          face->Temp2 = ffirst;
          ffirst = j;
        }
      }

      // first pass: face to vertex normals (if required)
      if((mode&6)==6) // face normals
      {
        //for(sInt f=0;f<mesh->Face.Count;f++)
        for(sInt f=ffirst;f!=-1;f=mesh->Face[f].Temp2)
        {
          GenMeshFace *face = &mesh->Face[f];
          
          if(face->Select && face->Temp == i)
          {
            sVector normal;
            mesh->CalcFaceNormal(normal,face->Edge);
            normal.UnitSafe3();

            e = ee = face->Edge;
            do
            {
              mesh->VertBuf[mesh->GetVertId(e) * vs + sn] = normal;
              e = mesh->NextFaceEdge(e);
            }
            while(e != ee);
          }
        }
      }

      // second pass: find average normal (if required)
      // third pass: vertex displacement and summing
      // fourth pass: local scale (if required)
      for(sInt pass=1;pass<4;pass++)
      {
        sInt passMask = 1 << pass;
        if(pass == 1)
        {
          if((mode & 6) != 2)
            continue;

          n.Init(0,0,0,0);
        }
        else if(pass == 2)
        {
          if((mode & 6) == 2)
          {
            n.UnitSafe3();
            n.Mul3((sVector&) dist);
          }

          p.Init(0,0,0,1);
        }
        else if(pass == 3)
        {
          if(!(mode & 8))
            continue;

          p.Scale3(1.0f / vcount);
        }

        vcount = 0;

        //for(sInt f=0;f<mesh->Face.Count;f++)
        for(sInt f=ffirst;f!=-1;f=mesh->Face[f].Temp2)
        {
          GenMeshFace *face = &mesh->Face[f];

          if(face->Select && face->Temp == i)
          {
            e = ee = face->Edge;

            do
            {
              GenMeshVert *vert = mesh->GetVert(e);
              if(vert->Temp & passMask)
              {
                e = mesh->NextFaceEdge(e);
                continue;
              }

              vert->Temp |= passMask;
              sInt v = mesh->GetVertId(e);
              sVector *vd = mesh->VertBuf + v * vs;

              if(pass == 1)
                n.Add3(vd[sn]);
              else if(pass == 2)
              {
                if((mode & 6) == 0 || (mode & 6) == 6) // indiv./face normal
                  vd->AddMul3(vd[sn],(sVector&) dist);
                else
                  vd->Add3(n);

                if(mode & 8)
                  p.Add3(*vd);
                else
                {
                  vd->Mul3((sVector&) s);
                  vd->Rotate3(mat);
                }

                vd[sn].Rotate3(mat);
              }
              else if(pass == 3)
              {
                sVector t;

                // MODIFIED BEHAVIOR!!!
                t.x = (vd->x - p.x) * s.x;
                t.y = (vd->y - p.y) * s.y;
                t.z = (vd->z - p.z) * s.z;
                vd->Rotate34(mat,t);
                vd->x += p.x;
                vd->y += p.y;
                vd->z += p.z;
              }

              vcount++;
              e = mesh->NextFaceEdge(e);
            }
            while(e != ee);
          }
        }
      }
    }
  }

  if(dmask&0xff)
    mesh->Mask2Sel((dmask & 0xff)<<8);
  mesh->Face2Vert();
  mesh->Sel2Mask((dmask & 0xff00)<<8);
  mesh->GotNormals = sFALSE;

  return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_Transform(KOp *upd,GenMesh *mesh,sInt mask,sFSRT srt)
{
	sMatrix mat;
  if(CheckMesh(mesh,mask<<16)) return 0;

  mat.InitSRT(srt.v);
#if !sINTRO
  if(mesh->Pivot!=-1)
    mat.PivotTransform(mesh->VertBuf[mesh->Pivot*mesh->VertSize()]);
#endif
	mesh->TransVert(mat);
  mesh->GotNormals = sFALSE;

	return mesh;
}


/****************************************************************************/

GenMesh * __stdcall Mesh_TransformEx(KOp *upd,GenMesh *mesh,sInt mask,sFSRT srt,sInt sj,sInt dj)
{
	sMatrix mat;

  if(CheckMesh(mesh,mask<<16)) return 0;

  if(sj == sGMI_NORMAL || sj == sGMI_TANGENT)
    mesh->NeedAllNormals();

  mat.InitSRT(srt.v);
#if !sINTRO
  if(mesh->Pivot!=-1 && mesh->VertMap(sj)!=-1)
    mat.PivotTransform(mesh->VertBuf[mesh->Pivot*mesh->VertSize()+mesh->VertMap(sj)]);
#endif
	mesh->TransVert(mat,sj,dj);
  if(dj == sGMI_POS || dj == sGMI_NORMAL || dj == sGMI_TANGENT)
    mesh->GotNormals = sFALSE;

	return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_Cylinder(sInt tx,sInt ty,sInt mode,sInt tz,sInt arc)
{
  sInt i,sj,e;
  sMatrix mat,matuv;
  sVector vc,vs;
  GenMesh *mesh;

  arc = sRange<sInt>(arc,tx-1,0);

  mesh = new GenMesh;
	mesh->Init(sGMF_DEFAULT,1024);

// create cylinder
  mesh->Ring(tx,0,0.5f,0,arc);

  mat.Init();
  mat.l.z = 1;
  matuv.Init();
  matuv.l.y = 1.0f/ty;

  mesh->Face[0].Select = sTRUE;
  for(i=0;i<ty;i++)
  {
		e=mesh->Edge.Count;
    mesh->Extrude(0);
		mesh->Edge[e].Sel(0x000001,1,MSM_ADD);
    mesh->Face2Vert();
    mesh->TransVert(mat);
    mesh->TransVert(matuv,sGMI_UV0,sGMI_UV0);
  }

// adjust size
  static const sF32 turn[] = { 0.25,0.0f,0.0f };

  mat.InitEulerPI2(turn);
  vs.Init(1.0f/tz,1.0f/ty,1.0f/tz,1.0f);
  mat.i.Mul4(vs);
  mat.j.Mul4(vs);
  mat.k.Mul4(vs);
  mat.l.Init(0,0.5f,0.0f,1);
  mesh->All2Sel(1,MAS_VERT);
  mesh->TransVert(mat);

  // make selection & caps
  mesh->All2Mask(0x010100,MSM_SUB);
  vc.Init();
  vs.Init(1025.0f,0.0001f,1025.0f,0.0f);
  vc.y = 0.5f;
  mesh->SelectCube(vc,vs,0x010100,MSM_ADD);
  vc.y = -0.5f;
  mesh->SelectCube(vc,vs,0x010100,MSM_ADD);
  mesh->Mask2Sel(0x100);
  mesh->Triangulate(4,0,0,(arc>0)?2:0);

  // extrude rings
  if(tz>1)
  {
    mesh->All2Sel(1,MAS_VERT);
    mat.Init();
    mat.j.y = 0.0f;
    mesh->TransVert(mat,sGMI_POS,sGMI_NORMAL);

    mesh->Mask2Sel(0x100);
    for(i=0;i<mesh->Face.Count;i++)
      mesh->Face[i].Select ^= 1;

    for(i=1;i<tz;i++)
    {
      e = mesh->Edge.Count;
      mesh->Extrude(0x100,MSM_ADD);
      while(e<mesh->Edge.Count)
        mesh->Edge[e++].Mask=0;
      mesh->Face2Vert();
      mesh->ExtrudeNormal(1.0f);
    }
  }

// correct mapping
  sj = mesh->VertMap(sGMI_UV0);
  if(sj!=-1)
  {
		for(i=-1;i<2;i+=2)
    {
      vs.Init(2,0.01f,2,1);
      vc.Init(0,i*0.5f,0,1);
      mesh->All2Sel(0,MAS_FACE|MAS_VERT);
      mesh->SelectCube(vc,vs,0,0);
      mesh->Crease();
      mesh->Face2Vert();
      matuv.Init();
      matuv.j.y = 0.0f;
      matuv.k.y = -i;
      matuv.l.Init(0.5f,0.5f,0,1);
      mesh->TransVert(matuv,sGMI_POS,sGMI_UV0);
    }

    mesh->All2Mask(0x020000,MSM_SUB);
    mesh->Mask2Sel(0x020001);
		mesh->Crease(1,0,0,sGMF_UV0); // uv0 only
		mesh->Edge2Vert(0);
    matuv.Init();
    matuv.l.x = 1.0f;
    mesh->TransVert(matuv,sGMI_UV0,sGMI_UV0);
 	}

  if(mode&1)
    mesh = Mesh_DeleteFaces(mesh,0x01);

  if(mode&2) // origin on bottom?
  {
    mat.Init();
    mat.l.y = 0.5f;
    mesh->All2Sel(1);
    mesh->TransVert(mat);
  }
  mesh->All2Mask(1,MSM_SUB);

  // just one convex part, starting at face 0
  *mesh->Parts.Add() = 0;

  return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_Crease(GenMesh *mesh,sInt mask,sInt what,sInt selType)
{
  if(CheckMesh(mesh,mask<<(selType ? 0 : 8))) return 0;

	mesh->Crease(selType,0,0,1<<(what+1));
  mesh->Face2Vert();
  mesh->Sel2Mask(mask<<16,MSM_SET);
  if(what & (sGMF_NORMAL | sGMF_TANGENT))
    mesh->GotNormals = sFALSE;

	return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_UnCrease(GenMesh *mesh,sInt mask,sInt what,sInt selType)
{
	SCRIPTVERIFY(what);
  if(CheckMesh(mesh,mask<<8)) return 0;

	mesh->UnCrease(selType,1<<(what+1));

	return mesh;
}

/****************************************************************************/


GenMesh * __stdcall Mesh_CalcNormals(GenMesh *mesh,sInt mode,sInt mask,sInt what)
{
  if(CheckMesh(mesh,mask<<8)) return 0;

	mesh->CalcNormals(mode,what);
	return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_Torus(sInt tx,sInt ty,sF32 ro,sF32 ri,sF32 phase,sF32 arclen,sInt flags)
{
	sInt i,n,e,f,t;
	sInt sj;
	sBool closed;
	sMatrix mat,matuv;
  GenMesh *mesh;

  mesh = new GenMesh;
	mesh->Init(sGMF_DEFAULT,1024);

	closed = arclen == 1.0f;

  if(flags&1) // absolute radii
  {
    ri = (ro - ri) * 0.5f;
    ro -= ri;
  }

// create torus
	mesh->Ring(ty,0,ri,phase*sPI2/ty);

	mat.Init();
	mat.l.x = -ro;
	mesh->Face[0].Select = sTRUE;

	mesh->Face2Vert();
	mesh->TransVert(mat);

	mat.InitEuler(0.0f,arclen*sPI2/tx,0.0f);
	matuv.Init();
	matuv.l.y = 1.0f/tx;

	for(i=0;i<tx-closed;i++)
	{
		e = mesh->Edge.Count;
		mesh->Extrude(0);
		mesh->Edge[e].Sel(0x000001,1,MSM_ADD);
		mesh->Face2Vert();
		mesh->TransVert(mat);
		mesh->TransVert(matuv,sGMI_UV0,sGMI_UV0);
	}

// that was easy... now we need to merge start and end faces!
	if(closed)
	{
		// delete the start/end faces
		mesh->Face[1].Select = sTRUE;
		mesh->DeleteFaces();

		e = mesh->Edge.Count;
		f = mesh->Face.Count;

		mesh->Edge.AtLeast(e+ty);	mesh->Edge.Count+=ty;
		mesh->Face.AtLeast(f+ty); mesh->Face.Count+=ty;

		// make the new edges
		for(i=0;i<ty;i++)
		{
			mesh->Edge[e+i].Init();
			mesh->Edge[e+i].Vert[0] = mesh->Vert.Count-ty+i;
			mesh->Edge[e+i].Vert[1] = i;
		}
		mesh->Edge[e].Sel(0x000001,1,MSM_ADD);

		// make the faces (and mark the v crease while we're at it)
		for(i=0;i<ty;i++)
		{
			n = (i+1)%ty;
			t = mesh->NextFaceEdge(mesh->Face[2+i].Edge)^1;
			mesh->MakeFace(f+i,4,t,(e+i)*2+1,mesh->PrevFaceEdge(mesh->Face[f-ty+i].Edge)^1,(e+n)*2);
			mesh->Edge[t/2].Sel(0x000002,1,MSM_ADD);
		}
	}

// prepare the creases
	sj = mesh->VertMap(sGMI_UV0);
	if(sj!=-1)
	{
		if(!closed)
		{
      mesh->All2Sel(0,MAS_FACE);
			mesh->Face[0].Select = 1;
			mesh->Face[1].Select = 1;
			mesh->Crease(0,0,0,sGMF_UV0);
			// perform uv projection at caps
		}

		for(i=0;i<2;i++)
		{
      mesh->All2Mask(0x010000,MSM_SUB);
			mesh->Mask2Sel((i+1)|0x010000);
			mesh->Crease(1,0,0,sGMF_UV0);
			matuv.Init();
			(&matuv.l.x)[i] = 1.0f;
			mesh->Edge2Vert(i);
			mesh->TransVert(matuv,sGMI_UV0,sGMI_UV0);
		}
	}

// origin on bottom?
  if(flags&2)
  {
    mat.Init();
    mat.l.y = ri;
    mesh->All2Sel(1);
    mesh->TransVert(mat);
  }

  // just one closed part, starting at face 0
  *mesh->Parts.Add() = 0;

  mesh->All2Mask(~0,MSM_SETNOT);
	return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_Sphere(sInt tx,sInt ty)
{
	sInt i,j,v,sj;
	sMatrix mat,matuv;
  GenMesh *mesh;

  mesh = new GenMesh;
	mesh->Init(sGMF_DEFAULT,1024);

// first, generate a cheesy cylinder
	matuv.Init();
	matuv.l.y = 1.0f / ty;

	mesh->Ring(tx,0,0.5f,0.0f);
	mesh->Face[mesh->Face.Count-1].Select = sTRUE;
	for(i=0;i<ty;i++)
	{
		mesh->Extrude(0);
		mesh->Face2Vert();
		mesh->TransVert(matuv,sGMI_UV0,sGMI_UV0);
	}

  matuv.j.y = -1.0f;
  matuv.l.y = 1.0f;
  mesh->All2Sel(1,MAS_VERT);
  mesh->TransVert(matuv,sGMI_UV0,sGMI_UV0);

// swap axes
	mat.Init();
	mat.j.y = 0.0f; mat.j.z = 1.0f;
	mat.k.y = 1.0f; mat.k.z = 0.0f;
  mesh->All2Sel(1,MAS_VERT);
	mesh->TransVert(mat);

// make it a sphere
	v=mesh->Vert.Count-tx*(ty+1);
	for(i=0;i<=ty;i++)
	{
    mesh->All2Sel(0,MAS_VERT);
		for(j=0;j<tx;j++,v++)
			mesh->Vert[v].Select=1;

		mat.Init();
		mat.i.x = mat.k.z = sFSin((i+0.5f)*sPI/(ty+1));
		mat.l.y = -0.5f*sFCos((i+0.5f)*sPI/(ty+1));
		mesh->TransVert(mat);
	}

// fix top/bottom
	for(i=0;i<2;i++)
	{
    mesh->All2Sel(0,MAS_FACE);
		mesh->Face[i].Select=1;
    mesh->Face[i].Sel(0x000100,1,MSM_ADD);
		mesh->Triangulate(4,0x000100);
	}

// make the u-crease
	sj=mesh->VertMap(sGMI_UV0);
	if(sj!=-1)
	{
    mesh->All2Sel(0,MAS_VERT);
		mesh->VertBuf[(mesh->Vert.Count-2)*mesh->VertSize()+sj].x=0;
		mesh->VertBuf[(mesh->Vert.Count-1)*mesh->VertSize()+sj].x=0;

		for(i=0;i<mesh->Edge.Count;i++)
		{
			if(mesh->VertBuf[mesh->GetVertId(i*2+0)*mesh->VertSize()+sj].x==0 &&
				 mesh->VertBuf[mesh->GetVertId(i*2+1)*mesh->VertSize()+sj].x==0)
			{
				mesh->Edge[i].Select=1;
			}
			else
				mesh->Edge[i].Select=0;
		}

		mesh->Crease(1,0,0,sGMF_UV0); // uv0 only
		mesh->Edge2Vert(0);
		mesh->Vert[0].Select=0;

		matuv.Init();
		matuv.l.x = 1.0f;
		mesh->TransVert(matuv,sGMI_UV0,sGMI_UV0);
	}

  // just one closed part, starting at face 0
  *mesh->Parts.Add() = 0;

  mesh->All2Mask(0xfffeff,MSM_SUB);
	return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_Triangulate(GenMesh *mesh,sInt mask,sInt thres,sInt type)
{
  if(CheckMesh(mesh,mask<<8)) return 0;

	mesh->Triangulate(thres,0,0,type);
  mesh->GotNormals = sFALSE;
	return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_Cut(GenMesh *mesh,sF322 dir,sF32 offs,sInt mode)
{
	sMatrix mat;
	sVector plane;

  if(CheckMesh(mesh)) return 0;
	
  mat.InitEuler(dir.x*sPI2F,dir.y*sPI2F,0.0f);
	plane.Init4(mat.i.x,mat.j.x,mat.k.x,offs);
  mesh->All2Sel();
	mesh->Cut(plane,mode);
  mesh->GotNormals = sFALSE;

  return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_ExtrudeNormal(KOp *upd,GenMesh *mesh,sInt mask,sF32 distance)
{
  if(CheckMesh(mesh,mask<<16)) return 0;

  mesh->NeedAllNormals();
	mesh->ExtrudeNormal(distance);
  mesh->GotNormals = sFALSE;
	return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_Displace(KOp *upd,GenMesh *mesh,GenBitmap *bmp,sInt mask,sF323 ampli)
{
	SCRIPTVERIFY(bmp);
  if(CheckMesh(mesh,mask<<16)) return 0;

  mesh->NeedAllNormals();
  mesh->Displace(bmp,ampli.x,ampli.y,ampli.z);
  bmp->Release();
  mesh->GotNormals = sFALSE;
	return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_Bevel(KOp *upd,GenMesh *mesh,sInt mask,sF32 elev,sF32 pull,sInt mode)
{
  if(CheckMesh(mesh,mask<<8)) return 0;

  mesh->NeedAllNormals();
	mesh->Bevel(elev,pull,mode,0,mask<<8);
  mesh->GotNormals = sFALSE;
  //mesh->Sel2Mask(dmask<<16,0);

	return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_Perlin(KOp *upd,GenMesh *mesh,sInt mask,sFSRT srt,sF323 ampl)
{
	sMatrix mat;

  if(CheckMesh(mesh,mask<<16)) return 0;
  mat.InitSRT(srt.v);
  mesh->Perlin(mat,(sVector&) ampl.x);
	return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_Add(sInt count,GenMesh *mesh,...)
{
  sInt i;
  GenMesh *other;

  if(CheckMesh(mesh)) return 0;

  for(i=1;i<count;i++)
  {
    other = (&mesh)[i];
    if(other)
    {
      mesh->Add(other);
      other->Release();
    }
  }
  mesh->GotNormals = sFALSE;
 
  return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_DeleteFaces(GenMesh *mesh,sInt mask)
{
  if(CheckMesh(mesh,mask<<8)) return 0;

  mesh->DeleteFaces();

	return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_BeginRecord(GenMesh *mesh)
{
  return mesh;
/*
  SCRIPTVERIFY(mesh);
  if(CheckMesh(mesh)) return 0;
  mesh->RecStoreMode();
  return mesh;*/
}

/****************************************************************************/

GenMesh * __stdcall Mesh_SelectRandom(GenMesh *mesh,sU32 dmask,sInt dmode,sU32 ratio,sInt seed)
{
	sInt i;

  if(CheckMesh(mesh)) return 0;

	sSetRndSeed(seed+seed*31743^(seed<<23));
	
	for(i=0;i<mesh->Edge.Count;i++)
		mesh->Edge[i].Select=(sGetRnd()>>24)<=ratio;
	for(i=0;i<mesh->Vert.Count;i++)
    mesh->Vert[i].Select=(sGetRnd()>>24)<=ratio;
  for(i=0;i<mesh->Vert.Count;i++)
    mesh->Vert[i].Select=mesh->Vert[mesh->Vert[i].First].Select;
	for(i=0;i<mesh->Face.Count;i++)
    mesh->Face[i].Select=(sGetRnd()>>24)<=ratio && mesh->Face[i].Material;

	mesh->Sel2Mask(dmask,dmode);
	return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_Multiply(KOp *kop,GenMesh *mesh,sFSRT srt,sInt count,sInt mode,sF32 tu,sF32 tv,sF323 lrot,sF32 extrude)
{
	sInt i,j;
	GenMesh *out;
	sMatrix xform,step,lxform,lstep,tmp;

  if(CheckMesh(mesh)) return 0;

	step.InitSRT(srt.v);
  lstep.InitEulerPI2(&lrot.x);
	out = new GenMesh;
  out->Init(mesh->VertMask(),0);
	xform.Init();
  lxform.Init();
  if(extrude)
    mesh->NeedAllNormals();

  sSetRndSeed(count);
	for(j=0;j<count;j++)
	{
		out->Add(mesh,!!j);   // 0:not keepmaterial: 1..n keepmaterial
		for(i=0;i<out->Vert.Count-mesh->Vert.Count;i++)
			out->Vert[i].Select=0;
		while(i<out->Vert.Count)
			out->Vert[i++].Select=1;

    if(extrude)
    {
      tmp.Init();
      tmp.i.x = j*extrude;
      tmp.j.y = j*extrude;
      tmp.k.z = j*extrude;
      out->TransVert(tmp,sGMI_NORMAL,sGMI_POS | 0x10);
    }

    tmp.MulA(lxform,xform);
		out->TransVert(tmp);

    if(mode&2)
      xform.InitRandomSRT(srt.v);
    else
		  xform.MulA(step);

    lxform.MulA(lstep);

    if(mode&1)
    {
      tmp.Init();
      tmp.l.x = j*tu;
      tmp.l.y = j*tv;
      out->TransVert(tmp,sGMI_UV0,sGMI_UV0);
    }
	}

  mesh->Release();
  out->GotNormals = sFALSE;

	return out;
}

/****************************************************************************/

/*GenMesh * __stdcall Mesh_SelectAngle(GenMesh *mesh,sF32 angle,sU32 dmask1,sU32 dmask0)
{
  sInt i;
  sVector f0,f1;

  if(CheckMesh(mesh)) return 0;

  angle = cos(angle*sPI2F*0.25f);
  for(i=0;i<mesh->Edge.Count;i++)
  {
    mesh->CalcFaceNormal(f0,i*2  ); f0.UnitSafe3();
    mesh->CalcFaceNormal(f1,i*2+1); f1.UnitSafe3();

    if(sFAbs(f0.Dot3(f1))<angle)
      mesh->Edge[i].Sel(dmask1,dmask0);
  }

  return mesh;
}*/

/****************************************************************************/

GenMesh * __stdcall Mesh_Bend(KOp *upd,GenMesh *mesh,sInt mask,sFSRT srt1,sFSRT srt2,sF322 dir,sF322 yrange,sInt mode)
{
  sMatrix mat1,mat2,matt;

  if(CheckMesh(mesh,mask<<16)) return 0;

  mat1.InitSRT(srt1.v);
#if !sINTRO
  if(mesh->Pivot!=-1)
    mat1.PivotTransform(mesh->VertBuf[mesh->Pivot*mesh->VertSize()]);
#endif

  mat2.InitSRT(srt2.v);
#if !sINTRO
  if(mesh->Pivot!=-1)
    mat2.PivotTransform(mesh->VertBuf[mesh->Pivot*mesh->VertSize()]);
#endif

  matt.InitEuler(dir.x*sPI2F,0.0f,dir.y*sPI2F);

  // first pass: find min/max (transformed) y coordinate (if necessary)
  sF32 ymin,ymax;

  if(mode & 2)
  {
    ymin =  1e+30f;
    ymax = -1e+30f;

    for(sInt i=0;i<mesh->Vert.Count;i++)
    {
      if(mesh->Vert[i].Select)
      {
        sVector *v = mesh->VertBuf + i * mesh->VertSize();
        sF32 t = v->x*matt.i.y + v->y*matt.j.y + v->z*matt.k.y;

        ymin = sMin(ymin,t);
        ymax = sMax(ymax,t);
      }
    }
  }
  else
  {
    ymin = yrange.x;
    ymax = yrange.y;
  }

  sF32 yscale = 1.0f / (ymax - ymin);
  for(sInt i=0;i<mesh->Vert.Count;i++)
  {
    if(mesh->Vert[i].Select)
    {
      sVector *v = mesh->VertBuf + i * mesh->VertSize();
      sF32 t = (v->x*matt.i.y + v->y*matt.j.y + v->z*matt.k.y - ymin) * yscale;

      t = sRange(t,1.0f,0.0f);
      switch(mode & 5)
      {
      case 1: t = t * t * (3.0f - 2.0f * t);                  break;
      case 4: t = 1.0f - fabs(t - 0.5f) * 2.0f;               break;
      case 5: t = t * t * (16.0f + t * (16.0f * t - 32.0f));  break;
      }

      sVector v1,v2;
      v1.Rotate34(mat1,*v);
      v2.Rotate34(mat2,*v);
      v->Lin3(v1,v2,t);
    }
  }

  return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_CollisionCube(GenMesh *input,KOp *event,KOp *event2,sF32 x0,sF32 x1,sF32 y0,sF32 y1,sF32 z0,sF32 z1,sInt mode,sInt tx,sInt ty,sInt tz,sInt eventa,sInt eventb,sInt eventc,sInt eventd)
{
  GenMesh *mesh;
  sInt i,ix,iy,iz,max,count,index;
  sF32 phi[2];
  sVector *v;
  sVector v0,vs;

  if(input && CheckMesh(input,0)) return 0;

#if !sPLAYER
  if((mode&3)==3)
    sDPrintF("oops\n");
#endif

  if((mode&16) && tz<3) tz = 3;
  max = tx*ty*tz;
  sVERIFY(max>0);

  mesh = new GenMesh;
  mesh->Init(sGMF_DEFAULT,max*8);
  mesh->Coll.AtLeast(max);
  mesh->Coll.Count = max;
  v0.w = 1;
  vs.x = x0-x1;
  vs.y = y0-y1;
  vs.z = z0-z1;
  vs.w = 0;
  if(!(mode&16))
  {
    v0.x = x1;
    v0.y = y1;
    v0.z = z1;
    vs.x /= tx;
    vs.y /= ty;
    vs.z /= tz;
  }
  else
  {
    v0.x = (x0+x1)*0.5f;
    v0.y = y0;
    v0.z = (z0+z1)*0.5f;
    vs.x /= -tx*2.0f;
    vs.y /= -ty;
    vs.z /= -tx*2.0f;
  }

  count = 0;

  for(ix=0;ix<tx;ix++)
  {
    for(iy=0;iy<ty;iy++)
    {
      for(iz=0;iz<tz;iz++)
      {
        mesh->Coll[count].Mode = mode&15;
        mesh->Coll[count].Logic.Code = eventa;
        mesh->Coll[count].Logic.Condition = eventb;
        mesh->Coll[count].Logic.Value = eventc;
        mesh->Coll[count].Logic.Output = eventd;
        mesh->Coll[count].Logic.Event[0] = event;
        mesh->Coll[count].Logic.Event[1] = event2;

        phi[0] = (iz+1)*sPI2F/tz;
        phi[1] = (iz+0)*sPI2F/tz;

        for(i=0;i<8;i++)
        {
          index = mesh->AddNewVert();
          mesh->Coll[count].Vert[i] = index;
          v = mesh->VertBuf + index * mesh->VertSize();
          if(mode&16)
          {
            v->x = v0.x + sFSin(phi[i&1])*vs.x*(ix+((i>>2)&1));
            v->y = v0.y + vs.y*(iy+((i>>1)&1));
            v->z = v0.z + sFCos(phi[i&1])*vs.z*(ix+((i>>2)&1));
          }
          else
          {
            v->x = v0.x + vs.x*(ix+((i>>0)&1));
            v->y = v0.y + vs.y*(iy+((i>>1)&1));
            v->z = v0.z + vs.z*(iz+((i>>2)&1));
          }
          v->w = 1.0;
        }
        count++;
      }
    }
  }
  sVERIFY(count==max);

  if(input)
  {
    input->Add(mesh);
    mesh->Release();
    return input;
  }
  else
  {
    return mesh;
  }
}

/****************************************************************************/

static void MultiSplit(GenMesh *mesh,sInt e,sInt splits)
{
  sInt va,vb,va0,va1,vb0,vb1,vs;
  sInt i,j;
  sF32 t;

  va0 = mesh->GetVertId(e);
  va1 = mesh->GetVertId(mesh->NextFaceEdge(e));
  vb0 = mesh->GetVertId(mesh->NextFaceEdge(e^1));
  vb1 = mesh->GetVertId(e^1);
  vs = mesh->VertSize();

  for(i=0;i<splits;i++)
  {
    va = mesh->AddNewVert();
    mesh->SplitBridge(mesh->NextVertEdge(e^1),mesh->PrevVertEdge(e^1),va,vb);

    for(j=0;j<vs;j++)
    {
      t = (i + 1.0f) / (splits + 1.0f);
      mesh->VertBuf[va*vs+j].Lin4(mesh->VertBuf[va0*vs+j],mesh->VertBuf[va1*vs+j],t);
      mesh->VertBuf[vb*vs+j].Lin4(mesh->VertBuf[vb0*vs+j],mesh->VertBuf[vb1*vs+j],t);
    }

    e = mesh->NextFaceEdge(e);
    mesh->Edge[e/2].Select = 1;
  }
}

static void QuadSplit(GenMesh *mesh,sInt e,sInt splits,sInt offs,sInt offt)
{
  sInt e1,e2;

  if(!splits)
    return;

  e1 = mesh->SkipFaceEdge(e,offs);
  e2 = mesh->SkipFaceEdge(e1,2);
  if(!mesh->Edge[e1/2].Select)
  {
    mesh->Edge[e1/2].Select = 1;
    MultiSplit(mesh,e1,splits);
  }

  if(!mesh->Edge[e2/2].Select)
  {
    mesh->Edge[e2/2].Select = 1;
    MultiSplit(mesh,e2,splits);
  }

  while(splits--)
  {
    e2 = mesh->SkipFaceEdge(e,offt+1);
    mesh->SplitFace(e2,mesh->SkipFaceEdge(e2,-3));
    e = mesh->SkipFaceEdge(e2,-offt);
  }
}

GenMesh * __stdcall Mesh_Grid(sInt mode,sInt tesu,sInt tesv)
{
  GenMesh *mesh;
  sMatrix mat;
  sInt i,j;
  sVector *v,t;

  mesh = new GenMesh;
  mesh->Init(sGMF_DEFAULT,4);
  mesh->Ring(4,0,1.4142135623730950488016887242097f/2,sPI2F/8);

  // make crease
  mesh->All2Sel();
  mesh->Face[1].Select = 0;
  mesh->Crease();
  mesh->Face[1].Select = 1;
  mat.Init();
  mat.j.y = -1.0f;
  mat.l.x = 0.5f;
  mat.l.y = 0.5f;
  mesh->TransVert(mat,sGMI_POS,sGMI_UV0);
  mat.InitEuler(0.25f*sPI2F,0.0f,0.0f);
  mesh->TransVert(mat);
  mesh->CalcNormals();

  for(i=0;i<mesh->Vert.Count;i++)
  {
    v = mesh->VertBuf + i * mesh->VertSize();
    if(v[1].y < 0.0f)
    {
      v += mesh->VertMap(sGMI_UV0);
      v->x = 1.0f - v->x;
    }
  }

  // split vertical
  mesh->All2Sel(0);
  QuadSplit(mesh,mesh->Face[0].Edge,tesv-1,0,0);
  QuadSplit(mesh,mesh->Face[1].Edge,tesv-1,0,2);
  
  // split horizontal
  mesh->All2Sel(0);

  for(i=0;i<tesv*2;i++)
    QuadSplit(mesh,mesh->Face[i].Edge,tesu-1,-1,1);

  // selection: side 1/2
  mesh->All2Mask(~0,MSM_SETNOT);
  mesh->All2Sel(0);
  for(i=0;i<mesh->Face.Count;i++)
    mesh->Face[i].Select = mesh->VertNorm(mesh->GetVertId(mesh->Face[i].Edge)).y<0.0f;
  mesh->Face2Vert();
  mesh->Sel2Mask(0x010100,MSM_SET);
  mesh = Mesh_SelectLogic(mesh,0x010100,0x010100,0x020200,19);

  // selection: outer x/z
  for(i=0;i<=2;i+=2)
  {
    mesh->All2Sel(0);
    for(j=0;j<mesh->Edge.Count*2;j++)
    {
      if(mesh->Edge[j/2].Crease)
      {
        t.Sub3(mesh->VertPos(mesh->GetVertId(j)),mesh->VertPos(mesh->GetVertId(j^1)));
        if(sFAbs((&t.x)[i]) > 1e-6f)
          mesh->GetFace(j)->Select = 1;
      }
    }

    mesh->Face2Vert();
    mesh->Sel2Mask(0x040400 << i,MSM_SET);
    mesh = Mesh_SelectLogic(mesh,0x040400<<i,0x040400<<i,0x080800<<i,19);
  }

  // selection: outer both
  mesh = Mesh_SelectLogic(mesh,0x040400,0x101000,0x404000,0);
  mesh = Mesh_SelectLogic(mesh,0x404000,0x404000,0x808000,19);

  if(!(mode & 1))
    mesh = Mesh_DeleteFaces(mesh,0x02);

  // just one closed part, starting at face 0
  *mesh->Parts.Add() = 0;

  return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_SelectLogic(GenMesh *msh,sU32 smask1,sU32 smask2,sU32 dmask,sInt mode)
{
  sInt i;
  sU32 smask;
//  sBool s1,s2;

  if(CheckMesh(msh)) return 0;

  smask = smask1 | smask2;

  if((smask & 0x0000ff) && (dmask & 0x0000ff))
  {
    for(i=0;i<msh->Edge.Count;i++)
      msh->Edge[i].SelLogic(smask1,smask2,dmask,mode & 31);
  }

  if((smask & 0x00ff00) && (dmask & 0x00ff00))
  {
    for(i=0;i<msh->Face.Count;i++)
      msh->Face[i].SelLogic(smask1>>8,smask2>>8,dmask>>8,mode & 31);
  }

  if((smask & 0xff0000) && (dmask & 0xff0000))
  {
    for(i=0;i<msh->Vert.Count;i++)
      msh->Vert[i].SelLogic(smask1>>16,smask2>>16,dmask>>16,mode & 31);
  }

  if((dmask & 0xff0000) && (dmask & 0x00ff00) && (mode & 32))
  {
    msh->Mask2Sel(dmask & 0x00ff00);
    msh->Face2Vert();
    msh->Sel2Mask(dmask & 0xff0000,MSM_SET);
  }

  return msh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_SelectGrow(GenMesh *msh,sU32 smask,sU32 dmask,sInt dmode,sInt amount)
{
  if(CheckMesh(msh,smask<<8)) return 0;

  while(amount--)
    msh->SelectGrow();

  msh->Face2Vert();
  msh->Sel2Mask(dmask<<8,dmode);
  return msh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_Invert(GenMesh *msh)
{
  sInt i;
  GenMeshEdge *edg;
//  GenMeshColl *coll;

  if(CheckMesh(msh)) return 0;

  for(i=0;i<msh->Edge.Count;i++)
  {
    edg = &msh->Edge[i];
    edg->Temp[0] = msh->GetVertId(edg->Next[0]);
    edg->Temp[1] = msh->GetVertId(edg->Next[1]);
  }

  for(i=0;i<msh->Edge.Count;i++)
  {
    edg = &msh->Edge[i];
    sSwap(edg->Next[0],edg->Prev[0]);
    sSwap(edg->Next[1],edg->Prev[1]);
    edg->Vert[0] = edg->Temp[0];
    edg->Vert[1] = edg->Temp[1];
  }

  msh->GotNormals = sFALSE;
  return msh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_SetPivot(GenMesh *msh,sInt attr,sF323 pivot)
{
  sInt j;

  if(CheckMesh(msh)) return 0;

  j = msh->VertMap(attr);
  if(j!=-1)
    msh->VertBuf[msh->AddPivot()*msh->VertSize()+j].Init(pivot.x,pivot.y,pivot.z,1.0f);

  return msh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_UVProjection(GenMesh *msh,sInt mask,sFSRT srt,sInt type)
{
  sInt i,sj;
  sVector n,*v0;
  sMatrix mat;
  //sInt sv,ind;
  //sVector *v1;
  //sF32 d;

  if(CheckMesh(msh)) return 0;

  mat.InitSRT(srt.v);

  if(!type || type == 3) // cubic projection mapping
    msh->CubicProjection(mat,mask,sGMI_UV0,type == 0);
  else // spherical or cylindrical projection
  {
    sj = msh->VertMap(sGMI_UV0);

    // make initial mapping
    /*for(i=0;i<msh->Vert.Count;i++)
      msh->Vert[i].Temp = 0;*/

    for(i=0;i<msh->Vert.Count;i++)
    {
      // get transformed source pos
      v0 = msh->VertBuf + i * msh->VertSize();
      n.Rotate34(mat,v0[0]);

      /*if(sFAbs(n.x) < 1e-6f) n.x = 0.0f;
      if(sFAbs(n.z) < 1e-6f) n.z = 0.0f;*/

      sF32 xzd = n.x*n.x+n.z*n.z;

      v0[sj].x = 0.5f-sFATan2(n.x,n.z)/sPI2F;
      if(type==1) // cylindrical
        v0[sj].y = -n.y;
      else
        v0[sj].y = 0.5f-sFATan2(n.y,sFSqrt(xzd))/sPIF;

      /*// tag poles as singular
      if(n.y / sFAbs(xzd) > 1e+6f)
      {
        msh->Vert[msh->Vert[i].First].Temp = 2;
        msh->Vert[i].Temp = 2;
      }*/
    }

    /*
    // tag singular verts
    for(i=0;i<msh->Edge.Count*2;i++)
    {
      v0 = msh->VertBuf + msh->GetVertId(i) * msh->VertSize();
      v1 = msh->VertBuf + msh->GetVertId(i^1) * msh->VertSize();
      d = -(v0[sj].x - v1[sj].x);

      if(d >= 0.5f) // singular edge
      {
        sv = msh->GetVertId(i);
        if(msh->Vert[msh->GetVert(i^1)->First].Temp != 2)
        {
          // tag vert as singular
          msh->Vert[msh->Vert[sv].First].Temp = 1;
          msh->Vert[sv].Temp = 1;
          msh->Edge[i/2].Temp[0] = i;
        }
        else
          msh->Edge[i/2].Temp[0] = -1;
      }
      else if(d > -0.5f)
        msh->Edge[i/2].Temp[0] = -1;
    }

    // tag crease edges
    for(i=0;i<msh->Edge.Count;i++)
    {
      msh->Edge[i].Select = msh->Vert[msh->GetVert(i*2)->First].Temp
      && msh->Vert[msh->GetVert(i*2+1)->First].Temp;
    }

    // make uv creases
    msh->Crease(1,0,0,sGMF_UV0);

    // adjust UVs
    for(i=0;i<msh->Edge.Count;i++)
    {
      ind = msh->Edge[i].Temp[0];
      if(ind != -1) // singular edge
      {
        if(msh->GetVert(ind)->Temp) // still singular vert
        {
          v0 = msh->VertBuf + msh->GetVertId(ind) * msh->VertSize();
          v0[sj].x += 1.0f;
          msh->GetVert(ind)->Temp = 0;
        }
      }
    }*/
  }

  return msh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_Center(GenMesh *mesh,sInt mask,sInt which)
{
  sMatrix mat;
  sAABox box;

  if(CheckMesh(mesh,mask<<16)) return 0;

  mesh->CalcBBox(box);
  mesh->All2Sel(1,MAS_VERT);

  mat.Init();
  if(which&1) mat.l.x = -0.5f * (box.Min.x + box.Max.x);
  if(which&2) mat.l.y = (which & 128) ? -box.Min.y : -0.5f * (box.Min.y + box.Max.y);
  if(which&4) mat.l.z = -0.5f * (box.Min.z + box.Max.z);

  mesh->TransVert(mat);
  return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_AutoCollision(GenMesh *mesh,sF32 enlarge,sInt mode,sInt tx,sInt ty,sInt tz,sInt outmask)
{
  sAABox box;
  GenMesh *coll;

  if(CheckMesh(mesh)) return 0;

  mesh->CalcBBox(box);
  
  coll = Mesh_CollisionCube(0,0,0,box.Min.x-enlarge,box.Max.x+enlarge,
    box.Min.y-enlarge,box.Max.y+enlarge,box.Min.z-enlarge,box.Max.z+enlarge,mode,tx,ty,tz);
  mesh->All2Mask(outmask<<16,MSM_SETNOT);
  coll->All2Mask(outmask<<16,MSM_SET);
  mesh->Add(coll);
  coll->Release();

  return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_SelectSphere(GenMesh *mesh,sU32 dmask,sInt dmode,sF323 center,sF323 size)
{
  if(CheckMesh(mesh)) return 0;
  size.x *= 0.5f;
  size.y *= 0.5f;
  size.z *= 0.5f;
  mesh->SelectSphere((sVector&)center.x,(sVector&)size.x,0,0);
  if(dmode&4)
    mesh->Face2Vert();
  mesh->Sel2Mask(dmask,dmode&3);

  return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_SelectFace(GenMesh *mesh,sU32 dmask,sInt dmode,sInt face)
{
  if(CheckMesh(mesh)) return 0;
  
  mesh->All2Sel(0,MAS_FACE);
  if(face<mesh->Face.Count && mesh->Face[face].Material)
    mesh->Face[face].Select = 1;
  mesh->Sel2Mask(dmask<<8,dmode);

  return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_SelectAngle(GenMesh *mesh,sU32 dmask,sInt dmode,sF322 dir,sF32 thresh)
{
  sInt i;
  sMatrix mat;
  sVector dirv;

  if(CheckMesh(mesh)) return 0;
  
  mesh->CalcNormals(0,7);
  thresh = (thresh - 0.5f) * 2.0f;
  mat.InitEuler(dir.x*sPI2F,0.0f,dir.y*sPI2F);
  dirv.Init(mat.i.y,mat.j.y,mat.k.y,0.0f);

  for(i=0;i<mesh->Vert.Count;i++)
    mesh->Vert[i].Select = mesh->VertNorm(i).Dot3(dirv) >= thresh;
  mesh->Vert2FaceEdge();
  if(dmode&4)
    mesh->Face2Vert();
  mesh->Sel2Mask(dmask,dmode&3);

  return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_Bend2(GenMesh *mesh,sF323 center,sF323 rotate,sF32 len,sF32 angle)
{
  sMatrix mt,mb;
  sVector vt,*vp;
  sF32 vx,vy,t,sa,ca;
  sInt i;

  if(CheckMesh(mesh)) return 0;

  mt.InitEulerPI2(&rotate.x);
  mt.l.x = -center.x;
  mt.l.y = -center.y;
  mt.l.z = -center.z;
  mb = mt;
  mb.TransR();
  angle *= sPI2F;

  for(i=0;i<mesh->Vert.Count;i++)
  {
    vp = mesh->VertBuf + i * mesh->VertSize();
    vt.Rotate34(mt,vp[0]);
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

    vp[0].Rotate34(mb,vt);
  }
  mesh->GotNormals = sFALSE;

  return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_SmoothAngle(GenMesh *mesh,sF32 angle)
{
  sInt i;
  sVector n1,n2;

  if(CheckMesh(mesh))
    return 0;

  angle = sFCos(angle * sPI2F * 0.25f);
  for(i=0;i<mesh->Edge.Count;i++)
  {
    mesh->CalcFaceNormalAccurate(n1,mesh->GetFaceId(i*2+0));
    mesh->CalcFaceNormalAccurate(n2,mesh->GetFaceId(i*2+1));
    mesh->Edge[i].Select = sFAbs(n1.Dot3(n2)) < angle;
  }
  mesh->Crease(1,0,0,sGMF_NORMAL|sGMF_TANGENT);
  mesh->GotNormals = sFALSE;

  return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_Color(GenMesh *mesh,sF323 pos,sF322 dir,sU32 color,sF32 amplify,sF32 range,sInt op)
{
  sInt i,j,sc,sn;
  sMatrix mat;
  sVector *vt,*vl,lightDir,d,lightCol;
  sF32 intens,len,rsq,invr;

  if(CheckMesh(mesh))
    return 0;

  mesh->NeedAllNormals();
  mat.InitEuler(dir.x*sPI2F,0.0f,dir.y*sPI2F);
  lightDir.Init(mat.i.y,mat.j.y,mat.k.y,0.0f);
  lightCol.InitColor(color);
  lightCol.Scale4(amplify);
  rsq = range * range;
  invr = 1.0f / range;

  sc = mesh->VertMap(sGMI_COLOR0);
  sn = mesh->VertMap(sGMI_NORMAL);

  for(i=0;i<mesh->Vert.Count;i++)
  {
    vt = mesh->VertBuf + i * mesh->VertSize();

    switch(op)
    {
    case 0: // light directional
      intens = lightDir.Dot3(vt[sn]);
      if(intens > 0.0f)
        vt[sc].AddScale3(lightCol,intens);
      break;

    case 1: // light point
      d.Sub3((const sVector&) pos,vt[0]);
      len = d.Dot3(d);
      if(len < rsq)
      {
        intens = d.Dot3(vt[sn]);
        if(intens > 0.0f)
          vt[sc].AddScale3(lightCol,intens * (sFInvSqrt(len) - invr));
      }
      break;

    case 2: // render slots
      for(j=0;j<mesh->Lgts.Count;j++)
      {
        vl = mesh->VertBuf + mesh->Lgts[j] * mesh->VertSize();
        rsq = vl[sn+1].y;
        d.z = vl[0].z-vt[0].z; len  = d.z*d.z;
        if(len < rsq)
        {
          d.x = vl[0].x-vt[0].x; len += d.x*d.x;
          d.y = vl[0].y-vt[0].y; len += d.y*d.y;
          if(len < rsq)
          {
            intens = d.x*vt[sn].x + d.y*vt[sn].y + d.z*vt[sn].z;
            if(intens > 0.0f)
              vt[sc].AddScale3(vl[sc],intens * (sFInvSqrt(len) - vl[sn+1].z));
          }
        }
      }
      //mesh->Lgts.Count = 0;
      break;

    case 3: // ambient light
      vt[sc].Add3(lightCol);
      break;
    }
  }

  return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_BendS(GenMesh *mesh,sF323 anchor,sF323 rotate,sF32 len,KSpline *spline)
{
  sMatrix mt;
  static sMatrix mat[129];
  sVector vt,pt,dir,v0,v1,*vp;
  sInt i,it;
  sF32 t;

  if(!spline || CheckMesh(mesh)) return 0;

  mt.InitEulerPI2(&rotate.x);
  mt.l.x = -anchor.x;
  mt.l.y = -anchor.y;
  mt.l.z = -anchor.z;

  for(i=0;i<128;i++)
  {
    spline->Eval(i/128.0f,pt);
    spline->Eval((i+1)/128.0f,dir);
    dir.Sub3(pt);
    pt.w = 1.0f;
    mat[i].InitDir(dir);
    mat[i].l = pt;
  }

  spline->Eval(1.0f,pt);
  pt.w = 1.0f;
  mat[128].InitDir(dir);
  mat[128].l = pt;

  for(i=0;i<mesh->Vert.Count;i++)
  {
    vp = mesh->VertBuf + i * mesh->VertSize();
    vt.Rotate34(mt,vp[0]);
    t = vt.z;
    if(t>=0.0f)
      vt.z -= sMin(t,len);

    t = sRange(t/len,1.0f,0.0f) * 128.0f;
    it = t;
    t -= it;

    v0.Rotate34(mat[it+0],vt);
    v1.Rotate34(mat[it+1],vt);
    vp[0].Lin3(v0,v1,t);
  }
  mesh->GotNormals = sFALSE;

  return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_LightSlot(sF323 pos,sU32 color,sF32 amplify,sF32 range)
{
  GenMesh *mesh;

  mesh = new GenMesh;
  mesh->Init(sGMF_DEFAULT,1);

  mesh->AddNewVert();
  *mesh->Lgts.Add() = 0;
  mesh->VertBuf[mesh->VertMap(sGMI_POS)].Init(pos.x,pos.y,pos.z,1);
  mesh->VertBuf[mesh->VertMap(sGMI_COLOR0)].InitColor(color);
  mesh->VertBuf[mesh->VertMap(sGMI_COLOR0)].Scale4(amplify);
  mesh->VertBuf[mesh->VertMap(sGMI_TANGENT)].Init(range,range*range,1.0f/range,0);

  return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_ShadowEnable(GenMesh *mesh,sBool enable)
{
  sInt i;

  if(CheckMesh(mesh)) return 0;

  for(i=0;i<mesh->Face.Count;i++)
    mesh->Face[i].Used = enable;

  return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_Multiply2(sInt seed,sInt3 count1,sF323 translate1,sInt3 count2,sF323 translate2,sInt random,sInt3 count3,sF323 translate3,sInt inCount,GenMesh *inMesh,...)
{
  if(!inCount)
    return 0;

  GenMesh *mesh = new GenMesh;
  mesh->Init(sGMF_DEFAULT,1024);
  sSetRndSeed(seed);

  sInt lastStart,start = 0;
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

                    lastStart = start;
                    start = mesh->Vert.Count;

                    GenMesh *in = (&inMesh)[sMin<sInt>(sGetRnd(inCount+random),inCount-1)];
                    mesh->Add(in,!!start);

                    for(sInt i=lastStart;i<start;i++)
                      mesh->Vert[i].Select = 0;

                    for(sInt i=start;i<mesh->Vert.Count;i++)
                      mesh->Vert[i].Select = 1;

                    xform.l.x = x1 * translate1.x + x2 * translate2.x + x3 * translate3.x;
                    xform.l.y = y1 * translate1.y + y2 * translate2.y + y3 * translate3.y;
                    xform.l.z = z1 * translate1.z + z2 * translate2.z + z3 * translate3.z;
                    mesh->TransVert(xform);

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

  mesh->GotNormals = sFALSE;
  return mesh;
}

/****************************************************************************/

GenSimpleMesh * __stdcall Mesh_BSP(GenMesh *mesh)
{
  GenSimpleBrush ba,bb;
  sAABox box;

  if(!mesh)
    return 0;

  box.Min.Init(-1.0f,-1.0f,-1.0f,1.0f);
  box.Max.Init( 1.0f, 1.0f, 1.0f,1.0f);
  ba.Cube(box);

  box.Min.Init( 1.0f,-1.0f,-1.0f,1.0f);
  box.Max.Init( 3.0f, 1.0f, 1.0f,1.0f);
  bb.Cube(box);

  ba.CSGAgainst(bb,sTRUE);
  bb.CSGAgainst(ba,sTRUE);

  GenSimpleMesh *result = new GenSimpleMesh;
  result->Add(ba.Mesh);
  result->Add(bb.Mesh);

  return result;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_XSI(sChar *filename)
{
  GenMesh *mesh;
#if sLINK_XSI
  sXSILoader loader;

  if(loader.LoadXSI(filename))
  {
    loader.Optimise();

    mesh = new GenMesh;
    mesh->Init(sGMF_DEFAULT,1024);
    mesh->ImportXSI(&loader);
  }
  else
#endif
    mesh = 0;

  return mesh;
}

/****************************************************************************/

GenMesh * __stdcall Mesh_SingleVert()
{
  GenMesh *mesh = new GenMesh;
  mesh->Init(sGMF_DEFAULT,3);
  mesh->Ring(3,0,0,0);
  mesh->Face[1].Material = 0;
  mesh->Vert[0].Next = 1;
  mesh->Vert[1].First = 0;
  mesh->Vert[1].Next = 2;
  mesh->Vert[2].First = 0;
  mesh->Vert[2].Next = 0;

  return mesh;
}

/****************************************************************************/

static void ToMinVector(GenMinVector &dst,const sVector &src)
{
  dst.x = src.x;
  dst.y = src.y;
  dst.z = src.z;
}

static void ToUV(sF32 *dst,const sVector &src)
{
  dst[0] = src.x;
  dst[1] = src.y;
}

#if sLINK_MINMESH
GenMinMesh * __stdcall Mesh_ToMin(GenMesh *mesh)
{
  if(CheckMesh(mesh))
    return 0;

  // triangulate anything with >=8 vertices
  mesh->Triangulate(KMM_MAXVERT);

  GenMinMesh *minMesh = new GenMinMesh;
  mesh->All2Sel(1,MAS_FACE);
  mesh->Face2Vert();

  // count output vertices
  sInt outVerts = 0;
  for(int i=0;i<mesh->Vert.Count;i++)
    outVerts += mesh->Vert[i].Select;

  minMesh->Vertices.Resize(outVerts);

  // remap vertices
  outVerts = 0;

  sInt vs = mesh->VertSize();
  sInt vc0 = mesh->VertMap(sGMI_COLOR0);
  sInt vu0 = mesh->VertMap(sGMI_UV0);
  sInt vu1 = mesh->VertMap(sGMI_UV1);
  sVector *vb = mesh->VertBuf;

  for(sInt i=0;i<mesh->Vert.Count;i++)
  {
    GenMeshVert *vert = &mesh->Vert[i];
    if(!vert->Select)
    {
      vert->Temp = -1;
      continue;
    }

    GenMinVert *outVert = &minMesh->Vertices[outVerts];
    vert->Temp = outVerts;

    outVert->Select = 0;
    outVert->BoneCount = 0;
    outVert->Color = (vc0 == -1) ? 0xffffffff : vb[i * vs + vc0].GetColor();
    ToMinVector(outVert->Pos,vb[vert->First * vs]);
    if(vu0 != -1) ToUV(outVert->UV[0],vb[i * vs + vu0]);
    if(vu1 != -1) ToUV(outVert->UV[1],vb[i * vs + vu1]);

    outVerts++;
  }

  // convert materials to clusters
  for(sInt i=0;i<minMesh->Clusters.Count;i++)
  {
    GenMinCluster *cluster = &minMesh->Clusters[i];

    if(cluster->Mtrl)
    {
      cluster->Mtrl->Release();
      cluster->Mtrl = 0;
    }
  }

  minMesh->Clusters.Resize(mesh->Mtrl.Count);
  for(sInt i=0;i<mesh->Mtrl.Count;i++)
  {
    GenMeshMtrl *inMtrl = &mesh->Mtrl[i];
    GenMinCluster *outCluster = &minMesh->Clusters[i];

    outCluster->Mtrl = inMtrl->Material;
    outCluster->RenderPass = inMtrl->Pass;
    outCluster->Id = i;
    outCluster->AnimType = 0;
    outCluster->AnimMatrix = -1;

    if(outCluster->Mtrl)
      outCluster->Mtrl->AddRef();
  }

  // count faces not deleted
  sInt outFaceCount = 0;
  for(sInt i=0;i<mesh->Face.Count;i++)
    outFaceCount += mesh->Face[i].Material != 0;

  // convert faces
  minMesh->Faces.Resize(outFaceCount);
  GenMinFace *outFace = &minMesh->Faces[0];

  for(sInt i=0;i<mesh->Face.Count;i++)
  {
    GenMeshFace *inFace = &mesh->Face[i];
    if(!inFace->Material)
      continue;

    outFace->Select = 0;
    outFace->Count = 0;
    outFace->Cluster = inFace->Material;

    sInt e,ee;
    e = ee = inFace->Edge;
    do
    {
      sVERIFY(mesh->GetVert(e)->Temp!=-1);
      outFace->Vertices[outFace->Count] = mesh->GetVert(e)->Temp;
      outFace->Count++;
      e = mesh->NextFaceEdge(e);
    }
    while(e != ee && outFace->Count < 8);

    outFace++;
  }

  // update+cleanup
  minMesh->ChangeTopo();
  mesh->Release();

  return minMesh;
}
#endif

/****************************************************************************/
/****************************************************************************/

#if sLINK_XSI

void GenMesh::ImportXSI(sXSILoader *xsi)
{
  sInt i,j,k,fi;
  sInt first;
  sXSIModel *xm;
  sXSICluster *xc;
  sXSIFCurve *xf;

  sVERIFY(KeyCount==0);
  sVERIFY(CurveCount==0);
  sVERIFY(KeyBuf==0);
  KeyCount = 0;
  CurveCount = 0;
  ImportXSIR(xsi->RootModel,-1);
  KeyBuf = new sF32[KeyCount*CurveCount];
  sSetMem4((sU32 *)KeyBuf,0,KeyCount*CurveCount);

  for(i=0;i<xsi->Models->GetCount();i++)
  {
    xm = xsi->Models->Get(i);
    first = Vert.Count;
    for(j=0;j<xm->Clusters->GetCount();j++)
    {
      xc = xm->Clusters->Get(j);
      ImportXSI(xc,first);
    }
    Verify();

    for(j=0;j<xm->FCurves->GetCount();j++)
    {
      xf = xm->FCurves->Get(j);
      fi = xf->Index;
      sVERIFY(fi>=0 && fi<CurveCount);
      BoneCurve[fi].Curve = xf->Offset;
      BoneCurve[fi].Matrix = xm->Index;
      for(k=0;k<xf->KeyCount;k++)
      {
        sVERIFY(xf->Keys[k].Num>=0 && xf->Keys[k].Num<KeyCount);
        KeyBuf[KeyCount*fi+xf->Keys[k].Num] = xf->Keys[k].Pos;
      }
    }
  }
}

void GenMesh::ImportXSIR(sXSIModel *xm,sInt parent)
{
  sInt i,cm,cc,fcc;
  GenMeshMatrix *mat;
  sXSIFCurve *xfc;
  
  cm = BoneMatrix.Count;
  xm->Index = cm;
  BoneMatrix.AtLeast(cm+1);
  BoneMatrix.Count = cm+1;
  mat = &BoneMatrix[cm];
  mat->BaseSRT[0] = xm->BaseS.x;
  mat->BaseSRT[1] = xm->BaseS.y;
  mat->BaseSRT[2] = xm->BaseS.z;
  mat->BaseSRT[3] = xm->BaseR.x;
  mat->BaseSRT[4] = xm->BaseR.y;
  mat->BaseSRT[5] = xm->BaseR.z;
  mat->BaseSRT[6] = xm->BaseT.x;
  mat->BaseSRT[7] = xm->BaseT.y;
  mat->BaseSRT[8] = -xm->BaseT.z;
  mat->TransSRT[0] = xm->TransS.x;
  mat->TransSRT[1] = xm->TransS.y;
  mat->TransSRT[2] = xm->TransS.z;
  mat->TransSRT[3] = xm->TransR.x;
  mat->TransSRT[4] = xm->TransR.y;
  mat->TransSRT[5] = xm->TransR.z;
  mat->TransSRT[6] = xm->TransT.x;
  mat->TransSRT[7] = xm->TransT.y;
  mat->TransSRT[8] = -xm->TransT.z;
  mat->Matrix.Init();
  mat->Parent = parent;
  mat->Used = 0;

  fcc = xm->FCurves->GetCount();
  if(fcc>0)
  {
    cc = BoneCurve.Count;
    BoneCurve.AtLeast(cc+fcc);
    BoneCurve.Count = cc+fcc;
    for(i=0;i<fcc;i++)
    {
      xfc = xm->FCurves->Get(i);
      KeyCount = sMax(KeyCount,xfc->Keys[xfc->KeyCount-1].Num+1);
      xfc->Index = cc+i;
    }
    CurveCount+=fcc;
  }

  for(i=0;i<xm->Childs->GetCount();i++)
    ImportXSIR(xm->Childs->Get(i),cm);
}

static sInt VectorHash(const sVector &v)
{
  sU32 *bits = (sU32 *) &v.x;

  // FNV-based hash on x,y,z
  sU32 hash = 0x9dc5;
  hash = (hash ^ bits[0]) * 0x0193;
  hash = (hash ^ bits[1]) * 0x0193;
  hash = (hash ^ bits[2]) * 0x0193;

  return hash & 0x3ff;
}

void GenMesh::ImportXSI(sXSICluster *xc,sInt first)
{
  sInt i,j,vc,fc,max;
  sVector v;
  sVector *vp;
  sInt vi,vj,fi;
  sInt *fp;
  sInt edges[64];
  sInt hashbucket[1024],vhash;

  sDPrintF("cluster %08x\n",xc);

  vc = Vert.Count;

  Vert.AtLeast(vc+xc->VertexCount);
  Realloc(vc+xc->VertexCount);

// clear hash buckets
  for(i=0;i<1024;i++)
    hashbucket[i] = -1;

// add vertices

  for(i=0;i<xc->VertexCount;i++)
  {
    v = xc->Vertices[i].Pos;
    vhash = VectorHash(v);

    for(j=hashbucket[vhash];j!=-1;j=Vert[j].Temp)
    {
      vp = &VertPos(j);
      if(v.x == vp->x && v.y == vp->y && -v.z == vp->z)
      {
        vi = j;
        goto vertfound;
      }
    }

    vi = Vert.Count;

vertfound:
    vj = Vert.Count++;
    xc->Vertices[i].Temp = vj;
    vp = &VertPos(vj);
    *vp = xc->Vertices[i].Pos;
    vp->z = -vp->z;
    vp->w = 1;
    vp++;
    Vert[vj].Init();
    Vert[vj].Temp2 = -1; // used for addedge
    if(vi == vj) // vertex was new, add to hash table
    {
      Vert[vi].Temp = hashbucket[vhash];
      hashbucket[vhash] = vi;
    }
    for(j=0;j<4;j++)
    {
      if(j<xc->Vertices[i].WeightCount)
      {
        Vert[vj].Matrix[j] = xc->Vertices[i].WeightModel[j]->Index;
        Vert[vj].Weight[j] = xc->Vertices[i].Weight[j]*255/100;
      }
      else
      {
        Vert[vj].Matrix[j] = 0xff;
        Vert[vj].Weight[j] = 0x00;
      }
    }
    if(VertMask() & sGMF_NORMAL)
    {
      *vp = xc->Vertices[i].Normal;
      vp->w = 0;
      vp++;
    }
    if(VertMask() & sGMF_TANGENT)
    {
      vp->Init4(0,0,0,0);
      vp++;
    }
    if(VertMask() & sGMF_COLOR0)
    {
      vp->Init4(((xc->Vertices[i].Color>>16)&0xff)/255.0f,
                ((xc->Vertices[i].Color>> 8)&0xff)/255.0f,
                ((xc->Vertices[i].Color    )&0xff)/255.0f,
                ((xc->Vertices[i].Color>>24)&0xff)/255.0f);
      vp++;
    }
    if(VertMask() & sGMF_COLOR1)
    {
      vp->Init4(1,1,1,1);
      vp++;
    }
    if(VertMask() & sGMF_UV0)
    {
      vp->Init(xc->Vertices[i].UV[0][0],xc->Vertices[i].UV[0][1],0,0);
      //vp->Init(xc->Vertices[i].Pos.x*0.1,xc->Vertices[i].Pos.y*0.1,0,0);
      vp++;
    }
    if(VertMask() & sGMF_UV1)
    {
      vp->Init(xc->Vertices[i].UV[1][0],xc->Vertices[i].UV[1][1],0,0);
      vp++;
    }
    if(VertMask() & sGMF_UV2)
    {
      vp->Init(xc->Vertices[i].UV[2][0],xc->Vertices[i].UV[2][1],0,0);
      vp++;
    }
    if(VertMask() & sGMF_UV3)
    {
      vp->Init(xc->Vertices[i].UV[3][0],xc->Vertices[i].UV[3][1],0,0);
      vp++;
    }
    sVERIFY(vp==&VertBuf[(vj+1)*VertSize()]);
    if(vi==vj)
    {
       Vert[vj].Next = vi;
       Vert[vi].First = vj;
    }
    else
    {
      sVERIFY(Vert[vi].First == vi);
      Vert[vj].Next = Vert[vi].Next;
      Vert[vj].First = vi;
      Vert[vi].Next = vj;
    }
  }

// add faces

  fp = xc->Faces;
  fc = 0;

  while(fp<xc->Faces+xc->IndexCount*2)
  {
    max = *fp;
    sVERIFY(max>0);
    sVERIFY(max<64);
    fi = Face.Count;
    Face.AtLeast(fi+1);
    Face.Count=fi+1;
    Face[fi].Init();
    Face[fi].Material = 1;
    vi = fp[1];//max*2-1];
 
    for(i=0;i<max;i++)
    {
      sVERIFY(i==0 || fp[i*2]==0);
      vj = vi;
      vi = fp[max*2-1-2*i];
      edges[i] = AddEdge(xc->Vertices[vj].Temp,xc->Vertices[vi].Temp,fi);
    }
    fp += max*2;

    for(i=0;i<max;i++)
    {
      j = edges[i];
      Edge[j/2].Next[j&1] = edges[(i+1    )%max];
      Edge[j/2].Prev[j&1] = edges[(i-1+max)%max];
    }
  }
}

#endif

/****************************************************************************/
/****************************************************************************/

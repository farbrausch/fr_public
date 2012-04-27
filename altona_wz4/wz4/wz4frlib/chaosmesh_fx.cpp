/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "chaosmesh_fx.hpp"
#include "wz4frlib/chaosmesh_code.hpp"

static const sF32 ClassifyEpsilon = 0.0001f;

/****************************************************************************/

struct ChaosFXTri
{
  sInt Cluster;
  sInt Vertex[3];
};

class ChaosFXMesh
{
public:
  sArray<ChaosMeshVertexFat> Vertices;
  sArray<ChaosFXTri> Tris;

  void LoadFrom(const ChaosMesh *mesh);
  void StoreTo(ChaosMesh *mesh);

  sInt AddVert(const ChaosMesh *source,sInt pos,sInt norm,sInt prop,sInt tang);
  sInt AddTri(sInt v0,sInt v1,sInt v2,sInt cluster);
  void RemoveDoubleVerts();

  // doesn't actually change topology, just builds a new vertex
  sInt SplitEdgeAlongPlane(sInt va,sInt vb,const sVector4 &plane);
};

void ChaosFXMesh::LoadFrom(const ChaosMesh *mesh)
{
  Vertices.Clear();
  Tris.Clear();

  for(sInt i=0;i<mesh->Faces.GetCount();i++)
  {
    const ChaosMeshFace *face = &mesh->Faces[i];
    sInt verts[4];

    for(sInt j=0;j<face->Count;j++)
      verts[j] = AddVert(mesh,face->Positions[j],face->Normals[j],face->Property[j],face->Tangents[j]);

    for(sInt j=2;j<face->Count;j++)
      AddTri(verts[0],verts[j-1],verts[j],face->Cluster);
  }

  RemoveDoubleVerts();
}

void ChaosFXMesh::StoreTo(ChaosMesh *mesh)
{
  mesh->Positions.Clear();
  mesh->Normals.Clear();
  mesh->Tangents.Clear();
  mesh->Properties.Clear();
  mesh->Faces.Clear();

  for(sInt i=0;i<Vertices.GetCount();i++)
  {
    const ChaosMeshVertexFat *src = &Vertices[i];

    ChaosMeshVertexPosition pos;
    pos.Position = src->Position;
    for(sInt i=0;i<CHAOSMESHWEIGHTS;i++)
    {
      pos.MatrixIndex[i] = src->MatrixIndex[i];
      pos.MatrixWeight[i] = src->MatrixWeight[i];
    }
    pos.Select = 0;
    mesh->Positions.AddTail(pos);

    ChaosMeshVertexNormal normal;
    normal.Normal = src->Normal;
    mesh->Normals.AddTail(normal);

    ChaosMeshVertexTangent tangent;
    tangent.Tangent = src->Tangent;
    tangent.BiSign = src->BiSign;
    mesh->Tangents.AddTail(tangent);

    ChaosMeshVertexProperty prop;
    prop.C[0] = src->C[0];
    prop.C[1] = src->C[1];
    for(sInt i=0;i<4;i++)
    {
      prop.U[i] = src->U[i];
      prop.V[i] = src->V[i];
    }
    mesh->Properties.AddTail(prop);
  }

  for(sInt i=0;i<Tris.GetCount();i++)
  {
    const ChaosFXTri *tri = &Tris[i];
    ChaosMeshFace face;

    face.Cluster = tri->Cluster;
    face.Count = 3;
    face.Select = 0;
    for(sInt j=0;j<3;j++)
    {
      face.Positions[j] = tri->Vertex[j];
      face.Normals[j] = tri->Vertex[j];
      face.Property[j] = tri->Vertex[j];
      face.Tangents[j] = tri->Vertex[j];
    }

    mesh->Faces.AddTail(face);
  }

  mesh->FlushBuffers();
}

sInt ChaosFXMesh::AddVert(const ChaosMesh *source,sInt pos,sInt norm,sInt prop,sInt tang)
{
  ChaosMeshVertexFat fat;

  const ChaosMeshVertexPosition *poss = &source->Positions[pos];
  fat.Position = poss->Position;
  for(sInt i=0;i<CHAOSMESHWEIGHTS;i++)
  {
    fat.MatrixIndex[i] = poss->MatrixIndex[i];
    fat.MatrixWeight[i] = poss->MatrixWeight[i];
  }
  
  fat.Normal = source->Normals[norm].Normal;
  fat.Tangent = source->Tangents[tang].Tangent;
  fat.BiSign = source->Tangents[tang].BiSign;

  const ChaosMeshVertexProperty *props = &source->Properties[prop];
  fat.C[0] = props->C[0];
  fat.C[1] = props->C[1];
  for(sInt i=0;i<4;i++)
  {
    fat.U[i] = props->U[i];
    fat.V[i] = props->V[i];
  }

  Vertices.AddTail(fat);
  return Vertices.GetCount() - 1;
}

sInt ChaosFXMesh::AddTri(sInt v0,sInt v1,sInt v2,sInt cluster)
{
  ChaosFXTri tri;
  tri.Cluster = cluster;
  tri.Vertex[0] = v0;
  tri.Vertex[1] = v1;
  tri.Vertex[2] = v2;
  Tris.AddTail(tri);
  return Tris.GetCount() - 1;
}

void ChaosFXMesh::RemoveDoubleVerts()
{
  for(sInt i=0;i<Vertices.GetCount();i++)
    Vertices[i].Index = i;

  sHeapSortUp(Vertices);

  sInt *vertMap = new sInt[Vertices.GetCount()];
  sInt outInd = 0;
  for(sInt i=0;i<Vertices.GetCount();i++)
  {
    if(i!=0 && Vertices[i-1] < Vertices[i])
      outInd++;

    vertMap[Vertices[i].Index] = outInd;
    Vertices[outInd] = Vertices[i];
  }
  Vertices.Resize(outInd+1);

  for(sInt i=0;i<Tris.GetCount();i++)
    for(sInt j=0;j<3;j++)
      Tris[i].Vertex[j] = vertMap[Tris[i].Vertex[j]];

  delete[] vertMap;
}

sInt ChaosFXMesh::SplitEdgeAlongPlane(sInt vai,sInt vbi,const sVector4 &plane)
{
  if(vai > vbi)
    sSwap(vai,vbi);

  ChaosMeshVertexFat& va = Vertices[vai];
  ChaosMeshVertexFat& vb = Vertices[vbi];

  sF32 ta = va.Position^plane;
  sF32 tb = vb.Position^plane;
  sVERIFY(sFAbs(ta - tb) >= 2.0f*ClassifyEpsilon);

  sF32 t = -ta / (tb - ta);
  sVERIFY(t >= 0.0f && t <= 1.0f);

  ChaosMeshVertexFat dest;
  dest.Position.Fade(t,va.Position,vb.Position);
  dest.Normal.Fade(t,va.Normal,vb.Normal);      dest.Normal.Unit();
  dest.Tangent.Fade(t,va.Tangent,vb.Tangent);   dest.Tangent.Unit();
  dest.BiSign = va.BiSign;

  for(sInt i=0;i<sMVF_COLORS;i++)
  {
    sVector4 ac,bc,oc;

    ac.InitColor(va.C[i]);
    bc.InitColor(vb.C[i]);
    oc = (1.0f-t)*ac + t*bc;
    dest.C[i] = oc.GetColor();
  }

  for(sInt i=0;i<sMVF_UVS;i++)
  {
    dest.U[i] = va.U[i] + t * (vb.U[i] - va.U[i]);
    dest.V[i] = va.V[i] + t * (vb.V[i] - va.V[i]);
  }

  Vertices.AddTail(dest);
  return Vertices.GetCount() - 1;
}

/****************************************************************************/

static sInt ClassifyPoint(const sVector31 &point, const sVector4 &plane)
{
  sF32 dp = point^plane;
  return (dp > ClassifyEpsilon) ? 1 : (dp < -ClassifyEpsilon) ? -1 : 0;
}

static sInt SplitAlongPlane(ChaosFXMesh *mesh, sArray<sInt>& faces, const sVector4& plane)
{
  sArray<sInt> newFaces;
  sInt nFrontFaces = 0;

  newFaces.HintSize(faces.GetCount() + (faces.GetCount() >> 3));
  sArray<ChaosFXTri> inTris;
  inTris.Swap(mesh->Tris);

  for(sInt i=0;i<faces.GetCount();i++)
  {
    sInt backVert[4], frontVert[4], backCount = 0, frontCount = 0;
    sInt realBack = 0, realFront = 0;

    sInt faceI = faces[i], cls, lastCls, vi, vl, vn;
    vl = inTris[faceI].Vertex[2];
    lastCls = ClassifyPoint(mesh->Vertices[vl].Position, plane);

    // actual clipping loop
    for(sInt j=2,k=0; k<3; j=k,lastCls=cls,vl=vi,k++)
    {
      vi = inTris[faceI].Vertex[k];
      cls = ClassifyPoint(mesh->Vertices[vi].Position, plane);

      if(cls == -1)
        realBack++;
      else if(cls == 1)
        realFront++;

      switch(lastCls * 3 + cls)
      {
      case -1*3 - 1: // back->back
        backVert[backCount++] = vi;
        break;

      case -1*3 + 0: // back->on
        backVert[backCount++]  = vi;
        frontVert[frontCount++] = vi;
        break;

      case -1*3 + 1: // back->front
        vn = mesh->SplitEdgeAlongPlane(vl, vi, plane);
        backVert[backCount++] = vn;
        frontVert[frontCount++] = vn;
        frontVert[frontCount++] = vi;
        break;

      case 0*3 - 1: // on->back
        backVert[backCount++] = vi;
        break;

      case 0*3 + 0: // on->on
        // i'm pretty certain this is bogus. fixme later.
        backVert[backCount++] = vi;
        frontVert[frontCount++] = vi;
        break;

      case 0*3 + 1: // on->front
        frontVert[frontCount++] = vi;
        break;

      case 1*3 - 1: // front->back
        vn = mesh->SplitEdgeAlongPlane(vl, vi, plane);
        frontVert[frontCount++] = vn;
        backVert[backCount++] = vn;
        backVert[backCount++] = vi;
        break;

      case 1*3 + 0: // front->on
        frontVert[frontCount++] = vi;
        backVert[backCount++] = vi;
        break;

      case 1*3 + 1: // front->front
        frontVert[frontCount++] = vi;
        break;
      }
    }

    // generate tris
    if(realFront)
    {
      for(sInt i=2;i<frontCount;i++)
      {
        sInt newTri = mesh->AddTri(frontVert[0],frontVert[i-1],frontVert[i],inTris[faceI].Cluster);
        newFaces.AddTail(newTri);
        sSwap(newFaces[nFrontFaces++],newFaces.GetTail());
      }
    }

    if(realBack)
    {
      for(sInt i=2;i<backCount;i++)
      {
        sInt newTri = mesh->AddTri(backVert[0],backVert[i-1],backVert[i],inTris[faceI].Cluster);
        newFaces.AddTail(newTri);
      }
    }
  }

  newFaces.Swap(faces);
  return nFrontFaces;
}

void ChaosMeshFX_SliceAndDice(ChaosMesh *mesh)
{
  ChaosFXMesh fxm;
  sVector4 plane(-1,1,0,0);

  fxm.LoadFrom(mesh);

  sArray<sInt> faces;
  for(sInt i=0;i<fxm.Tris.GetCount();i++)
    faces.AddTail(i);
  SplitAlongPlane(&fxm,faces,plane);

  fxm.StoreTo(mesh);
}

/****************************************************************************/


/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4_bsp.hpp"
#include "wz4_bsp_ops.hpp"

#define RANDOMPLANES 1

/****************************************************************************/

const sChar *Wz4BSPGetErrorString(Wz4BSPError err)
{
  switch(err)
  {
  case WZ4BSP_OK:                   return L"No error"; break;
  case WZ4BSP_NONPLANAR_INPUT:      return L"Nonplanar input (try increasing epsilon)"; break;
  case WZ4BSP_NONPLANAR_POLY:       return L"Cannot produce planar polygon - increase epsilon."; break;
  case WZ4BSP_DEGENERATE_POLY:      return L"Unfixable degenerate polygon produced - increase epsilon."; break;
  case WZ4BSP_CANT_HIT_CLIP_PLANE:  return L"Cannot hit clip plane in SplitEdgeAlongPlane - increase epsilon."; break;
  case WZ4BSP_CANT_HIT_BOTH_PLANES: return L"Cannot hit both clip and poly plane in SplitEdgeAlongPlane - increase epsilon!"; break;
  case WZ4BSP_ROUNDOFF_PROBLEM:     return L"Nonplanar polygons due to roundoff drift (try increasing epsilon)"; break;
  case WZ4BSP_WHAT_THE_FUCK:        return L"\"What the fuck?\" error. Give a reproduce to ryg ASAP."; break;
  default:                          return L"This error code doesn't exist. Go away.";
  }
}

/****************************************************************************/

static sInt ClassifyPoint(const sVector31 &p,const sVector4 &plane,sF32 eps)
{
  sF32 dp = p ^ plane;
  return (dp > eps) ? 1 : (dp < -eps) ? -1 : 0;
}

static Wz4BSPError SplitEdgeAlongPlane(const sVector31 &a,const sVector31 &b,const sVector4 &plane,const sVector4 &projPlane,sVector31 &out,sF32 eps)
{
  sF32 da = a ^ plane;
  sF32 db = b ^ plane;
  sVERIFY(sSign(da) != sSign(db) && sFAbs(da-db) >= 2.0f*eps);

  sF32 t = da / (da - db); // =(0-da) / (db-da)
  sVERIFY(t >= 0.0f && t <= 1.0f);

  out = sVector31((1.0f - t) * a + t * b);
  if(ClassifyPoint(out,plane,eps) != 0) // if the point doesn't lie on the plane, we're screwed.
    return WZ4BSP_CANT_HIT_CLIP_PLANE;

  // it gets even trickier, though, since we want it to lie on the plane of the original
  // face, too... cue ominous music.
  if(ClassifyPoint(out,projPlane,eps) != 0)
  {
    // move it to halfway between both planes, cross fingers and hope everything works out
    sF32 adjust = 0.5f * (out ^ projPlane);
    out -= adjust * sVector30(projPlane);

    if(ClassifyPoint(out,plane,eps) != 0 || ClassifyPoint(out,projPlane,eps) != 0)
      return WZ4BSP_CANT_HIT_BOTH_PLANES;
  }

  return WZ4BSP_OK;
}

static void SolveCoordFromPlane(sVector31 &p,const sVector4 &plane,sInt axis)
{
  switch(axis)
  {
  case 0: p.x = (-plane.w - p.y*plane.y - p.z*plane.z) / plane.x; break;
  case 1: p.y = (-plane.w - p.z*plane.z - p.x*plane.x) / plane.y; break;
  case 2: p.z = (-plane.w - p.x*plane.x - p.y*plane.y) / plane.z; break;
  default: sVERIFYFALSE;
  }
}

static void CalcChildBB(const sAABBox &parentBB,const sVector4 &plane,sAABBox *bb,sInt &emptyFlag)
{
  bb[0] = bb[1] = parentBB;

  sInt normalAxis = sVector30(plane).MaxAxisAbs();
  sInt sign = plane[normalAxis] < 0.0f;

  sVector31 minp = parentBB.Min;
  sVector31 maxp = parentBB.Max;

  if(plane.x < 0.0f) sSwap(minp.x,maxp.x);
  if(plane.y < 0.0f) sSwap(minp.y,maxp.y);
  if(plane.z < 0.0f) sSwap(minp.z,maxp.z);
  SolveCoordFromPlane(minp,plane,normalAxis);
  SolveCoordFromPlane(maxp,plane,normalAxis);

  if(minp[normalAxis]>maxp[normalAxis])
    sSwap(minp[normalAxis],maxp[normalAxis]);

  bb[sign^0].Max[normalAxis] = sMin(bb[sign^0].Max[normalAxis],maxp[normalAxis]);
  bb[sign^1].Min[normalAxis] = sMax(bb[sign^1].Min[normalAxis],minp[normalAxis]);

  emptyFlag  = (bb[0].Min[normalAxis] >= bb[0].Max[normalAxis]) ? 1 : 0;
  emptyFlag |= (bb[1].Min[normalAxis] >= bb[1].Max[normalAxis]) ? 2 : 0;
}

static sVector4 MakeRandomPlane(const sAABBox &bounds,sRandomMT &rand)
{
  sVector30 dir;
  sVector4 plane;
  do
  {
    dir.InitRandom(rand);
  }
  while(!dir.LengthSq());
  dir.Unit();
  plane = dir;

  sVector30 halfExtent = 0.5f * (bounds.Max - bounds.Min);
  sF32 r = sAbs(halfExtent.x*dir.x) + sAbs(halfExtent.y*dir.y) + sAbs(halfExtent.z*dir.z);
  plane.w = -(bounds.Center() ^ dir) + r * (rand.Float(2.0f) - 1.0f);

  return plane;
}

static sVector4 MakeRandomPlaneThroughPoint(const sVector31 &point,sRandomMT &rand)
{
  sVector30 dir;
  sVector4 plane;
  do
  {
    dir.InitRandom(rand);
  }
  while(!dir.LengthSq());
  dir.Unit();
  plane = dir;
  plane.w = -(point ^ dir);

  return plane;
}

/****************************************************************************/

Wz4BSPFace *Wz4BSPFace::Alloc(sInt count)
{
  Wz4BSPFace *face = (Wz4BSPFace *) sAllocMem(sizeof(Wz4BSPFace)+(count-1)*sizeof(sVector31),4,0);
  face->Count = count;
  face->RefCount = 1;
  return face;
}

void Wz4BSPFace::Free(Wz4BSPFace *face)
{
  if(face) sFreeMem(face);
}

Wz4BSPFace *Wz4BSPFace::MakeTri(const sVector31 &a,const sVector31 &b,const sVector31 &c)
{
  Wz4BSPFace *face = Alloc(3);
  face->Points[0] = a;
  face->Points[1] = b;
  face->Points[2] = c;
  face->CalcPlane();

  return face;
}

Wz4BSPFace *Wz4BSPFace::MakeQuad(const sVector31 &a,const sVector31 &b,const sVector31 &c,const sVector31 &d)
{
  Wz4BSPFace *face = Alloc(4);
  face->Points[0] = a;
  face->Points[1] = b;
  face->Points[2] = c;
  face->Points[3] = d;
  face->CalcPlane();

  return face;
}

Wz4BSPFace *Wz4BSPFace::CleanUp(Wz4BSPFace *in)
{
  sVector31 *cleaned = sALLOCSTACK(sVector31,in->Count);
  sInt cleanedCount = in->Count;

  for(sInt i=0;i<in->Count;i++)
    cleaned[i] = in->Points[i];

  // remove duplicates of first vertex at end
  while(cleanedCount>1 && cleaned[cleanedCount-1] == cleaned[0])
    cleanedCount--;

  // remove duplicate points elsewhere
  sInt outCount = 1;
  for(sInt i=1;i<cleanedCount;i++)
    if(!(cleaned[i] == cleaned[outCount-1]))
      cleaned[outCount++] = cleaned[i];

  if(outCount != in->Count)
  {
    if(outCount >= 3)
    {
      Wz4BSPFace *out = Alloc(outCount);
      out->Plane = in->Plane;
      for(sInt i=0;i<outCount;i++)
        out->Points[i] = cleaned[i];

      in->Release();
      return out;
    }
    else
    {
      in->Release();
      return 0;
    }
  }
  else
    return in;
}

Wz4BSPFace *Wz4BSPFace::MakeCopy() const
{
  Wz4BSPFace *out = Alloc(Count);
  out->Plane = Plane;
  for(sInt i=0;i<Count;i++)
    out->Points[i] = Points[i];

  return out;
}

sF32 Wz4BSPFace::CalcArea() const
{
  // sum over triangles
  sF32 area = 0.0f;
  for(sInt i=2;i<Count;i++)
    area += ((Points[i-1]-Points[0]) % (Points[i]-Points[0])).Length();

  return 0.5f * area;
}

void Wz4BSPFace::CalcPlane()
{
  // compute polygon normal via newell's method
  sVector30 centroid(0.0f), normal(0.0f);
  for(sInt i=Count-1,j=0;j<Count;i=j,j++)
  {
    normal.x += (Points[i].y - Points[j].y) * (Points[i].z + Points[j].z);
    normal.y += (Points[i].z - Points[j].z) * (Points[i].x + Points[j].x);
    normal.z += (Points[i].x - Points[j].x) * (Points[i].y + Points[j].y);
    centroid += sVector30(Points[j]);
  }

  if(Count == 3)
    normal = (Points[1]-Points[0]) % (Points[2]-Points[0]);

  sVector30 origNormal = normal;

  sF32 len = normal.Length();
  normal.x /= len;
  normal.y /= len;
  normal.z /= len;

  Plane = normal;
  Plane.w = -(centroid^normal) / Count;
}

Wz4BSPError Wz4BSPFace::Split(const sVector4 &plane,Wz4BSPFace *&front,Wz4BSPFace *&back,sF32 eps) const
{
  front = 0;
  back = 0;

  if(!IsPlanar(eps))
    return WZ4BSP_ROUNDOFF_PROBLEM;

  sInt *backVert = sALLOCSTACK(sInt,Count+3);
  sInt *frontVert = sALLOCSTACK(sInt,Count+3);
  sVector31 tempVert[4];
  sInt backCount = 0, frontCount = 0, tempCount = 0;
  Wz4BSPError error;

  sInt cls,lastCls;
  lastCls = ClassifyPoint(Points[Count-1],plane,eps);
  for(sInt j=Count-1,k=0;k<Count;j=k,lastCls=cls,k++)
  {
    cls = ClassifyPoint(Points[k],plane,eps);

    switch(lastCls*3 + cls)
    {
      case -1*3 - 1: // back->back
        backVert[backCount++] = k;
        break;

      case -1*3 + 0: // back->on
        backVert[backCount++] = k;
        frontVert[frontCount++] = k;
        break;

      case -1*3 + 1: // back->front
        error = SplitEdgeAlongPlane(Points[k],Points[j],plane,Plane,tempVert[tempCount++],eps);
        if(error != WZ4BSP_OK)
          return error;
        backVert[backCount++] = -tempCount;
        frontVert[frontCount++] = -tempCount;
        frontVert[frontCount++] = k;
        break;

      case 0*3 - 1: // on->back
        backVert[backCount++] = j;
        backVert[backCount++] = k;
        break;

      case 0*3 + 0: // on->on
      case 0*3 + 1: // on->front
        frontVert[frontCount++] = k;
        break;

      case 1*3 - 1: // front->back
        error = SplitEdgeAlongPlane(Points[j],Points[k],plane,Plane,tempVert[tempCount++],eps);
        if(error != WZ4BSP_OK)
          return error;
        frontVert[frontCount++] = -tempCount;
        backVert[backCount++] = -tempCount;
        backVert[backCount++] = k;
        break;

      case 1*3 + 0: // front->on
      case 1*3 + 1: // front->front
        frontVert[frontCount++] = k;
        break;
    }

    if(frontCount > Count+2 || backCount > Count+2)
      return WZ4BSP_ROUNDOFF_PROBLEM;
  }

  if(frontCount < 3 && backCount < 3) // no poly generated?!
  {
    sDPrintF(L"No polygons generated whatsoever in Wz4BSPFace::Split!\n");
    return WZ4BSP_WHAT_THE_FUCK;
  }

  if(frontCount >= 3)
  {
    front = Wz4BSPFace::Alloc(frontCount);
    front->Plane = Plane;
    for(sInt i=0;i<frontCount;i++)
      front->Points[i] = (frontVert[i] >= 0) ? Points[frontVert[i]] : tempVert[-1-frontVert[i]];

    if(!front->IsPlanar(eps))
    {
      sRelease(front);
      return WZ4BSP_NONPLANAR_POLY;
    }

    front = Wz4BSPFace::CleanUp(front);
  }

  if(backCount >= 3)
  {
    back = Wz4BSPFace::Alloc(backCount);
    back->Plane = Plane;
    for(sInt i=0;i<backCount;i++)
      back->Points[i] = (backVert[i] >= 0) ? Points[backVert[i]] : tempVert[-1-backVert[i]];

    if(!back->IsPlanar(eps))
    {
      sRelease(front);
      sRelease(back);
      return WZ4BSP_NONPLANAR_POLY;
    }

    back = Wz4BSPFace::CleanUp(back);
  }

  return WZ4BSP_OK;
}

void Wz4BSPFace::CalcFaceIntegrals(sInt c,sVector30 &f,sVector30 &f2,sVector30 &f3,sVector30 &fmix) const
{
  const sInt a = (c+1)%3;
  const sInt b = (a+1)%3;

  sF32 p1 = 0.0f;
  sF32 pa = 0.0f, pb = 0.0f;
  sF32 pa2 = 0.0f, pab = 0.0f, pb2 = 0.0f;
  sF32 pa3 = 0.0f, pa2b = 0.0f, pab2 = 0.0f, pb3 = 0.0f;

  // calc projection integrals
  for(sInt i=Count-1,j=0;j<Count;i=j,j++)
  {
    sF32 a0 = Points[i][a], b0 = Points[i][b];
    sF32 a1 = Points[j][a], b1 = Points[j][b];

    sF32 da = a1-a0, db = b1-b0;
    sF32 a02 = a0*a0, a03 = a02*a0, a04 = a03*a0;
    sF32 b02 = b0*b0, b03 = b02*b0, b04 = b03*b0;
    sF32 a12 = a1*a1, a13 = a12*a1;
    sF32 b12 = b1*b1, b13 = b12*b1;

    sF32 C1 = a1 + a0;
    sF32 Ca = a1 * C1 + a02, Ca2 = a1 * Ca + a03, Ca3 = a1 * Ca2 + a04;
    sF32 Cb = b12 + b1*b0 + b02, Cb2 = b1 * Cb + b03, Cb3 = b1*Cb2 + b04;

    sF32 Cab = 3*a12 + 2*a1*a0 + a02, Kab = a12 + 2*a1*a0 + 3*a02;
    sF32 Ca2b = a0*Cab + 4*a13, Ka2b = a1*Kab + 4*a03;
    sF32 Cab2 = 4*b13 + 3*b12*b0 + 2*b1*b02 + b03, Kab2 = b13 + 2*b12*b0 + 3*b1*b02 + 4*b03;

    p1 += db * C1;
    pa += db * Ca;
    pa2 += db * Ca2;
    pa3 += db * Ca3;
    pb += da * Cb;
    pb2 += da * Cb2;
    pb3 += da * Cb3;
    pab += db * (b1*Cab + b0*Kab);
    pa2b += db * (b1*Ca2b + b0*Ka2b);
    pab2 += da * (a1*Cab2 + a0*Kab2);
  }

  p1 /= 2.0f;
  pa /= 6.0f;
  pa2 /= 12.0f;
  pa3 /= 20.0f;
  pb /= -6.0f;
  pb2 /= -12.0f;
  pb3 /= -20.0f;
  pab /= 24.0f;
  pa2b /= 60.0f;
  pab2 /= -60.0f;

  // face integrals
  sF32 k1 = 1.0f / Plane[c];
  sF32 k2 = k1*k1, k3=k2*k1, k4=k3*k1;
  sF32 na = Plane[a], nb = Plane[b], w = Plane.w;

  // calc face integrals
  f[a] = k1 * pa;
  f[b] = k1 * pb;
  f[c] = -k2 * (na*pa + nb*pb + w*p1);

  f2[a] = k1 * pa2;
  f2[b] = k1 * pb2;
  f2[c] = k3 * (na*na*pa2 + 2*na*nb*pab + nb*nb*pb2 + 2*na*w*pa + 2*nb*w*pb + w*w*p1);

  f3[a] = k1 * pa3;
  f3[b] = k1 * pb3;
  f3[c] = -k4 * (na*na*na*pa3 + 3*na*na*nb*pa2b + 3*na*nb*nb*pab2 + nb*nb*nb*pb3 + w*w*w*p1
                + 3*na*na*w*pa2 + 6*na*nb*w*pab + 3*nb*nb*w*pb2 + 3*na*w*w*pa + 3*nb*w*w*pb);

  fmix[a] = k1 * pa2b;
  fmix[b] = -k2 * (na*pab2 + nb*pb3 + w*pb2);
  fmix[c] = k3 * (na*na*pa3 + 2*na*nb*pa2b + nb*nb*pab2 + 2*na*w*pa2 + 2*nb*w*pab + w*w*pa);
}

Wz4BSPPolyClass Wz4BSPFace::Classify(const sVector4 &plane,sF32 eps) const
{
  sVERIFY(Count >= 3);
  sInt nBack=0,nFront=0;

  for(sInt i=0;i<Count;i++)
  {
    sInt cl = ClassifyPoint(Points[i],plane,eps);
    nBack += (cl == -1);
    nFront += (cl == 1);
  }

  if(nBack && nFront)
    return WZ4BSP_STRADDLING;

  if(nBack)
    return WZ4BSP_BACK;
  else if(nFront)
    return WZ4BSP_FRONT;
  
  return WZ4BSP_ON;
}

sVector4 Wz4BSPFace::GetPlane() const
{
  return Plane;
}

sBool Wz4BSPFace::IsPlanar(sF32 eps) const
{
  for(sInt i=0;i<Count;i++)
  {
    sInt cls = ClassifyPoint(Points[i],Plane,eps);
    if(cls != 0)
      return sFALSE;
  }

  return sTRUE;
}

void Wz4BSPFace::AddToBBox(sAABBox &box) const
{
  for(sInt i=0;i<Count;i++)
    box.Add(Points[i]);
}

/****************************************************************************/

static const sU8 CubeFaceTable[6][4] =
{
  { 1,3,7,5 }, // +x
  { 4,6,2,0 }, // -x
  { 2,6,7,3 }, // +y
  { 4,0,1,5 }, // -y
  { 5,7,6,4 }, // +z
  { 0,2,3,1 }  // -z
};

Wz4BSPPolyhedron::Wz4BSPPolyhedron()
{
}

Wz4BSPPolyhedron::Wz4BSPPolyhedron(const Wz4BSPPolyhedron& x)
{
  Faces.Resize(x.Faces.GetCount());
  for(sInt i=0;i<Faces.GetCount();i++)
  {
    Faces[i] = x.Faces[i];
    Faces[i]->AddRef();
  }
}

Wz4BSPPolyhedron::~Wz4BSPPolyhedron()
{
  Clear();
}

Wz4BSPPolyhedron& Wz4BSPPolyhedron::operator =(const Wz4BSPPolyhedron &x)
{
  Wz4BSPPolyhedron(x).Swap(*this);
  return *this;
}

void Wz4BSPPolyhedron::Clear()
{
  for(sInt i=0;i<Faces.GetCount();i++)
    Faces[i]->Release();

  Faces.Resize(0);
}

void Wz4BSPPolyhedron::InitBBox(const sAABBox &box)
{
  sVERIFY(IsEmpty());
  Faces.Resize(6);

  sVector31 points[8];
  for(sInt i=0;i<8;i++)
    points[i].Init((i&1)?box.Max.x:box.Min.x,(i&2)?box.Max.y:box.Min.y,(i&4)?box.Max.z:box.Min.z);

  for(sInt i=0;i<6;i++)
  {
    sInt a = CubeFaceTable[i][0];
    sInt b = CubeFaceTable[i][1];
    sInt c = CubeFaceTable[i][2];
    sInt d = CubeFaceTable[i][3];
    Faces[i] = Wz4BSPFace::MakeQuad(points[a],points[b],points[c],points[d]);
  }
}

void Wz4BSPPolyhedron::InitTetrahedron(const sVector31 &o,const sVector31 &x,const sVector31 &y,const sVector31 &z)
{
  sVERIFY(IsEmpty());
  Faces.HintSize(4);

  Faces.AddTail(Wz4BSPFace::MakeTri(o,y,x));
  Faces.AddTail(Wz4BSPFace::MakeTri(x,y,z));
  Faces.AddTail(Wz4BSPFace::MakeTri(o,z,y));
  Faces.AddTail(Wz4BSPFace::MakeTri(x,z,o));
}

void Wz4BSPPolyhedron::Swap(Wz4BSPPolyhedron &x)
{
  Faces.Swap(x.Faces);
}

void Wz4BSPPolyhedron::CalcBBox(sAABBox &box) const
{
  box.Clear();
  for(sInt i=0;i<Faces.GetCount();i++)
    Faces[i]->AddToBBox(box);
}

void Wz4BSPPolyhedron::CalcCentroid(sVector31 &centroid) const
{
  if(IsEmpty())
  {
    centroid = sVector31(0);
    return;
  }

  sVector30 ctemp(0);
  sInt count = 0;

  for(sInt i=0;i<Faces.GetCount();i++)
  {
    for(sInt j=0;j<Faces[i]->Count;j++)
      ctemp += sVector30(Faces[i]->Points[j]);

    count += Faces[i]->Count;
  }

  centroid = sVector31((1.0f/count) * ctemp);
}

sF32 Wz4BSPPolyhedron::CalcArea() const
{
  sF32 area = 0.0f;
  for(sInt i=0;i<Faces.GetCount();i++)
    area += Faces[i]->CalcArea();

  return area;
}

sF32 Wz4BSPPolyhedron::CalcVolume() const
{
  if(IsEmpty())
    return 0.0f;

  sVector31 corner = Faces[0]->Points[0];
  sF32 volume = 0.0f;

  for(sInt i=1;i<Faces.GetCount();i++)
  {
    sF32 height = -(corner ^ Faces[i]->Plane);
    volume += height * Faces[i]->CalcArea();
  }

  return volume / 3.0f;
}

void Wz4BSPPolyhedron::FindCloserVertex(const sVector31 &target,sVector31 &closest,sF32 closestDistSq) const
{
  const Wz4BSPFace *face;
  sFORALL(Faces,face)
  {
    for(sInt k=0;k<face->Count;k++)
    {
      sF32 distSq = (face->Points[k] - target).LengthSq();
      if(distSq < closestDistSq)
      {
        closest = face->Points[k];
        closestDistSq = distSq;
      }
    }
  }
}

Wz4BSPError Wz4BSPPolyhedron::Split(const sVector4 &plane,Wz4BSPPolyhedron &front,Wz4BSPPolyhedron &back,sF32 eps) const
{
  Wz4BSPError error = WZ4BSP_OK;
  front.Clear();
  back.Clear();

  // trivial cases
  switch(Classify(plane,eps))
  {
  case WZ4BSP_BACK:   back = *this; return WZ4BSP_OK;
  case WZ4BSP_ON: // fall-through
  case WZ4BSP_FRONT:  front = *this; return WZ4BSP_OK;
  default:            break;
  }

  // calculate bounding box
  sAABBox box;
  CalcBBox(box);

  sVector31 bbPoints[8];
  for(sInt i=0;i<8;i++)
    bbPoints[i].Init((i&1)?box.Max.x:box.Min.x,(i&2)?box.Max.y:box.Min.y,(i&4)?box.Max.z:box.Min.z);

  // split all the faces
  for(sInt i=0;i<Faces.GetCount();i++)
  {
    Wz4BSPFace *frontFace,*backFace;
    if(error == WZ4BSP_OK)
    {
      error = Faces[i]->Split(plane,frontFace,backFace,eps);
      if(frontFace) front.Faces.AddTail(frontFace);
      if(backFace) back.Faces.AddTail(backFace);
    }
  }

  if(error == WZ4BSP_OK && back.IsEmpty() && front.IsEmpty()) // polygons clipped out of existence
  {
    sDPrintF(L"Polygons clipped out of existence - this shouldn't happen.\n");
    error = WZ4BSP_WHAT_THE_FUCK;
  }

  if(error != WZ4BSP_OK)
  {
    back.Clear();
    front.Clear();
    return error;
  }

  // generate faces on splitting plane to close the clipped polyhedrons
  sInt normalAxis = sVector30(plane).MaxAxisAbs();
  sInt flip = plane[normalAxis] < 0.0f;
  sInt configuration = normalAxis*2 + flip;
  sVector31 v[4];

  for(sInt i=0;i<4;i++)
  {
    v[i] = bbPoints[CubeFaceTable[configuration][i]];
    SolveCoordFromPlane(v[i],plane,normalAxis);
  }

  Wz4BSPFace *frontFace;
  frontFace = Wz4BSPFace::MakeQuad(v[0],v[1],v[2],v[3]);
  frontFace->Plane = plane;
  if(!frontFace->IsPlanar(eps))
    error = WZ4BSP_NONPLANAR_POLY;

  // clip against all input planes
  for(sInt i=0;frontFace && i<Faces.GetCount();i++)
  {
    Wz4BSPFace *newFront,*temp;
    if(error == WZ4BSP_OK)
    {
      error = frontFace->Split(Faces[i]->Plane,temp,newFront,eps);
      sRelease(temp);
      frontFace->Release();
      frontFace = newFront;
    }
  }

  if(error != WZ4BSP_OK)
  {
    back.Clear();
    front.Clear();
    return error;
  }

  if(frontFace)
    frontFace = Wz4BSPFace::CleanUp(frontFace);

  if(frontFace)
  {
    // snap frontFace vertices to closest points on existing polygons (to close the mesh)
    for(sInt i=0;i<frontFace->Count;i++)
    {
      sVector31 target = frontFace->Points[i];
      sF32 closestDistSq = 1e+30f;
      sVector31 closest;

      back.FindCloserVertex(frontFace->Points[i],closest,closestDistSq);
      front.FindCloserVertex(frontFace->Points[i],closest,closestDistSq);
      
      frontFace->Points[i] = closest;
    }

    if(frontFace->IsPlanar(eps))
    {
      // generate corresponding polygon for back face and insert into destination polyhedrons
      Wz4BSPFace *backFace = Wz4BSPFace::Alloc(frontFace->Count);
      for(sInt j=0;j<frontFace->Count;j++)
        backFace->Points[j] = frontFace->Points[frontFace->Count-1-j];

      backFace->Plane = plane;
      backFace->Plane.Neg1();

      back.Faces.AddTail(frontFace);
      front.Faces.AddTail(backFace);

      if(!backFace->IsPlanar(eps))
      {
        sDPrintF(L"frontface planar but backface isn't - this shouldn't happen.\n");
        back.Clear();
        front.Clear();
        return WZ4BSP_WHAT_THE_FUCK;
      }
    }
    else // need to triangulate
    {
      for(sInt j=2;j<frontFace->Count;j++)
      {
        Wz4BSPFace *frontf = Wz4BSPFace::Alloc(3);
        frontf->Points[0] = frontFace->Points[0];
        frontf->Points[1] = frontFace->Points[j-1];
        frontf->Points[2] = frontFace->Points[j];
        frontf = Wz4BSPFace::CleanUp(frontf);
        if(frontf)
        {
          frontf->CalcPlane();
          back.Faces.AddTail(frontf);
        }

        Wz4BSPFace *backf = Wz4BSPFace::Alloc(3);
        backf->Points[0] = frontFace->Points[0];
        backf->Points[1] = frontFace->Points[j];
        backf->Points[2] = frontFace->Points[j-1];
        backf = Wz4BSPFace::CleanUp(backf);
        if(backf)
        {
          backf->CalcPlane();
          front.Faces.AddTail(backf);
        }

        if((frontf && !frontf->IsPlanar(eps)) || (backf && !backf->IsPlanar(eps)))
        {
          sRelease(frontFace);
          back.Clear();
          front.Clear();
          return WZ4BSP_NONPLANAR_POLY;
        }
      }

      sRelease(frontFace);
    }
  }
  else
  {
    // frontFace was clipped out of existence; the split is apparently degenerate/tiny.
    sInt nBackOn=0,nFront=0;

    for(sInt i=0;i<Faces.GetCount();i++)
    {
      for(sInt j=0;j<Faces[i]->Count;j++)
      {
        if(ClassifyPoint(Faces[i]->Points[j],plane,eps) > 0)
          nFront++;
        else
          nBackOn++;
      }
    }

    back.Clear();
    front.Clear();

    if(nBackOn >= nFront)
      back = *this;
    else
      front = *this;
  }

  return WZ4BSP_OK;
}

void Wz4BSPPolyhedron::GenMesh(Wz4Mesh *mesh,sF32 explode,const sVector30 &shift) const
{
  sAABBox box;
  CalcBBox(box);

  sInt boneIdx = mesh->Chunks.GetCount();
  sInt firstVert = mesh->Vertices.GetCount();
  sInt firstFace = mesh->Faces.GetCount();
  sVector30 offset = shift + explode * sVector30(box.Center());

  const Wz4BSPFace *face;
  sFORALL(Faces,face)
  {
    // vertices
    sInt vb = mesh->Vertices.GetCount();
    Wz4MeshVertex *mv = mesh->Vertices.AddMany(face->Count);
    sInt projAxis = sVector30(face->Plane).MaxAxisAbs();

    for(sInt i=0;i<face->Count;i++)
    {
      mv->Init();
      mv->Pos = face->Points[i] + offset;
      mv->Normal = sVector30(face->Plane);

      switch(projAxis)
      {
      case 0: mv->U0 = mv->Pos.y; mv->V0 = mv->Pos.z; break;
      case 1: mv->U0 = mv->Pos.z; mv->V0 = mv->Pos.x; break;
      case 2: mv->U0 = mv->Pos.x; mv->V0 = mv->Pos.y; break;
      default: sVERIFYFALSE;
      }
      
      mv->Index[0] = boneIdx;
      mv->Weight[0] = 1.0f;
      mv++;
    }

    // faces
    if(face->Count <= 4)
    {
      // one single face
      Wz4MeshFace *mf = mesh->Faces.AddMany(1);
      mf->Init(face->Count);

      for(sInt i=0;i<face->Count;i++)
        mf->Vertex[i] = vb + i;
    }
    else
    {
      // triangulate
      Wz4MeshFace *mf = mesh->Faces.AddMany(face->Count-2);
      for(sInt i=2;i<face->Count;i++)
      {
        mf->Init(3);
        mf->Vertex[0] = vb;
        mf->Vertex[1] = vb + i-1;
        mf->Vertex[2] = vb + i;
        mf++;
      }
    }
  }

  Wz4ChunkPhysics *phys = mesh->Chunks.AddMany(1);
  CalcMassProperties(phys->Volume,phys->COM,phys->InertD,phys->InertOD);

  phys->COM += offset;
  phys->FirstVert = (mesh->Vertices.GetCount() == firstVert) ? -1 : firstVert;
  phys->FirstFace = firstFace;
  phys->FirstIndex = 0;
  phys->Random.Init(1,0,0);
  phys->Normal = phys->InertD;
  phys->Normal.Unit();
}

void Wz4BSPPolyhedron::Dump() const
{
  if(Faces.GetCount())
  {
    sDPrintF(L"--- polyhedron with %d faces:\n",Faces.GetCount());
    for(sInt i=0;i<Faces.GetCount();i++)
    {
      sDPrintF(L"  [%2d]",i);
      for(sInt j=0;j<Faces[i]->Count;j++)
        sDPrintF(L" %f",Faces[i]->Points[j]);

      sDPrint(L"\n");
    }
  }
  else
    sDPrintF(L"--- empty polyhedron\n");
}

void Wz4BSPPolyhedron::CalcMassProperties(sF32 &vol,sVector31 &cog,sVector30 &inertD,sVector30 &inertOD) const
{
  // this algorithm works for all solid objects, not just convex ones...
  // (Mirtich, "Fast and accurate computation of polyhedral mass properties", JGT, 1996)
  sInt i;
  vol = 0.0f;
  cog.Init(0.0f,0.0f,0.0f);
  inertD.Init(0.0f,0.0f,0.0f);
  inertOD.Init(0.0f,0.0f,0.0f);

  for(i=0;i<Faces.GetCount();i++)
  {
    sVector30 n = sVector30(Faces[i]->Plane);
    sInt gamma = n.MaxAxisAbs();

    sVector30 f,f2,f3,fmix;
    Faces[i]->CalcFaceIntegrals(gamma,f,f2,f3,fmix);

    vol += n[0] * f[0];
    cog += n * f2;
    inertD += n * f3;
    inertOD += n * fmix;
  }

  // finish up calculations
  cog = sVector31((0.5f / vol) * cog);
  inertD *= (1.0f/3.0f);
  inertOD *= 0.5f;

  // transform to body-centered coordinate system
  inertD -= vol*(sVector30(cog)*sVector30(cog));
  inertOD.x -= vol*cog.x*cog.y;
  inertOD.y -= vol*cog.y*cog.z;
  inertOD.z -= vol*cog.z*cog.x;

  for(i=0;i<Faces.GetCount();i++)
  {
    if(ClassifyPoint(cog,Faces[i]->Plane,1e-5f) > 0)
      break;
  }

  if(vol <= 0.0f || i != Faces.GetCount()) // degenerate body; use fallback
  {
    static const sF32 tinyEdge = 1e-5f;

    sVector30 com(0.0f);
    sInt count=0;

    for(sInt i=0;i<Faces.GetCount();i++)
    {
      for(sInt j=0;j<Faces[i]->Count;j++)
        com += sVector30(Faces[i]->Points[j]);

      count += Faces[i]->Count;
    }

    com *= (1.0f / count);
    cog = sVector31(com);
    vol = tinyEdge * tinyEdge * tinyEdge;

    inertD.Init(1.0f/12.0f,1.0f/12.0f,1.0f/12.0f); // unit box
    inertOD.Init(0,0,0);

    inertD *= vol * tinyEdge * tinyEdge; // assume it's a tiny cube with edge length tinyEdge
  }
}


Wz4BSPPolyClass Wz4BSPPolyhedron::Classify(const sVector4 &plane,sF32 eps) const
{
  sInt nBack=0,nFront=0,nStraddling=0;

  for(sInt i=0;i<Faces.GetCount();i++)
  {
    switch(Faces[i]->Classify(plane,eps))
    {
    case WZ4BSP_STRADDLING: nStraddling++; break;
    case WZ4BSP_BACK:       nBack++; break;
    case WZ4BSP_FRONT:      nFront++; break;
    case WZ4BSP_ON:  break;
    }
  }

  if(nStraddling || (nBack && nFront))
    return WZ4BSP_STRADDLING;

  if(nBack)
    return WZ4BSP_BACK;
  else if(nFront)
    return WZ4BSP_FRONT;

  return WZ4BSP_ON;
}

/****************************************************************************/

Wz4BSPNode::Wz4BSPNode(sInt type)
{
  Plane.Init(0,0,0,0);
  Type = type;
  Child[0] = Child[1] = 0;
}

Wz4BSPNode::~Wz4BSPNode()
{
  delete Child[0];
  delete Child[1];
}

/****************************************************************************/

sVector4 Wz4BSP::PickSplittingPlane(const sArray<Wz4BSPFace*> &faces,sRandomMT &rand)
{
  static const sF32 K = 0.9f;
  static const sInt MaxTests = 8;

  sF32 bestCost;
  sVector4 bestPlane;
  sInt candidates[MaxTests];
  sInt maxTries = 0, bestCandidate;

  if(faces.GetCount() <= MaxTests)
  {
    maxTries = faces.GetCount();
    for(sInt i=0;i<maxTries;i++)
      candidates[i] = i;
  }
  else
  {
    maxTries = MaxTests;
    
#if RANDOMPLANES
    // select a bunch of random candidate faces
    for(sInt i=0;i<MaxTests;i++)
    {
      sInt j;
      do
      {
        candidates[i] = rand.Int(faces.GetCount());
        for(j=0;j<i;j++)
          if(candidates[j]==candidates[i])
            break;
      }
      while(j != i);
    }
#else
    for(sInt i=0;i<maxTries;i++)
      candidates[i] = i;
#endif
  }

  for(sInt i=0;i<maxTries;i++)
  {
    sInt inFront = 0, behind = 0, straddling = 0;
    sVector4 plane = faces[candidates[i]]->Plane;

    for(sInt j=0;j<faces.GetCount();j++)
    {
      sVERIFY(faces[j]->IsPlanar(PlaneThickness));

      switch(faces[j]->Classify(plane,PlaneThickness))
      {
      case WZ4BSP_ON: // on count for back
      case WZ4BSP_BACK:
        behind++;
        break;

      case WZ4BSP_FRONT:
        inFront++;
        break;

      case WZ4BSP_STRADDLING:
        straddling++;
        break;
      }
    }

    sF32 cost = K * straddling + (1.0f - K) * sAbs(inFront - behind);
    if(i==0 || cost < bestCost)
    {
      bestCost = cost;
      bestPlane = plane;
      bestCandidate = candidates[i];
    }
  }

  return bestPlane;
}

Wz4BSPNode *Wz4BSP::BuildTreeR(sArray<Wz4BSPFace*> &splitFaces,sRandomMT &rand)
{
  if(ErrorCode != WZ4BSP_OK)
    return 0;

  if(!splitFaces.GetCount())
    return new Wz4BSPNode(Wz4BSPNode::Empty);

  sVector4 splitPlane = PickSplittingPlane(splitFaces,rand);
  sArray<Wz4BSPFace*> frontSplit,backSplit;
  Wz4BSPFace *frontFace,*backFace;

  for(sInt i=0;i<splitFaces.GetCount();i++)
  {
    switch(splitFaces[i]->Classify(splitPlane,PlaneThickness))
    {
    case WZ4BSP_BACK:
      splitFaces[i]->AddRef();
      backSplit.AddTail(splitFaces[i]);
      break;

    case WZ4BSP_FRONT:
      splitFaces[i]->AddRef();
      frontSplit.AddTail(splitFaces[i]);
      break;

    case WZ4BSP_STRADDLING:
      if(ErrorCode == WZ4BSP_OK)
      {
        ErrorCode = splitFaces[i]->Split(splitPlane,frontFace,backFace,PlaneThickness);
        if(frontFace) frontSplit.AddTail(frontFace);
        if(backFace) backSplit.AddTail(backFace);
      }
      break;
    case WZ4BSP_ON:
      break;
    }
  }

  if(ErrorCode != WZ4BSP_OK)
  {
    for(sInt i=0;i<backSplit.GetCount();i++)
      sRelease(backSplit[i]);

    for(sInt i=0;i<frontSplit.GetCount();i++)
      sRelease(frontSplit[i]);

    return 0;
  }

  Wz4BSPNode *node = new Wz4BSPNode;
  node->Plane = splitPlane;
  node->Child[0] = BuildTreeR(backSplit,rand);
  for(sInt i=0;i<backSplit.GetCount();i++)
    sRelease(backSplit[i]);

  node->Child[1] = BuildTreeR(frontSplit,rand);
  for(sInt i=0;i<frontSplit.GetCount();i++)
    sRelease(frontSplit[i]);

  if(ErrorCode == WZ4BSP_OK)
  {
    if(node->Child[0]->Type == Wz4BSPNode::Empty)
      node->Child[0]->Type = Wz4BSPNode::Solid;

    return node;
  }
  else
  {
    delete node;
    return 0;
  }
}

Wz4BSPNode *Wz4BSP::CopyTreeR(Wz4BSPNode *node)
{
  Wz4BSPNode *out = new Wz4BSPNode;
  *out = *node;
  if(out->Type == Wz4BSPNode::Inner)
  {
    out->Child[0] = CopyTreeR(node->Child[0]);
    out->Child[1] = CopyTreeR(node->Child[1]);
  }

  return out;
}

Wz4BSPNode *Wz4BSP::SplitTreeR(Wz4BSPNode *node,const sAABBox &box,const sVector4 &plane,sInt side,sInt &splitSolids)
{
  // check bounding box
  sAABBox bb[2];
  sInt empty;
  CalcChildBB(box,plane,bb,empty);

  if(empty & (1<<side))
    return new Wz4BSPNode(Wz4BSPNode::Empty);
  else
  {
    Wz4BSPNode *outNode = new Wz4BSPNode;
    *outNode = *node;
    outNode->Bounds = bb[side];

    if(outNode->Type == Wz4BSPNode::Inner)
    {
      sAABBox refBB = bb[side];
      CalcChildBB(refBB,node->Plane,bb,empty);

      for(sInt i=0;i<2;i++)
      {
        if(empty & (1<<i))
          outNode->Child[i] = new Wz4BSPNode(Wz4BSPNode::Empty);
        else
          outNode->Child[i] = SplitTreeR(node->Child[i],bb[i],plane,side,splitSolids);
      }
    }
    else if(outNode->Type == Wz4BSPNode::Solid)
      splitSolids++;

    return outNode;
  }
}

Wz4BSPNode *Wz4BSP::SplitTree(Wz4BSPNode *root,const sVector4 &plane,sInt &splitSolids)
{
  sAABBox bb[2];
  sInt empty;
  CalcChildBB(root->Bounds,plane,bb,empty);

  Wz4BSPNode *node = new Wz4BSPNode;
  node->Plane = plane;
  node->Bounds = root->Bounds;

  for(sInt i=0;i<2;i++)
  {
    if(empty&(1<<i))
      node->Child[i] = new Wz4BSPNode(Wz4BSPNode::Empty);
    else
      node->Child[i] = SplitTreeR(root,bb[i],plane,i,splitSolids);
  }

  delete root;
  return node;
}

void Wz4BSP::GeneratePolyhedronsR(Wz4BSPNode *node,const Wz4BSPPolyhedron &base,Wz4Mesh *out,sF32 explode)
{
  sVERIFY(node->Type == Wz4BSPNode::Inner);
  sVERIFY(ErrorCode == WZ4BSP_OK);

  Wz4BSPPolyhedron front,back;
  ErrorCode = base.Split(node->Plane,front,back,PlaneThickness);

  if(ErrorCode == WZ4BSP_OK && !back.IsEmpty())
  {
    if(node->Child[0]->Type == Wz4BSPNode::Inner)
      GeneratePolyhedronsR(node->Child[0],back,out,explode);
    else if(node->Child[0]->Type == Wz4BSPNode::Solid)
      back.GenMesh(out,explode,CenterPos);
  }

  if(ErrorCode == WZ4BSP_OK && !front.IsEmpty())
  {
    if(node->Child[1]->Type == Wz4BSPNode::Inner)
      GeneratePolyhedronsR(node->Child[1],front,out,explode);
    else if(node->Child[1]->Type == Wz4BSPNode::Solid)
      front.GenMesh(out,explode,CenterPos);
  }
}

void Wz4BSP::CalcBoundsR(Wz4BSPNode *node,const sAABBox &box)
{
  sVERIFY(node != 0);
  node->Bounds = box;

  if(node->Type == Wz4BSPNode::Inner)
  {
    // calc back/front bounding boxes
    sAABBox bb[2];
    sInt empty;
    CalcChildBB(box,node->Plane,bb,empty);

    // recurse
    if(!(empty&1))
      CalcBoundsR(node->Child[0],bb[0]);

    if(!(empty&2))
      CalcBoundsR(node->Child[1],bb[1]);
  }
}

Wz4BSPNode *Wz4BSP::MakeRandomSplitsR(Wz4BSPNode *node,sRandomMT &rand,const Wz4BSPPolyhedron &poly,sInt &maxSplits)
{
  if(node->Type == Wz4BSPNode::Solid)
  {
    sF32 volume = poly.CalcVolume();
    sBool cut = sFALSE;

    if(volume >= MaxVolume)
      cut = sTRUE;
    else
    {
      sF32 t = sClamp<sF32>((volume - MinVolume) / (MaxVolume - MinVolume),0.0f,1.0f);
      if(t > 0.0f)
        t = sFPow(t,1.0f/3.0f);

      sF32 prob = sFade(t,RandomPlaneProb,1.0f);
      if(rand.Float(1.0f) < prob)
        cut = sTRUE;
    }

    if(cut && maxSplits>0)
    {
      sAABBox bounds;
      poly.CalcBBox(bounds);
      sInt nSplits = 0;
      node = SplitTree(node,MakeRandomPlane(bounds,rand),nSplits);
      maxSplits -= nSplits;
    }
  }

  /*if(node->Type == Wz4BSPNode::Solid && (rand.Int16() < RandomPlaneThresh || poly.CalcVolume() >= 0.3f*0.3f*0.3f))
    node = SplitTree(node,MakeRandomPlane(bounds,rand));*/

  if(node->Type == Wz4BSPNode::Inner)
  {
    Wz4BSPPolyhedron front,back;
    poly.Split(node->Plane,front,back,PlaneThickness);

    if(!back.IsEmpty())   node->Child[0] = MakeRandomSplitsR(node->Child[0],rand,back,maxSplits);
    if(!front.IsEmpty())  node->Child[1] = MakeRandomSplitsR(node->Child[1],rand,front,maxSplits);
  }

  return node;
}

Wz4BSP::Wz4BSP()
{
  Type = Wz4BSPType;
  Root = 0;
  PlaneThickness = 1e-5f;
}

Wz4BSP::~Wz4BSP()
{
  delete Root;
}

void Wz4BSP::CopyFrom(Wz4BSP *bsp)
{
  sDelete(Root);

  Root = CopyTreeR(bsp->Root);
  Bounds = bsp->Bounds;
  CenterPos = bsp->CenterPos;
  PlaneThickness = bsp->PlaneThickness;
}

void Wz4BSP::MakePolyhedron(sInt nFaces,sInt nIter,sF32 power,sBool dualize,sInt seed)
{
  sVERIFY(nFaces >= 4);
  sVector30 *facePoints = new sVector30[nFaces];
  sRandomMT rand;

  // bounds (intentionally too coarse)
  sAABBox box; 
  box.Min.Init(-2,-2,-2);
  box.Max.Init( 2, 2, 2);

  rand.Seed(seed);
  facePoints[0].Init(0,0,-1);
  for(sInt i=1;i<nFaces;i++)
  {
    facePoints[i].InitRandom(rand);
    facePoints[i].Unit();
  }

  for(sInt iter=0;iter<nIter;iter++)
  {
    for(sInt i=0;i<nFaces;i++)
    {
      sVector30 forceV(0.0f);

      for(sInt j=0;j<nFaces;j++)
      {
        sVector30 d = facePoints[i]-facePoints[j];
        sF32 d2 = d.LengthSq();
        if(!d2)
          continue;

        forceV += (power / d2) * d;
      }

      facePoints[i] += forceV;
      facePoints[i].Unit();
    }
  }

  // dualize if requested
  if(dualize)
  {
    Wz4BSPPolyhedron poly,temp1,temp2;
    poly.InitBBox(box);

    // cut off all face planes
    for(sInt i=0;i<nFaces;i++)
    {
      sVector4 plane;
      plane = facePoints[i];
      plane.w = -0.5f;
      poly.Split(plane,temp1,temp2,PlaneThickness);
      poly.Swap(temp2);
    }

    // find vertices
    sArray<sVector30> verts;
    Wz4BSPFace *face;
    sFORALL(poly.Faces,face)
    {
      for(sInt i=0;i<face->Count;i++)
      {
        sVector30 pt = sVector30(face->Points[i]);
        if(!sFind(verts,pt))
          verts.AddTail(pt);
      }
    }

    // turn into new face points
    delete[] facePoints;
    facePoints = new sVector30[verts.GetCount()];
    for(sInt i=0;i<verts.GetCount();i++)
    {
      facePoints[i] = verts[i];
      facePoints[i].Unit();
    }

    nFaces = verts.GetCount();
  }

  // build polyhedron
  sDelete(Root);
  Wz4BSPNode **Last = &Root;
  for(sInt i=0;i<nFaces;i++)
  {
    Wz4BSPNode *node = new Wz4BSPNode;
    node->Plane = facePoints[i];
    node->Plane.w = -0.5f;
    node->Child[0] = 0;
    node->Child[1] = new Wz4BSPNode(Wz4BSPNode::Empty);

    *Last = node;
    Last = &node->Child[0];
  }

  *Last = new Wz4BSPNode(Wz4BSPNode::Solid);
  Bounds = box;
  CalcBoundsR(Root,Bounds);

  delete[] facePoints;
}

Wz4BSPError Wz4BSP::FromMesh(const Wz4Mesh *mesh,sF32 planeThickness)
{
  sArray<Wz4BSPFace*> faces;
  sRandomMT rand;

  sDelete(Root);

  PlaneThickness = planeThickness;
  rand.Init();
  Bounds.Clear();

  sAABBox origBounds;
  mesh->CalcBBox(origBounds);
  CenterPos = (sVector30) sAverage(origBounds.Min,origBounds.Max);

  faces.HintSize(mesh->Faces.GetCount());

  for(sInt i=0;i<mesh->Faces.GetCount();i++)
  {
    Wz4BSPFace *face = Wz4BSPFace::Alloc(mesh->Faces[i].Count);
    for(sInt j=0;j<mesh->Faces[i].Count;j++)
      face->Points[j] = mesh->Vertices[mesh->Faces[i].Vertex[j]].Pos - CenterPos;

    face->CalcPlane();
    face->AddToBBox(Bounds);
    if(!face->IsPlanar(PlaneThickness))
    {
      face->Release();

      // triangulate on the fly
      for(sInt j=2;j<mesh->Faces[i].Count;j++)
      {
        face = Wz4BSPFace::Alloc(3);
        face->Points[0] = mesh->Vertices[mesh->Faces[i].Vertex[0]].Pos - CenterPos;
        face->Points[1] = mesh->Vertices[mesh->Faces[i].Vertex[j-1]].Pos - CenterPos;
        face->Points[2] = mesh->Vertices[mesh->Faces[i].Vertex[j]].Pos - CenterPos;
        face->CalcPlane();
        if(!face->IsPlanar(PlaneThickness))
        {
          // cleanup
          for(sInt i=0;i<faces.GetCount();i++)
            faces[i]->Release();

          return WZ4BSP_NONPLANAR_INPUT;
        }

        faces.AddTail(face);
      }
    }
    else
      faces.AddTail(face);
  }

  ErrorCode = WZ4BSP_OK;
  Root = BuildTreeR(faces,rand);
  if(Root)
    CalcBoundsR(Root,Bounds);

  for(sInt i=0;i<faces.GetCount();i++)
    faces[i]->Release();

  return ErrorCode;
}

void Wz4BSP::MakeRandomSplits(sF32 randomPlaneProb,sF32 minEdge,sF32 maxEdge,sInt seed,sInt maxSplits)
{
  sRandomMT rand;
  Wz4BSPPolyhedron poly;

  RandomPlaneProb = sClamp(randomPlaneProb*0.5f,0.0f,0.45f);
  MinVolume = minEdge*minEdge*minEdge;
  MaxVolume = maxEdge*maxEdge*maxEdge;

  rand.Seed(seed);
  poly.InitBBox(Bounds);
  Root = MakeRandomSplitsR(Root,rand,poly,maxSplits);
}

Wz4BSPError Wz4BSP::GeneratePolyhedrons(Wz4Mesh *out,sF32 explode)
{
  Wz4BSPPolyhedron poly;

  sVERIFY(out->IsEmpty());
  out->AddDefaultCluster(); 

  ErrorCode = WZ4BSP_OK;
  poly.InitBBox(Bounds);
  GeneratePolyhedronsR(Root,poly,out,explode);
  out->SplitClustersChunked(74);

  return ErrorCode;

  /*out->CalcNormals();
  out->CalcTangents();*/
}

sBool Wz4BSP::TraceRay(const sRay &inRay,sF32 tMin,sF32 tMax,sF32 &tHit,sVector30 &hitNormal)
{
  static const sInt StackSize=256;
  struct StackEntry
  {
    Wz4BSPNode *node,*parent;
    sF32 time;
  };
  StackEntry stack[StackSize];
  sVector30 normal;
  sInt stackp = 0;
  sF32 IntersectEpsilon = 2.0f * PlaneThickness;

  sRay ray = inRay;
  ray.Start -= CenterPos;

  sVERIFY(Root != 0);
  Wz4BSPNode *node = Root, *parent = 0;
  while(1)
  {
    if(node->Type == Wz4BSPNode::Inner)
    {
      sF32 denom = ray.Dir ^ node->Plane;
      sF32 dist = -(ray.Start ^ node->Plane);
      sInt nearIndex = dist < 0.0f;

      if(denom != 0.0f)
      {
        sF32 t = dist / denom;
        if(0.0f <= t && t <= tMax+IntersectEpsilon)
        {
          if(t >= tMin-IntersectEpsilon)
          {
            stack[stackp].node = node->Child[1-nearIndex];
            stack[stackp].time = tMax;
            stack[stackp].parent = node;
            stackp++;
            tMax = t;
          }
          else
            nearIndex = 1 - nearIndex;
        }
      }
      else if(sFAbs(dist) < IntersectEpsilon)
      {
        stack[stackp].node = node->Child[1-nearIndex];
        stack[stackp].time = tMax;
        stackp++;
      }

      parent = node;
      node = node->Child[nearIndex];
    }
    else
    {
      if(node->Type == Wz4BSPNode::Solid)
      {
        //sInt col = sClamp<sInt>(-normal.z*255,0,255);
        //colHit = col*0x010101;
        //colHit = Tree[parent].Color;
        hitNormal = normal;
        tHit = tMin;
        return sTRUE;
      }

      if(!stackp)
        break;

      normal = sVector30(parent->Plane);
      tMin = tMax;
      stackp--;
      node = stack[stackp].node;
      tMax = stack[stackp].time;
      parent = stack[stackp].parent;
    }
  }

  return sFALSE;
}


sBool Wz4BSP::IsInside(const sVector31 &pos_)
{
  const sVector31 pos(pos_-CenterPos);

  Wz4BSPNode *n = Root;

  while(n->Type==Wz4BSPNode::Inner)
  {
    if((n->Plane ^ pos)<0)
      n = n->Child[0];
    else 
      n = n->Child[1];
  }
  return (n->Type==Wz4BSPNode::Solid);
}

/****************************************************************************/


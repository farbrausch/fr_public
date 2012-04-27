/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2008 Bastian Zuehlke, all rights reserved                      ***/
/***                                                                      ***/
/****************************************************************************/

#include "tADF.hpp"
#include "base/System.hpp"

/****************************************************************************/

#define MULTICORE 1


/****************************************************************************/

inline static bool IsIn(sF32 _min1, sF32 _max1, sF32 _min2, sF32 _max2)
{
  return !(_min2>_max1  || _max2<_min1);
}

inline static bool IsIn(sAABBox &_a, sAABBox &_b)
{
  return IsIn(_a.Min.x, _a.Max.x, _b.Min.x, _b.Max.x) &&
         IsIn(_a.Min.y, _a.Max.y, _b.Min.y, _b.Max.y) &&
         IsIn(_a.Min.z, _a.Max.z, _b.Min.z, _b.Max.z);
}


unsigned int tAABBoxOctree::FindAddEdge(unsigned int _i1, unsigned int _i2)
{ 

  tAABBoxOctreeEdge e;
  
  if (_i1<_i2)
  {
    e.key.i1 = _i1;
    e.key.i2 = _i2;
  }
  else
  {
    e.key.i1 = _i2;
    e.key.i2 = _i1;
  }
  
  tAABBoxOctreeEdge *ptr = edgehash.Find(&e.key);

  if (ptr==0)
  {    
    e.n  = sVector30(0,0,0);
    e.ep = (unsigned int)edges.GetCount();    
    edges.AddTail(e);
    ptr = &edges[e.ep];
    edgehash.Add(&ptr->key,ptr);
  }

  return ptr->ep; 
}

Wz4BSPError tAABBoxOctree::FromMesh(Wz4Mesh *in, sF32 planeThickness, sBool ForceCubeSampling, sBool UserBox, const sVector31 &BoxPos, const sVector30 &BoxDimH, sBool BruteForce, sF32 GuardBand)
{
  sInt i=0;
  Wz4MeshVertex *vp;
  Wz4MeshFace   *fp;

  if (in->IsEmpty())
    return WZ4BSP_WHAT_THE_FUCK;

  if (in->GetTriCount()<3)
    return WZ4BSP_WHAT_THE_FUCK;

  sFORALL(in->Vertices,vp)
  {    
    AddVertex(vp->Pos,vp->Normal);  
  }

  in->CalcNormals();

  FinishVertices(GuardBand,ForceCubeSampling,UserBox,BoxPos,BoxDimH);

  i=0;
  sFORALL(in->Faces,fp)
  {
    if (in->IsDegenerateFace(i))
    {
      sDPrintF(L"SDF degnerated face fount\n");      
    }
    else if (fp->Count==3)
    {
      
        AddTriangle(fp->Vertex[0], fp->Vertex[1], fp->Vertex[2], in->GetFaceNormal(i));
    }
    else if (fp->Count==4)
    {
      AddTriangle(fp->Vertex[0], fp->Vertex[1], fp->Vertex[2], in->GetFaceNormal(i));
      AddTriangle(fp->Vertex[0], fp->Vertex[2], fp->Vertex[3], in->GetFaceNormal(i));    
    }
    else
    {
      sDPrintF(L"Error SDF only supports triangles\n");      
    }
    i++;
  }

  Finalize();
  return bsp.FromMesh(in,planeThickness);
}


void tAABBoxOctree::AddVertex(const sVector31 &_v, const sVector30 &_n)
{
  tAABBoxOctreeVertex v;  
  v.v = _v;
  v.n = _n;
  vertices.AddTail(v);
}

void tAABBoxOctree::AddTriangle(unsigned int _i1, unsigned int _i2, unsigned int _i3, const sVector30 &_n)
{
  tAABBoxOctreeTri tri;

  if (_i2<_i1)
  {
    unsigned int t=_i1;
    _i1=_i2;
    _i2=_i3;
    _i3=t;
  }
  
  if (_i2<_i1)
  {
    unsigned int t=_i1;
    _i1=_i2;
    _i2=_i3;
    _i3=t;
  }

  sF32 minx = sMin( vertices[_i1].v.x, vertices[_i2].v.x);
  sF32 miny = sMin( vertices[_i1].v.y, vertices[_i2].v.y);
  sF32 minz = sMin( vertices[_i1].v.z, vertices[_i2].v.z);

  sF32 maxx = sMax( vertices[_i1].v.x, vertices[_i2].v.x);
  sF32 maxy = sMax( vertices[_i1].v.y, vertices[_i2].v.y);
  sF32 maxz = sMax( vertices[_i1].v.z, vertices[_i2].v.z);

  minx = sMin( vertices[_i3].v.x, minx);
  miny = sMin( vertices[_i3].v.y, miny);
  minz = sMin( vertices[_i3].v.z, minz);

  maxx = sMax( vertices[_i3].v.x, maxx);
  maxy = sMax( vertices[_i3].v.y, maxy);
  maxz = sMax( vertices[_i3].v.z, maxz);

  tri.aabb.Min.Init(minx,miny,minz);
  tri.aabb.Max.Init(maxx,maxy,maxz);
  tri.i1 = _i1;
  tri.i2 = _i2;
  tri.i3 = _i3;
  tri.m  = ~0;  
  tri.n  = _n;
  tri.e1 = FindAddEdge(_i1, _i2);
  tri.e2 = FindAddEdge(_i2, _i3);
  tri.e3 = FindAddEdge(_i3, _i1);  
  
  edges[tri.e1].n += _n;
  edges[tri.e2].n += _n;
  edges[tri.e3].n += _n;


#if 1 //Test implementation
  //vertices[_i1].n += n;
  //vertices[_i2].n += n;
  //vertices[_i3].n += n;

  /*
  e1 = vertices[_i2].v - vertices[_i1].v;
  e2 = vertices[_i3].v - vertices[_i1].v;
  a  = e1.Angle(e2);
  vertices[_i1].n += a*n;
  vertices[_i1].n.Unit();

  e1 = vertices[_i1].v - vertices[_i2].v;
  e2 = vertices[_i3].v - vertices[_i2].v;
  a  = e1.Angle(e2);
  vertices[_i2].n += a*n;
  vertices[_i2].n.Unit();

  e1 = vertices[_i1].v - vertices[_i3].v;
  e2 = vertices[_i2].v - vertices[_i3].v;
  a  = e1.Angle(e2);
  vertices[_i3].n += a*n;
  vertices[_i3].n.Unit();
*/
#else //old implementation
  sVector30 e1;
  sVector30 e2;  
  sF32 a;

  e1 = vertices[_i2].v - vertices[_i1].v;
  e2 = vertices[_i3].v - vertices[_i1].v;
  a  = e1.Angle(e2);
  vertices[_i1].n += a*n;

  e1 = vertices[_i1].v - vertices[_i2].v;
  e2 = vertices[_i3].v - vertices[_i2].v;
  a  = e1.Angle(e2);
  vertices[_i2].n += a*n;

  e1 = vertices[_i1].v - vertices[_i3].v;
  e2 = vertices[_i2].v - vertices[_i3].v;
  a  = e1.Angle(e2);
  vertices[_i3].n += a*n;
#endif

  sInt id = tris.GetCount();
  tris.AddTail(tri);

  Add(head,id);
}


void tAABBoxOctree::Add(tAABBoxOctreeChild * _c, sInt _id)
{
  //In AABB
  if (IsIn(tris[_id].aabb,_c->aabb))
  {
    if (!_c->leaf)
    {
      for (int i=0;i<8;i++)
      {       
        Add(_c->child[i], _id); 
      }
    }
    else
    {
      _c->ta.AddTail(_id);
    }
  }
}


static sF32 s_Octab[8][3] =
{
  0,0,0,
  1,0,0,
  0,1,0,
  1,1,0,
  0,0,1,
  1,0,1,
  0,1,1,
  1,1,1
};

tAABBoxOctreeChild *tAABBoxOctree::InitChild(int _depth, sAABBox _aabb)
{
  if (_depth==maxdepth)
    return 0;

  sAABBox aabb;
  tAABBoxOctreeChild *res = new tAABBoxOctreeChild;
 
  res->leaf = _depth==(maxdepth-1);
  res->aabb = _aabb;    

  _depth++;

  sF32 hwx =(_aabb.Max.x - _aabb.Min.x)/2;
  sF32 hwy =(_aabb.Max.y - _aabb.Min.y)/2;
  sF32 hwz =(_aabb.Max.z - _aabb.Min.z)/2;

  for (int i=0;i<8;i++)
  {
    aabb.Min.x = _aabb.Min.x + hwx * s_Octab[i][0];
    aabb.Min.y = _aabb.Min.y + hwy * s_Octab[i][1];
    aabb.Min.z = _aabb.Min.z + hwz * s_Octab[i][2];
    aabb.Max.x = _aabb.Min.x + hwx * (s_Octab[i][0] + 1.0f);
    aabb.Max.y = _aabb.Min.y + hwy * (s_Octab[i][1] + 1.0f);
    aabb.Max.z = _aabb.Min.z + hwz * (s_Octab[i][2] + 1.0f);
    res->child[i] = InitChild(_depth, aabb);
  }

  return res;
}

tAABBoxOctree::tAABBoxOctree(int _depth)
{  
  maxdepth = _depth;
  vertices.HintSize( 1000000 );
  m = 0;      
}

tAABBoxOctreeChild *tAABBoxOctree::Free(tAABBoxOctreeChild *_c)
{
  
  if (!_c->leaf)
  {    
    for (int i=0;i<8;i++)
    {
      if (_c->child[i])
        Free(_c->child[i]);
    }   
  }
  delete _c;
  return 0;
}

tAABBoxOctree::~tAABBoxOctree()
{
  if (head)
    Free(head);
}



#define USE_NEW_ALG

enum
{
  TH_V0 = 0,
  TH_V1,
  TH_V2,
  TH_E0,
  TH_E1,
  TH_E2,
  TH_IN
};



//http://www.geometrictools.com/Documentation/DistancePoint3Triangle3.pdf
//http://www.geometrictools.com/LibMathematics/Distance/Distance.html

sF32 tAABBoxOctree::GetDistanceToTriangleSq(sInt _tri, sVector31 &_p, sBool &_isneg)
{
  unsigned int i1=tris[_tri].i1;
  unsigned int i2=tris[_tri].i2;
  unsigned int i3=tris[_tri].i3;

  sVector31 _a= vertices[i1].v;
  sVector31 _b= vertices[i2].v;
  sVector31 _c= vertices[i3].v;

  _isneg = sFALSE;

#ifdef USE_NEW_ALG    
  sVector30 diff  = _a - _p;
  sVector30 edge0 = _b - _a;
  sVector30 edge1 = _c - _a;
  double a00 = edge0.LengthSq();
  double a01 = edge0^edge1;
  double a11 = edge1.LengthSq();
  double b0 = diff^edge0;
  double b1 = diff^edge1;
  double c = diff.LengthSq();
  double det = a00*a11 - a01*a01; det=det<0.0f ? -det:det;
  double s = a01*b1 - a11*b0;
  double t = a01*b0 - a00*b1;
  double sqrDistance;

  if (s + t <= det)
  {
    if (s < 0.0)
    {
      if (t < 0.0)  // region 4
      {
        if (b0 < 0.0)
        {
          t = 0.0;
          if (-b0 >= a00)
          {
            s = 1;
            sqrDistance = a00 + (2)*b0 + c;
          }
          else
          {
            s = -b0/a00;
            sqrDistance = b0*s + c;
          }
        }
        else
        {
          s = 0;
          if (b1 >= 0)
          {
            t = 0;
            sqrDistance = c;
          }
          else if (-b1 >= a11)
          {
            t = 1;
            sqrDistance = a11 + (2)*b1 + c;
          }
          else
          {
            t = -b1/a11;
            sqrDistance = b1*t + c;
          }
        }
      }
      else  // region 3
      {
        s = 0;
        if (b1 >= 0)
        {
          t = 0;
          sqrDistance = c;
        }
        else if (-b1 >= a11)
        {
          t = 1;
          sqrDistance = a11 + (2)*b1 + c;
        }
        else
        {
          t = -b1/a11;
          sqrDistance = b1*t + c;
        }
      }
    }
    else if (t < 0)  // region 5
    {
      t = 0;
      if (b0 >= 0)
      {
        s = 0;
        sqrDistance = c;
      }
      else if (-b0 >= a00)
      {
        s = 1;
        sqrDistance = a00 + (2)*b0 + c;
      }
      else
      {
        s = -b0/a00;
        sqrDistance = b0*s + c;
      }
    }
    else  // region 0
    {
      // minimum at interior point
      double invDet = (1)/det;
      s *= invDet;
      t *= invDet;
      sqrDistance = s*(a00*s + a01*t + (2)*b0) +
        t*(a01*s + a11*t + (2)*b1) + c;
    }
  }
  else
  {
    double tmp0, tmp1, numer, denom;

    if (s < 0)  // region 2
    {
      tmp0 = a01 + b0;
      tmp1 = a11 + b1;
      if (tmp1 > tmp0)
      {
        numer = tmp1 - tmp0;
        denom = a00 - (2)*a01 + a11;
        if (numer >= denom)
        {
          s = 1;
          t = 0;
          sqrDistance = a00 + (2)*b0 + c;
        }
        else
        {
          s = numer/denom;
          t = 1 - s;
          sqrDistance = s*(a00*s + a01*t + (2)*b0) +
            t*(a01*s + a11*t + (2)*b1) + c;
        }
      }
      else
      {
        s = 0;
        if (tmp1 <= 0)
        {
          t = 1;
          sqrDistance = a11 + (2)*b1 + c;
        }
        else if (b1 >= 0)
        {
          t = 0;
          sqrDistance = c;
        }
        else
        {
          t = -b1/a11;
          sqrDistance = b1*t + c;
        }
      }
    }
    else if (t < 0)  // region 6
    {
      tmp0 = a01 + b1;
      tmp1 = a00 + b0;
      if (tmp1 > tmp0)
      {
        numer = tmp1 - tmp0;
        denom = a00 - (2)*a01 + a11;
        if (numer >= denom)
        {
          t = 1;
          s = 0;
          sqrDistance = a11 + (2)*b1 + c;
        }
        else
        {
          t = numer/denom;
          s = 1 - t;
          sqrDistance = s*(a00*s + a01*t + (2)*b0) +
            t*(a01*s + a11*t + (2)*b1) + c;
        }
      }
      else
      {
        t = 0;
        if (tmp1 <= 0)
        {
          s = 1;
          sqrDistance = a00 + (2)*b0 + c;
        }
        else if (b0 >= 0)
        {
          s = 0;
          sqrDistance = c;
        }
        else
        {
          s = -b0/a00;
          sqrDistance = b0*s + c;
        }
      }
    }
    else  // region 1
    {
      numer = a11 + b1 - a01 - b0;
      if (numer <= 0)
      {
        s = 0;
        t = 1;
        sqrDistance = a11 + (2)*b1 + c;
      }
      else
      {
        denom = a00 - (2)*a01 + a11;
        if (numer >= denom)
        {
          s = 1;
          t = 0;
          sqrDistance = a00 + (2)*b0 + c;
        }
        else
        {
          s = numer/denom;
          t = 1 - s;
          sqrDistance = s*(a00*s + a01*t + (2)*b0) +
            t*(a01*s + a11*t + (2)*b1) + c;
        }
      }
    }
  }

  // Account for numerical round-off error.
  if (sqrDistance < 0)
  {
    sqrDistance = 0;
  }


  sVector30 n;
  if (s==0.0 && t==0.0)
  {
    n=vertices[i1].n; //V0
  }
  else if (s==1.0 && t==0.0)
  {
    n=vertices[i2].n; //V1
  }
  else if (s==0.0 && t==1.0)
  {
    n=vertices[i3].n; //V2
  }
  else if (t==0.0)
  {
    sVERIFY(s<1.0f);
    n=edges[tris[_tri].e1].n;
  }
  else if (s==0.0)
  {
    sVERIFY(t<1.0f);
    n=edges[tris[_tri].e3].n;
  }
  else if ((s+t)==1.0)
  {
    n=edges[tris[_tri].e2].n;
  } 
  else
  {
    n=tris[_tri].n;
  }
//  sVERIFY( (s+t)<1.0f);

  sVector30 vd  = _a-_p;  
  vd.Unit();

  _isneg=sFALSE;
//  if (vd.Length()>0.0f)
  {
    sVERIFY(n.LengthSq()>0.0f)
    sF32 angle = vd^n;
    if(angle>=(0.01f))
    {
      _isneg=sTRUE;
    }
  }


  return (float)sqrDistance;
#else
  sVector30 ab = _b - _a;
  sVector30 ac = _c - _a;
  sVector30 ap = _p - _a;

  sF32 d1 = ab ^ ap;
  sF32 d2 = ac ^ ap;

  if (d1<= 0.0f && d2<= 0.0f) 
    return (ap).LengthSq();

  sVector30 bp = _p - _b;

  sF32 d3 = ab ^ bp;
  sF32 d4 = ac ^ bp;

  if (d3>= 0.0f && d4<=d3) 
    return bp.LengthSq();

  sVector30 cp = _p - _c;
  sF32 d5 = ab ^ cp;
  sF32 d6 = ac ^ cp;
  if (d6>= 0.0f && d5<= d6) 
    return cp.LengthSq();


  sF32 vc = d1*d4 - d3*d2;

  if (vc<=0.0f && d1 >= 0.0f && d3 <= 0.0f)
  {
    sF32 v = d1 / (d1-d3);
    sVector30 res = _a + v * ab - _p;
    return res.LengthSq();
  }

  sF32 vb = d5*d2 - d1*d6;
  if (vb <= 0.0f &&  d2 >= 0.0f && d6 <= 0.0f)
  {
    sF32 w = d2 /(d2-d6);
    sVector30 res = _a + w * ac - _p;
    return res.LengthSq();
  }
  
  float va = d3*d6 - d5*d4;
  if (va <= 0.0f && (d4-d3) >= 0.0f && (d5-d6) >= 0.0f)
  {
    sF32 w = (d4-d3) / ((d4-d3) + (d5-d6));
    sVector30 res = _b + w * (_c - _b ) - _p;
    return res.LengthSq();
  }

  sF32 denom = 1.0f / (va+vb+vc);
  sF32 v = vb * denom;
  sF32 w = vc * denom;
  
  sVector30 res = _a + (v * ab) + (w * ac)  - _p;
  return res.LengthSq();
#endif
}

#define TRON_EPSILON 1e-16f

sF32 tAABBoxOctree::GetClosestDistance(sVector31 &_p, unsigned int *_id, sBool _bruteforce, sBool _nobsp)
{   
  sF32 d = 10000000.0f;  
  sInt id = -1;

  sVector31 p = _p;

  if (_bruteforce)
  {
    sBool in=false;

    //Brute Force
    for (int i=0;i<tris.GetCount();i++)
    {
      sBool isneg;
      sF32 td = GetDistanceToTriangleSq(i,p,isneg);

      sF32 diff=td-d;
      sF32 adiff=sAbs(diff);

      if (adiff<TRON_EPSILON) //Check
      {
        if (!isneg)
        {
          id=i;
          d=td;
          in=isneg;
        }
      }
      else if (diff<0.0f)
      {
        id=i;
        d=td;
        in=isneg;
      }    
    }  

    sVERIFY(id!=-1);

    d = sFSqrt(d);
    //if (in)
    if (_nobsp&&bsp.IsInside(_p))
      d=-d;
  }
  else
  {
    sBool isneg=false;
    GetClosestDistance(head, p, d, id, m, isneg);
    //sVERIFY(id!=-1);
    if (id==-1)
      return 0.0f;   
    d = sFSqrt(d);
    //if (isneg)
    if (_nobsp&&bsp.IsInside(_p))
      d=-d;
  }

  m++;

  if (_id)
    *_id = id;

  return d;
}

void tAABBoxOctree::GetClosestDistance(tAABBoxOctreeChild *_c, sVector31 &_p, sF32 &_d, sInt &_id, unsigned int _m, sBool &_isneg )
{
  if (_c==0)
    return;

  if (_c->leaf)
  {
    for (sInt i=0;i<_c->ta.GetCount();i++)
    {
      tAABBoxOctreeTri *t = &tris[_c->ta[i]];
      //if (t->m != m)
      {
        //t->m = m;       
        sF32 df = t->aabb.DistanceToSq(_p);  //Get first distance to AABB
        if (df<_d)        
        {
          sBool isneg;
          df = GetDistanceToTriangleSq( _c->ta[i], _p, isneg);
          if (df<_d)
          {
            _d  = df;
            _id = _c->ta[i];
            _isneg = isneg;
          }
        }
      }
    }
  }
  else
  {
    sF32 cd[8];
    sF32 td = _d;
    int  j=-1;

    for (int i=0;i<8;i++)
    {
      if (_c->child[i])
      {
        cd[i] = _c->child[i]->aabb.DistanceToSq(_p);

        if (cd[i]<td)
        {
          td=cd[i];
          j=i;
        }
      }
    }
     
    if (j!=-1)
    {
      GetClosestDistance(_c->child[j], _p, _d, _id, _m, _isneg);

      for (int i=0;i<8;i++)
      {
        if (_c->child[i])
        {
          if ((cd[i]<_d) && i!=j)
          {
            GetClosestDistance(_c->child[i], _p, _d, _id, _m, _isneg);
          }
        }
      }
    }
  }
}

void tAABBoxOctree::CalcBox(sF32 _guardband, sBool _cube, sBool UserBox, const sVector31 &BoxPos, const sVector30 &BoxDimH)
{
  int i;
  sF32  minx;  
  sF32  maxx;
  sF32  miny;  
  sF32  maxy;
  sF32  minz;  
  sF32  maxz;

  if (UserBox)
  {
    minx = BoxPos.x-BoxDimH.x;
    miny = BoxPos.y-BoxDimH.y;
    minz = BoxPos.z-BoxDimH.z;
    maxx = BoxPos.x+BoxDimH.x;
    maxy = BoxPos.y+BoxDimH.y;
    maxz = BoxPos.z+BoxDimH.z;
  }
  else
  {  
    if (vertices.GetCount()==0)
      return;

    minx = vertices[0].v.x;  
    maxx = vertices[0].v.x;

    miny = vertices[0].v.y;  
    maxy = vertices[0].v.y;

    minz = vertices[0].v.z;  
    maxz = vertices[0].v.z;

    for (i=1;i<vertices.GetCount();i++)
    {
      minx = sMin( minx, vertices[i].v.x);
      miny = sMin( miny, vertices[i].v.y);
      minz = sMin( minz, vertices[i].v.z);

      maxx = sMax( maxx, vertices[i].v.x);
      maxy = sMax( maxy, vertices[i].v.y);
      maxz = sMax( maxz, vertices[i].v.z);
    }

    minx -= _guardband;
    miny -= _guardband;
    minz -= _guardband;
    maxx += _guardband;
    maxy += _guardband;
    maxz += _guardband;
  }

  if (_cube)
  {
    minx=sMin(sMin(minx,miny),minz);
    maxx=sMax(sMax(maxx,maxy),maxz);
    miny=minz=minx;
    maxy=maxz=maxx;
  }

  box.Min.Init(minx,miny,minz);
  box.Max.Init(maxx,maxy,maxz);
}

void tAABBoxOctree::FinishVertices(sF32 _guardband, sBool _cube, sBool UserBox, const sVector31 &BoxPos, const sVector30 &BoxDimH)
{ 
  CalcBox(_guardband, _cube, UserBox, BoxPos, BoxDimH);
 
  sAABBox aabb = box;  
  head = InitChild(0,aabb);

  edges.HintSize(vertices.GetCount()*4);
}

void tAABBoxOctree::NormVertex()
{
  int i; 
 
  sF32  minx = vertices[0].v.x;  
  sF32  maxx = vertices[0].v.x;

  sF32  miny = vertices[0].v.y;  
  sF32  maxy = vertices[0].v.y;

  sF32  minz = vertices[0].v.z;  
  sF32  maxz = vertices[0].v.z;

  for (i=1;i<vertices.GetCount();i++)
  {
    minx = sMin( minx, vertices[i].v.x);
    miny = sMin( miny, vertices[i].v.y);
    minz = sMin( minz, vertices[i].v.z);

    maxx = sMax( maxx, vertices[i].v.x);
    maxy = sMax( maxy, vertices[i].v.y);
    maxz = sMax( maxz, vertices[i].v.z);
  }

  sF32 wx = maxx-minx;
  sF32 wy = maxy-miny;
  sF32 wz = maxz-minz;
  sF32 w;

  
  w = sMax(wx,wy);
  w = sMax(wz,w);

  sF32 sf = 2.0f/w;

  sF32 sx = wx/2 - maxx;
  sF32 sy = wy/2 - maxy;
  sF32 sz = wz/2 - maxz;

  for (i=0;i<vertices.GetCount();i++)
  {
    vertices[i].v.x =  (vertices[i].v.x + sx) * sf;
    vertices[i].v.y =  (vertices[i].v.y + sy) * sf;
    vertices[i].v.z =  (vertices[i].v.z + sz) * sf;
  }
}

tAABBoxOctreeChild *tAABBoxOctree::Kill(tAABBoxOctreeChild *_c)
{
  if (_c->leaf)
  {
    if (_c->ta.IsEmpty())
    {
      delete _c;
      return 0;
    }

    return _c;
  }
  else
  {
    int j=0;
    for (int i=0;i<8;i++)
    {
      _c->child[i] = Kill(_c->child[i]);
      if (_c->child[i]==0)
        j++;
    }
    if (j==8)
    {
      delete _c;
      return 0;
    }
    return _c;
  }
}

void tAABBoxOctree::Finalize()
{
  for (int i=0;i<edges.GetCount();i++)
  {
    edges[i].n.Unit();
  }
  for (int i=0;i<vertices.GetCount();i++)
  {
    vertices[i].n.Unit();
  }

  head = Kill(head);
}

//-------------------

tSDF::tSDF()
{
  DimX = DimY = DimZ  = DimXY = 0;
  Box.Min.Init(0,0,0);
  Box.Max.Init(0,0,0);
  SDF = 0;
  GuardBand=0.0f;
}

tSDF::~tSDF()
{
  if (SDF)
    delete []SDF;
}

void tSDF::Init(sChar *fname)
{
  sVERIFY(fname);
  sFile *fp = sCreateFile((const sChar *)fname,sFA_READ);  
  sVERIFY(fp);
  
  sBool b;
  sU32 id;
  sU32 size;

  b = fp->Read(&id, 4);
  b = b && fp->Read(&DimX, 4);
  b = b && fp->Read(&DimY, 4);
  b = b && fp->Read(&DimZ, 4);
  b = b && fp->Read(&InBox, sizeof(InBox));

  sVERIFY(DimX>0);
  sVERIFY(DimY>0);
  sVERIFY(DimZ>0);

  size = DimX*DimY*DimZ;
  SDF  = new sF32[size];

  b = b && fp->Read(SDF, size*4);    

  DimXY = DimX*DimY;
  sF32 wx  = (InBox.Max.x - InBox.Min.x);
  sF32 wy  = (InBox.Max.y - InBox.Min.y);
  sF32 wz  = (InBox.Max.z - InBox.Min.z);

  PStepX = wx / (DimX-2);
  PStepY = wy / (DimY-2);
  PStepZ = wz / (DimZ-2);

  //Calc Inner Bounding Box
  Box.Min.x = InBox.Min.x + PStepX;
  Box.Max.x = InBox.Max.x - PStepX;
  Box.Min.y = InBox.Min.y + PStepY;
  Box.Max.y = InBox.Max.y - PStepY;
  Box.Min.z = InBox.Min.z + PStepZ;
  Box.Max.z = InBox.Max.z - PStepZ;

  STBX  = (DimX-1) / wx;
  STBY  = (DimY-1) / wy;
  STBZ  = (DimZ-1) / wz;

  sVERIFY(b);
  delete fp;  
}

struct tSDF_Create
{
  tSDF          *sdf;
  tAABBoxOctree *oct;
  sBool          bruteforce;
};


void TaskCodeSDF(sStsManager *m,sStsThread *th,sInt start,sInt count,void *data)
{
  sVERIFY(count==1);
  sInt z=start;
  sVector31 p;
  tSDF_Create *mi=(tSDF_Create *)data;
  sF32 *d=mi->sdf->SDF+z*mi->sdf->DimXY;


  //for (sInt z=0;z<DimZ;z++)
  {
    p.z = z * mi->sdf->PStepZ + mi->sdf->InBox.Min.z;// + mi->sdf->PStepZ/2;
    for (sInt y=0;y<mi->sdf->DimY;y++)
    {
      p.y = y * mi->sdf->PStepY + mi->sdf->InBox.Min.y;// + mi->sdf->PStepY/2;
      for (sInt x=0;x<mi->sdf->DimX;x++)
      {
        p.x = x * mi->sdf->PStepX + mi->sdf->InBox.Min.x;// + mi->sdf->PStepX/2;
        *d++ = mi->oct->GetClosestDistance(p,0,mi->bruteforce);
      }
    }
  }  

}

void tSDF::Init(tAABBoxOctree *oct, sInt depth, sBool bruteforce, sF32 guardband)
{
  sVERIFY(oct);
  sVERIFY(depth>=0 && depth<12);

  GuardBand=guardband;

  //Get Box from oct
  Box = oct->box;

  sF32 wx = Box.Max.x - Box.Min.x;
  sF32 wy = Box.Max.y - Box.Min.y;
  sF32 wz = Box.Max.z - Box.Min.z;

  sVERIFY(wx>=0);
  sVERIFY(wy>=0);
  sVERIFY(wz>=0);

  //Determine max
  sF32 w = sMax(sMax(wx,wy),wz);
  
  //Calc required Dimension
  DimX = (int)(sRoundUp(wx/w*(1<<depth)));
  DimY = (int)(sRoundUp(wy/w*(1<<depth)));
  DimZ = (int)(sRoundUp(wz/w*(1<<depth)));

  DimX = sMax(DimX,4);
  DimY = sMax(DimY,4);
  DimZ = sMax(DimZ,4);

  //if (DimX<DimZ)
  //{
  //  DimX=DimZ;
  //}
  //if (DimX<DimY)
  //{
  //  DimX=DimY;
  //}
  //DimY=DimZ=DimX;

  DimXY = DimX*DimY;

  wx = wx / (DimX-2) * DimX;
  wy = wy / (DimY-2) * DimY;
  wz = wz / (DimZ-2) * DimZ;

  PStepX = wx / (DimX-2);
  PStepY = wy / (DimY-2);
  PStepZ = wz / (DimZ-2);

  //Calc Inner Bounding Box
  InBox.Min.x = Box.Min.x - PStepX*1;
  InBox.Max.x = Box.Max.x + PStepX*1;
  InBox.Min.y = Box.Min.y - PStepY*1;
  InBox.Max.y = Box.Max.y + PStepY*1;
  InBox.Min.z = Box.Min.z - PStepZ*1;
  InBox.Max.z = Box.Max.z + PStepZ*1;


  SDF  = new sF32[DimX*DimY*DimZ];

  sVector31 p;  


#if MULTICORE==1
  sInt ms =  sGetTime();
  sDPrintF(L"Start building Distance Field ..... %d \n ",ms);
  
  tSDF_Create sc;
  sc.oct=oct;
  sc.sdf=this;
  sc.bruteforce=bruteforce;
  
  sStsWorkload *wl = sSched->BeginWorkload();
  sStsTask *task = wl->NewTask(TaskCodeSDF,&sc,DimZ,0);
  wl->AddTask(task);
  wl->Start();
  wl->Sync();
  wl->End();
    
  sDPrintF(L"needed %5.3f[sec] / %5.3f [minutes] / %5.3f [hours] \n ",(sGetTime()-ms)/1000.0f,(sGetTime()-ms)/1000.0f/60,(sGetTime()-ms)/1000.0f/3600);
#else

  sF32 *d = SDF;

  sInt ms =  sGetTime();
  sDPrintF(L"Start building Distance Field ..... %d \n ",ms);

  for (sInt z=0;z<DimZ;z++)
  {
    p.z = z * PStepZ + InBox.Min.z;
    for (sInt y=0;y<DimY;y++)
    {
      p.y = y * PStepY + InBox.Min.y;
      for (sInt x=0;x<DimX;x++)
      {
        p.x = x * PStepX + InBox.Min.x;
        *d++ = oct->GetClosestDistance(p);
      }
    }
    float until = ((z+1)/(float)DimZ);            
    float to    = 1.0f-until;
    float time = (sGetTime()-ms)/1000.0f; 
    to = time / until * to;
    sDPrintF(L"(%d/%d %5.3f (%5.3f,%5.3f) , %5.3f (%5.3f %5.3f)\n",z+1,DimZ,time,time/60.0f,time/3600.0f,to,to/60.0f,to/3600.0f);  
  }  
  sDPrintF(L"needed %5.3f[sec] / %5.3f [minutes] / %5.3f [hours] \n ",(sGetTime()-ms)/1000.0f,(sGetTime()-ms)/1000.0f/60,(sGetTime()-ms)/1000.0f/3600);

#endif

  STBX  = (DimX-1) / wx;
  STBY  = (DimY-1) / wy;
  STBZ  = (DimZ-1) / wz;
}

void tSDF::Init(sF32 *distancefield, sAABBox &box, sInt dimx, sInt dimy, sInt dimz)
{
  sVERIFY(0);
}

void tSDF::WriteToFile(sChar *fname)
{
  sVERIFY(fname);
  sFile *fp = sCreateFile((const sChar *)fname,sFA_WRITE);  
  sVERIFY(fp);
  char id[4] = "SDF";
  sBool b = fp->Write(id,4);
  b = b && fp->Write(&DimX, 4);
  b = b && fp->Write(&DimY, 4);
  b = b && fp->Write(&DimZ, 4);
  b = b && fp->Write(&InBox, sizeof(InBox));    
  b = b && fp->Write(SDF,DimX*DimY*DimZ*4);
}

sBool tSDF::IsInBox(const sVector31 &pos)
{
  return Box.HitPoint(pos);
}



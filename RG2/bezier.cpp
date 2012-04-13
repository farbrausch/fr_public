// This code is in the public domain. See LICENSE for details.

// 2d bezier curves implementation

#include "stdafx.h"
#include "bezier.h"
#include "types.h"
#include "math3d_2.h"
#include "tool.h"

namespace fr
{
  // ---- cubicBezierSegment2D

  void cubicBezierSegment2D::eval(vector2& out, sF32 t) const
  {
    sF32 tt = t * t, ttt = tt * t;

    out.scale(A, 1-3*t+3*tt-ttt);
    out.addscale(B, 3*t-6*tt+3*ttt);
    out.addscale(C, 3*tt-3*ttt);
    out.addscale(D, ttt);
  }

  vector2 cubicBezierSegment2D::nearestPointOnCurve(const vector2& P) const
  {
    sInt    nSolutions;
    vector2 w[6], p;
    sF32    tCandidate[5];

    convertToBezierForm(w, P);
    nSolutions=findRoots(w, tCandidate);

    sF32  dist=(P-A).lenSq(), t=0;

    for (sInt i=0; i<nSolutions; i++)
    {
      eval(p, tCandidate[i]);
      sF32 newDist=(P-p).lenSq();

      if (newDist<dist)
      {
        dist=newDist;
        t=tCandidate[i];
      }
    }

    sF32 newDist=(P-D).lenSq();
    if (newDist<dist)
      t=1.0f;

    return eval(t);
  }

  // ---- tools

  void cubicBezierSegment2D::convertToBezierForm(vector2 *w, const vector2& P) const
  {
    vector2 c[4], d[3];
    sF64    cdTable[3][4];
    sInt    i;

    c[0].sub(A, P); c[1].sub(B, P); c[2].sub(C, P); c[3].sub(D, P);
    d[0].sub(B, A); d[1].sub(C, B); d[2].sub(D, C);
    d[0].scale(3);  d[1].scale(3);  d[2].scale(3);

    for (sInt row=0; row<=2; row++)
    {
      for (sInt col=0; col<=3; col++)
        cdTable[row][col]=d[row].dot(c[col]);
    }
    
    for (i=0; i<=5; i++)
    {
      w[i].y=0.0f;
      w[i].x=i/5.0f;
    }
    
    static const sF32 z[3][4]={
      { 1.0f, 0.6f, 0.3f, 0.1f},
      { 0.4f, 0.6f, 0.6f, 0.4f},
      { 0.1f, 0.3f, 0.6f, 1.0f}
    };
    
    sInt n=3, m=2, k;
    for (k=0; k<=n+m; k++)
    {
      sInt lb=maximum(0, k-m);
      sInt ub=minimum(k, n);
      
      for (i=lb; i<=ub; i++)
      {
        sInt j=k-i;
        w[i+j].y+=cdTable[j][i]*z[j][i];
      }
    }
  }

  sInt cubicBezierSegment2D::findRoots(const vector2 *w, sF32 *t, sInt depth) const
  {
    sInt cc=crossingCount(w, 5);

    switch (cc)
    {
    case 0:
      return 0;
      
    case 1:
      if (depth>=64)
      {
        t[0]=(w[0].x+w[5].x)*0.5;
        return 1;
      }
      
      if (controlPolygonFlatEnough(w, 5))
      {
        t[0]=computeXIntercept(w, 5);
        return 1;
      }
    }
    
    vector2   left[6], right[6], vTemp[6][6];
    sInt      i;
    
    for (i=0; i<6; i++)
      vTemp[0][i]=w[i];
    
    for (i=1; i<6; i++)
    {
      for (sInt j=0; j<6-i; j++)
        vTemp[i][j]=0.5f*(vTemp[i-1][j]+vTemp[i-1][j+1]);
    }

    for (i=0; i<6; i++)
    {
      left[i]=vTemp[i][0];
      right[5-i]=vTemp[i][5-i];
    }
    
    sF32      lt[6], rt[6];
    sInt      lCount=findRoots(left, lt, depth+1);
    sInt      rCount=findRoots(right, rt, depth+1);
    
    for (i=0; i<lCount; i++)
      t[i]=lt[i];
    
    for (i=0; i<rCount; i++)
      t[i+lCount]=rt[i];
    
    return lCount+rCount;
  }

  sInt cubicBezierSegment2D::crossingCount(const vector2 *V, sInt degree) const
  {
    sInt i, nCrossings=0;
    
    for (i=0; i<degree; i++)
    {
      if (V[i].y*V[i+1].y<0)
        nCrossings++;
    }
    
    return nCrossings;
  }

  sBool cubicBezierSegment2D::controlPolygonFlatEnough(const vector2 *V, sInt degree) const
  {
    sInt i;
    sF32 a=V[0].y-V[degree].y, b=V[degree].x-V[0].x, c=V[0].x*V[degree].y-V[degree].x*V[0].y;
    sF32 invABSquared=1.0f / (a*a+b*b);
    sF32 maxDistAbove=0, maxDistBelow=0;
    
    for (i=1; i<degree; i++)
    {
      sF32 d=a*V[i].x+b*V[i].y+c, dv=d*d*invABSquared;
      
      if (d<0)
        maxDistBelow=minimum(maxDistBelow, -dv);
      else
        maxDistAbove=maximum(maxDistAbove, dv);
    }
    
    return fabs(maxDistAbove-maxDistBelow)<(-2e-8f * a);
  }

  sF32 cubicBezierSegment2D::computeXIntercept(const vector2 *V, sInt degree) const
  {
    sF32 XNM=V[degree].x-V[0].x, YNM=V[degree].y-V[0].y, XMK=V[0].x, YMK=V[0].y;
    return -(XNM*YMK-YNM*XMK)/YNM;
  }
}

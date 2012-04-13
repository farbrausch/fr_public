// This code is in the public domain. See LICENSE for details.

#ifndef __fr_bezier_h_
#define __fr_bezier_h_

#include "types.h"
#include "math3d_2.h"

namespace fr
{
  // ---- cubic bezier curve segment

  class cubicBezierSegment2D
  {
  public:
    vector2 A, B, C, D;
    
    inline cubicBezierSegment2D()                                               { }
    inline cubicBezierSegment2D(const vector2& _A, const vector2& _B, const vector2& _C, const vector2& _D)
    {
      A=_A;
      B=_B;
      C=_C;
      D=_D;
    }
    
    inline void     set(const vector2& _A, const vector2& _B, const vector2& _C, const vector2& _D)
    {
      A=_A;
      B=_B;
      C=_C;
      D=_D;
    }
    
    void eval(vector2 &out, sF32 t) const;
    inline vector2  eval(sF32 t) const                        { vector2 temp; eval(temp, t); return temp; }
    
    vector2 nearestPointOnCurve(const vector2& P) const;
    
  private:
    void convertToBezierForm(vector2 *w, const vector2& P) const;
    sInt findRoots(const vector2 *w, sF32 *t, sInt depth=0) const;
    sInt crossingCount(const vector2 *V, sInt degree) const;
    sBool controlPolygonFlatEnough(const vector2 *V, sInt degree) const;
    sF32 computeXIntercept(const vector2 *V, sInt degree) const;
  };

  // ---- cubic bezier curve

  class cubicBezierCurve2D
  {
  public:
    cubicBezierSegment2D* segment;
    sInt                  nSegments;

    inline cubicBezierCurve2D(sInt nSegs)             { segment=new cubicBezierSegment2D[nSegs]; nSegments=nSegs; }
    inline cubicBezierCurve2D(const cubicBezierCurve2D& x)
    {
      nSegments=x.nSegments;
      segment=new cubicBezierSegment2D[nSegments];
      
      for (sInt i=0; i<nSegments; i++)
        segment[i]=x.segment[i];
    }

    inline ~cubicBezierCurve2D()                      { delete[] segment; }

    cubicBezierCurve2D &operator = (const cubicBezierCurve2D &x)
    {
      delete[] segment;
      nSegments=x.nSegments;
      segment=new cubicBezierSegment2D[nSegments];

      for (sInt i=0; i<nSegments; i++)
        segment[i]=x.segment[i];
    }

    void resize(sInt newSize)
    {
      delete[] segment;
      nSegments=newSize;
      segment=new cubicBezierSegment2D[nSegments];
    }

    vector2 nearestPointOnCurve(const vector2& P) const
    {
      vector2 minVec;
      sF32    minDist=0.0f;

      for (sInt i=0; i<nSegments; i++)
      {
        vector2 vec=segment[i].nearestPointOnCurve(P);
        sF32    dist=(P-vec).lenSq();

        if (dist<minDist || i==0)
        {
          minDist=dist;
          minVec=vec;
        }
      }

      return minVec;
    }
  };
}

#endif

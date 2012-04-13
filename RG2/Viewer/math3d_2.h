// This code is in the public domain. See LICENSE for details.

#ifndef __fr_math3d_h_
#define __fr_math3d_h_

#include "types.h"
#include "rtlib.h"
#include <math.h>

#pragma warning (push)
#pragma warning (disable: 4244)

struct _D3DMATRIX;

namespace fr
{
  struct matrix;

  // ---- 3d vector

	struct vector
	{
	  union
	  {
	    struct
	    {
	      sF32 x, y, z;
	    };
	    sF32 v[3];
	  };

	  inline vector()																											  { }
    inline vector(const vector &v) : x(v.x), y(v.y), z(v.z)               { }
    inline vector(sF32 a) : x(a), y(a), z(a)                              { }
    inline vector(sF32 _x, sF32 _y, sF32 _z) : x(_x), y(_y), z(_z)        { }

	  inline void set(sF32 _x, sF32 _y, sF32 _z)                			      { x=_x; y=_y; z=_z; }
	  inline void add(const vector& a) 																			{ x+=a.x; y+=a.y; z+=a.z; }
	  inline void add(const vector& a, const vector& b) 										{ x=a.x+b.x; y=a.y+b.y; z=a.z+b.z; }
	  inline void sub(const vector& a) 																			{ x-=a.x; y-=a.y; z-=a.z; }
	  inline void sub(const vector& a, const vector& b)  										{ x=a.x-b.x; y=a.y-b.y; z=a.z-b.z; }
	  inline void mul(const vector& a)                                      { x*=a.x; y*=a.y; z*=a.z; }
	  inline void mul(const vector& a, const vector& b)                     { x=a.x*b.x; y=a.y*b.y; z=a.z*b.z; }
	  inline void scale(const sF32 f)									    									{ x*=f; y*=f; z*=f; }
    inline void scale(const vector& a, const sF32 f)                      { x=a.x*f; y=a.y*f; z=a.z*f; }
	  
    void addscale(const vector& a, const sF32 f);											
	  void addscale(const vector& a, const vector& b, const sF32 f);	  
    inline void subscale(const vector& a, const sF32 f)                   { addscale(a, -f); }
    inline void subscale(const vector& a, const vector& b, const sF32 f)  { addscale(a, b, -f); }
    void transform(const vector& src, const matrix &m);
		void transform(const matrix &m);          
	  
	  sF32       			dot(const vector& a) const; 
	  inline sF32			lenSq() const                             { return x*x+y*y+z*z; }
	  inline sF32			len() const                               { return sqrt(x*x+y*y+z*z); }

		inline vector   operator +  (const vector& a) const 			{ return vector(x+a.x,y+a.y,z+a.z); }
		inline vector   operator -  (const vector& a) const 			{ return vector(x-a.x,y-a.y,z-a.z); }
		inline vector   operator *  (const sF32 s) const					{ return vector(x*s,y*s,z*s); }
		inline vector   operator /  (const sF32 s) const					{ sF32 inv=1.0f/s; return vector(x*inv,y*inv,z*inv); }
		inline sF32 		operator *  (const vector& a) const			  { return x*a.x+y*a.y+z*a.z; }
		inline vector   operator %  (const vector& a) const		  	{ return vector(y*a.z-z*a.y,z*a.x-x*a.z,x*a.y-y*a.x); }
	  inline vector   operator ^  (const vector& a) const       { return vector(x*a.x, y*a.y, z*a.z); }

		inline vector   operator -  () const											{ return vector(-x,-y,-z); }

		inline vector&  operator += (const vector& a) 					  { x+=a.x; y+=a.y; z+=a.z; return *this; }
	  inline vector&  operator -= (const vector& a)	  					{ x-=a.x; y-=a.y; z-=a.z; return *this; }
		inline vector&  operator *= (const sF32 s)	  					  { x*=s; y*=s; z*=s; return *this;}
		inline vector&  operator /= (const sF32 s)  						  { sF32 inv=1.0f/s; x*=inv; y*=inv; z*=inv; return *this; }
		inline vector&  operator %= (const vector& a)		  				{ *this=vector(y*a.z-z*a.y,z*a.x-x*a.z,x*a.y-y*a.x); return *this; }
	  inline vector&  operator ^= (const vector& a)             { x*=a.x, y*=a.y; z*=a.z; return *this;}

    vector          operator *  (const matrix& m) const;
    vector&         operator *= (const matrix& m);
	  
	  void cross(const vector& a, const vector& b);

	  void norm()
	  {
	    sF32 l=lenSq();

	    if (l)
        scale(1.0f/sqrt(l));
	  }

    void norm(sF32 targetLen)
    {
      sF32 l = lenSq();

      if (l)
        scale(targetLen/sqrt(l));
    }

	  inline void reflect(vector &n)
	  {
	    subscale(n, 2.0f*n.dot(*this));
	  }

	  inline void reflect(vector &v, vector &n)
	  {
	    subscale(v, n, 2.0f*n.dot(v));
	  }
	  
	  inline void spin(const vector &b, sF32 angle)
	  {
	    scale(cos(angle));
	    addscale(b, sin(angle));
	  }

    void lerp(const vector &s1, const vector &s2, sF32 t);

    sF32 distanceToSquared(const vector &v2) const;
    sF32 distanceTo(const vector &v2) const                      { return sqrt(distanceToSquared(v2)); }
	};

	inline vector operator * (const sF32 s, const vector& a)
	{
	  return a * s;
	}
	// ---- matrix

	struct matrix
	{
	  sF32 _v[4][4];

	public:
	  __forceinline sF32 &operator ()(const sUInt i, const sUInt j)
	  {
      return _v[i][j];
	  }

	  __forceinline const sF32 &operator ()(const sUInt i, const sUInt j) const
	  {
      return _v[i][j];
    }

	  __forceinline const sF32 eval(const sU32 i, const sU32 j) const
	  {
      return _v[i][j];
    }

	  inline matrix& operator =(const matrix& m)
	  {
	    if (&m!=this)
	      memcpy(_v, m._v, sizeof(_v));

	    return *this;
	  }

	  // rechenoperationen

	  void mul(const matrix& a, const matrix& b);

		inline matrix operator * (const matrix& b) const
	  {
	    matrix out;
	    out.mul(*this, b);
	    return out;
	  }

	  inline matrix& operator *= (const matrix& b)
	  {
	    matrix temp;
	    temp.mul(*this, b);
	    *this=temp;
	    return *this;
	  }

    void transpose(const matrix& in);

	  // bestimmte arten von matrizen erzeugen

    void identity();
	  void rotateHPB(sF32 hd, sF32 pt, sF32 bk);

		inline void rotateHPB(const vector& rot)
		{
			rotateHPB(rot.x, rot.y, rot.z);
		}

	  inline void scale(sF32 x, sF32 y, sF32 z)
	  {
      memset(_v, 0, sizeof(sF32)*4*4);
	    (*this)(0,0)=x;
	    (*this)(1,1)=y;
	    (*this)(2,2)=z;
      (*this)(3,3)=1.0f;
	  }

		inline void scale(const vector& scl)
		{
			scale(scl.x, scl.y, scl.z);
		}

	  inline void translate(sF32 x, sF32 y, sF32 z)
	  {
      identity();
	    (*this)(3,0)=x;
      (*this)(3,1)=y;
      (*this)(3,2)=z;
	  }

		inline void translate(const vector& trn)
		{
			translate(trn.x, trn.y, trn.z);
		}

	  inline void project(sF32 w, sF32 h, sF32 znear, sF32 zfar)
	  {
	    sF32 Q=zfar/(zfar-znear);

      identity();
      (*this)(0,0)=w;
      (*this)(1,1)=h;
      (*this)(2,2)=Q;
      (*this)(2,3)=1;
      (*this)(3,2)=-Q*znear;
      (*this)(3,3)=0;
	  }

		void camera(const vector& pos, const vector& lookAt, const vector& up);

	  inline vector getXVector() const
	  {
	    return vector(eval(0,0), eval(1,0), eval(2,0));
	  }

	  inline vector getYVector() const
	  {
	    return vector(eval(0,1), eval(1,1), eval(2,1));
	  }

	  inline vector getZVector() const
	  {
	    return vector(eval(0,2), eval(1,2), eval(2,2));
	  }
	  
	  inline vector getWVector() const
	  {
	    return vector(eval(0,3), eval(1,3), eval(2,3));
	  }

    _D3DMATRIX *d3d()
    {
      return (_D3DMATRIX *) this;
    }

    const _D3DMATRIX *d3d() const
    {
      return (const _D3DMATRIX *) this;
    }
  };

	// ---- helpers

	struct identityMatrix: matrix
	{
		identityMatrix()
		{
			identity();
		}
	};
}

#pragma warning (pop)

#endif

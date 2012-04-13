// This code is in the public domain. See LICENSE for details.

// math3d implementation

#include "types.h"
#include "math3d_2.h"

namespace fr
{
  // ---- vector

  vector vector::operator * (const matrix& m) const
  {
    vector ret = *this;
    ret.transform(m);

    return ret;
  }

  void vector::addscale(const vector& a, const sF32 f)
  {
    x += a.x * f;
    y += a.y * f;
    z += a.z * f;
  }

  void vector::addscale(const vector& a, const vector& b, const sF32 f)
  {
    x = a.x + b.x * f;
    y = a.y + b.y * f;
    z = a.z + b.z * f;
  }

  void vector::transform(const vector& src, const matrix& m)
  {
    v[0]=src.v[0]*m(0,0)+src.v[1]*m(1,0)+src.v[2]*m(2,0)+m(3,0);
    v[1]=src.v[0]*m(0,1)+src.v[1]*m(1,1)+src.v[2]*m(2,1)+m(3,1);
    v[2]=src.v[0]*m(0,2)+src.v[1]*m(1,2)+src.v[2]*m(2,2)+m(3,2);
  }

  void vector::transform(const matrix& m)
  {
    vector ret=*this;
    transform(ret, m);
  }

  sF32 vector::dot(const vector& a) const
  {
    return x*a.x+y*a.y+z*a.z;
  }

  void vector::cross(const vector& a, const vector& b)
  {
    x=a.y*b.z-a.z*b.y;
    y=a.z*b.x-a.x*b.z;
    z=a.x*b.y-a.y*b.x;
  }

  void vector::lerp(const vector& s1, const vector& s2, sF32 t)
  {
    x=s1.x+(s2.x-s1.x)*t;
    y=s1.y+(s2.y-s1.y)*t;
    z=s1.z+(s2.z-s1.z)*t;
  }

  sF32 vector::distanceToSquared(const vector& v2) const
  {
    return (x-v2.x)*(x-v2.x)+(y-v2.y)*(y-v2.y)+(z-v2.z)*(z-v2.z);
  }

  vector& vector::operator *= (const matrix& m)
  {
    transform(m);
    return *this;
  }

  // ---- matrix

  void matrix::mul(const matrix& a, const matrix& b)
  {
    for (sInt i=0; i<4; i++)
    {
      for (sInt j=0; j<4; j++)
      {
        (*this)(i,j)=a(i,0)*b(0,j)+a(i,1)*b(1,j)+a(i,2)*b(2,j)+a(i,3)*b(3,j);
      }
    }
  }

  void matrix::transpose(const matrix& in)
  {
    for (sInt i=0; i<4; i++)
    {
      for (sInt j=0; j<4; j++)
        (*this)(i,j)=in(j,i);
    }
  }

  void matrix::identity()
  {
    memset(_v, 0, sizeof(sF32)*4*4);
    (*this)(0,0)=1.0f;
    (*this)(1,1)=1.0f;
    (*this)(2,2)=1.0f;
    (*this)(3,3)=1.0f;
  }

  void matrix::rotateHPB(sF32 hd, sF32 pt, sF32 bk)
	{
	  sF32 sh=sin(hd), sp=sin(pt), sb=sin(bk), ch=cos(hd), cp=cos(pt), cb=cos(bk);

    identity();

	  (*this)(0,0)=ch*cb+sb*sp*sh;
	  (*this)(0,1)=ch*sb-sh*sp*cb;
	  (*this)(0,2)=sh*cp;
	  (*this)(1,0)=-sb*cp;
	  (*this)(1,1)=cp*cb;
	  (*this)(1,2)=sp;
	  (*this)(2,0)=ch*sp*sb-sh*cb;
	  (*this)(2,1)=-ch*sp*cb-sh*sb;
	  (*this)(2,2)=ch*cp;
	}

  void matrix::camera(const vector& pos, const vector& lookAt, const vector& up)
	{
		vector fwd = lookAt - pos;
	  fwd.norm();

    vector upt = up;
    upt.norm();
    if (fwd.distanceToSquared(upt)<1e-6f)
      upt.z+=1e-7f;

    vector upp = upt - (fwd * (upt * fwd));
	  upp.norm();

    vector rgt;
    rgt.cross(upp, fwd);

	  (*this)(0,0)=rgt.x;     (*this)(0,1)=upp.x;     (*this)(0,2)=fwd.x;     (*this)(0,3)=(sF32) 0.0f;
	  (*this)(1,0)=rgt.y;     (*this)(1,1)=upp.y;     (*this)(1,2)=fwd.y;     (*this)(1,3)=(sF32) 0.0f;
	  (*this)(2,0)=rgt.z;     (*this)(2,1)=upp.z;     (*this)(2,2)=fwd.z;     (*this)(2,3)=(sF32) 0.0f;
	  (*this)(3,0)=-pos*rgt;  (*this)(3,1)=-pos*upp;  (*this)(3,2)=-pos*fwd;  (*this)(3,3)=(sF32) 1.0f;
	}
}

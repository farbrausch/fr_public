/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Math.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void sVector4::SetPlane(const sVector41 &pos,const sVector40 &norm)
{
    x = norm.x;
    y = norm.y;
    z = norm.z;
    w = - sDot(pos,norm);
}

void sVector4::SetPlane(const sVector41 &p0,const sVector41 &p1,const sVector41 &p2)
{
    sVector40 norm = sNormalize(sCross(p0-p1,p0-p2));
    x = norm.x;
    y = norm.y;
    z = norm.z;
    w = - sDot(p0,norm);
}


/****************************************************************************/
/***                                                                      ***/
/***   Matrix 2x2                                                         ***/
/***                                                                      ***/
/****************************************************************************/

sMatrix22 operator* (const sMatrix22 &a,const sMatrix22 &b)
{
    sMatrix22 r;
    r.i.x = a.i.x*b.i.x + a.i.y*b.j.x;
    r.i.y = a.i.x*b.i.y + a.i.y*b.j.y;
    r.j.x = a.j.x*b.i.x + a.j.y*b.j.x;
    r.j.y = a.j.x*b.i.y + a.j.y*b.j.y;
    return r;
}

sMatrix22 sMul(const sVector2 &a,const sVector2 &b)
{
    sMatrix22 r;
    r.i.x = a.x*b.x;
    r.i.y = a.x*b.y;
    r.j.x = a.y*b.x;
    r.j.y = a.y*b.y;
    return r;
}


float sDeterminant(const sMatrix22 &m)
{
    return m.i.x*m.j.y - m.i.y*m.j.x;
}

sMatrix22 sTranspose(const sMatrix22 &m)
{
    sMatrix22 r;
    r.i.x = m.i.x;
    r.i.y = m.j.x;
    r.j.x = m.i.y;
    r.j.y = m.j.y;
    return r;
}

sMatrix22 sInvert(const sMatrix22 &m)
{
    float idet = 1.0f/sDeterminant(m);
    sMatrix22 r;
    r.i.x = ( m.j.y)*idet;
    r.i.y = (-m.i.y)*idet;
    r.j.x = (-m.j.x)*idet;
    r.j.y = ( m.i.x)*idet;
    return r;
}

sMatrix22 sEulerXYZ22(float a)
{
    float s,c;
    sSinCos(a,s,c);

    sMatrix22 r;

    r.i.x = c;
    r.i.y = -s;
    r.j.x = s;
    r.j.y = c;

    return r;
}

sMatrix22 sRotate22(float a)
{
    float s,c;
    sSinCos(a,s,c);

    sMatrix22 r;

    r.i.x = c;
    r.i.y = -s;
    r.j.x = s;
    r.j.y = c;

    return r;
}

sMatrix33A sRotate33A(float a)
{
    float s,c;
    sSinCos(a,s,c);

    sMatrix33A r;

    r.i.x = c;
    r.i.y = -s;
    r.j.x = s;
    r.j.y = c;

    return r;
}

/****************************************************************************/
/***                                                                      ***/
/***   Matrix 3x3                                                         ***/
/***                                                                      ***/
/****************************************************************************/

sMatrix33 operator* (const sMatrix33 &a,const sMatrix33 &b)
{
    sMatrix33 r;
    r.i.x = a.i.x*b.i.x + a.i.y*b.j.x + a.i.z*b.k.x;
    r.i.y = a.i.x*b.i.y + a.i.y*b.j.y + a.i.z*b.k.y;
    r.i.z = a.i.x*b.i.z + a.i.y*b.j.z + a.i.z*b.k.z;

    r.j.x = a.j.x*b.i.x + a.j.y*b.j.x + a.j.z*b.k.x;
    r.j.y = a.j.x*b.i.y + a.j.y*b.j.y + a.j.z*b.k.y;
    r.j.z = a.j.x*b.i.z + a.j.y*b.j.z + a.j.z*b.k.z;

    r.k.x = a.k.x*b.i.x + a.k.y*b.j.x + a.k.z*b.k.x;
    r.k.y = a.k.x*b.i.y + a.k.y*b.j.y + a.k.z*b.k.y;
    r.k.z = a.k.x*b.i.z + a.k.y*b.j.z + a.k.z*b.k.z;
    return r;
}

sMatrix33 sMul(const sVector3 &a,const sVector3 &b)
{
    sMatrix33 r;
    r.i.x = a.x*b.x;
    r.i.y = a.x*b.y;
    r.i.z = a.x*b.z;

    r.j.x = a.y*b.x;
    r.j.y = a.y*b.y;
    r.j.z = a.y*b.z;

    r.k.x = a.z*b.x;
    r.k.y = a.z*b.y;
    r.k.z = a.z*b.z;
    return r;
}


float sDeterminant(const sMatrix33 &m)
{
    return m.i.x*m.j.y*m.k.z + m.i.y*m.j.z*m.k.x + m.i.z*m.j.x*m.k.y
        - m.i.z*m.j.y*m.k.x - m.i.y*m.j.x*m.k.z - m.i.x*m.j.z*m.k.y;
}

sMatrix33 sTranspose(const sMatrix33 &m)
{
    sMatrix33 r;
    r.i.Set(m.i.x,m.j.x,m.k.x);
    r.j.Set(m.i.y,m.j.y,m.k.y);
    r.k.Set(m.i.z,m.j.z,m.k.z);
    return r;
}

sMatrix33 sInvert(const sMatrix33 &m)
{
    float idet = 1.0f/sDeterminant(m);

    sMatrix33 r;
    r.i.x = (m.j.y*m.k.z - m.j.z*m.k.y)*idet;
    r.i.y = (m.i.z*m.k.y - m.i.y*m.k.z)*idet;
    r.i.z = (m.i.y*m.j.z - m.i.z*m.j.y)*idet;
    r.j.x = (m.j.z*m.k.x - m.j.x*m.k.z)*idet;
    r.j.y = (m.i.x*m.k.z - m.i.z*m.k.x)*idet;
    r.j.z = (m.i.z*m.j.x - m.i.x*m.j.z)*idet;
    r.k.x = (m.j.x*m.k.y - m.j.y*m.k.x)*idet;
    r.k.y = (m.i.y*m.k.x - m.i.x*m.k.y)*idet;
    r.k.z = (m.i.x*m.j.y - m.i.y*m.j.x)*idet;
    return r;
}

sMatrix33 sEulerXYZ33(float rx,float ry,float rz)
{
    float sx,sy,sz;
    float cx,cy,cz;

    sSinCos(rx,sx,cx);
    sSinCos(ry,sy,cy);
    sSinCos(rz,sz,cz);

    sMatrix33 r;

    r.i.x = cy*cz;
    r.i.y = sx*cz*sy - cx*sz;
    r.i.z = cx*cz*sy + sx*sz;
    r.j.x = cy*sz;
    r.j.y = sx*sz*sy + cx*cz;
    r.j.z = cx*sz*sy - sx*cz;
    r.k.x = -sy;
    r.k.y = sx*cy;
    r.k.z = cx*cy;

    return r;
}

/****************************************************************************/
/***                                                                      ***/
/***   Matrix 4x4                                                         ***/
/***                                                                      ***/
/****************************************************************************/

sMatrix44 operator*(const sMatrix44 &a,const sMatrix44 &b)
{
    sMatrix44 r;

    r.i.x = a.i.x*b.i.x + a.i.y*b.j.x + a.i.z*b.k.x + a.i.w*b.l.x;
    r.i.y = a.i.x*b.i.y + a.i.y*b.j.y + a.i.z*b.k.y + a.i.w*b.l.y;
    r.i.z = a.i.x*b.i.z + a.i.y*b.j.z + a.i.z*b.k.z + a.i.w*b.l.z;
    r.i.w = a.i.x*b.i.w + a.i.y*b.j.w + a.i.z*b.k.w + a.i.w*b.l.w;

    r.j.x = a.j.x*b.i.x + a.j.y*b.j.x + a.j.z*b.k.x + a.j.w*b.l.x;
    r.j.y = a.j.x*b.i.y + a.j.y*b.j.y + a.j.z*b.k.y + a.j.w*b.l.y;
    r.j.z = a.j.x*b.i.z + a.j.y*b.j.z + a.j.z*b.k.z + a.j.w*b.l.z;
    r.j.w = a.j.x*b.i.w + a.j.y*b.j.w + a.j.z*b.k.w + a.j.w*b.l.w;

    r.k.x = a.k.x*b.i.x + a.k.y*b.j.x + a.k.z*b.k.x + a.k.w*b.l.x;
    r.k.y = a.k.x*b.i.y + a.k.y*b.j.y + a.k.z*b.k.y + a.k.w*b.l.y;
    r.k.z = a.k.x*b.i.z + a.k.y*b.j.z + a.k.z*b.k.z + a.k.w*b.l.z;
    r.k.w = a.k.x*b.i.w + a.k.y*b.j.w + a.k.z*b.k.w + a.k.w*b.l.w;

    r.l.x = a.l.x*b.i.x + a.l.y*b.j.x + a.l.z*b.k.x + a.l.w*b.l.x;
    r.l.y = a.l.x*b.i.y + a.l.y*b.j.y + a.l.z*b.k.y + a.l.w*b.l.y;
    r.l.z = a.l.x*b.i.z + a.l.y*b.j.z + a.l.z*b.k.z + a.l.w*b.l.z;
    r.l.w = a.l.x*b.i.w + a.l.y*b.j.w + a.l.z*b.k.w + a.l.w*b.l.w;

    return r;
}

sMatrix44  operator*(const sMatrix44A &a,const sMatrix44 &b)
{
    sMatrix44 r;

    r.i.x = a.i.x*b.i.x + a.i.y*b.j.x + a.i.z*b.k.x + a.i.w*b.l.x;
    r.i.y = a.i.x*b.i.y + a.i.y*b.j.y + a.i.z*b.k.y + a.i.w*b.l.y;
    r.i.z = a.i.x*b.i.z + a.i.y*b.j.z + a.i.z*b.k.z + a.i.w*b.l.z;
    r.i.w = a.i.x*b.i.w + a.i.y*b.j.w + a.i.z*b.k.w + a.i.w*b.l.w;

    r.j.x = a.j.x*b.i.x + a.j.y*b.j.x + a.j.z*b.k.x + a.j.w*b.l.x;
    r.j.y = a.j.x*b.i.y + a.j.y*b.j.y + a.j.z*b.k.y + a.j.w*b.l.y;
    r.j.z = a.j.x*b.i.z + a.j.y*b.j.z + a.j.z*b.k.z + a.j.w*b.l.z;
    r.j.w = a.j.x*b.i.w + a.j.y*b.j.w + a.j.z*b.k.w + a.j.w*b.l.w;

    r.k.x = a.k.x*b.i.x + a.k.y*b.j.x + a.k.z*b.k.x + a.k.w*b.l.x;
    r.k.y = a.k.x*b.i.y + a.k.y*b.j.y + a.k.z*b.k.y + a.k.w*b.l.y;
    r.k.z = a.k.x*b.i.z + a.k.y*b.j.z + a.k.z*b.k.z + a.k.w*b.l.z;
    r.k.w = a.k.x*b.i.w + a.k.y*b.j.w + a.k.z*b.k.w + a.k.w*b.l.w;

    r.l.x = b.l.x;
    r.l.y = b.l.y;
    r.l.z = b.l.z;
    r.l.w = b.l.w;

    return r;
}

sMatrix44 operator*(const sMatrix44 &a,const sMatrix44A &b)
{
    sMatrix44 r;

    r.i.x = a.i.x*b.i.x + a.i.y*b.j.x + a.i.z*b.k.x + a.i.w*  0  ;
    r.i.y = a.i.x*b.i.y + a.i.y*b.j.y + a.i.z*b.k.y + a.i.w*  0  ;
    r.i.z = a.i.x*b.i.z + a.i.y*b.j.z + a.i.z*b.k.z + a.i.w*  0  ;
    r.i.w = a.i.x*b.i.w + a.i.y*b.j.w + a.i.z*b.k.w + a.i.w*  1  ;

    r.j.x = a.j.x*b.i.x + a.j.y*b.j.x + a.j.z*b.k.x + a.j.w*  0  ;
    r.j.y = a.j.x*b.i.y + a.j.y*b.j.y + a.j.z*b.k.y + a.j.w*  0  ;
    r.j.z = a.j.x*b.i.z + a.j.y*b.j.z + a.j.z*b.k.z + a.j.w*  0  ;
    r.j.w = a.j.x*b.i.w + a.j.y*b.j.w + a.j.z*b.k.w + a.j.w*  1  ;

    r.k.x = a.k.x*b.i.x + a.k.y*b.j.x + a.k.z*b.k.x + a.k.w*  0  ;
    r.k.y = a.k.x*b.i.y + a.k.y*b.j.y + a.k.z*b.k.y + a.k.w*  0  ;
    r.k.z = a.k.x*b.i.z + a.k.y*b.j.z + a.k.z*b.k.z + a.k.w*  0  ;
    r.k.w = a.k.x*b.i.w + a.k.y*b.j.w + a.k.z*b.k.w + a.k.w*  1  ;

    r.l.x = a.l.x*b.i.x + a.l.y*b.j.x + a.l.z*b.k.x + a.l.w*  0  ;
    r.l.y = a.l.x*b.i.y + a.l.y*b.j.y + a.l.z*b.k.y + a.l.w*  0  ;
    r.l.z = a.l.x*b.i.z + a.l.y*b.j.z + a.l.z*b.k.z + a.l.w*  0  ;
    r.l.w = a.l.x*b.i.w + a.l.y*b.j.w + a.l.z*b.k.w + a.l.w*  1  ;

    return r;
}

sMatrix44 sMul(const sVector4 &a,const sVector4 &b)
{
    sMatrix44 r;

    r.i.x = a.x*b.x;
    r.i.y = a.x*b.y;
    r.i.z = a.x*b.z;
    r.i.w = a.x*b.w;

    r.j.x = a.y*b.x;
    r.j.y = a.y*b.y;
    r.j.z = a.y*b.z;
    r.j.w = a.y*b.w;

    r.k.x = a.z*b.x;
    r.k.y = a.z*b.y;
    r.k.z = a.z*b.z;
    r.k.w = a.z*b.w;

    r.l.x = a.w*b.x;
    r.l.y = a.w*b.y;
    r.l.z = a.w*b.z;
    r.l.w = a.w*b.w;

    return r;
}

float sDeterminant(const sMatrix44 &m)
{
    float a0 = m.i.x*m.j.y - m.i.y*m.j.x;
    float a1 = m.i.x*m.j.z - m.i.z*m.j.x;
    float a2 = m.i.x*m.j.w - m.i.w*m.j.x;
    float a3 = m.i.y*m.j.z - m.i.z*m.j.y;
    float a4 = m.i.y*m.j.w - m.i.w*m.j.y;
    float a5 = m.i.z*m.j.w - m.i.w*m.j.z;
    float b0 = m.k.x*m.l.y - m.k.y*m.l.x;
    float b1 = m.k.x*m.l.z - m.k.z*m.l.x;
    float b2 = m.k.x*m.l.w - m.k.w*m.l.x;
    float b3 = m.k.y*m.l.z - m.k.z*m.l.y;
    float b4 = m.k.y*m.l.w - m.k.w*m.l.y;
    float b5 = m.k.z*m.l.w - m.k.w*m.l.z;

    return a0*b5 - a1*b4 + a2*b3 + a3*b2 - a4*b1 + a5*b0;
}

sMatrix44 sTranspose(const sMatrix44 &m)
{
    sMatrix44 r;
    r.i.Set(m.i.x,m.j.x,m.k.x,m.l.x);
    r.j.Set(m.i.y,m.j.y,m.k.y,m.l.y);
    r.k.Set(m.i.z,m.j.z,m.k.z,m.l.z);
    r.l.Set(m.i.w,m.j.w,m.k.w,m.l.w);
    return r;
}

sMatrix44 sInvert(const sMatrix44 &m)
{
    float a0 = m.i.x*m.j.y - m.i.y*m.j.x;
    float a1 = m.i.x*m.j.z - m.i.z*m.j.x;
    float a2 = m.i.x*m.j.w - m.i.w*m.j.x;
    float a3 = m.i.y*m.j.z - m.i.z*m.j.y;
    float a4 = m.i.y*m.j.w - m.i.w*m.j.y;
    float a5 = m.i.z*m.j.w - m.i.w*m.j.z;
    float b0 = m.k.x*m.l.y - m.k.y*m.l.x;
    float b1 = m.k.x*m.l.z - m.k.z*m.l.x;
    float b2 = m.k.x*m.l.w - m.k.w*m.l.x;
    float b3 = m.k.y*m.l.z - m.k.z*m.l.y;
    float b4 = m.k.y*m.l.w - m.k.w*m.l.y;
    float b5 = m.k.z*m.l.w - m.k.w*m.l.z;

    float idet = 1.0f/(a0*b5 - a1*b4 + a2*b3 + a3*b2 - a4*b1 + a5*b0);
    sMatrix44 r;

    r.i.x = (+ m.j.y*b5 - m.j.z*b4 + m.j.w*b3);
    r.i.y = (- m.i.y*b5 + m.i.z*b4 - m.i.w*b3);
    r.i.z = (+ m.l.y*a5 - m.l.z*a4 + m.l.w*a3);
    r.i.w = (- m.k.y*a5 + m.k.z*a4 - m.k.w*a3);
    r.j.x = (- m.j.x*b5 + m.j.z*b2 - m.j.w*b1);
    r.j.y = (+ m.i.x*b5 - m.i.z*b2 + m.i.w*b1);
    r.j.z = (- m.l.x*a5 + m.l.z*a2 - m.l.w*a1);
    r.j.w = (+ m.k.x*a5 - m.k.z*a2 + m.k.w*a1);
    r.k.x = (+ m.j.x*b4 - m.j.y*b2 + m.j.w*b0);
    r.k.y = (- m.i.x*b4 + m.i.y*b2 - m.i.w*b0);
    r.k.z = (+ m.l.x*a4 - m.l.y*a2 + m.l.w*a0);
    r.k.w = (- m.k.x*a4 + m.k.y*a2 - m.k.w*a0);
    r.l.x = (- m.j.x*b3 + m.j.y*b1 - m.j.z*b0);
    r.l.y = (+ m.i.x*b3 - m.i.y*b1 + m.i.z*b0);
    r.l.z = (- m.l.x*a3 + m.l.y*a1 - m.l.z*a0);
    r.l.w = (+ m.k.x*a3 - m.k.y*a1 + m.k.z*a0);

    return r;
}

sMatrix44 sEulerXYZ44(float rx,float ry,float rz)
{
    float sx,sy,sz;
    float cx,cy,cz;

    sSinCos(rx,sx,cx);
    sSinCos(ry,sy,cy);
    sSinCos(rz,sz,cz);

    sMatrix44 r;

    r.i.x = cy*cz;
    r.i.y = sx*cz*sy - cx*sz;
    r.i.z = cx*cz*sy + sx*sz;
    r.i.w = 0;
    r.j.x = cy*sz;
    r.j.y = sx*sz*sy + cx*cz;
    r.j.z = cx*sz*sy - sx*cz;
    r.j.w = 0;
    r.k.x = -sy;
    r.k.y = sx*cy;
    r.k.z = cx*cy;
    r.k.w = 0;
    r.l.x = 0;
    r.l.y = 0;
    r.l.z = 0;
    r.l.w = 1;  

    return r;
}

void sMatrix44::SetViewportScreen()
{
    i.Set(2,0,0,-1);
    j.Set(0,-2,0,1);
    k.Set(0,0,1,0);
    l.Set(0,0,0,1);
}

void sMatrix44::SetViewportPixels(int sx,int sy)
{
    i.Set(2.0f/sx,0,0,-1);
    j.Set(0,-2.0f/sy,0,1);
    k.Set(0,0,1,0);
    l.Set(0,0,0,1);
}

/****************************************************************************/

void sMatrix44A::Orthonormalize()
{
    sVector3 _i(i.x,j.x,k.x);
    sVector3 _j(i.y,j.y,k.y);
    sVector3 _k(i.z,j.z,k.z);

    _i = sNormalize(_i);
    _j = _j - (sDot(_i,_j) * _i);
    _j = sNormalize(_j);
    _k = _k - (sDot(_i,_k) * _i + sDot(_j,_k)*_j);
    _k = sNormalize(_k);

    i.Set(_i.x,_j.x,_k.x,i.w);
    j.Set(_i.y,_j.y,_k.y,j.w);
    k.Set(_i.z,_j.z,_k.z,k.w);
}

void sMatrix44A::Init(sQuaternion &q)
{
  float xx = 2.0f*q.i*q.i;
  float xy = 2.0f*q.i*q.j;
  float xz = 2.0f*q.i*q.k;

  float yy = 2.0f*q.j*q.j;
  float yz = 2.0f*q.j*q.k;
  float zz = 2.0f*q.k*q.k;

  float xw = 2.0f*q.i*q.r;
  float yw = 2.0f*q.j*q.r;
  float zw = 2.0f*q.k*q.r;

  i.x = 1.0f - (yy + zz);
  i.y =        (xy + zw);
  i.z =        (xz - yw);

  j.x =        (xy - zw);
  j.y = 1.0f - (xx + zz);
  j.z =        (yz + xw);

  k.x =        (xz + yw);
  k.y =        (yz - xw);
  k.z = 1.0f - (xx + yy);

  //Clear Translation
  i.w = 0.0f;
  j.w = 0.0f;
  k.w = 0.0f;
}


sMatrix44A operator*(const sMatrix44A &a,const sMatrix44A &b)
{
    sMatrix44A r;

    r.i.x = a.i.x*b.i.x + a.i.y*b.j.x + a.i.z*b.k.x        ;
    r.i.y = a.i.x*b.i.y + a.i.y*b.j.y + a.i.z*b.k.y        ;
    r.i.z = a.i.x*b.i.z + a.i.y*b.j.z + a.i.z*b.k.z        ;
    r.i.w = a.i.x*b.i.w + a.i.y*b.j.w + a.i.z*b.k.w + a.i.w;

    r.j.x = a.j.x*b.i.x + a.j.y*b.j.x + a.j.z*b.k.x        ;
    r.j.y = a.j.x*b.i.y + a.j.y*b.j.y + a.j.z*b.k.y        ;
    r.j.z = a.j.x*b.i.z + a.j.y*b.j.z + a.j.z*b.k.z        ;
    r.j.w = a.j.x*b.i.w + a.j.y*b.j.w + a.j.z*b.k.w + a.j.w;

    r.k.x = a.k.x*b.i.x + a.k.y*b.j.x + a.k.z*b.k.x        ;
    r.k.y = a.k.x*b.i.y + a.k.y*b.j.y + a.k.z*b.k.y        ;
    r.k.z = a.k.x*b.i.z + a.k.y*b.j.z + a.k.z*b.k.z        ;
    r.k.w = a.k.x*b.i.w + a.k.y*b.j.w + a.k.z*b.k.w + a.k.w;

    return r;
}

float sDeterminant(const sMatrix44A &m)
{
    return m.i.w*m.j.z*m.k.y* 0  - m.i.z*m.j.w*m.k.y* 0  - m.i.w*m.j.y*m.k.z* 0  + m.i.y*m.j.w*m.k.z* 0 
        + m.i.z*m.j.y*m.k.w* 0  - m.i.y*m.j.z*m.k.w* 0  - m.i.w*m.j.z*m.k.x* 0  + m.i.z*m.j.w*m.k.x* 0 
        + m.i.w*m.j.x*m.k.z* 0  - m.i.x*m.j.w*m.k.z* 0  - m.i.z*m.j.x*m.k.w* 0  + m.i.x*m.j.z*m.k.w* 0 
        + m.i.w*m.j.y*m.k.x* 0  - m.i.y*m.j.w*m.k.x* 0  - m.i.w*m.j.x*m.k.y* 0  + m.i.x*m.j.w*m.k.y* 0 
        + m.i.y*m.j.x*m.k.w* 0  - m.i.x*m.j.y*m.k.w* 0  - m.i.z*m.j.y*m.k.x* 1  + m.i.y*m.j.z*m.k.x* 1 
        + m.i.z*m.j.x*m.k.y* 1  - m.i.x*m.j.z*m.k.y* 1  - m.i.y*m.j.x*m.k.z* 1  + m.i.x*m.j.y*m.k.z* 1 ;
}

sMatrix44A sInvert(const sMatrix44A &m)
{
    float idet = 1.0f/sDeterminant(m);
    sMatrix44A r;
    r.i.x = (                                                                              - m.j.z*m.k.y       + m.j.y*m.k.z      )*idet;
    r.i.y = (                                                                              + m.i.z*m.k.y       - m.i.y*m.k.z      )*idet;
    r.i.z = (                                                                              - m.i.z*m.j.y       + m.i.y*m.j.z      )*idet;
    r.i.w = (m.i.w*m.j.z*m.k.y - m.i.z*m.j.w*m.k.y - m.i.w*m.j.y*m.k.z + m.i.y*m.j.w*m.k.z + m.i.z*m.j.y*m.k.w - m.i.y*m.j.z*m.k.w)*idet;
    r.j.x = (                                                                              + m.j.z*m.k.x       - m.j.x*m.k.z      )*idet;
    r.j.y = (                                                                              - m.i.z*m.k.x       + m.i.x*m.k.z      )*idet;
    r.j.z = (                                                                              + m.i.z*m.j.x       - m.i.x*m.j.z      )*idet;
    r.j.w = (m.i.z*m.j.w*m.k.x - m.i.w*m.j.z*m.k.x + m.i.w*m.j.x*m.k.z - m.i.x*m.j.w*m.k.z - m.i.z*m.j.x*m.k.w + m.i.x*m.j.z*m.k.w)*idet;
    r.k.x = (                                                                              - m.j.y*m.k.x       + m.j.x*m.k.y      )*idet;
    r.k.y = (                                                                              + m.i.y*m.k.x       - m.i.x*m.k.y      )*idet;
    r.k.z = (                                                                              - m.i.y*m.j.x       + m.i.x*m.j.y      )*idet;
    r.k.w = (m.i.w*m.j.y*m.k.x - m.i.y*m.j.w*m.k.x - m.i.w*m.j.x*m.k.y + m.i.x*m.j.w*m.k.y + m.i.y*m.j.x*m.k.w - m.i.x*m.j.y*m.k.w)*idet;

    // l.xyz is trivially zero
    // l.w is one, because it turns out to be det(m)/det(m).

    return r;
}

sMatrix44A sEulerXYZ(float rx,float ry,float rz)
{
    float sx,sy,sz;
    float cx,cy,cz;

    sSinCos(rx,sx,cx);
    sSinCos(ry,sy,cy);
    sSinCos(rz,sz,cz);

    sMatrix44A r;

    r.i.x = cy*cz;
    r.i.y = sx*cz*sy - cx*sz;
    r.i.z = cx*cz*sy + sx*sz;
    r.i.w = 0;
    r.j.x = cy*sz;
    r.j.y = sx*sz*sy + cx*cz;
    r.j.z = cx*sz*sy - sx*cz;
    r.j.w = 0;
    r.k.x = -sy;
    r.k.y = sx*cy;
    r.k.z = cx*cy;
    r.k.w = 0;

    return r;
}

sVector3 sFindEulerXYZ(const sMatrix44A &m)
{
    float rx,ry,rz;
    float sy = -m.k.x;

    if(sy >= 0.99999f) // ry very close to 90° (singular)
    {
        ry = sPi / 2.0f;
        rx = sATan2(m.i.y,m.j.y);
        rz = 0.0f;
    }
    else if(sy <= -0.99999f) // ry very close to -90° (singular)
    {
        ry = -sPi / 2.0f;
        rx = sATan2(-m.i.y,-m.i.z);
        rz = 0.0f;
    }
    else
    {
        float rzp = sATan2(m.j.x,m.i.x);
        float rzn = sATan2(-m.j.x,-m.i.x);
        float cy = sSqrt(m.k.y*m.k.y + m.k.z*m.k.z);    // evil square root that has two possible outcomes!

        if(sAbs(rzp)<sAbs(rzn))
        {
            rx = sATan2(m.k.y,m.k.z);
            ry = sATan2(sy,cy);
            rz = rzp;
        }
        else
        {
            rx = sATan2(-m.k.y,-m.k.z);
            ry = sATan2(sy,-cy);
            rz = rzn;
        }
    }

    return sVector3(rx,ry,rz);
}

sMatrix44A sSetSRT(const sVector41 &scale,const sVector3 &rot,const sVector41 &trans)
{
    sMatrix44A r = sEulerXYZ(rot);
    r.i*=scale;
    r.j*=scale;
    r.k*=scale;
    r.SetTrans(trans);
    return r;
}

sMatrix44A sRotateAxis(const sVector40 &v,float a)
{
    sVector40 u;
    float as,ac;

    u = sNormalize(v);

    ac = float(sCos(a));
    as = float(sSin(a));

    sMatrix44A r;

    r.i.x = (1-u.x*u.x)*ac + u.x*u.x + 0;
    r.i.y = ( -u.y*u.x)*ac + u.y*u.x + u.z*as;
    r.i.z = ( -u.z*u.x)*ac + u.z*u.x - u.y*as;
    r.i.w = 0;

    r.j.x = ( -u.x*u.y)*ac + u.x*u.y - u.z*as;
    r.j.y = (1-u.y*u.y)*ac + u.y*u.y + 0;
    r.j.z = ( -u.z*u.y)*ac + u.z*u.y + u.x*as;
    r.j.w = 0;

    r.k.x = ( -u.x*u.z)*ac + u.x*u.z + u.y*as;
    r.k.y = ( -u.y*u.z)*ac + u.y*u.z - u.x*as;
    r.k.z = (1-u.z*u.z)*ac + u.z*u.z + 0;
    r.k.w = 0;

    return r;
}

sMatrix44A sLook(const sVector40 &v)
{
    sVector40 i,j,k;

    j.Set(0,1,0);
    k = sNormalize(v);
    i = sNormalize(sCross(j,k));
    j = sNormalize(sCross(k,i));

    sMatrix44A r;

    r.SetBaseX(i);
    r.SetBaseY(j);
    r.SetBaseZ(k);
    r.SetTrans(sVector41(0,0,0));

    return r;
}

sMatrix44A sLook(const sVector40 &dir,const sVector40 &up)
{
    sVector40 i,j,k;

    k = sNormalize(dir);
    i = sNormalize(sCross(up,k));
    j = sNormalize(sCross(k,i));

    sMatrix44A r;

    r.SetBaseX(i);
    r.SetBaseY(j);
    r.SetBaseZ(k);
    r.SetTrans(sVector41(0,0,0));

    return r;
}

sMatrix44A sLookAt(const sVector41 &dest,const sVector41 &pos)
{
    sVector40 dir = sNormalize(dest-pos);

    sMatrix44A r = sLook(dir);
    r.SetTrans(pos);

    return r;
}

sMatrix44A sLookAt(const sVector41 &dest,const sVector41 &pos,const sVector40 &up)
{
    sVector40 dir = sNormalize(dest-pos);

    sMatrix44A r = sLook(dir,up);
    r.SetTrans(pos);

    return r;
}


sVector2 sSolve(const sMatrix22 &m,const sVector2 &a)
{
    float d = 1.0f/sDeterminant(m);
    sVector2 r;
    sMatrix22 n;

    n = m; n.i = a; r.x = sDeterminant(n)*d;
    n = m; n.j = a; r.y = sDeterminant(n)*d;

    return r;
}

sVector3 sSolve(const sMatrix33 &m,const sVector3 &a)
{
    float d = 1.0f/sDeterminant(m);
    sVector3 r;
    sMatrix33 n;

    n = m; n.i = a; r.x = sDeterminant(n)*d;
    n = m; n.j = a; r.y = sDeterminant(n)*d;
    n = m; n.k = a; r.z = sDeterminant(n)*d;

    return r;
}

sVector4 sSolve(const sMatrix44 &m,const sVector4 &a)
{
    float d = 1.0f/sDeterminant(m);
    sVector4 r;
    sMatrix44 n;

    n = m; n.i = a; r.x = sDeterminant(n)*d;
    n = m; n.j = a; r.y = sDeterminant(n)*d;
    n = m; n.k = a; r.z = sDeterminant(n)*d;
    n = m; n.l = a; r.w = sDeterminant(n)*d;

    return r;
}

/****************************************************************************/
/***                                                                      ***/
/***   Intersection Tests                                                 ***/
/***                                                                      ***/
/****************************************************************************/

bool sIntersect(const sVector2 &a0,const sVector2 &a1,const sVector2 &b0,const sVector2 &b1,sVector2 &x,float *af,float *bf)
{
    return sIntersectLineSegment(a0,a1,b0,b1,x,af,bf);
}

bool sIntersectLineSegment(const sVector2 &a0,const sVector2 &a1,const sVector2 &b0,const sVector2 &b1,sVector2 &x,float *af,float *bf)
{
    sMatrix22 m;
    m.i = a1-a0;
    m.j = b0-b1;
    sVector2 a = b0-a0;
    sVector2 q = sSolve(m,a);
    if(af) *af = q.x;
    if(bf) *bf = q.y;
    if(q.x>=0 && q.x<=1 && q.y>=0 && q.y<=1)
    {
        x = a0+(a1-a0)*q.x;
        return 1;
    }
    return 0;
}

bool sIntersectLine(const sVector2 &a0,const sVector2 &a1,const sVector2 &b0,const sVector2 &b1,sVector2 &x,float *af,float *bf)
{
    sMatrix22 m;
    m.i = a1-a0;
    m.j = b0-b1;
    sVector2 a = b0-a0;
    sVector2 q = sSolve(m,a);
    if(af) *af = q.x;
    if(bf) *bf = q.y;
    x = a0+(a1-a0)*q.x;
    return 1;
}

/****************************************************************************/
/***                                                                      ***/
/***   Quaternions                                                        ***/
/***                                                                      ***/
/****************************************************************************/

void sQuaternion::Lerp(float fade,sQuaternion &a,sQuaternion &b)
{
    float dot = a.r*b.r + a.i*b.i + a.j*b.j + a.k*b.k;
    if(dot<0.f)
    {
        r = -a.r + (b.r+a.r)*fade;
        i = -a.i + (b.i+a.i)*fade;
        j = -a.j + (b.j+a.j)*fade;
        k = -a.k + (b.k+a.k)*fade;
    }
    else
    {
        r = a.r + (b.r-a.r)*fade;
        i = a.i + (b.i-a.i)*fade;
        j = a.j + (b.j-a.j)*fade;
        k = a.k + (b.k-a.k)*fade;
    }
}

void sQuaternion::Init(const sMatrix44A &mat)
{
    float tr,s;

//    tr = mat.i.x + mat.j.y + mat.k.z;
    tr = mat.i.x + mat.j.y + mat.k.z;

    if(tr>=0) 
    {
        s = sSqrt(tr + 1);
        r = s*0.5f;
        s = 0.5f / s;

        //i = (mat.k.y - mat.j.z) * s;
        //j = (mat.i.z - mat.k.x) * s;
        //k = (mat.j.x - mat.i.y) * s;

        i = (mat.j.z - mat.k.y) * s;
        j = (mat.k.x - mat.i.z) * s;
        k = (mat.i.y - mat.j.x) * s;
    }
    else 
    {
        int index;
        //if (mat.j.y > mat.i.x)
        if (mat.j.y > mat.i.x)
        {      
            //if (mat.k.z > mat.j.y) index=2; else index=1;
            if (mat.k.z > mat.j.y) index=2; else index=1;
        }
        else
        {      
            //if (mat.k.z > mat.i.x) index=2; else index=0;
            if (mat.k.z > mat.i.x) index=2; else index=0;
        }

        switch(index)
        {
        case 0:
            //s = sSqrt((mat.i.x - (mat.j.y+mat.k.z)) + 1.0f );
            s = sSqrt((mat.i.x - (mat.j.y+mat.k.z)) + 1.0f );
            i = s*0.5f;
            s = 0.5f / s;
            //j = (mat.i.y + mat.j.x) * s;
            //k = (mat.k.x + mat.i.z) * s;
            //r = (mat.k.y - mat.j.z) * s;

            j = (mat.i.y + mat.j.x) * s;
            k = (mat.k.x + mat.i.z) * s;
            r = (mat.j.z - mat.k.y) * s;
            break;
        case 1:
            s = sSqrt( (mat.j.y - (mat.k.z+mat.i.x)) + 1.0f );
            j = s*0.5f;
            s = 0.5f / s;

            //k = (mat.j.z + mat.k.y) * s;
            //i = (mat.i.y + mat.j.x) * s;
            //r = (mat.i.z - mat.k.x) * s;

            k = (mat.j.z + mat.k.y) * s;
            i = (mat.i.y + mat.j.x) * s;
            r = (mat.k.x - mat.i.z) * s;
            break;
        case 2:
            s = sSqrt( (mat.k.z - (mat.i.x+mat.j.y)) + 1.0f );
            k = s*0.5f;
            s = 0.5f / s;

            //i = (mat.k.x + mat.i.z) * s;
            //j = (mat.j.z + mat.k.y) * s;
            //r = (mat.j.x - mat.i.y) * s;

            i = (mat.k.x + mat.i.z) * s;
            j = (mat.j.z + mat.k.y) * s;
            r = (mat.i.y - mat.j.x) * s;
            break;
        }
    }
}


/****************************************************************************/
/***                                                                      ***/
/***   Axis Aligned Bounding Box                                          ***/
/***                                                                      ***/
/****************************************************************************/

void sAABBox41::GetCorners(sVector41 *vp) const
{
    vp[0] = sVector41(Min.x,Min.y,Min.z);
    vp[1] = sVector41(Min.x,Min.y,Max.z);
    vp[2] = sVector41(Min.x,Max.y,Min.z);
    vp[3] = sVector41(Min.x,Max.y,Max.z);
    vp[4] = sVector41(Max.x,Min.y,Min.z);
    vp[5] = sVector41(Max.x,Min.y,Max.z);
    vp[6] = sVector41(Max.x,Max.y,Min.z);
    vp[7] = sVector41(Max.x,Max.y,Max.z);
}

void sAABBox41::Add(const sMatrix44A &mat,const sAABBox41 &box)
{
    sVector41 p[8];

    box.GetCorners(p);
    for(int i=0;i<8;i++)
        Add(mat*p[i]);
}

void sAABBox41::Add(const sMatrix44A &mat,const sAABBox41C &box)
{
    sVector41 p[8];

    box.GetCorners(p);
    for(int i=0;i<8;i++)
        Add(mat*p[i]);
}

void sAABBox41C::GetCorners(sVector41 *vp) const
{
    vp[0] = sVector41(Center.x-Radius.x,Center.y-Radius.y,Center.z-Radius.z);
    vp[1] = sVector41(Center.x-Radius.x,Center.y-Radius.y,Center.z+Radius.z);
    vp[2] = sVector41(Center.x-Radius.x,Center.y+Radius.y,Center.z-Radius.z);
    vp[3] = sVector41(Center.x-Radius.x,Center.y+Radius.y,Center.z+Radius.z);
    vp[4] = sVector41(Center.x+Radius.x,Center.y-Radius.y,Center.z-Radius.z);
    vp[5] = sVector41(Center.x+Radius.x,Center.y-Radius.y,Center.z+Radius.z);
    vp[6] = sVector41(Center.x+Radius.x,Center.y+Radius.y,Center.z-Radius.z);
    vp[7] = sVector41(Center.x+Radius.x,Center.y+Radius.y,Center.z+Radius.z);
}

/****************************************************************************/

void sFrustum::Init(const sMatrix44 &mat,float xMin,float xMax,float yMin,float yMax,float zMin,float zMax)
{
    Planes[0] = mat.i - xMin*mat.l;     // sFP_Left
    Planes[1] = xMax*mat.l - mat.i;     // sFP_Right
    Planes[2] = mat.j - yMin*mat.l;     // sFP_Bottom
    Planes[3] = yMax*mat.l - mat.j;     // sFP_Top
    Planes[4] = mat.k - zMin*mat.l;     // sFP_Near
    Planes[5] = zMax*mat.l - mat.k;     // sFP_Far

    for(int i=0;i<6;i++)
    {
        AbsPlanes[i].x = sAbs(Planes[i].x);
        AbsPlanes[i].y = sAbs(Planes[i].y);
        AbsPlanes[i].z = sAbs(Planes[i].z);
        AbsPlanes[i].w =      Planes[i].w ;
    }
}

sClipTest sFrustum::IsInside(const sAABBox41C &b) const
{
    sClipTest result = sCT_In;
    for(int i=0;i<6;i++)
    {
        float m = sDot(b.Center , Planes[i]);
        float n = sDot(b.Radius , AbsPlanes[i]);
        if(m+n<0) return sCT_Out;
        if(m-n<0) result = sCT_Partial;
    }
    return result;
}


void sFrustum::Transform(const sFrustum &src,const sMatrix44A &matx)
{
    sMatrix44 mat = sTranspose(sMatrix44(matx));
    for(int i=0;i<6;i++)
    {
        Planes[i] = mat * src.Planes[i];
        AbsPlanes[i].x = sAbs(Planes[i].x);
        AbsPlanes[i].y = sAbs(Planes[i].y);
        AbsPlanes[i].z = sAbs(Planes[i].z);
        AbsPlanes[i].w =      Planes[i].w ;
    }
}

/****************************************************************************/
/***                                                                      ***/
/***   Bezier Curves                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void sBezier2::Divide(sBezier2 &s0,sBezier2 &s1,float t0) const
{
    float t1 = 1.0f-t0;
    sVector2 i0 = c0*t0 + c1*t1;
    sVector2 i1 = c1*t0 + c2*t1;
    sVector2 i2 = c2*t0 + c3*t1;
    sVector2 j0 = i0*t0 + i1*t1;
    sVector2 j1 = i1*t0 + i2*t1;
    sVector2 k0 = j0*t0 + j1*t1;
    s0.c0 = c0;
    s0.c1 = i0;
    s0.c2 = j0;
    s0.c3 = k0;
    s1.c0 = k0;
    s1.c1 = j1;
    s1.c2 = i2;
    s1.c3 = c3;
}

void sBezier2::Divide(sBezier2 &s0,sBezier2 &s1) const
{
    sVector2 i0 = (c0+c1) * 0.5f;
    sVector2 i1 = (c1+c2) * 0.5f;
    sVector2 i2 = (c2+c3) * 0.5f;
    sVector2 j0 = (i0+i1) * 0.5f;
    sVector2 j1 = (i1+i2) * 0.5f;
    sVector2 k0 = (j0+j1) * 0.5f;
    s0.c0 = c0;
    s0.c1 = i0;
    s0.c2 = j0;
    s0.c3 = k0;
    s1.c0 = k0;
    s1.c1 = j1;
    s1.c2 = i2;
    s1.c3 = c3;
}

bool sBezier2::Divide(float t)
{
    sVector2 i0 = c0*0.5f + c1*0.5f;
    sVector2 i1 = c1*0.5f + c2*0.5f;
    sVector2 i2 = c2*0.5f + c3*0.5f;
    sVector2 j0 = i0*0.5f + i1*0.5f;
    sVector2 j1 = i1*0.5f + i2*0.5f;
    sVector2 k0 = j0*0.5f + j1*0.5f;
    if(t < k0.x)
    {
        c0 = c0;
        c1 = i0;
        c2 = j0;
        c3 = k0;
        return 0;
    }
    else
    {
        c0 = k0;
        c1 = j1;
        c2 = i2;
        c3 = c3;
        return 1;
    }
}

float sBezier2::FindX(float x)
{
    sBezier2 b = *this;
    float t0 = 0;
    float t1 = 1;
    for(int i=0;i<15;i++)
    {
        if(b.Divide(x))
            t1 = t0*0.5f + t1*0.5f;
        else
            t0 = t0*0.5f + t1*0.5f;
    }
    sASSERT(x>=b.c0.x && x<=b.c3.x);
    return sLerp(t0,t1,(x-b.c0.x)/(b.c3.x-b.c0.x));
}

bool sBezier2::IsLine(float abserrorsq) const
{
    float dot = sDot(c0-c3,c0-c3);

    if(sCross(c0-c3,c0-c1) > abserrorsq*dot)
        return false;

    if(sCross(c0-c3,c0-c2) > abserrorsq*dot)
        return false;

    return true;
}

/****************************************************************************/
/***                                                                      ***/
/***   Viewport                                                           ***/
/***                                                                      ***/
/****************************************************************************/

sViewport::sViewport()
{
    Model.Identity();
    Camera.Identity();

    ZNear = 0.125f;
    ZFar = 2048.0f;
    ZoomX = 1;
    ZoomY = 1;
    Mode = sVM_Perspective;

    MS2CS.Identity();
    MS2SS.Identity();
}

void sViewport::Prepare(const Altona2::sTargetPara &tp)
{
    Prepare(tp.Window.SizeX(),tp.Window.SizeY());
}

void sViewport::Prepare(int sx,int sy)
{  
    float q=0;
    switch(Mode)
    {
    case sVM_Perspective:
        q = 1/(ZFar-ZNear);

        Projection.i.x = ZoomX;
        Projection.i.y = 0;
        Projection.i.z = 0;
        Projection.i.w = 0;

        Projection.j.x = 0;
        Projection.j.y = ZoomY;
        Projection.j.z = 0;
        Projection.j.w = 0;

        Projection.k.x = 0;
        Projection.k.y = 0;
        Projection.k.z = q*ZFar;
        Projection.k.w = -q*ZFar*ZNear;

        Projection.l.x = 0;
        Projection.l.y = 0;
        Projection.l.z = 1;
        Projection.l.w = 0;
        break;

    case sVM_Orthogonal:
        q = 1/(ZFar-ZNear);

        Projection.i.x = ZoomX;
        Projection.i.y = 0;
        Projection.i.z = 0;
        Projection.i.w = 0;

        Projection.j.x = 0;
        Projection.j.y = ZoomY;
        Projection.j.z = 0;
        Projection.j.w = 0;

        Projection.k.x = 0;
        Projection.k.y = 0;
        Projection.k.z = q;
        Projection.k.w = -q*ZNear;

        Projection.l.x = 0;
        Projection.l.y = 0;
        Projection.l.z = 0;
        Projection.l.w = 1;
        break;

    case sVM_Pixels:
        Projection.i.x = 2.0f/sx*ZoomX;
        Projection.i.y = 0;
        Projection.i.z = 0;
        Projection.i.w = -1;

        Projection.j.x = 0;
        Projection.j.y = -2.0f/sy*ZoomY;
        Projection.j.z = 0;
        Projection.j.w = 1;

        Projection.k.x = 0;
        Projection.k.y = 0;
        Projection.k.z = 1;
        Projection.k.w = 0;

        Projection.l.x = 0;
        Projection.l.y = 0;
        Projection.l.z = 0;
        Projection.l.w = 1;
        break;

    default:
        sASSERT0();
        break;  
    }

    if(sConfigRender==sConfigRenderGL2 || sConfigRender==sConfigRenderGLES2)
    {
        Projection.k.z = 2*Projection.k.z - 1;
        Projection.k.w = 2*Projection.k.w;
    }


    WS2CS = sInvert(Camera);

    MS2CS = WS2CS * Model;
    MS2SS = Projection * MS2CS;
}


/****************************************************************************/
/***                                                                      ***/
/***   Perlin Noise                                                       ***/
/***                                                                      ***/
/****************************************************************************/

float sPerlinRandom2D[256][2];
float sPerlinRandom3D[256][3];
int sPerlinPermute[512];

static void sInitPerlin()
{
    int i,j;
    sRandom rnd;
    int order[256];

    rnd.Seed(0);
    // permutation
    for(i=0;i<256;i++)
    {
        order[i]=rnd.Int16();
        sPerlinPermute[i]=i;
    }

    for(i=0;i<255;i++)
    {
        for(j=i+1;j<256;j++)
        {
            if(order[i]>order[j])
            {
                sSwap(order[i],order[j]);
                sSwap(sPerlinPermute[i],sPerlinPermute[j]);
            }
        }
    }
    for(int k=0;k<256;k++)
        sPerlinPermute[k+256] = sPerlinPermute[k];

    // random 2d
    for(i=0;i<256;)
    {
        float x,y;
        x = rnd.Float(2.0f)-1.0f;
        y = rnd.Float(2.0f)-1.0f;
        if(x*x+y*y<1.0f)
        {
            sPerlinRandom2D[i][0] = x;
            sPerlinRandom2D[i][1] = y;
            i++;
        }
    }

    // random 3d
    for(i=0;i<256;)
    {
        float x,y,z;
        x = rnd.Float(2.0f)-1.0f;
        y = rnd.Float(2.0f)-1.0f;
        z = rnd.Float(2.0f)-1.0f;
        if(x*x+y*y+z*z<1.0f)
        {
            sPerlinRandom3D[i][0] = x;
            sPerlinRandom3D[i][1] = y;
            sPerlinRandom3D[i][2] = z;
            i++;
        }
    }

}

float sPerlin2D(int x,int y,int mask,int seed)
{
    int vx0,vy0,vx1,vy1;
    int v00,v01,v10,v11;
    float f00,f01,f10,f11;
    float f0,f1,f;
    float tx,ty;

    mask &= 255;
    vx0 = (x>>16) & mask; vx1 = (vx0+1) & mask; tx=(x&0xffff)/65536.0f;
    vy0 = (y>>16) & mask; vy1 = (vy0+1) & mask; ty=(y&0xffff)/65536.0f;
    v00 = sPerlinPermute[((vx0))+sPerlinPermute[((vy0))^seed]];
    v01 = sPerlinPermute[((vx1))+sPerlinPermute[((vy0))^seed]];
    v10 = sPerlinPermute[((vx0))+sPerlinPermute[((vy1))^seed]];
    v11 = sPerlinPermute[((vx1))+sPerlinPermute[((vy1))^seed]];
    f00 = sPerlinRandom2D[v00][0]*(tx-0)+sPerlinRandom2D[v00][1]*(ty-0);
    f01 = sPerlinRandom2D[v01][0]*(tx-1)+sPerlinRandom2D[v01][1]*(ty-0);
    f10 = sPerlinRandom2D[v10][0]*(tx-0)+sPerlinRandom2D[v10][1]*(ty-1);
    f11 = sPerlinRandom2D[v11][0]*(tx-1)+sPerlinRandom2D[v11][1]*(ty-1);
    tx = tx*tx*tx*(10+tx*(6*tx-15));
    ty = ty*ty*ty*(10+ty*(6*ty-15));

    f0 = f00+(f01-f00)*tx;
    f1 = f10+(f11-f10)*tx;
    f = f0+(f1-f0)*ty;

    return f;
}

float sPerlin3D(int x,int y,int z,int mask,int seed)
{
    int vx0,vy0,vz0,vx1,vy1,vz1;
    int v000,v001,v010,v011;
    int v100,v101,v110,v111;
    float f000,f001,f010,f011;
    float f100,f101,f110,f111;
    float f00,f01,f10,f11;
    float f0,f1,f;
    float tx,ty,tz;

    mask &= 255;
    vx0 = (x>>16) & mask; vx1 = (vx0+1) & mask; tx=(x&0xffff)/65536.0f;
    vy0 = (y>>16) & mask; vy1 = (vy0+1) & mask; ty=(y&0xffff)/65536.0f;
    vz0 = (z>>16) & mask; vz1 = (vz0+1) & mask; tz=(z&0xffff)/65536.0f;
    v000 = sPerlinPermute[vx0+sPerlinPermute[vy0+sPerlinPermute[vz0^seed]]];
    v001 = sPerlinPermute[vx1+sPerlinPermute[vy0+sPerlinPermute[vz0^seed]]];
    v010 = sPerlinPermute[vx0+sPerlinPermute[vy1+sPerlinPermute[vz0^seed]]];
    v011 = sPerlinPermute[vx1+sPerlinPermute[vy1+sPerlinPermute[vz0^seed]]];
    v100 = sPerlinPermute[vx0+sPerlinPermute[vy0+sPerlinPermute[vz1^seed]]];
    v101 = sPerlinPermute[vx1+sPerlinPermute[vy0+sPerlinPermute[vz1^seed]]];
    v110 = sPerlinPermute[vx0+sPerlinPermute[vy1+sPerlinPermute[vz1^seed]]];
    v111 = sPerlinPermute[vx1+sPerlinPermute[vy1+sPerlinPermute[vz1^seed]]];

    f000 = sPerlinRandom3D[v000][0]*(tx-0)+sPerlinRandom3D[v000][1]*(ty-0)+sPerlinRandom3D[v000][2]*(tz-0);
    f001 = sPerlinRandom3D[v001][0]*(tx-1)+sPerlinRandom3D[v001][1]*(ty-0)+sPerlinRandom3D[v001][2]*(tz-0);
    f010 = sPerlinRandom3D[v010][0]*(tx-0)+sPerlinRandom3D[v010][1]*(ty-1)+sPerlinRandom3D[v010][2]*(tz-0);
    f011 = sPerlinRandom3D[v011][0]*(tx-1)+sPerlinRandom3D[v011][1]*(ty-1)+sPerlinRandom3D[v011][2]*(tz-0);
    f100 = sPerlinRandom3D[v100][0]*(tx-0)+sPerlinRandom3D[v100][1]*(ty-0)+sPerlinRandom3D[v100][2]*(tz-1);
    f101 = sPerlinRandom3D[v101][0]*(tx-1)+sPerlinRandom3D[v101][1]*(ty-0)+sPerlinRandom3D[v101][2]*(tz-1);
    f110 = sPerlinRandom3D[v110][0]*(tx-0)+sPerlinRandom3D[v110][1]*(ty-1)+sPerlinRandom3D[v110][2]*(tz-1);
    f111 = sPerlinRandom3D[v111][0]*(tx-1)+sPerlinRandom3D[v111][1]*(ty-1)+sPerlinRandom3D[v111][2]*(tz-1);
    tx = tx*tx*tx*(10+tx*(6*tx-15));
    ty = ty*ty*ty*(10+ty*(6*ty-15));
    tz = tz*tz*tz*(10+tz*(6*tz-15));

    f00 = f000+(f001-f000)*tx;
    f01 = f010+(f011-f010)*tx;
    f10 = f100+(f101-f100)*tx;
    f11 = f110+(f111-f110)*tx;

    f0 = f00+(f01-f00)*ty;
    f1 = f10+(f11-f10)*ty;

    f = f0+(f1-f0)*tz;

    return f;
}


void sPerlinDerive3D(int x,int y,int z,int mask,int seed,float &value,sVector40 &dir)
{
    int vx0,vy0,vz0,vx1,vy1,vz1;
    int v000,v001,v010,v011;
    int v100,v101,v110,v111;
    float tx,ty,tz;
    float px000,px001,px010,px011,px100,px101,px110,px111;
    float py000,py001,py010,py011,py100,py101,py110,py111;
    float pz000,pz001,pz010,pz011,pz100,pz101,pz110,pz111;
    float rx000,rx001,rx010,rx011,rx100,rx101,rx110,rx111;
    float ry000,ry001,ry010,ry011,ry100,ry101,ry110,ry111;
    float rz000,rz001,rz010,rz011,rz100,rz101,rz110,rz111;
    float ra0,ra1,rb0,rb1;

    mask &= 255;
    vx0 = (x>>16) & mask; vx1 = (vx0+1) & mask; tx=(x&0xffff)/65536.0f;
    vy0 = (y>>16) & mask; vy1 = (vy0+1) & mask; ty=(y&0xffff)/65536.0f;
    vz0 = (z>>16) & mask; vz1 = (vz0+1) & mask; tz=(z&0xffff)/65536.0f;
    v000 = sPerlinPermute[vx0+sPerlinPermute[vy0+sPerlinPermute[vz0^seed]]];
    v001 = sPerlinPermute[vx1+sPerlinPermute[vy0+sPerlinPermute[vz0^seed]]];
    v010 = sPerlinPermute[vx0+sPerlinPermute[vy1+sPerlinPermute[vz0^seed]]];
    v011 = sPerlinPermute[vx1+sPerlinPermute[vy1+sPerlinPermute[vz0^seed]]];
    v100 = sPerlinPermute[vx0+sPerlinPermute[vy0+sPerlinPermute[vz1^seed]]];
    v101 = sPerlinPermute[vx1+sPerlinPermute[vy0+sPerlinPermute[vz1^seed]]];
    v110 = sPerlinPermute[vx0+sPerlinPermute[vy1+sPerlinPermute[vz1^seed]]];
    v111 = sPerlinPermute[vx1+sPerlinPermute[vy1+sPerlinPermute[vz1^seed]]];
    px000=sPerlinRandom3D[v000][0];  py000=sPerlinRandom3D[v000][1];  pz000=sPerlinRandom3D[v000][2];
    px001=sPerlinRandom3D[v001][0];  py001=sPerlinRandom3D[v001][1];  pz001=sPerlinRandom3D[v001][2];
    px010=sPerlinRandom3D[v010][0];  py010=sPerlinRandom3D[v010][1];  pz010=sPerlinRandom3D[v010][2];
    px011=sPerlinRandom3D[v011][0];  py011=sPerlinRandom3D[v011][1];  pz011=sPerlinRandom3D[v011][2];
    px100=sPerlinRandom3D[v100][0];  py100=sPerlinRandom3D[v100][1];  pz100=sPerlinRandom3D[v100][2];
    px101=sPerlinRandom3D[v101][0];  py101=sPerlinRandom3D[v101][1];  pz101=sPerlinRandom3D[v101][2];
    px110=sPerlinRandom3D[v110][0];  py110=sPerlinRandom3D[v110][1];  pz110=sPerlinRandom3D[v110][2];
    px111=sPerlinRandom3D[v111][0];  py111=sPerlinRandom3D[v111][1];  pz111=sPerlinRandom3D[v111][2];

    float hx = 3*tx*tx - 2*tx*tx*tx;
    float hy = 3*ty*ty - 2*ty*ty*ty;
    float hz = 3*tz*tz - 2*tz*tz*tz;

    // X

    rx000 = px000*(1-hy)*(1-hz);
    rx001 = px001*(1-hy)*(1-hz);
    rx010 = px010*(  hy)*(1-hz);
    rx011 = px011*(  hy)*(1-hz);
    rx100 = px100*(1-hy)*(  hz);
    rx101 = px101*(1-hy)*(  hz);
    rx110 = px110*(  hy)*(  hz);
    rx111 = px111*(  hy)*(  hz);

    ry000 = py000*(1-hy)*(1-hz) * (ty-0);
    ry001 = py001*(1-hy)*(1-hz) * (ty-0);
    ry010 = py010*(  hy)*(1-hz) * (ty-1);
    ry011 = py011*(  hy)*(1-hz) * (ty-1);
    ry100 = py100*(1-hy)*(  hz) * (ty-0);
    ry101 = py101*(1-hy)*(  hz) * (ty-0);
    ry110 = py110*(  hy)*(  hz) * (ty-1);
    ry111 = py111*(  hy)*(  hz) * (ty-1);

    rz000 = pz000*(1-hy)*(1-hz) * (tz-0);
    rz001 = pz001*(1-hy)*(1-hz) * (tz-0);
    rz010 = pz010*(  hy)*(1-hz) * (tz-0);
    rz011 = pz011*(  hy)*(1-hz) * (tz-0);
    rz100 = pz100*(1-hy)*(  hz) * (tz-1);
    rz101 = pz101*(1-hy)*(  hz) * (tz-1);
    rz110 = pz110*(  hy)*(  hz) * (tz-1);
    rz111 = pz111*(  hy)*(  hz) * (tz-1);

    ra0 = rx000 + rx010 + rx100 + rx110;
    ra1 = rx001 + rx011 + rx101 + rx111;
    rb0 = ry000 + ry010 + ry100 + ry110 + rz000 + rz010 + rz100 + rz110;
    rb1 = ry001 + ry011 + ry101 + ry111 + rz001 + rz011 + rz101 + rz111;

    value = rb0 + tx*ra0 + tx*tx*3*(rb1-rb0-ra1) + tx*tx*tx*(5*ra1-3*ra0-2*rb1+2*rb0) - tx*tx*tx*tx*2*(ra1-ra0); 
    dir.x = ra0 + 6*tx*(rb1-rb0-ra1) + 3*tx*tx*(5*ra1-3*ra0-2*rb1+2*rb0) - 8*tx*tx*tx*(ra1-ra0); 

    // Y

    rx000 = px000*(1-hx)*(1-hz) * (tx-0);
    rx001 = px001*(  hx)*(1-hz) * (tx-1);
    rx010 = px010*(1-hx)*(1-hz) * (tx-0);
    rx011 = px011*(  hx)*(1-hz) * (tx-1);
    rx100 = px100*(1-hx)*(  hz) * (tx-0);
    rx101 = px101*(  hx)*(  hz) * (tx-1);
    rx110 = px110*(1-hx)*(  hz) * (tx-0);
    rx111 = px111*(  hx)*(  hz) * (tx-1);

    ry000 = py000*(1-hx)*(1-hz);
    ry001 = py001*(  hx)*(1-hz);
    ry010 = py010*(1-hx)*(1-hz);
    ry011 = py011*(  hx)*(1-hz);
    ry100 = py100*(1-hx)*(  hz);
    ry101 = py101*(  hx)*(  hz);
    ry110 = py110*(1-hx)*(  hz);
    ry111 = py111*(  hx)*(  hz);

    rz000 = pz000*(1-hx)*(1-hz) * (tz-0);
    rz001 = pz001*(  hx)*(1-hz) * (tz-0);
    rz010 = pz010*(1-hx)*(1-hz) * (tz-0);
    rz011 = pz011*(  hx)*(1-hz) * (tz-0);
    rz100 = pz100*(1-hx)*(  hz) * (tz-1);
    rz101 = pz101*(  hx)*(  hz) * (tz-1);
    rz110 = pz110*(1-hx)*(  hz) * (tz-1);
    rz111 = pz111*(  hx)*(  hz) * (tz-1);

    ra0 = ry000 + ry001 + ry100 + ry101;
    ra1 = ry010 + ry011 + ry110 + ry111;
    rb0 = rx000 + rx001 + rx100 + rx101 + rz000 + rz001 + rz100 + rz101;
    rb1 = rx010 + rx011 + rx110 + rx111 + rz010 + rz011 + rz110 + rz111;

    dir.y = ra0 + 6*ty*(rb1-rb0-ra1) + 3*ty*ty*(5*ra1-3*ra0-2*rb1+2*rb0) - 8*ty*ty*ty*(ra1-ra0); 

    // Z

    rx000 = px000*(1-hy)*(1-hx) * (tx-0);
    rx001 = px001*(1-hy)*(  hx) * (tx-1);
    rx010 = px010*(  hy)*(1-hx) * (tx-0);
    rx011 = px011*(  hy)*(  hx) * (tx-1);
    rx100 = px100*(1-hy)*(1-hx) * (tx-0);
    rx101 = px101*(1-hy)*(  hx) * (tx-1);
    rx110 = px110*(  hy)*(1-hx) * (tx-0);
    rx111 = px111*(  hy)*(  hx) * (tx-1);

    ry000 = py000*(1-hy)*(1-hx) * (ty-0);
    ry001 = py001*(1-hy)*(  hx) * (ty-0);
    ry010 = py010*(  hy)*(1-hx) * (ty-1);
    ry011 = py011*(  hy)*(  hx) * (ty-1);
    ry100 = py100*(1-hy)*(1-hx) * (ty-0);
    ry101 = py101*(1-hy)*(  hx) * (ty-0);
    ry110 = py110*(  hy)*(1-hx) * (ty-1);
    ry111 = py111*(  hy)*(  hx) * (ty-1);

    rz000 = pz000*(1-hy)*(1-hx);
    rz001 = pz001*(1-hy)*(  hx);
    rz010 = pz010*(  hy)*(1-hx);
    rz011 = pz011*(  hy)*(  hx);
    rz100 = pz100*(1-hy)*(1-hx);
    rz101 = pz101*(1-hy)*(  hx);
    rz110 = pz110*(  hy)*(1-hx);
    rz111 = pz111*(  hy)*(  hx);

    ra0 = rz000 + rz001 + rz010 + rz011;
    ra1 = rz100 + rz101 + rz110 + rz111;
    rb0 = rx000 + rx001 + rx010 + rx011 + ry000 + ry001 + ry010 + ry011;
    rb1 = rx100 + rx101 + rx110 + rx111 + ry100 + ry101 + ry110 + ry111;

    dir.z = ra0 + 6*tz*(rb1-rb0-ra1) + 3*tz*tz*(5*ra1-3*ra0-2*rb1+2*rb0) - 8*tz*tz*tz*(ra1-ra0); 
}

float sPerlin2D(float x,float y,int octaves,float falloff,int mode,int seed)
{
    int xi = int(x*0x10000);
    int yi = int(y*0x10000);
    int mask = 0xff;
    float sum,amp;

    sum = 0;
    amp = 1.0f;

    for(int i=0;i<octaves && i<8;i++)
    {
        float val = sPerlin2D(xi,yi,mask,seed);
        if(mode&1)
            val = (float)sAbs(val)*2-1;
        if(mode&2)
            val = (float)sSin(val*s2Pi);
        sum += val * amp;

        amp *= falloff;
        xi = xi*2;
        yi = yi*2;
    }

    return sum;
}

/****************************************************************************/
/***                                                                      ***/
/***   Biquad Filters                                                     ***/
/***                                                                      ***/
/****************************************************************************/

sBiquad::sBiquad()
{
    a1 = a2 = b0 = b1 = b2 = 0;
    Reset();
}

void sBiquad::Reset()
{
    x1 = x2 = y1 = y2 = 0;
}

float sBiquad::Filter(float x)
{
    float y = b0*x + b1*x1 + b2*x2 - a1*y1 - a2*y2;

    x2 = x1; x1 = x;
    y2 = y1; y1 = y;

    return y;
}

void sBiquad::LowPass(float freq,float q)
{
    double cof = cos(sPiDouble*2*freq);
    double alpha = sin(sPiDouble*2*freq) / (2*q);

    double B0 = (1-cof)*0.5;
    double B1 = 1-cof;
    double B2 = (1-cof)*0.5;
    double A0 = 1+alpha;
    double A1 = -2*cof;
    double A2 = 1-alpha;

    b0 = float(B0/A0);
    b1 = float(B1/A0);
    b2 = float(B2/A0);
    a1 = float(B1/A0);
    a2 = float(B2/A0);
}

void sBiquad::HighPass(float freq,float q)
{
    double cof = cos(sPiDouble*2*freq);
    double sof = sin(sPiDouble*2*freq);
    double alpha = sof / (2*q);

    double B0 = (1+cof)*0.5;
    double B1 =-(1+cof);
    double B2 = (1+cof)*0.5;
    double A0 = 1+alpha;
    double A1 = -2*cof;
    double A2 = 1-alpha;

    b0 = float(B0/A0);
    b1 = float(B1/A0);
    b2 = float(B2/A0);
    a1 = float(B1/A0);
    a2 = float(B2/A0);
}

void sBiquad::BandPass(float freq,float q)
{
    double cof = cos(sPiDouble*2*freq);
    double sof = sin(sPiDouble*2*freq);
    double alpha = sof / (2*q);

    double B0 = sof*0.5;
    double B1 = 0;
    double B2 = -sof*0.5;
    double A0 = 1+alpha;
    double A1 = -2*cof;
    double A2 = 1-alpha;

    b0 = float(B0/A0);
    b1 = float(B1/A0);
    b2 = float(B2/A0);
    a1 = float(B1/A0);
    a2 = float(B2/A0);
}

void sBiquad::BandPassConstantPeakGain(float freq,float q)
{
    double cof = cos(sPiDouble*2*freq);
    double sof = sin(sPiDouble*2*freq);
    double alpha = sof / (2*q);

    double B0 = alpha;
    double B1 = 0;
    double B2 = -alpha;
    double A0 = 1+alpha;
    double A1 = -2*cof;
    double A2 = 1-alpha;

    b0 = float(B0/A0);
    b1 = float(B1/A0);
    b2 = float(B2/A0);
    a1 = float(B1/A0);
    a2 = float(B2/A0);
}

/****************************************************************************/
/****************************************************************************/

class sMathSubsystem : public sSubsystem
{
public:
    sMathSubsystem() : sSubsystem("Math",0x02) {}
    void Init()
    {
        sInitPerlin();
    }
} sMathSubsystem_;

/****************************************************************************/

}

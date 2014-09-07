/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_BASE_MATH_HPP
#define FILE_ALTONA2_LIBS_BASE_MATH_HPP

#include "Altona2/Libs/Base/Base.hpp"

namespace Altona2 {

struct sTargetPara;

/****************************************************************************/
/***                                                                      ***/
/***   The types we are talking about...                                  ***/
/***                                                                      ***/
/****************************************************************************/

// Matrices are stored completely different from Altona(1).
//
// the 4x4 matrix translates into an affine matrix like this:
// y = Ax + t ; with A being the 3x3 rotation matrix, t being the translation
//            ; x being the orignal vector and y being the transformed vector
// 
// Then we build the matrix:
//
// | AAAt |
// | AAAt |
// | AAAt |
// | 0001 |
// 
// that is stored in sMatrix44A as 3 4d vectors i,j,k:
//
// i.Set(A00,A01,A02,tx)
// j.Set(A10,A11,A12,ty)
// k.Set(A20,A21,A22,tz)
//
// This is consistent with OpenGL and most textbooks and inconsistent with
// DirectX fixed function pipeline, which is obsolete anyway.
//
// This is more convenient to push into a vertex constant array, and
// less convenient if you want to extract the base vectors.
//
// Matrix x Matrix and Matrix x Vector multiplies follow standard notation,
// where the Matrix that transforms a vector precedes the vector like an
// operator.

struct sVector2;                  // 2d vector
struct sVector3;                  // 3d vector
//struct sVector30;                 // 3d vector with Z=0
//struct sVector31;                 // 3d vector with Z=1
struct sVector4;                  // 4d vector
struct sVector40;                 // 4d vector with W=0
struct sVector41;                 // 4d vector with W=1

struct sMatrix22;                 // 2x2 Matrix (row major)
struct sMatrix33;                 // 3x3 Matrix
//struct sMatrix33A;                // 3x3 Affine Matrix
struct sMatrix44;                 // 4x4 Matrix
struct sMatrix44A;                // 4x4 Affine Matrix

struct sQuaternion;               // quaternion rotation

/****************************************************************************/
/***                                                                      ***/
/***   Some small fry...                                                  ***/
/***                                                                      ***/
/****************************************************************************/

inline float sSaturate(float s) { if(s<0) return 0; if(s>1) return 1; return s; }
inline float sLerp(float v0,float v1,float f) { return v0*(1-f) + v1*f; }

enum sClipTest
{
    sCT_Out = 0,
    sCT_Partial = 1,
    sCT_In = 2,
};

/****************************************************************************/
/***                                                                      ***/
/***   Vectors 2d                                                         ***/
/***                                                                      ***/
/****************************************************************************/

struct sVector2
{
    float x,y;
    sVector2()                  { x=0; y=0; }
    sVector2(float X,float Y)     { x=X; y=Y; }
    void Set()                  { x=0; y=0; }
    void Set(float X,float Y)     { x=X; y=Y; }
    void Set(const sVector2 &a) { x=a.x; y=a.y; }

    template <class Ser> void Serialize_(Ser &s) { s | x | y; }
};

/****************************************************************************/
/***                                                                      ***/
/***   Vectors 3d                                                         ***/
/***                                                                      ***/
/****************************************************************************/

struct sVector3
{
    float x,y,z;
    sVector3()                     { x=0; y=0; z=0; }
    sVector3(float X,float Y,float Z) { x=X; y=Y; z=Z; }
    sVector3(const sVector40 &a);
    sVector3(const sVector41 &a);
    void Set()                     { x=0; y=0; z=0; }
    void Set(float X,float Y,float Z) { x=X; y=Y; z=Z; }
    void Set(const sVector3 &a)    { x=a.x; y=a.y; z=a.z; }
    void Set(const sVector4 &a);
    void Set(const sVector40 &a);
    void Set(const sVector41 &a);
    void SetColor(uint c) { x=((c>>16)&255)/255.0f; y=((c>>8)&255)/255.0f; z=((c>>0)&255)/255.0f; }
    uint GetColor() const { return (sClamp(int(x*255),0,255)<<16) | (sClamp(int(y*255),0,255)<<8) | (sClamp(int(z*255),0,255)<<0); };

    template <class Ser> void Serialize_(Ser &s) { s | x | y | z; }
};

/****************************************************************************/
/***                                                                      ***/
/***   Vectors 4d                                                         ***/
/***                                                                      ***/
/****************************************************************************/

struct sVector4
{
    float x,y,z,w;

    sVector4()                            { x=0; y=0; z=0; w=0; }
    sVector4(float X,float Y,float Z,float W) { x=X; y=Y; z=Z; w=W; }
    sVector4(const sVector4 &a);
    explicit sVector4(const sVector40 &a);
    explicit sVector4(const sVector41 &a);
    explicit sVector4(const sVector3 &a) { x=a.x;y=a.y;z=a.z; w=0; }
    void Set()                            { x=0; y=0; z=0; w=0; }
    void Set(float X,float Y,float Z,float W) { x=X; y=Y; z=Z; w=W; }
    void Set(const sVector2 &a,const sVector2 &b) { Set(a.x,a.y,b.x,b.y); }
    void Set(const sVector4  &a);
    void Set(const sVector40 &a);
    void Set(const sVector41 &a);
    void SetColor(uint c) { x=((c>>16)&255)/255.0f; y=((c>>8)&255)/255.0f; z=((c>>0)&255)/255.0f; w=((c>>24)&255)/255.0f; }
    uint GetColor() const { return (sClamp(int(x*255),0,255)<<16) | (sClamp(int(y*255),0,255)<<8) | (sClamp(int(z*255),0,255)<<0) | (sClamp(int(w*255),0,255)<<24); };
    void SetPlane(const sVector41 &pos,const sVector40 &norm);
    void SetPlane(const sVector41 &p0,const sVector41 &p1,const sVector41 &p2);

    template <class Ser> void Serialize_(Ser &s) { s | x | y | z | w; }
};

/****************************************************************************/

struct sVector40
{
    float x,y,z;
    sVector40()                     { x=0; y=0; z=0; }
    sVector40(float X,float Y,float Z) { x=X; y=Y; z=Z; }
    explicit sVector40(const sVector4 &a);
    sVector40(const sVector40 &a);
    explicit sVector40(const sVector41 &a);
    explicit sVector40(const sVector3 &a) { x=a.x;y=a.y;z=a.z; }
    void Set()                      { x=0; y=0; z=0; }
    void Set(float X,float Y,float Z)  { x=X; y=Y; z=Z; }
    void Set(const sVector3 &a);
    void Set(const sVector4 &a);
    void Set(const sVector40 &a);
    void Set(const sVector41 &a);
    void SetColor(uint c) { x=((c>>16)&255)/255.0f; y=((c>>8)&255)/255.0f; z=((c>>0)&255)/255.0f; }
    uint GetColor() const { return (sClamp(int(x*255),0,255)<<16) | (sClamp(int(y*255),0,255)<<8) | (sClamp(int(z*255),0,255)<<0); };

    template <class Ser> void Serialize_(Ser &s) { s | x | y | z; }
};

/****************************************************************************/

struct sVector41
{
    float x,y,z;

    sVector41()                     { x=0; y=0; z=0; }
    sVector41(float X,float Y,float Z) { x=X; y=Y; z=Z; }
    explicit sVector41(const sVector4 &a);
    explicit sVector41(const sVector40 &a);
    explicit sVector41(const sVector3 &a) { x=a.x;y=a.y;z=a.z; }
    sVector41(const sVector41 &a);
    void Set()                      { x=0; y=0; z=0; }
    void Set(float X,float Y,float Z)  { x=X; y=Y; z=Z; }
    void Set(const sVector3 &a);
    void Set(const sVector4 &a);
    void Set(const sVector40 &a);
    void Set(const sVector41 &a);
    void SetColor(uint c) { x=((c>>16)&255)/255.0f; y=((c>>8)&255)/255.0f; z=((c>>0)&255)/255.0f; }
    uint GetColor() const { return (sClamp(int(x*255),0,255)<<16) | (sClamp(int(y*255),0,255)<<8) | (sClamp(int(z*255),0,255)<<0) | 0xff000000; };

    template <class Ser> void Serialize_(Ser &s) { s | x | y | z; }
};

/****************************************************************************/
/***                                                                      ***/
/***   vector operators                                                   ***/
/***                                                                      ***/
/****************************************************************************/

inline sReader& operator| (sReader &s,sVector2 &v) { v.Serialize_(s); return s; }
inline sWriter& operator| (sWriter &s,sVector2 &v) { v.Serialize_(s); return s; }

inline sReader& operator| (sReader &s,sVector3 &v) { v.Serialize_(s); return s; }
inline sWriter& operator| (sWriter &s,sVector3 &v) { v.Serialize_(s); return s; }

inline sReader& operator| (sReader &s,sVector4 &v) { v.Serialize_(s); return s; }
inline sWriter& operator| (sWriter &s,sVector4 &v) { v.Serialize_(s); return s; }

inline sReader& operator| (sReader &s,sVector40 &v) { v.Serialize_(s); return s; }
inline sWriter& operator| (sWriter &s,sVector40 &v) { v.Serialize_(s); return s; }

inline sReader& operator| (sReader &s,sVector41 &v) { v.Serialize_(s); return s; }
inline sWriter& operator| (sWriter &s,sVector41 &v) { v.Serialize_(s); return s; }

/****************************************************************************/

inline void sVector3 ::Set(const sVector4  &a)                    { x=a.x; y=a.y; z=a.z; }
inline void sVector3 ::Set(const sVector40 &a)                    { x=a.x; y=a.y; z=a.z; }
inline void sVector3 ::Set(const sVector41 &a)                    { x=a.x; y=a.y; z=a.z; }
inline sVector3 ::sVector3(const sVector40 &a)                    { x=a.x; y=a.y; z=a.z; }
inline sVector3 ::sVector3(const sVector41 &a)                    { x=a.x; y=a.y; z=a.z; }

inline sVector4 ::sVector4 (const sVector4  &a)                   { x=a.x; y=a.y; z=a.z; w=a.w; }
inline sVector4 ::sVector4 (const sVector40 &a)                   { x=a.x; y=a.y; z=a.z; w= 0 ; }
inline sVector4 ::sVector4 (const sVector41 &a)                   { x=a.x; y=a.y; z=a.z; w= 1 ; }
inline void sVector4 ::Set(const sVector4  &a)                    { x=a.x; y=a.y; z=a.z; w=a.w; }
inline void sVector4 ::Set(const sVector40 &a)                    { x=a.x; y=a.y; z=a.z; w= 0 ; }
inline void sVector4 ::Set(const sVector41 &a)                    { x=a.x; y=a.y; z=a.z; w= 1 ; }

inline sVector40::sVector40(const sVector4  &a)                   { x=a.x; y=a.y; z=a.z; }
inline sVector40::sVector40(const sVector40 &a)                   { x=a.x; y=a.y; z=a.z; }
inline sVector40::sVector40(const sVector41 &a)                   { x=a.x; y=a.y; z=a.z; }
inline void sVector40::Set(const sVector3 &a)                     { x=a.x; y=a.y; z=a.z; }
inline void sVector40::Set(const sVector4 &a)                     { x=a.x; y=a.y; z=a.z; }
inline void sVector40::Set(const sVector40 &a)                    { x=a.x; y=a.y; z=a.z; }
inline void sVector40::Set(const sVector41 &a)                    { x=a.x; y=a.y; z=a.z; }

inline sVector41::sVector41(const sVector4  &a)                   { x=a.x; y=a.y; z=a.z; }
inline sVector41::sVector41(const sVector40 &a)                   { x=a.x; y=a.y; z=a.z; }
inline sVector41::sVector41(const sVector41 &a)                   { x=a.x; y=a.y; z=a.z; }
inline void sVector41::Set(const sVector3 &a)                     { x=a.x; y=a.y; z=a.z; }
inline void sVector41::Set(const sVector4 &a)                     { x=a.x; y=a.y; z=a.z; }
inline void sVector41::Set(const sVector40 &a)                    { x=a.x; y=a.y; z=a.z; }
inline void sVector41::Set(const sVector41 &a)                    { x=a.x; y=a.y; z=a.z; }

/****************************************************************************/

inline const sVector2  operator+ (const sVector2  &a,const sVector2  &b)  { return sVector2(a.x+b.x,a.y+b.y); }
inline const sVector3  operator+ (const sVector3  &a,const sVector3  &b)  { return sVector3(a.x+b.x,a.y+b.y,a.z+b.z); }
inline const sVector4  operator+ (const sVector4  &a,const sVector4  &b)  { return sVector4(a.x+b.x,a.y+b.y,a.z+b.z,a.w+b.w); }
inline const sVector4  operator+ (const sVector4  &a,const sVector40 &b)  { return sVector4(a.x+b.x,a.y+b.y,a.z+b.z,a.w); }
inline const sVector4  operator+ (const sVector4  &a,const sVector41 &b)  { return sVector4(a.x+b.x,a.y+b.y,a.z+b.z,a.w+1); }
inline const sVector40 operator+ (const sVector40 &a,const sVector40 &b)  { return sVector40(a.x+b.x,a.y+b.y,a.z+b.z); }
inline const sVector41 operator+ (const sVector41 &a,const sVector40 &b)  { return sVector41(a.x+b.x,a.y+b.y,a.z+b.z); }
inline const sVector41 operator+ (const sVector40 &a,const sVector41 &b)  { return sVector41(a.x+b.x,a.y+b.y,a.z+b.z); }
inline const sVector2  operator+ (const sVector2  &a,float b)             { return sVector2(a.x+b,a.y+b); }
inline const sVector3  operator+ (const sVector3  &a,float b)             { return sVector3(a.x+b,a.y+b,a.z+b); }
inline const sVector4  operator+ (const sVector4  &a,float b)             { return sVector4(a.x+b,a.y+b,a.z+b,a.w+b); }
inline const sVector2  operator+ (float a,const sVector2  &b)             { return sVector2(a+b.x,a+b.y); }
inline const sVector3  operator+ (float a,const sVector3  &b)             { return sVector3(a+b.x,a+b.y,a+b.z); }
inline const sVector4  operator+ (float a,const sVector4  &b)             { return sVector4(a+b.x,a+b.y,a+b.z,a+b.w); }
inline       sVector2 &operator+=(      sVector2  &a,const sVector2  &b)  { a = a+b; return a; }
inline       sVector3 &operator+=(      sVector3  &a,const sVector3  &b)  { a = a+b; return a; }
inline       sVector4 &operator+=(      sVector4  &a,const sVector4  &b)  { a = a+b; return a; }
inline       sVector4 &operator+=(      sVector4  &a,const sVector40 &b)  { a = a+b; return a; }
inline       sVector4 &operator+=(      sVector4  &a,const sVector41 &b)  { a = a+b; return a; }
inline       sVector40&operator+=(      sVector40 &a,const sVector40 &b)  { a = a+b; return a; }
inline       sVector41&operator+=(      sVector41 &a,const sVector40 &b)  { a = a+b; return a; }

inline const sVector2  operator- (const sVector2  &a,const sVector2  &b)  { return sVector2(a.x-b.x,a.y-b.y); }
inline const sVector3  operator- (const sVector3  &a,const sVector3  &b)  { return sVector3(a.x-b.x,a.y-b.y,a.z-b.z); }
inline const sVector4  operator- (const sVector4  &a,const sVector4  &b)  { return sVector4(a.x-b.x,a.y-b.y,a.z-b.z,a.w-b.w); }
inline const sVector40 operator- (const sVector40 &a,const sVector40 &b)  { return sVector40(a.x-b.x,a.y-b.y,a.z-b.z); }
inline const sVector40 operator- (const sVector41 &a,const sVector41 &b)  { return sVector40(a.x-b.x,a.y-b.y,a.z-b.z); }
inline const sVector41 operator- (const sVector41 &a,const sVector40 &b)  { return sVector41(a.x-b.x,a.y-b.y,a.z-b.z); }
inline const sVector2  operator- (const sVector2  &a,float b)             { return sVector2(a.x-b,a.y-b); }
inline const sVector3  operator- (const sVector3  &a,float b)             { return sVector3(a.x-b,a.y-b,a.z-b); }
inline const sVector4  operator- (const sVector4  &a,float b)             { return sVector4(a.x-b,a.y-b,a.z-b,a.w-b); }
inline const sVector2  operator- (float a,const sVector2  &b)             { return sVector2(a-b.x,a-b.y); }
inline const sVector3  operator- (float a,const sVector3  &b)             { return sVector3(a-b.x,a-b.y,a-b.z); }
inline const sVector4  operator- (float a,const sVector4  &b)             { return sVector4(a-b.x,a-b.y,a-b.z,a-b.w); }
inline       sVector2 &operator-=(      sVector2  &a,const sVector2  &b)  { a = a-b; return a; }
inline       sVector3 &operator-=(      sVector3  &a,const sVector3  &b)  { a = a-b; return a; }
inline       sVector4 &operator-=(      sVector4  &a,const sVector4  &b)  { a = a-b; return a; }
inline       sVector40&operator-=(      sVector40 &a,const sVector40 &b)  { a = a-b; return a; }
inline       sVector41&operator-=(      sVector41 &a,const sVector40 &b)  { a = a-b; return a; }

inline const sVector2  operator* (const sVector2  &a,const sVector2  &b)  { return sVector2(a.x*b.x,a.y*b.y); }
inline const sVector3  operator* (const sVector3  &a,const sVector3  &b)  { return sVector3(a.x*b.x,a.y*b.y,a.z*b.z); }
inline const sVector4  operator* (const sVector4  &a,const sVector4  &b)  { return sVector4(a.x*b.x,a.y*b.y,a.z*b.z,a.w*b.w); }
inline const sVector4  operator* (const sVector4  &a,const sVector40 &b)  { return sVector4(a.x*b.x,a.y*b.y,a.z*b.z,0); }
inline const sVector4  operator* (const sVector4  &a,const sVector41 &b)  { return sVector4(a.x*b.x,a.y*b.y,a.z*b.z,a.w); }
inline const sVector40 operator* (const sVector40 &a,const sVector40 &b)  { return sVector40(a.x*b.x,a.y*b.y,a.z*b.z); }
inline const sVector40 operator* (const sVector41 &a,const sVector40 &b)  { return sVector40(a.x*b.x,a.y*b.y,a.z*b.z); }
inline const sVector40 operator* (const sVector40 &a,const sVector41 &b)  { return sVector40(a.x*b.x,a.y*b.y,a.z*b.z); }
inline const sVector41 operator* (const sVector41 &a,const sVector41 &b)  { return sVector41(a.x*b.x,a.y*b.y,a.z*b.z); }

inline const sVector2  operator* (float a,const sVector2  &b)              { return sVector2(a*b.x,a*b.y); }
inline const sVector3  operator* (float a,const sVector3  &b)              { return sVector3(a*b.x,a*b.y,a*b.z); }
inline const sVector4  operator* (float a,const sVector4  &b)              { return sVector4(a*b.x,a*b.y,a*b.z,a*b.w); }
inline const sVector40 operator* (float a,const sVector40 &b)              { return sVector40(a*b.x,a*b.y,a*b.z); }
inline const sVector2  operator* (const sVector2  &a,float b)              { return sVector2(a.x*b,a.y*b); }
inline const sVector3  operator* (const sVector3  &a,float b)              { return sVector3(a.x*b,a.y*b,a.z*b); }
inline const sVector4  operator* (const sVector4  &a,float b)              { return sVector4(a.x*b,a.y*b,a.z*b,a.w*b); }
inline const sVector40 operator* (const sVector40 &a,float b)              { return sVector40(a.x*b,a.y*b,a.z*b); }

inline       sVector2 &operator*=(      sVector2  &a,float b)              { a=a*b; return a; }
inline       sVector3 &operator*=(      sVector3  &a,float b)              { a=a*b; return a; }
inline       sVector4 &operator*=(      sVector4  &a,float b)              { a=a*b; return a; }
inline       sVector40&operator*=(      sVector40 &a,float b)              { a=a*b; return a; }
inline       sVector2 &operator*=(      sVector2  &a,const sVector2  &b)        { a=a*b; return a; }
inline       sVector3 &operator*=(      sVector3  &a,const sVector3  &b)        { a=a*b; return a; }
inline       sVector4 &operator*=(      sVector4  &a,const sVector4  &b)        { a=a*b; return a; }
inline       sVector4 &operator*=(      sVector4  &a,const sVector40 &b)        { a=a*b; return a; }
inline       sVector4 &operator*=(      sVector4  &a,const sVector41 &b)        { a=a*b; return a; }
inline       sVector40&operator*=(      sVector40 &a,const sVector40 &b)        { a=a*b; return a; }
inline       sVector41&operator*=(      sVector41 &a,const sVector41 &b)        { a=a*b; return a; }

inline float sDot(const sVector2 &a,const sVector2 &b) { return a.x*b.x + a.y*b.y; }
inline float sDot(const sVector3 &a,const sVector3 &b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline float sDot(const sVector4  &a,const sVector4  &b) { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }
inline float sDot(const sVector40 &a,const sVector4  &b) { return a.x*b.x + a.y*b.y + a.z*b.z +  0 *b.w; }
inline float sDot(const sVector41 &a,const sVector4  &b) { return a.x*b.x + a.y*b.y + a.z*b.z +  1 *b.w; }
inline float sDot(const sVector4  &a,const sVector40 &b) { return a.x*b.x + a.y*b.y + a.z*b.z + a.w* 0 ; }
inline float sDot(const sVector40 &a,const sVector40 &b) { return a.x*b.x + a.y*b.y + a.z*b.z +  0 * 0 ; }
inline float sDot(const sVector41 &a,const sVector40 &b) { return a.x*b.x + a.y*b.y + a.z*b.z +  1 * 0 ; }
inline float sDot(const sVector4  &a,const sVector41 &b) { return a.x*b.x + a.y*b.y + a.z*b.z + a.w* 1 ; }
inline float sDot(const sVector40 &a,const sVector41 &b) { return a.x*b.x + a.y*b.y + a.z*b.z +  0 * 1 ; }
inline float sDot(const sVector41 &a,const sVector41 &b) { return a.x*b.x + a.y*b.y + a.z*b.z +  1 * 1 ; }

inline sVector2 sPerpendicular(const sVector2 &a) { return sVector2(a.y,-a.x); }
inline float sCross(const sVector2 &a,const sVector2 &b) { return a.x*b.y - a.y*b.x; }
inline sVector3 sCross(const sVector3 &a,const sVector3 &b) { return sVector3(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x); }
inline sVector40 sCross(const sVector40 &a,const sVector40 &b) { return sVector40(a.y*b.z-a.z*b.y , a.z*b.x-a.x*b.z , a.x*b.y-a.y*b.x); }

inline sVector2 sAbs(const sVector2 &a) { return sVector2 (sAbs(a.x),sAbs(a.y)); }
inline sVector3 sAbs(const sVector3 &a) { return sVector3 (sAbs(a.x),sAbs(a.y),sAbs(a.z)); }
inline sVector4 sAbs(const sVector4 &a) { return sVector4 (sAbs(a.x),sAbs(a.y),sAbs(a.z),sAbs(a.w)); }
inline sVector40 sAbs(const sVector40 &a) { return sVector40(sAbs(a.x),sAbs(a.y),sAbs(a.z)); }
inline sVector41 sAbs(const sVector41 &a) { return sVector41(sAbs(a.x),sAbs(a.y),sAbs(a.z)); }

inline float sLength(const sVector2 &a) { return sSqrt(a.x*a.x + a.y*a.y); }
inline float sLength(const sVector3 &a) { return sSqrt(a.x*a.x + a.y*a.y + a.z*a.z); }
inline float sLength(const sVector4 &a) { return sSqrt(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w); }
inline float sLength(const sVector40 &a) { return sSqrt(a.x*a.x + a.y*a.y + a.z*a.z); }
inline float sLength(const sVector41 &a) { return sSqrt(a.x*a.x + a.y*a.y + a.z*a.z); }

inline float sDist(const sVector2  &a,const sVector2  &b) { return sLength(a-b); }
inline float sDist(const sVector3  &a,const sVector3  &b) { return sLength(a-b); }
inline float sDist(const sVector4  &a,const sVector4  &b) { return sLength(a-b); }
inline float sDist(const sVector40 &a,const sVector40 &b) { return sLength(a-b); }
inline float sDist(const sVector41 &a,const sVector41 &b) { return sLength(a-b); }

inline float sLengthSquared(const sVector2 &a) { return a.x*a.x + a.y*a.y; }
inline float sLengthSquared(const sVector3 &a) { return a.x*a.x + a.y*a.y + a.z*a.z; }
inline float sLengthSquared(const sVector4 &a) { return a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w; }
inline float sLengthSquared(const sVector40 &a) { return a.x*a.x + a.y*a.y + a.z*a.z; }
inline float sLengthSquared(const sVector41 &a) { return a.x*a.x + a.y*a.y + a.z*a.z; }

inline float sDistSquared(const sVector2  &a,const sVector2  &b) { return sLengthSquared(a-b); }
inline float sDistSquared(const sVector3  &a,const sVector3  &b) { return sLengthSquared(a-b); }
inline float sDistSquared(const sVector4  &a,const sVector4  &b) { return sLengthSquared(a-b); }
inline float sDistSquared(const sVector40 &a,const sVector40 &b) { return sLengthSquared(a-b); }
inline float sDistSquared(const sVector41 &a,const sVector41 &b) { return sLengthSquared(a-b); }

inline sVector2 sNormalize(const sVector2 &a) { return a*sRSqrt(sLengthSquared(a)); }
inline sVector3 sNormalize(const sVector3 &a) { return a*sRSqrt(sLengthSquared(a)); }
inline sVector4 sNormalize(const sVector4 &a) { return a*sRSqrt(sLengthSquared(a)); }
inline sVector40 sNormalize(const sVector40 &a) { return a*sRSqrt(sLengthSquared(a)); }

inline sVector2 sSaturate(const sVector2 &a) { return sVector2(sSaturate(a.x),sSaturate(a.y)); }
inline sVector3 sSaturate(const sVector3 &a) { return sVector3(sSaturate(a.x),sSaturate(a.y),sSaturate(a.z)); }
inline sVector4 sSaturate(const sVector4 &a) { return sVector4(sSaturate(a.x),sSaturate(a.y),sSaturate(a.z),sSaturate(a.w)); }
inline sVector40 sSaturate(const sVector40 &a) { return sVector40(sSaturate(a.x),sSaturate(a.y),sSaturate(a.z)); }
inline sVector41 sSaturate(const sVector41 &a) { return sVector41(sSaturate(a.x),sSaturate(a.y),sSaturate(a.z)); }

inline sVector2 sAverage(const sVector2 &a,const sVector2 &b) { return (a+b)*0.5f; }
inline sVector3 sAverage(const sVector3 &a,const sVector3 &b) { return (a+b)*0.5f; }
inline sVector4 sAverage(const sVector4 &a,const sVector4 &b) { return (a+b)*0.5f; }
inline sVector40 sAverage(const sVector40 &a,const sVector40 &b) { return (a+b)*0.5f; }
inline sVector41 sAverage(const sVector41 &a,const sVector41 &b) { return sVector41((a.x+b.x)*0.5f,(a.y+b.y)*0.5f,(a.z+b.z)*0.5f); }

inline sVector2  sLerp(const sVector2  &v0,const sVector2  &v1,float f1) { float f0=1-f1; return sVector2 (v0.x*f0+v1.x*f1,v0.y*f0+v1.y*f1); };
inline sVector3  sLerp(const sVector3  &v0,const sVector3  &v1,float f1) { float f0=1-f1; return sVector3 (v0.x*f0+v1.x*f1,v0.y*f0+v1.y*f1,v0.z*f0+v1.z*f1); };
inline sVector4  sLerp(const sVector4  &v0,const sVector4  &v1,float f1) { float f0=1-f1; return sVector4 (v0.x*f0+v1.x*f1 , v0.y*f0+v1.y*f1 , v0.z*f0+v1.z*f1 , v0.w*f0+v1.w*f1); };
inline sVector40 sLerp(const sVector40 &v0,const sVector40 &v1,float f1) { float f0=1-f1; return sVector40(v0.x*f0+v1.x*f1,v0.y*f0+v1.y*f1,v0.z*f0+v1.z*f1); };
inline sVector41 sLerp(const sVector41 &v0,const sVector41 &v1,float f1) { float f0=1-f1; return sVector41(v0.x*f0+v1.x*f1,v0.y*f0+v1.y*f1,v0.z*f0+v1.z*f1); };
inline sVector2  sLerp(const sVector2  &v0,const sVector2  &v1,const sVector2  &f1) { return sVector2 (v0.x*(1-f1.x)+v1.x*f1.x , v0.y*(1-f1.y)+v1.y*f1.y); };
inline sVector3  sLerp(const sVector3  &v0,const sVector3  &v1,const sVector3  &f1) { return sVector3 (v0.x*(1-f1.x)+v1.x*f1.x , v0.y*(1-f1.y)+v1.y*f1.y , v0.z*(1-f1.z)+v1.z*f1.z); };
inline sVector40 sLerp(const sVector40 &v0,const sVector40 &v1,const sVector40 &f1) { return sVector40(v0.x*(1-f1.x)+v1.x*f1.x , v0.y*(1-f1.y)+v1.y*f1.y , v0.z*(1-f1.z)+v1.z*f1.z); };
inline sVector41 sLerp(const sVector41 &v0,const sVector41 &v1,const sVector41 &f1) { return sVector41(v0.x*(1-f1.x)+v1.x*f1.x , v0.y*(1-f1.y)+v1.y*f1.y , v0.z*(1-f1.z)+v1.z*f1.z); };
inline sVector4  sLerp(const sVector4  &v0,const sVector4  &v1,const sVector4  &f1) { return sVector4 (v0.x*(1-f1.x)+v1.x*f1.x , v0.y*(1-f1.y)+v1.y*f1.y , v0.z*(1-f1.z)+v1.z*f1.z , v0.w*(1-f1.w)+v1.w*f1.w); };

inline sVector2 sMin(const sVector2 &a,const sVector2 &b) { return sVector2(sMin(a.x,b.x),sMin(a.y,b.y)); }
inline sVector3 sMin(const sVector3 &a,const sVector3 &b) { return sVector3(sMin(a.x,b.x),sMin(a.y,b.y),sMin(a.z,b.z)); }
inline sVector4 sMin(const sVector4 &a,const sVector4 &b) { return sVector4(sMin(a.x,b.x),sMin(a.y,b.y),sMin(a.z,b.z),sMin(a.w,b.w)); }
inline sVector40 sMin(const sVector40 &a,const sVector40 &b) { return sVector40(sMin(a.x,b.x),sMin(a.y,b.y),sMin(a.z,b.z)); }
inline sVector41 sMin(const sVector41 &a,const sVector41 &b) { return sVector41(sMin(a.x,b.x),sMin(a.y,b.y),sMin(a.z,b.z)); }

inline sVector2 sMax(const sVector2 &a,const sVector2 &b) { return sVector2(sMax(a.x,b.x),sMax(a.y,b.y)); }
inline sVector3 sMax(const sVector3 &a,const sVector3 &b) { return sVector3(sMax(a.x,b.x),sMax(a.y,b.y),sMax(a.z,b.z)); }
inline sVector4 sMax(const sVector4 &a,const sVector4 &b) { return sVector4(sMax(a.x,b.x),sMax(a.y,b.y),sMax(a.z,b.z),sMax(a.w,b.w)); }
inline sVector40 sMax(const sVector40 &a,const sVector40 &b) { return sVector40(sMax(a.x,b.x),sMax(a.y,b.y),sMax(a.z,b.z)); }
inline sVector41 sMax(const sVector41 &a,const sVector41 &b) { return sVector41(sMax(a.x,b.x),sMax(a.y,b.y),sMax(a.z,b.z)); }

/****************************************************************************/

inline bool operator==(const sVector2  &a,const sVector2  &b)             { return a.x==b.x && a.y==b.y; }
inline bool operator==(const sVector3  &a,const sVector3  &b)             { return a.x==b.x && a.y==b.y && a.z==b.z; }
inline bool operator==(const sVector4  &a,const sVector4  &b)             { return a.x==b.x && a.y==b.y && a.z==b.z && a.w==b.w; }
inline bool operator==(const sVector40 &a,const sVector40 &b)             { return a.x==b.x && a.y==b.y && a.z==b.z; }
inline bool operator==(const sVector41 &a,const sVector41 &b)             { return a.x==b.x && a.y==b.y && a.z==b.z; }

inline bool operator!=(const sVector2  &a,const sVector2  &b)             { return a.x!=b.x || a.y!=b.y; }
inline bool operator!=(const sVector3  &a,const sVector3  &b)             { return a.x!=b.x || a.y!=b.y || a.z!=b.z; }
inline bool operator!=(const sVector4  &a,const sVector4  &b)             { return a.x!=b.x || a.y!=b.y || a.z!=b.z || a.w!=b.w; }
inline bool operator!=(const sVector40 &a,const sVector40 &b)             { return a.x!=b.x || a.y!=b.y || a.z!=b.z; }
inline bool operator!=(const sVector41 &a,const sVector41 &b)             { return a.x!=b.x || a.y!=b.y || a.z!=b.z; }

/****************************************************************************/
/***                                                                      ***/
/***   Matrix 2x2                                                         ***/
/***                                                                      ***/
/****************************************************************************/

struct sMatrix22
{
    sVector2 i,j;
    sMatrix22(const sVector2 &I,const sVector2 &J) : i(I),j(J) {}
    sMatrix22()                                           { Identity(); }
    void Clear()                                          { i.Set(); j.Set(); }
    void Identity()                                       { i.Set(1,0); j.Set(0,1); }

    template <class Ser> void Serialize_(Ser &s) { s | i | j; }
};

/****************************************************************************/
/***                                                                      ***/
/***   Matrix 3x3                                                         ***/
/***                                                                      ***/
/****************************************************************************/

struct sMatrix33
{
    sVector3 i,j,k;
    sMatrix33(const sVector3 &I,const sVector3 &J,const sVector3 &K) : i(I),j(J),k(K) {}
    explicit sMatrix33(const sMatrix44 &m);
    explicit sMatrix33(const sMatrix44A &m);
    sMatrix33()                                           { Identity(); }
    void Clear()                                          { i.Set(); j.Set(); k.Set(); }
    void Identity()                                       { i.Set(1,0,0); j.Set(0,1,0); k.Set(0,0,1); }

    template <class Ser> void Serialize_(Ser &s) { s | i | j | k; }
};

struct sMatrix33A
{
    sVector3 i,j;
    sMatrix33A(const sVector3 &I,const sVector3 &J) : i(I),j(J) {}
    sMatrix33A(const sMatrix33A &m)                       { i=m.i; j=m.j; }
    sMatrix33A()                                          { Identity(); }
    void Clear()                                          { i.Set(); j.Set(); }
    void Identity()                                       { i.Set(1,0,0); j.Set(0,1,0); }

    template <class Ser> void Serialize_(Ser &s) { s | i | j; }
};

/****************************************************************************/
/***                                                                      ***/
/***   Matrix 4x4                                                         ***/
/***                                                                      ***/
/****************************************************************************/

struct sMatrix44
{
    sVector4 i,j,k,l;
    sMatrix44(const sVector4 &I,const sVector4 &J,const sVector4 &K,const sVector4 &L) : i(I),j(J),k(K),l(L) {}
    sMatrix44(const sMatrix33 &m) : i(m.i.x,m.i.y,m.i.z,0),j(m.j.x,m.j.y,m.j.z,0),k(m.k.x,m.k.y,m.k.z,0),l(0,0,0,1) {}
    sMatrix44(const sMatrix44A &m);
    sMatrix44()                                           { Identity(); }
    void Clear()                                          { i.Set(); j.Set(); k.Set(); l.Set(); }
    void Identity()                                       { i.Set(1,0,0,0); j.Set(0,1,0,0); k.Set(0,0,1,0); l.Set(0,0,0,1); }

    template <class Ser> void Serialize_(Ser &s) { s | i | j | k | l; }

    void SetViewportScreen();
    void SetViewportPixels(int sx,int sy);


    inline void Set(const sF32 *d, bool t)
    {
        if (!t)
        {
            i.Set(d[0] , d[1] , d[2] , d[3]);
            j.Set(d[4] , d[5] , d[6] , d[7]);
            k.Set(d[8] , d[9] , d[10], d[11]);
            l.Set(d[12], d[13], d[14], d[15]);
        }
        else
        {
            i.Set(d[0] , d[4] , d[8] , d[12]);
            j.Set(d[1] , d[5] , d[9] , d[13]);
            k.Set(d[2] , d[6] , d[10], d[14]);
            l.Set(d[3] , d[7] , d[11], d[15]);
        }
    }
};

/****************************************************************************/

struct sMatrix44A
{
    sVector4 i,j,k;
    sMatrix44A(const sVector4 &I,const sVector4 &J,const sVector4 &K) : i(I),j(J),k(K) {}
    sMatrix44A(const sMatrix33 &m) : i(m.i.x,m.i.y,m.i.z,0),j(m.j.x,m.j.y,m.j.z,0),k(m.k.x,m.k.y,m.k.z,0) {}
    explicit sMatrix44A(const sMatrix44 &m) : i(m.i),j(m.j),k(m.k) {}
    sMatrix44A()                                          { Identity(); }
    void Clear()                                          { i.Set(); j.Set(); k.Set(); }
    void Identity()                                       { i.Set(1,0,0,0); j.Set(0,1,0,0); k.Set(0,0,1,0);  }
    void Orthonormalize();
   
    void SetBaseX(const sVector40 &a)                     { i.x=a.x; j.x=a.y; k.x=a.z; }
    void SetBaseY(const sVector40 &a)                     { i.y=a.x; j.y=a.y; k.y=a.z; }
    void SetBaseZ(const sVector40 &a)                     { i.z=a.x; j.z=a.y; k.z=a.z; }
    void SetTrans(const sVector41 &a)                     { i.w=a.x; j.w=a.y; k.w=a.z; }
 
    sVector40 GetBaseX()                                  { return sVector40(i.x,j.x,k.x); }
    sVector40 GetBaseY()                                  { return sVector40(i.y,j.y,k.y); }
    sVector40 GetBaseZ()                                  { return sVector40(i.z,j.z,k.z); }
    sVector41 GetTrans()                                  { return sVector41(i.w,j.w,k.w); }

    void Init(sQuaternion &q);

    template <class Ser> void Serialize_(Ser &s) { s | i | j | k; }
};

/****************************************************************************/
/***                                                                      ***/
/***   Matrix Operators                                                   ***/
/***                                                                      ***/
/****************************************************************************/

inline sReader& operator| (sReader &s,sMatrix22 &v) { v.Serialize_(s); return s; }
inline sWriter& operator| (sWriter &s,sMatrix22 &v) { v.Serialize_(s); return s; }

inline sReader& operator| (sReader &s,sMatrix33 &v) { v.Serialize_(s); return s; }
inline sWriter& operator| (sWriter &s,sMatrix33 &v) { v.Serialize_(s); return s; }

inline sReader& operator| (sReader &s,sMatrix44 &v) { v.Serialize_(s); return s; }
inline sWriter& operator| (sWriter &s,sMatrix44 &v) { v.Serialize_(s); return s; }

inline sReader& operator| (sReader &s,sMatrix44A &v) { v.Serialize_(s); return s; }
inline sWriter& operator| (sWriter &s,sMatrix44A &v) { v.Serialize_(s); return s; }

/****************************************************************************/

inline bool operator==(const sMatrix22  &a,const sMatrix22  &b)           { return a.i==b.i && a.j==b.j; }
inline bool operator==(const sMatrix33  &a,const sMatrix33  &b)           { return a.i==b.i && a.j==b.j && a.k==b.k; }
inline bool operator==(const sMatrix44  &a,const sMatrix44  &b)           { return a.i==b.i && a.j==b.j && a.k==b.k && a.l==b.l; }
inline bool operator==(const sMatrix44A &a,const sMatrix44A &b)           { return a.i==b.i && a.j==b.j && a.k==b.k; }

inline bool operator!=(const sMatrix22  &a,const sMatrix22  &b)           { return a.i!=b.i || a.j!=b.j; }
inline bool operator!=(const sMatrix33  &a,const sMatrix33  &b)           { return a.i!=b.i || a.j!=b.j || a.k!=b.k; }
inline bool operator!=(const sMatrix44  &a,const sMatrix44  &b)           { return a.i!=b.i || a.j!=b.j || a.k!=b.k || a.l!=b.l; }
inline bool operator!=(const sMatrix44A &a,const sMatrix44A &b)           { return a.i!=b.i || a.j!=b.j || a.k!=b.k; }

inline sMatrix22  operator+(const sMatrix22 &a,const sMatrix22 &b)        { return sMatrix22 (a.i+b.i , a.j+b.j); }
inline sMatrix33  operator+(const sMatrix33 &a,const sMatrix33 &b)        { return sMatrix33 (a.i+b.i , a.j+b.j , a.k+b.k); }
inline sMatrix44  operator+(const sMatrix44 &a,const sMatrix44 &b)        { return sMatrix44 (a.i+b.i , a.j+b.j , a.k+b.k , a.l+b.l); }
inline sMatrix44A operator+(const sMatrix44A &a,const sMatrix44A &b)      { return sMatrix44A(a.i+b.i , a.j+b.j , a.k+b.k); }

inline sMatrix22  operator-(const sMatrix22 &a,const sMatrix22 &b)        { return sMatrix22 (a.i-b.i , a.j-b.j); }
inline sMatrix33  operator-(const sMatrix33 &a,const sMatrix33 &b)        { return sMatrix33 (a.i-b.i , a.j-b.j , a.k-b.k); }
inline sMatrix44  operator-(const sMatrix44 &a,const sMatrix44 &b)        { return sMatrix44 (a.i-b.i , a.j-b.j , a.k-b.k , a.l-b.l); }
inline sMatrix44A operator-(const sMatrix44A &a,const sMatrix44A &b)      { return sMatrix44A(a.i-b.i , a.j-b.j , a.k-b.k); }

inline sMatrix22  operator*(const sMatrix22 &a,float s)                    { return sMatrix22 (a.i*s , a.j*s); }
inline sMatrix33  operator*(const sMatrix33 &a,float s)                    { return sMatrix33 (a.i*s , a.j*s , a.k*s); }
inline sMatrix44  operator*(const sMatrix44 &a,float s)                    { return sMatrix44 (a.i*s , a.j*s , a.k*s , a.l*s); }
inline sMatrix44A operator*(const sMatrix44A &a,float s)                   { return sMatrix44A(a.i*s , a.j*s , a.k*s); }

/****************************************************************************/

inline sMatrix33::sMatrix33(const sMatrix44 &m)  : i(m.i.x,m.i.y,m.i.z),j(m.j.x,m.j.y,m.j.z),k(m.k.x,m.k.y,m.k.z) {}
inline sMatrix33::sMatrix33(const sMatrix44A &m) : i(m.i.x,m.i.y,m.i.z),j(m.j.x,m.j.y,m.j.z),k(m.k.x,m.k.y,m.k.z) {}
inline sMatrix44::sMatrix44(const sMatrix44A &m) : i(m.i),j(m.j),k(m.k),l(0,0,0,1) {}

inline sVector2 sBaseX(const sMatrix22 &m)                                { return sVector2(m.i.x,m.j.x); }
inline sVector2 sBaseY(const sMatrix22 &m)                                { return sVector2(m.i.y,m.j.y); }
inline sVector3 sBaseX(const sMatrix33 &m)                                { return sVector3(m.i.x,m.j.x,m.k.x); }
inline sVector3 sBaseY(const sMatrix33 &m)                                { return sVector3(m.i.y,m.j.y,m.k.y); }
inline sVector3 sBaseZ(const sMatrix33 &m)                                { return sVector3(m.i.z,m.j.z,m.k.z); }
inline sVector4 sBaseX(const sMatrix44 &m)                                { return sVector4(m.i.x,m.j.x,m.k.x,m.l.x); }
inline sVector4 sBaseY(const sMatrix44 &m)                                { return sVector4(m.i.y,m.j.y,m.k.y,m.l.y); }
inline sVector4 sBaseZ(const sMatrix44 &m)                                { return sVector4(m.i.z,m.j.z,m.k.z,m.l.z); }
inline sVector4 sTrans(const sMatrix44 &m)                                { return sVector4(m.i.w,m.j.w,m.k.w,m.l.w); }
inline sVector40 sBaseX(const sMatrix44A &m)                              { return sVector40(m.i.x,m.j.x,m.k.x); }
inline sVector40 sBaseY(const sMatrix44A &m)                              { return sVector40(m.i.y,m.j.y,m.k.y); }
inline sVector40 sBaseZ(const sMatrix44A &m)                              { return sVector40(m.i.z,m.j.z,m.k.z); }
inline sVector41 sTrans(const sMatrix44A &m)                              { return sVector41(m.i.w,m.j.w,m.k.w); }

sMatrix22  sInvert(const sMatrix22  &m);
sMatrix33  sInvert(const sMatrix33  &m);
sMatrix44  sInvert(const sMatrix44  &m);
sMatrix44A sInvert(const sMatrix44A &m);

sMatrix22  sTranspose(const sMatrix22  &m);
sMatrix33  sTranspose(const sMatrix33  &m);
sMatrix44  sTranspose(const sMatrix44  &m);
sMatrix44A sTranspose(const sMatrix44A &m);

float sDeterminant(const sMatrix22  &m);
float sDeterminant(const sMatrix33  &m);
float sDeterminant(const sMatrix44  &m);
float sDeterminant(const sMatrix44A &m);

/****************************************************************************/

inline sVector2 operator*(const sVector2 &v,const sMatrix22 &m)
{
    return sVector2(  
        v.x*m.i.x + v.y*m.j.x,
        v.x*m.i.y + v.y*m.j.y);
}

inline sVector2 operator*(const sMatrix22 &m,const sVector2 &v)
{
    return sVector2(  
        m.i.x*v.x + m.i.y*v.y,
        m.j.x*v.x + m.j.y*v.y);
}


inline sVector3 operator*(const sVector3 &v,const sMatrix33 &m)
{
    return sVector3(  
        v.x*m.i.x + v.y*m.j.x + v.z*m.k.x,
        v.x*m.i.y + v.y*m.j.y + v.z*m.k.y,
        v.x*m.i.z + v.y*m.j.z + v.z*m.k.z);
}

inline sVector3 operator*(const sMatrix33 &m,const sVector3 &v)
{
    return sVector3(  
        m.i.x*v.x + m.i.y*v.y + m.i.z*v.z,
        m.j.x*v.x + m.j.y*v.y + m.j.z*v.z,
        m.k.x*v.x + m.k.y*v.y + m.k.z*v.z);
}

/****************************************************************************/


inline sVector4 operator*(const sVector4 &v,const sMatrix44 &m)
{
    return sVector4(
        v.x*m.i.x + v.y*m.j.x + v.z*m.k.x + v.w*m.k.x,
        v.x*m.i.y + v.y*m.j.y + v.z*m.k.y + v.w*m.k.y,
        v.x*m.i.z + v.y*m.j.z + v.z*m.k.z + v.w*m.k.z,
        v.x*m.i.w + v.y*m.j.w + v.z*m.k.w + v.w*m.k.w);
}

inline sVector4 operator*(const sMatrix44 &m,const sVector4 &v)
{
    return sVector4(
        m.i.x*v.x + m.i.y*v.y + m.i.z*v.z + m.i.w*v.w,
        m.j.x*v.x + m.j.y*v.y + m.j.z*v.z + m.j.w*v.w,
        m.k.x*v.x + m.k.y*v.y + m.k.z*v.z + m.k.w*v.w,
        m.l.x*v.x + m.l.y*v.y + m.l.z*v.z + m.l.w*v.w);
}

inline sVector4 operator*(const sVector4 &v,const sMatrix44A &m)
{
    return sVector4(
        v.x*m.i.x + v.y*m.j.x + v.z*m.k.x,
        v.x*m.i.y + v.y*m.j.y + v.z*m.k.y,
        v.x*m.i.z + v.y*m.j.z + v.z*m.k.z,
        v.x*m.i.w + v.y*m.j.w + v.z*m.k.w + v.w);
}

inline sVector4 operator*(const sMatrix44A &m,const sVector4 &v)
{
    return sVector4(
        m.i.x*v.x + m.i.y*v.y + m.i.z*v.z + m.i.w*v.w,
        m.j.x*v.x + m.j.y*v.y + m.j.z*v.z + m.j.w*v.w,
        m.k.x*v.x + m.k.y*v.y + m.k.z*v.z + m.k.w*v.w,
        v.w);
}

inline sVector40 operator*(const sMatrix44A &m,const sVector40 &v)
{
    return sVector40(
        m.i.x*v.x + m.i.y*v.y + m.i.z*v.z,
        m.j.x*v.x + m.j.y*v.y + m.j.z*v.z,
        m.k.x*v.x + m.k.y*v.y + m.k.z*v.z);
}

inline sVector40 operator*(const sMatrix33 &m,const sVector40 &v)
{
    return sVector40(
        m.i.x*v.x + m.i.y*v.y + m.i.z*v.z,
        m.j.x*v.x + m.j.y*v.y + m.j.z*v.z,
        m.k.x*v.x + m.k.y*v.y + m.k.z*v.z);
}

inline sVector41 operator*(const sMatrix44A &m,const sVector41 &v)
{
    return sVector41(
        m.i.x*v.x + m.i.y*v.y + m.i.z*v.z + m.i.w,
        m.j.x*v.x + m.j.y*v.y + m.j.z*v.z + m.j.w,
        m.k.x*v.x + m.k.y*v.y + m.k.z*v.z + m.k.w);
}

/****************************************************************************/

sMatrix22 operator*(const sMatrix22 &a,const sMatrix22 &b);
sMatrix33 operator*(const sMatrix33 &a,const sMatrix33 &b);
sMatrix44 operator*(const sMatrix44 &a,const sMatrix44 &b);
sMatrix44 operator*(const sMatrix44A &a,const sMatrix44 &b);
sMatrix44 operator*(const sMatrix44 &a,const sMatrix44A &b);
sMatrix44A operator*(const sMatrix44A &a,const sMatrix44A &b);

sMatrix22 sMul(const sVector2 &a,const sVector2 &b);
sMatrix33 sMul(const sVector3 &a,const sVector3 &b);
sMatrix44 sMul(const sVector4 &a,const sVector4 &b);

sMatrix22  sEulerXYZ22(float a);
sMatrix33  sEulerXYZ33(float x,float y,float z);
sMatrix44  sEulerXYZ44(float x,float y,float z);
sMatrix33A sRotate33A(float a);
sMatrix22  sRotate22(float a);

sMatrix44A sEulerXYZ(float x,float y,float z);
sVector3 sFindEulerXYZ(const sMatrix44A &m);
sMatrix44A sSetSRT(const sVector41 &scale,const sVector3 &rot,const sVector41 &trans);
sMatrix44A sRotateAxis(const sVector40 &v,float a);
sMatrix44A sLook(const sVector40 &v);
sMatrix44A sLook(const sVector40 &dir,const sVector40 &up);
sMatrix44A sLookAt(const sVector41 &dest,const sVector41 &pos);
sMatrix44A sLookAt(const sVector41 &dest,const sVector41 &pos,const sVector40 &up);


inline sMatrix33  sEulerXYZ33 (const sVector3  &a) { return sEulerXYZ33 (a.x,a.y,a.z); }
inline sMatrix33  sEulerXYZ33 (const sVector40 &a) { return sEulerXYZ33 (a.x,a.y,a.z); }
inline sMatrix33  sEulerXYZ33 (const sVector41 &a) { return sEulerXYZ33 (a.x,a.y,a.z); }
inline sMatrix44  sEulerXYZ44 (const sVector3  &a) { return sEulerXYZ44 (a.x,a.y,a.z); }
inline sMatrix44  sEulerXYZ44 (const sVector40 &a) { return sEulerXYZ44 (a.x,a.y,a.z); }
inline sMatrix44  sEulerXYZ44 (const sVector41 &a) { return sEulerXYZ44 (a.x,a.y,a.z); }
inline sMatrix44A sEulerXYZ   (const sVector3  &a) { return sEulerXYZ   (a.x,a.y,a.z); }
inline sMatrix44A sEulerXYZ   (const sVector40 &a) { return sEulerXYZ   (a.x,a.y,a.z); }
inline sMatrix44A sEulerXYZ   (const sVector41 &a) { return sEulerXYZ   (a.x,a.y,a.z); }

/****************************************************************************/
/***                                                                      ***/
/***   Linear Algebra                                                     ***/
/***                                                                      ***/
/****************************************************************************/

// m.i holds the coefficiants for a.x
// m.j holds the coefficiants for a.y
// ...
// solve using cramers rule

sVector2 sSolve(const sMatrix22 &m,const sVector2 &a);
sVector3 sSolve(const sMatrix33 &m,const sVector3 &a);
sVector4 sSolve(const sMatrix44 &m,const sVector4 &a);

/****************************************************************************/
/***                                                                      ***/
/***   Lines                                                              ***/
/***                                                                      ***/
/****************************************************************************/

bool sIntersectLine(const sVector2 &a0,const sVector2 &a1,const sVector2 &b0,const sVector2 &b1,sVector2 &x,float *af=0,float *bf=0);
bool sIntersectLineSegment(const sVector2 &a0,const sVector2 &a1,const sVector2 &b0,const sVector2 &b1,sVector2 &x,float *af=0,float *bf=0);
bool sIntersect(const sVector2 &a0,const sVector2 &a1,const sVector2 &b0,const sVector2 &b1,sVector2 &x,float *af=0,float *bf=0);

inline float sDistSqToLine(const sVector2 &a,const sVector2 &b,const sVector2 &p)
{
    sVector2 ab = b - a;
    sVector2 ap = p - a;
    float absq = sDot(ab,ab);
    float dun = sCross(ap,ab);
    float dsq = dun*dun/absq;
    return dsq;
}

inline float sDistSqToLineSegment(const sVector2 &a,const sVector2 &b,const sVector2 &p)
{
    sVector2 ab = b - a;
    sVector2 ap = p - a;
    float absq = sDot(ab,ab);
    float dun = sCross(ap,ab);
    float dsq = dun*dun/absq;           // distance to line

    float t = sDot(ap,ab) / absq;   // check endpoints
    if(t<=0)
        return sDistSquared(p,a);
    else if(t>=1)
        return sDistSquared(p,b);

    return dsq;
}

/****************************************************************************/
/***                                                                      ***/
/***   Quaternions                                                        ***/
/***                                                                      ***/
/****************************************************************************/

struct sQuaternion
{
public:
  float r,i,j,k;

  sQuaternion() { i=j=k=0.0f; r=1.0f; }
  void Lerp(float fade,sQuaternion &a,sQuaternion &b);  
  void Init(const sMatrix44A &mat);
};


/****************************************************************************/
/***                                                                      ***/
/***   Axis Aligned Bounding Box                                          ***/
/***                                                                      ***/
/****************************************************************************/

struct sAABBox2
{
    sVector2 Min;
    sVector2 Max;
    bool Empty;

    sAABBox2() { Empty = 1; }
    void Clear() { Empty = 1; }
    void Add(const sVector2 &a) { if(Empty) { Empty=0;Min=Max=a; } else { Min=sMin(Min,a); Max=sMax(Max,a); } }
};

struct sAABBox3
{
    sVector3 Min;
    sVector3 Max;
    bool Empty;

    sAABBox3() { Empty = 1; }
    void Clear() { Empty = 1; }
    void Add(const sVector3 &a) { if(Empty) { Empty=0;Min=Max=a; } else { Min=sMin(Min,a); Max=sMax(Max,a); } }
};

struct sAABBox41C;

struct sAABBox41
{
    sVector41 Min;
    sVector41 Max;
    bool Empty;

    sAABBox41() { Empty = 1; }
    void Clear() { Empty = 1; }
    void Add(const sVector41 &a) { if(Empty) { Empty=0;Min=Max=a; } else { Min=sMin(Min,a); Max=sMax(Max,a); } }
    void GetCorners(sVector41 *vp) const;
    void Add(const sMatrix44A &mat,const sAABBox41 &box);
    void Add(const sMatrix44A &mat,const sAABBox41C &box);
};

struct sAABBox41C
{
    sVector41 Center;
    sVector40 Radius;

    void Clear() { Center.Set(0,0,0); Radius.Set(0,0,0); }
    sAABBox41C() {}
    sAABBox41C(const sAABBox41 &box) { Center = sAverage(box.Min,box.Max); Radius = (box.Max-box.Min)*0.5f; }
    void GetCorners(sVector41 *vp) const;
};

struct sFrustum
{
    sVector4 Planes[6];
    sVector4 AbsPlanes[6];


    void Init(const sMatrix44 &mat,float xMin,float xMax,float yMin,float yMax,float zMin,float zMax);
    inline void Init(const sMatrix44 &mat) { Init(mat,-1.0f,1.0f,-1.0f,1.0f,0.0f,1.0f); }

    // Frustum for a subset (in normalized clip coordinates)
    inline void Init(const sMatrix44 &mat,const sFRect &r,float zMin,float zMax) { Init(mat,r.x0,r.x1,r.y0,r.y1,zMin,zMax); }

    sClipTest IsInside(const sAABBox41C &b) const;
    inline sClipTest IsInside(const sAABBox41 &b) const { return IsInside(sAABBox41C(b)); }

    // Just the rejection test (fully outside yes/no)
    inline bool IsOutside(const sAABBox41C &b) const
    {
        for(int i=0;i<6;i++)
        {
            float m = sDot(b.Center , Planes[i]);
            float n = sDot(b.Radius , AbsPlanes[i]);
            if(m+n<0.0f) return 1;
        }
        return 0;
    }
    inline bool IsOutside(const sAABBox41 &b) const { return IsOutside(sAABBox41C(b)); }

    void Transform(const sFrustum &src,const sMatrix44A &mat);
};

/****************************************************************************/
/***                                                                      ***/
/***   Bezier Curves                                                      ***/
/***                                                                      ***/
/****************************************************************************/

struct sBezier2
{
    sVector2 c0,c1,c2,c3;
    sBezier2() {}
    sBezier2(const sVector2 &C0,const sVector2 &C1,const sVector2 &C2,const sVector2 &C3)
    { c0=C0; c1=C1; c2=C2; c3=C3; }

    void Divide(sBezier2 &s0,sBezier2 &s1,float t0) const;
    void Divide(sBezier2 &s0,sBezier2 &s1) const;
    bool Divide(float t);
    float FindX(float x);
    bool IsLine(float abserrorsq) const;
};

/****************************************************************************/
/***                                                                      ***/
/***   Viewport2                                                          ***/
/***                                                                      ***/
/****************************************************************************/

enum sViewportMode
{
    sVM_Perspective = 1,
    sVM_Orthogonal,
    sVM_Pixels,
    sVM_ARCam
};

struct sViewport
{
    sMatrix44A Model;
    sMatrix44A Camera;
    float ZNear;
    float ZFar;
    float ZoomX;
    float ZoomY;
    sViewportMode Mode;

    sMatrix44A WS2CS;
    sMatrix44A MS2CS;
    sMatrix44 MS2SS;
    sMatrix44 Projection;     // a.k.a. CS2SS

    sViewport();
    void Prepare(const Altona2::sTargetPara &tp);
    void Prepare(int sx,int sy);
};

/****************************************************************************/
/***                                                                      ***/
/***   Perlin Noise                                                       ***/
/***                                                                      ***/
/****************************************************************************/

// x,y = 8:16 (größere zahlen loopen)

float sPerlin2D(int x,int y,int mask=255,int seed=0);  
float sPerlin3D(int x,int y,int z,int mask=255,int seed=0);  
void sPerlinDerive3D(int x,int y,int z,int mask,int seed,float &value,sVector40 &dir);

// x,y = 0..256 (größere zahlen loopen)
// mode = 0: x
// mode = 1: abs(x)
// mode = 2: sin(x*sPI)
// mode = 3: sin(abs(x)*sPI)

float sPerlin2D(float x,float y,int octaves=1,float falloff=1.0f,int mode=0,int seed=0);

/****************************************************************************/
/***                                                                      ***/
/***   Biquad Filters                                                     ***/
/***                                                                      ***/
/****************************************************************************/

class sBiquad
{
    float a1,a2,b0,b1,b2;
    float x1,x2;
    float y1,y2;
public:
    sBiquad();
    void Reset();
    float Filter(float x);

    void LowPass(float freq,float q);
    void HighPass(float freq,float q);
    void BandPass(float freq,float q);
    void BandPassConstantPeakGain(float freq,float q);
};

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

} // namespace Altona2

#endif  // FILE_ALTONA2_LIBS_BASE_MATH_HPP


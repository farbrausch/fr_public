/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_BASE_SERIALIZE_HPP
#define FILE_ALTONA2_LIBS_BASE_SERIALIZE_HPP

#include "Altona2/Libs/Base/Machine.hpp"
#include "Altona2/Libs/Base/Containers.hpp"

#define sSerExtraChecking 0

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class sReader
{
    sptr Alloc;
    sptr EndPtr;
    int64 Left;
    sptr Used;
    sptr Limit;
    uint8 *Buffer;
    class sFile *File;
    bool Ok;

    sArray<void *> PtrMap;
    void Flush();
    void VoidPtr(void *&obj);
public:
    sReader(const char *name);
    ~sReader();
    bool IsReading() { return 1; }
    bool IsWriting() { return 0; }

    bool IsOk() { return Ok; }
    void SetError() { Ok = false; }
    bool Finish();
    int Begin(uint id,int currentversion,int minversion);
    bool End();
    void Check(int bytes) { sASSERT(Used<=Limit); if(Used+bytes>EndPtr && Left>0) Flush(); sASSERT(bytes<=Alloc); Limit=Used+bytes; }

    // basic

    template <class T> void Value(T &v)  {  if(sSerExtraChecking) sASSERT(Used+int(sizeof(T))<=EndPtr);  v = *(T*)(Buffer+Used); Used+=sizeof(T); }
    sReader& operator| (bool&v)  { uint8 vv; Value(vv); v=vv?1:0; return *this; }
    sReader& operator| (int8  &v)  { Value(v); return *this; }
    sReader& operator| (uint8  &v)  { Value(v); return *this; }
    sReader& operator| (int16 &v)  { Value(v); return *this; }
    sReader& operator| (uint16 &v)  { Value(v); return *this; }
    sReader& operator| (int &v)  { Value(v); return *this; }
    sReader& operator| (uint &v)  { Value(v); return *this; }
    sReader& operator| (float &v)  { Value(v); return *this; }
    sReader& operator| (int64 &v)  { Value(v); return *this; }
    sReader& operator| (uint64 &v)  { Value(v); return *this; }
    sReader& operator| (double &v)  { Value(v); return *this; }
    void ValueAs32(uptr &v)  { uint x; Value(x); v=uptr(x); }
    void ValueAs32(sptr &v) { int x; Value(x); v=sptr(x); }
    void ValueAs64(uptr &v)  { uint64 x; Value(x); v=uptr(x); }
    void ValueAs64(sptr &v) { int64 x; Value(x); v=sptr(x); }
    template <class T> void Enum(T &v) { uint n; Value(n); v = (T) n; }
    bool If(bool c) { uint c1; Value(c1); return c1!=0; }

    // pointers

    void RegisterPtr(void *ptr);
    template <typename T> void Ptr(T *&obj) { VoidPtr((void *&)obj); }

    // object

    template <typename T> void Obj(T *&obj) { obj=0; if(If(0)) { obj=new T; *this | *obj; } }

    // arrays

    void SmallData(uptr size,void *v);
    void LargeData(uptr size,void *v);
    void Align(int n);
    void String(char *data,uptr max);
    const char *ReadString();

    template <class Type> void Array(sArray<Type> &a) 
    { int max; Value(max); sASSERT(a.GetCount()==0); a.SetSize(max); } 
    template <class Type> void ArrayNew(sArray<Type *> &a) 
    { int max; Value(max); sASSERT(a.GetCount()==0); a.SetSize(max); 
    for(int i=0;i<max;i++) { a[i]=new Type; } }
    template <class Type> void ArrayNewRegister(sArray<Type *> &a) 
    { int max; Value(max); sASSERT(a.GetCount()==0); a.SetSize(max); 
    for(int i=0;i<max;i++) { a[i]=new Type; RegisterPtr(a[i]); } }
    template <class Type> void ArrayValue(sArray<Type> &a) 
    { int max; Value(max); sASSERT(a.GetCount()==0); a.SetSize(max); 
    for(int i=0;i<max;i++) { (*this) | a[i]; } }
    template <class Type> void ArrayPtr(sArray<Type *> &a) 
    { int max; Value(max); sASSERT(a.GetCount()==0); a.SetSize(max); 
    for(int i=0;i<max;i++) { if((i&63)==0) Check(1024); Ptr(a[i]); } }
};

/****************************************************************************/

class sWriter
{
    sptr Alloc;
    sptr Used;
    sptr Limit;
    uint8 *Buffer;
    class sFile *File;
    bool Ok;
    int ObjId;

    void Flush();
public:
    sWriter(const char *name);
    ~sWriter();
    bool IsReading() { return 0; }
    bool IsWriting() { return 1; }

    bool IsOk() { return Ok; };
    void SetError() { Ok = false; }
    bool Finish();
    int Begin(uint id,int currentversion,int minversion);
    bool End();

    // basic

    void Check(int bytes) { sASSERT(Used<=Limit); if(Used+bytes>Alloc) Flush(); sASSERT(bytes<=Alloc); Limit=Used+bytes; }
    template <class T> void Value(const T &v)  { if(sSerExtraChecking) sASSERT(Used+int(sizeof(T))<=Alloc); *(T*)(Buffer+Used) = v; Used+=sizeof(T); }
    sWriter& operator| (const bool&v)  { uint8 vv=v; Value(vv); return *this; }
    sWriter& operator| (const int8  &v)  { Value(v); return *this; }
    sWriter& operator| (const uint8  &v)  { Value(v); return *this; }
    sWriter& operator| (const int16 &v)  { Value(v); return *this; }
    sWriter& operator| (const uint16 &v)  { Value(v); return *this; }
    sWriter& operator| (const int &v)  { Value(v); return *this; }
    sWriter& operator| (const uint &v)  { Value(v); return *this; }
    sWriter& operator| (const float &v)  { Value(v); return *this; }
    sWriter& operator| (const int64 &v)  { Value(v); return *this; }
    sWriter& operator| (const uint64 &v)  { Value(v); return *this; }
    sWriter& operator| (const double &v)  { Value(v); return *this; }
    void ValueAs32(const uptr &v)  { uint x=uint(v); Value(x); }
    void ValueAs32(const sptr &v) { int x=int(v); Value(x); }
    void ValueAs64(const uptr &v)  { uint64 x=uint64(v); Value(x); }
    void ValueAs64(const sptr &v) { int64 x=int64(v); Value(x); }
    template <class T> void Enum(const T &v) { uint n = uint(v); Value(n); }
    bool If(bool c) { Value(uint(c)); return c; }

    // pointers

    template <typename T> void RegisterPtr(T *obj) { obj->SerObjId = ObjId++; }
    template <typename T> void Ptr(T *obj) { int id = (obj ? obj->SerObjId : 0); (*this)|id; }

    // object

    template <typename T> void Obj(T *&obj) { if(If(obj!=0)) { *this | *obj; } }

    // arrays

    void SmallData(uptr size,const void *v);
    void LargeData(uptr size,const void *v);
    void Align(int n);
    void String(const char *data,uptr max);
    void WriteString(const char *data);

    template <class Type> void Array(const sArray<Type> &a) 
    { Value((int)a.GetCount()); }
    template <class Type> void ArrayNew(const sArray<Type *> &a) 
    { Value((int)a.GetCount()); }
    template <class Type> void ArrayNewRegister(const sArray<Type *> &a) 
    { Value((int)a.GetCount()); for(int i=0;i<a.GetCount();i++) RegisterPtr(a[i]); }
    template <class Type> void ArrayValue(const sArray<Type> &a) 
    { Value((int)a.GetCount()); for(int i=0;i<a.GetCount();i++) (*this)|a[i]; }
    template <class Type> void ArrayPtr(const sArray<Type *> &a) 
    { Value((int)a.GetCount()); for(int i=0;i<a.GetCount();i++) { if((i&63)==0) Check(1024); Ptr(a[i]); } }
};

/****************************************************************************/

template <class T> bool sLoadObject(T *obj,const char *name)
{ sReader s(name);  s | *obj;  return s.Finish(); };

template <class T> bool sSaveObject(T *obj,const char *name)
{ sWriter s(name);  s | *obj;  return s.Finish(); };

template <class T> T *sLoadObject(const char *name)
{ T *obj = new T;  sReader s(name);  s | *obj;  if(!s.Finish()) sDelete(obj); return obj; };

template <class T> T *sSaveObject(const char *name)
{ T *obj = new T;  sWriter s(name);  s | *obj;  if(!s.Finish()) sDelete(obj); return obj; };

/****************************************************************************/

inline sReader& operator| (sReader &s,sRect &v) { v.Serialize(s); return s; }
inline sWriter& operator| (sWriter &s,sRect &v) { v.Serialize(s); return s; }
inline sReader& operator| (sReader &s,sFRect &v) { v.Serialize(s); return s; }
inline sWriter& operator| (sWriter &s,sFRect &v) { v.Serialize(s); return s; }

/****************************************************************************/

namespace sSerId
{
    enum Users
    {
        Altona          = 0x10000000,
        Free            = 0x80000000,
        Wz5             = 0x11000000,
        Desktop         = 0x12000000,
        ScreenPaint     = 0x13000000,
    };
    enum sSerId 
    {
        sImage          = Altona+0x0000,            // used for plain text
        sFImage         = Altona+0x0001,
        sPainterFont    = Altona+0x0002,
        sCharPtr        = Altona+0x0003,
        sImageData      = Altona+0x0004,
        sFMonoImage     = Altona+0x0005,
        sTextureFont    = Altona+0x0006,

        wLaunchGrid     = Desktop+0x0001,
    };
}

/****************************************************************************/

} // namespace Altona2

#endif  // FILE_ALTONA2_LIBS_BASE_SERIALIZE_HPP


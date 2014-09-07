/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_BASE_TYPES_HPP
#define FILE_ALTONA2_LIBS_BASE_TYPES_HPP

#include "Altona2/Libs/Base/Machine.hpp"
#include "Altona2/Libs/Base/Containers.hpp"

namespace Altona2 {

struct sFormatStringBuffer;

/****************************************************************************/
/***                                                                      ***/
/***   Simple Stuff                                                       ***/
/***                                                                      ***/
/****************************************************************************/

inline uint sMakeFourCC(uint8 a,uint8 b,uint8 c,uint8 d) { return a | (b<<8) | (c<<16) | (d<<24); }
inline uint sMake2(uint a,uint b)                  { return (a&0xffff) | ((b&0xffff)<<16); }
inline uint sMake3(uint a,uint b,uint c)           { return (a&0xfff ) | ((b&0xfff)<<12) | ((c&0xfff)<<24); }
inline uint sMake4(uint a,uint b,uint c,uint d)    { return (a&0xff  ) | ((b&0xff )<< 8) | ((c&0xff )<<16) | ((d&0xff)<<24); }

#define sMAKE2(a, b)            ( (uint(a)&0xffff) | ((uint(b)&0xffff)<<16) )
#define sMAKE3(a, b, c)         ( (uint(a)&0xfff ) | ((uint(b)&0xfff)<<12) | ((uint(c)&0xfff)<<24) )
#define sMAKE4(a, b, c, d)      ( (uint(a)&0xff  ) | ((uint(b)&0xff )<< 8) | ((uint(c)&0xff )<<16) | ((uint(d)&0xff)<<24) )

/****************************************************************************/
/***                                                                      ***/
/***   Hash Functions                                                     ***/
/***                                                                      ***/
/****************************************************************************/

uint sChecksumCRC32(const uint8 *data,uptr size);
uint sChecksumMurMur(const uint *data,int words);
uint sHashString(const char *string);
uint sHashString(const char *string,uptr len);

struct sMurMur
{
private:
    uint h;
public:
    void Begin();
    void Add(const uint *data,int words);
    template <class T> void Add(const T &x) { Add((const uint *)&x,sizeof(x)/4); }
    uint End();
};

struct sChecksumMD5
{
private:
    inline void f(uint &a,uint b,uint c,uint d,int k,int s,uint t,uint *data) { a += (d^(b&(c^d))) + data[k] + t;  a = ((a<<s)|(a>>(32-s))) + b; }
    inline void g(uint &a,uint b,uint c,uint d,int k,int s,uint t,uint *data) { a += (c^(d&(b^c))) + data[k] + t;  a = ((a<<s)|(a>>(32-s))) + b; }
    inline void h(uint &a,uint b,uint c,uint d,int k,int s,uint t,uint *data) { a += (b^c^d)       + data[k] + t;  a = ((a<<s)|(a>>(32-s))) + b; }
    inline void i(uint &a,uint b,uint c,uint d,int k,int s,uint t,uint *data) { a += (c^(b|~d))    + data[k] + t;  a = ((a<<s)|(a>>(32-s))) + b; }
    void Block(const uint8 *data);
public:
    uint Hash[4];

    void CalcBegin();
    uptr CalcAdd(const uint8 *data, uptr size);   // calculate 64 byte blocks, returns consumed bytes
    void CalcEnd(const uint8 *data, uptr size, uptr sizeall);

    void Calc(const uint8 *data,uptr size);
    bool Check(const uint8 *data,uptr size);
    bool operator== (const sChecksumMD5 &)const;
    bool operator!= (const sChecksumMD5 &md5)const    { return !(*this==md5); }
};

sFormatStringBuffer& operator% (sFormatStringBuffer &f, const sChecksumMD5 &md5);


/****************************************************************************/
/***                                                                      ***/
/***   Random Generators                                                  ***/
/***                                                                      ***/
/****************************************************************************/

// Linear congruential generator (mul add)

class sRandom                               
{
private:
    uint kern;
    uint Step();
public:
    sRandom() { Seed(0); }
    sRandom(uint seed) { Seed(seed); }
    void Init() { Seed(0); }
    void Seed(uint seed);
    int Int(int max);
    int Int16() { return Step()&0xffff; }
    uint Int32();
    float Float(float max);
    inline float FloatSigned(float max) { return Float(2.0f*max)-max; }
};

//  Mersenne twister (pretty good and fast)

class sRandomMT                             
{
private:
    enum Enums
    {
        Count = 624,
        Period = 397,
    };
    uint State[Count];
    int Index;

    uint Step();
    void Reload();
public:
    sRandomMT() { Seed(0); }
    sRandomMT(uint seed) { Seed(seed); }
    void Init() { Seed(0); }
    void Seed(uint seed);
    void Seed64(uint64 seed);
    int Int(int max);
    int Int16() { return Step()&0xffff; }
    uint Int32() { return Step(); }
    float Float(float max);
    inline float FloatSigned(float max) { return Float(2.0f*max)-max; }
};


//  KISS (smaller period than MT, but still long enough for games)

class sRandomKISS                           
{
private:
    uint x,y,z,c;
    uint Step();
public:
    sRandomKISS() { Seed(0); }
    sRandomKISS(uint seed) { Seed(seed); }
    void Init() { Seed(0); }
    void Seed(uint seed);
    int Int(int max);
    int Int16() { return Step()&0xffff; }
    uint Int32() { return Step(); }
    float Float(float max);
    inline float FloatSigned(float max) { return Float(2.0f*max)-max; }
};

/****************************************************************************/
/***                                                                      ***/
/***   Runtime Type Information                                           ***/
/***                                                                      ***/
/****************************************************************************/

extern int RttiAtomic;

typedef void *(*sRttiNew)();

struct sRtti
{
    const char *Name;
    const sRtti *Base;
    int Atomic;
    sRttiNew New;

    sRtti(const char *n,const sRtti *b,sRttiNew new_) { Name=n; Base=b; Atomic=++RttiAtomic; New=new_; }
    bool CanBeCastTo(const sRtti &to) const;
};

template <class T,class O>
bool sCanCast(O *obj) 
{ 
    if(obj==0)
        return false;
    auto t = T::GetRttiStatic(); 
    auto o = obj->GetRtti(); 
    while(o)
    {
        if(o==t)
            return true;
        o = o->Base;
    }
    return false;
}

template <class T,class O>
T *sCast(O *obj)
{
    sASSERT(sCanCast<T>(obj));
    return (T *) obj;
}

template <class T,class O>
T *sTryCast(O *obj)
{
    if(sCanCast<T>(obj))
        return (T *) obj;
    else
        return 0;
}

#define sRTTIBASE(name) \
static void *RttiNew() { return new name; } \
static const sRtti *GetRttiStatic() { static const sRtti r(#name,0,RttiNew); return &r; } \
virtual const sRtti *GetRtti() const { return GetRttiStatic(); } 

#define sRTTIBASE_ABSTRACT(name) \
static const sRtti *GetRttiStatic() { static const sRtti r(#name,0,0); return &r; } \
virtual const sRtti *GetRtti() const { return GetRttiStatic(); } 

#define sRTTI(name,base) \
static void *RttiNew() { return new name; } \
static const sRtti *GetRttiStatic() { static const sRtti r(#name,base::GetRttiStatic(),RttiNew); return &r; } \
virtual const sRtti *GetRtti() const { return GetRttiStatic(); } 

#define sRTTI_ABSTRACT(name,base) \
static const sRtti *GetRttiStatic() { static const sRtti r(#name,base::GetRttiStatic(),0); return &r; } \
virtual const sRtti *GetRtti() const { return GetRttiStatic(); } 

/****************************************************************************/
/***                                                                      ***/
/***   Reference Counting                                                 ***/
/***                                                                      ***/
/****************************************************************************/

// use this as a pattern when creating your own refcounted classes
// you do not need to inherit from this, just copy and paste.

class sRCObject
{
private:
    static int TotalCount;
protected:
    int RefCount;
    virtual ~sRCObject() { TotalCount--; }
public:
    sRTTIBASE(sRCObject);
    sRCObject() { RefCount = 1; TotalCount++; }
    void AddRef() { if(this) RefCount++; }
    void Release() { if(this) { RefCount--; sASSERT(RefCount>=0); if(RefCount==0) delete this; } }
    void CheckRefCount() { sASSERT(RefCount>0); }
    static int GetTotalCount() { return TotalCount; }
};

/****************************************************************************/
/***                                                                      ***/
/***   Garbage Collection (I)                                             ***/
/***                                                                      ***/
/****************************************************************************/

class sGCObject
{
private:
    static int TotalCount;
    friend class sGarbageCollector;
    int TagFlag;
protected:
    virtual ~sGCObject();
public:
    sRTTIBASE(sGCObject);
    sGCObject();
    virtual void Tag() {}
    virtual void Finalize() {}
    void Need();
    static int GetTotalCount() { return TotalCount; }
};

/****************************************************************************/
/***                                                                      ***/
/***   Single Linked List - put a "Next" member in class!                 ***/
/***                                                                      ***/
/****************************************************************************/

template<class T>
class sSListRev
{
    T *First;
public:

    sSListRev()               { First = 0; }
    void Init()               { First = 0; }
    void Add(T *e)            { e->Next = First; First = e; }
    void AddHead(T *e)        { e->Next = First; First = e; }
    T *RemHead()              { sASSERT(First); T *e = First; First = e->Next; return e; }
    bool IsEmpty() const     { return First!=0; }
    void Clear()              { First = 0; }
    T *GetFirst() const       { return First; }
    int GetCount() const     { int n=0; T *p=First; while(p) { p=p->Next; n++; } return n; }
};

template<class T>
class sSList
{
    T **NextP;
    T *First;
public:

    sSList()                  { First = 0; NextP = &First; }
    void Init()               { First = 0; NextP = &First; }
    void Add(T *e)            { *NextP = e; e->Next = 0; NextP = &e->Next; }
    void AddTail(T *e)        { *NextP = e; e->Next = 0; NextP = &e->Next; }
    void AddHead(T *e)        { e->Next = First; First = e; if(e->Next==0) { NextP = &e->Next; } }
    T *RemHead()              { sASSERT(First); T *e = First; First = e->Next; if(NextP==&e->Next) NextP=&First; return e; }
    bool IsEmpty() const     { return First!=0; }
    void Clear()              { First = 0; NextP = &First; }
    T *GetFirst() const       { return First; }
    int GetCount() const     { int n=0; T *p=First; while(p) { p=p->Next; n++; } return n; }
};

#define sFORLIST(l,p)         for((p)=(l).GetFirst();(p)!=0;(p)=(p)->Next)


/****************************************************************************/
/***                                                                      ***/
/***   Simple Hash Table                                                  ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   - does not manage memory for keys or values                        ***/
/***   - add does not check if the key already exists                     ***/
/***                                                                      ***/
/***   sHashTableBase contains the implementation                         ***/
/***                                                                      ***/
/***   Use the sHashTable template if the key has comparison operator     ***/
/***   and a Hash() member function                                       ***/
/***                                                                      ***/
/***   You can also derive the class yourself                             ***/
/***                                                                      ***/
/****************************************************************************/

// base class

class sHashTableBase
{
private:
    struct Node                     // a node in the hashtable
    {
        Node *Next;                   // linked list inside bin
        const void *Key;              // ptr to key
        void *Value;                  // ptr to value
    };

    // the hashtable

    int HashSize;                  // size of hashtable (power of two)
    int HashMask;                  // size-1;
    Node **HashTable;                // the hashtable itself

    // don't allocate nodes one-by-one!

    int NodesPerBlock;             // nodes per allocation block
    Node *CurrentNodeBlock;         // current node allocation block
    int CurrentNode;               // next free node in allocation block
    sArray<Node *> NodeBlocks;      // all allocation blocks
    Node *FreeNodes;                // list of free nodes for reuse

    // interface

    Node *AllocNode();
public:
    sHashTableBase(int size=0x4000,int nodesperblock=0x100);
    virtual ~sHashTableBase();
    void Clear();
    void ClearAndDeleteValues();
    void ClearAndDeleteKeys();
    void ClearAndDelete();

protected:
    void Add(const void *key,void *value);
    void *Find(const void *key);
    void *Rem(const void *key);
    void GetAll(sArray<void *> *a);

    virtual bool CompareKey(const void *k0,const void *k1)=0;
    virtual uint HashKey(const void *key)=0;
};


// usage

template<class KeyType,class ValueType>
class sHashTable : public sHashTableBase
{
protected:
    bool CompareKey(const void *k0,const void *k1)    { return (*(const KeyType *)k0)==(*(const KeyType *)k1); }
    uint HashKey(const void *key)                      { return ((const KeyType *)key)->Hash(); }
public:
    sHashTable(int s=0x4000,int n=0x100) : sHashTableBase(s,n)  {}
    void Add(const KeyType *key,ValueType *value)         { sHashTableBase::Add(key,value); }
    ValueType *Find(const KeyType *key)                   { return (ValueType *) sHashTableBase::Find(key); }
    ValueType *Rem(const KeyType *key)                    { return (ValueType *) sHashTableBase::Rem(key); }

    void GetAll(sArray<ValueType *> *a)                { sHashTableBase::GetAll((sArray<void *> *)a); }
};

/****************************************************************************/
/***                                                                      ***/
/***   Really Really Lame Dictionary. Please Replace with cool one...     ***/
/***                                                                      ***/
/****************************************************************************/

template<class K,class V>
class sDict
{
    struct Entry
    {
        K Key;
        V Value;
    };
    sArray<Entry> Entries;
public:
    V *Get(K key) const
    {
        Entry *e = Entries.FindValue([=](Entry &i){return i.Key==key;});
        if(e)
            return &e->Value;
        return 0;
    }

    void Add(K key,V value)
    {
        Entry *e = Entries.FindValue([=](Entry &i){return i.Key==key;});
        if(e)
        {
            e->Value = value;
            return;
        }
        e = Entries.AddMany(1);
        e->Key = key;
        e->Value = value;
    }

    void Clear()
    {
        Entries.Clear();
    }
};

template<class K,class V>
class sDictPtr
{
    struct Entry
    {
        K Key;
        V *Value;
    };
    sArray<Entry> Entries;
public:
    V *Get(K key) const
    {
        //    Entry *e = Entries.FindMember(&Entry::Key,key);
        Entry *e = Entries.FindValue([&](Entry &i){return i.Key==key;});
        if(e)
            return e->Value;
        return 0;
    }

    void Add(K key,V *value)
    {
        //    Entry *e = Entries.FindMember(&Entry::Key,key);
        Entry *e = Entries.FindValue([&](Entry &i){return i.Key==key;});
        if(e)
        {
            e->Value = value;
            return;
        }
        e = Entries.AddMany(1);
        e->Key = key;
        e->Value = value;
    }

    void Clear() { Entries.Clear(); }
    void DeleteAll() { for(int i=0;i<Entries.GetCount();i++) delete Entries[i].Value; Entries.Clear(); }
    void ReleaseAll() { for(int i=0;i<Entries.GetCount();i++) Entries[i].Value->Release(); Entries.Clear(); }

    int GetCount() { return Entries.GetCount(); }
    const V *operator[](int n) const  { sASSERT(Entries.IsIndex(n)); return Entries[n].Value; }
    V *operator[](int n) { sASSERT(Entries.IsIndex(n)); return Entries[n].Value; }
};


/****************************************************************************/
/***                                                                      ***/
/***   Rectangles (int and float)                                         ***/
/***                                                                      ***/
/****************************************************************************/

template <class Type> struct sBaseRect
{
    Type x0,y0,x1,y1;
    sBaseRect() { x0=y0=x1=y1=0; }
    sBaseRect(Type XS,Type YS) { x0=0;y0=0;x1=XS;y1=YS; }
    sBaseRect(Type X0,Type Y0,Type X1,Type Y1) { x0=X0;y0=Y0;x1=X1;y1=Y1; }
    sBaseRect(const sBaseRect &r) { *this=r; }
    template <typename T2> explicit sBaseRect(const sBaseRect<T2> &r) { x0 = Type(r.x0); y0 = Type(r.y0); x1 = Type(r.x1); y1 = Type(r.y1); }

    void Init() { x0=y0=x1=y1=0; }
    void Init(Type XS,Type YS) { x0=0;y0=0;x1=XS;y1=YS; }
    void Init(Type X0,Type Y0,Type X1,Type Y1) { x0=X0;y0=Y0;x1=X1;y1=Y1; }
    void Set() { x0=y0=x1=y1=0; }
    void Set(Type XS,Type YS) { x0=0;y0=0;x1=XS;y1=YS; }
    void Set(Type X0,Type Y0,Type X1,Type Y1) { x0=X0;y0=Y0;x1=X1;y1=Y1; }

    Type SizeX() const { return x1-x0; }
    Type SizeY() const { return y1-y0; }
    Type CenterX() const { return (x0+x1)/2; }
    Type CenterY() const { return (y0+y1)/2; }
    Type Area() const    { return SizeX()*SizeY(); }
    bool IsInside(const sBaseRect &r) const { return (r.x0<x1 && r.x1>x0 && r.y0<y1 && r.y1>y0); }   // this is partially inside r
    bool IsPartiallyInside(const sBaseRect &r) const { return (r.x0<x1 && r.x1>x0 && r.y0<y1 && r.y1>y0); }   // this is partially inside r
    bool IsCompletelyInside(const sBaseRect &r) const { return (r.x0<=x0 && r.x1>=x1 && r.y0<=y0 && r.y1>=y1); }   // this is completely inside r
    bool IsEmpty() const { return x0==x1 || y0==y1; }

    void Extend(Type e) { x0-=e;y0-=e;x1+=e;y1+=e; }
    void Extend(Type x,Type y) { x0-=x;y0-=y;x1+=x;y1+=y; }
    void Extend(const sBaseRect &r) { x0-=r.x0;y0-=r.y0;x1+=r.x1;y1+=r.y1; }
    void Trim(const sBaseRect &r) { x0+=r.x0;y0+=r.y0;x1-=r.x1;y1-=r.y1; }
    void Add(const sBaseRect &a,const sBaseRect &b) { if(b.IsEmpty()) { *this=a; } else if(a.IsEmpty()) { *this=b; } else { x0=sMin(a.x0,b.x0); y0=sMin(a.y0,b.y0); x1=sMax(a.x1,b.x1); y1=sMax(a.y1,b.y1); } } 
    void Add(const sBaseRect &a)                    { if(  IsEmpty()) { *this=a; } else if(!a.IsEmpty())                  { x0=sMin(a.x0,  x0); y0=sMin(a.y0,  y0); x1=sMax(a.x1,  x1); y1=sMax(a.y1,  y1); } } 
    void Add(int x,int y)                         { if(IsEmpty()) { Set(x,y,x+1,y+1); } else { x0=sMin(x0,x); y0=sMin(y0,y); x1=sMax(x1,x+1); y1=sMax(y1,y+1); } } 
    void And(const sBaseRect &a,const sBaseRect &b) { x0=sMax(a.x0,b.x0); y0=sMax(a.y0,b.y0); x1=sMin(a.x1,b.x1); y1=sMin(a.y1,b.y1); if(x0>=x1||y0>=y1) Init(); } 
    void And(const sBaseRect &a)                    { x0=sMax(a.x0,  x0); y0=sMax(a.y0,  y0); x1=sMin(a.x1,  x1); y1=sMin(a.y1,  y1); if(x0>=x1||y0>=y1) Init(); } 
    void Move(Type x, Type y)                       { x0+=x; x1+=x; y0+=y; y1+=y; }
    void Scale(Type s)                              { x0*=s; x1*=s; y0*=s; y1*=s; }
    void Scale(Type x, Type y)                      { x0*=x; x1*=x; y0*=y; y1*=y; }
    void ScaleCenter(float x, float y)                { Type sx = SizeX()/2; Type sy = SizeY()/2; Type cx = CenterX(); Type cy = CenterY(); x0 = Type(cx-sx*x); y0 = Type(cy-sy*y); x1 = Type(cx+sx*x); y1 = Type(cy+sy*y); }
    void ScaleCenter(float s)                        { ScaleCenter(s,s); }
    void MoveCenter(Type cx, Type cy)               { Type sx = SizeX()/2; Type sy = SizeY()/2; x0=cx-sx;x1=cx+sx;y0=cy-sy;y1=cy+sy; }
    void MoveCenterX(Type cx)                       { Type sx = SizeX()/2; x0=cx-sx;x1=cx+sx; }
    void MoveCenterY(Type cy)                       { Type sy = SizeY()/2; y0=cy-sy;y1=cy+sy; }
    void SetSize(Type sx, Type sy)                  { x1 = x0 + sx; y1 = y0 + sy; }

    int operator !=(const sBaseRect<Type> &r) const { return x0!=r.x0 || x1!=r.x1 || y0!=r.y0 || y1!=r.y1; }
    int operator ==(const sBaseRect<Type> &r) const { return x0==r.x0 && x1==r.x1 && y0==r.y0 && y1==r.y1; }
    sBaseRect<Type> & operator = (const sBaseRect<Type> &r) { x0=r.x0; x1=r.x1; y0=r.y0; y1=r.y1; return *this; }
    bool Hit(Type x,Type y) const { return x>=x0 && x<x1 && y>=y0 && y<y1; }
    void Sort() { if(x0>x1) sSwap(x0,x1); if(y0>y1) sSwap(y0,y1); }
    void Inset(Type o) { InsetX(o); InsetY(o); } 
    void Inset(const sBaseRect &r) { x0+=r.x0;y0+=r.y0;x1-=r.x1;y1-=r.y1; }
    void InsetX(Type o) { Type mx=sMin(o,SizeX()/2); x0+=mx; x1-=mx; } 
    void InsetY(Type o) { Type my=sMin(o,SizeY()/2); y0+=my; y1-=my; }

    void SwapX() { sSwap(x0, x1); }
    void SwapY() { sSwap(y0, y1); }
    void Swap() { SwapX(); SwapY(); }
    void Fix() { if (x1 < x0) SwapX(); if (y1 < y0) SwapY(); }

    void Blend(const sBaseRect &l, const sBaseRect &r, float n) { float m=1-n; x0 = l.x0 * m + r.x0 * n; x1 = l.x1 * m + r.x1 * n; y0 = l.y0 * m + r.y0 * n; y1 = l.y1 * m + r.y1 * n; }
    void Clip(const sBaseRect &c)   { x0 = sClamp(x0,c.x0,c.x1); y0 = sClamp(y0,c.y0,c.y1); x1 = sClamp(x1,c.x0,c.x1); y1 = sClamp(y1,c.y0,c.y1); }
    void Div(Type x, Type y) { x0 /= x; y0 /= y; x1 /= x; y1 /= y; }

    template <class Ser> void Serialize(Ser &s)
    { s | x0 | y0 | x1 | y1; }
};

typedef sBaseRect<int> sRect;
typedef sBaseRect<float> sFRect;

/****************************************************************************/
/***                                                                      ***/
/***   Colors                                                             ***/
/***                                                                      ***/
/****************************************************************************/

uint sScaleColor(uint a,int scale);          // a*b, scale = 0..0x10000...
uint sScaleColorFast(uint a,int scale);    // a*scale, scale = 0..0x100
uint sMulColor(uint a,uint b);              // a*b
uint sAddColor(uint a,uint b);              // a+b
uint sSubColor(uint a,uint b);              // a-b
uint sMadColor(uint mul1,uint mul2,uint add); // mul1*mul2+add
uint sFadeColor(int fade,uint a,uint b);   // fade=0..0x10000

uint sColorFade (uint a, uint b, float f);   // fade=0.0f..1.0f
uint sAlphaFade (uint c, float f);           // fade=0.0f..1.0f
uint sPMAlphaFade (uint c, float f);         // fade=0.0f..1.0f
float sScaleFade(float a, float b, float f);    // fade=0.0f..1.0f

/****************************************************************************/
/***                                                                      ***/
/***   Delegates and Hooks                                                ***/
/***                                                                      ***/
/****************************************************************************/

struct sDelegateDummy
{
};

template <typename R,typename A0,typename A1,typename A2>
class sDelegateBase
{
protected:
    typedef R (sDelegateDummy::*DType)();
    typedef R (*FType)();
    union
    {
        //    DType DPtr;
        R (sDelegateDummy::*DPtr)();
        R (sDelegateDummy::*DPtr1)(A0);
        R (sDelegateDummy::*DPtr2)(A0,A1);
        R (sDelegateDummy::*DPtr3)(A0,A1,A2);

        //    FType FPtr;
        R (*FPtr)();
        R (*FPtr1)(A0);
        R (*FPtr2)(A0,A1);
        R (*FPtr3)(A0,A1,A2);
    };
    sDelegateDummy *DThis;
public:
    sDelegateBase() { DPtr = 0; DThis=0; }
    sDelegateBase(R (*fp)()) { DThis = 0; FPtr=(FType)fp; }
    template<class T>
    sDelegateBase(T *t,R (T::*p)()) { DThis = (sDelegateDummy *)t; DPtr=(DType)p; }
    template<class T>
    sDelegateBase(T *t,R (T::*p)(A0)) { DThis = (sDelegateDummy *)t; DPtr=(DType)p; }
    template<class T>
    sDelegateBase(T *t,R (T::*p)(A0,A1)) { DThis = (sDelegateDummy *)t; DPtr=(DType)p; }
    template<class T>
    sDelegateBase(T *t,R (T::*p)(A0,A1,A2)) { DThis = (sDelegateDummy *)t; DPtr=(DType)p; }

    bool IsEmpty() { return DPtr==0; }

    bool operator!=(const sDelegateBase &a)const { return DPtr!=a.DPtr || DThis!=a.DThis; }
    bool operator==(const sDelegateBase &a)const { return DPtr==a.DPtr && DThis==a.DThis; }
};

template <typename R>
class sDelegate : public sDelegateBase<R,int,int,int>
{
public:
    sDelegate() : sDelegateBase<R,int,int,int>() {}
    sDelegate(R (*fp)()) : sDelegateBase<R,int,int,int>(fp) {}
    template<class T> 
    sDelegate(T *t,R (T::*p)()) : sDelegateBase<R,int,int,int>(t,p) {}
    R Call() const { return this->DThis ? (this->DThis->*this->DPtr)() : (*this->FPtr)(); }
};

template <typename R,typename A0>
class sDelegate1 : public sDelegateBase<R,A0,int,int>
{
public:
    sDelegate1() : sDelegateBase<R,A0,int,int>() {}
    sDelegate1(R (*fp)()) : sDelegateBase<R,A0,int,int>(fp) {}
    template<class T> 
    sDelegate1(T *t,R (T::*p)(A0)) : sDelegateBase<R,A0,int,int>(t,p) {}
    R Call(A0 a0) const { return this->DThis ? (this->DThis->*this->DPtr1)(a0) : (*this->FPtr1)(a0); }
};

template <typename R,typename A0,typename A1>
class sDelegate2 : public sDelegateBase<R,A0,A1,int>
{
public:
    sDelegate2() : sDelegateBase<R,A0,A1,int>() {}
    sDelegate2(R (*fp)()) : sDelegateBase<R,A0,A1,int>(fp) {}
    template<class T> 
    sDelegate2(T *t,R (T::*p)(A0,A1)) : sDelegateBase<R,A0,A1,int>(t,p) {}
    R Call(A0 a0,A1 a1) const { return  this->DThis ? (this->DThis->*this->DPtr2)(a0,a1) : (*this->FPtr2)(a0,a1); }
};

template <typename R,typename A0,typename A1,typename A2>
class sDelegate3 : public sDelegateBase<R,A0,A1,A2>
{
public:
    sDelegate3() : sDelegateBase<R,A0,A1,A2>() {}
    sDelegate3(R (*fp)()) : sDelegateBase<R,A0,A1,A2>(fp) {}
    template<class T> 
    sDelegate3(T *t,R (T::*p)(A0,A1,A2)) : sDelegateBase<R,A0,A1,A2>(t,p) {}
    R Call(A0 a0,A1 a1,A2 a2) const { return  this->DThis ? (this->DThis->*this->DPtr3)(a0,a1,a2) : (*this->FPtr3)(a0,a1,a2); }
};

/****************************************************************************/

template <class D>
class sHookBase 
{
protected:
    sArray<D> Hooks;
public:
    void Add(D d) { Hooks.Add(d); }
    void Rem(D d) { Hooks.Rem(d); }
    void Clear() { Hooks.Clear(); }
    void FreeMemory() { Hooks.FreeMemory(); }
};

class sHook : public sHookBase<sDelegate<void> >
{
public:
    void Call() const { for(int i=0;i<Hooks.GetCount();i++) Hooks[i].Call(); }
};

template <typename A0>
class sHook1 : public sHookBase<sDelegate1<void,A0> >
{
public:
    void Call(A0 a0) const { for(int i=0;i<this->Hooks.GetCount();i++) this->Hooks[i].Call(a0); }
};

template <typename A0,typename A1>
class sHook2 : public sHookBase<sDelegate2<void,A0,A1> >
{
public:
    void Call(A0 a0,A1 a1) const { for(int i=0;i<this->Hooks.GetCount();i++) this->Hooks[i].Call(a0,a1); }
};

/****************************************************************************/
/***                                                                      ***/
/***   Garbage Collection (II)                                            ***/
/***                                                                      ***/
/****************************************************************************/

class sGarbageCollector
{
    friend class sGCObject;
    sArray<sGCObject *> Objects;
    sArray<sGCObject *> ObjectsToDelete;
    sArray<sGCObject *> Roots;
    uint TriggerTime;

    void CollectRecursion();
public:
    sGarbageCollector();
    ~sGarbageCollector();

    void AddObject(sGCObject *);
    void AddRoot(sGCObject *);
    void RemRoot(sGCObject *);
    void CollectNow();
    void Trigger();
    void TriggerNow();
    void CollectIfTriggered();
    sHook TagHook;

    int GetObjectCount() { return Objects.GetCount(); }
    sArray<sGCObject *> ObjectStack[2];
};

extern sGarbageCollector *sGC;

/****************************************************************************/
/****************************************************************************/

} // namespace Altona2

#endif  // FILE_ALTONA2_LIBS_BASE_TYPES_HPP

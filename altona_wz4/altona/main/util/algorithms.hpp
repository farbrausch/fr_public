/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_UTIL_ALGORITHMS_HPP
#define FILE_UTIL_ALGORITHMS_HPP

#include "base/types.hpp"

/****************************************************************************/

/****************************************************************************/
/***                                                                      ***/
/***   Range utilities                                                    ***/
/***                                                                      ***/
/****************************************************************************/

template<typename Range>
sINLINE typename Range::ValueType &sPopHead(Range &r) { typename Range::ValueType &item = r.GetHead(); r.RemHead(); return item; }
template<typename Range>
sINLINE typename Range::ValueType &sPopTail(Range &r) { typename Range::ValueType &item = r.GetTail(); r.RemTail(); return item; }

/****************************************************************************/
/***                                                                      ***/
/***   Array Range definition                                             ***/
/***                                                                      ***/
/****************************************************************************/

// This is a random-access range for arrays
template<typename T>
class sArrayRange
{
  T *Begin,*End;  
public:
  typedef sArrayRange<T> ThisType;

  typedef T   ValueType;
  typedef T*  DirectType; // if in doubt, use ThisTyp==DirectType and return *this in GetDirect()

  enum { IsRandomAccess = 1 };

  sArrayRange() : Begin(0), End(0)                    {} // empty range
  sArrayRange(T *beg,T *end) : Begin(beg), End(end)   {}

  sINLINE sBool IsEmpty() const                       { return Begin == End; }
  sINLINE sInt GetCount() const                       { return (sInt) (End - Begin); }

  sINLINE T &GetHead()                                { return *Begin; }
  sINLINE T &GetTail()                                { return *(End - 1); }

  sINLINE T &operator[](sInt pos) const               { return Begin[pos]; }
  sINLINE sBool operator==(const sArrayRange &x)      { return Begin==x.Begin && End==x.End; }
  sINLINE sBool operator!=(const sArrayRange &x)      { return Begin!=x.Begin || End!=x.End; }

  sINLINE void RemHead()                              { ++Begin; }
  sINLINE void RemTail()                              { --End; }

  sINLINE ThisType Slice(sInt first,sInt last) const  { return ThisType(Begin+first,Begin+last); }

  sINLINE DirectType GetDirect() const                { return Begin; }

  // Swap isn't part of the range definition, but always good to have.
  sINLINE void Swap(sArrayRange<T> &b)                { sSwap(Begin,b.Begin); sSwap(End,b.End); }
};

// corresponding specialization of sSwap
template<typename T>
void sSwap(sArrayRange<T> &a,sArrayRange<T> &b)
{
  a.Swap(b);
}
  
namespace sAlgoImpl
{
  template<typename T>
  struct GetAll
  {
    /* No impl. of Get, T is not an array type! */
  };

  template<typename T,sInt N>
  struct GetAll<T[N]>
  {
    typedef T ValueType;
    static sINLINE sArrayRange<T> Get(T x[N])             { return sArrayRange<T>(x,x+N); }
    static sINLINE sArrayRange<const T> Get(const T x[N]) { return sArrayRange<T>(x,x+N); }
  };
}

// sAll returns the whole range corresponding to an array. This is meant to
// work plain C-arrays as well as sArray and all variants thereof.
template<typename T>
sINLINE sArrayRange<typename sAlgoImpl::GetAll<T>::ValueType> sAll(T &x) { return sAlgoImpl::GetAll<T>::Get(x); }

template<typename T>
sINLINE sArrayRange<const typename sAlgoImpl::GetAll<T>::ValueType> sAll(const T &x) { return sAlgoImpl::GetAll<T>::Get(x); }

template<template<typename> class ArrayType, typename T>
sINLINE sArrayRange<T> sAll(ArrayType<T> &x) { T *ptr = x.GetData(); return sArrayRange<T>(ptr,ptr+x.GetCount()); }

template<template<typename> class ArrayType, typename T>
sINLINE sArrayRange<const T> sAll(const ArrayType<T> &x) { const T *ptr = x.GetData(); return sArrayRange<const T>(ptr,ptr+x.GetCount()); }

template<template<typename,int> class ArrayType, typename T, int size>
sINLINE sArrayRange<T> sAll(ArrayType<T,size> &x) { T *ptr = x.GetData(); return sArrayRange<T>(ptr,ptr+x.GetCount()); }

template<template<typename,int> class ArrayType, typename T, int size>
sINLINE sArrayRange<const T> sAll(const ArrayType<T,size> &x) { const T *ptr = x.GetData(); return sArrayRange<const T>(ptr,ptr+x.GetCount()); }


//
// this needs clearance from the tech-team
//


    // sAllMax returns the whole range corresponding to an array -including nonvalid but reserved data-. This is meant to
    // work plain C-arrays as well as sArray and all variants thereof.
    template<typename T>
    sINLINE sArrayRange<typename sAlgoImpl::GetAll<T>::ValueType> sAllMax(T &x) { return sAlgoImpl::GetAll<T>::Get(x); }

    template<typename T>
    sINLINE sArrayRange<const typename sAlgoImpl::GetAll<T>::ValueType> sAllMax(const T &x) { return sAlgoImpl::GetAll<T>::Get(x); }

    template<template<typename> class ArrayType, typename T>
    sINLINE sArrayRange<T> sAllMax(ArrayType<T> &x) { T *ptr = x.GetData(); return sArrayRange<T>(ptr,ptr+x.GetSize()); }

    template<template<typename> class ArrayType, typename T>
    sINLINE sArrayRange<const T> sAllMax(const ArrayType<T> &x) { const T *ptr = x.GetData(); return sArrayRange<const T>(ptr,ptr+x.GetSize()); }

    template<template<typename,int> class ArrayType, typename T, int size>
    sINLINE sArrayRange<T> sAllMax(ArrayType<T,size> &x) { T *ptr = x.GetData(); return sArrayRange<T>(ptr,ptr+x.GetSize()); }

    template<template<typename,int> class ArrayType, typename T, int size>
    sINLINE sArrayRange<const T> sAllMax(const ArrayType<T,size> &x) { const T *ptr = x.GetData(); return sArrayRange<const T>(ptr,ptr+x.GetSize()); }


    // if you might need generic resizing....
    template<typename T>
    sINLINE sArrayRange<typename sAlgoImpl::GetAll<T>::ValueType> sAllResize(T &x, sInt newCount) { return sAlgoImpl::GetAll<T>::Get(x); }

    template<template<typename> class ArrayType, typename T>
    sINLINE sArrayRange<T> sAllResize(ArrayType<T> &x, sInt newCount)      { x.Resize(newCount); T *ptr = x.GetData(); return sArrayRange<T>(ptr,ptr+x.GetSize()); }

    template<template<typename,int> class ArrayType, typename T, int size>
    sINLINE sArrayRange<T> sAllResize(ArrayType<T,size> &x, sInt newCount) { x.Resize(newCount); T *ptr = x.GetData(); return sArrayRange<T>(ptr,ptr+x.GetSize()); }

    template<typename T>
    struct sArrayInfo
    {
      sArrayRange<T> CurrentContent, MaxContent;
    };

//
// end - this needs clearance from the tech-team
//


/****************************************************************************/
/***                                                                      ***/
/***   Doubly-linked list range                                           ***/
/***                                                                      ***/
/****************************************************************************/

// This is a bidirectional range for lists
template<typename T,sDNode T::*Offset>
class sDListRange
{
  T *Begin,*End;
  typedef sArrayRange<T> ThisType;

  sINLINE const sDNode *GetNode(const T *e) const     { return &(e->*Offset); }
  sINLINE const T *GetType(const sDNode *n) const     { return (const T*) (((sU8*)n)-sOFFSETOMP((T*)0,Offset)); }    

  sINLINE sDNode *GetNode(T *e)                       { return &(e->*Offset); }
  sINLINE T *GetType(sDNode *n)                       { return (T*) (((sU8*)n)-sOFFSETOMP((T*)0,Offset)); }

public:
  typedef T   ValueType;

  enum { IsRandomAccess = 0 };

  sDListRange() : Begin(0), End(0)                    {} // empty range
  sDListRange(T *beg,T *end) : Begin(beg), End(end)   {} // beg inclusive, end exclusive
  sDListRange(sDList<T,Offset> &x)                    { if(x.IsEmpty()) Begin = End = 0; else { Begin = x.GetHead(); End = x.GetNext(x.GetTail()); } }

  sINLINE sBool IsEmpty() const                       { return Begin == End; }

  sINLINE T &GetHead()                                { return *Begin; }
  sINLINE T &GetTail()                                { return *GetType(GetNode(End)->Prev); }

  sINLINE sBool operator==(const sDListRange &x)      { return Begin==x.Begin && End==x.End; }
  sINLINE sBool operator!=(const sDListRange &x)      { return Begin!=x.Begin || End!=x.End; }

  sINLINE void RemHead()                              { Begin = GetType(GetNode(Begin)->Next); }
  sINLINE void RemTail()                              { End = GetType(GetNode(End)->Prev); }
};

template<typename Type,sDNode Type::*Offset>
sINLINE sDListRange<Type,Offset> sAll(sDList<Type,Offset> &x) { return sDListRange<Type,Offset>(x); }

/****************************************************************************/
/***                                                                      ***/
/***  Reverse range adapter                                               ***/
/***                                                                      ***/
/****************************************************************************/

namespace sAlgoImpl
{
  // Reverse range for bidirectional ranges
  template<typename Range,sBool RandomAccess>
  class ReverseRange
  {
  protected:
    Range Host;

  public:
    typedef typename Range::ValueType ValueType;
    enum { IsRandomAccess = 0 };

    ReverseRange(const Range &host) : Host(host)      {}

    sINLINE sBool IsEmpty() const                     { return Host.IsEmpty(); }

    sINLINE ValueType &GetHead()                      { return Host.GetTail(); }
    sINLINE ValueType &GetTail()                      { return Host.GetHead(); }

    sINLINE sBool operator==(const ReverseRange<Range,RandomAccess> &x) { return Host == x.Host; }
    sINLINE sBool operator!=(const ReverseRange<Range,RandomAccess> &x) { return Host != x.Host; }

    sINLINE void RemHead()                            { Host.RemTail(); }
    sINLINE void RemTail()                            { Host.RemHead(); }
  };

  // Reverse range for random-access ranges
  template<typename Range>
  class ReverseRange<Range,sTRUE> : public ReverseRange<Range,sFALSE>
  {
    typedef ReverseRange<Range,sTRUE> ThisType;

  public:
    typedef ThisType DirectType;
    typedef typename Range::ValueType ValueType;
    enum { IsRandomAccess = 1 };

    ReverseRange(const Range &host) : ReverseRange<Range,sFALSE>(host) {}

    sINLINE sInt GetCount() const                       { return ReverseRange<Range,sFALSE>::Host.GetCount(); }

    sINLINE ValueType &operator[](sInt pos) const       { return ReverseRange<Range,sFALSE>::Host[ReverseRange<Range,sFALSE>::Host.GetCount() - 1 - pos]; }
    sINLINE ThisType Slice(sInt first,sInt last) const  { sInt cnt = ReverseRange<Range,sFALSE>::Host.GetCount(); return ReverseRange(ReverseRange<Range,sFALSE>::Host.Slice(cnt-1-last,cnt-1-first)); }

    sINLINE DirectType GetDirect() const                { return *this; }
  };
}

template<typename Range>
sAlgoImpl::ReverseRange<Range,Range::IsRandomAccess> sReverse(const Range &rng)
{
  return sAlgoImpl::ReverseRange<Range,Range::IsRandomAccess>(rng);
}

/****************************************************************************/
/***                                                                      ***/
/***  Foreach macro                                                       ***/
/***                                                                      ***/
/****************************************************************************/

namespace sAlgoImpl
{
  // "Auto any" type
  struct AutoAnyBase {};

  template<typename T>
  struct AutoAny : public AutoAnyBase
  {
    AutoAny(const T &t) : Val(t) {}
    mutable T Val;
  };

  template<typename T>
  sINLINE AutoAny<T> WrapAny(const T &x) { return x; }

  // Type encoding
  template<typename T>
  struct WrappedType {};

  template<typename T>
  WrappedType<T> WrapType(const T &t) { return WrappedType<T>(); }

  struct AnyType
  {
    template<typename T>
    operator WrappedType<T>() const { return WrappedType<T>(); }
  };

  // Range accessors
  template<typename T>
  sINLINE sBool IsEmpty(const AutoAnyBase &range,WrappedType<T>)
  {
    return ((const AutoAny<T> &) range).Val.IsEmpty();
  }

  template<typename T>
  sINLINE typename T::ValueType &GetHead(const AutoAnyBase &range,WrappedType<T>)
  {
    return ((const AutoAny<T> &) range).Val.GetHead();
  }

  template<typename T>
  sINLINE void RemHead(const AutoAnyBase &range,WrappedType<T>)
  {
    ((const AutoAny<T> &) range).Val.RemHead();
  }

  // Reference->pointer conversion
  template<typename T>
  sINLINE T *ToPtr(T &x) { return &x; }

  template<typename T>
  sINLINE T *ToPtr(T *&x) { return x; }
}

#define sWRAPTYPE(expr) (1 ? sAlgoImpl::AnyType() : sAlgoImpl::WrapType(expr))

// sFOREACH works with *any forward range type whatsoever*.
#define sFOREACH(range,val) \
  for(const sAlgoImpl::AutoAnyBase &ir=sAlgoImpl::WrapAny(range); \
    !sAlgoImpl::IsEmpty(ir,sWRAPTYPE(range)) ? (val)=sAlgoImpl::ToPtr(sAlgoImpl::GetHead(ir,sWRAPTYPE(range))), 1 : 0; \
    sAlgoImpl::RemHead(ir,sWRAPTYPE(range)))

/****************************************************************************/
/***                                                                      ***/
/***   Default ordering predicates                                        ***/
/***                                                                      ***/
/****************************************************************************/

// Less and greater (straightforward).
template<typename T> struct sCmpLess    { sINLINE sBool operator()(const T &a,const T &b) const { return a<b; } };
template<typename T> struct sCmpGreater { sINLINE sBool operator()(const T &a,const T &b) const { return b<a; } };

// Less and greater of a designated member (key): Implementation helpers
namespace sAlgoImpl
{
  template<typename T,typename KeyType>
  struct MemberLess
  {
    KeyType T::*Key;
    sINLINE MemberLess(KeyType T::*key) : Key(key)                    {}
    sINLINE sBool operator()(const T &a,const T &b) const             { return a.*Key < b.*Key; }
    sINLINE sBool operator()(const T &a,const KeyType &b) const       { return a.*Key < b; }
    sINLINE sBool operator()(const KeyType &a,const T &b) const       { return a < b.*Key; }
    sINLINE sBool operator()(const KeyType &a,const KeyType &b) const { return a < b; }
  };

  template<typename T,typename KeyType>
  struct MemberGreater
  {
    KeyType T::*Key;
    sINLINE MemberGreater(KeyType T::*key) : Key(key)                 {}
    sINLINE sBool operator()(const T &a,const T &b) const             { return b.*Key < a.*Key; }
    sINLINE sBool operator()(const T &a,const KeyType &b) const       { return b < a.*Key; }
    sINLINE sBool operator()(const KeyType &a,const T &b) const       { return b.*Key < a; }
    sINLINE sBool operator()(const KeyType &a,const KeyType &b) const { return b < a; }
  };

  // Specializations for when the comparands are pointers
  template<typename T,typename KeyType>
  struct MemberPtrLess
  {
    KeyType T::*Key;
    sINLINE MemberPtrLess(KeyType T::*key) : Key(key)                 {}
    sINLINE sBool operator()(const T *a,const T *b) const             { return a->*Key < b->*Key; }
    sINLINE sBool operator()(const T *a,const KeyType &b) const       { return a->*Key < b; }
    sINLINE sBool operator()(const KeyType &a,const T *b) const       { return a < b->*Key; }
    sINLINE sBool operator()(const KeyType &a,const KeyType &b)       { return a < b; }
  };

  template<typename T,typename KeyType>
  struct MemberPtrGreater
  {
    KeyType T::*Key;
    sINLINE MemberPtrGreater(KeyType T::*key) : Key(key)              {}
    sINLINE sBool operator()(const T *a,const T *b) const             { return b->*Key < a->*Key; }
    sINLINE sBool operator()(const T *a,const KeyType &b) const       { return b < a->*Key; }
    sINLINE sBool operator()(const KeyType &a,const T *b) const       { return b->*Key < a; }
    sINLINE sBool operator()(const KeyType &a,const KeyType &b) const { return b < a; }
  };
}

// Less and greater of a designated member (key)
template<typename T,typename KeyType>
sAlgoImpl::MemberLess<T,KeyType> sMemberLess(KeyType T::*key)           { return sAlgoImpl::MemberLess<T,KeyType>(key); } 

template<typename T,typename KeyType>
sAlgoImpl::MemberGreater<T,KeyType> sMemberGreater(KeyType T::*key)     { return sAlgoImpl::MemberGreater<T,KeyType>(key); } 

// Less and greater of a designated member (key) in when the comparands are pointers
template<typename T,typename KeyType>
sAlgoImpl::MemberPtrLess<T,KeyType> sMemPtrLess(KeyType T::*key)        { return sAlgoImpl::MemberPtrLess<T,KeyType>(key); } 

template<typename T,typename KeyType>
sAlgoImpl::MemberPtrGreater<T,KeyType> sMemPtrGreater(KeyType T::*key)  { return sAlgoImpl::MemberPtrGreater<T,KeyType>(key); } 

/****************************************************************************/
/***                                                                      ***/
/***   Binary Heaps                                                       ***/
/***                                                                      ***/
/****************************************************************************/

template<typename Range,typename Pred=sCmpLess<typename Range::ValueType> >
class sBinaryHeap
{
  Range Store;
  sInt Length;
  Pred Less;

  typedef typename Range::ValueType ValueType;

public:
  // Turns the given range into a (max)-Heap.
  sBinaryHeap(const Range &range,const Pred &less=Pred(),sInt size=-1);

  // Changes the storage for this heap. Needed after e.g. an array resize.
  // The data needs to be there already, this is just to complete the relocation.
  void ChangeStore(const Range &range)    { Store = range; }

  // Returns the current length of the heap
  sInt GetLength() const                  { return Length; }

  // Returns the top (largest) element of the heap
  ValueType Top() const                   { sVERIFY(Length > 0); return Store[0]; }

  // Pops the top (largest) element
  void Pop();

  // Pop the largest N elements, put them in increasing order and return the corresponding range.
  // If there's less than N elements in the heap, return however many are there.
  Range Pop(sInt N);
};

// siftdown for heap operations. we can't rely on compiler inlining, because
// we'd have dismal performance for basic operations in debug builds.
#define SIFTDOWN(arr,start,count,less)                    \
  {                                                       \
    sInt root = start, child;                             \
    while((child=root*2+1) < count)                       \
    {                                                     \
      if(child<count-1 && less(arr[child],arr[child+1]))  \
        child++;                                          \
      if(less(arr[root],arr[child]))                      \
        sSwap(arr[root],arr[child]), root=child;          \
      else                                                \
        break;                                            \
    }                                                     \
  }

template<typename Range,typename Pred>
sBinaryHeap<Range,Pred>::sBinaryHeap(const Range &range,const Pred &less,sInt size)
  : Store(range), Less(less)
{
  sVERIFYSTATIC(Range::IsRandomAccess);
  sInt count = range.GetCount();
  if(size == -1)
    size = count;

  typename Range::DirectType r = range.GetDirect();
  Length = count = sMin(count,size);
  for(sInt i=count >> 1; --i >= 0;)
    SIFTDOWN(r,i,count,Less)
}

template<typename Range,typename Pred>
void sBinaryHeap<Range,Pred>::Pop()
{
  sVERIFY(Length > 0);
  sInt remCount = --Length; // length after pop

  if(remCount > 0)
    sSwap(Store.GetHead(),Store[remCount]); // replace top with range[remCount]

  // then restore heap property
  SIFTDOWN(Store,0,remCount,Less);
}

template<typename Range,typename Pred>
Range sBinaryHeap<Range,Pred>::Pop(sInt N)
{
  sVERIFY(N >= 0);
  
  Range range = Store;      // copy so it's in a local var (=quicker)
  sInt count = Length;
  N = sMin(N,count);
  
  sInt newCount = count-N;
  Range result = range.Slice(newCount,Length);
  typename Range::DirectType r = range.GetDirect();

  while(count > newCount)
  {
    sSwap(r[0],r[--count]);       // move largest to end
    SIFTDOWN(r,0,count,Less);     // restore heap property
  }

  Length = newCount;
  return result;
}

/****************************************************************************/
/***                                                                      ***/
/***   Heapsort                                                           ***/
/***                                                                      ***/
/****************************************************************************/

// Default implementation of Heapsort with an ordering predicate, using sBinaryHeap.
template<typename Range,typename Pred>
void sHeapSort(const Range &r,const Pred &pred)
{
  sBinaryHeap<Range,Pred> heap(r,pred); // build the heap
  heap.Pop(r.GetCount());               // pop off all elements in order
}

// Heapsort with the default less-than ordering. Inlined so we avoid the extra
// indirection involved in an ordering predicate, which severaly hurts performance
// in debug builds.
template<typename Range>
void sHeapSort(const Range &range)
{
#define LESS(a,b) a<b

  sInt count = range.GetCount();
  typename Range::DirectType r = range.GetDirect();
  for(sInt i=count >> 1; --i >= 0;)
    SIFTDOWN(r,i,count,LESS)

  while(--count > 0)
  {
    sSwap(r[0],r[count]);
    SIFTDOWN(r,0,count,LESS)
  }

#undef LESS
}

#undef SIFTDOWN

/****************************************************************************/
/***                                                                      ***/
/***   Some insertion sorts. Great for small arrays.                      ***/
/***                                                                      ***/
/****************************************************************************/

// Insertion sort.
template<typename Range,typename Pred>
void sInsertionSort(const Range &range,const Pred &less)
{
  sInt count = range.GetCount();
  typename Range::DirectType r = range.GetDirect();

  for(sInt i=1;i<count;i++)
  {
    if(less(r[i],r[i-1]))
    {
      typename Range::ValueType v = r[i];
      sInt j = i;

      do r[j] = r[j-1], j--; while(j>0 && less(v,r[j-1]));
      r[j] = v;
    }
  }
}

template<typename Range>
void sInsertionSort(const Range &range)
{
  sInt count = range.GetCount();
  typename Range::DirectType r = range.GetDirect();

  for(sInt i=1;i<count;i++)
  {
    if(r[i]<r[i-1])
    {
      typename Range::ValueType v = r[i];
      sInt j = i;

      do r[j] = r[j-1], j--; while(j>0 && v<r[j-1]);
      r[j] = v;
    }
  }
}

// Median of 3.
template<typename T,typename Pred>
T sMedianOf3(const T &a,const T &b,const T &c,const Pred &less)
{
  // based on optimal sort tree for 3 elements
  if(less(b,a))
  {
    if(less(c,a))
      return less(b,c) ? c : b;
    else
      return a;
  }
  else
    return (less(c,b) || less(a,c)) ? b : a;
}

template<typename T>
T sMedianOf3(const T &a,const T &b,const T &c)
{
  // based on optimal sort tree for 3 elements
  if(b<a)
  {
    if(c<a)
      return (b<c) ? c : b;
    else
      return a;
  }
  else
    return (c<b || a<c) ? b : a;
}

/****************************************************************************/
/***                                                                      ***/
/***   Introsort. Fast and has O(n log n) guarantee, but lots of code.    ***/
/***                                                                      ***/
/****************************************************************************/

namespace sAlgoImpl
{
  // The insertion sort threshold is somewhat tricky. You can use 32
  // and get notable gains as long as you're sorting integers - that's
  // what MSVC std::sort does. But I'd rather play it a bit safer in
  // case our user has more expensive comparision functions.
  // UPDATE: After some experimenting with nontrivial compares, 32 *does*
  // in fact seem like a reasonable value. Weird. That's a lot larger
  // than I would've expected.
  static const sInt ISortThreshold = 32;

  // Introsort implementation with an ordering predicate.
  // References:
  // Musser:  "Introspective sorting and selection algorithms", Software
  //   Practice and Experience, Vol. 27(8), pp. 983-993 (Aug 1997)
  template<typename Range,typename Pred>
  void IntroSortR(const Range &range,sInt cutoff,const Pred &less)
  {
    sInt count = range.GetCount();
    typename Range::DirectType r = range.GetDirect();

    if(count > ISortThreshold)
    {
      if(cutoff > 0) // use quicksort
      {
        // choose pivot (median of 3)
        typename Range::ValueType pivot = sMedianOf3(r[0],r[count>>1],r[count-1],less);

        // hoare partition algorithm
        // invariant:
        //  |  <=  |  ???  | >= |
        //  0      i       j    n
        sInt i = 0, j = count;

        for(;;)
        {
          while(less(r[i],pivot)) i++;
          do --j; while(less(pivot,r[j]));
          if(i>=j) break;
          sSwap(r[i++],r[j]);
        }

        // "fit pivot": peel off equal elements
        while(j>=0 && !less(r[j],pivot)) j--;
        while(i<count && !less(pivot,r[i])) i++;

        // allow roughly 1.5 log2(N) recursions before switching over to heapsort.
        cutoff >>= 1;
        cutoff += cutoff >> 2;

        // recurse
        sInt n;
        if((n = j+1) > 1)     IntroSortR(range.Slice(0,n),cutoff,less);
        if((n = count-i) > 1) IntroSortR(range.Slice(i,count),cutoff,less);
      }
      else // using too many recursions, try and finish it off with heapsort instead
        sHeapSort(range,less);
    }
    else if(count > 1) // small range, use insertion sort
      sInsertionSort(range,less);
  }

  // Introsort implementation using direct less-than.
  template<typename Range>
  void IntroSortR(const Range &range,sInt cutoff)
  {
    sInt count = range.GetCount();
    typename Range::DirectType r = range.GetDirect();

    if(count > ISortThreshold)
    {
      if(cutoff > 0) // use quicksort
      {
        // choose pivot (median of 3)
        typename Range::ValueType pivot = sMedianOf3(r[0],r[count>>1],r[count-1]);

        // hoare partition algorithm
        // invariant:
        //  |  <=  |  ???  | >= |
        //  0      i       j    n
        sInt i = 0, j = count;

        for(;;)
        {
          while(r[i]<pivot) i++;
          do --j; while(pivot<r[j]);
          if(i>=j) break;
          sSwap(r[i++],r[j]);
        }

        // "fit pivot": peel off equal elements
        while(j>=0 && !(r[j]<pivot)) j--;
        while(i<count && !(pivot<r[i])) i++;

        // allow roughly 1.5 log2(N) recursions before switching over to heapsort.
        cutoff >>= 1;
        cutoff += cutoff >> 2;

        // recurse
        sInt n;
        if((n = j+1) > 1)     IntroSortR(range.Slice(0,n),cutoff);
        if((n = count-i) > 1) IntroSortR(range.Slice(i,count),cutoff);
      }
      else // using too many recursions, try and finish it off with heapsort instead
        sHeapSort(range);
    }
    else if(count > 1) // small range, use insertion sort
      sInsertionSort(range);
  }
}

// Introsort - fast introspective sorting algorithm. Guaranteed O(N log N)
// worst-case runtime.
template<typename Range,typename Pred>
void sIntroSort(const Range &range,const Pred &less)
{
  sAlgoImpl::IntroSortR(range,range.GetCount(),less);
}

template<typename Range>
void sIntroSort(const Range &range)
{
  sAlgoImpl::IntroSortR(range,range.GetCount());
}

/****************************************************************************/
/***                                                                      ***/
/***   Binary search - various functions. All assume a sorted range.      ***/
/***                                                                      ***/
/****************************************************************************/

// Lower bound: Assuming a sorted range, this returns the first position
// where x could be inserted without violating the order.
template<typename Range,typename T,typename Pred>
sInt sLowerBound(const Range &range,const T &x,const Pred &less)
{
  sInt l=0, r=range.GetCount();
  // invariant: the "crossover point" p is between range[l] and range[r],
  // i.e. for all 0<=i<l:less(range[i],x) and for all r<=i<count:!less(range[i],x)
  while(l<r)
  {
    sInt m = l + (r-l)/2;
    if(less(range[m],x))
      l = m+1;
    else
      r = m;
  }

  return l; // now l=r, which must be the crossover point.
}

template<typename Range,typename T>
sInt sLowerBound(const Range &range,const T &x)
{
  sInt l=0, r=range.GetCount();
  while(l<r)
  {
    sInt m = l + (r-l)/2;
    if(range[m]<x)
      l = m+1;
    else
      r = m;
  }

  return l;
}

// Upper bound: Assuming a sorted range, this returns the last position
// where x could be inserted without violating the order.
template<typename Range,typename T,typename Pred>
sInt sUpperBound(const Range &range,const T &x,const Pred &less)
{
  sInt l=0, r=range.GetCount();
  // invariant: the "crossover point" p is between range[l] and range[r],
  // i.e. for all 0<=i<l:!less(x,range[i]) and for all r<=i<count:less(x,range[i])
  while(l<r)
  {
    sInt m = l + (r-l)/2;
    if(!less(x,range[m]))
      l = m+1;
    else
      r = m;
  }

  return l; // now l=r, which must be the crossover point.
}

template<typename Range,typename T>
sInt sUpperBound(const Range &range,const T &x)
{
  sInt l=0, r=range.GetCount();
  while(l<r)
  {
    sInt m = l + (r-l)/2;
    if(!(x<range[m]))
      l = m+1;
    else
      r = m;
  }

  return l;
}

// All less-than: Assuming a sorted range, this returns the subrange of all
// elements less than x.
template<typename Range,typename T,typename Pred>
Range sAllLessThan(const Range &range,const T &x,const Pred &less)
{
  return range.Slice(0,sLowerBound(range,x,less));
}

template<typename Range,typename T>
Range sAllLessThan(const Range &range,const T &x)
{
  return range.Slice(0,sLowerBound(range,x));
}

// All greater-than: Assuming a sorted range, this returns the subrange
// of all elements greater than x.
template<typename Range,typename T,typename Pred>
Range sAllGreaterThan(const Range &range,const T &x,const Pred &less)
{
  return range.Slice(sUpperBound(range,x,less),range.GetCount());
}

template<typename Range,typename T>
Range sAllGreaterThan(const Range &range,const T &x)
{
  return range.Slice(sUpperBound(range,x),range.GetCount());
}

// All equal: Assuming a sorted range, this returns the subrange
// of all elements equal (actually, equivalent) to x, per the chosen ordering.
template<typename Range,typename T,typename Pred>
Range sAllEqual(const Range &range,const T &x,const Pred &less)
{
  sInt l = sLowerBound(range,x,less);
  sInt count = sUpperBound(range.Slice(l,range.GetCount()),x,less);
  return range.Slice(l,l+count);
}

template<typename Range,typename T>
Range sAllEqual(const Range &range,const T &x)
{
  sInt l = sLowerBound(range,x);
  sInt count = sUpperBound(range.Slice(l,range.GetCount()),x);
  return range.Slice(l,l+count);
}

// Find first: Assuming a sorted range, returns the index of the first
// occurence of item x (or -1 if not found)
template<typename Range,typename T,typename Pred>
sInt sFindFirst(const Range &range,const T &x,const Pred &less)
{
  sInt l = sLowerBound(range,x,less);
  if(l < range.GetCount() && range[l] == x)
    return l;
  else
    return -1;
}

template<typename Range,typename T>
sInt sFindFirst(const Range &range,const T &x)
{
  sInt l = sLowerBound(range,x);
  if(l < range.GetCount() && range[l] == x)
    return l;
  else
    return -1;
}

/****************************************************************************/

#endif // FILE_UTIL_ALGORITHMS_HPP


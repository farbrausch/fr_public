/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_BASE_TYPES2_HPP
#define FILE_ALTONA2_LIBS_BASE_TYPES2_HPP

#include "Altona2/Libs/Base/Machine.hpp"
#include <memory>
#include <type_traits>

namespace Altona2 {

template <class T> class sShared;
template <class T> class sWeakShared;


/****************************************************************************/
/***                                                                      ***/
/***   Utilties                                                           ***/
/***                                                                      ***/
/****************************************************************************/

template<class T> struct sRemoveRef
{
    typedef T type;
};

template<class T> struct sRemoveRef<T &>
{
    typedef T type;
};

template<class T> struct sRemoveRef<T &&>
{
    typedef T type;
};

template<class T> inline typename sRemoveRef<T>::type &&sMove(T &&a) throw()
{
    typedef typename sRemoveRef<T>::type &&rtype;
    return static_cast<rtype>(a);
}

/****************************************************************************/
/***                                                                      ***/
/***   Pointers                                                           ***/
/***                                                                      ***/
/****************************************************************************/

template <class T> class sUnique
{
    T *ptr;

    typedef sUnique<T> mytype;
    sUnique(const mytype &x)                    { sASSERT0(); }
    mytype &operator=(const mytype &x)          { sASSERT0(); }
public:
    sUnique()                           throw() { ptr = 0; }
    sUnique(mytype &&x)                 throw() { sSwap(ptr,x.ptr); }
    sUnique(T *x)                       throw() { ptr = x; }
    ~sUnique()                                  { delete ptr; }
    mytype &operator=(mytype &&x)       throw() { sSwap(ptr,x.ptr); return *this; }

    T& operator*() const                throw() { return *ptr; }
    T* operator->() const               throw() { return ptr; }
};

/****************************************************************************/

template <class T> class sRefWrap : public T
{
private:
    friend class sShared<T>;
    friend class sWeakShared<T>;
    uint RefCount;
    uint WeakCount;

    void AddRef()       { if(this) { sAtomicInc(&RefCount); sASSERT(RefCount!=0); } }
    void Release()      { if(this) { sASSERT(RefCount!=0); sAtomicDec(&RefCount); if(RefCount==0) delete this; } }
    void WeakAddRef()   { if(this) { sAtomicInc(&WeakCount); sASSERT(RefCount!=0); } }
    void WeakRelease()  { if(this) { sASSERT(WeakCount!=0); sAtomicDec(&WeakCount); if(WeakCount==0) delete this; } }
public:

    sRefWrap()                 : T(        ) { RefCount = WeakCount = 0; }
    template<typename A>
    sRefWrap(A a)              : T(a       ) { RefCount = WeakCount = 0; }
    template<typename A,typename B>
    sRefWrap(A a,B b)          : T(a,b     ) { RefCount = WeakCount = 0; }
    template<typename A,typename B,typename C>
    sRefWrap(A a,B b,C c)      : T(a,b,c   ) { RefCount = WeakCount = 0; }
};


template <class T>                                  inline sRefWrap<T> *sMakeShared()
{   return new sRefWrap<T>();          };
template <class T,typename A>                       inline sRefWrap<T> *sMakeShared(A a)
{   return new sRefWrap<T>(a);         };
template <class T,typename A,typename B>            inline sRefWrap<T> *sMakeShared(A a,B b)
{   return new sRefWrap<T>(a,b);       };
template <class T,typename A,typename B,typename C> inline sRefWrap<T> *sMakeShared(A a,B b,C c)
{   return new sRefWrap<T>(a,b,c);     };


template <class T> class sShared
{
    sRefWrap<T> *ptr;
    typedef sShared<T> mytype;
//    sShared(T *x)                           throw() { ptr = x; x.AddRef(); }
public:
    sShared()                               throw() { ptr = 0; }
    sShared(const mytype &x)                throw() { ptr = x.ptr; ptr->AddRef(); }
    sShared(mytype &&x)                     throw() { sSwap(ptr,x.ptr); }
    sShared(sRefWrap<T> *x)                 throw() { ptr = x; x->AddRef(); }
    ~sShared()                                      { ptr->Release(); }
    mytype &operator=(const mytype &x)              { ptr->Release(); ptr = x.ptr; ptr->AddRef(); return *this; }
    mytype &operator=(mytype &&x)           throw() { sSwap(ptr,x.ptr); return *this; }
    mytype &operator=(sRefWrap<T> *x)               { ptr->Release(); ptr = x; x->AddRef(); return *this; }

    T& operator*() const                    throw() { return *ptr; }
    T* operator->() const                   throw() { return ptr; }
};

/*
template <class T> class sWeakShared
{
    sReferenceWrapper<T> *ptr;
    typedef sUnique<T> mytype;
public:
    sWeakShared()                           { ptr = 0; }
    sWeakShared(mytype &&x)                 { sSwap(ptr,x.ptr); }
    sWeakShared(T *x)                       { ptr = x; }
    mytype &operator=(mytype &&x)       { sSwap(ptr,x.ptr); return *this; }
};
*/

/****************************************************************************/
/***                                                                      ***/
/***   Arrays                                                             ***/
/***                                                                      ***/
/****************************************************************************/

template<class T> class sArrayContainer
{
public:
    T *First;
    T *Last;
    T *End;

    sArrayContainer()
    {
        First = Last = End = 0;
    }
    ~sArrayContainer()
    {
        SetCount(0);
        delete[] (uint8 *) First;
    }

    void SetCount(sPtr n)
    {
        T *newlast = First+n;

        while(newlast<Last)
        {
            Last--;
            Last->~T();
        }
        if(newlast>End)
            GrowCapacity(n);
        while(newlast>Last)
        {
            sPlacementNew<T>(Last);
            Last++;
        }
    }
    void SetCapacity(sPtr n)
    {
        T *newfirst = (T *) new uint8[sizeof(T)*n];
        T *stop = sMin(Last,First+n);
        T *newlast = stop - First + newfirst;
        T *s = First;
        T *d = newfirst;
        while(s<stop)
            *d++ = *s++;
        while(s<Last)
            s->~T();

        delete[] (uint8 *) First;
        First = newfirst;
        Last = newlast;
        End = newfirst + n;
    }
    void GrowCapacity(sPtr n)
    {
        if(Last+n>End)
        {
            sPtr count = Last-First;
            SetCapacity(sMax(count+n,sMax(count*2,64/sizeof(T))));
        }
    }
};


template<class T,int capacity> class sStackContainer
{
    uint8 Data[sizeof(T)*capacity];
public:
    T *First;
    T *Last;
    T *End;

    sStackContainer()
    {
        First = Last = (T *) Data;
        End = First + capacity;
    }
    ~sStackContainer()
    {
    }

    void SetCount(sPtr n)
    {
        T *newlast = First+n;

        while(newlast<Last)
        {
            Last--;
            Last->~T();
        }
        if(newlast>End)
            GrowCapacity(n);
        while(newlast>Last)
        {
            sPlacementNew<T>(Last);
            Last++;
        }
    }
    void SetCapacity(sPtr n)
    {
        sASSERT(n<=capacity);
    }
    void GrowCapacity(sPtr n)
    {
        sASSERT(First+n<=End);
    }
};

template<class T,class D> class sArrayBase : protected D
{
public:
    // information

    int Count() const                   { return Last-First; }
    int Capacity() const                { return End-First; }
    T *GetData() const                  { return First; }
    bool IsIndex(sPtr n) const          { return First+n<Last; }
    bool IsEmpty() const                { return Last==First; }
    T &operator[](sPtr n) const         { sASSERT(IsIndex(n)); return Data[n]; }
    T &GetTail() const                  { sASSERT(!IsEmpty()); return Data[Used-1]; }
    T &GetHead() const                  { sASSERT(!IsEmpty()); return Data[0]; }

    // unordered

    void Add(const T &e)
    {
        GrowCapacity(1);
        sPlacementNewConst<T>(Last,e);
        Last++;
    }

    void Add(T &&e)
    {
        GrowCapacity(1);
        sPlacementNew<T>(Last);
        *Last = sMove(e);
        Last++;
    }

    void RemTail()
    {
        sASSERT(!IsEmpty());
        Last--;
        Last->~T();
    }
    void RemAt(sPtr pos)
    {
        Last--;
        First[pos] = sMove(*Last);
        Last->~T();
    }
    bool Rem(const T &e)
    {
        for(T *i=First;i<Last;i++)
        {
            if(*i==e)
            {
                Last--;
                *i = sMove(*Last);
                Last->~T();
                return true;
            }
        }
        return false;
    }



//    void SetAndGrow(int n,const T &e)   { n; if(Count<n+1) SetCount(n+1); Data[i] = e; }
//    void SetAndGrow(int n,const T &&e)   { n; if(Count<n+1) SetCount(n+1); Data[i] = sMove(e); }

};

template <class T> class sDynArray : public sArrayBase<T,sArrayContainer<T> >
{
};

template <class T,int n> class sStackArray : public sArrayBase<T,sStackContainer<T,n> >
{
};


/****************************************************************************/
} // namespace Base
#endif  // FILE_ALTONA2_LIBS_BASE_TYPES2_HPP


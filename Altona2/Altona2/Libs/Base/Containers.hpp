/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_BASE_CONTAINERS_HPP
#define FILE_ALTONA2_LIBS_BASE_CONTAINERS_HPP

#include "Altona2/Libs/Base/Machine.hpp"

namespace Altona2 {
class sMemoryPool;
class sReader;
class sWriter;

/****************************************************************************/
/***                                                                      ***/
/***   Base class for arrays                                              ***/
/***                                                                      ***/
/****************************************************************************/

template<class T>
class sArray
{
    int Used;
    int Alloc;
    T *Data;

    void Realloc(int n) { T *newdata=new T[n]; sCopyMem(newdata,this->Data,sizeof(T)*this->Used); delete[] this->Data; this->Data=newdata; this->Alloc=n; }
    void Grow(int n) { if(this->Used+n>this->Alloc) Realloc(sMax(sMax(4,int(64/sizeof(T))),sMax(this->Alloc*2,this->Used+n))); this->Used+=n; }
public:
    sArray() { Used = Alloc = 0; Data = 0; }
    ~sArray() { delete[] Data; }
    sArray(const sArray<T> &src) { CopyFrom(src); }
    sArray<T> &operator=(const sArray<T> &src) { CopyFrom(src); return *this; }
    void OverrideStorage(T* data,int count) { Data = data; Alloc = count; Used = 0; }
    void FreeMemory() { this->Used=0; this->Alloc=0; sDelete(this->Data); }
    void SetCapacity(int n) { sASSERT(n>=this->Used); if(this->Alloc!=n) this->Realloc(n); }
    void HintSize(int n) { if(this->Alloc<n) this->Realloc(n); }
    void SetSize(int n) { if(this->Alloc<n) this->Realloc(n); this->Used=n; }
    void Clear() { this->Used=0; }
    void CopyFrom(const sArray<T> &src) { Clear(); Add(src); }
    void SetAndGrow(int index,const T &e) { if(!IsIndex(index)) { int old=Used; Grow(index+1-Used); for(int i=old;i<=index;i++) Data[i]=0; } Data[index]=e; }
    void Exchange(sArray<T> &b) { sSwap(Used,b.Used); sSwap(Alloc,b.Alloc); sSwap(Data,b.Data); }

// attributes

    int GetCount() const { return Used; }
    bool IsEmpty() const { return Used==0; }
    bool IsNotEmpty() const { return Used!=0; }
    bool IsIndex(int n) const { return n>=0 && n<Used; }
    bool IsEqual(const sArray<T> &b) const { if(GetCount()!=b.GetCount()) return 0; for(int i=0;i<GetCount();i++) { if(Data[i]!=b[i]) return 0; } return 1; }

    bool operator == (const sArray<T> &b) const { return IsEqual(b); }
    bool operator != (const sArray<T> &b) const { return !IsEqual(b); }

// access

    T &operator[](int n) const { sASSERT(IsIndex(n)); return Data[n]; }
    T *GetData() const { return Data; }
    T &GetTail() const { sASSERT(!IsEmpty()); return Data[Used-1]; }
    T &GetHead() const { sASSERT(!IsEmpty()); return Data[0]; }

// unordered add / remove

    void Add(const T &e) { this->Grow(1); this->Data[this->Used-1] = e; }
    T *Add() { this->Grow(1); return this->Data+this->Used-1; }
    void Add(const sArray<T> &ar) { T *e=AddMany(ar.GetCount()); for(int i=0;i<ar.GetCount();i++) e[i]=ar[i]; }
    void RemAt(int n) { this->Data[n] = this->Data[--this->Used]; }
    bool Rem(const T &e) { bool result = 0; for(int i=0;i<this->Used;) if(this->Data[i]==e) { result = 1; RemAt(i); } else i++; return result; }
    bool AddUnique(const T &e) { for(int i=0;i<this->Used;i++) if(this->Data[i]==e) return false; Add(e); return true; }

// ordered add / remove

    T *AddMany(int n) { this->Grow(n); return this->Data+this->Used-n; }
    void AddTail(sArray<T> &a) { T *n=AddMany(a.GetCount()); for(int i=0;i<a.GetCount();i++) n[i]=a[i]; }
    void AddTail(const T &e) { this->Grow(1); this->Data[this->Used-1] = e; }
    T RemTail() { sASSERT(this->Used>0); this->Used--; return this->Data[this->Used]; }
    void AddHead(const T &e) { this->Grow(1); for(int i=this->Used-2;i>=0;i--) this->Data[i+1]=this->Data[i]; this->Data[0] = e; }
    bool RemOrder(const T &e) { for(int i=0;i<this->Used;i++) { if(this->Data[i]==e) { RemAtOrder(i); return 1; } } return 0; }
    void RemAtOrder(int n) { sASSERT(this->IsIndex(n)); this->Used--; for(int i=n;i<this->Used;i++) this->Data[i]=this->Data[i+1]; }
    void AddAt(const T &e,int n) { sASSERT(n>=0&&n<=this->Used); this->Grow(1); for(int i=this->Used-2;i>=n;i--) this->Data[i+1]=this->Data[i]; this->Data[n] = e; }
    void Swap(int i,int j) { sASSERT(i>=0 && i<this->Used && j>=0 && j<this->Used) sSwap(this->Data[i],this->Data[j]); }
    void Move(const T &e,int before) { int si = FindEqualIndex(e); if(si>=0 && si<before) before--; RemAtOrder(si); AddAt(e,before); }  // move to position in folder, handle case where when moving from same folder positions change!
    void Move(sArray<T> &from,const T &e,int before) { int si = FindEqualIndex(e); if(si>=0 && si<before) before--; from.RemOrder(e); AddAt(e,before); }  // move to position in folder, handle case where when moving from same folder positions change!

// find ptr

    template <typename Pred>
    T Find(Pred pred) const
    { for(int i=0;i<this->Used;i++) if(pred(this->Data[i])) return this->Data[i]; return 0; }

    template <typename Pred>
    T FindFirst(Pred pred) const
    { for(int i=0;i<this->Used;i++) if(pred(this->Data[i])) return this->Data[i]; return 0; }

    template <typename Pred>
    T FindLast(Pred pred) const
    { for(int i=this->Used-1;i>=0;i--) if(pred(this->Data[i])) return this->Data[i]; return 0; }
/*
    template <typename Pred>
    T FindPtr(Pred pred) const
    { for(int i=0;i<this->Used;i++) if(pred(this->Data[i])) return this->Data[i]; return 0; }
*/
// find value. need to return value as pointer in order to allow 0 to represent "not found". I hate exceptions.

    template <typename Pred>
    T *FindValue(Pred pred) const
    { for(int i=0;i<this->Used;i++) if(pred(this->Data[i])) return &this->Data[i]; return 0; }

    template <typename Pred>
    T *FindValueFirst(Pred pred) const
    { for(int i=0;i<this->Used;i++) if(pred(this->Data[i])) return &this->Data[i]; return 0; }

    template <typename Pred>
    T *FindValueLast(Pred pred) const
    { for(int i=this->Used-1;i>=0;i--) if(pred(this->Data[i])) return &this->Data[i]; return 0; }

// find index & other

    template <typename Pred>
    int FindIndex(Pred pred) const
    { for(int i=0;i<this->Used;i++) if(pred(this->Data[i])) return i; return -1; }

    template <typename Pred>
    int FindFirstIndex(Pred pred) const
    { for(int i=0;i<this->Used;i++) if(pred(this->Data[i])) return i; return -1; }

    template <typename Pred>
    int FindLastIndex(Pred pred) const
    { for(int i=this->Used-1;i>=0;i--) if(pred(this->Data[i])) return i; return -1; }

    bool FindEqual(const T &a) const
    { for(int i=this->Used-1;i>=0;i--) if(this->Data[i]==a) return 1; return 0; }

    int FindEqualIndex(const T &a) const
    { for(int i=this->Used-1;i>=0;i--) if(this->Data[i]==a) return i; return -1; }

// remove

    template <typename Pred>
    void RemIf(Pred pred)
    { for(int i=0;i<this->Used;) if(pred(this->Data[i])) this->Data[i] = this->Data[--this->Used]; else i++; }

    template <typename Pred>
    void RemIfOrder(Pred pred)
    { int d=0; for(int s=0;s<this->Used;s++) if(!pred(this->Data[s])) this->Data[d++] = this->Data[s]; this->Used=d; }

// pointer

    void DeleteAll() { for(int i=0;i<this->Used;i++) delete this->Data[i]; this->Clear(); }
    void DeleteArrayAll() { for(int i=0;i<this->Used;i++) delete[] this->Data[i]; this->Clear(); }
    void ReleaseAll() { for(int i=0;i<this->Used;i++) this->Data[i]->Release(); this->Clear(); }
    void AddRefAll() const { for(int i=0;i<this->Used;i++) this->Data[i]->AddRef(); }
    void NeedAll() const { for(int i=0;i<this->Used;i++) this->Data[i]->Need(); }
    void AddAddRef(const T &e) { Add(e); e->AddRef(); }
    bool RemRelease(const T &e) { bool result = 0; for(int i=0;i<this->Used;) if(this->Data[i]==e) { result = 1; RemAt(i); e->Release(); } else i++; return result; }


    template <typename Pred>
    void DeleteIf(Pred pred)
    { for(int i=0;i<this->Used;) if(pred(this->Data[i])) { delete this->Data[i]; this->Data[i] = this->Data[--this->Used]; } else i++; }

    template <typename Pred>
    void DeleteArrayIf(Pred pred)
    { for(int i=0;i<this->Used;) if(pred(this->Data[i])) { delete[] this->Data[i]; this->Data[i] = this->Data[--this->Used]; } else i++; }

    template <typename Pred>
    void ReleaseIf(Pred pred)
    { for(int i=0;i<this->Used;) if(pred(this->Data[i])) { this->Data[i]->Release(); this->Data[i] = this->Data[--this->Used]; } else i++; }

    template <typename Pred>
    void DeleteIfOrder(Pred pred)
    { int d=0; for(int s=0;s<this->Used;s++) if(!pred(this->Data[s])) this->Data[d++] = this->Data[s]; else delete this->Data[s]; this->Used=d; }

    template <typename Pred>
    void DeleteArrayIfOrder(Pred pred)
    { int d=0; for(int s=0;s<this->Used;s++) if(!pred(this->Data[s])) this->Data[d++] = this->Data[s]; else delete[] this->Data[s]; this->Used=d; }

    template <typename Pred>
    void ReleaseIfOrder(Pred pred)
    { int d=0; for(int s=0;s<this->Used;s++) if(!pred(this->Data[s])) this->Data[d++] = this->Data[s]; else this->Data[s]->Release(); this->Used=d; }

/****************************************************************************/

// sorting

/****************************************************************************/

// insertion sort. 
// stable, in-place, n^2, n on presorted array
// often used by other sorting algorithms for sorting subarrays.

    template <typename Pred>
    void InsertionSort(const Pred &pred)      
    {
        InsertionSort(pred,this->Data,this->Used);
    }

private:
    template <typename Pred>
    void InsertionSort(const Pred &pred,T *d,int count)
    {
        for(int i=1;i<count;i++)
        {
            if(pred(d[i],d[i-1]))
            {
                T v = d[i];
                int j=i;
                do 
                {
                    d[j]=d[j-1];
                    j--;
                }
                while(j>0 && pred(v,d[j-1]));
                d[j] = v;
            }
        }
    }
public:

/****************************************************************************/

// merge sort
// stable, not in-place, n log n

    template <typename Pred>
    void MergeSort(const Pred &pred,sArray<T> &buffer)      
    {
        buffer.SetSize(this->GetCount());
        sArray<T> *a = this;
        sArray<T> *b = &buffer;
        int n = this->Used;
        int startwidth = 32;
        for(int i=0;i<n;i+=startwidth)
        {
            InsertionSort(pred,this->Data+i,sMin((i+startwidth),n)-i);
        }
        for(int width=startwidth;width<n;width*=2)
        {
            for(int i=0;i<n;i+=2*width)
            {
                int i0 = i;
                int i1 = sMin(i+width,n);
                int i2 = sMin(i+2*width,n);
                int ic = i1;
                for(int j=i0;j<i2;j++)
                {
                    if(i0<ic && (i1>=i2 || pred(a->Data[i0],a->Data[i1])))
                        b->Data[j]=a->Data[i0++];
                    else
                        b->Data[j]=a->Data[i1++];
                }
            }
            sSwap(a,b);
        }
        if(a!=this)
        {
            Clear();
            Add(*a);
        }
    }

// quick sort (untweaked)
// not stable, in-place, n log n with n^2 worst case

/****************************************************************************/

    template <typename Pred>
    void QuickSort(const Pred &pred)      
    {
        if(this->Used>0)
            QuickSort(pred,this->Data,this->Used);
    }

private:
    template <typename Pred>
    void QuickSort(const Pred &pred,T *d,int count)
    {
        sASSERT(count>0);
        if(count<=32)
        {
            InsertionSort(pred,d,count);
        }
        else
        {
            // median of 3 avoids some of the worst case scenarios.

            T p = MedianOf3(pred,d[0],d[count>>1],d[count-1]);
            int j0 = 0;
            int j1 = count;

            // all elements below j0 are <=p, all element above j1 are >=p
            // j1 is of by one to allow do-loop which is faster than while loop. 

            for(;;)
            {
                while(pred(d[j0],p)) j0++;
                do j1--; while(pred(p,d[j1]));
                if(j0>=j1) break;
                sSwap(d[j0++],d[j1]);
            }

            // from now on, j0 and j1 swap places! skip elements that are ==p

            while(j1>0 && !pred(d[j1],p)) j1--;
            while(j0<count-1 && !pred(p,d[j0])) j0++;

            // recursion

            int n;
            n=j1+1; if(n>1) QuickSort(pred,d,n);
            n=count-j0; if(n>1) QuickSort(pred,d+j0,n);
        }
    }

// Median of 3.
    template<typename Pred>
    T MedianOf3(const Pred &pred,const T &a,const T &b,const T &c)
    {
        if(pred(b,a))
        {
            if(pred(c,a))
                return pred(b,c) ? c : b;
            else
                return a;
        }
        else
            return (pred(c,b) || pred(a,c)) ? b : a;
    }
public:

/****************************************************************************/

    template <typename Pred>
    void Sort(const Pred &pred)      
    {
        if(this->Used>0)
            QuickSort(pred,this->Data,this->Used);
    }

/****************************************************************************/

};

template<typename T> T *begin(const sArray<T> &a) { return a.GetData(); }
template<typename T> T *end(const sArray<T> &a) { return a.GetData()+a.GetCount(); }
template<typename T> uptr sGetIndex(const sArray<T> &a,T &i) { return &i-a.GetData(); }

/****************************************************************************/
/***                                                                      ***/
/***   Const Array                                                        ***/
/***                                                                      ***/
/****************************************************************************/

template<class T>
class sConstArray
{
    const sArray<T> *Array;
public:
    sConstArray(const sArray<T> &a) : Array(&a) {}

// attributes

    int GetCount() const { return Array->GetCount(); }
    bool IsEmpty() const { return Array->IsEmpty(); }
    bool IsNotEmpty() const { return Array->IsNotEmpty(); }
    bool IsIndex(int n) const { return Array->IsIndex(n); }
    bool IsEqual(const sArray<T> &b) const { return Array->IsEqual(b); }

    bool operator == (const sArray<T> &b) const { return Array->IsEqual(b); }
    bool operator != (const sArray<T> &b) const { return !Array->IsEqual(b); }

// access

    T &operator[](int n) const { sASSERT(Array->IsIndex(n)); return Array->GetData()[n]; }
    T *GetData() const { return Array->GetData(); }
    T GetTail() const { return GetTail(); }
    T GetHead() const { return GetHead(); }

// find ptr

    template <typename Pred> T Find(Pred pred) const { return Array->Find(pred); }
    template <typename Pred> T FindFirst(Pred pred) const { return Array->FindFirst(pred); }
    template <typename Pred> T FindLast(Pred pred) const { return Array->FindLast(pred); }

// find value. need to return value as pointer in order to allow 0 to represent "not found". I hate exceptions.

    template <typename Pred> T *FindValue(Pred pred) const { return Array->FindValue(pred); }
    template <typename Pred> T *FindValueFirst(Pred pred) const { return Array->FindValueFirst(pred); }
    template <typename Pred> T *FindValueLast(Pred pred) const { return Array->FindValueLast(pred); }

// find index & other

    template <typename Pred> int FindIndex(Pred pred) const { return Array->FindIndex(pred); }
    template <typename Pred> int FindFirstIndex(Pred pred) const { return Array->FindFirstIndex(pred); }
    template <typename Pred> int FindLastIndex(Pred pred) const { return Array->FindLastIndex(pred); }
    bool FindEqual(const T &a) const { return Array->FindEqual(a); }
    int FindEqualIndex(const T &a) const { return Array->FindEqualIndex(a); }

// pointer

    void AddRefAll() const { Array->AddRefAll(); }
    void NeedAll() const { Array->NeedAll(); }
};

template<typename T> T *begin(const sConstArray<T> &a) { return a.GetData(); }
template<typename T> T *end(const sConstArray<T> &a) { return a.GetData()+a.GetCount(); }
template<typename T> uptr sGetIndex(const sConstArray<T> &a,T &i) { return &i-a.GetData(); }

/****************************************************************************/
/***                                                                      ***/
/***   sMemoryPool                                                        ***/
/***                                                                      ***/
/****************************************************************************/

class sMemoryPool
{
    sArray<uptr> UsedPool;
    sArray<uptr> FreePool;

    uptr Start;
    uptr Ptr;
    uptr End;
    uptr BlockSize;
    uptr TotalUsed;
public:
    sMemoryPool(uptr defaultsize = 65536);
    ~sMemoryPool();
    uint8 *Alloc(uptr size,int align);
    void Reset();
    uptr GetUsed();

    template <typename T> T *Alloc(uptr n=1) { return (T*) Alloc(sizeof(T)*n,sALIGNOF(T)); }
    char *AllocString(const char *);
    char *AllocString(const char *,uptr len);

    void Serialize(sReader &s,const char *&str);
    void Serialize(sWriter &s,const char *&str);
};

/****************************************************************************/
/***                                                                      ***/
/***   PoolArray                                                          ***/
/***                                                                      ***/
/****************************************************************************/

template<class T>
class sPoolArray
{
    int Used;
    int Alloc;
    T *Data;
    sMemoryPool *Pool;

    void Realloc(int n) { T *newdata=Pool->Alloc<T>(n); sCopyMem(newdata,this->Data,sizeof(T)*this->Used); this->Data=newdata; this->Alloc=n; }
    void Grow(int n) { if(this->Used+n>this->Alloc) Realloc(sMax(sMax(4,int(64/sizeof(T))),sMax(this->Alloc*2,this->Used+n))); this->Used+=n; }
public:
    sPoolArray(sMemoryPool *pool) { Used = Alloc = 0; Data = 0; Pool=pool; }
    sPoolArray() { Used = Alloc = 0; Data = 0; Pool=0; }
    void SetPool(sMemoryPool *pool) { Pool = pool; }
    ~sPoolArray() { }
    sPoolArray(sMemoryPool *pool,const sPoolArray<T> &src) { Used = Alloc = 0; Data = 0; Pool=pool; CopyFrom(src); }
    sPoolArray<T> &operator=(const sPoolArray<T> &src) { CopyFrom(src); return *this; }
    void SetCapacity(int n) { sASSERT(n>=this->Used); if(this->Alloc!=n) this->Realloc(n); }
    void HintSize(int n) { if(this->Alloc<n) this->Realloc(n); }
    void SetSize(int n) { if(this->Alloc<n) this->Realloc(n); this->Used=n; }
    void Clear() { this->Used=0; }
    void Reset() { this->Used=0; this->Alloc=0; this->Data=0; }
    void CopyFrom(const sPoolArray<T> &src) { Clear(); Add(src); }
    void SetAndGrow(int index,const T &e) { if(!IsIndex(index)) { int old=Used; Grow(index+1-Used); for(int i=old;i<=index;i++) Data[i]=0; } Data[index]=e; }
    void Exchange(sPoolArray<T> &b) { sSwap(Used,b.Used); sSwap(Alloc,b.Alloc); sSwap(Data,b.Data); }

// attributes

    int GetCount() const { return Used; }
    bool IsEmpty() const { return Used==0; }
    bool IsNotEmpty() const { return Used!=0; }
    bool IsIndex(int n) const { return n>=0 && n<Used; }
    bool IsEqual(const sPoolArray<T> &b) const { if(GetCount()!=b.GetCount()) return 0; for(int i=0;i<GetCount();i++) { if(Data[i]!=b[i]) return 0; } return 1; }

    bool operator == (const sPoolArray<T> &b) const { return IsEqual(b); }
    bool operator != (const sPoolArray<T> &b) const { return !IsEqual(b); }

// access

    const T &operator[](int n) const  { sASSERT(IsIndex(n)); return Data[n]; }
    T &operator[](int n) { sASSERT(IsIndex(n)); return Data[n]; }
    T *GetData() { return Data; }
    const T *GetData() const { return Data; }

// unordered add / remove

    void Add(const T &e) { this->Grow(1); this->Data[this->Used-1] = e; }
    T *Add() { this->Grow(1); return this->Data+this->Used-1; }
    void Add(const sPoolArray<T> &ar) { T *e=AddMany(ar.GetCount()); for(int i=0;i<ar.GetCount();i++) e[i]=ar[i]; }
    void RemAt(int n) { this->Data[n] = this->Data[--this->Used]; }
    bool Rem(const T &e) { bool result = 0; for(int i=0;i<this->Used;) if(this->Data[i]==e) { result = 1; RemAt(i); } else i++; return result; }
    bool AddUnique(const T &e) { for(int i=0;i<this->Used;i++) if(this->Data[i]==e) return false; Add(e); return true; }

// ordered add / remove

    T *AddMany(int n) { this->Grow(n); return this->Data+this->Used-n; }
    void AddTail(sPoolArray<T> &a) { T *n=AddMany(a.GetCount()); for(int i=0;i<a.GetCount();i++) n[i]=a[i]; }
    void AddTail(const T &e) { this->Grow(1); this->Data[this->Used-1] = e; }
    T RemTail() { sASSERT(this->Used>0); this->Used--; return this->Data[this->Used]; }
    void AddHead(const T &e) { this->Grow(1); for(int i=this->Used-2;i>=0;i--) this->Data[i+1]=this->Data[i]; this->Data[0] = e; }
    bool RemOrder(const T &e) { for(int i=0;i<this->Used;i++) { if(this->Data[i]==e) { RemAtOrder(i); return 1; } } return 0; }
    void RemAtOrder(int n) { sASSERT(this->IsIndex(n)); this->Used--; for(int i=n;i<this->Used;i++) this->Data[i]=this->Data[i+1]; }
    void AddAt(const T &e,int n) { sASSERT(n>=0&&n<=this->Used); this->Grow(1); for(int i=this->Used-2;i>=n;i--) this->Data[i+1]=this->Data[i]; this->Data[n] = e; }
    void Swap(int i,int j) { sASSERT(i>=0 && i<this->Used && j>=0 && j<this->Used) sSwap(this->Data[i],this->Data[j]); }

// find ptr

    template <typename Pred>
    T Find(Pred pred) const
    { for(int i=0;i<this->Used;i++) if(pred(this->Data[i])) return this->Data[i]; return 0; }

    template <typename Pred>
    T FindFirst(Pred pred) const
    { for(int i=0;i<this->Used;i++) if(pred(this->Data[i])) return this->Data[i]; return 0; }

    template <typename Pred>
    T FindLast(Pred pred) const
    { for(int i=this->Used-1;i>=0;i--) if(pred(this->Data[i])) return this->Data[i]; return 0; }
/*
    template <typename Pred>
    T FindPtr(Pred pred) const
    { for(int i=0;i<this->Used;i++) if(pred(this->Data[i])) return this->Data[i]; return 0; }
*/
// find value. need to return value as pointer in order to allow 0 to represent "not found". I hate exceptions.

    template <typename Pred>
    T *FindValue(Pred pred) const
    { for(int i=0;i<this->Used;i++) if(pred(this->Data[i])) return &this->Data[i]; return 0; }

    template <typename Pred>
    T *FindValueFirst(Pred pred) const
    { for(int i=0;i<this->Used;i++) if(pred(this->Data[i])) return &this->Data[i]; return 0; }

    template <typename Pred>
    T *FindValueLast(Pred pred) const
    { for(int i=this->Used-1;i>=0;i--) if(pred(this->Data[i])) return &this->Data[i]; return 0; }

// find index & other

    template <typename Pred>
    int FindIndex(Pred pred) const
    { for(int i=0;i<this->Used;i++) if(pred(this->Data[i])) return i; return -1; }

    template <typename Pred>
    int FindFirstIndex(Pred pred) const
    { for(int i=0;i<this->Used;i++) if(pred(this->Data[i])) return i; return -1; }

    template <typename Pred>
    int FindLastIndex(Pred pred) const
    { for(int i=this->Used-1;i>=0;i--) if(pred(this->Data[i])) return i; return -1; }

    bool FindEqual(const T &a) const
    { for(int i=this->Used-1;i>=0;i--) if(this->Data[i]==a) return 1; return 0; }

    int FindEqualIndex(const T &a) const
    { for(int i=this->Used-1;i>=0;i--) if(this->Data[i]==a) return i; return -1; }

// remove

    template <typename Pred>
    void RemIf(Pred pred)
    { for(int i=0;i<this->Used;) if(pred(this->Data[i])) this->Data[i] = this->Data[--this->Used]; else i++; }

    template <typename Pred>
    void RemIfOrder(Pred pred)
    { int d=0; for(int s=0;s<this->Used;s++) if(!pred(this->Data[s])) this->Data[d++] = this->Data[s]; this->Used=d; }
};

template<typename T> T *begin(sPoolArray<T> &a) { return a.GetData(); }
template<typename T> T *end(sPoolArray<T> &a) { return a.GetData()+a.GetCount(); }
template<typename T> const T *begin(const sPoolArray<T> &a) { return a.GetData(); }
template<typename T> const T *end(const sPoolArray<T> &a) { return a.GetData()+a.GetCount(); }


/****************************************************************************/
} // namespace Altona2
#endif  // FILE_ALTONA2_LIBS_BASE_CONTAINERS_HPP

/****************************************************************************/

#ifdef _MSC_VER
  #define __PLACEMENT_NEW_INLINE
  #define __PLACEMENT_VEC_NEW_INLINE
  #include <algorithm>
#endif

#include "util/algorithms.hpp"
#include "base/system.hpp"
#include "base/types2.hpp"

#ifdef __GNUC__
  #define _NEW
  #undef new
  
  namespace std { struct nothrow_t {}; nothrow_t nothrow; }
  void *operator new(size_t sz,std::nothrow_t) { return ::operator new(sz); }
  
  #include <algorithm>  
  
  #define new sDEFINE_NEW
#endif

// we need some "random" int array to sort for testing, so let's just use
// these here. (recognize them?)
static const sInt testInts[] = { 503,87,512,61,908,170,897,275,653,426,154,509,612,677,765,703 };
static const sInt nTestInts = sCOUNTOF(testInts);

/****************************************************************************/

static void InitTest(sInt *arr)
{
  for(sInt i=0;i<nTestInts;i++)
    arr[i] = testInts[i];
}

static void VerifySorted(sInt *x,sInt count)
{
  for(sInt i=1;i<count;i++)
    sVERIFY(x[i] >= x[i-1]);
}

static void VerifyRevSorted(sInt *x,sInt count)
{
  for(sInt i=0;i<count-1;i++)
    sVERIFY(x[i] >= x[i+1]);
}

/****************************************************************************/

static void TestBasicSort()
{
  // This just tests the insertion sort and heap sort functions; there's
  // no point running introsort on this small a dataset, it'll just use
  // insertion sort on it anyway.
  sInt x[nTestInts];

  // test plain direct insertion sort
  InitTest(x);
  sInsertionSort(sAll(x));
  VerifySorted(x,nTestInts);

  // test insertion sort with predicate (and reverse order)
  InitTest(x);
  sInsertionSort(sAll(x),sCmpGreater<sInt>());
  VerifyRevSorted(x,nTestInts);

  // test heapsort with direct compare
  InitTest(x);
  sHeapSort(sAll(x));
  VerifySorted(x,nTestInts);

  // test heapsort with predicate (and reverse order)
  InitTest(x);
  sHeapSort(sAll(x),sCmpGreater<sInt>());
  VerifyRevSorted(x,nTestInts);
}

/****************************************************************************/

typedef sPair<sInt,sString<4> > KVPair;

static void InitKVTest(KVPair *arr)
{
  for(sInt i=0;i<nTestInts;i++)
  {
    arr[i].Key = testInts[i];
    arr[i].Value.PrintF(L"%03d",arr[i].Key);
  }
}

static void InitKVIndir(KVPair **arr,KVPair *src)
{
  for(sInt i=0;i<nTestInts;i++)
    arr[i] = &src[i];
}

template<typename Range>
static void PrintKVRange(Range r)
{
  while(!r.IsEmpty())
    sPrintF(L"%s ",sPopHead(r).Value);
  sPrintF(L"\n");
}

static void PrintKV(const KVPair *arr)
{
  for(sInt i=0;i<nTestInts;i++)
    sPrintF(L"%s%c",arr[i].Value,(i==nTestInts-1) ? '\n' : ' ');
}

static void PrintKVIndir(KVPair * const *arr)
{
  for(sInt i=0;i<nTestInts;i++)
    sPrintF(L"%s%c",arr[i]->Value,(i==nTestInts-1) ? '\n' : ' ');
}

static void TestPredicates()
{
  KVPair x[nTestInts];
  KVPair *y[nTestInts];

  sPrintF(L"Ref member predicate test (mainly a compile test):\n");
  InitKVTest(x);
  sInsertionSort(sAll(x),sMemberLess(&KVPair::Key));
  PrintKV(x);

  InitKVTest(x);
  sHeapSort(sAll(x),sMemberGreater(&KVPair::Key));
  PrintKV(x);

  sPrintF(L"Ptr member predicate test (mainly a compile test):\n");
  InitKVTest(x);
  InitKVIndir(y,x);
  sInsertionSort(sAll(y),sMemPtrGreater(&KVPair::Key));
  PrintKVIndir(y);

  InitKVIndir(y,x);
  sHeapSort(sAll(y),sMemPtrLess(&KVPair::Key));
  PrintKVIndir(y);
}

/****************************************************************************/

struct KVList : public KVPair
{
  sDNode Node;
};

static void TestReverseRange()
{
  KVPair x[nTestInts];
  KVList lx[nTestInts];
  sDList<KVList,&KVList::Node> keysOrig;

  InitKVTest(x);
  for(sInt i=0;i<nTestInts;i++)
  {
    lx[i].Key = x[i].Key;
    lx[i].Value = x[i].Value;
    keysOrig.AddTail(&lx[i]);
  }

  sPrintF(L"Reverse range test:\n");
  sInsertionSort(sReverse(sAll(x)),sMemberGreater(&KVPair::Key));
  PrintKV(x);

  PrintKVRange(sAll(x));
  PrintKVRange(sReverse(sAll(x)));

  sPrintF(L"List range test:\n");
  PrintKVRange(sAll(keysOrig));
  PrintKVRange(sReverse(sAll(keysOrig)));
}

/****************************************************************************/

static void TestBinarySearch()
{
  static const sInt arr[6] = { 0,10,30,30,40,50 };
  static const sInt tests[][3] = // key/expected value of sLowerBound/expected value of sUpperBound
  {
    { -1, 0,0 }, // -1: out on the left side
    {  0, 0,1 }, //  0: right at the beginning
    { 20, 2,2 }, // 20: not in list
    { 30, 2,4 }, // 30: contained twice
    { 40, 4,5 }, // 40: in the list exactly once
    { 50, 5,6 }, // 50: right at the end
    { 55, 6,6 }, // 55: past the end
  };

  static sArrayRange<const sInt> rng = sAll(arr);
  
  // Non-predicate variants
  for(sInt i=0;i<sCOUNTOF(tests);i++)
  {
    sInt v = tests[i][0];
    sVERIFY(sLowerBound(rng,v) == tests[i][1]);
    sVERIFY(sUpperBound(rng,v) == tests[i][2]);
  }

  // Same with predicate variants
  for(sInt i=0;i<sCOUNTOF(tests);i++)
  {
    sInt v = tests[i][0];
    sVERIFY(sLowerBound(rng,v,sCmpLess<sInt>()) == tests[i][1]);
    sVERIFY(sUpperBound(rng,v,sCmpLess<sInt>()) == tests[i][2]);
  }

  // Tests for sAllEqual
  sVERIFY(sAllEqual(rng,-1).IsEmpty());
  sVERIFY(sAllEqual(rng,30) == sArrayRange<const sInt>(arr+2,arr+4));
  sVERIFY(!sAllEqual(rng,40).IsEmpty());
  sVERIFY(sAllEqual(rng,40).GetHead() == 40);
  sVERIFY(sAllEqual(rng,40).GetTail() == 40);

  // And the predicate variants
  sVERIFY(sAllEqual(rng,-1,sCmpLess<sInt>()).IsEmpty());
  sVERIFY(sAllEqual(rng,30,sCmpLess<sInt>()) == sArrayRange<const sInt>(arr+2,arr+4));
  sVERIFY(!sAllEqual(rng,40,sCmpLess<sInt>()).IsEmpty());
  sVERIFY(sAllEqual(rng,40,sCmpLess<sInt>()).GetHead() == 40);
  sVERIFY(sAllEqual(rng,40,sCmpLess<sInt>()).GetTail() == 40);
}

/****************************************************************************/

typedef void (*SortFunc1)(sStaticArray<sInt> &range);

template<typename T>
static void SortFuncHeapOld(sStaticArray<T> &range)
{
  sHeapSortUp(range);
}

template<typename T>
static void SortFuncHeapNew(sStaticArray<T> &range)
{
  sHeapSort(sAll(range));
}

template<typename T>
static void SortFuncSTL(sStaticArray<T> &range)
{
  std::sort(&range[0],&range[0] + range.GetCount());
}

template<typename T>
static void SortFuncIntro(sStaticArray<T> &range)
{
  sIntroSort(sAll(range));
}

static SortFunc1 sortFuncs1[] = { &SortFuncHeapOld<sInt>, &SortFuncHeapNew<sInt>, &SortFuncSTL<sInt>, &SortFuncIntro<sInt> };
static const sChar *sortFuncNames[sCOUNTOF(sortFuncs1)] = { L"heap old", L"heap new", L"stl", L"intro" };

static void PrintSortHeader()
{
  sPrintF(L"%12s ",L""); // space for sequence type
  for(sInt i=0;i<sCOUNTOF(sortFuncs1);i++)
    sPrintF(L"%10s ",sortFuncNames[i]);

  sPrint(L"\n");
}

static void ThrashCache()
{
  static const sInt thrashSize = 1024*1024;
  sRandomKISS rnd;
  
  rnd.Seed(1234);

  sU8 *mem = new sU8[thrashSize];
  for(sInt i=0;i<thrashSize;i++)
    mem[i] = i&255;

  // do random memory accesses all over
  for(sInt i=0;i<thrashSize;i++)
    sSwap(mem[rnd.Int(thrashSize)],mem[rnd.Int(thrashSize)]);

  delete[] mem;
}

static void TestSequence1(const sChar *desc,sArrayRange<sInt> &seq)
{
  static const sInt nAttempts = 3;
  sStaticArray<sInt> arr(seq.GetCount());

  sPrintF(L"%-12s ",desc);
  for(sInt i=0;i<sCOUNTOF(sortFuncs1);i++)
  {
    // report minimum runtime over nAttempts runs
    sInt minTime = 0x7fffffff;

    for(sInt j=0;j<nAttempts;j++)
    {
      arr.Clear();
      sCopyMem(arr.AddMany(seq.GetCount()),&seq[0],seq.GetCount()*sizeof(sInt));

      ThrashCache();

      sU64 start = sGetTimeUS();
      sortFuncs1[i](arr);
      minTime = sMin(minTime,(sInt) (sGetTimeUS() - start));
    }

    VerifySorted(&arr[0],arr.GetCount());

    sPrintF(L"%10d ",minTime);
  }

  sPrint(L"\n");
}

// sorting stress tests, part 1: large arrays of ints in various configurations
static void TestSort1()
{
  static const sInt count = 262144;
  sStaticArray<sInt> arr(count);
  sRandomKISS rand;
  arr.AddMany(count);
  sArrayRange<sInt> rng = sAll(arr);

  rand.Seed(123456);

  sPrint(L"\nSort timings (array of ints):\n");

  PrintSortHeader();

  // seq 1: increasing (already sorted)
  for(sInt i=0;i<count;i++)
    arr[i] = i;

  TestSequence1(L"increasing",rng);

  // seq 2: decreasing (reversed)
  for(sInt i=0;i<count;i++)
    arr[i] = count-1-i;

  TestSequence1(L"decreasing",rng);

  // seq 3: random (just random 32-bit numbers)
  for(sInt i=0;i<count;i++)
    arr[i] = rand.Int32();
  TestSequence1(L"random",rng);

  // seq 4: random4 (random numbers between 0 and 3)
  for(sInt i=0;i<count;i++)
    arr[i] = rand.Int(4);
  TestSequence1(L"random4",rng);

  // seq 5: random256 (random numbers between 0 and 255)
  for(sInt i=0;i<count;i++)
    arr[i] = rand.Int(256);
  TestSequence1(L"random256",rng);

  // seq 6: random64k (random numbers between 0 and 65535)
  for(sInt i=0;i<count;i++)
    arr[i] = rand.Int16();
  TestSequence1(L"random64k",rng);

  // seq 7: nearly increasing (2% inversions)
  for(sInt i=0;i<count;i++)
    arr[i] = i;

  for(sInt i=0;i<(count*2/100);i++)
    sSwap(arr[rand.Int(count)],arr[rand.Int(count)]);
  TestSequence1(L"nearIncr 2%",rng);

  // seq 8: nearly increasing, 2% random at the end
  for(sInt i=0;i<count;i++)
    arr[i] = i;

  for(sInt i=(count*98)/100;i<count;i++)
    arr[i] = rand.Int(count);
  TestSequence1(L"incr+rnd end",rng);
}

/****************************************************************************/

struct FakeVertex
{
  sF32 x,y,z;
  sU32 RandomPayload[12];

  void InitPayload(sRandomKISS &rand)
  {
    for(sInt i=0;i<sCOUNTOF(RandomPayload);i++)
      RandomPayload[i] = rand.Int32();
  }

  sBool operator <(const FakeVertex &b) const
  {
    if(x < b.x) return sTRUE; else if (x > b.x) return sFALSE;
    if(y < b.y) return sTRUE; else if (y > b.y) return sFALSE;
    return z < b.z;
  }
};

namespace std
{
  template<>
  void swap(FakeVertex &a,FakeVertex &b)
  {
    sU32 *pa = (sU32 *) &a.x;
    sU32 *pb = (sU32 *) &b.x;
    for(sU32 i=0;i<sizeof(FakeVertex)/sizeof(sInt);i++)
    {
      sU32 x = pa[i];
      pa[i] = pb[i];
      pb[i] = x;
    }
  }
}

template<>
void sSwap(FakeVertex &a,FakeVertex &b)
{
  sU32 *pa = (sU32 *) &a.x;
  sU32 *pb = (sU32 *) &b.x;
  for(sU32 i=0;i<sizeof(FakeVertex)/sizeof(sInt);i++)
  {
    sU32 x = pa[i];
    pa[i] = pb[i];
    pb[i] = x;
  }
}

typedef void (*SortFunc2)(sStaticArray<FakeVertex> &range);
static SortFunc2 sortFuncs2[] = { &SortFuncHeapOld<FakeVertex>, &SortFuncHeapNew<FakeVertex>, &SortFuncSTL<FakeVertex>, &SortFuncIntro<FakeVertex> };

static void TestSequence2(const sChar *desc,sArrayRange<FakeVertex> &seq)
{
  static const sInt nAttempts = 3;
  sStaticArray<FakeVertex> arr(seq.GetCount());

  sPrintF(L"%-12s ",desc);
  for(sInt i=0;i<sCOUNTOF(sortFuncs2);i++)
  {
    // report minimum runtime over nAttempts runs
    sInt minTime = 0x7fffffff;

    for(sInt j=0;j<nAttempts;j++)
    {
      arr.Clear();
      sCopyMem(arr.AddMany(seq.GetCount()),&seq[0],seq.GetCount()*sizeof(FakeVertex));

      ThrashCache();

      sU64 start = sGetTimeUS();
      sortFuncs2[i](arr);
      minTime = sMin(minTime,(sInt) (sGetTimeUS() - start));

      // verify that the sorted sequence is nondecreasing
      for(sInt i=1;i<arr.GetCount();i++)
        sVERIFY(!(arr[i] < arr[i-1]));
    }

    sPrintF(L"%10d ",minTime);
  }

  sPrint(L"\n");
}

// sorting stress tests, part 2: "vertex data"-like arrays with more expensive to compare
// keys and extra payload to move around
static void TestSort2()
{
  static const sInt count = 262144;
  static const sInt randFCount = 32; // no. of distinct of random floats (so there are double keys)
  sStaticArray<FakeVertex> arr(count);
  sRandomKISS rand;
  sF32 randFloats[randFCount];

  arr.AddMany(count);
  sArrayRange<FakeVertex> rng = sAll(arr);
  rand.Seed(123456);

  for(sInt i=0;i<randFCount;i++)
    randFloats[i] = rand.Float(2.0f) - 1.0f;

  sPrint(L"\nSort timings (fake vertex data):\n");

  PrintSortHeader();

  // prepare some random "vertex data"
  for(sInt i=0;i<count;i++)
  {
    arr[i].x = randFloats[rand.Int(randFCount)];
    arr[i].y = randFloats[rand.Int(randFCount)];
    arr[i].z = randFloats[rand.Int(randFCount)];
    arr[i].InitPayload(rand);
  }

  // seq 1: random vertex data (which we just prepared)
  TestSequence2(L"random",rng);

  // seq 2: sorted in increasing order
  sIntroSort(rng);
  TestSequence2(L"increasing",rng);

  // seq 3: sorted in decreasing order
  sIntroSort(rng,sCmpGreater<FakeVertex>());
  TestSequence2(L"decreasing",rng);

  // seq 4: nearly increasing (2% inversions)
  sIntroSort(rng);
  for(sInt i=0;i<(count*2/100);i++)
    sSwap(arr[rand.Int(count)],arr[rand.Int(count)]);
  TestSequence2(L"nearIncr 2%",rng);

  // seq 5: nearly increasing, 2% random at the end
  sIntroSort(rng);

  sInt start = (count*98)/100;
  for(sInt i=start;i<count;i++)
    sSwap(arr[i],arr[i+rand.Int(count-i)]);

  TestSequence2(L"incr+rnd end",rng);
}

/****************************************************************************/

// just a correctness test on tons of random arrays
static void TestSort3()
{
  static const sInt count = 10000;

  sRandomKISS rand;
  sFixedArray<sInt> arr(count);

  sPrint(L"Sort3");

  rand.Seed(2187462);

  for(sInt run=0;run<10000;run++)
  {
    sInt cnt = (count/2) + rand.Int(count/2);
    for(sInt i=0;i<cnt;i++)
      arr[i] = rand.Int32();

    sIntroSort(sArrayRange<sInt>(&arr[0],&arr[0]+cnt));
    VerifySorted(&arr[0],cnt);

    if((run%200) == 0)
      sPrint(L".");
  }

  sPrint(L"\n");
}

/****************************************************************************/

class PrimesSmallerThan
{
  sInt Counter,Max;

public:
  typedef const sInt ValueType;

  PrimesSmallerThan(sInt max) : Counter(2), Max(max) { }

  sBool IsEmpty() const         { return Counter >= Max; }
  const sInt &GetHead() const   { return Counter; }
  void RemHead()
  {
    while(++Counter < Max)
    {
      // is it a prime? if yes, return.
      sInt i;
      for(i=2;i*i<=Counter;i++)
        if(Counter % i == 0)
          break;

      if(i*i>Counter) // no divisors found
        return;
    }
  }
};

static void TestForEach()
{
  KVPair x[nTestInts];
  KVList lx[nTestInts];
  sDList<KVList,&KVList::Node> keysOrig;

  InitKVTest(x);
  for(sInt i=0;i<nTestInts;i++)
  {
    lx[i].Key = x[i].Key;
    lx[i].Value = x[i].Value;
    keysOrig.AddTail(&lx[i]);
  }

  sInsertionSort(sReverse(sAll(x)),sMemberGreater(&KVPair::Key));

  sPrint(L"Iteration using sFOREACH:\n");

  KVPair *pair;
  sFOREACH(sAll(x),pair)
    sPrintF(L"%s ",pair->Value);
  sPrint(L"\n");

  KVList *lpair;
  sFOREACH(sReverse(sAll(keysOrig)),lpair)
    sPrintF(L"%s ",lpair->Value);
  sPrint(L"\n");

  sPrint(L"Primes smaller than 100:\n");
  const sInt *pr;
  sFOREACH(PrimesSmallerThan(100),pr)
    sPrintF(L"%2d ",*pr);
  sPrint(L"\n");
}

/****************************************************************************/

void sMain()
{
  TestBasicSort();
  TestPredicates();
  TestReverseRange();
  TestBinarySearch();
  TestForEach();

  // run Sort3 before Sort1 and Sort2 since it's a correctness test, not a
  // performance test.
  TestSort3();

  TestSort1();
  TestSort2();
}

/****************************************************************************/


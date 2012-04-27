/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "blobheap.hpp"

#define TESTALL 0

/****************************************************************************/

sBlobHeap *sGlobalBlobHeap;

static void sInitGlobalBlobHeap()
{
  sGlobalBlobHeap = new sBlobHeap(128*1024*1024,32*1024*1024,4*1024);
}

static void sExitGlobalBlobHeap()
{
  delete sGlobalBlobHeap;
}

void sAddGlobalBlobHeap()
{
  static sInt once=1;
  if(once)
  {
    once = 0;
    sAddSubsystem(L"GlobalBlobHeap",0x80,sInitGlobalBlobHeap,sExitGlobalBlobHeap);
  }
}

/****************************************************************************/

void sBlobHeap::AddBuffer()
{
  Buffer *b;

  b = Buffers.AddMany(1);
  b->Start = (sPtr) sAllocMem(ChunkSize,BlockSize,0);
  b->End = b->Start+ChunkSize;
  sSortUp(Buffers,&Buffer::Start);

  sBlobHeapHandle *hnd = (sBlobHeapHandle *) b->Start;
  hnd->Ranges = (sBlobHeap::UseRange *) (b->Start + sizeof(sBlobHeapHandle));
  hnd->DataOffset = 0;
  hnd->Size = 0;
  hnd->RangeCount = 1;
  hnd->Ranges[0].Start = b->Start;
  hnd->Ranges[0].End = b->End;

  Free(hnd);
}

sBlobHeap::sBlobHeap(sDInt totalsize,sDInt chunksize,sDInt blocksize)
{
  sVERIFY(chunksize % blocksize == 0);
  sVERIFY(blocksize>=sizeof(FreeRange));
  BlockSize = blocksize;
  ChunkSize = chunksize;
  Buffers.HintSize(1+totalsize/chunksize);

  while(totalsize>0)
  {
    AddBuffer();

    totalsize -= chunksize;
  }

  sVERIFY(sIsPower2(BlockSize));

  TempUseRange.HintSize(512);
  TempUseRange2.HintSize(512);
#if TESTALL
  CheckFree();
#endif
}

sBlobHeap::~sBlobHeap()
{
  Buffer *b;
  sFORALL(Buffers,b)
    sFreeMem((void *)b->Start);
}

void sBlobHeap::GetStats(sPtr &free_,sInt &frags_)
{
  FreeRange *fr;
  sPtr free = 0;
  sInt frags = 0;
  Lock.Lock();
  sFORALL_LIST(FreeList,fr)
  {
    free += fr->End - fr->Start;
    frags ++;
  }
  Lock.Unlock();

  free_ = free;
  frags_ = frags;
}

sPtr sBlobHeap::GetTotalSize()
{
  sPtr total = 0;
  Buffer *b;

  Lock.Lock();
  sFORALL(Buffers,b)
    total += b->End - b->Start;
  Lock.Unlock();

  return total;
}

void sBlobHeap::CheckFree()
{
  FreeRange *fr;
  FreeRange *last = 0;
  sInt bn = 0;
  sFORALL_LIST(FreeList,fr)
  {
    // should be larger than zero and aligned to blocksize
    
    sVERIFY(sPtr(fr)==fr->Start);
    sVERIFY(fr->Start<fr->End);     
    sVERIFY((fr->Start & (BlockSize-1))==0);
    sVERIFY((fr->End & (BlockSize-1))==0);

    // should be inside buffer

    while(!(fr->Start<Buffers[bn].End && fr->End>Buffers[bn].Start))
    {
      bn++;
      sVERIFY(bn<Buffers.GetCount());
    }

    // should be sorted and not zusammenhängend

    if(last)                
      sVERIFY(last->End < fr->Start);
    last = fr;
  }
}

void sBlobHeap::CheckUsed(sBlobHeapHandle *hnd)
{
  sPtr last = 0;
  Buffer *b;
  for(sInt i=0;i<hnd->RangeCount;i++)
  {
    UseRange *ur = &hnd->Ranges[i];

    // should be larger than zero and aligned by blocksize

    sVERIFY(ur->Start < ur->End);
    sVERIFY((ur->Start & (BlockSize-1))==0);
    sVERIFY((ur->End & (BlockSize-1))==0);

    // should be inside buffer

    sFORALL(Buffers,b)
      if(ur->Start<b->End && ur->End>b->Start)
        goto ok;
    sFatal(L"sBlobHeap: handle range outside buffers");
ok:;

    // should be sorted and not zusammenhängend

//    sVERIFY(ur->Start > last);   // not any more
    last = ur->End;
  }
}

/****************************************************************************/

sBlobHeap::FreeRange *sBlobHeap::Alloc(FreeRange *fr,UseRange &ur,sPtr min,sPtr max)
{
  for(;;)
  {
    while(!FreeList.IsStart(fr))
    {
      if(fr->End-fr->Start >= min)
      {
        sPtr chunk = sMin(max,fr->End-fr->Start);
        ur.Start = fr->End-chunk;
        ur.End = fr->End;
        if(fr->End-fr->Start == chunk)    // get all
        {
          FreeRange *t = FreeList.GetNext(fr);
          FreeList.Rem(fr);
          return FreeList.GetPrev(t);
        }
        else                              // just steal some
        {
          sVERIFY(fr->End>chunk);
          fr->End -= chunk;
          return fr;
        }
      }
      fr = FreeList.GetPrev(fr);
    }
    sVERIFY(max*2<ChunkSize);
    sDPrintF(L"sBlobHeap eats more memory\n");
    AddBuffer();
    fr = FreeList.GetTail();
  }
}

sBlobHeapHandle *sBlobHeap::Alloc(sDInt size_)
{
  sPtr size = sAlign(size_,16) + sAlign(sizeof(sBlobHeapHandle),16) + sizeof(UseRange);
  sDInt left = size;

  Lock.Lock();

  // allocate chunks

  TempUseRange.Clear();
  UseRange r;
  FreeRange *fr = FreeList.GetTail();
  while(left>0)
  {
    fr = Alloc(fr,r,BlockSize,sAlign(left,BlockSize));
    TempUseRange.AddTail(r);
    left -= r.End-r.Start;
  }

  // allocate ranges

  UseRange *ranges = 0;
  sInt rangesize = sizeof(UseRange)*TempUseRange.GetCount();
  if(rangesize < -left)  // can squeeze in
  {
    ranges = (UseRange *)(r.End+left);
  }
  else
  {
    rangesize = sAlign(rangesize+sizeof(UseRange),BlockSize);
    fr = Alloc(fr,r,rangesize,rangesize);
    TempUseRange.AddTail(r);
    ranges = (UseRange *)r.Start;
  }
  sCopyMem(ranges,TempUseRange.GetData(),sizeof(UseRange)*TempUseRange.GetCount());
  
  // make header

  sBlobHeapHandle *hnd = (sBlobHeapHandle *) TempUseRange[0].Start;
  hnd->DataOffset = sAlign(sizeof(sBlobHeapHandle),16);
  hnd->RangeCount = TempUseRange.GetCount();
  hnd->Ranges = ranges;
  hnd->Size = size;

  // done

  Lock.Unlock();

#if TESTALL
  CheckFree();
  CheckUsed(hnd);
#endif
  return hnd;
}

void sBlobHeap::Free(sBlobHeapHandle *hnd)
{
  if(hnd==0) return;

  Lock.Lock();

#if TESTALL
  CheckUsed(hnd);
  CheckFree();
  sPtr totalfree,free;
  sInt frags;
  GetStats(free,frags);
  totalfree = free;
  for(sInt i=0;i<hnd->RangeCount;i++)
    totalfree += hnd->Ranges[i].End - hnd->Ranges[i].Start;
#endif  
  // copy used ranges, becaus we might be changing the memory
  
  TempUseRange2.Resize(hnd->RangeCount);
  sCopyMem(TempUseRange2.GetData(),hnd->Ranges,sizeof(UseRange)*hnd->RangeCount);

  // ranges are usually sorted. if not, sort them

  sBool sorted = 1;
  for(sInt i=1;i<TempUseRange2.GetCount() && sorted;i++)
    if(TempUseRange2[i-1].Start < TempUseRange2[i].Start)
      sorted = 0;
  if(!sorted)
    sSortDown(TempUseRange2,&UseRange::Start);

  // prepare

  UseRange *ur = TempUseRange2.GetData();
  sInt left = hnd->RangeCount;
  FreeRange *fr = FreeList.GetTail();
  FreeRange *nr = 0;

  // fit inbetween

  while(left>0 && !FreeList.IsStart(fr))
  {
    if(ur->Start >= fr->End)
    {
      if(ur->Start==fr->End)
      {
        fr->End = ur->End;
      }
      else
      {
        nr = (FreeRange *) ur->Start;
        nr->Start = ur->Start;
        nr->End = ur->End;
        FreeList.AddAfter(nr,fr);
        fr = nr;
/*
        FreeRange *next = FreeList.GetNext(fr);
        if(!FreeList.IsLast(fr) && nr->End==next->Start)
        {
          nr->End = next->End;
          FreeList.Rem(next);
        }
*/
      }
      ur++;
      left--;

      FreeRange *next = FreeList.GetNext(fr);
      if(!FreeList.IsEnd(next) && fr->End==next->Start)
      {
        fr->End = next->End;
        FreeList.Rem(next);
      }
    }
    else
    {
      fr = FreeList.GetPrev(fr);
    }
  }

  // fit at end (both cases are not tested!)

  if(left>0)
  {
    if(!FreeList.IsEmpty())
    {
      fr = FreeList.GetHead();
      if(fr->Start==ur->End)
      {
        nr = (FreeRange *) ur->Start;
        nr->Start = ur->Start;
        nr->End = fr->End;
        FreeList.AddHead(nr);
        FreeList.Rem(fr);
        ur++;
        left--;
      }
    }
    while(left>0)
    {
      nr = (FreeRange *) ur->Start;
      nr->Start = ur->Start;
      nr->End = ur->End;
      FreeList.AddHead(nr);
      ur++;
      left--;
    }
  }

#if TESTALL
  CheckFree();
  GetStats(free,frags);
  sVERIFY(free==totalfree);
#endif

  Lock.Unlock();
}

/****************************************************************************/

void sBlobHeap::CopyFrom(sBlobHeapHandle *hnd,void *d_,sPtr sn,sPtr size)
{
  sn += hnd->DataOffset;
  sU8 *d = (sU8 *) d_;

  sVERIFY(sn+size<=hnd->Size);

  UseRange *ur = hnd->Ranges;
 
  // seek to required range

  while(sn >= ur->End-ur->Start)
  {
    sVERIFY(ur-hnd->Ranges<hnd->RangeCount);

    sn -= (ur->End-ur->Start);
    ur++;
  }
  sVERIFY(sn < ur->End-ur->Start);

  // copy

  while(size>0)
  {
    sVERIFY(ur-hnd->Ranges<hnd->RangeCount);

    sDInt batch = sMin(size,ur->End-(ur->Start+sn));
    sCopyMem(d,(sU8*)(ur->Start+sn),batch);
    d+= batch;
    size-=batch;
    sn = 0;
    ur++;
  }
}

void sBlobHeap::CopyFrom_UnpackIndex(sBlobHeapHandle *hnd,void *d_,sPtr sn,sPtr size,sU32 v)
{
  sn += hnd->DataOffset;
  sU8 *d = (sU8 *) d_;

  sVERIFY(sn+size<=hnd->Size);

  UseRange *ur = hnd->Ranges;
 
  // seek to required range

  while(sn >= ur->End-ur->Start)
  {
    sVERIFY(ur-hnd->Ranges<hnd->RangeCount);

    sn -= (ur->End-ur->Start);
    ur++;
  }
  sVERIFY(sn < ur->End-ur->Start);

  // copy

  while(size>0)
  {
    sVERIFY(ur-hnd->Ranges<hnd->RangeCount);

    sDInt batch = sMin(size,ur->End-(ur->Start+sn));

    {
      const sU16 *ss = (const sU16 *)(ur->Start+sn);
      sU32 *dd = (sU32 *) d;
      for(sInt i=0;i<batch/2;i++)
        dd[i] = ss[i] + v;
    }

    d+= batch*2;
    size-=batch;
    sn = 0;
    ur++;
  }
}


void sBlobHeap::CopyInto(sBlobHeapHandle *hnd,sPtr dn,const void *s_,sPtr size)
{
  dn += hnd->DataOffset;
  const sU8 *s = (const sU8 *) s_;

  sVERIFY(dn+size<=hnd->Size);

  // seek to required range

  UseRange *ur = hnd->Ranges;

  while(dn >= ur->End-ur->Start)
  {
    sVERIFY(ur-hnd->Ranges<hnd->RangeCount);

    dn -= (ur->End-ur->Start);
    ur++;
  }
  sVERIFY(dn < ur->End-ur->Start);

  // copy

  while(size>0)
  {
    sVERIFY(ur-hnd->Ranges<hnd->RangeCount);

    sDInt batch = sMin(size,ur->End-(ur->Start+dn));
    sCopyMem((sU8*)(ur->Start+dn),s,batch);
    s+= batch;
    size-=batch;
    dn = 0;
    ur++;
  }
}

/****************************************************************************/


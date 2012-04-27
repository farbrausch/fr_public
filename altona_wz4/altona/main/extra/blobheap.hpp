/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_EXTRA_BLOBALLOCATOR_HPP
#define FILE_EXTRA_BLOBALLOCATOR_HPP

#include "base/types2.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Put blobs in memory. more efficient than normal heap because:      ***/
/***                                                                      ***/
/***   Only operation allowed are                                         ***/
/***   * alloc                                                            ***/
/***   * free                                                             ***/
/***   * copy into                                                        ***/
/***   * copy from                                                        ***/
/***                                                                      ***/
/***   * no direct access to memory                                       ***/
/***   * alignment requirement for source and destinatin: 16 bytes        ***/
/***                                                                      ***/
/***                                                                      ***/
/***   internally, storage may be split into multiple ranges              ***/
/***   based on 4k pages                                                  ***/
/***                                                                      ***/
/***                                                                      ***/
/***   i use this to store index/vertex buffers that are sometimes        ***/
/***   used to refill dynamic geometry                                    ***/
/***                                                                      ***/
/**************************************************************************+*/

struct sBlobHeapHandle;

class sBlobHeap
{
  friend struct sBlobHeapHandle;
  struct FreeRange
  {
    sDNode Node;
    sPtr Start;
    sPtr End;
  };
  struct Buffer
  {
    sPtr Start;
    sPtr End;
  };
  struct UseRange
  {
    sPtr Start;
    sPtr End;
  };


  sArray<Buffer> Buffers;
  sDList2<FreeRange> FreeList;

  sPtr BlockSize;
  sPtr ChunkSize;
  sArray<UseRange> TempUseRange;
  sArray<UseRange> TempUseRange2;
  
  void CheckFree();
  void CheckUsed(sBlobHeapHandle *);
  FreeRange *Alloc(FreeRange *fr,UseRange &ur,sPtr min,sPtr max);
  void AddBuffer();
  sThreadLock Lock;
public:
  sBlobHeap(sDInt totalsize=1024*1024*1024,sDInt chunksize=128*1024*1024,sDInt blocksize=4*1024);
  ~sBlobHeap();
  void GetStats(sPtr &free,sInt &frags);
  sPtr GetTotalSize();

  sBlobHeapHandle *Alloc(sDInt size);
  void Free(sBlobHeapHandle *);   // this is rather slow, since it has to go through the free-list. putting free-list in array would enable binary search and is better for cache!
  void CopyFrom(sBlobHeapHandle *,void *d,sPtr s,sPtr size);
  void CopyFrom_UnpackIndex(sBlobHeapHandle *,void *d,sPtr s,sPtr sourcesize,sU32 v);
  void CopyInto(sBlobHeapHandle *,sPtr d,const void *s,sPtr size);
};

struct sBlobHeapHandle
{
  sBlobHeap::UseRange *Ranges;    // the actual ranges. must not be fragmentated
  sPtr DataOffset;                // offset into data, since ranges are stored first
  sPtr Size;                      // total size of blob (excluding ranges)
  sInt RangeCount;                // count ranges
}; 

void sAddGlobalBlobHeap();

extern sBlobHeap *sGlobalBlobHeap;

/****************************************************************************/

#endif // FILE_EXTRA_BLOBALLOCATOR_HPP


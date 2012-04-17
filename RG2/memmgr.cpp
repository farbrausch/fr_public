// This code is in the public domain. See LICENSE for details.

// memmgr.cpp: implementations of the bufferMemoryManager interface

#pragma warning (disable: 4786)

#include "memmgr.h"
#include "types.h"
#include <map>
#include <list>
#include <vector>
#include <algorithm>
#include <string.h>
#include "debug.h"
#include "sync.h"

namespace fr
{
  // ---- simpleBufferMemoryManager

  sU32 simpleBufferMemoryManager::alloc(sU32 size)
  {
    return reinterpret_cast<sU32>(new sU8[size]);
  }

  void simpleBufferMemoryManager::dealloc(sU32 id)
  {
    delete[] reinterpret_cast<sU8*>(id);
  }

  void *simpleBufferMemoryManager::lock(sU32 id, sBool &lost)
  {
    lost=sFALSE;
    return reinterpret_cast<void*>(id);
  }

  void simpleBufferMemoryManager::unlock(sU32 id)
  {
  }

  // ---- poolBufferMemoryManager

  struct poolBufferMemoryManager::privateData
  {
    struct bufferInfo
    {
      sU32  size;
      mutable sU8   *ptr;   // having to make this mutable sucks...
      mutable sUInt locks;
      
      bufferInfo(const sU32 initSize=0)
        : size(initSize), ptr(0), locks(0)
      {
      }

      bufferInfo(const bufferInfo &x)
        : size(x.size), ptr(x.ptr), locks(x.locks)
      {
        x.locks = 0; // not really a copy, more a "transfer ownership" operation
        x.ptr = 0;
      }
      
      ~bufferInfo()
      {
        FRDASSERT(locks == 0);
        FRDASSERT(ptr == 0);
      }
    };

    struct interval
    {
      sU32    start, len;
      sU32    id;

      interval(sU32 aStart, sU32 aLen, sU32 aID)
        : start(aStart), len(aLen), id(aID)
      {
      }

      interval(const interval &x)
        : start(x.start), len(x.len), id(x.id)
      {
      }

      bool operator < (const interval &b) const
      {
        return start < b.start;
      }
    };
    
    typedef std::vector<bufferInfo>       bufInfoVec;
    typedef bufInfoVec::iterator          bufInfoVecIt;
    typedef bufInfoVec::const_iterator    bufInfoVecCIt;
    
    typedef std::list<sU32>               handleList;
    typedef handleList::iterator          handleListIt;
    typedef handleList::const_iterator    handleListCIt;

    typedef std::vector<interval>         allocVector;
    typedef allocVector::iterator         allocVectorIt;
    typedef allocVector::const_iterator   allocVectorCIt;
    
    bufInfoVec      m_buffers;
    handleList      m_accesses;
    handleList      m_freeList;

    sU32            m_poolSize;
    sU32            m_totalLockCount;
		sU8*						m_pool;

    sU32            m_freeSpace, m_cleanUpThreshold;
		fr::cSection		m_sync;
    
    allocVector     m_allocations;

    privateData(sU32 poolSize)
      : m_poolSize(poolSize), m_totalLockCount(0)
    {
      m_pool = new sU8[m_poolSize];
      m_freeSpace = m_poolSize;
      m_cleanUpThreshold = (m_freeSpace + 9) / 10;
    }
    
    ~privateData()
    {
      for (bufInfoVecIt it = m_buffers.begin(); it != m_buffers.end(); ++it)
      {
        if (it->size != sU32(-1)) // not freed yet?
          freeBuffer(it - m_buffers.begin());
      }

      delete[] m_pool;
    }
    
    void addAllocation(sU32 start, sU32 len, sU32 id)
    {
      allocVectorIt insertPos = m_allocations.begin();
      while (insertPos != m_allocations.end() && insertPos->start < start)
        ++insertPos;

      m_allocations.insert(insertPos, interval(start, len, id));
      m_freeSpace -= len;
    }
    
    void removeAllocation(sU32 start)
    {
      for (allocVectorIt it = m_allocations.begin(); it != m_allocations.end(); ++it)
      {
        if (it->start == start)
        {
          m_freeSpace += it->len;
          m_allocations.erase(it); // sorting stays intact
          break;
        }
      }
    }
    
    sInt findAllocationSpace(sU32 size)
    {
      sU32 p0 = 0, p1;
      for (allocVectorCIt it = m_allocations.begin(); it != m_allocations.end(); ++it)
      {
        p1 = it->start;
        if (p1 - p0 >= size)
          return p0;

        p0 = p1 + it->len;
      }

      if (m_poolSize - p0 >= size)
        return p0;
      else
        return -1;
    }

    sU32 allocBuffer(sU32 size)
    {
      sU32 hnd;

      if (m_freeList.size())
      {
        hnd = m_freeList.back();
        m_freeList.pop_back();
        m_buffers[hnd] = bufferInfo(size);
      }
      else
      {
        hnd = m_buffers.size();
        m_buffers.push_back(bufferInfo(size));
      }

      m_accesses.push_back(hnd);

      return hnd;
    }

    void freeBuffer(sU32 hnd)
    {
      FRDASSERT(isValidBufferHandle(hnd));

      m_buffers[hnd].size = (sU32) -1;

      freeMemoryOf(m_buffers[hnd]);
      m_freeList.push_back(hnd);
      m_accesses.remove(hnd);
    }

    sBool isValidBufferHandle(sU32 hnd) const
    {
      if (hnd >= m_buffers.size())
        return sFALSE;

      for (handleListCIt it = m_freeList.begin(); it != m_freeList.end(); ++it)
      {
        if (*it == hnd)
          return sFALSE;
      }

      return sTRUE;
    }
    
    inline void freeMemoryOf(bufferInfo& inf)
    {
      if (inf.ptr)
        removeAllocation(inf.ptr - m_pool);

      inf.ptr = 0;
    }
    
    inline sU8 *allocate(sU32 id, sU32 size)
    {
      sInt pos;
      
      while ((pos = findAllocationSpace(size)) == -1)
      {
        handleList::reverse_iterator it;
        for (it = m_accesses.rbegin(); it != m_accesses.rend(); it++)
        {
          bufferInfo& inf = m_buffers[*it];
          
          if (inf.ptr && inf.locks == 0)
          {
            freeMemoryOf(inf);
            tidyUp();
            break;
          }
        }

        if (it == m_accesses.rend()) // nothing found? this is fatal, but print some info first
        {
          fr::debugOut("poolBufferMemoryManager::<internal>::allocate failed. stats dump:\n");

          fr::debugOut("buffers:\n");
          sInt bufCount=0;

          for (sUInt bind = 0; bind < m_buffers.size(); ++bind)
          {
            if (m_buffers[bind].size != sU32(-1))
              fr::debugOut("  %4d. id=%d, size=%d, ptr=%08x, %d locks\n", ++bufCount, bind, m_buffers[bind].size, m_buffers[bind].ptr, m_buffers[bind].locks);
            else
              fr::debugOut("  xxxx. free id %d\n", bind);
          }

          fr::debugOut("\nallocs:\n");
          for (allocVectorCIt it = m_allocations.begin(); it != m_allocations.end(); ++it)
            fr::debugOut("  start=%d size=%d buffer=%d\n", it->start, it->len, it->id);

          fr::errorExit("poolBufferMemoryManager::<internal>::allocate failed! (buf %d, size %d)", id, size);
        }
      }

      addAllocation(pos, size, id);
      return m_pool + pos;
    }
    
    void tidyUp()
    {
      if (m_totalLockCount != 0)
        return;

      sU32 pos = 0;
      for (allocVectorIt it = m_allocations.begin(); it != m_allocations.end(); ++it)
      {
        const sU8* src = m_pool + it->start;
        sU8* dst = m_pool + pos;

        FRDASSERT(dst <= src);

        if (dst != src)
          memmove(dst, src, it->len);

        it->start = pos;
        pos += it->len;

        FRDASSERT(isValidBufferHandle(it->id));
        bufferInfo& inf = m_buffers[it->id];
        
        inf.ptr = dst;
      }
    }

		inline sBool accessesListValid()
		{
			for (handleListIt it = m_accesses.begin(); it != m_accesses.end(); ++it)
			{
				if (!isValidBufferHandle(*it))
					return sFALSE;

				for (handleListIt it2 = m_accesses.begin(); it2 != it; ++it2)
				{
					if (*it == *it2)
						return sFALSE;
				}
			}

			return sTRUE;
		}
  };
  
  poolBufferMemoryManager::poolBufferMemoryManager(sU32 size)
  {
    d=new privateData(size);
  }

  poolBufferMemoryManager::~poolBufferMemoryManager()
  {
    delete d;
  }

  sU32 poolBufferMemoryManager::alloc(sU32 size)
  {
		fr::cSectionLocker lock(d->m_sync);
    return d->allocBuffer(size);
  }

  void poolBufferMemoryManager::dealloc(sU32 id)
  {
		fr::cSectionLocker lock(d->m_sync);
    if (d->isValidBufferHandle(id))
		{
      d->freeBuffer(id);
			FRDASSERT(d->accessesListValid());
		}
  }

  void *poolBufferMemoryManager::lock(sU32 id, sBool &lost)
  {
		fr::cSectionLocker lock(d->m_sync);

    if (!d->isValidBufferHandle(id))
      return 0;

    privateData::bufferInfo& inf = d->m_buffers[id];

    if (!inf.ptr)
    {
      lost = sTRUE;
      inf.ptr = d->allocate(id, inf.size);
    }
    else
      lost = sFALSE;

    privateData::handleListIt ait = d->m_accesses.begin();
    while (ait != d->m_accesses.end() && *ait != id)
      ++ait;

		FRDASSERT(ait != d->m_accesses.end());

    if (ait != d->m_accesses.begin())
    {
      sU32 newFront = *ait;
			d->m_accesses.erase(ait);
      d->m_accesses.push_front(newFront);
			FRDASSERT(d->accessesListValid());
    }

    d->m_totalLockCount++;
    inf.locks++;

    return inf.ptr;
  }

  void poolBufferMemoryManager::unlock(sU32 id)
  {
		fr::cSectionLocker lock(d->m_sync);

    if (d->isValidBufferHandle(id))
    {
      privateData::bufferInfo& inf = d->m_buffers[id];

      inf.locks--;
      if (d->m_freeSpace <= d->m_cleanUpThreshold)
        d->tidyUp();
    }
  }
}

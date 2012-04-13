// This code is in the public domain. See LICENSE for details.

#ifndef __fr_memmgr_h_
#define __fr_memmgr_h_

#include "types.h"

namespace fr
{
  class bufferMemoryManager  
  {
  public:
    bufferMemoryManager()              {}
    virtual ~bufferMemoryManager()     {}
    
    virtual sU32    alloc(sU32 size)=0; // size is in bytes
    virtual void    dealloc(sU32 id)=0;
    
    virtual void    *lock(sU32 id, sBool &lost)=0;
    virtual void    unlock(sU32 id)=0;
  };

  class simpleBufferMemoryManager: public bufferMemoryManager
  {
  public:
    virtual sU32    alloc(sU32 size);
    virtual void    dealloc(sU32 id);

    virtual void    *lock(sU32 id, sBool &lost);
    virtual void    unlock(sU32 id);
  };

  class poolBufferMemoryManager: public bufferMemoryManager
  {
  public:
    poolBufferMemoryManager(sU32 size);
    ~poolBufferMemoryManager();

    virtual sU32    alloc(sU32 size);
    virtual void    dealloc(sU32 id);

    virtual void    *lock(sU32 id, sBool &lost);
    virtual void    unlock(sU32 id);

  private:
    struct privateData;

    privateData     *d;
  };
}

#endif

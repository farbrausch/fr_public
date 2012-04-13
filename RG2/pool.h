// This code is in the public domain. See LICENSE for details.

#ifndef __fvs_pool_h_
#define __fvs_pool_h_

#include <list>
#include "types.h"
#include "fvsconfig.h"

#ifdef FVS_SUPPORT_MULTITHREADING
#define  WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace fvs
{
  template<class T> class FR_NOVTABLE pool
  {
  protected:
    struct poolEntry
    {
      sChar id[256];            ///< ID des Objekts
      sInt  refs;               ///< Referenzzahl
      sInt  size;               ///< Größe des Objekts
      T     obj;                ///< Das eigentliche Objekt
    };

    typedef   std::list<poolEntry> poolList;
    poolList  thePool;

#ifdef FVS_SUPPORT_MULTITHREADING
    HANDLE    poolMutex;        ///< Handle für einen Pool-Mutex (Pool-Operationen sind atomisch)
#endif

    virtual   sBool rawGet(const sChar *id, T *ptr, sInt *size)=0;
    virtual   void rawDestroy(T &elem)=0;

  public:
    pool()
    {
#ifdef FVS_SUPPORT_MULTITHREADING
      poolMutex=CreateMutex(0, FALSE, 0);
#endif
    }

    virtual ~pool()
    {
      releaseAll();
#ifdef FVS_SUPPORT_MULTITHREADING
      CloseHandle(poolMutex);
#endif
    }

    void add(const sChar *id, T &what, sInt size)
    {
      poolEntry p;

      strncpy(p.id, id, 255);
      p.id[255]=0;
      p.refs=1;
      p.size=size;
      p.obj=what;

#ifdef FVS_SUPPORT_MULTITHREADING
      WaitForSingleObject(poolMutex, INFINITE);
#endif
      thePool.push_front(p);
#ifdef FVS_SUPPORT_MULTITHREADING
      ReleaseMutex(poolMutex);
#endif
    }

    sInt count()
    {
      return thePool.size();
    }

    sInt refCount(const sChar *id)
    {
      sInt refs=0;

#ifdef FVS_SUPPORT_MULTITHREADING
      WaitForSingleObject(poolMutex, INFINITE);
#endif
      
      for (poolList::iterator it=thePool.begin(); it!=thePool.end(); it++)
        if (!strnicmp(id, it->id, 255))
          refs=it->refs;

#ifdef FVS_SUPPORT_MULTITHREADING
        ReleaseMutex(poolMutex);
#endif
      
      return refs;
    }

    void enumerate(sBool (*callback)(T &what, const sChar *id, sInt count, void *user), void *user)
    {
      poolList::iterator  it;
      sInt                count=0;

      if (!callback)
        return;

#ifdef FVS_SUPPORT_MULTITHREADING
      WaitForSingleObject(poolMutex, INFINITE);
#endif
      
      for (it=thePool.begin(); it!=thePool.end(); it++)
      {
        if (callback(it->obj, it->id, count++, user)==sFALSE)
          break;
      }

#ifdef FVS_SUPPORT_MULTITHREADING
      ReleaseMutex(poolMutex);
#endif
    }

    sBool get(const sChar *id, T *ptr, sInt *size=0)
    {
      poolList::iterator  it;
      poolEntry           p;
      sBool               res=sFALSE;

#ifdef FVS_SUPPORT_MULTITHREADING
      WaitForSingleObject(poolMutex, INFINITE);
#endif
      
      for (it=thePool.begin(); it!=thePool.end(); it++)
        if (!strnicmp(id, it->id, 255))
        {
          it->refs++;
		      p=*it;
		      res=sTRUE;
		      break;
        }
	  
	    if (!res)
	    {
		    strncpy(p.id, id, 255);
		    p.id[255]=0;
		    p.refs=1;

	      if ((res=rawGet(id, &p.obj, &p.size)))
			    thePool.push_front(p);
	    }

      if (res)
	    {
	      *ptr=p.obj;
	      if (size)
		      *size=p.size;
	    }

#ifdef FVS_SUPPORT_MULTITHREADING
      ReleaseMutex(poolMutex);
#endif
      
      return res;
    }

    void release(T &obj)
    {
      poolList::iterator  it;

      if (!thePool.size())
        return;

#ifdef FVS_SUPPORT_MULTITHREADING
      WaitForSingleObject(poolMutex, INFINITE);
#endif
      
      for (it=thePool.begin(); it!=thePool.end(); it++)
      {
        if (it->obj==obj)
          if (!--it->refs)
		      {
			      rawDestroy(it->obj);
            thePool.erase(it);
            break;
		      }
      }

#ifdef FVS_SUPPORT_MULTITHREADING
      ReleaseMutex(poolMutex);
#endif
    }

    void releaseAll()
    {
#ifdef FVS_SUPPORT_MULTITHREADING
      WaitForSingleObject(poolMutex, INFINITE);
#endif
      
      while (thePool.size())
      {
        rawDestroy(thePool.front().obj);
        thePool.pop_front();
      }

#ifdef FVS_SUPPORT_MULTITHREADING
      ReleaseMutex(poolMutex);
#endif
    }
  };

  template<class T> class FR_NOVTABLE cache: public pool<T>
  {
  protected:
    sU32  maxSize;
    sU32  curSize;

    void manageMemory()
    {
#ifdef FVS_SUPPORT_MULTITHREADING
      WaitForSingleObject(poolMutex, INFINITE);
#endif
      
      while (curSize>maxSize)
      {
        poolList::iterator  it, bestmatch;
        sU32                delta, bestdelta;

        delta=curSize-maxSize;
        bestdelta=(sU32) -1;
        bestmatch=thePool.end();
    
        for (it=thePool.begin(); it!=thePool.end(); ++it)
        {
          sU32 myDelta;

          if (it->refs)
            continue;

          if (it->size>delta)
            myDelta=it->size-delta;
          else
            myDelta=delta-it->size;

          if (myDelta<bestdelta)
          {
            bestdelta=myDelta;
            bestmatch=it;
          }
        }

        if (bestmatch==thePool.end())
          break;

        curSize-=bestmatch->size;
        rawDestroy(bestmatch->obj);
        thePool.erase(bestmatch);
      }

#ifdef FVS_SUPPORT_MULTITHREADING
      ReleaseMutex(poolMutex);
#endif
    }

  public:
    cache(sU32 cacheSize=1048576)
    {
      maxSize=cacheSize;
      curSize=0;
    }

    sBool get(const sChar *id, T *ptr, sInt *size=0)
    {
      sBool res=pool<T>::get(id, ptr, size);
      manageMemory();
      return res;
    }

    void release(T &obj)
    {
      poolList::iterator  it;
    
      if (!thePool.size())
        return;

#ifdef FVS_SUPPORT_MULTITHREADING
      WaitForSingleObject(poolMutex, INFINITE);
#endif
    
      for (it=thePool.begin(); it!=thePool.end(); it++)
      {
        if (it->obj==obj)
          if (it->refs>0)
            it->refs--;
      }

#ifdef FVS_SUPPORT_MULTITHREADING
      ReleaseMutex(poolMutex);
#endif
    }
  };
}

#endif

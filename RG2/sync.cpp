// This code is in the public domain. See LICENSE for details.

#include <windows.h>
#include "sync.h"

namespace fr
{
  // ---- frCSection

  cSection::cSection()
  {
    InitializeCriticalSection(&m_cSection);
  }

  cSection::~cSection()
  {
    DeleteCriticalSection(&m_cSection);
  }

  void cSection::enter()
  {
    EnterCriticalSection(&m_cSection);
  }
  
  sBool cSection::tryEnter()
  {
#if 0
    return !!TryEnterCriticalSection(&m_cSection);
#else
    enter();
    return sTRUE;
#endif
  }

  void cSection::leave()
  {
    LeaveCriticalSection(&m_cSection);
  }

  // ---- mutex

  mutex::mutex()
  {
    m_hMutex=INVALID_HANDLE_VALUE;
  }

  mutex::mutex(const mutex &x)
  {
    if (x.m_hMutex!=INVALID_HANDLE_VALUE)
    {
      if (!DuplicateHandle(GetCurrentProcess(), x.m_hMutex, GetCurrentProcess(),
        &m_hMutex, 0, FALSE, DUPLICATE_SAME_ACCESS))
        m_hMutex=INVALID_HANDLE_VALUE;
    }
    else
      m_hMutex=INVALID_HANDLE_VALUE;
  }

  mutex::mutex(const HANDLE hnd)
  {
    if (hnd!=INVALID_HANDLE_VALUE)
    {
      if (!DuplicateHandle(GetCurrentProcess(), hnd, GetCurrentProcess(), &m_hMutex, 0,
        FALSE, DUPLICATE_SAME_ACCESS))
        m_hMutex=INVALID_HANDLE_VALUE;
    }
    else
      m_hMutex=INVALID_HANDLE_VALUE;
  }

  mutex::~mutex()
  {
    destroy();
  }

  mutex &mutex::operator =(const mutex &x)
  {
    if (x.m_hMutex!=INVALID_HANDLE_VALUE)
    {
      if (!DuplicateHandle(GetCurrentProcess(), x.m_hMutex, GetCurrentProcess(), &m_hMutex,
        0, FALSE, DUPLICATE_SAME_ACCESS))
        m_hMutex=INVALID_HANDLE_VALUE;
    }
    else
      m_hMutex=INVALID_HANDLE_VALUE;

    return *this;
  }

  mutex &mutex::operator =(const HANDLE hnd)
  {
    m_hMutex=hnd;
    return *this;
  }

  mutex::operator HANDLE() const
  {
    return m_hMutex;
  }

  sBool mutex::create(sBool initial)
  {
    m_hMutex=CreateMutex(0, initial, 0);
    if (!m_hMutex)
    {
      m_hMutex=INVALID_HANDLE_VALUE;
      return sFALSE;
    }
    else
      return sTRUE;
  }

  sBool mutex::create(const sChar *name, sBool initial)
  {
    m_hMutex=CreateMutex(0, initial, name);
    if (!m_hMutex)
    {
      m_hMutex=INVALID_HANDLE_VALUE;
      return sFALSE;
    }
    else
      return sTRUE;
  }

  sBool mutex::destroy()
  {
    if (m_hMutex!=INVALID_HANDLE_VALUE)
    {
      CloseHandle(m_hMutex);
      m_hMutex=INVALID_HANDLE_VALUE;
      return sTRUE;
    }
    else
      return sFALSE;
  }

  sBool mutex::get(sU32 timeout)
  {
    if (m_hMutex==INVALID_HANDLE_VALUE)
      return sFALSE;

    return WaitForSingleObject(m_hMutex, timeout)!=WAIT_TIMEOUT;
  }

  sBool mutex::release()
  {
    if (m_hMutex==INVALID_HANDLE_VALUE)
      return sFALSE;

    return !!ReleaseMutex(m_hMutex);
  }

  // ---- event

  event::event()
  {
    m_hEvent=INVALID_HANDLE_VALUE;
  }

  event::event(const event &x)
  {
    if (x.m_hEvent!=INVALID_HANDLE_VALUE)
    {
      if (!DuplicateHandle(GetCurrentProcess(), x.m_hEvent, GetCurrentProcess(),
        &m_hEvent, 0, FALSE, DUPLICATE_SAME_ACCESS))
        m_hEvent=INVALID_HANDLE_VALUE;
    }
    else
      m_hEvent=INVALID_HANDLE_VALUE;
  }

  event::event(const HANDLE hnd)
  {
    if (hnd!=INVALID_HANDLE_VALUE)
    {
      if (!DuplicateHandle(GetCurrentProcess(), hnd, GetCurrentProcess(), &m_hEvent, 0,
        FALSE, DUPLICATE_SAME_ACCESS))
        m_hEvent=INVALID_HANDLE_VALUE;
    }
    else
      m_hEvent=INVALID_HANDLE_VALUE;
  }

  event::~event()
  {
    destroy();
  }

  event &event::operator =(const event &x)
  {
    if (x.m_hEvent!=INVALID_HANDLE_VALUE)
    {
      if (!DuplicateHandle(GetCurrentProcess(), x.m_hEvent, GetCurrentProcess(), &m_hEvent,
        0, FALSE, DUPLICATE_SAME_ACCESS))
        m_hEvent=INVALID_HANDLE_VALUE;
    }
    else
      m_hEvent=INVALID_HANDLE_VALUE;

    return *this;
  }

  event &event::operator =(const HANDLE hnd)
  {
    m_hEvent=hnd;
    return *this;
  }

  event::operator HANDLE() const
  {
    return m_hEvent;
  }

  sBool event::create(sBool initial)
  {
    m_hEvent=CreateEvent(0, TRUE, initial, 0);
    if (!m_hEvent)
    {
      m_hEvent=INVALID_HANDLE_VALUE;
      return sFALSE;
    }
    else
      return sTRUE;
  }

  sBool event::create(const sChar *name, sBool initial)
  {
    m_hEvent=CreateEvent(0, TRUE, initial, name);
    if (!m_hEvent)
    {
      m_hEvent=INVALID_HANDLE_VALUE;
      return sFALSE;
    }
    else
      return sTRUE;
  }

  sBool event::destroy()
  {
    if (m_hEvent!=INVALID_HANDLE_VALUE)
    {
      CloseHandle(m_hEvent);
      m_hEvent=INVALID_HANDLE_VALUE;
      return sTRUE;
    }
    else
      return sFALSE;
  }

  sBool event::wait(sU32 timeout)
  {
    if (m_hEvent==INVALID_HANDLE_VALUE)
      return sFALSE;

    return WaitForSingleObject(m_hEvent, timeout)!=WAIT_TIMEOUT;
  }

  void event::set()
  {
    SetEvent(m_hEvent);
  }

  void event::reset()
  {
    ResetEvent(m_hEvent);
  }

  // ---- rwSync

  rwSync::rwSync()
  {
    m_counter=0;
    m_writing.create(sFALSE);
    m_counterZero.create(sTRUE);
  }

  sBool rwSync::requestRead(sU32 timeout)
  {
    if (!m_writing.get(timeout)) // Readaccess wird geblockt, bis keiner mehr schreibt
      return sFALSE;

    m_counterZero.reset();
    InterlockedIncrement((long *) &m_counter); // Lesecounter inkrementieren

    m_writing.release();

    return sTRUE;
  }

  void rwSync::releaseRead()
  {
    if (m_counter>0)
    {
      InterlockedDecrement((long *) &m_counter);
      if (m_counter==0)
        m_counterZero.set();
    }
  }

  sBool rwSync::requestWrite(sU32 timeout)
  {
    HANDLE waits[2]={ m_counterZero, m_writing };

    return WaitForMultipleObjects(2, waits, TRUE, timeout)!=WAIT_TIMEOUT;
  }

  void rwSync::releaseWrite()
  {
    m_writing.release();
  }
}

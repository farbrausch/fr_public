/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_BASE_SYSTEM_WIN_HPP
#define FILE_ALTONA2_LIBS_BASE_SYSTEM_WIN_HPP

#include "Altona2/Libs/Base/Base.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class sThreadPrivate
{
protected:
  void *ThreadHandle;
};

class sThreadLockPrivate
{
protected:
  void *CriticalSection;
};

class sThreadEventPrivate
{
protected:
  void *EventHandle;
};

class sSharedMemoryPrivate
{
protected:
  bool Master;
  bool Initialized;
  void *FileMapping;
  void *Memory;
  sArray<void *> MasterToSlave;
  sArray<void *> SlaveToMaster;
  sArray<void *> SectionMutex;
};

namespace Private {

struct sWinMessage
{
    void *Window;
    uint Message;
    uint wparam;
    sptr lparam;
};

extern sHook1<const sWinMessage &> sWinMessageHook;

enum sMouseLockId
{
    sMLI_None = 0,
    sMLI_Mouse,
    sMLI_Wintab,            // Wacom
};

bool AquireMouseLock(sMouseLockId mouselockid);
bool TestMouseLock(sMouseLockId mouselockid);
void ReleaseMouseLock(sMouseLockId mouselockid);
}

/****************************************************************************/

}

#endif  // FILE_ALTONA2_LIBS_BASE_SYSTEM_WIN_HPP

/*+**************************************************************************/
/***                                                                      ***/
/***   Copyright (C) 2005-2006 by Dierk Ohlerich                          ***/
/***   all rights reserverd                                               ***/
/***                                                                      ***/
/***   To license this software, please contact the copyright holder.     ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "..\..\..\..\altona\main\base\system_xsi.hpp"

/****************************************************************************/

sThreadContext sEmergencyThreadContext;

void sInitMem1()
{
}

void sExitMem1()
{
}

void sRender3DFlush()
{
}

void sInitEmergencyThread()
{
  sEmergencyThreadContext.ThreadName = L"MainThread";
  sEmergencyThreadContext.MemTypeStack[0] = sAMF_HEAP;
  sEmergencyThreadContext.TlsSize = sizeof(sEmergencyThreadContext);
}

sThreadLock::sThreadLock() {}
sThreadLock::~sThreadLock() {}
void sThreadLock::Lock() {  sFatal(L"not implemented"); }
void sThreadLock::Unlock() {  sFatal(L"not implemented"); }
sBool sThreadLock::TryLock() { return 0; }
struct sThreadContext *sGetThreadContext(void) { return &sEmergencyThreadContext; }
void sPrint(const sChar *text) {}
void sTriggerEvent(sInt) { sFatal(L"not implemented"); }

sU8 *sFile::MapAll() { sFatal(L"not implemented"); }

/****************************************************************************/


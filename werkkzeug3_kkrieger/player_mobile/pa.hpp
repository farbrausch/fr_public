// This file is distributed under a BSD license. See LICENSE.txt for details.

#pragma once
#include "_types.hpp"
#include "start_mobile.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Performance Analysis                                               ***/
/***                                                                      ***/
/****************************************************************************/

#if sMOBILE

extern struct PAEntry *PAFirst;

struct PAEntry
{
  sChar *Name;
  sInt Time;
  PAEntry *Next;
  
  PAEntry(sChar *name) { Name=name; Time=0; Next=PAFirst; PAFirst=this; }
  void Enter() { Time -= sGetTime(); }
  void Leave() { Time += sGetTime(); }
};

struct PAScope
{
  PAEntry *pa;
  PAScope(PAEntry &p) { pa = &p; pa->Enter(); }
  ~PAScope() { pa->Leave(); }
};

#define PA(name) static PAEntry PAE(name); PAScope PAS(PAE);
//#define PA(name)

/****************************************************************************/
 
void InitPA();
void ExitPA();
void PrintPA(class SoftEngine *soft);

/****************************************************************************/

#else

#define PA(x)

#endif

/****************************************************************************/

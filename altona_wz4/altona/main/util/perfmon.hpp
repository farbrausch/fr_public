/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_UTIL_PERFMON_HPP
#define FILE_UTIL_PERFMON_HPP

#include "base/types.hpp"
struct sThreadContext;

#define sPERFMON_ENABLED (!sSTRIPPED)

/****************************************************************************/

#if sPERFMON_ENABLED

// put this into sMain()
void sAddPerfMon(sInt maxThreads=8, sInt maxEvents=8000, sInt maxValues=100, sInt maxSwitches=40);

// returns if perf mon is running
sBool sPerfMonInited();

// sets the maximum and frame time for the bar graph
void sSetPerfMonTimes(sF32 maxtime, sF32 frametime);

// for turning perf mon display on/off
void sShowPerfMon(sBool show);
void sTogglePerfMon();

// put this into your OnInput (or filter the events beforehand). 
// If this function returns sTRUE, the input event has done something.
sBool sSendPerfMonInput(const sInput2Event &ie);


/****************************************************************************/
/***                                                                      ***/
/*** Thread handling                                                      ***/
/***                                                                      ***/
/****************************************************************************/

// main thread is added automatically
void sPerfAddThread(sThreadContext *tid=0);
void sPerfRemThread(sThreadContext *tid=0);

/****************************************************************************/
/***                                                                      ***/
/*** Function/Scope tracking                                              ***/
/***                                                                      ***/
/****************************************************************************/

// function/scope/range start
void sPerfEnter(const sChar *name, sU32 color, sThreadContext * tid=0);
void sPerfEnter(const sChar8 *name, sU32 color, sThreadContext * tid=0);

// function/scope/range end
void sPerfLeave(sThreadContext * tid=0);

// for things you don't know the end of
void sPerfSet(const sChar *name, sU32 color, sThreadContext * tid=0);

// helper class for scopes
class sPerfScope
{
public:
  inline sPerfScope(const sChar *name, sU32 color) { sPerfEnter(name,color); }
  inline sPerfScope(const sChar8 *name, sU32 color) { sPerfEnter(name,color); }
  inline ~sPerfScope() { sPerfLeave(); }
};
#define sPERF_SCOPE__(name,color,line) sPerfScope _perfdummy##line(name,color)
#define sPERF_SCOPE_(name,color,line) sPERF_SCOPE__(name,color,line)
#define sPERF_SCOPE(name,color) sPERF_SCOPE_(name,color,__LINE__)

// put this at the beginning of a function to track it
#define sPERF_FUNCTION__(color,line) sPerfScope _perfdummy##line(__FUNCTION__,color)
#define sPERF_FUNCTION_(color,line) sPERF_FUNCTION__(color,line)
#define sPERF_FUNCTION(color) sPERF_FUNCTION_(color,__LINE__)

// put sPERF_SCOPE_MUTE in the scope where you don't want sPERF_SCOPEs anymore
// do not mix sPERF_SCOPE and sPERF_SCOPE_MUTE in the same scope because i (bjoern) 
// don't know about destruction standards
// bad:
// {
//    sPERF_SCOPE
//    sPERF_SCOPE_MUTE
//    sPERF_SCOPE
// }
// good:
// {
//    sPERF_SCOPE
//    {
//      sPERF_SCOPE_MUTE
//      {
//        sPERF_SCOPE  // this will be muted
//      }
//    }
// }
// scope start/end
void sPerfMuteEnter(sThreadContext * tid=0);
void sPerfMuteLeave(sThreadContext * tid=0);

class sPerfScopeMute
{
public:
  inline sPerfScopeMute()   { sPerfMuteEnter(); }
  inline ~sPerfScopeMute()  { sPerfMuteLeave(); }
};

#define sPERF_SCOPE_MUTE__(line) sPerfScopeMute _perfmutedummy##line
#define sPERF_SCOPE_MUTE_(line) sPERF_SCOPE_MUTE__(line)
#define sPERF_SCOPE_MUTE() sPERF_SCOPE_MUTE_(__LINE__)


/****************************************************************************/
/***                                                                      ***/
/*** Value tracking                                                       ***/
/***                                                                      ***/
/****************************************************************************/

// submits an integer variable for tracking. if you've got many values
// to track, give them the same group id and they'll be sorted nicely.
// the display supports overflow/underflow, so choose min/max accordingly :)
void sPerfAddValue(const sChar *name, const sInt *ptr, sInt min, sInt max, sInt group=-1);

// removes variable from tracking
void sPerfRemValue(const sInt *ptr);


/****************************************************************************/
/***                                                                      ***/
/*** Debugging switches                                                   ***/
/***                                                                      ***/
/****************************************************************************/

// adds debug switch. Specify a name, a choice (like "off|on") and an integer
// variable to store the result in.
void sPerfAddSwitch(const sChar *name, const sChar *choice, sInt *ptr);

void sPerfRemSwitch(sInt *ptr);

/****************************************************************************/
/***                                                                      ***/
/*** Internal API for HTTP debugging                                      ***/
/***                                                                      ***/
/****************************************************************************/

void sPerfIntGetValue(sInt index, const sChar *&name, sInt &value, sBool &groupstart);

void sPerfIntGetSwitch(sInt index, const sChar *&name, const sChar *&choice, sInt &value);
void sPerfIntSetSwitch(const sChar *name, sInt newvalue);

#else

class sPerfScope {};

#define sAddPerfMon
#define sShowPerfMon(x)
#define sTogglePerfMon() {}
#define sSetPerfMonTimes(x,y)
#define sSendPerfMonInput(x) 0

#define sPerfMonInited() 0

#define sPerfAddThread
#define sPerfRemThread

#define sPerfEnter(x,y)
#define sPerfLeave()
#define sPerfSet(x,y)
#define sPERF_FUNCTION(x)
#define sPERF_SCOPE(name,color)
#define sPERF_SCOPE_MUTE()

#define sPerfAddValue
#define sPerfRemValue(x)

#define sPerfAddSwitch(x,y,z)
#define sPerfRemSwitch(x);

#endif

/****************************************************************************/

#endif // FILE_UTIL_PERFMON_HPP

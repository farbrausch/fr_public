// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __STARTCONSOLE_HPP__
#define __STARTCONSOLE_HPP__

/****************************************************************************/
/***                                                                      ***/
/***   Forwards                                                           ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/
/***                                                                      ***/
/***   Main Program                                                       ***/
/***                                                                      ***/
/****************************************************************************/

// implement this to write a program

sInt sAppMain(sInt argc,sChar **argv);

/****************************************************************************/

struct sSystem_
{
// init/exit/debug

  void Log(sChar *);                                      // print out debug string
  sNORETURN void Abort(sChar *msg);                       // terminate now
  void Tag();                                             // called by broker (in a direct hackish fashion)

// console io

  void PrintF(const sChar *format,...);

// file

  sU8 *LoadFile(const sChar *name,sInt &size);                  // load file entirely, return size
  sU8 *LoadFile(const sChar *name);                             // load file entirely
  sChar *LoadText(const sChar *name);                           // load file entirely and add trailing zero
  sBool SaveFile(const sChar *name,const sU8 *data,sInt size);  // save file entirely

// misc
  sInt GetTime();
};

extern sSystem_ *sSystem;

#endif

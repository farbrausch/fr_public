/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_PACKFILEGEN_HPP
#define FILE_WZ4FRLIB_PACKFILEGEN_HPP

#include "base/types.hpp"
#include "base/types2.hpp"

/****************************************************************************/

struct sPackFileCreateEntry
{
  sPoolString Name;
  sBool PackEnable;

  sPackFileCreateEntry() : PackEnable(sTRUE) {}
  sPackFileCreateEntry(sPoolString name, sBool enable=sTRUE) : Name(name), PackEnable(enable) {}

};

/****************************************************************************/

// logs all files that are opened via sCreateFile into an array
void sAddFileLogger(sStaticArray<sPackFileCreateEntry> &files);
void sRemFileLogger();

/****************************************************************************/

void sCreateDemoPackFile(const sChar *packfilename, const sArray<sPackFileCreateEntry> files, sBool logfile);

/****************************************************************************/

#endif // FILE_EXTRA_PACKFILEGEN_HPP


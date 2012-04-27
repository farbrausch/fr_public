/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_WZ4FRLIB_PACKFILE_HPP
#define FILE_WZ4FRLIB_PACKFILE_HPP

#include "base/types.hpp"
#include "base/system.hpp"

/****************************************************************************/

class sDemoPackFile : public sFileHandler
{
  struct PackHeader;

  sInt Count;
  PackHeader *Dir;
  sU8 *Data;
  sFile *File;

public:
  sDemoPackFile(const sChar *name);
  ~sDemoPackFile();

  sFile *Create(const sChar *name,sFileAccess access);
};


extern sBool sProgressDoCallBeginEnd;
void sProgressBegin();
void sProgressEnd();
void sProgress(sInt done,sInt max);

/****************************************************************************/


/****************************************************************************/

#endif // FILE_EXTRA_PACKFILE_HPP


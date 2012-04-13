// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __MAPFILE_HPP_
#define __MAPFILE_HPP_

#include "debuginfo.hpp"

/****************************************************************************/

class MAPFileReader : public DebugInfoReader
{
  struct Section;
  sArray<Section> Sections;

  sInt ScanString(sChar *&string,DebugInfo &to);
  static sBool IsHexString(const sChar *str,sInt count);

  Section *GetSection(sInt num,sU32 offs);

public:
  sBool ReadDebugInfo(sChar *fileName,DebugInfo &to);
};

/****************************************************************************/

#endif
